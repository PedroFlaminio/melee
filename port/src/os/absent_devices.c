/* Console peripherals the host does not have.
 *
 * The development kit's USB adapter (MCC, with FIO over it) is how a
 * development build talks to a PC.  The retail game still links that code, and
 * the scene frame loop pumps two of its queues every frame; both return at
 * once, because only the adapter's callbacks fill them.  The calls the game
 * makes to look for the adapter report it absent, in the SDK's own terms, and
 * the game already handles that answer.  Every call that only makes sense on
 * an open channel stops with its name instead, because nothing on the host
 * opens one.
 *
 * The memory card slots are empty: CARDProbe finds no card in either.
 */

#include <dolphin/types.h>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wstrict-prototypes"
#endif
#include <dolphin/card.h>
#include <dolphin/mcc.h>
#include <dolphin/os.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include <stddef.h>

#define MELEE_HOST_NO_CHANNEL(name)                                           \
    OSPanic(__FILE__, __LINE__,                                               \
            "%s needs an open USB adapter channel; the host has no adapter",  \
            name)

/* The code fn_80392CD8 in hsd_392C.c reports as "MCC is no initialize". */
enum { MELEE_HOST_MCC_NOT_INITIALIZED = 1 };

/* Probing for the adapter. */

int MCCInit(enum MCC_EXI exiChannel, u8 timeout,
            MCC_CBSysEvent callbackSysEvent)
{
    (void) exiChannel;
    (void) timeout;
    (void) callbackSysEvent;
    return 0;
}

void MCCExit(void) {}

/* No device answers, so the callback is never called. */
int MCCEnumDevices(MCC_CBEnumDevices callbackEnumDevices)
{
    (void) callbackEnumDevices;
    return 0;
}

u8 MCCGetLastError(void)
{
    return MELEE_HOST_MCC_NOT_INITIALIZED;
}

int MCCOpen(enum MCC_CHANNEL chID, u8 blockSize, MCC_CBEvent callbackEvent)
{
    (void) chID;
    (void) blockSize;
    (void) callbackEvent;
    return 0;
}

int FIOInit(enum MCC_EXI exiChannel, enum MCC_CHANNEL chID, u8 blockSize)
{
    (void) exiChannel;
    (void) chID;
    (void) blockSize;
    return 0;
}

void FIOExit(void) {}

/* Using an open channel. */

int MCCGetConnectionStatus(enum MCC_CHANNEL chID, enum MCC_CONNECT* connect)
{
    (void) chID;
    (void) connect;
    MELEE_HOST_NO_CHANNEL("MCCGetConnectionStatus");
    return 0;
}

int MCCClose(enum MCC_CHANNEL chID)
{
    (void) chID;
    MELEE_HOST_NO_CHANNEL("MCCClose");
    return 0;
}

u8 MCCGetFreeBlocks(enum MCC_MODE mode)
{
    (void) mode;
    MELEE_HOST_NO_CHANNEL("MCCGetFreeBlocks");
    return 0;
}

int MCCNotify(enum MCC_CHANNEL chID, u32 notify)
{
    (void) chID;
    (void) notify;
    MELEE_HOST_NO_CHANNEL("MCCNotify");
    return 0;
}

int MCCRead(enum MCC_CHANNEL chID, u32 offset, void* data, long size,
            enum MCC_SYNC_STATE async)
{
    (void) chID;
    (void) offset;
    (void) data;
    (void) size;
    (void) async;
    MELEE_HOST_NO_CHANNEL("MCCRead");
    return 0;
}

int MCCWrite(enum MCC_CHANNEL chID, u32 offset, void* data, long size,
             enum MCC_SYNC_STATE async)
{
    (void) chID;
    (void) offset;
    (void) data;
    (void) size;
    (void) async;
    MELEE_HOST_NO_CHANNEL("MCCWrite");
    return 0;
}

int MCCStreamOpen(enum MCC_CHANNEL chID, u8 blockSize)
{
    (void) chID;
    (void) blockSize;
    MELEE_HOST_NO_CHANNEL("MCCStreamOpen");
    return 0;
}

int FIOQuery(void)
{
    MELEE_HOST_NO_CHANNEL("FIOQuery");
    return 0;
}

int FIOFopen(const char* filename, u32 mode)
{
    (void) filename;
    (void) mode;
    MELEE_HOST_NO_CHANNEL("FIOFopen");
    return 0;
}

int FIOFclose(int handle)
{
    (void) handle;
    MELEE_HOST_NO_CHANNEL("FIOFclose");
    return 0;
}

u32 FIOFwrite(int handle, void* data, u32 size)
{
    (void) handle;
    (void) data;
    (void) size;
    MELEE_HOST_NO_CHANNEL("FIOFwrite");
    return 0;
}

/* The memory card slots. */

int CARDProbe(long chan)
{
    (void) chan;
    return FALSE;
}
