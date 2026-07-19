/*
 *	swAnalysisDialog.h
 */

#ifndef __SWANALYSISDIALOG_H
#define __SWANALYSISDIALOG_H

#include "swWindow.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SW_USE_ANALYSIS
    
typedef struct _swAnalysisDialog {
    swConfig config;
    
    spComponent window;
    spComponent analysis_type_combo;
    spComponent window_type_combo;
    spComponent fft_length_combo;
    spComponent max_fft_length_combo;
    spComponent lifter_combo;
    
    spComponent new_window_button;
    spComponent linear_spectrum_button;
    spComponent normalize_spectrum_button;
    spComponent normalize_spectrum_max_button;
    
#if defined(SW_SUPPORT_CQT_ANALYSIS)
    spComponent cqt_bins_per_octave_combo_box;
    spComponent cqt_min_freq_combo_box;
    spComponent cqt_min_freq_based_on_musical_note_check_box;
    spComponent cqt_use_erb_check_box;
    spComponent cqt_block_margin_factor_combo_box;
    spComponent cqt_block_transition_factor_combo_box;
#endif

    spBool fft_spectrogram_parameter_updated;
    spBool lifter_updated;
    spBool cqt_spectrogram_parameter_updated;
} *swAnalysisDialog;

typedef struct _swSpectrogramDialog {
    spComponent dialog;

    spComponent range_track_bar;
    spComponent limit_threshold_track_bar;

    spComponent simplified_check_box;
    spComponent gray_scale_check_box;
    
    swWindow current_window;
    swConfig config;
} *swSpectrogramDialog;
    
extern swAnalysisDialog swCreateAnalysisDialog(swConfig config);
extern void swPopupAnalysisDialogCB(spComponent component, swWindow window);
extern void swPopdownAnalysisDialogCB(spComponent component, swAnalysisDialog analysis_dialog);

extern void swPopupSpectrogramDialogCB(spComponent component, swWindow window);
    
#endif
    
#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration */
#endif

#endif /* __SWANALYSISDIALOG_H */
