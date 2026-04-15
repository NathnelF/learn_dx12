#include "headers.h"
#include <stdio.h>

#include "SDL3/SDL_main.h"

#include "camera.cpp"
#include "context.cpp"
#include "mesh.cpp"
#include "pipeline.cpp"
#include "render.cpp"
#include "swapchain.cpp"

int main(int argC, char **argV)
{
#ifdef DEBUG_BUILD
    AllocConsole();
    SetConsoleTitle("Debug Console");
    freopen("CONOUT$", "w", stderr);
    freopen("CONOUT$", "w", stdout);
#endif
    puts("hello world");
    State state = {};
    CreateContext(&state);
    CreateSwapchainResources(&state);

    LoadMeshes(&state);
    CreatePipeline(&state);
    CreateCameraBuffer(&state);
    SDL_Event event;
    int running = 1;
    int frame_index = 0;
    SDL_SetWindowRelativeMouseMode(state.context.window, true);

    u64 freq = SDL_GetPerformanceFrequency();
    u64 last = SDL_GetPerformanceCounter();
    while (running)
    {

        u64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - last) / (float)freq;
        last = now;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                debug("quitting");
                running = 0;
            }
            if (event.type == SDL_EVENT_WINDOW_RESIZED)
            {
                RecreateSwapchain(&state);
            }
        }
        UpdateCamera(&state, dt);
        Render(&state, frame_index);
        frame_index = (frame_index + 1) % FRAMES;
    }
    return 0;
}