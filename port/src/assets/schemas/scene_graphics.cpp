#include "assets/schemas/scene_graphics.hpp"

#include <cmath>
#include <unordered_set>

namespace melee::assets::schemas {
namespace {

constexpr std::uint32_t kJObjParticle = 1U << 5U;
constexpr std::uint32_t kJObjSpline = 1U << 14U;
constexpr std::uint32_t kJObjMtxIndependentParent = 1U << 24U;
constexpr std::uint32_t kVertexDescriptorSize = 0x18;
constexpr std::uint32_t kNullAttribute = 0xFF;
constexpr std::uint32_t kDirectAttribute = 1;
constexpr std::size_t kTraversalLimit = 16384;
constexpr std::size_t kVertexDescriptorLimit = 64;

std::optional<HsdRuntimeNode> optional_reference(
    const HsdRuntimeArchive& archive, HsdRuntimeNode node,
    std::uint32_t relative_offset)
{
    if (!archive.has_reference_at(node, relative_offset)) {
        return std::nullopt;
    }
    return archive.reference_at(node, relative_offset);
}

std::vector<HsdVertexDescriptor> decode_vertex_descriptors(
    const HsdRuntimeArchive& archive, HsdRuntimeNode begin)
{
    std::vector<HsdVertexDescriptor> result;
    for (std::size_t index = 0; index < kVertexDescriptorLimit; ++index) {
        const std::uint32_t relative =
            static_cast<std::uint32_t>(index) * kVertexDescriptorSize;
        const HsdRuntimeNode descriptor{ begin.data_offset + relative };
        const std::uint32_t attribute = archive.read_u32(descriptor, 0);
        if (attribute == kNullAttribute) {
            return result;
        }
        const std::uint32_t attribute_type = archive.read_u32(descriptor, 4);
        std::optional<HsdRuntimeNode> array;
        if (attribute_type != kDirectAttribute) {
            array = optional_reference(archive, descriptor, 0x14);
            if (!array.has_value()) {
                throw HsdArchiveError(
                    "indexed HSD vertex descriptor has no array reference");
            }
        }
        const auto fraction = archive.bytes_at(
            { descriptor.data_offset + 0x10 }, 1);
        result.push_back({
            attribute,
            attribute_type,
            archive.read_u32(descriptor, 8),
            archive.read_u32(descriptor, 0xC),
            std::to_integer<std::uint8_t>(fraction[0]),
            archive.read_u16(descriptor, 0x12),
            array,
        });
    }
    throw HsdArchiveError("HSD vertex descriptor list has no terminator");
}

HsdAffineTransform identity_transform()
{
    return { { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 } } };
}

HsdAffineTransform concat(const HsdAffineTransform& left,
                          const HsdAffineTransform& right)
{
    HsdAffineTransform result{};
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            result.values[row][column] =
                left.values[row][0] * right.values[0][column] +
                left.values[row][1] * right.values[1][column] +
                left.values[row][2] * right.values[2][column];
        }
        result.values[row][3] = left.values[row][0] * right.values[0][3] +
                                left.values[row][1] * right.values[1][3] +
                                left.values[row][2] * right.values[2][3] +
                                left.values[row][3];
    }
    return result;
}

HsdAffineTransform joint_transform(const HsdRuntimeArchive& archive,
                                   HsdRuntimeNode joint)
{
    const float x = archive.read_f32(joint, 0x14);
    const float y = archive.read_f32(joint, 0x18);
    const float z = archive.read_f32(joint, 0x1C);
    const float sx = archive.read_f32(joint, 0x20);
    const float sy = archive.read_f32(joint, 0x24);
    const float sz = archive.read_f32(joint, 0x28);
    const float tx = archive.read_f32(joint, 0x2C);
    const float ty = archive.read_f32(joint, 0x30);
    const float tz = archive.read_f32(joint, 0x34);
    const float sin_x = std::sin(x);
    const float cos_x = std::cos(x);
    const float sin_y = std::sin(y);
    const float cos_y = std::cos(y);
    const float sin_z = std::sin(z);
    const float cos_z = std::cos(z);
    return { { { cos_z * sx * cos_y,
                 sy * (cos_z * sin_x * sin_y - cos_x * sin_z),
                 sz * (cos_z * cos_x * sin_y + sin_x * sin_z), tx },
               { sin_z * sx * cos_y,
                 sy * (sin_z * sin_x * sin_y + cos_x * cos_z),
                 sz * (sin_z * cos_x * sin_y - sin_x * cos_z), ty },
               { -sx * sin_y, sy * sin_x * cos_y, sz * cos_x * cos_y, tz } } };
}

HsdMaterial decode_material(const HsdRuntimeArchive& archive,
                            std::optional<HsdRuntimeNode> mobj)
{
    constexpr HsdMaterial default_material{
        0, { 255, 255, 255, 255 }, false, std::nullopt, std::nullopt,
        0, 0, 0, 0, 0, 0, 0
    };
    if (!mobj.has_value()) {
        return default_material;
    }
    const auto tobj = optional_reference(archive, *mobj, 8);
    HsdMaterial material{ archive.read_u32(*mobj, 4), { 255, 255, 255, 255 },
                          tobj.has_value(), std::nullopt, std::nullopt,
                          0, 0, 0, 0, 0, 0, 0 };
    const auto material_desc = optional_reference(archive, *mobj, 0xC);
    if (material_desc.has_value()) {
        const auto diffuse =
            archive.bytes_at({ material_desc->data_offset + 4 }, 4);
        for (std::size_t channel = 0; channel < material.diffuse.size(); ++channel) {
            material.diffuse[channel] =
                std::to_integer<std::uint8_t>(diffuse[channel]);
        }
        const float alpha = archive.read_f32(*material_desc, 0xC);
        if (alpha >= 0.0F && alpha <= 1.0F) {
            material.diffuse[3] =
                static_cast<std::uint8_t>(alpha * 255.0F + 0.5F);
        }
    }
    if (!tobj.has_value()) {
        return material;
    }
    material.texture_wrap_s = archive.read_u32(*tobj, 0x34);
    material.texture_wrap_t = archive.read_u32(*tobj, 0x38);
    const auto image = optional_reference(archive, *tobj, 0x4C);
    if (!image.has_value()) {
        return material;
    }
    material.image_data = optional_reference(archive, *image, 0);
    material.texture_width = archive.read_u16(*image, 4);
    material.texture_height = archive.read_u16(*image, 6);
    material.texture_format = archive.read_u32(*image, 8);
    const auto tlut = optional_reference(archive, *tobj, 0x50);
    if (tlut.has_value()) {
        material.tlut_data = optional_reference(archive, *tlut, 0);
        material.tlut_format = archive.read_u32(*tlut, 4);
        material.tlut_entries = archive.read_u16(*tlut, 0xC);
    }
    return material;
}

HsdPObjGeometry decode_pobj(const HsdRuntimeArchive& archive,
                            HsdRuntimeNode descriptor,
                            const HsdAffineTransform& transform,
                            HsdMaterial material)
{
    const auto vertices = optional_reference(archive, descriptor, 8);
    const auto display = optional_reference(archive, descriptor, 0x10);
    if (!vertices.has_value() || !display.has_value()) {
        throw HsdArchiveError("HSD PObj is missing geometry references");
    }
    return {
        descriptor,
        archive.read_u16(descriptor, 0xC),
        archive.read_u16(descriptor, 0xE),
        *display,
        transform,
        material,
        decode_vertex_descriptors(archive, *vertices),
    };
}

} // namespace

std::vector<HsdPObjGeometry> find_scene_pobjs(
    const HsdRuntimeArchive& archive, std::string_view public_symbol)
{
    const HsdRuntimeNode scene = archive.public_root(public_symbol);
    const auto models = optional_reference(archive, scene, 0);
    if (!models.has_value()) {
        throw HsdArchiveError("HSD scene has no model table");
    }

    struct PendingJoint {
        HsdRuntimeNode node;
        HsdAffineTransform parent_transform;
    };
    std::vector<PendingJoint> joints;
    for (std::size_t index = 0; index < kTraversalLimit; ++index) {
        const std::uint32_t relative = static_cast<std::uint32_t>(index * 4);
        const auto model = optional_reference(archive, *models, relative);
        if (!model.has_value()) {
            break;
        }
        const auto joint = optional_reference(archive, *model, 0);
        if (joint.has_value()) {
            joints.push_back({ *joint, identity_transform() });
        }
    }

    std::unordered_set<std::uint32_t> visited_joints;
    std::unordered_set<std::uint32_t> visited_dobjs;
    std::unordered_set<std::uint32_t> visited_pobjs;
    std::vector<HsdPObjGeometry> result;
    std::size_t traversed = 0;
    while (!joints.empty() && traversed++ < kTraversalLimit) {
        const PendingJoint pending = joints.back();
        joints.pop_back();
        const HsdRuntimeNode joint = pending.node;
        if (!visited_joints.insert(joint.data_offset).second) {
            continue;
        }
        const HsdAffineTransform local = joint_transform(archive, joint);
        const std::uint32_t flags = archive.read_u32(joint, 4);
        const HsdAffineTransform world =
            (flags & kJObjMtxIndependentParent) != 0 ? local
                                                     : concat(pending.parent_transform, local);
        if (const auto next = optional_reference(archive, joint, 0xC)) {
            joints.push_back({ *next, pending.parent_transform });
        }
        if (const auto child = optional_reference(archive, joint, 8)) {
            joints.push_back({ *child, world });
        }
        if ((flags & (kJObjParticle | kJObjSpline)) != 0) {
            continue;
        }

        auto dobj = optional_reference(archive, joint, 0x10);
        while (dobj.has_value() && traversed++ < kTraversalLimit) {
            if (!visited_dobjs.insert(dobj->data_offset).second) {
                break;
            }
            const HsdMaterial material =
                decode_material(archive, optional_reference(archive, *dobj, 8));
            auto pobj = optional_reference(archive, *dobj, 0xC);
            while (pobj.has_value() && traversed++ < kTraversalLimit) {
                if (!visited_pobjs.insert(pobj->data_offset).second) {
                    break;
                }
                const std::uint16_t display_blocks =
                    archive.read_u16(*pobj, 0xE);
                if (display_blocks != 0) {
                    result.push_back(decode_pobj(archive, *pobj, world, material));
                }
                pobj = optional_reference(archive, *pobj, 4);
            }
            dobj = optional_reference(archive, *dobj, 4);
        }
    }
    if (result.empty()) {
        throw HsdArchiveError("HSD scene contains no drawable PObj");
    }
    return result;
}

HsdPObjGeometry find_first_scene_pobj(const HsdRuntimeArchive& archive,
                                      std::string_view public_symbol)
{
    return find_scene_pobjs(archive, public_symbol).front();
}

} // namespace melee::assets::schemas
