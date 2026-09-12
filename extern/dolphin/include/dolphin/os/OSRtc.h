#ifndef _DOLPHIN_OSRTC_H_
#define _DOLPHIN_OSRTC_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(MELEE_HOST)
typedef u32 OSRtcUlong;
#else
typedef unsigned long OSRtcUlong;
#endif

// make the assert happy
#define OS_SOUND_MODE_MONO 0
#define OS_SOUND_MODE_STEREO 1

// make the asserts happy
#define OS_VIDEO_MODE_NTSC 0
#define OS_VIDEO_MODE_MPAL 2

struct SramControl {
    unsigned char sram[64];
    OSRtcUlong offset;
    int enabled;
    int locked;
    int sync;
#if defined(MELEE_HOST)
    void (* callback)(void);
#else
    void (* callback)();
#endif
};

typedef struct OSSram {
    unsigned short checkSum;
    unsigned short checkSumInv;
    OSRtcUlong ead0;
    OSRtcUlong ead1;
    OSRtcUlong counterBias;
    signed char displayOffsetH;
    unsigned char ntd;
    unsigned char language;
    unsigned char flags;
} OSSram;

typedef struct OSSramEx {
    unsigned char flashID[2][12];
    OSRtcUlong wirelessKeyboardID;
    unsigned short wirelessPadID[4];
    unsigned char dvdErrorCode;
    unsigned char _padding0;
    unsigned char flashIDCheckSum[2];
    unsigned char _padding1[4];
} OSSramEx;

#if defined(MELEE_HOST)
OSRtcUlong OSGetSoundMode(void);
void OSSetSoundMode(OSRtcUlong mode);
OSRtcUlong OSGetVideoMode(void);
void OSSetVideoMode(OSRtcUlong mode);
#else
unsigned long OSGetSoundMode();
void OSSetSoundMode(unsigned long mode);
unsigned long OSGetVideoMode();
void OSSetVideoMode(unsigned long mode);
#endif
unsigned char OSGetLanguage();
void OSSetLanguage(unsigned char language);
#if defined(MELEE_HOST)
OSRtcUlong OSGetProgressiveMode(void);
#else
unsigned long OSGetProgressiveMode(void);
#endif
void OSSetProgressiveMode(u32 mode);
u16 OSGetWirelessID(s32);

#ifdef __cplusplus
}
#endif

#endif // _DOLPHIN_OSRTC_H_
