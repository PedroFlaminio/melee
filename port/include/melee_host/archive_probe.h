#ifndef MELEE_HOST_ARCHIVE_PROBE_H
#define MELEE_HOST_ARCHIVE_PROBE_H

#include <melee_host/host.h>
#include <melee_host/hsd_archive.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MeleeHostArchiveProbeSymbol {
    const char* symbol;
    MeleeHostHsdSymbolKind kind;
    /* HSD_ArchiveGetPublicAddress returned a descriptor. */
    mh_u8 translated;
    /* The kind has an original loader that runs on its own, and it ran:
     * joints, cameras, light tables and fog.  Animation trees need the tree
     * they drive, and sprites need the sprite library, so those stop at
     * translation. */
    mh_u8 load_attempted;
    mh_u8 loaded;
    /* Objects that loader built: JObjs for a joint, LObjs for a light table. */
    mh_u32 objects;
    /* Why translation or loading failed.  Valid during the callback only. */
    const char* error;
} MeleeHostArchiveProbeSymbol;

typedef void (*MeleeHostArchiveProbeCallback)(
    const MeleeHostArchiveProbeSymbol* result, void* user_data);

/* Loads a file the way lbArchive_LoadSymbols does, minus the game's heaps and
 * DVD layer: a 32-byte aligned buffer, one HSD_ArchiveParse, the extern loop
 * of lbArchive_InitializeDAT, then HSD_ArchiveGetPublicAddress for each name
 * in `symbols`, in order.  A count of zero asks for every public symbol in the
 * archive's own order.  Each descriptor is handed to the original loader for
 * its kind and everything that loader built is freed again.  The callback
 * runs once per symbol. */
MeleeHostStatus
melee_host_archive_probe_file(const char* path, const char* const* symbols,
                              mh_u32 symbol_count,
                              MeleeHostArchiveProbeCallback callback,
                              void* user_data);

const char* melee_host_archive_probe_last_error(void);

#ifdef __cplusplus
}
#endif

#endif
