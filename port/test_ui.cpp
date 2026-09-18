#include <iostream>
#include <GL/gl.h>
#include <SDL3/SDL.h>
#include "render/settings_ui.hpp"

using namespace melee::render;

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Test", 800, 600, SDL_WINDOW_OPENGL);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    
    init_settings_ui(window, gl_context);
    VideoSettings settings{};
    bool open = true;
    
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            process_settings_event(&event);
        }
        
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        draw_settings_ui(&settings, &open, 60.0);
        SDL_GL_SwapWindow(window);
    }
    
    destroy_settings_ui();
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
