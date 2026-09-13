#ifndef MELEE_HOST_HSD_ARCHIVE_H
#define MELEE_HOST_HSD_ARCHIVE_H

#include <melee_host/host.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The host implements sysdolphin's archive API (HSD_ArchiveParse,
 * HSD_ArchiveGetPublicAddress, HSD_ArchiveGetExtern and
 * HSD_ArchiveLocateExtern) in place of archive.c.
 *
 * The original relocates the file in place, turning every 32-bit offset into
 * a pointer, which a 64-bit host cannot do: each pointer needs eight bytes
 * where the file has four.  Here the parse keeps the file's buffer as the
 * archive's identity, and a public symbol is rebuilt in host layout when it is
 * first asked for.  The symbol's suffix says what it is, which is the same
 * naming the game's own lookups rely on.
 *
 * The descriptors live until the same buffer is parsed again, which is when
 * the console would have overwritten the bytes they came from, or until they
 * are released explicitly. */

typedef enum MeleeHostHsdSymbolKind {
    MELEE_HOST_HSD_SYMBOL_UNSUPPORTED = 0,
    MELEE_HOST_HSD_SYMBOL_JOINT,
    MELEE_HOST_HSD_SYMBOL_ANIM_JOINT,
    MELEE_HOST_HSD_SYMBOL_MAT_ANIM_JOINT,
    MELEE_HOST_HSD_SYMBOL_SHAPE_ANIM_JOINT,
    MELEE_HOST_HSD_SYMBOL_CAMERA,
    MELEE_HOST_HSD_SYMBOL_SCENE_LIGHTS,
    MELEE_HOST_HSD_SYMBOL_FOG,
    MELEE_HOST_HSD_SYMBOL_SOBJ_DESC,
    MELEE_HOST_HSD_SYMBOL_FIGATREE,
    /* A symbol whose name carries no kind, recognised by its whole name. */
    MELEE_HOST_HSD_SYMBOL_RUMBLE_TABLE,
    MELEE_HOST_HSD_SYMBOL_KIND_COUNT
} MeleeHostHsdSymbolKind;

typedef struct MeleeHostHsdArchiveStats {
    mh_u32 archives_live;
    /* Public symbols rebuilt in host layout, and those refused: an unknown
     * kind, or a record the materializer would not guess at. */
    mh_u32 symbols_translated;
    mh_u32 symbols_refused;
    /* Pointer fields an extern resolved to NULL. */
    mh_u32 extern_fields_nulled;
    /* Externs the game asked to bind to an address, which the host cannot
     * do yet. */
    mh_u32 externs_refused;
    mh_u64 descriptor_bytes;
    mh_u64 payload_bytes;
} MeleeHostHsdArchiveStats;

/* The kind a public symbol's name selects, longest suffix first, so
 * `_matanim_joint` is not taken for `_joint`. */
MeleeHostHsdSymbolKind melee_host_hsd_symbol_kind(const char* symbol);
const char* melee_host_hsd_symbol_kind_name(MeleeHostHsdSymbolKind kind);

MeleeHostStatus
melee_host_hsd_archive_stats(MeleeHostHsdArchiveStats* out_stats);

/* Drops what was parsed from `bytes`.  Objects the original loaders built
 * still point into those descriptors, so this is only safe once they are
 * gone. */
MeleeHostStatus melee_host_hsd_archive_release(const void* bytes);
void melee_host_hsd_archive_release_all(void);

/* Why the last parse or lookup failed, including a refused symbol. */
const char* melee_host_hsd_archive_last_error(void);

#ifdef __cplusplus
}
#endif

#endif
