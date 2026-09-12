#include "render/sdl_gl_renderer.hpp"

#include <melee_host/gx.h>
#include <melee_host/input.h>
#include <melee_host/host.h>

#include <GL/gl.h>
#include <SDL3/SDL.h>

#include <dolphin/pad.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <utility>

namespace melee::render {
namespace {

std::vector<TextureImage> texture_images;
constexpr mh_u32 kRenderTranslucent = 1U << 30U;

struct Bounds {
    float minimum[3] = { std::numeric_limits<float>::max(),
                         std::numeric_limits<float>::max(),
                         std::numeric_limits<float>::max() };
    float maximum[3] = { std::numeric_limits<float>::lowest(),
                         std::numeric_limits<float>::lowest(),
                         std::numeric_limits<float>::lowest() };
};

void extend(Bounds* bounds, const MeleeHostGxPosition3f32& position)
{
    const std::array<float, 3> values{ position.x, position.y, position.z };
    for (std::size_t axis = 0; axis < values.size(); ++axis) {
        bounds->minimum[axis] = std::min(bounds->minimum[axis], values[axis]);
        bounds->maximum[axis] = std::max(bounds->maximum[axis], values[axis]);
    }
}

void configure_projection(const Bounds& bounds, int width, int height,
                          float yaw, float pitch, float zoom)
{
    const float center_x = (bounds.minimum[0] + bounds.maximum[0]) * 0.5F;
    const float center_y = (bounds.minimum[1] + bounds.maximum[1]) * 0.5F;
    const float extent_x = bounds.maximum[0] - bounds.minimum[0];
    const float extent_y = bounds.maximum[1] - bounds.minimum[1];
    const float extent_z = bounds.maximum[2] - bounds.minimum[2];
    const float aspect = static_cast<float>(width) / static_cast<float>(height);
    const float radius = std::max({ extent_x, extent_y, extent_z, 1.0F }) * 0.7F;
    const float near_plane = std::max(radius * 0.01F, 0.01F);
    const float distance = radius * 3.0F * zoom;
    const float half_height = near_plane * 0.5F;

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-half_height * aspect, half_height * aspect, -half_height,
              half_height, near_plane, distance + radius * 4.0F);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0F, 0.0F, -distance);
    glRotatef(pitch, 1.0F, 0.0F, 0.0F);
    glRotatef(yaw, 0.0F, 1.0F, 0.0F);
    glTranslatef(-center_x, -center_y,
                 -(bounds.minimum[2] + bounds.maximum[2]) * 0.5F);
}

GLuint create_checker_texture()
{
    constexpr std::array<GLubyte, 16> pixels{
        245, 245, 245, 255,  35,  45,  85,  255,
         35,  45,  85, 255, 245, 245, 245, 255,
    };
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, pixels.data());
    return texture;
}

GLint wrap_mode(std::uint32_t mode)
{
    if (mode == 1) {
        return GL_REPEAT;
    }
    if (mode == 2) {
        return GL_MIRRORED_REPEAT;
    }
    return GL_CLAMP_TO_EDGE;
}

} // namespace

bool show_captured_geometry(MeleeHostContext* context, std::string* error)
{
    const std::size_t triangle_count = melee_host_gx_triangle_count();
    if (triangle_count == 0) {
        if (error != nullptr) {
            *error = "there is no captured geometry to draw";
        }
        return false;
    }
    if (context == nullptr || !SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        if (error != nullptr) {
            *error = SDL_GetError();
        }
        return false;
    }
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_Window* window = SDL_CreateWindow(
        "Melee PC — preview: setas/analógico direito movem a câmera; Back/Esc sai",
        960, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (window == nullptr) {
        if (error != nullptr) {
            *error = SDL_GetError();
        }
        SDL_Quit();
        return false;
    }
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (gl_context == nullptr) {
        if (error != nullptr) {
            *error = SDL_GetError();
        }
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }
    SDL_GL_SetSwapInterval(1);
    int gamepad_count = 0;
    SDL_JoystickID* gamepad_ids = SDL_GetGamepads(&gamepad_count);
    SDL_Gamepad* gamepad =
        gamepad_count > 0 ? SDL_OpenGamepad(gamepad_ids[0]) : nullptr;
    SDL_free(gamepad_ids);
    const GLuint checker_texture = create_checker_texture();
    std::vector<GLuint> textures;
    textures.reserve(texture_images.size());
    for (const auto& image : texture_images) {
        GLuint texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_mode(image.wrap_s));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_mode(image.wrap_t));
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, image.rgba.data());
        textures.push_back(texture);
    }

    Bounds bounds;
    for (std::size_t index = 0; index < triangle_count; ++index) {
        MeleeHostGxCapturedTriangle triangle{};
        if (!melee_host_gx_captured_triangle_at(index, &triangle)) {
            continue;
        }
        for (const auto& vertex : triangle.vertices) {
            extend(&bounds, vertex.position);
        }
    }

    bool running = true;
    float yaw = 20.0F;
    float pitch = -20.0F;
    float zoom = 1.0F;
    while (running) {
        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_GAMEPAD_ADDED &&
                       gamepad == nullptr)
            {
                gamepad = SDL_OpenGamepad(event.gdevice.which);
            } else if (event.type == SDL_EVENT_GAMEPAD_REMOVED &&
                       gamepad != nullptr &&
                       SDL_GetGamepadID(gamepad) == event.gdevice.which)
            {
                SDL_CloseGamepad(gamepad);
                gamepad = nullptr;
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                switch (event.key.scancode) {
                case SDL_SCANCODE_ESCAPE:
                    running = false;
                    break;
                case SDL_SCANCODE_LEFT:
                    yaw -= 5.0F;
                    break;
                case SDL_SCANCODE_RIGHT:
                    yaw += 5.0F;
                    break;
                case SDL_SCANCODE_UP:
                    pitch = std::min(pitch + 5.0F, 89.0F);
                    break;
                case SDL_SCANCODE_DOWN:
                    pitch = std::max(pitch - 5.0F, -89.0F);
                    break;
                case SDL_SCANCODE_PAGEUP:
                    zoom = std::max(zoom * 0.9F, 0.1F);
                    break;
                case SDL_SCANCODE_PAGEDOWN:
                    zoom = std::min(zoom * 1.1F, 100.0F);
                    break;
                default:
                    break;
                }
            }
        }
        const bool* const keys = SDL_GetKeyboardState(nullptr);
        const auto axis = [](bool negative, bool positive) {
            return static_cast<mh_s8>((positive ? 127 : 0) -
                                      (negative ? 127 : 0));
        };
        MeleeHostPadState pad{
            .buttons = static_cast<mh_u16>(
                (keys[SDL_SCANCODE_J] ? PAD_BUTTON_A : 0) |
                (keys[SDL_SCANCODE_K] ? PAD_BUTTON_B : 0) |
                (keys[SDL_SCANCODE_U] ? PAD_BUTTON_X : 0) |
                (keys[SDL_SCANCODE_I] ? PAD_BUTTON_Y : 0) |
                (keys[SDL_SCANCODE_Q] ? PAD_TRIGGER_Z : 0) |
                (keys[SDL_SCANCODE_RETURN] ? PAD_BUTTON_START : 0)),
            .stick_x = axis(keys[SDL_SCANCODE_A], keys[SDL_SCANCODE_D]),
            .stick_y = axis(keys[SDL_SCANCODE_S], keys[SDL_SCANCODE_W]),
            .c_stick_x = axis(keys[SDL_SCANCODE_LEFT], keys[SDL_SCANCODE_RIGHT]),
            .c_stick_y = axis(keys[SDL_SCANCODE_DOWN], keys[SDL_SCANCODE_UP]),
            .trigger_left = static_cast<mh_u8>(
                keys[SDL_SCANCODE_H] ? 255U : 0U),
            .trigger_right = static_cast<mh_u8>(
                keys[SDL_SCANCODE_L] ? 255U : 0U),
            .connected = true,
        };
        if (gamepad != nullptr) {
            const auto gamepad_axis = [](Sint16 value) {
                return static_cast<mh_s8>(static_cast<int>(value) / 258);
            };
            const auto gamepad_trigger = [](Sint16 value) {
                return static_cast<mh_u8>(std::clamp(static_cast<int>(value) / 128,
                                                      0, 255));
            };
            pad.buttons = static_cast<mh_u16>(
                (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH)
                     ? PAD_BUTTON_A
                     : 0) |
                (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_EAST)
                     ? PAD_BUTTON_B
                     : 0) |
                (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_WEST)
                     ? PAD_BUTTON_X
                     : 0) |
                (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_NORTH)
                     ? PAD_BUTTON_Y
                     : 0) |
                (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)
                     ? PAD_TRIGGER_Z
                     : 0) |
                (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_START)
                     ? PAD_BUTTON_START
                     : 0));
            pad.stick_x = gamepad_axis(
                SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX));
            pad.stick_y = gamepad_axis(
                SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY));
            pad.c_stick_x = gamepad_axis(
                SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTX));
            pad.c_stick_y = gamepad_axis(
                SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTY));
            pad.trigger_left = gamepad_trigger(
                SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER));
            pad.trigger_right = gamepad_trigger(
                SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER));

            /* The preview has no gameplay loop yet, so use the C-stick and
             * triggers as visible camera controls while continuing to submit
             * the complete PAD state to the host. */
            const float camera_x = static_cast<float>(pad.c_stick_x) / 127.0F;
            const float camera_y = static_cast<float>(pad.c_stick_y) / 127.0F;
            yaw += camera_x * 2.5F;
            pitch = std::clamp(pitch - camera_y * 2.5F, -89.0F, 89.0F);
            const float zoom_axis =
                static_cast<float>(pad.trigger_right) -
                static_cast<float>(pad.trigger_left);
            zoom = std::clamp(zoom * (1.0F - zoom_axis / 8192.0F), 0.1F,
                              100.0F);
            if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_BACK)) {
                running = false;
            }
        }
        if (melee_host_submit_pad_state(context, 0, &pad) != MELEE_HOST_OK) {
            if (error != nullptr) {
                *error = "could not submit SDL input";
            }
            running = false;
        }
        static_cast<void>(melee_host_step(context));
        int width = 0;
        int height = 0;
        SDL_GetWindowSizeInPixels(window, &width, &height);
        if (width <= 0 || height <= 0) {
            continue;
        }
        configure_projection(bounds, width, height, yaw, pitch, zoom);
        glClearColor(0.035F, 0.045F, 0.08F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, checker_texture);
        for (int pass = 0; pass < 2; ++pass) {
            const bool translucent_pass = pass == 1;
            if (translucent_pass) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDepthMask(GL_FALSE);
            } else {
                glDisable(GL_BLEND);
                glDepthMask(GL_TRUE);
            }
            for (std::size_t index = 0; index < triangle_count; ++index) {
                MeleeHostGxCapturedTriangle triangle{};
                if (!melee_host_gx_captured_triangle_at(index, &triangle)) {
                    continue;
                }
                const bool translucent =
                    (triangle.vertices[0].render_mode & kRenderTranslucent) != 0;
                if (translucent != translucent_pass) {
                    continue;
                }
                glBegin(GL_TRIANGLES);
                for (const auto& vertex : triangle.vertices) {
                    if ((vertex.attributes & MELEE_HOST_GX_VERTEX_COLOR) != 0) {
                        glColor4ub(vertex.color[0], vertex.color[1], vertex.color[2],
                                   vertex.color[3]);
                    } else {
                        glColor4f(0.85F, 0.85F, 0.9F, 1.0F);
                    }
                    const GLuint texture =
                        (vertex.attributes & MELEE_HOST_GX_VERTEX_TEXTURE_IMAGE) != 0 &&
                                vertex.texture_image < textures.size()
                            ? textures[vertex.texture_image]
                            : checker_texture;
                    glBindTexture(GL_TEXTURE_2D, texture);
                    if ((vertex.attributes & MELEE_HOST_GX_VERTEX_TEXCOORD) != 0) {
                        glTexCoord2f(vertex.texcoord[0], vertex.texcoord[1]);
                    } else {
                        glTexCoord2f(0.0F, 0.0F);
                    }
                    glVertex3f(vertex.position.x, vertex.position.y,
                               vertex.position.z);
                }
                glEnd();
            }
        }
        glDepthMask(GL_TRUE);
        SDL_GL_SwapWindow(window);
    }

    glDeleteTextures(1, &checker_texture);
    if (!textures.empty()) {
        glDeleteTextures(static_cast<GLsizei>(textures.size()), textures.data());
    }
    SDL_GL_DestroyContext(gl_context);
    if (gamepad != nullptr) {
        SDL_CloseGamepad(gamepad);
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
    return true;
}

void set_texture_images(std::vector<TextureImage> images)
{
    texture_images = std::move(images);
}

} // namespace melee::render
