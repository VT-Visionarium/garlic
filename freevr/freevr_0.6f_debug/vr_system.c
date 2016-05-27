/* ======================================================================
 *
 *  CCCCC          vr_system.c
 * CC   CC         Author(s): Bill Sherman, John Stone
 * CC              Created: December 20, 1998
 * CC   CC         Last Modified: September 1, 2014
 *  CCCCC
 *
 * Code file for FreeVR system initialization
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include "vr_system.h"
#include "vr_debug.h"
#include "vr_config.h"
#include "vr_objects.h"
#include "vr_parse.h"
#include "vr_utils.h"
#ifdef SEM_TCP
#  include "vr_sem_tcp.h"
#endif


/* Set a system variable with the version so applications   */
/*   will have an embedded reference to the library against */
/*   which they were linked.                                */
char	*vrLibraryVersion = FREEVRVERSION
	" -- library compiled " __DATE__
#ifdef HOST
	" on " HOST
#endif
#ifdef ARCH
	"[" ARCH "]"
#endif
#ifdef MP_PTHREADS
	" pt1"
#endif
#ifdef MP_PTHREADS2
	" pt2"
#endif
#ifdef MP_NONE
	" st"
#endif
	"";



/********************************************************/
/* system-wide global for accessing context information */
	vrContextInfo	*vrContext = NULL;


/*************************************************************************/
/* Functions */


/***************************************************/
char *vrSystemStatusName(vrSystemStatus status)
{
	switch (status) {
	case VRSTATUS_UNITIALIZED:	return "uninitialized";
	case VRSTATUS_INITIALIZING:	return "initializing";
	case VRSTATUS_RUNNING:		return "running";
	case VRSTATUS_TERMINATING:	return "terminating";
	case VRSTATUS_TERMINATED:	return "terminated";
	}

	return "unknown";
}


/***************************************************/
char *vrVisrenmodeTypeName(vrVisrenModeType mode)
{
	switch (mode) {
	case VRVISREN_MONO:		return "mono"; break;
	case VRVISREN_LEFT:		return "left"; break;
	case VRVISREN_RIGHT:		return "right"; break;
	case VRVISREN_DUALFB:		return "dualfb"; break;
	case VRVISREN_DUALVP:		return "dualvp"; break;
	case VRVISREN_ANAGLYPHIC:	return "anaglyphic"; break;
	}

	return "unknown";
}


/***************************************************/
char *vrUIStyleName(vrUIStyle style)
{
	switch (style) {
	case VRUI_UNKNOWN:		return "unknown";
	case VRUI_DEFAULT:		return "default";
	case VRUI_SIMULATOR:		return "simulator";
	case VRUI_STATIONARY:		return "stationary";
	case VRUI_HBD:			return "head-based";
	case VRUI_HAND:			return "hand-based";
	case VRUI_WALL:			return "wall";
	case VRUI_IMMERSADESK:		return "immersadesk";
	case VRUI_WORKBENCH:		return "workbench";
	case VRUI_PIT:			return "pit";
	case VRUI_DESKTOP:		return "desktop";
	}

	return "unknown";
}


/************************************************************/
vrUIStyle vrUIStyleValue(char *name)
{
	if      (!strcasecmp(name, "unknown"))		return VRUI_UNKNOWN;
	if      (!strcasecmp(name, "default"))		return VRUI_DEFAULT;
	else if (!strcasecmp(name, "sim"))		return VRUI_SIMULATOR;
	else if (!strcasecmp(name, "simulator"))	return VRUI_SIMULATOR;
	else if (!strcasecmp(name, "stationary"))	return VRUI_STATIONARY;
	else if (!strcasecmp(name, "cave"))		return VRUI_STATIONARY;
	else if (!strcasecmp(name, "cube"))		return VRUI_STATIONARY;
	else if (!strcasecmp(name, "ipd"))		return VRUI_STATIONARY;
	else if (!strcasecmp(name, "head"))		return VRUI_HBD;
	else if (!strcasecmp(name, "hbd"))		return VRUI_HBD;
	else if (!strcasecmp(name, "hmd"))		return VRUI_HBD;
	else if (!strcasecmp(name, "hand"))		return VRUI_HAND;
	else if (!strcasecmp(name, "wall"))		return VRUI_WALL;
	else if (!strcasecmp(name, "immersadesk"))	return VRUI_IMMERSADESK;
	else if (!strcasecmp(name, "workbench"))	return VRUI_WORKBENCH;
	else if (!strcasecmp(name, "pit"))		return VRUI_PIT;
	else if (!strcasecmp(name, "desktop"))		return VRUI_DESKTOP;
	else {
		vrErrPrintf("Unknown UIStyle type '%s' using 'unknown' type\n", name);
		return VRUI_UNKNOWN;
	}

	/* Can't get to this statement */
	return VRUI_UNKNOWN;
}


/***************************************************/
void vrSettingsClear(vrSystemSettings *settings)
{
	settings->debug_level = 1;
	settings->debug_exact = 0;

	settings->pre_context_print = def;
	settings->pre_config_print = def;
	settings->pre_input_print = def;
	settings->post_context_print = def;
	settings->post_config_print = def;
	settings->post_input_print = def;

	settings->exec_start = NULL;
	settings->exec_stop = NULL;
	settings->exec_uponerror = NULL;
	settings->exit_uponerror = -1;			/* (i.e. inherit) */

	settings->lock_proc = VRTOKEN_DEFAULT;		/* (i.e. inherit, but from whom?) */
	settings->proc_lock_cmd = NULL;
	settings->proc_unlock_cmd = NULL;

	settings->visrenmode = VRVISREN_DEFAULT;
	settings->eyelist_name = NULL;
	settings->near_clip = -1;			/* (i.e. inherit) */
	settings->far_clip =  -1;			/* (i.e. inherit) */
}


/*********************************************/
void vrFprintSystemSettings(FILE *file, vrSystemSettings *settings, vrPrintStyle style)
{
	switch (style) {
	default:
	case verbose:
		vrFprintf(file, "\rInheritable System Settings = {\n");
		vrFprintf(file, "\r"
			"\tdebug_level = %d\n\tdebug_exact = %d\n"
			"\tpre_context_print = %s\n"
			"\tpre_config_print = %s\n"
			"\tpre_input_print = %s\n"
			"\tpost_context_print = %s\n"
			"\tpost_config_print = %s\n"
			"\tpost_input_print = %s\n"
			"\texec_start = \"%s\"\n"
			"\texec_stop = \"%s\"\n"
			"\texec_uponerror = \"%s\"\n"
			"\texit_uponerror = %d\n"
			"\tlock_proc = %d\n"
			"\tproc_lock_cmd = \"%s\"\n"
			"\tproc_unlock_cmd = \"%s\"\n"
			"\tvisrenmode = \"%s\" (%d)\n"
			"\teyelist_name = \"%s\"\n"
			"\tnear_clip = %.3f\n"
			"\tfar_clip = %.3f\n",
			settings->debug_level,
			settings->debug_exact,
			vrPrintStyleName(settings->pre_context_print),
			vrPrintStyleName(settings->pre_config_print),
			vrPrintStyleName(settings->pre_input_print),
			vrPrintStyleName(settings->post_context_print),
			vrPrintStyleName(settings->post_config_print),
			vrPrintStyleName(settings->post_input_print),
			(settings->exec_start == NULL ? "(nil)" : settings->exec_start),
			(settings->exec_stop == NULL ? "(nil)" : settings->exec_stop),
			(settings->exec_uponerror == NULL ? "(nil)" : settings->exec_uponerror),
			settings->exit_uponerror,
			settings->lock_proc,
			(settings->proc_lock_cmd == NULL ? "(nil)" : settings->proc_lock_cmd),
			(settings->proc_unlock_cmd == NULL ? "(nil)" : settings->proc_unlock_cmd),
			vrVisrenModeName(settings->visrenmode), settings->visrenmode,
			(settings->eyelist_name == NULL ? "(nil)" : settings->eyelist_name),
			settings->near_clip,
			settings->far_clip);
		vrFprintf(file, "\r}\n");
		break;

	case file_format:
		vrFprintf(file, "set DebugLevel = %d;\n", settings->debug_level);
		vrFprintf(file, "set DebugThisToo = %d;\n", settings->debug_exact);
		vrFprintf(file, "\n");

		vrFprintf(file, "setDefault PreContextPrint = %s;\n", vrPrintStyleName(settings->pre_context_print));
		vrFprintf(file, "setDefault PreConfigPrint = %s;\n", vrPrintStyleName(settings->pre_config_print));
		vrFprintf(file, "setDefault PreInputPrint = %s;\n", vrPrintStyleName(settings->pre_input_print));
		vrFprintf(file, "setDefault PostContextPrint = %s;\n", vrPrintStyleName(settings->post_context_print));
		vrFprintf(file, "setDefault PostConfigPrint = %s;\n", vrPrintStyleName(settings->post_config_print));
		vrFprintf(file, "setDefault PostInputPrint = %s;\n", vrPrintStyleName(settings->post_input_print));
		vrFprintf(file, "\n");

		if (settings->exec_start != NULL)
			vrFprintf(file, "setDefault ExecAtStart = \"%s\";\n", settings->exec_start);
		else	vrFprintf(file, "#setDefault ExecAtStart = NULL;\n");
		if (settings->exec_stop != NULL)
			vrFprintf(file, "setDefault ExecAtStop = \"%s\";\n", settings->exec_stop);
		else	vrFprintf(file, "#setDefault ExecAtStop = NULL;\n");
		if (settings->exec_uponerror != NULL)
			vrFprintf(file, "setDefault ExecUponError = \"%s\";\n", settings->exec_uponerror);
		else	vrFprintf(file, "#setDefault ExecUponError = NULL;\n");
		vrFprintf(file, "setDefault ExitUponError = %d;\n", settings->exit_uponerror);
		vrFprintf(file, "\n");

		vrFprintf(file, "setDefault LockCPU = %d;\n", settings->lock_proc);
		if (settings->proc_lock_cmd != NULL)
			vrFprintf(file, "setDefault LockCommand = \"%s\";\n", settings->proc_lock_cmd);
		else	vrFprintf(file, "#setDefault LockCommand = NULL;\n");
		if (settings->proc_unlock_cmd != NULL)
			vrFprintf(file, "setDefault UnLockCommand = \"%s\";\n", settings->proc_unlock_cmd);
		else	vrFprintf(file, "#setDefault UnLockCommand = NULL;\n");
		vrFprintf(file, "\n");

		vrFprintf(file, "setDefault VisrenMode = \"%s\";\n", vrVisrenModeName(settings->visrenmode));
		if (settings->eyelist_name != NULL)
			vrFprintf(file, "setDefault EyeList = \"%s\";\n", settings->eyelist_name);
		else	vrFprintf(file, "#setDefault EyeList = NULL;  [will use \"default\" by default]\n");
		if (settings->near_clip != -1)
			vrFprintf(file, "setDefault NearClip = %.3f;\n", settings->near_clip);
		else	vrFprintf(file, "#setDefault NearClip = \"default\";\n");
		if (settings->far_clip != -1)
			vrFprintf(file, "setDefault FarClip = %.3f;\n", settings->far_clip);
		else	vrFprintf(file, "#setDefault FarClip = \"default\";\n");
		vrFprintf(file, "\n");
	}
}


/***************************************************/
void vrSystemClear(vrSystemInfo *system)
{
	system->type = VRUI_UNKNOWN;
	system->input_map_name = NULL;
	system->spawn_procs = VRSYSTEM_SPAWNALL;
	system->num_procs = 0;
	system->proc_names = NULL;
	system->procs = NULL;

	system->master_name = NULL;		/* the cluster master */
	system->slave_names = NULL;		/* the cluster slaves list */
	system->num_slaves = 0;			/* the number of cluster slaves */

	/* the inheritable parameters */
	vrSettingsClear(&system->settings);
	system->settings.debug_level = DEFAULT_DEBUG_LEVEL;
}


#if 0
void *vrCopyList(void *src_list, int list_len, int element_size)
{
	int	count;

	if (list_len == 0)
		return NULL;

	src_list = vrShmemAlloc0(list_len * element_size);
	for (count = 0; count < list_len; count++) {
	}
}
#endif


/***************************************************/
void vrSystemCopy(vrSystemInfo *dest_object, vrSystemInfo *src_object)
{
	void	*dest_mem;
	void	*src_mem;
	int	memlen;
	int	count;

	/* copy only the memory after the generic vrObjectInfo stuff */
	dest_mem = (char *)dest_object + sizeof(vrObjectInfo);
	src_mem = (char *)src_object + sizeof(vrObjectInfo);
	memlen = sizeof(vrSystemInfo) - sizeof(vrObjectInfo);
	memcpy(dest_mem, src_mem, memlen);

	/* make independent copy of some fields */
#if 0 /* TODO: I don't think this method (using vrCopyList() above) will work  [12/23/2008: why not?  Was it because all the vrProcessInfo stuff also had to be copied?] */
	dest_object->proc_names = (char **)vrCopyList(src_object->proc_names, src_object->num_procs, sizeof(*src_object->proc_names));
	dest_object->procs = (vrProcessInfo **)vrCopyList(src_object->procs, src_object->num_procs, sizeof(*src_object->procs));
#else
	dest_object->proc_names = (char **)vrShmemAlloc0(src_object->num_procs * sizeof(char *));
	dest_object->procs = (vrProcessInfo **)vrShmemAlloc0(src_object->num_procs * sizeof(vrProcessInfo *));
	for (count = 0; count < src_object->num_procs; count++) {
		dest_object->proc_names[count] = vrShmemStrDup(src_object->proc_names[count]);
		if (src_object->procs != NULL)
			dest_object->procs[count] = vrShmemMemDup(src_object->procs[count], sizeof(vrProcessInfo *));
	}

	/*   now copy the cluster information */
	dest_object->master_name = vrShmemStrDup(src_object->master_name);
	dest_object->slave_names = (char **)vrShmemAlloc0(src_object->num_slaves * sizeof(char *));
	for (count = 0; count < src_object->num_slaves; count++) {
		dest_object->slave_names[count] = vrShmemStrDup(src_object->slave_names[count]);
	}
#endif
}


/***************************************************/
void vrFprintSystemInfo(FILE *file, vrSystemInfo *sysinfo, vrPrintStyle style)
{
	int	count;

	switch (style) {

	case brief:
		vrFprintf(file, "system '%s' -- processes:", sysinfo->name);
		for (count = 0; count < sysinfo->num_procs; count++) {
			vrFprintf(file, " \"%s\"", sysinfo->proc_names[count]);
		}
		vrFprintf(file, "\n");
		break;
	case one_line:
		vrFprintf(file, "system " BOLD_TEXT "'%s'" NORM_TEXT " -- processes:", sysinfo->name);
		for (count = 0; count < sysinfo->num_procs; count++) {
			vrFprintf(file, " \"%s\"", sysinfo->proc_names[count]);
		}
		vrFprintf(file, "\n");
		break;
	case machine:
		vrFprintf(file, "%s:%d", sysinfo->name, sysinfo->num_procs);
		for (count = 0; count < sysinfo->num_procs; count++) {
			vrFprintf(file, ":%s", sysinfo->proc_names[count]);
		}
		vrFprintf(file, "\n");
		break;

	default:
	case verbose:
		vrFprintf(file, "{\n");
		vrFprintf(file, "\r"
			"\tobject_type = %s (%d)\n\tid = %d\n\tname = \"%s\"\n"
			"\tmalleable = %d\n\tnext = %#p\n"
			"\tCreated at %s, line %d\n"
			"\tLast modified at %s, line %d\n",
			vrObjectTypeName(sysinfo->object_type),
			sysinfo->object_type,
			sysinfo->id,
			sysinfo->name,
			sysinfo->malleable,
			sysinfo->next,
			sysinfo->file_created,
			sysinfo->line_created,
			sysinfo->file_lastmod,
			sysinfo->line_lastmod);
		vrFprintf(file, "\r"
			"\ttype = %s\n"
			"\tspawn_procs = %d\n",
			vrUIStyleName(sysinfo->type),
			sysinfo->spawn_procs);
		vrFprintf(file, "\r"
			"\tnum_procs = %d\n\tproc names = %#p [",
			sysinfo->num_procs,
			sysinfo->proc_names);
		for (count = 0; count < sysinfo->num_procs; count++) {
			vrFprintf(file, " \"%s\"", sysinfo->proc_names[count]);
		}
		vrFprintf(file, " ]\n\tprocs = %#p [",
			sysinfo->procs);
		for (count = 0; count < sysinfo->num_procs; count++) {
			if (sysinfo->procs == NULL)
				vrFprintf(file, " --  ");
			else	vrFprintf(file, " %#p", sysinfo->procs[count]);
		}
		vrFprintf(file, "] \n\tmaster_name = \"%s\" (%#p)\n",
			sysinfo->master_name);
		vrFprintf(file, "\r"
			"\tnum_slaves = %d\n\tslaves names = %#p [",
			sysinfo->num_slaves,
			sysinfo->slave_names);
		for (count = 0; count < sysinfo->num_slaves; count++) {
			vrFprintf(file, " \"%s\"", sysinfo->slave_names[count]);
		}
		vrFprintf(file, "] \n\tinput_map = \"%s\" (%#p)\n",
			(sysinfo->input_map_name == NULL ? "(nil)" : sysinfo->input_map_name),
			sysinfo->input_map);


		/* Here are the inheritible settings */

		vrFprintf(file, "\r"
			"\texec_start = \"%s\"\n"
			"\texec_stop = \"%s\"\n"
			"\texec_uponerror = \"%s\"\n"
			"\texit_uponerror = %d\n",
			(sysinfo->settings.exec_start == NULL ? "(none)" : sysinfo->settings.exec_start),
			(sysinfo->settings.exec_stop == NULL ? "(none)" : sysinfo->settings.exec_stop),
			(sysinfo->settings.exec_uponerror == NULL ? "(none)" : sysinfo->settings.exec_uponerror),
			sysinfo->settings.exit_uponerror);
		vrFprintf(file, "\r"
			"\tdefault visrenmode = \"%s\" (%d)\n"
			"\teyelist system default = \"%s\"\n"
			"\tlock_proc = %d\n"
			"\tproc_lock_cmd = \"%s\"\n"
			"\tproc_unlock_cmd = \"%s\"\n",
			vrVisrenModeName(sysinfo->settings.visrenmode),
			sysinfo->settings.visrenmode,
			(sysinfo->settings.eyelist_name == NULL ? "(none)" : sysinfo->settings.eyelist_name),
			sysinfo->settings.lock_proc,
			(sysinfo->settings.proc_lock_cmd == NULL ? "(none)" : sysinfo->settings.proc_lock_cmd),
			(sysinfo->settings.proc_unlock_cmd == NULL ? "(none)" : sysinfo->settings.proc_unlock_cmd));
		vrFprintf(file, "\r"
			"\tnear_clip = %.2f\n\tfar_clip = %.2f\n",
			sysinfo->settings.near_clip,
			sysinfo->settings.far_clip);
		vrFprintf(file, "\r}\n");
		break;

	case file_format:
		vrFprintf(file, "usesystem \"%s\";\n\n", sysinfo->name);

		vrFprintf(file, "system \"%s\" = {\n", sysinfo->name);

		if (!sysinfo->malleable)
			vrFprintf(file, "\tmalleable = %d;\n", sysinfo->malleable);
		vrFprintf(file, "\tinputmap = \"%s\";\n", sysinfo->input_map_name);
		if (sysinfo->num_procs > 1) {
			vrFprintf(file, "\tprocs =");
			for (count = 1; count < sysinfo->num_procs; count++) {
				vrFprintf(file, " \"%s\"", sysinfo->proc_names[count]);
				if (count+1 < sysinfo->num_procs)
					vrFprintf(file, ",");
			}
			vrFprintf(file, ";\n");
		}

		if (sysinfo->master_name != NULL)
			vrFprintf(file, "\tmaster_name = %s;\n", sysinfo->master_name);
		if (sysinfo->num_slaves > 1) {
			vrFprintf(file, "\tslaves =");
			for (count = 1; count < sysinfo->num_slaves; count++) {
				vrFprintf(file, " \"%s\"", sysinfo->slave_names[count]);
				if (count+1 < sysinfo->num_slaves)
					vrFprintf(file, ",");
			}
			vrFprintf(file, ";\n");
		}

		if (sysinfo->settings.pre_context_print != def)
			vrFprintf(file, "\tPreContextPrint = %s;\n", vrPrintStyleName(sysinfo->settings.pre_context_print));
		else	vrFprintf(file, "\t# Inherit from Global: PreContextPrint = %s;\n", vrPrintStyleName(sysinfo->settings.pre_context_print));
		if (sysinfo->settings.pre_config_print != def)
			vrFprintf(file, "\tPreConfigPrint = %s;\n", vrPrintStyleName(sysinfo->settings.pre_config_print));
		else	vrFprintf(file, "\t# Inherit from Global: PreConfigPrint = %s;\n", vrPrintStyleName(sysinfo->settings.pre_config_print));
		if (sysinfo->settings.pre_input_print != def)
			vrFprintf(file, "\tPreInputPrint = %s;\n", vrPrintStyleName(sysinfo->settings.pre_input_print));
		else	vrFprintf(file, "\t# Inherit from Global: PreInputPrint = %s;\n", vrPrintStyleName(sysinfo->settings.pre_input_print));
		if (sysinfo->settings.post_context_print != def)
			vrFprintf(file, "\tPostContextPrint = %s;\n", vrPrintStyleName(sysinfo->settings.post_context_print));
		else	vrFprintf(file, "\t# Inherit from Global: PostContextPrint = %s;\n", vrPrintStyleName(sysinfo->settings.post_context_print));
		if (sysinfo->settings.post_config_print != def)
			vrFprintf(file, "\tPostConfigPrint = %s;\n", vrPrintStyleName(sysinfo->settings.post_config_print));
		else	vrFprintf(file, "\t# Inherit from Global: PostConfigPrint = %s;\n", vrPrintStyleName(sysinfo->settings.post_config_print));
		if (sysinfo->settings.post_input_print != def)
			vrFprintf(file, "\tPostInputPrint = %s;\n", vrPrintStyleName(sysinfo->settings.post_input_print));
		else	vrFprintf(file, "\t# Inherit from Global: PostInputPrint = %s;\n", vrPrintStyleName(sysinfo->settings.post_input_print));
		vrFprintf(file, "\n");

		if (sysinfo->settings.exec_start != NULL)
			vrFprintf(file, "\texecAtStart = \"%s\";\n", sysinfo->settings.exec_start);
		else	vrFprintf(file, "\t# Inherit from Global: execAtStart = NULL\n");
		if (sysinfo->settings.exec_stop != NULL)
			vrFprintf(file, "\texecAtStop = \"%s\";\n", sysinfo->settings.exec_stop);
		else	vrFprintf(file, "\t# Inherit from Global: execAtStop = NULL\n");
		if (sysinfo->settings.exec_uponerror != NULL)
			vrFprintf(file, "\texecUponError = \"%s\";\n", sysinfo->settings.exec_uponerror);
		else	vrFprintf(file, "\t# Inherit from Global: execUponError = NULL\n");
		if (sysinfo->settings.exit_uponerror != -1)
			vrFprintf(file, "\texitUponError = %d;\n", sysinfo->settings.exit_uponerror);
		else	vrFprintf(file, "\t# Inherit from Global: exitUponError = \"default\";\n");
		vrFprintf(file, "\n");

		if (sysinfo->settings.lock_proc != VRTOKEN_DEFAULT)
			vrFprintf(file, "\tLockCPU = %d;\n", sysinfo->settings.lock_proc);
		else	vrFprintf(file, "\t# Inherit from Global: LockCPU = \"default\";\n", sysinfo->settings.lock_proc);
		if (sysinfo->settings.proc_lock_cmd != NULL)
			vrFprintf(file, "\tLockCommand = \"%s\";\n", sysinfo->settings.proc_lock_cmd);
		else	vrFprintf(file, "\t# Inherit from Global: LockCommand = NULL;\n");
		if (sysinfo->settings.proc_unlock_cmd != NULL)
			vrFprintf(file, "\tUnLockCommand = \"%s\";\n", sysinfo->settings.proc_unlock_cmd);
		else	vrFprintf(file, "\t# Inherit from Global: UnLockCommand = NULL;\n");
		vrFprintf(file, "\n");

		if (sysinfo->settings.visrenmode != VRVISREN_DEFAULT)
			vrFprintf(file, "\tvisrenMode = \"%s\";\n", vrVisrenModeName(sysinfo->settings.visrenmode));
		else	vrFprintf(file, "\t# Inherit from Global: visrenMode = \"%s\";\n", vrVisrenModeName(sysinfo->settings.visrenmode));
		if (sysinfo->settings.eyelist_name != NULL)
			vrFprintf(file, "\teyelist = \"%s\";\n", sysinfo->settings.eyelist_name);
		else	vrFprintf(file, "\t# Inherit from Global: eyes = \"default\";\n");
		if (sysinfo->settings.near_clip != -1)
			vrFprintf(file, "\tNearClip = %.3f;\n", sysinfo->settings.near_clip);
		else	vrFprintf(file, "\t# Inherit from Global: NearClip = \"default\";\n");
		if (sysinfo->settings.far_clip != -1)
			vrFprintf(file, "\tFarClip = %.3f;\n", sysinfo->settings.far_clip);
		else	vrFprintf(file, "\t# Inherit from Global: FarClip = \"default\";\n");
		vrFprintf(file, "\n");

		vrFprintf(file, "}\n\n");
		break;
	}
}



/***************************************************/
void vrFprintContext(FILE *file, vrContextInfo *vrContext, vrPrintStyle style)
{

	if (vrContext == NULL) {
		vrErrPrintf("vrFprintContext(): " RED_TEXT "memory argument is NULL!\n" NORM_TEXT);
		return;
	}

	if (vrContext->object_type != VROBJECT_CONTEXT) {
		vrErrPrintf("vrFprintContext(): " RED_TEXT "memory argument does not point to the vrContextInfo structure!\n" NORM_TEXT);
		return;
	}

	switch(style) {
	case none:
		break;

	case file_format:
	case one_line:
	case machine:
		vrFprintf(file, "TODO: put a special version of context info here\n");
		break;

	default:
	case brief:
		vrFprintf(file, "**************************************************\n");
		vrFprintf(file, "%s\n", vrContext->version);
		vrFprintf(file, "%s\n", vrContext->compile);
		vrFprintf(file, "Home directory = '%s'\n", vrContext->homedir);
		vrFprintf(file, "status = '%s' (%d)\n", vrSystemStatusName(vrContext->status), vrContext->status);
		vrFprintf(file, "shmem_size = %d\n", vrContext->shmem_size);
		vrFprintf(file, "time immemorial = %.2lf\n", vrContext->time_immemorial);
		vrFprintf(file, "**************************************************\n");
		break;

	case everything:
		/* here, this is the same as verbose -- it also means more info */
		/* during the read phase of configuration.                      */
		/* TODO: decide if it should be more.                           */

	case verbose:
		vrFprintf(file, "**************************************************\n");
		vrFprintf(file, "%s\n", vrContext->version);
		vrFprintf(file, "%s\n", vrContext->compile);
		vrFprintf(file, "Application name = %s\n", vrContext->name);
		vrFprintf(file, "Application authors = %s\n", vrContext->authors);
		vrFprintf(file, "Application info = %s\n", vrContext->extra_info);
		vrFprintf(file, "Application status = %s\n", vrContext->status_info);
		vrFprintf(file, "Home directory = '%s'\n", vrContext->homedir);
		vrFprintf(file, "status = '%s' (%d)\n", vrSystemStatusName(vrContext->status), vrContext->status);
		vrFprintf(file, "startup_error = 0x%x\n", vrContext->startup_error);
#if 0
		vrFprintf(file, "%s\n", vrApplicationVersion);	/* TODO: ?? */
#endif
		vrFprintf(file, "shmem_size = %d\n", vrContext->shmem_size);
		vrFprintf(file, "context structure = %#p\n", vrContext);
		vrFprintf(file, "config structure = %#p\n", vrContext->config);
		vrFprintf(file, "input structure = %#p\n", vrContext->input);
		vrFprintf(file, "callbacks structure = %#p\n", vrContext->callbacks);
		vrFprintf(file, "object_lists structure = %#p\n", vrContext->object_lists);
		vrFprintf(file, "time immemorial = %.2lf\n", vrContext->time_immemorial);
		vrFprintf(file, "paused time = %.2lf\n", vrContext->paused_time);
		vrFprintf(file, "pause w-time = %.2lf\n", vrContext->pause_wtime);
		vrFprintf(file, "paused? = %d\n", vrContext->paused);
		if (vrContext->callbacks != NULL) {
			vrFprintf(file, "--------- current callback list:\n");
			vrFprintCallbackList(file, vrContext->callbacks, style);
		}
		vrFprintf(file, "Lock list: head/lock = %#p, tail = %#p\n",
			vrContext->head_lock, vrContext->tail_lock);
#if defined(SEM_SYSVIPC)
		vrFprintf(file, "Lock values: semset = %p, semnum = %d\n",
			vrContext->lock_semset, vrContext->lock_semnum);
#endif
		vrFprintf(file, "Barrier list: head = %#p, tail = %#p, lock = %#p\n",
			vrContext->head_barrier, vrContext->tail_barrier, vrContext->barrier_lock);
#ifdef MP_PTHREADS2
		vrFprintf(file, "pthread process key = %ld\n", vrContext->this_proc_key);
#endif

		vrFprintf(file, "**************************************************\n");
		break;

	}
}


/***************************************************/
vrContextInfo *vrContextInitialize()
{
	vrContextInfo	*context;
static	int		initialized = 0;
	long		actual_arena_size;

	if (initialized)
		return NULL;


	/*****************************************************/
	/*** initialize shared memory so we have somewhere ***/
	/*** to put all the configuration info             ***/
	/*****************************************************/
#if 0 /* don't include this when not debugging this part of the library, or it will always print -- because no Shared memory yet! */
	vrTrace("vrContextInitialize", "before shmem init");
#endif
	actual_arena_size = vrShmemInit(VRCONFIG_SHMEM_SIZE);


	/********************************************/
	/*** initialize the global context struct ***/
	/********************************************/
#if 0 /* don't include this when not debugging this part of the library, or it will always print -- because no Shared memory yet! */
	vrTrace("vrContextInitialize", "before vrContext shmem alloc");
#endif
	context = (vrContextInfo*)vrShmemAlloc0(sizeof(vrContextInfo));
	if (!context) {
		vrErr("Couldn't allocate vrContext memory.");
		exit(1);	/* can't do much if we have no context memory */
	}
	/* set the global context pointer to point to allocated memory */
	vrContext = context;

#ifdef MP_PTHREADS2
	if (pthread_key_create(&context->this_proc_key, NULL) != 0) {
		vrErr("Couldn't create a pthread_key.");
		exit(1);	/* exit -- for now anyway. */
	}
#endif

	/* set the basic context fields */
	context->object_type = VROBJECT_CONTEXT;
	/* TODO: When I get around to having a structure store all the */
	/*   shared memory information, here is where that should be   */
	/*   assigned to the context.                                  */
	/*        context->shmem = vrShmemGetInfo();                   */

	context->version = vrShmemStrDup(FREEVRVERSION);

	context->arch = vrShmemStrDup(""
#ifdef mips /* this #define is understood both by gcc and the native IRIX cc */
		"mips"
#endif
#ifdef _MIPS_SIM
#  if _MIPS_SIM==_ABIO32
		"_o32"
#  endif
#  if _MIPS_SIM==_ABIN32
		"_n32"
#  endif
#  if _MIPS_SIM==_ABI64
		"_n64"
#  endif
#endif
#  ifdef __i386__
		"i386"
#  endif
#  ifdef __x86_64__
		"x86_64"
#  endif
	/* TODO: add any differences between i386 binary formats here */

	/* TODO: add all the other architecture types here */
	);

	context->compile = vrShmemStrDup("Compiled: " __DATE__ " at " __TIME__
#ifdef HOST
		" on " HOST
#endif
#ifdef ARCH
		"[" ARCH "]"
#endif
#ifdef GFX_PERFORMER
		" with"
		" PF"
#elif defined(GFX_OSG)
		" with"
		" OSG/GLX"
#elif defined(WIN_GLX)
		" with"
		" GLX"
#elif defined(WIN_WGL)
		" with"
		" WGL"
#else
		" with"
		" Unknown Rendering/windowing system"
#endif
#ifdef MP_PTHREADS
		" (pt1)"
#endif
#ifdef MP_PTHREADS2
		" (pt2)"
#endif
#ifdef MP_NONE
		" (st)"
#endif
	);
	context->compile_target = vrShmemStrDup(""
#ifdef ARCH
		ARCH
#endif
	);


	if (getenv(VRCONFIG_HOME_ENVVAR) != NULL) {
		context->homedir = vrShmemStrDup(getenv(VRCONFIG_HOME_ENVVAR));
		/* TODO: may want to consider doing a check for the existence  */
		/*   of the directory here, or just let the check be done when */
		/*   the first attempt to read a file in the directory occurs. */
	} else {
		context->homedir = vrShmemStrDup(VRCONFIG_DEFAULTHOME);
	}

	context->name = NULL;				/* must be specified by application program */
	context->authors = NULL;			/* must be specified by application program */
	context->extra_info = NULL;			/* must be specified by application program */
	context->status_info = NULL;			/* must be specified by application program */
	context->shmem_size = actual_arena_size;
	context->startup_error = VRSTARTUP_OKAY;	/* thus far anyway */
	context->status = VRSTATUS_UNITIALIZED;
	context->time_immemorial = vrCurrentWallTime();
	context->paused = 0;
	context->paused_time = 0.0;
	context->pause_wtime = -1.0;
#if defined(SEM_SYSVIPC)
	context->lock_semset = 0;
	context->lock_semnum = 0;
#endif
	context->head_lock = vrLockCreateName(context, "lock list");
	context->tail_lock = context->head_lock;
	context->head_barrier = NULL;
	context->tail_barrier = NULL;
	context->barrier_lock = vrLockCreateName(context, "barrier list");
#if 0 /* this was for debugging, but remove from distribution version */
vrLockTrace(context->barrier_lock, 1);
#endif
	context->print_lock = vrLockCreateName(context, "print lock");
	context->xpixmap_lock = vrLockCreateName(context, "xpixmap lock");
#if 0
	vrLockTrace(context->xpixmap_lock, 1);	/* TODO: get rid of this line once we're done testing this lock */
#endif


	/***************************/
	/* initialize object lists */
	/* NOTE: this needs to happen before the "built-in main" process is defined */
	context->object_lists = (vrObjectLists *)vrShmemAlloc0(sizeof(vrObjectLists));
	if (!context->object_lists) {
		vrErr("Couldn't allocate object list data.");
		vrExit();
		exit(-1);
	}
	vrObjectListsInitialize(context->object_lists);

	/*********************************************/
	/*** setup the main (parent) process info. ***/
	/*********************************************/
	/* vrProcessCreateMainProcessInfo needs vrContext to be allocated */
#ifdef MP_PTHREADS2
	pthread_setspecific(context->this_proc_key, (void *)vrProcessCreateMainProcessInfo(context));
#else
	vrThisProc = vrProcessCreateMainProcessInfo(context);
#endif


	/**************************************************************/
	/** Initialize the three other primary information structures **/
	/**************************************************************/

	/************************/
	/* initialize callbacks */
	context->callbacks = (vrCallbackList *)vrShmemAlloc0(sizeof(vrCallbackList));
	vrCallbackListInitialize(context->callbacks);

	/***************************/
	/* initialize vrConfigInfo */
	context->config = (vrConfigInfo *)vrShmemAlloc0(sizeof(vrConfigInfo));
	if (!context->config) {
		vrErr("Couldn't allocate configuration data.");
		vrExit();
		exit(-1);
	}
	context->config->context = context;
	vrConfigInitialize(context->config);

	/**************************/
	/* initialize vrInputInfo */
	context->input = (vrInputInfo *)vrShmemAlloc0(sizeof(vrInputInfo));
	if (!context->input) {
		vrErr("Couldn't allocate input structure data.");
		vrExit();
		exit(-1);
	}
	context->input->context = context;
	context->input->object_type = VROBJECT_INPUTINFO;	/* NOTE: don't wait for call to vrInputInitialize() */


	/****************************/
	/* set the initialized flag */
	initialized = 1;
	vrTrace("vrContextInitialize", "initialized");

	return (context);
}


/***************************************************/
void vrSystemSetName(char *name)
{
	vrContext->name = vrShmemStrDup(name);
}


/***************************************************/
void vrSystemSetAuthors(char *authors)
{
	vrContext->authors = vrShmemStrDup(authors);
}


/***************************************************/
void vrSystemSetExtraInfo(char *info)
{
	vrContext->extra_info = vrShmemStrDup(info);
}


/***************************************************/
void vrSystemSetStatusDescription(char *info)
{
	if (vrContext->status_info != NULL)
		vrShmemFree(vrContext->status_info);

	vrContext->status_info = vrShmemStrDup(info);
}


/***************************************************/
void vrSystemSimCategory(int tag)
{
	vrProcessInfo	*this_proc = vrThisProc;		/* Ugh: use of global */

	switch (tag) {
	case 1:
		vrProcessStatsMark(this_proc->stats, VR_TIME_SIM, 1);
		break;
	case 2:
		vrProcessStatsMark(this_proc->stats, VR_TIME_SIM2, 1);
		break;
	default:
		vrErrPrintf("vrSystemSimStats(): Warning, the statistics tag must be 0 or 1\n");
		break;
	}

	return;
}


/***************************************************/
#ifdef NOT_CURRENTLY_USED /* { */
static void vrChildDeathSignalHandler(int which)
{

	/* ELP: this error message is simply meaningless without more */
	/* information, so I'm commenting it out for now              */
	/* [8/7/2000] BS: hmmm, seems to have been re-added at some point */
	vrDbgPrintfN(ALWAYS_DBGLVL, "FreeVR: " RED_TEXT "A child process died from signal %d.\n" NORM_TEXT, which);
#if 0
	vrDbgPrintfN(ALWAYS_DBGLVL, "FreeVR: " RED_TEXT "Time of death = %f, number of frames = %d\n" NORM_TEXT, proc_info->frame_time, proc_info->frame_count); /* TODO: the proc_info stuff is not correct need to find a good source for time */
#endif

	vrDbgPrintfN(ALWAYS_DBGLVL, "FreeVR: Shared memory usage:%7ld (%ld freed)\n",
		vrShmemUsage(), vrShmemFreed());

	/* ELP: we'd love to re-add this signal handler (since we might */
	/* have more than one child) but that seems to loop on us ...   */
	/*    signal(SIGCHLD, vrChildDeathSignalHandler);               */
	return;
}
#endif /* } */


/***************************************************/
/* vrStart(): The function that starts it all up.  */
/*   vrConfigure() can be called prior to calling  */
/*   vrStart.  In that case, the context will also */
/*   have already been initialized, so the first   */
/*   two operations of vrStart() will already have */
/*   been completed.                               */
/***************************************************/
void vrStart()
{
	vrContextInfo	*context;
	vrConfigInfo	*config = NULL;

	/****************************/
	/** Do some initialization **/
	/****************************/
	context = vrConfigure(NULL, NULL, NULL);
	context->status = VRSTATUS_INITIALIZING;
	config = context->config;

	/************************************/
	/** Check for configuration errors **/
	/************************************/
	if (config->context->startup_error != VRSTARTUP_OKAY) {
		int	error_flag = config->context->startup_error;
		int	exit_flag;

		/* tell user there was a problem with the configuration */
		vrErrPrintf("vrConfigure(): " RED_TEXT "Problem(s) during configuration (0x%02x):\n" NORM_TEXT, error_flag);
		if (error_flag & VRSTARTUP_BADSHMEM)
			vrErrPrintf(RED_TEXT "\r\t-> Unable to open shared memory.\n" NORM_TEXT);
		if (error_flag & VRSTARTUP_NOPROCS)
			vrErrPrintf(RED_TEXT "\r\t-> No valid processes.\n" NORM_TEXT);
		if (error_flag & VRSTARTUP_BADPROC)
			vrErrPrintf(RED_TEXT "\r\t-> Unknown process in configuration.\n" NORM_TEXT);
		if (error_flag & VRSTARTUP_BADWINDOW)
			vrErrPrintf(RED_TEXT "\r\t-> Unknown window in configuration.\n" NORM_TEXT);
		if (error_flag & VRSTARTUP_BADINDEV)
			vrErrPrintf(RED_TEXT "\r\t-> Unknown input device in configuration.\n" NORM_TEXT);
		if (error_flag & VRSTARTUP_BADINMAP)
			vrErrPrintf(RED_TEXT "\r\t-> Unable to fully parse input mapping.\n" NORM_TEXT);
		if (error_flag & VRSTARTUP_NOLOCKSEM)
			vrErrPrintf(RED_TEXT "\r\t-> Unable to create a semaphore for a lock.\n" NORM_TEXT);
		if (error_flag & VRSTARTUP_NOBARRSEM)
			vrErrPrintf(RED_TEXT "\r\t-> Unable to create a semaphore for a barrier.\n" NORM_TEXT);

		/* if there's a command to execute, do it (first check system then default values) */
		if (config->system->settings.exec_uponerror != NULL)
			vrShellCmd(config->system->settings.exec_uponerror, vrThisProc->name, vrThisProc->pid, error_flag);
		else if (config->defaults.exec_uponerror != NULL)
			vrShellCmd(config->defaults.exec_uponerror, vrThisProc->name, vrThisProc->pid, error_flag);

		/* if the exit-upon-error flag is set, then exit */
		if (config->system->settings.exit_uponerror == -1)
			exit_flag = config->defaults.exit_uponerror;
		else	exit_flag = config->system->settings.exit_uponerror;

		if (exit_flag & error_flag) {
			vrErrPrintf("vrConfigure(): " RED_TEXT "Exit-Upon-Error flag match (%d), so exitting.\n" NORM_TEXT, exit_flag & error_flag);
			vrExit();
			exit(exit_flag & error_flag);
		}
	}


	vrDbgPrintfN(ALWAYS_DBGLVL, "FreeVR: " BOLD_TEXT "**** Initializing the System! ****\n" NORM_TEXT);
	if (config->system->settings.exec_start != NULL)
		vrShellCmd(config->system->settings.exec_start, config->system->name, vrThisProc->pid, vrContext->status);
	else if (config->defaults.exec_start != NULL) {
		vrShellCmd(config->defaults.exec_start, config->system->name, vrThisProc->pid, vrContext->status);
	}


	/* TODO: lock the main process to a single CPU if requested   */
	/*   (of course, this is after vrConfigure because we don't   */
	/*   know the system call for locking the process before that.*/

	/*************************/
	/** Spawn the processes **/
	/*************************/
	vrDbgPrintfN(SPAWN_DBGLVL, "vrStart(): #### About to spawn FreeVR subprocesses\n");
	if (config->num_procs < 2) {
		vrErr("vrStart(): no visual rendering or input processes specified");
	} else {
		int	count;

#if 0 /* [8/7/2000] BS: testing to see if this sig-catcher is hindering debuging */ /* 09/01/2014 TODO: I should turn this back on to see if it works -- but not now while I'm testing some drastic new code. */
		signal(SIGCHLD, vrChildDeathSignalHandler);
#endif
		vrDbgPrintfN(SPAWN_DBGLVL, "vrStart(): About to start %d processes\n", config->num_procs);
		for (count = 1; count < config->num_procs; count++) {
			vrDbgPrintfN(SPAWN_DBGLVL, "vrStart(): About to start process # %d\n", count);
			vrProcessStart(context, config->procs[count]);
		}
	}
	config->procs_init = 1;
	vrDbgPrintfN(SPAWN_DBGLVL, "vrStart(): #### All FreeVR subprocesses begun.\n");

	/***************************************/
	/** Do some post-spawn initialization **/
	/***************************************/
	/* Even as we wait for all the input devices, the world is   */
	/*   already being rendered.  But with a null sensor for the */
	/*   users' heads.  Once the input devices are all open, we  */
	/*   can initialize the vrInput arrays, and the user/prop    */
	/*   structures.                                             */
	vrInputWaitForAllInputsToBeCreated(context);
	vrInputInitialize(context);
	vrUserInitialize(context);

	/* initialize the timing statistics for the main process */
	if (vrThisProc->stats_args) {
#ifndef MP_NONE
		vrThisProc->stats = vrProcessStatsCreate(vrThisProc->name, VR_TIME_SLEEP+1, vrThisProc->stats_args);
#else
		vrThisProc->stats = vrProcessStatsCreate(vrThisProc->name, VR_TIME_VP0+1+config->num_procs, vrThisProc->stats_args);
#endif

		/* if no y-location was assigned then change the default */
		if (strstr(vrThisProc->stats_args, "yloc") == NULL)
			vrThisProc->stats->yloc = 0.333;

		/* set the element labels */
		vrThisProc->stats->elem_labels[VR_TIME_SIM] = vrShmemStrDup("sim-1");
		vrThisProc->stats->elem_labels[VR_TIME_SIM2] = vrShmemStrDup("sim-2");
		vrThisProc->stats->elem_labels[VR_TIME_PAUSE] = vrShmemStrDup("paused");
		vrThisProc->stats->elem_labels[VR_TIME_SLEEP] = vrShmemStrDup("sleep");
#ifdef MP_NONE
		/* In the single-thread variant, the main process also reports each of the virtual processes */
		int	count;	/* loop over all the virtual processes */
		for (count = 1; count < config->num_procs; count++) {
			vrThisProc->stats->elem_labels[VR_TIME_VP0 + count] = vrShmemStrDup(config->procs[count]->name);
		}
#endif

		/* And make the two simulation stats colors to be somewhat alike, */
		/*   but distinguishable from the others.                         */
		vrThisProc->stats->elem_colors[VR_TIME_SIM2][0] = 0.80;
		vrThisProc->stats->elem_colors[VR_TIME_SIM2][1] = 0.20;
		vrThisProc->stats->elem_colors[VR_TIME_SIM2][2] = 0.20;
	}

	/***************************************************************/
	/** Print post-startup configuration information if requested **/
	/***************************************************************/
	vrFprintContext(stdout, context, (config->system->settings.post_context_print == def) ? config->defaults.post_context_print : config->system->settings.post_context_print);
	vrFprintConfig(stdout, config, (config->system->settings.post_config_print == def) ? config->defaults.post_config_print : config->system->settings.post_config_print);
	vrFprintInput(stdout, context->input, (config->system->settings.post_input_print == def) ? config->defaults.post_input_print : config->system->settings.post_input_print);

	/*******************************************************/
	/** Indicate that the configuration stage is complete **/
	/*******************************************************/
	context->status = VRSTATUS_RUNNING;
	config->system_init = 1;
	vrMsgPrintf("FreeVR: " BOLD_TEXT "**** FreeVR library startup complete. ****\n" NORM_TEXT);
	vrTrace("vrStart", "Startup complete.");
}


/******************************************************************/
int vrSystemInitialized()
{
	return vrContext->config->system_init;
}


/******************************************************************/
int vrProcessesInitialized()
{
	return vrContext->config->procs_init;
}


/******************************************************************/
/* vrExit(): The function to close everything down -- hopefully   */
/*   cleanly.  This is done by setting a global flag in vrContext */
/*   that lets each process know that it should self-terminate.   */
/******************************************************************/
void vrExit()
{
static	char		exec_stop[256];
static	char		exec_stop_name[256];
	vrConfigInfo	*config;
	int		pid;
	int		status;
	int		count;

	/***************************************************/
	/* Make sure all the primary system pointers exist */
	/*  (ie. we don't want to shut down a system that isn't running */
	if (vrShmemArena() == NULL && USE_SHMEM) {
		vrNormPrintf(RED_TEXT "ERROR: vrExit() called, but there isn't any shared memory. (perhaps called twice)\n" NORM_TEXT);
		exit(-1);
	}

	if (vrContext == NULL) {
		vrNormPrintf(RED_TEXT "ERROR: vrExit() called, but vrContext is NULL.  (perhaps called twice)\n" NORM_TEXT);
		exit(-1);
	}

	if (vrContext->status != VRSTATUS_RUNNING) {
		vrDbgPrintfN(ALWAYS_DBGLVL, "WARNING: vrExit() called, but system status is %d (ie. not running)\n", vrContext->status);
	}

	config = vrContext->config;
	if (config == NULL) {
		vrNormPrintf(RED_TEXT "ERROR: vrExit() called, but vrConfigInfo is NULL.  (this shouldn't happen)\n" NORM_TEXT);
		exit(-1);
	}

#if 0
	vrNormPrintf(RED_TEXT "vrExit called from process %d  shmem_arena = %#p, setting dl to 200\n", getpid(), vrShmemArena());
	vrContext->config->debug_level = 200;
#endif

	/***********************/
	/* determine exec_stop */
	/* TODO: I guess this is here because we'll be losing our shared memory soon */
	/*   (it would be preferable if this were closer to the call to vrShellCmd().*/
	if (config->system->settings.exec_stop != NULL) {
		strcpy(exec_stop, config->system->settings.exec_stop);
		strcpy(exec_stop_name, config->system->name);
		pid = vrThisProc->pid;
		status = vrContext->status;
	} else if (config->defaults.exec_stop != NULL) {
		strcpy(exec_stop, config->defaults.exec_stop);
		strcpy(exec_stop_name, config->system->name);
		pid = vrThisProc->pid;
		status = vrContext->status;
	} else {
		exec_stop[0] = '\0';
	}

	vrDbgPrintfN(ALWAYS_DBGLVL,
		"FreeVR: " BOLD_TEXT "FreeVR library terminating from a call to vrExit().\n" NORM_TEXT);
#if 0 /* set to 1 for debugging the hang on exit */
vrThisProc->debug_level = 200;	/* TODO: delete this */
#endif
	vrContext->status = VRSTATUS_TERMINATING;

	/* since we're about to kill the children, ignore their cries. */
	signal(SIGCHLD, SIG_IGN);

	vrSleep(100000);  /* 1/10 second */

	/******************************************/
	/* Release all the barriers in the system */
	vrBarrierReleaseAll(vrContext);

#ifndef MP_NONE /* single-threaded variant of FreeVR doesn't need to close each process */
	/***********************************************/
	/* tell all processes to terminate themselves. */
	/*   First end the non-input, non-main, non-telnet processes. */
	/*   The main and telnet processes have to wait because they  */
	/*   are probably from where the call to vrExit() came, and   */
	/*   so we shouldn't end them while we're here (though really,*/
	/*   that shouldn't happen, because we're not killing them,   */
	/*   just telling them to kill themselves.  Note that we don't*/
	/*   have to check for whether a process is the MAIN one,     */
	/*   because the main process is always number 0, so we just  */
	/*   start counting at 1.  The reason we need to wait on the  */
	/*   inputs is that if they go away before the others, there  */
	/*   will be problems when the others try to access no longer */
	/*   existent inputs.                                         */
	/* [11/8/01: we may have a circular logic problem.  As stated */
	/*   above, inputs shouldn't be killed before displays because*/
	/*   display processes will refer to input values.  However,  */
	/*   input processes that rely on an Xwindow, which of course */
	/*   is associated with a display process, need to stop       */
	/*   themself (or at least stop referencing the Xwindows GLX  */
	/*   stuff) before the display process stops.  The latter is  */
	/*   probably the best solution.                              */
	/* TODO: make this work properly when called from the telnet process. */
	vrDbgPrintfN(RARE_DBGLVL, "vrExit(): about to close non-input, non-telnet processes\n");
	for (count = 1; count < config->num_procs; count++) {
		if (config->procs[count] == NULL)
			continue;			/* skip null processes */

		if (config->procs[count]->pid < 1)
			continue;			/* skip unspawned processes */

		switch (config->procs[count]->type) {
#ifdef GFX_PERFORMER /* TODO: 12/10/2002 -- this may no longer be necessary now that the check for a valid pid as been added */
		case VRPROC_VISREN:
#endif
		case VRPROC_INPUT:
		case VRPROC_TELNET:
			vrDbgPrintfN(RARE_DBGLVL, "\tvrExit(): holding off on ending process %d\n", count);
			break;


		default:
			if (config->procs[count]->proc_done) {
				vrDbgPrintfN(RARE_DBGLVL, "\tvrExit(): Hmm, proc_done for process %d is already 1\n", count);
			}
			vrDbgPrintfN(RARE_DBGLVL, "\tvrExit(): about to set end_proc for process %d to 1\n", count);
			config->procs[count]->end_proc = 1;
			break;
		}
	}
	/***************************************/
	/* now wait until those procs are done */
	for (count = 1; count < config->num_procs; count++) {
		if (config->procs[count] == NULL)
			continue;			/* skip null processes */

		if (config->procs[count]->pid < 1)
			continue;			/* skip unspawned processes */

		switch (config->procs[count]->type) {
#ifdef GFX_PERFORMER /* TODO: 12/10/2002 -- this may no longer be necessary now that the check for a valid pid as been added */
		case VRPROC_VISREN:
#endif
		case VRPROC_NOCONFIG:			/* this isn't really necessary because of the pid check */
		case VRPROC_INPUT:
		case VRPROC_TELNET:
			break;

		default:
			vrDbgPrintfN(RARE_DBGLVL, "\tvrExit(): about to wait for proc_done of process %d\n", count);

			/* TODO: this while loop should only go a second or two, then something */
			/*   should be done to actively kill the slow-to-die process.           */
#if 1 /* 6/5/03: I think this will help prevent this loop from waiting for processes that never got started, and aren't looping */
			while (!config->procs[count]->proc_done && !config->procs[count]->initialized);
#else
			while (!config->procs[count]->proc_done);
#endif
			vrDbgPrintfN(RARE_DBGLVL, "\tvrExit(): process %d now reporting \"proc_done == 1\"\n", count);
		}
	}

	/*********************************/
	/* now close all the other procs ** redundantly closing the non-inputs doesn't hurt */
	vrDbgPrintfN(RARE_DBGLVL, "vrExit(): about to close all processes\n");
	for (count = 1; count < config->num_procs; count++) {
		if (config->procs[count] == NULL)
			continue;			/* skip null processes */

		if (config->procs[count]->pid < 1)
			continue;			/* skip unspawned processes */

		switch (config->procs[count]->type) {
#ifdef GFX_PERFORMER /* TODO: 12/10/2002 -- this may no longer be necessary now that the check for a valid pid as been added */
		case VRPROC_VISREN:
			/* TODO: call the Performer window close routine */
			break;
#endif

		default:
			if (config->procs[count]->proc_done) {
				vrDbgPrintfN(RARE_DBGLVL, "\tvrExit(): Hmm, proc_done for process %d is already 1\n", count);
			}
			vrDbgPrintfN(RARE_DBGLVL, "\tvrExit(): about to set end_proc for process %d to 1\n", count);
			config->procs[count]->end_proc = 1;
		}
	}

	/****************************************************/
	/* now wait for all the non-main procs to be closed */
	/* TODO: we'll get stuck here if vrExit() is called from a non-main process */
	for (count = 1; count < config->num_procs; count++) {
		if (config->procs[count] == NULL)
			continue;			/* skip null processes */

		if (config->procs[count]->pid < 1)
			continue;			/* skip unspawned processes */

		switch (config->procs[count]->type) {
#ifdef GFX_PERFORMER /* TODO: 12/10/2002 -- this may no longer be necessary now that the check for a valid pid as been added */
		case VRPROC_VISREN:
			break;
#endif
		case VRPROC_NOCONFIG:			/* this isn't really necessary because of the pid check */
			break;

		default:
			vrDbgPrintfN(RARE_DBGLVL, "\tabout to wait for proc_done of process %d\n", count);

			/* TODO: this while loop should only go a second or two, then something */
			/*   should be done to actively kill the slow-to-die process.           */
#if 0 /* 6/5/03: I thought the initialized flag would be helpful in preventing a hang-up but it causes problems with locks */
			while (!config->procs[count]->proc_done && !config->procs[count]->initialized);
#else
			while (!config->procs[count]->proc_done);
#endif
			vrDbgPrintfN(RARE_DBGLVL, "\tprocess %d now reporting \"proc_done == 1\"\n", count);
		}
	}

	vrDbgPrintfN(RARE_DBGLVL, "vrExit(): all processes reported as terminated.\n");

#if 0
	vrSleep(500000);  /* 1/2 second */  /* TODO: we really should wait for info that all the other processes have died. */
#endif
#else
	/* Here is where we close shop for the single-threaded variant of FreeVR */

	/******************************************************/
	/* now terminate the elements of each virtual process */
	vrDbgPrintfN(RARE_DBGLVL, "vrExit(): about to close all virtual processes\n");
	for (count = 1; count < config->num_procs; count++) {
		if (config->procs[count] == NULL)
			continue;			/* skip null processes */

		if (config->procs[count]->pid < 1)
			continue;			/* skip unspawned processes */

		switch (config->procs[count]->type) {
		case VRPROC_INPUT:
			vrInputTermProc(config->procs[count]);
			break;

		case VRPROC_TELNET:
			vrTelnetTermProc(config->procs[count]);
			break;

		case VRPROC_VISREN:
			vrVisrenTermProc(config->procs[count]);
			break;

		default:
			break;
		}
	}

#endif

	/****************************************************/
	/* indicate that the main FreeVR process is no more */
	/* NOTE: we're going to terminate shared memory next, so these are of limited value. */
	config->procs[0]->end_proc = 1;
	vrContext->status = VRSTATUS_TERMINATED;

	/* run any VR-closing system commands */
	/* NOTE: ideally this should be after the call to vrShmemExit(), but the command */
	/*   doesn't seem to be executed, despite some debugging print statements in     */
	/*   vrShellCmd() that show everything to be okay, and there are no shared memory*/
	/*   dependencies in vrShellCmd() either.  [I only tested this on IRIX.]         */
	/* TODO: figure out the above problem and move this to the end of vrExit().      */
	if (exec_stop[0] != '\0') {
		vrShellCmd(exec_stop, exec_stop_name, pid, status);
	}

	/* NOTE: can't read-lock vrContext->head_lock, because a write-lock */
	/*   will be done on it inside vrLockFreeAll() -- we could store    */
	/*   the value in a temporary variable, but I'm not sure it's worth */
	/*   it, since we'll be waiting at the write-lock in a few machine  */
	/*   instructions anyway.                                           */
	vrLockFreeAll(vrContext->head_lock);

	vrDbgPrintfN(RARE_DBGLVL, "vrExit(): about to close down Shared Memory.\n");
	vrShmemExit();

#ifdef SEM_TCP
	vrTcpShutdown();
#endif
}


/******************************************************************/
/* This function is (currently) optional, and is called once each frame */
/*   by the simulation process of an application.  It can be used to    */
/*   pause the simulation as well as do some timings.                   */
/* NOTE: the return value indicates whether to keep on simulating */
/* NOTE: this function is NOT OPTIONAL for the single-thread FreeVR variant */
int vrFrame()
{
static	vrTime		loop_wtime = -1;			/* time of the beginning of each loop */
	int		return_value;
	vrProcessInfo	*this_proc = vrThisProc;
#ifdef MP_NONE
	int		count;
#endif

	vrTrace("vrFrame", "top");

	if (loop_wtime < 0)
		loop_wtime = vrCurrentWallTime();

	/* measure: time spent in simulation */
	vrProcessStatsMark(this_proc->stats, VR_TIME_SIM, 1);

#ifdef MP_NONE
	/* NOTE: when multiprocessing is disabled, all the other processes */
	/*   become virtual processes and are executed here in vrFrame().  */
	/*   This only happens in the "single-thread" variant of FreeVR -- */
	/*   ie. this is a compile-time only option.                       */

	/* TODO: this is probably where we need to do the do the input data  */
	/*   "freeze" operation -- either locally, or by calling a (probably */
	/*   modified) version of vrProcessSync().                           */
	vrInputFreezeVisren(vrContext);
	vrUserTravelFreezeVisren(vrContext);
	vrPropFreezeVisren(vrContext);

	/***************************************/
	/** Do all virtual-process operations **/
	/* TODO: ideally we'll get rid of the global vrContext reference (perhaps when I implement handles) */
	for (count = 1; count < vrContext->config->num_procs; count++) {
		// Sample code: if (vrContext->config->procs[count]->proc_done) { ... }
		switch (vrContext->config->procs[count]->type) {
		case VRPROC_MAIN:
			vrErrPrintf("vrFrame(): There shouldn't be a MAIN virtual process\n");
			break;

		case VRPROC_INPUT:
			vrInputOneFrame(vrContext->config->procs[count]);
			vrProcessStatsMark(this_proc->stats, VR_TIME_VP0 + count, 0);
			break;

		case VRPROC_TELNET:
			/* For now this is a no-op because the telnet process would block once-connected */
			vrProcessStatsMark(this_proc->stats, VR_TIME_VP0 + count, 0);
			break;

		case VRPROC_VISREN:
			vrVisrenOneFrame(vrContext->config->procs[count]);
			vrProcessStatsMark(this_proc->stats, VR_TIME_VP0 + count, 0);
			break;
		}
	}
	this_proc->stats->elements = VR_TIME_VP0 + vrContext->config->num_procs;
#endif

	/*************************************/
	/** Do vrFrame() process operations **/

	/* calculate frame rates and set the process and renderinfo time values */
	this_proc->frame_count++;
	vrProcessCalcFrameRate(this_proc);

	/* handle system pausing */
	if (vrContext->paused) {
		vrContext->pause_wtime = vrCurrentWallTime();
		while (vrContext->paused && !vrGet2switchValue(0)) {
			vrSleep(1000);		/* sleep 1ms (or more) */
		}

		/* add time spent paused to vrContext */
		vrContext->paused_time += vrCurrentWallTime() - vrContext->pause_wtime;

		vrContext->pause_wtime = -1.0;
	}
	/* measure: time spent paused */
	vrProcessStatsMark(this_proc->stats, VR_TIME_PAUSE, 0);


	/*******************************************/
	/** Now synchronize with given frame time **/
	vrSleep(this_proc->usec_min - (long)((vrCurrentWallTime() - loop_wtime) * 1000000.0));

	/* measure: time spent in sleep */
	vrProcessStatsMark(this_proc->stats, VR_TIME_SLEEP, 0);

	/* start measuring for minimal loop time */
	loop_wtime = vrCurrentWallTime();

	/* set measurement for next frame */
	vrProcessStatsNextFrame(this_proc->stats);
	this_proc->frame_wtime = vrCurrentWallTime();

	vrTrace("vrFrame", "calculating return_value");
	return_value = !vrGet2switchValue(0);

	vrTrace("vrFrame", "bottom");
	return (return_value);
}

