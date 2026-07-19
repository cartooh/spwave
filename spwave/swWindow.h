/*
 *	swWindow.h
 */

#ifndef __SWWINDOW_H
#define __SWWINDOW_H

#include <sp/spDefs.h>
#include <sp/spComponentLib.h>

#include "swWave.h"
#include "swStringDefs.h"
#if defined(SW_SUPPORT_MORPHING)
#include "swMorphing.h"
#endif

#if defined(SW_AH_CUSTOM)
#include "swDefsAH.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define SW_USE_POST_EVENT
/*#undef SW_USE_POST_EVENT*/

#if defined(MACOS)
#define SW_AUDIO_BUFFER_SIZE 8192
#else
#define SW_AUDIO_BUFFER_SIZE 1024
#endif
#define SW_READ_AFTER

#define SW_SUPPORT_METER

#define SW_SUPPORT_PRINT

#define SW_SUPPORT_INFO_AREA_SELECTION_TAB
    
#define SW_USE_TOOL_BAR
#define SW_USE_ANALYSIS
#define SW_SUPPORT_EDIT
#define SW_SUPPORT_SAVE
#define SW_SUPPORT_INFO_DIALOG
#define SW_SUPPORT_PROPERTY_DIALOG
#define SW_SUPPORT_AUTOSAVE
#define SW_SUPPORT_DRAG_DROP
#define SW_SUPPORT_CLIPBOARD
#define SW_CLOSE_PROMPT
#define SW_SUPPORT_THREAD_WRITE
#define SW_SUPPORT_LOG_FREQUENCY_AXIS

#define SW_SUPPORT_CQT_ANALYSIS
#define SW_SUPPORT_CQT_SPECTROGRAM

/*#undef SW_SUPPORT_SBL_NAMING_CUSTOM*/
#define SW_SUPPORT_SBL_NAMING_CUSTOM

/*#define SW_SUPPORT_ANALYSIS_SUBPLOT*/
/*#undef SW_SUPPORT_ANALYSIS_SUBPLOT*/

#if defined(SW_AH_CUSTOM)
#define SW_VERSION_STRING SW_AH_VERSION_STRING
#else
#define SW_VERSION_STRING "0.9.0-1"
#endif
#define SW_HELP_MESSAGE "Audio file editor version %s"
#define SW_SETUP_FILE "~/.spwave"
    
#define SW_INIT_TIME_FORMAT SW_TIME_FORMAT_SEC

#define SW_MAX_NUM_SUB_AREA 8

/* groups for sense levels */
#define SW_OPEN_GROUP_ID 10L
#define SW_SAVE_GROUP_ID 20L
#define SW_PLAY_GROUP_ID 30L
#define SW_PAGE_GROUP_ID 40L
    
#define SW_CHANNEL_GROUP_ID 50L
#define SW_WINDOW_GROUP_ID 60L
#define SW_QUIT_GROUP_ID 65L
#define SW_DATA_GROUP_ID 70L
#define SW_CLIPBOARD_DATA_GROUP_ID 75L
    
#define SW_LABEL_GROUP_ID 80L
#define SW_NORMAL_LABEL_GROUP_ID 90L
#define SW_REGION_LABEL_GROUP_ID 100L
#define SW_LABEL_LIST_GROUP_ID 105L
#define SW_ANCHOR_GROUP_ID 106L
#define SW_ANCHOR_SELECTION_GROUP_ID 107L
#define SW_ANCHOR_CLIPBOARD_GROUP_ID 108L

#define SW_UNDO_GROUP_ID 110L
#define SW_SCROLL_GROUP_ID 120L
#define SW_CLIPBOARD_GROUP_ID 130L
    
#if defined(SW_AH_CUSTOM)
#define SW_AH_GROUP_ID 300L
#define SW_STATE_AH_SESSION_NONE 0L
#define SW_STATE_AH_SESSION_STARTED 10L
#define SW_STATE_AH_SESSION_FILE_LOADED 20L
#endif
    
/* sense levels for default group */
#define SW_STATE_PLAY_WAVE 0L
#define SW_STATE_NO_WAVE 10L
#define SW_STATE_EXIST_WAVE 20L
#define SW_STATE_NOT_PLAY_WAVE SW_STATE_EXIST_WAVE
#define SW_STATE_SELECT_WAVE 30L
#define SW_STATE_EDIT_WAVE 30L

/* sense levels for channel group */
#define SW_STATE_MONO_WAVE 20L
#define SW_STATE_STEREO_WAVE 30L
#define SW_STATE_MULTI_CHANNEL_WAVE 40L
    
/* sense levels for window group */
#define SW_STATE_ONE_WINDOW 5L
#define SW_STATE_SOME_WINDOWS 10L
#define SW_STATE_STOP_SOME_WINDOWS 20L

/* sense levels for data group */
#define SW_STATE_NO_DATA 0L
#define SW_STATE_FREQ_DATA 10L
#define SW_STATE_PLAY_TIME_DATA 15L
#define SW_STATE_EDIT_TIME_DATA 18L
#define SW_STATE_TIME_DATA 20L
#define SW_STATE_SELECT_TIME_DATA 30L
#define SW_STATE_SELECT_TIME_CHANNELS 40L
#define SW_STATE_SELECT_TIME_MULTI_CHANNEL 50L

/* sense levels for label group */
#define SW_STATE_NO_LABEL 0L
#define SW_STATE_EXIST_LABEL 10L
#define SW_STATE_EXIST_LABEL_HERE 20L

/* sense levels for anchor group */
#define SW_STATE_NO_ANCHOR 0L
#define SW_STATE_EXIST_ANCHOR 10L
#define SW_STATE_EXIST_TIME_ANCHOR_HERE 20L
#define SW_STATE_EXIST_FREQUENCY_ANCHOR_HERE 30L
#define SW_STATE_SELECT_ANCHOR 30L
#define SW_STATE_EXIST_ANCHOR_CLIPBOARD 30L
    
/* sense levels for clipboard group */
#define SW_STATE_NO_CLIPBOARD 0L
#define SW_STATE_EXIST_CLIPBOARD 30L
#define SW_STATE_EXIST_CLIPBOARD_SELECTED 40L
    
/* sizes */
#define SW_WINDOW_WIDTH 800
#define SW_WINDOW_HEIGHT /*200*/240
#define SW_MIN_WINDOW_WIDTH 50
#define SW_MIN_WINDOW_HEIGHT 20
#define SW_METER_WIDTH 20

    
#define SW_INFO_AREA_WIDTH 340
    
#define SW_FREQ_WINDOW_WIDTH 400
#define SW_FREQ_WINDOW_HEIGHT 300
    
#define SW_DEFAULT_SLIDER_WIDTH 15

#define SW_EDGE_MOTION_RANGE 15
    
#define SW_LABEL_DELETE_RANGE 30

#define SW_MIN_DRAW_WIDTH 40
#define SW_MIN_DRAW_HEIGHT 20

#define SW_TIME_STRING_TOP_OFFSET -15
#define SW_TIME_STRING_LEFT_OFFSET 15

#define SW_AMP_STRING_TOP_OFFSET 25
#define SW_AMP_STRING_LEFT_OFFSET 15

#define SW_WAVE_VSPACE 5


#define SW_MIN_HSCALE_DIV /*80*/110
#define SW_MIN_VSCALE_DIV /*30*//*25*/35
#define SW_MIN_HSCALE_DIV_FOR_PRINT 95
#define SW_MIN_VSCALE_DIV_FOR_PRINT 30
#define SW_TICK_LENGTH 5
#define SW_HSCALE_LEFT_SPACING 65
#define SW_HSCALE_BOTTOM_SPACING 25
#define SW_HSCALE_STRING_LEFT_OFFSET 2
#define SW_HSCALE_STRING_BOTTOM_OFFSET 3
#define SW_VSCALE_STRING_LEFT_OFFSET 5
#define SW_VSCALE_STRING_TOP_OFFSET 2
#define SW_VSCALE_STRING_BOTTOM_OFFSET 2

#define SW_TIME_STRING_HEIGHT 16

/* for printing */
#define SW_PRINT_LEFT_MARGIN /*80*/90
#define SW_PRINT_RIGHT_MARGIN 38
#define SW_PRINT_TOP_MARGIN 10
#define SW_PRINT_BOTTOM_MARGIN 50
#define SW_PRINT_VSCALE_STRING_SPACING 5
#define SW_PRINT_HSCALE_STRING_SPACING 4
#define SW_PRINT_XLABEL_SPACING /*15*/11
#define SW_PRINT_YLABEL_SPACING /*15*/19
#define SW_PRINT_VSCALE_STRING_TOP_OFFSET (-2)
#define SW_PRINT_VSCALE_STRING_BOTTOM_OFFSET 0
    
/* shortcuts */
#define SW_OPEN_SHORTCUT "A-o"
#define SW_GENERATE_SHORTCUT ""
#define SW_SAVE_SHORTCUT "A-s"
#define SW_CLOSE_SHORTCUT "A-w"
#define SW_PRINT_SHORTCUT "A-p"
#define SW_QUIT_SHORTCUT "A-q"
    
#define SW_OPEN_NEW_SHORTCUT "A-n"
#define SW_SAVE_AS_SHORTCUT ""
    
#define SW_OPEN_NORMAL_LABEL_SHORTCUT "A-b"
#define SW_SAVE_NORMAL_LABEL_SHORTCUT "A-l"
#define SW_SAVE_AS_NORMAL_LABEL_SHORTCUT ""
#define SW_OPEN_REGION_LABEL_SHORTCUT ""
#define SW_SAVE_REGION_LABEL_SHORTCUT "A-g"
#define SW_SAVE_AS_REGION_LABEL_SHORTCUT ""
    
#define SW_FORWARD_SHORTCUT "C-f"
#define SW_BACKWARD_SHORTCUT "C-b"
#define SW_GO_HEAD_SHORTCUT "C-a"
#define SW_GO_TAIL_SHORTCUT "C-e"
#define SW_NEXT_WINDOW_SHORTCUT "C-n"
#define SW_PREV_WINDOW_SHORTCUT "C-p"
#define SW_ALIGN_WINDOW_SHORTCUT "C-l"
#define SW_SELECT_REGION_SHORTCUT "C-Space"
#define SW_SELECT_ALL_REGION_SHORTCUT "S-a"
#define SW_SELECT_ALL_SHORTCUT "A-a"
#define SW_SELECT_NEXT_CHANNEL_SHORTCUT "S-t"
    
#define SW_ZOOM_IN_SHORTCUT "C-i"
#define SW_ZOOM_OUT_SHORTCUT "C-o"
#define SW_ZOOM_FULL_OUT_SHORTCUT "S-f"
#define SW_ZOOM_REGION_SHORTCUT "S-b"

#define SW_DISPLAY_INFO_AREA_SHORTCUT "A-i"
    
#define SW_UNDO_SHORTCUT "A-z"
#define SW_REDO_SHORTCUT "A-y"
#define SW_CROP_SHORTCUT "S-c"
#define SW_DELETE_SHORTCUT "Delete"
#define SW_ERASE_SHORTCUT ""
#define SW_EXTRACT_SHORTCUT "S-x"
#define SW_EXTRACT_AUTOSAVE_SHORTCUT "S-v"
#define SW_PREFERENCE_SHORTCUT ""
    
#define SW_INFO_SHORTCUT ""
    
#define SW_PLAY_SHORTCUT "S-p"
#define SW_LOOP_PLAY_SHORTCUT "S-o"
#define SW_PLAY_WINDOW_SHORTCUT "S-w"
#define SW_PLAY_FILE_SHORTCUT "S-e"
#define SW_RECORD_SHORTCUT "A-r"
#define SW_PLAY_STOP_SHORTCUT "S-s"
#define SW_SYNC_PLAY_SHORTCUT "F"/*""*/
#define SW_PAUSE_CURSOR_SHORTCUT "0"/*""*/

#define SW_ANALYSIS_REGION_SHORTCUT "S-n"
#define SW_INSERT_SIMPLE_LABEL_SHORTCUT "S-l"
#define SW_ERASE_LABEL_SHORTCUT "C-d"
#define SW_SET_REGION_LABEL_SHORTCUT "S-g"
#define SW_CAT_REGION_LABELS_SHORTCUT "A-t"
#define SW_DIVIDE_REGION_LABEL_SHORTCUT "C-v"
    
#define SW_CUT_SHORTCUT "A-x"
#define SW_COPY_SHORTCUT "A-c"
#define SW_PASTE_SHORTCUT "A-v"
#define SW_MIX_SHORTCUT "S-m"
#define SW_INSERT_SHORTCUT "S-i"
#define SW_CAT_SHORTCUT "A-d"
#define SW_CATTOP_SHORTCUT "S-d"
#define SW_REPLACE_SHORTCUT "S-r"
#ifdef SW_SUPPORT_MORPHING
#define SW_INSERT_ANCHOR_SHORTCUT ""
#endif

/* window title */    
#define SW_NO_WAVE_TITLE /*"No Wave"*/"spwave"
#define SW_UNTITLED_TITLE "Untitled"
#define SW_CLIPBOARD_TITLE "Clipboard"

/* enums */
typedef enum {
    SW_UNKNOWN_DATA = -1,
    SW_TIME_DATA = 0,
    SW_FREQ_DATA = 1,
} swDataType;

/* not used yet */
typedef enum {
    SW_UNKNOWN_PLOT = -1,
    SW_NORMAL_PLOT = 0,
    SW_LOG_PLOT = 1,
    SW_DECIBEL_PLOT = 2,
    SW_DECIBEL10_PLOT = 3,
    SW_ABS_PLOT = 4,
    SW_LOG_ABS_PLOT = 5,
    SW_DECIBEL_ABS_PLOT = 6,
    SW_DECIBEL10_ABS_PLOT = 7,
} swPlotType;

typedef enum {
    SW_FREQ_FORMAT_NONE = -1,
    SW_FREQ_FORMAT_HZ = 0,
    SW_FREQ_FORMAT_KHZ = 1,
    SW_FREQ_FORMAT_POINT = 2,
} swFreqFormat;

typedef enum {
    SW_AUTOSAVE_ID_2DIGIT_SERIAL = 0,
    SW_AUTOSAVE_ID_3DIGIT_SERIAL = 1,
    SW_AUTOSAVE_ID_4DIGIT_SERIAL = 2,
    SW_AUTOSAVE_ID_SERIAL = 3,
    SW_AUTOSAVE_ID_EDGE_POINTS = 4,
} swAutosaveIdType;

typedef enum {
    SW_DRAG_NO_LABEL = 0,
    SW_DRAG_START_LABEL = 1,
    SW_DRAG_END_LABEL = 2,
	
    SW_DRAG_TIME_ANCHOR = 10,
    SW_DRAG_FREQUENCY_ANCHOR = 11,
} swDragLabelType;

typedef enum {
    SW_OVERVIEW_DRAG_NONE = -1,
    SW_OVERVIEW_DRAG_START = 0,
    SW_OVERVIEW_DRAG_END = 1,
    SW_OVERVIEW_DRAG_THUMB = 2,
    SW_OVERVIEW_DRAG_AUDIO = 3,
} swOverviewDragType;

typedef enum {
    SW_SBL_NAMING_UNKNOWN = -1,
    SW_SBL_NAMING_LABEL = 0,
    SW_SBL_NAMING_DATA = 1,
    SW_SBL_NAMING_ORIG_AND_LABEL = 2,
    SW_SBL_NAMING_ORIG_AND_DATA = 3,
    SW_SBL_NAMING_AUTOSAVE = 4,
    SW_SBL_NAMING_CUSTOM = 5,
} swSaveByLabelNamingRule;

#define SW_SBL_NAMING_DEFAULT_CUSTOM_FORMAT_LABEL "%L"
#define SW_SBL_NAMING_DEFAULT_CUSTOM_FORMAT_DATA "%D"
#define SW_SBL_NAMING_DEFAULT_CUSTOM_FORMAT_ORIG_AND_LABEL "%F-%L"
#define SW_SBL_NAMING_DEFAULT_CUSTOM_FORMAT_ORIG_AND_DATA "%F-%D"
#define SW_SBL_NAMING_DEFAULT_CUSTOM_FORMAT_AUTOSAVE "%F-%02d"

typedef enum {
    SW_MOUSE_MODE_NORMAL = 0,
    SW_MOUSE_MODE_PLAY = 1,
	
    SW_MOUSE_MODE_NORMAL_LABEL_SINGLE = 10,
    SW_MOUSE_MODE_NORMAL_LABEL_MULTI = 11,

    SW_MOUSE_MODE_REGION_LABEL_SINGLE = 15,
    SW_MOUSE_MODE_REGION_LABEL_MULTI = 16,
} swMouseMode;

typedef enum {
    SW_REC_BUTTON_STATE_STOP = 0,
    SW_REC_BUTTON_STATE_PRESSING = 1,
    SW_REC_BUTTON_STATE_RECORDING = 2,
    SW_REC_BUTTON_STATE_PAUSE = 3,
} swRecButtonState;


#define SW_ANALYSIS_CONFIG_FLAG_NONE (0)
#define SW_ANALYSIS_CONFIG_FLAG_NARROW_SPECTRUM (1)
#define SW_ANALYSIS_CONFIG_FLAG_WIDE_SPECTRUM (1<<1)
#define SW_ANALYSIS_CONFIG_FLAG_SMOOTHED_SPECTRUM (1<<2)
#define SW_ANALYSIS_CONFIG_FLAG_CQT_SPECTRUM (1<<3)
#define SW_ANALYSIS_CONFIG_FLAG_NARROW_SMOOTHED_SPECTRUM (SW_ANALYSIS_CONFIG_FLAG_NARROW_SPECTRUM|SW_ANALYSIS_CONFIG_FLAG_SMOOTHED_SPECTRUM)
#define SW_ANALYSIS_CONFIG_FLAG_WIDE_SMOOTHED_SPECTRUM (SW_ANALYSIS_CONFIG_FLAG_WIDE_SPECTRUM|SW_ANALYSIS_CONFIG_FLAG_SMOOTHED_SPECTRUM)
typedef unsigned long swAnalysisConfigFlag;


#define SW_LABEL_CAPS_NONE (0)
#define SW_LABEL_CAPS_NORMAL (1) /* not checked */
#define SW_LABEL_CAPS_MULTI_NORMAL (1<<1)
#define SW_LABEL_CAPS_CHANNEL_NORMAL (1<<2)
#define SW_LABEL_CAPS_NONCHANNEL_NORMAL (1<<3)
#define SW_LABEL_CAPS_REGION (1<<5) /* not checked */
#define SW_LABEL_CAPS_MULTI_REGION (1<<6)
#define SW_LABEL_CAPS_CHANNEL_REGION (1<<7)
#define SW_LABEL_CAPS_NONCHANNEL_REGION (1<<8)
#define SW_LABEL_CAPS_ALL (SW_LABEL_CAPS_NORMAL | SW_LABEL_CAPS_MULTI_NORMAL | SW_LABEL_CAPS_CHANNEL_NORMAL | SW_LABEL_CAPS_NONCHANNEL_NORMAL | SW_LABEL_CAPS_REGION | SW_LABEL_CAPS_MULTI_REGION | SW_LABEL_CAPS_CHANNEL_REGION | SW_LABEL_CAPS_NONCHANNEL_REGION)
typedef unsigned long swLabelCaps;
    
/* structures */
typedef struct _swConfig swConfigRec, *swConfig;
typedef struct _swTopLevel *swTopLevel;
typedef struct _swWindow *swWindow;
typedef struct _swLabelList *swLabelList;
typedef struct _swWaveSubArea *swWaveSubArea;

struct _swWaveSubArea {
    swWave wave;
    spBool specgram_flag;
    
    long pos_index;
    
    spLong drawn_pos;

    double y_d;
    double height_d;            /* height for all channels */
    double draw_height;         /* drawing height for each channel */

    swWaveSubArea prev;
    swWaveSubArea next;
};
    
struct _swConfig {
    spBool help_flag;
    spBool prev_flag;
    spBool use_def_format;
    spBool use_lowlevel_thread;
    spBool use_play_command;
    spBool use_loop_play;
    spBool use_sync_play;
    spBool play_draw;
    spBool pause_cursor;
    spBool alt_ctrl_swap;
    spBool scroll_left_by_wheel_down;
    spBool no_tool_bar;
    spBool draw_detail;
    spBool log_frequency_axis;
    spBool display_overview;
    spBool display_meter;
    spBool draw_selection_times;
    spBool draw_selection_length;
    spBool link_region_label;
    spBool erase_label_prompt;
    spBool quit_prompt;
    spBool show_dialog_in_first_label_save;
    spBool add_default_label_suffix_for_save_as;
    spBool display_info_area;
    spBool display_freq_info_area;
    spBool analysis_new_window;
    spBool draw_piano_keys_for_specgram;
    spBool draw_piano_keys_for_spectrum;
    spBool vertical_piano_keys_right;
    spBool horizontal_piano_keys_top;
    
    spBool specgram_simplified;
    spBool specgram_gray_scale;
    
    spBool scale_flag;
    spBool grid_flag;
    spBool zero_flag;
    
    swDataType data_type;
    swAnalysisType analysis_type;
    swAnalysisType cqt_type;
    swWindowType window_type;
    int num_channel;
    int def_num_channel;
    int head_size;
    int def_head_size;
    int samp_bit;
    int def_samp_bit;
    long num_order;
    int audio_buffer_size;

    int width;
    int height;
    int print_width;
    int print_height;
    int freq_width;
    int freq_height;
    int overview_height;
    int info_area_width;
    int meter_range;
    int specgram_range;
    int specgram_limit_threshold; /* must have minus value or 0 */
    long fftl;
    long max_fftl;
    double lifterm;
    double shiftm;
    double framem;
    double wide_framem;
    double narrow_framem;
    double samp_rate;
    double def_samp_rate;

    int mid_C_octave_index; /* 3: SPN/IPN/Roland, 4: YAMAHA/Apple */

    spBool pause_play_sync_pos;
    double pause_play_step_time;
    
    char data_type_string[SP_MAX_SETUP_VALUE];
    char analysis_type_string[SP_MAX_SETUP_VALUE];
    char window_type_string[SP_MAX_SETUP_VALUE];
    char format_string[SP_MAX_SETUP_VALUE];
    char def_format_string[SP_MAX_SETUP_VALUE];
    char play_command[SP_MAX_SETUP_VALUE];
    char browser_command[SP_MAX_SETUP_VALUE];
    char doc_dir[SP_MAX_SETUP_VALUE];
    char setup_file[SP_MAX_SETUP_VALUE];
    char temp_dir[SP_MAX_SETUP_VALUE];

    spBool autosave_use_region;
    spBool autosave_label_fullpath;
    spBool autosave_by_drop;
    swAutosaveIdType autosave_id_type;
    char autosave_dir[SP_MAX_SETUP_VALUE];
    char autosave_id_prefix[SP_MAX_SETUP_VALUE];
    char autosave_label_suffix[SP_MAX_SETUP_VALUE];

    swSaveByLabelNamingRule sbl_naming_rule;
    spBool sbl_index_from_1st;
    spBool sbl_illegal_char_to_space;
    spBool sbl_data_as_filename;
    spBool sbl_no_overwrite_prompt;
    spBool sbl_create_window;
    /* %d, %02d, %03d: number, %L: label string, %D: data string, %F: original filename */
    char sbl_naming_custom_format[SP_MAX_SETUP_VALUE];
    char sbl_naming_repetition_suffix_format[SP_MAX_SETUP_VALUE]; /* can include number only */
    
    char wave_fg[SP_MAX_SETUP_VALUE];
    char wave_bg[SP_MAX_SETUP_VALUE];
    char pointer_color[SP_MAX_SETUP_VALUE];
    char string_color[SP_MAX_SETUP_VALUE];
    char label_color[SP_MAX_SETUP_VALUE];
    char scale_color[SP_MAX_SETUP_VALUE];
    char canvas_font[SP_MAX_SETUP_VALUE];
    
    swTimeFormat time_format;
    swFreqFormat freq_format;

    spBool percent_amplitude;

    /* private data */
    spBool format_specified;
    
    swWaveConfig wave_config;
    swTopLevel toplevel;
};

struct _swTopLevel {
    spTopLevel toplevel;
    swWindow null_window;
    swWindow current_window;
    swWindow clipboard_window;
    spBool format_specified;
    unsigned long graphics_mode_caps;
    spBool rubber_band_selection;
    double log_min_value;

    spComponent print_canvas;
    
    long num_window;
    int num_process;
    spBool playable;
    spBool using_clipboard;
    void *main_mutex;

#if defined(SW_AH_CUSTOM)
    swAHInfoRec ahinforec;
    swAHFile ahfile;
    swAHSessionWindow ahwindow;
#endif
};

struct _swWindow {
    char *title;
    char *name;
    swDataType data_type;
    long undo_group_id;
    int index;
    spLong scroll_coef;
    int width, height;
    int overview_width, overview_height;
    int meter_width;
    double draw_width;
    double vertical_keys_width;
    double draw_height_horizontal_keys;
    spLong current_play_pos;

#if 0
    spLong drawn_pos;
#endif
    double amp_min, amp_max;
    
    spBool selecting;
    spLong sel_st, sel_ed;
    int sel_st_d, sel_ed_d;
    double sel_st_f, sel_ed_f;
    spLong point;
    int point_d;
    double point_f;
    double prev_point_f;
    swWaveSubArea target_sub_area;
    int target_channel;
    long target_order;
    long active_label_index;
    
    spLong offset, length;
    swWave wave;

    spBool cb_return;

    swAnalysisConfigFlag specgram_config_flag;
    swAnalysisType specgram_analysis_type;
    swWave specgram;

#ifdef SW_SUPPORT_ANALYSIS_SUBPLOT
    spBool subplot_sgram, subplot_f0, subplot_power;
    swWave f0, power;
#ifdef SW_SUPPORT_STRAIGHT
    spBool subplot_straight_f0, subplot_straight_sgram, subplot_straight_ap;
    swWave straight_f0, straight_sgram, straight_ap;
#endif
#endif

    long num_sub_area;
    /*swWaveSubArea sub_areas[SW_MAX_NUM_SUB_AREA];*/
    swWaveSubArea first_sub_area;
    swWaveSubArea last_sub_area;

    spBool visible_flag;
    spBool analysis_flag;
    spBool loop_play;
    spBool sync_play;
    spBool pause_cursor;
    spBool drag_region;
    spBool draw_label;
    spBool draw_specgram;
    spBool draw_vertical_keys;
    spBool draw_horizontal_keys;
    spBool draw_detail;
    spBool log_frequency_axis;
    spBool display_info_area;
    spBool autosave_started;
    spBool thread_entered;
    spBool process_flag;
    int num_blocked;
    swDragLabelType drag_label_type;

#if defined(SW_AH_CUSTOM)
    spBool link_ah_file;
#endif
    
    swLabelCaps label_caps;
    swMouseMode mouse_mode;
    
    spBool execute_save_by_label;
    swWave save_by_label_wave;
    
    int pointer;
    int direction;
    swConfig config;
    spComponent window;
    spComponent wave_box;
    spComponent overview_canvas;
    spComponent canvas;
    spComponent image;
    spComponent meter_image;
    spComponent meter_bar_image;
    spComponent hscroll;
    spComponent vscroll;
    spComponent tool_bar;
    spComponent drawing_plugin_menu;
    spComponent undo_menu;
    spComponent redo_menu;
    spComponent time_format_menu;
#ifdef SW_SUPPORT_LOG_FREQUENCY_AXIS
    spComponent log_frequency_axis_menu;
#endif
#ifdef SW_SUPPORT_ANALYSIS_SUBPLOT
    spComponent subplot_menu;
#endif
    spComponent specgram_menu;
    spComponent info_box;

    spComponent loop_play_menu;
    spComponent loop_play_tool_item;

    spComponent sync_play_menu;
    spComponent sync_play_tool_item;
    
    spComponent pause_cursor_menu;
    spComponent pause_cursor_tool_item;
    
#if defined(SW_AH_CUSTOM)
    spComponent toggle_ah_mode_menu;
    spComponent link_ah_file_menu;
#endif
    
    swLabelList label_list;
    struct _swWindow *related_window;

    void *mutex;
    
#ifdef SW_SUPPORT_MORPHING
    swAnchors time_anchors;
    swAnchor drag_anchor;
#endif
};

#if defined(MACOS)
#pragma import on
#endif

extern swWaveSubArea swSetWaveToSubArea(swWindow window, swWave wave, spBool specgram_flag, spBool non_first_only);
extern spBool swUnsetWaveToSubArea(swWindow window, swWave wave, spBool non_first_only);
extern swWaveSubArea swSetWaveToFirstSubArea(swWindow window, swWave wave, spBool specgram_flag);
extern swWaveSubArea swReplaceWaveSubArea(swWindow window, swWave old_wave, swWave new_wave, spBool specgram_flag);
extern swWaveSubArea swGetWaveSubArea(swWindow window, swWave wave);
extern spBool swIsWaveSubAreaVisible(swWaveSubArea sub_area);
extern spBool swIsWaveSubAreaSpectrogram(swWaveSubArea sub_area);
extern spBool swResetDrawnPos(swWindow window, swWave wave);
extern long swResetAllDrawnPos(swWindow window);
extern long swGetNumWaveSubArea(swWindow window);
extern long swGetNumVisibleWaveSubArea(swWindow window);
extern double swUpdateWaveSubAreaSize(swWindow window);
extern spBool swIsLastWaveSubArea(swWindow window, swWaveSubArea sub_area);
extern spBool swIsFirstWaveSubArea(swWindow window, swWaveSubArea sub_area);
extern swWaveSubArea swGetNextWaveSubArea(swWindow window, swWaveSubArea sub_area);
extern void swDestroyWindowWave(swWindow window, swWave *wave);
    
extern swWindow swGetNextWindow(swWindow window);
extern swWindow swGetPrevWindow(swWindow window);

extern void swInitialize(spTopLevel toplevel, swConfig config);
extern void swCreateWindow(const char *filename, const char *label_file, spBool is_region_label,
			   double offset_s, double length_s, swConfig config);
extern void swUpdateMeterWidth(swWindow window);
extern swWindow swInitWindow(swConfig config, swDataType data_type);
extern void swInitVscroll(swWindow window);
extern void swUpdateVscroll(swWindow window);
extern void swSetWaveWithRegion(swWindow window, swWave wave, double offset_s, double length_s);
extern void swSetWave(swWindow window, swWave wave);
extern void swSetWindowTitle(swWindow window);
extern spDialogResponse swDisplayContinuePrompt(swWindow window, spBool close_flag);
extern spDialogResponse swDisplaySaveChangesDialog(swWindow window, spBool label_flag, spBool region_flag);
extern spBool swCloseWindowOrRevertToNullWindow(swWindow window, spBool quit_prompt, spBool can_revert_to_null_window);
extern spBool swCloseWindow(swWindow window, spBool quit_prompt);
extern void swSetSelectSenseLevel(swWindow window, spBool select);
extern void swSetProcessSenseLevel(swWindow window, spBool process);
extern void swSetWindowsSenseLevel(void);
extern void swSetWindowSenseLevel(swWindow window, spBool windows_flag);
extern void swSetSenseLevel(swWindow window);
extern void swResetWindow(swWindow window, spBool in_thread);
extern void swRedrawWindow(swWindow window);
extern void swSelectRadioButtonSubMenu(spComponent parent_menu, const char *name);
extern void swToggleCheckBoxSubMenu(spComponent parent_menu, char *name, spBool set);
extern spBool swGetCheckBoxSubMenuToggleState(spComponent parent_menu, char *name, spBool *set);
extern void swUpdateSpectrogramMenu(swWindow window);
extern void swResetSpectrogramMenu(swWindow window);
extern void swDestroySpectrogram(swWindow window, spBool reset_menu);

extern swWindow swCreateWaveWindowAt(swWave wave, swConfig config, swDataType data_type,
				     double offset_s, double length_s, int x, int y);
extern swWindow swCreateWaveWindow(swWave wave, swConfig config, swDataType data_type,
				   double offset_s, double length_s);
    
extern void swOpenNewWindow(spComponent component, swWindow window);
extern void swOpenWindow(spComponent component, swWindow window);
extern int swGetPluginNames(char ***plugin_names_ptr, char ***file_types_ptr, char ***file_filters_ptr);
extern char *xswGetSaveFileName(spComponent component, swWindow window, char *orig_filename,
                                int index, char **plugin_name, char **file_type);
extern void swSaveAsWindow(spComponent component, swWindow window);
extern void swSaveWindow(spComponent component, swWindow window);

extern void swUpdateInfoAreaDisplay(swWindow window);
extern void swUpdateAllInfoAreaDisplay(swWindow window);
    
extern void swCreateToolBar(swWindow window);
extern void swCreateMainWindow(swWindow window);

extern void swSetWindowValue(swWindow window);
extern spBool swIsNoWave(swWindow window);
extern spBool swIsNoLabel(swWindow window);
extern spBool swIsProcessing(swWindow window);
extern spBool swIsVisible(swWindow window);
extern spBool swIsSubplotVisible(swWindow window);
extern spBool swIsPlayable(swWindow window);
extern spBool swNeedWindowProcessBlock(swWindow src_window, swWindow window);
extern void swPopupWindow(swWindow window, int x, int y);

extern spBool swIsClipboardWindow(swWindow window);
extern spBool swIsClipboardVisible(swWindow window);
extern spBool swIsClipboardNone(swWindow window);
extern void swCreateClipboardWindow(swConfig config);
extern void swPopupClipboardWindowCB(spComponent component, swConfig config);

extern spBool swLockWindowMutex(swWindow window);
extern spBool swUnlockWindowMutex(swWindow window);
extern spBool swLockMainMutex(swWindow window);
extern spBool swUnlockMainMutex(swWindow window);
extern void swInitTopLevel(spTopLevel toplevel, swConfig config);
extern long swGetNumWindow(void);
extern long swGetNumProcess(void);

extern spBool swIsSpectrogramVisible(swWindow window);

#ifdef SW_SUPPORT_PRINT
extern void swPageSetupCB(spComponent component, swWindow window);
extern void swPrintCB(spComponent component, swWindow window);
extern spComponent swGetPrintCanvas(swWindow window);
extern const char *swGetPrintCanvasName(swConfig config);
extern spComponent swCreatePrintCanvas(swConfig config, const char *plugin_name);
extern void swSelectDrawingPluginCB(spComponent component, swWindow window);
extern void swCreateDrawingPluginMenu(swWindow window, spComponent parent_menu);
#endif
    
#if defined(MACOS)
#pragma import off
#endif

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration */
#endif

#endif /* __SWWINDOW_H */
