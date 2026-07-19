/*
 *	swLabel.h
 */

#ifndef __SWLABEL_H
#define __SWLABEL_H

#include "swWave.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SW_MAX_LABEL_STRING 512
#define SW_NUM_LABEL_BUFFER 16

typedef enum {
    SW_LABEL_TYPE_UNKNOWN = -1,
    SW_LABEL_TYPE_ALL = 0,
    SW_LABEL_TYPE_NORMAL = 1,
    SW_LABEL_TYPE_REGION = 2,
} swLabelType;

struct _swLabel {
    long index;
    double time;
    double end_time;
    int channel;
    char data[SW_MAX_LABEL_STRING];
    char string[SW_MAX_LABEL_STRING];
};

struct _swLabels {
    char *filename;
    char *region_filename;
    swTimeFormat format;
    long num_buffer;
    long num_label;
    long num_region;
    swLabel *label;

    spBool edit_flag;
    spBool region_edit_flag;

    spBool saved_before_flag;
    spBool region_saved_before_flag;
};

#if defined(MACOS)
#pragma import on
#endif

/* the folloing 6 functions are a kind of access functions,
   so it doesn't check invalid memory access */
extern spBool swIsRegionLabel(swWave wave, long index);
extern void swSetLabelTime(swWave wave, long index, double time);
extern void swSetLabelStartTime(swWave wave, long index, double time);
extern void swSetLabelEndTime(swWave wave, long index, double end_time);
extern void swSetLabelChannel(swWave wave, long index, int channel);
extern double swGetLabelStartTime(swWave wave, long index);
extern double swGetLabelEndTime(swWave wave, long index);
extern int swGetLabelChannel(swWave wave, long index);
    
extern long swGetNumLabelBuffer(swWave wave);
extern long swIsLabelValid(swWave wave, long index);
extern spBool swIsLabelNone(swWave wave);
extern spBool swChangeLabelTime(swWave wave, long index, double time, double end_time);
extern long swAddChannelLabel(swWave wave, double time, double end_time, int channel,
			      char *data, char *string);
extern long swAddLabel(swWave wave, double time, double end_time,
		       char *data, char *string);
extern long swChangeChannelLabel(swWave wave, long index, double time, double end_time, int channel,
				 char *data, char *string);
extern long swChangeLabel(swWave wave, long index, double time, double end_time,
			  char *data, char *string);
extern spBool swReadLabel(swWave wave, const char *filename, swTimeFormat format, spBool region_flag);
extern spBool swWriteLabel(swWave wave, const char *name, const char *filename, swTimeFormat format, spBool region_flag);
extern spBool swCopyLabels(swWave dest_wave, swWave src_wave);
extern long swGetNumRegionLabel(swWave wave);
extern long swGetNumNormalLabel(swWave wave);
extern long swGetNumLabel(swWave wave);
extern long swGetNumChannelLabel(swWave wave, int channel, spBool region_flag);

extern long swFindNeighborLabelIndex(swWave wave, long current_index, spBool prev_flag);
    
extern swLabels swAllocLabels(swWaveConfig config, spBool region_flag);
extern void swFreeLabels(swLabels labels);
extern swLabels swReallocLabels(swLabels labels, swWaveConfig config, spBool region_flag);
extern void swClearLabel(swLabels labels, long index);
extern void swClearLabels(swLabels labels, swLabelType label_type);
extern void swSortLabels(swLabels labels, long *converted_index);
extern void swSetLabelFormat(swLabels labels, swTimeFormat format);
extern void swSetRegionLabelFileName(swLabels labels, const char *filename);
extern void swSetLabelFileName(swLabels labels, const char *filename);
extern char *swGetRegionLabelFileName(swLabels labels);
extern char *swGetLabelFileName(swLabels labels);
extern spBool swIsRegionLabelEdited(swLabels labels);
extern spBool swIsLabelEdited(swLabels labels);
extern spBool swHasRegionLabelSavedBefore(swLabels labels);
extern spBool swHasLabelSavedBefore(swLabels labels);

extern spBool swIsTimeFormatWithData(swTimeFormat swformat);
extern spTimeFormat swConvertTimeFormatFrom(swTimeFormat swformat);
extern swTimeFormat swConvertTimeFormatTo(spTimeFormat format, spBool with_data);
extern spBool swGetTimeString(double sec, swTimeFormat swformat, char *buf);
extern spBool swConvertTimeString(char *buf, swTimeFormat swformat, double *sec);
extern void swAddDefaultLabelFileSuffix(swWaveConfig config, char *buf, int buf_size,
                                        char *orig_filename, spBool remove_orig_suffix, spBool force_flag, spBool region_flag);

#if defined(MACOS)
#pragma import off
#endif
    
#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration */
#endif

#endif /* __SWLABEL_H */
