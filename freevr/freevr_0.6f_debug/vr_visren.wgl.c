/* ======================================================================
 *
 *  CCCCC          vr_visren.wgl.c
 * CC   CC         Author(s): Sukru Tikves
 * CC              Created: August 5, 2002
 * CC   CC         Last Modified: June 7, 2003
 *  CCCCC
 *
 * Code file for FreeVR visual rendering into a WGL window.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/*************************************************************************

FreeVR USAGE:
	...

TODO:
	...

**************************************************************************/
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>		/* The WGL version of GL/glx.h */

#include <windows.h>

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "vr_visren.h"
#include "vr_visren.wgl.h"
#include "vr_basicgfx.glx.h"	/* needed for vrGLRenderDefaultSimulator() */
#include "vr_callback.h"	/* also included within vr_input.h, but left for clarity */
#if 0 /* this is a test to see if this is necessary */
#include "vr_entity.h"
#endif
#include "vr_input.h"
#include "vr_debug.h"

#define MAX_ARGS_VALUE_CNT 128
#define MAX_INPUT_HANDLER  16
#define MAX_WINDOW_CNT     64

#define WGL_BUF_ALPHA   1
#define WGL_BUF_ACCUM   2
#define WGL_BUF_DEPTH   4
#define WGL_BUF_STENCIL 8
#define WGL_BUF_AUX     16

#define WGL_VERSION "WGL Renderer Window 0.1"


/****************************************************************************/
typedef struct {
		char      sign[4];

		HDC       hDC;
		HGLRC     hRC;
		HWND      hWnd;
		HINSTANCE hInstance;

		DWORD     dwStyle;
		DWORD     dwExStyle;

		DWORD     dwFlags;
		DWORD     dwBuffers;

		char     *title;

		int       initialized;

		int       x, y;
		int       width, height;
		int       stereo;
		int       double_buffer;
		int       color_depth;

		int       font_base;

#if 0
		vrLock    input_lock;
#endif
		HWND      input_handler[MAX_INPUT_HANDLER];
	} _WGLPrivateInfo;


/****************************************/
/**** Locally scoped global variables ***/

static	_WGLPrivateInfo	*windows[MAX_WINDOW_CNT];
static	vrLock		windows_lock;


/****************************************************************************/
static void _WGLCloseFunc(vrWindowInfo *window);
static void _WGLRenderText(char *text);


/****************************************************************************/
static GLvoid _WGLResizeScene(GLsizei width, GLsizei height)
{
	if (height == 0)
		height = 1;

	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, (GLfloat)width/(GLfloat)height, 1.0f, 100.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}


/****************************************************************************/
static int _WGLInitOpenGL(void)
{
	glEnable(GL_DEPTH_TEST);

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDrawBuffer(GL_FRONT_AND_BACK);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glFlush();

	return TRUE;
}


/****************************************************************************/
static void _WGLPostInputMessage(HWND win, UINT msg, WPARAM wPrm, LPARAM lPrm)
{
	int	win_count;
	int	inp_count;

	if (!windows_lock)
		return;
	vrLockReadSet(windows_lock);

	for (win_count = 0; win_count < MAX_WINDOW_CNT; win_count++)
		if (windows[win_count] && windows[win_count]->hWnd == win) {

#if 0
			if (!windows[win_count]->input_lock)
				break;
			vrLockReadSet(windows[win_count]->input_lock);
#endif

			for (inp_count = 0; inp_count < MAX_INPUT_HANDLER; inp_count++)
				if (windows[win_count]->input_handler[inp_count])
					PostMessage(windows[win_count]->input_handler[inp_count], msg, wPrm, lPrm);

#if 0
			vrLockReadRelease(windows[win_count]->input_lock);
#endif
		}

	vrLockReadRelease(windows_lock);
}


/****************************************************************************/
static LRESULT CALLBACK _Win32WindowProcess(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {

	case WM_SYSCOMMAND:
		switch(wParam) {

		case SC_SCREENSAVE:
		case SC_MONITORPOWER:
			return 0;
		}
		break;

	case WM_CLOSE:
		/* NOTE: We ignore close requests... */
		return 0;

	case WM_SIZE:
		_WGLResizeScene(LOWORD(lParam), HIWORD(lParam));
		_WGLPostInputMessage(window, message, wParam, lParam);
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		SetCapture(window);
		_WGLPostInputMessage(window, message, wParam, lParam);
		return 0;

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		ReleaseCapture();

	case WM_KEYDOWN:
	case WM_KEYUP:
		_WGLPostInputMessage(window, message, wParam, lParam);
		return 0;
	}

	return DefWindowProc(window, message, wParam, lParam);
}


/****************************************************************************/
/* TODO: see if this section can be replaced by one of the functions in vr_parse.c */
#ifndef USE_FREEVR_PARSING /* { */


/****************************************************************************/
static char *get_token(char **line)
{
	char *token;

	if (!line || !*line)
		return NULL;

	for ( ; **line; (*line)++)
		switch (**line) {

		case ' ':
		case '\t':
		case '\n':
		case '\r':

			break;

		case '=':
		case ',':
		case ';':
			token = (char *)malloc(2 * sizeof(char));
			token[0] = *(*line)++;
			token[1] = 0;

			return token;

		case '"':
			(*line)++;
			token = *line;
			for ( ; **line && **line != '"'; (*line)++)
				;
			if (**line == '"')
				*(*line)++ = 0;

			return strdup(token);

		default:
			token = *line;
			for ( ; **line && !isspace(**line) &&
					**line != '"' &&
					**line != '=' &&
					**line != ',' &&
					**line != ';'; (*line)++)
				;

			if (**line) {
				char old = **line;

				**line = 0;
				token = strdup(token);
				**line = old;

				return token;
			}

			return strdup(token);
		}

	return NULL;
}


/****************************************************************************/
typedef struct
{
	char	*name;
	char	**values;
	int	value_cnt;
} ARGS_ELEMENT;


/****************************************************************************/
ARGS_ELEMENT *new_args_element(char *name, int value_cnt, char **values)
{
	ARGS_ELEMENT *element;

	if (!name || !*name)
		return NULL;

	element = (ARGS_ELEMENT *)malloc(sizeof(ARGS_ELEMENT));
	element->name = name;
	element->value_cnt = value_cnt;

	if (element->value_cnt) {
		int	count;

		element->values = (char **)
			malloc(element->value_cnt * sizeof(char *));
		for (count = 0; count < element->value_cnt; count++)
			element->values[count] = values[count];
	} else {
		element->values = NULL;
	}

	return element;
}


/****************************************************************************/
ARGS_ELEMENT *get_args_element(char **args)
{
	char	*name;
	char	*values[MAX_ARGS_VALUE_CNT];
	char	*value;
	int	value_cnt;

	do {
		name = get_token(args);

		if (!name)
			return NULL;

		if (!*name || *name == ';') {
			free(name);
			continue;
		}

		if (*name == '=' || *name == ',') {
			vrDbgPrintfN(ALWAYS_DBGLVL, RED_TEXT
					"WGL: Syntax error in args "
					"[empty argument name]\n" NORM_TEXT);

			do {
				free(name);
				name = get_token(args);
			} while (name && *name != ';');

			if (!name)
				return NULL;
			else	free(name);

			continue;
		}

		value = get_token(args);

		if (!value)
			return new_args_element(name, 0, NULL);

		if (*value == ';') {
			free(value);
			return new_args_element(name, 0, NULL);
		}

		if (*value == '=' || *name == ',') {
			free(value);
			value = get_token(args);
		} else {
			vrDbgPrintfN(ALWAYS_DBGLVL, RED_TEXT
					"WGL: Syntax error in args "
					"['=' expected]\n" NORM_TEXT);
		}

		value_cnt = 0;
		do {
			if (!value)
				break;

			if (!*value || *value == ';') {
				free(value);
				break;
			}

			if (*value == '=' || *value == ',') {
				free(value);
				value = get_token(args);
				continue;
			}

			if (value_cnt == MAX_ARGS_VALUE_CNT) {
				vrDbgPrintfN(ALWAYS_DBGLVL, RED_TEXT
						"WGL: Syntax error in args "
						"[too many values]\n"
						NORM_TEXT);
				free(value);
				continue;
			}

			values[value_cnt++] = value;
			value = get_token(args);
		} while (1);

		return new_args_element(name, value_cnt, values);
	} while (1);

	return NULL;
}


/****************************************************************************/
void free_args_element(ARGS_ELEMENT *element)
{
	if (!element)
		return;

	if (element->name)
		free(element->name);

	if (element->values) {
		int	count;

		for (count = 0; count < element->value_cnt; count++)
			free(element->values[count]);
		free(element->values);
	}

	free(element);
}


/****************************************************************************/
static void parse_geometry_string(const char *param, int *x, int *y, int *w, int *h)
{
	char	*geometry = strdup(param);
	char	*pos;

	if ((pos = strchr(geometry, 'x')) == NULL) {
		free(geometry);
		return;
	}

	*pos++ = 0;
	if (atoi(geometry) > 0)
		*w = atoi(geometry);
	if (atoi(pos) > 0)
		*h = atoi(pos);

	if ((pos = strchr(pos, '+')) == NULL) {
		free(geometry);
		return;
	}

	*x = atoi(++pos);

	if ((pos = strchr(pos, '+')) == NULL) {
		free(geometry);
		return;
	}

	*y = atoi(++pos);
}


/****************************************************************************/
static void parse_decoration(ARGS_ELEMENT *element, _WGLPrivateInfo *info)
{
	int	count;

	/* NOTE: It would be good to separate "borders" and "title" sometime... */

	/* TODO: handle "minmax" and "all" decoration options */

	for (count = 0; count < element->value_cnt; count++) {
		if (!strcasecmp(element->values[count], "borders")) {
			info->dwStyle &= ~WS_POPUP;
			info->dwStyle |= WS_OVERLAPPEDWINDOW;
		} else if (!strcasecmp(element->values[count], "border")) {
			info->dwStyle &= ~WS_POPUP;
			info->dwStyle |= WS_OVERLAPPEDWINDOW;
			info->dwExStyle |= WS_EX_WINDOWEDGE;
		} else if (!strcasecmp(element->values[count], "title")) {
			info->dwStyle &= ~WS_POPUP;
			info->dwStyle |= WS_OVERLAPPEDWINDOW;
			info->dwExStyle |= WS_EX_WINDOWEDGE;
		} else if (!strcasecmp(element->values[count], "titlebar")) {
			info->dwStyle &= ~WS_POPUP;
			info->dwStyle |= WS_OVERLAPPEDWINDOW;
			info->dwExStyle |= WS_EX_WINDOWEDGE;
		} else if (!strcasecmp(element->values[count], "window")) {
			info->dwStyle &= ~WS_POPUP;
			info->dwStyle |= WS_OVERLAPPEDWINDOW;
			info->dwExStyle |= WS_EX_WINDOWEDGE;
		} else if (!strcasecmp(element->values[count], "none")) {
			info->dwStyle = WS_POPUP;
			info->dwExStyle = WS_EX_APPWINDOW;
		} else {
			vrErrPrintf("WGL: Unknown decoration '%s'\n", element->values[count]);
		}
	}
}


/****************************************************************************/
static void parse_buffers(ARGS_ELEMENT *element, _WGLPrivateInfo *info)
{
	int	count;

	for (count = 0; count < element->value_cnt; count++) {
		if (!strcasecmp(element->values[count], "double"))
			info->dwFlags |= PFD_DOUBLEBUFFER;
		else if (!strcasecmp(element->values[count], "single"))
			info->dwFlags &= ~PFD_DOUBLEBUFFER;
		else if (!strcasecmp(element->values[count], "stereo"))
			info->dwFlags |= PFD_STEREO;
		else if (!strcasecmp(element->values[count], "mono"))
			info->dwFlags &= ~PFD_STEREO;
		else if (!strcasecmp(element->values[count], "depth"))
			info->dwBuffers |= WGL_BUF_DEPTH;
		else if (!strcasecmp(element->values[count], "nodepth"))
			info->dwBuffers &= ~WGL_BUF_DEPTH;
		else if (!strcasecmp(element->values[count], "alpha"))
			info->dwBuffers |= WGL_BUF_ALPHA;
		else if (!strcasecmp(element->values[count], "noalpha"))
			info->dwBuffers &= ~WGL_BUF_ALPHA;
		else if (!strcasecmp(element->values[count], "accum"))
			info->dwBuffers |= WGL_BUF_ACCUM;
		else if (!strcasecmp(element->values[count], "noaccum"))
			info->dwBuffers &= ~WGL_BUF_ACCUM;
		else if (!strcasecmp(element->values[count], "stencil"))
			info->dwBuffers |= WGL_BUF_STENCIL;
		else if (!strcasecmp(element->values[count], "nostencil"))
			info->dwBuffers &= ~WGL_BUF_STENCIL;
		else if (!strcasecmp(element->values[count], "aux"))
			info->dwBuffers |= WGL_BUF_AUX;
		else if (!strcasecmp(element->values[count], "noaux"))
			info->dwBuffers &= ~WGL_BUF_AUX;
		else
			vrErrPrintf("WGL: Unknown buffer type '%s'\n", element->values[count]);
	}
}

#endif /* } USE_FREEVR_PARSING */

/****************************************************************************/
/* TODO: include this functionality directly in __WGLParseArgs */
static char *get_decoration_str(_WGLPrivateInfo *info)
{
static	char	decoration[128];

	if (info->dwStyle & WS_POPUP)
		strcpy(decoration, "notitle");
	else	strcpy(decoration, "title");

	if (info->dwExStyle & WS_EX_WINDOWEDGE)
		strcat(decoration, ", borders");
	else	strcat(decoration, ", noborders");

	return decoration;
}


/****************************************************************************/
/* TODO: include this functionality directly in __WGLParseArgs */
static char *get_buffers_str(_WGLPrivateInfo *info)
{
static	char	buffers[128];

	if (info->dwFlags & PFD_DOUBLEBUFFER)
		strcpy(buffers, "double");
	else	strcpy(buffers, "single");

	if (info->dwFlags & PFD_STEREO)
		strcat(buffers, ", stereo");

	if (info->dwBuffers & WGL_BUF_ALPHA)
		strcat(buffers, ", alpha");

	if (info->dwBuffers & WGL_BUF_ACCUM)
		strcat(buffers, ", accum");

	if (info->dwBuffers & WGL_BUF_DEPTH)
		strcat(buffers, ", depth");

	if (info->dwBuffers & WGL_BUF_STENCIL)
		strcat(buffers, ", stencil");

	if (info->dwBuffers & WGL_BUF_AUX)
		strcat(buffers, ", aux");

	return buffers;
}


/****************************************************************************/
static void _WGLParseArgs(_WGLPrivateInfo *info, char *args)
{
	ARGS_ELEMENT	*element;

	vrDbgPrintfN(DEFAULT_DBGLVL, NORM_TEXT BOLD_TEXT "WGL: Arguments = '%s'\n", args);

	while ((element = get_args_element(&args)) != NULL) {
		if (!strcasecmp(element->name, "geometry"))
			if (element->value_cnt != 1)
				vrErr("WGL: Invalid geometry string\n");
			else	parse_geometry_string(element->values[0],
						&info->x, &info->y,
						&info->width, &info->height);
		else if (!strcasecmp(element->name, "decoration"))
			parse_decoration(element, info);
		else if (!strcasecmp(element->name, "buffers"))
			parse_buffers(element, info);
		else if (!strcasecmp(element->name, "color_depth"))
			if (element->value_cnt != 1) {
				vrErr("WGL: Invalid color_depth value\n");
			} else {
				if (atoi(element->values[0]) > 0)
					info->color_depth =
						atoi(element->values[0]);
			}
		else
			vrErrPrintf("WGL: Unknown argument '%s'\n",
					element->name);

		free_args_element(element);
	}

	vrDbgPrintf("WGL: =============================================\n");
	vrDbgPrintf("WGL:  Done parsing arguments string.\n");
	vrDbgPrintf("WGL:  info->geometry = %dx%d+%d+%d\n", info->width, info->height, info->x, info->y);
	vrDbgPrintf("WGL:  info->decoration = %s\n", get_decoration_str(info));
	vrDbgPrintf("WGL:  info->buffers = %s\n", get_buffers_str(info));
	vrDbgPrintf("WGL:  info->color_depth = %d\n", info->color_depth);
	vrDbgPrintf("WGL: =============================================\n");
}


/****************************************************************************/
static void _WGLOpenFunc(vrWindowInfo *window)
{
	_WGLPrivateInfo		*info;
	char			trace_msg[256];
	GLuint			pixel_format;
	WNDCLASS 		wc;
	RECT			window_rect;
	PIXELFORMATDESCRIPTOR	pfd;
	int			count;

	sprintf(trace_msg, BOLD_TEXT "Entering for window '%s' %#p" NORM_TEXT, window->name, window);
	vrTrace("_WGLOpenFunc", trace_msg);

	info = (_WGLPrivateInfo *)vrShmemAlloc0(sizeof(_WGLPrivateInfo));
	window->aux_data = info;

	info->sign[0] = 'W';
	info->sign[1] = 'G';
	info->sign[2] = 'L';

	info->title = vrShmemAlloc(30 + strlen(window->name));
	sprintf(info->title, "FreeVR: %s window", window->name);

	info->x = 100;
	info->y = 100;
	info->width = 200;
	info->height = 200;

	info->dwExStyle = WS_EX_APPWINDOW;
	info->dwStyle = WS_POPUP;

	info->dwFlags = PFD_DOUBLEBUFFER | PFD_STEREO;
	info->dwBuffers = WGL_BUF_DEPTH;

	info->color_depth = 16;

	_WGLParseArgs(info, window->args);

	window_rect.left   = info->x;
	window_rect.right  = info->x + info->width;
	window_rect.top    = info->y;
	window_rect.bottom = info->y + info->height;

	info->hInstance      = GetModuleHandle(NULL);
	wc.style             = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc       = (WNDPROC)_Win32WindowProcess;
	wc.cbClsExtra        = 0;
	wc.cbWndExtra        = 0;
	wc.hInstance         = info->hInstance;
	wc.hIcon             = LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor           = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground     = NULL;
	wc.lpszMenuName      = NULL;
	wc.lpszClassName     = "OpenGL";

	if (!RegisterClass(&wc)) {
		vrErr("WGL: Failed to register window class.\n");
		return;
	}

	AdjustWindowRectEx(&window_rect, info->dwStyle, FALSE, info->dwExStyle);

	if (!(info->hWnd = CreateWindowEx(
					info->dwExStyle,
					"OpenGL",
					info->title,
					info->dwStyle |
					WS_CLIPSIBLINGS |
					WS_CLIPCHILDREN,
					window_rect.left,
					window_rect.top,
					window_rect.right - window_rect.left,
					window_rect.bottom - window_rect.top,
					NULL,
					NULL,
					info->hInstance,
					NULL))) {
		_WGLCloseFunc(window);
		vrErr("WGL: Could not open OpenGL window.\n");
		return;
	}

	pfd.nSize        = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion     = 1;
	pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | info->dwFlags;
	pfd.iPixelType   = PFD_TYPE_RGBA;
	pfd.cColorBits   = info->color_depth;
	pfd.cRedBits     = 0;
	pfd.cRedShift    = 0;
	pfd.cGreenBits   = 0;
	pfd.cGreenShift  = 0;
	pfd.cBlueBits    = 0;
	pfd.cBlueShift   = 0;
	pfd.cAlphaBits   = (info->dwBuffers & WGL_BUF_ALPHA) ? 4 : 0;
	pfd.cAlphaShift  = 0;
	pfd.cAccumBits   = (info->dwBuffers & WGL_BUF_ACCUM) ? 8 : 0;
	pfd.cAccumRedBits = 0;
	pfd.cAccumGreenBits = 0;
	pfd.cAccumBlueBits = 0;
	pfd.cAccumAlphaBits = 0;
	pfd.cDepthBits   = (info->dwBuffers & WGL_BUF_DEPTH) ? 16 : 0;
	pfd.cStencilBits = (info->dwBuffers & WGL_BUF_STENCIL) ? 16 : 0;
	pfd.cAuxBuffers  = (info->dwBuffers & WGL_BUF_AUX) ? 16 : 0;
	pfd.iLayerType   = PFD_MAIN_PLANE;
	pfd.bReserved    = 0;
	pfd.dwLayerMask  = 0;
	pfd.dwVisibleMask = 0;
	pfd.dwDamageMask = 0;

	if (!(info->hDC = GetDC(info->hWnd))) {
		_WGLCloseFunc(window);
		vrErr("WGL: Can't create a GL Device Context.\n");
		return;
	}

	if (!(pixel_format = ChoosePixelFormat(info->hDC, &pfd))) {
		_WGLCloseFunc(window);
		vrErr("WGL: Can't find a suitable pixel format.\n");
		return;
	}

	DescribePixelFormat(info->hDC, pixel_format, sizeof(pfd), &pfd);

	info->stereo = pfd.dwFlags & PFD_STEREO;
	info->double_buffer = pfd.dwFlags & PFD_DOUBLEBUFFER;

	window->dualeye_buffer = pfd.dwFlags & PFD_STEREO;
	if (window->dualeye_buffer)
		vrDbgPrintfN(ALWAYS_DBGLVL, NORM_TEXT BOLD_TEXT
				"WGL: Stereo buffer enabled.\n" NORM_TEXT);

	if (!SetPixelFormat(info->hDC, pixel_format, &pfd)) {
		_WGLCloseFunc(window);
		vrErr("WGL: Can't set the pixel format.\n");
		return;
	}

	if (!(info->hRC = wglCreateContext(info->hDC))) {
		_WGLCloseFunc(window);
		vrErr("WGL: Can't create GL rendering context.\n");
		return;
	}

	if (!wglMakeCurrent(info->hDC, info->hRC)) {
		_WGLCloseFunc(window);
		vrErr("WGL: Can't activate the GL rendering context.\n");
		return;
	}

	ShowWindow(info->hWnd, SW_SHOW);
	SetForegroundWindow(info->hWnd);
	SetFocus(info->hWnd);
	_WGLResizeScene(info->width, info->height);

	if (!_WGLInitOpenGL()) {
		_WGLCloseFunc(window);
		vrErr("WGL: Cannot initialize OpenGL.\n");
		return;
	}

#if 0
	info->input_lock = vrLockCreateName(vrContext, "wgl input");
#endif

	if (windows_lock) {
		vrLockWriteSet(windows_lock);
		for (count = 0; count < MAX_WINDOW_CNT; count++)
			if (!windows[count]) {
				windows[count] = info;
				break;
			}
		vrLockWriteRelease(windows_lock);

		if (count == MAX_WINDOW_CNT)
			vrErr("WGL: Too many windows in process. Disabling "
					"input handling for window.\n");
	} else {
		vrErr("WGL: Window list is not initialized.\n");
	}

	info->initialized = 1;

	sprintf(trace_msg, BOLD_TEXT "Exiting for window '%s' %#p" NORM_TEXT, window->name, window);
	vrTrace("_WGLOpenFunc", trace_msg);
}


/****************************************************************************/
static void _WGLCloseFunc(vrWindowInfo *window)
{
	_WGLPrivateInfo	*info = (_WGLPrivateInfo *)window->aux_data;
	int		count;

	vrTrace("_WGLCloseFunc", "closing window");

	if (windows_lock) {
		vrLockWriteSet(windows_lock);
		for (count = 0; count < MAX_WINDOW_CNT; count++)
			if (windows[count] == info)
				windows[count] = NULL;
		vrLockWriteRelease(windows_lock);
	}

#if 0
	if (info->input_lock) {
		vrLockWriteSet(info->input_lock);

		for (count = 0; count < MAX_INPUT_HANDLER; count++)
			if (info->input_handler[count])
				SendMessage(info->input_handler[count], WM_CLOSE, 0, 0);

		/* TODO: There may be a problem if an input device tries */
		/*       to attach before this function returns.         */
		vrLockWriteRelease(info->input_lock);
		vrLockFree(info->input_lock);
	}
#endif

	if (info->hRC) {
		if (!wglMakeCurrent(NULL, NULL))
			vrErr("WGL: Cannot set NULL OpenGL context.\n");
		if (!wglDeleteContext(info->hRC))
			vrErr("WGL: Release of rendering context failed.\n");
		info->hRC = NULL;
	}

	if (info->hDC && !ReleaseDC(info->hWnd, info->hDC))
		vrErr("WGL: Release of device context failed.\n");
	info->hDC = NULL;

	if (info->hWnd && !DestroyWindow(info->hWnd))
		vrErr("WGL: Could not release window handle.\n");
	info->hWnd = NULL;

	vrShmemFree(info);
	window->aux_data = NULL;

	vrTrace("_WGLCloseFunc", "exit");
}


/****************************************************************************/
static void _WGLRenderFunc(vrRenderInfo *render_info)
{
	char		trace_msg[256];
	MSG		msg;

	vrWindowInfo	*window = render_info->window;
	_WGLPrivateInfo	*info   = (_WGLPrivateInfo *)window->aux_data;
	vrPerspData	*pd     = render_info->persp;
	vrEyeInfo	*eye    = render_info->eye;
	vrUserInfo	*user   = eye->user;
	vrCallback	*callback = NULL;

	sprintf(trace_msg, "Beginning of render for window '%s' at %#p", window->name, window);
	vrTrace("_WGLRenderFunc", trace_msg);

	if (!info) {
		vrErr("WGL: Cannot render NULL window.\n");
		return;
	}

	if (!wglMakeCurrent(info->hDC, info->hRC)) {
		vrErr("WGL: Cannot set OpenGL context.\n");
		return;
	}

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) {
			/* NOTE: Normally we should not get WM_QUIT unless user forces */
			/*   "End Task". We currently obey this end terminate.         */
			_WGLCloseFunc(window);
			return;
		} else {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	glPushMatrix();

	vrTrace("_WGLRenderFunc", "(ii) handle viewport");

	switch (eye->render_framebuffer) {

	default:
		vrMsgPrintf("By default rendering to back buffer\n");
	case VRFB_FULL:
		glDrawBuffer(GL_BACK);
		break;

	case VRFB_LEFT:
		glDrawBuffer(GL_BACK_LEFT);
		break;

	case VRFB_RIGHT:
		glDrawBuffer(GL_BACK_RIGHT);
		break;

	case VRFB_FULL_LEFTEYE:
	case VRFB_SPLIT_LEFTEYE:
		glDrawBuffer(GL_BACK);
		glViewport(window->viewport_left.origX,
				window->viewport_left.origY,
				window->viewport_left.width,
				window->viewport_left.height);
		glScissor(window->viewport_left.origX,
				window->viewport_left.origY,
				window->viewport_left.width,
				window->viewport_left.height);
		break;

	case VRFB_FULL_RIGHTEYE:
	case VRFB_SPLIT_RIGHTEYE:
		glDrawBuffer(GL_BACK);
		glViewport(window->viewport_right.origX,
				window->viewport_right.origY,
				window->viewport_right.width,
				window->viewport_right.height);
		glScissor(window->viewport_right.origX,
				window->viewport_right.origY,
				window->viewport_right.width,
				window->viewport_right.height);
		break;
	}

	switch (eye->color) {

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
		vrDbgPrintf(RED_TEXT "WGL: Invalid frustum, "
				"viewpoint on render plane\n" NORM_TEXT);
	}
#else
	if (!VRMAT_ROWCOL(&pd->mat, VR_W, VR_W) == 0.0)
		glLoadMatrixd(pd->mat.v)
	else	vrDbgPrintf(RED_TEXT "WGL: Invalid perspective matrix, viewpoint on render plane\n" NORM_TEXT);
#endif

	vrTrace("_WGLRenderFunc", "(vi) render the world");

	callback = user->VisrenWorld;
	if (!callback)
		callback = window->VisrenWorld;
	if (!callback)
		callback = vrContext->callbacks->VisrenWorld;
	if (!callback)
		vrErr("WGL: No callback for rendering the world available.\n");

	glPushMatrix();
	vrTrace("_WGLRenderFunc", "(vi) invoking the callback");
	vrCallbackInvoke(callback);
	glPopMatrix();

	if (window->mount == VRWINDOW_SIMULATOR) {
		vrTrace("_WGLRenderFunc", "(vii) call simulator_render");

		callback = user->VisrenSim;
		if (!callback)
			callback = window->VisrenSim;
		if (!callback)
			callback = vrContext->callbacks->VisrenSim;
		if (callback)
			vrCallbackInvokeDynamic(callback, 2, render_info, window->simulator_mask);
		else	vrErr("WGL: No callback for simulator available.\n");
	}

	if (window->fps_show) {
		char fps_string[128];

		vrTrace("_WGLRenderFunc", "(viii) display the frame rate");

		glPushAttrib(
				GL_CURRENT_BIT |
				GL_ENABLE_BIT |
				GL_TRANSFORM_BIT
			    );

		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_1D);
		glDisable(GL_TEXTURE_2D);

		glDisable(GL_DEPTH_TEST);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glColor3ub((GLubyte)window->fps_color[0],
				(GLubyte)window->fps_color[1],
				(GLubyte)window->fps_color[2]);
		glRasterPos2f(window->fps_loc[0], window->fps_loc[1]);
		sprintf(fps_string, "FPS: %5.2f(1) %5.2f(10)",
				window->proc->fps1, window->proc->fps10);
		_WGLRenderText(fps_string);
		glPopMatrix();
		glPopAttrib();
	}

	glPopMatrix();

	sprintf(trace_msg, "Ending render for window '%s' at %#p", window->name, window);
	vrTrace("_WGLRenderFunc", trace_msg);
}


/****************************************************************************/
static void _WGLRenderText(char *string)
{
	GLenum		err;

	vrWindowInfo	*curr_window = vrCurrentWindow();
	_WGLPrivateInfo *info = (_WGLPrivateInfo *)curr_window->aux_data;

	while ((err = glGetError()) != GL_NO_ERROR)
		;

	if (!info->font_base) {
		HFONT font;
		HFONT old;

		info->font_base = glGenLists(96);

		font = CreateFont(-24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
				ANSI_CHARSET, OUT_TT_PRECIS,
				CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
				FF_DONTCARE | DEFAULT_PITCH,
				"Courier New");

		old = (HFONT)SelectObject(info->hDC, font);
		wglUseFontBitmaps(info->hDC, 32, 96, info->font_base);
		SelectObject(info->hDC, old);
		DeleteObject(font);
	}

	glPushAttrib(GL_LIST_BIT);
	glListBase(info->font_base - 32);

	glCallLists(strlen(string), GL_UNSIGNED_BYTE, (GLubyte *)string);

	glPopAttrib();

	while ((err = glGetError()) != GL_NO_ERROR)
		vrDbgPrintf("WGL: RenderText: GL error #%d\n", err);
}


/****************************************************************************/
static void _WGLRenderNullWorld(void)
{
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}


/****************************************************************************/
static void _WGLRenderTransform(vrMatrix *mat)
{
	glMultMatrixd(mat->v);
}


/****************************************************************************/
static void _WGLSwapFunc(vrWindowInfo *window)
{
	_WGLPrivateInfo *info = (_WGLPrivateInfo *)window->aux_data;

	if (!info)
		return;

	if (info->double_buffer) {
		SwapBuffers(info->hDC);
	} else {
		if (wglMakeCurrent(info->hDC, info->hRC))
			glFlush();
	}
}


/****************************************************************************/
int vrIsWGLWindow(vrWindowInfo *window)
{
	_WGLPrivateInfo	*info;
	int		wait;

	if (!window)
		return 0;

	if (strcmp(window->version, WGL_VERSION))
		return 0;

	for (wait = 0; !window->aux_data && wait < 5; wait++)
		sleep(1);

	if (!window->aux_data)
		return 0;

	info = (_WGLPrivateInfo *)window->aux_data;

	if (info->sign[0] != 'W' || info->sign[1] != 'G' || info->sign[2] != 'L' || info->sign[3])
		return 0;

	return 1;
}


/****************************************************************************/
int vrWGLRegisterInputHandler(vrWindowInfo *window, HWND hWnd)
{
	_WGLPrivateInfo	*info;
	int	 	count;

	vrDbgPrintfN(DEFAULT_DBGLVL, "WGL: RegisterInputHandler\n");

	if (!vrIsWGLWindow(window))
		return 0;

	info = (_WGLPrivateInfo *)window->aux_data;

	while (!info->initialized) {
		vrDbgPrintfN(ALWAYS_DBGLVL, "WGL: RegisterInputHandler: "
				"Waiting for window...\n");
		sleep(1);
	}

#if 0
	vrLockWriteSet(info->input_lock);
#endif

	for (count = 0; count < MAX_INPUT_HANDLER; count++)
		if (info->input_handler[count] == NULL) {
			info->input_handler[count] = hWnd;
#if 0
			vrLockWriteRelease(info->input_lock);
#endif
			return 1;
		}

#if 0
	vrLockWriteRelease(info->input_lock);
#endif

	return 0;
}


/****************************************************************************/
int vrWGLUnRegisterInputHandler(vrWindowInfo *window, HWND hWnd)
{
	_WGLPrivateInfo	*info;
	int		count;

	if (!vrIsWGLWindow(window))
		return 0;

	info = (_WGLPrivateInfo *)window->aux_data;

	if (!info->initialized) {
		vrErr("WGL: UnRegisterInputHandler: "
				"Window not initialized!\n");
		return 0;
	}

#if 0
	vrLockWriteSet(info->input_lock);
#endif

	for (count = 0; count < MAX_INPUT_HANDLER; count++)
		if (info->input_handler[count] == hWnd)
			info->input_handler[count] = NULL;

#if 0
	vrLockWriteRelease(info->input_lock);
#endif
	return 1;
}


/****************************************************************************/
void vrWGLInitWindowInfo(vrWindowInfo *info)
{
	vrDbgPrintfN(DEFAULT_DBGLVL,
			"WGL: Initializing callback and version Info for "
			"Window at %#p\n", info);

	if (!windows_lock) {
		vrDbgPrintf("WGL: Registering window list lock.\n");

#if 0
		windows_lock = vrLockCreateTraced(vrContext, "wgl window");
#else
		windows_lock = vrLockCreateName(vrContext, "wgl window");
#endif
	}

	info->version         = (char *)vrShmemStrDup(WGL_VERSION);
	info->PreOpenInit     = vrCallbackCreateNamed("WglWindow:PreOpenInit-DN", vrDoNothing, 0);
	info->Open            = vrCallbackCreateNamed("WglWindow:Opent-Def", _WGLOpenFunc, 1, info);
	info->Close           = vrCallbackCreateNamed("WglWindow:Close-Def", _WGLCloseFunc, 1, info);
	info->Render          = vrCallbackCreateNamed("WglWindow:Render-Def", _WGLRenderFunc, 0);
	info->RenderText      = vrCallbackCreateNamed("WglWindow:RenderText-Def", _WGLRenderText, 0);
	info->RenderNullWorld = vrCallbackCreateNamed("WglWindow:RenderNW-Def", _WGLRenderNullWorld, 0);
#if 1 /* TODO: Use this instead when inputs are ready... */
	info->RenderSimulator = vrCallbackCreateNamed("WglWindow:RenderSim-Def", vrGLRenderDefaultSimulator, 0);
#else
	info->RenderSimulator = vrCallbackCreateNamed("WglWindow:RenderSim-DN", vrDoNothing, 0);
#endif
	info->RenderTransform = vrCallbackCreateNamed("WglWindow:Transform-Def", _WGLRenderTransform, 0);
	info->Swap            = vrCallbackCreateNamed("WglWindow:Swap-Def", _WGLSwapFunc, 1, info);
	info->PrintAux        = vrCallbackCreateNamed("WglWindow:PrintAux-Def", vrDoNothing, 0);
}

