/*
 *	swWave.c
 *
 *	Last modified: <2025-04-13 20:26:18 hideki>
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

#include <sp/sp.h>
#include <sp/vector.h>
#include <sp/voperate.h>
#include <sp/fft.h>
#include <sp/cqt.h>
#include <sp/window.h>
#include <sp/filter.h>
#include <sp/sfconv.h>

#include "swWave.h"
#include "swWaveAudio.h"
#include "swLabel.h"

#define SW_SUFFICIENT_THIN_LENGTH /*8*/6

#define SW_READ_CALLBACK_FACTOR /*16*/32

swAnalysisStringInfo sw_ana_string_infos[] =
{
    {SW_ANALYSIS_F0, "F0", SW_ANALYSIS_UNIT_HZ, SP_FALSE, 50.0, 400.0},
    {SW_ANALYSIS_STRAIGHT, "STRAIGHT Spectrum", SW_ANALYSIS_UNIT_ABS_AMPLITUDE, SP_TRUE, 0.0, -1.0},
    {SW_ANALYSIS_WAVEFORM, "Amplitude", SW_ANALYSIS_UNIT_AMPLITUDE, SP_FALSE, -1.0, 1.0},
    {SW_ANALYSIS_SMOOTHED_SPECTRUM, "Smoothed Amplitude", SW_ANALYSIS_UNIT_ABS_AMPLITUDE, SP_TRUE, 0.0, -1.0},
    {SW_ANALYSIS_SPECTRUM, "Amplitude", SW_ANALYSIS_UNIT_ABS_AMPLITUDE, SP_TRUE, 0.0, -1.0},
    {SW_ANALYSIS_UNWRAPPED_PHASE, "Unwrapped Phase", SW_ANALYSIS_UNIT_RADIAN, SP_FALSE, 0.0, -1.0},
    {SW_ANALYSIS_PHASE, "Phase", SW_ANALYSIS_UNIT_RADIAN, SP_FALSE, -PI, PI},
    {SW_ANALYSIS_SMOOTHED_GROUP_DELAY, "Smoothed Group Delay", SW_ANALYSIS_UNIT_SEC, SP_FALSE, 0.0, -1.0},
    {SW_ANALYSIS_GROUP_DELAY, "Group Delay", SW_ANALYSIS_UNIT_SEC, SP_FALSE, 0.0, -1.0},
    {SW_ANALYSIS_CEPSTRUM, "Cepstrum", SW_ANALYSIS_UNIT_NONE, SP_FALSE, 0.0, -1.0},
    {SW_ANALYSIS_POWER, "Power", SW_ANALYSIS_UNIT_DB, SP_FALSE, 0.0, -1.0},
    {SW_ANALYSIS_APERIODICITY, "Aperiodicity", SW_ANALYSIS_UNIT_DB, SP_FALSE, 0.0, -1.0},
    {SW_ANALYSIS_CQT_SPECTRUM, "Amplitude", SW_ANALYSIS_UNIT_ABS_AMPLITUDE, SP_TRUE, 0.0, -1.0},
    {SW_ANALYSIS_ERB_CQT_SPECTRUM, "Amplitude", SW_ANALYSIS_UNIT_ABS_AMPLITUDE, SP_TRUE, 0.0, -1.0},
};

static swAnalysisStringInfo *getAnalysisStringInfo(swAnalysisType analysis_type)
{
    int i;
    static int sw_num_ana_string_info = 0;

    if (sw_num_ana_string_info <= 0) {
	sw_num_ana_string_info = spArraySize(sw_ana_string_infos);
    }

    for (i = 0; i < sw_num_ana_string_info; i++) {
	if ((sw_ana_string_infos[i].analysis_type & analysis_type)
	    == sw_ana_string_infos[i].analysis_type) {
	    return &sw_ana_string_infos[i];
	}
    }

    return NULL;
}

spBool swUpdateAnalysisStringInfo(swWave wave, swAnalysisType analysis_type)
{
    if (wave == NULL || wave->core == NULL) return SP_FALSE;
    
    if ((wave->core->ana_string_info = getAnalysisStringInfo(analysis_type)) != NULL) {
	wave->core->analysis_type = analysis_type;
	return SP_TRUE;
    } else {
	wave->core->analysis_type = SW_ANALYSIS_NONE;
	return SP_FALSE;
    }
}

char *swGetAnalysisUnitString(swWave wave, spBool brackets_flag)
{
    if (wave == NULL || wave->core == NULL || wave->core->ana_string_info == NULL) {
	return NULL;
    }
    
    switch (wave->core->ana_string_info->unit_type) {
      case SW_ANALYSIS_UNIT_DB:
	return brackets_flag ? "[dB]" : "dB";
      case SW_ANALYSIS_UNIT_HZ:
	return brackets_flag ? "[Hz]" : "Hz";
      case SW_ANALYSIS_UNIT_SEC:
	return brackets_flag ? "[s]" : "s";
      case SW_ANALYSIS_UNIT_RADIAN:
	return brackets_flag ? "[rad]" : "rad";
      case SW_ANALYSIS_UNIT_ABS_AMPLITUDE:
	if (wave->config->linear_spectrum == SP_FALSE) {
	    return brackets_flag ? "[dB]" : "dB";
	}
      default:
        break;
    }

    return NULL;
}

char *swGetAnalysisNameString(swWave wave)
{
    if (wave == NULL || wave->core == NULL || wave->core->ana_string_info == NULL) {
	return NULL;
    }
    
    return wave->core->ana_string_info->name;
}

spBool swIsAnalysisTypeF0(swAnalysisType type)
{
    if ((type & SW_ANALYSIS_F0) == SW_ANALYSIS_F0) {
	return SP_TRUE;
    } else {
	return SP_FALSE;
    }
}

spBool swIsAnalysisTypePower(swAnalysisType type)
{
    if ((type & SW_ANALYSIS_POWER) == SW_ANALYSIS_POWER) {
	return SP_TRUE;
    } else {
	return SP_FALSE;
    }
}

spBool swIsAnalysisTypeAperiodicity(swAnalysisType type)
{
    if ((type & SW_ANALYSIS_APERIODICITY) == SW_ANALYSIS_APERIODICITY) {
	return SP_TRUE;
    } else {
	return SP_FALSE;
    }
}

spBool swIsAnalysisTypeStraight(swAnalysisType type)
{
    if ((type & SW_ANALYSIS_STRAIGHT_MASK) == SW_ANALYSIS_STRAIGHT_MASK) {
	return SP_TRUE;
    } else {
	return SP_FALSE;
    }
}

swAnalysisType swGetAnalysisTypeFromLabel(char *label)
{
    if (streq(label, SW_ANALYSIS_SPECTRUM_LABEL)
	|| streq(label, SW_ANALYSIS_SPECTRUM_PARAM_LABEL)) {
	return SW_ANALYSIS_SPECTRUM;
    } else if (streq(label, SW_ANALYSIS_SMOOTHED_SPECTRUM_LABEL)
	       || streq(label, SW_ANALYSIS_SMOOTHED_SPECTRUM_PARAM_LABEL)) {
	return SW_ANALYSIS_SMOOTHED_SPECTRUM;
    } else if (streq(label, SW_ANALYSIS_PHASE_LABEL)
	       || streq(label, SW_ANALYSIS_PHASE_PARAM_LABEL)) {
	return SW_ANALYSIS_PHASE;
    } else if (streq(label, SW_ANALYSIS_UNWRAPPED_PHASE_LABEL)
	       || streq(label, SW_ANALYSIS_UNWRAPPED_PHASE_PARAM_LABEL)) {
	return SW_ANALYSIS_UNWRAPPED_PHASE;
    } else if (streq(label, SW_ANALYSIS_GROUP_DELAY_LABEL)
	       || streq(label, SW_ANALYSIS_GROUP_DELAY_PARAM_LABEL)) {
	return SW_ANALYSIS_GROUP_DELAY;
    } else if (streq(label, SW_ANALYSIS_SMOOTHED_GROUP_DELAY_LABEL)
	       || streq(label, SW_ANALYSIS_SMOOTHED_GROUP_DELAY_PARAM_LABEL)) {
	return SW_ANALYSIS_SMOOTHED_GROUP_DELAY;
    } else if (streq(label, SW_ANALYSIS_GROUP_DELAY_LABEL)
	       || streq(label, SW_ANALYSIS_TD_GROUP_DELAY_PARAM_LABEL)) {
	return SW_ANALYSIS_TD_GROUP_DELAY;
    } else if (streq(label, SW_ANALYSIS_CEPSTRUM_LABEL)
	       || streq(label, SW_ANALYSIS_CEPSTRUM_PARAM_LABEL)) {
	return SW_ANALYSIS_CEPSTRUM;
    } else {
	return SW_ANALYSIS_NONE;
    }
}

char *swGetAnalysisTypeParamLabel(swAnalysisType type)
{
    switch (type) {
      case SW_ANALYSIS_SPECTRUM:
	return SW_ANALYSIS_SPECTRUM_PARAM_LABEL;
      case SW_ANALYSIS_SMOOTHED_SPECTRUM:
	return SW_ANALYSIS_SMOOTHED_SPECTRUM_PARAM_LABEL;
      case SW_ANALYSIS_PHASE:
	return SW_ANALYSIS_PHASE_PARAM_LABEL;
      case SW_ANALYSIS_UNWRAPPED_PHASE:
	return SW_ANALYSIS_UNWRAPPED_PHASE_PARAM_LABEL;
      case SW_ANALYSIS_GROUP_DELAY:
	return SW_ANALYSIS_GROUP_DELAY_PARAM_LABEL;
      case SW_ANALYSIS_SMOOTHED_GROUP_DELAY:
	return SW_ANALYSIS_SMOOTHED_GROUP_DELAY_PARAM_LABEL;
      case SW_ANALYSIS_TD_GROUP_DELAY:
	return SW_ANALYSIS_TD_GROUP_DELAY_PARAM_LABEL;
      case SW_ANALYSIS_CEPSTRUM:
	return SW_ANALYSIS_CEPSTRUM_PARAM_LABEL;
      default:
	return "";
    }
}

char *swGetAnalysisTypeLabel(swAnalysisType type)
{
    switch (type) {
      case SW_ANALYSIS_SPECTRUM:
	return SW_ANALYSIS_SPECTRUM_LABEL;
      case SW_ANALYSIS_SMOOTHED_SPECTRUM:
	return SW_ANALYSIS_SMOOTHED_SPECTRUM_LABEL;
      case SW_ANALYSIS_PHASE:
	return SW_ANALYSIS_PHASE_LABEL;
      case SW_ANALYSIS_UNWRAPPED_PHASE:
	return SW_ANALYSIS_UNWRAPPED_PHASE_LABEL;
      case SW_ANALYSIS_GROUP_DELAY:
	return SW_ANALYSIS_GROUP_DELAY_LABEL;
      case SW_ANALYSIS_SMOOTHED_GROUP_DELAY:
	return SW_ANALYSIS_SMOOTHED_GROUP_DELAY_LABEL;
      case SW_ANALYSIS_TD_GROUP_DELAY:
	return SW_ANALYSIS_TD_GROUP_DELAY_LABEL;
      case SW_ANALYSIS_CEPSTRUM:
	return SW_ANALYSIS_CEPSTRUM_LABEL;
      default:
	return "";
    }
}

swWindowType swGetWindowTypeFromLabel(char *label)
{
    if (streq(label, SW_WINDOW_RECTANGLE_LABEL)
	|| streq(label, SW_WINDOW_RECTANGLE_PARAM_LABEL)) {
	return SW_WINDOW_RECTANGLE;
    } else if (streq(label, SW_WINDOW_HAMMING_LABEL)
	       || streq(label, SW_WINDOW_HAMMING_PARAM_LABEL)) {
	return SW_WINDOW_HAMMING;
    } else if (streq(label, SW_WINDOW_HANNING_LABEL)
	       || streq(label, SW_WINDOW_HANNING_PARAM_LABEL)) {
	return SW_WINDOW_HANNING;
    } else if (streq(label, SW_WINDOW_BLACKMAN_LABEL)
	       || streq(label, SW_WINDOW_BLACKMAN_PARAM_LABEL)) {
	return SW_WINDOW_BLACKMAN;
    } else if (streq(label, SW_WINDOW_GAUSS_LABEL)
	       || streq(label, SW_WINDOW_GAUSS_PARAM_LABEL)) {
	return SW_WINDOW_GAUSS;
    } else {
	return SW_WINDOW_NONE;
    }
}

char *swGetWindowTypeParamLabel(swWindowType type)
{
    switch (type) {
      case SW_WINDOW_RECTANGLE:
	return SW_WINDOW_RECTANGLE_PARAM_LABEL;
      case SW_WINDOW_HAMMING:
	return SW_WINDOW_HAMMING_PARAM_LABEL;
      case SW_WINDOW_HANNING:
	return SW_WINDOW_HANNING_PARAM_LABEL;
      case SW_WINDOW_BLACKMAN:
	return SW_WINDOW_BLACKMAN_PARAM_LABEL;
      case SW_WINDOW_GAUSS:
	return SW_WINDOW_GAUSS_PARAM_LABEL;
      default:
	return "";
    }
}

spBool swIsWaveNone(swWave wave)
{
    if (wave == NULL) return SP_TRUE;

    if (strnone(wave->plugin_name) && strnone(wave->new_plugin_name)) {
	spDebug(100, "swIsWaveNone", "no wave\n");
	return SP_TRUE;
    } else {
	return SP_FALSE;
    }
}

spBool swIsWaveFloat(swWave wave)
{
    if (wave == NULL) return SP_FALSE;
    
    return wave->float_flag;
}

spBool swIsWaveLong(swWave wave)
{
    if (wave == NULL) return SP_FALSE;
    
    if (swIsWaveFloat(wave) == SP_FALSE
	&& (wave->samp_bit > 16 && wave->samp_bit <= 32)) {
	return SP_TRUE;
    } else {
	return SP_FALSE;
    }
}

spBool swIsWaveOverflow(swWave wave)
{
    if (wave == NULL) return SP_FALSE;

    return wave->overflow;
}

spBool swIsWavePlaying(swWave wave)
{
    if (wave == NULL) return SP_FALSE;

    return wave->core->play_flag;
}

spBool swIsWaveProcessing(swWave wave)
{
    if (wave == NULL) return SP_FALSE;

    if (wave->core->process_flag == SP_TRUE
	/*|| wave->core->process_finished == SP_FALSE*/) {
	return SP_TRUE;
    } else {
	return SP_FALSE;
    }
}

spBool swIsWaveProcessFinished(swWave wave)
{
    if (wave == NULL) return SP_FALSE;

    return wave->core->process_finished;
}

int swGetWaveLastError(swWave wave)
{
    if (wave == NULL) return 0;

    return wave->core->last_error;
}

spBool swIsWaveEdited(swWave wave)
{
    if (wave == NULL) return SP_FALSE;

    return wave->edit_flag;
}

spBool swIsWaveThreadSafe(swWave wave)
{
    if (wave == NULL) return SP_FALSE;

    return wave->thread_safe;
}

spBool swIsWavePeakAvailable(swWave wave)
{
    if (wave == NULL || wave->peak_filename == NULL
	|| wave->peak_length <= 0) {
	return SP_FALSE;
    }

    return SP_TRUE;
}

spBool swNeedProcessBlock(swWave src_wave, swWave wave)
{
    if (src_wave == NULL || wave == NULL) return SP_FALSE;

    spDebug(80, "swNeedProcessBlock", "thread_safe = %d, %s %s\n",
	    src_wave->thread_safe, src_wave->plugin_name, wave->plugin_name);

    if (src_wave->thread_safe == SP_FALSE
	&& streq(src_wave->plugin_name, wave->plugin_name)) {
	return SP_TRUE;
    }

    return SP_FALSE;
}

double swGetWaveSampleRate(swWave wave)
{
    if (wave == NULL) return 0.0;

    return wave->samp_rate;
}

double swGetWaveExactSampleRate(swWave wave)
{
    if (wave == NULL) return 0.0;

    return wave->exact_samp_rate;
}

int swGetWaveSampleBit(swWave wave)
{
    if (wave == NULL) return 0;

    return wave->samp_bit;
}

int swGetWaveSampleByte(swWave wave)
{
    int samp_byte;
    
    if (wave == NULL) return 0;

    if (swIsWaveFloat(wave) == SP_TRUE) {
	samp_byte = sizeof(double);
    } else if (swIsWaveLong(wave) == SP_TRUE) {
	samp_byte = sizeof(long);
    } else {
	samp_byte = sizeof(short);
    }
    
    return samp_byte;
}

int swGetWaveNumChannel(swWave wave)
{
    if (wave == NULL) return 0;

    return wave->num_channel;
}

int swGetWaveSelectedChannel(swWave wave)
{
    if (wave == NULL) return 0;

    return wave->selected_channel;
}

spLong swGetWaveOffset(swWave wave)
{
    if (wave == NULL) return 0;

    return wave->offset;
}

spLong swGetWaveLength(swWave wave)
{
    if (wave == NULL) return 0;

    return wave->length;
}

spLong swGetWaveEditOffset(swWave wave)
{
    if (wave == NULL) return 0;

    return wave->edit_offset;
}

spLong swGetWaveEditLength(swWave wave)
{
    if (wave == NULL) return 0;

    return wave->edit_length;
}

spBool swSetWaveTotalLength(swWave wave, spLong length)
{
    if (wave == NULL) return SP_FALSE;

    wave->total_length = length;

    return SP_TRUE;
}

spLong swGetWaveTotalLength(swWave wave)
{
    if (wave == NULL) return 0;

    return wave->total_length;
}

spLong swGetWaveThinLength(swWave wave)
{
    if (wave == NULL) return 1;

    return wave->thin_length;
}

char *swGetWaveFileName(swWave wave)
{
    if (wave == NULL) return NULL;

    return wave->filename;
}

char *swGetWaveOriginalFileName(swWave wave)
{
    if (wave == NULL) return NULL;

    return wave->core->orig_filename;
}

char *swGetWaveFileType(swWave wave)
{
    if (wave == NULL) return NULL;

    if (strnone(wave->file_type)) {
	return "None";
    } else {
	return wave->file_type;
    }
}

char *swGetWavePluginName(swWave wave)
{
    if (wave == NULL) return NULL;

    return wave->plugin_name;
}

spSongInfoV2 *swGetWaveSongInfo(swWave wave)
{
    if (wave == NULL) return NULL;

    return &wave->song_info;
}

swLabels swGetWaveLabels(swWave wave)
{
    if (wave == NULL) return NULL;

    return wave->labels;
}

swWaveConfig swGetWaveConfig(swWave wave)
{
    if (wave == NULL) return NULL;

    return wave->config;
}

spBool swGetDetailFlag(swWave wave)
{
    if (wave == NULL) return SP_FALSE;

    return wave->detail_flag;
}

spBool swSetDetailFlag(swWave wave, spBool detail_flag)
{
    if (wave == NULL) return SP_FALSE;

    wave->detail_flag = detail_flag;
    
    return SP_TRUE;
}

char *swGetEditName(swWave wave)
{
    if (wave == NULL) return NULL;
    return wave->edit_name;
}

spBool swSetEditName(swWave wave, char *edit_name)
{
    if (wave == NULL) return SP_FALSE;

    if (edit_name != wave->edit_name) {
	if (wave->edit_name != NULL) xfree(wave->edit_name);
	wave->edit_name = strclone(edit_name);
    }

    return SP_TRUE;
}

spLong swConvertLengthToBufferLength(swWave wave, spLong length)
{
    if (wave == NULL) return 0;

    return length * (spLong)wave->num_order * (spLong)wave->num_channel;
}

spLong swConvertLengthToByte(swWave wave, spLong length)
{
    if (wave == NULL) return 0;

    return length * (spLong)wave->num_order * (spLong)wave->num_channel * (spLong)swGetWaveSampleByte(wave);
}

spLong swConvertBufferLengthToLength(swWave wave, spLong length)
{
    if (wave == NULL) return 0;
    
    return length / (spLong)wave->num_channel / (spLong)wave->num_order;
}

spLong swConvertByteToLength(swWave wave, spLong size)
{
    spLong length;

    if (wave == NULL) return 0;

    length = size / (spLong)wave->num_channel / (spLong)wave->num_order;

    if (swIsWaveFloat(wave) == SP_TRUE) {
	length /= sizeof(double);
    } else if (swIsWaveLong(wave) == SP_TRUE) {
	length /= sizeof(long);
    } else {
	length /= sizeof(short);
    }

    return length;
}

spLong swLoadIntoBuffer(spPlugin *plugin, swWave wave, int channel, spLong length, char *buf)
{
    spLong index, index2;
    spLong k, l;
    spLong nread;
    short *sbuf;
    long *lbuf;
    double *dbuf;
    
    if (wave == NULL || plugin == NULL || length <= 0) return -1;

    spDebug(50, "swLoadIntoBuffer", "in\n");
    
    nread = spReadPlugin(plugin, buf, (long)swConvertLengthToBufferLength(wave, length));
    nread = swConvertBufferLengthToLength(wave, nread);
    
    dbuf = (double *)buf;
    lbuf = (long *)buf;
    sbuf = (short *)buf;
    
    if (channel >= 0) {
	index = 0;
	for (k = 0; k < nread; k++) {
	    for (l = 0; l < wave->num_order; l++) {
		index2 = swConvertLengthToBufferLength(wave, k) + (channel * wave->num_order) + l;
		if (swIsWaveFloat(wave) == SP_TRUE) {
		    dbuf[index] = dbuf[index2];
		} else if (swIsWaveLong(wave) == SP_TRUE) {
		    lbuf[index] = lbuf[index2];
		} else {
		    sbuf[index] = sbuf[index2];
		}
		index++;
	    }
	}
    }

    spDebug(50, "swLoadIntoBuffer", "done: nread = %ld\n", nread);
    
    return nread;
}


spBool swSetTempDir(swWaveConfig config, char *dir)
{
    char *path = NULL;
    
    if (!strnone(dir) && (path = xspGetExactName(dir)) != NULL) {
	if (spIsDir(path) == SP_FALSE
	    && spCreateDir(path, 0700) == SP_FALSE) {
	    xfree(path); path = NULL;
	}
    }

    if (path == NULL) {
	path = xspGetApplicationTempDir();
    }

    if (path != NULL) {
	spStrCopy(config->temp_dir, SP_MAX_PATHNAME, path);
	spAddDirSeparator(config->temp_dir);
	xfree(path);
    
	spDebug(10, "swSetTempDir", "temp_dir = %s\n", config->temp_dir);
	
	return SP_TRUE;
    } else {
	return SP_FALSE;
    }
}

void swGetTempFile(swWaveConfig config, char *suffix, char *filename)
{
    static long pid = -1;
    static long id = 0;
#ifdef SW_USE_THREAD
    static void *temp_file_mutex = NULL;
#endif
    char suffixbuf[SP_MAX_PATHNAME];

    if (pid <= 0) {
	pid = spGetProcessId();
    }
#ifdef SW_USE_THREAD
    if (temp_file_mutex == NULL) {
	temp_file_mutex = spCreateMutex(NULL);
    }
    spLockMutex(temp_file_mutex);
#endif

    if (suffix != NULL) {
	spStrCopy(suffixbuf, SP_MAX_PATHNAME, suffix);
    } else {
	spStrCopy(suffixbuf, SP_MAX_PATHNAME, SW_DEFAULT_TEMP_FILE_SUFFIX);
    }

    if (strnone(config->temp_dir)) {
	sprintf(filename, "spwave%ld_%ld%s", pid, id, suffixbuf);
    } else {
	sprintf(filename, "%sspwave%ld_%ld%s", config->temp_dir, pid, id, suffixbuf);
    }
    id++;

#ifdef SW_USE_THREAD
    spUnlockMutex(temp_file_mutex);
#endif
    
    return;
}

char *xswBackupOriginal(char *filename)
{
    char buf[SP_MAX_PATHNAME];
    
    if (spIsFile(filename) == SP_FALSE) return NULL;

    spStrCopy(buf, SP_MAX_PATHNAME, filename);
    while (1) {
	spStrCat(buf, SP_MAX_PATHNAME, "~");
	if (spExists(buf) == SP_FALSE) {
	    break;
	}
    }
    
    if (spRenameFile(filename, buf) == SP_TRUE) {
	return strclone(buf);
    } else {
	return NULL;
    }
}

spBool swIsOverflow(int samp_bit, double value)
{
    if (samp_bit > 32) {
	return SP_FALSE;
    } else if (samp_bit >= 32){
	if (value >= -2147483648.0 && value <= 2147483647.0) {
	    return SP_FALSE;
	}
    } else if (samp_bit >= 24){
	if (value >= -8388608.0 && value <= 8388607.0) {
	    return SP_FALSE;
	}
    } else {
	if (value >= -32768.0 && value <= 32767.0) {
	    return SP_FALSE;
	}
    }

    return SP_TRUE;
}

static double swGetClippedValue(int samp_bit, double value, spBool *overflow)
{
    spBool flag = SP_FALSE;
    
    if (samp_bit > 32) {
    } else if (samp_bit >= 32){
	if (value < -2147483647.0) {
	    value = -2147483647.0;
	    flag = SP_TRUE;
	} else if (value > 2147483647.0) {
	    value = 2147483647.0;
	    flag = SP_TRUE;
	} else {
	    value = spRound(value);
	}
    } else if (samp_bit >= 24){
	if (value < -8388607.0) {
	    value = -8388607.0;
	    flag = SP_TRUE;
	} else if (value > 8388607.0) {
	    value = 8388607.0;
	    flag = SP_TRUE;
	} else {
	    value = spRound(value);
	}
    } else {
	if (value < -32767.0) {
	    value = -32767.0;
	    flag = SP_TRUE;
	} else if (value > 32767.0) {
	    value = 32767.0;
	    flag = SP_TRUE;
	} else {
	    value = spRound(value);
	}
    }

    if (flag == SP_TRUE && overflow != NULL) {
	spDebug(80, "swGetClippedValue", "overflow\n");
	*overflow = flag;
    }

    return value;
}

double swGetClipValue(int samp_bit)
{
    double value = 1.0;
    
    if (samp_bit <= 16) {
	value = 32767.0;
    } else if (samp_bit <= 24) {
	value = 8388607.0;
    } else if (samp_bit <= 32) {
	value = 2147483647.0;
    }

    return value;
}

double swGetLimitValue(int samp_bit)
{
    double value = 1.0;
    
    if (samp_bit <= 16) {
	value = 32768.0;
    } else if (samp_bit <= 24) {
	value = 8388608.0;
    } else if (samp_bit <= 32) {
	value = 2147483648.0;
    }

    return value;
}

spBool swAllocData(swWave wave, spLong alloc_length)
{
    spLong alloc_size;
    
    if (wave == NULL) return SP_FALSE;

    alloc_size = swConvertLengthToByte(wave, alloc_length);

    if (alloc_size <= wave->alloc_size) {
	return SP_TRUE;
    } else {
	wave->data = xrealloc(wave->data, alloc_size, char);
	memset(wave->data + wave->alloc_size, 0, alloc_size - wave->alloc_size);
	wave->alloc_size = alloc_size;
    }
    spDebug(10, "swAllocData", "done\n");
    
    return SP_TRUE;
}

spBool swFreeData(swWave wave)
{
    if (wave == NULL) return SP_FALSE;

    if (wave->data != NULL) xfree(wave->data);
    wave->data = NULL;
    wave->alloc_size = 0;

    if (wave->custom_x_axis != NODATA) xdvfree(wave->custom_x_axis);

    return SP_TRUE;
}

spBool swAllocBuffer(swWave wave, spLong alloc_length)
{
    spLong alloc_size;
    
    if (wave == NULL) return SP_FALSE;

    alloc_size = swConvertLengthToByte(wave, alloc_length);
    spDebug(80, "swAllocBuffer", "alloc_size = %ld, wave->buf_alloc_size = %ld\n",
	    alloc_size, wave->buf_alloc_size);

    if (alloc_size <= wave->buf_alloc_size) {
	return SP_TRUE;
    } else {
	wave->buf = xrealloc(wave->buf, alloc_size, char);
	wave->buf_alloc_size = alloc_size;
    }
    spDebug(80, "swAllocBuffer", "done\n");

    return SP_TRUE;
}

spBool swFreeBuffer(swWave wave)
{
    if (wave == NULL) return SP_FALSE;

    if (wave->buf != NULL) xfree(wave->buf);
    wave->buf = NULL;
    wave->buf_alloc_size = 0;
    wave->buf_load_length = 0;

    return SP_TRUE;
}

spBool swAllocPeakBuffer(swWave wave, spLong alloc_length)
{
    spLong alloc_size;
    
    if (wave == NULL) return SP_FALSE;

    alloc_size = swConvertLengthToByte(wave, alloc_length);
    spDebug(80, "swAllocPeakBuffer", "alloc_size = %ld, wave->peak_buf_alloc_size = %ld\n",
	    alloc_size, wave->peak_buf_alloc_size);

    if (alloc_size <= wave->peak_buf_alloc_size) {
	return SP_TRUE;
    } else {
	wave->peak_buf = xrealloc(wave->peak_buf, alloc_size, char);
	wave->peak_buf_alloc_size = alloc_size;
    }
    spDebug(80, "swAllocPeakBuffer", "done\n");

    return SP_TRUE;
}

spBool swFreePeakBuffer(swWave wave)
{
    if (wave == NULL) return SP_FALSE;

    if (wave->peak_buf != NULL) xfree(wave->peak_buf);
    wave->peak_buf = NULL;
    wave->peak_buf_alloc_size = 0;
    wave->peak_buf_thin_length = 0;

    return SP_TRUE;
}

spBool swAllocReadBuffer(swWave wave, spLong alloc_size)
{
    if (wave == NULL) return SP_FALSE;

    spDebug(80, "swAllocReadBuffer", "alloc_size = %ld, wave->readbuf_size = %ld\n",
	    alloc_size, wave->readbuf_size);
    
    if (alloc_size <= wave->readbuf_size) {
	return SP_TRUE;
    } else {
	wave->readbuf = xrealloc(wave->readbuf, alloc_size, char);
	wave->readbuf_size = alloc_size;
    }
    
    return SP_TRUE;
}

spBool swFreeReadBuffer(swWave wave)
{
    if (wave == NULL) return SP_FALSE;

    if (wave->readbuf != NULL) {
	xfree(wave->readbuf);
    }
    wave->readbuf = NULL;
    wave->readbuf_size = 0;
    
    return SP_TRUE;
}

spBool swAllocEditBuffer(swWave wave, spLong alloc_length)
{
    if (wave == NULL) return SP_FALSE;

    alloc_length *= (spLong)wave->num_channel * (spLong)wave->num_order;

    if (alloc_length <= wave->editbuf_length) {
	return SP_TRUE;
    } else {
	wave->editbuf = xrealloc(wave->editbuf,
				 alloc_length * sizeof(double), char);
	wave->editbuf_length = alloc_length;
    }
    
    return SP_TRUE;
}

spBool swFreeEditBuffer(swWave wave)
{
    if (wave == NULL) return SP_FALSE;

    if (wave->editbuf != NULL) {
	xfree(wave->editbuf);
    }
    wave->editbuf = NULL;
    wave->editbuf_length = 0;
    
    return SP_TRUE;
}

spBool swInitRangeBuffer(swWave wave)
{
    long l;
    
    if (wave->minindex == NULL) {
	wave->minindex = xalloc(wave->num_order * wave->num_channel, spLong);
    }
    if (wave->maxindex == NULL) {
	wave->maxindex = xalloc(wave->num_order * wave->num_channel, spLong);
    }
    if (wave->minvalue == NULL) {
	wave->minvalue = xalloc(wave->num_order * wave->num_channel, double);
    }
    if (wave->maxvalue == NULL) {
	wave->maxvalue = xalloc(wave->num_order * wave->num_channel, double);
    }
    
    for (l = 0; l < wave->num_order * wave->num_channel; l++) {
	wave->minindex[l] = -1; wave->maxindex[l] = -1;
	wave->minvalue[l] = 0.0; wave->maxvalue[l] = 0.0;
    }

    return SP_TRUE;
}

spBool swFreeRangeBuffer(swWave wave)
{
    if (wave->minindex != NULL) {
	xfree(wave->minindex);
    }
    if (wave->maxindex != NULL) {
	xfree(wave->maxindex);
    }
    if (wave->minvalue != NULL) {
	xfree(wave->minvalue);
    }
    if (wave->maxvalue != NULL) {
	xfree(wave->maxvalue);
    }
    wave->minindex = NULL;
    wave->maxindex = NULL;
    wave->minvalue = NULL;
    wave->maxvalue = NULL;
    
    return SP_TRUE;
}

spBool swInitPeakRangeBuffer(swWave wave)
{
    long l;
    
    if (wave->peak_minindex == NULL) {
	wave->peak_minindex = xalloc(wave->num_order * wave->num_channel, spLong);
    }
    if (wave->peak_maxindex == NULL) {
	wave->peak_maxindex = xalloc(wave->num_order * wave->num_channel, spLong);
    }
    if (wave->peak_minvalue == NULL) {
	wave->peak_minvalue = xalloc(wave->num_order * wave->num_channel, double);
    }
    if (wave->peak_maxvalue == NULL) {
	wave->peak_maxvalue = xalloc(wave->num_order * wave->num_channel, double);
    }
    
    for (l = 0; l < wave->num_order * wave->num_channel; l++) {
	wave->peak_minindex[l] = -1; wave->peak_maxindex[l] = -1;
	wave->peak_minvalue[l] = 0.0; wave->peak_maxvalue[l] = 0.0;
    }

    return SP_TRUE;
}

spBool swFreePeakRangeBuffer(swWave wave)
{
    if (wave->peak_minindex != NULL) {
	xfree(wave->peak_minindex);
    }
    if (wave->peak_maxindex != NULL) {
	xfree(wave->peak_maxindex);
    }
    if (wave->peak_minvalue != NULL) {
	xfree(wave->peak_minvalue);
    }
    if (wave->peak_maxvalue != NULL) {
	xfree(wave->peak_maxvalue);
    }
    wave->peak_minindex = NULL;
    wave->peak_maxindex = NULL;
    wave->peak_minvalue = NULL;
    wave->peak_maxvalue = NULL;
    
    return SP_TRUE;
}

spBool swInitWaveRange(swWave wave)
{
    if (wave == NULL) return SP_FALSE;

    wave->min = 0; wave->max = -1;
    wave->lmin = 0; wave->lmax = -1;
    wave->dmin = 0.0; wave->dmax = -1.0;
    wave->range_available = SP_FALSE;

    return SP_TRUE;
}

void swRemovePeakFile(swWave wave)
{
    if (wave->peak_filename != NULL) {
	spDebug(10, "swRemovePeakFile", "remove peak: %s\n", wave->peak_filename);
	spRemoveFile(wave->peak_filename);
	xfree(wave->peak_filename);
    }
    wave->peak_filename = NULL;
    wave->peak_length = 0;
    wave->peak_thin_length = SW_DEFAULT_PEAK_THIN_LENGTH;
    
    return;
}

swWave swInitWave(swWaveConfig config, swWave ref_wave, const char *filename, const char *plugin_name, const char *file_type,
		  int samp_bit, int num_channel, double samp_rate, long num_order,
		  spBool orig_flag, spBool detail_flag)
{
    swWave wave = NULL;

    wave = xalloc(1, struct _swWave);

    if (ref_wave == NULL || ref_wave->core == NULL) {
	wave->core = xalloc(1, struct _swWaveCore);
	
	if (file_type != NULL) {
	    wave->core->orig_file_type = strclone(file_type);
	} else {
	    wave->core->orig_file_type = strclone("None");
	}
	if (orig_flag == SP_TRUE) {
	    if (plugin_name != NULL) {
		wave->core->orig_plugin_name = strclone(plugin_name);
	    } else {
		wave->core->orig_plugin_name = NULL;
	    }
	    if (filename != NULL) {
		wave->core->orig_filename = strclone(filename);
	    } else {
		wave->core->orig_filename = NULL;
	    }
	} else {
	    wave->core->orig_plugin_name = NULL;
	    wave->core->orig_filename = NULL;
	}
	
	wave->core->process_finished = SP_TRUE;
	wave->core->process_flag = SP_FALSE;
	wave->core->play_flag = SP_FALSE;
        wave->core->restart_flag = SP_FALSE;
        wave->core->in_record_pause = SP_FALSE;
        wave->core->record_monitor_flag = SP_FALSE;
	wave->core->last_error = SP_PLUGIN_ERROR_NO_ERROR;
	wave->core->call_data = NULL;

	swUpdateAnalysisStringInfo(wave, SW_ANALYSIS_WAVEFORM);

	wave->core->edit_thread = NULL;
	wave->core->audio_thread = NULL;
#ifdef SW_USE_THREAD
	wave->core->mutex = spCreateMutex(NULL);
	wave->core->io_mutex = spCreateMutex(NULL);
	wave->core->event = spCreateEvent(SP_TRUE, SP_TRUE);
	wave->core->thread_start_event = spCreateEvent(SP_TRUE, SP_TRUE);
#else
	wave->core->mutex = NULL;
	wave->core->io_mutex = NULL;
	wave->core->event = NULL;
	wave->core->thread_start_event = NULL;
#endif
	wave->core->ref_count = 1;
    } else {
	swLockMutex(ref_wave);
	wave->core = ref_wave->core;
	wave->core->ref_count++;
	swUnlockMutex(ref_wave);
    }
    
    if (plugin_name != NULL) {
	spDebug(40, "swInitWave", "set plugin name: plugin_name = %s\n", plugin_name);
	wave->plugin_name = strclone(plugin_name);
    } else {
	wave->plugin_name = NULL;
    }
    if (filename != NULL) {
	wave->filename = strclone(filename);
    } else {
	wave->filename = NULL;
    }
    if (file_type != NULL) {
	spDebug(40, "swInitWave", "file_type = %s\n", file_type);
	wave->file_type = strclone(file_type);
    } else {
	wave->file_type = NULL;
    }
    wave->new_plugin_name = NULL;
    wave->new_filename = NULL;
    wave->new_file_type = NULL;
    
    wave->orig_flag = orig_flag;
    wave->detail_flag = detail_flag;
    wave->edit_flag = SP_FALSE;
    wave->order_frequency_flag = SP_FALSE;
    wave->overflow = SP_FALSE;
    wave->thread_safe = SP_FALSE;
    wave->num_channel = num_channel;
    wave->selected_channel = -1;
    wave->samp_bit = samp_bit;
    wave->samp_rate = samp_rate;
    wave->exact_samp_rate = samp_rate;
    wave->song_info_mask = 0;
    spInitSongInfoV2(&wave->song_info);
    wave->num_order = num_order;

    wave->offset = 0;
    wave->length = 0;
    wave->data_length = 0;
    wave->total_length = 0;
    wave->thin_length = 1;
    wave->alloc_size = 0;
    if (wave->samp_bit > 32) {
	wave->float_flag = SP_TRUE;
    } else {
	wave->float_flag = SP_FALSE;
    }

    wave->data = NULL;
    wave->min = 0;
    wave->max = -1;
    wave->lmin = 0;
    wave->lmax = -1;
    wave->dmin = 0.0;
    wave->dmax = -1.0;
    wave->range_available = SP_FALSE;

    wave->log_like_custom_x_axis = SP_FALSE;
    wave->custom_x_axis = NODATA;
    
    wave->peak_filename = NULL;
    wave->peak_length = 0;
    wave->peak_thin_length = /* num_order * */SW_DEFAULT_PEAK_THIN_LENGTH;

    wave->peak_buf_alloc_size = 0;
    wave->peak_buf_length = 0;
    wave->peak_buf_thin_length = 0;
    wave->peak_buf = NULL;

    wave->buf_alloc_size = 0;
    wave->buf_length = 0;
    wave->buf_load_length = 0;
    wave->buf = NULL;
    swAllocBuffer(wave, MAX(8, config->buffer_length / num_order));

    wave->readbuf_size = 0;
    wave->readbuf = NULL;
    swAllocReadBuffer(wave, swConvertLengthToByte(wave,
						  MAX(8, config->normal_read_length / num_order)));
    
    wave->editbuf_length = 0;
    wave->editbuf = NULL;
    
    wave->edit_offset = 0;
    wave->edit_length = 0;
    wave->play_offset = 0;
    wave->play_length = 0;
    wave->play_start_offset = 0;
    wave->play_start_offset_updated = SP_FALSE;
    wave->play_start_sync_pos = SP_TRUE;
    wave->suspend_play_callback = SP_FALSE;

    wave->minindex = NULL;
    wave->maxindex = NULL;
    wave->minvalue = NULL;
    wave->maxvalue = NULL;
    wave->peak_minindex = NULL;
    wave->peak_maxindex = NULL;
    wave->peak_minvalue = NULL;
    wave->peak_maxvalue = NULL;
    
    wave->labels = NULL;

    wave->config = config;

    wave->edit_name = NULL;
    wave->prev_wave = NULL;
    wave->next_wave = NULL;

    spDebug(80, "swInitWave", "done\n");
    
    return wave;
}

void _swFreeWave(swWave wave, spBool lock_flag)
{
    spDebug(80, "swFreeWave", "in\n");
    
    if (wave != NULL) {
        spDebug(80, "swFreeWave", "before swLockMutex\n");
	if (lock_flag == SP_TRUE) swLockMutex(wave);
	--wave->core->ref_count;
	if (lock_flag == SP_TRUE) swUnlockMutex(wave);
        spDebug(80, "swFreeWave", "after swUnlockMutex\n");
	
	if (wave->core->ref_count <= 0) {
#ifdef SW_USE_THREAD
            wave->core->process_config.wave = NULL;
            
	    if (wave->core->edit_thread != NULL) {
                if (wave->core->in_edit_thread == SP_FALSE) {
                    spDebug(80, "swFreeWave", "before thread_wait_func for edit_thread\n");
                    wave->config->thread_wait_func(wave, wave->core->edit_thread, wave->core->call_data);
                    spDebug(80, "swFreeWave", "after thread_wait_func for edit_thread\n");
                }
		spDestroyThread(wave->core->edit_thread);
                spDebug(80, "swFreeWave", "after spDestroyThread for edit_thread\n");
	    }
	    if (wave->core->audio_thread != NULL) {
                if (wave->core->in_audio_thread == SP_FALSE) {
                    wave->config->thread_wait_func(wave, wave->core->audio_thread, wave->core->call_data);
                }
		spDestroyThread(wave->core->audio_thread);
	    }
	    if (wave->core->mutex != NULL) {
		spDestroyMutex(wave->core->mutex);
	    }
	    if (wave->core->io_mutex != NULL) {
		spDestroyMutex(wave->core->io_mutex);
	    }
	    if (wave->core->event != NULL) {
		spDestroyEvent(wave->core->event);
	    }
#endif
            
	    if (wave->core->orig_plugin_name != NULL) xfree(wave->core->orig_plugin_name);
	    if (wave->core->orig_filename != NULL) xfree(wave->core->orig_filename);
	    if (wave->core->orig_file_type != NULL) xfree(wave->core->orig_file_type);

	    xfree(wave->core);
	}
	
	if (wave->prev_wave != NULL) {
	    wave->prev_wave->next_wave = wave->next_wave;
	}
	if (wave->next_wave != NULL) {
	    wave->next_wave->prev_wave = wave->prev_wave;
	}

	swRemovePeakFile(wave);

	if (wave->orig_flag == SP_FALSE) {
	    spRemoveFile(wave->filename);
	    spDebug(10, "swFreeWave", "remove file done: %s\n", wave->filename);
	}
	
	if (wave->plugin_name != NULL) xfree(wave->plugin_name);
	if (wave->filename != NULL) xfree(wave->filename);
	if (wave->file_type != NULL) xfree(wave->file_type);
	if (wave->new_plugin_name != NULL) xfree(wave->new_plugin_name);
	if (wave->new_filename != NULL) xfree(wave->new_filename);
	if (wave->new_file_type != NULL) xfree(wave->new_file_type);

	/* free buffers */
	swFreeData(wave);
	swFreeBuffer(wave);
	swFreeReadBuffer(wave);
	swFreeEditBuffer(wave);
	swFreePeakBuffer(wave);
	swFreeRangeBuffer(wave);
	swFreePeakRangeBuffer(wave);
	
	if (wave->labels != NULL) swFreeLabels(wave->labels);
	
	if (wave->edit_name != NULL) xfree(wave->edit_name);
	
	xfree(wave);
    }

    spDebug(80, "swFreeWave", "done\n");
    
    return;
}

void _swDestroyWave(swWave wave)
{
    swWave temp, temp2;
    
    if (wave != NULL) {
	temp = wave->prev_wave;
	while (temp != NULL) {
	    temp2 = temp->prev_wave;
	    swFreeWave(temp, SP_TRUE);
	    temp = temp2;
	}

	temp = wave->next_wave;
	while (temp != NULL) {
	    temp2 = temp->next_wave;
	    swFreeWave(temp, SP_TRUE);
	    temp = temp2;
	}
	
	swFreeWave(wave, SP_TRUE);
    }
    return;
}

unsigned long swGetSongInfoMask(swWave wave)
{
    if (wave == NULL) {
	return SP_SONG_NO_INFO;
    }

    return wave->song_info_mask;
}

typedef struct _swCallbackData {
    swWaveConfig config;
    swWave wave;
    spBool in_thread;
    void *call_data;
} swCallbackData;

static spBool pluginOpenCB(spPlugin *o_plugin, spPluginCallbackReason reason,
			   void *host_data, void *data)
{
    spBool flag = SP_TRUE;
    swCallbackData *call_data = (swCallbackData *)data;

    if (reason == SP_PLUGIN_CR_OPTION) {
	spOptions options = (spOptions)host_data;
	
	if (call_data->wave != NULL && call_data->config->option_func != NULL) {
	    if (call_data->config->option_func(call_data->wave, call_data->in_thread,
					       options, call_data->call_data) == SP_FALSE) {
		flag = SP_FALSE;
	    }
	}
    } else if (reason == SP_PLUGIN_CR_ERROR) {
	int error = (int)host_data;

        if (call_data->wave != NULL) {
            call_data->wave->core->last_error = error;
        }
        
	if (call_data->config->error_func != NULL) {
	    if (call_data->config->error_func(call_data->wave, call_data->in_thread,
					      error, SW_EDIT_NONE, call_data->call_data) == SP_FALSE) {
		flag = SP_FALSE;
	    }
	}
    }

    return flag;
}

spLong swUpdateTotalLength(spPlugin *plugin, swWave wave)
{
    spLong total_length;
    
    wave->thread_safe = spIsPluginCapable(plugin, SP_PLUGIN_CAPS_THREAD_SAFE);
    spDebug(10, "swUpdateTotalLength", "wave->thread_safe = %d, wave->num_order = %ld\n",
            wave->thread_safe, wave->num_order);
    
    if ((total_length = spGetPluginTotalLength(plugin) / wave->num_order) > 0) {
	wave->total_length = total_length;
    }
    spDebug(10, "swUpdateTotalLength", "total_length = %ld\n", total_length);

    return total_length;
}

static swWave getWave(swWaveConfig config, swWave ref_wave, const char *filename, const char *plugin_name, const char *file_type,
		      int samp_bit, int num_channel, double samp_rate, long num_order,
		      spBool orig_flag, spBool detail_flag, spBool callback_flag,
		      spPluginError *error, spBool in_thread)
{
    swWave wave;
    char *string;
    spPlugin *plugin;
    spWaveInfo wave_info;
    spSongInfoV2 song_info;
    spPluginError err;
    spPluginOpenCallback opencb = NULL;
    swCallbackData call_data = {NULL, NULL, SP_FALSE, NULL};
    
    if (strnone(filename)) return NULL;

    spDebug(10, "getWave", "orig_flag = %d, detail_flag = %d, callback_flag = %d\n",
	    orig_flag, detail_flag, callback_flag);
    
    /* initialize option */
    spInitWaveInfo(&wave_info);
    if (orig_flag == SP_TRUE) {
	wave_info.samp_bit = samp_bit;
    } else {
	wave_info.samp_bit = MAX(samp_bit, 16);
    }
    wave_info.num_channel = num_channel;
    wave_info.samp_rate = samp_rate;
    if (!strnone(file_type)) {
	spStrCopy(wave_info.file_type, SP_WAVE_FILE_TYPE_SIZE, file_type);
        spDebug(80, "getWave", "wave_info.file_type = %s\n", wave_info.file_type);
    }
    num_order = MAX(1, num_order);
    spDebug(80, "getWave", "samp_rate = %f, num_order = %ld\n", samp_rate, num_order);

    if (callback_flag == SP_TRUE) {
	opencb = (spPluginOpenCallback)pluginOpenCB;
	call_data.config = config;
	call_data.in_thread = in_thread;
	call_data.wave = NULL;
	call_data.call_data = NULL;
    }
    
    /* open file */
    if ((plugin = spOpenFilePlugin(NULL, filename, "r",
				   SP_PLUGIN_DEVICE_FILE,
				   &wave_info, (spSongInfo *)&song_info, opencb, (void *)&call_data, &err)) == NULL) {
	if (error != NULL) *error = err;
	
	if (err != SP_PLUGIN_ERROR_SUITABLE_NOT_FOUND && err != SP_PLUGIN_ERROR_FAILURE) {
	    return NULL;
	}
	spDebug(10, "getWave", "can't find suitable plugin: err = %d\n", err);
	if (strnone(plugin_name)) {
	    if (strnone(file_type)) {
		return NULL;
	    }
		
	    if (strncaseeq(file_type, "text", 4)
		|| strcaseeq(file_type, "time")
		|| strcaseeq(file_type, "freq")
		|| strcaseeq(file_type, SP_WAVE_FORMAT_TEXT_LABEL)
		|| strcaseeq(file_type, SP_WAVE_FORMAT_TEXT_TIME_LABEL)
		|| strcaseeq(file_type, SP_WAVE_FORMAT_TEXT_FREQ_LABEL)) {
		plugin_name = "input_text";
	    } else {
		spStrCopy(wave_info.file_type, SP_WAVE_FILE_TYPE_SIZE, file_type);
		plugin_name = "input_raw";
	    }

	    spDebug(10, "getWave", "before open: file_type = %s\n", file_type);
	}
	spDebug(10, "getWave", "before open: plugin_name = %s\n", plugin_name);
	
	if ((plugin = spOpenFilePlugin(plugin_name, filename, "r",
				       SP_PLUGIN_DEVICE_FILE,
				       &wave_info, (spSongInfo *)&song_info, /*opencb*/NULL, (void *)&call_data, error)) == NULL) {
	    spDebug(10, "getWave", "can't open file: %s\n", filename);
	    return NULL;
	}
    }
    if (orig_flag == SP_TRUE) {
	samp_bit = wave_info.samp_bit;
    }

    spDebug(10, "getWave", "after open: file_type = %s\n", wave_info.file_type);

    string = xspGetPluginFileType(plugin, SP_TRUE);
    wave = swInitWave(config, ref_wave, filename, spGetPluginName(plugin),
		      string, samp_bit, wave_info.num_channel, wave_info.samp_rate, num_order,
		      orig_flag, detail_flag);
    xfree(string);
    spCopySongInfoV2(&wave->song_info, &song_info);
    if (orig_flag == SP_TRUE) {
	spGetPluginSongInfoMask(plugin, &wave->song_info_mask);
        spDebug(10, "getWave", "spGetPluginSongInfoMask result: wave->song_info_mask = %lx\n", wave->song_info_mask);
    }

    if (swUpdateTotalLength(plugin, wave) <= 0) {
	wave->total_length = config->max_read_length;
    }
    spCloseFilePlugin(plugin);

#if 1
    if (orig_flag == SP_TRUE) {
	if (config->load_default_label == SP_TRUE
	    && (!strnone(config->default_label_suffix) || !strnone(config->default_region_label_suffix))) {
	    char label_file[SP_MAX_PATHNAME];

            if (!strnone(config->default_label_suffix)) {
                spStrCopy(label_file, sizeof(label_file), filename);

                if (spReplaceSuffix(label_file, config->default_label_suffix) == SP_TRUE
                    && !streq(label_file, filename)) {
                    swReadLabel(wave, label_file, SW_TIME_FORMAT_UNKNOWN, SP_FALSE);
                }
            }
            if (!strnone(config->default_region_label_suffix)
                && !streq(config->default_region_label_suffix, config->default_label_suffix)) {
                spStrCopy(label_file, sizeof(label_file), filename);

                if (spReplaceSuffix(label_file, config->default_region_label_suffix) == SP_TRUE
                    && !streq(label_file, filename)) {
                    swReadLabel(wave, label_file, SW_TIME_FORMAT_UNKNOWN, SP_TRUE);
                }
            }
	}
    }
#endif
    
    spDebug(10, "getWave", "total_length = %ld\n", wave->total_length);
    
    return wave;
}

swWave swGetWave(swWaveConfig config, swWave ref_wave, const char *filename, const char *plugin_name, const char *file_type,
		 int samp_bit, int num_channel, double samp_rate, long num_order,
		 spBool orig_flag, spBool detail_flag, spBool callback_flag, spPluginError *error)
{
    return getWave(config, ref_wave, filename, plugin_name, file_type,
		   samp_bit, num_channel, samp_rate, num_order,
		   orig_flag, detail_flag, callback_flag, error, SP_FALSE);
}

spPlugin *swOpenWave(swWave wave, char *mode)
{
    spWaveInfo wave_info;
    char *plugin_name;
    spPlugin *plugin;
    
    if (wave == NULL || strnone(wave->plugin_name) || mode == NULL) {
	return NULL;
    }

    if (strnone(wave->filename)) {
	spDebug(20, "swOpenWave", "error: no filename\n");
	return SP_FALSE;
    }
    
    if (mode[0] == 'w') {
	if ((plugin_name = xspFindRelatedPluginFile(wave->plugin_name)) == NULL) {
	    return NULL;
	}
    } else {
	plugin_name = strclone(wave->plugin_name);
    }
    spDebug(20, "swOpenWave", "plugin_name = %s\n", plugin_name);
    
    /* initialize option */
    spInitWaveInfo(&wave_info);
    if (wave->file_type != NULL) {
	spDebug(20, "swOpenWave", "format = %s\n", wave->file_type);
	spStrCopy(wave_info.file_type, SP_WAVE_FILE_TYPE_SIZE, wave->file_type);
    }
    if (wave->orig_flag == SP_TRUE) {
	wave_info.samp_bit = wave->samp_bit;
    } else {
	wave_info.samp_bit = MAX(wave->samp_bit, 16);
    }
    wave_info.num_channel = wave->num_channel;
    wave_info.samp_rate = wave->samp_rate;
    spDebug(20, "swOpenWave", "num_channel = %d, samp_bit = %d\n",
	    wave_info.num_channel, wave_info.samp_bit);

    /* open file */
    if ((plugin = spOpenFilePlugin(plugin_name, wave->filename, mode,
				   SP_PLUGIN_DEVICE_FILE,
				   &wave_info, NULL, NULL, NULL, NULL)) == NULL) {
	spDebug(20, "swOpenWave", "spOpenFilePlugin error\n");
	xfree(plugin_name);
	return NULL;
    }

    spDebug(10, "swOpenWave", "file_type = %s\n", wave_info.file_type);
    spDebug(10, "swOpenWave", "total_length = %ld\n", wave->total_length);
    
    xfree(plugin_name);
    
    return plugin;
}

static spLong getInitThinLength(swWave wave, spLong length)
{
    spLong thin_length_weight;
    spLong thin_length;
    double factor;
    double order_weight = 0.3;

    if (wave->custom_x_axis != NODATA) {
        return 1;
    }
    
    /*thin_length = (spLong)ceil((double)length / (double)wave->config->max_read_length);*/
    /*thin_length = wave->num_order * (spLong)ceil((double)length / (double)wave->config->max_read_length);*/
    thin_length = (spLong)ceil((order_weight * (double)wave->num_order + (1.0 - order_weight))
			     * (double)length / (double)wave->config->max_read_length);
    spDebug(10, "getInitThinLength", "length = %ld, thin_length = %ld\n", length, thin_length);

    /* prevent from too much data thin */
    if (wave->detail_flag == SP_TRUE) {
	/*1024.0*//*512.0*/
	factor = 1024.0;
    } else {
	/*256.0*//*128.0*/
	factor = 512.0;
    }
    thin_length = MIN(thin_length, (spLong)ceil((double)/*wave->total_length*/length / factor));
    spDebug(10, "getInitThinLength", "thin_length = %ld\n", thin_length);

    /* if thin_length is sufficient, using peak_thin_length */
    if (wave->detail_flag == SP_TRUE) {
	if (wave->num_order > 1) {
	    thin_length_weight = 2;
	} else {
	    thin_length_weight = 32;
	}
    } else {
	if (wave->num_order > 1) {
	    thin_length_weight = 4;
	} else {
	    thin_length_weight = 64;
	}
    }
    if (wave->peak_thin_length > 1 && thin_length >= SW_SUFFICIENT_THIN_LENGTH
	&& thin_length * thin_length_weight >= wave->peak_thin_length) {
	spDebug(10, "getInitThinLength", "using peak_thin_length = %ld, thin_length = %ld\n",
		wave->peak_thin_length, thin_length);
	thin_length = MAX(wave->peak_thin_length, thin_length);
    }

    thin_length = MAX(thin_length, 1);
    spDebug(10, "getInitThinLength", "final thin_length = %ld\n", thin_length);

    return thin_length;
}

static void getThinReadLength(swWave wave, spLong length, spLong thin_length, spLong *read_length)
{
    if (read_length != NULL) {
	if (thin_length >= 2) {
	    *read_length = 2 * (spLong)ceil((double)(length * wave->num_order * (spLong)wave->num_channel)
					  / (double)(2 * thin_length * wave->num_order * (spLong)wave->num_channel));
	} else {
	    *read_length = length;
	}
    }

    return;
}

static spLong getThinLength(swWave wave, spLong length, spLong thin_length, spLong *read_length)
{
    if (wave->custom_x_axis == NODATA && thin_length != 1 /*&& length >= wave->config->max_read_length*/) {
	if (thin_length <= 0) {
	    thin_length = getInitThinLength(wave, length);
	}
	getThinReadLength(wave, length, thin_length, read_length);
    } else {
	thin_length = 1;
	if (read_length != NULL) {
	    *read_length = length;
	}
    }

    return thin_length;
}

static spLong readPeakFile(swWave wave, spLong offset, spLong thin_length, spLong read_length,
			 char *data, spBool callback_flag, spBool in_thread)
{
    spBool flag = SP_TRUE;
    spLong k, l;
    spLong ndata;
    spLong offset_byte;
    spLong samp_byte;
    spLong pos, prev_pos;
    spLong nread;
    spLong buf_pos;
    spLong data_pos;
    spLong prev_call_pos;
    spLong read_pos;
    spLong data_length;
    spLong buf_length;
    double value;
    short *sbuf, *sdata;
    long *lbuf, *ldata;
    double *dbuf, *ddata;
    spBool play_flag, prev_process_flag;
    FILE *peak_fp;
    
    if (wave == NULL || offset < 0 || read_length <= 0
	|| thin_length < wave->peak_thin_length	|| wave->peak_filename == NULL) return -1;

    spLockMutex(wave->core->io_mutex);
    
    if ((peak_fp = spOpenFile(wave->peak_filename, "rb")) == NULL) {
	spDebug(10, "readPeakFile", "spOpenFile of %s failed\n", wave->peak_filename);
	spUnlockMutex(wave->core->io_mutex);
	return -1;
    }
    spDebug(10, "readPeakFile", "wave->peak_filename = %s, wave->peak_length = %ld\n",
	    wave->peak_filename, wave->peak_length);
    
    if (callback_flag == SP_TRUE && wave->config->read_func != NULL) {
	flag = wave->config->read_func(wave, in_thread, SW_PROCESS_STARTED, wave->core->call_data);
    }
    
    data_pos = 0;
    prev_call_pos = 0;
    read_pos = 0;
    
    if (flag == SP_TRUE) {
	buf_length = MIN(swConvertByteToLength(wave, wave->readbuf_size), read_length);
	data_length = swConvertLengthToBufferLength(wave, read_length);

	samp_byte = swGetWaveSampleByte(wave);

	if (wave->peak_thin_length >= 2) {
	    offset_byte = samp_byte * 2 * (offset / (2 * wave->peak_thin_length))
		* wave->num_order * (spLong)wave->num_channel;
	} else {
	    offset_byte = samp_byte * (offset / (wave->peak_thin_length))
		* wave->num_order * (spLong)wave->num_channel;
	}
	
	spDebug(10, "readPeakFile", "buf_length = %ld, data_length = %ld, offset_byte = %ld\n",
		buf_length, data_length, offset_byte);

	if (offset_byte > 0) spSeekFile(peak_fp, offset_byte, 0);
	
	sbuf = (short *)wave->readbuf;
	lbuf = (long *)wave->readbuf;
	dbuf = (double *)wave->readbuf;
	sdata = (short *)data;
	ldata = (long *)data;
	ddata = (double *)data;

	swInitRangeBuffer(wave);
    
	prev_process_flag = wave->core->process_flag;
	play_flag = wave->core->play_flag;

	prev_pos = 0;
	while (1) {
	    if ((nread = fread(wave->readbuf, samp_byte, swConvertLengthToBufferLength(wave, buf_length), peak_fp)) <= 0) {
		spDebug(80, "readPeakFile", "fread finished: nread = %ld\n", nread);
		break;
	    }
	    spDebug(100, "readPeakFile", "nread = %ld, data_pos = %ld\n", nread, data_pos);
	
	    for (buf_pos = 0; buf_pos < nread; buf_pos += (wave->num_order * wave->num_channel)) {
		if (thin_length == wave->peak_thin_length) {
		    for (l = 0; l < wave->num_order * wave->num_channel; l++) {
			k = buf_pos + l;
			if (swIsWaveFloat(wave) == SP_TRUE) {
			    ddata[data_pos] = dbuf[k];
			} else if (swIsWaveLong(wave) == SP_TRUE) {
			    ldata[data_pos] = lbuf[k];
			} else {
			    sdata[data_pos] = sbuf[k];
			}
			data_pos++;
		    }
		} else {
		    pos = swConvertBufferLengthToLength(wave, read_pos + buf_pos) * wave->peak_thin_length;
		    for (l = 0; l < wave->num_order * wave->num_channel; l++) {
			k = buf_pos + l;
			if (swIsWaveFloat(wave) == SP_TRUE) {
			    value = dbuf[k];
			} else if (swIsWaveLong(wave) == SP_TRUE) {
			    value = (double)lbuf[k];
			} else {
			    value = (double)sbuf[k];
			}
		    
			if (wave->maxindex[l] < 0 || value > wave->maxvalue[l]) {
			    wave->maxvalue[l] = value;
			    wave->maxindex[l] = pos;
			}
			if (wave->minindex[l] < 0 || value < wave->minvalue[l]) {
			    wave->minvalue[l] = value;
			    wave->minindex[l] = pos;
			}
		    }
		    
		    if ((pos / wave->peak_thin_length + 1) % 2 == 0 
			&& pos - prev_pos + wave->peak_thin_length >= 2 * thin_length) {
			for (k = 0; k < 2; k++) {
			    for (l = 0; l < wave->num_order * wave->num_channel; l++) {
				if (k == 0) {
				    if (wave->minindex[l] < wave->maxindex[l]) {
					value = wave->minvalue[l];
				    } else {
					value = wave->maxvalue[l];
				    }
				} else {
				    if (wave->minindex[l] < wave->maxindex[l]) {
					value = wave->maxvalue[l];
				    } else {
					value = wave->minvalue[l];
				    }
				}
			
				if (swIsWaveFloat(wave) == SP_TRUE) {
				    ddata[data_pos] = value;
				} else if (swIsWaveLong(wave) == SP_TRUE) {
				    ldata[data_pos] = (long)value;
				} else {
				    sdata[data_pos] = (short)value;
				}
				data_pos++;
			    }
			}
			
			for (l = 0; l < wave->num_order * wave->num_channel; l++) {
			    wave->minindex[l] = -1; wave->maxindex[l] = -1;
			    wave->minvalue[l] = 0.0; wave->maxvalue[l] = 0.0;
			}
		
			prev_pos += 2 * thin_length;
		    }
		}
		
		spDebug(100, "readPeakFile", "data_pos = %ld, data_length = %ld\n", data_pos, data_length);
		if (data_pos >= data_length) {
		    nread = 0;
		    break;
		}
	    }

	    if (nread <= 0) {
		break;
	    }
	    if (play_flag == SP_FALSE && prev_process_flag != wave->core->process_flag) {
		data_pos = 0;
		break;
	    }

	    spDebug(80, "readPeakFile", "data_pos = %ld, prev_call_pos = %ld\n", data_pos, prev_call_pos);
	    if (callback_flag == SP_TRUE && wave->config->read_func != NULL
		&& thin_length * (data_pos - prev_call_pos) >= wave->config->read_callback_length) {
		flag = wave->config->read_func(wave, in_thread,
					       swConvertBufferLengthToLength(wave, data_pos), wave->core->call_data);
		prev_call_pos = data_pos;
		if (flag == SP_FALSE) {
		    break;
		}
	    }

	    read_pos += nread;
	}
    }
    
    spCloseFile(peak_fp);
    
    if (callback_flag == SP_TRUE && wave->config->read_func != NULL) {
	flag = wave->config->read_func(wave, in_thread, SW_PROCESS_FINISHED, wave->core->call_data);
    }
    
    spUnlockMutex(wave->core->io_mutex);

    ndata = swConvertBufferLengthToLength(wave, data_pos);
    spDebug(80, "readPeakFile", "done: thin_length = %ld, ndata = %ld\n",
	    thin_length, ndata);
	
    return ndata;
}

static spLong readThinWaveUsingPeak(swWave wave, spLong offset, spLong thin_length, spLong read_length,
				  spBool callback_flag, spBool in_thread)
{
    spLong nread;
    
    spDebug(80, "readThinWaveUsingPeak",
	    "offset = %ld, thin_length = %ld, read_length = %ld\n",
	    offset, thin_length, read_length);
    
    swAllocData(wave, 256 * (((read_length - 1) / 256) + 1));

    spDebug(80, "readThinWaveUsingPeak",
	    "peak_buf_length = %ld, peak_buf_thin_length = %ld\n",
	    wave->peak_buf_length, wave->peak_buf_thin_length);
    
    if (thin_length == wave->peak_buf_thin_length) {
	spLong offsetbyte;
	spLong readbyte;
	spLong peak_offset;
	spLong avail_length;

	spLockMutex(wave->core->io_mutex);

	if (thin_length >= 2) {
	    peak_offset = 2 * (offset / (2 * thin_length));
	} else {
	    peak_offset = offset;
	}
	avail_length = wave->peak_buf_length - peak_offset;
	spDebug(80, "readThinWaveUsingPeak", "avail_length = %ld\n", avail_length);

	if (avail_length <= 0) {
	    nread = 0;
	} else {
	    if (read_length > avail_length) {
		spDebug(80, "readThinWaveUsingPeak",
			"******** read_length %ld over peak_buf_length %ld\n",
			read_length, avail_length);
		read_length = avail_length;
	    }

	    offsetbyte = swConvertLengthToByte(wave, peak_offset);
	    readbyte = swConvertLengthToByte(wave, read_length);
	    spDebug(100, "readThinWaveUsingPeak",
		    "peak_offset = %ld, offsetbyte = %ld, readbyte = %ld\n",
		    peak_offset, offsetbyte, readbyte);
	    memmove(wave->data, wave->peak_buf + offsetbyte, readbyte);
	
	    nread = read_length;
	}
	
	spUnlockMutex(wave->core->io_mutex);
    } else {
	nread = readPeakFile(wave, offset, thin_length, read_length,
			     wave->data, callback_flag, in_thread);
    }

    if (nread > 0) {
	swLockMutex(wave);
	wave->length = nread;
	swUnlockMutex(wave);
    }

    return nread;
}

static spLong preparePeakBuffer(swWave wave, spLong *read_length)
{
    spLong thin_length;
    
    thin_length = getThinLength(wave, wave->total_length, 0, read_length);
    if (thin_length < wave->peak_thin_length) {
	thin_length = wave->peak_thin_length;
	getThinReadLength(wave, wave->total_length, thin_length, read_length);
    }

    spDebug(80, "preparePeakBuffer", "thin_length = %ld, peak_thin_length = %ld, read_length = %ld\n",
	    thin_length, wave->peak_thin_length, *read_length);
    
    swAllocPeakBuffer(wave, 256 * (((*read_length - 1) / 256) + 1));

    return thin_length;
}

static spLong readPeakBuffer(swWave wave, spBool in_thread)
{
    spLong thin_length;
    spLong read_length;
    spLong peak_buf_length;

    thin_length = preparePeakBuffer(wave, &read_length);

    if ((peak_buf_length = readPeakFile(wave, 0, thin_length, read_length,
					wave->peak_buf, SP_FALSE, in_thread)) > 0) {
	swLockMutex(wave);
	wave->peak_buf_length = peak_buf_length;
	wave->peak_buf_thin_length = thin_length;
	spDebug(80, "readPeakBuffer", "peak_buf_length = %ld, peak_buf_thin_length = %ld\n",
		wave->peak_buf_length, wave->peak_buf_thin_length);
	swUnlockMutex(wave);
    }
    
    spDebug(80, "readPeakBuffer", "peak_buf_length = %ld\n", peak_buf_length);

    return peak_buf_length;
}

static spLong calculatePeakData(swWave wave, void *data, spLong *data_pos, spLong data_length,
                                spLong *read_pos, void *buf, spLong nread, FILE *peak_fp, spBool get_range)
{
    spLong l, k;
    spLong buf_pos;
    spLong offset;
    spLong nwrite;
    double value;
    short *sbuf, *sdata;
    long *lbuf, *ldata;
    double *dbuf, *ddata;
    
    sbuf = (short *)buf;
    lbuf = (long *)buf;
    dbuf = (double *)buf;
    sdata = (short *)data;
    ldata = (long *)data;
    ddata = (double *)data;
	
    swLockMutex(wave);
    
    for (buf_pos = 0; buf_pos < nread; buf_pos += (wave->num_order * wave->num_channel)) {
	offset = swConvertBufferLengthToLength(wave, (*read_pos + buf_pos));
	for (l = 0; l < wave->num_order * wave->num_channel; l++) {
	    k = buf_pos + l;
	    if (swIsWaveFloat(wave) == SP_TRUE) {
		value = dbuf[k];
		if (get_range == SP_TRUE) {
		    if (dbuf[k] < wave->dmin) wave->dmin = dbuf[k];
		    if (dbuf[k] > wave->dmax) wave->dmax = dbuf[k];
		}
	    } else if (swIsWaveLong(wave) == SP_TRUE) {
		value = (double)lbuf[k];
		if (get_range == SP_TRUE) {
		    if (lbuf[k] < wave->lmin) wave->lmin = lbuf[k];
		    if (lbuf[k] > wave->lmax) wave->lmax = lbuf[k];
		}
	    } else {
		value = (double)sbuf[k];
		if (get_range == SP_TRUE) {
		    if (sbuf[k] < wave->min) wave->min = sbuf[k];
		    if (sbuf[k] > wave->max) wave->max = sbuf[k];
		}
	    }
		    
	    if (wave->thin_length > 1) {
		if (wave->maxindex[l] < 0 || value > wave->maxvalue[l]) {
		    wave->maxvalue[l] = value;
		    wave->maxindex[l] = offset;
		}
		if (wave->minindex[l] < 0 || value < wave->minvalue[l]) {
		    wave->minvalue[l] = value;
		    wave->minindex[l] = offset;
		}
	    }

	    if (peak_fp != NULL && wave->peak_length >= 0) {
		if (wave->peak_thin_length > 1) {
		    if (wave->peak_maxindex[l] < 0 || value > wave->peak_maxvalue[l]) {
			wave->peak_maxvalue[l] = value;
			wave->peak_maxindex[l] = offset;
		    }
		    if (wave->peak_minindex[l] < 0 || value < wave->peak_minvalue[l]) {
			wave->peak_minvalue[l] = value;
			wave->peak_minindex[l] = offset;
		    }
		}
	    }
	}

	if (peak_fp != NULL && wave->peak_length >= 0) {
	    if (wave->peak_thin_length > 1) {
		/* making peak file */
		if ((offset + 1) % (2 * wave->peak_thin_length) == 0) {
		    for (k = 0; k < 2; k++) {
			for (l = 0; l < wave->num_order * wave->num_channel; l++) {
			    if (k == 0) {
				if (wave->peak_minindex[l] < wave->peak_maxindex[l]) {
				    value = wave->peak_minvalue[l];
				} else {
				    value = wave->peak_maxvalue[l];
				}
			    } else {
				if (wave->peak_minindex[l] < wave->peak_maxindex[l]) {
				    value = wave->peak_maxvalue[l];
				} else {
				    value = wave->peak_minvalue[l];
				}
			    }

			    if (swIsWaveFloat(wave) == SP_TRUE) {
				nwrite = fwritedouble(&value, 1, 0, peak_fp);
			    } else if (swIsWaveLong(wave) == SP_TRUE) {
				nwrite = fwritedoubletol(&value, 1, 0, peak_fp);
			    } else {
				nwrite = fwritedoubletos(&value, 1, 0, peak_fp);
			    }

			    if (nwrite > 0) {
				wave->peak_length += nwrite;
			    } else {
				wave->peak_length = -1;
				break;
			    }
			}
			if (wave->peak_length < 0) {
			    break;
			}
		    }
			
		    for (l = 0; l < wave->num_order * wave->num_channel; l++) {
			wave->peak_minindex[l] = -1; wave->peak_maxindex[l] = -1;
			wave->peak_minvalue[l] = 0.0; wave->peak_maxvalue[l] = 0.0;
		    }
		}
	    } else {
		if (swIsWaveFloat(wave) == SP_TRUE) {
		    nwrite = fwritedouble(dbuf + buf_pos, wave->num_order * wave->num_channel, 0, peak_fp);
		} else if (swIsWaveLong(wave) == SP_TRUE) {
		    nwrite = fwritelong(lbuf + buf_pos, wave->num_order * wave->num_channel, 0, peak_fp);
		} else {
		    nwrite = fwriteshort(sbuf + buf_pos, wave->num_order * wave->num_channel, 0, peak_fp);
		}

		if (nwrite > 0) {
		    wave->peak_length += nwrite;
		} else {
		    wave->peak_length = -1;
		}
	    }
	}

        if (data != NULL) {
            if (wave->thin_length > 1) {
                if ((offset + 1) % (2 * wave->thin_length) == 0) {
                    for (k = 0; k < 2; k++) {
                        for (l = 0; l < wave->num_order * wave->num_channel; l++) {
                            if (k == 0) {
                                if (wave->minindex[l] < wave->maxindex[l]) {
                                    value = wave->minvalue[l];
                                } else {
                                    value = wave->maxvalue[l];
                                }
                            } else {
                                if (wave->minindex[l] < wave->maxindex[l]) {
                                    value = wave->maxvalue[l];
                                } else {
                                    value = wave->minvalue[l];
                                }
                            }
			
                            if (swIsWaveFloat(wave) == SP_TRUE) {
                                ddata[*data_pos] = value;
                            } else if (swIsWaveLong(wave) == SP_TRUE) {
                                ldata[*data_pos] = (long)value;
                            } else {
                                sdata[*data_pos] = (short)value;
                            }
                            (*data_pos)++;
                        }
                    }
			
                    for (l = 0; l < wave->num_order * wave->num_channel; l++) {
                        wave->minindex[l] = -1; wave->maxindex[l] = -1;
                        wave->minvalue[l] = 0.0; wave->maxvalue[l] = 0.0;
                    }
                }
            } else {
                for (l = 0; l < wave->num_order * wave->num_channel; l++) {
                    k = buf_pos + l;
                    if (swIsWaveFloat(wave) == SP_TRUE) {
                        ddata[*data_pos] = dbuf[k];
                    } else if (swIsWaveLong(wave) == SP_TRUE) {
                        ldata[*data_pos] = lbuf[k];
                    } else {
                        sdata[*data_pos] = sbuf[k];
                    }
                    (*data_pos)++;
                }
            }
		
            if (*data_pos >= data_length) {
                nread = 0;
                break;
            }
        }
    }

    if (nread > 0) {
	*read_pos += nread;
    }

    swUnlockMutex(wave);
    
    spDebug(100, "calculatePeakData", "nread = %ld\n", nread);
    
    return nread;
}

static FILE *getPeakFile(swWave wave)
{
    FILE *peak_fp = NULL;
    char filename[SP_MAX_PATHNAME];
    
    swLockMutex(wave);
    
    swGetTempFile(wave->config, SW_PEAK_FILE_SUFFIX, filename);
    spDebug(10, "getPeakFile", "create new peak: %s\n", filename);
	    
    if ((peak_fp = spOpenFile(filename, "wb")) != NULL) {
	swRemovePeakFile(wave);
	wave->peak_filename = strclone(filename);

	/*wave->peak_thin_length = 1 + wave->total_length / wave->config->max_read_length;*/
	wave->peak_thin_length = getInitThinLength(wave, wave->total_length);
	wave->peak_thin_length = MIN(wave->peak_thin_length, SW_DEFAULT_PEAK_THIN_LENGTH);
	spDebug(10, "getPeakFile", "peak_thin_length = %ld, total_length = %ld\n",
		wave->peak_thin_length, wave->total_length);
    }

    swUnlockMutex(wave);
    
    return peak_fp;
}

static spLong readData(spPlugin *plugin, swWave wave, spLong read_length,
		     spBool get_range, spBool callback_flag, spBool in_thread)
{
    spBool flag = SP_TRUE;
    spBool make_peak = SP_FALSE;
    spLong nread;
    spLong data_pos;
    spLong prev_call_pos;
    spLong read_pos;
    spLong data_length;
    spLong buf_length;
    spBool play_flag, prev_process_flag;
    FILE *peak_fp = NULL;
    
    if (wave == NULL || plugin == NULL || read_length <= 0) return -1;

    spLockMutex(wave->core->io_mutex);

    buf_length = MIN(swConvertByteToLength(wave, wave->readbuf_size), read_length);
    data_length = swConvertLengthToBufferLength(wave, read_length);
    spDebug(10, "readData", "buf_length = %ld, data_length = %ld, callback_flag = %d, samp_rate = %f\n",
	    buf_length, data_length, callback_flag, wave->samp_rate);

    if (get_range == SP_TRUE
	|| swIsWaveRangeAvailable(wave) == SP_FALSE) {
	get_range = SP_TRUE;
	swInitWaveRange(wave);
	
	if ((peak_fp = getPeakFile(wave)) != NULL) {
	    make_peak = SP_TRUE;
	}
    }

    if (callback_flag == SP_TRUE && wave->config->read_func != NULL) {
	flag = wave->config->read_func(wave, in_thread, SW_PROCESS_STARTED, wave->core->call_data);
    }
    
    data_pos = 0;
    prev_call_pos = 0;
    read_pos = 0;
    
    if (flag == SP_TRUE) {
	if (wave->thin_length > 1) {
	    swInitRangeBuffer(wave);
	}
	if (make_peak == SP_TRUE && wave->peak_thin_length > 1) {
	    swInitPeakRangeBuffer(wave);
	}

	prev_process_flag = wave->core->process_flag;
	play_flag = wave->core->play_flag;
	
	while (1) {
	    if ((nread = swLoadIntoBuffer(plugin, wave, -1, buf_length, wave->readbuf)) <= 0) {
		break;
	    }

	    nread = calculatePeakData(wave, wave->data, &data_pos, data_length,
				      &read_pos, wave->readbuf,
				      swConvertLengthToBufferLength(wave, nread),
				      peak_fp, get_range);

	    if (nread <= 0) {
		break;
	    }
	    if (play_flag == SP_FALSE && prev_process_flag != wave->core->process_flag) {
		data_pos = 0;
		break;
	    }

	    spDebug(80, "readData", "data_pos = %ld, prev_call_pos = %ld\n", data_pos, prev_call_pos);
	    if (callback_flag == SP_TRUE && wave->config->read_func != NULL
		&& wave->thin_length * (data_pos - prev_call_pos) >= wave->config->read_callback_length) {
		flag = wave->config->read_func(wave, in_thread,
					       swConvertBufferLengthToLength(wave, data_pos), wave->core->call_data);
		prev_call_pos = data_pos;
		if (flag == SP_FALSE) {
		    break;
		}
	    }
	}
    }

    spUnlockMutex(wave->core->io_mutex);
    
    if (make_peak == SP_TRUE) {
	swLockMutex(wave);
	spCloseFile(peak_fp);

	spDebug(10, "readData", "wave->peak_length = %ld\n", wave->peak_length);
	if (wave->peak_length <= 0) {
	    swRemovePeakFile(wave);
	} else {
	    wave->peak_length = swConvertBufferLengthToLength(wave, wave->peak_length);
	}
	swUnlockMutex(wave);
    }
    
    if (data_pos > 0 && get_range == SP_TRUE) {
	swLockMutex(wave);
	wave->range_available = SP_TRUE;
	swUnlockMutex(wave);
    }
    
    if (make_peak == SP_TRUE && wave->peak_length > 0) {
	readPeakBuffer(wave, in_thread);
    }
    
    if (callback_flag == SP_TRUE && wave->config->read_func != NULL) {
	flag = wave->config->read_func(wave, in_thread, SW_PROCESS_FINISHED, wave->core->call_data);
    }
    spDebug(10, "readData", "data_pos = %ld\n", data_pos);
    
    return swConvertBufferLengthToLength(wave, data_pos);
}

static spLong readThinWaveDirectly(swWave wave, spLong offset, spLong thin_length, spLong read_length,
				 spBool get_range, spBool callback_flag, spBool in_thread)
{
    spLong ndata;
    spPlugin *plugin;
    
    if (wave == NULL || read_length <= 0 || offset < 0) return -1;

    spDebug(20, "readThinWaveDirectly", "offset = %ld, read_length = %ld\n", offset, read_length);
    
    if ((plugin = swOpenWave(wave, "r")) == NULL) {
	spDebug(20, "readThinWaveDirectly", "open error\n");
	return -1;
    }
    spDebug(20, "readThinWaveDirectly", "open done\n");

    swAllocData(wave, 256 * (((read_length - 1) / 256) + 1));

    spSeekPlugin(plugin, offset * wave->num_order);
    spDebug(20, "readThinWaveDirectly", "seek done, offset = %ld, num_order = %ld\n", offset, wave->num_order);
    
    if ((ndata = readData(plugin, wave, read_length, get_range, callback_flag, in_thread)) > 0) {
	swLockMutex(wave);
	wave->length = ndata;
	swUnlockMutex(wave);
    }

    spDebug(10, "readThinWaveDirectly", "thin_length = %ld, ndata = %ld\n",
	    thin_length, ndata);
    
    spCloseFilePlugin(plugin);

    return ndata;
}

static spLong readThinWaveMain(swWave wave, spLong offset, spLong length, spLong thin_length, 
			     spBool get_range, spBool callback_flag, spBool in_thread)
{
    spLong read_length;

    spDebug(80, "readThinWaveMain", "offset = %ld, length = %ld, thin_length = %ld, wave->total_length = %ld\n",
	    offset, length, thin_length, wave->total_length);
    
    length = MIN(length, wave->total_length - offset);
    thin_length = getThinLength(wave, length, thin_length, &read_length);
    spDebug(80, "readThinWaveMain", "length = %ld, thin_length = %ld, read_length = %ld\n",
	    length, thin_length, read_length);
    
    if (swIsWaveThreadSafe(wave) == SP_FALSE
	&& swIsWaveProcessing(wave) == SP_TRUE
	&& (swIsWavePeakAvailable(wave) == SP_FALSE
	    || thin_length < MIN(wave->peak_thin_length, wave->peak_buf_thin_length))) {
	return -1;
    }
    
    swLockMutex(wave);
    wave->thin_length = thin_length;
    wave->offset = offset;
    wave->length = read_length;
    wave->data_length = length;
    swUnlockMutex(wave);

    spDebug(80, "readThinWaveMain", "wave->thin_length = %ld, wave->peak_thin_length = %ld\n",
	    wave->thin_length, wave->peak_thin_length);
    
    if (wave->thin_length >= 1 && wave->thin_length >= wave->peak_thin_length
	&& swIsWaveRangeAvailable(wave) == SP_TRUE) {
	return readThinWaveUsingPeak(wave, offset, wave->thin_length, read_length,
				     callback_flag, in_thread);
    } else {
#if 1
	spLong weight;

	if (swIsWaveRangeAvailable(wave) == SP_FALSE) {
	    weight = 1;
	} else {
	    weight = SW_READ_CALLBACK_FACTOR;
	}
	
	if (wave->num_channel * wave->num_order * length >= weight * wave->config->read_callback_length) {
	    callback_flag = SP_TRUE;
	}
#else
	if (swIsWaveRangeAvailable(wave) == SP_FALSE
	    && wave->num_channel * wave->num_order * length >= wave->config->read_callback_length) {
	    callback_flag = SP_TRUE;
	}
#endif
	return readThinWaveDirectly(wave, offset, wave->thin_length, read_length,
				    get_range, callback_flag, in_thread);
    }
}

spBool swIsWaveInstantiatable(swWave wave)
{
    spBool flag = SP_TRUE;
    
    if (wave == NULL || swIsWaveProcessing(wave) == SP_TRUE) {
	return SP_FALSE;
    }
    
    if (swIsWaveThreadSafe(wave) == SP_FALSE) {
	spPlugin *plugin;

	plugin = spLoadPlugin(wave->plugin_name);
	
	flag = spIsPluginInstantiatable(plugin);
	spDebug(80, "swIsWaveInstantiatable", "flag = %d\n", flag);
	
	spFreePlugin(plugin);
    }

    return flag;
}

static spLong readThinWave(swWave wave, spLong offset, spLong length, spLong thin_length, 
			 spBool get_range, spBool callback_flag, spBool in_thread)
{
    if (wave == NULL) return -1;

    spDebug(80, "readThinWave", "offset = %ld, length = %ld, total_length = %ld, callback_flag = %d\n",
	    offset, length, wave->total_length, callback_flag);
    
    if (swIsWaveRangeAvailable(wave) == SP_FALSE
	&& (offset != 0 || length != wave->total_length)) {
	/* create peak */
	spDebug(80, "readThinWave", "read just for creating peak file\n");
	if (readThinWaveMain(wave, 0, wave->total_length, 0, 
			     SP_TRUE, SP_FALSE, in_thread) > 0
	    && wave->peak_length > 0) {
	    get_range = SP_FALSE;
	}
    }

    if (thin_length <= 0 && swIsWaveInstantiatable(wave) == SP_FALSE) {
	callback_flag = SP_FALSE;
	thin_length = wave->peak_buf_thin_length;
    }
    spDebug(80, "readThinWave", "thin_length = %ld, wave->peak_buf_thin_length = %ld\n",
	    thin_length, wave->peak_buf_thin_length);
    
    return readThinWaveMain(wave, offset, length, thin_length, 
			    get_range, callback_flag, in_thread);
}

spLong swReadWave(swWave wave, spBool in_thread, spLong offset, spLong length)
{
    return readThinWave(wave, offset, length, 0, SP_FALSE, SP_FALSE, in_thread);
}

spLong swReadTotalWave(swWave wave, spBool in_thread)
{
    if (wave == NULL) return -1;

    return readThinWave(wave, 0, wave->total_length, 0, SP_TRUE, SP_TRUE, in_thread);
}

spLong swReadBuffer(spPlugin *plugin, swWave wave, int channel, spLong length)
{
    spLong nread;
    
    if (wave == NULL || plugin == NULL || length <= 0) return -1;

    spDebug(80, "swReadBuffer", "in: length = %ld, num_order = %ld\n",
	    length, wave->num_order);
    
    swAllocBuffer(wave, length);

    nread = swLoadIntoBuffer(plugin, wave, channel, length, wave->buf);

    if (nread > 0) {
	wave->buf_length = nread;
    } else {
	wave->buf_length = 0;
    }

    spDebug(80, "swReadBuffer", "done: nread = %ld\n", nread);
    
    return nread;
}

spLong swWriteData(spPlugin *plugin, swWave wave)
{
    spLong nwrite;
    
    if (wave == NULL || plugin == NULL) return -1;

    nwrite = spWritePlugin(plugin, wave->data, (long)swConvertLengthToBufferLength(wave, wave->length));

    spDebug(10, "swWriteData", "wave->length = %ld, nwrite = %ld\n", wave->length, nwrite);

    if (nwrite > 0) {
	nwrite = swConvertBufferLengthToLength(wave, nwrite);
    }

    return nwrite;
}

spLong swWriteBuffer(spPlugin *plugin, swWave wave, spLong offset, spLong length)
{
    spLong nwrite;
    
    if (wave == NULL || plugin == NULL) return -1;

    nwrite = spWritePlugin(plugin,
			   wave->buf + swConvertLengthToByte(wave, offset),
			   (long)swConvertLengthToBufferLength(wave, length));

    spDebug(10, "swWriteBuffer", "wave->length = %ld, nwrite = %ld\n", wave->length, nwrite);

    if (nwrite > 0) {
	nwrite = swConvertBufferLengthToLength(wave, nwrite);
    }

    return nwrite;
}

static spBool swSetEditBuffer(spPlugin *plugin, swWave wave, long channel, long order, spLong pos,
			      double value, spBool *overflow)
{
    int o_samp_bit;
    spLong offset;
    
    if (wave == NULL) {
	return SP_FALSE;
    }

    if (channel >= 0) {
	offset = swConvertLengthToBufferLength(wave, pos) + (wave->num_order * channel) + order;
    } else {
	offset = pos;
    }

    if (offset >= wave->editbuf_length) {
	return SP_FALSE;
    }

    if (spGetPluginSampleBit(plugin, &o_samp_bit) == SP_FALSE) {
	o_samp_bit = wave->samp_bit;
    }

    if (o_samp_bit > 32) {
	double *buf = (double *)wave->editbuf;
	buf[offset] = value;
    } else if (o_samp_bit > 16) {
	long *buf = (long *)wave->editbuf;
	buf[offset] = (long)swGetClippedValue(o_samp_bit, value, overflow);
    } else {
	short *buf = (short *)wave->editbuf;
	buf[offset] = (short)swGetClippedValue(o_samp_bit, value, overflow);
    }
    
    return SP_TRUE;
}

spLong swWriteEditBuffer(spPlugin *plugin, swWave wave, spLong length, spBool single_channel)
{
    int num_channel;
    spLong nwrite;
    
    if (wave == NULL || plugin == NULL) return -1;

    if (single_channel == SP_TRUE) {
	num_channel = 1;
    } else {
	num_channel = wave->num_channel;
    }
    
    nwrite = spWritePlugin(plugin, wave->editbuf, (long)(length * num_channel * wave->num_order));
    
    if (nwrite > 0) {
	nwrite = nwrite / num_channel / wave->num_order;
    }
    
    return nwrite;
}

static spPlugin *openWaveToWrite(swWave wave, spWaveInfo *wave_info, char *filename,
				 char *o_plugin_name, char *file_type, char *file_desc,
				 int samp_bit, int num_channel, double samp_rate,
				 spLong length, spBool orig_flag, spBool callback_flag,
				 spBool in_thread)
{
    spPlugin *o_plugin;
    spSongInfoV2 song_info;
    
    spDebug(50, "openWaveToWrite", "filename = %s, orig_flag = %d, callback_flag = %d\n",
	    filename, orig_flag, callback_flag);
    
    /* initialize option */
    spInitWaveInfo(wave_info);
    if (file_type != NULL) {
	spDebug(50, "openWaveToWrite", "copy file_type = %s\n", file_type);
	spStrCopy(wave_info->file_type, SP_WAVE_FILE_TYPE_SIZE, file_type);
    } else if (file_desc != NULL) {
	spDebug(50, "openWaveToWrite", "copy file_desc = %s\n", file_desc);
	spStrCopy(wave_info->file_desc, SP_WAVE_FILE_DESC_SIZE, file_desc);
    } else {
	char *i_plugin_name;
	
	if ((i_plugin_name = xspFindRelatedPluginFile(o_plugin_name)) != NULL) {
	    if (streq(i_plugin_name, wave->core->orig_plugin_name)) {
		if (orig_flag == SP_TRUE && wave->core->orig_file_type != NULL) {
		    spDebug(50, "openWaveToWrite", "copy wave->core->orig_file_type = %s\n", wave->core->orig_file_type);
		    spStrCopy(wave_info->file_type, SP_WAVE_FILE_TYPE_SIZE, wave->core->orig_file_type);
		} else if (wave->file_type != NULL) {
		    spDebug(50, "openWaveToWrite", "copy wave->file_type = %s\n", wave->file_type);
		    spStrCopy(wave_info->file_type, SP_WAVE_FILE_TYPE_SIZE, wave->file_type);
		}
	    }
	    xfree(i_plugin_name);
	}
    }
    spDebug(50, "openWaveToWrite", "wave_info->file_type = %s, wave_info->file_desc = %s\n",
	    wave_info->file_type, wave_info->file_desc);
    
    if (samp_bit <= 0) {
	samp_bit = wave->samp_bit;
    }
    if (orig_flag == SP_TRUE) {
	wave_info->samp_bit = samp_bit;
    } else {
	wave_info->samp_bit = MAX(samp_bit, 16);
    }
    if (num_channel >= 1) {
	wave_info->num_channel = num_channel;
    } else {
	wave_info->num_channel = wave->num_channel;
    }
    if (samp_rate > 0.0) {
	wave_info->samp_rate = samp_rate;
    } else {
	wave_info->samp_rate = wave->samp_rate;
    }
    if (length > 0) {
	wave_info->length = length;
    } else {
	wave_info->length = wave->total_length;
    }
    spCopySongInfoV2(&song_info, &wave->song_info);

    spDebug(10, "openWaveToWrite", "wave->samp_bit = %d, wave_info->samp_bit = %d, song_info.info_mask = %lx\n",
	    wave->samp_bit, wave_info->samp_bit, song_info.info_mask);
    
    /* open output file */
    if (orig_flag == SP_TRUE && callback_flag == SP_TRUE) {
	swCallbackData call_data = {NULL, NULL, SP_FALSE, NULL};

	call_data.config = wave->config;
	call_data.wave = wave;
	call_data.in_thread = in_thread;
	call_data.call_data = wave->core->call_data;
	
	o_plugin = spOpenFilePlugin(o_plugin_name, filename, "w",
				    SP_PLUGIN_DEVICE_FILE,
				    wave_info, (spSongInfo *)&song_info,
				    (spPluginOpenCallback)pluginOpenCB, (void *)&call_data, NULL);
    } else {
	o_plugin = spOpenFilePlugin(o_plugin_name, filename, "w",
				    SP_PLUGIN_DEVICE_FILE,
				    wave_info, (spSongInfo *)&song_info, NULL, NULL, NULL);
    }

    return o_plugin;
}

static double getBitConvWeight(swWave wave, int o_samp_bit)
{
    double weight = 1.0;
    
    if (wave->samp_bit > 32 || o_samp_bit > 32) {
	if (wave->config->float_normalized == SP_TRUE
	    && (wave->samp_bit <= 32 || o_samp_bit <= 32)) {
	    if (o_samp_bit <= 32) {
		weight = swGetClipValue(o_samp_bit);
	    } else if (wave->samp_bit <= 32) {
		weight = 1.0 / swGetClipValue(wave->samp_bit);
	    }
	}
    } else if (wave->samp_bit <= 16) {
	if (o_samp_bit <= 16) {
	    weight = 1.0;
	} else if (o_samp_bit <= 24) {
	    weight = 256.0;
	} else if (o_samp_bit <= 32) {
	    weight = 65536.0;
	}
    } else if (wave->samp_bit <= 24) {
	if (o_samp_bit <= 16) {
	    weight = 1.0 / 256.0;
	} else if (o_samp_bit <= 24) {
	    weight = 1.0;
	} else if (o_samp_bit <= 32) {
	    weight = 256.0;
	}
    } else if (wave->samp_bit <= 32) {
	if (o_samp_bit <= 16) {
	    weight = 1.0 / 65536.0;
	} else if (o_samp_bit <= 24) {
	    weight = 1.0 / 256.0;
	} else if (o_samp_bit <= 32) {
	    weight = 1.0;
	}
    }

    return weight;
}

#if 1
static spLong generateWaveform(spPlugin *o_plugin, swWave wave, 
			     swEditType edit_type, spLong delay, spLong duration,
			     double f0, double initial_phase, double gain, 
			     spBool *overflow, spBool callback_flag, spBool in_thread)
{
    spLong k, n;
    long l;
    spLong len;
    spLong total_len;
    spLong nread;
    spLong nwrite;
    long nharmonics;
    long sign;
    spLong data_pos;
    spLong data_length;
    spLong read_pos;
    double weight;
    double theta;
    double value;
    FILE *peak_fp;
    swAudio audio = NULL;
    spBool audio_duplex_flag = SP_FALSE;
    spBool make_peak = SP_FALSE;
    spBool in_record_pause = SP_FALSE;
    spBool prev_in_record_pause = SP_FALSE;
    
    spDebug(50, "generateWaveform", "callback_flag = %d, in_thread = %d\n", callback_flag, in_thread);

    if (edit_type == SW_EDIT_GENERATE_RECORDING) {
	if (swInitAudio(wave->config) == SP_FALSE) {
            spDebug(10, "generateWaveform", "swInitAudio failed\n");
            wave->core->last_error = SW_ERROR_AUDIO_DEVICE_ERROR;
	    return 0;
	}
#if defined(SW_USE_THREAD)
        /* pause for recording */
        if (edit_type == SW_EDIT_GENERATE_RECORDING && in_thread == SP_TRUE
            && wave->config->record_use_pause == SP_TRUE) {
            wave->core->in_record_pause = in_record_pause = prev_in_record_pause = SP_TRUE;
#if 0
            swWaitEvent(wave);
            wave->core->in_record_pause = SP_FALSE;
#endif
        }
#endif
	if ((audio = swBeginAudio(wave, in_thread)) == NULL
	    || spOpenAudioDevice(audio->audio, "r") == SP_FALSE) {
            spDebug(10, "generateWaveform", "swBeginAudio or spOpenAudioDevice failed\n");
            wave->core->last_error = SW_ERROR_AUDIO_DEVICE_ERROR;
	    return 0;
	}
	spDebug(50, "generateWaveform", "spOpenAudioDevice with read mode done\n");
	if (spOpenAudioDevice(audio->audio, "w") == SP_TRUE) {
	    spDebug(50, "generateWaveform", "spOpenAudioDevice with write mode done\n");
	    audio_duplex_flag = SP_TRUE;
	}
    }
	
    swInitWaveRange(wave);
    if ((peak_fp = getPeakFile(wave)) != NULL) {
	make_peak = SP_TRUE;
    }
    
    swLockMutex(wave);
    
    spDebug(50, "generateWaveform", "before getThinLength: wave->thin_length = %ld, wave->length = %ld\n",
	    wave->thin_length, wave->length);
    wave->thin_length = getThinLength(wave, wave->total_length, 0, &wave->length);
    spDebug(50, "generateWaveform", "after getThinLength: wave->thin_length = %ld, wave->length = %ld\n",
	    wave->thin_length, wave->length);
    if (wave->thin_length > 1) {
	swInitRangeBuffer(wave);
    }
    if (make_peak == SP_TRUE && wave->peak_thin_length > 1) {
	swInitPeakRangeBuffer(wave);
    }
    
    wave->core->process_finished = SP_FALSE;
    swUnlockMutex(wave);
    
    if (callback_flag == SP_TRUE && wave->config->edit_func != NULL) {
	spDebug(50, "generateWaveform", "call edit_func (SW_PROCESS_STARTED)\n");
	wave->config->edit_func(wave, in_thread, SW_PROCESS_STARTED, edit_type, wave->core->call_data);
	spDebug(50, "generateWaveform", "call edit_func (pos = 0)\n");
	wave->config->edit_func(wave, in_thread, 0, edit_type, wave->core->call_data);
    }
    
    if (duration <= 0) duration = wave->total_length;
	
    total_len = 0;
    len = MIN(wave->config->normal_read_length, wave->total_length);
    swAllocEditBuffer(wave, len);

    swAllocData(wave, 256 * (((wave->length - 1) / 256) + 1));
    
    weight = swGetLimitValue(wave->samp_bit);
#if 0
    if (gain != 0.0) {
	weight *= pow(10.0, (gain / 20.0));
    }
#else
    if (gain != 1.0) {
	weight *= gain;
    }
#endif

    nharmonics = (long)((wave->samp_rate / 2.0) / f0);
    if (edit_type == SW_EDIT_GENERATE_SQUARE) {
	weight *= (4.0 / PI);
    } else if (edit_type == SW_EDIT_GENERATE_TRIANGLE) {
	weight *= (8.0 / pow(PI, 2.0));
    } else if (edit_type == SW_EDIT_GENERATE_SAWTOOTH) {
	weight *= (-2.0 / PI);
    }

    data_length = swConvertLengthToBufferLength(wave, wave->length);
    data_pos = 0;
    read_pos = 0;
    
    while (1) {
#ifdef SW_USE_THREAD
        in_record_pause = wave->core->in_record_pause;
        /* pause for recording */
        if (edit_type == SW_EDIT_GENERATE_RECORDING && in_thread == SP_TRUE && in_record_pause == SP_TRUE) {
            /* do nothing */
        } else
#endif
	if (total_len + len > wave->total_length) {
	    len = wave->total_length - total_len;
	}
        
        nread = 0;
	if (edit_type == SW_EDIT_GENERATE_RECORDING) {
	    nread = spReadAudio(audio->audio, wave->editbuf, (long)(len * wave->num_channel));
	    if (nread > 0) {
		len = nread / wave->num_channel;
	    } else {
		len = 0;
	    }
	} else {
	    for (k = 0; k < len; k++) {
		if (total_len + k < delay
		    || total_len + k >= duration) {
		    value = 0.0;
		} else {
		    if (edit_type == SW_EDIT_GENERATE_SILENCE) {
			value = 0.0;
		    } else if (edit_type == SW_EDIT_GENERATE_WHITE_NOISE) {
			value = weight * randn();
		    } else {
			theta = initial_phase +  2.0 * PI * (f0 * (double)(total_len + k - delay) / wave->samp_rate);
			if (edit_type == SW_EDIT_GENERATE_SQUARE) {
			    value = 0.0;
			    for (n = 1; n <= nharmonics; n += 2) {
				value += sin((double)n * theta) / (double)n;
			    }
			    value *= weight;
			} else if (edit_type == SW_EDIT_GENERATE_TRIANGLE) {
			    value = 0.0;
			    sign = 1;
			    for (n = 1; n <= nharmonics; n += 2) {
				value += (sin((double)n * theta) / (double)(sign * (n * n)));
				sign *= -1;
			    }
			    value *= weight;
			} else if (edit_type == SW_EDIT_GENERATE_SAWTOOTH) {
			    value = 0.0;
			    for (n = 1; n <= nharmonics; n++) {
				value += sin((double)n * theta) / (double)n;
			    }
			    value *= weight;
			} else {
			    value = weight * sin(theta);
			}
		    }
		}
	    
		for (l = 0; l < wave->num_channel; l++) {
		    swSetEditBuffer(o_plugin, wave, l, 0, k, value, overflow);
		}
	    }
	}

#ifdef SW_USE_THREAD
        /* pause for recording */
        if (edit_type == SW_EDIT_GENERATE_RECORDING && in_thread == SP_TRUE && in_record_pause == SP_TRUE) {
            calculatePeakData(wave, NULL, NULL, 0,
                              &read_pos, wave->editbuf,
                              swConvertLengthToBufferLength(wave, len),
                              NULL, SP_FALSE);
        } else
#endif
        {
            if (len <= 0) {
                nwrite = 0;
            } else {
                nwrite = swWriteEditBuffer(o_plugin, wave, len, SP_FALSE);
            }
	
            if (nwrite <= 0) {
                total_len = -1;
                break;
            }
	
            calculatePeakData(wave, wave->data, &data_pos, data_length,
                              &read_pos, wave->editbuf,
                              swConvertLengthToBufferLength(wave, len),
                              peak_fp, SP_TRUE);
            spDebug(10, "generateWaveform", "data_pos = %ld, read_pos = %ld\n",
                    data_pos, read_pos);
	
            total_len += len;

            if (total_len >= wave->total_length) {
                break;
            }
        }
	
        if (audio_duplex_flag == SP_TRUE && nread > 0 && wave->core->record_monitor_flag == SP_TRUE) {
            spWriteAudio(audio->audio, wave->editbuf, (long)nread);
        }
        
        if (callback_flag == SP_TRUE && wave->config->edit_func != NULL) {
            spDebug(50, "generateWaveform", "call edit_func (pos = total_len)\n", (long)total_len);
            wave->config->edit_func(wave, in_thread, total_len, edit_type, wave->core->call_data);
        }

	if (callback_flag == SP_TRUE && wave->core->process_flag == SP_FALSE) {
	    break;
	}
        prev_in_record_pause = in_record_pause;
    }

    if (peak_fp != NULL) {
	spCloseFile(peak_fp);
    }
    if (edit_type == SW_EDIT_GENERATE_RECORDING) {
	spCloseAudioDevice(audio->audio);
	swEndAudio(audio);
    }
    
    swLockMutex(wave);
    wave->range_available = SP_TRUE;

#if 0
    if (callback_flag == SP_TRUE && wave->core->process_flag == SP_FALSE) {
	total_len = 0;
    }
#else
    if (total_len > 0) {
	wave->core->process_flag = SP_TRUE;
    }
#endif
    swUnlockMutex(wave);

    if (total_len > 0 && make_peak == SP_TRUE && wave->peak_length > 0) {
	readPeakBuffer(wave, in_thread);
    }
    
    spDebug(10, "generateWaveform", "data_pos = %ld, total_len = %ld, process_flag = %d\n",
	    data_pos, total_len, wave->core->process_flag);
    
    return total_len;
}
#endif

static spLong cropWaveRegion(spPlugin *i_plugin, spPlugin *o_plugin, swWave wave, spWaveInfo *wave_info,
                             swEditType edit_type, spLong offset, spLong length,
                             spBool *overflow, spBool callback_flag, spBool in_thread)
{
    spLong k;
    long l, m;
    spLong len;
    spLong total_len;
    spLong nread, nwrite;
    double value;
    double weight = 1.0;
    
    spDebug(50, "cropWaveRegion", "callback_flag = %d\n", callback_flag);
    
    swLockMutex(wave);
    wave->core->process_finished = SP_FALSE;
    swUnlockMutex(wave);
    
    if (callback_flag == SP_TRUE && wave->config->edit_func != NULL) {
	wave->config->edit_func(wave, in_thread, SW_PROCESS_STARTED, edit_type, wave->core->call_data);
	wave->config->edit_func(wave, in_thread, offset, edit_type, wave->core->call_data);
    }
    
    if (edit_type == SW_EDIT_BIT_CONV) {
	int o_samp_bit;
	
	o_samp_bit = wave->samp_bit;
	spGetPluginSampleBit(o_plugin, &o_samp_bit);
	
	weight = getBitConvWeight(wave, o_samp_bit);
    }
    
    spSeekPlugin(i_plugin, offset * wave->num_order);
    
    if (length <= 0) length = wave->total_length;
	
    total_len = 0;
    len = MIN(wave->config->normal_read_length, wave->total_length - offset);
    swAllocEditBuffer(wave, len);
    
    while ((nread = swReadBuffer(i_plugin, wave, -1, len)) > 0) {
	if (total_len + nread > length) {
	    nread = length - total_len;
	}
	if (edit_type == SW_EDIT_BIT_CONV) {
	    for (k = 0; k < nread; k++) {
		for (l = 0; l < wave->num_channel; l++) {
		    for (m = 0; m < wave->num_order; m++) {
			value = swGetBufferData(wave, l, m, k);
			swSetEditBuffer(o_plugin, wave, l, m, k, weight * value, overflow);
		    }
		}
	    }
	    nwrite = swWriteEditBuffer(o_plugin, wave, nread, SP_FALSE);
	} else if (edit_type == SW_EDIT_MONAURALIZE) {
	    for (k = 0; k < nread; k++) {
		value = 0.0;
		for (m = 0; m < wave->num_order; m++) {
		    for (l = 0; l < wave->num_channel; l++) {
			value += swGetBufferData(wave, l, m, k);
		    }
		    value /= (double)wave->num_channel;
		    swSetEditBuffer(o_plugin, wave, -1, m, k, value, overflow);
		}
	    }
	    nwrite = swWriteEditBuffer(o_plugin, wave, nread, SP_TRUE);
	} else if (edit_type != SW_EDIT_COPY && wave->selected_channel >= 0) {
	    for (k = 0; k < nread; k++) {
		for (m = 0; m < wave->num_order; m++) {
		    value = swGetBufferData(wave, wave->selected_channel, m, k);
		    swSetEditBuffer(o_plugin, wave, -1, m, k, value, overflow);
		}
	    }
	    nwrite = swWriteEditBuffer(o_plugin, wave, nread, SP_TRUE);
	} else {
	    nwrite = swWriteBuffer(o_plugin, wave, 0, nread);
	}
	if (nwrite <= 0) {
	    total_len = -1;
	    break;
	}
	
	total_len += nread;

	if (total_len >= length) {
	    break;
	}
	
	if (callback_flag == SP_TRUE && wave->config->edit_func != NULL) {
	    wave->config->edit_func(wave, in_thread, offset + total_len, edit_type, wave->core->call_data);
	}
	if (callback_flag == SP_TRUE && wave->core->process_flag == SP_FALSE) {
	    break;
	}
    }

    swLockMutex(wave);
    if (callback_flag == SP_TRUE && wave->core->process_flag == SP_FALSE) {
	total_len = 0;
    }
    swUnlockMutex(wave);

    spDebug(10, "cropWaveRegion", "total_len = %ld, process_flag = %d\n",
	    total_len, wave->core->process_flag);
    
    return total_len;
}

static spLong editWaveRegion(spPlugin *i_plugin, spPlugin *o_plugin, swWave wave, spWaveInfo *wave_info,
                             swEditType edit_type, spLong offset, spLong length,
                             double value, spBool *overflow, spBool in_thread)
{
    int target_channel = 0;
    long j, m;
    spLong i, k, n;
    spLong len;
    spLong total_len;
    spLong nread;
    spLong edit_count;
    double edit_value;
    int flag1 = 0, flag2 = 0;

    swLockMutex(wave);
    wave->core->process_finished = SP_FALSE;
    swUnlockMutex(wave);
    
    if (wave->config->edit_func != NULL) {
	wave->config->edit_func(wave, in_thread, SW_PROCESS_STARTED, edit_type, wave->core->call_data);
	wave->config->edit_func(wave, in_thread, 0, edit_type, wave->core->call_data);
    }

    if (edit_type == SW_EDIT_CHANGE_VALUE) {
	target_channel = MAX((int)length, 0);
	target_channel = MIN(target_channel, wave->num_channel - 1);
	length = 1;
    } else {
	if (length <= 0) length = wave->total_length;
    }
    
    edit_count = 0;
    total_len = 0;
    len = MIN(wave->config->normal_read_length, wave->total_length);
    swAllocEditBuffer(wave, len);
    
    while ((nread = swReadBuffer(i_plugin, wave, -1, len)) > 0) {
	if (total_len + nread >= offset && total_len < offset + length) {
	    if (!flag1) {
		/* before the region */
		n = offset - total_len;
		if (n > 0) {
		    if (swWriteBuffer(o_plugin, wave, 0, n) <= 0) {
			total_len = -1;
			break;
		    }
		}
	    } else {
		n = 0;
	    }
	    flag1 = 1;

	    if (edit_type != SW_EDIT_DELETE) {
		k = 0;
		for (i = n; i < nread; i++) {
		    if (total_len + i >= offset + length) {
			break;
		    }
			    
		    if (edit_type == SW_EDIT_CHANNEL_SWAP) {
			for (j = 0; j < wave_info->num_channel; j++) {
			    for (m = 0; m < wave->num_order; m++) {
				swSetEditBuffer(o_plugin, wave, j, m, k,
						swGetBufferData(wave, wave_info->num_channel - 1 - j, m, i),
						overflow);
			    }
			}
		    } else if (edit_type == SW_EDIT_CHANGE_VALUE) {
			for (j = 0; j < wave_info->num_channel; j++) {
			    if (j == target_channel) {
				edit_value = value;
			    } else {
				edit_value = swGetBufferData(wave, j, 0, i);
			    }
			    swSetEditBuffer(o_plugin, wave, j, 0, k, edit_value, overflow);
			}
		    } else {
			for (j = 0; j < wave_info->num_channel; j++) {
			    for (m = 0; m < wave->num_order; m++) {
				if (wave->selected_channel < 0 || j == wave->selected_channel) {
				    if (edit_type == SW_EDIT_AMPLIFY) {
					edit_value = value * swGetBufferData(wave, j, m, i);
				    } else if (edit_type == SW_EDIT_FADE_IN) {
					edit_value = ((double)edit_count / (double)length)
					    * swGetBufferData(wave, j, m, i);
				    } else if (edit_type == SW_EDIT_FADE_OUT) {
					edit_value = ((double)(length - 1 - edit_count) / (double)length)
					    * swGetBufferData(wave, j, m, i);
				    } else {
					edit_value = 0.0;
				    }
				} else {
				    edit_value = swGetBufferData(wave, j, m, i);
				}
				swSetEditBuffer(o_plugin, wave, j, m, k, edit_value, overflow);
			    }
			}
		    }
		    k++;
		    edit_count++;
		}
		
		if (k > 0) {
		    if (swWriteEditBuffer(o_plugin, wave, k, SP_FALSE) <= 0) {
			total_len = -1;
			break;
		    }
		}
	    }
		
	    if (!flag2) {
		/* after the region */
		k = offset + length - total_len;
		n = total_len + nread - (offset + length);
		if (k >= 0 && n > 0) {
		    if (swWriteBuffer(o_plugin, wave, k, n) <= 0) {
			total_len = -1;
			break;
		    }
		    flag2 = 1;
		}
	    }
	} else {
	    /* before & after the region */
	    if (swWriteBuffer(o_plugin, wave, 0, nread) <= 0) {
		total_len = -1;
		break;
	    }
	}
	total_len += nread;

	if (wave->config->edit_func != NULL) {
	    wave->config->edit_func(wave, in_thread, total_len, edit_type, wave->core->call_data);
	}
	if (wave->core->process_flag == SP_FALSE) {
	    break;
	}
    }

    swLockMutex(wave);
    if (wave->core->process_flag == SP_FALSE) {
	total_len = 0;
    }
    swUnlockMutex(wave);
    
    return total_len;
}

static spLong pasteWave(spPlugin *i_plugin, spPlugin *o_plugin, swWave src_wave, swWave wave, 
		      spWaveInfo *wave_info, swEditType edit_type, spLong offset, spLong length,
		      spBool *overflow, spBool in_thread)
{
    int target_channel;
    long j, l;
    spLong i, k, n;
    spLong len;
    spLong src_len = 0;
    spLong total_len;
    spLong nread, src_nread;
    spLong edit_len;
    double edit_value;
    spPlugin *src_plugin = NULL;
    swAudio audio = NULL;
    spBool audio_duplex_flag = SP_FALSE;
    int flag1 = 0, flag2 = 0;

    if (edit_type == SW_EDIT_INSERT_PAUSE) {
	src_len = length;
    } else if (edit_type == SW_EDIT_RECORD) {
	src_len = length;
	if (swInitAudio(wave->config) == SP_FALSE) {
	    return 0;
	}
	if ((audio = swBeginAudio(wave, in_thread)) == NULL
	    || spOpenAudioDevice(audio->audio, "r") == SP_FALSE) {
	    return 0;
	}
	spDebug(50, "pasteWave", "spOpenAudioDevice with read mode done\n");
	if (spOpenAudioDevice(audio->audio, "w") == SP_TRUE) {
	    spDebug(50, "pasteWave", "spOpenAudioDevice with write mode done\n");
	    audio_duplex_flag = SP_TRUE;
	}
    } else {
	if (swIsWaveNone(src_wave) == SP_TRUE) {
	    return 0;
	}
	
	swLockMutex(src_wave);
	if (swIsWaveProcessing(src_wave) == SP_TRUE
	    && swIsWavePlaying(src_wave) == SP_FALSE) {
	    swUnlockMutex(src_wave);
	    return 0;
	}
	swUnlockMutex(src_wave);
	
	if (src_wave->num_channel != 1 && src_wave->num_channel != wave->num_channel) {
	    return 0;
	}
	if (src_wave->num_order != wave->num_order) {
	    return 0;
	}
	
	if ((src_plugin = swOpenWave(src_wave, "r")) == NULL) {
	    return 0;
	}
    }

    swLockMutex(wave);
    wave->core->process_finished = SP_FALSE;
    swUnlockMutex(wave);

    if (wave->config->edit_func != NULL) {
	wave->config->edit_func(wave, in_thread, SW_PROCESS_STARTED, edit_type, wave->core->call_data);
	wave->config->edit_func(wave, in_thread, 0, edit_type, wave->core->call_data);
    }

    target_channel = MAX((int)length, 0);
    target_channel = MIN(target_channel, wave->num_channel - 1);
    
    if (edit_type == SW_EDIT_INSERT || edit_type == SW_EDIT_INSERT_PAUSE) {
	length = 0;
    } else if (edit_type == SW_EDIT_REPLACE) {
	target_channel = wave->selected_channel;
    } else if (edit_type == SW_EDIT_RECORD) {
	length = MIN(wave->total_length - offset, length);
    } else {
	length = MIN(src_wave->total_length, wave->total_length - offset);
    }
    spDebug(50, "pasteWave", "target_channel = %d, offset = %ld, length = %ld\n",
	    target_channel, offset, length);
    
    total_len = 0;
    edit_len = 0;
    
    len = MIN(wave->config->normal_read_length, wave->total_length);
    if (edit_type != SW_EDIT_INSERT_PAUSE && edit_type != SW_EDIT_RECORD) {
	src_len = MIN(wave->config->normal_read_length, src_wave->total_length);
    }
    swAllocEditBuffer(wave, len);

    while ((nread = swReadBuffer(i_plugin, wave, -1, len)) > 0) {
	spDebug(80, "pasteWave", "nread = %ld, total_len = %ld\n", nread, total_len);
	/* if (total_len + nread >= offset && total_len < offset + length) { */
	if (offset <= total_len + nread && total_len <= offset + length) {
	    if (!flag1) {
		/* before the region */
		n = offset - total_len;
		if (n > 0) {
		    if (swWriteBuffer(o_plugin, wave, 0, n) <= 0) {
			total_len = -1;
			break;
		    }
		}
	    } else {
		n = 0;
	    }
	    flag1 = 1;

	    if (edit_type == SW_EDIT_INSERT || edit_type == SW_EDIT_REPLACE
		|| edit_type == SW_EDIT_INSERT_PAUSE || edit_type == SW_EDIT_RECORD) {
		if (edit_len <= 0) {
		    while (1) {
			if (edit_type == SW_EDIT_INSERT_PAUSE) {
			    src_nread = MIN(wave->config->normal_read_length, src_len);
			    if (src_nread > 0) {
				for (i = 0; i < src_nread; i++) {	
				    for (j = 0; j < wave_info->num_channel; j++) {
					for (l = 0; l < wave->num_order; l++) {
					    swSetEditBuffer(o_plugin, wave, j, l, i, 0.0, overflow);
					}
				    }
				}
				if (swWriteEditBuffer(o_plugin, wave, src_nread, SP_FALSE) <= 0) {
				    total_len = -1;
				}
				edit_len += src_nread;
				src_len -= src_nread;
			    }
			} else if (edit_type == SW_EDIT_RECORD) {
			    if (src_len <= 0) {
				src_nread = 0;
			    } else {
				src_nread = spReadAudio(audio->audio, wave->editbuf,
							(long)(MIN(len, src_len) * wave_info->num_channel));
				src_nread /= wave_info->num_channel;
				spDebug(80, "pasteWave", "read audio: len = %ld, src_nread = %ld\n", len, src_nread);
			    
				if (src_nread > 0) {
				    if (swWriteEditBuffer(o_plugin, wave, src_nread, SP_FALSE) <= 0) {
					total_len = -1;
				    }
				    if (audio_duplex_flag == SP_TRUE) {
					spWriteAudio(audio->audio, wave->editbuf, (long)(src_nread * wave_info->num_channel));
				    }
				    edit_len += src_nread;
				    src_len -= src_nread;
				}
			    }
			} else {
			    swLockMutex(src_wave);
			    if ((src_nread = swReadBuffer(src_plugin, src_wave, -1, src_len)) > 0) {
				if (src_wave->num_channel == wave->num_channel) {
				    if (swWriteBuffer(o_plugin, src_wave, 0, src_nread) <= 0) {
					total_len = -1;
				    }
				} else {
				    for (i = 0; i < src_nread; i++) {
					for (j = 0; j < wave_info->num_channel; j++) {
					    for (l = 0; l < wave->num_order; l++) {
						if (j == target_channel) {
						    edit_value = swGetBufferData(src_wave, 0, l, i);
						} else {
						    edit_value = 0.0;
						}
						swSetEditBuffer(o_plugin, wave, j, l, i, edit_value, overflow);
					    }
					}
				    }
				    if (swWriteEditBuffer(o_plugin, wave, src_nread, SP_FALSE) <= 0) {
					total_len = -1;
				    }
				}
				edit_len += src_nread;
			    }
			    swUnlockMutex(src_wave);
			}
			
			if (total_len < 0 || src_nread <= 0 || wave->core->process_flag == SP_FALSE) {
			    break;
			}
		    
			if (wave->config->edit_func != NULL) {
			    wave->config->edit_func(wave, in_thread, total_len, edit_type, wave->core->call_data);
			}
		    }
		}
	    } else {
		swLockMutex(src_wave);
		if ((src_nread = swReadBuffer(src_plugin, src_wave, -1, nread - n)) > 0) {
		    for (i = 0; i < src_nread; i++) {
			for (j = 0; j < wave_info->num_channel; j++) {
			    for (l = 0; l < wave->num_order; l++) {
				if (src_wave->num_channel == wave->num_channel) {
				    edit_value = swGetBufferData(src_wave, j, l, i);
				} else if (j == target_channel) {
				    edit_value = swGetBufferData(src_wave, 0, l, i);
				} else if (edit_type == SW_EDIT_PASTE) {
				    edit_value = swGetBufferData(wave, j, l, n + i);
				} else {
				    edit_value = 0.0;
				}
				if (edit_type == SW_EDIT_MIX) {
				    edit_value += swGetBufferData(wave, j, l, n + i);
				}
				swSetEditBuffer(o_plugin, wave, j, l, i, edit_value, overflow);
			    }
			}
		    }
		    if (swWriteEditBuffer(o_plugin, wave, src_nread, SP_FALSE) <= 0) {
			total_len = -1;
		    }
		}
		swUnlockMutex(src_wave);
	    }

	    if (total_len < 0) break;
		
	    if (!flag2) {
		/* after the region */
		k = offset + length - total_len;
		n = total_len + nread - (offset + length);
		if (k >= 0 && n > 0) {
		    if (swWriteBuffer(o_plugin, wave, k, n) <= 0) {
			total_len = -1;
			break;
		    }
		    flag2 = 1;
		}
	    }
	} else {
	    /* before & after the region */
	    if (swWriteBuffer(o_plugin, wave, 0, nread) <= 0) {
		total_len = -1;
		break;
	    }
	}
	total_len += nread;

	if (wave->config->edit_func != NULL) {
	    wave->config->edit_func(wave, in_thread, total_len, edit_type, wave->core->call_data);
	}
	if (wave->core->process_flag == SP_FALSE) {
	    break;
	}
    }
    
    if (total_len >= 0 && (edit_type == SW_EDIT_INSERT || edit_type == SW_EDIT_INSERT_PAUSE)) {
	total_len += edit_len;
    }

    if (edit_type == SW_EDIT_RECORD) {
	spCloseAudioDevice(audio->audio);
	swEndAudio(audio);
    } else if (edit_type != SW_EDIT_INSERT_PAUSE) {
	/* close source file */
	spCloseFilePlugin(src_plugin);
    }
    
    swLockMutex(wave);
    if (wave->core->process_flag == SP_FALSE) {
	total_len = 0;
    }
    swUnlockMutex(wave);
    
    spDebug(50, "pasteWave", "done\n");
    
    return total_len;
}

static spLong convSampFreq(spPlugin *i_plugin, spPlugin *o_plugin, swWave wave, spWaveInfo *wave_info,
			 swEditType edit_type, spLong upratio, spLong downratio, spLong buffer_length, double gain,
			 spDVector filter, spBool *overflow,
			 spBool in_thread)
{
    long l;
    spLong k;
    long fftl;
    spLong len;
    spLong total_len;
    spLong obuf_len;
    spLong nread;
    spLong nwrite;
    spLong pos, prev_pos;
    spLong shift;
    spLong syn_pos;
    spLong syn_offset;
    double value;
    spDVector isig;
    spDVector *osigs;
    spDVector upsig;
    spDVector filsig;

    swLockMutex(wave);
    wave->core->process_finished = SP_FALSE;
    swUnlockMutex(wave);

    if (wave->config->edit_func != NULL) {
	wave->config->edit_func(wave, in_thread, SW_PROCESS_STARTED, edit_type, wave->core->call_data);
	wave->config->edit_func(wave, in_thread, 0, edit_type, wave->core->call_data);
    }
    
    spDebug(50, "convSampFreq", "up = %ld, down = %ld\n", upratio, downratio);

    fftl = 1024;
    len = (buffer_length - filter->length + 1) / upratio;
    spDebug(30, "convSampFreq", "len = %ld\n", len);
    len = MAX(len, 128);
    len = MIN(len, wave->total_length);
    len = MIN(len, fftl);
    
    /* allocate memory */
    isig = xdvalloc((long)len);
    osigs = xalloc(wave->num_order * wave->num_channel, spDVector);
    obuf_len = (len * upratio + filter->length - 1) / downratio + 1;
    for (l = 0; l < wave->num_order * wave->num_channel; l++) {
	osigs[l] = xdvzeros(obuf_len);
    }
    swAllocEditBuffer(wave, (len * upratio) / downratio + 1);
    
    pos = (-filter->length / 2);
    syn_offset = 0;
    total_len = 0;
    while ((nread = swReadBuffer(i_plugin, wave, -1, len)) > 0) {
	shift = (spLong)nread * upratio;
	prev_pos = pos;
	pos += shift;

	spDebug(80, "convSampFreq", "nread = %ld, pos = %ld\n", nread, pos);

	nwrite = 0;
	syn_pos = 0;
	for (l = 0; l < wave->num_order * wave->num_channel; l++) {
	    if (in_thread == SP_TRUE) spYieldThread();
	
	    for (k = 0; k < nread; k++) {
		isig->data[k] = swGetBufferData(wave, l / wave->num_order, l % wave->num_order, k);
	    }
	    isig->length = (long)nread;

	    if (upratio != 1) {
		/* up sampling */
		upsig = xdvupsample(isig, (long)upratio);

		if (in_thread == SP_TRUE) spYieldThread();

		/* lowpass filtering */
		filsig = xdvfftfilt(filter, upsig, fftl);
		xdvfree(upsig);
	    } else {
		filsig = xdvfftfilt(filter, isig, fftl);
	    }
	    
	    if (in_thread == SP_TRUE) spYieldThread();
	    
	    spDebug(80, "convSampFreq", "osigs[%ld]->length = %ld, filsig->length = %ld\n",
		    l, osigs[l]->length, filsig->length);

	    if (prev_pos < 0) {
		syn_pos = -prev_pos;
	    } else {
		syn_pos = syn_offset;
	    }
	    for (k = 0; k < osigs[l]->length; k++) {
		if (syn_pos >= filsig->length) break;
		    
		osigs[l]->data[k] += filsig->data[syn_pos];
		syn_pos += downratio;
	    }

	    if (pos > 0) {
		nwrite = 0;
		syn_pos = syn_offset;
		for (k = 0; k < osigs[l]->length; k++) {
		    if (syn_pos >= pos) break;

		    value = gain * osigs[l]->data[k];
		    
		    if (swSetEditBuffer(o_plugin, wave, l / wave->num_order, l % wave->num_order,
					nwrite, value, overflow) == SP_FALSE) {
			spDebug(80, "convSampFreq", "swSetEditBuffer error\n");
		    }
		    nwrite++;
		    syn_pos += downratio;
		}
	    
		dvdatashift(osigs[l], (long)-k);
	    }
	    
	    /* free memory */
	    xdvfree(filsig);
		
	    if (in_thread == SP_TRUE) spYieldThread();
	}
	
	if (pos > 0) {
	    syn_offset = syn_pos - pos;
	    pos = 0;
	
	    if (swWriteEditBuffer(o_plugin, wave, nwrite, SP_FALSE) <= 0) {
		total_len = -1;
		break;
	    }
	}
	
	total_len += nread;

	if (wave->config->edit_func != NULL) {
	    wave->config->edit_func(wave, in_thread, total_len, edit_type, wave->core->call_data);
	}
	if (wave->core->process_flag == SP_FALSE) {
	    break;
	}
    }

    swLockMutex(wave);
    if (wave->core->process_flag == SP_FALSE) {
	total_len = 0;
    }
    swUnlockMutex(wave);

    xdvfree(isig);
    for (l = 0; l < wave->num_order * wave->num_channel; l++) {
	xdvfree(osigs[l]);
    }
    xfree(osigs);

    return total_len;
}

static spLong writeWaveRegion(spPlugin *i_plugin, spPlugin *o_plugin, swWave wave,
			    spWaveInfo *wave_info, swEditType edit_type,
			    spLong offset, spLong length, double value, double *ovalue,
			    void *data, spBool *overflow, spBool callback_flag, spBool in_thread)
{
    spDVector filter;
    spLong total_len = 0;
    
    if (edit_type == SW_EDIT_CROP || edit_type == SW_EDIT_COPY || edit_type == SW_EDIT_WRITE
	|| edit_type == SW_EDIT_BIT_CONV || edit_type == SW_EDIT_MONAURALIZE) {
	return cropWaveRegion(i_plugin, o_plugin, wave, wave_info, edit_type,
			      offset, length, overflow, callback_flag, in_thread);
    } else if (edit_type == SW_EDIT_FILTERING) {
	filter = (spDVector)data;
	return convSampFreq(i_plugin, o_plugin, wave, wave_info, edit_type, 1, 1,
			    2048, value, filter, overflow, in_thread);
    } else if (edit_type == SW_EDIT_SAMP_RATE_CONV) {
	long upratio, downratio;
	double new_samp_rate;
	double ratio;
	double cutoff;
	double transition;

	new_samp_rate = getsfcratio(wave->samp_rate, value, 
				    wave->config->sfc_tolerance / 100.0,
				    &upratio, &downratio);
	spDebug(10, "writeWaveRegion", "New Sampling Frequency: %f\n", new_samp_rate);
	spDebug(10, "writeWaveRegion", "upratio = %ld, downratio = %ld\n", upratio, downratio);
	if (ovalue != NULL) {
	    *ovalue = new_samp_rate;
	}
	
	/* get lowpass filter */
	ratio = (double)MAX(upratio, downratio);
	cutoff = wave->config->sfc_cutoff / ratio;
	transition = wave->config->sfc_transition / ratio;
	spDebug(10, "writeWaveRegion", "ratio = %f, cutoff = %f, transition = %f, sidelobe = %f\n",
		ratio, cutoff, transition, wave->config->sfc_sidelobe);
	if ((filter = xdvlowpass(cutoff, wave->config->sfc_sidelobe, transition, (double)upratio)) != NODATA) {
	    total_len = convSampFreq(i_plugin, o_plugin, wave, wave_info, edit_type, upratio, downratio,
				     wave->config->sfc_buffer_length, wave->config->sfc_gain, filter,
				     overflow, in_thread);
	    xdvfree(filter);
	} else {
            swLockMutex(wave);
            wave->core->process_flag = SP_FALSE;
            wave->core->process_finished = SP_TRUE;
            wave->core->last_error = SW_ERROR_SAMP_FREQ_CONV_FILTER;
            swUnlockMutex(wave);
	    if (callback_flag == SP_TRUE && wave->config->error_func != NULL) {
		wave->config->error_func(wave, in_thread, wave->core->last_error, edit_type, wave->core->call_data);
	    }
	}

	return total_len;
    } else if (edit_type == SW_EDIT_PASTE || edit_type == SW_EDIT_INSERT || edit_type == SW_EDIT_INSERT_PAUSE
	       || edit_type == SW_EDIT_MIX || edit_type == SW_EDIT_REPLACE
	       || edit_type == SW_EDIT_RECORD) {
	return pasteWave(i_plugin, o_plugin, (swWave)data, wave, wave_info, edit_type,
			 offset, length, overflow, in_thread);
    } else {
	return editWaveRegion(i_plugin, o_plugin, wave, wave_info, edit_type,
			      offset, length, value, overflow, in_thread);
    }
}

static swWave findBackupWave(swWave wave, char *o_filename)
{
    swWave owave;
    swWave prev_wave, next_wave;

    owave = NULL;
    prev_wave = wave->prev_wave;
    while (prev_wave != NULL) {
	if (streq(prev_wave->filename, o_filename)) {
	    owave = prev_wave;
	    break;
	}

	prev_wave = prev_wave->prev_wave;
    }

    if (owave == NULL) {
	next_wave = wave->next_wave;
	while (next_wave != NULL) {
	    if (streq(next_wave->filename, o_filename)) {
		owave = next_wave;
		break;
	    }
	    next_wave = next_wave->next_wave;
	}
    }

    return owave;
}

static spBool createBackupFile(swWave wave, char *o_filename, spBool in_thread)
{
    spBool flag = SP_FALSE;
    char filename[SP_MAX_PATHNAME];
    spPlugin *i_plugin;
    spPlugin *o_plugin;
    spWaveInfo wave_info;
    swWave backup_wave;
    swWave prev_wave, next_wave;

    if (wave == NULL || strnone(o_filename)) return SP_FALSE;

    if ((backup_wave = findBackupWave(wave, o_filename)) == NULL) {
	spDebug(10, "createBackupFile", "There is no file that needs backup\n");
	return SP_TRUE;
    }

    swGetTempFile(wave->config, NULL, filename);

    /* open input file */
    if ((i_plugin = swOpenWave(backup_wave, "r")) != NULL) {
	o_plugin = openWaveToWrite(backup_wave, &wave_info, filename,
				   "output_raw", "raw", NULL, 0, 0, 0.0, 0, SP_FALSE,
				   SP_TRUE, in_thread);

	if (o_plugin != NULL) {
	    if (writeWaveRegion(i_plugin, o_plugin, backup_wave, &wave_info, SW_EDIT_COPY,
				0, 0, 0.0, NULL, NULL, NULL, SP_FALSE, in_thread) > 0) {
		flag = SP_TRUE;
	    }
	    
	    /* close output file */
	    spCloseFilePlugin(o_plugin);

	    if (flag == SP_FALSE) {
		spDebug(10, "createBackupFile", "remove file: %s\n", filename);
		spRemoveFile(filename);
	    }
	}
	
	/* close input file */
	spCloseFilePlugin(i_plugin);
    }
    
    if (flag == SP_TRUE) {
	prev_wave = wave->prev_wave;
	while (prev_wave != NULL) {
	    if (streq(prev_wave->filename, o_filename)) {
		spDebug(10, "createBackupFile", "o_filename = %s, prev_wave->filename = %s\n",
			o_filename, prev_wave->filename);
	
		swLockMutex(prev_wave);
		if (prev_wave->plugin_name != NULL) xfree(prev_wave->plugin_name);
		prev_wave->plugin_name = strclone("input_raw");

		if (prev_wave->file_type != NULL) xfree(prev_wave->file_type);
		prev_wave->file_type = strclone("raw");

		if (prev_wave->filename != NULL) xfree(prev_wave->filename);
		prev_wave->filename = strclone(filename);
	    
		prev_wave->orig_flag = SP_FALSE;
		prev_wave->edit_flag = SP_TRUE;
		swUnlockMutex(prev_wave);
	    }

	    prev_wave = prev_wave->prev_wave;
	}
	
	next_wave = wave->next_wave;
	while (next_wave != NULL) {
	    if (streq(next_wave->filename, o_filename)) {
		spDebug(10, "createBackupFile", "o_filename = %s, next_wave->filename = %s\n",
			o_filename, next_wave->filename);
	
		swLockMutex(next_wave);
		if (next_wave->plugin_name != NULL) xfree(next_wave->plugin_name);
		next_wave->plugin_name = strclone("input_raw");

		if (next_wave->file_type != NULL) xfree(next_wave->file_type);
		next_wave->file_type = strclone("raw");

		if (next_wave->filename != NULL) xfree(next_wave->filename);
		next_wave->filename = strclone(filename);
	    
		next_wave->orig_flag = SP_FALSE;
		next_wave->edit_flag = SP_TRUE;
		swUnlockMutex(next_wave);
	    }
	
	    next_wave = next_wave->next_wave;
	}

	return SP_TRUE;
    } else {
	return SP_FALSE;
    }
}

static void updateWaveInfo(spPlugin *o_plugin, swWave wave, spWaveInfo *wave_info,
			   char *filename, char *i_plugin_name, spBool orig_flag)
{
    char *string;
    swWave prev_wave, next_wave;

    if (i_plugin_name != NULL) {
	if (orig_flag == SP_TRUE || wave->plugin_name == NULL) {
	    swLockMutex(wave);
	    
	    if (wave->file_type != NULL) xfree(wave->file_type);
	    
	    if ((string = xspGetPluginFileType(o_plugin, SP_TRUE)) != NULL) {
		wave->file_type = string;
	    } else {
		wave->file_type = NULL;
	    }
	    
	    if (wave->plugin_name != NULL) xfree(wave->plugin_name);
	    wave->plugin_name = strclone(i_plugin_name);

	    if (wave->filename == NULL
		|| (wave->filename != filename && !streq(wave->filename, filename))) {
		if (orig_flag == SP_TRUE) {
		    if (wave->orig_flag == SP_FALSE && !strnone(wave->filename)) {
			spDebug(10, "updateWaveInfo", "remove file: %s\n", wave->filename);
			spRemoveFile(wave->filename);
		    }
		}
		if (wave->filename != NULL) xfree(wave->filename);
		wave->filename = strclone(filename);
	    }
	    
	    if (orig_flag == SP_TRUE) {
		if (wave->core->orig_plugin_name != NULL) xfree(wave->core->orig_plugin_name);
		wave->core->orig_plugin_name = strclone(i_plugin_name);
	    
		wave->orig_flag = SP_TRUE;
	    }
	    
	    swUnlockMutex(wave);
	}
    }
    
    if (orig_flag == SP_TRUE) {
	swLockMutex(wave);
	if (wave->core->orig_filename != filename
	    && !streq(wave->core->orig_filename, filename)) {
	    if (wave->core->orig_filename != NULL) xfree(wave->core->orig_filename);
	    wave->core->orig_filename = strclone(filename);
	    spDebug(10, "updateWaveInfo", "updated: orig_filename = %s\n", wave->core->orig_filename);
	}
	
	if (wave->core->orig_file_type != NULL) xfree(wave->core->orig_file_type);
	wave->core->orig_file_type = xspGetPluginFileType(o_plugin, SP_TRUE);
	spDebug(10, "updateWaveInfo", "wave->core->orig_file_type = %s\n", wave->core->orig_file_type);

	wave->edit_flag = SP_FALSE;

	wave->samp_bit = wave_info->samp_bit;
	wave->num_channel = wave_info->num_channel;
	wave->samp_rate = wave_info->samp_rate;
	spDebug(10, "updateWaveInfo", "wave->samp_bit = %d\n", wave->samp_bit);
	swUnlockMutex(wave);
	
	prev_wave = wave->prev_wave;
	while (prev_wave != NULL) {
	    swLockMutex(prev_wave);
#if 0
	    if (prev_wave->core->orig_plugin_name != NULL) xfree(prev_wave->core->orig_plugin_name);
	    if (prev_wave->core->orig_filename != NULL) xfree(prev_wave->core->orig_filename);
	    if (prev_wave->core->orig_file_type != NULL) xfree(prev_wave->core->orig_file_type);
	    
	    prev_wave->core->orig_plugin_name = strclone(wave->core->orig_plugin_name);
	    prev_wave->core->orig_filename = strclone(wave->core->orig_filename);
	    prev_wave->core->orig_file_type = strclone(wave->core->orig_file_type);
#endif
	    prev_wave->edit_flag = SP_TRUE;
	    swUnlockMutex(prev_wave);
	    
	    prev_wave = prev_wave->prev_wave;
	}
	
	next_wave = wave->next_wave;
	while (next_wave != NULL) {
	    swLockMutex(next_wave);
#if 0
	    if (next_wave->core->orig_plugin_name != NULL) xfree(next_wave->core->orig_plugin_name);
	    if (next_wave->core->orig_filename != NULL) xfree(next_wave->core->orig_filename);
	    if (next_wave->core->orig_file_type != NULL) xfree(next_wave->core->orig_file_type);
	    
	    next_wave->core->orig_plugin_name = strclone(wave->core->orig_plugin_name);
	    next_wave->core->orig_filename = strclone(wave->core->orig_filename);
	    next_wave->core->orig_file_type = strclone(wave->core->orig_file_type);
#endif
	    next_wave->edit_flag = SP_TRUE;
	    swUnlockMutex(next_wave);
	    
	    next_wave = next_wave->next_wave;
	}
    }
    
    return;
}

static spBool swWriteWaveRegion(char *filename, char *plugin_name, char *file_type, char *file_desc,
				int samp_bit, swWave wave, swEditType edit_type,
				spLong offset, spLong length, double value, double *ovalue,
				void *data, spBool *overflow, spBool orig_flag,
				spBool callback_flag, spBool in_thread,
				char **p_i_plugin_name, char **p_o_plugin_name)
{
    spBool flag;
    spLong nwrite = 0;
    double o_samp_rate;
    char *o_plugin_name;
    char *i_plugin_name;
    spPlugin *i_plugin;
    spPlugin *o_plugin;
    spWaveInfo wave_info;
    char *orig_backup = NULL;
    
    if (swIsWaveNone(wave) == SP_TRUE || strnone(filename)) return SP_FALSE;

    spDebug(10, "swWriteWaveRegion", "filename = %s\n", filename);
    
    /* find output plugin */
    if (strnone(plugin_name)) {
	if ((o_plugin_name = xspFindSuitablePluginFile(SP_PLUGIN_DEVICE_FILE,
						       filename, "w")) == NULL) {
	    spDebug(10, "swWriteWaveRegion", "xspFindSuitablePluginFile failed\n");
	    
	    if (0 && wave->core->orig_plugin_name != NULL/* && spGetSuffix(filename) == NULL*/) {
		o_plugin_name = strclone(wave->core->orig_plugin_name);
	    } else {
                swLockMutex(wave);
                wave->core->process_flag = SP_FALSE;
                wave->core->process_finished = SP_TRUE;
                wave->core->last_error = SP_PLUGIN_ERROR_SUITABLE_NOT_FOUND;
                swUnlockMutex(wave);
		if (callback_flag == SP_TRUE && wave->config->error_func != NULL) {
		    return wave->config->error_func(wave, in_thread, wave->core->last_error, edit_type, wave->core->call_data);
		}
		return SP_FALSE;
	    }
	}
    } else {
	o_plugin_name = strclone(plugin_name);
    }
    spDebug(50, "swWriteWaveRegion", "o_plugin_name = %s, orig_flag = %d\n", o_plugin_name, orig_flag);

    if (orig_flag == SP_TRUE) {
	if (createBackupFile(wave, filename, in_thread) == SP_FALSE) {
	    xfree(o_plugin_name);
	    return SP_FALSE;
	}
	orig_backup = xswBackupOriginal(filename);
    }

    i_plugin_name = xspFindRelatedPluginFile(o_plugin_name);
    spDebug(50, "swWriteWaveRegion", "i_plugin_name = %s\n", i_plugin_name);

    flag = SP_FALSE;
    /* open input file */
    if ((i_plugin = swOpenWave(wave, "r")) != NULL) {
	if (edit_type == SW_EDIT_SAMP_RATE_CONV) {
	    o_samp_rate = value;
	} else {
	    o_samp_rate = 0.0;
	}
	
	o_plugin = openWaveToWrite(wave, &wave_info, filename,
				   o_plugin_name, file_type, file_desc,
				   samp_bit, 0, o_samp_rate,
				   length, orig_flag, callback_flag, in_thread);

	if (o_plugin != NULL) {
	    spDebug(10, "swWriteWaveRegion", "openWaveToWrite done\n");
	    if ((nwrite = writeWaveRegion(i_plugin, o_plugin, wave, &wave_info, edit_type,
					  offset, length, value, ovalue,
					  data, overflow, /*!orig_flag*/callback_flag,
					  in_thread)) > 0) {
		flag = SP_TRUE;
	    } else {
		spDebug(10, "swWriteWaveRegion", "writeWaveRegion error\n");
	    }
	}
	    
	/* close input file */
	spCloseFilePlugin(i_plugin);

	if (o_plugin != NULL) {
	    spWaitMutex(wave->core->io_mutex);
	    
	    if (flag == SP_TRUE) {
		updateWaveInfo(o_plugin, wave, &wave_info, filename, i_plugin_name, orig_flag);
	    }
	    
	    /* close output file */
	    spCloseFilePlugin(o_plugin);

	    if (flag == SP_FALSE) {
		spDebug(10, "swWriteWaveRegion", "failed: remove current file: %s\n", filename);
		spRemoveFile(filename);
	    }
	}
    }

    if (p_o_plugin_name != NULL) {
	*p_o_plugin_name = o_plugin_name;
    } else {
	xfree(o_plugin_name);
    }
    if (p_i_plugin_name != NULL) {
	*p_i_plugin_name = i_plugin_name;
    } else {
	if (i_plugin_name != NULL) xfree(i_plugin_name);
    }

    if (orig_backup != NULL) {
	if (flag == SP_TRUE) {
	    spDebug(10, "swWriteWaveRegion", "remove backup file: %s\n", orig_backup);
	    spRemoveFile(orig_backup);
	} else {
	    spDebug(10, "swWriteWaveRegion", "restore backup file: %s --> %s\n",
		    orig_backup, filename);
	    spRenameFile(orig_backup, filename);
	}
	xfree(orig_backup);
    }

    spDebug(10, "swWriteWaveRegion", "done: flag = %d\n", flag);
    
    return flag;
}

static spBool swGenerateWaveformMain(char *filename, char *plugin_name, char *file_type, char *file_desc,
				     swWave wave, swEditType edit_type,
				     spLong delay, spLong duration, double value, double value2, double value3,
				     spBool *overflow, spBool orig_flag,
				     spBool callback_flag, spBool in_thread)
{
    spBool flag;
    spLong nwrite = 0;
    char *o_plugin_name;
    char *i_plugin_name;
    spPlugin *i_plugin;
    spPlugin *o_plugin;
    spWaveInfo wave_info;
    
    if (strnone(filename)) return SP_FALSE;

    spDebug(10, "swGenerateWaveformMain", "in: filename = %s\n", filename);
    
    /* find output plugin */
    if (strnone(plugin_name)) {
	if ((o_plugin_name = xspFindSuitablePluginFile(SP_PLUGIN_DEVICE_FILE,
						       filename, "w")) == NULL) {
	    spDebug(10, "swGenerateWaveformMain", "can't find suitable plugin\n");
            swLockMutex(wave);
            wave->core->process_flag = SP_FALSE;
            wave->core->process_finished = SP_TRUE;
            wave->core->last_error = SP_PLUGIN_ERROR_SUITABLE_NOT_FOUND;
            swUnlockMutex(wave);
	    if (callback_flag == SP_TRUE && wave->config->error_func != NULL) {
		return wave->config->error_func(wave, in_thread, wave->core->last_error, edit_type, wave->core->call_data);
	    }
	    return SP_FALSE;
	}
    } else {
	o_plugin_name = strclone(plugin_name);
	spDebug(10, "swGenerateWaveformMain", "o_plugin_name = %s\n", o_plugin_name);
    }

    i_plugin_name = xspFindRelatedPluginFile(o_plugin_name);
    spDebug(10, "swGenerateWaveformMain", "i_plugin_name = %s\n", i_plugin_name);

    flag = SP_FALSE;
    
    o_plugin = openWaveToWrite(wave, &wave_info, filename,
			       o_plugin_name, file_type, file_desc,
			       wave->samp_bit, 0, wave->samp_rate,
			       wave->length, orig_flag, callback_flag, in_thread);

    if (o_plugin != NULL) {
	spDebug(10, "swGenerateWaveformMain", "openWaveToWrite done\n");
	if ((nwrite = generateWaveform(o_plugin, wave, edit_type,
				       delay, duration, value, value2, value3,
				       overflow, callback_flag,
				       in_thread)) > 0) {
	    flag = SP_TRUE;
	}
	spDebug(10, "swGenerateWaveformMain", "generate done: flag = %d\n", flag);
	
	spWaitMutex(wave->core->io_mutex);
	    
	if (flag == SP_TRUE) {
	    updateWaveInfo(o_plugin, wave, &wave_info, filename, i_plugin_name, orig_flag);
	}
	    
	/* close output file */
	spCloseFilePlugin(o_plugin);

	if (flag == SP_FALSE) {
	    spDebug(10, "swGenerateWaveformMain", "failed: remove current file: %s\n", filename);
	    spRemoveFile(filename);
	} else {
	    if ((i_plugin = swOpenWave(wave, "r")) != NULL) {
		swLockMutex(wave);
		swUpdateTotalLength(i_plugin, wave);
		if (orig_flag == SP_FALSE) {
		    wave->edit_flag = SP_TRUE;
		}
		spDebug(10, "swGenerateWaveformMain", "edit_flag = %d, orig_flag = %d\n",
			wave->edit_flag, wave->orig_flag);
		swUnlockMutex(wave);
		spCloseFilePlugin(i_plugin);
	    } else {
		spDebug(10, "swGenerateWaveformMain", "open error\n");
	    }
	}
    }
	    
    xfree(o_plugin_name);
    if (i_plugin_name != NULL) xfree(i_plugin_name);

    spDebug(100, "swGenerateWaveformMain", "done: flag = %d\n", flag);
    
    return flag;
}

static spBool writeWave(char *filename, char *plugin_name, char *file_type, char *file_desc,
			swWave wave, swEditType edit_type, spLong offset, spLong length, spBool callback_flag, spBool in_thread)
{
    spPlugin *plugin;
    spBool flag = SP_FALSE;
    
    spDebug(100, "writeWave", "in\n");
    
    if (swIsWaveNone(wave) == SP_TRUE) {
        spDebug(100, "writeWave", "wave is none, return SP_FALSE\n");
        return SP_FALSE;
    }

    if (strnone(filename)) {
	if (strnone(wave->core->orig_filename)) {
	    return SP_FALSE;
	}
	
	filename = wave->core->orig_filename;
    }
    spDebug(50, "writeWave", "filename = %s, callback_flag = %d, wave->song_info_mask = %lx\n",
            filename, callback_flag, wave->song_info_mask);

    /* write wave */
    if (!streq(filename, wave->filename) || offset != 0 || length != 0) {
	if ((flag = swWriteWaveRegion(filename, plugin_name, file_type, file_desc, 0, wave,
				      edit_type, offset, length, 0.0, NULL, NULL, NULL,
				      SP_TRUE, callback_flag, in_thread, NULL, NULL)) == SP_TRUE) {
	    if ((plugin = swOpenWave(wave, "r")) != NULL) {
		swLockMutex(wave);
		swUpdateTotalLength(plugin, wave);
		spGetPluginSongInfoMask(plugin, &wave->song_info_mask);
		spGetPluginSongInfo(plugin, (spSongInfo *)&wave->song_info);
                spDebug(50, "writeWave", "after swWriteWaveRegion: wave->song_info_mask = %lx, wave->song_info.info_mask = %lx\n",
                        wave->song_info_mask, wave->song_info.info_mask);
		swUnlockMutex(wave);
		spCloseFilePlugin(plugin);
	    }
	}
    } else {
	flag = SP_TRUE;
    }
    
    return flag;
}

swWave swGetCroppedWave(char *filename, char *plugin_name, char *file_type, char *file_desc,
			swEditType edit_type, swWave wave, spLong offset, spLong length,
			spBool orig_flag, spBool in_thread)
{
    int num_channel;
    char *i_plugin_name;
    char *o_plugin_name;
    spBool flag;
    swWave owave;
    
    if (swIsWaveNone(wave) == SP_TRUE || strnone(filename)) return NULL;

#if 0
    if (plugin_name == NULL) {
	plugin_name = "output_raw";
	file_type = "raw";
    }
#endif

    /* write wave */
    flag = swWriteWaveRegion(filename, plugin_name, file_type, NULL, 0, wave,
			     SW_EDIT_CROP, offset, length, 0.0, NULL, NULL, NULL,
			     SP_FALSE, SP_TRUE, in_thread, &i_plugin_name, &o_plugin_name);

    if (flag == SP_TRUE) {
	spDebug(10, "swGetCroppedWave", "i_plugin_name = %s, o_plugin_name = %s\n", i_plugin_name, o_plugin_name);
	
	if (wave->selected_channel >= 0) {
	    num_channel = 1;
	} else {
	    num_channel = wave->num_channel;
	}
	
	/* allocate wave */
	owave = getWave(wave->config, (edit_type == SW_EDIT_EXTRACT ? NULL : wave),
			filename, i_plugin_name, file_type, wave->samp_bit,
			num_channel, wave->samp_rate, wave->num_order, orig_flag,
			wave->detail_flag, SP_FALSE, NULL, in_thread);

	spDebug(10, "swGetCroppedWave", "getWave done\n");
	
	xfree(i_plugin_name);
	    
	return owave;
    }
    
    spDebug(10, "swGetCroppedWave", "failed\n");
    
    return NULL;
}

spBool swSetOriginalName(swWave wave, char *orig_filename, char *orig_plugin_name)
{
    if (wave == NULL) return SP_FALSE;

#if 1
    if (wave->core->orig_filename != orig_filename && !strnone(orig_filename)) {
	if (wave->core->orig_filename != NULL) xfree(wave->core->orig_filename);
	wave->core->orig_filename = strclone(orig_filename);
    }
    if (wave->core->orig_plugin_name != orig_plugin_name && !strnone(orig_plugin_name)) {
	if (wave->core->orig_plugin_name != NULL) xfree(wave->core->orig_plugin_name);
	wave->core->orig_plugin_name = strclone(orig_plugin_name);
    }
#endif

    return SP_TRUE;
}

spBool swSetOriginalParams(swWave wave, swWave orig_wave, swEditType edit_type, spBool copy_label)
{
    if (wave == NULL || orig_wave == NULL) return SP_FALSE;

#if 0
    if (wave->core->orig_file_type != NULL) xfree(wave->core->orig_file_type);
    if (orig_wave->core->orig_file_type != NULL) {
	wave->core->orig_file_type = strclone(orig_wave->core->orig_file_type);
    } else {
	wave->core->orig_file_type = strclone(orig_wave->file_type);
    }
#endif
    if (copy_label == SP_TRUE) {
	swCopyLabels(wave, orig_wave);
    }
    
    spDebug(50, "swSetOriginalParams", "before spCopySongInfoV2: wave->song_info.info_mask = %lx, orig_wave->song_info.info_mask = %lx, wave->song_info_mask = %lx\n",
            wave->song_info.info_mask, orig_wave->song_info.info_mask, wave->song_info_mask);
    spCopySongInfoV2(&wave->song_info, &orig_wave->song_info);
    if (edit_type != SW_EDIT_WRITE) {
	wave->song_info_mask = orig_wave->song_info_mask;
    }
    spDebug(50, "swSetOriginalParams", "after spCopySongInfoV2: wave->song_info.info_mask = %lx, orig_wave->song_info.info_mask = %lx, wave->song_info_mask = %lx\n",
            wave->song_info.info_mask, orig_wave->song_info.info_mask, wave->song_info_mask);
    
    return SP_TRUE;
}

spBool swSetOriginalRange(swWave wave, swWave orig_wave, double weight)
{
    double min, max;

    if (wave == NULL || orig_wave == NULL) return SP_FALSE;

    min = swGetWaveMin(orig_wave);
    max = swGetWaveMax(orig_wave);
    if (weight < 0.0) {
	return swSetWaveRange(wave, weight * max, weight * min);
    } else {
	return swSetWaveRange(wave, weight * min, weight * max);
    }
}
    
spBool swSetPrevWave(swWave wave, swWave prev_wave, spBool lock_flag)
{
    int i;
    int flag;
    swWave temp, temp2;
    
    if (wave == NULL || prev_wave == NULL) return SP_FALSE;

    flag = 0;
    temp = prev_wave;
    for (i = 0;; i++) {
	if (temp == NULL)
	    break;
	
	if (i >= wave->config->max_num_undo) {
	    temp2 = temp->prev_wave;
	    swFreeWave(temp, lock_flag);
	    temp = temp2;
	    if (i <= 0) {
		flag = 1;
	    }
	} else {
	    temp = temp->prev_wave;
	}
    }

    if (!flag) {
	temp = prev_wave->next_wave;
	for (;;) {
	    if (temp == NULL)
		break;

	    temp2 = temp->next_wave;
	    swFreeWave(temp, lock_flag);
	    temp = temp2;
	}
	
	prev_wave->next_wave = wave;
	wave->prev_wave = prev_wave;
    }

    wave->edit_flag = SP_TRUE;
    
    return SP_TRUE;
}

spBool swCanUndoWave(swWave wave)
{
    if (wave == NULL || wave->prev_wave == NULL) return SP_FALSE;

    return SP_TRUE;
}

spBool swCanRedoWave(swWave wave)
{
    if (wave == NULL || wave->next_wave == NULL) return SP_FALSE;

    return SP_TRUE;
}

spBool swUndoWave(swWave *wave)
{
    if (swCanUndoWave(*wave) == SP_FALSE) return SP_FALSE;

    swFreeEditBuffer(*wave);
    *wave = (*wave)->prev_wave;

    return SP_TRUE;
}

spBool swRedoWave(swWave *wave)
{
    if (swCanRedoWave(*wave) == SP_FALSE) return SP_FALSE;

    swFreeEditBuffer(*wave);
    *wave = (*wave)->next_wave;

    return SP_TRUE;
}

spBool swIsWaveRangeAvailable(swWave wave)
{
    if (swIsWaveNone(wave) == SP_TRUE
	|| wave->range_available == SP_FALSE) return SP_FALSE;

    if ((swIsWaveFloat(wave) == SP_TRUE && wave->dmin > wave->dmax)
	|| (swIsWaveLong(wave) == SP_TRUE && wave->lmin > wave->lmax)
	|| (swIsWaveFloat(wave) == SP_FALSE && swIsWaveLong(wave) == SP_FALSE
	    && wave->min > wave->max)) {
	return SP_FALSE;
    }

    return SP_TRUE;
}

spBool swSetWaveRange(swWave wave, double min, double max)
{
    if (swIsWaveNone(wave) == SP_TRUE) return SP_FALSE;
    
    if (swIsWaveFloat(wave) == SP_TRUE) {
	wave->dmin = min;
	wave->dmax = max;
    } else if (swIsWaveLong(wave) == SP_TRUE) {
	wave->lmin = (long)spRound(min);
	wave->lmax = (long)spRound(max);
    } else {
	wave->min = (short)spRound(min);
	wave->max = (short)spRound(max);
    }

    return SP_TRUE;
}

double swGetWaveMax(swWave wave)
{
    if (swIsWaveNone(wave) == SP_TRUE) return 0.0;
    
    if (swIsWaveFloat(wave) == SP_TRUE) {
	return wave->dmax;
    } else if (swIsWaveLong(wave) == SP_TRUE) {
	return (double)wave->lmax;
    } else {
	return (double)wave->max;
    }
}

double swGetWaveMin(swWave wave)
{
    if (swIsWaveNone(wave) == SP_TRUE) return 0.0;
    
    if (swIsWaveFloat(wave) == SP_TRUE) {
	return wave->dmin;
    } else if (swIsWaveLong(wave) == SP_TRUE) {
	return (double)wave->lmin;
    } else {
	return (double)wave->min;
    }
}

double swGetWaveAbsMax(swWave wave)
{
    double amp_max;
    
    amp_max = ABS(swGetWaveMax(wave));
    amp_max = MAX(ABS(swGetWaveMin(wave)), amp_max);

    return amp_max;
}

spBool swGetWaveBufferRange(swWave wave, long channel, long order, char *buf, spLong buf_length,
			    double *minp, double *maxp, spLong *min_offsetp, spLong *max_offsetp)
{
    spLong k, l;
    spLong min_offset = 0, max_offset = 0;
    
    if (wave == NULL || buf == NULL || buf_length <= 0
	|| channel < 0 || channel >= wave->num_channel
	|| order < 0 || order >= wave->num_order) return SP_FALSE;

    if (swIsWaveFloat(wave) == SP_TRUE) {
	double dmin, dmax;
	double *dbuf = (double *)buf;

	l = (wave->num_order * channel) + order;
	dmin = dmax = dbuf[l]; 
	for (k = 1; k < buf_length; k++) {
	    l = swConvertLengthToBufferLength(wave, k) + (wave->num_order * channel) + order;
	    if (dbuf[l] < dmin) {
		dmin = dbuf[l];
		min_offset = k;
	    }
	    if (dbuf[l] > dmax) {
		dmax = dbuf[l];
		max_offset = k;
	    }
	}
	if (minp != NULL) *minp = dmin;
	if (maxp != NULL) *maxp = dmax;
    } else if (swIsWaveLong(wave) == SP_TRUE) {
	long lmin, lmax;
	long *lbuf = (long *)buf;
	
	l = (wave->num_order * channel) + order;
	lmin = lmax = lbuf[l]; 
	for (k = 1; k < buf_length; k++) {
	    l = swConvertLengthToBufferLength(wave, k) + (wave->num_order * channel) + order;
	    if (lbuf[l] < lmin) {
		lmin = lbuf[l];
		min_offset = k;
	    }
	    if (lbuf[l] > lmax) {
		lmax = lbuf[l];
		max_offset = k;
	    }
	}
	if (minp != NULL) *minp = (double)lmin;
	if (maxp != NULL) *maxp = (double)lmax;
    } else {
	short smin, smax;
	short *sbuf = (short *)buf;
	
	l = (wave->num_order * channel) + order;
	smin = smax = sbuf[l]; 
	for (k = 1; k < buf_length; k++) {
	    l = swConvertLengthToBufferLength(wave, k) + (wave->num_order * channel) + order;
	    if (sbuf[l] < smin) {
		smin = sbuf[l];
		min_offset = k;
	    }
	    if (sbuf[l] > smax) {
		smax = sbuf[l];
		max_offset = k;
	    }
	}
	if (minp != NULL) *minp = (double)smin;
	if (maxp != NULL) *maxp = (double)smax;
    }
    
    if (min_offsetp != NULL) {
	*min_offsetp = min_offset;
    }
    if (max_offsetp != NULL) {
	*max_offsetp = max_offset;
    }

    return SP_TRUE;
}

spBool swGetBufferRange(swWave wave, long channel, long order, double *minp, double *maxp,
			spLong *min_offsetp, spLong *max_offsetp)
{
    if (wave == NULL || wave->buf == NULL) return SP_FALSE;
    
    return swGetWaveBufferRange(wave, channel, order, wave->buf, wave->buf_length, minp, maxp, min_offsetp, max_offsetp);
}

spBool swGetWaveBufferData(swWave wave, long channel, long order, spLong pos, spLong buf_length, char *buf, double *value)
{
    spLong k, l;

    if (buf == NULL || value == NULL || pos < 0) return SP_FALSE;
    
    if (swIsWaveNone(wave) == SP_FALSE) {
	if (channel < 0 || channel >= wave->num_channel) channel = 0;
	if (order < 0 || order >= wave->num_order) order = 0;

	k = pos;
	if (k >= 0 && k < buf_length) {
	    l = swConvertLengthToBufferLength(wave, k) + (wave->num_order * channel) + order;
	    if (swIsWaveFloat(wave) == SP_TRUE) {
		double *dbuf = (double *)buf;
		*value = dbuf[l];
	    } else if (swIsWaveLong(wave) == SP_TRUE) {
		long *lbuf = (long *)buf;
		*value = (double)lbuf[l];
	    } else {
		short *sbuf = (short *)buf;
		*value = (double)sbuf[l];
	    }
	    return SP_TRUE;
	}
    }
    *value = 0.0;
    
    return SP_FALSE;
}

spBool swSetWaveBufferData(swWave wave, long channel, long order, spLong pos, spLong buf_length, char *buf, double value)
{
    spLong k, l;
    
    if (buf == NULL || pos < 0) return SP_FALSE;
    
    if (swIsWaveNone(wave) == SP_FALSE) {
	if (channel < 0 || channel >= wave->num_channel) channel = 0;
        if (order < 0 || order >= wave->num_order) order = 0;

        k = pos;
        if (k >= 0 && k < buf_length) {
            l = swConvertLengthToBufferLength(wave, k) + (wave->num_order * channel) + order;
            if (swIsWaveFloat(wave) == SP_TRUE) {
                double *dbuf = (double *)buf;
                dbuf[l] = value;
            } else if (swIsWaveLong(wave) == SP_TRUE) {
                long *lbuf = (long *)buf;
                lbuf[l] = (long)swGetClippedValue(wave->samp_bit, value, &wave->overflow);
            } else {
                short *sbuf = (short *)buf;
                sbuf[l] = (short)swGetClippedValue(wave->samp_bit, value, &wave->overflow);
            }
            return SP_TRUE;
        }
    }
    
    return SP_FALSE;
}

spBool swGetWaveData(swWave wave, long channel, long order, spLong pos, double *value)
{
    if (wave != NULL) {
        return swGetWaveBufferData(wave, channel, order, pos, wave->length, wave->data, value);
    }
    return SP_FALSE;
}

spBool swSetWaveData(swWave wave, long channel, long order, spLong pos, double value)
{
    if (wave != NULL) {
        return swSetWaveBufferData(wave, channel, order, pos, wave->length, wave->data, value);
    }
    return SP_FALSE;
}

double swGetBufferData(swWave wave, long channel, long order, spLong pos)
{
    double value = 0.0;
    
    if (wave != NULL) {
        swGetWaveBufferData(wave, channel, order, pos, wave->buf_length, wave->buf, &value);
    }
    return value;
}

spBool swSetBufferData(swWave wave, long channel, long order, spLong pos, double value)
{
    if (wave != NULL) {
        swSetWaveBufferData(wave, channel, order, pos, wave->buf_length, wave->buf, value);
    }
    return SP_FALSE;
}

spBool swGetPeakData(swWave wave, long channel, long order, spLong pos, double *value)
{
    if (wave != NULL && wave->peak_buf != NULL
        && wave->peak_buf_length > 0) {
        return swGetWaveBufferData(wave, channel, order,
                                   /*2 * (pos / (2 * wave->peak_buf_thin_length))*/
                                   pos / wave->peak_buf_thin_length,
                                   wave->peak_buf_length, wave->peak_buf, value);
    }
    return SP_FALSE;
}

#ifdef SW_USE_THREAD
void *swCreateProcessThread(int priority, spThreadFunc func, swProcessConfig *config)
{
    void *thread;
    
    if ((thread = spCreateThread(0, priority, func, (void *)config)) != NULL) {
        spYieldThread();
        return thread;
    }

    return NULL;
}
#endif

spBool swLockMutex(swWave wave)
{
#ifdef SW_USE_THREAD
    if (wave == NULL) return SP_FALSE;

    if (wave->core->mutex == NULL) {
        return SP_FALSE;
    }
    
    spDebug(100, "swLockMutex", "++++++++ lock ++++++++\n");
    
    return spLockMutex(wave->core->mutex);
#else
    return SP_FALSE;
#endif
}

spBool swUnlockMutex(swWave wave)
{
#ifdef SW_USE_THREAD
    if (wave == NULL) return SP_FALSE;

    if (wave->core->mutex == NULL) {
        return SP_FALSE;
    }

    spDebug(100, "swUnlockMutex", "-------- unlock --------\n");
    
    return spUnlockMutex(wave->core->mutex);
#else
    return SP_FALSE;
#endif
}

spBool swWaitMutex(swWave wave)
{
#ifdef SW_USE_THREAD
    if (wave == NULL) return SP_FALSE;

    if (wave->core->mutex == NULL) {
        return SP_FALSE;
    }

    spDebug(100, "swWaitMutex", "wating...\n");
    
    return spWaitMutex(wave->core->mutex);
#else
    return SP_FALSE;
#endif
}

spBool swSetEvent(swWave wave)
{
#ifdef SW_USE_THREAD
    if (wave == NULL || wave->core->event == NULL) return SP_FALSE;

    spDebug(100, "swSetEvent", "set...\n");
    
    return spSetEvent(wave->core->event);
#else
    return SP_FALSE;
#endif
}

spBool swResetEvent(swWave wave)
{
#ifdef SW_USE_THREAD
    if (wave == NULL || wave->core->event == NULL) return SP_FALSE;

    spDebug(100, "swResetEvent", "reset...\n");
    
    return spResetEvent(wave->core->event);
#else
    return SP_FALSE;
#endif
}

spBool swWaitEvent(swWave wave)
{
#ifdef SW_USE_THREAD
    if (wave == NULL || wave->core->event == NULL) return SP_FALSE;

    spDebug(100, "swWaitEvent", "waiting...\n");
    
    return spWaitEvent(wave->core->event);
#else
    return SP_FALSE;
#endif
}

spBool swSetThreadStartEvent(swWave wave)
{
#ifdef SW_USE_THREAD
    if (wave == NULL || wave->core->thread_start_event == NULL) return SP_FALSE;

    spDebug(100, "swSetThreadStartEvent", "set...\n");
    
    return spSetEvent(wave->core->thread_start_event);
#else
    return SP_FALSE;
#endif
}

spBool swResetThreadStartEvent(swWave wave)
{
#ifdef SW_USE_THREAD
    if (wave == NULL || wave->core->thread_start_event == NULL) return SP_FALSE;

    spDebug(100, "swResetThreadStartEvent", "reset...\n");
    
    return spResetEvent(wave->core->thread_start_event);
#else
    return SP_FALSE;
#endif
}

spBool swWaitThreadStartEvent(swWave wave)
{
#ifdef SW_USE_THREAD
    if (wave == NULL || wave->core->thread_start_event == NULL) return SP_FALSE;

    spDebug(100, "swWaitThreadStartEvent", "waiting...\n");
    
    return spWaitEvent(wave->core->thread_start_event);
#else
    return SP_FALSE;
#endif
}

static swWave editWaveMainCore(swWave wave, swEditType edit_type, char *file_desc, int samp_bit, 
                               spLong offset, spLong length,
                               double value, double value2, double value3, void *data,
                               spBool in_thread)
{
    spBool copy_label;
    spBool overflow;
    char filename[SP_MAX_PATHNAME];
    char *plugin_name;
    char *file_type;
    char *i_plugin_name;
    char *orig_filename;
    char *orig_plugin_name;
    int num_channel;
    double samp_rate;
    double ovalue;
    swWave owave = NULL;

    spDebug(50, "editWaveMain", "process_flag = %d, new_filename = %s\n", wave->core->process_flag, wave->new_filename);
    
    swLockMutex(wave);
    if (/*swIsWaveNone(wave) == SP_TRUE ||*/ swIsWaveProcessing(wave) == SP_TRUE) {
        spDebug(10, "editWaveMain", "currently processing, return NULL\n");
        swUnlockMutex(wave);
        return NULL;
    }
    wave->core->process_flag = SP_TRUE;
    wave->core->restart_flag = SP_FALSE;
    wave->core->last_error = SP_PLUGIN_ERROR_NO_ERROR;
    swUnlockMutex(wave);

    spDebug(50, "editWaveMain", "edit_type = %d\n", edit_type);
    
    if (edit_type == SW_EDIT_WRITE) {
        spBool callback_flag;
        
        callback_flag = (spRound(value) == 0.0 ? SP_FALSE : SP_TRUE);
        spDebug(50, "editWaveMain", "SW_EDIT_WRITE, callback_flag = %d\n", callback_flag);
        if (writeWave(wave->new_filename, wave->new_plugin_name,
                      wave->new_file_type, file_desc, wave, edit_type, offset, length, callback_flag, in_thread) == SP_TRUE) {
            spDebug(50, "editWaveMain", "writeWave done\n");
            owave = wave;
        } else {
            spDebug(50, "editWaveMain", "writeWave failed\n");
        }
    } else if (edit_type >= SW_EDIT_GENERATE_RECORDING && edit_type <= SW_EDIT_GENERATE_WHITE_NOISE) {
        spDebug(50, "editWaveMain", "generate waveform: edit_type = %d\n", edit_type);
        if (swGenerateWaveformMain(wave->new_filename, wave->new_plugin_name, wave->new_file_type, file_desc,
                                   wave, edit_type,
                                   offset, length, value, value2, value3,
                                   &overflow, (spBool)data,
                                   SP_TRUE, in_thread) == SP_TRUE) {
            spDebug(50, "editWaveMain", "swGenerateWaveformMain succeeded: edit_type = %d, overflow = %d\n",
                    edit_type, overflow);
            wave->overflow = overflow;
            owave = wave;
        } else {
            spDebug(50, "editWaveMain", "swGenerateWaveformMain failed\n");
        }
    } else {
        if (!strnone(wave->new_filename)) {
            spStrCopy(filename, sizeof(filename), wave->new_filename);
        } else {
            swGetTempFile(wave->config, NULL, filename);
        }
        spDebug(50, "editWaveMain", "filename = %s\n", filename);
        
        if (edit_type == SW_EDIT_EXTRACT) {
            orig_filename = wave->new_filename;
            if (wave->new_plugin_name != NULL) {
                orig_plugin_name = wave->new_plugin_name;
            } else {
                orig_plugin_name = wave->core->orig_plugin_name;
            }
            copy_label = SP_FALSE;
        } else {
            orig_filename = wave->core->orig_filename;
            orig_plugin_name = wave->core->orig_plugin_name;
            copy_label = SP_TRUE;
        }
        
        if (!strnone(wave->new_plugin_name) || !strnone(wave->new_filename)) {
            plugin_name = wave->new_plugin_name;
            file_type = wave->new_file_type;
        } else {
            plugin_name = "output_raw";
            file_type = "raw";
        }
            
        if (edit_type == SW_EDIT_CROP || edit_type == SW_EDIT_EXTRACT) {
            owave = swGetCroppedWave(filename, plugin_name, file_type, file_desc,
                                     edit_type, wave, offset, length,
                                     (wave->new_filename != NULL ? SP_TRUE : SP_FALSE),
                                     in_thread);
        } else {
            if (edit_type != SW_EDIT_BIT_CONV) {
                samp_bit = wave->samp_bit;
            }
            overflow = SP_FALSE;
            
            /* write wave */
            if ((i_plugin_name = xspFindRelatedPluginFile(plugin_name)) != NULL
                && swWriteWaveRegion(filename, plugin_name, file_type, NULL, samp_bit, wave,
                                     edit_type, offset, length, value, &ovalue, data, &overflow,
                                     SP_FALSE, SP_TRUE, in_thread, NULL, NULL) == SP_TRUE) {
                if (edit_type == SW_EDIT_SAMP_RATE_CONV) {
                    samp_rate = value;
                } else {
                    samp_rate = wave->samp_rate;
                }
                if (edit_type == SW_EDIT_MONAURALIZE) {
                    num_channel = 1;
                } else {
                    num_channel = wave->num_channel;
                }
                
                /* allocate wave */
                if ((owave  = getWave(wave->config, wave, filename, i_plugin_name, file_type, samp_bit,
                                      num_channel, samp_rate, wave->num_order,
                                      (wave->new_filename != NULL ? SP_TRUE : SP_FALSE),
                                      wave->detail_flag, SP_FALSE,
                                      NULL, in_thread)) != NULL) {
                    spDebug(50, "editWaveMain", "overflow = %d\n", overflow);
                    owave->overflow = overflow;
                    if (edit_type == SW_EDIT_SAMP_RATE_CONV) {
                        owave->exact_samp_rate = ovalue;
                    }
                }
                
                xfree(i_plugin_name);
            }
        }
        
        if (owave != NULL) {
            spDebug(50, "editWaveMain", "before swLockMutex(wave)\n");
            swLockMutex(wave);
                
            spDebug(50, "editWaveMain", "call swSetOriginalName\n");
            swSetOriginalName(owave, orig_filename, orig_plugin_name);
            spDebug(50, "editWaveMain", "call swSetOriginalParams\n");
            swSetOriginalParams(owave, wave, edit_type, copy_label);
        
            if (edit_type == SW_EDIT_EXTRACT) {
                if (strnone(wave->new_filename)) {
                    owave->edit_flag = SP_TRUE;
                }
            } else {
#if 0
                if (edit_type == SW_EDIT_BIT_CONV) {
                    value = getBitConvWeight(wave, owave->samp_bit);
                } else if (edit_type != SW_EDIT_AMPLIFY) {
                    value = 1.0;
                }

                /* set wave range */
                swSetOriginalRange(owave, wave, value);
#endif
        
                /* set previous wave */
                spDebug(50, "editWaveMain", "call swSetPrevWave\n");
                swSetPrevWave(owave, wave, SP_FALSE);

                wave->edit_offset = offset;
                if (edit_type == SW_EDIT_PASTE
                    || edit_type == SW_EDIT_INSERT || edit_type == SW_EDIT_MIX) {
                    swWave src_wave = (swWave)data;
                    wave->edit_length = MIN(src_wave->total_length, wave->total_length - offset);
                } else {
                    wave->edit_length = length;
                }
            }
            
            swUnlockMutex(wave);
            spDebug(50, "editWaveMain", "after swUnlockMutex(wave)\n");
        }
    }

    spDebug(50, "editWaveMain", "done\n");
    
    return owave;
}

static swWave editWaveMain(swWave wave, swEditType edit_type, char *file_desc, int samp_bit, 
                           spLong offset, spLong length,
                           double value, double value2, double value3, void *data,
                           spBool in_thread)
{
    spBool process_flag;
    spBool process_finished;
    swWave owave = NULL;

    do {
        owave = editWaveMainCore(wave, edit_type, file_desc, samp_bit, offset, length,
                                 value, value2, value3, data, in_thread);
        spDebug(10, "editWaveMain", "wave->core->restart_flag = %d\n", wave->core->restart_flag);
    } while (wave->core->restart_flag == SP_TRUE);

    swLockMutex(wave);
    process_flag = wave->core->process_flag;
    process_finished = wave->core->process_finished;
    
    if (wave->new_filename != NULL) xfree(wave->new_filename);
    if (wave->new_plugin_name != NULL) xfree(wave->new_plugin_name);
    if (wave->new_file_type != NULL) xfree(wave->new_file_type);
    wave->new_filename = NULL;
    wave->new_plugin_name = NULL;
    wave->new_file_type = NULL;
    
    wave->core->process_flag = SP_FALSE;
    wave->core->process_finished = SP_TRUE;
    swUnlockMutex(wave);
    
    spDebug(50, "editWaveMain", "process_flag = %d, process_finished = %d\n", process_flag, process_finished);

    if (edit_type >= SW_EDIT_GENERATE_RECORDING && edit_type <= SW_EDIT_GENERATE_WHITE_NOISE
        && owave == NULL) {
        spDebug(50, "editWaveMain", "swGenerateWaveformMain failed: edit_type = %d\n", edit_type);
        if (wave->config->error_func != NULL) {
            wave->config->error_func(wave, in_thread, wave->core->last_error != SP_PLUGIN_ERROR_NO_ERROR ? SW_ERROR_GENERATE_FAILED : SW_ERROR_GENERATE_STOPPED,
                                     edit_type, wave->core->call_data);
        }
    } else {
        if (process_finished == SP_FALSE
            && process_flag == SP_TRUE /* not interrupted */
            && wave->config->edit_finish_func != NULL) {
            spDebug(50, "editWaveMain", "call edit_finish_func\n");
            wave->config->edit_finish_func(wave, owave, in_thread, edit_type, wave->core->call_data);
        }

        if (process_finished == SP_FALSE && wave->config->edit_func != NULL) {
            spDebug(50, "editWaveMain", "call edit_func with SW_PROCESS_FINISHED\n");
            wave->config->edit_func(wave, in_thread, SW_PROCESS_FINISHED, edit_type, wave->core->call_data);
        }
    }
    
    spDebug(100, "editWaveMain", "done\n");
    
    return owave;
}

#ifdef SW_USE_THREAD
spThreadReturn editWaveThread(void *data)
{
    swWave wave;
    swProcessConfig *config = (swProcessConfig *)data;

    if (config == NULL) return SP_THREAD_RETURN_FAILURE;
    
    spDebug(10, "editWaveThread", "in\n");

    swSetThreadStartEvent(config->wave);
    
    config->wave->core->in_edit_thread = SP_TRUE;
    
    wave = editWaveMain(config->wave, config->edit_type, config->file_desc, config->samp_bit,
                        config->offset, config->length,
                        config->value, config->value2, config->value3, config->data, SP_TRUE);

    if (config->wave != NULL) { /* config->wave can be NULL in callback functions */
        config->wave->core->in_edit_thread = SP_FALSE;
    }
    
    spDebug(10, "editWaveThread", "done\n");
    
    if (wave != NULL) {
        return SP_THREAD_RETURN_SUCCESS;
    } else {
        return SP_THREAD_RETURN_FAILURE;
    }
}
#endif

static swWave editWave(swWave wave, swEditType edit_type, int samp_bit,
                       spLong offset, spLong length,
                       double value, double value2, double value3, void *data,
                       char *filename, char *plugin_name, char *file_type, char *file_desc,
                       spBool use_thread, spBool *thread_flag)
{
    spDebug(100, "editWave", "in: edit_type = %d, use_thread = %d\n", edit_type, use_thread);
    
    if (wave->new_filename != NULL) xfree(wave->new_filename);
    if (wave->new_plugin_name != NULL) xfree(wave->new_plugin_name);
    if (wave->new_file_type != NULL) xfree(wave->new_file_type);
    
    if (filename != NULL) {
        wave->new_filename = strclone(filename);
    } else {
        wave->new_filename = NULL;
    }
    if (plugin_name != NULL) {
        wave->new_plugin_name = strclone(plugin_name);
    } else {
        wave->new_plugin_name = NULL;
    }
    if (file_type != NULL) {
        wave->new_file_type = strclone(file_type);
    } else {
        wave->new_file_type = NULL;
    }
    
#ifdef SW_USE_THREAD
    if (use_thread == SP_TRUE
        && wave->config->thread_safe == SP_TRUE && wave->config->process_use_thread == SP_TRUE
        && wave->config->edit_finish_func != NULL && edit_type != SW_EDIT_EXTRACT) {
        spDebug(1, "editWave", "thread-based processing...\n");
        wave->core->process_config.edit_type = edit_type;
        wave->core->process_config.wave = wave;
        wave->core->process_config.samp_bit = samp_bit;
        spStrCopy(wave->core->process_config.file_desc, sizeof(wave->core->process_config.file_desc), file_desc);
        wave->core->process_config.offset = offset;
        wave->core->process_config.length = length;
        wave->core->process_config.value = value;
        wave->core->process_config.value2 = value2;
        wave->core->process_config.value3 = value3;
        wave->core->process_config.data = data;

        if (wave->core->edit_thread != NULL) {
            if (wave->core->in_edit_thread == SP_FALSE) {
                wave->config->thread_wait_func(wave, wave->core->edit_thread, wave->core->call_data);
            }
            spDestroyThread(wave->core->edit_thread);
            wave->core->edit_thread = NULL;
            spDebug(1, "editWave", "spDestroyThread done\n");
        }

        swResetThreadStartEvent(wave);
        
        spDebug(1, "editWave", "call swCreateProcessThread\n");
        if ((wave->core->edit_thread = swCreateProcessThread(SP_THREAD_PRIORITY_BELOW_NORMAL,
                                                             editWaveThread, &wave->core->process_config)) != NULL) {
            spDebug(1, "editWave", "swCreateProcessThread done\n");
            if (thread_flag != NULL) *thread_flag = SP_TRUE;
            swWaitThreadStartEvent(wave);
            spDebug(1, "editWave", "swWaitThreadStartEvent done\n");
            return NULL;
        }
        
        spDebug(1, "editWave", "thread error\n");
        swSetThreadStartEvent(wave);
    }
#endif

    if (thread_flag != NULL) *thread_flag = SP_FALSE;
    
    spDebug(1, "editWave", "call editWaveMain\n");
    
    return editWaveMain(wave, edit_type, file_desc, samp_bit, offset, length, value, value2, value3, data, SP_FALSE);
}

static swWave swEditWave(swWave wave, swEditType edit_type, int samp_bit,
                         spLong offset, spLong length, double value, void *data)
{
    return editWave(wave, edit_type, samp_bit, offset, length, value, 0.0, 0.0, data,
                    NULL, NULL, NULL, NULL, SP_TRUE, NULL);
}

spBool swWriteWave(char *filename, char *plugin_name, char *file_type, char *file_desc,
                   swWave wave, spBool callback_flag, spBool use_thread)
{
    spBool thread_flag = SP_FALSE;
    
    if (editWave(wave, SW_EDIT_WRITE, 0, 0, 0, (double)callback_flag, 0.0, 0.0, NULL,
                 filename, plugin_name, file_type, file_desc, use_thread, &thread_flag) == NULL
        && thread_flag == SP_FALSE) {
        return SP_FALSE;
    }
    
    return SP_TRUE;
}

spBool swGenerateWaveform(char *filename, char *plugin_name, char *file_type, char *file_desc,
                          swWave wave, swEditType edit_type, spLong delay, spLong duration,
                          double f0, double initial_phase, double gain, spBool use_thread)
{
    char buf[SP_MAX_PATHNAME];
    spBool orig_flag = SP_TRUE;
    spBool thread_flag = SP_FALSE;

    if (strnone(filename)) {
        swGetTempFile(wave->config, NULL, buf);
        filename = buf;
        plugin_name = "output_raw";
        file_type = "raw";
        orig_flag = SP_FALSE;
#if 0
    } else if (spGetSuffix(filename) == NULL && plugin_name == NULL) {
        spDebug(10, "swGenerateWaveform", "no suffix: filename = %s\n", filename);
        spStrCopy(buf, sizeof(buf), filename);
        spReplaceNSuffix(buf, sizeof(buf), ".wav");
        filename = buf;
        plugin_name = "output_wav";
#endif
    }
    spDebug(10, "swGenerateWaveform", "filename = %s\n", filename);
    
    if (editWave(wave, edit_type, 0, delay, duration, f0, initial_phase, gain, (void *)orig_flag,
                 filename, plugin_name, file_type, file_desc, use_thread, &thread_flag) == NULL
        && thread_flag == SP_FALSE) {
        spDebug(10, "swGenerateWaveform", "editWave error\n");
        return SP_FALSE;
    }
    
    spDebug(10, "swGenerateWaveform", "done\n");
    
    return SP_TRUE;
}

swWave swExtractWave(swWave wave, spLong offset, spLong length)
{
    return swEditWave(wave, SW_EDIT_EXTRACT, 0, offset, length, 0.0, NULL);
}

swWave swExtractAndWriteWave(swWave wave, spLong offset, spLong length,
                             char *filename, char *plugin_name, char *file_type, char *file_desc)
{
    return editWave(wave, SW_EDIT_EXTRACT, 0, offset, length, 0.0, 0.0, 0.0, NULL,
                    filename, plugin_name, file_type, file_desc, SP_FALSE, NULL);
}

swWave swCropWave(swWave wave, spLong offset, spLong length)
{
    return swEditWave(wave, SW_EDIT_CROP, 0, offset, length, 0.0, NULL);
}

swWave swDeleteWave(swWave wave, spLong offset, spLong length)
{
    return swEditWave(wave, SW_EDIT_DELETE, 0, offset, length, 0.0, NULL);
}

swWave swEraseWave(swWave wave, spLong offset, spLong length)
{
    return swEditWave(wave, SW_EDIT_ERASE, 0, offset, length, 0.0, NULL);
}

swWave swAmplifyWave(swWave wave, spLong offset, spLong length, double value)
{
    return swEditWave(wave, SW_EDIT_AMPLIFY, 0, offset, length, value, NULL);
}

swWave swFadeInWave(swWave wave, spLong offset, spLong length)
{
    return swEditWave(wave, SW_EDIT_FADE_IN, 0, offset, length, 0.0, NULL);
}

swWave swFadeOutWave(swWave wave, spLong offset, spLong length)
{
    return swEditWave(wave, SW_EDIT_FADE_OUT, 0, offset, length, 0.0, NULL);
}

swWave swSwapWaveChannel(swWave wave, spLong offset, spLong length)
{
    return swEditWave(wave, SW_EDIT_CHANNEL_SWAP, 0, offset, length, 0.0, NULL);
}

swWave swConvertWaveBit(swWave wave, int samp_bit)
{
    return swEditWave(wave, SW_EDIT_BIT_CONV, samp_bit, 0, 0, 0.0, NULL);
}

swWave swConvertWaveSampFreq(swWave wave, double samp_rate)
{
    return swEditWave(wave, SW_EDIT_SAMP_RATE_CONV, 0, 0, 0, samp_rate, NULL);
}

swWave swFilteringWave(swWave wave, spDVector filter)
{
    return swEditWave(wave, SW_EDIT_FILTERING, 0, 0, 0, 1.0, filter);
}

swWave swMonauralizeWave(swWave wave)
{
    return swEditWave(wave, SW_EDIT_MONAURALIZE, 0, 0, 0, 0.0, NULL);
}

swWave swChangeWaveValue(swWave wave, int channel, spLong offset, double value)
{
    return swEditWave(wave, SW_EDIT_CHANGE_VALUE, 0, offset, channel, value, NULL);
}

swWave swPasteWave(swWave wave, swWave src_wave, spLong offset, int channel)
{
    return swEditWave(wave, SW_EDIT_PASTE, 0, offset, channel, 0.0, (void *)src_wave);
}

swWave swMixWave(swWave wave, swWave src_wave, spLong offset, int channel)
{
    return swEditWave(wave, SW_EDIT_MIX, 0, offset, channel, 0.0, (void *)src_wave);
}

swWave swInsertWave(swWave wave, swWave src_wave, spLong offset, int channel)
{
    return swEditWave(wave, SW_EDIT_INSERT, 0, offset, channel, 0.0, (void *)src_wave);
}

swWave swInsertPauseWave(swWave wave, spLong offset, spLong pause_length)
{
    return swEditWave(wave, SW_EDIT_INSERT_PAUSE, 0, offset, pause_length, 0.0, NULL);
}

swWave swRecordWave(swWave wave, spLong offset, spLong length)
{
    return swEditWave(wave, SW_EDIT_RECORD, 0, offset, length, 0.0, NULL);
}

swWave swReplaceWave(swWave wave, swWave src_wave, spLong offset, spLong length)
{
    return swEditWave(wave, SW_EDIT_REPLACE, 0, offset, length, 0.0, (void *)src_wave);
}

spBool swSetWaveErrorCallback(swWaveConfig config, swWaveErrorCallback func)
{
    if (config == NULL) return SP_FALSE;

    config->error_func = func;

    return SP_TRUE;
}

spBool swSetWaveOptionCallback(swWaveConfig config, swWaveOptionCallback func)
{
    if (config == NULL) return SP_FALSE;

    config->option_func = func;

    return SP_TRUE;
}

spBool swSetWavePlayCallback(swWaveConfig config, swWavePlayCallback func)
{
    if (config == NULL) return SP_FALSE;

    config->play_func = func;

    return SP_TRUE;
}

spBool swSetWaveReadCallback(swWaveConfig config, swWaveIoCallback func)
{
    if (config == NULL) return SP_FALSE;

    config->read_func = func;

    return SP_TRUE;
}

spBool swSetWaveEditCallback(swWaveConfig config, swWaveEditCallback func)
{
    if (config == NULL) return SP_FALSE;

    config->edit_func = func;

    return SP_TRUE;
}

spBool swSetWaveEditFinishCallback(swWaveConfig config, swWaveEditFinishCallback func)
{
    if (config == NULL) return SP_FALSE;

    config->edit_finish_func = func;

    return SP_TRUE;
}

spBool swSetWaveThreadWaitCallback(swWaveConfig config, swWaveThreadWaitCallback func)
{
    if (config == NULL) return SP_FALSE;

    config->thread_wait_func = func;

    return SP_TRUE;
}

spBool swSetWaveCallbackData(swWave wave, void *data)
{
    if (wave == NULL) return SP_FALSE;

    wave->core->call_data = data;

    return SP_TRUE;
}

spBool swProcessStop(swWave wave)
{
    spBool flag = SP_FALSE;
    
    if (wave == NULL) return SP_FALSE;

    if (wave->core->process_finished == SP_FALSE) {
        if (wave->core->process_flag == SP_TRUE) {
            wave->core->process_flag = SP_FALSE;
            flag = SP_TRUE;
        }
    }
    spDebug(1, "swProcessStop", "flag = %d\n", flag);
    
    return flag;
}

spDVector xswGetAnalysisWindow(swWindowType window_type, spLong length, double *power)
{
    spDVector ana_window;

    /* create analysis window */
    ana_window = xdvalloc((long)length);

    switch (window_type) {
      case SW_WINDOW_HAMMING:
        hamming(ana_window->data, ana_window->length);
        break;
      case SW_WINDOW_HANNING:
        hanning(ana_window->data, ana_window->length);
        break;
      case SW_WINDOW_BLACKMAN:
        blackman(ana_window->data, ana_window->length);
        break;
      case SW_WINDOW_GAUSS:
        gausswin(ana_window->data, ana_window->length, 2.5);
        break;
      default:
        dvones(ana_window, ana_window->length);
        break;
    }

    if (power != NULL) {
        *power = dvsqsum(ana_window) / (double)ana_window->length;
    }
    
    return ana_window;
}

spBool swNormalizeSpectrum(swWaveConfig config, swAnalysisType analysis_type,
                           spDVector data, double min_value_for_dB, double amp_max, spBool force_dB_flag)
{
    if ((analysis_type & SW_ANALYSIS_SPECTRUM) == SW_ANALYSIS_SPECTRUM) {
        if (force_dB_flag == SP_FALSE && config->linear_spectrum == SP_TRUE) {
            if (config->normalize_spectrum == SP_TRUE) {
                if (amp_max > 0.0) {
                    dvscoper(data, "/", amp_max);
                }

                return SP_TRUE;
            }
        } else {
            dvscmax(data, min_value_for_dB);
            dvdecibel(data);
            if (config->normalize_spectrum == SP_TRUE && amp_max != 1.0) {
                double dbmax;
                
                dbmax = dB(amp_max);
                dvscoper(data, "-", dbmax);
            }
            
            return SP_TRUE;
        }
    }

    return SP_FALSE;
}

long swAnalysisWaveFrame(void *anarec, swWaveConfig config, swAnalysisType analysis_type,
                         spDVector ana_window, spDVector data, long current_framel, long lifl, double samp_rate,
                         double min_value_for_dB, double amp_max, spBool force_dB_flag, spDVector odata)
{
    long fftl;
    long length;
    spBool flag = SP_TRUE;
    spFFTRec fftrec = NULL;
    spCQTRec cqtrec = NULL;

    if (analysis_type & SW_ANALYSIS_CQT_MASK) {
        cqtrec = (spCQTRec)anarec;
    } else {
        fftrec = (spFFTRec)anarec;
    }
    
    fftl = data->length;
    length = ana_window->length;
    if (current_framel > 0) {
        length = MIN(length, current_framel);
    }

    switch (analysis_type) {
      case SW_ANALYSIS_GROUP_DELAY:
      case SW_ANALYSIS_SMOOTHED_GROUP_DELAY:
      case SW_ANALYSIS_TD_GROUP_DELAY:
        {
            spDVector tmp;
            tmp = xdvfftgrpdlyex(fftrec, data);
                
            if (analysis_type == SW_ANALYSIS_SMOOTHED_GROUP_DELAY
                && lifl >= 1) {
                dvifftabsex(fftrec, tmp);
                dvlif(tmp, fftl, lifl);
                dvfftabsex(fftrec, tmp);
            }
                
            dvscoper(tmp, "-", (double)(length / 2));
                
            if (analysis_type == SW_ANALYSIS_TD_GROUP_DELAY) {
                dvifftabsex(fftrec, tmp);
            } else {
                dvscoper(tmp, "*", 1000.0 / samp_rate);
            }

            dvreal(odata);
            dvcopy(odata, tmp);
                
            xdvfree(tmp);
        }
        break;
      case SW_ANALYSIS_CQT_SPECTRUM:
      case SW_ANALYSIS_ERB_CQT_SPECTRUM:
        break;
      default:
        dvcopy(odata, data);
        break;
    }
    
    switch (analysis_type) {
      case SW_ANALYSIS_SPECTRUM:
        dvfftabsex(fftrec, odata);
        break;
      case SW_ANALYSIS_SMOOTHED_SPECTRUM:
        if (lifl >= 1) {
            dvrcepex(fftrec, odata);
            dvlif(odata, fftl, lifl);
            dvfftabsex(fftrec, odata);
            dvexp(odata);
        } else {
            dvfftabsex(fftrec, odata);
        }
        break;
      case SW_ANALYSIS_CEPSTRUM:
      case SW_ANALYSIS_CEPSTRUM_F0:
        dvrcepex(fftrec, odata);
        break;
      case SW_ANALYSIS_PHASE:
      case SW_ANALYSIS_UNWRAPPED_PHASE:
        dvfftangleex(fftrec, odata);
        if (analysis_type == SW_ANALYSIS_UNWRAPPED_PHASE) {
            dvunwrap(odata, 0.0);
        }
        break;
      case SW_ANALYSIS_CQT_SPECTRUM:
      case SW_ANALYSIS_ERB_CQT_SPECTRUM:
        spExtractCQTWholeInputSpectrum(cqtrec, data, length / 2, odata, NULL);
        break;
        
      case SW_ANALYSIS_GROUP_DELAY:
      case SW_ANALYSIS_SMOOTHED_GROUP_DELAY:
      case SW_ANALYSIS_TD_GROUP_DELAY:
        break;
        
      default:
        flag = SP_FALSE;
        break;
    }

    if (flag != SP_FALSE) {
        swNormalizeSpectrum(config, analysis_type, odata, min_value_for_dB, amp_max, force_dB_flag);
        return fftl / 2 + 1;
    } else {
        return -1;
    }
}

long swAnalysisCQTBlock(spCQTRec cqtrec, swWaveConfig config, spDVector data, long current_framel, long odata_offset, long odata_stride,
                        spCQTBlock io_block, spDVector odata)
{
    long k;
    long frames;
    long freq_count;
    long nprocess;
    long nwrite;
    spDVector spectrum;

    if ((nprocess = spProcessCQTBlock(cqtrec, data, 0, current_framel, io_block)) <= 0) {
        spDebug(10, "swAnalysisCQTBlock", "spProcessCQTBlock failed: nprocess = %ld\n", nprocess);
        return nprocess;
    }

    frames = spGetCQTBlockCurrentValidResultLength(cqtrec, io_block);
    freq_count = spGetCQTFreqCount(cqtrec);
    spDebug(100, "swAnalysisCQTBlock", "nprocess = %ld, frames = %ld, freq_count = %ld\n", nprocess, frames, freq_count);

    spectrum = xdvzeros(freq_count);
    
    for (k = 0, nwrite = 0; k < frames; k++) {
        spCopyCQTBlockSpectrumAt(cqtrec, io_block, k, spectrum);
        dvpaste(odata, spectrum, odata_offset, spectrum->length, SP_FALSE);
        nwrite += spectrum->length;
        odata_offset += odata_stride;
    }

    xdvfree(spectrum);

    return nwrite;
}

static void *swInitAnalysisRec(swWaveConfig config, swAnalysisType analysis_type, double samp_rate, long data_length, long shift_length,
                               long *io_ana_internal_length, double *io_optional_ana_cond, long *o_ana_length, long *o_sig_buffer_length)
{
    void *anarec = NULL;
    
    spDebug(10, "swInitAnalysisRec", "in: analysis_type = %lx, samp_rate = %f, data_length = %ld\n",
            analysis_type, samp_rate, data_length);
    
    if (analysis_type == SW_ANALYSIS_CQT_SPECTRUM || analysis_type == SW_ANALYSIS_ERB_CQT_SPECTRUM) {
        spCQTConfig cqt_config;
        spCQTRec cqtrec;

        spInitCQTConfig(&cqt_config);
        cqt_config.samp_freq = samp_rate;
        cqt_config.bins_per_octave = config->cqt_bins_per_octave;
        cqt_config.min_freq = config->cqt_min_freq;
        if (shift_length > 0) {
            cqt_config.block_length = 2 * shift_length;
            cqt_config.whole_input_length = 0;
            cqt_config.block_transition_factor = config->cqt_block_transition_factor;
        } else {
            cqt_config.whole_input_length = data_length;
            cqt_config.block_length = 0;
        }
        if (io_optional_ana_cond != NULL) {
            cqt_config.reference_stride = *io_optional_ana_cond;
        }
        
        if (analysis_type == SW_ANALYSIS_ERB_CQT_SPECTRUM) {
            cqt_config.gamma = spCalcCQTGammaOfERB(*io_ana_internal_length);
        }
        cqt_config.options = /*SP_CQT_OPTION_RASTERIZE_METHOD_FULL*/SP_CQT_OPTION_RASTERIZE_METHOD_FULL_POWER_OF_2 | SP_CQT_OPTION_NORMALIZE_METHOD_SINE | SP_CQT_OPTION_PHASE_MODE_GLOBAL/*SP_CQT_OPTION_PHASE_MODE_LOCAL*/;
        if (config->cqt_min_freq_based_on_musical_note == SP_TRUE) {
            cqt_config.options |= SP_CQT_OPTION_MIN_FREQ_BASED_ON_MUSICAL_NOTE;
        }
        
        if ((cqtrec = spInitCQT(&cqt_config)) == NULL) {
            return NULL;
        }
        if (shift_length > 0) {
            *o_sig_buffer_length = spGetCQTBlockPushLength(cqtrec);
        } else {
            *o_sig_buffer_length = cqt_config.whole_input_length;
        }
        *o_ana_length = spGetCQTFreqCount(cqtrec);
        if (io_optional_ana_cond != NULL) {
            *io_optional_ana_cond = cqt_config.reference_stride;
        }
        /* *io_ana_internal_length = cqt_config.reference_stride; */
        anarec = cqtrec;
        spDebug(10, "swInitAnalysisRec", "CQT: o_sig_buffer_length = %ld, gamma = %f, o_ana_length = %ld, cqt_config.reference_stride = %f\n",
                *o_sig_buffer_length, cqt_config.gamma, *o_ana_length, cqt_config.reference_stride);
    } else {
        long fftorder;
        spFFTRec fftrec;
        
        fftorder = spNextPow2(*io_ana_internal_length);
        
        *o_sig_buffer_length = spPow2(fftorder);
        *io_ana_internal_length = *o_sig_buffer_length;
        *o_ana_length = *io_ana_internal_length / 2 + 1;
    
        if ((fftrec = dvinitfft(fftorder, SP_FFT_DEFAULT_PRECISION)) == NULL) {
            return NULL;
        }
        anarec = fftrec;
    }

    return anarec;
}

static void swFreeAnalysisRec(swAnalysisType analysis_type, void *anarec)
{
    if (analysis_type & SW_ANALYSIS_CQT_MASK) {
        spFreeCQT((spCQTRec)anarec);
    } else {
        dvfreefft((spFFTRec)anarec);
    }
    
    return;
}

swWave swAnalysisData(swWaveConfig config, double *data, spLong length,
                      swAnalysisType analysis_type, swWindowType window_type,
                      double samp_rate, long ana_internal_length, double lifterm,
                      spBool dB_flag, spBool detail_flag) {
    long k;
    long ana_length;
    long lifl;
    long sig_buffer_length;
    char filename[SP_MAX_PATHNAME];
    swWave wave;
    spDVector ana_data;
    spDVector ana_window;
    spDVector odata;
    void *anarec;

    if (analysis_type & SW_ANALYSIS_CQT_MASK) {
        window_type = SW_WINDOW_RECTANGLE;
    }
    
    if ((anarec = swInitAnalysisRec(config, analysis_type, samp_rate, (long)length, 0, &ana_internal_length, NULL,
                                    &ana_length, &sig_buffer_length)) == NULL) {
        return NULL;
    }

    lifl = (long)spRound(samp_rate * lifterm / 1000.0);
    
    swGetTempFile(config, NULL, filename);
    
    wave = swInitWave(config, NULL,
                      filename, "input_raw", "raw",
                      64, 1, samp_rate, 1, SP_FALSE, detail_flag);
    swAllocData(wave, ana_length);
    wave->length = ana_length;
    wave->total_length = ana_length;
    
    ana_window = xswGetAnalysisWindow(window_type, length, NULL);
    ana_data = xdvzeros(sig_buffer_length);
    if (analysis_type & SW_ANALYSIS_CQT_MASK) {
        odata = xdvzeros(ana_length);
    } else {
        odata = xdvzeros(sig_buffer_length);
    }
    
    for (k = 0; k < ana_data->length; k++) {
        if (k < length) {
            ana_data->data[k] = data[k] * ana_window->data[k];
        } else {
            ana_data->data[k] = 0.0;
        }
#if 0
        if (ana_data->imag != NULL) {
            ana_data->imag[k] = 0.0;
        }
#endif
    }

    if (swAnalysisWaveFrame(anarec, wave->config, analysis_type, ana_window, ana_data, (long)length,
                            lifl, samp_rate, 1.0e-30, 1.0, dB_flag, odata) > 0) {
        for (k = 0; k < ana_length; k++) {
            swSetWaveData(wave, 0, 0, k, odata->data[k]);
        }
    }

    if (analysis_type & SW_ANALYSIS_CQT_MASK) {
        wave->custom_x_axis = xspGetCQTFreqAxis((spCQTRec)anarec);
        wave->log_like_custom_x_axis = SP_TRUE;
    }
    
    xdvfree(ana_window);
    xdvfree(ana_data);
    xdvfree(odata);

    swFreeAnalysisRec(analysis_type, anarec);
    
    return wave;
}

swWave swAnalysisWave(swWave iwave, spLong offset, spLong length,
                      swAnalysisType analysis_type, swWindowType window_type,
                      long ana_internal_length, double lifterm)
{
    int n;
    int channel;
    int num_channel;
    long k;
    long ana_length;
    long lifl;
    long sig_buffer_length;
    spLong nread, nwrite;
    double amp_max;
    char filename[SP_MAX_PATHNAME];
    swWave wave;
    spPlugin *plugin;
    spDVector ana_window;
    spDVector data;
    spDVector odata;
    void *anarec = NULL;

    swLockMutex(iwave);
    if (swIsWaveNone(iwave) == SP_TRUE || swIsWaveProcessing(iwave) == SP_TRUE
        || iwave->num_order > 1) {
        swUnlockMutex(iwave);
        return NULL;
    }
    iwave->core->process_flag = SP_TRUE;
    iwave->core->process_finished = SP_FALSE;
    iwave->core->restart_flag = SP_FALSE;
    iwave->core->last_error = SP_PLUGIN_ERROR_NO_ERROR;
    swUnlockMutex(iwave);
    
    if (analysis_type & SW_ANALYSIS_CQT_MASK) {
        ana_internal_length = iwave->config->cqt_bins_per_octave;
        window_type = SW_WINDOW_RECTANGLE;
    } else {
        ana_internal_length = spPow2(spNextPow2(ana_internal_length));
    }
    spDebug(80, "swAnalysisWave", "ana_internal_length = %ld\n", ana_internal_length);
    ana_length = 0;
    lifl = 0;

    if (iwave->selected_channel >= 0) {
        num_channel = 1;
    } else {
        num_channel = iwave->num_channel;
    }

    swGetTempFile(iwave->config, NULL, filename);
    
    wave = swInitWave(iwave->config, NULL,
                      filename, "input_raw", "raw",
                      64, num_channel, iwave->samp_rate, 1, SP_FALSE, iwave->detail_flag);
        
    /* read wave */
    if ((plugin = swOpenWave(iwave, "r")) == NULL
        || (anarec = swInitAnalysisRec(iwave->config, analysis_type, iwave->samp_rate, (long)length, 0,
                                       &ana_internal_length, NULL, &ana_length, &sig_buffer_length)) == NULL) {
        if (plugin != NULL) spCloseFilePlugin(plugin);
        nread = 0;
    } else {
        swAllocData(wave, ana_length);
        wave->length = ana_length;
        wave->total_length = ana_length;
        spDebug(80, "swAnalysisWave", "wave->length = %ld, wave->total_length = %ld\n", wave->length, wave->total_length);
        
        lifl = (long)spRound(iwave->samp_rate * lifterm / 1000.0);
        
        spSeekPlugin(plugin, offset * iwave->num_order);
        nread = swReadBuffer(plugin, iwave, -1, length);
        spCloseFilePlugin(plugin);

        spDebug(10, "swAnalysisWave", "length = %ld, nread = %ld\n", length, nread);
    }

    if (nread > 0) {
        /*data = xdvrizeros(sig_buffer_length);*/
        data = xdvzeros(sig_buffer_length);
        odata = xdvzeros(ana_length);
        ana_window = xswGetAnalysisWindow(window_type, nread, NULL);

        if (iwave->config->normalize_spectrum_max == SP_TRUE) {
            amp_max = swGetWaveAbsMax(iwave);
        } else {
            amp_max = swGetLimitValue(iwave->samp_bit);
        }
        
        for (n = 0; n < iwave->num_channel; n++) {
            if (iwave->selected_channel >= 0 && n != iwave->selected_channel) {
                continue;
            }

            for (k = 0; k < data->length; k++) {
                if (k < nread) {
                    data->data[k] = swGetBufferData(iwave, n, 0, k) * ana_window->data[k];
                } else {
                    data->data[k] = 0.0;
                }
                if (data->imag != NULL) {
                    data->imag[k] = 0.0;
                }
            }

            if (iwave->selected_channel >= 0) {
                channel = 0;
            } else {
                channel = n;
            }
            if (swAnalysisWaveFrame(anarec, iwave->config, analysis_type, ana_window, data, (long)nread,
                                    lifl, iwave->samp_rate, /*1.0e-5*/1.0e-30, amp_max, SP_FALSE, odata) <= 0) {
                break;
            }
            
            for (k = 0; k < odata->length; k++) {
                swSetWaveData(wave, channel, 0, k, odata->data[k]);
            }
        }

        xdvfree(ana_window);
        xdvfree(odata);
        xdvfree(data);
    }

    spDebug(10, "swAnalysisWave", "set analysis data done\n");

    if (nread > 0) {
        /* write wave */
        if ((plugin = swOpenWave(wave, "w")) != NULL) {
            if ((nwrite = swWriteData(plugin, wave)) <= 0) {
                nread = 0;
            }
            spCloseFilePlugin(plugin);
            spDebug(10, "swAnalysisWave", "write data done\n");
        } else {
            nread = 0;
        }
    }

    if (analysis_type & SW_ANALYSIS_CQT_MASK) {
        wave->custom_x_axis = xspGetCQTFreqAxis((spCQTRec)anarec);
        /*dvfdump(wave->custom_x_axis, spgetstderr());*/
        wave->log_like_custom_x_axis = SP_TRUE;
        spDebug(10, "swAnalysisWave", "wave->custom_x_axis->length = %ld, wave->length = %ld\n",
                wave->custom_x_axis->length, wave->length);
    }
    
    if (anarec != NULL) swFreeAnalysisRec(analysis_type, anarec);
    
    swLockMutex(iwave);
    iwave->core->process_flag = SP_FALSE;
    iwave->core->process_finished = SP_TRUE;
    swUnlockMutex(iwave);
    
    if (nread <= 0) {
        swFreeWave(wave, SP_TRUE);
        return NULL;
    } else {
        swUpdateAnalysisStringInfo(wave, analysis_type);
        return wave;
    }
}

#if 1
static double calcCepstrumF0(swWave wave, spDVector data, long min_lifl, long max_lifl, double uv_thresh)
{
    spDVector cdata;
    double peak;
    long length;
    long index = 0;

    if (max_lifl <= 0) {
        length = data->length - min_lifl;
    } else {
        length = max_lifl - min_lifl;
    }
    
    cdata = xdvcut(data, min_lifl, length);
    peak = dvmax(cdata, &index);
    xdvfree(cdata);

    if (peak >= uv_thresh) {
        return wave->samp_rate / (double)(min_lifl + index);
    }
    
    return 0.0;
}

swWave swGetSpectrogram(swWave wave, spLong offset, spLong length, long framel, long shiftl,
                        swAnalysisType analysis_type, swWindowType window_type,
                        long ana_internal_length, double lifterm, spBool *interrupted)
{
    long k, l, n;
    long sig_buffer_length;
    long lifl;
    long ana_length;
    long num_order;
    long first_skip_frames;
    long frames;
    long current_framel;
    spLong total_read_len;
    spLong total_write_len;
    spLong write_len;
    spLong current_write_len;
    spLong nread, nwrite;
    double samp_rate;
    double f0;
    double power, window_power;
    double amp_max;
    double min_value_for_dB;
    double optional_ana_cond;
    char filename[SP_MAX_PATHNAME];
    spWaveInfo wave_info;
    spPlugin *plugin;
    spPlugin *o_plugin;
    spDVector buf, data;
    spDVector ana_window;
    spDVector odata;
    swWave owave;
    void *anarec = NULL;

    if (swIsWaveNone(wave) == SP_TRUE 
        || wave->num_order > 1 || ((analysis_type & SW_ANALYSIS_CQT_MASK) == 0 && (framel <= 0 || shiftl <= 0))) {
        spDebug(10, "swGetSpectrogram", "error: first check, framel = %ld, shiftl = %ld, num_order = %ld\n",
                framel, shiftl, wave->num_order);
        return NULL;
    }

    swLockMutex(wave);
    if (swIsWaveProcessing(wave) == SP_TRUE) {
        swUnlockMutex(wave);
        return NULL;
    }
    wave->core->process_flag = SP_TRUE;
    wave->core->process_finished = SP_FALSE;
    wave->core->restart_flag = SP_FALSE;
    
    if (interrupted != NULL) *interrupted = SP_FALSE;
    
    swUnlockMutex(wave);

    if (wave->config->edit_func != NULL) {
        wave->config->edit_func(wave, SP_FALSE, SW_PROCESS_STARTED, SW_EDIT_SPECTROGRAM, wave->core->call_data);
    }
    
    swGetTempFile(wave->config, NULL, filename);
    spDebug(80, "swGetSpectrogram", "swGetTempFile result: filename = %s\n", filename);

    if (analysis_type & SW_ANALYSIS_CQT_MASK) {
        double gamma = 0.0;
        
        if (analysis_type & SW_ANALYSIS_ERB_AXIS_MASK) {
            gamma = spCalcCQTGammaOfERB(wave->config->cqt_bins_per_octave);
        }
        shiftl = spCalcCQTMinBlockLength(wave->config->cqt_min_freq, wave->config->cqt_bins_per_octave, gamma, wave->config->cqt_block_margin_factor,
                                         wave->samp_rate, SP_TRUE, wave->config->cqt_min_freq_based_on_musical_note) / 2;
        spDebug(80, "swGetSpectrogram", "CQT shiftl = %ld, cqt_min_freq = %f, gamma = %f, cqt_bins_per_octave = %ld, cqt_block_margin_factor = %f\n",
                shiftl, wave->config->cqt_min_freq, gamma, wave->config->cqt_bins_per_octave, wave->config->cqt_block_margin_factor);
        framel = shiftl;
        window_type = SW_WINDOW_RECTANGLE;
        first_skip_frames = 0;
        optional_ana_cond = SP_CQT_REFERENCE_STRIDE_DEFAULT;
    } else {
        shiftl = MIN(framel, shiftl);
        shiftl = MAX(framel / 8, shiftl);
        first_skip_frames = MAX((framel / 2) / shiftl - 1, 0);
        optional_ana_cond = 0.0;
    }
    samp_rate = 1.0 / ((double)shiftl / wave->samp_rate);
    min_value_for_dB = /*1.0e-5*/1.0e-10;
    spDebug(80, "swGetSpectrogram", "samp_rate = %f\n", samp_rate);

    if (offset < 0) offset = 0;
    if (length <= 0) length = wave->total_length;

    spInitWaveInfo(&wave_info);
    
    spStrCopy(wave_info.file_type, SP_WAVE_FILE_TYPE_SIZE, "raw");
    wave_info.num_channel = wave->num_channel;
    wave_info.samp_bit = 33;
    wave_info.samp_rate = samp_rate;
    
    if ((o_plugin = spOpenFilePlugin("output_raw", filename, "w",
                                     SP_PLUGIN_DEVICE_FILE,
                                     &wave_info, NULL, NULL, NULL, NULL)) == NULL) {
        spDebug(20, "swGetSpectrogram", "spOpenFilePlugin error: %s\n", filename);
        owave = NULL;
    } else {
        if ((anarec = swInitAnalysisRec(wave->config, analysis_type, wave->samp_rate, framel, shiftl, &ana_internal_length,
                                        &optional_ana_cond, &ana_length, &sig_buffer_length)) == NULL) {
            owave = NULL;
        } else {
            long block_frames = 0;
            spCQTBlock *cqtblocks = NULL;
            
            if (analysis_type & SW_ANALYSIS_CQT_MASK) {
                framel = shiftl = sig_buffer_length;
                samp_rate = 1.0 / (optional_ana_cond / wave->samp_rate);
                spDebug(20, "swGetSpectrogram", "CQT new samp_rate = %f, optional_ana_cond = %f, wave->num_channel = %d\n",
                        samp_rate, optional_ana_cond, wave->num_channel);
                cqtblocks = xalloc(wave->num_channel, spCQTBlock);
                for (n = 0; n < wave->num_channel; n++) {
                    cqtblocks[n] = spInitCQTBlock((spCQTRec)anarec, NULL, NULL);
                }
                block_frames = spGetCQTBlockValidResultLength((spCQTRec)anarec);
                lifl = 0;
            } else {
                lifl = (long)spRound(wave->samp_rate * lifterm / 1000.0);
            }
            
            if (analysis_type == SW_ANALYSIS_CEPSTRUM_F0 || analysis_type == SW_ANALYSIS_POWER) {
                num_order = 1;
                write_len = 1;
            } else {
                num_order = ana_length;
                if (analysis_type & SW_ANALYSIS_CQT_MASK) {
                    write_len = ana_length * block_frames;
                } else {
                    write_len = ana_length;
                }
            }
            
            spDebug(20, "swGetSpectrogram", "samp_rate = %f, ana_length = %ld, num_order = %ld, framel = %ld, shiftl = %ld, lifl = %ld\n",
                    samp_rate, ana_length, num_order, framel, shiftl, lifl);
            
            nread = -1;
            nwrite = -1;
    
            if ((plugin = swOpenWave(wave, "r")) != NULL) {
                spSeekPlugin(plugin, offset * wave->num_order);

                buf = xdvzeros(framel * wave->num_channel);
                ana_window = xswGetAnalysisWindow(window_type, framel, &window_power);
                total_write_len = 0;
                frames = 0;
                
                if (analysis_type & SW_ANALYSIS_CQT_MASK) {
                    amp_max = 1.0;
                } else if (wave->config->normalize_spectrum_max == SP_TRUE) {
                    amp_max = swGetWaveAbsMax(wave);
                } else {
                    amp_max = swGetLimitValue(wave->samp_bit);
                }
                spDebug(80, "swGetSpectrogram", "amp_max = %f\n", frames, amp_max);
                        
                if (analysis_type == SW_ANALYSIS_POWER) {
                    data = xdvalloc(ana_window->length);
                    odata = NODATA;
                } else if (analysis_type & SW_ANALYSIS_CQT_MASK) {
                    data = xdvalloc(framel);
                    odata = xdvalloc((long)write_len * wave->num_channel);
                } else {
                    data = xdvalloc(ana_internal_length);
                    odata = xdvalloc(ana_length);
                }
                
                /*for (total_read_len = 0; total_read_len <= length;) {*/
                for (total_read_len = 0; total_read_len < length + (first_skip_frames + 1) * shiftl;) {
                    if (wave->config->edit_func != NULL) {
                        wave->config->edit_func(wave, SP_FALSE, total_read_len, SW_EDIT_SPECTROGRAM, wave->core->call_data);
                    }
                    spDebug(80, "swGetSpectrogram", "frames = %ld, wave->core->process_flag = %d\n",
                            frames, wave->core->process_flag);
                    if (wave->core->process_flag == SP_FALSE) {
                        break;
                    }

                    if (total_read_len < length) {
                        if ((nread = swReadBuffer(plugin, wave, -1, shiftl)) < 0) {
                            break;
                        }
                        spDebug(80, "swGetSpectrogram", "framel = %ld, shiftl = %ld, nread = %ld\n",
                                framel, shiftl, nread);
                        if (nread != shiftl) {
                            spDebug(80, "swGetSpectrogram", "**** final frame: shiftl = %ld, nread = %ld ****\n",
                                    shiftl, nread);
                        }
                    } else {
                        nread = 0;
                        spDebug(80, "swGetSpectrogram", "total_read_len (%ld) >= length (%ld), nread = 0\n",
                                total_read_len, length);
                    }

                    current_framel = framel - shiftl;
                    l = current_framel * wave->num_channel;
                
                    for (k = 0; k < shiftl; k++) {
                        for (n = 0; n < wave->num_channel; n++) {
                            if (l >= 0) {
                                if (k >= nread) {
                                    buf->data[l++] = 0.0;
                                } else {
                                    buf->data[l++] = swGetBufferData(wave, n, 0, k);
                                }
                            }
                        }
                    }

                    current_framel += (long)nread;
                    
                    if (frames >= first_skip_frames) {
                        nwrite = -1;

                        for (n = 0; n < wave->num_channel; n++) {
                            if (data->imag != NULL) {
                                dvreal(data);
                            }
                            
                            for (k = 0; k < data->length; k++) {
                                if (k < ana_window->length) {
                                    l = k * wave->num_channel + n;
                                    data->data[k] = buf->data[l] * ana_window->data[k];
                                } else {
                                    data->data[k] = 0.0;
                                }
                            }

                            if (analysis_type == SW_ANALYSIS_POWER) {
                                power = dvsqsum(data);
                                power /= (double)ana_window->length;
                            
                                if (wave->config->normalize_spectrum == SP_TRUE) {
                                    power /= SQUARE(amp_max);
                                }
                                power = dBpow(power);
                                nwrite = spWritePlugin(o_plugin, &power, 1);
                            } else {
                                if (analysis_type & SW_ANALYSIS_CQT_MASK) {
                                    if ((current_write_len = swAnalysisCQTBlock((spCQTRec)anarec, wave->config, nread <= 0 ? NODATA : data, current_framel,
                                                                                ana_length * n, ana_length * wave->num_channel, cqtblocks[n], odata)) <= 0) {
                                        spDebug(10, "swGetSpectrogram", "swAnalysisCQTBlock failed\n");
                                        break;
                                    }
                                } else {
                                    if ((current_write_len = swAnalysisWaveFrame(anarec, wave->config, analysis_type, ana_window, data, current_framel,
                                                                                 lifl, wave->samp_rate, min_value_for_dB, amp_max, SP_FALSE, odata)) <= 0) {
                                        spDebug(10, "swGetSpectrogram", "swAnalysisWaveFrame failed\n");
                                        break;
                                    }
                                }
                                if (analysis_type == SW_ANALYSIS_CEPSTRUM_F0) {
                                    f0 = calcCepstrumF0(wave, odata, lifl, ana_length, /*0.08*/0.15);
                                    spDebug(20, "swGetSpectrogram", "f0 = %f\n", f0);
                                    nwrite = spWritePlugin(o_plugin, &f0, 1);
                                } else if (!(analysis_type & SW_ANALYSIS_CQT_MASK)) {
                                    nwrite = spWritePlugin(o_plugin, odata->data, (long)current_write_len);
                                } else {
                                    nwrite = current_write_len;
                                }
                                spDebug(100, "swGetSpectrogram", "n = %ld, nwrite = %ld, current_write_len = %ld / %ld\n",
                                        n, nwrite, current_write_len, write_len);
                            }
                            total_write_len += MAX(nwrite, 0);
                            
                            if (nwrite <= 0 || nread <= 0) {
                                spDebug(20, "swGetSpectrogram", "loop break/continue: n = %ld, nwrite = %ld, nread = %ld\n", n, nwrite, nread);
                                if (n + 1 >= wave->num_channel) {
                                    break;
                                } else {
                                    continue;
                                }
                            }
                        }

                        if ((analysis_type & SW_ANALYSIS_CQT_MASK) && frames >= 1) {
                            swNormalizeSpectrum(wave->config, SW_ANALYSIS_CQT_SPECTRUM, odata, min_value_for_dB, amp_max, SP_FALSE);
    
                            nwrite = spWritePlugin(o_plugin, odata->data, odata->length);
                        }
                        
                        if ((!(analysis_type & SW_ANALYSIS_CQT_MASK) || frames >= 1) && (nwrite <= 0 || nread <= 0)) {
                            break;
                        }
                    }
        
                    dvdatashift(buf, -shiftl * wave->num_channel);
                    total_read_len += shiftl;
                    ++frames;
                }
                spDebug(80, "swGetSpectrogram", "total_read_len = %ld, total_write_len = %ld, frames = %ld, wave->total_length = %ld\n",
                        total_read_len, total_write_len, frames, wave->total_length);
    
                xdvfree(data);
                if (odata != NODATA) xdvfree(odata);
                        
                xdvfree(ana_window);
                xdvfree(buf);
    
                spCloseFilePlugin(plugin);
            }
    
            spCloseFilePlugin(o_plugin);

            if (wave->core->process_flag == SP_FALSE) {
                if (interrupted != NULL) *interrupted = SP_TRUE;
                nwrite = -1;
            }

            spDebug(20, "swGetSpectrogram", "end: nwrite = %ld, nread = %ld, samp_rate = %f, ana_length = %ld, num_order = %ld\n",
                    nwrite, nread, samp_rate, ana_length, num_order);
    
            if (nwrite <= 0 || nread < 0) {
                /* error */
#if 1
                if (nwrite < 0) {
                    spRemoveFile(filename);
                }
#endif
                owave = NULL;
            } else {
                /* allocate wave */
                owave = getWave(wave->config, NULL, filename, "input_raw", "raw", 33,
                                wave->num_channel, samp_rate, num_order,
                                SP_FALSE, wave->detail_flag, SP_FALSE, NULL, SP_FALSE);
                swUpdateAnalysisStringInfo(owave, analysis_type);
                if (analysis_type != SW_ANALYSIS_WAVEFORM && analysis_type != SW_ANALYSIS_CEPSTRUM) {
                    owave->order_frequency_flag = SP_TRUE;
                }
                if (analysis_type & SW_ANALYSIS_CQT_MASK) {
                    owave->custom_x_axis = xspGetCQTFreqAxis((spCQTRec)anarec);
                    /*dvfdump(owave->custom_x_axis, spgetstderr());*/
                    owave->log_like_custom_x_axis = SP_TRUE;
                    spDebug(10, "swGetSpectrogram", "owave->custom_x_axis->length = %ld, owave->total_length = %ld\n",
                            owave->custom_x_axis->length, owave->total_length);
                }
            }
            
            if (cqtblocks != NULL) {
                for (n = 0; n < wave->num_channel; n++) {
                    spFreeCQTBlock(cqtblocks[n]);
                }
                xfree(cqtblocks);
            }
            swFreeAnalysisRec(analysis_type, anarec);
        }
    }

    swLockMutex(wave);
    wave->core->process_flag = SP_FALSE;
    wave->core->process_finished = SP_TRUE;
    swUnlockMutex(wave);

    if (wave->config->edit_func != NULL) {
        wave->config->edit_func(wave, SP_FALSE, SW_PROCESS_FINISHED, SW_EDIT_SPECTROGRAM, wave->core->call_data);
    }
    
    spDebug(50, "swGetSpectrogram", "done\n");
    
    return owave;
}
#endif
