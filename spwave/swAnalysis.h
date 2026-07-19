/*
 *	swAnalysis.h
 */

#ifndef __SWANALYSIS_H
#define __SWANALYSIS_H

#include "swWindow.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SW_USE_ANALYSIS
    
typedef struct _swFilteringDialog *swFilteringDialog;
    
#if defined(MACOS)
#pragma import on
#endif

extern swWindow swAnalysisWindow(swWindow window, spLong offset, spLong length, spBool cqt_flag, spBool new_flag);
extern void swAnalysisRegionCB(spComponent component, swWindow window);
extern void swAnalysisFrame(spComponent component, swWindow window, swAnalysisConfigFlag config_flag);
extern void swAnalysisWideCB(spComponent component, swWindow window);
extern void swAnalysisNarrowCB(spComponent component, swWindow window);
extern void swAnalysisCQTCB(spComponent component, swWindow window);
extern void swReanalysisAll(spComponent component, spBool update_fft_specgram, spBool update_lifter_related_specgram, spBool update_cqt_specgram);

extern void swSubplotSpectrogramCB(spComponent component, swWindow window);
    
extern spBool swUpdateSpectrogram(swWindow window, spBool update_subplot);
extern void swUpdateSpectrogramCB(spComponent component, swWindow window);
extern void swClearSpectrogramCB(spComponent component, swWindow window);
extern void swCheckSpectrogramDrawKeysCB(spComponent component, swWindow window);
extern void swDrawWideSpectrogramCB(spComponent component, swWindow window);
extern void swDrawNarrowSpectrogramCB(spComponent component, swWindow window);
extern void swDrawNarrowSmoothedSpectrogramCB(spComponent component, swWindow window);
#if defined(SW_SUPPORT_CQT_SPECTROGRAM)
extern void swDrawCQTSpectrogramCB(spComponent component, swWindow window);
#endif

extern void swPopupFilteringDialogCB(spComponent component, swWindow window);
    
#if defined(MACOS)
#pragma import off
#endif

#endif
    
#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration */
#endif

#endif /* __SWANALYSIS_H */
