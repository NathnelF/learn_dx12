#include "headers.h"

struct RawMesh
{
    float *vertex_staging; // CPU side accumulation buffer for all vertices
    u32 *index_staging;    // CPU side accumulation buffer for all indices
    u32 vertex_bytes_used;
    u32 index_bytes_used;
};

void CreateMegaBuffer(State *state)
{
    D3D12_RESOURCE_DESC buffer_desc = {
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Width = MEGA_BUFFER_SIZE,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = { .Count = 1 },
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = D3D12_RESOURCE_FLAG_NONE,
    };

    D3D12_HEAP_PROPERTIES heap_props = {
        .Type = D3D12_HEAP_TYPE_DEFAULT,
    };

    validate(state->context.device->CreateCommittedResource(
               &heap_props,
               D3D12_HEAP_FLAG_NONE,
               &buffer_desc,
               D3D12_RESOURCE_STATE_COMMON,
               NULL,
               IID_PPV_ARGS(&state->mesh_data.buffer)),
             "could not create megabuffer");

    state->mesh_data.vertex_write_position = 0;
    state->mesh_data.index_write_position = MEGA_BUFFER_SIZE / 2;
    state->mesh_data.mesh_count = 0;

    debug("created a mega buffer (%llu megabytes)",
          MEGA_BUFFER_SIZE / 1024 / 1024);
}

u32 LoadMesh(State *state, RawMesh *upload_mesh, const char *path)
{
    cgltf_options options = {};
    cgltf_data *data = NULL;

    cgltf_result result = cgltf_parse_file(&options, path, &data);
    if (result != cgltf_result_success)
    {
        err("could not parse glb file %s", path);
    }

    result = cgltf_load_buffers(&options, data, path);
    if (result != cgltf_result_success)
    {
        cgltf_free(data);
        err("could not load glb buffers %s", path);
    }

    if (data->meshes_count == 0 || data->meshes[0].primitives_count == 0)
    {
        cgltf_free(data);
        err("no mesh data found %s", path);
    }

    cgltf_primitive *primitive = &data->meshes[0].primitives[0];

    cgltf_accessor *position_accessor = NULL;
    for (u32 i = 0; i < primitive->attributes_count; i++)
    {
        if (primitive->attributes[i].type == cgltf_attribute_type_position)
        {
            position_accessor = primitive->attributes[i].data;
            break;
        }
    }

    if (position_accessor == NULL)
    {
        cgltf_free(data);
        err("could not find position accessor %s", path);
    }

    u32 vertex_count = (u32)position_accessor->count;
    // each vertex has 3 floats
    u32 float_count = vertex_count * 3;
    u32 vertex_size = vertex_count * 3 * sizeof(float);

    if (upload_mesh->vertex_bytes_used + vertex_size > MEGA_BUFFER_SIZE / 2)
    {
        cgltf_free(data);
        err("vertex staging buffer full %s", path);
    }

    float *vertex_dest = (float *)((u8 *)upload_mesh->vertex_staging +
                                   upload_mesh->vertex_bytes_used);

    cgltf_accessor_unpack_floats(position_accessor, vertex_dest, float_count);

    // indices
    cgltf_accessor *index_accessor = primitive->indices;
    if (index_accessor == NULL)
    {
        cgltf_free(data);
        err("no positon data found in %s", path);
    }

    u32 index_count = (u32)index_accessor->count;
    u32 index_size = index_count * sizeof(u32);

    if (upload_mesh->index_bytes_used + index_size > MEGA_BUFFER_SIZE / 2)
    {
        cgltf_free(data);
        err("index staging buffer full %s:", path);
    }

    u32 *index_dest =
      (u32 *)((u8 *)upload_mesh->index_staging + upload_mesh->index_bytes_used);

    cgltf_accessor_unpack_indices(
      index_accessor, index_dest, sizeof(u32), index_count);

    u32 mesh_index = state->mesh_data.mesh_count;
    state->mesh_data.meshes[mesh_index] = {
        .vertex_offset = state->mesh_data.vertex_write_position,
        .vertex_count = vertex_count,
        .index_offset = state->mesh_data.index_write_position,
        .index_count = index_count,
    };

    state->mesh_data.mesh_count++;
    state->mesh_data.vertex_write_position += vertex_size;
    state->mesh_data.index_write_position += index_size;

    upload_mesh->vertex_bytes_used += vertex_size;
    upload_mesh->index_bytes_used += index_size;
    cgltf_free(data);

    debug("loaded in mesh %s, %u vertices and %u indices with mesh slot %u",
          path,
          vertex_count,
          index_count,
          mesh_index);

    return mesh_index;
}

void UploadMeshToGPU(State *state, RawMesh *upload_mesh)
{
    if (upload_mesh->vertex_bytes_used == 0 &&
        upload_mesh->index_bytes_used == 0)
    {
        err("no mesh data to upload");
    }

    u32 total_size =
      upload_mesh->vertex_bytes_used + upload_mesh->index_bytes_used;

    D3D12_RESOURCE_DESC upload_desc = {
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Width = total_size,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = { .Count = 1 },
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = D3D12_RESOURCE_FLAG_NONE,
    };

    D3D12_HEAP_PROPERTIES upload_heap_properties = {
        .Type = D3D12_HEAP_TYPE_UPLOAD,
    };

    ID3D12Resource *upload_buffer;
    validate(state->context.device->CreateCommittedResource(
               &upload_heap_properties,
               D3D12_HEAP_FLAG_NONE,
               &upload_desc,
               D3D12_RESOURCE_STATE_GENERIC_READ,
               NULL,
               IID_PPV_ARGS(&upload_buffer)),
             "could not create upload buffer");

    void *mapped_ptr;
    D3D12_RANGE read_range = { 0, 0 };
    validate(upload_buffer->Map(0, &read_range, &mapped_ptr),
             "could not create mapped ptr");

    memcpy(
      mapped_ptr, upload_mesh->vertex_staging, upload_mesh->vertex_bytes_used);

    memcpy((u8 *)mapped_ptr + upload_mesh->vertex_bytes_used,
           upload_mesh->index_staging,
           upload_mesh->index_bytes_used);

    upload_buffer->Unmap(0, NULL);

    Frame *frame = &state->context.frames[0];

    validate(frame->allocator->Reset(),
             "could not reset frame 0 cmd allocator");

    validate(frame->command_list->Reset(frame->allocator, NULL),
             "could not reset frame 0 cmd list");

    D3D12_RESOURCE_BARRIER barrier = {
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
        .Transition = {
            .pResource = state->mesh_data.buffer,
            .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            .StateBefore = D3D12_RESOURCE_STATE_COMMON,
            .StateAfter = D3D12_RESOURCE_STATE_COPY_DEST,
        },
    };

    frame->command_list->ResourceBarrier(1, &barrier);

    frame->command_list->CopyBufferRegion(state->mesh_data.buffer,
                                          0,
                                          upload_buffer,
                                          0,
                                          upload_mesh->vertex_bytes_used);

    frame->command_list->CopyBufferRegion(state->mesh_data.buffer,
                                          MEGA_BUFFER_SIZE / 2,
                                          upload_buffer,
                                          upload_mesh->vertex_bytes_used,
                                          upload_mesh->index_bytes_used);

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter =
      D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    frame->command_list->ResourceBarrier(1, &barrier);

    validate(frame->command_list->Close(), "could not close list after upload");

    ID3D12CommandList *lists[] = { frame->command_list };

    state->context.queue->ExecuteCommandLists(1, lists);

    state->context.fence.value++;
    validate(state->context.queue->Signal(state->context.fence.handle,
                                          state->context.fence.value),
             "could not signal fence");

    validate(state->context.fence.handle->SetEventOnCompletion(
               state->context.fence.value, state->context.fence.event),
             "could not set fence event");

    WaitForSingleObject(state->context.fence.event, INFINITE);

    upload_buffer->Release();

    debug("uploaded mesh data to gpu: %u vertex bytes, %u index bytes, %u "
          "total size",
          upload_mesh->vertex_bytes_used,
          upload_mesh->index_bytes_used,
          total_size);
}

void LoadMeshes(State *state)
{
    CreateMegaBuffer(state);

    // CPU side staging buffer
    RawMesh upload_mesh = {};
    upload_mesh.vertex_staging = (float *)malloc(MEGA_BUFFER_SIZE / 2);
    upload_mesh.index_staging = (u32 *)malloc(MEGA_BUFFER_SIZE / 2);

    // Load meshes into staging area
    LoadMesh(state, &upload_mesh, "assets/Cube.glb");

    // TODO(Nate): Eventually we want to load the mesh data into the proper
    // format offline so we can just memcpy a file directly into vram

    // Transfer from staging area to VRAM
    UploadMeshToGPU(state, &upload_mesh);

    debug("Mesh data finished!");
}