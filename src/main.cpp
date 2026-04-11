#include "headers.h"
#include <stdio.h>

#include "SDL3/SDL_main.h"

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
    SDL_Event event;
    int running = 1;
    int frame_index = 0;
    while (running)
    {
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
            Render(&state, frame_index);
            frame_index = (frame_index + 1) % FRAMES;
        }
    }
    return 0;
}