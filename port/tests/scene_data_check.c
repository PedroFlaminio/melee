/* Checks, through the game's SceneDesc, the `_scene_data` symbol the host
 * archive API translated from the archive hsd_host_archive_test.cpp builds. */

#include <melee/sc/types.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/fog.h>
#include <sysdolphin/baselib/lobj.h>

#include <stddef.h>
#include <stdio.h>

int melee_host_test_check_scene_data(void* translated, char* message,
                                     size_t size);

#define CHECK(condition)                                                      \
    do {                                                                      \
        if (!(condition)) {                                                   \
            snprintf(message, size, "%s", #condition);                        \
            return 0;                                                         \
        }                                                                     \
    } while (0)

int melee_host_test_check_scene_data(void* translated, char* message,
                                     size_t size)
{
    const SceneDesc* const scene = translated;
    const DynamicModelDesc* model;

    CHECK(scene != NULL);

    /* One model: its joint and a material animation table of one. */
    CHECK(scene->models != NULL && scene->models[0] != NULL);
    CHECK(scene->models[1] == NULL);
    model = scene->models[0];
    CHECK(model->joint != NULL);
    CHECK(model->anims == NULL && model->shapeanims == NULL);
    CHECK(model->matanims != NULL && model->matanims[0] != NULL);
    CHECK(model->matanims[1] == NULL);

    /* One camera, with an animation table of one. */
    CHECK(scene->cameras != NULL && scene->cameras[0].desc != NULL);
    CHECK(scene->cameras[0].desc->common.nnear == 1.5f);
    CHECK(scene->cameras[0].anims != NULL &&
          scene->cameras[0].anims[0] != NULL);
    CHECK(scene->cameras[0].anims[1] == NULL);

    /* Light lists, NULL-terminated, with their animations. */
    CHECK(scene->lights != NULL && scene->lights[0] != NULL);
    CHECK(scene->lights[1] == NULL);
    CHECK(scene->lights[0]->desc != NULL);
    CHECK(scene->lights[0]->anims != NULL && scene->lights[0]->anims[0] != NULL);

    /* One fog, which the SceneDesc itself follows. */
    CHECK(scene->fogs != NULL && scene->fogs[0].desc != NULL);
    CHECK(scene->fogs[0].desc->start == 10.0f);
    CHECK(scene->fogs[0].anims == NULL);
    return 1;
}
