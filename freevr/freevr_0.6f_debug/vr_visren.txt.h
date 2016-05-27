/* ======================================================================
 *
 * HH   HH         vr_visren.txt.h
 * HH   HH         Author(s): Bill Sherman
 * HHHHHHH         Created: September 19, 2011 (based on vr_visren.glx.h)
 * HH   HH         Last Modified: September 19, 2011
 * HH   HH
 *
 * Header file for FreeVR visual rendering into TXT windows.
 *
 * Copyright 2011, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#ifndef __VRVISREN_TXT_H__
#define __VRVISREN_TXT_H__

#include <stdio.h>

#if !defined(TEST_APP) /* { */
#include "vr_visren.h"
#endif /* } !TEST_APP */


#if 0 /* Unlikely that this is needed for the TXT interface */
/**********************************************/
/*** Defined values for X-windows interface ***/

/* these are used to indicate what window decorations should be rendered */
#define	DECORATION_TITLE	0x01
#define	DECORATION_BORDER	0x02
#define	DECORATION_MINMAX	0x04
#define	DECORATION_ALL		0xFF

/* these are for specifying the background and icon bitmap files */
#define	XBM_BACK_FILE	"freevr_back.xbm"
#define	XBM_BACK_BITS	freevr_logo_bits
#define	XBM_BACK_WIDTH	freevr_logo_width
#define	XBM_BACK_HEIGHT	freevr_logo_height
#define	XBM_ICON_FILE	"freevr_icon.xbm"
#define	XBM_ICON_BITS	freevr_icon_bits
#define	XBM_ICON_WIDTH	freevr_icon_width
#define	XBM_ICON_HEIGHT	freevr_icon_height
#endif



/**************************************************/
/* TXT-specific data (private to each TXT window) */
/*   This structure is pointed to by the "aux_data" field of the vrWindowInfo structure.*/
/* NOTE: typically this would go directly in the vr_visren.txt.c source file, but       */
/*   in this case this structure is also required by vr_visren.pf.c, and the            */
/*   vr_input.xwindows.c code.                                                          */
typedef struct {

		char		*window_display;/* CONFIG-arg: string to name the window */
		FILE		*window_fp;	/* file-pointer to the terminal */
		int		mapped;		/* a boolean to indicate that the window is mapped */
	} vrTxtPrivateInfo;


/*****************************/
/*** Function declarations ***/

/* TODO: for OSG, determine whether I need to enclose all the functions in an 'extern "C" { ... }' bracket */
#ifdef __cplusplus
extern "C" {
#endif

void	vrTxtInitWindowInfo(vrWindowInfo *);

/* locally scoped functions (ie. used only in vr_visren.txt.c) */
/* TODO: question: so why are these here?   They probably should only be in vr_visren.txt.c */
static	void	_TxtRenderFunc(vrRenderInfo *renderinfo);
static	void	_TxtRenderNullWorld();
#if 0
static	void	_TxtRenderDefaultSimulator(vrRenderInfo *renderinfo, int mask);
#endif
static	void	_TxtRenderText(vrRenderInfo *renderinfo, char *string);

#endif

#ifdef __cplusplus
}
#endif

