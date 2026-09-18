with open('src/render/sdl_gl_renderer.cpp', 'r') as f:
    content = f.read()

# We need to rewrite the 'present' loop to decouple the 60Hz sim from the target rate
# Let's find the current logic
search = """            if (state.video_settings.rate == PresentationRate::Unlimited) {
                /* Unlimited: the simulation still advances at 60 Hz, but
                 * the last frame is re-blitted to the window as fast as
                 * the GPU allows until the next simulation tick. */
                constexpr std::uint64_t kSimTickNs = 1'000'000'000ULL / 60;
                const Uint64 now = SDL_GetTicksNS();
                Uint64& next = state.next_frame_ns;
                if (next == 0 || now > next + kSimTickNs) {
                    next = now;
                }
                next += kSimTickNs;
                do {
                    blit_and_swap();
                    double fps = 0.0;
                    if (state.frame_rate.add_frame(SDL_GetTicksNS(), &fps)) {
                        state.last_fps = fps;
                        SDL_SetWindowTitle(
                            state.window.window,
                            video_menu_title(state.video_settings,
                                             state.video_menu_open,
                                             state.video_menu_row, fps)
                                .c_str());
                    }
                } while (SDL_GetTicksNS() < next);
            } else {
                /* Fixed target rate: single blit, then pace. */
                blit_and_swap();
                pace(1'000'000'000ULL /
                     static_cast<std::uint16_t>(state.video_settings.rate));
                double fps = 0.0;
                if (state.frame_rate.add_frame(SDL_GetTicksNS(), &fps)) {
                    state.last_fps = fps;
                    SDL_SetWindowTitle(
                        state.window.window,
                        video_menu_title(state.video_settings,
                                         state.video_menu_open,
                                         state.video_menu_row, fps)
                            .c_str());
                }
            }"""

replace = """            constexpr std::uint64_t kSimTickNs = 1'000'000'000ULL / 60;
            const Uint64 now = SDL_GetTicksNS();
            Uint64& next_sim = state.next_frame_ns;
            if (next_sim == 0 || now > next_sim + kSimTickNs) {
                next_sim = now;
            }
            next_sim += kSimTickNs;

            if (state.video_settings.rate == PresentationRate::Unlimited) {
                do {
                    blit_and_swap();
                    double fps = 0.0;
                    if (state.frame_rate.add_frame(SDL_GetTicksNS(), &fps)) {
                        state.last_fps = fps;
                    }
                } while (SDL_GetTicksNS() < next_sim);
            } else {
                std::uint64_t target_rate = static_cast<std::uint16_t>(state.video_settings.rate);
                std::uint64_t frame_ns = 1'000'000'000ULL / target_rate;
                Uint64 next_blit = SDL_GetTicksNS() + frame_ns;
                
                do {
                    blit_and_swap();
                    double fps = 0.0;
                    if (state.frame_rate.add_frame(SDL_GetTicksNS(), &fps)) {
                        state.last_fps = fps;
                    }
                    
                    // Pace to the target frame rate
                    Uint64 current_time = SDL_GetTicksNS();
                    if (next_blit > current_time) {
                        // Don't delay past the simulation tick
                        Uint64 delay = next_blit - current_time;
                        if (current_time + delay > next_sim) {
                            delay = next_sim > current_time ? next_sim - current_time : 0;
                        }
                        if (delay > 0) {
                            SDL_DelayPrecise(delay);
                        }
                    }
                    next_blit += frame_ns;
                } while (SDL_GetTicksNS() < next_sim);
            }"""

if search in content:
    with open('src/render/sdl_gl_renderer.cpp', 'w') as f:
        f.write(content.replace(search, replace))
    print("Patched!")
else:
    print("Search string not found!")
