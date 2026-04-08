#include "headers.h"

void CreateRTVResources(State *state)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        .NumDescriptors = FRAMES,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
    };

    validate(state->context.device->CreateDescriptorHeap(
               &desc, IID_PPV_ARGS(&state->swapchain.rtv_heap)),
             "could not create rtv heap");

    u32 rtv_size = state->context.device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle =
      state->swapchain.rtv_heap->GetCPUDescriptorHandleForHeapStart();

    for (u32 i = 0; i < FRAMES; i++)
    {
        validate(state->context.swapchain->GetBuffer(
                   i, IID_PPV_ARGS(&state->swapchain.buffers[i])),
                 "could not get swapchain buffer");

        state->context.device->CreateRenderTargetView(
          state->swapchain.buffers[i], NULL, rtv_handle);

        state->swapchain.rtv_handles[i] = rtv_handle;

        rtv_handle.ptr += rtv_size;
    }

    debug("created RTV resources");
}

void CreateDSVResources(State *state)
{
    D3D12_RESOURCE_DESC depth_desc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		.Width = state->swapchain.width,
		.Height = state->swapchain.height,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = DXGI_FORMAT_D32_FLOAT,
		.SampleDesc = { 
			.Count = 1,
		},
		.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
	};

    D3D12_CLEAR_VALUE clear_value = {
		.Format = DXGI_FORMAT_D32_FLOAT,
		.DepthStencil = {
			.Depth = 1.0f,
			.Stencil = 0,
		},
	};

    D3D12_HEAP_PROPERTIES heap_properties = {
        .Type = D3D12_HEAP_TYPE_DEFAULT,
    };

    // create the depth buffer
    validate(state->context.device->CreateCommittedResource(
               &heap_properties,
               D3D12_HEAP_FLAG_NONE,
               &depth_desc,
               D3D12_RESOURCE_STATE_DEPTH_WRITE,
               &clear_value,
               IID_PPV_ARGS(&state->swapchain.depth_buffer)),
             "could not create depth buffer");

    // create the dsv heap
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        .NumDescriptors = 1,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
    };

    validate(state->context.device->CreateDescriptorHeap(
               &heap_desc, IID_PPV_ARGS(&state->swapchain.dsv_heap)),
             "could not create dsv heap");

    state->swapchain.dsv_handle =
      state->swapchain.dsv_heap->GetCPUDescriptorHandleForHeapStart();

    state->context.device->CreateDepthStencilView(
      state->swapchain.depth_buffer, NULL, state->swapchain.dsv_handle);

    debug("created DSV resources");
}

void CreateSwapchainResources(State *state)
{
    CreateRTVResources(state);
    CreateDSVResources(state);
}

void WaitForGPUIdle(State *state)
{
    u64 wait_value = ++state->context.fence.value;
    state->context.queue->Signal(state->context.fence.handle, wait_value);
    state->context.fence.handle->SetEventOnCompletion(
      wait_value, state->context.fence.event);
    WaitForSingleObject(state->context.fence.event, INFINITE);
}

void ReleaseSwapchainResources(State *state)
{
    for (u32 i = 0; i < FRAMES; i++)
    {
        if (state->swapchain.buffers[i])
        {
            state->swapchain.buffers[i]->Release();
            state->swapchain.buffers[i] = NULL;
        }
    }

    if (state->swapchain.depth_buffer)
    {
        state->swapchain.depth_buffer->Release();
        state->swapchain.depth_buffer = NULL;
    }

    if (state->swapchain.rtv_heap)
    {
        state->swapchain.rtv_heap->Release();
        state->swapchain.rtv_heap = NULL;
    }

    if (state->swapchain.dsv_heap)
    {
        state->swapchain.dsv_heap->Release();
        state->swapchain.dsv_heap = NULL;
    }
}

void RecreateSwapchain(State *state)
{
    WaitForGPUIdle(state);
    ReleaseSwapchainResources(state);

    int width, height;
    SDL_GetWindowSize(state->context.window, &width, &height);
    state->swapchain.width = (u32)width;
    state->swapchain.height = (u32)height;

    validate(state->context.swapchain->ResizeBuffers(FRAMES,
                                                     state->swapchain.width,
                                                     state->swapchain.height,
                                                     DXGI_FORMAT_B8G8R8A8_UNORM,
                                                     0),
             "could not resize swapchain buffers");

    CreateSwapchainResources(state);
    debug("recreated swapchain %ux%u",
          state->swapchain.width,
          state->swapchain.height);
}