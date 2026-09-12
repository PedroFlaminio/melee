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

/* GX's compare functions are enumerated in the same order as OpenGL's, so the
 * mapping is an offset rather than a table. */
GLenum gl_compare(mh_u32 compare)
{
    return static_cast<GLenum>(GL_NEVER + (compare & 7U));
}

/* A blend factor names the other side's colour depending on which side it is
 * used on: GX gives GX_BL_SRCCLR and GX_BL_DSTCLR the same value, and which
 * one it means follows from whether it multiplies the source or the
 * destination. */
GLenum gl_blend_factor(mh_u32 factor, bool source_side)
{
    switch (factor) {
    case 0: // GX_BL_ZERO
        return GL_ZERO;
    case 1: // GX_BL_ONE
        return GL_ONE;
    case 2: // GX_BL_SRCCLR as a destination factor, GX_BL_DSTCLR as a source
        return source_side ? GL_DST_COLOR : GL_SRC_COLOR;
    case 3:
        return source_side ? GL_ONE_MINUS_DST_COLOR : GL_ONE_MINUS_SRC_COLOR;
    case 4: // GX_BL_SRCALPHA
        return GL_SRC_ALPHA;
    case 5: // GX_BL_INVSRCALPHA
        return GL_ONE_MINUS_SRC_ALPHA;
    case 6: // GX_BL_DSTALPHA
        return GL_DST_ALPHA;
    case 7: // GX_BL_INVDSTALPHA
        return GL_ONE_MINUS_DST_ALPHA;
    default:
        return source_side ? GL_ONE : GL_ZERO;
    }
}

/* The reduction from GX's two alpha comparisons to one lives with the GX
 * model, not here; this only maps the result onto the fixed-function test. */
bool resolve_alpha_test(const MeleeHostGxDrawState& state, GLenum* out_func,
                        GLclampf* out_reference)
{
    mh_u32 compare = 0;
    mh_u8 reference = 0;
    if (!melee_host_gx_resolve_alpha_test(&state, &compare, &reference)) {
        return false;
    }
    *out_func = gl_compare(compare);
    *out_reference = static_cast<GLclampf>(reference) / 255.0F;
    return true;
}

/* How a group of triangles is coloured, read off the captured TEV program
 * where that is possible.  A program the host cannot read exactly falls back
 * to texture times vertex colour, which is what the viewer always did. */
struct Shading {
    bool use_texture = true;
    bool replace = false;
    float konst[4] = { 1.0F, 1.0F, 1.0F, 1.0F };
    float constant_alpha = 1.0F;
    bool exact = false;
};

Shading resolve_shading(const MeleeHostGxTevState& tev)
{
    Shading shading;
    MeleeHostGxResolvedShading resolved{};
    if (!melee_host_gx_resolve_shading(&tev, &resolved)) {
        return shading;
    }
    shading.constant_alpha =
        static_cast<float>(resolved.constant_alpha) / 255.0F;
    shading.exact = resolved.kind != MELEE_HOST_GX_SHADING_APPROXIMATED;
    switch (resolved.kind) {
    case MELEE_HOST_GX_SHADING_TEXTURE_TIMES_COLOR:
        break;
    case MELEE_HOST_GX_SHADING_KONST_TIMES_COLOR:
        shading.use_texture = false;
        for (std::size_t channel = 0; channel < 4; ++channel) {
            shading.konst[channel] =
                static_cast<float>(resolved.konst_color[channel]) / 255.0F;
        }
        break;
    case MELEE_HOST_GX_SHADING_TEXTURE:
        shading.replace = true;
        break;
    case MELEE_HOST_GX_SHADING_COLOR:
        shading.use_texture = false;
        break;
    default:
        shading.constant_alpha = 1.0F;
        break;
    }
    return shading;
}

/* Applies one captured state.  Returns false when the state draws nothing at
 * all, which GX_CULL_ALL does. */
bool apply_draw_state(const MeleeHostGxDrawState& state, bool front_face_cw)
{
    glFrontFace(front_face_cw ? GL_CW : GL_CCW);
    switch (state.cull_mode) {
    case 0: // GX_CULL_NONE
        glDisable(GL_CULL_FACE);
        break;
    case 1: // GX_CULL_FRONT
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        break;
    case 2: // GX_CULL_BACK
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        break;
    default: // GX_CULL_ALL
        return false;
    }

    if (state.z_compare_enable) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(gl_compare(state.z_func));
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    glDepthMask(state.z_update_enable ? GL_TRUE : GL_FALSE);

    switch (state.blend_mode) {
    case 1: // GX_BM_BLEND
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(gl_blend_factor(state.blend_src_factor, true),
                    gl_blend_factor(state.blend_dst_factor, false));
        break;
    case 3: // GX_BM_SUBTRACT, which GX fixes at destination minus source
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
        glBlendFunc(GL_ONE, GL_ONE);
        break;
    default: // GX_BM_NONE, and GX_BM_LOGIC which this viewer does not model
        glDisable(GL_BLEND);
        break;
    }

    GLenum alpha_func = GL_ALWAYS;
    GLclampf alpha_reference = 0.0F;
    if (resolve_alpha_test(state, &alpha_func, &alpha_reference)) {
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(alpha_func, alpha_reference);
    } else {
        glDisable(GL_ALPHA_TEST);
    }

    const GLboolean color = state.color_update_enable ? GL_TRUE : GL_FALSE;
    const GLboolean alpha = state.alpha_update_enable ? GL_TRUE : GL_FALSE;
    glColorMask(color, color, color, alpha);
    return true;
}

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

/* A neutral texture for a material that names none: modulating by white
 * leaves the colour the program computed untouched. */
GLuint create_white_texture()
{
    constexpr std::array<GLubyte, 4> pixels{ 255, 255, 255, 255 };
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA,
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

bool show_captured_geometry(MeleeHostContext* context, std::string* error,
                            FrameCallback on_frame, void* user_data)
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
    const GLuint white_texture = create_white_texture();
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

    /* Draw order between states: everything that does not blend first, so the
     * opaque geometry has written depth before a blended group reads it.  This
     * is the ordering the original render passes imply, derived from the state
     * each draw actually ran under rather than assumed. */
    /* A group is one pixel state paired with one material program, because a
     * triangle needs both to be drawn the way it was captured. */
    struct Group {
        mh_u32 draw_state;
        mh_u32 tev_state;
    };
    const auto collect_groups = [](std::size_t count) {
        std::vector<Group> groups;
    for (int blended = 0; blended < 2; ++blended) {
        for (std::size_t index = 0; index < count; ++index) {
            MeleeHostGxCapturedTriangle triangle{};
            if (!melee_host_gx_captured_triangle_at(index, &triangle)) {
                continue;
            }
            const mh_u32 draw_id = triangle.vertices[0].draw_state;
            const mh_u32 tev_id = triangle.vertices[0].tev_state;
            MeleeHostGxDrawState state{};
            if (!melee_host_gx_captured_draw_state_at(draw_id, &state)) {
                continue;
            }
            const bool is_blended =
                state.blend_mode == 1 || state.blend_mode == 3;
            if (is_blended != (blended != 0)) {
                continue;
            }
            const bool known =
                std::any_of(groups.begin(), groups.end(),
                            [&](const Group& group) {
                                return group.draw_state == draw_id &&
                                       group.tev_state == tev_id;
                            });
            if (!known) {
                groups.push_back({ draw_id, tev_id });
            }
        }
    }
        return groups;
    };
    std::vector<Group> groups = collect_groups(triangle_count);
    std::size_t frame_triangles = triangle_count;

    bool running = true;
    /* GX treats a clockwise winding as the front face.  The viewer starts
     * there and can flip it, because a model that looks inside out is the
     * clearest evidence the assumption is wrong for a given asset. */
    bool front_face_cw = true;
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
                case SDL_SCANCODE_F:
                    front_face_cw = !front_face_cw;
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
        if (on_frame != nullptr) {
            /* The viewer advances the animation and captures again here, so
             * the geometry read below is this frame's, not the first one's. */
            on_frame(user_data);
            const std::size_t captured = melee_host_gx_triangle_count();
            if (captured != frame_triangles) {
                groups = collect_groups(captured);
                frame_triangles = captured;
            }
        }
        int width = 0;
        int height = 0;
        SDL_GetWindowSizeInPixels(window, &width, &height);
        if (width <= 0 || height <= 0) {
            continue;
        }
        configure_projection(bounds, width, height, yaw, pitch, zoom);
        glClearColor(0.035F, 0.045F, 0.08F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, checker_texture);
        /* One group per captured state, blended groups last so the opaque
         * geometry has already written depth.  The order within a group is the
         * order the draws happened in. */
        for (const Group& group : groups) {
            MeleeHostGxDrawState state{};
            if (!melee_host_gx_captured_draw_state_at(group.draw_state,
                                                      &state)) {
                continue;
            }
            if (!apply_draw_state(state, front_face_cw)) {
                continue;
            }
            MeleeHostGxTevState tev{};
            const Shading shading =
                melee_host_gx_captured_tev_state_at(group.tev_state, &tev)
                    ? resolve_shading(tev)
                    : Shading{};
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,
                      shading.replace ? GL_REPLACE : GL_MODULATE);
            for (std::size_t index = 0; index < frame_triangles; ++index) {
                MeleeHostGxCapturedTriangle triangle{};
                if (!melee_host_gx_captured_triangle_at(index, &triangle)) {
                    continue;
                }
                if (triangle.vertices[0].draw_state != group.draw_state ||
                    triangle.vertices[0].tev_state != group.tev_state)
                {
                    continue;
                }
                glBegin(GL_TRIANGLES);
                for (const auto& vertex : triangle.vertices) {
                    float red = 0.85F;
                    float green = 0.85F;
                    float blue = 0.9F;
                    float alpha = 1.0F;
                    if ((vertex.attributes & MELEE_HOST_GX_VERTEX_COLOR) != 0) {
                        red = static_cast<float>(vertex.color[0]) / 255.0F;
                        green = static_cast<float>(vertex.color[1]) / 255.0F;
                        blue = static_cast<float>(vertex.color[2]) / 255.0F;
                        alpha = static_cast<float>(vertex.color[3]) / 255.0F;
                    }
                    /* The material's own constant colour and alpha, which the
                     * TEV program multiplies in. */
                    glColor4f(red * shading.konst[0], green * shading.konst[1],
                              blue * shading.konst[2],
                              alpha * shading.konst[3] *
                                  shading.constant_alpha);
                    const bool has_texture =
                        shading.use_texture &&
                        (vertex.attributes &
                         MELEE_HOST_GX_VERTEX_TEXTURE_IMAGE) != 0 &&
                        vertex.texture_image < textures.size();
                    glBindTexture(GL_TEXTURE_2D,
                                  has_texture
                                      ? textures[vertex.texture_image]
                                      : (shading.use_texture ? checker_texture
                                                             : white_texture));
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
        glDisable(GL_CULL_FACE);
        glDisable(GL_ALPHA_TEST);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        SDL_GL_SwapWindow(window);
    }

    glDeleteTextures(1, &checker_texture);
    glDeleteTextures(1, &white_texture);
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
