/*
 *	swDraw.h
 */

#ifndef __SWDRAW_H
#define __SWDRAW_H

#include "swWindow.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SW_PIANO_KEYS_DEFAULT_SIZE /*80*/90
#define SW_PIANO_KEYS_MAX_SIZE_PERCENT 20
#define SW_HORIZONTAL_PIANO_KEYS_DEFAULT_SIZE_FOR_PRINT 85
#define SW_VERTICAL_PIANO_KEYS_DEFAULT_SIZE_FOR_PRINT 85

#define SW_CURRENT_AREA_COLOR "#c0c0c0"
    
#define SW_REGION_PIXEL_NEGA_INCR -12
#define SW_REGION_PIXEL_POSI_INCR /*16*/32

#define SW_LOG_FREQUENCY_MIN_VALUE /*0.9*/0.5
/*#define SW_LOG_FREQUENCY_MIN_CENTER_VALUE 0.95*/
#define SW_LOG_FREQUENCY_MIN_VALUE_MARGIN 0.01
#define SW_LOG_FREQUENCY_MIN_BOUNDARY_VALUE 0.707106781186547

typedef struct _swGraphics {
    spGraphics gx_fg;
    spGraphics gx_bg;
    spGraphics gx_xor;
    spGraphics gx_pointer;
    spGraphics gx_string;
    spGraphics gx_label;
    spGraphics gx_region;
    spGraphics gx_region_bg;
    spGraphics gx_scale;
    spGraphics gx_meter;
    spGraphics gx_shading;
    spGraphics gx_overview_xor;
    void *mutex;

    spPixel region_pixel;
    spPixel current_region_pixel;
    spPixel region_bg_pixel;
    int region_pixel_incr;
    char region_color[SP_MAX_LINE];
    char region_label_color[SP_MAX_LINE];
    char region_line_color[SP_MAX_LINE];
    char region_bg_color[SP_MAX_LINE];

    char non_xor_selection_color[SP_MAX_LINE];
} *swGraphics;
    
#if defined(MACOS)
#pragma import on
#endif

extern void swLockGraphicsMutex(void);
extern void swUnlockGraphicsMutex(void);
extern void swSetColor(swConfig config);
extern void swGetDrawRange(double y_draw_offset, double draw_height, int channel, int *draw_min, int *draw_max);
extern int swGetDrawWidth(swWindow window, spBool inside_vertical_keys);
extern double swCalcVerticalKeysWidth(swWindow window, int default_keys_size, double width);
extern double swGetWindowLimitValue(swWindow window, swWave wave);
extern spBool swGetMainStringExtent(spComponent canvas, char *string, int *x, int *y, int *width, int *height);
extern void swDrawMainString(spComponent canvas, int left_offset, int top_offset, char *string);
extern spBool swIsMeterVisible(swWindow window);
extern void swDrawMeterBar(spComponent component, swWindow window, 
			   double draw_height, int left_offset, int top_offset, double value);

extern spBool swIsXAxisLogFrequency(swWindow window);
extern double swGetAmplitudeRange(swWindow window, swWave wave, spBool scaled, double *data_min, double *data_max);
extern double swGetDataRange(swWindow window, swWave wave, spBool scaled, double *data_min, double *data_max);
extern double swGetDataHeight(swWindow window, swWave wave, spBool scaled, double *data_min, double *data_max);
extern void swGetOrderMinMax(swWindow window, swWave wave, spBool log_flag, spBool output_linear,
                             double *p_order_min, double *p_order_max);
extern double swGetOrderRange(swWindow window, swWave wave, spBool log_flag, spBool output_linear,
                              double *p_order_min, double *p_order_max);

extern void swDrawHScaleMain(swConfig config, spComponent component, swDataType data_type, int num_channel, int channel,
                             int x_offset, int y_offset, int x_width_waveform, int x_width_vertical_keys,
                             int y_width, int y_width_horizontal_keys,
                             int draw_min, int draw_max, double x_min, double x_max, double x_samp_dim_factor, 
                             spBool log_flag, spBool for_print);
extern void swDrawVScaleMain(swConfig config, spComponent component, swDataType data_type, int num_channel, int channel,
			     int direction, int x_width, int x_width_vertical_keys,
                             int y_width, int y_width_horizontal_keys, int draw_min, int draw_max,
                             double y_scale_min, double y_scale_max, double y_data_factor, char *ylabel,
                             spBool draw_flag, spBool log_flag, spBool for_print,
                             double *y_factor_waveform, double *y_zero_offset, double *y_scale_min_mod, double *y_scale_max_mod);
extern void swDrawVScale(spComponent component, swWindow window, swWave wave, int channel,
                         int x_offset, int x_width, int y_width, int draw_min, int draw_max,
                         double data_min, double data_max, spBool draw_flag, spBool specgram_flag,
                         spBool draw_vertical_keys_flag, spBool for_print,
                         double *y_data_factor, double *y_factor_waveform, double *y_zero_offset,
                         double *y_scale_min, double *y_scale_max);
extern void swDrawWaveformLine(spComponent component, swWave wave, int channel, spLong offset, spLong length,
                               int x_offset, int x_width, int draw_min, int draw_max,
                               double x_factor, double y_factor, double y_zero_offset,
                               spLong draw_offset, spLong draw_length,
                               spBool x_log_flag, spBool peak_flag, spBool for_print);

extern void swFillBackground(spComponent component, int width, int height);
extern void swDrawBackground(spComponent component, swWindow window);
extern swWave swGetTargetWave(swWindow window);
extern spLong swDrawWaveSubAreaImage(spComponent component, swWindow window, swWaveSubArea sub_area, spLong draw_pos,
				     double amp_min, double amp_max, double draw_height_horizontal_keys);
extern void swDrawWholeWaveImage(spComponent component, swWindow window);
extern void swDrawWaveImage(spComponent component, swWindow window, spLong draw_pos);
extern void swDrawWaveImageCB(spComponent component, swWindow window);

#ifdef SW_SUPPORT_PRINT
extern void swPluginCanvasCB(spComponent component, void *data);
extern spBool swPrintWaveImage(spComponent component, swWindow window);
#endif
    
extern void swGetAmplitudeString(swWave wave, double limit, double value, char *string, spBool percent_flag, spBool spacing_flag);
extern void swGetAmplitudedBString(swWave wave, double limit, double value, char *string, spBool spacing_flag);

extern void swCopyImage(swWindow window);
extern void swGetTimeStringTitle(swConfig config, swDataType data_type, char *title_string, char *dim_string);
extern spBool swGetDrawTimeString(swWindow window, double point_f, char *string, spBool align_left);
extern void swDrawString(swWindow window);
extern void swDrawLabels(spComponent component, swWindow window);
extern void swRedrawLabels(swWindow window);
extern void swRedrawLabelsCB(spComponent component, swWindow window);
extern void swUpdateLabels(swWindow window);
extern void swUnselectActiveLabel(swWindow window);
extern void swResetRegion(swWindow window);
extern void swUpdateDisplayRegion(swWindow window);
extern void swSelectRegion(swWindow window, int channel, spLong start, spLong end);
extern void swSelectRegionCB(spComponent component, swWindow window);
extern void swSelectFromHereCB(spComponent component, swWindow window);
extern void swSelectToHereCB(spComponent component, swWindow window);
extern void swSelectAllRegionCB(spComponent component, swWindow window);
extern void swSelectAllCB(spComponent component, swWindow window);
extern void swSelectNextChannelCB(spComponent component, swWindow window);
extern void swReverseRegionForComponent(spComponent component, swWindow window, int channel,
					int start_d, int end_d, spBool main_flag, spBool overview_flag);
extern void swReverseRegion(swWindow window, int channel,
			    int start_d, int end_d, spBool main_flag, spBool overview_flag);
extern void swRedrawRegion(spComponent component, swWindow window, spBool main_flag, spBool overview_flag);
extern void swClearRegion(swWindow window);
extern void swRefreshWindowWithSelection(swWindow window, int channel, int start_d, int end_d,
					 spBool draw_cursor, spBool overview_flag);
extern void swRefreshWindow(swWindow window, spBool draw_cursor, spBool overview_flag);
extern void swDrawCursorWithSelection(swWindow window, int channel, int start_d, int end_d, spBool overview_flag);
extern void swDrawCursor(swWindow window, spBool overview_flag);
extern void swReverseOverviewRegion(swWindow window, int channel, int start_d, int end_d);
extern void swUpdateOverview(swWindow window, spBool point_flag, spBool current_area_flag,
			     spBool selection_flag, spBool refresh_flag);
extern void swDrawOverview(swWindow window, spBool refresh_flag);
extern void swDrawWave(swWindow window);
extern void swDrawOverviewCB(spComponent component, swWindow window);
extern void swDrawWaveCB(spComponent component, swWindow window);
extern void swRedrawWave(swWindow window);
extern void swReloadWave(swWindow window, swWave wave, spBool draw_subplot, spBool in_thread);
extern void swDrawAllWave(swWindow window);
extern void swAlignWindowCB(spComponent component, swWindow window);
extern void swZoomRegion(swWindow window, spLong offset, spLong length, spBool in_thread);
extern void swZoomRegionCB(spComponent component, swWindow window);
extern void swZoomWindow(swWindow window, double factor, spBool use_point);
extern void swZoomInCB(spComponent component, swWindow window);
extern void swZoomOutCB(spComponent component, swWindow window);
extern void swZoomFullOutCB(spComponent component, swWindow window);
extern void swAlignAmplitudeCB(spComponent component, swWindow window);
extern void swZoomInAmplitudeCB(spComponent component, swWindow window);
extern void swZoomOutAmplitudeCB(spComponent component, swWindow window);
extern void swZoomFullOutAmplitudeCB(spComponent component, swWindow window);
extern void swScrollAmplitudeCB(spComponent component, swWindow window);
extern spBool swScrollWindowEx(swWindow window,
			       int direction/* -1: prev, 0: use offset, 1: next */,
			       spLong offset, spBool move_cursor, spBool disable_play_position_update,
			       spBool in_thread);
extern spBool swScrollWindow(swWindow window, spLong offset, spBool move_cursor, spBool in_thread);
extern void swScrollCB(spComponent component, swWindow window);
extern void swForwardCB(spComponent component, swWindow window);
extern void swBackwardCB(spComponent component, swWindow window);
extern void swGoHeadCB(spComponent component, swWindow window);
extern void swGoTailCB(spComponent component, swWindow window);
extern void swNextWindowCB(spComponent component, swWindow window);
extern void swPrevWindowCB(spComponent component, swWindow window);

#if defined(MACOS)
#pragma import off
#endif

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration */
#endif

#endif /* __SWDRAW_H */
