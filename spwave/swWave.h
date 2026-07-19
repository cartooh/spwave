/*
 *	swWave.h
 */

#ifndef __SWWAVE_H
#define __SWWAVE_H

#include <sp/spDefs.h>
#include <sp/spString.h>
#include <sp/spThread.h>
#include <sp/spPlugin.h>
#include <sp/spOutputPlugin.h>
#include <sp/spInputPlugin.h>
#include <sp/spLib.h>
#ifdef SW_SUPPORT_MORPHING
#include <straight/straight.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define SW_DEFAULT_TEMP_FILE_SUFFIX ".ad"
#define SW_PEAK_FILE_SUFFIX ".spk"
#define SW_DEFAULT_PEAK_THIN_LENGTH 256
    
/* parameters for analysis */    
#define SW_ANALYSIS_SPECTRUM_LABEL "Spectrum"
#define SW_ANALYSIS_SMOOTHED_SPECTRUM_LABEL "Smoothed Spectrum"
#define SW_ANALYSIS_PHASE_LABEL "Phase"
#define SW_ANALYSIS_UNWRAPPED_PHASE_LABEL "Unwrapped Phase"
#define SW_ANALYSIS_GROUP_DELAY_LABEL "Group Delay"
#define SW_ANALYSIS_SMOOTHED_GROUP_DELAY_LABEL "Smoothed Group Delay"
#define SW_ANALYSIS_TD_GROUP_DELAY_LABEL "Time-Domain Group Delay"
#define SW_ANALYSIS_CEPSTRUM_LABEL "Cepstrum"

#define SW_ANALYSIS_SPECTRUM_PARAM_LABEL "spectrum"
#define SW_ANALYSIS_SMOOTHED_SPECTRUM_PARAM_LABEL "smoothed_spectrum"
#define SW_ANALYSIS_PHASE_PARAM_LABEL "phase"
#define SW_ANALYSIS_UNWRAPPED_PHASE_PARAM_LABEL "unwrapped_phase"
#define SW_ANALYSIS_GROUP_DELAY_PARAM_LABEL "group_delay"
#define SW_ANALYSIS_SMOOTHED_GROUP_DELAY_PARAM_LABEL "smoothed_group_delay"
#define SW_ANALYSIS_TD_GROUP_DELAY_PARAM_LABEL "td_group_delay"
#define SW_ANALYSIS_CEPSTRUM_PARAM_LABEL "cepstrum"

#define SW_WINDOW_RECTANGLE_LABEL "Rectangle"
#define SW_WINDOW_HAMMING_LABEL "Hamming"
#define SW_WINDOW_HANNING_LABEL "Hanning"
#define SW_WINDOW_BLACKMAN_LABEL "Blackman"
#define SW_WINDOW_GAUSS_LABEL "Gauss"

#define SW_WINDOW_RECTANGLE_PARAM_LABEL "rectangle"
#define SW_WINDOW_HAMMING_PARAM_LABEL "hamming"
#define SW_WINDOW_HANNING_PARAM_LABEL "hanning"
#define SW_WINDOW_BLACKMAN_PARAM_LABEL "blackman"
#define SW_WINDOW_GAUSS_PARAM_LABEL "gauss"

/* Callback procedure
 *
 * edit_func [pos = SW_PROCESS_STARTED] --> edit_func [pos = current position]
 * --> edit_finish_func --> edit_func [pos = SW_PROCESS_FINISHED]
 *
 * play_func [pos = SW_PROCESS_STARTED] --> play_func [pos = current position]
 * --> play_func [pos = SW_PROCESS_FINISHED]
 *
 * swIsWaveProcessing will be SP_FALSE when pos = SW_PROCESS_FINISHED
 */

#define SW_USE_THREAD

#define SW_ERROR_PLAY_LONG SP_PLUGIN_ERROR_USER
#define SW_ERROR_LOOP_PLAY_SHORT (SP_PLUGIN_ERROR_USER-1)
#define SW_ERROR_DIFFERENT_CHANNEL (SP_PLUGIN_ERROR_USER-2)
#define SW_ERROR_DIFFERENT_SAMP_RATE (SP_PLUGIN_ERROR_USER-3)
#define SW_ERROR_SOURCE_WAVE_NOT_FOUND (SP_PLUGIN_ERROR_USER-4)
#define SW_ERROR_SOURCE_WAVE_PROCESSING (SP_PLUGIN_ERROR_USER-5)
#define SW_ERROR_SAMP_FREQ_CONV (SP_PLUGIN_ERROR_USER-6)
#define SW_ERROR_SAMP_FREQ_CONV_FILTER (SP_PLUGIN_ERROR_USER-7)
#define SW_ERROR_WRITE (SP_PLUGIN_ERROR_USER-8)
#define SW_ERROR_GENERATE_STOPPED (SP_PLUGIN_ERROR_USER-9)
#define SW_ERROR_GENERATE_FAILED (SP_PLUGIN_ERROR_USER-10)
#define SW_ERROR_AUDIO_DEVICE_ERROR (SP_PLUGIN_ERROR_USER-11)

#define SW_PROCESS_FINISHED -1
#define SW_PROCESS_ERROR -2
#define SW_PROCESS_STARTED -3
#define SW_PROCESS_REACH_END -4

#define SW_MIN_LOOP_PLAY_LENGTH 8192
    
typedef enum {
    SW_TIME_FORMAT_UNKNOWN = -1,
    SW_TIME_FORMAT_SEC = 0,
    SW_TIME_FORMAT_MSEC = 1,
    SW_TIME_FORMAT_POINT = 2,
    SW_TIME_FORMAT_SEPARATED_SEC = 3,
    SW_TIME_FORMAT_FLOORED_MSEC = 4,
    SW_TIME_FORMAT_INDEXED = 5,
    SW_TIME_FORMAT_SEC_WITH_DATA = 6,
    SW_TIME_FORMAT_MSEC_WITH_DATA = 7,
    SW_TIME_FORMAT_POINT_WITH_DATA = 8,
    SW_TIME_FORMAT_SEPARATED_SEC_WITH_DATA = 9,
    SW_TIME_FORMAT_FLOORED_MSEC_WITH_DATA = 10,
    SW_TIME_FORMAT_INDEXED_WITH_DATA = 11,
    SW_TIME_FORMAT_ESPS = 12,
} swTimeFormat;

typedef enum {
    SW_EDIT_NONE = -1,
    SW_EDIT_CROP,
    SW_EDIT_EXTRACT,
    SW_EDIT_DELETE,
    SW_EDIT_ERASE,
    SW_EDIT_COPY, /* 4 */
    
    SW_EDIT_PASTE,
    SW_EDIT_INSERT,
    SW_EDIT_MIX,
    SW_EDIT_REPLACE,
    
    SW_EDIT_AMPLIFY, /* 9 */
    SW_EDIT_FADE_IN,
    SW_EDIT_FADE_OUT,
    SW_EDIT_CHANNEL_SWAP,
    SW_EDIT_BIT_CONV,
    SW_EDIT_SAMP_RATE_CONV,
    SW_EDIT_CHANNEL_CONV,
    SW_EDIT_MONAURALIZE,
    SW_EDIT_CHANGE_VALUE,
    
    SW_EDIT_READ, /* 18 */
    SW_EDIT_WRITE,
    
    SW_EDIT_INSERT_PAUSE,
    SW_EDIT_SPECTROGRAM,
    SW_EDIT_RECORD,
    SW_EDIT_LONG_TERM_ANALYSIS,
    
    SW_EDIT_GENERATE_RECORDING = 1000,
    SW_EDIT_GENERATE_SILENCE,
    SW_EDIT_GENERATE_SINE,
    SW_EDIT_GENERATE_SQUARE,
    SW_EDIT_GENERATE_TRIANGLE,
    SW_EDIT_GENERATE_SAWTOOTH,
    SW_EDIT_GENERATE_WHITE_NOISE,

    SW_EDIT_FILTERING = 1100,
} swEditType;

typedef unsigned long swAnalysisType;
#define SW_ANALYSIS_FREQUENCY_DOMAIN_MASK (1L<<0)
#define SW_ANALYSIS_TIME_DOMAIN_MASK (1L<<1)
#define SW_ANALYSIS_SCALAR_MASK (1L<<2)
    
#define SW_ANALYSIS_F0_MASK (1L<<6)
#define SW_ANALYSIS_AMPLITUDE_MASK (1L<<7)
#define SW_ANALYSIS_POWER_MASK (1L<<8)
#define SW_ANALYSIS_PHASE_MASK (1L<<9)
#define SW_ANALYSIS_GROUP_DELAY_MASK (1L<<10)
#define SW_ANALYSIS_CEPSTRUM_MASK (1L<<11)
#define SW_ANALYSIS_APERIODICITY_MASK (1L<<12)
#define SW_ANALYSIS_STRAIGHT_MASK (1L<<13)
#define SW_ANALYSIS_CQT_MASK (1L<<14)

#define SW_ANALYSIS_UNWRAPPED_MASK (1L<<17)
#define SW_ANALYSIS_SMOOTHED_MASK (1L<<18)
#define SW_ANALYSIS_ERB_AXIS_MASK (1L<<19)
#define SW_ANALYSIS_MEL_AXIS_MASK (1L<<20)
#define SW_ANALYSIS_AXIS_SPECIFIED_MASK (1L<<21)

#define SW_ANALYSIS_NONE 0L

#define SW_ANALYSIS_WAVEFORM (SW_ANALYSIS_TIME_DOMAIN_MASK|SW_ANALYSIS_SCALAR_MASK|SW_ANALYSIS_AMPLITUDE_MASK)
#define SW_ANALYSIS_SPECTRUM (SW_ANALYSIS_FREQUENCY_DOMAIN_MASK|SW_ANALYSIS_AMPLITUDE_MASK)
#define SW_ANALYSIS_POWER_SPECTRUM (SW_ANALYSIS_FREQUENCY_DOMAIN_MASK|SW_ANALYSIS_POWER_MASK)
#define SW_ANALYSIS_SMOOTHED_SPECTRUM (SW_ANALYSIS_SPECTRUM|SW_ANALYSIS_SMOOTHED_MASK)
#define SW_ANALYSIS_PHASE (SW_ANALYSIS_FREQUENCY_DOMAIN_MASK|SW_ANALYSIS_PHASE_MASK)
#define SW_ANALYSIS_UNWRAPPED_PHASE (SW_ANALYSIS_PHASE|SW_ANALYSIS_UNWRAPPED_MASK)
#define SW_ANALYSIS_CEPSTRUM (SW_ANALYSIS_TIME_DOMAIN_MASK|SW_ANALYSIS_CEPSTRUM_MASK)
#define SW_ANALYSIS_GROUP_DELAY (SW_ANALYSIS_FREQUENCY_DOMAIN_MASK|SW_ANALYSIS_GROUP_DELAY_MASK)
#define SW_ANALYSIS_SMOOTHED_GROUP_DELAY (SW_ANALYSIS_GROUP_DELAY|SW_ANALYSIS_SMOOTHED_MASK)
#define SW_ANALYSIS_TD_GROUP_DELAY (SW_ANALYSIS_TIME_DOMAIN_MASK|SW_ANALYSIS_GROUP_DELAY_MASK)
#define SW_ANALYSIS_CQT_SPECTRUM (SW_ANALYSIS_FREQUENCY_DOMAIN_MASK|SW_ANALYSIS_AMPLITUDE_MASK|SW_ANALYSIS_CQT_MASK|SW_ANALYSIS_AXIS_SPECIFIED_MASK)
#define SW_ANALYSIS_ERB_CQT_SPECTRUM (SW_ANALYSIS_CQT_SPECTRUM|SW_ANALYSIS_ERB_AXIS_MASK)

#define SW_ANALYSIS_F0 (SW_ANALYSIS_TIME_DOMAIN_MASK|SW_ANALYSIS_SCALAR_MASK|SW_ANALYSIS_F0_MASK)
#define SW_ANALYSIS_CEPSTRUM_F0 (SW_ANALYSIS_F0|SW_ANALYSIS_CEPSTRUM_MASK)
#define SW_ANALYSIS_STRAIGHT_F0 (SW_ANALYSIS_F0|SW_ANALYSIS_STRAIGHT_MASK)
#define SW_ANALYSIS_POWER (SW_ANALYSIS_TIME_DOMAIN_MASK|SW_ANALYSIS_SCALAR_MASK|SW_ANALYSIS_POWER_MASK)

#define SW_ANALYSIS_APERIODICITY (SW_ANALYSIS_FREQUENCY_DOMAIN_MASK|SW_ANALYSIS_APERIODICITY_MASK)

#define SW_ANALYSIS_STRAIGHT (SW_ANALYSIS_SPECTRUM|SW_ANALYSIS_STRAIGHT_MASK|SW_ANALYSIS_SMOOTHED_MASK)
#define SW_ANALYSIS_STRAIGHT_APERIODICITY (SW_ANALYSIS_APERIODICITY|SW_ANALYSIS_STRAIGHT_MASK|SW_ANALYSIS_SMOOTHED_MASK)
    
typedef enum {
    SW_WINDOW_NONE = -1,
    SW_WINDOW_RECTANGLE = 0,
    SW_WINDOW_HAMMING = 1,
    SW_WINDOW_HANNING = 2,
    SW_WINDOW_BLACKMAN = 3,
    SW_WINDOW_GAUSS = 4,
} swWindowType;

typedef enum {
    SW_ANALYSIS_UNIT_NONE = -1,
    SW_ANALYSIS_UNIT_AMPLITUDE = 0,
    SW_ANALYSIS_UNIT_ABS_AMPLITUDE = 1,
    SW_ANALYSIS_UNIT_POWER,
    SW_ANALYSIS_UNIT_DB,
    SW_ANALYSIS_UNIT_HZ,
    SW_ANALYSIS_UNIT_SEC,
    SW_ANALYSIS_UNIT_RADIAN,
	
    SW_ANALYSIS_UNIT_CUSTOM = 63,
} swAnalysisUnitType;

typedef struct _swAnalysisStringInfo {
    swAnalysisType analysis_type;
    char *name;
    swAnalysisUnitType unit_type;
    spBool default_is_dB;
    double default_min;
    double default_max;
} swAnalysisStringInfo;

typedef struct _swWave *swWave;
typedef struct _swWaveCore *swWaveCore;
    
typedef spBool (*swWaveErrorCallback)(swWave wave, spBool in_thread, int error, swEditType edit_type, void *data);
typedef spBool (*swWaveOptionCallback)(swWave wave, spBool in_thread, spOptions options, void *data);
typedef spBool (*swWavePlayCallback)(swWave wave, spBool in_thread, spLong pos, void *data);
typedef spBool (*swWaveEditCallback)(swWave wave, spBool in_thread, spLong pos, swEditType edit_type, void *data);
typedef spBool (*swWaveEditFinishCallback)(swWave wave, swWave owave, spBool in_thread, swEditType edit_type, void *data);
typedef spBool (*swWaveIoCallback)(swWave wave, spBool in_thread, spLong pos, void *data);
typedef spBool (*swWaveThreadWaitCallback)(swWave wave, void *thread, void *data);

typedef struct _swLabel swLabel;
typedef struct _swLabels *swLabels;

typedef struct _swProcessConfig  swProcessConfig;

struct _swProcessConfig {
    swWave wave;
    swEditType edit_type;
    char file_desc[SP_WAVE_FILE_DESC_SIZE];
    int samp_bit;

    spLong offset;
    spLong length;
    double value;
    double value2;
    double value3;
    spBool flag;
    void *data;
};

typedef struct _swWaveConfig {
    long max_read_length;
    long read_callback_length;
    long normal_read_length;
    long play_read_length;
    long buffer_length;
    int max_num_undo;
    
    spBool thread_safe;
    spBool play_use_audio;
    spBool play_use_wav;
    spBool record_use_pause;
    spBool process_use_thread;
    spBool linear_spectrum;
    spBool normalize_spectrum;
    spBool normalize_spectrum_max;
    spBool float_normalized;
    spBool load_default_label;
    
    long audio_buffer_size;
    char temp_dir[SP_MAX_PATHNAME];
    char audio_driver[SP_MAX_PATHNAME];
    char default_label_suffix[SP_MAX_SETUP_VALUE];
    char default_region_label_suffix[SP_MAX_SETUP_VALUE];

    swTimeFormat label_format;
    swTimeFormat region_label_format;
    
    long sfc_buffer_length;
    double sfc_cutoff;
    double sfc_sidelobe;
    double sfc_transition;
    double sfc_tolerance;
    double sfc_gain;

    long cqt_bins_per_octave;
    double cqt_min_freq;
    spBool cqt_min_freq_based_on_musical_note;
    spBool cqt_use_erb;
    double cqt_block_margin_factor;
    double cqt_block_transition_factor;
    
    swWaveErrorCallback error_func;
    swWaveOptionCallback option_func;
    swWavePlayCallback play_func;
    swWaveEditCallback edit_func;
    swWaveEditFinishCallback edit_finish_func;
    swWaveIoCallback read_func;
    swWaveThreadWaitCallback thread_wait_func;
} swWaveConfigRec, *swWaveConfig;
    
struct _swWave {
    swWaveCore core;
    
    char *plugin_name;
    char *filename;
    char *file_type;
    char *new_plugin_name;
    char *new_filename;
    char *new_file_type;
    int num_channel;
    int selected_channel;	/* -1: not selected */
    int samp_bit;
    double samp_rate;
    double exact_samp_rate;
    unsigned long song_info_mask;
    spSongInfoV2 song_info;
    long num_order;
    spLong offset;		/* offset position in file */
    spLong length;		/* buffer length */
    spLong data_length;		/* data length (!= length) */
    spLong total_length;		/* total data length */
    spLong thin_length;		/* data thinned out length */
    spLong alloc_size;		/* allocated buffer size */
    spBool float_flag;
    spBool orig_flag;
    spBool detail_flag;
    spBool edit_flag;
    spBool order_frequency_flag;
    spBool overflow;
    spBool thread_safe;
    char *data;
    short min;
    short max;
    long lmin;
    long lmax;
    double dmin;
    double dmax;
    spBool range_available;

    spBool log_like_custom_x_axis;
    spDVector custom_x_axis;

    char *peak_filename;
    spLong peak_length;
    spLong peak_thin_length;

    spLong peak_buf_alloc_size;
    spLong peak_buf_length;
    spLong peak_buf_thin_length;
    char *peak_buf;

    spLong buf_alloc_size;
    spLong buf_length;
    spLong buf_load_length;
    char *buf;

    spLong readbuf_size;
    char *readbuf;
    spLong editbuf_length;
    char *editbuf;

    spLong edit_offset;
    spLong edit_length;
    spLong play_offset;
    spLong play_length;
    spLong play_start_offset;
    spBool play_start_offset_updated;
    spBool play_start_sync_pos;
    spBool suspend_play_callback;

    spLong *minindex;
    spLong *maxindex;
    double *minvalue;
    double *maxvalue;
    spLong *peak_minindex;
    spLong *peak_maxindex;
    double *peak_minvalue;
    double *peak_maxvalue;

    swLabels labels;
    
    swWaveConfig config;

    char *edit_name;
    swWave prev_wave;
    swWave next_wave;

#ifdef SW_SUPPORT_MORPHING
    StraightConfig st_config;
    Straight straight;
    StraightFile sf;
#endif
};

struct _swWaveCore {
    char *orig_plugin_name;
    char *orig_filename;
    char *orig_file_type;

    volatile spBool process_finished;
    volatile spBool process_flag;
    volatile spBool play_flag;
    volatile spBool restart_flag;
    volatile spBool in_record_pause;
    volatile spBool record_monitor_flag;
    volatile int last_error;
    void *call_data;

    swAnalysisType analysis_type;
    swAnalysisStringInfo *ana_string_info;
    
    swProcessConfig process_config;

    volatile spBool in_edit_thread;
    volatile spBool in_audio_thread;
    void *edit_thread;
    void *audio_thread;
    void *mutex;
    void *io_mutex;
    void *event;
    void *thread_start_event;
    
    int ref_count;
};

#if defined(MACOS)
#pragma import on
#endif

extern spBool swUpdateAnalysisStringInfo(swWave wave, swAnalysisType analysis_type);
extern char *swGetAnalysisUnitString(swWave wave, spBool brackets_flag);
extern char *swGetAnalysisNameString(swWave wave);
    
extern spBool swIsAnalysisTypeF0(swAnalysisType type);
extern spBool swIsAnalysisTypePower(swAnalysisType type);
extern spBool swIsAnalysisTypeAperiodicity(swAnalysisType type);
extern spBool swIsAnalysisTypeStraight(swAnalysisType type);
extern swAnalysisType swGetAnalysisTypeFromLabel(char *label);
extern char *swGetAnalysisTypeParamLabel(swAnalysisType type);
extern char *swGetAnalysisTypeLabel(swAnalysisType type);
extern swWindowType swGetWindowTypeFromLabel(char *label);
extern char *swGetWindowTypeParamLabel(swWindowType type);

extern spBool swIsWaveNone(swWave wave);
extern spBool swIsWaveFloat(swWave wave);
extern spBool swIsWaveLong(swWave wave);
extern spBool swIsWaveOverflow(swWave wave);
extern spBool swIsWavePlaying(swWave wave);
extern spBool swIsWaveProcessing(swWave wave);
extern spBool swIsWaveProcessFinished(swWave wave);
extern int swGetWaveLastError(swWave wave);
extern spBool swIsWaveEdited(swWave wave);
extern spBool swIsWaveThreadSafe(swWave wave);
extern spBool swIsWavePeakAvailable(swWave wave);
extern spBool swNeedProcessBlock(swWave src_wave, swWave wave);

extern double swGetWaveSampleRate(swWave wave);
extern double swGetWaveExactSampleRate(swWave wave);
extern int swGetWaveSampleBit(swWave wave);
extern int swGetWaveNumChannel(swWave wave);
extern int swGetWaveSelectedChannel(swWave wave);

extern spLong swGetWaveOffset(swWave wave);
extern spLong swGetWaveLength(swWave wave);
extern spLong swGetWaveEditOffset(swWave wave);
extern spLong swGetWaveEditLength(swWave wave);
extern spBool swSetWaveTotalLength(swWave wave, spLong length);
extern spLong swGetWaveTotalLength(swWave wave);
extern spLong swGetWaveThinLength(swWave wave);
extern spBool swSetWavePlayRegion(swWave wave, spLong offset, spLong length);

extern char *swGetWaveFileName(swWave wave);
extern char *swGetWaveOriginalFileName(swWave wave);
extern char *swGetWaveFileType(swWave wave);
extern char *swGetWavePluginName(swWave wave);
extern spSongInfoV2 *swGetWaveSongInfo(swWave wave);
extern swLabels swGetWaveLabels(swWave wave);
extern spBool swGetDetailFlag(swWave wave);
extern spBool swSetDetailFlag(swWave wave, spBool detail_flag);
extern char *swGetEditName(swWave wave);
extern spBool swSetEditName(swWave wave, char *edit_name);
extern swWaveConfig swGetWaveConfig(swWave wave);

extern spLong swConvertLengthToByte(swWave wave, spLong length);
extern spLong swConvertByteToLength(swWave wave, spLong size);
extern spLong swLoadIntoBuffer(spPlugin *plugin, swWave wave, int channel, spLong length, char *buf);
    

extern spBool swSetTempDir(swWaveConfig config, char *dir);
extern void swGetTempFile(swWaveConfig config, char *suffix, char *filename);

extern spBool swIsOverflow(int samp_bit, double value);
extern double swGetClipValue(int samp_bit);
extern double swGetLimitValue(int samp_bit);
extern spBool swAllocData(swWave wave, spLong alloc_length);
extern spBool swAllocBuffer(swWave wave, spLong alloc_length);
extern spBool swInitWaveRange(swWave wave);
extern swWave swInitWave(swWaveConfig config, swWave ref_wave, const char *filename, const char *plugin_name, const char *file_type,
			 int samp_bit, int num_channel, double samp_rate, long num_order,
			 spBool orig_flag, spBool detail_flag);
extern void _swFreeWave(swWave wave, spBool lock_flag);
extern void _swDestroyWave(swWave wave);
extern unsigned long swGetSongInfoMask(swWave wave);
extern swWave swGetWave(swWaveConfig config, swWave ref_wave, const char *filename, const char *plugin_name, const char *file_type,
			int samp_bit, int num_channel, double samp_rate, long num_order,
			spBool orig_flag, spBool detail_flag, spBool callback_flag, spPluginError *error);
extern spPlugin *swOpenWave(swWave wave, char *mode);
extern spLong swReadWave(swWave wave, spBool in_thread, spLong offset, spLong length);
extern spLong swReadTotalWave(swWave wave, spBool in_thread);
extern spLong swReadBuffer(spPlugin *plugin, swWave wave, int channel, spLong length);
extern spLong swWriteData(spPlugin *plugin, swWave wave);
extern spBool swWriteWave(char *filename, char *plugin_name, char *file_type, char *file_desc,
			  swWave wave, spBool callback_flag, spBool use_thread);
extern swWave swGetCroppedWave(char *filename, char *plugin_name, char *file_type, char *file_desc,
			       swEditType edit_type, swWave wave, spLong offset, spLong length,
			       spBool orig_flag, spBool in_thread);
extern spBool swSetPrevWave(swWave wave, swWave prev_wave, spBool lock_flag);
extern spBool swCanUndoWave(swWave wave);
extern spBool swCanRedoWave(swWave wave);
extern spBool swUndoWave(swWave *wave);
extern spBool swRedoWave(swWave *wave);
extern spBool swIsWaveRangeAvailable(swWave wave);
extern spBool swSetWaveRange(swWave wave, double min, double max);
extern double swGetWaveMax(swWave wave);
extern double swGetWaveMin(swWave wave);
extern double swGetWaveAbsMax(swWave wave);
extern spBool swGetBufferRange(swWave wave, long channel, long order, double *minp, double *maxp,
			       spLong *min_offsetp, spLong *max_offsetp);
extern spBool swGetWaveBufferData(swWave wave, long channel, long order, spLong pos, spLong buf_length, char *buf, double *value);
extern spBool swSetWaveBufferData(swWave wave, long channel, long order, spLong pos, spLong buf_length, char *buf, double value);
extern spBool swGetWaveData(swWave wave, long channel, long order, spLong pos, double *value);
extern spBool swSetWaveData(swWave wave, long channel, long order, spLong pos, double value);
extern double swGetBufferData(swWave wave, long channel, long order, spLong pos);
extern spBool swSetBufferData(swWave wave, long channel, long order, spLong pos, double value);
extern spBool swGetPeakData(swWave wave, long channel, long order, spLong pos, double *value);

extern void *swCreateProcessThread(int priority, spThreadFunc func, swProcessConfig *config);
extern spBool swLockMutex(swWave wave);
extern spBool swUnlockMutex(swWave wave);
extern spBool swWaitMutex(swWave wave);
extern spBool swSetEvent(swWave wave);
extern spBool swResetEvent(swWave wave);
extern spBool swWaitEvent(swWave wave);
extern spBool swSetThreadStartEvent(swWave wave);
extern spBool swResetThreadStartEvent(swWave wave);
extern spBool swWaitThreadStartEvent(swWave wave);
    
extern spBool swGenerateWaveform(char *filename, char *plugin_name, char *file_type, char *file_desc,
				 swWave wave, swEditType edit_type, spLong delay, spLong duration,
				 double f0, double initial_phase, double gain, spBool use_thread);
extern swWave swExtractWave(swWave wave, spLong offset, spLong length);
extern swWave swExtractAndWriteWave(swWave wave, spLong offset, spLong length,
				    char *filename, char *plugin_name, char *file_type, char *file_desc);
extern swWave swCropWave(swWave wave, spLong offset, spLong length);
extern swWave swDeleteWave(swWave wave, spLong offset, spLong length);
extern swWave swEraseWave(swWave wave, spLong offset, spLong length);
extern swWave swAmplifyWave(swWave wave, spLong offset, spLong length, double value);
extern swWave swFadeInWave(swWave wave, spLong offset, spLong length);
extern swWave swFadeOutWave(swWave wave, spLong offset, spLong length);
extern swWave swSwapWaveChannel(swWave wave, spLong offset, spLong length);
extern swWave swConvertWaveBit(swWave wave, int samp_bit);
extern swWave swConvertWaveSampFreq(swWave wave, double samp_rate);
extern swWave swFilteringWave(swWave wave, spDVector filter);
extern swWave swMonauralizeWave(swWave wave);
extern swWave swChangeWaveValue(swWave wave, int channel, spLong offset, double value);

extern swWave swPasteWave(swWave wave, swWave src_wave, spLong offset, int channel);
extern swWave swMixWave(swWave wave, swWave src_wave, spLong offset, int channel);
extern swWave swInsertWave(swWave wave, swWave src_wave, spLong offset, int channel);
extern swWave swInsertPauseWave(swWave wave, spLong offset, spLong pause_length);
extern swWave swRecordWave(swWave wave, spLong offset, spLong length);
extern swWave swReplaceWave(swWave wave, swWave src_wave, spLong offset, spLong length);

extern spBool swSetWaveErrorCallback(swWaveConfig config, swWaveErrorCallback func);
extern spBool swSetWaveOptionCallback(swWaveConfig config, swWaveOptionCallback func);
extern spBool swSetWavePlayCallback(swWaveConfig config, swWavePlayCallback func);
extern spBool swSetWaveReadCallback(swWaveConfig config, swWaveIoCallback func);
extern spBool swSetWaveEditCallback(swWaveConfig config, swWaveEditCallback func);
extern spBool swSetWaveEditFinishCallback(swWaveConfig config, swWaveEditFinishCallback func);
extern spBool swSetWaveThreadWaitCallback(swWaveConfig config, swWaveThreadWaitCallback func);
extern spBool swSetWaveCallbackData(swWave wave, void *data);
extern spBool swProcessStop(swWave wave);

extern spBool swNormalizeSpectrum(swWaveConfig config, swAnalysisType analysis_type,
                                  spDVector data, double min_value_for_dB, double amp_max, spBool force_dB);
extern long swAnalysisWaveFrame(void *anarec, swWaveConfig config, swAnalysisType analysis_type,
                                spDVector ana_window, spDVector data, long current_framel, long lifl, double samp_rate,
                                double min_value_for_dB, double amp_max, spBool force_dB_flag, spDVector odata);
extern swWave swAnalysisData(swWaveConfig config, double *data, spLong length,
			     swAnalysisType analysis_type, swWindowType window_type,
			     double samp_rate, long ana_internal_length, double lifterm,
			     spBool dB_flag, spBool detail_flag);
extern swWave swAnalysisWave(swWave iwave, spLong offset, spLong length,
			     swAnalysisType analysis_type, swWindowType window_type,
			     long fftl, double lifterm);
extern swWave swGetSpectrogram(swWave wave, spLong offset, spLong length, long framel, long shiftl,
			       swAnalysisType analysis_type, swWindowType window_type,
			       long fftl, double lifterm, spBool *interrupted);

#define swFreeWave(wave, flag) {_swFreeWave(wave, flag);(wave)=NULL;}
#define swDestroyWave(wave) {_swDestroyWave(wave);(wave)=NULL;}

#if defined(MACOS)
#pragma import off
#endif

#define SP_WAVE_PLAY_DUMMY_TIME (0.5) /* [ms] */

#if 0
#ifndef _WIN32
#define ESPS
#endif
#endif
    
#define SP_WAVE_FORMAT_UNKNOWN_LABEL "Unknown"
#define SP_WAVE_FORMAT_LITTLE_LABEL "Little Endian"
#define SP_WAVE_FORMAT_BIG_LABEL "Big Endian"
#define SP_WAVE_FORMAT_ULAW_LABEL "U-law"
#define SP_WAVE_FORMAT_ALAW_LABEL "A-law"
#define SP_WAVE_FORMAT_TEXT_LABEL "Text"
#define SP_WAVE_FORMAT_TEXT_TIME_LABEL "Text with Time"
#define SP_WAVE_FORMAT_TEXT_FREQ_LABEL "Text with Frequency"
#define SP_WAVE_FORMAT_WAV_LABEL "Wav"
#ifdef ESPS
#define SP_WAVE_FORMAT_ESPS_LABEL "ESPS"
#endif
#define SP_WAVE_FORMAT_RAW_LABEL "Raw"
#define SP_WAVE_FORMAT_SWAP_LABEL "Swap Raw"

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration */
#endif

#endif /* __SWWAVE_H */
