/*
 *	swEdit.c
 *
 *	Last modified: <2025-03-24 19:07:25 hideki>
 */

#include <stdio.h>
#include <stdlib.h>

#include <sp/spBaseLib.h>
#include <sp/spComponentLib.h>

#include "swWindow.h"
#include "swDraw.h"
#include "swCursor.h"
#include "swLabelDialog.h"
#include "swWave.h"
#include "swAnalysis.h"

spBool swGetEdge(swWindow window, spLong st, spLong ed, spLong *offset, spLong *length)
{
    if (swIsNoWave(window) == SP_TRUE ||
	(offset == NULL && length == NULL)) return SP_FALSE;
    
    spDebug(30, "swGetEdge", "in\n");
    
    /* get edge */
    if ((st < 0 && ed < 0) || st == ed) {
	return SP_FALSE;
    } else if (st >= window->wave->total_length && ed >= window->wave->total_length) {
	return SP_FALSE;
    } else if (st < ed) {
	st = MAX(st, 0);
	st = MIN(st, window->wave->total_length - 1);
	ed = MAX(ed, st + 1);
	ed = MIN(ed, window->wave->total_length - 1);

	if (offset != NULL) *offset = st;
	if (length != NULL) *length = ed - st + 1;
    } else {
	ed = MAX(ed, 0);
	ed = MIN(ed, window->wave->total_length - 1);
	st = MAX(st, ed + 1);
	st = MIN(st, window->wave->total_length - 1);

	if (offset != NULL) *offset = ed;
	if (length != NULL) *length = st - ed + 1;
    }

    spDebug(10, "swGetEdge", "st = %ld, ed = %ld\n", st, ed);
    
    return SP_TRUE;
}

#ifdef SW_SUPPORT_EDIT
swWindow swExtractAndSaveWindowAt(swWindow window, char *filename, char *plugin_name, char *file_type, char *file_desc,
				  spLong st, spLong ed, spBool autosave_flag, int x, int y)
{
    spLong offset;
    spLong length;
    swWave wave;
    swWindow new_window;

    if (swIsNoWave(window) == SP_TRUE) return NULL;

    /* get edge */
    if (swGetEdge(window, st, ed, &offset, &length) != SP_TRUE) {
	spDisplayError(window->window, SW_ERROR_TITLE, SW_REGION_ERROR_MESSAGE);
	return NULL;
    }

#ifdef SW_SUPPORT_AUTOSAVE
    if (autosave_flag == SP_TRUE) {
	if (swGetNumRegionLabel(window->wave) > 0
	    && window->autosave_started == SP_FALSE) {
	    /* popup yes-no dialog */
	    if (spCreateMessageBox(window->window, NULL,
				   SW_AUTOSAVE_CLEAR_LABEL_QUESTION_MESSAGE,
				   SppDialogType, SP_QUESTION_DIALOG,
				   SppMessageBoxButtonType, SP_MB_YES_NO,
				   NULL) == SP_DR_NO) {
		return NULL;
	    }
	}
    }
#endif

    /* get new wave */
    if (strnone(filename)) {
	wave = swExtractWave(window->wave, offset, length);
    } else {
	wave = swExtractAndWriteWave(window->wave, offset, length, filename, plugin_name, file_type, file_desc);
    }
    
    if (wave == NULL) {
	spDisplayError(window->window, SW_ERROR_TITLE, SW_EDIT_ERROR_MESSAGE);
	return NULL;
    }

    /* create new window */
    new_window = swCreateWaveWindowAt(wave, window->config, window->data_type, -1.0, -1.0, x, y);
    /*new_window->wave->edit_flag = SP_TRUE;*/
    
    if (autosave_flag == SP_FALSE) {
	/* extract label */
	swExtractLabel(new_window, window, offset, length);
    }

    /* set window title to update edit state */
    swSetWindowTitle(new_window);

    swDrawOverview(new_window, SP_TRUE);

    if (window->draw_specgram == SP_TRUE || swIsSubplotVisible(window) == SP_TRUE) {
	spDebug(10, "swExtractWindowAt", "amp_min = %f, amp_max = %f\n", 
		window->amp_min, window->amp_max);
	new_window->amp_min = window->amp_min;
	new_window->amp_max = window->amp_max;
	new_window->draw_specgram = window->draw_specgram;
	new_window->specgram_config_flag = window->specgram_config_flag;
	new_window->specgram_analysis_type = window->specgram_analysis_type;
#ifdef SW_SUPPORT_ANALYSIS_SUBPLOT
	new_window->subplot_sgram = window->subplot_sgram;
	new_window->subplot_f0 = window->subplot_f0;
	new_window->subplot_power = window->subplot_power;
#ifdef SW_SUPPORT_STRAIGHT
	new_window->subplot_straight_sgram = window->subplot_straight_sgram;
	new_window->subplot_straight_ap = window->subplot_straight_ap;
#endif
	swUpdateSubplotMenu(new_window);
#endif
	swUpdateSpectrogramMenu(new_window);
	swUpdateSpectrogram(new_window, SP_TRUE);
    }
    
    return new_window;
}

swWindow swExtractWindowAt(swWindow window, spLong st, spLong ed, spBool autosave_flag, int x, int y)
{
    return swExtractAndSaveWindowAt(window, NULL, NULL, NULL, NULL, st, ed, autosave_flag, x, y);
}

swWindow swExtractWindow(swWindow window, spLong st, spLong ed, spBool autosave_flag)
{
    return swExtractWindowAt(window, st, ed, autosave_flag, -1, -1);
}

void swExtractWindowCB(spComponent component, swWindow window)
{
    if (window != NULL) {
	swExtractWindow(window, window->sel_st, window->sel_ed, SP_FALSE);
    }
    return;
}

#ifdef SW_SUPPORT_AUTOSAVE
static swAutosaveIdType getAutosaveId(swWindow window, int serial_number, char *autosave_id)
{
    swAutosaveIdType autosave_id_type;
    
    autosave_id_type = window->config->autosave_id_type;
    
    if (autosave_id_type == SW_AUTOSAVE_ID_EDGE_POINTS) {
	sprintf(autosave_id, "%ld-%ld", (long)window->sel_st, (long)window->sel_ed);
    } else {
	switch (autosave_id_type) {
	  case SW_AUTOSAVE_ID_2DIGIT_SERIAL:
	    sprintf(autosave_id, "%.2d", serial_number);
	    break;
	  case SW_AUTOSAVE_ID_3DIGIT_SERIAL:
	    sprintf(autosave_id, "%.3d", serial_number);
	    break;
	  case SW_AUTOSAVE_ID_4DIGIT_SERIAL:
	    sprintf(autosave_id, "%.4d", serial_number);
	    break;
	  default:		/* SW_AUTOSAVE_ID_SERIAL */
	    sprintf(autosave_id, "%d", serial_number);
	    break;
	}
    }
    
    return autosave_id_type;
}

static void getCoreName(swWindow window, const char *target_dirname, spBool no_dir, char *o_corename, char *o_suffix)
{
    char buf[SP_MAX_PATHNAME];
    char dirname[SP_MAX_PATHNAME];
    
    /* remove original suffix */
    spStrCopy(buf, SP_MAX_PATHNAME, window->wave->core->orig_filename);
    spRemoveSuffix(buf, o_suffix);
    
    /* get new filename */
    if (no_dir == SP_TRUE) {
        strcpy(o_corename, spGetBaseName(buf));
    } else if (spIsDir(target_dirname) == SP_TRUE) {
        spStrCopy(dirname, SP_MAX_PATHNAME, target_dirname);
        spAddDirSeparator(dirname);
		
	sprintf(o_corename, "%s%s", dirname, spGetBaseName(buf));
    } else {
	spStrCopy(o_corename, SP_MAX_PATHNAME, buf);
    }

    return;
}

static void getAutosaveFileName(swWindow window, char *filename, char *corename)
{
    int serial_number;
    swAutosaveIdType autosave_id_type;
    char autosave_id[SP_MAX_PATHNAME];
    char suffix[SP_MAX_PATHNAME];
    
    for (serial_number = 1;; serial_number++) {
	/* get autosave ID */
	autosave_id_type = getAutosaveId(window, serial_number, autosave_id);
	
	/* get core name */
	getCoreName(window, window->config->autosave_dir, SP_FALSE, corename, suffix);
	
	/* get new filename */
	if (strnone(window->config->autosave_id_prefix)) {
	    sprintf(filename, "%s%s%s", corename, autosave_id, suffix);
	} else {
	    sprintf(filename, "%s%s%s%s", corename, window->config->autosave_id_prefix,
		    autosave_id, suffix);
	}

	if (autosave_id_type == SW_AUTOSAVE_ID_EDGE_POINTS
	    || spIsFile(filename) == SP_FALSE) {
	    break;
	}
    }

    return;
}

void swExtractAutosaveWindowAt(swWindow window, int x, int y)
{
    double point_f, end_point_f;
    swWindow new_window;
    char *string;
    char corename[SP_MAX_PATHNAME];
    char filename[SP_MAX_PATHNAME];
    char labelfile[SP_MAX_PATHNAME];
    
    if (window != NULL) {
	if (strnone(window->wave->core->orig_filename)) {
	    spDisplayError(window->window, SW_ERROR_TITLE, SW_SAVE_FIRST_ERROR_MESSAGE);
	    return;
	}
	
	/* get autosave filename */
	getAutosaveFileName(window, filename, corename);
	spDebug(10, "swExtractAutosaveWindowAt", "filename = %s\n", filename);

	if ((new_window = swExtractAndSaveWindowAt(window, filename, NULL, NULL, NULL,
						   window->sel_st, window->sel_ed, SP_TRUE, x, y)) != NULL) {
	    swSetMouseCursor(new_window, SP_CURSOR_WAIT);
	    
	    /* set title of new window */
	    swSetWindowTitle(new_window);
		
	    /* set filename */
	    sprintf(labelfile, "%s%s", corename, window->config->autosave_label_suffix);

	    if (spIsFile(labelfile) == SP_TRUE) {
		if (window->autosave_started == SP_TRUE
		    && streq(swGetRegionLabelFileName(window->wave->labels), labelfile)) {
		} else {
		    /* read labels */
		    swReadLabel(window->wave, labelfile, SW_TIME_FORMAT_SEC, SP_TRUE);
		}
	    } else {
		/* clear labels */
		swClearLabels(window->wave->labels, SW_LABEL_TYPE_REGION);
		swSetLabelFileName(window->wave->labels, labelfile);
	    }

	    window->autosave_started = SP_TRUE;

	    if (window->config->autosave_label_fullpath == SP_TRUE) {
		string = filename;
	    } else {
		string = spGetBaseName(filename);
	    }

	    /* get end edge */
	    end_point_f = swSampToDim(window, window->sel_ed);

	    /* add label */
	    point_f = swSampToDim(window, window->sel_st);
	    swAddLabel(window->wave, point_f, end_point_f, string, NULL);
		
	    /* write label */
	    swWriteLabel(window->wave, window->name, labelfile,
			 SW_TIME_FORMAT_SEC, SP_TRUE);

	    /* redraw wave */
	    swSetSenseLevel(window);
	    swRedrawWave(window);
	    
	    swSetMouseCursor(new_window, SP_CURSOR_UNKNOWN);
	}
    }
    
    return;
}

void swExtractAutosaveWindow(swWindow window)
{
    swExtractAutosaveWindowAt(window, -1, -1);
    return;
}

void swExtractAutosaveWindowCB(spComponent component, swWindow window)
{
    swExtractAutosaveWindow(window);
    return;
}

#if 1
typedef struct _swSaveByLabelFileInfo swSaveByLabelFileInfo;

struct _swSaveByLabelFileInfo {
    char filename[SP_MAX_PATHNAME];
    long repetition_index;
    
    swLabel *label;
    spLong offset;
    spLong length;
};

spBool swUpdateSaveByLabelRepetitionIndex(swSaveByLabelFileInfo *file_infos, long index, spBool use_data_flag)
{
    long k;
    spBool updated = SP_FALSE;

    spDebug(50, "swUpdateSaveByLabelRepetitionIndex",  "index = %ld\n", index);
    
    file_infos[index].repetition_index = 0;

    for (k = index - 1; k >= 0; k--) {
	if (use_data_flag == SP_TRUE) {
	    spDebug(50, "swUpdateSaveByLabelRepetitionIndex",
		    "file_infos[%ld].label->data = %s, file_infos[%ld].label->data = %s\n",
		    k, file_infos[k].label->data, index, file_infos[index].label->data);
	    
	    if ((strnone(file_infos[k].label->data) && strnone(file_infos[index].label->data))
		|| streq(file_infos[k].label->data, file_infos[index].label->data)) {
		file_infos[index].repetition_index = file_infos[k].repetition_index + 1;
		updated = SP_TRUE;
		break;
	    }
	} else {
	    spDebug(50, "swUpdateSaveByLabelRepetitionIndex",
		    "file_infos[%ld].label->string = %s, file_infos[%ld].label->string = %s\n",
		    k, file_infos[k].label->string, index, file_infos[index].label->string);
	    
	    if ((strnone(file_infos[k].label->string) && strnone(file_infos[index].label->string))
		|| streq(file_infos[k].label->string, file_infos[index].label->string)) {
		file_infos[index].repetition_index = file_infos[k].repetition_index + 1;
		updated = SP_TRUE;
		break;
	    }
	}
    }

    spDebug(50, "swUpdateSaveByLabelRepetitionIndex",  "updated = %d\n", updated);
    
    return updated;
}

static void swReplaceSaveByLabelNameIllegalChar(swConfig config, char *string)
{
    int i;
    int len;
    int prev_c;

    len = (int)strlen(string);

    prev_c = NUL;
    for (i = 0; i < len; i++) {
	if (spIsMBTailCandidate(prev_c, string[i]) == SP_FALSE
	    && (string[i] == SP_DIR_SEPARATOR || string[i] == SP_ANOTHER_DIR_SEPARATOR)) {
	    if (config->sbl_illegal_char_to_space == SP_TRUE) {
		string[i] = ' ';
	    } else {
		string[i] = '_';
	    }
	}
        prev_c = string[i];
    }
    
    return;
}

static void swGetSaveByLabelNameBody(swConfig config, swSaveByLabelNamingRule naming_rule,
				     swSaveByLabelFileInfo *file_info, char *labelpart, int labelpart_size)
{
    char *string;
	
    labelpart[0] = NUL;
    
    if (naming_rule == SW_SBL_NAMING_LABEL
	|| naming_rule == SW_SBL_NAMING_DATA
	|| naming_rule == SW_SBL_NAMING_ORIG_AND_LABEL
	|| naming_rule == SW_SBL_NAMING_ORIG_AND_DATA) {
	if (naming_rule == SW_SBL_NAMING_DATA
	    || naming_rule == SW_SBL_NAMING_ORIG_AND_DATA) {
	    string = file_info->label->data;
	} else {
	    string = file_info->label->string;
	}

	if (config->sbl_data_as_filename == SP_TRUE) {
	    spStrCopy(labelpart, labelpart_size, spGetBaseName(string));
	    spRemoveSuffix(labelpart, NULL);
	} else {
	    spStrCopy(labelpart, labelpart_size, string);
	}

	swReplaceSaveByLabelNameIllegalChar(config, labelpart);
    }

    return;
}

spBool swUpdateSaveByLabelFileName(swWindow window, swSaveByLabelNamingRule naming_rule, char *repetition_suffix_format,
                                   const char *target_dirname, swSaveByLabelFileInfo *file_info, long index, char *o_labelpart)
{
    swConfig config;
    long name_index;
    char labelpart[SP_MAX_PATHNAME];
    char corename[SP_MAX_PATHNAME];
    char autosave_id[SP_MAX_PATHNAME];
    char suffix[SP_MAX_PATHNAME];

    config = window->config;
    
    file_info->filename[0] = NUL;

    swGetSaveByLabelNameBody(config, naming_rule, file_info, labelpart, sizeof(labelpart));
    getCoreName(window, target_dirname, SP_FALSE, corename, suffix);
    spDebug(50, "swUpdateSaveByLabelFileName", "labelpart = %s, corename = %s\n", labelpart, corename);

    if (naming_rule == SW_SBL_NAMING_AUTOSAVE
	|| naming_rule == SW_SBL_NAMING_ORIG_AND_LABEL
	|| naming_rule == SW_SBL_NAMING_ORIG_AND_DATA
	|| strnone(labelpart)) {
	spStrCopy(file_info->filename, sizeof(file_info->filename), corename);
	if (naming_rule == SW_SBL_NAMING_ORIG_AND_LABEL
	    || naming_rule == SW_SBL_NAMING_ORIG_AND_DATA) {
	    if (!strnone(config->autosave_id_prefix)) {
		spStrCat(file_info->filename, sizeof(file_info->filename), config->autosave_id_prefix);
	    }
	    spStrCat(file_info->filename, sizeof(file_info->filename), labelpart);
	}
    } else {
	if (spIsDir(target_dirname) == SP_TRUE) {
	    spStrCopy(file_info->filename, SP_MAX_PATHNAME, target_dirname);
	    spAddDirSeparator(file_info->filename);
	}
	spStrCat(file_info->filename, sizeof(file_info->filename), labelpart);
    }

    if (naming_rule == SW_SBL_NAMING_AUTOSAVE) {
	name_index = index + 1;
    } else if (config->sbl_index_from_1st == SP_TRUE || strnone(labelpart)) {
	name_index = file_info->repetition_index + 1;
    } else {
	name_index = file_info->repetition_index;
    }
	
    if (name_index > 0) {
        if (strnone(repetition_suffix_format)) {
            autosave_id[0] = NUL;
            if (!strnone(config->autosave_id_prefix)) {
                spStrCopy(autosave_id, sizeof(autosave_id), config->autosave_id_prefix);
            }
            getAutosaveId(window, name_index, autosave_id + strlen(autosave_id));
        } else {
            if (sprintf(autosave_id, repetition_suffix_format, name_index) <= 0) {
                spDebug(10, "swUpdateSaveByLabelFileName", "error, repetition_suffix_format = %s\n",
                        repetition_suffix_format);
                return SP_FALSE;
            }
        }
	spStrCat(file_info->filename, sizeof(file_info->filename), autosave_id);
    }
	
    spStrCat(file_info->filename, sizeof(file_info->filename), suffix);
    spDebug(50, "swUpdateSaveByLabelFileName", "filename = %s\n", file_info->filename);

    if (o_labelpart != NULL) spStrCopy(o_labelpart, SP_MAX_PATHNAME, labelpart);

    return SP_TRUE;
}

spBool swUpdateSaveByLabelFileNameCustom(swWindow window, char *custom_format, char *repetition_suffix_format,
                                         const char *target_dirname, swSaveByLabelFileInfo *file_info, long index, char *o_labelpart)
{
    int i, j;
    int len;
    int pos;
    int c, prev_c;
    swConfig config;
    long name_index;
    char corename[SP_MAX_PATHNAME];
    char suffix[SP_MAX_PATHNAME];
    char buf[SP_MAX_PATHNAME];
    char labelpart[SP_MAX_PATHNAME];
    char numpart[SP_MAX_PATHNAME];
    char autosave_id[SP_MAX_PATHNAME];

    config = window->config;

    getCoreName(window, NULL, SP_TRUE, corename, suffix);
    
    labelpart[0] = numpart[0] = NUL;
    file_info->filename[0] = NUL;
    len = (int)strlen(custom_format);

    if (spIsDir(target_dirname) == SP_TRUE) {
        spStrCopy(file_info->filename, SP_MAX_PATHNAME, target_dirname);
        spAddDirSeparator(file_info->filename);
        pos = (int)strlen(file_info->filename);
    } else {
        pos = 0;
    }
    
    prev_c = NUL;
    for (i = 0; i < len; i++) {
        c = custom_format[i];
	if (spIsMBTailCandidate(prev_c, c) == SP_FALSE && c == '%') {
            c = custom_format[++i];
            if (c == '%') {
                file_info->filename[pos++] = c;
            } else if (c == 'F') {
                file_info->filename[pos] = NUL;
                spStrCat(file_info->filename, sizeof(file_info->filename), corename);
                pos = (int)strlen(file_info->filename);
                spDebug(80, "swUpdateSaveByLabelFileNameCustom", "%%%c: corename = %s, filename = %s, new pos = %d\n",
                        c, corename, file_info->filename, pos);
            } else if (c == 'L' || c == 'D') {
                swGetSaveByLabelNameBody(config, c == 'D' ? SW_SBL_NAMING_DATA : SW_SBL_NAMING_LABEL,
                                         file_info, labelpart, sizeof(labelpart));
                file_info->filename[pos] = NUL;
                spStrCat(file_info->filename, sizeof(file_info->filename), labelpart);
                pos = (int)strlen(file_info->filename);
                spDebug(80, "swUpdateSaveByLabelFileNameCustom", "%%%c: labelpart = %s, filename = %s, new pos = %d\n",
                        c, labelpart, file_info->filename, pos);
            } else {
                buf[0] = '%';
                for (j = 1; c != NUL; j++) {
                    buf[j] = c;
                    if (c == 'd' || c == 'i' || c == 'x' || c == 'u' || c == 'o') {
                        buf[++j] = NUL;
                        if (sprintf(numpart, buf, index + 1) <= 0) {
                            spDebug(10, "swUpdateSaveByLabelFileNameCustom", "%%%c: error, format = %s\n", c, buf);
                            return SP_FALSE;
                        }
                        file_info->filename[pos] = NUL;
                        spStrCat(file_info->filename, sizeof(file_info->filename), numpart);
                        pos = (int)strlen(file_info->filename);
                        spDebug(80, "swUpdateSaveByLabelFileNameCustom", "%s: numpart = %s, filename = %s, new pos = %d\n",
                                buf, numpart, file_info->filename, pos);
                        break;
                    } else if (c == 'c' || c == 's' || c == 'f' || c == 'e' || c == 'E' || c == 'g' || c == 'G') {
                        return SP_FALSE;
                    }

                    c = custom_format[++i];
                }
            }
            c = NUL;
        } else {
            file_info->filename[pos++] = c;
        }
        if (pos + 1 >= sizeof(file_info->filename)) {
            break;
        }
        prev_c = c;
    }
    file_info->filename[pos] = NUL;

    if (strnone(numpart)) {
        if (config->sbl_index_from_1st == SP_TRUE || strnone(labelpart)) {
            name_index = file_info->repetition_index + 1;
        } else {
            name_index = file_info->repetition_index;
        }
        if (name_index > 0) {
            if (strnone(repetition_suffix_format)) {
                autosave_id[0] = NUL;
                if (!strnone(config->autosave_id_prefix)) {
                    spStrCopy(autosave_id, sizeof(autosave_id), config->autosave_id_prefix);
                }
                getAutosaveId(window, name_index, autosave_id + strlen(autosave_id));
            } else {
                if (sprintf(autosave_id, repetition_suffix_format, name_index) <= 0) {
                    spDebug(10, "swUpdateSaveByLabelFileNameCustom", "error, repetition_suffix_format = %s\n",
                            repetition_suffix_format);
                    return SP_FALSE;
                }
            }
            spStrCat(file_info->filename, sizeof(file_info->filename), autosave_id);
        }
    }
    
    spStrCat(file_info->filename, sizeof(file_info->filename), suffix);
    spDebug(80, "swUpdateSaveByLabelFileNameCustom", "final filename = %s\n", file_info->filename);

    if (o_labelpart != NULL) spStrCopy(o_labelpart, SP_MAX_PATHNAME, labelpart);
	
    return SP_TRUE;
}

spBool swUpdateSaveByLabelFileNameAll(swWindow window, swSaveByLabelNamingRule naming_rule, char *custom_format,
                                      char *repetition_suffix_format, const char *target_dirname,
                                      swSaveByLabelFileInfo *file_infos, long num_file_info, long *o_num_null_label)
{
    long k;
    char labelpart[SP_MAX_PATHNAME];
    long num_null_label = 0;
    spBool flag = SP_TRUE;

    for (k = 0; k < num_file_info; k++) {
        if (naming_rule == SW_SBL_NAMING_CUSTOM) {
            if (swUpdateSaveByLabelFileNameCustom(window, custom_format, repetition_suffix_format, target_dirname, &file_infos[k], k, labelpart) == SP_FALSE) {
                flag = SP_FALSE;
                num_null_label = num_file_info;
                break;
            }
        } else {
            swUpdateSaveByLabelFileName(window, naming_rule, repetition_suffix_format, target_dirname, &file_infos[k], k, labelpart);
        }
	if (strnone(labelpart)) {
	    ++num_null_label;
	}
    }

    if (o_num_null_label != NULL) *o_num_null_label = num_null_label;

    return flag;
}

swSaveByLabelFileInfo *swGetSaveByLabelFileInfo(swWindow window, spBool region_label_flag,
						swSaveByLabelNamingRule naming_rule, long *num_file_info)
{
    long k;
    long num_label;
    spLong st, ed;
    spLong prev_ed;
    long prev_label_index;
    swLabel *label;
    long file_info_index;
    spBool use_data_flag;
    swSaveByLabelFileInfo *file_infos;

    if (swIsNoWave(window) == SP_TRUE) return NULL;

    if (region_label_flag == SP_TRUE) {
	num_label = swGetNumRegionLabel(window->wave);
    } else {
	num_label = swGetNumNormalLabel(window->wave);
    }
    spDebug(50, "swGetSaveByLabelFileInfo", "num_label = %ld\n", num_label);
    
    if (num_label <= 0) {
	return NULL;
    }

    if (naming_rule == SW_SBL_NAMING_DATA
	|| naming_rule == SW_SBL_NAMING_ORIG_AND_DATA) {
	use_data_flag = SP_TRUE;
    } else {
	use_data_flag = SP_FALSE;
    }

    swSortLabels(window->wave->labels, NULL);
    
    file_infos = xalloc(num_label, swSaveByLabelFileInfo);
    
    prev_ed = -1;
    prev_label_index = -1;
    file_info_index = 0;
    
    for (k = 0; k <= window->wave->labels->num_buffer; k++) {
	label = NULL;
	st = ed = -1;
	
	if (region_label_flag == SP_TRUE) {
	    if (k >= window->wave->labels->num_buffer) {
		break;
	    }
	    
	    if (window->wave->labels->label[k].time >= 0.0
		&& window->wave->labels->label[k].end_time >= 0.0) {
		st = swDimToSamp(window, window->wave->labels->label[k].time);
		ed = swDimToSamp(window, window->wave->labels->label[k].end_time);
		label = &window->wave->labels->label[k];
	    }
	} else {
	    if (k == window->wave->labels->num_buffer
		|| window->wave->labels->label[k].time >= 0.0) {
		if (prev_label_index >= 0) {
		    label = &window->wave->labels->label[prev_label_index];
		}
		st = prev_ed;
		if (k == window->wave->labels->num_buffer) {
		    ed = window->wave->total_length - 1;
		} else {
		    ed = swDimToSamp(window, window->wave->labels->label[k].time);
		}
		prev_ed = ed;
		prev_label_index = k;
	    }
	}

	if (label != NULL) {
	    file_infos[file_info_index].filename[0] = NUL;
	    file_infos[file_info_index].label = label;
	    file_infos[file_info_index].offset = st;
	    file_infos[file_info_index].length = ed - st + 1;
	    spDebug(50, "swGetSaveByLabelFileInfo", "index = %ld, st = %ld, ed = %ld, length = %ld\n",
		    file_info_index, st, ed, file_infos[file_info_index].length);
	    
	    swUpdateSaveByLabelRepetitionIndex(file_infos, file_info_index, use_data_flag);

	    ++file_info_index;
	}
    }

    *num_file_info = file_info_index;

    return file_infos;
}
    
void swExtractSaveByLabel(swWindow window, swSaveByLabelFileInfo *file_infos, long num_file_info,
			  spBool no_overwrite_prompt, spBool create_window)
{
    long k;
    swWave wave;
    swWindow new_window;
    
    if (swIsNoWave(window) == SP_TRUE) return;

    if (strnone(window->wave->core->orig_filename)) {
	spDisplayError(window->window, SW_ERROR_TITLE, SW_SAVE_FIRST_ERROR_MESSAGE);
	return;
    }
	
    if (num_file_info <= 0) {
	spDisplayError(window->window, SW_ERROR_TITLE, SW_NO_LABEL_ERROR_MESSAGE);
	return;
    }

    window->execute_save_by_label = SP_TRUE;
    
    for (k = 0; k < num_file_info; k++) {
	if (!strnone(file_infos[k].filename)) {
	    if (no_overwrite_prompt == SP_FALSE && spExists(file_infos[k].filename) == SP_TRUE
		&& spOverwritePrompt(window->window, file_infos[k].filename) == SP_FALSE) {
		continue;
	    }
	    
	    /* get new wave */
	    if ((window->save_by_label_wave
		 = swExtractAndWriteWave(window->wave, file_infos[k].offset, file_infos[k].length,
					 file_infos[k].filename, NULL, NULL, NULL)) == NULL) {
		spDisplayError(window->window, SW_ERROR_TITLE, SW_EDIT_ERROR_MESSAGE);
		return;
	    }

	    if (create_window == SP_TRUE) {
		new_window = swCreateWaveWindowAt(window->save_by_label_wave, window->config,
						  window->data_type, -1.0, -1.0, -1, -1);
		swSetWindowTitle(new_window);
		swDrawOverview(new_window, SP_TRUE);
	    } else {
		wave = window->save_by_label_wave;
		window->save_by_label_wave = NULL;
		swDestroyWave(wave);
	    }
	}
    }

    window->execute_save_by_label = SP_FALSE;
    
    return;
}

spBool swProcessSaveByLabel(swWindow window, swWave wave, spLong pos, swEditType edit_type)
{
    if (window->save_by_label_wave == NULL) return SP_FALSE;
    
    spDebug(50, "swProcessSaveByLabel", "edit_type = %d, pos = %ld\n", edit_type, pos);
    
    return SP_TRUE;
}

typedef struct _swSaveByLabelDialog {
    spComponent dialog;
    
    spComponent save_dir_field;
    spComponent naming_rule_field;

    spBool text_edit_flag;
    spComponent naming_custom_format_field;
    spComponent naming_repetition_suffix_format_field;
    
    spComponent index_from_1st_checkbox;
    spComponent illegal_char_to_space_checkbox;
    spComponent data_as_filename_checkbox;
    spComponent no_overwrite_prompt_checkbox;
    spComponent create_window_checkbox;

    spComponent filename_list;

    const char *naming_rule_strings[10];
    /* %d, %02d, %03d: number, %L: label string, %D: data string, %F: original filename */
    spBool naming_custom_format_fixed;
    char *naming_custom_format;
    char *naming_repetition_suffix_format; /* can include number only */
    
    swWindow current_window;
    spBool region_label_flag;
    swSaveByLabelNamingRule current_naming_rule;

    long num_file_info;
    swSaveByLabelFileInfo *file_infos;
    long num_null_label;
    
} *swSaveByLabelDialog;

static char *sw_sbl_dialog_default_repetition_suffix_format_strings[] = {
    "-%d",
    "-%02d",
    "-%03d",
    "_%d",
    "_%02d",
    "_%03d",
    NULL,
};

#if defined(SW_SUPPORT_SBL_NAMING_CUSTOM)
static char *sw_sbl_dialog_default_custom_format_strings[] = {
    SW_SBL_NAMING_DEFAULT_CUSTOM_FORMAT_LABEL,
    SW_SBL_NAMING_DEFAULT_CUSTOM_FORMAT_DATA,
    SW_SBL_NAMING_DEFAULT_CUSTOM_FORMAT_ORIG_AND_LABEL,
    SW_SBL_NAMING_DEFAULT_CUSTOM_FORMAT_ORIG_AND_DATA,
    SW_SBL_NAMING_DEFAULT_CUSTOM_FORMAT_AUTOSAVE,
    NULL
};

static spBool swCheckSaveByLabelNamingCustomFormatText(const char *text)
{
    int len;
    int i, j;
    int c, prev_c;
    int label_format_count;
    int file_format_count;
    int serial_number_format_count;
    char buf[SP_MAX_LINE];
    char numpart[SP_MAX_LINE];
    spBool flag = SP_FALSE;

    len = (int)strlen(text);
    serial_number_format_count = label_format_count = file_format_count = 0;
    prev_c = NUL;

    for (i = 0; i < len; i++) {
        c = text[i];
	if (spIsMBTailCandidate(prev_c, c) == SP_FALSE && c == '%' && text[i+1] != '%') {
            c = text[++i];
            if (c == 'F' || c == 'L' || c == 'D') {
                if (c == 'F') {
                    if (file_format_count >= 1) {
                        return SP_FALSE;
                    }
                    ++file_format_count;
                } else {
                    if (label_format_count >= 1) {
                        return SP_FALSE;
                    }
                    ++label_format_count;
                }
                flag = SP_TRUE;
            } else {
                if (serial_number_format_count >= 1) {
                    return SP_FALSE;
                }
                
                buf[0] = '%';
                for (j = 1; c != NUL; j++) {
                    buf[j] = c;
                    if (c == 'd' || c == 'i' || c == 'x' || c == 'u' || c == 'o') {
                        buf[j+1] = NUL;
                        if (sprintf(numpart, buf, 300) <= 2) {
                            spDebug(10, "swCheckSaveByLabelNamingCustomFormatText", "%%%c: error, format = %s\n", c, buf);
                            return SP_FALSE;
                        }
                        flag = SP_TRUE;
                        ++serial_number_format_count;
                        break;
                    } else if (c == 'c' || c == 's' || c == 'f' || c == 'e' || c == 'E' || c == 'g' || c == 'G') {
                        return SP_FALSE;
                    }
                
                    c = text[++i];
                }
            }
        }

        prev_c = c;
    }

    spDebug(80, "swCheckSaveByLabelNamingCustomFormatText", "flag = %d, serial_number_format_count = %d, file_format_count = %d, label_format_count = %d\n",
            flag, serial_number_format_count, file_format_count, label_format_count);
    
    if (!(serial_number_format_count == 1 || label_format_count == 1)) {
        return SP_FALSE;
    }
    
    return flag;
}
#endif

static spBool swCheckSaveByLabelNamingRepetitionSuffixFormatText(const char *text)
{
    int len;
    int i, j;
    int c, prev_c;
    int serial_number_format_count;
    char buf[SP_MAX_LINE];
    char numpart[SP_MAX_LINE];
    spBool flag = SP_FALSE;

    len = (int)strlen(text);
    serial_number_format_count = 0;
    prev_c = NUL;

    for (i = 0; i < len; i++) {
        c = text[i];
	if (spIsMBTailCandidate(prev_c, c) == SP_FALSE && c == '%' && text[i+1] != '%') {
            if (serial_number_format_count >= 1) {
                return SP_FALSE;
            }
            c = text[++i];
            buf[0] = '%';
            for (j = 1; c != NUL; j++) {
                buf[j] = c;
                if (c == 'd' || c == 'i' || c == 'x' || c == 'u' || c == 'o') {
                    buf[j+1] = NUL;
                    if (sprintf(numpart, buf, 300) <= 2) {
                        spDebug(10, "swCheckSaveByLabelNamingRepetitionSuffixFormatText", "%%%c: error, format = %s\n", c, buf);
                        return SP_FALSE;
                    }
                    flag = SP_TRUE;
                    ++serial_number_format_count;
                    break;
                } else if (c == 'c' || c == 's' || c == 'f' || c == 'e' || c == 'E' || c == 'g' || c == 'G') {
                    return SP_FALSE;
                }
                
                c = text[++i];
            }
        }

        prev_c = c;
    }

    if (serial_number_format_count != 1) {
        return SP_FALSE;
    }
    
    return flag;
}

void swSetSaveByLabelDialogSensitive(swSaveByLabelDialog dialog)
{
    spDebug(50, "swSetSaveByLabelDialogSensitive", "current_naming_rule = %d\n", dialog->current_naming_rule);
    
    if (dialog->current_naming_rule == SW_SBL_NAMING_AUTOSAVE) {
	spSetSensitive(dialog->index_from_1st_checkbox, SP_FALSE);
	spSetSensitive(dialog->illegal_char_to_space_checkbox, SP_FALSE);
	spSetSensitive(dialog->data_as_filename_checkbox, SP_FALSE);
	spSetSensitive(dialog->naming_repetition_suffix_format_field, SP_FALSE);
#if defined(SW_SUPPORT_SBL_NAMING_CUSTOM)
	spSetSensitive(dialog->naming_custom_format_field, SP_FALSE);
#endif
    } else {
	spSetSensitive(dialog->index_from_1st_checkbox, SP_TRUE);
	spSetSensitive(dialog->illegal_char_to_space_checkbox, SP_TRUE);
	spSetSensitive(dialog->data_as_filename_checkbox, SP_TRUE);
	spSetSensitive(dialog->naming_repetition_suffix_format_field, SP_TRUE);
#if defined(SW_SUPPORT_SBL_NAMING_CUSTOM)
        if (dialog->current_naming_rule == SW_SBL_NAMING_CUSTOM) {
            spSetSensitive(dialog->naming_custom_format_field, SP_TRUE);
        } else {
            spSetSensitive(dialog->naming_custom_format_field, SP_FALSE);
        }
#endif
    }

    return;
}

spBool swUpdateSaveByLabelList(swSaveByLabelDialog dialog, const char *initial_dir)
{
    long k;
    const char *ptr;
    const char *dir;
    spBool custom_error_flag = SP_TRUE;
    spBool flag = SP_TRUE;
    
    if (dialog->file_infos != NULL) {
	xfree(dialog->file_infos);
        dialog->file_infos = NULL;
    }

#if defined(SW_SUPPORT_SBL_NAMING_CUSTOM)
    if ((ptr = spGetTextString(dialog->naming_custom_format_field)) != NULL) {
        if (!strnone(ptr) && dialog->current_naming_rule == SW_SBL_NAMING_CUSTOM
            && swCheckSaveByLabelNamingCustomFormatText(ptr) == SP_FALSE) {
            if (initial_dir == NULL) {
                /* editing in dialog window */
                spDisplayError(dialog->dialog, SW_ERROR_TITLE, SW_SBL_NAMING_WRONG_FORMAT_ERROR_MESSAGE);
            }
            return SP_FALSE;
        }
        spStrCopy(dialog->naming_custom_format, SP_MAX_SETUP_VALUE, ptr);
    }
#endif
    if ((ptr = spGetTextString(dialog->naming_repetition_suffix_format_field)) != NULL) {
        if (!strnone(ptr) && dialog->current_naming_rule != SW_SBL_NAMING_AUTOSAVE
            && swCheckSaveByLabelNamingRepetitionSuffixFormatText(ptr) == SP_FALSE) {
            if (initial_dir == NULL) {
                /* editing in dialog window */
                spDisplayError(dialog->dialog, SW_ERROR_TITLE, SW_SBL_NAMING_WRONG_FORMAT_ERROR_MESSAGE);
            }
            return SP_FALSE;
        }
        spStrCopy(dialog->naming_repetition_suffix_format, SP_MAX_SETUP_VALUE, ptr);
    }

    if (!strnone(initial_dir)) {
	dir = initial_dir;
    } else {
	dir = spGetTextString(dialog->save_dir_field);
    }
    
    dialog->file_infos = swGetSaveByLabelFileInfo(dialog->current_window, dialog->region_label_flag,
						  dialog->current_naming_rule, &dialog->num_file_info);
    custom_error_flag = swUpdateSaveByLabelFileNameAll(dialog->current_window, dialog->current_naming_rule, dialog->naming_custom_format,
                                                       dialog->naming_repetition_suffix_format, dir, dialog->file_infos,
                                                       dialog->num_file_info, &dialog->num_null_label);

    if (custom_error_flag == SP_FALSE) {
        flag = SP_FALSE;
	if (initial_dir == NULL) {
            /* editing in dialog window */
	    spDisplayError(dialog->dialog, SW_ERROR_TITLE, SW_SBL_NAMING_WRONG_FORMAT_ERROR_MESSAGE);
	}
    } else if (!(dialog->current_naming_rule == SW_SBL_NAMING_CUSTOM || dialog->current_naming_rule == SW_SBL_NAMING_AUTOSAVE)
               && dialog->num_file_info == dialog->num_null_label) {
        flag = SP_FALSE;
	if (initial_dir == NULL) {
            /* editing in dialog window */
	    spDisplayError(dialog->dialog, SW_ERROR_TITLE, SW_SBL_NAMING_RULE_NULL_LABEL_ERROR_MESSAGE);
	}
	dialog->current_naming_rule = SW_SBL_NAMING_AUTOSAVE;
        swUpdateSaveByLabelFileNameAll(dialog->current_window, dialog->current_naming_rule, dialog->naming_custom_format,
                                       dialog->naming_repetition_suffix_format, dir, dialog->file_infos,
                                       dialog->num_file_info, &dialog->num_null_label);
	spSelectListIndex(dialog->naming_rule_field, SW_SBL_NAMING_AUTOSAVE);
    }

    spClearList(dialog->filename_list);
    if (custom_error_flag == SP_TRUE) {
        for (k = 0; k < dialog->num_file_info; k++) {
            spDebug(50, "swUpdateSaveByLabelList", "dialog->file_infos[%ld].filename = %s\n", k, dialog->file_infos[k].filename);
            spAddListItem(dialog->filename_list, spGetBaseName(dialog->file_infos[k].filename));
        }
    }

    swSetSaveByLabelDialogSensitive(dialog);
    
    return SP_TRUE;
}

void swEditSaveByLabelDialogText(spComponent component, swSaveByLabelDialog dialog)
{
    dialog->text_edit_flag = SP_TRUE;
    return;
}

void swSelectSaveByLabelNamingRule(spComponent component, swSaveByLabelDialog dialog)
{
    int index;

    if ((index = spGetSelectedListIndex(component)) >= 0
	&& index != dialog->current_naming_rule) {
	spDebug(50, "swSelectSaveByLabelNamingRule", "old rule = %d, new rule = %d\n",
		dialog->current_naming_rule, index);
	
#if defined(SW_SUPPORT_SBL_NAMING_CUSTOM)
        if (index == SW_SBL_NAMING_CUSTOM) {
            if (dialog->naming_custom_format_fixed == SP_FALSE || strnone(dialog->naming_custom_format)) {
                char *str;
                dialog->naming_custom_format_fixed = SP_FALSE;
                str = sw_sbl_dialog_default_custom_format_strings[MAX(dialog->current_naming_rule, 0)];
                spStrCopy(dialog->naming_custom_format, SP_MAX_SETUP_VALUE, str);
            }
            spSetTextString(dialog->naming_custom_format_field, dialog->naming_custom_format);
        }
#endif
	dialog->current_naming_rule = index;

	if (swUpdateSaveByLabelList(dialog, NULL) == SP_TRUE) {
            dialog->current_window->config->sbl_naming_rule = dialog->current_naming_rule;
        }
    }
    
    return;
}

void swSaveByLabelDialogCheckBoxCB(spComponent component, swSaveByLabelDialog dialog)
{
    const char *name;
    spBool set;
    spBool updated = SP_FALSE;
    swConfig config;

    if (dialog->current_window == NULL
	|| spGetToggleState(component, &set) == SP_FALSE) return;

    name = spGetName(component);

    config = dialog->current_window->config;

    if (streq(name, "saveByLabelIndexFrom1stCheckBox")) {
	if (set != config->sbl_index_from_1st) {
	    config->sbl_index_from_1st = set;
	    updated = SP_TRUE;
	}
    } else if (streq(name, "saveByLabelIllegalCharToSpaceCheckBox")) {
	if (config->sbl_illegal_char_to_space != set) {
	    config->sbl_illegal_char_to_space = set;
	    updated = SP_TRUE;
	}
    } else if (streq(name, "saveByLabelDataAsFileNameCheckBox")) {
	if (config->sbl_data_as_filename != set) {
	    config->sbl_data_as_filename = set;
	    updated = SP_TRUE;
	}
    } else if (streq(name, "saveByLabelNoOverwritePromptCheckBox")) {
	if (config->sbl_no_overwrite_prompt != set) {
	    config->sbl_no_overwrite_prompt = set;
	}
    } else if (streq(name, "saveByLabelCreateWindowCheckBox")) {
	if (config->sbl_create_window != set) {
	    config->sbl_create_window = set;
	}
    }

    if (updated == SP_TRUE) {
	swUpdateSaveByLabelList(dialog, NULL);
    }
    
    return;
}

void swPopdownSaveByLabelDialog(spComponent component, swSaveByLabelDialog dialog)
{
    spCallbackReason reason;

    if (dialog->current_window == NULL) return;
    
    reason = spGetCallbackReason(component);
    
    if (reason == SP_CR_OK) {
	if (swUpdateSaveByLabelList(dialog, NULL) == SP_FALSE
            || dialog->text_edit_flag == SP_TRUE) {
            dialog->text_edit_flag = SP_FALSE;
            return;
        }
        dialog->text_edit_flag = SP_FALSE;
#if defined(SW_SUPPORT_SBL_NAMING_CUSTOM)
        if (dialog->current_naming_rule == SW_SBL_NAMING_CUSTOM && !strnone(dialog->naming_custom_format)) {
            dialog->naming_custom_format_fixed = SP_TRUE;
        }
#endif
	
	swExtractSaveByLabel(dialog->current_window, dialog->file_infos, dialog->num_file_info,
			     dialog->current_window->config->sbl_no_overwrite_prompt,
                             dialog->current_window->config->sbl_create_window);
    }
    
    /* popdown dialog */
    spPopdownWindow(dialog->dialog);
    
    return;
}

static swSaveByLabelDialog createSaveByLabelDialog(swConfig config)
{
    static swSaveByLabelDialog dialog = NULL;
    
    if (dialog != NULL) return dialog;

    /* initialize dialog */
    dialog = xalloc(1, struct _swSaveByLabelDialog);
    memset(dialog, 0, sizeof(struct _swSaveByLabelDialog));

    /* create dialog */
    dialog->dialog =
	spCreateDialogBox("saveByLabelDialog",
			  SppCallbackFunc, swPopdownSaveByLabelDialog,
			  SppCallbackData, dialog,
			  SppPopupStyle, SP_MODAL_POPUP,
			  SppCloseStyle, SP_UNMAP_CLOSE,
			  SppHelpButtonVisible, SP_TRUE,
			  /*SppHelpPath, "dialog/save_by_label.html",*/
			  NULL);

    dialog->save_dir_field = spCreateParamField(dialog->dialog, "saveByLabelSaveDirField", 0,
						SppTitle, SW_SBL_SAVE_DIR_LABEL,
						SppFieldType, SP_FIELD_TYPE_DIR_TEXT,
						SppFieldOffset, 160,
						SppFieldSize, 200,
						SppEditable, SP_TRUE,
						NULL);

    dialog->naming_rule_strings[0] = SW_SBL_NAMING_LABEL_LABEL;
    dialog->naming_rule_strings[1] = SW_SBL_NAMING_DATA_LABEL;
    dialog->naming_rule_strings[2] = SW_SBL_NAMING_ORIG_AND_LABEL_LABEL;
    dialog->naming_rule_strings[3] = SW_SBL_NAMING_ORIG_AND_DATA_LABEL;
    dialog->naming_rule_strings[4] = SW_SBL_NAMING_AUTOSAVE_LABEL;
#if defined(SW_SUPPORT_SBL_NAMING_CUSTOM)
    dialog->naming_rule_strings[5] = SW_SBL_NAMING_CUSTOM_LABEL;
    dialog->naming_rule_strings[6] = NULL;
#else
    dialog->naming_rule_strings[5] = NULL;
#endif
    
    dialog->naming_custom_format = config->sbl_naming_custom_format;
    dialog->naming_repetition_suffix_format = config->sbl_naming_repetition_suffix_format;

    dialog->naming_rule_field = spCreateParamField(dialog->dialog, "saveByLabelNamingRuleField", 0,
						   SppTitle, SW_SBL_NAMING_RULE_LABEL,
						   SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
						   SppFieldOffset, 160,
						   SppFieldSize, 200,
						   SppFieldStrings, dialog->naming_rule_strings,
						   SppEditable, SP_FALSE,
						   SppCallbackFunc, swSelectSaveByLabelNamingRule,
						   SppCallbackData, dialog,
						   NULL);

    dialog->naming_custom_format_fixed = SP_FALSE;
#if defined(SW_SUPPORT_SBL_NAMING_CUSTOM)
    dialog->naming_custom_format_field = spCreateParamField(dialog->dialog, "saveByLabelNamingCustomFormatField", 0,
                                                            SppTitle, SW_SBL_NAMING_CUSTOM_FORMAT_LABEL,
                                                            SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
                                                            SppFieldOffset, 160,
                                                            SppFieldSize, 200,
                                                            SppEditable, SP_TRUE,
                                                            SppFieldStrings, sw_sbl_dialog_default_custom_format_strings,
                                                            SppTextString, dialog->naming_custom_format,
                                                            SppCallbackFunc, swEditSaveByLabelDialogText,
                                                            SppCallbackData, dialog,
                                                            SppDescription, SW_SBL_NAMING_CUSTOM_FORMAT_FIELD_DESC,
                                                            NULL);
#else
    dialog->naming_custom_format_field = NULL;
#endif
    dialog->naming_repetition_suffix_format_field = spCreateParamField(dialog->dialog, "saveByLabelNamingRepetitionSuffixFormatField", 0,
                                                                       SppTitle, SW_SBL_NAMING_REPETITION_SUFFIX_FORMAT_LABEL,
                                                                       SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
                                                                       SppFieldOffset, 160,
                                                                       SppFieldSize, 200,
                                                                       SppEditable, SP_TRUE,
                                                                       SppFieldStrings, sw_sbl_dialog_default_repetition_suffix_format_strings,
                                                                       SppTextString, dialog->naming_repetition_suffix_format,
                                                                       SppCallbackFunc, swEditSaveByLabelDialogText,
                                                                       SppCallbackData, dialog,
                                                                       SppDescription, SW_SBL_NAMING_REPETITION_SUFFIX_FORMAT_FIELD_DESC,
                                                                       NULL);
    
    dialog->index_from_1st_checkbox = spCreateCheckBox(dialog->dialog, "saveByLabelIndexFrom1stCheckBox",
						       SppTitle, SW_SBL_INDEX_FROM_1ST_LABEL,
						       SppSet, config->sbl_index_from_1st,
						       SppCallbackFunc, swSaveByLabelDialogCheckBoxCB,
						       SppCallbackData, dialog,
						       NULL);
    
    dialog->illegal_char_to_space_checkbox = spCreateCheckBox(dialog->dialog, "saveByLabelIllegalCharToSpaceCheckBox",
							      SppTitle, SW_SBL_ILLEGAL_CHAR_TO_SPACE_LABEL,
							      SppSet, config->sbl_illegal_char_to_space,
							      SppCallbackFunc, swSaveByLabelDialogCheckBoxCB,
							      SppCallbackData, dialog,
							      NULL);
    
    dialog->data_as_filename_checkbox = spCreateCheckBox(dialog->dialog, "saveByLabelDataAsFileNameCheckBox",
							 SppTitle, SW_SBL_DATA_AS_FILENAME_LABEL,
							 SppSet, config->sbl_data_as_filename,
							 SppCallbackFunc, swSaveByLabelDialogCheckBoxCB,
							 SppCallbackData, dialog,
							 NULL);

    dialog->no_overwrite_prompt_checkbox = spCreateCheckBox(dialog->dialog, "saveByLabelNoOverwritePromptCheckBox",
                                                            SppTitle, SW_SBL_NO_OVERWRITE_PROMPT_LABEL,
                                                            SppSet, config->sbl_no_overwrite_prompt,
                                                            SppCallbackFunc, swSaveByLabelDialogCheckBoxCB,
                                                            SppCallbackData, dialog,
                                                            NULL);

    dialog->create_window_checkbox = spCreateCheckBox(dialog->dialog, "saveByLabelCreateWindowCheckBox",
							 SppTitle, SW_SBL_CREATE_WINDOW_LABEL,
							 SppSet, config->sbl_create_window,
							 SppCallbackFunc, swSaveByLabelDialogCheckBoxCB,
							 SppCallbackData, dialog,
							 NULL);

    dialog->filename_list = spCreateList(dialog->dialog, "saveByLabelFileNameList",
					 SppInitialHeight, 200,
					 NULL);
    
    return dialog;
}
    
void swPopupSaveByLabelDialog(swWindow window, spBool region_label_flag)
{
    int current_naming_rule_index;
    const char *title;
    char initial_dir[SP_MAX_PATHNAME];
    swSaveByLabelDialog dialog;

    if (window == NULL || window->wave == NULL)
	return;

    if (strnone(window->wave->core->orig_filename)) {
	spDisplayError(window->window, SW_ERROR_TITLE, SW_SAVE_FIRST_ERROR_MESSAGE);
	return;
    }
	
    /* get dialog */
    dialog = createSaveByLabelDialog(window->config);
    dialog->text_edit_flag = SP_FALSE;
    dialog->current_window = window;
    dialog->region_label_flag = region_label_flag;
    dialog->current_naming_rule = window->config->sbl_naming_rule;

    if (region_label_flag == SP_TRUE) {
	title = SW_SAVE_BY_REGION_LABEL_TITLE;
    } else {
	title = SW_SAVE_BY_NORMAL_LABEL_TITLE;
    }
    spSetParams(dialog->dialog, SppTitle, title, NULL);

    spStrCopy(initial_dir, sizeof(initial_dir), window->wave->core->orig_filename);
    spGetDirName(initial_dir);
    spDebug(50, "swPopupSaveByLabelDialog", "initial_dir = %s\n", initial_dir);

    spSetParams(dialog->save_dir_field, SppInitialDir, initial_dir, NULL);

    current_naming_rule_index = MAX(dialog->current_naming_rule, 0);
    spSelectListIndex(dialog->naming_rule_field, current_naming_rule_index);

    spSetToggleState(dialog->index_from_1st_checkbox, window->config->sbl_index_from_1st);
    spSetToggleState(dialog->illegal_char_to_space_checkbox, window->config->sbl_illegal_char_to_space);
    spSetToggleState(dialog->data_as_filename_checkbox, window->config->sbl_data_as_filename);
    spSetToggleState(dialog->no_overwrite_prompt_checkbox, window->config->sbl_no_overwrite_prompt);
    spSetToggleState(dialog->create_window_checkbox, window->config->sbl_create_window);

#if defined(SW_SUPPORT_SBL_NAMING_CUSTOM)
    if (dialog->naming_custom_format_fixed == SP_FALSE || strnone(dialog->naming_custom_format)) {
        dialog->naming_custom_format_fixed = SP_FALSE;
        spStrCopy(dialog->naming_custom_format, SP_MAX_SETUP_VALUE,
                  sw_sbl_dialog_default_custom_format_strings[current_naming_rule_index]);
    }
    spSetTextString(dialog->naming_custom_format_field, dialog->naming_custom_format);
#endif
    spSetTextString(dialog->naming_repetition_suffix_format_field, dialog->naming_repetition_suffix_format);

    swUpdateSaveByLabelList(dialog, initial_dir);
    
    /* popup dialog */
    spPopupWindow(dialog->dialog);

    return;
}

void swPopupSaveByNormalLabelDialogCB(spComponent component, swWindow window)
{
    swPopupSaveByLabelDialog(window, SP_FALSE);
    return;
}

void swPopupSaveByRegionLabelDialogCB(spComponent component, swWindow window)
{
    swPopupSaveByLabelDialog(window, SP_TRUE);
    return;
}
#endif

#endif

#undef SW_UNDO_RELOAD

static spBool undoWindow(swWindow window, spBool redo_flag)
{
    spLong edit_offset, edit_length;
    double prev_samp_rate;
    double prev_amp_min, prev_amp_max;
    spBool flag = SP_FALSE;
    
    if (window != NULL) {
	prev_samp_rate = window->wave->samp_rate;
	
	if (redo_flag == SP_TRUE) {
	    edit_offset = window->wave->edit_offset;
	    edit_length = window->wave->edit_length;
	    flag = swRedoWave(&window->wave);
	} else {
	    flag = swUndoWave(&window->wave);
	    edit_offset = window->wave->edit_offset;
	    edit_length = window->wave->edit_length;
	}
	if (flag == SP_TRUE) {
	    swDestroySpectrogram(window, SP_FALSE);
	    
	    window->offset = window->wave->offset;
	    window->length = window->wave->data_length;

	    /* save previous amplitude value */
	    prev_amp_min = window->amp_min;
	    prev_amp_max = window->amp_max;
	    window->amp_min = 0.0; window->amp_max = -1.0;
	    
#ifdef SW_UNDO_RELOAD
	    swResetWindow(window, SP_FALSE);
#else
	    swSetWaveToFirstSubArea(window, window->wave, SP_FALSE);
	    swRedrawWindow(window);
	    swDrawOverview(window, SP_TRUE);
#endif
	    if (window->draw_specgram == SP_TRUE || swIsSubplotVisible(window) == SP_TRUE) {
		if (window->wave->samp_rate == prev_samp_rate) {
		    /* restore amplitude value */
		    window->amp_min = prev_amp_min; window->amp_max = prev_amp_max;
		}
		swUpdateSpectrogram(window, SP_TRUE);
	    }
	    swUpdateLabels(window);
	    if (edit_length > 0) {
		swSelectRegion(window, window->wave->selected_channel, edit_offset,
			       edit_offset + edit_length - 1);
	    }
	}
    }

    return flag;
}

void swUndoWindowCB(spComponent component, swWindow window)
{
    undoWindow(window, SP_FALSE);
    return;
}

void swRedoWindowCB(spComponent component, swWindow window)
{
    undoWindow(window, SP_TRUE);
    return;
}

static spBool checkAmplitude(swWindow window, double value)
{
    if ((value > 1.0 || value <= -1.0)) {
	/* overflow check */
	if (swIsOverflow(window->wave->samp_bit,
			 value * swGetWaveMax(window->wave)) == SP_TRUE ||
	    swIsOverflow(window->wave->samp_bit,
			 value * swGetWaveMin(window->wave)) == SP_TRUE) {
	    if (spCreateMessageBox(window->window, SW_WARNING_TITLE,
				   SW_OVERFLOW_WARNING_MESSAGE,
				   SppDialogType, SP_WARNING_DIALOG,
				   SppMessageBoxButtonType, SP_MB_YES_NO,
				   NULL) != SP_DR_YES) {
		return SP_FALSE;
	    }
	}
    }

    return SP_TRUE;
}

spBool swEditFinish(swWindow window, swWave wave, spBool in_thread, swEditType edit_type)
{
    spBool flag = SP_TRUE;
    int selected_channel;
    spLong offset, length;
    spLong sel_st = -1, sel_ed = -1;
    double prev_amp_min, prev_amp_max;
    
    if (swIsNoWave(window) == SP_TRUE || edit_type == SW_EDIT_EXTRACT) return SP_FALSE;

    spDebug(10, "swEditFinish", "edit_type = %d, in_thread = %d\n", edit_type, in_thread);

    if (wave == NULL) {
	spDisplayError(window->window, SW_ERROR_TITLE, SW_EDIT_ERROR_MESSAGE);
	flag = SP_FALSE;
    } else if (edit_type == SW_EDIT_WRITE
	       || (edit_type >= SW_EDIT_GENERATE_RECORDING && edit_type <= SW_EDIT_GENERATE_WHITE_NOISE)) {
	window->offset = 0;
	window->length = 0;

	swInitWaveRange(window->wave);
	spDebug(10, "swEditFinish", "swInitWaveRange done\n");
		
	/* reset window */
	swResetWindow(window, in_thread);
    } else {
	selected_channel = window->wave->selected_channel;
	
	if (edit_type == SW_EDIT_CROP) {
	    offset = window->wave->edit_offset;
	    length = window->wave->edit_length;
	
	    window->wave = wave;
	    window->offset = 0;
	    window->length = 0;
		
	    /* crop label */
	    swCropLabel(window, offset, length);
	} else if (edit_type == SW_EDIT_DELETE) {
	    offset = window->wave->edit_offset;
	    length = window->wave->edit_length;
	    
	    window->wave = wave;
	    
	    /* delete label */
	    swDeleteLabel(window, offset, length);
	} else if (edit_type == SW_EDIT_SAMP_RATE_CONV) {
	    double st_f, ed_f;
	    double sel_st_f, sel_ed_f;
	    
	    st_f = swSampToDim(window, window->offset);
	    ed_f = swSampToDim(window, window->offset + window->length - 1);
	    sel_st_f = swSampToDim(window, window->sel_st);
	    sel_ed_f = swSampToDim(window, window->sel_ed);

	    window->wave = wave;
	    
	    window->offset = swDimToSamp(window, st_f);
	    window->length = swDimToSamp(window, ed_f) - window->offset + 1;
	    sel_st = swDimToSamp(window, sel_st_f);
	    sel_ed = swDimToSamp(window, sel_ed_f);
	} else {
	    sel_st = window->sel_st;
	    sel_ed = window->sel_ed;

	    window->wave = wave;
	}
	
	swDestroySpectrogram(window, SP_FALSE);

	/* save previous amplitude value */
	prev_amp_min = window->amp_min;
	prev_amp_max = window->amp_max;
	window->amp_min = 0.0; window->amp_max = -1.0;
	
	/* reset window */
	swResetWindow(window, in_thread);

	if (window->draw_specgram == SP_TRUE || swIsSubplotVisible(window) == SP_TRUE) {
	    if (edit_type != SW_EDIT_SAMP_RATE_CONV) {
		/* restore amplitude value */
		window->amp_min = prev_amp_min; window->amp_max = prev_amp_max;
	    }
	    swUpdateSpectrogram(window, SP_TRUE);
	}
	
	if (sel_st >= 0 && sel_ed >= 0) {
	    swSelectRegion(window, selected_channel, sel_st, sel_ed);
	}
	
	if (window->wave->overflow == SP_TRUE) {
	    spDisplayWarning(window->window, NULL, SW_OVERFLOW_HAPPENED_WARNING_MESSAGE);
	    window->wave->overflow = SP_FALSE;
	}
	if (edit_type == SW_EDIT_SAMP_RATE_CONV
	    && window->wave->exact_samp_rate != window->wave->samp_rate) {
	    spDisplayWarning(window->window, NULL, SW_SAMP_RATE_CHANGED_WARNING_MESSAGE,
			     window->wave->exact_samp_rate);
	}
    }

    spDebug(40, "swEditFinish", "done\n");

    return flag;
}

static spBool editWindow(swWindow window, swEditType edit_type,
			 spLong st, spLong ed, double value, int samp_bit, double samp_rate)
{
    spBool flag = SP_TRUE;
    spLong offset = 0;
    spLong length = 0;
    swWave wave = NULL;

    if (swIsNoWave(window) == SP_TRUE) return SP_FALSE;

    spDebug(10, "editWindow", "in: edit_type = %d\n", edit_type);
    
    if (edit_type != SW_EDIT_BIT_CONV && edit_type != SW_EDIT_SAMP_RATE_CONV
	&& edit_type != SW_EDIT_MONAURALIZE && edit_type != SW_EDIT_CHANGE_VALUE
	&& edit_type != SW_EDIT_PASTE && edit_type != SW_EDIT_MIX
	&& edit_type != SW_EDIT_INSERT && edit_type != SW_EDIT_INSERT_PAUSE) {
	/* get edge */
	if (swGetEdge(window, st, ed, &offset, &length) != SP_TRUE) {
	    spDisplayError(window->window, SW_ERROR_TITLE, SW_REGION_ERROR_MESSAGE);
	    return SP_FALSE;
	}
    }
    
    if (edit_type == SW_EDIT_PASTE || edit_type == SW_EDIT_MIX
	|| edit_type == SW_EDIT_INSERT || edit_type == SW_EDIT_REPLACE) {
	int clipboard_num_channel;
	double clipboard_samp_rate;
	
	if (window->config->toplevel->clipboard_window == NULL
	    || swIsWaveNone(window->config->toplevel->clipboard_window->wave) == SP_TRUE) {
	    spDisplayError(window->window, SW_ERROR_TITLE, SW_CB_NOT_FOUND_ERROR_MESSAGE);
	    return SP_FALSE;
	}
	if (swIsWaveProcessing(window->config->toplevel->clipboard_window->wave) == SP_TRUE
	    && swIsWavePlaying(window->config->toplevel->clipboard_window->wave) == SP_FALSE) {
	    spDisplayError(window->window, SW_ERROR_TITLE, SW_CB_PROCESSING_ERROR_MESSAGE);
	    return SP_FALSE;
	}
	
	clipboard_num_channel = swGetWaveNumChannel(window->config->toplevel->clipboard_window->wave);
	
	if (clipboard_num_channel != 1
	    && clipboard_num_channel != swGetWaveNumChannel(window->wave)) {
	    spDisplayError(window->window, SW_ERROR_TITLE, SW_CB_DIFFERENT_CHANNEL_ERROR_MESSAGE);
	    return SP_FALSE;
	}
	
	clipboard_samp_rate = swGetWaveSampleRate(window->config->toplevel->clipboard_window->wave);
	if (FABS(clipboard_samp_rate - swGetWaveSampleRate(window->wave)) > 100.0) {
	    spDisplayError(window->window, SW_ERROR_TITLE, SW_CB_DIFFERENT_SAMP_RATE_ERROR_MESSAGE);
	    return SP_FALSE;
	}
	spDebug(10, "editWindow", "target_channel = %d\n", window->target_channel);
    }
    
    if (edit_type == SW_EDIT_AMPLIFY
	&& checkAmplitude(window, value) == SP_FALSE) {
	return SP_FALSE;
    }
    
    if (edit_type == SW_EDIT_CROP) {
	/* crop wave */
	wave = swCropWave(window->wave, offset, length);
    } else if (edit_type == SW_EDIT_DELETE) {
	if (length >= window->wave->total_length) {
	    spDisplayError(window->window, SW_ERROR_TITLE, SW_DELETE_WHOLE_ERROR_MESSAGE);
	    flag = SP_FALSE;
	} else {
	    /* delete wave */
	    wave = swDeleteWave(window->wave, offset, length);
	}
    } else {
	if (edit_type == SW_EDIT_ERASE) {
	    /* erase wave */
	    wave = swEraseWave(window->wave, offset, length);
	} else if (edit_type == SW_EDIT_AMPLIFY) {
	    /* amplify wave */
	    wave = swAmplifyWave(window->wave, offset, length, value);
	} else if (edit_type == SW_EDIT_FADE_IN) {
	    /* fade in */
	    wave = swFadeInWave(window->wave, offset, length);
	} else if (edit_type == SW_EDIT_FADE_OUT) {
	    /* fade out */
	    wave = swFadeOutWave(window->wave, offset, length);
	} else if (edit_type == SW_EDIT_CHANNEL_SWAP) {
	    /* amplify wave */
	    wave = swSwapWaveChannel(window->wave, offset, length);
	} else if (edit_type == SW_EDIT_BIT_CONV) {
	    wave = swConvertWaveBit(window->wave, samp_bit);
	} else if (edit_type == SW_EDIT_SAMP_RATE_CONV) {
	    wave = swConvertWaveSampFreq(window->wave, samp_rate);
	} else if (edit_type == SW_EDIT_MONAURALIZE) {
	    wave = swMonauralizeWave(window->wave);
	} else if (edit_type == SW_EDIT_CHANGE_VALUE) {
	    wave = swChangeWaveValue(window->wave, (int)st, ed, value);
	} else if (edit_type == SW_EDIT_PASTE) {
	    wave = swPasteWave(window->wave, window->config->toplevel->clipboard_window->wave,
			       st, window->target_channel);
	} else if (edit_type == SW_EDIT_MIX) {
	    wave = swMixWave(window->wave, window->config->toplevel->clipboard_window->wave,
			     st, window->target_channel);
	} else if (edit_type == SW_EDIT_INSERT) {
	    wave = swInsertWave(window->wave, window->config->toplevel->clipboard_window->wave,
				st, window->target_channel);
	} else if (edit_type == SW_EDIT_INSERT_PAUSE) {
	    wave = swInsertPauseWave(window->wave, st, ed);
	} else if (edit_type == SW_EDIT_REPLACE) {
	    wave = swReplaceWave(window->wave, window->config->toplevel->clipboard_window->wave, offset, length);
	}
    }

    spDebug(10, "editWindow", "done: flag = %d\n", flag);
    
    return flag;
}

spBool swEditWindow(swWindow window, swEditType edit_type,
		    spLong st, spLong ed, double value)
{
    return editWindow(window, edit_type, st, ed, value, 0, 0.0);
}

void swCropWindowCB(spComponent component, swWindow window)
{
    if (window != NULL) {
	swEditWindow(window, SW_EDIT_CROP, window->sel_st, window->sel_ed, 0.0);
    }
    return;
}

void swDeleteWindowCB(spComponent component, swWindow window)
{
    if (window != NULL) {
	swEditWindow(window, SW_EDIT_DELETE, window->sel_st, window->sel_ed, 0.0);
    }
    return;
}

void swEraseWindowCB(spComponent component, swWindow window)
{
    if (window != NULL) {
	swEditWindow(window, SW_EDIT_ERASE, window->sel_st, window->sel_ed, 0.0);
    }
    return;
}

void swInvertWindowCB(spComponent component, swWindow window)
{
    if (window != NULL) {
	swEditWindow(window, SW_EDIT_AMPLIFY, window->sel_st, window->sel_ed, -1.0);
    }
    return;
}

void swFadeInWindowCB(spComponent component, swWindow window)
{
    if (window != NULL) {
	swEditWindow(window, SW_EDIT_FADE_IN, window->sel_st, window->sel_ed, 0.0);
    }
    return;
}

void swFadeOutWindowCB(spComponent component, swWindow window)
{
    if (window != NULL) {
	swEditWindow(window, SW_EDIT_FADE_OUT, window->sel_st, window->sel_ed, 0.0);
    }
    return;
}

void swSwapWaveChannelCB(spComponent component, swWindow window)
{
    if (window != NULL) {
	if (window->wave != NULL && window->wave->num_channel >= 2) {
	    swEditWindow(window, SW_EDIT_CHANNEL_SWAP, window->sel_st, window->sel_ed, 0.0);
	}
    }
    return;
}

void swChangeWindowValue(swWindow window, int channel, spLong point, double value)
{
    if (window != NULL) {
	swEditWindow(window, SW_EDIT_CHANGE_VALUE, channel, point, value);
    }
    return;
}

spBool swConvertWindow(swWindow window, int samp_bit, double samp_rate)
{
    if (samp_bit > 0) {
	return editWindow(window, SW_EDIT_BIT_CONV, 0, 0, 0.0, samp_bit, samp_rate);
    } else {
	return editWindow(window, SW_EDIT_SAMP_RATE_CONV, 0, 0, 0.0, samp_bit, samp_rate);
    }
}

spBool swConvertBit(swWindow window, int samp_bit)
{
    return swConvertWindow(window, samp_bit, 0.0);
}

spBool swConvertSampFreq(swWindow window, double samp_rate)
{
    return swConvertWindow(window, 0, samp_rate);
}

void swMonauralizeCB(spComponent component, swWindow window)
{
    if (window != NULL) {
	if (window->wave != NULL && window->wave->num_channel >= 2) {
	    editWindow(window, SW_EDIT_MONAURALIZE, 0, 0, 0.0, 0, 0.0);
	}
    }
    return;
}

#ifdef SW_SUPPORT_CLIPBOARD
spBool swCopyWindow(swWindow window, spLong st, spLong ed)
{
    spLong offset;
    spLong length;
    swWave wave;
    swWindow clipboard;
    
    if (swIsNoWave(window) == SP_TRUE) return SP_FALSE;

    /* get edge */
    if (swGetEdge(window, st, ed, &offset, &length) != SP_TRUE) {
	spDisplayError(window->window, SW_ERROR_TITLE, SW_REGION_ERROR_MESSAGE);
	return SP_FALSE;
    }

    /* get new wave */
    if ((wave = swExtractWave(window->wave, offset, length)) == NULL) {
	spDisplayError(window->window, SW_ERROR_TITLE, SW_EDIT_ERROR_MESSAGE);
	return SP_FALSE;
    }

    clipboard = window->config->toplevel->clipboard_window;
    swSetWave(clipboard, wave);
    
    /* extract label */
    swExtractLabel(clipboard, window, offset, length);
    
    /* reload wave */
    swReloadWave(clipboard, clipboard->wave, SP_TRUE, SP_FALSE);
    swDrawOverview(clipboard, SP_TRUE);

    /* set window title */
    swSetWindowTitle(clipboard);
    
    swSetSenseLevel(clipboard);
    
    return SP_TRUE;
}

void swCopyWindowCB(spComponent component, swWindow window)
{
    if (window != NULL) {
	swCopyWindow(window, window->sel_st, window->sel_ed);
    }
    return;
}

void swCutWindowCB(spComponent component, swWindow window)
{
    if (window != NULL) {
	swCopyWindow(window, window->sel_st, window->sel_ed);
	swEditWindow(window, SW_EDIT_DELETE, window->sel_st, window->sel_ed, 0.0);
    }
    return;
}

void swPasteWindowCB(spComponent component, swWindow window)
{
    if (window != NULL) {
	swEditWindow(window, SW_EDIT_PASTE, window->point, 0, 0.0);
    }
    return;
}

void swMixWindowCB(spComponent component, swWindow window)
{
    if (window != NULL) {
	swEditWindow(window, SW_EDIT_MIX, window->point, 0, 0.0);
    }
    return;
}

void swInsertWindowCB(spComponent component, swWindow window)
{
    if (window != NULL) {
	swEditWindow(window, SW_EDIT_INSERT, window->point, 0, 0.0);
    }
    return;
}

void swCatWindowCB(spComponent component, swWindow window)
{
    if (window != NULL && window->wave != NULL) {
	swEditWindow(window, SW_EDIT_INSERT, window->wave->total_length, 0, 0.0);
    }
    return;
}

void swCatTopWindowCB(spComponent component, swWindow window)
{
    if (window != NULL) {
	swEditWindow(window, SW_EDIT_INSERT, 0, 0, 0.0);
    }
    return;
}

void swReplaceWindowCB(spComponent component, swWindow window)
{
    if (window != NULL) {
	swEditWindow(window, SW_EDIT_REPLACE, window->sel_st, window->sel_ed, 0.0);
    }
    return;
}
#endif
#endif
