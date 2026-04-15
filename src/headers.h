#pragma once

#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>

#include "HandmadeMath.h"

#include <wrl/client.h>

#include "SDL3/SDL.h"
#include "cgltf.h"

#include "debug.h"
#include "types.h"

#include <windows.h>

#define FRAMES 2

struct Fence
{
    ID3D12Fence *handle;
    u64 value;
    HANDLE event;
};

struct Frame
{
    ID3D12CommandAllocator *allocator;
    ID3D12GraphicsCommandList *command_list;
    u64 fence_value;
};

#define MEGA_BUFFER_SIZE megabytes(128)
#define MAX_MESHES 128

struct MeshInfo
{
    u32 vertex_offset;
    u32 vertex_count;
    u32 index_offset;
    u32 index_count;
};

struct MeshData
{
    ID3D12Resource *buffer;

    MeshInfo meshes[MAX_MESHES];
    u32 mesh_count;

    u32 vertex_write_position;
    u32 index_write_position;
};

struct Context
{
    IDXGIFactory6 *factory; // this is the interface into the hardware systems
    IDXGIAdapter1 *adapter; // this is the gpu
    ID3D12Device2 *device;  // this is the logical device
    ID3D12CommandQueue *queue; // this is our graphics queue
    Frame frames[FRAMES];
    Fence fence;
    SDL_Window *window;
    HWND window_handle;

    IDXGISwapChain3 *swapchain;
};

struct Swapchain
{
    ID3D12Resource *buffers[FRAMES];
    ID3D12DescriptorHeap *rtv_heap;
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handles[FRAMES];
    ID3D12Resource *depth_buffer;
    ID3D12DescriptorHeap *dsv_heap;
    D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle;
    u32 width;
    u32 height;
};

struct Pipeline
{
    ID3D12RootSignature *root_signature;
    ID3D12PipelineState *handle;
    // ID3D12GraphicsCommandList *command_list;

    // ID3D12Fence *fence;
    // u64 fence_value;
    // HANDLE fence_event;

    ID3D12Resource *vertex_buffer;
    D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view;
};

struct Camera
{
    HMM_Vec3 position;
    float yaw;
    float pitch;

    float speed;
    float sensitivity;

    ID3D12Resource *buffer;
    void *ptr;
};

struct CameraConstants
{
    HMM_Mat4 mvp;
};

struct State
{
    Context context;
    Swapchain swapchain;
    Pipeline pipeline;
    MeshData mesh_data;
    Camera camera;
};

#define megabytes(n) ((u64)(n) * 1024 * 1024)