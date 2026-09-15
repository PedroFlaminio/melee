#ifndef MELEE_HOST_AX_MIXER_H
#define MELEE_HOST_AX_MIXER_H

#include <melee_host/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The host's AX: the SDK's voice API over a pool of voices, played by a
 * software mixer in frames of 5 ms, 160 stereo samples at 32 kHz.  The DSP
 * ucode is not emulated; the mixer reads the parameter block fields the SDK's
 * setters write. */
#define MELEE_HOST_AX_FRAME_SAMPLES 160U

struct _AXPB;

/* Plays `count` output samples of one voice at 32 kHz, adding them to `left`
 * and `right` (either may be NULL), and advances the voice.  Sample data is
 * read from `aram`, `aram_size` bytes: DSP ADPCM with addresses in nibbles,
 * PCM16 in samples or PCM8 in bytes.  A voice that passes its end address
 * without looping stops (state 0) once its last sample has played. */
void melee_host_ax_render_voice(struct _AXPB* pb, const mh_u8* aram,
                                mh_u32 aram_size, mh_s32* left,
                                mh_s32* right, mh_u32 count);

/* One AX frame: every playing voice into `stereo`, which holds
 * MELEE_HOST_AX_FRAME_SAMPLES left and right pairs clamped to 16 bits (NULL
 * discards them), then the callback AXRegisterCallback registered, as the
 * SDK calls it after handing the frame to the DSP. */
void melee_host_ax_run_frame(mh_s16* stereo);

/* Receives each AX frame's MELEE_HOST_AX_FRAME_SAMPLES stereo pairs. */
typedef void (*MeleeHostAxOutputSink)(const mh_s16* stereo, mh_u32 pairs,
                                      void* user_data);
void melee_host_ax_set_output_sink(MeleeHostAxOutputSink sink,
                                   void* user_data);

/* The DSP's clock: runs one AX frame, handed to the output sink, for every
 * 5 ms of `nanoseconds` accumulated.  The host advances it by one field at
 * every VI retrace, so audio follows the retraces and not the wall clock.
 * AXInit starts the count again. */
void melee_host_ax_advance_time(mh_u64 nanoseconds);

/* Whether AXAcquireVoice hands out voices.  Off by default: a voice opens the
 * game's sound effect and music paths, which are not ready on the host. */
void melee_host_ax_set_voices_enabled(bool enabled);
bool melee_host_ax_voices_enabled(void);
/* Voices acquired and not freed. */
mh_u32 melee_host_ax_acquired_voices(void);

/* The host's ARAM, which ARQ transfers fill. */
const mh_u8* melee_host_aram_bytes(void);
mh_u32 melee_host_aram_size(void);

#ifdef __cplusplus
}
#endif

#endif
