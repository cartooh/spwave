/*
 *	swWindow.c
 *
 *	Last modified: <2025-04-27 19:57:07 hideki>
 */

#include <stdio.h>
#include <stdlib.h>

#include <sp/spBaseLib.h>
#include <sp/spAudioLib.h>
#include <sp/spComponentLib.h>
#include <sp/spPlugin.h>

#include "swWave.h"
#include "swWaveAudio.h"

#include "swWindow.h"
#include "swDialog.h"
#include "swDraw.h"
#include "swCursor.h"
#include "swEdit.h"
#include "swLabel.h"
#include "swLabelDialog.h"
#include "swAnalysis.h"
#include "swAnalysisDialog.h"
#include "swInfoDialog.h"
#include "swLabelList.h"
#if defined(SW_SUPPORT_MORPHING)
#include "swMorphingDraw.h"
#endif
#if defined(SW_AH_CUSTOM)
#include "swWindowAH.h"
#include "swLabelAH.h"
#endif


#if defined(SW_USE_POST_EVENT)

typedef struct _swPostEventData {
    swWindow window;
    spLong cb_pos;
    swEditType cb_edit_type;
    void *cb_data;
} *swPostEventData;

#define SW_WAVE_OPTION_EVENT (SP_EVENT_USER)
#define SW_WAVE_ERROR_EVENT (SP_EVENT_USER+1)
#define SW_WAVE_READ_EVENT (SP_EVENT_USER+2)
#define SW_WAVE_EDIT_FINISH_EVENT (SP_EVENT_USER+3)
#define SW_WAVE_PROCESS_EVENT (SP_EVENT_USER+4)
#endif

#define SW_DEFAULT_CLOSE_STYLE SP_CALLBACK_CLOSE

#define SW_MIN_DRAW_INTERVAL_F /*0.2*/0.1

swWaveSubArea swInitWaveSubArea(swWindow window)
{
    swWaveSubArea sub_area;
    
    sub_area = xalloc(1, struct _swWaveSubArea);
    sub_area->wave = NULL;
    sub_area->specgram_flag = SP_FALSE;
    sub_area->pos_index = 0;
    sub_area->drawn_pos = 0;
    sub_area->y_d = 0.0;
    sub_area->height_d = 1.0;	/* visible state */
    sub_area->draw_height = 0.0;
    sub_area->prev = NULL;
    sub_area->next = NULL;
    
    if (window->first_sub_area == NULL) {
	window->first_sub_area = sub_area;
    }
    if (window->last_sub_area != NULL) {
	window->last_sub_area->next = sub_area;
	sub_area->prev = window->last_sub_area;
    }
    window->last_sub_area = sub_area;

    return sub_area;
}

spBool swFreeWaveSubArea(swWindow window, swWaveSubArea sub_area)
{
    if (sub_area == NULL) return SP_FALSE;
    
    if (sub_area->prev != NULL) {
	sub_area->prev->next = sub_area->next;
    }
    if (sub_area->next != NULL) {
	sub_area->next->prev = sub_area->prev;
    }
    if (window->first_sub_area == sub_area) {
	window->first_sub_area = sub_area->next;
    }
    if (window->last_sub_area == sub_area) {
	window->last_sub_area = sub_area->prev;
    }
    window->num_sub_area = MAX(window->num_sub_area - 1, 0);
    
    xfree(sub_area);
    
    return SP_TRUE;
}

swWaveSubArea swSetWaveToSubArea(swWindow window, swWave wave, spBool specgram_flag, spBool non_first_only)
{
    swWaveSubArea sub_area;

    if (wave == NULL) return NULL;

    if ((sub_area = swGetWaveSubArea(window, wave)) != NULL
	&& (non_first_only == SP_FALSE || window->first_sub_area != sub_area)) {
	/* already exist */
	sub_area->height_d = MAX(sub_area->height_d, 1.0);
	return sub_area;
    }
    
    if ((sub_area = swGetWaveSubArea(window, NULL)) == NULL) {
	sub_area = swInitWaveSubArea(window);
	window->num_sub_area++;
    }
    sub_area->wave = wave;
    sub_area->specgram_flag = specgram_flag;
    sub_area->height_d = 1.0;
    spDebug(100, "swSetWaveToSubArea", "done\n");
    
    return sub_area;
}

spBool swUnsetWaveToSubArea(swWindow window, swWave wave, spBool non_first_only)
{
    long count;
    swWaveSubArea sub_area;

    spDebug(100, "swUnsetWaveToSubArea", "wave = %ld\n", (long)wave);
    
    if (wave != NULL) {
	count = 0;
	sub_area = swGetNextWaveSubArea(window, NULL);
	
	while (sub_area != NULL) {
	    if (sub_area->wave == wave) {
		if (non_first_only == SP_TRUE && sub_area == window->first_sub_area) {
		    /* do nothing */
		} else {
		    spDebug(100, "swUnsetWaveToSubArea", "unset OK: specgram_flag = %d\n", sub_area->specgram_flag);
		    sub_area->wave = NULL;
		    sub_area->specgram_flag = SP_FALSE;
		    sub_area->drawn_pos = 0;
		    sub_area->y_d = 0.0;
		    sub_area->height_d = 0.0;
		    ++count;
		}
	    }
	    
	    sub_area = swGetNextWaveSubArea(window, sub_area);
	}

	if (count >= 1) {
	    return SP_TRUE;
	}
    }
    
    spDebug(100, "swUnsetWaveToSubArea", "failed\n");
    return SP_FALSE;
}

swWaveSubArea swSetWaveToFirstSubArea(swWindow window, swWave wave, spBool specgram_flag)
{
    swWaveSubArea sub_area;
    
    spDebug(100, "swSetWaveToFirstSubArea", "specgram_flag = %d\n", specgram_flag);
    
    if (window->first_sub_area == NULL) {
	spDebug(100, "swSetWaveToFirstSubArea", "set wave firstly\n");
	return swSetWaveToSubArea(window, wave, specgram_flag, SP_FALSE);
    }
    sub_area = window->first_sub_area;
    
    sub_area->wave = wave;
    sub_area->specgram_flag = specgram_flag;
    sub_area->height_d = MAX(sub_area->height_d, 1.0);

    spDebug(100, "swSetWaveToFirstSubArea", "done: height_d = %f\n", sub_area->height_d);
    
    return sub_area;
}

swWaveSubArea swReplaceWaveSubArea(swWindow window, swWave old_wave, swWave new_wave, spBool specgram_flag)
{
    swWaveSubArea sub_area;
    
    if ((sub_area = swGetWaveSubArea(window, old_wave)) == NULL) {
	return swSetWaveToSubArea(window, new_wave, specgram_flag, SP_FALSE);
    }
    
    sub_area->wave = new_wave;
    sub_area->specgram_flag = specgram_flag;

    return sub_area;
}

swWaveSubArea swGetWaveSubArea(swWindow window, swWave wave)
{
    long k;
    swWaveSubArea sub_area;

    if (window == NULL) return NULL;

    sub_area = window->first_sub_area;
    
    for (k = 0; sub_area != NULL; k++) {
	if (sub_area->wave == wave) {
	    return sub_area;
	}
	sub_area = sub_area->next;
    }

    return NULL;
}
    
spBool swIsWaveSubAreaVisible(swWaveSubArea sub_area)
{
    if (sub_area == NULL || sub_area->wave == NULL
	|| sub_area->height_d <= 0.0) {
	return SP_FALSE;
    } else {
	return SP_TRUE;
    }
}

spBool swIsWaveSubAreaSpectrogram(swWaveSubArea sub_area)
{
    if (sub_area == NULL || sub_area->wave == NULL) return SP_FALSE;
    
    return sub_area->specgram_flag;
}

spBool swResetDrawnPos(swWindow window, swWave wave)
{
    swWaveSubArea sub_area;
    
    if (wave != NULL && (sub_area = swGetWaveSubArea(window, wave)) != NULL) {
	sub_area->drawn_pos = 0;
	return SP_TRUE;
    } else {
	return SP_FALSE;
    }
}

long swResetAllDrawnPos(swWindow window)
{
    long k;
    swWaveSubArea sub_area;

    sub_area = window->first_sub_area;
    
    for (k = 0; sub_area != NULL; k++) {
	sub_area->drawn_pos = 0;
	sub_area = sub_area->next;
    }

    return k;
}

long swGetNumWaveSubArea(swWindow window)
{
    long k, count;
    swWaveSubArea sub_area;

    sub_area = swGetNextWaveSubArea(window, NULL);

    count = 0;
    for (k = 0; sub_area != NULL; k++) {
	spDebug(100, "swGetNumWaveSubArea", "k = %ld: y_d = %f, height_d = %f\n",
		k, sub_area->y_d, sub_area->height_d);
        ++count;
	sub_area = swGetNextWaveSubArea(window, sub_area);
    }

    spDebug(100, "swGetNumWaveSubArea", "done: k = %ld, count = %ld\n", k, count);
    
    return count;
}

long swGetNumVisibleWaveSubArea(swWindow window)
{
    long k, count;
    swWaveSubArea sub_area;

    sub_area = swGetNextWaveSubArea(window, NULL);

    count = 0;
    for (k = 0; sub_area != NULL; k++) {
	spDebug(100, "swGetNumVisibleWaveSubArea", "k = %ld: y_d = %f, height_d = %f\n",
		k, sub_area->y_d, sub_area->height_d);
	if (swIsWaveSubAreaVisible(sub_area) == SP_TRUE) {
	    ++count;
	}
	sub_area = swGetNextWaveSubArea(window, sub_area);
    }

    spDebug(100, "swGetNumVisibleWaveSubArea", "done: k = %ld, count = %ld\n", k, count);
    
    return count;
}

double swUpdateWaveSubAreaSize(swWindow window)
{
    int num_channel;
    long count;
    long num_visible;
    double main_height;
    double height, total_height;
    double orig_height_d;
    swWaveSubArea sub_area;

    window->draw_width = (double)swGetDrawWidth(window, SP_FALSE);
    window->vertical_keys_width = swCalcVerticalKeysWidth(window, SW_PIANO_KEYS_DEFAULT_SIZE, window->draw_width);
    
    spDebug(80, "swUpdateWaveSubAreaSize", "window->data_type = %d, window->draw_horizontal_keys = %d\n",
	    window->data_type, window->draw_horizontal_keys);
    
    num_visible = swGetNumVisibleWaveSubArea(window);
    main_height = (double)window->height;
    if (window->data_type == SW_FREQ_DATA &&
        (window->config->draw_piano_keys_for_spectrum == SP_TRUE || window->draw_horizontal_keys == SP_TRUE)) {
        window->draw_horizontal_keys = SP_TRUE;
        window->draw_height_horizontal_keys = SW_PIANO_KEYS_DEFAULT_SIZE;
        window->draw_height_horizontal_keys = MIN(window->draw_height_horizontal_keys,
                                                  (double)window->height * SW_PIANO_KEYS_MAX_SIZE_PERCENT / 100.0);
        main_height -= window->draw_height_horizontal_keys;
        main_height = MAX(main_height, (double)(2 * num_visible));
        window->draw_height_horizontal_keys = (double)window->height - main_height;
        window->draw_height_horizontal_keys = MAX(window->draw_height_horizontal_keys, 0.0);
        if (window->config->horizontal_piano_keys_top == SP_TRUE) {
            total_height = window->draw_height_horizontal_keys;
        } else {
            total_height = 0.0;
        }
    } else {
        window->draw_horizontal_keys = SP_FALSE;
        window->draw_height_horizontal_keys = 0.0;
        total_height = 0.0;
    }
    height = main_height / (double)MAX(num_visible, 1);
    spDebug(80, "swUpdateWaveSubAreaSize", "window->height = %d, height = %f, num_visible = %ld\n",
	    window->height, height, num_visible);
    
    sub_area = swGetNextWaveSubArea(window, NULL);

    count = 0;
    orig_height_d = height;

    while (sub_area != NULL) {
	if (swIsWaveSubAreaVisible(sub_area) == SP_TRUE) {
            num_channel = MAX(sub_area->wave->num_channel, 1);
    
            spDebug(80, "swUpdateWaveSubAreaSize",
                    "height = %d, window->data_type = %d, sub_area->wave->num_order = %ld, window->config->draw_piano_keys_for_spectrum = %d\n",
                    height, window->data_type, sub_area->wave->num_order, window->config->draw_piano_keys_for_spectrum);
            
	    sub_area->y_d = total_height;
	    sub_area->height_d = height;
	    sub_area->draw_height = height / (double)num_channel;
	    sub_area->draw_height = MAX(sub_area->draw_height, 2);
	    sub_area->draw_height = MAX(sub_area->draw_height, SW_MIN_DRAW_HEIGHT);
	    total_height += sub_area->height_d;
            spDebug(80, "swUpdateWaveSubAreaSize", "total_height = %f\n", total_height);
	}
	sub_area = swGetNextWaveSubArea(window, sub_area);
    }
    spDebug(80, "swUpdateWaveSubAreaSize", "total_height = %f\n", total_height);
    
    return total_height;
}

spBool swIsLastWaveSubArea(swWindow window, swWaveSubArea sub_area)
{
    swWaveSubArea next;
    
    if (window == NULL || sub_area == NULL) return SP_FALSE;

    next = sub_area->next;

    while (next != NULL) {
        if (next->wave != NULL || next->height_d > 0.0) {
            return SP_FALSE;
        }
	next = next->next;
    }
    
    return SP_TRUE;
}

spBool swIsFirstWaveSubArea(swWindow window, swWaveSubArea sub_area)
{
    swWaveSubArea prev;
    
    if (window == NULL || sub_area == NULL) return SP_FALSE;

    prev = sub_area->prev;

    while (prev != NULL) {
        if (prev->wave != NULL || prev->height_d > 0.0) {
            return SP_FALSE;
        }
	prev = prev->prev;
    }
    
    return SP_TRUE;
}

swWaveSubArea swGetNextWaveSubArea(swWindow window, swWaveSubArea sub_area)
{
    swWaveSubArea next;
    
    if (window == NULL) return NULL;

    if (sub_area == NULL) {
	next = window->first_sub_area;
    } else {
	next = sub_area->next;
    }

    while (next != NULL && next->wave == NULL) {
	next = next->next;
    }

    return next;
}

void swDestroyWindowWave(swWindow window, swWave *wave)
{
    if (wave != NULL && *wave != NULL) {
	swUnsetWaveToSubArea(window, *wave, SP_FALSE);
	swDestroyWave(*wave);
	*wave = NULL;
    }
    
    spDebug(80, "swDestroyWindowWave", "done\n");
    
    return;
}
    
swWindow swGetNextWindow(swWindow window)
{
    spComponent next;
    swWindow next_window = NULL;

    if (window == NULL || window->window == NULL) return NULL;

    next = spGetNextWindow(window->window, SP_FALSE);
    while (next != NULL && next != window->window) {
	next_window = (swWindow)spGetUserData(next);
	if (swIsClipboardWindow(next_window) == SP_TRUE) {
	    if (swIsClipboardVisible(next_window) == SP_TRUE) {
		break;
	    }
	} else if (next_window != NULL) {
	    break;
	}
	next_window = NULL;
	spDebug(80, "swGetNextWindow", "next\n");
	next = spGetNextWindow(next, SP_FALSE);
    }

    return next_window;
}

swWindow swGetPrevWindow(swWindow window)
{
    spComponent prev;
    swWindow prev_window = NULL;

    if (window == NULL || window->window == NULL) return NULL;

    prev = spGetPrevWindow(window->window, SP_FALSE);
    while (prev != NULL && prev != window->window) {
	prev_window = (swWindow)spGetUserData(prev);
	if (swIsClipboardWindow(prev_window) == SP_TRUE) {
	    if (swIsClipboardVisible(prev_window) == SP_TRUE) {
		break;
	    }
	} else if (prev_window != NULL) {
	    break;
	}
	prev_window = NULL;
	spDebug(80, "swGetNextWindow", "prev\n");
	prev = spGetPrevWindow(prev, SP_FALSE);
    }

    return prev_window;
}

static void popdownOptionDialogCB(spComponent component, spCallbackReason *preason)
{
    spCallbackReason reason;
    
    reason = spGetCallbackReason(component);

    if (reason == SP_CR_OK || reason == SP_CR_APPLY) {
	spGetOptionDialogData(component);
    }
    
    if (preason != NULL) {
	*preason = reason;
	spDebug(10, "popdownOptionDialogCB", "reason = %d\n", *preason);
    }

    if (reason == SP_CR_OK || reason == SP_CR_CANCEL) {
	spPopdownWindow(component);
    }

    return;
}

#ifdef SW_USE_POST_EVENT
spBool swPostUserEvent(swWindow window, spEventType event_type,
		       spLong cb_pos, swEditType cb_edit_type, void *cb_data)
{
    spBool return_value;
    spBool need_event_wait = SP_TRUE;
    swPostEventData post_event_data;
	
    spDebug(80, "swPostUserEvent", "cb_pos = %ld, cb_edit_type = %d\n", (long)cb_pos, cb_edit_type);

#if 1
    if (event_type == SW_WAVE_PROCESS_EVENT
	&& !(cb_pos == SW_PROCESS_STARTED || cb_pos == SW_PROCESS_FINISHED
	     || cb_pos == SW_PROCESS_REACH_END || cb_pos == SW_PROCESS_ERROR)) {
	need_event_wait = SP_FALSE;
    }
#endif
    spDebug(80, "swPostUserEvent", "need_event_wait = %d\n", need_event_wait);

    post_event_data = xalloc(1, struct _swPostEventData);
    post_event_data->window = window;
    post_event_data->cb_pos = cb_pos;
    post_event_data->cb_edit_type = cb_edit_type;
    post_event_data->cb_data = cb_data;
	
    if (need_event_wait == SP_TRUE) {
	if (event_type == SW_WAVE_EDIT_FINISH_EVENT) {
	    swResetEvent((swWave)cb_data);
	} else {
	    swResetEvent(window->wave);
	}
	spDebug(80, "swPostUserEvent", "swResetEvent done\n");
    }
       
    spDebug(80, "swPostUserEvent", "before spThreadEnter\n");
    spThreadEnter();
    spDebug(80, "swPostUserEvent", "posting...\n");
    spPostUserEvent(window->window, event_type, post_event_data);
    spDebug(80, "swPostUserEvent", "posting done\n");
    spThreadLeave();
    spDebug(80, "swPostUserEvent", "after spThreadLeave\n");

    if (need_event_wait == SP_TRUE) {
	spDebug(80, "swPostUserEvent", "call swWaitEvent: window->wave = %ld, cb_data = %ld\n",
		(long)window->wave, (long)cb_data);
	
	if (event_type == SW_WAVE_EDIT_FINISH_EVENT) {
	    swWaitEvent((swWave)cb_data);
	} else {
	    swWaitEvent(window->wave);
	}
	spDebug(80, "swPostUserEvent", "swWaitEvent done: cb_return = %d\n", window->cb_return);
	/*return SP_TRUE;*/
	return_value = window->cb_return;
    } else {
	spDebug(80, "swPostUserEvent", "done\n");
	return_value = SP_TRUE;
    }

    return return_value;
}
#endif

static spBool waveOptionCallbackEx(swWave wave, spBool in_thread, spBool dispatch_flag, spOptions options, void *data)
{
    spBool flag = SP_TRUE;
    spBool thread_entered = SP_FALSE;
    spComponent dialog;
    spCallbackReason reason = SP_CR_CANCEL;
    swWindow window = (swWindow)data;

    if (window != NULL && wave != NULL) {
#ifdef SW_USE_POST_EVENT
	if (in_thread == SP_TRUE) {
	    return swPostUserEvent(window, SW_WAVE_OPTION_EVENT, 0, SW_EDIT_NONE, options);
	}
#endif
	
	if (in_thread == SP_TRUE) {
	    swLockWindowMutex(window);
	    if ((thread_entered = window->thread_entered) == SP_FALSE) {
		window->thread_entered = SP_TRUE;
	    }
	    spDebug(10, "waveOptionCallback", "in_thread = %d, thread_entered = %d\n",
		    in_thread, thread_entered);
	    swUnlockWindowMutex(window);
	    
	    if (thread_entered == SP_FALSE) {
		spThreadEnter();
	    }
	}
	
	swSetMouseCursor(window, SP_CURSOR_UNKNOWN);
	
	spDebug(10, "waveOptionCallback", "in\n");
	dialog = spCreateOptionDialog("Options for Output File",
				      options,
				      SppCallbackFunc, popdownOptionDialogCB,
				      SppCallbackData, &reason,
				      SppDialogBoxButtonType, SP_DB_OK_CANCEL,
				      SppCloseStyle, SP_UNMAP_CLOSE,
				      NULL);
	spDebug(10, "waveOptionCallback", "create dialog done\n");
	spPopupWindow(dialog);
	spDebug(10, "waveOptionCallback", "spPopupWindow done\n");
	spDestroyWindow(dialog);
	
	spDebug(10, "waveOptionCallback", "reason = %d\n", reason);
	if (reason == SP_CR_OK) {
	    swSetMouseCursor(window, SP_CURSOR_WAIT);
	} else {
	    flag = SP_FALSE;
	}
	
	if (in_thread == SP_TRUE) {
	    if (thread_entered == SP_FALSE) {
		spThreadLeave();
		
		swLockWindowMutex(window);
		window->thread_entered = SP_FALSE;
		swUnlockWindowMutex(window);
		spDebug(10, "waveOptionCallback", "spThreadLeave called\n");
	    }

	    spYieldThread();
	} else {
	    if (dispatch_flag == SP_TRUE) {
		spDispatchEvent(window->config->toplevel->toplevel);
	    }
	}
    }
    
    spDebug(10, "waveOptionCallback", "done\n");
    
    return flag;
}

static spBool waveOptionCallback(swWave wave, spBool in_thread, spOptions options, void *data)
{
    return waveOptionCallbackEx(wave, in_thread, SP_TRUE, options, data);
}

static spBool waveErrorCallbackEx(swWave wave, spBool in_thread, spBool dispatch_flag, int error, swEditType edit_type, void *data)
{
    int last_error;
    spComponent w;
    spBool flag;
    spBool thread_entered = SP_FALSE;
    swWindow window = (swWindow)data;

    spDebug(10, "waveErrorCallback", "error = %d, edit_type = %d, in_thread = %d\n", error, edit_type, in_thread);
    
#ifdef SW_USE_POST_EVENT
    if (in_thread == SP_TRUE) {
        spDebug(10, "waveErrorCallback", "in thread: error = %d, edit_type = %d, call swPostUserEvent\n", error, edit_type);
	return swPostUserEvent(window, SW_WAVE_ERROR_EVENT, (spLong)error, /*SW_EDIT_NONE*/edit_type, NULL);
    }
#endif
	
    if (in_thread == SP_TRUE) {
	if (window != NULL) {
	    swLockWindowMutex(window);
	    if ((thread_entered = window->thread_entered) == SP_FALSE) {
		window->thread_entered = SP_TRUE;
	    }
	    swUnlockWindowMutex(window);
	}
	spDebug(10, "waveErrorCallback", "in_thread = %d, thread_entered = %d\n",
		in_thread, thread_entered);
	
	if (thread_entered == SP_FALSE) {
	    spThreadEnter();
	}
    }
    
    if (window == NULL) {
	w = swGetFormatDialogWindow();
    } else {
	w = window->window;
    }

    flag = SP_FALSE;

    switch (error) {
      case SW_ERROR_GENERATE_STOPPED:
      case SW_ERROR_GENERATE_FAILED:
        if (wave != NULL) {
            last_error = swGetWaveLastError(wave);
            spDebug(10, "waveErrorCallback", "SW_ERROR_GENERATE_STOPPED or SW_ERROR_GENERATE_FAILED: last_error = %d\n", last_error);
            if (error == SW_ERROR_GENERATE_FAILED) {
                spDisplayError(w, SW_ERROR_TITLE,
                               last_error == SW_ERROR_AUDIO_DEVICE_ERROR ? SW_AUDIO_DEVICE_ERROR_MESSAGE : SW_EDIT_ERROR_MESSAGE);
            }
        }
        if (window != NULL) {
            swCloseWindowOrRevertToNullWindow(window, SP_FALSE, SP_TRUE);
        }
        break;
      case SP_PLUGIN_ERROR_MULTIPLE_INSTANCE:
	spDisplayError(w, SW_ERROR_TITLE, SW_MULTIPLE_INSTANCE_ERROR_MESSAGE);
	break;
      case SP_PLUGIN_ERROR_SUITABLE_NOT_FOUND:
	spDisplayError(w, SW_ERROR_TITLE, SW_SUFFIX_ERROR_MESSAGE);
	if (in_thread == SP_FALSE && wave != NULL) {
            spDebug(10, "waveErrorCallback", "SP_PLUGIN_ERROR_SUITABLE_NOT_FOUND: edit_type = %d\n", edit_type);
	    if (window != NULL) {
                char *filename = NULL;
                char *plugin_name = NULL;
                char *file_type = NULL;
                
                spDebug(10, "waveErrorCallback", "before xswGetSaveFileName: wave->new_filename = %s\n",
                        wave->new_filename);
                
                if ((filename = xswGetSaveFileName(w, window, wave->new_filename, 0, &plugin_name, &file_type)) != NULL) {
                    flag = SP_TRUE;
                    swLockMutex(wave);
                    if (wave->new_filename != NULL) xfree(wave->new_filename);
                    if (wave->new_plugin_name != NULL) xfree(wave->new_plugin_name);
                    if (wave->new_file_type != NULL) xfree(wave->new_file_type);
                    
                    wave->new_filename = filename;
                    wave->new_plugin_name = strclone(plugin_name);
                    wave->new_file_type = strclone(file_type);
                    wave->core->restart_flag = SP_TRUE;
                    swUnlockMutex(wave);
                    swSetEvent(wave);
                }
                spDebug(10, "waveErrorCallback", "xswGetSaveFileName result = %d\n", flag);
            }
	}
	break;
      case SP_PLUGIN_ERROR_SAMP_RATE:
	spDisplayError(w, SW_ERROR_TITLE, SW_SAMP_RATE_ERROR_MESSAGE);
	break;
      case SP_PLUGIN_ERROR_SAMP_BIT:
	spDisplayError(w, SW_ERROR_TITLE, SW_SAMP_BIT_ERROR_MESSAGE);
	break;
      case SP_PLUGIN_ERROR_NUM_CHANNEL:
	spDisplayError(w, SW_ERROR_TITLE, SW_NUM_CHANNEL_ERROR_MESSAGE);
	break;
      case SW_ERROR_PLAY_LONG:
	spDisplayError(w, SW_ERROR_TITLE, SW_PLAY_LONG_ERROR_MESSAGE);
	break;
      case SW_ERROR_LOOP_PLAY_SHORT:
	spDisplayError(w, SW_ERROR_TITLE, SW_LOOP_PLAY_SHORT_ERROR_MESSAGE);
	break;
      case SP_PLUGIN_ERROR_INSTANTIATE:
      case SP_PLUGIN_ERROR_LOAD:
      case SP_PLUGIN_ERROR_WRONG_PLUGIN:
	spDisplayError(w, SW_ERROR_TITLE, SW_PLUGIN_ERROR_MESSAGE);
	break;
      case SP_PLUGIN_ERROR_FAILURE:
	spDisplayError(w, SW_ERROR_TITLE, SW_FORMAT_NOT_SUPPORTED_MESSAGE);
	break;
      case SW_ERROR_SAMP_FREQ_CONV:
	spDisplayError(w, SW_ERROR_TITLE, _("SW_SAMP_FREQ_CONV_ERROR_MESSAGE"));
	break;
      case SW_ERROR_SAMP_FREQ_CONV_FILTER:
	spDisplayError(w, SW_ERROR_TITLE, _("SW_SAMP_FREQ_CONV_FILTER_ERROR_MESSAGE"));
	break;
      default:
	spDisplayError(w, SW_ERROR_TITLE, SW_SAVE_ERROR_MESSAGE);
	break;
    }
    
    if (in_thread == SP_TRUE) {
	if (thread_entered == SP_FALSE) {
	    spThreadLeave();

	    if (window != NULL) {
		swLockWindowMutex(window);
		window->thread_entered = SP_FALSE;
		swUnlockWindowMutex(window);
	    }
	    spDebug(10, "waveErrorCallback", "spThreadLeave called\n");
	}

	spYieldThread();
    } else {
	if (dispatch_flag == SP_TRUE) {
	    spDispatchEvent(spGetTopLevel(w));
	}
    }

    spDebug(100, "waveErrorCallback", "flag = %d\n", flag);
    
    return flag;
}

static spBool waveErrorCallback(swWave wave, spBool in_thread, int error, swEditType edit_type, void *data)
{
    return waveErrorCallbackEx(wave, in_thread, SP_TRUE, error, edit_type, data);
}

static spBool waveProcessCallbackEx(swWave wave, spBool in_thread, spBool dispatch_flag, spLong pos, swEditType edit_type, void *data)
{
    long point_d;
    spLong pos2;
    spLong drawn_pos;
    double point_f;
    spLong target_wave_length;
    spBool thread_entered = SP_FALSE;
    swWindow window = (swWindow)data;

    if (window != NULL && wave != NULL) {
	spDebug(80, "waveProcessCallback", "in_thread = %d, pos = %ld, edit_type = %d\n", in_thread, (long)pos, edit_type);
	
#ifdef SW_USE_POST_EVENT
	if (in_thread == SP_TRUE) {
	    return swPostUserEvent(window, SW_WAVE_PROCESS_EVENT, pos, edit_type, NULL);
	}
#endif
	
	if (in_thread == SP_TRUE) {
	    swLockWindowMutex(window);
	    if ((thread_entered = window->thread_entered) == SP_FALSE) {
		window->thread_entered = SP_TRUE;
	    }
	    swUnlockWindowMutex(window);
	    /*spDebug(10, "waveProcessCallback", "in_thread = %d, thread_entered = %d\n",
	      in_thread, thread_entered);*/
	    
	    if (thread_entered == SP_FALSE) {
		spThreadEnter();
	    }
	}
    
	if (edit_type == SW_EDIT_NONE && pos == SW_PROCESS_REACH_END){
	    spDebug(30, "waveProcessCallback",
		    "call in final position: pos = %ld, play_offset = %ld, pause_cursor = %d\n",
		    pos, window->wave->play_offset, window->pause_cursor);
	    if (window->pause_cursor == SP_TRUE) {
		swLockWindowMutex(window);
		swUpdatePoint(window, window->wave->play_offset, SP_TRUE);
		window->point_d = swSampToDisp(window, window->point);
		spDebug(30, "waveProcessCallback", "new point = %ld, sel_st = %ld\n",
			window->point, window->sel_st);
		swUnlockWindowMutex(window);
		swRefreshWindow(window, SP_TRUE, SP_TRUE);
	    }
	    window->current_play_pos = -1;
	} else if (pos == SW_PROCESS_FINISHED) {		/* process finished */
	    spDebug(30, "waveProcessCallback", "process finished: point_f = %f, prev_point_f = %f\n",
		    window->point_f, window->prev_point_f);
	    if (edit_type != SW_EDIT_NONE/* || window->pause_cursor == SP_FALSE*/) {
		swLockWindowMutex(window);
		window->point_f = window->prev_point_f;
		swUpdatePoint(window, swDimToSamp(window, window->point_f), SP_TRUE);
		window->point_d = swSampToDisp(window, window->point);
		swUnlockWindowMutex(window);
		swRefreshWindow(window, SP_TRUE, SP_TRUE);
	    }

	    swSetMouseCursor(window, SP_CURSOR_UNKNOWN);
	    swSetProcessSenseLevel(window, SP_FALSE);
	    
	    if (edit_type == SW_EDIT_PASTE || edit_type == SW_EDIT_INSERT
		|| edit_type == SW_EDIT_MIX || edit_type == SW_EDIT_REPLACE) {
		swLockMainMutex(window);
		window->config->toplevel->using_clipboard = SP_FALSE;
		swUnlockMainMutex(window);
	    }
	} else if (pos == SW_PROCESS_ERROR) {		/* process error */
	    spDebug(30, "waveProcessCallback", "error: edit_type = %d\n", edit_type);
	    
	    if (edit_type == SW_EDIT_NONE) {
		spDebug(30, "waveProcessCallback", "before spDisplayError\n");
		spDisplayError(window->window, NULL, SW_PLAY_ERROR_MESSAGE);
		spDebug(30, "waveProcessCallback", "after spDisplayError\n");
	    }
	} else if (pos == SW_PROCESS_STARTED) {
	    spDebug(80, "waveProcessCallback", "SW_PROCESS_STARTED: edit_type = %d\n", edit_type);
	    if (edit_type == SW_EDIT_PASTE || edit_type == SW_EDIT_INSERT 
		|| edit_type == SW_EDIT_MIX || edit_type == SW_EDIT_REPLACE) {
		swLockMainMutex(window);
		window->config->toplevel->using_clipboard = SP_TRUE;
		swUnlockMainMutex(window);
	    }
	    swLockWindowMutex(window);
	    spDebug(80, "waveProcessCallback", "process started: prev_point_f: %f --> %f\n",
		    window->prev_point_f, window->point_f);
	    window->prev_point_f = window->point_f;
	    swUnlockWindowMutex(window);
	    
	    spDebug(30, "waveProcessCallback", "process started\n");
	    if (wave != NULL && wave->core->play_flag == SP_FALSE) {
		swSetMouseCursor(window, SP_CURSOR_WAIT);
	    }
	    swSetProcessSenseLevel(window, SP_TRUE);
	} else {
	    spLong prev_play_pos = -1;
	    spDebug(80, "waveProcessCallback", "processing... %ld, edit_type = %d\n", pos, edit_type);
	    if (edit_type == SW_EDIT_NONE) {
		prev_play_pos = window->current_play_pos;
		window->current_play_pos = pos;
	    }
	    
	    if (edit_type != SW_EDIT_NONE || window->config->play_draw == SP_TRUE) {
		swWaveSubArea sub_area;
		
		sub_area = swGetWaveSubArea(window, wave);
                spDebug(100, "waveProcessCallback", "sub_area = %lx, window->specgram = %lx\n",
                        (unsigned long)sub_area, (unsigned long)window->specgram);

		if (sub_area == NULL) {
                    if (edit_type == SW_EDIT_SPECTROGRAM) {
			point_d = swSampToDisp(window, pos);
			point_f = swSampToDim(window, pos);
                        swLockWindowMutex(window);
                        swUpdatePoint(window, pos, SP_TRUE);
                        window->point_d = point_d;
                        window->point_f = point_f;
                        swUnlockWindowMutex(window);
                        swRefreshWindow(window, SP_TRUE, SP_TRUE);
                    }
		} else if (sub_area != NULL) {
		    target_wave_length = sub_area->wave->length;
		    drawn_pos = sub_area->drawn_pos;
		
		    spDebug(80, "waveProcessCallback", "drawn_pos = %ld, target_wave_length = %ld\n",
                            drawn_pos, target_wave_length);
		
		    if (edit_type >= SW_EDIT_GENERATE_RECORDING && edit_type <= SW_EDIT_GENERATE_WHITE_NOISE) {
			pos2 = pos / wave->thin_length;
			if (pos2 > 0) {
			    /*swDrawWaveImage(window->image, window, pos2);*/
			    {
				swDrawWaveSubAreaImage(window->image, window, sub_area, pos2, 0.0, 0.0,
                                                       swIsLastWaveSubArea(window, sub_area) == SP_TRUE
                                                       ? window->draw_height_horizontal_keys : 0.0);
			    }
			    swRefreshWindow(window, SP_FALSE, SP_TRUE);
			}
		    } else if (drawn_pos >= target_wave_length) {	/* if drawing has been completed */
			point_d = swSampToDisp(window, pos);
			point_f = swSampToDim(window, pos);
		    
			/*spDebug(80, "waveProcessCallback", "point_f = %f, window->point_f = %f\n",
                          point_f, window->point_f);*/
		    
			if (window->prev_point_f != window->point_f
			    && point_d >= window->point_d
			    && point_f - window->point_f < SW_MIN_DRAW_INTERVAL_F) {
			    /* do nothing */
			} else {
			    if (window->sync_play == SP_TRUE
				/*&& swIsWaveThreadSafe(wave) == SP_TRUE */
				&& (point_d < 0 || point_d >= window->width)) {
#if 0
				swScrollWindow(window, pos, /*SP_TRUE*/SP_FALSE, in_thread);
#else
				swScrollWindowEx(window, 0, pos, SP_FALSE, SP_TRUE, in_thread);
#endif
			    
				swLockWindowMutex(window);
				/*spDebug(80, "waveProcessCallback", "prev_point_f: %f --> %f\n",
				  window->prev_point_f, window->point_f);*/
				window->prev_point_f = window->point_f;
				swUnlockWindowMutex(window);
			    } else {
				swLockWindowMutex(window);
				swUpdatePoint(window, pos, SP_TRUE);
				window->point_d = point_d;
				window->point_f = point_f;
				swUnlockWindowMutex(window);
				swRefreshWindow(window, SP_TRUE, SP_TRUE);
			    }
			}
		    }
		}
	    }
	}
	
	if (window->execute_save_by_label == SP_TRUE) {
	    swProcessSaveByLabel(window, wave, pos, edit_type);
	}

	if (in_thread == SP_TRUE) {
	    if (thread_entered == SP_FALSE) {
		spThreadLeave();

		swLockWindowMutex(window);
		window->thread_entered = SP_FALSE;
		swUnlockWindowMutex(window);
		spDebug(10, "waveProcessCallback", "spThreadLeave called\n");
	    }

	    spYieldThread();
	} else {
	    if (dispatch_flag == SP_TRUE) {
		spDebug(10, "waveProcessCallback", "call spDispatchEvent\n");
		spDispatchEvent(window->config->toplevel->toplevel);
	    }
	}
	
	/*spDebug(80, "waveProcessCallback", "done\n");*/
    }
    
    return SP_TRUE;
}

static spBool wavePlayCallback(swWave wave, spBool in_thread, spLong pos, void *data)
{
    return waveProcessCallbackEx(wave, in_thread, SP_TRUE, pos, SW_EDIT_NONE, data);
}

static spBool waveEditCallback(swWave wave, spBool in_thread, spLong pos, swEditType edit_type, void *data)
{
    return waveProcessCallbackEx(wave, in_thread, SP_TRUE, pos, edit_type, data);
}

static spBool waveEditFinishCallbackEx(swWave wave, swWave owave, spBool in_thread, spBool dispatch_flag,
				       swEditType edit_type, void *data)
{
    spBool flag;
    spBool thread_entered = SP_FALSE;
    swWindow window = (swWindow)data;

    if (window == NULL) return SP_TRUE;
    
    spDebug(30, "waveEditFinishCallback", "in_thread = %d, edit_type = %d\n",
	    in_thread, edit_type);

#ifdef SW_USE_POST_EVENT
    if (in_thread == SP_TRUE) {
	return swPostUserEvent(window, SW_WAVE_EDIT_FINISH_EVENT, 0, edit_type, owave);
    }
#endif
	
    if (in_thread == SP_TRUE) {
	swLockWindowMutex(window);
	if ((thread_entered = window->thread_entered) == SP_FALSE) {
	    window->thread_entered = SP_TRUE;
	}
	swUnlockWindowMutex(window);
	spDebug(10, "waveEditFinishCallback", "in_thread = %d, thread_entered = %d\n",
		in_thread, thread_entered);
	
	if (thread_entered == SP_FALSE) {
	    spThreadEnter();
	}
    }
    
    flag = swEditFinish(window, owave, in_thread, edit_type);
    
    if (in_thread == SP_TRUE) {
	if (thread_entered == SP_FALSE) {
	    spThreadLeave();
	    
	    swLockWindowMutex(window);
	    window->thread_entered = SP_FALSE;
	    swUnlockWindowMutex(window);
	    spDebug(10, "waveEditFinishCallback", "spThreadLeave called\n");
	}

	spYieldThread();
    } else {
	if (dispatch_flag == SP_TRUE) {
	    spDispatchEvent(window->config->toplevel->toplevel);
	}
    }

    spDebug(10, "waveEditFinishCallback", "done: flag = %d\n", flag);
    
    return flag;
}

static spBool waveEditFinishCallback(swWave wave, swWave owave, spBool in_thread, swEditType edit_type, void *data)
{
    return waveEditFinishCallbackEx(wave, owave, in_thread, SP_TRUE, edit_type, data);
}

static spBool waveReadCallbackEx(swWave wave, spBool in_thread, spBool dispatch_flag, spLong pos, void *data)
{
    spLong pos2;
    spBool thread_entered = SP_FALSE;
    swWindow window = (swWindow)data;

    spDebug(80, "waveReadCallback", "in_thread = %d, pos = %ld\n", in_thread, (long)pos);

    if (window == NULL) return SP_TRUE;

#ifdef SW_USE_POST_EVENT
    if (in_thread == SP_TRUE) {
	return swPostUserEvent(window, SW_WAVE_READ_EVENT, pos, SW_EDIT_NONE, NULL);
    }
#endif

    if (in_thread == SP_TRUE) {
	swLockWindowMutex(window);
	if ((thread_entered = window->thread_entered) == SP_FALSE) {
	    window->thread_entered = SP_TRUE;
	}
	swUnlockWindowMutex(window);
	spDebug(10, "waveReadCallback", "in_thread = %d, thread_entered = %d\n",
		in_thread, thread_entered);
	
	if (thread_entered == SP_FALSE) {
	    spThreadEnter();
	}
    }
    
    if (pos == SW_PROCESS_FINISHED) {
	pos2 = wave->length;
    } else {
	pos2 = pos;
    }
    spDebug(80, "waveReadCallback", "pos2 = %ld\n", pos2);
#if 0
    swDrawWaveImage(window->image, window, pos2);
#else
    {
	swWaveSubArea sub_area;
        double draw_height_horizontal_keys;

	sub_area = swGetWaveSubArea(window, wave);
        
        if (window->config->horizontal_piano_keys_top == SP_TRUE) {
            draw_height_horizontal_keys = (swIsFirstWaveSubArea(window, sub_area) == SP_TRUE
                                           ? window->draw_height_horizontal_keys : 0.0);
        } else {
            draw_height_horizontal_keys = (swIsLastWaveSubArea(window, sub_area) == SP_TRUE
                                           ? window->draw_height_horizontal_keys : 0.0);
        }

	swDrawWaveSubAreaImage(window->image, window, sub_area, pos2, 0.0, 0.0, draw_height_horizontal_keys);
    }
#endif

    if (pos == SW_PROCESS_STARTED) {
	swSetMouseCursor(window, SP_CURSOR_WAIT);
	spSetSensitive(window->window, SP_FALSE);
	
	if (window->pause_cursor == SP_FALSE) {
	    swLockWindowMutex(window);
	    spDebug(80, "waveReadCallback", "prev_point_f: %f --> %f\n",
		    window->prev_point_f, window->point_f);
	    window->prev_point_f = window->point_f;
	    swUnlockWindowMutex(window);
	}
    } else if (pos == SW_PROCESS_FINISHED) {
	swDrawOverview(window, SP_TRUE);
	spDebug(80, "waveReadCallback", "SW_PROCESS_FINISHED\n");
	swSetMouseCursor(window, SP_CURSOR_UNKNOWN);
	spSetSensitive(window->window, SP_TRUE);
    }
    
    if (pos2 >= 0) {
	/* refresh canvas */
	swRefreshWindow(window, SP_FALSE, SP_FALSE);
    }

    if (in_thread == SP_TRUE) {
	if (thread_entered == SP_FALSE) {
	    spThreadLeave();
	    
	    swLockWindowMutex(window);
	    window->thread_entered = SP_FALSE;
	    swUnlockWindowMutex(window);
	    spDebug(10, "waveReadCallback", "spThreadLeave called\n");
	}

	spYieldThread();
    } else {
	spDebug(80, "waveReadCallback", "call spDispatchEvent\n");
	if (dispatch_flag == SP_TRUE) {
	    spDispatchEvent(window->config->toplevel->toplevel);
	}
    }
    
    spDebug(80, "waveReadCallback", "done\n");
    
    return SP_TRUE;
}

static spBool waveReadCallback(swWave wave, spBool in_thread, spLong pos, void *data)
{
    return waveReadCallbackEx(wave, in_thread, SP_TRUE, pos, data);
}

static spBool waveThreadWaitCallback(swWave wave, void *thread, void *data)
{
    swWindow window = (swWindow)data;
    
    spDebug(80, "waveThreadWaitCallback", "in\n");

    spWaitThreadLoop(window->config->toplevel->toplevel, thread);
    
    spDebug(80, "waveThreadWaitCallback", "done\n");
    
    return SP_TRUE;
}

#ifdef SW_USE_POST_EVENT
spBool swEventCB(spComponent component, spEventType event_type, void *data)
{
    spBool cb_return = SP_TRUE;
    spBool need_event_wait = SP_TRUE;
    swWindow window;
    swPostEventData post_event_data = (swPostEventData)data;

    window = post_event_data->window;

    if (window != NULL) {
	spDebug(80, "swEventCB", "in: event_type = %lx, cb_pos = %ld\n", event_type, (long)post_event_data->cb_pos);
#if 1
	if ((event_type == SW_WAVE_PROCESS_EVENT
             && !(post_event_data->cb_pos == SW_PROCESS_STARTED || post_event_data->cb_pos == SW_PROCESS_FINISHED
                  || post_event_data->cb_pos == SW_PROCESS_REACH_END || post_event_data->cb_pos == SW_PROCESS_ERROR))
            || (event_type == SW_WAVE_ERROR_EVENT && (int)post_event_data->cb_pos == SW_ERROR_GENERATE_STOPPED)) {
	    need_event_wait = SP_FALSE;
	}
#endif
	spDebug(80, "swEventCB", "need_event_wait = %d\n", need_event_wait);
	
	if (event_type == SW_WAVE_READ_EVENT) {
	    cb_return = waveReadCallbackEx(window->wave, SP_FALSE, SP_FALSE, post_event_data->cb_pos, window);
	} else if (event_type == SW_WAVE_OPTION_EVENT) {
	    cb_return = waveOptionCallbackEx(window->wave, SP_FALSE, SP_FALSE, (spOptions)post_event_data->cb_data, window);
	} else if (event_type == SW_WAVE_ERROR_EVENT) {
	    cb_return = waveErrorCallbackEx(window->wave, SP_FALSE, SP_FALSE, (int)post_event_data->cb_pos, post_event_data->cb_edit_type, window);
	} else if (event_type == SW_WAVE_PROCESS_EVENT) {
	    cb_return = waveProcessCallbackEx(window->wave, SP_FALSE, SP_FALSE, post_event_data->cb_pos, post_event_data->cb_edit_type, window);
	} else if (event_type == SW_WAVE_EDIT_FINISH_EVENT) {
	    cb_return = waveEditFinishCallbackEx(window->wave, (swWave)post_event_data->cb_data, SP_FALSE, SP_FALSE, post_event_data->cb_edit_type, window);
	}

	spDebug(80, "swEventCB", "cb_return = %d (event_type = %lx, cb_pos = %ld), need_event_wait = %d\n",
		cb_return, event_type, (long)post_event_data->cb_pos, need_event_wait);
	
	if (need_event_wait == SP_TRUE) {
	    window->cb_return = cb_return;
	    if (event_type == SW_WAVE_EDIT_FINISH_EVENT) {
		swSetEvent((swWave)post_event_data->cb_data);
	    } else {
		swSetEvent(window->wave);
	    }
	}
    }

    xfree(post_event_data);
    
    spDebug(100, "swEventCB", "done: event_type = %lx\n", event_type);
	
    return SP_TRUE;
}
#endif

void swInitialize(spTopLevel toplevel, swConfig config)
{
    swInitTopLevel(toplevel, config);

    swSetColor(config);

    config->wave_config->thread_safe = spIsThreadSafe();

    config->time_format = MIN(config->time_format, SW_TIME_FORMAT_SEPARATED_SEC);

    if (strnone(config->def_format_string)) {
	strcpy(config->def_format_string, "raw");
    }
    if (strnone(config->format_string)) {
	spStrCopy(config->format_string, SP_MAX_SETUP_VALUE, config->def_format_string);
    } else {
	config->format_specified = SP_TRUE;
    }
    if (config->samp_rate <= 0.0) {
	config->samp_rate = config->def_samp_rate;
    } else {
	config->format_specified = SP_TRUE;
    }
    if (config->head_size < 0) {
	config->head_size = config->def_head_size;
    } else {
	config->format_specified = SP_TRUE;
    }
    if (config->num_channel <= 0) {
	config->num_channel = config->def_num_channel;
    } else {
	config->format_specified = SP_TRUE;
    }
    if (config->samp_bit <= 0) {
	config->samp_bit = config->def_samp_bit;
    } else {
	config->format_specified = SP_TRUE;
    }

#ifdef SW_USE_ANALYSIS
    if (!strnone(config->analysis_type_string)) {
	config->analysis_type = swGetAnalysisTypeFromLabel(config->analysis_type_string);
	if (config->analysis_type == SW_ANALYSIS_NONE) {
	    config->analysis_type = SW_ANALYSIS_SPECTRUM;
	}
    } else {
	config->analysis_type = SW_ANALYSIS_SPECTRUM;
    }
    if (!strnone(config->window_type_string)) {
	config->window_type = swGetWindowTypeFromLabel(config->window_type_string);
	if (config->window_type == SW_WINDOW_NONE) {
	    config->window_type = SW_WINDOW_HAMMING;
	}
    } else {
	config->window_type = SW_WINDOW_HAMMING;
    }
#endif

    if (!strnone(config->play_command)) {
	spSetPlayCommand(config->play_command);
	config->wave_config->play_use_audio  =
	    (config->use_play_command == SP_TRUE ? SP_FALSE : SP_TRUE);
    }
    config->wave_config->audio_buffer_size = config->audio_buffer_size;

    swSetTempDir(config->wave_config, config->temp_dir);
    
    swSetWaveOptionCallback(config->wave_config, waveOptionCallback);
    swSetWaveErrorCallback(config->wave_config, waveErrorCallback);
    swSetWavePlayCallback(config->wave_config, wavePlayCallback);
    swSetWaveReadCallback(config->wave_config, waveReadCallback);
    swSetWaveEditCallback(config->wave_config, waveEditCallback);
    swSetWaveEditFinishCallback(config->wave_config, waveEditFinishCallback);
    swSetWaveThreadWaitCallback(config->wave_config, waveThreadWaitCallback);
    
    swCreateFormatDialog(config);
#ifdef SW_SUPPORT_PROPERTY_DIALOG
    swCreatePropertyDialog(config);
#endif
    swCreatePreferenceDialog(config);
    swCreatePrintCanvas(config, SP_DRAWING_PLUGIN_PRINTER);
    swCreateClipboardWindow(config);

    spSetPluginLang(spGetLanguage());

    return;
}

swDataType swGetDataType(swConfig config)
{
    if (config != NULL
	&& (strcaseeq(config->format_string, "freq")
	    || strcaseeq(config->format_string, SP_WAVE_FORMAT_TEXT_FREQ_LABEL))) {
	return SW_FREQ_DATA;
    }
    
    return SW_TIME_DATA;
}

void swCreateWindow(const char *filename, const char *label_file, spBool is_region_label,
		    double offset_s, double length_s, swConfig config)
{
    char *format;
    swDataType data_type = SW_TIME_DATA;
    swWave wave = NULL;
    swWindow window = NULL;
    spPluginError err;

    if (!strnone(filename)) {
	if (spIsFile(filename) == SP_FALSE) {
	    spDisplayError(NULL, SW_ERROR_TITLE, SW_FILE_NOT_EXIST_ERROR_MESSAGE, filename);
	    return;
	}
	
	if (config->use_def_format == SP_TRUE || config->format_specified == SP_TRUE) {
	    format = config->format_string;
	} else {
	    format = NULL;
	}
	
	if ((wave = swGetWave(config->wave_config, NULL, filename, NULL, format, config->samp_bit,
			      config->num_channel, config->samp_rate, config->num_order,
			      SP_TRUE, config->draw_detail, SP_TRUE, &err)) == NULL) {
	    spDebug(10, "swCreateWindow", "first swGetWave: err = %d\n", err);
	    
	    if (format != NULL ||
		(err != SP_PLUGIN_ERROR_SUITABLE_NOT_FOUND && err != SP_PLUGIN_ERROR_FAILURE)) {
		return;
	    } else {
		if (swPopupFormatDialog(config, filename) == SP_TRUE) {
		    data_type = swGetDataType(config);
		    spDebug(10, "swCreateWindow", "format_string = %s, type = %d\n",
			    config->format_string, data_type);
		    
		    if ((wave = swGetWave(config->wave_config, NULL, filename, NULL, config->format_string, config->samp_bit,
					  config->num_channel, config->samp_rate, config->num_order,
					  SP_TRUE, config->draw_detail, SP_FALSE, &err)) == NULL) {
			spDebug(10, "swCreateWindow", "second swGetWave: err = %d\n", err);
			spDisplayError(NULL, SW_ERROR_TITLE, SW_OPEN_ERROR_MESSAGE, filename);
			return;
		    }
		} else {
		    return;
		}
	    }
	}

        spDebug(100, "swCreateWindow", "swGetWave done\n");
        
	if (wave != NULL && !strnone(label_file)) {
	    spDebug(10, "swCreateWindow", "is_region_labe = %d, label_file = %s\n",
		    is_region_label, label_file);
	    swReadLabel(wave, label_file, SW_TIME_FORMAT_UNKNOWN, is_region_label);
	}
    }

    window = swCreateWaveWindow(wave, config, data_type, offset_s, length_s);
    
    spDebug(100, "swCreateWindow", "done\n");
    
    return;
}

void swUpdateMeterWidth(swWindow window)
{
    if (window == NULL) return;

    spDebug(80, "swUpdateMeterWidth", "original: window->meter_width = %d, window->draw_width = %f\n",
            window->meter_width, window->draw_width);
    
    if (window->data_type == SW_FREQ_DATA || swIsSpectrogramVisible(window) == SP_TRUE) {
	window->meter_width = 0;
    } else {
	if (window->config->display_meter == SP_TRUE) {
	    window->meter_width = SW_METER_WIDTH;
	} else {
	    window->meter_width = 0;
	}
    }
    window->draw_width = (double)swGetDrawWidth(window, SP_FALSE);
    window->vertical_keys_width = swCalcVerticalKeysWidth(window, SW_PIANO_KEYS_DEFAULT_SIZE, window->draw_width);
    
    spDebug(80, "swUpdateMeterWidth", "updated: window->meter_width = %d, window->draw_width = %f\n",
            window->meter_width, window->draw_width);

    return;
}

swWindow swInitWindow(swConfig config, swDataType data_type)
{
    swWindow window = NULL;

    window = xalloc(1, struct _swWindow);

    window->mutex = spCreateMutex(NULL);

    window->title = NULL;
    window->name = NULL;
    window->data_type = data_type;
    window->index = 1;
    window->scroll_coef = 1;
    if (data_type == SW_FREQ_DATA) {
	window->width = (config->freq_width > 0 ? config->freq_width :
			 SW_FREQ_WINDOW_WIDTH);
	window->height = (config->freq_height > 0 ? config->freq_height :
			  SW_FREQ_WINDOW_HEIGHT);
    } else {
	window->width = (config->width > 0 ? config->width : SW_WINDOW_WIDTH);
	window->height = (config->height > 0 ? config->height : SW_WINDOW_HEIGHT);
    }
    window->overview_width = 0;
    window->overview_height = 0;
    window->meter_width = 0;
    window->draw_width = 0.0;
    window->vertical_keys_width = 0.0;
    window->draw_height_horizontal_keys = 0.0;
    window->current_play_pos = -1;
    
    window->amp_min = 0.0;
    window->amp_max = -1.0;
    
    window->selecting = SP_FALSE;
    window->sel_st = -1;
    window->sel_ed = -1;
    window->sel_st_d = -1;
    window->sel_ed_d = -1;
    window->point = -1;
    window->point_d = -1;
    window->point_f = -1.0;
    window->prev_point_f = -1.0;
    window->target_sub_area = NULL;
    window->target_channel = 0;
    window->target_order = 0;
    window->active_label_index = -1;
    
    window->offset = 0;
    window->length = 0;
    window->wave = NULL;
    
    window->cb_return = SP_TRUE;
    
    window->specgram_config_flag = SW_ANALYSIS_CONFIG_FLAG_NONE;
    window->specgram_analysis_type = SW_ANALYSIS_SPECTRUM;
    window->specgram = NULL;
#ifdef SW_SUPPORT_ANALYSIS_SUBPLOT
    window->subplot_sgram = window->subplot_f0 = window->subplot_power = SP_FALSE;
    window->f0 = NULL;
    window->power = NULL;
#ifdef SW_SUPPORT_STRAIGHT
    window->subplot_straight_f0 = window->subplot_straight_sgram = window->subplot_straight_ap = SP_FALSE;
    window->straight_f0 = window->straight_sgram = window->straight_ap = NULL;
#endif
#endif

    window->num_sub_area = 0;
    /*window->sub_areas[0].wave = NULL;*/
    window->first_sub_area = NULL;
    window->last_sub_area = NULL;

    window->visible_flag = SP_FALSE;
    window->analysis_flag = SP_FALSE;
    window->loop_play = config->use_loop_play;
    window->sync_play = config->use_sync_play;
    window->pause_cursor = config->pause_cursor;
    window->drag_region = SP_FALSE;
    window->draw_label = SP_TRUE;
    window->draw_specgram = SP_FALSE;
    window->draw_vertical_keys = SP_FALSE;
    window->draw_horizontal_keys = SP_FALSE;
    window->draw_detail = config->draw_detail;
    window->log_frequency_axis = config->log_frequency_axis;
    if (data_type == SW_FREQ_DATA) {
	window->display_info_area = config->display_freq_info_area;
    } else {
	window->display_info_area = config->display_info_area;
    }
    window->autosave_started = SP_FALSE;
    window->thread_entered = SP_FALSE;
    window->process_flag = SP_FALSE;
    window->num_blocked = 0;
    window->drag_label_type = SW_DRAG_NO_LABEL;

#if defined(SW_AH_CUSTOM)
    window->link_ah_file = SP_FALSE;
#endif
    
    window->label_caps = SW_LABEL_CAPS_ALL;
    window->mouse_mode = SW_MOUSE_MODE_NORMAL;
    
    window->execute_save_by_label = SP_FALSE;
    window->save_by_label_wave = NULL;
    
    window->pointer = 0;
    window->direction = 1;
    window->config = config;
    window->window = NULL;
    window->wave_box = NULL;
    window->overview_canvas = NULL;
    window->canvas = NULL;
    window->image = NULL;
    window->hscroll = NULL;
    window->vscroll = NULL;
    window->tool_bar = NULL;
    window->drawing_plugin_menu = NULL;
    window->undo_menu = NULL;
    window->redo_menu = NULL;
    window->time_format_menu = NULL;
    window->specgram_menu = NULL;
    window->info_box = NULL;
    window->label_list = NULL;
    window->related_window = NULL;

#if defined(SW_AH_CUSTOM)
    window->toggle_ah_mode_menu;
    window->link_ah_file_menu;
#endif
    
#ifdef SW_SUPPORT_MORPHING
    window->time_anchors = NULL;
    window->drag_anchor = NULL;
#endif    

    swUpdateMeterWidth(window);

    return window;
}

void swInitVscroll(swWindow window)
{
    if (window != NULL) {
	window->amp_min = 0.0;
	window->amp_max = -1.0;
	if (window->vscroll != NULL) {
	    spSetParams(window->vscroll,
			SppMinimum, 0,
			SppMaximum, 10000,
			SppIncrement, 1000,
			SppPageIncrement, 10000,
			SppSliderSize, 10000,
			SppValue, 0,
			NULL);
	}
    }
    
    spDebug(80, "swInitVscroll", "done\n");
    
    return;
}

void swUpdateVscroll(swWindow window)
{
    int size, value;
    double data_min, data_max;
    double data_range;
    double amp_range;
    swWave wave;
    
    if (window != NULL) {
	if (window->vscroll != NULL) {
            spDebug(50, "swUpdateVscroll", "window->amp_min = %f, window->amp_max = %f\n",
                    window->amp_min, window->amp_max);
            
	    if (window->wave == NULL || window->amp_max <= window->amp_min) {
		swInitVscroll(window);
	    } else {
                spBool y_log_flag = SP_FALSE;
                
		wave = swGetTargetWave(window);		
                if (window->log_frequency_axis == SP_TRUE && wave->order_frequency_flag == SP_TRUE) {
                    y_log_flag = SP_TRUE;
                }
		
		data_range = swGetDataHeight(window, wave, y_log_flag, &data_min, &data_max);
                spDebug(50, "swUpdateVscroll", "y_log_flag = %d, data_min = %f, data_max = %f, data_range = %f\n",
                        y_log_flag, data_min, data_max, data_range);

                if (y_log_flag == SP_TRUE) {
                    double log_amp_min, log_amp_max;
                    double log_amp_range;

                    log_amp_min = log10(MAX(window->amp_min, SW_LOG_FREQUENCY_MIN_VALUE));
                    log_amp_max = log10(MAX(window->amp_max, SW_LOG_FREQUENCY_MIN_VALUE));
                    log_amp_range = log_amp_max - log_amp_min;
                    spDebug(50, "swUpdateVscroll", "log_amp_min = %f, log_amp_max = %f, log_amp_range = %f\n",
                            log_amp_min, log_amp_max, log_amp_range);
                
                    size = (int)round(10000.0 * log_amp_range / data_range);
                    value = (int)round(10000.0 * (data_max - log_amp_max) / data_range);
                } else {
                    amp_range = window->amp_max - window->amp_min;
                    spDebug(50, "swUpdateVscroll", "amp_range = %f\n", amp_range);
                
                    size = (int)round(10000.0 * amp_range / data_range);
                    value = (int)round(10000.0 * (data_max - window->amp_max) / data_range);
                }
                spDebug(50, "swUpdateVscroll", "size = %d, value = %d\n", size, value);
		
		spSetParams(window->vscroll,
			    SppIncrement, size / 4,
			    SppPageIncrement, size,
			    SppSliderSize, size,
			    SppValue, value,
			    NULL);
	    }
	}
    }
    
    spDebug(80, "swUpdateVscroll", "done\n");
    
    return;
}
     
void swSetWaveWithRegion(swWindow window, swWave wave, double offset_s, double length_s)
{
    if (window != NULL) {
	window->sel_st = -1;
	window->sel_ed = -1;
	window->sel_st_d = -1;
	window->sel_ed_d = -1;
#if 0
	window->point = -1;
	window->point_d = -1;
	window->point_f = -1.0;
	window->prev_point_f = -1.0;
#endif

	if (window->wave != wave) {
#ifdef SW_SUPPORT_MORPHING
	    if (window->time_anchors != NULL) {
		swAnchorsDestroy(window->time_anchors);
		window->time_anchors = NULL;
	    }
#endif

#ifdef SW_SUPPORT_ANALYSIS_SUBPLOT
	    swDestroyWindowWave(window, &window->f0);
	    swDestroyWindowWave(window, &window->power);
#endif
	    swDestroyWindowWave(window, &window->specgram);
	    swDestroyWindowWave(window, &window->wave);
	    
	    if (window->related_window != NULL
		&& window->related_window->related_window == window) {
		window->related_window->related_window = NULL;
	    }
	    window->related_window = NULL;
	}
	
	if (wave != NULL) {
	    if (window->data_type != SW_FREQ_DATA) {
		if (offset_s > 0.0) {
		    window->offset = (spLong)((offset_s * wave->samp_rate) + 0.5);
		    if (window->offset >= wave->total_length - 1) {
			window->offset = 0;
		    }
		}
		if (length_s > 0.0) {
		    window->length = (spLong)((length_s * wave->samp_rate) + 0.5);
		}
	    }
	    if (window->offset <= 0) {
		window->offset = 0;
	    } else {
		window->offset = MIN(window->offset, wave->total_length - 1);
	    }
	    if (window->length <= 0) {
		window->length = MAX(wave->total_length - window->offset, 1);
	    } else {
		window->length = MIN(window->length, wave->total_length - window->offset);
	    }
	    spDebug(80, "swSetWaveWithRegion", "window->offset = %ld, window->length = %ld, total_length = %ld\n",
		    window->offset, window->length, wave->total_length);
	    
	    window->wave = wave;
	    if (window->draw_specgram == SP_TRUE && window->specgram != NULL) {
		swSetWaveToFirstSubArea(window, window->specgram, SP_TRUE);
	    } else {
		swSetWaveToFirstSubArea(window, window->wave, SP_FALSE);
	    }
	    swUpdateWaveSubAreaSize(window);
	    
	    swSetWaveCallbackData(window->wave, (void *)window);
	    if (window->specgram != NULL) {
		swSetWaveCallbackData(window->specgram, (void *)window);
	    }
#ifdef SW_SUPPORT_ANALYSIS_SUBPLOT
	    if (window->f0 != NULL) {
		swSetWaveCallbackData(window->f0, (void *)window);
	    }
	    if (window->power != NULL) {
		swSetWaveCallbackData(window->power, (void *)window);
	    }
#endif
	    
	    window->config->toplevel->null_window = NULL;
	} else {
	    window->offset = 0;
	    window->length = 0;
	    window->wave = NULL;
	}
    }

    spDebug(10, "swSetWaveWithRegion", "done\n");
    
    return;
}

void swSetWave(swWindow window, swWave wave)
{
    spDebug(10, "swSetWave", "in\n");
    swSetWaveWithRegion(window, wave, -1.0, -1.0);
    return;
}

void swGetWindowIndex(swWindow window)
{
    long k;
    int index = 1;
    spComponent next = NULL;
    swWindow next_window = NULL;

    if (window == NULL) return;

    next = window->window;
    for (k = 0;; k++) {
	next = spGetNextWindow(next, SP_FALSE);
	if (next == NULL || next == window->window) {
	    break;
	}
	
	if ((next_window = (swWindow)spGetUserData(next)) != NULL
	    && streq(next_window->name, window->name)) {
	    index = MAX(next_window->index + 1, index);
	}
    }

    window->index = index;
    
    return;
}

void swSetWindowTitle(swWindow window)
{
    char *path, *string;
    char samp_bit_string[SP_MAX_LINE];
    char name[SP_MAX_MESSAGE];
    char title[SP_MAX_MESSAGE];

    if (window == NULL)
	return;

    if (window->wave != NULL) {
	path = NULL;
	if (/*window->wave->orig_flag == SP_TRUE
	      &&*/ (path = xspGetReadablePath(window->wave->core->orig_filename)) != NULL) {
	    string = spGetBaseName(path);
	} else {
	    string = NULL;
	}

	if (window->name == NULL
	    || (string != NULL && !streq(string, window->name))
	    || streq(window->name, SW_NO_WAVE_TITLE)) {
	    if (window->name != NULL) {
		xfree(window->name);
	    }
	    if (swIsClipboardWindow(window) == SP_TRUE) {
		window->name = strclone(SW_CLIPBOARD_TITLE);
	    } else if (string == NULL) {
		window->name = strclone(SW_UNTITLED_TITLE);
	    } else {
		window->name = strclone(string);
	    }	
	    spDebug(10, "swSetWindowTitle", "window name: %s\n", window->name);
	    
	    swGetWindowIndex(window);
	}

	if (path != NULL) xfree(path);
	
	if (window->index >= 2) {
	    sprintf(name, "%s<%d>", window->name, window->index);
	} else {
	    strcpy(name, window->name);
	}
	if (swIsWaveEdited(window->wave) == SP_TRUE) {
	    strcat(name, " *");
	}

	swGetSampleBitString(samp_bit_string, swGetWaveSampleBit(window->wave));

	sprintf(title,
		"%s  (Sampling Frequency: %.1f Hz  Data Format: %s  %s Bits/Sample)", 
		name, swGetWaveSampleRate(window->wave),
		swGetWaveFileType(window->wave), samp_bit_string);

	spDebug(10, NULL, "title = %s, name = %s\n", title, name);

	/* change title */
	spSetParams(window->window, SppTitle, title, SppIconName, name, NULL);
    } else {
        spDebug(10, "swSetWindowTitle", "no wave\n");
        
	if (window->name == NULL) {
	    if (swIsClipboardWindow(window) == SP_TRUE) {
		window->name = strclone(SW_CLIPBOARD_TITLE);
	    } else {
		window->name = strclone(SW_NO_WAVE_TITLE);
	    }
	}
	
	/* set window title */
	strcpy(title, window->name);

	/* change title */
	spSetParams(window->window, SppTitle, title, SppIconName, title, NULL);
    }

    spDebug(10, "swSetWindowTitle", "done\n");
    
    return;
}

void swDestroyWindow(swWindow window)
{
    spDebug(10, "swDestroyWindow", "in\n");
    
    if (window != NULL) {
	if (window == window->config->toplevel->clipboard_window) {
	    window->config->toplevel->clipboard_window = NULL;
	}
	if (window->title != NULL) xfree(window->title);
	if (window->name != NULL) xfree(window->name);
#ifdef SW_SUPPORT_MORPHING
	if (window->time_anchors != NULL) swAnchorsDestroy(window->time_anchors);
#endif
#ifdef SW_SUPPORT_ANALYSIS_SUBPLOT
	if (window->f0 != NULL) swDestroyWave(window->f0);
	if (window->power != NULL) swDestroyWave(window->power);
#ifdef SW_SUPPORT_STRAIGHT
	if (window->straight_sgram != NULL) swDestroyWave(window->straight_sgram);
	if (window->straight_ap != NULL) swDestroyWave(window->straight_ap);
	if (window->straight_f0 != NULL) swDestroyWave(window->straight_f0);
#endif
#endif
	if (window->specgram != NULL) swDestroyWave(window->specgram);
	if (window->wave != NULL) swDestroyWave(window->wave);
	spDebug(10, "swDestroyWindow", "destroy wave done\n");

	if (window->mutex != NULL) spDestroyMutex(window->mutex);
    
	xfree(window);
    }

    spDebug(10, "swDestroyWindow", "done\n");
    
    return;
}

spDialogResponse swDisplayContinuePrompt(swWindow window, spBool close_flag)
{
    if (close_flag == SP_TRUE) {
	return spCreateMessageBox(window->window, NULL,
				  SW_SAVE_ERROR_CLOSE_QUESTION_MESSAGE,
				  SppDialogType, SP_WARNING_DIALOG,
				  SppMessageBoxButtonType, SP_MB_YES_NO,
				  NULL);
    } else {
	return spCreateMessageBox(window->window, NULL,
				  SW_SAVE_ERROR_CONTINUE_QUESTION_MESSAGE,
				  SppDialogType, SP_WARNING_DIALOG,
				  SppMessageBoxButtonType, SP_MB_YES_NO,
				  NULL);
    }
}

spDialogResponse swDisplaySaveChangesDialog(swWindow window, spBool label_flag, spBool region_flag)
{
    char *filename;
    char buf[SP_MAX_MESSAGE];

    if (label_flag == SP_FALSE) {
	if (window->name != NULL) {
	    filename = window->name;
	} else {
	    filename = spGetBaseName(swGetWaveOriginalFileName(window->wave));
	}
	if (strnone(filename)) {
	    return SP_DR_CANCEL;
	}
	sprintf(buf, SW_SAVE_CHANGES_QUESTION_MESSAGE, filename);
    } else {
	if (region_flag == SP_FALSE) {
	    filename = swGetLabelFileName(window->wave->labels);
	} else {
	    filename = swGetRegionLabelFileName(window->wave->labels);
	}
	if (strnone(filename)) {
	    if (region_flag == SP_FALSE) {
		strcpy(buf, SW_SAVE_LABEL_CHANGES_QUESTION_MESSAGE);
	    } else {
		strcpy(buf, SW_SAVE_REGION_LABEL_CHANGES_QUESTION_MESSAGE);
	    }
	} else {
	    sprintf(buf, SW_SAVE_CHANGES_QUESTION_MESSAGE, filename);
	}
    }

    return spCreateMessageBox(window->window, NULL, buf,
			      SppTitle, SW_SAVE_CHANGES_QUESTION_TITLE,
			      SppDialogType, SP_QUESTION_DIALOG,
			      SppMessageBoxButtonType, SP_MB_YES_NO_CANCEL,
			      NULL);
}

spBool swDisplayClosePrompt(swWindow window, spBool label_flag, spBool region_flag)
{
    spBool write_success;
    spDialogResponse response = SP_DR_YES;
    
    spMapWindow(window->window);
    spDebug(10, "swDisplayClosePrompt", "map window done\n");

    response = swDisplaySaveChangesDialog(window, label_flag, region_flag);
    spDebug(10, "swDisplayClosePrompt", "response = %d\n", response);
 
    if (response == SP_DR_CANCEL) {
	return SP_FALSE;
    } else if (response == SP_DR_YES) {
	if (label_flag == SP_TRUE) {
	    /* write label */
#if 0
	    write_success = swWriteLabel(window->wave, window->name,
					 filename, window->wave->labels->format, region_flag);
#else
	    write_success = swSaveLabel(window, SP_FALSE, region_flag);
#endif
	} else {
	    write_success = swWriteWave(NULL, NULL, NULL, NULL, window->wave, SP_TRUE, SP_FALSE);
	}
	
	if (write_success == SP_FALSE) {
	    if (swDisplayContinuePrompt(window, SP_TRUE) != SP_DR_YES) {
		return SP_FALSE;
	    }
	}
    }
    
    return SP_TRUE;
}

spBool swCloseWindowOrRevertToNullWindow(swWindow window, spBool quit_prompt, spBool can_revert_to_null_window)
{
    spComponent component;
    swConfig config;
    
    spDebug(10, "swCloseWindow", "in\n");
    
    if (window != NULL && window->window != NULL
	&& swIsVisible(window) == SP_TRUE) {
	config = window->config;

	if (window->wave != NULL && swIsClipboardWindow(window) == SP_FALSE
	    && !strnone(swGetWaveOriginalFileName(window->wave))) {
	    /* check if the file has been edited. */
	    if (swIsWaveEdited(window->wave) == SP_TRUE) {
		if (swDisplayClosePrompt(window, SP_FALSE, SP_FALSE) == SP_FALSE) {
		    return SP_FALSE;
		}
	    }
	    
	    if (swIsLabelEdited(window->wave->labels) == SP_TRUE) {
		if (swDisplayClosePrompt(window, SP_TRUE, SP_FALSE) == SP_FALSE) {
		    return SP_FALSE;
		}
	    }
	    if (swIsRegionLabelEdited(window->wave->labels) == SP_TRUE) {
		if (swDisplayClosePrompt(window, SP_TRUE, SP_TRUE) == SP_FALSE) {
		    return SP_FALSE;
		}
	    }
	}
	
	spDebug(10, "swCloseWindow", "num_window = %ld\n", window->config->toplevel->num_window);
        
        if (can_revert_to_null_window == SP_TRUE && window->config->toplevel->num_window <= 1
            && window->config->toplevel->null_window == NULL) {
            swDestroyWindowWave(window, &window->wave);
            spDebug(10, "swCloseWindow", "swDestroyWindowWave done: window->wave = %ld\n", (unsigned long)window->wave);
            
            window->config->toplevel->null_window = window;
            if (window->name != NULL) {
                xfree(window->name); window->name = NULL;
            }
            swSetWindowTitle(window);
            
	    swDrawBackground(window->image, window);
            swRefreshWindow(window, SP_FALSE, SP_TRUE);
            swSetSenseLevel(window);
        } else {
            if (quit_prompt == SP_TRUE && window->config->toplevel->num_window == 1) {
                if (spQuitPrompt(window->window) == SP_FALSE) {
                    return SP_FALSE;
                }
            }

            if (window == config->toplevel->current_window) {
                config->toplevel->current_window = NULL;
            }
	
            if (window->related_window != NULL
                && window->related_window->related_window == window) {
                window->related_window->related_window = NULL;
            }

            component = window->window;
            window->visible_flag = SP_FALSE;
	
            swLockMainMutex(window);
            --config->toplevel->num_window;
            swUnlockMainMutex(window);
	
            if (swIsClipboardWindow(window) == SP_TRUE) {
                spDebug(10, "swCloseWindow", "num_window = %ld\n",
                        window->config->toplevel->num_window);
                spPopdownWindow(window->window);
            } else {
                window->window = NULL;
                spDebug(10, "swCloseWindow", "call spCloseWindow\n");
                spCloseWindow(component);
                /* swDestroyWindow will be called in swDestroyWindowCB */
                /*swDestroyWindow(window);*/
            }
            
            if (config->toplevel->num_window == 0) {
#if 0
                if (config->toplevel->clipboard_window != NULL) {
                    swDestroyWindow(config->toplevel->clipboard_window);
                }
#endif
                spQuit(0);
            }

            swSetSenseLevel(NULL);
        }
    }

    spDebug(10, "swCloseWindow", "done\n");
    
    return SP_TRUE;
}

spBool swCloseWindow(swWindow window, spBool quit_prompt)
{
    return swCloseWindowOrRevertToNullWindow(window, quit_prompt, SP_FALSE);
}

static void setSelectDataSenseLevel(swWindow window, long group_id, spBool select)
{
    if (select == SP_TRUE) {
	if (window->data_type == SW_TIME_DATA
	    && window->analysis_flag == SP_FALSE) {
	    if (window->wave->selected_channel < 0) {
		if (window->wave->num_channel > 1) {
		    spSetWindowSenseLevel(window->window, group_id,
					  SW_STATE_SELECT_TIME_MULTI_CHANNEL);
		} else {
		    spSetWindowSenseLevel(window->window, group_id,
					  SW_STATE_SELECT_TIME_CHANNELS);
		}
	    } else {
		spSetWindowSenseLevel(window->window, group_id, SW_STATE_SELECT_TIME_DATA);
	    }
	} else {
	    spSetWindowSenseLevel(window->window, group_id, SW_STATE_FREQ_DATA);
	}
    } else {
	if (window->data_type == SW_TIME_DATA
	    && window->analysis_flag == SP_FALSE) {
	    spSetWindowSenseLevel(window->window, group_id, SW_STATE_TIME_DATA);
	} else {
	    spSetWindowSenseLevel(window->window, group_id, SW_STATE_FREQ_DATA);
	}
    }
    
    return;
}

void swSetSelectSenseLevel(swWindow window, spBool select)
{
    if (window == NULL || window->wave == NULL) return;
    
    if (swIsWaveProcessing(window->wave) == SP_TRUE || window->num_blocked > 0) {
	if (0 && window->sync_play == SP_TRUE && swIsWavePlaying(window->wave) == SP_TRUE) {
	    spSetWindowSenseLevel(window->window, SW_PAGE_GROUP_ID, SW_STATE_PLAY_WAVE);
	    spSetWindowSenseLevel(window->window, SP_DEFAULT_GROUP_ID, SW_STATE_PLAY_WAVE);
	} else if (window->num_blocked > 0) {
	    if (select == SP_TRUE) {
		spSetWindowSenseLevel(window->window, SW_PAGE_GROUP_ID, SW_STATE_SELECT_WAVE);
	    } else {
		spSetWindowSenseLevel(window->window, SW_PAGE_GROUP_ID, SW_STATE_EXIST_WAVE);
	    }
	    spSetWindowSenseLevel(window->window, SP_DEFAULT_GROUP_ID, SW_STATE_PLAY_WAVE);
	} else {
	    if (select == SP_TRUE) {
		spSetWindowSenseLevel(window->window, SW_PAGE_GROUP_ID, SW_STATE_SELECT_WAVE);
		spSetWindowSenseLevel(window->window, SP_DEFAULT_GROUP_ID, SW_STATE_SELECT_WAVE);
	    } else {
		spSetWindowSenseLevel(window->window, SW_PAGE_GROUP_ID, SW_STATE_EXIST_WAVE);
		spSetWindowSenseLevel(window->window, SP_DEFAULT_GROUP_ID, SW_STATE_EXIST_WAVE);
	    }
	}
	spSetWindowSenseLevel(window->window, SW_DATA_GROUP_ID, SW_STATE_PLAY_TIME_DATA);
	spSetWindowSenseLevel(window->window, SW_CLIPBOARD_DATA_GROUP_ID, SW_STATE_PLAY_TIME_DATA);
	spSetWindowSenseLevel(window->window, SW_CHANNEL_GROUP_ID, SW_STATE_PLAY_WAVE);
	
	spSetWindowSenseLevel(window->window, SW_CLIPBOARD_GROUP_ID, SW_STATE_NO_CLIPBOARD);
    } else {
	if (select == SP_TRUE) {
	    spSetWindowSenseLevel(window->window, SP_DEFAULT_GROUP_ID, SW_STATE_SELECT_WAVE);
	    spSetWindowSenseLevel(window->window, SW_PAGE_GROUP_ID, SW_STATE_SELECT_WAVE);
	} else {
	    spSetWindowSenseLevel(window->window, SP_DEFAULT_GROUP_ID, SW_STATE_EXIST_WAVE);
	    spSetWindowSenseLevel(window->window, SW_PAGE_GROUP_ID, SW_STATE_EXIST_WAVE);
	}

	if (swIsClipboardNone(window) == SP_TRUE
	    || swIsWaveProcessing(window->wave) == SP_TRUE
	    || window->num_blocked > 0) {
	    spSetWindowSenseLevel(window->window, SW_CLIPBOARD_GROUP_ID, SW_STATE_NO_CLIPBOARD);
	} else {
	    if (select == SP_TRUE) {
		spSetWindowSenseLevel(window->window, SW_CLIPBOARD_GROUP_ID, SW_STATE_EXIST_CLIPBOARD_SELECTED);
	    } else {
		spSetWindowSenseLevel(window->window, SW_CLIPBOARD_GROUP_ID, SW_STATE_EXIST_CLIPBOARD);
	    }
	}

	setSelectDataSenseLevel(window, SW_DATA_GROUP_ID, select);
	if (window->config->toplevel->using_clipboard == SP_TRUE) {
	    setSelectDataSenseLevel(window, SW_CLIPBOARD_DATA_GROUP_ID, SP_FALSE);
	} else {
	    setSelectDataSenseLevel(window, SW_CLIPBOARD_DATA_GROUP_ID, select);
	}
    }
    
    return;
}

void swSetProcessSenseLevel(swWindow window, spBool process)
{
    long num_blocked;
    spBool need_block;
    swWindow next_window;
    spCloseStyle close_style = SW_DEFAULT_CLOSE_STYLE;
    
    if (window == NULL) return;

    spDebug(80, "swSetProcessSenseLevel", "process = %d\n", process);

    if (process == SP_TRUE) {
	swLockMainMutex(window);
	if (window->config->toplevel->playable == SP_TRUE) {
	    if (swIsWavePlaying(window->wave) == SP_TRUE) {
		window->config->toplevel->playable = SP_FALSE;
	    }
	}
	window->config->toplevel->num_process++;
	swSetWindowsSenseLevel();
	swUnlockMainMutex(window);
		
	next_window = window;
	while (next_window != NULL) {
	    swLockMutex(next_window->wave);

	    need_block = swNeedWindowProcessBlock(window, next_window);
	    
	    if (need_block == SP_TRUE) {
		next_window->num_blocked++;
	    }
	    swUnlockMutex(next_window->wave);
		
	    spDebug(80, "swSetProcessSenseLevel",
		    "process: need_block = %d, num_process = %d, num_blocked = %d\n",
		    need_block, window->config->toplevel->num_process,
		    next_window->num_blocked);
	    
	    if (window->config->toplevel->num_process == 1 || need_block == SP_TRUE) {
		swSetWindowSenseLevel(next_window, SP_FALSE);

		if (next_window == window || next_window->num_blocked == 1) {
		    spSetParams(next_window->window,
				SppCloseStyle, SP_NO_CLOSE,
				NULL);
		}
	    }
		
	    next_window = swGetNextWindow(next_window);
	    if (next_window == window) {
		break;
	    }
	}
    } else {
	swLockMainMutex(window);
	if (window->config->toplevel->playable == SP_FALSE
	    && swIsWavePlaying(window->wave) == SP_TRUE) {
	    window->config->toplevel->playable = SP_TRUE;
	}
	window->config->toplevel->num_process =
	    MAX(window->config->toplevel->num_process - 1, 0);
	swSetWindowsSenseLevel();
	swUnlockMainMutex(window);

	/* because need_block changes after editing */
	num_blocked = window->num_blocked;
	
	next_window = window;
	while (next_window != NULL) {
	    if (num_blocked > 0) {
		need_block = SP_TRUE;
	    } else {
		need_block = swNeedWindowProcessBlock(window, next_window);
	    }
	    
	    swLockMutex(next_window->wave);
	    if (need_block == SP_TRUE) {
		next_window->num_blocked = MAX(next_window->num_blocked - 1, 0);
	    }
	    swUnlockMutex(next_window->wave);
	    
	    spDebug(80, "swSetProcessSenseLevel",
		    "no process: need_block = %d, num_process = %d, num_blocked = %d\n",
		    need_block, window->config->toplevel->num_process,
		    next_window->num_blocked);
	    
	    if ((need_block == SP_TRUE && next_window->num_blocked == 0)
		|| window->config->toplevel->num_process == 0) {
		swSetWindowSenseLevel(next_window, SP_FALSE);
		
		if (next_window == window || next_window->num_blocked == 0) {
		    spSetParams(next_window->window,
				SppCloseStyle, close_style,
				NULL);
		}
	    }
	    
	    next_window = swGetNextWindow(next_window);
	    if (next_window == window) {
		break;
	    }
	}
    }

    return;
}

void swSetWindowsSenseLevel(void)
{
    if (swGetNumWindow() >= 2) {
	spDebug(10, "swSetWindowsSenseLevel", "SW_STATE_SOME_WINDOWS\n");
	if (swGetNumProcess() > 0) {
	    spSetCurrentSenseLevel(SW_WINDOW_GROUP_ID, SW_STATE_SOME_WINDOWS);
	} else {
	    spSetCurrentSenseLevel(SW_WINDOW_GROUP_ID, SW_STATE_STOP_SOME_WINDOWS);
	}
    } else {
	spSetCurrentSenseLevel(SW_WINDOW_GROUP_ID, SW_STATE_ONE_WINDOW);
    }
    
    if (swGetNumProcess() > 0) {
	spSetCurrentSenseLevel(SW_QUIT_GROUP_ID, SW_STATE_PLAY_WAVE);
    } else {
	spSetCurrentSenseLevel(SW_QUIT_GROUP_ID, SW_STATE_ONE_WINDOW);
    }
    
    return;
}

void swSetUndoSenseLevel(swWindow window)
{
    if (window == NULL) return;
    
    if (window->wave == NULL || swIsWaveProcessing(window->wave) == SP_TRUE) {
	spSetSensitive(window->undo_menu, SP_FALSE);
	spSetSensitive(window->redo_menu, SP_FALSE);
    } else {
	if (swCanRedoWave(window->wave) == SP_TRUE) {
	    spSetSensitive(window->redo_menu, SP_TRUE);
	} else {
	    spSetSensitive(window->redo_menu, SP_FALSE);
	}
	if (swCanUndoWave(window->wave) == SP_TRUE) {
	    spSetSensitive(window->undo_menu, SP_TRUE);
	} else {
	    spSetSensitive(window->undo_menu, SP_FALSE);
	}
    }
    
    return;
}

void swSetWindowSenseLevel(swWindow window, spBool windows_flag)
{
    spBool process_flag = SP_FALSE;
    
    spDebug(10, "swSetWindowSenseLevel", "windows_flag = %d\n", windows_flag);
    
    if (windows_flag == SP_TRUE) {
	swSetWindowsSenseLevel();
	if (swIsClipboardWindow(window) == SP_TRUE) {
	    swWindow first_window, next_window;

	    first_window = swGetNextWindow(window);

	    if (first_window != NULL
		&& swIsClipboardWindow(first_window) == SP_FALSE) {
		next_window = first_window;
		while (next_window != NULL) {
		    swSetWindowSenseLevel(next_window, SP_FALSE);
		
		    next_window = swGetNextWindow(next_window);
		    if (next_window == first_window) {
			break;
		    }
		}
	    }
	}
    }

    if (window != NULL) {
	if (window->wave == NULL) {
	    if (/*window->config->toplevel->num_process > 0 ||*/
		window->num_blocked > 0) {
		spSetWindowSenseLevel(window->window, SP_DEFAULT_GROUP_ID, SW_STATE_PLAY_WAVE);
		spSetWindowSenseLevel(window->window, SW_PAGE_GROUP_ID, SW_STATE_PLAY_WAVE);
		spSetWindowSenseLevel(window->window, SW_OPEN_GROUP_ID, SW_STATE_PLAY_WAVE);
		spSetWindowSenseLevel(window->window, SW_SAVE_GROUP_ID, SW_STATE_PLAY_WAVE);
	    } else {
		spSetWindowSenseLevel(window->window, SP_DEFAULT_GROUP_ID, SW_STATE_NO_WAVE);
		spSetWindowSenseLevel(window->window, SW_PAGE_GROUP_ID, SW_STATE_NO_WAVE);
		spSetWindowSenseLevel(window->window, SW_OPEN_GROUP_ID, SW_STATE_NO_WAVE);
		spSetWindowSenseLevel(window->window, SW_SAVE_GROUP_ID, SW_STATE_NO_WAVE);
	    }
	    spSetWindowSenseLevel(window->window, SW_DATA_GROUP_ID, SW_STATE_NO_DATA);
	    spSetWindowSenseLevel(window->window, SW_CLIPBOARD_DATA_GROUP_ID, SW_STATE_NO_DATA);
	    spSetWindowSenseLevel(window->window, SW_CHANNEL_GROUP_ID, SW_STATE_NO_WAVE);
	    spSetWindowSenseLevel(window->window, SW_PLAY_GROUP_ID, SW_STATE_NO_WAVE);

	    spSetWindowSenseLevel(window->window, SW_LABEL_GROUP_ID, SW_STATE_NO_LABEL);
	    spSetWindowSenseLevel(window->window, SW_NORMAL_LABEL_GROUP_ID, SW_STATE_NO_LABEL);
	    spSetWindowSenseLevel(window->window, SW_REGION_LABEL_GROUP_ID, SW_STATE_NO_LABEL);
#ifdef SW_SUPPORT_MORPHING
	    spSetWindowSenseLevel(window->window, SW_ANCHOR_GROUP_ID, SW_STATE_NO_ANCHOR);
#endif
	    
	    spSetWindowSenseLevel(window->window, SW_CLIPBOARD_GROUP_ID, SW_STATE_NO_CLIPBOARD);
	} else {
	    if (/*window->config->toplevel->num_process > 0 ||*/
		swIsWaveProcessing(window->wave) == SP_TRUE
		|| window->num_blocked > 0) {
		process_flag = SP_TRUE;
		spSetWindowSenseLevel(window->window, SW_OPEN_GROUP_ID, SW_STATE_PLAY_WAVE);
		spSetWindowSenseLevel(window->window, SW_SAVE_GROUP_ID, SW_STATE_PLAY_WAVE);
	    } else {
		spSetWindowSenseLevel(window->window, SW_OPEN_GROUP_ID, SW_STATE_EXIST_WAVE);
		if (swIsWaveEdited(window->wave) == SP_TRUE) {
		    spSetWindowSenseLevel(window->window, SW_SAVE_GROUP_ID, SW_STATE_EDIT_WAVE);
		} else {
		    spSetWindowSenseLevel(window->window, SW_SAVE_GROUP_ID, SW_STATE_EXIST_WAVE);
		}
	    }
	    if (window->data_type == SW_TIME_DATA
		&& window->analysis_flag == SP_FALSE) {
		if (process_flag == SP_TRUE) {
		    spSetWindowSenseLevel(window->window, SW_CHANNEL_GROUP_ID, SW_STATE_PLAY_WAVE);
		} else {
		    if (window->wave->num_channel >= 3) {
			spSetWindowSenseLevel(window->window, SW_CHANNEL_GROUP_ID, SW_STATE_MULTI_CHANNEL_WAVE);
		    } else if (window->wave->num_channel >= 2) {
			spSetWindowSenseLevel(window->window, SW_CHANNEL_GROUP_ID, SW_STATE_STEREO_WAVE);
		    } else {
			spSetWindowSenseLevel(window->window, SW_CHANNEL_GROUP_ID, SW_STATE_MONO_WAVE);
		    }
		}
		spDebug(80, "swSetWindowSenseLevel", "playable = %d, processing = %d\n",
			window->config->toplevel->playable, swIsWaveProcessing(window->wave));
		if (window->config->toplevel->playable == SP_FALSE
		    || swIsWaveProcessing(window->wave) == SP_TRUE
		    || window->num_blocked > 0) {
		    spSetWindowSenseLevel(window->window, SW_PLAY_GROUP_ID, SW_STATE_PLAY_WAVE);
		} else {
		    spSetWindowSenseLevel(window->window, SW_PLAY_GROUP_ID, SW_STATE_NOT_PLAY_WAVE);
		}
	    } else {
		spSetWindowSenseLevel(window->window, SW_CHANNEL_GROUP_ID, SW_STATE_NO_WAVE);
		spSetWindowSenseLevel(window->window, SW_PLAY_GROUP_ID, SW_STATE_NO_WAVE);
	    }
	    
	    /* set sensitive for labels */
	    if (swGetNumLabel(window->wave) <= 0) {
		spSetWindowSenseLevel(window->window, SW_LABEL_GROUP_ID, SW_STATE_NO_LABEL);
	    } else {
		spSetWindowSenseLevel(window->window, SW_LABEL_GROUP_ID, SW_STATE_EXIST_LABEL);
	    }
	    if (swGetNumNormalLabel(window->wave) <= 0) {
		spSetWindowSenseLevel(window->window, SW_NORMAL_LABEL_GROUP_ID, SW_STATE_NO_LABEL);
	    } else {
		spSetWindowSenseLevel(window->window, SW_NORMAL_LABEL_GROUP_ID, SW_STATE_EXIST_LABEL);
	    }
	    if (swGetNumRegionLabel(window->wave) <= 0) {
		spSetWindowSenseLevel(window->window, SW_REGION_LABEL_GROUP_ID, SW_STATE_NO_LABEL);
	    } else {
		spSetWindowSenseLevel(window->window, SW_REGION_LABEL_GROUP_ID, SW_STATE_EXIST_LABEL);
	    }
#if defined(SW_SUPPORT_MORPHING)
	    if (swGetNumTimeAnchor(window) <= 0) {
		spSetWindowSenseLevel(window->window, SW_ANCHOR_GROUP_ID, SW_STATE_NO_ANCHOR);
	    } else {
		spSetWindowSenseLevel(window->window, SW_ANCHOR_GROUP_ID, SW_STATE_EXIST_ANCHOR);
	    }
#endif
	    spDebug(80, "swSetWindowSenseLevel", "num_window = %ld, num_blocked = %d\n",
		    window->config->toplevel->num_window, window->num_blocked);
	    
	    swSetSelectSenseLevel(window, ((window->sel_st >= 0 && window->sel_ed >= 0)
					   ? SP_TRUE : SP_FALSE));
	}
	
#if defined(SW_AH_CUSTOM)
	if (window->config->toplevel->ahinforec.session_started == SP_FALSE) {
	    spSetWindowSenseLevel(window->window, SW_AH_GROUP_ID, SW_STATE_AH_SESSION_NONE);
	} else if (!strnone(window->config->toplevel->ahinforec.output_file)) {
	    spSetWindowSenseLevel(window->window, SW_AH_GROUP_ID, SW_STATE_AH_SESSION_FILE_LOADED);
	} else {
	    spSetWindowSenseLevel(window->window, SW_AH_GROUP_ID, SW_STATE_AH_SESSION_STARTED);
	}
#endif

	swSetUndoSenseLevel(window);
    }
    
    spDebug(10, "swSetWindowSenseLevel", "done\n");
    
    return;
}

void swSetSenseLevel(swWindow window)
{
    swSetWindowSenseLevel(window, SP_TRUE);
    return;
}

swWindow swCreateWaveWindowAt(swWave wave, swConfig config, swDataType data_type,
			      double offset_s, double length_s, int x, int y)
{
    swWindow window = NULL;
    
    if (wave != NULL || config->toplevel->num_window == 0) {
        spDebug(80, "swCreateWaveWindowAt", "in: num_window = %ld\n", config->toplevel->num_window);
        
	if (wave != NULL && config->toplevel->null_window != NULL) {
	    window = config->toplevel->null_window;
	    window->data_type = data_type;
	    window->wave = wave;
	    swSetWaveToFirstSubArea(window, window->wave, SP_FALSE);
	    swUpdateWaveSubAreaSize(window);
	    
	    swResetWindow(window, SP_FALSE);
	} else {
	    window = swInitWindow(config, data_type);
	    swSetWaveWithRegion(window, wave, offset_s, length_s);
	    swCreateMainWindow(window);
	    swSetWindowTitle(window);
	    
	    if (swIsClipboardWindow(window) == SP_FALSE) {
		swLockMainMutex(window);
		++config->toplevel->num_window;
		swUnlockMainMutex(window);
		
		spDebug(10, "swCreateWaveWindowAt", "num_window = %ld\n",
			config->toplevel->num_window);
	
		window->visible_flag = SP_TRUE;
		swPopupWindow(window, x, y);
	    
		if (wave == NULL) {
		    config->toplevel->null_window = window;
		}
	    }
	}
        
        spDebug(80, "swCreateWaveWindowAt", "done: num_window = %ld\n", config->toplevel->num_window);
    }
    
    return window;
}

swWindow swCreateWaveWindow(swWave wave, swConfig config, swDataType data_type,
			    double offset_s, double length_s)
{
    return swCreateWaveWindowAt(wave, config, data_type, offset_s, length_s, -1, -1);
}

void swResetWindow(swWindow window, spBool in_thread)
{
    swWave wave;
    
    if (window != NULL) {
	spDebug(10, "swResetWindow", "in_thread = %d\n", in_thread);
    
	swSetWave(window, window->wave);
	/*window->amp_min = 0.0; window->amp_max = -1.0;*/
	swUpdateVscroll(window);
	swSetWindowTitle(window);
	swSetSenseLevel(window);
	
#if defined(SW_AH_CUSTOM)
	if (window->config->toplevel->ahinforec.session_started == SP_TRUE) {
	    swUpdateAHSessionState(window);
	    swUpdateLabelList(window->label_list);
	}
#endif

	wave = swGetTargetWave(window);
	if (wave != NULL && window->wave != wave) {
	    spDebug(10, "swResetWindow", "target wave is not window->wave\n");
	    swReloadWave(window, window->wave, SP_FALSE, in_thread);
	    swReloadWave(window, wave, SP_TRUE, in_thread);
	} else {
	    swReloadWave(window, window->wave, SP_TRUE, in_thread);
	}
	
	swDrawOverview(window, SP_TRUE);
    }
    
    spDebug(10, "swResetWindow", "done\n");
    
    return;
}

void swRedrawWindow(swWindow window)
{
    if (window != NULL) {
	swSetWave(window, window->wave);
	swSetWindowTitle(window);
	swSetSenseLevel(window);
	swRedrawWave(window);
    }
    return;
}

void swSelectRadioButtonSubMenu(spComponent parent_menu, const char *name)
{
    spComponent component;

    component = spGetChild(parent_menu);

    while (component != NULL) {
	if (streq(name, spGetName(component))) {
	    spDebug(10, "swSelectRadioButtonSubMenu", "found menu: name = %s\n", name);
	    spCheckRadioButton(component);
	    break;
	}

	component = spGetNextComponent(component);
    }

    return;
}

void swToggleCheckBoxSubMenu(spComponent parent_menu, char *name, spBool set)
{
    spComponent component;

    component = spGetChild(parent_menu);

    while (component != NULL) {
	if (streq(name, spGetName(component))) {
	    spDebug(10, "swToggleCheckBoxSubMenu", "found menu: name = %s, set = %d\n", name, set);
	    spSetToggleState(component, set);
	    break;
	}

	component = spGetNextComponent(component);
    }

    return;
}

spBool swGetCheckBoxSubMenuToggleState(spComponent parent_menu, char *name, spBool *set)
{
    spBool flag = SP_FALSE;
    spComponent component;

    component = spGetChild(parent_menu);

    while (component != NULL) {
	if (streq(name, spGetName(component))) {
	    flag = spGetToggleState(component, set);
	    spDebug(10, "swGetCheckBoxSubMenuToggleState", "found menu: name = %s, flag = %d\n", name, flag);
	    break;
	}

	component = spGetNextComponent(component);
    }

    return flag;
}

void swUpdateSpectrogramMenu(swWindow window)
{
    if (window->draw_specgram == SP_FALSE) {
	swSelectRadioButtonSubMenu(window->specgram_menu, "noSpectrogram");
    } else if (window->specgram_config_flag == SW_ANALYSIS_CONFIG_FLAG_WIDE_SPECTRUM) {
	swSelectRadioButtonSubMenu(window->specgram_menu, "wideSpectrogram");
    } else if (window->specgram_config_flag == SW_ANALYSIS_CONFIG_FLAG_NARROW_SPECTRUM) {
	swSelectRadioButtonSubMenu(window->specgram_menu, "narrowSpectrogram");
    } else if (window->specgram_config_flag == SW_ANALYSIS_CONFIG_FLAG_NARROW_SMOOTHED_SPECTRUM) {
	swSelectRadioButtonSubMenu(window->specgram_menu, "narrowSmoothedSpectrogram");
#if defined(SW_SUPPORT_CQT_SPECTROGRAM)
    } else if (window->specgram_config_flag == SW_ANALYSIS_CONFIG_FLAG_CQT_SPECTRUM) {
	swSelectRadioButtonSubMenu(window->specgram_menu, "cqtSpectrogram");
#endif
    }
    
    return;
}

void swResetSpectrogramMenu(swWindow window)
{
    window->draw_specgram = SP_FALSE;
    window->draw_vertical_keys = SP_FALSE;
    window->amp_min = 0.0; window->amp_max = -1.0;
    
    /* reset spectrogram radio button menu */
    swSelectRadioButtonSubMenu(window->specgram_menu, "noSpectrogram");
    
    return;
}

void swUpdateSubplotMenu(swWindow window)
{
#ifdef SW_SUPPORT_ANALYSIS_SUBPLOT
    swToggleCheckBoxSubMenu(window->subplot_menu, "subplotSpectrogram", window->subplot_sgram);
    swToggleCheckBoxSubMenu(window->subplot_menu, "subplotF0", window->subplot_f0);
    swToggleCheckBoxSubMenu(window->subplot_menu, "subplotPower", window->subplot_power);
#ifdef SW_SUPPORT_STRAIGHT
    swToggleCheckBoxSubMenu(window->subplot_menu, "subplotStraightSpecgram", window->subplot_straight_sgram);
    swToggleCheckBoxSubMenu(window->subplot_menu, "subplotAperiodicity", window->subplot_straight_ap);
    swToggleCheckBoxSubMenu(window->subplot_menu, "subplotStraightF0", window->subplot_straight_f0);
#endif
#endif
    
    return;
}

void swResetSubplotMenu(swWindow window)
{
#ifdef SW_SUPPORT_ANALYSIS_SUBPLOT
    window->subplot_sgram = SP_FALSE;
    window->subplot_f0 = SP_FALSE;
    window->subplot_power = SP_FALSE;
#ifdef SW_SUPPORT_STRAIGHT
    window->subplot_straight_sgram = SP_FALSE;
    window->subplot_straight_ap = SP_FALSE;
    window->subplot_straight_f0 = SP_FALSE;
#endif
    
    swUpdateSubplotMenu(window);
#endif
    
    return;
}

void swDestroySpectrogram(swWindow window, spBool reset_menu)
{
    if (reset_menu == SP_TRUE) {
	swResetSpectrogramMenu(window);
	swResetSubplotMenu(window);
    }
    
    swDestroyWindowWave(window, &window->specgram);
#ifdef SW_SUPPORT_ANALYSIS_SUBPLOT
    swDestroyWindowWave(window, &window->f0);
    swDestroyWindowWave(window, &window->power);
#ifdef SW_SUPPORT_STRAIGHT
    swDestroyWindowWave(window, &window->straight_sgram);
    swDestroyWindowWave(window, &window->straight_ap);
    swDestroyWindowWave(window, &window->straight_f0);
#endif
#endif
    
    return;
}

void swOpenFile(spComponent component, swWindow window, spBool new_flag)
{
    char *filename = NULL;
    swWave wave = NULL;
    swDataType data_type = SW_TIME_DATA;
    spPluginError err;
    static char *file_filters[] =
    {
	"*",
	"*",
#ifdef SW_USE_ANALYSIS
	"*",
#endif
	NULL,
    };
    static char *file_types[] =
    {
	"Auto Detect",
	"Default",
#ifdef SW_USE_ANALYSIS
	"Frequency Data",
#endif
	NULL,
    };
    static int open_file_type_index = -1;

    if (window != NULL) {
	if (window->config->use_def_format == SP_TRUE) {
	    open_file_type_index = 1;
	} else {
	    open_file_type_index = 0;
	}
	
	/* get open file name */
	filename = xspGetOpenFileName(component, SW_OPEN_DIALOG_TITLE,
				      SppFileMustExist, SP_TRUE,
				      SppFileFilters, file_filters,
				      SppFileTypes, file_types,
				      SppFileFilterIndex, &open_file_type_index,
				      NULL);

	if (filename != NULL) {
	    err = SP_PLUGIN_ERROR_SUCCESS;
	    
	    if ((wave = swGetWave(window->config->wave_config, NULL, filename, NULL, NULL,
				  window->config->samp_bit,
				  window->config->num_channel,
				  window->config->samp_rate,
				  window->config->num_order,
				  SP_TRUE, window->draw_detail, SP_TRUE, &err)) == NULL) {
		spDebug(10, "swOpenFile", "err = %d\n", err);
		
		if (err == SP_PLUGIN_ERROR_SUITABLE_NOT_FOUND
		    || err == SP_PLUGIN_ERROR_FAILURE) {
		    if (open_file_type_index == 0
			|| open_file_type_index == 2) {
			if (swPopupFormatDialog(window->config, filename) == SP_FALSE) {
			    xfree(filename);
			    return;
			}
			data_type = swGetDataType(window->config);
			spDebug(10, "swOpenFile", "data_type = %d, type_index = %d, format_string = %s\n",
				data_type, open_file_type_index, window->config->format_string);
			
#ifdef SW_USE_ANALYSIS
			if (open_file_type_index == 2) {
			    data_type = SW_FREQ_DATA;
			}
#endif
		    } else {
			if (!strnone(window->config->def_format_string)) {
			    spStrCopy(window->config->format_string, SP_MAX_SETUP_VALUE,
				      window->config->def_format_string);
			}
			window->config->samp_bit = window->config->def_samp_bit;
			window->config->num_channel = window->config->def_num_channel;
			window->config->head_size = window->config->def_head_size;
			window->config->samp_rate = window->config->def_samp_rate;
		    }
		}
	    }
	    
	    spDebug(10, "swOpenFile", "filename = %s\n", filename);

	    if (wave == NULL &&
		(err == SP_PLUGIN_ERROR_SUITABLE_NOT_FOUND || err == SP_PLUGIN_ERROR_FAILURE)) {
		/* open wave file */
		wave = swGetWave(window->config->wave_config, NULL, filename, NULL, window->config->format_string,
				 window->config->samp_bit, window->config->num_channel,
				 window->config->samp_rate, window->config->num_order,
				 SP_TRUE, window->draw_detail, SP_FALSE, &err);
		if (wave == NULL) {
		    spDisplayError(NULL, SW_ERROR_TITLE, SW_OPEN_ERROR_MESSAGE, filename);
		}
	    }
	    
	    if (wave != NULL) {
		spDebug(10, "swOpenFile", "read wave done: %ld\n", wave->total_length);
		swSetMouseCursor(window, SP_CURSOR_WAIT);

		if (new_flag == SP_FALSE && window->wave != NULL) {
#ifdef SW_SUPPORT_MORPHING
		    if (window->time_anchors != NULL) {
			swAnchorsDestroy(window->time_anchors);
			window->time_anchors = NULL;
		    }
#endif
		    swDestroySpectrogram(window, SP_TRUE);
		    
		    swUnsetWaveToSubArea(window, window->wave, SP_FALSE);
		    swDestroyWave(window->wave);
		    window->wave = NULL;
		    window->offset = 0;
		    window->length = 0;
		    
		    swUpdateLabelList(window->label_list);
		}

		if (window->wave == NULL &&
		    (swIsClipboardWindow(window) == SP_FALSE || new_flag == SP_FALSE)) {
		    /* set wave to current window */
		    window->data_type = data_type;
		    window->analysis_flag = SP_FALSE;
		    window->wave = wave;
		    swSetWaveToFirstSubArea(window, window->wave, SP_FALSE);
		    swUpdateWaveSubAreaSize(window);
		    
		    swResetWindow(window, SP_FALSE);
		    window->config->toplevel->null_window = NULL;
		} else {
		    /* create new window */
		    swCreateWaveWindow(wave, window->config, data_type, -1.0, -1.0);
		}
		swSetMouseCursor(window, SP_CURSOR_UNKNOWN);
	    }
	    
	    spDebug(10, "swOpenFile", "filename = %s\n", filename);
	    
	    /* memory free */
	    xfree(filename);
	}
    }
    
    return;
}

void swOpenNewWindow(spComponent component, swWindow window)
{
    swOpenFile(component, window, SP_TRUE);
    return;
}

void swOpenWindow(spComponent component, swWindow window)
{
    swOpenFile(component, window, SP_FALSE);
    return;
}

#define SW_MAX_PLUGIN /*64*/128

static int sw_save_num_plugin = 0;
static char **sw_save_plugin_names = NULL;
static char **sw_save_file_types = NULL;
static char **sw_save_file_filters = NULL;

int swGetPluginNames(char ***plugin_names_ptr, char ***file_types_ptr, char ***file_filters_ptr)
{
    int i;
    
    if (sw_save_num_plugin <= 0) {
	sw_save_plugin_names = xalloc(SW_MAX_PLUGIN + 1, char *);
	sw_save_file_types = xalloc(SW_MAX_PLUGIN + 1, char *);
	sw_save_file_filters = xalloc(SW_MAX_PLUGIN + 1, char *);
	
	for (i = 0; i < SW_MAX_PLUGIN; i++) {
	    spDebug(50, "swGetPluginNames", "i = %d / %d\n", i, SW_MAX_PLUGIN);
	    sw_save_plugin_names[i] = xalloc(SP_MAX_PATHNAME, char);
	    sw_save_file_types[i] = xalloc(SP_MAX_LINE, char);
	    sw_save_file_filters[i] = xalloc(SP_MAX_LINE, char);
	    
	    if (i == 0) {
		strcpy(sw_save_plugin_names[i], "");
		strcpy(sw_save_file_types[i], "By Extension");
		strcpy(sw_save_file_filters[i], "*");
	    } else {
		if (spSearchPluginFileType(i - 1, SP_PLUGIN_OUTPUT,
					   SP_PLUGIN_DEVICE_FILE, sw_save_plugin_names[i],
					   NULL, sw_save_file_types[i], sw_save_file_filters[i]) == SP_FALSE) {
                    spDebug(50, "swGetPluginNames", "i = %d, spSearchPluginFileType return SP_FALSE\n", i);
		    xfree(sw_save_plugin_names[i]);
		    xfree(sw_save_file_types[i]);
		    xfree(sw_save_file_filters[i]);
		    break;
		}
	    }
	    spDebug(50, "swGetPluginNames", "sw_save_plugin_name = %s, sw_save_file_type = %s, sw_save_file_filters = %s\n",
		    sw_save_plugin_names[i], sw_save_file_types[i], sw_save_file_filters[i]);
	}
	sw_save_num_plugin = i;
	sw_save_plugin_names[i] = NULL;
	sw_save_file_types[i] = NULL;
	sw_save_file_filters[i] = NULL;
    }

    if (plugin_names_ptr != NULL) *plugin_names_ptr = sw_save_plugin_names;
    if (file_types_ptr != NULL) *file_types_ptr = sw_save_file_types;
    if (file_filters_ptr != NULL) *file_filters_ptr = sw_save_file_filters;

    return sw_save_num_plugin;
}

char *xswGetSaveFileName(spComponent component, swWindow window, char *orig_filename,
			 int index, char **plugin_name, char **file_type)
{
    char *filename;
    int num_plugin;
    char **plugin_names = NULL;
    char **file_types = NULL;
    char **file_filters = NULL;

    num_plugin = swGetPluginNames(&plugin_names, &file_types, &file_filters);
    spDebug(50, "xswGetSaveFileName", "num_plugin = %d\n", num_plugin);

    filename = NULL;
    
    if (window != NULL && window->wave != NULL) {
        if (orig_filename == NULL) {
            orig_filename = window->wave->core->orig_filename;
            spDebug(10, "xswGetSaveFileName", "original wave: orig_filename = %s\n", orig_filename);
        }
        
	/* get save file name */
	filename = xspGetSaveFileName(component, SW_SAVE_DIALOG_TITLE,
				      SppInitialFileName, orig_filename,
				      SppPathMustExist, SP_TRUE,
				      SppFileMustExist, SP_FALSE,
				      SppOverwritePrompt, SP_TRUE,
				      SppFileFilters, file_filters,
				      SppFileTypes, file_types,
				      SppFileFilterIndex, &index,
				      NULL);
	spDebug(10, "xswGetSaveFileName", "index = %d\n", index);

	if (filename != NULL && plugin_name != NULL && file_type != NULL) {
	    if (index >= 1) {
		*plugin_name = plugin_names[index];
		*file_type = file_types[index];
		spDebug(10, "xswGetSaveFileName", "plugin_name = %s, file_type = %s\n",
			*plugin_name, *file_type);
	    } else {
		*plugin_name = NULL;
		*file_type = NULL;
	    }
	}
    }

    return filename;
}

spBool swSaveAs(spComponent component, swWindow window, int index)
{
    char *filename = NULL;
    char *plugin_name = NULL;
    char *file_type = NULL;
    spBool flag = SP_FALSE;
    
    if (window != NULL) {
	/* get save file name */
	filename = xswGetSaveFileName(component, window, NULL, index, &plugin_name, &file_type);
	spDebug(10, "swSaveAs", "index = %d, filename = %s\n", index, filename);

	if (filename != NULL) {
#ifdef SW_SUPPORT_THREAD_WRITE
            spDebug(10, "swSaveAs", "SW_SUPPORT_THREAD_WRITE, call swWriteWave: index = %d, filename = %s\n", index, filename);
	    swWriteWave(filename, plugin_name, NULL, file_type, window->wave, SP_TRUE, SP_TRUE);
#else
	    swSetMouseCursor(window, SP_CURSOR_WAIT);
	    if (swWriteWave(filename, plugin_name, NULL, file_type, window->wave, SP_TRUE, SP_FALSE) == SP_TRUE) {
		/*window->wave->edit_flag = SP_FALSE;*/
		window->offset = 0;
		window->length = 0;

		swInitWaveRange(window->wave);
		
		/* reset window */
		swResetWindow(window, SP_FALSE);

                flag = SP_TRUE;
	    }
	    swSetMouseCursor(window, SP_CURSOR_UNKNOWN);
#endif
	    xfree(filename);
	}
    }

    spDebug(100, "swSaveAs", "done: flag = %d\n", flag);
    
    return flag;
}

void swSaveAsWindow(spComponent component, swWindow window)
{
    swSaveAs(component, window, 0);
    return;
}

void swSaveWindow(spComponent component, swWindow window)
{
    if (window != NULL) {
	if (strnone(window->wave->core->orig_filename)) {
	    swSaveAs(component, window, 0);
	} else {
#ifdef SW_SUPPORT_THREAD_WRITE
	    swWriteWave(NULL, NULL, NULL, NULL, window->wave, SP_TRUE, SP_TRUE);
#else
	    swSetMouseCursor(window, SP_CURSOR_WAIT);
	    if (swWriteWave(NULL, NULL, NULL, NULL, window->wave, SP_TRUE, SP_FALSE) == SP_TRUE) {
		swSetWindowTitle(window);
	    }
	    swSetMouseCursor(window, SP_CURSOR_UNKNOWN);
#endif
	}
    }

    return;
}

void swCloseWindowCB(spComponent component, swWindow window)
{
    swCloseWindow(window, SP_TRUE);
    return;
}

void swDestroyWindowCB(spComponent component, swWindow window)
{
    spDebug(30, "swDestroyWindowCB", "in\n");
    swDestroyWindow(window);
    spDebug(30, "swDestroyWindowCB", "done\n");
    return;
}

void swQuitCB(spComponent component, swWindow window)
{
    swWindow current_window, next_window;

    spDebug(10, "swQuitCB", "in\n");
    
    if (spQuitPrompt(window->window) == SP_FALSE) {
	return;
    }
    
    if ((next_window = swGetNextWindow(window)) == NULL) {
	/*spQuit(0);*/
	swCloseWindow(window, SP_FALSE);
    } else {
	while (next_window != NULL) {
	    current_window = next_window;
	    next_window = swGetNextWindow(next_window);
	    if (swCloseWindow(current_window, SP_FALSE) == SP_FALSE) {
		break;
	    }

	    if (next_window == window) {
		swCloseWindow(window, SP_FALSE);
		break;
	    }
	}
    }
    
    return;
}

void swDisplayHelpCB(spComponent component, void *data)
{
    if (spDisplayHelp(component, "index2.html") == SP_FALSE) {
	spDisplayError(component, NULL, SW_DISPLAY_HELP_ERROR_MESSAGE);
    }
    
    return;
}

void swDisplayInfoCB(spComponent component, void *data)
{
    spDisplayInformation(/*component*/NULL, SW_INFO_TITLE, SW_INFO_MESSAGE, SW_VERSION_STRING);
    return;
}

static void createInfoArea(swWindow window)
{
    if (window->display_info_area == SP_TRUE) {
	if (window->info_box == NULL) {
	    /* create container for information display area */
	    window->info_box = spCreateBox(window->window, "infoBox", 0,
					   NULL);

	    window->label_list = swCreateLabelList(window->info_box, window);
	}
    }
    
    return;
}

void swUpdateInfoAreaDisplay(swWindow window)
{
    int default_slider_width;
    
    default_slider_width = spGetScrollBarDefaultWidth();
	    
    if (window->display_info_area == SP_TRUE) {
	spSetParams(window->wave_box, SppWidth, -spScaleX(window->config->info_area_width + default_slider_width), NULL);
	if (window->info_box != NULL) {
	    spMapComponent(window->info_box);
	} else {
	    createInfoArea(window);
	}

	if (window->sel_st >= 0 && window->sel_ed >= 0) {
	    swUpdateInfoAreaSelection(window);
	}
    } else {
	spSetParams(window->wave_box, SppWidth, -spScaleX(default_slider_width), NULL);
	if (window->info_box != NULL) {
	    spUnmapComponent(window->info_box);
	}
    }
    spAdjustWindowSize(window->window, SP_FALSE);
    
    return;
}

void swUpdateAllInfoAreaDisplay(swWindow window)
{
    spComponent next = NULL;
    swWindow next_window = NULL;
    
    if (window == NULL) return;

    swUpdateInfoAreaDisplay(window);
    
    next = window->window;
    while (1) {
	next = spGetNextWindow(next, SP_FALSE);
	if (next == NULL || next == window->window) {
	    break;
	}

	if ((next_window = (swWindow)spGetUserData(next)) != NULL) {
	    swUpdateInfoAreaDisplay(next_window);
	}
    }

    return;
}

void swCheckDisplayInfoAreaCB(spComponent component, swWindow window)
{
    spBool set;
    
    if (spGetToggleState(component, &set) == SP_TRUE) {
	if (set != window->display_info_area) {
	    window->display_info_area = set;
	    swUpdateInfoAreaDisplay(window);
	}
    }
    
    return;
}

void swCheckDrawDetailCB(spComponent component, swWindow window)
{
    spBool set;
    swWave wave;
    
    if (spGetToggleState(component, &set) == SP_TRUE) {
	if (set != window->draw_detail) {
	    window->draw_detail = set;

	    wave = swGetTargetWave(window);
	    swSetDetailFlag(wave, window->draw_detail);
	    swReloadWave(window, wave, SP_TRUE, SP_FALSE);

	    window->config->draw_detail = window->draw_detail;
	}
    }
    
    return;
}

void swCheckDrawLabelCB(spComponent component, swWindow window)
{
    spBool set;
    
    if (spGetToggleState(component, &set) == SP_TRUE) {
	if (set != window->draw_label) {
	    window->draw_label = set;
	    swRedrawLabels(window);
	}
    }
    
    return;
}

#ifdef SW_SUPPORT_LOG_FREQUENCY_AXIS
void swCheckLogFrequencyAxisCB(spComponent component, swWindow window)
{
    spBool set;
    swWave wave;
    
    if (spGetToggleState(component, &set) == SP_TRUE) {
	if (set != window->log_frequency_axis) {
	    window->log_frequency_axis = set;
            
            if (swIsSpectrogramVisible(window) == SP_TRUE) {
                swUpdateVscroll(window);
                swDrawWave(window);
                if (window->related_window != NULL && window->related_window->data_type == SW_FREQ_DATA
                    && set != window->related_window->log_frequency_axis) {
                    window->related_window->log_frequency_axis = set;
                    spSetToggleState(window->related_window->log_frequency_axis_menu, set);
                    wave = swGetTargetWave(window->related_window);

                    swReloadWave(window->related_window, wave, SP_TRUE, SP_FALSE);
                    swDrawOverview(window->related_window, SP_TRUE);
                }
            } else if (window->data_type == SW_FREQ_DATA) {
                wave = swGetTargetWave(window);

                swReloadWave(window, wave, SP_TRUE, SP_FALSE);
                swDrawOverview(window, SP_TRUE);
            }
	}
    }
    
    return;
}
#endif

void swChangeTimeFormatCB(spComponent component, swWindow window)
{
    const char *name;
    
    name = spGetName(component);

    spDebug(10, "swChangeTimeFormatCB", "name = %s\n", name);
    
    if (streq(name, "timeFormatSec")) {
	window->config->time_format = SW_TIME_FORMAT_SEC;
    } else if (streq(name, "timeFormatMsec")) {
	window->config->time_format = SW_TIME_FORMAT_MSEC;
    } else if (streq(name, "timeFormatPoint")) {
	window->config->time_format = SW_TIME_FORMAT_POINT;
    } else if (streq(name, "timeFormatSeparatedSec")) {
	window->config->time_format = SW_TIME_FORMAT_SEPARATED_SEC;
    }

    if (window->config->scale_flag == SP_TRUE || window->config->grid_flag == SP_TRUE) {
	/* redraw all window */
	swDrawAllWave(window);
    } else {
	/* redraw all cursor */
	swMoveAllCursor(window);
    }
    
    swUpdateAllLabelList(window);

    {
	swWindow next_window;

	next_window = swGetNextWindow(window);

	/* update radio button of other windows */
	while (next_window != NULL) {
	    if (next_window == window) {
		break;
	    }

	    swSelectRadioButtonSubMenu(next_window->time_format_menu, name);
	    
	    next_window = swGetNextWindow(next_window);
	}
    }
    
    return;
}

void swDisplayPluginInfoCB(spComponent component, char *plugin_name)
{
    const char *information;
    spPlugin *plugin;

    /* load plugin */
    if ((plugin = spLoadPlugin(plugin_name)) != NULL) {
	information = spGetPluginInformation(plugin);
	
	spDisplayInformation(/*component*/NULL, NULL, information);
	
	spFreePlugin(plugin);
    }
    return;
}

void swCreatePluginInfoMenu(swWindow window, spComponent sub_menu)
{
    int i;
    char *plugin_name;
    spComponent menu_item;
    spPlugin *plugin;
    
    for (i = 0;; i++) {
        spDebug(100, "swCreatePluginInfoMenu", "i = %d\n", i);
	if ((plugin_name = xspSearchPluginFile(i)) == NULL) {
            spDebug(100, "swCreatePluginInfoMenu", "xspSearchPluginFile finished.\n");
	    break;
	}
	if ((plugin = spLoadPlugin(plugin_name)) != NULL) {
	    menu_item = spAddMenuItem(sub_menu, "pluginInfoMenu",
				      SppTitle, spGetPluginDescription(plugin),
				      SppCallbackFunc, swDisplayPluginInfoCB,
				      SppCallbackData, plugin_name,
				      NULL);
	    spFreePlugin(plugin);
	} else {
            spDebug(100, "swCreatePluginInfoMenu", "spLoadPlugin returns NULL.\n");
        }
    }
    
    spDebug(100, "swCreatePluginInfoMenu", "done: i = %d\n", i);
    
    return;
}

void swToggleLoopPlayCB(spComponent component, swWindow window)
{
    if (spGetToggleState(component, &window->loop_play) == SP_TRUE) {
	spDebug(10, "swToggleLoopPlayCB", "sync_play = %d\n", window->loop_play);
	spSetToggleState(window->loop_play_menu, window->loop_play);
	spSetToggleState(window->loop_play_tool_item, window->loop_play);
    }
    
    return;
}

void swToggleSyncPlayCB(spComponent component, swWindow window)
{
    if (spGetToggleState(component, &window->sync_play) == SP_TRUE) {
	spDebug(10, "swToggleSyncPlayCB", "sync_play = %d\n", window->sync_play);
	spSetToggleState(window->sync_play_menu, window->sync_play);
	spSetToggleState(window->sync_play_tool_item, window->sync_play);
    }
    
    return;
}

void swCreateToolBar(swWindow window)
{
#ifdef SW_USE_TOOL_BAR
    if (window->tool_bar == NULL) {
#if defined(MACOS) /* MPW doesn't allow the data type. Why?*/
	static char *tool_bar_xpm[] = {"", ""};
#else
#include "tool_bar.xpm"
#endif
	
	spComponent tool_item;
	long sense_level;
	int group_index = -1;
	
	spDebug(10, "swCreateToolBar", "in\n");

	if (swIsClipboardWindow(window) == SP_TRUE) {
	    group_index = 0;
	}
	
	window->tool_bar = spCreateToolBar(window->window, "tool_bar", 18,
					   SppToolBarGroupIndex, group_index,
					   SppBitmapData, tool_bar_xpm,
					   SppVisible, (window->config->no_tool_bar == SP_FALSE ?
							SP_TRUE : SP_FALSE),
					   NULL);
	if (swIsClipboardWindow(window) == SP_FALSE) {
	    tool_item = spAddToolItem(window->tool_bar, NULL,
				      SppBitmapIndex, 0,
				      SppGroupId, SW_OPEN_GROUP_ID,
				      SppSenseLevel, SW_STATE_NO_WAVE,
				      SppCallbackFunc, swOpenWindow,
				      SppCallbackData, window,
				      SppDescription, SW_MENU_OPEN_DESC,
				      NULL);
	}
	if (swIsClipboardWindow(window) == SP_TRUE) {
	    sense_level = SW_STATE_NO_WAVE;
	} else {
	    sense_level = SW_STATE_EXIST_WAVE;
	}
	tool_item = spAddToolItem(window->tool_bar, NULL,
				  SppBitmapIndex, 1,
				  /*SppGroupId, SW_OPEN_GROUP_ID,*/
				  SppSenseLevel, sense_level,
				  SppCallbackFunc, swOpenNewWindow,
				  SppCallbackData, window,
				  SppDescription, SW_MENU_OPEN_NEW_DESC,
				  NULL);
#ifdef SW_SUPPORT_SAVE
	if (swIsClipboardWindow(window) == SP_FALSE) {
	    tool_item = spAddToolItem(window->tool_bar, NULL,
				      SppBitmapIndex, 2,
				      SppSenseLevel, SW_STATE_EDIT_WAVE,
				      SppGroupId, SW_SAVE_GROUP_ID,
				      SppCallbackFunc, swSaveWindow,
				      SppCallbackData, window,
				      SppDescription, SW_MENU_SAVE_DESC,
				      NULL);
	    tool_item = spAddToolItem(window->tool_bar, NULL,
				      SppBitmapIndex, 3,
				      SppGroupId, SW_OPEN_GROUP_ID,
				      SppSenseLevel, SW_STATE_EXIST_WAVE,
				      SppCallbackFunc, swSaveAsWindow,
				      SppCallbackData, window,
				      SppDescription, SW_MENU_SAVE_AS_DESC,
				      NULL);
	}
#endif
	spAddToolSeparator(window->tool_bar, NULL, NULL);

	window->sync_play_tool_item = spAddCheckToolItem(window->tool_bar, NULL,
							 SppBitmapIndex, 17,
							 SppGroupId, SW_DATA_GROUP_ID,
							 SppSenseLevel, SW_STATE_PLAY_TIME_DATA,
							 SppSet, window->sync_play,
							 SppCallbackFunc, swToggleSyncPlayCB,
							 SppCallbackData, window,
							 SppDescription, SW_MENU_SYNC_PLAY_DESC,
							 NULL);
	window->loop_play_tool_item = spAddCheckToolItem(window->tool_bar, NULL,
							 SppBitmapIndex, 16,
							 SppGroupId, SW_DATA_GROUP_ID,
							 /*SppSenseLevel, SW_STATE_PLAY_TIME_DATA,*/
							 SppSenseLevel, SW_STATE_TIME_DATA,
							 SppSet, window->loop_play,
							 SppCallbackFunc, swToggleLoopPlayCB,
							 SppCallbackData, window,
							 SppDescription, SW_MENU_LOOP_PLAY_DESC,
							 NULL);
	window->pause_cursor_tool_item = spAddCheckToolItem(window->tool_bar, NULL,
							    SppBitmapIndex, 15,
							    SppGroupId, SW_PAGE_GROUP_ID,
							    SppSenseLevel, SW_STATE_EXIST_WAVE,
							    SppSet, window->pause_cursor,
							    SppCallbackFunc, swPauseCursorCB,
							    SppCallbackData, window,
							    SppDescription, _("SW_MENU_PAUSE_CURSOR_DESC"),
							    NULL);
	tool_item = spAddToolItem(window->tool_bar, NULL,
				  SppBitmapIndex, 7,
				  SppGroupId, SW_DATA_GROUP_ID,
				  SppSenseLevel, SW_STATE_PLAY_TIME_DATA,
				  SppCallbackFunc, swPlayStopCB,
				  SppCallbackData, window,
				  SppDescription, SW_MENU_PLAY_STOP_DESC,
				  NULL);
	tool_item = spAddToolItem(window->tool_bar, NULL,
				  SppBitmapIndex, 8,
				  SppGroupId, SW_PLAY_GROUP_ID,
				  SppSenseLevel, SW_STATE_NOT_PLAY_WAVE,
				  SppCallbackFunc, swPlayRegionCB,
				  SppCallbackData, window,
				  SppDescription, SW_MENU_PLAY_DESC,
				  NULL);
	tool_item = spAddToolItem(window->tool_bar, NULL,
				  SppBitmapIndex, 9,
				  SppGroupId, SW_PAGE_GROUP_ID,
				  SppSenseLevel, SW_STATE_EXIST_WAVE,
				  SppCallbackFunc, swBackwardCB,
				  SppCallbackData, window,
				  SppDescription, SW_MENU_BACKWARD_DESC,
				  NULL);
	tool_item = spAddToolItem(window->tool_bar, NULL,
				  SppBitmapIndex, 10,
				  SppGroupId, SW_PAGE_GROUP_ID,
				  SppSenseLevel, SW_STATE_EXIST_WAVE,
				  SppCallbackFunc, swForwardCB,
				  SppCallbackData, window,
				  SppDescription, SW_MENU_FORWARD_DESC,
				  NULL);
	tool_item = spAddToolItem(window->tool_bar, NULL,
				  SppBitmapIndex, 11,
				  SppGroupId, SW_PAGE_GROUP_ID,
				  SppSenseLevel, SW_STATE_EXIST_WAVE,
				  SppCallbackFunc, swGoHeadCB,
				  SppCallbackData, window,
				  SppDescription, SW_MENU_GO_HEAD_DESC,
				  NULL);
	tool_item = spAddToolItem(window->tool_bar, NULL,
				  SppBitmapIndex, 12,
				  SppGroupId, SW_PAGE_GROUP_ID,
				  SppSenseLevel, SW_STATE_EXIST_WAVE,
				  SppCallbackFunc, swGoTailCB,
				  SppCallbackData, window,
				  SppDescription, SW_MENU_GO_TAIL_DESC,
				  NULL);
	tool_item = spAddToolItem(window->tool_bar, NULL,
				  SppBitmapIndex, 13,
				  SppGroupId, SW_PAGE_GROUP_ID,
				  SppSenseLevel, SW_STATE_EXIST_WAVE,
				  SppCallbackFunc, swZoomInCB,
				  SppCallbackData, window,
				  SppDescription, SW_MENU_ZOOM_IN_DESC,
				  NULL);
	tool_item = spAddToolItem(window->tool_bar, NULL,
				  SppBitmapIndex, 14,
				  SppGroupId, SW_PAGE_GROUP_ID,
				  SppSenseLevel, SW_STATE_EXIST_WAVE,
				  SppCallbackFunc, swZoomOutCB,
				  SppCallbackData, window,
				  SppDescription, SW_MENU_ZOOM_OUT_DESC,
				  NULL);
    }
#endif

    return;
}

long swGetScrollCoef(swWave wave)
{
    if (wave->total_length >= 32768) {
	return (long)ceil((double)wave->total_length / 32768.0);
    } else {
	return 1;
    }
}

#ifdef SW_SUPPORT_DRAG_DROP
void swDropCB(spComponent component, swWindow window, int argc, char **argv)
{
    int i;

    if (window == NULL) return;

#if 1
    if (swIsProcessing(window) == SP_TRUE || window->config->toplevel->num_process > 0) {
	spDisplayWarning(window->window, NULL, SW_PROCESSING_WARNING_MESSAGE);
	return;
    }
#endif
    
    for (i = 0; i < argc; i++) {
	swCreateWindow(argv[i], NULL, SP_FALSE, -1.0, -1.0, window->config);
    }

    return;
}
#endif

void swCreateMainWindow(swWindow window)
{
    int size;
    int width;
    int default_slider_width;
    long sense_level;
    spComponent menu, sub_menu, menu_bar, menu_item;
    spCloseStyle close_style = SW_DEFAULT_CLOSE_STYLE;
    
    if (window == NULL) return;

    window->window = spCreateFrame("mainWindow",
				   SppCallbackFunc, swCloseWindowCB,
				   SppCallbackData, window,
				   SppUserData, window,
				   SppCloseStyle, close_style,
				   SppSimplifiable, SP_TRUE,
				   SppOrientation, SP_HORIZONTAL,
				   NULL);
    spAddCallback(window->window, SP_DESTROY_CALLBACK,
		  (spCallbackFunc)swDestroyWindowCB, (void *)window);
    spAddCallback(window->window, SP_SIMPLIFY_CALLBACK,
		  (spCallbackFunc)swShowAllToolBarCB, (void *)window);
#ifdef SW_SUPPORT_DRAG_DROP
    spAddDropCallback(window->window, (spDropCallbackFunc)swDropCB, (void *)window);
#endif
#ifdef SW_USE_POST_EVENT
    spAddEventCallback(window->window, swEventCB);
#endif

    menu_bar = spCreateMenuBar(window->window, "menuBar", NULL);
    
    /* `File' menu */
    menu = spCreatePulldownMenu(menu_bar, "fileMenu",
				SppTitle, SW_MENU_FILE_LABEL,
				SppHelpPath, "menu/file_menu.html",
				NULL);
    if (swIsClipboardWindow(window) == SP_FALSE) {
	menu_item = spAddMenuItem(menu, "open",
				  SppTitle, SW_MENU_OPEN_LABEL,
				  SppGroupId, SW_OPEN_GROUP_ID,
				  SppSenseLevel, SW_STATE_NO_WAVE,
				  SppCallbackFunc, swOpenWindow,
				  SppCallbackData, window,
				  SppShortcut, SW_OPEN_SHORTCUT,
				  SppHelpPath, "menu/file_menu.html#file_open",
				  NULL);
    }
    if (swIsClipboardWindow(window) == SP_TRUE) {
	sense_level = SW_STATE_NO_WAVE;
    } else {
	sense_level = SW_STATE_EXIST_WAVE;
    }
    menu_item = spAddMenuItem(menu, "openNew",
			      SppTitle, SW_MENU_OPEN_NEW_LABEL,
			      /*SppGroupId, SW_OPEN_GROUP_ID,*/
			      SppSenseLevel, sense_level,
			      SppCallbackFunc, swOpenNewWindow,
			      SppCallbackData, window,
			      SppShortcut, SW_OPEN_NEW_SHORTCUT,
			      SppHelpPath, "menu/file_menu.html#file_open_new",
			      NULL);
    menu_item = spAddMenuItem(menu, "generate",
			      SppTitle, _("SW_MENU_GENERATE_LABEL"),
			      SppCallbackFunc, swPopupWaveformGenerateDialogCB,
			      SppCallbackData, window,
			      SppHelpPath, "menu/file_menu.html#file_generate",
			      SppShortcut, SW_GENERATE_SHORTCUT,
			      NULL);
#ifdef SW_SUPPORT_SAVE
    if (swIsClipboardWindow(window) == SP_FALSE) {
	menu_item = spAddMenuItem(menu, "save",
				  SppTitle, SW_MENU_SAVE_LABEL,
				  SppSenseLevel, SW_STATE_EDIT_WAVE,
				  SppGroupId, SW_SAVE_GROUP_ID,
				  SppCallbackFunc, swSaveWindow,
				  SppCallbackData, window,
				  SppShortcut, SW_SAVE_SHORTCUT,
				  SppHelpPath, "menu/file_menu.html#file_save",
				  NULL);
	menu_item = spAddMenuItem(menu, "saveAs",
				  SppTitle, SW_MENU_SAVE_AS_LABEL,
				  SppGroupId, SW_OPEN_GROUP_ID,
				  SppSenseLevel, SW_STATE_EXIST_WAVE,
				  SppCallbackFunc, swSaveAsWindow,
				  SppCallbackData, window,
				  SppShortcut, SW_SAVE_AS_SHORTCUT,
				  SppHelpPath, "menu/file_menu.html#file_save_as",
				  NULL);
    }
#endif
    {
	spComponent label_menu;
	
	/* create menus for labels */
	spAddMenuSeparator(menu, "labelSeparator", NULL);
	label_menu = spAddSubMenu(menu, "labelMenu",
				  SppTitle, SW_MENU_LABEL_LABEL,
				  SppHelpPath, "menu/file_menu.html#file_label",
				  NULL);
	menu_item = spAddMenuItem(label_menu, "openLabel",
				  SppTitle, SW_MENU_OPEN_LABEL_LABEL,
				  SppSenseLevel, SW_STATE_EXIST_WAVE,
				  SppCallbackFunc, swOpenLabelCB,
				  SppCallbackData, window,
				  SppShortcut, SW_OPEN_NORMAL_LABEL_SHORTCUT,
				  SppHelpPath, "menu/file_menu.html#file_label_open",
				  NULL);
	menu_item = spAddMenuItem(label_menu, "saveLabel",
				  SppTitle, SW_MENU_SAVE_LABEL_LABEL,
				  SppGroupId, SW_NORMAL_LABEL_GROUP_ID,
				  SppSenseLevel, SW_STATE_EXIST_LABEL,
				  SppCallbackFunc, swSaveLabelCB,
				  SppCallbackData, window,
				  SppShortcut, SW_SAVE_NORMAL_LABEL_SHORTCUT,
				  SppHelpPath, "menu/file_menu.html#file_label_save",
				  NULL);
	menu_item = spAddMenuItem(label_menu, "saveAsLabel",
				  SppTitle, SW_MENU_SAVE_AS_LABEL_LABEL,
				  SppGroupId, SW_NORMAL_LABEL_GROUP_ID,
				  SppSenseLevel, SW_STATE_EXIST_LABEL,
				  SppCallbackFunc, swSaveAsLabelCB,
				  SppCallbackData, window,
				  SppShortcut, SW_SAVE_AS_NORMAL_LABEL_SHORTCUT,
				  SppHelpPath, "menu/file_menu.html#file_label_save_as",
				  NULL);
	menu_item = spAddMenuItem(label_menu, "clearLabel",
				  SppTitle, SW_MENU_CLEAR_LABEL_LABEL,
				  SppGroupId, SW_NORMAL_LABEL_GROUP_ID,
				  SppSenseLevel, SW_STATE_EXIST_LABEL,
				  SppCallbackFunc, swClearLabelCB,
				  SppCallbackData, window,
				  SppHelpPath, "menu/file_menu.html#file_label_clear",
				  NULL);
	spAddMenuSeparator(label_menu, "labelSeparator", NULL);
	menu_item = spAddMenuItem(label_menu, "openRegionLabel",
				  SppTitle, SW_MENU_OPEN_REGION_LABEL_LABEL,
				  SppSenseLevel, SW_STATE_EXIST_WAVE,
				  SppCallbackFunc, swOpenRegionLabelCB,
				  SppCallbackData, window,
				  SppShortcut, SW_OPEN_REGION_LABEL_SHORTCUT,
				  SppHelpPath, "menu/file_menu.html#file_region_label_open",
				  NULL);
	menu_item = spAddMenuItem(label_menu, "saveRegionLabel",
				  SppTitle, SW_MENU_SAVE_REGION_LABEL_LABEL,
				  SppGroupId, SW_REGION_LABEL_GROUP_ID,
				  SppSenseLevel, SW_STATE_EXIST_LABEL,
				  SppCallbackFunc, swSaveRegionLabelCB,
				  SppCallbackData, window,
				  SppShortcut, SW_SAVE_REGION_LABEL_SHORTCUT,
				  SppHelpPath, "menu/file_menu.html#file_region_label_save",
				  NULL);
	menu_item = spAddMenuItem(label_menu, "saveAsRegionLabel",
				  SppTitle, SW_MENU_SAVE_AS_REGION_LABEL_LABEL,
				  SppGroupId, SW_REGION_LABEL_GROUP_ID,
				  SppSenseLevel, SW_STATE_EXIST_LABEL,
				  SppCallbackFunc, swSaveAsRegionLabelCB,
				  SppCallbackData, window,
				  SppShortcut, SW_SAVE_AS_REGION_LABEL_SHORTCUT,
				  SppHelpPath, "menu/file_menu.html#file_region_label_save_as",
				  NULL);
	menu_item = spAddMenuItem(label_menu, "clearRegionLabel",
				  SppTitle, SW_MENU_CLEAR_REGION_LABEL_LABEL,
				  SppGroupId, SW_REGION_LABEL_GROUP_ID,
				  SppSenseLevel, SW_STATE_EXIST_LABEL,
				  SppCallbackFunc, swClearRegionLabelCB,
				  SppCallbackData, window,
				  SppHelpPath, "menu/file_menu.html#file_region_clear",
				  NULL);
    }

#if defined(SW_AH_CUSTOM)
    {
	spAddMenuSeparator(menu, "AHSeparator", NULL);
	spAddMenuItem(menu, "startAHSessionMenuItem",
		      SppTitle, _("SW_MENU_START_AH_SESSION_LABEL"),
		      SppGroupId, SW_OPEN_GROUP_ID,
		      SppSenseLevel, SW_STATE_NO_WAVE,
		      SppCallbackFunc, swStartAHSessionCB,
		      SppCallbackData, window->config->toplevel->ahwindow,
		      NULL);
    }
#endif
    
#ifdef SW_SUPPORT_PROPERTY_DIALOG
    {
	spAddMenuSeparator(menu, "propertySeparator", NULL);
	spAddMenuItem(menu, "propertyMenuItem",
		      SppTitle, _("SW_MENU_PROPERTY_LABEL"),
		      SppSenseLevel, SW_STATE_EXIST_WAVE,
		      SppCallbackFunc, swPopupPropertyDialogCB,
		      SppCallbackData, window,
		      SppHelpPath, "menu/file_menu.html#file_property",
		      NULL);
    }
#endif
    
#ifdef SW_SUPPORT_PRINT
    {
	spAddMenuSeparator(menu, "printSeparator", NULL);
	swCreateDrawingPluginMenu(window, menu);
	spAddMenuItem(menu, "pageSetupMenuItem",
		      SppTitle, _("SW_MENU_PAGE_SETUP_LABEL"),
		      SppSenseLevel, SW_STATE_EXIST_WAVE,
		      SppCallbackFunc, swPageSetupCB,
		      SppCallbackData, window,
		      SppHelpPath, "menu/file_menu.html#file_page_setup",
		      NULL);
	spAddMenuItem(menu, "printMenuItem",
		      SppTitle, _("SW_MENU_PRINT_LABEL"),
		      SppSenseLevel, SW_STATE_EXIST_WAVE,
		      SppCallbackFunc, swPrintCB,
		      SppCallbackData, window,
		      SppShortcut, SW_PRINT_SHORTCUT,
		      SppHelpPath, "menu/file_menu.html#file_print",
		      NULL);
    }
#endif
    spAddMenuSeparator(menu, "fileSeparator", NULL);
    menu_item = spAddMenuItem(menu, "close",
			      SppTitle, SW_MENU_CLOSE_LABEL,
			      SppGroupId, SW_WINDOW_GROUP_ID,
			      SppSenseLevel, SW_STATE_STOP_SOME_WINDOWS,
			      SppCallbackFunc, swCloseWindowCB,
			      SppCallbackData, window,
			      SppShortcut, SW_CLOSE_SHORTCUT,
			      SppHelpPath, "menu/file_menu.html#file_close",
			      NULL);
    menu_item = spAddMenuItem(menu, SP_QUIT_MENU_ITEM_NAME,
			      SppTitle, SW_MENU_QUIT_LABEL,
			      SppGroupId, SW_QUIT_GROUP_ID,
			      /*SppSenseLevel, SW_STATE_ONE_WINDOW,*/
			      SppCallbackFunc, swQuitCB,
			      SppCallbackData, window,
			      SppShortcut, SW_QUIT_SHORTCUT,
			      SppHelpPath, "menu/file_menu.html#file_exit",
			      NULL);
    
    /* `Edit' menu */
    menu = spCreatePulldownMenu(menu_bar, "editMenu",
				SppTitle, SW_MENU_EDIT_LABEL,
				SppHelpPath, "menu/edit_menu.html",
				NULL);
#ifdef SW_SUPPORT_EDIT
    window->undo_menu = spAddMenuItem(menu, "undoWave",
				      SppTitle, SW_MENU_UNDO_LABEL,
				      SppGroupId, SW_UNDO_GROUP_ID,
				      SppCallbackFunc, swUndoWindowCB,
				      SppCallbackData, window,
				      SppShortcut, SW_UNDO_SHORTCUT,
				      SppHelpPath, "menu/edit_menu.html#edit_undo",
				      NULL);
    window->redo_menu = spAddMenuItem(menu, "redoWave",
				      SppTitle, SW_MENU_REDO_LABEL,
				      SppGroupId, SW_UNDO_GROUP_ID,
				      SppCallbackFunc, swRedoWindowCB,
				      SppCallbackData, window,
				      SppShortcut, SW_REDO_SHORTCUT,
				      SppHelpPath, "menu/edit_menu.html#edit_redo",
				      NULL);
    spAddMenuSeparator(menu, "editSeparator", NULL);
    menu_item = spAddMenuItem(menu, "cropWave",
			      SppTitle, SW_MENU_CROP_LABEL,
			      SppGroupId, SW_DATA_GROUP_ID,
			      SppSenseLevel, SW_STATE_SELECT_TIME_DATA,
			      SppCallbackFunc, swCropWindowCB,
			      SppCallbackData, window,
			      SppShortcut, SW_CROP_SHORTCUT,
			      SppHelpPath, "menu/edit_menu.html#edit_crop",
			      NULL);
    menu_item = spAddMenuItem(menu, "deleteWave",
			      SppTitle, SW_MENU_DELETE_LABEL,
			      SppGroupId, SW_DATA_GROUP_ID,
			      SppSenseLevel, SW_STATE_SELECT_TIME_CHANNELS,
			      SppCallbackFunc, swDeleteWindowCB,
			      SppCallbackData, window,
			      /*SppShortcut, SW_DELETE_SHORTCUT,*/
			      SppHelpPath, "menu/edit_menu.html#edit_delete",
			      NULL);
    menu_item = spAddMenuItem(menu, "eraseWave",
			      SppTitle, SW_MENU_ERASE_LABEL,
			      SppGroupId, SW_DATA_GROUP_ID,
			      SppSenseLevel, SW_STATE_SELECT_TIME_DATA,
			      SppCallbackFunc, swEraseWindowCB,
			      SppCallbackData, window,
			      SppShortcut, SW_ERASE_SHORTCUT,
			      SppHelpPath, "menu/edit_menu.html#edit_erase",
			      NULL);
    menu_item = spAddMenuItem(menu, "extractWave",
			      SppTitle, SW_MENU_EXTRACT_LABEL,
			      SppGroupId, SW_DATA_GROUP_ID,
			      SppSenseLevel, SW_STATE_SELECT_TIME_DATA,
			      SppCallbackFunc, swExtractWindowCB,
			      SppCallbackData, window,
			      SppShortcut, SW_EXTRACT_SHORTCUT,
			      SppHelpPath, "menu/edit_menu.html#edit_extract",
			      NULL);
#ifdef SW_SUPPORT_AUTOSAVE
    if (swIsClipboardWindow(window) == SP_FALSE) {
	menu_item = spAddMenuItem(menu, "extractAutosaveWave",
				  SppTitle, SW_MENU_EXTRACT_AUTOSAVE_LABEL,
				  SppGroupId, SW_DATA_GROUP_ID,
				  SppSenseLevel, SW_STATE_SELECT_TIME_DATA,
				  SppCallbackFunc, swExtractAutosaveWindowCB,
				  SppCallbackData, window,
				  SppShortcut, SW_EXTRACT_AUTOSAVE_SHORTCUT,
				  SppHelpPath, "menu/edit_menu.html#edit_extract_autosave",
				  NULL);
	menu_item = spAddMenuItem(menu, "saveByNormalLabel",
				  SppTitle, SW_MENU_SAVE_BY_NORMAL_LABEL_LABEL,
				  SppGroupId, SW_NORMAL_LABEL_GROUP_ID,
				  SppSenseLevel, SW_STATE_EXIST_LABEL,
				  SppCallbackFunc, swPopupSaveByNormalLabelDialogCB,
				  SppCallbackData, window,
				  /*SppHelpPath, "menu/edit_menu.html#edit_save_by_normal_label",*/
				  NULL);
	menu_item = spAddMenuItem(menu, "saveByRegionLabel",
				  SppTitle, SW_MENU_SAVE_BY_REGION_LABEL_LABEL,
				  SppGroupId, SW_REGION_LABEL_GROUP_ID,
				  SppSenseLevel, SW_STATE_EXIST_LABEL,
				  SppCallbackFunc, swPopupSaveByRegionLabelDialogCB,
				  SppCallbackData, window,
				  /*SppHelpPath, "menu/edit_menu.html#edit_save_by_region_label",*/
				  NULL);
    }
#endif
#ifdef SW_SUPPORT_CLIPBOARD
    if (swIsClipboardWindow(window) == SP_FALSE) {
	spAddMenuSeparator(menu, "editClipboardSeparator", NULL);
	menu_item = spAddMenuItem(menu, "cutWave",
				  SppTitle, SW_MENU_CUT_LABEL,
				  SppGroupId, SW_CLIPBOARD_DATA_GROUP_ID,
				  SppSenseLevel, SW_STATE_SELECT_TIME_CHANNELS,
				  SppCallbackFunc, swCutWindowCB,
				  SppCallbackData, window,
				  SppShortcut, SW_CUT_SHORTCUT,
				  SppHelpPath, "menu/edit_menu.html#edit_cut",
				  NULL);
	menu_item = spAddMenuItem(menu, "copyWave",
				  SppTitle, SW_MENU_COPY_LABEL,
				  SppGroupId, SW_CLIPBOARD_DATA_GROUP_ID,
				  SppSenseLevel, SW_STATE_SELECT_TIME_DATA,
				  SppCallbackFunc, swCopyWindowCB,
				  SppCallbackData, window,
				  SppShortcut, SW_COPY_SHORTCUT,
				  SppHelpPath, "menu/edit_menu.html#edit_copy",
				  NULL);
	menu_item = spAddMenuItem(menu, "cat",
				  SppTitle, SW_MENU_CAT_LABEL,
				  SppGroupId, SW_CLIPBOARD_GROUP_ID,
				  SppSenseLevel, SW_STATE_EXIST_CLIPBOARD,
				  SppCallbackFunc, swCatWindowCB,
				  SppCallbackData, window,
				  SppShortcut, SW_CAT_SHORTCUT,
				  SppHelpPath, "menu/edit_menu.html#edit_concat",
				  NULL);
	menu_item = spAddMenuItem(menu, "cattop",
				  SppTitle, SW_MENU_CATTOP_LABEL,
				  SppGroupId, SW_CLIPBOARD_GROUP_ID,
				  SppSenseLevel, SW_STATE_EXIST_CLIPBOARD,
				  SppCallbackFunc, swCatTopWindowCB,
				  SppCallbackData, window,
				  SppShortcut, SW_CATTOP_SHORTCUT,
				  SppHelpPath, "menu/edit_menu.html#edit_concat_top",
				  NULL);
	menu_item = spAddMenuItem(menu, "clipboard",
				  SppTitle, SW_MENU_CLIPBOARD_WINDOW_LABEL,
				  SppCallbackFunc, swPopupClipboardWindowCB,
				  SppCallbackData, window->config,
				  SppHelpPath, "menu/edit_menu.html#edit_clipboard_window",
				  NULL);
    }
#endif
    spAddMenuSeparator(menu, "editGainSeparator", NULL);
    menu_item = spAddMenuItem(menu, "amplifyWave",
			      SppTitle, SW_MENU_AMPLIFY_LABEL,
			      SppGroupId, SW_DATA_GROUP_ID,
			      SppSenseLevel, SW_STATE_SELECT_TIME_DATA,
			      SppCallbackFunc, swPopupAmplifyDialogCB,
			      SppCallbackData, window,
			      SppHelpPath, "menu/edit_menu.html#edit_amplify",
			      NULL);
    menu_item = spAddMenuItem(menu, "invertWave",
			      SppTitle, SW_MENU_INVERT_LABEL,
			      SppGroupId, SW_DATA_GROUP_ID,
			      SppSenseLevel, SW_STATE_SELECT_TIME_DATA,
			      SppCallbackFunc, swInvertWindowCB,
			      SppCallbackData, window,
			      SppHelpPath, "menu/edit_menu.html#edit_invert",
			      NULL);
    menu_item = spAddMenuItem(menu, "fadeInWave",
			      SppTitle, SW_MENU_FADE_IN_LABEL,
			      SppGroupId, SW_DATA_GROUP_ID,
			      SppSenseLevel, SW_STATE_SELECT_TIME_DATA,
			      SppCallbackFunc, swFadeInWindowCB,
			      SppCallbackData, window,
			      SppHelpPath, "menu/edit_menu.html#edit_fade_in",
			      NULL);
    menu_item = spAddMenuItem(menu, "fadeOutWave",
			      SppTitle, SW_MENU_FADE_OUT_LABEL,
			      SppGroupId, SW_DATA_GROUP_ID,
			      SppSenseLevel, SW_STATE_SELECT_TIME_DATA,
			      SppCallbackFunc, swFadeOutWindowCB,
			      SppCallbackData, window,
			      SppHelpPath, "menu/edit_menu.html#edit_fade_out",
			      NULL);
    menu_item = spAddMenuItem(menu, "swapWaveChannel",
			      SppTitle, SW_MENU_CHANNEL_SWAP_LABEL,
			      SppGroupId, SW_DATA_GROUP_ID,
			      SppSenseLevel, SW_STATE_SELECT_TIME_MULTI_CHANNEL,
			      SppCallbackFunc, swSwapWaveChannelCB,
			      SppCallbackData, window,
			      SppHelpPath, "menu/edit_menu.html#edit_channel_swap",
			      NULL);
    
    spAddMenuSeparator(menu, "editFileSeparator", NULL);
    menu_item = spAddMenuItem(menu, "maximizeWave",
			      SppTitle, SW_MENU_MAXIMIZE_LABEL,
			      SppGroupId, SW_DATA_GROUP_ID,
			      SppSenseLevel, SW_STATE_TIME_DATA,
			      SppCallbackFunc, swPopupMaximizeDialogCB,
			      SppCallbackData, window,
			      SppHelpPath, "menu/edit_menu.html#edit_maximize",
			      NULL);
    menu_item = spAddMenuItem(menu, "bitConv",
			      SppTitle, SW_MENU_BIT_CONV_LABEL,
			      SppGroupId, SW_DATA_GROUP_ID,
			      SppSenseLevel, SW_STATE_TIME_DATA,
			      SppCallbackFunc, swPopupBitConvDialogCB,
			      SppCallbackData, window,
			      SppHelpPath, "menu/edit_menu.html#edit_bit_conv",
			      NULL);
    menu_item = spAddMenuItem(menu, "sampFreqConv",
			      SppTitle, SW_MENU_SAMP_RATE_CONV_LABEL,
			      SppGroupId, SW_DATA_GROUP_ID,
			      SppSenseLevel, SW_STATE_TIME_DATA,
			      SppCallbackFunc, swPopupSampFreqConvDialogCB,
			      SppCallbackData, window,
			      SppHelpPath, "menu/edit_menu.html#edit_samp_freq_conv",
			      NULL);
    menu_item = spAddMenuItem(menu, "monauralize",
			      SppGroupId, SW_CHANNEL_GROUP_ID,
			      SppSenseLevel, SW_STATE_STEREO_WAVE,
			      SppTitle, SW_MENU_MONAURALIZE_LABEL,
			      SppCallbackFunc, swMonauralizeCB,
			      SppCallbackData, window,
			      SppHelpPath, "menu/edit_menu.html#edit_monauralize",
			      NULL);
    menu_item = spAddMenuItem(menu, "filtering",
			      SppTitle, _("SW_MENU_FILTERING_LABEL"),
			      SppGroupId, SW_DATA_GROUP_ID,
			      SppSenseLevel, SW_STATE_TIME_DATA,
			      SppCallbackFunc, swPopupFilteringDialogCB,
			      SppCallbackData, window,
			      SppHelpPath, "menu/edit_menu.html#edit_filtering",
			      NULL);
#ifdef SW_SUPPORT_INFO_DIALOG
    menu_item = spAddMenuItem(menu, "infoDialog",
			      SppTitle, SW_MENU_INFO_DIALOG_LABEL,
			      SppCallbackFunc, swPopupInfoDialogCB,
			      SppCallbackData, window,
			      SppSenseLevel, SW_STATE_EXIST_WAVE,
			      SppHelpPath, "menu/edit_menu.html#edit_metadata",
			      NULL);
#endif
    spAddMenuSeparator(menu, "editPrefSeparator", NULL);
#endif
    menu_item = spAddMenuItem(menu, "preferenceDialog",
			      SppTitle, SW_MENU_PREFERENCE_LABEL,
			      SppCallbackFunc, swPopupPreferenceDialogCB,
			      SppCallbackData, window,
			      SppShortcut, SW_PREFERENCE_SHORTCUT,
			      SppHelpPath, "menu/edit_menu.html#preference",
			      NULL);
#ifdef SW_USE_ANALYSIS
    menu_item = spAddMenuItem(menu, "analysisDialog",
			      SppTitle, SW_MENU_ANALYSIS_PREFERENCE_LABEL,
			      SppCallbackFunc, swPopupAnalysisDialogCB,
			      SppCallbackData, window,
			      SppHelpPath, "menu/edit_menu.html#analysis_preference",
			      NULL);
#endif

    /* `Page' menu */
    menu = spCreatePulldownMenu(menu_bar, "pageMenu",
				SppTitle, SW_MENU_PAGE_LABEL,
				SppHelpPath, "menu/page_menu.html",
				NULL);
    menu_item = spAddMenuItem(menu, "forward",
			      SppTitle, SW_MENU_FORWARD_LABEL,
			      SppGroupId, SW_PAGE_GROUP_ID,
			      SppSenseLevel, SW_STATE_EXIST_WAVE,
			      SppCallbackFunc, swForwardCB,
			      SppCallbackData, window,
			      SppShortcut, SW_FORWARD_SHORTCUT,
			      SppHelpPath, "menu/page_menu.html#page_forward",
			      NULL);
    menu_item = spAddMenuItem(menu, "backward",
			      SppTitle, SW_MENU_BACKWARD_LABEL,
			      SppGroupId, SW_PAGE_GROUP_ID,
			      SppSenseLevel, SW_STATE_EXIST_WAVE,
			      SppCallbackFunc, swBackwardCB,
			      SppCallbackData, window,
			      SppShortcut, SW_BACKWARD_SHORTCUT,
			      SppHelpPath, "menu/page_menu.html#page_backward",
			      NULL);
    menu_item = spAddMenuItem(menu, "goHead",
			      SppTitle, SW_MENU_GO_HEAD_LABEL,
			      SppGroupId, SW_PAGE_GROUP_ID,
			      SppSenseLevel, SW_STATE_EXIST_WAVE,
			      SppCallbackFunc, swGoHeadCB,
			      SppCallbackData, window,
			      SppShortcut, SW_GO_HEAD_SHORTCUT,
			      SppHelpPath, "menu/page_menu.html#page_go_head",
			      NULL);
    menu_item = spAddMenuItem(menu, "goTail",
			      SppTitle, SW_MENU_GO_TAIL_LABEL,
			      SppGroupId, SW_PAGE_GROUP_ID,
			      SppSenseLevel, SW_STATE_EXIST_WAVE,
			      SppCallbackFunc, swGoTailCB,
			      SppCallbackData, window,
			      SppShortcut, SW_GO_TAIL_SHORTCUT,
			      SppHelpPath, "menu/page_menu.html#page_go_tail",
			      NULL);
    menu_item = spAddMenuItem(menu, "nextWindow",
			      SppTitle, SW_MENU_NEXT_WINDOW_LABEL,
			      SppGroupId, SW_WINDOW_GROUP_ID,
			      SppSenseLevel, SW_STATE_SOME_WINDOWS,
			      SppCallbackFunc, swNextWindowCB,
			      SppCallbackData, window,
			      SppShortcut, SW_NEXT_WINDOW_SHORTCUT,
			      SppHelpPath, "menu/page_menu.html#page_next",
			      NULL);
    menu_item = spAddMenuItem(menu, "prevWindow",
			      SppTitle, SW_MENU_PREV_WINDOW_LABEL,
			      SppGroupId, SW_WINDOW_GROUP_ID,
			      SppSenseLevel, SW_STATE_SOME_WINDOWS,
			      SppCallbackFunc, swPrevWindowCB,
			      SppCallbackData, window,
			      SppShortcut, SW_PREV_WINDOW_SHORTCUT,
			      SppHelpPath, "menu/page_menu.html#page_previous",
			      NULL);
    menu_item = spAddMenuItem(menu, "alignWindow",
			      SppTitle, SW_MENU_ALIGN_WINDOW_LABEL,
			      SppGroupId, SW_WINDOW_GROUP_ID,
			      SppSenseLevel, SW_STATE_STOP_SOME_WINDOWS,
			      SppCallbackFunc, swAlignWindowCB,
			      SppCallbackData, window,
			      SppShortcut, SW_ALIGN_WINDOW_SHORTCUT,
			      SppHelpPath, "menu/page_menu.html#page_align",
			      NULL);
    spAddMenuSeparator(menu, "pageSeparator", NULL);
    menu_item = spAddMenuItem(menu, "selectRegion",
			      SppTitle, SW_MENU_SELECT_REGION_LABEL,
			      SppSenseLevel, SW_STATE_EXIST_WAVE,
			      SppCallbackFunc, swSelectRegionCB,
			      SppCallbackData, window,
			      SppShortcut, SW_SELECT_REGION_SHORTCUT,
			      SppHelpPath, "menu/page_menu.html#page_select",
			      NULL);
    menu_item = spAddMenuItem(menu, "selectAll",
			      SppTitle, SW_MENU_SELECT_ALL_LABEL,
			      SppSenseLevel, SW_STATE_EXIST_WAVE,
			      SppCallbackFunc, swSelectAllCB,
			      SppCallbackData, window,
			      SppShortcut, SW_SELECT_ALL_SHORTCUT,
			      SppHelpPath, "menu/page_menu.html#page_select_all",
			      NULL);
    menu_item = spAddMenuItem(menu, "selectAllRegion",
			      SppTitle, SW_MENU_SELECT_ALL_REGION_LABEL,
			      SppGroupId, SW_WINDOW_GROUP_ID,
			      SppSenseLevel, SW_STATE_SOME_WINDOWS,
			      SppCallbackFunc, swSelectAllRegionCB,
			      SppCallbackData, window,
			      SppShortcut, SW_SELECT_ALL_REGION_SHORTCUT,
			      SppHelpPath, "menu/page_menu.html#page_select_all_region",
			      NULL);
    menu_item = spAddMenuItem(menu, "selectNextChannel",
			      SppTitle, SW_MENU_SELECT_NEXT_CHANNEL_LABEL,
			      SppGroupId, SW_DATA_GROUP_ID,
			      SppSenseLevel, SW_STATE_SELECT_TIME_DATA,
			      SppCallbackFunc, swSelectNextChannelCB,
			      SppCallbackData, window,
			      SppShortcut, SW_SELECT_NEXT_CHANNEL_SHORTCUT,
			      SppHelpPath, "menu/page_menu.html#page_select_next_channel",
			      NULL);

    /* `View' menu */
    menu = spCreatePulldownMenu(menu_bar, "viewMenu",
				SppTitle, SW_MENU_VIEW_LABEL,
				SppHelpPath, "menu/view_menu.html",
				NULL);
    menu_item = spAddMenuItem(menu, "zoomIn",
			      SppTitle, SW_MENU_ZOOM_IN_LABEL,
			      SppGroupId, SW_PAGE_GROUP_ID,
			      SppSenseLevel, SW_STATE_EXIST_WAVE,
			      SppCallbackFunc, swZoomInCB,
			      SppCallbackData, window,
			      SppShortcut, SW_ZOOM_IN_SHORTCUT,
			      SppHelpPath, "menu/view_menu.html#view_zoom_in",
			      NULL);
    menu_item = spAddMenuItem(menu, "zoomOut",
			      SppTitle, SW_MENU_ZOOM_OUT_LABEL,
			      SppGroupId, SW_PAGE_GROUP_ID,
			      SppSenseLevel, SW_STATE_EXIST_WAVE,
			      SppCallbackFunc, swZoomOutCB,
			      SppCallbackData, window,
			      SppShortcut, SW_ZOOM_OUT_SHORTCUT,
			      SppHelpPath, "menu/view_menu.html#view_zoom_out",
			      NULL);
    menu_item = spAddMenuItem(menu, "zoomFullOut",
			      SppTitle, SW_MENU_ZOOM_FULL_OUT_LABEL,
			      SppGroupId, SW_PAGE_GROUP_ID,
			      SppSenseLevel, SW_STATE_EXIST_WAVE,
			      SppCallbackFunc, swZoomFullOutCB,
			      SppCallbackData, window,
			      SppShortcut, SW_ZOOM_FULL_OUT_SHORTCUT,
			      SppHelpPath, "menu/view_menu.html#view_zoom_full_out",
			      NULL);
    menu_item = spAddMenuItem(menu, "zoomRegion",
			      SppTitle, SW_MENU_ZOOM_REGION_LABEL,
			      SppGroupId, SW_PAGE_GROUP_ID,
			      SppSenseLevel, SW_STATE_SELECT_WAVE,
			      SppCallbackFunc, swZoomRegionCB,
			      SppCallbackData, window,
			      SppShortcut, SW_ZOOM_REGION_SHORTCUT,
			      SppHelpPath, "menu/view_menu.html#view_zoom_region",
			      NULL);

    sub_menu = spAddSubMenu(menu, "timeFormat",
			    SppTitle, SW_MENU_TIME_FORMAT_LABEL,
			    SppHelpPath, "menu/view_menu.html#view_time",
			    NULL);
    menu_item = spAddRadioButtonMenuItem(sub_menu, "timeFormatSec",
					 SppTitle, SW_MENU_TIME_FORMAT_SEC_LABEL,
					 SppGroupId, SW_DATA_GROUP_ID,
					 SppSenseLevel, SW_STATE_TIME_DATA,
					 SppCallbackFunc, swChangeTimeFormatCB,
					 SppCallbackData, window,
					 SppSet, (window->config->time_format == SW_TIME_FORMAT_SEC ?
						  SP_TRUE : SP_FALSE),
					 SppHelpPath, "menu/view_menu.html#time_sec",
					 NULL);
    menu_item = spAddRadioButtonMenuItem(sub_menu, "timeFormatMsec",
					 SppTitle, SW_MENU_TIME_FORMAT_MSEC_LABEL,
					 SppGroupId, SW_DATA_GROUP_ID,
					 SppSenseLevel, SW_STATE_TIME_DATA,
					 SppCallbackFunc, swChangeTimeFormatCB,
					 SppCallbackData, window,
					 SppSet, (window->config->time_format == SW_TIME_FORMAT_MSEC ?
						  SP_TRUE : SP_FALSE),
					 SppHelpPath, "menu/view_menu.html#time_msec",
					 NULL);
    menu_item = spAddRadioButtonMenuItem(sub_menu, "timeFormatPoint",
					 SppTitle, SW_MENU_TIME_FORMAT_POINT_LABEL,
					 SppGroupId, SW_DATA_GROUP_ID,
					 SppSenseLevel, SW_STATE_TIME_DATA,
					 SppCallbackFunc, swChangeTimeFormatCB,
					 SppCallbackData, window,
					 SppSet, (window->config->time_format == SW_TIME_FORMAT_POINT ?
						  SP_TRUE : SP_FALSE),
					 SppHelpPath, "menu/view_menu.html#time_point",
					 NULL);
    menu_item = spAddRadioButtonMenuItem(sub_menu, "timeFormatSeparatedSec",
					 SppTitle, SW_MENU_TIME_FORMAT_SEPARATED_SEC_LABEL,
					 SppGroupId, SW_DATA_GROUP_ID,
					 SppSenseLevel, SW_STATE_TIME_DATA,
					 SppCallbackFunc, swChangeTimeFormatCB,
					 SppCallbackData, window,
					 SppSet, (window->config->time_format == SW_TIME_FORMAT_SEPARATED_SEC ?
						  SP_TRUE : SP_FALSE),
					 SppHelpPath, "menu/view_menu.html#time_separated_sec",
					 NULL);
    window->time_format_menu = sub_menu;
    
    spAddMenuSeparator(menu, "viewSeparator", NULL);
    menu_item = spAddMenuItem(menu, "zoomInAmplitude",
			      SppTitle, SW_MENU_ZOOM_IN_AMPLITUDE_LABEL,
			      SppGroupId, SW_PAGE_GROUP_ID,
			      SppSenseLevel, SW_STATE_EXIST_WAVE,
			      SppCallbackFunc, swZoomInAmplitudeCB,
			      SppCallbackData, window,
			      SppHelpPath, "menu/view_menu.html#view_zoom_in_vaxis",
			      NULL);
    menu_item = spAddMenuItem(menu, "zoomOutAmplitude",
			      SppTitle, SW_MENU_ZOOM_OUT_AMPLITUDE_LABEL,
			      SppGroupId, SW_PAGE_GROUP_ID,
			      SppSenseLevel, SW_STATE_EXIST_WAVE,
			      SppCallbackFunc, swZoomOutAmplitudeCB,
			      SppCallbackData, window,
			      SppHelpPath, "menu/view_menu.html#view_zoom_out_vaxis",
			      NULL);
    menu_item = spAddMenuItem(menu, "zoomFullOutAmplitude",
			      SppTitle, SW_MENU_ZOOM_FULL_OUT_AMPLITUDE_LABEL,
			      SppGroupId, SW_PAGE_GROUP_ID,
			      SppSenseLevel, SW_STATE_EXIST_WAVE,
			      SppCallbackFunc, swZoomFullOutAmplitudeCB,
			      SppCallbackData, window,
			      SppHelpPath, "menu/view_menu.html#view_zoom_full_out_vaxis",
			      NULL);
    menu_item = spAddMenuItem(menu, "alignAmplitude",
			      SppTitle, SW_MENU_ALIGN_AMPLITUDE_LABEL,
			      SppGroupId, SW_WINDOW_GROUP_ID,
			      SppSenseLevel, SW_STATE_STOP_SOME_WINDOWS,
			      SppCallbackFunc, swAlignAmplitudeCB,
			      SppCallbackData, window,
			      SppHelpPath, "menu/view_menu.html#view_align_vaxis",
			      NULL);
    
    spAddMenuSeparator(menu, "viewSeparator2", NULL);
    menu_item = spAddCheckBoxMenuItem(menu, "displayInfoArea",
				      SppTitle, SW_MENU_DISPLAY_INFO_AREA_LABEL,
				      SppSet, window->display_info_area,
				      SppCallbackFunc, swCheckDisplayInfoAreaCB,
				      SppCallbackData, window,
				      SppShortcut, SW_DISPLAY_INFO_AREA_SHORTCUT,
				      SppHelpPath, "menu/view_menu.html#view_display_info_area",
				      NULL);
    menu_item = spAddCheckBoxMenuItem(menu, "drawDetail",
				      SppTitle, SW_MENU_DRAW_DETAIL_LABEL,
				      SppSet, window->draw_detail,
				      SppCallbackFunc, swCheckDrawDetailCB,
				      SppCallbackData, window,
				      SppHelpPath, "menu/view_menu.html#view_draw_detail",
				      NULL);
    menu_item = spAddCheckBoxMenuItem(menu, "drawLabel",
				      SppTitle, SW_MENU_DRAW_LABEL_LABEL,
				      SppSet, window->draw_label,
				      SppCallbackFunc, swCheckDrawLabelCB,
				      SppCallbackData, window,
				      SppHelpPath, "menu/view_menu.html#view_draw_label",
				      NULL);
#ifdef SW_SUPPORT_LOG_FREQUENCY_AXIS
    window->log_frequency_axis_menu = spAddCheckBoxMenuItem(menu, "logFrequencyAxisMenuItem",
                                                            SppTitle, SW_MENU_LOG_FREQUENCY_AXIS_LABEL,
                                                            SppSet, window->log_frequency_axis,
                                                            SppCallbackFunc, swCheckLogFrequencyAxisCB,
                                                            SppCallbackData, window,
                                                            SppHelpPath, "menu/view_menu.html#log_frequency_axis",
                                                            NULL);
#endif
#ifdef SW_SUPPORT_ANALYSIS_SUBPLOT
    sub_menu = spAddSubMenu(menu, "analysisSubplot",
			    SppTitle, SW_MENU_ANALYSIS_SUBPLOT_LABEL,
			    NULL);
    menu_item = spAddCheckBoxMenuItem(sub_menu, "subplotSpectrogram",
				      SppTitle, SW_MENU_ANALYSIS_SUBPLOT_SPECTROGRAM_LABEL,
				      SppGroupId, SW_DATA_GROUP_ID,
				      SppSenseLevel, SW_STATE_TIME_DATA,
				      SppCallbackFunc, swSubplotSpectrogramCB,
				      SppCallbackData, window,
				      SppSet, SP_FALSE,
				      NULL);
    menu_item = spAddCheckBoxMenuItem(sub_menu, "subplotF0",
				      SppTitle, SW_MENU_ANALYSIS_SUBPLOT_F0_LABEL,
				      SppGroupId, SW_DATA_GROUP_ID,
				      SppSenseLevel, SW_STATE_TIME_DATA,
				      SppCallbackFunc, swSubplotSpectrogramCB,
				      SppCallbackData, window,
				      SppSet, SP_FALSE,
				      NULL);
    menu_item = spAddCheckBoxMenuItem(sub_menu, "subplotPower",
				      SppTitle, SW_MENU_ANALYSIS_SUBPLOT_POWER_LABEL,
				      SppGroupId, SW_DATA_GROUP_ID,
				      SppSenseLevel, SW_STATE_TIME_DATA,
				      SppCallbackFunc, swSubplotSpectrogramCB,
				      SppCallbackData, window,
				      SppSet, SP_FALSE,
				      NULL);
#ifdef SW_SUPPORT_STRAIGHT
    menu_item = spAddCheckBoxMenuItem(sub_menu, "subplotStraightSpecgram",
				      SppTitle, SW_MENU_ANALYSIS_SUBPLOT_STRAIGHT_SPECGRAM_LABEL,
				      SppGroupId, SW_DATA_GROUP_ID,
				      SppSenseLevel, SW_STATE_TIME_DATA,
				      SppCallbackFunc, swSubplotSpectrogramCB,
				      SppCallbackData, window,
				      SppSet, SP_FALSE,
				      NULL);
    menu_item = spAddCheckBoxMenuItem(sub_menu, "subplotAperiodicity",
				      SppTitle, SW_MENU_ANALYSIS_SUBPLOT_APERIODICITY_LABEL,
				      SppGroupId, SW_DATA_GROUP_ID,
				      SppSenseLevel, SW_STATE_TIME_DATA,
				      SppCallbackFunc, swSubplotSpectrogramCB,
				      SppCallbackData, window,
				      SppSet, SP_FALSE,
				      NULL);
    menu_item = spAddCheckBoxMenuItem(sub_menu, "subplotStraightF0",
				      SppTitle, SW_MENU_ANALYSIS_SUBPLOT_STRAIGHT_F0_LABEL,
				      SppGroupId, SW_DATA_GROUP_ID,
				      SppSenseLevel, SW_STATE_TIME_DATA,
				      SppCallbackFunc, swSubplotSpectrogramCB,
				      SppCallbackData, window,
				      SppSet, SP_FALSE,
				      NULL);
#endif
    window->subplot_menu = sub_menu;
#endif
    sub_menu = spAddSubMenu(menu, "spectrogram",
			    SppTitle, SW_MENU_SPECTROGRAM_LABEL,
			    SppHelpPath, "menu/view_menu.html#view_spectrogram",
			    NULL);
    menu_item = spAddRadioButtonMenuItem(sub_menu, "noSpectrogram",
					 SppTitle, SW_MENU_NO_SPECTROGRAM_LABEL,
					 SppGroupId, SW_DATA_GROUP_ID,
					 SppSenseLevel, SW_STATE_TIME_DATA,
					 SppCallbackFunc, swClearSpectrogramCB,
					 SppCallbackData, window,
					 SppSet, SP_TRUE,
					 SppHelpPath, "menu/view_menu.html#view_no_spectrogram",
					 NULL);
    menu_item = spAddRadioButtonMenuItem(sub_menu, "wideSpectrogram",
					 SppTitle, SW_MENU_WIDE_SPECTROGRAM_LABEL,
					 SppGroupId, SW_DATA_GROUP_ID,
					 SppSenseLevel, SW_STATE_TIME_DATA,
					 SppCallbackFunc, swDrawWideSpectrogramCB,
					 SppCallbackData, window,
					 SppSet, SP_FALSE,
					 SppHelpPath, "menu/view_menu.html#view_wide_spectrogram",
					 NULL);
    menu_item = spAddRadioButtonMenuItem(sub_menu, "narrowSpectrogram",
					 SppTitle, SW_MENU_NARROW_SPECTROGRAM_LABEL,
					 SppGroupId, SW_DATA_GROUP_ID,
					 SppSenseLevel, SW_STATE_TIME_DATA,
					 SppCallbackFunc, swDrawNarrowSpectrogramCB,
					 SppCallbackData, window,
					 SppSet, SP_FALSE,
					 SppHelpPath, "menu/view_menu.html#view_narrow_spectrogram",
					 NULL);
    menu_item = spAddRadioButtonMenuItem(sub_menu, "narrowSmoothedSpectrogram",
					 SppTitle, SW_MENU_NARROW_SMOOTHED_SPECTROGRAM_LABEL,
					 SppGroupId, SW_DATA_GROUP_ID,
					 SppSenseLevel, SW_STATE_TIME_DATA,
					 SppCallbackFunc, swDrawNarrowSmoothedSpectrogramCB,
					 SppCallbackData, window,
					 SppSet, SP_FALSE,
					 SppHelpPath, "menu/view_menu.html#view_narrow_smoothed_spectrogram",
					 NULL);
#if defined(SW_SUPPORT_CQT_SPECTROGRAM)
    menu_item = spAddRadioButtonMenuItem(sub_menu, "cqtSpectrogram",
					 SppTitle, SW_MENU_CQT_SPECTROGRAM_LABEL,
					 SppGroupId, SW_DATA_GROUP_ID,
					 SppSenseLevel, SW_STATE_TIME_DATA,
					 SppCallbackFunc, swDrawCQTSpectrogramCB,
					 SppCallbackData, window,
					 SppSet, SP_FALSE,
					 SppHelpPath, "menu/view_menu.html#view_cqt_spectrogram",
					 NULL);
#endif
    spAddMenuSeparator(sub_menu, "spectrogramSeparator", NULL);
    menu_item = spAddCheckBoxMenuItem(sub_menu, "spectrogramDrawKeys",
				      SppTitle, SW_MENU_SPECTROGRAM_DRAW_KEYS_LABEL,
                                      SppGroupId, SW_DATA_GROUP_ID,
                                      SppSenseLevel, SW_STATE_TIME_DATA,
				      SppSet, window->config->draw_piano_keys_for_specgram,
				      SppCallbackFunc, swCheckSpectrogramDrawKeysCB,
				      SppCallbackData, window,
				      SppHelpPath, "menu/view_menu.html#view_draw_spectrogram_keys",
				      NULL);
    spAddMenuSeparator(sub_menu, "spectrogramSeparator2", NULL);
    menu_item = spAddMenuItem(sub_menu, "updateSpectrogram",
			      SppTitle, SW_MENU_UPDATE_SPECTROGRAM_LABEL,
			      SppGroupId, SW_DATA_GROUP_ID,
			      SppSenseLevel, SW_STATE_TIME_DATA,
			      SppCallbackFunc, swUpdateSpectrogramCB,
			      SppCallbackData, window,
			      SppHelpPath, "menu/view_menu.html#view_update_spectrogram",
			      NULL);
    menu_item = spAddMenuItem(sub_menu, "spectrogramDialog",
			      SppTitle, SW_MENU_SPECTROGRAM_DIALOG_LABEL,
			      SppGroupId, SW_DATA_GROUP_ID,
			      SppSenseLevel, SW_STATE_TIME_DATA,
			      SppCallbackFunc, swPopupSpectrogramDialogCB,
			      SppCallbackData, window,
			      SppHelpPath, "menu/view_menu.html#view_spectrogram_dialog",
			      NULL);
    window->specgram_menu = sub_menu;
    
    /* `Sound' menu */
    menu = spCreatePulldownMenu(menu_bar, "soundMenu",
				SppTitle, SW_MENU_SOUND_LABEL,
				SppHelpPath, "menu/sound_menu.html",
				NULL);
    menu_item = spAddMenuItem(menu, "playWave",
			      SppTitle, SW_MENU_PLAY_LABEL,
			      SppGroupId, SW_PLAY_GROUP_ID,
			      SppSenseLevel, SW_STATE_NOT_PLAY_WAVE,
			      SppCallbackFunc, swPlayRegionCB,
			      SppCallbackData, window,
			      SppShortcut, SW_PLAY_SHORTCUT,
			      SppHelpPath, "menu/sound_menu.html#sound_play",
			      NULL);
    menu_item = spAddMenuItem(menu, "playWindow",
			      SppTitle, SW_MENU_PLAY_WINDOW_LABEL,
			      SppGroupId, SW_PLAY_GROUP_ID,
			      SppSenseLevel, SW_STATE_NOT_PLAY_WAVE,
			      SppCallbackFunc, swPlayWindowCB,
			      SppCallbackData, window,
			      SppShortcut, SW_PLAY_WINDOW_SHORTCUT,
			      SppHelpPath, "menu/sound_menu.html#sound_play_window",
			      NULL);
    menu_item = spAddMenuItem(menu, "playFile",
			      SppTitle, SW_MENU_PLAY_FILE_LABEL,
			      SppGroupId, SW_PLAY_GROUP_ID,
			      SppSenseLevel, SW_STATE_NOT_PLAY_WAVE,
			      SppCallbackFunc, swPlayFileCB,
			      SppCallbackData, window,
			      SppShortcut, SW_PLAY_FILE_SHORTCUT,
			      SppHelpPath, "menu/sound_menu.html#sound_play_file",
			      NULL);
    menu_item = spAddMenuItem(menu, "playStop",
			      SppTitle, SW_MENU_PLAY_STOP_LABEL,
			      SppGroupId, SW_DATA_GROUP_ID,
			      SppSenseLevel, SW_STATE_PLAY_TIME_DATA,
			      SppCallbackFunc, swPlayStopCB,
			      SppCallbackData, window,
			      SppShortcut, SW_PLAY_STOP_SHORTCUT,
			      SppHelpPath, "menu/sound_menu.html#sound_stop",
			      NULL);
#ifdef SP_SUPPORT_AUDIO
    spAddMenuSeparator(menu, "soundSeparator", NULL);
    window->loop_play_menu = spAddCheckBoxMenuItem(menu, "loopPlay",
						   SppTitle, SW_MENU_LOOP_PLAY_LABEL,
						   SppGroupId, SW_DATA_GROUP_ID,
						   /*SppSenseLevel, SW_STATE_PLAY_TIME_DATA,*/
						   SppSenseLevel, SW_STATE_TIME_DATA,
						   SppSet, window->loop_play,
						   SppCallbackFunc, swToggleLoopPlayCB,
						   SppCallbackData, window,
						   SppShortcut, SW_LOOP_PLAY_SHORTCUT,
						   SppHelpPath, "menu/sound_menu.html#sound_loop_play",
						   NULL);
    window->sync_play_menu = spAddCheckBoxMenuItem(menu, "syncPlay",
						   SppTitle, SW_MENU_SYNC_PLAY_LABEL,
						   SppGroupId, SW_DATA_GROUP_ID,
						   /*SppSenseLevel, SW_STATE_TIME_DATA,*/
						   SppSenseLevel, SW_STATE_PLAY_TIME_DATA,
						   SppSet, window->sync_play,
						   SppCallbackFunc, swToggleSyncPlayCB,
						   SppCallbackData, window,
						   SppShortcut, SW_SYNC_PLAY_SHORTCUT,
						   SppHelpPath, "menu/sound_menu.html#sound_sync_play",
						   NULL);
#endif
    window->pause_cursor_menu = spAddCheckBoxMenuItem(menu, "pauseCursorMenuItem",
						      SppTitle, _("SW_MENU_PAUSE_CURSOR_LABEL"),
						      SppGroupId, SW_PAGE_GROUP_ID,
						      SppSenseLevel, SW_STATE_EXIST_WAVE,
						      SppSet, window->pause_cursor,
						      SppCallbackFunc, swPauseCursorCB,
						      SppCallbackData, window,
						      SppShortcut, SW_PAUSE_CURSOR_SHORTCUT,
						      SppHelpPath, "menu/sound_menu.html#pause_cursor",
						      NULL);

    /* `Help' menu */
    menu = spCreatePulldownMenu(menu_bar, "helpMenu",
				SppTitle, SW_MENU_HELP_LABEL,
				SppMenuHelp, SP_TRUE,
				NULL);
    menu_item = spAddMenuItem(menu, "displayHelp",
			      SppTitle, SW_MENU_DISPLAY_HELP_LABEL,
			      SppCallbackFunc, swDisplayHelpCB,
			      NULL);
    spAddMenuSeparator(menu, "helpSeparator", NULL);
    menu_item = spAddMenuItem(menu, "info",
			      SppTitle, SW_MENU_INFO_LABEL,
			      /*SppSenseLevel, SW_STATE_NO_WAVE,*/
			      SppCallbackFunc, swDisplayInfoCB,
			      SppShortcut, SW_INFO_SHORTCUT,
			      NULL);
    sub_menu = spAddSubMenu(menu, "pluginInfo",
			    SppTitle, SW_MENU_PLUGIN_INFO_LABEL,
			    NULL);
    swCreatePluginInfoMenu(window, sub_menu);

    swCreateToolBar(window);
    
    /* create container for waveform canvas */
    width = window->width;
    default_slider_width = spGetScrollBarDefaultWidth();
    if (window->display_info_area == SP_TRUE) {
	size = -(window->config->info_area_width + default_slider_width);
	width = MAX(width - (window->config->info_area_width + default_slider_width), SW_MIN_WINDOW_WIDTH);
    } else {
	size = -default_slider_width;
    }
    window->wave_box = spCreateBox(window->window, "waveBox", size,
				   SppInitialWidth, width,
				   SppInitialHeight, window->height + window->config->overview_height,
				   NULL);
    
    spCreateCanvas(window->wave_box, "overviewCanvas",
		   width, window->config->overview_height,
		   SppBorderOn, SP_TRUE,
		   SppUseArrowKey, SP_TRUE,
		   SppHeight, window->config->overview_height,
		   SppCallbackFunc, swDrawOverviewCB,
		   SppCallbackData, window,
		   SppWheelTargetHorizontal, SP_TRUE,
		   SppAcceptWheelEvent, SP_TRUE,
		   NULL);

    spCreateCanvas(window->wave_box, "canvas",
		   width, window->height,
		   SppBorderOn, SP_TRUE,
		   SppUseArrowKey, SP_TRUE,
		   SppHeight, -1 * default_slider_width,
		   SppCallbackFunc, swDrawWaveCB,
		   SppCallbackData, window,
		   SppWheelTargetHorizontal, SP_TRUE,
		   SppAcceptWheelEvent, SP_TRUE,
		   NULL);
    
    if (swIsNoWave(window) == SP_FALSE) {
	window->scroll_coef = swGetScrollCoef(window->wave);
	
	window->hscroll = spCreateScrollBar(window->wave_box, "hscroll",
					    SppGroupId, SW_PAGE_GROUP_ID,
					    SppSenseLevel, SW_STATE_EXIST_WAVE,
					    SppHeight, default_slider_width,
					    SppOrientation, SP_HORIZONTAL,
					    SppTrackCallbackOn, SP_TRUE,
					    SppMinimum, 0,
					    SppMaximum, (int)(window->wave->total_length / window->scroll_coef),
					    SppIncrement, (int)MAX(window->length / window->scroll_coef / 4, 1),
					    SppPageIncrement, (int)MAX(window->length / window->scroll_coef, 1),
					    SppSliderSize, (int)MAX(window->length / window->scroll_coef, 1),
					    SppValue, (int)(window->offset / window->scroll_coef),
					    SppCallbackFunc, swScrollCB,
					    SppCallbackData, window,
					    NULL);
    } else {
	window->hscroll = spCreateScrollBar(window->wave_box, "hscroll",
					    SppGroupId, SW_PAGE_GROUP_ID,
					    SppSenseLevel, SW_STATE_EXIST_WAVE,
					    SppHeight, default_slider_width,
					    SppOrientation, SP_HORIZONTAL,
					    SppTrackCallbackOn, SP_TRUE,
					    SppMinimum, 0,
					    SppMaximum, 100,
					    SppIncrement, 10,
					    SppPageIncrement, 100,
					    SppSliderSize, 100,
					    SppValue, 0,
					    SppCallbackFunc, swScrollCB,
					    SppCallbackData, window,
					    NULL);
    }

    window->vscroll = spCreateScrollBar(window->window, "vscroll",
					SppGroupId, SW_PAGE_GROUP_ID,
					SppSenseLevel, SW_STATE_EXIST_WAVE,
					SppHeight, -default_slider_width,
					SppOrientation, SP_VERTICAL,
					SppTrackCallbackOn, SP_TRUE,
					SppMinimum, 0,
					SppMaximum, 10000,
					SppIncrement, 1000,
					SppPageIncrement, 10000,
					SppSliderSize, 10000,
					SppValue, 0,
					SppCallbackFunc, swScrollAmplitudeCB,
					SppCallbackData, window,
					NULL);

    createInfoArea(window);

    return;
}

void swSetWindowValue(swWindow window)
{
    spBool track_call_on;
    
    if (window == NULL) return;
    
    if (window->wave != NULL) {
#if 0
	if (window->length >= window->config->wave_config->read_callback_length) {
	    track_call_on = SP_FALSE;
	} else {
	    track_call_on = SP_TRUE;
	}
#else
	track_call_on = SP_TRUE;
#endif

	window->scroll_coef = swGetScrollCoef(window->wave);
	
	spSetParams(window->hscroll,
		    SppMinimum, 0,
		    SppMaximum, (int)(window->wave->total_length / window->scroll_coef),
		    SppIncrement, (int)MAX(window->length / window->scroll_coef / 4, 1),
		    SppPageIncrement, (int)MAX(window->length / window->scroll_coef, 1),
		    SppSliderSize, (int)MAX(window->length / window->scroll_coef, 1),
		    SppValue, (int)(window->offset / window->scroll_coef),
		    SppTrackCallbackOn, track_call_on,
		    NULL);
	
	spDebug(40, NULL, "offset = %ld, length = %ld\n", window->offset, window->length);
	spDebug(10, NULL, "wave length = %ld, scroll_coef = %ld\n",
		window->wave->total_length, window->scroll_coef);
	
	if (window->point >= 0) {
	    window->point_d = swSampToDisp(window, window->point);
	}
	
	if (window->sel_st >= 0 && window->sel_ed >= 0) {
	    window->sel_st_d = swSampToDisp(window, window->sel_st);
	    window->sel_ed_d = swSampToDisp(window, window->sel_ed);

	    if (window->sel_st != window->sel_ed && 
		window->sel_st_d == window->sel_ed_d) {
		window->sel_ed_d += 1;
	    }
	}
    } else {
	spSetParams(window->hscroll,
		    SppMinimum, 0,
		    SppMaximum, 100,
		    SppIncrement, 10,
		    SppPageIncrement, 100,
		    SppSliderSize, 100,
		    SppValue, 0,
		    NULL);
    }
    
    spDebug(10, "swSetWindowValue", "done\n");
    
    return;
}

spBool swIsNoWave(swWindow window)
{
    if (window == NULL) return SP_TRUE;
    
    if (swIsWaveNone(window->wave) == SP_TRUE) {
	return SP_TRUE;
    } else {
	return SP_FALSE;
    }
}

spBool swIsNoLabel(swWindow window)
{
    if (swIsNoWave(window) == SP_TRUE
	|| swIsLabelNone(window->wave) == SP_TRUE) {
	return SP_TRUE;
    } else {
	return SP_FALSE;
    }
}

spBool swIsProcessing(swWindow window)
{
    if (window == NULL || window->wave == NULL) return SP_FALSE;

    if (swIsWaveProcessing(window->wave) == SP_TRUE) {
	return SP_TRUE;
    }

    return SP_FALSE;
}

spBool swIsVisible(swWindow window)
{
    if (window == NULL) return SP_FALSE;

    spDebug(10, "swIsVisible", "visible_flag = %d\n", window->visible_flag);
    
    return window->visible_flag;
}

spBool swIsSubplotVisible(swWindow window)
{
#ifdef SW_SUPPORT_ANALYSIS_SUBPLOT
    if (window == NULL) return SP_FALSE;

    if (window->subplot_sgram == SP_TRUE || window->subplot_f0 == SP_TRUE
	|| window->subplot_power == SP_TRUE
#ifdef SW_SUPPORT_STRAIGHT
	|| window->subplot_straight_sgram == SP_TRUE || window->subplot_straight_ap == SP_TRUE
	|| window->subplot_straight_f0 == SP_TRUE
#endif
	) {
	return SP_TRUE;
    } else {
	return SP_FALSE;
    }
#else
    return SP_FALSE;
#endif
}

spBool swIsPlayable(swWindow window)
{
    if (window == NULL || window->wave == NULL) return SP_FALSE;

    if (swIsWaveProcessing(window->wave) == SP_TRUE
	|| swIsWavePlayable(window->wave) == SP_FALSE
	|| window->config->toplevel->playable == SP_FALSE) {
	return SP_FALSE;
    }

    return SP_TRUE;
}

spBool swNeedWindowProcessBlock(swWindow src_window, swWindow window)
{
    spBool need_block;
    
    if (src_window == NULL || window == NULL) return SP_FALSE;

    if (window->config->toplevel->using_clipboard == SP_TRUE
	&& swIsClipboardWindow(window) == SP_TRUE) {
	need_block = SP_TRUE;
    } else {
	need_block = swNeedProcessBlock(src_window->wave, window->wave);
    }

    return need_block;
}

void swPopupWindow(swWindow window, int x, int y)
{
    if (window == NULL) return;

    spDebug(30, "swPopupWindow", "x = %d, y = %d\n", x, y);
    
    if (x >= 0 && y >= 0) {
	spSetWindowPosition(window->window, x, y);
	spMapWindow(window->window);
    } else {
	spPopupWindow(window->window);
    }
    
    spDebug(30, "swPopupWindow", "done\n");
    
    return;
}

/**/
spBool swIsClipboardWindow(swWindow window)
{
    if (window == NULL) return SP_FALSE;

    if (window->config->toplevel->clipboard_window == window) {
	return SP_TRUE;
    }

    return SP_FALSE;
}

spBool swIsClipboardVisible(swWindow window)
{
    if (window == NULL) return SP_FALSE;

    return spIsVisible(window->config->toplevel->clipboard_window->window);
}

spBool swIsClipboardNone(swWindow window)
{
    if (window == NULL) return SP_FALSE;

    return swIsNoWave(window->config->toplevel->clipboard_window);
}

void swCreateClipboardWindow(swConfig config)
{
    config->toplevel->clipboard_window = swInitWindow(config, SW_TIME_DATA);
    swSetWave(config->toplevel->clipboard_window, NULL);
    swCreateMainWindow(config->toplevel->clipboard_window);
    swSetWindowTitle(config->toplevel->clipboard_window);
    
    return;
}

#ifdef SW_SUPPORT_CLIPBOARD
void swPopupClipboardWindowCB(spComponent component, swConfig config)
{
    if (config->toplevel->clipboard_window != NULL) {
	if (spIsVisible(config->toplevel->clipboard_window->window) == SP_TRUE) {
	    spMapComponent(config->toplevel->clipboard_window->window);
	} else {
	    spDebug(10, "swPopupClipboardWindowCB", "popup clipboard\n");
	    config->toplevel->clipboard_window->visible_flag = SP_TRUE;
	    spPopupWindow(config->toplevel->clipboard_window->window);
	    
	    spLockMutex(config->toplevel->main_mutex);
	    ++config->toplevel->num_window;
	    spUnlockMutex(config->toplevel->main_mutex);
	    
	    swSetSenseLevel(NULL);
	}
    }
    return;
}
#endif

spBool swLockWindowMutex(swWindow window)
{
    if (window == NULL) return SP_FALSE;
    
    spDebug(100, "swLockWindowMutex", "++++++++ lock ++++++++\n");
    
    return spLockMutex(window->mutex);
}

spBool swUnlockWindowMutex(swWindow window)
{
    if (window == NULL) return SP_FALSE;
    
    spDebug(100, "swUnlockWindowMutex", "-------- unlock --------\n");
    
    return spUnlockMutex(window->mutex);
}

spBool swLockMainMutex(swWindow window)
{
    if (window == NULL) return SP_FALSE;
    
    spDebug(100, "swLockMainMutex", "++++++++ lock ++++++++\n");
    
    return spLockMutex(window->config->toplevel->main_mutex);
}

spBool swUnlockMainMutex(swWindow window)
{
    if (window == NULL) return SP_FALSE;
    
    spDebug(100, "swUnlockMainMutex", "-------- unlock --------\n");
    
    return spUnlockMutex(window->config->toplevel->main_mutex);
}

static swTopLevel sw_toplevel = NULL;

void swInitTopLevel(spTopLevel toplevel, swConfig config)
{
    if (sw_toplevel == NULL) {
	sw_toplevel = xalloc(1, struct _swTopLevel);
	sw_toplevel->toplevel = toplevel;
	sw_toplevel->null_window = NULL;
	sw_toplevel->current_window = NULL;
	sw_toplevel->clipboard_window = NULL;
	sw_toplevel->format_specified = SP_FALSE;
	sw_toplevel->graphics_mode_caps = spGetSystemGraphicsModeCaps();
	if (sw_toplevel->graphics_mode_caps & SP_GRAPHICS_MODE_CAPS_XOR) {
#if 1
	    sw_toplevel->rubber_band_selection = SP_TRUE;
#else
	    sw_toplevel->rubber_band_selection = SP_FALSE;
#endif
	} else {
	    sw_toplevel->rubber_band_selection = SP_FALSE;
	}
	spDebug(80, "swInitTopLevel", "rubber_band_selection = %d\n", sw_toplevel->rubber_band_selection);

        sw_toplevel->log_min_value = log10(SW_LOG_FREQUENCY_MIN_VALUE);
        
	sw_toplevel->print_canvas = NULL;
	
	sw_toplevel->num_window = 0;
	sw_toplevel->num_process = 0;
	sw_toplevel->playable = SP_TRUE;
	sw_toplevel->using_clipboard = SP_FALSE;
	sw_toplevel->main_mutex = spCreateMutex(NULL);

#if defined(SW_AH_CUSTOM)
	swInitAHInfo(&sw_toplevel->ahinforec);
	sw_toplevel->ahfile = swAllocAHFile(&sw_toplevel->ahinforec);
	sw_toplevel->ahwindow = swCreateAHSessionWindow(config, &sw_toplevel->ahinforec, sw_toplevel->ahfile);
#endif
	
	config->toplevel = sw_toplevel;
    }

    return;
}

long swGetNumWindow(void)
{
    if (sw_toplevel == NULL) return 0;
    
    return sw_toplevel->num_window;
}

long swGetNumProcess(void)
{
    if (sw_toplevel == NULL) return 0;
    
    return sw_toplevel->num_process;
}

spBool swIsSpectrogramVisible(swWindow window)
{
    if (window == NULL) return SP_FALSE;

    if (window->specgram != NULL && window->draw_specgram == SP_TRUE) {
	return SP_TRUE;
    } else {
	return SP_FALSE;
    }
}

#ifdef SW_SUPPORT_PRINT
void swPageSetupCB(spComponent component, swWindow window)
{
    spBool flag;
    
    flag = spOpenDrawingSetupDialog(swGetPrintCanvas(window), NULL);
    
    spDebug(20, "swPageSetupCB", "done: flag = %d\n", flag);
    
    return;
}

void swPrintCB(spComponent component, swWindow window)
{
    spComponent print_canvas;

    if (swIsNoWave(window) == SP_TRUE) return;
    
    print_canvas = swGetPrintCanvas(window);
    spDebug(20, "swPrintCB", "print_canvas = %ld\n", (long)print_canvas);
    
    if (spOpenDrawingTargetDialog(print_canvas, NULL) == SP_TRUE) {
        spDebug(20, "swPrintCB", "spOpenDrawingTargetDialog done\n");
	swPrintWaveImage(print_canvas, window);
	spCloseDrawingTarget(print_canvas);
    } else {
        spDebug(20, "swPrintCB", "spOpenDrawingTargetDialog failed\n");
    }
    
    return;
}

spComponent swGetPrintCanvas(swWindow window)
{
    if (window == NULL) return NULL;
    
    return window->config->toplevel->print_canvas;
}

const char *swGetPrintCanvasName(swConfig config)
{
    return spGetPluginCanvasName(config->toplevel->print_canvas);
}

spComponent swCreatePrintCanvas(swConfig config, const char *plugin_name)
{
    const char *old_plugin_name;

    if (!strnone(plugin_name)) {
	spDebug(50, "swCreatePrintCanvas", "plugin_name = %s\n", plugin_name);
	if (config->toplevel->print_canvas != NULL) {
	    old_plugin_name = swGetPrintCanvasName(config);
	    spDebug(50, "swCreatePrintCanvas", "current canvas = %s\n", old_plugin_name);
	    if (streq(old_plugin_name, plugin_name)) {
		spDebug(50, "swCreatePrintCanvas", "already created\n");
		return config->toplevel->print_canvas;
	    }
	    
	    spDestroyComponent(config->toplevel->print_canvas);
	}
	
	config->toplevel->print_canvas = spCreatePluginCanvas(NULL, plugin_name,
                                                              SppDrawFunc, swPluginCanvasCB,
							      NULL);
	if (config->toplevel->print_canvas == NULL) {
	    spDisplayError(NULL, NULL, "Cannot open drawing plugin `%s'.", plugin_name);
	}
    }

    /*spDebug(80, "swCreatePrintCanvas", "done: print_canvas = %ld\n", (long)config->toplevel->print_canvas);*/
    
    return config->toplevel->print_canvas;
}

void swSelectDrawingPluginCB(spComponent component, swWindow window)
{
    const char *name;
    swWindow next_window;

    name = spGetName(component);
    swCreatePrintCanvas(window->config, name);
    
    next_window = window;
    while (next_window != NULL) {
	swSelectRadioButtonSubMenu(next_window->drawing_plugin_menu, name);
	    
	next_window = swGetNextWindow(next_window);
	
	if (next_window == window) {
	    break;
	}
    }
    
    return;
}

void swCreateDrawingPluginMenu(swWindow window, spComponent parent_menu)
{
    int i;
    spComponent sub_menu, menu_item;
    const char *plugin_name;
    const char *plugin_desc;
    int current_index = -1;

    sub_menu = spAddSubMenu(parent_menu, "drawingPluginSubMenu",
			    SppTitle, _("SW_MENU_PRINT_TARGET_LABEL"),
			    SppHelpPath, "menu/file_menu.html#file_print_target",
			    NULL);

    for (i = 0;; i++) {
	plugin_name = spSearchDrawingPluginFile(i, SP_DRAWING_DEVICE_FILE | SP_DRAWING_DEVICE_PRINTER);
	if (plugin_name == NULL) break;
	
	plugin_desc = spGetDrawingPluginDescription(plugin_name);
	
	if (plugin_name != NULL && plugin_desc != NULL) {
	    if (streq(plugin_name, swGetPrintCanvasName(window->config))) {
		current_index = i;
	    }
	    menu_item = spAddRadioButtonMenuItem(sub_menu, plugin_name,
						 SppTitle, spGetString(plugin_desc),
						 SppCallbackFunc, swSelectDrawingPluginCB,
						 SppCallbackData, window,
						 SppSet, (current_index == i ? SP_TRUE : SP_FALSE),
						 NULL);
	}
    }

    window->drawing_plugin_menu = sub_menu;

    spDebug(80, "swCreateDrawingPluginMenu", "done\n");
    
    return;
}
#endif

