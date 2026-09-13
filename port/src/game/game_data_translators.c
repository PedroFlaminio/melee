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

#include <stdalign.h>
#include <stddef.h>

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

void melee_host_game_register_data_translators(void)
{
    (void) melee_host_hsd_register_translator("sqEventInitDataLevelTbl",
                                              event_level_table);
    (void) melee_host_hsd_register_translator("lbAudioLoadData",
                                              audio_load_data);
    (void) melee_host_hsd_register_translator("MemCardIconData",
                                              card_icon_table);
    (void) melee_host_hsd_register_translator("MemSnapIconData",
                                              card_icon_table);
}
