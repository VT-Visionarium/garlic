/* ======================================================================
 *
 *  CCCCC          vr_callback.c
 * CC   CC         Author(s): Ed Peters, Bill Sherman, John Stone
 * CC              Created: June 4, 1998
 * CC   CC         Last Modified: October 21, 2005
 *  CCCCC
 *
 * Code file for FreeVR callbacks.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "vr_debug.h"
#include "vr_callback.h"
#include "vr_shmem.h"
#include "vr_visren.h"
#include "vr_objects.h"


#define	PRINT_FIRST_N_CALLBACKS	4
#undef BELOUD				/* define to make No-op callbacks shout-out when they are called */


/**********************************************************************/
char *vrCallbackTypeName(vrFuncType type)
{
	switch (type) {
	case VRFUNC_ONE_DISPLAY_INIT:
		return "DisplayInitOne";
	case VRFUNC_ONE_DISPLAY_EXIT:
		return "DisplayExitOne";
	case VRFUNC_ONE_DISPLAY_FRAME:
		return "DisplayFrameOne";
	case VRFUNC_ONE_DISPLAY:
		return "DisplayOne";
	case VRFUNC_ONE_DISPLAY_SIM:
		return "DisplaySimOne";

	case VRFUNC_ALL_DISPLAY_INIT:
		return "DisplayInit";
	case VRFUNC_ALL_DISPLAY_EXIT:
		return "DisplayExit";
	case VRFUNC_ALL_DISPLAY_FRAME:
		return "DisplayFrame";
	case VRFUNC_ALL_DISPLAY:
		return "Display";
	case VRFUNC_ALL_DISPLAY_SIM:
		return "DisplaySim";

	case VRFUNC_HANDLE_USR2:
		return "HandleUSR2";

	default:
		return "unknown";
	}
}


/*****************************************************************/
void vrFprintCallbackList(FILE *file, vrCallbackList *cblist, vrPrintStyle style)
{
	/* TODO: print different things for different styles */

	/* if null pointer given, print an empty shell and return */
	if (cblist == NULL) {
		vrFprintf(file, "cblist = { <nil> }\n");
		return;
	}

	switch (style) {
	case one_line:
		break;

	default:
	case verbose:
		vrFprintf(file, "CBlist = {\n");
		vrFprintf(file,
			"\tVisrenInit = %#p (%s)\n\tVisrenFrame = %#p (%s)\n\tVisrenWorld = %#p (%s)\n"
			"\tVisrenSim = %#p (%s)\n\tVisrenExit = %#p (%s)\n"
			"\tHandleUSR2 = %#p (%s)\n",
			cblist->VisrenInit,		cblist->VisrenInit->name,
			cblist->VisrenFrame,		cblist->VisrenFrame->name,
			cblist->VisrenWorld,		cblist->VisrenWorld->name,
			cblist->VisrenSim,		cblist->VisrenSim->name,
			cblist->VisrenExit,		cblist->VisrenExit->name,
			cblist->HandleUSR2,		cblist->HandleUSR2->name);
		vrFprintf(file, "}\n");

		break;
	}
}


/*********************************************************************/
void vrCallbackListInitialize(vrCallbackList *cblist)
{
	if (cblist == NULL) {
		vrErrPrintf(RED_TEXT "vrCallbackListInitialize(): NULL callback list pointer.\n" NORM_TEXT);
		return;
	}

	/* assign callbacks to basic null operations */
#ifdef BELOUD
	cblist->VisrenInit = vrCallbackCreateNamed("Default:VisrenInit-DN", vrDoNothingLoud, 0);
#else
	cblist->VisrenInit = vrCallbackCreateNamed("Default:VisrenInit-DN", vrDoNothing, 0);
#endif
	cblist->VisrenExit = vrCallbackCreateNamed("Default:VisrenExit-DN", vrDoNothing, 0);
	cblist->VisrenFrame = vrCallbackCreateNamed("Default:VisrenFrame-DN", vrDoNothing, 0);
	//cblist->VisrenWorld = vrCallbackCreateNamed("Default:VisrenWorld-NullWorld", vrRenderNullWorld, 0);
#ifdef BELOUD
	cblist->VisrenWorld = vrCallbackCreateNamed("Default:VisrenWorld-NullWorld", vrDoNothingLoud, 0);
#else
	cblist->VisrenWorld = vrCallbackCreateNamed("Default:VisrenWorld-NullWorld", vrDoNothing, 0);
#endif
	cblist->VisrenSim = vrCallbackCreateNamed("Default:VisrenSim-DefSim", vrRenderDefaultSimulator, 0);
	cblist->HandleUSR2 = vrCallbackCreateNamed("Default:HandleUSR2-DN", vrDoNothing, 0);
}


/*****************************************************************/
void vrFprintCallback(FILE *file, vrCallback *callback, vrPrintStyle style)
{
	/* TODO: print different things for different styles */

	/* if null pointer given, print an empty shell and return */
	if (callback == NULL) {
		vrFprintf(file, "Callback = { <nil> }\n");
		return;
	}

	switch (style) {
	case one_line:

	default:
	case verbose:
		vrFprintf(file, "{\n");
		vrFprintf(file, "\r"
			"\tObject_type = %s (%d)\n\tid = %d\n\tname = \"%s\"\n"
			"\tmalleable = %d\n\tnext = %#p\n"
			"\tCreated at %s, line %d\n"
			"\tLast modified at %s, line %d\n",
			vrObjectTypeName(callback->object_type),
			callback->object_type,
			callback->id,
			callback->name,
			callback->malleable,
			callback->next,
			callback->file_created,
			callback->line_created,
			callback->file_lastmod,
			callback->line_lastmod);
		vrFprintf(file,
			"\r\tinvoked = %d\n\tsets = %d\n\tunsets = %d\n",
			callback->invocations,
			callback->set_refcount,
			callback->unset_refcount);
		vrFprintf(file,
			"\tfunc = %#p\n\targc = %d\n"
			"\targv[] = [ %#p %#p %#p %#p %#p ]\n",
			callback->func,
			callback->argc,
			callback->argv[0],
			callback->argv[1],
			callback->argv[2],
			callback->argv[3],
			callback->argv[4]);
		vrFprintf(file, "}\n");

		break;
	}
}


/*********************************************************************/
/* allocate a callback out of shared memory (returns NULL on error)  */
/*********************************************************************/
vrCallback *vrCallbackCreate(void func(), int argc,...)
{
	int		count = 0;
	vrCallback	*callback;
	char		string[128];

	callback = (vrCallback *)vrObjectNew(vrContext, VROBJECT_CALLBACK, "<unnamed>");
	sprintf(string, "%#p", callback);
	callback->name = vrShmemStrDup(string);

	if (!callback) {
		vrErr("couldn't allocate callback");
		return NULL;
	}

	callback->invocations = 0;
	callback->set_refcount = 0;
	callback->unset_refcount = 0;

	callback->func = (void *)func;
	callback->argc = argc;
	for (count = 0; count < 5; count++)
		callback->argv[count] = NULL;

	if (argc > 0) {
		va_list ap;

		va_start(ap, argc);
		for (count = 0; count < argc; count++) {
			callback->argv[count] = va_arg(ap, void *);
		}
		va_end(ap);
	}

	vrDbgPrintfN(CALLBACK_DBGLVL, "vrCallbackCreate(): addr = %#p, name = '%s' func = %#p, argc = %d\n",
		callback, callback->name, callback->func, callback->argc);

	return callback;
}


/*********************************************************************/
/* allocate a callback out of shared memory (returns NULL on error)  */
/* This version takes a name that is given to the callback. */
/*********************************************************************/
vrCallback *vrCallbackCreateNamed(char *name, void func(), int argc,...)
{
	int		count = 0;
	vrCallback	*callback;

	callback = (vrCallback *)vrObjectNew(vrContext, VROBJECT_CALLBACK, name);

#undef NEW_CALLBACK_DEBUG
#ifdef NEW_CALLBACK_DEBUG
vrPrintf("Setting callback '%s', func = %p, argc = %d\n", name, func, argc);
#endif
	if (!callback) {
		vrErr("couldn't allocate named callback");
		return NULL;
	}

	callback->invocations = 0;
	callback->set_refcount = 0;
	callback->unset_refcount = 0;

	callback->func = (void *)func;
	callback->argc = argc;
	for (count = 0; count < 5; count++)
		callback->argv[count] = NULL;

	if (argc > 0) {
		va_list ap;

		va_start(ap, argc);
		for (count = 0; count < argc; count++) {
			callback->argv[count] = va_arg(ap, void *);
		}
		va_end(ap);
#ifdef NEW_CALLBACK_DEBUG
vrPrintf("    argv[0] = %p\n", callback->argv[0]);
#endif
	}

	vrDbgPrintfN(CALLBACK_DBGLVL, "vrCallbackCreateNamed(): addr = %#p, name = '%s' func = %#p, argc = %d\n",
		callback, callback->name, callback->func, callback->argc);

	return callback;
}


/*****************************************************************/
/* A callback is invoked by casting the function pointer into    */
/* one of the right type (based on how many arguments there are) */
/* and calling it.  This function will fail silently.            */
/*****************************************************************/
void vrCallbackInvoke(vrCallback *callback)
{
	int	state_return = 0;	/* flag to print when the function returns */

	/*****************************/
	/** handle degenerate cases **/
	if (callback == NULL) {
		vrDbgPrintfN(CALLBACK_DBGLVL, "vrCallbackInvoke(): Warning, invoked NULL callback\n");
		return;
	}

	if (callback->func == NULL) {
		vrDbgPrintfN(CALLBACK_DBGLVL, "vrCallbackInvoke(): Warning, invoked callback with NULL function\n");
		return;
	}

	/***********************************/
	/** handle the invocation counter **/
	if ((callback->invocations == 0) && vrDbgDo(CALLBACK_DBGLVL) || vrDbgDo(CALLBACK_DETAIL_DBGLVL)) {
		vrPrintf("vrCallbackInvoke(): First invocation of callback '%s' (%#p)\n", callback->name, callback);
		state_return = 1;
	} else if ((callback->invocations < PRINT_FIRST_N_CALLBACKS) && vrDbgDo(CALLBACK_DBGLVL) || vrDbgDo(CALLBACK_DETAIL_DBGLVL)) {
		vrPrintf("vrCallbackInvoke(): %dth invocation of callback '%s' (%#p)\n", callback->invocations+1, callback->name, callback);
		state_return = 1;
	}

	callback->invocations++;

	/******************************************************/
	/** call the function (based on number of arguments) **/
	switch (callback->argc) {

	case 0: {
		void (*func)() = (void (*)())callback->func;

		func();
		break;
	}
	case 1: {
		void (*func)(void *) = (void (*)(void *))callback->func;

		func(callback->argv[0]);
		break;
	}
	case 2: {
		void (*func)(void *, void *) = (void (*)(void *, void *))callback->func;

		func(callback->argv[0], callback->argv[1]);
		break;
	}
	case 3: {
		void (*func)(void *, void *, void *) = (void (*)(void *, void *, void *))callback->func;

		func(callback->argv[0], callback->argv[1], callback->argv[2]);
		break;
	}
	case 4: {
		void (*func)(void *, void *, void *, void *) = (void (*)(void *, void *, void *, void *))callback->func;

		func(callback->argv[0], callback->argv[1], callback->argv[2], callback->argv[3]);
		break;
	}
	case 5: {
		void (*func)(void *, void *, void *, void *, void *) = (void (*)(void *, void *, void *, void *, void *))callback->func;

		func(callback->argv[0], callback->argv[1], callback->argv[2], callback->argv[3], callback->argv[4]);
		break;
	}
	default:
		/* NOTE: this should never happen, but we'll be pedantic. */
		vrErrPrintf(RED_TEXT "vrCallbackInvoke: Too many arguments for callback '%s' -- %d.\n" NORM_TEXT, callback->name, callback->argc);
		break;

	}

	if (state_return)
		vrPrintf("vrCallbackInvoke(): Done with %dth invocation of callback '%s' (%#p)\n", callback->invocations, callback->name, callback);

	return;
}


/*****************************************************************/
/* A callback is invoked by casting the function pointer into    */
/* one of the right type (based on how many arguments there are) */
/* and calling it.  A dynamic callback might have more arguments */
/* at call-time than were specified at creation.  This function  */
/* will fail silently.                                           */
/*****************************************************************/
void vrCallbackInvokeDynamic(vrCallback *callback, int argc, ...)
{
	int	total_argc = 0;		/* total number of arguments after new and old are combined */
	void	*new_argv[6];		/* an array to store the new argument list */
	void	**total_argv;		/* a pointer to an array (new or old) with an argument list */
	int	allocated = 0;		/* whether or not the argument list was created from shared memory */
	int	state_return = 0;	/* flag to print when the function returns */

	/*****************************/
	/** handle degenerate cases **/
	if (callback == NULL) {
		vrErrPrintf(RED_TEXT "vrCallbackInvokeDynamic: NULL callback\n" NORM_TEXT);
		return;
	}
	if (callback->func == NULL) {
		vrErrPrintf(RED_TEXT "vrCallbackInvokeDynamic: NULL callback function\n" NORM_TEXT);
		return;
	}
	if (argc + callback->argc > 5) {
		vrErrPrintf(RED_TEXT "vrCallbackInvokeDynamic: too many callback arguments (%d)\n" NORM_TEXT, argc + callback->argc);
		return;
	}

	/**********************************/
	/** append the dynamic arguments **/
	if (argc > 0) {
		int	total_argv_count = 0;
		int	new_argv_count = 0;
		va_list	ap;

		total_argc = argc + callback->argc;
		total_argv = new_argv;

		for (total_argv_count = 0; total_argv_count < callback->argc; total_argv_count++)
			total_argv[total_argv_count] = callback->argv[total_argv_count];

		va_start(ap, argc);
		for (new_argv_count = 0; new_argv_count < argc; new_argv_count++)
			total_argv[total_argv_count++] = va_arg(ap, void *);	/* NOTE: it's total_argv_count++ because we're adding new arguments */
										/*   in addition to the ones from the above loop.    */

		va_end(ap);
	} else {
		total_argv = callback->argv;
	}

	/***********************************/
	/** handle the invocation counter **/
	if ((callback->invocations == 0) && vrDbgDo(CALLBACK_DBGLVL) || vrDbgDo(CALLBACK_DETAIL_DBGLVL)) {
		vrPrintf("vrInvokeDynamiCB(): First invocation of callback '%s' (%#p)\n", callback->name, callback);
		state_return = 1;
	} else if (vrDbgDo(CALLBACK_DETAIL_DBGLVL) || (vrDbgDo(CALLBACK_DBGLVL) && (callback->invocations < PRINT_FIRST_N_CALLBACKS))) {
		vrPrintf("vrInvokeDynamiCB(): %dth invocation of callback '%s' (%#p)\n", callback->invocations+1, callback->name, callback);
		state_return = 1;
	}

	callback->invocations++;

	/******************************************************/
	/** call the function (based on number of arguments) **/
	switch (total_argc) {

	case 0: {
		void (*func)() = (void (*)())callback->func;

		func();
		break;
	}
	case 1: {
		void (*func)(void *) = (void (*)(void *))callback->func;

		func(total_argv[0]);
		break;
	}
	case 2: {
		void (*func)(void *, void *) = (void (*)(void *, void *))callback->func;

		func(total_argv[0], total_argv[1]);
		break;
	}
	case 3: {
		void (*func)(void *, void *, void *) = (void (*)(void *, void *, void *))callback->func;

		func(total_argv[0], total_argv[1], total_argv[2]);
		break;
	}
	case 4: {
		void (*func)(void *, void *, void *, void *) = (void (*)(void *, void *, void *, void *))callback->func;

		func(total_argv[0], total_argv[1], total_argv[2], total_argv[3]);
		break;
	}
	case 5: {
		void (*func)(void *, void *, void *, void *, void *) = (void (*)(void *, void *, void *, void *, void *))callback->func;

		func(total_argv[0], total_argv[1], total_argv[2], total_argv[3], total_argv[4]);
		break;
	}
	default:
		vrErrPrintf(RED_TEXT "vrCallbackInvokeDynamic: Too many arguments for callback '%s' -- %d.\n" NORM_TEXT, callback->name, total_argc);
		break;

	}

	if (state_return)
		vrPrintf("vrInvokeDynamiCB(): Done with %dth invocation of callback '%s' (%#p)\n", callback->invocations, callback->name, callback);

	if (allocated)
		vrShmemFree(total_argv);
	return;
}


/************************************************************************/
/* vrFunctionSetCallback(): sets global or window-specific function pointers        */
/* If you're setting a global function (*ALL* in the list above),       */
/* vrFunctionSetCallback takes a single pointer to the vrCallback for that          */
/* function.  If you're setting a window-specific function (*ONE*       */
/* in the list above), it takes a pointer to the window and a           */
/* pointer to the callback, in that order.                              */
/* For example:                                                         */
/*   vrCallback *cb = vrCallbackCreate (my_func, 1, data);              */
/*   vrCallback *cb2 = vrCallbackCreate (my_func, 1, other_data);       */
/*   vrFunctionSetCallback (VRFUNC_ALL_DISPLAY, cb);                                */
/*   vrFunctionSetCallback (VRFUNC_ONE_DISPLAY, vrContext->config->windows[0], cb2);*/
/*                                                                      */
/* (NOTE: thus far only callback assignments are possible options)      */
/*                                                                      */
/* TODO: we may want to have vrContext passed as an argument.           */
/************************************************************************/
void vrFunctionSetCallback(vrFuncType type,...)
{
	va_list		ap;
	vrCallbackList	*cblist;
	vrCallback	*callback = NULL;
	vrCallback	*previous_cb = NULL;
	vrWindowInfo	*window = NULL;

	cblist = vrContext->callbacks;

	va_start(ap, type);

	switch (type) {

	/*****************************************************************/
	/* These five options set global function pointers.  (ie. all    */
	/* windows that are not assigned their own individual callback   */
	/* will use the global callback instead.                         */
	/* Global callback assignment operations only take one arg -- a  */
	/* pointer to the callback                                       */

	case VRFUNC_ALL_DISPLAY_INIT: {
		callback = va_arg(ap, vrCallback *);

		previous_cb = cblist->VisrenInit;
		cblist->VisrenInit = callback;
		break;
	}

	case VRFUNC_ALL_DISPLAY_FRAME: {
		callback = va_arg(ap, vrCallback *);

		previous_cb = cblist->VisrenFrame;
		cblist->VisrenFrame = callback;
		break;
	}

	case VRFUNC_ALL_DISPLAY: {
		callback = va_arg(ap, vrCallback *);

		previous_cb = cblist->VisrenWorld;
		cblist->VisrenWorld = callback;
		break;
	}

	case VRFUNC_ALL_DISPLAY_SIM: {
		callback = va_arg(ap, vrCallback *);

		previous_cb = cblist->VisrenSim;
		cblist->VisrenSim = callback;
		break;
	}

	case VRFUNC_ALL_DISPLAY_EXIT: {
		callback = va_arg(ap, vrCallback *);

		previous_cb = cblist->VisrenExit;
		cblist->VisrenExit = callback;
		break;
	}


	/*******************************************************************/
	/* These five set the function pointers for a particular window,   */
	/* so, the callback assigned to just one window will override the  */
	/* globally set callback.  Window specific callbacks take two      */
	/* arguments a pointer to the window and a pointer to the callback */
	case VRFUNC_ONE_DISPLAY_INIT: {
		window = va_arg(ap, vrWindowInfo *);
		callback = va_arg(ap, vrCallback *);

		window->VisrenInit = callback;
		break;
	}

	case VRFUNC_ONE_DISPLAY_FRAME: {
		window = va_arg(ap, vrWindowInfo *);
		callback = va_arg(ap, vrCallback *);

		window->VisrenFrame = callback;
		break;
	}

	case VRFUNC_ONE_DISPLAY: {
		window = va_arg(ap, vrWindowInfo *);
		callback = va_arg(ap, vrCallback *);

		window->VisrenWorld = callback;
		break;
	}

	case VRFUNC_ONE_DISPLAY_SIM: {
		window = va_arg(ap, vrWindowInfo *);
		callback = va_arg(ap, vrCallback *);

		window->VisrenSim = callback;
		break;
	}

	case VRFUNC_ONE_DISPLAY_EXIT: {
		window = va_arg(ap, vrWindowInfo *);
		callback = va_arg(ap, vrCallback *);

		window->VisrenExit = callback;
		break;
	}

	/********************************************************/
	/* this(ese) function(s) are for handling signal events */

	case VRFUNC_HANDLE_USR2: {
		callback = va_arg(ap, vrCallback *);

		cblist->HandleUSR2 = callback;
		break;
	}


	}

	va_end(ap);

	/*******************************/
	/* update the reference counts */
	if (callback != NULL)
		callback->set_refcount++;
	if (previous_cb != NULL)
		previous_cb->unset_refcount++;

	/*****************************/
	/* print some debugging info */
	if (window == NULL) {
		/* TODO: check for NULL values of callback */
		vrDbgPrintfN(CALLBACK_DBGLVL, "vrFunctionSetCallback(): Set global callback '%s' (%#p) for type %s (%d) -- prev was '%s' (%#p)\n",
			(callback ? callback->name : "<nil>"), callback, vrCallbackTypeName(type), type,
			(previous_cb ? previous_cb->name : "<nil>"), previous_cb);
	} else {
		vrDbgPrintfN(CALLBACK_DBGLVL, "vrFunctionSetCallback(): Set window '%s' callback '%s' (%#p) for type %s (%d) -- prev was '%s' (%#p)\n",
			window->name,
			(callback ? callback->name : "<nil>"), callback, vrCallbackTypeName(type), type,
			(previous_cb ? previous_cb->name : "<nil>"), previous_cb);
	}
}


/********************************************************************/
/* A short hand for calling vrFunctionSetCallback(type, vrCallbackCreateNamed(char *name, void func(), int argc,...)) */
void vrCallbackSet(vrFuncType type, char *name, void func(), int argc, ...)
{
	va_list		ap;
	vrCallback	*callback;

#ifdef NEW_CALLBACK_DEBUG
vrPrintf("vrCallbackSet called: type = %d, name = '%s', func = %p, argc = %d\n", type, name, func, argc);
#endif
#if 0
	va_start(ap, argc);
	vrPrintf("hmmm, *ap = %p\n", *ap);
	va_end(ap);
#endif
	va_start(ap, argc);
	callback = vrCallbackCreateNamed(name, func, argc, ap);
	va_end(ap);

	vrFunctionSetCallback(type, callback);
}


/********************************************************************/
/* vrCallbackUpdate(): checks whether a new (replacement) callback  */
/*   has recently been set, and if so pushes it onto the top of the */
/*   given callback stack.                                          */
/* NOTE: if new callback is NULL, nothing happens, otherwise new    */
/*   callback is pushed on the stack.                               */
/* '1' is returned when a new callback is assigned, otherwise '0'   */
/*   is returned.                                                   */
/********************************************************************/
int vrCallbackUpdate(vrCallback **origcb, vrCallback **newcb)
{
	if (*newcb == NULL)
		return 0;

	if (*origcb == NULL) {
		vrErrPrintf("vrCallbackUpdate(): Hmmm, encountered a NULL callback in the list -- it is uncertain whether this should happen.\n");
	}

#if 1 /* this is a temporary version of this function while working out the "rending" bug, however, it may be the way of the future. */
	/* TODO: see if we need to adjust the callback refcounts here */
	if (*origcb != *newcb) {
		/* We have a new callback! */
		vrMsgPrintf("FYI: vrCallbackUpdate(%#p, %#p) called, orig cb = %#p, new cb = %#p\n", *origcb, *newcb, *(*origcb), *(*newcb));

		/* TODO: if we're going to do the linked list thing, here might be where to start */
		*origcb = *newcb;
		return 1;
	}
	return 0;
#else

	/* make the original callback the next node of the new callback */
	(*newcb)->next = *origcb;

	/* move the new callback in place of the old, and remove the old new */
	*origcb = *newcb;
	*newcb = NULL;

	return 1;
#endif
}


/********************************************************************/
/* vrPopCallback(): checks whether there are any callbacks below    */
/*   the given callback, and if so pops the given callback from the */
/*   stack replacing it with the next callback on the list.         */
/* Returns '1' if a pop operation was performed, '0' if not.        */
/********************************************************************/
int vrPopCallback(vrCallback **origcb)
{
	vrCallback	*oldcb;		/* the old original callback */

#if 0 /* TODO: delete this */
	vrMsgPrintf("FYI: vrPopCallback(%#p) called\n", *origcb);
#endif
	if (*origcb == NULL) {
		vrErrPrintf("vrPopCallback(): Hmmm, encountered a NULL callback in the list -- it is uncertain whether this should happen.\n");
	}

	if ((*origcb)->next == NULL)
		return 0;

	oldcb = *origcb;
	*origcb = oldcb->next;

	/* TODO: delete the old callback to save some memory */

	return 1;
}


/*********************************************************************/
/* vrDoNothing(): A function that doesn't do anything.  For use with */
/*   callbacks when the desired function isn't assigned yet, or the  */
/*   callback shouldn't do anything.                                 */
/*********************************************************************/
void vrDoNothing() {}
void vrDoNothingLoud()
{
	vrPrintf("doing nothing\n");
}

