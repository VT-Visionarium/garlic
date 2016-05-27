/* ======================================================================
 *
 *  CCCCC          vr_visren.pf.c
 * CC   CC         Author(s): Bill Sherman
 * CC              Created: September 19, 2001
 * CC   CC         Last Modified: January 31, 2007
 *  CCCCC
 *
 * Code file for FreeVR visual rendering into Performer windows.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/*************************************************************************

FreeVR USAGE:
	Here are the FreeVR configuration options for the Performer visual rendering window:
		"display=[host]:server[.screen]" -- set the display where the window should appear
		"geometry=[WxH][+X+Y]" -- set where on the display the window should appear
		"decoration={borders|title|minmax|window|none}*" -- set look of window
			(use "none" when windows will be joined with neighboring windows)
		"title=window title" -- set the title on the window title bar
		"cursor={default|blank|dot|bigdot}" -- set the name of the cursor

	Controls are specified in the freevrrc file:
	  :-(	e.g.: control "<control option>" = "switch2(button[{1|2|3|4|5|6|7|8|Star}])";

	Available control options are:
	  :-(	"print_help" -- print info on how to use the display device
	  :-(	"print_struct" -- print the internal PF data structure (for debugging)
		-- simulator controls should be made to be control options --

TODO:
	- choose the correct visual, based on need/request

	- figure out the proper way to assign the value to "window->dualeye_buffer"

	- make sure viewport changes are handled, by hand if not automatically
		done by Performer.

	- determine if the count/fps process information can be updated here.

	- see also the vr_visren.glx.c TODO list.

**************************************************************************/
#include <stdlib.h>			/* used by getenv() */
#include <string.h>
#include <X11/keysym.h>

#ifdef __cplusplus
#  include <Performer/pf/pfChannel.h>
#  include <Performer/pf/pfDCS.h>	/* also handles pfSCS */
#  include <Performer/pf/pfGeode.h>
#  include <Performer/pf/pfLightSource.h>
#  include <Performer/pf/pfTraverser.h>
#  include <Performer/pfdu.h>
#  include <Performer/prmath.h>		/* for pfMatrix & pfVec4 */
#else /* !__cplusplus */
#  define PF_C_API 1
#  include <Performer/pf.h>
#  include <Performer/pfdu.h>
#  include <Performer/pr.h>
#  include <Performer/prmath.h>
#  if __mips != 2
#    define  pfMat_NoWarn	(const float (*)[4])	/* use this to eliminate false warnings due to a quirk of C and the Performer declarations */
#  else
#    define  pfMat_NoWarn	/* empty comment */
#  endif
#if 0
#  include <Performer/pf/pfTraverser.h>
#endif
#endif /* __cplusplus */



#include "vr_visren.h"
#include "vr_visren.glx.h"		/* How does Performer make use of this? */
#include "vr_basicgfx.glx.h"		/* needed for vrGLRenderDefaultSimulator() */
#include "vr_callback.h"		/* also included within vr_input.h, but left for clarity */
#include "vr_input.h"
#include "vr_debug.h"
#include "vr_utils.h"			/* for declarations of vrStringCharWidth() & vrStringCharHeight() */
#include "vr_config.h"			/* needed in vrPfPreFrame() */


#if 0 /* set to 1 to prevent local vrTrace messages */
#  ifdef vrTrace
#    undef	vrTrace
#    define	vrTrace(a,b) ;
#  endif
#endif

/* Function declarations required for forward referencing. */
static void	_PfRenderFunc(pfChannel *chan, void *data);


/****************************************************************************/
/* NOTE: this function used by vr_input.windows.c */
int vrXwindowsErrorHandler(Display *display, XErrorEvent *event)
{
static	char	string[1024];

	vrPrintf(BOLD_TEXT "Got an X error in 'dbx -p %d'\n" NORM_TEXT, getpid());
	XGetErrorText(display, event->error_code, string, 1023);
	vrPrintf(BOLD_TEXT "Reported error is '%s'\n", string);

	pause();

	return 0;	/* meaningless return value for picky compilers */
}


/*****************************************************************/
/* NOTE: this function used by vr_input.windows.c */
Cursor vrXwindowsMakeCursor(vrGlxPrivateInfo *aux, char *cursor_name)
{
static	char	blank_data[1] = { 0x00 };
static	char	dot_data[1] = { 0x01 };
static	char	bigdot_data[2] = { 0xFF, 0xFF };
	Pixmap	cursor_map;
	int	width, height;
	XColor	fg;
	XColor	bg;
	Cursor	return_cursor;

	if (!strcasecmp(cursor_name, "blank")) {
		width = 1;
		height = 1;
		cursor_map = XCreateBitmapFromData(aux->xdisplay, RootWindow(aux->xdisplay, aux->xscreen), blank_data, width, height);
	} else if (!strcasecmp(cursor_name, "dot")) {
		width = 1;
		height = 1;
		cursor_map = XCreateBitmapFromData(aux->xdisplay, RootWindow(aux->xdisplay, aux->xscreen), dot_data, width, height);
	} else if (!strcasecmp(cursor_name, "bigdot")) {
		width = 2;
		height = 2;
		cursor_map = XCreateBitmapFromData(aux->xdisplay, RootWindow(aux->xdisplay, aux->xscreen), bigdot_data, width, height);
	} else if (!strcasecmp(cursor_name, "default")) {
		/* don't set a map and go with what's already there */
		return None;
	} else {
		vrDbgPrintfN(AALWAYS_DBGLVL, "vrXwindowsMakeCursor(): unknown cursor name.\n");
		return None;
	}

	if (cursor_map == None)
		vrPrintf("vrXwindowsMakeCursor(): " RED_TEXT "out of memory for cursor.\n" NORM_TEXT);

	if (aux->xcolormap != None) {
		XParseColor(aux->xdisplay, aux->xcolormap, "red", &fg);
		XParseColor(aux->xdisplay, aux->xcolormap, "white", &bg);
	} else {
		vrDbgPrintfN(AALWAYS_DBGLVL, "vrXwindowsMakeCursor(): " RED_TEXT "Warning: no colormap from which to assign cursor colors.\n");
		fg.pixel = WhitePixel(aux->xdisplay, aux->xscreen);
		bg.pixel = BlackPixel(aux->xdisplay, aux->xscreen);
	}

	return_cursor = XCreatePixmapCursor(aux->xdisplay, cursor_map, cursor_map, &fg, &bg, width, height);
	XFreePixmap(aux->xdisplay, cursor_map);

	return return_cursor;
}


/*****************************************************************/
void vrFprintGlxPrivateInfo(FILE *file, vrGlxPrivateInfo *aux, vrPrintStyle style)
{
	/* TODO: print different things for different styles */

	/* if null pointer given, print an empty shell and return */
	if (aux == NULL) {
		vrFprintf(file, "glxinfo \"(nil)\" = { }\n");
		return;
	}

	switch (style) {
	case one_line:
		vrFprintf(file, "GlxInfo = { xhost = \"%s\" xserver = %d xscreen = %d Display = %#p Window = %#p }\n",
			aux->xhost,
			aux->xserver,
			aux->xscreen,
			aux->xdisplay,
			aux->xwindow);
		break;

	default:
	case verbose:
		vrFprintf(file, "GlxInfo (%#p) = {\n", aux);
		vrFprintf(file, "\r"
			"\txhost = \"%s\"\n\txserver = %d\n\txscreen = %d\n"
			"\tDisplay = %#p\n\tWindow = %#p\n\tXvisual = %#p\n"
			"\tXcolormap = %#p\n\tGLXContext = 0x%x\n"
			"\tscreen_size = %dx%d\n\tdecorations = 0x%x\n\n"
			"\tmapped = %d\n\tused_by_input = %d\n\tstereo buffer = %d\n\tdouble buffer = %d\n",
			aux->xhost,
			aux->xserver,
			aux->xscreen,
			aux->xdisplay,
			aux->xwindow,
			aux->xvisual,
			aux->xcolormap,
			aux->glx_context,
			aux->xscreen_size_x,
			aux->xscreen_size_y,
			aux->decorations,
			aux->mapped,
			aux->used_by_input,
			aux->stereo_buf,
			aux->doub_buf);
		vrFprintf(file, "\r}  /* TODO: add more GLX details */\n");

		/* TODO: the rest of the fields */
		break;
	}
}


/*****************************************************************/
/* TODO: except for "blank" (the important one), none of these */
/*   cursor shapes really match the GLX versions (which are    */
/*   created by hand), so probably should correct this.        */
/* NOTE: the list of available Performer cursors can be found in */
/*   /usr/include/Performer/pfutil.h */
/* NOTE: this is currently unused */
static int _GetCursorFromName(char *cursor_name)
{

	if      (!strcasecmp(cursor_name, "blank"))	return PFU_CURSOR_OFF;
	else if (!strcasecmp(cursor_name, "dot"))	return PFU_CURSOR_dot;
	else if (!strcasecmp(cursor_name, "bigdot"))	return PFU_CURSOR_dotbox;
	else if (!strcasecmp(cursor_name, "arrow"))	return PFU_CURSOR_arrow;
	else if (!strcasecmp(cursor_name, "default")) {
		/* don't set a map and go with what's already there */
		/* TODO: figure out if there's a Performer equivalent to this */
		return PFU_CURSOR_arrow;
	} else {
		vrDbgPrintfN(AALWAYS_DBGLVL, "_GetCursorFromName(): unknown cursor name.\n");
		return PFU_CURSOR_arrow;
	}

	return PFU_CURSOR_arrow;
}


/*****************************************************************/
/* convert from a FreeVR style vrMatrix to a Performer pfMatrix */
/* NOTE: vrMatrices are stored in OpenGL order, which is the */
/*   transpose of how most textbooks specify the 4x4 transform */
/*   matrix, and how Performer seems to print its matrices, though */
/*   according to the man page Performer pfMatrix is specified in  */
/*   row-major order (ie. rows count faster than columns) which    */
/*   matches the OpenGL (and FreeVR) formats.                      */
pfMatrix *vrPfMatrixFromVrMatrix(pfMatrix *pfmat, vrMatrix *vrmat)
{
	(*pfmat)[0][0] = vrmat->v[ 0];
	(*pfmat)[0][1] = vrmat->v[ 1];
	(*pfmat)[0][2] = vrmat->v[ 2];
	(*pfmat)[0][3] = vrmat->v[ 3];
	(*pfmat)[1][0] = vrmat->v[ 4];
	(*pfmat)[1][1] = vrmat->v[ 5];
	(*pfmat)[1][2] = vrmat->v[ 6];
	(*pfmat)[1][3] = vrmat->v[ 7];
	(*pfmat)[2][0] = vrmat->v[ 8];
	(*pfmat)[2][1] = vrmat->v[ 9];
	(*pfmat)[2][2] = vrmat->v[10];
	(*pfmat)[2][3] = vrmat->v[11];
	(*pfmat)[3][0] = vrmat->v[12];
	(*pfmat)[3][1] = vrmat->v[13];
	(*pfmat)[3][2] = vrmat->v[14];
	(*pfmat)[3][3] = vrmat->v[15];

	return pfmat;
}


/*****************************************************************/
/* convert from a Performer pfMatrix to a FreeVR style vrMatrix */
vrMatrix *vrMatrixFromPfMatrix(vrMatrix *vrmat, pfMatrix *pfmat)
{
	vrmat->v[ 0] = (*pfmat)[0][0];
	vrmat->v[ 1] = (*pfmat)[0][1];
	vrmat->v[ 2] = (*pfmat)[0][2];
	vrmat->v[ 3] = (*pfmat)[0][3];
	vrmat->v[ 4] = (*pfmat)[1][0];
	vrmat->v[ 5] = (*pfmat)[1][1];
	vrmat->v[ 6] = (*pfmat)[1][2];
	vrmat->v[ 7] = (*pfmat)[1][3];
	vrmat->v[ 8] = (*pfmat)[2][0];
	vrmat->v[ 9] = (*pfmat)[2][1];
	vrmat->v[10] = (*pfmat)[2][2];
	vrmat->v[11] = (*pfmat)[2][3];
	vrmat->v[12] = (*pfmat)[3][0];
	vrmat->v[13] = (*pfmat)[3][1];
	vrmat->v[14] = (*pfmat)[3][2];
	vrmat->v[15] = (*pfmat)[3][3];

	return vrmat;
}


/****************************************************************************/
static char *_GlxGLErrorCodeString(GLenum code)
{
	switch (code) {
	case GL_NO_ERROR:		return "No Error";
	case GL_INVALID_ENUM:		return "Invalid Enumerated argument";
	case GL_INVALID_VALUE:		return "Invalid argument value";
	case GL_INVALID_OPERATION:	return "Invalid operation within a state";
	case GL_STACK_OVERFLOW:		return "Stack Overflow";
	case GL_STACK_UNDERFLOW:	return "Stack Underflow";
	case GL_OUT_OF_MEMORY:		return "Out of Memory";
	case GL_TABLE_TOO_LARGE:	return "Table too large";
	}

	return "Unknown GL-error type";
}


/****************************************************************************/
static void _GlxParseArgs(vrGlxPrivateInfo *aux, char *args)
{
	char	*str = NULL;

	/* TODO: see how much of the rest of this parsing stuff can be made generic */

	/**********************************************************************/
	/** Argument format: "display=" [host]:server[.screen] [(";" | ",")] **/
	/**********************************************************************/
	if (str = strstr(args, "display=")) {
		char	*host = strchr(str, '=') + 1;
		char	*colon = strchr(str, ':');

		if (colon == NULL) {
			aux->xhost = vrShmemStrDup("");
			colon = str-1;
		} else {
			*colon = '\0';
			aux->xhost = vrShmemStrDup(host);
			*colon = ':';
		}
		sscanf(colon, ":%d.%d", &(aux->xserver), &(aux->xscreen));
		vrDbgPrintfN(SELDOM_DBGLVL, "parsed host = '%s', xserver = %d, screen = %d\n", host, aux->xserver, aux->xscreen);
	} else {
		aux->xhost = NULL;
		aux->xserver = -1;
	}

	/************************************************************/
	/** Argument format: "geometry=" [WxH][+X+Y] [(";" | ",")] **/
	/************************************************************/
	/* information is stored in the XSizeHints structure */
	if (str = strstr(args, "geometry=")) {
		char		*geom = strchr(str, '=');
		char		*space = strchr(geom, ' ');
		char		*semi = strchr(geom, ';');
		int		parsed;				/* return mask from XParseGeometry() */
		int		x, y;				/* return values from XParseGeometry () */
		unsigned int	width, height;			/* return values from XParseGeometry () */

		/* figure out which (non-NULL) terminator comes first, and end the string there */
		if (space == NULL) {
			if (semi != NULL)
				*semi = '\0';
		} else {
			if (semi != NULL && semi < space)
				*semi = '\0';
			else	*space = '\0';
		}

		parsed = XParseGeometry(geom, &x, &y, &width, &height);
#if 0 /* no need to limit the size of the window */
		aux->xsize_hints.flags = USPosition | PSize | PMinSize | PMaxSize;
		aux->xsize_hints.max_width = width;
		aux->xsize_hints.max_height = height;
#endif
		aux->xsize_hints.flags = USPosition | PSize | PMinSize;
		if (parsed & XValue)
			aux->xsize_hints.x = x;
		if (parsed & YValue)
			aux->xsize_hints.y = y;
		if (parsed & WidthValue)
			aux->xsize_hints.width = width;
		aux->xsize_hints.min_width = 10;
		if (parsed & HeightValue)
			aux->xsize_hints.height = height;
		aux->xsize_hints.min_height = 10;

		/* restore the original string */
		if (space == NULL) {
			if (semi != NULL)
				*semi = ';';
		} else {
			if (semi != NULL && semi < space)
				*semi = ';';
			else	*space = ' ';
		}
	} else {
		vrDbgPrintf("No geometry field to parse: args = '%s'\n", args);
		aux->xsize_hints.flags = 0;
	}

	/**********************************************************/
	/** Argument format: "decoration=" {opts,} [(";" | ",")] **/
	/**********************************************************/
	/* where opts is a comma-separated string with one         */
	/* or more of these tokens in it:                          */
	/*   borders|border (resize handles)                       */
	/*   title|titlebar (title bar - for moving, not resizing) */
	/*   minmax (minimize and maximize buttons on titlebar)    */
	/* these two options set multiple resources:               */
	/*   all|window (full title bar and borders)               */
	/*   none (nothing -- the default)                         */

	/* set the decorations flag */
	if (str = strstr(args, "decoration=")) {
		char	hints[128];
		char	*hint_str = strchr(str, '=') + 1;
		char	*tok;

		aux->decorations = 0;

		sscanf(hint_str, "%s", hints);
		tok = strtok(hints, ", ");
		while (tok) {
			if (!strcasecmp(tok, "borders")) {
				aux->decorations |= DECORATION_BORDER;
			} else if (!strcasecmp(tok, "border")) {
				aux->decorations |= DECORATION_BORDER;
			} else if (!strcasecmp(tok, "minmax")) {
				aux->decorations |= DECORATION_MINMAX;
			} else if (!strcasecmp(tok, "title")) {
				aux->decorations |= DECORATION_TITLE;
			} else if (!strcasecmp(tok, "titlebar")) {
				aux->decorations |= DECORATION_TITLE;
			} else if (!strcasecmp(tok, "window")) {
				aux->decorations = DECORATION_ALL;
			} else if (!strcasecmp(tok, "all")) {
				aux->decorations = DECORATION_ALL;
			} else if (!strcasecmp(tok, "none")) {
				aux->decorations = 0;
			}
			tok = strtok(NULL, ", ");
		}
	}

	/**********************************************************/
	/** Argument format: "title=" <string> [(";" | ",")] **/
	/**********************************************************/
	/* where <string> is a string that will be used for the */
	/*   window title message.                              */

	/* set the title string */
	if (str = strstr(args, "title=")) {
		char	*title_str = strchr(str, '=') + 1;
		char	*tok;

		tok = strtok(title_str, ",;");
		aux->window_title = vrShmemStrDup(tok);
	}

	/******************************************************/
	/** Argument format: "cursor= <string> [(";" | ",")] **/
	/******************************************************/
	/* where <string> is a string that will be used for setting */
	/*   the name of the cursor that is used.                   */

	/* set the cursor_name string */
	if (str = strstr(args, "cursor=")) {
		char	*title_str = strchr(str, '=') + 1;
		char	*tok;

		tok = strtok(title_str, ",;");
		aux->cursor_name = vrShmemStrDup(tok);
	}

	vrDbgPrintf("================================================\n");
	vrDbgPrintf("done parsing argument string `%s'\n", args);
	vrDbgPrintf("aux->window_title = '%s'\n", aux->window_title);
	vrDbgPrintf("aux->xhost = '%s'\n", aux->xhost);
	vrDbgPrintf("aux->xserver = %d\n", aux->xserver);
	vrDbgPrintf("aux->xscreen = %d\n", aux->xscreen);
	vrDbgPrintf("aux->decoration = 0x%02x\n", aux->decorations);
	vrDbgPrintf("geometry: %dx%d+%d+%d\n",
		aux->xsize_hints.width, aux->xsize_hints.height,
		aux->xsize_hints.x, aux->xsize_hints.y);
	vrDbgPrintf("xsize_hints.flags = %d\n", aux->xsize_hints.flags);
	vrDbgPrintf("aux->cursor_name = '%s'\n", aux->cursor_name);
	vrDbgPrintf("================================================\n");
}



#ifndef GFX_PERFORMER /* This is used by GLX, but not Performer. */
/*****************************************************************/
/* vrWaitForWindowMapping(): used by _PfOpenFunc() to wait for   */
/*   a window to be mapped to an X-screen before continuing.     */
/*****************************************************************/
static Bool vrWaitForWindowMapping(Display *xdisplay, XEvent *event, XPointer data)
{
	Window	xwindow = (Window)data;

	if (event->xmap.window == xwindow && event->type == MapNotify)
		return True;
	else	return False;
}
#endif


/*****************************************************************/
/* _PfInitFunc() initializes the auxiliary graphics system prior */
/*   to opening any windows.                                     */
/*****************************************************************/
static void _PfInitFunc(vrWindowInfo *info)
{
	vrConfigInfo	*vrConfig = info->context->config;
	int		mp_bitmask;
	int		return_proc;
	int		return_pipe;


	return_proc = pfMultiprocess(PFMP_DEFAULT);	/* TODO: should this be PFMP_APP_CULL_DRAW ?? */
	mp_bitmask = pfGetMultiprocess();
	if (!return_proc) {
		vrDbgPrintfN(PERFORMER_DBGLVL, "_PfInitFunc(): " RED_TEXT "WARNING! Requested multiprocessing mode not granted.  req: %x, grant: %x\n" NORM_TEXT, PFMP_DEFAULT, mp_bitmask);
	}

	/* NOTE: the we don't really know that all <num_windows> FreeVR-windows are   */
	/*   actually on different hardware-screens (ie. rendering pipes), so setting */
	/*   pfMultipipe() to this value is really giving an upper bound on how many  */
	/*   pfPipes() we may want.  However, FreeVR-windows on the same hardware-    */
	/*   screen should share the same pfPipe.                                     */
	/* NOTE also: unfortunately there is no way to reduce the number of pfPipes   */
	/*   after this point, so we have to use all the pipes that are requested.    */
	/*   Thus ideally we need to figure out exactly how many hardware-screens are */
	/*   used by the current configuration -- a hard question, because we haven't */
	/*   parsed the window arguments yet.                                         */
#if 1 /* Should always be 1 except set to 0 to test multi pipewindows per pipe on IRIX */
	return_pipe = pfMultipipe(vrConfig->num_windows);
	vrDbgPrintfN(PERFORMER_DBGLVL, "_PfInitFunc(): " BOLD_TEXT "just called pfMultipipe(%d), with return value of '%d' -- pfGetMultipipe() returns '%d'\n" NORM_TEXT, vrConfig->num_windows, return_pipe, pfGetMultipipe());
#else
	vrDbgPrintfN(ALWAYS_DBGLVL, "_PfInitFunc(): " RED_TEXT "ALERT!  Code for testing multiple pipes on one screen activated, this will break multi-screen operation!\n" NORM_TEXT);
	return_pipe = pfMultipipe(1);
#endif
	vrConfig->num_pipes = pfGetMultipipe();
	vrConfig->num_pipes_used = 0;

	/* NOTE: Some versions of Linux on Performer improperly return without warning that */
	/*   the multipipe option is not available.  Specifically, version 2.3 works, but   */
	/*   version 2.5 does not.  When working, the call to pfMultipipe() should return   */
	/*   0, and the call to pfGetMultipipe() should return 1.  Thus, we will force the  */
	/*   correct values here, unless the PFLINUXMULTIPIPE environment variable is set.  */
	/* NOTE: 07/05/2006 -- current version of Performer (3.2.2) seems to work fine on Linux (SuSE 9.0 on Prism) */
#if defined(__linux) && 0	/* BS: 07/05/2006 -- setting to "0" now, since current version of Performer on Linux seems to work as defined */
	if (!getenv("PFLINUXMULTIPIPE") && vrConfig->num_windows > 1 && return_pipe == 1) {
		vrDbgPrintfN(PERFORMER_DBGLVL, "_PfInitFunc(): " RED_TEXT "NOTE: this version of Linux improperly accepted more than one pipe -- correcting.\n" NORM_TEXT);
		return_pipe = pfMultipipe(1);
		if (return_pipe < 1) {
			vrDbgPrintfN(AALWAYS_DBGLVL, "_PfInitFunc(): " RED_TEXT "WARNING!  Unable to reset the number of pipes to 1.\n" NORM_TEXT);
		}
		return_pipe = 0;
		vrConfig->num_pipes = pfGetMultipipe();
	}
#endif

	if (return_pipe < 1) {
		vrDbgPrintfN(AALWAYS_DBGLVL, "_PfInitFunc(): " RED_TEXT "WARNING! Requested number of pipes not granted.  req: %d, grant: %d\n" NORM_TEXT, vrConfig->num_windows, vrConfig->num_pipes);
	}


	/* TODO: if we were to have a single place to store primary Performer */
	/*   information, then this would be a good place to set the channel  */
	/*   mask for how shared channels are linked together.                */
	/* As it stands, the channel mask stuff is done each time a window    */
	/*   is opened.                                                       */
}


/***************************************************************************/
/* set a window frame buffer configuration through this callback . */
/* Here we:                                             */
/*      - TODO: turn off screen saver                   */
/*      - DONE: lock the process                        */
/*      - DONE: execute any window 'start' commands     */
/*	- DONE: set the cursor                          */
/*	- PARTIAL: determine and set the proper GLX visual */
/*	- DONE: open the window via pfOpenPWin()        */
/*                                                      */
/* - NOTE: a pointer to the FreeVR:vrWindowInfo structure  */
/*      for the "window" associated with this pfPipeWindow */
/*	is passed as a pfObject's UserData.                */
static void _PfPipeWindowDrawProcessInitialization(pfPipeWindow *pipe_window)
{
	pfFBConfig	visual;		/* same as X-windows type (XVisualInfo *) */
	vrWindowInfo	*window;
	vrGlxPrivateInfo *aux;
	Status		status;		/* X status return value */
#if 0 /* these aren't currently used */
	Display		*display;	/* same as X-windows type (Display) */
	Window		win;
	int		x_screen;	/* NOTE: not currently used, set, but not used */
#endif


	window = pfGetUserData(pipe_window);
	aux = (vrGlxPrivateInfo *)window->aux_data;


	/*********************************************************/
	/* get the proper visual, and then call pfPWinFBConfig() */
	/* TODO: for now, always choosing stereo visual, if available. */
	visual = pfChoosePWinFBConfig(pipe_window, /* display, xscreen, */ stereo_buf_attribs);
	pfPWinFBConfig(pipe_window, visual);
	aux->xvisual = visual;

	/* TODO: need to set "window->dualeye_buffer" to appropriate value here */
	window->dualeye_buffer = 1;	/* just a hack for now -- needs to be based on the visual that Performer returns -- or perhaps query performer for the correct value. */

	/*******************/
	/* Open the Window */
	vrTrace("_PfPipeWindowDrawProcessInitialization():", BOLD_TEXT "About to setup Performer Pipe-Window" NORM_TEXT);
	pfOpenPWin(pipe_window);				/* C++: pipe_window->open(); */

printf("pipe = '%x'\n", pipe);	/* !!! I have no idea where "pipe" is defined! */
printf("aux->xscreen = '%x'\n", aux->xscreen);
#if 0 /* BS: 20070131 test -- doing this gets rid of some errors, but still doesn't work. *//* TODO: Do we need to do this?  Was being done in the _PfOpenFunc() */
printf("aux->xdisplay = '%x'\n", aux->xdisplay);
/*	pfPipeWSConnectionName(pipe, aux->xdisplay);	/* C++: pipe->setWSConnectionName(aux->xdisplay); */
/*	pfPipeWSConnectionName(pipe, "");	/* C++: pipe->setWSConnectionName(aux->xdisplay); */
	pfPipeScreen(pipe, aux->xscreen);		/* C++: pipe->setScreen(aux->xscreen); */
/* BS: 20070131 -- hmmm, okay, it seems that the call to pfPipeWSConnectionName() here is actually */
/*   causing the process to crash, which explains why we don't get any errors after this point.    */
/*   Also, calling pfPipeScreen() here seems to cause a process termination at this point.         */
/*   So, to answer the question: it seems like "no" we do not want to do this here!                */
/*   Furhtermore!  I have no idea where the variable "pipe" is declared, much let set.             */
#endif

	/*****************************************************/
	/* Set some values in the auxiliary window structure */
	aux->pipewindow = pipe_window;
	aux->xdisplay = pfGetCurWSConnection();	/* a Performer "*pfWSConnection" type is an Xwindows "Display" type */
printf("NOW: aux->xdisplay = '%x'\n", aux->xdisplay);
printf("NOW: aux->xscreen = '%x'\n", aux->xscreen);
	vrDbgPrintfN(PERFORMER_DBGLVL, RED_TEXT "pfGetCurWSConnection() reports display '%s'\n" NORM_TEXT, DisplayString(aux->xdisplay));

	aux->xdisplay_string = vrShmemStrDup(DisplayString(aux->xdisplay));
printf("NOW: aux->xdisplay_string = '%s'\n", aux->xdisplay_string);
	if (aux->xscreen < 0)
		aux->xscreen = DefaultScreen(aux->xdisplay);

	aux->xwindow = pfGetPWinWSWindow(pipe_window);	/* a Performer "pfWSWindow" type is an Xwindows "Window" type */

#if 0
	x_screen = pfGetPWinScreen(pipe_window);	/* NOTE: not currently used */
	aux->glx_context = pfGetPWinGLCxt(pipe_window);
#endif

	aux->mapped = 1;

#if 1 /* 6/5/03: test */
vrPrintf("Hey: pfIsPWinOpen(pipe_window) = %d\n", pfIsPWinOpen(pipe_window));
#endif
#if 0  /* NOTE: 6/5/03: if multiple windows are assigned to the same process, then only one will appear, causing problems with the colormap.  NOTE: [07/05/2006] it's possible that the problem with this code is that it is not reentrant, and so must be projected with a lock. */
	/**************************************/
	/* Create a colormap for the X window */
	/* NOTE: this is used by the cursor making routine */
	aux->xcolormap = XCreateColormap(aux->xdisplay,
		RootWindow(aux->xdisplay, aux->xscreen),
		aux->xvisual->visual,
		AllocNone	/* try AllocAll for grayscale tests */
		);

	/******************/
	/* Set the Cursor */
	aux->cursor = 255;
	pfuCursor(vrXwindowsMakeCursor(aux, aux->cursor_name), aux->cursor);
	pfuLoadPWinCursor(pipe_window, aux->cursor);
#endif

	/***********************/
	/* Initialize X-inputs */

	status = XSelectInput(aux->xdisplay, aux->xwindow, EnterWindowMask | LeaveWindowMask | KeyPressMask);
	if (status != 1) {
		vrDbgPrintfN(AALWAYS_DBGLVL, "_PfPipeWindowDPI(): XSelectInput returned status %d\n", status);
	}
}


/* The opening and initialization function -- this line is to help diff match */
/**************************************************************************/
/* _PfOpenFunc() initializes the auxiliary window info for the specified  */
/* window.                                                                */
/**************************************************************************/
static void _PfOpenFunc(vrWindowInfo *window)
{
static	char		trace_msg[256];
	vrConfigInfo	*vrConfig = window->context->config;
	vrGlxPrivateInfo *aux = NULL;
	char		xdisplay_name[64];
	vrRenderInfo	*chan_data;
	int		count_eye;
#if 0
	int		count;			/* used to loop through existing pipes (when pfMultipipe() works) */
#endif

	/* Performer variables */
	int		use_pipe;		/* Number of the pipe to use for this window */
	pfPipe		*pipe;
	pfPipeWindow	*pipe_window;
	pfPipeWindow	*pw_of_new_chan;	/* the pipewindow of a newly created channel */
	pfChannel	*channel;
	unsigned int	channel_mask;

	sprintf(trace_msg, BOLD_TEXT "Entering for window '%s' %#p\n" NORM_TEXT, window->name, window);
	vrTrace("_PfOpenFunc", trace_msg);

	/****************************************/
	/*** initialize memory for the window ***/
	/****************************************/

	window->aux_data = (void *)vrShmemAlloc0(sizeof(vrGlxPrivateInfo));
	aux = (vrGlxPrivateInfo *)window->aux_data;


	/*************************************/
	/*** set/get the window parameters ***/
	/*************************************/

	/* set some default values */
	aux->xhost = NULL;
	aux->xserver = -1;
	aux->xscreen = -1;
	aux->decorations = DECORATION_ALL;	/* default to all decorations */
	aux->xsize_hints.x = 10;		/* TODO: ideally x, y, width & height would be part */
	aux->xsize_hints.y = 10;		/*   of the overall window structure, and would be  */
	aux->xsize_hints.width = 200;		/*   given initial values in vr_visren.c.           */
	aux->xsize_hints.height = 200;

	if (window->mount == VRWINDOW_SIMULATOR)
		aux->cursor_name = vrShmemStrDup("default");	/* the simulator default cursor */
	else	aux->cursor_name = vrShmemStrDup("bigdot");	/* the default default cursor */

	aux->window_title = (char *)vrShmemAlloc(30 + strlen(window->name));
	sprintf(aux->window_title, "FreeVR(%d): %s window", getpid(), window->name);

	/* parse the arguments to override defaults            */
	/*   this will fill in aux->xserver, aux->xscreen, etc. */
	_GlxParseArgs(aux, window->args);


	/* create the channels for viewing the world */

	if (aux->xhost == NULL) {
		sprintf(xdisplay_name, ":0.0");
	} else {
		sprintf(xdisplay_name, "%s:%d.%d", aux->xhost, aux->xserver, aux->xscreen);
	}

	/**********************/
	/* Do Performer stuff */
	/**********************/

	/* first need to check whether the hardware-screen used by this */
	/*   FreeVR-window (aux->xscreen) is already tied to a pfPipe.  */
	use_pipe = -1;
#if 1 /* this code should be used, but only if pfMultipipe() specifies the correct number of pipes */
	/* NOTE: 6/5/03: this is necessary to get multiple pipes on the same screen to work */
{ int count;
	for (count = 0; count < vrConfig->num_pipes_used; count++) {

		if (pfGetPipeScreen(pfGetPipe(count)) == aux->xscreen) {	/* TODO: this is wrong!  Only paying attention to the screen part of the display name */
			use_pipe = count;
		}
	}
}
#endif

	/****************/
	/* get the pipe */
	if (use_pipe != -1) {
		/* using an existing pipe */
		pipe = pfGetPipe(use_pipe);
		vrDbgPrintfN(PERFORMER_DBGLVL, "_PfOpenFunc(): " BOLD_TEXT "Reusing pipe %d.\n" NORM_TEXT, use_pipe);
	} else {
		/* need to get a new pipe, but first make sure one is available on this system */
		if (vrConfig->num_pipes_used > vrConfig->num_pipes-1) {
			/* No pipes available */
			vrDbgPrintfN(PERFORMER_DBGLVL, "_PfOpenFunc(): " BOLD_TEXT "Trying to allocate pipe %d, but only %d pipes available.  Will try reusing pipe 0.\n" NORM_TEXT, vrConfig->num_pipes_used, vrConfig->num_pipes);
			pipe = pfGetPipe(0);
		} else {
			/* Get the next pipe */
			use_pipe = vrConfig->num_pipes_used++;
			pipe = pfGetPipe(use_pipe);
printf("YO: aux->xdisplay = %x; pipe = %x\n", aux->xdisplay, pipe);
#if 0 /* BS: 20070131 test (set to 1 for the test) */
printf("_PfOpenFunc(): pipe = %x, use_pipe = %d\n", pipe, use_pipe);
pfPipeWSConnectionName(pipe, aux->xdisplay);
#endif
			pfPipeScreen(pipe, aux->xscreen);	/* C++: pipe->setScreen(aux->xscreen); */
			vrDbgPrintfN(PERFORMER_DBGLVL, "_PfOpenFunc(): " BOLD_TEXT "Using pipe %d (%#p) for the first time.\n" NORM_TEXT, use_pipe, pipe);
		}
	}
	if (pipe == NULL) {
		vrDbgPrintfN(AALWAYS_DBGLVL, "_PfOpenFunc(): " RED_TEXT "Unable to open window '%s' on pipe %d -- WINDOW WILL NOT APPEAR!\n" NORM_TEXT, window->name, use_pipe);
		return;
	}

	/*****************************************************/
	/* now that we have a pfPipe, we can open the window */

	pipe_window = pfNewPWin(pipe);
	vrDbgPrintfN(PERFORMER_DBGLVL, "_PfOpenFunc(): " BOLD_TEXT "creating new pipewindow (%#p) for window '%s', pipe %#p\n" NORM_TEXT, pipe_window, window->name, pipe);

	pfPWinType(pipe_window, PFPWIN_TYPE_X);		/* C++: pipe_window->setWinType(PFPWIN_TYPE_X); */
	pfPWinName(pipe_window, aux->window_title);	/* C++: pipe_window->setName("pfSimple: Pipe 0"); */

	/* Set the window origin to be the upper-left (ie. standard Xwindows) */
	pfPWinMode(pipe_window, PFWIN_ORIGIN_LL, 0);	/* C++: ... */

	pfPWinOriginSize(pipe_window, aux->xsize_hints.x, aux->xsize_hints.y, aux->xsize_hints.width, aux->xsize_hints.height);	/* C++: pipe_window->setOriginSize(...); */
	if (!(aux->decorations & DECORATION_BORDER) && !(aux->decorations & DECORATION_TITLE)) {
		pfPWinMode(pipe_window, PFWIN_NOBORDER, 1);
	}
	/* TODO: do we care if we can't independently turn the title bar off from the borders? */

	/* NOTE: here we are in the Performer App-process.  There are several things  */
	/*   that must be initialized in the Draw-process.  Those things are done via */
	/*   the callback to _PfPipeWindowDrawProcessInitialization().                */
	pfPWinConfigFunc(pipe_window, _PfPipeWindowDrawProcessInitialization);

	/* Now assign the FreeVR-window object as the UserData for this pfPipeWindow */
	pfUserData(pipe_window, window);

	/* TODO: any other callbacks to set?  Swap-function? (may be more important on multi-machine setups) */

	pfConfigPWin(pipe_window);

#if 0 /* 6/5/03: test -- seems to always claim to be open by now anyway.  I also tried putting this at the bottom of _PfOpenFunc() */
	while (!pfIsPWinOpen(pipe_window)) pfFrame();
#endif

	vrDbgPrintfN(SELDOM_DBGLVL, "_PfOpenFunc(): pfDRAW pid for Pipe %d is %d\n", use_pipe, pfGetPID(use_pipe, PFPROC_DRAW));
	vrDbgPrintfN(SELDOM_DBGLVL, "_PfOpenFunc(): pid name for PID %d is '%s'\n", pfGetPID(use_pipe, PFPROC_DRAW), pfGetPIDName(pfGetPID(use_pipe, PFPROC_DRAW)));


	/* fill in the window-info structure with the correct viewport details */
	window->geometry.origX = aux->xsize_hints.x;
	window->geometry.origY = aux->xsize_hints.y;
	window->geometry.width = aux->xsize_hints.width;
	window->geometry.height = aux->xsize_hints.height;

	/* Calculate the left viewport values */
	if (window->viewport_left.origX == -1) {
		/* Calculate the absolute viewport in OpenGL terms from the fractional viewports. */
		/* These absolute pixel values will be what is used to set the glViewport().      */
		/* NOTE: fractional viewports default to 0.0 -> 1.0 when not specified. */
		window->viewport_left.origX =  window->geometry.width  *  (int)window->viewportF_left.min_X;
		window->viewport_left.width =  window->geometry.width  * (int)(window->viewportF_left.max_X - window->viewportF_left.min_X);
		window->viewport_left.origY =  window->geometry.height *  (int)window->viewportF_left.min_Y;
		window->viewport_left.height = window->geometry.height * (int)(window->viewportF_left.max_Y - window->viewportF_left.min_Y);
		vrDbgPrintfN(PERFORMER_DBGLVL, "viewport_left = %d %d %d %d\n", window->viewport_left.origX, window->viewport_left.width, window->viewport_left.origY, window->viewport_left.height);
	} else {
		/* Calculate the fractional viewport size from the given absolute viewport. */
		/* Later, when the window is resized, the fractional viewport values will   */
		/*   will used to determine the pixel viewport values.                      */

		/* NOTE: I just figured these out by algebraically manipulating the above equations */
		/* NOTE also: need to force the divisions to be floating point operations. */
		window->viewportF_left.min_X =  (float)window->viewport_left.origX / window->geometry.width;
		window->viewportF_left.max_X = ((float)window->viewport_left.width / window->geometry.width) + window->viewportF_left.min_X;
		window->viewportF_left.min_Y =  (float)window->viewport_left.origY / window->geometry.height;
		window->viewportF_left.max_Y = ((float)window->viewport_left.height / window->geometry.height) + window->viewportF_left.min_Y;
		vrDbgPrintfN(PERFORMER_DBGLVL, "geometry = %d %d %d %d\n", window->geometry.origX, window->geometry.origY, window->geometry.width, window->geometry.height);
		vrDbgPrintfN(PERFORMER_DBGLVL, "viewport_left = %d %d %d %d\n", window->viewport_left.origX, window->viewport_left.origY, window->viewport_left.width, window->viewport_left.height);
		vrDbgPrintfN(PERFORMER_DBGLVL, "viewportF_left = %f %f %f %f\n", window->viewportF_left.min_X, window->viewportF_left.max_X, window->viewportF_left.min_Y, window->viewportF_left.max_Y);
	}

	/* Now do the same with the right viewport */
	if (window->viewport_right.origX == -1) {
		window->viewport_right.origX =  window->geometry.width  *  (int)window->viewportF_right.min_X;
		window->viewport_right.width =  window->geometry.width  * (int)(window->viewportF_right.max_X - window->viewportF_right.min_X);
		window->viewport_right.origY =  window->geometry.height *  (int)window->viewportF_right.min_Y;
		window->viewport_right.height = window->geometry.height * (int)(window->viewportF_right.max_Y - window->viewportF_right.min_Y);
	} else {
		window->viewportF_right.min_X =  (float)window->viewport_right.origX / window->geometry.width;
		window->viewportF_right.max_X = ((float)window->viewport_right.width / window->geometry.width) + window->viewportF_right.min_X;
		window->viewportF_right.min_Y =  (float)window->viewport_right.origY / window->geometry.height;
		window->viewportF_right.max_Y = ((float)window->viewport_right.height / window->geometry.height) + window->viewportF_right.min_Y;
	}


	/****************************************************************************/
	/* Now loop over each eye for this window and setting up a channel for each */

	/* create an array of pointers to all the pfChannels */
	aux->pfchan = (pfChannel **)vrShmemAlloc0(window->num_eyes * sizeof(pfChannel *));
	vrDbgPrintfN(PERFORMER_DBGLVL, RED_TEXT "Just allocated aux->pfchan = %p\n", aux->pfchan);

	for (count_eye = 0; count_eye < window->num_eyes; count_eye++) {

		vrDbgPrintfN(PERFORMER_DBGLVL, "_PfOpenFunc(): creating new channel for window %s, eye %d, pipe %#p\n", window->name, count_eye, pipe);
		channel = pfNewChan(pipe);			/* C++: channel = new pfChannel(pipe); */

		/* if the channel was added to the wrong pipe_window, then move to the correct one */
		/* NOTE: this will always happen when more than one pfPipeWindow is mapped to the  */
		/*   same pfPipe -- all the pfChannels for that pipe will go to the first window.  */
		pw_of_new_chan = pfGetChanPWin(channel);
		if (pw_of_new_chan != pipe_window) {
#if 0 /* NOTE: would remove first, but pfRemoveChan() is broken! -- but pfAddChan() will do the remove for us */
			vrDbgPrintfN(PERFORMER_DBGLVL, "_PfOpenFunc(): moving channel from pfPipewindow %#p to %#p.\n", pw_of_new_chan, pipe_window);
			if (pfRemoveChan(pw_of_new_chan, channel) < 0) {
				vrDbgPrintfN(ALWAYS_DBGLVL, "_PfOpenFunc(): " RED_TEXT "unable to remove channel from pfPipewindow %#p.\n", pw_of_new_chan);
			}
#endif
			/* NOTE: pfAddChan() will also remove the channel from its current pipewindow */
			if (pfAddChan(pipe_window, channel) < 0) {
				vrDbgPrintfN(ALWAYS_DBGLVL, "_PfOpenFunc(): " RED_TEXT "unable to add channel to pfPipewindow %#p.\n", pipe_window);
			}
#if 0 /* NOTE: pfRemoveChan() is broken */
			if (pfRemoveChan(pw_of_new_chan, channel) < 0) {
				vrDbgPrintfN(ALWAYS_DBGLVL, "_PfOpenFunc(): " RED_TEXT "unable to remove channel from pfPipewindow %#p.\n", pw_of_new_chan);
			}
#endif

		}
		vrDbgPrintfN(PERFORMER_DBGLVL, RED_TEXT "POST: %d channels now attached to pipewindow %p and %d to pw %p\n" NORM_TEXT, pfGetNumChans(pipe_window), pipe_window, pfGetNumChans(pw_of_new_chan), pw_of_new_chan);

		pfChanFOV(channel, 90.0, 65.0);			/* C++: channel->setFOV(90.0, 65.0); */
		pfChanNearFar(channel, 1.0f, 8000.0f);		/* C++: channel->setNearFar(1.0f, 8000.0f); */
		pfChanAutoAspect(channel, PFFRUST_CALC_NONE);	/* C++: channel->setAutoAspect(PFFRUST_CALC_NONE); */
		switch (window->eyes[count_eye]->type) {
		case VREYE_DEFAULT:
		case VREYE_CYCLOPS:
			/* cyclops eye fills entire screen */
			pfChanViewport(channel, 0.0, 1.0, 0.0, 1.0);
			break;

		case VREYE_LEFT:
			pfChanViewport(channel, window->viewportF_left.min_X, window->viewportF_left.max_X, window->viewportF_left.min_Y, window->viewportF_left.max_Y);
			break;

		case VREYE_RIGHT:
			pfChanViewport(channel, window->viewportF_right.min_X, window->viewportF_right.max_X, window->viewportF_right.min_Y, window->viewportF_right.max_Y);
			break;
		}

		/****************************/
		/* setup the shared channel */

		if (vrPfMasterChannel() == NULL) {
			vrDbgPrintfN(PERFORMER_DBGLVL, "_PfOpenFunc(): setting the channel mask for the Master Channel\n");
			/* we get here only for the first channel */
			/* NOTE: this assumes that window 0 is handled first */

			/* first set the property-sharing-mask */
			channel_mask = pfGetChanShare(channel);	/* C++: channel_mask = channel->getShare(); */
			channel_mask |= PFCHAN_SCENE;
			channel_mask |= PFCHAN_SWAPBUFFERS;
			channel_mask |= PFCHAN_APPFUNC;
			channel_mask |= PFCHAN_CULLFUNC;
			channel_mask |= PFCHAN_DRAWFUNC;
			channel_mask |= PFCHAN_EARTHSKY;
			channel_mask |= PFCHAN_NEARFAR;
			/*   and those that are *not* shared */
			channel_mask &= ~PFCHAN_VIEW;
			channel_mask &= ~PFCHAN_FOV;
			channel_mask &= ~PFCHAN_VIEW_OFFSETS;	/* optional: was added to try to fix lighting problems -- didn't work */
			channel_mask &= ~PFCHAN_VIEWPORT;	/* optional: was added to try to fix lighting problems -- didn't work */
			pfChanShare(channel, channel_mask);	/* C++: channel->setShare(channel_mask); */

			pfChanTravFunc(channel, PFTRAV_DRAW, _PfRenderFunc);

			/* TODO: the near/far stuff can be set for the one channel here */

		} else {
			/* all other channels are attached to the "MasterChannel" */
			vrDbgPrintfN(PERFORMER_DBGLVL, "_PfOpenFunc(): attaching new channel to the Master Channel\n");
			pfAttachChan(vrPfMasterChannel(), channel);/* C++: vrPfMasterChannel()->attach(channel); */
		}

		/* set the window info as the data passed to the rendering callback */
		chan_data = (vrRenderInfo *)pfAllocChanData(channel, sizeof(vrRenderInfo));
		chan_data->context = window->context;
		chan_data->persp = (vrPerspData *)vrShmemAlloc0(sizeof(vrPerspData));
		chan_data->window = window;
		chan_data->eye = window->eyes[count_eye];
		chan_data->frame_stime = 0;
		chan_data->frame_count = -1;	/* We haven't even begun to start rendering frames */
		pfPassChanData(channel);

		aux->pfchan[count_eye] = channel;
	}
}
/* end of open function -- for diff match */


/*****************************************************************/
/* NOTE: called in the Performer APP process */
/* NOTE: this function is called once per group-rendering, thus it */
/*   handles the same functionality as the last-to-sync process of */
/*   the standard OpenGL form of rendering.                        */
/* TODO: in a future version we may have "context" as an argument, */
/*   but for now we get the context from the global value.         */
void vrPfPreFrame(void)
{
	vrContextInfo	*context;
	vrWindowInfo	*window;
	vrGlxPrivateInfo *aux;
	pfChannel	*curr_channel;
	vrRenderInfo	*renderinfo;
	vrPerspData	*pd;
	vrMatrix	head_rwpos;
	vrMatrix	eye_rwpos;
	vrUserInfo	*curr_user;
	vrEyeInfo	*curr_eye;
	pfMatrix	pf_rw2w;
	pfMatrix	pf_eye2w;
	pfMatrix	pf_w2eye;
	pfMatrix	zup_preadjust;
	pfMatrix	zup_postadjust;
	int		count;
	int		count_eye;

	vrTrace("vrPfPreFrame", BOLD_TEXT "Entering vrPfPreFrame()" NORM_TEXT);

	/* set the OGL<-->PF conversion matrices */
	pfMakeRotMat(zup_preadjust,  -90.0, 1.0, 0.0, 0.0);
	pfMakeRotMat(zup_postadjust,  90.0, 1.0, 0.0, 0.0);


	/* for now we get the context from the global value */
	context = vrContext;		/* TODO: get this as an argument */

	/***********************/
	/** freeze the inputs **/
	vrTrace("vrPfPreFrame", "before freezing the input/travel/etc data");
	vrInputFreezeVisren(context);
	vrUserTravelFreezeVisren(context);
	vrPropFreezeVisren(context);
	vrTrace("vrPfPreFrame", "after freeze");

	/***********************************************/
	/** PERHAPS: share data across a cluster here **/

	/*****************************************/
	/** calculate perspective matrices here **/

	/* we loop through all the windows making the proper perspective calculations for each. */
	for (count = 0; count < context->config->num_windows; count++) {
		window = context->config->windows[count];
		aux = (vrGlxPrivateInfo *)window->aux_data;

		/* TODO: we need to set the channel parameters for each eye [4/23/02 -- don't we do this now?] */
		for (count_eye = 0; count_eye < window->num_eyes; count_eye++) {
			if (aux->pfchan == NULL) {
				vrDbgPrintfN(AALWAYS_DBGLVL, "vrPfPreFrame(): " RED_TEXT "WARNING! There is no pfChannel to render to for window %d, eye %d\n" NORM_TEXT, count, count_eye);
				continue;
			}

			curr_channel = aux->pfchan[count_eye];
			renderinfo = (vrRenderInfo *)pfGetChanData(curr_channel);
			renderinfo->frame_count++;
			/* TODO: set renderinfo->frame_stime to the correct (perhaps frozen) value */
			pd = renderinfo->persp;		/* store the perspective-data in the channel data memory */

			if (curr_channel == NULL) {
				vrDbgPrintfN(AALWAYS_DBGLVL, "vrPfPreFrame(): hmmm, window %d,eye %d has no Performer channel\n", count, count_eye);
				continue;
			}

			curr_eye = window->eyes[count_eye];
			curr_user = curr_eye->user;
			vrMatrixGet6sensorValuesDirectNoLastUpdate(&head_rwpos, curr_user->head);
			vrMatrixCopy(&eye_rwpos, &head_rwpos);
			switch (curr_eye->type) {
			case VREYE_DEFAULT:
			case VREYE_CYCLOPS:
				/* NOP: leave the eye at the generic head position */
				break;
			case VREYE_LEFT:
				vrMatrixPostTranslate3d(&eye_rwpos, -curr_user->iod * 0.5, 0.0, 0.0);
				break;
			case VREYE_RIGHT:
				vrMatrixPostTranslate3d(&eye_rwpos,  curr_user->iod * 0.5, 0.0, 0.0);
				break;

			default:
				/* TODO: error */
				break;
			}


			/* get and set the viewing frustum */
			vrCalcPerspFrustumEye(pd, window, &eye_rwpos);

			/* In the GL version, we test to make sure we have valid frustum  */
			/*   values here by comparing "left" to negative infinity (ie.    */
			/*   -HUGE_VAL).  However Performer seems to handle this case     */
			/*   itself, so for now we'll do nothing in the Performer version */
			/*   of the code.                                                 */
			pfMakePerspChan(curr_channel,
				pd->frustum.n.left,
				pd->frustum.n.right,
				pd->frustum.n.bottom,
				pd->frustum.n.top);
			pfChanNearFar(curr_channel,
				pd->frustum.n.near_clip,
				pd->frustum.n.far_clip);

			/* calculate and set the viewing matrix */
#if 1 /* { */
			pf_rw2w[0][0] = window->rw2w_xform->v[ 0];
			pf_rw2w[0][1] = window->rw2w_xform->v[ 1];
			pf_rw2w[0][2] = window->rw2w_xform->v[ 2];
			pf_rw2w[0][3] = window->rw2w_xform->v[ 3];
			pf_rw2w[1][0] = window->rw2w_xform->v[ 4];
			pf_rw2w[1][1] = window->rw2w_xform->v[ 5];
			pf_rw2w[1][2] = window->rw2w_xform->v[ 6];
			pf_rw2w[1][3] = window->rw2w_xform->v[ 7];
			pf_rw2w[2][0] = window->rw2w_xform->v[ 8];
			pf_rw2w[2][1] = window->rw2w_xform->v[ 9];
			pf_rw2w[2][2] = window->rw2w_xform->v[10];
			pf_rw2w[2][3] = window->rw2w_xform->v[11];
			pf_rw2w[3][0] = window->rw2w_xform->v[12];
			pf_rw2w[3][1] = window->rw2w_xform->v[13];
			pf_rw2w[3][2] = window->rw2w_xform->v[14];
			pf_rw2w[3][3] = window->rw2w_xform->v[15];

			pfPostTransMat(pf_eye2w, pfMat_NoWarn pf_rw2w, -(float)(pd->eye[VR_X]), -(float)(pd->eye[VR_Y]), -(float)(pd->eye[VR_Z]));	/* perhaps Pre */

			/* vrJuggler ZUP adjustment */
			pfInvertFullMat(pf_w2eye, pf_eye2w);		/* TODO: I think this can be less than a full mat */

			/* Basically what these two lines do is make the operation pf_w2eye      */
			/*   happen in OpenGL space.  This is the same as doing a transformation */
			/*   to OpenGL space, doing the operation in OpenGL space, and then      */
			/*   transforming back to Performer space.  Only we use pre and post     */
			/*   multiplies to do all the conversion stuff right here.               */
			pfPreMultMat(pf_w2eye, pfMat_NoWarn zup_preadjust);
			pfPostMultMat(pf_w2eye, pfMat_NoWarn zup_postadjust);
#else /* } { TODO: test the following -- this is an attempt at a cleaner version of the above */
			vrPfMatrixFromVrMatrix(&pf_rw2w, window->rw2w_xform);
			pfCopyMat(pf_w2rw, pfMat_NoWarn pf_rw2w);
			pfPreMultMat(pf_w2rw, pfMat_NoWarn zup_preadjust);
			pfPostMultMat(pf_w2rw, pfMat_NoWarn zup_postadjust);
{
pfMatrix tmp_mat;
			pfInvertFullMat(tmp_mat, pf_w2rw);		/* TODO: I think this can be less than a full mat */
			pfCopyMat(pf_w2rw, pfMat_NoWarn tmp_mat);
}
#  if 0
			pfPostTransMat(pf_w2rw, pfMat_NoWarn pf_rw2w, -(float)(pd->eye[VR_X]), -(float)(pd->eye[VR_Y]), -(float)(pd->eye[VR_Z]));	/* perhaps Pre */
#  else
			pfPreTransMat(pf_w2rw, -(float)(pd->eye[VR_X]), -(float)(pd->eye[VR_Y]), -(float)(pd->eye[VR_Z]), pfMat_NoWarn pf_rw2w);	/* perhaps Post */
#  endif
#endif /* } */

			pfChanViewMat(curr_channel, pf_w2eye);
		}
	}


	/****************************************************/
	/* TODO: also may want to update any callbacks here -- though */
	/*   we may already be handling that elsewhere.               */

#if 0
	/* TODO: should this be called here?  It seems like a logical place for it. */
	/*   [9/17/02] except that the function we're in is in the APP process,     */
	/*   and the VisrenFrame probably should be in the DRAW process.  So really */
	/*   this callback should probably be made in _PfRenderFunc() somewhere     */
	/*   before the call to pfDraw().                                           */
	vrCallbackInvoke(context->callbacks->VisrenFrame);
#endif
}


/*****************************************************************/
void vrPfPostFrame(void)
{
	vrTrace("vrPfPostFrame", BOLD_TEXT "Entering vrPfPostFrame()" NORM_TEXT);
	/* not sure if we need to do anything here -- currently exists to match CL API */
}


/*****************************************************************/
pfChannel *vrPfMasterChannel(void)
{
	vrWindowInfo	*window;
	vrGlxPrivateInfo *aux;

	vrTrace("vrPfMasterChannel", BOLD_TEXT "Entering vrPfMasterChannel()" NORM_TEXT);

	window = vrContext->config->windows[0];
	aux = (vrGlxPrivateInfo *)window->aux_data;

	return aux->pfchan[0];
}


/*****************************************************************/
void vrPfPostDraw(void)
{
	vrTrace("vrPfPostDraw", BOLD_TEXT "Entering vrPfPostDraw()" NORM_TEXT);

	/* TODO: this is where the simulator rendering will go. */
	/*   I think we may have to call this from our own replacement of pfDraw() */

	/* 12/28/01 -- Hmmm, the simulator call is now in our pfDraw() routine, */
	/*   this routine is for the app-prog to call, I guess.  Not sure what  */
	/*   this may be used for at the moment.                                */
}


/*****************************************************************/
/* NOTE: directly setting the matrix to the Sensor DCS loses any */
/*   XXscaleXX, translation, and rotation already in the DCS. */

/* DONE: the scale of the DCS should be preserved, if */
/*   possible -- that's what other libraries do.      */
void vrPfDCSTransform6sensor(pfDCS *dcs, int sensor6_num)
{
static  vrMatrix        sensor6_mat;
static	pfMatrix	pfmat;
	pfMatrix	zup_preadjust;
	pfMatrix	zup_postadjust;
	double		scale;

	/* preserve the scale value so we can restore it later */
	/* NOTE: we use "sensor6_mat" here just as a temporary memory holder of the vrMatrix. */
	scale = vrMatrixGetScale(vrMatrixFromPfMatrix(&sensor6_mat, (pfMatrix *)pfGetDCSMatPtr(dcs)));

#ifdef PF_YUP /* old method with FreeVR reporting in Y-up coordinates */
	/* get the sensor matrix into a Performer pfMatrix */
	vrMatrixGet6sensorValues(&sensor6_mat, sensor6_num);
	vrPfMatrixFromVrMatrix(&pfmat, &sensor6_mat);

	/* convert to Zup coord-sys using the vrJuggler technique (from the perspective calculation) */
	pfMakeRotMat(zup_preadjust,  -90.0, 1.0, 0.0, 0.0);	/* TODO: only do once */
	pfMakeRotMat(zup_postadjust,  90.0, 1.0, 0.0, 0.0);	/* TODO: only do once */
	pfPreMultMat(pfmat, pfMat_NoWarn zup_preadjust);
	pfPostMultMat(pfmat, pfMat_NoWarn zup_postadjust);
#else
	/* get the sensor matrix into a Performer pfMatrix */
	vrPfMatrixGet6sensorValues(&sensor6_mat, sensor6_num);
	vrPfMatrixFromVrMatrix(&pfmat, &sensor6_mat);
#endif

	/* set the calculated matrix as the DCS transformation */
	pfDCSMat(dcs, pfmat);				/* in C++ dcs->setMat(pfmat); */

	/* restore the scale factor without altering rotation or translation */
	pfDCSScale(dcs, scale);				/* in C++ dcs->setScale(scale); */
}


/*****************************************************************/
void vrPfDCSTransformUserTravel(pfDCS *dcs, vrUserInfo *user)
{
static  vrMatrix        *travel_mat;
static	pfMatrix	pfmat;
	pfMatrix	zup_preadjust;
	pfMatrix	zup_postadjust;

	travel_mat = user->rw2vw_xform;

	/* TODO: use vrPfMatrixFromVrMatrix() instead */
	pfmat[0][0] = travel_mat->v[ 0];
	pfmat[0][1] = travel_mat->v[ 1];
	pfmat[0][2] = travel_mat->v[ 2];
	pfmat[0][3] = travel_mat->v[ 3];
	pfmat[1][0] = travel_mat->v[ 4];
	pfmat[1][1] = travel_mat->v[ 5];
	pfmat[1][2] = travel_mat->v[ 6];
	pfmat[1][3] = travel_mat->v[ 7];
	pfmat[2][0] = travel_mat->v[ 8];
	pfmat[2][1] = travel_mat->v[ 9];
	pfmat[2][2] = travel_mat->v[10];
	pfmat[2][3] = travel_mat->v[11];
	pfmat[3][0] = travel_mat->v[12];
	pfmat[3][1] = travel_mat->v[13];
	pfmat[3][2] = travel_mat->v[14];
	pfmat[3][3] = travel_mat->v[15];

	/* use the vrJuggler technique (from the perspective calculation) */
	pfMakeRotMat(zup_preadjust,  -90.0, 1.0, 0.0, 0.0);
	pfMakeRotMat(zup_postadjust,  90.0, 1.0, 0.0, 0.0);
	pfPreMultMat(pfmat, pfMat_NoWarn zup_preadjust);
	pfPostMultMat(pfmat, pfMat_NoWarn zup_postadjust);

	pfDCSMat(dcs, pfmat);			/* in C++ dcs->setMat(pfmat); */
}


/*****************************************************************/
/* This function is a callback function that can be tied to a pfDCS */
/*   node as a PFCULL callback.  It will use the traversal data to  */
/*   find the channel being rendered, and from that the eye, and    */
/*   thus the user being rendered to.  Then it will call a function */
/*   that will convert the travel matrix associated with that user  */
/*   and set the DCS node to that matrix.                           */
/* NOTE: make sure to call this in the CULL traversal, not in the   */
/*   DRAW traversal, because if the node gets culled, you'll never  */
/*   be able to set the value again.                                */
int vrPfDCSTransformUserTravelCB(pfTraverser *trav, void *data)
{
	pfDCS           *dcs = (pfDCS *)pfGetTravNode(trav);	/* Get a pointer to the DCS node itself */
	pfChannel       *chan = pfGetTravChan(trav);		/* Get the channel we are rendering to */
	vrRenderInfo    *chandata = (vrRenderInfo *)pfGetChanData(chan);
	vrEyeInfo       *curr_eye = chandata->eye;

	/* copy (and transform) the travel matrix of the user into this pfDCS node */
	vrPfDCSTransformUserTravel(dcs, curr_eye->user);

	pfCullResult(PFIS_MAYBE);	/* Force the Cull traversal to independently check the cull state of children */

	/* Tell Performer to continue traversing */
	return PFTRAV_CONT;
}


/*****************************************************************/
/* NOTE: this version is used when the internal coord sys is kept as Z-up rather than converting to OpenGL */
void vrPfDCSTransformUserTravel_zup(pfDCS *dcs, vrUserInfo *user)
{
static  vrMatrix        *travel_mat;
static	pfMatrix	pfmat;

	travel_mat = user->rw2vw_xform;

	/* TODO: use vrPfMatrixFromVrMatrix() instead */
	pfmat[0][0] = travel_mat->v[ 0];
	pfmat[0][1] = travel_mat->v[ 1];
	pfmat[0][2] = travel_mat->v[ 2];
	pfmat[0][3] = travel_mat->v[ 3];
	pfmat[1][0] = travel_mat->v[ 4];
	pfmat[1][1] = travel_mat->v[ 5];
	pfmat[1][2] = travel_mat->v[ 6];
	pfmat[1][3] = travel_mat->v[ 7];
	pfmat[2][0] = travel_mat->v[ 8];
	pfmat[2][1] = travel_mat->v[ 9];
	pfmat[2][2] = travel_mat->v[10];
	pfmat[2][3] = travel_mat->v[11];
	pfmat[3][0] = travel_mat->v[12];
	pfmat[3][1] = travel_mat->v[13];
	pfmat[3][2] = travel_mat->v[14];
	pfmat[3][3] = travel_mat->v[15];

	pfDCSMat(dcs, pfmat);			/* in C++ dcs->setMat(pfmat); */
}


/*****************************************************************/
/* This function is a callback function that can be tied to a pfDCS */
/*   node as a PFCULL callback.  It will use the traversal data to  */
/*   find the channel being rendered, and from that the eye, and    */
/*   thus the user being rendered to.  Then it will call a function */
/*   that will convert the travel matrix associated with that user  */
/*   and set the DCS node to that matrix.                           */
/* NOTE: make sure to call this in the CULL traversal, not in the   */
/*   DRAW traversal, because if the node gets culled, you'll never  */
/*   be able to set the value again.                                */
/* NOTE: this version is used when the internal coord sys is kept as Z-up rather than converting to OpenGL */
int vrPfDCSTransformUserTravelCB_zup(pfTraverser *trav, void *data)
{
	pfDCS           *dcs = (pfDCS *)pfGetTravNode(trav);	/* Get a pointer to the DCS node itself */
	pfChannel       *chan = pfGetTravChan(trav);		/* Get the channel we are rendering to */
	vrRenderInfo    *chandata = (vrRenderInfo *)pfGetChanData(chan);
	vrEyeInfo       *curr_eye = chandata->eye;

	/* copy (and transform) the travel matrix of the user into this pfDCS node */
	vrPfDCSTransformUserTravel_zup(dcs, curr_eye->user);

	pfCullResult(PFIS_MAYBE);	/* Force the Cull traversal to independently check the cull state of children */

	/* Tell Performer to continue traversing */
	return PFTRAV_CONT;
}


/* The render function */
/****************************************************************************/
/* this function is called for each channel, which equates to each rendered eye. */
static void _PfRenderFunc(pfChannel *chan, void *data)
{
static	char		trace_msg[256];
	char		*buffer_name;		/* name of the GL buffer selected */
	vrRenderInfo	*renderinfo = (vrRenderInfo *)data;
	vrWindowInfo	*curr_window = renderinfo->window;
	vrEyeInfo	*curr_eye = renderinfo->eye;
	vrUserInfo	*curr_user = curr_eye->user;
	vrGlxPrivateInfo *aux = (vrGlxPrivateInfo *)curr_window->aux_data;
	vrCallback	*callback = NULL;
	XEvent		event;
	int		twod_ortho_mode = 0;	/* set to one when rendering in 2D */

	if (curr_window->name == NULL) {
		printf("_PfRenderFunc(): something is seriously wrong -- no window name.\n");
		return;
	}
	sprintf(trace_msg, "beginning window render loop for window '%s' %#p", curr_window->name, curr_window);
	vrTrace("_PfRenderFunc", trace_msg);

	vrDbgPrintfN(PERFORMER_DETAIL_DBGLVL, RED_TEXT "_PfRenderFunc(): rendering to window '%s', chan %p\n" NORM_TEXT, renderinfo->window->name, chan);

#ifndef GFX_PERFORMER /* We don't need this in the Performer version */
	/* NOTE: in the GLX version, the first thing I do is:  */
	glXMakeCurrent(aux->xdisplay, aux->xwindow, aux->glx_context);
#endif

#if 0 /* 6/5/03: test to see if pipe window is open */
vrPrintf("There: pfIsPWinOpen(pipe_window) = %d\n", pfIsPWinOpen(pfGetChanPWin(chan)));
#endif

	/****************************/
	/* get x events, if any ... */
      if (aux->xwindow != 0) {	 /* Note using non-standard indentation so as to match with vr_visren.glx.c */
	while (XPending(aux->xdisplay)) {
		XNextEvent(aux->xdisplay, &event);

		switch (event.type) {

		case ConfigureNotify:
#if 0
			glViewport((GLint)0, (GLint)0, (GLsizei)event.xconfigure.width, (GLsizei)event.xconfigure.height);
#endif
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

			vrDbgPrintfN(PERFORMER_DBGLVL, "_PfRenderFunc(): Got a reconfiguration event.  Setting window %#p viewport to (%d, %d) (%dx%d).\n",
				curr_window,
				curr_window->geometry.origX, curr_window->geometry.origY,
				curr_window->geometry.width, curr_window->geometry.height);
			vrDbgPrintfN(PERFORMER_DBGLVL, "viewport_left = %d %d %d %d\n",
				curr_window->viewport_left.origX,
				curr_window->viewport_left.origY,
				curr_window->viewport_left.width,
				curr_window->viewport_left.height);
			vrDbgPrintfN(PERFORMER_DBGLVL, "viewport_right = %d %d %d %d\n",
				curr_window->viewport_right.origX,
				curr_window->viewport_right.origY,
				curr_window->viewport_right.width,
				curr_window->viewport_right.height);

			vrDbgPrintfN(XWIN_DBGLVL, "_PfRenderFunc(): Got a reconfiguration event.  Setting window %#p viewport to (%d, %d) (%dx%d).\n",
				curr_window,
				curr_window->geometry.origX, curr_window->geometry.origY,
				curr_window->geometry.width, curr_window->geometry.height);

			break;

		case KeyPress: {
/* TODO: this keysym lookup code should take into account the current modifier states */
			KeySym	key = XLookupKeysym((XKeyEvent *)&event, 0);
			int	key_used_by_sim = 1;

			vrDbgPrintfN(XWIN_DBGLVL, "DRAW: _PfRenderFunc(): Got a key press of type %X\n", XLookupKeysym((XKeyEvent *)&event, 0));

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
#ifdef __sun
				case XK_F25:		/* toggle the simulator_mask "flag" */
#endif
				case XK_KP_Divide:	/* toggle the simulator_mask "flag" */
					vrSimulatorMove(curr_window, VR_SIMMOVE_TOGGLEMASK, NULL);
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
			case XK_KP_Delete:	/* toggle the stats_show flag */
				curr_window->stats_show ^= 1;
				break;

#ifdef __APPLE__ /* TODO: consider allowing this for all versions, not just Apple Macintosh's */
			case XK_KP_9:
#endif
			case XK_KP_Page_Up:		/* toggle the user-interface display */
				curr_window->ui_show ^= 1;

#if 1 /* TODO: remove this -- for testing, prints ui info to text output of window */
				vrSprintInputUI(trace_msg, renderinfo->context->input, sizeof(trace_msg), verbose);
				vrFprintf(stdout, "=================================================================\n");
				vrFprintf(stdout, trace_msg);
				vrFprintf(stdout, "=================================================================\n");
#endif
				break;


			default:
				if (!key_used_by_sim)
					vrDbgPrintfN(INPUT_DBGLVL, "_PfRenderFunc(): Got an unused key press of type %X\n", XLookupKeysym((XKeyEvent *)&event, 0));
				break;
			}
		}	break;

		case KeyRelease:
			break;

		case EnterNotify:
			/* I don't need to handle this, but it seems to be an event that is automatically */
			/*   included in the standard event mask, and I want to avoid the warning message,*/
			/*   thus the empty case.                                                         */
			vrDbgPrintfN(DEFAULT_DBGLVL, "_PfRenderFunc(): got an 'EnterNotify' event for window '%s'\n", curr_window->name);
			break;

		case UnmapNotify:
			vrDbgPrintfN(DEFAULT_DBGLVL, "_PfRenderFunc(): got an 'UnmapNotify' event for window '%s'\n", curr_window->name);
			break;

		case MapNotify:
			vrDbgPrintfN(DEFAULT_DBGLVL, "_PfRenderFunc(): got an 'MapNotify' event for window '%s'\n", curr_window->name);
			break;

		default:
			vrDbgPrintfN(INPUT_DBGLVL, "_PfRenderFunc(): got an unhandled X event of type %d\n", event.type);
			break;
		}
	}
      /* Note not using standard indentation so as to match with vr_visren.glx.c */
      } else {
	vrPrintf(RED_TEXT "_PfRenderFunc(): If we never get here, then perhaps we can delete this if-statement.\n" NORM_TEXT);
      }


#if 0	/* TODO: hmmm, this probably shouldn't be here at all, because this function */
	/*   will be called for every channel, which equates to each rendered eye.   */
	/* [6/12/02 -- perhaps this/these callback(s) should be in vrPfPreFrame() */
	/*   (though the app-prog is not required to call vrPfPreFrame().)        */

	/* call global frame function (same for all windows) */
	vrCallbackInvoke(vrContext->callbacks->VisrenFrame);
	vrTrace("_PfRenderFunc", "after GENERAL visrenframe callback");

	/* TODO: add window-specific frame callback (VR_ONE_DISPLAY_FRAME) */
	/* TODO: determine if we can call user-specific frame functions here (probably not, do GLX version first  */
#endif

#ifndef GFX_PERFORMER /* I don't think we need this in the Performer system */
#if 1	/* don't include this if GFXINIT_TOP is not set to 2 in vr_visren.c */
	/* TODO: 6/27/01 -- this is now handled in the main visren loop */
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
vrPrintf("_PfRenderFunc(): " RED_TEXT "call_visreninit was set to 1 for window '%s'\n" NORM_TEXT, curr_window->name);

		vrTrace("_PfRenderFunc", "prep: initialization callback");
		callback = curr_window->VisrenInit;
		vrCallbackInvokeDynamic(callback, 1, renderinfo);
		curr_window->call_visreninit = 0;
		vrTrace("_PfRenderFunc", "done: initialization callback");
	}
#endif
#endif


	/*****************************/
	/* (i) push gfx matrix/state */

	/* NOTE: attributes are not pushed to allow the application render */
	/*   routine to set the state in one call, and be able to expect   */
	/*   that state to be unchanging until it decides to change it.    */
	/* Perhaps we are now giving the same courtesy to the transformation matrix. */


	/*****************************************/
	/* (ii) handle viewport and frame buffer */
	vrTrace("_PfRenderFunc", "(ii) handle viewport");

	/* set the buffer into which we should render (based on the eye) */
	switch (curr_eye->render_framebuffer) {
	default:
		/* Really, we shouldn't need to rely on the default, so print a warning message and continue. */
		vrMsgPrintf("By default rendering to the Back buffer\n");
	case VRFB_FULL:
		glDrawBuffer(GL_BACK);
		buffer_name = "GL_BACK";
		break;
	case VRFB_LEFT:
		glDrawBuffer(GL_BACK_LEFT);
		buffer_name = "GL_BACK_LEFT";
		break;
	case VRFB_RIGHT:
		if (curr_window->dualeye_buffer == 1) {
			glDrawBuffer(GL_BACK_RIGHT);
			buffer_name = "GL_BACK_RIGHT";
		} else {
			static	int	dualeye_warning = 0;

			if (dualeye_warning == 0) {
				vrDbgPrintfN(ALWAYS_DBGLVL, "_PfRenderFunc(): " RED_TEXT "There is no stereo buffer on window '%s', so rendering to right-eye has been disabled.\n" NORM_TEXT, curr_window->name);
				dualeye_warning = 1;
			}

			sprintf(trace_msg, "premature ending window render loop for window '%s' %#p -- non-existent stereo buffer", curr_window->name, curr_window);
			vrTrace("_PfRenderFunc", trace_msg);

			return;
		}
		break;

	/* NOTE: [5/5/02] Do we may want a special dualvp rendering mode   */
	/*   that is independent of specifically rendering a left or right */
	/*   eye view.                                                     */
	/* NOTE: [6/11/02: hmmm, now trying to use this as the only option, */
	/*   and for systems with just one eye to a window, the viewport    */
	/*   will take care of things.                                      */
	case VRFB_FULL_LEFTEYE:	  /* NOTE: this will just have a viewport encompassing the entire window */
	case VRFB_SPLIT_LEFTEYE:
		glDrawBuffer(GL_BACK);
		buffer_name = "GL_BACK";
#ifndef GLX_PERFORMER /* Performer handles the viewport automatically */
		glViewport(curr_window->viewport_left.origX, curr_window->viewport_left.origY,
			curr_window->viewport_left.width, curr_window->viewport_left.height);
		glScissor(curr_window->viewport_left.origX, curr_window->viewport_left.origY,
			curr_window->viewport_left.width, curr_window->viewport_left.height);
#endif
		break;
	case VRFB_FULL_RIGHTEYE:  /* NOTE: this will just have a viewport encompassing the entire window */
	case VRFB_SPLIT_RIGHTEYE:
		glDrawBuffer(GL_BACK);
		buffer_name = "GL_BACK";
#ifndef GLX_PERFORMER /* Performer handles the viewport automatically */
		glViewport(curr_window->viewport_right.origX, curr_window->viewport_right.origY,
			curr_window->viewport_right.width, curr_window->viewport_right.height);
		glScissor(curr_window->viewport_right.origX, curr_window->viewport_right.origY,
			curr_window->viewport_right.width, curr_window->viewport_right.height);
#endif
		break;
	}


	/******************************/
	/* (iii) handle color masking */
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


	/************************/
	/* (iv) handle viewmask */
	/* TODO: perhaps this task should be done after all the rendering is complete */
	/* TODO: ... */


	/*****************************************/
	/* (v) put transform matrix on the stack */

	/* NOTE: for Performer, this step is handled in vrPfPreFrame() */


	/**************************/
	/* (vi) render the world  */
	vrTrace("_PfRenderFunc", "(vi) render the world");

	/* clear the rendering buffers */
	pfClearChan(chan);


	/* TODO: do programmer-specified pre-draw callback here */
	/*       Hmmm, should this be per-eye, or per-window? */

	/* if a non-default rendermode is set, then set it */
	if (curr_window->frontrendermode != GL_NONE) {
		glPolygonMode(GL_FRONT, curr_window->frontrendermode);
	}
	if (curr_window->backrendermode != GL_NONE) {
		glPolygonMode(GL_BACK, curr_window->backrendermode);
	}

	/* let Performer draw the scenegraph */
	pfDraw();

	/* if a non-default rendermode is set, then restore to fill-mode */
	if (curr_window->frontrendermode != GL_NONE) {
		glPolygonMode(GL_FRONT, GL_FILL);
	}
	if (curr_window->backrendermode != GL_NONE) {
		glPolygonMode(GL_BACK, GL_FILL);
	}

	/* TODO: do programmer-specified post-draw callback here */
	/*       Hmmm, should this be per-eye, or per-window? */


	/* TODO: check whether GL is in an error state (or has set */
	/*   the error flag), and print a warning if it has.       */


	/*****************************************************/
	/* (vii) call simulator_render (if simulator window) */
	if (curr_window->mount == VRWINDOW_SIMULATOR) {
		vrTrace("_PfRenderFunc", "(vii) call simulator_render");
		callback = curr_user->VisrenSim;
		if (!callback) {
			callback = curr_window->VisrenSim;
		}
		if (!callback) {
			callback = vrContext->callbacks->VisrenSim;
		}
		if (callback) {
			glPushMatrix();
			glRotated(90.0, 1.0, 0.0, 0.0);		/* render in Performer coordinates */
			vrCallbackInvokeDynamic(callback, 2, renderinfo, curr_window->simulator_mask);
			glPopMatrix();
		} else	vrDbgPrintfN(ALWAYS_DBGLVL, "_PfRenderFunc(): " RED_TEXT "no simulator callback available\n" NORM_TEXT);
	}


	/****************************************************/
	/* (viii) display the frame statistics when flagged */

#ifndef GFX_PERFORMER	/* TODO: I think we should add the stats stuff to the Performer version */
	/*** render the timing statistics as a bar-graph ***/
	...
#endif

	/*** render the frame rate as text ***/
	if (curr_window->fps_show) {
#if 1 /* { Use built-in Performer function for rendering FPS and other info */
		vrTrace("_PfRenderFunc", "(viii) display the frame rate");
		pfDrawChanStats(chan);
#else /* } do simple GL version (NOTE: also will include _GlxRenderText() below) { */
static		char	fps_string[128];

		vrTrace("_PfRenderFunc", "(viii) display the frame rate");

		/* if not currently rendering in 2D-ortho mode, then do so */
		if (!twod_ortho_mode) {
			/** go into 2-D ortho perspective mode **/
			glPushAttrib(0
				| GL_CURRENT_BIT	/* covers current color and rendering mode */
				| GL_ENABLE_BIT		/* covers all enable bits (inc. texture 2d)*/
				| GL_TRANSFORM_BIT	/* covers glMatrixMode, glNormalize & others */
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
		_GlxRenderText(renderinfo, fps_string);
#define INCLUDE_GLXRENDERTEXT


	/*** render the user-interface information on screen ***/
	if (curr_window->ui_show) {
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

		vrTrace("_GlxRenderFunc", "(viii) display the user-interface information");

		/* if not currently rendering in 2D-ortho mode, then do so */
		if (!twod_ortho_mode) {
			/** go into 2-D ortho perspective mode **/
			glPushAttrib(0
				| GL_CURRENT_BIT	/* covers current color and rendering mode */
				| GL_ENABLE_BIT		/* covers all enable bits (inc. texture 2d)*/
				| GL_TRANSFORM_BIT	/* covers glMatrixMode, glNormalize & others */
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
			_GlxRenderText(renderinfo, next_line);
#define INCLUDE_GLXRENDERTEXT
			y -= yscale;
		} while (next_line = strtok_r(NULL, "\n", (char **)&ui_string_tok));
	}

	/*** restore to 3D mode if we changed into 2D ortho mode ***/
	if (twod_ortho_mode) {
		glPopMatrix();
		glPopAttrib();
	}
#endif /* } */
	}


	/**************************************/
	/* TODO: does viewmask stuff go here? */


	/***********************************/
	/* (ix) restore gfx matrix/state */
#ifndef GFX_PERFORMER /* I don't think we need this in the Performer system */
	glPopMatrix(); /* } */
#endif

	sprintf(trace_msg, "ending window render loop for window '%s' %#p\n", curr_window->name, curr_window);
	vrTrace("_PfRenderFunc", trace_msg);
}
/* end of render function */


/****************************************************************************/
static void _GlxRenderTransform(vrMatrix *mat)
{
	glMultMatrixd(mat->v);
}


#ifdef INCLUDE_GLXRENDERTEXT /* Only necessary if using 2D text in this file to display debug info */
/****************************************************************************/
static void _GlxRenderText(vrRenderInfo *renderinfo, char *string)
{
	char		*font = "fixed";
	vrWindowInfo	*curr_window = vrRenderCurrentWindow(renderinfo);
	vrGlxPrivateInfo *aux = NULL;
	GLenum		err;

	/* attempting to rendering a NULL string (not the empty string) is bad */
	if (string == NULL)
		return;

	aux = (vrGlxPrivateInfo *)curr_window->aux_data;

	/* if no font has been initialized, do so */
	if (!aux->fontListBase) {
		aux->fontStruct = XLoadQueryFont(aux->xdisplay, font);
		if (aux->fontStruct) {
			aux->fontListBase = glGenLists(aux->fontStruct->max_char_or_byte2);
			glXUseXFont(aux->fontStruct->fid,0,aux->fontStruct->max_char_or_byte2,aux->fontListBase);
		}

	}

	/* if we now have a font setup, render the text */
	if (aux->fontListBase) {
		glPushAttrib(GL_LIST_BIT);
		glListBase(aux->fontListBase);
		glCallLists(strlen(string),GL_UNSIGNED_BYTE,(GLubyte *)string);
		glPopAttrib();
	} else {
		vrDbgPrintf("Unable to render text in GLX.\n");
	}
}
#endif


/****************************************************************************/
/* A function to check the rendering library error flag(s) and report any   */
/*   error conditions.                                                      */
static void _GlxRenderErrors()
{
	GLenum	error_code;

	while ((error_code = glGetError()) != GL_NO_ERROR) {
		vrPrintf("_GlxRenderErrors(): " RED_TEXT" a GL error of type '%s' (%d) has occurred.\n" NORM_TEXT,
			_GlxGLErrorCodeString(error_code),
			(int)error_code);
	}
}


/****************************************************************************/
/* vrGlxInitWindowInfo(): setup all the callbacks needed for GLX rendering. */
/****************************************************************************/
void vrGlxInitWindowInfo(vrWindowInfo *info)
{
	vrDbgPrintfN(PERFORMER_DBGLVL, "Initializing callback and version Info for Window at %#p\n", info);
	info->version = (char *)vrShmemStrDup("Performer render window");
	info->PreOpenInit = vrCallbackCreateNamed("PFGlxWin:PreOpenInit-Def", _PfInitFunc, 1, info);	/* TODO: perhaps this callback should be associated with the entire system configuration, and not each window. (this would work better when the vrContext is passed to this function) */
	info->Open = vrCallbackCreateNamed("PFGlxWin:PreOpenInit-Def", _PfOpenFunc, 1, info);
	info->Close = vrCallbackCreateNamed("PFGlxWin:PreOpenInit-DN", vrDoNothing, 0);
	info->Render = vrCallbackCreateNamed("PFGlxWin:PreOpenInit-Def", _PfRenderFunc, 0);
	info->RenderText = vrCallbackCreateNamed("PFGlxWin:PreOpenInit-DN", vrDoNothing, 0);
	info->RenderNullWorld = vrCallbackCreateNamed("PFGlxWin:PreOpenInit-DN", vrDoNothing, 0);
	info->RenderSimulator = vrCallbackCreateNamed("PFGlxWin:PreOpenInit-Def", vrGLRenderDefaultSimulator, 0);
	info->RenderTransform = vrCallbackCreateNamed("PFGlxWin:PreOpenInit-Def", _GlxRenderTransform, 0);
	info->Swap = vrCallbackCreateNamed("PFGlxWin:PreOpenInit-DN", vrDoNothing, 1, info);
	info->Errors = vrCallbackCreateNamed("GlxWindow:Errors-Def", _GlxRenderErrors, 0);
	info->PrintAux = vrCallbackCreateNamed("PFGlxWin:PrintAux-Def", vrFprintGlxPrivateInfo, 0);
}

