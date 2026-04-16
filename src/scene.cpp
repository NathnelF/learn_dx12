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

void CreateStaticScene(State *state)
{
    HMM_Mat4 rotation = HMM_Rotate_RH(HMM_AngleDeg(0.0f), { 0.0f, 1.0f, 0.0f });
    HMM_Mat4 scale = HMM_Scale({ 1.0f, 1.0f, 1.0f });
    HMM_Mat4 rot_scale = HMM_MulM4(rotation, scale);

    HMM_Vec3 position[5] = {
        { 0.0f, 0.0f, 0.0f }, { 3.0f, 0.0f, 0.0f },  { -3.0f, 0.0f, 0.0f },
        { 0.0f, 3.0f, 0.0f }, { 0.0f, -3.0f, 0.0f },
    };

    for (u32 i = 0; i < 5; i++)
    {
        HMM_Mat4 translation = HMM_Translate(position[i]);
        state->scene.mesh_indices[i] = 0;
        state->scene.transforms[i] = HMM_MulM4(translation, rot_scale);
        state->scene.entity_count += 1;
    }
}