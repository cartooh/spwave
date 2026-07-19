/*
 *	swLabelList.h
 */

#ifndef __SWLABELLIST_H
#define __SWLABELLIST_H

#include "swWindow.h"
#include "swLabel.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(MACOS)
#pragma import on
#endif

extern void swUpdateInfoAreaSelection(swWindow window);
    
extern swLabelList swCreateLabelList(spComponent parent, swWindow window);
extern spBool swSelectLabelList(swLabelList label_list);
extern spBool swUpdateLabelList(swLabelList label_list);
extern void swUpdateAllLabelList(swWindow window);
    
#if defined(MACOS)
#pragma import off
#endif

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration */
#endif

#endif /* __SWLABELLIST_H */
