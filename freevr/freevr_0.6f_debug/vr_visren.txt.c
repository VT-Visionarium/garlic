/* ======================================================================
 *
 *  CCCCC          vr_visren.txt.c
 * CC   CC         Author(s): Bill Sherman
 * CC              Created: September 19, 2011 (based on vr_visren.glx.c)
 * CC   CC         Last Modified: September 19, 2011
 *  CCCCC
 *
 * Code file for FreeVR visual rendering into a text window.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/*************************************************************************
 
ABOUT:
	While unlikely to be used as part of a real VR application, the
	text rendering interface has a handful of useful applications in
	and of itself.  Primary among these is when FreeVR is being used
	as a basic input device server, and thus any visual rendering is
	superfluous -- and thus by avoiding it with a text rendering is
	beneficial.  But another important usage is as a means of testing
	and debugging on new systems for which the visual rendering component
	has yet to be developed (e.g.  MS-Windows).  Finally, it serves as
	a curiosity rendering technique (ASCII-Art) -- which could lead
	people to discover FreeVR, which is also a good thing!


FreeVR USAGE:
	Here are the FreeVR configuration options for the text visual rendering window:
		"display=<terminal shell>" -- set the terminal display where the rendering should appear

	Controls are specified in the freevrrc file:
	  :-(	e.g.: control "<control option>" = "switch2(button[{1|2|3|4|5|6|7|8|Star}])";

	Available control options are:
	  :-(	"print_help" -- print info on how to use the display device
	  :-(	"print_struct" -- print the internal Txt data structure (for debugging)
		-- simulator controls should be made to be control options --

TODO:
	- the whole thing

	- allow a choice of render styles:
		one-line (using carriage returns)
		full-window (ie. curses-like -- using ANSI-terminal commands)

**************************************************************************/
#include <string.h>
#include <math.h>			/* needed for HUGE_VAL (aka __infinity) definition */

#include "vr_visren.h"
#include "vr_visren.txt.h"
#include "vr_callback.h"		/* also included within vr_input.h, but left for clarity */
#include "vr_input.h"
#include "vr_debug.h"
#include "vr_utils.h"			/* for declarations of vrStringCharWidth() & vrStringCharHeight() */

#if 0 /* set to 1 to prevent local vrTrace messages */
#  ifdef vrTrace
#    undef	vrTrace
#    define	vrTrace(a,b) ;
#  endif
#endif


/*****************************************************************/
void vrFprintTxtPrivateInfo(FILE *file, vrTxtPrivateInfo *aux, vrPrintStyle style)
{
	/* TODO: print different things for different styles */

	/* if null pointer given, print an empty shell and return */
	if (aux == NULL) {
		vrFprintf(file, "Txtinfo \"(nil)\" = { }\n");
		return;
	}

	switch (style) {
	case one_line:
#ifdef NYI
		vrFprintf(file, "TxtInfo = { xhost = \"%s\" xserver = %d xscreen = %d Display = %#p Window = %#p }\n",
			aux->xhost,
			aux->xserver,
			aux->xscreen,
			aux->xdisplay,
			aux->xwindow);
#endif
		break;

	default:
	case verbose:
#ifdef NYI
		vrFprintf(file, "TxtInfo (%#p) = {\n", aux);
		vrFprintf(file, "\r"
			"\txhost = \"%s\"\n\txserver = %d\n\txscreen = %d\n"
			"\tDisplay = %#p\n\tWindow = %#p\n\tXvisual = %#p\n"
			"\tXcolormap = %#p\n\tTxtContext = 0x%x\n"
			"\tscreen_size = %dx%d\n\tdecorations = 0x%x\n\n"
			"\tmapped = %d\n\tused_by_input = %d\n\tstereo buffer = %d\n\tdouble buffer = %d\n",
			aux->xhost,
			aux->xserver,
			aux->xscreen,
			aux->xdisplay,
			aux->xwindow,
			aux->xvisual,
			aux->xcolormap,
			aux->Txt_context,
			aux->xscreen_size_x,
			aux->xscreen_size_y,
			aux->decorations,
			aux->mapped,
			aux->used_by_input,
			aux->stereo_buf,
			aux->doub_buf);
		vrFprintf(file, "\r}  /* TODO: add more Txt details */\n");
#endif

		/* TODO: the rest of the fields */
		break;
	}
}


/****************************************************************************/
static void _TxtParseArgs(vrTxtPrivateInfo *aux, char *args)
{
	char	*str = NULL;

	/* TODO: see how much of the rest of this parsing stuff can be made generic */

	/**********************************************************/
	/** Argument format: "display=" <string> [(";" | ",")] **/
	/**********************************************************/
	/* where <string> is a string that will be used for the */
	/*   window title message.                              */

	/* set the title string */
	if (str = strstr(args, "display=")) {
		char	*title_str = strchr(str, '=') + 1;
		char	*tok;

		tok = strtok(title_str, ",;");
		aux->window_display = vrShmemStrDup(tok);
	}

	vrDbgPrintf("================================================\n");
	vrDbgPrintf("done parsing argument string `%s'\n", args);
	vrDbgPrintf("aux->window_display = '%s'\n", aux->window_display);
	vrDbgPrintf("================================================\n");
}



/* The opening and initialization function -- this line is to help diff match */
/**************************************************************************/
/* _TxtOpenFunc() initializes the auxiliary window info for the specified */
/* window.                                                                */
/**************************************************************************/
static void _TxtOpenFunc(vrWindowInfo *window)
{
static	char		trace_msg[256];
	vrTxtPrivateInfo *aux = NULL;
	int		dummy = 0;

	sprintf(trace_msg, BOLD_TEXT "Entering for window '%s' %#p\n" NORM_TEXT, window->name, window);
	vrTrace("_TxtOpenFunc", trace_msg);


	/****************************************/
	/*** initialize memory for the window ***/
	/****************************************/

	window->aux_data = (void *)vrShmemAlloc0(sizeof(vrTxtPrivateInfo));
	aux = (vrTxtPrivateInfo *)window->aux_data;


	/*************************************/
	/*** set/get the window parameters ***/
	/*************************************/

	/* set some default values */
	aux->window_display = (char *)vrShmemAlloc(128);
	sprintf(aux->window_display, "/dev/tty");
	aux->window_fp = NULL;

	/* parse the arguments to override defaults            */
	/*   this will fill in aux->xserver, aux->xscreen, etc. */
	_TxtParseArgs(aux, window->args);
	vrTrace("_TxtOpenFunc", "Arguments parsed");


	/******************************************/
	/*** open connection to terminal window ***/
	/******************************************/

	aux->window_fp = fopen(aux->window_display, "r+");
vrPrintf("aux->window_display = '%s', aux->window_fp = %p\n", aux->window_display, aux->window_fp);
	sprintf(trace_msg, "Text display '%s' has been opened.", aux->window_display);
	vrTrace("_TxtOpenFunc", trace_msg);

	/******************************/
	/* handle stereo visuals here */

	/**************************/
	/* check color style here */

	vrTrace("_TxtOpenFunc", BOLD_TEXT "about to create a rendering context" NORM_TEXT);

	/***************************************************************/
	/* open a gl rendering context w/ no display-list sharing etc. */

	/**************************************/
	/* create a colormap for the X window */

	/********************************/
	/* create a pixmap for the icon */

	/************************************/
	/* create a cursor for the X window */

	/***********************************************************/
	/* set some window attributes prior to creating the window */

	/**************************/
	/*** create the window! ***/
	/**************************/

	/****************************************************/
	/* set some window manager look and feel properties */

	/************************************************/
	/* set standard window properties and pop it up */

	/*************************************************/
	/* when the window has been successfully mapped  */
	/*   to the X-screen, set the flag and continue. */
	aux->mapped = 1;


	/**********************************************************/
	/*** Configure this Text terminal context for rendering ***/
	/**********************************************************/

	/* TODO: perhaps "clear the window" here */

	vrTrace("_TxtOpenFunc", "about to make Txt calls");

	sprintf(trace_msg, BOLD_TEXT "Exiting _TxtOpenFunc() for window '%s' %#p\n" NORM_TEXT, window->name, window);
	vrTrace("_TxtOpenFunc", trace_msg);
}
/* end of open function -- for diff match */


/****************************************************************************/
/* TODO: put the cursor back at the bottom of the screen? */
static void _TxtCloseFunc(vrWindowInfo *window)
{
	vrTxtPrivateInfo	*aux = (vrTxtPrivateInfo *)(window->aux_data);

	/* TODO: should we also free memory of the things aux points to? */
	vrShmemFree(aux);
}


/****************************************************************************/
static void _TxtSwapFunc(vrWindowInfo *info)
{
	vrTxtPrivateInfo	*aux = (vrTxtPrivateInfo *)(info->aux_data);

	/* If this window hasn't been mapped yet, then skip the swap */
	if (!aux->mapped)
		return;

	/* TODO: this function -- some sort of flushing at least */
}


/****************************************************************************/
/* A function to check the rendering library error flag(s) and report any   */
/*   error conditions.                                                      */
static void _TxtRenderErrors()
{
#ifdef NYI /* { */
	GLenum	error_code;

	while ((error_code = glGetError()) != GL_NO_ERROR) {
		vrPrintf("_GlxRenderErrors(): " RED_TEXT" a GL error of type '%s' (%d) has occurred.\n" NORM_TEXT,
			_GlxGLErrorCodeString(error_code),
			(int)error_code);
	}
#endif /* } NYI */
}


/****************************************************************************/
static void _TxtRenderTransform(vrMatrix *mat)
{
#ifdef NYI
	glMultMatrixd(mat->v);
#endif
}


/****************************************************************************/
/* _TxtSetProjectionTransform() allows the top value of the GL projection   */
/*   stack to be set to the corresponding values in the vrPerspData         */
/*   structure.  The pushing/popping of this data is left to the calling    */
/*   function(s) to handle.                                                 */
/* NOTE: this is primarily used for the feature of allowing a 2nd POV to    */
/*   be rendered to the screen for particular elements in the virtual world.*/
static void _TxtSetProjectionTransform(vrPerspData *pd)
{

#ifdef NYI
	/* 6/18/01: by putting the viewing transformation on the projection-matrix */
	/*   stack, lighting and environment maps should work.  However, to have   */
	/*   fog effects work the viewing transform needs to go on the ModelView   */
	/*   stack instead.                                                        */

	/* TODO: there should be be an option that allows the application to */
	/*   choose to put the perspective matrix on the ModelView stack in  */
	/*   case it prefers to have working fog over working lighting.      */

	glMatrixMode(GL_PROJECTION);

#ifdef USE_FRUSTUMEYE
	glLoadIdentity();
	if (pd->frustum.n.left != -HUGE_VAL) {
		glFrustum(pd->frustum.n.left,
			pd->frustum.n.right,
			pd->frustum.n.bottom,
			pd->frustum.n.top,
			pd->frustum.n.near_clip,
			pd->frustum.n.far_clip);
	} else {
		vrDbgPrintfN(TXT_DBGLVL, "_TxtRenderFunc(): " RED_TEXT "Invalid Frustum, viewpoint on render plane\n" NORM_TEXT);
		glFrustum(-1.0, 1.0, -1.0, 1.0, 0.1, 10.0);	/* NOTE: this is of course, wrong, but at least puts something on the screen. */
	}
#else
	if (!VRMAT_ROWCOL(&pd->mat, VR_W, VR_W) == 0.0) {
		glLoadMatrixd(pd->mat.v);
	} else {
		vrDbgPrintfN(TXT_DBGLVL, "_TxtRenderFunc(): " RED_TEXT "Invalid Perspective Matrix, viewpoint on render plane\n" NORM_TEXT);
		glFrustum(-1.0, 1.0, -1.0, 1.0, 0.1, 10.0);	/* NOTE: this is of course, wrong, but at least puts something on the screen. */
	}
#endif

#ifdef USE_FRUSTUMEYE
	/* NOTE: Stuart says the real-world to window transform should be  */
	/*   on the ModelView stack in order to get proper lighting, etc.  */
	/* TODO: provide an option to do the eye & rw2s transforms on the  */
	/*   perspective stack.                                            */
	/* TODO: test the difference in stack choice on the VRtut lighting */
	/*   solution.                                                     */
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslated(-pd->eye[VR_X], -pd->eye[VR_Y], -pd->eye[VR_Z]);
	glMultMatrixd(pd->rw2w_xform.v);
#else
	/* BS: (TODO:) should we go into ModelView mode first and then do the transformation? */
	glMultMatrixd(pd->rw2w_xform.v);
	glMatrixMode(GL_MODELVIEW);
	/* BS: (TODO:) should there be a glLoadIdentity() after going into MODELVIEW matrix mode? */
#endif
#endif /* NYI */

}


/* The render function */
/****************************************************************************/
static void _TxtRenderFunc(vrRenderInfo *renderinfo)
{
static	int		nocallback_msg = 0;	/* has the no rendering callback message been displayed yet? */
static	char		trace_msg[2048];
static	char		render_string[2048];
	char		*buffer_name;		/* name of the GL buffer selected */
	vrPerspData	*pd = renderinfo->persp;
	vrWindowInfo	*curr_window = renderinfo->window;
	vrEyeInfo	*curr_eye = renderinfo->eye;
	vrUserInfo	*curr_user = curr_eye->user;
	vrTxtPrivateInfo *aux = (vrTxtPrivateInfo *)curr_window->aux_data;
	vrCallback	*callback = NULL;
	int		count;
	int		twod_ortho_mode = 0;	/* set to one when rendering in 2D */

	if (curr_window->name == NULL) {
		printf("_TxtRenderFunc(): something is seriously wrong -- no window name.\n");
		return;
	}
	sprintf(trace_msg, "beginning window render loop for window '%s' %#p", curr_window->name, curr_window);
	vrTrace("_TxtRenderFunc", trace_msg);

	if (!aux->mapped) {
		vrTrace("_TxtRenderFunc", "unmapped window -- skip rendering");
		return;
	}

#ifdef NYI
	TxtMakeCurrent(aux->xdisplay, aux->xwindow, aux->Txt_context);
#endif


	/****************************/
	/* TODO: get input events, if any ... */
#ifdef NYI
	while (XPending(aux->xdisplay)) {
		XNextEvent(aux->xdisplay, &event);

		switch (event.type) {

		case ConfigureNotify:
			/* Update the viewport */
			glViewport((GLint)0, (GLint)0, (GLsizei)event.xconfigure.width, (GLsizei)event.xconfigure.height);
			curr_window->geometry.origX = event.xconfigure.x;
			curr_window->geometry.origY = event.xconfigure.y;
			curr_window->geometry.width = event.xconfigure.width;
			curr_window->geometry.height = event.xconfigure.height;

			/* re-calculate the absolute viewport in OpenGL terms from the fractional viewports */
			curr_window->viewport_left.origX =  (int)(curr_window->geometry.width  *  curr_window->viewportF_left.min_X);
			curr_window->viewport_left.width =  (int)(curr_window->geometry.width  * (curr_window->viewportF_left.max_X - curr_window->viewportF_left.min_X));
			curr_window->viewport_left.origY =  (int)(curr_window->geometry.height *  curr_window->viewportF_left.min_Y);
			curr_window->viewport_left.height = (int)(curr_window->geometry.height * (curr_window->viewportF_left.max_Y - curr_window->viewportF_left.min_Y));

			curr_window->viewport_right.origX =  (int)(curr_window->geometry.width  *  curr_window->viewportF_right.min_X);
			curr_window->viewport_right.width =  (int)(curr_window->geometry.width  * (curr_window->viewportF_right.max_X - curr_window->viewportF_right.min_X));
			curr_window->viewport_right.origY =  (int)(curr_window->geometry.height *  curr_window->viewportF_right.min_Y);
			curr_window->viewport_right.height = (int)(curr_window->geometry.height * (curr_window->viewportF_right.max_Y - curr_window->viewportF_right.min_Y));

			vrDbgPrintfN(TXT_DBGLVL, "_TxtRenderFunc(): Got a reconfiguration event.  Setting window %#p viewport to (%d, %d) (%dx%d).\n",
				curr_window,
				curr_window->geometry.origX, curr_window->geometry.origY,
				curr_window->geometry.width, curr_window->geometry.height);
			vrDbgPrintfN(TXT_DBGLVL, "viewport_left = %d %d %d %d\n",
				curr_window->viewport_left.origX,
				curr_window->viewport_left.origY,
				curr_window->viewport_left.width,
				curr_window->viewport_left.height);
			vrDbgPrintfN(TXT_DBGLVL, "viewport_right = %d %d %d %d\n",
				curr_window->viewport_right.origX,
				curr_window->viewport_right.origY,
				curr_window->viewport_right.width,
				curr_window->viewport_right.height);

			break;

		case KeyPress: {
			/* TODO: this keysym lookup code should take into account the current modifier states */
			KeySym	key = XLookupKeysym((XKeyEvent *)&event, 0);
			int	key_used_by_sim = 1;

#if 0 /* print each keypress received for debugging */
vrPrintf("_TxtRenderFunc(): Got a key press of type %X\n", XLookupKeysym((XKeyEvent *)&event, 0));
#endif
			vrDbgPrintfN(XWIN_DBGLVL, "_TxtRenderFunc(): Got a key press of type %X\n", XLookupKeysym((XKeyEvent *)&event, 0));

			if (curr_window->mount == VRWINDOW_SIMULATOR) {
				switch (key) {

				case XK_KP_Add:		/* move away from origin */
					vrSimulatorMove(curr_window, VR_SIMMOVE_AWAY, NULL);
					break;

#ifdef __sun
				case XK_F24:		/* move toward origin */
#endif
				case XK_KP_Subtract:	/* move toward origin */
					vrSimulatorMove(curr_window, VR_SIMMOVE_TOWARD, NULL);
					break;

#ifdef __sun
				case XK_F31:		/* reset to center position */
#endif
				case XK_KP_Begin:	/* reset to center position */
					vrSimulatorMove(curr_window, VR_SIMMOVE_CENTER, NULL);
					break;

				case XK_KP_Enter:	/* set the home position */
					vrSimulatorMove(curr_window, VR_SIMMOVE_SETHOME, NULL);
					break;

#ifdef __sun
				case XK_F27:		/* jump to the home position */
#endif
#ifdef __APPLE__ /* TODO: consider allowing this for all versions, not just Apple Macintosh's */
				case XK_KP_7:
#endif
				case XK_KP_Home:	/* jump to the home position */
					vrSimulatorMove(curr_window, VR_SIMMOVE_GOHOME, NULL);
					break;

				case XK_F11:		/* toggle whether to continuously jump to the user's head/eye position */
					curr_window->sim_follow_head ^= 1;
					vrPrintf("sim_follow_head flag for window '%s' is now %d\n", curr_window->name, curr_window->sim_follow_head);
					break;

				case XK_F12:		/* jump to the user's head/eye position */
#if 0 /* example code */
					vrSimulatorMove(curr_window, VR_SIMMOVE_CENTER, NULL);
					vrMatrixGet6sensorValuesDirect(&dstmat, curr_user->head)->v);
					memcpy(curr_window->rw2w_xform, curr_user->head->visren_position, sizeof(*curr_user->head->visren_position));

					vrMatrixSetIdentity(curr_window->rw2w_xform);
					vrMatrixPreMult(curr_window->rw2w_xform, curr_user->head->visren_position);
#endif
#if 1
					/* TODO: try to convert this into a vrSimulatorMove() operation */
					vrMatrixInvert(curr_window->rw2w_xform, curr_user->head->visren_position);
					vrMatrixPostTranslate3d(curr_window->rw2w_xform, pd->eye[VR_X], pd->eye[VR_Y], pd->eye[VR_Z]);
#else
					vrSimulatorMove(curr_window, VR_SIMMOVE_GOHEAD, NULL);		/* TODO: at some point we may want to be able to select which user's position is used */
#endif
					break;

/* TODO: Suns generate the same key codes for the KP directions as the regular arrows */
#ifdef __APPLE__ /* TODO: consider allowing this for all versions, not just Apple Macintosh's */
				case XK_KP_4:
#endif
				case XK_KP_Left:	/* rotate about Y axis */
					vrSimulatorMove(curr_window, VR_SIMMOVE_POSY, NULL);
					break;

#ifdef __APPLE__ /* TODO: consider allowing this for all versions, not just Apple Macintosh's */
				case XK_KP_6:
#endif
				case XK_KP_Right:	/* rotate about Y axis */
					vrSimulatorMove(curr_window, VR_SIMMOVE_NEGY, NULL);
					break;

#ifdef __APPLE__ /* TODO: consider allowing this for all versions, not just Apple Macintosh's */
				case XK_KP_8:
#endif
				case XK_KP_Up:		/* rotate about X axis */
					vrSimulatorMove(curr_window, VR_SIMMOVE_POSX, NULL);
					break;

#ifdef __APPLE__ /* TODO: consider allowing this for all versions, not just Apple Macintosh's */
				case XK_KP_2:
#endif
				case XK_KP_Down:	/* rotate about X axis */
					vrSimulatorMove(curr_window, VR_SIMMOVE_NEGX, NULL);
					break;

				/*******************************/
				/* some non-movement functions */
				case XK_KP_Insert:	/* toggle the simulator_mask "flag" */
					vrSimulatorMove(curr_window, VR_SIMMOVE_TOGGLEMASK, NULL);
					break;

				case XK_KP_End:		/* toggle the render-the-world flag -- it's the rend of the world or not show it */
					curr_window->world_show ^= 1;
					vrPrintf("world_show flag for window '%s' is now %d\n", curr_window->name, curr_window->world_show);
					break;

				default:
					key_used_by_sim = 0;
				}
			} /* end of if this window is a simulator code */

			/* now handle the non-simulator related keyboard input commands */
			switch (key) {

			/***************************************************/
			/* some non-movement functions                     */
			/* (which are not restricted to simulator windows) */
#ifdef __sun
			case XK_F26:		/* toggle the fps_show flag */
#endif
			case XK_KP_Multiply:	/* toggle the fps_show flag */
				curr_window->fps_show ^= 1;
				break;

#ifdef __sun
			/* TODO: figure out a good sun key */
#endif
#ifdef __APPLE__ /* TODO: consider allowing this for all versions, not just Apple Macintosh's */
			case XK_KP_9:
#endif
			case XK_KP_Page_Up:	/* toggle the stats_show flag */
				curr_window->stats_show ^= 1;
				break;

#ifdef __sun
			/* TODO: figure out a good sun key */
#endif
#ifdef __APPLE__ /* TODO: consider allowing this for all versions, not just Apple Macintosh's */
			/* TODO: figure out a corresponding Apple key */
#endif
			case XK_KP_Delete:	/* toggle the user-interface ("help") display */
				curr_window->ui_show ^= 1;

#if 1 /* TODO: remove this -- for testing, prints ui info to text output of window */
				vrSprintInputUI(trace_msg, renderinfo->context->input, sizeof(trace_msg), verbose);
				vrFprintf(stdout, "=================================================================\n");
				vrFprintf(stdout, trace_msg);
				vrFprintf(stdout, "=================================================================\n");
#endif
				break;

			case XK_KP_3:
			case XK_KP_Page_Down:	/* toggle the user-inputs display */
				curr_window->inputs_show ^= 1;
				break;

#ifdef __sun
			case XK_F25:		/* toggle whether to display current window's outline in simulator */
#endif
			case XK_KP_Divide:	/* toggle whether to display current window's outline in simulator */
				curr_window->show_in_simulator ^= 1;
				vrPrintf("show_in_simulator flag for window '%s' is now %d\n", curr_window->name, curr_window->show_in_simulator);
				break;

#if 1 /* NOTE: this is only for debugging the library */
			case XK_F10:		/* set the visreninit callback needs to be called flag */
				vrPrintf("_TxtRenderFunc() (For library debugging only): setting call_visreninit = 1 for '%s'\n", curr_window->name);
				curr_window->call_visreninit = 1;
				break;
#endif

			default:
				if (!key_used_by_sim)
					vrDbgPrintfN(INPUT_DBGLVL, "_TxtRenderFunc(): Got an unused key press of type %X\n", XLookupKeysym((XKeyEvent *)&event, 0));
				break;
			}
		}	break;

		case KeyRelease:
			break;

		case EnterNotify:
			/* I don't need to handle this, but it seems to be an event that is automatically */
			/*   included in the standard event mask, and I want to avoid the warning message,*/
			/*   thus the empty case.                                                         */
			vrDbgPrintfN(DEFAULT_DBGLVL, "_TxtRenderFunc(): got an 'EnterNotify' event for window '%s'\n", curr_window->name);
			break;

		case UnmapNotify:
			vrDbgPrintfN(DEFAULT_DBGLVL, "_TxtRenderFunc(): got an 'UnmapNotify' event for window '%s'\n", curr_window->name);
			break;

		case MapNotify:
			vrDbgPrintfN(DEFAULT_DBGLVL, "_TxtRenderFunc(): got an 'MapNotify' event for window '%s'\n", curr_window->name);
			break;

		default:
			vrDbgPrintfN(DEFAULT_DBGLVL, "_TxtRenderFunc(): got an unhandled X event of type %d\n", event.type);
			break;
		}
	} /* end while pending X events */
#endif /* NYI */

	/********************************************/
	/* Handle other possible simulator controls */
#ifdef NYI
	if (curr_window->mount == VRWINDOW_SIMULATOR) {

		/* when the flag is set, continually jump to the user's head position */
		if (curr_window->sim_follow_head) {
			/* TODO: try to convert this into a vrSimulatorMove() operation */
			vrMatrixInvert(curr_window->rw2w_xform, curr_user->head->visren_position);
			vrMatrixPreTranslate3d(curr_window->rw2w_xform, pd->eye[VR_X], pd->eye[VR_Y], pd->eye[VR_Z] + 0.0);
		}
	}
#endif /* NYI */


#if 1	/* don't include this if GFXINIT_TOP is not set to 2 in vr_visren.c */
	/*****************************************************/
	/* (0) call graphics initialization callback, if new */
	if (curr_window->call_visreninit == 1) {
		/* The initialization functions are only called once per window  */
		/*   per setting of the initialization function, so this is only */
		/*   done when a new VisrenInit callback was assigned.           */

		/* TODO: consider whether a per-user initialization routine */
		/*   is important.  This implementation does not refer to a */
		/*   per-user init callback -- because we don't know the    */
		/*   user until we get down to phase (3c).                  */
vrPrintf("_TxtRenderFunc(): " RED_TEXT "Calling VisrenInit callback for window '%s' -- call_visreninit was set to 1.\n" NORM_TEXT, curr_window->name);

		vrTrace("_TxtRenderFunc", "prep: initialization callback");
		callback = curr_window->VisrenInit;
		vrCallbackInvokeDynamic(callback, 1, renderinfo);
		curr_window->call_visreninit = 0;
		vrTrace("_TxtRenderFunc", "done: initialization callback");
	}
#endif


	/*****************************/
	/* (i) push gfx matrix/state */

	/* NOTE: attributes are not pushed to allow the application render */
	/*   routine to set the state in one call, and be able to expect   */
	/*   that state to be unchanging until it decides to change it.    */
	/* Perhaps we are now giving the same courtesy to the transformation matrix. */


	/*****************************************/
	/* (ii) handle viewport and frame buffer */
	vrTrace("_TxtRenderFunc", "(ii) handle viewport");

	/* set the buffer into which we should render (based on the eye) */
	switch (curr_eye->render_framebuffer) {
	default:
		/* Really, we shouldn't need to rely on the default, so print a warning message and continue. */
		vrMsgPrintf("By default rendering to the Back buffer\n");
	case VRFB_FULL:
#ifdef NYI
		glDrawBuffer(GL_BACK);
		buffer_name = "GL_BACK";
#endif /* NYI */
		break;
	case VRFB_LEFT:
#ifdef NYI
		glDrawBuffer(GL_BACK_LEFT);
		buffer_name = "GL_BACK_LEFT";
#endif /* NYI */
		break;
	case VRFB_RIGHT:
#ifdef NYI
		/* NOTE: dualeye_buffer gets set to 1 during the window initialization if a quad-buffer request was denied */
		if (curr_window->dualeye_buffer == 1) {
			glDrawBuffer(GL_BACK_RIGHT);
			buffer_name = "GL_BACK_RIGHT";
		} else {
			static	int	dualeye_warning = 0;

			if (dualeye_warning == 0) {
				vrDbgPrintfN(ALWAYS_DBGLVL, "_TxtRenderFunc(): " RED_TEXT "There is no stereo buffer for window '%s', so rendering to right-eye has been disabled.\n" NORM_TEXT, curr_window->name);
				dualeye_warning = 1;
			}

			sprintf(trace_msg, "premature ending window render loop for window '%s' %#p -- non-existant stereo buffer", curr_window->name, curr_window);
			vrTrace("_TxtRenderFunc", trace_msg);

			return;
		}
#endif /* NYI */
		break;

	/* NOTE: [5/5/02] We may want a special dualvp rendering mode      */
	/*   that is independent of specifically rendering a left or right */
	/*   eye view.                                                     */
	/* NOTE: [6/11/02: hmmm, now trying to use this as the only option, */
	/*   and for systems with just one eye to a window, the viewport    */
	/*   will take care of things.                                      */
	case VRFB_FULL_LEFTEYE:	  /* NOTE: this will just have a viewport encompassing the entire window */
	case VRFB_SPLIT_LEFTEYE:
#ifdef NYI
		glDrawBuffer(GL_BACK);
		buffer_name = "GL_BACK";
		glViewport(curr_window->viewport_left.origX, curr_window->viewport_left.origY,
			curr_window->viewport_left.width, curr_window->viewport_left.height);
		glScissor(curr_window->viewport_left.origX, curr_window->viewport_left.origY,
			curr_window->viewport_left.width, curr_window->viewport_left.height);
#endif /* NYI */
		break;
	case VRFB_FULL_RIGHTEYE:  /* NOTE: this will just have a viewport encompassing the entire window */
	case VRFB_SPLIT_RIGHTEYE:
#ifdef NYI
		glDrawBuffer(GL_BACK);
		buffer_name = "GL_BACK";
		glViewport(curr_window->viewport_right.origX, curr_window->viewport_right.origY,
			curr_window->viewport_right.width, curr_window->viewport_right.height);
		glScissor(curr_window->viewport_right.origX, curr_window->viewport_right.origY,
			curr_window->viewport_right.width, curr_window->viewport_right.height);
#endif /* NYI */
		break;
	}


	/******************************/
	/* (iii) handle color masking */
#ifdef NYI
#ifndef ENABLE_STENCIL_STEREO_TEST /* NOTE: this is a temporary change, replacing anaglyphic rendering with stencil checkerboard rendering for the TI DLP chip TVs such as the Samsung 61A750 */
	switch (curr_eye->color) {
	case VRANAGLYPH_ALL:
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		break;
	case VRANAGLYPH_RED:
		glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE);
		break;
	case VRANAGLYPH_GREEN:
		glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE);
		break;
	case VRANAGLYPH_BLUE:
		glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_TRUE);
		break;
	}
#else
	/* use the stencil buffer to render stereoscopic on DLP screens */
  {
#define STENCIL_MASK 0x01
static	int	stencil_initialized = 0;	/* flag to indicate whether the pattern array is established & other one-time operations */
	int	iindex, jindex;			/* counters for creating the checkerboard pattern */
	int	isize = 2000;			/* width of the screen */
	int	jsize = 1100;			/* height of the screen */
	int	checkersize = 1;		/* TODO: set checkersize to 1 for actual use */
	GLint	glintvalue;
static	char	*pattern;			/* the checkerboard pattern goes here -- TODO: make this based on screen size */

	if (!stencil_initialized) {

		/* a checkerboard pattern */
		/* TODO: setup for GL_BITMAP mode, but for now this is easier for testing */

		pattern = vrShmemAlloc(isize*jsize * sizeof(char));
		for (iindex = 0; iindex < isize; iindex++) {
			for (jindex = 0; jindex < jsize; jindex++) {
				if ((iindex/checkersize + jindex/checkersize) % 2)
					pattern[iindex + jindex*isize] = (char)(1);
				else	pattern[iindex + jindex*isize] = (char)(0);

			}
		}

		/* Setup the stencil stuff */
		glStencilMask(STENCIL_MASK);		/* using stencil mask bit-0 NOTE: I also tried 0x02, and that failed, I expected it to work. */
#if 1
		glEnable(GL_STENCIL_TEST);
#endif
		glClearStencil(0);		/* set the value of a stencil-clear */
		glGetIntegerv(GL_STENCIL_BITS, &glintvalue); printf("GL_STENCIL_BITS = %d\n", glintvalue);
		glGetIntegerv(GL_STENCIL_WRITEMASK, &glintvalue); printf("GL_STENCIL_WRITEMASK = %d\n", glintvalue);

		stencil_initialized = 1;
	}

	/* Okay, now draw into the stencil buffer -- must be done every frame */
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_FALSE);

	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
	glStencilFunc(GL_ALWAYS, 1, STENCIL_MASK);
	glClear(GL_STENCIL_BUFFER_BIT);		/* TODO: I doubt this is necessary */
	glDisable(GL_DEPTH_TEST);		/* TODO: determine whether this is necessary */
	glWindowPos2i(0, 0);
	glDrawPixels(isize, jsize, GL_STENCIL_INDEX, GL_BYTE, pattern);

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);		/* TODO: determine whether this is necessary */

	/* Now choose which form of the stencil to activate */
	switch (curr_eye->color) {
#if 1
	case VRANAGLYPH_ALL:
		glDisable(GL_STENCIL_TEST);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	/* TODO: this should probably be in the app frame function or something, needs to be figured out */
		break;
#endif
	case VRANAGLYPH_RED:
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glClear(GL_DEPTH_BUFFER_BIT);		/* TODO: is this the best way to do this?  I'm not sure I like doing the clear by the library, but how does the red-blue stuff work? */
#if 1
		glEnable(GL_STENCIL_TEST);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		glStencilFunc(GL_EQUAL, 1, STENCIL_MASK);
#endif
		break;
	/* treat green & blue the same */
	case VRANAGLYPH_GREEN:
	case VRANAGLYPH_BLUE:
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	/* TODO: this should probably be in the app frame function or something, needs to be figured out */
		glClear(GL_DEPTH_BUFFER_BIT);		/* TODO: is this the best way to do this?  I'm not sure I like doing the clear by the library, but how does the red-blue stuff work? */
#if 1
		glEnable(GL_STENCIL_TEST);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
#  if 0 /* Neither of these options seems to work -- but I think both should! */
		glStencilFunc(GL_NOTEQUAL, 1, STENCIL_MASK);
#  else
		glStencilFunc(GL_EQUAL, 0, STENCIL_MASK);
#  endif
#endif
		break;
	default:
printf("YO: default case, color = %d\n", curr_eye->color);
		break;
	}
#endif /* NYI */
  }
#endif


	/************************/
	/* (iv) handle viewmask */
	/* TODO: perhaps this task should be done after all the rendering is complete */

	/* TODO: this task */


	/*****************************************/
	/* (v) put transform matrix on the stack */
	vrTrace("_TxtRenderFunc", "(v) put transform matrix on the stack");

	/* 6/18/01: by putting the viewing transformation on the projection-matrix */
	/*   stack, lighting and environment maps should work.  However, to have   */
	/*   fog effects work the viewing transform needs to go on the ModelView   */
	/*   stack instead.                                                        */

	/* TODO: there should be be an option that allows the application to */
	/*   choose to put the perspective matrix on the ModelView stack in  */
	/*   case it prefers to have working fog over working lighting.      */

#ifdef NYI /* { */
	glPushMatrix(); /* { */
#endif /* } NYI */

#if 1 /* since I've just created this new function for an additional purpose, I thought I might also use it here. */
	_TxtSetProjectionTransform(pd);		/* this call puts the proper projection matrices at the top of the PROJECTION & MODELVIEW stacks */

#else /* { */
#ifdef NYI /* { */
	glMatrixMode(GL_PROJECTION);
#ifdef USE_FRUSTUMEYE
	glLoadIdentity();
	if (pd->frustum.n.left != -HUGE_VAL) {
		glFrustum(pd->frustum.n.left,
			pd->frustum.n.right,
			pd->frustum.n.bottom,
			pd->frustum.n.top,
			pd->frustum.n.near_clip,
			pd->frustum.n.far_clip);
	} else {
		vrDbgPrintfN(TXT_DBGLVL, "_TxtRenderFunc(): " RED_TEXT "Invalid Frustum, viewpoint on render plane\n" NORM_TEXT);
	}
#endif /* } NYI */
#else
	if (!VRMAT_ROWCOL(&pd->mat, VR_W, VR_W) == 0.0) {
#ifdef NYI /* { */
		glLoadMatrixd(pd->mat.v);
#endif /* } NYI */
	} else	vrDbgPrintfN(TXT_DBGLVL, "_TxtRenderFunc(): " RED_TEXT "Invalid Perspective Matrix, viewpoint on render plane\n" NORM_TEXT);
#endif

#ifdef NYI /* { */
#ifdef USE_FRUSTUMEYE
	/* NOTE: Stuart says the real-world to window transform should be */
	/*   on the ModelView stack in order to get proper lighting, etc. */
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslated(-pd->eye[VR_X], -pd->eye[VR_Y], -pd->eye[VR_Z]);
	glMultMatrixd(curr_window->rw2w_xform->v);
#else
	glMultMatrixd(curr_window->rw2w_xform->v);
	glMatrixMode(GL_MODELVIEW);
#endif
#endif /* } NYI */
#endif /* } */

	/**************************/
	/* (vi) render the world  */
      if (curr_window->world_show || curr_window->mount != VRWINDOW_SIMULATOR) {	/* NOTE: world rendering can only be disabled in simulator windows */
	vrTrace("_TxtRenderFunc", "(vi) render the world");

/* NOTE: if this is included, then the GLX rendering routine is called, and oddly it doesn't die! */
/* CONTINUE: we need to only set callbacks when the graphics type matches that of the rendering routine. */
	/* set the world rendering callback.  Start with the user's callback because that takes precedence. */
	callback = curr_user->VisrenWorld;
	if (!callback)
		callback = curr_window->VisrenWorld;
	if (!callback) {
		if (nocallback_msg == 0) {
			vrErrPrintf("_TxtRenderFunc(): no callback for rendering the world available\n");
			nocallback_msg = 1;
		}
	}

#ifdef NYI /* { */
	/* if a non-default rendermode is set, then set it */
	/* TODO: WARNING -- does this mean there is GLX code inside the vanilla visren code? */
	if (curr_window->frontrendermode != GL_NONE) {
		glPolygonMode(GL_FRONT, curr_window->frontrendermode);
	}
	if (curr_window->backrendermode != GL_NONE) {
		glPolygonMode(GL_BACK, curr_window->backrendermode);
	}
#endif /* } NYI */

#ifdef NYI /* { */
	glPushMatrix(); /* { */
#endif /* } NYI */
	vrTrace("_TxtRenderFunc", "(vi) invoking the callback");
#ifdef NYI /* { */
	vrCallbackInvokeDynamic(callback, 1, renderinfo);
#else
	/* some form of text rendering */
	sprintf(render_string, "\rFPS: %5.2f(1) %5.2f(10)", curr_window->proc->fps1, curr_window->proc->fps10);
	fprintf(aux->window_fp, render_string);
#endif /* } NYI */

#ifdef NYI /* { */
	glPopMatrix(); /* } */
#endif /* } NYI */

	/* TODO: check whether GL is in an error state (or has set */
	/*   the error flag), and print a warning if it has.       */
	/*   Probably best (timewise) to only do this when at a    */
	/*   sufficiently high debug level the default is probably */
	/*   good.                                                 */
	/* Hmmmm, why is the error check being done in the _TxtRenderText() routine?  Did I put it in the wrong place? */
	/*   Looking back, I think what happened is that Ed put the error calls int _TxtRenderText() in order to debug */
	/*   that routine, and then they got left there.  Currently, the call is made in phase 1 of the rendering in   */
	/*   vr_visren.c, just before the framebuffer swap.  NOTE: it's called via a callback to the _TxtRenderErrors()*/
	/*   function contained here in this file.  So, ideally, that should be sufficient -- having it there also     */
	/*   covers possible rendering errors in FreeVR, but I should see and solve those right away.                  */

	/* if a non-default rendermode is set, then restore to fill-mode */
#ifdef NYI /* { */
	if (curr_window->frontrendermode != GL_NONE) {
		glPolygonMode(GL_FRONT, GL_FILL);
	}
	if (curr_window->backrendermode != GL_NONE) {
		glPolygonMode(GL_BACK, GL_FILL);
	}
#endif /* } NYI */
       } else {
		/* If we don't render the world, then we at least need to clear the screen */
#ifdef NYI /* { */
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif /* } NYI */
       }

	/* measure: time just spent in rendering each eye (not counting the simulator and information displays) */
	vrProcessStatsMark(curr_window->proc->stats, VR_TIME_RENDER1, 1);

	/*****************************************************/
	/* (vii) call simulator_render (if simulator window) */
	/* NOTE: this is done after world_render because the window */
	/*       is typically cleared by world_render.              */
	if (curr_window->mount == VRWINDOW_SIMULATOR) {
		vrTrace("_TxtRenderFunc", "(vii) call simulator_render");

		callback = curr_user->VisrenSim;
		if (!callback) {
			callback = curr_window->VisrenSim;
		}
		if (callback) {
			vrCallbackInvokeDynamic(callback, 2, renderinfo, curr_window->simulator_mask);
		} else	vrDbgPrintfN(ALWAYS_DBGLVL, "_TxtRenderFunc(): " RED_TEXT "no simulator callback available\n" NORM_TEXT);
	}


	/**************************************************************/
	/* (viii) display extra-world information (fps, stats, etc. ) */


	/*** render the timing statistics as a bar-graph ***/
      if (curr_window->stats_show) {
#ifdef NYI /* { */
	for (count = 0; count < VR_MAXSTATS; count++) {
		if (curr_window->stats[count] != NULL && *curr_window->stats[count] != NULL && (*(curr_window->stats[count]))->show_flag) {
			vrProcessStats	*stats = *(curr_window->stats[count]);
			double		char_height = 13;		/* empirically determined height of characters in pixels */
			double		char_width = 6.05;		/* empirically determined width of characters in pixels */
			double		scale = stats->time_scale * 2.0;
			double		text_scale = (char_height / curr_window->geometry.height);
			double		bottom = stats->yloc;			/* y location of bottom of chart */
			double		top =    bottom+(scale*stats->top_time);/* y location of top of chart */
			float		bar_width;
			double		y;
			int		frame;					/* for counting through the frames */
			int		segment;				/* for counting through the elements */

			/* if not currently rendering in 2D-ortho mode, then do so */
			if (!twod_ortho_mode) {
				/** go into 2-D ortho perspective mode **/
				glPushAttrib(0
					| GL_CURRENT_BIT	/* covers current color and rendering mode */
					| GL_ENABLE_BIT		/* covers all enable bits (inc. texture 2d)*/
					| GL_TRANSFORM_BIT	/* covers glMatrixMode, glNormalize & others */
					| GL_LINE_BIT		/* covers glLineWidth */
					| GL_STENCIL_BUFFER_BIT	/* covers all the stencil stuff */
				);
				glDisable(GL_LIGHTING);
				glDisable(GL_TEXTURE_1D);
				glDisable(GL_TEXTURE_2D);
				/* TODO: add 3D, after checking for that extension */

	#if 1 /* Option: without this the text will be masked by polygons "between the window and the viewer" */
				glDisable(GL_DEPTH_TEST);
	#endif

				glMatrixMode(GL_PROJECTION);
				glLoadIdentity();
				glOrtho(0.0, 1.0,  0.0, 1.0, -1.0, 1.0);	/* allows raster pos to be specified as 0.0 to 1.0 */
				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glLoadIdentity();
				twod_ortho_mode = 1;
			}

			/* draw a semi-transparent background to help contrast */
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);/* also works with GL_ONE as arg2 */

			glColor4fv(stats->back_color);			/* usually semi-transparent */
			glBegin(GL_POLYGON);
				glVertex2d(0.00, bottom);
				glVertex2d(0.00, top);
				glVertex2d(1.00, top);
				glVertex2d(1.00, bottom);
			glEnd();

			/* draw some horizontal lines to mark time */
			glShadeModel(GL_FLAT);
			glLineWidth(1.0);
			glColor3ub(0, 0, 0);		/* black */
			for (y = bottom; y <= top; y += stats->hline_interval * scale) {
				glBegin(GL_LINE_STRIP);
					glVertex2d(0.0, y);
					glVertex2d(1.0, y);
				glEnd();
			}

			/* draw the statistics as a bar graph */
			bar_width = (curr_window->geometry.width/stats->frames) * 0.80;
			glLineWidth(bar_width);
			for (frame = 0; frame < stats->frames; frame++) {
				int	first_element = stats->elements * frame;
				double	yloc = bottom;
#if 1 /* set to 0 for xloc that produces a heart-rate (oscilloscope) style display */
				double	xloc = (((frame-stats->time_frame)+(stats->frames-1)) % stats->frames)/(double)stats->frames + 0.005;
#else
				double	xloc = (double)frame/stats->frames + 0.005;
#endif
				for (segment = 0; segment < stats->elements; segment++) {

#if 0
vrPrintf("showmask = %x, compared with %x (segment %d)\n", stats->show_mask, (1 << segment), segment);
#endif
					if (stats->show_mask & (1 << segment)) {
						glBegin(GL_LINE_STRIP);
							glVertex2d(xloc, yloc);

#if 0
							switch (segment) {
							case 0:	glColor3ub(240,  50,  50); break;	/* red */
							case 1:	glColor3ub(240, 240, 240); break;	/* white */
							case 2:	glColor3ub( 50, 240, 240); break;	/* cyan */
							case 3:	glColor3ub(100,   0, 255); break;	/* dark purple */
							case 4: glColor3ub(  0, 255,   0); break;	/* green */
							case 5: glColor3ub(191, 121,   0); break;	/* dark yellow */
							case 6: glColor3ub(240, 138, 133); break;	/* rose */
							case 7: glColor3ub(240, 130,  30); break;	/* orange */
							}
#else
							glColor4fv(stats->elem_colors[segment]);
#endif
							yloc += scale * stats->measures[first_element+segment];
							glVertex2d(xloc, yloc);
						glEnd();
					}
				}
			}

			/* redraw lines over the bars in semi-transparent color */
			glLineWidth(1.0);
			glColor4ub(0, 0, 0, 64);		/* semi-transparent black */
			for (y = bottom; y <= top; y += stats->hline_interval * scale) {
				glBegin(GL_LINE_STRIP);
					glVertex2d(0.0, y);
					glVertex2d(1.0, y);
				glEnd();
			}

			/* draw the element labels */
			y = bottom + (2.2 * text_scale);
			for (segment = 0; segment < stats->elements; segment++) {
				glColor4fv(stats->elem_colors[segment]);
				glRasterPos2f(0.04, y);
				_TxtRenderText(renderinfo, stats->elem_labels[segment]);
				y += text_scale /* 0.012 */;	/* TODO: was ... "0.012 * scale", but that doesn't make sense -- perhaps we could have a "window_scale" value based on the size of the window */
			}

			/* draw the overall stats label */
			glColor4fv(stats->label_color);
			glRasterPos2f(0.01, bottom + (text_scale /*0.005*/ /*  * scale */));
			_TxtRenderText(renderinfo, stats->label);
		}
	}
#endif /* } NYI */
      }

	/*** render the frame rate as text ***/
	if (curr_window->fps_show) {
#ifdef NYI /* { */
static		char	fps_string[128];

		vrTrace("_TxtRenderFunc", "(viii) display the frame rate");

		/* if not currently rendering in 2D-ortho mode, then do so */
		if (!twod_ortho_mode) {
			/** go into 2-D ortho perspective mode **/
			glPushAttrib(0
				| GL_CURRENT_BIT	/* covers current color and rendering mode */
				| GL_ENABLE_BIT		/* covers all enable bits (inc. texture 2d)*/
				| GL_TRANSFORM_BIT	/* covers glMatrixMode, glNormalize & others */
				| GL_LINE_BIT		/* covers glLineWidth */
				| GL_STENCIL_BUFFER_BIT	/* covers all the stencil stuff */
			);
			glDisable(GL_LIGHTING);
			glDisable(GL_TEXTURE_1D);
			glDisable(GL_TEXTURE_2D);
			/* TODO: add 3D, after checking for that extension */

#if 1 /* Option: without this the text will be masked by polygons "between the window and the viewer" */
			glDisable(GL_DEPTH_TEST);
#endif

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0.0, 1.0,  0.0, 1.0, -1.0, 1.0);	/* allows raster pos to be specified as 0.0 to 1.0 */
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();
			twod_ortho_mode = 1;
		}

		/** now render the frame rate data **/
		glColor3fv(curr_window->fps_color);
		glRasterPos2f(curr_window->fps_loc[0], curr_window->fps_loc[1]);
		sprintf(fps_string, "FPS: %5.2f(1) %5.2f(10)", curr_window->proc->fps1, curr_window->proc->fps10);
		_TxtRenderText(renderinfo, fps_string);
#endif /* } NYI */
	}

	/*** render input histories ***/
	if (curr_window->inputs_show) {
#ifdef NYI /* { */
static		char		string[256];
		vrInputInfo	*inputs = renderinfo->context->input;	/* aka "vrInputs" */
		int		count;

		vrTrace("_TxtRenderFunc", "(viii) display input histories");

		/* first do a simple render of the first wand */
		glPushMatrix();
		vrRenderTransform6sensor(renderinfo, 1);
		glBegin(GL_LINES);
			/* a cyan line pointing out */
			glColor3ub(0, 255, 255);
			glVertex3f(0.0, 0.0, 0.0);
			glVertex3f(0.0, 0.0, -1.0);

			/* a red line pointing up */
			glColor3ub(255, 50, 50);
			glVertex3f(0.0, 0.0, 0.0);
			glVertex3f(0.0, 0.5, 0.0);

			/* a green line side to side */
			glColor3ub(50, 255, 50);
			glVertex3f(-0.5, 0.0, 0.0);
			glVertex3f( 0.5, 0.0, 0.0);
		glEnd();
		glPopMatrix();

		/* if not currently rendering in 2D-ortho mode, then do so */
		if (!twod_ortho_mode) {
			/** go into 2-D ortho perspective mode **/
			glPushAttrib(0
				| GL_CURRENT_BIT	/* covers current color and rendering mode */
				| GL_ENABLE_BIT		/* covers all enable bits (inc. texture 2d)*/
				| GL_TRANSFORM_BIT	/* covers glMatrixMode, glNormalize & others */
				| GL_LINE_BIT		/* covers glLineWidth */
				| GL_STENCIL_BUFFER_BIT	/* covers all the stencil stuff */
			);
			glDisable(GL_LIGHTING);
			glDisable(GL_TEXTURE_1D);
			glDisable(GL_TEXTURE_2D);
			/* TODO: add 3D, after checking for that extension */

#if 1 /* Option: without this the text will be masked by polygons "between the window and the viewer" */
			glDisable(GL_DEPTH_TEST);
#endif

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0.0, 1.0,  0.0, 1.0, -1.0, 1.0);	/* allows raster pos to be specified as 0.0 to 1.0 */
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();
			twod_ortho_mode = 1;
		}

		/** now render the historical input data **/
		glColor3fv(curr_window->fps_color);			/* TODO: should this have it's own color? */
		glLineWidth(5.0);					/* TODO: should this be configurable somewhere */

#if 1
		/* history of the 2-switches */
		for (count = 0; count < inputs->num_2ways; count++) {
			vr2switch	*input = inputs->switch2[count];

			/* if the history for this input is enabled, then render it as a ... */
			if (input->num_measures > 0) {
				int	step;

				for (step = 0; step < input->num_measures; step++) { 
#if 0 /* [12/04/09: This is a choice between sweep/scope view and stream/scroll view] */
					int	measurement = ((step-(input->current_measure-1))+(input->num_measures)) % input->num_measures;
#else
					int	measurement = (step+(input->current_measure+1)) % input->num_measures;
#endif
#if 0
if (count == 1)
printf("input %d: step = %d, current = %d, measurement = %d, timestamp = %lf\n", count, step, input->current_measure, measurement, input->timestamp);
#endif

					if (input->measures[measurement]) {
						double	yloc = 0.18 + count * 0.05;
#if 0
						double	xloc = (double)(measurement)/(double)(input->num_measures) + 0.003;
#else
						double	xloc = (double)(step)/(double)(input->num_measures) + 0.0003;
#endif

						glBegin(GL_LINE_STRIP);
							glVertex2d(xloc, yloc);
							yloc += 0.04;
							glVertex2d(xloc, yloc);
						glEnd();
					}
				}
			}
		}
#endif

		/* history of the valuators */
		for (count = 0; count < inputs->num_valuators; count++) { vrValuator	*input = inputs->valuator[count]; /* if the history for this input is enabled, then render it as a ... */ if (input->num_measures > 0) { int	step; switch (count) { case 0:	glColor3ub(240,  50,  50); break;	/* red */ case 1:	glColor3ub(240, 240, 240); break;	/* white */ case 2:	glColor3ub( 50, 240, 240); break;	/* cyan */
				case 3:	glColor3ub(100,   0, 255); break;	/* dark purple */
				case 4: glColor3ub(  0, 255,   0); break;	/* green */
				case 5: glColor3ub(191, 121,   0); break;	/* dark yellow */
				case 6: glColor3ub(240, 138, 133); break;	/* rose */
				case 7: glColor3ub(240, 130,  30); break;	/* orange */
				}
				for (step = 0; step < input->num_measures; step++) { 
#if 0 /* [12/04/09: This is a choice between sweep/scope view and stream/scroll view] */
					int	measurement = ((step-(input->current_measure-1))+(input->num_measures)) % input->num_measures;
#else
					int	measurement = (step+(input->current_measure+1)) % input->num_measures;
#endif
					double	yloc = 0.06 + count * 0.08;
					double	xloc = (double)(step)/(double)(input->num_measures) + 0.0003;

#if 0
if (count == 1)
printf("input %d: step = %d, current = %d, measurement = %d, xloc = %0.4f\n", count, step, input->current_measure, measurement, xloc);
#endif

					glBegin(GL_LINE_STRIP);
						glVertex2d(xloc, yloc);
						yloc += input->measures[measurement] * 0.05;
						glVertex2d(xloc, yloc);
					glEnd();
				}

				/** now render the input name and current value **/
#if 0
				glColor3fv(curr_window->fps_color);
				glRasterPos2f(curr_window->fps_loc[0], curr_window->fps_loc[1]);
#else
				glRasterPos2f(0.70, 0.05 + count * 0.08);
#endif
				sprintf(string, "input: %s (%5.2f)", input->my_object->name, input->measures[(input->current_measure-1) % input->num_measures]);
				_TxtRenderText(renderinfo, string);
			}
		}
#endif /* } NYI */
	}

	/*** render the user-interface information on screen ***/
	if (curr_window->ui_show) {
#ifdef NYI /* { */
#if defined(MP_PTHREADS) || defined(MP_PTHREADS2)	/* pthreads requires a separate copy for each string to avoid overwriting */
		char	ui_string[2048];
		char	ui_string_tok[2048];
#else
static		char	ui_string[2048];
static		char	ui_string_tok[2048];
#endif
		char	*next_line;			/* the current line to be rendered */
		float	back_color[4] = { 0.20, 0.20, 0.20, 0.50 };	/* a semi-transparent dark gray */
		float	border_color[4] = { 1.00, 1.00, 1.00, 1.00 };	/* an opaque white */
		double	char_height = 13;		/* empirically determined height of characters in pixels */
		double	char_width = 6.00;		/* empirically determined width of characters in pixels */
		double	xscale;				/* text char separation (basically a percentage of screen) */
		double	yscale;				/* text line separation (basically a percentage of screen) */
		double	top = curr_window->ui_loc[1];	/* y location of top of display */
		double	left =  curr_window->ui_loc[0];	/* x location of left of display */
		double	bottom;				/* y location of bottom of display (calculated) */
		double	right;				/* x location of right of display (calculated) */
		double	y;				/* for advancing text lines */
		int	width, height;			/* the character dimensions of the string */

		vrTrace("_TxtRenderFunc", "(viii) display the user-interface information");

		/* if not currently rendering in 2D-ortho mode, then do so */
		if (!twod_ortho_mode) {
			/** go into 2-D ortho perspective mode **/
			glPushAttrib(0
				| GL_CURRENT_BIT	/* covers current color and rendering mode */
				| GL_ENABLE_BIT		/* covers all enable bits (inc. texture 2d)*/
				| GL_TRANSFORM_BIT	/* covers glMatrixMode, glNormalize & others */
				| GL_LINE_BIT		/* covers glLineWidth */
				| GL_STENCIL_BUFFER_BIT	/* covers all the stencil stuff */
			);
			glDisable(GL_LIGHTING);
			glDisable(GL_TEXTURE_1D);
			glDisable(GL_TEXTURE_2D);
			/* TODO: add 3D, after checking for that extension */

#if 1 /* Option: without this the text will be masked by polygons "between the window and the viewer" */
			glDisable(GL_DEPTH_TEST);
#endif

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0.0, 1.0,  0.0, 1.0, -1.0, 1.0);	/* allows raster pos to be specified as 0.0 to 1.0 */
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();
			twod_ortho_mode = 1;
		}

		/* make the user-interface output string */
		vrSprintInputUI(ui_string, renderinfo->context->input, sizeof(ui_string), verbose);
		width = vrStringCharWidth(ui_string);
		height = vrStringCharHeight(ui_string) - 1;	/* subtract the trailing, empty, line */

		/* adjust the right and bottom values based on size of window and string */

#if 0
		scale = curr_window->geometry.height * 0.0001;
		bottom = top - (height * scale);
		right = left + (width * scale * 0.2);
#else
		xscale = char_width / curr_window->geometry.width;
		yscale = char_height / curr_window->geometry.height;
		right = left + (xscale * (width + 2));
		bottom = top - (yscale * (height + 1));
#endif

		/* adjust the colors of the background (darker and transparent)  */
		/*   and the border (lighter and opaque) based on the text color */
		back_color[0] = curr_window->ui_color[0] * 0.20;
		back_color[1] = curr_window->ui_color[1] * 0.20;
		back_color[2] = curr_window->ui_color[2] * 0.20;
		back_color[3] = 0.50;

		border_color[0] = 1.0 - ((1.0 - curr_window->ui_color[0]) * 0.50);
		border_color[1] = 1.0 - ((1.0 - curr_window->ui_color[1]) * 0.50);
		border_color[2] = 1.0 - ((1.0 - curr_window->ui_color[2]) * 0.50);
		border_color[3] = 1.0;

		/* draw a semi-transparent background to help contrast */
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);/* also works with GL_ONE as arg2 */

		glColor4fv(back_color);				/* usually semi-transparent */
		glBegin(GL_POLYGON);
			glVertex2d(left,  bottom);
			glVertex2d(left,  top);
			glVertex2d(right, top);
			glVertex2d(right, bottom);
		glEnd();

		/* and put a white border around three sides -- since we don't know how far left the text will go */
		glColor4fv(border_color);
		glBegin(GL_LINE_LOOP);
			glVertex2d(right, top);
			glVertex2d(left,  top);
			glVertex2d(left,  bottom);
			glVertex2d(right, bottom);
		glEnd();

		/** now render the user-interface data **/
		glColor3fv(curr_window->ui_color);

		y = top - (1.30 * yscale);
		next_line = strtok_r(ui_string, "\n", (char **)&ui_string_tok);
		do {
			glRasterPos2f(left + 1.0 * xscale, y);
			_TxtRenderText(renderinfo, next_line);
			y -= yscale;
		} while (next_line = strtok_r(NULL, "\n", (char **)&ui_string_tok));
#endif /* } NYI */
	}

	/*** restore to 3D mode if we changed into 2D ortho mode ***/
	if (twod_ortho_mode) {
#ifdef NYI /* { */
		glPopMatrix();
		glPopAttrib();
#endif /* } NYI */
	}

	/* measure: time just spent in rendering the simulator and information displays */
	vrProcessStatsMark(curr_window->proc->stats, VR_TIME_RENDERINFO, 1);


	/**************************************/
	/* TODO: does viewmask stuff go here? */


	/***********************************/
	/* (ix) restore gfx matrix/state */
#ifdef NYI /* { */
	glPopMatrix(); /* } */
#endif /* } NYI */

	sprintf(trace_msg, "ending window render loop for window '%s' %#p", curr_window->name, curr_window);
	vrTrace("_TxtRenderFunc", trace_msg);
}
/* end of render function */


/****************************************************************************/
static void _TxtRenderNullWorld()
{
#ifdef NYI /* { */
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif /* } NYI */
}


/****************************************************************************/
static void _TxtRenderText(vrRenderInfo *renderinfo, char *string)
{
	char		*font = "fixed";
	vrWindowInfo	*curr_window = vrRenderCurrentWindow(renderinfo);
	vrTxtPrivateInfo *aux = NULL;

	/* attempting to rendering a NULL string (not the empty string) is bad */
	if (string == NULL)
		return;

#if 0 /* 05/26/2006: I think Ed put this here, and I don't think we want it here */
	/* clear out any error messages in the glError queue */
	while ((err = glGetError()) != GL_NO_ERROR ) ;
#endif

	aux = (vrTxtPrivateInfo *)curr_window->aux_data;

#ifdef NYI /* { */
	/* if no font has been initialized, do so */
	if (!aux->fontListBase) {
		aux->fontStruct = XLoadQueryFont(aux->xdisplay, font);
		if (aux->fontStruct) {
			aux->fontListBase = glGenLists(aux->fontStruct->max_char_or_byte2);
			TxtUseXFont(aux->fontStruct->fid,0,aux->fontStruct->max_char_or_byte2,aux->fontListBase);
		}

	}

	/* if we now have a font setup, render the text */
	if (aux->fontListBase) {
		glPushAttrib(GL_LIST_BIT);
		glListBase(aux->fontListBase);
		glCallLists(strlen(string),GL_UNSIGNED_BYTE,(GLubyte *)string);
		glPopAttrib();
	} else {
		vrDbgPrintf("Unable to render text in Txt.\n");
	}
#endif /* } NYI */
}


/****************************************************************************/
/* vrTxtInitWindowInfo(): setup all the callbacks needed for Txt rendering. */
/****************************************************************************/
void vrTxtInitWindowInfo(vrWindowInfo *info)
{
	vrDbgPrintfN(DEFAULT_DBGLVL, "Initializing callback and version Info for Window at %#p\n", info);

	info->version = (char *)vrShmemStrDup("Txt render window");
	info->PrintAux = vrCallbackCreateNamed("TxtWindow:PrintAux-Def", vrFprintTxtPrivateInfo, 0);
	info->PreOpenInit = vrCallbackCreateNamed("TxtWindow:PreOpenInit-DN", vrDoNothing, 0);
	info->Open = vrCallbackCreateNamed("TxtWindow:Open-Def", _TxtOpenFunc, 1, info);
	info->Close = vrCallbackCreateNamed("TxtWindow:Close-Def", _TxtCloseFunc, 1, info);
	info->Swap = vrCallbackCreateNamed("TxtWindow:Swap-Def", _TxtSwapFunc, 1, info);
	info->Errors = vrCallbackCreateNamed("TxtWindow:Errors-Def", _TxtRenderErrors, 0);
	info->RenderTransform = vrCallbackCreateNamed("TxtWindow:Transform-Def", _TxtRenderTransform, 0);
	info->SetProjectionTransform = vrCallbackCreateNamed("TxtWindow:ProjectionTransform-Def", _TxtSetProjectionTransform, 0);
	info->Render = vrCallbackCreateNamed("TxtWindow:Render-Def", _TxtRenderFunc, 0);
	info->RenderText = vrCallbackCreateNamed("TxtWindow:RenderText-Def", _TxtRenderText, 0);
	info->RenderNullWorld = vrCallbackCreateNamed("TxtWindow:RenderNW-Def", _TxtRenderNullWorld, 0);
	info->RenderSimulator = vrCallbackCreateNamed("TxtWindow:RenderSim-Def", _TxtRenderNullWorld, 0);	/* was vrGLRenderDefaultSimulator() */
}

