/*
 *	swEdit.h
 */

#ifndef __SWEDIT_H
#define __SWEDIT_H

#include "swWindow.h"
#include "swWave.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(MACOS)
#pragma import on
#endif

extern spBool swGetEdge(swWindow window, spLong st, spLong ed, spLong *offset, spLong *length);
extern swWindow swExtractWindowAt(swWindow window, spLong st, spLong ed, spBool autosave_flag, int x, int y);
extern swWindow swExtractWindow(swWindow window, spLong st, spLong ed, spBool autosave_flag);
extern void swExtractWindowCB(spComponent component, swWindow window);
extern void swExtractAutosaveWindowAt(swWindow window, int x, int y);
extern void swExtractAutosaveWindow(swWindow window);
extern void swExtractAutosaveWindowCB(spComponent component, swWindow window);

extern spBool swProcessSaveByLabel(swWindow window, swWave wave, spLong pos, swEditType edit_type);
extern void swPopupSaveByNormalLabelDialogCB(spComponent component, swWindow window);
extern void swPopupSaveByRegionLabelDialogCB(spComponent component, swWindow window);
    
extern void swUndoWindowCB(spComponent component, swWindow window);
extern void swRedoWindowCB(spComponent component, swWindow window);

extern spBool swEditFinish(swWindow window, swWave wave, spBool in_thread, swEditType edit_type);
extern spBool swEditWindow(swWindow window, swEditType edit_type,
			   spLong st, spLong ed, double weight);
extern void swCropWindowCB(spComponent component, swWindow window);
extern void swDeleteWindowCB(spComponent component, swWindow window);
extern void swEraseWindowCB(spComponent component, swWindow window);
extern void swInvertWindowCB(spComponent component, swWindow window);
extern void swFadeInWindowCB(spComponent component, swWindow window);
extern void swFadeOutWindowCB(spComponent component, swWindow window);
extern void swSwapWaveChannelCB(spComponent component, swWindow window);
extern void swChangeWindowValue(swWindow window, int channel, spLong point, double value);
extern spBool swConvertBit(swWindow window, int samp_bit);
extern spBool swConvertSampFreq(swWindow window, double samp_rate);
extern void swMonauralizeCB(spComponent component, swWindow window);

extern void swCopyWindowCB(spComponent component, swWindow window);
extern void swCutWindowCB(spComponent component, swWindow window);
extern void swPasteWindowCB(spComponent component, swWindow window);
extern void swMixWindowCB(spComponent component, swWindow window);
extern void swInsertWindowCB(spComponent component, swWindow window);
extern void swCatWindowCB(spComponent component, swWindow window);
extern void swCatTopWindowCB(spComponent component, swWindow window);
extern void swReplaceWindowCB(spComponent component, swWindow window);

#if defined(MACOS)
#pragma import off
#endif

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration */
#endif

#endif /* __SWEDIT_H */
