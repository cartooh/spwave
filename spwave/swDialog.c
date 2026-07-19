/*
 *	swDialog.c
 *
 *	Last modified: <2025-04-24 20:25:57 hideki>
 */

#include <stdio.h>
#include <stdlib.h>

#include <sp/spBaseLib.h>
#include <sp/spAudioLib.h>
#include <sp/spComponentLib.h>

#include <sp/sp.h>

#include "swWindow.h"
#include "swDraw.h"
#include "swEdit.h"
#include "swCursor.h"
#include "swLabel.h"
#include "swDialog.h"

static char *format_list_strings[] =
{
    SP_WAVE_FORMAT_RAW_LABEL,
    SP_WAVE_FORMAT_SWAP_LABEL,
    SP_WAVE_FORMAT_LITTLE_LABEL,
    SP_WAVE_FORMAT_BIG_LABEL,
    SP_WAVE_FORMAT_ULAW_LABEL,
    SP_WAVE_FORMAT_ALAW_LABEL,
    SP_WAVE_FORMAT_TEXT_LABEL,
    SP_WAVE_FORMAT_TEXT_TIME_LABEL,
    SP_WAVE_FORMAT_TEXT_FREQ_LABEL,
    NULL,
};

static char *format_list_short_strings[] =
{
    "raw",
    "swap",
    "little",
    "big",
    "ulaw",
    "alaw",
    "text",
    "time",
    "freq",
    NULL,
};

static char *bit_list_strings[] =
{
    "8",
    "16",
    "24",
    "32",
    SW_FLOAT_BIT_STRING,
    "64",
    NULL,
};

static char *samp_rate_list_strings[] =
{
    "8000",
    "10000",
    "11025",
    "12000",
    "16000",
    "22050",
    "24000",
    "32000",
    "44100",
    "48000",
    "88200",
    "96000",
    NULL,
};

static char *channel_list_strings[] =
{
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "8",
    "10",
    "12",
    "16",
    "24",
    "32",
    NULL,
};

static void percentageTodBString(char *buf, double gain_percent)
{
    if (gain_percent <= 1.0e-10) {
	sprintf(buf, "-Inf");
    } else {
	sprintf(buf, "%.3f", dB(gain_percent/100.0));
    }
    spDebug(80, "percentageTodBString",
	    "gain_percent = %f, buf = %s\n", gain_percent, buf);
    
    return;
}

static double dBStringToPercentage(char *dB_string, double min, double max)
{
    double rate;
    
    if (strcaseeq(dB_string, "-Inf")) {
	if (min != max) {
	    rate = min;
	} else {
	    rate = 0.0;
	}
    } else {
	rate = 100.0 * pow(10.0, atof(dB_string) / 20.0);
	if (min != max) {
	    rate = MIN(rate, max);
	    rate = MAX(rate, min);
	}
    }
    
    return rate;
}

int swGetSampleBit(char *string)
{
    if (string == NULL) return 0;
    
    if (streq(string, SW_FLOAT_BIT_STRING)) {
	return 33;
    } else {
	return atoi(string);
    }
}

spBool swGetSampleBitString(char *string, int samp_bit)
{
    if (string == NULL) return SP_FALSE;
    
    if (samp_bit == 33) {
	strcpy(string, SW_FLOAT_BIT_STRING);
    } else {
	sprintf(string, "%d", samp_bit);
    }
    
    return SP_TRUE;
}

#ifdef SW_USE_PREFERENCE_DIALOG

static swLook sw_looks[] = {
    {SW_MY_LOOK_INDEX, SW_MY_LOOK_LABEL, "", "", "", "", "", ""},
    {SW_ORIGINAL_LOOK_INDEX, SW_ORIGINAL_LOOK_LABEL,
	 "black", "white", "red", "blue", "red", "grey50"},
    {SW_OSCILLO_LOOK_INDEX, SW_OSCILLO_LOOK_LABEL,
	 "aquamarine", "black", "orange", "white", "orange", "grey60"}, 
    {SW_MONOTONE_LOOK_INDEX, SW_MONOTONE_LOOK_LABEL,
	 "white", "black", "grey70", "grey80", "grey70", "grey60"},
    {SW_SKY_LOOK_INDEX, SW_SKY_LOOK_LABEL,
	 "white", /*"MediumBlue"*/"royalblue3", "yellow", "SkyBlue", "yellow", "LightBlue1"},
    {SW_FOREST_LOOK_INDEX, SW_FOREST_LOOK_LABEL,
	 "DarkGreen", /*"LimeGreen"*/"palegreen3", "yellow", "green", "yellow", "GreenYellow"},
    {SW_TOMATO_LOOK_INDEX, SW_TOMATO_LOOK_LABEL,
	 "white", /*"red"*/"tomato2", "green3", "orange", "green3", "pink"},
    {SW_MATLAB_LOOK_INDEX, SW_MATLAB_LOOK_LABEL,
	 "yellow", "black", "red", "blue", "red", "white"},
    {SW_WAVES_LOOK_INDEX, SW_WAVES_LOOK_LABEL,
	 "black", "#00afaf", "yellow", "white", "#9400d3", "#0000d2"},
};
static int sw_num_looks = 0;

#ifdef SW_USE_TOOL_BAR
void swShowAllToolBar(swWindow window, spBool flag)
{
    spComponent next = NULL;
    swWindow next_window = NULL;
    
    if (window == NULL) return;

    spDebug(10, "swShowAllToolBar", "in\n");

    if (window->tool_bar != NULL) {
	if (flag == SP_TRUE) {
	    spMapComponent(window->tool_bar);
	} else {
	    spUnmapComponent(window->tool_bar);
	}
    }
    
    next = window->window;
    while (1) {
	next = spGetNextWindow(next, SP_FALSE);
	if (next == NULL || next == window->window) {
	    break;
	}

	if ((next_window = (swWindow)spGetUserData(next)) != NULL) {
	    if (next_window->tool_bar != NULL) {
		if (flag == SP_TRUE) {
		    spMapComponent(next_window->tool_bar);
		} else {
		    spUnmapComponent(next_window->tool_bar);
		}
	    }
	}
    }

    return;
}
#endif

void swShowAllToolBarCB(spComponent component, swWindow window)
{
#ifdef SW_USE_TOOL_BAR
    if (window->config->no_tool_bar == SP_FALSE) {
	swShowAllToolBar(window, SP_FALSE);
	window->config->no_tool_bar = SP_TRUE;
    } else {
	swShowAllToolBar(window, SP_TRUE);
	window->config->no_tool_bar = SP_FALSE;
    }
#endif
    
    return;
}

static spBool swGetSizeFromTextField(spComponent component, int *pwidth, int *pheight)
{
    char *string;
    spBool sscanf_ok = SP_FALSE;
    spBool updated = SP_FALSE;
    
    if ((string = xspGetTextString(component)) != NULL) {
	int width = 0, height = 0;
	spDebug(10, "swGetSizeFromTextField", "size string = %s\n", string);
	if (sscanf(string, "%dx%d", &width, &height) == 2) {
	    sscanf_ok = SP_TRUE;
	} else if (sscanf(string, "%d %d", &width, &height) == 2) {
	    sscanf_ok = SP_TRUE;
	} else if (sscanf(string, "%d,%d", &width, &height) == 2) {
	    sscanf_ok = SP_TRUE;
	}
	
	if (sscanf_ok == SP_TRUE) {
	    if (width > 0 && pwidth != NULL) {
		*pwidth = width;
		updated = SP_TRUE;
	    }
	    if (height > 0 && pheight != NULL) {
		*pheight = height;
		updated = SP_TRUE;
	    }
	}
	    
	xfree(string);
    }

    return updated;
}

void swPopdownPreferenceDialogCB(spComponent component, swPrefDialog pref_dialog)
{
    int i;
    spBool flag = SP_FALSE;
    spBool set_color = SP_FALSE;
    spBool update_info_area_width = SP_FALSE;
    spBool use_tool_bar;
    int width;
    int index;
    char *string;
    spCallbackReason reason = SP_CR_NONE;

    if (pref_dialog == NULL || spIsCreated(pref_dialog->window) == SP_FALSE) return;

    reason = spGetCallbackReason(component);
    spDebug(10, "swPopdownPreferenceDialogCB", "reason = %d\n", reason);

    if (reason == SP_CR_OK || reason == SP_CR_CANCEL) {
	/* popdown preference dialog */
	spPopdownWindow(pref_dialog->window);
    }
    
    if (reason == SP_CR_OK || reason == SP_CR_APPLY) {
	spGetToggleState(pref_dialog->default_format_button, &pref_dialog->config->use_def_format);
	
	if ((string = xspGetTextString(pref_dialog->format_combo)) != NULL) {
	    spDebug(10, "swPopdownPreferenceDialogCB", "format string = %s\n", string);
	    spStrCopy(pref_dialog->config->def_format_string, SP_MAX_SETUP_VALUE, string);
	    xfree(string);
	}

	if ((string = xspGetTextString(pref_dialog->bit_combo)) != NULL) {
	    spDebug(10, "swPopdownPreferenceDialogCB", "bit string = %s\n", string);
	    pref_dialog->config->def_samp_bit = swGetSampleBit(string);
	    xfree(string);
	}
	
	if ((string = xspGetTextString(pref_dialog->samp_rate_combo)) != NULL) {
	    spDebug(10, "swPopdownPreferenceDialogCB", "samp_rate string = %s\n", string);
	    pref_dialog->config->def_samp_rate = atof(string);
	    xfree(string);
	}
	
	if ((string = xspGetTextString(pref_dialog->channel_combo)) != NULL) {
	    spDebug(10, "swPopdownPreferenceDialogCB", "channel string = %s\n", string);
	    pref_dialog->config->def_num_channel = atoi(string);
	    xfree(string);
	}

	if (spGetToggleState(pref_dialog->use_play_command_button,
			     &pref_dialog->config->use_play_command) == SP_TRUE) {
	    pref_dialog->config->wave_config->play_use_audio = 
		(pref_dialog->config->use_play_command == SP_TRUE ? SP_FALSE : SP_TRUE);
	}
	
	if ((string = xspGetTextString(pref_dialog->play_command_text)) != NULL) {
	    spDebug(10, "swPopdownPreferenceDialogCB", "play command string = %s\n", string);
	    spStrCopy(pref_dialog->config->play_command, SP_MAX_SETUP_VALUE, string);
	    spSetPlayCommand(pref_dialog->config->play_command);
	    xfree(string);
	}

#ifdef SP_SUPPORT_AUDIO
	if ((string = xspGetTextString(pref_dialog->audio_driver_combo)) != NULL) {
	    spStrCopy(pref_dialog->config->wave_config->audio_driver,
		      SP_MAX_PATHNAME, string);
	    spDebug(10, "swPopdownPreferenceDialogCB", "audio driver = %s\n",
		    pref_dialog->config->wave_config->audio_driver);
	    xfree(string);
	}
	if ((string = xspGetTextString(pref_dialog->buffer_size_combo)) != NULL) {
	    spDebug(10, "swPopdownPreferenceDialogCB", "buffer size = %s\n", string);
	    pref_dialog->config->audio_buffer_size = atoi(string);
	    pref_dialog->config->wave_config->audio_buffer_size = pref_dialog->config->audio_buffer_size;
	    xfree(string);
	}
	
	spGetToggleState(pref_dialog->use_loop_button, &pref_dialog->config->use_loop_play);
	spGetToggleState(pref_dialog->use_sync_button, &pref_dialog->config->use_sync_play);
	spGetToggleState(pref_dialog->play_draw_button, &pref_dialog->config->play_draw);
#endif
	
#ifdef SW_USE_THREAD
	if (pref_dialog->config->wave_config->thread_safe == SP_TRUE) {
	    spGetToggleState(pref_dialog->use_thread_button,
			     &pref_dialog->config->wave_config->process_use_thread);

#if 0	    
	    if (spIsThreadLevelVariable() == SP_TRUE) {
		spGetToggleState(pref_dialog->use_lowlevel_thread_button,
				 &pref_dialog->config->use_lowlevel_thread);
	    }
#endif
	}
#endif

	spGetToggleState(pref_dialog->display_info_area_button,
			 &pref_dialog->config->display_info_area);
	spGetToggleState(pref_dialog->display_freq_info_area_button,
			 &pref_dialog->config->display_freq_info_area);
	spGetToggleState(pref_dialog->link_region_label_button,
			 &pref_dialog->config->link_region_label);
	spGetToggleState(pref_dialog->erase_label_prompt_button,
			 &pref_dialog->config->erase_label_prompt);
	spGetToggleState(pref_dialog->show_dialog_in_first_label_save_button,
			 &pref_dialog->config->show_dialog_in_first_label_save);
	spGetToggleState(pref_dialog->add_default_label_suffix_for_save_as_button,
			 &pref_dialog->config->add_default_label_suffix_for_save_as);

#if 1
	spGetToggleState(pref_dialog->load_default_label_button,
			 &pref_dialog->config->wave_config->load_default_label);
	if ((string = xspGetTextString(pref_dialog->default_label_suffix_combo)) != NULL) {
	    spDebug(10, "swPopdownPreferenceDialogCB", "default_label_suffix = %s\n", string);
	    spStrCopy(pref_dialog->config->wave_config->default_label_suffix, SP_MAX_SETUP_VALUE, string);
	    xfree(string);
	}
	if ((string = xspGetTextString(pref_dialog->default_region_label_suffix_combo)) != NULL) {
	    spDebug(10, "swPopdownPreferenceDialogCB", "default_region_label_suffix = %s\n", string);
	    spStrCopy(pref_dialog->config->wave_config->default_region_label_suffix, SP_MAX_SETUP_VALUE, string);
	    xfree(string);
	}
#endif
        
#ifdef SW_SUPPORT_AUTOSAVE
	if ((string = xspGetTextString(pref_dialog->autosave_dir_text)) != NULL) {
	    spDebug(10, "swPopdownPreferenceDialogCB", "autosave_dir = %s\n", string);
	    spStrCopy(pref_dialog->config->autosave_dir, SP_MAX_SETUP_VALUE, string);
	    xfree(string);
	}
	if ((index = spGetSelectedListIndex(pref_dialog->autosave_id_type_combo)) >= 0) {
	    spDebug(10, "swPopdownPreferenceDialogCB", "autosave ID = %d\n", index);
	    pref_dialog->config->autosave_id_type = index;
	}
	if ((string = xspGetTextString(pref_dialog->autosave_id_prefix_combo)) != NULL) {
	    spDebug(10, "swPopdownPreferenceDialogCB", "autosave_id_prefix = %s\n", string);
	    spStrCopy(pref_dialog->config->autosave_id_prefix, SP_MAX_SETUP_VALUE, string);
	    xfree(string);
	}
	if ((string = xspGetTextString(pref_dialog->autosave_label_suffix_combo)) != NULL) {
	    spDebug(10, "swPopdownPreferenceDialogCB", "autosave_label_suffix = %s\n", string);
	    spStrCopy(pref_dialog->config->autosave_label_suffix, SP_MAX_SETUP_VALUE, string);
	    xfree(string);
	}
	spGetToggleState(pref_dialog->autosave_label_fullpath_button,
			 &pref_dialog->config->autosave_label_fullpath);
	spGetToggleState(pref_dialog->autosave_by_drop_button,
			 &pref_dialog->config->autosave_by_drop);
#endif

	if ((string = xspGetTextString(pref_dialog->canvas_font_field)) != NULL) {
	    if (!streq(string, pref_dialog->config->canvas_font)) {
		spStrCopy(pref_dialog->config->canvas_font, SP_MAX_SETUP_VALUE, string);
		flag = SP_TRUE;
		set_color = SP_TRUE;
	    }
	    xfree(string);
	}
	
	swGetSizeFromTextField(pref_dialog->size_text,
			       &pref_dialog->config->width, &pref_dialog->config->height);
#ifdef SW_SUPPORT_PRINT
	swGetSizeFromTextField(pref_dialog->print_size_text,
			       &pref_dialog->config->print_width, &pref_dialog->config->print_height);
#endif

	if ((string = xspGetTextString(pref_dialog->info_area_width_text)) != NULL) {
	    width = atoi(string);
	    if (width > 0 && width != pref_dialog->config->info_area_width) {
		pref_dialog->config->info_area_width = width;
		update_info_area_width = SP_TRUE;
	    }
	    xfree(string);
	}
	
	spGetToggleState(pref_dialog->alt_ctrl_swap_button,
			 &pref_dialog->config->alt_ctrl_swap);
	spGetToggleState(pref_dialog->scroll_left_by_wheel_down_button,
			 &pref_dialog->config->scroll_left_by_wheel_down);
	
#ifdef SW_USE_TOOL_BAR
	if (spGetToggleState(pref_dialog->use_tool_bar_button, &use_tool_bar) == SP_TRUE) {
	    if (use_tool_bar == SP_TRUE) {
		pref_dialog->config->no_tool_bar = SP_FALSE;
	    } else {
		pref_dialog->config->no_tool_bar = SP_TRUE;
	    }
	    swShowAllToolBar(pref_dialog->config->toplevel->current_window, use_tool_bar);
	    /*flag = SP_TRUE;*/
	}
#endif
	
	if (spGetToggleState(pref_dialog->display_meter_button,
			     &pref_dialog->config->display_meter) == SP_TRUE) {
	    flag = SP_TRUE;
	}
	if (spGetToggleState(pref_dialog->draw_selection_times_button,
			     &pref_dialog->config->draw_selection_times) == SP_TRUE) {
	    flag = SP_TRUE;
	}
	if (spGetToggleState(pref_dialog->draw_selection_length_button,
			     &pref_dialog->config->draw_selection_length) == SP_TRUE) {
	    flag = SP_TRUE;
	}
	if (spGetToggleState(pref_dialog->draw_scale_button, &pref_dialog->config->scale_flag) == SP_TRUE) {
	    flag = SP_TRUE;
	}
	if (spGetToggleState(pref_dialog->draw_grid_button, &pref_dialog->config->grid_flag) == SP_TRUE) {
	    flag = SP_TRUE;
	}
	if (spGetToggleState(pref_dialog->percent_amplitude_button, &pref_dialog->config->percent_amplitude) == SP_TRUE) {
	    flag = SP_TRUE;
	}
#ifdef SW_SUPPORT_LOG_FREQUENCY_AXIS
	if (spGetToggleState(pref_dialog->log_frequency_axis_button, &pref_dialog->config->log_frequency_axis) == SP_TRUE) {
	    flag = SP_TRUE;
	}
#endif
	if (spGetToggleState(pref_dialog->draw_spectrogram_keys_as_default_button, &pref_dialog->config->draw_piano_keys_for_specgram) == SP_TRUE) {
	    flag = SP_TRUE;
	}
	if (spGetToggleState(pref_dialog->draw_spectrogram_keys_right_button, &pref_dialog->config->vertical_piano_keys_right) == SP_TRUE) {
	    flag = SP_TRUE;
	}
	if (spGetToggleState(pref_dialog->draw_horizontal_keys_as_default_button, &pref_dialog->config->draw_piano_keys_for_spectrum) == SP_TRUE) {
	    flag = SP_TRUE;
	}
	if (spGetToggleState(pref_dialog->draw_horizontal_keys_top_button, &pref_dialog->config->horizontal_piano_keys_top) == SP_TRUE) {
	    flag = SP_TRUE;
	}
	
	if ((index = spGetSelectedListIndex(pref_dialog->look_like_combo)) >= 0) {
	    if (sw_num_looks <= 0) {
		sw_num_looks = spArraySize(sw_looks);
	    }

	    for (i = 1; i < sw_num_looks; i++) {
		if (index == sw_looks[i].id) {
		    strcpy(pref_dialog->config->wave_fg, sw_looks[i].wave_fg);
		    strcpy(pref_dialog->config->wave_bg, sw_looks[i].wave_bg);
		    strcpy(pref_dialog->config->pointer_color, sw_looks[i].pointer_color);
		    strcpy(pref_dialog->config->label_color, sw_looks[i].label_color);
		    strcpy(pref_dialog->config->string_color, sw_looks[i].string_color);
		    strcpy(pref_dialog->config->scale_color, sw_looks[i].scale_color);
		    flag = SP_TRUE;
		    set_color = SP_TRUE;
		    break;
		}
	    }
	}

	if (set_color == SP_TRUE) {
	    swSetColor(pref_dialog->config);
	}
	
	if (update_info_area_width == SP_TRUE) {
	    spDebug(10, "swPopdownPreferenceDialogCB", "call swUpdateAllInfoAreaDisplay\n");
	    swUpdateAllInfoAreaDisplay(pref_dialog->config->toplevel->current_window);
	} else if (flag == SP_TRUE) {
	    spDebug(10, "swPopdownPreferenceDialogCB", "call swDrawAllWave\n");
	    swDrawAllWave(pref_dialog->config->toplevel->current_window);
	}
    }
    
    return;
}

static int selectFormatItem(spComponent combo, char *format_string)
{
    int i;
    int index = -1;
    
    for (i = 0; format_list_strings[i] != NULL; i++) {
	if (strcaseeq(format_string, format_list_strings[i])
	    || strcaseeq(format_string, format_list_short_strings[i])) {
	    spSelectListIndex(combo, i);
	    index = i;
	    break;
	}
    }

    return index;
}

static void createPreferenceFileTab(swPrefDialog pref_dialog, spComponent tab_box)
{
    char string[SP_MAX_LINE];
    
    pref_dialog->file_tab = spAddTabItem(tab_box, "fileTab", -1,
					 SppTitle, SW_FILE_TAB_LABEL,
					 SppHelpPath, "dialog/preference.html#file",
					 NULL);
    
    /* create check box to set whether you use the default format */
    pref_dialog->default_format_button = spCreateCheckBox(pref_dialog->file_tab, "useDefaultFormatButton",
							  SppTitle, SW_USE_DEFAULT_FORMAT_LABEL,
							  SppSet, pref_dialog->config->use_def_format,
							  SppHelpPath, "dialog/preference.html#use_default_format",
							  NULL);

    /* create combo box to input the wave format */
    pref_dialog->format_combo = spCreateParamField(pref_dialog->file_tab, "formatComboBox", 60,
						   SppTitle, SW_FORMAT_LABEL,
						   SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
						   SppEditable, SP_FALSE,
						   SppFieldStrings, format_list_strings,
						   SppFieldOffset, 140,
						   SppFieldSize, 160,
						   SppHelpPath, "dialog/preference.html#file_format",
						   NULL);
    selectFormatItem(pref_dialog->format_combo, pref_dialog->config->def_format_string);
    
    /* create combo box to input bits per sample */
    pref_dialog->bit_combo = spCreateParamField(pref_dialog->file_tab, "bitComboBox", 60,
						SppTitle, SW_BIT_LABEL,
						SppDimension, "bits/sample",
						SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
						SppEditable, SP_FALSE,
						SppFieldStrings, bit_list_strings,
						SppFieldOffset, 140,
						SppFieldSize, 160,
						SppHelpPath, "dialog/preference.html#file_bit",
						NULL);
    swGetSampleBitString(string, pref_dialog->config->def_samp_bit);
    spSelectListItem(pref_dialog->bit_combo, string);
    
    /* create combo box to input sampling frequency */
    pref_dialog->samp_rate_combo = spCreateParamField(pref_dialog->file_tab, "sampRateComboBox", 60,
						      SppTitle, SW_SAMP_RATE_LABEL,
						      SppDimension, "Hz",
						      SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
						      SppEditable, SP_TRUE,
						      SppFieldStrings, samp_rate_list_strings,
						      SppFieldOffset, 140,
						      SppFieldSize, 160,
						      SppHelpPath, "dialog/preference.html#file_samp_rate",
						      NULL);
    sprintf(string, "%.0f", pref_dialog->config->def_samp_rate);
    spSetTextString(pref_dialog->samp_rate_combo, string);
    
    /* create combo box to input a number of channel */
    pref_dialog->channel_combo = spCreateParamField(pref_dialog->file_tab, "channelComboBox", 60,
						    SppTitle, SW_CHANNEL_LABEL,
						    SppDimension, "channels",
						    SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
						    SppEditable, SP_TRUE,
						    SppFieldStrings, channel_list_strings,
						    SppFieldOffset, 140,
						    SppFieldSize, 160,
						    SppHelpPath, "dialog/preference.html#file_channel",
						    NULL);
    sprintf(string, "%d", pref_dialog->config->def_num_channel);
    spSetTextString(pref_dialog->channel_combo, string);

    return;
}

static void displayRestartWarningCB(spComponent component, void *data)
{
    spDisplayWarning(component, NULL, SW_NEED_RESTART_WARNING_MESSAGE);
    return;
}

static void createPreferenceLookTab(swPrefDialog pref_dialog, spComponent tab_box)
{
    int i;
    char string[SP_MAX_LINE];
    static char **list_strings = NULL;

    if (list_strings == NULL) {
	if (sw_num_looks <= 0) {
	    sw_num_looks = spArraySize(sw_looks);
	}
	list_strings = xalloc(sw_num_looks + 1, char *);
	for (i = 0; i < sw_num_looks; i++) {
	    list_strings[i] = sw_looks[i].label;
	}
	list_strings[sw_num_looks] = NULL;
    }
    
    pref_dialog->look_tab = spAddTabItem(tab_box, "lookTab", -1,
					 SppTitle, SW_LOOK_TAB_LABEL,
					 SppHelpPath, "dialog/preference.html#look",
					 NULL);
    
    /* create combo box to input the look name */
    pref_dialog->look_like_combo = spCreateParamField(pref_dialog->look_tab, "lookComboBox", 60,
						      SppTitle, SW_LOOK_LIKE_LABEL,
						      SppDimension, SW_LOOK_LIKE_DIMENSION,
						      SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
						      SppEditable, SP_FALSE,
						      SppFieldStrings, list_strings,
						      SppFieldOffset, 140,
						      SppFieldSize, 160,
						      SppHelpPath, "dialog/preference.html#look_look",
						      NULL);
    spSelectListIndex(pref_dialog->look_like_combo, 0);

    pref_dialog->canvas_font_field = spCreateParamField(pref_dialog->look_tab, "canvasFontField", 60,
							SppTitle, SW_CANVAS_FONT_LABEL,
							SppFieldType, SP_FIELD_TYPE_FONT_TEXT,
							SppFieldOffset, 140,
							SppFieldSize, 180,
							SppEditable, SP_FALSE,
							SppInitialFont, pref_dialog->config->canvas_font,
							SppHelpPath, "dialog/preference.html#canvas_font",
							NULL);
    
    /* create text field to input window size */
    pref_dialog->size_text = spCreateParamField(pref_dialog->look_tab, "sizeText", 60,
						SppTitle, SW_WINDOW_SIZE_LABEL,
						SppFieldType, SP_FIELD_TYPE_TEXT,
						SppEditable, SP_TRUE,
						SppFieldOffset, 140,
						SppFieldSize, 160,
						SppHelpPath, "dialog/preference.html#window_size",
						NULL);
    sprintf(string, "%dx%d", pref_dialog->config->width, pref_dialog->config->height);
    spSetTextString(pref_dialog->size_text, string);

#ifdef SW_SUPPORT_PRINT
    /* create text field to input size for print */
    pref_dialog->print_size_text = spCreateParamField(pref_dialog->look_tab, "printSizeText", 60,
						      SppTitle, SW_PRINT_SIZE_LABEL,
						      SppFieldType, SP_FIELD_TYPE_TEXT,
						      SppEditable, SP_TRUE,
						      SppFieldOffset, 140,
						      SppFieldSize, 160,
						      SppHelpPath, "dialog/preference.html#print_size",
						      NULL);
    sprintf(string, "%dx%d", pref_dialog->config->print_width, pref_dialog->config->print_height);
    spSetTextString(pref_dialog->print_size_text, string);
#endif
    
    /* create text field to input width of information area */
    sprintf(string, "%d", pref_dialog->config->info_area_width);
    pref_dialog->info_area_width_text = spCreateParamField(pref_dialog->look_tab, "infoAreaWidthText", 60,
							   SppTitle, SW_INFO_AREA_WIDTH_LABEL,
							   SppFieldType, SP_FIELD_TYPE_TEXT,
							   SppEditable, SP_TRUE,
							   SppFieldOffset, 140,
							   SppFieldSize, 160,
							   SppTextString, string,
							   SppHelpPath, "dialog/preference.html#info_area_width",
							   NULL);

    pref_dialog->alt_ctrl_swap_button = spCreateCheckBox(pref_dialog->look_tab, "altCtrlSwapButton",
							 SppTitle, SW_ALT_CTRL_SWAP_LABEL,
							 SppSet, pref_dialog->config->alt_ctrl_swap,
							 SppCallbackFunc, displayRestartWarningCB,
							 SppHelpPath, "dialog/preference.html#alt_ctrl_swap",
							 NULL);
    pref_dialog->scroll_left_by_wheel_down_button = spCreateCheckBox(pref_dialog->look_tab, "scrollLeftByWheelDownButton",
								     SppTitle, SW_SCROLL_LEFT_BY_WHEEL_DOWN_LABEL,
								     SppSet, pref_dialog->config->scroll_left_by_wheel_down,
								     SppHelpPath, "dialog/preference.html#scroll_left_by_wheel_down",
								     NULL);
    
#ifdef SW_USE_TOOL_BAR
    /* create check box to set whether you use the tool bar */
    pref_dialog->use_tool_bar_button = spCreateCheckBox(pref_dialog->look_tab, "useToolBarButton",
							SppTitle, SW_USE_TOOL_BAR_LABEL,
							SppSet, (pref_dialog->config->no_tool_bar == SP_TRUE ? SP_FALSE : SP_TRUE),
							SppHelpPath, "dialog/preference.html#use_tool_bar",
							NULL);
#endif
    
    return;
}

static void createPreferenceDisplayTab(swPrefDialog pref_dialog, spComponent tab_box)
{
    pref_dialog->display_tab = spAddTabItem(tab_box, "displayTab", -1,
					    SppTitle, SW_DISPLAY_TAB_LABEL,
					    SppHelpPath, "dialog/preference.html#display",
					    NULL);

    /* create check box to set whether you draw the following */
    pref_dialog->display_meter_button = spCreateCheckBox(pref_dialog->display_tab, "displayMeterButton",
							 SppTitle, SW_DISPLAY_METER_LABEL,
							 SppSet, pref_dialog->config->display_meter,
							 SppHelpPath, "dialog/preference.html#display_meter",
							 NULL);
    pref_dialog->draw_selection_times_button = spCreateCheckBox(pref_dialog->display_tab, "drawSelectionTimesButton",
								SppTitle, SW_DRAW_SELECTION_TIMES_LABEL,
								SppSet, pref_dialog->config->draw_selection_times,
								SppHelpPath, "dialog/preference.html#draw_selection_times",
								NULL);
    pref_dialog->draw_selection_length_button = spCreateCheckBox(pref_dialog->display_tab, "drawSelectionLengthButton",
								 SppTitle, SW_DRAW_SELECTION_LENGTH_LABEL,
								 SppSet, pref_dialog->config->draw_selection_length,
								 SppHelpPath, "dialog/preference.html#draw_selection_length",
								 NULL);
    pref_dialog->draw_scale_button = spCreateCheckBox(pref_dialog->display_tab, "drawScaleButton",
						      SppTitle, SW_DRAW_SCALE_LABEL,
						      SppSet, pref_dialog->config->scale_flag,
						      SppHelpPath, "dialog/preference.html#draw_scale",
						      NULL);
    pref_dialog->draw_grid_button = spCreateCheckBox(pref_dialog->display_tab, "drawGridButton",
						     SppTitle, SW_DRAW_GRID_LABEL,
						     SppSet, pref_dialog->config->grid_flag,
						     SppHelpPath, "dialog/preference.html#draw_grid",
						     NULL);
    pref_dialog->percent_amplitude_button = spCreateCheckBox(pref_dialog->display_tab, "percentAmplitudeButton",
							     SppTitle, SW_PERCENT_AMPLITUDE_LABEL,
							     SppSet, pref_dialog->config->percent_amplitude,
							     SppHelpPath, "dialog/preference.html#percent_amplitude",
							     NULL);

#ifdef SW_SUPPORT_LOG_FREQUENCY_AXIS
    pref_dialog->log_frequency_axis_button = spCreateCheckBox(pref_dialog->display_tab, "logFrequencyAxisButton",
                                                              SppTitle, SW_LOG_FREQUENCY_AXIS_LABEL,
                                                              SppSet, pref_dialog->config->log_frequency_axis,
                                                              SppHelpPath, "dialog/preference.html#log_frequency_axis",
                                                              NULL);
#endif
    pref_dialog->draw_spectrogram_keys_as_default_button = spCreateCheckBox(pref_dialog->display_tab, "drawSpectrogramKeysAsDefaultButton",
                                                                            SppTitle, SW_DRAW_SPECTROGRAM_KEYS_AS_DEFAULT_LABEL,
                                                                            SppSet, pref_dialog->config->draw_piano_keys_for_specgram,
                                                                            SppHelpPath, "dialog/preference.html#draw_spectrogram_keys_as_default",
                                                                            NULL);
    pref_dialog->draw_spectrogram_keys_right_button = spCreateCheckBox(pref_dialog->display_tab, "drawSpectrogramKeysRightButton",
                                                                            SppTitle, SW_DRAW_SPECTROGRAM_KEYS_RIGHT_LABEL,
                                                                            SppSet, pref_dialog->config->vertical_piano_keys_right,
                                                                            SppHelpPath, "dialog/preference.html#draw_spectrogram_keys_right",
                                                                            NULL);
    pref_dialog->draw_horizontal_keys_as_default_button = spCreateCheckBox(pref_dialog->display_tab, "drawHorizontalKeysAsDefaultButton",
                                                                           SppTitle, SW_DRAW_HORIZONTAL_KEYS_AS_DEFAULT_LABEL,
                                                                           SppSet, pref_dialog->config->draw_piano_keys_for_spectrum,
                                                                           SppHelpPath, "dialog/preference.html#draw_horizontal_keys_as_default",
                                                                           NULL);
    pref_dialog->draw_horizontal_keys_top_button = spCreateCheckBox(pref_dialog->display_tab, "drawHorizontalKeysRightButton",
                                                                    SppTitle, SW_DRAW_HORIZONTAL_KEYS_TOP_LABEL,
                                                                    SppSet, pref_dialog->config->horizontal_piano_keys_top,
                                                                    SppHelpPath, "dialog/preference.html#draw_horizontal_keys_top",
                                                                    NULL);
    
    return;
}

static char **getAudioDriverList(void)
{
    int l;
    int index;
    int num_device;
    char *device_name;
    static char **list = NULL;

    if (list == NULL) {
	num_device = spGetNumAudioDriverDevice(NULL);
	spMessage("number of driver device = %d\n", num_device);

	list = xalloc(num_device + 1, char *);

	index = 0;
	for (l = 0; l < num_device; l++) {
	    if ((device_name = xspGetAudioDriverDeviceName(NULL, l)) != NULL) {
		spDebug(50, "getAudioDriverList", "driver device %d = %s\n", l, device_name);
		list[index] = device_name;
		index++;
	    }
	}
	list[index] = NULL;
    }

    
    return list;
}

static void createPreferenceSoundTab(swPrefDialog pref_dialog, spComponent tab_box)
{
    char **list;
    char string[SP_MAX_SETUP_VALUE];
    static char *buffer_list_strings[] =
    {
	"256",
	"512",
	"1024",
	"2048",
	"4096",
	"8192",
	"16384",
	"32768",
	"65536",
	NULL,
    };

    pref_dialog->sound_tab = spAddTabItem(tab_box, "soundTab", -1,
					  SppTitle, SW_SOUND_TAB_LABEL,
					  SppHelpPath, "dialog/preference.html#sound",
					  NULL);
    
    /* create check box to set whether you use the play command */
    pref_dialog->use_play_command_button = spCreateCheckBox(pref_dialog->sound_tab, "usePlayCommandButton",
							    SppTitle, SW_USE_PLAY_COMMAND_LABEL,
							    SppSet, pref_dialog->config->use_play_command,
							    SppHelpPath, "dialog/preference.html#use_play_command",
							    NULL);
    
    /* create text field to input play command */
    pref_dialog->play_command_text = spCreateParamField(pref_dialog->sound_tab, "playCommandText", 60,
							SppTitle, SW_PLAY_COMMAND_LABEL,
							SppFieldType, SP_FIELD_TYPE_TEXT,
							SppEditable, SP_TRUE,
							SppFieldOffset, 140,
							SppFieldSize, 240,
							SppHelpPath, "dialog/preference.html#play_command",
							NULL);
    spStrCopy(string, SP_MAX_SETUP_VALUE, pref_dialog->config->play_command);
    spSetTextString(pref_dialog->play_command_text, string);
    
#ifdef SP_SUPPORT_AUDIO
    list = getAudioDriverList();
    pref_dialog->audio_driver_combo = spCreateParamField(pref_dialog->sound_tab, "audioDriverComboBox", 60,
							 SppTitle, SW_AUDIO_DRIVER_LABEL,
							 SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
							 SppEditable, SP_FALSE,
							 SppFieldStrings, list,
							 SppFieldOffset, 140,
							 SppFieldSize, 240,
							 SppHelpPath, "dialog/preference.html#audio_driver",
							 NULL);
    if (!strnone(pref_dialog->config->wave_config->audio_driver)) {
	spSelectListItem(pref_dialog->audio_driver_combo,
			 pref_dialog->config->wave_config->audio_driver);
    }
    
    /* create combo box to input buffer size */
    pref_dialog->buffer_size_combo = spCreateParamField(pref_dialog->sound_tab, "bufferSizeComboBox", 60,
							SppTitle, SW_BUFFER_SIZE_LABEL,
							SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
							SppEditable, SP_TRUE,
							SppFieldStrings, buffer_list_strings,
							SppFieldOffset, 140,
							SppFieldSize, 160,
							SppHelpPath, "dialog/preference.html#audio_buffer_size",
							NULL);
    sprintf(string, "%d", pref_dialog->config->audio_buffer_size);
    spSetTextString(pref_dialog->buffer_size_combo, string);

    pref_dialog->use_loop_button = spCreateCheckBox(pref_dialog->sound_tab, "useLoopButton",
						    SppTitle, SW_USE_LOOP_PLAY_LABEL,
						    SppSet, pref_dialog->config->use_loop_play,
						    SppHelpPath, "dialog/preference.html#use_loop_play",
						    NULL);
    pref_dialog->use_sync_button = spCreateCheckBox(pref_dialog->sound_tab, "useSyncButton",
						    SppTitle, SW_USE_SYNC_PLAY_LABEL,
						    SppSet, pref_dialog->config->use_sync_play,
						    SppHelpPath, "dialog/preference.html#use_sync_play",
						    NULL);
    
    pref_dialog->play_draw_button = spCreateCheckBox(pref_dialog->sound_tab, "playDrawButton",
						     SppTitle, SW_PLAY_DRAW_LABEL,
						     SppSet, pref_dialog->config->play_draw,
						     SppHelpPath, "dialog/preference.html#play_draw",
						     NULL);
#endif

#ifdef SW_USE_THREAD
    if (pref_dialog->config->wave_config->thread_safe == SP_TRUE) {
	pref_dialog->use_thread_button = spCreateCheckBox(pref_dialog->sound_tab, "useThreadButton",
							  SppTitle, SW_USE_THREAD_LABEL,
							  SppSet, pref_dialog->config->wave_config->process_use_thread,
							  SppHelpPath, "dialog/preference.html#process_use_thread",
							  NULL);
#if 0
	if (spIsThreadLevelVariable() == SP_TRUE) {
	    pref_dialog->use_lowlevel_thread_button = spCreateCheckBox(pref_dialog->sound_tab, "useLowLevelThreadButton",
						       SppTitle, SW_USE_LOWLEVEL_THREAD_LABEL,
						       SppSet, pref_dialog->config->use_lowlevel_thread,
						       SppCallbackFunc, displayRestartWarningCB,
						       SppHelpPath, "dialog/preference.html#use_lowlevel_thread",
						       NULL);
	}
#endif
    }
#endif
    
    return;
}

static void createPreferenceLabelTab(swPrefDialog pref_dialog, spComponent tab_box)
{
#if 1
    static char *label_suffix_list[] =
    {
	".txt",
	".lab",
	"_lab.txt",
	"_label.txt",
	"",
	NULL,
    };
    static char *region_label_suffix_list[] =
    {
	"_reg.txt",
	"_region.txt",
	"_reglab.txt",
	"_regionlabel.txt",
	"",
	NULL,
    };
#endif
    
    pref_dialog->label_tab = spAddTabItem(tab_box, "labelTab", -1,
					  SppTitle, SW_LABEL_TAB_LABEL,
					  SppHelpPath, "dialog/preference.html#label",
					  NULL);
    
    pref_dialog->display_info_area_button = spCreateCheckBox(pref_dialog->label_tab, "displayInfoAreaButton",
							     SppTitle, SW_DISPLAY_INFO_AREA_LABEL,
							     SppSet, pref_dialog->config->display_info_area,
							     SppHelpPath, "dialog/preference.html#display_info_area",
							     NULL);

    pref_dialog->display_freq_info_area_button = spCreateCheckBox(pref_dialog->label_tab, "displayFreqInfoAreaButton",
								  SppTitle, SW_DISPLAY_FREQ_INFO_AREA_LABEL,
								  SppSet, pref_dialog->config->display_freq_info_area,
								  SppHelpPath, "dialog/preference.html#display_freq_info_area",
								  NULL);

    pref_dialog->link_region_label_button = spCreateCheckBox(pref_dialog->label_tab, "linkRegionLabelButton",
							     SppTitle, SW_LINK_REGION_LABEL_LABEL,
							     SppSet, pref_dialog->config->link_region_label,
							     SppHelpPath, "dialog/preference.html#link_region_label",
							     NULL);
    
    pref_dialog->erase_label_prompt_button = spCreateCheckBox(pref_dialog->label_tab, "eraseLabelPromptButton",
							      SppTitle, SW_ERASE_LABEL_PROMPT_LABEL,
							      SppSet, pref_dialog->config->erase_label_prompt,
							      SppHelpPath, "dialog/preference.html#erase_label_prompt",
							      NULL);
    pref_dialog->show_dialog_in_first_label_save_button = spCreateCheckBox(pref_dialog->label_tab, "showDialogInFirstLabelSaveButton",
                                                                           SppTitle, SW_SHOW_DIALOG_IN_FIRST_LABEL_SAVE_LABEL,
                                                                           SppSet, pref_dialog->config->show_dialog_in_first_label_save,
                                                                           SppHelpPath, "dialog/preference.html#show_dialog_in_first_label_save",
                                                                           NULL);
    pref_dialog->add_default_label_suffix_for_save_as_button = spCreateCheckBox(pref_dialog->label_tab, "addDefaultLabelSuffixForSaveAsButton",
                                                                                SppTitle, SW_ADD_DEFAULT_LABEL_SUFFIX_FOR_SAVE_AS_LABEL,
                                                                                SppSet, pref_dialog->config->add_default_label_suffix_for_save_as,
                                                                                //SppHelpPath, "dialog/preference.html#add_default_label_suffix_for_save_as",
                                                                                NULL);

#if 1
    pref_dialog->load_default_label_button = spCreateCheckBox(pref_dialog->label_tab, "loadDefaultLabelButton",
							      SppTitle, SW_LOAD_DEFAULT_LABEL_LABEL,
							      SppSet, pref_dialog->config->wave_config->load_default_label,
							      SppHelpPath, "dialog/preference.html#load_default_label",
							      NULL);
    pref_dialog->default_label_suffix_combo =
	spCreateParamField(pref_dialog->label_tab, "defaultLabelSuffixCombo", 60,
			   SppTitle, SW_DEFAULT_LABEL_SUFFIX_LABEL,
			   SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
			   SppEditable, SP_TRUE,
			   SppFieldStrings, label_suffix_list,
			   SppFieldOffset, 180,
			   SppFieldSize, 160,
			   SppHelpPath, "dialog/preference.html#default_label_suffix",
			   NULL);
    spSetTextString(pref_dialog->default_label_suffix_combo,
		    pref_dialog->config->wave_config->default_label_suffix);

    pref_dialog->default_region_label_suffix_combo =
	spCreateParamField(pref_dialog->label_tab, "defaultRegionLabelSuffixCombo", 60,
			   SppTitle, SW_DEFAULT_REGION_LABEL_SUFFIX_LABEL,
			   SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
			   SppEditable, SP_TRUE,
			   SppFieldStrings, region_label_suffix_list,
			   SppFieldOffset, 180,
			   SppFieldSize, 160,
			   SppHelpPath, "dialog/preference.html#default_region_label_suffix",
			   NULL);
    spSetTextString(pref_dialog->default_region_label_suffix_combo,
		    pref_dialog->config->wave_config->default_region_label_suffix);
#endif
    
    return;
}

static void createPreferenceAutosaveTab(swPrefDialog pref_dialog, spComponent tab_box)
{
#ifdef SW_SUPPORT_AUTOSAVE
    static char *id_prefix_list[] =
    {
	"",
	"-",
	"_",
	NULL,
    };
    static char *id_type_list[] =
    {
	"2-Digit Serial Number",
	"3-Digit Serial Number",
	"4-Digit Serial Number",
	"Serial Number",
	"Edge Points",
	NULL,
    };
    static char *label_suffix_list[] =
    {
	"_cut.txt",
	".cut",
	".lab",
	NULL,
    };

    pref_dialog->autosave_tab = spAddTabItem(tab_box, "autosaveTab", -1,
					     SppTitle, SW_AUTOSAVE_TAB_LABEL,
					     SppHelpPath, "dialog/preference.html#autosave",
					     NULL);
    
    /* create text field to input directory */
    pref_dialog->autosave_dir_text =
	spCreateParamField(pref_dialog->autosave_tab, "autosaveDirText", 60,
			   SppTitle, SW_AUTOSAVE_DIR_LABEL,
			   SppFieldType, SP_FIELD_TYPE_DIR_TEXT,
			   SppEditable, SP_TRUE,
			   SppFieldOffset, 140,
			   SppFieldSize, 180,
			   SppDimensionSize, 60,
			   SppHelpPath, "dialog/preference.html#autosave_dir",
			   NULL);
    spSetTextString(pref_dialog->autosave_dir_text,
		    pref_dialog->config->autosave_dir);

    /* create combo box to input the ID type */
    pref_dialog->autosave_id_type_combo =
	spCreateParamField(pref_dialog->autosave_tab, "autosaveIdTypeCombo", 60,
			   SppTitle, SW_AUTOSAVE_ID_TYPE_LABEL,
			   SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
			   SppEditable, SP_FALSE,
			   SppFieldStrings, id_type_list,
			   SppFieldOffset, 140,
			   SppFieldSize, 180,
			   SppHelpPath, "dialog/preference.html#autosave_id_type",
			   NULL);
    spSelectListIndex(pref_dialog->autosave_id_type_combo,
		      pref_dialog->config->autosave_id_type);
    
    /* create combo box to input the ID prefix */
    pref_dialog->autosave_id_prefix_combo =
	spCreateParamField(pref_dialog->autosave_tab, "autosaveIdPrefixCombo", 60,
			   SppTitle, SW_AUTOSAVE_ID_PREFIX_LABEL,
			   SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
			   SppEditable, SP_TRUE,
			   SppFieldStrings, id_prefix_list,
			   SppFieldOffset, 140,
			   SppFieldSize, 160,
			   SppHelpPath, "dialog/preference.html#autosave_id_prefix",
			   NULL);
    spSetTextString(pref_dialog->autosave_id_prefix_combo,
		    pref_dialog->config->autosave_id_prefix);

    /* create combo box to input the suffix of a label file */
    pref_dialog->autosave_label_suffix_combo =
	spCreateParamField(pref_dialog->autosave_tab, "autosaveLabelSuffixCombo", 60,
			   SppTitle, SW_AUTOSAVE_LABEL_SUFFIX_LABEL,
			   SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
			   SppEditable, SP_TRUE,
			   SppFieldStrings, label_suffix_list,
			   SppFieldOffset, 140,
			   SppFieldSize, 160,
			   SppHelpPath, "dialog/preference.html#autosave_label_suffix",
			   NULL);
    spSetTextString(pref_dialog->autosave_label_suffix_combo,
		    pref_dialog->config->autosave_label_suffix);
    
    /* create check box to set whether you write the fullpath into a label file */
    pref_dialog->autosave_label_fullpath_button =
	spCreateCheckBox(pref_dialog->autosave_tab, "autosaveLabelFullpathButton",
			 SppTitle, SW_AUTOSAVE_LABEL_FULLPATH_LABEL,
			 SppSet, pref_dialog->config->autosave_label_fullpath,
			 SppHelpPath, "dialog/preference.html#autosave_label_fullpath",
			 NULL);

    /* create check box */
    pref_dialog->autosave_by_drop_button =
	spCreateCheckBox(pref_dialog->autosave_tab, "autosaveByDropButton",
			 SppTitle, SW_AUTOSAVE_DROP_LABEL,
			 SppSet, pref_dialog->config->autosave_by_drop,
			 SppHelpPath, "dialog/preference.html#autosave_by_drop",
			 NULL);
#endif
    
    return;
}

static swPrefDialog createPreferenceDialog(swConfig config)
{
    swPrefDialog pref_dialog;

    pref_dialog = xalloc(1, struct _swPrefDialog);
    memset(pref_dialog, 0, sizeof(struct _swPrefDialog));
    pref_dialog->config = config;

    pref_dialog->window = spCreateDialogBox("preferenceDialog",
					    SppTitle, SW_PREFERENCE_TITLE,
					    SppCallbackFunc, swPopdownPreferenceDialogCB,
					    SppCallbackData, pref_dialog,
					    SppDialogBoxButtonType, SP_DB_OK_CANCEL_APPLY,
					    SppCloseStyle, SP_UNMAP_CLOSE,
					    SppSpacingOn, SP_TRUE,
					    SppHelpButtonVisible, SP_TRUE,
					    SppHelpPath, "dialog/preference.html",
					    NULL);

    /* create tab box */
    pref_dialog->tab_box = spCreateTabBox(pref_dialog->window, "preferenceTabBox", 460,
					  NULL);
    
    /* create file tab */
    createPreferenceFileTab(pref_dialog, pref_dialog->tab_box);
    
    /* create look tab */
    createPreferenceLookTab(pref_dialog, pref_dialog->tab_box);
    
    /* create display tab */
    createPreferenceDisplayTab(pref_dialog, pref_dialog->tab_box);
    
    /* create sound tab */
    createPreferenceSoundTab(pref_dialog, pref_dialog->tab_box);
    
    /* create label tab */
    createPreferenceLabelTab(pref_dialog, pref_dialog->tab_box);
    
    /* create autosave tab */
    createPreferenceAutosaveTab(pref_dialog, pref_dialog->tab_box);
    
    return pref_dialog;
}

swPrefDialog swCreatePreferenceDialog(swConfig config)
{
    static swPrefDialog sw_pref_dialog = NULL;

    if (sw_pref_dialog == NULL) {
	sw_pref_dialog = createPreferenceDialog(config);
    }
    
    return sw_pref_dialog;
}

void swPopupPreferenceDialogCB(spComponent component, swWindow window)
{
    swPrefDialog pref_dialog;
    
    window->config->toplevel->current_window = window;

    pref_dialog = swCreatePreferenceDialog(window->config);

    /* popup dialog */
    spPopupWindow(pref_dialog->window);

    return;
}
#endif /* SW_USE_PREFERENCE_DIALOG */

/*
 *	functions for format dialog
 */
void swPopdownFormatDialogCB(spComponent component, swFormatDialog format_dialog)
{
    const char *name;
    char *string;

    if (spIsCreated(format_dialog->window) == SP_FALSE) return;

    /* popdown format dialog */
    spPopdownWindow(format_dialog->window);
    
    format_dialog->call_reason = spGetCallbackReason(component);

    name = spGetName(format_dialog->window);
    spDebug(80, "swPopdownFormatDialogCB", "name = %s\n", name);
    
    if (streq(name, "formatDialog") && format_dialog->call_reason == SP_CR_OK) {
	if ((string = xspGetTextString(format_dialog->format_field)) != NULL) {
	    spDebug(10, "swPopdownFormatDialogCB", "format string = %s\n", string);
	    spStrCopy(format_dialog->config->format_string, SP_MAX_SETUP_VALUE, string);
	    xfree(string);
	}

	if ((string = xspGetTextString(format_dialog->bit_field)) != NULL) {
	    spDebug(10, "swPopdownFormatDialogCB", "bit string = %s\n", string);
	    format_dialog->config->samp_bit = swGetSampleBit(string);
	    xfree(string);
	}
	
	if ((string = xspGetTextString(format_dialog->samp_rate_field)) != NULL) {
	    spDebug(10, "swPopdownFormatDialogCB", "samp_rate string = %s\n", string);
	    format_dialog->config->samp_rate = atof(string);
	    xfree(string);
	}
	
	if ((string = xspGetTextString(format_dialog->channel_field)) != NULL) {
	    spDebug(10, "swPopdownFormatDialogCB", "channel string = %s\n", string);
	    format_dialog->config->num_channel = atoi(string);
	    xfree(string);
	}
    }
    
    return;
}

static swFormatDialog createFormatDialog(swConfig config, spBool property_flag)
{
    swFormatDialog format_dialog;
    char string[SP_MAX_LINE];
    
    format_dialog = xalloc(1, struct _swFormatDialog);
    memset(format_dialog, 0, sizeof(struct _swFormatDialog));
    format_dialog->call_reason = SP_CR_NONE;
    format_dialog->config = config;

    format_dialog->window = spCreateDialogBox(property_flag ? "propertyDialog" : "formatDialog",
					      SppTitle, property_flag ? SW_PROPERTY_TITLE : NULL,
					      SppCallbackFunc, swPopdownFormatDialogCB,
					      SppCallbackData, format_dialog,
                                              SppDialogBoxButtonType, property_flag ? SP_DB_OK : SP_DB_OK_CANCEL,
					      SppCloseStyle, SP_UNMAP_CLOSE,
					      SppHelpButtonVisible, SP_TRUE,
					      SppHelpPath, property_flag ? "dialog/property.html" : "dialog/format.html",
					      NULL);
    
    if (property_flag == SP_TRUE) {
        format_dialog->file_path_field = spCreateParamField(format_dialog->window, "filePathField", 60,
                                                            SppTitle, SW_FILE_PATH_LABEL,
                                                            SppFieldType, SP_FIELD_TYPE_TEXT,
                                                            SppEditable, SP_FALSE,
                                                            SppFieldOffset, 140,
                                                            SppFieldSize, 250,
                                                            SppHelpPath, "dialog/property.html#file_path",
                                                            NULL);
        format_dialog->plugin_field = spCreateParamField(format_dialog->window, "pluginField", 60,
                                                         SppTitle, SW_PLUGIN_LABEL,
                                                         SppFieldType, SP_FIELD_TYPE_TEXT,
                                                         SppEditable, SP_FALSE,
                                                         SppFieldOffset, 140,
                                                         SppFieldSize, 250,
                                                         SppHelpPath, "dialog/property.html#plugin",
                                                         NULL);
        format_dialog->amp_max_field = spCreateParamField(format_dialog->window, "ampMaxField", 60,
                                                          SppTitle, SW_AMPLITUDE_MAX_LABEL,
                                                          SppFieldType, SP_FIELD_TYPE_TEXT,
                                                          SppEditable, SP_FALSE,
                                                          SppFieldOffset, 140,
                                                          SppFieldSize, 250,
                                                          SppHelpPath, "dialog/property.html#amp_max",
                                                          NULL);
        format_dialog->amp_min_field = spCreateParamField(format_dialog->window, "ampMinField", 60,
                                                          SppTitle, SW_AMPLITUDE_MIN_LABEL,
                                                          SppFieldType, SP_FIELD_TYPE_TEXT,
                                                          SppEditable, SP_FALSE,
                                                          SppFieldOffset, 140,
                                                          SppFieldSize, 250,
                                                          SppHelpPath, "dialog/property.html#amp_min",
                                                          NULL);
    }
    
    /* create combo box to input wave format */
    format_dialog->format_field = spCreateParamField(format_dialog->window, "formatField", 60,
						     SppTitle, SW_FORMAT_LABEL,
						     SppFieldType, property_flag ? SP_FIELD_TYPE_TEXT : SP_FIELD_TYPE_COMBO_BOX,
						     SppEditable, SP_FALSE,
						     SppFieldStrings, property_flag ? NULL : format_list_strings,
						     SppFieldOffset, 140,
						     SppFieldSize, property_flag ? 250 : 160,
						     SppHelpPath, property_flag ? "dialog/property.html#format" : "dialog/format.html#format_format",
						     NULL);
    if (property_flag == SP_FALSE) {
        selectFormatItem(format_dialog->format_field, config->format_string);
        /*spSelectListItem(format_dialog->format_field, config->format_string);*/
    }
    
    /* create combo box to input bits per sample */
    format_dialog->bit_field = spCreateParamField(format_dialog->window, "bitField", 60,
						  SppTitle, SW_BIT_LABEL,
						  SppDimension, "bits/sample",
						  SppFieldType, property_flag ? SP_FIELD_TYPE_TEXT : SP_FIELD_TYPE_COMBO_BOX,
						  SppEditable, SP_FALSE,
						  SppFieldStrings, property_flag ? NULL : bit_list_strings,
						  SppFieldOffset, 140,
						  SppFieldSize, 160,
						  SppHelpPath, property_flag ? "dialog/property.html#bit" : "dialog/format.html#format_bit",
						  NULL);
    if (property_flag == SP_FALSE) {
        swGetSampleBitString(string, config->samp_bit);
        spSelectListItem(format_dialog->bit_field, string);
    }
    
    /* create combo box to input sampling frequency */
    format_dialog->samp_rate_field = spCreateParamField(format_dialog->window, "sampRateField", 60,
							SppTitle, SW_SAMP_RATE_LABEL,
							SppDimension, "Hz",
							SppFieldType, property_flag ? SP_FIELD_TYPE_TEXT : SP_FIELD_TYPE_COMBO_BOX,
							SppEditable, property_flag ? SP_FALSE : SP_TRUE,
							SppFieldStrings, property_flag ? NULL : samp_rate_list_strings,
							SppFieldOffset, 140,
							SppFieldSize, 160,
							SppHelpPath, property_flag ? "dialog/property.html#samp_rate" : "dialog/format.html#format_samp_rate",
							NULL);
    if (property_flag == SP_FALSE) {
        sprintf(string, "%.0f", config->samp_rate);
        spSetTextString(format_dialog->samp_rate_field, string);
    }
    
    /* create combo box to input number of channel */
    format_dialog->channel_field = spCreateParamField(format_dialog->window, "channelField", 60,
						      SppTitle, SW_CHANNEL_LABEL,
						      SppDimension, "channels",
						      SppFieldType, property_flag ? SP_FIELD_TYPE_TEXT : SP_FIELD_TYPE_COMBO_BOX,
						      SppEditable, property_flag ? SP_FALSE : SP_TRUE,
						      SppFieldStrings, property_flag ? NULL : channel_list_strings,
						      SppFieldOffset, 140,
						      SppFieldSize, 160,
						      SppHelpPath, property_flag ? "dialog/property.html#channel" : "dialog/format.html#format_channel",
						      NULL);
    if (property_flag == SP_FALSE) {
        sprintf(string, "%d", config->num_channel);
        spSetTextString(format_dialog->channel_field, string);
    }
    
    return format_dialog;
}

static swFormatDialog sw_format_dialog = NULL;

swFormatDialog swCreateFormatDialog(swConfig config)
{
    if (sw_format_dialog == NULL) {
	sw_format_dialog = createFormatDialog(config, SP_FALSE);
    }
    sw_format_dialog->call_reason = SP_CR_NONE;
    
    return sw_format_dialog;
}

spComponent swGetFormatDialogWindow(void)
{
    if (sw_format_dialog == NULL) {
	return NULL;
    }

    return sw_format_dialog->window;
}

spBool swPopupFormatDialog(swConfig config, const char *filename)
{
    swFormatDialog format_dialog;
    char title[SP_MAX_MESSAGE];
    
    format_dialog = swCreateFormatDialog(config);

    /* set window title */
    if (!strnone(filename)) {
	spStrCopy(title, SP_MAX_MESSAGE, SW_FORMAT_TITLE);
	spStrCat(title, SP_MAX_MESSAGE, ": ");
	spStrCat(title, SP_MAX_MESSAGE, filename);
	
	spSetParams(format_dialog->window, SppTitle, title, NULL);
    } else {
	spSetParams(format_dialog->window, SppTitle, SW_FORMAT_TITLE, NULL);
    }
    
    /* popup dialog */
    spPopupWindow(format_dialog->window);

    if (format_dialog->call_reason == SP_CR_OK) {
	return SP_TRUE;
    } else {
	return SP_FALSE;
    }
}

#ifdef SW_SUPPORT_PROPERTY_DIALOG
static swFormatDialog sw_property_dialog = NULL;

swFormatDialog swCreatePropertyDialog(swConfig config)
{
    if (sw_property_dialog == NULL) {
	sw_property_dialog = createFormatDialog(config, SP_TRUE);
    }
    sw_property_dialog->call_reason = SP_CR_NONE;
    
    return sw_property_dialog;
}

spComponent swGetPropertyDialogWindow(void)
{
    if (sw_property_dialog == NULL) {
	return NULL;
    }

    return sw_property_dialog->window;
}

spBool swPopupPropertyDialog(swConfig config, swWindow window, swWave wave)
{
    swFormatDialog property_dialog;
    char *path;
    char buf[SP_MAX_MESSAGE];
    
    property_dialog = swCreatePropertyDialog(config);

    if ((path = xspGetReadablePath(wave->core->orig_filename)) != NULL) {
        spStrCopy(buf, sizeof(buf), path);
        xfree(path);
    } else {
        buf[0] = NUL;
    }
    spSetTextString(property_dialog->file_path_field, buf);

    spSetTextString(property_dialog->plugin_field, wave->plugin_name != NULL ? wave->plugin_name : "");

    {
        int i;
        char *unit_str;
        double limit;
        double value;
        char amplitude_buf[SP_MAX_MESSAGE];
        char percent_amplitude_buf[SP_MAX_MESSAGE];
        char amplitudedB_buf[SP_MAX_MESSAGE];
        spBool accept_dB;
        
        limit = swGetWindowLimitValue(window, wave);
        unit_str = swGetAnalysisUnitString(wave, SP_FALSE);

        if (wave->num_order <= 1 && window->data_type != SW_FREQ_DATA && strnone(unit_str)) {
            accept_dB = SP_TRUE;
        } else {
            accept_dB = SP_FALSE;
        }
        
        for (i = 0; i <= 1; i++) {
            if (i == 0) {
                value = swGetWaveMax(wave);
            } else {
                value = swGetWaveMin(wave);
            }

            swGetAmplitudeString(wave, limit, value, amplitude_buf, SP_FALSE, SP_FALSE);
            
            if (accept_dB) {
                swGetAmplitudeString(wave, limit, value, percent_amplitude_buf, SP_TRUE, SP_FALSE);
                swGetAmplitudedBString(wave, limit, value, amplitudedB_buf, SP_FALSE);
                sprintf(buf, "%s  (%s %%,  %s dB)", amplitude_buf, percent_amplitude_buf, amplitudedB_buf);
            } else {
                sprintf(buf, "%s %s", amplitude_buf, unit_str);
            }

            if (i == 0) {
                spSetTextString(property_dialog->amp_max_field, buf);
            } else {
                spSetTextString(property_dialog->amp_min_field, buf);
            }
        }
    }
    
    spStrCopy(buf, sizeof(buf), swGetWaveFileType(wave));
    spSetTextString(property_dialog->format_field, buf);
    
    swGetSampleBitString(buf, swGetWaveSampleBit(wave));
    spSetTextString(property_dialog->bit_field, buf);

    sprintf(buf, "%.1f", swGetWaveSampleRate(wave));
    spSetTextString(property_dialog->samp_rate_field, buf);

    sprintf(buf, "%d", wave->num_channel);
    spSetTextString(property_dialog->channel_field, buf);
    
    /* popup dialog */
    spPopupWindow(property_dialog->window);

    if (property_dialog->call_reason == SP_CR_OK) {
	return SP_TRUE;
    } else {
	return SP_FALSE;
    }
}

void swPopupPropertyDialogCB(spComponent component, swWindow window)
{
    swPopupPropertyDialog(window->config, window, window->wave);
    return;
}
#endif

/*
 *	functions for bit conversion
 */
typedef struct _swBitConvDialog {
    spComponent window;
    spComponent bit_combo;
    spComponent float_normalized_button;
    int samp_bit;
    spBool *float_normalized;
} *swBitConvDialog;

static void popdownBitConvDialogCB(spComponent component, swBitConvDialog dialog)
{
    char *string;

    spPopdownWindow(component);
    
    if (dialog == NULL) return;
    
    if (spGetCallbackReason(component) == SP_CR_OK) {
	if ((string = xspGetTextString(dialog->bit_combo)) != NULL) {
	    spDebug(10, "popdownBitConvDialogCB", "bit string = %s\n", string);
	    dialog->samp_bit = swGetSampleBit(string);
	    xfree(string);
	}
	spGetToggleState(dialog->float_normalized_button, dialog->float_normalized);
    }

    return;
}

void swPopupBitConvDialogCB(spComponent component, swWindow window)
{
    char string[SP_MAX_LINE];
    static swBitConvDialog dialog = NULL;

    if (window == NULL || window->wave == NULL) return;

    if (dialog == NULL) {
	dialog = xalloc(1, struct _swBitConvDialog);
	memset(dialog, 0, sizeof(struct _swBitConvDialog));
	
	dialog->window = spCreateDialogBox("bitConvDialog",
					   SppTitle, SW_BIT_CONV_DIALOG_TITLE,
					   SppCallbackFunc, popdownBitConvDialogCB,
					   SppCallbackData, dialog,
					   SppDialogBoxButtonType, SP_DB_OK_CANCEL,
					   SppCloseStyle, SP_UNMAP_CLOSE,
					   NULL);
	
	/* create combo box to input bits per sample */
	dialog->bit_combo = spCreateParamField(dialog->window, "bitComboBox", 0,
					       SppTitle, SW_BIT_LABEL,
					       SppDimension, "bits/sample",
					       SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
					       SppEditable, SP_FALSE,
					       SppFieldStrings, bit_list_strings,
					       SppFieldOffset, 120,
					       SppFieldSize, 120,
					       NULL);
	
	dialog->float_normalized_button = spCreateCheckBox(dialog->window, "floatNormalizedButton",
							   SppTitle, SW_FLOAT_NORMALIZED_LABEL,
							   NULL);
    }
    swGetSampleBitString(string, window->wave->samp_bit);
    spSelectListItem(dialog->bit_combo, string);
    dialog->samp_bit = window->wave->samp_bit;

    spSetToggleState(dialog->float_normalized_button, window->config->wave_config->float_normalized);
    dialog->float_normalized = &window->config->wave_config->float_normalized;
    
    /* popup dialog */
    spPopupWindow(dialog->window);

    if (dialog->samp_bit != window->wave->samp_bit) {
	swConvertBit(window, dialog->samp_bit);
    }

    return;
}

#define SW_MIN_AMPLIFY_RATE 0
#define SW_MAX_AMPLIFY_RATE 300

void swPopupAmplifyDialogCB(spComponent component, swWindow window)
{
    double rate;
    static swRateChangeDialog dialog = NULL;

    if (window == NULL || window->wave == NULL) return;

    spDebug(60, "swPopupAmplifyDialogCB", "in\n");

    if (dialog == NULL) {
	static char *amplify_rate_strings[] =
	{
	    "50",
	    "80",
	    "90",
	    "100",
	    "110",
	    "120",
	    "150",
	    "200",
	    NULL,
	};
	static char *amplify_dB_strings[] =
	{
	    "-6.0",
	    "-3.0",
	    "-2.0",
	    "-1.0",
	    "0.0",
	    "1.0",
	    "2.0",
	    "3.0",
	    "6.0",
	    NULL,
	};
	
	dialog = swCreateRateChangeDialog(SW_AMPLIFY_DIALOG_TITLE,
					  SW_AMPLIFY_RATE_LABEL, amplify_rate_strings,
					  SW_AMPLIFY_DB_LABEL, amplify_dB_strings,
					  SW_MIN_AMPLIFY_RATE, SW_MAX_AMPLIFY_RATE);
    }

    rate = 100.0;
    
    /* popup dialog */
    if (swPopupRateChangeDialog(dialog, &rate) == SP_TRUE) {
	swEditWindow(window, SW_EDIT_AMPLIFY, window->sel_st, window->sel_ed, rate / 100.0);
    }

    return;
}

void swPopupMaximizeDialogCB(spComponent component, swWindow window)
{
    double min, max;
    double absmax;
    double rate;
    static swRateChangeDialog dialog = NULL;

    if (window == NULL || window->wave == NULL) return;

    spDebug(60, "swPopupMaximizeDialogCB", "in\n");

    min = swGetWaveMin(window->wave);
    max = swGetWaveMax(window->wave);
    absmax = MAX(FABS(min), FABS(max));
    spDebug(60, "swPopupMaximizeDialogCB", "min = %f, max = %f, absmax = %f\n",
	    min, max, absmax);

    if (absmax <= 0.0) {
	/* maximum value is too small. */
	spDisplayError(window->window, NULL, SW_MAXIMUM_VALUE_TOO_SMALL_ERROR_MESSAGE);
	return;
    }
    
    if (swIsWaveFloat(window->wave) == SP_TRUE
	&& window->config->wave_config->float_normalized == SP_FALSE) {
	if (spCreateMessageBox(window->window, NULL,
			       SW_ASSUME_FLOAT_NORMALIZED_WARNING_MESSAGE,
			       SppDialogType, SP_WARNING_DIALOG,
			       SppMessageBoxButtonType, SP_MB_YES_NO,
			       NULL) != SP_DR_YES) {
	    return;
	}
	window->config->wave_config->float_normalized = SP_TRUE;
    }

    if (dialog == NULL) {
	static char *maximize_rate_strings[] =
	{
	    "50",
	    "60",
	    "70",
	    "80",
	    "90",
	    "100",
	    NULL,
	};
	static char *maximize_dB_strings[] =
	{
	    "-6.0",
	    "-3.0",
	    "-2.0",
	    "-1.0",
	    "-0.5",
	    "0.0",
	    NULL,
	};
	
	dialog = swCreateRateChangeDialog(SW_MAXIMIZE_DIALOG_TITLE,
					  SW_MAXIMIZE_RATE_LABEL, maximize_rate_strings,
					  SW_MAXIMIZE_DB_LABEL, maximize_dB_strings,
					  0, 100);
    }

    rate = 100.0 * absmax / swGetClipValue(window->wave->samp_bit);
    spDebug(60, "swPopupMaximizeDialogCB", "initial rate = %f\n", rate);
	
    /* popup dialog */
    if (swPopupRateChangeDialog(dialog, &rate) == SP_TRUE) {
	spDebug(60, "swPopupMaximizeDialogCB", "rate = %f\n", rate);
	swEditWindow(window, SW_EDIT_AMPLIFY, 0, window->wave->total_length,
		     swGetClipValue(window->wave->samp_bit) * rate / 100.0 / absmax);
    }

   return;
}

/*
 *	functions for dialog for sampling frequency conversion
 */
typedef struct _swSampFreqConvDialog {
    spComponent window;
    spComponent samp_rate_field;
    spComponent cutoff_field;
    spComponent transition_field;
    spComponent gain_field;
    spComponent sidelobe_field;
    spComponent tolerance_field;
    spComponent buffer_length_field;
    
    double samp_rate;
    swConfig config;
} *swSampFreqConvDialog;

static void popdownSampFreqConvDialogCB(spComponent component, swSampFreqConvDialog dialog)
{
    char *string;
    double orig_samp_rate;
    double value;

    spPopdownWindow(component);
    
    if (dialog == NULL) return;
    
    if (spGetCallbackReason(component) == SP_CR_OK) {
	orig_samp_rate = dialog->samp_rate;
	
	if ((string = xspGetTextString(dialog->samp_rate_field)) != NULL) {
	    spDebug(10, "popdownSampFreqConvDialogCB", "string = %s\n", string);
	    if ((value = atof(string)) <= 0.0) {
		spDisplayError(component, NULL, SW_PARAMETER_ERROR_MESSAGE, SW_SAMP_RATE_LABEL);
	    } else {
		dialog->samp_rate = value;
	    }
	    xfree(string);
	}
	
	if ((string = xspGetTextString(dialog->cutoff_field)) != NULL) {
	    if ((value = atof(string)) <= 0.0) {
		dialog->samp_rate = orig_samp_rate;
		spDisplayError(component, NULL, SW_PARAMETER_ERROR_MESSAGE, SW_SFC_CUTOFF_LABEL);
	    } else {
		dialog->config->wave_config->sfc_cutoff = value;
	    }
	    xfree(string);
	}
	if ((string = xspGetTextString(dialog->transition_field)) != NULL) {
	    if ((value = atof(string)) <= 0.0) {
		dialog->samp_rate = orig_samp_rate;
		spDisplayError(component, NULL, SW_PARAMETER_ERROR_MESSAGE, SW_SFC_TRANSITION_LABEL);
	    } else {
		dialog->config->wave_config->sfc_transition = value;
	    }
	    xfree(string);
	}
	if ((string = xspGetTextString(dialog->sidelobe_field)) != NULL) {
	    if ((value = atof(string)) <= 0.0) {
		dialog->samp_rate = orig_samp_rate;
		spDisplayError(component, NULL, SW_PARAMETER_ERROR_MESSAGE, SW_SFC_SIDELOBE_LABEL);
	    } else {
		dialog->config->wave_config->sfc_sidelobe = value;
	    }
	    xfree(string);
	}
	if ((string = xspGetTextString(dialog->gain_field)) != NULL) {
	    if ((value = atof(string)) <= 0.0) {
		dialog->samp_rate = orig_samp_rate;
		spDisplayError(component, NULL, SW_PARAMETER_ERROR_MESSAGE, SW_SFC_GAIN_LABEL);
	    } else {
		dialog->config->wave_config->sfc_gain = value;
	    }
	    xfree(string);
	}
	if ((string = xspGetTextString(dialog->tolerance_field)) != NULL) {
	    if ((value = atof(string)) < 0.0) {
		dialog->samp_rate = orig_samp_rate;
		spDisplayError(component, NULL, SW_PARAMETER_ERROR_MESSAGE, SW_SFC_TOLERANCE_LABEL);
	    } else {
		dialog->config->wave_config->sfc_tolerance = value;
	    }
	    xfree(string);
	}
	if ((string = xspGetTextString(dialog->buffer_length_field)) != NULL) {
	    long buffer_length;
	    
	    if ((buffer_length = atol(string)) <= 0) {
		dialog->samp_rate = orig_samp_rate;
		spDisplayError(component, NULL, SW_PARAMETER_ERROR_MESSAGE, SW_SFC_BUFFER_LENGTH_LABEL);
	    } else {
		dialog->config->wave_config->sfc_buffer_length = buffer_length;
	    }
	    xfree(string);
	}
    }

    return;
}

void swPopupSampFreqConvDialogCB(spComponent component, swWindow window)
{
    char string[SP_MAX_LINE];
    static swSampFreqConvDialog dialog = NULL;
    static char *cutoff_list_strings[] =
    {
	"0.90",
	"0.92",
	"0.93",
	"0.94",
	"0.95",
	"0.96",
	"0.97",
	"0.98",
	NULL,
    };
    static char *transition_list_strings[] =
    {
	"0.02",
	"0.03",
	"0.04",
	"0.05",
	"0.06",
	"0.07",
	"0.08",
	"0.09",
	"0.10",
	NULL,
    };
    static char *sidelobe_list_strings[] =
    {
	"40.0",
	"50.0",
	"60.0",
	"70.0",
	"80.0",
	"90.0",
	"100.0",
	"120.0",
	NULL,
    };
    static char *tolerance_list_strings[] =
    {
	"0.2",
	"0.5",
	"1.0",
	"1.5",
	"2.0",
	"2.5",
	"3.0",
	NULL,
    };
    static char *buffer_length_list_strings[] =
    {
	"8192",
	"16384",
	"32768",
	"65536",
	"131072",
	"262144",
	NULL,
    };
    static char *gain_list_strings[] =
    {
	"0.5",
	"0.8",
	"0.9",
	"1.0",
	"1.1",
	"1.2",
	"1.5",
	"2.0",
	NULL,
    };

    if (window == NULL || window->wave == NULL) return;

    if (dialog == NULL) {
	dialog = xalloc(1, struct _swSampFreqConvDialog);
	memset(dialog, 0, sizeof(struct _swSampFreqConvDialog));
	dialog->config = window->config;
	
	dialog->window = spCreateDialogBox("sampFreqConvDialog",
					   SppTitle, SW_SAMP_RATE_CONV_DIALOG_TITLE,
					   SppCallbackFunc, popdownSampFreqConvDialogCB,
					   SppCallbackData, dialog,
					   SppDialogBoxButtonType, SP_DB_OK_CANCEL,
					   SppCloseStyle, SP_UNMAP_CLOSE,
					   SppHelpButtonVisible, SP_TRUE,
					   SppHelpPath, "dialog/freq_conv.html",
					   NULL);
	
	/* create combo box to input sampling frequency */
	dialog->samp_rate_field = spCreateParamField(dialog->window, "sampFreqField", 60,
						     SppTitle, SW_SAMP_RATE_LABEL,
						     SppDimension, "Hz",
						     SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
						     SppEditable, SP_TRUE,
						     SppFieldStrings, samp_rate_list_strings,
						     SppFieldOffset, 140,
						     SppFieldSize, 160,
						     SppHelpPath, "dialog/freq_conv.html#sfc_samp_freq",
						     NULL);
	
	sprintf(string, "%.2f", dialog->config->wave_config->sfc_cutoff);
	dialog->cutoff_field = spCreateParamField(dialog->window, "cutoffField", 60,
						  SppTitle, SW_SFC_CUTOFF_LABEL,
						  SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
						  SppEditable, SP_TRUE,
						  SppFieldString, string,
						  SppFieldStrings, cutoff_list_strings,
						  SppFieldOffset, 140,
						  SppFieldSize, 160,
						  SppHelpPath, "dialog/freq_conv.html#sfc_cutoff",
						  NULL);
	
	sprintf(string, "%.2f", dialog->config->wave_config->sfc_transition);
	dialog->transition_field = spCreateParamField(dialog->window, "transitionField", 60,
						      SppTitle, SW_SFC_TRANSITION_LABEL,
						      SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
						      SppEditable, SP_TRUE,
						      SppFieldString, string,
						      SppFieldStrings, transition_list_strings,
						      SppFieldOffset, 140,
						      SppFieldSize, 160,
						      SppHelpPath, "dialog/freq_conv.html#sfc_transition",
						      NULL);
	
	sprintf(string, "%.1f", dialog->config->wave_config->sfc_sidelobe);
	dialog->sidelobe_field = spCreateParamField(dialog->window, "sidelobeField", 60,
						    SppTitle, SW_SFC_SIDELOBE_LABEL,
						    SppDimension, "dB",
						    SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
						    SppEditable, SP_TRUE,
						    SppFieldString, string,
						    SppFieldStrings, sidelobe_list_strings,
						    SppFieldOffset, 140,
						    SppFieldSize, 160,
						    SppHelpPath, "dialog/freq_conv.html#sfc_sidelobe",
						    NULL);
	
	sprintf(string, "%.1f", dialog->config->wave_config->sfc_gain);
	dialog->gain_field = spCreateParamField(dialog->window, "gainField", 60,
						SppTitle, SW_SFC_GAIN_LABEL,
						SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
						SppEditable, SP_TRUE,
						SppFieldString, string,
						SppFieldStrings, gain_list_strings,
						SppFieldOffset, 140,
						SppFieldSize, 160,
						SppHelpPath, "dialog/freq_conv.html#sfc_gain",
						NULL);
	
	sprintf(string, "%.1f", dialog->config->wave_config->sfc_tolerance);
	dialog->tolerance_field = spCreateParamField(dialog->window, "toleranceField", 60,
						     SppTitle, SW_SFC_TOLERANCE_LABEL,
						     SppDimension, "%",
						     SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
						     SppEditable, SP_TRUE,
						     SppFieldString, string,
						     SppFieldStrings, tolerance_list_strings,
						     SppFieldOffset, 140,
						     SppFieldSize, 160,
						     SppHelpPath, "dialog/freq_conv.html#sfc_tolerance",
						     NULL);

	sprintf(string, "%ld", dialog->config->wave_config->sfc_buffer_length);
	dialog->buffer_length_field = spCreateParamField(dialog->window, "bufferLengthField", 60,
							 SppTitle, SW_SFC_BUFFER_LENGTH_LABEL,
							 SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
							 SppEditable, SP_TRUE,
							 SppFieldString, string,
							 SppFieldStrings, buffer_length_list_strings,
							 SppFieldOffset, 140,
							 SppFieldSize, 160,
							 SppHelpPath, "dialog/freq_conv.html#sfc_buffer_length",
							 NULL);
    }
    sprintf(string, "%.0f", window->wave->samp_rate);
    spSetTextString(dialog->samp_rate_field, string);
    
    dialog->samp_rate = window->wave->samp_rate;
    
    /* popup dialog */
    spPopupWindow(dialog->window);

    if (dialog->samp_rate != window->wave->samp_rate) {
	swConvertSampFreq(window, dialog->samp_rate);
    }

    return;
}

void swPopupValueChangeDialogCB(spComponent component, swWindow window)
{
    int channel;
    spLong point;
    spLong data_offset;
    double new_value;
    double value = 0.0;
    char *string;
    char buf[SP_MAX_MESSAGE];
    static swStringChangeDialog dialog = NULL;

    if (swIsNoWave(window) == SP_TRUE) return;

    if (dialog == NULL) {
	dialog = swCreateStringChangeDialog(SW_CHANGE_VALUE_DIALOG_TITLE, SW_VALUE_LABEL, NULL,
					    NULL, NULL, 80, 120, SP_TRUE);
    }

    channel = window->target_channel;
    point = window->point;
    data_offset = (window->point - window->offset) / swGetWaveThinLength(window->wave);
    swGetWaveData(window->wave, window->target_channel, window->target_order, data_offset, &value);
    if (swIsWaveFloat(window->wave) == SP_TRUE) {
	sprintf(buf, "%f", value);
    } else {
	sprintf(buf, "%ld", (long)value);
    }

    if ((string = xswPopupStringChangeDialog(dialog, NULL, buf)) != NULL) {
	new_value = atof(string);
	if (new_value != value) {
	    swChangeWindowValue(window, channel, point, new_value);
	}
	xfree(string);
    }
    
    return;
}

void swPopupInsertPauseDialogCB(spComponent component, swWindow window)
{
    char *string;
    char *dim_string;
    static swStringChangeDialog dialog = NULL;

    if (swIsNoWave(window) == SP_TRUE) return;

    if (dialog == NULL) {
	dialog = swCreateStringChangeDialog(SW_INSERT_PAUSE_DIALOG_TITLE,
					    SW_PAUSE_LABEL, "points",
					    NULL, NULL, 100, 140, SP_TRUE);
    }

    if (window->config->time_format == SW_TIME_FORMAT_MSEC
	|| window->config->time_format == SW_TIME_FORMAT_FLOORED_MSEC) {
	dim_string = "msec";
    } else {
	if (window->config->time_format == SW_TIME_FORMAT_POINT) {
	    dim_string = "points";
	} else {
	    dim_string = "sec";
	}
    }

    if ((string = xswPopupStringChangeDialog(dialog, dim_string, "0")) != NULL) {
	double sec;
	spLong pause_length;

	spDebug(50, "swInsertPauseDialogCB", "string = %s\n", string);
	
	if (swConvertTimeString(string, window->config->time_format, &sec) == SP_TRUE) {
	    pause_length = swDimToSamp(window, sec);
	    spDebug(50, "swInsertPauseDialogCB", "sec = %f, pause_length = %ld\n",
		    sec, pause_length);
	    
	    if (pause_length > 0) {
		swEditWindow(window, SW_EDIT_INSERT_PAUSE, window->point, pause_length, 0.0);
	    }
	}

	xfree(string);
    }
    
    return;
}

/*
 *	functions for dialog to change string
 */
struct _swStringChangeDialog {
    spComponent window;
    spComponent field;
    spCallbackReason reason;
};

static void popdownStringChangeCB(spComponent component, swStringChangeDialog dialog)
{
    if (dialog != NULL) {
	dialog->reason = spGetCallbackReason(component);
    }

    spPopdownWindow(component);
    
    return;
}

swStringChangeDialog swCreateStringChangeDialog(const char *title, const char *label, const char *dimension,
						const char **list, const char *default_string, 
						int offset, int size, spBool editable)
{
    swStringChangeDialog dialog;

    dialog = xalloc(1, struct _swStringChangeDialog);
    memset(dialog, 0, sizeof(struct _swStringChangeDialog));
    dialog->reason = SP_CR_NONE;
    
    dialog->window = spCreateDialogBox("stringChangeDialog",
				       SppTitle, title,
				       SppCallbackFunc, popdownStringChangeCB,
				       SppCallbackData, dialog,
				       SppDialogBoxButtonType, SP_DB_OK_CANCEL,
				       SppCloseStyle, SP_UNMAP_CLOSE,
				       NULL);

    if (list != NULL) {
	/* create combo box */
	dialog->field = spCreateParamField(dialog->window, "stringChangeCombo", 60,
					   SppTitle, label,
					   SppDimension, dimension,
					   SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
					   SppEditable, editable,
					   SppFieldStrings, list,
					   SppFieldOffset, offset,
					   SppFieldSize, size,
					   NULL);

	if (editable == SP_TRUE) {
	    spSetTextString(dialog->field, default_string);
	} else {
	    spSelectListItem(dialog->field, default_string);
	}
    } else {
	/* create text field */
	dialog->field = spCreateParamField(dialog->window, "stringChangeText", 60,
					   SppTitle, label,
					   SppDimension, dimension,
					   SppFieldType, SP_FIELD_TYPE_TEXT,
					   SppEditable, editable,
					   SppTextString, default_string,
					   SppFieldOffset, offset,
					   SppFieldSize, size,
					   NULL);
    }

    return dialog;
}

char *xswPopupStringChangeDialog(swStringChangeDialog dialog, const char *dimension, const char *string)
{
    dialog->reason = SP_CR_NONE;

    if (!strnone(dimension)) {
	spSetParams(dialog->field,
		    SppDimension, dimension,
		    NULL);
    }
    if (!strnone(string)) {
	spSetTextString(dialog->field, string);
    }

    spPopupWindow(dialog->window);

    if (dialog->reason == SP_CR_OK) {
	return xspGetTextString(dialog->field);
    } else {
	return NULL;
    }
}

/*
 *	functions for dialog to change rate in dB
 */
struct _swRateChangeDialog {
    spComponent window;

    spComponent track_bar;
    spComponent rate_combo;
    spComponent db_combo;

    int min_rate;
    int max_rate;
    
    double rate;
    spCallbackReason reason;
};

static void popdownRateChangeDialogCB(spComponent component, swRateChangeDialog dialog)
{
    char *string;

    spPopdownWindow(component);
    
    if (dialog == NULL) return;

    dialog->reason = spGetCallbackReason(component);
    if (dialog->reason == SP_CR_OK) {
	if ((string = xspGetTextString(dialog->rate_combo)) != NULL) {
	    spDebug(10, "popdownRateChangeDialogCB", "rate string = %s\n", string);
	    dialog->rate = atof(string);
	    xfree(string);
	}
    }

    return;
}

static void updateRateChangeDialog(spComponent component, swRateChangeDialog dialog, double rate)
{
    char buf[SP_MAX_LINE];
    static spBool flag = SP_FALSE; /* flag to prevent endless loop */

    if (flag == SP_TRUE
	|| dialog == NULL || dialog->track_bar == NULL
	|| dialog->rate_combo == NULL || dialog->db_combo == NULL) return;

    flag = SP_TRUE;
    
    rate = MAX(rate, dialog->min_rate);
    rate = MIN(rate, dialog->max_rate);
    
    spDebug(10, "updateRateChangeDialog", "rate = %f\n", rate);

    if (component != dialog->db_combo) {
	percentageTodBString(buf, rate);
	spSetTextString(dialog->db_combo, buf);
    }
    
    if (component != dialog->rate_combo) {
	sprintf(buf, "%.1f", rate);
	spSetTextString(dialog->rate_combo, buf);
    }

    if (component != dialog->track_bar) {
	spSetSliderValue(dialog->track_bar, (int)rate);
    }

    flag = SP_FALSE;
    
    return;
}

static void rateChangeTrackBarCB(spComponent component, swRateChangeDialog dialog)
{
    int value;

    if (spGetSliderValue(component, &value) == SP_TRUE) {
	spDebug(10, "rateChangeTrackBarCB", "value = %d\n", value);
	updateRateChangeDialog(dialog->track_bar, dialog, (double)value);
    }
    
    return;
}

static void rateChangeEditRateCB(spComponent component, swRateChangeDialog dialog)
{
    double rate;
    char *string;
    
    if ((string = xspGetTextString(component)) != NULL) {
	rate = atof(string);
	updateRateChangeDialog(dialog->rate_combo, dialog, rate);
	xfree(string);
    }
    
    return;
}

static void rateChangeEditdBCB(spComponent component, swRateChangeDialog dialog)
{
    double rate;
    char *string;
    
    if ((string = xspGetTextString(component)) != NULL) {
	rate = dBStringToPercentage(string, dialog->min_rate, dialog->max_rate);
	updateRateChangeDialog(dialog->db_combo, dialog, rate);
	xfree(string);
    }
    
    return;
}

swRateChangeDialog swCreateRateChangeDialog(const char *title,
					    const char *rate_label, char **rate_strings,
					    const char *db_label, char **dB_strings,
					    int min_rate, int max_rate)
{
    swRateChangeDialog dialog = NULL;

    dialog = xalloc(1, struct _swRateChangeDialog);
    memset(dialog, 0, sizeof(struct _swRateChangeDialog));
    dialog->min_rate = min_rate;
    dialog->max_rate = max_rate;
    dialog->rate = 100.0;
    dialog->reason = SP_CR_NONE;
    
    dialog->window = spCreateDialogBox("rateChangeDialog",
				       SppTitle, title,
				       SppCallbackFunc, popdownRateChangeDialogCB,
				       SppCallbackData, dialog,
				       SppDialogBoxButtonType, SP_DB_OK_CANCEL,
				       SppCloseStyle, SP_UNMAP_CLOSE,
				       NULL);
    
    dialog->track_bar = spCreateTrackBar(dialog->window, "rateChangeTrackBar",
					 SppCallbackFunc, rateChangeTrackBarCB,
					 SppCallbackData, dialog,
					 SppTrackCallbackOn, SP_TRUE,
					 SppShowScale, SP_TRUE,
					 SppShowValue, SP_TRUE,
					 SppValue, (int)dialog->rate,
					 SppMinimum, min_rate,
					 SppMaximum, max_rate,
					 NULL);
    
    /* create combo box to input rate */
    dialog->rate_combo = spCreateParamField(dialog->window, "rateChangeRateCombo", 60,
					    SppTitle, rate_label,
					    SppDimension, "%",
					    SppCallbackFunc, rateChangeEditRateCB,
					    SppCallbackData, dialog,
					    SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
					    SppEditable, SP_TRUE,
					    SppFieldStrings, rate_strings,
					    SppFieldOffset, 120,
					    SppFieldSize, 120,
					    NULL);
    
    /* create combo box to input factor in dB */
    dialog->db_combo = spCreateParamField(dialog->window, "rateChangedBCombo", 60,
					  SppTitle, db_label,
					  SppDimension, "dB",
					  SppCallbackFunc, rateChangeEditdBCB,
					  SppCallbackData, dialog,
					  SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
					  SppEditable, SP_TRUE,
					  SppFieldStrings, dB_strings,
					  SppFieldOffset, 120,
					  SppFieldSize, 120,
					  NULL);

   return dialog;
}

spBool swPopupRateChangeDialog(swRateChangeDialog dialog, double *rate)
{
    dialog->reason = SP_CR_NONE;
    dialog->rate = *rate;

    updateRateChangeDialog(NULL, dialog, dialog->rate);
    
    spPopupWindow(dialog->window);

    if (dialog->reason == SP_CR_OK) {
	*rate = dialog->rate;
	return SP_TRUE;
    } else {
	return SP_FALSE;
    }
}

#if 1
static char *waveform_list_strings[] =
{
    "Recording",
    "Silence",
    "Sine",
    "Square",
    "Triangle",
    "Sawtooth",
    "White Noise",
    NULL,
};

static char *waveform_f0_list_strings[] =
{
    "50",
    "100",
    "200",
    "500",
    "1000",
    "2000",
    "3000",
    NULL,
};

static char *waveform_phase_list_strings[] =
{
    "0",
    "30",
    "60",
    "90",
    "120",
    "150",
    "180",
    "210",
    "240",
    "270",
    "300",
    "330",
    NULL,
};

static char *waveform_gain_dB_list_strings[] =
{
    "0.0",
    "-1.0",
    "-3.0",
    "-6.0",
    "-9.0",
    "-12.0",
    "-15.0",
    "-18.0",
    "-24.0",
    NULL,
};

static char *waveform_gain_list_strings[] =
{
    "100",
    "90",
    "80",
    "70",
    "60",
    "50",
    "40",
    "30",
    "20",
    "10",
    "5",
    NULL,
};

static char *waveform_delay_list_strings[] =
{
    "0.0",
    "0.1",
    "0.2",
    "0.3",
    "0.4",
    "0.5",
    "0.8",
    "1.0",
    NULL,
};

static char *waveform_duration_list_strings[] =
{
    "0.5",
    "0.7",
    "1.0",
    "1.1",
    "1.2",
    "1.5",
    "2.0",
    "2.5",
    "3.0",
    "5.0",
    "7.0",
    "10.0",
    NULL,
};

static char *waveform_length_list_strings[] =
{
    "0.0",
    "1.0",
    "1.5",
    "2.0",
    "2.5",
    "3.0",
    "5.0",
    "7.0",
    "10.0",
    NULL,
};

static char *waveform_channel_list_strings[] =
{
    "1",
    "2",
    NULL,
};

typedef struct _swWaveformGenerateDialog {
    spComponent window;

    spComponent tab_box;
    
    spComponent waveform_tab;
    
    spComponent waveform_combo;
    spComponent f0_combo;
    spComponent phase_combo;
    spComponent gain_dB_combo;
    spComponent gain_combo;
    spComponent delay_combo;
    spComponent duration_combo;


    spComponent file_tab;
    
    spComponent length_combo;
    
    spComponent bit_combo;
    spComponent samp_rate_combo;
    spComponent channel_combo;
    
    spComponent filename_field;
    int num_plugin;
    char **plugin_names;
    char **file_types;
    char **file_filters;
    int file_type_index;

    swEditType edit_type;
    
    double f0;
    double phase;
    double gain;
    double delay;
    double duration;

    double length_s;
    
    int samp_bit;
    double samp_rate;
    int num_channel;

    char *filename;
    char *plugin_name;
    char *file_type;
    
    spCallbackReason reason;
    swConfig config;
} *swWaveformGenerateDialog;

void swCreateWindowForWaveformGenerate(swConfig config,
				       char *filename, char *plugin_name, char *file_type,
				       int samp_bit, int num_channel, double samp_rate,
				       double length_s,
				       swEditType edit_type, double delay, double duration,
				       double f0, double phase, double gain)
{
    long length;
    long delay_l, duration_l;
    double phase_radian;
    swWave wave;
    swWindow window;
    
    spDebug(10, "swCreateWindowForWaveformGenerate", "in\n");
    
    /* initialize wave */
    wave = swInitWave(config->wave_config, NULL, NULL, NULL, NULL,
		      samp_bit, num_channel,
		      samp_rate, 1,
		      SP_FALSE, config->draw_detail);
    if (wave == NULL) {
	spDisplayError(NULL, SW_ERROR_TITLE, SW_OPEN_ERROR_MESSAGE, filename);
    } else {
	phase_radian = 2.0 * PI * (phase / 360.0);
	length = (long)((length_s * samp_rate) + 0.5);
	delay_l = (long)((delay * samp_rate) + 0.5);
	delay_l = MAX(delay_l, 0);
	duration_l = (long)((duration * samp_rate) + 0.5);
	if (length <= delay_l + duration_l) {
	    length = delay_l + duration_l;
	}
	swSetWaveTotalLength(wave, length);
	spDebug(10, "swCreateWindowForWaveformGenerate",
		"length = %ld, delay_l = %ld, duration_l = %ld\n", length, delay_l, duration_l);
	
	/* create new window */
	window = swCreateWaveWindow(wave, config, SW_TIME_DATA, -1.0, -1.0);
	spDebug(10, "swCreateWindowForWaveformGenerate", "swCreateWaveWindow done\n");

	if (swGenerateWaveform(filename, plugin_name, file_type, NULL,
			       window->wave, edit_type, delay_l, duration_l,
			       f0, phase_radian, gain, /*SP_FALSE*/SP_TRUE) == SP_TRUE) {
	    /* do nothing */
	} else {
            spDebug(10, "swCreateWindowForWaveformGenerate", "swGenerateWaveform failed\n");
        }
    }
    
    spDebug(10, "swCreateWindowForWaveformGenerate", "done\n");
    
    return;
}

swEditType swGetWaveformGenerateEditType(char *string)
{
    swEditType edit_type;
    
    if (strcaseeq(string, "Sine")) {
	edit_type = SW_EDIT_GENERATE_SINE;
    } else if (strcaseeq(string, "Square")) {
	edit_type = SW_EDIT_GENERATE_SQUARE;
    } else if (strcaseeq(string, "Triangle")) {
	edit_type = SW_EDIT_GENERATE_TRIANGLE;
    } else if (strcaseeq(string, "Sawtooth")) {
	edit_type = SW_EDIT_GENERATE_SAWTOOTH;
    } else if (strcaseeq(string, "White Noise")) {
	edit_type = SW_EDIT_GENERATE_WHITE_NOISE;
    } else if (strcaseeq(string, "Recording")) {
	edit_type = SW_EDIT_GENERATE_RECORDING;
    } else {
	edit_type = SW_EDIT_GENERATE_SILENCE;
    }

    return edit_type;
}

void swPopdownWaveformGenerateDialogCB(spComponent component, swWaveformGenerateDialog dialog)
{
    char *string;
    
    spDebug(10, "swPopdownWaveformGenerateDialogCB", "in\n");
    
    /* popdown format dialog */
    spPopdownWindow(dialog->window);
    
    dialog->reason = spGetCallbackReason(component);
    spDebug(10, "swPopdownWaveformGenerateDialogCB", "reason = %d\n", dialog->reason);

    if (dialog->reason == SP_CR_OK) {
#if 1
        dialog->file_type = NULL;
        dialog->plugin_name = NULL;
        
	if ((dialog->filename = xspGetTextString(dialog->filename_field)) != NULL) {
            spDebug(10, "swPopdownWaveformGenerateDialogCB", "filename = %s\n", dialog->filename);
	    if (dialog->file_type_index >= 1) {
		dialog->plugin_name = dialog->plugin_names[dialog->file_type_index];
		dialog->file_type = dialog->file_types[dialog->file_type_index];
		spDebug(10, "swPopdownWaveformGenerateDialogCB", "plugin_name = %s, file_type = %s\n",
			dialog->plugin_name, dialog->file_type);
	    }
        }
#endif
        
	if ((string = xspGetTextString(dialog->waveform_combo)) != NULL) {
	    spDebug(10, "swPopdownWaveformGenerateDialogCB", "waveform string = %s\n", string);
	    dialog->edit_type = swGetWaveformGenerateEditType(string);
	    spDebug(10, "swPopdownWaveformGenerateDialogCB", "edit_type = %d\n", dialog->edit_type);
	    xfree(string);
	}
	if ((string = xspGetTextString(dialog->f0_combo)) != NULL) {
	    spDebug(10, "swPopdownWaveformGenerateDialogCB", "f0 string = %s\n", string);
	    dialog->f0 = atof(string);
	    xfree(string);
	}
	if ((string = xspGetTextString(dialog->phase_combo)) != NULL) {
	    spDebug(10, "swPopdownWaveformGenerateDialogCB", "phase string = %s\n", string);
	    dialog->phase = atof(string);
	    xfree(string);
	}
	if ((string = xspGetTextString(dialog->gain_combo)) != NULL) {
	    spDebug(10, "swPopdownWaveformGenerateDialogCB", "gain string = %s\n", string);
	    dialog->gain = atof(string) / 100.0;
	    xfree(string);
	}
	if ((string = xspGetTextString(dialog->delay_combo)) != NULL) {
	    spDebug(10, "swPopdownWaveformGenerateDialogCB", "delay string = %s\n", string);
	    dialog->delay = atof(string);
	    xfree(string);
	}
	if ((string = xspGetTextString(dialog->duration_combo)) != NULL) {
	    spDebug(10, "swPopdownWaveformGenerateDialogCB", "duration string = %s\n", string);
	    dialog->duration = atof(string);
	    xfree(string);
	}

	if ((string = xspGetTextString(dialog->length_combo)) != NULL) {
	    spDebug(10, "swPopdownWaveformGenerateDialogCB", "length string = %s\n", string);
	    dialog->length_s = atof(string);
	    xfree(string);
	}
	if ((string = xspGetTextString(dialog->bit_combo)) != NULL) {
	    spDebug(10, "swPopdownWaveformGenerateDialogCB", "bit string = %s\n", string);
	    dialog->samp_bit = swGetSampleBit(string);
	    xfree(string);
	}
	if ((string = xspGetTextString(dialog->samp_rate_combo)) != NULL) {
	    spDebug(10, "swPopdownWaveformGenerateDialogCB", "samp_rate string = %s\n", string);
	    dialog->samp_rate = atof(string);
	    xfree(string);
	}
	if ((string = xspGetTextString(dialog->channel_combo)) != NULL) {
	    spDebug(10, "swPopdownWaveformGenerateDialogCB", "channel string = %s\n", string);
	    dialog->num_channel = atoi(string);
	    xfree(string);
	}

	dialog->delay = MAX(dialog->delay, 0.0);
	
	if (dialog->duration <= 0.0 && dialog->length_s - dialog->delay > 0.0) {
	    dialog->duration = dialog->length_s - dialog->delay;
	}

	if (dialog->duration <= 0.0) {
	    spDisplayError(component, NULL, SW_PARAMETER_ERROR_MESSAGE,
			   _("SW_GENERATE_DIALOG_DURATION_LABEL"));
	} else if (dialog->f0 <= 0.0 || dialog->f0 > dialog->samp_rate) {
	    spDisplayError(component, NULL, SW_PARAMETER_ERROR_MESSAGE,
			   _("SW_GENERATE_DIALOG_FREQUENCY_LABEL"));
	} else if (dialog->samp_bit < 8) {
	    spDisplayError(component, NULL, SW_PARAMETER_ERROR_MESSAGE,
			   SW_BIT_LABEL);
	} else if (dialog->samp_rate <= 0.0) {
	    spDisplayError(component, NULL, SW_PARAMETER_ERROR_MESSAGE,
			   SW_SAMP_RATE_LABEL);
	} else if (dialog->num_channel <= 0) {
	    spDisplayError(component, NULL, SW_PARAMETER_ERROR_MESSAGE,
			   SW_CHANNEL_LABEL);
	} else {
	    spDebug(10, "swPopdownWaveformGenerateDialogCB", "before swCreateWindowForWaveformGenerate\n");
	    swCreateWindowForWaveformGenerate(dialog->config,
					      dialog->filename, dialog->plugin_name, dialog->file_type,
					      dialog->samp_bit, dialog->num_channel, dialog->samp_rate,
					      dialog->length_s,
					      dialog->edit_type, dialog->delay, dialog->duration,
					      dialog->f0, dialog->phase, dialog->gain);
	    spDebug(10, "swPopdownWaveformGenerateDialogCB", "after swCreateWindowForWaveformGenerate\n");
	}
    }
    
    return;
}

static void updateWaveformGenerateDialogSensitive(swWaveformGenerateDialog dialog, swEditType edit_type)
{
    if (edit_type == SW_EDIT_GENERATE_SILENCE) {
	spSetSensitive(dialog->f0_combo, SP_FALSE);
	spSetSensitive(dialog->phase_combo, SP_FALSE);
	spSetSensitive(dialog->gain_dB_combo, SP_FALSE);
	spSetSensitive(dialog->gain_combo, SP_FALSE);
	spSetSensitive(dialog->delay_combo, SP_TRUE);
    } else if (edit_type == SW_EDIT_GENERATE_WHITE_NOISE) {
	spSetSensitive(dialog->f0_combo, SP_FALSE);
	spSetSensitive(dialog->phase_combo, SP_FALSE);
	spSetSensitive(dialog->gain_dB_combo, SP_TRUE);
	spSetSensitive(dialog->gain_combo, SP_TRUE);
	spSetSensitive(dialog->delay_combo, SP_TRUE);
    } else if (edit_type == SW_EDIT_GENERATE_RECORDING) {
	spSetSensitive(dialog->f0_combo, SP_FALSE);
	spSetSensitive(dialog->phase_combo, SP_FALSE);
	spSetSensitive(dialog->gain_dB_combo, SP_FALSE);
	spSetSensitive(dialog->gain_combo, SP_FALSE);
	spSetSensitive(dialog->delay_combo, SP_FALSE);
    } else {
	spSetSensitive(dialog->f0_combo, SP_TRUE);
	spSetSensitive(dialog->phase_combo, SP_TRUE);
	spSetSensitive(dialog->gain_dB_combo, SP_TRUE);
	spSetSensitive(dialog->gain_combo, SP_TRUE);
	spSetSensitive(dialog->delay_combo, SP_TRUE);
    }
    
    return;
}

static void selectWaveformComboCB(spComponent component, swWaveformGenerateDialog dialog)
{
    swEditType edit_type;
    char *string;
    
    spDebug(10, "selectWaveformComboCB", "selected\n");

    if ((string = xspGetSelectedListItem(component)) != NULL) {
	edit_type = swGetWaveformGenerateEditType(string);
	spDebug(10, "selectWaveformComboCB", "edit_type = %d\n", edit_type);

	updateWaveformGenerateDialogSensitive(dialog, edit_type);
	
	xfree(string);
    }
    
    spDebug(10, "selectWaveformComboCB", "done\n");
    
    return;
}

static void waveformGenerateChangeGainCB(spComponent component, swWaveformGenerateDialog dialog)
{
    double percentage;
    char *string;
    char buf[SP_MAX_LINE];
    
    if ((string = xspGetTextString(component)) != NULL) {
	spDebug(10, "waveformGenerateChangeGainCB", "string = %s\n", string);
    
	percentage = atof(string);
	percentageTodBString(buf, percentage);
	spSetTextString(dialog->gain_dB_combo, buf);
	
	xfree(string);
    }
    
    spDebug(10, "waveformGenerateChangeGainCB", "done\n");
    
    return;
}

static void waveformGenerateChangeGaindBCB(spComponent component, swWaveformGenerateDialog dialog)
{
    double percentage;
    char *string;
    char buf[SP_MAX_LINE];
    
    if ((string = xspGetTextString(component)) != NULL) {
	spDebug(10, "waveformGenerateChangeGaindBCB", "string = %s\n", string);
	
	percentage = dBStringToPercentage(string, 0.0, 0.0);
	sprintf(buf, "%.1f", percentage);
	spSetTextString(dialog->gain_combo, buf);
	
	xfree(string);
    }
    
    spDebug(10, "waveformGenerateChangeGaindBCB", "done\n");
    
    return;
}


static swWaveformGenerateDialog createWaveformGenerateDialog(swConfig config)
{
    char string[SP_MAX_LINE];
    swWaveformGenerateDialog dialog;
    
    dialog = xalloc(1, struct _swWaveformGenerateDialog);
    memset(dialog, 0, sizeof(struct _swWaveformGenerateDialog));
    dialog->reason = SP_CR_NONE;
    dialog->config = config;

    dialog->edit_type = SW_EDIT_GENERATE_SINE;
    dialog->f0 = 200.0;
    dialog->phase = 0.0;
    /*dialog->gain = -3.0;*/
    dialog->gain = 0.5;
    dialog->delay = 0.0;
    dialog->duration = 1.0;
    
    dialog->length_s = 0.0;
    dialog->samp_bit = config->samp_bit;
    dialog->samp_rate = config->samp_rate;
    dialog->num_channel = config->num_channel;
    
    dialog->window = spCreateDialogBox("waveformGenerateDialog",
				       SppTitle, _("SW_GENERATE_DIALOG_TITLE"),
				       SppCallbackFunc, swPopdownWaveformGenerateDialogCB,
				       SppCallbackData, dialog,
				       SppDialogBoxButtonType, SP_DB_OK_CANCEL,
				       SppCloseStyle, SP_UNMAP_CLOSE,
				       SppSpacingOn, SP_TRUE,
				       SppHelpButtonVisible, SP_TRUE,
				       SppHelpPath, "dialog/generate.html",
				       NULL);

    /* create tab box */
    dialog->tab_box = spCreateTabBox(dialog->window, "waveformGenerateTabBox", 300,
				     NULL);
    
    /* create waveform tab */
    dialog->waveform_tab = spAddTabItem(dialog->tab_box, "waveformGenerateWaveformTab", -1,
					SppTitle, _("SW_GENERATE_DIALOG_WAVEFORM_TAB_LABEL"),
					SppHelpPath, "dialog/generate.html#waveform_tab",
					NULL);
    dialog->waveform_combo = spCreateParamField(dialog->waveform_tab, "waveformGenerateWaveformCombo", 0,
						SppTitle, _("SW_GENERATE_DIALOG_WAVEFORM_LABEL"),
						SppCallbackFunc, selectWaveformComboCB,
						SppCallbackData, dialog,
						SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
						SppEditable, SP_FALSE,
						SppFieldStrings, waveform_list_strings,
						SppFieldOffset, 120,
						SppFieldSize, 140,
						SppHelpPath, "dialog/generate.html#waveform",
						NULL);
    spSelectListItem(dialog->waveform_combo, "Sine");
    dialog->f0_combo = spCreateParamField(dialog->waveform_tab, "waveformGenerateF0Combo", 0,
					  SppTitle, _("SW_GENERATE_DIALOG_FREQUENCY_LABEL"),
					  SppDimension, "Hz",
					  SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
					  SppEditable, SP_TRUE,
					  SppFieldStrings, waveform_f0_list_strings,
					  SppFieldOffset, 120,
					  SppFieldSize, 140,
					  SppHelpPath, "dialog/generate.html#f0",
					  NULL);
    sprintf(string, "%.0f", dialog->f0);
    spSetTextString(dialog->f0_combo, string);
    dialog->phase_combo = spCreateParamField(dialog->waveform_tab, "waveformGeneratePhaseCombo", 0,
					     SppTitle, _("SW_GENERATE_DIALOG_PHASE_LABEL"),
					     SppDimension, "degree",
					     SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
					     SppEditable, SP_TRUE,
					     SppFieldStrings, waveform_phase_list_strings,
					     SppFieldOffset, 120,
					     SppFieldSize, 140,
					     SppHelpPath, "dialog/generate.html#phase",
					     NULL);
    sprintf(string, "%.0f", dialog->phase);
    spSetTextString(dialog->phase_combo, string);

    
    dialog->gain_dB_combo = spCreateParamField(dialog->waveform_tab, "waveformGenerateGaindBCombo", 0,
					       SppTitle, _("SW_GENERATE_DIALOG_GAIN_DB_LABEL"),
					       SppDimension, "dB",
					       SppCallbackFunc, waveformGenerateChangeGaindBCB,
					       SppCallbackData, dialog,
					       SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
					       SppEditable, SP_TRUE,
					       SppFieldStrings, waveform_gain_dB_list_strings,
					       SppFieldOffset, 120,
					       SppFieldSize, 140,
					       SppHelpPath, "dialog/generate.html#gain_dB",
					       NULL);
    percentageTodBString(string, dialog->gain * 100.0);
    spSetTextString(dialog->gain_dB_combo, string);
    
    dialog->gain_combo = spCreateParamField(dialog->waveform_tab, "waveformGenerateGainCombo", 0,
					    SppTitle, _("SW_GENERATE_DIALOG_GAIN_LABEL"),
					    SppDimension, /*"dB"*/"%",
					    SppCallbackFunc, waveformGenerateChangeGainCB,
					    SppCallbackData, dialog,
					    SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
					    SppEditable, SP_TRUE,
					    SppFieldStrings, waveform_gain_list_strings,
					    SppFieldOffset, 120,
					    SppFieldSize, 140,
					    SppHelpPath, "dialog/generate.html#gain",
					    NULL);
    sprintf(string, "%.1f", dialog->gain * 100.0);
    spSetTextString(dialog->gain_combo, string);
    
    dialog->delay_combo = spCreateParamField(dialog->waveform_tab, "waveformGenerateDelayCombo", 0,
					     SppTitle, _("SW_GENERATE_DIALOG_DELAY_LABEL"),
					     SppDimension, "sec",
					     SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
					     SppEditable, SP_TRUE,
					     SppFieldStrings, waveform_delay_list_strings,
					     SppFieldOffset, 120,
					     SppFieldSize, 140,
					     SppHelpPath, "dialog/generate.html#delay",
					     NULL);
    sprintf(string, "%.1f", dialog->delay);
    spSetTextString(dialog->delay_combo, string);
    dialog->duration_combo = spCreateParamField(dialog->waveform_tab, "waveformGenerateDurationCombo", 0,
					     SppTitle, _("SW_GENERATE_DIALOG_DURATION_LABEL"),
					     SppDimension, "sec",
					     SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
					     SppEditable, SP_TRUE,
					     SppFieldStrings, waveform_duration_list_strings,
					     SppFieldOffset, 120,
					     SppFieldSize, 140,
						SppHelpPath, "dialog/generate.html#duration",
					     NULL);
    sprintf(string, "%.1f", dialog->duration);
    spSetTextString(dialog->duration_combo, string);
    
    
    /* create file tab */
    dialog->file_tab = spAddTabItem(dialog->tab_box, "waveformGenerateFileTab", -1,
				    SppTitle, _("SW_GENERATE_DIALOG_FILE_TAB_LABEL"),
				    SppHelpPath, "dialog/generate.html#file_tab",
				    NULL);
    dialog->length_combo = spCreateParamField(dialog->file_tab, "waveformGenerateLengthCombo", 0,
					      SppTitle, _("SW_GENERATE_DIALOG_LENGTH_LABEL"),
					      SppDimension, "sec",
					      SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
					      SppEditable, SP_TRUE,
					      SppFieldStrings, waveform_length_list_strings,
					      SppFieldOffset, 120,
					      SppFieldSize, 140,
					      SppHelpPath, "dialog/generate.html#length",
					      NULL);
    sprintf(string, "%.1f", dialog->length_s);
    spSetTextString(dialog->length_combo, string);
    
    dialog->bit_combo = spCreateParamField(dialog->file_tab, "waveformGenerateBitCombo", 0,
					   SppTitle, SW_BIT_LABEL,
					   SppDimension, "bits/sample",
					   SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
					   SppEditable, SP_FALSE,
					   SppFieldStrings, bit_list_strings,
					   SppFieldOffset, 120,
					   SppFieldSize, 140,
					   SppHelpPath, "dialog/generate.html#bit",
					   NULL);
    swGetSampleBitString(string, dialog->samp_bit);
    spSelectListItem(dialog->bit_combo, string);
    
    dialog->samp_rate_combo = spCreateParamField(dialog->file_tab, "waveformGenerateSampRateCombo", 0,
						 SppTitle, SW_SAMP_RATE_LABEL,
						 SppDimension, "Hz",
						 SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
						 SppEditable, SP_TRUE,
						 SppFieldStrings, samp_rate_list_strings,
						 SppFieldOffset, 120,
						 SppFieldSize, 140,
						 SppHelpPath, "dialog/generate.html#samp_rate",
						 NULL);
    sprintf(string, "%.0f", dialog->samp_rate);
    spSetTextString(dialog->samp_rate_combo, string);

    dialog->channel_combo = spCreateParamField(dialog->file_tab, "waveformGenerateChannelCombo", 0,
					       SppTitle, SW_CHANNEL_LABEL,
					       SppDimension, "channels",
					       SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
					       SppEditable, SP_FALSE,
					       SppFieldStrings, waveform_channel_list_strings,
					       SppFieldOffset, 120,
					       SppFieldSize, 140,
					       SppHelpPath, "dialog/generate.html#channel",
					       NULL);
    sprintf(string, "%d", dialog->num_channel);
    spSelectListItem(dialog->channel_combo, string);

#if 1
    dialog->num_plugin = swGetPluginNames(&dialog->plugin_names, &dialog->file_types, &dialog->file_filters);
    spDebug(50, "createWaveformGenerateDialog", "dialog->num_plugin = %d\n", dialog->num_plugin);
        
    dialog->filename_field = spCreateParamField(dialog->file_tab, "waveformGenerateFilenameField", 0,
                                                SppTitle, SW_FILENAME_LABEL,
                                                SppFieldType, SP_FIELD_TYPE_SAVE_FILE_TEXT,
                                                SppEditable, /*SP_TRUE*/SP_FALSE,
                                                SppInitialFileName, "",
                                                SppPathMustExist, SP_TRUE,
                                                SppOverwritePrompt, SP_TRUE,
                                                SppFileFilters, dialog->file_filters,
                                                SppFileTypes, dialog->file_types,
                                                SppFileFilterIndex, &dialog->file_type_index,
                                                SppFieldOffset, 120,
                                                SppFieldSize, 140,
                                                SppHelpPath, "dialog/generate.html#filename",
                                                NULL);
#endif

    return dialog;
}

void swPopupWaveformGenerateDialogCB(spComponent component, swWindow window)
{
    static swWaveformGenerateDialog dialog = NULL;

    spDebug(10, "swPopupWaveformGenerateDialogCB", "in\n");
    
    if (dialog == NULL) {
	dialog = createWaveformGenerateDialog(window->config);
    }
#if 1
    dialog->file_type_index = 0;
    if (dialog->filename != NULL) {
        xfree(dialog->filename);
        dialog->filename = NULL;
    }
    spSetTextString(dialog->filename_field, "");
#endif

    /* popup dialog */
    spPopupWindow(dialog->window);
    
    spDebug(10, "swPopupWaveformGenerateDialogCB", "done\n");
    
    return;
}

#endif
