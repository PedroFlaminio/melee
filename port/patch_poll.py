with open('src/render/sdl_gl_renderer.cpp', 'r') as f:
    lines = f.readlines()

new_lines = []
skip = False
for i, line in enumerate(lines):
    if "while (SDL_PollEvent(&event)) {" in line and "if (event.type == SDL_EVENT_QUIT)" in lines[i+1]:
        skip = True
        new_lines.append(line)
        new_lines.append("            if (process_settings_event(&event)) {\n")
        new_lines.append("                continue;\n")
        new_lines.append("            }\n")
        new_lines.append("            if (event.type == SDL_EVENT_QUIT) {\n")
        new_lines.append("                running = false;\n")
        new_lines.append("            } else if (event.type == SDL_EVENT_KEY_DOWN &&\n")
        new_lines.append("                       event.key.scancode == SDL_SCANCODE_ESCAPE) {\n")
        new_lines.append("                state.video_menu_open = !state.video_menu_open;\n")
        new_lines.append("            }\n")
        new_lines.append("        }\n")
        continue

    if skip:
        if "        pad->connected =" in line:
            skip = False
            new_lines.append(line)
        continue
    
    new_lines.append(line)

with open('src/render/sdl_gl_renderer.cpp', 'w') as f:
    f.writelines(new_lines)
