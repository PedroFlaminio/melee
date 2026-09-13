#ifndef MELEE_HOST_DOL_H
#define MELEE_HOST_DOL_H

#include <melee_host/host.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Some of what the game uses is data inside main.dol rather than a file on the
 * disc, such as the SIS font atlas.  The matching build embeds it from bytes
 * extracted out of the DOL.  The host must not ship those bytes, so it reads
 * them from the user's own extracted main.dol, by the console address the game
 * refers to them by.
 *
 * INVALID_ARGUMENT when no section covers the whole range, IO_ERROR when the
 * file cannot be read. */
MeleeHostStatus melee_host_dol_read(const char* dol_path, mh_u32 address,
                                    void* output, size_t length);

#ifdef __cplusplus
}
#endif

#endif
