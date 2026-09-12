/* Host stand-ins for the parts of the HSD audio library that the rest of the
 * library links against while `synth.c` is not ported.
 *
 * Unlike `hsd_graphics_stubs.c`, these are not abort traps: they are real
 * definitions of state that `initialize.c` writes during HSD startup.  The
 * audio heap handle in particular is created by HSD_Init and only ever read
 * back by the synth, so defining it here keeps startup honest without
 * pretending audio works.
 *
 * Delete this file when `synth.c` and `debug.c` enter the host build; both
 * symbols below belong to them. */

#include <dolphin/os/OSAlloc.h>

/* Written by HSD_Init when it carves the audio heap out of the arena.  It
 * stays -1 until then, which is what OSAllocFromHeap treats as no heap. */
OSHeapHandle HSD_Synth_804D6018 = -1;

/* The original redirects the MSL stdio write hook through HSD's report
 * callback.  The host has no MSL stdio: OSReport already writes to the host's
 * error stream, so there is nothing to redirect and nothing is lost. */
void HSD_LogInit(void)
{
}
