/*
 *	swLabelDialog.c
 *
 *	Last modified: <2025-01-19 00:12:17 hideki>
 */

#include <stdio.h>
#include <stdlib.h>

#include <sp/spBaseLib.h>
#include <sp/spComponentLib.h>

#include "swWindow.h"
#include "swDraw.h"
#include "swCursor.h"
#include "swEdit.h"
#include "swLabel.h"
#include "swLabelList.h"
#include "swLabelDialog.h"
#if defined(SW_AH_CUSTOM)
#include "swLabelAH.h"
#endif

#define SW_LABEL_DIALOG_FIELD_HEIGHT 60
#define SW_LABEL_DIALOG_FIELD_OFFSET 60
#define SW_LABEL_DIALOG_FIELD_SIZE 250

static char *label_file_filter[] =
{
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    NULL,
};

static char *label_file_type[] =
{
    "Seconds",
    "Milli Seconds",
    "Points",
    "Separated Seconds (hh:mm:ss.ssss)",
    "Floored Separated Seconds (hh:mm:ss)",
    "Indexed Separated Seconds",
    "Seconds with Data",
    "Milli Seconds with Data",
    "Points with Data",
    "Separated Seconds (hh:mm:ss.ssss) with Data",
    "Floored Separated Seconds (hh:mm:ss) with Data",
    "Indexed Separated Seconds with Data",
    "ESPS",
    NULL,
};

static char *region_label_file_filter[] =
{
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    "*",
    NULL,
};

static char *region_label_file_type[] =
{
    "Seconds",
    "Milli Seconds",
    "Points",
    "Separated Seconds (hh:mm:ss.ssss)",
    "Floored Separated Seconds (hh:mm:ss)",
    "Indexed Separated Seconds",
    "Seconds with Data",
    "Milli Seconds with Data",
    "Points with Data",
    "Separated Seconds (hh:mm:ss.ssss) with Data",
    "Floored Separated Seconds (hh:mm:ss) with Data",
    "Indexed Separated Seconds with Data",
    NULL,
};

static swLabelDialog createLabelInsertDialog(swConfig config)
{
    static swLabelDialog label_dialog = NULL;
    
    if (label_dialog != NULL) return label_dialog;

    /* initialize dialog */
    label_dialog = xalloc(1, struct _swLabelDialog);
    memset(label_dialog, 0, sizeof(struct _swLabelDialog));
    
    label_dialog->region_flag = SP_FALSE;
    label_dialog->current_index = -1;
    label_dialog->current_window = NULL;
    
    /* create dialog */
    label_dialog->window =
	spCreateDialogBox("labelDialog",
			  SppCallbackFunc, swPopdownLabelInsertDialog,
			  SppCallbackData, label_dialog,
			  SppPopupStyle, SP_MODAL_POPUP,
			  SppCloseStyle, SP_UNMAP_CLOSE,
			  SppHelpButtonVisible, SP_TRUE,
			  SppHelpPath, "dialog/label.html",
			  NULL);
    
    /* create text field */
    label_dialog->time_field =
	spCreateParamField(label_dialog->window, "labelDialogTimeField", SW_LABEL_DIALOG_FIELD_HEIGHT,
			   SppTitle, SW_LABEL_DIALOG_TIME_LABEL,
			   SppFieldOffset, SW_LABEL_DIALOG_FIELD_OFFSET,
			   SppFieldSize, SW_LABEL_DIALOG_FIELD_SIZE,
			   SppHelpPath, "dialog/label.html#label_time",
			   NULL);
    label_dialog->end_time_field =
	spCreateParamField(label_dialog->window, "labelDialogEndTimeField", SW_LABEL_DIALOG_FIELD_HEIGHT,
			   SppTitle, SW_LABEL_DIALOG_END_TIME_LABEL,
			   SppFieldOffset, SW_LABEL_DIALOG_FIELD_OFFSET,
			   SppFieldSize, SW_LABEL_DIALOG_FIELD_SIZE,
			   SppHelpPath, "dialog/label.html#label_end",
			   NULL);
    label_dialog->symbol_field =
	spCreateParamField(label_dialog->window, "labelDialogSymbolField", SW_LABEL_DIALOG_FIELD_HEIGHT,
			   SppTitle, SW_LABEL_DIALOG_SYMBOL_LABEL,
			   SppFieldOffset, SW_LABEL_DIALOG_FIELD_OFFSET,
			   SppFieldSize, SW_LABEL_DIALOG_FIELD_SIZE,
			   SppHelpPath, "dialog/label.html#label_symbol",
			   NULL);
    label_dialog->data_field =
	spCreateParamField(label_dialog->window, "labelDialogDataField", SW_LABEL_DIALOG_FIELD_HEIGHT,
			   SppTitle, SW_LABEL_DIALOG_DATA_LABEL,
			   SppFieldOffset, SW_LABEL_DIALOG_FIELD_OFFSET,
			   SppFieldSize, SW_LABEL_DIALOG_FIELD_SIZE,
			   SppHelpPath, "dialog/label.html#label_data",
			   NULL);
    label_dialog->channel_field =
	spCreateParamField(label_dialog->window, "labelDialogChannelField", SW_LABEL_DIALOG_FIELD_HEIGHT,
			   SppTitle, SW_LABEL_DIALOG_CHANNEL_LABEL,
			   SppFieldOffset, SW_LABEL_DIALOG_FIELD_OFFSET,
			   SppFieldSize, SW_LABEL_DIALOG_FIELD_SIZE,
			   SppHelpPath, "dialog/label.html#label_channel",
			   NULL);
    
    return label_dialog;
}

static void setLabelInsertText(swWindow window, swLabelDialog label_dialog, long index)
{
    int channel;
    double point_f;
    double weight;
    char time_string[SW_MAX_LABEL_STRING];
    char end_time_string[SW_MAX_LABEL_STRING];
    char data_string[SW_MAX_LABEL_STRING];
    char symbol_string[SW_MAX_LABEL_STRING];
    char channel_string[SW_MAX_LABEL_STRING];

    if (window == NULL || window->wave == NULL)
	return;

    if (window->config->time_format == SW_TIME_FORMAT_POINT) {
	weight = window->wave->samp_rate;
    } else {
	weight = 1.0;
    }

    time_string[0] = NUL;
    end_time_string[0] = NUL;
    data_string[0] = NUL;
    symbol_string[0] = NUL;
    channel = -1;
    
    if (swIsLabelValid(window->wave, index) == SP_FALSE) {
	point_f = swSampToDim(window, window->point);
    } else {
	if (swIsRegionLabel(window->wave, index) == SP_TRUE) {
	    point_f = window->wave->labels->label[index].end_time;
	    swGetTimeString(weight * point_f, window->config->time_format, end_time_string);
	}
	
	point_f = window->wave->labels->label[index].time;
	sprintf(data_string, "%s", window->wave->labels->label[index].data);
	sprintf(symbol_string, "%s", window->wave->labels->label[index].string);

	channel = window->wave->labels->label[index].channel;
    }

    if (channel >= 0) {
	sprintf(channel_string, "%d", 1 + channel);
    } else {
	channel_string[0] = NUL;
    }
    
    swGetTimeString(weight * point_f, window->config->time_format, time_string);

    spSetTextString(label_dialog->time_field, time_string);
    spSetTextString(label_dialog->end_time_field, end_time_string);
    spSetTextString(label_dialog->symbol_field, symbol_string);
    spSetTextString(label_dialog->data_field, data_string);
    spSetTextString(label_dialog->channel_field, channel_string);
    
    if ((window->label_caps & SW_LABEL_CAPS_CHANNEL_NORMAL) == 0) {
	spSetSensitive(label_dialog->channel_field, SP_FALSE);
    } else {
	spSetSensitive(label_dialog->channel_field, SP_TRUE);
    }

    spDebug(10, "setLabelInsertText", "time: %s\n", time_string);

    return;
}

void swPopupLabelInsertDialog(swWindow window, long index)
{
    swLabelDialog label_dialog;

    if (window == NULL || window->wave == NULL)
	return;

    if ((window->mouse_mode == SW_MOUSE_MODE_NORMAL_LABEL_SINGLE || (window->label_caps & SW_LABEL_CAPS_MULTI_NORMAL) == 0
	 || (window->label_caps & SW_LABEL_CAPS_NONCHANNEL_NORMAL) == 0)
	&& index < 0) {
	if ((swGetNumNormalLabel(window->wave) > 0 || (window->label_caps & SW_LABEL_CAPS_NONCHANNEL_NORMAL) == 0)
	    && (window->active_label_index < 0
		|| window->wave->labels->label[window->active_label_index].end_time >= 0.0)) {
	    spDisplayError(window->window, SW_ERROR_TITLE, SW_FIND_ACTIVE_LABEL_ERROR_MESSAGE);
	    return;
	}
	index = window->active_label_index;
    }
    
    /* create dialog */
    label_dialog = createLabelInsertDialog(window->config);
    label_dialog->region_flag = SP_FALSE;

    if (index >= 0) {
	window->active_label_index = index;
	swRedrawLabels(window);
	swSelectLabelList(window->label_list);
	
	if (swIsRegionLabel(window->wave, index) == SP_TRUE) {
	    label_dialog->region_flag = SP_TRUE;
	    spSetSensitive(label_dialog->end_time_field, SP_TRUE);
	    /*spSetSensitive(label_dialog->symbol_field, SP_FALSE);*/
	} else {
	    spSetSensitive(label_dialog->end_time_field, SP_FALSE);
	    /*spSetSensitive(label_dialog->symbol_field, SP_TRUE);*/
	}
	spSetParams(label_dialog->window,
		    SppTitle, "Change Label",
		    NULL);
    } else {
	spSetSensitive(label_dialog->end_time_field, SP_FALSE);
	/*spSetSensitive(label_dialog->symbol_field, SP_TRUE);*/
	spSetParams(label_dialog->window,
		    SppTitle, "Insert Label",
		    NULL);
    }

    /* set dialog text */
    setLabelInsertText(window, label_dialog, index);

    /* set current window */
    label_dialog->current_window = window;
    label_dialog->current_index = index;

    /* popup dialog */
    spPopupWindow(label_dialog->window);

    return;
}

void swPopdownLabelInsertDialog(spComponent component, swLabelDialog label_dialog)
{
    long index;
    long current_index;
    double weight;
    double time, end_time;
    int channel;
    char *time_string;
    char *end_time_string;
    char *data_string;
    char *symbol_string;
    char *channel_string;
    swTimeFormat format;
    spCallbackReason reason;
    
    if (label_dialog == NULL || spIsCreated(label_dialog->window) == SP_FALSE) return;
    
    reason = spGetCallbackReason(component);
    
    if (reason == SP_CR_OK) {
	if (label_dialog->current_window != NULL
	    && label_dialog->current_window->wave != NULL) {
	    format = label_dialog->current_window->config->time_format;
	    if (format == SW_TIME_FORMAT_POINT) {
		weight = label_dialog->current_window->wave->samp_rate;
	    } else {
		weight = 1.0;
	    }

	    /* get string */
	    time_string = xspGetTextString(label_dialog->time_field);
	    end_time_string = xspGetTextString(label_dialog->end_time_field);
	    symbol_string = xspGetTextString(label_dialog->symbol_field);
	    data_string = xspGetTextString(label_dialog->data_field);
	    channel_string = xspGetTextString(label_dialog->channel_field);

	    if (swConvertTimeString(time_string, format, &time) == SP_TRUE
		&& (label_dialog->region_flag == SP_FALSE
		    || swConvertTimeString(end_time_string, format, &end_time) == SP_TRUE)) {
		time /= weight;

		if (label_dialog->region_flag == SP_TRUE) {
		    end_time /= weight;
		} else {
		    end_time = -1.0;
		}

		if (strnone(channel_string) || streq(channel_string, "0")) {
		    channel = -1;
		} else {
		    channel = atoi(channel_string) - 1;
		}
		spDebug(50, "swPopdownLabelInsertDialog", "channel_string = %s, channel = %d\n",
			channel_string, channel);

		if (label_dialog->current_index >= 0) {
		    current_index = label_dialog->current_index;
		} else {
		    current_index = -1;
		}
		
		if (current_index >= 0) {
		    if ((index = swChangeChannelLabel(label_dialog->current_window->wave,
						      current_index, 
						      time, end_time, channel, data_string, symbol_string)) >= 0) {
			label_dialog->current_window->active_label_index = index;
			swUpdateLabels(label_dialog->current_window);
		    }
		} else {
		    if ((index = swAddChannelLabel(label_dialog->current_window->wave,
						   time, end_time, channel, data_string, symbol_string)) >= 0) {
			label_dialog->current_window->active_label_index = index;
			swSetSenseLevel(label_dialog->current_window);
			swUpdateLabels(label_dialog->current_window);
		    }
		}
	    } else {
		spDisplayError(component, SW_ERROR_TITLE,
			       SW_TIME_FORMAT_ERROR_MESSAGE);
	    }

	    /* memory free */
	    if (time_string != NULL) xfree(time_string);
	    if (end_time_string != NULL) xfree(end_time_string);
	    if (data_string != NULL) xfree(data_string);
	    if (symbol_string != NULL) xfree(symbol_string);
	    if (channel_string != NULL) xfree(channel_string);
	}
    }
    
    /* popdown dialog */
    spPopdownWindow(label_dialog->window);
    
    return;
}


static void openLabel(spComponent component, swWindow window, spBool region_flag)
{
    char *filename = NULL;
    swTimeFormat format_index;
    spDialogResponse response;
    
    if (window == NULL || window->wave == NULL) {
	spDisplayError(component, SW_ERROR_TITLE, SW_FIND_DATA_ERROR_MESSAGE);
	return;
    }

    /* get open file name */
    if (region_flag == SP_TRUE) {
	if (swGetNumRegionLabel(window->wave) > 0) {
	    response = swDisplaySaveChangesDialog(window, SP_TRUE, SP_TRUE);
	    if (response == SP_DR_YES) {
		if (swSaveLabel(window, SP_FALSE, SP_TRUE) == SP_FALSE) {
		    if (swDisplayContinuePrompt(window, SP_FALSE) != SP_DR_YES) {
			return;
		    }
		}
	    } else if (response == SP_DR_CANCEL) {
		return;
	    }
	}
	
	spDebug(50, "openLabel", "region_label_format = %d\n", window->wave->config->region_label_format);
	filename = xspGetOpenFileName(component, NULL,
				      SppFileMustExist, SP_TRUE,
				      SppFileFilters, region_label_file_filter,
				      SppFileTypes, region_label_file_type,
				      SppFileFilterIndex, &window->wave->config->region_label_format,
				      NULL);
	format_index = window->wave->config->region_label_format;
    } else {
	if (swGetNumNormalLabel(window->wave) > 0) {
	    response = swDisplaySaveChangesDialog(window, SP_TRUE, SP_FALSE);
	    
	    if (response == SP_DR_YES) {
		if (swSaveLabel(window, SP_FALSE, SP_FALSE) == SP_FALSE
		    && swDisplayContinuePrompt(window, SP_FALSE) != SP_DR_YES) {
		    return;
		}
	    } else if (response == SP_DR_CANCEL) {
		return;
	    }
	}
	
	filename = xspGetOpenFileName(component, NULL,
				      SppFileMustExist, SP_TRUE,
				      SppFileFilters, label_file_filter,
				      SppFileTypes, label_file_type,
				      SppFileFilterIndex, &window->wave->config->label_format,
				      NULL);
	format_index = window->wave->config->label_format;
    }

    if (filename != NULL) {
	/* read label */
	if (swReadLabel(window->wave, filename, format_index, region_flag) == SP_TRUE) {
	    swSetSenseLevel(window);
	    swUpdateLabels(window);
	} else {
	    spDisplayError(component, SW_ERROR_TITLE, SW_OPEN_ERROR_MESSAGE,
			   filename);
	}
	
	xfree(filename);
    }

    return;
}

void swOpenLabelCB(spComponent component, swWindow window)
{
    openLabel(component, window, SP_FALSE);
}

void swOpenRegionLabelCB(spComponent component, swWindow window)
{
    openLabel(component, window, SP_TRUE);
}

spBool swSaveLabel(swWindow window, spBool save_as, spBool region_flag)
{
    char *filename;
    char *label_filename;
    char filename_buf[SP_MAX_PATHNAME];
    spBool flag;
    swTimeFormat format_index = SW_TIME_FORMAT_SEC;
    
    if (window == NULL || swIsLabelNone(window->wave) == SP_TRUE) {
	spDisplayError(window->window, SW_ERROR_TITLE, SW_FIND_DATA_ERROR_MESSAGE);
	return SP_FALSE;
    }

    format_index = window->wave->labels->format;

    if (region_flag == SP_TRUE) {
	label_filename = swGetRegionLabelFileName(window->wave->labels);
    } else {
	label_filename = swGetLabelFileName(window->wave->labels);
    }

    if (save_as == SP_FALSE && window->config->show_dialog_in_first_label_save == SP_TRUE) {
        if (region_flag == SP_TRUE) {
            if (swHasRegionLabelSavedBefore(window->wave->labels) == SP_FALSE) {
                save_as = SP_TRUE;
            }
        } else {
            if (swHasLabelSavedBefore(window->wave->labels) == SP_FALSE) {
                save_as = SP_TRUE;
            }
        }
    }

    flag = SP_TRUE;
    
    if (save_as == SP_TRUE || label_filename == NULL) {
        if (strnone(label_filename) && !strnone(window->wave->core->orig_filename)) {
            swAddDefaultLabelFileSuffix(window->wave->config, filename_buf, SP_MAX_PATHNAME,
                                        window->wave->core->orig_filename, SP_TRUE, SP_TRUE, region_flag);
        } else {
            if (strnone(label_filename)) {
                filename_buf[0] = NUL;
            } else {
                spStrCopy(filename_buf, SP_MAX_PATHNAME, label_filename);
            }
        }
        
	/* get save file name */
	if (region_flag == SP_TRUE) {
	    filename = xspGetSaveFileName(window->window, NULL,
					  SppInitialFileName, filename_buf,
					  SppPathMustExist, SP_TRUE,
					  SppFileMustExist, SP_FALSE,
					  SppOverwritePrompt, SP_TRUE,
					  SppFileFilters, region_label_file_filter,
					  SppFileTypes, region_label_file_type,
					  SppFileFilterIndex, &format_index,
					  NULL);
	} else {
	    filename = xspGetSaveFileName(window->window, NULL,
					  SppInitialFileName, filename_buf,
					  SppPathMustExist, SP_TRUE,
					  SppFileMustExist, SP_FALSE,
					  SppOverwritePrompt, SP_TRUE,
					  SppFileFilters, label_file_filter,
					  SppFileTypes, label_file_type,
					  SppFileFilterIndex, &format_index,
					  NULL);
	}
	
	if (filename != NULL) {
            if (window->config->add_default_label_suffix_for_save_as == SP_TRUE) {
                swAddDefaultLabelFileSuffix(window->wave->config, filename_buf, SP_MAX_PATHNAME,
                                            filename, SP_FALSE, SP_FALSE, region_flag);
            } else {
                spStrCopy(filename_buf, SP_MAX_PATHNAME, filename);
            }
            
	    /* write label */
	    if (swWriteLabel(window->wave, window->name,
			     filename_buf, format_index, region_flag) != SP_TRUE) {
		spDisplayError(window->window, SW_ERROR_TITLE, SW_SAVE_ERROR_MESSAGE, filename_buf);
		flag = SP_FALSE;
	    }
	
	    xfree(filename);
	}
    } else {
	/* write label */
	if (swWriteLabel(window->wave, window->name,
			 label_filename, format_index, region_flag) != SP_TRUE) {
	    spDisplayError(window->window, SW_ERROR_TITLE, SW_SAVE_ERROR_MESSAGE,
			   label_filename);
	    flag = SP_FALSE;
	}
    }
    

    return flag;
}

void swSaveLabelCB(spComponent component, swWindow window)
{
    swSaveLabel(window, SP_FALSE, SP_FALSE);
    return;
}

void swSaveRegionLabelCB(spComponent component, swWindow window)
{
    swSaveLabel(window, SP_FALSE, SP_TRUE);
    return;
}

void swSaveAsLabelCB(spComponent component, swWindow window)
{
    swSaveLabel(window, SP_TRUE, SP_FALSE);
    return;
}

void swSaveAsRegionLabelCB(spComponent component, swWindow window)
{
    swSaveLabel(window, SP_TRUE, SP_TRUE);
    return;
}

static void clearLabel(spComponent component, swWindow window, swLabelType label_type)
{
    if (window == NULL || window->wave == NULL || window->wave->labels == NULL) return;
    
    if (spCreateMessageBox(window->window, NULL,
			   SW_CLEAR_LABEL_QUESTION_MESSAGE,
			   SppDialogType, SP_QUESTION_DIALOG,
			   SppMessageBoxButtonType, SP_MB_YES_NO,
			   NULL) == SP_DR_YES) {
	swClearLabels(window->wave->labels, label_type);
	
	swSetSenseLevel(window);
	swUpdateLabels(window);
    }
    
    return;
}

void swClearLabelCB(spComponent component, swWindow window)
{
    clearLabel(component, window, SW_LABEL_TYPE_NORMAL);
    return;
}

void swClearRegionLabelCB(spComponent component, swWindow window)
{
    clearLabel(component, window, SW_LABEL_TYPE_REGION);
    return;
}

void swInsertLabelCB(spComponent component, void *data)
{
    swWindow window = (swWindow)data;
    
    swPopupLabelInsertDialog(window, -1);

    return;
}

void swChangeLabelCB(spComponent component, void *data)
{
    long index;
    double point_f;
    swWindow window = (swWindow)data;

    if (window == NULL || window->wave == NULL)
	return;

    point_f = swSampToDim(window, window->point);
    
    if ((index = swFindNearLabelIndex(window, point_f, SP_FALSE, NULL)) >= 0) {
	swPopupLabelInsertDialog(window, index);
    } else {
	spDisplayError(component, SW_ERROR_TITLE, SW_FIND_LABEL_NEAR_CURSOR_ERROR_MESSAGE);
    }

    return;
}

void swInsertChannelLabel(swWindow window, double point_f, int channel, char *string)
{
    long index = -1;

    if (window == NULL || window->wave == NULL)
	return;

    if (((window->label_caps & SW_LABEL_CAPS_NONCHANNEL_NORMAL) == 0 && channel < 0)
	|| ((window->mouse_mode == SW_MOUSE_MODE_NORMAL_LABEL_SINGLE || (window->label_caps & SW_LABEL_CAPS_MULTI_NORMAL) == 0)
	    && swGetNumChannelLabel(window->wave, channel, SP_FALSE) > 0)) {
	index = window->active_label_index;

	if (index < 0
	    || (window->wave->labels->label[index].end_time >= 0.0
		|| window->wave->labels->label[index].channel != channel)) {
	    spDisplayError(window->window, SW_ERROR_TITLE, SW_FIND_ACTIVE_LABEL_ERROR_MESSAGE);
	    return;
	}
    }

    if (index >= 0) {
	if (strnone(string)) {
	    string = window->wave->labels->label[index].string;
	}
	index = swChangeChannelLabel(window->wave,
				     index, 
				     point_f, -1,
				     window->wave->labels->label[index].channel,
				     window->wave->labels->label[index].data,
				     string);
    } else {
	index = swAddChannelLabel(window->wave, point_f, -1.0, channel, NULL, string);
    }

    if (index >= 0) {
	window->active_label_index = index;
	swSetSenseLevel(window);
	swUpdateLabels(window);
    }

    return;
}

void swInsertSimpleLabelCB(spComponent component, swWindow window)
{
    int channel = -1;
    double point_f;

    if (window == NULL || window->wave == NULL)
	return;

    if ((window->mouse_mode == SW_MOUSE_MODE_NORMAL_LABEL_SINGLE || (window->label_caps & SW_LABEL_CAPS_MULTI_NORMAL) == 0)
	&& swGetNumNormalLabel(window->wave) > 0
	&& window->active_label_index >= 0
	&& window->wave->labels->label[window->active_label_index].end_time < 0.0) {
	channel = window->wave->labels->label[window->active_label_index].channel;
    }

    point_f = swSampToDim(window, window->point);
    swInsertChannelLabel(window, point_f, channel, NULL);

    return;
}

void swInsertValueLabelCB(spComponent component, swWindow window)
{
    int channel = -1;
    double point_f;
    double value;
    spLong data_offset;
    char *string;
    char buf[SP_MAX_MESSAGE];

    if (window == NULL || window->wave == NULL)
	return;

    if ((window->mouse_mode == SW_MOUSE_MODE_NORMAL_LABEL_SINGLE || (window->label_caps & SW_LABEL_CAPS_MULTI_NORMAL) == 0)
	&& swGetNumNormalLabel(window->wave) > 0
	&& window->active_label_index >= 0
	&& window->wave->labels->label[window->active_label_index].end_time < 0.0) {
	channel = window->wave->labels->label[window->active_label_index].channel;
    }

    point_f = swSampToDim(window, window->point);
    data_offset = (window->point - window->offset) / window->wave->thin_length;

    if (swGetWaveData(window->wave, window->target_channel, window->target_order, data_offset, &value) == SP_FALSE) {
	string = NULL;
    } else {
	if (swIsWaveFloat(window->wave) == SP_TRUE) {
	    sprintf(buf, "%.6f", value);
	} else {
	    sprintf(buf, "%.0f", value);
	}
	string = buf;
    }
    
    swInsertChannelLabel(window, point_f, channel, string);

    return;
}

spBool swEraseLabelPrompt(spComponent component, swWindow window)
{
    if (swIsLabelNone(window->wave) == SP_TRUE) return SP_FALSE;
    
    if (window->config->erase_label_prompt == SP_FALSE)
	return SP_TRUE;
    
    if (spCreateMessageBox(component, NULL,
			   SW_ERASE_LABEL_QUESTION_MESSAGE,
			   SppTitle, SW_ERASE_LABEL_QUESTION_TITLE,
			   SppDialogType, SP_QUESTION_DIALOG,
			   SppMessageBoxButtonType, SP_MB_OK_CANCEL,
			   NULL) == SP_DR_OK) {
	return SP_TRUE;
    }

    return SP_FALSE;
}

void swEraseLabelIndex(spComponent component, swWindow window, long label_index)
{
    if (window == NULL || window->wave == NULL
	|| swEraseLabelPrompt(component, window) == SP_FALSE)
	return;

    swClearLabel(window->wave->labels, label_index);
    swSetSenseLevel(window);
    swUpdateLabels(window);
    
    return;
}

void swEraseActiveLabel(swWindow window)
{
    long label_index;
    
    if (window == NULL || window->wave == NULL) return;
    
    if (window->active_label_index >= 0) {
	label_index = window->active_label_index;
	window->active_label_index = -1;
	swEraseLabelIndex(window->window, window, label_index);
    }
    
    return;
}

void swEraseLabelCB(spComponent component, swWindow window)
{
    double point_f;

    if (window == NULL || window->wave == NULL
	|| swEraseLabelPrompt(window->window, window) == SP_FALSE)
	return;

    point_f = swSampToDim(window, window->point);

    if (swEraseLabel(window, point_f) == SP_TRUE) {
	swSetSenseLevel(window);
	swUpdateLabels(window);
    } else {
	spDisplayError(component, SW_ERROR_TITLE, SW_FIND_LABEL_NEAR_CURSOR_ERROR_MESSAGE);
    }

    return;
}

void swEraseLabelRegionCB(spComponent component, swWindow window)
{
    spLong offset, length;
    
    if (window == NULL || window->wave == NULL)
	return;
    
    if (swGetEdge(window, window->sel_st, 
		  window->sel_ed, &offset, &length) == SP_TRUE) {
	if (swEraseLabelRegion(window, offset, length) == SP_TRUE) {
	    swSetSenseLevel(window);
	    swUpdateLabels(window);
	}
    } else {
	spDisplayError(component, SW_ERROR_TITLE, SW_REGION_ERROR_MESSAGE);
    }
    
    return;
}

spBool swGetLabelEdge(swWindow window, long index, int *channel, spLong *st, spLong *ed)
{
    if (window == NULL || window->wave == NULL
	|| swIsLabelValid(window->wave, index) == SP_FALSE)
	return SP_FALSE;

    if (channel != NULL) {
	*channel = window->wave->labels->label[index].channel;
    }
    
    if (swIsRegionLabel(window->wave, index) == SP_TRUE) {
	*st = swDimToSamp(window, window->wave->labels->label[index].time);
	*ed = swDimToSamp(window, window->wave->labels->label[index].end_time);
    } else {
	long next_index;
    
	*st = swDimToSamp(window, window->wave->labels->label[index].time);
	if ((next_index = swFindNeighborLabelIndex(window->wave, index, SP_FALSE)) >= 0) {
	    *ed = swDimToSamp(window, window->wave->labels->label[next_index].time);
	} else {
	    *ed = window->wave->total_length - 1;
	}
    }

    return SP_TRUE;
}

void swSelectBetweenLabelsCB(spComponent component, swWindow window)
{
    long index;
    int channel = -1;
    spLong st, ed;
    double point_f;
    spBool flag = SP_FALSE;

    if (window == NULL || window->wave == NULL || window->wave->labels == NULL)
	return;

    point_f = swSampToDim(window, window->point);

    if ((index = swFindLabelIndex(window, point_f, -1, SP_TRUE, SP_FALSE, NULL, NULL)) >= 0) {
	if (swGetLabelEdge(window, index, &channel, &st, &ed) == SP_TRUE) {
	    swSelectRegion(window, channel, st, ed);
	    flag = SP_TRUE;
	}
    } else if ((index = swFindLabelIndex(window, point_f, 1, SP_TRUE, SP_FALSE, NULL, NULL)) >= 0) {
	ed = swDimToSamp(window, window->wave->labels->label[index].time);
	channel = window->wave->labels->label[index].channel;
	swSelectRegion(window, channel, 0, ed);
	flag = SP_TRUE;
    }

    if (flag == SP_FALSE) {
	spDisplayError(component, SW_ERROR_TITLE, SW_FIND_LABEL_ERROR_MESSAGE);
    }

    return;
}

void swSetRegionLabelCB(spComponent component, swWindow window)
{
    long index;
    spLong offset, length;
    
    if (window == NULL || window->wave == NULL)
	return;
    
    if (swGetEdge(window, window->sel_st, 
		  window->sel_ed, &offset, &length) == SP_TRUE) {
	/* add label */
	if ((index = swAddLabel(window->wave, swSampToDim(window, offset),
				swSampToDim(window, offset + length - 1), NULL, NULL)) >= 0) {
	    window->active_label_index = index;
	    swSetSenseLevel(window);
	    swUpdateLabels(window);

	    swMoveCursor(window, offset, SP_TRUE);
	}
    } else {
	spDisplayError(component, SW_ERROR_TITLE, SW_REGION_ERROR_MESSAGE);
    }
    
    return;
}

spBool swSetLabelAsRegion(swWindow window, long index)
{
    int channel = -1;
    spLong st, ed;
    
    if (window == NULL || window->wave == NULL)
	return SP_FALSE;

    if (swGetLabelEdge(window, index, &channel, &st, &ed) == SP_TRUE) {
	swSelectRegion(window, channel, st, ed);
	return SP_TRUE;
    }

    return SP_FALSE;
}

void swSetRegionLabelAsRegionCB(spComponent component, swWindow window)
{
    long index;
    double point_f;

    if (window == NULL || window->wave == NULL)
	return;

    point_f = swSampToDim(window, window->point);
    index = swFindNearLabelIndex(window, point_f, SP_TRUE, NULL);
    
    if (swSetLabelAsRegion(window, index) == SP_FALSE) {
	spDisplayError(component, SW_ERROR_TITLE, SW_FIND_LABEL_NEAR_CURSOR_ERROR_MESSAGE);
    }

    return;
}

void swDivideRegionLabelCB(spComponent component, swWindow window)
{
    long index;
    
    if (swIsNoWave(window) == SP_TRUE || window->wave->labels == NULL) {
	return;
    }

    if ((index = swFindLabelIndex(window, window->point_f,
				  -1, SP_FALSE, SP_TRUE, NULL, NULL)) >= 0
	&& window->point_f > window->wave->labels->label[index].time
	&& window->point_f < window->wave->labels->label[index].end_time) {
	swAddLabel(window->wave, window->point_f,
		   window->wave->labels->label[index].end_time,
		   NULL, NULL);
	
	swSetLabelEndTime(window->wave, index, window->point_f);
	
	swSetSenseLevel(window);
	swUpdateLabels(window);
    } else {
	spDisplayError(component, SW_ERROR_TITLE, SW_FIND_LABEL_ERROR_MESSAGE);
    }

    return;
}

void swCatRegionLabels(swWindow window, long index, spBool end_is_nearer)
{
    long target_index;
    long start_index, end_index;
    double start_time, end_time;
    char data[SW_MAX_LABEL_STRING];
    char string[SW_MAX_LABEL_STRING];

    if (window == NULL || window->wave == NULL
	|| window->wave->labels == NULL	|| index < 0)
	return;
    
    target_index = swFindNeighborLabelIndex(window->wave, index, spIsFalse(end_is_nearer));

    if (target_index >= 0) {
	if (end_is_nearer == SP_TRUE) {
	    start_index = index;
	    end_index = target_index;
	} else {
	    start_index = target_index;
	    end_index = index;
	}
	start_time = window->wave->labels->label[start_index].time;
	end_time = window->wave->labels->label[end_index].end_time;

	spStrCopy(data, SW_MAX_LABEL_STRING, window->wave->labels->label[start_index].data);
#if 0 /* don't concatenate data */
	if (!strnone(window->wave->labels->label[start_index].data)) {
	    spStrCat(data, SW_MAX_LABEL_STRING, " ");
	}
	spStrCat(data, SW_MAX_LABEL_STRING, window->wave->labels->label[end_index].data);
#endif
	
	spStrCopy(string, SW_MAX_LABEL_STRING, window->wave->labels->label[start_index].string);
	if (!strnone(window->wave->labels->label[start_index].string)) {
	    spStrCat(string, SW_MAX_LABEL_STRING, " ");
	}
	spStrCat(string, SW_MAX_LABEL_STRING, window->wave->labels->label[end_index].string);
	
	swChangeLabel(window->wave, start_index, start_time, end_time, data, string);

	if (end_index == window->active_label_index) {
	    window->active_label_index = start_index;
	}
	swClearLabel(window->wave->labels, end_index);
	
	swSetSenseLevel(window);
	swUpdateLabels(window);
    }
    
    return;
}

void swCatRegionLabelsCB(spComponent component, swWindow window)
{
    long index;
    spBool end_is_nearer = SP_FALSE;

    if (swIsNoWave(window) == SP_TRUE) {
	return;
    }

    if ((index = swFindNearLabelIndex(window, swSampToDim(window, window->point),
				      SP_TRUE, &end_is_nearer)) >= 0) {
	swCatRegionLabels(window, index, end_is_nearer);
    } else if ((index = window->active_label_index) >= 0) {	/* maybe called by shortcut key */
	swCatRegionLabels(window, index, SP_FALSE);
    }
    
    return;
}

spBool swFindIdenticalLabelIndex(swWindow window, double time, 
				spBool region_flag, long *pindex, 
				spBool *start_flag, spBool *end_flag)
{
    long k;
    spBool flag;
    double fragment;

    if (window == NULL || window->wave == NULL
	|| window->wave->labels == NULL || pindex == NULL) return SP_FALSE;

    flag = SP_FALSE;
    if (start_flag != NULL) *start_flag = SP_FALSE;
    if (end_flag != NULL) *end_flag = SP_FALSE;
    
    fragment = 1.0 / window->wave->samp_rate / 8.0;
    *pindex = MAX(*pindex, -1);

    for (k = *pindex + 1; k < window->wave->labels->num_buffer; k++) {
	if (k == window->active_label_index
	    || (region_flag != swIsRegionLabel(window->wave, k))) {
	    continue;
	}
	
	if (window->wave->labels->label[k].time >= 0.0
	    && FABS(time - window->wave->labels->label[k].time) < fragment) {
	    *pindex = k;
	    flag = SP_TRUE;
	    if (start_flag != NULL) *start_flag = SP_TRUE;
	}
	if (window->wave->labels->label[k].end_time >= 0.0
	    && FABS(time - window->wave->labels->label[k].end_time) < fragment) {
	    *pindex = k;
	    flag = SP_TRUE;
	    if (end_flag != NULL) *end_flag = SP_TRUE;
	}

	if (flag == SP_TRUE) {
	    break;
	}
    }
    
    return flag;
}

spBool swReplaceIdenticalLabelIndex(swWindow window, spBool drag_end_flag,
				    long orig_index, double target_time)
{
    long k;
    long index, prev_index;
    double orig_time;
    double start_time, end_time;
    double left_time, right_time;
    double fragment;
    spBool move_right_flag;
    spBool found_flag;
    spBool start_flag, end_flag;

    start_time = swGetLabelStartTime(window->wave, orig_index);
    end_time = swGetLabelEndTime(window->wave, orig_index);
    
    if (end_time < 0.0) return SP_FALSE;

    fragment = 1.0 / window->wave->samp_rate;
    
    if (drag_end_flag == SP_TRUE) {
	orig_time = end_time;
    } else {
	orig_time = start_time;
    }

    move_right_flag = SP_FALSE;
    
    if (start_time < end_time) {
	if ((drag_end_flag == SP_TRUE && end_time < target_time)
	    || (drag_end_flag == SP_FALSE && start_time < target_time)) {
	    move_right_flag = SP_TRUE;
	}
	
	if (end_time < target_time) {
	    left_time = start_time;
	    right_time = target_time;
	} else if (target_time < start_time) {
	    left_time = target_time;
	    right_time = end_time;
	} else {
	    left_time = start_time;
	    right_time = end_time;
	}
    } else {
	if ((drag_end_flag == SP_FALSE && start_time < target_time)
	    || (drag_end_flag == SP_TRUE && end_time < target_time)) {
	    move_right_flag = SP_TRUE;
	}
	
	if (start_time < target_time) {
	    left_time = end_time;
	    right_time = target_time;
	} else if (target_time < end_time) {
	    left_time = target_time;
	    right_time = start_time;
	} else {
	    left_time = end_time;
	    right_time = start_time;
	}
    }
	
    spDebug(50, "swReplaceIdenticalLabelIndex", "orig_time = %f, target_time = %f, move_right_flag = %d\n",
	    orig_time, target_time, move_right_flag);
    
    index = -1;
    prev_index = -1;
    while (1) {
	found_flag = swFindIdenticalLabelIndex(window, orig_time,
					       SP_TRUE, &index, 
					       &start_flag, &end_flag);

	if (found_flag == SP_FALSE) {
	    index = window->wave->labels->num_buffer;
	} else if (index != orig_index){
	    if (start_flag == SP_TRUE) {
		if (window->wave->labels->label[index].end_time < target_time) {
		    window->wave->labels->label[index].time
			= window->wave->labels->label[index].end_time;
		    window->wave->labels->label[index].end_time = target_time;
		} else {
		    window->wave->labels->label[index].time = target_time;
		}
	    }
	    if (end_flag == SP_TRUE) {
		if (target_time < window->wave->labels->label[index].time) {
		    window->wave->labels->label[index].end_time
			= window->wave->labels->label[index].time;
		    window->wave->labels->label[index].time = target_time;
		} else {
		    window->wave->labels->label[index].end_time = target_time;
		}
	    }
	}

	spDebug(50, "swReplaceIdenticalLabelIndex", "prev_index = %ld, index = %ld, num_buffer = %ld\n",
		prev_index, index, window->wave->labels->num_buffer);

#if 1
	for (k = prev_index + 1; k < index; k++) {
	    if (k != orig_index && swIsRegionLabel(window->wave, k) == SP_TRUE) {
		if (move_right_flag == SP_TRUE) {
		    if (left_time - fragment <= window->wave->labels->label[k].time
			&& window->wave->labels->label[k].time <= right_time + fragment) {
			window->wave->labels->label[k].time = right_time + fragment;
			window->wave->labels->label[k].end_time
			    = MAX(window->wave->labels->label[k].time + fragment, window->wave->labels->label[k].end_time);
			spDebug(50, "swReplaceIdenticalLabelIndex",
				"move right: k = %ld, target_time = %f, time = %f, end_time = %f\n",
				k, target_time, window->wave->labels->label[k].time, window->wave->labels->label[k].end_time);
		    }
		} else {
		    if (left_time - fragment <= window->wave->labels->label[k].end_time
			&& window->wave->labels->label[k].end_time <= right_time + fragment) {
			window->wave->labels->label[k].end_time = MAX(left_time - fragment, 0.0);
			window->wave->labels->label[k].time
			    = MIN(MAX(window->wave->labels->label[k].end_time - fragment, 0.0),
				  window->wave->labels->label[k].time);
			spDebug(50, "swReplaceIdenticalLabelIndex",
				"move left: k = %ld, target_time = %f, time = %f, end_time = %f\n",
				k, target_time, window->wave->labels->label[k].time, window->wave->labels->label[k].end_time);
		    }
		}
	    }
	}
#endif
	
	if (found_flag == SP_FALSE) {
	    break;
	}

	prev_index = index;
    }
    
    return SP_TRUE;
}
     

long swFindChannelLabelIndex(swWindow window, double time, int channel, int direction,
			     spBool normal_only, spBool region_only,
			     spBool *end_is_nearer, spLong *pmin_dist_l)
{
    long k;
    long index;
    double dist;
    double min_dist;
    spLong min_dist_l;
    spBool end_flag = SP_FALSE;

    if (window == NULL || window->wave == NULL || window->wave->labels == NULL)
	return -1;

    index = -1;
    /* min_dist = FABS(window->wave->labels->label[0].time - time); */
    min_dist = 1000000000.0;
    for (k = 0; k < window->wave->labels->num_buffer; k++) {
	if ((region_only == SP_TRUE && swIsRegionLabel(window->wave, k) == SP_FALSE)
	    || (normal_only == SP_TRUE && swIsRegionLabel(window->wave, k) == SP_TRUE)) {
	    continue;
	}
	if ((direction > 0 && window->wave->labels->label[k].time < time)
	    || (direction < 0 && window->wave->labels->label[k].time > time)) {
	    continue;
	}
	
	if (window->wave->labels->label[k].time >= 0.0) {
	    dist = FABS(window->wave->labels->label[k].time - time);
	    
	    if ((dist < min_dist
		 || (dist == min_dist && index != window->active_label_index))
		&& (channel < 0 || channel == window->wave->labels->label[k].channel)) {
		end_flag = SP_FALSE;
		min_dist = dist;
		index = k;
	    }
	    
	    if (swIsRegionLabel(window->wave, k) == SP_TRUE) {
		dist = FABS(window->wave->labels->label[k].end_time - time);
	    
		if (dist < min_dist
		    && (channel < 0 || channel == window->wave->labels->label[k].channel)) {
		    end_flag = SP_TRUE;
		    min_dist = dist;
		    index = k;
		}
	    }
	}
    }
    spDebug(100, "swFindChannelLabelIndex", "index = %ld, min_dist = %f, end_flag = %d\n",
	    index, min_dist, end_flag);

    min_dist_l = swDimToSamp(window, min_dist);
    if (min_dist_l < window->wave->total_length) {
	if (pmin_dist_l != NULL) {
	    *pmin_dist_l = min_dist_l;
	}
	if (end_is_nearer != NULL) {
	    *end_is_nearer = end_flag;
	}
	return index;
    }

    return -1;
}

long swFindLabelIndex(swWindow window, double time, int direction,
		      spBool normal_only, spBool region_only,
		      spBool *end_is_nearer, spLong *pmin_dist_l)
{
    return swFindChannelLabelIndex(window, time, -1, direction,
				   normal_only, region_only,
				   end_is_nearer, pmin_dist_l);

}

long swFindNearChannelLabelIndex(swWindow window, double time, int channel,
				 spBool region_only, spBool *end_is_nearer)
{
    long index;
    spLong min_dist_l;
    int min_dist_d;

    if (window != NULL && window->draw_label == SP_TRUE) {
	if ((index = swFindChannelLabelIndex(window, time, channel, 0, SP_FALSE, region_only,
					     end_is_nearer, &min_dist_l)) >= 0) {
	    min_dist_d = swLengthToDisp(window, min_dist_l);
	
	    if (min_dist_d <= /*SW_LABEL_DELETE_RANGE / 2 */SW_EDGE_MOTION_RANGE / 2) {
		return index;
	    }
	}
    }

    return -1;
}

long swFindNearLabelIndex(swWindow window, double time, spBool region_only, spBool *end_is_nearer)
{
    return swFindNearChannelLabelIndex(window, time, -1, region_only, end_is_nearer);
}

spBool swEraseLabel(swWindow window, double time)
{
    long index;

    if (window == NULL || window->wave == NULL || window->wave->labels == NULL)
	return SP_FALSE;

    if ((index = swFindNearLabelIndex(window, time, SP_FALSE, NULL)) >= 0) {
	swClearLabel(window->wave->labels, index);

	if (index == window->active_label_index) {
	    window->active_label_index = - 1;
	}
	
	return SP_TRUE;
    }
    
    return SP_FALSE;
}

spBool swCropLabel(swWindow window, spLong offset, spLong length)
{
    long k;
    double stp, len;

    if (window == NULL || window->wave == NULL || window->wave->labels == NULL) {
	return SP_FALSE;
    }

    stp = swSampToDim(window, offset);
    len = swSampToDim(window, length);

    spDebug(10, "swCropLabel", "stp = %ld, len = %ld\n", stp, len);
    
    for (k = 0; k < window->wave->labels->num_buffer; k++) {
	if (window->wave->labels->label[k].time >= 0.0) {
	    /* shift time */
	    window->wave->labels->label[k].time -= stp;

	    if (window->wave->labels->label[k].end_time >= 0.0) {
		window->wave->labels->label[k].end_time -= stp;
		if (window->wave->labels->label[k].end_time > len) {
		    window->wave->labels->label[k].end_time = len;
		}
		if (window->wave->labels->label[k].end_time > 0.0
		    && window->wave->labels->label[k].time < 0.0) {
		    window->wave->labels->label[k].time = 0.0;
		}
		window->wave->labels->region_edit_flag = SP_TRUE;
	    } else {
		window->wave->labels->edit_flag = SP_TRUE;
	    }
	    
	    /* if over region, erase label */
	    if (window->wave->labels->label[k].time < 0.0 || window->wave->labels->label[k].time >= len) {
		swClearLabel(window->wave->labels, k);
	    }
	}
    }

    swSetSenseLevel(window);
    swUpdateLabels(window);
    
    return SP_TRUE;
}

spBool swDeleteLabel(swWindow window, spLong offset, spLong length)
{
    long k;
    double stp, len;
    spBool clear_flag;

    if (window == NULL || window->wave == NULL || window->wave->labels == NULL) {
	return SP_FALSE;
    }

    stp = swSampToDim(window, offset);
    len = swSampToDim(window, length);

    for (k = 0; k < window->wave->labels->num_buffer; k++) {
	if (window->wave->labels->label[k].time >= 0.0) {
	    clear_flag = SP_FALSE;
	    if (window->wave->labels->label[k].time >= stp + len) {
		/* shift time */
		window->wave->labels->label[k].time -= len;
	    } else if (window->wave->labels->label[k].time >= stp) {
		clear_flag = SP_TRUE;
	    }

	    if (swIsRegionLabel(window->wave, k) == SP_TRUE) {
		if (window->wave->labels->label[k].end_time > stp + len) {
		    if (clear_flag == SP_TRUE) {
			window->wave->labels->label[k].time = stp;
			clear_flag = SP_FALSE;
		    }
		    /* shift time */
		    window->wave->labels->label[k].end_time -= len;
		} else if (window->wave->labels->label[k].end_time > stp) {
		    window->wave->labels->label[k].end_time = stp;
		}
		window->wave->labels->region_edit_flag = SP_TRUE;
	    } else {
		window->wave->labels->edit_flag = SP_TRUE;
	    }

	    if (clear_flag == SP_TRUE) {
		/* if inside region, erase label */
		swClearLabel(window->wave->labels, k);
	    }
	}
    }

    swSetSenseLevel(window);
    swUpdateLabels(window);
    
    return SP_TRUE;
}

spBool swExtractLabel(swWindow newwindow, swWindow window,
		      spLong offset, spLong length)
{
    int flag = 0;
    long k;
    double stp, edp;
    double start_time, end_time;

    if (window == NULL || window->wave == NULL || window->wave->labels == NULL ||
	newwindow == NULL) {
	return SP_FALSE;
    }

    stp = swSampToDim(window, offset);
    /*edp = stp + swSampToDim(window, length);*/
    edp = swSampToDim(window, offset + length - 1);
    spDebug(80, "swExtractLabel", "stp = %f, edp = %f\n", stp, edp);

    for (k = 0; k < window->wave->labels->num_buffer; k++) {
	if (window->wave->labels->label[k].time >= 0.0) {
	    start_time = -1.0;
	    end_time = -1.0;
	    
	    if (window->wave->labels->label[k].end_time >= 0.0) {
		if (window->wave->labels->label[k].time >= stp
		    && window->wave->labels->label[k].time < edp) {
		    start_time = window->wave->labels->label[k].time - stp;
		    spDebug(80, "swExtractLabel", "start time = %f\n", start_time);
		}
		if (window->wave->labels->label[k].end_time > stp
		    && window->wave->labels->label[k].end_time < edp) {
		    end_time = window->wave->labels->label[k].end_time - stp;
		    spDebug(80, "swExtractLabel", "end time = %f\n", end_time);
		    if (start_time < 0.0) {
			start_time = 0.0;
		    }
		} else if (start_time >= 0.0) {
		    end_time = edp - stp;
		}
	    } else {
		if (window->wave->labels->label[k].time >= stp
		    && window->wave->labels->label[k].time < edp) {
		    start_time = window->wave->labels->label[k].time - stp;
		}
	    }

	    /* if inside region, add label */
	    if (start_time >= 0.0) {
		swAddChannelLabel(newwindow->wave, start_time, end_time,
				  window->wave->labels->label[k].channel,
				  window->wave->labels->label[k].data,
				  window->wave->labels->label[k].string);
		flag = 1;
	    }
	}
    }

    if (flag) {
	swSetSenseLevel(newwindow);
	swUpdateLabels(newwindow);
    
	return SP_TRUE;
    } else {
	return SP_FALSE;
    }
}

spBool swEraseLabelRegion(swWindow window, spLong offset, spLong length)
{
    int i;
    long k;
    double stp, edp;
    spBool flag = SP_FALSE;

    if (window == NULL || window->wave == NULL || window->wave->labels == NULL) {
	return SP_FALSE;
    }

    stp = swSampToDim(window, offset);
    edp = stp + swSampToDim(window, length);

    for (i = 0; i < 2; i++) {
	for (k = 0; k < window->wave->labels->num_buffer; k++) {
	    if (window->wave->labels->label[k].time >= 0.0) {
		if (window->wave->labels->label[k].time >= stp
		    && window->wave->labels->label[k].time < edp) {
		    if (i == 0) {
			flag = SP_TRUE;
		    } else {
			swClearLabel(window->wave->labels, k);
		    }
		}
	    }
	}

	if (flag == SP_FALSE) {
	    spDisplayError(window->window, SW_ERROR_TITLE, SW_FIND_LABEL_REGION_ERROR_MESSAGE);
	    break;
	} else if (i == 0 && swEraseLabelPrompt(window->window, window) == SP_FALSE) {
	    flag = SP_FALSE;
	    break;
	}
    }

    if (flag == SP_TRUE) {
	swSetSenseLevel(window);
	swUpdateLabels(window);
    }

    return flag;
}

spBool swSetLabelCaps(swWindow window, swLabelCaps caps)
{
    int c;
    spDialogResponse response;
    spBool clear_normal_label = SP_FALSE;
    spBool clear_region_label = SP_FALSE;
    spBool flag = SP_TRUE;

    if (swGetNumLabel(window->wave) > 0) {
	if (((caps & SW_LABEL_CAPS_MULTI_NORMAL) == 0) && swGetNumNormalLabel(window->wave) >= 2) {
	    clear_normal_label = SP_TRUE;
	}
	if (((caps & SW_LABEL_CAPS_MULTI_REGION) == 0) && swGetNumRegionLabel(window->wave) >= 2) {
	    clear_region_label = SP_TRUE;
	}
	if (clear_normal_label == SP_FALSE || clear_region_label == SP_FALSE) {
	}
	
	if (clear_normal_label == SP_FALSE && (caps & SW_LABEL_CAPS_CHANNEL_NORMAL) == 0) {
	    for (c = 0; c < window->wave->num_channel; c++) {
		if (swGetNumChannelLabel(window->wave, c, SP_FALSE) > 0) {
		    clear_normal_label = SP_TRUE;
		    break;
		}
	    }
	}
	if (clear_normal_label == SP_FALSE && (caps & SW_LABEL_CAPS_NONCHANNEL_NORMAL) == 0) {
	    if (swGetNumChannelLabel(window->wave, -1, SP_FALSE) > 0) {
		clear_normal_label = SP_TRUE;
	    }
	}
	
	if (clear_region_label == SP_FALSE && (caps & SW_LABEL_CAPS_CHANNEL_REGION) == 0) {
	    for (c = 0; c < window->wave->num_channel; c++) {
		if (swGetNumChannelLabel(window->wave, c, SP_TRUE) > 0) {
		    clear_region_label = SP_TRUE;
		    break;
		}
	    }
	}
	if (clear_region_label == SP_FALSE && (caps & SW_LABEL_CAPS_NONCHANNEL_REGION) == 0) {
	    if (swGetNumChannelLabel(window->wave, -1, SP_TRUE) > 0) {
		clear_region_label = SP_TRUE;
	    }
	}

	if (clear_normal_label == SP_TRUE && clear_region_label == SP_TRUE) {
	    response = spCreateMessageBox(window->window, SW_UNSUPPORTED_LABEL_QUESTION_TITLE,
					  SW_UNSUPPORTED_LABEL_QUESTION_MESSAGE,
					  SppDialogType, SP_WARNING_DIALOG,
					  SppMessageBoxButtonType, SP_MB_YES_NO,
					  NULL);
	    if (response == SP_DR_YES) {
		swClearLabels(window->wave->labels, SW_LABEL_TYPE_ALL);
	    } else {
		flag = SP_FALSE;
	    }
	} else if (clear_normal_label == SP_TRUE) {
	    response = spCreateMessageBox(window->window, SW_UNSUPPORTED_NORMAL_LABEL_QUESTION_TITLE,
					  SW_UNSUPPORTED_NORMAL_LABEL_QUESTION_MESSAGE,
					  SppDialogType, SP_WARNING_DIALOG,
					  SppMessageBoxButtonType, SP_MB_YES_NO,
					  NULL);
	    if (response == SP_DR_YES) {
		swClearLabels(window->wave->labels, SW_LABEL_TYPE_NORMAL);
	    } else {
		flag = SP_FALSE;
	    }
	} else if (clear_region_label == SP_TRUE) {
	    response = spCreateMessageBox(window->window, SW_UNSUPPORTED_REGION_LABEL_QUESTION_TITLE,
					  SW_UNSUPPORTED_REGION_LABEL_QUESTION_MESSAGE,
					  SppDialogType, SP_WARNING_DIALOG,
					  SppMessageBoxButtonType, SP_MB_YES_NO,
					  NULL);
	    if (response == SP_DR_YES) {
		swClearLabels(window->wave->labels, SW_LABEL_TYPE_REGION);
	    } else {
		flag = SP_FALSE;
	    }
	}
    }

    if (flag) {
	window->label_caps = caps;
    }

    return flag;
}
