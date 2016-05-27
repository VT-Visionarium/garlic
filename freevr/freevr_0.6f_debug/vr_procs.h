/* ======================================================================
 *
 * HH   HH         vr_procs.h
 * HH   HH         Author(s): Ed Peters, Bill Sherman, Jeff Stuart
 * HHHHHHH         Created: June 4, 1998
 * HH   HH         Last Modified: March 14, 2006
 * HH   HH
 *
 * Header file for FreeVR process control information.
 *
 * There are currently four main types of processes in a VR simulation:
 *	- visual rendering (window drawing);
 *	- input (polling trackers and controllers) (and can now do output too);
 *	- telnet (interfacing with a running application); and
 *	- computation (everything else).
 *
 * Each rendering process loops calling some user-defined rendering
 * procedures (as well as library code).  Each input procedure
 * loops calling an input update routine.  The base process of any
 * FreeVR application will be a compute process, and others can be
 * spawned off as necessary.
 *
 * The functions in this module are responsible for starting/spawning
 * (forking off, or initiating new pthreads) the various processes,
 * as described in the global configuration structure.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#ifndef __VRPROCS_H__
#define __VRPROCS_H__

#include <stdio.h>
#include <sys/types.h>
#if defined(MP_PTHREADS) || defined(MP_PTHREADS2)
#  include <pthread.h>
#endif

#include "vr_system.h"
#include "vr_shmem.h"

#define VRPROC_NUMTRACEMSGS	40	/* NOTE: increasing this often requires an increase of vr_system.h:VRCONFIG_SHMEM_SIZE */


#ifdef __cplusplus
extern "C" {
#endif


/***************************************************************/
typedef	enum vrProcessType_en {
		VRPROC_NONE,
		VRPROC_NOCONFIG,
		VRPROC_UNKNOWN,
		VRPROC_MAIN,
		VRPROC_PARENT = VRPROC_MAIN,
		VRPROC_TELNET,
		VRPROC_COMPUTE,
		VRPROC_INPUT,
		VRPROC_VISREN,
		VRPROC_AUDREN,
		VRPROC_HAPREN,
		VRPROC_OLFREN,
		VRPROC_TASREN,
		VRPROC_VESREN
	} vrProcessType;


/************************************************************/
/* vrProcessStats: A structure containing time measurements */
/*   for a process.                                         */
/************************************************************/
/* TODO: method of specifying additional horizontal lines (eg. possible monitor syncs) */
/* DONE: consider adding colors for the bars, and color settings for the background */
/* DONE: consider adding a label for the entire statistics */
/* DONE: consider adding labels for each measure -- which could be used for a legend */
/* TODO: consider a show_label flag */
/* TODO: consider a mask to specify which subset of labels and other items to show */
/*       (label types: overall, hlines times, elements, monitor line times) */
/*       (other things: backdrop, time lines, monitor line time lines) */
typedef struct vrProcessStats_st {
		unsigned int	calc_flag;	/* boolean flag of whether to calc new stats */
		unsigned int	show_flag;	/* boolean flag of whether to show anything  */
		unsigned int	show_mask;	/* bitmask of what to show                   */

		float		back_color[4];	/* color of the backdrop */
		float		label_color[4];	/* color of the overall label */
		float		**elem_colors;	/* colors of each element */

		char		*label;		/* overall label of this statistics display  */
		char		**elem_labels;	/* label for each of the elements            */
    /* TODO: */	float		xloc;		/* left edge of stats display (0.0 to 1.0)   */
    /* TODO: */	float		width;		/* width of stats display                    */
		float		yloc;		/* lower edge of stats display (0.0 to 1.0)  */
		float		top_time;	/* upper time of stats background -- NOTE: stats may go off the top */
		float		hline_interval;	/* interval between horizontal lines         */
		float		time_scale;	/* scale of stats vertical bars              */
		int		elements;	/* number of elements to store for each time */
		int		frames;		/* number of frames stored in this struct    */
		int		time_frame;	/* current incoming measures element         */
		vrTime		mark_wtime;	/* last wall-time we were here               */
		vrTime		*measures;	/* array of times for all frames             */
	} vrProcessStats;


/***************************************************************/
/* vrProcessInfo: A structure containing all the details about */
/*   a particular process.                                     */
/***************************************************************/
typedef struct vrProcessInfo_st {

		/*************************/
		/* Generic Object fields */
		vrObject	object_type;	/* The type of this object (ie. VROBJECT_PROC) */
		int		id;		/* numeric id for this process. */
		char		*name;		/* CONFIG: name of this process */
		int		malleable;	/* CONFIG: Whether the data of this process can be modified */
	struct vrProcessInfo_st	*next;		/* the next proc in a linked list */
		vrContextInfo	*context;	/* pointer to the vrContext memory structure */
		int		line_created;	/* line number of where this object was created in config */
		int		line_lastmod;	/* line number of last time this object modified in config */
		char		file_created[512];/* file (or other) where this object was created*/
		char		file_lastmod[512];/* file (or other) where this object was created*/

		/********************************/
		/* Fields specific to Processes */

		vrProcessType	type;		/* CONFIG: type of this process */
		void		*aux_data;	/* auxiliary data specific for each type of process (allocated in the main loop of each process) */
		int		num;		/* the number of this process in running system (1-based) */
		pid_t		pid;		/* The Unix process ID of this process. */
#if defined(MP_PTHREADS) || defined(MP_PTHREADS2)
		pthread_t	tid;
#endif
		vrSystemSettings settings;	/* CONFIG: values for many inheritable fields */
		char		*machine_name;	/* CONFIG: the name of the machine on which this process should be run. */
		int		sync_id;	/* CONFIG: the group of processes this process should sync with */
		vrBarrier	*barrier;	/* barrier of this process' sync-group. */
		vrBarrier	*barrier2;	/* 2nd barrier of this process' sync-group. (test!) */
		int		group_master;	/* flag that indicates this process is master of a sync-group */
		int		used;		/* flag that indicates whether this process object is used by the system */
		int		initialized;	/* flag that indicates whether this process has been initialized yet */
		int		end_proc;	/* flag to indicate that the process should exit */
		int		proc_done;	/* state that indicates that the process has exited */

		int		num_thing_names;/* CONFIG: the number of things (windows, input devices, etc.) this process should handle */
		int		num_things;	/* CONFIG: the number of things (windows, input devices, etc.) this process does handle */
		char		**thing_names;	/* CONFIG-ONLY: the names of the things handled by this process */
		void		**things;	/* pointers to the things handled by this process (calculated later using thing_names */
		char		*args;		/* CONFIG: TODO: not sure we need arguments to this */
		int		usec_min;	/* CONFIG: minimum number of micro seconds to spend per frame */
		long		frame_count;	/* number of iterations through the process' mainloop (so far) */
		vrTime		spawn_stime;	/* sim-time when this process began.  */
		vrTime		frame_wtime;	/* wall-time when this frame began.    */
		vrTime		frame_wtimes[10];/* array of wall-times used in calculating frame rates */
		float		fps1;		/* frame rate as computed by one frame */
		float		fps10;		/* frame rate as computed by ten frames */

		int		print_color;	/* CONFIG: sets the color used for text printed by this process. */
		char		*print_string;	/* CONFIG: a text string prepended to each line printed by this proc. */
		char		*print_filename;/* CONFIG: the name of a file or device to which to print. */
		FILE		*print_file;	/* CONFIG: a terminal or file to which this proc prints. */
		int		tracemsgcnt;	/* where to put the next trace string in the tracemsg array */
		char		tracemsg[VRPROC_NUMTRACEMSGS][1024];/* a place where TRACE_DBGLVL messages are stored */
		vrTime		tracetime[VRPROC_NUMTRACEMSGS];/* the time in which each trace message occurred. */

		char		*stats_args;	/* CONFIG: arguments for stats configuration */
		vrProcessStats	*stats;		/* time statistics for this process */

	} vrProcessInfo;


/*************************************************/
/*** Globally accessible function declarations ***/
void		vrProcessClear(vrProcessInfo *object);
void		vrProcessCopy(vrProcessInfo *dest_object, vrProcessInfo *src_object);
vrProcessInfo	*vrProcessCreateMainProcessInfo(vrContextInfo *vrContext);
void		vrProcessCalcFrameRate(vrProcessInfo *procinfo);
void		vrFprintProcessInfo(FILE *file, vrProcessInfo *procinfo, vrPrintStyle style);

void		vrProcessStart(vrContextInfo *context, vrProcessInfo *info);
void		vrProcessSync(vrProcessInfo *proc_info, int stats_sync, int stats_freeze);
void		vrProcessStop(vrProcessInfo *info);
void		vrProcessStopAll(int which);

void		vrSetSignalHandler(void (*)(int));

vrProcessStats	*vrProcessStatsCreate(char *label, int elements, char *args);
vrTime		vrProcessStatsMark(vrProcessStats *stats, int element, unsigned int sum_flag);
void		vrProcessStatsNextFrame(vrProcessStats *stats);

/*****************************************************************************/
/** system-wide global for accessing information about the current process. **/
#ifdef MP_PTHREADS
vrProcessInfo	**vrThisProcFunc();
#  define vrThisProc (*vrThisProcFunc())	/* warning -- requires that vrThisProcFunc() never return a NULL, or we may dereference a NULL */
#elif defined(MP_PTHREADS2)
#  define vrThisProc ((vrProcessInfo*)pthread_getspecific(vrContext->this_proc_key))	/* warning -- requires that pthread_getspecific() never return a NULL, or we may dereference a NULL */
#else
extern	vrProcessInfo	*vrThisProc;
#endif


#ifdef __cplusplus
}
#endif

#endif

