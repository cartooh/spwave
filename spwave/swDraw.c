/*
 *        swDraw.c
 *
 *        Last modified: <2025-04-27 22:04:44 hideki>
 */

#include <stdio.h>
#include <stdlib.h>

#include <sp/spBaseLib.h>
#include <sp/spAudioLib.h>
#include <sp/spLib.h>
#include <sp/spComponentLib.h>

#include "swWaveAudio.h"

#include "swWindow.h"
#include "swDraw.h"
#include "swCursor.h"
#include "swEdit.h"
#include "swDialog.h"
#include "swLabel.h"
#include "swLabelDialog.h"
#include "swLabelList.h"
#include "swAnalysis.h"
#if defined(SW_SUPPORT_MORPHING)
#include "swMorphingDraw.h"
#endif
#if defined(SW_AH_CUSTOM)
#include "swWindowAH.h"
#include "swLabelAH.h"
#include "swDrawAH.h"
#endif

#define SW_DRAW_LABEL_CANVAS        /* cursor will be sometimes slow */
/*#undef SW_DRAW_LABEL_CANVAS*/ /* dragging a label will be slow */

#define SW_METER_FONT "-*-*-medium-r-normal--8-*-*-*-*-*-*-*"

swGraphics sw_graphics = NULL;

void swLockGraphicsMutex(void)
{
    if (sw_graphics == NULL) return;
    
    spLockMutex(sw_graphics->mutex);
    
    return;
}

void swUnlockGraphicsMutex(void)
{
    if (sw_graphics == NULL) return;
    
    spUnlockMutex(sw_graphics->mutex);
    
    return;
}

void swSetColor(swConfig config)
{
    if (sw_graphics == NULL) {
        sw_graphics = xalloc(1, struct _swGraphics);
        sw_graphics->gx_fg = NULL;
        sw_graphics->gx_bg = NULL;
        sw_graphics->gx_xor = NULL;
        sw_graphics->gx_pointer = NULL;
        sw_graphics->gx_string = NULL;
        sw_graphics->gx_label = NULL;
        sw_graphics->gx_region = NULL;
        sw_graphics->gx_region_bg = NULL;
        sw_graphics->gx_scale = NULL;
        sw_graphics->gx_meter = NULL;
        sw_graphics->gx_shading = NULL;
        sw_graphics->gx_overview_xor = NULL;
        
        sw_graphics->region_pixel = 0L;
        sw_graphics->current_region_pixel = 0L;
        sw_graphics->region_bg_pixel = 0L;
        sw_graphics->region_pixel_incr = 0;
        sw_graphics->mutex = spCreateMutex(NULL);
    }
    
    spLockMutex(sw_graphics->mutex);
    
    spDebug(40, "swCreateGraphics", "%s %s %s %s\n",
            config->wave_fg, config->wave_bg, config->pointer_color, config->label_color);
    
    if (sw_graphics->gx_fg == NULL) {
        sw_graphics->gx_fg = spCreateGraphics("Foreground",
                                              SppForeground, config->wave_fg,
                                              SppFontName, config->canvas_font,
                                              NULL);
    } else {
        spSetGraphicsParams(sw_graphics->gx_fg,
                            SppForeground, config->wave_fg,
                            SppFontName, config->canvas_font,
                            NULL);
    }
    
    if (sw_graphics->gx_bg == NULL) {
        sw_graphics->gx_bg = spCreateGraphics("Background",
                                              SppForeground, config->wave_bg,
                                              SppLineWidth, 3,
                                              NULL);
    } else {
        spSetGraphicsParams(sw_graphics->gx_bg,
                            SppForeground, config->wave_bg,
                            NULL);
    }
    
    if (sw_graphics->gx_pointer == NULL) {
        sw_graphics->gx_pointer = spCreateGraphics("Pointer",
                                                   SppForeground, config->pointer_color,
                                                   NULL);
    } else {
        spSetGraphicsParams(sw_graphics->gx_pointer,
                            SppForeground, config->pointer_color,
                            NULL);
    }
    
    if (sw_graphics->gx_string == NULL) {
        sw_graphics->gx_string = spCreateGraphics("String",
                                                  SppForeground, config->string_color,
                                                  SppFontName, config->canvas_font,
                                                  NULL);
    } else {
        spSetGraphicsParams(sw_graphics->gx_string,
                            SppForeground, config->string_color,
                            SppFontName, config->canvas_font,
                            NULL);
    }
    
    if (sw_graphics->gx_label == NULL) {
        sw_graphics->gx_label = spCreateGraphics("Label",
                                                 SppForeground, config->label_color,
                                                 SppFontName, config->canvas_font,
                                                 NULL);
    } else {
        spSetGraphicsParams(sw_graphics->gx_label,
                            SppForeground, config->label_color,
                            SppFontName, config->canvas_font,
                            NULL);
    }
    {
        int i;
        spPixel pixel;
        int r, g, b;
        spGraphicsMode region_bg_mode;
        spGraphicsMode xor_mode;
        spGraphicsMode overview_xor_mode;
        char *xor_color_name;
        
        pixel = spGetColorPixel(config->wave_bg);
        r = spGetRValue(pixel);
        g = spGetGValue(pixel);
        b = spGetBValue(pixel);
        
        if (r + g + b >= /*386*/192) {
            if (config->toplevel->graphics_mode_caps & SP_GRAPHICS_MODE_CAPS_PLUS_DARKER) {
                xor_mode = SP_GM_PLUS_DARKER;
            } else {
                xor_mode = SP_GM_MULTIPLY;
            }
            xor_color_name = /*"#808080"*//*"#a8cdf1"*/"#a0c6ff"/*"#b5d5ff"*/;
        } else {
            xor_mode = SP_GM_PLUS_LIGHTER/*SP_GM_SCREEN*/;
            xor_color_name = "#304080"/*"#a8cdf1"*//*"#b5d5ff"*/;
        }
        
        if (r + g + b >= /*386*/192) {
            if (config->toplevel->graphics_mode_caps & SP_GRAPHICS_MODE_CAPS_AND) {
                region_bg_mode = SP_GM_AND;
                r = MAX(r, 128);
                g = MAX(g, 128);
                b = MAX(b, 128);
            } else if (config->toplevel->graphics_mode_caps & SP_GRAPHICS_MODE_CAPS_PLUS_DARKER) {
                region_bg_mode = SP_GM_PLUS_DARKER;
                r = g = b = 255;
            } else {
                region_bg_mode = SP_GM_MULTIPLY;
                r = g = b = 255;
            }
            sw_graphics->region_pixel_incr = SW_REGION_PIXEL_NEGA_INCR;
        } else {
            if (config->toplevel->graphics_mode_caps & SP_GRAPHICS_MODE_CAPS_AND) {
                region_bg_mode = SP_GM_OR;
                r = MIN(r, 128);
                g = MIN(g, 128);
                b = MIN(b, 128);
            } else {
                region_bg_mode = SP_GM_PLUS_LIGHTER;
                r = b = 8;
                g = 0;
            }
            sw_graphics->region_pixel_incr = SW_REGION_PIXEL_POSI_INCR;
        }
        
        overview_xor_mode = SP_GM_COPY;
        
        if (config->toplevel->graphics_mode_caps & SP_GRAPHICS_MODE_CAPS_XOR) {
            xor_mode = SP_GM_XOR;
            xor_color_name = "max";
            overview_xor_mode = SP_GM_XOR;
        }
        spDebug(80, "swSetColor", "xor_mode = %d, overview_xor_mode = %d\n",
                xor_mode, overview_xor_mode);

        for (i = 0;; i++) {
            r = MAX(r + 2 * (i + 1) * sw_graphics->region_pixel_incr, 0); r = MIN(r, 255);
            g = MAX(g + 2 * (i + 1) * sw_graphics->region_pixel_incr, 0); g = MIN(g, 255);
            b = MAX(b + 2 * (i + 1) * sw_graphics->region_pixel_incr, 0); b = MIN(b, 255);
            pixel = spRGB(r, g, b);

            if (i == 0) {
                sw_graphics->region_pixel = pixel;
                spGetColorName(pixel, sw_graphics->region_color);

                sw_graphics->region_bg_pixel = pixel;
                spGetColorName(sw_graphics->region_bg_pixel, sw_graphics->region_bg_color);
            } else if (i == 1) {
                spGetColorName(pixel, sw_graphics->region_line_color);
            } else {
                spGetColorName(pixel, sw_graphics->region_label_color);
                break;
            }
        }
        
        if (sw_graphics->gx_region == NULL) {
            sw_graphics->gx_region = spCreateGraphics("Region",
                                                      SppForeground, sw_graphics->region_color,
                                                      NULL);
        } else {
            spSetGraphicsParams(sw_graphics->gx_region,
                                SppForeground, sw_graphics->region_color,
                                NULL);
        }
        if (sw_graphics->gx_region_bg == NULL) {
            sw_graphics->gx_region_bg = spCreateGraphics("RegionBG",
                                                         SppGraphicsMode, region_bg_mode,
                                                         SppForeground, sw_graphics->region_bg_color,
                                                         NULL);
        } else {
            spSetGraphicsParams(sw_graphics->gx_region_bg,
                                SppGraphicsMode, region_bg_mode,
                                SppForeground, sw_graphics->region_bg_color,
                                NULL);
        }

        if (sw_graphics->gx_xor == NULL) {
            sw_graphics->gx_xor = spCreateGraphics("XOR",
                                                   SppForeground, xor_color_name,
                                                   SppGraphicsMode, xor_mode,
                                                   NULL);
        } else {
            spSetGraphicsParams(sw_graphics->gx_xor,
                                SppForeground, xor_color_name,
                                SppGraphicsMode, xor_mode,
                                NULL);
        }
        if (sw_graphics->gx_overview_xor == NULL) {
            sw_graphics->gx_overview_xor = spCreateGraphics("OverviewXOR",
                                                            SppForeground, xor_color_name,
                                                            SppGraphicsMode, overview_xor_mode,
                                                            NULL);
        } else {
            spSetGraphicsParams(sw_graphics->gx_overview_xor,
                                SppForeground, xor_color_name,
                                SppGraphicsMode, overview_xor_mode,
                                NULL);
        }
    }
    
    if (sw_graphics->gx_scale == NULL) {
        sw_graphics->gx_scale = spCreateGraphics("Scale",
                                                 SppForeground, config->scale_color,
                                                 SppFontName, config->canvas_font,
                                                 NULL);
    } else {
        spSetGraphicsParams(sw_graphics->gx_scale,
                            SppForeground, config->scale_color,
                            SppFontName, config->canvas_font,
                            NULL);
    }
    
    if (sw_graphics->gx_meter == NULL) {
        sw_graphics->gx_meter = spCreateGraphics("Meter",
                                                 SppFontName, SW_METER_FONT,
                                                 NULL);
    }
    if (sw_graphics->gx_shading == NULL) {
        sw_graphics->gx_shading = spCreateGraphics("Shading",
                                                   NULL);
    }
    spUnlockMutex(sw_graphics->mutex);
    
    return;
}

void swGetDrawRange(double y_draw_offset, double draw_height, int channel, int *draw_min, int *draw_max)
{
    double dmin, dmax;

    dmin = (double)channel * draw_height + y_draw_offset;
    dmax = dmin + draw_height;

    if (draw_min != NULL) *draw_min = (int)spRound(dmin);
    if (draw_max != NULL) *draw_max = (int)spRound(dmax);

    return;
}

int swGetDrawWidth(swWindow window, spBool inside_vertical_keys)
{
    int draw_width;
    
#ifdef SW_SUPPORT_METER
    draw_width = window->width - window->meter_width;
#else
    draw_width = window->width;
#endif
    
    if (inside_vertical_keys == SP_TRUE) {
        if (window->draw_vertical_keys == SP_TRUE) {
            draw_width -= (int)spRound(window->vertical_keys_width);
        }
    }

    return MAX(draw_width, SW_MIN_DRAW_WIDTH);
}

double swCalcVerticalKeysWidth(swWindow window, int default_keys_size, double width)
{
    double width_keys, width_waveform;
    
    width_keys = default_keys_size;
    width_waveform = spRound(MAX(width - width_keys, 2.0));
    width_keys = MIN(width_keys, width - width_waveform);

    return width_keys;
}

double swGetWindowLimitValue(swWindow window, swWave wave)
{
#if 0
    if (swIsWaveFloat(wave) == SP_TRUE) {
        double min, max;
        min = swGetWaveMin(wave);
        max = swGetWaveMax(wave);
        max = MAX(FABS(min), FABS(max));
        if (max < 1.0) {
            max = 1.0;
        }
        return max;
    }
#endif
    return swGetLimitValue(wave->samp_bit);
}

spBool swGetMainStringExtent(spComponent canvas, char *string, int *x, int *y, int *width, int *height)
{
    return spGetStringExtent(canvas, sw_graphics->gx_string, string, x, y, width, height, NULL);
}

void swDrawMainString(spComponent canvas, int left_offset, int top_offset, char *string)
{
    spDrawString(canvas, sw_graphics->gx_string, left_offset, top_offset, string);
    return;
}

#ifdef SW_SUPPORT_METER
spBool swIsMeterVisible(swWindow window)
{
    if (window == NULL /*|| window->draw_height <= 0.0*/
        || window->meter_width <= 0) {
        return SP_FALSE;
    }

    return SP_TRUE;
}

#define SW_MIN_METER_VALUE -300.0
#define SW_METER_DRAW_HEIGHT_THRESHOLD 150
#define SW_METER_TICK_STRING_LEFT_OFFSET 5
#define SW_METER_TICK_STRING_BOTTOM_OFFSET 5

#define SW_HORIZONTAL_METER_DRAW_WIDTH_THRESHOLD 200
#define SW_HORIZONTAL_METER_TICK_STRING_LEFT_OFFSET -5
#define SW_HORIZONTAL_METER_TICK_STRING_BOTTOM_OFFSET 5

#define SW_REC_BUTTON_MARGIN 10
#define SW_REC_BUTTON_INNER_MARGIN 4
#define SW_REC_BUTTON_ICON_MARGIN 6
#define SW_REC_BUTTON_OUTER_CIRCLE_LINE_WIDTH 2
#define SW_REC_BUTTON_MAIN_COLOR "red"
#define SW_REC_BUTTON_SUB_COLOR "maroon"/*"darkred"*/

static void intdBToRG(int inputdB, unsigned char *red, unsigned char *green)
{
    int value;
    
    if (inputdB < -24) {
        value = 255 + 3 * (24 + inputdB);
        *green = (unsigned char)MAX(value, 0);
        *red = 0;
    } else if (inputdB > -6) {
        if (inputdB >= 0) {
            *green = 0;
        } else {
            value = -128 * inputdB / 3;
            *green = (unsigned char)MAX(value, 0);
        }
        *red = 255;
    } else if (inputdB > -18) {
        *green = 255;
        value = 64 * inputdB / 3 + 384;
        *red = (unsigned char)MIN(value, 255);
    } else {
        *green = 255;
        *red = 0;
    }
    
    return;
}

static void doubledBToRG(double inputdB, unsigned char *red, unsigned char *green)
{
    double value;
    
    if (inputdB < -24.0) {
        value = 255.0 + 3.0 * (24.0 + inputdB);
        value = MAX(value, 0.0);
        *green = (unsigned char)value;
        *red = 0;
    } else if (inputdB > -6.0) {
        if (inputdB >= 0.0) {
            *green = 0;
        } else {
            value = -127.5 * inputdB / 3.0;
            value = MAX(value, 0.0);
            value = MIN(value, 255.0);
            *green = (unsigned char)value;
        }
        *red = 255;
    } else if (inputdB > -18.0) {
        *green = 255;
        value = 63.75 * inputdB / 3.0 + 382.5;
        value = MAX(value, 0.0);
        value = MIN(value, 255.0);
        *red = (unsigned char)value;
    } else {
        *green = 255;
        *red = 0;
    }
    
    return;
}

void swDrawMeterBar(spComponent component, swWindow window, 
                    double draw_height, int left_offset, int top_offset, double value)
{
    int i;
    int y, py;
    int height;
    int meter_range;
    int meter_min, meter_max;
    double y_factor;
    unsigned char green, red;
    char buf[SP_MAX_LINE];

    if (swIsMeterVisible(window) == SP_FALSE) return;

    meter_min = -window->config->meter_range;
    meter_max = 6;
    meter_range = meter_max - meter_min;

    if (meter_range <= 0) return;
    
    spLockMutex(sw_graphics->mutex);
    
    y_factor = draw_height / (double)meter_range;
    spDebug(60, "swDrawMeterBar", "y_factor = %f, draw_height = %f, left_offset = %d, top_offset = %d\n",
            y_factor, draw_height, left_offset, top_offset);

    py = top_offset + (int)draw_height;
    for (i = meter_min; i <= meter_max; i++) {
        y = top_offset + (int)draw_height - (int)spRound(y_factor * (double)(i - meter_min));
        height = py - y;

        if (height >= 1) {
            intdBToRG(i, &red, &green);
            
            if ((double)i > value) {
                green /= 2;
                red /= 2;
            }
            
            spDebug(120, "swDrawMeterBar", "i = %d, green = %d, red = %d, y = %d, height = %d\n",
                    i, green, red, y, height);
            spSetForegroundPixel(sw_graphics->gx_meter, spRGB(red, green, 0));
            
            spFillRectangle(component, sw_graphics->gx_meter,
                            left_offset, y,
                            window->meter_width, height);
            
            py = y;
        }
    }
    
    py = top_offset + (int)draw_height;
    for (i = meter_min; i < meter_max; i++) {
        y = top_offset + (int)draw_height - (int)spRound(y_factor * (double)(i - meter_min));
        if ((draw_height > SW_METER_DRAW_HEIGHT_THRESHOLD && i >= -6 && i % 3 == 0)
            || (i >= -12 && i % 6 == 0)
            || (i % 24 == 0)) {
            /* draw tick */
            spSetForegroundPixel(sw_graphics->gx_meter, spRGB(255, 255, 255));
            
            spDrawLine(component, sw_graphics->gx_meter,
                       left_offset, y, left_offset + 2, y);
            spDrawLine(component, sw_graphics->gx_meter,
                       left_offset + window->meter_width,
                       y, left_offset + window->meter_width - 3, y);

            sprintf(buf, "%3d", i);
            spDrawString(component, sw_graphics->gx_meter,
                         left_offset + SW_METER_TICK_STRING_LEFT_OFFSET,
                         y + SW_METER_TICK_STRING_BOTTOM_OFFSET, buf);
        }
    }
    
    spUnlockMutex(sw_graphics->mutex);
    
    return;
}

void swDrawMeter(spComponent component, swWindow window, swWave wave,
                 double y_draw_offset, double draw_height, int left_offset, int channel, double value)
{
    double max;
    double dBvalue;
    
    if (swIsMeterVisible(window) == SP_FALSE) return;

    max = swGetWindowLimitValue(window, wave);

    dBvalue = FABS(value) / max;
    dBvalue = (dBvalue < 1e-30 ? -300.0 : dB(dBvalue));
    
    spDebug(80, "swDrawMeter", "channel = %d, value = %f, dBvalue = %f, max = %f\n",
            channel, value, dBvalue, max);
    swDrawMeterBar(component, window, draw_height, left_offset,
                   (int)spRound((double)channel * draw_height + y_draw_offset),
                   dBvalue);
    
    return;
}

void swDrawHorizontalMeterBar(spComponent component, swWindow window, double dB_increment,
                              int draw_width, int meter_height, int left_offset, int top_offset, double value)
{
    int x, px;
    int width;
    double meter_value;
    double meter_range;
    double meter_min, meter_max;
    double x_factor;
    unsigned char green, red;
    char buf[SP_MAX_LINE];

    meter_min = -(double)window->config->meter_range;
    meter_max = 6.0;
    meter_range = meter_max - meter_min;

    if (meter_range <= 0.0) return;
    
    spLockMutex(sw_graphics->mutex);
    
    x_factor = draw_width / meter_range;
    spDebug(60, "swDrawHorizontalMeterBar", "x_factor = %f, draw_width = %f, left_offset = %d, top_offset = %d\n",
            x_factor, draw_width, left_offset, top_offset);

    px = left_offset;
    for (meter_value = meter_min; meter_value <= meter_max; meter_value += dB_increment) {
        x = left_offset + (int)spRound(x_factor * (meter_value - meter_min));
        width = x - px;

        if (width >= 1) {
            doubledBToRG(meter_value, &red, &green);
            
            if (meter_value > value) {
                green /= 2;
                red /= 2;
            }
            
            spDebug(120, "swDrawHorizontalMeterBar", "meter_value = %f, green = %d, red = %d, x = %d, width = %d\n",
                    meter_value, green, red, x, width);
            spSetForegroundPixel(sw_graphics->gx_meter, spRGB(red, green, 0));
            
            spFillRectangle(component, sw_graphics->gx_meter,
                            px, top_offset,
                            width, meter_height);
            
            px = x;
        }
    }
    
    px = left_offset;
    for (meter_value = meter_min; meter_value < meter_max; meter_value += 1.0) {
        x = left_offset + (int)spRound(x_factor * (meter_value - meter_min));
        if ((draw_width > SW_HORIZONTAL_METER_DRAW_WIDTH_THRESHOLD && meter_value >= -6.0 && (int)meter_value % 3 == 0)
            || (meter_value >= -12.0 && (int)meter_value % 6 == 0)
            || ((int)meter_value % 24 == 0)) {
            /* draw tick */
            spSetForegroundPixel(sw_graphics->gx_meter, spRGB(255, 255, 255));
            
            spDrawLine(component, sw_graphics->gx_meter,
                       x, top_offset, x, top_offset + 2);
            spDrawLine(component, sw_graphics->gx_meter,
                       x, top_offset + meter_height - 2, x, top_offset + meter_height);

            sprintf(buf, "%3.0f", meter_value);
            spDrawString(component, sw_graphics->gx_meter,
                         x + SW_HORIZONTAL_METER_TICK_STRING_LEFT_OFFSET,
                         top_offset + meter_height - SW_HORIZONTAL_METER_TICK_STRING_BOTTOM_OFFSET, buf);
        }
    }
    
    spUnlockMutex(sw_graphics->mutex);
    
    return;
}

void swDrawRecButton(spComponent component, swWindow window, int left_offset, int top_offset, int draw_width, int draw_height,
                     int margin, int inner_margin, int icon_margin, swRecButtonState state)
{
    int offset_x, offset_y;
    int radius, diameter;
    int size;
    
    spLockMutex(sw_graphics->mutex);

    if (draw_height < draw_width) {
        offset_y = margin;
        radius = draw_height / 2 - margin;
        offset_x = draw_width / 2 - radius;
    } else {
        offset_x = margin;
        radius = draw_width / 2 - margin;
        offset_y = draw_height / 2 - radius;
    }
    diameter = radius * 2;
    
    spSetGraphicsParams(sw_graphics->gx_fg,
                        SppForeground, state == SW_REC_BUTTON_STATE_PRESSING ? SW_REC_BUTTON_MAIN_COLOR : window->config->wave_fg,
                        SppLineWidth, SW_REC_BUTTON_OUTER_CIRCLE_LINE_WIDTH,
                        NULL);
    spDrawArc(component, sw_graphics->gx_fg, offset_x, offset_y, diameter, diameter, 0.0, 360.0);

    offset_x += inner_margin;
    offset_y += inner_margin;
    diameter -= 2 * inner_margin;
    
    spSetGraphicsParams(sw_graphics->gx_fg,
                        SppForeground, SW_REC_BUTTON_MAIN_COLOR,
                        SppLineWidth, 1,
                        NULL);    
    spFillArc(component, sw_graphics->gx_fg, offset_x, offset_y, diameter, diameter, 0.0, 360.0);

    offset_x += icon_margin;
    offset_y += icon_margin;
    diameter -= 2 * icon_margin;
    
    spSetGraphicsParams(sw_graphics->gx_fg,
                        SppForeground, SW_REC_BUTTON_SUB_COLOR,
                        NULL);
    if (state == SW_REC_BUTTON_STATE_RECORDING) {
        size = diameter / 3;
        spFillRectangle(component, sw_graphics->gx_fg, offset_x, offset_y, size, diameter);
        spFillRectangle(component, sw_graphics->gx_fg, offset_x + diameter - size, offset_y, size, diameter);
    } else {
        spFillArc(component, sw_graphics->gx_fg, offset_x, offset_y, diameter, diameter, 0.0, 360.0);
    }
    
    spSetGraphicsParams(sw_graphics->gx_fg,
                        SppForeground, "black",
                        NULL);
    
    spUnlockMutex(sw_graphics->mutex);

    return;
}

#if 1
void swDrawRecBar(spComponent component, swWindow window, int left_offset, int top_offset, int draw_width, int draw_height,
                  swRecButtonState state)
{
    int n;
    int meter_width, meter_height;
    int button_area_size;
    double max;
    double value;
    double dBvalue;
    swWave wave;

    wave = window->wave;
    max = swGetWindowLimitValue(window, wave);

    button_area_size = draw_height;
    meter_width = draw_width - button_area_size;
    meter_height = draw_height / MAX(wave->num_channel, 1);

    swLockMutex(wave);
    
    for (n = 0; n < wave->num_channel; n++) {
        if (wave->maxindex[n] < 0) {
            value = 0.0;
            dBvalue = -300.0;
        } else {
            value = wave->maxvalue[n];
            dBvalue = FABS(value) / max;
            dBvalue = (dBvalue < 1e-30 ? -300.0 : dB(dBvalue));
        } 
        

        spDebug(80, "swDrawRecBar", "n = %d, value = %f, dBvalue = %f, max = %f\n",
                n, value, dBvalue, max);
        
        swDrawHorizontalMeterBar(component, window, 1.0,
                                 meter_width, meter_height, left_offset + button_area_size, top_offset, dBvalue);

        wave->minindex[n] = -1; wave->maxindex[n] = -1;
        wave->minvalue[n] = 0.0; wave->maxvalue[n] = 0.0;
    }

    swUnlockMutex(wave);
    
    swDrawRecButton(component, window, left_offset, top_offset, button_area_size, button_area_size,
                    SW_REC_BUTTON_MARGIN, SW_REC_BUTTON_INNER_MARGIN, SW_REC_BUTTON_ICON_MARGIN, state);
    
    return;
}
#endif
#endif

spBool swIsXAxisLogFrequency(swWindow window)
{
    if (window == NULL) return SP_FALSE;
    
    if (window->data_type == SW_FREQ_DATA && window->log_frequency_axis == SP_TRUE) {
        return SP_TRUE;
    } else {
        return SP_FALSE;
    }
}

double swGetDataRange(swWindow window, swWave wave, spBool scaled, double *data_min, double *data_max)
{
    double dmin, dmax;

    if (wave->num_order > 1) {
        if (wave->custom_x_axis != NODATA) {
            if (scaled == SP_TRUE && window->log_frequency_axis == SP_TRUE && wave->order_frequency_flag == SP_TRUE) {
                dmin = swCustomXMinLogValue(wave->custom_x_axis);
                dmax = log10(wave->custom_x_axis->data[wave->custom_x_axis->length - 1]);
            } else {
                dmin = wave->custom_x_axis->data[0];
                dmax = wave->custom_x_axis->data[wave->custom_x_axis->length - 1];
            }
        } else {
            dmin = 0.0;
            dmax = (double)(wave->num_order - 1);
            if (scaled == SP_TRUE && window->log_frequency_axis == SP_TRUE && wave->order_frequency_flag == SP_TRUE) {
                dmin = log10(SW_LOG_FREQUENCY_MIN_VALUE);
                dmax = log10(dmax);
            }
        }
    } else {
        dmin = swGetWaveMin(wave);
        dmax = swGetWaveMax(wave);
    }
    spDebug(100, "swGetDataRange", "dmin = %f, dmax = %f, wave->num_order = %ld\n", dmin, dmax, wave->num_order);
    
    if (data_min != NULL) *data_min = dmin;
    if (data_max != NULL) *data_max = dmax;

    return dmax - dmin;
}

double swGetAmplitudeRange(swWindow window, swWave wave, spBool scaled, double *data_min, double *data_max)
{
    double dmin, dmax;

    if (window->amp_max <= window->amp_min) {
        return swGetDataRange(window, wave, scaled, data_min, data_max);
    }

    if (scaled && window->log_frequency_axis == SP_TRUE && wave->order_frequency_flag == SP_TRUE) {
        if (wave->custom_x_axis != NODATA && window->amp_min <= 0.0) {
            dmin = swCustomXMinLogValue(wave->custom_x_axis);
        } else {
            dmin = log10(MAX(window->amp_min, SW_LOG_FREQUENCY_MIN_VALUE));
        }
        if (wave->custom_x_axis != NODATA && window->amp_max <= 0.0) {
            dmax = swCustomXMinLogValue(wave->custom_x_axis);
        } else {
            dmax = log10(MAX(window->amp_max, SW_LOG_FREQUENCY_MIN_VALUE));
        }
    } else {
        dmin = window->amp_min;
        dmax = window->amp_max;
    }
    spDebug(100, "swGetAmplitudeRange", "dmin = %f, dmax = %f, wave->num_order = %ld, scaled = %d\n", dmin, dmax, wave->num_order, scaled);
    
    if (data_min != NULL) *data_min = dmin;
    if (data_max != NULL) *data_max = dmax;

    return dmax - dmin;
}

void swGetOrderMinMax(swWindow window, swWave wave, spBool log_flag, spBool output_linear,
                      double *p_order_min, double *p_order_max)
{
    double amp_min, amp_max;
    double amp_range;
    double order_min;
    double order_max;
    double min_value, max_value;

    spDebug(100, "swGetOrderMinMax", "log_flag = %d, output_linear = %d\n", log_flag, output_linear);
    
    if (wave->custom_x_axis != NODATA) {
        max_value = wave->custom_x_axis->data[wave->custom_x_axis->length - 1];
        if (log_flag == SP_FALSE) {
            min_value = wave->custom_x_axis->data[0];
        } else {
            min_value = swCustomXMinLogValue(wave->custom_x_axis);
            max_value = log10(max_value);
        }
    } else {
        max_value = (double)MAX(wave->num_order - 1, 1);
        if (log_flag == SP_FALSE) {
            min_value = 0.0;
        } else {
            min_value = log10(SW_LOG_FREQUENCY_MIN_VALUE);
            max_value = log10(max_value);
        }
    }
    spDebug(100, "swGetOrderMinMax", "min_value = %f, max_value = %f\n", min_value, max_value);
    
    amp_range = swGetAmplitudeRange(window, wave, log_flag, &amp_min, &amp_max);
    spDebug(100, "swGetOrderMinMax", "amp_min = %f, amp_max = %f, amp_range = %f\n", amp_min, amp_max, amp_range);
    
    if (log_flag == SP_TRUE && output_linear == SP_FALSE) {
        order_min = MAX(amp_min, min_value);
        order_max = MIN(amp_max, max_value);
    } else {
        if (log_flag == SP_TRUE && output_linear == SP_TRUE) {
            order_min = pow(10.0, amp_min);
            order_max = pow(10.0, amp_max);
            min_value = pow(10.0, min_value);
            max_value = pow(10.0, max_value);
        } else {
            order_min = amp_min;
            order_max = amp_max;
        }
        order_min = MAX(order_min, min_value);
        order_max = MIN(order_max, max_value);
    }
    order_max = MAX(order_max, order_min);
    spDebug(100, "swGetOrderMinMax", "final order_min = %f, order_max = %f\n", order_min, order_max);
    
    if (p_order_min != NULL) *p_order_min = order_min;
    if (p_order_max != NULL) *p_order_max = order_max;
    
    return;
}

double swGetOrderRange(swWindow window, swWave wave, spBool log_flag, spBool output_linear,
                       double *p_order_min, double *p_order_max)
{
    double order_min_tmp;
    double order_max_tmp;
    double order_range;

    swGetOrderMinMax(window, wave, log_flag, output_linear, &order_min_tmp, &order_max_tmp);

    if (log_flag == SP_TRUE) {
        if (output_linear == SP_TRUE) {
            double min_value;
            if (wave->custom_x_axis != NODATA) {
                min_value = pow(10.0, swCustomXMinLogValue(wave->custom_x_axis));
            } else {
                min_value = SW_LOG_FREQUENCY_MIN_VALUE;
            }
            
            order_range = log10(MAX((double)order_max_tmp, min_value)) - log10(MAX((double)order_min_tmp, min_value));
        } else {
            order_range = order_max_tmp - order_min_tmp;
        }
    } else {
        order_range = order_max_tmp - order_min_tmp;
        if (wave->custom_x_axis == NODATA) {
            order_range = MAX(order_range, 1.0);
            order_range = MIN(order_range, (double)(wave->num_order - 1));
        }
    }
    spDebug(100, "swGetOrderRange", "order_min_tmp = %f, order_max_tmp = %f, order_range = %f\n",
            order_min_tmp, order_max_tmp, order_range);
    
    if (p_order_min != NULL) *p_order_min = order_min_tmp;
    if (p_order_max != NULL) *p_order_max = order_max_tmp;

    return order_range;
}

double swGetDataHeight(swWindow window, swWave wave, spBool scaled, double *data_min, double *data_max)
{
    double dmin, dmax;
    
    swGetDataRange(window, wave, scaled, &dmin, &dmax);
    
    if (dmin >= dmax) {
        dmin = 0.0; dmax = 1.0;
    }
    
    if (data_min != NULL) *data_min = dmin;
    if (data_max != NULL) *data_max = dmax;

    return dmax - dmin;
}

static double swCalcScaleFactor(int target_width, double data_min, double data_max, int direction, spBool vscale_flag, double *o_zero_offset)
{
    double data_width;
    double factor;

    data_width = data_max - data_min;
    if (data_width == 0.0) {
        factor = (double)direction;
    } else {
        factor = (double)(direction * target_width) / data_width;
    }
    
    if (o_zero_offset != NULL) {
        if (vscale_flag == SP_FALSE) {
            *o_zero_offset = factor * ((direction > 0) ? -data_min : data_max);
        } else {
            *o_zero_offset = factor * ((direction > 0) ? data_max : -data_min);
        }
        spDebug(80, "swCalcScaleFactor", "target_width = %d, factor = %f, zero_offset = %f\n", target_width, factor, *o_zero_offset);
    }
    
    return factor;
}

static double swGetScaleTick(int width, double data_width, double min_scale_div, double *pw, spBool v_flag)
{
    int idx;
    double ndiv;
    double pn, fract;
    double tick, ntick;
    /* table for drawing scale */
    static double scale_table[] = {1.0, 2.0, 5.0, 10.0};
    static double scale_log_table[] = {0.0, 0.301, 0.699, 1.0};

    spDebug(100, "swGetScaleTick", "width = %d, data_width = %f\n", width, data_width);
    
    if (data_width <= 0.0) {
        fract = 0.0;
        pn = 0.0;
        ndiv = 0.0;
    } else {
        ndiv = MAX(floor((double)width / min_scale_div), 2.0);
        fract = modf(log10(data_width / ndiv), &pn);
    }

    if (pn < 0.0 || fract < 0.0) {
        pn = pn - 1.0;
        fract = 1.0 + fract;
    }

    spDebug(40, "swGetScaleTick", "pn = %f, fract = %f, ndiv = %f\n", pn, fract, ndiv);

    if (fract > scale_log_table[0] && fract <= scale_log_table[1]) {
        idx = 1;
    } else if (fract > scale_log_table[1] && fract <= scale_log_table[2]) {
        idx = 2;
    } else if (fract > scale_log_table[2] && fract <= scale_log_table[3]) {
        idx = 0;
        pn++;
    } else {
        idx = 0;
    }
    tick = scale_table[idx];

    ntick =  tick * pow(10.0, pn);
    ndiv = floor(data_width / ntick);
    if (ndiv * tick >= 10.0) {
        tick /= 10.0;
        pn++;
    }

    if (pw != NULL) {
        *pw = pn;
    }

    return tick;
}

void swCalcLinearScaleTickParams(int target_width, double data_min, double data_max, double min_scale_div, spBool vscale_flag,
                                 double *o_tick, double *o_ntick, double *o_n, double *o_pn,
                                 double *o_data_width, long *o_ndmin, long *o_ndmax)
{
    double data_width;
    double tick, ntick;
    double n, pn;
    
    data_width = data_max - data_min;

    tick = swGetScaleTick(target_width, data_width, min_scale_div, &pn, vscale_flag);
    n = pow(10.0, pn);
    ntick = n * tick;

    if (o_tick != NULL) *o_tick = tick;
    if (o_ntick != NULL) *o_ntick = ntick;
    if (o_n != NULL) *o_n = n;
    if (o_pn != NULL) *o_pn = pn;
    if (o_data_width != NULL) *o_data_width = data_width;
    if (o_ndmin != NULL) *o_ndmin = (long)floor(data_min / ntick);
    if (o_ndmax != NULL) *o_ndmax = (long)ceil(data_max / ntick);

    return;
}

void swCalcLogScaleTickParams(double data_factor, double data_min, double data_max, 
                              double *o_tick, double *o_ntick, double *o_n, double *o_pn,
                              double *o_data_width, long *o_ndmin, long *o_ndmax,
                              double *o_log_scale_min, double *o_log_scale_max)
{
    double log_scale_min, log_scale_max;
    double data_width;
    
    log_scale_min = log10(MAX(data_min, SW_LOG_FREQUENCY_MIN_VALUE * data_factor));
    log_scale_max = log10(MAX(data_max, SW_LOG_FREQUENCY_MIN_VALUE * data_factor));
    data_width = log_scale_max - log_scale_min;
    spDebug(80, "swCalcLogScaleTickParams", "data_min = %f, log_scale_min = %f, data_max = %f, log_scale_max = %f, data_width = %f\n",
            data_min, log_scale_min, data_max, log_scale_max, data_width);

    if (o_tick != NULL) *o_tick = 10.0;
    if (o_ntick != NULL) *o_ntick = 1.0;
    if (o_n != NULL) *o_n = 1.0;
    if (o_pn != NULL) *o_pn = 0.0;
    if (o_data_width != NULL) *o_data_width = data_width;
    if (o_ndmin != NULL) *o_ndmin = (long)floor(log_scale_min);
    if (o_ndmax != NULL) *o_ndmax = (long)floor(log_scale_max);
    if (o_log_scale_min != NULL) *o_log_scale_min = log_scale_min;
    if (o_log_scale_max != NULL) *o_log_scale_max = log_scale_max;

    return;
}

void swCalcTickBasedScaleRange(double ntick, long ndmin, long ndmax,
                               double *o_scale_width, double *o_scale_min, double *o_scale_max)
{
    double scale_min, scale_max;
    
    scale_min = ntick * (double)ndmin;
    scale_max = ntick * (double)ndmax;

    if (scale_max == scale_min) {
        scale_min -= 1.0;
        scale_max += 1.0;
    }
    if (o_scale_width != NULL) *o_scale_width = scale_max - scale_min;
    if (o_scale_min != NULL) *o_scale_min = scale_min;
    if (o_scale_max != NULL) *o_scale_max = scale_max;

    return;
}

int swGetScaleString(char *string, double tick, double pn, double value)
{
    int flag = 0;
    int pni;
    double n, ntick;
    char exp_string[SP_MAX_MESSAGE];

    n = pow(10.0, pn);
    ntick = n * tick;
    
    if (value == 0.0) {
        sprintf(string, "0");
    } else if (pn < 0) {
        pni = (int)spRound(pn);
        if (tick < 1.0) {
            pni--;
        }
        switch (pni) {
          case -1:
            sprintf(string, "%.1f", value);
            break;
          case -2:
            sprintf(string, "%.2f", value);
            break;
          case -3:
            sprintf(string, "%.3f", value);
            break;
          case -4:
            sprintf(string, "%.4f", value);
            break;
          case -5:
            sprintf(string, "%.5f", value);
            break;
          default:
            if (tick < 1.0) {
                sprintf(string, "%.1f", value / n);
            } else {
                sprintf(string, "%.0f", value / n);
            }
            flag = 1;
        }
    } else {
        if (pn >= 6.0) {
            if (tick < 1.0) {
                sprintf(string, "%.1f", value / n);
            } else {
                sprintf(string, "%.0f", value / n);
            }
            flag = 1;
        } else {
            if (ntick < 1.0) {
                sprintf(string, "%.1f", value);
            } else {
                sprintf(string, "%.0f", value);
            }
        }
    }
                
    if (flag) {
        sprintf(exp_string, "x10e%.0f", pn);
        strcat(string, exp_string);
    }

    return flag;
}

static void swClipZeroChars(char *string)
{
    int i;
    
    for (i = (int)strlen(string) - 1; i >= 0; i--) {
        if (string[i] == '0') {
            string[i] = NUL;
        } else {
            if (i >= 1 && string[i] == '.') {
                string[i] = NUL;
            }
            break;
        }
    }

    return;
}

int swGetDrawXOffset(swConfig config, int keys_width, spBool draw_vertical_keys_flag, spBool for_print, int *io_width)
{
    int x_offset;
    
    if (for_print == SP_TRUE) {
        x_offset = SW_PRINT_LEFT_MARGIN;
    } else {
        x_offset = 0;
    }
    if (draw_vertical_keys_flag == SP_TRUE) {
        if (config->vertical_piano_keys_right == SP_FALSE) {
            x_offset += keys_width;
        }
        if (io_width != NULL) *io_width -= keys_width;
    }

    return x_offset;
}

void swGetHScaleParams(swConfig config, int x_width_waveform, double x_min, double x_max, double x_samp_dim_factor, 
                       double min_scale_div, spBool log_flag, spBool for_print,
                       double *o_x_factor, double *o_x_zero_offset, double *o_x_min_mod, double *o_x_max_mod,
                       double *o_tick, double *o_ntick, double *o_n, double *o_pn, long *o_ndmin, long *o_ndmax)
{
    long ndmax, ndmin;
    double tick, ntick;
    double n, pn;
    double x_data_width;
    double x_zero_offset, x_factor;

    if (log_flag == SP_FALSE) {
        swCalcLinearScaleTickParams(x_width_waveform, x_min, x_max, min_scale_div, SP_FALSE, &tick, &ntick, &n, &pn, &x_data_width, &ndmin, &ndmax);
    } else {
        swCalcLogScaleTickParams(x_samp_dim_factor, x_min, x_max, &tick, &ntick, &n, &pn, &x_data_width,
                                 &ndmin, &ndmax, &x_min, &x_max);
    }
    x_factor = swCalcScaleFactor(x_width_waveform, x_min, x_max, 1, SP_FALSE, &x_zero_offset);
    
    if (o_x_factor != NULL) *o_x_factor = x_factor;
    if (o_x_zero_offset != NULL) *o_x_zero_offset = x_zero_offset;
    if (o_x_min_mod != NULL) {
        if (log_flag == SP_FALSE) {
            *o_x_min_mod = x_min;
        } else {
            *o_x_min_mod = pow(10.0, x_min);
        }
    }
    if (o_x_max_mod != NULL) {
        if (log_flag == SP_FALSE) {
            *o_x_max_mod = x_max;
        } else {
            *o_x_max_mod = pow(10.0, x_max);
        }
    }
    
    if (o_tick != NULL) *o_tick = tick;
    if (o_ntick != NULL) *o_ntick = ntick;
    if (o_n != NULL) *o_n = n;
    if (o_pn != NULL) *o_pn = pn;
    if (o_ndmin != NULL) *o_ndmin = ndmin;
    if (o_ndmax != NULL) *o_ndmax = ndmax;
    
    return;
}

void swDrawHScaleCore(swConfig config, spComponent component, swDataType data_type, int num_channel, int channel,
                      int x_offset, int y_offset, int x_width_waveform, int x_width_vertical_keys,
                      int y_width, int y_width_horizontal_keys,
                      int draw_min, int draw_max, double x_factor, double x_zero_offset,
                      double tick, double ntick, double n, double pn, long ndmin, long ndmax, double min_scale_div,
                      spBool log_flag, spBool for_print)
{
    int w, h;
    int x, y;
    int strx, stry;
    int max_str_height;
    long k, m;
    double sx;
    double sx_sub;
    char string[SP_MAX_MESSAGE];

    spDebug(40, "swDrawHScaleCore", "x_width_waveform = %d, y_width = %d, draw_min = %d, draw_max = %d\n",
            x_width_waveform, y_width, draw_min, draw_max);
    
    if (for_print == SP_FALSE && ((channel <= 0 && draw_min > 0) || channel >= 1)) {
        /* for print, draw later in black color */
        spDrawLine(component, sw_graphics->gx_scale, x_offset, draw_min,
                   x_offset + x_width_waveform, draw_min);
    }
    
    if (config->grid_flag == SP_TRUE) {
        spLockMutex(sw_graphics->mutex);
        spSetGraphicsParams(sw_graphics->gx_scale, SppLineType, SP_LINE_DOT, NULL);
        for (k = ndmin; k <= ndmax; k++) {
            sx = (double)k * ntick;
            
            for (m = 1; m <= 9; m++) {
                sx_sub = sx + log10((double)m);
                x = (int)spRound(x_zero_offset + sx_sub * x_factor) + x_offset;

                if (x > x_offset && x < x_offset + x_width_waveform) {
                    spDrawLine(component, sw_graphics->gx_scale, x, draw_min, x, draw_max);
                }
                
                if (log_flag == SP_FALSE) {
                    break;
                }
            }
        }
        spSetGraphicsParams(sw_graphics->gx_scale, SppLineType, SP_LINE_SOLID, NULL);
        spUnlockMutex(sw_graphics->mutex);
    }

    if (for_print == SP_FALSE && draw_max <= draw_min + y_width) {
        spDebug(40, "swDrawHScaleCore", "draw channel separator, draw_min = %d, draw_max = %d, y_width = %d\n",
                draw_min, draw_max, y_width);
        
        /* for print, draw later in black color */
        spDrawLine(component, sw_graphics->gx_scale, x_offset, draw_max,
                   x_offset + x_width_waveform, draw_max);
    } else {
        spDebug(40, "swDrawHScaleCore", "**** draw_min = %d, draw_max = %d, y_width = %d\n",
                draw_min, draw_max, y_width);
    }
    
    if (config->scale_flag == SP_FALSE && for_print == SP_FALSE) {
        return;
    }

    spLockMutex(sw_graphics->mutex);
    
    if (for_print == SP_TRUE) {
        spSetGraphicsParams(sw_graphics->gx_scale,
                            SppForeground, "black",
                            NULL);
        if ((channel <= 0 && draw_min > 0) || channel >= 1) {
            spDrawLine(component, sw_graphics->gx_scale, x_offset, draw_min,
                       x_offset + x_width_waveform, draw_min);
        }
        
        if (draw_max <= draw_min + y_width) {
            spDrawLine(component, sw_graphics->gx_scale, x_offset, draw_max,
                       x_offset + x_width_waveform, draw_max);
        }
    }

    max_str_height = 0;
    
    for (k = ndmin; k <= ndmax; k++) {
        sx = (double)k * ntick;
        
        for (m = 1; m <= 9; m++) {
            sx_sub = sx + log10((double)m);
            x = (int)spRound(x_zero_offset + sx_sub * x_factor) + x_offset;

            if (x >= x_offset && x <= x_offset + x_width_waveform) {
                /*if (x == x_width_waveform) x -= 1;*/
            
                if (x > x_offset && x < x_offset + x_width_waveform) {
                    spDrawLine(component, sw_graphics->gx_scale, x, draw_min, x, draw_min + SW_TICK_LENGTH);
                    spDrawLine(component, sw_graphics->gx_scale, x, draw_max, x, draw_max - SW_TICK_LENGTH);
                }

                if ((m == 1 || (x_factor > 16 * min_scale_div) || (x_factor > 4 * min_scale_div && m == 5))
                    && (x >= SW_HSCALE_LEFT_SPACING || for_print == SP_TRUE)
                    && channel == num_channel - 1) {
                    if (data_type != SW_FREQ_DATA
                        && config->time_format == SW_TIME_FORMAT_SEPARATED_SEC) {
                        spGetTimeString(sx_sub, SP_TIME_FORMAT_SEPARATED_SEC, string);
                        swClipZeroChars(string);
                    } else {
                        if (log_flag == SP_FALSE) {
                            swGetScaleString(string, tick, pn, sx_sub);
                        } else {
                            swGetScaleString(string, tick, (double)k, pow(10.0, sx_sub));
                        }
                    }
                
                    if (spGetStringExtent(component, sw_graphics->gx_scale, string, &strx, &stry, &w, &h, NULL) == SP_TRUE) {
                        if (max_str_height < h) max_str_height = h;
                    
                        /*x = x - w / 2;*/
                        if (for_print == SP_TRUE) {
                            y = draw_max - stry + SW_PRINT_HSCALE_STRING_SPACING;
                            if (config->horizontal_piano_keys_top == SP_FALSE) {
                                y += y_width_horizontal_keys;
                            }
                        } else {
                            x = MAX(x, SW_HSCALE_STRING_LEFT_OFFSET);
                            y = draw_max - SW_TICK_LENGTH - SW_HSCALE_STRING_BOTTOM_OFFSET;
                        }
                        spSetGraphicsParams(sw_graphics->gx_scale,
                                            SppStringAlignment, SP_ALIGNMENT_CENTER,
                                            NULL);
                        spDrawString(component, sw_graphics->gx_scale, x, y, string);
                        spSetGraphicsParams(sw_graphics->gx_scale,
                                            SppStringAlignment, SP_ALIGNMENT_BEGINNING,
                                            NULL);
                    }
                }
            }
                
            if (log_flag == SP_FALSE) {
                break;
            }
        }
    }

    if (for_print == SP_TRUE) {
        if (channel == num_channel - 1) {
            char title_string[SP_MAX_LINE];
            char dim_string[SP_MAX_LINE];
            int x_offset_vertical_keys;
            int draw_max_with_horizontal_keys;

            if (config->vertical_piano_keys_right == SP_FALSE) {
                x_offset_vertical_keys = x_offset - x_width_vertical_keys;
            } else {
                x_offset_vertical_keys = x_offset + x_width_waveform;
            }
        
            spSetGraphicsParams(sw_graphics->gx_scale,
                                SppLineWidth, 2,
                                SppStringAlignment, SP_ALIGNMENT_CENTER,
                                NULL);
            if (x_width_vertical_keys > 0) {
                spDrawRectangle(component, sw_graphics->gx_scale, x_offset_vertical_keys, y_offset,
                                x_width_vertical_keys, draw_max - y_offset);
            }

            if (config->horizontal_piano_keys_top == SP_FALSE) {
                draw_max_with_horizontal_keys = draw_max + y_width_horizontal_keys;
            } else {
                draw_max_with_horizontal_keys = draw_max;
            }

            if (y_width_horizontal_keys > 0) {
                spDrawRectangle(component, sw_graphics->gx_scale, x_offset,
                                config->horizontal_piano_keys_top == SP_FALSE ? draw_max : y_offset - y_width_horizontal_keys,
                                x_width_waveform, y_width_horizontal_keys);
            }
            
            spDrawRectangle(component, sw_graphics->gx_scale, x_offset, y_offset,
                            x_width_waveform, draw_max_with_horizontal_keys - y_offset);

            swGetTimeStringTitle(config, data_type, title_string, dim_string);
            sprintf(string, "%s %s", title_string, dim_string);
        
            if (spGetStringExtent(component, sw_graphics->gx_scale, string, &strx, &stry, &w, &h, NULL) == SP_TRUE) {
                spDrawString(component, sw_graphics->gx_scale, x_offset + x_width_waveform / 2,
                             draw_max_with_horizontal_keys + max_str_height + SW_PRINT_XLABEL_SPACING - stry, string);
            }
        }
            
        spSetGraphicsParams(sw_graphics->gx_scale,
                            SppForeground, config->scale_color,
                            SppLineWidth, 1, 
                            SppStringAlignment, SP_ALIGNMENT_BEGINNING,
                            NULL);
    }
    
    spUnlockMutex(sw_graphics->mutex);
    
    return;
}

void swDrawHScaleMain(swConfig config, spComponent component, swDataType data_type, int num_channel, int channel,
                      int x_offset, int y_offset, int x_width_waveform, int x_width_vertical_keys,
                      int y_width, int y_width_horizontal_keys,
                      int draw_min, int draw_max, double x_min, double x_max, double x_samp_dim_factor, 
                      spBool log_flag, spBool for_print)
{
    double x_zero_offset, x_factor;
    double tick, ntick;
    double n, pn;
    double min_scale_div;
    long ndmax, ndmin;

    min_scale_div = for_print ? SW_MIN_HSCALE_DIV_FOR_PRINT : SW_MIN_HSCALE_DIV;
    
    swGetHScaleParams(config, x_width_waveform, x_min, x_max, x_samp_dim_factor, min_scale_div, log_flag, for_print,
                      &x_factor, &x_zero_offset, &x_min, &x_max, &tick, &ntick, &n, &pn, &ndmin, &ndmax);

    swDrawHScaleCore(config, component, data_type, num_channel, channel,
                     x_offset, y_offset, x_width_waveform, x_width_vertical_keys,
                     y_width, y_width_horizontal_keys, draw_min, draw_max,
                     x_factor, x_zero_offset, tick, ntick, n, pn, ndmin, ndmax, min_scale_div,
                     log_flag, for_print);
    
    return;
}

void swCalcDrawXMinMax(swWindow window, spLong offset, spLong length,
                       double *o_x_min, double *o_x_max, double *o_x_samp_dim_factor)
{
    spLong end_pos;
    double x_max;

    end_pos = offset + length - 1;
    x_max = swSampToCurrentDim(window, end_pos);
    if (o_x_max != NULL) *o_x_max = x_max;
    if (o_x_min != NULL) *o_x_min = swSampToCurrentDim(window, offset);
    if (o_x_samp_dim_factor) *o_x_samp_dim_factor = (end_pos >= 1 ? x_max / (double)end_pos : 1.0);
    
    return;
}

void swDrawHScale(spComponent component, swWindow window, swWave wave, int channel, spLong offset, spLong length,
                  int x_offset, int y_offset, int x_width, int x_width_vertical_keys, int y_width, int y_width_horizontal_keys,
                  int draw_min, int draw_max, spBool for_print)
{
    double x_min, x_max;
    double x_samp_dim_factor;

    swCalcDrawXMinMax(window, offset, length, &x_min, &x_max, &x_samp_dim_factor);

    swDrawHScaleMain(window->config, component, window->data_type, wave->num_channel, channel,
                     x_offset, y_offset, x_width, x_width_vertical_keys, y_width, y_width_horizontal_keys,
                     draw_min, draw_max, x_min, x_max, x_samp_dim_factor, 
                     swIsXAxisLogFrequency(window), for_print);

    return;
}

static double swCalcDataFactor(swWindow window, swWave wave, double data_min, double data_max, spBool specgram_flag, 
                               double *o_scaled_data_min, double *o_scaled_data_max, spBool *o_y_log_flag)
{
    double y_data_factor;
    spBool y_log_flag = SP_FALSE;

    y_data_factor = 1.0;

    spDebug(40, "swCalcDataFactor", "input: data_min = %f, data_max = %f\n", data_min, data_max);
    
    if (wave->num_order > 1) {
        if (specgram_flag == SP_TRUE && wave->custom_x_axis == NODATA) {
            y_data_factor = (window->wave->samp_rate / 2.0) / (double)(wave->num_order - 1);
            spDebug(40, "swCalcDataFactor", "spectrogram: y_data_factor = %f, data_min = %f, data_max = %f\n",
                    y_data_factor, data_min, data_max);
            data_min *= y_data_factor;
            data_max *= y_data_factor;
        }
        y_log_flag = window->log_frequency_axis;
    } else {
        if (wave->num_order <= 1
            && window->config->percent_amplitude == SP_TRUE
            && window->data_type != SW_FREQ_DATA) {
            y_data_factor = 100.0 / swGetWindowLimitValue(window, wave);
        
            data_min *= y_data_factor;
            data_max *= y_data_factor;
        }
    }

    spDebug(40, "swCalcDataFactor", "output: data_min = %f, data_max = %f\n", data_min, data_max);

    if (o_scaled_data_min != NULL) *o_scaled_data_min = data_min;
    if (o_scaled_data_max != NULL) *o_scaled_data_max = data_max;
    if (o_y_log_flag != NULL) *o_y_log_flag = y_log_flag;
    
    return y_data_factor;
}

void swGetVScaleParams(swConfig config, int direction, int y_width, 
                       double y_scale_min, double y_scale_max, double y_data_factor, double min_scale_div, spBool log_flag,
                       double *o_y_factor, double *o_y_zero_offset, double *o_y_scale_min_mod, double *o_y_scale_max_mod,
                       double *o_tick, double *o_ntick, double *o_n, double *o_pn, long *o_ndmin, long *o_ndmax)
{
    double y_scale_height;
    double y_factor, y_zero_offset;
    double tick, ntick;
    double n, pn;
    long ndmax, ndmin;

    spDebug(40, "swGetVScaleParams", "y_scale_min = %f, y_scale_max = %f\n", y_scale_min, y_scale_max);

    if (log_flag == SP_FALSE) {
        swCalcLinearScaleTickParams(y_width, y_scale_min, y_scale_max, min_scale_div, SP_TRUE, &tick, &ntick, &n, &pn,
                                    &y_scale_height, &ndmin, &ndmax);
    } else {
        swCalcLogScaleTickParams(y_data_factor, y_scale_min, y_scale_max, &tick, &ntick, &n, &pn,
                                 &y_scale_height, &ndmin, &ndmax, &y_scale_min, &y_scale_max);
    }
    spDebug(80, "swGetVScaleParams", "tick = %f, ntick = %f, n = %f, pn = %f, y_scale_height = %f, ndmin = %ld, ndmax = %ld\n",
            tick, ntick, n, pn, y_scale_height, ndmin, ndmax);
    
    if (y_scale_height <= 0.0) {
        swCalcTickBasedScaleRange(ntick, ndmin, ndmax, &y_scale_height, &y_scale_min, &y_scale_max);
    }

    spDebug(40, "swGetVScaleParams", "y_scale_min = %f, y_scale_max = %f, y_scale_height = %f\n",
            y_scale_min, y_scale_max, y_scale_height);
    
    y_factor = swCalcScaleFactor(y_width, y_scale_min, y_scale_max, direction, SP_TRUE, &y_zero_offset);
    spDebug(80, "swGetVScaleParams", "y_width = %d, y_factor = %f, y_zero_offset = %f\n", y_width, y_factor, y_zero_offset);

    if (o_y_factor != NULL) *o_y_factor = y_factor;
    if (o_y_zero_offset != NULL) *o_y_zero_offset = y_zero_offset;
    if (o_y_scale_min_mod != NULL) {
        if (log_flag == SP_FALSE) {
            *o_y_scale_min_mod = y_scale_min;
        } else {
            *o_y_scale_min_mod = pow(10.0, y_scale_min);
        }
    }
    if (o_y_scale_max_mod != NULL) {
        if (log_flag == SP_FALSE) {
            *o_y_scale_max_mod = y_scale_max;
        } else {
            *o_y_scale_max_mod = pow(10.0, y_scale_max);
        }
    }
    if (o_tick != NULL) *o_tick = tick;
    if (o_ntick != NULL) *o_ntick = ntick;
    if (o_n != NULL) *o_n = n;
    if (o_pn != NULL) *o_pn = pn;
    if (o_ndmin != NULL) *o_ndmin = ndmin;
    if (o_ndmax != NULL) *o_ndmax = ndmax;
    
    return;
}

void swDrawVScaleCore(swConfig config, spComponent component, swDataType data_type, int num_channel, int channel,
                      int x_offset, int x_width_waveform, int x_width_vertical_keys,
                      int y_width, int y_width_horizontal_keys, int draw_min, int draw_max,
                      double y_data_factor, double y_factor, double y_zero_offset, 
                      double tick, double ntick, double n, double pn, long ndmin, long ndmax, double min_scale_div,
                      char *y_label, spBool log_flag, spBool for_print)
{
    int y_draw_direct;
    int x, y;
    int w, h;
    int strx, stry;
    int max_str_width;
    int v_str_offset;
    int x_offset_left_edge;
    long k, m;
    double y_zero;
    double sy;
    double sy_sub;
    char string[SP_MAX_MESSAGE];

    y_draw_direct = -1;

    y_zero = y_zero_offset;
    y_zero += (double)draw_min;

    if (x_width_vertical_keys > 0 && config->vertical_piano_keys_right == SP_TRUE) {
        x_offset_left_edge = x_offset;
    } else {
        x_offset_left_edge = x_offset - x_width_vertical_keys;
    }
    
    if (config->grid_flag == SP_TRUE || config->zero_flag == SP_TRUE) {
        spLockMutex(sw_graphics->mutex);
        spSetGraphicsParams(sw_graphics->gx_scale, SppLineType, SP_LINE_DOT, NULL);
        if (config->grid_flag == SP_TRUE) {
            for (k = ndmin; k <= ndmax; k++) {
                sy = (double)k * ntick;

                for (m = 1; m <= 9; m++) {
                    sy_sub = sy + log10((double)m);
                
                    y = (int)spRound(y_zero - sy_sub * y_factor);

                    if (y > draw_min && y < draw_max) {
                        spDrawLine(component, sw_graphics->gx_scale, x_offset, y, x_offset + x_width_waveform, y);
                    }
                
                    if (log_flag == SP_FALSE) {
                        break;
                    }
                }
            }
        } else {
            y = (int)spRound(y_zero);
            /*if (y >= draw_min && y <= draw_max) {*/
            if (y > draw_min && y < draw_max) {
                spDrawLine(component, sw_graphics->gx_scale, x_offset, y, x_offset + x_width_waveform, y);
            }
        }
        spSetGraphicsParams(sw_graphics->gx_scale, SppLineType, SP_LINE_SOLID, NULL);
        spUnlockMutex(sw_graphics->mutex);
    }

    if (config->scale_flag == SP_TRUE) {
        spLockMutex(sw_graphics->mutex);
        
        if (for_print == SP_TRUE) {
            spSetGraphicsParams(sw_graphics->gx_scale,
                                SppForeground, "black",
                                SppStringAlignment, SP_ALIGNMENT_END,
                                NULL);
        }

        max_str_width = 0;
        
        for (k = ndmin; k <= ndmax; k++) {
            sy = (double)k * ntick;
            
            for (m = 1; m <= 9; m++) {
                sy_sub = sy + log10((double)m);
                y = (int)spRound(y_zero - sy_sub * y_factor);

                if (y >= draw_min && y <= draw_max) {
                    if (y > draw_min && y < draw_max) {
                        spDrawLine(component, sw_graphics->gx_scale, x_offset, y, x_offset + SW_TICK_LENGTH, y);
                        spDrawLine(component, sw_graphics->gx_scale,
                                   x_offset + x_width_waveform - SW_TICK_LENGTH, y, x_offset + x_width_waveform, y);
                    }

                    if (m == 1 || (y_factor > 14 * min_scale_div) || (y_factor > 3 * min_scale_div && m == 5)) {
                        if (log_flag == SP_FALSE) {
                            swGetScaleString(string, tick, pn, sy_sub);
                        } else {
                            swGetScaleString(string, tick, (double)k, pow(10.0, sy_sub));
                        }
            
                        if (spGetStringExtent(component, sw_graphics->gx_scale, string, &strx, &stry, &w, &h, NULL) == SP_TRUE) {
                            if (max_str_width < w) max_str_width = w;
                    
                            y = y - y_draw_direct * ((-stry) / 2 - SW_VSCALE_STRING_BOTTOM_OFFSET);
                    
                            if (for_print == SP_TRUE) {
                                x = x_offset_left_edge - SW_PRINT_VSCALE_STRING_SPACING;
                            } else {
                                x = x_offset + SW_TICK_LENGTH + SW_VSCALE_STRING_LEFT_OFFSET;
                            }

                            if (for_print == SP_FALSE || channel > 0) {
                                if (for_print == SP_TRUE) {
                                    v_str_offset = SW_PRINT_VSCALE_STRING_TOP_OFFSET;
                                } else {
                                    v_str_offset = SW_VSCALE_STRING_TOP_OFFSET;
                                }
                                y = MAX(y, (draw_min + v_str_offset - y_draw_direct * (-stry)));
                            }
                            if (for_print == SP_FALSE || channel < num_channel - 1) {
                                if (for_print == SP_TRUE) {
                                    v_str_offset = SW_PRINT_VSCALE_STRING_BOTTOM_OFFSET;
                                } else {
                                    v_str_offset = SW_VSCALE_STRING_BOTTOM_OFFSET;
                                }
                                y = MIN(y, draw_max + y_draw_direct * v_str_offset);
                            }
                        
                            spDrawString(component, sw_graphics->gx_scale, x, y, string);
                        }
                    }
                }
                
                if (log_flag == SP_FALSE) {
                    break;
                }
            }
        }
        
        if (for_print == SP_TRUE) {
            if (!strnone(y_label) && channel == num_channel - 1) {
                spSetGraphicsParams(sw_graphics->gx_scale,
                                    SppStringRotation, 90,
                                    SppStringAlignment, SP_ALIGNMENT_CENTER,
                                    NULL);

                if (config->horizontal_piano_keys_top == SP_FALSE) {
                    y = draw_max / 2;
                } else {
                    y = y_width_horizontal_keys + (draw_max - y_width_horizontal_keys) / 2;
                }
                
                spDrawString(component, sw_graphics->gx_scale,
                             x_offset_left_edge - max_str_width - SW_PRINT_YLABEL_SPACING,
                             y, y_label);
            }
            
            spSetGraphicsParams(sw_graphics->gx_scale,
                                SppForeground, config->scale_color,
                                SppStringAlignment, SP_ALIGNMENT_BEGINNING,
                                SppStringRotation, 0,
                                NULL);
        }
        
        spUnlockMutex(sw_graphics->mutex);
    }

    return;
}

void swDrawVScaleMain(swConfig config, spComponent component, swDataType data_type, int num_channel, int channel,
                      int direction, int x_width, int x_width_vertical_keys,
                      int y_width, int y_width_horizontal_keys, int draw_min, int draw_max,
                      double y_scale_min, double y_scale_max, double y_data_factor, char *y_label,
                      spBool draw_flag, spBool log_flag, spBool for_print,
                      double *y_factor_waveform, double *y_zero_offset, double *y_scale_min_mod, double *y_scale_max_mod)
{
    int x_offset;
    long ndmax, ndmin;
    double tick, ntick;
    double n, pn;
    double y_factor, y_zero;
    double min_scale_div;

    min_scale_div = for_print ? SW_MIN_VSCALE_DIV_FOR_PRINT : SW_MIN_VSCALE_DIV;
    
    swGetVScaleParams(config, direction, y_width, y_scale_min, y_scale_max, y_data_factor, min_scale_div, log_flag,
                      &y_factor, &y_zero, y_scale_min_mod, y_scale_max_mod, &tick, &ntick, &n, &pn, &ndmin, &ndmax);
    if (y_factor_waveform != NULL) *y_factor_waveform = y_factor * y_data_factor;
    if (y_zero_offset != NULL) *y_zero_offset = y_zero;

    if (draw_flag == SP_FALSE) return;

    /* x_width --> x_width_waveform */
    x_offset = swGetDrawXOffset(config, x_width_vertical_keys,
                                x_width_vertical_keys > 0 ? SP_TRUE : SP_FALSE, for_print, &x_width);

    swDrawVScaleCore(config, component, data_type, num_channel, channel,
                     x_offset, x_width, x_width_vertical_keys, y_width, y_width_horizontal_keys,
                     draw_min, draw_max, y_data_factor, y_factor, y_zero, 
                     tick, ntick, n, pn, ndmin, ndmax, min_scale_div, y_label, log_flag, for_print);

    return;
}

static void swGetYLabelString(char *y_label, swWindow window, swWave wave, spBool specgram_flag)
{
    char *str;
    
    if (wave->num_order > 1) {
        if (specgram_flag == SP_FALSE) {
            strcpy(y_label, "Order");
        } else {
            strcpy(y_label, "Frequency [Hz]");
        }
    } else {
        str = swGetAnalysisNameString(wave);
        if (!strnone(str)) {
            strcpy(y_label, str);
        } else {
            strcpy(y_label, "Amplitude");
        }
        str = swGetAnalysisUnitString(wave, SP_TRUE);
        if (!strnone(str)) {
            if (streq(y_label, "Amplitude") && streq(str, "[dB]")) {
                strcpy(y_label, "Magnitude");
            }
            strcat(y_label, " ");
            strcat(y_label, str);
        } else if (window->config->percent_amplitude == SP_TRUE) {
            strcat(y_label, " ");
            strcat(y_label, "[%]");
        }
    }
    
    return;
}

void swDrawVScale(spComponent component, swWindow window, swWave wave, int channel,
                  int x_offset, int x_width, int y_width, int draw_min, int draw_max,
                  double data_min, double data_max, spBool draw_flag, spBool specgram_flag,
                  spBool draw_vertical_keys_flag, spBool for_print,
                  double *y_data_factor, double *y_factor_waveform, double *y_zero_offset,
                  double *y_scale_min, double *y_scale_max)
{
    int x_width_vertical_keys = 0;
    int y_width_horizontal_keys;
    char y_label[SP_MAX_LINE];
    spBool log_flag = SP_FALSE;

    *y_data_factor = swCalcDataFactor(window, wave, data_min, data_max, specgram_flag, &data_min, &data_max, &log_flag);
    
    if (draw_flag == SP_TRUE) {
        swGetYLabelString(y_label, window, wave, specgram_flag);
    }
    if (draw_vertical_keys_flag == SP_TRUE) {
        x_width_vertical_keys = (int)spRound(window->vertical_keys_width);
    }
    y_width_horizontal_keys = (int)spRound(window->draw_height_horizontal_keys);
    
    swDrawVScaleMain(window->config, component, window->data_type, wave->num_channel, channel,
                     window->direction, x_width, x_width_vertical_keys, y_width, y_width_horizontal_keys,
                     draw_min, draw_max, data_min, data_max, *y_data_factor,
                     y_label, draw_flag, log_flag, for_print,
                     y_factor_waveform, y_zero_offset, y_scale_min, y_scale_max);
    
    return;
}

static void drawCurrentLine(spComponent component, spGraphics graphics,
                            spLong thin_length, int draw_min, int draw_max,
                            int pmin, int pmax, int min, int max, int px, int py, int x, int y)
{
    spBool rect_flag = SP_FALSE;

    if (max < draw_min || min > draw_max) {
        /* outside of canvas */
    } else {
        if (x - px > 1) {
            rect_flag = SP_TRUE;
#if 0
            spDebug(100, "drawCurrentLine", "thin_length = %ld, x - px = %d\n",
                    thin_length, x - px);
#endif
        }
    
        if (min > pmax || max < pmin) {
            /* draw line from previous rectangle */
            pmin = MIN(min, pmin);
            pmax = MAX(max, pmax);
            pmin = MAX(pmin, draw_min);
            pmax = MIN(pmax, draw_max);
            if (rect_flag == SP_TRUE) {
                spDrawLine(component, sw_graphics->gx_fg, px, pmin, px, pmax);
                pmin = min; pmax = max;
            }
        } else {
            pmin = min; pmax = max;
        }

        pmin = MAX(pmin, draw_min);
        pmax = MIN(pmax, draw_max);
        if (rect_flag == SP_TRUE) {
            spFillRectangle(component, graphics, px, pmin, x - px,  MAX(pmax - pmin, 1));
        } else {
            spDrawLine(component, graphics, px, pmin, px, pmax);
        }
    }

    return;
}

spPixel swGetShadingPixel(swWindow window, swWave wave, double value)
{
    double min, max;
    double range;

    range = window->config->specgram_range;
    if (range <= 0.0) range = 1.0;
    max = swGetWaveMax(wave) + (double)MIN(0, window->config->specgram_limit_threshold);
    min = max - range;
    
    if (window->config->specgram_gray_scale == SP_TRUE) {
        unsigned char r, g, b;
        
        value = 255.0 * (1.0 - ((value - min) / range));

        if (value >= 255.0) {
            r = g = b = 255;
        } else if (value <= 0.0) {
            r = g = b = 0;
        } else {
            r = g = b = (unsigned char)round(value);
        }

        return spRGB(r, g, b);
    } else {
        double r, g, b;
        
        value = 256.0 * ((value - min) / range);

        if (value >= 256.0) {
            r = 128.0;
            g = b = 0;
        } else if (value >= 224.0) {
            r = 256.0 - (4.0 * (value - 224.0));
            g = b = 0;
        } else if (value >= 160.0) {
            g = 256.0 - (4.0 * (value - 160.0));
            r = 256.0;
            b = 0;
        } else if (value >= 96.0) {
            r = 4.0 * (value - 96.0);
            b = 256.0 - 4.0 * (value - 96.0);
            g = 256.0;
        } else if (value >= 32.0) {
            g = 4.0 * (value - 32.0);
            b = 256.0;
            r = 0;
        } else if (value > 0.0) {
            b = 4.0 * (value + 32.0);
            r = g = 0;
        } else {
            b = 128.0;
            r = g = 0;
        }

        r = MAX(0.0, r); r = MIN(255.0, r);
        g = MAX(0.0, g); g = MIN(255.0, g);
        b = MAX(0.0, b); b = MIN(255.0, b);
        
        return spRGB((unsigned char)round(r), (unsigned char)round(g),
                     (unsigned char)round(b));
    }
}

static void swFillShadingRectangle(spComponent component, swWindow window, swWave wave,
                                   int x_offset, int draw_min, int draw_size, 
                                   int px, int x, double min, double max, spLong min_index, spLong max_index,
                                   spBool ignore_min, double *last_value, spLong *last_index)
{
    spPixel first_pixel, latter_pixel;
    int first_len, latter_len;
    int x_update_min_size = 2;

    if (x <= x_offset || x <= px) return;

    px = MAX(px, x_offset);
    
    if (ignore_min == SP_TRUE || (min == max && (*last_value == max || *last_index == -1))) {
        first_pixel = swGetShadingPixel(window, wave, max);
        spSetForegroundPixel(sw_graphics->gx_shading, first_pixel);
        spFillRectangle(component, sw_graphics->gx_shading, px, draw_min, x - px, draw_size);
        *last_index = MAX(min_index, max_index);
        *last_value = max;
    } else {
        int i;
        int num_loop;
        int pos;
        int remain_size;
        int current_draw_size;
        double weight, weight1, weight2;
        double first_value, half_value;
        double prev_value, value;
        spPixel pixel;

        num_loop = (x - px) / x_update_min_size;

        if (num_loop < 3) {
            /* fill rectangle in first half */
            if (min_index < max_index) {
                first_pixel = swGetShadingPixel(window, wave, min);
                latter_pixel = swGetShadingPixel(window, wave, max);
                *last_index = max_index;
                *last_value = max;
            } else {
                first_pixel = swGetShadingPixel(window, wave, max);
                latter_pixel = swGetShadingPixel(window, wave, min);
                *last_index = min_index;
                *last_value = min;
            }
            first_len = (x - px) / 2;
            if (first_len > 0) {
                spSetForegroundPixel(sw_graphics->gx_shading, first_pixel);
                spFillRectangle(component, sw_graphics->gx_shading, px, draw_min, first_len, draw_size);
            }

            /* fill rectangle in latter half */
            latter_len = (x - px) - first_len;
            if (latter_len > 0) {
                spSetForegroundPixel(sw_graphics->gx_shading, latter_pixel);
                spFillRectangle(component, sw_graphics->gx_shading, px + first_len, draw_min, latter_len, draw_size);
            }
        } else {
#if 0
            if (px < 30 && min_index != max_index) {
                spDebug(80, "swFillShadingRectangle", "num_loop = %d, x = %d, px = %d, last_index = %ld, last_value = %f\n",
                        num_loop, x, px, *last_index, *last_value);
            }
#endif
            
            if (*last_index < 0) {
                if (min_index < max_index) {
                    first_value = min;
                } else {
                    first_value = max;
                }
            } else {
                first_value = *last_value;
            }
            
            if (min_index < max_index) {
                half_value = min;
                *last_index = max_index;
                *last_value = max;
            } else {
                if (min_index == max_index) {
                    half_value = (first_value + max) / 2.0;
                } else {
                    half_value = max;
                }
                *last_index = min_index;
                *last_value = min;
            }
#if 0
            if (px < 30 && min_index != max_index) {
                spDebug(80, "swFillShadingRectangle",
                        "min_index = %ld, max_index = %ld, first_value = %f half_value = %f, last_value = %f\n",
                        min_index, max_index, first_value, half_value, *last_value);
            }
#endif

            weight1 = (half_value - first_value) / (double)(num_loop / 2);
            weight2 = (*last_value - half_value) / (double)(num_loop / 2);

            pos = px;
            remain_size = x - px;
            prev_value = first_value;
        
            for (i = 0; i < num_loop; i++) {
                if (remain_size <= 0) {
                    break;
                }

                if (i >= num_loop - 1) {
                    current_draw_size = remain_size;
                } else {
                    current_draw_size = x_update_min_size;
                }

                if (i < num_loop / 2) {
                    weight = weight1;
                } else {
                    weight = weight2;
                }
            
                value = prev_value + weight;

#if 0
                if (px < 30 && min_index != max_index && (i == num_loop / 2 - 1 || i == num_loop - 1)) {
                    spDebug(80, "swFillShadingRectangle",
                            "i = %d, value = %f, first_value = %f, half_value = %f, last_value = %f, weight = %f\n",
                            i, value, first_value, half_value, *last_value, weight, value);
                }
#endif

                pixel = swGetShadingPixel(window, wave, value);
                spSetForegroundPixel(sw_graphics->gx_shading, pixel);
                spFillRectangle(component, sw_graphics->gx_shading, pos, draw_min, current_draw_size, draw_size);

                remain_size -= current_draw_size;
                pos += current_draw_size;

                prev_value = value;
            }
        }
    }

    return;
}

void swDrawShadingOrderBlock(spComponent component, swWindow window, swWave wave, int channel,
                             int x_offset, int x_width, double start_sec, double end_sec,
                             spLong wave_offset, spLong thin_length, 
                             spLong draw_offset_mod, spLong draw_length_mod, 
                             long prev_order_l, long order_l,
                             int order_draw_size, int order_draw_min, int order_draw_max, int x_update_size, 
                             spBool peak_flag, spBool ignore_min)
{
    int x, px;
    double current_sec;
    double next_sec;
    double boundary_sec;
    long m;
    spLong k, l;
    spLong draw_k_end;
    spLong min_index, max_index;
    double min, max;
    double value;
    spLong last_index;
    double last_value;

    spDebug(100, "swDrawShadingOrderBlock",
            "%ld: fill rectangle directly, draw_length_mod = %ld, x_update_size =  %d, start_sec = %f, end_sec = %f\n",
            order_l, draw_length_mod, x_update_size, start_sec, end_sec);

    last_index = -1;
    last_value = 0.0;
    min = max = 0.0;
    min_index = max_index = -1;
                
    if (draw_length_mod <= 1) {
        draw_k_end = 0;
        current_sec = start_sec;
    } else {
        draw_k_end = draw_length_mod - 2;
        current_sec = swTargetSampToDim(window, wave, SP_TRUE, wave_offset + draw_offset_mod * thin_length);
    }
    spDebug(100, "swDrawShadingOrderBlock", "%ld: draw_k_end = %ld, draw_length_mod = %ld, current_sec = %f\n",
            order_l, draw_k_end, draw_length_mod, current_sec);
    
    px = 0;

    for (k = 0; k <= draw_k_end; k++) {
        l = draw_offset_mod + k + 1;
        
        for (m = prev_order_l + 1; m <= order_l; m++) {
            if (peak_flag == SP_TRUE) {
                swGetPeakData(wave, channel, m, (l - 1) * thin_length, &value);
            } else {
                swGetWaveData(wave, channel, m, (l - 1), &value);
            }
                    
            if (max_index < 0) {
                /* initialize max */
                max = value;
                max_index = k;
            } else {
                if (value > max) {
                    max = value;
                    max_index = k;
                }
            }
        }

        if (min_index < 0 || max < min) {
            min = max;
            min_index = k;
        }
        
        if (k == draw_k_end) {
            boundary_sec = end_sec;
        } else {
            next_sec = swTargetSampToDim(window, wave, SP_TRUE, wave_offset + l * thin_length);
            boundary_sec = current_sec + (next_sec - current_sec) / 2.0;
            boundary_sec = MIN(boundary_sec, end_sec);
        }
        spDebug(100, "swDrawShadingOrderBlock",
                "%ld: k = %ld / %ld, l = %ld, boundary_sec = %f, next_sec = %f, start_sec = %f, end_sec = %f\n",
                order_l, k, draw_k_end, l, boundary_sec, next_sec, start_sec, end_sec);

        if ((boundary_sec > start_sec && !(thin_length >= 2 && l % 2 != 0)) || boundary_sec >= end_sec) {
            x = swDimToDrawWidth(window, x_width, boundary_sec);
            x = MIN(x, x_width);

            if (x - px >= x_update_size) {
                spDebug(100, "swDrawShadingOrderBlock", "%ld: k = %ld / %ld, x = %d, px = %d, min = %f (%ld), max = %f (%ld), boundary_sec = %f\n",
                        order_l, k, draw_k_end, x, px, min, min_index, max, max_index, boundary_sec);
                swFillShadingRectangle(component, window, wave, x_offset, order_draw_min, order_draw_size, 
                                       x_offset + px, x_offset + x, min, max, min_index, max_index,
                                       ignore_min, &last_value, &last_index);
                min_index = max_index = -1;
                px = x;
            }
        }

        if (boundary_sec >= end_sec) {
            break;
        }
        current_sec = next_sec;
    }

    return;
}

void swDrawWaveformShading(spComponent component, swWindow window, swWave wave, int channel, spLong offset, spLong length,
                           int x_offset, int x_width, int draw_min, int draw_max,
                           double y_factor, spLong draw_offset, spLong draw_length,
                           spBool order_log_flag, spBool peak_flag, spBool for_print)
{
    int x_update_size;
    double draw_factor;
    double wave_start_frac;
    double wave_end_frac;
    spLong thin_length;
    spLong wave_offset;
    double draw_offset_frac;
    double draw_end_frac;
    spLong draw_offset_mod;
    spLong draw_length_mod;
    double start_sec, end_sec;
    double simplify_thresh;
    double next_order;
    double prev_order;
    long order_index, prev_order_index;
    double order_min, order_max, order_range;
    double length_ms;
    int current_draw_min, current_draw_max;
    int current_draw_size;
    int current_draw_size_min;
    double log_order_max;
    double boundary_order, log_boundary_order;
    double min_boundary_order_for_log;
    spBool ignore_min;

    spDebug(50, "swDrawWaveformShading", "y_factor = %f, draw_offset = %ld, draw_length = %ld\n",
            y_factor, draw_offset, draw_length);

    spLockMutex(sw_graphics->mutex);

    order_range = swGetOrderRange(window, wave, order_log_flag, SP_TRUE, &order_min, &order_max);
    draw_factor = (double)(draw_max - draw_min) / order_range;
    
    spDebug(50, "swDrawWaveformShading", "order_min = %f, order_max = %f, order_range = %f, draw_factor = %f, draw_min = %d, draw_max = %d\n",
            order_min, order_max, order_range, draw_factor, draw_min, draw_max);
    
    if (peak_flag == SP_TRUE) {
        thin_length = wave->peak_buf_thin_length;
    } else {
        thin_length = wave->thin_length;
    }
    
    wave_offset = swGetWaveOffset(wave);
    spDebug(50, "swDrawWaveformShading", "draw_factor = %f, thin_length = %ld, wave_offset = %ld, play_flag = %d, process_flag = %d\n",
            draw_factor, thin_length, wave_offset, window->wave->core->play_flag, window->wave->core->process_flag);

    if (window->draw_detail == SP_TRUE || for_print == SP_TRUE) {
        simplify_thresh = 2000.0;
    } else {
        simplify_thresh = 1000.0;
    }
    length_ms = 1000.0 * (double)window->length / window->wave->samp_rate;
    spDebug(50, "swDrawWaveformShading", "length_ms = %f, simplify_thresh = %f\n", length_ms, simplify_thresh);
    
    if (swIsWaveProcessing(window->wave) == SP_TRUE
        || ((window->config->specgram_simplified == SP_TRUE && for_print == SP_FALSE)
             && length_ms > simplify_thresh)) {
        current_draw_size_min = 2;
        x_update_size = 4;
        ignore_min = window->config->specgram_simplified;
    } else {
        current_draw_size_min = 1;
        x_update_size = 2;
        ignore_min = SP_FALSE;
    }
    spDebug(50, "swDrawWaveformShading", "current_draw_size_min = %d, x_update_size = %d\n",
            current_draw_size_min, x_update_size);
    
    current_draw_max = draw_max; /* draw bottom */
    min_boundary_order_for_log = SW_LOG_FREQUENCY_MIN_BOUNDARY_VALUE;
    
    if (wave->custom_x_axis != NODATA) {
        prev_order = order_min;
        prev_order_index = -1;
        if (wave->custom_x_axis->data[0] <= SW_LOG_FREQUENCY_MIN_VALUE && wave->custom_x_axis->length >= 2) {
            min_boundary_order_for_log = (swCustomXMinLogValue(wave->custom_x_axis) + log10(wave->custom_x_axis->data[1])) / 2.0;
        } else {
            min_boundary_order_for_log = (log10(wave->custom_x_axis->data[0]) + log10(wave->custom_x_axis->data[1])) / 2.0;
        }
        min_boundary_order_for_log = pow(10.0, min_boundary_order_for_log);
    } else {
        prev_order = order_min - 1.0;
        prev_order = MAX(prev_order, -1);
        prev_order_index = (long)floor(prev_order);
    }
    log_order_max = log10(order_max);
    spDebug(50, "swDrawWaveformShading", "current_draw_max = %d, prev_order = %f, prev_order_index = %ld\n",
            current_draw_max, prev_order, prev_order_index);

    start_sec = swSampToDim(window, window->offset); /* in second */
    wave_start_frac = swDimToFractionalTargetSamp(window, wave, start_sec);
    end_sec = swSampToDim(window, window->offset + window->length - 1); /* in second */
    wave_end_frac = swDimToFractionalTargetSamp(window, wave, end_sec);
    spDebug(50, "swDrawWaveformShading",
            "start_sec = %f, end_sec = %f, wave_start_frac = %f, wave_end_frac = %f, wave_offset = %ld, draw_offset = %ld, draw_length = %ld\n",
            start_sec, end_sec, wave_start_frac, wave_end_frac, wave_offset, draw_offset, draw_length);

    draw_offset_frac = (wave_start_frac - (double)wave_offset) / (double)thin_length;
    draw_end_frac = (wave_end_frac - (double)wave_offset) / (double)thin_length;
    spDebug(50, "swDrawWaveformShading", "draw_offset_frac = %f (%ld), draw_end_frac = %f (%ld)\n",
            draw_offset_frac, draw_offset, draw_end_frac, draw_length);
    draw_offset_mod = (spLong)spRound(draw_offset_frac);
    draw_length_mod = (spLong)spRound(draw_end_frac) - draw_offset_mod + 1;
    spDebug(50, "swDrawWaveformShading", "draw_offset_mod = %ld (%ld), draw_length_mod = %ld (%ld)\n",
            draw_offset_mod, draw_offset, draw_length_mod, draw_length);

    /* order_min: draw bottom, order_max: draw top */
    for (order_index = prev_order_index + 1;; order_index++) {
        if (wave->custom_x_axis != NODATA) {
            if (order_index >= wave->custom_x_axis->length - 1) {
                next_order = order_max + 1.0;
                spDebug(100, "swDrawWaveformShading", "custom_x_axis final: order_index = %ld / %ld, next_order = %f\n",
                        order_index, wave->custom_x_axis, next_order);
            } else {
                next_order = wave->custom_x_axis->data[order_index + 1];
            }
            boundary_order = (next_order + wave->custom_x_axis->data[order_index]) / 2.0;
        } else {
            next_order = (double)(order_index + 1);
            boundary_order = next_order - 0.5;
        }
        if (boundary_order > order_max) {
            current_draw_min = draw_min;
            spDebug(100, "swDrawWaveformShading", "final (order_index = %ld, order_max = %f): current_draw_min = %d, current_draw_max = %d\n",
                    order_index, order_max, current_draw_min, current_draw_max);
        } else {
            if (order_log_flag == SP_FALSE) {
                current_draw_min = draw_min + (int)round((order_max - boundary_order) * draw_factor);
            } else {
                boundary_order = MAX(boundary_order, min_boundary_order_for_log);
                log_boundary_order = log10(boundary_order);
                current_draw_min = draw_min + (int)round((log_order_max - log_boundary_order) * draw_factor);
                spDebug(100, "swDrawWaveformShading", "%ld: boundary_order = %f, log_boundary_order = %f, log_order_max = %f, current_draw_min = %d\n",
                        order_index, boundary_order, log_boundary_order, log_order_max, current_draw_min);
            }
        }
        current_draw_size = current_draw_max - current_draw_min;
        spDebug(100, "swDrawWaveformShading", "%ld: next_order = %f, boundary_order = %f, current_draw_min = %d, current_draw_max = %d, current_draw_size = %d\n",
                order_index, next_order, boundary_order, current_draw_min, current_draw_max, current_draw_size);

        if ((boundary_order > order_max && current_draw_size >= 1)
            || current_draw_size >= current_draw_size_min) {
#if 0
            if (prev_order_index < 0) {
                spDebug(50, "swDrawWaveformShading", "first draw: order_index = %ld, current_draw_size = %d\n",
                        order_index, current_draw_size);
            }
#endif
            swDrawShadingOrderBlock(component, window, wave, channel, x_offset, x_width,
                                    start_sec, end_sec, wave_offset,thin_length, draw_offset_mod, draw_length_mod, 
                                    prev_order_index, order_index, current_draw_size, current_draw_min, current_draw_max,
                                    x_update_size, peak_flag, ignore_min);
            
            current_draw_max = current_draw_min;
            prev_order_index = order_index;
        } else if (current_draw_size <= 0) {
            prev_order_index = order_index;
        }

        if (current_draw_min <= draw_min) {
            spDebug(100, "swDrawWaveformShading", "loop break: order_index = %ld, current_draw_min = %d, draw_min = %d\n",
                    order_index, current_draw_min, draw_min);
            break;
        }
    }
    
    spUnlockMutex(sw_graphics->mutex);
    
    return;
}

static int getXAxisPosition(spDVector custom_x_axis, double x_factor, spLong offset, spLong n, spBool x_log_flag)
{
    int x;
    double n_factored;
    
    if (x_log_flag == SP_FALSE) {
        if (custom_x_axis != NODATA) {
            if (n == 0) {
                n_factored = 0.0;
            } else {
                n_factored = (custom_x_axis->data[offset + n] - custom_x_axis->data[offset]) * x_factor;
            }
        } else {
            n_factored = (double)n * x_factor;
        }
    } else {
        if (n < 1) {
            n_factored = 0.0;
        } else {
            double region_min, samp;
            double log_region_min;
            double log_samp;

            spDebug(100, "getXAxisPosition", "x_factor = %f, offset = %ld, n = %ld\n",
                    x_factor, (long)offset, n);

            if (custom_x_axis != NODATA) {
                region_min = custom_x_axis->data[offset];
                if (offset <= 0) {
                    log_region_min = swCustomXMinLogValue(custom_x_axis);
                } else {
                    log_region_min = log10(MAX(region_min, SW_LOG_FREQUENCY_MIN_VALUE));
                }
                samp = custom_x_axis->data[offset + n];
            } else {
                region_min = (double)offset;
                log_region_min = log10(MAX(region_min, SW_LOG_FREQUENCY_MIN_VALUE));
                samp = (double)(offset + n);
            }
            
            log_samp = log10(MAX(samp, SW_LOG_FREQUENCY_MIN_VALUE));

            n_factored = (log_samp - log_region_min) * x_factor;
            spDebug(100, "getXAxisPosition", "log_region_min = %f, log_samp = %f, n_factored = %f\n",
                    log_region_min, log_samp, n_factored);
        }
    }
    
    x = (int)round(n_factored);

    return x;
}

void swDrawWaveformLine(spComponent component, swWave wave, int channel, spLong offset, spLong length,
                        int x_offset, int x_width, int draw_min, int draw_max,
                        double x_factor, double y_factor, double y_zero_offset,
                        spLong draw_offset, spLong draw_length,
                        spBool x_log_flag, spBool peak_flag, spBool for_print)
{
    int x, y;
    int px, py;
    int tx, ty;
    int min, max;
    int pmin, pmax;
    spLong k, l;
    spLong thin_length;
    double value;
    double zero_height;

    zero_height = (double)draw_min + y_zero_offset;

    if (peak_flag == SP_TRUE) {
        thin_length = wave->peak_buf_thin_length;
        swGetPeakData(wave, channel, 0, draw_offset * thin_length, &value);
    } else {
        thin_length = wave->thin_length;
        swGetWaveData(wave, channel, 0, draw_offset, &value);
    }
    
    spDebug(50, "swDrawWaveformLine", "x_factor = %f, thin_length = %ld, draw_offset = %ld, draw_length = %ld, peak_flag = %d\n",
            x_factor, thin_length, (long)draw_offset, (long)draw_length, peak_flag);
    
    px = getXAxisPosition(wave->num_order <= 1 ? wave->custom_x_axis : NODATA, x_factor, offset, draw_offset, x_log_flag) + x_offset;
    py = (int)round(zero_height - (value * y_factor));
    if (wave->custom_x_axis != NODATA || (x_factor > /*0.2*/0.1 && thin_length < 16)) {
        for (k = 1; k < draw_length; k++) {
            l = draw_offset + k;
            if (peak_flag == SP_TRUE) {
                swGetPeakData(wave, channel, 0, l * thin_length, &value);
            } else {
                swGetWaveData(wave, channel, 0, l, &value);
            }
            x = getXAxisPosition(wave->num_order <= 1 ? wave->custom_x_axis : NODATA, x_factor, offset, l, x_log_flag) + x_offset;
            y = (int)round(zero_height - (value * y_factor));

            if (y >= draw_min && y <= draw_max) {
                if (py >= draw_min && py <= draw_max) {
                    /* inside --> inside */
                    spDrawLine(component, sw_graphics->gx_fg, px, py, x, y);
                } else {
                    /* outside --> inside */
                    if (py < draw_min) {
                        px = px + (int)round((double)(draw_min - py) * (double)(x - px) / (double)(y - py));
                        py = draw_min;
                    } else {
                        px = px + (int)round((double)(draw_max - py) * (double)(x - px) / (double)(y - py));
                        py = draw_max;
                    }
                    
                    spDrawLine(component, sw_graphics->gx_fg, px, py, x, y);
                }
            } else {
                /* outside of canvas */
                if (py >= draw_min && py <= draw_max) {
                    /* inside --> outside */
                    if (y < draw_min) {
                        tx = px + (int)round((double)(draw_min - py) * (double)(x - px) / (double)(y - py));
                        ty = draw_min;
                    } else {
                        tx = px + (int)round((double)(draw_max - py) * (double)(x - px) / (double)(y - py));
                        ty = draw_max;
                    }
                    
                    spDrawLine(component, sw_graphics->gx_fg, px, py, tx, ty);
                } else {
                    /* outside --> outside */
                    if (py < draw_min && y > draw_max) {
                        tx = px + (int)round((double)(draw_min - py) * (double)(x - px) / (double)(y - py));
                        ty = draw_min;
                        px = px + (int)round((double)(draw_max - py) * (double)(x - px) / (double)(y - py));
                        py = draw_max;
                        spDrawLine(component, sw_graphics->gx_fg, px, py, tx, ty);
                    } else if (py > draw_max && y < draw_min) {
                        tx = px + (int)round((double)(draw_max - py) * (double)(x - px) / (double)(y - py));
                        ty = draw_max;
                        px = px + (int)round((double)(draw_min - py) * (double)(x - px) / (double)(y - py));
                        py = draw_min;
                        spDrawLine(component, sw_graphics->gx_fg, px, py, tx, ty);
                    }
                }
            }
            py = y;
            px = x;
        }
    } else {
        spDebug(50, "swDrawWaveformLine", "before drawing: px = %d, py = %d\n", px, py);
        x = px;        y = py;
        min = max = py;
        pmin = pmax = py;
        for (k = 1; k < draw_length; k++) {
            l = draw_offset + k;
            if (peak_flag == SP_TRUE) {
                swGetPeakData(wave, channel, 0, l * thin_length, &value);
            } else {
                swGetWaveData(wave, channel, 0, l, &value);
            }
            x = getXAxisPosition(wave->num_order <= 1 ? wave->custom_x_axis : NODATA, x_factor, offset, l, x_log_flag) + x_offset;
            y = (int)round(zero_height - (value * y_factor));
                
            if (py < min) min = py;
            if (py > max) max = py;
            
            if (x == px || l % 2 != 0) {
            } else {
                drawCurrentLine(component, sw_graphics->gx_fg, thin_length, draw_min, draw_max,
                                pmin, pmax, min, max, px, py, x, y);
                
                pmin = min; pmax = max;
                
                px = x;
                min = max = y;
            }
            py = y;
        }
        spDebug(50, "swDrawWaveformLine", "pmin = %d, pmax = %d, min = %d, max = %d\n", pmin, pmax, min, max);
        
        if (x > px) {
            drawCurrentLine(component, sw_graphics->gx_fg, thin_length, draw_min, draw_max,
                            pmin, pmax, min, max, px, py, x, y);
        }
        spDebug(50, "swDrawWaveformLine", "drawing done: x = %d, px = %d\n", x, px);
    }

    spDebug(80, "swDrawWaveformLine", "done\n");
    
    return;
}

void swDrawWaveform(spComponent component, swWindow window, swWave wave, int channel, spLong offset, spLong length,
                    int x_offset, int x_width, int draw_min, int draw_max,
                    double x_factor, double y_factor, double y_zero_offset,
                    spLong draw_offset, spLong draw_length,
                    spBool log_flag, spBool peak_flag, spBool for_print)
{
    spDebug(50, "swDrawWaveform", "offset = %ld, length = %ld, draw_offset = %ld, draw_length = %ld\n",
            (long)offset, (long)length, draw_offset, draw_length);

    if (wave->num_order > 1) {
        swDrawWaveformShading(component, window, wave, channel, offset, length,
                              x_offset, x_width, draw_min, draw_max, y_factor,
                              draw_offset, draw_length, log_flag, peak_flag, for_print);
    } else {
        swDrawWaveformLine(component, wave, channel, offset, length, 
                           x_offset, x_width, draw_min, draw_max, x_factor, y_factor, y_zero_offset,
                           draw_offset, draw_length, log_flag, peak_flag, for_print);
    }

    return;
}

void swFillRegion(spComponent component, spGraphics graphics, swWindow window,
                  double y_draw_offset, double draw_width, double draw_height, int channel, int start_d, int end_d, spBool overview_flag)
{
    int st, ed;
    int width;
    int window_width;
    int x_offset;

    if (window == NULL || (start_d < 0 && end_d < 0) || start_d == end_d)
        return;

    window_width = (int)draw_width;
    
    if (overview_flag == SP_FALSE
        && window->draw_vertical_keys == SP_TRUE && window->config->vertical_piano_keys_right == SP_FALSE) {
        x_offset = (int)spRound(window->vertical_keys_width);
        /*window_width -= x_offset;*/
    } else {
        x_offset = 0;
    }
    spDebug(50, "swFillRegion", "start_d = %d, end_d = %d, draw_width = %f\n", start_d, end_d, draw_width);
        
    if (start_d < end_d) {
        st = MAX(start_d, 0);
        ed = MIN(end_d, window_width);
    } else {
        st = MAX(end_d, 0);
        ed = MIN(start_d, window_width);
    }

    width = ed - st;
    spDebug(50, "swFillRegion", "st = %d, ed = %d, width = %d\n", st, ed, width);

    if (width > 0) {

        if (channel >= 0) {
            spFillRectangle(component, graphics, st + x_offset,
                            (int)spRound((double)channel * draw_height + y_draw_offset),
                            width, (int)spRound(draw_height));
        } else {
#if 0
            if (draw_height <= 0.0) {
                spFillRectangle(component, graphics, st + x_offset, (int)spRound(y_draw_offset), width, window->height);
            } else {
                spFillRectangle(component, graphics, st + x_offset, (int)spRound(y_draw_offset), width, (int)draw_height);
            }
#else
            spFillRectangle(component, graphics, st + x_offset, (int)spRound(y_draw_offset), width,
                            (int)spRound(draw_height * (double)window->wave->num_channel));
#endif
        }
    }

    return;
}

void swDrawRegionLabels(spComponent component, swWindow window)
{
    long k;
    int st_d, ed_d;
    int draw_width;

    if (swIsNoLabel(window) == SP_TRUE || window->draw_label == SP_FALSE)
        return;

#if 0
    if (window->drag_label_type == SW_DRAG_NO_LABEL) {
        swSortLabels(window->wave->labels, &window->active_label_index);
        spDebug(30, "swDrawRegionLabels", "sorted active label %ld\n", window->active_label_index);
    }
#endif
    
    draw_width = swGetDrawWidth(window, SP_TRUE);
    
    spDebug(30, "swDrawRegionLabels", "num_buffer = %ld\n", window->wave->labels->num_buffer);

    for (k = 0; k < window->wave->labels->num_buffer; k++) {
        if (window->wave->labels->label[k].time >= 0.0
            && window->wave->labels->label[k].end_time >= 0.0) {
#if 0
            long st, ed;
            st = swDimToSamp(window, window->wave->labels->label[k].time);
            ed = swDimToSamp(window, window->wave->labels->label[k].end_time);
            st_d = swSampToDisp(window, st);
            ed_d = swSampToDisp(window, ed);
#else
            st_d = swDimToDisp(window, window->wave->labels->label[k].time);
            ed_d = swDimToDisp(window, window->wave->labels->label[k].end_time);
#endif

            swFillRegion(component, sw_graphics->gx_region_bg, window,
                         0.0, (double)draw_width, /*window->draw_height*/window->height, -1, st_d, ed_d, SP_FALSE);
        }
    }
    
    spDebug(80, "swDrawRegionLabels", "done\n");
    
    return;
}

void swFillBackground(spComponent component, int width, int height)
{
    spFillRectangle(component, sw_graphics->gx_bg, 0, 0, width, height);
    spDebug(80, "swFillBackground", "done\n");
    return;
}

void swDrawBackground(spComponent component, swWindow window)
{
    if (window == NULL)
        return;

    swFillBackground(component, window->width, window->height);
    spDebug(40, "swDrawBackground", "fill rectangle done\n");

#ifndef SW_DRAW_LABEL_CANVAS
    swDrawRegionLabels(component, window);
#endif

    return;
}

swWave swGetTargetWave(swWindow window)
{
    swWave wave;
    
    if (window == NULL) return NULL;

#if 0
    if (swIsSpectrogramVisible(window) == SP_TRUE) {
        wave = window->specgram;
        spDebug(100, "swGetTargetWave", "wave is spectrogram\n");
    } else {
        wave = window->wave;
        spDebug(100, "swGetTargetWave", "wave is waveform\n");
    }
#else
    if (window->first_sub_area == NULL || window->first_sub_area->wave == NULL) {
        spDebug(100, "swGetTargetWave", "not found\n");
        return NULL;
    }
    
    wave = window->first_sub_area->wave;
    spDebug(100, "swGetTargetWave", "wave = %ld\n", (long)wave);
#endif

    return wave;
}

#if 1
#define SW_PIANO_BLACK_KEY_PERCENT_LENGTH 63
#define SW_PIANO_MIN_OCTAVE_INDEX 0
#define SW_PIANO_MAX_OCTAVE_INDEX 8
#define SW_PIANO_KEY_END_COLOR "grey80"
#define SW_PIANO_C_MARK_MARGIN /*4*/3
#define SW_PIANO_C_MARK_MIN_MARGIN 2
#define SW_PIANO_C_MARK_PERCENT_LENGTH /*20*/28
#define SW_PIANO_EACH_KEY_DRAW_THRESHOLD 60
#define SW_PIANO_BLACK_KEY_NOTE_ON_COLOR "grey50"
#define SW_PIANO_WHITE_KEY_NOTE_ON_COLOR "grey90"

static void swDrawPianoCMark(spComponent component, swWindow window, int draw_min, int draw_max,
                             double factor, double zero_pos, double min_pos, double max_pos,
                             int octave_index, double prev_boundary_freq,
                             int C_mark_default_size, int ipos, int x1, int y1, int x2, int y2,
                             spBool horizontal_flag, spBool reverse_direction, spBool log_flag)
{
    int ipos2;
    int offset;
    int current_size;
    int current_margin_size;
    int C_mark_size;
    int sx, sy, swidth, sheight;
    int sx2, sy2;
    double prev_boundary_pos;
    spBool mid_C_flag = SP_FALSE;
    char string[SP_MAX_MESSAGE];

    if (window->config->mid_C_octave_index == octave_index) {
        mid_C_flag = SP_TRUE;
    }
    
    if (log_flag == SP_FALSE) {
        prev_boundary_pos = zero_pos + factor * prev_boundary_freq;
    } else {
        prev_boundary_pos = zero_pos + factor * log10(prev_boundary_freq);
    }
                            
    sx = sy = swidth = sheight = sx2 = sy2 = 0;
    
    if (horizontal_flag == SP_FALSE) {
        ipos2 = (int)spRound(prev_boundary_pos);
        current_margin_size = MAX((ipos2 - ipos - C_mark_default_size) / 2, SW_PIANO_C_MARK_MIN_MARGIN);
        y1 = ipos + current_margin_size;
        y2 = ipos2 - current_margin_size;
        C_mark_size = y2 - y1;
        y1 = MAX(y1, draw_min);
        y2 = MIN(y2, draw_max);
        current_size = y2 - y1;
        if (current_size > 0) {
            sprintf(string, "C%d", octave_index);
            if (spGetStringExtent(component, sw_graphics->gx_fg, string, &sx, &sy, &swidth, &sheight, NULL) == SP_TRUE) {
                if (swidth > C_mark_size || sheight > current_size) {
                    if (mid_C_flag == SP_FALSE) {
                        current_size = 0;
                    }
                    swidth = sheight = 0;
                } else {
                    sx2 = (C_mark_size - swidth) / 2 + x1 - sx;
                    sy2 = (current_size - sheight) / 2 + y1 - sy;
                }
            } else {
                if (mid_C_flag == SP_FALSE) {
                    current_size = 0;
                }
            }
        } else if (mid_C_flag == SP_TRUE) {
            C_mark_size = ipos2 - ipos;
            if (C_mark_size >= 5) {
                y1 = ipos + 1; y2 = ipos2 - 1;
                C_mark_size -= 2;
            } else {
                if (C_mark_size < 2) {
                    y1 = (ipos + ipos2) / 2 - 1;
                    y2 = y1 + 2;
                    C_mark_size = 2;
                } else {
                    y1 = ipos; y2 = ipos2;
                }
            }
            current_size = y2 - y1;
        }

        if (current_size > 0) {
            if (reverse_direction == SP_FALSE) {
                offset = (x2 - x1) - C_mark_size - SW_PIANO_C_MARK_MARGIN;
            } else {
                offset = SW_PIANO_C_MARK_MARGIN;
            }
            spSetGraphicsParams(sw_graphics->gx_fg, SppForeground, "red", NULL);
            if (mid_C_flag == SP_TRUE) {
                spFillRectangle(component, sw_graphics->gx_fg, x1 + offset, y1, C_mark_size, current_size);
                if (swidth > 0) {
                    spSetGraphicsParams(sw_graphics->gx_fg, SppForeground, "white", NULL);
                    spDrawString(component, sw_graphics->gx_fg, sx2 + offset, sy2, string);
                }
            } else {
                //spDrawRectangle(component, sw_graphics->gx_fg, x1 + offset, y1, C_mark_size, current_size);
                if (swidth > 0) {
                    spDrawString(component, sw_graphics->gx_fg, sx2 + offset, sy2, string);
                }
            }
            spSetGraphicsParams(sw_graphics->gx_fg, SppForeground, "black", NULL);
        }
    } else {
        ipos2 = (int)spRound(prev_boundary_pos);
        current_margin_size = MAX((ipos - ipos2 - C_mark_default_size) / 2, SW_PIANO_C_MARK_MIN_MARGIN);
        x1 = ipos2 + current_margin_size;
        x2 = ipos - current_margin_size;
        C_mark_size = x2 - x1;
        x1 = MAX(x1, (int)spRound(min_pos));
        x2 = MIN(x2, (int)spRound(max_pos));
        current_size = x2 - x1;
        if (current_size > 0) {
            sprintf(string, "C%d", octave_index);
            if (spGetStringExtent(component, sw_graphics->gx_fg, string, &sx, &sy, &swidth, &sheight, NULL) == SP_TRUE) {
                if (swidth > current_size || sheight > C_mark_size) {
                    if (mid_C_flag == SP_FALSE) {
                        current_size = 0;
                    }
                    swidth = sheight = 0;
                } else {
                    sx2 = (current_size - swidth) / 2 + x1 - sx;
                    sy2 = (C_mark_size - sheight) / 2 + y1 - sy;
                }
            } else {
                if (mid_C_flag == SP_FALSE) {
                    current_size = 0;
                }
            }
        } else if (mid_C_flag == SP_TRUE) {
            C_mark_size = ipos - ipos2;
            if (C_mark_size >= 5) {
                x1 = ipos2 + 1; x2 = ipos - 1;
                C_mark_size -= 2;
            } else {
                if (C_mark_size < 2) {
                    x1 = (ipos2 + ipos) / 2 - 1;
                    x2 = x1 + 2;
                    C_mark_size = 2;
                } else {
                    x1 = ipos2; x2 = ipos;
                }
            }
            current_size = x2 - x1;
        }
            
        if (current_size > 0) {
            if (reverse_direction == SP_FALSE) {
                offset = (y2 - y1) - C_mark_size - SW_PIANO_C_MARK_MARGIN;
            } else {
                offset = SW_PIANO_C_MARK_MARGIN;
            }
            spSetGraphicsParams(sw_graphics->gx_fg, SppForeground, "red", NULL);
            if (mid_C_flag == SP_TRUE) {
                spFillRectangle(component, sw_graphics->gx_fg, x1, y1 + offset, current_size, C_mark_size);
                if (swidth > 0) {
                    spSetGraphicsParams(sw_graphics->gx_fg, SppForeground, "white", NULL);
                    spDrawString(component, sw_graphics->gx_fg, sx2, sy2 + offset, string);
                }
            } else {
                //spDrawRectangle(component, sw_graphics->gx_fg, x1, y1 + offset, current_size, C_mark_size);
                if (swidth > 0) {
                    spDrawString(component, sw_graphics->gx_fg, sx2, sy2 + offset, string);
                }
            }
            spSetGraphicsParams(sw_graphics->gx_fg, SppForeground, "black", NULL);
        }
    }

    return;
}

static void swDrawPianoBoundaryLine(spComponent component, swWindow window, int black_key_size,
                                    int x1, int x2, int y1, int y2, int ipos,
                                    spBool no_black_key_overlap, spBool horizontal_flag, spBool reverse_direction)
{
    int offset = 0;

    if (no_black_key_overlap == SP_FALSE) {
        if (reverse_direction == SP_FALSE) {
            offset = black_key_size;
        } else {
            offset = 0;
        }
    }
                    
    if (horizontal_flag == SP_FALSE) {
        spDrawLine(component, sw_graphics->gx_fg, x1 + offset, ipos, x2, ipos);
    } else {
        spDrawLine(component, sw_graphics->gx_fg, ipos, y1 + offset, ipos, y2);
    }
    
    return;
}

#if 1
static void swFillPianoRectangle(spComponent component, swWindow window, int draw_min, int draw_max, 
                                 double factor, double zero_pos, double min_pos, double max_pos, int key_size,
                                 int x1, int x2, int y1, int y2, int low_pos, int high_pos,
                                 spBool horizontal_flag, spBool reverse_direction)
{
    int offset;
    int current_size;
    
    if (horizontal_flag == SP_FALSE) {
        y1 = MAX(high_pos, draw_min);
        y2 = MIN(low_pos, draw_max);
        current_size = y2 - y1;
        if (current_size > 0) {
            if (reverse_direction == SP_FALSE) {
                offset = 0;
            } else {
                offset = (x2 - x1) - key_size;
            }
            spFillRectangle(component, sw_graphics->gx_fg, x1 + offset, y1, key_size, current_size);
        }
    } else {
        x1 = MAX(low_pos, (int)spRound(min_pos));
        x2 = MIN(high_pos, (int)spRound(max_pos));
        current_size = x2 - x1;
        if (current_size > 0) {
            if (reverse_direction == SP_FALSE) {
                offset = 0;
            } else {
                offset = (y2 - y1) - key_size;
            }
            spFillRectangle(component, sw_graphics->gx_fg, x1, y1 + offset, current_size, key_size);
        }
    }
    
    return;
}
#endif

void swDrawPianoKeys(spComponent component, swWindow window, int channel,
                     int x_offset, int x_width, int y_width, int draw_min, int draw_max, double min_freq, double max_freq,
                     int only_octave_index, int only_key,
                     spBool horizontal_flag, spBool reverse_direction, spBool log_flag)
{
    int i;
    int key;
    int white_key_size;
    int black_key_size;
    int C_mark_default_size;
    int x1, y1;
    int x2, y2;
    int ipos, ipos2;
    int current_size;
    int draw_key_count;
    int drawn_key;
    double log_min_freq = 0.0;
    double log_max_freq = 0.0;
    double log_freq_range = 0.0;
    double min_boundary_freq;
    double max_boundary_freq;
    double prev_boundary_freq;
    double next_boundary_freq;
    double min_freq_check_freq;
    double prev_boundary_pos;
    double next_boundary_pos;
    double octave_start_freq;
    double octave_end_freq;
    double octave_interval_length;
    double factor;
    double zero_pos;
    double min_pos, max_pos;
    double cent, cent2;
    double freq, freq2;
    spBool prev_boundary_visible;
    spBool next_boundary_visible;
    spBool freq_visible, freq2_visible;
    int octave_index;
    char notename[8];

    if (min_freq >= max_freq) return;

    if (x_width <= 0 || y_width <= 0 || draw_max <= draw_min) {
        spDebug(50, "swDrawPianoKeys", "parameter error: draw_min = %d, draw_max = %d, x_width = %d, y_width = %d\n",
                draw_min, draw_max, x_width, y_width);
        return;
    }

    spNoteNameToFreq("C", SW_PIANO_MIN_OCTAVE_INDEX, -50.0, window->config->mid_C_octave_index, &min_boundary_freq);
    spNoteNameToFreq("B", SW_PIANO_MAX_OCTAVE_INDEX, 50.0, window->config->mid_C_octave_index, &max_boundary_freq);
    spDebug(50, "swDrawPianoKeys", "min_boundary_freq = %f, max_boundary_freq = %f, min_freq = %f, max_freq = %f\n",
            min_boundary_freq, max_boundary_freq, min_freq, max_freq);
    
    spDebug(50, "swDrawPianoKeys", "draw_min = %d, draw_max = %d, x_width = %d, y_width = %d\n",
            draw_min, draw_max, x_width, y_width);

    spLockMutex(sw_graphics->mutex);
    
    x1 = y1 = 0;
    x2 = y2 = 0;
    
    if (horizontal_flag == SP_FALSE) {
        min_pos = (double)draw_min;
        max_pos = (double)draw_max;
        spDebug(50, "swDrawPianoKeys", "vertical: min_pos = %f, max_pos = %f\n", min_pos, max_pos);
        x1 = x_offset;
        x2 = x_offset + x_width;
        white_key_size = x_width;
        black_key_size = (int)spRound((double)white_key_size * SW_PIANO_BLACK_KEY_PERCENT_LENGTH / 100.0);
        C_mark_default_size = (int)spRound((double)x_width * SW_PIANO_C_MARK_PERCENT_LENGTH / 100.0);
        spDebug(50, "swDrawPianoKeys", "vertical: x1 = %d, x2 = %d, black_key_size = %d, C_mark_default_size = %d\n",
                x1, x2, black_key_size, C_mark_default_size);
    } else {
        min_pos = (double)x_offset;
        max_pos = (double)(x_offset + x_width);
        spDebug(50, "swDrawPianoKeys", "horizontal: min_pos = %f, max_pos = %f\n", min_pos, max_pos);
        y1 = draw_min;
        y2 = draw_max;
        white_key_size = y_width;
        black_key_size = (int)spRound((double)white_key_size * SW_PIANO_BLACK_KEY_PERCENT_LENGTH / 100.0);
        C_mark_default_size = (int)spRound((double)y_width * SW_PIANO_C_MARK_PERCENT_LENGTH / 100.0);
        spDebug(50, "swDrawPianoKeys", "horizontal: y1 = %d, y2 = %d, black_key_size = %d, C_mark_default_size = %d\n",
                y1, y2, black_key_size, C_mark_default_size);
    }
    
    if ((min_freq > max_boundary_freq || max_freq < min_boundary_freq)
        || (only_octave_index >= 0
            && (only_octave_index < SW_PIANO_MIN_OCTAVE_INDEX || only_octave_index > SW_PIANO_MAX_OCTAVE_INDEX))) {
        if (only_key < 0) {
            spSetGraphicsParams(sw_graphics->gx_fg, SppForeground, SW_PIANO_KEY_END_COLOR, NULL);
            spFillRectangle(component, sw_graphics->gx_fg, x_offset, draw_min, x_width, y_width);
            spSetGraphicsParams(sw_graphics->gx_fg, SppForeground, "black", NULL);
        }
        goto loop_end;
    }

    if (log_flag == SP_FALSE) {
        if (horizontal_flag == SP_FALSE) {
            factor = (double)(-y_width) / (max_freq - min_freq);
            zero_pos = factor * (-max_freq);
        } else {
            factor = (double)x_width / (max_freq - min_freq);
            zero_pos = factor * (-min_freq);
        }
    } else {
        log_min_freq = log10(min_freq);
        log_max_freq = log10(max_freq);
        log_freq_range = log_max_freq - log_min_freq;
        if (horizontal_flag == SP_FALSE) {
            factor = (double)(-y_width) / log_freq_range;
            zero_pos = factor * (-log_max_freq);
        } else {
            factor = (double)x_width / log_freq_range;
            zero_pos = factor * (-log_min_freq);
        }
    }
    zero_pos += min_pos;
    spDebug(50, "swDrawPianoKeys", "factor = %f, zero_pos = %f\n", factor, zero_pos);

    if (only_octave_index < 0) {
        spFreqToNoteName(MAX(min_freq, min_boundary_freq), window->config->mid_C_octave_index, notename, &octave_index, NULL);
    } else {
        octave_index = only_octave_index;
    }
    strcpy(notename, "C");
    spNoteNameToFreq(notename, octave_index, -50.0, window->config->mid_C_octave_index, &prev_boundary_freq);
    if (log_flag == SP_FALSE) {
        prev_boundary_pos = zero_pos + factor * prev_boundary_freq;
    } else {
        prev_boundary_pos = zero_pos + factor * log10(prev_boundary_freq);
    }
    spDebug(50, "swDrawPianoKeys", "prev_boundary_freq = %f, prev_boundary_pos = %f octave_index = %d\n",
            prev_boundary_freq, prev_boundary_pos, octave_index);

    spSetGraphicsParams(sw_graphics->gx_fg,
                        SppForeground, "black",
                        NULL);

    draw_key_count = 0;
    octave_start_freq = octave_end_freq = 0.0;
    octave_interval_length = -1.0;
    
    for (i = 0;; i++) {
        drawn_key = -1;
        
        spDebug(100, "swDrawPianoKeys", "i = %d, octave_index = %d, draw_key_count = %d\n", i, octave_index, draw_key_count);
        for (key = 0; key < 12; key++) {
            spNoteNameToFreq(notename, octave_index, (double)key * 100.0 + 50.0, window->config->mid_C_octave_index, &next_boundary_freq);
            spDebug(100, "swDrawPianoKeys", "%d: key = %d, only_key = %d, draw_key_count = %d, next_boundary_freq = %f, prev_boundary_freq = %f, min_freq = %f, max_freq = %f\n",
                    i, key, only_key, draw_key_count, next_boundary_freq, prev_boundary_freq, min_freq, max_freq);
            if (spIsIndexBlackKey(key) == SP_FALSE && !(key == 4/*E*/ || key == 11/*B*/)) {
                spNoteNameToFreq(notename, octave_index, (double)key * 100.0 + 100.0, window->config->mid_C_octave_index, &freq2);
                
                min_freq_check_freq = freq2;
            } else {
                min_freq_check_freq = next_boundary_freq;
            }

            if (key == 0) {
                octave_start_freq = prev_boundary_freq;
                spNoteNameToFreq(notename, octave_index, 1150.0, window->config->mid_C_octave_index, &octave_end_freq);
                octave_interval_length = -1.0;
            }

            if (only_key < 0 || key == only_key || (only_key >= 0 && draw_key_count > 0)) {
                if (min_freq_check_freq >= min_freq && prev_boundary_freq <= max_freq) {
                    if (octave_interval_length < 0.0) {
                        if (log_flag == SP_FALSE) {
                            octave_interval_length = fabs(factor) * (octave_end_freq - octave_start_freq);
                        } else {
                            octave_interval_length = fabs(factor) * log10(octave_end_freq / octave_start_freq);
                        }
                        spDebug(80, "swDrawPianoKeys", "%d: key = %d, octave_interval_length = %f\n",
                                i, key, octave_interval_length);
                    }
                    if (log_flag == SP_FALSE) {
                        next_boundary_pos = zero_pos + factor * next_boundary_freq;
                    } else {
                        next_boundary_pos = zero_pos + factor * log10(next_boundary_freq);
                    }
                    spDebug(100, "swDrawPianoKeys", "%d: key = %d, next_boundary_freq = %f, next_boundary_pos = %f octave_index = %d\n",
                            i, key, next_boundary_freq, next_boundary_pos, octave_index);

                    prev_boundary_visible = (prev_boundary_freq >= min_freq && prev_boundary_freq <= max_freq) ? SP_TRUE : SP_FALSE;
                
                    if (only_key < 0 && draw_key_count == 0 && key == 0 && prev_boundary_visible == SP_TRUE) {
                        if (log_flag == SP_FALSE) {
                            prev_boundary_pos = zero_pos + factor * prev_boundary_freq;
                        } else {
                            prev_boundary_pos = zero_pos + factor * log10(prev_boundary_freq);
                        }
                        ipos = (int)spRound(prev_boundary_pos);
                        spDebug(50, "swDrawPianoKeys", "%d: first C draw, octave_index = %d, ipos = %d\n", i, octave_index, ipos);
                    
                        if (octave_index == SW_PIANO_MIN_OCTAVE_INDEX) {
                            if (horizontal_flag == SP_FALSE) {
                                current_size = draw_max - ipos;
                            } else {
                                current_size = ipos - (int)spRound(min_pos);
                            }
                            spDebug(50, "swDrawPianoKeys", "%d: draw lower end, current_size = %d\n", i, current_size);
                            if (current_size > 0) {
                                /* draw lower end */
                                spSetGraphicsParams(sw_graphics->gx_fg, SppForeground, SW_PIANO_KEY_END_COLOR, NULL);
                                if (horizontal_flag == SP_FALSE) {
                                    spFillRectangle(component, sw_graphics->gx_fg, x1, ipos, x2 - x1, current_size);
                                } else {
                                    spFillRectangle(component, sw_graphics->gx_fg, (int)spRound(min_pos), y1, current_size, y2 - y1);
                                }
                                spSetGraphicsParams(sw_graphics->gx_fg, SppForeground, "black", NULL);
                            }
                        }

                        swDrawPianoBoundaryLine(component, window, black_key_size,
                                                x1, x2, y1, y2, ipos, SP_TRUE, horizontal_flag, reverse_direction);
                        ++draw_key_count;
                    }

                    if (spIsIndexBlackKey(key) == SP_FALSE && !(key == 4/*E*/ || key == 11/*B*/)
                        && (only_key >= 0 || (key == 0 && window->config->mid_C_octave_index == octave_index)
                            || octave_interval_length >= SW_PIANO_EACH_KEY_DRAW_THRESHOLD)) { /* white key (exept for E, B)*/
                        cent = spGetNearestWhiteKeyBoundaryCent(key, SP_TRUE);
                        spNoteNameToFreq(notename, octave_index, cent, window->config->mid_C_octave_index, &freq);
                        freq2_visible = SP_FALSE;
                        
                        if (only_key >= 0) {
                            cent2 = spGetNearestWhiteKeyBoundaryCent(/*only_key*/key, SP_FALSE);
                            spNoteNameToFreq(notename, octave_index, cent2, window->config->mid_C_octave_index, &freq2);
                            if (log_flag == SP_FALSE) {
                                ipos2 = (int)spRound(zero_pos + factor * freq2);
                            } else {
                                ipos2 = (int)spRound(zero_pos + factor * log10(freq2));
                            }
                            freq2_visible = (freq2 >= min_freq && freq2 <= max_freq) ? SP_TRUE : SP_FALSE;
                        }

                        if (log_flag == SP_FALSE) {
                            ipos = (int)spRound(zero_pos + factor * freq);
                        } else {
                            ipos = (int)spRound(zero_pos + factor * log10(freq));
                        }

                        freq_visible = (freq >= min_freq && freq <= max_freq) ? SP_TRUE : SP_FALSE;
                    
                        if (freq_visible == SP_TRUE || (only_key >= 0 && freq2_visible == SP_TRUE)) {
                            spDebug(30, "swDrawPianoKeys", "%d: key = %d, cent = %f, freq = %f, ipos = %d\n",
                                    i, key, cent, freq, ipos);

                            if (only_key >= 0) {
                                spSetGraphicsParams(sw_graphics->gx_fg,
                                                    SppForeground, SW_PIANO_WHITE_KEY_NOTE_ON_COLOR,
                                                    NULL);
                                swFillPianoRectangle(component, window, draw_min, draw_max, 
                                                     factor, zero_pos, min_pos, max_pos, white_key_size,
                                                     x1, x2, y1, y2, ipos2, ipos,
                                                     horizontal_flag, reverse_direction);
                                spSetGraphicsParams(sw_graphics->gx_fg,
                                                    SppForeground, "black",
                                                    NULL);
                            }

                            if (octave_interval_length >= SW_PIANO_EACH_KEY_DRAW_THRESHOLD) {
                                if (freq_visible == SP_TRUE) {
                                    swDrawPianoBoundaryLine(component, window, black_key_size,
                                                            x1, x2, y1, y2, ipos, SP_FALSE, horizontal_flag, reverse_direction);
                                }
                                if (only_key >= 0 && freq2_visible == SP_TRUE) {
                                    swDrawPianoBoundaryLine(component, window, black_key_size,
                                                            x1, x2, y1, y2, ipos2, (key == 0 || key == 5) ? SP_TRUE : SP_FALSE,
                                                            horizontal_flag, reverse_direction);
                                }
                            }
                            ++draw_key_count;
                        }

                        if (key == 0) {
                            spDebug(80, "swDrawPianoKeys", "%d: draw C mark, octave_index = %d, cent = %f, freq = %f, ipos = %d\n",
                                    i, octave_index, cent, freq, ipos);
                            /* draw C mark */
                            swDrawPianoCMark(component, window, draw_min, draw_max, factor, zero_pos, min_pos, max_pos,
                                             octave_index, prev_boundary_freq, C_mark_default_size, ipos, x1, y1, x2, y2,
                                             horizontal_flag, reverse_direction, log_flag);
                        }
                    }
                    if ((key == 4/* E */
                         && (only_key >= 0 || octave_interval_length >= SW_PIANO_EACH_KEY_DRAW_THRESHOLD)) || key == 11/* B */) {
                        next_boundary_visible = (next_boundary_freq >= min_freq && next_boundary_freq <= max_freq) ? SP_TRUE : SP_FALSE;
                        
                        if (next_boundary_visible == SP_TRUE || only_key >= 0) {
                            ipos = (int)spRound(next_boundary_pos);
                            if (key == 11 && octave_index == SW_PIANO_MAX_OCTAVE_INDEX) {
                                spDebug(50, "swDrawPianoKeys", "%d: last draw, octave_index = %d, ipos = %d\n", i, octave_index, ipos);
                                if (only_key < 0) {
                                    if (horizontal_flag == SP_FALSE) {
                                        current_size = ipos - draw_min;
                                    } else {
                                        current_size = (int)spRound(max_pos) - ipos;
                                    }
                                    spDebug(50, "swDrawPianoKeys", "%d: draw higher end, current_size = %d\n", i, current_size);
                                    if (current_size > 0) {
                                        /* draw higher end */
                                        spSetGraphicsParams(sw_graphics->gx_fg, SppForeground, SW_PIANO_KEY_END_COLOR, NULL);
                                        if (horizontal_flag == SP_FALSE) {
                                            spFillRectangle(component, sw_graphics->gx_fg, x1, draw_min, x2 - x1, current_size);
                                        } else {
                                            spFillRectangle(component, sw_graphics->gx_fg, ipos, y1, current_size, y2 - y1);
                                        }
                                        spSetGraphicsParams(sw_graphics->gx_fg, SppForeground, "black", NULL);
                                    }
                                }
                            }
                        
                            if (only_key >= 0/* && octave_interval_length >= SW_PIANO_EACH_KEY_DRAW_THRESHOLD*/) {
                                cent2 = spGetNearestWhiteKeyBoundaryCent(only_key, SP_FALSE);
                                spNoteNameToFreq(notename, octave_index, cent2, window->config->mid_C_octave_index, &freq2);
                                if (log_flag == SP_FALSE) {
                                    ipos2 = (int)spRound(zero_pos + factor * freq2);
                                } else {
                                    ipos2 = (int)spRound(zero_pos + factor * log10(freq2));
                                }
                                freq2_visible = (freq2 >= min_freq && freq2 <= max_freq) ? SP_TRUE : SP_FALSE;
                                spSetGraphicsParams(sw_graphics->gx_fg,
                                                    SppForeground, SW_PIANO_WHITE_KEY_NOTE_ON_COLOR,
                                                    NULL);
                                swFillPianoRectangle(component, window, draw_min, draw_max, 
                                                     factor, zero_pos, min_pos, max_pos, white_key_size,
                                                     x1, x2, y1, y2, ipos2, ipos,
                                                     horizontal_flag, reverse_direction);
                                spSetGraphicsParams(sw_graphics->gx_fg,
                                                    SppForeground, "black",
                                                    NULL);
                                
                                if (octave_interval_length >= SW_PIANO_EACH_KEY_DRAW_THRESHOLD
                                    && freq2_visible == SP_TRUE) {
                                    swDrawPianoBoundaryLine(component, window, black_key_size,
                                                            x1, x2, y1, y2, ipos2, SP_TRUE, horizontal_flag, reverse_direction);
                                }
                            }
                            
                            if (next_boundary_visible == SP_TRUE
                                && (key == 11 || octave_interval_length >= SW_PIANO_EACH_KEY_DRAW_THRESHOLD)) {
                                swDrawPianoBoundaryLine(component, window, black_key_size,
                                                        x1, x2, y1, y2, ipos, SP_TRUE, horizontal_flag, reverse_direction);
                            }
                            ++draw_key_count;
                        }
                    }
                
                    if (spIsIndexBlackKey(key) == SP_TRUE) {
                        if (only_key >= 0 && key == only_key) {
                            spSetGraphicsParams(sw_graphics->gx_fg,
                                                SppForeground, SW_PIANO_BLACK_KEY_NOTE_ON_COLOR,
                                                NULL);
                        }

                        if (log_flag == SP_FALSE) {
                            prev_boundary_pos = zero_pos + factor * prev_boundary_freq;
                        } else {
                            prev_boundary_pos = zero_pos + factor * log10(prev_boundary_freq);
                        }

                        swFillPianoRectangle(component, window, draw_min, draw_max, 
                                             factor, zero_pos, min_pos, max_pos, black_key_size,
                                             x1, x2, y1, y2, (int)spRound(prev_boundary_pos), (int)spRound(next_boundary_pos),
                                             horizontal_flag, reverse_direction);
                        ++draw_key_count;
                    }
                } else if (prev_boundary_freq > max_boundary_freq) {
                    spDebug(30, "swDrawPianoKeys", "%d: goto loop_end: key = %d, prev_boundary_freq = %f, max_boundary_freq = %f\n",
                            i, key, prev_boundary_freq, max_boundary_freq);
                    goto loop_end;
                }

                if (only_key >= 0) {
                    if (only_key == 2/*D*/ || only_key == 7/*G*/ || only_key == 9/*A*/) {
                        if (key == only_key && draw_key_count >= 2) {
                            break;
                        } else if (key == only_key + 1) {
                            key -= 3; /* --> only_key - 1 */
                            spNoteNameToFreq(notename, octave_index, (double)key * 100.0 + 50.0, window->config->mid_C_octave_index, &next_boundary_freq);
                        } else if (key == only_key - 1 && draw_key_count >= 2) {
                            break;
                        }
                    } else if (only_key == 0/*C*/ || only_key == 5/*F*/) {
                        if (key == only_key + 1) {
                            break;
                        }
                    } else if (only_key == 4/*E*/ || only_key == 11/*B*/) {
                        if (key == only_key) {
                            if (draw_key_count <= 0 || draw_key_count >= 2) {
                                break;
                            }
                            key -= 2; /* --> only_key - 1 */
                            spNoteNameToFreq(notename, octave_index, (double)key * 100.0 + 50.0, window->config->mid_C_octave_index, &next_boundary_freq);
                        } else {
                            break;
                        }
                    } else {
                        break;
                    }
                }
            }
            
            prev_boundary_freq = next_boundary_freq;
        }

        ++octave_index;
        if (only_octave_index >= 0 || octave_index > SW_PIANO_MAX_OCTAVE_INDEX) {
            break;
        }
    }
  loop_end:
    
    if (horizontal_flag == SP_FALSE) {
        if (reverse_direction == SP_FALSE) {
            spDrawLine(component, sw_graphics->gx_fg, x2, (int)min_pos, x2, (int)max_pos);
        } else {
            spDrawLine(component, sw_graphics->gx_fg, x1, (int)min_pos, x1, (int)max_pos);
        }
        if (channel >= 1) {
            /* draw edge line for multi-channel data */
            spDrawLine(component, sw_graphics->gx_fg, x1, (int)min_pos, x2, (int)min_pos);
        }
    } else {
        if (reverse_direction == SP_FALSE) {
            spDrawLine(component, sw_graphics->gx_fg, (int)min_pos, draw_min, (int)max_pos, draw_min);
        } else {
            spDrawLine(component, sw_graphics->gx_fg, (int)min_pos, draw_max, (int)max_pos, draw_max);
        }
        if (channel >= 1) {
            /* draw edge line for multi-channel data */
            spDrawLine(component, sw_graphics->gx_fg, (int)min_pos, draw_min, (int)min_pos, draw_max);
        }
    }
    spSetGraphicsParams(sw_graphics->gx_fg,
                        SppForeground, window->config->wave_fg,
                        NULL);
    spUnlockMutex(sw_graphics->mutex);
    
    return;
}
#endif

static double swCalcXFactor(spDVector custom_x_axis, spLong wave_length, spLong offset, spLong length, double draw_current_width,
                            double log_min_value, spBool x_log_flag)
{
    double x_factor;
    
    if (x_log_flag == SP_FALSE) {
        if (custom_x_axis != NODATA) {
            double range;
            spLong end_pos;

            end_pos = offset + MAX(length - 1, 1);
            end_pos = MIN(end_pos, custom_x_axis->length - 1);
            spDebug(80, "swCalcXFactor", "custom_x_axis->data[%ld] = %f, custom_x_axis->data[%ld] = %f\n",
                    end_pos, custom_x_axis->data[end_pos], offset, custom_x_axis->data[offset]);
            range = custom_x_axis->data[end_pos] - custom_x_axis->data[offset];
            x_factor = draw_current_width / (double)range;
            spDebug(80, "swCalcXFactor", "range = %f, draw_current_width = %f, x_factor = %f\n", range, draw_current_width, x_factor);
        } else {
            x_factor = draw_current_width / (double)(MAX(wave_length - 1, 1));
        }
    } else {
        if (wave_length < 2 || (offset <= 0 && length < 2)
            || (custom_x_axis != NODATA && custom_x_axis->data[offset] <= 0 && custom_x_axis->data[offset + length - 1] <= 0)) {
            x_factor = draw_current_width;
        } else {
            double log_region_min, log_region_max;
            double log_region_range;

            if (custom_x_axis != NODATA && offset <= 0) {
                log_region_min = swCustomXMinLogValue(custom_x_axis);
            } else if (custom_x_axis == NODATA && offset <= 0) {
                log_region_min = log_min_value;
            } else {
                if (custom_x_axis != NODATA) {
                    log_region_min = log10(MAX(custom_x_axis->data[offset], SW_LOG_FREQUENCY_MIN_VALUE));
                } else {
                    log_region_min = log10(MAX((double)offset, SW_LOG_FREQUENCY_MIN_VALUE));
                }
            }
            if (custom_x_axis != NODATA) {
                log_region_max = log10(custom_x_axis->data[offset + length - 1]);
            } else {
                log_region_max = log10((double)(offset + length - 1));
            }
            log_region_range = log_region_max - log_region_min;

            x_factor = draw_current_width / log_region_range;
        }
    }

    return x_factor;
}

static void swCalcDrawBasicSizeParams(swWindow window, swWave wave, spLong offset, spLong length,
                                      double draw_width, double draw_height, double draw_height_horizontal_keys,
                                      spBool specgram_flag, spBool for_print,
                                      int *o_x_offset_waveform, int *o_x_offset_vertical_keys,
                                      int *o_x_width_waveform, int *o_x_width_vertical_keys,
                                      int *o_y_width_waveform, int *o_y_width_horizontal_keys,
                                      double *o_x_factor_waveform, spBool *o_x_log_flag, spBool *o_y_log_flag,
                                      spBool *o_draw_vertical_keys_flag, spBool *o_draw_horizontal_keys_flag)
{
    int x_offset_waveform;
    int x_offset_vertical_keys;
    double x_factor_waveform;
    double draw_width_keys;
    double draw_width_waveform;
    spBool x_log_flag, y_log_flag;
    spBool draw_vertical_keys_flag = SP_FALSE;
    spBool draw_horizontal_keys_flag = SP_FALSE;

    spDebug(30, "swCalcDrawBasicSizeParams", "width = %f, height = %f, window->draw_vertical_keys = %d, specgram_flag = %d\n",
            draw_width, draw_height, window->draw_vertical_keys, specgram_flag);
    spDebug(80, "swCalcDrawBasicSizeParams", "offset = %ld, length = %ld\n", (long)offset, (long)length);

    if (window->draw_vertical_keys == SP_TRUE && specgram_flag == SP_TRUE) {
        draw_vertical_keys_flag = SP_TRUE;
        if (for_print) {
            draw_width_keys = SW_VERTICAL_PIANO_KEYS_DEFAULT_SIZE_FOR_PRINT;
        } else {
            draw_width_keys = window->vertical_keys_width;
        }
        draw_width_keys = MIN(draw_width_keys, draw_width * SW_PIANO_KEYS_MAX_SIZE_PERCENT / 100.0);
        draw_width_waveform = spRound(MAX(draw_width - draw_width_keys, 2.0));
        draw_width_keys = MIN(draw_width_keys, draw_width - draw_width_waveform);
    } else {
        draw_width_keys = 0.0;
        draw_width_waveform = draw_width;
    }
    spDebug(30, "swCalcDrawBasicSizeParams", "draw_width_keys = %f, draw_width_waveform = %f\n",
            draw_width_keys, draw_width_waveform);

    x_log_flag = y_log_flag = SP_FALSE;
    if (specgram_flag == SP_FALSE) {
        if (window->data_type == SW_FREQ_DATA) {
            x_log_flag = window->log_frequency_axis;
            draw_horizontal_keys_flag = window->config->draw_piano_keys_for_spectrum;
            if (draw_horizontal_keys_flag == SP_FALSE && window->draw_horizontal_keys == SP_TRUE) {
                draw_horizontal_keys_flag = SP_TRUE;
            }
        }
    } else {
        y_log_flag = window->log_frequency_axis;
    }
    spDebug(30, "swCalcDrawBasicSizeParams",
            "specgram_flag = %d, for_print = %d, x_log_flag = %d, y_log_flag = %d, draw_vertical_keys_flag = %d, draw_horizontal_keys_flag = %d\n",
            specgram_flag, for_print, x_log_flag, y_log_flag, draw_vertical_keys_flag, draw_horizontal_keys_flag);

    if (o_x_offset_waveform != NULL) {
        if (for_print == SP_TRUE) {
            x_offset_waveform = SW_PRINT_LEFT_MARGIN;
        } else {
            x_offset_waveform = 0;
        }
        if (draw_vertical_keys_flag == SP_TRUE && window->config->vertical_piano_keys_right == SP_FALSE) {
            x_offset_waveform += (int)spRound(draw_width_keys);
        }
        *o_x_offset_waveform = x_offset_waveform;
    }
    if (o_x_offset_vertical_keys != NULL) {
        x_offset_vertical_keys = 0;
        if (draw_vertical_keys_flag == SP_TRUE) {
            if (for_print == SP_TRUE) {
                x_offset_vertical_keys = SW_PRINT_LEFT_MARGIN;
            }
            if (window->config->vertical_piano_keys_right == SP_TRUE) {
                x_offset_vertical_keys += (int)spRound(draw_width_waveform);
            }
        }
        *o_x_offset_vertical_keys = x_offset_vertical_keys;
    }
    if (o_x_width_waveform != NULL) *o_x_width_waveform = (int)spRound(draw_width_waveform);
    if (o_x_width_vertical_keys != NULL) *o_x_width_vertical_keys = (int)spRound(draw_width_keys);
    if (o_y_width_waveform != NULL) *o_y_width_waveform = (int)spRound(draw_height);
    if (o_y_width_horizontal_keys != NULL) {
        if (draw_horizontal_keys_flag == SP_FALSE) {
            *o_y_width_horizontal_keys = 0;
        } else {
            *o_y_width_horizontal_keys = (int)spRound(draw_height_horizontal_keys);
        }
    }
    if (o_x_factor_waveform != NULL) {
        x_factor_waveform = swCalcXFactor(wave->num_order <= 1 ? wave->custom_x_axis : NODATA,
                                          wave->length, offset, length, draw_width_waveform,
                                          window->config->toplevel->log_min_value, x_log_flag);
        spDebug(30, "swCalcDrawBasicSizeParams", "x_factor_waveform = %f\n", x_factor_waveform);
        *o_x_factor_waveform = x_factor_waveform;
    }
    if (o_x_log_flag != NULL) *o_x_log_flag = x_log_flag;
    if (o_y_log_flag != NULL) *o_y_log_flag = y_log_flag;
    if (o_draw_vertical_keys_flag != NULL) *o_draw_vertical_keys_flag = draw_vertical_keys_flag;
    if (o_draw_horizontal_keys_flag != NULL) *o_draw_horizontal_keys_flag = draw_horizontal_keys_flag;
    
    return;
}

void swDrawWaveImageCore(spComponent component, swWindow window, swWave wave, spLong offset, spLong length,
                         double y_draw_offset, double draw_width, double draw_height, double draw_height_horizontal_keys,
                         double data_min, double data_max,
                         spLong draw_offset, spLong draw_pos, spBool specgram_flag, spBool for_print)
{
    int n;
    int x_offset_waveform, x_offset_vertical_keys;
    int x_width_waveform, x_width_vertical_keys;
    int y_width_waveform, y_width_horizontal_keys;
    int draw_min, draw_max;
    double x_factor, x_factor_waveform;
    double x_zero_offset;
    double y_factor, y_factor_waveform;
    double y_data_factor;
    double y_zero_offset;
    double y_scale_min_mod, y_scale_max_mod;
    double x_min, x_max;
    double x_samp_dim_factor;
    double min_freq, max_freq;
    double v_tick, v_ntick;
    double v_n, v_pn;
    long v_ndmax, v_ndmin;
    double h_tick, h_ntick;
    double h_n, h_pn;
    long h_ndmax, h_ndmin;
    double min_hscale_div, min_vscale_div;
    spBool x_log_flag, y_log_flag;
    spBool draw_vertical_keys_flag = SP_FALSE;
    spBool draw_horizontal_keys_flag = SP_FALSE;
    char y_label[SP_MAX_LINE];

    min_hscale_div = for_print ? SW_MIN_HSCALE_DIV_FOR_PRINT : SW_MIN_HSCALE_DIV;
    min_vscale_div = for_print ? SW_MIN_VSCALE_DIV_FOR_PRINT : SW_MIN_VSCALE_DIV;
    
    swCalcDrawBasicSizeParams(window, wave, offset, length,
                              draw_width, draw_height, draw_height_horizontal_keys, specgram_flag, for_print,
                              &x_offset_waveform, &x_offset_vertical_keys, &x_width_waveform, &x_width_vertical_keys,
                              &y_width_waveform, &y_width_horizontal_keys, &x_factor_waveform,
                              &x_log_flag, &y_log_flag, &draw_vertical_keys_flag, &draw_horizontal_keys_flag);
    spDebug(80, "swDrawWaveImageCore", "x_offset_waveform = %d, x_factor_waveform = %f, x_width_waveform = %d, x_width_vertical_keys = %d\n",
            x_offset_waveform, x_factor_waveform, x_width_waveform, x_width_vertical_keys);
    
    y_data_factor = swCalcDataFactor(window, wave, data_min, data_max, specgram_flag, &data_min, &data_max, &y_log_flag);
    swGetVScaleParams(window->config, window->direction, y_width_waveform, data_min, data_max, y_data_factor, min_vscale_div, y_log_flag,
                      &y_factor, &y_zero_offset, &y_scale_min_mod, &y_scale_max_mod,
                      &v_tick, &v_ntick, &v_n, &v_pn, &v_ndmin, &v_ndmax);
    y_factor_waveform = y_factor * y_data_factor;
    spDebug(80, "swDrawWaveImageCore", "y_factor_waveform = %f, y_factor = %f, y_data_factor = %f, y_zero_offset = %f\n",
            y_factor_waveform, y_factor, y_data_factor, y_zero_offset);
    spDebug(80, "swDrawWaveImageCore", "v_tick = %f, v_ntick = %f, v_n = %f, v_pn = %f, v_ndmin = %ld, v_ndmax = %ld\n",
            v_tick, v_ntick, v_n, v_pn, v_ndmin, v_ndmax);
    
    swGetYLabelString(y_label, window, wave, specgram_flag);
    spDebug(80, "swDrawWaveImageCore", "y_label = %s\n", y_label);

    swCalcDrawXMinMax(window, offset, length, &x_min, &x_max, &x_samp_dim_factor);
    spDebug(80, "swDrawWaveImageCore", "x_min = %f, x_max = %f, x_samp_dim_factor = %f\n",
            x_min, x_max, x_samp_dim_factor);
    
    swGetHScaleParams(window->config, x_width_waveform, x_min, x_max, x_samp_dim_factor, min_hscale_div, x_log_flag, for_print,
                      &x_factor, &x_zero_offset, &x_min, &x_max, &h_tick, &h_ntick, &h_n, &h_pn, &h_ndmin, &h_ndmax);
    spDebug(80, "swDrawWaveImageCore", "x_factor = %f, x_zero_offset = %f, x_min = %f, x_max = %f\n",
            x_factor, x_zero_offset, x_min, x_max);
    spDebug(80, "swDrawWaveImageCore", "h_tick = %f, h_ntick = %f, h_n = %f, h_pn = %f, h_ndmin = %ld, h_ndmax = %ld\n",
            h_tick, h_ntick, h_n, h_pn, h_ndmin, h_ndmax);

    if (draw_horizontal_keys_flag == SP_TRUE && (for_print == SP_TRUE || draw_offset == 0)) {
        int y_offset_piano;

        y_offset_piano = (int)spRound(y_draw_offset);
        if (window->config->horizontal_piano_keys_top == SP_TRUE) {
            y_offset_piano -= y_width_horizontal_keys;
        } else {
            y_offset_piano += wave->num_channel * y_width_waveform;
        }
        min_freq = x_min;
        max_freq = x_max;
        spDebug(30, "swDrawWaveImageCore", "draw horizontal keys: min_freq = %f, max_freq = %f, y_offset_piano = %d\n",
                min_freq, max_freq, y_offset_piano);
        swDrawPianoKeys(component, window, -1, x_offset_waveform, x_width_waveform, y_width_horizontal_keys,
                        y_offset_piano, y_offset_piano + y_width_horizontal_keys,
                        min_freq, max_freq, -1, -1, SP_TRUE, SP_FALSE/*SP_TRUE*/, x_log_flag);
    }
    
    for (n = 0; n < wave->num_channel; n++) {
        spDebug(80, "swDrawWaveImageCore", "n = %d / %d\n", n, wave->num_channel);
        swGetDrawRange(y_draw_offset, draw_height, n, &draw_min, &draw_max);
        spDebug(80, "swDrawWaveImageCore", "n = %d, draw_height = %f, draw_min = %d, draw_max = %d, draw_offset = %ld\n",
                n, draw_height, draw_min, draw_max, draw_offset);

        if (draw_pos > draw_offset) {
            swDrawWaveform(component, window, wave, n, offset, length,
                           x_offset_waveform, x_width_waveform, draw_min, draw_max, 
                           x_factor_waveform, y_factor_waveform, y_zero_offset,
                           draw_offset, draw_pos - draw_offset,
                           specgram_flag ? y_log_flag : x_log_flag, SP_FALSE, for_print);
        }
        if (for_print == SP_TRUE || draw_offset == 0) {
            if (draw_vertical_keys_flag == SP_TRUE) {
                min_freq = y_scale_min_mod;
                max_freq = y_scale_max_mod;
                spDebug(30, "swDrawWaveImageCore",
                        "draw vertical keys: min_freq = %f, max_freq = %f, x_offset_vertical_keys = %d\n",
                        min_freq, max_freq, x_offset_vertical_keys);
                swDrawPianoKeys(component, window, n, x_offset_vertical_keys, x_width_vertical_keys,
                                y_width_waveform, draw_min, draw_max, min_freq, max_freq, -1, -1,
                                SP_FALSE, window->config->vertical_piano_keys_right, y_log_flag);
            }
            swDrawHScaleCore(window->config, component, window->data_type, wave->num_channel, n,
                             x_offset_waveform, (int)spRound(y_draw_offset), x_width_waveform, x_width_vertical_keys,
                             y_width_waveform, y_width_horizontal_keys,
                             draw_min, draw_max, x_factor, x_zero_offset, h_tick, h_ntick, h_n, h_pn, h_ndmin, h_ndmax,
                             min_hscale_div, x_log_flag, for_print);
            
            swDrawVScaleCore(window->config, component, window->data_type, wave->num_channel, n,
                             x_offset_waveform, x_width_waveform, x_width_vertical_keys,
                             y_width_waveform, y_width_horizontal_keys, draw_min, draw_max,
                             y_data_factor, y_factor, y_zero_offset, 
                             v_tick, v_ntick, v_n, v_pn, v_ndmin, v_ndmax,
                             min_vscale_div, y_label, y_log_flag, for_print);
            spDebug(30, "swDrawWaveImageCore", "y_factor_waveform = %f, y_zero_offset = %f\n",
                    y_factor_waveform, y_zero_offset);
        }
    }
    
    spDebug(80, "swDrawWaveImageCore", "done\n");
    
    return;
}

#if 1
void swDrawSubAreaBackground(spComponent component, swWindow window, swWaveSubArea sub_area)
{
    if (window == NULL || sub_area == NULL)
        return;

    spDebug(40, "swDrawSubAreaBackground", "y_d = %f, height_d = %f\n",
            sub_area->y_d, sub_area->height_d);
    spFillRectangle(component, sw_graphics->gx_bg, 0, (int)spRound(sub_area->y_d),
                    window->width, (int)spRound(sub_area->height_d));

    return;
}

spLong swDrawWaveSubAreaImage(spComponent component, swWindow window, swWaveSubArea sub_area, spLong draw_pos,
                              double amp_min, double amp_max, double draw_height_horizontal_keys)
{
    spLong offset = 0, length = 0;
    spLong draw_offset;
    spLong wave_length;
    spLong update_length;
    double data_min, data_max;
    spBool online_draw = SP_FALSE;
    swWave wave;

    spDebug(40, "swDrawWaveSubAreaImage", "in: created = %d, draw_pos = %ld\n",
            spIsCreated(component), draw_pos);
    
    if (component == NULL || window == NULL || sub_area == NULL)
        return 0;

    if (sub_area->wave == NULL) {
        swDrawSubAreaBackground(component, window, sub_area);
        return 0;
    }

    wave = sub_area->wave;
    wave_length = wave->length;
        
    if (draw_pos < 0) {
        draw_offset = 0;
    } else {
        if (draw_pos > 0) online_draw = SP_TRUE;
            
        spDebug(40, "swDrawWaveSubAreaImage", "sub_area->drawn_pos = %ld, wave_length = %ld, wave->num_order = %ld\n",
                sub_area->drawn_pos, wave_length, wave->num_order);
        draw_offset = MAX(sub_area->drawn_pos, 0);
        if (draw_offset >= wave_length) {
            /* finished case */
            draw_offset = 0;
            draw_pos = wave_length;
        } else {
            if (wave->num_order <= 0 && swIsWaveRangeAvailable(wave) == SP_FALSE) {
                draw_offset = 0;
            } else if (draw_pos <= 0){
                draw_pos = wave_length/* - draw_offset*/;
            }
        }
    }
        
    spDebug(40, "swDrawWaveSubAreaImage", "draw_offset = %ld, draw_pos = %ld, amp_min = %f, amp_max = %f, wave_length = %ld\n",
            draw_offset, draw_pos, amp_min, amp_max, wave_length);

    if (draw_offset == 0) {
        swDrawSubAreaBackground(component, window, sub_area);

        offset = window->offset;
        length = window->length;
        spDebug(40, "swDrawWaveSubAreaImage", "offset = %ld, length = %ld\n", offset, length);
    }

    if (online_draw == SP_TRUE && swIsWaveSubAreaSpectrogram(sub_area) == SP_FALSE) {
        data_min = -swGetLimitValue(wave->samp_bit);
        data_max = swGetLimitValue(wave->samp_bit);
    } else {
        if (amp_max <= amp_min) {
            swGetDataHeight(window, wave, SP_FALSE, &data_min, &data_max);
        } else {
            data_min = amp_min;
            data_max = amp_max;
        }
    }
    
    spDebug(40, "swDrawWaveSubAreaImage", "data_min = %f, data_max = %f\n", data_min, data_max);
    spDebug(40, "swDrawWaveSubAreaImage", "y_d = %f, draw_width = %f, draw_height = %f\n",
            sub_area->y_d, window->draw_width, sub_area->draw_height);
        
    swDrawWaveImageCore(component, window, wave, offset, length,
                        sub_area->y_d, window->draw_width, sub_area->draw_height, draw_height_horizontal_keys,
                        data_min, data_max, draw_offset, draw_pos,
                        sub_area->specgram_flag, SP_FALSE);
    update_length = draw_pos - draw_offset;
        
    if (update_length > 0) {
        sub_area->drawn_pos = draw_pos;
        /*window->drawn_pos = draw_pos;*/
    }
    
    spDebug(30, "swDrawWaveSubAreaImage", "done: update_length = %ld\n", update_length);
    
    return update_length;
}

static void swGetSubAreaAmpMinMax(swWindow window, swWaveSubArea sub_area, double *o_amp_min, double *o_amp_max)
{
    if (sub_area->wave == window->wave || sub_area->wave == window->specgram) {
        if (sub_area->wave->num_order > 1) {
            swGetOrderMinMax(window, sub_area->wave, window->log_frequency_axis, SP_TRUE, o_amp_min, o_amp_max);
        } else {
            *o_amp_min = window->amp_min;
            *o_amp_max = window->amp_max;
        }
    } else {
        *o_amp_min = 0.0;
        *o_amp_max = -1.0;
    }
    
    return;
}

void swDrawWholeWaveImage(spComponent component, swWindow window)
{
    swWaveSubArea sub_area;
    double amp_min, amp_max;
    double draw_height_horizontal_keys;
    long k;
    long sub_area_count;
    spLong update_length;
    spLong ret;

    swFillBackground(component, window->width, window->height);
    spDebug(80, "swDrawWholeWaveImage", "swFillBackground done\n");

    if (window->wave != NULL) {
        if (window->config->toplevel->rubber_band_selection == SP_TRUE) {
            /* unselect overview region */
            spDebug(80, "swDrawWholeWaveImage", "call swReverseOverviewRegion\n");
            swReverseOverviewRegion(window, window->wave->selected_channel, 
                                    swSampToOverviewDisp(window, window->sel_st),
                                    swSampToOverviewDisp(window, window->sel_ed));
            
        }
    }
    
    update_length = 0;
    sub_area = swGetNextWaveSubArea(window, NULL);
    sub_area_count = 0;
    
    for (k = 1; sub_area != NULL; k++) {
        if (swIsWaveSubAreaVisible(sub_area) == SP_TRUE) {
            swGetSubAreaAmpMinMax(window, sub_area, &amp_min, &amp_max);
            spDebug(80, "swDrawWholeWaveImage", "amp_min = %f, amp_max = %f\n", amp_min, amp_max);

            if (window->config->horizontal_piano_keys_top == SP_TRUE) {
                draw_height_horizontal_keys = (sub_area_count == 0 ? window->draw_height_horizontal_keys : 0.0);
            } else {
                draw_height_horizontal_keys = (swIsLastWaveSubArea(window, sub_area) == SP_TRUE
                                               ? window->draw_height_horizontal_keys : 0.0);
            }
            
            ret = swDrawWaveSubAreaImage(component, window, sub_area, 0, amp_min, amp_max, draw_height_horizontal_keys);
            if (ret > update_length) {
                update_length = ret;
            }
            ++sub_area_count;
        }
        sub_area = swGetNextWaveSubArea(window, sub_area);
    }

#ifndef SW_DRAW_LABEL_CANVAS
    swDrawLabels(component, window);
#ifdef SW_SUPPORT_MORPHING
    swDrawAnchors(component, window);
#endif
#endif

    if (window->config->toplevel->rubber_band_selection == SP_TRUE) {
        /* select region */
        swRedrawRegion(component, window, SP_TRUE, SP_TRUE);
    }
    
    spDebug(80, "swDrawWholeWaveImage", "done\n");
    
    return;
}
#endif

void swDrawWaveImageCB(spComponent component, swWindow window)
{
    swWave wave;
    swWaveSubArea sub_area;
    spLong drawn_pos;
    
    if (window == NULL) return;

    spDebug(30, "swDrawWaveImageCB", "%s: in\n", window->name);
    
    wave = swGetTargetWave(window);
    sub_area = swGetWaveSubArea(window, wave);
    
    if (window->image == NULL) {
        spDebug(30, "swDrawWaveImageCB", "initial callback\n");
        window->image = component;
        if (wave != NULL) {
            /* drawing will be performed in the callback */
            swReadTotalWave(wave, SP_FALSE);
            spDebug(30, "swDrawWaveImageCB", "swReadTotalWave done\n");

            if (window->offset > 0 || swSampToTargetSamp(window, wave, window->length - 1) != wave->total_length - 1) {
                double start_s, end_s;
                spLong start, end;
                spDebug(30, "swDrawWaveImageCB", "window->offset = %ld, window->length = %ld\n",
                        window->offset, window->length);
                start_s = swSampToDim(window, window->offset);
                end_s = swSampToDim(window, window->offset + window->length - 1);
                start = swDimToTargetSamp(window, wave, start_s);
                end = swDimToTargetSamp(window, wave, end_s);
                spDebug(30, "swDrawWaveImageCB", "start_s = %f, start = %ld, end_s = %f, end = %ld\n",
                        start_s, start, end_s, end);
                
                /* reload wave to display different region */
                swReadWave(wave, SP_FALSE, /*swSampToTargetSamp(window, wave, window->offset)*/start,
                           /*swSampToTargetSamp(window, wave, window->length)*/end - start + 1);
                if (sub_area != NULL) {
                    sub_area->drawn_pos = 0;
                }
                /*window->drawn_pos = 0;*/
            }

            swDrawWholeWaveImage(window->image, window);
            spRefreshCanvas(window->overview_canvas);
        } else {
            swDrawBackground(window->image, window);
        }
    } else if (wave == NULL
               || swIsWaveRangeAvailable(wave) == SP_TRUE) {
        drawn_pos = 0;
        if (sub_area != NULL) {
            drawn_pos = sub_area->drawn_pos;
        }
        
        spDebug(30, "swDrawWaveImageCB", "drawn_pos = %ld\n", drawn_pos);
        if (drawn_pos <= 0
            || (wave != NULL && drawn_pos >= wave->length)) { /* if drawing has been completed */
            if (sub_area != NULL) {
                sub_area->drawn_pos = 0;
            }
            swDrawWholeWaveImage(component, window);
        }
    }

    spDebug(30, "swDrawWaveImageCB", "%s: done\n", window->name);
    
    return;
}

#ifdef SW_SUPPORT_PRINT
void swPrintWaveImagePage(spComponent component, swWindow window)
{
    spLong offset = 0, length = 0;
    double amp_min, amp_max;
    double data_min, data_max;
    double draw_width, draw_height;
    double draw_height_horizontal_keys;
    double y_draw_offset;
    swWave wave;
    
    wave = swGetTargetWave(window);

    if (wave->num_order > 1) {
        swGetOrderMinMax(window, wave, window->log_frequency_axis, SP_TRUE, &amp_min, &amp_max);
    } else {
        amp_min = window->amp_min;
        amp_max = window->amp_max;
    }
    spDebug(40, "swPrintWaveImagePage", "amp_min = %f, amp_max = %f\n", amp_min, amp_max);

    if (amp_max <= amp_min) {
        swGetDataHeight(window, wave, SP_FALSE, &data_min, &data_max);
    } else {
        data_min = amp_min;
        data_max = amp_max;
    }
    spDebug(40, "swPrintWaveImagePage", "data_min = %f, data_max = %f\n", data_min, data_max);

    offset = window->offset;
    length = window->length;
    spDebug(40, "swPrintWaveImagePage", "offset = %ld, length = %ld\n", offset, length);

    y_draw_offset = SW_PRINT_TOP_MARGIN;
    draw_width = (double)window->config->print_width;
    draw_height = (double)window->config->print_height;
    if (wave->num_order <= 1 && window->data_type == SW_FREQ_DATA
        && (window->config->draw_piano_keys_for_spectrum == SP_TRUE || window->draw_horizontal_keys == SP_TRUE)) {
        draw_height_horizontal_keys = /*SW_PIANO_KEYS_DEFAULT_SIZE*/SW_HORIZONTAL_PIANO_KEYS_DEFAULT_SIZE_FOR_PRINT;
        draw_height_horizontal_keys = MIN(draw_height_horizontal_keys,
                                          (double)draw_height * SW_PIANO_KEYS_MAX_SIZE_PERCENT / 100.0);
        draw_height -= draw_height_horizontal_keys;
    } else {
        draw_height_horizontal_keys = 0.0;
    }
    draw_height = draw_height / (double)window->wave->num_channel;
    draw_height = MAX(draw_height, 2);
    draw_height = MAX(draw_height, SW_MIN_DRAW_HEIGHT);
    if (draw_height_horizontal_keys > 0.0) {
        draw_height_horizontal_keys = MIN(draw_height_horizontal_keys,
                                          (double)window->config->print_height - draw_height * (double)window->wave->num_channel);
        draw_height_horizontal_keys = MAX(draw_height_horizontal_keys, 0.0);
        if (window->config->horizontal_piano_keys_top == SP_TRUE) {
            y_draw_offset += draw_height_horizontal_keys;
        }
    }

    spDebug(40, "swPrintWaveImagePage", "draw_width = %f, draw_height = %f, draw_height_horizontal_keys = %f\n",
            draw_width, draw_height, draw_height_horizontal_keys);

    swDrawWaveImageCore(component, window, wave, offset, length,
                        y_draw_offset, draw_width, draw_height, draw_height_horizontal_keys,
                        data_min, data_max, 0, wave->length,
                        spIsTrue(wave == window->specgram), SP_TRUE);
        
    return;
}

void swPluginCanvasCB(spComponent component, void *data)
{
    swWindow window;

    if ((window = spGetUserData(component)) != NULL) {
        swPrintWaveImagePage(component, window);
    }
    
    return;
}

spBool swPrintWaveImage(spComponent component, swWindow window)
{
    int width, height;
    double print_factor, print_x_factor, print_y_factor;
    
    if (component == NULL || window == NULL || window->wave == NULL || window->wave->length <= 1) {
        spDebug(40, "swPrintWaveImage", "no data\n");
        return SP_FALSE;
    }

    spSetUserData(component, window);

#if 0
    window->config->print_width = 400;
    window->config->print_height = 350;
#endif

    spSetScalingFactor(component, 1.0, 1.0);
    
    if (spGetClientSize(component, &width, &height) == SP_FALSE) {
        spDebug(40, "swPrintWaveImage", "spGetClientSize failed\n");
        return SP_FALSE;
    }
    
    spDebug(40, "swPrintWaveImage", "width = %d, height = %d\n", width, height);
    
    print_x_factor = (double)width / (double)(window->config->print_width + SW_PRINT_LEFT_MARGIN + SW_PRINT_RIGHT_MARGIN);
    print_y_factor = (double)height / (double)(window->config->print_height + SW_PRINT_TOP_MARGIN + SW_PRINT_BOTTOM_MARGIN);
    print_factor = MIN(print_x_factor, print_y_factor);
    spDebug(30, "swPrintWaveImage", "print_x_factor = %f, print_y_factor = %f\n",
            print_x_factor, print_y_factor);
    
    spSetScalingFactor(component, print_factor, print_factor);

    if (spStartDrawing(component, window->wave->core->orig_filename) == SP_TRUE) {
        spEndDrawing(component);
        spDebug(40, "swPrintWaveImage", "done\n");
        return SP_TRUE;
    } else {
        spDebug(40, "swPrintWaveImage", "spStartDrawing failed\n");
        return SP_FALSE;
    }
}
#endif

void swCopyImage(swWindow window)
{
    if (window == NULL) return;
    
    spDebug(100, "swCopyImage", "in\n");
    
    /* copy wave image */
    spCopyImage(window->image, window->canvas, 0, 0,
                window->width, window->height, 0, 0);
    
    spDebug(100, "swCopyImage", "done\n");
    
    return;
}

spBool swGetDrawTimeString(swWindow window, double point_f, char *string, spBool align_left)
{
    spLong point;
    char *format1;
    char format[SP_MAX_MESSAGE];
    static char format_left[] = "%-9";
    static char format_right[] = "%9";
    
    if (window == NULL || swIsNoWave(window) == SP_TRUE) {
        string[0] = NUL;
        return SP_FALSE;
    }
    
    if (align_left == SP_TRUE) {
        format1 = format_left;
    } else {
        format1 = format_right;
    }
    
    if (window->data_type == SW_FREQ_DATA) {
        if (window->config->freq_format == SW_FREQ_FORMAT_KHZ) {
            sprintf(format, "%s.6f", format1);
            sprintf(string, format, point_f / 1000.0);
        } else if (window->config->freq_format == SW_FREQ_FORMAT_POINT) {
            point = swDimToSamp(window, point_f);
            sprintf(format, "%sld", format1);
            sprintf(string, format, point);
        } else {
            sprintf(format, "%s.3f", format1);
            sprintf(string, format, point_f);
        }
    } else {
        if (window->config->time_format == SW_TIME_FORMAT_POINT) {
            point = swDimToSamp(window, point_f);
            sprintf(format, "%sld", format1);
            sprintf(string, format, point);
        } else {
            char buf[SP_MAX_MESSAGE];
            
            swGetTimeString(point_f, window->config->time_format, buf);
            sprintf(format, "%ss", format1);
            sprintf(string, format, buf);
        }
    }

    return SP_TRUE;
}

void swGetTimeStringTitle(swConfig config, swDataType data_type, char *title_string, char *dim_string)
{
    if (data_type == SW_FREQ_DATA) {
        strcpy(title_string, "Frequency");
        if (config->freq_format == SW_FREQ_FORMAT_KHZ) {
            strcpy(dim_string, "[kHz]");
        } else if (config->freq_format == SW_FREQ_FORMAT_POINT) {
            strcpy(dim_string, "[points]");
        } else {
            strcpy(dim_string, "[Hz]");
        }
    } else {
        strcpy(title_string, "Time");
        if (config->time_format == SW_TIME_FORMAT_MSEC
            || config->time_format == SW_TIME_FORMAT_FLOORED_MSEC) {
            strcpy(dim_string, "[ms]");
        } else if (config->time_format == SW_TIME_FORMAT_POINT) {
            strcpy(dim_string, "[points]");
        } else {
            strcpy(dim_string, "[s]");
        }
    }

    return;
}

void swGetAmplitudeString(swWave wave, double limit, double value, char *string, spBool percent_flag, spBool spacing_flag)
{
    if (percent_flag) {
        sprintf(string, spacing_flag ? "%8.3f" : "%.3f", 100.0 * value / limit);
    } else {
        if (swIsWaveFloat(wave) == SP_TRUE) {
            sprintf(string, spacing_flag ? "%10.4f" : "%.4f", value);
        } else if (swIsWaveLong(wave) == SP_TRUE) {
            sprintf(string, spacing_flag ? "%10.0f" : "%.0f", value);
        } else {
            sprintf(string, spacing_flag ? "%7.0f" : "%.0f", value);
        }
    }
    
    return;
}

void swGetAmplitudedBString(swWave wave, double limit, double value, char *string, spBool spacing_flag)
{
    double absvalue;
    
    absvalue = FABS(value) / limit;
    if (absvalue <= 1e-30) {
        sprintf(string, spacing_flag ? "%8s" : "%s", "-oo");
    } else {
        sprintf(string, spacing_flag ? "%8.3f" : "%.3f", dB(absvalue));
    }

    return;
}

#if 1
void swDrawCursorPianoKeys(spComponent component, swWindow window, swWave wave, spLong offset, spLong length,
                           double y_draw_offset, double draw_width, double draw_height, double draw_height_horizontal_keys,
                           double data_min, double data_max, int octave_index, int key, spBool specgram_flag)
{
    int n;
    int x_offset_waveform, x_offset_vertical_keys;
    int x_width_waveform, x_width_vertical_keys;
    int y_width_waveform, y_width_horizontal_keys;
    int draw_min, draw_max;
    double x_factor, x_factor_waveform;
    double x_zero_offset;
    double y_data_factor;
    double y_scale_min_mod, y_scale_max_mod;
    double x_min, x_max;
    double x_samp_dim_factor;
    double min_freq, max_freq;
    spBool x_log_flag, y_log_flag;
    spBool draw_vertical_keys_flag = SP_FALSE;
    spBool draw_horizontal_keys_flag = SP_FALSE;
    
    swCalcDrawBasicSizeParams(window, wave, offset, length,
                              draw_width, draw_height, draw_height_horizontal_keys, specgram_flag, SP_FALSE,
                              &x_offset_waveform, &x_offset_vertical_keys, &x_width_waveform, &x_width_vertical_keys,
                              &y_width_waveform, &y_width_horizontal_keys, &x_factor_waveform,
                              &x_log_flag, &y_log_flag, &draw_vertical_keys_flag, &draw_horizontal_keys_flag);

    if (draw_horizontal_keys_flag == SP_FALSE && draw_vertical_keys_flag == SP_FALSE) {
        return;
    }
    
    y_data_factor = swCalcDataFactor(window, wave, data_min, data_max, specgram_flag, &data_min, &data_max, &y_log_flag);
    swGetVScaleParams(window->config, window->direction, y_width_waveform, data_min, data_max, y_data_factor, SW_MIN_VSCALE_DIV, y_log_flag,
                      NULL, NULL, &y_scale_min_mod, &y_scale_max_mod,
                      NULL, NULL, NULL, NULL, NULL, NULL);

    swCalcDrawXMinMax(window, offset, length, &x_min, &x_max, &x_samp_dim_factor);
    swGetHScaleParams(window->config, x_width_waveform, x_min, x_max, x_samp_dim_factor, SW_MIN_HSCALE_DIV, x_log_flag, SP_FALSE,
                      &x_factor, &x_zero_offset, &x_min, &x_max, NULL, NULL, NULL, NULL, NULL, NULL);
    
    if (draw_horizontal_keys_flag == SP_TRUE) {
        int y_offset_piano;

        y_offset_piano = (int)spRound(y_draw_offset);
        if (window->config->horizontal_piano_keys_top == SP_TRUE) {
            y_offset_piano -= y_width_horizontal_keys;
        } else {
            y_offset_piano += wave->num_channel * y_width_waveform;
        }
        min_freq = x_min;
        max_freq = x_max;
            
        swDrawPianoKeys(window->canvas, window, -1, x_offset_waveform, x_width_waveform, y_width_horizontal_keys,
                        y_offset_piano, y_offset_piano + y_width_horizontal_keys,
                        min_freq, max_freq, octave_index, key, SP_TRUE, SP_FALSE/*SP_TRUE*/, x_log_flag);
    }
    if (draw_vertical_keys_flag == SP_TRUE) {
        for (n = 0; n < wave->num_channel; n++) {
            swGetDrawRange(y_draw_offset, draw_height, n, &draw_min, &draw_max);
            
            min_freq = y_scale_min_mod;
            max_freq = y_scale_max_mod;
            swDrawPianoKeys(window->canvas, window, n, x_offset_vertical_keys, x_width_vertical_keys,
                            y_width_waveform, draw_min, draw_max, min_freq, max_freq, octave_index, key,
                            SP_FALSE, window->config->vertical_piano_keys_right, y_log_flag);
        }
    }
    
    return;
}
#endif

void swDrawWaveSubAreaString(swWindow window, swWaveSubArea sub_area)
{
    spBool zero_flag = SP_FALSE;
    spBool accept_dB;
    int n;
    int sx, sy, swidth, sheight;
    int left_offset;
    spLong point;
    double top_offset;
    double value;
    double point_f;
    double limit;
    spLong data_offset;
    char string[SP_MAX_MESSAGE];
    char label[SP_MAX_MESSAGE];
    char buf[SP_MAX_MESSAGE];
    char amplitude_buf[SP_MAX_MESSAGE];
    char *name_str;
    char *unit_str;
    swWave wave;
    int octave_index;
    double cent;
    char notename[8];
    spBool fill_bg = SP_FALSE;
    spBool need_gx_string_color_reset = SP_FALSE;
    
    if (window == NULL || sub_area == NULL || sub_area->wave == NULL) return;

    octave_index = -1;
    notename[0] = NUL;
        
    wave = sub_area->wave;

    top_offset = SW_TIME_STRING_TOP_OFFSET;
    left_offset = SW_TIME_STRING_LEFT_OFFSET;
    if (top_offset < 0) {
        top_offset += /*window->height*/sub_area->height_d;
    }
    if (left_offset < 0) {
        /*left_offset += window->width;*/
        left_offset += swGetDrawWidth(window, SP_TRUE);
    } else if (window->draw_vertical_keys == SP_TRUE && window->config->vertical_piano_keys_right == SP_FALSE) {
        left_offset += (int)spRound(window->vertical_keys_width);
    }
    if (window->config->scale_flag == SP_TRUE) {
        top_offset = MIN(top_offset, /*window->height*/sub_area->height_d - SW_HSCALE_BOTTOM_SPACING);
        left_offset = MAX(left_offset, SW_HSCALE_LEFT_SPACING);
    }
    top_offset += sub_area->y_d;
    
    /*point = window->point;*/
    point = swSampToTargetSamp(window, wave, window->point);
    
    if (point < 0 || point >= wave->total_length) {
        point_f = 0.0;
        zero_flag = SP_TRUE;
    } else {
        point_f = window->point_f;
    }
    spDebug(100, "swDrawWaveSubAreaString", "wave->total_length = %ld, point = %ld, point_f = %f\n",
            (long)wave->total_length, (long)point, point_f);

    {
        char title_string[SP_MAX_LINE];
        char time_string[SP_MAX_LINE];
        char dim_string[SP_MAX_LINE];
        spBool use_order_flag = SP_FALSE;

        swGetDrawTimeString(window, point_f, time_string, SP_FALSE);
        swGetTimeStringTitle(window->config, window->data_type, title_string, dim_string);
        
        sprintf(string, "%s: %s %s", title_string, time_string, dim_string);
        spDebug(100, "swDrawWaveSubAreaString", "string = %s\n", string);

        if (wave->num_order > 1 || window->data_type == SW_FREQ_DATA) {
            double freq;
            
            if (wave->num_order > 1 && swIsWaveSubAreaSpectrogram(sub_area) == SP_TRUE) {
#if 0
                if (window->config->specgram_gray_scale == SP_FALSE) {
                    spSetGraphicsParams(sw_graphics->gx_string,
                                        SppForeground, "white"/*"grey50"*/,
                                        NULL);
                    need_gx_string_color_reset = SP_TRUE;
                }
#else
                if (window->config->specgram_gray_scale == SP_FALSE) {
                    fill_bg = SP_TRUE;
                }
#endif
                
                if (wave->custom_x_axis != NODATA) {
                    long custom_order;
                    custom_order = MAX(window->target_order, 0);
                    custom_order = MIN(window->target_order, wave->custom_x_axis->length - 1);
                    freq = wave->custom_x_axis->data[custom_order];
                } else {
                    freq = (window->wave->samp_rate / 2.0) * (double)window->target_order
                        / (double)(wave->num_order - 1);
                }
            } else if (!(wave->num_order > 1) && window->data_type == SW_FREQ_DATA) {
                freq = point_f;
            } else {
                freq = -1.0;
                use_order_flag = SP_TRUE;
            }

            if (use_order_flag == SP_TRUE) {
                if (wave->num_order > 1) {
                    sprintf(buf, "    Order: %4ld", window->target_order);
                } else {
                    buf[0] = NUL;
                }
            } else {
                if (freq <= 0.0 || freq > window->wave->samp_rate / 2.0) {
                    if (wave->num_order > 1) {
                        sprintf(buf, "    Frequency: %9.2f [Hz]", freq);
                    } else {
                        buf[0] = NUL;
                    }
                } else {
                    spFreqToNoteName(freq, window->config->mid_C_octave_index, notename, &octave_index, &cent);
                    
                    if (wave->num_order > 1) {
                        sprintf(buf, "    Frequency: %9.2f [Hz] (%2s%d %+-5.1f [cent])", freq,
                                notename, octave_index, cent);
                    } else {
                        sprintf(buf, " (%2s%d %+-5.1f [cent])", notename, octave_index, cent);
                    }
                }
            }
            strcat(string, buf);
        }

        if (fill_bg == SP_TRUE) {
            if (spGetStringExtent(window->canvas, sw_graphics->gx_string, string, &sx, &sy, &swidth, &sheight, NULL) == SP_TRUE) {
                spFillRectangle(window->canvas, sw_graphics->gx_bg, left_offset + sx, (int)top_offset + sy, swidth, sheight);
                //spDrawRectangle(window->canvas, sw_graphics->gx_fg, left_offset + sx, (int)top_offset + sy, swidth, sheight);
            }
        }
        
        /* draw time string */
        spDebug(100, "swDrawWaveSubAreaString", "time string = %s\n", string);
        spDrawString(window->canvas, sw_graphics->gx_string,
                     left_offset, (int)top_offset, string);

        if (swGetTargetWave(window) == wave) {
            if (window->config->draw_selection_times == SP_TRUE) {
                char end_time_string[SP_MAX_LINE];

                if (window->sel_st >= 0 && window->sel_ed >= 0) {
                    double st_f, ed_f;

                    st_f = swSampToDim(window, window->sel_st);
                    ed_f = swSampToDim(window, window->sel_ed);
                
                    swGetDrawTimeString(window, st_f, time_string, SP_FALSE);
                    swGetDrawTimeString(window, ed_f, end_time_string, SP_TRUE);
                } else {
                    swGetDrawTimeString(window, 0.0, time_string, SP_FALSE);
                    swGetDrawTimeString(window, 0.0, end_time_string, SP_TRUE);
                }
            
                sprintf(string, "Selection: %s - %s %s", time_string, end_time_string, dim_string);
            
                if (spGetStringExtent(window->canvas, sw_graphics->gx_string, string, &sx, &sy, &swidth, &sheight, NULL) == SP_TRUE) {
                    top_offset -= (-sy + 2);
                } else {
                    top_offset -= SW_TIME_STRING_HEIGHT;
                }

                if (fill_bg == SP_TRUE) {
                    if (spGetStringExtent(window->canvas, sw_graphics->gx_string, string, &sx, &sy, &swidth, &sheight, NULL) == SP_TRUE) {
                        spFillRectangle(window->canvas, sw_graphics->gx_bg, left_offset + sx, (int)top_offset + sy, swidth, sheight);
                        //spDrawRectangle(window->canvas, sw_graphics->gx_fg, left_offset + sx, (int)top_offset + sy, swidth, sheight);
                    }
                }
                
                /* draw selection string */
                spDebug(100, "swDrawWaveSubAreaString", "selection string = %s\n", string);
                spDrawString(window->canvas, sw_graphics->gx_string,
                             left_offset, (int)top_offset, string);
            }

            if (window->config->draw_selection_length == SP_TRUE) {
                spLong len = 0;
                double len_f = 0.0;

                if (window->sel_st >= 0 && window->sel_ed >= 0) {
                    if (window->sel_st < window->sel_ed) {
                        len = window->sel_ed - window->sel_st;
                    } else {
                        len = window->sel_st - window->sel_ed;
                    }
                    len_f = swSampToDim(window, len);
                
                    swGetDrawTimeString(window, len_f, time_string, SP_FALSE);
                } else {
                    swGetDrawTimeString(window, 0.0, time_string, SP_FALSE);
                }
            
                if (window->data_type == SW_FREQ_DATA) {
                    sprintf(string, "Length: %s %s", time_string, dim_string);
                } else {
                    char freq_string[SP_MAX_LINE];

                    if (len_f > 0.0) {
                        sprintf(freq_string, "%.4f", 1.0/len_f);
                    } else {
                        sprintf(freq_string, "oo");
                    }
                    sprintf(string, "Length: %s %s (%9s [Hz])", time_string, dim_string, freq_string);
                }
            
                if (spGetStringExtent(window->canvas, sw_graphics->gx_string, string, &sx, &sy, &swidth, &sheight, NULL) == SP_TRUE) {
                    top_offset -= (-sy + 2);
                } else {
                    top_offset -= SW_TIME_STRING_HEIGHT;
                }

                if (fill_bg == SP_TRUE) {
                    if (spGetStringExtent(window->canvas, sw_graphics->gx_string, string, &sx, &sy, &swidth, &sheight, NULL) == SP_TRUE) {
                        spFillRectangle(window->canvas, sw_graphics->gx_bg, left_offset + sx, (int)top_offset + sy, swidth, sheight);
                        //spDrawRectangle(window->canvas, sw_graphics->gx_fg, left_offset + sx, (int)top_offset + sy, swidth, sheight);
                    }
                }
                
                /* draw length string */
                spDebug(100, "swDrawWaveSubAreaString", "length string = %s\n", string);
                spDrawString(window->canvas, sw_graphics->gx_string,
                             left_offset, (int)top_offset, string);
            }
        }

#if 1
        if (octave_index >= 0) {
            double amp_min, amp_max;
            int key;
            key = spGetKeyIndex(notename);
            swGetSubAreaAmpMinMax(window, sub_area, &amp_min, &amp_max);
            spDebug(80, "swDrawWaveSubAreaString", "key = %d, octave_index = %d, amp_min = %f, amp_max = %f\n",
                    key, octave_index, amp_min, amp_max);
            
            swDrawCursorPianoKeys(window->canvas, window, wave, window->offset, window->length,
                                  sub_area->y_d, window->draw_width, sub_area->draw_height,
                                  window->draw_height_horizontal_keys,
                                  amp_min, amp_max, octave_index, key,
                                  sub_area->specgram_flag);
        }
#endif
    }

#ifdef SW_SUPPORT_METER
    spDebug(40, "swDrawWaveSubAreaString", "window->meter_width = %d\n",window-> meter_width);
    if (swGetTargetWave(window) == wave && swIsMeterVisible(window) == SP_TRUE) {
        int meter_left_offset;

        meter_left_offset = (int)window->draw_width;
        spDebug(40, "swDrawWaveSubAreaString", "draw meter, meter_left_offset = %d\n", meter_left_offset);
        
        for (n = 0; n < wave->num_channel; n++) {
            if (swGetPeakData(wave, n, window->target_order, window->point, &value) == SP_FALSE) {
                value = 0.0;
            }
            swDrawMeter(window->canvas, window, wave, /*sub_area->y_d*/0.0,
                        (double)window->height / (double)MAX(wave->num_channel, 1),
                        meter_left_offset, n, value);
        }
    } else {
        spDebug(40, "swDrawWaveSubAreaString", "draw meter, not target wave\n");
    }
#endif
    
    if (!(zero_flag == SP_TRUE || swIsWaveProcessing(wave) == SP_TRUE
          /*|| wave->num_order > 1*/)) {
        /*data_offset = swSampToTargetSamp(window, wave, window->point - window->offset) / wave->thin_length;*/
        data_offset = (swSampToTargetSamp(window, wave, window->point) - swGetWaveOffset(wave)) / wave->thin_length;
        spDebug(40, "swDrawWaveSubAreaString", "data_offset = %ld\n", data_offset);
    
        top_offset = SW_AMP_STRING_TOP_OFFSET;
        left_offset = SW_AMP_STRING_LEFT_OFFSET;
        if (top_offset < 0) {
            top_offset += sub_area->height_d;
        }
        if (left_offset < 0) {
            left_offset += swGetDrawWidth(window, SP_TRUE);
        } else if (window->draw_vertical_keys == SP_TRUE && window->config->vertical_piano_keys_right == SP_FALSE) {
            left_offset += (int)spRound(window->vertical_keys_width);
        }
        if (window->config->scale_flag == SP_TRUE) {
            top_offset = MIN(top_offset, /*window->height*/sub_area->height_d - SW_HSCALE_BOTTOM_SPACING);
            left_offset = MAX(left_offset, SW_HSCALE_LEFT_SPACING);
        }
        top_offset += sub_area->y_d;

        spDebug(40, "swDrawWaveSubAreaString", "data_offset = %ld, point = %ld, offset = %ld, target_order = %ld\n",
                data_offset, window->point, window->offset, window->target_order);
        spDebug(40, "swDrawWaveSubAreaString", "length = %ld, total_length = %ld, num_channel = %d\n",
                wave->length, wave->total_length, wave->num_channel);

        limit = swGetWindowLimitValue(window, wave);

        name_str = swGetAnalysisNameString(wave);
        unit_str = swGetAnalysisUnitString(wave, SP_TRUE);
    
        if (!strnone(name_str)) {
            strcpy(label, name_str);
        } else {
            strcpy(label, "Amplitude");
        }
    
        if (wave->num_order <= 1 && window->data_type != SW_FREQ_DATA && strnone(unit_str)) {
            accept_dB = SP_TRUE;
        } else {
            accept_dB = SP_FALSE;
        }
        spDebug(40, "swDrawWaveSubAreaString", "accept_dB = %d, num_order = %ld, unit_str = %s\n",
                accept_dB, wave->num_order, unit_str);
    
        for (n = 0; n < wave->num_channel; n++) {
            if (swGetWaveData(wave, n, window->target_order, data_offset, &value) == SP_FALSE) {
                break;
            }

            swGetAmplitudeString(wave, limit, value, amplitude_buf,
                                 accept_dB == SP_TRUE && window->config->percent_amplitude == SP_TRUE, SP_TRUE);

            if (accept_dB == SP_TRUE && window->config->percent_amplitude == SP_TRUE) {
                sprintf(string, "%s: %s %%", label, amplitude_buf);
            } else {
                sprintf(string, "%s:%s", label, amplitude_buf);

                if (!strnone(unit_str)) {
                    strcat(string, " ");
                    strcat(string, unit_str);
                }
            }

            if (accept_dB == SP_TRUE) {
                swGetAmplitudedBString(wave, limit, value, amplitude_buf, SP_TRUE);
                sprintf(buf, " (%s dB)", amplitude_buf);
                strcat(string, buf);
            }

            spDebug(50, "swDrawWaveSubAreaString", "string = %s\n", string);

            if (fill_bg == SP_TRUE) {
                if (spGetStringExtent(window->canvas, sw_graphics->gx_string, string, &sx, &sy, &swidth, &sheight, NULL) == SP_TRUE) {
                    spFillRectangle(window->canvas, sw_graphics->gx_bg, left_offset + sx, (int)top_offset + sy, swidth, sheight);
                    //spDrawRectangle(window->canvas, sw_graphics->gx_fg, left_offset + sx, (int)top_offset + sy, swidth, sheight);
                }
            }
                
            /* draw amplitude string */
            spDrawString(window->canvas, sw_graphics->gx_string,
                         left_offset, (int)top_offset, string);

            top_offset += sub_area->draw_height;
        }
    }
    
    if (need_gx_string_color_reset == SP_TRUE) {
        spSetGraphicsParams(sw_graphics->gx_string,
                            SppForeground, window->config->string_color,
                            NULL);
    }
    
    return;
}

void swDrawString(swWindow window)
{
    swWaveSubArea sub_area;
    
    sub_area = swGetNextWaveSubArea(window, NULL);
    
    while (sub_area != NULL) {
        if (swIsWaveSubAreaVisible(sub_area) == SP_TRUE) {
            swDrawWaveSubAreaString(window, sub_area);
        }
        sub_area = swGetNextWaveSubArea(window, sub_area);
    }
    
    return;
}

static int sw_label_string_px = 0;
static int sw_label_string_py = 0;
static int sw_label_string_prev_channel = -1;

static int sw_region_label_px = 0;
static int sw_region_label_height = 0;
static int sw_region_label_prev_channel = -1;

#define SW_LABEL_STRING_OFFSET 5
/*#define SW_LABEL_STRING_TOP_OFFSET 25*/
#define SW_LABEL_STRING_TOP_OFFSET 10
#define SW_REGION_LABEL_OFFSET 8
#define SW_REGION_LABEL_BOTTOM_OFFSET 23

#define SW_ACTIVE_LABEL_RECT_SIZE 6

void swDrawChannelLabel(spComponent component, int draw_width, int window_height, int x_offset, int num_channel,
                        int st_d, int ed_d, int channel, char *string, spBool active_flag)
{
    int cx, cy;
    int x, y;
    int width, height;
    int y_offset;
    int label_height;
    double region_height;
    spGraphics graphics;
    
    spLockMutex(sw_graphics->mutex);

    if (channel < 0) {
        y_offset = 0;
        label_height = window_height;
    } else {
        y_offset = (int)spRound((double)channel * (double)window_height / (double)num_channel);
        label_height = (int)spRound((double)window_height / (double)num_channel);
    }
    
    if (ed_d >= 0) {
        graphics = sw_graphics->gx_region;

        spSetGraphicsParams(graphics,
                            SppForeground, sw_graphics->region_line_color,
                            NULL);

        /* draw arrow */
        if (st_d >= 0 && sw_region_label_px > st_d && sw_region_label_prev_channel == channel) {
            region_height = sw_region_label_height + SW_REGION_LABEL_OFFSET;
            if (region_height > label_height) {
                region_height = SW_REGION_LABEL_BOTTOM_OFFSET;
            }
        } else {
            region_height = SW_REGION_LABEL_BOTTOM_OFFSET;
        }
            
        y = y_offset + label_height - (int)region_height;
        
        if (ed_d <= draw_width) {
            if (channel >= 0) {
                spSetGraphicsParams(graphics, SppLineType, SP_LINE_DASH, NULL);
                spDrawLine(component, graphics, ed_d + x_offset, 0, ed_d + x_offset, window_height);
                spSetGraphicsParams(graphics, SppLineType, SP_LINE_SOLID, NULL);
            }
            spDrawLine(component, graphics, ed_d + x_offset, y_offset, ed_d + x_offset, y_offset + label_height);
            
            if (active_flag == SP_TRUE) {
                spFillRectangle(component, graphics, ed_d + x_offset - SW_ACTIVE_LABEL_RECT_SIZE/2, y_offset,
                                SW_ACTIVE_LABEL_RECT_SIZE, SW_ACTIVE_LABEL_RECT_SIZE);
                spFillRectangle(component, graphics, ed_d + x_offset - SW_ACTIVE_LABEL_RECT_SIZE/2,
                                y_offset + label_height - SW_ACTIVE_LABEL_RECT_SIZE,
                                SW_ACTIVE_LABEL_RECT_SIZE, SW_ACTIVE_LABEL_RECT_SIZE);
            }
            
            spSetGraphicsParams(graphics,
                                SppForeground, sw_graphics->region_label_color,
                                NULL);

            /* arrow's head */
            spDrawLine(component, graphics, ed_d + x_offset, y, ed_d + x_offset - 9, y - 3);
            spDrawLine(component, graphics, ed_d + x_offset, y, ed_d + x_offset - 9, y + 3);
            spDrawLine(component, graphics, MAX(st_d, 0) + x_offset, y, ed_d + x_offset, y);

            sw_region_label_px = MAX(ed_d, sw_region_label_px);
            sw_region_label_height = (int)region_height;
            sw_region_label_prev_channel = channel;
        } else {
            spSetGraphicsParams(graphics,
                                SppForeground, sw_graphics->region_label_color,
                                NULL);

            if (st_d <= draw_width) {
                /* arrow's body */
                spDrawLine(component, graphics, MAX(st_d, 0) + x_offset, y, MIN(ed_d, draw_width) + x_offset, y);
            }
        }
    } else {
        graphics = sw_graphics->gx_label;
    }
    
    if (st_d >= 0 && st_d <= draw_width) {
        if (channel >= 0) {
            spSetGraphicsParams(graphics, SppLineType, SP_LINE_DASH, NULL);
            spDrawLine(component, graphics, st_d + x_offset, 0, st_d + x_offset, window_height);
            spSetGraphicsParams(graphics, SppLineType, SP_LINE_SOLID, NULL);
        }
        spDrawLine(component, graphics, st_d + x_offset, y_offset, st_d + x_offset, y_offset + label_height);
        
        if (active_flag == SP_TRUE) {
            spFillRectangle(component, graphics, st_d + x_offset - SW_ACTIVE_LABEL_RECT_SIZE/2, y_offset,
                            SW_ACTIVE_LABEL_RECT_SIZE, SW_ACTIVE_LABEL_RECT_SIZE);
            spFillRectangle(component, graphics, st_d + x_offset - SW_ACTIVE_LABEL_RECT_SIZE/2,
                            y_offset + label_height - SW_ACTIVE_LABEL_RECT_SIZE,
                            SW_ACTIVE_LABEL_RECT_SIZE, SW_ACTIVE_LABEL_RECT_SIZE);
        }
        
        if (!strnone(string)) {
            spGetStringExtent(component, graphics, string, &x, &y, &width, &height, NULL);
            
            cx = st_d;
            if (sw_label_string_px >= cx && sw_label_string_prev_channel == channel) {
                cy = y_offset + sw_label_string_py + height + SW_LABEL_STRING_OFFSET;
                if (cy > label_height) {
                    cy = y_offset + height + SW_LABEL_STRING_TOP_OFFSET;
                }
            } else {
                cy = y_offset + height + SW_LABEL_STRING_TOP_OFFSET;
            }
            
            sw_label_string_px = cx + width;
            sw_label_string_py = cy;
            sw_label_string_prev_channel = channel;
            
            spDrawLine(component, graphics, cx + x_offset, cy, sw_label_string_px, cy);
            spDrawString(component, graphics, st_d + x_offset, cy - 1, string);
        }
    }

    if (ed_d >= 0) {
        spSetGraphicsParams(sw_graphics->gx_region,
                            SppForeground, sw_graphics->region_color,
                            NULL);
    }
    
    spUnlockMutex(sw_graphics->mutex);
    
    return;
}

void swDrawLabel(spComponent component, int draw_width, int window_height, int x_offset, 
                 int st_d, int ed_d, char *string, spBool active_flag)
{
    swDrawChannelLabel(component, draw_width, window_height, x_offset, 1,
                       st_d, ed_d, -1, string, active_flag);
    
    return;
}

void swDrawLabels(spComponent component, swWindow window)
{
    long k;
    int st_d, ed_d;
    int x_offset;
    int draw_width;
    spBool active_flag;

    if (swIsNoLabel(window) == SP_TRUE
        || window->draw_label == SP_FALSE || component == NULL)
        return;

    spDebug(80, "swDrawLabels", "in\n");
    
    sw_label_string_px = 0;
    sw_label_string_py = SW_LABEL_STRING_TOP_OFFSET;
    sw_label_string_prev_channel = -1;
    sw_region_label_px = 0;
    sw_region_label_height = SW_REGION_LABEL_BOTTOM_OFFSET;
    sw_region_label_prev_channel = -1;

    draw_width = swGetDrawWidth(window, SP_TRUE);
    
    if (window->draw_vertical_keys == SP_TRUE && window->config->vertical_piano_keys_right == SP_FALSE) {
        x_offset = (int)spRound(window->vertical_keys_width);
    } else {
        x_offset = 0;
    }
    
    for (k = 0; k < window->wave->labels->num_buffer; k++) {
        if (window->wave->labels->label[k].time >= 0.0) {
            st_d = swDimToDisp(window, window->wave->labels->label[k].time);

            if (k == window->active_label_index) {
                active_flag = SP_TRUE;
            } else {
                active_flag = SP_FALSE;
            }

            if (window->wave->labels->label[k].end_time >= 0.0) {
                ed_d = swDimToDisp(window, window->wave->labels->label[k].end_time);
            } else {
                ed_d = -1;
            }
            swDrawChannelLabel(component, draw_width, window->height, x_offset, window->wave->num_channel,
                               st_d, ed_d, window->wave->labels->label[k].channel,
                               window->wave->labels->label[k].string, active_flag);
        }
    }

    spDebug(80, "swDrawLabels", "done\n");
    
    return;
}

void swRedrawLabels(swWindow window)
{
#ifndef SW_DRAW_LABEL_CANVAS
    swRedrawWave(window);
#else
    swDrawCursor(window, SP_TRUE);
#endif
    
    return;
}

void swUpdateLabels(swWindow window)
{
    if (swIsNoWave(window) == SP_TRUE) return;

    if (swIsLabelNone(window->wave) == SP_TRUE) {
        window->active_label_index = -1;
    } else {
        if (window->drag_label_type == SW_DRAG_NO_LABEL) {
            swSortLabels(window->wave->labels, &window->active_label_index);
            spDebug(50, "swUpdateLabels", "sorted active label %ld\n", window->active_label_index);
        }
        
        if (window->active_label_index >= 0
            && window->wave->labels->label[window->active_label_index].time < 0.0) {
            window->active_label_index = -1;
        }
    }
    
    swRedrawLabels(window);
    swUpdateLabelList(window->label_list);

#if defined(SW_AH_CUSTOM)
    if (window->link_ah_file) {
        swLabelsToAHFile(window->wave, window->config->toplevel->ahfile);
        swDrawAHSessionCanvas(window->config->toplevel->ahwindow, SP_TRUE, SP_TRUE);
    }
#endif
    
    return;
}

void swUnselectActiveLabel(swWindow window)
{
    if (swIsNoWave(window) == SP_TRUE) {
        return;
    }
    
    if (window->active_label_index >= 0) {
        window->active_label_index = -1;
        swRedrawLabels(window);
    }

    return;
}

void swResetRegion(swWindow window)
{
    if (window == NULL) return;

    window->sel_st = -1;
    window->sel_ed = -1;
    window->sel_st_d = -1;
    window->sel_ed_d = -1;
        
    swSetSelectSenseLevel(window, SP_FALSE);
    
    return;
}

void swUpdateDisplayRegion(swWindow window)
{
    if (window->sel_st == -1 || window->sel_ed == -1) {
        window->sel_st_d = -1;
        window->sel_ed_d = -1;
    } else {
        spDebug(100, "swUpdateDisplayRegion", "window->sel_st = %ld, window->sel_ed = %ld, window->sel_st_d = %d, window->sel_ed_d = %d\n",
                window->sel_st, window->sel_ed, window->sel_st_d, window->sel_ed_d);
        window->sel_st_d = swSampToDisp(window, window->sel_st);
        window->sel_ed_d = swSampToDisp(window, window->sel_ed);
        spDebug(100, "swUpdateDisplayRegion", "updated: window->sel_st_d = %d, window->sel_ed_d = %d\n",
                window->sel_st_d, window->sel_ed_d);
    
        if (window->sel_st != window->sel_ed && 
            window->sel_st_d == window->sel_ed_d) {
            window->sel_ed_d += 1;
        }
    }
    
    return;
}

void swSelectRegion(swWindow window, int channel, spLong start, spLong end)
{
    if (window == NULL || window->wave == NULL) return;

    spDebug(50, "swSelectRegion", "in\n");
    
    swLockWindowMutex(window);

    if (window->config->toplevel->rubber_band_selection == SP_TRUE) {
        swReverseRegion(window, window->wave->selected_channel, window->sel_st_d, window->sel_ed_d,
                        SP_TRUE, SP_TRUE);
        spDebug(50, "swSelectRegion", "reverse done\n");
    }

    if ((start < 0 && end < 0) || 
        (start >= window->wave->total_length && end >= window->wave->total_length)) {
        swResetRegion(window);
    } else {
        if (start < end) {
            window->sel_st = MAX(start, 0);
            window->sel_ed = MIN(end, window->wave->total_length - 1);
        } else {
            window->sel_st = MAX(end, 0);
            window->sel_ed = MIN(start, window->wave->total_length - 1);
        }
        
        if (window->sel_st == window->sel_ed) {
            swResetRegion(window);
        } else {
            swUpdateDisplayRegion(window);
            
            window->wave->selected_channel = channel;

            swUpdateInfoAreaSelection(window);
            
            /* redraw region */
            swReverseRegion(window, window->wave->selected_channel, window->sel_st_d, window->sel_ed_d,
                            SP_TRUE, SP_TRUE);

            swSetSelectSenseLevel(window, SP_TRUE);
        }
    }

    /* draw cursor */
    swDrawCursor(window, SP_TRUE);
    /*spRefreshCanvas(window->overview_canvas);*/
    
    swUnlockWindowMutex(window);
    
    spDebug(50, "swSelectRegion", "done\n");
    
    return;
}

void swSelectRegionCB(spComponent component, swWindow window)
{
    spLong start, end;

    if (window == NULL || window->wave == NULL) return;
    
    start = window->offset;
    end = start + window->length - 1;
    
    if (start == window->sel_st && end == window->sel_ed) {
        start = -1;
        end = -1;
    }
    
    swSelectRegion(window, -1, start, end);

    return;
}

void swSelectFromHereCB(spComponent component, swWindow window)
{
    int channel;
    spLong start, end;

    if (window == NULL || window->wave == NULL) return;
    
    start = window->point;
    if (window->sel_ed > start) {
        end = window->sel_ed;
        channel = window->wave->selected_channel;
    } else {
        end = window->wave->total_length - 1;
        channel = -1;
    }
    
    swSelectRegion(window, channel, start, end);

    return;
}

void swSelectToHereCB(spComponent component, swWindow window)
{
    int channel;
    spLong start, end;

    if (window == NULL || window->wave == NULL) return;
    
    end = window->point;
    if (window->sel_st >= 0 && window->sel_st < end) {
        start = window->sel_st;
        channel = window->wave->selected_channel;
    } else {
        start = 0;
        channel = -1;
    }
    
    swSelectRegion(window, channel, start, end);

    return;
}

/*
 *        select region of other window
 */
void swSelectAllRegionCB(spComponent component, swWindow window)
{
    spLong k;
    spLong sel_st, sel_ed;
    double sel_st_f, sel_ed_f;
    spComponent next = NULL;
    swWindow next_window = NULL;

    if (window == NULL || window->wave == NULL
        || (window->sel_st < 0 && window->sel_ed < 0))        return;

    sel_st_f = swSampToDim(window, window->sel_st);
    sel_ed_f = swSampToDim(window, window->sel_ed);

    /* draw cursor */
    swDrawCursor(window, SP_TRUE);

    next = window->window;
    for (k = 0;; k++) {
        next = spGetNextWindow(next, SP_FALSE);
        if (next == NULL || next == window->window) {
            break;
        }
        
        if ((next_window = (swWindow)spGetUserData(next)) != NULL
            && next_window->wave != NULL) {
            if (next_window->data_type == window->data_type
                && swIsWaveProcessing(next_window->wave) == SP_FALSE) {
                sel_st = swDimToSamp(next_window, sel_st_f);
                sel_ed = swDimToSamp(next_window, sel_ed_f);

                /* select region */
                swSelectRegion(next_window, -1, sel_st, sel_ed);
            }
        }
    }

    return;
}

void swSelectAllCB(spComponent component, swWindow window)
{
    spLong start, end;

    if (window == NULL || window->wave == NULL) return;
    
    start = 0;
    end = window->wave->total_length - 1;
    
    swSelectRegion(window, -1, start, end);

    return;
}

void swSelectNextChannelCB(spComponent component, swWindow window)
{
    int channel;

    if (window == NULL || window->wave == NULL) return;

    
    if (window->wave->selected_channel >= window->wave->num_channel - 1) {
        channel = -1;
    } else if (window->wave->selected_channel < 0) {
        channel = 0;
    } else {
        channel = window->wave->selected_channel + 1;
    }
    
    swSelectRegion(window, channel, window->sel_st, window->sel_ed);

    return;
}

void swReverseMainRegion(spComponent component, swWindow window, int channel, int start_d, int end_d)
{
    swWaveSubArea sub_area;
    double draw_width;
    
    if (window == NULL || component == NULL || (start_d < 0 && end_d < 0)) return;

    draw_width = (double)swGetDrawWidth(window, SP_TRUE);
    spDebug(80, "swReverseMainRegion", "draw_width = %f, start_d = %d, end_d = %d\n",
            draw_width, start_d, end_d);
    
    sub_area = swGetNextWaveSubArea(window, NULL);
                    
    while (sub_area != NULL) {
        if (swIsWaveSubAreaVisible(sub_area) == SP_TRUE) {
            swFillRegion(component, sw_graphics->gx_xor, window,
                         sub_area->y_d, draw_width, sub_area->draw_height, channel, start_d, end_d, SP_FALSE);
        }
        sub_area = swGetNextWaveSubArea(window, sub_area);
    }
    
    return;
}

void swReverseRegionForComponent(spComponent component, swWindow window, int channel,
                                 int start_d, int end_d, spBool main_flag, spBool overview_flag)
{
    if (window == NULL) return;

    spDebug(80, "swReverseRegionForComponent", "in: start_d = %d, end_d = %d\n", start_d, end_d);
    
    if (main_flag == SP_TRUE) {
        swReverseMainRegion(component, window, channel, start_d, end_d);
    }

    if (overview_flag == SP_TRUE && window->overview_canvas != NULL) {
        spDebug(80, "swReverseRegionForComponent", "draw overview region\n");
        
        if (window->config->toplevel->rubber_band_selection == SP_TRUE) {
            spLong start, end;
            int overview_st_d, overview_ed_d;
    
            start = swDispToSamp(window, start_d);
            end = swDispToSamp(window, end_d);
            overview_st_d = swSampToOverviewDisp(window, start);
            overview_ed_d = swSampToOverviewDisp(window, end);
            spDebug(80, "swReverseRegionForComponent", "overview: start = %ld, end = %ld, overview_st_d = %d, overview_ed_d = %d\n",
                    start, end, overview_st_d, overview_ed_d);
            swReverseOverviewRegion(window, channel, overview_st_d, overview_ed_d);
        } else {
            swDrawOverview(window, SP_FALSE);
        }
    }
    
    spDebug(80, "swReverseRegionForComponent", "done\n");
    
    return;
}

void swReverseRegion(swWindow window, int channel,
                     int start_d, int end_d, spBool main_flag, spBool overview_flag)
{
    spComponent component;
    
    if (window == NULL) return;

    spDebug(80, "swReverseRegion", "in: start_d = %d, end_d = %d\n", start_d, end_d);
    
    if (window->config->toplevel->rubber_band_selection == SP_TRUE) {
        component = window->image;
    } else {
        component = window->canvas;
    }
    swReverseRegionForComponent(component, window, channel, start_d, end_d,
                                main_flag, overview_flag);
    
    spDebug(80, "swReverseRegion", "done\n");
    
    return;
}

void swRedrawRegion(spComponent component, swWindow window, spBool main_flag, spBool overview_flag)
{
    if (window == NULL || (window->sel_st < 0 && window->sel_ed < 0))
        return;
    
    spDebug(30, "swRedrawRegion", "sel_st = %ld, sel_ed = %ld, window->draw_width = %f\n",
            (long)window->sel_st, (long)window->sel_ed, window->draw_width);
    
    swUpdateDisplayRegion(window);

    spDebug(30, "swRedrawRegion", "set_st_d = %d, sel_ed_d = %d\n",
            window->sel_st_d, window->sel_ed_d);
    
    swReverseRegionForComponent(component, window, window->wave->selected_channel,
                                window->sel_st_d, window->sel_ed_d, main_flag, overview_flag);

    spDebug(80, "swRedrawRegion", "done\n");
    
    return;
}

void swClearRegion(swWindow window)
{
    if (window == NULL) return;
    
    spDebug(30, "swClearRegion", "sel_st_d = %d, sel_ed_d = %d\n",
            window->sel_st_d, window->sel_ed_d);

    if (window->config->toplevel->rubber_band_selection == SP_TRUE) {
        /* clear region */
        swReverseRegion(window, window->wave->selected_channel, window->sel_st_d, window->sel_ed_d,
                        SP_TRUE, SP_TRUE);
    }
    
    window->sel_st_d = -1;
    window->sel_ed_d = -1;
    window->sel_st = -1;
    window->sel_ed = -1;

    /* refresh window */
    swDrawCursor(window, SP_TRUE);

    if (window->overview_canvas != NULL) {
        spRefreshCanvas(window->overview_canvas);
    }
    
    return;
}

static void drawHCursor(swWindow window, spComponent canvas, spGraphics graphics, int point_d, spBool need_cursor_outline)
{
    int y_start;
    int y_end;

    y_start = 0;
    y_end = window->height;
    
    if (window->draw_height_horizontal_keys > 0.0) {
        if (window->config->horizontal_piano_keys_top == SP_TRUE) {
            y_start = (int)spRound(window->draw_height_horizontal_keys);
        } else {
            y_end -= (int)spRound(window->draw_height_horizontal_keys);
            y_end = MAX(y_end, 1);
        }
    }
    
    if (window->draw_vertical_keys == SP_TRUE && window->config->vertical_piano_keys_right == SP_FALSE) {
        point_d += (int)spRound(window->vertical_keys_width);
    }
    if (window->draw_vertical_keys == SP_TRUE && window->config->vertical_piano_keys_right == SP_TRUE) {
        point_d = MIN(point_d, window->width - window->meter_width - 1 - (int)spRound(window->vertical_keys_width));
    } else {
        point_d = MIN(point_d, window->width - window->meter_width - 1);
    }
    
    /* draw cursor line */
    if (need_cursor_outline) {
        spDrawLine(canvas, sw_graphics->gx_bg, point_d, y_start, point_d, y_end);
    }
    spDrawLine(canvas, graphics, point_d, y_start, point_d, y_end);
    
    return;
}

static spBool drawOrderCursor(swWindow window, swWaveSubArea sub_area, spComponent canvas, spGraphics graphics, int point_d)
{
    int n;
    int height;
    int offset;
    double draw_min;
    int x_min, x_max;
    swWave wave;
    spBool outline_drawn = SP_FALSE;

    wave = sub_area->wave;

    if (wave->num_order <= 1
        || window->target_order < 0 || window->target_order >= wave->num_order) {
        spDebug(100, "drawOrderCursor", "not required: wave->num_order = %ld, window->target_order = %ld\n",
                wave->num_order, window->target_order);
        return SP_FALSE;
    }

    offset = swOrderToYPos(window, sub_area, window->target_order);
    draw_min = sub_area->y_d;
    spDebug(100, "drawOrderCursor", "offset = %d / %.1f, draw_min = %f, window->target_order = %ld / %ld\n",
            offset, sub_area->draw_height, draw_min, window->target_order, wave->num_order);

    {
        int x_offset = 0;
        int x_width;

        x_width = swGetDrawWidth(window, SP_TRUE);
        
        if (window->draw_vertical_keys == SP_TRUE) {
            if (window->config->vertical_piano_keys_right == SP_FALSE) {
                x_offset += (int)spRound(window->vertical_keys_width);
            }
        }
        x_min = x_offset;
        x_max = x_offset + x_width;
    }
            
    for (n = 0; n < wave->num_channel; n++) {
        height = (int)round(draw_min) + offset;

        if (window->config->specgram_gray_scale == SP_FALSE) {
            /* draw outline of the order line for non-gray case */
            spDrawLine(canvas, sw_graphics->gx_bg, x_min, height, x_max, height);
            outline_drawn = SP_TRUE;
        }
        
        /* draw order cursor line */
        spDrawLine(canvas, graphics, x_min, height, x_max, height);
        
        draw_min += sub_area->draw_height;
    }
    
    return outline_drawn;
}

void swDrawHCursor(swWindow window)
{
    swWaveSubArea sub_area;
    spBool need_cursor_outline = SP_FALSE;
    
    sub_area = swGetNextWaveSubArea(window, NULL);
                    
    while (sub_area != NULL) {
        if (swIsWaveSubAreaVisible(sub_area) == SP_TRUE) {
            if (drawOrderCursor(window, sub_area, window->canvas,
                                sw_graphics->gx_pointer, window->point_d) == SP_TRUE) {
                need_cursor_outline = SP_TRUE;
            }
        }
        sub_area = swGetNextWaveSubArea(window, sub_area);
    }
        
    if (window->point_d >= 0 && window->point_d <= swGetDrawWidth(window, SP_TRUE)) {
        drawHCursor(window, window->canvas, sw_graphics->gx_pointer, window->point_d, need_cursor_outline);
    }
    
    return;
}

void swRefreshWindowWithSelection(swWindow window, int channel, int start_d, int end_d,
                                  spBool draw_cursor, spBool overview_flag)
{
    if (window->canvas == NULL) return;
    
    spDebug(100, "swRefreshWindowWithSelection", "%s: in: draw_cursor = %d, overview_flag = %d, start_d = %d, end_d = %d, window->draw_width = %f\n",
            window->name, draw_cursor, overview_flag, start_d, end_d, window->draw_width);

    /* copy wave image */
    swCopyImage(window);

    if (window->config->toplevel->rubber_band_selection == SP_FALSE) {
        if (window->wave != NULL && window->overview_canvas != NULL) {
            swReverseRegionForComponent(window->canvas, window, channel, start_d, end_d,
                                        SP_TRUE, overview_flag); /* reverse main and overview */
            if (overview_flag == SP_TRUE) {
                spRefreshCanvas(window->overview_canvas);           /* refresh overview canvas only (refresh main canvas later) */
            }
        }
    }

#ifdef SW_DRAW_LABEL_CANVAS
#ifdef SW_SUPPORT_MORPHING
    swDrawAnchors(window->canvas, window);
#endif
    swDrawRegionLabels(window->canvas, window);
    swDrawLabels(window->canvas, window);
#endif

    if (draw_cursor == SP_TRUE) {
        /* draw horizontal cursor */
        swDrawHCursor(window);

        /* draw string */
        swDrawString(window);
    }
    
    spDebug(100, "swRefreshWindowWithSelection", "%s: before spRefreshCanvas\n", window->name);
    
    /* refresh window */
    spRefreshCanvas(window->canvas);

    spDebug(100, "swRefreshWindowWithSelection", "%s: done\n", window->name);
    
    return;
}

void swRefreshWindow(swWindow window, spBool draw_cursor, spBool overview_flag)
{
    int channel;

    if (window == NULL) return;

    if (window->wave != NULL) {
        channel = window->wave->selected_channel;
    } else {
        channel = -1;
    }
    
    swRefreshWindowWithSelection(window, channel, window->sel_st_d, window->sel_ed_d,
                                 draw_cursor, overview_flag);
    
    return;
}

void swDrawCursorWithSelection(swWindow window, int channel, int start_d, int end_d, spBool overview_flag)
{
    swRefreshWindowWithSelection(window, channel, start_d, end_d,
                                 spIsFalse(swIsProcessing(window)), overview_flag);
    
    return;
}

void swDrawCursor(swWindow window, spBool overview_flag)
{
    swRefreshWindow(window, spIsFalse(swIsProcessing(window)), overview_flag);
    
    return;
}


static spBool swGetOverviewEdgeDiff(swWindow window, int x, int *stdiff, int *eddiff,
                                    spBool *thumb_flag)
{
    int st_d, ed_d;
    
    if (window == NULL || window->wave == NULL
        || (swIsWaveProcessing(window->wave) == SP_TRUE
            && (swIsWavePlaying(window->wave) == SP_FALSE
                /*|| window->sync_play == SP_TRUE*/
                /*|| swIsWaveThreadSafe(window->wave) == SP_FALSE*/))) {
        spDebug(50, "swGetOverviewEdgeDiff", "failed\n");
        return SP_FALSE;
    }

    st_d = swSampToOverviewDisp(window, window->offset);
    ed_d = swSampToOverviewDisp(window, window->offset + window->length - 1);
    spDebug(100, "swGetOverviewEdgeDiff", "st_d = %d, ed_d = %d, window->offset = %ld, window->length = %ld\n",
            st_d, ed_d, window->offset, window->length);

    *stdiff = x - st_d;
    *eddiff = x - ed_d;

    if (ed_d - st_d <= SW_EDGE_MOTION_RANGE * 2
        && (x > st_d && x < ed_d)) {
        *thumb_flag = SP_TRUE;
    } else {
        *thumb_flag = SP_FALSE;
    }

    return SP_TRUE;
}

static swOverviewDragType swGetOverviewDragType(swWindow window,
                                                int stdiff, int eddiff, spBool thumb_flag)
{
    swOverviewDragType drag_type = SW_OVERVIEW_DRAG_NONE;

    if (thumb_flag == SP_FALSE && ABS(stdiff) <= SW_EDGE_MOTION_RANGE / 2
        && ABS(stdiff) < ABS(eddiff)) {
        drag_type = SW_OVERVIEW_DRAG_START;
    } else if (thumb_flag == SP_FALSE && ABS(eddiff) <= SW_EDGE_MOTION_RANGE / 2) {
        drag_type = SW_OVERVIEW_DRAG_END;
    } else if (stdiff >= 0 && eddiff <= 0) {
        drag_type = SW_OVERVIEW_DRAG_THUMB;
    }
        
    return drag_type;
}

void swOverviewPointerMotionCB(spComponent component, swWindow window)
{
    int x, y;
    int stdiff, eddiff;
    spBool thumb_flag;
    swOverviewDragType drag_type = SW_OVERVIEW_DRAG_NONE;
    static swOverviewDragType prev_drag_type = SW_OVERVIEW_DRAG_NONE;
    
    if (swIsNoWave(window) == SP_TRUE || window->overview_canvas == NULL) return;

    if (swGetCallbackMousePosition(component, window, SP_TRUE, &x, &y) == SP_TRUE) {
        spDebug(100, "swOverviewPointerMotionCB",
                "component = %ld, overview_canvas = %ld, canvs = %ld\n",
                component, window->overview_canvas, window->canvas);
        x = MAX(x, 0);
        x = MIN(x, window->overview_width);
        if (swGetOverviewEdgeDiff(window, x, &stdiff, &eddiff, &thumb_flag) == SP_TRUE) {
            drag_type = swGetOverviewDragType(window, stdiff, eddiff, thumb_flag);
            if (drag_type == SW_OVERVIEW_DRAG_START) {
                swSetCanvasCursor(component, SP_CURSOR_W_RESIZE);
            } else if (drag_type == SW_OVERVIEW_DRAG_END) {
                swSetCanvasCursor(component, SP_CURSOR_E_RESIZE);
            } else if (drag_type == SW_OVERVIEW_DRAG_THUMB) {
                swSetCanvasCursor(component, SP_CURSOR_MOVE);
            } else {
                drag_type = SW_OVERVIEW_DRAG_NONE;
            }
        }
    }

    if (drag_type == SW_OVERVIEW_DRAG_NONE && prev_drag_type != SW_OVERVIEW_DRAG_NONE) {
        spUnsetCanvasCursor(component);
    }
    prev_drag_type = drag_type;
    
    return;
}

void swOverviewButtonCB(spComponent component, swWindow window)
{
    int x, y;
    int stdiff, eddiff;
    spLong offset, length;
    spBool x_log_flag;
    spBool thumb_flag;
    spCallbackReason reason;
    spModifierMask modmask;
    static spBool overview_play_start = SP_FALSE;
    static swOverviewDragType drag_type = SW_OVERVIEW_DRAG_NONE;
    static int offset_diff_disp = 0;
    static spLong overview_play_start_offset = -1;
    
    if (swIsNoWave(window) == SP_TRUE || window->overview_canvas == NULL) return;

    spDebug(50, "swOverviewButton", "in\n");
    
    if (swGetCallbackMousePosition(component, window, SP_TRUE, &x, &y) == SP_TRUE) {
        x = MAX(x, 0);
        x = MIN(x, window->overview_width);
        
        x_log_flag = swIsXAxisLogFrequency(window);
        
        reason = spGetCallbackReason(component);
        spDebug(50, "swOverviewButton", "reason = %d\n", reason);
        
        if (swGetOverviewEdgeDiff(window, x, &stdiff, &eddiff, &thumb_flag) == SP_TRUE) {
            spDebug(80, "swOverviewButton", "stdiff = %d, eddiff = %d, thumb_flag = %d\n", stdiff, eddiff, thumb_flag);
            switch (reason) {
              case SP_CR_LBUTTON_PRESS:
                modmask = 0;
                spGetModifierKeyMask(window->window, &modmask);
                
                drag_type = swGetOverviewDragType(window, stdiff, eddiff, thumb_flag);
                if (modmask & SP_COMMAND_MODIFIER_MASK) {
                    offset = swOverviewDispToSamp(window, x);
                    swScrollWindow(window, MAX(offset - window->length / 2, 0), /*SP_TRUE*/SP_FALSE, SP_FALSE);
                } else if (modmask & SP_EXTEND_MODIFIER_MASK) {
                    if (stdiff < 0 || (eddiff <= 0 && ABS(eddiff) > ABS(stdiff))) {
                        offset = swOverviewDispToSamp(window, x);
                        length = window->length - (offset - window->offset);
                        swZoomRegion(window, offset, length, SP_FALSE);
                        drag_type = SW_OVERVIEW_DRAG_START;
                    } else {
                        offset = window->offset;
                        length = swOverviewDispToSamp(window, x) - offset + 1;
                        swZoomRegion(window, offset, length, SP_FALSE);
                        drag_type = SW_OVERVIEW_DRAG_END;
                    }
                } else {
                    spDebug(80, "swOverviewButton", "window->offset = %ld\n", window->offset);
                    offset = -1;
                    if (drag_type == SW_OVERVIEW_DRAG_NONE) {
                        if (x_log_flag == SP_TRUE && window->length >= 2) {
                            spLong end_pos;
                            double log_offset;
                            double log_range;
                            if (window->wave->custom_x_axis != NODATA) {
                                if (window->offset <= 0) {
                                    log_offset = swCustomXMinLogValue(window->wave->custom_x_axis);
                                } else {
                                    log_offset = log10(window->wave->custom_x_axis->data[window->offset]);
                                }
                                end_pos = (spLong)spRound(window->wave->custom_x_axis->data[window->offset + window->length - 1]);
                            } else {
                                log_offset = log10(MAX(window->offset, SW_LOG_FREQUENCY_MIN_VALUE));
                                end_pos = window->offset + window->length - 1;
                            }
                            log_range = log10((double)end_pos) - log_offset;
                            
                            if (window->wave->custom_x_axis != NODATA) {
                                offset = swXValueToCustomXIndex(window->wave->custom_x_axis, 0, window->wave->custom_x_axis->length,
                                                                log_offset + (stdiff >= 0 ? log_range : -log_range), SP_TRUE, NULL);
                                    
                            } else {
                                if (stdiff < 0) {
                                    /* backward scroll */
                                    offset = (spLong)spRound(pow(10.0, log_offset - log_range));
                                } else if (eddiff > 0) {
                                    /* forward scroll */
                                    offset = (spLong)spRound(pow(10.0, log_offset + log_range));
                                }
                            }
                        } else {
                            if (window->wave->custom_x_axis != NODATA) {
                                offset = swXValueToCustomXIndex(window->wave->custom_x_axis, 0, window->wave->custom_x_axis->length - 1,
                                                                (double)(window->offset + (stdiff >= 0 ? window->length : -window->length)), SP_FALSE, NULL);
                                    
                            } else {
                                if (stdiff < 0) {
                                    /* backward scroll */
                                    offset = window->offset - MAX(window->length, 1);
                                    offset = MAX(offset, 0);
                                } else if (eddiff > 0) {
                                    /* forward scroll */
                                    offset = window->offset + MAX(window->length, 1);
                                    offset = MIN(offset, window->wave->total_length - window->length - 1);
                                }
                            }
                        }
                        spDebug(80, "swOverviewButton", "offset = %ld\n", offset);
                        if (offset >= 0) {
                            swScrollWindow(window, offset, /*SP_TRUE*/SP_FALSE, SP_FALSE);
                        }
                    } else if (drag_type == SW_OVERVIEW_DRAG_THUMB) {
                        offset_diff_disp = x - swSampToOverviewDisp(window, window->offset);
                        spDebug(50, "swOverviewButton",
                                "SP_CR_LBUTTON_PRESS, SW_OVERVIEW_DRAG_THUMB: x = %d, window->offset = %ld, offset_diff_disp = %d\n",
                                x, window->offset, offset_diff_disp);
                    }
                }
                break;
                
              case SP_CR_LBUTTON_MOTION:
                if (drag_type == SW_OVERVIEW_DRAG_START) {
                    offset = swOverviewDispToSamp(window, x);
                    length = window->length - (offset - window->offset);
                    swZoomRegion(window, offset, length, SP_FALSE);
                } else if (drag_type == SW_OVERVIEW_DRAG_END) {
                    offset = window->offset;
                    length = swOverviewDispToSamp(window, x) - offset + 1;
                    swZoomRegion(window, offset, length, SP_FALSE);
                } else if (drag_type == SW_OVERVIEW_DRAG_THUMB) {
                    offset = swOverviewDispToSamp(window, x - offset_diff_disp);
                    swScrollWindow(window, offset, /*SP_TRUE*/SP_FALSE, SP_FALSE);
                    spDebug(50, "swOverviewButton",
                            "SP_CR_LBUTTON_MOTION, SW_OVERVIEW_DRAG_THUMB: x = %d, window->offset = %ld, offset_diff_disp = %d\n",
                            x, window->offset, offset_diff_disp);
                }
                break;
                
              default:
                break;
            }
        }
            
        switch (reason) {
          case SP_CR_LBUTTON_RELEASE:
          case SP_CR_MBUTTON_RELEASE:
          case SP_CR_RBUTTON_RELEASE:
            if (window->data_type != SW_FREQ_DATA) {
                spDebug(50, "swOverviewButton", "drag audio end\n");
                if (drag_type == SW_OVERVIEW_DRAG_AUDIO && overview_play_start == SP_TRUE) {
                    swPlayStop(window);
                }
            }
            overview_play_start = SP_FALSE;
            drag_type = SW_OVERVIEW_DRAG_NONE;
            overview_play_start_offset = -1;
            break;
            
          case SP_CR_MBUTTON_PRESS:
          case SP_CR_RBUTTON_PRESS:
            if (window->data_type != SW_FREQ_DATA) {
                offset = swOverviewDispToSamp(window, x);
                spDebug(50, "swOverviewButton", "drag audio started: offset = %ld\n", offset);
                if (swIsWavePlaying(window->wave) == SP_TRUE) {
                    spDebug(50, "swOverviewButton", "button press while playing\n");
                    swSetPlayStartOffset(window->wave, offset - window->wave->play_offset, SP_FALSE, SP_FALSE);
                    window->current_play_pos = offset;
                } else {
                    spDebug(50, "swOverviewButton", "button press for swPlayRegionEx\n");
                    swUpdatePoint(window, offset, SP_TRUE);
                    swPlayRegionEx(window, 0, window->wave->total_length - 1, offset);
                    overview_play_start = SP_TRUE;
                }
                drag_type = SW_OVERVIEW_DRAG_AUDIO;
                overview_play_start_offset = offset;
                spDebug(50, "swOverviewButton", "button press done\n");
            }
            break;
                
          case SP_CR_MBUTTON_MOTION:
          case SP_CR_RBUTTON_MOTION:
            if (window->data_type != SW_FREQ_DATA && drag_type == SW_OVERVIEW_DRAG_AUDIO) {
                offset = swOverviewDispToSamp(window, x);
                spDebug(50, "swOverviewButton", "drag audio motion: offset = %ld\n", offset);
                if (swIsWavePlaying(window->wave) == SP_TRUE) {
                    if (overview_play_start_offset != offset) {
                        swSetPlayStartOffset(window->wave, offset - window->wave->play_offset, SP_FALSE, SP_FALSE);
                        window->current_play_pos = offset;
                    }
                } else {
                    swPlayRegionEx(window, 0, window->wave->total_length - 1, offset);
                    overview_play_start = SP_TRUE;
                }
                overview_play_start_offset = offset;
            }
            break;
          default:
            return;
        }
    }
    
    spDebug(50, "swOverviewButton", "done\n");
    
    return;
}

void swReverseOverviewRegion(swWindow window, int channel, int start_d, int end_d)
{
    if (window == NULL || window->overview_canvas == NULL || start_d < 0 || end_d < 0) return;

    spDebug(80, "swReverseOverviewRegion", "channel = %d, start_d = %d, end_d = %d, window->overview_width = %d\n",
            channel, start_d, end_d, window->overview_width);

    swFillRegion(window->overview_canvas, sw_graphics->gx_xor, window,
                 0.0, (double)window->overview_width,
                 (double)window->overview_height / (double)window->wave->num_channel,
                 channel, start_d, end_d, SP_TRUE);
    
    spDebug(80, "swReverseOverviewRegion", "done\n");
    
    return;
}

void swUpdateOverview(swWindow window, spBool point_flag, spBool current_area_flag,
                      spBool selection_flag, spBool refresh_flag)
{
    int point_d = 0;
    int offset_d = 0;
    int length_d = 0;
    spGraphics graphics;
    
    if (window == NULL || window->overview_canvas == NULL) return;

    spDebug(100, "swUpdateOverview", "in: selection_flag = %d, refresh_flag = %d\n",
            selection_flag, refresh_flag);
    
    /*spLockMutex(sw_graphics->mutex);*/
    
    if (point_flag == SP_TRUE && window->point >= 0) {
        point_d = swSampToOverviewDisp(window, window->point) + 1;
    }
    if (current_area_flag == SP_TRUE) {
        /* line width of drawing frame is 2. */
        offset_d = swSampToOverviewDisp(window, window->offset) + 1;
        if (swIsXAxisLogFrequency(window) == SP_TRUE || window->wave->custom_x_axis != NODATA) {
            int right_d;
            right_d = swSampToOverviewDisp(window, window->offset + window->length - 1);
            length_d = MAX(right_d - offset_d - 1, 1);
        } else {
            length_d = MAX(swSampToOverviewDisp(window, window->length - 1) - 2, 1);
        }
        spDebug(100, "swUpdateOverview", "window->offset = %ld, window->length = %ld, offset_d = %d, length_d = %d\n",
                window->offset, window->length, offset_d, length_d);
        
        spSetGraphicsParams(sw_graphics->gx_overview_xor,
                            SppForeground, SW_CURRENT_AREA_COLOR,
                            SppLineWidth, 2,
                            NULL);
        spDrawRectangle(window->overview_canvas, sw_graphics->gx_overview_xor,
                        offset_d, 1, length_d, window->overview_height - 2);
    }
    
    if (selection_flag == SP_TRUE) {
        int overview_st_d, overview_ed_d;
        
        overview_st_d = swSampToOverviewDisp(window, window->sel_st);
        overview_ed_d = swSampToOverviewDisp(window, window->sel_ed);
        
        spDebug(100, "swUpdateOverview", "call swReverseOverviewRegion: window->sel_st = %ld, window->sel_ed = %ld, overview_st_d = %d, overview_ed_d = %d\n",
                window->sel_st, window->sel_ed, overview_st_d, overview_ed_d);
        swReverseOverviewRegion(window, window->wave->selected_channel, overview_st_d, overview_ed_d);
    }
    
    if (point_flag == SP_TRUE && window->point >= 0) {
        spPixel pixel;
        
        if (window->config->toplevel->graphics_mode_caps & SP_GRAPHICS_MODE_CAPS_XOR) {
            pixel = spGetForegroundPixel(sw_graphics->gx_bg) ^ spGetForegroundPixel(sw_graphics->gx_pointer);
            graphics = sw_graphics->gx_overview_xor;
            spSetGraphicsParams(graphics,
                                SppForegroundPixel, pixel,
                                SppLineWidth, 1,
                                NULL);
        } else {
            pixel = spGetForegroundPixel(sw_graphics->gx_pointer);
            graphics = sw_graphics->gx_pointer;
        }
        spDrawLine(window->overview_canvas, graphics,
                   point_d, 0, point_d, window->overview_height);
    }

    /*spUnlockMutex(sw_graphics->mutex);*/
    
    if (refresh_flag == SP_TRUE) {
        spDebug(100, "swUpdateOverview", "call spRefreshCanvas\n");
        spRefreshCanvas(window->overview_canvas);
    }

    spDebug(100, "swUpdateOverview", "done\n");
    
    return;
}

void swDrawOverview(swWindow window, spBool refresh_flag)
{
    int n;
    int draw_min, draw_max;
    int x_width, y_width;
    double y_zero_offset;
    double x_factor, y_factor_waveform;
    double y_data_factor;
    double data_min, data_max;
    double draw_width, draw_height;
    spBool x_log_flag;
    
    if (window == NULL || window->overview_canvas == NULL) {
        spDebug(30, "swDrawOverview", "canvas is not prepared\n");
        return;
    }

    spDebug(30, "swDrawOverview", "refresh_flag = %d\n", refresh_flag);
    
    spGetSize(window->overview_canvas, &(window->overview_width), &(window->overview_height));
    spDebug(30, "swDrawOverview", "width = %d, height = %d\n",
            window->overview_width, window->overview_height);

    spFillRectangle(window->overview_canvas, sw_graphics->gx_bg,
                    0, 0, window->overview_width, window->overview_height);
    spDebug(30, "swDrawOverview", "spFillRectangle done\n");

    if (swIsWavePeakAvailable(window->wave) == SP_FALSE) {
        spDebug(30, "swDrawOverview", "peak is not available\n");
        if (refresh_flag == SP_TRUE) {
            spDebug(100, "swDrawOverview", "call spRefreshCanvas\n");
            spRefreshCanvas(window->overview_canvas);
        }
        return;
    }

    draw_width = (double)window->overview_width;
    draw_height = (double)window->overview_height / (double)window->wave->num_channel;
    draw_height = MAX(draw_height, 2);

    x_width = (int)spRound(draw_width);
    y_width = (int)spRound(draw_height);
    
    spDebug(30, "swDrawOverview", "peak_buf_length = %ld, draw_width = %f, draw_height = %f\n",
            window->wave->peak_buf_length, draw_width, draw_height);
    
    swGetDataHeight(window, window->wave, SP_FALSE, &data_min, &data_max);

    if (window->data_type != SW_FREQ_DATA) {
        x_log_flag = SP_FALSE;
    } else {
        x_log_flag = window->log_frequency_axis;
    }

    x_factor = swCalcXFactor(window->wave->custom_x_axis, window->wave->peak_buf_length, 0, window->wave->peak_buf_length, draw_width,
                             window->config->toplevel->log_min_value, x_log_flag);
    spDebug(30, "swDrawOverview", "data_min = %f, data_max = %f, x_factor = %f\n",
            data_min, data_max, x_factor);
    
    for (n = 0; n < window->wave->num_channel; n++) {
        spDebug(80, "swDrawOverview", "n = %d / %d\n", n, window->wave->num_channel);
        swGetDrawRange(0.0, draw_height, n, &draw_min, &draw_max);
        swDrawHScale(window->overview_canvas, window, window->wave, n, 0, window->wave->total_length,
                     0, 0, x_width, 0, y_width, 0, draw_min, draw_max, SP_FALSE);
        swDrawVScale(window->overview_canvas, window, window->wave, n,
                     0, x_width, y_width, draw_min, draw_max, data_min, data_max, 
                     SP_FALSE, SP_FALSE, SP_FALSE, SP_FALSE,
                     &y_data_factor, &y_factor_waveform, &y_zero_offset, NULL, NULL);
        
        spDebug(30, "swDrawOverview", "n = %d, y_factor_waveform = %f, y_zero_offset = %f\n",
                n, y_factor_waveform, y_zero_offset);
        
        swDrawWaveform(window->overview_canvas, window, window->wave, n, 0, window->wave->total_length,
                       0, x_width, draw_min, draw_max, x_factor, y_factor_waveform, y_zero_offset,
                       0, window->wave->peak_buf_length, x_log_flag, SP_TRUE, SP_FALSE);
    }

    swUpdateOverview(window, SP_TRUE, SP_TRUE, SP_TRUE, refresh_flag);
    
    spDebug(30, "swDrawOverview", "done\n");
    
    return;
}

void swDrawWave(swWindow window)
{
    int width, height;
    
    if (window == NULL || spIsCreated(window->canvas) == SP_FALSE)
        return;

    spDebug(30, "swDrawWave", "%s: in\n", window->name);
    
    /*window->drawn_pos = 0;*/
    swResetAllDrawnPos(window);
    
    if (spGetSize(window->canvas, &width, &height) == SP_FALSE) {
        return;
    }

    if (window->image == NULL
        || (width != window->width || height != window->height)) {
        window->width = width;
        window->height = height;
        swUpdateDisplayRegion(window);
        swUpdateWaveSubAreaSize(window);
    }
    
    spSetParams(window->canvas,
                SppHorizontalContentFactor, 1.0,
                SppVerticalContentFactor, 1.0,
                NULL);
    spSetParams(window->overview_canvas,
                SppHorizontalContentFactor, 1.0,
                SppVerticalContentFactor, 1.0,
                NULL);

    spDebug(30, "swDrawWave", "%s: width = %d, height = %d\n",
            window->name, window->width, window->height);
    
    if (window->image == NULL) {
        spCreateImage(window->canvas, "waveImage",
                      window->width, window->height,
                      SppCallbackFunc, swDrawWaveImageCB,
                      SppCallbackData, window,
                      NULL);
    } else {
        swUpdateMeterWidth(window);
        
        spRedrawImage(window->image, window->width, window->height);

        window->point_d = swSampToDisp(window, window->point);
    }
        
    swDrawCursor(window, SP_FALSE);
    
    spDebug(30, "swDrawWave", "%s: done\n", window->name);
    
    return;
}

void swDrawOverviewCB(spComponent component, swWindow window)
{
    spCallbackReason reason;
    
    if (window == NULL) return;

    reason = spGetCallbackReason(component);
    
    spDebug(50, "swDrawOverviewCB", "reason = %d\n", reason);
    
    if (window->overview_canvas == NULL) {
        window->overview_canvas = component;
        spAddCallback(window->overview_canvas, SP_BUTTON_MOTION_CALLBACK |
                      SP_BUTTON_PRESS_CALLBACK | SP_BUTTON_RELEASE_CALLBACK,
                      (spCallbackFunc)swOverviewButtonCB, (void *)window);
        spAddCallback(window->overview_canvas, SP_POINTER_MOTION_CALLBACK,
                      (spCallbackFunc)swOverviewPointerMotionCB, (void *)window);
        spAddCallback(window->overview_canvas, SP_KEY_PRESS_CALLBACK,
                      (spCallbackFunc)swKeyPressCB, (void *)window);
        spAddCallback(window->overview_canvas, SP_WHEEL_CALLBACK | SP_ZOOM_CALLBACK,
                      (spCallbackFunc)swWheelCB, (void *)window);
    }

    swDrawOverview(window, SP_TRUE);
    
    spDebug(50, "swDrawOverviewCB", "done\n");
    
    return;
}

void swDrawWaveCB(spComponent component, swWindow window)
{
    spCallbackReason reason;
    spComponent menu, menu_item, sub_menu;
    
    if (window == NULL) {
        spDebug(30, "swDrawWaveCB", "window == NULL, return\n");
        return;
    }
    
    reason = spGetCallbackReason(component);
    
    spDebug(30, "swDrawWaveCB", "reason = %d\n", reason);

    if (window->canvas == NULL) {
        spDebug(30, "swDrawWaveCB", "create popup menu\n");
        window->canvas = component;
        spAddCallback(window->canvas, SP_BUTTON_MOTION_CALLBACK |
                      SP_BUTTON_PRESS_CALLBACK | SP_BUTTON_RELEASE_CALLBACK,
                      (spCallbackFunc)swButtonMotionCB, (void *)window);
        spAddCallback(window->canvas, SP_POINTER_MOTION_CALLBACK,
                      (spCallbackFunc)swMoveCursorCB, (void *)window);
        spAddCallback(window->canvas, SP_KEY_PRESS_CALLBACK,
                      (spCallbackFunc)swKeyPressCB, (void *)window);
        spAddCallback(window->canvas, SP_WHEEL_CALLBACK | SP_ZOOM_CALLBACK,
                      (spCallbackFunc)swWheelCB, (void *)window);

        menu = spCreatePopupMenu(window->canvas, "popupMenu",
                                 SppHelpPath, "menu/popup_menu.html",
                                 NULL);
#if defined(SW_AH_CUSTOM)
        window->toggle_ah_mode_menu = spAddCheckBoxMenuItem(menu, "toggleAHMode",
                                                            SppTitle, _("SW_MENU_TOGGLE_AH_MODE_LABEL"),
                                                            SppGroupId, SW_AH_GROUP_ID,
                                                            SppSenseLevel, SW_STATE_AH_SESSION_STARTED,
                                                            SppCallbackFunc, swToggleAHModeCB,
                                                            SppCallbackData, window,
                                                            SppSet, SP_FALSE,
                                                            NULL);
        window->link_ah_file_menu = spAddCheckBoxMenuItem(menu, "linkAHFile",
                                                          SppTitle, _("SW_MENU_LINK_TO_AH_FILE_LABEL"),
                                                          SppGroupId, SW_AH_GROUP_ID,
                                                          SppSenseLevel, SW_STATE_AH_SESSION_STARTED,
                                                          SppCallbackFunc, swLinkToAHFileCB,
                                                          SppCallbackData, window,
                                                          SppSet, window->link_ah_file,
                                                          NULL);
        spAddMenuSeparator(menu, "popupMenuAHModeSeparator", NULL);
#endif
        menu_item = spAddMenuItem(menu, "playWave",
                                  SppTitle, SW_MENU_PLAY_LABEL,
                                  SppGroupId, SW_PLAY_GROUP_ID,
                                  SppSenseLevel, SW_STATE_NOT_PLAY_WAVE,
                                  SppCallbackFunc, swPlayRegionCB,
                                  SppCallbackData, window,
                                  SppShortcut, SW_PLAY_SHORTCUT,
                                  SppHelpPath, "menu/popup_menu.html#popup_play",
                                  NULL);
        menu_item = spAddMenuItem(menu, "playWindow",
                                  SppTitle, SW_MENU_PLAY_WINDOW_LABEL,
                                  SppGroupId, SW_PLAY_GROUP_ID,
                                  SppSenseLevel, SW_STATE_NOT_PLAY_WAVE,
                                  SppCallbackFunc, swPlayWindowCB,
                                  SppCallbackData, window,
                                  SppShortcut, SW_PLAY_WINDOW_SHORTCUT,
                                  SppHelpPath, "menu/popup_menu.html#popup_play_window",
                                  NULL);
        menu_item = spAddMenuItem(menu, "playFile",
                                  SppTitle, SW_MENU_PLAY_FILE_LABEL,
                                  SppGroupId, SW_PLAY_GROUP_ID,
                                  SppSenseLevel, SW_STATE_NOT_PLAY_WAVE,
                                  SppCallbackFunc, swPlayFileCB,
                                  SppCallbackData, window,
                                  SppShortcut, SW_PLAY_FILE_SHORTCUT,
                                  SppHelpPath, "menu/popup_menu.html#popup_play_file",
                                  NULL);
        menu_item = spAddMenuItem(menu, "playStop",
                                  SppTitle, SW_MENU_PLAY_STOP_LABEL,
                                  SppGroupId, SW_DATA_GROUP_ID,
                                  SppSenseLevel, SW_STATE_PLAY_TIME_DATA,
                                  SppCallbackFunc, swPlayStopCB,
                                  SppCallbackData, window,
                                  SppShortcut, SW_PLAY_STOP_SHORTCUT,
                                  SppHelpPath, "menu/popup_menu.html#popup_stop",
                                  NULL);
        spAddMenuSeparator(menu, "popupMenuPlaySeparator", NULL);
        menu_item = spAddMenuItem(menu, "zoomIn",
                                  SppTitle, SW_MENU_ZOOM_IN_LABEL,
                                  SppGroupId, SW_PAGE_GROUP_ID,
                                  SppSenseLevel, SW_STATE_EXIST_WAVE,
                                  SppCallbackFunc, swZoomInCB,
                                  SppCallbackData, window,
                                  SppShortcut, SW_ZOOM_IN_SHORTCUT,
                                  SppHelpPath, "menu/popup_menu.html#popup_zoom_in",
                                  NULL);
        menu_item = spAddMenuItem(menu, "zoomOut",
                                  SppTitle, SW_MENU_ZOOM_OUT_LABEL,
                                  SppGroupId, SW_PAGE_GROUP_ID,
                                  SppSenseLevel, SW_STATE_EXIST_WAVE,
                                  SppCallbackFunc, swZoomOutCB,
                                  SppCallbackData, window,
                                  SppShortcut, SW_ZOOM_OUT_SHORTCUT,
                                  SppHelpPath, "menu/popup_menu.html#popup_zoom_out",
                                  NULL);
        menu_item = spAddMenuItem(menu, "zoomFullOut",
                                  SppTitle, SW_MENU_ZOOM_FULL_OUT_LABEL,
                                  SppGroupId, SW_PAGE_GROUP_ID,
                                  SppSenseLevel, SW_STATE_EXIST_WAVE,
                                  SppCallbackFunc, swZoomFullOutCB,
                                  SppCallbackData, window,
                                  SppShortcut, SW_ZOOM_FULL_OUT_SHORTCUT,
                                  SppHelpPath, "menu/popup_menu.html#popup_zoom_full_out",
                                  NULL);
        menu_item = spAddMenuItem(menu, "zoomRegion",
                                  SppTitle, SW_MENU_ZOOM_REGION_LABEL,
                                  SppGroupId, SW_PAGE_GROUP_ID,
                                  SppSenseLevel, SW_STATE_SELECT_WAVE,
                                  SppCallbackFunc, swZoomRegionCB,
                                  SppCallbackData, window,
                                  SppShortcut, SW_ZOOM_REGION_SHORTCUT,
                                  SppHelpPath, "menu/popup_menu.html#popup_zoom_region",
                                  NULL);
        spAddMenuSeparator(menu, "popupMenuZoomSeparator", NULL);
        menu_item = spAddMenuItem(menu, "alignWindow",
                                  SppTitle, SW_MENU_ALIGN_WINDOW_LABEL,
                                  SppGroupId, SW_WINDOW_GROUP_ID,
                                  SppSenseLevel, SW_STATE_STOP_SOME_WINDOWS,
                                  SppCallbackFunc, swAlignWindowCB,
                                  SppCallbackData, window,
                                  SppShortcut, SW_ALIGN_WINDOW_SHORTCUT,
                                  SppHelpPath, "menu/popup_menu.html#popup_align",
                                  NULL);
        menu_item = spAddMenuItem(menu, "selectAllRegion",
                                  SppTitle, SW_MENU_SELECT_ALL_REGION_LABEL,
                                  SppGroupId, SW_WINDOW_GROUP_ID,
                                  SppSenseLevel, SW_STATE_SOME_WINDOWS,
                                  SppCallbackFunc, swSelectAllRegionCB,
                                  SppCallbackData, window,
                                  SppShortcut, SW_SELECT_ALL_REGION_SHORTCUT,
                                  SppHelpPath, "menu/popup_menu.html#popup_select_all_region",
                                  NULL);
        menu_item = spAddMenuItem(menu, "selectFromHere",
                                  SppTitle, SW_MENU_SELECT_FROM_HERE_LABEL,
                                  SppSenseLevel, SW_STATE_EXIST_WAVE,
                                  SppCallbackFunc, swSelectFromHereCB,
                                  SppCallbackData, window,
                                  SppHelpPath, "menu/popup_menu.html#popup_select_from_here",
                                  NULL);
        menu_item = spAddMenuItem(menu, "selectToHere",
                                  SppTitle, SW_MENU_SELECT_TO_HERE_LABEL,
                                  SppSenseLevel, SW_STATE_EXIST_WAVE,
                                  SppCallbackFunc, swSelectToHereCB,
                                  SppCallbackData, window,
                                  SppHelpPath, "menu/popup_menu.html#popup_select_to_here",
                                  NULL);

#ifdef SW_SUPPORT_CLIPBOARD
        if (swIsClipboardWindow(window) == SP_FALSE) {
            spAddMenuSeparator(menu, "popupMenuEditSeparator", NULL);
            sub_menu = spAddSubMenu(menu, "editPopupMenu",
                                    SppTitle, SW_MENU_EDIT_LABEL,
                                    SppHelpPath, "menu/popup_menu.html#popup_edit",
                                    NULL);
            menu_item = spAddMenuItem(sub_menu, "cutWave",
                                      SppTitle, SW_MENU_CUT_LABEL,
                                      SppGroupId, SW_CLIPBOARD_DATA_GROUP_ID,
                                      SppSenseLevel, SW_STATE_SELECT_TIME_CHANNELS,
                                      SppCallbackFunc, swCutWindowCB,
                                      SppCallbackData, window,
                                      SppShortcut, SW_CUT_SHORTCUT,
                                      SppHelpPath, "menu/popup_menu.html#popup_cut",
                                      NULL);
            menu_item = spAddMenuItem(sub_menu, "copyWave",
                                      SppTitle, SW_MENU_COPY_LABEL,
                                      SppGroupId, SW_CLIPBOARD_DATA_GROUP_ID,
                                      SppSenseLevel, SW_STATE_SELECT_TIME_DATA,
                                      SppCallbackFunc, swCopyWindowCB,
                                      SppCallbackData, window,
                                      SppShortcut, SW_COPY_SHORTCUT,
                                      SppHelpPath, "menu/popup_menu.html#popup_copy",
                                      NULL);
            spAddMenuSeparator(sub_menu, "popupSubMenuClipboardSeparator", NULL);
            menu_item = spAddMenuItem(sub_menu, "paste",
                                      SppTitle, SW_MENU_PASTE_LABEL,
                                      SppGroupId, SW_CLIPBOARD_GROUP_ID,
                                      SppSenseLevel, SW_STATE_EXIST_CLIPBOARD,
                                      SppCallbackFunc, swPasteWindowCB,
                                      SppCallbackData, window,
                                      SppShortcut, SW_PASTE_SHORTCUT,
                                      SppHelpPath, "menu/popup_menu.html#popup_paste",
                                      NULL);
            menu_item = spAddMenuItem(sub_menu, "mix",
                                      SppTitle, SW_MENU_MIX_LABEL,
                                      SppGroupId, SW_CLIPBOARD_GROUP_ID,
                                      SppSenseLevel, SW_STATE_EXIST_CLIPBOARD,
                                      SppCallbackFunc, swMixWindowCB,
                                      SppCallbackData, window,
                                      SppShortcut, SW_MIX_SHORTCUT,
                                      SppHelpPath, "menu/popup_menu.html#popup_mix",
                                      NULL);
            menu_item = spAddMenuItem(sub_menu, "insert",
                                      SppTitle, SW_MENU_INSERT_LABEL,
                                      SppGroupId, SW_CLIPBOARD_GROUP_ID,
                                      SppSenseLevel, SW_STATE_EXIST_CLIPBOARD,
                                      SppCallbackFunc, swInsertWindowCB,
                                      SppCallbackData, window,
                                      SppShortcut, SW_INSERT_SHORTCUT,
                                      SppHelpPath, "menu/popup_menu.html#popup_insert",
                                      NULL);
            menu_item = spAddMenuItem(sub_menu, "replace",
                                      SppTitle, SW_MENU_REPLACE_LABEL,
                                      SppGroupId, SW_CLIPBOARD_GROUP_ID,
                                      SppSenseLevel, SW_STATE_EXIST_CLIPBOARD_SELECTED,
                                      SppCallbackFunc, swReplaceWindowCB,
                                      SppCallbackData, window,
                                      SppShortcut, SW_REPLACE_SHORTCUT,
                                      SppHelpPath, "menu/popup_menu.html#popup_replace",
                                      NULL);
            spAddMenuSeparator(sub_menu, "popupSubMenuEditSeparator", NULL);
            menu_item = spAddMenuItem(sub_menu, "valueChange",
                                      SppTitle, SW_MENU_CHANGE_VALUE_LABEL,
                                      SppGroupId, SW_DATA_GROUP_ID,
                                      SppSenseLevel, SW_STATE_TIME_DATA,
                                      SppCallbackFunc, swPopupValueChangeDialogCB,
                                      SppCallbackData, window,
                                      SppHelpPath, "menu/popup_menu.html#popup_change_value",
                                      NULL);
            menu_item = spAddMenuItem(sub_menu, "insertPause",
                                      SppTitle, SW_MENU_INSERT_PAUSE_LABEL,
                                      SppGroupId, SW_DATA_GROUP_ID,
                                      SppSenseLevel, SW_STATE_TIME_DATA,
                                      SppCallbackFunc, swPopupInsertPauseDialogCB,
                                      SppCallbackData, window,
                                      SppHelpPath, "menu/popup_menu.html#popup_insert_pause",
                                      NULL);
        }
#endif
        
        spAddMenuSeparator(menu, "popupMenuLabelSeparator", NULL);
        sub_menu = spAddSubMenu(menu, "labelPopupMenu",
                                SppTitle, SW_MENU_LABEL_LABEL,
                                SppHelpPath, "menu/popup_menu.html#popup_label",
                                NULL);
        menu_item = spAddMenuItem(sub_menu, "insertSimpleLabel",
                                  SppTitle, SW_MENU_INSERT_SIMPLE_LABEL_LABEL,
                                  SppSenseLevel, SW_STATE_EXIST_WAVE,
                                  SppCallbackFunc, swInsertSimpleLabelCB,
                                  SppCallbackData, window,
                                  SppShortcut, SW_INSERT_SIMPLE_LABEL_SHORTCUT,
                                  SppHelpPath, "menu/popup_menu.html#popup_insert_simple_label",
                                  NULL);
        menu_item = spAddMenuItem(sub_menu, "insertValueLabel",
                                  SppTitle, SW_MENU_INSERT_VALUE_LABEL_LABEL,
                                  SppSenseLevel, SW_STATE_EXIST_WAVE,
                                  SppCallbackFunc, swInsertValueLabelCB,
                                  SppCallbackData, window,
                                  SppHelpPath, "menu/popup_menu.html#popup_insert_value_label",
                                  NULL);
        menu_item = spAddMenuItem(sub_menu, "insertLabel",
                                  SppTitle, SW_MENU_INSERT_LABEL_LABEL,
                                  SppSenseLevel, SW_STATE_EXIST_WAVE,
                                  SppCallbackFunc, swInsertLabelCB,
                                  SppCallbackData, window,
                                  SppHelpPath, "menu/popup_menu.html#popup_insert_label",
                                  NULL);
        menu_item = spAddMenuItem(sub_menu, "changeLabel",
                                  SppTitle, SW_MENU_CHANGE_LABEL_LABEL,
                                  SppGroupId, SW_LABEL_GROUP_ID,
                                  SppSenseLevel, SW_STATE_EXIST_LABEL_HERE,
                                  SppCallbackFunc, swChangeLabelCB,
                                  SppCallbackData, window,
                                  SppHelpPath, "menu/popup_menu.html#popup_change_label",
                                  NULL);
        menu_item = spAddMenuItem(sub_menu, "eraseLabel",
                                  SppTitle, SW_MENU_ERASE_LABEL_LABEL,
                                  SppGroupId, SW_LABEL_GROUP_ID,
                                  SppSenseLevel, SW_STATE_EXIST_LABEL_HERE,
                                  SppShortcut, SW_ERASE_LABEL_SHORTCUT,
                                  SppCallbackFunc, swEraseLabelCB,
                                  SppHelpPath, "menu/popup_menu.html#popup_erase_label",
                                  SppCallbackData, window,
                                  NULL);
        menu_item = spAddMenuItem(sub_menu, "eraseLabelRegion",
                                  SppTitle, SW_MENU_ERASE_LABEL_REGION_LABEL,
                                  SppGroupId, SW_LABEL_GROUP_ID,
                                  SppSenseLevel, SW_STATE_EXIST_LABEL,
                                  SppCallbackFunc, swEraseLabelRegionCB,
                                  SppCallbackData, window,
                                  SppHelpPath, "menu/popup_menu.html#popup_erase_label_region",
                                  NULL);
        menu_item = spAddMenuItem(sub_menu, "selectBetweenLabels",
                                  SppTitle, SW_MENU_SELECT_BETWEEN_LABELS_LABEL,
                                  /*SppGroupId, SW_LABEL_GROUP_ID,*/
                                  SppGroupId, SW_NORMAL_LABEL_GROUP_ID,
                                  SppSenseLevel, SW_STATE_EXIST_LABEL,
                                  SppCallbackFunc, swSelectBetweenLabelsCB,
                                  SppCallbackData, window,
                                  SppHelpPath, "menu/popup_menu.html#popup_select_between_labels",
                                  NULL);
        spAddMenuSeparator(sub_menu, "popupMenuRegionLabelSeparator", NULL);
        menu_item = spAddMenuItem(sub_menu, "setRegionLabel",
                                  SppTitle, SW_MENU_SET_REGION_LABEL_LABEL,
                                  SppSenseLevel, SW_STATE_SELECT_WAVE,
                                  SppShortcut, SW_SET_REGION_LABEL_SHORTCUT,
                                  SppCallbackFunc, swSetRegionLabelCB,
                                  SppCallbackData, window,
                                  SppHelpPath, "menu/popup_menu.html#popup_set_region_label",
                                  NULL);
        menu_item = spAddMenuItem(sub_menu, "setRegionLabelAsRegion",
                                  SppTitle, SW_MENU_SET_REGION_LABEL_AS_REGION_LABEL,
                                  SppGroupId, SW_REGION_LABEL_GROUP_ID,
                                  SppSenseLevel, SW_STATE_EXIST_LABEL_HERE,
                                  SppCallbackFunc, swSetRegionLabelAsRegionCB,
                                  SppCallbackData, window,
                                  SppHelpPath, "menu/popup_menu.html#popup_set_region_label_as_region",
                                  NULL);
        menu_item = spAddMenuItem(sub_menu, "catRegionLabelsRegion",
                                  SppTitle, SW_MENU_CAT_REGION_LABELS_LABEL,
                                  SppGroupId, SW_REGION_LABEL_GROUP_ID,
                                  SppSenseLevel, SW_STATE_EXIST_LABEL_HERE,
                                  SppShortcut, SW_CAT_REGION_LABELS_SHORTCUT,
                                  SppCallbackFunc, swCatRegionLabelsCB,
                                  SppCallbackData, window,
                                  SppHelpPath, "menu/popup_menu.html#popup_concat_region_labels",
                                  NULL);
        menu_item = spAddMenuItem(sub_menu, "divideRegionLabelRegion",
                                  SppTitle, SW_MENU_DIVIDE_REGION_LABEL_LABEL,
                                  SppGroupId, SW_REGION_LABEL_GROUP_ID,
                                  SppSenseLevel, SW_STATE_EXIST_LABEL,
                                  SppShortcut, SW_DIVIDE_REGION_LABEL_SHORTCUT,
                                  SppCallbackFunc, swDivideRegionLabelCB,
                                  SppCallbackData, window,
                                  SppHelpPath, "menu/popup_menu.html#popup_divide_region_label",
                                  NULL);
#ifdef SW_SUPPORT_MORPHING
        spAddMenuSeparator(menu, "popupMenuAnchorSeparator", NULL);
        swCreateAnchorSubMenu(window, menu);
#endif
        
#ifdef SW_USE_ANALYSIS
        spAddMenuSeparator(menu, "popupMenuAnalysisSeparator", NULL);
        sub_menu = spAddSubMenu(menu, "analysisPopupMenu",
                                SppTitle, SW_MENU_ANALYSIS_LABEL,
                                SppHelpPath, "menu/popup_menu.html#popup_analysis",
                                NULL);
        menu_item = spAddMenuItem(sub_menu, "analysisRegion",
                                  SppTitle, SW_MENU_ANALYSIS_REGION_LABEL,
                                  SppGroupId, SW_DATA_GROUP_ID,
                                  SppSenseLevel, SW_STATE_SELECT_TIME_DATA,
                                  SppShortcut, SW_ANALYSIS_REGION_SHORTCUT,
                                  SppCallbackFunc, swAnalysisRegionCB,
                                  SppCallbackData, window,
                                  SppHelpPath, "menu/popup_menu.html#popup_analysis_region",
                                  NULL);
        menu_item = spAddMenuItem(sub_menu, "analysisWide",
                                  SppTitle, SW_MENU_ANALYSIS_WIDE_LABEL,
                                  SppGroupId, SW_DATA_GROUP_ID,
                                  SppSenseLevel, SW_STATE_TIME_DATA,
                                  SppCallbackFunc, swAnalysisWideCB,
                                  SppCallbackData, window,
                                  SppHelpPath, "menu/popup_menu.html#popup_analysis_wide",
                                  NULL);
        menu_item = spAddMenuItem(sub_menu, "analysisNarrow",
                                  SppTitle, SW_MENU_ANALYSIS_NARROW_LABEL,
                                  SppGroupId, SW_DATA_GROUP_ID,
                                  SppSenseLevel, SW_STATE_TIME_DATA,
                                  SppCallbackFunc, swAnalysisNarrowCB,
                                  SppCallbackData, window,
                                  SppHelpPath, "menu/popup_menu.html#popup_analysis_narrow",
                                  NULL);
#if defined(SW_SUPPORT_CQT_ANALYSIS)
        menu_item = spAddMenuItem(sub_menu, "analysisCQT",
                                  SppTitle, SW_MENU_ANALYSIS_CQT_LABEL,
                                  SppGroupId, SW_DATA_GROUP_ID,
                                  SppSenseLevel, SW_STATE_TIME_DATA,
                                  SppCallbackFunc, swAnalysisCQTCB,
                                  SppCallbackData, window,
                                  SppHelpPath, "menu/popup_menu.html#popup_analysis_cqt",
                                  NULL);
#endif
#endif

#if 1 && defined(SW_AH_CUSTOM)
        if (window->config->toplevel->ahinforec.session_started == SP_TRUE) {
            swUpdateAHSessionState(window);
        }
#endif
        
        swSetSenseLevel(window);
    }

    swDrawWave(window);
    
    spDebug(30, "swDrawWaveCB", "done\n");
    
    return;
}

void swRedrawWave(swWindow window)
{
    if (window == NULL) return;

    spDebug(20, "swRedrawWave", "in\n");

    /* draw wave */
    swDrawWave(window);
    
    /* set parameter value of window  */
    swSetWindowValue(window);

    spDebug(20, "swRedrawWave", "done\n");
    
    return;
}

void swReloadWave(swWindow window, swWave wave, spBool draw_subplot, spBool in_thread)
{
    spLong offset;
    spLong length;
    
    if (window == NULL) return;

    spDebug(20, "swReloadWave", "in: in_thread = %d\n", in_thread);

    if (wave == NULL) wave = window->wave;
    
    /*window->drawn_pos = 0;*/
    swResetAllDrawnPos(window);
    swSetWindowValue(window);
    
    /* read wave */
    offset = MAX(swSampToTargetSamp(window, wave, window->offset), 0);
    length = MAX(swSampToTargetSamp(window, wave, window->offset + window->length - 1) + 1 - offset, 1);
    spDebug(20, "swReloadWave", "read: offset = %ld -> %ld, length = %ld -> %ld\n",
            window->offset, offset, window->length, length);
    swReadWave(wave, in_thread, offset, length);

    if (draw_subplot == SP_TRUE) {
        swWaveSubArea sub_area;
        
        sub_area = swGetNextWaveSubArea(window, NULL);
                    
        while (sub_area != NULL) {
            if (sub_area->wave != NULL && sub_area->wave != wave) {
                offset = MAX(swSampToTargetSamp(window, sub_area->wave, window->offset), 0);
                length = MAX(swSampToTargetSamp(window, sub_area->wave, window->offset + window->length - 1) + 1 - offset, 1);
                spDebug(20, "swReloadWave", "read for subplot: offset = %ld -> %ld, length = %ld -> %ld\n",
                        window->offset, offset, window->length, length);
                swReadWave(sub_area->wave, in_thread, offset, length);
            }
            sub_area = swGetNextWaveSubArea(window, sub_area);
        }
    }
    
    swDrawWave(window);

    spDebug(20, "swReloadWave", "done\n");

    return;
}

void swDrawAllWave(swWindow window)
{
    spComponent next = NULL;
    swWindow next_window = NULL;
    
    if (window == NULL) return;

    spDebug(100, "swDrawAllWave", "%s: in\n", window->name);
    
    swUpdateWaveSubAreaSize(window);
    swDrawWave(window);
    spDebug(100, "swDrawAllWave", "call swDrawOverview\n");
    swDrawOverview(window, SP_TRUE);
    
    next = window->window;
    while (1) {
        next = spGetNextWindow(next, SP_FALSE);
        if (next == NULL || next == window->window) {
            break;
        }

        if ((next_window = (swWindow)spGetUserData(next)) != NULL) {
            spDebug(100, "swDrawAllWave", "call swDrawWave of next_window (%s)\n", next_window->name);
            swUpdateWaveSubAreaSize(next_window);
            swDrawWave(next_window);
            spDebug(100, "swDrawAllWave", "call swDrawOverview of next_window (%s)\n", next_window->name);
            swDrawOverview(next_window, SP_TRUE);
        }
    }

    spDebug(100, "swDrawAllWave", "done\n");
    
    return;
}

static void swSetWindowAmpMinMax(swWindow window, swWave wave, double min, double max, spBool normalized_minmax);

void swAlignWindowCB(spComponent component, swWindow window)
{
    spLong k;
    spLong offset, length;
    double start_f, end_f;
    double min, max;
    spComponent next = NULL;
    swWindow next_window = NULL;

    if (window == NULL || window->wave == NULL) return;

    start_f = swSampToDim(window, window->offset);
    end_f = swSampToDim(window, window->offset + window->length - 1);
    
    next = window->window;
    for (k = 0;; k++) {
        next = spGetNextWindow(next, SP_FALSE);
        if (next == NULL || next == window->window) {
            break;
        }
        
        if ((next_window = (swWindow)spGetUserData(next)) != NULL
            && next_window->wave != NULL) {
            if (next_window->data_type == window->data_type) {
                offset = swDimToSamp(next_window, start_f);
                length = swDimToSamp(next_window, end_f) - offset + 1;
                length = MAX(length, 2);
                length = MIN(length, next_window->wave->total_length - offset);
            
                swZoomRegion(next_window, offset, length, SP_FALSE);
            } else if (window->data_type == SW_FREQ_DATA && swIsSpectrogramVisible(next_window) == SP_TRUE) {
                min = start_f / (next_window->wave->samp_rate / 2.0);
                max = end_f / (next_window->wave->samp_rate / 2.0);
                swSetWindowAmpMinMax(next_window, swGetTargetWave(next_window), min, max, SP_TRUE);
            }
        }
    }
    
    return;
}

static spBool swEqualLength(spLong ref_length, spLong length, double permit_percent)
{
    spLong diff;
    double error_percent;

    diff = ref_length - length;
    diff = ABS(diff);
    error_percent = 100.0 * (double)diff / (double)ref_length;

    if (error_percent > permit_percent) {
        return SP_FALSE;
    } else {
        return SP_TRUE;
    }
}

void swZoomRegion(swWindow window, spLong offset, spLong length, spBool in_thread)
{
    swWave wave;
    spLong input_length;
    double factor;
    
    if (window->process_flag == SP_TRUE || window->wave == NULL) return;
    
    spDebug(50, "swZoomRegion", "in, offset = %ld, length = %ld, wave->total_length = %ld\n",
            offset, length, window->wave->total_length);
    
    input_length = length;
    factor = (double)window->length / (double)input_length;
    spDebug(50, "swZoomRegion", "window->length = %ld, length = %ld, factor = %f\n", window->length, length, factor);
    
    wave = swGetTargetWave(window);
#if 1
    offset = swTargetSampToSamp(window, wave, swSampToTargetSamp(window, wave, offset));
    length = swTargetSampToSamp(window, wave, swSampToTargetSamp(window, wave, length - 1)) + 1;
#endif
    spDebug(80, "swZoomRegion", "offset = %ld, length = %ld, input_length = %ld, window->offset = %ld, window->length = %ld\n",
            offset, length, input_length, window->offset, window->length);

#if 1
    if (factor < 1.0 && swEqualLength(input_length, length, 0.1) == SP_TRUE) {
        spDebug(80, "swZoomRegion", "The corrected length (%ld) is identical to the window length (%ld). So, set to %ld\n",
                length, window->length, input_length);
        length = input_length;
    }
#endif

#if 1
    spDebug(80, "swZoomRegion", "offset (%ld) + length (%ld) - 1 = %ld, swTargetSampToSamp(window, wave, wave->total_length - 1) = %ld\n",
            offset, length, offset + length - 1, swTargetSampToSamp(window, wave, wave->total_length - 1));
    spDebug(80, "swZoomRegion", "offset (%ld) + length (%ld) = %ld, swTargetSampToSamp(window, wave, wave->total_length) = %ld\n",
            offset, length, offset + length, swTargetSampToSamp(window, wave, wave->total_length));
#endif
    
    if (offset >= 0 && length > 1 && 
        /*offset + length <= window->wave->total_length &&*/
        /*offset + length - 1 <= swTargetSampToSamp(window, wave, wave->total_length - 1) &&*/
        offset + (long)spRound((double)length * 0.99) <= swTargetSampToSamp(window, wave, wave->total_length) &&
        !(offset == window->offset && length == window->length)) {
        swUnsetMouseCursor(window);
        
        swLockWindowMutex(window);
        window->process_flag = SP_TRUE;
        
        if (window->config->toplevel->rubber_band_selection == SP_TRUE) {
            spDebug(100, "swZoomRegion", "call swUpdateOverview\n");
            swUpdateOverview(window, SP_FALSE, SP_TRUE, SP_FALSE, SP_FALSE);
        }
        
        window->offset = offset;
        window->length = MIN(length, window->wave->total_length - offset);
        /*window->length = length;*/
        
        spDebug(10, "swZoomRegion", "zoom: %ld, %ld\n", window->offset, window->length);

        swReloadWave(window, wave, SP_TRUE, in_thread);
        
        if (window->config->toplevel->rubber_band_selection == SP_TRUE) {
            swUpdateOverview(window, SP_FALSE, SP_TRUE, SP_FALSE, SP_TRUE);
        } else {
            swDrawOverview(window, SP_TRUE);
        }
        
        window->process_flag = SP_FALSE;
        swUnlockWindowMutex(window);
    }
    
    spDebug(50, "swZoomRegion", "done\n");
    
    return;
}

void swZoomRegionCB(spComponent component, swWindow window)
{
    spLong offset, length;

    if (window != NULL && window->wave != NULL) { 
        offset = window->sel_st;
        length = window->sel_ed - window->sel_st + 1;

        swZoomRegion(window, offset, length, SP_FALSE);
    }

    return;
}

void swZoomWindow(swWindow window, double factor, spBool use_point)
{
    spLong offset, length;

    if (window != NULL && window->wave != NULL) {
        length = (spLong)spRound((double)window->length / factor);
        length = MAX(length, 2L);
        length = MIN(length, window->wave->total_length);
        spDebug(50, "swZoomWindow", "factor = %f, window->length = %ld, length = %ld\n", factor, window->length, length);

        if (use_point == SP_FALSE || swIsProcessing(window) == SP_TRUE) {
            offset = window->offset + window->length / 2 - length / 2;
        } else {
            offset = window->point - length / 2;
        }
        offset = MAX(offset, 0L);
        offset = MIN(offset, window->wave->total_length - length);
        spDebug(50, "swZoomWindow", "factor = %f, window->offset = %ld, offset = %ld, window->wave->total_length = %ld\n",
                factor, window->offset, offset, window->wave->total_length);

        swZoomRegion(window, offset, length, SP_FALSE);
    }

    return;
}

void swZoomInCB(spComponent component, swWindow window)
{
    swZoomWindow(window, 2.0, spIsFalse(window->pause_cursor));

    return;
}

void swZoomOutCB(spComponent component, swWindow window)
{
    swZoomWindow(window, 0.5, spIsFalse(window->pause_cursor));

    return;
}

void swZoomFullOutCB(spComponent component, swWindow window)
{
    spLong offset, length;

    if (window != NULL && window->wave != NULL) {
        length = window->wave->total_length;
        offset = 0L;

        swZoomRegion(window, offset, length, SP_FALSE);
    }

    return;
}

static void swSetWindowAmpMinMax(swWindow window, swWave wave, double min, double max, spBool normalized_minmax)
{
    window->amp_min = min;
    window->amp_max = max;
    if (normalized_minmax == SP_TRUE && wave->num_order > 1) {
        spDebug(100, "swSetWindowAmpMinMax", "window: num_order = %ld, min = %f, max = %f\n", wave->num_order, min, max);
        if (wave->custom_x_axis != NODATA) {
            window->amp_min *= wave->custom_x_axis->data[wave->custom_x_axis->length - 1];
            window->amp_max *= wave->custom_x_axis->data[wave->custom_x_axis->length - 1];
        } else {
            window->amp_min *= (double)(wave->num_order - 1);
            window->amp_max *= (double)(wave->num_order - 1);
        }
        spDebug(100, "swSetWindowAmpMinMax", "window: modified amp_min = %f, amp_max = %f\n", window->amp_min, window->amp_max);
    }
    swUpdateVscroll(window);
    swRedrawWave(window);
    
    return;
}

void swAlignAmplitudeCB(spComponent component, swWindow window)
{
    spLong k;
    spLong start_pos, end_pos;
    spLong length;
    spBool freq_flag;
    double range;
    double min, max;
    double min_freq, max_freq;
    swWave wave, next_wave;
    spComponent next = NULL;
    swWindow next_window = NULL;

    if (window == NULL || window->wave == NULL) return;

    wave = swGetTargetWave(window);
    freq_flag = SP_FALSE;
    min_freq = max_freq = -1.0;
    
    if ((range = swGetAmplitudeRange(window, wave, SP_FALSE, &min, &max)) > 0.0) {
        spDebug(80, "swAlignAmplitudeCB", "range = %f, min = %f, max = %f\n", range, min, max);
        if (wave->num_order > 1) {
            freq_flag = wave->order_frequency_flag;
            if (wave->custom_x_axis != NODATA) {
                if (freq_flag == SP_TRUE) {
                    min_freq = min;
                    max_freq = max;
                }
                min /= wave->custom_x_axis->data[wave->custom_x_axis->length - 1];
                max /= wave->custom_x_axis->data[wave->custom_x_axis->length - 1];
            } else {
                min /= (double)(wave->num_order - 1);
                max /= (double)(wave->num_order - 1);
                if (freq_flag == SP_TRUE) {
                    min_freq = min * (window->wave->samp_rate / 2.0);
                    max_freq = max * (window->wave->samp_rate / 2.0);
                }
            }
            spDebug(80, "swAlignAmplitudeCB", "num_order = %ld, modified min = %f, max = %f, min_freq = %f, max_freq = %f\n",
                    wave->num_order, min, max, min_freq, max_freq);
        }
        
        next = window->window;
        for (k = 0;; k++) {
            next = spGetNextWindow(next, SP_FALSE);
            if (next == NULL || next == window->window) {
                break;
            }
        
            if ((next_window = (swWindow)spGetUserData(next)) != NULL
                && next_window->wave != NULL) {
                next_wave = swGetTargetWave(next_window);
                
                if (next_window->data_type == window->data_type
                    && ((wave->num_order <= 1 && next_wave->num_order <= 1)
                        || (wave->num_order > 1 && next_wave->num_order > 1))) {
                    swSetWindowAmpMinMax(next_window, next_wave, min, max, SP_TRUE);
                } else if (freq_flag == SP_TRUE && next_window->data_type == SW_FREQ_DATA) {
                    spDebug(80, "swAlignAmplitudeCB", "frequency axis window found\n");
                    start_pos = swDimToSamp(next_window, min_freq);
                    end_pos = swDimToSamp(next_window, max_freq);
                    length = end_pos - start_pos + 1;
                    length = MAX(length, 2);
                    spDebug(80, "swAlignAmplitudeCB", "frequency axis window: start_pos = %ld, end_pos = %ld, length = %ld\n",
                            start_pos, end_pos, length);
                    swZoomRegion(next_window, start_pos, length, SP_FALSE);
                }
            }
        }
    }
    
    return;
}

void swZoomInAmplitudeCB(spComponent component, swWindow window)
{
    double min, max;
    double amp_min, amp_max;
    double range, newrange;
    swWave wave;
    spBool y_log_flag = SP_FALSE;

    if (window != NULL && window->wave != NULL) {
        wave = swGetTargetWave(window);
        
        if (window->log_frequency_axis == SP_TRUE && wave->order_frequency_flag == SP_TRUE) {
            y_log_flag = SP_TRUE;
        }
        
        if ((range = swGetAmplitudeRange(window, wave, y_log_flag, &min, &max)) > 0.0) {
            newrange = range / 2.0;
            spDebug(80, "swZoomInAmplitudeCB", "min = %f, max = %f, range = %f, newrange = %f\n",
                    min, max, range, newrange);

            if (max > 0.0 && min <= 0.0) {
                amp_max = max / 2.0;
                amp_min = amp_max - newrange;
            } else {
                if (max == 0.0 && min < 0.0) {
                    amp_min = min / 2.0;
                } else {
                    amp_min = min + (range - newrange) / 2.0;
                }
                amp_max = amp_min + newrange;
            }
            spDebug(80, "swZoomInAmplitudeCB", "amp_min = %f, amp_max = %f\n", amp_min, amp_max);

            if (y_log_flag == SP_FALSE) {
                window->amp_min = amp_min;
                window->amp_max = amp_max;
            } else {
                window->amp_min = pow(10.0, amp_min);
                if (window->amp_min <= SW_LOG_FREQUENCY_MIN_VALUE+SW_LOG_FREQUENCY_MIN_VALUE_MARGIN) {
                    window->amp_min = 0.0;
                }
                window->amp_max = pow(10.0, amp_max);
                if (window->amp_max <= SW_LOG_FREQUENCY_MIN_VALUE+SW_LOG_FREQUENCY_MIN_VALUE_MARGIN) {
                    window->amp_max = 0.0;
                }
            }
            spDebug(80, "swZoomInAmplitudeCB", "updated: window->amp_min = %f, window->amp_max = %f\n",
                    window->amp_min, window->amp_max);
            
            swUpdateVscroll(window);
            swRedrawWave(window);
        }
    }

    return;
}

void swZoomOutAmplitudeCB(spComponent component, swWindow window)
{
    double min, max;
    double amp_min, amp_max;
    double range, newrange;
    double limit;
    swWave wave;
    spBool y_log_flag = SP_FALSE;

    if (window != NULL && window->wave != NULL) {
        wave = swGetTargetWave(window);
        
        if (window->log_frequency_axis == SP_TRUE && wave->order_frequency_flag == SP_TRUE) {
            y_log_flag = SP_TRUE;
        }
        
        if ((range = swGetAmplitudeRange(window, wave, y_log_flag, &min, &max)) > 0.0) {
            newrange = range * 2.0;

            limit = swGetWindowLimitValue(window, wave);
            spDebug(80, "swZoomOutAmplitudeCB", "min = %f, max = %f, range = %f, newrange = %f, limit = %f\n",
                    min, max, range, newrange, limit);
            
            if (max > 0.0 && min <= 0.0) {
                amp_max = max * 2.0;
                amp_min = amp_max - newrange;
            } else {
                if (max == 0.0 && min < 0.0) {
                    amp_min = min * 2.0;
                } else {
                    amp_min = min + (range - newrange) / 2.0;
                }
                amp_max = amp_min + newrange;
            }
            spDebug(80, "swZoomOutAmplitudeCB", "amp_min = %f, amp_max = %f\n", amp_min, amp_max);
            
            if (y_log_flag == SP_FALSE) {
                window->amp_min = amp_min;
                window->amp_max = amp_max;
                if (limit > 1.0) {
                    window->amp_min = MAX(-limit, window->amp_min);
                    window->amp_max = MIN(limit, window->amp_max);
                }
            } else {
                window->amp_min = pow(10.0, amp_min);
                if (window->amp_min <= SW_LOG_FREQUENCY_MIN_VALUE+SW_LOG_FREQUENCY_MIN_VALUE_MARGIN) {
                    window->amp_min = 0.0;
                }
                window->amp_max = pow(10.0, amp_max);
                if (window->amp_max <= SW_LOG_FREQUENCY_MIN_VALUE+SW_LOG_FREQUENCY_MIN_VALUE_MARGIN) {
                    window->amp_max = 0.0;
                }
            }
            spDebug(80, "swZoomOutAmplitudeCB", "updated: window->amp_min = %f, window->amp_max = %f\n",
                    window->amp_min, window->amp_max);
            
            swUpdateVscroll(window);
            swRedrawWave(window);
        }
    }

    return;
}

void swZoomFullOutAmplitudeCB(spComponent component, swWindow window)
{
    if (window != NULL && window->wave != NULL && window->amp_max > window->amp_min) {
        window->amp_min = 0.0;
        window->amp_max = -1.0;
        swUpdateVscroll(window);
        swRedrawWave(window);
    }
    
    spDebug(80, "swZoomFullOutAmplitudeCB", "done\n");
    
    return;
}

void swScrollAmplitudeCB(spComponent component, swWindow window)
{
    int value;
    double data_min, data_max;
    double data_range;
    double amp_range;
    swWave wave;
    spBool y_log_flag = SP_FALSE;
    
    spDebug(80, "swScrollAmplitudeCB", "window->amp_min = %f, window->amp_max = %f\n", window->amp_min, window->amp_max);
    
    if (window != NULL && window->wave != NULL && window->amp_max > window->amp_min) {
        if (spGetSliderValue(component, &value) == SP_FALSE) return;
        spDebug(80, "swScrollAmplitudeCB", "value = %d\n", value);

        wave = swGetTargetWave(window);
        
        if (window->log_frequency_axis == SP_TRUE && wave->order_frequency_flag == SP_TRUE) {
            y_log_flag = SP_TRUE;
        }
        
        data_range = swGetDataRange(window, wave, /*SP_TRUE*/y_log_flag, &data_min, &data_max);
        spDebug(80, "swScrollAmplitudeCB", "y_log_flag = %d, data_min = %f, data_max = %f, data_range = %f\n",
                y_log_flag, data_min, data_max, data_range);

        if (y_log_flag == SP_TRUE) {
            double log_amp_min, log_amp_max;
            double log_amp_range;
            double lin_amp_min, lin_amp_max;

#if 1
            log_amp_min = log10(MAX(window->amp_min, SW_LOG_FREQUENCY_MIN_VALUE));
            log_amp_max = log10(MAX(window->amp_max, SW_LOG_FREQUENCY_MIN_VALUE));
#else
            log_amp_min = window->amp_min;
            log_amp_max = window->amp_max;
#endif
            log_amp_range = log_amp_max - log_amp_min;
            spDebug(50, "swScrollAmplitudeCB", "log_amp_min = %f, log_amp_max = %f, log_amp_range = %f, data_range = %f\n",
                    log_amp_min, log_amp_max, log_amp_range, data_range);
            
            log_amp_max = data_max - data_range * ((double)value / 10000.0);
            log_amp_min = log_amp_max - log_amp_range;
            log_amp_min = MAX(log_amp_min, data_min);
            lin_amp_max = pow(10.0, log_amp_max);
            lin_amp_min = pow(10.0, log_amp_min);
            if (lin_amp_min <= SW_LOG_FREQUENCY_MIN_VALUE+SW_LOG_FREQUENCY_MIN_VALUE_MARGIN) {
                lin_amp_min = 0.0;
            }
            if (lin_amp_max <= SW_LOG_FREQUENCY_MIN_VALUE+SW_LOG_FREQUENCY_MIN_VALUE_MARGIN) {
                lin_amp_max = 0.0;
            }
            window->amp_min = lin_amp_min;
            window->amp_max = lin_amp_max;
        } else {
            amp_range = window->amp_max - window->amp_min;
            spDebug(80, "swScrollAmplitudeCB", "data_range = %f (%f-%f), amp_range = %f (%f-%f)\n",
                    data_range, data_min, data_max, amp_range, window->amp_min, window->amp_max);

            window->amp_max = data_max - data_range * ((double)value / 10000.0);
            window->amp_min = window->amp_max - amp_range;
            window->amp_min = MAX(window->amp_min, data_min);
        }
        spDebug(80, "swScrollAmplitudeCB", "updated: amp_min = %f, amp_max = %f\n",
                window->amp_min, window->amp_max);

        swRedrawWave(window);
    }

    return;
}

spLong swGetPausePlayStepTime(swWindow window, swWave wave)
{
    spLong step_time;

    step_time = swDimToSamp(window, window->config->pause_play_step_time);
    step_time = MIN(wave->total_length / 4, step_time);
    spDebug(50, "swGetPausePlayStepTime", "step_time = %ld\n", step_time);

    return step_time;
}

spBool swScrollWindowEx(swWindow window,
                        int direction/* -1: prev, 0: use offset, 1: next */,
                        spLong offset, spBool move_cursor, spBool disable_play_position_update,
                        spBool in_thread)
{
    swWave wave;
    spLong prev_offset;
    spLong input_offset;
    spLong orig_samp_end, samp_end;
    spLong window_length;
    spBool x_log_flag;
    spBool flag = SP_FALSE;

    if (window->process_flag == SP_TRUE) return SP_FALSE;
    
    spDebug(50, "swScrollWindowEx",
            "in_thread = %d, direction = %d, offset = %ld, window->offset = %ld, window->length = %ld, move_cursor = %d, pause_cursor = %d\n",
            in_thread, direction, offset, window->offset, window->length, move_cursor, window->pause_cursor);

    swUnsetMouseCursor(window);
    
    swLockWindowMutex(window);
    window->process_flag = SP_TRUE;

    wave = swGetTargetWave(window);

    prev_offset = window->offset;
    orig_samp_end = swTargetSampToSamp(window, wave, wave->total_length - 1);
    samp_end = MIN(orig_samp_end, window->wave->total_length - 1);
    window_length = window->length;
    x_log_flag = swIsXAxisLogFrequency(window);
    spDebug(50, "swScrollWindowEx", "prev_offset = %ld, orig_samp_end = %ld, samp_end = %ld / %ld, window_length = %ld, x_log_flag = %d\n",
            prev_offset, orig_samp_end, samp_end, window->wave->total_length, window_length, x_log_flag);
    
    if (direction != 0 && move_cursor == SP_TRUE
        /*&& window->pause_cursor == SP_TRUE*/
        && swIsWavePlaying(window->wave) == SP_TRUE) {
        spLong step_time;

        step_time = swGetPausePlayStepTime(window, wave);
        offset = MAX(window->current_play_pos, 0);
        
        if (direction < 0) {
            offset -= step_time;
        } else {
            offset += step_time;
        }
        input_offset = offset;
    } else {
        /* skip if wave->custom_x_axis != NODATA */
        if (x_log_flag == SP_TRUE && wave->custom_x_axis == NODATA) {
            double conv_offset;
            double conv_offset_limit;
            double conv_prev_offset;
            double conv_samp_end;
            double conv_range;
            double conv_end_new;

            if (samp_end > 0) {
                if (wave->custom_x_axis != NODATA) {
                    conv_samp_end = log10(wave->custom_x_axis->data[samp_end]);
                    if (prev_offset <= 0) {
                        conv_prev_offset = swCustomXMinLogValue(wave->custom_x_axis);
                    } else {
                        conv_prev_offset = log10(wave->custom_x_axis->data[prev_offset]);
                    }
                    conv_range = log10(wave->custom_x_axis->data[prev_offset + window_length - 1]) - conv_prev_offset;
                    if (offset <= 0) {
                        conv_offset = swCustomXMinLogValue(wave->custom_x_axis);
                    } else {
                        conv_offset = log10(wave->custom_x_axis->data[offset]);
                    }
                } else {
                    conv_samp_end = log10((double)samp_end);
                    conv_prev_offset = log10(MAX((double)prev_offset, SW_LOG_FREQUENCY_MIN_VALUE));
                    conv_range = log10((double)(prev_offset + window_length - 1)) - conv_prev_offset;
                    conv_offset = log10(MAX((double)offset, SW_LOG_FREQUENCY_MIN_VALUE));
                }
                spDebug(50, "swScrollWindowEx", "log: conv_samp_end = %f, conv_prev_offset = %f, conv_range = %f, conv_offset = %f\n",
                        conv_samp_end, conv_prev_offset, conv_range, conv_offset);
                
                conv_end_new = conv_offset + conv_range;
                spDebug(50, "swScrollWindowEx", "original conv_end_new = %f, conv_samp_end = %f\n", conv_end_new, conv_samp_end);
                conv_end_new = MIN(conv_end_new, conv_samp_end);
                //conv_offset_limit = MAX(conv_end_new - conv_range, 0.0);
                conv_offset_limit = conv_end_new - conv_range;
                conv_offset = MIN(conv_offset, conv_offset_limit);
                spDebug(50, "swScrollWindowEx", "conv_end_new = %f, conv_offset_limit = %f, conv_offset = %f\n",
                        conv_end_new, conv_offset_limit, conv_offset);

                if (wave->custom_x_axis != NODATA) {
                    spLong end_new_l;
                        
                    offset = swXValueToCustomXIndex(wave->custom_x_axis, 0, samp_end, conv_offset, x_log_flag, NULL);
                    end_new_l = swXValueToCustomXIndex(wave->custom_x_axis, 0, samp_end, conv_end_new, x_log_flag, NULL);
                    end_new_l = MAX(end_new_l, offset + 1);
                    window_length = end_new_l - offset + 1;
                    spDebug(50, "swScrollWindowEx", "custom: offset = %ld, end_new_l = %ld, offset = %ld, window_length = %ld\n",
                            offset, end_new_l, offset, window_length);
                } else {
                    double offset_new;
                    double end_new;
                        
                    offset_new = pow(10.0, conv_offset);
                    if (offset_new <= SW_LOG_FREQUENCY_MIN_VALUE+/*0.01*/SW_LOG_FREQUENCY_MIN_VALUE_MARGIN) {
                        offset = 0;
                    } else {
                        offset = (spLong)spRound(offset_new);
                    }
                    end_new = pow(10.0, conv_end_new);
                    window_length = (spLong)spRound(end_new) - offset + 1;
                    spDebug(50, "swScrollWindowEx", "log: offset_new = %f, end_new = %f, offset = %ld, window_length = %ld\n",
                            offset_new, end_new, offset, window_length);
                }
            }
        }

        if (move_cursor == SP_FALSE && window->sync_play == SP_TRUE) {
            if (window->current_play_pos >= offset && window->current_play_pos < offset + window_length) {
                disable_play_position_update = SP_TRUE;
            }
        }
    }

    offset = MAX(offset, 0);
    if (disable_play_position_update == SP_FALSE) {
        if ((move_cursor == SP_TRUE || window->sync_play == SP_TRUE)
            && swIsWavePlaying(window->wave) == SP_TRUE) {
            swSetPlayStartOffset(window->wave, offset - wave->play_offset, window->sync_play, SP_FALSE);
            window->current_play_pos = offset;
        }
    }
    offset = MIN(offset, samp_end + 1 - window_length);
    
    spDebug(50, "swScrollWindowEx", "window->offset = %ld, window->length = %ld, offset = %ld, samp_end = %ld, window_length = %ld, prev_offset = %ld\n",
            window->offset, window->length, offset, samp_end, window_length, prev_offset);

    if (offset != prev_offset) {
        spBool update_point = SP_FALSE;
        
        if (move_cursor == SP_TRUE
            && swIsWaveProcessing(window->wave) == SP_FALSE
            && swIsWavePlaying(window->wave) == SP_FALSE) {
            update_point = SP_TRUE;
        }
        
        if (window->config->toplevel->rubber_band_selection == SP_TRUE) {
            swUpdateOverview(window, update_point, SP_TRUE, SP_FALSE, SP_FALSE);
        }
        
        window->offset = offset;
        window->length = window_length;

        swReloadWave(window, wave, SP_TRUE, in_thread);
        
        if (update_point == SP_TRUE) {
            window->point = offset + window_length / 2;
        } else {
            move_cursor = SP_FALSE;
        }

        if (window->config->toplevel->rubber_band_selection == SP_TRUE) {
            swUpdateOverview(window, update_point, SP_TRUE, SP_FALSE, SP_TRUE);
        } else {
            swDrawOverview(window, SP_TRUE);
        }
    }
        
    window->process_flag = SP_FALSE;
    swUnlockWindowMutex(window);

    if (move_cursor == SP_TRUE) {
        swMoveAllCursor(window);
    }
        
    spDebug(50, "swScrollWindowEx", "done\n");
        
    return flag;
}

spBool swScrollWindow(swWindow window, spLong offset, spBool move_cursor, spBool in_thread)
{
    return swScrollWindowEx(window, 0, offset, move_cursor, SP_FALSE, in_thread);
}

void swScrollCB(spComponent component, swWindow window)
{
    int value;
    
    spDebug(80, "swScrollCB", "in\n");
        
    if (window != NULL && window->wave != NULL) {
        if (spGetSliderValue(component, &value) == SP_FALSE) return;
        spDebug(80, "swScrollCB", "value = %d\n", value);

        swScrollWindow(window, (spLong)value * window->scroll_coef,
                       spIsFalse(window->pause_cursor), SP_FALSE);
    }

    spDebug(80, "swScrollCB", "done\n");
    
    return;
}

void swForwardCB(spComponent component, swWindow window)
{
    if (window != NULL && window->wave != NULL) {
        swScrollWindowEx(window, 1, window->offset + MAX(window->length / 4, 1),
                         spIsFalse(window->pause_cursor), SP_FALSE, SP_FALSE);
    }
    
    return;
}

void swBackwardCB(spComponent component, swWindow window)
{
    if (window != NULL && window->wave != NULL) {
        swScrollWindowEx(window, -1, window->offset - MAX(window->length / 4, 1),
                         spIsFalse(window->pause_cursor), SP_FALSE, SP_FALSE);
    }

    return;
}

void swGoHeadCB(spComponent component, swWindow window)
{
    if (window != NULL && window->wave != NULL) {
        swScrollWindow(window, 0, spIsFalse(window->pause_cursor), SP_FALSE);
    }

    return;
}

void swGoTailCB(spComponent component, swWindow window)
{
    spLong offset;
    
    if (window != NULL && window->wave != NULL) {
        offset = window->wave->total_length - window->length;
        if (offset <= 0 && swIsWavePlaying(window->wave) == SP_TRUE) {
            offset = window->wave->total_length - swGetPausePlayStepTime(window, window->wave);
        }
        swScrollWindow(window, offset, spIsFalse(window->pause_cursor), SP_FALSE);
    }

    return;
}

void swNextWindowCB(spComponent component, swWindow window)
{
    swWindow next_window;
    
    if ((next_window = swGetNextWindow(window)) != NULL) {
        spPopupWindow(next_window->window);
    }
    
    return;
}

void swPrevWindowCB(spComponent component, swWindow window)
{
    swWindow prev_window;

    if ((prev_window = swGetPrevWindow(window)) != NULL) {
        spPopupWindow(prev_window->window);
    }
    
    return;
}
