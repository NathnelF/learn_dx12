#include "headers.h"

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

    debug("created a mega buffer (%llu mgeabytes)",
          MEGA_BUFFER_SIZE / 1024 / 1024);
}

u32 LoadMesh(State *state, const char *path)
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
    float *positions = (float *)malloc(vertex_size);

    cgltf_accessor_unpack_floats(position_accessor, positions, float_count);

    // indices
    cgltf_accessor *index_accessor = primitive->indices;
    if (index_accessor == NULL)
    {
        free(positions);
        cgltf_free(data);
        err("no positon data found in %s", path);
    }

    u32 index_count = (u32)index_accessor->count;
    u32 index_size = index_count * sizeof(u32);
    u32 *indices = (u32 *)malloc(index_size);

    cgltf_accessor_unpack_indices(
      index_accessor, indices, sizeof(u32), index_count);

    // check for bounds overflow
    if (state->mesh_data.vertex_write_position + vertex_size >
        MEGA_BUFFER_SIZE / 2)
    {
        // too much vertex data
        err("mega buffer vertex region full");
    }

    if (state->mesh_data.index_write_position + index_size > MEGA_BUFFER_SIZE)
    {
        err("mega buffer index region full");
    }

    // TODO(Nate): upload position / indices to mega buffer

    // TODO(Nate): Or dump data to a file later on

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

    free(indices);
    free(positions);
    cgltf_free(data);

    debug("loaded in mesh %s, %u vertices and %u indices with mesh slot %u",
          path,
          vertex_count,
          index_count,
          mesh_index);

    return mesh_index;
}

void LoadMeshes(State *state)
{
    CreateMegaBuffer(state);
    // this is where we will load all the mesh data specified into vram
    // unless we have truly egregious meshes we can probably keep them
    // for the entire application

    // textures will likely have to be streamed though
    // or perhaps we can compress them enough to fit all the needed
    // textures for a level into vram at level load.
}