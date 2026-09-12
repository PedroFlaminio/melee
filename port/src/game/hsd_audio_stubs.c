/* Host stand-ins for the parts of the HSD audio library that the rest of the
 * library links against while `synth.c` is not ported.
 *
 * Unlike `hsd_graphics_stubs.c`, these are not abort traps: they are real
 * definitions of state that `initialize.c` writes during HSD startup.  The
 * audio heap handle in particular is created by HSD_Init and only ever read
 * back by the synth, so defining it here keeps startup honest without
 * pretending audio works.
 *
 * `synth.c` now supplies its audio heap handle.  Keep this small host-only
 * bridge until debug.c can be made independent of the GameCube MSL stdio
 * implementation. */

/* The original redirects the MSL stdio write hook through HSD's report
 * callback.  The host has no MSL stdio: OSReport already writes to the host's
 * error stream, so there is nothing to redirect and nothing is lost. */
void HSD_LogInit(void)
{
}
