/*
 *	swLabelList.c
 *
 *	Last modified: <2023-03-04 19:44:55 hideki>
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
#include "swLabelDialog.h"
#include "swLabelList.h"

struct _swLabelList {
    swWindow window;

    spBool no_callback;
    
    spComponent tab_box;

#ifdef SW_SUPPORT_INFO_AREA_SELECTION_TAB
    /* selection tab */
    spComponent selection_tab;
    spComponent selection_point_box;
    spComponent selection_start_pt_text;
    spComponent selection_end_pt_text;
    spComponent selection_length_pt_text;
    spComponent selection_play_button;
    
    spComponent selection_time_box;
    spComponent selection_start_time_text;
    spComponent selection_end_time_text;
    spComponent selection_length_time_text;

    spBool selection_updating;
#endif
    
    /* region label tab */
    spComponent region_label_tab;
    spComponent region_label_list;
    
    spComponent region_label_text_box;
    spComponent region_label_data_text;
    spComponent region_label_text;
    
    spComponent region_button_box;
    spComponent region_update_button;
    spComponent region_select_button;
    spComponent region_edit_button;
    spComponent region_erase_button;
    spComponent region_cat_button;
    
    spComponent region_button_box2;
    spComponent region_play_button;
    spComponent region_prev_button;
    spComponent region_next_button;
    
    /* normal label tab */
    spComponent normal_label_tab;
    spComponent normal_label_list;
    
    spComponent normal_label_text_box;
    spComponent normal_label_data_text;
    spComponent normal_label_text;

    spComponent normal_button_box;
    spComponent normal_update_button;
    spComponent normal_select_button;
    spComponent normal_edit_button;
    spComponent normal_erase_button;
    spComponent normal_cat_button;

    spComponent normal_button_box2;
    spComponent normal_play_button;
    spComponent normal_prev_button;
    spComponent normal_next_button;
};

void swUpdateInfoAreaSelection(swWindow window)
{
#ifdef SW_SUPPORT_INFO_AREA_SELECTION_TAB
    char *string;
    char time_str[SP_MAX_LINE];
    spLong sel_st, sel_ed;
    double sel_st_t, sel_ed_t, length_t;

    if (window->label_list == NULL
	|| spIsCreated(window->label_list->selection_tab) == SP_FALSE) return;

    window->label_list->selection_updating = SP_TRUE;
    
    spDebug(50, "swUpdateInfoAreaSelection", "sel_st = %ld, sel_ed = %ld\n");

    if (window->sel_st >= 0 && window->sel_ed >= 0) {
	sel_st = window->sel_st;
	sel_ed = window->sel_ed;

	/* to keep start time even if selection is released */
	sprintf(time_str, "%ld", (long)sel_st);
	spSetTextString(window->label_list->selection_start_pt_text, time_str);
    } else {
	sel_st = 0;
	sel_ed = 0;
	
	if ((string = xspGetTextString(window->label_list->selection_start_pt_text)) != NULL) {
	    sel_st = atol(string);
	    sel_ed = sel_st;
	    xfree(string);
	} else {
	    spSetTextString(window->label_list->selection_start_pt_text, "0");
	}
    }
    
    sprintf(time_str, "%ld", (long)sel_ed);
    spSetTextString(window->label_list->selection_end_pt_text, time_str);

    sprintf(time_str, "%ld", (long)(sel_ed - sel_st));
    spSetTextString(window->label_list->selection_length_pt_text, time_str);

    sel_st_t = swSampToDim(window, sel_st);
    swGetDrawTimeString(window, sel_st_t, time_str, SP_TRUE);
    spSetTextString(window->label_list->selection_start_time_text, time_str);
    
    sel_ed_t = swSampToDim(window, sel_ed);
    swGetDrawTimeString(window, sel_ed_t, time_str, SP_TRUE);
    spSetTextString(window->label_list->selection_end_time_text, time_str);
    
    length_t = swSampToDim(window, sel_ed - sel_st);
    swGetDrawTimeString(window, length_t, time_str, SP_TRUE);
    spSetTextString(window->label_list->selection_length_time_text, time_str);

    window->label_list->selection_updating = SP_FALSE;
    
#endif
    return;
}

#ifdef SW_SUPPORT_INFO_AREA_SELECTION_TAB
void swSelectionTabPlayCB(spComponent component, swWindow window)
{
    if (swIsNoWave(window) == SP_TRUE) return;

    if (window->sel_st >= 0 && window->sel_ed - window->sel_st > 0) {
	if (swPlayStop(window) == SP_FALSE) {
	    swPlayRegion(window, window->sel_st, window->sel_ed);
	}
    }
}

void swSelectionTabTextCB(spComponent component, swLabelList label_list)
{
    char *string;
    const char *name;
    long point;
    swWindow window;
    
    spDebug(50, "swSelectionTabTextCB", "in\n");
    
    if (label_list == NULL || label_list->window == NULL
	|| label_list->window->wave == NULL || label_list->selection_updating == SP_TRUE) {
	spDebug(50, "swSelectionTabTextCB", "process not required\n");
	return;
    }

    if ((string = xspGetTextString(component)) != NULL) {
	name = spGetName(component);
	spDebug(50, "swSelectionTabTextCB", "name = %s, string = %s\n", name, string);

	point = atol(string);
	xfree(string);

	window = label_list->window;
	
	if (streq(name, "selectionStartPointText")) {
	    if (point >= 0 && point < window->sel_ed) {
		swSelectRegion(window, window->wave->selected_channel,
			       point, window->sel_ed);
	    }
	} else if (streq(name, "selectionEndPointText")) {
	    if (point >= 1 && point > window->sel_st) {
		swSelectRegion(window, window->wave->selected_channel,
			       window->sel_st, point);
	    }
	} else if (streq(name, "selectionLengthPointText")) {
	    if (point > 0) {
		swSelectRegion(window, window->wave->selected_channel,
			       window->sel_st, window->sel_st + point);
	    }
	}
    }
    
    return;
}

static void createSelectionTab(swLabelList label_list)
{
    int text_height;

    label_list->selection_updating = SP_FALSE;
    
    text_height = spGetTextFieldDefaultHeight(SP_FALSE) + 6;
    
    /* create tab for selection */
    label_list->selection_tab = spAddTabItem(label_list->tab_box, "selectionTab", -1,
					     SppTitle, SW_SELECTION_TAB_TITLE,
					     SppSenseLevel, SW_STATE_EXIST_WAVE,
					     NULL);
    label_list->selection_point_box = spCreateBox(label_list->selection_tab, "selectionPointBox", 4 * text_height,
						  SppTitle, SW_SELECTION_TAB_POINT_BOX_TITLE,
						  SppTitleOn, SP_TRUE,
						  SppBorderOn, SP_TRUE,
						  SppSpacingOn, SP_TRUE,
						  NULL);
    label_list->selection_start_pt_text = spCreateParamField(label_list->selection_point_box,
							     "selectionStartPointText", 0,
							     SppTitle, SW_SELECTION_TAB_START_LABEL,
							     SppUseTextHeight, SP_TRUE,
							     SppFieldType, SP_FIELD_TYPE_TEXT,
							     SppEditable, SP_TRUE,
							     SppDisableShortcut, SP_TRUE,
							     SppFieldOffset, 80,
							     SppFieldSize, 160,
							     SppMarginHeight, 3,
							     SppCallbackFunc, swSelectionTabTextCB,
							     SppCallbackData, label_list,
							     NULL);
    label_list->selection_end_pt_text = spCreateParamField(label_list->selection_point_box,
							   "selectionEndPointText", 0,
							   SppTitle, SW_SELECTION_TAB_END_LABEL,
							   SppUseTextHeight, SP_TRUE,
							   SppFieldType, SP_FIELD_TYPE_TEXT,
							   SppEditable, SP_TRUE,
							   SppDisableShortcut, SP_TRUE,
							   SppFieldOffset, 80,
							   SppFieldSize, 160,
							   SppMarginHeight, 3,
							   SppCallbackFunc, swSelectionTabTextCB,
							   SppCallbackData, label_list,
							   NULL);
    label_list->selection_length_pt_text = spCreateParamField(label_list->selection_point_box,
							      "selectionLengthPointText", 0,
							      SppTitle, SW_SELECTION_TAB_LENGTH_LABEL,
							      SppUseTextHeight, SP_TRUE,
							      SppFieldType, SP_FIELD_TYPE_TEXT,
							      SppEditable, SP_TRUE,
							      SppDisableShortcut, SP_TRUE,
							      SppFieldOffset, 80,
							      SppFieldSize, 160,
							      SppMarginHeight, 3,
							      SppCallbackFunc, swSelectionTabTextCB,
							      SppCallbackData, label_list,
							      NULL);
    label_list->selection_play_button = spCreatePushButton(label_list->selection_point_box, "selectionPlayButton",
							   SppTitle, SW_SELECTION_TAB_PLAY_LABEL,
							   SppWidth, 60,
							   SppCallbackFunc, swSelectionTabPlayCB,
							   SppCallbackData, label_list->window,
							   SppSenseLevel, SW_STATE_SELECT_TIME_DATA,
							   SppMarginHeight, 3,
							   NULL);

    label_list->selection_time_box = spCreateBox(label_list->selection_tab, "selectionTimeBox", /*3 * text_height*/0,
						 SppTitle, SW_SELECTION_TAB_TIME_BOX_TITLE,
						 SppTitleOn, SP_TRUE,
						 SppBorderOn, SP_TRUE,
						 SppSpacingOn, SP_TRUE,
						 NULL);
    label_list->selection_start_time_text = spCreateParamField(label_list->selection_time_box,
							       "selectionStartTimeText", 0,
							       SppTitle, SW_SELECTION_TAB_START_LABEL,
							       SppUseTextHeight, SP_TRUE,
							       SppFieldType, SP_FIELD_TYPE_TEXT,
							       SppEditable, SP_FALSE,
							       SppDisableShortcut, SP_TRUE,
							       SppFieldOffset, 80,
							       SppFieldSize, 160,
							       SppMarginHeight, 3,
							       NULL);
    label_list->selection_end_time_text = spCreateParamField(label_list->selection_time_box,
							     "selectionEndTimeText", 0,
							     SppTitle, SW_SELECTION_TAB_END_LABEL,
							     SppUseTextHeight, SP_TRUE,
							     SppFieldType, SP_FIELD_TYPE_TEXT,
							     SppEditable, SP_FALSE,
							     SppDisableShortcut, SP_TRUE,
							     SppFieldOffset, 80,
							     SppFieldSize, 160,
							     SppMarginHeight, 3,
							     NULL);
    label_list->selection_length_time_text = spCreateParamField(label_list->selection_time_box,
								"selectionLengthTimeText", 0,
								SppTitle, SW_SELECTION_TAB_LENGTH_LABEL,
								SppUseTextHeight, SP_TRUE,
								SppFieldType, SP_FIELD_TYPE_TEXT,
								SppEditable, SP_FALSE,
								SppDisableShortcut, SP_TRUE,
								SppFieldOffset, 80,
								SppFieldSize, 160,
								SppMarginHeight, 3,
								NULL);
    
    return;
}
#endif

static long getListIndex(swWindow window, long label_index, spBool *is_region_label)
{
    long k;
    long list_index = 0;
    spBool region_label_flag = SP_FALSE;

    if (label_index < 0) return -1;

    if (window->wave->labels->label[label_index].end_time >= 0.0) {
	region_label_flag = SP_TRUE;
    }

    for (k = 0; k < window->wave->labels->num_label; k++) {
	if (k == label_index) {
	    if (is_region_label != NULL) *is_region_label = region_label_flag;
	    return list_index;
	}
	if (window->wave->labels->label[k].time >= 0.0) {
	    if (region_label_flag == SP_TRUE) {
		if (window->wave->labels->label[k].end_time >= 0.0) {
		    if (k == label_index) {
			break;
		    }
		    list_index++;
		}
	    } else {
		if (window->wave->labels->label[k].end_time < 0.0) {
		    if (k == label_index) {
			break;
		    }
		    list_index++;
		}
	    }
	}
    }
    
    if (k == label_index) {
	if (is_region_label != NULL) *is_region_label = region_label_flag;
	return list_index;
    }

    return -1;
}

static long getLabelIndex(swWindow window, long list_index, spBool is_region_label)
{
    long k;
    long current_list_index = 0;

    if (list_index < 0) return -1;

    for (k = 0; k < window->wave->labels->num_label; k++) {
	if (window->wave->labels->label[k].time >= 0.0) {
	    if (is_region_label == SP_TRUE) {
		if (window->wave->labels->label[k].end_time >= 0.0) {
		    if (list_index == current_list_index) {
			return k;
		    }
		    current_list_index++;
		}
	    } else {
		if (window->wave->labels->label[k].end_time < 0.0) {
		    if (list_index == current_list_index) {
			return k;
		    }
		    current_list_index++;
		}
	    }
	}
    }
    
    return -1;
}

static void updateLabelListText(swLabelList label_list, long label_index, spBool is_region_label)
{
    if (label_index < 0 || label_list->window->wave == NULL) {
	if (is_region_label == SP_TRUE) {
	    spDebug(50, "updateLabelListText", "insensitive region buttons\n");
	    spSetTextString(label_list->region_label_data_text, "");
	    spSetSensitive(label_list->region_label_data_text, SP_FALSE);
	    spSetTextString(label_list->region_label_text, "");
	    spSetSensitive(label_list->region_label_text, SP_FALSE);
	    spSetSensitive(label_list->region_button_box, SP_FALSE);
	} else {
	    spDebug(50, "updateLabelListText", "insensitive normal buttons\n");
	    spSetTextString(label_list->normal_label_data_text, "");
	    spSetSensitive(label_list->normal_label_data_text, SP_FALSE);
	    spSetTextString(label_list->normal_label_text, "");
	    spSetSensitive(label_list->normal_label_text, SP_FALSE);
	    spSetSensitive(label_list->normal_button_box, SP_FALSE);
	}
    } else if (label_list->window->wave->labels->label[label_index].time >= 0.0) {
	if (is_region_label == SP_TRUE) {
	    spSetSensitive(label_list->region_button_box, SP_TRUE);
	    spSetSensitive(label_list->region_label_data_text, SP_TRUE);
	    spSetTextString(label_list->region_label_data_text,
			    label_list->window->wave->labels->label[label_index].data);
	    spSetSensitive(label_list->region_label_text, SP_TRUE);
	    spSetTextString(label_list->region_label_text,
			    label_list->window->wave->labels->label[label_index].string);
	    
	    spSetTextString(label_list->normal_label_data_text, "");
	    spSetSensitive(label_list->normal_label_data_text, SP_FALSE);
	    spSetTextString(label_list->normal_label_text, "");
	    spSetSensitive(label_list->normal_label_text, SP_FALSE);
	    spSetSensitive(label_list->normal_button_box, SP_FALSE);
	} else {
	    spSetSensitive(label_list->normal_button_box, SP_TRUE);
	    spSetSensitive(label_list->normal_label_data_text, SP_TRUE);
	    spSetTextString(label_list->normal_label_data_text,
			    label_list->window->wave->labels->label[label_index].data);
	    spSetSensitive(label_list->normal_label_text, SP_TRUE);
	    spSetTextString(label_list->normal_label_text,
			    label_list->window->wave->labels->label[label_index].string);
	    
	    spSetTextString(label_list->region_label_data_text, "");
	    spSetSensitive(label_list->region_label_data_text, SP_FALSE);
	    spSetTextString(label_list->region_label_text, "");
	    spSetSensitive(label_list->region_label_text, SP_FALSE);
	    spSetSensitive(label_list->region_button_box, SP_FALSE);
	}

	if (label_list->window->config->link_region_label == SP_TRUE) {
	    swSetLabelAsRegion(label_list->window, label_index);
	}
    }

    return;
}

spBool swPlayActiveLabelRegion(swLabelList label_list)
{
    if (label_list->window->active_label_index >= 0) {
	if (swPlayStop(label_list->window) == SP_FALSE) {
	    spLong st, ed;
	    int channel;
	    
	    if (swGetLabelEdge(label_list->window,
			       label_list->window->active_label_index, &channel, &st, &ed) == SP_TRUE) {
		return swPlayRegion(label_list->window, st, ed);
	    }
	}
    }

    return SP_FALSE;
}

static void selectLabelListCB(spComponent component, swLabelList label_list, spBool is_region_label)
{
    long index;
    
    if (label_list == NULL || swIsNoWave(label_list->window) == SP_TRUE
	|| label_list->window->wave->labels == NULL
	|| label_list->no_callback == SP_TRUE) return;

    if ((index = getLabelIndex(label_list->window,
			       spGetSelectedListIndex(component), is_region_label)) >= 0
	&& index != label_list->window->active_label_index) {
	label_list->window->active_label_index = index;
	updateLabelListText(label_list, index, is_region_label);
	
	{
	    spLong timel;
	    spLong offset;

	    timel = swDimToSamp(label_list->window,
				label_list->window->wave->labels->label[label_list->window->active_label_index].time);

	    offset = timel - label_list->window->length / 2;

	    swScrollWindow(label_list->window, offset, SP_FALSE, SP_FALSE);
	    swMoveCursor(label_list->window, timel, SP_TRUE);
	}
    } else {
	updateLabelListText(label_list, index, is_region_label);
    }
	
    spDebug(80, "selectLabelList", "index = %ld, activated label = %ld\n",
	    index, label_list->window->active_label_index);
    
    if (spGetCallbackReason(component) == SP_CR_ACTIVATE) {
	swPlayActiveLabelRegion(label_list);
    }
    
    return;
}

static void regionLabelListCB(spComponent component, swLabelList label_list)
{
    selectLabelListCB(component, label_list, SP_TRUE);
    return;
}

static void normalLabelListCB(spComponent component, swLabelList label_list)
{
    selectLabelListCB(component, label_list, SP_FALSE);
    
    return;
}

static void selectPrevNextLabelItem(swLabelList label_list, spBool prev_flag, spBool is_region_label)
{
    int index;
    spComponent list;
    
    if (is_region_label == SP_TRUE) {
	list = label_list->region_label_list;
    } else {
	list = label_list->normal_label_list;
    }
    
    if ((index = spGetSelectedListIndex(list)) >= 0) {
	if (prev_flag == SP_TRUE) {
	    index--;
	} else {
	    index++;
	}
	spSelectListIndex(list, index);
	
	if (list == label_list->region_label_list) {
	    regionLabelListCB(list, label_list);
	} else {
	    normalLabelListCB(list, label_list);
	}
    }

    return;
}

static void activateTextCB(spComponent component, swLabelList label_list)
{
    char *string;
    spBool is_region_label = SP_FALSE;
    
    if (label_list->window == NULL || label_list->window->active_label_index < 0)
	return;
    
    if ((string = xspGetTextString(component)) != NULL) {
	spDebug(50, "activateTextCB", "string = %s\n", string);
	if (streq(spGetName(component), "regionLabelDataText")
	    || streq(spGetName(component), "normalLabelDataText")) {
	    spStrCopy(label_list->window->wave->labels->label[label_list->window->active_label_index].data,
		      SW_MAX_LABEL_STRING, string);
	} else {
	    spStrCopy(label_list->window->wave->labels->label[label_list->window->active_label_index].string,
		      SW_MAX_LABEL_STRING, string);
	}
	swUpdateLabels(label_list->window);

	if (strveq(spGetName(component), "regionLabel")) {
	    is_region_label = SP_TRUE;
	}
	selectPrevNextLabelItem(label_list, SP_FALSE, is_region_label);
	
	xfree(string);
    }
    
    return;
}

static void buttonCB(spComponent component, swLabelList label_list, spBool is_region_label)
{
    long index;
    char *string;
    spComponent list, text, data_text;
    swWindow window;
    
    if (label_list->window == NULL) return;

    window = label_list->window;

    if (is_region_label == SP_TRUE) {
	list = label_list->region_label_list;
	text = label_list->region_label_text;
	data_text = label_list->region_label_data_text;
    } else {
	list = label_list->normal_label_list;
	text = label_list->normal_label_text;
	data_text = label_list->normal_label_data_text;
    }
    
    if ((index = getLabelIndex(window,
			       spGetSelectedListIndex(list), is_region_label)) >= 0) {
	if (streq(spGetName(component), "updateButton")) {
	    if ((string = xspGetTextString(data_text)) != NULL) {
		spDebug(50, "buttonCB", "string = %s\n", string);
		spStrCopy(window->wave->labels->label[index].data,
			  SW_MAX_LABEL_STRING, string);
		xfree(string);
	    } else {
		index = -1;
	    }
	    if ((string = xspGetTextString(text)) != NULL) {
		spDebug(50, "buttonCB", "string = %s\n", string);
		spStrCopy(window->wave->labels->label[index].string,
			  SW_MAX_LABEL_STRING, string);
		xfree(string);
	    } else {
		index = -1;
	    }
	    
	    if (index >= 0) {
		swUpdateLabels(window);
	    }
	} else if (streq(spGetName(component), "selectButton")) {
	    swSetLabelAsRegion(window, index);
	} else if (streq(spGetName(component), "editButton")) {
	    swPopupLabelInsertDialog(window, index);
	} else if (streq(spGetName(component), "eraseButton")) {
	    swEraseLabelIndex(component, window, index);
	} else if (streq(spGetName(component), "catButton")) {
	    swCatRegionLabels(window, index, SP_FALSE);
	} else if (streq(spGetName(component), "playButton")) {
	    swPlayActiveLabelRegion(label_list);
	} else if (streq(spGetName(component), "prevButton")) {
	    selectPrevNextLabelItem(label_list, SP_TRUE, is_region_label);
	} else if (streq(spGetName(component), "nextButton")) {
	    selectPrevNextLabelItem(label_list, SP_FALSE, is_region_label);
	}
    }
    
    return;
}

static void regionButtonCB(spComponent component, swLabelList label_list)
{
    buttonCB(component, label_list, SP_TRUE);
    return;
}

static void normalButtonCB(spComponent component, swLabelList label_list)
{
    buttonCB(component, label_list, SP_FALSE);
    return;
}

static void createRegionLabelTab(swLabelList label_list)
{
    int field_height;

    field_height = spGetTextFieldDefaultHeight(SP_TRUE);
    
    /* create tab for region label */
    label_list->region_label_tab = spAddTabItem(label_list->tab_box, "regionLabelTab", -1,
						SppTitle, SW_REGION_LABEL_LABEL,
						SppGroupId, SW_REGION_LABEL_GROUP_ID,
						SppSenseLevel, SW_STATE_EXIST_LABEL,
						NULL);
    label_list->region_label_list = spCreateList(label_list->region_label_tab, "regionLabelList",
						 SppInitialHeight, 10,
						 SppHeight, -field_height * 3,
						 SppSpacingOn, SP_TRUE,
						 SppCallbackFunc, regionLabelListCB,
						 SppCallbackData, label_list,
						 NULL);

    label_list->region_label_text_box = spCreateBox(label_list->region_label_tab, "regionLabelTextBox", 0,
						    SppOrientation, SP_HORIZONTAL,
						    SppUseTextHeight, SP_TRUE,
						    NULL);
    label_list->region_label_data_text = spCreateTextField(label_list->region_label_text_box, "regionLabelDataText",
							   SppWidth, 80,
							   SppDisableShortcut, SP_TRUE,
							   NULL);
    label_list->region_label_text = spCreateTextField(label_list->region_label_text_box, "regionLabelText",
						      SppWidth, -1,
						      SppDisableShortcut, SP_TRUE,
						      NULL);
    spAddCallback(label_list->region_label_data_text, SP_ACTIVATE_CALLBACK,
		  (spCallbackFunc)activateTextCB, label_list);
    spAddCallback(label_list->region_label_text, SP_ACTIVATE_CALLBACK,
		  (spCallbackFunc)activateTextCB, label_list);
    
    /* create container for buttons */
    label_list->region_button_box = spCreateBox(label_list->region_label_tab, "regionButtonBox", 0,
						SppOrientation, SP_HORIZONTAL,
						SppUseTextHeight, SP_TRUE,
						SppSpacingOn, SP_TRUE,
						SppSpacing, 0,
						SppMarginHeight, 0,
						SppGroupId, SW_LABEL_LIST_GROUP_ID,
						NULL);
    label_list->region_update_button = spCreatePushButton(label_list->region_button_box, "updateButton",
							  SppTitle, SW_UPDATE_LABEL_LABEL,
							  SppWidth, 60,
							  SppCallbackFunc, regionButtonCB,
							  SppCallbackData, label_list,
							  SppGroupId, SW_LABEL_LIST_GROUP_ID,
							  NULL);
    label_list->region_select_button = spCreatePushButton(label_list->region_button_box, "selectButton",
							  SppTitle, SW_SELECT_LABEL_LABEL,
							  SppWidth, 60,
							  SppCallbackFunc, regionButtonCB,
							  SppCallbackData, label_list,
							  SppGroupId, SW_LABEL_LIST_GROUP_ID,
							  NULL);
    label_list->region_edit_button = spCreatePushButton(label_list->region_button_box, "editButton",
							SppTitle, SW_EDIT_LABEL_LABEL,
							SppWidth, 60,
							SppCallbackFunc, regionButtonCB,
							SppCallbackData, label_list,
							SppGroupId, SW_LABEL_LIST_GROUP_ID,
							NULL);
    label_list->region_erase_button = spCreatePushButton(label_list->region_button_box, "eraseButton",
							 SppTitle, SW_ERASE_LABEL_LABEL,
							 SppWidth, 60,
							 SppCallbackFunc, regionButtonCB,
							 SppCallbackData, label_list,
							 SppGroupId, SW_LABEL_LIST_GROUP_ID,
							 NULL);
    label_list->region_cat_button = spCreatePushButton(label_list->region_button_box, "catButton",
						       SppTitle, SW_CAT_LABEL_LABEL,
						       SppWidth, 60,
						       SppCallbackFunc, regionButtonCB,
						       SppCallbackData, label_list,
						       SppGroupId, SW_LABEL_LIST_GROUP_ID,
						       NULL);

    label_list->region_button_box2 = spCreateBox(label_list->region_label_tab, "regionButtonBox2", 0,
						 SppOrientation, SP_HORIZONTAL,
						 SppUseTextHeight, SP_TRUE,
						 SppSpacingOn, SP_TRUE,
						 SppSpacing, 0,
						 SppMarginHeight, 0,
						 SppGroupId, SW_LABEL_LIST_GROUP_ID,
						 NULL);
    label_list->region_play_button = spCreatePushButton(label_list->region_button_box2, "playButton",
							SppTitle, SW_PLAY_LABEL_LABEL,
							SppWidth, 60,
							SppCallbackFunc, regionButtonCB,
							SppCallbackData, label_list,
							SppGroupId, SW_LABEL_LIST_GROUP_ID,
							NULL);
    label_list->region_prev_button = spCreatePushButton(label_list->region_button_box2, "prevButton",
							SppTitle, SW_PREV_LABEL_LABEL,
							SppWidth, 60,
							SppCallbackFunc, regionButtonCB,
							SppCallbackData, label_list,
							SppGroupId, SW_LABEL_LIST_GROUP_ID,
							NULL);
    label_list->region_next_button = spCreatePushButton(label_list->region_button_box2, "nextButton",
							SppTitle, SW_NEXT_LABEL_LABEL,
							SppWidth, 60,
							SppCallbackFunc, regionButtonCB,
							SppCallbackData, label_list,
							SppGroupId, SW_LABEL_LIST_GROUP_ID,
							NULL);
    
    return;
}

static void createNormalLabelTab(swLabelList label_list)
{
    int field_height;

    field_height = spGetTextFieldDefaultHeight(SP_TRUE);
    
    /* create tab for normal label */
    label_list->normal_label_tab = spAddTabItem(label_list->tab_box, "normalLabelTab", -1,
						SppTitle, SW_NORMAL_LABEL_LABEL,
						SppGroupId, SW_NORMAL_LABEL_GROUP_ID,
						SppSenseLevel, SW_STATE_EXIST_LABEL,
						NULL);
    label_list->normal_label_list = spCreateList(label_list->normal_label_tab, "normalLabelList",
						 SppInitialHeight, 10,
						 SppHeight, -field_height * 3,
						 SppSpacingOn, SP_TRUE,
						 SppCallbackFunc, normalLabelListCB,
						 SppCallbackData, label_list,
						 NULL);

    label_list->normal_label_text_box = spCreateBox(label_list->normal_label_tab, "normalLabelTextBox", 0,
						    SppOrientation, SP_HORIZONTAL,
						    SppUseTextHeight, SP_TRUE,
						    NULL);
    label_list->normal_label_data_text = spCreateTextField(label_list->normal_label_text_box, "normalLabelDataText",
							   SppWidth, 80,
							   SppDisableShortcut, SP_TRUE,
							   NULL);
    label_list->normal_label_text = spCreateTextField(label_list->normal_label_text_box, "normalLabelText",
						      SppWidth, -1,
						      SppDisableShortcut, SP_TRUE,
						      NULL);
    spAddCallback(label_list->normal_label_data_text, SP_ACTIVATE_CALLBACK,
		  (spCallbackFunc)activateTextCB, label_list);
    spAddCallback(label_list->normal_label_text, SP_ACTIVATE_CALLBACK,
		  (spCallbackFunc)activateTextCB, label_list);

    /* create container for buttons */
    label_list->normal_button_box = spCreateBox(label_list->normal_label_tab, "normalButtonBox", 0,
						SppOrientation, SP_HORIZONTAL,
						SppUseTextHeight, SP_TRUE,
						SppSpacingOn, SP_TRUE,
						SppSpacing, 0,
						SppMarginHeight, 0,
						SppGroupId, SW_LABEL_LIST_GROUP_ID,
						NULL);
    label_list->normal_update_button = spCreatePushButton(label_list->normal_button_box, "updateButton",
							  SppTitle, SW_UPDATE_LABEL_LABEL,
							  SppWidth, 60,
							  SppCallbackFunc, normalButtonCB,
							  SppCallbackData, label_list,
							  SppGroupId, SW_LABEL_LIST_GROUP_ID,
							  NULL);
    label_list->normal_select_button = spCreatePushButton(label_list->normal_button_box, "selectButton",
							  SppTitle, SW_SELECT_LABEL_LABEL,
							  SppWidth, 60,
							  SppCallbackFunc, normalButtonCB,
							  SppCallbackData, label_list,
							  SppGroupId, SW_LABEL_LIST_GROUP_ID,
							  NULL);
    label_list->normal_edit_button = spCreatePushButton(label_list->normal_button_box, "editButton",
							SppTitle, SW_EDIT_LABEL_LABEL,
							SppWidth, 60,
							SppCallbackFunc, normalButtonCB,
							SppCallbackData, label_list,
							SppGroupId, SW_LABEL_LIST_GROUP_ID,
							NULL);
    label_list->normal_erase_button = spCreatePushButton(label_list->normal_button_box, "eraseButton",
							 SppTitle, SW_ERASE_LABEL_LABEL,
							 SppWidth, 60,
							 SppCallbackFunc, normalButtonCB,
							 SppCallbackData, label_list,
							 SppGroupId, SW_LABEL_LIST_GROUP_ID,
							 NULL);
    label_list->normal_cat_button = spCreatePushButton(label_list->normal_button_box, "catButton",
						       SppTitle, SW_CAT_LABEL_LABEL,
						       SppWidth, 60,
						       SppCallbackFunc, normalButtonCB,
						       SppCallbackData, label_list,
						       SppGroupId, SW_LABEL_LIST_GROUP_ID,
						       NULL);
    
    label_list->normal_button_box2 = spCreateBox(label_list->normal_label_tab, "normalButtonBox2", 0,
						 SppOrientation, SP_HORIZONTAL,
						 SppUseTextHeight, SP_TRUE,
						 SppSpacingOn, SP_TRUE,
						 SppSpacing, 0,
						 SppMarginHeight, 0,
						 SppGroupId, SW_LABEL_LIST_GROUP_ID,
						 NULL);
    label_list->normal_play_button = spCreatePushButton(label_list->normal_button_box2, "playButton",
							SppTitle, SW_PLAY_LABEL_LABEL,
							SppWidth, 60,
							SppCallbackFunc, normalButtonCB,
							SppCallbackData, label_list,
							SppGroupId, SW_LABEL_LIST_GROUP_ID,
							NULL);
    label_list->normal_prev_button = spCreatePushButton(label_list->normal_button_box2, "prevButton",
							SppTitle, SW_PREV_LABEL_LABEL,
							SppWidth, 60,
							SppCallbackFunc, normalButtonCB,
							SppCallbackData, label_list,
							SppGroupId, SW_LABEL_LIST_GROUP_ID,
							NULL);
    label_list->normal_next_button = spCreatePushButton(label_list->normal_button_box2, "nextButton",
							SppTitle, SW_NEXT_LABEL_LABEL,
							SppWidth, 60,
							SppCallbackFunc, normalButtonCB,
							SppCallbackData, label_list,
							SppGroupId, SW_LABEL_LIST_GROUP_ID,
							NULL);
    
    return;
}

swLabelList swCreateLabelList(spComponent parent, swWindow window)
{
    swLabelList label_list;

    if (spIsCreated(parent) == SP_FALSE) return NULL;
    
    label_list = xalloc(1, struct _swLabelList);
    memset(label_list, 0, sizeof(struct _swLabelList));
    label_list->window = window;
    label_list->no_callback = SP_FALSE;

    /* create tab box */
    label_list->tab_box = spCreateTabBox(parent, "labelListTabBox", 0,
					 NULL);

#ifdef SW_SUPPORT_INFO_AREA_SELECTION_TAB
    createSelectionTab(label_list);
#endif
    createRegionLabelTab(label_list);
    createNormalLabelTab(label_list);

    swUpdateLabelList(label_list);
    swSetWindowSenseLevel(label_list->window, SP_FALSE);
    
    return label_list;
}


spBool swSelectLabelList(swLabelList label_list)
{
    swWindow window;
    long index = -1;
    spBool is_region_label = SP_FALSE;

    if (label_list == NULL || label_list->window == NULL
	|| label_list->window->wave == NULL || label_list->window->wave->labels == NULL)
	return SP_FALSE;

    window = label_list->window;
    
    if (window->active_label_index >= 0
	&& (index = getListIndex(window, window->active_label_index, &is_region_label)) >= 0) {
	spDebug(50, "swSelectLabelList", "is_region_label = %d, index = %ld\n",
		is_region_label, index);
	if (is_region_label == SP_TRUE) {
	    index = spSelectListIndex(label_list->region_label_list, index);
	} else {
	    index = spSelectListIndex(label_list->normal_label_list, index);
	}
	updateLabelListText(label_list, window->active_label_index, is_region_label);
    }

    if (index < 0) {
	return SP_FALSE;
    } else {
	return SP_TRUE;
    }
}

spBool swUpdateLabelList(swLabelList label_list)
{
    long k;
    long region_label_index = 0;
    long normal_label_index = 0;
    char time_buf[SP_MAX_LINE];
    char end_time_buf[SP_MAX_LINE];
    char buf[SP_MAX_MESSAGE];
    char channel_buf[SP_MAX_LINE];
    swWindow window;
    
    if (label_list == NULL || label_list->window == NULL
	|| spIsVisible(label_list->tab_box) == SP_FALSE)
	return SP_FALSE;

    spDebug(50, "swUpdateLabelList", "in\n");

    window = label_list->window;
    label_list->no_callback = SP_TRUE;
    
    if (label_list->window->wave != NULL && label_list->window->wave->labels != NULL) {
	for (k = 0; k < window->wave->labels->num_label; k++) {
	    if (window->wave->labels->label[k].channel >= 0) {
		sprintf(channel_buf, "%d/", window->wave->labels->label[k].channel + 1);
	    } else {
		channel_buf[0] = NUL;
	    }
	    
	    if (window->wave->labels->label[k].time >= 0.0) {
		if (window->wave->labels->label[k].end_time >= 0.0) {
		    swGetDrawTimeString(window, window->wave->labels->label[k].time, time_buf, SP_FALSE);
		    swGetDrawTimeString(window, window->wave->labels->label[k].end_time, end_time_buf, SP_TRUE);
		    sprintf(buf, "%s%s : %s  %4s  %s",
			    channel_buf, time_buf, end_time_buf,
			    window->wave->labels->label[k].data,
			    window->wave->labels->label[k].string);

		    spAddListIndex(label_list->region_label_list, buf, region_label_index);
		    region_label_index++;
		
		    spDeleteListIndex(label_list->region_label_list, region_label_index);
		} else {
		    swGetDrawTimeString(window, window->wave->labels->label[k].time, time_buf, SP_FALSE);
		    sprintf(buf, "%s%s  %4s  %s", channel_buf, time_buf,
			    window->wave->labels->label[k].data,
			    window->wave->labels->label[k].string);

		    spAddListIndex(label_list->normal_label_list, buf, normal_label_index);
		    normal_label_index++;
		
		    spDeleteListIndex(label_list->normal_label_list, normal_label_index);
		}
	    }
	}
	spDebug(50, "swUpdateLabelList", "add new list done\n");
    }

    while (1) {
	spDebug(100, "swUpdateLabelList", "delete region label list\n");
	if (spDeleteListIndex(label_list->region_label_list, region_label_index) < 0) {
	    break;
	}
    }
    while (1) {
	spDebug(100, "swUpdateLabelList", "delete normal label list\n");
	if (spDeleteListIndex(label_list->normal_label_list, normal_label_index) < 0) {
	    break;
	}
    }
    spDebug(50, "swUpdateLabelList", "delete list done\n");

    if (swSelectLabelList(label_list) == SP_FALSE) {
	long index;
	
	if ((index = spGetSelectedListIndex(label_list->region_label_list)) < 0) {
	    updateLabelListText(label_list, -1, SP_TRUE);
	}
	if ((index = spGetSelectedListIndex(label_list->normal_label_list)) < 0) {
	    updateLabelListText(label_list, -1, SP_FALSE);
	}
    }

    swUpdateInfoAreaSelection(window);
    
    label_list->no_callback = SP_FALSE;

    spDebug(50, "swUpdateLabelList", "done\n");

    return SP_TRUE;
}

void swUpdateAllLabelList(swWindow window)
{
    spComponent next = NULL;
    swWindow next_window = NULL;
    
    if (window == NULL) return;

    spDebug(50, "swUpdateAllLabelList", "in\n");
    
    swUpdateLabelList(window->label_list);
    
    next = window->window;
    while (1) {
	next = spGetNextWindow(next, SP_FALSE);
	if (next == NULL || next == window->window) {
	    break;
	}

	if ((next_window = (swWindow)spGetUserData(next)) != NULL) {
	    swUpdateLabelList(next_window->label_list);
	}
    }

    return;
}

