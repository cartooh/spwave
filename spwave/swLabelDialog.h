/*
 *	swLabelDialog.h
 */

#ifndef __SWLABELDIALOG_H
#define __SWLABELDIALOG_H

#include "swWindow.h"
#include "swLabel.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _swLabelDialog {
    spComponent window;
    
    spComponent time_field;
    spComponent end_time_field;
    spComponent symbol_field;
    spComponent data_field;
    spComponent channel_field;

    spBool region_flag;
    long current_index;
    swWindow current_window;
} *swLabelDialog;

#if defined(MACOS)
#pragma import on
#endif

extern void swPopupLabelInsertDialog(swWindow, long index);
extern void swPopdownLabelInsertDialog(spComponent component, swLabelDialog);

extern void swOpenLabelCB(spComponent component, swWindow window);
extern void swOpenRegionLabelCB(spComponent component, swWindow window);
extern spBool swSaveLabel(swWindow window, spBool save_as, spBool region_flag);
extern void swSaveLabelCB(spComponent component, swWindow window);
extern void swSaveRegionLabelCB(spComponent component, swWindow window);
extern void swSaveAsLabelCB(spComponent component, swWindow window);
extern void swSaveAsRegionLabelCB(spComponent component, swWindow window);
extern void swClearLabelCB(spComponent component, swWindow window);
extern void swClearRegionLabelCB(spComponent component, swWindow window);

extern void swInsertChannelLabel(swWindow window, double point_f, int channel, char *string);
extern void swInsertLabelCB(spComponent component, void *data);
extern void swChangeLabelCB(spComponent component, void *data);
extern void swInsertSimpleLabelCB(spComponent component, swWindow window);
extern void swInsertValueLabelCB(spComponent component, swWindow window);
extern spBool swEraseLabelPrompt(spComponent component, swWindow window);
extern void swEraseLabelIndex(spComponent component, swWindow window, long label_index);
extern void swEraseActiveLabel(swWindow window);
extern void swEraseLabelCB(spComponent component, swWindow window);
extern void swEraseLabelRegionCB(spComponent component, swWindow window);
extern spBool swGetLabelEdge(swWindow window, long index, int *channel, spLong *st, spLong *ed);
extern void swSelectBetweenLabelsCB(spComponent component, swWindow window);
extern void swSetRegionLabelCB(spComponent component, swWindow window);
extern spBool swSetLabelAsRegion(swWindow window, long index);
extern void swSetRegionLabelAsRegionCB(spComponent component, swWindow window);

extern void swDivideRegionLabelCB(spComponent component, swWindow window);
extern void swCatRegionLabels(swWindow window, long index, spBool end_is_nearer);
extern void swCatRegionLabelsCB(spComponent component, swWindow window);
    
extern spBool swFindIdenticalLabelIndex(swWindow window, double time, 
					spBool region_flag, long *pindex, 
					spBool *start_flag, spBool *end_flag);
extern spBool swReplaceIdenticalLabelIndex(swWindow window, spBool drag_end_flag,
					   long orig_index, double target_time);
    
extern long swFindChannelLabelIndex(swWindow window, double time, int channel, int direction,
				    spBool normal_only, spBool region_only,
				    spBool *end_is_nearer, spLong *pmin_dist_l);
extern long swFindLabelIndex(swWindow window, double time, int direction,
			     spBool normal_only, spBool region_only,
			     spBool *end_is_nearer, spLong *pmin_dist_l);
extern long swFindNearChannelLabelIndex(swWindow window, double time, int channel,
					spBool region_only, spBool *end_is_nearer);
extern long swFindNearLabelIndex(swWindow window, double time, spBool region_only, spBool *end_is_nearer);
extern spBool swEraseLabel(swWindow window, double time);
extern spBool swCropLabel(swWindow window, spLong offset, spLong length);
extern spBool swDeleteLabel(swWindow window, spLong offset, spLong length);
extern spBool swExtractLabel(swWindow newwindow, swWindow window,
			     spLong offset, spLong length);
extern spBool swEraseLabelRegion(swWindow window, spLong offset, spLong length);

extern spBool swSetLabelCaps(swWindow window, swLabelCaps caps);
    
#if defined(MACOS)
#pragma import off
#endif

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration */
#endif

#endif /* __SWLABELDIALOG_H */
