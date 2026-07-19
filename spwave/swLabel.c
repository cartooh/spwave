/*
 *	swLabel.c
 *
 *	Last modified: <2025-01-19 11:25:28 hideki>
 */

#include <stdio.h>
#include <stdlib.h>

#include <sp/spBaseLib.h>
#include <sp/spComponentLib.h>

#include "swLabel.h"

spBool swIsRegionLabel(swWave wave, long index)
{
    return (swGetLabelEndTime(wave, index) >= 0.0 ? SP_TRUE: SP_FALSE);
}

void swSetLabelTime(swWave wave, long index, double time)
{
    wave->labels->label[index].time = time;
    wave->labels->edit_flag = SP_TRUE;
    return;
}

void swSetLabelStartTime(swWave wave, long index, double time)
{
    wave->labels->label[index].time = time;
    wave->labels->region_edit_flag = SP_TRUE;
    return;
}

void swSetLabelEndTime(swWave wave, long index, double end_time)
{
    wave->labels->label[index].end_time = end_time;
    wave->labels->region_edit_flag = SP_TRUE;
    return;
}

void swSetLabelChannel(swWave wave, long index, int channel)
{
    wave->labels->label[index].channel = channel;
    wave->labels->edit_flag = SP_TRUE;
    return;
}

double swGetLabelStartTime(swWave wave, long index)
{
    return wave->labels->label[index].time;
}

double swGetLabelEndTime(swWave wave, long index)
{
    return wave->labels->label[index].end_time;
}

int swGetLabelChannel(swWave wave, long index)
{
    return wave->labels->label[index].channel;
}

long swGetNumLabelBuffer(swWave wave)
{
    if (wave == NULL || wave->labels == NULL) return -1;
    
    return wave->labels->num_buffer;
}

long swIsLabelValid(swWave wave, long index)
{
    if (wave == NULL || wave->labels == NULL
	|| index < 0 || index >= wave->labels->num_buffer 
	|| wave->labels->label[index].time < 0.0) {
	return SP_FALSE;
    }

    return SP_TRUE;
}

spBool swIsLabelNone(swWave wave)
{
    if (swIsWaveNone(wave) == SP_TRUE
	|| wave->labels == NULL || wave->labels->num_label <= 0) {
	return SP_TRUE;
    } else {
	return SP_FALSE;
    }
}

spBool swChangeLabelTime(swWave wave, long index, double time, double end_time)
{
    if (wave == NULL || wave->labels == NULL
	|| index < 0 || index >= wave->labels->num_label)
	return SP_FALSE;

    wave->labels->label[index].time = time;
    wave->labels->label[index].end_time = end_time;
    
    if (end_time >= 0.0) {
	wave->labels->region_edit_flag = SP_TRUE;
    } else {
	wave->labels->edit_flag = SP_TRUE;
    }
    
    return SP_TRUE;
}

long swAddChannelLabel(swWave wave, double time, double end_time, int channel,
		       char *data, char *string)
{
    long k;
    long index;
    spBool region_flag;

    if (wave == NULL || time < 0.0) {
	return -1;
    }
    
    region_flag = (end_time >= 0.0 ? SP_TRUE : SP_FALSE);
    
    if (wave->labels == NULL) {
	wave->labels = swAllocLabels(wave->config, region_flag);
	index = 0;
    } else {
	index = -1;
	for (k = 0; k < wave->labels->num_buffer; k++) {
	    if (index < 0 && wave->labels->label[k].time < 0.0) {
		index = k;
	    }
#if 0
	    if (wave->labels->label[k].time == time
		&& wave->labels->label[k].end_time == end_time) {
		return -1;
	    }
#endif
	}
	if (index < 0) {
	    wave->labels = swReallocLabels(wave->labels, wave->config, region_flag);
	    index = wave->labels->num_label;
	}
    }

    wave->labels->label[index].index = index;
    wave->labels->label[index].channel = channel;
    wave->labels->label[index].time = time;
    if (end_time >= 0.0) {
	wave->labels->label[index].end_time = end_time;
	wave->labels->num_region++;
    }
    
    spStrCopy(wave->labels->label[index].data, SW_MAX_LABEL_STRING, data);
    spStrCopy(wave->labels->label[index].string, SW_MAX_LABEL_STRING, string);
    spDebug(80, "swAddChannelLabel", "channel = %d, index = %ld, data = '%s', string = '%s'\n",
            channel, index, wave->labels->label[index].data, wave->labels->label[index].string);
        
    wave->labels->num_label++;

    if (region_flag == SP_TRUE) {
	wave->labels->region_edit_flag = SP_TRUE;
    } else {
	wave->labels->edit_flag = SP_TRUE;
    }

    return index;
}

long swAddLabel(swWave wave, double time, double end_time,
		char *data, char *string)
{
    return swAddChannelLabel(wave, time, end_time, -1, data, string);
}

long swChangeChannelLabel(swWave wave, long index, double time, double end_time, int channel,
			  char *data, char *string)
{
    if (wave == NULL || wave->labels == NULL
	|| index < 0 || index >= wave->labels->num_label)
	return -1;

    wave->labels->label[index].time = time;
    wave->labels->label[index].channel = channel;
    wave->labels->label[index].end_time = end_time;
    spStrCopy(wave->labels->label[index].data, SW_MAX_LABEL_STRING, data);
    spStrCopy(wave->labels->label[index].string, SW_MAX_LABEL_STRING, string);

    if (end_time >= 0.0) {
	wave->labels->region_edit_flag = SP_TRUE;
    } else {
	wave->labels->edit_flag = SP_TRUE;
    }
    
    return index;
}

long swChangeLabel(swWave wave, long index, double time, double end_time,
		   char *data, char *string)
{
    return swChangeChannelLabel(wave, index, time, end_time, -1, data, string);
}

static spBool swParseChannelPrefix(char *time_string, swTimeFormat format, int num_channel, int *channel)
{
    char *p;
    
    if (format != SW_TIME_FORMAT_ESPS) {
	p = strchr(time_string, '/');
	if (p == NULL && !(format == SW_TIME_FORMAT_SEPARATED_SEC
			   || format == SW_TIME_FORMAT_SEPARATED_SEC_WITH_DATA)) {
	    p = strchr(time_string, ':');
	}
	
	if (p != NULL && p != time_string) {
	    *p = NUL;
	    *channel = MAX(atoi(time_string) - 1, 0);
	    *channel = MIN(*channel, num_channel - 1);

	    ++p;
	    memmove(time_string, p, strlen(p) + 1);
	    return SP_TRUE;
	}
    }

    return SP_FALSE;
}

spBool swReadLabel(swWave wave, const char *filename, swTimeFormat format, spBool region_flag)
{
    double weight;
    double time, end_time;
    int channel;
    char *p;
    char *next;
    char line[SP_MAX_PATHNAME/*+100*/];
    char buf[SP_MAX_PATHNAME];
    char time_string[SW_MAX_LABEL_STRING];
    char end_time_string[SW_MAX_LABEL_STRING];
    char data_string[SW_MAX_LABEL_STRING];
    char symbol_string[SW_MAX_LABEL_STRING];
    FILE *fp;

    if (wave == NULL) {
	return SP_FALSE;
    }

    /* open file */
    if (NULL == (fp = fopen(filename, "r"))) {
	spDebug(20, "swReadLabel", "can't open file: %s\n", filename);
	return SP_FALSE;
    }

    /* clear labels */
    if (region_flag == SP_TRUE) {
	swClearLabels(wave->labels, SW_LABEL_TYPE_REGION);
    } else {
	swClearLabels(wave->labels, SW_LABEL_TYPE_NORMAL);
    }

    if (format == SW_TIME_FORMAT_UNKNOWN) {
	format = (region_flag == SP_TRUE ? wave->config->region_label_format : wave->config->label_format);
    }
    spDebug(20, "swReadLabel", "format = %d\n", format);

    if (format == SW_TIME_FORMAT_POINT) {
	format = SW_TIME_FORMAT_SEC;
	weight = 1.0 / wave->samp_rate;
    } else {
	weight = 1.0;
    }
    
    if (format == SW_TIME_FORMAT_ESPS) {
	while (fgets((char *)line, SP_MAX_PATHNAME, fp) != NULL) {
	    if (line[0] == '#') {
		break;
	    }
	}
    }

    while (fgetsn(line, SP_MAX_PATHNAME, fp) != NULL) {
	spDebug(50, "swReadLabel", "format = %d, line = '%s', strlen(line) = %d\n",
		format, line, strlen(line));
	
	if (line[0] == '#') continue;

	next = line;

	buf[0] = NUL;
	time_string[0] = NUL;
	end_time_string[0] = NUL;
	data_string[0] = NUL;
	symbol_string[0] = NUL;
	channel = -1;

	next = sgetnextncol(time_string, sizeof(time_string), next);
	    
	spDebug(50, "swReadLabel", "first sgetnextncol: format = %d, next = '%s'\n", format, next);

	swParseChannelPrefix(time_string, format, wave->num_channel, &channel);
	
	if (format == SW_TIME_FORMAT_INDEXED || format == SW_TIME_FORMAT_INDEXED_WITH_DATA) {
	    if (next == NULL) continue;
	    
	    /* ignore first column: time_string */
	    time_string[0] = NUL;
	    
	    next = sgetnextncol(buf, sizeof(buf), next);
	    if ((p = strchr(buf, '-')) != NULL) {
		*p = NUL;
		spStrCopy(time_string, SW_MAX_LABEL_STRING, buf);
		spStrCopy(end_time_string, SW_MAX_LABEL_STRING, p + 1);
	    }
	    spDebug(50, "swReadLabel", "time_string = '%s', end_time_string = '%s', buf = '%s'\n",
		    time_string, end_time_string, buf);
	} else {
	    if (format != SW_TIME_FORMAT_ESPS && region_flag == SP_TRUE) {
		if (next == NULL) continue;
		next = sgetnextncol(end_time_string, sizeof(end_time_string), next);
	    }
	}
	if (next != NULL) {
	    spDebug(50, "swReadLabel", "format = %d, region_flag = %d, next = '%s'\n", format, region_flag, next);
	    spConvertKanjiToLocaleCode((unsigned char *)next, SP_MAX_PATHNAME, SP_KANJI_CODE_UNKNOWN);
	    spDebug(50, "swReadLabel", "format = %d, region_flag = %d, converted next = '%s'\n",
		    format, region_flag, next);
	}
	
	if (format == SW_TIME_FORMAT_ESPS) {
	    if (next == NULL || (next = sgetnextncol(data_string, sizeof(data_string), next)) == NULL) {
		continue;
	    }
	    if (region_flag == SP_TRUE) {
		next = sgetnextncol(end_time_string, sizeof(end_time_string), next);
	    } else {
		next = sgetnextncol(symbol_string, sizeof(symbol_string), next);
	    }
	} else {
	    if (swIsTimeFormatWithData(format) == SP_TRUE && next != NULL) {
		if ((next = sgetnextncol(data_string, sizeof(data_string), next)) != NULL) {
		    spStrCopy(symbol_string, SW_MAX_LABEL_STRING, next);
                    spDebug(50, "swReadLabel", "with data: symbol_string = '%s'\n", symbol_string);
		}
                spDebug(50, "swReadLabel", "with data: data_string = '%s'\n", data_string);
	    } else {
		if (format == SW_TIME_FORMAT_INDEXED || format == SW_TIME_FORMAT_INDEXED_WITH_DATA) {
		    if (next != NULL) {
			spStrCopy(symbol_string, SW_MAX_LABEL_STRING, next);
                        spDebug(50, "swReadLabel", "indexed: symbol_string = '%s'\n", symbol_string);
		    }
		} else {
		    if (next != NULL
			&& (next = sgetnextncol(symbol_string, sizeof(symbol_string), next)) != NULL) {
                        spDebug(50, "swReadLabel", "non-indexed: next = '%s'\n", next);
			spStrCopy(data_string, SW_MAX_LABEL_STRING, next);
		    }
                    spDebug(50, "swReadLabel", "non-indexed: symbol_string = '%s', data_string = '%s'\n",
                            symbol_string, data_string);
		}
	    }
	}
	
	spDebug(50, "swReadLabel", "format = %d, time_string: '%s', data_string: '%s', symbol_string: '%s'\n",
		format, time_string, data_string, symbol_string);

	if (region_flag == SP_TRUE) {
	    if (swConvertTimeString(time_string, format, &time) == SP_TRUE
		&& swConvertTimeString(end_time_string, format, &end_time) == SP_TRUE) {
		time *= weight;
		end_time *= weight;
		swAddChannelLabel(wave, time, end_time, channel, data_string, symbol_string);
	    }
	} else {
	    if (swConvertTimeString(time_string, format, &time) == SP_TRUE) {
		time *= weight;
		swAddChannelLabel(wave, time, -1.0, channel, data_string, symbol_string);
	    }
	}
    }
    spDebug(50, "swReadLabel", "parse file done\n");

    /* close file */
    fclose(fp);

    /* set filename */
    if (region_flag == SP_TRUE) {
	if (swGetNumRegionLabel(wave) <= 0) {
	    return SP_FALSE;
	}
	swSetRegionLabelFileName(wave->labels, filename);
    } else {
	if (swGetNumNormalLabel(wave) <= 0) {
	    return SP_FALSE;
	}
	swSetLabelFileName(wave->labels, filename);
    }
    spDebug(50, "swReadLabel", "set filename done\n");

    /* set format */
    swSetLabelFormat(wave->labels, format);

    spDebug(50, "swReadLabel", "done\n");
    
    return SP_TRUE;
}

spBool swWriteLabel(swWave wave, const char *name, const char *filename, swTimeFormat format, spBool region_flag)
{
    long k;
    long index;
    double weight;
    double prev_time;
    FILE *fp;
    char time_string[SW_MAX_LABEL_STRING];
    char buf[SW_MAX_LABEL_STRING];

    if (wave == NULL || wave->labels == NULL) {
	return SP_FALSE;
    }

    if (strnone(filename)) {
	filename = wave->labels->filename;
    }
    spDebug(20, "swWriteLabel", "filename = %s\n", filename);
    
    
    /* open file */
    if (NULL == (fp = fopen(filename, "w"))) {
	return SP_FALSE;
    }

    /* sort labels */
    swSortLabels(wave->labels, NULL);

    if (format == SW_TIME_FORMAT_POINT) {
	weight = wave->samp_rate;
    } else {
	weight = 1.0;
    }

    if (format == SW_TIME_FORMAT_ESPS) {
	if (strnone(name)) {
	    fprintf(fp, "signal null\n");
	} else {
	    fprintf(fp, "signal %s\n", name);
	}
	fprintf(fp, "type 0\n");
	fprintf(fp, "color 121\n");
	fprintf(fp, "font -misc-*-bold-*-*-*-15-*-*-*-*-*-*-*\n");
	fprintf(fp, "separator ;\n");
	fprintf(fp, "nfields 1\n");
	fprintf(fp, "#\n");
    } else if (format != SW_TIME_FORMAT_INDEXED && format != SW_TIME_FORMAT_INDEXED_WITH_DATA) {
	if (strnone(name)) {
	    fprintf(fp, "# file: null\n");
	} else {
	    fprintf(fp, "# file: %s\n", name);
	}
    }

    spDebug(20, "swWriteLabel", "wave->labels->num_buffer = %ld\n", wave->labels->num_buffer);
    
    index = 1;
    prev_time = -1.0;
    for (k = 0; k < wave->labels->num_buffer; k++) {
	if (wave->labels->label[k].time >= 0.0) {
	    if (region_flag == SP_TRUE) {
		if (wave->labels->label[k].end_time < 0.0) {
		    continue;
		}
	    } else {
		if (wave->labels->label[k].end_time >= 0.0) {
		    continue;
		}
	    }
	    
	    swGetTimeString(weight * wave->labels->label[k].time, format, time_string);
	    spDebug(20, "swWriteLabel", "time_string = '%s'\n", time_string);
	    
	    if (format == SW_TIME_FORMAT_ESPS) {
		fprintf(fp, "    %s  121 ", time_string);
		fputstring(wave->labels->label[k].string, fp);
	    } else {
		if (wave->labels->label[k].channel >= 0) {
		    if (format == SW_TIME_FORMAT_SEPARATED_SEC || format == SW_TIME_FORMAT_SEPARATED_SEC_WITH_DATA) {
			fprintf(fp, "%d/", wave->labels->label[k].channel + 1);
		    } else {
			fprintf(fp, "%d:", wave->labels->label[k].channel + 1);
		    }
		}
		
		if (format == SW_TIME_FORMAT_INDEXED
		    || format == SW_TIME_FORMAT_INDEXED_WITH_DATA) {
		    fprintf(fp, "%04ld %s", index, time_string);
		    if (region_flag == SP_TRUE) {
			putc('-', fp);
			swGetTimeString(weight * wave->labels->label[k].end_time, format, buf);
			spDebug(20, "swWriteLabel", "end_time buf = '%s'\n", buf);
			fputstring(buf, fp);
		    }

		    if (format == SW_TIME_FORMAT_INDEXED_WITH_DATA) {
			putc(' ', fp);
			fputstring(wave->labels->label[k].data, fp);
		    }
		    fprintf(fp, " %s", wave->labels->label[k].string);
		} else {
		    spDebug(20, "swWriteLabel",
			    "wave->labels->label[%ld].string = '%s', wave->labels->label[%ld].data = '%s'\n",
			    k, wave->labels->label[k].string, k, wave->labels->label[k].data);
		
		    fputstring(time_string, fp);
		
		    if (region_flag == SP_FALSE
			&& strnone(wave->labels->label[k].string)
			&& strnone(wave->labels->label[k].data)) {
			/* do nothing */
		    } else {
			if (region_flag == SP_TRUE) {
			    putc(' ', fp);
			    swGetTimeString(weight * wave->labels->label[k].end_time, format, buf);
			    fputstring(buf, fp);
			}

			if (swIsTimeFormatWithData(format) == SP_TRUE) {
			    putc(' ', fp);
			    fputstring(wave->labels->label[k].data, fp);
			    fprintf(fp, " %s", wave->labels->label[k].string);
			} else {
			    putc(' ', fp);
			    fputstring(wave->labels->label[k].string, fp);
			    fprintf(fp, " %s", wave->labels->label[k].data);
			}
		    }
		}
	    }
	    fprintf(fp, "\n");
	    
	    prev_time = wave->labels->label[k].time;
	    index++;
	}
    }

    /* close file */
    fclose(fp);

    /* set filename */
    if (region_flag == SP_TRUE) {
	swSetRegionLabelFileName(wave->labels, filename);
        wave->labels->region_saved_before_flag = SP_TRUE;
    } else {
	swSetLabelFileName(wave->labels, filename);
        wave->labels->saved_before_flag = SP_TRUE;
    }

    /* set format */
    swSetLabelFormat(wave->labels, format);
    
    spDebug(50, "swWriteLabel", "done\n");
    
    return SP_TRUE;
}

spBool swCopyLabels(swWave dest_wave, swWave src_wave)
{
    long k;
    
    if (dest_wave == NULL || src_wave == NULL || src_wave->labels == NULL)
	return SP_FALSE;

    for (k = 0; k < src_wave->labels->num_buffer; k++) {
	swAddChannelLabel(dest_wave, src_wave->labels->label[k].time,
			  src_wave->labels->label[k].end_time,
			  src_wave->labels->label[k].channel,
			  src_wave->labels->label[k].data,
			  src_wave->labels->label[k].string);
    }

    if (k > 0 && dest_wave->labels != NULL) {
	if (src_wave->labels->filename != NULL) {
	    dest_wave->labels->filename = strclone(src_wave->labels->filename);
	}
	if (src_wave->labels->region_filename != NULL) {
	    dest_wave->labels->region_filename = strclone(src_wave->labels->region_filename);
	}
	dest_wave->labels->format = src_wave->labels->format;

	return SP_TRUE;
    } else {
	return SP_FALSE;
    }
}

long swGetNumRegionLabel(swWave wave)
{
    long num = 0;

    if (wave != NULL && wave->labels != NULL) {
	num = wave->labels->num_region;
    }
    spDebug(80, "swGetNumRegionLabel", "num = %ld\n", num);
    
    return num;
}

long swGetNumNormalLabel(swWave wave)
{
    long num = 0;

    if (wave != NULL && wave->labels != NULL) {
	num = wave->labels->num_label - wave->labels->num_region;
    }
    spDebug(80, "swGetNumNormalLabel", "num = %ld\n", num);
    
    return num;
}

long swGetNumLabel(swWave wave)
{
    long num = 0;

    if (wave != NULL && wave->labels != NULL) {
	num = wave->labels->num_label;
    }
    
    return num;
}

long swGetNumChannelLabel(swWave wave, int channel, spBool region_flag)
{
    long k;
    long num = 0;
    swLabels labels;

    if (wave != NULL && wave->labels != NULL) {
	labels = wave->labels;
	
	for (k = 0; k < labels->num_buffer; k++) {
	    if (labels->label[k].time >= 0.0
		&& ((region_flag == SP_TRUE && labels->label[k].end_time >= 0.0)
		    || (region_flag == SP_FALSE && labels->label[k].end_time < 0.0))
		&& labels->label[k].channel == channel) {
		++num;
	    }
	}
    }
    
    return num;
}

long swGetNumChannelNormalLabel(swWave wave, int channel)
{
    long k;
    long num = 0;
    swLabels labels;

    if (wave != NULL && wave->labels != NULL) {
	labels = wave->labels;
	
	for (k = 0; k < labels->num_buffer; k++) {
	    if (labels->label[k].time >= 0.0 && labels->label[k].end_time < 0.0
		&& labels->label[k].channel == channel) {
		++num;
	    }
	}
    }
    
    return num;
}

long swFindNeighborLabelIndex(swWave wave, long current_index, spBool prev_flag)
{
    long k;
    int channel;
    double time;
    double dist;
    double test_time;
    double min_dist;
    double another_time;
    double found_another_time;
    spBool region_flag;
    long index;
    
    if (wave == NULL || wave->labels == NULL)
	return -1;

    region_flag = (wave->labels->label[current_index].end_time >= 0.0 ? SP_TRUE : SP_FALSE);
    
    if (prev_flag == SP_FALSE && region_flag == SP_TRUE) {
	time = wave->labels->label[current_index].end_time;
    } else {
	time = wave->labels->label[current_index].time;
    }
    channel = wave->labels->label[current_index].channel;
    
    index = -1;
    min_dist = 1000000000.0;
    found_another_time = -1.0;
    for (k = 0; k < wave->labels->num_buffer; k++) {
	if (k == current_index || channel != wave->labels->label[k].channel) continue;
	
	if (wave->labels->label[k].time >= 0.0
	    && ((region_flag == SP_FALSE && wave->labels->label[k].end_time < 0.0)
		|| (region_flag == SP_TRUE && wave->labels->label[k].end_time >= 0.0))) {
	    if (prev_flag == SP_TRUE) {
		if (region_flag == SP_TRUE) {
		    test_time = wave->labels->label[k].end_time;
		    another_time = wave->labels->label[k].time;
		} else {
		    test_time = wave->labels->label[k].time;
		    another_time = wave->labels->label[k].end_time;
		}
		dist = time - test_time;
	    } else {
		test_time = wave->labels->label[k].time;
		another_time = wave->labels->label[k].end_time;
		
		dist = test_time - time;
	    }
	    
	    if (dist >= 0.0
		&& (dist < min_dist
		    || (dist == min_dist &&
			((prev_flag == SP_TRUE && another_time > found_another_time)
			 || (prev_flag == SP_FALSE && another_time < found_another_time))))) {
		min_dist = dist;
		if (region_flag == SP_TRUE) {
		    found_another_time = another_time;
		}
		index = k;
	    }
	}
    }

    return index;
}

swLabels swAllocLabels(swWaveConfig config, spBool region_flag)
{
    long k;
    swLabels labels;

    labels = xalloc(1, struct _swLabels);
    labels->filename = NULL;
    labels->region_filename = NULL;
    labels->format = (region_flag == SP_TRUE ? config->region_label_format : config->label_format);
    labels->label = xalloc(SW_NUM_LABEL_BUFFER, swLabel);
    labels->num_buffer = SW_NUM_LABEL_BUFFER;
    labels->num_label = 0;
    labels->num_region = 0;
    labels->edit_flag = SP_FALSE;
    labels->region_edit_flag = SP_FALSE;
    labels->saved_before_flag = SP_FALSE;
    labels->region_saved_before_flag = SP_FALSE;

    for (k = 0; k < labels->num_buffer; k++) {
	labels->label[k].index = k;
	labels->label[k].time = -1.0;
	labels->label[k].end_time = -1.0;
	labels->label[k].channel = -1;
	labels->label[k].data[0] = NUL;
	labels->label[k].string[0] = NUL;
    }

    return labels;
}

void swFreeLabels(swLabels labels)
{
    if (labels != NULL) {
	if (labels->filename != NULL) {
	    xfree(labels->filename);
	}
	if (labels->region_filename != NULL) {
	    xfree(labels->region_filename);
	}
	if (labels->label != NULL) {
	    xfree(labels->label);
	}
	xfree(labels);
    }

    return;
}

swLabels swReallocLabels(swLabels labels, swWaveConfig config, spBool region_flag)
{
    long k;
    long prev_num_buffer;

    if (labels == NULL) {
	labels = swAllocLabels(config, region_flag);
	return labels;
    }

    labels = labels;
    /*labels->num_label = labels->num_buffer;*/
    prev_num_buffer = labels->num_buffer;
    labels->num_buffer += SW_NUM_LABEL_BUFFER;
    labels->label = xrealloc(labels->label, labels->num_buffer, swLabel);

    for (k = prev_num_buffer; k < labels->num_buffer; k++) {
	labels->label[k].index = k;
	labels->label[k].time = -1.0;
	labels->label[k].end_time = -1.0;
	labels->label[k].channel = -1;
	labels->label[k].data[0] = NUL;
	labels->label[k].string[0] = NUL;
    }

    return labels;
}

void swClearLabel(swLabels labels, long index)
{
    if (labels == NULL) return;

    if (labels->label[index].end_time >= 0.0) {
	labels->num_region--;
	labels->region_edit_flag = SP_TRUE;
    } else {
	labels->edit_flag = SP_TRUE;
    }
    labels->label[index].index = index;
    labels->label[index].time = -1.0;
    labels->label[index].end_time = -1.0;
    labels->label[index].channel = -1;
    labels->label[index].data[0] = NUL;
    labels->label[index].string[0] = NUL;
    
    labels->num_label--;

    return;
}

void swClearLabels(swLabels labels, swLabelType label_type)
{
    long k;

    if (labels == NULL) return;

    for (k = 0; k < labels->num_buffer; k++) {
	if (labels->label[k].time >= 0.0) {
	    if ((label_type == SW_LABEL_TYPE_NORMAL
		 && labels->label[k].end_time < 0.0)
		|| (label_type == SW_LABEL_TYPE_REGION
		    && labels->label[k].end_time >= 0.0)
		|| (label_type == SW_LABEL_TYPE_ALL)) {
		swClearLabel(labels, k);
	    }
	}
    }

    if (label_type == SW_LABEL_TYPE_NORMAL || label_type == SW_LABEL_TYPE_ALL) {
	if (labels->filename != NULL) {
	    xfree(labels->filename);
            labels->filename = NULL;
	}
	labels->edit_flag = SP_FALSE;
    }
    if (label_type == SW_LABEL_TYPE_REGION || label_type == SW_LABEL_TYPE_ALL) {
	if (labels->region_filename != NULL) {
	    xfree(labels->region_filename);
            labels->region_filename = NULL;
	}
	labels->region_edit_flag = SP_FALSE;
    }

    return;
}

static int qsort_label_cmp(const void *x_in, const void *y_in)
{
    const swLabel *x;
    const swLabel *y;

    x = (const swLabel *)x_in;
    y = (const swLabel *)y_in;
    
    if ((*x).time < 0.0 && (*y).time < 0.0) {
	return (0);
    } else if ((*x).time < 0.0) {
	return (1);
    } else if ((*y).time < 0.0) {
	return (-1);
    } else if ((*x).time < (*y).time) {
	return (-1);
    } else if ((*x).time > (*y).time) {
	return (1);
    } else {
	return (0);
    }
}

void swSortLabels(swLabels labels, long *converted_index)
{
    int flag;
    long k;
    int (*func)(
#if !defined(MACOS9)
        const void *, const void *
#endif
        );

    if (labels == NULL || labels->num_buffer <= 1)
        return;

    func = qsort_label_cmp;
    qsort(labels->label, (unsigned)labels->num_buffer, sizeof(swLabel), func);

    flag = 0;
    for (k = 0; k < labels->num_buffer; k++) {
	if (!flag && converted_index != NULL && labels->label[k].index == *converted_index) {
	    if (labels->label[k].time < 0) {
		*converted_index = -1;
	    } else {
		*converted_index = k;
	    }
	    flag = 1;
	}
	labels->label[k].index = k;
    }

    return;
}

void swSetLabelFormat(swLabels labels, swTimeFormat format)
{
    if (labels == NULL) return;

    labels->format = format;

    return;
}
    
void swSetRegionLabelFileName(swLabels labels, const char *filename)
{
    char *string;
    
    if (labels == NULL || strnone(filename)) return;

    if (filename != labels->region_filename) {
	string = strclone(filename);
	if (labels->region_filename != NULL) {
	    xfree(labels->region_filename);
	}
	labels->region_filename = string;
    }
    labels->region_edit_flag = SP_FALSE;
    
    return;
}

void swSetLabelFileName(swLabels labels, const char *filename)
{
    char *string;
    
    if (labels == NULL || strnone(filename)) return;

    if (filename != labels->filename) {
	string = strclone(filename);
	if (labels->filename != NULL) {
	    xfree(labels->filename);
	}
	labels->filename = string;
    }
    labels->edit_flag = SP_FALSE;
    
    return;
}

char *swGetRegionLabelFileName(swLabels labels)
{
    if (labels == NULL) return NULL;

    return labels->region_filename;
}
    
char *swGetLabelFileName(swLabels labels)
{
    if (labels == NULL) return NULL;

    return labels->filename;
}

spBool swIsRegionLabelEdited(swLabels labels)
{
    if (labels == NULL) return SP_FALSE;

    return labels->region_edit_flag;
}
    
spBool swIsLabelEdited(swLabels labels)
{
    if (labels == NULL) return SP_FALSE;

    return labels->edit_flag;
}
    
spBool swHasRegionLabelSavedBefore(swLabels labels)
{
    if (labels == NULL) return SP_FALSE;

    return labels->region_saved_before_flag;
}
    
spBool swHasLabelSavedBefore(swLabels labels)
{
    if (labels == NULL) return SP_FALSE;

    return labels->saved_before_flag;
}
    
spBool swIsTimeFormatWithData(swTimeFormat swformat)
{
    if ((swformat >= SW_TIME_FORMAT_SEC_WITH_DATA && swformat <= SW_TIME_FORMAT_FLOORED_MSEC_WITH_DATA)
	|| swformat == SW_TIME_FORMAT_INDEXED_WITH_DATA) {
	return SP_TRUE;
    } else {
	return SP_FALSE;
    }
}

spTimeFormat swConvertTimeFormatFrom(swTimeFormat swformat)
{
    spTimeFormat format;

    switch (swformat) {
      case SW_TIME_FORMAT_MSEC:
      case SW_TIME_FORMAT_MSEC_WITH_DATA:
	format = SP_TIME_FORMAT_MSEC;
	break;
      case SW_TIME_FORMAT_FLOORED_MSEC:
      case SW_TIME_FORMAT_FLOORED_MSEC_WITH_DATA:
	format = SP_TIME_FORMAT_FLOORED_MSEC;
	break;

      case SW_TIME_FORMAT_SEC:
      case SW_TIME_FORMAT_SEC_WITH_DATA:
      case SW_TIME_FORMAT_ESPS:
	format = SP_TIME_FORMAT_SEC;
	break;
      case SW_TIME_FORMAT_SEPARATED_SEC:
      case SW_TIME_FORMAT_SEPARATED_SEC_WITH_DATA:
      case SW_TIME_FORMAT_INDEXED:
      case SW_TIME_FORMAT_INDEXED_WITH_DATA:
	format = SP_TIME_FORMAT_SEPARATED_SEC;
	break;
	
      case SW_TIME_FORMAT_POINT:
      case SW_TIME_FORMAT_POINT_WITH_DATA:
	format = SP_TIME_FORMAT_POINT;
	break;
	
      default:
	format = SP_TIME_FORMAT_UNKNOWN;
	break;
    }

    return format;
}

swTimeFormat swConvertTimeFormatTo(spTimeFormat format, spBool with_data)
{
    swTimeFormat swformat;

    switch (format) {
      case SP_TIME_FORMAT_MSEC:
	swformat = (with_data == SP_TRUE ? SW_TIME_FORMAT_MSEC_WITH_DATA : SW_TIME_FORMAT_MSEC);
	break;
      case SP_TIME_FORMAT_FLOORED_MSEC:
	swformat = (with_data == SP_TRUE ? SW_TIME_FORMAT_FLOORED_MSEC_WITH_DATA
		    : SW_TIME_FORMAT_FLOORED_MSEC);
	break;
	
      case SP_TIME_FORMAT_SEC:
      case SP_TIME_FORMAT_FLOORED_SEC:
	swformat = (with_data == SP_TRUE ? SW_TIME_FORMAT_SEC_WITH_DATA : SW_TIME_FORMAT_SEC);
	break;
      case SP_TIME_FORMAT_SEPARATED_SEC:
      case SP_TIME_FORMAT_FLOORED_SEPARATED_SEC:
	swformat = (with_data == SP_TRUE ? SW_TIME_FORMAT_SEPARATED_SEC_WITH_DATA
		    : SW_TIME_FORMAT_SEPARATED_SEC);
	break;
	
      case SP_TIME_FORMAT_POINT:
	swformat = (with_data == SP_TRUE ? SW_TIME_FORMAT_POINT_WITH_DATA : SW_TIME_FORMAT_POINT);
	break;
	
      default:
	swformat = SW_TIME_FORMAT_UNKNOWN;
	break;
    }

    return swformat;
}

spBool swGetTimeString(double sec, swTimeFormat swformat, char *buf)
{
    return spGetTimeString(sec, swConvertTimeFormatFrom(swformat), buf);
}

spBool swConvertTimeString(char *buf, swTimeFormat swformat, double *sec)
{
    if (spConvertTimeString(buf, swConvertTimeFormatFrom(swformat), sec) == SP_TIME_FORMAT_UNKNOWN) {
	return SP_FALSE;
    } else {
	return SP_TRUE;
    }
}

void swAddDefaultLabelFileSuffix(swWaveConfig config, char *buf, int buf_size,
                                 char *orig_filename, spBool remove_orig_suffix, spBool force_flag, spBool region_flag)
{
    int len;
    char *default_suffix_ptr;
    
    default_suffix_ptr = (region_flag == SP_TRUE ? config->default_region_label_suffix
                          : config->default_label_suffix);

    spStrCopy(buf, buf_size, orig_filename);
    if (remove_orig_suffix == SP_TRUE) {
        spRemoveSuffix(buf, NULL);
    }
    
    if (!strnone(default_suffix_ptr)) {
        len = (int)strlen(buf);
            
        if (force_flag == SP_TRUE || (len <= 5 || !(buf[len - 4] == '.' || buf[len - 5] == '.'))) {
            spReplaceSuffix(buf, default_suffix_ptr);
        }
    }

    return;
}
