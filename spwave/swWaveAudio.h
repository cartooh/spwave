/*
 *	swWaveAudio.h
 */

#ifndef __SWWAVEAUDIO_H
#define __SWWAVEAUDIO_H

#include <sp/spAudio.h>

#include "swWave.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(MACOS)
#pragma import on
#endif

#define SW_NUM_PLAY_POSITION_INFO 4
    
typedef struct _swAudio *swAudio;
typedef struct _swPlayPositionInfo swPlayPositionInfo;

struct _swPlayPositionInfo {
    spLong total_write;
    spLong offset;
    spLong end_pos;
};

struct _swAudio {
    spAudio audio;
    char audio_driver[SP_MAX_PATHNAME];
    
    spLong buf_size;
    char *buf;
    swWave wave;

    spBool in_thread;
    spBool position_callback_used;
    
    swPlayPositionInfo play_pos_info[SW_NUM_PLAY_POSITION_INFO];
    spLong suspend_release_pos;
};

extern spBool swSetPlayRegion(swWave wave, spLong offset, spLong length);
extern spBool swSetPlayStartOffset(swWave wave, spLong start_offset, spBool suspend_callback, spBool from_current_pos);
extern spBool swPlayWave(swWave wave, spLong offset, spLong length, spLong start_offset, spBool loop_flag);

extern spBool swIsWavePlayable(swWave wave);
extern spBool swInitAudio(swWaveConfig config);
extern swAudio swBeginAudio(swWave wave, spBool in_thread);
extern spBool swEndAudio(swAudio audio);

#if defined(MACOS)
#pragma import off
#endif

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration */
#endif

#endif /* __SWWAVEAUDIO_H */
