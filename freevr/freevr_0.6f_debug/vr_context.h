/* ======================================================================
 *
 * HH   HH         vr_context.h
 * HH   HH         Author(s): Bill Sherman
 * HHHHHHH         Created: June 21, 2001
 * HH   HH         Last Modified: April 3, 2012
 * HH   HH
 *
 * Header file for FreeVR context information.  Defines the
 * vrContext info structure (which contains pointers to the other
 * primary info structures -- input & config), which is the
 * single entry point to context configuration.
 *
 * Copyright 2015, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#ifndef __VRCONTEXT_H__
#define __VRCONTEXT_H__

#define FREEVR_VERSION_MAJOR	0
#define FREEVR_VERSION_MINOR	6
#define FREEVR_VERSION_SUB	f
#define FREEVR_VERSION		000606		/* integer encoding for 0.6f */

#ifdef MP_THREADS2
#  include <pthread.h>
#endif
#include "vr_shmem.h"		/* for vrLock type definition, includes vr_enums.h */

typedef struct vrContextInfo_st {
		/***************************/
		/* Generic Object field(s) */
		vrObject	object_type;	/* The type of this object (ie. VROBJECT_CONTEXT) */
		/* NOTE: the rest of the generic object fields not included with individual inputs*/
		/* TODO: consider adding the other object fields to the input structures.         */

		/***************************/
		char		*version;	/* specifies which version of the library this is */
		char		*arch;		/* specifies the architecture (and abi) of compilation */
		char		*compile;	/* specifies information about the compile-options used for this library */
		char		*compile_target;/* specifies the Makefile target given (assuming the Makefile cooperates by setting the ARCH variable to this value) */
		char		*homedir;	/* specifies the home directory under which system FreeVR files can be found */
		char		*name;		/* specifies the name of the application (by application programmer) */
		char		*authors;	/* specifies the authors of the application */
		char		*extra_info;	/* specifies any extra information about the app that the author(s) want to provide */
		char		*status_info;	/* specifies any information about the app's status that the authors provide -- intended to be changed as the application runs */

		size_t		shmem_size;	/* Must be declared prior to reading config */
	struct vrShmemInfo_st	*shmem;		/* Information about the shared memory system   TODO: use this. */

		unsigned int	startup_error;	/* Flags for any problems encountered during startup of the library */
		vrSystemStatus	status;		/* The status of the FreeVR system (of this context) */
		unsigned int	paused;		/* Flag indicating system is/should-be paused */

		vrTime		time_immemorial;/* the time when vrStart was called         */
		vrTime		paused_time;	/* the amount of time spent paused          */
		vrTime		pause_wtime;	/* the time at which pausing began          */

	struct vrConfigInfo_st	*config;	/* pointer to the configuration structure   */
	struct vrInputInfo_st	*input;		/* pointer to the inputs structure          */
	struct vrObjectLists_st	*object_lists;	/* pointer to the possible config objects   */
	struct vrCallbackList_st *callbacks;	/* pointer to the system callbacks          */
		vrLock		head_lock;	/* pointer to the head of the list of locks */
		vrLock		tail_lock;	/* pointer to the tail of the list of locks */
#  if defined(SEM_SYSVIPC)
		int		lock_semset;	/* the current value of SYSVIPC semaphore set (initialize to NULL) */
		int		lock_semnum;	/* the next semaphore number to assign (initialize to  0) */
#  endif
	struct vrBarrier_st	*head_barrier;	/* pointer to the head of the list of barriers */
	struct vrBarrier_st	*tail_barrier;	/* pointer to the tail of the list of barriers */
		vrLock		barrier_lock;	/* to prevent simultaneous modification of the list */

		vrLock		print_lock;	/* a lock to keep printf's from overwriting */
		vrLock		xpixmap_lock;	/* a lock to protect possible thread-unsafe code in X11 */

#ifdef MP_PTHREADS2
		pthread_key_t	this_proc_key;	/* a key to access a local pointer to each proc's/thread's proc_info struct */
#endif

	} vrContextInfo;

#if 1 /* if would be preferable if a global scoped variable wasn't necessary. */
	/*  currently required by vr_debug.c and vr_math.c::vrStartTime().  */
extern vrContextInfo *vrContext;
#endif

#endif /* __VRCONTEXT_H__ */
