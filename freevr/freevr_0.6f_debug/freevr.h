/* ======================================================================
 *
 * HH   HH         freevr.h
 * HH   HH         Author(s): Ed Peters, Bill Sherman, John Stone
 * HHHHHHH         Created: June 20, 1998
 * HH   HH         Last Modified: June 20, 2006
 * HH   HH
 *
 * Global header file for FreeVR application programs.
 *
 * TODO: We may want to refine (ie. limit) this to include only
 *   particular structures.
 *   [I'm not sure at the moment.]
 *
 * Copyright 2006, Bill Sherman & Friends, All Rights Reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#ifndef __FREEVR_H__
#define __FREEVR_H__

#include "vr_config.h"		/* includes vr_system.h->vr_context.h which includes version defines */
#include "vr_callback.h"
#include "vr_entity.h"
#include "vr_visren.h"
#include "vr_input.h"
#include "vr_math.h"
#include "vr_procs.h"
#include "vr_shmem.h"
#include "vr_debug.h"
#include "vr_utils.h"

/* for versions 0.4f to 0.6z */
#if (FREEVR_VERSION>=000407) && (FREEVR_VERSION<=000626)
#  define vrLockCreate	vrNCLockCreate
#  ifdef __cplusplus
extern "C" {
#  endif
vrLock	vrNCLockCreate();
#  ifdef __cplusplus
}
#  endif
#endif


/* TODO: why isn't vr_visren.glx.h included for the normal OpenGL version?      */
/*   or a better question is why is Performer here?  Hmmm, I guess it's because */
/*   Performer (and OSG) have a handful of extra functions for the application  */
/*   developer to use.                                                          */
#ifdef GFX_PERFORMER
#include "vr_visren.pf.h"
#endif

#ifdef GFX_OSG
#include "vr_visren.glx.h"		/* This file is shared by OpenGL & OSG */
#endif


/* NOTE: unfortunately there does not seem to be a way to get the filename */
/*   of the file that includes this "freevr.h" file.  The __FILE__ value   */
/*   will always report "freevr.h", which isn't very useful.               */
static	const char	*vrApplicationVersion = FREEVRVERSION
		" -- application compiled " __DATE__ " at " __TIME__
#ifdef HOST_NAME
		" on " HOST_NAME
#endif
#ifdef HOST
		" on " HOST
#endif
#ifdef ARCH
		"[" ARCH "]"
#endif
	"";

/* this function is here just to force the application */
/*   string to be included in the object file.         */
static void vrUnusedFunction()
{
	vrApplicationVersion++;		/* this line is to keep the compiler from printing a warning about an unused variable */
	vrUnusedFunction();		/* this line is to keep compilers from complaining about vrUnusedFunction being defined, but not used */
}

#endif

