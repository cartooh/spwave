/*
 *	swCursor.h
 */

#ifndef __SWCURSOR_H
#define __SWCURSOR_H

#include "swWindow.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(MACOS)
#pragma import on
#endif

extern spBool swGetCallbackMousePosition(spComponent component, swWindow window, spBool overview_flag, int *x, int *y);
extern double swCustomXMinLogValue(DVector custom_x_axis);
extern spLong swXValueToCustomXIndex(DVector custom_x_axis, spLong start_pos, spLong end_pos, double x_value, spBool log_flag, double *o_index_d);
extern int swSampToOverviewDisp(swWindow window, spLong samp);
extern int swSampToDrawWidth(swWindow window, int draw_width, double samp);
extern int swSampToDisp(swWindow window, spLong samp);
extern spLong swOverviewDispToSamp(swWindow window, int disp);
extern double swTargetSampToDim(swWindow window, swWave wave, spBool to_draw, spLong samp);
extern spLong swDrawWidthToSamp(swWindow window, int draw_width, int disp);
extern spLong swDispToSamp(swWindow window, int disp);
extern double swSampToDim(swWindow window, spLong samp);
extern double swTargetSampToCurrentDim(swWindow window, swWave wave, spLong samp);
extern double swSampToCurrentDim(swWindow window, spLong samp);
extern spLong swDimToTargetSamp(swWindow window, swWave wave, double dim);
extern double swDimToFractionalTargetSamp(swWindow window, swWave wave, double dim);
extern spLong swDimToSamp(swWindow window, double dim);
extern int swDimToDrawWidth(swWindow window, int draw_width, double dim);
extern int swDimToDisp(swWindow window, double dim);
extern int swLengthToDrawWidth(swWindow window, int draw_width, spLong length);
extern int swLengthToDisp(swWindow window, spLong length);
#if 0
extern spLong swDrawWidthToLength(swWindow window, int draw_width, int disp);
extern spLong swDispToLength(swWindow window, int disp);
#endif
extern spLong swSampToTargetSamp(swWindow window, swWave wave, spLong samp);
extern double swSampToFractionalTargetSamp(swWindow window, swWave wave, spLong samp);
extern spLong swTargetSampToSamp(swWindow window, swWave wave, spLong samp);
extern double swOrderToDim(swWindow window, swWaveSubArea sub_area, long order);
extern long swDimToOrder(swWindow window, swWaveSubArea sub_area, double dim);
extern long swYPosToOrder(swWindow window, swWaveSubArea sub_area, int channel, int y);
extern int swOrderToYPos(swWindow window, swWaveSubArea sub_area, long order);
extern void swUpdatePoint(swWindow window, spLong point, spBool update_flag);
extern void swSetCanvasCursor(spComponent canvas, spCursorType cursor_type);
extern void swSetMouseCursor(swWindow window, spCursorType cursor_type);
extern void swUnsetMouseCursor(swWindow window);
extern int swGetCursorOrder(swWindow window, swWaveSubArea sub_area, int y, long *order);
extern void swMoveAllCursor(swWindow window);
extern spBool swMoveCursor(swWindow window, spLong point, spBool update_flag);
extern void swMoveCursorCB(spComponent component, swWindow window);
extern spBool swPlayRegionEx(swWindow window, spLong st, spLong ed, spLong start_offset);
extern spBool swPlayRegion(swWindow window, spLong st, spLong ed);
extern spBool swPlayStop(swWindow window);
extern void swPlayRegionCB(spComponent component, swWindow window);
extern void swLoopPlayRegionCB(spComponent component, swWindow window);
extern void swPlayWindowCB(spComponent component, swWindow window);
extern void swPlayFileCB(spComponent component, swWindow window);
extern void swRecordRegionCB(spComponent component, swWindow window);
extern void swPlayStopCB(spComponent component, swWindow window);
extern void swPauseCursor(swWindow window);
extern void swPauseCursorCB(spComponent component, swWindow window);
extern void swButtonMotionCB(spComponent component, swWindow window);
extern void swKeyPressCB(spComponent component, swWindow window);
extern void swWheelCB(spComponent component, swWindow window);

#if defined(MACOS)
#pragma import off
#endif

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration */
#endif

#endif /* __SWCURSOR_H */
