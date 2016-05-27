/* ======================================================================
 *
 * HH   HH         vr_config.h
 * HH   HH         Author(s): Ed Peters, Bill Sherman
 * HHHHHHH         Created: June 4, 1998
 * HH   HH         Last Modified: May 24, 2006
 * HH   HH
 *
 * Header file for FreeVR configuration information.  Defines the
 * configuration info structure.
 *
 * vrConfigure() will read in resource files from the predefined
 * path (see the #defines below) and process command-line arguments,
 * as well.  Before returning, it may also use a helper routine to
 * print the configuration info.
 *
 * Copyright 2014, Bill Sherman & Friends, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#ifndef __VRCONFIG_H__
#define __VRCONFIG_H__

#include <stdio.h>
#include <stdarg.h>

#include "vr_system.h"
#include "vr_visren.h"
#include "vr_callback.h"
#include "vr_input.h"


#ifdef __cplusplus
extern "C" {
#endif


/*******************************************************************/
/* various compile-time-configurable options -- resource filename, */
/* search path, and environment variables, and shared memory size  */
#define VRCONFIG_DEFAULTHOME "/usr/local/freevr"
#define VRCONFIG_HOME_ENVVAR "FREEVR_HOME"
#define VRCONFIG_RCFILENAME ".freevrrc"
#define VRCONFIG_RC_ENVVAR "FREEVR"

/* TODO: should default_debug_level be 1 or 0 for releases? */
/*   1/7/2003: I think "2" is actually a better value because that prints */
/*     information printed using vrMsgPrintf() functions and is the       */
/*     "almost always" setting.                                           */
/*   1/13/2003: Now I think "5" is even better, because than includes     */
/*     errors during configuration, plus a couple other low values.  In   */
/*     fact, 25 may be even better as a default, but we'll go with 5 now. */
/*   5/24/2006: There are some configuration warnings (10) that I thought */
/*     would be important, so I'm setting the value to that.              */
#define DEFAULT_DEBUG_LEVEL 10
#define DEFAULT_PROCESSOR_LOCK 0
#define DEFAULT_VISRENMODE VRVISREN_MONO
#define DEFAULT_PROC_LOCKING_CMD "echo \"Lock command has not been set.\""
#define DEFAULT_PROC_UNLOCKING_CMD "echo \"Unlock command has not been set.\""

/* Set the names of default configuration objects */
/*   Warning: changing these names will also affect system and user */
/*   configuration files.                                           */
#define DEFAULT_USER_NAME "default-user"		/* frequently referenced by "eyelist" definitions */
#define DEFAULT_PROP_NAME "default"
#define DEFAULT_INDEV_NAME "default"
#define DEFAULT_WINDOW_NAME "default"			/* often referred to by "xwindows" inputs in config */
#define DEFAULT_EYELIST_NAME "default"
#define DEFAULT_INPUTPROC_NAME "default-input"
#define DEFAULT_VISRENPROC_NAME "default-visren"
#define DEFAULT_TELNETPROC_NAME "default-telnet"


/********************************************************************/
/* Configuration information structure -- contains pointers to most */
/* all the configurable info structures of the system.  Only one of */
/* these is (usually) ever created.                                 */
typedef struct vrConfigInfo_st {
		/***************************/
		/* Generic Object field(s) */
		vrObject	object_type;	/* The type of this object (ie. VROBJECT_CONFIG)  */
		/* NOTE: the rest of the generic object fields not included with individual inputs*/
		/* TODO: consider adding the other object fields to the input structures.         */

		/***************************/
		vrContextInfo	*context;	/* pointer to the vrContext memory structure */
		int		configured;	/* flag that indicates if config stuff is done */
		int		system_init;	/* flag indicating whether system has been completely initialized */

		char		*config_print;	/* CONFIG: Name of file to write the actual config*/

		char		*system_name;	/* CONFIG: Name of the system to use. */
		vrSystemInfo	*system;
#if 0 /* 02/20/2014 -- these two configuration options are not used -- see vrObjectLists */
		int		num_systems;	/* TODO: use this?  CONFIG: (as they are declared/defined) number of known systems */
		vrSystemInfo	*systems;	/* TODO: use this?  pointer to array of possible systems */
#endif

		/*** Global default values ***/
		vrSystemSettings defaults;	/* CONFIG: values for many inheritable fields */

		/*** Values for the system in use (ie. filled in based on "usesystem" value) ***/
		int		num_procs;	/* CONFIG: (as specified by system in use) number of processes */
		vrProcessInfo	**procs;	/* pointer to array of processes. */
		int		procs_init;	/* flag indicating whether processes have all been started */

		int		num_windows;	/* CONFIG: (as specified by system in use) number of windows */
		vrWindowInfo	**windows;	/* pointer to array of windows. */
		int		windows_init;	/* flag indicating whether windows have all been initialized */
#ifdef GFX_PERFORMER
		int		num_pipes;	/* number of rendering pipes granted by the system */
		int		num_pipes_used;	/* number of rendering pipes used so far */
#endif

		int		num_eyelists;	/* CONFIG: (as specified by system in use) number of eyelists */
		vrEyelistInfo	**eyelists;	/* pointer to array of eyelists. */

		int		num_eyes;	/* CONFIG: (as specified by system in use) number of eyes */
		vrEyeInfo	**eyes;		/* pointer to array of eyes. */

		int		num_users;	/* CONFIG: (as specified by system in use) number of users */
		vrUserInfo	**users;	/* pointer to array of users. */
		int		users_init;	/* flag indicating whether users have all been initialized */

		int		num_props;	/* CONFIG: (as specified by system in use) number of props */
		vrPropInfo	**props;	/* pointer to array of props. */
		int		props_init;	/* flag indicating whether props have all been initialized */

		char		*input_map_name;/* CONFIG: (as specified by system in use) name of the input map to use */
		void		*input_map;	/* CONFIG: (as specified by system in use) TODO: input device map */

		int		num_input_devices;/* CONFIG: (as specified by system in use) number of indevs */
		vrInputDevice	**input_devices;/* pointer to array of input devices */
		int		inputs_init;	/* flag indicating whether inputs have all been initialized */

	} vrConfigInfo;


/*****************************/
/*** function declarations ***/
void		vrConfigInitialize(vrConfigInfo *config);
vrContextInfo	*vrConfigure(int *argc, char **argv, char **appargs);
void		vrFprintConfig(FILE *file, vrConfigInfo *config, vrPrintStyle style);

char		*vrEvaluateVariable(vrContextInfo *context, char *variable_name);
void		vrSetNear(vrContextInfo *context, double newnear);
void		vrSetFar(vrContextInfo *context, double newnear);


#ifdef __cplusplus
}
#endif

#endif
