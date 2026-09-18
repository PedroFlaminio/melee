import re

with open('src/render/sdl_gl_renderer.cpp', 'r') as f:
    content = f.read()

# 1. Include header
content = re.sub(r'#include "render/play_window.hpp"', '#include "render/play_window.hpp"\n#include "render/settings_ui.hpp"', content)

# 2. FramePresenter::open
open_code = """        if (!hidden) {
            apply_window_mode(*state_);
        }
        init_settings_ui(state_->window.window, state_->window.context);
        return true;"""
content = re.sub(r'        if \(!hidden\) \{\n            apply_window_mode\(\*state_\);\n        \}\n        return true;', open_code, content)

# 3. FramePresenter::~FramePresenter
dtor_code = """        if (state_->audio != nullptr) {
            SDL_DestroyAudioStream(state_->audio);
        }
        destroy_settings_ui();
        close_gl_window(&state_->window);"""
content = re.sub(r'        if \(state_->audio != nullptr\) \{\n            SDL_DestroyAudioStream\(state_->audio\);\n        \}\n        close_gl_window\(&state_->window\);', dtor_code, content)

# 4. FramePresenter::poll
poll_search = r"""        while \(SDL_PollEvent\(&event\)\) \{
            if \(event.type == SDL_EVENT_QUIT\) \{
                running = false;
            \} else if \(event.type == SDL_EVENT_KEY_DOWN &&
                       event.key.scancode == SDL_SCANCODE_ESCAPE\)
            \{
            state.video_menu_open = !state.video_menu_open;
            state.video_menu_dirty = true;
                SDL_SetWindowTitle\(state.window.window,
                                   video_menu_title\(state.video_settings,
                                                    state.video_menu_open,
                                                    state.video_menu_row\)
                                       .c_str\(\)\);
            \} else if \(event.type == SDL_EVENT_KEY_DOWN &&
                       state.video_menu_open\)
            \{
                const auto apply_change = \[&\]\(\) \{
                    cycle\(&state.video_settings, state.video_menu_row,
                          event.key.scancode == SDL_SCANCODE_LEFT \? -1 : 1\);
                    state.video_menu_dirty = true;
                    /\* Apply the change for the active row. \*/
                    switch \(state.video_menu_row\) \{
                    case 0: /\* Resolution: recreate render target. \*/
                        configure_render_target\(state, nullptr\);
                        break;
                    case 3: /\* Window mode: apply immediately. \*/
                        apply_window_mode\(state\);
                        break;
                    case 4: /\* Target rate: update VSync. \*/
                        SDL_GL_SetSwapInterval\(state.video_settings.rate == PresentationRate::Unlimited \? 0 : 1\);
                        break;
                    default:
                        /\* Aspect \(1\) and filter \(2\) are read every frame
                         \* during blit, rate \(4\) during pace — no extra
                         \* action needed. \*/
                        break;
                    \}
                    save_video_settings\(state\);
                \};
                switch \(event.key.scancode\) \{
                case SDL_SCANCODE_UP:
                state.video_menu_row = static_cast<std::uint8_t>\(
                    \(state.video_menu_row \+ kVideoMenuRows - 1\) %
                    kVideoMenuRows\);
                state.video_menu_dirty = true;
                    break;
                case SDL_SCANCODE_DOWN:
                state.video_menu_row = static_cast<std::uint8_t>\(
                    \(state.video_menu_row \+ 1\) % kVideoMenuRows\);
                state.video_menu_dirty = true;
                    break;
            case SDL_SCANCODE_LEFT:
            case SDL_SCANCODE_RIGHT:
            case SDL_SCANCODE_RETURN:
                    apply_change\(\);
                    break;
                default:
                    break;
                \}
            \}
        \}"""

poll_replace = """        while (SDL_PollEvent(&event)) {
            if (process_settings_event(&event)) {
                // ImGui wants this event, don't pass to game
                continue;
            }
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN &&
                       event.key.scancode == SDL_SCANCODE_ESCAPE) {
                state.video_menu_open = !state.video_menu_open;
            }
        }"""
content = re.sub(poll_search, poll_replace, content)

# 5. FramePresenter::present (blit_and_swap)
present_search = r"""                draw_video_menu\(state\);
                draw_fps_counter\(state\);
                SDL_GL_SwapWindow\(state.window.window\);"""

present_replace = """                if (draw_settings_ui(&state.video_settings, &state.video_menu_open, state.last_fps)) {
                    configure_render_target(state, nullptr);
                    apply_window_mode(state);
                    SDL_GL_SetSwapInterval(state.video_settings.rate == PresentationRate::Unlimited ? 0 : 1);
                    save_video_settings(state);
                }
                SDL_GL_SwapWindow(state.window.window);"""
content = re.sub(present_search, present_replace, content)

with open('src/render/sdl_gl_renderer.cpp', 'w') as f:
    f.write(content)
