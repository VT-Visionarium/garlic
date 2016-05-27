/* ======================================================================
 *
 *  CCCCC          vr_visren.glx.c
 * CC   CC         Author(s): Ed Peters, Bill Sherman, John Stone, Michael Penick (OSG version)
 * CC              Created: June 4, 1998
 * CC   CC         Last Modified: May 13, 2013
 *  CCCCC
 *
 * Code file for FreeVR visual rendering into a GLX window.
 * NOTE: also serves as source of OpenSceneGraph (OSG) rendering interface
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/*************************************************************************

FreeVR USAGE:
	Here are the FreeVR configuration options for the GLX visual rendering window:
		"display=[host]:server[.screen]" -- set the display where the window should appear
		"geometry=[WxH][+X+Y]" -- set where on the display the window should appear
		"decoration={borders|title|minmax|window|none}*" -- set look of window
			(use "none" when windows will be joined with neighboring windows)
		"title=window title" -- set the title on the window title bar
		"cursor={default|blank|dot|bigdot|superdot}" -- set the name of the cursor

	Controls are specified in the freevrrc file:
	  :-(	e.g.: control "<control option>" = "switch2(button[{1|2|3|4|5|6|7|8|Star}])";

	Available control options are:
	  :-(	"print_help" -- print info on how to use the display device
	  :-(	"print_struct" -- print the internal GLX data structure (for debugging)
		-- simulator controls should be made to be control options --

Coding Notes:
	Some useful webpages (which I found well after I had everything solved,
	but could be useful in future debugging):
		- http://standards.freedesktop.org/wm-spec/wm-spec-latest.html
		- http://tronche.com/gui/x/xlib/window/attributes/
		- http://odl.sysworks.biz/disk$cddoc04sep11/decw$book/d3b0aa63.p264.decw$book
		- http://fixunix.com/xwindows/91627-xchangeproperty-questions.html
		- http://stackoverflow.com/questions/5134297/xlib-how-does-this-removing-window-decoration-work
		- http://menehune.opt.wfu.edu/Kokua/Irix_6.5.21_doc_cd/usr/share/Insight/library/SGI_bookshelves/SGI_Developer/books/XLib_WinSys/sgi_html/ch11.html

TODO:
	- make simulator controls control options

	- cleanup the non-generic arguments, and make them generic if
		possible/reasonable

	- some form of key-repeat like behavior on simulator movement
		controls (ie. keep moving while key is held down)

	- grayscale rendering option with correct luminance

	- better handling of the window attributes
		(currently using Motif controls, which work but don't seem
		overly universal.)

**************************************************************************/
#include <string.h>
#include <X11/keysym.h>
#include <math.h>			/* needed for HUGE_VAL (aka __infinity) definition */

#include "vr_visren.h"
#include "vr_visren.glx.h"
#include "vr_basicgfx.glx.h"		/* needed for vrGLRenderDefaultSimulator() */
#include "vr_callback.h"		/* also included within vr_input.h, but left for clarity */
#include "vr_input.h"
#include "vr_debug.h"
#include "vr_utils.h"			/* for declarations of vrStringCharWidth() & vrStringCharHeight() */

#ifdef GFX_OSG
#include <osgUtil/SceneView>
#include <osg/FrameStamp>
#include <osg/Timer>

/* Local Globals */
/* Question: do these need to be local to a single process, or all visren processes? */
/* TODO: determine the necessity of these as global variables */
static osg::ref_ptr<osg::Node>			osg_scenedata = NULL;	/* Pointer to scene graph */
static vrLock					osg_sg_lock;		/* Lock to protect scene graph changes */
static osg::ref_ptr<osgDB::DatabasePager>	osg_databasePager = NULL;/* Database pager for PagedLOD */
#endif

#if 0 /* set to 1 to prevent local vrTrace messages */
#  ifdef vrTrace
#    undef	vrTrace
#    define	vrTrace(a,b) ;
#  endif
#endif


/****************************************************************************/
/* NOTE: this function also used by vr_input.windows.c */
int vrXwindowsErrorHandler(Display *display, XErrorEvent *event)
{
static	char	string[1024];

	vrPrintf(BOLD_TEXT "Got an X error in 'dbx -p %d'\n" NORM_TEXT, getpid());
	XGetErrorText(display, event->error_code, string, 1023);
	vrPrintf(BOLD_TEXT "Reported error is '%s'\n", string);

#if 0 /* an infinite while() is much better than pause, because it doesn't add to the gdb/dbx call list */
	pause();
#else
	while(1);
#endif

	return 0;	/* meaningless return value for picky compilers */
}


/*****************************************************************/
/* NOTE: this function used by vr_input.windows.c */
Cursor vrXwindowsMakeCursor(vrGlxPrivateInfo *aux, char *cursor_name)
{
static	char	blank_data[1] = { 0x00 };
static	char	dot_data[1] = { 0x01 };
static	char	bigdot_data[2] = { 0xFF, 0xFF };
static	char	test_data[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
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
	} else if (!strcasecmp(cursor_name, "superdot")) {
		width = 8;
		height = 8;
		cursor_map = XCreateBitmapFromData(aux->xdisplay, RootWindow(aux->xdisplay, aux->xscreen), test_data, width, height);
	} else if (!strcasecmp(cursor_name, "default")) {
		/* don't set a map and go with what's already there */
		return None;
	} else {
		vrDbgPrintfN(AALWAYS_DBGLVL, "vrXwindowsMakeCursor(): unknown cursor name.\n");
		return None;
	}

	if (cursor_map == None)
		vrDbgPrintfN(ALWAYS_DBGLVL, "vrXwindowsMakeCursor(): " RED_TEXT "out of memory for cursor.\n" NORM_TEXT);

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
		vrDbgPrintfN(GLX_DBGLVL, "_GlxParseArgs(): parsed host = '%s', xserver = %d, screen = %d\n", host, aux->xserver, aux->xscreen);
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
	vrDbgPrintf("aux->decorations = 0x%02x\n", aux->decorations);
	vrDbgPrintf("geometry: %dx%d+%d+%d\n",
		aux->xsize_hints.width, aux->xsize_hints.height,
		aux->xsize_hints.x, aux->xsize_hints.y);
	vrDbgPrintf("xsize_hints.flags = %d\n", aux->xsize_hints.flags);
	vrDbgPrintf("aux->cursor_name = '%s'\n", aux->cursor_name);
	vrDbgPrintf("================================================\n");
}



#ifndef GFX_PERFORMER /* This is used by GLX, but not Performer. */
/*****************************************************************/
/* vrWaitForWindowMapping(): used by _GlxOpenFunc() to wait for  */
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


/* The opening and initialization function -- this line is to help diff match */
/**************************************************************************/
/* _GlxOpenFunc() initializes the auxiliary window info for the specified */
/* window.                                                                */
/**************************************************************************/
static void _GlxOpenFunc(vrWindowInfo *window)
{
#include XBM_ICON_FILE
static	char		trace_msg[256];
	vrGlxPrivateInfo *aux = NULL;
	char		xdisplay_name[64];
	unsigned long	winattr_mask = 0;	/* the window attributes that have been set in the attribute structure */
	Atom		mwm_hints_atom;		/* used for hinting at the decorations to the WM */
	Pixmap		icon_pixmap;		/* An X11 pixmap for creating an icon -- TODO: how to make this thread safe? */
	XEvent		event;
	int		dummy = 0;

	sprintf(trace_msg, BOLD_TEXT "Entering for window '%s' %#p\n" NORM_TEXT, window->name, window);
	vrTrace("_GlxOpenFunc", trace_msg);
	vrDbgPrintfN(GLX_DBGLVL, "_GlxOpenFunc(): %s", trace_msg);


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
	vrTrace("_GlxOpenFunc", "Arguments parsed");
	vrDbgPrintfN(GLX_DBGLVL, "_GlxOpenFunc(): %s", "Arguments parsed");


	/****************************************************/
	/*** open connection to X server (one per window) ***/
	/****************************************************/

	/* TODO: not always one per vr_window.  We also need to allow (prefer */
	/*   even) that multiple vr_windows are handled by a single X-window. */
	/*   Though, it should be pointed out that this is still not as bad   */
	/*   as pre2.6 CAVE library because the rendering in the same process */
	/*   is done sequentially, not in parallel.                           */

	if (aux->xhost == NULL) {
		if (!(aux->xdisplay = XOpenDisplay(NULL))) {
			vrErrPrintf("_GlxOpenFunc(): " RED_TEXT "Default Xdisplay -- no DISPLAY defined, using ':0.0'\n" NORM_TEXT);
			if (!(aux->xdisplay = XOpenDisplay(":0.0"))) {
				vrErrPrintf("_GlxOpenFunc(): " RED_TEXT "Could not open Default X11-display ':0.0'\n" NORM_TEXT);
				vrTrace("_GlxOpenFunc", "Could not open Default X11-display ':0.0'");
				return;		/* returning without mapping the window */
			} else {
				strcpy(xdisplay_name, ":0.0");	/* set the string for the trace message below */
			}
		} else {
			strcpy(xdisplay_name, "<default>");	/* set the string for the trace message below */
		} 
	} else {
		sprintf(xdisplay_name, "%s:%d.%d", aux->xhost, aux->xserver, aux->xscreen);
		if (!(aux->xdisplay = XOpenDisplay(xdisplay_name))) {
			vrErrPrintf("_GlxOpenFunc(): " RED_TEXT "Could not open X11-display '%s'\n" NORM_TEXT, xdisplay_name);
			sprintf(trace_msg, "Could not open X11-display '%s'", xdisplay_name);
			vrTrace("_GlxOpenFunc", trace_msg);
			return;		/* returning without mapping the window */
		}
	}
	sprintf(trace_msg, "X11 display '%s' has been opened.", xdisplay_name);
	vrTrace("_GlxOpenFunc", trace_msg);
	vrDbgPrintfN(GLX_DBGLVL, "_GlxOpenFunc(): %s", trace_msg);

	aux->xdisplay_string = vrShmemStrDup(DisplayString(aux->xdisplay));
	if (aux->xscreen < 0)
		aux->xscreen = DefaultScreen(aux->xdisplay);

	aux->xscreen_size_x = DisplayWidth(aux->xdisplay, aux->xscreen);
	aux->xscreen_size_y = DisplayHeight(aux->xdisplay, aux->xscreen);

	/**************************/
	/* setup X error handling */
	XSetErrorHandler(vrXwindowsErrorHandler);
	XSynchronize(aux->xdisplay, True);

	/****************************************/
	/* make sure this X server supports GLX */
	if (!glXQueryExtension(aux->xdisplay, &dummy, &dummy)) {
		vrErr("X server doesn't have OpenGL GLX extension");
		return;		/* returning without mapping the window */
	}

	/*********************************************/
	/* get an appropriate visual for this window */
/* TODO: what if I don't want stereo?  Why request a stereo visual? (ie. fix this)  [DONE -- but not tested in a CAVE!] */
vrPrintf("_GlxOpenFunc(): " RED_TEXT "dualeye_buffer = %d, visrenmode = '%s' (%d) -- TODO: don't request a stereo visual if one is not desired.\n" NORM_TEXT, window->dualeye_buffer, vrVisrenModeName(window->visrenmode), window->visrenmode);
	aux->doub_buf = GL_TRUE;		/* assume a double buffer unless informed otherwise */
	if (window->visrenmode == VRVISREN_DUALFB) {
		aux->xvisual = glXChooseVisual(aux->xdisplay, aux->xscreen, stereo_buf_attribs);

		aux->stereo_buf = GL_TRUE;
	} else {
		aux->xvisual = 0;		/* force the request of a non-stereo double buffer */
	}

	/* if no xvisual is assigned yet, try for a standard double-buffer */
	if (!aux->xvisual) {
		aux->stereo_buf = GL_FALSE;
		aux->xvisual = glXChooseVisual(aux->xdisplay, aux->xscreen, doub_buf_attribs);

		/* if we couldn't get a double-buffer, request a single-buffer */
		if (!aux->xvisual) {
			aux->doub_buf = GL_FALSE;	/* not a double-buffer */
			aux->xvisual = glXChooseVisual(aux->xdisplay, aux->xscreen, sing_buf_attribs);
		}
	}

	/* Set "dualeye_buffer" so the rendering routine will know whether we have a quad buffer or not */
	window->dualeye_buffer = (aux->stereo_buf == GL_TRUE);

	/* Check whether we were demoted from a stereo buffer to non-stereo */
	if (!window->dualeye_buffer && (window->visrenmode == VRVISREN_DUALFB)) {
		/* NOTE: some effects of having the visrenmode set to VRVISREN_DUALFB have already  */
		/*   occurred.  In particular, the list of eyes for this window has been selected   */
		/*   based on the requested visrenmode, and we're not going back to change them now.*/
		window->visrenmode == VRVISREN_MONO;		/* NOTE: We don't set the "window->settings.visrenmode" value to mono, as that represents the requested setting, not the actuality */
		vrDbgPrintfN(COMMON_DBGLVL, "FreeVR: Demoting window '%s' to the monoscopic visrenMode.\n", window->name);
	}

	vrDbgPrintfN(COMMON_DBGLVL, "FreeVR: Chose the %s X11 visual type.\n",
		((aux->stereo_buf == GL_TRUE && aux->doub_buf == GL_TRUE) ? "quad (stereo & double) buffer" :
		((aux->doub_buf == GL_TRUE) ? "double (non-stereo) buffer" : "single (non-stereo) buffer")));

	if (!aux->xvisual) {
		vrErr("no RGB visual with depth buffer.");
		return;		/* returning without mapping the window */
#ifdef GFX_OSG
	} else if (aux->xvisual->c_class != TrueColor) {
#else
	} else if (aux->xvisual->class != TrueColor) {
#endif
		/* TODO: Do we need to do anything special for non-TrueColor visuals? */
	}

	vrTrace("_GlxOpenFunc", BOLD_TEXT "about to create a rendering context" NORM_TEXT);

	/***************************************************************/
	/* open a gl rendering context w/ no display-list sharing etc. */
	aux->glx_context = glXCreateContext(aux->xdisplay, aux->xvisual, None, GL_TRUE);
	if (!aux->glx_context) {
		vrErr("couldn't create rendering context");
		return;		/* returning without mapping the window */
	}

	/**************************************/
	/* create a colormap for the X window */
	aux->xcolormap = XCreateColormap(aux->xdisplay,
		RootWindow(aux->xdisplay, aux->xscreen),
		aux->xvisual->visual,
		AllocNone	/* try AllocAll for grayscale tests */
		);
	aux->xwindow_attr.colormap = aux->xcolormap;		winattr_mask |= CWColormap;
#if 0
{
XColor newc;
	newc.flags = 0x0;
	newc.pixel = 1;
	newc.red = newc.blue = newc.green = 65535;
	XStoreColor(aux->xdisplay, aux->xcolormap, &newc);
	newc.pixel = 2;
	newc.red = newc.blue = newc.green = 60000;
	XStoreColor(aux->xdisplay, aux->xcolormap, &newc);
	newc.pixel = 3;
	newc.red = newc.blue = newc.green = 50000;
	XStoreColor(aux->xdisplay, aux->xcolormap, &newc);
	newc.pixel = 4;
	newc.red = newc.blue = newc.green = 40000;
	XStoreColor(aux->xdisplay, aux->xcolormap, &newc);
	newc.pixel = 5;
	newc.red = newc.blue = newc.green = 30000;
	XStoreColor(aux->xdisplay, aux->xcolormap, &newc);
	newc.pixel = 6;
	newc.red = newc.blue = newc.green = 20000;
	XStoreColor(aux->xdisplay, aux->xcolormap, &newc);
	newc.pixel = 7;
	newc.red = newc.blue = newc.green = 10000;
	XStoreColor(aux->xdisplay, aux->xcolormap, &newc);
	newc.pixel = 8;
	newc.red = newc.blue = newc.green = 00000;
	XStoreColor(aux->xdisplay, aux->xcolormap, &newc);
}
#endif

	/* NOTE: it seems that for the pthreads version, gdb totally ignores the write lock, and plows on through */
	vrLockWriteSet(window->context->xpixmap_lock);		/* The X11 bitmap code for pixmaps & cursors may not be pthread safe, so protecting with a lock */

	/********************************/
	/* create a pixmap for the icon */
	/* TODO: is this thread safe?  I'm getting occasional core dumps at this point in pthreads versions of the library */
vrPrintf("about to create the icon pixmap for window '%s'\n", window->name);
	icon_pixmap = XCreateBitmapFromData(
		aux->xdisplay,
		RootWindow(aux->xdisplay, aux->xscreen),
		XBM_ICON_BITS, XBM_ICON_WIDTH, XBM_ICON_HEIGHT);
vrPrintf("finished creating the icon pixmap for window '%s' -- pixmap = %d\n", window->name, icon_pixmap);

	/************************************/
	/* create a cursor for the X window */
	aux->cursor = vrXwindowsMakeCursor(aux, aux->cursor_name);
vrPrintf("now finished creating the cursor for window '%s' -- cursor = %d\n", window->name, aux->cursor);
	aux->xwindow_attr.cursor = aux->cursor;			winattr_mask |= CWCursor;

	vrLockWriteRelease(window->context->xpixmap_lock);


	/***********************************************************/
	/* set some window attributes prior to creating the window */
	vrTrace("_GlxOpenFunc", "about to open window");
	aux->xwindow_attr.border_pixel = 0;			winattr_mask |= CWBorderPixel;
	aux->xwindow_attr.background_pixel = 0x009000;		winattr_mask |= CWBackPixel;
	aux->xwindow_attr.event_mask =
#if 0					/* 06/13/2006: removed since we weren't using the "Expose" events */
		ExposureMask |
#endif
#if 0
		ButtonPressMask | 	/* can't do this here because Xwin-input needs this */
#endif
		StructureNotifyMask |
		DestroyNotify |		/* so we can cleanly go away when closed */
		UnmapNotify |		/* also so we can cleanly go away when closed */
		0;						winattr_mask |= CWEventMask;

	/**************************/
	/*** create the window! ***/
	/**************************/
	aux->xwindow = XCreateWindow(aux->xdisplay,	/* display to put the window on */
		RootWindow(aux->xdisplay, aux->xscreen),	/* make the root window the parent*/
		0, 0,				/* default x & y location */
		10, 10,				/* default width & height -- this sets lower limit on window size */
		0,				/* default width of the border */
		aux->xvisual->depth,		/* the desired color depth */
		InputOutput,			/* window class */
		aux->xvisual->visual,		/* the desired visual type */
		winattr_mask,			/* attribute options -- mask */
		&(aux->xwindow_attr));		/* window attributes */

	/****************************************************/
	/* set some window manager look and feel properties */

	/* NOTE: ideally it would be nice to not be dependent on the Motif  */
	/*   definitions and hint structure.  However, there appears to be  */
	/*   no generic Xlib equivalent, especially for hinting at how the  */
	/*   decorations should appear.  So the solution we use that avoids */
	/*   the requirement of the Motif header files and library is to    */
	/*   simply reproduce the necessary defines and type definitions    */
	/*   (found in vr_visren.glx.h), and then use them here.  It seems  */
	/*   that while they are not part of the generic window manager     */
	/*   hint system, they do seem to be globally accepted as hints!    */
vrPrintf("sizeof mwm_hints = %d, sizeof mwm_hints.flags = %d\n", sizeof(aux->mwm_hints), sizeof(aux->mwm_hints.flags));
	aux->mwm_hints.flags = MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS;
	aux->mwm_hints.functions = MWM_FUNC_ALL;
	aux->mwm_hints.decorations = 0;
	if (aux->decorations == DECORATION_ALL) {
		aux->mwm_hints.decorations = MWM_DECOR_ALL;
	} else {
		if (aux->decorations & DECORATION_TITLE) {
			aux->mwm_hints.decorations |= MWM_DECOR_TITLE;
		}
		if (aux->decorations & DECORATION_MINMAX) {
			aux->mwm_hints.decorations |= MWM_DECOR_MINIMIZE;
			aux->mwm_hints.decorations |= MWM_DECOR_MAXIMIZE;
		}
		if (aux->decorations & DECORATION_BORDER) {
			aux->mwm_hints.decorations |= MWM_DECOR_BORDER;
			aux->mwm_hints.decorations |= MWM_DECOR_RESIZEH;
		}
	}

	mwm_hints_atom = XInternAtom(aux->xdisplay, _XA_MOTIF_WM_HINTS, False);
	XChangeProperty(aux->xdisplay, aux->xwindow, mwm_hints_atom, mwm_hints_atom,
		32, PropModeReplace, (unsigned char *)(&(aux->mwm_hints)), PROP_MWM_HINTS_ELEMENTS);

	/************************************************/
	/* set standard window properties and pop it up */
	XSetStandardProperties(
		aux->xdisplay,		/* Xwindow display pointer */
		aux->xwindow,		/* Xwindow reference number */
		aux->window_title,	/* window title */
		window->name,		/* icon title */
		icon_pixmap,		/* Pixmap icon_pixmap */
		NULL,			/* char **argv */
		0,			/* int argc */
		&(aux->xsize_hints));	/* Xwindow geometry info */

	/* specify where the window should be placed */
	XMoveWindow(aux->xdisplay, aux->xwindow, aux->xsize_hints.x, aux->xsize_hints.y);
	if (aux->xsize_hints.width == 0) {
		vrDbgPrintfN(AALWAYS_DBGLVL, "_GlxOpenFunc(): " RED_TEXT "window width is 0, setting to 100\n" NORM_TEXT);
		aux->xsize_hints.width = 100;
	}
	if (aux->xsize_hints.height == 0) {
		vrDbgPrintfN(AALWAYS_DBGLVL, "_GlxOpenFunc(): " RED_TEXT "window height is 0, setting to 100\n" NORM_TEXT);
		aux->xsize_hints.height = 100;
	}

	/* specify the size of the window */
        XResizeWindow(aux->xdisplay, aux->xwindow, aux->xsize_hints.width, aux->xsize_hints.height);

	/* cause the window to appear on the screen */
	XMapWindow(aux->xdisplay, aux->xwindow);	/* this is when the window border appears */

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
		vrDbgPrintfN(GLX_DBGLVL, "_GlxOpenFunc(): viewport_left = %d %d %d %d\n", window->viewport_left.origX, window->viewport_left.width, window->viewport_left.origY, window->viewport_left.height);
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
		vrDbgPrintfN(GLX_DBGLVL, "_GlxOpenFunc(): geometry = %d %d %d %d\n", window->geometry.origX, window->geometry.origY, window->geometry.width, window->geometry.height);
		vrDbgPrintfN(GLX_DBGLVL, "_GlxOpenFunc(): viewport_left = %d %d %d %d\n", window->viewport_left.origX, window->viewport_left.origY, window->viewport_left.width, window->viewport_left.height);
		vrDbgPrintfN(GLX_DBGLVL, "_GlxOpenFunc(): viewportF_left = %f %f %f %f\n", window->viewportF_left.min_X, window->viewportF_left.max_X, window->viewportF_left.min_Y, window->viewportF_left.max_Y);
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


	/*************************************************/
	/* when the window has been successfully mapped  */
	/*   to the X-screen, set the flag and continue. */
	XIfEvent(aux->xdisplay, &event, vrWaitForWindowMapping, (XPointer)aux->xwindow);
	aux->mapped = 1;


	/***************************************************/
	/*** Configure this OpenGL context for rendering ***/
	/***************************************************/

	vrTrace("_GlxOpenFunc", "about to make GLX calls");

	glXMakeCurrent(aux->xdisplay, aux->xwindow, aux->glx_context);

#ifdef GFX_OSG
	/* TODO: make sure this is the proper location for this code */

	/* Allocate and setup a  osgUtil::SceneView for each rendering context */
	/* ***************** THIS MUST BE DONE AFTER THE CONTEXT IS CURRENT ***************** */
	aux->frame_num = 0;						/* Initialize frame count */
	aux->start_tick = osg::Timer::instance()->tick();		/* We need the approximate time of the start of the first frame */

	vrTrace("_GlxOpenFunc-OSG", "Just created osg-lock");
	aux->sceneview = new osgUtil::SceneView;
	vrTrace("_GlxOpenFunc-OSG", "Just created osg sceneview");
	aux->sceneview->setDefaults();					/* Must be called before following functions */
	aux->sceneview->setClearColor(osg::Vec4f(0.0, 0.0, 1.0, 0.0));
	aux->sceneview->setDrawBufferValue(GL_NONE);			/* Needed for stereo to work */
	aux->sceneview->getState()->setContextID(window->id);		/* Needed for OpenGL "objects" e.g. textures */
	vrTrace("_GlxOpenFunc-OSG", "done initializing OSG");

	/* Setup database pager if it's been initialized */
	if (osg_databasePager.valid())
		((vrGlxPrivateInfo *)aux)->sceneview->getCullVisitor()->setDatabaseRequestHandler(osg_databasePager.get());

	vrDbgPrintfN(GLX_DBGLVL, "_GlxOpenFunc(): OSG Info: Window: %d SceneView: %p\n", window->id, ((vrGlxPrivateInfo *)aux)->sceneview.get());
#endif

	glEnable(GL_DEPTH_TEST);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-1.0, 1.0, -1.0, 1.0, 1.0, 1000.0);
	glMatrixMode(GL_MODELVIEW);

	/* Clear the buffer for cases where only part (or none) of   */
	/*   the buffer is used in the rendering.  One case is where */
	/*   anaglyphic stereo only clears two of the colors.        */
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDrawBuffer(GL_FRONT_AND_BACK);	/* NOTE: this is changed during render phase */
	glClearColor(0.0, 0.0, 0.0, 0.0);	/* was (0.1, 0.1, 0.1, 1.0) */
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* For windows that use the dual-viewport method of rendering, enable the Scissor test */
	if (window->visrenmode == VRVISREN_DUALVP) {
		glEnable(GL_SCISSOR_TEST);
	}

#if defined(ENABLE_STENCIL_STEREO_TEST) && 0
	/* NOTE: it seems that stencil stereo also requires the stencil test to be enabled, and for an unknown reason, must be here where the window is opened -- doesn't work in the render function.  I have no idea why not. */
	glEnable(GL_SCISSOR_TEST);		/* TODO: determine why this is necessary -- seems to be the case that it is */
#endif


	if (aux->doub_buf)
		glXSwapBuffers(aux->xdisplay, aux->xwindow);
	else	glFlush();

	sprintf(trace_msg, BOLD_TEXT "Exiting _GlxOpenFunc() for window '%s' %#p\n" NORM_TEXT, window->name, window);
	vrTrace("_GlxOpenFunc", trace_msg);
}
/* end of open function -- for diff match */


/****************************************************************************/
/* TODO: do we need to cleanly close the X windows? */
static void _GlxCloseFunc(vrWindowInfo *window)
{
	vrGlxPrivateInfo	*aux = (vrGlxPrivateInfo *)(window->aux_data);

	/* TODO: should we also free memory of the things aux points to? */
	vrShmemFree(aux);
}


#if 0 /* I started adding this function, but then went with inline code in the render process */
/* The graphics initialization function */
/*   NOTE: frequently this is only called once per window, but that may */
/*     not be until after some of the rendering (of a NULL world) has   */
/*     already been taking place.  Plus it is possible to set a new     */
/*     initialization callback (or pretend one did with the KP-INS key) */
/*     so this function may get called more than once per window        */
/*     (though still infrequently).                                     */
/* This function was created 03/14/2006 when I realized that maybe we   */
/*   can't rely on calling the render function first to get the graphics*/
/*   context.                                                           */
/****************************************************************************/
static void _GlxInitFunc(vrRenderInfo *renderinfo)
{

}
#endif


/****************************************************************************/
static void _GlxSwapFunc(vrWindowInfo *info)
{
	vrGlxPrivateInfo	*aux = (vrGlxPrivateInfo *)(info->aux_data);

	/* If this window hasn't been mapped yet, then skip the swap */
	if (!aux->mapped)
		return;

	glXMakeCurrent(aux->xdisplay, aux->xwindow, aux->glx_context);
	if (aux->doub_buf) {
		glXSwapBuffers(aux->xdisplay, aux->xwindow);
#if 1
		glXWaitGL();
#elif 1
		glFinish();
#endif
	} else {
		glFlush();
	}
}


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
static void _GlxRenderTransform(vrMatrix *mat)
{
#ifdef GFX_OSG
	/* NOTE: In the initial merged-in version, there was some debugging code    */
	/*   for OSG here, which required that this function take a second argument */
	/*   of the vrWindowInfo.  Since that code was #if'd out, we'll leave this  */
	/*   function as is for now.                                                */
#endif
	glMultMatrixd(mat->v);
}


/****************************************************************************/
/* _GlxSetProjectionTransform() allows the top value of the GL projection   */
/*   stack to be set to the corresponding values in the vrPerspData         */
/*   structure.  The pushing/popping of this data is left to the calling    */
/*   function(s) to handle.                                                 */
/* NOTE: this is primarily used for the feature of allowing a 2nd POV to    */
/*   be rendered to the screen for particular elements in the virtual world.*/
static void _GlxSetProjectionTransform(vrPerspData *pd)
{

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
		vrDbgPrintfN(GLX_DBGLVL, "_GlxSetProjectionTransform(): " RED_TEXT "Invalid Frustum, viewpoint on render plane\n" NORM_TEXT);
		glFrustum(-1.0, 1.0, -1.0, 1.0, 0.1, 10.0);	/* NOTE: this is of course, wrong, but at least puts something on the screen. */
	}
#else
	if (!VRMAT_ROWCOL(&pd->mat, VR_W, VR_W) == 0.0) {
		glLoadMatrixd(pd->mat.v);
	} else {
		vrDbgPrintfN(GLX_DBGLVL, "_GlxSetProjectionTransform(): " RED_TEXT "Invalid Perspective Matrix, viewpoint on render plane\n" NORM_TEXT);
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

}


/* The render function */
/****************************************************************************/
static void _GlxRenderFunc(vrRenderInfo *renderinfo)
{
static	int		nocallback_msg = 0;	/* has the no rendering callback message been displayed yet? */
static	char		trace_msg[2048];
	char		*buffer_name;		/* name of the GL buffer selected */
	vrPerspData	*pd = renderinfo->persp;
	vrWindowInfo	*curr_window = renderinfo->window;
	vrEyeInfo	*curr_eye = renderinfo->eye;
	vrUserInfo	*curr_user = curr_eye->user;
	vrGlxPrivateInfo *aux = (vrGlxPrivateInfo *)curr_window->aux_data;
	vrCallback	*callback = NULL;
	XEvent		event;
	int		count;
	int		twod_ortho_mode = 0;	/* set to one when rendering in 2D */

	if (curr_window->name == NULL) {
		printf("_GlxRenderFunc(): something is seriously wrong -- no window name.\n");
		return;
	}
	sprintf(trace_msg, "beginning window render loop for window '%s' %#p", curr_window->name, curr_window);
	vrTrace("_GlxRenderFunc", trace_msg);

	if (!aux->mapped) {
		vrTrace("_GlxRenderFunc", "unmapped window -- skip rendering");
		return;
	}

//printf("xdis = %p, xwin = %p, glx_context = %p\n", aux->xdisplay, aux->xwindow, aux->glx_context);
	glXMakeCurrent(aux->xdisplay, aux->xwindow, aux->glx_context);


	/****************************/
	/* get x events, if any ... */
	while (XPending(aux->xdisplay)) {
		XNextEvent(aux->xdisplay, &event);

		switch (event.type) {

		case ConfigureNotify:
			/* Update the viewport */
#ifdef GFX_OSG
			aux->sceneview->setViewport(0, 0, event.xconfigure.width, event.xconfigure.height);
#else
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

			vrDbgPrintfN(GLX_DBGLVL, "_GlxRenderFunc(): Got a reconfiguration event.  Setting window %#p viewport to (%d, %d) (%dx%d).\n",
				curr_window,
				curr_window->geometry.origX, curr_window->geometry.origY,
				curr_window->geometry.width, curr_window->geometry.height);
			vrDbgPrintfN(GLX_DBGLVL, "_GlxRenderFunc(): viewport_left = %d %d %d %d\n",
				curr_window->viewport_left.origX,
				curr_window->viewport_left.origY,
				curr_window->viewport_left.width,
				curr_window->viewport_left.height);
			vrDbgPrintfN(GLX_DBGLVL, "_GlxRenderFunc(): viewport_right = %d %d %d %d\n",
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
vrPrintf("_GlxRenderFunc(): Got a key press of type %X\n", XLookupKeysym((XKeyEvent *)&event, 0));
#endif
			vrDbgPrintfN(XWIN_DBGLVL, "_GlxRenderFunc(): Got a key press of type %X\n", XLookupKeysym((XKeyEvent *)&event, 0));

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
				vrPrintf("_GlxRenderFunc() (For library debugging only): setting call_visreninit = 1 for '%s'\n", curr_window->name);
				curr_window->call_visreninit = 1;
				break;
#endif

			default:
				if (!key_used_by_sim)
					vrDbgPrintfN(INPUT_DBGLVL, "_GlxRenderFunc(): Got an unused key press of type %X\n", XLookupKeysym((XKeyEvent *)&event, 0));
				break;
			}
		}	break;

		case KeyRelease:
			break;

		case EnterNotify:
			/* I don't need to handle this, but it seems to be an event that is automatically */
			/*   included in the standard event mask, and I want to avoid the warning message,*/
			/*   thus the empty case.                                                         */
			vrDbgPrintfN(DEFAULT_DBGLVL, "_GlxRenderFunc(): got an 'EnterNotify' event for window '%s'\n", curr_window->name);
			break;

		case UnmapNotify:
			vrDbgPrintfN(DEFAULT_DBGLVL, "_GlxRenderFunc(): got an 'UnmapNotify' event for window '%s'\n", curr_window->name);
			break;

		case MapNotify:
			vrDbgPrintfN(DEFAULT_DBGLVL, "_GlxRenderFunc(): got an 'MapNotify' event for window '%s'\n", curr_window->name);
			break;

		default:
			vrDbgPrintfN(DEFAULT_DBGLVL, "_GlxRenderFunc(): got an unhandled X event of type %d\n", event.type);
			break;
		}
	} /* end while pending X events */

	/********************************************/
	/* Handle other possible simulator controls */
	if (curr_window->mount == VRWINDOW_SIMULATOR) {

		/* when the flag is set, continually jump to the user's head position */
		if (curr_window->sim_follow_head) {
			/* TODO: try to convert this into a vrSimulatorMove() operation */
			vrMatrixInvert(curr_window->rw2w_xform, curr_user->head->visren_position);
			vrMatrixPreTranslate3d(curr_window->rw2w_xform, pd->eye[VR_X], pd->eye[VR_Y], pd->eye[VR_Z] + 0.0);
		}
	}


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
vrPrintf("_GlxRenderFunc(): " RED_TEXT "Calling VisrenInit callback for window '%s' -- call_visreninit was set to 1.\n" NORM_TEXT, curr_window->name);

		vrTrace("_GlxRenderFunc", "prep: initialization callback");
		callback = curr_window->VisrenInit;
		vrCallbackInvokeDynamic(callback, 1, renderinfo);
		curr_window->call_visreninit = 0;
		vrTrace("_GlxRenderFunc", "done: initialization callback");
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
	vrTrace("_GlxRenderFunc", "(ii) handle viewport");

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
		/* NOTE: dualeye_buffer gets set to 1 during the window initialization if a quad-buffer request was denied */
		if (curr_window->dualeye_buffer == 1) {
			glDrawBuffer(GL_BACK_RIGHT);
			buffer_name = "GL_BACK_RIGHT";
		} else {
			static	int	dualeye_warning = 0;

			if (dualeye_warning == 0) {
				vrDbgPrintfN(ALWAYS_DBGLVL, "_GlxRenderFunc(): " RED_TEXT "There is no stereo buffer for window '%s', so rendering to right-eye has been disabled.\n" NORM_TEXT, curr_window->name);
				dualeye_warning = 1;
			}

			sprintf(trace_msg, "premature ending window render loop for window '%s' %#p -- non-existant stereo buffer", curr_window->name, curr_window);
			vrTrace("_GlxRenderFunc", trace_msg);

			return;
		}
		break;

	/* NOTE: [5/5/02] We may want a special dualvp rendering mode      */
	/*   that is independent of specifically rendering a left or right */
	/*   eye view.                                                     */
	/* NOTE: [6/11/02: hmmm, now trying to use this as the only option, */
	/*   and for systems with just one eye to a window, the viewport    */
	/*   will take care of things.                                      */
	case VRFB_FULL_LEFTEYE:	  /* NOTE: this will just have a viewport encompassing the entire window */
	case VRFB_SPLIT_LEFTEYE:
		glDrawBuffer(GL_BACK);
		buffer_name = "GL_BACK";
		glViewport(curr_window->viewport_left.origX, curr_window->viewport_left.origY,
			curr_window->viewport_left.width, curr_window->viewport_left.height);
		glScissor(curr_window->viewport_left.origX, curr_window->viewport_left.origY,
			curr_window->viewport_left.width, curr_window->viewport_left.height);
		break;
	case VRFB_FULL_RIGHTEYE:  /* NOTE: this will just have a viewport encompassing the entire window */
	case VRFB_SPLIT_RIGHTEYE:
		glDrawBuffer(GL_BACK);
		buffer_name = "GL_BACK";
		glViewport(curr_window->viewport_right.origX, curr_window->viewport_right.origY,
			curr_window->viewport_right.width, curr_window->viewport_right.height);
		glScissor(curr_window->viewport_right.origX, curr_window->viewport_right.origY,
			curr_window->viewport_right.width, curr_window->viewport_right.height);
		break;
	}
#if 1	/* I don't like removing the error from the OpenGL state at this point, but I at least need to figure out how to keep from attempting to access a bad buffer before this section can be removed. */
    {
	GLenum gl_error = glGetError();
	if (gl_error != 0) {
		if (gl_error == GL_INVALID_OPERATION) {
			vrErrPrintf("_GlxRenderFunc(): " RED_TEXT "attempt to set GL-buffer to '%s' (%d) has failed -- visrenmode is '%s'(%d).\n" NORM_TEXT, buffer_name, curr_eye->render_framebuffer,
			vrVisrenModeName(curr_window->visrenmode), curr_window->visrenmode);
		} else {
			vrErrPrintf("_GlxRenderFunc(): " RED_TEXT" a GL error of type '%s' (%d) has occurred.\n" NORM_TEXT,
				_GlxGLErrorCodeString(gl_error),
				(int)gl_error);
		}
	}
    }
#endif


	/******************************/
	/* (iii) handle color masking */
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
  }
#endif


	/************************/
	/* (iv) handle viewmask */
	/* TODO: perhaps this task should be done after all the rendering is complete */

	/* TODO: this task */


#ifdef GFX_OSG /* NOTE: the OSG-specific version of FreeVR has been deprecated -- it isn't needed, as OSG can now be used with the basic OpenGL version { */
	/************************************************/
	/* (OSG v & vi) Do the OSG scenegraph rendering */
	/* NOTE: we leave open the possibility of rendering pure OpenGL in the next segment */

	/* For performance reasons depth testing should only be on during simulation mode */ 
	if (curr_window->mount == VRWINDOW_SIMULATOR) {
		/***************** ENABLE DEPTH TEST *****************/
		glEnable(GL_DEPTH_TEST); /* We need depth testing for simulator mode and external rendering to work */
		glClear(GL_DEPTH_BUFFER_BIT);
	}

	/* Draw scene */
	/* Scene data is a ref counted pointer this is equivalent to (osg_scenedata.get() == NULL) */
	if(osg_scenedata.valid()) {      
		/* Save our current GL state */
		glPushAttrib(GL_ALL_ATTRIB_BITS & ~GL_TEXTURE_BIT);
		glPushAttrib(GL_TRANSFORM_BIT);
		glPushAttrib(GL_VIEWPORT_BIT);

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();

		glMatrixMode(GL_TEXTURE);
		glPushMatrix();

		/* Setup osg's Viewport */
		aux->sceneview->setComputeNearFarMode(osgUtil::CullVisitor::DO_NOT_COMPUTE_NEAR_FAR);
		aux->sceneview->setCullingMode(osgUtil::CullVisitor::ENABLE_ALL_CULLING);
		aux->sceneview->setViewport(0, 0, curr_window->geometry.width	, curr_window->geometry.height);

		/* Setup osg's Projection and ModelView matrix */
		if (pd->frustum.n.left != -HUGE_VAL) {
			aux->sceneview->setProjectionMatrixAsFrustum(pd->frustum.n.left,
				pd->frustum.n.right,
				pd->frustum.n.bottom,
				pd->frustum.n.top,
				pd->frustum.n.near_clip,
				pd->frustum.n.far_clip);
		} else {
			vrDbgPrintfN(GLX_DBGLVL, "_GlxRenderFunc(): " RED_TEXT "Invalid Frustum, viewpoint on render plane\n" NORM_TEXT);
		}

		osg::Matrixd eyeMat;
		eyeMat.setTrans(-pd->eye[VR_X], -pd->eye[VR_Y], -pd->eye[VR_Z]);
		osg::Matrixd transMat(curr_window->rw2w_xform->v);
		eyeMat.preMult(transMat);
		aux->sceneview->setViewMatrix(eyeMat);

		/* We don't want this but apparently OSG changes stuff during it's READ-ONLY cull traversal!? */
		/* This prevents random segfaults when OSG is changing the scene graph                        */
		/* TODO: Find which nodes or parts of OSG are changing during cull traversal */
		osg::ref_ptr<osg::FrameStamp> frameStamp = new osg::FrameStamp;
		frameStamp->setReferenceTime(osg::Timer::instance()->delta_s(aux->start_tick, osg::Timer::instance()->tick()));
		frameStamp->setFrameNumber(aux->frame_num++);
		aux->sceneview->setFrameStamp(frameStamp.get());

		if (osg_databasePager.valid())
			osg_databasePager->signalBeginFrame(frameStamp.get());

		vrLockWriteSet(osg_sg_lock);
		aux->sceneview->setSceneData(osg_scenedata.get());
		if (osg_databasePager.valid())
			osg_databasePager->updateSceneGraph(frameStamp->getReferenceTime());
#if 0
		aux->sceneview->update();
#endif
		vrLockWriteRelease(osg_sg_lock);

		if (osg_databasePager.valid())
			osg_databasePager->signalEndFrame();
#if 0
		printf("Near: %lf Far: %lf\n", 
			aux->sceneview->getCullVisitor()->getCalculatedNearPlane(), 
			aux->sceneview->getCullVisitor()->getCalculatedFarPlane());
#endif

		/* Render scene */
		vrLockReadSet(osg_sg_lock);
		aux->sceneview->cull();
		aux->sceneview->draw();
		vrLockReadRelease(osg_sg_lock);

		/* Restore our original (pre-osg) GL state */
		glMatrixMode(GL_TEXTURE);
		glPopMatrix();

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();

		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

		glPopAttrib();
		glPopAttrib();
		glPopAttrib();
	}

	/* Setup Viewport and Matrices for simulator or possibly external rendering */
	glViewport(0, 0, curr_window->geometry.width , curr_window->geometry.height);
#endif /* } GFX_OSG section */

	/*****************************************/
	/* (v) put transform matrix on the stack */
	vrTrace("_GlxRenderFunc", "(v) put transform matrix on the stack");

	/* 6/18/01: by putting the viewing transformation on the projection-matrix */
	/*   stack, lighting and environment maps should work.  However, to have   */
	/*   fog effects work the viewing transform needs to go on the modelview   */
	/*   stack instead.                                                        */

	/* TODO: there should be be an option that allows the application to */
	/*   choose to put the perspective matrix on the ModelView stack in  */
	/*   case it prefers to have working fog over working lighting.      */

	glPushMatrix(); /* { */

#if 1 /* since I've just created this new function for an additional purpose, I thought I might also use it here. */
	_GlxSetProjectionTransform(pd);		/* this call puts the proper projection matrices at the top of the PROJECTION & MODELVIEW stacks */

#else /* { */
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
		vrDbgPrintfN(GLX_DBGLVL, "_GlxRenderFunc(): " RED_TEXT "Invalid Frustum, viewpoint on render plane\n" NORM_TEXT);
	}
#else
	if (!VRMAT_ROWCOL(&pd->mat, VR_W, VR_W) == 0.0)
		glLoadMatrixd(pd->mat.v);
	else	vrDbgPrintfN(GLX_DBGLVL, "_GlxRenderFunc(): " RED_TEXT "Invalid Perspective Matrix, viewpoint on render plane\n" NORM_TEXT);
#endif

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
#endif /* } */

	/**************************/
	/* (vi) render the world  */
      if (curr_window->world_show || curr_window->mount != VRWINDOW_SIMULATOR) {	/* NOTE: world rendering can only be disabled in simulator windows */
	vrTrace("_GlxRenderFunc", "(vi) render the world");

	/* set the world rendering callback.  Start with the user's callback because that takes precedence. */
	callback = curr_user->VisrenWorld;
	if (!callback)
		callback = curr_window->VisrenWorld;
	if (!callback) {
		if (nocallback_msg == 0) {
			vrErrPrintf("_GlxRenderFunc(): no callback for rendering the world available\n");
			nocallback_msg = 1;
		}
	}

	/* if a non-default rendermode is set, then set it */
	if (curr_window->frontrendermode != GL_NONE) {
		glPolygonMode(GL_FRONT, curr_window->frontrendermode);
	}
	if (curr_window->backrendermode != GL_NONE) {
		glPolygonMode(GL_BACK, curr_window->backrendermode);
	}

	glPushMatrix(); /* { */
	vrTrace("_GlxRenderFunc", "(vi) invoking the callback");
	/* TODO: beginning with version 0.5a, add the renderinfo to this callback -- 2/27/03, or maybe 0.4f */
	vrCallbackInvokeDynamic(callback, 1, renderinfo);
	glPopMatrix(); /* } */

	/* TODO: check whether GL is in an error state (or has set */
	/*   the error flag), and print a warning if it has.       */
	/*   Probably best (timewise) to only do this when at a    */
	/*   sufficiently high debug level the default is probably */
	/*   good.                                                 */
	/* Hmmmm, why is the error check being done in the _GlxRenderText() routine?  Did I put it in the wrong place? */
	/*   Looking back, I think what happened is that Ed put the error calls int _GlxRenderText() in order to debug */
	/*   that routine, and then they got left there.  Currently, the call is made in phase 1 of the rendering in   */
	/*   vr_visren.c, just before the framebuffer swap.  NOTE: it's called via a callback to the _GlxRenderErrors()*/
	/*   function contained here in this file.  So, ideally, that should be sufficient -- having it there also     */
	/*   covers possible rendering errors in FreeVR, but I should see and solve those right away.                  */

	/* if a non-default rendermode is set, then restore to fill-mode */
	if (curr_window->frontrendermode != GL_NONE) {
		glPolygonMode(GL_FRONT, GL_FILL);
	}
	if (curr_window->backrendermode != GL_NONE) {
		glPolygonMode(GL_BACK, GL_FILL);
	}
       } else {
		/* If we don't render the world, then we at least need to clear the screen */
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
       }

	/* measure: time just spent in rendering each eye (not counting the simulator and information displays) */
	vrProcessStatsMark(curr_window->proc->stats, VR_TIME_RENDER1, 1);

	/*****************************************************/
	/* (vii) call simulator_render (if simulator window) */
	/* NOTE: this is done after world_render because the window */
	/*       is typically cleared by world_render.              */
	if (curr_window->mount == VRWINDOW_SIMULATOR) {
		vrTrace("_GlxRenderFunc", "(vii) call simulator_render");

		callback = curr_user->VisrenSim;
		if (!callback) {
			callback = curr_window->VisrenSim;
		}
		if (callback) {
			vrCallbackInvokeDynamic(callback, 2, renderinfo, curr_window->simulator_mask);
		} else	vrDbgPrintfN(ALWAYS_DBGLVL, "_GlxRenderFunc(): " RED_TEXT "no simulator callback available\n" NORM_TEXT);

#ifdef GFX_OSG
		/***************** DISABLE DEPTH TEST *****************/
		glDisable(GL_DEPTH_TEST);
#endif
	}


	/**************************************************************/
	/* (viii) display extra-world information (fps, stats, etc. ) */


	/*** render the timing statistics as a bar-graph ***/
      if (curr_window->stats_show) {
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


			/* draw semi-transparent backdrop specifically for the element labels */
			glColor4fv(stats->back_color);			/* usually semi-transparent */
			glBegin(GL_POLYGON);
				glVertex2d(0.03, bottom + (2.0 * text_scale));
				glVertex2d(0.03, bottom + (2.6 * text_scale) + (stats->elements * text_scale * 0.95));
				glVertex2d(0.05 + (8 * text_scale), bottom + (2.6 * text_scale) + (stats->elements * text_scale * 0.95));
				glVertex2d(0.05 + (8 * text_scale), bottom + (2.0 * text_scale));
			glEnd();

			/* draw the element labels */
			y = bottom + (2.2 * text_scale);
			for (segment = 0; segment < stats->elements; segment++) {
				glColor4fv(stats->elem_colors[segment]);
				glRasterPos2f(0.04, y);
				_GlxRenderText(renderinfo, stats->elem_labels[segment]);
				y += text_scale /* 0.012 */;	/* TODO: was ... "0.012 * scale", but that doesn't make sense -- perhaps we could have a "window_scale" value based on the size of the window */
			}


			/* draw semi-transparent backdrop specifically for the overall label */
			glColor4fv(stats->back_color);			/* usually semi-transparent */
			glBegin(GL_POLYGON);
				glVertex2d(0.005, bottom + (0.80 * text_scale));
				glVertex2d(0.005, bottom + (1.95 * text_scale));
				glVertex2d(0.060, bottom + (1.95 * text_scale));
				glVertex2d(0.060, bottom + (0.80 * text_scale));
			glEnd();

			/* draw the overall stats label */
			glColor4fv(stats->label_color);
			glRasterPos2f(0.01, bottom + (text_scale /*0.005*/ /*  * scale */));
			_GlxRenderText(renderinfo, stats->label);
		}
	}
      }

	/*** render the frame rate as text ***/
	if (curr_window->fps_show) {
static		char	fps_string[128];

		vrTrace("_GlxRenderFunc", "(viii) display the frame rate");

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
		_GlxRenderText(renderinfo, fps_string);
	}

	/*** render input histories ***/
	if (curr_window->inputs_show) {
static		char		string[256];
		vrInputInfo	*inputs = renderinfo->context->input;	/* aka "vrInputs" */
		int		count;

		vrTrace("_GlxRenderFunc", "(viii) display input histories");

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
				_GlxRenderText(renderinfo, string);
			}
		}
	}

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
			_GlxRenderText(renderinfo, next_line);
			y -= yscale;
		} while (next_line = strtok_r(NULL, "\n", (char **)&ui_string_tok));
	}

	/*** restore to 3D mode if we changed into 2D ortho mode ***/
	if (twod_ortho_mode) {
		glPopMatrix();
		glPopAttrib();
	}

	/* measure: time just spent in rendering the simulator and information displays */
	vrProcessStatsMark(curr_window->proc->stats, VR_TIME_RENDERINFO, 1);


	/**************************************/
	/* TODO: does viewmask stuff go here? */


	/***********************************/
	/* (ix) restore gfx matrix/state */
	glPopMatrix(); /* } */

	sprintf(trace_msg, "ending window render loop for window '%s' %#p", curr_window->name, curr_window);
	vrTrace("_GlxRenderFunc", trace_msg);
}
/* end of render function */


/****************************************************************************/
static void _GlxRenderNullWorld()
{
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}


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

#if 0 /* 05/26/2006: I think Ed put this here, and I don't think we want it here */
	/* clear out any error messages in the glError queue */
	while ((err = glGetError()) != GL_NO_ERROR ) ;
#endif

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

#if 0 /* 05/26/2006: I think Ed put this here, and I don't think we want it here */
	/* print message of any GL errors that occurred within this routine */
	while ((err = glGetError()) != GL_NO_ERROR ) {
#  if 0 /* using gluErrorString() requires the inclusion of the -lGLU library */
		vrDbgPrintf("_GlxRenderText: gl error %s\n", gluErrorString(err));
#  else
		vrDbgPrintf("_GlxRenderText: gl error num %d\n", err);
#  endif
	}
#endif
}


#ifdef GFX_OSG
/****************************************************************************/
/* vrOSGSetSceneData(): Used to set scenegraph that will be rendered.       */
/****************************************************************************/
void vrOSGSetSceneData(osg::Node *sd)
{
	osg_scenedata = sd;
	printf("Setting scene Data!\n");
}

/****************************************************************************/
void vrOSGSetDatabasePager (osgDB::DatabasePager *pager)
{
	osg_databasePager = pager;
}

/****************************************************************************/
void vrOSGBeginModifySceneGraph()
{
	vrLockWriteSet(osg_sg_lock);
}

/****************************************************************************/
void vrOSGEndModifySceneGraph()
{
	vrLockWriteRelease(osg_sg_lock);
}
#endif


/****************************************************************************/
/* vrGlxInitWindowInfo(): setup all the callbacks needed for GLX rendering. */
/****************************************************************************/
void vrGlxInitWindowInfo(vrWindowInfo *info)
{
	vrDbgPrintfN(DEFAULT_DBGLVL, "Initializing callback and version Info for Window at %#p\n", info);

#ifdef GFX_OSG
        osg_sg_lock = vrLockCreateName(vrContext, "OSG scenegraph lock");
#endif

	info->version = (char *)vrShmemStrDup("GLX render window");
	info->PrintAux = vrCallbackCreateNamed("GlxWindow:PrintAux-Def", vrFprintGlxPrivateInfo, 0);
	info->PreOpenInit = vrCallbackCreateNamed("GlxWindow:PreOpenInit-DN", vrDoNothing, 0);
	info->Open = vrCallbackCreateNamed("GlxWindow:Open-Def", _GlxOpenFunc, 1, info);
	info->Close = vrCallbackCreateNamed("GlxWindow:Close-Def", _GlxCloseFunc, 1, info);
	info->Swap = vrCallbackCreateNamed("GlxWindow:Swap-Def", _GlxSwapFunc, 1, info);
	info->Errors = vrCallbackCreateNamed("GlxWindow:Errors-Def", _GlxRenderErrors, 0);
	info->RenderTransform = vrCallbackCreateNamed("GlxWindow:Transform-Def", _GlxRenderTransform, 0);
	info->SetProjectionTransform = vrCallbackCreateNamed("GlxWindow:ProjectionTransform-Def", _GlxSetProjectionTransform, 0);
	info->Render = vrCallbackCreateNamed("GlxWindow:Render-Def", _GlxRenderFunc, 0);
	info->RenderText = vrCallbackCreateNamed("GlxWindow:RenderText-Def", _GlxRenderText, 0);
	info->RenderNullWorld = vrCallbackCreateNamed("GlxWindow:RenderNW-Def", _GlxRenderNullWorld, 0);
	info->RenderSimulator = vrCallbackCreateNamed("GlxWindow:RenderSim-Def", vrGLRenderDefaultSimulator, 0);
}

