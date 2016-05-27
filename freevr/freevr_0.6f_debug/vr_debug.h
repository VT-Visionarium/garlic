/* ======================================================================
 *
 * HH   HH         vr_debug.h
 * HH   HH         Author(s): Ed Peters, Bill Sherman
 * HHHHHHH         Created: June 4, 1998
 * HH   HH         Last Modified: February 18, 2005
 * HH   HH
 *
 * Header file for FreeVR debugging and error output.  It seems
 * convenient to be able to distinguish between debugging and error
 * messages, so that debugging messages could be turned off without
 * having to turn off error messages.  For the most part, they will
 * both be printed on stderr.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#ifndef __VRDEBUG_H__
#define __VRDEBUG_H__

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif


/*** These defines are for modifying the color/mode ***/
/***   of vt100 (ie. ansi) text output.             ***/
#define	NORM_TEXT	"[0m"
#define	BOLD_TEXT	"[1m"
#define	RED_TEXT	"[31m"
#define	SET_TEXT	"[%dm"


/*** These defines are for setting the debug-level of     ***/
/****  various bits of information helpful for debugging. ***/

#define	ALWAYS_DBGLVL		   1
#define	AALWAYS_DBGLVL		   2
#define	COMMON_DBGLVL		  40
#define	DEFAULT_DBGLVL		  50
#define	SELDOM_DBGLVL		 100
#define	RARE_DBGLVL		 200
#define	ALMOSTNEVER_DBGLVL	1000

/* do by default */
#define	CONFIG_ERROR_DBGLVL	   3
#define	CONFIG_WARN_DBGLVL	  10
#define	SPAWN_DBGLVL		  20

/* less often than default */
#define PERFORMER_DBGLVL	  60
#define PERFORMER_DETAIL_DBGLVL	  60 + 500
#define GLX_DBGLVL		  60
#define	VARIABLE_DBGLVL		  65
#define	PARSE_DBGLVL		  70
#define PARSE_DETAIL_DBGLVL	 190
#define SELFCTRL_DBGLVL		 105
#define	TRACE_DBGLVL		 125

/* less often than seldom */
#define	MAG_DBGLVL		 150
#define	TRACKD_DBGLVL		 151
#define	STATIC_DBGLVL		 152 + 500  /* gives minutiae that should be more rare than rare */
#define	XWIN_DBGLVL		 153
#define	PINCHG_DBGLVL		 154
#define	ASCFOB_DBGLVL		 155
#define	FASTRK_DBGLVL		 156
#define	SHMEMD_DBGLVL		 157
#define	VRUIDD_DBGLVL		 158
#define	VRPN_DBGLVL		 159
#define	INPUT_DBGLVL		 180
#define	CONFIG_DBGLVL		 185
#define	CALLBACK_DBGLVL		 195
#define	CALLBACK_DETAIL_DBGLVL	 195 + 500
#define	BARRIER_DBGLVL		 196

/* less often than rare */
#define SERIAL_DBGLVL		 250
#define	MATH_DBGLVL		 260
#define OBJSEARCH_DBGLVL	 270
#define TRACKMEM_DBGLVL		 296
#define TRACKLOCKS_DBGLVL	 297


/* This structure is really a generic struct to link strings with */
/*   integer values, used here for debug names and debug level    */
/*   values, but it could be used elsewhere too.                  */
typedef	struct {
		int	code;		/* the debug value */
		char	*string;	/* the debug value's name */
	} vrCodeString;

#if defined(TEST_APP) || defined(CAVE)
/*=============================================================*/
/* Some defines specifically for compiling the test version of */
/*   some input devices -- mostly to keep the compiler happy.  */
#  define	vrDbgDo(n)	0
#  define	vrPrintf	printf
#  define	vrDbgPrintf	printf
#  define	vrErrPrintf	printf
#  define	vrFprintf	fprintf
#  define	vrAtoI		atoi
#  ifdef __linux
#    define	vrDbgPrintfN(...)/* print nothing */;
#  else
#    define	vrDbgPrintfN	/* print nothing */;
#  endif
#endif

#if !defined(vrPrintf)		/* ie. compiling something other than FreeVR */
#  define real_printf printf
#  define printf vrPrintf
void vrPrintf(char *fmt,...);
#endif
#if !defined(vrFprintf)		/* ie. compiling something other than FreeVR */
#  define real_fprintf fprintf
#  define fprintf vrFprintf
void vrFprintf(FILE *fp, char *fmt,...);
#endif
#if !defined(vrErrPrintf)
	void	vrErrPrintf(char *fmt,...);
#endif


#if !defined(vrDbgDo)
int	vrDbgDo(int debug_level);
#endif

#define vrTrace(proc, msg) \
		vrDbgPrintfN(TRACE_DBGLVL, "(%s::%d) %s -> %s\n", __FILE__, __LINE__, proc, msg)

#define vrDbg(msg) \
		vrDbgPrintfN(DEFAULT_DBGLVL, "(%s:%d) " BOLD_TEXT "%s\n" NORM_TEXT, __FILE__, __LINE__, msg)
#define vrErr(msg) vrErrPrintf("(%s:%d) " RED_TEXT "%s\n" NORM_TEXT, __FILE__, __LINE__, msg)

#if !defined(vrDbgPrintfN)
void	vrDbgPrintfN(int debug_level, char *fmt,...);
#endif
#if !defined(vrDbgPrintf)
void	vrDbgPrintf(char *fmt,...);
#endif
void	vrMsgPrintf(char *fmt,...);
void	vrDbgFlush();
void	vrNormPrintf(char *fmt,...);

void	vrDbgInfo();
int	vrDbgStringValue(char *debug_name);

void	vrFatalError(const char *msg);

char	*vrSigName(int);


#ifdef __cplusplus
}
#endif

#endif

