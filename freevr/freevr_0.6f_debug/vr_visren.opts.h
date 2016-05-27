/* ======================================================================
 *
 * HH   HH         vr_visren.opts.h
 * HH   HH         Author(s): Bill Sherman
 * HHHHHHH         Created: August 17, 1998
 * HH   HH         Last Modified: September 20, 2011
 * HH   HH
 *
 * Header file for FreeVR visual rendering options
 *
 * Copyright 2014, Bill Sherman & Friends, All rights reserved
 * With the intent to provide an open-source licence to be named later
 * ====================================================================== */
#include "vr_visren.h"


/**************************************************************************/
/* Library programmers: add new rendering options here.                   */
/*                                                                        */
/* To add a new type "T", #include the header file (eg. vr_visren.T.h),   */
/*   and add a new record to the vrVisrenOpts array.  The new record can  */
/*   be placed anywhere before the all NULL record.  The first field of   */
/*   the record is a string representation of the new type, followed by   */
/*   the function for putting the specific window info into the window    */
/*   structure.  This information includes what functions are used to     */
/*   render the scene, render the simulator, render text, perform a       */
/*   transform, etc.                                                      */
/*                                                                        */
/* The function can be named anything, but must take specific arguments:  */
/*   function1(vrWindowInfo *)                                            */
/*                                                                        */
/* The functions return "void".                                           */
/**************************************************************************/

void	vrTxtInitWindowInfo(vrWindowInfo *);
#ifdef WIN_GLX
#if 0
#include "vr_visren.glx.h"
#else
void	vrGlxInitWindowInfo(vrWindowInfo *);
#endif
#endif

#ifdef WIN_WGL
#if 0
#include "vr_visren.wgl.h"
#else
void	vrWGLInitWindowInfo(vrWindowInfo *);
#endif
#endif

vrVisrenOptsType vrVisrenOpts[] = {
		{ "txt", vrTxtInitWindowInfo },		/* fill in info struct for TXT */
#ifdef WIN_GLX
		{ "glx", vrGlxInitWindowInfo },		/* fill in info struct for GLX */
#endif

#ifdef WIN_WGL
		/* NOTE: "glx" is used because really all the window placement stuff */
		/*   is the same for both windowing systems -- it's a bad name.      */
		{ "glx", vrWGLInitWindowInfo },		/* Windows GL */
#endif
		{ NULL, NULL }
	};

