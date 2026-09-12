#ifndef MELEE_HOST_HSD_GRAPHICS_TYPES_HPP
#define MELEE_HOST_HSD_GRAPHICS_TYPES_HPP

/* The original sysdolphin headers are C without extern "C" guards, and they
 * use constructs the port's strict warning set rejects.  They have to keep
 * matching the PowerPC layout, so host C++ relaxes the diagnostics around the
 * include instead of editing them.  port/tests/hsd_include.hpp carries the
 * same pattern for test translation units.
 */

#if defined(__clang__)
#define MELEE_HOST_HSD_BEGIN                                                  \
    _Pragma("clang diagnostic push")                                          \
    _Pragma("clang diagnostic ignored \"-Wpedantic\"") extern "C" {
#define MELEE_HOST_HSD_END                                                    \
    }                                                                         \
    _Pragma("clang diagnostic pop")
#elif defined(__GNUC__)
#define MELEE_HOST_HSD_BEGIN                                                  \
    _Pragma("GCC diagnostic push")                                            \
    _Pragma("GCC diagnostic ignored \"-Wpedantic\"") extern "C" {
#define MELEE_HOST_HSD_END                                                    \
    }                                                                         \
    _Pragma("GCC diagnostic pop")
#else
#define MELEE_HOST_HSD_BEGIN extern "C" {
#define MELEE_HOST_HSD_END }
#endif

MELEE_HOST_HSD_BEGIN
#include <dolphin/gx/GXEnum.h>
#include <dolphin/gx/GXStruct.h>
#include <dolphin/mtx.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/objalloc.h>
#include <sysdolphin/baselib/pobj.h>
#include <sysdolphin/baselib/robj.h>
#include <sysdolphin/baselib/tobj.h>
#include <sysdolphin/baselib/wobj.h>
MELEE_HOST_HSD_END

#endif
