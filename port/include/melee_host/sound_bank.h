#ifndef MELEE_HOST_SOUND_BANK_H
#define MELEE_HOST_SOUND_BANK_H

#include <melee_host/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A .ssm sound bank as synth.c loads it: four big-endian words (header size,
 * sample data size, sound count, first sound id), then each sound's voice
 * count and sample rate and one 0x40-byte block per voice (AXPBADDR,
 * AXPBADPCM, AXPBADPCMLOOP), and the sample data from the header size plus
 * 0x10, rounded up to 32 bytes.  Voice addresses count from the start of the
 * sample data. */
typedef struct MeleeHostSoundBankVoice {
    mh_u32 sound_id;
    /* The voice's place among its sound's voices. */
    mh_u32 voice;
    mh_u32 sample_rate;
    bool loops;
} MeleeHostSoundBankVoice;

/* The voices of every sound in the bank, or 0 when its tables do not fit. */
mh_u32 melee_host_sound_bank_voice_count(const mh_u8* bank, mh_u32 size);

/* Plays voice `index` once through the host's AX mixer, at its own sample
 * rate, full volume and without its loop, into `out`.  Returns the samples
 * written, at most `capacity`, or -1 for a voice the bank does not hold. */
mh_s32 melee_host_sound_bank_decode_voice(const mh_u8* bank, mh_u32 size,
                                          mh_u32 index,
                                          MeleeHostSoundBankVoice* info,
                                          mh_s16* out, mh_u32 capacity);

#ifdef __cplusplus
}
#endif

#endif
