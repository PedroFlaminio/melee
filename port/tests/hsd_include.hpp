#ifndef MELEE_HOST_TEST_HSD_INCLUDE_HPP
#define MELEE_HOST_TEST_HSD_INCLUDE_HPP

/* Original sysdolphin headers use constructs the strict host warning set
 * rejects, such as the anonymous struct inside HSD_GObj_DelayedProcInfo.  The
 * decompiled headers must keep matching the PowerPC layout, so tests relax the
 * diagnostics around the include instead of editing them.
 *
 * Usage:
 *     #include "hsd_include.hpp"
 *     MELEE_HOST_TEST_HSD_BEGIN
 *     #include <sysdolphin/baselib/gobj.h>
 *     MELEE_HOST_TEST_HSD_END
 */

#if defined(__clang__)
#define MELEE_HOST_TEST_HSD_BEGIN                                             \
    _Pragma("clang diagnostic push")                                          \
    _Pragma("clang diagnostic ignored \"-Wpedantic\"") extern "C" {
#define MELEE_HOST_TEST_HSD_END                                               \
    }                                                                         \
    _Pragma("clang diagnostic pop")
#elif defined(__GNUC__)
#define MELEE_HOST_TEST_HSD_BEGIN                                             \
    _Pragma("GCC diagnostic push")                                            \
    _Pragma("GCC diagnostic ignored \"-Wpedantic\"") extern "C" {
#define MELEE_HOST_TEST_HSD_END                                               \
    }                                                                         \
    _Pragma("GCC diagnostic pop")
#else
#define MELEE_HOST_TEST_HSD_BEGIN extern "C" {
#define MELEE_HOST_TEST_HSD_END }
#endif

#endif
