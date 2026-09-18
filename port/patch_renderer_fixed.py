import sys

with open('src/render/sdl_gl_renderer.cpp', 'r') as f:
    lines = f.readlines()

new_lines = []
in_poll = False
poll_braces = 0

for line in lines:
    if line.startswith('#include "render/play_window.hpp"'):
        new_lines.append(line)
        new_lines.append('#include "render/settings_ui.hpp"\n')
        continue
        
    if "if (!hidden) {" in line:
        new_lines.append(line)
        continue
    if "apply_window_mode(*state_);" in line:
        new_lines.append(line)
        continue
    if "        return true;" in line and "init_settings_ui" not in new_lines[-1] and "apply_window_mode" in new_lines[-2]:
        new_lines.append("        init_settings_ui(state_->window.window, state_->window.context);\n")
        new_lines.append(line)
        continue

    if "        if (state_->audio != nullptr) {" in line and "destroy_settings_ui" not in "".join(new_lines[-10:]):
        new_lines.append(line)
        continue
    if "            SDL_DestroyAudioStream(state_->audio);" in line:
        new_lines.append(line)
        continue
    if "        close_gl_window(&state_->window);" in line and "destroy_settings_ui" not in new_lines[-1]:
        new_lines.append("        destroy_settings_ui();\n")
        new_lines.append(line)
        continue

    if "draw_video_menu(state);" in line:
        new_lines.append("                if (draw_settings_ui(&state.video_settings, &state.video_menu_open, state.last_fps)) {\n")
        new_lines.append("                    configure_render_target(state, nullptr);\n")
        new_lines.append("                    apply_window_mode(state);\n")
        new_lines.append("                    SDL_GL_SetSwapInterval(state.video_settings.rate == PresentationRate::Unlimited ? 0 : 1);\n")
        new_lines.append("                    save_video_settings(state);\n")
        new_lines.append("                }\n")
        continue
    if "draw_fps_counter(state);" in line:
        continue

    # Carefully replace the body of FramePresenter::poll's event loop
    if "bool FramePresenter::poll(MeleeHostPadState* pad)" in line:
        in_poll = True
        new_lines.append(line)
        continue
        
    if in_poll and "while (SDL_PollEvent(&event)) {" in line:
        new_lines.append(line)
        new_lines.append("            if (process_settings_event(&event)) {\n")
        new_lines.append("                continue;\n")
        new_lines.append("            }\n")
        continue
        
    if in_poll and "state.video_menu_open = !state.video_menu_open;" in line:
        new_lines.append(line)
        continue
    if in_poll and "state.video_menu_dirty = true;" in line:
        continue # delete
    if in_poll and "SDL_SetWindowTitle(state.window.window," in line and "video_menu_title" in lines[lines.index(line)+1]:
        # Skip the next 4 lines
        continue
    if in_poll and "video_menu_title(" in line:
        continue
    if in_poll and "state.video_menu_open," in line:
        continue
    if in_poll and "state.video_menu_row)" in line:
        continue
    if in_poll and ".c_str());" in line:
        continue

    # Skip the entire "else if (event.type == SDL_EVENT_KEY_DOWN && state.video_menu_open)" block
    if in_poll and "} else if (event.type == SDL_EVENT_KEY_DOWN &&" in line and "state.video_menu_open)" in lines[lines.index(line)+1]:
        continue
    if in_poll and "state.video_menu_open)" in line and "} else if (event.type == SDL_EVENT_KEY_DOWN &&" in new_lines[-1] == False: # already skipped
        pass # but we need to skip the block
        
    new_lines.append(line)

with open('src/render/sdl_gl_renderer.cpp', 'w') as f:
    f.writelines(new_lines)
