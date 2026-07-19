/*
 *	swAnalysisDialog.c
 *
 *	Last modified: <2025-04-25 01:48:43 hideki>
 */

#include <stdio.h>
#include <stdlib.h>

#include <sp/spBaseLib.h>
#include <sp/spComponentLib.h>

#include "swWindow.h"
#include "swDraw.h"
#include "swCursor.h"
#include "swEdit.h"

#ifdef SW_USE_ANALYSIS
#include "swAnalysis.h"
#include "swAnalysisDialog.h"

static char *analysis_type_list[] =
{
    SW_ANALYSIS_SPECTRUM_LABEL,
    SW_ANALYSIS_SMOOTHED_SPECTRUM_LABEL,
    SW_ANALYSIS_PHASE_LABEL,
    SW_ANALYSIS_UNWRAPPED_PHASE_LABEL,
    SW_ANALYSIS_GROUP_DELAY_LABEL,
    SW_ANALYSIS_CEPSTRUM_LABEL,
    SW_ANALYSIS_SMOOTHED_GROUP_DELAY_LABEL,
    SW_ANALYSIS_TD_GROUP_DELAY_LABEL,
    NULL,
};
static char *window_type_list[] =
{
    SW_WINDOW_RECTANGLE_LABEL,
    SW_WINDOW_HAMMING_LABEL,
    SW_WINDOW_HANNING_LABEL,
    SW_WINDOW_BLACKMAN_LABEL,
    SW_WINDOW_GAUSS_LABEL,
    NULL,
};
static char *fft_length_list[] =
{
    "128",
    "256",
    "512",
    "1024",
    "2048",
    "4096",
    "8192",
    "16384",
    NULL,
};
static char *max_fft_length_list[] =
{
    "2048",
    "4096",
    "8192",
    "16384",
    "16384",
    "32768",
    "65536",
    "131072",
    "262144",
    "524288",
    NULL,
};
static char *lifter_list[] =
{
    "0.5",
    "1.0",
    "1.5",
    "2.0",
    "2.5",
    "3.0",
    "4.0",
    "5.0",
    "6.0",
    NULL,
};

#if defined(SW_SUPPORT_CQT_ANALYSIS)
static void swGetCurrentCQTParameterValues(swAnalysisDialog analysis_dialog);
static void swCreateCQTParameterTab(swAnalysisDialog analysis_dialog, spComponent parent);
#endif

static swAnalysisDialog createAnalysisDialog(swConfig config)
{
    swAnalysisDialog analysis_dialog;
    spComponent tab_box;
    spComponent main_tab, attr_tab;
    char string[SP_MAX_LINE];
    
    analysis_dialog = xalloc(1, struct _swAnalysisDialog);
    memset(analysis_dialog, 0, sizeof(struct _swAnalysisDialog));
    analysis_dialog->config = config;
    
    analysis_dialog->window = spCreateDialogBox("analysisDialog",
						SppTitle, SW_ANALYSIS_DIALOG_TITLE,
						SppCallbackFunc, swPopdownAnalysisDialogCB,
						SppCallbackData, analysis_dialog,
						SppDialogBoxButtonType, SP_DB_OK_CANCEL_APPLY,
						SppPopupStyle, SP_MODELESS_POPUP,
						SppCloseStyle, SP_UNMAP_CLOSE,
						SppSpacingOn, SP_TRUE,
						SppHelpButtonVisible, SP_TRUE,
						SppHelpPath, "dialog/analysis.html",
						NULL);
    
    /* create tab box */
    tab_box = spCreateTabBox(analysis_dialog->window, "analysisTabBox", 380,
			     NULL);
    
    /* create main tab */
    main_tab = spAddTabItem(tab_box, "analysisMainTab", -1,
			    SppTitle, SW_ANALYSIS_PARAMETER_LABEL,
			    SppHelpPath, "dialog/analysis.html#analysis_parameter",
			    NULL);
    
    /* create combo box to select analysis type */
    analysis_dialog->analysis_type_combo = spCreateParamField(main_tab, "analysisTypeComboBox", 60,
							      SppTitle, SW_ANALYSIS_TYPE_LABEL,
							      SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
							      SppEditable, SP_FALSE,
							      SppFieldStrings, analysis_type_list,
							      SppFieldOffset, 150,
							      SppFieldSize, 160,
							      SppHelpPath, "dialog/analysis.html#analysis_type",
							      NULL);
    spSelectListItem(analysis_dialog->analysis_type_combo,
		     swGetAnalysisTypeLabel(config->analysis_type));

    /* create combo box to select window type */
    analysis_dialog->window_type_combo = spCreateParamField(main_tab, "windowTypeComboBox", 60,
							    SppTitle, SW_ANALYSIS_WINDOW_TYPE_LABEL,
							    SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
							    SppEditable, SP_FALSE,
							    SppFieldStrings, window_type_list,
							    SppFieldOffset, 150,
							    SppFieldSize, 160,
							    SppHelpPath, "dialog/analysis.html#analysis_window_type",
							    NULL);
    spSelectListIndex(analysis_dialog->window_type_combo, (int)config->window_type);
    
    /* create combo box to input fft length */
    analysis_dialog->fft_length_combo = spCreateParamField(main_tab, "fftLengthComboBox", 60,
							   SppTitle, SW_ANALYSIS_FFT_LENGTH_LABEL,
							   SppDimension, "points",
							   SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
							   SppEditable, SP_TRUE,
							   SppFieldStrings, fft_length_list,
							   SppFieldOffset, 150,
							   SppFieldSize, 160,
							   SppHelpPath, "dialog/analysis.html#minimum_fft_length",
							   NULL);
    sprintf(string, "%ld", config->fftl);
    spSetTextString(analysis_dialog->fft_length_combo, string);

    /* create combo box to input max fft length */
    analysis_dialog->max_fft_length_combo = spCreateParamField(main_tab, "maxFftLengthComboBox", 60,
							       SppTitle, SW_ANALYSIS_MAX_FFT_LENGTH_LABEL,
							       SppDimension, "points",
							       SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
							       SppEditable, SP_TRUE,
							       SppFieldStrings, max_fft_length_list,
							       SppFieldOffset, 150,
							       SppFieldSize, 160,
							       SppHelpPath, "dialog/analysis.html#maximum_fft_length",
							       NULL);
    sprintf(string, "%ld", config->max_fftl);
    spSetTextString(analysis_dialog->max_fft_length_combo, string);

    /* create combo box to input lifter */
    analysis_dialog->lifter_combo = spCreateParamField(main_tab, "lifterComboBox", 60,
						       SppTitle, SW_ANALYSIS_LIFTER_LABEL,
						       SppDimension, "ms",
						       SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
						       SppEditable, SP_TRUE,
						       SppFieldStrings, lifter_list,
						       SppFieldOffset, 150,
						       SppFieldSize, 160,
						       SppHelpPath, "dialog/analysis.html#analysis_lifter",
						       NULL);
    sprintf(string, "%f", config->lifterm);
    spSetTextString(analysis_dialog->lifter_combo, string);

#if defined(SW_SUPPORT_CQT_ANALYSIS)
    /* create CQT parameter tab */
    swCreateCQTParameterTab(analysis_dialog, tab_box);
#endif

    /* create attribute tab */
    attr_tab = spAddTabItem(tab_box, "analysisAttributeTab", -1,
			    SppTitle, SW_ANALYSIS_ATTRIBUTE_LABEL,
			    SppHelpPath, "dialog/analysis.html#analysis_attribute",
			    NULL);
    

    /* create check box to set whether analysis always makes a new window */
    analysis_dialog->new_window_button = spCreateCheckBox(attr_tab, "analysisNewWindowButton",
							  SppTitle, SW_ANALYSIS_NEW_WINDOW_LABEL,
							  SppSet, analysis_dialog->config->analysis_new_window,
							  SppHelpPath, "dialog/analysis.html#analysis_new_window",
							  NULL);

    /* create check box for spectral analysis */
    analysis_dialog->linear_spectrum_button = spCreateCheckBox(attr_tab, "analysisLinearSpectrumButton",
					SppTitle, SW_ANALYSIS_LINEAR_SPECTRUM_LABEL,
					SppSet, analysis_dialog->config->wave_config->linear_spectrum,
	    			        SppHelpPath, "dialog/analysis.html#analysis_linear_spectrum",
					NULL);
    analysis_dialog->normalize_spectrum_button = spCreateCheckBox(attr_tab, "analysisNormalizeSpectrumButton",
					SppTitle, SW_ANALYSIS_NORMALIZE_SPECTRUM_LABEL,
					SppSet, analysis_dialog->config->wave_config->normalize_spectrum,
					SppHelpPath, "dialog/analysis.html#analysis_normalize_spectrum",
					NULL);
    analysis_dialog->normalize_spectrum_max_button = spCreateCheckBox(attr_tab, "analysisNormalizeSpectrumMaxButton",
					SppTitle, SW_ANALYSIS_NORMALIZE_SPECTRUM_MAX_LABEL,
					SppSet, analysis_dialog->config->wave_config->normalize_spectrum_max,
					SppHelpPath, "dialog/analysis.html#analysis_normalize_spectrum_max",
					NULL);

    return analysis_dialog;
}

swAnalysisDialog swCreateAnalysisDialog(swConfig config)
{
    static swAnalysisDialog sw_analysis_dialog = NULL;

    if (sw_analysis_dialog == NULL) {
	sw_analysis_dialog = createAnalysisDialog(config);
    }
    return sw_analysis_dialog;
}

void swPopupAnalysisDialogCB(spComponent component, swWindow window)
{
    swAnalysisDialog analysis_dialog;
    
    analysis_dialog = swCreateAnalysisDialog(window->config);
    
    analysis_dialog->fft_spectrogram_parameter_updated = SP_FALSE;
    analysis_dialog->cqt_spectrogram_parameter_updated = SP_FALSE;

    /* popup dialog */
    spPopupWindow(analysis_dialog->window);

    return;
}

static void getCurrentAnalysisDialogValues(swAnalysisDialog analysis_dialog)
{
    int index;
    char *string;
    
    analysis_dialog->fft_spectrogram_parameter_updated = SP_FALSE;
    analysis_dialog->lifter_updated = SP_FALSE;
    
    if ((string = xspGetSelectedListItem(analysis_dialog->analysis_type_combo)) != NULL) {
        analysis_dialog->config->analysis_type = swGetAnalysisTypeFromLabel(string);
        strcpy(analysis_dialog->config->analysis_type_string,
               swGetAnalysisTypeParamLabel(analysis_dialog->config->analysis_type));
        xfree(string);
    }

    if ((index = spGetSelectedListIndex(analysis_dialog->window_type_combo)) >= 0) {
        if (index != (int)analysis_dialog->config->window_type) {
            analysis_dialog->config->window_type = (swWindowType)index;
            strcpy(analysis_dialog->config->window_type_string,
                   swGetWindowTypeParamLabel(analysis_dialog->config->window_type));
            analysis_dialog->fft_spectrogram_parameter_updated = SP_TRUE;
        }
    }
	
    if ((string = xspGetTextString(analysis_dialog->fft_length_combo)) != NULL) {
        long fftl;
        spDebug(10, "swPopdownAnalysisDialogCB", "fft length string = %s\n", string);
        fftl = atol(string);
        if (analysis_dialog->config->fftl != fftl) {
            analysis_dialog->config->fftl = fftl;
            analysis_dialog->fft_spectrogram_parameter_updated = SP_TRUE;
        }
        xfree(string);
    }
    if ((string = xspGetTextString(analysis_dialog->max_fft_length_combo)) != NULL) {
        spDebug(10, "swPopdownAnalysisDialogCB", "max fft length string = %s\n", string);
        analysis_dialog->config->max_fftl = atol(string);
        xfree(string);
    }

    if ((string = xspGetTextString(analysis_dialog->lifter_combo)) != NULL) {
        double lifterm;
        spDebug(10, "swPopdownAnalysisDialogCB", "lifter string = %s\n", string);
        lifterm = atof(string);
        if (analysis_dialog->config->lifterm != lifterm) {
            analysis_dialog->config->lifterm = lifterm;
            analysis_dialog->lifter_updated = SP_TRUE;
        }
        xfree(string);
    }
	
    spGetToggleState(analysis_dialog->new_window_button, &analysis_dialog->config->analysis_new_window);
    spGetToggleState(analysis_dialog->linear_spectrum_button,
                     &analysis_dialog->config->wave_config->linear_spectrum);
    spGetToggleState(analysis_dialog->normalize_spectrum_button,
                     &analysis_dialog->config->wave_config->normalize_spectrum);
    spGetToggleState(analysis_dialog->normalize_spectrum_max_button,
                     &analysis_dialog->config->wave_config->normalize_spectrum_max);

    return;
}

void swPopdownAnalysisDialogCB(spComponent component, swAnalysisDialog analysis_dialog)
{
    spCallbackReason reason = SP_CR_NONE;

    if (analysis_dialog == NULL || spIsCreated(analysis_dialog->window) == SP_FALSE) return;

    reason = spGetCallbackReason(component);

    if (reason == SP_CR_OK || reason == SP_CR_CANCEL) {
	/* popdown analysis dialog */
	spPopdownWindow(analysis_dialog->window);
    }
    
    if (reason == SP_CR_OK || reason == SP_CR_APPLY) {
        getCurrentAnalysisDialogValues(analysis_dialog);

#if defined(SW_SUPPORT_CQT_ANALYSIS)
        swGetCurrentCQTParameterValues(analysis_dialog);
#endif
        
        spDebug(10, "swPopdownAnalysisDialogCB",
                "call swReanalysisAll, fft_spectrogram_parameter_updated = %d, lifter_updated = %d, cqt_spectrogram_parameter_updated = %d\n",
                analysis_dialog->fft_spectrogram_parameter_updated,
                analysis_dialog->lifter_updated, analysis_dialog->cqt_spectrogram_parameter_updated);
	swReanalysisAll(analysis_dialog->window, analysis_dialog->fft_spectrogram_parameter_updated,
                        analysis_dialog->lifter_updated, analysis_dialog->cqt_spectrogram_parameter_updated);
    }
    
    return;
}

#if defined(SW_SUPPORT_CQT_ANALYSIS)
static void swGetCurrentCQTParameterValues(swAnalysisDialog analysis_dialog)
{
    spBool set;
    char *string;
    
    analysis_dialog->cqt_spectrogram_parameter_updated = SP_FALSE;
    
    if ((string = xspGetTextString(analysis_dialog->cqt_bins_per_octave_combo_box)) != NULL) {
        long cqt_bins_per_octave;
        spDebug(10, "swGetCurrentCQTParameterValues", "cqt_bins_per_octave string = %s\n", string);
        cqt_bins_per_octave = atol(string);
        if (analysis_dialog->config->wave_config->cqt_bins_per_octave != cqt_bins_per_octave) {
            analysis_dialog->config->wave_config->cqt_bins_per_octave = cqt_bins_per_octave;
            analysis_dialog->cqt_spectrogram_parameter_updated = SP_TRUE;
        }
        xfree(string);
    }

    if ((string = xspGetTextString(analysis_dialog->cqt_min_freq_combo_box)) != NULL) {
        spDebug(10, "swGetCurrentCQTParameterValues", "cqt_min_freq string = %s\n", string);
        analysis_dialog->config->wave_config->cqt_min_freq = atof(string);
        analysis_dialog->cqt_spectrogram_parameter_updated = SP_TRUE;
        xfree(string);
    }
	
    if (spGetToggleState(analysis_dialog->cqt_min_freq_based_on_musical_note_check_box, &set) == SP_TRUE) {
        if (set != analysis_dialog->config->wave_config->cqt_min_freq_based_on_musical_note) {
            analysis_dialog->config->wave_config->cqt_min_freq_based_on_musical_note = set;
            analysis_dialog->cqt_spectrogram_parameter_updated = SP_TRUE;
        }
    }
    if (spGetToggleState(analysis_dialog->cqt_use_erb_check_box, &set) == SP_TRUE) {
        if (set != analysis_dialog->config->wave_config->cqt_use_erb) {
            analysis_dialog->config->wave_config->cqt_use_erb = set;
            analysis_dialog->cqt_spectrogram_parameter_updated = SP_TRUE;
        }
    }

    if ((string = xspGetTextString(analysis_dialog->cqt_block_margin_factor_combo_box)) != NULL) {
        spDebug(10, "swGetCurrentCQTParameterValues", "cqt_block_margin_factor string = %s\n", string);
        analysis_dialog->config->wave_config->cqt_block_margin_factor = atof(string);
        analysis_dialog->cqt_spectrogram_parameter_updated = SP_TRUE;
        xfree(string);
    }
    
    if ((string = xspGetTextString(analysis_dialog->cqt_block_transition_factor_combo_box)) != NULL) {
        spDebug(10, "swGetCurrentCQTParameterValues", "cqt_block_transition_factor string = %s\n", string);
        analysis_dialog->config->wave_config->cqt_block_transition_factor = atof(string);
        analysis_dialog->cqt_spectrogram_parameter_updated = SP_TRUE;
        xfree(string);
    }
    
    return;
}

static char *cqt_bins_per_octave_list[] =
{
    "3",
    "6",
    "9",
    "12",
    "18",
    "24",
    "32",
    "48",
    "60",
    "72",
    NULL,
};

static char *cqt_min_freq_list[] =
{
    "27.5",
    "30.87",
    "32.70",
    "36.71",
    "41.20",
    "43.65",
    "49.00",
    "55.0",
    "61.74",
    "65.41",
    "73.42",
    "82.41",
    NULL,
};

static char *cqt_block_margin_factor_list[] =
{
    "1.0",
    "1.5",
    "2.0",
    "2.5",
    "3.0",
    "3.5",
    "4.0",
    "4.5",
    "5.0",
    "6.0",
    "7.0",
    "8.0",
    NULL,
};

static char *cqt_block_transition_factor_list[] =
{
    "2.0",
    "3.0",
    "4.0",
    "5.0",
    "6.0",
    "7.0",
    "8.0",
    "10.0",
    "12.0",
    "14.0",
    "16.0",
    NULL,
};

static void createCQTParameterComponents(swAnalysisDialog analysis_dialog, spComponent parent)
{
    char string[SP_MAX_LINE];
    
    /* create combo box to input bins per octave for CQT */
    analysis_dialog->cqt_bins_per_octave_combo_box = spCreateParamField(parent, "cqtParamBinsPerOctaveComboBox", 60,
                                                                        SppTitle, SW_CQT_PARAMETER_BINS_PER_OCTAVE_LABEL,
                                                                        SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
                                                                        SppEditable, SP_TRUE,
                                                                        SppFieldStrings, cqt_bins_per_octave_list,
                                                                        SppFieldOffset, 210,
                                                                        SppFieldSize, 160,
                                                                        SppHelpPath, "dialog/analysis.html#cqt_bins_per_octave",
                                                                        NULL);
    sprintf(string, "%ld", analysis_dialog->config->wave_config->cqt_bins_per_octave);
    spSetTextString(analysis_dialog->cqt_bins_per_octave_combo_box, string);
    
    /* create combo box to input minimum frequency of CQT */
    analysis_dialog->cqt_min_freq_combo_box = spCreateParamField(parent, "cqtParamMinFreqComboBox", 60,
                                                                 SppTitle, SW_CQT_PARAMETER_MIN_FREQ_LABEL,
                                                                 SppDimension, "Hz",
                                                                 SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
                                                                 SppEditable, SP_TRUE,
                                                                 SppFieldStrings, cqt_min_freq_list,
                                                                 SppFieldOffset, 210,
                                                                 SppFieldSize, 160,
                                                                 SppHelpPath, "dialog/analysis.html#cqt_min_freq",
                                                                 NULL);
    spNFtos(string, sizeof(string), analysis_dialog->config->wave_config->cqt_min_freq);
    spSetTextString(analysis_dialog->cqt_min_freq_combo_box, string);
    
    /* create check box to set whether CQT uses ERB frequency axis */
    analysis_dialog->cqt_min_freq_based_on_musical_note_check_box = spCreateCheckBox(parent, "cqtParamMinFreqBasedOnMusicalNoteCheckBox",
                                                                                     SppTitle, SW_CQT_PARAMETER_MIN_FREQ_BASED_ON_MUSICAL_NOTE_LABEL,
                                                                                     SppSet, analysis_dialog->config->wave_config->cqt_min_freq_based_on_musical_note,
                                                                                     SppHelpPath, "dialog/analysis.html#cqt_min_freq_based_on_musical_note",
                                                                                     NULL);

    /* create check box to set whether CQT uses ERB frequency axis */
    analysis_dialog->cqt_use_erb_check_box = spCreateCheckBox(parent, "cqtParamUseERBCheckBox",
                                                              SppTitle, SW_CQT_PARAMETER_USE_ERB_LABEL,
                                                              SppSet, analysis_dialog->config->wave_config->cqt_use_erb,
                                                              SppHelpPath, "dialog/analysis.html#cqt_use_erb",
                                                              NULL);

    /* create combo box to input margin factor of block length for CQT */
    analysis_dialog->cqt_block_margin_factor_combo_box = spCreateParamField(parent, "cqtParamBlockMarginFactorComboBox", 60,
                                                                            SppTitle, SW_CQT_PARAMETER_BLOCK_MARGIN_FACTOR_LABEL,
                                                                            SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
                                                                            SppEditable, SP_TRUE,
                                                                            SppFieldStrings, cqt_block_margin_factor_list,
                                                                            SppFieldOffset, 210,
                                                                            SppFieldSize, 160,
                                                                            SppHelpPath, "dialog/analysis.html#cqt_block_margin_factor",
                                                                            NULL);
    spNFtos(string, sizeof(string), analysis_dialog->config->wave_config->cqt_block_margin_factor);
    spSetTextString(analysis_dialog->cqt_block_margin_factor_combo_box, string);
    
    /* create combo box to input transition factor of block length for CQT */
    analysis_dialog->cqt_block_transition_factor_combo_box = spCreateParamField(parent, "cqtParamBlockTransitionFactorComboBox", 60,
                                                                            SppTitle, SW_CQT_PARAMETER_BLOCK_TRANSITION_FACTOR_LABEL,
                                                                            SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
                                                                            SppEditable, SP_TRUE,
                                                                            SppFieldStrings, cqt_block_transition_factor_list,
                                                                            SppFieldOffset, 210,
                                                                            SppFieldSize, 160,
                                                                            SppHelpPath, "dialog/analysis.html#cqt_block_transition_factor",
                                                                            NULL);
    spNFtos(string, sizeof(string), analysis_dialog->config->wave_config->cqt_block_transition_factor);
    spSetTextString(analysis_dialog->cqt_block_transition_factor_combo_box, string);
    
    return;
}

static void swCreateCQTParameterTab(swAnalysisDialog analysis_dialog, spComponent parent)
{
    spComponent cqt_tab;
    
    /* create CQT parameter tab */
    cqt_tab = spAddTabItem(parent, "analysisCQTTab", -1,
			    SppTitle, SW_CQT_PARAMETER_LABEL,
			    SppHelpPath, "dialog/analysis.html#cqt_parameter",
			    NULL);
    createCQTParameterComponents(analysis_dialog, cqt_tab);
    
    return;
}
#endif

static void spectrogramTrackBarCB(spComponent component, swSpectrogramDialog dialog)
{
    int value;
    const char *name;

    if (spGetSliderValue(component, &value) == SP_TRUE) {
	name = spGetName(component);
    
	spDebug(10, "spectrogramTrackBarCB", "name = %s, value = %d\n", name, value);

	if (streq(name, "spectrogramRangeTrackBar")) {
	    dialog->config->specgram_range = value;
	} else {
	    dialog->config->specgram_limit_threshold = value;
	}

	if (swIsSpectrogramVisible(dialog->current_window) == SP_TRUE) {
	    swDrawWave(dialog->current_window);
	}
    }
    
    return;
}

static void spectrogramCheckBoxCB(spComponent component, swSpectrogramDialog dialog)
{
    spBool flag;
    const char *name;

    if (spGetToggleState(component, &flag) == SP_TRUE) {
	name = spGetName(component);
    
	spDebug(10, "spectrogramCheckBoxCB", "name = %s, flag = %d\n", name, flag);

	if (streq(name, "spectrogramSimplifiedCheckBox")) {
	    dialog->config->specgram_simplified = flag;
	} else {
	    dialog->config->specgram_gray_scale = flag;
	}

	if (swIsSpectrogramVisible(dialog->current_window) == SP_TRUE) {
	    swDrawWave(dialog->current_window);
	}
    }
    
    return;
}

void swPopdownSpectrogramDialogCB(spComponent component, swSpectrogramDialog dialog)
{
    spCallbackReason reason = SP_CR_NONE;

    if (dialog == NULL || spIsCreated(dialog->dialog) == SP_FALSE) return;

    reason = spGetCallbackReason(component);

    if (reason == SP_CR_OK || reason == SP_CR_CANCEL) {
	/* popdown analysis dialog */
	spPopdownWindow(dialog->dialog);
    }

    return;
}

static swSpectrogramDialog createSpectrogramDialog(swConfig config)
{
    swSpectrogramDialog dialog;

    dialog = xalloc(1, struct _swSpectrogramDialog);
    memset(dialog, 0, sizeof(struct _swSpectrogramDialog));
    dialog->config = config;

    dialog->dialog = spCreateDialogBox("spectrogramDialog",
				       SppTitle, SW_SPECTROGRAM_DIALOG_TITLE,
				       SppCallbackFunc, swPopdownSpectrogramDialogCB,
				       SppCallbackData, dialog,
				       /*SppDialogBoxButtonType, SP_DB_OK_CANCEL_APPLY,*/
				       SppDialogBoxButtonType, SP_DB_OK,
				       SppPopupStyle, SP_MODAL_POPUP,
				       /*SppPopupStyle, SP_MODELESS_POPUP,*/
				       SppCloseStyle, SP_UNMAP_CLOSE,
				       SppSpacingOn, SP_TRUE,
				       SppHelpButtonVisible, SP_TRUE,
				       SppHelpPath, "dialog/spectrogram.html",
				       NULL);
    
    dialog->range_track_bar = spCreateParamField(dialog->dialog, "spectrogramRangeTrackBar", 0,
						 SppFieldType, SP_FIELD_TYPE_TRACK_BAR,
						 SppTitle, SW_SPECTROGRAM_RANGE_LABEL,
						 SppFieldOffset, 120,
						 SppFieldSize, 180,
						 SppCallbackFunc, spectrogramTrackBarCB,
						 SppCallbackData, dialog,
						 SppTrackCallbackOn, SP_TRUE,
						 SppShowScale, SP_TRUE,
						 SppShowValue, SP_TRUE,
						 SppValue, config->specgram_range,
						 SppMinimum, 10,
						 SppMaximum, 120,
						 SppHelpPath, "dialog/spectrogram.html#spectrogram_range",
						 NULL);
    dialog->limit_threshold_track_bar = spCreateParamField(dialog->dialog, "spectrogramLimitThresholdTrackBar", 0,
							   SppFieldType, SP_FIELD_TYPE_TRACK_BAR,
							   SppTitle, SW_SPECTROGRAM_LIMIT_THRESHOLD_LABEL,
							   SppFieldOffset, 120,
							   SppFieldSize, 180,
							   SppCallbackFunc, spectrogramTrackBarCB,
							   SppCallbackData, dialog,
							   SppTrackCallbackOn, SP_TRUE,
							   SppShowScale, SP_TRUE,
							   SppShowValue, SP_TRUE,
							   SppValue, config->specgram_limit_threshold,
							   SppMinimum, -100,
							   SppMaximum, 0,
							   SppHelpPath, "dialog/spectrogram.html#spectrogram_limit_threshold",
							   NULL);

    dialog->simplified_check_box = spCreateCheckBox(dialog->dialog, "spectrogramSimplifiedCheckBox",
						    SppTitle, SW_SPECTROGRAM_SIMPLIFIED_LABEL,
						    SppCallbackFunc, spectrogramCheckBoxCB,
						    SppCallbackData, dialog,
						    SppSet, config->specgram_simplified,
						    SppHelpPath, "dialog/spectrogram.html#spectrogram_simplified",
						    NULL);
    dialog->gray_scale_check_box = spCreateCheckBox(dialog->dialog, "spectrogramGrayScaleCheckBox",
						    SppTitle, SW_SPECTROGRAM_GRAY_SCALE_LABEL,
						    SppCallbackFunc, spectrogramCheckBoxCB,
						    SppCallbackData, dialog,
						    SppSet, config->specgram_gray_scale,
						    SppHelpPath, "dialog/spectrogram.html#spectrogram_gray_scale",
						    NULL);
    
    return dialog;
}

void swPopupSpectrogramDialogCB(spComponent component, swWindow window)
{
    static swSpectrogramDialog dialog = NULL;

    if (dialog == NULL) {
	dialog = createSpectrogramDialog(window->config);
    }
    dialog->current_window = window;

    /* popup dialog */
    spPopupWindow(dialog->dialog);

    return;
}

#endif
