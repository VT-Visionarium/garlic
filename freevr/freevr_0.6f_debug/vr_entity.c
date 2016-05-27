/* ======================================================================
 *
 *  CCCCC          vr_entity.c
 * CC   CC         Author(s): Ed Peters, Bill Sherman, John Stone
 * CC              Created: August 4, 1998 (approx)
 * CC   CC         Last Modified: March 23, 2007
 *  CCCCC
 *
 * Code file for FreeVR input entities (ie. users & props).
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#include <stdio.h>
#include <string.h>

#include "vr_entity.h"
#include "vr_objects.h"
#include "vr_config.h"
#include "vr_input.h"
#include "vr_debug.h"


/************************************************************/
char *vrEyeTypeName(vrEyeType type)
{
	switch (type) {
	case VREYE_DEFAULT:
		return "default";
	case VREYE_LEFT:
		return "left";
	case VREYE_RIGHT:
		return "right";
	case VREYE_CYCLOPS:
		return "cyclops";
	case VREYE_VIBRATE:
		return "vibrate";
	}

	return "unknown";
}


/************************************************************/
vrEyeType vrEyeValue(char *name)
{
	if (!strcasecmp(name, "default"))
		return VREYE_DEFAULT;
	else if (!strcasecmp(name, "left"))
		return VREYE_LEFT;
	else if (!strcasecmp(name, "lefteye"))
		return VREYE_LEFT;
	else if (!strcasecmp(name, "right"))
		return VREYE_RIGHT;
	else if (!strcasecmp(name, "righteye"))
		return VREYE_RIGHT;
	else if (!strcasecmp(name, "cyclops"))
		return VREYE_CYCLOPS;
	else if (!strcasecmp(name, "cyclopseye"))
		return VREYE_CYCLOPS;
	else if (!strcasecmp(name, "mono"))
		return VREYE_CYCLOPS;
	else if (!strcasecmp(name, "monoeye"))
		return VREYE_CYCLOPS;
	else if (!strcasecmp(name, "vibrate"))
		return VREYE_VIBRATE;
	else if (!strcasecmp(name, "vibe"))
		return VREYE_VIBRATE;
	else {
		vrErrPrintf("Unknown eye type '%s' using 'cyclops' type\n", name);
		return VREYE_CYCLOPS;
	}

	/* Can't get to this statement */
	return VREYE_DEFAULT;
}


/************************************************************/
char *vrFrameBufferName(vrFrameBufferType type)
{
	switch (type) {
	case VRFB_FULL:
		return "full";
	case VRFB_LEFT:
		return "left";
	case VRFB_RIGHT:
		return "right";
	case VRFB_FULL_LEFTEYE:
		return "full-left";
	case VRFB_FULL_RIGHTEYE:
		return "full-right";
	}

	return "unknown";
}


/************************************************************/
vrFrameBufferType vrFrameBufferValue(char *name)
{
	if (!strcasecmp(name, "default"))
		return VRFB_DEFAULT;
	else if (!strcasecmp(name, "full"))
		return VRFB_FULL;
	else if (!strcasecmp(name, "fullfb"))
		return VRFB_FULL;
	else if (!strcasecmp(name, "left"))
		return VRFB_LEFT;
	else if (!strcasecmp(name, "leftfb"))
		return VRFB_LEFT;
	else if (!strcasecmp(name, "right"))
		return VRFB_RIGHT;
	else if (!strcasecmp(name, "rightfb"))
		return VRFB_RIGHT;
	else if (!strcasecmp(name, "full-left"))
		return VRFB_FULL_LEFTEYE;
	else if (!strcasecmp(name, "leftvp"))
		return VRFB_FULL_LEFTEYE;
	else if (!strcasecmp(name, "full-right"))
		return VRFB_FULL_RIGHTEYE;
	else if (!strcasecmp(name, "rightvp"))
		return VRFB_FULL_RIGHTEYE;
	else {
		vrErrPrintf("Unknown frame buffer type '%s' using 'full' type\n", name);
		return VRFB_FULL;
	}

	/* Can't get to this statement */
	return VRFB_DEFAULT;
}


/************************************************************/
char *vrAnaglyphTypeName(vrAnaglyphType type)
{
	switch (type) {
	case VRANAGLYPH_DEFAULT:
		return "all";
	case VRANAGLYPH_RED:
		return "red";
	case VRANAGLYPH_GREEN:
		return "green";
	case VRANAGLYPH_BLUE:
		return "blue";
	}

	return "unknown";
}


/************************************************************/
vrAnaglyphType vrAnaglyphValue(char *name)
{
	if (!strcasecmp(name, "default"))
		return VRANAGLYPH_DEFAULT;
	else if (!strcasecmp(name, "all"))
		return VRANAGLYPH_ALL;
	else if (!strcasecmp(name, ""))
		return VRANAGLYPH_ALL;
	else if (!strcasecmp(name, "red"))
		return VRANAGLYPH_RED;
	else if (!strcasecmp(name, "green"))
		return VRANAGLYPH_GREEN;
	else if (!strcasecmp(name, "blue"))
		return VRANAGLYPH_BLUE;
	else {
		vrErrPrintf("Unknown anaglyph type '%s' using 'all colors' type\n", name);
		return VRANAGLYPH_ALL;
	}

	/* Can't get to this statement */
	return VRANAGLYPH_DEFAULT;
}


/************************************************************/
char *vrPrintStyleName(vrPrintStyle style)
{
	switch (style) {
	case def:
		return "default";
	case none:
		return "none";
	case brief:
		return "brief";
	case one_line:
		return "one_line";
	case verbose:
		return "verbose";
	case machine:
		return "machine";
	case file_format:
		return "file_format";
	case everything:
		return "everything";
	}

	return "unknown";
}


/************************************************************/
vrPrintStyle vrPrintStyleValue(char *name)
{
	if (!strcasecmp(name, "default"))
		return def;
	else if (!strcasecmp(name, "def"))
		return def;
	else if (!strcasecmp(name, "none"))
		return none;
	else if (!strcasecmp(name, "brief"))
		return brief;
	else if (!strcasecmp(name, "oneline"))
		return one_line;
	else if (!strcasecmp(name, "one_line"))
		return one_line;
	else if (!strcasecmp(name, "verbose"))
		return verbose;
	else if (!strcasecmp(name, "machine"))
		return machine;
	else if (!strcasecmp(name, "file"))
		return file_format;
	else if (!strcasecmp(name, "fileformat"))
		return file_format;
	else if (!strcasecmp(name, "file_format"))
		return file_format;
	else if (!strcasecmp(name, "everything"))
		return everything;
	else {
		vrErrPrintf("Unknown printstyle type '%s' using 'brief' type\n", name);
		return brief;
	}

	/* Can't get to this statement */
	return brief;
}



/*********************************************************************/
/***********************   Routines for Users   **********************/

/************************************************************/
void vrUserClear(vrUserInfo *object)
{
	object->iod = 0.23;
	object->num_eyes = VREYE_DEFAULT;
	object->color[0] = 0.0;
	object->color[1] = 0.0;
	object->color[2] = 0.0;
	object->num_inputs = 0;
	object->simdata_lock = vrLockCreateName(vrContext, "user object sim data");
	object->visren_lock = vrLockCreateName(vrContext, "user object visren data"); /* this may now be redundant with the new app_lock */
	object->app_lock = vrLockCreateName(vrContext, "user object travel data");

	/* TODO: set a default value for the "head" field here? */

	if (object->rw2vw_xform == NULL)
		object->rw2vw_xform = vrMatrixCreateIdentity();
	else	vrMatrixSetIdentity(object->rw2vw_xform);
	if (object->vw2rw_xform == NULL)
		object->vw2rw_xform = vrMatrixCreateIdentity();
	else	vrMatrixSetIdentity(object->vw2rw_xform);
	if (object->visren_headpos == NULL)
		object->visren_headpos = vrMatrixCreateIdentity();
	else	vrMatrixSetIdentity(object->visren_headpos);
	if (object->visren_rw2vw == NULL)
		object->visren_rw2vw = vrMatrixCreateIdentity();
	else	vrMatrixSetIdentity(object->visren_rw2vw);
	if (object->visren_vw2rw == NULL)
		object->visren_vw2rw = vrMatrixCreateIdentity();
	else	vrMatrixSetIdentity(object->visren_vw2rw);

	vrSettingsClear(&object->settings);
}


/************************************************************/
void vrUserCopy(vrUserInfo *dest_object, vrUserInfo *src_object)
{
	void	*dest_mem;
	void	*src_mem;
	int	memlen;
	int	count;

	/* copy only the memory after the generic vrObjectInfo stuff */
	dest_mem = (char *)dest_object + sizeof(vrObjectInfo);
	src_mem = (char *)src_object + sizeof(vrObjectInfo);
	memlen = sizeof(vrUserInfo) - sizeof(vrObjectInfo);
	memcpy(dest_mem, src_mem, memlen);

	/* make independent copy of some fields */
	dest_object->simdata_lock = vrLockCreateName(vrContext, "user object sim data");
	dest_object->visren_lock = vrLockCreateName(vrContext, "user object visren data");
	dest_object->app_lock = vrLockCreateName(vrContext, "user object travel data");
printf("created sim & visren locks for user (dest) object '%s'\n", dest_object->name);
	dest_object->input_names = (char **)vrShmemAlloc0(src_object->num_inputs * sizeof(char *));
	dest_object->input_data = (vrGenericInput **)vrShmemAlloc0(src_object->num_eyes * sizeof(vrGenericInput *));
	for (count = 0; count < src_object->num_eyes; count++) {
		dest_object->input_names[count] = vrShmemStrDup(src_object->input_names[count]);
		dest_object->input_data[count] = vrShmemMemDup(src_object->input_data[count], sizeof(vrGenericInput *));
	}
	dest_object->head = src_object->head;
	dest_object->rw2vw_xform = vrShmemMemDup(src_object->rw2vw_xform, sizeof(src_object->rw2vw_xform));
	dest_object->vw2rw_xform = vrShmemMemDup(src_object->vw2rw_xform, sizeof(src_object->vw2rw_xform));

	dest_object->visren_headpos = vrShmemMemDup(src_object->visren_headpos, sizeof(src_object->visren_headpos));
	dest_object->visren_rw2vw = vrShmemMemDup(src_object->visren_rw2vw, sizeof(src_object->visren_rw2vw));
	dest_object->visren_vw2rw = vrShmemMemDup(src_object->visren_vw2rw, sizeof(src_object->visren_vw2rw));
}


/************************************************************/
void vrFprintUserInfo(FILE *file, vrUserInfo *userinfo, vrPrintStyle style)
{

	switch (style) {
	case one_line:
	case brief:
		vrFprintf(file, "id = %d, name = '%s', iod = %.3f, color = [ %.3f %.3f %.3f ]\n",
			userinfo->id,
			userinfo->name,
			userinfo->iod,
			userinfo->color[0], userinfo->color[1], userinfo->color[2]);
		break;

	case machine:
		vrFprintf(file, "user:%s:%d:%.3f:%3.1f:%3.1f:%3.1f:%3.1f:%3.1f:%3.1f:%3.1f:%3.1f:%3.1f:%3.1f:%3.1f:%3.1f:%3.1f:%3.1f:%3.1f:%3.1f\n",
			userinfo->name,
			userinfo->num_eyes,
			userinfo->iod,
			userinfo->visren_rw2vw->v[ 0],
			userinfo->visren_rw2vw->v[ 4],
			userinfo->visren_rw2vw->v[ 8],
			userinfo->visren_rw2vw->v[12],
			userinfo->visren_rw2vw->v[ 1],
			userinfo->visren_rw2vw->v[ 5],
			userinfo->visren_rw2vw->v[ 9],
			userinfo->visren_rw2vw->v[13],
			userinfo->visren_rw2vw->v[ 2],
			userinfo->visren_rw2vw->v[ 6],
			userinfo->visren_rw2vw->v[10],
			userinfo->visren_rw2vw->v[14],
			userinfo->visren_rw2vw->v[ 3],
			userinfo->visren_rw2vw->v[ 7],
			userinfo->visren_rw2vw->v[11],
			userinfo->visren_rw2vw->v[15]);
		break;

	default:
	case verbose:
		vrFprintf(file, "{\n");
		vrFprintf(file, "\r"
			"\tObject_type = %s (%d)\n\tid = %d\n\tname = \"%s\"\n"
			"\tmalleable = %d\n\tnext = %#p\n"
			"\tCreated at %s, line %d\n"
			"\tLast modified at %s, line %d\n",
			vrObjectTypeName(userinfo->object_type),
			userinfo->object_type,
			userinfo->id,
			userinfo->name,
			userinfo->malleable,
			userinfo->next,
			userinfo->file_created,
			userinfo->line_created,
			userinfo->file_lastmod,
			userinfo->line_lastmod);
		vrFprintf(file, "\r"
			"\tsimdata lock = %#p\n\tvisren lock = %#p\n"
			"\tnum in system = %d\n\tiod = %f\n\tnum_eyes = %d\n"
			"\tcolor = [ %f %f %f ]\n"
			"\tnear_clip = %.2f\n\tfar_clip = %.2f\n"
			"\thead = %#p ('%s')\n",
			userinfo->simdata_lock,
			userinfo->visren_lock,
			/* TODO: add app_lock here -- or just replace visren_lock with app_lock */
			userinfo->num,
			userinfo->iod,
			userinfo->num_eyes,
			userinfo->color[0], userinfo->color[1], userinfo->color[2],
			userinfo->settings.near_clip,
			userinfo->settings.far_clip,
			userinfo->head,
			(userinfo->head != NULL ? (userinfo->head->my_object != NULL ? userinfo->head->my_object->name : "--") : "-"));
		if (userinfo->rw2vw_xform != NULL) {
			vrFprintf (file, 
				"\r\ttravel xform: rw2vw = (%3.1f %3.1f %3.1f %3.1f/%3.1f %3.1f %3.1f %3.1f/%3.1f %3.1f %3.1f %3.1f/%3.1f %3.1f %3.1f %3.1f)\n",
				userinfo->rw2vw_xform->v[ 0],
				userinfo->rw2vw_xform->v[ 4],
				userinfo->rw2vw_xform->v[ 8],
				userinfo->rw2vw_xform->v[12],
				userinfo->rw2vw_xform->v[ 1],
				userinfo->rw2vw_xform->v[ 5],
				userinfo->rw2vw_xform->v[ 9],
				userinfo->rw2vw_xform->v[13],
				userinfo->rw2vw_xform->v[ 2],
				userinfo->rw2vw_xform->v[ 6],
				userinfo->rw2vw_xform->v[10],
				userinfo->rw2vw_xform->v[14],
				userinfo->rw2vw_xform->v[ 3],
				userinfo->rw2vw_xform->v[ 7],
				userinfo->rw2vw_xform->v[11],
				userinfo->rw2vw_xform->v[15]);
		} else {
			vrFprintf (file, "\r\ttravel: rw2vw = <null>\n");
		}
		if (userinfo->vw2rw_xform != NULL) {
			vrFprintf (file, 
				"\r\ttravel xform: vw2rw = (%3.1f %3.1f %3.1f %3.1f/%3.1f %3.1f %3.1f %3.1f/%3.1f %3.1f %3.1f %3.1f/%3.1f %3.1f %3.1f %3.1f)\n",
				userinfo->vw2rw_xform->v[ 0],
				userinfo->vw2rw_xform->v[ 4],
				userinfo->vw2rw_xform->v[ 8],
				userinfo->vw2rw_xform->v[12],
				userinfo->vw2rw_xform->v[ 1],
				userinfo->vw2rw_xform->v[ 5],
				userinfo->vw2rw_xform->v[ 9],
				userinfo->vw2rw_xform->v[13],
				userinfo->vw2rw_xform->v[ 2],
				userinfo->vw2rw_xform->v[ 6],
				userinfo->vw2rw_xform->v[10],
				userinfo->vw2rw_xform->v[14],
				userinfo->vw2rw_xform->v[ 3],
				userinfo->vw2rw_xform->v[ 7],
				userinfo->vw2rw_xform->v[11],
				userinfo->vw2rw_xform->v[15]);
		} else {
			vrFprintf (file, "\r\ttravel: vw2rw = <null>\n");
		}
		if (userinfo->visren_headpos != NULL) {
			vrFprintf (file, 
				"\r\ttravel headpos: rw2vw = (%3.1f %3.1f %3.1f %3.1f/%3.1f %3.1f %3.1f %3.1f/%3.1f %3.1f %3.1f %3.1f/%3.1f %3.1f %3.1f %3.1f)\n",
				userinfo->visren_headpos->v[ 0],
				userinfo->visren_headpos->v[ 4],
				userinfo->visren_headpos->v[ 8],
				userinfo->visren_headpos->v[12],
				userinfo->visren_headpos->v[ 1],
				userinfo->visren_headpos->v[ 5],
				userinfo->visren_headpos->v[ 9],
				userinfo->visren_headpos->v[13],
				userinfo->visren_headpos->v[ 2],
				userinfo->visren_headpos->v[ 6],
				userinfo->visren_headpos->v[10],
				userinfo->visren_headpos->v[14],
				userinfo->visren_headpos->v[ 3],
				userinfo->visren_headpos->v[ 7],
				userinfo->visren_headpos->v[11],
				userinfo->visren_headpos->v[15]);
		} else {
			vrFprintf (file, "\r\ttravel: visren_headpos = <null>\n");
		}
		if (userinfo->visren_rw2vw != NULL) {
			vrFprintf (file, 
				"\r\ttravel visren: rw2vw = (%3.1f %3.1f %3.1f %3.1f/%3.1f %3.1f %3.1f %3.1f/%3.1f %3.1f %3.1f %3.1f/%3.1f %3.1f %3.1f %3.1f)\n",
				userinfo->visren_rw2vw->v[ 0],
				userinfo->visren_rw2vw->v[ 4],
				userinfo->visren_rw2vw->v[ 8],
				userinfo->visren_rw2vw->v[12],
				userinfo->visren_rw2vw->v[ 1],
				userinfo->visren_rw2vw->v[ 5],
				userinfo->visren_rw2vw->v[ 9],
				userinfo->visren_rw2vw->v[13],
				userinfo->visren_rw2vw->v[ 2],
				userinfo->visren_rw2vw->v[ 6],
				userinfo->visren_rw2vw->v[10],
				userinfo->visren_rw2vw->v[14],
				userinfo->visren_rw2vw->v[ 3],
				userinfo->visren_rw2vw->v[ 7],
				userinfo->visren_rw2vw->v[11],
				userinfo->visren_rw2vw->v[15]);
		} else {
			vrFprintf (file, "\r\ttravel: visren_rw2vw = <null>\n");
		}
		if (userinfo->visren_vw2rw != NULL) {
			vrFprintf (file, 
				"\r\ttravel visren: vw2rw = (%3.1f %3.1f %3.1f %3.1f/%3.1f %3.1f %3.1f %3.1f/%3.1f %3.1f %3.1f %3.1f/%3.1f %3.1f %3.1f %3.1f)\n",
				userinfo->visren_vw2rw->v[ 0],
				userinfo->visren_vw2rw->v[ 4],
				userinfo->visren_vw2rw->v[ 8],
				userinfo->visren_vw2rw->v[12],
				userinfo->visren_vw2rw->v[ 1],
				userinfo->visren_vw2rw->v[ 5],
				userinfo->visren_vw2rw->v[ 9],
				userinfo->visren_vw2rw->v[13],
				userinfo->visren_vw2rw->v[ 2],
				userinfo->visren_vw2rw->v[ 6],
				userinfo->visren_vw2rw->v[10],
				userinfo->visren_vw2rw->v[14],
				userinfo->visren_vw2rw->v[ 3],
				userinfo->visren_vw2rw->v[ 7],
				userinfo->visren_vw2rw->v[11],
				userinfo->visren_vw2rw->v[15]);
		} else {
			vrFprintf (file, "\r\ttravel: visren_vw2rw = <null>\n");
		}
		vrFprintf (file,
			"\r\tnum_sensors = %d\n\tsensor list = ...\n"
			"\tVisrenInit = %#p\n\tVisrenFrame = %#p\n\tVisrenWorld = %#p\n"
			"\tVisrenSim = %#p\n\tVisrenExit = %#p\n"
			"}\n",
			userinfo->num_inputs,	/* TODO: is this ever set to anything? */
			/* TODO: input names and pointers */
			userinfo->VisrenInit,
			userinfo->VisrenFrame,
			userinfo->VisrenWorld,
			userinfo->VisrenSim,
			userinfo->VisrenExit);
		break;

	case file_format:
		vrFprintf(file, "user \"%s\" = {\n", userinfo->name);

		vrFprintf(file, "\tiod = %f;\n", userinfo->iod);
		vrFprintf(file, "\tcolor = %f, %f, %f;\n", userinfo->color[0], userinfo->color[1], userinfo->color[2]);
		if (userinfo->settings.near_clip != -1 /* VRTOKEN_DEFAULT */)
			vrFprintf(file, "\tnearClip = %.3f;\n", userinfo->settings.near_clip);
		else	vrFprintf(file, "\t# Inherit from Global: nearClip = \"default\";\n");
		if (userinfo->settings.far_clip != -1 /* VRTOKEN_DEFAULT */)
			vrFprintf(file, "\tfarClip = %.3f;\n", userinfo->settings.far_clip);
		else	vrFprintf(file, "\t# Inherit from Global: farClip = \"default\";\n");
		vrFprintf(file, "\n");

		/* TODO: perhaps r2b_translate, and head sensor info */
		vrFprintf(file, "\t# TODO: *perhaps* print r2b_translate, and head sensor info\n");

		vrFprintf(file, "}\n\n");
		break;
	}
}


/************************************************************/
/* map user body parts to sensors */
void vrUserInitialize(vrContextInfo *context)
{
	vrInputInfo	*vrInputs = context->input;
	vrConfigInfo	*vrConfig = context->config;
	int		count;
	int		sensor_num;

	/* map user body parts to sensors */
	/* TODO: currently this is hardwired, needs to use config info */
	/*   WARNING. this hardwired version assumes there are at least */
	/*   as many sensors as there are users.                        */
	/* [11/4/99] This needs to wait until input mapping is implemented, */
	/*   because that's where user body parts will be mapped to inputs. */
	/* [12/7/99] For now, we skip one sensor after the first user to act*/
	/*   as the default input controller (ie. the wand).                */
	for (count = 0; count < vrConfig->num_users; count++) {
#if 0 /* [11/4/08] Set to "1" to skip two sensors for a 2nd user.  Order will be: head #1, wand #1, wand #2, head #2. */
		sensor_num = (count == 0 ? count : count+2);
#else /* the default */
		sensor_num = (count == 0 ? count : count+1);
#endif
		if (vrInputs->num_6sensors <= sensor_num) {
			vrDbgPrintfN(ALWAYS_DBGLVL, "vrUserInitialize(): " RED_TEXT
				"No 6-Sensor inputs for user %d, using dummy sensor (%#p) instead.\n" NORM_TEXT,
				count, vrConfig->users[count]->head);
		} else {
			vrConfig->users[count]->head = vrContext->input->sensor6[sensor_num];
			vrDbgPrintfN(AALWAYS_DBGLVL, "vrUserInitialize(): " RED_TEXT "Hardwire of "
				BOLD_TEXT "vrConfig->users[%d]->head = %#p\n" NORM_TEXT,
				count, vrConfig->users[count]->head);
		}
	}

	vrConfig->users_init = 1;
}


/************************************************************/
/* report whether user body parts have been mapped to sensors */
int vrUsersInitialized(vrContextInfo *context)
{
	return context->config->users_init;
}


/* rwvec = headvec * vrMatrixGetRWFromUserHead(<head matrix>, <usernum>) */
/* canonical head coordsys -- matches CG camera: -z=forward, y=up, x=right */
/* NOTE: this used to be: vrMatrix *vrUserGetHeadPos(int usernum, vrMatrix *headmat) */
/* NOTE: I don't think this function should be called from the render process */
/*   it doesn't use the frozen head data, and since it can be called from the */
/*   simulation process, it shouldn't use the frozen head data.  There should */
/*   be an alternative function call or structure reference for the render    */
/*   process.                                                                 */
/************************************************************/
vrMatrix *vrMatrixGetRWFromUserHead(vrMatrix *headmat, int usernum)
{
	if (vrContext->input->users == NULL)
		return NULL;
	if (usernum >= vrContext->input->num_users)
		return NULL;
	if (vrContext->input->users[usernum]->head == NULL) {
		return NULL;
	} else {
		vrMatrixGet6sensorValuesDirect(headmat, vrContext->input->users[usernum]->head);
		return headmat;
	}
}


/************************************************************/
/* NOTE: this function is to be called from the simulation process */
vrMatrix *vrMatrixGetVWUserFromRWMatrix(vrMatrix *virtual_mat, int usernum, vrMatrix *real_mat)
{
	vrUserInfo	*user_data;

	if (usernum >= vrContext->input->num_users || usernum < 0) {
		vrErrPrintf(RED_TEXT "vrMatrixGetVWUserFromRWMatrix(): invalid user number %d, range is [0..%d].\n" NORM_TEXT,
			usernum, vrContext->input->num_users-1);
		return real_mat;
	} else {
		vrMatrixCopy(virtual_mat, real_mat);

		user_data = vrContext->input->users[usernum];
		vrLockReadSet(user_data->app_lock);
		vrLockReadSet(user_data->simdata_lock);
		vrMatrixPreMult(virtual_mat, user_data->vw2rw_xform);
		vrLockReadRelease(user_data->simdata_lock);
		vrLockReadRelease(user_data->app_lock);

		return virtual_mat;
	}
}


/************************************************************/
/* DONE (and then apparently undone): should this be renamed to vrUserConvertPointVWfromRW() to keep args in same order? */
/*       or what about vrPointConvertFromUserVW()? */
/*	vrPointUserVWFromRW() has Stuart's vote (and that's what it was for a time) */

/* NOTE: that if the given user isn't found, then we return the */
/*   original RW-point which is effectively the identity transformation */
/*   (ie. as if the imaginary user hadn't moved.) */
/* NOTE: this used to be:                                                                      */
/*       vrPoint *vrUserConvertPointRWtoVW(int usernum, vrPoint *real_pnt, vrPoint *virtual_pnt) */
/* NOTE: this used to be:                                                                      */
/*       vrPoint *vrPointUserVWFromRW(int usernum, vrPoint *virtual_pnt, vrPoint *real_pnt)      */
/* NOTE: this function is to be called from the simulation process */
vrPoint *vrPointGetVWUserFromRWPoint(vrPoint *virtual_pnt, int usernum, vrPoint *real_pnt)
{
	vrUserInfo	*user_data;

	if (usernum >= vrContext->input->num_users || usernum < 0) {
		vrErrPrintf(RED_TEXT "vrPointGetVWUserFromRWPoint(): invalid user number %d, range is [0..%d].\n" NORM_TEXT,
			usernum, vrContext->input->num_users-1);
		return real_pnt;
	} else {
		user_data = vrContext->input->users[usernum];
		vrLockReadSet(user_data->app_lock);
		vrLockReadSet(user_data->simdata_lock);
		vrPointTransformByMatrix(virtual_pnt, real_pnt, user_data->vw2rw_xform);
		vrLockReadRelease(user_data->simdata_lock);
		vrLockReadRelease(user_data->app_lock);

		return virtual_pnt;
	}
}


/************************************************************/
/* NOTE: this function is to be called from the simulation process */
vrPoint *vrPointGetRWFromVWUserPoint(vrPoint *real_pnt, int usernum, vrPoint *virtual_pnt)
{
	vrUserInfo	*user_data;

	if (usernum >= vrContext->input->num_users || usernum < 0) {
		vrErrPrintf(RED_TEXT "vrPointGetRWFromVWUserPoint(): invalid user number %d, range is [0..%d].\n" NORM_TEXT,
			usernum, vrContext->input->num_users-1);
		return real_pnt;
	} else {
		user_data = vrContext->input->users[usernum];
		vrLockReadSet(user_data->app_lock);
		vrLockReadSet(user_data->simdata_lock);
		vrPointTransformByMatrix(real_pnt, virtual_pnt, user_data->rw2vw_xform);
		vrLockReadRelease(user_data->simdata_lock);
		vrLockReadRelease(user_data->app_lock);

		return real_pnt;
	}
}


/************************************************************/
/* NOTE: this function is to be called from the simulation process */
vrPointf *vrPointfGetRWFromVWUserPointf(vrPointf *real_pnt, int usernum, vrPointf *virtual_pnt)
{
	vrPoint	double_real;
	vrPoint	double_virt;

	double_virt.v[VR_X] = virtual_pnt->v[VR_X];
	double_virt.v[VR_Y] = virtual_pnt->v[VR_Y];
	double_virt.v[VR_Z] = virtual_pnt->v[VR_Z];

	vrPointGetRWFromVWUserPoint(&double_real, usernum, &double_virt);

	real_pnt->v[VR_X] = double_real.v[VR_X];
	real_pnt->v[VR_Y] = double_real.v[VR_Y];
	real_pnt->v[VR_Z] = double_real.v[VR_Z];

	return real_pnt;
}


/************************************************************/
/* NOTE: this used to be:                                                                        */
/*   vrVector *vrUserConvertVectorRWtoVW(int usernum, vrVector *real_vec, vrVector *virtual_vec) */
/* NOTE: this function is to be called from the simulation process (and this is because of */
/*   the reference to user_data->vw2rw_xform -- user_data->visren_vw2rw would be used in   */
/*   the rendering process.                                                                */
/* TODO: 6/2/03: I think this may be returning a mirror'd vector -- ie. not correct.  I */
/*   changed the transformation from vw2rw to rw2vw.                                    */
/*   6/3/03: Well, today I decided that it was probably correct in the first place (ie. */
/*   before my change yesterday.  The confusion was that I was using virtual world      */
/*   coordinates but rendering in real world space.                                     */
vrVector *vrVectorUserVWFromRW(int usernum, vrVector *virtual_vec, vrVector *real_vec)
{
	vrUserInfo	*user_data;

	if (usernum >= vrContext->input->num_users || usernum < 0) {
		vrErrPrintf(RED_TEXT "vrVectorUserVWFromRW(): invalid user number %d, range is [0..%d].\n" NORM_TEXT,
			usernum, vrContext->input->num_users-1);
		return real_vec;
	} else {
		user_data = vrContext->input->users[usernum];
		vrLockReadSet(user_data->app_lock);
		vrLockReadSet(user_data->simdata_lock);
#if 1 /* 6/2/03 - vw2rw may be the opposite transformation -- 6/3/03: today I decided that it was correct in the first place. */
		vrVectorTransformByMatrix(virtual_vec, real_vec, user_data->vw2rw_xform);
#else
		vrVectorTransformByMatrix(virtual_vec, real_vec, user_data->rw2vw_xform);
#endif
		vrLockReadRelease(user_data->simdata_lock);
		vrLockReadRelease(user_data->app_lock);

		return virtual_vec;
	}
}


/************************************************************/
/* NOTE: this used to be: vrPoint *vrGetMatrixLocationRW(vrPoint *real_point, vrMatrix *mat) */
/* implies that Matrix is vw2rw */
/* NOTE: since this is a RW transformation is may be called from the simulation or rendering process */
vrPoint *vrPointGetRWLocationFromMatrix(vrPoint *real_point, vrMatrix *mat)
{
	return vrPointGetTransFromMatrix(real_point, mat);
}


/************************************************************/
/* NOTE: [1/26/01] this may be incorrect due to a test change to vrPointGetVWUserFromRWPoint() */
/*  it is currently not used by the library, and only used for informational display in ex8 (as a method of emulating the CAVENavGetMatrix() function). */
/* NOTE: this used to be: vrPoint *vrGetMatrixLocationVW(int usernum, vrPoint *virtual_point, vrMatrix *mat) */
/* NOTE: then this used to be: vrPointGetUserVWLocationFromMatrix(usernum, vrPoint *virtual_point, vrMatrix *mat) */
/* NOTE: this function is to be called from the simulation process */
vrPoint *vrPointGetVWFromUserMatrix(vrPoint *virtual_point, int usernum, vrMatrix *mat)
{
	vrPoint	real_point;

	vrPointGetTransFromMatrix(&real_point, mat);

	return vrPointGetVWUserFromRWPoint(virtual_point, usernum, &real_point);
}


/************************************************************/
/* NOTE: this function is to be called from the simulation process */
vrMatrix *vrMatrixGetUserTravel(vrMatrix *mat, int usernum)
{
	vrUserInfo	*user_data;

	if (usernum >= vrContext->input->num_users || usernum < 0) {
		vrErrPrintf(RED_TEXT "vrMatrixGetUserTravel(): invalid user number %d, range is [0..%d].  Will give User 0 value.\n" NORM_TEXT,
			usernum, vrContext->input->num_users-1);

		usernum = 0;
	}

	user_data = vrContext->input->users[usernum];
	vrLockReadSet(user_data->app_lock);
	vrLockReadSet(user_data->simdata_lock);
	memcpy(mat, user_data->rw2vw_xform, sizeof(*mat));
	vrLockReadRelease(user_data->simdata_lock);
	vrLockReadRelease(user_data->app_lock);

	return mat;
}


/************************************************************/
/* NOTE: this function is to be called from the simulation process */
vrMatrix *vrMatrixGetUserTravelInv(vrMatrix *mat, int usernum)
{
	vrUserInfo	*user_data;

	if (usernum >= vrContext->input->num_users || usernum < 0) {
		vrErrPrintf(RED_TEXT "vrMatrixGetUserTravel(): invalid user number %d, range is [0..%d].  Will give User 0 value.\n" NORM_TEXT,
			usernum, vrContext->input->num_users-1);

		usernum = 0;
	}

	user_data = vrContext->input->users[usernum];
	vrLockReadSet(user_data->app_lock);
	vrLockReadSet(user_data->simdata_lock);
	memcpy(mat, user_data->vw2rw_xform, sizeof(*mat));
	vrLockReadRelease(user_data->simdata_lock);
	vrLockReadRelease(user_data->app_lock);

	return mat;
}


/************************************************************/
/* NOTE: this function is to be called from the simulation process */
int vrUserTravelReset(int usernum)
{
	int		count;
	vrUserInfo	*user_data;

	if (usernum == VR_ALLUSERS) {
		for (count = 0; count < vrContext->input->num_users; count++) {
			/* NOTE: we could just recursively call this function for each user */

			user_data = vrContext->input->users[count];
			vrLockWriteSet(user_data->simdata_lock);
			vrMatrixSetIdentity(user_data->vw2rw_xform);
			vrMatrixSetIdentity(user_data->rw2vw_xform);
			vrLockWriteRelease(user_data->simdata_lock);
		}
	} else if (usernum < 0) {
		vrErrPrintf(RED_TEXT "vrUserTravelReset(): invalid user number %d.\n" NORM_TEXT, usernum);
		return -1;
	} else if (usernum >= vrContext->input->num_users) {
		vrErrPrintf(RED_TEXT "vrUserTravelReset(): invalid user number %d, only %d users.\n" NORM_TEXT,
			usernum, vrContext->input->num_users);
		return -1;
	} else {
		user_data = vrContext->input->users[usernum];
		vrLockWriteSet(user_data->simdata_lock);
		vrMatrixSetIdentity(user_data->vw2rw_xform);
		vrMatrixSetIdentity(user_data->rw2vw_xform);
		vrLockWriteRelease(user_data->simdata_lock);
	}

	return 0;
}


/************************************************************/
/* translate the User in their coordinate system */
/* NOTE: this function is to be called from the simulation process */
int vrUserTravelTranslate3d(int usernum, double x, double y, double z)
{
	int		count;
	vrUserInfo	*user_data;

	if (usernum == VR_ALLUSERS) {
		for (count = 0; count < vrContext->input->num_users; count++) {
			/* NOTE: we could just recursively call this function for each user */

			user_data = vrContext->input->users[count];
			vrLockWriteSet(user_data->simdata_lock);
			vrMatrixPostTranslate3d(user_data->vw2rw_xform,  x,  y,  z);
			vrMatrixPreTranslate3d(user_data->rw2vw_xform,  -x, -y, -z);
			vrLockWriteRelease(user_data->simdata_lock);
		}
	} else if (usernum < 0) {
		vrErrPrintf(RED_TEXT "vrUserTravelTranslate3d(): invalid user number %d.\n" NORM_TEXT, usernum);
		return -1;
	} else if (usernum >= vrContext->input->num_users) {
		vrErrPrintf(RED_TEXT "vrUserTravelTranslate3d(): invalid user number %d, only %d users.\n" NORM_TEXT,
			usernum, vrContext->input->num_users);
		return -1;
	} else {
		user_data = vrContext->input->users[count];
		vrLockWriteSet(user_data->simdata_lock);
		vrMatrixPostTranslate3d(user_data->vw2rw_xform,  x,  y,  z);
		vrMatrixPreTranslate3d(user_data->rw2vw_xform,  -x, -y, -z);
		vrLockWriteRelease(user_data->simdata_lock);
	}

	return 0;
}


/************************************************************/
/* NOTE: this function is to be called from the simulation process */
int vrUserTravelTranslateAd(int usernum, double *array)
{
	return vrUserTravelTranslate3d(usernum, array[VR_X], array[VR_Y], array[VR_Z]);
}


/************************************************************/
/* NOTE: this function is to be called from the simulation process */
int vrUserTravelTranslateVec(int usernum, vrVector *vec)
{
	return vrUserTravelTranslate3d(usernum, vec->v[VR_X], vec->v[VR_Y], vec->v[VR_Z]);
}


/************************************************************/
/* NOTE: this function is to be called from the simulation process */
int vrUserTravelRotateId(int usernum, int axis, double theta)
{
	int		count;
	vrUserInfo	*user_data;

	if (usernum == VR_ALLUSERS) {
		for (count = 0; count < vrContext->input->num_users; count++) {
			/* NOTE: we could just recursively call this function for each user */

			user_data = vrContext->input->users[count];
			vrLockWriteSet(user_data->simdata_lock);
			vrMatrixPreRotateId(user_data->rw2vw_xform, axis, -theta);

#if 0 /* 03/23/2007 -- an attempt at addressing the following question (and this method was tested with my billborads tutorial example, and that seems to verify that the new version works). */
			/* TODO: why aren't we just doing the opposite rotation? */
			vrMatrixInvertEuclidean(user_data->vw2rw_xform, user_data->rw2vw_xform);
#else
			vrMatrixPostRotateId(user_data->vw2rw_xform, axis,  theta);
#endif

			vrLockWriteRelease(user_data->simdata_lock);
		}
	} else if (usernum < 0) {
		vrErrPrintf(RED_TEXT "vrUserTravelRotateId(): invalid user number %d.\n" NORM_TEXT, usernum);
		return -1;
	} else if (usernum >= vrContext->input->num_users) {
		vrErrPrintf(RED_TEXT "vrUserTravelRotateId(): invalid user number %d, only %d users.\n" NORM_TEXT,
			usernum, vrContext->input->num_users);
		return -1;
	} else {
		user_data = vrContext->input->users[count];
		vrLockWriteSet(user_data->simdata_lock);
		vrMatrixPreRotateId(user_data->rw2vw_xform, axis,  -theta);
#if 0 /* 03/23/2007 (see above) */
		vrMatrixInvertEuclidean(user_data->vw2rw_xform, user_data->rw2vw_xform);
#else
		vrMatrixPostRotateId(user_data->vw2rw_xform, axis,  theta);
#endif
		vrLockWriteRelease(user_data->simdata_lock);
	}

	return 0;
}


/************************************************************/
/* NOTE: we only allow uniform scales to maintain the Euclidean */
/*   nature of the travel matrix.                               */
/* NOTE: this function is to be called from the simulation process */
int vrUserTravelScale(int usernum, double scale)
{
	int		count;
	vrUserInfo	*user_data;
	double		scale_recip = 1.0 / scale;

	if (usernum == VR_ALLUSERS) {
		for (count = 0; count < vrContext->input->num_users; count++) {
			/* NOTE: we could just recursively call this function for each user */

			user_data = vrContext->input->users[count];
			vrLockWriteSet(user_data->simdata_lock);
#if 0 /* 03/23/2007 -- scaling was reversed (was scaling world by factor rather than the user */
			vrMatrixPreScale3d(user_data->rw2vw_xform, scale, scale, scale);

			/* TODO: why aren't we just doing the opposite scaling? */
			vrMatrixInvertEuclidean(user_data->vw2rw_xform, user_data->rw2vw_xform);
#else
			vrMatrixPostScale3d(user_data->vw2rw_xform, scale, scale, scale);
#  if 0 /* 03/23/2007 -- attempting to just do the opposite transform */
			vrMatrixInvertEuclidean(user_data->rw2vw_xform, user_data->vw2rw_xform);
#  else
			vrMatrixPreScale3d(user_data->rw2vw_xform, scale_recip, scale_recip, scale_recip);
#  endif
#endif

			vrLockWriteRelease(user_data->simdata_lock);
		}
	} else if (usernum < 0) {
		vrErrPrintf(RED_TEXT "vrUserTravelScale(): invalid user number %d.\n" NORM_TEXT, usernum);
		return -1;
	} else if (usernum >= vrContext->input->num_users) {
		vrErrPrintf(RED_TEXT "vrUserTravelScale(): invalid user number %d, only %d users.\n" NORM_TEXT,
			usernum, vrContext->input->num_users);
		return -1;
	} else {
		user_data = vrContext->input->users[count];
		vrLockWriteSet(user_data->simdata_lock);
#if 0 /* 03/23/2007 -- this should make this right (see above) */
		vrMatrixPreScale3d(user_data->rw2vw_xform, scale, scale, scale);
		vrMatrixInvertEuclidean(user_data->vw2rw_xform, user_data->rw2vw_xform);
#else
		vrMatrixPostScale3d(user_data->vw2rw_xform, scale, scale, scale);
		vrMatrixPreScale3d(user_data->rw2vw_xform, scale, scale, scale);
#endif
		vrLockWriteRelease(user_data->simdata_lock);
	}

	return 0;
}


/************************************************************/
/* NOTE: this function is to be called from the simulation process */
int vrUserTravelTransformMatrix(int usernum, vrMatrix *mat)
{
	int		count;
	vrUserInfo	*user_data;
	vrMatrix	tmpmat;		/* for storing a modified version of the matrix if nece */

	/* Test whether supplied matrix is Euclidean */
	if (!vrMatrixIsEuclidean(mat)) {
		vrErrPrintf("vrUserTravelTransformMatrix(): " RED_TEXT "Matrix is not Euclidean!  Will be modified.\n" NORM_TEXT);
		vrMatrixMakeEuclidean(&tmpmat, mat);
		mat = &tmpmat;		/* Use the local copy of the matrix */
	}

	if (usernum == VR_ALLUSERS) {
		for (count = 0; count < vrContext->input->num_users; count++) {
			/* NOTE: we could just recursively call this function for each user */

			user_data = vrContext->input->users[count];
			vrLockWriteSet(user_data->simdata_lock);
			vrMatrixPreMult(user_data->rw2vw_xform, mat);
			vrMatrixInvertEuclidean(user_data->vw2rw_xform, user_data->rw2vw_xform);
			vrLockWriteRelease(user_data->simdata_lock);
		}
	} else if (usernum < 0) {
		vrErrPrintf(RED_TEXT "vrUserTravelTransformMatrix(): invalid user number %d.\n" NORM_TEXT, usernum);
		return -1;
	} else if (usernum >= vrContext->input->num_users) {
		vrErrPrintf(RED_TEXT "vrUserTravelTransformMatrix(): invalid user number %d, only %d users.\n" NORM_TEXT,
			usernum, vrContext->input->num_users);
		return -1;
	} else {
		user_data = vrContext->input->users[count];
		vrLockWriteSet(user_data->simdata_lock);
		vrMatrixPreMult(user_data->rw2vw_xform, mat);
		vrMatrixInvertEuclidean(user_data->vw2rw_xform, user_data->rw2vw_xform);
		vrLockWriteRelease(user_data->simdata_lock);
	}

	return 0;
}


/************************************************************/
/* NOTE: this function is to be called from the simulation process */
int vrUserTravelLockSet(int usernum)
{
	int		count;
	vrUserInfo	*user_data;

	if (usernum == VR_ALLUSERS) {
		for (count = 0; count < vrContext->input->num_users; count++) {
			/* NOTE: we could just recursively call this function for each user */

			user_data = vrContext->input->users[count];
			vrLockWriteSet(user_data->app_lock);
		}
	} else if (usernum < 0) {
		vrErrPrintf(RED_TEXT "vrUserTravelLockSet(): invalid user number %d.\n" NORM_TEXT, usernum);
		return -1;
	} else if (usernum >= vrContext->input->num_users) {
		vrErrPrintf(RED_TEXT "vrUserTravelLockSet(): invalid user number %d, only %d users.\n" NORM_TEXT,
			usernum, vrContext->input->num_users);
		return -1;
	} else {
		user_data = vrContext->input->users[count];
		vrLockWriteSet(user_data->app_lock);
	}

	return 0;
}


/************************************************************/
/* NOTE: this function is to be called from the simulation process */
int vrUserTravelLockRelease(int usernum)
{
	int		count;
	vrUserInfo	*user_data;

	if (usernum == VR_ALLUSERS) {
		for (count = 0; count < vrContext->input->num_users; count++) {
			/* NOTE: we could just recursively call this function for each user */

			user_data = vrContext->input->users[count];
			vrLockWriteRelease(user_data->app_lock);
		}
	} else if (usernum < 0) {
		vrErrPrintf(RED_TEXT "vrUserTravelLockRelease(): invalid user number %d.\n" NORM_TEXT, usernum);
		return -1;
	} else if (usernum >= vrContext->input->num_users) {
		vrErrPrintf(RED_TEXT "vrUserTravelLockRelease(): invalid user number %d, only %d users.\n" NORM_TEXT,
			usernum, vrContext->input->num_users);
		return -1;
	} else {
		user_data = vrContext->input->users[count];
		vrLockWriteRelease(user_data->app_lock);
	}

	return 0;
}


/***************************************************************/
/* NOTE: the user's copy of the head position matrix is copied */
/*   from the "visren_position" field of the input because     */
/*   inputs are frozen first, and so we want to be consistent  */
/*   with the frozen values of the corresponding input for the */
/*   head.  Also, there is no need to do a read-lock on this   */
/*   field, since we know we're outside the area where that    */
/*   field can be altered.                                     */
/* NOTE: we could use vrMatrixCopy() instead of direct copies  */
/*   from the "=" operator, but I don't know that that buys us */
/*   anything.                                                 */
/* NOTE: this function is called by the library in the visual rendering */
/*   process, but at a time when no visual rendering can occur.         */
void vrUserTravelFreezeVisren(vrContextInfo *context)
{
	vrUserInfo	*user;
	int		count;

	/***************************************************/
	/* freeze all the user values for visual rendering */
	for (count = 0; count < vrContext->input->num_users; count++) {
		user = vrContext->input->users[count];
		if (user != NULL) {
			vrLockReadSet(user->app_lock);
			vrLockReadSet(user->simdata_lock);
			vrLockWriteSet(user->visren_lock);
			*(user->visren_headpos) = *(user->head->visren_position);	/* NOTE: user->head is a 6-sensor input */
			*(user->visren_rw2vw) = *(user->rw2vw_xform);
			*(user->visren_vw2rw) = *(user->vw2rw_xform);
			vrLockWriteRelease(user->visren_lock);
			vrLockReadRelease(user->simdata_lock);
			vrLockReadRelease(user->app_lock);
		}
	}
}

	/**************************************************************/
	/**************************************************************/

#ifdef GFX_PERFORMER /* { */
/***********************************************************************************/
/* Utility functions that translate FreeVR coordinates in to Performer Z-up coords */
/***********************************************************************************/
/* TODO: in the future we probably will just have a conversion matrix stored */
/*   with the context, and we'll use that matrix in the original functions   */
/*   to convert to whatever coordinate system the graphics system uses.      */


/**********************************************************************/
int vrPfUserTravelReset(int usernum)
{
	return vrUserTravelReset(usernum);
}


/**********************************************************************/
int vrPfUserTravelTranslate3d(int usernum, double x, double y, double z)
{
	return vrUserTravelTranslate3d(usernum, x, -z, y);
}


/**********************************************************************/
int vrPfUserTravelTranslateAd(int usernum, double *array)
{
	return vrUserTravelTranslate3d(usernum, array[VR_X], -array[VR_Z], array[VR_Y]);
}


/**********************************************************************/
int vrPfUserTravelTranslateVec(int usernum, vrVector *vec)
{
	return vrUserTravelTranslate3d(usernum, vec->v[VR_X], -vec->v[VR_Z], vec->v[VR_Y]);
}


/**********************************************************************/
/* NOTE: this really needs testing */
int vrPfUserTravelRotateId(int usernum, int axis, double theta)
{
	switch (axis) {
	case VR_X:
		return vrUserTravelRotateId(usernum, axis, theta);
	case VR_Y:
		return vrUserTravelRotateId(usernum, VR_Z, theta);
	case VR_Z:
		return vrUserTravelRotateId(usernum, VR_Y, -theta);
	}

	return vrUserTravelRotateId(usernum, axis, theta);	/* We shouldn't get here, but the compiler wants a final return */
}


/**********************************************************************/
/* NOTE: since we scale the same way in all directions, there is no need to manipulate the coord-sys */
int vrPfUserTravelScale(int usernum, double scale)
{
	return vrUserTravelScale(usernum, scale);
}


/**********************************************************************/
/* NOTE: this requires testing, I don't know if the matrix multiply is in the right order */
int vrPfUserTravelTransformMatrix(int usernum, vrMatrix *mat)
{
static	vrMatrix	mat_pos90x = { 1.0,0.0,0.0,0.0,  0.0,0.0,1.0,0.0,  0.0,-1.0,0.0,0.0,  0.0,0.0,0.0,1.0 };
	vrMatrix	tmpmat;

	return vrUserTravelTransformMatrix(usernum, vrMatrixProduct(&tmpmat, mat, &mat_pos90x));
}

#endif /* GFX_PERFORMER } */


	/**************************************************************/
	/**************************************************************/



/*********************************************************************/
/***********************   Routines for Props   **********************/

/************************************************************/
void vrPropClear(vrPropInfo *object)
{
	/* nothing really to clear */
}


/************************************************************/
void vrPropCopy(vrPropInfo *dest_object, vrPropInfo *src_object)
{
	void	*dest_mem;
	void	*src_mem;
	int	memlen;

	/* copy only the memory after the generic vrObjectInfo stuff */
	dest_mem = (char *)dest_object + sizeof(vrObjectInfo);
	src_mem = (char *)src_object + sizeof(vrObjectInfo);
	memlen = sizeof(vrPropInfo) - sizeof(vrObjectInfo);
	memcpy(dest_mem, src_mem, memlen);
}


/************************************************************/
void vrPropFreezeVisren(vrContextInfo *context)
{
#if 0
	vrPrintf("vrPropFreezeVisren(): Not yet implemented\n");
#endif
}


/************************************************************/
void vrFprintPropInfo(FILE *file, vrPropInfo *propinfo, vrPrintStyle style)
{
	if (propinfo == NULL) {
		vrFprintf(file, "prop \"(nil)\" = { }\n");
		return;
	}

	switch (style) {

	case machine:
		/* TODO: not yet implemented -- output easily machine parsed */
		/* fall through to "one_line" for now */
	case one_line:
		/* TODO: not yet independently implemented -- very brief output */
	case brief:
		vrFprintf(file, "id = %d, name = '%s'\n",
			propinfo->id,
			propinfo->name);
		break;

	default:
	case verbose:
		vrFprintf(file, "{\n");
		vrFprintf(file, "\r"
			"\tObject_type = %s (%d)\n\tid = %d\n\tname = \"%s\"\n"
			"\tmalleable = %d\n\tnext = %#p\n"
			"\tCreated at %s, line %d\n"
			"\tLast modified at %s, line %d\n"
			"}\n",
			vrObjectTypeName(propinfo->object_type),
			propinfo->object_type,
			propinfo->id,
			propinfo->name,
			propinfo->malleable,
			propinfo->next,
			propinfo->file_created,
			propinfo->line_created,
			propinfo->file_lastmod,
			propinfo->line_lastmod);
		break;

	case file_format:
		vrFprintf(file, "prop \"%s\" = {\n", propinfo->name);

		/* TODO: */
		vrFprintf(file, "\t# TODO: no details associated with props yet.\n");

		vrFprintf(file, "}\n\n");
		break;
	}
}


/********************************************************************/
/***********************   Routines for Eyes   **********************/


/************************************************************/
vrEyeInfo *vrMakeEye(vrContextInfo *context, char *eyestring)
{
	vrEyeInfo	*eyeinfo;
	char		*username;
	char		*eyename;
	char		*colorname;
	int		toklen;

	eyeinfo = (vrEyeInfo *)vrShmemAlloc0(sizeof(vrEyeInfo));
	eyeinfo->object_type = VROBJECT_EYEINFO;
	eyeinfo->name = vrShmemStrDup(eyestring);
	eyeinfo->next = NULL;

	/* the user name portion goes to the first ":" (or end-of-string) */
	toklen = strcspn(eyestring, ":");
	username = eyestring;
	eyestring += toklen;
	if (eyestring[0] != '\0') {
		eyestring[0] = '\0';
		eyestring++;
	}

	/* the eye name portion goes to the first ":" (or end-of-string) */
	toklen = strcspn(eyestring, ":");
	eyename = eyestring;
	eyestring += toklen;
	if (eyestring[0] != '\0') {
		eyestring[0] = '\0';
		eyestring++;
	}

	/* the color name portion is the rest of the string */
	colorname = eyestring;


	/* now assign the user */
	eyeinfo->user = vrObjectSearch(context, VROBJECT_USER, username);
	if (eyeinfo->user == NULL) {
		vrErrPrintf("vrMakeEye():" RED_TEXT "Unknown user in eyelist (%s), using default.\n" NORM_TEXT, username);
		eyeinfo->user = vrObjectSearch(context, VROBJECT_USER, DEFAULT_USER_NAME);
		if (eyeinfo->user == NULL) {
			vrErrPrintf("vrMakeEye():" RED_TEXT "Unable to find \"" DEFAULT_USER_NAME "\" user, must abort.\n" NORM_TEXT);
			vrExit();
			exit(-1);
		}
	}

	/* now assign the eye type */
	eyeinfo->type = vrEyeValue(eyename);

	/* now assign the color type */
	eyeinfo->color = vrAnaglyphValue(colorname);

	/* assign the eye to the default frame buffer (for now) */
	eyeinfo->render_framebuffer = VRFB_DEFAULT;

#if 0 /* some debug info */
	vrPrintf("Made an eye at %#p = ", eyeinfo);
	vrFprintEyeInfo(stdout, eyeinfo, style);
	vrPrintf("------\n");
#endif

	return (eyeinfo);
}


/************************************************************/
void vrFprintEyeInfo(FILE *file, vrEyeInfo *eyeinfo, vrPrintStyle style)
{
	switch (style) {

	case machine:
		/* TODO: not yet implemented -- output easily machine parsed */
		/* fall through to "one_line" for now */
	case one_line:
		/* TODO: not yet independently implemented -- very brief output */
	case brief:
		vrFprintf(file, "name = '%s' (%d), type = %s (%d), color-type = %s (%d), fb-name = %s (%d)\n",
			eyeinfo->name,
			eyeinfo->num,
			vrEyeTypeName(eyeinfo->type),
			eyeinfo->type,
			vrAnaglyphTypeName(eyeinfo->color),
			eyeinfo->color,
			vrFrameBufferName(eyeinfo->render_framebuffer),
			eyeinfo->render_framebuffer);
		break;

	default:
	case verbose:
		vrFprintf(file, "{\n");
		vrFprintf(file,
			"\r\tname = \"%s\"\n\tnum in system = %d\n"
			"\tuser = %#p (\"%s\")\n\ttype = %d (%s)\n"
			"\tcolor type = %d (%s)\n\tframe buffer = \"%s\"\n"
			"\tlocation = (%lf, %lf, %lf)\n"
			"}\n",
			eyeinfo->name,
			eyeinfo->num,
			eyeinfo->user,
			eyeinfo->user->name,
			eyeinfo->type,
			vrEyeTypeName(eyeinfo->type),
			eyeinfo->color,
			vrAnaglyphTypeName(eyeinfo->color),
			vrFrameBufferName(eyeinfo->render_framebuffer),
			eyeinfo->loc.v[VR_X], eyeinfo->loc.v[VR_Y], eyeinfo->loc.v[VR_Z]);
		break;

	case file_format:
		vrFprintf(file, "#eye \"%s\" = {\n", eyeinfo->name);
		vrFprintf(file, "#\ttype = \"%s\"\n", vrEyeTypeName(eyeinfo->type));
		vrFprintf(file, "#\tcolor = \"%s\"\n", vrAnaglyphTypeName(eyeinfo->color));
		vrFprintf(file, "#\tfb = \"%s\"\n", vrFrameBufferName(eyeinfo->render_framebuffer));
		vrFprintf(file, "#}\n\n");
		break;
	}
}


/************************************************************/
void vrEyelistClear(vrEyelistInfo *object)
{
	/* TODO: this */
}


/************************************************************/
void vrEyelistCopy(vrEyelistInfo *dest_object, vrEyelistInfo *src_object)
{
	void	*dest_mem;
	void	*src_mem;
	int	memlen;
	int	count;

	/* copy only the memory after the generic vrObjectInfo stuff */
	dest_mem = (char *)dest_object + sizeof(vrObjectInfo);
	src_mem = (char *)src_object + sizeof(vrObjectInfo);
	memlen = sizeof(vrEyelistInfo) - sizeof(vrObjectInfo);
	memcpy(dest_mem, src_mem, memlen);

	/* make independent copy of some fields */
	dest_object->monofb_names = (char **)vrShmemAlloc0(src_object->num_monofb_names * sizeof(char *));
	for (count = 0; count < src_object->num_monofb_names; count++) {
		dest_object->monofb_names[count] = vrShmemStrDup(src_object->monofb_names[count]);
	}

	dest_object->leftfb_names = (char **)vrShmemAlloc0(src_object->num_leftfb_names * sizeof(char *));
	for (count = 0; count < src_object->num_leftfb_names; count++) {
		dest_object->leftfb_names[count] = vrShmemStrDup(src_object->leftfb_names[count]);
	}

	dest_object->rightfb_names = (char **)vrShmemAlloc0(src_object->num_rightfb_names * sizeof(char *));
	for (count = 0; count < src_object->num_rightfb_names; count++) {
		dest_object->rightfb_names[count] = vrShmemStrDup(src_object->rightfb_names[count]);
	}

	dest_object->leftvp_names = (char **)vrShmemAlloc0(src_object->num_leftvp_names * sizeof(char *));
	for (count = 0; count < src_object->num_leftvp_names; count++) {
		dest_object->leftvp_names[count] = vrShmemStrDup(src_object->leftvp_names[count]);
	}

	dest_object->rightvp_names = (char **)vrShmemAlloc0(src_object->num_rightvp_names * sizeof(char *));
	for (count = 0; count < src_object->num_rightvp_names; count++) {
		dest_object->rightvp_names[count] = vrShmemStrDup(src_object->rightvp_names[count]);
	}

	dest_object->anaglfb_names = (char **)vrShmemAlloc0(src_object->num_anaglfb_names * sizeof(char *));
	for (count = 0; count < src_object->num_anaglfb_names; count++) {
		dest_object->anaglfb_names[count] = vrShmemStrDup(src_object->anaglfb_names[count]);
	}
}


/************************************************************/
static void vrEyelistExpandNames(vrContextInfo *context, int num_eyes, vrEyeInfo **firstEye, char **eyelistnames, vrFrameBufferType frame_type)
{
#if 1 /* set to 1 to include '-' in the list of characters a name can have */
static	char		*strnumchars = "_:abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-.0123456789";
#else
static	char		*strnumchars = "_:abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.0123456789";
#endif
	char		*nexteyename;
	vrEyeInfo	**nexteye;
	char		*eyestring;
	int		toklen;
	int		count;


	nexteye = firstEye;

	for (count = 0; count < num_eyes; count++) {
		nexteyename = eyelistnames[count];
		while (strlen(nexteyename) > 0) {
			toklen = strspn(nexteyename, strnumchars);
			if (toklen > 0) {
				eyestring = vrShmemAlloc(toklen+1);
				strncpy(eyestring, nexteyename, toklen);
				eyestring[toklen] = '\0';
				*nexteye = vrMakeEye(context, eyestring);
				(*nexteye)->render_framebuffer = frame_type;
				vrShmemFree(eyestring);
				nexteye = &((*nexteye)->next);
			}
			nexteyename += toklen;	/* skip the alphanumeric string */
			if (nexteyename[0] != '\0')
				nexteyename++;	/* skip the (non-terminal) char that ended the string */
		}
	}
}


/************************************************************/
void vrEyelistInitialize(vrContextInfo *context, vrEyelistInfo *eyelistinfo)
{
	/* do "monofb" (aka "single") */
	vrEyelistExpandNames(context, eyelistinfo->num_monofb_names, &(eyelistinfo->monofb), eyelistinfo->monofb_names, VRFB_FULL);

	vrEyelistExpandNames(context, eyelistinfo->num_leftfb_names, &(eyelistinfo->leftfb), eyelistinfo->leftfb_names, VRFB_LEFT);
	vrEyelistExpandNames(context, eyelistinfo->num_rightfb_names, &(eyelistinfo->rightfb), eyelistinfo->rightfb_names, VRFB_RIGHT);
	vrEyelistExpandNames(context, eyelistinfo->num_leftvp_names, &(eyelistinfo->leftvp), eyelistinfo->leftvp_names, VRFB_FULL_LEFTEYE);
	vrEyelistExpandNames(context, eyelistinfo->num_rightvp_names, &(eyelistinfo->rightvp), eyelistinfo->rightvp_names, VRFB_FULL_RIGHTEYE);
	vrEyelistExpandNames(context, eyelistinfo->num_anaglfb_names, &(eyelistinfo->anaglfb), eyelistinfo->anaglfb_names, VRFB_FULL);
}


/************************************************************/
void vrFprintEyelistInfo(FILE *file, vrEyelistInfo *eyelistinfo, vrPrintStyle style)
{
	vrEyeInfo	*eye;

	switch (style) {

	case machine:
		/* TODO: not yet implemented -- output easily machine parsed */
		/* fall through to "one_line" for now */
	case one_line:
		/* TODO: not yet independently implemented -- very brief output */
	case brief:
		vrFprintf(file, "id = %d, name = '%s', mono's = %d, leftfb's = %d, rightfb's = %d, leftvp's = %d, rightvp's = %d, anaglfb's = %d\n",
			eyelistinfo->id,
			eyelistinfo->name,
			eyelistinfo->num_monofb_names,
			eyelistinfo->num_leftfb_names,
			eyelistinfo->num_rightfb_names,
			eyelistinfo->num_leftvp_names,
			eyelistinfo->num_rightvp_names,
			eyelistinfo->num_anaglfb_names);
		break;

	default:
	case verbose:
		vrFprintf(file, "{\n");
		vrFprintf(file, "\r"
			"\tObject_type = %s (%d)\n\tid = %d\n\tname = \"%s\"\n"
			"\tmalleable = %d\n\tnext = %#p\n"
			"\tCreated at %s, line %d\n"
			"\tLast modified at %s, line %d\n",
			vrObjectTypeName(eyelistinfo->object_type),
			eyelistinfo->object_type,
			eyelistinfo->id,
			eyelistinfo->name,
			eyelistinfo->malleable,
			eyelistinfo->next,
			eyelistinfo->file_created,
			eyelistinfo->line_created,
			eyelistinfo->file_lastmod,
			eyelistinfo->line_lastmod);
		vrFprintf(file, 
#if 0
			"\r\ttype = %d\n"
#endif
			"\r\tallFB list = (%d) [",
#if 0
			eyelistinfo->stereo,
#endif
			eyelistinfo->num_monofb_names);
#undef SHOW_BOTH
#ifdef SHOW_BOTH
		for (count = 0; count < eyelistinfo->num_monofb_names; count++)
			vrFprintf(file, " \"%s\"", eyelistinfo->monofb_names[count]);
		vrFprintf(file, " ]\n\tallFB linked list = [");
#endif
		for (eye = eyelistinfo->monofb; eye != NULL; eye = eye->next)
			vrFprintf(file, " \"%s\"", eye->name);

		vrFprintf(file, " ]\n\tleftFB list = (%d) [",
			eyelistinfo->num_leftfb_names);
#undef SHOW_BOTH
#ifdef SHOW_BOTH
		for (count = 0; count < eyelistinfo->num_leftfb_names; count++)
			vrFprintf(file, " \"%s\"", eyelistinfo->leftfb_names[count]);
		vrFprintf(file, " ]\n\tleftFB linked list = [");
#endif
		for (eye = eyelistinfo->leftfb; eye != NULL; eye = eye->next)
			vrFprintf(file, " \"%s\"", eye->name);

		vrFprintf(file, " ]\n\trightFB list = (%d) [",
			eyelistinfo->num_rightfb_names);
#undef SHOW_BOTH
#ifdef SHOW_BOTH
		for (count = 0; count < eyelistinfo->num_rightfb_names; count++)
			vrFprintf(file, " \"%s\"", eyelistinfo->rightfb_names[count]);
		vrFprintf(file, " ]\n\trightFB linked list = [");
#endif
		for (eye = eyelistinfo->rightfb; eye != NULL; eye = eye->next)
			vrFprintf(file, " \"%s\"", eye->name);

		vrFprintf(file, " ]\n\tleftVP list = (%d) [",
			eyelistinfo->num_leftvp_names);
#undef SHOW_BOTH
#ifdef SHOW_BOTH
		for (count = 0; count < eyelistinfo->num_leftvp_names; count++)
			vrFprintf(file, " \"%s\"", eyelistinfo->leftvp_names[count]);
		vrFprintf(file, " ]\n\tleftVP linked list = [");
#endif
		for (eye = eyelistinfo->leftvp; eye != NULL; eye = eye->next)
			vrFprintf(file, " \"%s\"", eye->name);

		vrFprintf(file, " ]\n\trightVP list = (%d) [",
			eyelistinfo->num_rightvp_names);
#undef SHOW_BOTH
#ifdef SHOW_BOTH
		for (count = 0; count < eyelistinfo->num_rightvp_names; count++)
			vrFprintf(file, " \"%s\"", eyelistinfo->rightvp_names[count]);
		vrFprintf(file, " ]\n\trightVP linked list = [");
#endif
		for (eye = eyelistinfo->rightvp; eye != NULL; eye = eye->next)
			vrFprintf(file, " \"%s\"", eye->name);

		vrFprintf(file, " ]\n\tanaglFB list = (%d) [",
			eyelistinfo->num_anaglfb_names);
#undef SHOW_BOTH
#ifdef SHOW_BOTH
		for (count = 0; count < eyelistinfo->num_anaglfb_names; count++)
			vrFprintf(file, " \"%s\"", eyelistinfo->anaglfb_names[count]);
		vrFprintf(file, " ]\n\tanaglFB linked list = [");
#endif
		for (eye = eyelistinfo->anaglfb; eye != NULL; eye = eye->next)
			vrFprintf(file, " \"%s\"", eye->name);
		vrFprintf(file, " ]\n}\n");

		break;

	case file_format:
		vrFprintf(file, "eyelist \"%s\" = {\n", eyelistinfo->name);

		/* monofb */
		if (eyelistinfo->monofb != NULL) {
			vrFprintf(file, "\tmonofb = ");
			for (eye = eyelistinfo->monofb; eye != NULL; eye = eye->next) {
				vrFprintf(file, "\"%s\"", eye->name);
				if (eye->next != NULL)
					vrFprintf(file, ", ");
			}
			vrFprintf(file, ";\n");
		}

		/* leftfb */
		if (eyelistinfo->leftfb != NULL) {
			vrFprintf(file, "\tleftfb = ");
			for (eye = eyelistinfo->leftfb; eye != NULL; eye = eye->next) {
				vrFprintf(file, "\"%s\"", eye->name);
				if (eye->next != NULL)
					vrFprintf(file, ", ");
			}
			vrFprintf(file, ";\n");
		}

		/* rightfb */
		if (eyelistinfo->rightfb != NULL) {
			vrFprintf(file, "\trightfb = ");
			for (eye = eyelistinfo->rightfb; eye != NULL; eye = eye->next) {
				vrFprintf(file, "\"%s\"", eye->name);
				if (eye->next != NULL)
					vrFprintf(file, ", ");
			}
			vrFprintf(file, ";\n");
		}

		/* leftvp */
		if (eyelistinfo->leftvp != NULL) {
			vrFprintf(file, "\tleftvp = ");
			for (eye = eyelistinfo->leftvp; eye != NULL; eye = eye->next) {
				vrFprintf(file, "\"%s\"", eye->name);
				if (eye->next != NULL)
					vrFprintf(file, ", ");
			}
			vrFprintf(file, ";\n");
		}

		/* rightvp */
		if (eyelistinfo->rightvp != NULL) {
			vrFprintf(file, "\trightvp = ");
			for (eye = eyelistinfo->rightvp; eye != NULL; eye = eye->next) {
				vrFprintf(file, "\"%s\"", eye->name);
				if (eye->next != NULL)
					vrFprintf(file, ", ");
			}
			vrFprintf(file, ";\n");
		}

		/* anaglfb */
		if (eyelistinfo->anaglfb != NULL) {
			vrFprintf(file, "\tanaglfb = ");
			for (eye = eyelistinfo->anaglfb; eye != NULL; eye = eye->next) {
				vrFprintf(file, "\"%s\"", eye->name);
				if (eye->next != NULL)
					vrFprintf(file, ", ");
			}
			vrFprintf(file, ";\n");
		}

		vrFprintf(file, "}\n\n");
		break;
	}
}

