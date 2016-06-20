/* ======================================================================
 *
 *  CCCCC          vr_visren.c
 * CC   CC         Author(s): Ed Peters, Bill Sherman, John Stone
 * CC              Created: June 4, 1998
 * CC   CC         Last Modified: September 1, 2014
 *  CCCCC
 *
 * Code file for FreeVR generic rendering handles.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/*************************************************************************

FreeVR USAGE:
	Here are the FreeVR configuration options for visual rendering windows:
		"GraphicsType" =  { "glx" }

		"mount" = { "fixed", "head", "hand", "simulator" }
		"visrenMode" =  { "default", "mono", "right", "left", "dualvp", "dualfb", "anaglyphic", "checkerboard", "vibrate" }
		"eyelist" = <name>
			- name is the string name of an eyelist object
	  :-(	"viewMask" =
	  :-(	"colorMask" =

		"viewPort" = x-origin, width, y-origin, height		[in pixels]
		"viewPort:left" = x-origin, width, y-origin, height	[in pixels]
		"viewPort:right" = x-origin, width, y-origin, height	[in pixels]
		"viewPortf" = x-origin, width, y-origin, height		[0.0 to 1.0 scale]
		"viewPortf:left" = x-origin, width, y-origin, height	[0.0 to 1.0 scale]
		"viewPortf:right" = x-origin, width, y-origin, height	[0.0 to 1.0 scale]

		"rw2w_coords" = ll-x,ll-y,ll-z, lr-x,lr-y,lr-z, ul-x,ul-y,ul-z
		"rw2w_translate" = x, y, z
		"rw2w_rotate" = x, y, z, w
		"rw2w_transform" = 00, 01, 02, 03, 10, 11, 12, 13, 20, 21, 22, 23, 30, 31, 32, 33
		"e2w_coords" =  ll-x,ll-y,ll-z, lr-x,lr-y,lr-z, ul-x,ul-y,ul-z

		"simMask" = { 0 | 1 }
		"showStats" = { 0 | 1 }
		"statsProcs" = <string>
			- string is a comma-separated list of processes for which to show the stats
		"showFps" = { 0 | 1 }
		"placeFps" = x, y   					[0.0 to 1.0 scale]
		"colorFps" = red, green, blue				[0 to 255 scale]
		"showFrame" = { 0 | 1 }

		"args" = <string>
			- string is a semicolon-separated list of renderer-specific arguments

	  :-(	"execAtStart" =
	  :-(	"execAtStop" =

	Controls are specified in the freevrrc file:
	  :-(	control "<control option>" = "switch2(button[{1|2|3|4|5|6|7|8|Star}])";

TODO:
	- fill in the things to do list

	- complete the above Usage chart

**************************************************************************/

#undef	COREDUMP_IS_GOOD
#undef	VERBOSE		/* define this for even more debugging info */
#define	NEW_BS_METHOD	/* define this to use new barrier syncing method */

/* TODO: Determine if this is really worth it. */
#ifdef WIN_GLX
#include <GL/gl.h>		/* for the definition of GL_NONE */
#endif

#include "lance_debug.h"

#include "vr_config.h"		/* needed for getting default values (eg. near_clip) */
#include "vr_visren.h"
#include "vr_entity.h"
#include "vr_objects.h"
#include "vr_input.h"
#include "vr_callback.h"
#include "vr_utils.h"
#include "vr_debug.h"
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <math.h>		/* for acos in vrRenderGetBillboardAngles...() */

#include "vr_visren.opts.h"



#if 0 /* 6/6/03: this isn't necessary with new way of getting the current rendering information */
/* static (file-scope) globals for the (TODO: each?) visren process */
static vrProcessInfo	*proc_info = NULL;	/* TODO: is this nece since we also have vrThisProc? */
#endif



/**********************************************************************/
/** A structure of private information for a particular *PROCESS*.   **/
/**   Note that each window also has private information based on    **/
/**   the type of window, but that is pointed to in the vrWindowInfo **/
/**   structure, not this one (which is pointed to by a vrProcInfo   **/
/**   structure.                                                     **/
typedef struct {
		int		num_windows;	/* TODO: this is redundant with proc_info->num_things */
		vrWindowInfo	**windows;	/* TODO: this is redundant with proc_info->things */
		vrWindowInfo	*curr_window;
		vrUserInfo	*curr_user;
		vrEyeInfo	*curr_eye;
		vrRenderInfo	*renderinfo;	/* Used for maintaining the RenderInfo data on a per-process basis */
	} _VisrenPrivate;


/*****************************************************************/
void vrFprintRenderInfo(FILE *file, vrRenderInfo *rendinfo, vrPrintStyle style)
{
	if (rendinfo == NULL) {
		vrFprintf(file, "vrRenderInfo = (null)\n");
		return;
	}

	switch (style) {
	case file_format:
		/* TODO: not yet implemented -- output that can be reread. */
	case one_line:
		/* TODO: not yet implemented -- very brief output */
	case machine:
		/* TODO: not yet implemented -- output easily machine parsed */
		vrFprintf(file, "TODO: put special version of render-info here\n");
		break;

	case brief:

	default:
	case verbose:

		vrFprintf(file, "vrRenderInfo = [context:%#p gfx:%#p persp:%#p window:%#p eye:%#p f-count = %d stime = %.3f]\n",
			rendinfo->context,
			rendinfo->gfx_context,
			rendinfo->persp,
			rendinfo->window,
			rendinfo->eye,
			rendinfo->frame_count,
			rendinfo->frame_stime);
		break;
	}
}


/*****************************************************************/
void vrFprintPerspData(FILE *file, vrPerspData *pd, vrPrintStyle style)
{
	if (pd == NULL) {
		vrFprintf(file, "vrPerspData = (null)\n");
		return;
	}

	switch (style) {
	case one_line:
	case brief:

	default:
	case verbose:

#ifdef USE_FRUSTUMEYE
		vrFprintf(file, "vrPerspData = [frust (lrbtnf): %.2f %.2f %.2f %.2f %.2f %.2f; ",
			pd->frustum.v[0], pd->frustum.v[1], pd->frustum.v[2], pd->frustum.v[3], pd->frustum.v[4], pd->frustum.v[5]);
#else
		vrFprintMatrix(file, "persp mat", &(pd->mat));
#endif
		vrFprintf(file, "eye: %.2f %.2f %.2f;]\n",
			pd->eye[0], pd->eye[1], pd->eye[2]);

		/* TODO: add the mount-type and rw2w_xform information here */
		break;
	}
}


/*****************************************************************/
char *vrWindowName(vrWindowMountType type)
{
	switch (type) {
	case VRWINDOW_FIXED:
		return "fixed";
	case VRWINDOW_HEADMOUNT:
		return "headmount";
	case VRWINDOW_HANDHELD:
		return "handheld";
	case VRWINDOW_SIMULATOR:
		return "simulator";
	}

	return "unknown";
}


/*****************************************************************/
vrWindowMountType vrWindowType(char *name)
{
	/*** fixed displays ***/
	if (!strcasecmp(name, "fixed"))		return VRWINDOW_FIXED;

	/*** head-based displays ***/
	else if (!strncasecmp(name, "head", 4))	return VRWINDOW_HEADMOUNT;
	else if (!strcasecmp(name, "hmd"))	return VRWINDOW_HEADMOUNT;

	/*** hand-based displays ***/
	else if (!strncasecmp(name, "hand", 4))	return VRWINDOW_HANDHELD;

	/*** simulator displays ***/
	else if (!strncasecmp(name, "sim", 3))	return VRWINDOW_SIMULATOR;
	else if (!strcasecmp(name, "default"))	return VRWINDOW_SIMULATOR;
	else {
		vrErrPrintf(RED_TEXT "Unknown window type '%s' using 'simulator' type\n" NORM_TEXT, name);
		return VRWINDOW_SIMULATOR;
	}
}


/*****************************************************************/
char *vrVisrenModeName(vrVisrenModeType mode)
{
	switch (mode) {
	case VRVISREN_DEFAULT:			return "default";
	case VRVISREN_MONO:			return "mono";
	case VRVISREN_LEFT:			return "left";
	case VRVISREN_RIGHT:			return "right";
#if 0 /* deprecated!  -- removed even */
	case VRVISREN_STEREO:			return "stereo";
#endif
	case VRVISREN_DUALFB:			return "dualfb";
	case VRVISREN_DUALVP:			return "dualvp";
	case VRVISREN_ANAGLYPHIC:		return "anaglyphic";
	case VRVISREN_CHECKERBOARD:		return "checkerboard";
	case VRVISREN_VIBRATE:			return "vibrate";

	default:				return "unknown";
	}
}


/*****************************************************************/
#if 0
/* this is more correct, but generates compiler errors for vr_config.c */
vrVisrenModeType vrVisrenModeValue(char *modename)
#else
int vrVisrenModeValue(char *modename)
#endif
{
	if (!strcasecmp(modename, "default"))		return VRVISREN_DEFAULT;
	else if (!strcasecmp(modename, "mono"))		return VRVISREN_MONO;
	else if (!strcasecmp(modename, "left"))		return VRVISREN_LEFT;
	else if (!strcasecmp(modename, "mono-left"))	return VRVISREN_LEFT;
	else if (!strcasecmp(modename, "right"))	return VRVISREN_RIGHT;
	else if (!strcasecmp(modename, "mono-right"))	return VRVISREN_RIGHT;
#if 0 /* deprecated!  -- removed even */
	else if (!strcasecmp(modename, "stereo"))	return VRVISREN_STEREO;
#endif
	else if (!strcasecmp(modename, "dualfb"))	return VRVISREN_DUALFB;
	else if (!strcasecmp(modename, "dualvp"))	return VRVISREN_DUALVP;
	else if (!strcasecmp(modename, "dualsplit"))	return VRVISREN_DUALVP;
	else if (!strcasecmp(modename, "anaglyphic"))	return VRVISREN_ANAGLYPHIC;
	else if (!strcasecmp(modename, "checkerboard"))	return VRVISREN_CHECKERBOARD;
	else if (!strcasecmp(modename, "vibrate"))	return VRVISREN_VIBRATE;

	/* default */
	/* TODO: probably should print a warning message here */
	return VRVISREN_DEFAULT;
}


/*****************************************************************/
void vrWindowClear(vrWindowInfo *object)
{
	object->mount = VRWINDOW_SIMULATOR;
	object->graphics = NULL;
	object->gfx_context = NULL;
	object->args = vrShmemStrDup("");
	object->dualeye_buffer = 0;
	object->eyelist = NULL;
	object->num_eyes = 0;
	object->eyes = NULL;
	object->simulator_mask = 1;
#ifdef WIN_GLX
	object->frontrendermode = (int)GL_NONE;		/* ie. don't alter the rendermode */
	object->backrendermode = (int)GL_NONE;		/* ie. don't alter the rendermode */
#else
	object->frontrendermode = 0;			/* ie. don't alter the rendermode */
	object->backrendermode = 0;			/* ie. don't alter the rendermode */
#endif
	object->call_visreninit = 0;
	object->sim_follow_head = 0;
	object->world_show = 1;
	object->ui_show = 0;
	object->ui_loc[0] = 0.15;
	object->ui_loc[1] = 0.9;
	object->ui_color[0] = 1.0;
	object->ui_color[1] = 1.0;
	object->ui_color[2] = 1.0;
	object->fps_show = 0;
	object->fps_loc[0] = 0.1;
	object->fps_loc[1] = 0.1;
	object->fps_color[0] = 1.0;
	object->fps_color[1] = 1.0;
	object->fps_color[2] = 1.0;
	object->stats_show = 0;
	object->inputs_show = 0;
	object->show_in_simulator = 0;
	object->geometry.origX = -1;
	object->geometry.origY = -1;
	object->geometry.width = -1;
	object->geometry.height = -1;
	object->viewport_left.origX  = -1;
	object->viewport_left.origY  = -1;
	object->viewport_left.width  = -1;
	object->viewport_left.height = -1;
	object->viewport_right.origX  = -1;
	object->viewport_right.origY  = -1;
	object->viewport_right.width  = -1;
	object->viewport_right.height = -1;
	object->viewportF_left.min_X = 0.0;
	object->viewportF_left.max_X = 1.0;
	object->viewportF_left.min_Y = 0.0;
	object->viewportF_left.max_Y = 1.0;
	object->viewportF_right.min_X = 0.0;
	object->viewportF_right.max_X = 1.0;
	object->viewportF_right.min_Y = 0.0;
	object->viewportF_right.max_Y = 1.0;
	if (object->rw2w_xform == NULL)
		object->rw2w_xform = vrMatrixCreateIdentity();
	else	vrMatrixSetIdentity(object->rw2w_xform);
	if (object->rw2w_homexform == NULL)
		object->rw2w_homexform = vrMatrixCreateIdentity();
	else	vrMatrixSetIdentity(object->rw2w_homexform);

	vrSettingsClear(&object->settings);
}


/*****************************************************************/
void vrWindowCopy(vrWindowInfo *dest_object, vrWindowInfo *src_object)
{
	void	*dest_mem;
	void	*src_mem;
	int	memlen;
	int	count;

	/* copy only the memory after the generic vrObjectInfo stuff */
	dest_mem = (char *)dest_object + sizeof(vrObjectInfo);
	src_mem = (char *)src_object + sizeof(vrObjectInfo);
	memlen = sizeof(vrWindowInfo) - sizeof(vrObjectInfo);
	memcpy(dest_mem, src_mem, memlen);

	/* make independent copy of some fields */
	dest_object->eyes = (vrEyeInfo **)vrShmemAlloc0(src_object->num_eyes * sizeof(vrEyeInfo *));
	for (count = 0; count < src_object->num_eyes; count++) {
		dest_object->eyes[count] = vrShmemMemDup(src_object->eyes[count], sizeof(vrEyeInfo *));
	}
	dest_object->rw2w_xform = vrShmemMemDup(src_object->rw2w_xform, sizeof(vrMatrix));
	dest_object->rw2w_homexform = vrShmemMemDup(src_object->rw2w_homexform, sizeof(vrMatrix));
}


/*****************************************************************/
void vrFprintWindowInfo(FILE *file, vrWindowInfo *windowinfo, vrPrintStyle style)
{
	int	num;

	/* if null window given, print an empty shell and return */
	if (windowinfo == NULL) {
		vrFprintf(file, "window \"(nil)\" = { }\n");
		return;
	}

	switch (style) {
	case one_line:
	case brief:
		vrFprintf(file, "id = %d, name = '%s', mount = '%s', render mode = '%s', eyelist = '%s'\n",
			windowinfo->id,
			windowinfo->name,
			vrWindowName(windowinfo->mount),
			((windowinfo->settings.visrenmode != VRVISREN_DEFAULT) ?
				vrVisrenModeName(windowinfo->settings.visrenmode) :
				vrVisrenModeName(windowinfo->visrenmode)),
			(windowinfo->settings.eyelist_name == NULL ? "(none)" : windowinfo->settings.eyelist_name));

		break;

	case machine:
		/* TODO: not yet implemented -- output easily machine parsed */
		vrFprintf(file, "TODO: put special version of window info here\n");
		break;

	default:
	case verbose:
		vrFprintf(file, "{\n");
		vrFprintf(file, "\r"
			"\tObject_type = %s (%d)\n\tid = %d\n\tname = \"%s\"\n"
			"\tmalleable = %d\n\tnext = %#p\n"
			"\tCreated at %s, line %d\n"
			"\tLast modified at %s, line %d\n",
			vrObjectTypeName(windowinfo->object_type),
			windowinfo->object_type,
			windowinfo->id,
			windowinfo->name,
			windowinfo->malleable,
			windowinfo->next,
			windowinfo->file_created,
			windowinfo->line_created,
			windowinfo->file_lastmod,
			windowinfo->line_lastmod);
		vrFprintf(file,
			"\r\tproc = %#p (pid %d)\n"
			"\tnum in system = %d\n"
			"\tgraphicsType = '%s'\n\tgfx_context = %#p\n"
			"\tversion = '%s'\n\targs = \"%s\"\n"
			"\tmountType = %d (%s)\n\tsimulator_mask = 0x%x\n"
			"\tfront & back rendermodes = (%x, %x)\n"
			"\tsim_follow_head = %d\n"
			"\tworld_show = %d\n"
			"\tfps_show = %d\n\tfps_loc[2] = [%.2f %.2f]\n"
			"\tfps_color = [%.3f %.3f %.3f]\n"
			"\tframe_show = %d\n"
			"\tstats_show = %d\n\tstats_procs = '%s'\n"
			"\tstats = [",
			windowinfo->proc,
			(windowinfo->proc == NULL ? -1 : windowinfo->proc->pid),
			windowinfo->num,
			windowinfo->graphics,
			windowinfo->gfx_context,
			windowinfo->version,
			windowinfo->args,
			windowinfo->mount,
			vrWindowName(windowinfo->mount),
			windowinfo->simulator_mask,
			windowinfo->frontrendermode,
			windowinfo->backrendermode,
			windowinfo->sim_follow_head,
			windowinfo->world_show,
			windowinfo->fps_show,
			windowinfo->fps_loc[0],
			windowinfo->fps_loc[1],
			windowinfo->fps_color[0],
			windowinfo->fps_color[1],
			windowinfo->fps_color[2],
			windowinfo->show_in_simulator,
			windowinfo->stats_show,
			windowinfo->stats_procs);
		for (num = 0; num < VR_MAXSTATS; num++)
			if (windowinfo->stats[num] != NULL) {
				vrFprintf(file, " <%p>", windowinfo->stats[num]);
			} else {
				vrFprintf(file, " <null>");
			}
		vrFprintf(file, " ]\n");

		vrFprintf(file,
			"\r\tvisrenmode requested = \"%s\" (%d)\n\tvisrenmode selected = \"%s\" (%d)\n"
			"\teyelist requested = \"%s\"\n\teyelist selected = %#p (\"%s\")\n"
			"\teyes = (%d) [",
			vrVisrenModeName(windowinfo->settings.visrenmode),
			windowinfo->settings.visrenmode,
			vrVisrenModeName(windowinfo->visrenmode),
			windowinfo->visrenmode,
			(windowinfo->settings.eyelist_name == NULL ? "(none)" : windowinfo->settings.eyelist_name),
			windowinfo->eyelist,
			(windowinfo->eyelist == NULL ? " -- " : windowinfo->eyelist->name),
			windowinfo->num_eyes
			);
		for (num = 0; num < windowinfo->num_eyes; num++)
			vrFprintf(file, " \"%s\"", windowinfo->eyes[num]->name);
		vrFprintf(file, " ]\n"
			"\tnear_clip = %.2f\n\tfar_clip = %.2f\n"
			"\trw2w: (%3.1lf %3.1lf %3.1lf %3.1lf/%3.1lf %3.1lf %3.1lf %3.1lf/%3.1lf %3.1lf %3.1lf %3.1lf/%3.1lf %3.1lf %3.1lf %3.1lf)\n",
			windowinfo->settings.near_clip,
			windowinfo->settings.far_clip,
			windowinfo->rw2w_xform[ 0],
			windowinfo->rw2w_xform[ 4],
			windowinfo->rw2w_xform[ 8],
			windowinfo->rw2w_xform[12],
			windowinfo->rw2w_xform[ 1],
			windowinfo->rw2w_xform[ 5],
			windowinfo->rw2w_xform[ 9],
			windowinfo->rw2w_xform[13],
			windowinfo->rw2w_xform[ 2],
			windowinfo->rw2w_xform[ 6],
			windowinfo->rw2w_xform[10],
			windowinfo->rw2w_xform[14],
			windowinfo->rw2w_xform[ 3],
			windowinfo->rw2w_xform[ 7],
			windowinfo->rw2w_xform[11],
			windowinfo->rw2w_xform[15]);
		vrFprintf(file, "\r\tcoords: LL = (%3.1lf %3.1lf %3.1lf), LR = (%3.1lf %3.1lf %3.1lf), UL = (%3.1lf %3.1lf %3.1lf)\n"
			"\tgeometry = { %d %d %d %d }\n"
			"\tviewport:left = { %d %d %d %d }\n"
			"\tviewport:right = { %d %d %d %d }\n"
			"\tviewportF:left { %.2f %.2f %.2f %.2f }\n"
			"\tviewportF:right { %.2f %.2f %.2f %.2f }\n"
			"\tviewport_mask = %#p\n"
			"\tcall_visreninit = %d\n"
			"\tVisrenInit = %#p (%s)\n\tVisrenFrame = %#p (%s)\n"
			"\tVisrenWorld = %#p (%s)\n\tVisrenSim = %#p (%s)\n"
			"\tVisrenExit = %#p (%s)\n"
			"\tOpen = %#p (%s)\n\tRender = %#p (%s)\n\tRender Text = %#p (%s)\n"
			"\tRender NullWorld = %#p (%s)\n\tRender Default Simulator = %#p (%s)\n"
			"\tRender Transform = %#p (%s)\n\tSwap = %#p (%s)\n\tClose = %#p (%s)\n"
			"\tPrintAux = %#p (%s)\n"
			"\taux_data = %#p\n}\n",

			windowinfo->coords_ll[0],
			windowinfo->coords_ll[1],
			windowinfo->coords_ll[2],
			windowinfo->coords_lr[0],
			windowinfo->coords_lr[1],
			windowinfo->coords_lr[2],
			windowinfo->coords_ul[0],
			windowinfo->coords_ul[1],
			windowinfo->coords_ul[2],

			windowinfo->geometry.origX,
			windowinfo->geometry.origY,
			windowinfo->geometry.width,
			windowinfo->geometry.height,
			windowinfo->viewport_left.origX,
			windowinfo->viewport_left.origY,
			windowinfo->viewport_left.width,
			windowinfo->viewport_left.height,
			windowinfo->viewport_right.origX,
			windowinfo->viewport_right.origY,
			windowinfo->viewport_right.width,
			windowinfo->viewport_right.height,
			windowinfo->viewportF_left.min_X,
			windowinfo->viewportF_left.max_X,
			windowinfo->viewportF_left.min_Y,
			windowinfo->viewportF_left.max_Y,
			windowinfo->viewportF_right.min_X,
			windowinfo->viewportF_right.max_X,
			windowinfo->viewportF_right.min_Y,
			windowinfo->viewportF_right.max_Y,
			windowinfo->viewport_mask,
			windowinfo->call_visreninit,
			windowinfo->VisrenInit,		(windowinfo->VisrenInit !=NULL ? windowinfo->VisrenInit->name : "-"),
			windowinfo->VisrenFrame,	(windowinfo->VisrenFrame !=NULL ? windowinfo->VisrenFrame->name : "-"),
			windowinfo->VisrenWorld,	(windowinfo->VisrenWorld !=NULL ? windowinfo->VisrenWorld->name : "-"),
			windowinfo->VisrenSim,		(windowinfo->VisrenSim !=NULL ? windowinfo->VisrenSim->name : "-"),
			windowinfo->VisrenExit,		(windowinfo->VisrenExit !=NULL ? windowinfo->VisrenExit->name : "-"),
			windowinfo->Open,		(windowinfo->Open !=NULL ? windowinfo->Open->name : "-"),
			windowinfo->Render,		(windowinfo->Render !=NULL ? windowinfo->Render->name : "-"),
			windowinfo->RenderText,		(windowinfo->RenderText !=NULL ? windowinfo->RenderText->name : "-"),
			windowinfo->RenderNullWorld,	(windowinfo->RenderNullWorld !=NULL ? windowinfo->RenderNullWorld->name : "-"),
			windowinfo->RenderSimulator,	(windowinfo->RenderSimulator !=NULL ? windowinfo->RenderSimulator->name : "-"),
			windowinfo->RenderTransform,	(windowinfo->RenderTransform !=NULL ? windowinfo->RenderTransform->name : "-"),
			windowinfo->Swap,		(windowinfo->Swap !=NULL ? windowinfo->Swap->name : "-"),
			windowinfo->Close,		(windowinfo->Close !=NULL ? windowinfo->Close->name : "-"),
			windowinfo->PrintAux,		(windowinfo->PrintAux !=NULL ? windowinfo->PrintAux->name : "-"),
			windowinfo->aux_data);

		/* Now print the auxiliary data, if available */
		if (windowinfo->PrintAux != NULL)
			vrCallbackInvokeDynamic(windowinfo->PrintAux, 3, file, windowinfo->aux_data, style);
		break;

	case file_format:
/* TODO: add e2w_coords -- if it ends up getting used */
		vrFprintf(file, "window \"%s\" = {\n", windowinfo->name);

		if (!windowinfo->malleable)
			vrFprintf(file, "\tmalleable = %d;\n", windowinfo->malleable);

		if (windowinfo->args != NULL)
			vrFprintf(file, "\targs = \"%s\";\n", windowinfo->args);

		vrFprintf(file, "\tgraphicsType = \"%s\";\n", windowinfo->graphics);
		vrFprintf(file, "\tmount = \"%s\";\n", vrWindowName(windowinfo->mount));
		if (windowinfo->settings.visrenmode != VRVISREN_DEFAULT)
			vrFprintf(file, "\tvisrenMode = \"%s\";\n", vrVisrenModeName(windowinfo->settings.visrenmode));
		else	vrFprintf(file, "\t# Inherit from System: visrenMode = <%s>;\n", vrVisrenModeName(windowinfo->visrenmode));

		if (windowinfo->settings.eyelist_name != NULL)
			vrFprintf(file, "\teyelist = \"%s\";", windowinfo->settings.eyelist_name);

		if (windowinfo->num_eyes > 0) {
			vrFprintf(file, "\t# Eyes in use =");
			for (num = 0; num < windowinfo->num_eyes; num++) {
				if (windowinfo->eyes == NULL)
					vrFprintf(file, "");
				else	vrFprintf(file, " \"%s\"", windowinfo->eyes[num]->name);
				if (num+1 < windowinfo->num_eyes)
					vrFprintf(file, ",");
			}
			vrFprintf(file, ";\n");
		}

		if (windowinfo->settings.near_clip != -1 /* VRTOKEN_DEFAULT */)
			vrFprintf(file, "\tnearClip = %.3f;\n", windowinfo->settings.near_clip);
		else	vrFprintf(file, "\t# Inherit from Global: nearClip = \"default\";\n");
		if (windowinfo->settings.far_clip != -1 /* VRTOKEN_DEFAULT */)
			vrFprintf(file, "\tfarClip = %.3f;\n", windowinfo->settings.far_clip);
		else	vrFprintf(file, "\t# Inherit from Global: farClip = \"default\";\n");
		vrFprintf(file, "\n");

		if (windowinfo->simulator_mask != 1)
			vrFprintf(file, "\tsimMask = 0x%02x;\n", windowinfo->simulator_mask);

		if (windowinfo->fps_show != 0)
			vrFprintf(file, "\tshowFps = %d;\n", windowinfo->fps_show);

		if (windowinfo->fps_loc[0] != 0.1 && windowinfo->fps_loc[1] != 0.1)
			vrFprintf(file, "\tplaceFps = %f, %f;\n", windowinfo->fps_loc[0], windowinfo->fps_loc[1]);

		if (windowinfo->fps_color[0] != 1.0 &&
				windowinfo->fps_color[1] != 1.0 &&
				windowinfo->fps_color[2] != 1.0)
			vrFprintf(file, "\tcolorFps = %.4f, %.4f, %.4f;\n", windowinfo->fps_color[0], windowinfo->fps_color[1], windowinfo->fps_color[2]);

		if (windowinfo->stats_show != 0)
			vrFprintf(file, "\tshowStats = %d;\n", windowinfo->stats_show);

		if (windowinfo->stats_procs != NULL)
			vrFprintf(file, "\tstatsProcs = \"%s\";\n", windowinfo->stats_procs);

		if (windowinfo->show_in_simulator != 0)
			vrFprintf(file, "\tshowFrame = %d;\n", windowinfo->show_in_simulator);

		/* TODO: rw2w_translate, rw2w_rotate */
		vrFprintf(file, "\t# TODO: print rw2w_translate, rw2w_rotate\n");

		vrFprintf(file, "}\n\n");
		break;

	}
}


/*****************************************************************/
/* NOTE: the necessity of this function isn't certain. */
vrWindowInfo *vrRenderCurrentWindow(vrRenderInfo *renderinfo)
{
	return (renderinfo->window);
}


/*****************************************************************/
/* NOTE: the necessity of this function isn't certain. */
vrUserInfo *vrRenderCurrentUser(vrRenderInfo *renderinfo)
{
	return (renderinfo->eye->user);
}


/*****************************************************************/
/* NOTE: the necessity of this function isn't certain. */
vrEyeInfo *vrRenderCurrentEye(vrRenderInfo *renderinfo)
{
	return (renderinfo->eye);
}


/*****************************************************************/
vrTime vrRenderCurrentSimTime(vrRenderInfo *renderinfo)
{
	return (renderinfo->frame_stime);
}


/*****************************************************************/
long vrRenderCurrentFrame(vrRenderInfo *renderinfo)
{
	return (renderinfo->frame_count);
}


/*****************************************************************/
void vrRenderNullWorld(vrRenderInfo *renderinfo)
{
	vrCallback	*nullWorld = vrRenderCurrentWindow(renderinfo)->RenderNullWorld;

	if (nullWorld != NULL)
		vrCallbackInvokeDynamic(nullWorld, 1, renderinfo);
}


/*****************************************************************/
void vrRenderDefaultSimulator(vrRenderInfo *renderinfo, int mask)
{
	vrCallback	*simulator = renderinfo->window->RenderSimulator;

	if (simulator != NULL)
		vrCallbackInvokeDynamic(simulator, 2, renderinfo, mask);
}


/*****************************************************************/
void vrRenderText(vrRenderInfo *renderinfo, char *string)
{
	vrCallback	*textfunc = vrRenderCurrentWindow(renderinfo)->RenderText;

	if (textfunc != NULL)
		vrCallbackInvokeDynamic(textfunc, 2, renderinfo, string);
}


/*****************************************************************/
void vrRenderSetWindowContext(vrRenderInfo *renderinfo, void *gfx_context)
{
	renderinfo->window->gfx_context = gfx_context;
}


/*****************************************************************/
void *vrRenderGetWindowContext(vrRenderInfo *renderinfo)
{
	return (renderinfo->window->gfx_context);
}


/*****************************************************************/
void vrRenderTransform(vrRenderInfo *renderinfo, vrMatrix *mat)
{
	vrCallback	*transform = vrRenderCurrentWindow(renderinfo)->RenderTransform;

#if 0 /* MPVR bug */
printf("vrRenderTransform: current window = %p mat = %p transform = %p\n", vrCurrentWindow(renderinfo), mat, transform);
#endif
	if (transform != NULL)
		vrCallbackInvokeDynamic(transform, 1, mat);
}


/************************************************************/
/* use the function associated with the particular type of rendering        */
/*   window to factor in the user's travel location onto the graphics stack */
/* NOTE: this function is to be called from the render process */
void vrRenderTransformUserTravel(vrRenderInfo *renderinfo)
{
#if 0 /* 10/10/06 -- on second thought, rather than putting these new lock functions here, I think they belong inside the mainloop where the renderinfo values are filled in. */
	vrLockReadSet(renderinfo->eye->user->app_lock);
	vrRenderTransform(renderinfo, renderinfo->rw2vw_xform);
	vrLockReadRelease(renderinfo->eye->user->app_lock);
#else
	vrRenderTransform(renderinfo, renderinfo->rw2vw_xform);
#endif
}


/************************************************************/
/* use the function associated with the particular type of rendering        */
/*   window to factor out the user's travel location onto the graphics stack*/
/* NOTE: this function is to be called from the render process */
void vrRenderTransformUserTravelInv(vrRenderInfo *renderinfo)
{
	vrRenderTransform(renderinfo, renderinfo->vw2rw_xform);
}


/************************************************************/
/* NOTE: this function is to be called from the render process */
void vrRenderTransformWindow(vrRenderInfo *renderinfo)
{
	vrRenderTransform(renderinfo, vrRenderCurrentWindow(renderinfo)->rw2w_xform);
}


/************************************************************/
/* use the function associated with the particular type of rendering */
/*   window to factor in the given sensor-6 onto the graphics stack  */
/* NOTE: this function is to be called from the rendering process    */
/* 05/25/2006: decided to return a value indicating whether an error */
/*   has occurred -- i.e. no such sensor number exists.              */
int vrRenderTransform6sensor(vrRenderInfo *renderinfo, int sensor6_num)
{
	vr6sensor	*sensor;
	int		num_6sensors = renderinfo->context->input->num_6sensors;

	/* return without altering the render stack if no such sensor exists */
	if (num_6sensors <= sensor6_num) {
		vrDbgPrintfN(AALWAYS_DBGLVL, "vrRenderTransform6sensor(): called with a 6-sensor value (%d) that doesn't exist (yet?)\n", sensor6_num);
		return 0;
	}

	/* NOTE: in theory, we don't need to do a read-lock here because this */
	/*   data can only be written when none of the rendering processes    */
	/*   are rendering -- only while waiting for synchronization.         */
	sensor = vrGet6sensor(sensor6_num);
	if (sensor)
		vrRenderTransform(renderinfo, sensor->visren_position);

	return 1;
}


/************************************************************/
/* use the function associated with the particular type of rendering   */
/*   window to factor in the given sensor-6 onto the graphics stack    */
/* NOTE: because this is a function called by in a rendering process   */
/*   (hence the name), we use the frozen "visren_position" matrix in   */
/*   the 6-sensor input structure.  Also, because this is called       */
/*   outside the barriers where the freezing takes place, there is no  */
/*   need to surround this use with a read-lock.                       */
void vrRenderTransform6sensorDirect(vrRenderInfo *renderinfo, vr6sensor *sensor6)
{
	/* NOTE: in theory, we don't need to do a read-lock here because this */
	/*   data can only be written when none of the rendering processes    */
	/*   are rendering -- only while waiting for synchronization.         */
	vrRenderTransform(renderinfo, sensor6->visren_position);
}


/*****************************************************************/
void vrRenderSetProjectionTransform(vrRenderInfo *renderinfo, vrPerspData *pd)
{
	vrCallback	*proj_transform = vrRenderCurrentWindow(renderinfo)->SetProjectionTransform;

#if 0 /* MPVR bug */
printf("vrRenderTransform: current window = %p mat = %p transform = %p\n", vrCurrentWindow(renderinfo), mat, transform);
#endif
	if (proj_transform != NULL)
		vrCallbackInvokeDynamic(proj_transform, 1, pd);
}


/******************************************************************/
int vrRenderPushPerspFrom6sensor(vrRenderInfo *renderinfo, int sensor6_num)
{
	vrMatrix	eye_rwpos;	/* matrix representing the position of the new eye's perspective from which to render */
	vr6sensor	*sensor;
	int		num_6sensors = renderinfo->context->input->num_6sensors;

	/* return without altering the projection stack if no such sensor exists */
	if (num_6sensors <= sensor6_num) {
		vrDbgPrintfN(AALWAYS_DBGLVL, "vrRenderPushPerspFrom6sensor(): called with a 6-sensor value (%d) that doesn't exist (yet?)\n", sensor6_num);
		return 0;
	}

	/* return without altering the projection stack if the FreeVR render-info stack is full */
	if (renderinfo->persp_stack_depth >= VR_MAXPERSPDEPTH) {
		vrDbgPrintfN(AALWAYS_DBGLVL, "vrRenderPushPerspFrom6sensor(): Perspective entry stack is already full.\n");
		return 0;
	}

	/* First, need to "push" the stack of perspective entries in the renderinfo structure */
	renderinfo->persp_stack[renderinfo->persp_stack_depth] = *renderinfo->persp;
	renderinfo->persp_stack_depth++;

	/* Second, need to calculate the location of new eye via the given 6-sensor */
	sensor = vrGet6sensor(sensor6_num);
	if (sensor) {
		vrMatrixCopy(&eye_rwpos, sensor->visren_position);

		/* TODO: we need to adjust the eye position based on some user's IOD */
		/*   (for now could just use the given user's IOD, but that isn't a  */
		/*   clean solution).                                                */
#if 1 /* Here is a temporary bit of code that will use the primary user's IOD to do the IOD for the new sensor/user */
/* TODO: do the right thing here, not a temporary band-aid */
/* NOTE: copied from the vrVisrenMainLoop loop from stage 3c. */
		switch (renderinfo->eye->type) {
		case VREYE_DEFAULT:
		case VREYE_CYCLOPS:
			/* NOP: leave the eye at the generic head position */
			break;
		case VREYE_LEFT:
			/* TODO: the sign seems backward on these, need to figure */
			/*  out why the left eye is translated in positive-X, etc.*/
			/*  3/5/3 -- that was yesterday, today, it appears that   */
			/*  yesterday's change was backward, so I went back to the*/
			/*  way it was.                                           */
			vrMatrixPostTranslate3d(&eye_rwpos, -renderinfo->eye->user->iod * 0.5, 0.0, 0.0);
			break;
	case VREYE_VIBRATE: /* TODO: implement this here */
		case VREYE_RIGHT:
			vrMatrixPostTranslate3d(&eye_rwpos,  renderinfo->eye->user->iod * 0.5, 0.0, 0.0);
			break;

		default:
			/* TODO: error */
			break;
		}
#endif
	}

	/* Third, calculate the new perspective data. */
#ifdef USE_FRUSTUMEYE
	vrCalcPerspFrustumEye(renderinfo->persp, renderinfo->window, &eye_rwpos);
#else
	vrCalcPerspMatrix(renderinfo->persp, renderinfo->window, &eye_rwpos);
#endif

	/* Fourth, use the new perspective data to alter the projection stack of the graphics system. */
#if 0
	renderinfo->persp->frustum.n.left = -HUGE_VAL;	/* TODO: NOTE: this is a debugging statement -- to force the else clause in _GlxSetProjectionTransform() */
#endif
	vrRenderSetProjectionTransform(renderinfo, renderinfo->persp);

	return 1;	/* success */
}


/******************************************************************/
int vrRenderPopPersp(vrRenderInfo *renderinfo)
{
	/* return without altering the projection stack if the FreeVR render-info stack is empty */
	if (renderinfo->persp_stack_depth <= 0) {
		vrDbgPrintfN(AALWAYS_DBGLVL, "vrRenderPopPersp(): Perspective entry stack is already empty.\n");
		return 0;
	}

	/* Pop the stack */
	renderinfo->persp_stack_depth--;
	*renderinfo->persp = renderinfo->persp_stack[renderinfo->persp_stack_depth];

	/* Restore the new perspective data onto the projection stack of the graphics system. */
	vrRenderSetProjectionTransform(renderinfo, renderinfo->persp);

	return 1;	/* success */
}


/******************************************************************/
vrEuler *vrRenderGetBillboardAngles3d(vrRenderInfo *renderinfo, vrEuler *eulers, double x_loc, double y_loc, double z_loc)
{
	vrPoint		eyeloc;			/* the location of the eye (aka "camera") */
	vrVector	object_to_eye;		/* vector of object to eye */
	vrVector	object_to_eye_proj;	/* vector of object to eye, disregarding elevation (i.e. projected onto XZ plane) */
	vrVector	lookat_dir;		/* the forward direction of the billboard */
	vrVector	up_dir;			/* the calculated up vector of the billboard (NOTE: set, but unused -- should be +Y axis! */
	double		cos_angle;

	/*******************************************************************/
	/* first, set the Euler translation to be the given x,y,z location */
	eulers->t[VR_X] = x_loc;
	eulers->t[VR_Y] = y_loc;
	eulers->t[VR_Z] = z_loc;

	/**************************************************************/
	/* Now do the real work and calculate the billboarding angles */

	eyeloc = vrRenderCurrentEye(renderinfo)->loc;				/* get the eye's location */
	vrPointTransformByMatrix(&eyeloc, &eyeloc, renderinfo->vw2rw_xform);	/* convert to virtual world coords */

	/**************************************************************/
	/* this part does the cylindrical portion of the billboarding */
	object_to_eye_proj.v[VR_X] = eyeloc.v[VR_X] - x_loc;
	object_to_eye_proj.v[VR_Y] = 0;						/* this is what makes it a projection on the XZ-plane */
	object_to_eye_proj.v[VR_Z] = eyeloc.v[VR_Z] - z_loc;
	vrVectorNormalize(&object_to_eye_proj, &object_to_eye_proj);

	vrVectorSet3d(&lookat_dir, 0, 0, 1);					/* assumption: facing out the +Z axis */
	vrVectorCrossProduct(&up_dir, &lookat_dir, &object_to_eye_proj);	/* because we project onto the XZ-plane up_dir s/b up or down */
	/* NOTE: to be pedantic, we could verify that updir.v[VR_X] & updir.v[VR_Z] are 0.0 */

	cos_angle = vrVectorDotProduct(&lookat_dir, &object_to_eye_proj);
	if (up_dir.v[VR_Y] < 0.0)
		eulers->r[VR_AZIM] = -vrRadiansToDegrees(acos(cos_angle));
	else	eulers->r[VR_AZIM] =  vrRadiansToDegrees(acos(cos_angle));

	/*********************************************************************/
	/* this part does the rest of the process for spherical billboarding */
	object_to_eye.v[VR_X] = eyeloc.v[VR_X] - x_loc;
	object_to_eye.v[VR_Y] = eyeloc.v[VR_Y] - y_loc;
	object_to_eye.v[VR_Z] = eyeloc.v[VR_Z] - z_loc;
	vrVectorNormalize(&object_to_eye, &object_to_eye);

	cos_angle = vrVectorDotProduct(&object_to_eye_proj, &object_to_eye);

	/* NOTE: it is generally wise to make sure the cos_angle is definitely within (-1.0,1.0) */
	if (object_to_eye.v[VR_Y] < 0.0)
		eulers->r[VR_ELEV] =  vrRadiansToDegrees(acos(cos_angle));
	else	eulers->r[VR_ELEV] = -vrRadiansToDegrees(acos(cos_angle));

	/* NOTE: At this point, if I want to do twist, I need to specify an up-vector for the eye */
	eulers->r[VR_ROLL] = 0.0;

	return eulers;
}


/******************************************************************/
vrEuler *vrRenderGetBillboardAnglesAd(vrRenderInfo *renderinfo, vrEuler *eulers, double *loc)
{
	return vrRenderGetBillboardAngles3d(renderinfo, eulers, loc[VR_X], loc[VR_Y], loc[VR_Z]);
}


/******************************************************************/
void vrRenderCategory(vrRenderInfo *renderinfo, int tag)
{
	switch (tag) {
	case 1:
		vrProcessStatsMark(renderinfo->window->proc->stats, VR_TIME_RENDER1, 1);
		break;
	case 2:
		vrProcessStatsMark(renderinfo->window->proc->stats, VR_TIME_RENDER2, 1);
		break;
	default:
		vrErrPrintf(RED_TEXT "vrSystemSimStats(): Warning, the statistics tag must be 0 or 1\n" NORM_TEXT);
		break;
	}

	return;
}

#ifdef NOT_CURRENTLY_USED /* { */
/******************************************************************/
/* vrVisrenSignalHandler(): on a SIGINT signal, this process will */
/*   close down all devices it's responsible for and exit.        */
/******************************************************************/
static void vrVisrenSignalHandler(int which)
{
	int		count_window = 0;

	switch (which) {
	case SIGUSR2:
		vrMsgPrintf("vrVisrenSignalHandler(): Hey, input process got a USR2 signal\n");
		vrCallbackInvoke(vrContext->callbacks->HandleUSR2);
		break;

	default:
		/* for all other signals, assume death */
		vrMsgPrintf("vrVisrenSignalHandler(): (visren proc %d) " RED_TEXT "dying with %s\n" NORM_TEXT, getpid(), vrSigName(which));

		vrMsgPrintf("vrVisrenSignalHandler(): Time of death = %f, number of frames = %d\n",
                        proc_info->frame_wtime, proc_info->frame_count);

		vrMsgPrintf("vrVisrenSignalHandler(): Shared memory usage:%7ld (%ld freed)\n",
			vrShmemUsage(), vrShmemFreed());

		proc_info->end_proc = 1;
		vrFprintProcessInfo(stdout, proc_info, verbose);

#ifdef COREDUMP_IS_GOOD
		vrFprintf(stderr, "vrVisrenSignalHandler(): YO aborting in vr_input.c\n");
		abort();
#endif

		for (count_window = 0; count_window < proc_info->num_things; count_window++) {
			if (proc_info->things[count_window])
				vrCallbackInvoke(proc_info->things[count_window]->Close);
		}

		vrVisrenTermProc(proc_info);		/* TODO: not sure we want to do this */

		/* PAUSE to allow debugger to attach to the process. */
		vrMsgPrintf("vrVisrenSignalHandler(): Process pausing, use 'dbx -p %d' to debug.\n", getpid());
		pause();
	}
}
#endif /* } */


/******************************************************************/
/* vrVisrenInitProc(): Initialize the rendering devices assocated */
/*   with this process.                                           */
/******************************************************************/
void vrVisrenInitProc(vrProcessInfo *myproc_info)
{
	_VisrenPrivate	*visren_aux;
	vrRenderInfo	*renderinfo;				/* information passed to each render routine */
	vrCallback	*callback;
	int		wincount;
	int		count;
	int		total_eyes = 0;

	vrTrace("vrVisrenInitProc", "beginning");
#if 0
	proc_info = myproc_info;		/* "proc_info" is not used in vr_visren.c -- typically is only for the signal handler */
#endif

#if 0 /* TODO: readd this -- not doing this allows for seg faults, so we can debug easier (but it requires the global "proc_info") */
	vrSetSignalHandler(vrVisrenSignalHandler);
#endif

	if (vrDbgDo(DEFAULT_DBGLVL+1)) {
		vrMsgPrintf("Visren proc is: ");
		vrFprintProcessInfo(stdout, myproc_info, verbose);
	}

	/* TODO: this isn't really an error, but for the moment (6/7/01),    */
	/*   apps that don't assign the callbacks prior to calling vrStart() */
	/*   may experience difficulties.                                    */
	if (vrContext->callbacks->VisrenWorld == NULL)
		vrDbgPrintfN(AALWAYS_DBGLVL, "vrVisrenInitProc(): Warning: no general visual rendering callback yet assigned\n");

	/* create your own little memory space for proc-specific stuff */
	myproc_info->aux_data = (_VisrenPrivate *)vrShmemAlloc(sizeof(_VisrenPrivate));
	visren_aux = myproc_info->aux_data;
	visren_aux->renderinfo = (vrRenderInfo *)vrShmemAlloc0(sizeof(vrRenderInfo));

	renderinfo = visren_aux->renderinfo;
	renderinfo->persp = (vrPerspData *)vrShmemAlloc(sizeof(vrPerspData));


	renderinfo->context = myproc_info->context;
	renderinfo->window = NULL;	/* this will be set before renderinfo gets passed to a CB */
	renderinfo->eye = NULL;		/* the init and frame callbacks are not done on a per-eye basis */
	renderinfo->frame_stime = 0.0;
	renderinfo->frame_count = -1;	/* We haven't even begun to start rendering frames */
	renderinfo->gfx_context = NULL;	/* This is set and used by the application programmer */

	/* copy window info pointers into static array (to avoid the many   */
	/*   levels of indirection) and initialize the windows as necessary */
	visren_aux->num_windows = myproc_info->num_things;
	visren_aux->windows = (vrWindowInfo **)vrShmemAlloc(visren_aux->num_windows * sizeof(vrWindowInfo *));
	for (wincount = 0; wincount < visren_aux->num_windows; wincount++) {
		visren_aux->windows[wincount] = (vrWindowInfo *)(myproc_info->things[wincount]);
		visren_aux->curr_window = visren_aux->windows[wincount];
		renderinfo->window = visren_aux->curr_window;

		if (visren_aux->curr_window == NULL) {
			vrErrPrintf("vrVisrenInitProc(): " RED_TEXT "attempting to initialize a NULL window for thing %d -- this shouldn't happen!\n" NORM_TEXT, wincount);
			continue;
		}
		vrCalcPerspIntermediaries(visren_aux->curr_window);	/* do some pre-calculations for the perspective matrix */

		/* set the initial rendering callbacks to the global values */
		visren_aux->curr_window->VisrenInit = vrContext->callbacks->VisrenInit;
		visren_aux->curr_window->call_visreninit = 1;
		visren_aux->curr_window->VisrenExit = vrContext->callbacks->VisrenExit;
		visren_aux->curr_window->VisrenFrame = vrContext->callbacks->VisrenFrame;
		visren_aux->curr_window->VisrenWorld = vrContext->callbacks->VisrenWorld;
		visren_aux->curr_window->VisrenSim = vrContext->callbacks->VisrenSim;

		vrTrace("vrVisrenInitProc", "opening a window");
		vrCallbackInvoke(visren_aux->curr_window->Open);
		vrTrace("vrVisrenInitProc", "done: opening a window");

		/* make the visual rendering initialization callback */
		/*   NOTE: the correct GLX context should be active from the Open Callback */
		vrTrace("vrVisrenInitProc", "calling initialization callback");
		callback = visren_aux->curr_window->VisrenInit;
		vrCallbackInvokeDynamic(callback, 1, renderinfo);
		visren_aux->curr_window->call_visreninit = 0;
		vrTrace("vrVisrenInitProc", "done: initialization callback");
	}


	/*** TODO: we may want to wait here for the vrInput structure to be fully initialized. ***/
	/***   (otherwise, if the rendering function tries to make use of vrInput data without ***/
	/***   first making sure it's been filled in, then it could seg-fault.)                ***/
	/*** Then again, by not waiting here, the render process is free to render its world   ***/
	/***   without waiting for some (potentially slow initializing) inputs.                ***/

	/****************************************************************/
	/*** count the windows and eyes in the system -- quit if none ***/
	vrDbgPrintfN(SELDOM_DBGLVL, "vrVisrenInitProc(): num windows to render is %d\n", visren_aux->num_windows);

	if (visren_aux->num_windows == 0) {
		vrErrPrintf("vrVisrenInitProc(): " RED_TEXT "Hmmm, no reason to continue the visren loop with no windows.\n" NORM_TEXT);
		return;
	}

	for (wincount = 0; wincount < visren_aux->num_windows; wincount++) {
		total_eyes += visren_aux->windows[wincount]->num_eyes;
	}
	if (total_eyes == 0) {
		vrErrPrintf("vrVisrenInitProc(): " RED_TEXT "Hmmm, no reason to continue the visren loop with no eyes in any window.\n" NORM_TEXT);
		return;
	}


#ifdef GFX_PERFORMER
	/* TODO: we may not need this special compilation if the Open callback */
	/*   function simply sets the "myproc_info->end_proc" flag to 1 itself.  */
	/*   Hmmm, on the other hand, there is a problem in that the visren    */
	/*   process in the Performer library doesn't have it's own proc_info. */
	vrTrace("vrVisrenInitProc", "Performer exits from vrVisrenInitProc() after opening windows.");
	return;
#endif

#if 0 /* some test code to determine the clock resolution of different machines with POSIX clock */
{
#  include <time.h>
	struct timespec	time_data;

#  if 0
	clock_getres(CLOCK_PROCESS_CPUTIME_ID, &time_data);
	clock_getres(CLOCK_SGI_CYCLE, &time_data);
	clock_getres(CLOCK_SGI_FAST, &time_data);
#  else
	clock_getres(CLOCK_REALTIME, &time_data);
#  endif
	vrPrintf("resolution = %d sec, %d nsec\n", time_data.tv_sec, time_data.tv_nsec);
}
#endif

	/************************************/
	/*** Initialize timing statistics ***/
	/************************************/
	if (myproc_info->stats_args) {
		myproc_info->stats = vrProcessStatsCreate(myproc_info->name, VR_TIME_TRAVEL+1, myproc_info->stats_args);
		myproc_info->stats->elem_labels[VR_TIME_INIT]    = vrShmemStrDup("init");
		myproc_info->stats->elem_labels[VR_TIME_FRAME]   = vrShmemStrDup("frame");
		myproc_info->stats->elem_labels[VR_TIME_RENDER1] = vrShmemStrDup("render-1");
		myproc_info->stats->elem_labels[VR_TIME_RENDER2] = vrShmemStrDup("render-2");
		myproc_info->stats->elem_labels[VR_TIME_RENDERINFO] = vrShmemStrDup("info-render");
		myproc_info->stats->elem_labels[VR_TIME_WAIT]    = vrShmemStrDup("wait");
		myproc_info->stats->elem_labels[VR_TIME_SYNC]    = vrShmemStrDup("sync");
		myproc_info->stats->elem_labels[VR_TIME_FREEZE]  = vrShmemStrDup("freeze");
		myproc_info->stats->elem_labels[VR_TIME_SWAP]    = vrShmemStrDup("swap");
		myproc_info->stats->elem_labels[VR_TIME_TRAVEL]  = vrShmemStrDup("");	/* TODO: in the future "travel" */

		/* And make the three rendering stats colors to be somewhat alike, */
		/*   but distinguishable from the others.                          */
		myproc_info->stats->elem_colors[VR_TIME_RENDER2][0] = 0.20;
		myproc_info->stats->elem_colors[VR_TIME_RENDER2][1] = 0.85;
		myproc_info->stats->elem_colors[VR_TIME_RENDER2][2] = 0.85;

		myproc_info->stats->elem_colors[VR_TIME_RENDERINFO][0] = 0.20;
		myproc_info->stats->elem_colors[VR_TIME_RENDERINFO][1] = 0.75;
		myproc_info->stats->elem_colors[VR_TIME_RENDERINFO][2] = 0.75;
	}

}


/******************************************************************/
/* vrVisrenTermProc(): Terminate the rendering devices assocated  */
/*   with this process.                                           */
/* NOTE: currently this is a no-op.                               */
/* TODO: consider doing something other than no-op.               */
/******************************************************************/
void vrVisrenTermProc(vrProcessInfo *myproc_info)
{
}


/*****************************************************************************/
void vrVisrenOneFrame(vrProcessInfo *myproc_info)
{
static	char		trace_msg[256];
	_VisrenPrivate	*visren_aux;
	vrRenderInfo	*renderinfo;				/* information passed to each render routine */
	vrWindowInfo	*window;
	int		count_window;
	int		count_eye;

	visren_aux = myproc_info->aux_data;
	renderinfo = visren_aux->renderinfo;

	/*********************************************/
	/** graphics buffer swap & update callbacks **/

	/* loop through all the windows associated with THIS process only */
	for (count_window = 0; count_window < visren_aux->num_windows; count_window++) {
		window = visren_aux->windows[count_window];

		/* first report any errors that might have occurred during the rendering */
		vrCallbackInvoke(window->Errors);

		/* now swap the graphics */
		vrCallbackInvoke(window->Swap);

		/* now copy over any callbacks that need updating */
		/* TODO: we may want to put this inside a general lock covering all callbacks */
		window->call_visreninit |= vrCallbackUpdate(&window->VisrenInit, &vrContext->callbacks->VisrenInit);
if (window->call_visreninit != 0) {
printf("vrVisrenOneFrame(): HEY, just set visreninit to %d in callback updates section of window %s\n", window->call_visreninit, window->name);
}

		vrCallbackUpdate(&window->VisrenExit, &vrContext->callbacks->VisrenExit);
		vrCallbackUpdate(&window->VisrenFrame, &vrContext->callbacks->VisrenFrame);
		vrCallbackUpdate(&window->VisrenWorld, &vrContext->callbacks->VisrenWorld);
		vrCallbackUpdate(&window->VisrenSim, &vrContext->callbacks->VisrenSim);
	}
	vrTrace("vrVisrenOneFrame", "after display buffer swap & callback updates");

	/* measure: time spent swapping and updating callbacks */
	vrProcessStatsMark(myproc_info->stats, VR_TIME_SWAP, 0);

	/* calculate frame rates and set the process and renderinfo time values */
	myproc_info->frame_count++;
	vrProcessCalcFrameRate(myproc_info);

	renderinfo->frame_stime = vrSimTimeOf(myproc_info->frame_wtime);
	renderinfo->frame_count = myproc_info->frame_count;

	/* measure: update time measurement array index */
	vrProcessStatsNextFrame(myproc_info->stats);

	vrTrace("vrVisrenOneFrame", "after frame rate calculations");


	/*****************************************/
	/*** (??) Input/travel cluster sharing ***/
	/*****************************************/

	/* TODO: this is potentially where we would then share data with    */
	/*   other visual rendering processes running on separate computers */
	/*   in a clustered environment.  Or, maybe it should be in the     */
	/*   "group_master" block in phase 3, above.                        */
	/* Another possibility is to have the cluster-master/slave processes*/
	/*   be part of the local visren sync-group, and then use them to   */
	/*   handle the data sharing in sync with the visual rendering.     */


	/************************************************************/
	/*** (3) loop over each window of this process and render ***/
	/************************************************************/
	for (count_window = 0; count_window < visren_aux->num_windows; count_window++) {
		int	num_eyes;			/* for counting through the eyes */

		sprintf(trace_msg, "beginning window render loop for window %d", count_window);
		vrTrace("vrVisrenOneFrame", trace_msg);

		visren_aux->curr_window = visren_aux->windows[count_window];
		renderinfo->window = visren_aux->windows[count_window];
		renderinfo->eye = NULL;		/* the init and frame callbacks are not done on a per-eye basis */

#define GFXINIT_TOP 2	/* set to 1 to do here, 0 to do after rendering, and 2 to do not at all -- or in rendering */
#if GFXINIT_TOP == 1 /* test of moving until after rendering -- ie. after the proper GLXcontext is set */
		/******************************************/
		/*** (3a) call initialization functions ***/
		/******************************************/

		/* The initialization functions are only called once per window  */
		/*   per setting of the initialization function, so this is only */
		/*   done when a new VisrenInit callback was assigned.           */
		if (visren_aux->curr_window->call_visreninit == 1) {
vrPrintf("vrVisrenOneFrame()-A: Calling VisrenInit callback for window '%s' -- call_visreninit was set to 1.\n", visren_aux->curr_window->name);
			/* TODO: consider whether a per-user initialization routine */
			/*   is important.  This implementation does not refer to a */
			/*   per-user init callback -- because we don't know the    */
			/*   user until we get down to phase (3c).                  */
			vrTrace("vrVisrenOneFrame", "prep: initialization callback");
			callback = visren_aux->curr_window->VisrenInit;
			vrCallbackInvokeDynamic(callback, 1, renderinfo);
			visren_aux->curr_window->call_visreninit = 0;
			vrTrace("vrVisrenOneFrame", "done: initialization callback");
		}

		/* measure: time spent in init function */
		vrProcessStatsMark(myproc_info->stats, VR_TIME_INIT, 1);
#endif

		/*********************************/
		/*** (3b) call frame functions ***/
		/*********************************/

		/* NOTE: frame functions are called once per frame, per window */


		/* call global frame function (same for all windows) */
		vrCallbackInvokeDynamic(vrContext->callbacks->VisrenFrame, 1, renderinfo);
		vrTrace("vrVisrenOneFrame", "after GENERAL visrenframe callback");

		/* TODO: add window-specific frame callback (VR_ONE_DISPLAY_FRAME) */

#if 0 /* TODO: determine if we can to call user-specific frame functions here  */
/* Hmmm, here is probably bad since we don't know which user we are    */
/* until we know what eye we're rendering for.  Perhaps it should just */
/* be a generic user-specific rendering supplement.                    */
		/* invoke this user's frame function ... */
		vrCallbackInvokeDynamic(visren_aux->curr_user->VisrenFrame, 1, renderinfo);
		vrTrace("vrVisrenOneFrame", "after USER visrenframe callback");
#endif
		/* measure: time spent in frame function */
		vrProcessStatsMark(myproc_info->stats, VR_TIME_FRAME, 1);


		/****************************/
		/*** (3c) foreach eye ... ***/
		/****************************/
		num_eyes = visren_aux->curr_window->num_eyes;
		if (num_eyes == 0) {
			/* There's no reason to continue the loop with no eyes.  Is there?*/
			continue;
		}

		for (count_eye = 0; count_eye < num_eyes; count_eye++) {
			vrMatrix	head_rwpos;
			vrMatrix	eye_rwpos;

			visren_aux->curr_eye = visren_aux->curr_window->eyes[count_eye];
			visren_aux->curr_user = visren_aux->curr_eye->user;
			renderinfo->eye = visren_aux->curr_eye;
			vrLockReadSet(renderinfo->eye->user->app_lock);		/* NOTE: this can probably go away if the user->visren_Xw2Yw data are double buffered in the call to vrFrame() */
			vrLockReadSet(visren_aux->curr_user->visren_lock);	/* NOTE: there is probably no need for this anymore -- not that it did much in the past */
			renderinfo->rw2vw_xform = visren_aux->curr_user->visren_rw2vw;
			renderinfo->vw2rw_xform = visren_aux->curr_user->visren_vw2rw;
			vrLockReadRelease(visren_aux->curr_user->visren_lock);	/* NOTE: there is probably no need for this anymore -- not that it did much in the past */
			vrLockReadRelease(renderinfo->eye->user->app_lock);	/* NOTE: this can probably go away if the user->visren_Xw2Yw data are double buffered in the call to vrFrame() */

			/***********************************************************/
			/* now compute the current eye position based on the world */
			/*   position information from the tracker for this user.  */
			sprintf(trace_msg, "about to calculate the eye position for eye %d", count_eye);
			vrTrace("vrVisrenOneFrame", trace_msg);

#if 0 /* 1 = the unsynced version, 0 = the new frozen-data-storage version */
			vrMatrixGet6sensorValuesDirectNoLastUpdate(&head_rwpos, visren_aux->curr_user->head);
#else
			/* NOTE: no need for a read-lock, since this value can */
			/*   only be written in phase 3 of this loop.          */
			vrMatrixCopy(&head_rwpos, visren_aux->curr_user->visren_headpos);
#endif
			vrMatrixCopy(&eye_rwpos, &head_rwpos);

			switch (visren_aux->curr_eye->type) {
			case VREYE_DEFAULT:
			case VREYE_CYCLOPS:
				/* NOP: leave the eye at the generic head position */
#ifndef VIBRATE_TEST	/* when testing vibrate mode have the cyclops renderings fall into the vibrate code */
				break;
#endif
			case VREYE_VIBRATE:
				if ((renderinfo->frame_count / 20) % 2) {
					vrMatrixPostTranslate3d(&eye_rwpos, /*-visren_aux->curr_user->iod */-.3 * 0.5, 0.0, 0.0);
				} else {
					vrMatrixPostTranslate3d(&eye_rwpos, /* visren_aux->curr_user->iod */.3 * 0.5, 0.0, 0.0);
				}
				break;
			case VREYE_LEFT:
				/* TODO: the sign seems backward on these, need to figure */
				/*  out why the left eye is translated in positive-X, etc.*/
				/*  3/5/3 -- that was yesterday, today, it appears that   */
				/*  yesterday's change was backward, so I went back to the*/
				/*  way it was.                                           */
				vrMatrixPostTranslate3d(&eye_rwpos, -visren_aux->curr_user->iod * 0.5, 0.0, 0.0);
				break;
			case VREYE_RIGHT:
				vrMatrixPostTranslate3d(&eye_rwpos,  visren_aux->curr_user->iod * 0.5, 0.0, 0.0);
				break;


			default:
				/* TODO: error */
				break;
			}

			/**********************************************************/
			/* now compute the perspective matrix for this window/eye */
			vrTrace("vrVisrenOneFrame", "about to calculate the perspective matrix");

			/* TODO: for hand-based displays, we'll need to give information  */
			/*   on where the window is located to vrCalcPerspMatrix().       */
			/*   This could be done either by putting that info in the        */
			/*   renderinfo->window structure, or by passing another argument.*/
			/*   I prefer the former [BS: 9/13/2000].                         */
#ifdef USE_FRUSTUMEYE
			vrCalcPerspFrustumEye(renderinfo->persp, renderinfo->window, &eye_rwpos);
#else
			vrCalcPerspMatrix(renderinfo->persp, renderinfo->window, &eye_rwpos);
#endif

			/* Copy the eye's location into the data-structure of the current */
			/*   eye (and which is redundantly pointed to via vrRenderInfo).  */
			vrPointGetTransFromMatrix(&renderinfo->eye->loc, &eye_rwpos);

			/********************/
			/* do the rendering */
			vrTrace("vrVisrenOneFrame", "about to do the rendering");

			/* NOTE: the ...->Render callback is for the specific type of     */
			/*   graphics system (eg. GLX, Performer).  Currently (6/21/2001) */
			/*   it is that routine that will then determine the correct      */
			/*   world rendering callback to use (system, window, user).      */
			/*   TODO: consider whether to make that determination here, and  */
			/*     pass that callback as an argument to the ->Render callback.*/
			vrCallbackInvokeDynamic(visren_aux->windows[count_window]->Render, 1, renderinfo);

			vrTrace("vrVisrenOneFrame", "done rendering");
		}

		/* measure: time spent in rendering each eye */
		vrProcessStatsMark(myproc_info->stats, VR_TIME_RENDER1, 1);

#if GFXINIT_TOP == 0 /* test of moving until after rendering -- ie. after the proper GLXcontext is set */
		/******************************************/
		/*** (3a) call initialization functions ***/
		/******************************************/

		/* The initialization functions are only called once per window  */
		/*   per setting of the initialization function, so this is only */
		/*   done when a new VisrenInit callback was assigned.           */
		if (visren_aux->curr_window->call_visreninit == 1) {
vrPrintf("vrVisrenOneFrame()-B: Calling VisrenInit callback for window '%s' -- call_visreninit was set to 1.\n", visren_aux->curr_window->name);
			/* TODO: consider whether a per-user initialization routine */
			/*   is important.  This implementation does not refer to a */
			/*   per-user init callback -- because we don't know the    */
			/*   user until we get down to phase (3c).                  */
			vrTrace("vrVisrenOneFrame", "prep: initialization callback -- yo");
			callback = visren_aux->curr_window->VisrenInit;
			vrCallbackInvokeDynamic(callback, 1, renderinfo);
			visren_aux->curr_window->call_visreninit = 0;
			vrTrace("vrVisrenOneFrame", "done: initialization callback");
		}

		/* measure: time spent in init function */
		vrProcessStatsMark(myproc_info->stats, VR_TIME_INIT, 1);
#endif

		sprintf(trace_msg, "ending window render loop for window %d", count_window);
		vrTrace("vrVisrenOneFrame", trace_msg);
	}
}


/*****************************************************************/
/* vrVisrenMainLoop(): Loop over all visual renderings until the */
/*   end of the application is flagged.                          */
/*****************************************************************/
void vrVisrenMainLoop(vrProcessInfo *myproc_info)
{
	_VisrenPrivate	*visren_aux;
	vrTime		loop_wtime = vrCurrentWallTime();	/* time of the beginning of each loop */

#if 0 /* Moving this to vr_procs.c */
	vrVisrenInitProc(myproc_info);
#endif

	visren_aux = myproc_info->aux_data;

	/*************************************************************/
	/*** now enter a (seemingly) infinite loop to do rendering ***/
	vrTrace("vrVisrenMainLoop", "starting rendering loop");
	while (!myproc_info->end_proc) {

		int		sync_order;		/* order in which this process hit the sync barrier */
#ifndef NEW_BS_METHOD
		int		iwaslast;
#endif

		vrTrace("vrVisrenMainLoop", BOLD_TEXT "*** top of render loop ***" NORM_TEXT);

		/***************************************************************/
		/*** (1) synchronize on this process-group's barrier         ***/
		/***     for swapping the graphics buffer of each window and ***/
		/***     for updating any newly declared visren callbacks.   ***/
		/***************************************************************/

#ifndef NEW_BS_METHOD /* Old method of synchronizing and freezing { */

		/* The last process to sync is responsible for updating inputs, etc */
		/*   NOTE: but not when paused */
		vrTrace("vrVisrenMainLoop", "before barrier last to sync check");
		if (!vrContext->paused) {
			if (vrBarrierLastToSync(myproc_info->barrier)) {
				vrTrace("vrVisrenMainLoop", "before freezing the input/travel/etc data");
				vrInputFreezeVisren(myproc_info->context);
				vrUserTravelFreezeVisren(myproc_info->context);
				vrPropFreezeVisren(myproc_info->context);
				vrTrace("vrVisrenMainLoop", "after freeze, about to sync");
				iwaslast = 1;
			} else {
				vrTrace("vrVisrenMainLoop", "about to sync");
				iwaslast = 0;
			}
		}

		/* measure: determine whether we should measure times during this frame */

		/* measure: time spent freezing */
		vrProcessStatsMark(myproc_info->stats, VR_TIME_FREEZE, 0);	/* in old sync method */

		/* do minimal frame delay */
		vrTrace("vrVisrenMainLoop", "about to do minimal frame delay");
		vrSleep(myproc_info->usec_min - (long)((vrCurrentWallTime() - loop_wtime) * 1000000.0));
		/* measure: time spent waiting for minimal frame time */
		vrProcessStatsMark(myproc_info->stats, VR_TIME_WAIT, 0);	/* in old sync method */

		/**********/
		/** sync **/
		sync_order = vrBarrierSync(myproc_info->barrier);
		myproc_info->frame_wtime = myproc_info->barrier->wtime;
#  if 1 /* this is for testing the barrier race condition */
		if (iwaslast && (sync_order != myproc_info->barrier->num_clients)) {
			vrPrintf("Warning: I was reported as last, but was %d to sync, not %d\n",
				sync_order, myproc_info->barrier->num_clients);
		}
		if (!iwaslast && (sync_order == myproc_info->barrier->num_clients)) {
			vrPrintf("Warning: I was not reported as last, but was %d to sync, of %d\n",
				sync_order, myproc_info->barrier->num_clients);
		}
#  endif
		vrTrace("vrVisrenMainLoop", "after process sync barrier");

		/* measure: time spent waiting for sync */
		vrProcessStatsMark(myproc_info->stats, VR_TIME_SYNC, 0);	/* in old sync method */

		/* start measuring for minimal loop time */
		loop_wtime = vrCurrentWallTime();
#elif 0 /* } The following is the new method of doing the sync-freeze stuff { */
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

		/**************************/
		/* do minimal frame delay */
		vrTrace("vrVisrenMainLoop", "about to do minimal frame delay");
		vrSleep(myproc_info->usec_min - (long)((vrCurrentWallTime() - loop_wtime) * 1000000.0));
		/* measure: time spent waiting for minimal frame time */
		vrProcessStatsMark(myproc_info->stats, VR_TIME_WAIT, 0);	/* in new sync method */

		/**********/
		/** sync **/
		vrTrace("vrVisrenMainLoop", "about to do sync-1");
		sync_order = vrBarrierSync(myproc_info->barrier);
		vrDbgPrintfN(TRACE_DBGLVL, "(%s::%d) %s -> " RED_TEXT "after process sync barrier -- sync_order = %d" NORM_TEXT "\n", __FILE__, __LINE__, "vrVisrenMainLoop", sync_order);
		vrDbgPrintfN(BARRIER_DBGLVL, "vrVisrenMainLoop(): " RED_TEXT "after process sync barrier -- sync_order = %d\n" NORM_TEXT, "vrVisrenMainLoop", sync_order);

		/* measure: time spent waiting for sync */
		vrProcessStatsMark(myproc_info->stats, VR_TIME_SYNC, 0);	/* in new sync method */

		/************/
		/** freeze **/

		/* The first process to sync is responsible for updating inputs, etc */
		vrTrace("vrVisrenMainLoop", "before barrier first to sync check");
		if (sync_order <= 1 && !vrContext->paused) {
			/* NOTE: "<= 1" comparison is used because a sync_order of 0 means no sync-group, */
			/*   so someone has to do the copying, and we are forced to assume this process   */
			/*   must do it.                                                                  */
			vrTrace("vrVisrenMainLoop", "before freezing the input/travel/etc data");
			vrInputFreezeVisren(myproc_info->context);
			vrUserTravelFreezeVisren(myproc_info->context);
			vrPropFreezeVisren(myproc_info->context);
			vrTrace("vrVisrenMainLoop", "after freeze, about to sync");
		} else {
			/* NOTE: we really shouldn't get here when paused, so if any actual */
			/*   work is done here, need to check for the "paused" state.       */
			vrTrace("vrVisrenMainLoop", "about to sync");
		}
#  if 0 /* 03/03/05 -- changed from "1" to "0", and now process proceeds  -- this barrier was added 06/04/03 */
		/* TODO: consider waiting also for the callback updates */
		vrTrace("vrVisrenMainLoop", "about to do sync-2");
		sync_order = vrBarrierSync(myproc_info->barrier2);		/* Now barrier to wait for the freezing */
		vrDbgPrintfN(TRACE_DBGLVL, "(%s::%d) %s -> " RED_TEXT "after process freeze barrier -- sync_order = %d" NORM_TEXT "\n", __FILE__, __LINE__, "vrVisrenMainLoop", sync_order);
		vrDbgPrintfN(BARRIER_DBGLVL, "vrVisrenMainLoop(): " RED_TEXT "after process freeze barrier -- sync_order = %d\n" NORM_TEXT, "vrVisrenMainLoop", sync_order);
#  endif
		if (sync_order == 0)
			myproc_info->frame_wtime = vrCurrentWallTime();
		else	myproc_info->frame_wtime = myproc_info->barrier->wtime;

		/* measure: time spent freezing */
		vrProcessStatsMark(myproc_info->stats, VR_TIME_FREEZE, 0);	/* in new sync method */
#else /* this method calls a function that will handle synchronization the same in all processes */
/* 08/28/14 -- This is the actual bit of code that is presently used. */
		/**************************/
		/* do minimal frame delay */
		vrTrace("vrVisrenMainLoop", "about to do minimal frame delay");
		vrSleep(myproc_info->usec_min - (long)((vrCurrentWallTime() - loop_wtime) * 1000000.0));
		/* measure: time spent waiting for minimal frame time */
		vrProcessStatsMark(myproc_info->stats, VR_TIME_WAIT, 0);	/* in new new sync method */

		/************************************/
		/* Synchronize with other processes */
		vrProcessSync(myproc_info, VR_TIME_SYNC, VR_TIME_FREEZE);

#endif /* } end new sync method */

		/* start measuring for minimal loop time */
		loop_wtime = vrCurrentWallTime();

		vrVisrenOneFrame(myproc_info);
	}
}


/**************************************************************/
/* vrVisrenDefaultInfo(): assigns some harmless callbacks for */
/*   window rendering as the default -- we can't really do    */
/*   anything until it is known what type of graphics system  */
/*   the application will use, and thus it is up to each      */
/*   graphics system to make the actual assignments.          */
/**************************************************************/
static void vrVisrenDefaultInfo(vrWindowInfo *info)
{
	info->graphics = vrShmemStrDup("null");
	info->version = (char *)vrShmemStrDup("Uninitialized (null) render window");
	info->PreOpenInit = vrCallbackCreateNamed("DefWindow:PreOpenInit-DN", vrDoNothing, 0);
	info->Open = vrCallbackCreateNamed("DefWindow:Open-DN", vrDoNothing, 0);
	info->Render = vrCallbackCreateNamed("DefWindow:Render-DN", vrDoNothing, 0);
	info->RenderText = vrCallbackCreateNamed("DefWindow:RenderText-DN", vrDoNothing, 0);
	info->RenderNullWorld = vrCallbackCreateNamed("DefWindow:RenderNW-DN", vrDoNothing, 0);
	info->RenderSimulator = vrCallbackCreateNamed("DefWindow:RenderSim-DN", vrDoNothing, 0);
	info->RenderTransform = vrCallbackCreateNamed("DefWindow:Transform-DN", vrDoNothing, 0);
	info->Swap = vrCallbackCreateNamed("DefWindow:Swap-DN", vrDoNothing, 0);
	info->Errors = vrCallbackCreateNamed("DefWindow:Errors-DN", vrDoNothing, 0);
	info->Close = vrCallbackCreateNamed("DefWindow:Close-DN", vrDoNothing, 0);
}


/*******************************************************************/
int vrVisrenDeviceInList(char *device)
{
	vrVisrenOptsType	*option;	/* for looping through the list of possible visren devices */

	/* Loop through the visren device options specified in vr_visren.opts.h   */
	/*   until a string match is made with the "device" string, then call the */
	/*   associated initialization function to fill in the info structure.    */
	option = vrVisrenOpts;
	while (option->option_name != NULL) {
		if (!strcasecmp(option->option_name, device)) {
			return (1);
		}
		option++;
	}

	return (0);
}


/*********************************************************************/
/* vrVisrenGetInfo(): uses vrVisrenOpts[] array to fill in the       */
/*   window structure with the GetWindowInfo(), vrRenderNullWorld(), */
/*   vrRenderDefaultSimulator(), and vrRenderText() functions.       */
/*********************************************************************/
void vrVisrenGetInfo(vrWindowInfo *info)
{
	char		*graphics = info->graphics;
	vrVisrenOptsType *option;

	if (!graphics) {
		vrVisrenDefaultInfo(info);
		return;
	} else {
		option = vrVisrenOpts;
		while (option->option_name != NULL) {
			if (!strcmp(option->option_name, graphics)) {
				(*option->info_func)(info);
				return;
			}
			option++;
		}
	}
	vrVisrenDefaultInfo(info);
}


/*****************************************************************/
void vrCalcPerspIntermediaries(vrWindowInfo *window)
{
	/* get the L-point (lower-left point in screen coordinates (ie. 2D)) [Deering method] */
	vrPointSet3d(&(window->Lpoint), window->coords_ll[VR_X], window->coords_ll[VR_Y], window->coords_ll[VR_Z]);
	/* now put the L-point into real-world (RW) coordinate system */
	vrPointTransformByMatrix(&(window->Lpoint), &(window->Lpoint), window->rw2w_xform);

	/* get the H-point (upper-right point in screen coords (ie. 2D)) [Deering method] */
	/*   NOTE: all the math operations are because I don't store the upper-right corner in the config */
	/*   WARNING: what if the screen is configured by a 4x4 transform and not by 3 coordinates? */
	/*   or WARNING: what happens if I specify 3 coordinates, and then attempt to rotate that!?  -- maybe that's the issue with the 6-sided DRI CAVE */
	vrPointSet3d(&(window->Hpoint), window->coords_lr[VR_X], window->coords_ul[VR_Y], window->coords_lr[VR_Z] + (window->coords_ul[VR_Z] - window->coords_ll[VR_Z]));
	/* now put the H-point into real-world (RW) coordinate system */
	vrPointTransformByMatrix(&(window->Hpoint), &(window->Hpoint), window->rw2w_xform);

	/* QUESTION: if L-point & H-point are already in world-coords, then why the matrix multiply? */

	/* calculate the intermediate values for the Deering method */
	window->H_minus_L[VR_X] = window->Hpoint.v[VR_X] - window->Lpoint.v[VR_X];	/* view width */
	window->H_minus_L[VR_Y] = window->Hpoint.v[VR_Y] - window->Lpoint.v[VR_Y];	/* view height */
	window->H_plus_L[VR_X]  = window->Hpoint.v[VR_X] + window->Lpoint.v[VR_X];	/* twice X-center */
	window->H_plus_L[VR_Y]  = window->Hpoint.v[VR_Y] + window->Lpoint.v[VR_Y];	/* twice Y-center */
	window->HminusL_recip[VR_X] = 1.0 / window->H_minus_L[VR_X];
	window->HminusL_recip[VR_Y] = 1.0 / window->H_minus_L[VR_Y];
}

#ifdef USE_FRUSTUMEYE /* { */

/*************************************************************************/
/* vrCalcPerspFrustumEye() fills in the data of the given (via pointer)  */
/*   perspective-data instance based on the given window and RW position */
/*   of the eye.                                                         */
vrPerspData *vrCalcPerspFrustumEye(vrPerspData *pd, vrWindowInfo *this_window, vrMatrix *eye_rwpos)
{
	vrPoint		eye2window_coords = { { 0.0, 0.0, 0.0 } };	/* eye location in window coords */
	double		scale_factor;
	double		near_clip;
	double		far_clip;

	/*   NOTE: for user-based clipping planes, we will need to pass a   */
	/*   pointer to the complete eye object rather than just eye_rwpos. */

	/* calculate the near and far clipping planes, starting with the */
	/*   window and inheriting "default" (ie. -1) values from user,  */
	/*   system and general configuration values, and if the config  */
	/*   defaults are bad, set to some reasonable values.            */
	near_clip = this_window->settings.near_clip;
	if (near_clip < 0)
		near_clip = this_window->settings.near_clip;
#if 0 /* TODO: need a pointer to the eye structure */
	if (near_clip < 0)
		near_clip = this_user->settings.near_clip;
#endif
	if (near_clip < 0)
		near_clip = this_window->context->config->system->settings.near_clip;
	if (near_clip < 0)
		near_clip = this_window->context->config->defaults.near_clip;
	if (near_clip < 0)
		near_clip = 0.1;

	far_clip = this_window->settings.far_clip;
	if (far_clip < 0)
		far_clip = this_window->settings.far_clip;
#if 0 /* TODO: need a pointer to the eye structure */
	if (far_clip < 0)
		far_clip = this_user->settings.far_clip;
#endif
	if (far_clip < 0)
		far_clip = this_window->context->config->system->settings.far_clip;
	if (far_clip < 0)
		far_clip = this_window->context->config->defaults.far_clip;
	if (far_clip < 0)
		far_clip = 1000.0;

	switch (this_window->mount) {

	/**********************/
	case VRWINDOW_SIMULATOR:
	/**********************/

		/* set frustum values for simulator view */
		pd->frustum.n.near_clip  =  near_clip;
		pd->frustum.n.far_clip   =  far_clip;
		pd->frustum.n.left       = -near_clip;
		pd->frustum.n.right      =  near_clip;
		pd->frustum.n.bottom     = -near_clip;
		pd->frustum.n.top        =  near_clip;

		/* also return the Eye position in window coordinates */
		pd->eye[VR_X] = 0.0;	/* centered laterally (left/right) */
		pd->eye[VR_Y] = 5.0;	/* height at 5 feet */
		pd->eye[VR_Z] = 6.0;	/* one foot behind CAVE */

		/* copy some of the window data */
		pd->mount = this_window->mount;
		pd->rw2w_xform = *(this_window->rw2w_xform);

		return pd;


	/******************/
	case VRWINDOW_FIXED:
	/******************/

		/*******************************************************/
		/* Create an off-axis perspective projection matrix    */
		/*                                                     */
		/* There are a few ways to do this.                    */
		/*   NOTE: I'm not sure where I got this "Frustum-Eye" method from */
		/*                                                     */


		/* set eye2window_coords to now have the eye location in window coordinates */
		vrPointTransformByMatrix(&eye2window_coords, vrPointGetTransFromMatrix(&eye2window_coords, eye_rwpos), this_window->rw2w_xform);

		/* eye2window_coords = eye position in window CS */
		/*   window coordinates are:         */
		/*     lower left  = (L[VR_X], L[VR_Y], 0) */
		/*     upper right = (H[VR_X], H[VR_Y], 0) */

		/* Some notes:                                             */
		/*     relative to eye's position (what glFrustum wants):  */
		/*	left        = L[VR_X] - eye2window_coords.v[VR_X] */
		/*	right       = H[VR_X] - eye2window_coords.v[VR_X] */
		/*	bottom      = L[VR_Y] - eye2window_coords.v[VR_Y] */
		/*	top         = H[VR_Y] - eye2window_coords.v[VR_Y] */
		/*	near_clip   = near_clip     */
		/*	far_clip    = far_clip      */

		/*       lower left corner of window  = (L[VR_X] - eye2window_coords[VR_X], L[VR_Y] - eye2window_coords[VR_Y], -eye2window_coords[VR_Z]) */
		/*       upper right corner of window = (H[VR_X] - eye2window_coords[VR_X], H[VR_Y] - eye2window_coords[VR_Y], -eye2window_coords[VR_Z]) */

		/* NOTE: because glFrustum's coordinates are given in terms of the  */
		/*   near plane, we multiply the side planes (l,r,b,t) by the value */
		/*   of the near clipping plane.  This is then made a component of  */
		/*   an overall scale_factor which also takes into account the      */
		/*   distance to the eye (eye2window_coords.v[VR_Z).                */
		scale_factor             = near_clip / eye2window_coords.v[VR_Z];
		pd->frustum.n.near_clip  = near_clip;
		pd->frustum.n.far_clip   = far_clip;
		pd->frustum.n.left       = (this_window->Lpoint.v[VR_X] - eye2window_coords.v[VR_X]) * scale_factor;
		pd->frustum.n.right      = (this_window->Hpoint.v[VR_X] - eye2window_coords.v[VR_X]) * scale_factor;
		pd->frustum.n.bottom     = (this_window->Lpoint.v[VR_Y] - eye2window_coords.v[VR_Y]) * scale_factor;
		pd->frustum.n.top        = (this_window->Hpoint.v[VR_Y] - eye2window_coords.v[VR_Y]) * scale_factor;

#if 0 /* optional code -- seems to work okay with OpenGL, but not Performer */
	/* TODO: 2/24/2003 -- now that the near clip is used above, just reset the local */
	/*   copy of the near clipping plane to create this effect.                      */
		/* when this code included, near clip will never go outside window */
		/* 	Eye(z) = distance to window */
		if (pd->frustum.n.near_clip > eye2window_coords.v[VR_Z]) {
			pd->frustum.n.near_clip = eye2window_coords.v[VR_Z] - 0.0001;	/* a small epsilon to avoid flicker of polygons on plane of the window */

			/* adjust the edges of the frustum for new near clip value */
			pd->frustum.n.left   *= eye2window_coords.v[VR_Z];
			pd->frustum.n.right  *= eye2window_coords.v[VR_Z];
			pd->frustum.n.bottom *= eye2window_coords.v[VR_Z];
			pd->frustum.n.top    *= eye2window_coords.v[VR_Z];
			/* TODO: determine whether we should likewise factor the far clip */
		}
#endif

		/* also return the Eye position in window coordinates */
		pd->eye[VR_X] = eye2window_coords.v[VR_X];
		pd->eye[VR_Y] = eye2window_coords.v[VR_Y];
		pd->eye[VR_Z] = eye2window_coords.v[VR_Z];

		/* copy some of the window data */
		pd->mount = this_window->mount;
		pd->rw2w_xform = *(this_window->rw2w_xform);

		return pd;


	/**********************/
	case VRWINDOW_HEADMOUNT: { /* TODO: this block bracket may no longer be necessary */
	/**********************/
#ifdef NOT_CURRENTLY_USED
		vrMatrix	inverted_eye_rwpos;
		vrMatrix	fixed_persp;
#endif

#if 1 /* use new HBD code { */
		/*** Code for HBD case provided by Dave Edwards, 5/6/2002 ***/

		/* TODO: hmmm, I don't see where Dave uses this value */
		/* Calculate the real-world to window matrix */
		vrMatrixInvert(this_window->rw2w_xform, eye_rwpos);

#  if 0
		/* This is currently unnecessary because window limits are */
		/* given in eye coordinates.                               */

		/* Set eye2window_coords to be the eye location in window coords */
		vrPointTransformByMatrix(&eye2window_coords, &eye2window_coords, this_window->e2w_xform);
#  endif

		/* Set the frustum values based on the eye-window relationship */
		scale_factor             = near_clip / -this_window->coords_ll[VR_Z];
		pd->frustum.n.near_clip  = near_clip;
		pd->frustum.n.far_clip   = far_clip;
		pd->frustum.n.left       = this_window->Lpoint.v[VR_X] * scale_factor;
		pd->frustum.n.right      = this_window->Hpoint.v[VR_X] * scale_factor;
		pd->frustum.n.bottom     = this_window->Lpoint.v[VR_Y] * scale_factor;
		pd->frustum.n.top        = this_window->Hpoint.v[VR_Y] * scale_factor;

		/* Set the eye position in window coordinates */
		pd->eye[VR_X] = eye2window_coords.v[VR_X];
		pd->eye[VR_Y] = eye2window_coords.v[VR_Y];
		pd->eye[VR_Z] = eye2window_coords.v[VR_Z];

#else /* } { use placeholder simulator view */
		/* set frustum values for simulator view */
		pd->frustum.n.near_clip  =  near_clip;
		pd->frustum.n.far_clip   =  far_clip;
		pd->frustum.n.left       = -near_clip;
		pd->frustum.n.right      =  near_clip;
		pd->frustum.n.bottom     = -near_clip;
		pd->frustum.n.top        =  near_clip;

		/* also return the Eye position in window coordinates */
		pd->eye[VR_X] = 0.0;	/* centered laterally (left/right) */
		pd->eye[VR_Y] = 5.0;	/* height at 5 feet */
		pd->eye[VR_Z] = 6.0;	/* one foot behind CAVE */
#endif /* } */

		/* copy some of the window data */
		pd->mount = this_window->mount;
		pd->rw2w_xform = *(this_window->rw2w_xform);

		return pd;
	}

	/*********************/
	case VRWINDOW_HANDHELD:
	/*********************/
		vrErrPrintf("vrCalcPerspMatrix(): " RED_TEXT "Window type '%s' not yet implemented\n" NORM_TEXT, vrWindowName(this_window->mount));

#if 1 /* TODO: temporarily give a simulator view (to save Performer from getting out of whack) */
		/* set frustum values for simulator view */
		pd->frustum.n.near_clip   =  near_clip;
		pd->frustum.n.far_clip    =  far_clip;
		pd->frustum.n.left        = -near_clip;
		pd->frustum.n.right       =  near_clip;
		pd->frustum.n.bottom      = -near_clip;
		pd->frustum.n.top         =  near_clip;

		/* also return the Eye position in window coordinates */
		pd->eye[VR_X] = 0.0;	/* centered laterally (left/right) */
		pd->eye[VR_Y] = 5.0;	/* height at 5 feet */
		pd->eye[VR_Z] = 6.0;	/* one foot behind CAVE */

		/* copy some of the window data */
		pd->mount = this_window->mount;
		pd->rw2w_xform = *(this_window->rw2w_xform);

#endif

		return pd;

	/******/
	default:
		vrErrPrintf("vrCalcPerspMatrix(): " RED_TEXT "Unknown window type %d -- this should not happen\n" NORM_TEXT, this_window->mount);
		return pd;
	}
}

#else /* } !USE_FRUSTUMEYE { */


/*****************************************************************/
vrPerspData *vrCalcPerspMatrix(vrPerspData *pd, vrWindowInfo *this_window, vrMatrix *eye_rwpos)
{
	double		B;				/* back(far) clipping plane (rel to eye) */
	double		F;				/* front(near) clipping plane (ditto) */
	double		BminusF_recip;			/* an intermediate value */
#if 0 /* L & H are now determined by corner points, rest are stored in window structure */
	double		L[2], H[2];			/* LL & UR view coords in window CS */
	double		H_minus_L[2], H_plus_L[2];	/* intermediate values */
	double		HminusL_recip[2];		/* more intermediate values */
	vrPoint		Lpoint, Hpoint;
#endif
	double		near_clip;
	double		far_clip;
	vrPoint		eye2window_coords = { 0.0, 0.0, 0.0 };		/* eye location in window coords */
	vrMatrix	*persp = &pd->mat;

	vrMatrixSetIdentity(persp);

	switch (this_window->mount) {

	/**********************/
	case VRWINDOW_SIMULATOR:
	/**********************/

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
/* TODO: need to verify these matrices, since the discovery that */
/*   VRMAT_ROWCOL() may actually be doing VR_COLROW().           */
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

#if 1
		/* Based on the modified Angel(202) from below with */
		/*   the extra operations built right in.  (with    */
		/*   the exception of the post-translate).          */
		/*   1.000   0.000   0.000   0.000  */
		/*   0.000   1.000   0.000   0.000  */
		/*   0.000   0.000  -1.000   0.000  */
		/*   0.000   0.000  -1.000   1.000  */
#  if 0 /* same as identity matrix */
		VRMAT_ROWCOL(persp, 0, 0) =  1.0f;
		VRMAT_ROWCOL(persp, 1, 1) =  1.0f;
		VRMAT_ROWCOL(persp, 3, 3) =  1.0f;
#  endif
		VRMAT_ROWCOL(persp, 2, 2) = -1.0f;
		VRMAT_ROWCOL(persp, 3, 2) = -1.0f;

		vrMatrixPostTranslate3d(persp,  0.0, -4.0,  -6.0);
#elif 1
		/* modified from Angel p. 202 */
		VRMAT_ROWCOL(persp, 0, 0) =  1.0f;
		VRMAT_ROWCOL(persp, 1, 1) =  1.0f;
		VRMAT_ROWCOL(persp, 2, 2) =  1.0f;
		VRMAT_ROWCOL(persp, 3, 2) = -1.0f;
		/* At this point, the matrix is:    */
		/*   1.000   0.000   0.000   0.000  */
		/*   0.000   1.000   0.000   0.000  */
		/*   0.000   0.000   1.000   0.000  */
		/*   0.000   0.000  -1.000   0.000  */

		/* further modifications that seem to make this work pretty well */
		/* esp. if rw2w_xform = (0,0,0). */
		vrMatrixPreRotateId(persp, VR_Y, 180.0f);
		/* At this point, the matrix is:    */
		/*  -1.000   0.000   0.000   0.000  */
		/*   0.000   1.000   0.000   0.000  */
		/*   0.000   0.000  -1.000   0.000  */
		/*   0.000   0.000  -1.000   0.000  */

		vrPreScaleMatrix(persp, -1.0, 1.0, 1.0);
		/* At this point, the matrix is:    */
		/*   1.000   0.000   0.000   0.000  */
		/*   0.000   1.000   0.000   0.000  */
		/*   0.000   0.000  -1.000   0.000  */
		/*   0.000   0.000  -1.000   0.000  */

		/* The invert matrix only has the effect of putting      */
		/*   a 1.0 in the 3,3 location.  Without this, objects   */
		/*   seem to be rendered in the proper location, but     */
		/*   exhibit side effects similar to co-planar polygons. */
		vrMatrixInvert(persp);
		/* At this point, the matrix is:    */
		/*   1.000   0.000   0.000   0.000  */
		/*   0.000   1.000   0.000   0.000  */
		/*   0.000   0.000  -1.000   0.000  */
		/*   0.000   0.000  -1.000   1.000  */

#  if 0
		/* a pre translation on Z affects the perspective plane. */
		vrMatrixPreTranslate3d(persp, -0.5, 0.0,  0.1);
#  else
		/* a post translation works to position the view (center of projection?) */
		vrMatrixPostTranslate3d(persp,  0.0, -4.0,  -6.0);
		/* At this point, the matrix is:    */
		/*   1.000   0.000   0.000   0.000  */
		/*   0.000   1.000   0.000  -4.000  */
		/*   0.000   0.000  -1.000   6.000  */
		/*   0.000   0.000  -1.000   7.000  */

#  endif
#elif 1
		/* from Angel p. 202 */
		VRMAT_ROWCOL(persp, 0, 0) =  1.0f;
		VRMAT_ROWCOL(persp, 1, 1) =  1.0f;
		VRMAT_ROWCOL(persp, 2, 2) = -1.0f;
		VRMAT_ROWCOL(persp, 3, 2) =  1.0f;
#if 0 /* TODO: 05/25/2006 -- it seems like we can get rid of this small section */
#  if 1
		vrMatrixPostTranslate3d(persp, 0.0, 0.0, -0.0);
#  else
		/* this is the 0.1c version */
		vrMatrixPostTranslate3d(persp, 4.0, 2.0, -5.0);
#  endif
#endif
#  if 1
		/* this seems more correct, but less interesting visually */
		vrMatrixInvert(persp);
#  endif
#  if 0
		vrMatrixPostTranslate3d(persp, 0.0, 0.0, 5.0);
#  endif

#elif 0
		/* from Angel p. 203 */
		VRMAT_ROWCOL(persp, 0, 0) =  1.0f;
		VRMAT_ROWCOL(persp, 1, 1) =  1.0f;
		VRMAT_ROWCOL(persp, 3, 2) = -1.0f;
#elif 0
		/* from Angel p. 203 */
		VRMAT_ROWCOL(persp, 0, 0) =  1.0f;
		VRMAT_ROWCOL(persp, 1, 1) =  1.0f;
		VRMAT_ROWCOL(persp, 3, 2) = -1.0f;
		VRMAT_ROWCOL(persp, 2, 2) = -2.0f;
		VRMAT_ROWCOL(persp, 2, 3) = -2.0f;
#elif 0
		/* a simple ortho perspective */
		/* from Angel p. 188 */
		VRMAT_ROWCOL(persp, 0, 0) = 1.0f;
		VRMAT_ROWCOL(persp, 1, 1) = 1.0f;
		VRMAT_ROWCOL(persp, 3, 3) = 1.0f;
#else
		/* attempted perspective matrix from Hearn & Baker pp 444-5 */

		/* a generic viewer centered perspective projection transform */
		/* (i.e. viewer views the scene directly along the window's Z */
		/*  axis.)                                                    */
		double	Z_viewplane =  1.0;
		double	Z_viewpoint =  1.50;
		double	D_reciprocal = 1.0f / (Z_viewpoint - Z_viewplane);
		VRMAT_ROWCOL(persp, 0, 0) = 1.0f;
		VRMAT_ROWCOL(persp, 1, 1) = 1.0f;
		VRMAT_ROWCOL(persp, 2, 2) = -Z_viewplane * D_reciprocal;
		VRMAT_ROWCOL(persp, 3, 3) =  Z_viewpoint * D_reciprocal;
		VRMAT_ROWCOL(persp, 2, 3) =  Z_viewplane * Z_viewpoint * D_reciprocal;
		VRMAT_ROWCOL(persp, 3, 2) = -D_reciprocal;
#endif

#ifdef VERBOSE
		vrPrintMatrix("simulator persp = ", persp);
#endif

		return pd;


	/******************/
	case VRWINDOW_FIXED:
	/******************/

		/*******************************************************/
		/* Create an off-axis perspective projection matrix    */
		/*                                                     */
		/* There are a few ways to do this.  Three are:        */
		/*   The method described in Deering's SG '92 paper    */
		/*   The method described in Cruz-Neira's SG '93 paper */
		/*   The method described in Angel pp 205-207.         */
		/* We are currently using the Deering method, but the  */
		/*   Angel method also looks promising.                */
		/*                                                     */


#if 0 /* 9/27/2000 -- replace with values from coords fields */
		/* LL ('L') and UR ('H') window coords in window CS (and helpers) */

		/* TODO: this appears to be hardcoded to assume all windows are */
		/*   10 foot by 10 foot squares.  I don't think that's right.   */
		L[VR_X] = 0.0f;	/* left edge of window */
		L[VR_Y] = 0.0f;	/* lower edge of window */
		H[VR_X] = 10.0f;	/* right edge of window */
		H[VR_Y] = 10.0f;	/* upper edge of window */

		H_minus_L[VR_X] = H[VR_X] - L[VR_X];	/* view width */
		H_minus_L[VR_Y] = H[VR_Y] - L[VR_Y];	/* view height */
		H_plus_L[VR_X] = H[VR_X] + L[VR_X];	/* twice X-center */
		H_plus_L[VR_Y] = H[VR_Y] + L[VR_Y];	/* twice Y-center */
#else

#  if 0 /* 6/21/2001: moved to window structure -- caluclated in vrCalcPerspIntermediaries() */
		/* TODO: the Lpoint and Hpoint values should just be calculated */
		/*   once and then stored in the window structure.  Along with  */
		/*   the minus/plus and reciprical values while we're at it!    */
		vrPointSet3d(&Lpoint, this_window->coords_ll[VR_X], this_window->coords_ll[VR_Y], this_window->coords_ll[VR_Z]);
		vrPointTransformByMatrix(&Lpoint, &Lpoint, this_window->rw2w_xform);
		vrPointSet3d(&Hpoint, this_window->coords_lr[VR_X], this_window->coords_ul[VR_Y], this_window->coords_lr[VR_Z] + (this_window->coords_ul[VR_Z] - this_window->coords_ll[VR_Z]));
		vrPointTransformByMatrix(&Hpoint, &Hpoint, this_window->rw2w_xform);
		H_minus_L[VR_X] = Hpoint.v[VR_X] - Lpoint.v[VR_X];	/* view width */
		H_minus_L[VR_Y] = Hpoint.v[VR_Y] - Lpoint.v[VR_Y];	/* view height */
		H_plus_L[VR_X]  = Hpoint.v[VR_X] + Lpoint.v[VR_X];	/* twice X-center */
		H_plus_L[VR_Y]  = Hpoint.v[VR_Y] + Lpoint.v[VR_Y];	/* twice Y-center */
		HminusL_recip[VR_X] = 1.0 / H_minus_L[VR_X];
		HminusL_recip[VR_Y] = 1.0 / H_minus_L[VR_Y];
#  endif

#endif

		/* set eye2window_coords to now have the eye location in window coordinates */
		vrPointTransformByMatrix(&eye2window_coords, vrPointGetTransFromMatrix(&eye2window_coords, eye_rwpos), this_window->rw2w_xform);

		/* eye2window_coords = eye position in window CS */
		/*   window coordinates are:         */
		/*     lower left  = (L[VR_X], L[VR_Y], 0) */
		/*     upper right = (H[VR_X], H[VR_Y], 0) */

		/* Some notes:                                             */
		/*     relative to eye's position (what glFrustum wants):  */
		/*	right       = H[VR_X] - eye2window_coords.v[VR_X]  */
		/*	left        = L[VR_X] - eye2window_coords.v[VR_X]  */
		/*	top         = H[VR_Y] - eye2window_coords.v[VR_Y]  */
		/*	bottom      = L[VR_Y] - eye2window_coords.v[VR_Y]  */
		/*	near_clip   = eye2window_coords.v[VR_Z] - near_clip -- or perhaps: near_clip - eye2window_coords.v[VR_Z] (though disagrees w/ below) */
		/*	far_clip    = eye2window_coords.v[VR_Z] - far_clip -- or perhaps: far_clip - eye2window_coords.v[VR_Z]  (though disagrees w/ below) */

		/*       lower left  = (L[VR_X] - eye2window_coords[VR_X], L[VR_Y] - eye2window_coords[VR_Y], -eye2window_coords[VR_Z]) */
		/*       upper right = (H[VR_X] - eye2window_coords[VR_X], H[VR_Y] - eye2window_coords[VR_Y], -eye2window_coords[VR_Z]) */

		/* set the near and far clipping planes */
		F = eye2window_coords.v[VR_Z] - far_clip;	/* according to Deering, s/b near (front)?*/
		B = eye2window_coords.v[VR_Z] - near_clip;	/* according to Deering, s/b far (back)?  */
		BminusF_recip = 1.0 / (B - F);

#define DEERING

#ifdef DEERING /* { */
		/*
		 * Deering's matrix looks pretty complicated, but computing it
		 * is pretty straightforward
		 * in order to get this to work, make sure you set the GL
		 * projection matrix to the identity matrix -- DO NOT call
		 * glFrustum ...
		* BS: 2/5/01 -- actually, this will become the GL projection matrix
		 *
		 */

		/* from "man glfrustum" 0,0 = 2*near / (right-left) */
		VRMAT_ROWCOL(persp, 0, 0) = (2.0f*eye2window_coords.v[VR_Z]) * this_window->HminusL_recip[VR_X];

		/* from "man glfrustum" 0,2 (A) = (right+left) / (right-left) */
		VRMAT_ROWCOL(persp, 0, 2) = (this_window->H_plus_L[VR_X] - 2.0f*eye2window_coords.v[VR_X]) * this_window->HminusL_recip[VR_X];

		/* from "man glfrustum" 0,3 = 0  ??? */
		VRMAT_ROWCOL(persp, 0, 3) = -eye2window_coords.v[VR_Z] * (this_window->H_plus_L[VR_X] * this_window->HminusL_recip[VR_X]);

		/* from "man glfrustum" 1,1 = 2*near / (top-bottom) */
		VRMAT_ROWCOL(persp, 1, 1) = (2.0f*eye2window_coords.v[VR_Z]) * this_window->HminusL_recip[VR_Y];

		/* from "man glfrustum" 1,2 (B) = (top+bottom) / (right-left) */
		VRMAT_ROWCOL(persp, 1, 2) = (this_window->H_plus_L[VR_Y] - 2.0f*eye2window_coords.v[VR_Y]) * this_window->HminusL_recip[VR_Y];

		/* from "man glfrustum" 1,3 = 0  ??? */
		VRMAT_ROWCOL(persp, 1, 3) = -eye2window_coords.v[VR_Z] * (this_window->H_plus_L[VR_Y] * this_window->HminusL_recip[VR_Y]);

		/* from "man glfrustum" 2,2 (C) = - (far+near) / (far-near) */
		VRMAT_ROWCOL(persp, 2, 2) = (B + F - 2.0f*eye2window_coords.v[VR_Z]) * BminusF_recip;

		/* from "man glfrustum" 2,3 (D) = -2 * far * near / (far - near) */
		VRMAT_ROWCOL(persp, 2, 3) = B - eye2window_coords.v[VR_Z] - B * (B + F - 2.0f*eye2window_coords.v[VR_Z]) * BminusF_recip;

		/* from "man glfrustum" 3,2 = -1 */
		VRMAT_ROWCOL(persp, 3, 2) = -1.0f;

		/* from "man glfrustum" 3,3 = 0  ??? */
		VRMAT_ROWCOL(persp, 3, 3) = eye2window_coords.v[VR_Z];


#ifdef VERBOSE
		vrPrintMatrix("Deering persp = ", persp);
#endif
#endif /* } DEERING */

#ifdef CRUZNEIRA /* { */
		/*
		 * Cruz-Neira's calculations are straightforward, but how exactly
		 * they map to different walls is still something of a mystery
		 * (according to the paper, the reader should be able to "easily
		 * derive the matrices for the other walls ...")
		 * they don't really seem to work -- they produce some weird-ass
		 * squashed-flat geometry
		 */

#define PP 5.0f
		VRMAT_ROWCOL(persp, 2, 0) = - eye2window_coords.v[VR_X] / (eye2window_coords.v[VR_Z] - PP);
		VRMAT_ROWCOL(persp, 2, 1) = - eye2window_coords.v[VR_Y] / (eye2window_coords.v[VR_Z] - PP);
		VRMAT_ROWCOL(persp, 2, 3) = - 1.0f / (eye2window_coords.v[VR_Z] - PP);
		VRMAT_ROWCOL(persp, 3, 0) = (eye2window_coords.v[VR_X] * PP) / (eye2window_coords.v[VR_Z] - PP);
		VRMAT_ROWCOL(persp, 3, 1) = (eye2window_coords.v[VR_Y] * PP) / (eye2window_coords.v[VR_Z] - PP);
		VRMAT_ROWCOL(persp, 3, 3) = eye2window_coords.v[VR_Z] / (eye2window_coords.v[VR_Z] - PP);
		vrPrintf("PP = %f\n", PP);
		vrPrintMatrix("Cruz-Neira persp = ", persp);
#endif /* } CRUZNEIRA */


		return pd;


	/**********************/
	case VRWINDOW_HEADMOUNT: {
	/**********************/
		vrMatrix	inverted_eye_rwpos;
		vrMatrix	fixed_persp;

#if 0
		/* First pass, I just copied the deering fixed window method */
		L[VR_X] = -1.0f;	/* left edge of window */
		L[VR_Y] = -1.0f;	/* lower edge of window */
		H[VR_X] =  1.0f;	/* right edge of window */
		H[VR_Y] =  1.0f;	/* upper edge of window */

		H_minus_L[VR_X] = H[VR_X] - L[VR_X];	/* view width */
		H_minus_L[VR_Y] = H[VR_Y] - L[VR_Y];	/* view height */
		H_plus_L[VR_X] = H[VR_X] + L[VR_X];	/* twice X-center */
		H_plus_L[VR_Y] = H[VR_Y] + L[VR_Y];	/* twice Y-center */
#else
#  if 0 /* 6/21/2001: moved to window structure -- caluclated in vrCalcPerspIntermediaries() */
   {
		vrMatrix	rw2w_xform;
		/* TODO: the Lpoint and Hpoint values should just be calculated */
		/*   once and then stored in the window structure.  Along with  */
		/*   the minus/plus and reciprical values while we're at it!    */
		vrPointSet3d(&Lpoint, this_window->coords_ll[VR_X], this_window->coords_ll[VR_Y], this_window->coords_ll[VR_Z]);
		vrPointTransformByMatrix(&Lpoint, &Lpoint, this_window->rw2w_xform);
		vrPointSet3d(&Hpoint, this_window->coords_lr[VR_X], this_window->coords_ul[VR_Y], this_window->coords_lr[VR_Z] + (this_window->coords_ul[VR_Z] - this_window->coords_ll[VR_Z]));
		vrPointTransformByMatrix(&Hpoint, &Hpoint, this_window->rw2w_xform);
		H_minus_L[VR_X] = Hpoint.v[VR_X] - Lpoint.v[VR_X];	/* view width */
		H_minus_L[VR_Y] = Hpoint.v[VR_Y] - Lpoint.v[VR_Y];	/* view height */
		H_plus_L[VR_X]  = Hpoint.v[VR_X] + Lpoint.v[VR_X];	/* twice X-center */
		H_plus_L[VR_Y]  = Hpoint.v[VR_Y] + Lpoint.v[VR_Y];	/* twice Y-center */
   }
#  endif
#endif

		vrMatrixInvert(&inverted_eye_rwpos, eye_rwpos);

#if 0 /* { YO */
		/* calculate real-world to window transformation */
		vrMatrixCopy(&rw2w_xform, this_window->rw2w_xform);
#  define XFORM 1
#  if XFORM == 1
		/* rotations seem nearing correctness, but translations seem backward */
		vrMatrixPreMult(&rw2w_xform, eye_rwpos);
#  elif XFORM == 2
		/* translations seem to do nothing, and rotations are wierd */
		vrMatrixPreMult(&rw2w_xform, &inverted_eye_rwpos);
#  elif XFORM == 3
		/* objects move about, but some never leave view, even when 360 spinning */
		/* with clips reversed, objects never move, in any of the 4 window tests */
		vrMatrixPostMult(&rw2w_xform, eye_rwpos);
#  elif XFORM == 4
		/* doesn't seem to produce any visual changes (with clips same or reversed, in any of the first 4 window tests) */
		vrMatrixPostMult(&rw2w_xform, &inverted_eye_rwpos);
#  endif
#endif /* } YO */

#if 0 /* 9/27/2000: doesn't work with this code */
		/* TODO: not sure we need this for head-based displays */
		/*   9/20/2000, the reason this doesn't seem necessary is that the */
		/*     position of each eye relative to its window should remain   */
		/*     fixed in a head-based display.  However, the reason nothing */
		/*     seems to even begin to work (ie. the scene never changes)   */
		/*     is because eye2window_coords (E) is the only parameter in   */
		/*     the Deering formula below that gets changed.  So if 'E'     */
		/*     never changes, then the view will never change.  I guess    */
		/*     I'll have to relook at Deering's paper.                     */
		/* make eye2window_coords = eye position in window CS */
		vrPointTransformByMatrix(&eye2window_coords, vrPointGetTransFromMatrix(&eye2window_coords, eye_rwpos), &rw2w_xform);

#else
		/* this moves the eye away from the window */
		/* TODO: this should be based on the coords/rw2w_xform parameters */
		eye2window_coords.v[VR_X] =  0.0;
		eye2window_coords.v[VR_Y] =  0.0;
		eye2window_coords.v[VR_Z] =  0.5;

#  if 0 /* 9/27/2000 -- this doesn't work */
		/* The eye really should be based on the window coords values, so try: */
		eye2window_coords.v[VR_Z] = -(this_window->Lpoint.v[VR_Z] + this_window->Hpoint.v[VR_Z]) * 0.5;
vrPrintf("this_window->Lpoint.v[VR_Z] = %lf, this_window->Hpoint.v[VR_Z] = %lf, eye2window_coords.v[VR_Z] = %lf\n", this_window->Lpoint.v[VR_Z], this_window->Hpoint.v[VR_Z], eye2window_coords.v[VR_Z]);
#  endif
#endif

		/* set the near and far clipping planes */
#if 1
		/* code from stationary windows */
		F = eye2window_coords.v[VR_Z] - far_clip;
		B = eye2window_coords.v[VR_Z] - near_clip;
#else
		/* reversed code */
		/* This does not seem to work in any circumstances -- always */
		/*   makes objects inside-out.                               */
		B = eye2window_coords.v[VR_Z] - far_clip;
		F = eye2window_coords.v[VR_Z] - near_clip;
#endif
		BminusF_recip = 1.0 / (B - F);

		vrMatrixSetIdentity(&fixed_persp);

		VRMAT_ROWCOL(&fixed_persp, 0, 0) = (2.0f*eye2window_coords.v[VR_Z]) * (this_window->HminusL_recip[VR_X]);
		VRMAT_ROWCOL(&fixed_persp, 0, 2) = (this_window->H_plus_L[VR_X] - 2.0f*eye2window_coords.v[VR_X]) * this_window->HminusL_recip[VR_X];
		VRMAT_ROWCOL(&fixed_persp, 0, 3) = -eye2window_coords.v[VR_Z] * (this_window->H_plus_L[VR_X] * this_window->HminusL_recip[VR_X]);
		VRMAT_ROWCOL(&fixed_persp, 1, 1) = (2.0f*eye2window_coords.v[VR_Z]) * this_window->HminusL_recip[VR_Y];
		VRMAT_ROWCOL(&fixed_persp, 1, 2) = (this_window->H_plus_L[VR_Y] - 2.0f*eye2window_coords.v[VR_Y]) * this_window->HminusL_recip[VR_Y];
		VRMAT_ROWCOL(&fixed_persp, 1, 3) = -eye2window_coords.v[VR_Z] * (this_window->H_plus_L[VR_Y] * this_window->HminusL_recip[VR_Y]);
		VRMAT_ROWCOL(&fixed_persp, 2, 2) = (B + F - 2.0f*eye2window_coords.v[VR_Z]) * BminusF_recip;
		VRMAT_ROWCOL(&fixed_persp, 2, 3) = B - eye2window_coords.v[VR_Z] - B * (B + F - 2.0f*eye2window_coords.v[VR_Z]) * BminusF_recip;
		VRMAT_ROWCOL(&fixed_persp, 3, 2) = -1.0f;
		VRMAT_ROWCOL(&fixed_persp, 3, 3) = eye2window_coords.v[VR_Z];

#define MULT 3
	/* unless otherwise specified, trailing comments on each test */
	/*   are based on skipping all the XFORM tests, setting a fixed */
	/*   Eye position in Window space to (0,0,0.5), and using the   */
	/*   same clipping plane values as for the fixed window.        */
#if MULT == 1
		vrMatrixProduct(persp, &inverted_eye_rwpos, &fixed_persp);	/* objects squish back and forth */
#elif MULT == 2
		vrMatrixProduct(persp, &fixed_persp, eye_rwpos);		/* seems reversed, but close */
#elif MULT == 3
		vrMatrixProduct(persp, &fixed_persp, &inverted_eye_rwpos);	/* hey, this seems to work! */
#elif MULT == 4
		vrMatrixProduct(persp, eye_rwpos, &fixed_persp);		/* weird orbit effect */
#endif

		return pd;
	}

	/*********************/
	case VRWINDOW_HANDHELD:
	/*********************/
		vrErrPrintf("vrCalcPerspMatrix(): Window type '%s' not yet implemented\n", vrWindowName(this_window->mount));
		return pd;

	/******/
	default:
		vrErrPrintf("vrCalcPerspMatrix(): Unknown window type %d -- this should not happen\n", this_window->mount);
		return pd;
	}
}

#endif /* } USE_FRUSTUMEYE */


/****************************************************************************/
/* NOTE: if args == NULL then use the defaults */
/* TODO: for now, non-default usage has not been implemented (i.e. "args" is ignored) */
void vrSimulatorMove(vrWindowInfo *window, int command, void *args)
{
	vrPoint		volume_center;			/* used to rotate about center */
	vrUserInfo	*user;				/* used when movement information is based on a user */

#if 0
	/* TODO: there currently isn't a working_volume field for */
	/*   windows. in the future, we will add one that will be */
	/*   used for simulator rendering.                        */

	volume_center.v[VR_X] = (window->working_volume_max[VR_X] - window->working_volume_min[VR_X]) / 2.0;
	volume_center.v[VR_Y] = (window->working_volume_max[VR_Y] - window->working_volume_min[VR_Y]) / 2.0;
	volume_center.v[VR_Z] = (window->working_volume_max[VR_Z] - window->working_volume_min[VR_Z]) / 2.0;
#else
	/* TODO: for now we'll set the floor-center for the default CAVE */
	volume_center.v[VR_X] = 0.0;
	volume_center.v[VR_Y] = 0.0;
	volume_center.v[VR_Z] = 5.0;
#endif

	switch (command) {

	case VR_SIMMOVE_AWAY:		/* move away from origin */
		vrMatrixPreTranslate3d(window->rw2w_xform,  0.0, 0.0, -0.5);
		break;

	case VR_SIMMOVE_TOWARD:		/* move toward origin */
		vrMatrixPreTranslate3d(window->rw2w_xform,  0.0, 0.0,  0.5);
		break;

	case VR_SIMMOVE_CENTER:		/* reset to center position */
		vrMatrixSetIdentity(window->rw2w_xform);
		break;

	case VR_SIMMOVE_SETHOME:	/* set the home position */
		memcpy(window->rw2w_homexform, window->rw2w_xform, sizeof(*window->rw2w_homexform));
		break;

	case VR_SIMMOVE_GOHOME:		/* jump to the home position */
		memcpy(window->rw2w_xform, window->rw2w_homexform, sizeof(*window->rw2w_xform));
		break;

	case VR_SIMMOVE_GOHEAD:		/* jump to the user's head position */
#if 0 /* TODO: finish implementing this feature -- or get rid of it if it is not desired. */
		user = window->context->config-> ...
		memcpy(window->rw2w_xform, window->rw2w_homexform, sizeof(*window->rw2w_xform));
#endif
		break;

/* TODO: using pre-rotation is only a so-so solution,        */
/*   really what we want is a pre-rotate-about function      */
/*   that takes a point as an argument, with the point       */
/*   here being the inverse of the current rw2w translation  */
/*   with a slight offset to rotate about the center of the  */
/*   "workspace" rather than the center of the floor.        */
/* NOTE: that using pre-rot makes the rotations occur about  */
/*   the axes as defined by window space, whereas post-rots  */
/*   make the rotations in world space.                      */
/* [1/16/02] playing with cavevars, it seems that the CL is  */
/*   weird in that the Y axis rotation is done in world space*/
/*   and the X axis rotation is done in window space.        */
	case VR_SIMMOVE_POSY:		/* rotate about Y axis */
		vrMatrixPostRotateAboutPointId(window->rw2w_xform, &volume_center, VR_Y,  2.0);
		break;

	case VR_SIMMOVE_NEGY:		/* rotate about Y axis */
		vrMatrixPostRotateAboutPointId(window->rw2w_xform, &volume_center, VR_Y, -2.0);
		break;

	case VR_SIMMOVE_POSX:		/* rotate about X axis */
		vrMatrixPreRotateAboutPointId(window->rw2w_xform, &volume_center, VR_X,  2.0);
		break;

	case VR_SIMMOVE_NEGX:		/* rotate about X axis */
		vrMatrixPreRotateAboutPointId(window->rw2w_xform, &volume_center, VR_X, -2.0);
		break;

	/*******************************/
	/* some non-movement functions */

	case VR_SIMMOVE_TOGGLEMASK:	/* toggle the simulator_mask "flag" */
		window->simulator_mask ^= 1;
		break;
	}
}


/****************************************************************************/
void vrSimulatorMoveString(vrWindowInfo *window, char *strcommand)
{
	int	command = VR_SIMMOVE_NOP;

	if (!strcasecmp(strcommand, "away"))		command = VR_SIMMOVE_AWAY;
	else if (!strcasecmp(strcommand, "toward"))	command = VR_SIMMOVE_TOWARD;
	else if (!strcasecmp(strcommand, "center"))	command = VR_SIMMOVE_CENTER;
	else if (!strcasecmp(strcommand, "sethome"))	command = VR_SIMMOVE_SETHOME;
	else if (!strcasecmp(strcommand, "gohome"))	command = VR_SIMMOVE_GOHOME;
	else if (!strcasecmp(strcommand, "posy"))	command = VR_SIMMOVE_POSY;
	else if (!strcasecmp(strcommand, "negy"))	command = VR_SIMMOVE_NEGY;
	else if (!strcasecmp(strcommand, "posx"))	command = VR_SIMMOVE_POSX;
	else if (!strcasecmp(strcommand, "negx"))	command = VR_SIMMOVE_NEGX;
	else if (!strcasecmp(strcommand, "toggle"))	command = VR_SIMMOVE_TOGGLEMASK;

	vrSimulatorMove(window, command, NULL);
}

