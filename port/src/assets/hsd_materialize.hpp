#ifndef MELEE_HOST_HSD_MATERIALIZE_HPP
#define MELEE_HOST_HSD_MATERIALIZE_HPP

#include "assets/hsd_runtime_archive.hpp"
#include "hsd_graphics_types.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <unordered_map>

namespace melee::assets {

struct HsdMaterializeStats {
    std::size_t joints;
    std::size_t dobj_descs;
    std::size_t mobj_descs;
    std::size_t tobj_descs;
    std::size_t pobj_descs;
    std::size_t vertex_descriptors;
    std::size_t image_descs;
    std::size_t tlut_descs;
    std::size_t robj_descs;
    std::size_t camera_descs;
    std::size_t shape_set_descs;
    std::size_t envelope_descs;
    std::size_t descriptor_bytes;
    std::size_t payload_bytes;
};

/*
 * Rebuilds HSD descriptors in host layout so the original object loaders can
 * walk them unchanged.
 *
 * The archive stores descriptors exactly as the PowerPC saw them: big-endian
 * scalars, and pointer fields holding a 32-bit offset that archive.c adds the
 * data base to, in place.  A 64-bit host cannot do that.  Every pointer field
 * would need eight bytes where the file has four, so relocating in place would
 * overwrite the field that follows.  Instead each descriptor is allocated
 * again in host layout and its pointers filled with real host addresses.
 *
 * GX payloads are not translated.  Display lists, vertex arrays, image data
 * and palettes keep their big-endian bytes, because the display-list
 * interpreter and the texture decoders read them in that order.  They live in
 * one verbatim copy of the data section, which is why pointers into them need
 * no length.
 *
 * Descriptors come out of a single contiguous block.  That is what keeps
 * `jobj->id = (u32) joint` usable: the original keys its ID table on the
 * truncated joint address, and truncation stays injective as long as every
 * joint shares the same high 32 bits.
 */
class HsdMaterializedArchive final {
public:
    explicit HsdMaterializedArchive(const HsdRuntimeArchive& archive);
    ~HsdMaterializedArchive();

    HsdMaterializedArchive(const HsdMaterializedArchive&) = delete;
    HsdMaterializedArchive& operator=(const HsdMaterializedArchive&) = delete;

    /* Materializes the joint tree rooted at a public symbol naming a joint. */
    [[nodiscard]] HSD_Joint* joint(std::string_view public_symbol);

    /* Materializes SceneDesc.cameras[index].desc, the camera the scene
     * carries.  Returns nullptr when the scene names no camera there. */
    [[nodiscard]] HSD_CObjDesc* scene_camera(std::string_view public_symbol,
                                             std::size_t camera_index);

    /* Materializes SceneDesc.models[index]->joint, the indirection the game's
     * own scene entry points walk. */
    [[nodiscard]] HSD_Joint* scene_model_joint(std::string_view public_symbol,
                                               std::size_t model_index);
    [[nodiscard]] std::size_t
    scene_model_count(std::string_view public_symbol) const;

    [[nodiscard]] const HsdMaterializeStats& stats() const noexcept;

private:
    template <typename T> T* allocate();
    void* allocate_bytes(std::size_t size, std::size_t alignment);

    [[nodiscard]] std::optional<HsdRuntimeNode>
    reference(HsdRuntimeNode node, std::uint32_t relative_offset) const;
    [[nodiscard]] void* payload(HsdRuntimeNode node,
                                std::size_t length) const;
    [[nodiscard]] char* payload_string(HsdRuntimeNode node) const;
    void read_vec3(HsdRuntimeNode node, std::uint32_t relative_offset,
                   Vec3* out) const;
    void read_color(HsdRuntimeNode node, std::uint32_t relative_offset,
                    GXColor* out) const;

    HSD_Joint* joint_chain(HsdRuntimeNode node);
    HSD_DObjDesc* dobj_chain(HsdRuntimeNode node);
    HSD_PObjDesc* pobj_chain(HsdRuntimeNode node);
    HSD_MObjDesc* mobj_desc(HsdRuntimeNode node);
    HSD_TObjDesc* tobj_chain(HsdRuntimeNode node);
    HSD_RObjDesc* robj_chain(HsdRuntimeNode node);
    HSD_CObjDesc* camera_desc(HsdRuntimeNode node);
    HSD_WObjDesc* world_desc(HsdRuntimeNode node);
    HSD_VtxDescList* vertex_descriptors(HsdRuntimeNode node);
    HSD_ShapeSetDesc* shape_set_desc(HsdRuntimeNode node);
    HSD_EnvelopeDesc** envelope_array(HsdRuntimeNode node);
    HSD_EnvelopeDesc* envelope_list(HsdRuntimeNode node);
    u8** payload_pointer_array(HsdRuntimeNode node, std::size_t count);
    HSD_ImageDesc* image_desc(HsdRuntimeNode node);
    HSD_TlutDesc* tlut_desc(HsdRuntimeNode node);
    HSD_Material* material(HsdRuntimeNode node);
    HSD_PEDesc* pixel_engine_desc(HsdRuntimeNode node);
    HSD_TexLODDesc* lod_desc(HsdRuntimeNode node);
    HSD_TObjTevDesc* tev_desc(HsdRuntimeNode node);
    f32* matrix(HsdRuntimeNode node);

    const HsdRuntimeArchive& archive_;
    std::byte* payload_ = nullptr;
    std::size_t payload_size_ = 0;
    std::byte* descriptors_ = nullptr;
    std::size_t descriptor_capacity_ = 0;
    std::size_t descriptor_used_ = 0;
    std::size_t depth_ = 0;

    std::unordered_map<std::uint32_t, HSD_Joint*> joints_;
    std::unordered_map<std::uint32_t, HSD_DObjDesc*> dobjs_;
    std::unordered_map<std::uint32_t, HSD_MObjDesc*> mobjs_;
    std::unordered_map<std::uint32_t, HSD_TObjDesc*> tobjs_;
    std::unordered_map<std::uint32_t, HSD_PObjDesc*> pobjs_;
    std::unordered_map<std::uint32_t, HSD_VtxDescList*> vertex_lists_;
    std::unordered_map<std::uint32_t, HSD_ImageDesc*> images_;
    std::unordered_map<std::uint32_t, HSD_TlutDesc*> tluts_;
    std::unordered_map<std::uint32_t, HSD_RObjDesc*> robjs_;
    std::unordered_map<std::uint32_t, HSD_ShapeSetDesc*> shape_sets_;
    std::unordered_map<std::uint32_t, HSD_EnvelopeDesc**> envelope_arrays_;
    HsdMaterializeStats stats_{};
};

} // namespace melee::assets

#endif
