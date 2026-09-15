#include <melee_host/sound_bank.h>

#include <melee_host/ax_mixer.h>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wstrict-prototypes"
#endif
#include <dolphin/ax.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include <string.h>

static mh_u32 sound_bank_be32(const mh_u8* p)
{
    return ((mh_u32) p[0] << 24) | ((mh_u32) p[1] << 16) |
           ((mh_u32) p[2] << 8) | (mh_u32) p[3];
}

static u16 sound_bank_be16(const mh_u8* p)
{
    return (u16) (((mh_u32) p[0] << 8) | (mh_u32) p[1]);
}

/* The block of voice `index`, or NULL past the last voice or when a table
 * does not fit.  `voices` receives how many voices the walk passed. */
static const mh_u8* sound_bank_block(const mh_u8* bank, mh_u32 size,
                                     mh_u32 index,
                                     MeleeHostSoundBankVoice* info,
                                     mh_u32* voices)
{
    mh_u32 records_end;
    mh_u32 count;
    mh_u32 offset = 0x10;
    mh_u32 seen = 0;
    mh_u32 sound;

    *voices = 0;
    if (size < 0x10) {
        return NULL;
    }
    /* The records start at 0x10 and take the header size in bytes. */
    records_end = sound_bank_be32(bank);
    count = sound_bank_be32(bank + 8);
    if (records_end > size - 0x10) {
        return NULL;
    }
    records_end += 0x10;
    for (sound = 0; sound < count; sound++) {
        mh_u32 voice_count;
        mh_u32 voice;

        if (offset > records_end || records_end - offset < 8) {
            return NULL;
        }
        voice_count = sound_bank_be32(bank + offset);
        if (voice_count > (records_end - offset - 8) / 0x40) {
            return NULL;
        }
        for (voice = 0; voice < voice_count; voice++, seen++) {
            if (seen == index) {
                info->sound_id = sound_bank_be32(bank + 12) + sound;
                info->voice = voice;
                info->sample_rate = sound_bank_be32(bank + offset + 4);
                return bank + offset + 8 + voice * 0x40;
            }
        }
        offset += 8 + voice_count * 0x40;
        *voices = seen;
    }
    return NULL;
}

mh_u32 melee_host_sound_bank_voice_count(const mh_u8* bank, mh_u32 size)
{
    MeleeHostSoundBankVoice info;
    mh_u32 voices;

    if (sound_bank_block(bank, size, 0xFFFFFFFFU, &info, &voices) != NULL) {
        return 0;
    }
    return voices;
}

mh_s32 melee_host_sound_bank_decode_voice(const mh_u8* bank, mh_u32 size,
                                          mh_u32 index,
                                          MeleeHostSoundBankVoice* info,
                                          mh_s16* out, mh_u32 capacity)
{
    const mh_u8* block;
    mh_u32 voices;
    mh_u32 data_start;
    mh_u32 data_size;
    mh_u32 written = 0;
    mh_u32 i;
    AXPB pb;

    block = sound_bank_block(bank, size, index, info, &voices);
    if (block == NULL) {
        return -1;
    }
    data_start = (sound_bank_be32(bank) + 0x10U + 31U) & ~31U;
    data_size = sound_bank_be32(bank + 4);
    if (data_start > size || data_size > size - data_start) {
        return -1;
    }
    memset(&pb, 0, sizeof(pb));
    info->loops = sound_bank_be16(block) != 0;
    pb.addr.loopFlag = 0;
    pb.addr.format = sound_bank_be16(block + 2);
    pb.addr.loopAddressHi = sound_bank_be16(block + 4);
    pb.addr.loopAddressLo = sound_bank_be16(block + 6);
    pb.addr.endAddressHi = sound_bank_be16(block + 8);
    pb.addr.endAddressLo = sound_bank_be16(block + 10);
    pb.addr.currentAddressHi = sound_bank_be16(block + 12);
    pb.addr.currentAddressLo = sound_bank_be16(block + 14);
    for (i = 0; i < 16; i++) {
        pb.adpcm.a[i / 2][i % 2] = sound_bank_be16(block + 16 + i * 2);
    }
    pb.adpcm.gain = sound_bank_be16(block + 48);
    pb.adpcm.pred_scale = sound_bank_be16(block + 50);
    pb.adpcm.yn1 = sound_bank_be16(block + 52);
    pb.adpcm.yn2 = sound_bank_be16(block + 54);
    pb.adpcmLoop.loop_pred_scale = sound_bank_be16(block + 56);
    pb.adpcmLoop.loop_yn1 = sound_bank_be16(block + 58);
    pb.adpcmLoop.loop_yn2 = sound_bank_be16(block + 60);
    pb.state = 1;
    pb.src.ratioHi = 1;
    pb.ve.currentVolume = 0x8000;
    pb.mix.vL = 0x8000;
    /* One sample at a time: a voice plays a sample whenever it is playing
     * when asked, unless its first sample is already past its end. */
    while (pb.state == 1 && written < capacity) {
        mh_s32 sample = 0;

        melee_host_ax_render_voice(&pb, bank + data_start, data_size, &sample,
                                   NULL, 1);
        if (pb.src.last_samples[0] == 0) {
            break;
        }
        out[written++] = (mh_s16) sample;
    }
    return (mh_s32) written;
}
