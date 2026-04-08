#include "headers.h"

void EnableDebugLayer(State *state)
{
    ID3D12Debug1 *debug_controller;
    validate(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)),
             "could not get debug interface");
    debug_controller->EnableDebugLayer();
    debug_controller->SetEnableGPUBasedValidation(true);
    debug_controller->Release();

    debug("enabled debug layers")
#ifdef DEBUG_BUILD

#endif
}

void CreateFactory(State *state)
{
    u32 flags = 0;
#ifdef DEBUG_BUILD
    flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    validate(CreateDXGIFactory2(flags, IID_PPV_ARGS(&state->context.factory)),
             "could not crate DXGI factory");

    debug("created DXGI factory");
}

void GetAdapter2(State *state)
{
    IDXGIAdapter1 *adapter;
    IDXGIAdapter1 *best_adapter = nullptr;
    SIZE_T best_vram = 0;
    u32 i = 0;

    while (true)
    {
        HRESULT hr = state->context.factory->EnumAdapters1(i++, &adapter);
        if (hr == DXGI_ERROR_NOT_FOUND)
        {
            break;
        }

        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            adapter->Release();
            continue;
        }

        if (FAILED(D3D12CreateDevice(adapter,
                                     D3D_FEATURE_LEVEL_12_0,
                                     __uuidof(ID3D12Device),
                                     nullptr)))
        {
            adapter->Release();
            continue;
        }

        if (desc.DedicatedVideoMemory > best_vram)
        {
            if (best_adapter)
                best_adapter->Release();
            best_adapter = adapter;
            best_vram = desc.DedicatedVideoMemory;
        }
        else
        {
            adapter->Release();
        }
    }

    if (!best_adapter)
    {
        err("failed to find capable d3d12 adapter");
    }

    state->context.adapter = best_adapter;

    DXGI_ADAPTER_DESC1 desc;
    best_adapter->GetDesc1(&desc);
    debug("chose adapter: %ls (%zu MB VRAM)",
          desc.Description,
          best_vram / (1024 * 1024));
}

void GetAdapter(State *state)
{
    IDXGIAdapter1 *adapter;
    u32 i = 0;
    while (true)
    {
        HRESULT hr = state->context.factory->EnumAdapterByGpuPreference(
          i++, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter));
        if (hr == DXGI_ERROR_NOT_FOUND)
        {
            break;
        }
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            adapter->Release();
            continue;
        }
        if (SUCCEEDED(D3D12CreateDevice(adapter,
                                        D3D_FEATURE_LEVEL_12_0,
                                        __uuidof(ID3D12Device),
                                        nullptr)))
        {
            state->context.adapter = adapter;
            debug("chose adapter: %ls", desc.Description);
            return;
        }
        adapter->Release();
    }
    err("failed to find capable d3d12 adapter");
}

void CreateDevice(State *state)
{
    validate(D3D12CreateDevice(state->context.adapter,
                               D3D_FEATURE_LEVEL_12_0,
                               IID_PPV_ARGS(&state->context.device)),
             "could not create d3d12 device");

    debug("created device");

#ifdef DEBUG_BUILD
    // info queue callback to our window
    ID3D12InfoQueue1 *info_queue;
    if (SUCCEEDED(
          state->context.device->QueryInterface(IID_PPV_ARGS(&info_queue))))
    {
        DWORD cookie;
        info_queue->RegisterMessageCallback(
          [](D3D12_MESSAGE_CATEGORY category,
             D3D12_MESSAGE_SEVERITY severity,
             D3D12_MESSAGE_ID id,
             LPCSTR description,
             void *context) { fprintf(stderr, "[DX12]: %s\n", description); },
          D3D12_MESSAGE_CALLBACK_FLAG_NONE,
          nullptr,
          &cookie);

        info_queue->Release();
    }
    debug("routed info queue to window");
#endif
}

void CreateQueue(State *state)
{
    D3D12_COMMAND_QUEUE_DESC description = {
        .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
        .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
        .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
    };

    validate(state->context.device->CreateCommandQueue(
               &description, IID_PPV_ARGS(&state->context.queue)),
             "could not create command queue");

    debug("created command queue");
}

void CreateFence(State *state)
{
    // create global fence for frames in flight synchronization
    validate(
      state->context.device->CreateFence(
        0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&state->context.fence.handle)),
      "could not create fence");

    state->context.fence.value = 0;

    state->context.fence.event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!state->context.fence.event)
    {
        err("could not create fence event");
    }

    debug("created fence");
}

void CreateFrameContext(State *state)
{
    // fill out per frame data to be used in render loop for frames in flight
    for (u32 i = 0; i < FRAMES; i++)
    {
        validate(state->context.device->CreateCommandAllocator(
                   D3D12_COMMAND_LIST_TYPE_DIRECT,
                   IID_PPV_ARGS(&state->context.frames[i].allocator)),
                 "could not create allocator");

        validate(state->context.device->CreateCommandList(
                   0,
                   D3D12_COMMAND_LIST_TYPE_DIRECT,
                   state->context.frames[i].allocator,
                   NULL,
                   IID_PPV_ARGS(&state->context.frames[i].command_list)),
                 "could not create command list");

        state->context.frames[i].command_list->Close();
        state->context.frames[i].fence_value = 0;

        debug("created frame context %u", i);
    }
}

void CreateWindow1(State *state)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        err("could not initialize SDL");
    }

    state->context.window =
      SDL_CreateWindow("Pilfer", 800, 600, SDL_WINDOW_RESIZABLE);

    if (state->context.window == NULL)
    {
        err("failed to create SDL window");
    }

    SDL_PropertiesID properties =
      SDL_GetWindowProperties(state->context.window);

    state->context.window_handle = (HWND)SDL_GetPointerProperty(
      properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);

    if (state->context.window_handle == NULL)
    {
        err("could not get window handle");
    }

    int width, height;
    SDL_GetWindowSize(state->context.window, &width, &height);
    state->swapchain.width = (u32)width;
    state->swapchain.height = (u32)height;

    debug("created window");
}

void CreateSwapchain(State *state)
{
    DXGI_SWAP_CHAIN_DESC1 desc = {
		.Width = state->swapchain.width,
		.Height = state->swapchain.height,
		.Format = DXGI_FORMAT_B8G8R8A8_UNORM,
		.SampleDesc = {
			.Count = 1,
		},
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = 2,
		.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
	};

    IDXGISwapChain1 *swapchain1;
    validate(state->context.factory->CreateSwapChainForHwnd(
               state->context.queue,
               state->context.window_handle,
               &desc,
               NULL,
               NULL,
               &swapchain1),
             "could not create swapchain 1");

    validate(
      swapchain1->QueryInterface(IID_PPV_ARGS(&state->context.swapchain)),
      "could not update swapchain one to three");

    swapchain1->Release();

    debug("created swapchain");
}

void CreateContext(State *state)
{
    EnableDebugLayer(state);
    CreateFactory(state);
    GetAdapter2(state);
    CreateDevice(state);
    CreateQueue(state);
    CreateFence(state);
    CreateFrameContext(state);
    CreateWindow1(state);
    CreateSwapchain(state);
    debug("created context");
}