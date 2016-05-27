/* ======================================================================
 *
 * HH   HH         vr_callback.h
 * HH   HH         Author(s): Ed Peters, Bill Sherman, John Stone
 * HHHHHHH         Created: June 4, 1998
 * HH   HH         Last Modified: October 13, 2003
 * HH   HH
 *
 * Header file for FreeVR callbacks.
 *
 * A vrCallback is a function which returns no value and has up to 5
 * parameters, each of which is interpreted as being of type (void*).
 * The five-parameter limit has to be hard-coded, because of the way
 * the function is invoked.  Failure to create a callback results in
 * a NULL return value; failure to invoke a callback happens silently,
 * since these things are invoked so frequently.
 *
 * Added 7/15/1998 -- you can now specify additional arguments to a
 * callback function at call-time.  This was specifically added to
 * support the callbacks for window, tracker and controller-specific
 * data structures.  See vr_input.* and vr_visren.* for examples of
 * how this is useful.
 *
 * There are many different system callbacks in the FreeVR library.
 * They are enumerated in the vrFuncType enumeration, and can be
 * set generically using the vrSetFunctionCallback routine.
 *
 * Copyright 2014, Bill Sherman & Friends, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#ifndef __VRCALLBACK_H__
#define __VRCALLBACK_H__

#include <stdarg.h>
#include "vr_enums.h"	/* for vrObject type */

#ifdef __cplusplus
extern "C" {
#endif


/**************************************************************************/
/* different possible callbacks -- display (eye) function (called once    */
/* per eye per frame per window), frame function (called once per frame   */
/* per window), and so forth.  They can be set individually or all in one */
/* fell swoop.                                                            */
typedef enum vrFuncType_en {
		VRFUNC_ONE_DISPLAY_INIT,
		VRFUNC_ONE_DISPLAY_EXIT,
		VRFUNC_ONE_DISPLAY_FRAME,
		VRFUNC_ONE_DISPLAY,
		VRFUNC_ONE_DISPLAY_EYE = VRFUNC_ONE_DISPLAY,
		VRFUNC_ONE_DISPLAY_SIM,

		VRFUNC_ALL_DISPLAY_INIT,
		VRFUNC_ALL_DISPLAY_EXIT,
		VRFUNC_ALL_DISPLAY_FRAME,
		VRFUNC_ALL_DISPLAY,
		VRFUNC_ALL_DISPLAY_EYE = VRFUNC_ALL_DISPLAY,
		VRFUNC_ALL_DISPLAY_SIM,

		VRFUNC_DISPLAY_INIT = VRFUNC_ALL_DISPLAY_INIT,
		VRFUNC_DISPLAY_EXIT = VRFUNC_ALL_DISPLAY_EXIT,
		VRFUNC_DISPLAY_FRAME = VRFUNC_ALL_DISPLAY_FRAME,
		VRFUNC_DISPLAY = VRFUNC_ALL_DISPLAY,
		VRFUNC_DISPLAY_EYE = VRFUNC_ALL_DISPLAY,
		VRFUNC_DISPLAY_SIM = VRFUNC_ALL_DISPLAY_SIM,

		VRFUNC_HANDLE_USR2
	} vrFuncType;


/**************************************************************************/
/* Basic callbacks type definition: This stores a pointer to the function */
/*   and up to 5 arguments that will be passed to the function.           */
/*   A different "next" pointer would be necessary to have the possibility*/
/*   of a stack of callbacks.                                             */
/* The vrCallback structure is an extension of the vrObjectInfo structure,*/
/*   so most functions that operate on any type of vrObject can be used   */
/*   on vrCallback's.                                                     */
typedef struct vrCallback_st {
		/* Generic Object fields */
		vrObject	object_type;	/* really type vrObject */
		int		id;
		char		*name;
		int		malleable;
		void		*next;
	struct vrContextInfo_st	*context;	/* pointer to the vrContext memory structure */
		int		line_created;	/* line number of where this object was created in config */
		int		line_lastmod;	/* line number of last time this object modified in config */
		char		file_created[512];/* file (or other) where this object was created*/
		char		file_lastmod[512];/* file (or other) where this object was created*/

		/* callback specific fields */
		void		*func;		/* pointer to the function to call */
		int		argc;		/* number of stored arguments      */
		void		*argv[5];	/* array of up to five arguments   */
		int		invocations;	/* # of times this callback called */
		int		set_refcount;	/* # of times set by vrSetFunctionCallback()   */
		int		unset_refcount;	/* # of times unset by vrSetFunctionCallback() */
	} vrCallback;


/********************************************************************/
/* Configuration information structure -- contains pointers to most */
/* all the configurable info structures of the system.  Only one of */
/* these is (usually) ever created.                                 */
typedef struct vrCallbackList_st {

		vrCallback	*VisrenInit;	/* cb for initialization of the visual rendering */
		vrCallback	*VisrenExit;	/* cb for closing down a visual rendering stream */
		vrCallback	*VisrenFrame;	/* cb for the pre-rendering per-frame commands   */
		vrCallback	*VisrenWorld;	/* cb to visually render the world.              */
		vrCallback	*VisrenSim;	/* cb to visually render simulator controls      */

		vrCallback	*HandleUSR2;	/* cb for what to do upon receiving a USR2 signal */

	} vrCallbackList;


void		vrCallbackListInitialize(vrCallbackList *cblist);
#ifdef __cplusplus
vrCallback	*vrCallbackCreate(...);
vrCallback	*vrCallbackCreateNamed(...);
void		vrCallbackSet(...);
#else
vrCallback	*vrCallbackCreate(void(), int argc, ...);
vrCallback	*vrCallbackCreateNamed(char *,void(), int argc, ...);
void		vrCallbackSet(vrFuncType type, char *name, void(), int argc, ...);
#endif
void		vrFunctionSetCallback(vrFuncType, ...);
int		vrCallbackUpdate(vrCallback **origcb, vrCallback **newcb);

void		vrCallbackInvoke(vrCallback *callback);
void		vrCallbackInvokeDynamic(vrCallback *callback, int argc, ...);
void		vrDoNothing();
void		vrDoNothingLoud();


#ifdef __cplusplus
}
#endif

#endif
