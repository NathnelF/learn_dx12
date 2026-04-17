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
    state->camera.target = { 0.0f, 0.0f, 0.0f };
    state->camera.yaw = 45.0f;
    state->camera.pitch = 45.0f;
    state->camera.distance = 60.0f;

    state->camera.pan_speed = 50.0f;
    state->camera.zoom_speed = 20.0f;
    state->camera.rotate_speed = 180.0f;

    debug("created camera buffer");
}

static HMM_Vec3 CameraGetForward(Camera *cam)
{
    float yaw_rad = HMM_AngleDeg(cam->yaw);
    HMM_Vec3 forward =
      HMM_NormV3({ HMM_SinF(yaw_rad), 0.0f, HMM_CosF(yaw_rad) });
    return forward;
}

static HMM_Vec3 CameraGetRight(Camera *cam, HMM_Vec3 forward)
{
    HMM_Vec3 up = { 0.0f, 1.0f, 0.0f };
    HMM_Vec3 right = HMM_NormV3(HMM_Cross(forward, up));
    return right;
}

void UpdateCamera(State *state, float dt)
{
    // TODO(Nate): implement
    Camera *camera = &state->camera;
    const bool *keys = SDL_GetKeyboardState(NULL);

    HMM_Vec3 forward = CameraGetForward(camera);
    HMM_Vec3 right = CameraGetRight(camera, forward);

    float pan = camera->pan_speed * dt;

    // move with keys
    if (keys[SDL_SCANCODE_W])
    {
        debug("forward");
        camera->target = HMM_AddV3(camera->target, HMM_MulV3F(forward, pan));
    }
    if (keys[SDL_SCANCODE_S])
    {
        debug("back");
        camera->target = HMM_AddV3(camera->target, HMM_MulV3F(forward, -pan));
    }

    if (keys[SDL_SCANCODE_D])
    {
        debug("right");
        camera->target = HMM_AddV3(camera->target, HMM_MulV3F(right, pan));
    }
    if (keys[SDL_SCANCODE_A])
    {
        debug("left");
        camera->target = HMM_AddV3(camera->target, HMM_MulV3F(right, -pan));
    }

    // Zoom

    float zoom = dt * camera->zoom_speed;

    if (keys[SDL_SCANCODE_R])
    {
        camera->distance += pan;
    }
    if (keys[SDL_SCANCODE_F])
    {
        camera->distance -= pan;
    }
    camera->distance = HMM_Clamp(5.0f, camera->distance, 80.0f);

    // Rotate
    // --- Orbit (arrow keys or RF) ---
    float rot = camera->rotate_speed * dt;

    if (keys[SDL_SCANCODE_Q])
        camera->yaw += rot;
    if (keys[SDL_SCANCODE_E])
        camera->yaw -= rot;

    // Clamp pitch so camera doesn't flip — your raylib version had fixed 45deg,
    // expose it here so you can add controls later
    camera->pitch = HMM_Clamp(5.0f, camera->pitch, 89.0f);

    // --- Rebuild position from spherical coords, same math as your raylib ---
    float yaw_rad = HMM_AngleDeg(camera->yaw);
    float pitch_rad = HMM_AngleDeg(camera->pitch);

    // Start with offset along -Z, then rotate by pitch then yaw (your exact
    // logic)
    HMM_Vec3 offset = { 0.0f, 0.0f, -camera->distance };

    // Rotate by pitch around X axis
    float cp = HMM_CosF(pitch_rad), sp = HMM_SinF(pitch_rad);
    offset = {
        offset.X,
        offset.Y * cp - offset.Z * sp,
        offset.Y * sp + offset.Z * cp,
    };

    // Rotate by yaw around Y axis
    float cy = HMM_CosF(yaw_rad), sy = HMM_SinF(yaw_rad);
    offset = {
        offset.X * cy + offset.Z * sy,
        offset.Y,
        -offset.X * sy + offset.Z * cy,
    };

    HMM_Vec3 position = HMM_AddV3(camera->target, offset);

    // --- Build MVP and upload ---
    HMM_Vec3 up = { 0.0f, 1.0f, 0.0f };
    HMM_Mat4 view = HMM_LookAt_RH(position, camera->target, up);
    HMM_Mat4 proj = HMM_Perspective_RH_ZO(
      HMM_AngleDeg(35.0f), // matching your raylib fovy
      (float)state->swapchain.width / (float)state->swapchain.height,
      0.1f,
      500.0f);

    HMM_Mat4 model = HMM_M4D(1.0f);
    CameraConstants c = { HMM_MulM4(proj, HMM_MulM4(view, model)) };
    memcpy(state->camera.ptr, &c, sizeof(CameraConstants));
}