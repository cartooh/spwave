/*
 *	swWaveAudio.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sp/spDefs.h>
#include <sp/spBase.h>
#include <sp/spMemory.h>
#include <sp/spFile.h>
#include <sp/spThread.h>
#include <sp/spOption.h>
#include <sp/spWave.h>
#include <sp/spAudio.h>

#include "swWaveAudio.h"

void swInitPlayPositionInfo(swAudio audio, spLong offset, spLong end_pos)
{
    int i;

    for (i = 0; i < SW_NUM_PLAY_POSITION_INFO - 1; i++) {
	memset(&audio->play_pos_info[i], 0, sizeof(struct _swPlayPositionInfo));
    }
    audio->play_pos_info[i].total_write = 0;
    audio->play_pos_info[i].offset = offset;
    audio->play_pos_info[i].end_pos = end_pos;
    
    return;
}

void swShiftPlayPositionInfo(swAudio audio)
{
    int i;

    for (i = 0; i < SW_NUM_PLAY_POSITION_INFO - 1; i++) {
	memcpy(&audio->play_pos_info[i], &audio->play_pos_info[i + 1], sizeof(struct _swPlayPositionInfo));
    }

    return;
}

void swSetNewPlayPositionInfo(swAudio audio, spLong nwrite, spLong offset, spLong end_pos)
{
    int i;
    
    swShiftPlayPositionInfo(audio);

    i = SW_NUM_PLAY_POSITION_INFO - 1;
    audio->play_pos_info[i].total_write = audio->play_pos_info[i - 1].total_write + nwrite;
    audio->play_pos_info[i].offset = offset;
    audio->play_pos_info[i].end_pos = end_pos;

    return;
}

void swUpdatePlayPositionInfo(swAudio audio, spLong offset, spLong end_pos)
{
    int i;
    
    i = SW_NUM_PLAY_POSITION_INFO - 1;
    audio->play_pos_info[i].offset = offset;
    audio->play_pos_info[i].end_pos = end_pos;

    return;
}

spLong swGetFilePlayPosition(swAudio audio, spLong driver_pos)
{
    int i;
    spLong delta;
    spLong file_pos = 0;

    for (i = SW_NUM_PLAY_POSITION_INFO - 1; i >= 0; i--) {
	spDebug(100, "swGetFilePlayPosition",
		"i = %d, total_write = %ld\n", i, (long)audio->play_pos_info[i].total_write);
	
	if (audio->play_pos_info[i].total_write <= driver_pos) {
	    delta = driver_pos - audio->play_pos_info[i].total_write;
	    break;
	}
    }
    if (i < 0) {
	spDebug(10, "swGetFilePlayPosition", "******** playing position %ld is too old\n", (long)driver_pos);
	i = 0;
	delta = 0;
    }
    
    file_pos = audio->play_pos_info[i].offset + delta;
    if (file_pos > audio->play_pos_info[i].end_pos) {
	file_pos = audio->play_pos_info[i].end_pos;
    }
    spDebug(80, "swGetFilePlayPosition", "delta = %ld, driver_pos = %ld, file_pos = %ld\n",
	    (long)delta, (long)driver_pos, (long)file_pos);

    return file_pos;
}

static spBool playWaveAudio(swWave wave, spBool loop_flag, spBool in_thread)
{
    spBool flag = SP_FALSE;
    spBool started = SP_FALSE;
    spBool break_flag;
    spBool repeat_flag;
    int num_channel;
    int selected_channel;
    spLong k;
    spLong max_loop;
    spLong pos;
    spLong driver_pos;
    spLong ref_pos;
    spLong offset, length;
    spLong play_start_offset;
    spLong buf_offset;
    spLong nread, nwrite;
    spLong play_length;
    spLong audio_length, min_audio_length;
    spLong wlength;
    spLong total_write;
    spPlugin *plugin;
    swAudio audio;

    if (swIsWavePlayable(wave) == SP_FALSE
	|| (audio = swBeginAudio(wave, in_thread)) == NULL) {
	spDebug(80, "playWaveAudio", "initialize error\n");
	return SP_FALSE;
    }

    swLockMutex(wave);
    wave->core->process_flag = SP_TRUE;
    wave->core->process_finished = SP_FALSE;
    wave->core->play_flag = SP_TRUE;
    
    selected_channel = wave->selected_channel;
    if (selected_channel >= 0) {
	num_channel = 1;
    } else {
	num_channel = wave->num_channel;
    }
    swUnlockMutex(wave);
    
    spDebug(10, "playWaveAudio", "play_read_length = %ld, samp_rate = %f\n",
	    wave->config->play_read_length, wave->samp_rate);
    
    min_audio_length = swConvertByteToLength(wave, audio->buf_size) / 2;
    wlength = (spLong)POW2(spNextPow2((long)((double)wave->config->play_read_length
					     * (wave->samp_rate / 48000.0))));
    spDebug(10, "playWaveAudio", "wlength = %ld\n", (long)wlength);
    wlength = MAX(wlength, 256);
    wlength = MIN(wlength, min_audio_length);
    
    /* open audio device */
    if (spOpenAudioDevice(audio->audio, "wo") == SP_FALSE) {
	spDebug(10, "playWaveAudio", "Can't open audio device.\n");
    } else {
	/* open wave */
	if ((plugin = swOpenWave(wave, "r")) != NULL) {
	    flag = SP_TRUE;
	    started = SP_TRUE;
	    break_flag = SP_FALSE;

	    swLockMutex(wave);
	    offset = wave->play_offset;
	    length = wave->play_length;
	    play_start_offset = wave->play_start_offset;
	    wave->play_start_offset = 0;
	    wave->play_start_offset_updated = SP_FALSE;

	    swInitPlayPositionInfo(audio, offset + play_start_offset, offset + length);
	    audio->suspend_release_pos = -1;
	    swUnlockMutex(wave);
	    
	    spDebug(50, "playWaveAudio",
		    "offset = %ld, length = %ld, play_start_offset = %ld, position_callback_used = %d\n",
		    (long)offset, (long)length, (long)play_start_offset, audio->position_callback_used);
	    
	    if (wave->config->play_func != NULL) {
		flag = wave->config->play_func(wave, in_thread, SW_PROCESS_STARTED, wave->core->call_data);
		if (flag == SP_TRUE) {
		    flag = wave->config->play_func(wave, in_thread, offset, wave->core->call_data);
		}
	    }

	    audio_length = 0;
	    play_length = 0;
	    total_write = 0;

	    max_loop = 2147483647L / wlength;
	    spDebug(50, "playWaveAudio", "max_loop = %ld\n", (long)max_loop);

	    for (k = 0; k < max_loop && flag == SP_TRUE; k++) {
		spDebug(80, "playWaveAudio", "offset = %ld, length = %ld, play_start_offset = %ld\n",
			(long)offset, (long)length, (long)play_start_offset);
		
		swLockMutex(wave);
		swSetNewPlayPositionInfo(audio, play_length,
					 offset + play_start_offset, offset + length);
		swUnlockMutex(wave);
		
		spSeekPlugin(plugin, offset + play_start_offset);
		    
		spDebug(80, "playWaveAudio", "wlength = %ld, min_audio_length = %ld, audio_length = %ld, buf_size = %ld\n",
			(long)wlength, (long)min_audio_length, (long)audio_length, (long)audio->buf_size);

		repeat_flag = SP_FALSE;
		nread = 0;
		play_length = 0;
		while (flag == SP_TRUE) {
		    if (selected_channel >= 0) {
			buf_offset = swConvertLengthToByte(wave, audio_length) / wave->num_channel;
		    } else {
			buf_offset = swConvertLengthToByte(wave, audio_length);
		    }
		    if ((nread = swLoadIntoBuffer(plugin, wave, selected_channel,
						  wlength, audio->buf + buf_offset)) <= 0) {
			spDebug(10, "playWaveAudio", "nread failed: nread = %ld\n", (long)nread);
			break;
		    }
		    if (play_length + nread >= length - play_start_offset) {
			nread = length - play_start_offset - play_length;
			spDebug(50, "playWaveAudio", "read final segment: nread = %ld\n", (long)nread);
		    }
		    audio_length += nread;
		    play_length += nread;

		    if (audio_length >= min_audio_length) {
			spDebug(80, "playWaveAudio", "audio_length = %ld, min_audio_length = %ld\n",
				(long)audio_length, (long)min_audio_length);
			
			/* write audio data */
			nwrite = spWriteAudio(audio->audio, audio->buf, (long)(audio_length * num_channel)) / num_channel;
			if (nwrite <= 0) {
			    spDebug(10, "playWaveAudio", "write failed: nwrite = %ld\n", (long)nwrite);
			    flag = SP_FALSE;
			    break;
			}
			total_write += nwrite;
			
			audio_length = 0;
			
			if (spGetAudioOutputPosition(audio->audio, &driver_pos) == SP_TRUE) {
			    pos = swGetFilePlayPosition(audio, driver_pos);
			} else {
			    pos = offset + play_start_offset + nwrite;
			    driver_pos = total_write;
			}
			pos = MIN(offset + length, pos);

			spDebug(80, "playWaveAudio", "driver_pos = %ld, pos = %ld, suspend_release_pos = %ld\n",
				(long)driver_pos, (long)pos, (long)audio->suspend_release_pos);
			
			if (audio->position_callback_used == SP_FALSE
			    && wave->config->play_func != NULL && flag == SP_TRUE
			    && driver_pos > audio->suspend_release_pos) {
			    flag = wave->config->play_func(wave, in_thread, pos, wave->core->call_data);
			    audio->suspend_release_pos = -1;
			}
			
			if (wave->core->process_flag == SP_FALSE) {
			    spStopAudio(audio->audio);
			    loop_flag = SP_FALSE;
			    break;
			}
			
			break_flag = SP_FALSE;
			
			swLockMutex(wave);
			if (wave->play_offset != offset || wave->play_length != length
			    || wave->play_start_offset_updated == SP_TRUE) {
			    repeat_flag = SP_TRUE;
			    
			    if (wave->play_offset != offset
				|| wave->play_offset > offset + length
				|| pos < wave->play_offset
				|| wave->play_offset + wave->play_length < pos) {
				break_flag = SP_TRUE;
				offset = wave->play_offset;
				length = wave->play_length;
			    } else if (wave->play_start_offset_updated == SP_TRUE) {
				break_flag = SP_TRUE;
			    } else {
				if (offset + length != wave->play_offset + wave->play_length) {
				    length = wave->play_offset + wave->play_length - offset;
				}
				swUpdatePlayPositionInfo(audio, offset + play_start_offset, offset + length);
				repeat_flag = SP_FALSE;
			    }

			    if (break_flag == SP_TRUE) {
				play_start_offset = 0;
			    }
			    
			    if (wave->play_start_offset_updated == SP_TRUE) {
				repeat_flag = SP_TRUE;
				spDebug(80, "playWaveAudio",
					"pos = %ld, play_length = %ld, play_start_offset = %ld, offset = %ld, length = %ld\n",
					(long)pos, (long)play_length, (long)wave->play_start_offset, (long)offset, (long)length);

				if (wave->play_start_sync_pos == SP_TRUE) {
				    ref_pos = pos;
				} else {
				    ref_pos = offset;
				}
				if ((loop_flag == SP_TRUE
				     && ref_pos + wave->play_start_offset < offset + length)
				    || ((ref_pos + wave->play_start_offset >= offset)
					&& (ref_pos + wave->play_start_offset < offset + length))
				    || (offset <= 0 && length >= wave->total_length)) {
				    play_start_offset = ref_pos - offset + wave->play_start_offset;
				    play_start_offset = MAX(play_start_offset, -offset);
				    play_start_offset = MIN(play_start_offset, length - 1);
				    
				    spDebug(80, "playWaveAudio", "updated play_start_offset = %ld, offset = %ld\n", (long)play_start_offset, (long)offset);
				} else {
				    offset = 0;
				    length = wave->total_length;
				    wave->play_offset = offset;
				    wave->play_length = length;
				    
				    play_start_offset = MAX(ref_pos - offset + wave->play_start_offset, -offset);
				    play_start_offset = MIN(play_start_offset, length - 1);
				    spDebug(80, "playWaveAudio", "updated play_start_offset = %ld (region cleared)\n",
					    (long)play_start_offset);
				}
				if (wave->suspend_play_callback == SP_TRUE) {
				    audio->suspend_release_pos = total_write;
				    wave->suspend_play_callback = SP_FALSE;
				}
			    }
			    wave->play_start_offset = 0;
			    wave->play_start_offset_updated = SP_FALSE;
			}
			swUnlockMutex(wave);
			
			if (break_flag == SP_TRUE) {
			    break;
			}
		    }

		    if (play_length >= length - play_start_offset) {
			if (loop_flag == SP_FALSE && audio_length > 0) {
			    spWriteAudio(audio->audio, audio->buf, (long)(audio_length * num_channel));
			}
			break;
		    }
		}
		spDebug(80, "playWaveAudio", "play_length = %ld, nread = %ld\n", (long)play_length, (long)nread);
		    
		if (nread < 0) {
		    spDebug(80, "playWaveAudio", "**** nread = %ld < 0 ****\n", (long)nread);
		    flag = SP_FALSE;
		}
		if (loop_flag == SP_FALSE && repeat_flag == SP_FALSE) {
		    break;
		}

		if (/*repeat_flag*/break_flag == SP_FALSE) {
		    play_start_offset = 0;
		}
	    }

	    /* close file */
	    spCloseFilePlugin(plugin);
	}
	
	spCloseAudioDevice(audio->audio);
	
	if (loop_flag == SP_FALSE && flag == SP_TRUE
	    && wave->core->process_flag == SP_TRUE
	    && wave->config->play_func != NULL) {
	    spDebug(10, "playWaveAudio", "call play_func in final position: %ld\n",
		    (long)(wave->play_offset + wave->play_length));
	    flag = wave->config->play_func(wave, in_thread, SW_PROCESS_REACH_END, wave->core->call_data);
	}
	    
	swLockMutex(wave);
	wave->core->process_flag = SP_FALSE;
	swUnlockMutex(wave);
	    
	if (started == SP_TRUE && wave->config->play_func != NULL) {
	    flag = wave->config->play_func(wave, in_thread, SW_PROCESS_FINISHED, wave->core->call_data);
	}
    }
    
    swLockMutex(wave);
    wave->core->process_flag = SP_FALSE;
    wave->core->process_finished = SP_TRUE;
    wave->core->play_flag = SP_FALSE;
    swUnlockMutex(wave);
    
    swEndAudio(audio);
    
    spDebug(80, "playWaveAudio", "done: flag = %d\n", flag);
    
    return flag;
}

static spBool playWaveFile(swWave wave, spLong offset, spLong length)
{
    spBool flag;
    int swap;
    int num_channel;
    int selected_channel;
    spLong k;
    spLong nread;
    spLong dlength;
    spLong play_length;
    spLong woffset, wlength;
    spLong rlength;
    short *dummy;
    spWaveInfo wave_info;
    char filename[SP_MAX_PATHNAME];
    FILE *fp;
    spPlugin *plugin;

    if (swIsWaveFloat(wave) == SP_TRUE) {
	return SP_FALSE;
    }

    if (length > wave->config->max_read_length) {
	if (wave->config->error_func != NULL) {
	    return wave->config->error_func(wave, SP_FALSE, SW_ERROR_PLAY_LONG, SW_EDIT_NONE, wave->core->call_data);
	}
	
	return SP_FALSE;
    }
    
    /* make temporary file */
    if (wave->config->play_use_wav == SP_TRUE) {
	swap = SP_WAV_NEED_SWAP;
	sprintf(filename, "%s%csp%ld.wav",
		spGetTempDir(), SP_DIR_SEPARATOR, spGetProcessId());
    } else {
	swap = 0;
	sprintf(filename, "%s%csp%ld.ad",
		spGetTempDir(), SP_DIR_SEPARATOR, spGetProcessId());
    }

    if (NULL == (fp = spOpenFile(filename, "wb"))) {
        return SP_FALSE;
    }

    spDebug(10, "playWaveFile", "file = %s\n", filename);

    selected_channel = wave->selected_channel;
    if (selected_channel >= 0) {
	num_channel = 1;
    } else {
	num_channel = wave->num_channel;
    }

    /* get dummy length */
    dlength =(spLong)(SP_WAVE_PLAY_DUMMY_TIME * wave->samp_rate);
    dlength = dlength * (spLong)num_channel;
    woffset = offset * (spLong)num_channel;
    wlength = length * (spLong)num_channel;
    
    if (wave->config->play_use_wav == SP_TRUE) {
	spInitWaveInfo(&wave_info);
	wave_info.num_channel = num_channel;
	wave_info.samp_rate = wave->samp_rate;
	wave_info.samp_bit = 16;
	wave_info.length = dlength * 2 + wlength;
	
	/* write header */
	spWriteWavInfo(&wave_info, fp);
    }

    /* write dummy */
    dummy = xalloc(dlength, short);
    for (k = 0; k < dlength; k++) dummy[k] = 0;
    fwriteshort(dummy, (long)dlength, swap, fp);

    /* open wave */
    if ((plugin = swOpenWave(wave, "r")) == NULL) {
	return SP_FALSE;
    }

    spSeekPlugin(plugin, offset);

    rlength = MAX(wave->config->play_read_length,
		  wave->config->audio_buffer_size / num_channel);
    spDebug(10, "playWaveFile", "rlength = %ld\n", (long)rlength);
    
    play_length = 0;
    while ((nread = swReadBuffer(plugin, wave, selected_channel, rlength)) > 0) {
	if (play_length + nread >= length) {
	    nread = length - play_length;
	}
	
	/* write signal */
	if (swIsWaveLong(wave) == SP_TRUE) {
	    if (wave->samp_bit == 24) {
		fwritelong24tos((long *)wave->buf, (long)(nread * num_channel), swap, fp);
	    } else {
		fwritelong32tos((long *)wave->buf, (long)(nread * num_channel), swap, fp);
	    }
	} else {
	    fwriteshort((short *)wave->buf, (long)(nread * num_channel), swap, fp);
	}
	play_length += nread;

	if (play_length >= length) {
	    break;
	}
    }
    
    /* write dummy */
    fwriteshort(dummy, (long)dlength, swap, fp);
    xfree(dummy);

    /* close file */
    spCloseFile(fp);
    
    /* execute play command */
    flag = spPlayFile(filename, num_channel, wave->samp_rate);

    /* delete temporary file */
    spRemoveFile(filename);

    /* close input file */
    spCloseFilePlugin(plugin);
    
    return flag;
}

#ifdef SW_USE_THREAD
static spThreadReturn playWaveThread(void *data)
{
    spBool flag;
    swProcessConfig *config = (swProcessConfig *)data;

    if (config == NULL) return SP_THREAD_RETURN_FAILURE;
    
    spDebug(10, "playWaveThread", "in\n");

    config->wave->core->in_audio_thread = SP_TRUE;
    
    flag = playWaveAudio(config->wave, config->flag, SP_TRUE);
    spDebug(10, "playWaveThread", "flag = %d\n", flag);
    
    if (config->wave != NULL) { /* config->wave can be NULL in callback functions */
        if (flag == SP_FALSE && config->wave->config->play_func != NULL) {
            spDebug(10, "playWaveThread", "---- error ----\n");
            config->wave->config->play_func(config->wave, SP_TRUE, SW_PROCESS_ERROR, config->wave->core->call_data);
            spDebug(10, "playWaveThread", "after play_func\n");
        }
        config->wave->core->in_audio_thread = SP_FALSE;
    }

    spDebug(10, "playWaveThread", "done\n");
    
    if (flag == SP_TRUE) {
	return SP_THREAD_RETURN_SUCCESS;
    } else {
	return SP_THREAD_RETURN_FAILURE;
    }
}
#endif

static void correctPlayRegion(swWave wave)
{
    wave->play_offset = MAX(wave->play_offset, 0);
    wave->play_offset = MIN(wave->play_offset, wave->total_length - 1);
    if (wave->play_length <= 0) wave->play_length = wave->total_length;
    wave->play_length = MIN(wave->play_length, wave->total_length - wave->play_offset);

    return;
}

spBool swSetPlayRegion(swWave wave, spLong offset, spLong length)
{
    if (swIsWaveNone(wave) == SP_TRUE) {
	return SP_FALSE;
    }
    
    swLockMutex(wave);
    spDebug(50, "swSetPlayRegion", "offset = %ld, length = %ld\n", (long)offset, (long)length);
    if (offset >= 0) wave->play_offset = offset;
    if (length >= 0) wave->play_length = length;
    correctPlayRegion(wave);
    swUnlockMutex(wave);
    
    return SP_TRUE;
}

spBool swSetPlayStartOffset(swWave wave, spLong start_offset, spBool suspend_callback, spBool from_current_pos)
{
    if (swIsWaveNone(wave) == SP_TRUE) {
	return SP_FALSE;
    }
    
    swLockMutex(wave);
    spDebug(50, "swSetPlayStartOffset", "start_offset = %ld, from_current_pos = %d\n",
	    (long)start_offset, (long)from_current_pos);
    wave->play_start_offset = start_offset;
    wave->play_start_offset_updated = SP_TRUE;
    wave->play_start_sync_pos = from_current_pos;
    
    wave->suspend_play_callback = suspend_callback;
    swUnlockMutex(wave);
    
    return SP_TRUE;
}

spBool swPlayWave(swWave wave, spLong offset, spLong length, spLong start_offset, spBool loop_flag)
{
    if (swIsWaveNone(wave) == SP_TRUE || swIsWaveProcessing(wave) == SP_TRUE
	|| swIsWavePlaying(wave) == SP_TRUE) {
	spDebug(10, "swPlayWave", "playing failed because of busy\n");
	return SP_FALSE;
    }

    swLockMutex(wave);
    wave->play_offset = offset;
    wave->play_length = length;
    wave->play_start_offset = start_offset;
    
    if (wave->play_length <= 0) {
	wave->selected_channel = -1;
    }
    correctPlayRegion(wave);
    swUnlockMutex(wave);

#ifdef SP_SUPPORT_AUDIO
    if (wave->config->play_use_audio == SP_TRUE /*&& swIsWaveFloat(wave) == SP_FALSE*/) {
	if (swInitAudio(wave->config) == SP_FALSE) {
	    return SP_FALSE;
	}
#ifdef SW_USE_THREAD
	if (wave->config->thread_safe == SP_TRUE && wave->config->process_use_thread == SP_TRUE) {
	    wave->core->play_flag = SP_TRUE;
	    wave->core->process_config.edit_type = SW_EDIT_NONE;
	    wave->core->process_config.wave = wave;
	    wave->core->process_config.flag = loop_flag;

	    if (wave->core->audio_thread != NULL) {
                if (wave->core->in_audio_thread == SP_FALSE) {
                    spDebug(10, "swPlayWave", "waiting thread...\n");
                    wave->config->thread_wait_func(wave, wave->core->audio_thread, wave->core->call_data);
                }
		spDestroyThread(wave->core->audio_thread);
		wave->core->audio_thread = NULL;
		spDebug(10, "swPlayWave", "waiting thread done\n");
	    }
	    
	    if ((wave->core->audio_thread = swCreateProcessThread(SP_THREAD_PRIORITY_NORMAL,
								  playWaveThread, &wave->core->process_config)) != NULL) {
		return SP_TRUE;
	    }
	    wave->core->play_flag = SP_FALSE;
	    spDebug(1, "swPlayWave", "thread error\n");
	}
#endif
	return playWaveAudio(wave, loop_flag, SP_FALSE);
    }
#endif

    return playWaveFile(wave, wave->play_offset, wave->play_length);
}

static spBool swAudioCallbackFunc(spAudio audio, spAudioCallbackType call_type,
				  void *data1, void *data2, void *user_data)
{
    swAudio swaudio = (swAudio)user_data;
    swWave wave;
    spBool flag;
    spLong pos;
    
    spDebug(10, "swAudioCallbackFunc", "in: call_type = %d\n", call_type);

    flag = SP_TRUE;
    wave = swaudio->wave;

    if (call_type == SP_AUDIO_OUTPUT_POSITION_CALLBACK) {
	spLong *ppos = (spLong *)data1;
	
	if (wave->config->play_func != NULL) {
	    swLockMutex(wave);
	    pos = swGetFilePlayPosition(swaudio, *ppos);
	    spDebug(10, "swAudioCallbackFunc", "updated pos = %ld\n", (long)pos);
	    swUnlockMutex(wave);

	    if (*ppos > swaudio->suspend_release_pos) {
		flag = wave->config->play_func(wave, swaudio->in_thread, pos, wave->core->call_data);
		
		swLockMutex(wave);
		swaudio->suspend_release_pos = -1;
		swUnlockMutex(wave);
	    }
	}
    }

    if (flag == SP_FALSE || wave->core->process_flag == SP_FALSE) {
	return SP_FALSE;
    }
    
    spDebug(10, "swAudioCallbackFunc", "done\n");
    
    return SP_TRUE;
}

static swAudio sw_audio = NULL;

spBool swIsWavePlayable(swWave wave)
{
    spBool flag = SP_TRUE;
    
    if (wave == NULL || wave->num_order > 1) {
	spDebug(80, "swIsWavePlayable", "non-playable wave\n");
	return SP_FALSE;
    }
    
    swLockMutex(wave);
    if (sw_audio != NULL && sw_audio->wave != NULL) {
	spDebug(80, "swIsWavePlayable", "already playing wave exists\n");
	flag = SP_FALSE;
    }
    swUnlockMutex(wave);
    
    return flag;
}

static void swTerminateAudio(void *data)
{
    spDebug(1, "swTerminateAudio", "in\n");
    
    if (sw_audio != NULL) {
	if (sw_audio->audio != NULL) {
	    spFreeAudioDriver(sw_audio->audio);
	}

	if (sw_audio->buf != NULL) {
	    xfree(sw_audio->buf);
	}
	
	xfree(sw_audio);
	sw_audio = NULL;
    }
    
    spDebug(1, "swTerminateAudio", "done\n");
    
    return;
}

spBool swInitAudio(swWaveConfig config)
{
    spAudio audio;
    char *audio_driver = NULL;
    
    if (sw_audio == NULL || !streq(config->audio_driver, sw_audio->audio_driver)) {
	if (sw_audio != NULL && sw_audio->audio != NULL) {
	    spFreeAudioDriver(sw_audio->audio);
	}
	
	if ((audio = spInitAudioDriver(config->audio_driver)) == NULL) {
	    /* open with default driver */
	    if (strnone(config->audio_driver) || (audio = spInitAudioDriver(NULL)) == NULL) {
                spDebug(10, "swInitAudio", "spInitAudioDriver failed\n");
		return SP_FALSE;
	    }
	} else {
	    audio_driver = config->audio_driver;
	}

	if (sw_audio == NULL) {
	    sw_audio = xalloc(1, struct _swAudio);
	    sw_audio->buf_size = 0;
	    sw_audio->buf = NULL;
	    sw_audio->in_thread = SP_FALSE;

	    spAddAudioExitCallback(audio, swTerminateAudio);
	}
	
	sw_audio->audio = audio;
	sw_audio->wave = NULL;
	strcpy(sw_audio->audio_driver, "");

	if (audio_driver != NULL) {
	    spStrCopy(sw_audio->audio_driver, sizeof(sw_audio->audio_driver), audio_driver);
	}
    }

    spDebug(80, "swInitAudio", "done\n");
    
    return SP_TRUE;
}

swAudio swBeginAudio(swWave wave, spBool in_thread)
{
    int num_channel;
    spLong read_size;
    
    if (wave == NULL) return NULL;
    
    spDebug(80, "swBeginAudio", "in\n");
    
    swLockMutex(wave);
    
    if (sw_audio->audio != NULL) {
	sw_audio->wave = wave;
	sw_audio->in_thread = in_thread;

	read_size = swConvertLengthToByte(wave, wave->config->play_read_length);
	if (sw_audio->buf_size < read_size) {
	    sw_audio->buf_size = read_size;
	    if (sw_audio->buf == NULL) {
		sw_audio->buf = xalloc(sw_audio->buf_size, char);
	    } else {
		sw_audio->buf = xrealloc(sw_audio->buf, sw_audio->buf_size, char);
	    }
	}
	
	spSetAudioSampleRate(sw_audio->audio, wave->samp_rate);
	spSetAudioBufferSize(sw_audio->audio, wave->config->audio_buffer_size);
	if (wave->samp_bit >= 33) {
	    spSetAudioSampleBit(sw_audio->audio, 64);
	} else {
	    spSetAudioSampleBit(sw_audio->audio, MAX(wave->samp_bit, 16));
	}
	sw_audio->position_callback_used = SP_FALSE;
	if (spSetAudioCallbackFunc(sw_audio->audio, SP_AUDIO_OUTPUT_POSITION_CALLBACK,
				   swAudioCallbackFunc, sw_audio) == SP_TRUE) {
	    sw_audio->position_callback_used = SP_TRUE;
	}
	
	if (wave->selected_channel >= 0) {
	    num_channel = 1;
	} else {
	    num_channel = wave->num_channel;
	}
	spSetAudioChannel(sw_audio->audio, num_channel);
    }
    
    swUnlockMutex(wave);

    spDebug(80, "swBeginAudio", "done\n");
    
    return sw_audio;
}

spBool swEndAudio(swAudio audio)
{
    swWave wave;
    
    if (audio == NULL || audio->wave == NULL) return SP_FALSE;

    wave = audio->wave;
    swLockMutex(wave);
    audio->wave = NULL;
    swUnlockMutex(wave);
    
    return SP_TRUE;
}
