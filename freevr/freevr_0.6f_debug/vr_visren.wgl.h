/* ======================================================================
 *
 * HH   HH         vr_visren.wgl.h
 * HH   HH         Author(s): Sukru Tikves
 * HHHHHHH         Created: August 5, 2002
 * HH   HH         Last Modified: September 9, 2002
 * HH   HH
 *
 * Header file for FreeVR visual rendering into WGL windows.
 *
 * Copyright 2002, Bill Sherman & Friends, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#ifndef __VRVISREN_WGL_H__
#define __VRVISREN_WGL_H__

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>

#include "vr_visren.h"

#ifdef __cplusplus
extern "C" {
#endif

void	vrWGLInitWindowInfo(vrWindowInfo *);
int	vrIsWGLWindow(vrWindowInfo *);

#ifdef WIN32
int	vrWGLRegisterInputHandler(vrWindowInfo *, HWND);
int	vrWGLUnRegisterInputHandler(vrWindowInfo *, HWND);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __VRVISREN_WGL_H__ */

