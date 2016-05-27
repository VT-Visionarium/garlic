/* ======================================================================
 *
 * HH   HH         vr_visren.glx.h
 * HH   HH         Author(s): Ed Peters, John Stone, Bill Sherman, Michael Penick
 * HHHHHHH         Created: June 4, 1998
 * HH   HH         Last Modified: July 6, 2006
 * HH   HH
 *
 * Header file for FreeVR visual rendering into GLX windows.
 * NOTE: also serves as header of OpenSceneGraph (OSG) rendering interface
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#ifndef __VRVISREN_GLX_H__
#define __VRVISREN_GLX_H__

#include <GL/glx.h>
#if defined(GFX_OSG) && defined(__cplusplus)
#  include <osgDB/ReadFile>
#  include <osgUtil/SceneView>
#  include <osg/Node>
#  include <osg/Timer>
#endif

#if !defined(TEST_APP) /* { */
#include "vr_visren.h"
#endif /* } !TEST_APP */

#undef	ENABLE_STENCIL_STEREO_TEST

#ifdef GFX_PERFORMER
#  define PF_C_API 1
#  include <Performer/pf.h>
#  if 0
#    include <Performer/pfdu.h>
#    include <Performer/pr.h>
#    include <Performer/prmath.h>
#  endif
#endif /* !GFX_PERFORMER */



/**********************************/
/*** Window manager definitions ***/

/* TODO: ideally make FreeVR less Motif-dependent here (but this seems to be the only standard way to control windows) */
#if !defined(__linux) && !defined(__OpenBSD__) && !defined(__NetBSD__) && !defined(__APPLE__) && !defined(__CYGWIN__)
#  include <Xm/MwmUtil.h>
#else
	/* some stuff from Xm/MwmUtil.h that we need (updated to include the bit-length 04/27/12) */
	/* TODO: note that the specific settings to using uint32_t and int32_t do not work at     */
	/*   least with 64-bit Fedora-11.  This is due to a bug in X11.  Thus at least for this   */
	/*   distribution, the use of the "long" types must continue.  However, if the bug in X11 */
	/*   is fixed in other distributions, then this will need to be re-addressed.             */
#  if 0 /* first option is theorectically correct, but practically wrong due to X11 bug */
typedef	struct {
		uint32_t	flags;
		uint32_t	functions;
		uint32_t	decorations;
		int32_t		input_mode;
		uint32_t	status;
	} MotifWmHints;
#  else
typedef	struct {
		unsigned long	flags;
		unsigned long	functions;
		unsigned long	decorations;
		long		input_mode;
		unsigned long	status;
	} MotifWmHints;
#  endif

/* bit definitions for MwmHints.flags */
#define MWM_HINTS_FUNCTIONS     (1L << 0)
#define MWM_HINTS_DECORATIONS   (1L << 1)
#define MWM_HINTS_INPUT_MODE    (1L << 2)
#define MWM_HINTS_STATUS        (1L << 3)

/* bit definitions for MwmHints.functions */
#define MWM_FUNC_ALL            (1L << 0)
#define MWM_FUNC_RESIZE         (1L << 1)
#define MWM_FUNC_MOVE           (1L << 2)
#define MWM_FUNC_MINIMIZE       (1L << 3)
#define MWM_FUNC_MAXIMIZE       (1L << 4)
#define MWM_FUNC_CLOSE          (1L << 5)
#define MWM_FUNC_QUIT_APP       (1L << 23)	/* for 4Dwm */

/* bit definitions for MwmHints.decorations */
#define MWM_DECOR_ALL           (1L << 0)
#define MWM_DECOR_BORDER        (1L << 1)
#define MWM_DECOR_RESIZEH       (1L << 2)
#define MWM_DECOR_TITLE         (1L << 3)
#define MWM_DECOR_MENU          (1L << 4)
#define MWM_DECOR_MINIMIZE      (1L << 5)
#define MWM_DECOR_MAXIMIZE      (1L << 6)

/* values for MwmHints.input_mode */
#define MWM_INPUT_MODELESS                      0
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL     1
#define MWM_INPUT_SYSTEM_MODAL                  2
#define MWM_INPUT_FULL_APPLICATION_MODAL        3

/* bit definitions for MwmHints.status */
#define MWM_TEAROFF_WINDOW      (1L << 0)

/* number of elements of size 32 in _MWM_HINTS */
#define PROP_MOTIF_WM_HINTS_ELEMENTS    5
#define PROP_MWM_HINTS_ELEMENTS         PROP_MOTIF_WM_HINTS_ELEMENTS

/* atom name for _MWM_HINTS property */
#define _XA_MOTIF_WM_HINTS      "_MOTIF_WM_HINTS"
#define _XA_MWM_HINTS           _XA_MOTIF_WM_HINTS

#endif


/**********************************************/
/*** Defined values for X-windows interface ***/

/* these are used to indicate what window decorations should be rendered */
#define	DECORATION_TITLE	0x01
#define	DECORATION_BORDER	0x02
#define	DECORATION_MINMAX	0x04
#define	DECORATION_ALL		0xFF

/* these are for specifying the background and icon bitmap files */
#define	XBM_BACK_FILE	"freevr_back.xbm"
#define	XBM_BACK_BITS	freevr_logo_bits
#define	XBM_BACK_WIDTH	freevr_logo_width
#define	XBM_BACK_HEIGHT	freevr_logo_height
#define	XBM_ICON_FILE	"freevr_icon.xbm"
#define	XBM_ICON_BITS	freevr_icon_bits
#define	XBM_ICON_WIDTH	freevr_icon_width
#define	XBM_ICON_HEIGHT	freevr_icon_height


/*****************************/
/* visual display parameters */
static	int	sing_buf_attribs[] = {
			GLX_RGBA,
			GLX_RED_SIZE, 1,
			GLX_GREEN_SIZE, 1,
			GLX_BLUE_SIZE, 1,
			GLX_DEPTH_SIZE, 1,
			None
		};
static	int	doub_buf_attribs[] = {
			GLX_RGBA,
			GLX_RED_SIZE, 1,
			GLX_GREEN_SIZE, 1,
			GLX_BLUE_SIZE, 1,
			GLX_DEPTH_SIZE, 1,
			GLX_DOUBLEBUFFER,
#ifdef ENABLE_STENCIL_STEREO_TEST /* TODO: this is added for the stencil-stereo method, probably shouldn't be hardcoded */
			GLX_STENCIL_SIZE, 1,	/* TODO: how many bits should be requested? */
#endif
			None
		};
static	int	stereo_buf_attribs[] = {
			GLX_RGBA,
			GLX_RED_SIZE, 1,
			GLX_GREEN_SIZE, 1,
			GLX_BLUE_SIZE, 1,
			GLX_DEPTH_SIZE, 1,
			GLX_DOUBLEBUFFER,
			GLX_STEREO,
			None
		};


/**************************************************/
/* GLX-specific data (private to each GLX window) */
/*   This structure is pointed to by the "aux_data" field of the vrWindowInfo structure.*/
/* NOTE: typically this would go directly in the vr_visren.glx.c source file, but       */
/*   in this case this structure is also required by vr_visren.pf.c, and the            */
/*   vr_input.xwindows.c code.                                                          */
typedef struct {

		/* display connection information of X-screen */
		char 		*xhost;
		int		xserver;
		int		xscreen;

		/* size and decoration of resulting X-screen */
		XSizeHints	xsize_hints;
		int		xscreen_size_x;	/* x-dimension of the X-screen */
		int		xscreen_size_y;	/* y-dimension of the X-screen */
/* TODO: need to make FreeVR less Motif-dependent here */
#if !defined(__linux) && !defined(__OpenBSD__) && !defined(__NetBSD__) && !defined(__APPLE__) || 1
		MotifWmHints	mwm_hints;
#endif
		unsigned int	decorations;	/* CONFIG-arg: bit flags that indicate what decorations to render */

		Display		*xdisplay;	/* the display structure for a window */
		char		*xdisplay_string;/* the name of the display as given by DisplayString() */
		Window		xwindow;
		char		*window_title;	/* CONFIG-arg: string to name the window */
		int		mapped;		/* a boolean to indicate that the window is mapped */
		int		used_by_input;	/* a flag to indicate an inputdevice is watching */
		char		*cursor_name;	/* CONFIG-arg: string to choose the cursor shape */
#ifndef GFX_PERFORMER
		Cursor		cursor;		/* X-windows cursor icon */
#else
		int		cursor;		/* X-windows cursor icon, Performer code */
#endif
		Colormap	xcolormap;
		XSetWindowAttributes xwindow_attr;/* X-window attributes */
		XVisualInfo	*xvisual;
		GLboolean	stereo_buf;	/* whether we have a stereo buffer */
		GLboolean	doub_buf;	/* whether we have a double buffer */
		GLXContext	glx_context;	/* the graphics context */
		XFontStruct	*fontStruct;	/* the X font structure for a context */
		GLuint		fontListBase;	/* the fontListBase for this window   */
#ifdef GFX_PERFORMER
		pfChannel	**pfchan;	/* a list of Performer pfChannels associated with each eye of this window */
		pfPipeWindow	*pipewindow;	/* the Performer pfPipeWindow associated with this window */
#endif
#if defined(GFX_OSG) && defined(__cplusplus)	/* NOTE: it's dangerous to have source files with different ideas about the size of a structure, but in this case, only one source file makes use of the structure definition -- the others want other things from this header */
		osg::ref_ptr<osgUtil::SceneView> sceneview;	/* camera & rendering data for each screen -- used to manage cull and render calls -- similar to Performer channels */
		unsigned int	frame_num;	/* Number of current frame. Used for creating osg::FrameStamp. */
		osg::Timer_t	start_tick;	/* Start tick of beginning of first frame. Used for creating osg::FrameStamp.*/
#endif
	} vrGlxPrivateInfo;


/*****************************/
/*** Function declarations ***/

/* TODO: for OSG, determine whether I need to enclose all the functions in an 'extern "C" { ... }' bracket */
#ifdef __cplusplus
extern "C" {
#endif

int	vrXwindowsErrorHandler(Display *xdisplay, XErrorEvent *event);
Cursor	vrXwindowsMakeCursor(vrGlxPrivateInfo *aux, char *cursor_name);
void	vrGlxInitWindowInfo(vrWindowInfo *);

#ifdef GFX_PERFORMER
/* some Performer functions */
void		vrPfPreframe();
void		vrPfPostframe();
pfChannel	*vrPfMasterChannel(void);
#endif

#if defined(GFX_OSG) && defined(__cplusplus)
/* some OSG specific functions */
void		vrOSGSetSceneData(osg::Node * sd);			/* Set OpenSceneGraph Scene Data */
void		vrOSGSetDatabasePager(osgDB::DatabasePager *pager);	/* Set OpenSceneGraph pager (for PagedLOD) */
void		vrOSGBeginModifySceneGraph();				/* Locking function prior to modifying scenegraph */
void		vrOSGEndModifySceneGraph();				/* Unlocking function post scenegraph modification */
#endif

/* locally scoped functions (ie. used only in vr_visren.glx.c) */
/* TODO: question: so why are these here?   They probably should only be in vr_visren.glx.c */
static	void	_GlxRenderFunc(vrRenderInfo *renderinfo);
static	void	_GlxRenderNullWorld();
#if 0
static	void	_GlxRenderDefaultSimulator(vrRenderInfo *renderinfo, int mask);
#endif
static	void	_GlxRenderText(vrRenderInfo *renderinfo, char *string);

#endif

#ifdef __cplusplus
}
#endif

