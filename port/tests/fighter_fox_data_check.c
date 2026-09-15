/* Checks, through the game's own C types, the ftDataFox the host archive API
 * translated from the archive hsd_host_archive_test.cpp builds, which C++
 * cannot include the fighter headers to read. */

#include <melee/ft/dobjlist.h>
#include <melee/ft/ftwaitanim.h>
#include <melee/ft/kinds/ftCommon/types.h>
#include <melee/ft/kinds/ftFox/types.h>
#include <melee/ft/types.h>
#include <melee/it/itCharItems.h>
#include <melee/it/types.h>
#include <melee/lb/types.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

int melee_host_test_check_fighter_fox_data(void* translated, char* message,
                                           size_t size);

#define CHECK(condition)                                                      \
    do {                                                                      \
        if (!(condition)) {                                                   \
            snprintf(message, size, "%s", #condition);                        \
            return 0;                                                         \
        }                                                                     \
    } while (0)

int melee_host_test_check_fighter_fox_data(void* translated, char* message,
                                           size_t size)
{
    const struct ftData* const data = translated;
    const struct ftFox_DatAttrs* fox;
    const struct ftData_x8* parts;
    const FtPartsVisLookup* lookup;
    const Fighter_WaitAnimData* actions;
    const WaitStruct* pairs;
    const struct ftDynamics* dynamics;
    const BoneDynamicsDesc* bone;
    const FtSFX* sounds;

    CHECK(data != NULL);

    /* The attributes: words, then the throw mask as a byte; Fox's end in the
     * reflector's behavior byte. */
    CHECK(data->x0 != NULL && data->x0->walk_accel_mul == 1.5f);
    CHECK(data->x0->weight_independent_throws_mask == 0x81);
    fox = data->ext_attr;
    CHECK(fox != NULL && fox->x0_FOX_BLASTER_UNK1 == 2.5f);
    CHECK(fox->xB0_FOX_REFLECTOR_REFLECTION.x20_behavior == 1);

    /* Parts: one model whose visibility row names a lookup of one TempS, and
     * one row of two TObj indices. */
    parts = data->x8;
    CHECK(parts != NULL && parts->x0.model_num == 1);
    CHECK(parts->x0.vis_table != NULL);
    lookup = parts->x0.vis_table[0][0];
    CHECK(lookup != NULL && parts->x0.vis_table[0][1] == NULL);
    CHECK(lookup[0].x0 == 1 && lookup[0].x4 != NULL);
    CHECK(lookup[0].x4[0].x0 == 2 && lookup[0].x4[0].x4[1] == 5);
    CHECK(parts->x8.x8 == 2 && parts->x8.xC != NULL);
    CHECK(parts->x8.xC[0][0] == 7 && parts->x8.xC[0][1] == 8);
    CHECK(parts->x10 == 3 && parts->x14 == 9);

    /* Two actions sized by the room before the next table: a named one with a
     * converted script, then an empty one.  x14 starts empty; the game writes
     * the animation's address there. */
    actions = data->xC;
    CHECK(actions != NULL);
    CHECK(actions[0].x0 != NULL && actions[0].x0[0] == 'W');
    CHECK(actions[0].x4 == 0x100 && actions[0].x8 == 0x40);
    CHECK(actions[0].xC != NULL);
    CHECK(((const u32*) (void*) actions[0].xC)[0] == ((1U << 26) | 5U));
    CHECK(actions[0].x10_animCurrFlags == 7 && actions[0].x14 == 0);
    CHECK(actions[1].x0 == NULL && actions[1].xC == NULL);
    CHECK(data->x14 != NULL && data->x14[0].x4 == 0x20 &&
          data->x14[0].x8 == 0x10);

    /* x20: small integers beside a joint. */
    CHECK(data->x20 != NULL && data->x20->x8 == 1.25f);
    CHECK(data->x20->x0 != NULL);
    CHECK((intptr_t) data->x20->x0[0] == 5 && data->x20->x0[1] == NULL);
    CHECK(data->x20->x0[2] != NULL && data->x20->x0[3] == NULL);

    /* Wait pairs, widened, up to and including the -1 entry. */
    pairs = data->x24;
    CHECK(pairs != NULL && pairs[0].u.i.x == 1 && pairs[0].u.i.y == 2);
    CHECK(pairs[1].u.i.x == -1);

    dynamics = data->x2C;
    CHECK(dynamics != NULL && dynamics->dynamicsNum == 1);
    CHECK(dynamics->ftDynamicBones != NULL);
    bone = &dynamics->ftDynamicBones->array[0];
    CHECK(bone->bone_id == 7 && bone->dyn_desc.count == 1);
    CHECK(bone->dyn_desc.data != NULL &&
          ((const f32*) (void*) bone->dyn_desc.data)[0] == 0.5f);
    CHECK(bone->dyn_desc.pos.x == 1.0f && bone->dyn_desc.pos.z == 3.0f);
    CHECK(dynamics->x4 == 3 && dynamics->x8 != NULL);
    CHECK(((const f32*) (void*) dynamics->x8)[1] == 10.0f);
    CHECK(dynamics->x10 == NULL);

    CHECK(data->x30 != NULL && data->x30->count == 1);
    CHECK(data->x30->inits != NULL && data->x30->inits[0].bone_idx == 5);
    CHECK(data->x30->inits[0].scale == 2.0f);

    CHECK(data->x44 != NULL && data->x44->unk0 == -3 &&
          data->x44->unkC == 4.5f);

    /* An item slot that names int pairs keeps its bytes; the blaster's slot
     * gets its special attributes as floats. */
    CHECK(data->x48_items != NULL);
    CHECK(((const u8*) data->x48_items[0])[3] == 3);
    {
        const Article* const blaster = data->x48_items[1];
        const FoxBlasterAttr* attrs;

        CHECK(blaster != NULL && blaster->x0_common_attr != NULL);
        CHECK(blaster->x14_dynamics == NULL);
        attrs = blaster->x4_specialAttributes;
        CHECK(attrs != NULL && attrs->x0 == 4.5f);
        CHECK(attrs->x18 == 1.0f && attrs->x24 == 7.25f);
    }

    sounds = data->x4C_sfx;
    CHECK(sounds != NULL && sounds->x4 == 11 && sounds->x1C == NULL);
    CHECK(sounds->smash != NULL && sounds->smash->num == 2);
    CHECK(sounds->smash->sfx_ids[1] == 101);
    CHECK(sounds->x20 != NULL && sounds->x20->sfx_ids[0] == 100);

    CHECK(data->x54 != NULL && data->x54[0] == 9 && data->x54[1] == 8);
    CHECK(data->x58 != NULL && data->x58->x0 == 1);
    CHECK(data->x58->x4 == 2.5f && data->x58->x18 == 6.0f);
    CHECK(data->x5C == NULL && data->x10 == NULL && data->x1C == NULL);
    return 1;
}
