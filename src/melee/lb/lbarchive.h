#ifndef _lbarchive_h_
#define _lbarchive_h_

#include <Runtime/platform.h>

#include <sysdolphin/baselib/forward.h>

#include <sysdolphin/baselib/archive.h>

void lbArchive_InitializeDAT(HSD_Archive* archive, void* data, size_t length);
void lbArchive_LoadSections(HSD_Archive* archive, void** symbols, ...);
HSD_Archive* lbArchive_LoadArchive(const char* filename);
HSD_Archive* lbArchive_LoadSymbols(const char* filename, void* symbols, ...);
HSD_Archive* lbArchive_80016DBC(const char* filename, void* symbols, ...);
void lbArchive_80016EFC(HSD_Archive*);
bool lbArchive_80016F80(HSD_Archive**, const char* filename);
bool lbArchive_80017040(HSD_Archive** dst, const char* filename, void* symbols,
                        ...);
bool lbArchive_800171CC(HSD_Archive** dst, const char* filename, void* symbols,
                        ...);
int lbArchiveRelocate(HSD_Archive*, u8*, size_t file_size, intptr_t base_addr);

#ifdef MELEE_HOST
#include <stddef.h>

/* The loaders that take a symbol list, with the list as an array of pointers
 * that ends, as on the console, at the first NULL symbol. */
void lbArchive_HostLoadSections(HSD_Archive* archive,
                                const void* const* symbols);
HSD_Archive* lbArchive_HostLoadSymbols(const char* filename,
                                       const void* const* symbols);
HSD_Archive* lbArchive_Host80016DBC(const char* filename,
                                    const void* const* symbols);
bool lbArchive_Host80017040(HSD_Archive** dst, const char* filename,
                            const void* const* symbols);
bool lbArchive_Host800171CC(HSD_Archive** dst, const char* filename,
                            const void* const* symbols);

#ifndef LB_ARCHIVE_IMPLEMENTATION
/* The game ends a symbol list with a literal 0, an int.  On x86-64 an int
 * passed after the sixth argument leaves whatever the stack held in the upper
 * half of its 64-bit slot, which va_arg reads as a pointer, so the list may
 * never end.  In an array of pointers the 0 is a null pointer; the NULL after
 * the list closes the array. */
#define lbArchive_LoadSections(archive, ...)                                  \
    lbArchive_HostLoadSections((archive),                                     \
                               (const void* const[]) { __VA_ARGS__, NULL })
#define lbArchive_LoadSymbols(filename, ...)                                  \
    lbArchive_HostLoadSymbols((filename),                                     \
                              (const void* const[]) { __VA_ARGS__, NULL })
#define lbArchive_80016DBC(filename, ...)                                     \
    lbArchive_Host80016DBC((filename),                                        \
                           (const void* const[]) { __VA_ARGS__, NULL })
#define lbArchive_80017040(dst, filename, ...)                                \
    lbArchive_Host80017040((dst), (filename),                                 \
                           (const void* const[]) { __VA_ARGS__, NULL })
#define lbArchive_800171CC(dst, filename, ...)                                \
    lbArchive_Host800171CC((dst), (filename),                                 \
                           (const void* const[]) { __VA_ARGS__, NULL })
#endif
#endif

#endif
