/* ======================================================================
 *
 * HH   HH         vr_system.h
 * HH   HH         Author(s): Bill Sherman
 * HHHHHHH         Created: December 20, 1998
 * HH   HH         Last Modified: April 3, 2012
 * HH   HH
 *
 * Header file for FreeVR system information.  Includes the
 * vrContext info structure (which contains ptrs to the other
 * primary info structures -- input & config), which is the
 * single entry point to system configuration.
 *
 * vrStart() will initialize shared memory and then then initialize
 * the main data structure, call the configuration reader, and spawn
 * all the subprocesses.
 *
 * Copyright 2015, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/*************************************************************************

USAGE:
	vrStart() will call vrConfigure() and then will start the appropriate
	processes/threads running.  vrExit() will stop all spawned processes/threads.
	You can also start/stop processes one at a time.  A process should be
	responsible for its own cleanup.  Processes are killed using the SIGINT call.

*************************************************************************/
#ifndef __VRSYSTEM_H__
#define __VRSYSTEM_H__

#include <stdio.h>

#include "vr_context.h"		/* which includes vr_shmem.h */
#include "vr_shmem.h"		/* which includes vr_enums.h & vr_math.h */

#ifdef __cplusplus
extern "C" {
#endif


/***************************************/
/** Compile-time configurable options **/
#define VRCONFIG_SHMEM_SIZE (16*1024*1024)	/* Default size of shared memory (ie. 16 Meg) */
#define FREEVRVERSION "FreeVR Version 0.6f"	/* Version string */

#define VRSYSTEM_SPAWNALL -1			/* Unused: whether to spawn off processes (I guess)*/


/***********************************************************************************/
/* Array indices for statistics measurements for the simulation (ie. main) process */
#define	VR_TIME_SIM	0
#define	VR_TIME_SIM2	1
#define	VR_TIME_PAUSE	2
#define	VR_TIME_SLEEP	3
#define VR_TIME_VP0	3

/*********************************************************************/
/* vrSystemSettings: A structure with values that are inherited from */
/*   one object to a higher ranking object.  All objects that work   */
/*   with configuration information that is inherited should include */
/*   this structure as a field, and not directly include them.  Each */
/*   field can have its own hierarchy of inheritance.  Thus, some    */
/*   fields will not be used by all the objects with a default       */
/*   structure.  The usual order of inheritance is at the beginning  */
/*   of the comment for each field.                                  */
/*********************************************************************/
typedef struct vrSystemSettings_st {

		/* debug output fields */
		int		debug_level;	/* [process || default]: sets the number below which Dbg messages should be printed. */
		int		debug_exact;	/* [process || default]: specify an exact debug level to also be printed */

		/* major structure printing fields */
		vrPrintStyle	pre_context_print; /* [system, default]: specify the style to print the vrContextInfo data before startup */
		vrPrintStyle	pre_config_print;  /* [system, default]: specify the style to print the vrConfigInfo data before startup */
		vrPrintStyle	pre_input_print;   /* [system, default]: specify the style to print the vrInputInfo data before startup */
		vrPrintStyle	post_context_print;/* [system, default]: specify the style to print the vrContextInfo data after startup */
		vrPrintStyle	post_config_print; /* [system, default]: specify the style to print the vrConfigInfo data after startup */
		vrPrintStyle	post_input_print;  /* [system, default]: specify the style to print the vrInputInfo data after startup */

		/* error handling fields */
		char		*exec_start;	/* [system, default * process]: shell command to do at startup */
		char		*exec_stop;	/* [system, default * process]: shell command to do at standdown */
		char		*exec_uponerror;/* [system, default]: shell command to do when encountering an error during startup */
		int		exit_uponerror;	/* [system, default]: flag determining whether to exit the app on startup errors */

		/* process locking fields */
		int		lock_proc;	/* [process && (system, default)]: Whether or not processes are locked. */
		char		*proc_lock_cmd;	/* [default]: The unix command for locking a processor to a process */
		char		*proc_unlock_cmd;/*[default]: The unix command for unlocking a processor */

		/* visual rendering fields */
		vrVisrenModeType visrenmode;	/* [window, system, default]: default system visren mode. */
		char		*eyelist_name;	/* [window, system, default]: The name of the default eyelist for this system */
		float		near_clip;	/* [window, user, system, default]: near clipping plane for window */
		float		far_clip;	/* [window, user, system, default]: far clipping plane for window */

		/* TODO: add fields for world transformation data [user, system, default] */
	} vrSystemSettings;


/**************************************************************/
/* vrSystemInfo: A structure containing all the details about */
/*   a particular VR system configuration.                    */
/**************************************************************/
typedef struct vrSystemInfo_st {

		/* Generic Object fields */
		vrObject	object_type;	/* The type of this object (ie. VROBJECT_SYSTEM) */
		int		id;		/* numeric id for this system. */
		char		*name;		/* CONFIG: name of this system */
		int		malleable;	/* CONFIG: Whether the data of this system can be modified */
	struct	vrSystemInfo_st	*next;		/* the next system in a linked list */
		vrContextInfo	*context;	/* pointer to the vrContext memory structure */
		int		line_created;	/* line number of where this object was created in config */
		int		line_lastmod;	/* line number of last time this object modified in config */
		char		file_created[512];/* file (or other) where this object was created*/
		char		file_lastmod[512];/* file (or other) where this object was created*/

		/* Fields specific to Systems */

		vrSystemSettings settings;	/* CONFIG: values for many inheritable fields */

		vrUIStyle	type;		/* CONFIG: the type of interface hardware of this system */
		int		spawn_procs;	/* CONFIG: the number of processes to spawn off */
		int		num_procs;	/* CONFIG: the number of processes in this system */
		char		**proc_names;	/* CONFIG-ONLY: the names of the processes used by this system */
	struct vrProcessInfo_st	**procs;	/* CONFIG: pointers to all the processes of this system */

		char		*master_name;	/* CONFIG: the name of the master machine in this system */
		char		**slave_names;	/* CONFIG: the names of all the Slaves in this system */
		int		num_slaves;	/* CONFIG: the number of slaves in this system */

		char		*input_map_name;/* CONFIG: the name of the input map used by this system */
		void		*input_map;	/* (perhaps???) a pointer to the input_map of this system (as yet an undefined struct) */
	} vrSystemInfo;



/***************************/
/** Function Declarations **/

char		*vrSystemStatusName(vrSystemStatus status);
char		*vrVisrenmodeTypeName(vrVisrenModeType mode);
char		*vrUIStyleName(vrUIStyle style);
vrUIStyle	vrUIStyleValue(char *name);

void		vrSettingsClear(vrSystemSettings *settings);
void		vrFprintSystemSettings(FILE *file, vrSystemSettings *settings, vrPrintStyle style);
void		vrSystemClear(vrSystemInfo *system);
void		vrSystemCopy(vrSystemInfo *dest_object, vrSystemInfo *src_object);
void		vrFprintSystemInfo(FILE *file, vrSystemInfo *sysinfo, vrPrintStyle style);
void		vrFprintContext(FILE *file, vrContextInfo *context, vrPrintStyle style);

vrContextInfo	*vrContextInitialize(void);
void		vrSystemSetName(char *name);
void		vrSystemSetAuthors(char *authors);
void 		vrSystemSetExtraInfo(char *info);
void		vrSystemSetStatusDescription(char *info);
void		vrSystemSimCategory(int tag);
void		vrStart(void);
void		vrExit();	/* TODO: add "void" for version 0.6a -- actually, will add the vrContext argument */
int		vrSystemInitialized(void);
int		vrProcessesInitialized(void);
int		vrFrame();


#ifdef __cplusplus
}
#endif

#endif
