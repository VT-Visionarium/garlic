/* ======================================================================
 *
 *  CCCCC          vr_procs.c
 * CC   CC         Author(s): Ed Peters, John Stone, Bill Sherman, Jeff Stuart
 * CC              Created: June 4, 1998
 * CC   CC         Last Modified: September 1, 2014
 *  CCCCC
 *
 * Code file for FreeVR process control.  See vr_procs.h for a
 * description of these functions.
 *
 * Copyright 2015, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */

#include "vr_procs.h"		/* NOTE: Sukru includes vr_shmem.h instead */
#include "vr_objects.h"
#include "vr_config.h"
#include "vr_debug.h"
#include "vr_utils.h"
#ifdef SEM_TCP
#  include "vr_sem_tcp.h"
#endif
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/time.h>
#include <math.h>		/* for pow() */


/* some helper functions local to this file */
static void	_FprintProcessStats(FILE *file, vrProcessStats *stats, vrPrintStyle style);
static void	*_ProcessInitChild(void *param);


#ifdef MP_PTHREADS
/*****************************************************************/
vrProcessInfo	**vrThisProcFunc()
{
static	vrProcessInfo	*null_process = NULL;
	int		i;
	pthread_t	tid = pthread_self();
	vrProcessInfo	**ret = &null_process;

	if (vrContext == NULL) {
		return &null_process;
	}

	if (vrContext->config == NULL) {
		return &null_process;
	}

	if (vrContext->config->num_procs == 0) {
		return &null_process;
	}

	for (i = 0; i < vrContext->config->num_procs; i++) {
		if (vrContext->config->procs[i] != NULL) {
			if (pthread_equal(tid, vrContext->config->procs[i]->tid)) {
				ret = &vrContext->config->procs[i];
				break;
			}
		}
	}

	return ret;
}
#elif defined(MP_PTHREADS2)
	/* We currently have no specific requirements for method-2 of using pthreads */

#elif defined(MP_NONE)
	/* 09/01/14: I have no idea what will happen in the single-thread version for "vrThisProc".  TODO: need to test. */
	/* Presently not setting vrThisProc for the single-threaded version of FreeVR. */
	vrProcessInfo	*vrThisProc = NULL;

#else
/*****************************************************************/
/** system-wide (process local) global for accessing information **/
/**   about the current process.                                 **/
/**   (ie. all processes have this, but it's different for each) **/
	vrProcessInfo	*vrThisProc = NULL;
#endif


/*****************************************************************/
char *vrProcessTypeName(vrProcessType type)
{
	switch (type) {
		case VRPROC_MAIN:	return "main";
		case VRPROC_INPUT:	return "input";
		case VRPROC_TELNET:	return "telnet";
		case VRPROC_VISREN:	return "visren";
		case VRPROC_COMPUTE:	return "compute";
		case VRPROC_AUDREN:	return "audren";
		case VRPROC_HAPREN:	return "hapren";
		case VRPROC_OLFREN:	return "olfren";
		case VRPROC_TASREN:	return "tasren";
		case VRPROC_VESREN:	return "vesren";
		case VRPROC_NOCONFIG:	return "unconfigured";
		default:		return "unknown";
	}

	return "unknown";
}


/*****************************************************************/
void vrProcessClear(vrProcessInfo *object)
{
	object->object_type = VROBJECT_PROCESS;
	object->type = VRPROC_NONE;
	object->machine_name = NULL;
	object->sync_id = 0;
	object->pid = -1;
#if defined(MP_PTHREADS) || defined(MP_PTHREADS2)
	object->tid = (pthread_t)-1;
#endif
	object->barrier = NULL;
	object->barrier2 = NULL;
	object->group_master = 0;
	object->used = 0;
	object->initialized = 0;
	object->end_proc = 0;
	object->proc_done = 0;
	object->num_things = 0;
	object->num_thing_names = 0;
	object->thing_names = NULL;
	object->things = NULL;
	object->usec_min = 1000;	/* this is a low minimum -- 1ms */
	object->frame_count = 0;
	object->spawn_stime = -1.0;
	object->frame_wtime = 0.0;
	object->fps1 = -1.0;
	object->fps10 = -1.0;
	object->print_color = 0;	/* ie. NORM_TEXT (-1 means no change) */
	object->print_string = "";
	object->print_filename = NULL;
	object->print_file = NULL;
	object->stats = NULL;
	object->stats_args = NULL;

	vrSettingsClear(&object->settings);
}


/*****************************************************************/
/* TODO: set a better value for the "next" field */
void vrProcessCopy(vrProcessInfo *dest_object, vrProcessInfo *src_object)
{
	void	*dest_mem;
	void	*src_mem;
	int	memlen;
	int	count;

#if 1 /* 3/8/2005 -- I decided that actually I do want to copy just about everything */
	/* copy only the memory after the generic vrObjectInfo stuff */
	dest_mem = (char *)dest_object + sizeof(vrObjectInfo);
	src_mem = (char *)src_object + sizeof(vrObjectInfo);
	memlen = sizeof(vrProcessInfo) - sizeof(vrObjectInfo);
	memcpy(dest_mem, src_mem, memlen);
#else
	/* but I do need to preserve the name  -- I had failed to do this before [3/25/05] */
	memcpy(dest_object, src_object, sizeof(vrProcessInfo));
	dest_object->malleable = 1;
	dest_object->next = NULL;		/* TODO: there must be a better answer than this, but if I leave it as a copy, the delay suggests I've created a loop in the linked list. */
#endif

	/* make independent copy of some fields */
	dest_object->thing_names = (char **)vrShmemAlloc0(src_object->num_thing_names * sizeof(char *));
	dest_object->things = (void **)vrShmemAlloc0(src_object->num_thing_names * sizeof(void *));
	for (count = 0; count < src_object->num_thing_names; count++) {
		dest_object->thing_names[count] = vrShmemStrDup(src_object->thing_names[count]);
	}
	if (src_object->things != NULL) {
		for (count = 0; count < src_object->num_thing_names; count++) {
			dest_object->things[count] = vrShmemMemDup(src_object->things[count], sizeof(void *));
		}
	} else {
		dest_object->things = src_object->things;  /* ie. NULL */
	}

	dest_object->machine_name = vrShmemStrDup(src_object->machine_name);
}


/*****************************************************************/
vrProcessInfo *vrProcessCreateMainProcessInfo(vrContextInfo *vrContext)
{
	vrProcessInfo	*mainProc;

#if 0 /* 05/18/06 -- today I question whether we really don't want the main process on the linked list -- because I'd like to be able to modify some of the parameters like the "stats_args"! */
	/* NOTE: we don't use the vrObjectNew() routing because */
	/*   we don't want this process on this linked list.    */
	mainProc = (vrProcessInfo *)vrShmemAlloc0(sizeof(vrProcessInfo));
	vrProcessClear(mainProc);

	mainProc->object_type = VROBJECT_PROCESS;
	mainProc->id = -1;	/* TODO: I'd like this to be 0, but need to make sure the other */
				/*   processes begin at 1 then.  */
				/*   Perhaps this could be done by adding this process to the */
				/*   list of config processes -- using a name that would be   */
				/*   dificult to duplicate in the config file -- or maybe that */
				/*   doesn't even matter since this one will always be first on */
				/*   the list, and can be set to be non-malleable.              */
	mainProc->type = VRPROC_MAIN;
	mainProc->name = vrShmemStrDup("built-in main");
#else
	mainProc = vrObjectNew(vrContext, VROBJECT_PROCESS, "built-in main");
	vrProcessClear(mainProc);
#endif

	/* Now alter some of the default process values */
	mainProc->line_created = __LINE__; strcpy(mainProc->file_created, __FILE__);
	mainProc->line_lastmod = __LINE__; strcpy(mainProc->file_lastmod, __FILE__);
	mainProc->sync_id = 0;			/* TODO: sync main with other procs via vrFrame() */
	mainProc->settings.lock_proc = -1;	/* TODO: figure out what this really should be */
	mainProc->pid = getpid();
#if defined(MP_PTHREADS) || defined(MP_PTHREADS2)
	mainProc->tid = pthread_self();
#endif
	mainProc->spawn_stime = 0.0;
	mainProc->args = NULL;
	mainProc->aux_data = NULL;

	mainProc->print_file = stderr;


	vrSettingsClear(&mainProc->settings);


#if 0 /* TODO: readd this -- not doing this allows for seg faults, so we can debug easier */
	vrSetSignalHandler(vrStopAllProcesses);
#endif

	return (mainProc);
}


/*****************************************************************/
/* calculate and fill in the fps1 and fps10 fields for a process */
void vrProcessCalcFrameRate(vrProcessInfo *proc_info)
{
	int		frame_mod;
	vrTime		last_time1;
	vrTime		last_time10;

	frame_mod = proc_info->frame_count % 10;

	last_time1 = proc_info->frame_wtimes[(proc_info->frame_count-1) % 10];
	proc_info->fps1 = 1.0 / (proc_info->frame_wtime - last_time1);

	last_time10 = proc_info->frame_wtimes[frame_mod];
	proc_info->fps10 = 10.0 / (proc_info->frame_wtime - last_time10);

	proc_info->frame_wtimes[frame_mod] = proc_info->frame_wtime;
}


/*****************************************************************/
void vrFprintProcessInfo(FILE *file, vrProcessInfo *proc_info, vrPrintStyle style)
{
	int	count;

	/* if null process given, print an empty shell and return */
	if (proc_info == NULL) {
		vrFprintf(file, "process \"(nil)\" = { }\n");
		return;
	}

	switch (style) {
	case one_line:
		vrFprintf(file, "pid = %d, "
#if defined(MP_PTHREADS) || defined(MP_PTHREADS2)
				"tid = %ld, "
#endif
				"'%s', type = '%s', sync = %d, count = %d, fps1 = %.1f, fps10 = %.1f\n",
			proc_info->pid,
#if defined(MP_PTHREADS) || defined(MP_PTHREADS2)
			proc_info->tid,
#endif
			proc_info->name,
			vrProcessTypeName(proc_info->type),
			proc_info->sync_id,
			proc_info->frame_count,
			proc_info->fps1,
			proc_info->fps10);
		break;

	case machine:
		vrFprintf(file,
#if defined(MP_PTHREADS) || defined(MP_PTHREADS2)
				"%ld:"
#else
				"%d:"
#endif
				"%s:%s:%d:%d:%.1f:%.1f\n",
#if defined(MP_PTHREADS) || defined(MP_PTHREADS2)
			proc_info->tid,
#else
			proc_info->pid,
#endif
			proc_info->name,
			vrProcessTypeName(proc_info->type),
			proc_info->sync_id,
			proc_info->frame_count,
			proc_info->fps1,
			proc_info->fps10);
		break;

	default:
	case verbose:
		vrFprintf(file, "{\n");
		vrFprintf(file, "\r"
			"\tobject_type = %s (%d)\n\tid = %d\n\tname = \"%s\"\n"
			"\tmachine_name = '%s'\n"
			"\tmalleable = %d\n\tnext = %#p\n"
			"\tCreated at %s, line %d\n"
			"\tLast modified at %s, line %d\n",
			vrObjectTypeName(proc_info->object_type),
			proc_info->object_type,
			proc_info->id,
			proc_info->name,
			proc_info->machine_name,
			proc_info->malleable,
			proc_info->next,
			proc_info->file_created,
			proc_info->line_created,
			proc_info->file_lastmod,
			proc_info->line_lastmod);
		vrFprintf(file, "\r"
			"\ttype = '%s' (%d)\n\tnum in system = %d\n\tpid = %d\n"
#if defined(MP_PTHREADS) || defined(MP_PTHREADS2)
			"\ttid = %ld\n"
#endif
			"\tmachine_name = '%s'\n"
			"\tsync_id = %d\n\tbarrier = %#p\n\tbarrier2 = %#p\n"
			"\tgroup_master = %d\n\tused = %d\n"
			"\tinitialized = %d\n\tend_proc = %d\n\tproc_done = %d\n"
			"\tusec_min = %d\n"
			"\tframe_count = %ld\n"
			"\tspawn_stime = %.2lf\n\tframe_wtime = %.2lf\n"
			"\tfps1 = %.2lf\n\tfps10 = %.2lf\n",
			vrProcessTypeName(proc_info->type),
			proc_info->type,
			proc_info->num,
			proc_info->pid,
#if defined(MP_PTHREADS) || defined(MP_PTHREADS2)
			proc_info->tid,
#endif
			(proc_info->machine_name == NULL) ? "" : proc_info->machine_name,
			proc_info->sync_id,
			proc_info->barrier,
			proc_info->barrier2,
			proc_info->group_master,
			proc_info->used,
			proc_info->initialized,
			proc_info->end_proc,
			proc_info->proc_done,
			proc_info->usec_min,
			proc_info->frame_count,
			proc_info->spawn_stime,
			proc_info->frame_wtime,
			proc_info->fps1,
			proc_info->fps10);
		vrFprintf(file, "\r"
			"\tdebug_level = %d\n\tdebug_exact = %d\n"
			"\tprint_color = %d\n\tprint_string = '%s'\n"
			"\tprint_filename = '%s'\n\tprint_file = %p\n"
#if 0 /* showing the last trace message here and again below can be confusing. */
			"\ttracemsg = '%s'\n"
#endif
			,
			proc_info->settings.debug_level,
			proc_info->settings.debug_exact,
			proc_info->print_color,
			proc_info->print_string,
			(proc_info->print_filename == NULL) ? "" : proc_info->print_filename,
			proc_info->print_file
#if 0 /* showing the last trace message here and again below can be confusing. */
			, proc_info->tracemsg[proc_info->tracemsgcnt]
#endif
			);
		vrFprintf(file, "\r"
			"\tnum_thing_names = %d\n\tthing names = %#p [",
			proc_info->num_thing_names,
			proc_info->thing_names);
		for (count = 0; count < proc_info->num_thing_names; count++) {
			vrFprintf(file, " \"%s\"", proc_info->thing_names[count]);
		}
		vrFprintf(file, " ]\n\tnum_things = %d\n\tthings = %#p [",
			proc_info->num_things, proc_info->things);
		for (count = 0; count < proc_info->num_things; count++) {
			if (proc_info->things == NULL)
				vrFprintf(file, " --  ");
			else	vrFprintf(file, " %#p", proc_info->things[count]);
		}
		vrFprintf(file, " ]\n");

		vrFprintf(file, "\r"
			"\targs = \"%s\"\n\texec_start = \"%s\"\n"
			"\texec_stop = \"%s\"\n\taux_data = %#p\n",
			(proc_info->args == NULL) ? "(none)" : proc_info->args,
			(proc_info->settings.exec_start == NULL) ? "(none)" : proc_info->settings.exec_start,
			(proc_info->settings.exec_stop == NULL) ? "(none)" : proc_info->settings.exec_stop,
			proc_info->aux_data);
		for (count = 1; count <= VRPROC_NUMTRACEMSGS; count ++) {
			vrFprintf(file, "\r"
				"\ttracemsg[%02d] (at %lf) = '%s'\n",
				(proc_info->tracemsgcnt+count)%VRPROC_NUMTRACEMSGS,
				proc_info->tracetime[(proc_info->tracemsgcnt+count)%VRPROC_NUMTRACEMSGS],
				proc_info->tracemsg[(proc_info->tracemsgcnt+count)%VRPROC_NUMTRACEMSGS]);
		}
		vrFprintf(file, "\r"
			"\tstats_args = \"%s\"\n\tstats = %#p\n",
			proc_info->stats_args,
			proc_info->stats);
		vrFprintf(file, "\r}\n");

		/* Now print the statistics data, if available */
		if (proc_info->stats != NULL)
			_FprintProcessStats(file, proc_info->stats, style);

		break;

	case file_format:
		vrFprintf(file, "process \"%s\" = {\n", proc_info->name);

		if (!proc_info->malleable)
			vrFprintf(file, "\tmalleable = %d;\n", proc_info->malleable);

		if (proc_info->type == VRPROC_MAIN) {
			vrFprintf(file, "\t# Warning, if this 'main' type process is the \"built-in main\"\n");
			vrFprintf(file, "\t#   process, then it will redefine the default 'main' process.\n");
			vrFprintf(file, "\t#   When using this file-formatted output as an active FreeVR\n");
			vrFprintf(file, "\t#   configuration file, it would be best to comment out this\n");\
			vrFprintf(file, "\t#   entire process definition to avoid any potential problems.\n");
		}
		vrFprintf(file, "\ttype = %s;\n", vrProcessTypeName(proc_info->type));

		if (proc_info->machine_name != NULL)
			vrFprintf(file, "\tmachine = \"%s\";\n", proc_info->machine_name);

		if (proc_info->args != NULL)
			vrFprintf(file, "\targs = \"%s\";\n", proc_info->args);

		if (proc_info->num_thing_names > 0) {
			vrFprintf(file, "\tobjects =");
			for (count = 0; count < proc_info->num_thing_names; count++) {
				if (proc_info->things == NULL)
					vrFprintf(file, "");
				else	vrFprintf(file, " \"%s\"", proc_info->thing_names[count]);
				if (count+1 < proc_info->num_thing_names)
					vrFprintf(file, ",");
			}
			vrFprintf(file, ";\n");
		}

		if (proc_info->usec_min != 1000)
			vrFprintf(file, "\tusecMin = %d;\n", proc_info->usec_min);

		if (proc_info->settings.debug_level != 1)
			vrFprintf(file, "\tDebugLevel = %d;\n", proc_info->settings.debug_level);
		if (proc_info->settings.debug_exact != 0)
			vrFprintf(file, "\tDebugThisToo = %d;\n", proc_info->settings.debug_exact);

		if (proc_info->print_color != 0)
			vrFprintf(file, "\tprintColor = %d;\n", proc_info->print_color);

		if (strlen(proc_info->print_string) > 0)
			vrFprintf(file, "\tprintString = \"%s\";\n", proc_info->print_string);
		if (proc_info->print_filename != NULL)
			vrFprintf(file, "\tprintFile = \"%s\";\n", proc_info->print_filename);

		if (proc_info->settings.exec_start != NULL)
			if (strlen(proc_info->settings.exec_start) > 0)
				vrFprintf(file, "\texecAtStart = \"%s\";\n", proc_info->settings.exec_start);
		if (proc_info->settings.exec_stop != NULL)
			if (strlen(proc_info->settings.exec_stop) > 0)
				vrFprintf(file, "\texecAtStop = \"%s\";\n", proc_info->settings.exec_stop);

		if (proc_info->stats_args != NULL)
			vrFprintf(file, "\tstats = \"%s\";\n", proc_info->stats_args);

		if (proc_info->settings.lock_proc != -1 /* VRTOKEN_DEFAULT */)
			vrFprintf(file, "\tlockCPU = %d;\n", proc_info->settings.lock_proc);
		else	vrFprintf(file, "\t# Inherit from Global: lockCPU = \"default\";\n", proc_info->settings.lock_proc);
		if (proc_info->settings.proc_lock_cmd != NULL)
			vrFprintf(file, "\tLockCommand = \"%s\";\n", proc_info->settings.proc_lock_cmd);
		else	vrFprintf(file, "\t# Inherit from Global: LockCommand = NULL;\n");
		if (proc_info->settings.proc_unlock_cmd != NULL)
			vrFprintf(file, "\tUnLockCommand = \"%s\";\n", proc_info->settings.proc_unlock_cmd);
		else	vrFprintf(file, "\t# Inherit from Global: UnLockCommand = NULL;\n");
		vrFprintf(file, "}\n\n");
		break;
	}
}


/**********************************************************************/
/* vrProcessStart(): This function spawns off the given process and   */
/*   returns.  The spawned process will initialize some process values,*/
/*   invoke the appropriate main loop and begin running, or die with  */
/*   an error message if there's something wrong.  When the main loop */
/*   terminates, some post-process operations are performed.          */
/*                                                                    */
/* TODO: this function should also handle the process locking and unlocking. */
/**********************************************************************/
void vrProcessStart(vrContextInfo *context, vrProcessInfo *proc_info)
{
	int	childpid;
	int	count;
	void	**param = (void**)malloc(sizeof(void *) * 2);

	/* NOTE: pthread_create() requires all new threads to take arguments as an array of void-pointers. */
	/*   To be consistent, this is how the context and proc_info are also passed to the forked         */
	/*   processes.                                                                                    */
	param[0] = (void *)context;	/* first argument to _ProcessInitChild() */
	param[1] = (void *)proc_info;	/* second argument to _ProcessInitChild() */

	/**********************************************/
	/* verify that this process requires starting */
	if (proc_info == NULL) {
		vrErrPrintf("vrProcessStart(): "
			RED_TEXT "Attempted to start NULL process!\n" NORM_TEXT);
		return;
	}

	if (proc_info->type == VRPROC_NOCONFIG) {
		vrErrPrintf("vrProcessStart(): "
			RED_TEXT "Attempted to start an unconfigured process ('%s')!\n" NORM_TEXT, proc_info->name);
		return;
	}

	/***************************************************************/
	/* open a file for processes that have their output redirected */
	if (proc_info->print_filename != NULL) {
		proc_info->print_file = fopen(proc_info->print_filename, "a+");
		vrDbgPrintfN(SELDOM_DBGLVL, "vrProcessStart(): just opened file '%s' with FILE * = %p\n", proc_info->print_filename, proc_info->print_file);
		if (proc_info->print_file == NULL) {
			/* try again, but this time just for writing */
			proc_info->print_file = fopen(proc_info->print_filename, "w");
			vrDbgPrintfN(SELDOM_DBGLVL, "vrProcessStart(): could only open file '%s' for writing with FILE * = %p\n", proc_info->print_filename, proc_info->print_file);
		}
	}

	/*****************************************************************/
	/*** Print a statement that we are ready to spawn this process ***/
	if (vrDbgDo(SPAWN_DBGLVL)) {
		/* TODO: make this all one string so it can't get split up! */
		vrPrintf("#### about to spawn a process of type '%s' (%d) with %d things:",
			vrProcessTypeName(proc_info->type), proc_info->type, proc_info->num_thing_names);
		for (count = 0; count < proc_info->num_thing_names; count++)
			vrPrintf(" %s", proc_info->thing_names[count]);
		vrPrintf("\n");
	}

#if defined(GFX_PERFORMER) && 0 /* TODO: 2/28/03 -- can we delete this? */
	if (proc_info->type == VRPROC_VISREN) {
		vrWindowInfo	*window;

		/* TODO: loop over all windows listed in this process */

		window = (vrWindowInfo *)(proc_info->things[0 /* TODO: s/b loop variable */]);
		vrCalcPerspIntermediaries(window);	/* do some pre-calculations for the perspective matrix */
		vrTrace("PF", "opening a window");
printf("hey, about to do Performer visren open callback (%#p)\n", window->Open);
		vrCallbackInvoke(window->Open);

		/* TODO: should we do the pre-process functions here?  I suppose so.         */
		/*   Or maybe we need a dummy process to run, that will do a nop until       */
		/*   told to close, and then afterword do the post-process commands.         */
		/*   However, that might have a big CPU-hog-loop going, and that's not good. */

		return;	/* don't spawn in Performer */
	}
#endif

	/**************************/
	/*** Spawn the process! ***/
	/**************************/
#if defined(MP_PTHREADS) || defined(MP_PTHREADS2)
{
	pthread_t threadid;

	pthread_create(&threadid, NULL, _ProcessInitChild, (void *)param);
}
#elif defined(MP_NONE)
	/* okay, in this case, initialize but don't spawn the process */
	vrTrace("vrProcessStart", "Starting a virtual process!");
	_ProcessInitChild(param);
#else
#  ifdef GFX_PERFORMER
	if (proc_info->type == VRPROC_VISREN)
		childpid = 0;
	else	childpid = fork();
#  else
	childpid = fork();
#  endif

	/* for Performer and "fork" style libraries (ie. non-pthreads) */
	if (childpid == -1) {
		vrErr("can't fork");

	} else if (childpid == 0) {
		_ProcessInitChild(param);
	}
	else free(param);
#endif
}


/*****************************************************************************/
void *_ProcessInitChild(void *param)
{
	void		**params = (void **)param;
	vrContextInfo	*context = (vrContextInfo *)params[0];
	vrProcessInfo	*proc_info = (vrProcessInfo *)params[1];
	int		count;
	vrTime		loop_wtime;

	free(param);

	/*********************************************/
	/*** This is for the spawned child process ***/
	/*********************************************/
	/* Each child process will record its pid in global */
	/*   struct and invoke appropriate function.        */

	/* Set the vrThisProc global pointer to the structure */
	/*   that describes the spawned process.              */

#ifdef GFX_PERFORMER /* Performer version of FreeVR doesn't spawn visren processes, so don't change vrThisProc */
	if (proc_info->type != VRPROC_VISREN) {
#endif
#  ifdef MP_PTHREADS
		/* No need to specifically set vrThisProc in pthread-1 version */
#  elif defined(MP_PTHREADS2)
		pthread_setspecific(context->this_proc_key, (void *)proc_info);
#  elif defined(MP_NONE)
		/* TODO: determine what to do for the single-threaded variant of FreeVR. */
#  else
		vrThisProc = proc_info;
#  endif
#ifdef GFX_PERFORMER
	}
#endif
#ifdef SEM_TCP
	/* TODO: or should this be vr_tcp_initialize() ?? */
# if 1
	vrTcpInitSockets();		/* Initialize socket routines (esp. for Win32) for this process */
# else
	tcp_init_sockets();
# endif
#endif

	/* 09/01/14: NOTE: the rest of this function is mis-indented -- TODO: fix the indentation! */

	/********************************************/
	/* Initialize the basic process information */
	vrTrace("_ProcessInitChild", "initializing newly spawned process");
	proc_info->pid = getpid();
#if defined(MP_PTHREADS) || defined(MP_PTHREADS2)
	proc_info->tid = pthread_self();
#endif
	proc_info->spawn_stime = vrCurrentSimTime();
	proc_info->frame_count = 0;
	vrDbgPrintfN(AALWAYS_DBGLVL,
#if defined(MP_PTHREADS) || defined(MP_PTHREADS2)
		"FreeVR: Thread \"%s\" spawned!  Pid = %d, tid = %ld, time = %lf\n",
#elif defined(MP_NONE)
		"FreeVR: Virtual-process \"%s\" initialized!  Pid = %d, time = %lf\n",
#else
		"FreeVR: Process \"%s\" spawned!  Pid = %d, time = %lf\n",
#endif
		proc_info->name,
		proc_info->pid,
#if defined(MP_PTHREADS) || defined(MP_PTHREADS2)
		proc_info->tid,
#endif
		proc_info->spawn_stime);


	/*******************************************************/
	/* Call the process locking shell command if specified */
	if ((context->config->system->settings.lock_proc > 0)
			|| ((context->config->system->settings.lock_proc == -1 /* VRTOKEN_DEFAULT */)
			    && (context->config->defaults.lock_proc > 0))
			&& (proc_info->settings.lock_proc != 0)) {

		/* We do this when process locking is enabled by the system or */
		/*   inherited from the default value, and the process itself  */
		/*   doesn't specifically indicate that locking should not be  */
		/*   done.                                                     */

		char	*cmd;

		cmd = context->config->system->settings.proc_lock_cmd;
		if (cmd == NULL)
			cmd = context->config->defaults.proc_lock_cmd;

		if (cmd != NULL)
			vrShellCmd(cmd, proc_info->name, proc_info->pid, 0);
		else	vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_ProcessInitChild(): " RED_TEXT "No process locking command available\n");
	}

	/********************************************/
	/** Execute any pre-process shell commands **/
	vrShellCmd(proc_info->settings.exec_start, proc_info->name, proc_info->pid, 0);

	/*****************************************/
	/* Find existing or start new sync group */
	if (proc_info->sync_id > 0) {
static		char		string[128];
		vrProcessInfo	*procsearch;
		vrBarrier	*sync_barrier = NULL;
		vrBarrier	*sync_barrier2 = NULL;

		vrTrace("_ProcessInitChild", "finding sync group");

		/* If another process of the same sync-group has already  */
		/*   created the barrier, then add this process to that   */
		/*   barrier.  Otherwise this is the first process of the */
		/*   sync-group, so create a new barrier for (currently)  */
		/*   a single process.                                    */

		vrLockWriteSet(context->barrier_lock);		/* we only want one process at a time messing with this */

		/* first, search for an existing process in the sync-group */
		vrDbgPrintfN(BARRIER_DBGLVL, "_ProcessInitChild(): Searching for sync barrier for proc %d with sync id %d\n",
			proc_info->num, proc_info->sync_id);
		for (count = 0; count < context->config->num_procs; count++) {
			if (count+1 == proc_info->num)
				continue;	/* no need to compare with myself */

			procsearch = context->config->procs[count];
			if (procsearch == NULL) {
				vrPrintf("Hmmm, procsearch is NULL, I don't think this should happen\n");
				continue;	/* go onto the next process */
			}
			if (procsearch->sync_id == proc_info->sync_id) {
				if (procsearch->barrier != NULL) {
					sync_barrier = procsearch->barrier;
					sync_barrier2 = procsearch->barrier2;
				}
			}
		}

		/* now, either add this proc to an existing sync-group or */
		/*   create a new barrier.                                */
		if (sync_barrier == NULL) {
			snprintf(string, sizeof(string), "sync group %d - bar #1", proc_info->sync_id);
			sync_barrier = vrBarrierCreate(context, string, 1);
			snprintf(string, sizeof(string), "sync group %d - bar #2", proc_info->sync_id);
			sync_barrier2 = vrBarrierCreate(context, string, 1);
			vrDbgPrintfN(BARRIER_DBGLVL, "_ProcessInitChild(): Created new barrier pair for sync id group %d -- %#p\n",
				proc_info->sync_id, sync_barrier);
			proc_info->group_master = 1;
		} else {
			vrDbgPrintfN(BARRIER_DBGLVL, "_ProcessInitChild(): Using existing barrier pair for sync group %d -- %#p\n",
				proc_info->sync_id, sync_barrier);
			vrBarrierIncrement(sync_barrier, 1);
			vrBarrierIncrement(sync_barrier2, 1);
			proc_info->group_master = 0;	/* redundant, but informative */
		}
		proc_info->barrier = sync_barrier;
		proc_info->barrier2 = sync_barrier2;

		vrLockWriteRelease(context->barrier_lock);	/* now let other processes check their for their barriers */
	} else {
		proc_info->barrier = NULL;
		proc_info->barrier2 = NULL;
	}

	/***********************************************************/
	/** Execute the appropriate loop for the type of process. **/
	vrTrace("_ProcessInitChild", "entering process type-specific code");
	switch(proc_info->type) {

	case VRPROC_MAIN:
		vrErrPrintf("## Attempt to spawn a Main Process. " RED_TEXT "Error, this should not occur!\n" NORM_TEXT);
		exit(0);

	case VRPROC_INPUT:
		vrDbgPrintfN(SPAWN_DBGLVL, "## Spawned Input Process %d (%s)\n", proc_info->pid, proc_info->name);
		vrInputInitProc(proc_info);
		proc_info->initialized = 1;

#ifndef MP_NONE
#if 0 /* set to 0 to do the main loop right here, using the one-frame function. */
	/* By including the loop here, we can combine multiple process types    */
	/* into a single process.  NOTE: we don't currently allow this, but     */
	/* we might in the future.                                              */
		vrInputMainLoop(proc_info);
#else
		vrTrace("_ProcessInitChild--vrInputMainLoop", BOLD_TEXT "beginning process loop" NORM_TEXT);
		/******************************/
		/*** Poll all input devices ***/
		/******************************/
		/* enter a (seemingly infinite) loop, polling each device */

		loop_wtime = vrCurrentWallTime();	/* time of the beginning of each loop */
		while (!proc_info->end_proc) {
			vrTrace("_ProcessInitChild--vrInputMainLoop", BOLD_TEXT "*** top of input loop ***" NORM_TEXT);

			/**************************/
			/* do minimal frame delay */
			/* NOTE: Delay's of 10,000us or less allow 50fps frame rate for the input process (at least on my Thinkpad 770Z running linux) */
			vrSleep(proc_info->usec_min - (long)((vrCurrentWallTime() - loop_wtime) * 1000000.0));
			vrProcessStatsMark(proc_info->stats, proc_info->num_things, 0);			/* tag the amount of time spent in vrSleep() -- i.e. "num_things" is one higher than the bins for all the things (devices) */


			/*************************************************************/
			/* Sync with other processes in group & calculate frame info */
			vrProcessSync(proc_info, proc_info->num_things+1, proc_info->num_things+2);

			/* start measuring for minimal loop time */
			loop_wtime = vrCurrentWallTime();

			vrInputOneFrame(proc_info);
		}
		vrTrace("_ProcessInitChild--vrInputMainLoop", BOLD_TEXT "ending main loop" NORM_TEXT);
#endif
		vrDbgPrintfN(AALWAYS_DBGLVL, "FreeVR: Input Main Loop has ended -- " RED_TEXT "exiting process \"%s\".\n" NORM_TEXT, proc_info->name);
		vrInputTermProc(proc_info);
#endif /* MP_NONE */
		break;

	case VRPROC_TELNET:
		vrDbgPrintfN(SPAWN_DBGLVL, "## Spawned Telnet Communication Process %d (%s)\n", proc_info->pid, proc_info->name);
		vrTelnetInitProc(proc_info);
		proc_info->initialized = 1;
#ifdef MP_NONE
		vrDbgPrintfN(AALWAYS_DBGLVL, "FreeVR: " RED_TEXT BOLD_TEXT "Telnet process is disabled in single-threaded operation " NORM_TEXT RED_TEXT "-- at least for now.\n" NORM_TEXT);
#endif
#ifndef MP_NONE
#if 0 /* set to 0 to do the main loop right here, using the one-frame function. */
		vrTelnetMainLoop(proc_info);
#else
		vrTrace("_ProcessInitChild--vrTelnetMainLoop", BOLD_TEXT "beginning process loop" NORM_TEXT);
		loop_wtime = vrCurrentWallTime();
		while (!proc_info->end_proc) {

			/* NOTE: I'm not sure why the frame delay and barrier stuff are at the top */
			/*   of this loop -- probably due to how things work for the visren loop.  */

			/**************************/
			/* do minimal frame delay */
			vrSleep(proc_info->usec_min - (long)((vrCurrentWallTime() - loop_wtime) * 1000000.0));

			/*************************************************************/
			/* Sync with other processes in group & calculate frame info */
			vrProcessSync(proc_info, 0, 0);		/* NOTE: the last two arguments are for stats info that we don't worry about for telnet */

			/* start measuring for minimal loop time */
			loop_wtime = vrCurrentWallTime();

			vrTelnetOneFrame(proc_info);
		}

		vrTrace("_ProcessInitChild--vrTelnetMainLoop", BOLD_TEXT "ending main loop" NORM_TEXT);
#endif
		vrDbgPrintfN(AALWAYS_DBGLVL, "FreeVR: Telnet Main Loop has ended -- " RED_TEXT "exiting process \"%s\".\n" NORM_TEXT, proc_info->name);
		vrTelnetTermProc(proc_info);
#endif /* MP_NONE */
		break;

	case VRPROC_VISREN:
		vrDbgPrintfN(SPAWN_DBGLVL, "## Spawned VisRen Process %d (%s)\n", proc_info->pid, proc_info->name);
		vrVisrenInitProc(proc_info);
		proc_info->initialized = 1;
#ifndef MP_NONE
#if 0 /* set to 0 to do the main loop right here, using the one-frame function. */
		vrVisrenMainLoop(proc_info);
#else
		vrTrace("_ProcessInitChild--vrVisrenMainLoop", BOLD_TEXT "beginning process loop" NORM_TEXT);

		loop_wtime = vrCurrentWallTime();	/* time of the beginning of each loop */
		vrTrace("vrVisrenMainLoop", "beginning");
		while (!proc_info->end_proc) {
			vrTrace("vrVisrenMainLoop", BOLD_TEXT "*** top of rendering loop ***" NORM_TEXT);

			/**************************/
			/* do minimal frame delay */
			/* NOTE: Delay's of 10,000us or less allow 50fps frame rate for the input process (at least on my Thinkpad 770Z running linux) */
			vrSleep(proc_info->usec_min - (long)((vrCurrentWallTime() - loop_wtime) * 1000000.0));
			vrProcessStatsMark(proc_info->stats, VR_TIME_WAIT, 0);			/* tag the amount of time spent in vrSleep() -- i.e. the time waiting */


			/*************************************************************/
			/* Sync with other processes in group & calculate frame info */
			vrProcessSync(proc_info, VR_TIME_SYNC, VR_TIME_FREEZE);

			/* start measuring for minimal loop time */
			loop_wtime = vrCurrentWallTime();

			vrVisrenOneFrame(proc_info);
		}
		vrTrace("_ProcessInitChild--vrVisrenMainLoop", BOLD_TEXT "ending main loop" NORM_TEXT);
#endif
#ifdef GFX_PERFORMER
		vrDbgPrintfN(SPAWN_DBGLVL, "FreeVR: Performer Visren Main Loop has ended -- " BOLD_TEXT "NOT exiting process \"%s\".\n" NORM_TEXT, proc_info->name);
		return; 	/* Performer doesn't really have a visren process, so return */
#endif
		vrDbgPrintfN(AALWAYS_DBGLVL, "FreeVR: Visren Main Loop has ended -- " RED_TEXT "exiting process \"%s\".\n" NORM_TEXT, proc_info->name);
		vrVisrenTermProc(proc_info);
#endif /* MP_NONE */
		break;

	case VRPROC_COMPUTE:
	case VRPROC_AUDREN:
	case VRPROC_HAPREN:
	case VRPROC_OLFREN:
	case VRPROC_TASREN:
	case VRPROC_VESREN:
		vrErr("_ProcessInitChild(): **** unimplemented process type, waiting for system termination");
		proc_info->initialized = 1;
		proc_info->proc_done = 1;	/* without this, vrExit() will hang waiting for this process */
		while (!proc_info->end_proc)
			vrSleep(100000);	/* 100ms */

	default:
		vrErr("_ProcessInitChild(): **** unknown process type, waiting for system termination");
		proc_info->initialized = 1;
		proc_info->proc_done = 1;	/* without this, vrExit() will hang waiting for this process */
		while (!proc_info->end_proc)
			vrSleep(100000);	/* 100ms */
	}
	vrTrace("_ProcessInitChild", "ended process type-specific code");
	/* NOTE: we only get here once the process loop has ended */
	/**********************************************************/

	/*********************************************/
	/** Execute any post-process shell commands **/
	vrShellCmd(proc_info->settings.exec_stop, proc_info->name, proc_info->pid, 0);

	if ((context->config->system->settings.lock_proc > 0)
			|| ((context->config->system->settings.lock_proc == -1 /* VRTOKEN_DEFAULT */)
			    && (context->config->defaults.lock_proc > 0))
			&& (proc_info->settings.lock_proc != 0)) {

		/* We do this when process locking is enabled by the system or */
		/*   inherited from the default value, and the process itself  */
		/*   doesn't specifically indicate that locking should not be  */
		/*   done.                                                     */

		char	*cmd;

		cmd = context->config->system->settings.proc_unlock_cmd;
		if (cmd == NULL)
			cmd = context->config->defaults.proc_unlock_cmd;

		if (cmd != NULL)
			vrShellCmd(cmd, proc_info->name, proc_info->pid, 0);
		else	vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_ProcessInitChild(): " RED_TEXT "No process unlocking command available\n");
	}

	/**********************************************/
	/* report that this child process is now done */
	proc_info->proc_done = 1;

	/********************************/
	/* terminate this child process */
#if defined(MP_PTHREADS) || defined(MP_PTHREADS2)
	return NULL;
#elif defined(MP_NONE)
	/* The single-threaded variant of FreeVR also returns NULL, but for a  */
	/*   different reason than the Pthread variants -- it still needs the  */
	/*   system to continue -- we were only here to initialize the process.*/
	return NULL;
#else
	exit(0);
#endif

	/***************************************/
	/* parent process will simply continue */
	/***************************************/
	return NULL;
}


/*************************************************************************/
/* vrProcessSync(): this function synchronizes all processes at a common */
/*   barrier.  It then "freezes" input and other data, so all processes  */
/*   in the next round will use the same data.  A second barrier         */
/*   operation holds all processes while this "freeze" is being done.    */
void vrProcessSync(vrProcessInfo *proc_info, int stats_sync, int stats_freeze)
{
	vrContextInfo	*context = proc_info->context;
	int		sync_order;		/* order in which this process hit the sync barrier */
	int		do_freeze;		/* a flag that holds the complicated calculation of whether to do the freeze */

	/* In order to prevent the synchronization code from being executed as new  */
	/*   processes are added to the barrier lists, this whole function must     */
	/*   prevent the modification of barriers until the process is complete.    */
	/*   I.e. this process must be atomic from a barrier perspective.           */

	/* NOTE: in this new method the barrier routine will return the count of the */
	/*   order of when this processor got to the barrier.  So, instead of the    */
	/*   last process doing the freezing, we'll go with the first process.       */
	/*   So, the way to know the order is to have the value returned by          */
	/*   vrBarrierSync(), which can either be done via a call by reference, or   */
	/*   by just returning the value.  Of course, the latter means that we'd     */
	/*   have to find another way to get the same time to all the processes.     */
	/*   I think we'll go with the latter method, and as part of the freezing    */
	/*   section, that process can get the time and copy it to shared memory,    */
	/*   where all the processes can copy it, and thus all will have the same    */
	/*   time -- the process that does the work may end up copying the data to   */
	/*   itself, but that should be okay.                                        */

	/**********/
	/** sync **/
	vrTrace("vrProcessSync", "about to do sync-1");
	sync_order = vrBarrierSync(proc_info->barrier);
	vrDbgPrintfN(TRACE_DBGLVL, "(%s::%d) %s -> " RED_TEXT "after process sync barrier -- sync_order = %d, time = %f" NORM_TEXT "\n", __FILE__, __LINE__, "vrProcessSync", sync_order, (proc_info->barrier ? (proc_info->barrier->wtime - proc_info->barrier->context->time_immemorial) : 0));
	vrDbgPrintfN(BARRIER_DBGLVL, "vrProcessSync(): " RED_TEXT "after process sync barrier -- sync_order = %d, time = %f\n" NORM_TEXT, sync_order, (proc_info->barrier ? (proc_info->barrier->wtime - proc_info->barrier->context->time_immemorial) : 0));

	/* measure: time spent waiting for sync */
	vrProcessStatsMark(proc_info->stats, stats_sync, 0);	/* in new sync method */

	/************/
	/** freeze **/

	/* The first process to sync is responsible for updating inputs, etc.    */
	/* But the calculation is a bit more complicated than that.              */
	/* To summarize:                                                         */
	/*	- if sync_order is 1, then that process is the "freeze" process. */
	/*	- if sync_order is greater than 1, then a sync-group exists and  */
	/*		this process is NOT the "freeze" process.                */
	/*      - if sync_order is 0, then if a sync-group exists, then this     */
	/*		process should leave the "freezing" to that process.     */
	/*		But if a sync-group does not exist (the barrier list is  */
	/*		NULL), then the process[1] should do the "freezing".     */
	/* NOTE: we need to at least make sure one of the processes transfers    */
	/*   the data from the "back buffer" to the "front".                     */
	vrTrace("vrProcessSync", "before barrier first to sync check");
	switch (sync_order) {
	case 1:
		do_freeze = 1;
		break;
	case 0:
		do_freeze = ((context->head_barrier == NULL) && (proc_info == context->config->procs[1]));
		break;
	default:
		do_freeze = 0;
		break;
	}
	vrDbgPrintfN(TRACE_DBGLVL, "(%s::%d) %s -> " RED_TEXT "do_freeze = %d (sync_order = %d, head_barrier = %p, procidis1? = %d)" NORM_TEXT "\n", __FILE__, __LINE__, "vrProcessSync", do_freeze, sync_order, context->head_barrier, proc_info == context->config->procs[1]);

	if (do_freeze && !context->paused) {
		vrTrace("vrProcessSync", "before freezing the input/travel/etc data");
		vrInputFreezeVisren(context);
		vrUserTravelFreezeVisren(context);
		vrPropFreezeVisren(context);
		vrTrace("vrProcessSync", "after freeze, now sync-2");
	} else {
		/* NOTE: we really shouldn't get here when paused, so if any actual */
		/*   work is done here, need to check for the "paused" state.       */
		vrTrace("vrProcessSync", "wait for freeze, using sync-2");
	}
#if 1 /* 03/03/05 -- changed from "1" to "0", and now process proceeds  -- this barrier was added 06/04/03 */
	/* TODO: consider waiting also for the callback updates (which now [3/08/2005] means splitting this function) */
	vrTrace("vrProcessSync", "about to do sync-2");
	sync_order = vrBarrierSync(proc_info->barrier2);		/* Now barrier to wait for the freezing */
	vrDbgPrintfN(TRACE_DBGLVL, "(%s::%d) %s -> " RED_TEXT "after process freeze barrier -- sync_order = %d, do_freeze = %d, time = %f" NORM_TEXT "\n", __FILE__, __LINE__, "vrProcessSync", sync_order, do_freeze, (proc_info->barrier ? (proc_info->barrier->wtime - proc_info->barrier->context->time_immemorial) : 0));
	vrDbgPrintfN(BARRIER_DBGLVL, "vrProcessSync(): " RED_TEXT "after process freeze barrier -- sync_order = %d, do_freeze = %d, time = %f\n" NORM_TEXT, sync_order, do_freeze, (proc_info->barrier ? (proc_info->barrier->wtime - proc_info->barrier->context->time_immemorial) : 0));
#endif
	if (sync_order == 0)
		proc_info->frame_wtime = vrCurrentWallTime();
	else	proc_info->frame_wtime = proc_info->barrier->wtime;

	/* measure: time spent freezing */
	vrProcessStatsMark(proc_info->stats, stats_freeze, 0);	/* in new sync method */
}



/********************************************************************/
/* vrProcessStop(): this function simply sends the SIGINT signal to */
/*   the given process (via its pid) to tell it to shut itself down.*/
/*                                                                  */
/* TODO: do we need to do more?                                     */
/* TODO: we should probably also unlock the process if locked.      */
/*   (actually, because we now simple tell each process to          */
/*   terminate itself, it can do its own process unlocking.)        */
/********************************************************************/
void vrProcessStop(vrProcessInfo *proc_info)
{
	proc_info->end_proc = 1;
	vrFprintProcessInfo(stdout, proc_info, verbose);
}


/******************************************************************/
/* vrStopAllProcesses(): goes through the array of processes, and */
/*   informs each to terminate itself via the vrProcessStop()     */
/*   function.                                                    */
/******************************************************************/
void vrProcessStopAll(int which)
{
	int	i;

	for (i = 1; i < vrContext->config->num_procs; i++)
		vrProcessStop(vrContext->config->procs[i]);
}


/******************************************************************/
/* vrSetSignalHandler(): this function sets the given function as */
/*   the signal-handling function for a specific list of commonly */
/*   caught signals -- SIGINT, SIGHUP, SIGBUS, SIGSEGV, SIGPIPE   */
/* NOTE: currently SIGINT is given the ignore flag, because it is */
/*   more desirable to have the main process catch the interrupt  */
/*   signal, and have it tell each process to terminate itself.   */
/******************************************************************/
void vrSetSignalHandler(void (*func)(int))
{
#if 0
	signal(SIGINT, func);
#else
	signal(SIGINT, SIG_IGN);	/* we want the main process to catch this and then stop the other procs. */
#endif
	signal(SIGHUP, func);
	signal(SIGBUS, func);
#if 0
	signal(SIGSEGV, func);
#endif
	signal(SIGPIPE, func);
}


	/******************************************************/
	/******* Functions for doing process statistics *******/
	/******************************************************/


/* NOTE: since these are just debugging measures, no effort is made */
/*   to read/write lock the data.                                   */

/******************************************************************/
static void _FprintProcessStats(FILE *file, vrProcessStats *stats, vrPrintStyle style)
{
	int	count;

	/* if null process-stats given, print an empty shell and return */
	if (stats == NULL) {
		vrFprintf(file, "process-statistics \"(nil)\" = { }\n");
		return;
	}

	switch (style) {
	case one_line:
	case machine:
		vrFprintf(file, "TODO: put short version of stats info here\n");
		break;

	default:
	case verbose:
		vrFprintf(file, "ProcStats (%#p) = {\n", stats);
		vrFprintf(file, "\r"
			"\tlabel = \"%s\"\n\telem_labels = ",
			stats->label);
		for (count = 0; count < stats->elements; count++)
			vrFprintf(file, "\"%s\" ", stats->elem_labels[count]);
		vrFprintf(file, "\n\r"
			"\tcalc_flag = %d\n\tshow_flag = %d\n\tshow_mask = 0x%x\n",
			stats->calc_flag,
			stats->show_flag,
			stats->show_mask);
		vrFprintf(file, "\r"
			"\txloc = %f\n\twidth = %f\n"
			"\tyloc = %f\n\ttop_time = %fms\n"
			"\thline_interval = %fms\n\ttime_scale = %f\n"
			"\telements = %d\n\tframes = %d\n",
			stats->xloc,
			stats->width,
			stats->yloc,
			stats->top_time,
			stats->hline_interval,
			stats->time_scale,
			stats->elements,
			stats->frames);
		vrFprintf(file, "\r"
			"\ttime_frame = %d\n\tmark_wtime = %lf\n\tmeasures = %#p\n",
			stats->time_frame,
			stats->mark_wtime,
			stats->measures);
		vrFprintf(file, "\r}\n");
		break;
	}
}


/******************************************************************/
static void _ProcStatsParseArgs(vrProcessStats *stats, char *args)
{
	/*****************************************/
	/** Argument format: "label" "=" string **/
	/*****************************************/
	vrArgParseString(args, "label", &(stats->label));

	/***********************************************************/
	/** Argument format: "calc" "=" { "on" | "off" | number } **/
	/***********************************************************/
	vrArgParseBool(args, "calc", &(stats->calc_flag));

	/***********************************************************/
	/** Argument format: "show" "=" { "on" | "off" | number } **/
	/***********************************************************/
	vrArgParseBool(args, "show", &(stats->show_flag));

	/****************************************/
	/** Argument format: "mask" "=" number **/
	/****************************************/
	vrArgParseInteger(args, "mask", &(stats->show_mask));

	/****************************************/
	/** Argument format: "xloc" "=" number **/
	/****************************************/
	vrArgParseFloat(args, "xloc", &(stats->xloc));

	/*****************************************/
	/** Argument format: "width" "=" number **/
	/*****************************************/
	vrArgParseFloat(args, "width", &(stats->width));

	/****************************************/
	/** Argument format: "yloc" "=" number **/
	/****************************************/
	vrArgParseFloat(args, "yloc", &(stats->yloc));

	/***************************************/
	/** Argument format: "top" "=" number **/
	/***************************************/
	vrArgParseFloat(args, "top", &(stats->top_time));

	/********************************************/
	/** Argument format: "interval" "=" number **/
	/********************************************/
	vrArgParseFloat(args, "interval", &(stats->hline_interval));

	/*****************************************/
	/** Argument format: "scale" "=" number **/
	/*****************************************/
	vrArgParseFloat(args, "scale", &(stats->time_scale));

	/*****************************************************************/
	/** Argument format: "bg" "=" number, number, number [, number] **/
	/*****************************************************************/
	vrArgParseFloatList(args, "bg", &(stats->back_color), 4);

}


/******************************************************************/
vrProcessStats *vrProcessStatsCreate(char *label, int elements, char *args)
{
	vrProcessStats	*stats;
	int		count;

	stats = vrShmemAlloc0(sizeof(vrProcessStats));

	/*********************************/
	/* set values given by arguments */
	stats->label = vrShmemStrDup(label);
	stats->elements = elements;

	/**************************************/
	/* set default values for most fields */
	stats->calc_flag = 0x01;
	stats->show_flag = 0x01;
	stats->show_mask = pow(2, elements) - 1;

	stats->xloc = 0.000;			/* TODO: currently unused */
	stats->width = 0.00;			/* TODO: currently unused */

	stats->yloc = 0.025;
	stats->top_time = 0.100;		/* 100ms = 10Hz */
	stats->hline_interval = 0.020;		/* line every 20ms */
	stats->time_scale = 1.0;
	stats->frames = 100;

	stats->back_color[0] = 0.20;		/* dark semi-transparent gray */
	stats->back_color[1] = 0.20;
	stats->back_color[2] = 0.20;
	stats->back_color[3] = 0.50;

	stats->label_color[0] = 0.90;		/* 90% white -- opaque */
	stats->label_color[1] = 0.90;
	stats->label_color[2] = 0.90;
	stats->label_color[3] = 1.00;

	stats->elem_labels = (char **)vrShmemAlloc0(sizeof(char *) * elements);
	stats->elem_colors = (float **)vrShmemAlloc0(sizeof(float *) * elements);
	for (count = 0; count < elements; count++) {
		/* assign a default label for each segment */
		stats->elem_labels[count] = vrShmemStrDup("*");

		/* assign a default color for each segment */
		stats->elem_colors[count] = (float *)vrShmemAlloc0(sizeof(float) * 4);
		switch (count % 8) {
		case 0:	/* red */
			stats->elem_colors[count][0] = 0.95;
			stats->elem_colors[count][1] = 0.20;
			stats->elem_colors[count][2] = 0.20;
			break;
		case 1:	/* white */
			stats->elem_colors[count][0] = 0.95;
			stats->elem_colors[count][1] = 0.95;
			stats->elem_colors[count][2] = 0.95;
			break;
		case 2:	/* cyan */
			stats->elem_colors[count][0] = 0.20;
			stats->elem_colors[count][1] = 0.95;
			stats->elem_colors[count][2] = 0.95;
			break;
		case 3: /* dark yellow (ie. brown) */
			stats->elem_colors[count][0] = 0.75;
			stats->elem_colors[count][1] = 0.47;
			stats->elem_colors[count][2] = 0.00;
			break;
		case 4: /* green */
			stats->elem_colors[count][0] = 0.00;
			stats->elem_colors[count][1] = 1.00;
			stats->elem_colors[count][2] = 0.00;
			break;
		case 5:	/* dark purple */
			stats->elem_colors[count][0] = 0.40;
			stats->elem_colors[count][1] = 0.00;
			stats->elem_colors[count][2] = 1.00;
			break;
		case 6: /* rose */
			stats->elem_colors[count][0] = 0.95;
			stats->elem_colors[count][1] = 0.54;
			stats->elem_colors[count][2] = 0.52;
			break;
		case 7: /* orange */
			stats->elem_colors[count][0] = 0.95;
			stats->elem_colors[count][1] = 0.51;
			stats->elem_colors[count][2] = 0.12;
			break;
		}
		stats->elem_colors[count][3] = 1.0;
	}

	/************************************/
	/* override defaults with arguments */
	_ProcStatsParseArgs(stats, args);

	/************************************/
	/* allocate storage of measurements */
	stats->measures = (vrTime *)vrShmemAlloc0(elements * stats->frames * sizeof(vrTime));

	/*********************************/
	/* initialize the timer settings */
	stats->time_frame = 0;
	stats->mark_wtime = vrCurrentWallTime();

	return (stats);
}


/******************************************************************/
/* NOTE: usually the return value of how much time was measured is */
/*   ignored by the calling process, but there may be cases where  */
/*   it is used -- although the initial need in vrFrame() turned   */
/*   out not to work because it needed a time delta even when the  */
/*   statistics calculation is turned off.                         */
/* NOTE: the "sum_flag" is set to 1 when there are multiple measurements   */
/*   that will be summed into a single value.  For example, when rendering */
/*   for multiple eyes, the render time for each eye will be added into    */
/*   the resultant measurement.                                            */
vrTime vrProcessStatsMark(vrProcessStats *stats, int element, unsigned int sum_flag)
{
	int	frame_start;

	/* If no statistics data then return immediately */
	if (stats == NULL)
		return 0.0;

	/* When the calculation flag is off, do nothing */
	if (!stats->calc_flag)
		return 0.0;

	frame_start = stats->elements * stats->time_frame;

	/***************************************************************/
	/** store time -- sum to current value if the sum_flag is set **/

	/* NOTE: by initial subtracting of the mark_wtime we only need to call vrCurrentWallTime() once */
	if (sum_flag)
		stats->measures[frame_start + element] -= stats->mark_wtime;
	else	stats->measures[frame_start + element] = -stats->mark_wtime;

	stats->mark_wtime = vrCurrentWallTime();

	stats->measures[frame_start + element] += stats->mark_wtime;

	return (stats->measures[frame_start + element]);
}


/******************************************************************/
void vrProcessStatsNextFrame(vrProcessStats *stats)
{
	int	count;
	int	frame_start;

	/* If no statistics data then return immediately */
	if (stats == NULL)
		return;

	/* When the calculation flag is off, do nothing */
	if (!stats->calc_flag)
		return;

	stats->time_frame++;
	stats->time_frame %= stats->frames;

	frame_start = stats->elements * stats->time_frame;

	/* clear all the times for this frame -- needed for summation elements */
	for (count = 0; count < stats->elements; count++)
		stats->measures[frame_start + count] = 0.0;
}

