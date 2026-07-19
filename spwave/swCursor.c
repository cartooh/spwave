#include <stdio.h>
#include <stdlib.h>

#include <sp/spBaseLib.h>
#include <sp/spComponentLib.h>

#include "swWave.h"
#include "swWaveAudio.h"
#include "swWindow.h"
#include "swDraw.h"
#include "swEdit.h"
#include "swLabelDialog.h"
#include "swLabelList.h"
#include "swCursor.h"
#ifdef SW_SUPPORT_MORPHING
#include "swMorphingDraw.h"
#endif

spBool swGetCallbackMousePosition(spComponent component, swWindow window, spBool overview_flag, int *x, int *y)
{
    spBool flag;
    
    if (window == NULL) return SP_FALSE;

    if ((flag = spGetCallbackMousePosition(component, x, y)) == SP_TRUE) {
        if (overview_flag == SP_FALSE) {
            if (window->draw_vertical_keys == SP_TRUE
                && window->config->vertical_piano_keys_right == SP_FALSE) {
                //*x -= SW_PIANO_KEYS_DEFAULT_SIZE;
                *x -= (int)spRound(window->vertical_keys_width);
            }
            if (window->draw_height_horizontal_keys > 0.0
                && window->config->horizontal_piano_keys_top == SP_TRUE) {
                *y -= (int)spRound(window->draw_height_horizontal_keys);
            }
        }
    }

    return flag;
}

double swCustomXMinLogValue(DVector custom_x_axis)
{
    if (custom_x_axis->data[0] > 0.0) {
        return custom_x_axis->data[0];
    } else {
        if (custom_x_axis->length < 3) {
            if (custom_x_axis->length == 2 && custom_x_axis->data[1] < SW_LOG_FREQUENCY_MIN_VALUE) {
                return log10(SW_LOG_FREQUENCY_MIN_VALUE / 2.0);
            } else {
                return log10(SW_LOG_FREQUENCY_MIN_VALUE);
            }
        } else {
            double log_data1, log_data2;
            double log_data0;
            double logdiff;

            log_data1 = log10(custom_x_axis->data[1]);
            log_data2 = log10(custom_x_axis->data[2]);
            
            logdiff = log_data2 - log_data1;
            log_data0 = log_data1 - logdiff;
            log_data0 = MAX(log_data0, log10(SW_LOG_FREQUENCY_MIN_VALUE));
            
            return log_data0;
        }
    }
}

spLong swXValueToCustomXIndex(DVector custom_x_axis, spLong start_pos, spLong end_pos, double x_value, spBool log_flag, double *o_index_d)
{
    spLong k;
    double center;
    double x_axis_value;
    double prev_x_axis_value;
    double fraction;

    /*end_pos = MIN(end_pos, custom_x_axis->length - 1);*/
    end_pos = custom_x_axis->length - 1;
    
    if (log_flag == SP_TRUE) {
        if (start_pos <= 0) {
            prev_x_axis_value = swCustomXMinLogValue(custom_x_axis);
        } else {
            prev_x_axis_value = log10(MAX(custom_x_axis->data[start_pos], SW_LOG_FREQUENCY_MIN_VALUE));
        }
    } else {
        prev_x_axis_value = custom_x_axis->data[start_pos];
    }
    spDebug(100, "swXValueToCustomXIndex", "x_value = %f, prev_x_axis_value = %f, start_pos = %ld\n",
            x_value, prev_x_axis_value, start_pos);

    fraction = 0.0;

    if (x_value < prev_x_axis_value && start_pos > 0) {
        for (k = start_pos; k > 0; k--) {
            if (log_flag == SP_TRUE) {
                x_axis_value = log10(custom_x_axis->data[k - 1]);
            } else {
                x_axis_value = custom_x_axis->data[k - 1];
            }
            center = (prev_x_axis_value + x_axis_value) / 2.0;
        
            if (center < x_value) {
                if (o_index_d != NULL) {
                    fraction = (x_value - prev_x_axis_value) / (prev_x_axis_value - x_axis_value);
                }
                spDebug(100, "swXValueToCustomXIndex", "reverse order: center = %f, x_value = %f, k = %ld / %ld\n",
                        center, x_value, k, custom_x_axis->length);
                break;
            }
            prev_x_axis_value = x_axis_value;
        }
    } else {
        for (k = start_pos; k < end_pos; k++) {
            if (log_flag == SP_TRUE) {
                x_axis_value = log10(custom_x_axis->data[k + 1]);
            } else {
                x_axis_value = custom_x_axis->data[k + 1];
            }
            center = (prev_x_axis_value + x_axis_value) / 2.0;
        
            if (center >= x_value) {
                if (o_index_d != NULL) {
                    fraction = (x_value - prev_x_axis_value) / (x_axis_value - prev_x_axis_value);
                }
                spDebug(100, "swXValueToCustomXIndex", "center = %f (%f-%f), x_value = %f, k = %ld / %ld\n",
                        center, prev_x_axis_value, x_axis_value, x_value, k, custom_x_axis->length);
                break;
            }
            prev_x_axis_value = x_axis_value;
        }
    }

    if (o_index_d != NULL) {
        *o_index_d = (double)k + fraction;
    }
    
    spDebug(100, "swXValueToCustomXIndex", "done: k = %ld / %ld\n", k, custom_x_axis->length);
    
    return k;
}

static int sampToDrawWidthLog(DVector custom_x_axis, int draw_width, spLong offset, spLong length, double samp)
{
    double log_region_min, log_region_max;
    double log_region_range;
    double log_samp;

    if (custom_x_axis != NODATA) {
        long samp_l;
        double min_log_value;
        double x_axis_value;
        double next_x_axis_value;
        double fraction;

        min_log_value = swCustomXMinLogValue(custom_x_axis);
        
        if (offset <= 0) {
            log_region_min = min_log_value;
        } else {
            log_region_min = log10(custom_x_axis->data[offset]);
        }
        log_region_max = log10(custom_x_axis->data[offset + length - 1]);
        samp_l = (long)samp;
        if (samp_l <= 0) {
            x_axis_value = min_log_value;
        } else {
            x_axis_value = log10(custom_x_axis->data[samp_l]);
        }
        next_x_axis_value = log10(custom_x_axis->data[MIN(samp_l + 1, custom_x_axis->length - 1)]);
        fraction = (samp - (double)samp_l) * (next_x_axis_value - x_axis_value);
        log_samp = x_axis_value + fraction;
        spDebug(100, "sampToDrawWidthLog", "samp = %f, fraction = %f, log_samp = %f\n",
                samp, fraction, log_samp);
    } else {
        log_region_min = log10(MAX((double)offset, SW_LOG_FREQUENCY_MIN_VALUE));
        log_region_max = log10((double)(offset + length - 1));
        log_samp = log10(MAX(samp, SW_LOG_FREQUENCY_MIN_VALUE));
    }
    log_region_range = log_region_max - log_region_min;

    return (int)((log_samp - log_region_min) * (double)draw_width / log_region_range + 0.5);
}

static int customXIndexToDrawWidthLinear(DVector custom_x_axis, int draw_width, spLong offset, spLong length, double index)
{
    spLong index_l;
    spLong end_pos;
    double range;
    double x_factor;
    double x_axis_value;
    double next_x_axis_value;
    double fraction;

    end_pos = offset + MAX(length - 1, 1);
    end_pos = MIN(end_pos, custom_x_axis->length - 1);
    range = custom_x_axis->data[end_pos] - custom_x_axis->data[offset];
    x_factor = (double)draw_width / (double)range;
    index_l = (spLong)index;
    x_axis_value = custom_x_axis->data[index_l];
    next_x_axis_value = custom_x_axis->data[MIN(index_l + 1, custom_x_axis->length - 1)];
    fraction = (index - (double)index_l) * (next_x_axis_value - x_axis_value);

    return (int)spRound(x_factor * (x_axis_value - custom_x_axis->data[offset] + fraction));
}

int swSampToOverviewDisp(swWindow window, spLong samp)
{
    int disp;
    
    if (window == NULL || window->wave == NULL || samp < 0)
	return -1;

    if (window->wave->total_length <= 1) return 0;
    
    if (swIsXAxisLogFrequency(window) == SP_TRUE && window->wave->total_length >= 2) {
        spDebug(100, "swSampToOverviewDisp", "log: window->overview_width = %d, window->wave->total_length = %ld, samp = %ld\n",
                window->overview_width, window->wave->total_length, samp);
        disp = sampToDrawWidthLog(window->wave->custom_x_axis, window->overview_width, 0, window->wave->total_length, (double)samp);
    } else {
        if (window->wave->custom_x_axis != NODATA && window->wave->num_order <= 1) {
            disp = customXIndexToDrawWidthLinear(window->wave->custom_x_axis, window->overview_width, 0,
                                                 window->wave->total_length, (double)samp);
        } else {
            disp = (int)((double)samp * (double)window->overview_width
                         / (double)(window->wave->total_length - 1) + 0.5);
        }
    }

    spDebug(100, "swSampToOverviewDisp", "done: disp = %d\n", disp);
    
    return disp;
}

int swSampToDrawWidth(swWindow window, int draw_width, double samp)
{
    if (window == NULL || window->wave == NULL || samp < 0.0)
	return -1;

    if (window->length <= 1) return 0;
    
    if (swIsXAxisLogFrequency(window) == SP_TRUE && window->length >= 2) {
        return sampToDrawWidthLog(window->wave->custom_x_axis, draw_width, window->offset, window->length, samp);
    } else {
        spDebug(100, "swSampToDrawWidth", "draw_width = %d, samp = %f, offset = %ld, length = %ld\n",
                draw_width, samp, window->offset, window->length);
        if (window->wave->custom_x_axis != NODATA && window->wave->num_order <= 1) {
            return customXIndexToDrawWidthLinear(window->wave->custom_x_axis, draw_width, window->offset, window->length, samp);
        } else {
            return (int)((samp - (double)window->offset) * (double)draw_width
                         / (double)(window->length - 1) + 0.5);
        }
    }
}

int swSampToDisp(swWindow window, spLong samp)
{
    int draw_width;

    draw_width = swGetDrawWidth(window, SP_TRUE);
    
    return swSampToDrawWidth(window, draw_width, (double)samp);
}

static spLong drawWidthToCustomXIndexLinear(DVector custom_x_axis, int draw_width, spLong offset, spLong length, int disp)
{
    spLong k;
    spLong end_pos;
    double range;
    double x_factor;
    double x_value;

    end_pos = offset + MAX(length - 1, 1);
    end_pos = MIN(end_pos, custom_x_axis->length - 1);
    range = custom_x_axis->data[end_pos] - custom_x_axis->data[offset];
    x_factor = (double)draw_width / (double)range;
            
    x_value = (double)disp / x_factor + custom_x_axis->data[offset];
    k = swXValueToCustomXIndex(custom_x_axis, offset, end_pos, x_value, SP_FALSE, NULL);
    spDebug(100, "drawWidthToCustomXIndexLinear", "disp = %d, x_value = %f, k = %ld / %ld, end_pos = %ld\n",
            disp, x_value, k, custom_x_axis->length, end_pos);
    
    return k;
}

static spLong drawWidthToSampLog(swWindow window, DVector custom_x_axis, int draw_width, spLong offset, spLong length, int disp)
{
    int disp_samp1_boundary;
    double log_region_min, log_region_max;
    double log_region_range;
    double log_samp;

    disp_samp1_boundary = swSampToDrawWidth(window, draw_width,
                                            SW_LOG_FREQUENCY_MIN_BOUNDARY_VALUE);
    spDebug(100, "drawWidthToSampLog", "disp = %d, disp_samp1_boundary = %d\n",
            disp, disp_samp1_boundary);
        
    if (0 && disp <= disp_samp1_boundary) {
        spDebug(80, "drawWidthToSampLog", "return: disp (%d) <= disp_samp1_boundary (%d)\n",
                disp, disp_samp1_boundary);
        return 0;
    } else {
        spLong end_pos;
            
        end_pos = offset + length - 1;
            
        if (custom_x_axis != NODATA) {
            end_pos = MIN(end_pos, custom_x_axis->length - 1);
            if (offset <= 0) {
                log_region_min = swCustomXMinLogValue(custom_x_axis);
            } else {
                log_region_min = log10(MAX(custom_x_axis->data[offset], SW_LOG_FREQUENCY_MIN_VALUE));
            }
            log_region_max = log10(custom_x_axis->data[end_pos]);
        } else {
            log_region_min = log10(MAX((double)offset, SW_LOG_FREQUENCY_MIN_VALUE));
            log_region_max = log10((double)end_pos);
        }
        
        log_region_range = log_region_max - log_region_min;

        log_samp = log_region_min + (double)disp * log_region_range / (double)draw_width;
            
        if (custom_x_axis != NODATA) {
            return swXValueToCustomXIndex(custom_x_axis, offset, end_pos, log_samp, SP_TRUE, NULL);
        } else {
            double samp_d;

            samp_d = pow(10.0, log_samp);
            spDebug(100, "drawWidthToSampLog", "log_samp = %f, samp_d = %f\n", log_samp, samp_d);
            if (samp_d < 0.75) {
                return 0;
            } else {
                return (spLong)(samp_d + 0.5);
            }
        }
    }
}

spLong swDrawWidthToSamp(swWindow window, int draw_width, int disp)
{
    if (window == NULL || window->wave == NULL)
	return -1;
	
    spDebug(100, "swDrawWidthToSamp",
	    "draw_width = %d, disp = %d, window->offset = %ld, window->length = %ld\n",
	    draw_width, disp, (long)window->offset, (long)window->length);
    
    if (swIsXAxisLogFrequency(window) == SP_TRUE && window->length >= 2) {
        return drawWidthToSampLog(window, window->wave->custom_x_axis, draw_width, window->offset, window->length, disp);
    } else {
        if (window->wave->custom_x_axis != NODATA && window->wave->num_order <= 1) {
            return drawWidthToCustomXIndexLinear(window->wave->custom_x_axis, draw_width, window->offset, window->length, disp);
        } else {
            return (spLong)((double)disp * (double)(window->length - 1) /
                            (double)draw_width + (double)window->offset + 0.5);
        }
    }
}

spLong swDispToSamp(swWindow window, int disp)
{
    int draw_width;

    draw_width = swGetDrawWidth(window, SP_TRUE);
    
    return swDrawWidthToSamp(window, draw_width, disp);
}

spLong swOverviewDispToSamp(swWindow window, int disp)
{
    if (window == NULL || window->wave == NULL)
	return -1;
	
    if (swIsXAxisLogFrequency(window) == SP_TRUE && window->wave->total_length >= 2) {
        return drawWidthToSampLog(window, window->wave->custom_x_axis, window->overview_width, 0, window->wave->total_length, disp);
    } else {
        if (window->wave->custom_x_axis != NODATA && window->wave->num_order <= 1) {
            return drawWidthToCustomXIndexLinear(window->wave->custom_x_axis, window->overview_width,
                                                 0, window->wave->total_length, disp);
        } else {
            return (spLong)((double)disp * (double)(window->wave->total_length - 1) /
                            (double)window->overview_width + 0.5);
        }
    }
}

double swTargetSampToDim(swWindow window, swWave wave, spBool to_draw, spLong samp)
{
    double dim;
    
    if (window == NULL || wave == NULL || samp < 0)
	return -1.0;

    if (wave->custom_x_axis != NODATA && wave->num_order <= 1) {
        if (samp >= wave->custom_x_axis->length) {
            dim = wave->custom_x_axis->data[wave->custom_x_axis->length - 1];
        } else {
            if (to_draw == SP_TRUE && (samp <= 0 && wave->custom_x_axis->data[samp] <= 0.0 && swIsXAxisLogFrequency(window) == SP_TRUE)) {
                double log_min;
                log_min = swCustomXMinLogValue(wave->custom_x_axis);
                dim = pow(10.0, log_min);
            } else {
                dim = wave->custom_x_axis->data[samp];
            }
        }
        spDebug(100, "swTargetSampToDim", "custom: samp = %ld / %ld, dim = %f\n", samp, wave->custom_x_axis->length, dim);
    } else if (window->data_type == SW_FREQ_DATA) {
	if (window->length <= 1) {
	    dim = 0.0;
	} else {
	    dim = ((double)samp * wave->samp_rate / 2.0 / (double)(wave->total_length - 1));
	}
    } else {
	dim = ((double)samp / (double)(wave->samp_rate));
    }

    return dim;
}

double swSampToDim(swWindow window, spLong samp)
{
    if (window == NULL || window->wave == NULL)
	return -1.0;
    
    return swTargetSampToDim(window, window->wave, SP_FALSE, samp);
}

/* The return value is affected by current format setting. */
double swTargetSampToCurrentDim(swWindow window, swWave wave, spLong samp)
{
    double dim = -1.0;
    
    if (window == NULL) return -1.0;

    if (window->data_type == SW_FREQ_DATA) {
	if (window->config->freq_format == SW_FREQ_FORMAT_KHZ) {
	    dim = (double)swTargetSampToDim(window, wave, SP_TRUE, samp) / 1000.0;
	} else if (window->config->freq_format == SW_FREQ_FORMAT_POINT) {
	    dim = (double)samp;
	} else {
	    dim = (double)swTargetSampToDim(window, wave, SP_TRUE, samp);
	}
    } else {
	if (window->config->time_format == SW_TIME_FORMAT_MSEC
	    || window->config->time_format == SW_TIME_FORMAT_FLOORED_MSEC) {
	    dim = (double)swTargetSampToDim(window, wave, SP_TRUE, samp) * 1000.0;
	} else if (window->config->time_format == SW_TIME_FORMAT_POINT) {
	    dim = (double)samp;
	} else {
	    dim = (double)swTargetSampToDim(window, wave, SP_TRUE, samp);
	}
    }

    return dim;
}

double swSampToCurrentDim(swWindow window, spLong samp)
{
    if (window == NULL || window->wave == NULL)
	return -1.0;

    return swTargetSampToCurrentDim(window, window->wave, samp);
}

static spLong dimToFractionalCustomXIndex(DVector custom_x_axis, spLong offset, spLong length, double dim,
                                          spBool log_flag, double *o_index_d)
{
    spLong k;

    if (log_flag == SP_TRUE) {
        dim = log10(MAX(dim, SW_LOG_FREQUENCY_MIN_VALUE));
    }

    k = swXValueToCustomXIndex(custom_x_axis, offset, offset + length - 1, dim, log_flag, o_index_d);
    
    return k;
}

spLong swDimToTargetSamp(swWindow window, swWave wave, double dim)
{
    if (window == NULL || wave == NULL || dim < 0)
	return -1;
    
    if (wave->custom_x_axis != NODATA && wave->num_order <= 1) {
        return dimToFractionalCustomXIndex(wave->custom_x_axis, 0, wave->total_length, dim,
                                           swIsXAxisLogFrequency(window), NULL);
    } else if (window->data_type == SW_FREQ_DATA) {
	return ((spLong)((dim * (double)(wave->total_length - 1)
                          / (wave->samp_rate / 2.0)) + 0.5));
    } else {
	/*return ((long)((double)dim * (double)(wave->samp_rate) + 0.5));*/
	return ((spLong)(0.5 + (dim * wave->samp_rate)));
    }
}

double swDimToFractionalTargetSamp(swWindow window, swWave wave, double dim)
{
    if (window == NULL || wave == NULL || dim < 0)
	return -1.0;
    
    if (wave->custom_x_axis != NODATA && wave->num_order <= 1) {
        double samp_d;
        
        dimToFractionalCustomXIndex(wave->custom_x_axis, 0, wave->total_length, dim,
                                    swIsXAxisLogFrequency(window), &samp_d);

        return samp_d;
    } else if (window->data_type == SW_FREQ_DATA) {
	return dim * (double)(wave->total_length - 1) / (wave->samp_rate / 2.0);
    } else {
	return dim * wave->samp_rate;
    }
}

spLong swDimToSamp(swWindow window, double dim)
{
    if (window == NULL || window->wave == NULL)
	return -1;
    
    return swDimToTargetSamp(window, window->wave, dim);
}

/* convert a dimension into an accurate display position */
int swDimToDrawWidth(swWindow window, int draw_width, double dim)
{
    double samp;
    
    if (window == NULL || window->wave == NULL || dim < 0)
	return -1;
    
    if (window->data_type == SW_FREQ_DATA) {
	samp = (double)dim * (double)(window->wave->total_length - 1) / (window->wave->samp_rate / 2.0);
    } else {
	samp = (double)dim * (double)(window->wave->samp_rate);
    }

#if 0
    return (int)((samp - (double)window->offset) * (double)draw_width
		 / (double)(window->length - 1) + 0.5);

#else
    return swSampToDrawWidth(window, draw_width, samp);
#endif
}

int swDimToDisp(swWindow window, double dim)
{
    int draw_width;

    draw_width = swGetDrawWidth(window, SP_TRUE);
    
    return swDimToDrawWidth(window, draw_width, dim);
}

int swLengthToDrawWidth(swWindow window, int draw_width, spLong length)
{
    if (window == NULL || window->wave == NULL || length < 0)
	return -1;

    if (window->length <= 1) return 0;
    
    return (int)(((double)length) * (double)draw_width
                 / (double)(window->length - 1) + 0.5);
}

int swLengthToDisp(swWindow window, spLong length)
{
    int draw_width;

    draw_width = swGetDrawWidth(window, SP_TRUE);
    
    return swLengthToDrawWidth(window, draw_width, length);
}

#if 0
spLong swDrawWidthToLength(swWindow window, int draw_width, int disp)
{
    if (window == NULL || window->wave == NULL)
	return -1;
	
    return (spLong)((double)disp * (double)(window->length - 1) /
		    (double)draw_width + 0.5);
}

spLong swDispToLength(swWindow window, int disp)
{
    int draw_width;

    draw_width = swGetDrawWidth(window, SP_TRUE);
    
    return swDrawWidthToLength(window, draw_width, disp);
}
#endif

spLong swSampToTargetSamp(swWindow window, swWave wave, spLong samp)
{
    double dim;
    spLong osamp;
    
    if (window == NULL || wave == NULL || window->wave == NULL || samp < 0)
	return -1;

    if (window->wave == wave) return samp;

    /*if (window->wave->total_length <= 1) return 0;*/

    dim = swSampToDim(window, samp);
    osamp = swDimToTargetSamp(window, wave, dim);

    spDebug(100, "swSampToTargetSamp", "samp = %ld, dim = %f, osamp = %ld\n", samp, dim, osamp);
    
    return osamp;
}

double swSampToFractionalTargetSamp(swWindow window, swWave wave, spLong samp)
{
    double dim;
    double osamp;
    
    if (window == NULL || wave == NULL || window->wave == NULL || samp < 0)
	return -1.0;

    if (window->wave == wave) return (double)samp;

    dim = swSampToDim(window, samp);
    osamp = swDimToFractionalTargetSamp(window, wave, dim);

    /*spDebug(80, "swSampToFractionalTargetSamp", "samp = %ld, osamp = %f\n", samp, osamp);*/
    
    return osamp;
}

spLong swTargetSampToSamp(swWindow window, swWave wave, spLong samp)
{
    double dim;
    spLong osamp;
    
    if (window == NULL || wave == NULL || window->wave == NULL || samp < 0)
	return -1;

    if (window->wave == wave) return samp;

    /*if (window->wave->total_length <= 1) return 0;*/

    dim = swTargetSampToDim(window, wave, SP_FALSE, samp);
    osamp = swDimToSamp(window, dim);

    spDebug(80, "swTargetSampToSamp", "samp = %ld, osamp = %ld, dim = %f\n", samp, osamp, dim);
    
    return osamp;
}

double swOrderToDim(swWindow window, swWaveSubArea sub_area, long order)
{
    double dim;
    swWave wave;
    
    if (window == NULL || sub_area == NULL || sub_area->wave == NULL)
	return -1.0;

    wave = sub_area->wave;
    
    if (wave->num_order <= 1) return -1.0;

    if (swIsWaveSubAreaSpectrogram(sub_area) == SP_TRUE) {
        if (wave->custom_x_axis != NODATA) {
            order = MAX(order, 0);
            order = MIN(order, wave->custom_x_axis->length - 1);
            dim = wave->custom_x_axis->data[order];
        } else {
            dim = (window->wave->samp_rate / 2.0) * (double)order / (double)(wave->num_order - 1);
        }
    } else {
	dim = (double)order;
    }
    
    return dim;
}

long swDimToOrder(swWindow window, swWaveSubArea sub_area, double dim)
{
    long order;
    swWave wave;
    
    if (window == NULL || sub_area == NULL || sub_area->wave == NULL)
	return -1;

    wave = sub_area->wave;
    
    if (wave->num_order <= 1) return -1;

    if (swIsWaveSubAreaSpectrogram(sub_area) == SP_TRUE) {
        if (wave->custom_x_axis != NODATA) {
            order = (long)swXValueToCustomXIndex(wave->custom_x_axis, 0, wave->custom_x_axis->length - 1,
                                                 dim, SP_FALSE, NULL);
        } else {
            order = (long)round((double)(wave->num_order - 1) * dim / (window->wave->samp_rate / 2.0));
        }
    } else {
	order = (long)round(dim);
    }
    
    return order;
}

int swOrderToYPosCore(swWindow window, swWave wave, double draw_height, double order, spBool order_log_flag);

long swYPosToOrderCore(swWindow window, swWave wave, double order_min, double order_max,
                       int draw_min, int draw_max, int y, spBool log_flag)
{
    long order;
    double order_d;
    int draw_height;
    double height_rate;
    
    if (draw_max <= draw_min || wave->num_order <= 1) {
        order = 0;
    } else {
        draw_height = draw_max - draw_min;
        height_rate = (double)(y - draw_min) / (double)draw_height;
        spDebug(100, "swYPosToOrderCore", "height_rate = %f, y = %d\n", height_rate, y);
        
        if (log_flag == SP_TRUE) {
            int y_order1_boundary;
            double log_min_boundary_value;
            double log_min_value;
            double linear_min_value;

            if (wave->custom_x_axis != NODATA) {
                log_min_value = swCustomXMinLogValue(wave->custom_x_axis);
                linear_min_value = pow(10.0, log_min_value);
                if (wave->custom_x_axis->length >= 2) {
                    log_min_boundary_value = log10((wave->custom_x_axis->data[1] + wave->custom_x_axis->data[0]) / 2.0);
                } else {
                    log_min_boundary_value = SW_LOG_FREQUENCY_MIN_BOUNDARY_VALUE;
                }
            } else {
                log_min_value = log10(SW_LOG_FREQUENCY_MIN_VALUE);
                linear_min_value = SW_LOG_FREQUENCY_MIN_VALUE;
                log_min_boundary_value = SW_LOG_FREQUENCY_MIN_BOUNDARY_VALUE;
            }
            
            y_order1_boundary = draw_min + swOrderToYPosCore(window, wave, (double)draw_height,
                                                             log_min_boundary_value, SP_TRUE);

	    spDebug(100, "swYPosToOrderCore", "y = %d, order_min = %f, order_max = %f, draw_min = %d, draw_max = %d, y_order1_boundary = %d\n",
                    y, order_min, order_max, draw_min, draw_max, y_order1_boundary);
            if (y > y_order1_boundary) {
                order = 0;
            } else {
                double log_order_min, log_order_max;
                double log_order_range;
                double log_order;
                
                if (order_max <= order_min) {
                    order_min = log_min_value;
                    if (wave->custom_x_axis != NODATA) {
                        order_max = log10(wave->custom_x_axis->data[wave->custom_x_axis->length - 1]);
                    } else {
                        order_max = log10((double)(wave->num_order - 1));
                    }
                }

#if 0
                log_order_min = log10(MAX(order_min, linear_min_value));
                log_order_max = log10(MAX(order_max, linear_min_value));
#else
                log_order_min = order_min;
                log_order_max = order_max;
#endif
                log_order_range = log_order_max - log_order_min;

                log_order = log_order_max - log_order_range * height_rate;
                spDebug(100, "swYPosToOrderCore", "log_order_min = %f, log_order_max = %f, log_order = %f\n",
                        log_order_min, log_order_max, log_order);

                if (wave->custom_x_axis != NODATA) {
                    order = (long)swXValueToCustomXIndex(wave->custom_x_axis, 0, wave->custom_x_axis->length - 1,
                                                         log_order, SP_TRUE, &order_d);
                } else {
                    order_d = pow(10.0, log_order);
                    if (order_d <= linear_min_value+SW_LOG_FREQUENCY_MIN_VALUE_MARGIN) {
                        order = 0;
                    } else {
                        order = (long)spRound(order_d);
                    }
                }
            }
        } else {
            if (order_max <= order_min) {
                /*order = (long)round((double)(wave->num_order - 1) * (double)(draw_max - y) / (double)(draw_max - draw_min));*/
                order = wave->num_order - 1 - (long)/*ceil*/round((double)(wave->num_order - 1) * height_rate);
            } else {
                order_d = order_max - (order_max - order_min) * height_rate;
                if (wave->custom_x_axis != NODATA) {
                    order = (long)swXValueToCustomXIndex(wave->custom_x_axis, 0, wave->custom_x_axis->length - 1,
                                                         order_d, SP_FALSE, NULL);
                } else {
                    order = (long)/*floor*/round(order_d);
                }
            }
        }
        spDebug(100, "swYPosToOrderCore", "order = %ld / %ld\n", order, wave->num_order);
        order = MAX(order, 0);
        order = MIN(order, wave->num_order - 1);
    }

    return order;
}

long swYPosToOrder(swWindow window, swWaveSubArea sub_area, int channel, int y)
{
    int draw_min, draw_max;
    double order_min, order_max;
    spBool order_log_flag;
    swWave wave;

    if (window == NULL || sub_area == NULL || sub_area->wave == NULL)
	return -1;

    wave = sub_area->wave;
    order_log_flag = sub_area->specgram_flag == SP_TRUE ? window->log_frequency_axis : SP_FALSE;
    
    swGetOrderMinMax(window, wave, order_log_flag, SP_FALSE, &order_min, &order_max);
    swGetDrawRange(sub_area->y_d, /*sub_area->height_d*/sub_area->draw_height, channel, &draw_min, &draw_max);

    return swYPosToOrderCore(window, wave, order_min, order_max, draw_min, draw_max, y, order_log_flag);
}

int swOrderToYPosCore(swWindow window, swWave wave, double draw_height, double order_internal, spBool order_log_flag)
{
    int y;
    double order_min, order_max;

    swGetOrderMinMax(window, wave, order_log_flag, SP_FALSE, &order_min, &order_max);

    if (order_log_flag == SP_FALSE || wave->num_order < 2) {
        if (order_max <= order_min) {
            double end_order;
            
            if (wave->custom_x_axis != NODATA) {
                end_order = wave->custom_x_axis->data[wave->custom_x_axis->length - 1];
            } else {
                end_order = (double)MAX(wave->num_order - 1, 1);
            }
            y = (int)round(draw_height * (end_order - order_internal) / end_order);
        } else {
            y = (int)round(draw_height * (order_max - order_internal) / (order_max - order_min));
        }
    } else {
        double linear_lower_limit;
        double log_lower_limit;
        double log_order = 0.0;
        double log_order_min, log_order_max;
        double log_order_range;

        if (wave->custom_x_axis != NODATA) {
            log_lower_limit = swCustomXMinLogValue(wave->custom_x_axis);
            linear_lower_limit = pow(10.0, log_lower_limit);
        } else {
            log_lower_limit = log10(SW_LOG_FREQUENCY_MIN_VALUE);
            linear_lower_limit = SW_LOG_FREQUENCY_MIN_VALUE;
        }
            
        if (order_max <= order_min) {
            order_min = log_lower_limit;
            if (wave->custom_x_axis != NODATA) {
                order_max = log10(wave->custom_x_axis->data[wave->custom_x_axis->length - 1]);
            } else {
                order_max = log10((double)(wave->num_order - 1));
            }
        }
        
        spDebug(100, "swOrderToYPosCore", "order_internal = %f, order_min = %f, order_max = %f\n", order_internal, order_min, order_max);
        
        log_order_min = order_min;
        log_order_max = order_max;
        log_order_range = log_order_max - log_order_min;

        if (order_internal <= 0.0 && order_min <= log_lower_limit) {
            //y = round(draw_height - 0.5 * draw_height * (-log_order_min) / log_order_range);
            y = (int)draw_height;
        } else {
            log_order = log10(MAX(order_internal/* + 0.5*/, linear_lower_limit));
                
            y = (int)round(draw_height * (log_order_max - log_order) / log_order_range);
        }
        
        spDebug(100, "swOrderToYPosCore", "log_order_min = %f, log_order_max = %f, log_order = %f, y = %d\n",
                log_order_min, log_order_max, log_order, y);
    }
    y = MAX(y, 0);
    y = MIN(y, (int)draw_height);

    return y;
}

int swOrderToYPos(swWindow window, swWaveSubArea sub_area, long order)
{
    double order_internal;
    spBool order_log_flag;
    swWave wave;
    
    if (window == NULL || sub_area == NULL || sub_area->wave == NULL)
	return -1;

    wave = sub_area->wave;
    order_log_flag = sub_area->specgram_flag == SP_TRUE ? window->log_frequency_axis : SP_FALSE;
    if (wave->custom_x_axis != NODATA) {
        order = MAX(order, 0);
        order = MIN(order, wave->custom_x_axis->length - 1);
        order_internal = wave->custom_x_axis->data[order];
    } else {
        order_internal = (double)order;
    }
    
    return swOrderToYPosCore(window, wave, sub_area->draw_height, order_internal, order_log_flag);
}

void swUpdatePoint(swWindow window, spLong point, spBool update_flag)
{
    if (window->config->toplevel->rubber_band_selection == SP_TRUE) {
	swUpdateOverview(window, SP_TRUE, SP_FALSE, SP_FALSE, SP_FALSE);
    }
    window->point = point;
    if (window->config->toplevel->rubber_band_selection == SP_TRUE) {
	swUpdateOverview(window, SP_TRUE, SP_FALSE, SP_FALSE, update_flag);
    } else {
	swDrawOverview(window, update_flag);
    }

    spDebug(100, "swUpdatePoint", "done: window->point = %ld, update_flag = %d\n", window->point, update_flag);
    
    return;
}

void swSetCanvasCursor(spComponent canvas, spCursorType cursor_type)
{
    spCursor cursor = NULL;
    static spCursor wait_cursor = NULL;
    static spCursor move_cursor = NULL;
    static spCursor left_cursor = NULL;
    static spCursor right_cursor = NULL;
    static spCursor text_cursor = NULL;

    if (canvas == NULL) return;

    switch (cursor_type) {
      case SP_CURSOR_WAIT:
	if (wait_cursor == NULL) wait_cursor = spGetCursor(cursor_type);
	cursor = wait_cursor;
	break;
      case SP_CURSOR_MOVE:
	if (move_cursor == NULL) move_cursor = spGetCursor(cursor_type);
	cursor = move_cursor;
	break;
      case SP_CURSOR_W_RESIZE:
	if (left_cursor == NULL) left_cursor = spGetCursor(cursor_type);
	cursor = left_cursor;
	break;
      case SP_CURSOR_E_RESIZE:
	if (right_cursor == NULL) right_cursor = spGetCursor(cursor_type);
	cursor = right_cursor;
	break;
      case SP_CURSOR_TEXT:
	if (text_cursor == NULL) text_cursor = spGetCursor(cursor_type);
	cursor = text_cursor;
	break;
      default:
	break;
    }

    if (cursor != NULL) {
	spSetCanvasCursor(canvas, cursor);
    } else {
	spUnsetCanvasCursor(canvas);
    }

    return;
}

void swSetMouseCursor(swWindow window, spCursorType cursor_type)
{
    if (window == NULL) return;

    swSetCanvasCursor(window->canvas, cursor_type);
    
    return;
}

void swUnsetMouseCursor(swWindow window)
{
    if (window == NULL) return;
    
    spUnsetCanvasCursor(window->canvas);
    return;
}

swWaveSubArea swGetCursorWaveSubArea(swWindow window, int y)
{
    double y_d;
    swWaveSubArea sub_area;

    y_d = (double)y;

    sub_area = swGetNextWaveSubArea(window, NULL);

    while (sub_area != NULL) {
	spDebug(100, "swGetCursorWaveSubArea", "y_d = %f, sub_area->y_d = %f, sub_area->height_d = %f\n",
		y_d, sub_area->y_d, sub_area->height_d);
	
	if (sub_area->wave != NULL
	    && (sub_area->y_d <= y_d
		&& y_d < sub_area->y_d + sub_area->height_d)) {
	    spDebug(100, "swGetCursorWaveSubArea", "OK, found: y = %d\n", y);
    
	    return sub_area;
	}
	
	sub_area = swGetNextWaveSubArea(window, sub_area);
    }
    
    spDebug(100, "swGetCursorWaveSubArea", "**** not found ****: y = %d\n", y);
    
    return NULL;
}

int swGetCursorOrder(swWindow window, swWaveSubArea sub_area, int y, long *order)
{
    int i;
    int channel;
    int draw_min, draw_max;
    double order_min, order_max;
    spBool order_log_flag;
    swWave wave;

    if (sub_area == NULL) return -1;

    if (order != NULL) {
	*order = 0;
    }
    wave = sub_area->wave;
    order_log_flag = sub_area->specgram_flag == SP_TRUE ? window->log_frequency_axis : SP_FALSE;
    
    swGetOrderMinMax(window, wave, order_log_flag, /*order_log_flag*/SP_FALSE, &order_min, &order_max);
    
    channel = -1;
    for (i = 0; i < wave->num_channel; i++) {
	swGetDrawRange(sub_area->y_d, /*sub_area->height_d*/sub_area->draw_height, i, &draw_min, &draw_max);

	if (y > draw_min && y <= draw_max) {
	/*if (y >= draw_min && y < draw_max) {*/
	    channel = i;
	    if (order != NULL) {
                *order = swYPosToOrderCore(window, wave, order_min, order_max,
                                           draw_min, draw_max, y, order_log_flag);
                spDebug(100, "swGetCursorOrder", "swYPosToOrderCore result: order = %ld / %ld, y = %d\n",
                        *order, wave->num_order, y);
	    }
	    break;
	}
    }

    return channel;
}

static int swGetCursorChannel(swWindow window, int y)
{
    return swGetCursorOrder(window, swGetCursorWaveSubArea(window, y), y, NULL);
}

static spBool swIsNearEdge(swWindow window, int x, int y, int *stdist, int *eddist)
{
    int std, edd;
    
    if (window == NULL || window->wave == NULL
	|| (window->sel_st_d < 0 && window->sel_ed_d < 0)
	|| (swIsWavePlaying(window->wave) == SP_FALSE
	    && swIsWaveProcessing(window->wave) == SP_TRUE)) {
	return SP_FALSE;
    }

    if (window->selecting == SP_FALSE
	&& window->wave->selected_channel >= 0
	&& window->wave->selected_channel != swGetCursorChannel(window, y)) {
	return SP_FALSE;
    }
    spDebug(100, "swIsNearEdge", "sel_st_d = %d, sel_ed_d = %d, x = %d\n",
            window->sel_st_d, window->sel_ed_d, x);

    std = ABS(window->sel_st_d - x);
    edd = ABS(window->sel_ed_d - x);
    spDebug(100, "swIsNearEdge", "std = %d, edd = %d\n", std, edd);
    
    if (std <= SW_EDGE_MOTION_RANGE / 2 || edd <= SW_EDGE_MOTION_RANGE / 2) {
        spDebug(100, "swIsNearEdge", "near edge\n");
	if (window->sel_st_d < window->sel_ed_d) {
	    *stdist = std;
	    *eddist = edd;
	} else {
	    *stdist = edd;
	    *eddist = std;
	}
	return SP_TRUE;
    }

    spDebug(100, "swIsNearEdge", "not near\n");
    
    return SP_FALSE;
}

static spBool swIsInsideSelection(swWindow window, int x, int y)
{
    if (window == NULL || window->wave == NULL) return SP_FALSE;
    
    if (window->sel_st_d < x && x < window->sel_ed_d
	&& (window->wave->selected_channel == -1
	    || window->wave->selected_channel == swGetCursorChannel(window, y))) {
	return SP_TRUE;
    }
    
    return SP_FALSE;
}

static spBool swIsNearSeparation(swWindow window, int x, int y)
{
    int i;
    int channel;
    int min, max;
    swWaveSubArea sub_area;

    if (window == NULL || window->wave == NULL
	|| (swIsWaveProcessing(window->wave) == SP_TRUE
	    && swIsWavePlaying(window->wave) == SP_FALSE)) return SP_FALSE;

    if (swIsInsideSelection(window, x, y) == SP_TRUE
	|| (sub_area = swGetCursorWaveSubArea(window, y)) == NULL) {
	return SP_FALSE;
    }

    channel = -1;
    for (i = 1; i < window->wave->num_channel; i++) {
	swGetDrawRange(sub_area->y_d, /*sub_area->height_d*/sub_area->draw_height, i, &min, &max);
	min -= SW_EDGE_MOTION_RANGE / 2;
	max = min + SW_EDGE_MOTION_RANGE;

	if (y >= min && y < max) {
	    return SP_TRUE;
	}
    }
    
    return SP_FALSE;
}

spBool swIsMultipleSelectionKeyPressed(swWindow window)
{
    spModifierMask modmask;

    if (spGetModifierKeyMask(window->window, &modmask) == SP_TRUE
	&& (modmask & SP_SELECT_MODIFIER_MASK)) {
	return SP_TRUE;
    }
    
    return SP_FALSE;
}

spBool swIsExtendKeyPressed(swWindow window)
{
    spModifierMask modmask;

    if (spGetModifierKeyMask(window->window, &modmask) == SP_TRUE
	&& (modmask & SP_EXTEND_MODIFIER_MASK)) {
	return SP_TRUE;
    }
    
    return SP_FALSE;
}

static spBool swIsExtendRequired(swWindow window, int x, int y)
{
    if (/*window->sel_st_d >= 0
	  &&*/ (window->wave->selected_channel < 0
	    || window->wave->selected_channel == swGetCursorChannel(window, y))
	&& swIsExtendKeyPressed(window) == SP_TRUE) {
	return SP_TRUE;
    }
    
    return SP_FALSE;
}

void swMoveAllCursor(swWindow window)
{
    long k;
    swWave wave;
    spBool ref_specgram_flag;
    long order = -1;
    double freq = -1.0;
    double ref_point_f = -1.0;
    swDataType ref_data_type = -1;
    swWaveSubArea sub_area;
    spComponent next = NULL;
    swWindow next_window = NULL;

    if (window == NULL) return;

    if (swIsNoWave(window) == SP_TRUE) {
	return;
    } else {
	if (/*window->selecting == SP_TRUE
	      ||*/window->selecting == SP_FALSE && swIsProcessing(window) == SP_FALSE) {
	    swLockWindowMutex(window);
	    window->point_d = swSampToDisp(window, window->point);
	    window->point_f = swSampToDim(window, window->point);
	    swUnlockWindowMutex(window);

	    /* draw cursor */
	    swDrawCursor(window, SP_TRUE);
	}
    }
    
    if (swIsProcessing(window) == SP_TRUE || window->pause_cursor == SP_TRUE) {
	return;
    }

    ref_specgram_flag = SP_FALSE;
    ref_point_f = window->point_f;
    ref_data_type = window->data_type;
    if (ref_data_type == SW_FREQ_DATA) {
        freq = ref_point_f;
    }
    
    if (window->target_sub_area != NULL && window->target_sub_area->wave != NULL) {
	wave = window->target_sub_area->wave;
	
	if (wave->num_order > 1 && window->target_order >= 0) {
	    if (swIsWaveSubAreaSpectrogram(window->target_sub_area) == SP_TRUE) {
		freq = swOrderToDim(window, window->target_sub_area, window->target_order);
                ref_specgram_flag = SP_TRUE;
	    } else {
		order = window->target_order;
	    }
        }
    }
    
    next = window->window;
    for (k = 0;; k++) {
	next = spGetNextWindow(next, SP_FALSE);
	if (next == NULL || next == window->window) {
	    break;
	}
	
	if ((next_window = (swWindow)spGetUserData(next)) != NULL
	    && next_window->wave != NULL) {
	    if ((next_window->data_type == ref_data_type || (freq >= 0.0 && (ref_specgram_flag == SP_TRUE
                                                                             || swIsSpectrogramVisible(next_window) == SP_TRUE)))
		&& next_window->pause_cursor == SP_FALSE) {
		swLockWindowMutex(next_window);
		if (swIsWaveProcessing(next_window->wave) == SP_FALSE) {
		    if ((next_window->data_type == ref_data_type && ref_point_f >= 0.0)
                        || (ref_specgram_flag == SP_TRUE && freq >= 0.0)) {
                        if (next_window->data_type == ref_data_type && ref_point_f >= 0.0) {
                            next_window->point_f = ref_point_f;
                        } else if (ref_specgram_flag == SP_TRUE && freq >= 0.0) {
                            next_window->point_f = freq;
                        }
			swUpdatePoint(next_window,
				      swDimToSamp(next_window, next_window->point_f), SP_TRUE);
			next_window->point_d = swSampToDisp(next_window, next_window->point);
		    }

		    /* update order */
		    sub_area = swGetNextWaveSubArea(next_window, NULL);
		    
		    while (sub_area != NULL) {
			
			if (swIsWaveSubAreaVisible(sub_area) == SP_TRUE) {
			    wave = sub_area->wave;
			    
			    if (wave->num_order > 1) {
				if (swIsWaveSubAreaSpectrogram(sub_area) == SP_TRUE) {
				    if (freq >= 0.0) {
					next_window->target_order = swDimToOrder(next_window, sub_area, freq);
                                        spDebug(80, "swMoveAllCursor", "freq = %f, next_window->target_order = %ld\n", freq, next_window->target_order);
				    } else {
					next_window->target_order = -1;
				    }
				} else {
				    if (order >= 0) {
					next_window->target_order = order;
				    } else {
					next_window->target_order = -1;
				    }
				}
                            }
			}
			sub_area = swGetNextWaveSubArea(next_window, sub_area);
		    }
		    
		    wave = swGetTargetWave(next_window);
		    sub_area = swGetWaveSubArea(next_window, wave);
		    
		    /* draw cursor */
		    swDrawCursor(next_window, SP_TRUE);
		} else {
		    next_window->prev_point_f = ref_point_f;
		}
		swUnlockWindowMutex(next_window);
	    }
	}
    }
    
    return;
}

spBool swMoveCursor(swWindow window, spLong point, spBool update_flag)
{
    if (swIsNoWave(window) == SP_TRUE
	|| point < 0 || point >= window->wave->total_length) {
	return SP_FALSE;
    }
    
    if (window->selecting == SP_TRUE || swIsProcessing(window) == SP_FALSE) {
	swLockWindowMutex(window);
	swUpdatePoint(window, point, update_flag);
	swUnlockWindowMutex(window);
    } else if (update_flag == SP_TRUE && window->overview_canvas != NULL) {
	spRefreshCanvas(window->overview_canvas);
    }

    swMoveAllCursor(window);
    
    return SP_TRUE;
}

void swMoveCursorCB(spComponent component, swWindow window)
{
    int x, y;
    int stdist, eddist;
    int point_d;
    int draw_width;
    spLong point;
    double point_f;
#ifdef SW_SUPPORT_MORPHING
    swAnchor time_anchor;
#endif
    static spBool cursor_set = SP_FALSE;

    spDebug(50, "swMoveCursorCB", "in\n");
    
    if (swIsNoWave(window) == SP_TRUE) return;

    draw_width = swGetDrawWidth(window, SP_TRUE);

    if (swGetCallbackMousePosition(component, window, SP_FALSE, &x, &y) == SP_TRUE) {
	x = MIN(x, draw_width);
	x = MAX(x, 0);

	window->target_sub_area = swGetCursorWaveSubArea(window, y);
	window->target_channel = MAX(swGetCursorOrder(window, window->target_sub_area,
						      y, &window->target_order), 0);
	    
	/* calculate new cursor position */
	point = swDispToSamp(window, x);
	point_d = swSampToDisp(window, point);
	point_f = swSampToDim(window, point);
        spDebug(100, "swMoveCursorCB", "x = %d, point = %ld, point_d = %d, point_f = %f\n",
                x, (long)point, point_d, point_f);

	if (window->selecting == SP_FALSE && window->drag_region == SP_FALSE
	    && window->drag_label_type == SW_DRAG_NO_LABEL) {
	    if (swIsExtendRequired(window, x, y) == SP_TRUE) {
		if (point_d < (window->sel_st_d + window->sel_ed_d) / 2) {
		    swSetMouseCursor(window, SP_CURSOR_W_RESIZE);
		} else {
		    swSetMouseCursor(window, SP_CURSOR_E_RESIZE);
		}
#ifdef SW_SUPPORT_MORPHING
	    } else if ((time_anchor = swFindNearTimeAnchor(window, point_f)) != NULL) {
		swAnchor freq_anchor;
		long order;
		double freq_hz = -1.0;

		if (swIsWaveSubAreaSpectrogram(window->target_sub_area) == SP_TRUE
		    && swGetCursorOrder(window, window->target_sub_area, y, &order) >= 0) {
		    freq_hz = swOrderToDim(window, window->target_sub_area, order);
		}
		
		if ((freq_anchor = swFindNearFreqAnchor(window, window->target_sub_area, time_anchor, freq_hz)) == NULL) {
		    swSetMouseCursor(window, SP_CURSOR_E_RESIZE);
		} else {
		    swSetMouseCursor(window, SP_CURSOR_MOVE);
		}
		cursor_set = SP_TRUE;
#endif
	    } else if (window->active_label_index >= 0
		       && window->active_label_index == swFindNearLabelIndex(window, point_f, SP_FALSE, NULL)) {
		swSetMouseCursor(window, SP_CURSOR_MOVE);
		cursor_set = SP_TRUE;
	    } else if (swIsNearEdge(window, point_d, y, &stdist, &eddist) == SP_TRUE) {
		if (stdist < eddist) {
		    swSetMouseCursor(window, SP_CURSOR_W_RESIZE);
		} else {
		    swSetMouseCursor(window, SP_CURSOR_E_RESIZE);
		}
		cursor_set = SP_TRUE;
	    } else if (swIsNearSeparation(window, point_d, y) == SP_TRUE) {
		swSetMouseCursor(window, SP_CURSOR_TEXT);
		cursor_set = SP_TRUE;
	    } else if (cursor_set == SP_TRUE) {
		swUnsetMouseCursor(window);
		cursor_set = SP_FALSE;
	    }
	}

	if (window->pause_cursor == SP_FALSE && swIsWaveProcessing(window->wave) == SP_FALSE) {
	    swLockWindowMutex(window);
	    swUpdatePoint(window, point, SP_TRUE);
	    window->point_d = point_d;
	    swUnlockWindowMutex(window);
	}
	    
	/* move all cursor */
	swMoveAllCursor(window);
    }
    
    return;
}

spBool swPlayRegionEx(swWindow window, spLong st, spLong ed, spLong start_offset)
{
    spLong offset;
    spLong length;
    spBool flag;

    if (swIsNoWave(window) == SP_TRUE
	|| window->data_type == SW_FREQ_DATA
	|| swIsProcessing(window) == SP_TRUE
	|| swIsWavePlayable(window->wave) == SP_FALSE
	|| window->num_blocked > 0
	|| window->wave->num_order > 1) {
	flag = SP_FALSE;
    } else {
	spDebug(30, "swPlayRegion", "in\n");

	swLockWindowMutex(window);
    
	/* get edge */
	if (swGetEdge(window, st, ed, &offset, &length) != SP_TRUE) {
	    offset = 0;
	    length = window->wave->total_length;
	    window->wave->selected_channel = -1;
	}

	spDebug(30, "swPlayRegion", "offset = %ld, length = %ld\n", offset, length);

	window->prev_point_f = window->point_f;
	if (start_offset < 0) {
	    if (window->pause_cursor == SP_TRUE) {
		spDebug(30, "swPlayRegion",
			"pause case: window->point = %ld, offset = %ld\n", window->point, offset);
		start_offset = window->point - offset;
		if (start_offset < 0 || start_offset >= length) {
		    start_offset = 0;
		}
	    } else {
		start_offset = 0;
	    }
	}
	swUnlockWindowMutex(window);
    
	flag = swPlayWave(window->wave, offset, length, start_offset, window->loop_play);
	spDebug(30, "swPlayRegion", "flag = %d\n", flag);
    }

    if (flag == SP_FALSE) {
	spDebug(30, "swPlayRegion", "**** error ****\n");
	spDisplayError(window->window, NULL, SW_PLAY_ERROR_MESSAGE);
	spDebug(30, "swPlayRegion", "spDisplayError done\n");
    }
    
    return flag;
}

spBool swPlayRegion(swWindow window, spLong st, spLong ed)
{
    return swPlayRegionEx(window, st, ed, -1);
}

spBool swPlayStop(swWindow window)
{
    if (swIsNoWave(window) == SP_TRUE) return SP_FALSE;
    
    if (swIsWavePlaying(window->wave) == SP_TRUE) {
	swProcessStop(window->wave);
	return SP_TRUE;
    }
    return SP_FALSE;
}

void swPlayRegionCB(spComponent component, swWindow window)
{
    if (swIsNoWave(window) == SP_TRUE) return;
    
    swPlayRegion(window, window->sel_st, window->sel_ed);
    
    return;
}

void swPlayWindowCB(spComponent component, swWindow window)
{
    if (swIsNoWave(window) == SP_TRUE) return;
    
    swPlayRegion(window, window->offset, window->offset + window->length - 1);
    
    return;
}

void swPlayFileCB(spComponent component, swWindow window)
{
    if (swIsNoWave(window) == SP_TRUE) return;
    
    swPlayRegion(window, -1, -1);
    
    return;
}

void swRecordRegionCB(spComponent component, swWindow window)
{
    spLong offset;
    spLong length;
    spBool flag;

    if (swIsNoWave(window) == SP_TRUE) return;
    
    if (swIsNoWave(window) == SP_TRUE
	|| window->data_type == SW_FREQ_DATA
	|| swIsProcessing(window) == SP_TRUE
	|| swIsWavePlayable(window->wave) == SP_FALSE
	|| window->num_blocked > 0
	|| window->wave->num_order > 1) {
	flag = SP_FALSE;
    } else {
	spDebug(30, "swRecordRegionCB", "in\n");

	/* get edge */
	if (swGetEdge(window, window->sel_st, window->sel_ed, &offset, &length) != SP_TRUE) {
	    spDisplayError(window->window, NULL, SW_REGION_ERROR_MESSAGE);
	    return;
	}

	spDebug(30, "swRecordRegionCB", "offset = %ld, length = %ld\n", offset, length);
    
	swRecordWave(window->wave, offset, length);
    }
    
    return;
}

void swPlayStopCB(spComponent component, swWindow window)
{
    spBool update_paused_pos = SP_FALSE;
    spDialogResponse response = SP_DR_YES;
    
    if (swIsNoWave(window) == SP_FALSE) {
	spDebug(30, "swPlayStopCB", "in\n");
	
	swLockMutex(window->wave);

	if (swIsProcessing(window) == SP_TRUE
	    && swIsWaveProcessFinished(window->wave) == SP_FALSE) {
#if 0
	    if (swIsWavePlaying(window->wave) == SP_FALSE) {
		response = spCreateMessageBox(window->window, NULL,
					      SW_STOP_EDIT_QUESTION_MESSAGE,
					      SppDialogType, SP_QUESTION_DIALOG,
					      SppMessageBoxButtonType, SP_MB_YES_NO,
					      NULL);
	    }
#else
	    response = SP_DR_YES;
#endif
	
	    if (response == SP_DR_YES) {
		swProcessStop(window->wave);
	    }
	} else {
	    if (window->pause_cursor == SP_TRUE
		&& swIsWavePlaying(window->wave) == SP_FALSE) {
		update_paused_pos = SP_TRUE;
	    }
	}
	
	swUnlockMutex(window->wave);

	if (update_paused_pos == SP_TRUE) {
	    swLockWindowMutex(window);
	    swUpdatePoint(window, MAX(window->sel_st, 0), SP_TRUE);
	    window->point_d = swSampToDisp(window, window->point);
	    swUnlockWindowMutex(window);
	    swDrawCursor(window, SP_TRUE);
	}
	
	spDebug(30, "swPlayStopCB", "done\n");
    }
    
    return;
}

void swPauseCursor(swWindow window)
{
    swPlayStop(window);
    
    spSetToggleState(window->pause_cursor_menu, window->pause_cursor);
    spSetToggleState(window->pause_cursor_tool_item, window->pause_cursor);

    return;
}

void swPauseCursorCB(spComponent component, swWindow window)
{
    if (spGetToggleState(component, &window->pause_cursor) == SP_TRUE) {
	spDebug(10, "swPauseCursorCB", "pause_play = %d\n", window->pause_cursor);
	swPauseCursor(window);
    }
    
    return;
}

void swLButtonPress(swWindow window, int x, int y, int *px, int *py)
{
    spBool flag;
    spBool extend_flag;
    spBool keep_active_label;
    spLong point;
    int point_d;
    double point_f;
    long index;
    int channel;
    int stdist = -1, eddist = -1;
    int x_new;
    spLong samp;
#ifdef SW_SUPPORT_MORPHING
    swAnchor time_anchor;
#endif

    spDebug(10, "swLButtonPress", "active label: %ld\n", window->active_label_index);

    if (window->pause_cursor == SP_TRUE) {
	point = swDispToSamp(window, x);
	point_d = swSampToDisp(window, point);
	point_f = swSampToDim(window, point);
    } else {
	point_f = window->point_f;
	point_d = window->point_d;
    }
	
    if (window->mouse_mode == SW_MOUSE_MODE_NORMAL_LABEL_SINGLE
	|| window->mouse_mode == SW_MOUSE_MODE_REGION_LABEL_SINGLE) {
	keep_active_label = SP_TRUE;
    } else {
	keep_active_label = SP_FALSE;
    }
    
    if (window->active_label_index >= 0
	&& window->active_label_index == swFindNearLabelIndex(window, point_f, SP_FALSE, &flag)) {
	swSetMouseCursor(window, SP_CURSOR_MOVE);
	if (flag == SP_TRUE) {
	    window->drag_label_type = SW_DRAG_END_LABEL;
	} else {
	    window->drag_label_type = SW_DRAG_START_LABEL;
	}
	*px = x;
	*py = y;

	keep_active_label = SP_TRUE;
    } else if ((extend_flag = swIsExtendRequired(window, x, y)) == SP_TRUE
	       || swIsNearEdge(window, x, y, &stdist, &eddist) == SP_TRUE) {
	if (swIsWavePlaying(window->wave) == SP_TRUE || swIsWaveProcessing(window->wave) == SP_FALSE) {
	    swLockWindowMutex(window);

            samp = swDispToSamp(window, x);
            x_new = swSampToDisp(window, samp);

	    if (extend_flag == SP_TRUE) {
		spDebug(10, "swLButtonPress", "extend: x = %d, x_new = %d, sel_st_d = %d, sel_ed_d = %d\n",
			x, x_new, window->sel_st_d, window->sel_ed_d);
		if (x_new < (window->sel_st_d + window->sel_ed_d) / 2) {
		    if (window->config->toplevel->rubber_band_selection == SP_TRUE) {
			swReverseRegion(window, window->wave->selected_channel,
					/*x*/x_new, window->sel_st_d, SP_TRUE, SP_TRUE);
		    }

#if 0
		    window->sel_st = swDispToSamp(window, x);
		    window->sel_st_d = swSampToDisp(window, window->sel_st);
#else
		    window->sel_st = samp;
		    window->sel_st_d = x_new;
#endif
		    
		    swDrawCursor(window, SP_TRUE);
		    stdist = 0; eddist = 1;
		} else {
		    if (window->config->toplevel->rubber_band_selection == SP_TRUE) {
			swReverseRegion(window, window->wave->selected_channel,
					window->sel_ed_d, /*x*/x_new, SP_TRUE, SP_TRUE);
		    }

#if 0
		    window->sel_ed = swDispToSamp(window, x);
		    window->sel_ed = MIN(window->sel_ed, window->wave->total_length - 1);
		    window->sel_ed_d = swSampToDisp(window, window->sel_ed);
#else
		    window->sel_ed = samp;
		    window->sel_ed = MIN(window->sel_ed, window->wave->total_length - 1);
		    window->sel_ed_d = swSampToDisp(window, window->sel_ed);
#endif
		    
		    swDrawCursor(window, SP_TRUE);
		    
		    stdist = 1; eddist = 0;
		}
		spDebug(10, "swLButtonPress", "extend end: sel_st_d = %d, sel_ed_d = %d\n",
			window->sel_st_d, window->sel_ed_d);
		spRefreshCanvas(window->overview_canvas);
	    }
	    spDebug(10, "swLButtonPress", "stdist = %d, eddist = %d\n", stdist, eddist);

	    if (stdist >= 0) {
		if (stdist < eddist) {
		    *px = window->sel_st_d;
		    window->sel_st_d = window->sel_ed_d;
		    window->sel_st = window->sel_ed;
		} else {
		    *px = window->sel_ed_d;
		}
		*py = y;
		window->selecting = SP_TRUE;
	    }
	    
	    swUnlockWindowMutex(window);
	}
#ifdef SW_SUPPORT_MORPHING
    } else if ((time_anchor = swFindNearTimeAnchor(window, point_f)) != NULL) {
	swAnchor freq_anchor;
	long order;
	double freq_hz = -1.0;
	swWaveSubArea sub_area;

	sub_area = swGetCursorWaveSubArea(window, y);

	if (swIsWaveSubAreaSpectrogram(sub_area) == SP_TRUE
	    && swGetCursorOrder(window, sub_area, y, &order) >= 0) {
	    freq_hz = swOrderToDim(window, sub_area, order);
	}
	
	if ((freq_anchor = swFindNearFreqAnchor(window, sub_area, time_anchor, freq_hz)) == NULL) {
	    swSetMouseCursor(window, SP_CURSOR_E_RESIZE);
	    window->drag_anchor = time_anchor;
	    window->drag_label_type = SW_DRAG_TIME_ANCHOR;
	} else {
	    swSetMouseCursor(window, SP_CURSOR_MOVE);
	    window->drag_anchor = freq_anchor;
	    window->drag_label_type = SW_DRAG_FREQUENCY_ANCHOR;
	}
	swAnchorBackupPosition(window->drag_anchor);
	
	*px = x;
	*py = y;
#endif	
    } else if ((index = swFindNearLabelIndex(window, point_f, SP_FALSE, NULL)) >= 0) {
	/* activate label */
	window->active_label_index = index;
	spDebug(10, "swLButtonPress", "label %ld is activated\n", index);
	swRedrawLabels(window);
	swSelectLabelList(window->label_list);
	*px = x;
	*py = y;
	
	keep_active_label = SP_TRUE;
    } else if (window->mouse_mode == SW_MOUSE_MODE_NORMAL_LABEL_SINGLE) {
	if (swIsNearSeparation(window, point_d, y) == SP_TRUE && (window->label_caps & SW_LABEL_CAPS_NONCHANNEL_NORMAL) != 0) {
	    channel = -1;
	} else {
	    channel = swGetCursorChannel(window, y);
	}
	swInsertChannelLabel(window, point_f, channel, NULL);
	keep_active_label = SP_TRUE;
    } else if (swIsInsideSelection(window, x, y) == SP_TRUE) {
	spDebug(10, "swLButtonPress", "set canvas cursor: x = %d\n", x);
	
	if (swIsProcessing(window) == SP_FALSE && window->num_blocked <= 0) {
	    swSetMouseCursor(window, SP_CURSOR_MOVE);
	    window->drag_region = SP_TRUE;
	}
	
	*px = x;
	*py = -1;
    } else {
	if (swIsWavePlaying(window->wave) == SP_TRUE || swIsWaveProcessing(window->wave) == SP_FALSE) {
	    swLockWindowMutex(window);
	    
	    if (window->selecting == SP_TRUE) { /* could't catch button release */
		window->sel_st_d = -1;
		window->sel_ed_d = -1;
		window->sel_st = -1;
		window->sel_ed = -1;
		swRedrawLabels(window);
	    } else {
		swClearRegion(window);
	    }
	    window->selecting = SP_TRUE;
	    
	    window->sel_st = swDispToSamp(window, x);
	    spDebug(50, "swLButtonPress", "press: sel_st = %ld\n", window->sel_st);
	    window->sel_st_d = swSampToDisp(window, window->sel_st);

	    if (swIsWaveProcessing(window->wave) == SP_FALSE
		/*|| swIsWavePlaying(window->wave) == SP_TRUE*/) {
		if (window->wave->num_channel > 1
		    && swIsNearSeparation(window, window->sel_st_d, y) == SP_FALSE) {
		    window->wave->selected_channel = swGetCursorChannel(window, y);
		} else {
		    window->wave->selected_channel = -1;
		}
	    }
	    
	    *px = window->sel_st_d;
	    *py = y;
	    spDebug(50, "swLButtonPress", "press: px = %d, py = %d\n", *px, *py);
	    spDebug(50, "swLButtonPress", "press: sel_st_d = %d\n", window->sel_st_d);
	    
	    swUnlockWindowMutex(window);
	}
    }

    if (keep_active_label == SP_FALSE) {
	swUnselectActiveLabel(window);
    }
    
    spDebug(10, "swLButtonPress", "done\n");

    return;
}

void swLButtonRelease(swWindow window, int ox, int oy, int px, int py)
{
    if (py < 0) {
	if (swIsProcessing(window) == SP_FALSE && window->drag_region == SP_TRUE
	    && window->num_blocked <= 0) {
	    spDebug(10, "swLButtonRelease", "ox = %d, oy = %d\n", ox, oy);
	    swUnsetMouseCursor(window);
	    if (ox < 0 || oy < 0 || ox > window->width || oy > window->height) {
		int wx, wy;

		spGetWindowPosition(window->window, &wx, &wy);
		wx += ox; wy += oy;
		wx = MAX(wx - window->config->width / 2, 0);
		wy = MAX(wy - window->config->height / 2, 0);
	    
		if (window->config->autosave_by_drop == SP_TRUE) {
		    swExtractAutosaveWindowAt(window, wx, wy);
		} else {
		    swExtractWindowAt(window, window->sel_st, window->sel_ed, SP_FALSE, wx, wy);
		}
	    } else {
		if (px == ox) {
		    swSelectRegion(window, window->wave->selected_channel, -1, -1);
		}
	    }
	} else {
	    if ((swIsWavePlaying(window->wave) == SP_TRUE
		 || swIsWaveProcessing(window->wave) == SP_FALSE)
		&& !(ox < 0 || oy < 0 || ox > window->width || oy > window->height)
		&& px == ox) {
		swSelectRegion(window, window->wave->selected_channel, -1, -1);
		swSetPlayRegion(window->wave, 0, 0);
	    }
	}
	window->drag_region = SP_FALSE;
#ifdef SW_SUPPORT_MORPHING
    } else if (window->drag_label_type == SW_DRAG_TIME_ANCHOR
	       || window->drag_label_type == SW_DRAG_FREQUENCY_ANCHOR) {
	swUnsetMouseCursor(window);
	
	spDebug(50, "swLButtonRelease", "release anchor, px = %d, ox = %d, oy = %d\n", px, ox, oy);
	
	if (px == ox) {
	    if (window->drag_anchor != NULL) {
		if (window->drag_label_type == SW_DRAG_TIME_ANCHOR
		    || py == oy) {
		    if (swAnchorIsSelected(window->drag_anchor) == SP_FALSE) {
			if (swIsMultipleSelectionKeyPressed(window) == SP_FALSE) {
			    swClearAnchorSelection(window);
			}
			swAddAnchorToSelection(window, window->drag_anchor);
		    } else {
			swRemoveAnchorFromSelection(window, window->drag_anchor);
		    }
		}
	    }
	} else {
	    swAnchorType type;
	    double time_s, freq_hz;

	    swAnchorRecoverPosition(window->drag_anchor);
	    
	    type = swAnchorGetType(window->drag_anchor);
	    
	    /* move anchor */
	    if (type == SW_ANCHOR_TYPE_TIME) {
		time_s = swSampToDim(window, swDispToSamp(window, ox));
		swReplaceTimeAnchor(window, window->drag_anchor, time_s);
	    } else if (type == SW_ANCHOR_TYPE_FREQUENCY) {
		long order;
		swWaveSubArea sub_area;

		sub_area = swGetCursorWaveSubArea(window, oy);
		
		if (swGetCursorOrder(window, sub_area, oy, &order) >= 0) {
		    freq_hz = swOrderToDim(window, sub_area, order);
		    swReplaceFreqAnchor(window, window->drag_anchor, freq_hz);
		}
	    }
	}
	
	window->drag_label_type = SW_DRAG_NO_LABEL;
	window->drag_anchor = NULL;
#endif
    } else if (window->drag_label_type != SW_DRAG_NO_LABEL) {
	swUnsetMouseCursor(window);

	if (px == ox) {
	    long index;
	    double time;
	    spBool start_flag, end_flag;

	    /* get end time of active label */
	    if (window->drag_label_type == SW_DRAG_END_LABEL
		&& window->active_label_index >= 0
		&& swIsRegionLabel(window->wave, window->active_label_index) == SP_TRUE) {
		time = swGetLabelEndTime(window->wave, window->active_label_index);
	    } else {
		time = -1.0;
	    }

	    spDebug(10, "swLButtonRelease", "px == ox: time = %f\n", time);
	    
	    index = -1;
	    window->drag_label_type = SW_DRAG_NO_LABEL;
	
	    if (time >= 0.0
		&& swFindIdenticalLabelIndex(window, time, SP_TRUE, &index,
					     &start_flag, &end_flag) == SP_TRUE
		&& index != window->active_label_index && start_flag == SP_TRUE) {
		/* if region label exists, select it. */
		spDebug(10, "swLButtonRelease", "found identical: %ld\n", index);
		window->active_label_index = index;
		swRedrawLabels(window);
		swSelectLabelList(window->label_list);
	    } else if (window->mouse_mode != SW_MOUSE_MODE_NORMAL_LABEL_SINGLE
		       && window->mouse_mode != SW_MOUSE_MODE_REGION_LABEL_SINGLE) {
		/* deactivate label */
		spDebug(10, "swLButtonRelease", "label %ld is deactivated\n", window->active_label_index);
		swUnselectActiveLabel(window);
	    }
	} else {
	    window->drag_label_type = SW_DRAG_NO_LABEL;
	    swUpdateLabels(window);
	    
	}
    } else if (window->selecting == SP_TRUE) {
	if (swIsWavePlaying(window->wave) == SP_TRUE
	    || swIsWaveProcessing(window->wave) == SP_FALSE) {
	    swLockWindowMutex(window);

	    spDebug(50, "swLButtonRelease", "selection finished\n");
	    
	    if (px == window->sel_st_d) {
		swResetRegion(window);
		swSetPlayRegion(window->wave, 0, 0);
	    } else {
		if (px < window->sel_st_d) {
		    window->sel_ed_d = window->sel_st_d;
		    window->sel_ed = window->sel_st;
		    window->sel_st_d = px;
		    window->sel_st = swDispToSamp(window, window->sel_st_d);
		} else {
		    window->sel_ed_d = px;
		    window->sel_ed = swDispToSamp(window, window->sel_ed_d);
		}
		spDebug(50, "swLButtonRelease",
			"release: sel_st = %d, sel_ed = %d\n", window->sel_st_d, 
			window->sel_ed_d);

		if (swIsWaveProcessing(window->wave) == SP_FALSE
		    /*|| swIsWavePlaying(window->wave) == SP_TRUE*/) {
		    if (window->wave->selected_channel != swGetCursorChannel(window, oy)) {
			window->wave->selected_channel = -1;
		    }

		    if (window->pause_cursor == SP_TRUE) {
			window->point = window->sel_st;
			window->point_d = swSampToDisp(window, window->point);
			window->point_f = swSampToDim(window, window->point);
		    }
		}
		
		swSetSelectSenseLevel(window, SP_TRUE);
		swSetPlayRegion(window->wave, window->sel_st, window->sel_ed - window->sel_st + 1);
	    }

	    swUpdateInfoAreaSelection(window);

	    spRefreshCanvas(window->overview_canvas);
	    
	    window->selecting = SP_FALSE;
	    swUnlockWindowMutex(window);
	}
    }
	
    return;
}

void swLButtonMotion(swWindow window, int x, int y, int *px, int *py)
{
    int orig_x;
    spLong x_s;
    double x_f;

    if (window->selecting == SP_TRUE || window->drag_label_type != SW_DRAG_NO_LABEL) {
	swLockWindowMutex(window);
        orig_x = x;
	x_s = swDispToSamp(window, x);
	if (x_s != swDispToSamp(window, *px)
	    || window->drag_label_type == SW_DRAG_FREQUENCY_ANCHOR) {
	    x = swSampToDisp(window, x_s);
	    
	    spDebug(70, "swLButtonMotion",  "(%d, %d), orig_x = %d / %d, x_s = %ld\n", *px, x, orig_x, window->width, x_s);

	    if (window->drag_label_type != SW_DRAG_NO_LABEL) {
#ifdef SW_SUPPORT_MORPHING
		if (window->drag_label_type == SW_DRAG_TIME_ANCHOR
		    || window->drag_label_type == SW_DRAG_FREQUENCY_ANCHOR) {
		    if (window->drag_anchor != NULL) {
			x_f = swSampToDim(window, x_s);
			spDebug(70, "swLButtonMotion",  "x_f = %f\n", x_f);
			if (window->drag_label_type == SW_DRAG_TIME_ANCHOR) {
			    swAnchorSetPosition(window->drag_anchor, x_f);
			} else if (window->drag_label_type == SW_DRAG_FREQUENCY_ANCHOR) {
			    long order;
			    double freq_hz;
			    swWaveSubArea sub_area;

			    sub_area = swGetCursorWaveSubArea(window, y);
			    
			    if (swGetCursorOrder(window, sub_area, y, &order) >= 0) {
				spDebug(70, "swLButtonMotion",  "order = %ld\n", order);
				freq_hz = swOrderToDim(window, sub_area, order);
				swAnchorSetPosition(window->drag_anchor, freq_hz);
			    }
			}
		    }
		} else 
#endif
		if (window->wave->labels != NULL && window->active_label_index >= 0) {
		    x_f = swSampToDim(window, x_s);
		    if (window->drag_label_type == SW_DRAG_END_LABEL
			&& swGetLabelEndTime(window->wave, window->active_label_index) >= 0.0) {
			/* dragging end label */
			spDebug(100, "swLButtonMotion",  "label %ld: %f --> %f\n",
				window->active_label_index,
				swGetLabelEndTime(window->wave, window->active_label_index), x_f);

			swReplaceIdenticalLabelIndex(window, SP_TRUE, window->active_label_index, x_f);

			if (x_f < swGetLabelStartTime(window->wave, window->active_label_index)) {
			    /* swap end for start */
			    swChangeLabelTime(window->wave,
					      window->active_label_index, x_f,
					      swGetLabelStartTime(window->wave, window->active_label_index));
			    /* dragging label is start label */
			    window->drag_label_type = SW_DRAG_START_LABEL;
			} else {
			    swSetLabelEndTime(window->wave, window->active_label_index, x_f);
			}
		    } else {
			/* dragging start label */
			spDebug(100, "swLButtonMotion",  "label %ld: %f --> %f\n",
				window->active_label_index,
				swGetLabelStartTime(window->wave, window->active_label_index), x_f);

			if (swIsRegionLabel(window->wave, window->active_label_index) == SP_TRUE) {
			    swReplaceIdenticalLabelIndex(window, SP_FALSE, window->active_label_index, x_f);
			
			    if (x_f > swGetLabelEndTime(window->wave, window->active_label_index)) {
				/* swap start for end */
				swChangeLabelTime(window->wave, window->active_label_index,
						  swGetLabelEndTime(window->wave, window->active_label_index),
						  x_f);
			    
				/* dragging label is end label */
				window->drag_label_type = SW_DRAG_END_LABEL;
			    } else {
				swSetLabelStartTime(window->wave, window->active_label_index, x_f);
			    }
			} else {
			    swSetLabelTime(window->wave, window->active_label_index, x_f);
			}
		    }
		    swRedrawLabels(window);
		}
	    } else {
		int current_selected_channel;

		current_selected_channel = window->wave->selected_channel;
		
		if (swIsWaveProcessing(window->wave) == SP_TRUE
		    /*&& swIsWavePlaying(window->wave) == SP_FALSE*/) {
		    if (window->config->toplevel->rubber_band_selection == SP_TRUE) {
			swReverseRegion(window, window->wave->selected_channel, *px, x, SP_TRUE, SP_TRUE);
		    }
		} else {
		    if (window->wave->selected_channel >= 0
			&& window->wave->selected_channel != swGetCursorChannel(window, y)) {
			if (window->wave->selected_channel != swGetCursorChannel(window, *py)) {
			    /* multiple channel --> multiple channel */
			    if (window->config->toplevel->rubber_band_selection == SP_TRUE) {
				swReverseRegion(window, -1, *px, x, SP_TRUE, SP_TRUE);
			    }
			    current_selected_channel = -1;
			} else {
			    /* single channel --> multiple channel */
			    if (window->config->toplevel->rubber_band_selection == SP_TRUE) {
				swReverseRegion(window, window->wave->selected_channel,
						window->sel_st_d, *px, SP_TRUE, SP_TRUE);
				swReverseRegion(window, -1, window->sel_st_d, x, SP_TRUE, SP_TRUE);
			    }
			    current_selected_channel = -1;
			}
		    } else {
			if (window->wave->selected_channel >= 0
			    && window->wave->selected_channel != swGetCursorChannel(window, *py)) {
			    /* multiple channel --> single channel */
			    if (window->config->toplevel->rubber_band_selection == SP_TRUE) {
				swReverseRegion(window, -1, window->sel_st_d, *px, SP_TRUE, SP_TRUE);
				swReverseRegion(window, window->wave->selected_channel,
						window->sel_st_d, x, SP_TRUE, SP_TRUE);
			    }
			} else {
			    /* single channel --> single channel */
			    /* or multiple channel --> multiple channel */
			    if (window->config->toplevel->rubber_band_selection == SP_TRUE) {
				swReverseRegion(window, window->wave->selected_channel, *px, x, SP_TRUE, SP_TRUE);
			    }
			}
		    }
		}
		window->sel_ed_d = x;
		window->sel_ed = swDispToSamp(window, window->sel_ed_d);
                spDebug(50, "swLButtonMotion",  "window->sel_ed_d = %d, window->sel_ed = %ld\n",
                        window->sel_ed_d, window->sel_ed);

		if (window->config->toplevel->rubber_band_selection == SP_TRUE) {
		    /* draw cursor */
		    swDrawCursor(window, SP_TRUE);
		} else {
		    swDrawCursorWithSelection(window, current_selected_channel,
					      window->sel_st_d, window->sel_ed_d, SP_TRUE);
		}
		
		*px = x;
		*py = y;
	    }
	}
	swUnlockWindowMutex(window);
    }
    
    return;
}

void swLButtonScroll(swWindow window, int ox, int oy, int *px, int *py)
{
    spLong value;
    spLong offset;
    spLong new_sel_ed;

    if ((swIsProcessing(window) == SP_TRUE && swIsWavePlaying(window->wave) == SP_FALSE)
	|| window->num_blocked > 0
	|| (swIsWavePeakAvailable(window->wave) == SP_FALSE
	    && window->length > window->config->wave_config->read_callback_length)) return;
	
    if (*py >= 0) {
	swLockWindowMutex(window);
	
	spDebug(60, "swLButtonScroll",
                "before: (sel_st, sel_ed) = (%ld, %ld), window->offset = %ld, window->length = %ld, total_length = %ld, *px = %d\n",
                window->sel_st, window->sel_ed, window->offset, window->length, window->wave->total_length, *px);
        
        if (swIsXAxisLogFrequency(window) == SP_TRUE && window->wave->total_length >= 2) {
            spLong end_pos;
            double log_end_pos;
            double log_offset;
            double log_range;
            double log_offset_new;
            double offset_d;
            double log_sel_ed;
            double new_sel_ed_d;
            
            log_offset = log10(MAX(window->offset, SW_LOG_FREQUENCY_MIN_VALUE));
            end_pos = window->offset + window->length - 1;
            log_end_pos = log10((double)end_pos); 
            log_range = (log_end_pos - log_offset) / 16.0;
            if (ox < 0) {
                log_range *= -1.0;
            }
            log_offset_new = log_offset + log_range;
            spDebug(60, "swLButtonScroll",
                    "log_offset = %f, end_pos = %ld, log_end_pos = %f, log_range = %f, log_offset_new = %f\n",
                    log_offset, end_pos, log_end_pos, log_range, log_offset_new);
            offset_d = pow(10.0, log_offset_new);
            if ((spLong)spRound(offset_d) == window->offset) {
                if (ox < 0) {
                    offset_d = offset_d - 1.0;
                } else {
                    offset_d = offset_d + 1.0;
                }
            }
            offset_d = MAX(spRound(offset_d), SW_LOG_FREQUENCY_MIN_VALUE);
            log_offset_new = log10(offset_d);
            log_range = log_offset_new - log_offset;
            spDebug(60, "swLButtonScroll", "updated: offset_d = %f, log_offset_new = %f, log_range = %f\n",
                    offset_d, log_offset_new, log_range);
#if 1
            {
                double log_end_pos_new;
                double end_pos_new;
                log_end_pos_new = log_end_pos + log_range;
                end_pos_new = pow(10.0, log_end_pos_new);
                spDebug(60, "swLButtonScroll", "end_pos = %ld, end_pos_new = %f, log_end_pos_new = %f\n",
                        end_pos, end_pos_new, log_end_pos_new);
            }
#endif
            if (window->sel_ed >= 0) {
                log_sel_ed = log10(MAX(window->sel_ed, SW_LOG_FREQUENCY_MIN_VALUE));
                log_sel_ed += log_range;
                new_sel_ed_d = pow(10.0, log_sel_ed);
                if (new_sel_ed_d <= SW_LOG_FREQUENCY_MIN_VALUE+SW_LOG_FREQUENCY_MIN_VALUE_MARGIN) {
                    new_sel_ed = 0;
                } else {
                    new_sel_ed = (spLong)spRound(new_sel_ed_d);
                }
                new_sel_ed = MIN(new_sel_ed, window->wave->total_length - 1);
                spDebug(60, "swLButtonScroll", "window->sel_ed = %ld, log_range = %f, log_sel_ed = %f, new_sel_ed = %ld\n",
                        window->sel_ed, log_range, log_sel_ed, new_sel_ed);
            } else {
                new_sel_ed = window->sel_ed;
            }
            if (offset_d <= SW_LOG_FREQUENCY_MIN_VALUE+SW_LOG_FREQUENCY_MIN_VALUE_MARGIN) {
                offset = 0;
            } else {
                offset = (spLong)spRound(offset_d);
            }
            offset = MIN(offset, window->wave->total_length - window->length);
            offset = MAX(offset, 0);
        } else {
            value = MAX(window->length / 16, 1);
            if (ox < 0) {
                value *= -1;
            }

            offset = window->offset + value;
            offset = MIN(offset, window->wave->total_length - window->length);
            offset = MAX(offset, 0);
            value = offset - window->offset;
            new_sel_ed = swDispToSamp(window, *px) + value;
        }

        if (offset != window->offset) {
            if (window->selecting == SP_TRUE) {
                if (window->wave->selected_channel >= 0
                    && window->wave->selected_channel != swGetCursorChannel(window, *py)) {
                    window->wave->selected_channel = -1;
                }
                if (window->config->toplevel->rubber_band_selection == SP_TRUE) {
                    spDebug(60, "swLButtonScroll", "call swReverseOverviewRegion: (%ld, %ld)\n", window->sel_st, window->sel_ed);
                    swReverseOverviewRegion(window, window->wave->selected_channel,
                                            swSampToOverviewDisp(window, window->sel_st),
                                            swSampToOverviewDisp(window, window->sel_ed));
                }
	
                window->sel_ed = new_sel_ed;
            }
	
            spDebug(60, "swLButtonScroll", "after: (%ld, %ld), offset = %ld (%ld)\n",
                    window->sel_st, window->sel_ed, offset, window->offset);
        }
	
	swUnlockWindowMutex(window);
	
        if (offset != window->offset) {
            swScrollWindow(window, offset, SP_FALSE, SP_FALSE);
        
            if (window->config->toplevel->rubber_band_selection == SP_TRUE) {
                spDebug(60, "swLButtonScroll",
                        "after scroll, call swReverseOverviewRegion: (%ld, %ld), *px = %d, sel_st_d = %d, sel_ed_d = %d\n",
                        window->sel_st, window->sel_ed, *px, window->sel_st_d, window->sel_ed_d);
                swReverseOverviewRegion(window, window->wave->selected_channel,
                                        swSampToOverviewDisp(window, window->sel_st),
                                        swSampToOverviewDisp(window, window->sel_ed));
            }
            window->sel_st_d = swSampToDisp(window, window->sel_st);
            window->sel_ed_d = swSampToDisp(window, window->sel_ed);
            *px = window->sel_ed_d;
            spDebug(100, "swLButtonScroll", "end: ox = %d, oy = %d, *px = %d, *py = %d, window->sel_st_d = %d, window->sel_ed_d = %d\n",
                    ox, oy, *px, *py, window->sel_st_d, window->sel_ed_d);
        }
    }
    
    spDebug(100, "swLButtonScroll", "done\n");
    
    return;
}

void swMButtonPress(swWindow window)
{
    spBool flag = SP_FALSE;
    
    swLockMutex(window->wave);
    if (swIsProcessing(window) == SP_TRUE) {
	if (swIsWavePlaying(window->wave) == SP_TRUE
	    && swIsWaveProcessFinished(window->wave) == SP_FALSE) {
	    swProcessStop(window->wave);
	}
	flag = SP_TRUE;
    }
    swUnlockMutex(window->wave);
    
    if (flag == SP_FALSE && swIsPlayable(window) == SP_TRUE) {
	spDebug(50, "swMButtonPress", "mbutton press: sel_st = %d sel_ed = %d\n",
		window->sel_st_d, window->sel_ed_d);
	
	swPlayRegion(window, window->sel_st, window->sel_ed);
    }
    
    return;
}

void swButtonMotionCB(spComponent component, swWindow window)
{
    int ox, oy;
    int x, y;
    int draw_width;
    spCallbackReason reason;
    static int prev_x = 0, prev_y = 0;
    
    if (swIsNoWave(window) == SP_TRUE) return;

    if (swGetCallbackMousePosition(component, window, SP_FALSE, &ox, &oy) == SP_FALSE) {
	return;
    }

    draw_width = swGetDrawWidth(window, SP_TRUE);
    spDebug(80, "swButtonMotionCB", "ox = %d, oy = %d, prev_x = %d, prev_y = %d, draw_width = %d\n",
            ox, oy, prev_x, prev_y, draw_width);
    
    x = MIN(ox, draw_width);
    y = MIN(oy, window->height);
    x = MAX(x, 0);
    y = MAX(y, 0);
    spDebug(80, "swButtonMotionCB", "x = %d / %d, y = %d / %d\n", x, draw_width, y, window->height);

    reason = spGetCallbackReason(component);

#if 0
    if (swIsWavePlaying(window->wave) == SP_TRUE && window->sync_play == SP_TRUE
	&& reason != SP_CR_MBUTTON_PRESS) {
        return;
    }
#endif
    
    switch (reason) {
      case SP_CR_LBUTTON_PRESS:
	swLButtonPress(window, x, y, &prev_x, &prev_y);
	break;
      case SP_CR_LBUTTON_RELEASE:
	swLButtonRelease(window, ox, oy, prev_x, prev_y);
	break;
      case SP_CR_LBUTTON_MOTION:
	if (ox > draw_width || ox < 0) {
	    swLButtonScroll(window, ox, oy, &prev_x, &prev_y);
	} else {
	    swLButtonMotion(window, x, y, &prev_x, &prev_y);
	}
	break;
      case SP_CR_MBUTTON_PRESS:
	swMButtonPress(window);
	break;
      case SP_CR_RBUTTON_PRESS:
#if 0
	if (window->pause_cursor == SP_TRUE) {
	    long point;
	    
	    swLockWindowMutex(window);
	    point = swDispToSamp(window, x);
	    swUpdatePoint(window, point, SP_TRUE);
	    window->point_d = swSampToDisp(window, point);
	    swUnlockWindowMutex(window);
	    swDrawCursor(window, SP_TRUE);
	}
#endif
#ifdef SW_SUPPORT_MORPHING
	if (swGetNumTimeAnchor(window) > 0) {
	    swAnchor time_anchor;
	    swAnchor freq_anchor;
	    long order;
	    double freq_hz = -1.0;
	    swWaveSubArea sub_area;

	    if ((time_anchor = swFindNearTimeAnchor(window, window->point_f)) != NULL) {
		sub_area = swGetCursorWaveSubArea(window, y);
	    
		if (swIsWaveSubAreaSpectrogram(sub_area) == SP_TRUE
		    && swGetCursorOrder(window, sub_area, y, &order) >= 0) {
		    freq_hz = swOrderToDim(window, sub_area, order);
		}
		
		if ((freq_anchor = swFindNearFreqAnchor(window, sub_area, time_anchor, freq_hz)) != NULL) {
		    spSetWindowSenseLevel(window->window, SW_ANCHOR_GROUP_ID, SW_STATE_EXIST_FREQUENCY_ANCHOR_HERE);
		} else {
		    spSetWindowSenseLevel(window->window, SW_ANCHOR_GROUP_ID, SW_STATE_EXIST_TIME_ANCHOR_HERE);
		}
	    } else {
		spSetWindowSenseLevel(window->window, SW_ANCHOR_GROUP_ID, SW_STATE_EXIST_ANCHOR);
	    }
	}
	else
#endif
	if (swGetNumLabel(window->wave) > 0 && window->draw_label == SP_TRUE) {
	    long index;

	    spDebug(10, "swButtonMotionCB", "right button pressed: point = %f\n", window->point_f);

	    if ((index = swFindNearLabelIndex(window, window->point_f, SP_FALSE, NULL)) >= 0) {
		if (swIsRegionLabel(window->wave, index) == SP_TRUE) {
		    spSetWindowSenseLevel(window->window, SW_REGION_LABEL_GROUP_ID,
					  SW_STATE_EXIST_LABEL_HERE);
		}
		spSetWindowSenseLevel(window->window, SW_LABEL_GROUP_ID,
				      SW_STATE_EXIST_LABEL_HERE);
	    } else {
		spSetWindowSenseLevel(window->window, SW_REGION_LABEL_GROUP_ID,
				      SW_STATE_EXIST_LABEL);
		spSetWindowSenseLevel(window->window, SW_LABEL_GROUP_ID,
				      SW_STATE_EXIST_LABEL);
	    }
	}
	break;
      default:
	break;
    }

    spDebug(100, "swButtonMotionCB", "done: prev_x = %d, prev_y = %d\n", prev_x, prev_y);
    
    return;
}

void swKeyOrWheelWindow(swWindow window, spKeySym key_sym, spLong delta)
{
    if (swIsExtendKeyPressed(window) == SP_TRUE) {
	int channel = -1;
	spLong sel_st, sel_ed;
	spLong point;
	    
	if (key_sym == SPK_Home) {
	    if (window->sel_st >= 0) {
		channel = window->wave->selected_channel;
		sel_ed = window->sel_ed;
	    } else {
		sel_ed = window->point;
	    }
	    sel_st = 0;
	    point = window->point;
	} else if (key_sym == SPK_End) {
	    if (window->sel_st >= 0) {
		channel = window->wave->selected_channel;
		sel_st = window->sel_st;
	    } else {
		sel_st = window->point;
	    }
	    sel_ed = window->wave->total_length - 1;
	    point = window->point;
	} else if (window->sel_st >= 0) {
	    sel_st = window->sel_st;
	    sel_ed = window->sel_ed;
		    
	    if (window->point < (window->sel_st + window->sel_ed) / 2) {
		sel_st += delta;
		point = sel_st;
	    } else {
		sel_ed += delta;
		point = sel_ed;
	    }
	    channel = window->wave->selected_channel;
	} else {
	    sel_st = sel_ed = window->point;
		    
	    if (delta < 0) {
		sel_st += delta;
		point = sel_st;
	    } else {
		sel_ed += delta;
		point = sel_ed;
	    }
	}
	swSelectRegion(window, channel, sel_st, sel_ed);
	swUpdatePoint(window, point, SP_TRUE);
	swMoveAllCursor(window);
    } else {
	swScrollWindow(window, window->offset + delta, /*SP_TRUE*/SP_FALSE, SP_FALSE);
    }
    
    return;
}

void swKeyPressCB(spComponent component, swWindow window)
{
    spKeySym key_sym;

    if (window == NULL || window->wave == NULL) return;
    
    spDebug(50, "swKeyPressCB", "in\n");

    swUnsetMouseCursor(window);
    
    if (spGetCallbackKeySym(component, &key_sym) == SP_TRUE) {
	spDebug(50, "swKeyPressCB", "%ld\n", key_sym);
	
	if (key_sym == SPK_Character) {
	    int len;
	    char buf[SP_MAX_LINE];
	    spBool overflow;
	    
	    if ((len = spGetCallbackKeyString(component, buf, sizeof(buf), &overflow)) >= 0) {
		spDebug(50, "swKeyPressCB", "len = %d, buf = %s\n", len, buf);
		if (streq(buf, " ")) {
		    if (swPlayStop(window) == SP_FALSE) {
			swPlayRegion(window, window->sel_st, window->sel_ed);
		    }
		}
	    }
	} else if (key_sym == SPK_Delete) {
	    if (window->active_label_index >= 0) {
		swEraseActiveLabel(window);
	    } else {
		swEditWindow(window, SW_EDIT_DELETE, window->sel_st, window->sel_ed, 0.0);
	    }
	} else {
	    spLong delta = 0;
	    
	    switch (key_sym) {
	      case SPK_Prior:
		delta = -MAX(window->length / 4, 1);
		break;
	      case SPK_Next:
		delta = MAX(window->length / 4, 1);
		break;
	
	      case SPK_Left:
		delta = -MAX(window->length / 8, 1);
		break;
	      case SPK_Right:
		delta = MAX(window->length / 8, 1);
		break;
	    
	      case SPK_Home:
		delta = -window->offset;
		break;
	      case SPK_End:
		delta = -window->offset + window->wave->total_length - window->length;
		break;

	      default:
		return;
	    }

	    if ((key_sym == SPK_Prior || key_sym == SPK_Next)
		&& window->config->scroll_left_by_wheel_down == SP_TRUE) {
		delta *= -1;
	    }

	    swKeyOrWheelWindow(window, key_sym, delta);
	}
    }
    
    return;
}

void swWheelCB(spComponent component, swWindow window)
{
    spCallbackReason reason;
    
    if (window == NULL || window->wave == NULL) return;
    
    reason = spGetCallbackReason(component);
    
    spDebug(50, "swWheelCB", "reason = %d\n", reason);

    swUnsetMouseCursor(window);

    if (reason == SP_CR_WHEEL) {
	double delta_x, delta_y;
	spLong delta;
	
	if (spGetCallbackWheelValue(component, &delta_x, &delta_y) == SP_TRUE) {
	    spDebug(20, "swWheelCB", "delta_x = %f, delta_y = %f\n", delta_x, delta_y);
	    delta = (spLong)spRound(delta_x * MAX(window->length / 8, 1));
	    spDebug(20, "swWheelCB", "window->length = %ld, delta = %ld\n", window->length, (long)delta);
	    swKeyOrWheelWindow(window, SPK_Unknown, delta);
	}
    } else if (reason == SP_CR_ZOOM) {
	if (1 || component == window->canvas) {
	    double h_factor, v_factor, factor;
	    spGetParams(component,
			SppHorizontalContentFactor, &h_factor,
			SppVerticalContentFactor, &v_factor,
			NULL);
	    factor = MAX(h_factor, v_factor);
	    spDebug(20, "swWheelCB", "h_factor = %f, v_factor = %f, factor = %f\n", h_factor, v_factor, factor);
	    swZoomWindow(window, factor, SP_FALSE);
	}
    }
    
    spDebug(50, "swWheelCB", "done\n");

    return;
}
