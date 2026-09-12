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
    std::size_t anim_joints;
    std::size_t mat_anim_joints;
    std::size_t shape_anim_joints;
    std::size_t aobj_descs;
    std::size_t fobj_descs;
    std::size_t anim_data_bytes;
    std::size_t figa_trees;
    std::size_t figa_tracks;
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

    /* Materializes a FigaTree, the game's own animation container.  Unlike the
     * HSD trees this one is flat: a list saying how many tracks each bone
     * takes, and the tracks laid end to end.  A character keeps one per
     * action. */
    [[nodiscard]] FigaTree* figa_tree(std::string_view public_symbol);

    /* Materializes an animation tree rooted at a public symbol.  The three
     * kinds travel together: one drives the joints' transforms, one the
     * materials, one the shape blends.  Any of them can be absent. */
    [[nodiscard]] HSD_AnimJoint* anim_joint(std::string_view public_symbol);
    [[nodiscard]] HSD_MatAnimJoint* mat_anim_joint(
        std::string_view public_symbol);
    [[nodiscard]] HSD_ShapeAnimJoint* shape_anim_joint(
        std::string_view public_symbol);

    /* The animation tables a scene model carries, which is how a scene names
     * several animations for one model.  Index selects within the table. */
    [[nodiscard]] HSD_AnimJoint* scene_model_anim(
        std::string_view public_symbol, std::size_t model_index,
        std::size_t anim_index);
    [[nodiscard]] HSD_MatAnimJoint* scene_model_mat_anim(
        std::string_view public_symbol, std::size_t model_index,
        std::size_t anim_index);
    [[nodiscard]] HSD_ShapeAnimJoint* scene_model_shape_anim(
        std::string_view public_symbol, std::size_t model_index,
        std::size_t anim_index);
    [[nodiscard]] std::size_t scene_model_anim_count(
        std::string_view public_symbol, std::size_t model_index);

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
    [[nodiscard]] std::optional<HsdRuntimeNode> scene_model_anim_entry(
        std::string_view public_symbol, std::size_t model_index,
        std::uint32_t table_offset, std::size_t anim_index);
    HSD_AnimJoint* anim_joint_chain(HsdRuntimeNode node);
    HSD_MatAnimJoint* mat_anim_joint_chain(HsdRuntimeNode node);
    HSD_ShapeAnimJoint* shape_anim_joint_chain(HsdRuntimeNode node);
    HSD_MatAnim* mat_anim_chain(HsdRuntimeNode node);
    HSD_TexAnim* tex_anim_chain(HsdRuntimeNode node);
    HSD_RenderAnim* render_anim(HsdRuntimeNode node);
    HSD_ShapeAnimDObj* shape_anim_dobj_chain(HsdRuntimeNode node);
    HSD_ShapeAnim* shape_anim_chain(HsdRuntimeNode node);
    HSD_RObjAnimJoint* robj_anim_chain(HsdRuntimeNode node);
    template <typename T> T* anim_link_chain(HsdRuntimeNode node);
    HSD_AObjDesc* aobj_desc(HsdRuntimeNode node);
    FigaTree* figa_tree_at(HsdRuntimeNode node);
    HSD_FObjDesc* fobj_chain(HsdRuntimeNode node);
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
    std::unordered_map<std::uint32_t, HSD_AnimJoint*> anim_joints_;
    std::unordered_map<std::uint32_t, HSD_MatAnimJoint*> mat_anim_joints_;
    std::unordered_map<std::uint32_t, HSD_ShapeAnimJoint*> shape_anim_joints_;
    std::unordered_map<std::uint32_t, HSD_AObjDesc*> aobj_descs_;
    HsdMaterializeStats stats_{};
};

} // namespace melee::assets

#endif
