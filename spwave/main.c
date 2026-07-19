/*
 *	main.c
 *
 *	Last modified: <2025-04-27 22:11:17 hideki>
 */

#define HIGH_LEVEL_EVENT

#include <stdio.h>
#include <stdlib.h>

#include <sp/spBaseLib.h>
#include <sp/spAudioLib.h>
#include <sp/spComponentLib.h>
#include <sp/spComponentMain.h>

#include "swWindow.h"
#include "swDialog.h"

#include "swStringDefs.h"
#include "swStringDefs_C.h"
#if defined(_WIN32) || defined(SP_SUPPORT_UTF8_STRING)
#include "swStringDefs_UTF8JP.h"
#endif
#if (defined(_WIN32) && !defined(__CYGWIN32__)) || defined(MACOS9)
#include "swStringDefs_SJIS.h"
#else
#include "swStringDefs_EUC.h"
#endif

static spBool sw_is_region_label = SP_FALSE;
static char sw_label_file[SP_MAX_PATHNAME] = "";

static double sw_display_offset_s = -1.0;
static double sw_display_length_s = -1.0;

static spBool toplevelCB(spTopLevel toplevel, spTopLevelCallbackReason reason, void *data)
{
    spDebug(10, "toplevelCB", "reason = %d\n", reason);

    if (reason == SP_TL_CR_PERMISSION_RATIONALE_ASKED) {
        unsigned long asked_permissions = 0L;
        
        spGetTopLevelParams(toplevel,
                            SppAskedPermissions, &asked_permissions, 
                            NULL);

        spDebug(10, "toplevelCB", "asked_permissions = %lx\n", asked_permissions);
        
        if (asked_permissions) {
            if (spCreateMessageBox(NULL,
                                   NULL,
                                   "Permission to record audio is required.\nProceed?",
                                   SppDialogType, SP_QUESTION_DIALOG,
                                   SppMessageBoxButtonType, SP_MB_YES_NO,
                                   NULL) == SP_DR_NO) {
                return SP_FALSE;
            }
        }
    } else if (reason == SP_TL_CR_PERMISSION_CHANGED) {
        unsigned long accepted_permissions = 0L;
        unsigned long disabled_permissions = 0L;
        
        spGetTopLevelParams(toplevel,
                            SppAcceptedPermissions, &accepted_permissions, 
                            SppDisabledPermissions, &disabled_permissions, 
                            NULL);

        spDebug(10, "toplevelCB", "accepted_permissions = %lx, disabled_permissions = %lx\n",
                accepted_permissions, disabled_permissions);

        if (!(accepted_permissions & SP_PERMISSION_RECORD_AUDIO)) {
            spDisplayError(NULL, NULL, "Permission to record audio is required.\nQuit application.");
            return SP_FALSE;
        }
    }
    
    return SP_TRUE;
}

static swWaveConfigRec sw_wave_config =
{
    16384,			/* max_read_length */
    262144/*65536*/,		/* read_callback_length */
    8192,			/* normal_read_length */
    4096,			/* play_read_length */
    8192,			/* buffer_length */
    32768,			/* max_num_undo */

    SP_FALSE,			/* thread_safe */
#ifdef SP_SUPPORT_AUDIO
    SP_TRUE,			/* play_use_audio */
#else
    SP_FALSE,			/* play_use_audio */
#endif
#ifdef _WIN32
    SP_TRUE,			/* play_use_wav */
#else
    SP_FALSE,			/* play_use_wav */
#endif
    SP_FALSE,			/* record_use_pause */
    SP_TRUE,			/* process_use_thread */
    SP_FALSE,			/* linear_spectrum */
    SP_FALSE,			/* normalize_spectrum */
    SP_TRUE,			/* normalize_spectrum_max */
    SP_FALSE,			/* float_normalized */
    SP_TRUE,			/* use_default_label */
	
    SW_AUDIO_BUFFER_SIZE,	/* audio_buffer_size */
    "",				/* temp_dir */
    "",				/* audio_driver */
    "",				/* default_label_suffix */
    "",				/* default_region_label_suffix */
    
    SW_TIME_FORMAT_SEC,		/* label_format */
    SW_TIME_FORMAT_SEC,		/* region_label_format */
    
    32768,			/* sfc_buffer_length */
    0.95,			/* sfc_cutoff */
    50.0,			/* sfc_sidelobe [dB] */
    0.05,			/* sfc_transition */
    2.5,			/* sfc_tolerance [%] */
    1.0,			/* sfc_gain */
    
    /*24*/48,			/* cqt_bins_per_octave */
    SP_CQT_DEFAULT_MIN_FREQ,	/* cqt_min_freq */
    SP_TRUE,			/* cqt_min_freq_based_on_musical_note */
    SP_FALSE,			/* cqt_use_erb */
    1.0,			/* cqt_block_margin_factor */
    8.0,			/* cqt_block_transition_factor */
    
    NULL,			/* error_func */
    NULL, 			/* option_func */
    NULL, 			/* play_func */
    NULL, 			/* edit_func */
    NULL, 			/* edit_finish_func */
    NULL,			/* read_func */
    NULL,			/* thread_wait_func */
};

static int debug_level = -1;
static spBool help_flag;
static swConfigRec sw_config =
{
    SP_FALSE,			/* help_flag */
    SP_FALSE,			/* prev_flag */
    SP_FALSE,			/* use_def_format */
    SP_FALSE,			/* use_lowlevel_thread */
    SP_FALSE,			/* use_play_command */
    SP_FALSE,			/* use_loop_play */
    SP_FALSE,			/* use_sync_play */
    SP_TRUE,			/* play_draw */
    SP_FALSE,			/* pause_cursor */
    SP_FALSE,			/* alt_ctrl_swap */
    SP_FALSE,			/* scroll_left_by_wheel_down */
    SP_FALSE,			/* no_tool_bar */
    SP_FALSE,			/* draw_detail */
    SP_FALSE,			/* log_frequency_axis */
    SP_TRUE,			/* display_overview */
    SP_TRUE,			/* display_meter */
    SP_TRUE,			/* draw_selection_times */
    SP_TRUE,			/* draw_selection_length */
    SP_FALSE,			/* link_region_label */
    SP_TRUE,			/* erase_label_prompt */
    SP_FALSE,			/* show_dialog_in_first_label_save */
    SP_TRUE,			/* add_default_label_suffix_for_save_as */
    SP_FALSE,			/* display_info_area */
    SP_FALSE,			/* display_freq_info_area */
    SP_FALSE,			/* analysis_new_window */
    SP_FALSE,			/* draw_piano_keys_for_specgram */
    SP_FALSE,			/* draw_piano_keys_for_spectrum */
    SP_FALSE,			/* vertical_piano_keys_right */
    SP_FALSE,			/* horizontal_piano_keys_top */

    SP_TRUE,			/* specgram_simplified */
    SP_FALSE,			/* specgram_gray_scale */
    
    SP_TRUE,			/* scale_flag */
    SP_TRUE,			/* grid_flag */
    SP_TRUE,			/* zero_flag */
    
    SW_TIME_DATA,		/* data_type */
    SW_ANALYSIS_SPECTRUM,	/* analysis_type */
    SW_ANALYSIS_CQT_SPECTRUM,	/* cqt_type */
    SW_WINDOW_HAMMING,		/* window_type */
    0,				/* num_channel */
    1,				/* def_num_channel */
    -1,				/* head_size */
    0,				/* def_head_size */
    0,				/* samp_bit */
    16,				/* def_samp_bit */
    1,				/* num_order */
    SW_AUDIO_BUFFER_SIZE,	/* audio_buffer_size */
    
    SW_WINDOW_WIDTH,		/* width */
    SW_WINDOW_HEIGHT,		/* height */
    400,			/* print_width */
    350,			/* print_height */
    SW_FREQ_WINDOW_WIDTH,	/* freq_width */
    SW_FREQ_WINDOW_HEIGHT,	/* freq_height */
    50,				/* overview_height */
    340,			/* info_area_width */
    60,				/* meter_range */
    80,				/* specgram_range */
    -10,			/* specgram_limit_threshold */
    256,			/* fftl */
    65536,			/* max_fftl */
    4.0,			/* lifterm */
    8.0,			/* shiftm */
    32.0,			/* framem */
    8.0,			/* wide_framem */
    51.1,			/* narrow_framem */
    0.0,			/* samp_rate */
    8000.0,			/* def_samp_rate */

    3,                          /* mid_C_octave_index */
    
    SP_TRUE,			/* pause_play_sync_pos */
    3.0,			/* pause_play_step_time */
        
    "",				/* data_type_string */
    "",				/* analysis_type_string */
    "",				/* window_type_string */
    "",				/* format_string */
    "",				/* def_format_string */
    "",				/* play_command */
    "",				/* browser_command */
    "",				/* doc_dir */
    "",				/* setup_file */
    "",				/* temp_dir */

    SP_TRUE,			/* autosave_use_region */
    SP_FALSE,			/* autosave_label_fullpath */
    SP_FALSE,			/* autosave_by_drop */
    0,				/* autosave_id_type */
    "",				/* autosave_dir */
    "",				/* autosave_id_prefix */
    "",				/* autosave_label_suffix */

   SW_SBL_NAMING_LABEL,		/* sbl_naming_rule */
   SP_FALSE,			/* sbl_index_from_1st */
   SP_FALSE,			/* sbl_illegal_char_to_space */
   SP_FALSE,			/* sbl_data_as_filename */
   SP_FALSE,			/* sbl_no_overwrite_prompt */
   SP_FALSE,			/* sbl_create_window */
    "",				/* sbl_naming_custom_format */
    "",				/* sbl_naming_repetition_suffix_format */
	
    "",				/* wave_fg */
    "",				/* wave_fg */
    "",				/* pointer_color */
    "",				/* string_color */
    "",				/* label_color */
    "",				/* scale_color */
    "",				/* canvas_font */

    SW_TIME_FORMAT_SEPARATED_SEC/*SW_TIME_FORMAT_SEC*/,		/* time_format */
    SW_FREQ_FORMAT_HZ,		/* freq_format */

    SP_FALSE,			/* percent_amplitude */
    
    SP_FALSE,			/* format_specified */
    
    &sw_wave_config,		/* wave_config */
    NULL,			/* toplevel */
};

static spOption sw_option[] = {
    {"-f", "-freq", "sampling frequency for raw data [Hz]", NULL,
	 SP_TYPE_DOUBLE, &sw_config.samp_rate, NULL},
    {NULL, NULL, "default sampling frequency [Hz]", "samp_freq",
	 SP_TYPE_DOUBLE, &sw_config.def_samp_rate, "8000.0"},
    {"-c", "-channel", "number of channels for raw data", NULL,
	 SP_TYPE_INT, &sw_config.num_channel, NULL},
    {NULL, NULL, "default number of channels", "num_channel",
	 SP_TYPE_INT, &sw_config.def_num_channel, "1"},
    {"-b", "-bit", "bits per sample for raw data", NULL,
	 SP_TYPE_INT, &sw_config.samp_bit, NULL},
    {NULL, NULL, "default bits per sample", "bits_per_sample",
	 SP_TYPE_INT, &sw_config.def_samp_bit, "16"},
    {"-F", "-format", "file format for raw data (raw|swap|little|big|text)", NULL,
	 SP_TYPE_STRING_A, sw_config.format_string, NULL},
    {NULL, NULL, "default file format (raw|swap|little|big|text)", "file_format",
	 SP_TYPE_STRING_A, sw_config.def_format_string, "raw"},
    {"-order", NULL, "number of order for raw data", NULL,
	 SP_TYPE_INT, &sw_config.num_order, NULL},
    {"-offset", NULL, "initial offset time for display [s]", NULL,
         SP_TYPE_DOUBLE, &sw_display_offset_s, NULL},
    {"-length", NULL, "initial length for display [s]", NULL,
         SP_TYPE_DOUBLE, &sw_display_length_s, NULL},
    {NULL, NULL, "mid-C octave index for musical notation", "mid_C_octave_index",
	 SP_TYPE_INT, &sw_config.mid_C_octave_index, "3"},
#if 0
    {"-head", "-header", "header length for raw data", NULL,
	 SP_TYPE_INT, &sw_config.head_size, NULL},
    {NULL, NULL, "default header length", "header_length",
	 SP_TYPE_INT, &sw_config.def_head_size, "0"},
#endif
    {"-play", NULL, "sound play command", "play_command",
	 SP_TYPE_STRING_A, sw_config.play_command, ""},
#ifdef SP_SUPPORT_AUDIO
    {"-useplay", NULL, "use play command forcedly", "use_play",
	 SP_TYPE_BOOLEAN, &sw_config.use_play_command, SP_FALSE_STRING},
    {NULL, NULL, "use loop play initially", "use_loop_play",
	 SP_TYPE_BOOLEAN, &sw_config.use_loop_play, SP_FALSE_STRING},
    {NULL, NULL, "use synchronous play initially", "use_sync_play",
	 SP_TYPE_BOOLEAN, &sw_config.use_sync_play, SP_FALSE_STRING},
    {NULL, NULL, "draw cursor on playing", "play_draw",
	 SP_TYPE_BOOLEAN, &sw_config.play_draw, SP_TRUE_STRING},
    {"-buf", NULL, "audio buffer size", "audio_buffer_size",
	 SP_TYPE_INT, &sw_config.audio_buffer_size, NULL},
    {"-driver", NULL, "audio driver name", "audio_driver",
         SP_TYPE_STRING_A, sw_wave_config.audio_driver, NULL},
#endif
    {NULL, NULL, "use low level thread", "use_lowlevel_thread",
	 SP_TYPE_BOOLEAN, &sw_config.use_lowlevel_thread,
#if defined(MACOS9)
         SP_FALSE_STRING
#else
         SP_TRUE_STRING
#endif
    },
    {NULL, NULL, "use thread for processing", "use_thread",
	 SP_TYPE_BOOLEAN, &sw_wave_config.process_use_thread, SP_TRUE_STRING},
    {NULL, NULL, "pause cursor initially", "pause_cursor",
	 SP_TYPE_BOOLEAN, &sw_config.pause_cursor, SP_FALSE_STRING},
    {"-tf", "-tformat", "time format index (0-3)", "time_format",
	 SP_TYPE_ENUM, &sw_config.time_format, /*"0"*/"3"},
    {NULL, NULL, "display amplitude in percent", "percent_amplitude",
	 SP_TYPE_BOOLEAN, &sw_config.percent_amplitude, SP_FALSE_STRING},
    {"-label", NULL, "label file for first input file", NULL,
         SP_TYPE_STRING_A, sw_label_file, ""},
    {"-reg", NULL, "label file specified by \"-label\" is region label", NULL,
	 SP_TYPE_BOOLEAN, &sw_is_region_label, SP_FALSE_STRING},
    {"-lf", "-lformat", "label format index (0-12)", "label_format",
	 SP_TYPE_ENUM, &sw_wave_config.label_format, "0"},
    {"-rf", "-rformat", "region label format index (0-11)", "region_label_format",
	 SP_TYPE_ENUM, &sw_wave_config.region_label_format, "0"},
    {NULL, NULL, "load default label", "load_default_label",
	 SP_TYPE_BOOLEAN, &sw_wave_config.load_default_label, SP_TRUE_STRING},
    {NULL, NULL, "default label suffix", "default_label_suffix",
	 SP_TYPE_STRING_A, sw_wave_config.default_label_suffix, ".txt"},
    {NULL, NULL, "default region label suffix", "default_region_label_suffix",
	 SP_TYPE_STRING_A, sw_wave_config.default_region_label_suffix, ""},
#ifdef SW_SUPPORT_AUTOSAVE
    {NULL, NULL, "write fullpaths into autosave label files", "autosave_label_fullpath",
	 SP_TYPE_BOOLEAN, &sw_config.autosave_label_fullpath, SP_FALSE_STRING},
    {NULL, NULL, "autosave by drag & drop", "autosave_by_drop",
	 SP_TYPE_BOOLEAN, &sw_config.autosave_by_drop, SP_FALSE_STRING},
    {NULL, NULL, "autosave ID type", "autosave_id_type",
	 SP_TYPE_INT, &sw_config.autosave_id_type, "0"},
    {NULL, NULL, "autosave directory", "autosave_dir",
	 SP_TYPE_STRING_A, sw_config.autosave_dir, ""},
    {NULL, NULL, "autosave ID prefix", "autosave_id_prefix",
	 SP_TYPE_STRING_A, sw_config.autosave_id_prefix, "-"},
    {NULL, NULL, "autosave label suffix", "autosave_label_suffix",
	 SP_TYPE_STRING_A, sw_config.autosave_label_suffix, "_cut.txt"},
    {NULL, NULL, "naming rule of save-by-label", "sbl_naming_rule",
	 SP_TYPE_INT, &sw_config.sbl_naming_rule, "0"},
    {NULL, NULL, "turn on index-from-1st setting of save-by-label", "sbl_index_from_1st",
	 SP_TYPE_BOOLEAN, &sw_config.sbl_naming_rule, SP_FALSE_STRING},
    {NULL, NULL, "turn on illegal-char-to-space setting of save-by-label", "sbl_illegal_char_to_space",
	 SP_TYPE_BOOLEAN, &sw_config.sbl_illegal_char_to_space, SP_FALSE_STRING},
    {NULL, NULL, "turn on data-as-filename setting of save-by-label", "sbl_data_as_filename",
	 SP_TYPE_BOOLEAN, &sw_config.sbl_data_as_filename, SP_FALSE_STRING},
    {NULL, NULL, "turn on no-overwrite-prompt setting of save-by-label", "sbl_no_overwrite_prompt",
	 SP_TYPE_BOOLEAN, &sw_config.sbl_no_overwrite_prompt, SP_FALSE_STRING},
    {NULL, NULL, "turn on create-window setting of save-by-label", "sbl_create_window",
	 SP_TYPE_BOOLEAN, &sw_config.sbl_create_window, SP_FALSE_STRING},
    {NULL, NULL, "custom format for naming rule of save-by-label", "sbl_naming_custom_format",
         SP_TYPE_STRING_A, sw_config.sbl_naming_custom_format, ""},
    {NULL, NULL, "format of suffix if name duplication exists in save-by-label", "sbl_naming_repetition_suffix_format",
         SP_TYPE_STRING_A, sw_config.sbl_naming_repetition_suffix_format, ""},
#endif
    {"-temp", NULL, "temporary directory", "temp_dir",
	 SP_TYPE_STRING_A, sw_config.temp_dir, ""},
    {"-wavefg", NULL, "wave foreground color", "wave_fg",
	 SP_TYPE_STRING_A, sw_config.wave_fg, "black"},
    {"-wavebg", NULL, "wave background color", "wave_bg",
	 SP_TYPE_STRING_A, sw_config.wave_bg, "white"},
    {"-pointfg", NULL, "pointer foreground color", "point_fg",
	 SP_TYPE_STRING_A, sw_config.pointer_color, "red"},
    {"-stringfg", NULL, "string foreground color", "string_fg",
	 SP_TYPE_STRING_A, sw_config.string_color, "red"},
    {"-labelfg", NULL, "label foreground color", "label_fg",
	 SP_TYPE_STRING_A, sw_config.label_color, "blue"},
    {"-scalefg", NULL, "scale foreground color", "scale_fg",
	 SP_TYPE_STRING_A, sw_config.scale_color, "grey50"},
    {NULL, NULL, "font for drawing canvas", "canvas_font",
	 SP_TYPE_STRING_A, sw_config.canvas_font, NULL},
    {"-width", NULL, "window width", "window_width",
	 SP_TYPE_INT, &sw_config.width, "800"},
    {"-height", NULL, "window height", "window_height",
	 SP_TYPE_INT, &sw_config.height, "240"},
#ifdef SW_SUPPORT_PRINT
    {NULL, NULL, "window width", "print_width",
	 SP_TYPE_INT, &sw_config.print_width, "500"},
    {NULL, NULL, "window height", "print_height",
	 SP_TYPE_INT, &sw_config.print_height, "350"},
#endif
#ifdef SW_USE_ANALYSIS
    {"-fwidth", NULL, "window width for frequency data", "freq_width",
	 SP_TYPE_INT, &sw_config.freq_width, "400"},
    {"-fheight", NULL, "window height for frequency data", "freq_height",
	 SP_TYPE_INT, &sw_config.freq_height, "300"},
    {NULL, NULL, "overview area height", "overview_height",
	 SP_TYPE_INT, &sw_config.overview_height, "50"},
    {NULL, NULL, "information area width", "info_area_width",
	 SP_TYPE_INT, &sw_config.info_area_width, "340"},
    
    {NULL, NULL, "default analysis type", "analysis_type",
	 SP_TYPE_STRING_A, sw_config.analysis_type_string, "spectrum"},
    {NULL, NULL, "default window type", "window_type",
	 SP_TYPE_STRING_A, sw_config.window_type_string, "hamming"},
    {NULL, NULL, "minimum FFT length", "fft_length",
	 SP_TYPE_LONG, &sw_config.fftl, "256"},
    {NULL, NULL, "minimum FFT length", "max_fft_length",
	 SP_TYPE_LONG, &sw_config.max_fftl, "65536"},
    {NULL, NULL, "lifter length for cepstrum analysis [ms]", "lifter",
	 SP_TYPE_DOUBLE, &sw_config.lifterm, "2.0"},
    {NULL, NULL, "frame length for wide band analysis [ms]", "wide_frame",
	 SP_TYPE_DOUBLE, &sw_config.wide_framem, "3.33"},
    {NULL, NULL, "frame length for narrow band analysis [ms]", "narrow_frame",
	 SP_TYPE_DOUBLE, &sw_config.narrow_framem, "22.2"},
    {NULL, NULL, "default frame shift [ms]", "shift",
	 SP_TYPE_DOUBLE, &sw_config.shiftm, "8.0"},
    {NULL, NULL, "default frame length [ms]", "frame",
	 SP_TYPE_DOUBLE, &sw_config.framem, "32.0"},
    {NULL, NULL, "create always new window on analysis", "analysis_new_window",
	 SP_TYPE_BOOLEAN, &sw_config.analysis_new_window, SP_FALSE_STRING},
    {NULL, NULL, "use linear spectrum (not use decibel)", "linear_spectrum",
	 SP_TYPE_BOOLEAN, &sw_wave_config.linear_spectrum, SP_FALSE_STRING},
    {NULL, NULL, "normalize spectrum", "normalize_spectrum",
         SP_TYPE_BOOLEAN, &sw_wave_config.normalize_spectrum, SP_FALSE_STRING},
    {NULL, NULL, "normalize spectrum with maximum amplitude of waveform", "normalize_spectrum_max",
         SP_TYPE_BOOLEAN, &sw_wave_config.normalize_spectrum_max, SP_TRUE_STRING},
#if defined(SW_SUPPORT_CQT_ANALYSIS)
    {NULL, NULL, "bins per octave for CQT", "cqt_bins_per_octave",
         SP_TYPE_LONG, &sw_wave_config.cqt_bins_per_octave, NULL},
    {NULL, NULL, "minimum frequency for CQT", "cqt_min_freq",
         SP_TYPE_DOUBLE, &sw_wave_config.cqt_min_freq, NULL},
    {NULL, NULL, "use a minimum frequency based on musical note", "cqt_min_freq_based_on_musical_note",
         SP_TYPE_BOOLEAN, &sw_wave_config.cqt_min_freq_based_on_musical_note, SP_TRUE_STRING},
    {NULL, NULL, "use ERB-based frequency warping", "cqt_use_erb",
         SP_TYPE_BOOLEAN, &sw_wave_config.cqt_use_erb, SP_FALSE_STRING},
    {NULL, NULL, "margin factor of block length for CQT", "cqt_block_margin_factor",
         SP_TYPE_DOUBLE, &sw_wave_config.cqt_block_margin_factor, NULL},
    {NULL, NULL, "transition factor of block length for CQT", "cqt_block_transition_factor",
         SP_TYPE_DOUBLE, &sw_wave_config.cqt_block_transition_factor, NULL},
#endif
#endif /* SW_USE_ANALYSIS */
    {NULL, NULL, "floating value is normalized from -1.0 to 1.0", "float_normalized",
	 SP_TYPE_BOOLEAN, &sw_wave_config.float_normalized, SP_FALSE_STRING},
    {NULL, NULL, "draw simplified spectrogram", "spectrogram_simplified",
         SP_TYPE_BOOLEAN, &sw_config.specgram_simplified, SP_TRUE_STRING},
    {NULL, NULL, "draw spectrogram with gray scale", "spectrogram_gray_scale",
         SP_TYPE_BOOLEAN, &sw_config.specgram_gray_scale, SP_FALSE_STRING},
    {NULL, NULL, "spectrogram range", "spectrogram_range",
	 SP_TYPE_INT, &sw_config.specgram_range, "80"},
    {NULL, NULL, "spectrogram limting threshold (must have minus value or 0)", "spectrogram_limit_threshold",
	 SP_TYPE_INT, &sw_config.specgram_limit_threshold, "-10"},
    {NULL, NULL, "step time for rewind and forward play in pause mode [s]", "pause_play_step_time",
	 SP_TYPE_DOUBLE, &sw_config.pause_play_step_time, "3.0"},
    {NULL, NULL, "turn on synchronization of play start position to current position", "pause_play_sync_pos",
	 SP_TYPE_BOOLEAN, &sw_config.pause_play_sync_pos, SP_TRUE_STRING},
    
    {NULL, NULL, "buffer length for sampling frequency conversion", "sfc_buffer_length", 
	 SP_TYPE_LONG, &sw_wave_config.sfc_buffer_length, "32768"},
    {NULL, NULL, "normalized cutoff frequency (nyquist = 1)", "cutoff", 
	 SP_TYPE_DOUBLE, &sw_wave_config.sfc_cutoff, "0.95"},
    {NULL, NULL, "height of sidelobe [dB]", "sidelobe", 
         SP_TYPE_DOUBLE, &sw_wave_config.sfc_sidelobe, /*"60.0"*/"80.0"},
    {NULL, NULL, "normalized transition width", "transition", 
	 SP_TYPE_DOUBLE, &sw_wave_config.sfc_transition, "0.05"},
    {NULL, NULL, "tolerance [%]", "tolerance", 
	 SP_TYPE_DOUBLE, &sw_wave_config.sfc_tolerance, "2.5"},
    
    {"-usedef", NULL, "use default format when opening file", "use_default",
	 SP_TYPE_BOOLEAN, &sw_config.use_def_format, SP_FALSE_STRING},
    {"-acswap", NULL, "alt-ctrl swap for shortcut keys", "alt_ctrl_swap",
	 SP_TYPE_BOOLEAN, &sw_config.alt_ctrl_swap, SP_FALSE_STRING},
    {NULL, NULL, "scroll left by wheel down", "scroll_left_by_wheel_down",
	 SP_TYPE_BOOLEAN, &sw_config.scroll_left_by_wheel_down, SP_FALSE_STRING},
#ifdef SW_USE_TOOL_BAR
    {NULL, NULL, "not use tool bar", "no_tool_bar",
	 SP_TYPE_BOOLEAN, &sw_config.no_tool_bar, SP_FALSE_STRING},
#endif
    {NULL, NULL, "draw waveform in detail", "draw_detail",
	 SP_TYPE_BOOLEAN, &sw_config.draw_detail, SP_FALSE_STRING},
    {NULL, NULL, "use logarithmic frequency axis", "log_frequency_axis",
	 SP_TYPE_BOOLEAN, &sw_config.log_frequency_axis, SP_FALSE_STRING},
    {NULL, NULL, "display overview of waveform", "display_overview",
         SP_TYPE_BOOLEAN, &sw_config.display_overview, SP_TRUE_STRING},
    {NULL, NULL, "display level meter", "display_meter",
         SP_TYPE_BOOLEAN, &sw_config.display_meter, SP_TRUE_STRING},
    {NULL, NULL, "draw selection times", "draw_selection_times",
         SP_TYPE_BOOLEAN, &sw_config.draw_selection_times, SP_TRUE_STRING},
    {NULL, NULL, "draw selection length", "draw_selection_length",
         SP_TYPE_BOOLEAN, &sw_config.draw_selection_length, SP_TRUE_STRING},
    {"-lreg", NULL, "link label to selection", "link_region_label",
	 SP_TYPE_BOOLEAN, &sw_config.link_region_label, SP_FALSE_STRING},
    {NULL, NULL, "prompt before erasing labels", "erase_label_prompt",
	 SP_TYPE_BOOLEAN, &sw_config.erase_label_prompt, SP_TRUE_STRING},
    {NULL, NULL, "show Save As dialog in first label save", "show_dialog_in_first_label_save",
	 SP_TYPE_BOOLEAN, &sw_config.show_dialog_in_first_label_save, SP_FALSE_STRING},
    {NULL, NULL, "add default label suffix for Save As dialog", "add_default_label_suffix_for_save_as",
	 SP_TYPE_BOOLEAN, &sw_config.add_default_label_suffix_for_save_as, SP_TRUE_STRING},
    {"-info", NULL, "display information area", "display_info_area",
	 SP_TYPE_BOOLEAN, &sw_config.display_info_area, SP_FALSE_STRING},
    {"-finfo", NULL, "display frequency information area", "display_freq_info_area",
	 SP_TYPE_BOOLEAN, &sw_config.display_freq_info_area, SP_FALSE_STRING},
    {NULL, NULL, "draw scale", "draw_scale",
	 SP_TYPE_BOOLEAN, &sw_config.scale_flag, SP_TRUE_STRING},
    {NULL, NULL, "draw grid", "draw_grid",
	 SP_TYPE_BOOLEAN, &sw_config.grid_flag, SP_TRUE_STRING},
    {NULL, NULL, "draw piano keys for specgram", "draw_piano_keys_for_specgram",
	 SP_TYPE_BOOLEAN, &sw_config.draw_piano_keys_for_specgram, SP_FALSE_STRING},
    {NULL, NULL, "draw piano keys for spectrum", "draw_piano_keys_for_spectrum",
	 SP_TYPE_BOOLEAN, &sw_config.draw_piano_keys_for_spectrum, SP_FALSE_STRING},
    {NULL, NULL, "locate vertical piano keys at right", "vertical_piano_keys_right",
	 SP_TYPE_BOOLEAN, &sw_config.vertical_piano_keys_right, SP_FALSE_STRING},
    {NULL, NULL, "locate horizontal piano keys at top", "horizontal_piano_keys_top",
	 SP_TYPE_BOOLEAN, &sw_config.horizontal_piano_keys_top, SP_FALSE_STRING},
    {"-debug", NULL, "debug level", NULL,
	 SP_TYPE_INT, &debug_level, NULL},
    {"-h", "-help", "display this message", NULL,
	 SP_TYPE_BOOLEAN, &help_flag, SP_FALSE_STRING},
};

static const char *sw_filelabel[] = {
    "[filename...]",
};

int spMain(int argc, char *argv[])
{
    const char *filename;
    spOptions options;
    spTopLevel toplevel;
    char buf[SP_MAX_MESSAGE];

#if 0
    spSetDebugLevel(100);
#endif

    spSetApplicationId("spwave");

    spSetStringTable(SW_JP_LANG, sw_string_table_jp);
#if defined(_WIN32) || defined(SP_SUPPORT_UTF8_STRING)
    spSetStringTable("ja_JP.utf8", sw_string_table_utf8jp);
#endif
    spSetStringTable("C", sw_string_table);

    /* initialize toolkit */
    toplevel = spInitialize(&argc, &argv,
			    SppIconName, "spwave_icon",
			    SppThreadSafe, SP_TRUE,
			    SppUseWindowMenu, SP_TRUE,
			    SppUniqueApplicationId, "spwave",
			    SppHelpPath, "help",
                            SppPermissions, SP_PERMISSION_RECORD_AUDIO,
                            SppTopLevelCallbackFunc, toplevelCB,
			    NULL);
    
    spSetOptionDescOffset(48);
    spSetSetup(SW_SETUP_FILE);
    spSetHelpMessage(&help_flag, SW_HELP_MESSAGE, SW_VERSION_STRING);

    /* get option value */
    options = spGetOptions(argc, argv, sw_option, sw_filelabel);
    spGetOptionsValue(argc, argv, options);
    spSetDebugLevel(debug_level);

    sw_config.use_lowlevel_thread = SP_TRUE;
    
    if (spIsThreadLevelVariable() == SP_TRUE) {
	if (1 || sw_config.use_lowlevel_thread == SP_TRUE) {
	    spSetThreadLevel(SP_THREAD_LEVEL_LOW);
	} else {
	    spSetThreadLevel(SP_THREAD_LEVEL_HIGH);
	}
    }
    
    /* set program information (useful on Mac) */
    sprintf(buf, SW_INFO_MESSAGE, SW_VERSION_STRING);
    spSetTopLevelParams(toplevel,
			SppInformation, buf,
			SppAltCtrlSwap, sw_config.alt_ctrl_swap,
			NULL);
    
    swInitialize(toplevel, &sw_config);

    spDebug(10, NULL, "label_file = %s, label_format = %d, region_label_format = %d\n",
	    sw_label_file, sw_wave_config.label_format, sw_wave_config.region_label_format);
    
    while ((filename = spGetFile(options)) != NULL) {
	spDebug(10, NULL, "file = %s\n", filename);
	swCreateWindow(filename, sw_label_file, sw_is_region_label,
		       sw_display_offset_s, sw_display_length_s, &sw_config);
    }
    swCreateWindow(NULL, NULL, SP_FALSE, -1.0, -1.0, &sw_config);

    spDebug(10, NULL, "loop of creating window done\n");

    /* main loop */
    return spMainLoop(toplevel);
}
