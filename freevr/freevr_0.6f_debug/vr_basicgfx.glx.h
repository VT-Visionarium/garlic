/* ======================================================================
 *
 * HH   HH         vr_basicgfx.glx.h
 * HH   HH         Author(s): Bill Sherman
 * HHHHHHH         Created: November 30, 1999
 * HH   HH         Last Modified: September 9, 2001
 * HH   HH
 *
 * Header file for FreeVR basic OpenGL graphics routines.
 *
 * Copyright 2001, Bill Sherman & Friends, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#ifndef __VRBASICGFX_GLX_H__
#define __VRBASICGFX_GLX_H__

#include "vr_visren.h"		/* just for vrRenderInfo type -- only using "eye" field */

#ifdef __cplusplus
extern "C" {
#endif

void	vrShapeGLCube();
void	vrShapeGLCubeOpen();
void	vrShapeGLCubeOutline();
void	vrShapeGLCubePlusOutline();
void	vrShapeGLPyramid();
void	vrShapeGLPyramidOutline();
void	vrShapeGLPyramidPlusOutline();
void	vrShapeGLFloor();
void	vrGLReportAttributes(char *msg, unsigned long reports);
void	vrGLRenderDefaultSimulator(vrRenderInfo *renderinfo, int mask);

#ifdef __cplusplus
}
#endif

#endif /* __VRBASICGFX_GLX_H__ */
