/*
 *	swAnalysis.c
 *
 *	Last modified: <2025-04-06 22:36:10 hideki>
 */

#include <stdio.h>
#include <stdlib.h>

#include <sp/spBaseLib.h>
#include <sp/spComponentLib.h>

#include "swAnalysis.h"
#include "swWindow.h"
#include "swDraw.h"
#include "swCursor.h"
#include "swWave.h"
#include "swEdit.h"

#ifdef SW_USE_ANALYSIS
#include <sp/spLib.h>

long swGetFFTLength(swWindow window, spLong length)
{
    long fftl;
    
    fftl = MAX((long)length, window->config->fftl);
    fftl = POW2(spNextPow2(fftl));
    if (fftl > window->config->max_fftl) {
	spDisplayError(window->window, NULL, SW_ANALYSIS_LONG_ERROR_MESSAGE);
	return -1;
    }

    return fftl;
}

swWindow swAnalysisWindow(swWindow window, spLong offset, spLong length, spBool cqt_flag, spBool new_flag)
{
    long fftl;
    swWave wave;
    spBool redraw_horizontal_keys = SP_FALSE;
    swWindow new_window = NULL;
    swDataType data_type = SW_FREQ_DATA;

    if (swIsNoWave(window) == SP_TRUE) return NULL;

    spDebug(10, "swAnalysisWindow", "analysis_type = %d, window_type = %d, cqt_flag = %d\n",
	    window->config->analysis_type, window->config->window_type, cqt_flag);

    if (cqt_flag) {
        fftl = 0;
    } else {
        if ((fftl = swGetFFTLength(window, length)) < 0) {
            return NULL;
        }
    }
    
    swSetMouseCursor(window, SP_CURSOR_WAIT);
    
    wave = swAnalysisWave(window->wave, offset, length,
			  cqt_flag ? window->config->cqt_type : window->config->analysis_type,
                          window->config->window_type, fftl, window->config->lifterm);

    if (wave == NULL) {
	swSetMouseCursor(window, SP_CURSOR_UNKNOWN);
	spDisplayError(window->window, NULL, SW_ANALYSIS_ERROR_MESSAGE);
	return NULL;
    }

    if (cqt_flag == SP_FALSE
        && (window->config->analysis_type == SW_ANALYSIS_CEPSTRUM
            || window->config->analysis_type == SW_ANALYSIS_TD_GROUP_DELAY)) {
	data_type = SW_TIME_DATA;
    }
    
    spDebug(50, "swAnalysisWindow", "window->draw_vertical_keys = %d\n", window->draw_vertical_keys);
    if (data_type == SW_FREQ_DATA && window->draw_vertical_keys == SP_TRUE) {
        redraw_horizontal_keys = SP_TRUE;
    }
    
    if (window->related_window != NULL
	&& window->related_window->analysis_flag == SP_TRUE
	&& (new_flag == SP_FALSE || window->config->analysis_new_window == SP_FALSE)) {
	new_window = window->related_window;
	
	swDestroyWindowWave(new_window, &new_window->wave);
	new_window->offset = 0;
	new_window->length = 0;
	new_window->analysis_flag = SP_TRUE;
	
	if (new_window->data_type != data_type) {
	    swInitVscroll(new_window);
	}
        if (redraw_horizontal_keys == SP_TRUE && new_window->draw_horizontal_keys == SP_FALSE) {
            new_window->draw_horizontal_keys = SP_TRUE;
        } else {
            redraw_horizontal_keys = SP_FALSE;
        }
	
	new_window->data_type = data_type;
	
	swSetWave(new_window, wave);
	swSetWindowTitle(new_window);
	swSetSenseLevel(new_window);
	swReloadWave(new_window, new_window->wave, SP_TRUE, SP_FALSE);
	swDrawOverview(new_window, SP_TRUE);
	
	new_flag = SP_FALSE;
    } else {
	/* create new window */
	new_window = swCreateWaveWindow(wave, window->config, data_type, -1.0, -1.0);
	new_window->analysis_flag = SP_TRUE;
        if (redraw_horizontal_keys == SP_TRUE && new_window->draw_horizontal_keys == SP_FALSE) {
            new_window->draw_horizontal_keys = SP_TRUE;
        } else {
            redraw_horizontal_keys = SP_FALSE;
        }
	
	new_flag = SP_TRUE;
    }
    window->related_window = new_window;
    new_window->related_window = window;

    spDebug(50, "swAnalysisWindow", "redraw_horizontal_keys = %d, new_window->draw_horizontal_keys = %d\n",
            redraw_horizontal_keys, new_window->draw_horizontal_keys);
    if (redraw_horizontal_keys == SP_TRUE) {
        swUpdateWaveSubAreaSize(new_window);
    }
    
    spDebug(10, "swAnalysisWindow", "find window done\n");
    
    swSetMouseCursor(window, SP_CURSOR_UNKNOWN);

    if (new_flag == SP_TRUE) {
	swDrawWave(new_window);
    }

    spDebug(10, "swAnalysisWindow", "done\n");
    
    return new_window;
}

void swAnalysisRegionCB(spComponent component, swWindow window)
{
    spLong offset, length;
    swWindow new_window;

    if (swIsNoWave(window) == SP_FALSE) {
	/* get edge */
	if (swGetEdge(window, window->sel_st, window->sel_ed, &offset, &length) != SP_TRUE) {
	    spDisplayError(window->window, NULL, SW_REGION_ERROR_MESSAGE);
	    return;
	}
  
	if ((new_window = swAnalysisWindow(window, offset, length, SP_FALSE, SP_TRUE)) == NULL) {
	    return;
	} else {
	    spMapWindow(new_window->window);
	}
    }
    return;
}

void swAnalysisFrame(spComponent component, swWindow window, swAnalysisConfigFlag config_flag)
{
    spLong framel;
    spLong ana_offset;
    spLong offset, length;
    swWindow new_window;

    if (swIsNoWave(window) == SP_FALSE) {
        if (config_flag == SW_ANALYSIS_CONFIG_FLAG_CQT_SPECTRUM || window->config->analysis_type & SW_ANALYSIS_CQT_MASK) {
            framel = spCalcCQTMinFFTLength(-1.0, window->config->wave_config->cqt_bins_per_octave,
                                           window->wave->samp_rate, SP_FALSE);
        } else {
            if (config_flag & SW_ANALYSIS_CONFIG_FLAG_WIDE_SPECTRUM) {
                framel = swDimToSamp(window, window->config->wide_framem / 1000.0);
            } else {
                framel = swDimToSamp(window, window->config->narrow_framem / 1000.0);
            }
        }
        ana_offset = window->point - framel / 2;
	
	/* get edge */
	if (swGetEdge(window, ana_offset, ana_offset + framel - 1, &offset, &length) != SP_TRUE) {
	    spDisplayError(window->window, NULL, SW_REGION_ERROR_MESSAGE);
	    return;
	}

	/* select region */
	swSelectRegion(window, window->wave->selected_channel, offset, offset + length - 1);
	
	/* analysis */
	if ((new_window = swAnalysisWindow(window, offset, length,
                                           (config_flag & SW_ANALYSIS_CONFIG_FLAG_CQT_SPECTRUM) ? SP_TRUE : SP_FALSE, SP_TRUE)) == NULL) {
	    return;
	} else {
	    spMapWindow(new_window->window);
	}
    }
    
    return;
}

void swAnalysisWideCB(spComponent component, swWindow window)
{
    swAnalysisFrame(component, window, SW_ANALYSIS_CONFIG_FLAG_WIDE_SPECTRUM);
    return;
}

void swAnalysisNarrowCB(spComponent component, swWindow window)
{
    swAnalysisFrame(component, window, SW_ANALYSIS_CONFIG_FLAG_NARROW_SPECTRUM);
    return;
}

#if defined(SW_SUPPORT_CQT_ANALYSIS)
void swAnalysisCQTCB(spComponent component, swWindow window)
{
    swAnalysisFrame(component, window, SW_ANALYSIS_CONFIG_FLAG_CQT_SPECTRUM);
    return;
}
#endif

void swReanalysisAll(spComponent component, spBool update_fft_specgram, spBool update_lifter_related_specgram, spBool update_cqt_specgram)
{
    spLong offset, length;
    spComponent next = NULL;
    swWindow window = NULL;
    swWindow next_window = NULL;

    if (spIsCreated(component) == SP_FALSE) return;

    next = component;
    while (1) {
	if (next == NULL) {
	    break;
	}
	
	if ((window = (swWindow)spGetUserData(next)) != NULL) {
	    break;
	}

	next = spGetNextWindow(next, SP_TRUE);
	
	if (next == component) {
	    break;
	}
    }

    if (window == NULL) return;

    next = window->window;
    while (1) {
	if (next == NULL) {
            spDebug(80, "swReanalysisAll", "no window, break\n");
	    break;
	}
	
	if ((next_window = (swWindow)spGetUserData(next)) != NULL
	    && next_window->wave != NULL) {
	    if (next_window->data_type == SW_TIME_DATA
		&& next_window->related_window != NULL) {
		/* get edge */
		if (swGetEdge(next_window, next_window->sel_st, next_window->sel_ed, &offset, &length) == SP_TRUE) {
                    spDebug(80, "swReanalysisAll", "reanalysis, offset = %ld, length = %ld\n", offset, length);
		    swInitVscroll(next_window->related_window);
		    swAnalysisWindow(next_window, offset, length, SP_FALSE, SP_FALSE);
		}
	    }
            if (swIsSpectrogramVisible(next_window) == SP_TRUE) {
                spDebug(80, "swReanalysisAll", "spectrogram visible, specgram_config_flag = %lx\n", next_window->specgram_config_flag);
                if ((update_cqt_specgram == SP_TRUE && (next_window->specgram_config_flag & SW_ANALYSIS_CONFIG_FLAG_CQT_SPECTRUM))
                    || (update_lifter_related_specgram == SP_TRUE && (next_window->specgram_config_flag & SW_ANALYSIS_CONFIG_FLAG_SMOOTHED_SPECTRUM))
                    || (update_fft_specgram == SP_TRUE && (next_window->specgram_config_flag & SW_ANALYSIS_CONFIG_FLAG_NARROW_SPECTRUM
                                                           || next_window->specgram_config_flag & SW_ANALYSIS_CONFIG_FLAG_WIDE_SPECTRUM))) {
                    swUpdateSpectrogram(next_window, SP_FALSE);
                }
            }
	}

	next = spGetNextWindow(next, SP_FALSE);
	
	if (next == window->window) {
            spDebug(80, "swReanalysisAll", "back to first window, break\n");
	    break;
	}
    }
    
    return;
}

static swWave getSpectrogramToDraw(swWindow window, swAnalysisConfigFlag config_flag, swAnalysisType analysis_type,
				   spBool *interrupted)
{
    long framel;
    long shiftl;
    long fftl;
    swWave specgram = NULL;
    
    spDebug(10, "getSpectrogramToDraw", "config_flag = %lx\n", config_flag);

    if (config_flag & SW_ANALYSIS_CONFIG_FLAG_CQT_SPECTRUM) {
        framel = shiftl = fftl = 0;
    } else {
        if (config_flag & SW_ANALYSIS_CONFIG_FLAG_WIDE_SPECTRUM) {
            framel = (long)swDimToSamp(window, window->config->wide_framem / 1000.0);
        } else {
            framel = (long)swDimToSamp(window, window->config->narrow_framem / 1000.0);
        }
	shiftl = (long)swDimToSamp(window, window->config->shiftm / 1000.0);
	shiftl = MIN(framel / 2, shiftl);

        if ((fftl = swGetFFTLength(window, framel)) < 0) {
            return NULL;
        }
    }
    
    *interrupted = SP_FALSE;
    spDebug(50, "getSpectrogramToDraw", "framel = %ld, shiftl = %ld, fftl = %ld\n", framel, shiftl, fftl);

    if ((specgram = swGetSpectrogram(window->wave, 0, 0, framel, shiftl,
                                     analysis_type, window->config->window_type,
                                     fftl, window->config->lifterm, interrupted)) == NULL) {
        /* error */
    } else {
        spDebug(10, "getSpectrogramToDraw", "framel = %ld, shiftl = %ld, fftl = %ld\n",
                framel, shiftl, fftl);
    }

    spDebug(80, "getSpectrogramToDraw", "done\n");
    
    return specgram;
}

static void setSpecgramToSubArea(swWindow window, swWave specgram, spBool specgram_flag, spBool main_flag)
{
    if (main_flag == SP_TRUE) {
	swSetWaveToFirstSubArea(window, specgram, specgram_flag);
    } else {
	swSetWaveToSubArea(window, specgram, specgram_flag, SP_TRUE);
    }
    swUpdateWaveSubAreaSize(window);

    /*window->drawn_pos = 0;*/
    swResetDrawnPos(window, specgram);
    
    return;
}

static void setSpecgramToDraw(swWindow window, swWave *target_specgram, swWave new_specgram,
			      spBool specgram_flag, spBool main_flag)
{
    spDebug(10, "setSpecgramToDraw", "specgram_flag = %d\n", specgram_flag);
    
    window->amp_min = 0.0; window->amp_max = -1.0;
    
    swDestroyWindowWave(window, target_specgram);

    *target_specgram = new_specgram;
    setSpecgramToSubArea(window, *target_specgram, specgram_flag, main_flag);
    
    spDebug(10, "setSpecgramToDraw", "done\n");
    
    return;
}

static spBool swSubplotSpectrogram(swWindow window, swAnalysisConfigFlag config_flag, swAnalysisType analysis_type,
				   spBool main_flag)
{
    spBool success;
    spBool interrupted;
    spBool specgram_flag;
    swWave specgram;
    swWave *target_wave;
    
    /*if (swIsNoWave(window) == SP_TRUE) return;*/
    if (window->wave == NULL) return SP_FALSE;

    spDebug(10, "swSubplotSpectrogram", "config_flag = %lx\n", config_flag);

    success = SP_TRUE;
    interrupted = SP_FALSE;
    specgram_flag = SP_FALSE;

#ifdef SW_SUPPORT_ANALYSIS_SUBPLOT
    if (swIsAnalysisTypeF0(analysis_type) == SP_TRUE) {
	target_wave = &window->f0;
#ifdef SW_SUPPORT_STRAIGHT
	if (swIsAnalysisTypeStraight(analysis_type) == SP_TRUE) {
	    target_wave = &window->straight_f0;
	    spDebug(10, "swSubplotSpectrogram", "analysis type: STRAIGHT F0\n");
	}
#endif
    } else if (swIsAnalysisTypePower(analysis_type) == SP_TRUE) {
	target_wave = &window->power;
#ifdef SW_SUPPORT_STRAIGHT
    } else if (swIsAnalysisTypeAperiodicity(analysis_type) == SP_TRUE) {
	target_wave = &window->straight_ap;
	specgram_flag = SP_TRUE;
    } else if (swIsAnalysisTypeStraight(analysis_type) == SP_TRUE) {
	target_wave = &window->straight_sgram;
	specgram_flag = SP_TRUE;
#endif
    } else
#endif
    {
	target_wave = &window->specgram;
	specgram_flag = SP_TRUE;
    }
    
    if (*target_wave != NULL) {
	setSpecgramToSubArea(window, *target_wave, specgram_flag, main_flag);
	
	/*swRedrawWave(window);*/
    } else {
#ifdef SW_SUPPORT_STRAIGHT
	if (swIsAnalysisTypeStraight(analysis_type) == SP_TRUE) {
	    long shiftl;
	    shiftl = (long)swDimToSamp(window, MAX(window->config->shiftm, 32.0) / 1000.0);
	    
	    if (swGetStraightSpecgram(window->wave, 0, 0, 5.0, shiftl, 512, &interrupted,
				      &window->straight_f0, &window->straight_ap, &window->straight_sgram) == SP_FALSE) {
		/* error */
		success = SP_FALSE;
	    } else {
		setSpecgramToSubArea(window, *target_wave, specgram_flag, main_flag);
	    }
	} else 
#endif
	if ((specgram = getSpectrogramToDraw(window, config_flag, analysis_type, &interrupted)) == NULL) {
	    /* error */
	    success = SP_FALSE;
	} else {
	    setSpecgramToDraw(window, target_wave, specgram, specgram_flag, main_flag);
	}

	if (success == SP_FALSE) {
            spDebug(10, "getSpectrogramToDraw", "failed, interrupted = %d\n", interrupted);
	    if (interrupted == SP_FALSE) {
		spDisplayError(window->window, NULL, SW_ANALYSIS_ERROR_MESSAGE);
	    }
	    if (*target_wave == window->specgram) {
		swResetSpectrogramMenu(window);
                swResetWindow(window, SP_FALSE);
	    }
	}
    }

    spDebug(80, "swSubplotSpectrogram", "done: success = %d\n", success);
    
    return success;
}

static spBool swSubplotMainSpectrogram(swWindow window, swAnalysisConfigFlag config_flag, swAnalysisType analysis_type)
{
    if (window->specgram != NULL
	&& config_flag == window->specgram_config_flag
	&& analysis_type == window->specgram_analysis_type) {
	if (window->draw_specgram == SP_FALSE) {
	    window->amp_min = 0.0; window->amp_max = -1.0;
	}
	window->draw_specgram = SP_TRUE;
        swGetCheckBoxSubMenuToggleState(window->specgram_menu, "spectrogramDrawKeys", &window->draw_vertical_keys);
        spDebug(30, "swSubplotMainSpectrogram", "window->draw_vertical_keys = %d, window->config->draw_piano_keys_for_specgram = %d\n",
                window->draw_vertical_keys, window->config->draw_piano_keys_for_specgram);
    } else {
	swDestroyWindowWave(window, &window->specgram);
	
#if 0
        /* reset to original waveform */
	window->draw_specgram = SP_FALSE;
        window->draw_vertical_keys = SP_FALSE;
        window->amp_min = 0.0; window->amp_max = -1.0;
        swSetWaveToFirstSubArea(window, window->wave, SP_FALSE);
        swResetWindow(window, SP_FALSE);
#else
        /* no reset case */
	if (window->draw_specgram == SP_FALSE) {
	    window->amp_min = 0.0; window->amp_max = -1.0;
            window->draw_vertical_keys = SP_FALSE;
	}
#endif
    }

    if (swSubplotSpectrogram(window, config_flag, analysis_type, SP_TRUE) == SP_TRUE) {
	window->specgram_config_flag = config_flag;
	window->specgram_analysis_type = analysis_type;
	window->draw_specgram = SP_TRUE;
        swGetCheckBoxSubMenuToggleState(window->specgram_menu, "spectrogramDrawKeys", &window->draw_vertical_keys);
        spDebug(30, "swSubplotMainSpectrogram", "after swSubplotSpectrogram, window->draw_vertical_keys = %d, window->config->draw_piano_keys_specgram = %d\n",
                window->draw_vertical_keys, window->config->draw_piano_keys_for_specgram);
	swResetWindow(window, SP_FALSE);
	
	return SP_TRUE;
    }

    return SP_FALSE;
}

static spBool swDrawSpectrogram(swWindow window, swAnalysisConfigFlag config_flag, swAnalysisType analysis_type)
{
    return swSubplotMainSpectrogram(window, config_flag, analysis_type);
}

#ifdef SW_SUPPORT_ANALYSIS_SUBPLOT
void swSubplotSpectrogramCB(spComponent component, swWindow window)
{
    char *name;
    spBool set;
    spBool do_reset = SP_FALSE;
    spBool failed = SP_FALSE;
    
    name = spGetName(component);
    spDebug(10, "swSubplotSpectrogramCB", "name = %s\n", name);

    if (spGetToggleState(component, &set) == SP_TRUE) {
	if (streq(name, "subplotSpectrogram")) {
	    if (window->subplot_sgram != set) {
                window->amp_min = 0.0; window->amp_max = -1.0;
		if (set == SP_TRUE) {
		    if (swSubplotSpectrogram(window, window->specgram_config_flag,
					     window->specgram_analysis_type, SP_FALSE) == SP_TRUE) {
			do_reset = SP_TRUE;
		    }
		} else if (window->specgram != NULL) {
		    swUnsetWaveToSubArea(window, window->specgram, SP_TRUE);
		    do_reset = SP_TRUE;
		}
		window->subplot_sgram = set;
	    }
	} else if (streq(name, "subplotF0")) {
	    if (window->subplot_f0 != set) {
		if (set == SP_TRUE) {
		    if (swSubplotSpectrogram(window, SP_FALSE,
					     SW_ANALYSIS_CEPSTRUM_F0, SP_FALSE) == SP_TRUE) {
			do_reset = SP_TRUE;
		    } else {
			set = SP_FALSE;	failed = SP_TRUE;
		    }
		} else if (window->f0 != NULL) {
		    swUnsetWaveToSubArea(window, window->f0, SP_TRUE);
		    do_reset = SP_TRUE;
		}
		window->subplot_f0 = set;
	    }
	} else if (streq(name, "subplotPower")) {
	    if (window->subplot_power != set) {
		if (set == SP_TRUE) {
		    if (swSubplotSpectrogram(window, SP_FALSE,
					     SW_ANALYSIS_POWER, SP_FALSE) == SP_TRUE) {
			do_reset = SP_TRUE;
		    } else {
			set = SP_FALSE;	failed = SP_TRUE;
		    }
		} else if (window->power != NULL) {
		    swUnsetWaveToSubArea(window, window->power, SP_TRUE);
		    do_reset = SP_TRUE;
		}
		window->subplot_power = set;
	    }
#ifdef SW_SUPPORT_STRAIGHT
	} else if (streq(name, "subplotStraightSpecgram")) {
	    if (window->subplot_straight_sgram != set) {
		if (set == SP_TRUE) {
		    if (swSubplotSpectrogram(window, window->specgram_config_flag,
					     SW_ANALYSIS_STRAIGHT, SP_FALSE) == SP_TRUE) {
			do_reset = SP_TRUE;
		    }
		} else if (window->straight_sgram != NULL) {
		    swUnsetWaveToSubArea(window, window->straight_sgram, SP_TRUE);
		    do_reset = SP_TRUE;
		}
		window->subplot_straight_sgram = set;
	    }
	} else if (streq(name, "subplotAperiodicity")) {
	    if (window->subplot_straight_ap != set) {
		if (set == SP_TRUE) {
		    if (swSubplotSpectrogram(window, window->specgram_config_flag,
					     SW_ANALYSIS_STRAIGHT_APERIODICITY, SP_FALSE) == SP_TRUE) {
			do_reset = SP_TRUE;
		    }
		} else if (window->straight_ap != NULL) {
		    swUnsetWaveToSubArea(window, window->straight_ap, SP_TRUE);
		    do_reset = SP_TRUE;
		}
		window->subplot_straight_ap = set;
	    }
	} else if (streq(name, "subplotStraightF0")) {
	    if (window->subplot_straight_f0 != set) {
		if (set == SP_TRUE) {
		    if (swSubplotSpectrogram(window, window->specgram_config_flag,
					     SW_ANALYSIS_STRAIGHT_F0, SP_FALSE) == SP_TRUE) {
			do_reset = SP_TRUE;
		    }
		} else if (window->straight_f0 != NULL) {
		    swUnsetWaveToSubArea(window, window->straight_f0, SP_TRUE);
		    do_reset = SP_TRUE;
		}
		window->subplot_straight_f0 = set;
	    }
#endif
	}
    }
    
    if (do_reset == SP_TRUE) {
	swResetWindow(window, SP_FALSE);
    }
    if (failed == SP_TRUE) {
	spSetToggleState(component, SP_FALSE);
    }
    
    return;
}

spBool swUpdateSubplot(swWindow window, spBool exclude_specgram)
{
    spBool flag = SP_TRUE;
    
    if (exclude_specgram == SP_FALSE && window->subplot_sgram == SP_TRUE) {
	swDestroyWindowWave(window, &window->specgram);
	
	if (swSubplotSpectrogram(window, window->specgram_config_flag,
				 window->specgram_analysis_type, SP_FALSE) == SP_FALSE) {
	    flag = SP_FALSE;
	}
    }
    if (window->subplot_f0 == SP_TRUE) {
	swDestroyWindowWave(window, &window->f0);
	
	if (swSubplotSpectrogram(window, SP_FALSE,
				 SW_ANALYSIS_CEPSTRUM_F0, SP_FALSE) == SP_FALSE) {
	    flag = SP_FALSE;
	}
    }
    if (window->subplot_power == SP_TRUE) {
	swDestroyWindowWave(window, &window->power);

	if (swSubplotSpectrogram(window, SP_FALSE,
				 SW_ANALYSIS_POWER, SP_FALSE) == SP_FALSE) {
	    flag = SP_FALSE;
	}
    }
#ifdef SW_SUPPORT_STRAIGHT
#endif

    return flag;
}
#endif

spBool swUpdateSpectrogram(swWindow window, spBool update_subplot)
{
    spBool flag = SP_TRUE;
    
    if (window->wave == NULL) return SP_FALSE;

    if (window->draw_specgram == SP_TRUE) {
	swDestroyWindowWave(window, &window->specgram);

	flag = swDrawSpectrogram(window, window->specgram_config_flag, window->specgram_analysis_type);
    }

#ifdef SW_SUPPORT_ANALYSIS_SUBPLOT
    if (flag == SP_TRUE) {
	if (update_subplot == SP_TRUE) {
	    if (swUpdateSubplot(window,
				/*spIsTrue(window->draw_specgram == SP_TRUE && window->specgram != NULL)*/SP_FALSE) == SP_TRUE) {
		swReloadWave(window, window->wave, SP_TRUE, SP_FALSE);
	    }
	}
    }
#endif
    
    return flag;
}

void swUpdateSpectrogramCB(spComponent component, swWindow window)
{
    if (window->draw_specgram == SP_TRUE) {
	swUpdateSpectrogram(window, SP_FALSE);
    }
    
    return;
}

void swClearSpectrogramCB(spComponent component, swWindow window)
{
    if (window->draw_specgram == SP_TRUE) {
	window->draw_specgram = SP_FALSE;
        window->draw_vertical_keys = SP_FALSE;
        
	if (window->specgram != NULL) {
	    window->amp_min = 0.0; window->amp_max = -1.0;
	    swSetWaveToFirstSubArea(window, window->wave, SP_FALSE);
	    swResetWindow(window, SP_FALSE);
	}
    }
    
    return;
}

void swCheckSpectrogramDrawKeysCB(spComponent component, swWindow window)
{
    spBool set;
    
    if (spGetToggleState(component, &set) == SP_TRUE) {
	if (set != window->draw_vertical_keys) {
	    window->draw_vertical_keys = set;
            
            if (swIsSpectrogramVisible(window) == SP_TRUE) {
                swRedrawWave(window);
            }
	}

        if (window->config->draw_piano_keys_for_spectrum == SP_FALSE
            && window->related_window != NULL && window->related_window->data_type == SW_FREQ_DATA) {
            spDebug(10, "swCheckSpectrogramDrawKeysCB", "related_window->draw_horizontal_keys = %d, set = %d\n",
                    window->related_window->draw_horizontal_keys, set);
            if (window->related_window->draw_horizontal_keys != set) {
                window->related_window->draw_horizontal_keys = set;
                swUpdateWaveSubAreaSize(window->related_window);
                swDrawWave(window->related_window);
            }
        }
    }
    
    return;
}

void swDrawWideSpectrogramCB(spComponent component, swWindow window)
{
    swDrawSpectrogram(window, SW_ANALYSIS_CONFIG_FLAG_WIDE_SPECTRUM, SW_ANALYSIS_SPECTRUM);
    return;
}

void swDrawNarrowSpectrogramCB(spComponent component, swWindow window)
{
    swDrawSpectrogram(window, SW_ANALYSIS_CONFIG_FLAG_NARROW_SPECTRUM, SW_ANALYSIS_SPECTRUM);
    return;
}

void swDrawNarrowSmoothedSpectrogramCB(spComponent component, swWindow window)
{
    swDrawSpectrogram(window, SW_ANALYSIS_CONFIG_FLAG_NARROW_SMOOTHED_SPECTRUM, SW_ANALYSIS_SMOOTHED_SPECTRUM);
    return;
}

#if defined(SW_SUPPORT_CQT_SPECTROGRAM)
void swDrawCQTSpectrogramCB(spComponent component, swWindow window)
{
    swDrawSpectrogram(window, SW_ANALYSIS_CONFIG_FLAG_CQT_SPECTRUM, SW_ANALYSIS_CQT_SPECTRUM);
    return;
}
#endif

typedef enum {
    SW_FILTER_TYPE_UNKNOWN = -1,
    SW_FILTER_TYPE_LOWPASS = 0,
    SW_FILTER_TYPE_HIGHPASS = 1,
    SW_FILTER_TYPE_BANDPASS = 2,
    SW_FILTER_TYPE_BANDSTOP = 3,
} swFilterType;

struct _swFilteringDialog
{
    spComponent window;
    spComponent canvas;

    spComponent setting_area;

    spComponent filter_type_combo;
    
    spComponent hp_cutoff_slider;
    spComponent lp_cutoff_slider;
    
    spComponent sidelobe_slider;
    spComponent transition_slider;
    spComponent gain_slider;

    int setting_area_size;

    double data_width;
    double data_min;
    double data_height;
    
    double draw_width;
    double draw_height;

    double x_factor;
    double y_factor;
    
    swConfig config;
    
    swFilterType filter_type;
    double hp_cutoff;
    double lp_cutoff;
    double sidelobe;
    double transition;
    double gain;

    double samp_rate;
    
    swWave wave;
    spDVector filter;

    swWindow target_window;
};

#if 1
#define SW_FILTERING_DEFAULT_LENGTH 1024

void swPopdownFilteringDialogCB(spComponent component, swFilteringDialog dialog)
{
    spCallbackReason reason;
    
    spDebug(80, "swPopdownFilteringDialogCB", "in\n");
    
    /* popdown format dialog */
    spPopdownWindow(dialog->window);
    
    reason = spGetCallbackReason(component);
    
    if (reason == SP_CR_OK && swIsNoWave(dialog->target_window) != SP_TRUE
	&& dialog->filter != NODATA) {
	swFilteringWave(dialog->target_window->wave, dialog->filter);
    }
    
    spDebug(80, "swPopdownFilteringDialogCB", "done\n");
    
    return;
}

#define SW_FILTER_IMAGE_STRING_TOP_OFFSET 10
#define SW_FILTER_IMAGE_STRING_RIGHT_OFFSET 10
#define SW_FILTER_IMAGE_STRING_SPACING 5

static void updateFilterInfoString(swFilteringDialog dialog, int draw_width, int draw_height)
{
    int i;
    char buf[SP_MAX_MESSAGE];
    int left_offset, top_offset;
    int sx, sy, swidth, sheight;
    double value;

    spDebug(10, "updateFilterInfoString", "in\n");
    
    top_offset = SW_FILTER_IMAGE_STRING_TOP_OFFSET;
    for (i = 0; i < 3; i++) {
	spDebug(80, "updateFilterInfoString", "i = %d\n", i);
	
	if (i == 0) {
	    if (dialog->filter_type == SW_FILTER_TYPE_LOWPASS) {
		continue;
	    }
	    value = dialog->hp_cutoff * dialog->samp_rate / 2.0;
	    sprintf(buf, "%s: %7.1f [Hz]",
		    _("SW_FILTERING_DIALOG_HIGHPASS_CUTOFF"), value);
	} else if (i == 1) {
	    if (dialog->filter_type == SW_FILTER_TYPE_HIGHPASS) {
		continue;
	    }
	    value = dialog->lp_cutoff * dialog->samp_rate / 2.0;
	    sprintf(buf, "%s: %7.1f [Hz]",
		    _("SW_FILTERING_DIALOG_LOWPASS_CUTOFF"), value);
	} else {
	    value = dialog->transition * dialog->samp_rate / 2.0;
	    sprintf(buf, "%s: %7.1f [Hz]", _("SW_FILTERING_DIALOG_TRANSITION"), value);
	}
	spDebug(80, "updateFilterInfoString", "buf = %s\n", buf);
	
	if (swGetMainStringExtent(dialog->canvas, buf, &sx, &sy, &swidth, &sheight) == SP_TRUE) {
	    spDebug(80, "updateFilterInfoString", "sx = %d, sy = %d, swidth = %d, sheight = %d\n",
		    sx, sy, swidth, sheight);
	    top_offset += sheight;
	    left_offset = draw_width - swidth - SW_FILTER_IMAGE_STRING_RIGHT_OFFSET;
	    swDrawMainString(dialog->canvas, left_offset, top_offset, buf);
	} else {
	    break;
	}

        top_offset += SW_FILTER_IMAGE_STRING_SPACING;
    }
    
    spDebug(10, "updateFilterInfoString", "done\n");
    
    return;
}

static void updateFilterImage(swFilteringDialog dialog)
{
    double y_zero_offset;
    int width, height;
    double x_samp_dim_factor;

    if (spGetClientSize(dialog->canvas, &width, &height) == SP_FALSE) return;
    
    spDebug(10, "updateFilterImage", "width = %d, height = %d\n", width, height);
    
    swFillBackground(dialog->canvas, width, height);

    if (dialog->wave != NULL) {
	dialog->x_factor = (double)width / (double)dialog->wave->length;
        
        x_samp_dim_factor = dialog->wave->length >= 2 ? (dialog->samp_rate/2.0) / (double)(dialog->wave->length - 1) : 1.0;
	swDrawHScaleMain(dialog->config, dialog->canvas, SW_FREQ_DATA, 1, 0,
                         0, 0, width, 0, height, 0, 0, height,
			 0.0, /*1.0*/dialog->samp_rate/2.0, x_samp_dim_factor, 
                         dialog->config->log_frequency_axis, SP_FALSE);
	
	swDrawVScaleMain(dialog->config, dialog->canvas, SW_FREQ_DATA, 1, 0,
                         1, width, 0, height, 0, 0, height,
			 dialog->data_min, dialog->data_min + dialog->data_height, 1.0,
			 "Magnitude [dB]", SP_TRUE, SP_FALSE, SP_FALSE,
                         &dialog->y_factor, &y_zero_offset, NULL, NULL);
	
	swDrawWaveformLine(dialog->canvas, dialog->wave, 0, 0, 0,
                           0, width, 0, height, dialog->x_factor, dialog->y_factor, y_zero_offset,
			   0, dialog->wave->length, dialog->config->log_frequency_axis, SP_FALSE, SP_FALSE);

	updateFilterInfoString(dialog, width, height);
    }
	
    spDebug(10, "updateFilterImage", "before spRefreshCanvas\n");
    
    spRefreshCanvas(dialog->canvas);

    spDebug(10, "updateFilterImage", "done\n");
    
    return;
}

static void updateComponentsState(swFilteringDialog dialog)
{
    int hp_cutoff_value, lp_cutoff_value;
    
    if (dialog->filter_type == SW_FILTER_TYPE_BANDPASS) {
	spSetSensitive(dialog->hp_cutoff_slider, SP_TRUE);
	spSetSensitive(dialog->lp_cutoff_slider, SP_TRUE);

	if (spGetSliderValue(dialog->hp_cutoff_slider, &hp_cutoff_value)
	    && spGetSliderValue(dialog->lp_cutoff_slider, &lp_cutoff_value)
	    && hp_cutoff_value >= lp_cutoff_value) {
	    hp_cutoff_value = MAX(lp_cutoff_value - 10, 0);
	    spSetSliderValue(dialog->hp_cutoff_slider, hp_cutoff_value);
	    dialog->hp_cutoff = (double)hp_cutoff_value / 100.0;
	}
    } else if (dialog->filter_type == SW_FILTER_TYPE_BANDSTOP) {
	spSetSensitive(dialog->hp_cutoff_slider, SP_TRUE);
	spSetSensitive(dialog->lp_cutoff_slider, SP_TRUE);
	
	if (spGetSliderValue(dialog->hp_cutoff_slider, &hp_cutoff_value)
	    && spGetSliderValue(dialog->lp_cutoff_slider, &lp_cutoff_value)
	    && hp_cutoff_value <= lp_cutoff_value) {
	    hp_cutoff_value = MIN(lp_cutoff_value + 10, 100);
	    spSetSliderValue(dialog->hp_cutoff_slider, hp_cutoff_value);
	    dialog->hp_cutoff = (double)hp_cutoff_value / 100.0;
	}
    } else if (dialog->filter_type == SW_FILTER_TYPE_HIGHPASS) {
	spSetSensitive(dialog->hp_cutoff_slider, SP_TRUE);
	spSetSensitive(dialog->lp_cutoff_slider, SP_FALSE);
    } else {
	spSetSensitive(dialog->hp_cutoff_slider, SP_FALSE);
	spSetSensitive(dialog->lp_cutoff_slider, SP_TRUE);
    }
    
    return;
}
     
static void updateFilter(swFilteringDialog dialog)
{
    if (dialog->filter != NODATA) {
	xdvfree(dialog->filter);
    }

    if (dialog->filter_type == SW_FILTER_TYPE_BANDPASS) {
	dialog->filter = xdvbandpass(dialog->hp_cutoff, dialog->lp_cutoff,
				     dialog->sidelobe, dialog->transition, dialog->gain);
    } else if (dialog->filter_type == SW_FILTER_TYPE_BANDSTOP) {
	dialog->filter = xdvbandstop(dialog->lp_cutoff, dialog->hp_cutoff, 
				      dialog->sidelobe, dialog->transition, dialog->gain);
    } else if (dialog->filter_type == SW_FILTER_TYPE_HIGHPASS) {
	dialog->filter = xdvhighpass(dialog->hp_cutoff, 
				     dialog->sidelobe, dialog->transition, dialog->gain);
    } else {
	dialog->filter = xdvlowpass(dialog->lp_cutoff,
				    dialog->sidelobe, dialog->transition, dialog->gain);
    }
    spDebug(10, "updateFilter", "filter length = %ld\n", dialog->filter->length);

    if (dialog->wave != NULL) {
	swDestroyWave(dialog->wave);
    }

    dialog->wave = swAnalysisData(dialog->config->wave_config, dialog->filter->data, dialog->filter->length,
				  SW_ANALYSIS_SPECTRUM, SW_WINDOW_RECTANGLE,
				  2.0, 1024, dialog->config->lifterm, SP_TRUE, SP_TRUE);
    spDebug(10, "updateFilter", "wave->length = %ld, wave->total_length = %ld\n",
	    dialog->wave->length, dialog->wave->total_length);

    updateFilterImage(dialog);
    
    
    return;
}

void swFilteringCanvasDrawCB(spComponent component, swFilteringDialog dialog)
{
    spDebug(80, "swFilteringCanvasDrawCB", "in\n");
    
    if (dialog->canvas == NULL) {
	dialog->canvas = component;
    }
    if (dialog->filter == NODATA || dialog->wave == NULL) {
	updateFilter(dialog);
    } else {
	updateFilterImage(dialog);
    }
    
    spDebug(80, "swFilteringCanvasDrawCB", "done\n");
    
    return;
}

static void selectFilterTypeCB(spComponent component, swFilteringDialog dialog)
{
    int index;

    if (dialog->canvas == NULL) return;

    if ((index = spGetSelectedListIndex(component)) >= 0) {
	spDebug(10, "selectFilterTypeCB", "index = %d\n", index);
	dialog->filter_type = (swFilterType)index;
	
	updateComponentsState(dialog);
	updateFilter(dialog);
    }
    
    return;
}

static void sliderCB(spComponent component, swFilteringDialog dialog)
{
    int value;
    const char *name;

    if (spGetSliderValue(component, &value) == SP_TRUE) {
	name = spGetName(component);
    
	spDebug(10, "sliderCB", "name = %s, value = %d\n", name, value);

	if (streq(name, "hpCutoffSlider")) {
	    dialog->hp_cutoff = (double)value / 100.0;
	} else if (streq(name, "lpCutoffSlider")) {
	    dialog->lp_cutoff = (double)value / 100.0;
	} else if (streq(name, "sidelobeSlider")) {
	    dialog->sidelobe = (double)value;
	} else if (streq(name, "transitionSlider")) {
	    dialog->transition = (double)value / 100.0;
	}

	updateFilter(dialog);
    }

    return;
}

static char *filter_type_strings[] =
{
    "Lowpass Filter",
    "Highpass Filter",
    "Bandpass Filter",
    "Bandstop Filter",
    NULL,
};

static swFilteringDialog createFilteringDialog(swConfig config)
{
    swFilteringDialog dialog;
    int field_offset;
    int field_size;

    dialog = xalloc(1, struct _swFilteringDialog);
    memset(dialog, 0, sizeof(struct _swFilteringDialog));
    dialog->canvas = NULL;
    dialog->config = config;

    dialog->setting_area_size = /*290*/380;
    dialog->data_width = 1.0;
    dialog->data_min = -100.0;
    dialog->data_height = 120.0;
    dialog->draw_width = 400;
    dialog->draw_height = 400;
    dialog->filter_type = SW_FILTER_TYPE_BANDPASS;
    dialog->hp_cutoff = 0.0;
    dialog->lp_cutoff = 1.0;
    dialog->sidelobe = 80.0;
    dialog->transition = 0.05;
    dialog->gain = 1.0;
    dialog->filter = NODATA;
    dialog->samp_rate = config->samp_rate;
    dialog->target_window = NULL;

    field_offset = /*100*/130;
    field_size = /*160*/220;
    
    dialog->window = spCreateDialogBox("filteringDialog",
				       SppTitle, _("SW_FILTERING_DIALOG_TITLE"),
				       SppCallbackFunc, swPopdownFilteringDialogCB,
				       SppCallbackData, dialog,
				       SppDialogBoxButtonType, SP_DB_OK_CANCEL,
				       SppCloseStyle, SP_UNMAP_CLOSE,
				       SppSpacingOn, SP_TRUE,
				       SppOrientation, SP_HORIZONTAL,
				       SppHelpButtonVisible, SP_TRUE,
				       SppHelpPath, "dialog/filtering.html",
				       NULL);
    spCreateCanvas(dialog->window, "filteringCanvas",
		   (int)dialog->draw_width, (int)dialog->draw_height,
		   SppBorderOn, SP_TRUE,
		   SppUseArrowKey, SP_TRUE,
		   SppWidth, (int)dialog->draw_width,
		   SppHeight, (int)dialog->draw_height,
		   SppCallbackFunc, swFilteringCanvasDrawCB,
		   SppCallbackData, dialog,
		   NULL);

    dialog->setting_area = spCreateBox(dialog->window, "filteringSettingArea", dialog->setting_area_size,
				       SppInitialWidth, dialog->setting_area_size,
				       SppInitialHeight, (int)dialog->draw_height,
				       NULL);
    
    dialog->filter_type_combo = spCreateParamField(dialog->setting_area, "filterTypeCombo", 0,
						   SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
						   SppHelpPath, "dialog/filtering.html#filter_type",
						   SppTitle, _("SW_FILTERING_DIALOG_FILTER_TYPE"),
						   SppCallbackFunc, selectFilterTypeCB,
						   SppCallbackData, dialog,
						   SppEditable, SP_FALSE,
						   SppFieldStrings, filter_type_strings,
						   SppFieldOffset, field_offset,
						   SppFieldSize, field_size,
						   NULL);
    spSelectListIndex(dialog->filter_type_combo, (int)dialog->filter_type);
    dialog->hp_cutoff_slider = spCreateParamField(dialog->setting_area, "hpCutoffSlider", 0,
						  SppFieldType, SP_FIELD_TYPE_TRACK_BAR_WITH_TEXT,
						  SppHelpPath, "dialog/filtering.html#hp_cutoff",
						  SppTitle, _("SW_FILTERING_DIALOG_HIGHPASS_CUTOFF"),
						  SppDimension, "%",
						  SppCallbackFunc, sliderCB,
						  SppCallbackData, dialog,
						  SppTrackCallbackOn, SP_TRUE,
						  SppMinimum, 0,
						  SppMaximum, 99,
						  SppShowScale, SP_TRUE,
						  SppShowValue, SP_TRUE,
						  SppValue, (int)spRound(dialog->hp_cutoff * 100.0),
						  SppFieldOffset, field_offset,
						  SppFieldSize, field_size,
						  SppDimensionSize, 30,
						  NULL);
    dialog->lp_cutoff_slider = spCreateParamField(dialog->setting_area, "lpCutoffSlider", 0,
						  SppFieldType, SP_FIELD_TYPE_TRACK_BAR_WITH_TEXT,
						  SppHelpPath, "dialog/filtering.html#lp_cutoff",
						  SppTitle, _("SW_FILTERING_DIALOG_LOWPASS_CUTOFF"),
						  SppDimension, "%",
						  SppCallbackFunc, sliderCB,
						  SppCallbackData, dialog,
						  SppTrackCallbackOn, SP_TRUE,
						  SppMinimum, 1,
						  SppMaximum, 100,
						  SppShowScale, SP_TRUE,
						  SppShowValue, SP_TRUE,
						  SppValue, (int)spRound(dialog->lp_cutoff * 100.0),
						  SppFieldOffset, field_offset,
						  SppFieldSize, field_size,
						  SppDimensionSize, 30,
						  NULL);
    dialog->sidelobe_slider = spCreateParamField(dialog->setting_area, "sidelobeSlider", 0,
						 SppFieldType, SP_FIELD_TYPE_TRACK_BAR_WITH_TEXT,
						 SppHelpPath, "dialog/filtering.html#sidelobe",
						 SppTitle, SW_SFC_SIDELOBE_LABEL,
						 SppDimension, "dB",
						 SppCallbackFunc, sliderCB,
						 SppCallbackData, dialog,
						 SppTrackCallbackOn, SP_TRUE,
						 SppMinimum, 30,
						 SppMaximum, 150,
						 SppShowScale, SP_TRUE,
						 SppShowValue, SP_TRUE,
						 SppValue, (int)spRound(dialog->sidelobe),
						 SppFieldOffset, field_offset,
						 SppFieldSize, field_size,
						 SppDimensionSize, 30,
						 NULL);
    dialog->transition_slider = spCreateParamField(dialog->setting_area, "transitionSlider", 0,
						   SppFieldType, SP_FIELD_TYPE_TRACK_BAR_WITH_TEXT,
						   SppHelpPath, "dialog/filtering.html#transition",
						   SppTitle, _("SW_FILTERING_DIALOG_TRANSITION"),
						   SppDimension, "%",
						   SppCallbackFunc, sliderCB,
						   SppCallbackData, dialog,
						   SppTrackCallbackOn, SP_TRUE,
						   SppMinimum, 1,
						   SppMaximum, 30,
						   SppShowScale, SP_TRUE,
						   SppShowValue, SP_TRUE,
						   SppValue, (int)spRound(dialog->transition * 100.0),
						   SppFieldOffset, field_offset,
						   SppFieldSize, field_size,
						   SppDimensionSize, 30,
						   NULL);

    return dialog;
}

void swPopupFilteringDialogCB(spComponent component, swWindow window)
{
    static swFilteringDialog dialog = NULL;

    if (swIsNoWave(window) == SP_TRUE) {
	return;
    }

    if (dialog == NULL) {
	dialog = createFilteringDialog(window->config);
    }
    dialog->samp_rate = window->wave->samp_rate;
    dialog->target_window = window;

    spDebug(80, "swPopupFilteringDialogCB", "before spPopupWindow\n");
    
    /* popup dialog */
    spPopupWindow(dialog->window);
    
    spDebug(80, "swPopupFilteringDialogCB", "after spPopupWindow\n");
    
    return;
}
#endif

#endif

