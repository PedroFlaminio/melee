#ifndef MELEE_HOST_SETTINGS_UI_HPP
#define MELEE_HOST_SETTINGS_UI_HPP

#include "render/play_window.hpp"
#include <SDL3/SDL.h>

namespace melee::render {
    void init_settings_ui(SDL_Window* window, SDL_GLContext gl_context);
    void destroy_settings_ui();
    bool process_settings_event(const SDL_Event* event);
    bool draw_settings_ui(VideoSettings* settings, bool* open,
                          double current_fps);
}

#endif
