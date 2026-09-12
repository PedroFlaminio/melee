/* Loads a real HSD scene through the original object layer.
 *
 * The archive on disk holds PowerPC descriptors, so it cannot be relocated in
 * place on a 64-bit host; assets/hsd_materialize rebuilds them in host layout
 * first.  From there this facade calls HSD_JObjLoadJoint, which is the entry
 * point every scene in the game uses, and the object layer below it allocates
 * the JObj, DObj, MObj, TObj and PObj tree from the host heap.
 *
 * The materialized descriptors have to outlive the tree: the loaded objects
 * keep pointers into them for vertex descriptors, display lists, images and
 * palettes, exactly as they would point into an archive still resident in the
 * console's heap.  One handle therefore owns both.
 */

#include <melee_host/scene_graphics.h>

#include <melee_host/baselib.h>

#include "assets/hsd_archive.hpp"
#include "assets/hsd_materialize.hpp"
#include "assets/hsd_runtime_archive.hpp"

#include <melee_host/gx.h>

MELEE_HOST_HSD_BEGIN
#include <dolphin/gx/GXFrameBuffer.h>
#include <melee/lb/lbanim.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/object.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/fobj.h>
#include <sysdolphin/baselib/displayfunc.h>
#include <sysdolphin/baselib/state.h>
#include <sysdolphin/baselib/video.h>
MELEE_HOST_HSD_END

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <span>
#include <string>
#include <vector>

/* hsd_materialize.hpp pulls in the original jobj/dobj/mobj/tobj/pobj headers
 * through the host's include shim. */

namespace {

using melee::assets::HsdMaterializedArchive;
using melee::assets::HsdRuntimeArchive;

constexpr std::size_t kModelLimit = 16;
constexpr std::size_t kWalkLimit = 65536;

struct LoadedModel {
    std::vector<std::byte> bytes;
    std::unique_ptr<HsdRuntimeArchive> archive;
    std::unique_ptr<HsdMaterializedArchive> descriptors;
    HSD_Joint* root_joint = nullptr;
    HSD_CObjDesc* camera = nullptr;
    HSD_JObj* root = nullptr;
    /* An animation can live in a different archive from the model, so the
     * handle keeps that one alive too: the loaded AObjs point into its
     * descriptors and its keyframe streams. */
    std::vector<std::byte> animation_bytes;
    std::unique_ptr<HsdRuntimeArchive> animation_archive;
    std::unique_ptr<HsdMaterializedArchive> animation_descriptors;
    float animation_frame = 0.0F;
    mh_u32 animation_bones = 0;
    bool in_use = false;
    mh_u16 generation = 0;
};

std::array<LoadedModel, kModelLimit> models;
std::string last_error = "no error";

MeleeHostStatus fail(MeleeHostStatus status, std::string message)
{
    last_error = std::move(message);
    return status;
}

MeleeHostSceneModel handle_of(std::size_t index)
{
    return static_cast<MeleeHostSceneModel>(
        (static_cast<mh_u32>(models[index].generation) << 16) |
        static_cast<mh_u32>(index + 1));
}

LoadedModel* model_of(MeleeHostSceneModel model)
{
    const mh_u32 index = model & 0xFFFFU;
    if (index == 0 || index > kModelLimit) {
        return nullptr;
    }
    LoadedModel& slot = models[index - 1];
    if (!slot.in_use ||
        slot.generation != static_cast<mh_u16>(model >> 16)) {
        return nullptr;
    }
    return &slot;
}

LoadedModel* free_slot(std::size_t* out_index)
{
    for (std::size_t index = 0; index < kModelLimit; ++index) {
        if (!models[index].in_use) {
            *out_index = index;
            return &models[index];
        }
    }
    return nullptr;
}

std::vector<std::byte> read_file(const char* path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error(std::string("cannot open ") + path);
    }
    stream.seekg(0, std::ios::end);
    const std::streamoff size = stream.tellg();
    if (size < 0) {
        throw std::runtime_error(std::string("cannot size ") + path);
    }
    stream.seekg(0, std::ios::beg);
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    if (size != 0 &&
        !stream.read(reinterpret_cast<char*>(bytes.data()), size)) {
        throw std::runtime_error(std::string("cannot read ") + path);
    }
    return bytes;
}

/* Counts what the original loaders built, by walking the same links the
 * display pass walks. */
struct TreeWalk {
    MeleeHostSceneModelStats stats{};
    std::size_t visited = 0;

    void walk_pobjs(HSD_PObj* pobj, bool textured, bool drawn)
    {
        for (; pobj != nullptr && visited++ < kWalkLimit; pobj = pobj->next) {
            stats.pobjs += 1;
            stats.display_blocks += pobj->n_display;
            if (textured) {
                stats.textured_pobjs += 1;
            }
            /* HSD_PObjDisp returns without drawing when both faces cull. */
            const bool culled = (pobj->flags &
                                 (POBJ_CULLFRONT | POBJ_CULLBACK)) ==
                                (POBJ_CULLFRONT | POBJ_CULLBACK);
            if (culled) {
                stats.culled_pobjs += 1;
            }
            if (!drawn || culled) {
                stats.undrawn_pobjs += 1;
            }
        }
    }

    void walk_dobjs(HSD_DObj* dobj, bool drawn)
    {
        for (; dobj != nullptr && visited++ < kWalkLimit; dobj = dobj->next) {
            stats.dobjs += 1;
            const bool hidden = (dobj->flags & DOBJ_HIDDEN) != 0;
            if (hidden) {
                stats.hidden_dobjs += 1;
            }
            bool textured = false;
            if (dobj->mobj != nullptr) {
                stats.mobjs += 1;
                for (HSD_TObj* tobj = dobj->mobj->tobj;
                     tobj != nullptr && visited++ < kWalkLimit;
                     tobj = tobj->next) {
                    stats.tobjs += 1;
                    textured = textured || tobj->imagedesc != nullptr;
                }
            }
            walk_pobjs(dobj->pobj, textured, drawn && !hidden);
        }
    }

    void walk_jobjs(HSD_JObj* jobj, mh_u32 depth)
    {
        for (; jobj != nullptr && visited++ < kWalkLimit; jobj = jobj->next) {
            stats.jobjs += 1;
            if (depth > stats.tree_depth) {
                stats.tree_depth = depth;
            }
            const bool hidden = (jobj->flags & JOBJ_HIDDEN) != 0;
            if (hidden) {
                stats.hidden_jobjs += 1;
            }
            /* HSD_JObjDispAll only reaches a node for a pass its own
             * transparency bits name. */
            const bool any_pass =
                (jobj->flags & (JOBJ_OPA | JOBJ_XLU | JOBJ_TEXEDGE)) != 0;
            if (union_type_dobj(jobj)) {
                walk_dobjs(jobj->u.dobj, !hidden && any_pass);
            }
            /* An instance joint borrows a subtree it does not own, so
             * descending into it would count the same objects twice. */
            if ((jobj->flags & JOBJ_INSTANCE) == 0) {
                walk_jobjs(jobj->child, depth + 1);
            }
        }
    }
};

/* Collects the tree in the order the loader built it, so a joint index is
 * stable between the loaded graph and the read-only schema. */
void collect_jobjs(HSD_JObj* jobj, std::vector<HSD_JObj*>* out)
{
    for (; jobj != nullptr && out->size() < kWalkLimit; jobj = jobj->next) {
        out->push_back(jobj);
        if ((jobj->flags & JOBJ_INSTANCE) == 0) {
            collect_jobjs(jobj->child, out);
        }
    }
}

MeleeHostStatus load(const char* path, const char* symbol, bool scene_model,
                     mh_u32 model_index, MeleeHostSceneModel* out_model)
{
    if (path == nullptr || symbol == nullptr || out_model == nullptr) {
        return fail(MELEE_HOST_INVALID_ARGUMENT, "null argument");
    }
    const MeleeHostStatus ready = melee_host_baselib_bootstrap();
    if (ready != MELEE_HOST_OK) {
        return fail(ready, "the baselib bootstrap failed");
    }

    std::size_t index = 0;
    LoadedModel* const slot = free_slot(&index);
    if (slot == nullptr) {
        return fail(MELEE_HOST_NOT_READY, "no free scene model slot");
    }

    try {
        slot->bytes = read_file(path);
        slot->archive = std::make_unique<HsdRuntimeArchive>(slot->bytes);
        slot->descriptors =
            std::make_unique<HsdMaterializedArchive>(*slot->archive);
        slot->root_joint =
            scene_model
                ? slot->descriptors->scene_model_joint(symbol, model_index)
                : slot->descriptors->joint(symbol);
        if (scene_model) {
            slot->camera = slot->descriptors->scene_camera(symbol, 0);
        }
    } catch (const std::exception& error) {
        slot->descriptors.reset();
        slot->archive.reset();
        slot->bytes.clear();
        return fail(MELEE_HOST_IO_ERROR, error.what());
    }

    if (slot->root_joint == nullptr) {
        slot->descriptors.reset();
        slot->archive.reset();
        slot->bytes.clear();
        return fail(MELEE_HOST_IO_ERROR, "the archive holds no joint there");
    }

    slot->root = HSD_JObjLoadJoint(slot->root_joint);
    if (slot->root == nullptr) {
        slot->descriptors.reset();
        slot->archive.reset();
        slot->bytes.clear();
        return fail(MELEE_HOST_INTERNAL_ERROR,
                    "the original loader returned no object");
    }

    slot->in_use = true;
    *out_model = handle_of(index);
    last_error = "no error";
    return MELEE_HOST_OK;
}

} // namespace

extern "C" MeleeHostStatus melee_host_scene_graphics_load_model(
    const char* path, const char* symbol, mh_u32 model_index,
    MeleeHostSceneModel* out_model)
{
    return load(path, symbol, true, model_index, out_model);
}

extern "C" MeleeHostStatus melee_host_scene_graphics_load_joint(
    const char* path, const char* symbol, MeleeHostSceneModel* out_model)
{
    return load(path, symbol, false, 0, out_model);
}

extern "C" MeleeHostStatus melee_host_scene_graphics_model_count(
    const char* path, const char* symbol, mh_u32* out_count)
{
    if (path == nullptr || symbol == nullptr || out_count == nullptr) {
        return fail(MELEE_HOST_INVALID_ARGUMENT, "null argument");
    }
    try {
        const std::vector<std::byte> bytes = read_file(path);
        const HsdRuntimeArchive archive(bytes);
        HsdMaterializedArchive descriptors(archive);
        *out_count =
            static_cast<mh_u32>(descriptors.scene_model_count(symbol));
    } catch (const std::exception& error) {
        return fail(MELEE_HOST_IO_ERROR, error.what());
    }
    last_error = "no error";
    return MELEE_HOST_OK;
}

extern "C" MeleeHostStatus melee_host_scene_graphics_stats(
    MeleeHostSceneModel model, MeleeHostSceneModelStats* out_stats)
{
    LoadedModel* const slot = model_of(model);
    if (slot == nullptr || out_stats == nullptr) {
        return fail(MELEE_HOST_INVALID_ARGUMENT, "unknown scene model");
    }
    TreeWalk walk;
    walk.walk_jobjs(slot->root, 1);
    const auto& translated = slot->descriptors->stats();
    walk.stats.descriptor_bytes =
        static_cast<mh_u32>(translated.descriptor_bytes);
    walk.stats.payload_bytes = static_cast<mh_u32>(translated.payload_bytes);
    *out_stats = walk.stats;
    last_error = "no error";
    return MELEE_HOST_OK;
}

extern "C" MeleeHostStatus melee_host_scene_graphics_joint_world_position(
    MeleeHostSceneModel model, mh_u32 joint_index, float* out_xyz)
{
    LoadedModel* const slot = model_of(model);
    if (slot == nullptr || out_xyz == nullptr) {
        return fail(MELEE_HOST_INVALID_ARGUMENT, "unknown scene model");
    }
    std::vector<HSD_JObj*> joints;
    collect_jobjs(slot->root, &joints);
    if (joint_index >= joints.size()) {
        return fail(MELEE_HOST_INVALID_ARGUMENT, "joint index out of range");
    }
    /* The matrix pass walks up to the root, so asking any joint for its
     * matrix builds every parent's first. */
    HSD_JObjSetupMatrix(joints[joint_index]);
    const MtxPtr world = joints[joint_index]->mtx;
    out_xyz[0] = world[0][3];
    out_xyz[1] = world[1][3];
    out_xyz[2] = world[2][3];
    last_error = "no error";
    return MELEE_HOST_OK;
}

extern "C" MeleeHostStatus melee_host_scene_graphics_attach_animation(
    MeleeHostSceneModel model, const char* path, const char* anim_symbol,
    const char* mat_anim_symbol, const char* shape_anim_symbol)
{
    LoadedModel* const slot = model_of(model);
    if (slot == nullptr || path == nullptr) {
        return fail(MELEE_HOST_INVALID_ARGUMENT, "unknown scene model");
    }
    if (anim_symbol == nullptr && mat_anim_symbol == nullptr &&
        shape_anim_symbol == nullptr)
    {
        return fail(MELEE_HOST_INVALID_ARGUMENT, "no animation symbol given");
    }

    HSD_AnimJoint* anim = nullptr;
    HSD_MatAnimJoint* mat_anim = nullptr;
    HSD_ShapeAnimJoint* shape_anim = nullptr;
    try {
        slot->animation_bytes = read_file(path);
        slot->animation_archive =
            std::make_unique<HsdRuntimeArchive>(slot->animation_bytes);
        slot->animation_descriptors =
            std::make_unique<HsdMaterializedArchive>(*slot->animation_archive);
        if (anim_symbol != nullptr) {
            anim = slot->animation_descriptors->anim_joint(anim_symbol);
        }
        if (mat_anim_symbol != nullptr) {
            mat_anim =
                slot->animation_descriptors->mat_anim_joint(mat_anim_symbol);
        }
        if (shape_anim_symbol != nullptr) {
            shape_anim = slot->animation_descriptors->shape_anim_joint(
                shape_anim_symbol);
        }
    } catch (const std::exception& error) {
        slot->animation_descriptors.reset();
        slot->animation_archive.reset();
        slot->animation_bytes.clear();
        return fail(MELEE_HOST_IO_ERROR, error.what());
    }

    /* The original walks the three trees alongside the object tree, so a
     * mismatch in shape is the archive's business, not the host's. */
    HSD_JObjAddAnimAll(slot->root, anim, mat_anim, shape_anim);
    slot->animation_frame = 0.0F;
    last_error = "no error";
    return MELEE_HOST_OK;
}

extern "C" MeleeHostStatus melee_host_scene_graphics_list_animations(
    const char* path, mh_u32 index, char* out_symbol, size_t capacity,
    mh_u32* out_count)
{
    if (path == nullptr) {
        return fail(MELEE_HOST_INVALID_ARGUMENT, "null argument");
    }
    try {
        const std::vector<std::byte> bytes = read_file(path);
        const std::vector<melee::assets::HsdArchiveMember> members =
            melee::assets::enumerate_hsd_archives(bytes);
        if (out_count != nullptr) {
            *out_count = static_cast<mh_u32>(members.size());
        }
        if (out_symbol != nullptr) {
            if (index >= members.size() || capacity == 0) {
                return fail(MELEE_HOST_INVALID_ARGUMENT,
                            "animation index out of range");
            }
            const std::string_view symbol = members[index].symbol;
            const std::size_t length =
                symbol.size() < capacity - 1 ? symbol.size() : capacity - 1;
            std::memcpy(out_symbol, symbol.data(), length);
            out_symbol[length] = '\0';
        }
    } catch (const std::exception& error) {
        return fail(MELEE_HOST_IO_ERROR, error.what());
    }
    last_error = "no error";
    return MELEE_HOST_OK;
}

extern "C" MeleeHostStatus melee_host_scene_graphics_attach_named_animation(
    MeleeHostSceneModel model, const char* path, const char* symbol)
{
    LoadedModel* const slot = model_of(model);
    if (slot == nullptr || path == nullptr || symbol == nullptr) {
        return fail(MELEE_HOST_INVALID_ARGUMENT, "unknown scene model");
    }

    FigaTree* figa = nullptr;
    HSD_AnimJoint* anim = nullptr;
    try {
        slot->animation_bytes = read_file(path);
        const std::vector<melee::assets::HsdArchiveMember> members =
            melee::assets::enumerate_hsd_archives(slot->animation_bytes);
        /* A file with one archive per action names each by its own single
         * public symbol.  A file that is one archive names many symbols
         * inside it, so a miss on the first has to look further in. */
        auto found = std::find_if(
            members.begin(), members.end(),
            [symbol](const melee::assets::HsdArchiveMember& member) {
                return member.symbol == symbol;
            });
        if (found == members.end()) {
            found = std::find_if(
                members.begin(), members.end(),
                [&](const melee::assets::HsdArchiveMember& member) {
                    const std::span<const std::byte> bytes{
                        slot->animation_bytes.data() + member.offset,
                        member.size
                    };
                    const melee::assets::HsdArchiveView view(bytes);
                    for (const auto& entry : view.public_symbols()) {
                        if (entry.name == symbol) {
                            return true;
                        }
                    }
                    return false;
                });
        }
        if (found == members.end()) {
            slot->animation_bytes.clear();
            return fail(MELEE_HOST_IO_ERROR,
                        std::string("no animation named ") + symbol);
        }
        /* Only the member's own bytes: each carries a complete archive whose
         * header states its length. */
        const std::span<const std::byte> member{
            slot->animation_bytes.data() + found->offset, found->size
        };
        slot->animation_archive =
            std::make_unique<HsdRuntimeArchive>(member);
        slot->animation_descriptors =
            std::make_unique<HsdMaterializedArchive>(*slot->animation_archive);
        /* Two formats answer to the same kind of symbol: a character's action
         * is a FigaTree, the game's own flat format, while a menu or title
         * model uses an HSD animation tree.  Both are validated field by
         * field, so trying one and falling back is safe: a wrong guess throws
         * rather than producing a plausible mess. */
        try {
            figa = slot->animation_descriptors->figa_tree(symbol);
        } catch (const melee::assets::HsdArchiveError&) {
            anim = slot->animation_descriptors->anim_joint(symbol);
        }
    } catch (const std::exception& error) {
        slot->animation_descriptors.reset();
        slot->animation_archive.reset();
        slot->animation_bytes.clear();
        return fail(MELEE_HOST_IO_ERROR, error.what());
    }

    if (figa == nullptr) {
        HSD_JObjAddAnimAll(slot->root, anim, nullptr, nullptr);
        slot->animation_frame = 0.0F;
        last_error = "no error";
        return MELEE_HOST_OK;
    }

    /* The node list pairs one entry with each bone, in the order the tree was
     * built, and says how many tracks that bone takes.  This is the walk
     * ftanim.c performs over a fighter's parts, done here over the plain
     * object tree. */
    std::vector<HSD_JObj*> joints;
    collect_jobjs(slot->root, &joints);
    const s8* node = figa->nodes;
    FigaTrack* track = figa->tracks;
    std::size_t bone = 0;
    while (*node >= 0 && bone < joints.size()) {
        lbAnim_8001E6D8(joints[bone], figa, track, *node);
        track += *node;
        ++node;
        ++bone;
    }
    slot->animation_bones = static_cast<mh_u32>(bone);
    slot->animation_frame = 0.0F;
    last_error = "no error";
    return MELEE_HOST_OK;
}

extern "C" MeleeHostStatus melee_host_scene_graphics_run_animation(
    MeleeHostSceneModel model, float start_frame, float rate, mh_u32 frames)
{
    LoadedModel* const slot = model_of(model);
    if (slot == nullptr) {
        return fail(MELEE_HOST_INVALID_ARGUMENT, "unknown scene model");
    }
    if (slot->animation_descriptors == nullptr) {
        return fail(MELEE_HOST_NOT_READY, "no animation is attached");
    }
    /* Requesting sets the starting frame; interpreting is what advances it,
     * once per frame, exactly as the game's loop does.  A loaded AObj already
     * plays at one frame per call, so setting the rate here is about choosing
     * a different speed or direction, which is what the game does per fighter
     * action.  The traversal is the original one rather than a hand-rolled
     * walk, so it reaches the same objects HSD_JObjAnimAll will. */
    HSD_JObjReqAnimAll(slot->root, start_frame);
    HSD_ForeachAnim(slot->root, JOBJ_TYPE, ALL_TYPE_MASK,
                    reinterpret_cast<void*>(HSD_AObjSetRate), AOBJ_ARG_AF,
                    static_cast<f64>(rate));
    for (mh_u32 frame = 0; frame < frames; ++frame) {
        HSD_JObjAnimAll(slot->root);
    }
    slot->animation_frame =
        start_frame + rate * static_cast<float>(frames);
    last_error = "no error";
    return MELEE_HOST_OK;
}

extern "C" MeleeHostStatus melee_host_scene_graphics_step_animation(
    MeleeHostSceneModel model, mh_u32 frames)
{
    LoadedModel* const slot = model_of(model);
    if (slot == nullptr) {
        return fail(MELEE_HOST_INVALID_ARGUMENT, "unknown scene model");
    }
    if (slot->animation_descriptors == nullptr) {
        return fail(MELEE_HOST_NOT_READY, "no animation is attached");
    }
    for (mh_u32 frame = 0; frame < frames; ++frame) {
        HSD_JObjAnimAll(slot->root);
    }
    slot->animation_frame += static_cast<float>(frames);
    last_error = "no error";
    return MELEE_HOST_OK;
}

extern "C" MeleeHostStatus melee_host_scene_graphics_animation_stats(
    MeleeHostSceneModel model, MeleeHostSceneAnimationStats* out_stats)
{
    LoadedModel* const slot = model_of(model);
    if (slot == nullptr || out_stats == nullptr) {
        return fail(MELEE_HOST_INVALID_ARGUMENT, "unknown scene model");
    }
    MeleeHostSceneAnimationStats stats{};
    if (slot->animation_descriptors != nullptr) {
        const auto& translated = slot->animation_descriptors->stats();
        stats.anim_joints = static_cast<mh_u32>(translated.anim_joints);
        stats.mat_anim_joints =
            static_cast<mh_u32>(translated.mat_anim_joints);
        stats.shape_anim_joints =
            static_cast<mh_u32>(translated.shape_anim_joints);
        stats.aobj_descs = static_cast<mh_u32>(translated.aobj_descs);
        stats.fobj_descs = static_cast<mh_u32>(translated.fobj_descs);
        stats.anim_data_bytes =
            static_cast<mh_u32>(translated.anim_data_bytes);
    }
    /* Read from the tree rather than from the host's own counter: this is
     * what the original interpreter actually advanced. */
    std::vector<HSD_JObj*> joints;
    collect_jobjs(slot->root, &joints);
    for (HSD_JObj* const jobj : joints) {
        if (jobj->aobj == nullptr) {
            continue;
        }
        if (stats.aobjs_in_tree == 0) {
            stats.aobj_frame = jobj->aobj->curr_frame;
            stats.aobj_end_frame = jobj->aobj->end_frame;
        }
        stats.aobjs_in_tree += 1;
    }
    stats.aobjs_live = HSD_ObjAllocGetUsing(HSD_AObjGetAllocData());
    stats.fobjs_live = HSD_ObjAllocGetUsing(HSD_FObjGetAllocData());
    stats.current_frame = slot->animation_frame;
    *out_stats = stats;
    last_error = "no error";
    return MELEE_HOST_OK;
}

extern "C" MeleeHostStatus melee_host_scene_graphics_render(
    MeleeHostSceneModel model, MeleeHostSceneView view,
    MeleeHostSceneRenderStats* out_stats)
{
    LoadedModel* const slot = model_of(model);
    if (slot == nullptr || out_stats == nullptr) {
        return fail(MELEE_HOST_INVALID_ARGUMENT, "unknown scene model");
    }

    /* The original camera setup scales the viewport by the render mode's
     * framebuffer-to-VI ratio, so a render mode has to be installed first.
     * This is the one gmMain picks. */
    HSD_VISetConfigure(&GXNtsc480IntDf);

    /* The scene's own camera when it has one, which is what the game draws
     * through.  Otherwise a stand-in framing the origin: the recorder captures
     * the vertices the display lists produce, before any clip, so the choice
     * changes the space they land in and not whether they are captured. */
    bool used_scene_camera = false;
    HSD_CObj* camera = nullptr;
    if (slot->camera != nullptr) {
        camera = HSD_CObjLoadDesc(slot->camera);
        used_scene_camera = camera != nullptr;
    }
    if (camera == nullptr) {
        camera = HSD_CObjAlloc();
        if (camera == nullptr) {
            return fail(MELEE_HOST_NOT_READY, "no camera could be allocated");
        }
        Vec3 eye{ 0.0F, 0.0F, 500.0F };
        Vec3 interest{ 0.0F, 0.0F, 0.0F };
        Vec3 up{ 0.0F, 1.0F, 0.0F };
        HSD_CObjSetEyePosition(camera, &eye);
        HSD_CObjSetInterest(camera, &interest);
        HSD_CObjSetUpVector(camera, &up);
        HSD_CObjSetNear(camera, 1.0F);
        HSD_CObjSetFar(camera, 10000.0F);
        HSD_CObjSetPerspective(camera, 60.0F, 640.0F / 480.0F);
        HSD_CObjSetViewportfx4(camera, 0.0F, 640.0F, 0.0F, 480.0F);
        HSD_CObjSetScissorx4(camera, 0, 640, 0, 480);
    }

    /* Both state layers start from a known point, so a count reflects this
     * pass and not whatever ran before it. */
    melee_host_gx_state_reset();
    HSD_StateInvalidate(HSD_STATE_ALL);
    melee_host_gx_reset_command_log();

    if (!HSD_CObjSetCurrent(camera)) {
        hsdDelete(camera);
        return fail(MELEE_HOST_INTERNAL_ERROR,
                    "the original camera setup refused the host viewport");
    }

    MeleeHostSceneRenderStats stats{};
    stats.used_scene_camera = used_scene_camera;
    /* A world-space capture asks the display path for an identity view, which
     * leaves the matrices it loads into GX as the joints' world transforms.
     * Passing NULL instead is what the game does: the path then takes the view
     * from the current camera. */
    Mtx world_view;
    PSMTXIdentity(world_view);
    MtxPtr const view_matrix =
        view == MELEE_HOST_SCENE_VIEW_WORLD ? world_view : nullptr;

    /* The order the original render callback uses, through the table it reads
     * its masks from. */
    const HSD_TrspMask passes[3] = { HSD_TRSP_OPA, HSD_TRSP_TEXEDGE,
                                     HSD_TRSP_XLU };
    std::size_t drawn = 0;
    for (std::size_t pass = 0; pass < 3; ++pass) {
        HSD_JObjDispAll(slot->root, view_matrix, passes[pass], 0);
        const std::size_t total = melee_host_gx_triangle_count();
        stats.pass_triangles[pass] = static_cast<mh_u32>(total - drawn);
        drawn = total;
    }
    HSD_CObjEndCurrent();
    HSD_CObjSetCurrent(nullptr);

    stats.triangles = static_cast<mh_u32>(melee_host_gx_triangle_count());
    stats.vertices =
        static_cast<mh_u32>(melee_host_gx_captured_vertex_count());
    stats.display_list_errors =
        static_cast<mh_u32>(melee_host_gx_display_list_error_count());
    stats.rejected_indices =
        static_cast<mh_u32>(melee_host_gx_rejected_index_count());
    stats.textures =
        static_cast<mh_u32>(melee_host_gx_captured_texture_count());
    stats.draw_states =
        static_cast<mh_u32>(melee_host_gx_captured_draw_state_count());
    stats.tev_states =
        static_cast<mh_u32>(melee_host_gx_captured_tev_state_count());
    for (std::size_t index = 0; index < stats.triangles; ++index) {
        MeleeHostGxCapturedTriangle triangle{};
        /* The attribute bit is the test, not the id: an untextured vertex
         * leaves the id at zero, which is a valid texture index. */
        if (!melee_host_gx_captured_triangle_at(index, &triangle)) {
            continue;
        }
        if ((triangle.vertices[0].attributes &
             MELEE_HOST_GX_VERTEX_TEXTURE_IMAGE) != 0)
        {
            stats.textured_triangles += 1;
        }
        MeleeHostGxTevState tev{};
        MeleeHostGxResolvedShading shading{};
        if (melee_host_gx_captured_tev_state_at(
                triangle.vertices[0].tev_state, &tev) &&
            melee_host_gx_resolve_shading(&tev, &shading) &&
            shading.kind != MELEE_HOST_GX_SHADING_APPROXIMATED)
        {
            stats.shading_exact_triangles += 1;
        } else {
            stats.shading_approximated_triangles += 1;
        }
    }

    hsdDelete(camera);
    *out_stats = stats;
    last_error = "no error";
    return MELEE_HOST_OK;
}

extern "C" MeleeHostStatus
melee_host_scene_graphics_release(MeleeHostSceneModel model)
{
    LoadedModel* const slot = model_of(model);
    if (slot == nullptr) {
        return fail(MELEE_HOST_INVALID_ARGUMENT, "unknown scene model");
    }
    /* The original destructor chain frees the objects; the descriptors they
     * point into go afterwards. */
    HSD_JObjRemoveAll(slot->root);
    slot->root = nullptr;
    slot->root_joint = nullptr;
    slot->camera = nullptr;
    slot->descriptors.reset();
    slot->archive.reset();
    slot->bytes.clear();
    slot->bytes.shrink_to_fit();
    slot->animation_descriptors.reset();
    slot->animation_archive.reset();
    slot->animation_bytes.clear();
    slot->animation_bytes.shrink_to_fit();
    slot->animation_frame = 0.0F;
    slot->animation_bones = 0;
    slot->in_use = false;
    slot->generation = static_cast<mh_u16>(slot->generation + 1);
    last_error = "no error";
    return MELEE_HOST_OK;
}

extern "C" const char* melee_host_scene_graphics_last_error(void)
{
    return last_error.c_str();
}
