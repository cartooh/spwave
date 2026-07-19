/*
 *	swInfoDialog.h
 */

#ifndef __SWINFODIALOG_H
#define __SWINFODIALOG_H

#include "swWindow.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SW_SUPPORT_INFO_DIALOG

typedef struct _swInfoDialog {
    spComponent window;
    spComponent container;
    spComponent container2;
    spComponent container3;
    
    spComponent title_text;
    spComponent artist_text;
    spComponent album_text;
    spComponent album_artist_text;
    
    spComponent composer_text;
    spComponent lyricist_text;
    spComponent producer_text;
    spComponent engineer_text;
    
    spComponent genre_text;
    spComponent id3_version_combo;
    spComponent track_text;
    spComponent track_total_text;
    spComponent disc_text;
    spComponent disc_total_text;
    spComponent release_text;
    spComponent tempo_text;
    
    spComponent copyright_text;
    spComponent source_text;
    spComponent isrc_text;
    spComponent software_text;
    spComponent subject_text;
    spComponent comment_text;
    
    swConfig config;
    unsigned long support_mask;
    unsigned long current_mask;
} *swInfoDialog;

#if defined(MACOS)
#pragma import on
#endif

extern swInfoDialog swCreateInfoDialog(swConfig config);
extern void swPopupInfoDialogCB(spComponent component, swWindow window);
extern void swPopdownInfoDialogCB(spComponent component, swInfoDialog dialog);
    
#if defined(MACOS)
#pragma import off
#endif

#endif

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration */
#endif

#endif /* __SWINFODIALOG_H */
