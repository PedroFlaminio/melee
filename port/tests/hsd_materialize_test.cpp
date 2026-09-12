#include "test.hpp"

#include "assets/hsd_materialize.hpp"
#include "assets/hsd_runtime_archive.hpp"

#include <melee_host/baselib.h>
#include <melee_host/gx.h>
#include <melee_host/scene_graphics.h>

MELEE_HOST_HSD_BEGIN
#include <dolphin/gx/GXFrameBuffer.h>
#include <dolphin/gx/GXGeometry.h>
#include <dolphin/gx/GXVert.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/displayfunc.h>
#include <sysdolphin/baselib/state.h>
#include <sysdolphin/baselib/video.h>
MELEE_HOST_HSD_END

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace {

void append_be32(std::vector<std::byte>& output, std::uint32_t value)
{
    output.push_back(static_cast<std::byte>(value >> 24U));
    output.push_back(static_cast<std::byte>(value >> 16U));
    output.push_back(static_cast<std::byte>(value >> 8U));
    output.push_back(static_cast<std::byte>(value));
}

std::uint32_t bits_of(float value)
{
    std::uint32_t result = 0;
    static_assert(sizeof(result) == sizeof(value));
    __builtin_memcpy(&result, &value, sizeof(result));
    return result;
}

bool near(f32 value, f32 expected, f32 tolerance = 1.0e-5F)
{
    return std::fabs(value - expected) <= tolerance;
}

/* Offsets inside the synthetic archive's data section.  Each record sits on
 * its own boundary so a field written by mistake cannot land in another one,
 * and the display list keeps the 32-byte alignment GX requires. */
enum : std::uint32_t {
    kScene = 0x000,
    kModels = 0x020,
    kModelDesc = 0x030,
    kRootJoint = 0x040,
    kChildJoint = 0x080,
    kDObjDesc = 0x0C0,
    kMObjDesc = 0x0D0,
    kMaterial = 0x0F0,
    kPObjDesc = 0x110,
    kVtxDescList = 0x130,
    kTObjDesc = 0x160,
    kImageDesc = 0x1C0,
    kTlutDesc = 0x1E0,
    kJointMtx = 0x200,
    kDisplayList = 0x240,
    kVertexArray = 0x280,
    kTlutData = 0x2C0,
    kClassName = 0x2D0,
    kDataSize = 0x300,
};

struct SyntheticArchive {
    std::vector<std::byte> bytes;
    std::size_t data_begin = 0;

    void field32(std::uint32_t offset, std::uint32_t value)
    {
        const std::size_t at = data_begin + offset;
        bytes[at] = static_cast<std::byte>(value >> 24U);
        bytes[at + 1] = static_cast<std::byte>(value >> 16U);
        bytes[at + 2] = static_cast<std::byte>(value >> 8U);
        bytes[at + 3] = static_cast<std::byte>(value);
    }

    void field16(std::uint32_t offset, std::uint16_t value)
    {
        const std::size_t at = data_begin + offset;
        bytes[at] = static_cast<std::byte>(value >> 8U);
        bytes[at + 1] = static_cast<std::byte>(value);
    }

    void field8(std::uint32_t offset, std::uint8_t value)
    {
        bytes[data_begin + offset] = static_cast<std::byte>(value);
    }

    void fieldf32(std::uint32_t offset, float value)
    {
        field32(offset, bits_of(value));
    }
};

/* Every pointer field the archive relocates.  A pointer field missing from
 * this list must read as NULL, which is what the materializer enforces. */
constexpr std::array<std::uint32_t, 18> kRelocations{
    kScene + 0x00,        // SceneDesc.models
    kModels + 0x00,       // models[0]
    kModelDesc + 0x00,    // DynamicModelDesc.joint
    kRootJoint + 0x08,    // HSD_Joint.child
    kChildJoint + 0x00,   // HSD_Joint.class_name
    kChildJoint + 0x10,   // HSD_Joint.u.dobjdesc
    kChildJoint + 0x38,   // HSD_Joint.mtx
    kDObjDesc + 0x08,     // HSD_DObjDesc.mobjdesc
    kDObjDesc + 0x0C,     // HSD_DObjDesc.pobjdesc
    kMObjDesc + 0x08,     // HSD_MObjDesc.texdesc
    kMObjDesc + 0x0C,     // HSD_MObjDesc.mat
    kPObjDesc + 0x08,     // HSD_PObjDesc.verts
    kPObjDesc + 0x10,     // HSD_PObjDesc.display
    kVtxDescList + 0x14,  // HSD_VtxDescList[0].vertex
    kTObjDesc + 0x4C,     // HSD_TObjDesc.imagedesc
    kTObjDesc + 0x50,     // HSD_TObjDesc.tlutdesc
    kImageDesc + 0x00,    // HSD_ImageDesc.image_ptr
    kTlutDesc + 0x00,     // HSD_TlutDesc.lut
};

SyntheticArchive make_scene()
{
    constexpr std::string_view symbol = "test_scene_data";
    constexpr std::uint32_t file_size =
        32 + kDataSize + static_cast<std::uint32_t>(kRelocations.size()) * 4 +
        8 + static_cast<std::uint32_t>(symbol.size()) + 1;

    SyntheticArchive archive;
    for (const std::uint32_t value : std::array<std::uint32_t, 8>{
             file_size, kDataSize,
             static_cast<std::uint32_t>(kRelocations.size()), 1, 0,
             0x01000000, 0, 0 }) {
        append_be32(archive.bytes, value);
    }
    archive.data_begin = archive.bytes.size();
    archive.bytes.resize(archive.data_begin + kDataSize);

    archive.field32(kScene + 0x00, kModels);
    archive.field32(kModels + 0x00, kModelDesc);
    archive.field32(kModels + 0x04, 0); // model table terminator
    archive.field32(kModelDesc + 0x00, kRootJoint);

    // The root carries no geometry, only a child and its own transform.  Its
    // JOBJ_ROOT_OPA bit is what makes HSD_JObjDispAll descend for the opaque
    // pass; the child's JOBJ_OPA is what makes it draw.
    archive.field32(kRootJoint + 0x04, 1U << 28U);
    archive.field32(kRootJoint + 0x08, kChildJoint);
    archive.fieldf32(kRootJoint + 0x20, 1.0F);
    archive.fieldf32(kRootJoint + 0x24, 1.0F);
    archive.fieldf32(kRootJoint + 0x28, 1.0F);
    archive.fieldf32(kRootJoint + 0x2C, 1.0F);
    archive.fieldf32(kRootJoint + 0x30, 2.0F);
    archive.fieldf32(kRootJoint + 0x34, 3.0F);

    archive.field32(kChildJoint + 0x00, kClassName);
    archive.field32(kChildJoint + 0x04, 1U << 18U);
    archive.field32(kChildJoint + 0x10, kDObjDesc);
    archive.fieldf32(kChildJoint + 0x20, 2.0F);
    archive.fieldf32(kChildJoint + 0x24, 2.0F);
    archive.fieldf32(kChildJoint + 0x28, 2.0F);
    archive.fieldf32(kChildJoint + 0x2C, 4.0F);
    archive.fieldf32(kChildJoint + 0x30, 5.0F);
    archive.fieldf32(kChildJoint + 0x34, 6.0F);
    archive.field32(kChildJoint + 0x38, kJointMtx);

    archive.field32(kDObjDesc + 0x08, kMObjDesc);
    archive.field32(kDObjDesc + 0x0C, kPObjDesc);

    archive.field32(kMObjDesc + 0x04, 0x10); // rendermode
    archive.field32(kMObjDesc + 0x08, kTObjDesc);
    archive.field32(kMObjDesc + 0x0C, kMaterial);

    archive.field8(kMaterial + 0x04, 64);  // diffuse r
    archive.field8(kMaterial + 0x05, 128); // diffuse g
    archive.field8(kMaterial + 0x06, 192); // diffuse b
    archive.field8(kMaterial + 0x07, 255); // diffuse a
    archive.fieldf32(kMaterial + 0x0C, 0.5F);  // alpha
    archive.fieldf32(kMaterial + 0x10, 2.0F);  // shininess

    archive.field32(kPObjDesc + 0x08, kVtxDescList);
    archive.field16(kPObjDesc + 0x0C, 0); // POBJ_SKIN
    archive.field16(kPObjDesc + 0x0E, 1); // one display list block
    archive.field32(kPObjDesc + 0x10, kDisplayList);

    archive.field32(kVtxDescList + 0x00, static_cast<std::uint32_t>(GX_VA_POS));
    archive.field32(kVtxDescList + 0x04, static_cast<std::uint32_t>(GX_INDEX8));
    archive.field32(kVtxDescList + 0x08, static_cast<std::uint32_t>(GX_POS_XYZ));
    archive.field32(kVtxDescList + 0x0C, static_cast<std::uint32_t>(GX_S16));
    archive.field8(kVtxDescList + 0x10, 2);  // fractional bits
    archive.field16(kVtxDescList + 0x12, 6); // stride
    archive.field32(kVtxDescList + 0x14, kVertexArray);
    archive.field32(kVtxDescList + 0x18, static_cast<std::uint32_t>(GX_VA_NULL));

    archive.field32(kTObjDesc + 0x08, static_cast<std::uint32_t>(GX_TEXMAP0));
    archive.field32(kTObjDesc + 0x0C, static_cast<std::uint32_t>(GX_TG_TEX0));
    archive.fieldf32(kTObjDesc + 0x1C, 1.0F); // scale x
    archive.fieldf32(kTObjDesc + 0x20, 1.0F);
    archive.fieldf32(kTObjDesc + 0x24, 1.0F);
    archive.field32(kTObjDesc + 0x34, static_cast<std::uint32_t>(GX_REPEAT));
    archive.field32(kTObjDesc + 0x38, static_cast<std::uint32_t>(GX_CLAMP));
    archive.field8(kTObjDesc + 0x3C, 1); // repeat_s
    archive.field8(kTObjDesc + 0x3D, 1); // repeat_t
    archive.fieldf32(kTObjDesc + 0x44, 1.0F); // blending
    archive.field32(kTObjDesc + 0x48, static_cast<std::uint32_t>(GX_LINEAR));
    archive.field32(kTObjDesc + 0x4C, kImageDesc);
    archive.field32(kTObjDesc + 0x50, kTlutDesc);

    archive.field32(kImageDesc + 0x00, kVertexArray); // reuse as image bytes
    archive.field16(kImageDesc + 0x04, 8);            // width
    archive.field16(kImageDesc + 0x06, 8);            // height
    archive.field32(kImageDesc + 0x08, static_cast<std::uint32_t>(GX_TF_C8));
    archive.fieldf32(kImageDesc + 0x14, 3.0F); // maxLOD

    archive.field32(kTlutDesc + 0x00, kTlutData);
    archive.field32(kTlutDesc + 0x04, static_cast<std::uint32_t>(GX_TL_RGB565));
    archive.field32(kTlutDesc + 0x08, 7); // tlut_name
    archive.field16(kTlutDesc + 0x0C, 2); // n_entries

    for (std::uint32_t index = 0; index < 12; ++index) {
        archive.fieldf32(kJointMtx + index * 4,
                         static_cast<float>(index) + 1.0F);
    }
    // One indexed triangle in GX_VTXFMT0, then NOP padding to the block.
    archive.field8(kDisplayList + 0, 0x90); // GX_TRIANGLES | GX_VTXFMT0
    archive.field16(kDisplayList + 1, 3);   // vertex count
    archive.field8(kDisplayList + 3, 0);
    archive.field8(kDisplayList + 4, 1);
    archive.field8(kDisplayList + 5, 2);

    // Three S16 positions with two fractional bits, so 400 reads back as 100.
    archive.field16(kVertexArray + 0x00, 0);
    archive.field16(kVertexArray + 0x02, 0);
    archive.field16(kVertexArray + 0x04, 0);
    archive.field16(kVertexArray + 0x06, 400);
    archive.field16(kVertexArray + 0x08, 0);
    archive.field16(kVertexArray + 0x0A, 0);
    archive.field16(kVertexArray + 0x0C, 0);
    archive.field16(kVertexArray + 0x0E, 400);
    archive.field16(kVertexArray + 0x10, 0);
    constexpr std::string_view class_name = "custom";
    for (std::uint32_t index = 0; index < class_name.size(); ++index) {
        archive.field8(kClassName + index,
                       static_cast<std::uint8_t>(class_name[index]));
    }
    archive.field8(kClassName + static_cast<std::uint32_t>(class_name.size()),
                   0);

    for (const std::uint32_t relocation : kRelocations) {
        append_be32(archive.bytes, relocation);
    }
    append_be32(archive.bytes, kScene); // public offset
    append_be32(archive.bytes, 0);      // public symbol offset
    for (const char letter : symbol) {
        archive.bytes.push_back(static_cast<std::byte>(letter));
    }
    archive.bytes.push_back(std::byte{ 0 });
    return archive;
}

u32 jobjs_live()
{
    return hsdJObj.parent.parent.head.nb_exist;
}

} // namespace

TEST_CASE("the materializer rebuilds HSD descriptors in host layout")
{
    const SyntheticArchive source = make_scene();
    const melee::assets::HsdRuntimeArchive runtime(source.bytes);
    melee::assets::HsdMaterializedArchive descriptors(runtime);

    REQUIRE(descriptors.scene_model_count("test_scene_data") == 1);
    HSD_Joint* const root =
        descriptors.scene_model_joint("test_scene_data", 0);
    REQUIRE(root != nullptr);

    // The tree keeps its shape, and the big-endian transforms decoded.
    REQUIRE(root->next == nullptr);
    REQUIRE(root->flags == (1U << 28U));
    REQUIRE(near(root->position.x, 1.0F));
    REQUIRE(near(root->position.y, 2.0F));
    REQUIRE(near(root->position.z, 3.0F));
    REQUIRE(near(root->scale.x, 1.0F));

    HSD_Joint* const child = root->child;
    REQUIRE(child != nullptr);
    REQUIRE(child->child == nullptr);
    REQUIRE(near(child->position.x, 4.0F));
    REQUIRE(near(child->position.z, 6.0F));
    REQUIRE(near(child->scale.y, 2.0F));
    REQUIRE(std::string_view(child->class_name) == "custom");

    // A joint matrix is copied into the tree by memcpy, so unlike a GX payload
    // it has to hold host-order floats.
    REQUIRE(child->mtx != nullptr);
    REQUIRE(near(child->mtx[0][0], 1.0F));
    REQUIRE(near(child->mtx[1][0], 5.0F));
    REQUIRE(near(child->mtx[2][3], 12.0F));

    HSD_DObjDesc* const dobj = child->u.dobjdesc;
    REQUIRE(dobj != nullptr);
    REQUIRE(dobj->next == nullptr);

    HSD_MObjDesc* const mobj = dobj->mobjdesc;
    REQUIRE(mobj != nullptr);
    REQUIRE(mobj->rendermode == 0x10);
    REQUIRE(mobj->pedesc == nullptr);
    REQUIRE(mobj->mat != nullptr);
    REQUIRE(mobj->mat->diffuse.r == 64);
    REQUIRE(mobj->mat->diffuse.g == 128);
    REQUIRE(mobj->mat->diffuse.b == 192);
    REQUIRE(mobj->mat->diffuse.a == 255);
    REQUIRE(near(mobj->mat->alpha, 0.5F));
    REQUIRE(near(mobj->mat->shininess, 2.0F));

    HSD_TObjDesc* const tobj = mobj->texdesc;
    REQUIRE(tobj != nullptr);
    REQUIRE(tobj->id == GX_TEXMAP0);
    REQUIRE(tobj->wrap_s == GX_REPEAT);
    REQUIRE(tobj->wrap_t == GX_CLAMP);
    REQUIRE(tobj->repeat_s == 1);
    REQUIRE(tobj->magFilt == GX_LINEAR);
    REQUIRE(tobj->lod == nullptr);
    REQUIRE(tobj->tev == nullptr);
    REQUIRE(tobj->imagedesc != nullptr);
    REQUIRE(tobj->imagedesc->width == 8);
    REQUIRE(tobj->imagedesc->height == 8);
    REQUIRE(tobj->imagedesc->format == GX_TF_C8);
    REQUIRE(near(tobj->imagedesc->maxLOD, 3.0F));
    REQUIRE(tobj->tlutdesc != nullptr);
    REQUIRE(tobj->tlutdesc->fmt == GX_TL_RGB565);
    REQUIRE(tobj->tlutdesc->tlut_name == 7);
    REQUIRE(tobj->tlutdesc->n_entries == 2);

    HSD_PObjDesc* const pobj = dobj->pobjdesc;
    REQUIRE(pobj != nullptr);
    REQUIRE(pobj->n_display == 1);
    REQUIRE(pobj->display != nullptr);
    // A GX payload is not translated: the display list has to stay the bytes
    // the file holds, because the interpreter reads them big-endian.
    REQUIRE(pobj->display[0] == 0x90);
    REQUIRE(pobj->display[2] == 0x03);

    HSD_VtxDescList* const verts = pobj->verts;
    REQUIRE(verts != nullptr);
    REQUIRE(verts[0].attr == GX_VA_POS);
    REQUIRE(verts[0].attr_type == GX_INDEX8);
    REQUIRE(verts[0].comp_cnt == GX_POS_XYZ);
    REQUIRE(verts[0].comp_type == GX_S16);
    REQUIRE(verts[0].frac == 2);
    REQUIRE(verts[0].stride == 6);
    REQUIRE(verts[0].vertex != nullptr);
    // The terminator the loaders stop at survives the copy.
    REQUIRE(verts[1].attr == GX_VA_NULL);

    // Host descriptors are larger than the records they come from, and they do
    // not touch the payload copy.
    REQUIRE(descriptors.stats().joints == 2);
    REQUIRE(descriptors.stats().pobj_descs == 1);
    REQUIRE(descriptors.stats().payload_bytes == kDataSize);
    REQUIRE(descriptors.stats().descriptor_bytes > 0);
}

TEST_CASE("the original object layer loads a materialized scene")
{
    REQUIRE(melee_host_baselib_bootstrap() == MELEE_HOST_OK);
    const SyntheticArchive source = make_scene();
    const melee::assets::HsdRuntimeArchive runtime(source.bytes);
    melee::assets::HsdMaterializedArchive descriptors(runtime);
    HSD_Joint* const joint =
        descriptors.scene_model_joint("test_scene_data", 0);

    const u32 live_before = jobjs_live();
    const u32 vectors_before = HSD_ObjAllocGetUsing(HSD_VecGetAllocData());
    const u32 matrices_before = HSD_ObjAllocGetUsing(HSD_MtxGetAllocData());
    HSD_JObj* const root = HSD_JObjLoadJoint(joint);
    REQUIRE(root != nullptr);
    REQUIRE(jobjs_live() == live_before + 2);

    // The loader copied the descriptor transform into the object.
    REQUIRE(near(root->translate.x, 1.0F));
    REQUIRE(near(root->translate.z, 3.0F));

    HSD_JObj* const child = root->child;
    REQUIRE(child != nullptr);
    REQUIRE(child->parent == root);
    REQUIRE(near(child->translate.y, 5.0F));
    // JObjLoad allocates an envelope matrix whenever the joint carries one.
    REQUIRE(child->envelopemtx != nullptr);
    REQUIRE(near(child->envelopemtx[0][0], 1.0F));

    HSD_DObj* const dobj = HSD_JObjGetDObj(child);
    REQUIRE(dobj != nullptr);
    REQUIRE(dobj->mobj != nullptr);
    // MObjLoad keeps the descriptor's render mode and adds RENDER_TOON.
    REQUIRE((dobj->mobj->rendermode & 0x10) != 0);
    REQUIRE(dobj->mobj->rendermode == (0x10 | RENDER_TOON));
    REQUIRE(dobj->mobj->tobj != nullptr);
    // The object points straight at the materialized descriptors, the way it
    // would point into an archive still resident in the console's heap.
    const HSD_MObjDesc* const mobjdesc = joint->child->u.dobjdesc->mobjdesc;
    REQUIRE(dobj->mobj->tobj->imagedesc == mobjdesc->texdesc->imagedesc);
    // MObjLoad copies the material rather than sharing the descriptor's.
    REQUIRE(dobj->mobj->mat != mobjdesc->mat);
    REQUIRE(dobj->mobj->mat->diffuse.r == 64);
    REQUIRE(near(dobj->mobj->mat->alpha, 0.5F));
    REQUIRE(dobj->pobj != nullptr);
    REQUIRE(dobj->pobj->n_display == 1);
    REQUIRE(dobj->pobj->verts == joint->child->u.dobjdesc->pobjdesc->verts);
    REQUIRE(dobj->pobj->display ==
            joint->child->u.dobjdesc->pobjdesc->display);

    // The matrix pass runs over objects the archive produced.  The root sits
    // at (1, 2, 3) with unit scale, so the child's world position is its own
    // offset added to it.
    HSD_JObjSetupMatrix(child);
    REQUIRE(near(child->mtx[0][3], 5.0F));
    REQUIRE(near(child->mtx[1][3], 7.0F));
    REQUIRE(near(child->mtx[2][3], 9.0F));

    HSD_JObjRemoveAll(root);
    REQUIRE(jobjs_live() == live_before);
    // The destructors give the pooled vectors and matrices back, including
    // the envelope matrix the loader took for the child.
    REQUIRE(HSD_ObjAllocGetUsing(HSD_VecGetAllocData()) == vectors_before);
    REQUIRE(HSD_ObjAllocGetUsing(HSD_MtxGetAllocData()) == matrices_before);
}

TEST_CASE("a materialized scene draws through the original display path")
{
    REQUIRE(melee_host_baselib_bootstrap() == MELEE_HOST_OK);
    const SyntheticArchive source = make_scene();
    const melee::assets::HsdRuntimeArchive runtime(source.bytes);
    melee::assets::HsdMaterializedArchive descriptors(runtime);
    HSD_Joint* const joint =
        descriptors.scene_model_joint("test_scene_data", 0);
    HSD_JObj* const root = HSD_JObjLoadJoint(joint);
    REQUIRE(root != nullptr);

    // The material's render mode has no blend bits, so DObjLoad puts the
    // object in the opaque pass.
    HSD_DObj* const dobj = HSD_JObjGetDObj(root->child);
    REQUIRE(dobj != nullptr);
    REQUIRE((dobj->flags & 2) != 0);

    melee_host_gx_state_reset();
    HSD_StateInvalidate(HSD_STATE_ALL);
    melee_host_gx_reset_command_log();
    HSD_VISetConfigure(&GXNtsc480IntDf);

    HSD_CObj* const camera = HSD_CObjAlloc();
    REQUIRE(camera != nullptr);
    Vec3 eye{ 0.0F, 0.0F, 500.0F };
    Vec3 interest{ 0.0F, 0.0F, 0.0F };
    HSD_CObjSetEyePosition(camera, &eye);
    HSD_CObjSetInterest(camera, &interest);
    HSD_CObjSetNear(camera, 1.0F);
    HSD_CObjSetFar(camera, 10000.0F);
    HSD_CObjSetPerspective(camera, 60.0F, 640.0F / 480.0F);
    HSD_CObjSetViewportfx4(camera, 0.0F, 640.0F, 0.0F, 480.0F);
    HSD_CObjSetScissorx4(camera, 0, 640, 0, 480);
    REQUIRE(HSD_CObjSetCurrent(camera));

    // An identity view makes the matrix the display path loads into GX the
    // joint's world transform, so the captured positions are world space.
    Mtx view;
    PSMTXIdentity(view);
    HSD_JObjDispAll(root, view, HSD_TRSP_OPA, 0);

    // The display list the archive holds reached GX and decoded, which means
    // the whole chain ran: materialized descriptors, the original loaders,
    // the original display path, then the host recorder.
    REQUIRE(melee_host_gx_display_list_error_count() == 0);
    REQUIRE(melee_host_gx_rejected_index_count() == 0);
    REQUIRE(melee_host_gx_triangle_count() == 1);
    REQUIRE(melee_host_gx_captured_vertex_count() == 3);

    MeleeHostGxCapturedTriangle triangle{};
    REQUIRE(melee_host_gx_captured_triangle_at(0, &triangle));
    // The S16 positions decode with two fractional bits, so 400 reads back as
    // 100, and GX transforms each by the position matrix the display path
    // loaded.  The child sits at (4, 5, 6) with scale 2 under a root at
    // (1, 2, 3), so the model origin lands at (5, 7, 9) and 100 along an axis
    // lands 200 further out.
    REQUIRE(near(triangle.vertices[0].position.x, 5.0F));
    REQUIRE(near(triangle.vertices[0].position.y, 7.0F));
    REQUIRE(near(triangle.vertices[0].position.z, 9.0F));
    REQUIRE(near(triangle.vertices[1].position.x, 205.0F));
    REQUIRE(near(triangle.vertices[2].position.y, 207.0F));

    HSD_CObjSetCurrent(nullptr);
    hsdDelete(camera);
    HSD_JObjRemoveAll(root);
}

TEST_CASE("an NBT stream keeps its index in the normal slot")
{
    // GX_VA_NBT describes the normal field, so its index sits where a normal
    // index would: after the position and before the texture coordinates, not
    // after them where its GXAttr number falls.  Each attribute below reads a
    // different element, so consuming them in the wrong order reads the wrong
    // array element and this test fails.
    melee_host_gx_state_reset();
    melee_host_gx_reset_command_log();

    std::vector<std::byte> positions(3 * 12);
    std::vector<std::byte> basis(3 * 36);
    std::vector<std::byte> texcoords(3 * 8);
    const auto put = [](std::vector<std::byte>& target, std::size_t offset,
                        float value) {
        const std::uint32_t bits = bits_of(value);
        target[offset] = static_cast<std::byte>(bits >> 24U);
        target[offset + 1] = static_cast<std::byte>(bits >> 16U);
        target[offset + 2] = static_cast<std::byte>(bits >> 8U);
        target[offset + 3] = static_cast<std::byte>(bits);
    };

    // Position element 1 is the one the stream asks for; the others differ so
    // a misread is visible.
    put(positions, 0, 9.0F);
    put(positions, 12, 1.0F);
    put(positions, 16, 2.0F);
    put(positions, 20, 3.0F);
    put(positions, 24, 7.0F);

    // Basis element 0 is the identity triple; element 2 is not.
    put(basis, 0, 1.0F);  // normal x
    put(basis, 16, 1.0F); // tangent y
    put(basis, 32, 1.0F); // binormal z
    put(basis, 72, 5.0F);

    // Texture coordinate element 2 is the one the stream asks for.
    put(texcoords, 0, 8.0F);
    put(texcoords, 16, 0.5F);
    put(texcoords, 20, 0.25F);

    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX8);
    GXSetVtxDesc(GX_VA_NBT, GX_INDEX8);
    GXSetVtxDesc(GX_VA_TEX0, GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_NBT, GX_NRM_NBT, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetArray(GX_VA_POS, positions.data(), 12);
    GXSetArray(GX_VA_NBT, basis.data(), 36);
    GXSetArray(GX_VA_TEX0, texcoords.data(), 8);

    // One triangle whose three vertices all read position 1, basis 0 and
    // texture coordinate 2, in the layout order the hardware uses.
    std::array<std::uint8_t, 32> list{};
    list[0] = 0x90; // GX_TRIANGLES | GX_VTXFMT0
    list[1] = 0x00;
    list[2] = 0x03;
    for (std::size_t vertex = 0; vertex < 3; ++vertex) {
        list[3 + vertex * 3] = 1; // position index
        list[4 + vertex * 3] = 0; // normal-basis index
        list[5 + vertex * 3] = 2; // texture coordinate index
    }
    GXCallDisplayList(list.data(), static_cast<u32>(list.size()));

    REQUIRE(melee_host_gx_display_list_error_count() == 0);
    REQUIRE(melee_host_gx_rejected_index_count() == 0);
    REQUIRE(melee_host_gx_triangle_count() == 1);
    MeleeHostGxCapturedTriangle triangle{};
    REQUIRE(melee_host_gx_captured_triangle_at(0, &triangle));
    const MeleeHostGxCapturedVertex& vertex = triangle.vertices[0];
    REQUIRE(near(vertex.position.x, 1.0F));
    REQUIRE(near(vertex.position.y, 2.0F));
    REQUIRE(near(vertex.position.z, 3.0F));
    REQUIRE(near(vertex.normal.x, 1.0F));
    REQUIRE(near(vertex.tangent.y, 1.0F));
    REQUIRE(near(vertex.binormal.z, 1.0F));
    REQUIRE(near(vertex.texcoord[0], 0.5F));
    REQUIRE(near(vertex.texcoord[1], 0.25F));
}

TEST_CASE("a pointer field the archive did not relocate is rejected")
{
    SyntheticArchive source = make_scene();
    // No relocation covers the reference constraint field, so a non-zero
    // value there is either a console address or a misread schema.
    source.field32(kChildJoint + 0x3C, kJointMtx);
    const melee::assets::HsdRuntimeArchive runtime(source.bytes);
    melee::assets::HsdMaterializedArchive descriptors(runtime);
    bool rejected = false;
    try {
        (void) descriptors.scene_model_joint("test_scene_data", 0);
    } catch (const melee::assets::HsdArchiveError&) {
        rejected = true;
    }
    REQUIRE(rejected);
}

TEST_CASE("a display list that runs past the data section is rejected")
{
    SyntheticArchive source = make_scene();
    source.field16(kPObjDesc + 0x0E, 0x1000);
    const melee::assets::HsdRuntimeArchive runtime(source.bytes);
    melee::assets::HsdMaterializedArchive descriptors(runtime);
    bool rejected = false;
    try {
        (void) descriptors.scene_model_joint("test_scene_data", 0);
    } catch (const melee::assets::HsdArchiveError&) {
        rejected = true;
    }
    REQUIRE(rejected);
}

TEST_CASE("the scene graphics facade reports an unknown symbol")
{
    MeleeHostSceneModel model = 0;
    REQUIRE(melee_host_scene_graphics_load_joint(
                "assets-local/does-not-exist.dat", "nope", &model) !=
            MELEE_HOST_OK);
    REQUIRE(std::string_view(melee_host_scene_graphics_last_error()).size() >
            0);
}
