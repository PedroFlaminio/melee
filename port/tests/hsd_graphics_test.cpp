#include "hsd_include.hpp"
#include "test.hpp"

#include <melee_host/baselib.h>
#include <melee_host/scene_runtime.h>

MELEE_HOST_TEST_HSD_BEGIN
#include <dolphin/mtx.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/objalloc.h>
MELEE_HOST_TEST_HSD_END

#include <cmath>

namespace {

bool near(f32 value, f32 expected, f32 tolerance = 1.0e-4F)
{
    return std::fabs(value - expected) <= tolerance;
}

} // namespace

TEST_CASE("a GObj owning a real JObj releases it through the class destructor")
{
    REQUIRE(melee_host_scene_runtime_init() == MELEE_HOST_OK);

    HSD_JObj* joint = HSD_JObjAlloc();
    REQUIRE(joint != nullptr);

    HSD_GObj* gobj = GObj_Create(0x700, 4, 0);
    REQUIRE(gobj != nullptr);
    // This is the path ground.c and fighter.c take.  Until the HSD graphics
    // objects were ported, freeing such a GObj hit a deliberate abort.
    HSD_GObjObject_80390A70(gobj, HSD_GObj_JObjKind, joint);
    REQUIRE(HSD_GObjGetHSDObj(gobj) == joint);

    HSD_GObjFree(gobj);
    REQUIRE(HSD_GObjPLinkHead[4] == nullptr);
}

TEST_CASE("a real JObj builds its own transform and returns its pooled vector")
{
    REQUIRE(melee_host_baselib_bootstrap() == MELEE_HOST_OK);
    const u32 vectors_before =
        HSD_ObjAllocGetUsing(HSD_VecGetAllocData());

    HSD_JObj* joint = HSD_JObjAlloc();
    REQUIRE(joint != nullptr);

    joint->translate = Vec3{ 10.0F, -4.0F, 2.0F };
    joint->scale = Vec3{ 2.0F, 2.0F, 2.0F };
    joint->rotate = Quaternion{ 0.0F, 0.0F, 0.0F, 1.0F };
    // A joint only rebuilds its matrix when the dirty bit is set; the loader
    // sets it when it reads a joint from an archive.
    joint->flags |= JOBJ_MTX_DIRTY;
    HSD_JObjSetupMatrix(joint);

    REQUIRE(near(joint->mtx[0][3], 10.0F));
    REQUIRE(near(joint->mtx[1][3], -4.0F));
    REQUIRE(near(joint->mtx[2][3], 2.0F));
    REQUIRE(near(joint->mtx[0][0], 2.0F));
    REQUIRE(near(joint->mtx[1][1], 2.0F));
    // Building the matrix takes a vector from the original pool for the
    // accumulated scale.
    REQUIRE(HSD_ObjAllocGetUsing(HSD_VecGetAllocData()) > vectors_before);

    HSD_JObjUnref(joint);
    REQUIRE(HSD_ObjAllocGetUsing(HSD_VecGetAllocData()) == vectors_before);
}

TEST_CASE("a real CObj builds a viewing matrix that centres on its interest")
{
    REQUIRE(melee_host_baselib_bootstrap() == MELEE_HOST_OK);

    HSD_CObj* camera = HSD_CObjAlloc();
    REQUIRE(camera != nullptr);

    Vec3 eye{ 0.0F, 0.0F, 20.0F };
    Vec3 interest{ 0.0F, 0.0F, 0.0F };
    HSD_CObjSetEyePosition(camera, &eye);
    HSD_CObjSetInterest(camera, &interest);
    HSD_CObjSetNear(camera, 1.0F);
    HSD_CObjSetFar(camera, 100.0F);

    Mtx view;
    HSD_CObjGetViewingMtx(camera, view);

    // The eye maps to the origin of view space and the interest sits down its
    // negative z axis, 20 units away.
    Vec eye_in_view;
    Vec interest_in_view;
    Vec eye_point{ eye.x, eye.y, eye.z };
    Vec interest_point{ interest.x, interest.y, interest.z };
    PSMTXMultVec(view, &eye_point, &eye_in_view);
    PSMTXMultVec(view, &interest_point, &interest_in_view);
    REQUIRE(near(eye_in_view.x, 0.0F));
    REQUIRE(near(eye_in_view.y, 0.0F));
    REQUIRE(near(eye_in_view.z, 0.0F));
    REQUIRE(near(interest_in_view.z, -20.0F));

    hsdDelete(camera);
}

TEST_CASE("light projections map the view frustum onto the unit texture")
{
    // The shadow code passes scale 0.5 and translate 0.5, which is the mapping
    // from the [-1, 1] device range to the [0, 1] texture range.
    const f32 scale = 0.5F;
    const f32 translate = 0.5F;

    Mtx perspective;
    MTXLightPerspective(perspective, 90.0F, 1.0F, scale, -scale, translate,
                        translate);

    // A point on the view axis lands at the centre of the texture.
    Vec centre{ 0.0F, 0.0F, -10.0F };
    Vec projected;
    PSMTXMultVec(perspective, &centre, &projected);
    REQUIRE(near(projected.z, 10.0F));
    REQUIRE(near(projected.x / projected.z, translate));
    REQUIRE(near(projected.y / projected.z, translate));

    // At a 90 degree field of view the frustum edge sits at x = -z, and the
    // right edge must land one half-width away from the centre.
    Vec edge{ 10.0F, 0.0F, -10.0F };
    PSMTXMultVec(perspective, &edge, &projected);
    REQUIRE(near(projected.x / projected.z, translate + scale));

    // The orthographic form needs no divide, so q stays one.
    Mtx ortho;
    MTXLightOrtho(ortho, 1.0F, -1.0F, -2.0F, 2.0F, scale, scale, translate,
                  translate);
    Vec ortho_centre{ 0.0F, 0.0F, -5.0F };
    PSMTXMultVec(ortho, &ortho_centre, &projected);
    REQUIRE(near(projected.z, 1.0F));
    REQUIRE(near(projected.x, translate));

    Vec ortho_edge{ 2.0F, 1.0F, -5.0F };
    PSMTXMultVec(ortho, &ortho_edge, &projected);
    REQUIRE(near(projected.x, translate + scale));
    REQUIRE(near(projected.y, translate + scale));

    // A symmetric frustum must agree with the perspective form it describes.
    Mtx frustum;
    MTXLightFrustum(frustum, 10.0F, -10.0F, -10.0F, 10.0F, 10.0F, scale,
                    -scale, translate, translate);
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 4; ++column) {
            REQUIRE(near(frustum[row][column], perspective[row][column]));
        }
    }
}
