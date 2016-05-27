/* ======================================================================
 *
 * HH   HH         vr_visren.h
 * HH   HH         Author(s): Ed Peters, Bill Sherman
 * HHHHHHH         Created: June 4, 1998
 * HH   HH         Last Modified: November 27, 2012
 * HH   HH
 *
 * Header file for FreeVR visual rendering information.
 *
 * Each Window onto a VE has a certain number of eyes for which to
 * render.  Each Window has its own (default) rendering function,
 * which might be overridden by a particular user of that window.
 * Each window also has projection coordinates, and a viewport mask.
 *
 * In order to at least maintain the fiction of window-system independence,
 * the really GLX-specific code is isolated in vr_visren.glx.{h,c}.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#ifndef __VRVISREN_H__
#define __VRVISREN_H__

#include <sys/types.h>
#include <unistd.h>

#include "vr_callback.h"
#include "vr_procs.h"
#include "vr_input.h"		/* for typedef of vr6sensor */
#include "vr_entity.h"		/* for typedef of vrEyeListInfo & vrEyeInfo */

#ifdef __cplusplus
extern "C" {
#endif

#define VR_MAXSTATS		5
#define VR_MAXPERSPDEPTH	8


/**************************************************************/
/* The four ways in which a window can be placed in the world */
typedef enum vrWindowMountType_en {
		VRWINDOW_FIXED,
		VRWINDOW_HEADMOUNT,
		VRWINDOW_HANDHELD,
		VRWINDOW_SIMULATOR
	} vrWindowMountType;


/****************************************************************************/
/* Array indices for different time measurement values */
#define	VR_TIME_INIT	 	0
#define	VR_TIME_FRAME	 	1
#define	VR_TIME_RENDER1	 	2
#define	VR_TIME_RENDER2	 	3
#define	VR_TIME_RENDERINFO	4
#define	VR_TIME_WAIT	 	5
#define	VR_TIME_SYNC	 	6
#define	VR_TIME_FREEZE	 	7
#define	VR_TIME_SWAP	 	8
#define	VR_TIME_TRAVEL	 	9


/****************************************************************************/
/* TODO: consider making these an enumerated type */
/*   NOTE: these are used by the vrSimulatorMove() function */
#define VR_SIMMOVE_NOP		 0
#define VR_SIMMOVE_AWAY		 1
#define VR_SIMMOVE_TOWARD	 2
#define	VR_SIMMOVE_CENTER	 3
#define	VR_SIMMOVE_SETHOME	 4
#define	VR_SIMMOVE_GOHOME	 5
#define	VR_SIMMOVE_GOHEAD	 6
#define	VR_SIMMOVE_POSY		 7
#define	VR_SIMMOVE_NEGY		 8
#define	VR_SIMMOVE_POSX		 9
#define	VR_SIMMOVE_NEGX		10
#define	VR_SIMMOVE_TOGGLEMASK	11


/********************************************************************************/
/* A rectangle viewport for specifying where a window viewport should be placed */
typedef struct {
		int		origX;
		int		origY;
		int		width;
		int		height;
	} vrViewport;


/****************************************************************************/
/* A rectangle *fractional* viewport for specifying in which subregion of a */
/*   window the viewport should be placed                                   */
typedef struct {
		float		min_X;
		float		max_X;
		float		min_Y;
		float		max_Y;
	} vrViewportFract;


/*************************************************************************************/
/* (currently unused) some data that indicates what part of the viewport to mask off */
typedef struct {
		void		*data;
	} vrViewportMask;


/***************************************************************/
/* the data used to calculate the perspective rendering matrix */
#define	USE_FRUSTUMEYE	/* should be defined -- undef to have FreeVR calc 4x4 persp matrix ([01/27/2009: which doesn't seem to work at the moment!]) */
typedef struct {
#ifdef USE_FRUSTUMEYE
		union	{
			double		v[6];		/* access to frustum values as an array (vector) */
			vrFrustum	n;		/* access to frustum values by name */
		} frustum;
#else
		vrMatrix		mat;		/* The hand calculated 4x4 perspective matrix */
#endif

		double			eye[3];		/* location of the eye from which the perspective is calculated */
		vrMatrix		rw2w_xform;	/* keep a copy of the real-world to window transform matrix */
		vrWindowMountType	mount;		/* keep a copy of the type of window to which we're rendering */
	} vrPerspData;


/***************************************************************************/
/* Fields marked as "CONFIG" are filled in direction from the config       */
/*   file.  Other fields are determined in combination with other factors. */
typedef struct vrWindowInfo_st {

		/*************************/
		/* Generic Object fields */
		vrObject	object_type;	/* The type of this object (ie. VROBJECT_WINDOW) */
		int		id;		/* numeric id for this window. */
		char		*name;		/* CONFIG: The name assigned to this window. */
		int		malleable;	/* CONFIG: Whether the data of this window can be modified */
	struct	vrWindowInfo_st	*next;		/* the next window in a linked list */
		vrContextInfo	*context;	/* pointer to the vrContext memory structure */
		int		line_created;	/* line number of where this object was created in config */
		int		line_lastmod;	/* line number of last time this object modified in config */
		char		file_created[512];/* file (or other) where this object was created*/
		char		file_lastmod[512];/* file (or other) where this object was created*/

		/******************************/
		/* Fields specific to Windows */
		char		*version;	/* The version of this window */
		vrProcessInfo	*proc;		/* Pointer to the process handling this window */
		void		*aux_data;	/* auxiliary data, specific to the type of window */
		void		*gfx_context;	/* an app-programmer setting for passing info from init to render */
		vrSystemSettings settings;	/* CONFIG: values for many inheritable fields */
		char		*graphics;	/* CONFIG: type of graphics renderer (eg. "GLX") */
		char		*args;		/* CONFIG: arguments for how this window should look/be rendered */

		vrWindowMountType mount;	/* CONFIG: The mount type of this window (eg. fixed, head-based) */
		int		num;		/* the number of this window in running system (1-based) */
		int		dualeye_buffer;	/* Boolean that indicates whether window has LEFT & RIGHT eye buffers */
		vrVisrenModeType visrenmode;	/* The type of visual rendering mode selected */
		int		num_eyes;	/* CONFIG: The number of eyes for this window */
		vrEyelistInfo	*eyelist;	/* Pointer to the eyelist given by settings.eyelist_name */
		vrEyeInfo	**eyes;		/* Array of pointers to the eyes rendered by this window */
		vrUserInfo	*sim_user;	/* Pointer to the user that simulator view is tethered to */
		double		coords_ll[3];	/* CONFIG: The location of the lower-left corner of the window.   */
						/*   (relative to fixed, head, hand movement based on mount type. */
		double		coords_lr[3];	/* CONFIG: The location of the lower-right corner of the window.  */
		double		coords_ul[3];	/* CONFIG: The location of the upper-left corner of the window.   */
		vrPoint		Lpoint;		/* lower left corner point of window */
		vrPoint		Hpoint;		/* upper right corner point of window */
		double		H_minus_L[2];	/* values calculated based on "coords" fields  */
		double		H_plus_L[2];	/* values calculated based on "coords" fields  */
		double		HminusL_recip[2];/* values calculated based on "coords" fields */
		vrMatrix	*e2w_xform;	/* A matrix that maps eye coords to window coords */
		vrMatrix	*rw2w_xform;	/* A matrix that maps real-world coordinates to window coords */
		vrMatrix	*rw2w_homexform;/* Home rw2w matrix for simulator view */
		vrViewport	geometry;	/* CONFIG: The window's geometry on the window-manager. */
		vrViewport	viewport_left;	/* CONFIG: left viewport in OpenGL parameters */
		vrViewport	viewport_right;	/* CONFIG: right viewport in OpenGL parameters */
		vrViewportFract	viewportF_left;	/* CONFIG: fractional viewport for left view for dualvp mode */
		vrViewportFract	viewportF_right;/* CONFIG: fractional viewport for right view for dualvp mode */
		vrViewportMask	viewport_mask;	/* CONFIG: TODO: A viewport mask to constrain rendering in the window. */
		int		simulator_mask;	/* Changeable/CONFIG: a bitmask indicating what simulator parts to display. */
		int		frontrendermode;/* A flag that allows the library to alter how the application is rendered (for front-facing polys) [NOTE: type is really GLenum, but we don't want to require the inclusion of gl.h here.]*/
		int		backrendermode;	/* A flag that allows the library to alter how the application is rendered (for back-facing polys) [NOTE: type is really GLenum, but we don't want to require the inclusion of gl.h here.]*/

		int		sim_follow_head;/* Changeable: A flag indicating whether a simulator window should continuously change its view to follow a user's head */
		int		world_show;	/* Changeable: A flag indicating whether to render the virtual world -- only in effect in simulator windows */
		int		ui_show;	/* Changeable/CONFIG: A flag indicating to put user-interface info on window */
		float		ui_loc[2];	/* Changeable/CONFIG: where on window to put user-interface info */
		float		ui_color[3];	/* Changeable/CONFIG: color to display the user-interface info */
		int		fps_show;	/* Changeable/CONFIG: A flag indicating to put fps on window. */
		float		fps_loc[2];	/* Changeable/CONFIG: where on window to put fps info */
		float		fps_color[3];	/* Changeable/CONFIG: color to display the fps data */
		int		stats_show;	/* Changeable/CONFIG: A flag/mask indicating what stats to put on window. */
		char		*stats_procs;	/* CONFIG: a string of which processes to show stats */
		vrProcessStats	**stats[VR_MAXSTATS];/* array of pointers to pointers to stats information from stats_procs */
		int		inputs_show;	/* Changeable/CONFIG: A flag indicating whether to show input histories */

		int		show_in_simulator;/* Changeable/CONFIG: A flag/mask indicating whether to show this window in the simulator view.  (NOTE: this is unique in that it affects what happens in OTHER windows) */

		/********************************/
		/* Callbacks for what to render */
		int		call_visreninit;/* Flag to indicate whether the current VisrenInit callback has been called for this window */
		vrCallback	*VisrenInit;	/* function to initialize the graphics in a rendering window */
		vrCallback	*VisrenFrame;	/* function to set state information for a rendering frame */
		vrCallback	*VisrenWorld;	/* function to render the virtual world scene (called per eye) */
		vrCallback	*VisrenSim;	/* function to render the simulator view */
		vrCallback	*VisrenExit;	/* function to restore any graphics state prior to exiting */

		/*******************************/
		/* Callbacks for how to render */
		/* TODO: consider whether these should be system-wide config callbacks, not per-window callbacks. */
		vrCallback	*PrintAux;	/* function to print the window's auxiliary data */
		vrCallback	*PreOpenInit;	/* callback for graphics systems that require info before opening windows (eg. pf) */
		vrCallback	*Open;		/* function to open the rendering window */
		vrCallback	*Close;		/* function to close the rendering window */
		vrCallback	*Swap;		/* function to swap front and back graphics buffers */
		vrCallback	*Errors;	/* function to report any errors from the graphics system */
		vrCallback	*RenderTransform;/* function to make a 3D transformation */
		vrCallback	*SetProjectionTransform;/* function to make set the top of the projection stack */
		vrCallback	*Render;	/* function to render the scene */
		vrCallback	*RenderText;	/* function to render text at the current 3D location */
		vrCallback	*RenderNullWorld;/* function to render an empty world */
		vrCallback	*RenderSimulator;/* function to render the simulated VR hardware */

	} vrWindowInfo;


/*******************************************************/
/* type definition for data passed to each render call */
typedef struct {
		vrContextInfo	*context;	/* Pointer to overall system context        */
		vrPerspData	*persp;		/* Pointer to how to render this frame      */
		vrPerspData	persp_stack[VR_MAXPERSPDEPTH];	/* A "stack" (array) of perspective structs */
		int		persp_stack_depth; /* Depth of the perspective stack        */
		vrWindowInfo	*window;	/* Pointer to window rendering into         */
		vrEyeInfo	*eye;		/* Pointer to eye rendering to              */
		vrMatrix	*rw2vw_xform;	/* transform from real to virtual CoordSys  */
		vrMatrix	*vw2rw_xform;	/* transform from virtual to real CoordSys  */
		long		frame_count;	/* This process' frame_count                */
		vrTime		frame_stime;	/* Sim-Time when this render frame began    */
		void		*gfx_context;	/* Generic pointer to graphics specific context */
	} vrRenderInfo;


/*****************************************************************/
/* vrVisrenOptsType: structure for adding new rendering options. */
typedef struct {
		char	*option_name;
		void	(*info_func)(vrWindowInfo *);
		void	(*nullworld_func)();
		void	(*defsim_func)();
		void	(*rentext_func)(char *);
	} vrVisrenOptsType;


/*************************************************/
/*** Globally accessible function declarations ***/

void		vrFprintRenderInfo(FILE *file, vrRenderInfo *rendinfo, vrPrintStyle style);
void		vrFprintPerspData(FILE *file, vrPerspData *pd, vrPrintStyle style);

char		*vrWindowName(vrWindowMountType type);
vrWindowMountType vrWindowType(char *name);
char		*vrVisrenModeName(vrVisrenModeType mode);
#if 0 /* this should work */
vrVisrenModeType vrVisrenModeValue(char *modename);
#else
int		 vrVisrenModeValue(char *modename);
#endif

void		vrWindowClear(vrWindowInfo *object);
void		vrWindowCopy(vrWindowInfo *dest_object, vrWindowInfo *src_object);
void		vrFprintWindowInfo(FILE *, vrWindowInfo *, vrPrintStyle style);

vrWindowInfo	*vrRenderCurrentWindow(vrRenderInfo *renderinfo);
vrUserInfo	*vrRenderCurrentUser(vrRenderInfo *renderinfo);
vrEyeInfo	*vrRenderCurrentEye(vrRenderInfo *renderinfo);
vrTime		vrRenderCurrentSimTime(vrRenderInfo *renderinfo);
long		vrRenderCurrentFrame(vrRenderInfo *renderinfo);
void		vrRenderNullWorld(vrRenderInfo *renderinfo);
void		vrRenderDefaultSimulator(vrRenderInfo *renderinfo, int mask);
void		vrRenderText(vrRenderInfo *renderinfo, char *string);
void		vrRenderSetWindowContext(vrRenderInfo *renderinfo, void *gfx_context);
void		*vrRenderGetWindowContext(vrRenderInfo *renderinfo);
void		vrRenderTransform(vrRenderInfo *renderinfo, vrMatrix *mat);
void		vrRenderTransformWindow(vrRenderInfo *renderinfo);
void		vrRenderTransformUserTravel(vrRenderInfo *renderinfo);
void		vrRenderTransformUserTravelInv(vrRenderInfo *renderinfo);
int		vrRenderTransform6sensor(vrRenderInfo *renderinfo, int sensor6_num);
void		vrRenderTransform6sensorDirect(vrRenderInfo *renderinfo, vr6sensor *sensor6);
void		vrRenderSetProjectionTransform(vrRenderInfo *renderinfo, vrPerspData *pd);
int		vrRenderPushPerspFrom6sensor(vrRenderInfo *renderinfo, int sensor6_num);
int		vrRenderPopPersp(vrRenderInfo *renderinfo);
vrEuler		*vrRenderGetBillboardAngles3d(vrRenderInfo *renderinfo, vrEuler *eulers, double x_loc, double y_loc, double z_loc);
vrEuler		*vrRenderGetBillboardAnglesAd(vrRenderInfo *renderinfo, vrEuler *eulers, double *loc);
void		vrRenderCategory(vrRenderInfo *renderinfo, int tag);

void		vrVisrenMainLoop(vrProcessInfo *myproc_info);
void		vrVisrenGetInfo(vrWindowInfo *);
void		vrCalcPerspIntermediaries(vrWindowInfo *window);
#ifdef USE_FRUSTUMEYE
vrPerspData	*vrCalcPerspFrustumEye(vrPerspData *pd, vrWindowInfo *this_window, vrMatrix *eye_rwpos);
#else
vrPerspData	*vrCalcPerspMatrix(vrPerspData *pd, vrWindowInfo *this_window, vrMatrix *eye_rwpos);
#endif

void		vrSimulatorMove(vrWindowInfo *window, int command, void *args);
void		vrSimulatorMoveString(vrWindowInfo *window, char *strcommand);

#ifdef __cplusplus
}
#endif

#endif
