#include "headers.h"

void CreateCameraBuffer(State *state)
{
    D3D12_HEAP_PROPERTIES props = {
        .Type = D3D12_HEAP_TYPE_UPLOAD,
    };

    D3D12_RESOURCE_DESC desc = {
    	.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
    	.Width = 256,
    	.Height = 1,
    	.DepthOrArraySize = 1,
    	.MipLevels = 1,
    	.Format = DXGI_FORMAT_UNKNOWN,
    	.SampleDesc = {
    		.Count = 1,
    	},
    	.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
    };

    validate(state->context.device->CreateCommittedResource(
               &props,
               D3D12_HEAP_FLAG_NONE,
               &desc,
               D3D12_RESOURCE_STATE_GENERIC_READ,
               NULL,
               IID_PPV_ARGS(&state->camera.buffer)),
             "could not create camera buffer");

    // Map it
    D3D12_RANGE range = { 0, 0 };
    state->camera.buffer->Map(0, &range, &state->camera.ptr);

    // Init position
    state->camera.position = { 0.0f, 0.0f, -3.0f };
    state->camera.yaw = 0.0f;
    state->camera.pitch = 0.0f;
    state->camera.speed = 5.0f;
    state->camera.sensitivity = 0.002f;

    debug("created camera buffer");
}

void UpdateCamera(State *state, float dt)
{
    // TODO(Nate): implement
    Camera *camera = &state->camera;
    const bool *keys = SDL_GetKeyboardState(NULL);

    HMM_Vec3 forward = {
        HMM_SinF(camera->yaw),
        0.0f,
        HMM_CosF(camera->yaw),
    };

    HMM_Vec3 right = {
        HMM_CosF(camera->yaw),
        0.0f,
        -HMM_SinF(camera->yaw),
    };

    float speed = camera->speed * dt;

    // move with keys
    if (keys[SDL_SCANCODE_W])
    {
        debug("forward");
        camera->position =
          HMM_AddV3(camera->position, HMM_MulV3F(forward, speed));
    }
    if (keys[SDL_SCANCODE_S])
    {
        debug("back");
        camera->position =
          HMM_AddV3(camera->position, HMM_MulV3F(forward, -speed));
    }

    if (keys[SDL_SCANCODE_D])
    {
        debug("right");
        camera->position =
          HMM_AddV3(camera->position, HMM_MulV3F(right, speed));
    }
    if (keys[SDL_SCANCODE_A])
    {
        debug("left");
        camera->position =
          HMM_AddV3(camera->position, HMM_MulV3F(right, -speed));
    }
    if (keys[SDL_SCANCODE_Q])
    {
        camera->position.Y += speed;
    }
    if (keys[SDL_SCANCODE_E])
    {
        camera->position.Y -= speed;
    }

    // Mouse pan
    float mouse_dx, mouse_dy;
    SDL_GetRelativeMouseState(&mouse_dx, &mouse_dy);
    camera->yaw += mouse_dx * camera->sensitivity;
    camera->pitch -= mouse_dy * camera->sensitivity;
    camera->pitch = HMM_Clamp(-1.5f, camera->pitch, 1.5f); // ~85 degrees

    // Build look direction from yaw + pitch
    HMM_Vec3 look_dir = { HMM_CosF(camera->pitch) * HMM_SinF(camera->yaw),
                          HMM_SinF(camera->pitch),
                          HMM_CosF(camera->pitch) * HMM_CosF(camera->yaw) };

    HMM_Vec3 target = HMM_AddV3(camera->position, look_dir);
    HMM_Vec3 up = { 0.0f, 1.0f, 0.0f };

    HMM_Mat4 view = HMM_LookAt_RH(camera->position, target, up);
    HMM_Mat4 proj = HMM_Perspective_RH_ZO(HMM_AngleDeg(75.0f),
                                          (float)state->swapchain.width /
                                            (float)state->swapchain.height,
                                          0.01f,
                                          1000.0f);

    // Identity model for now, combine later per-object
    HMM_Mat4 model = HMM_M4D(1.0f);
    CameraConstants constants = { HMM_MulM4(proj, HMM_MulM4(view, model)) };

    memcpy(state->camera.ptr, &constants, sizeof(CameraConstants));
}