#include "headers.h"

// This will be the entire visible 3D scene we are rendering.

// Essentially just a collection of mesh indices, and transforms for the moment.

void CreateSceneBuffer(State *state)
{
    // Create buffer for 3D transforms
    // create heap properties
    D3D12_HEAP_PROPERTIES heap = {
        .Type = D3D12_HEAP_TYPE_UPLOAD,
    };

    // create buffer description
    D3D12_RESOURCE_DESC desc = {
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Width = MAX_ENTITIES * sizeof(HMM_Mat4),
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = {
        	.Count = 1,
        },
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
    };

    state->context.device->CreateCommittedResource(
      &heap,
      D3D12_HEAP_FLAG_NONE,
      &desc,
      D3D12_RESOURCE_STATE_GENERIC_READ,
      NULL,
      IID_PPV_ARGS(&state->scene.buffer));

    D3D12_RANGE range = { 0, 0 };
    state->scene.buffer->Map(0, &range, &state->scene.ptr);
}

void RebuildTransform(State *state, u32 index)
{
    HMM_Mat4 translation = HMM_Translate(state->scene.positions[index]);
    HMM_Mat4 rotation = HMM_Rotate_RH(
      HMM_AngleDeg(state->scene.rotations[index]), { 0.0f, 1.0f, 0.0f });
    HMM_Mat4 scale = HMM_Scale(state->scene.scales[index]);
    state->scene.transforms[index] =
      HMM_MulM4(translation, HMM_MulM4(rotation, scale));
}

void CreateEntity(State *state, u32 mesh_index, HMM_Vec3 position)
{
    // default rotation for now
    if (state->scene.entity_count + 1 > MAX_ENTITIES)
    {
        err("Exceeded max entity count");
    }
    u32 count = state->scene.entity_count;
    state->scene.positions[count] = position;
    state->scene.rotations[count] = 0.0f;
    state->scene.scales[count] = { 1.0f, 1.0f, 1.0f };
    state->scene.mesh_indices[count] = mesh_index;

    RebuildTransform(state, count);
    state->scene.entity_count++;
}

void CreateStaticScene(State *state)
{
    for (int i = 0; i < 6; i++)
    {
        CreateEntity(state, 0, { (float)(i * 3), 0.0f, 0.0f });
        CreateEntity(state, 0, { (float)(i * -3), 0.0f, 0.0f });
    }
}

void MoveEntity(State *state, u32 index)
{
    debug("moving entity %u from (%.2f, %.2f, %.2f)\n",
          index,
          state->scene.positions[index].X,
          state->scene.positions[index].Y,
          state->scene.positions[index].Z);

    state->scene.positions[index] = { 0.0f, 6.0f, 0.0f };
    RebuildTransform(state, index);
}