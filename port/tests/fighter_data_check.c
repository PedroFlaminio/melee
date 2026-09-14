/* Checks, through the game's own C types, the ftLoadCommonData the host
 * archive API translated from the archive hsd_host_archive_test.cpp builds,
 * which C++ cannot include the fighter headers to read. */

#include <melee/ft/fighter.h>
#include <melee/ft/types.h>
#include <melee/lb/types.h>
#include <melee/sfx/crowdsfx.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

int melee_host_test_check_fighter_common_data(void* translated, char* message,
                                              size_t size);

#define CHECK(condition)                                                      \
    do {                                                                      \
        if (!(condition)) {                                                   \
            snprintf(message, size, "%s", #condition);                        \
            return 0;                                                         \
        }                                                                     \
    } while (0)

int melee_host_test_check_fighter_common_data(void* translated, char* message,
                                              size_t size)
{
    void** const tables = translated;
    const ftCommonData* common;
    const float* rows;
    float (*swing)[5];
    FighterPartsTable** parts;
    Fighter_804D6540_t** records;
    struct Fighter_804D653C_t* anims;
    struct Fighter_804D653C_t* platform_anims;
    void** platform;
    Vec2** pairs;
    struct Fighter_ShakeTable_t* shake;
    struct Fighter_804D64FC_t* cpu;
    const s32* attack;

    CHECK(tables != NULL);

    /* ftCommonData: words, and the colors and x6EC as bytes. */
    common = tables[0];
    CHECK(common != NULL);
    CHECK(common->horizontal_stick_deadzone == 0.25f);
    CHECK(common->x6DC_colorsByPlayer[0].r == 1 &&
          common->x6DC_colorsByPlayer[0].a == 4);
    CHECK(common->x6EC[0] == 9);
    CHECK(common->x7D8.r == 0x10 && common->x7D8.a == 0x40);
    CHECK(common->x814 == 7);

    /* Scalar arrays sized by the room before the next table. */
    rows = tables[1];
    CHECK(rows != NULL && rows[0] == 1.5f && rows[5] == 2.5f);
    swing = tables[2];
    CHECK(swing != NULL && swing[0][1] == 3.0f);
    CHECK(tables[3] != NULL && ((const float*) tables[3])[1] == 0.5f);

    /* Per-kind tables pointing at bytes. */
    parts = tables[4];
    CHECK(parts != NULL && parts[0] != NULL && parts[1] == NULL);
    CHECK(parts[0]->parts_num == 3);
    CHECK(parts[0]->joint_to_part[1] == 6 && parts[0]->part_to_joint[2] == 10);
    records = tables[5];
    CHECK(records != NULL && records[0] != NULL);
    CHECK(records[0]->x4 == 1 && records[0]->x0[0].x0 == 4);

    /* Both color animation tables reach one converted script. */
    anims = tables[6];
    CHECK(anims != NULL && anims[0].unk == NULL && anims[1].unk != NULL);
    CHECK(anims[1].unk4 == 30);
    CHECK(((const u32*) anims[1].unk)[0] == ((1U << 26) | 5U));
    platform_anims = tables[7];
    CHECK(platform_anims != NULL && platform_anims[0].unk == anims[1].unk);
    CHECK(platform_anims[0].unk4 == 50);

    /* One joint shared by the respawn platform and both joint entries. */
    platform = tables[8];
    CHECK(platform != NULL && platform[0] != NULL && platform[1] != NULL);
    CHECK(platform[0] == tables[16] && tables[16] == tables[20]);

    /* A Vec2 list, then its length in a pointer-wide slot. */
    pairs = tables[9];
    CHECK(pairs != NULL && pairs[0] != NULL && pairs[0][1].y == 4.0f);
    CHECK((u8) (u32) (intptr_t) pairs[1] == 2);

    shake = tables[10];
    CHECK(shake != NULL && shake->x4 == 1 && shake->x0[0].x == 5.0f);
    shake = tables[11];
    CHECK(shake != NULL && shake->x4 == 1 && shake->x0[0].y == 6.0f);

    CHECK(((struct Fighter_804D6524_t*) tables[12])->x0 == 1.25f);
    CHECK(((struct Fighter_804D6520_t*) tables[13])->x0 == 0.75f);
    CHECK(((struct Fighter_804D651C_t*) tables[14])->xC == -2.0f);
    CHECK(((struct Fighter_804D6518_t*) tables[15])->x4 == 4.0f);

    CHECK(tables[17] == NULL);
    CHECK(((const u8*) tables[18])[1] == 0x22);
    CHECK(((const u8*) tables[19])[0] == 0x33);
    CHECK(((CrowdConfig*) tables[21])->kb_threshold_low == 30.0f);

    /* The CPU tables: a byte script, an attack list and two float arrays. */
    cpu = tables[22];
    CHECK(cpu != NULL);
    CHECK(cpu->cmdscripts != NULL && cpu->cmdscripts[0] != NULL);
    CHECK(cpu->cmdscripts[0][0] == 0x92 && cpu->cmdscripts[1] == NULL);
    CHECK(cpu->x4 != NULL && cpu->x4[0] != NULL);
    attack = cpu->x4[0];
    CHECK(attack[0] == 2 && ((const f32*) attack)[6] == 0.5f);
    CHECK(attack[9] == 0);
    CHECK(cpu->x8 == NULL);
    CHECK(cpu->x20 != NULL && cpu->x20[1] == 13.0f);
    CHECK(cpu->x24 != NULL && ((const f32*) cpu->x24)[0] == 7.0f);
    return 1;
}
