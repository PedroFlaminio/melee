#ifndef MELEE_HOST_SCENE_GRAPHICS_H
#define MELEE_HOST_SCENE_GRAPHICS_H

#include <melee_host/host.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque handle to a scene graph the original object layer built.  The host
 * never sees an HSD_JObj, so the handle stays 32 bits wide regardless of the
 * runtime's pointer width. */
typedef mh_u32 MeleeHostSceneModel;

typedef struct MeleeHostSceneModelStats {
    /* Objects the original loaders allocated for this tree. */
    mh_u32 jobjs;
    mh_u32 dobjs;
    mh_u32 mobjs;
    mh_u32 tobjs;
    mh_u32 pobjs;
    /* Geometry reachable from the tree. */
    mh_u32 display_blocks;
    mh_u32 textured_pobjs;
    mh_u32 tree_depth;
    /* Why the render path can draw less than a schema that walks every PObj:
     * the original display code skips hidden objects, skips a PObj that culls
     * both faces, and only visits a JObj whose transparency bits name a pass
     * it is drawing. */
    mh_u32 hidden_jobjs;
    mh_u32 hidden_dobjs;
    mh_u32 culled_pobjs;
    mh_u32 undrawn_pobjs;
    /* Host memory the descriptor translation needed. */
    mh_u32 descriptor_bytes;
    mh_u32 payload_bytes;
} MeleeHostSceneModelStats;

/* Builds SceneDesc.models[index]->joint from an archive on disk, through the
 * original HSD_JObjLoadJoint and the object layer below it. */
MeleeHostStatus melee_host_scene_graphics_load_model(
    const char* path, const char* symbol, mh_u32 model_index,
    MeleeHostSceneModel* out_model);

/* The same, for a public symbol that names a joint directly. */
MeleeHostStatus melee_host_scene_graphics_load_joint(
    const char* path, const char* symbol, MeleeHostSceneModel* out_model);

MeleeHostStatus melee_host_scene_graphics_model_count(const char* path,
                                                      const char* symbol,
                                                      mh_u32* out_count);

MeleeHostStatus melee_host_scene_graphics_stats(
    MeleeHostSceneModel model, MeleeHostSceneModelStats* out_stats);

/* Runs the original matrix pass over the tree and reports where a joint ends
 * up, in the order the tree was walked.  This is the check that the loaded
 * graph is the one the read-only schema describes. */
MeleeHostStatus melee_host_scene_graphics_joint_world_position(
    MeleeHostSceneModel model, mh_u32 joint_index, float* out_xyz);

typedef struct MeleeHostSceneRenderStats {
    /* Geometry the GX recorder captured, per transparency pass and in
     * total.  The passes are the ones the original render callback runs, in
     * its order: opaque, texture-edge, translucent. */
    mh_u32 triangles;
    mh_u32 vertices;
    mh_u32 pass_triangles[3];
    mh_u32 textured_triangles;
    /* Display lists the interpreter rejected.  Any of these means the stream
     * the original code handed GX was not one the host could follow. */
    mh_u32 display_list_errors;
    /* Distinct textures the draws bound, which a consumer uploads once each
     * and indexes by the id the captured vertices carry. */
    mh_u32 textures;
    /* Distinct pixel states the draws ran under.  More than one means the
     * asset changes blend, culling or depth between objects, which a viewer
     * has to follow rather than assume. */
    mh_u32 draw_states;
    mh_u32 tev_states;
    /* Triangles whose material program the host could read off exactly, and
     * those it had to approximate.  The second number is the honest measure of
     * how far the image is from what the asset describes. */
    mh_u32 shading_exact_triangles;
    mh_u32 shading_approximated_triangles;
    /* Whether the scene's own camera was used.  False means the host's stand-in
     * view was used instead, either because the caller asked for it or because
     * the scene names no camera. */
    bool used_scene_camera;
    /* Indexed reads that fell outside a bounded vertex array.  The host drops
     * the attribute instead of reading past the archive, so a non-zero count
     * means the capture is missing data the console would have read out of
     * whatever followed the file in memory. */
    mh_u32 rejected_indices;
} MeleeHostSceneRenderStats;

typedef enum MeleeHostSceneView {
    /* The scene's own HSD_CObjDesc, which is what the game draws through.
     * Captured positions come out in that camera's view space.  Falls back to
     * the host's stand-in camera when the scene names none. */
    MELEE_HOST_SCENE_VIEW_SCENE_CAMERA = 0,
    /* An identity view, which leaves the matrices the display path loads as
     * pure world transforms.  Captured positions come out in world space,
     * which is what a viewer that moves its own camera wants. */
    MELEE_HOST_SCENE_VIEW_WORLD = 1,
} MeleeHostSceneView;

/* Runs the tree through HSD_JObjDispAll, which is the render path every scene
 * in the game takes, and reports what reached the host GX recorder.  The
 * captured vertices are transformed by the position matrix the display path
 * loaded, so `view` decides the space they come out in. */
MeleeHostStatus melee_host_scene_graphics_render(
    MeleeHostSceneModel model, MeleeHostSceneView view,
    MeleeHostSceneRenderStats* out_stats);

/* Releases the tree through the original destructors, then the descriptors. */
MeleeHostStatus
melee_host_scene_graphics_release(MeleeHostSceneModel model);

/* Describes why the last call failed.  Never NULL. */
const char* melee_host_scene_graphics_last_error(void);

#ifdef __cplusplus
}
#endif

#endif
