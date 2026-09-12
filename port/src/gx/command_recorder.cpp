#include <melee_host/gx.h>

#include <bit>
#include <mutex>
#include <vector>

namespace {

std::mutex command_mutex;
std::vector<MeleeHostGxCommand> commands;
std::vector<MeleeHostGxTriangle> triangles;

struct ActiveDraw {
    bool active = false;
    mh_u8 primitive = 0;
    mh_u16 expected_vertices = 0;
    std::vector<MeleeHostGxPosition3f32> positions;
    std::vector<MeleeHostGxCapturedVertex> vertices;
};

ActiveDraw active_draw;

constexpr mh_u8 kGxQuads = 0x80U;
constexpr mh_u8 kGxTriangles = 0x90U;
constexpr mh_u8 kGxTriangleStrip = 0x98U;
constexpr mh_u8 kGxTriangleFan = 0xA0U;

void append_triangle(const MeleeHostGxPosition3f32& first,
                     const MeleeHostGxPosition3f32& second,
                     const MeleeHostGxPosition3f32& third)
{
    triangles.push_back({ { first, second, third } });
}

void assemble_latest_position()
{
    const std::size_t count = active_draw.positions.size();
    if (active_draw.primitive == kGxTriangles && count % 3 == 0) {
        append_triangle(active_draw.positions[count - 3],
                        active_draw.positions[count - 2],
                        active_draw.positions[count - 1]);
    } else if (active_draw.primitive == kGxTriangleStrip && count >= 3) {
        if ((count - 3) % 2 == 0) {
            append_triangle(active_draw.positions[count - 3],
                            active_draw.positions[count - 2],
                            active_draw.positions[count - 1]);
        } else {
            append_triangle(active_draw.positions[count - 2],
                            active_draw.positions[count - 3],
                            active_draw.positions[count - 1]);
        }
    } else if (active_draw.primitive == kGxTriangleFan && count >= 3) {
        append_triangle(active_draw.positions[0],
                        active_draw.positions[count - 2],
                        active_draw.positions[count - 1]);
    } else if (active_draw.primitive == kGxQuads && count % 4 == 0) {
        append_triangle(active_draw.positions[count - 4],
                        active_draw.positions[count - 3],
                        active_draw.positions[count - 2]);
        append_triangle(active_draw.positions[count - 4],
                        active_draw.positions[count - 2],
                        active_draw.positions[count - 1]);
    }
}

void submit_locked(MeleeHostGxValueType type, mh_u32 bits)
{
    commands.push_back({ type, bits });
}

void submit(MeleeHostGxValueType type, mh_u32 bits)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    submit_locked(type, bits);
}

} // namespace

extern "C" void melee_host_gx_submit_u8(mh_u8 value)
{
    submit(MELEE_HOST_GX_U8, value);
}

extern "C" void melee_host_gx_submit_u16(mh_u16 value)
{
    submit(MELEE_HOST_GX_U16, value);
}

extern "C" void melee_host_gx_submit_u32(mh_u32 value)
{
    submit(MELEE_HOST_GX_U32, value);
}

extern "C" void melee_host_gx_submit_f32(mh_f32 value)
{
    submit(MELEE_HOST_GX_F32, std::bit_cast<mh_u32>(value));
}

extern "C" void melee_host_gx_begin(mh_u8 primitive, mh_u8 vertex_format,
                                     mh_u16 vertex_count)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    submit_locked(MELEE_HOST_GX_U8,
                  static_cast<mh_u32>(primitive | vertex_format));
    submit_locked(MELEE_HOST_GX_U16, vertex_count);
    active_draw.active = vertex_count != 0;
    active_draw.primitive = primitive;
    active_draw.expected_vertices = vertex_count;
    active_draw.positions.clear();
    active_draw.positions.reserve(vertex_count);
    active_draw.vertices.clear();
    active_draw.vertices.reserve(vertex_count);
}

extern "C" void melee_host_gx_end(void)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    active_draw.active = false;
}

extern "C" void melee_host_gx_submit_position3f32(mh_f32 x, mh_f32 y,
                                                    mh_f32 z)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    submit_locked(MELEE_HOST_GX_F32, std::bit_cast<mh_u32>(x));
    submit_locked(MELEE_HOST_GX_F32, std::bit_cast<mh_u32>(y));
    submit_locked(MELEE_HOST_GX_F32, std::bit_cast<mh_u32>(z));
    if (!active_draw.active) {
        return;
    }
    if (active_draw.positions.size() >= active_draw.expected_vertices) {
        return;
    }
    active_draw.positions.push_back({ x, y, z });
    MeleeHostGxCapturedVertex vertex{};
    vertex.attributes = MELEE_HOST_GX_VERTEX_POSITION;
    vertex.position = { x, y, z };
    active_draw.vertices.push_back(vertex);
    assemble_latest_position();
}

extern "C" void melee_host_gx_submit_normal3f32(mh_f32 x, mh_f32 y,
                                                  mh_f32 z)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    submit_locked(MELEE_HOST_GX_F32, std::bit_cast<mh_u32>(x));
    submit_locked(MELEE_HOST_GX_F32, std::bit_cast<mh_u32>(y));
    submit_locked(MELEE_HOST_GX_F32, std::bit_cast<mh_u32>(z));
    if (active_draw.active && !active_draw.vertices.empty()) {
        auto& vertex = active_draw.vertices.back();
        vertex.attributes |= MELEE_HOST_GX_VERTEX_NORMAL;
        vertex.normal = { x, y, z };
    }
}

extern "C" void melee_host_gx_submit_color4u8(mh_u8 red, mh_u8 green,
                                                mh_u8 blue, mh_u8 alpha)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    submit_locked(MELEE_HOST_GX_U8, red);
    submit_locked(MELEE_HOST_GX_U8, green);
    submit_locked(MELEE_HOST_GX_U8, blue);
    submit_locked(MELEE_HOST_GX_U8, alpha);
    if (active_draw.active && !active_draw.vertices.empty()) {
        auto& vertex = active_draw.vertices.back();
        vertex.attributes |= MELEE_HOST_GX_VERTEX_COLOR;
        vertex.color[0] = red;
        vertex.color[1] = green;
        vertex.color[2] = blue;
        vertex.color[3] = alpha;
    }
}

extern "C" void melee_host_gx_submit_texcoord2f32(mh_f32 s, mh_f32 t)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    submit_locked(MELEE_HOST_GX_F32, std::bit_cast<mh_u32>(s));
    submit_locked(MELEE_HOST_GX_F32, std::bit_cast<mh_u32>(t));
    if (active_draw.active && !active_draw.vertices.empty()) {
        auto& vertex = active_draw.vertices.back();
        vertex.attributes |= MELEE_HOST_GX_VERTEX_TEXCOORD;
        vertex.texcoord[0] = s;
        vertex.texcoord[1] = t;
    }
}

extern "C" void melee_host_gx_reset_command_log(void)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    commands.clear();
    triangles.clear();
    active_draw = {};
}

extern "C" size_t melee_host_gx_triangle_count(void)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    return triangles.size();
}

extern "C" bool melee_host_gx_triangle_at(size_t index,
                                           MeleeHostGxTriangle* output)
{
    if (output == nullptr) {
        return false;
    }
    const std::lock_guard<std::mutex> lock(command_mutex);
    if (index >= triangles.size()) {
        return false;
    }
    *output = triangles[index];
    return true;
}

extern "C" size_t melee_host_gx_captured_vertex_count(void)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    return active_draw.vertices.size();
}

extern "C" bool melee_host_gx_captured_vertex_at(
    size_t index, MeleeHostGxCapturedVertex* output)
{
    if (output == nullptr) {
        return false;
    }
    const std::lock_guard<std::mutex> lock(command_mutex);
    if (index >= active_draw.vertices.size()) {
        return false;
    }
    *output = active_draw.vertices[index];
    return true;
}

extern "C" size_t melee_host_gx_command_count(void)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    return commands.size();
}

extern "C" bool melee_host_gx_command_at(size_t index,
                                           MeleeHostGxCommand* output)
{
    if (output == nullptr) {
        return false;
    }

    const std::lock_guard<std::mutex> lock(command_mutex);
    if (index >= commands.size()) {
        return false;
    }
    *output = commands[index];
    return true;
}
