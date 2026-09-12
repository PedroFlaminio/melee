#include <melee_host/gx.h>

#include <dolphin/gx/GXGeometry.h>
#include <dolphin/gx/GXCommandList.h>
#include <dolphin/gx/GXDispList.h>

#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <mutex>
#include <vector>

namespace {

std::mutex command_mutex;
std::vector<MeleeHostGxCommand> commands;
std::vector<MeleeHostGxTriangle> triangles;
std::vector<MeleeHostGxCapturedVertex> captured_vertices;
std::vector<std::array<std::size_t, 3>> captured_triangle_indices;

struct ActiveDraw {
    bool active = false;
    mh_u8 primitive = 0;
    mh_u8 vertex_format = 0;
    mh_u16 expected_vertices = 0;
    std::size_t captured_vertex_start = 0;
    std::vector<MeleeHostGxPosition3f32> positions;
    std::vector<MeleeHostGxCapturedVertex> vertices;
};

ActiveDraw active_draw;

struct AttributeFormat {
    GXCompCnt component_count = GX_POS_XYZ;
    GXCompType component_type = GX_F32;
    mh_u8 fractional_bits = 0;
};

struct AttributeArray {
    const std::byte* base = nullptr;
    mh_u8 stride = 0;
    std::size_t byte_length = 0;
};

constexpr std::size_t kAttributeCount = static_cast<std::size_t>(GX_VA_MAX_ATTR);
constexpr std::size_t kVertexFormatCount =
    static_cast<std::size_t>(GX_MAX_VTXFMT);
constexpr std::size_t kUnboundedArraySize =
    std::numeric_limits<std::size_t>::max();

std::array<GXAttrType, kAttributeCount> vertex_descriptors{};
std::array<std::array<AttributeFormat, kAttributeCount>, kVertexFormatCount>
    attribute_formats{};
std::array<AttributeArray, kAttributeCount> attribute_arrays{};
std::size_t display_list_errors = 0;

constexpr mh_u8 kGxQuads = 0x80U;
constexpr mh_u8 kGxTriangles = 0x90U;
constexpr mh_u8 kGxTriangleStrip = 0x98U;
constexpr mh_u8 kGxTriangleFan = 0xA0U;

void append_triangle(std::size_t first, std::size_t second, std::size_t third)
{
    triangles.push_back({ { active_draw.positions[first],
                           active_draw.positions[second],
                           active_draw.positions[third] } });
    captured_triangle_indices.push_back(
        { active_draw.captured_vertex_start + first,
          active_draw.captured_vertex_start + second,
          active_draw.captured_vertex_start + third });
}

void assemble_latest_position()
{
    const std::size_t count = active_draw.positions.size();
    if (active_draw.primitive == kGxTriangles && count % 3 == 0) {
        append_triangle(count - 3, count - 2, count - 1);
    } else if (active_draw.primitive == kGxTriangleStrip && count >= 3) {
        if ((count - 3) % 2 == 0) {
            append_triangle(count - 3, count - 2, count - 1);
        } else {
            append_triangle(count - 2, count - 3, count - 1);
        }
    } else if (active_draw.primitive == kGxTriangleFan && count >= 3) {
        append_triangle(0, count - 2, count - 1);
    } else if (active_draw.primitive == kGxQuads && count % 4 == 0) {
        append_triangle(count - 4, count - 3, count - 2);
        append_triangle(count - 4, count - 2, count - 1);
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

void begin_locked(mh_u8 primitive, mh_u8 vertex_format,
                  mh_u16 vertex_count)
{
    submit_locked(MELEE_HOST_GX_U8,
                  static_cast<mh_u32>(primitive | vertex_format));
    submit_locked(MELEE_HOST_GX_U16, vertex_count);
    active_draw.active = vertex_count != 0;
    active_draw.primitive = primitive;
    active_draw.vertex_format = vertex_format;
    active_draw.expected_vertices = vertex_count;
    active_draw.captured_vertex_start = captured_vertices.size();
    active_draw.positions.clear();
    active_draw.positions.reserve(vertex_count);
    active_draw.vertices.clear();
    active_draw.vertices.reserve(vertex_count);
}

bool valid_attribute(GXAttr attribute)
{
    return static_cast<unsigned>(attribute) <
           static_cast<unsigned>(GX_VA_MAX_ATTR);
}

bool valid_vertex_format(GXVtxFmt format)
{
    return static_cast<unsigned>(format) <
           static_cast<unsigned>(GX_MAX_VTXFMT);
}

mh_u16 read_be_u16(const std::byte* source)
{
    return static_cast<mh_u16>((std::to_integer<mh_u16>(source[0]) << 8U) |
                               std::to_integer<mh_u16>(source[1]));
}

mh_u32 read_be_u32(const std::byte* source)
{
    return (std::to_integer<mh_u32>(source[0]) << 24U) |
           (std::to_integer<mh_u32>(source[1]) << 16U) |
           (std::to_integer<mh_u32>(source[2]) << 8U) |
           std::to_integer<mh_u32>(source[3]);
}

std::size_t component_size(GXCompType type)
{
    switch (type) {
    case GX_U8:
    case GX_S8:
        return 1;
    case GX_U16:
    case GX_S16:
        return 2;
    case GX_F32:
        return 4;
    default:
        return 0;
    }
}

mh_f32 decode_component(const std::byte* source, GXCompType type,
                        mh_u8 fractional_bits)
{
    mh_f32 value = 0.0F;
    switch (type) {
    case GX_U8:
        value = static_cast<mh_f32>(std::to_integer<mh_u8>(source[0]));
        break;
    case GX_S8:
        value = static_cast<mh_f32>(
            static_cast<std::int8_t>(std::to_integer<mh_u8>(source[0])));
        break;
    case GX_U16:
        value = static_cast<mh_f32>(read_be_u16(source));
        break;
    case GX_S16:
        value = static_cast<mh_f32>(
            static_cast<std::int16_t>(read_be_u16(source)));
        break;
    case GX_F32:
        return std::bit_cast<mh_f32>(read_be_u32(source));
    default:
        return 0.0F;
    }
    return std::ldexp(value, -static_cast<int>(fractional_bits));
}

const std::byte* indexed_data(GXAttr attribute, mh_u16 index,
                              GXAttrType expected_type)
{
    if (!valid_attribute(attribute)) {
        return nullptr;
    }
    const std::size_t attribute_index = static_cast<std::size_t>(attribute);
    const AttributeArray& array = attribute_arrays[attribute_index];
    if (vertex_descriptors[attribute_index] != expected_type ||
        array.base == nullptr || array.stride == 0)
    {
        return nullptr;
    }
    const std::size_t offset = static_cast<std::size_t>(index) * array.stride;
    if (array.byte_length != kUnboundedArraySize &&
        (offset > array.byte_length ||
         array.stride > array.byte_length - offset))
    {
        return nullptr;
    }
    return array.base + offset;
}

const AttributeFormat* active_format(GXAttr attribute)
{
    if (!valid_attribute(attribute) ||
        active_draw.vertex_format >= kVertexFormatCount)
    {
        return nullptr;
    }
    return &attribute_formats[active_draw.vertex_format]
                             [static_cast<std::size_t>(attribute)];
}

void capture_position(mh_f32 x, mh_f32 y, mh_f32 z)
{
    if (!active_draw.active ||
        active_draw.positions.size() >= active_draw.expected_vertices)
    {
        return;
    }
    active_draw.positions.push_back({ x, y, z });
    MeleeHostGxCapturedVertex vertex{};
    vertex.attributes = MELEE_HOST_GX_VERTEX_POSITION;
    vertex.position = { x, y, z };
    active_draw.vertices.push_back(vertex);
    captured_vertices.push_back(vertex);
    assemble_latest_position();
}

void capture_normal(mh_f32 x, mh_f32 y, mh_f32 z)
{
    if (active_draw.active && !active_draw.vertices.empty()) {
        auto& vertex = active_draw.vertices.back();
        vertex.attributes |= MELEE_HOST_GX_VERTEX_NORMAL;
        vertex.normal = { x, y, z };
        captured_vertices.back() = vertex;
    }
}

void capture_nbt(const MeleeHostGxPosition3f32& normal,
                 const MeleeHostGxPosition3f32& tangent,
                 const MeleeHostGxPosition3f32& binormal)
{
    if (active_draw.active && !active_draw.vertices.empty()) {
        auto& vertex = active_draw.vertices.back();
        vertex.attributes |= MELEE_HOST_GX_VERTEX_NORMAL |
                             MELEE_HOST_GX_VERTEX_TANGENT |
                             MELEE_HOST_GX_VERTEX_BINORMAL;
        vertex.normal = normal;
        vertex.tangent = tangent;
        vertex.binormal = binormal;
        captured_vertices.back() = vertex;
    }
}

void capture_color(mh_u8 red, mh_u8 green, mh_u8 blue, mh_u8 alpha)
{
    if (active_draw.active && !active_draw.vertices.empty()) {
        auto& vertex = active_draw.vertices.back();
        vertex.attributes |= MELEE_HOST_GX_VERTEX_COLOR;
        vertex.color[0] = red;
        vertex.color[1] = green;
        vertex.color[2] = blue;
        vertex.color[3] = alpha;
        captured_vertices.back() = vertex;
    }
}

void capture_texcoord(mh_f32 s, mh_f32 t)
{
    if (active_draw.active && !active_draw.vertices.empty()) {
        auto& vertex = active_draw.vertices.back();
        vertex.attributes |= MELEE_HOST_GX_VERTEX_TEXCOORD;
        vertex.texcoord[0] = s;
        vertex.texcoord[1] = t;
        captured_vertices.back() = vertex;
    }
}

void decode_position_index(mh_u16 index, GXAttrType index_type)
{
    const std::byte* source = indexed_data(GX_VA_POS, index, index_type);
    const AttributeFormat* format = active_format(GX_VA_POS);
    if (source == nullptr || format == nullptr) {
        return;
    }
    const std::size_t size = component_size(format->component_type);
    if (size == 0) {
        return;
    }
    const mh_f32 x = decode_component(source, format->component_type,
                                      format->fractional_bits);
    const mh_f32 y = decode_component(source + size, format->component_type,
                                      format->fractional_bits);
    const mh_f32 z = format->component_count == GX_POS_XYZ
                         ? decode_component(source + 2 * size,
                                            format->component_type,
                                            format->fractional_bits)
                         : 0.0F;
    capture_position(x, y, z);
}

MeleeHostGxPosition3f32 decode_normal(const std::byte* source,
                                      const AttributeFormat& format,
                                      std::size_t offset)
{
    const std::size_t size = component_size(format.component_type);
    return {
        decode_component(source + offset, format.component_type,
                         format.fractional_bits),
        decode_component(source + offset + size, format.component_type,
                         format.fractional_bits),
        decode_component(source + offset + 2 * size, format.component_type,
                         format.fractional_bits),
    };
}

void decode_normal_index(GXAttr attribute, mh_u16 index, GXAttrType index_type,
                         std::size_t nbt3_component = 0)
{
    const std::byte* source = indexed_data(attribute, index, index_type);
    const AttributeFormat* format = active_format(attribute);
    if (source == nullptr || format == nullptr)
    {
        return;
    }
    const std::size_t size = component_size(format->component_type);
    if (size == 0) {
        return;
    }
    if (attribute == GX_VA_NRM) {
        const auto normal = decode_normal(source, *format, 0);
        capture_normal(normal.x, normal.y, normal.z);
    } else if (format->component_count == GX_NRM_NBT) {
        capture_nbt(decode_normal(source, *format, 0),
                    decode_normal(source, *format, 3 * size),
                    decode_normal(source, *format, 6 * size));
    } else if (format->component_count == GX_NRM_NBT3) {
        const auto vector = decode_normal(source, *format, 0);
        if (nbt3_component == 0) {
            capture_normal(vector.x, vector.y, vector.z);
        } else if (active_draw.active && !active_draw.vertices.empty()) {
            auto& vertex = active_draw.vertices.back();
            if (nbt3_component == 1) {
                vertex.attributes |= MELEE_HOST_GX_VERTEX_TANGENT;
                vertex.tangent = vector;
            } else {
                vertex.attributes |= MELEE_HOST_GX_VERTEX_BINORMAL;
                vertex.binormal = vector;
            }
            captured_vertices.back() = vertex;
        }
    }
}

mh_u8 expand_bits(mh_u32 value, unsigned bits)
{
    const mh_u32 maximum = (1U << bits) - 1U;
    return static_cast<mh_u8>((value * 255U + maximum / 2U) / maximum);
}

void decode_color_data(const std::byte* source, const AttributeFormat& format)
{
    mh_u8 red = 0;
    mh_u8 green = 0;
    mh_u8 blue = 0;
    mh_u8 alpha = 255;
    switch (format.component_type) {
    case GX_RGB565: {
        const mh_u16 packed = read_be_u16(source);
        red = expand_bits((packed >> 11U) & 0x1FU, 5);
        green = expand_bits((packed >> 5U) & 0x3FU, 6);
        blue = expand_bits(packed & 0x1FU, 5);
        break;
    }
    case GX_RGB8:
        red = std::to_integer<mh_u8>(source[0]);
        green = std::to_integer<mh_u8>(source[1]);
        blue = std::to_integer<mh_u8>(source[2]);
        break;
    case GX_RGBX8:
        red = std::to_integer<mh_u8>(source[0]);
        green = std::to_integer<mh_u8>(source[1]);
        blue = std::to_integer<mh_u8>(source[2]);
        break;
    case GX_RGBA4: {
        const mh_u16 packed = read_be_u16(source);
        red = expand_bits((packed >> 12U) & 0xFU, 4);
        green = expand_bits((packed >> 8U) & 0xFU, 4);
        blue = expand_bits((packed >> 4U) & 0xFU, 4);
        alpha = expand_bits(packed & 0xFU, 4);
        break;
    }
    case GX_RGBA6: {
        const mh_u32 packed =
            (std::to_integer<mh_u32>(source[0]) << 16U) |
            (std::to_integer<mh_u32>(source[1]) << 8U) |
            std::to_integer<mh_u32>(source[2]);
        red = expand_bits((packed >> 18U) & 0x3FU, 6);
        green = expand_bits((packed >> 12U) & 0x3FU, 6);
        blue = expand_bits((packed >> 6U) & 0x3FU, 6);
        alpha = expand_bits(packed & 0x3FU, 6);
        break;
    }
    case GX_RGBA8:
        red = std::to_integer<mh_u8>(source[0]);
        green = std::to_integer<mh_u8>(source[1]);
        blue = std::to_integer<mh_u8>(source[2]);
        alpha = std::to_integer<mh_u8>(source[3]);
        break;
    default:
        return;
    }
    capture_color(red, green, blue, alpha);
}

void decode_color_index(mh_u16 index, GXAttrType index_type)
{
    const std::byte* source = indexed_data(GX_VA_CLR0, index, index_type);
    const AttributeFormat* format = active_format(GX_VA_CLR0);
    if (source != nullptr && format != nullptr) {
        decode_color_data(source, *format);
    }
}

void decode_texcoord_index(mh_u16 index, GXAttrType index_type)
{
    const std::byte* source = indexed_data(GX_VA_TEX0, index, index_type);
    const AttributeFormat* format = active_format(GX_VA_TEX0);
    if (source == nullptr || format == nullptr) {
        return;
    }
    const std::size_t size = component_size(format->component_type);
    if (size == 0) {
        return;
    }
    const mh_f32 s = decode_component(source, format->component_type,
                                      format->fractional_bits);
    const mh_f32 t = format->component_count == GX_TEX_ST
                         ? decode_component(source + size,
                                            format->component_type,
                                            format->fractional_bits)
                         : 0.0F;
    capture_texcoord(s, t);
}

bool is_matrix_index(GXAttr attribute)
{
    return attribute >= GX_VA_PNMTXIDX && attribute <= GX_VA_TEX7MTXIDX;
}

bool is_color(GXAttr attribute)
{
    return attribute == GX_VA_CLR0 || attribute == GX_VA_CLR1;
}

bool is_texcoord(GXAttr attribute)
{
    return attribute >= GX_VA_TEX0 && attribute <= GX_VA_TEX7;
}

std::size_t packed_color_size(GXCompType type)
{
    switch (type) {
    case GX_RGB565:
    case GX_RGBA4:
        return 2;
    case GX_RGB8:
    case GX_RGBA6:
        return 3;
    case GX_RGBX8:
    case GX_RGBA8:
        return 4;
    default:
        return 0;
    }
}

std::size_t direct_attribute_size(GXAttr attribute,
                                  const AttributeFormat& format)
{
    if (is_matrix_index(attribute)) {
        return 1;
    }
    if (attribute == GX_VA_POS) {
        const std::size_t size = component_size(format.component_type);
        return size * (format.component_count == GX_POS_XYZ ? 3U : 2U);
    }
    if (attribute == GX_VA_NRM || attribute == GX_VA_NBT) {
        const std::size_t size = component_size(format.component_type);
        return size * (format.component_count == GX_NRM_XYZ ? 3U : 9U);
    }
    if (is_color(attribute)) {
        return packed_color_size(format.component_type);
    }
    if (is_texcoord(attribute)) {
        const std::size_t size = component_size(format.component_type);
        return size * (format.component_count == GX_TEX_ST ? 2U : 1U);
    }
    return 0;
}

void decode_direct_attribute(GXAttr attribute, const std::byte* source,
                             const AttributeFormat& format)
{
    if (attribute == GX_VA_POS) {
        const std::size_t size = component_size(format.component_type);
        const mh_f32 x = decode_component(source, format.component_type,
                                          format.fractional_bits);
        const mh_f32 y = decode_component(source + size, format.component_type,
                                          format.fractional_bits);
        const mh_f32 z = format.component_count == GX_POS_XYZ
                             ? decode_component(source + 2 * size,
                                                format.component_type,
                                                format.fractional_bits)
                             : 0.0F;
        capture_position(x, y, z);
    } else if (attribute == GX_VA_NRM || attribute == GX_VA_NBT)
    {
        const std::size_t size = component_size(format.component_type);
        if (attribute == GX_VA_NRM) {
            const auto normal = decode_normal(source, format, 0);
            capture_normal(normal.x, normal.y, normal.z);
        } else {
            capture_nbt(decode_normal(source, format, 0),
                        decode_normal(source, format, 3 * size),
                        decode_normal(source, format, 6 * size));
        }
    } else if (attribute == GX_VA_CLR0) {
        decode_color_data(source, format);
    } else if (attribute == GX_VA_TEX0) {
        const std::size_t size = component_size(format.component_type);
        const mh_f32 s = decode_component(source, format.component_type,
                                          format.fractional_bits);
        const mh_f32 t = format.component_count == GX_TEX_ST
                             ? decode_component(source + size,
                                                format.component_type,
                                                format.fractional_bits)
                             : 0.0F;
        capture_texcoord(s, t);
    }
}

bool valid_draw_primitive(mh_u8 primitive)
{
    switch (primitive) {
    case GX_DRAW_QUADS:
    case GX_DRAW_TRIANGLES:
    case GX_DRAW_TRIANGLE_STRIP:
    case GX_DRAW_TRIANGLE_FAN:
    case GX_DRAW_LINES:
    case GX_DRAW_LINE_STRIP:
    case GX_DRAW_POINTS:
        return true;
    default:
        return false;
    }
}

bool consume_display_attribute(const std::byte*& cursor,
                               const std::byte* end, GXAttr attribute,
                               GXAttrType descriptor,
                               const AttributeFormat& format)
{
    if (descriptor == GX_DIRECT) {
        const std::size_t byte_count = direct_attribute_size(attribute, format);
        if (byte_count == 0 || static_cast<std::size_t>(end - cursor) < byte_count) {
            return false;
        }
        decode_direct_attribute(attribute, cursor, format);
        cursor += byte_count;
        return true;
    }

    if (descriptor != GX_INDEX8 && descriptor != GX_INDEX16) {
        return false;
    }
    const std::size_t index_size = descriptor == GX_INDEX8 ? 1U : 2U;
    const std::size_t index_count =
        (attribute == GX_VA_NRM || attribute == GX_VA_NBT) &&
                format.component_count == GX_NRM_NBT3
            ? 3U
            : 1U;
    if (static_cast<std::size_t>(end - cursor) < index_size * index_count) {
        return false;
    }

    for (std::size_t item = 0; item < index_count; ++item) {
        const mh_u16 index = descriptor == GX_INDEX8
                                 ? std::to_integer<mh_u8>(cursor[0])
                                 : read_be_u16(cursor);
        submit_locked(descriptor == GX_INDEX8 ? MELEE_HOST_GX_U8
                                              : MELEE_HOST_GX_U16,
                      index);
        if (item == 0 || (attribute == GX_VA_NBT &&
                          format.component_count == GX_NRM_NBT3)) {
            if (attribute == GX_VA_POS) {
                decode_position_index(index, descriptor);
            } else if (attribute == GX_VA_NRM || attribute == GX_VA_NBT) {
                decode_normal_index(attribute, index, descriptor, item);
            } else if (attribute == GX_VA_CLR0) {
                decode_color_index(index, descriptor);
            } else if (attribute == GX_VA_TEX0) {
                decode_texcoord_index(index, descriptor);
            }
        }
        cursor += index_size;
    }
    return true;
}

bool parse_display_list_locked(const std::byte* cursor, std::size_t byte_count)
{
    if (byte_count == 0) {
        return true;
    }
    if (cursor == nullptr && byte_count != 0) {
        return false;
    }
    const std::byte* const end = cursor + byte_count;
    while (cursor < end) {
        const mh_u8 opcode = std::to_integer<mh_u8>(*cursor++);
        if (opcode == GX_NOP) {
            return true;
        }
        const mh_u8 primitive = opcode & GX_OPCODE_MASK;
        const mh_u8 vertex_format = opcode & GX_VAT_MASK;
        if (!valid_draw_primitive(primitive) || end - cursor < 2) {
            return false;
        }
        const mh_u16 vertex_count = read_be_u16(cursor);
        cursor += 2;
        begin_locked(primitive, vertex_format, vertex_count);

        for (mh_u16 vertex = 0; vertex < vertex_count; ++vertex) {
            for (std::size_t index = 0; index < kAttributeCount; ++index) {
                const GXAttrType descriptor = vertex_descriptors[index];
                if (descriptor == GX_NONE) {
                    continue;
                }
                const GXAttr attribute = static_cast<GXAttr>(index);
                const AttributeFormat& format =
                    attribute_formats[vertex_format][index];
                if (!consume_display_attribute(cursor, end, attribute,
                                               descriptor, format))
                {
                    active_draw.active = false;
                    return false;
                }
            }
        }
        active_draw.active = false;
    }
    return true;
}

template <typename Index, typename Decoder>
void submit_index(Index index, MeleeHostGxValueType command_type,
                  GXAttrType index_type, Decoder decoder)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    submit_locked(command_type, static_cast<mh_u32>(index));
    decoder(static_cast<mh_u16>(index), index_type);
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
    begin_locked(primitive, vertex_format, vertex_count);
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
    capture_position(x, y, z);
}

extern "C" void melee_host_gx_submit_normal3f32(mh_f32 x, mh_f32 y,
                                                  mh_f32 z)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    submit_locked(MELEE_HOST_GX_F32, std::bit_cast<mh_u32>(x));
    submit_locked(MELEE_HOST_GX_F32, std::bit_cast<mh_u32>(y));
    submit_locked(MELEE_HOST_GX_F32, std::bit_cast<mh_u32>(z));
    capture_normal(x, y, z);
}

extern "C" void melee_host_gx_submit_color4u8(mh_u8 red, mh_u8 green,
                                                mh_u8 blue, mh_u8 alpha)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    submit_locked(MELEE_HOST_GX_U8, red);
    submit_locked(MELEE_HOST_GX_U8, green);
    submit_locked(MELEE_HOST_GX_U8, blue);
    submit_locked(MELEE_HOST_GX_U8, alpha);
    capture_color(red, green, blue, alpha);
}

extern "C" void melee_host_gx_submit_texcoord2f32(mh_f32 s, mh_f32 t)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    submit_locked(MELEE_HOST_GX_F32, std::bit_cast<mh_u32>(s));
    submit_locked(MELEE_HOST_GX_F32, std::bit_cast<mh_u32>(t));
    capture_texcoord(s, t);
}

extern "C" void melee_host_gx_submit_position_index8(mh_u8 index)
{
    submit_index(index, MELEE_HOST_GX_U8, GX_INDEX8, decode_position_index);
}

extern "C" void melee_host_gx_submit_position_index16(mh_u16 index)
{
    submit_index(index, MELEE_HOST_GX_U16, GX_INDEX16, decode_position_index);
}

extern "C" void melee_host_gx_submit_normal_index8(mh_u8 index)
{
    submit_index(index, MELEE_HOST_GX_U8, GX_INDEX8,
                 [](mh_u16 value, GXAttrType type) {
                     decode_normal_index(GX_VA_NRM, value, type);
                 });
}

extern "C" void melee_host_gx_submit_normal_index16(mh_u16 index)
{
    submit_index(index, MELEE_HOST_GX_U16, GX_INDEX16,
                 [](mh_u16 value, GXAttrType type) {
                     decode_normal_index(GX_VA_NRM, value, type);
                 });
}

extern "C" void melee_host_gx_submit_color_index8(mh_u8 index)
{
    submit_index(index, MELEE_HOST_GX_U8, GX_INDEX8, decode_color_index);
}

extern "C" void melee_host_gx_submit_color_index16(mh_u16 index)
{
    submit_index(index, MELEE_HOST_GX_U16, GX_INDEX16, decode_color_index);
}

extern "C" void melee_host_gx_submit_texcoord_index8(mh_u8 index)
{
    submit_index(index, MELEE_HOST_GX_U8, GX_INDEX8, decode_texcoord_index);
}

extern "C" void melee_host_gx_submit_texcoord_index16(mh_u16 index)
{
    submit_index(index, MELEE_HOST_GX_U16, GX_INDEX16,
                 decode_texcoord_index);
}

extern "C" void GXSetVtxDesc(GXAttr attribute, GXAttrType type)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    if (valid_attribute(attribute)) {
        vertex_descriptors[static_cast<std::size_t>(attribute)] = type;
    }
}

extern "C" void GXSetVtxDescv(const GXVtxDescList* list)
{
    if (list == nullptr) {
        return;
    }
    const std::lock_guard<std::mutex> lock(command_mutex);
    for (; list->attr != GX_VA_NULL; ++list) {
        if (valid_attribute(list->attr)) {
            vertex_descriptors[static_cast<std::size_t>(list->attr)] =
                list->type;
        }
    }
}

extern "C" void GXClearVtxDesc(void)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    vertex_descriptors.fill(GX_NONE);
}

extern "C" void GXSetVtxAttrFmt(GXVtxFmt vertex_format, GXAttr attribute,
                                  GXCompCnt component_count,
                                  GXCompType component_type, u8 fractional_bits)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    if (valid_vertex_format(vertex_format) && valid_attribute(attribute)) {
        attribute_formats[static_cast<std::size_t>(vertex_format)]
                         [static_cast<std::size_t>(attribute)] = {
                             component_count, component_type, fractional_bits
                         };
    }
}

extern "C" void GXSetVtxAttrFmtv(GXVtxFmt vertex_format,
                                   const GXVtxAttrFmtList* list)
{
    if (list == nullptr || !valid_vertex_format(vertex_format)) {
        return;
    }
    const std::lock_guard<std::mutex> lock(command_mutex);
    for (; list->attr != GX_VA_NULL; ++list) {
        if (valid_attribute(list->attr)) {
            attribute_formats[static_cast<std::size_t>(vertex_format)]
                             [static_cast<std::size_t>(list->attr)] = {
                                 list->cnt, list->type, list->frac
                             };
        }
    }
}

extern "C" void GXSetArray(GXAttr attribute, const void* base, u8 stride)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    if (valid_attribute(attribute)) {
        attribute_arrays[static_cast<std::size_t>(attribute)] = {
            static_cast<const std::byte*>(base), stride, kUnboundedArraySize
        };
    }
}

extern "C" void melee_host_gx_set_array_bounded(mh_u32 attribute,
                                                   const void* base,
                                                   size_t byte_length,
                                                   mh_u8 stride)
{
    const auto gx_attribute = static_cast<GXAttr>(attribute);
    const std::lock_guard<std::mutex> lock(command_mutex);
    if (valid_attribute(gx_attribute)) {
        attribute_arrays[static_cast<std::size_t>(gx_attribute)] = {
            static_cast<const std::byte*>(base), stride, byte_length
        };
    }
}

extern "C" void GXInvalidateVtxCache(void)
{
    // Desktop arrays are read at submission time, so there is no vertex cache.
}

extern "C" void GXCallDisplayList(void* list, u32 byte_count)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    if (!parse_display_list_locked(static_cast<const std::byte*>(list),
                                   byte_count))
    {
        ++display_list_errors;
    }
}

extern "C" void melee_host_gx_reset_command_log(void)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    commands.clear();
    triangles.clear();
    captured_vertices.clear();
    captured_triangle_indices.clear();
    display_list_errors = 0;
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

extern "C" bool melee_host_gx_captured_triangle_at(
    size_t index, MeleeHostGxCapturedTriangle* output)
{
    if (output == nullptr) {
        return false;
    }
    const std::lock_guard<std::mutex> lock(command_mutex);
    if (index >= captured_triangle_indices.size()) {
        return false;
    }
    const auto& indices = captured_triangle_indices[index];
    for (std::size_t vertex = 0; vertex < indices.size(); ++vertex) {
        if (indices[vertex] >= captured_vertices.size()) {
            return false;
        }
        output->vertices[vertex] = captured_vertices[indices[vertex]];
    }
    return true;
}

extern "C" size_t melee_host_gx_captured_vertex_count(void)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    return captured_vertices.size();
}

extern "C" bool melee_host_gx_captured_vertex_at(
    size_t index, MeleeHostGxCapturedVertex* output)
{
    if (output == nullptr) {
        return false;
    }
    const std::lock_guard<std::mutex> lock(command_mutex);
    if (index >= captured_vertices.size()) {
        return false;
    }
    *output = captured_vertices[index];
    return true;
}

extern "C" size_t melee_host_gx_display_list_error_count(void)
{
    const std::lock_guard<std::mutex> lock(command_mutex);
    return display_list_errors;
}

extern "C" void melee_host_gx_transform_vertices(
    size_t first, size_t count, const MeleeHostGxAffineTransform* matrix)
{
    if (matrix == nullptr) {
        return;
    }
    const std::lock_guard<std::mutex> lock(command_mutex);
    if (first > captured_vertices.size() ||
        count > captured_vertices.size() - first)
    {
        return;
    }
    const mh_f32 a = matrix->values[0][0];
    const mh_f32 b = matrix->values[0][1];
    const mh_f32 c = matrix->values[0][2];
    const mh_f32 d = matrix->values[1][0];
    const mh_f32 e = matrix->values[1][1];
    const mh_f32 f = matrix->values[1][2];
    const mh_f32 g = matrix->values[2][0];
    const mh_f32 h = matrix->values[2][1];
    const mh_f32 i = matrix->values[2][2];
    const mh_f32 determinant = a * (e * i - f * h) -
                               b * (d * i - f * g) +
                               c * (d * h - e * g);
    const auto transform_direction = [=](const MeleeHostGxPosition3f32& source) {
        return MeleeHostGxPosition3f32{
            a * source.x + b * source.y + c * source.z,
            d * source.x + e * source.y + f * source.z,
            g * source.x + h * source.y + i * source.z,
        };
    };
    const auto transform_normal = [=](const MeleeHostGxPosition3f32& source) {
        if (std::fabs(determinant) < 1.0e-8F) {
            return transform_direction(source);
        }
        return MeleeHostGxPosition3f32{
            ((e * i - f * h) * source.x + (f * g - d * i) * source.y +
             (d * h - e * g) * source.z) / determinant,
            ((c * h - b * i) * source.x + (a * i - c * g) * source.y +
             (b * g - a * h) * source.z) / determinant,
            ((b * f - c * e) * source.x + (c * d - a * f) * source.y +
             (a * e - b * d) * source.z) / determinant,
        };
    };
    const auto normalize = [](MeleeHostGxPosition3f32 vector) {
        const mh_f32 length = std::sqrt(vector.x * vector.x + vector.y * vector.y +
                                        vector.z * vector.z);
        if (length > 0.0F) {
            vector.x /= length;
            vector.y /= length;
            vector.z /= length;
        }
        return vector;
    };
    for (size_t index = first; index < first + count; ++index) {
        auto& vertex = captured_vertices[index];
        if ((vertex.attributes & MELEE_HOST_GX_VERTEX_POSITION) != 0) {
            const auto source = vertex.position;
            vertex.position = {
                matrix->values[0][0] * source.x + matrix->values[0][1] * source.y +
                    matrix->values[0][2] * source.z + matrix->values[0][3],
                matrix->values[1][0] * source.x + matrix->values[1][1] * source.y +
                    matrix->values[1][2] * source.z + matrix->values[1][3],
                matrix->values[2][0] * source.x + matrix->values[2][1] * source.y +
                    matrix->values[2][2] * source.z + matrix->values[2][3],
            };
        }
        if ((vertex.attributes & MELEE_HOST_GX_VERTEX_NORMAL) != 0) {
            vertex.normal = normalize(transform_normal(vertex.normal));
        }
        if ((vertex.attributes & MELEE_HOST_GX_VERTEX_TANGENT) != 0) {
            vertex.tangent = normalize(transform_direction(vertex.tangent));
        }
        if ((vertex.attributes & MELEE_HOST_GX_VERTEX_BINORMAL) != 0) {
            vertex.binormal = normalize(transform_direction(vertex.binormal));
        }
    }
    for (size_t index = 0; index < triangles.size(); ++index) {
        const auto& indices = captured_triangle_indices[index];
        triangles[index] = { { captured_vertices[indices[0]].position,
                               captured_vertices[indices[1]].position,
                               captured_vertices[indices[2]].position } };
    }
}

extern "C" void melee_host_gx_apply_material(size_t first, size_t count,
                                               const mh_u8 diffuse[4],
                                               mh_u32 texture_image,
                                               mh_u32 render_mode)
{
    if (diffuse == nullptr) {
        return;
    }
    const std::lock_guard<std::mutex> lock(command_mutex);
    if (first > captured_vertices.size() ||
        count > captured_vertices.size() - first)
    {
        return;
    }
    for (size_t index = first; index < first + count; ++index) {
        auto& vertex = captured_vertices[index];
        for (size_t channel = 0; channel < 4; ++channel) {
            const mh_u8 source =
                (vertex.attributes & MELEE_HOST_GX_VERTEX_COLOR) != 0
                    ? vertex.color[channel]
                    : static_cast<mh_u8>(255);
            vertex.color[channel] = static_cast<mh_u8>(
                (static_cast<mh_u16>(source) * diffuse[channel] + 127U) / 255U);
        }
        vertex.attributes |= MELEE_HOST_GX_VERTEX_COLOR;
        if (texture_image != MELEE_HOST_GX_NO_TEXTURE) {
            vertex.attributes |= MELEE_HOST_GX_VERTEX_TEXTURE_IMAGE;
            vertex.texture_image = texture_image;
        }
        vertex.render_mode = render_mode;
    }
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
