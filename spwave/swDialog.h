/*
 *	swDialog.h
 */

#ifndef __SWDIALOG_H
#define __SWDIALOG_H

#include "swWindow.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SW_USE_PREFERENCE_DIALOG

#define SW_FLOAT_BIT_STRING "32 (float)"

#define SW_MY_LOOK_INDEX 0
#define SW_MY_LOOK_LABEL ""
#define SW_ORIGINAL_LOOK_INDEX 1
#define SW_ORIGINAL_LOOK_LABEL "spwave (original)"
#define SW_OSCILLO_LOOK_INDEX 2
#define SW_OSCILLO_LOOK_LABEL "Oscilloscope"
#define SW_MONOTONE_LOOK_INDEX 3
#define SW_MONOTONE_LOOK_LABEL "Monotone"
#define SW_SKY_LOOK_INDEX 4
#define SW_SKY_LOOK_LABEL "Sky"
#define SW_FOREST_LOOK_INDEX 5
#define SW_FOREST_LOOK_LABEL "Forest"
#define SW_TOMATO_LOOK_INDEX 6
#define SW_TOMATO_LOOK_LABEL "Tomato"
#define SW_MATLAB_LOOK_INDEX 7
#define SW_MATLAB_LOOK_LABEL "M*TLAB"
#define SW_WAVES_LOOK_INDEX 8
#define SW_WAVES_LOOK_LABEL "W*ves"

typedef struct _swLook {
    int id;
    char *label;
    char *wave_fg;
    char *wave_bg;
    char *pointer_color;
    char *label_color;
    char *string_color;
    char *scale_color;
} swLook;
    
typedef struct _swFormatDialog {
    spComponent window;
    
    spComponent file_path_field;
    spComponent plugin_field;
    spComponent amp_max_field;
    spComponent amp_min_field;
    
    spComponent format_field;
    spComponent bit_field;
    spComponent samp_rate_field;
    spComponent channel_field;
    
    spCallbackReason call_reason;
    swConfig config;
} *swFormatDialog;

typedef struct _swPrefDialog {
    spComponent window;
    
    spComponent tab_box;
    
    spComponent file_tab;
    spComponent format_combo;
    spComponent bit_combo;
    spComponent samp_rate_combo;
    spComponent channel_combo;
    spComponent default_format_button;
    
    spComponent look_tab;
    spComponent look_like_combo;
    spComponent canvas_font_field;
    spComponent size_text;
    spComponent print_size_text;
    spComponent info_area_width_text;
    spComponent alt_ctrl_swap_button;
    spComponent scroll_left_by_wheel_down_button;
    spComponent use_tool_bar_button;
    
    spComponent display_tab;
    spComponent display_meter_button;
    spComponent draw_selection_times_button;
    spComponent draw_selection_length_button;
    spComponent draw_scale_button;
    spComponent draw_grid_button;
    spComponent percent_amplitude_button;
#ifdef SW_SUPPORT_LOG_FREQUENCY_AXIS
    spComponent log_frequency_axis_button;
#endif
    spComponent draw_spectrogram_keys_as_default_button;
    spComponent draw_spectrogram_keys_right_button;
    spComponent draw_horizontal_keys_as_default_button;
    spComponent draw_horizontal_keys_top_button;
    
    spComponent sound_tab;
    spComponent use_play_command_button;
    spComponent play_command_text;
    spComponent audio_driver_combo;
    spComponent buffer_size_combo;
    spComponent play_draw_button;
    spComponent use_loop_button;
    spComponent use_sync_button;
    spComponent use_thread_button;
    spComponent use_lowlevel_thread_button;

    spComponent label_tab;
    spComponent display_info_area_button;
    spComponent display_freq_info_area_button;
    spComponent link_region_label_button;
    spComponent erase_label_prompt_button;
#if 1
    spComponent show_dialog_in_first_label_save_button;
    spComponent add_default_label_suffix_for_save_as_button;
    spComponent load_default_label_button;
    spComponent default_label_suffix_combo;
    spComponent default_region_label_suffix_combo;
#endif
    
    spComponent autosave_tab;
    spComponent autosave_dir_text;
    spComponent autosave_id_type_combo;
    spComponent autosave_id_prefix_combo;
    spComponent autosave_label_suffix_combo;
    spComponent autosave_label_fullpath_button;
    spComponent autosave_by_drop_button;
    
    swConfig config;
} *swPrefDialog;

typedef struct _swRateChangeDialog *swRateChangeDialog;
typedef struct _swStringChangeDialog *swStringChangeDialog;
    
#if defined(MACOS)
#pragma import on
#endif

extern int swGetSampleBit(char *string);
extern spBool swGetSampleBitString(char *string, int samp_bit);
extern void swShowAllToolBarCB(spComponent component, swWindow window);
extern swPrefDialog swCreatePreferenceDialog(swConfig config);
extern void swPopupPreferenceDialogCB(spComponent component, swWindow window);
extern swFormatDialog swCreateFormatDialog(swConfig config);
extern spComponent swGetFormatDialogWindow(void);
extern spBool swPopupFormatDialog(swConfig config, const char *filename);
#ifdef SW_SUPPORT_PROPERTY_DIALOG
extern swFormatDialog swCreatePropertyDialog(swConfig config);
extern spBool swPopupPropertyDialog(swConfig config, swWindow window, swWave wave);
extern void swPopupPropertyDialogCB(spComponent component, swWindow window);
#endif
extern void swPopupBitConvDialogCB(spComponent component, swWindow window);
extern void swPopupAmplifyDialogCB(spComponent component, swWindow window);
extern void swPopupMaximizeDialogCB(spComponent component, swWindow window);
extern void swPopupSampFreqConvDialogCB(spComponent component, swWindow window);
extern void swPopupValueChangeDialogCB(spComponent component, swWindow window);
extern void swPopupInsertPauseDialogCB(spComponent component, swWindow window);

extern swStringChangeDialog swCreateStringChangeDialog(const char *title, const char *label, const char *dimension,
						       const char **list, const char *default_string, 
						       int offset, int size, spBool editable);
extern char *xswPopupStringChangeDialog(swStringChangeDialog dialog, const char *dimension, const char *string);

extern swRateChangeDialog swCreateRateChangeDialog(const char *title,
						   const char *rate_label, char **rate_strings,
						   const char *db_label, char **dB_strings,
						   int min_rate, int max_rate);
extern spBool swPopupRateChangeDialog(swRateChangeDialog dialog, double *rate);

extern void swPopupWaveformGenerateDialogCB(spComponent component, swWindow window);
    
#if defined(MACOS)
#pragma import off
#endif

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration */
#endif

#endif /* __SWDIALOG_H */
