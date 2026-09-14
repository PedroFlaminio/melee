/* Game data whose layout only the game's C headers describe, translated for
 * the host's archive API (melee_host_hsd_register_translator).
 *
 * Each translator reads the file big-endian through the reader and fills the
 * game's own types field by field, so the host compiler lays the result out
 * and the only offsets written by hand are the PowerPC ones on disk.  What a
 * translator cannot give a host meaning to is left out on purpose and said so
 * at the field, rather than handed to the game as bytes it would misread. */

#include <melee_host/boot.h>
#include <melee_host/hsd_archive.h>

#include <melee/gm/gmevent.h>
#include <melee/gr/types.h>
#include <melee/pl/types.h>
#include <melee/ty/types.h>

#include <stdalign.h>
#include <stddef.h>
#include <string.h>

/* A pointer field of the record at `at`: its target, or 0 with *present
 * false when the field is NULL. */
static mh_u32 target_of(MeleeHostHsdReader* reader, mh_u32 field,
                        bool* present)
{
    mh_u32 target = 0;
    *present = melee_host_hsd_reader_pointer(reader, field, &target);
    return target;
}

static gm_801BAB40_src* event_player(MeleeHostHsdReader* reader, mh_u32 at)
{
    gm_801BAB40_src* const player = melee_host_hsd_reader_allocate(
        reader, sizeof(*player), alignof(gm_801BAB40_src));
    if (player == NULL) {
        return NULL;
    }
    player->c_kind = (s8) melee_host_hsd_reader_u8(reader, at + 0x00);
    player->slot_type = melee_host_hsd_reader_u8(reader, at + 0x01);
    player->stocks = melee_host_hsd_reader_u8(reader, at + 0x02);
    player->color = melee_host_hsd_reader_u8(reader, at + 0x03);
    player->x5 = melee_host_hsd_reader_u8(reader, at + 0x04);
    player->sub_color = melee_host_hsd_reader_u8(reader, at + 0x05);
    player->team = melee_host_hsd_reader_u8(reader, at + 0x06);
    player->xB = melee_host_hsd_reader_u8(reader, at + 0x07);
    player->flags = melee_host_hsd_reader_u8(reader, at + 0x08);
    player->xE = melee_host_hsd_reader_u8(reader, at + 0x09);
    player->cpu_level = melee_host_hsd_reader_u8(reader, at + 0x0A);
    player->pad = melee_host_hsd_reader_u8(reader, at + 0x0B);
    player->x12 = melee_host_hsd_reader_u16(reader, at + 0x0C);
    player->hp = melee_host_hsd_reader_u16(reader, at + 0x0E);
    player->x18 = melee_host_hsd_reader_f32(reader, at + 0x10);
    player->x1C = melee_host_hsd_reader_f32(reader, at + 0x14);
    player->x20 = melee_host_hsd_reader_f32(reader, at + 0x18);
    return player;
}

static struct gm_evinit* event_init(MeleeHostHsdReader* reader, mh_u32 at)
{
    struct gm_evinit* const init = melee_host_hsd_reader_allocate(
        reader, sizeof(*init), alignof(struct gm_evinit));
    const mh_u8 byte0 = melee_host_hsd_reader_u8(reader, at + 0x00);
    const mh_u8 byte1 = melee_host_hsd_reader_u8(reader, at + 0x01);
    int i;

    if (init == NULL) {
        return NULL;
    }
    /* MWCC allocates bit-fields from the most significant bit down, so the
     * first field is the top of the byte.  The host compiler allocates them
     * its own way, which is why they are assigned by name. */
    init->x0_0 = (u32) (byte0 >> 5) & 7U;
    init->x0_3 = (u32) (byte0 >> 2) & 7U;
    init->x0_6 = (u32) (byte0 >> 1) & 1U;
    init->x0_7 = (u32) byte0 & 1U;
    init->x1_0 = (u32) (byte1 >> 7) & 1U;
    init->x1_1 = (u32) (byte1 >> 6) & 1U;
    init->x1_2 = (u32) (byte1 >> 5) & 1U;
    init->x1_3 = (u32) (byte1 >> 4) & 1U;
    init->x1_4 = (u32) (byte1 >> 3) & 1U;
    init->x1_5 = (u32) byte1 & 7U;
    init->is_teams = melee_host_hsd_reader_u8(reader, at + 0x02);
    init->item_freq = (s8) melee_host_hsd_reader_u8(reader, at + 0x03);
    init->sd_penalty = (s8) melee_host_hsd_reader_u8(reader, at + 0x04);
    init->unk5 = melee_host_hsd_reader_u8(reader, at + 0x05);
    init->stkind = melee_host_hsd_reader_u16(reader, at + 0x06);
    init->time_limit = melee_host_hsd_reader_u32(reader, at + 0x08);
    for (i = 0; i < 4; i++) {
        init->padC[i] = melee_host_hsd_reader_u8(reader, at + 0x0C + i);
    }
    init->x10 = ((u64) melee_host_hsd_reader_u32(reader, at + 0x10) << 32) |
                melee_host_hsd_reader_u32(reader, at + 0x14);
    init->x18 = (s32) melee_host_hsd_reader_u32(reader, at + 0x18);
    init->x1C = melee_host_hsd_reader_f32(reader, at + 0x1C);
    init->game_speed = melee_host_hsd_reader_f32(reader, at + 0x20);
    init->unk24 = melee_host_hsd_reader_f32(reader, at + 0x24);
    return init;
}

static struct gm_evbonus* event_bonus(MeleeHostHsdReader* reader, mh_u32 at)
{
    struct gm_evbonus* const bonus = melee_host_hsd_reader_allocate(
        reader, sizeof(*bonus), alignof(struct gm_evbonus));
    if (bonus == NULL) {
        return NULL;
    }
    bonus->c_kind = (s8) melee_host_hsd_reader_u8(reader, at + 0x00);
    bonus->x1 = melee_host_hsd_reader_u8(reader, at + 0x01);
    bonus->x2 = melee_host_hsd_reader_u8(reader, at + 0x02);
    bonus->x3 = melee_host_hsd_reader_u8(reader, at + 0x03);
    bonus->x4 = melee_host_hsd_reader_u8(reader, at + 0x04);
    bonus->x5 = melee_host_hsd_reader_u8(reader, at + 0x05);
    bonus->color = melee_host_hsd_reader_u8(reader, at + 0x06);
    bonus->pad7 = melee_host_hsd_reader_u8(reader, at + 0x07);
    bonus->x8 = melee_host_hsd_reader_f32(reader, at + 0x08);
    bonus->xC = melee_host_hsd_reader_f32(reader, at + 0x0C);
    bonus->x10 = melee_host_hsd_reader_f32(reader, at + 0x10);
    bonus->flags = melee_host_hsd_reader_u8(reader, at + 0x14);
    bonus->x15 = melee_host_hsd_reader_u8(reader, at + 0x15);
    bonus->x16 = melee_host_hsd_reader_u8(reader, at + 0x16);
    bonus->x17 = melee_host_hsd_reader_u8(reader, at + 0x17);
    return bonus;
}

static struct gm_evstage_table* event_stages(MeleeHostHsdReader* reader,
                                             mh_u32 at)
{
    struct gm_evstage_table* const stages = melee_host_hsd_reader_allocate(
        reader, sizeof(*stages), alignof(struct gm_evstage_table));
    int i;

    if (stages == NULL) {
        return NULL;
    }
    stages->count = melee_host_hsd_reader_u8(reader, at + 0x00);
    stages->pad1 = melee_host_hsd_reader_u8(reader, at + 0x01);
    for (i = 0; i < 7; i++) {
        stages->stage[i] =
            melee_host_hsd_reader_u16(reader, at + 0x02 + (mh_u32) i * 2);
    }
    for (i = 0; i < GM_MAX_PLAYERS; i++) {
        bool present;
        const mh_u32 player =
            target_of(reader, at + 0x10 + (mh_u32) i * 4, &present);
        stages->entries[i] = present ? event_player(reader, player) : NULL;
    }
    return stages;
}

static struct gm_804D6900_t* event_level(MeleeHostHsdReader* reader,
                                         mh_u32 at)
{
    struct gm_804D6900_t* const level = melee_host_hsd_reader_allocate(
        reader, sizeof(*level), alignof(struct gm_804D6900_t));
    bool present;
    mh_u32 target;
    int i;

    if (level == NULL) {
        return NULL;
    }
    level->kind = melee_host_hsd_reader_u8(reader, at + 0x00);
    level->flags = melee_host_hsd_reader_u8(reader, at + 0x01);
    level->pad2[0] = melee_host_hsd_reader_u8(reader, at + 0x02);
    level->pad2[1] = melee_host_hsd_reader_u8(reader, at + 0x03);

    /* x4 is each event's own parameters, and their shape changes with the
     * event: two ints, a coin count, three floats, a character list ending in
     * ChKind_Max, a list of ints, and in one level an int and a pointer.  Only
     * that event's code knows which, and it reads the bytes in place, so no
     * layout given here would be right for all of them.  The field is checked
     * and left NULL; everything that reads it is event mode code, which the
     * host's mode table does not have yet. */
    (void) target_of(reader, at + 0x04, &present);
    level->x4 = NULL;

    target = target_of(reader, at + 0x08, &present);
    level->evinit = present ? event_init(reader, target) : NULL;
    target = target_of(reader, at + 0x0C, &present);
    level->evbonus = present ? event_bonus(reader, target) : NULL;
    target = target_of(reader, at + 0x10, &present);
    level->evstage_table = present ? event_stages(reader, target) : NULL;
    for (i = 0; i < 5; i++) {
        target = target_of(reader, at + 0x14 + (mh_u32) i * 4, &present);
        level->player_init[i] = present ? event_player(reader, target) : NULL;
    }
    return level;
}

/* sqEventInitDataLevelTbl: one pointer per event, as many as are relocated in
 * a row.  GmEvent.dat has 51, the number gm_801BEBC0 searches. */
static void* event_level_table(MeleeHostHsdReader* reader, mh_u32 root)
{
    const mh_u32 data_size = melee_host_hsd_reader_data_size(reader);
    struct gm_804D6900_t** table;
    mh_u32 count = 0;
    mh_u32 i;

    while (root + (count + 1) * 4 <= data_size &&
           melee_host_hsd_reader_has_pointer(reader, root + count * 4))
    {
        count++;
    }
    if (count == 0) {
        melee_host_hsd_reader_fail(reader, "the event level table is empty");
        return NULL;
    }
    table = melee_host_hsd_reader_allocate(reader, sizeof(*table) * count,
                                           alignof(struct gm_804D6900_t*));
    if (table == NULL) {
        return NULL;
    }
    for (i = 0; i < count; i++) {
        bool present;
        const mh_u32 level = target_of(reader, root + i * 4, &present);
        table[i] = present ? event_level(reader, level) : NULL;
    }
    return melee_host_hsd_reader_failed(reader) ? NULL : table;
}

/* lbAudioLoadData in LbAd.dat: four tables, one per pairing of the language
 * setting and the saved language, each with the 30 lists lbAudioAx_80023968
 * accepts.  A list holds sound bank ids and ends in 0x83D60; the game reads
 * the ids in place, so the host converts each list to its own byte order,
 * terminator included.  Some lists start inside others, and each one is
 * walked to its own terminator as the game walks it. */
enum {
    AUDIO_LOAD_TABLES = 4,
    AUDIO_LOAD_LISTS = 30,
    AUDIO_LOAD_END = 0x83D60,
};

static int* audio_load_list(MeleeHostHsdReader* reader, mh_u32 at)
{
    const mh_u32 data_size = melee_host_hsd_reader_data_size(reader);
    mh_u32 count = 0;
    int* list;
    mh_u32 i;

    for (;;) {
        if (at + (count + 1) * 4 > data_size) {
            melee_host_hsd_reader_fail(
                reader, "a sound bank list runs past the data section");
            return NULL;
        }
        count++;
        if (melee_host_hsd_reader_u32(reader, at + (count - 1) * 4) ==
            AUDIO_LOAD_END)
        {
            break;
        }
    }
    list = melee_host_hsd_reader_allocate(reader, sizeof(*list) * count,
                                          alignof(int));
    if (list == NULL) {
        return NULL;
    }
    for (i = 0; i < count; i++) {
        list[i] = (int) melee_host_hsd_reader_u32(reader, at + i * 4);
    }
    return list;
}

static void* audio_load_data(MeleeHostHsdReader* reader, mh_u32 root)
{
    /* Laid out as the struct lbaudio_ax.c declares: four int** in a row. */
    int*** tables;
    int t;

    tables = melee_host_hsd_reader_allocate(
        reader, sizeof(*tables) * AUDIO_LOAD_TABLES, alignof(int**));
    if (tables == NULL) {
        return NULL;
    }
    for (t = 0; t < AUDIO_LOAD_TABLES; t++) {
        bool present;
        const mh_u32 table = target_of(reader, root + (mh_u32) t * 4, &present);
        int i;

        if (!present) {
            melee_host_hsd_reader_fail(reader, "a sound bank table is NULL");
            return NULL;
        }
        tables[t] = melee_host_hsd_reader_allocate(
            reader, sizeof(*tables[t]) * AUDIO_LOAD_LISTS, alignof(int*));
        if (tables[t] == NULL) {
            return NULL;
        }
        for (i = 0; i < AUDIO_LOAD_LISTS; i++) {
            const mh_u32 list =
                target_of(reader, table + (mh_u32) i * 4, &present);
            tables[t][i] = present ? audio_load_list(reader, list) : NULL;
        }
    }
    return melee_host_hsd_reader_failed(reader) ? NULL : tables;
}

/* MemCardIconData in LbMcGame and MemSnapIconData in LbMcSnap: the banners and
 * the icon a save file carries, as the addresses of image bytes the card layer
 * copies into the file unchanged, so the bytes stay verbatim.  The game reads
 * the entries as ints (lbcardgame.static.h, the union in lbsnap.c), which on
 * the host are pointer-wide; the card write path narrows them again, and only
 * runs with a card in the slot.  The table runs as far as its fields are
 * relocated, and the NULL word after it is kept. */
static void* card_icon_table(MeleeHostHsdReader* reader, mh_u32 root)
{
    const mh_u32 data_size = melee_host_hsd_reader_data_size(reader);
    intptr_t* table;
    mh_u32 count = 0;
    mh_u32 i;

    while (root + (count + 1) * 4 <= data_size &&
           melee_host_hsd_reader_has_pointer(reader, root + count * 4))
    {
        count++;
    }
    if (count == 0) {
        melee_host_hsd_reader_fail(reader, "the icon table is empty");
        return NULL;
    }
    table = melee_host_hsd_reader_allocate(
        reader, sizeof(*table) * (count + 1), alignof(intptr_t));
    if (table == NULL) {
        return NULL;
    }
    for (i = 0; i < count; i++) {
        bool present;
        const mh_u32 image = target_of(reader, root + i * 4, &present);
        table[i] = present ? (intptr_t) melee_host_hsd_reader_payload(
                                 reader, image, 1)
                           : 0;
    }
    table[count] = 0;
    return melee_host_hsd_reader_failed(reader) ? NULL : table;
}

/* lbRefData (LbRf.dat): two floats per refraction kind, the period and the
 * strength lbRefract_80021CE8 bends the texture behind a refracting object
 * with.  lbrefract.c declares the record privately as a count byte and a
 * pointer to the floats; this is the same layout, with the floats converted
 * from big-endian.  On disk the count is 3 and the six floats sit before the
 * record. */
struct RefractData {
    u8 count;
    f32* params;
};

static void* refract_data(MeleeHostHsdReader* reader, mh_u32 root)
{
    const mh_u32 count = melee_host_hsd_reader_u8(reader, root + 0x0);
    bool present;
    const mh_u32 params = target_of(reader, root + 0x4, &present);
    struct RefractData* data;
    mh_u32 i;

    if (!present || count == 0) {
        melee_host_hsd_reader_fail(reader, "the refraction table is empty");
        return NULL;
    }
    data = melee_host_hsd_reader_allocate(reader, sizeof(*data),
                                          alignof(struct RefractData));
    if (data == NULL) {
        return NULL;
    }
    data->params = melee_host_hsd_reader_allocate(
        reader, sizeof(f32) * count * 2, alignof(f32));
    if (data->params == NULL) {
        return NULL;
    }
    data->count = (u8) count;
    for (i = 0; i < count * 2; i++) {
        data->params[i] = melee_host_hsd_reader_f32(reader, params + i * 4);
    }
    return melee_host_hsd_reader_failed(reader) ? NULL : data;
}

/* plLoadCommonData (PdPm.dat): the thresholds the bonus and trick code
 * compares a match's stats against (plbonus.c, pltrick.c, pl_040D.c,
 * gm_16F1.c).  The symbol is a pointer to the table, which Player_80036DD8
 * dereferences into pl_804D6470.  Every field of pl_804D6470_t is a 4-byte
 * float or integer, so the table has the same offsets on the host and each
 * word is converted from big-endian in place.  The exception is xC0, which the
 * decomp types as four bytes and nothing reads: bytes keep their order.  On
 * disk the table sits at data+0 and the pointer after it. */
_Static_assert(sizeof(pl_804D6470_t) == 0x184,
               "pl_804D6470_t must keep the PowerPC offsets on the host");

static void* player_common_data(MeleeHostHsdReader* reader, mh_u32 root)
{
    const mh_u32 bytes_field = offsetof(pl_804D6470_t, xC0);
    bool present;
    const mh_u32 source = target_of(reader, root + 0x0, &present);
    pl_804D6470_t** record;
    pl_804D6470_t* table;
    mh_u32 at;
    mh_u32 i;

    if (!present) {
        melee_host_hsd_reader_fail(reader,
                                   "the player common data pointer is NULL");
        return NULL;
    }
    record = melee_host_hsd_reader_allocate(reader, sizeof(*record),
                                            alignof(pl_804D6470_t*));
    table = melee_host_hsd_reader_allocate(reader, sizeof(*table),
                                           alignof(pl_804D6470_t));
    if (record == NULL || table == NULL) {
        return NULL;
    }
    for (at = 0; at < sizeof(*table); at += 4) {
        if (at == bytes_field) {
            for (i = 0; i < sizeof(table->xC0); i++) {
                table->xC0[i] = melee_host_hsd_reader_u8(reader, source + at + i);
            }
        } else {
            const mh_u32 word = melee_host_hsd_reader_u32(reader, source + at);
            memcpy((unsigned char*) table + at, &word, sizeof(word));
        }
    }
    *record = table;
    return melee_host_hsd_reader_failed(reader) ? NULL : record;
}

/* grGroundParam (Gr*.dat): a stage's scalars, the StageParam rows
 * Ground_801C28CC looks up by StKind, and nine colors.  The rows hold no
 * pointer and keep their 0x64 bytes on the host.  GroundParam holds one
 * pointer, to the rows, so its fields keep their offsets up to it and move
 * after it; every field is read at its PowerPC offset.  On disk (GrSh.dat)
 * the 18 rows end where the parameters begin. */
_Static_assert(offsetof(GroundParam, stage_params) == 0xB0,
               "GroundParam must keep the PowerPC offsets up to its rows");
_Static_assert(sizeof(StageParam) == 0x64 && offsetof(StageParam, x1A) == 0x1A,
               "StageParam must keep the PowerPC layout on the host");

enum {
    /* No stage lists more rows than there are stage kinds. */
    GROUND_PARAM_MAX_STAGE_ROWS = 0x100,
};

static s16 read_s16(MeleeHostHsdReader* reader, mh_u32 at)
{
    return (s16) melee_host_hsd_reader_u16(reader, at);
}

static void stage_param_row(MeleeHostHsdReader* reader, mh_u32 at,
                            StageParam* row)
{
    mh_u32 i;

    row->stkind = (StKind) (s32) melee_host_hsd_reader_u32(reader, at + 0x0);
    row->x4 = (s32) melee_host_hsd_reader_u32(reader, at + 0x4);
    row->x8 = (s32) melee_host_hsd_reader_u32(reader, at + 0x8);
    row->xC = melee_host_hsd_reader_u32(reader, at + 0xC);
    row->x10 = melee_host_hsd_reader_u32(reader, at + 0x10);
    row->x14 = read_s16(reader, at + 0x14);
    row->x16 = read_s16(reader, at + 0x16);
    row->x18 = read_s16(reader, at + 0x18);
    for (i = 0; i < ARRAY_SIZE(row->x1A); i++) {
        row->x1A[i] = read_s16(reader, at + 0x1A + i * 2);
    }
}

static void* ground_param(MeleeHostHsdReader* reader, mh_u32 root)
{
    const s32 count = (s32) melee_host_hsd_reader_u32(reader, root + 0xB4);
    bool present;
    const mh_u32 rows = target_of(reader, root + 0xB0, &present);
    GroundParam* param;
    mh_u32 i;

    if (count < 0 || count > GROUND_PARAM_MAX_STAGE_ROWS ||
        (count != 0 && !present))
    {
        melee_host_hsd_reader_fail(reader,
                                   "the ground parameters list their stage "
                                   "rows wrongly");
        return NULL;
    }
    param = melee_host_hsd_reader_allocate(reader, sizeof(*param),
                                           alignof(GroundParam));
    if (param == NULL) {
        return NULL;
    }
    param->y = melee_host_hsd_reader_f32(reader, root + 0x0);
    param->x4 = read_s16(reader, root + 0x4);
    param->x6_pad[0] = melee_host_hsd_reader_u8(reader, root + 0x6);
    param->x6_pad[1] = melee_host_hsd_reader_u8(reader, root + 0x7);
    param->x8 = read_s16(reader, root + 0x8);
    param->xA = read_s16(reader, root + 0xA);
    param->xC = (s32) melee_host_hsd_reader_u32(reader, root + 0xC);
    param->x10 = (s32) melee_host_hsd_reader_u32(reader, root + 0x10);
    param->x14 = (s32) melee_host_hsd_reader_u32(reader, root + 0x14);
    param->x18 = melee_host_hsd_reader_f32(reader, root + 0x18);
    param->x1C = melee_host_hsd_reader_f32(reader, root + 0x1C);
    param->x20 = melee_host_hsd_reader_f32(reader, root + 0x20);
    param->x24 = melee_host_hsd_reader_f32(reader, root + 0x24);
    param->x28 = melee_host_hsd_reader_f32(reader, root + 0x28);
    param->x2C_pad[0] = melee_host_hsd_reader_u8(reader, root + 0x2C);
    param->x2C_pad[1] = melee_host_hsd_reader_u8(reader, root + 0x2D);
    param->x2E = read_s16(reader, root + 0x2E);
    param->x30 = (s32) melee_host_hsd_reader_u32(reader, root + 0x30);
    param->x34 = (s32) melee_host_hsd_reader_u32(reader, root + 0x34);
    param->x38 = (s32) melee_host_hsd_reader_u32(reader, root + 0x38);
    param->x3C = melee_host_hsd_reader_f32(reader, root + 0x3C);
    param->x40 = melee_host_hsd_reader_f32(reader, root + 0x40);
    param->x44 = melee_host_hsd_reader_f32(reader, root + 0x44);
    param->x48 = melee_host_hsd_reader_f32(reader, root + 0x48);
    /* A bool is one byte on both, followed by padding up to the float. */
    param->x4C_fixed_cam = melee_host_hsd_reader_u8(reader, root + 0x4C) != 0;
    param->x50 = melee_host_hsd_reader_f32(reader, root + 0x50);
    param->x54 = melee_host_hsd_reader_f32(reader, root + 0x54);
    param->x58 = melee_host_hsd_reader_f32(reader, root + 0x58);
    param->x5C = melee_host_hsd_reader_f32(reader, root + 0x5C);
    param->x60 = melee_host_hsd_reader_f32(reader, root + 0x60);
    param->x64 = melee_host_hsd_reader_f32(reader, root + 0x64);
    param->x68 = read_s16(reader, root + 0x68);
    for (i = 0; i < ARRAY_SIZE(param->x6A); i++) {
        param->x6A[i] = read_s16(reader, root + 0x6A + i * 2);
    }
    param->stage_params = NULL;
    if (present) {
        param->stage_params = melee_host_hsd_reader_allocate(
            reader, sizeof(StageParam) * (size_t) (count != 0 ? count : 1),
            alignof(StageParam));
        if (param->stage_params == NULL) {
            return NULL;
        }
        for (i = 0; i < (mh_u32) count; i++) {
            stage_param_row(reader, rows + i * 0x64, &param->stage_params[i]);
        }
    }
    param->stage_param_count = count;
    {
        GXColor* const colors[] = {
            &param->xB8, &param->xBC, &param->xC0, &param->xC4, &param->xC8,
            &param->xCC, &param->xD0, &param->xD4, &param->xD8,
        };
        for (i = 0; i < ARRAY_SIZE(colors); i++) {
            const mh_u32 at = root + 0xB8 + i * 4;
            colors[i]->r = melee_host_hsd_reader_u8(reader, at + 0);
            colors[i]->g = melee_host_hsd_reader_u8(reader, at + 1);
            colors[i]->b = melee_host_hsd_reader_u8(reader, at + 2);
            colors[i]->a = melee_host_hsd_reader_u8(reader, at + 3);
        }
    }
    return melee_host_hsd_reader_failed(reader) ? NULL : param;
}

/* The trophy tables of TyDatai.usd, which Toy_803124BC loads for the trophy
 * display.  None holds a pointer, so each keeps its PowerPC layout, and none
 * records its length: every one ends with a row whose first field is -1, which
 * the game walks to and, when a search finds nothing, reads.  The row is copied
 * whole.  The model and sort tables are also indexed by trophy up to
 * TY_TROPHY_COUNT, so they must hold that many rows before the end. */
_Static_assert(sizeof(TrophyData) == 0x24, "TrophyData keeps its layout");
_Static_assert(sizeof(TyDspEntry) == 0x10, "TyDspEntry keeps its layout");
_Static_assert(sizeof(ToyNameData) == 0xC, "ToyNameData keeps its layout");

enum {
    TROPHY_TABLE_MAX_ROWS = 0x1000,
};

/* The rows before the one whose first word, `width` bytes wide, is -1. */
static mh_u32 rows_before_end(MeleeHostHsdReader* reader, mh_u32 root,
                              mh_u32 row_size, mh_u32 width,
                              mh_u32 required)
{
    mh_u32 rows;

    for (rows = 0; rows < TROPHY_TABLE_MAX_ROWS; rows++) {
        const mh_u32 at = root + rows * row_size;
        const bool end = width == 2
            ? melee_host_hsd_reader_u16(reader, at) == 0xFFFF
            : melee_host_hsd_reader_u32(reader, at) == 0xFFFFFFFF;
        if (melee_host_hsd_reader_failed(reader)) {
            return 0;
        }
        if (end) {
            break;
        }
    }
    if (rows == TROPHY_TABLE_MAX_ROWS) {
        melee_host_hsd_reader_fail(reader, "the trophy table has no end row");
        return 0;
    }
    if (rows < required) {
        melee_host_hsd_reader_fail(reader,
                                   "the trophy table has fewer rows than "
                                   "trophies");
        return 0;
    }
    return rows;
}

static void* trophy_model_rows(MeleeHostHsdReader* reader, mh_u32 root,
                               mh_u32 required)
{
    const mh_u32 rows = rows_before_end(reader, root, 0x24, 4, required);
    TrophyData* table;
    mh_u32 i;

    if (melee_host_hsd_reader_failed(reader)) {
        return NULL;
    }
    table = melee_host_hsd_reader_allocate(reader, sizeof(*table) * (rows + 1),
                                           alignof(TrophyData));
    if (table == NULL) {
        return NULL;
    }
    for (i = 0; i <= rows; i++) {
        const mh_u32 at = root + i * 0x24;
        TrophyData* const row = &table[i];
        row->id = (s32) melee_host_hsd_reader_u32(reader, at + 0x00);
        row->x04 = (s32) melee_host_hsd_reader_u32(reader, at + 0x04);
        row->x08 = melee_host_hsd_reader_f32(reader, at + 0x08);
        row->x0C = melee_host_hsd_reader_f32(reader, at + 0x0C);
        row->x10 = melee_host_hsd_reader_f32(reader, at + 0x10);
        row->x14 = melee_host_hsd_reader_f32(reader, at + 0x14);
        row->x18 = melee_host_hsd_reader_f32(reader, at + 0x18);
        row->x1C = melee_host_hsd_reader_f32(reader, at + 0x1C);
        row->x20 = (s8) melee_host_hsd_reader_u8(reader, at + 0x20);
        row->x21 = (s8) melee_host_hsd_reader_u8(reader, at + 0x21);
        row->x22 = (s8) melee_host_hsd_reader_u8(reader, at + 0x22);
        row->x23 = (s8) melee_host_hsd_reader_u8(reader, at + 0x23);
    }
    return melee_host_hsd_reader_failed(reader) ? NULL : table;
}

/* tyInitModelTbl: one model row per trophy.  toy.c also counts through it by
 * TY_TROPHY_COUNT. */
static void* trophy_init_models(MeleeHostHsdReader* reader, mh_u32 root)
{
    return trophy_model_rows(reader, root, TY_TROPHY_COUNT);
}

/* tyInitModelDTbl: the few rows that differ in another language, only ever
 * searched to the end row. */
static void* trophy_init_models_other(MeleeHostHsdReader* reader, mh_u32 root)
{
    return trophy_model_rows(reader, root, 0);
}

/* tyModelSortTbl: six sort keys per trophy, read by trophy index. */
static void* trophy_sort_keys(MeleeHostHsdReader* reader, mh_u32 root)
{
    const mh_u32 rows = rows_before_end(reader, root, 0xC, 2, TY_TROPHY_COUNT);
    ToyNameData* table;
    mh_u32 i;

    if (melee_host_hsd_reader_failed(reader)) {
        return NULL;
    }
    table = melee_host_hsd_reader_allocate(reader, sizeof(*table) * (rows + 1),
                                           alignof(ToyNameData));
    if (table == NULL) {
        return NULL;
    }
    for (i = 0; i <= rows; i++) {
        const mh_u32 at = root + i * 0xC;
        table[i].x0 = read_s16(reader, at + 0x0);
        table[i].x2 = read_s16(reader, at + 0x2);
        table[i].x4 = read_s16(reader, at + 0x4);
        table[i].x6 = read_s16(reader, at + 0x6);
        table[i].x8 = read_s16(reader, at + 0x8);
        table[i].xA = read_s16(reader, at + 0xA);
    }
    return melee_host_hsd_reader_failed(reader) ? NULL : table;
}

/* tyExpDifferentTbl and tyNoGetUsTbl: trophy numbers ending in -1. */
static void* trophy_number_list(MeleeHostHsdReader* reader, mh_u32 root)
{
    const mh_u32 rows = rows_before_end(reader, root, 2, 2, 0);
    s16* list;
    mh_u32 i;

    if (melee_host_hsd_reader_failed(reader)) {
        return NULL;
    }
    list = melee_host_hsd_reader_allocate(reader, sizeof(*list) * (rows + 1),
                                          alignof(s16));
    if (list == NULL) {
        return NULL;
    }
    for (i = 0; i <= rows; i++) {
        list[i] = read_s16(reader, root + i * 2);
    }
    return melee_host_hsd_reader_failed(reader) ? NULL : list;
}

/* tyDisplayModelTbl and tyDisplayModelUsTbl: how each trophy stands in the
 * display, searched by trophy number to the end row. */
static void* trophy_display_rows(MeleeHostHsdReader* reader, mh_u32 root)
{
    const mh_u32 rows = rows_before_end(reader, root, 0x10, 4, 0);
    TyDspEntry* table;
    mh_u32 i;

    if (melee_host_hsd_reader_failed(reader)) {
        return NULL;
    }
    table = melee_host_hsd_reader_allocate(reader, sizeof(*table) * (rows + 1),
                                           alignof(TyDspEntry));
    if (table == NULL) {
        return NULL;
    }
    for (i = 0; i <= rows; i++) {
        const mh_u32 at = root + i * 0x10;
        table[i].x00 = (s32) melee_host_hsd_reader_u32(reader, at + 0x0);
        table[i].x04 = melee_host_hsd_reader_u8(reader, at + 0x4);
        table[i].x05 = melee_host_hsd_reader_u8(reader, at + 0x5);
        table[i].pad_06[0] = melee_host_hsd_reader_u8(reader, at + 0x6);
        table[i].pad_06[1] = melee_host_hsd_reader_u8(reader, at + 0x7);
        table[i].x08 = melee_host_hsd_reader_f32(reader, at + 0x8);
        table[i].x0C = melee_host_hsd_reader_f32(reader, at + 0xC);
    }
    return melee_host_hsd_reader_failed(reader) ? NULL : table;
}

void melee_host_game_register_data_translators(void)
{
    (void) melee_host_hsd_register_translator("tyInitModelTbl",
                                              trophy_init_models);
    (void) melee_host_hsd_register_translator("tyInitModelDTbl",
                                              trophy_init_models_other);
    (void) melee_host_hsd_register_translator("tyModelSortTbl",
                                              trophy_sort_keys);
    (void) melee_host_hsd_register_translator("tyExpDifferentTbl",
                                              trophy_number_list);
    (void) melee_host_hsd_register_translator("tyNoGetUsTbl",
                                              trophy_number_list);
    (void) melee_host_hsd_register_translator("tyDisplayModelTbl",
                                              trophy_display_rows);
    (void) melee_host_hsd_register_translator("tyDisplayModelUsTbl",
                                              trophy_display_rows);
    (void) melee_host_hsd_register_translator("grGroundParam", ground_param);
    (void) melee_host_hsd_register_translator("lbRefData", refract_data);
    (void) melee_host_hsd_register_translator("plLoadCommonData",
                                              player_common_data);
    (void) melee_host_hsd_register_translator("sqEventInitDataLevelTbl",
                                              event_level_table);
    (void) melee_host_hsd_register_translator("lbAudioLoadData",
                                              audio_load_data);
    (void) melee_host_hsd_register_translator("MemCardIconData",
                                              card_icon_table);
    (void) melee_host_hsd_register_translator("MemSnapIconData",
                                              card_icon_table);
}
