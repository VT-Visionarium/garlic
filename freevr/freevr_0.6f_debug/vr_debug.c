/* ======================================================================
 *
 *  CCCCC          vr_debug.c
 * CC   CC         Author(s): Ed Peters, Bill Sherman
 * CC              Created: June 4, 1998
 * CC   CC         Last Modified: March 14, 2006
 *  CCCCC
 *
 * Code file for FreeVR message output.
 *
 * NOTE: The fact that there are lots of checks that go one before
 *   each piece of text is printed is not a concern for real-time
 *   execution, because real-time applications shouldn't be printing
 *   much text to the terminal anyway.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>		/* for strlen() declaration */
#include <stdio.h>

#include <unistd.h>
#include <limits.h>
#include <signal.h>

#include "vr_debug.h"
#include "vr_system.h"
#include "vr_config.h"

#include "vr_shmem.h"
#include "vr_procs.h"

/* Here we need to use the real (f)printf() functions */
#undef	printf
#undef	fprintf

#undef	PRINT_WHICH_PROC
#undef	PRINT_LOCK

/* whether to default to printing stuff before shmem created */
#define	DEFAULT_NOPRINT


static vrCodeString _DebugCodes[] = {
			{ 0,				"debug:none" },
			{ ALWAYS_DBGLVL,		"debug:always" },
			{ AALWAYS_DBGLVL,		"debug:aalways" },
			{ COMMON_DBGLVL,		"debug:common" },
			{ 23,				"debug:all" },
			{ 23,				"debug:jordan" },
			{ DEFAULT_DBGLVL,		"debug:default" },
			{ SELDOM_DBGLVL,		"debug:seldom" },
			{ RARE_DBGLVL,			"debug:rare" },
			{ ALMOSTNEVER_DBGLVL,		"debug:almostnever" },

			{ CONFIG_ERROR_DBGLVL,		"debug:configerror" },
			{ CONFIG_WARN_DBGLVL,		"debug:configwarn" },
			{ SPAWN_DBGLVL,			"debug:spawn" },
			{ SPAWN_DBGLVL,			"debug:fork" },

			{ PERFORMER_DBGLVL,		"debug:performer" },
			{ PERFORMER_DETAIL_DBGLVL,	"debug:performerdetail" },
			{ GLX_DBGLVL,			"debug:glx" },
			{ VARIABLE_DBGLVL,		"debug:variable" },
			{ PARSE_DBGLVL,			"debug:parse" },
			{ PARSE_DETAIL_DBGLVL,		"debug:parsedetail" },
			{ SELFCTRL_DBGLVL,		"debug:selfcontrol" },
			{ TRACE_DBGLVL,			"debug:trace" },

			{ MAG_DBGLVL,			"debug:mag" },
			{ TRACKD_DBGLVL,		"debug:trackd" },
			{ TRACKD_DBGLVL,		"debug:shmem" },
			{ STATIC_DBGLVL,		"debug:static" },
			{ XWIN_DBGLVL,			"debug:xwin" },
			{ PINCHG_DBGLVL,		"debug:pinchg" },
			{ ASCFOB_DBGLVL,		"debug:ascfob" },
			{ FASTRK_DBGLVL,		"debug:fastrk" },
			{ SHMEMD_DBGLVL,		"debug:shmemd" },
			{ VRUIDD_DBGLVL,		"debug:vruidd" },
			{ VRPN_DBGLVL,			"debug:vrpn" },

			{ INPUT_DBGLVL,			"debug:input" },
			{ CONFIG_DBGLVL,		"debug:config" },
			{ CALLBACK_DBGLVL,		"debug:callback" },
			{ CALLBACK_DETAIL_DBGLVL,	"debug:callbackdetail" },
			{ BARRIER_DBGLVL,		"debug:barrier" },

			{ SERIAL_DBGLVL,		"debug:serial" },
			{ MATH_DBGLVL,			"debug:math" },
			{ OBJSEARCH_DBGLVL,		"debug:objsearch" },
			{ TRACKMEM_DBGLVL,		"debug:trackmem" },
			{ TRACKLOCKS_DBGLVL,		"debug:tracklocks" },
	};



/***************************************************************/
/* vrDbgDo(): Returns a boolean indicator of whether an action */
/*   should be taken based on the given debug level of the     */
/*   action, and the debug level of the process, and the       */
/*   overall debug level.                                      */
/***************************************************************/
int vrDbgDo(int action_debug_level)
{
	vrConfigInfo	*config;

	/* Return without printing anything if the debug_level of this    */
	/*   print statement is higher than both the overall debug level, */
	/*   and the process debug level (if the process memory has been  */
	/*   allocated).  Always return without printing when one of the  */
	/*   print-level selectors is zero (ie. even if negative value is */
	/*   used as the print-debug-level.                               */

	/* Of course, since #23 can do everything, setting a process, or  */
	/*   the overall debug_level to 23 will print/do everything.      */

	/* Also, an additional config setting allows exactly one other    */
	/*   debug value above the debug_levels to be printed/done (on a  */
	/*   global or process-specific level of control.                 */

	/* More stringent machines don't like dereferencing NULL pointers */
	if (vrContext == NULL)
#ifdef DEFAULT_NOPRINT
		/* default to print nothing until vrContext is allocated */
		return 0;
#else
		/* default to print stuff until vrContext is allocated */
		return 1;
#endif

	config = vrContext->config;
	if (config == NULL)
#ifdef DEFAULT_NOPRINT
		/* default to print nothing until config is allocated */
		return 0;
#else
		/* default to print stuff until config is allocated */
		return 1;
#endif


	if (vrThisProc != NULL) {
		if (config->defaults.debug_level == 23 || vrThisProc->settings.debug_level == 23)
			return 1;
		if (action_debug_level == config->defaults.debug_exact)
			return 1;
		if (action_debug_level == vrThisProc->settings.debug_exact)
			return 1;
		if ((action_debug_level > config->defaults.debug_level) &&
		    (action_debug_level > vrThisProc->settings.debug_level))
			return 0;
		if (config->defaults.debug_level == 0 || vrThisProc->settings.debug_level == 0)
			return 0;
	} else {
		if (config->defaults.debug_level == 23)
			return 1;
		if (action_debug_level > config->defaults.debug_level || config->defaults.debug_level == 0)
			return 0;
	}

	return 1;
}


/********************************************************************/
void vrDbgPrintfN(int debug_level, char *fmt, ...)
{
	va_list ap;
	FILE	*outfile = stderr;
	char	*tracemsg;

	/* First check to make sure FreeVR memory exists */
	if (vrContext == NULL || (vrShmemArena() == NULL && USE_SHMEM)) {
#if 0 /* Once the library is well debugged, we don't need these early print statements -- not sure whether that's true as of now though [07/06/2006] */
		/* If not, then use normal print routine */
		va_start(ap, fmt);
		vfprintf(outfile, fmt, ap);
		va_end(ap);
#endif
		return;
	}

	/* if this message is at the tracing debug level, then store it in the process struct */
	if (debug_level == TRACE_DBGLVL && vrThisProc != NULL) {
		vrThisProc->tracemsgcnt++;
		vrThisProc->tracemsgcnt %= VRPROC_NUMTRACEMSGS;
		tracemsg = vrThisProc->tracemsg[vrThisProc->tracemsgcnt];
		va_start(ap, fmt);
		vsnprintf(tracemsg, sizeof(vrThisProc->tracemsg[0]), fmt, ap);
		va_end(ap);

		/* remove the trailing carriage return, if one */
		if (tracemsg[strlen(tracemsg)-1] == '\n')
			tracemsg[strlen(tracemsg)-1] = '\0';

		/* set the time for this message */
		/* NOTE: we can either set the time based on the current frame time, or the  */
		/*   current wall time.  The latter means an additional system call, but the */
		/*   former is courser.                                                      */
		vrThisProc->tracetime[vrThisProc->tracemsgcnt] = vrThisProc->frame_wtime;
	}

	/* if we're not at the correct debug level, then print nothing */
	if (!vrDbgDo(debug_level))
		return;

#ifdef PRINT_LOCK
	if (vrContext == NULL)
#  ifdef DEFAULT_NOPRINT
		printf("yo: no context yet ...");
#  else
		;
#  endif

	else	if (vrContext->print_lock != NULL)
			vrLockWriteSet(vrContext->print_lock);
#endif

#ifdef PRINT_WHICH_PROC
	fprintf(stderr, RED_TEXT "Process #%d (at %p)\n" NORM_TEXT, vrThisProc->id, vrThisProc);
#endif

	if (vrThisProc != NULL) {
		if (vrThisProc->print_file != NULL)
			outfile = vrThisProc->print_file;
		if (vrThisProc->print_color >= 0)
			fprintf(outfile, SET_TEXT, vrThisProc->print_color);
		fprintf(outfile, vrThisProc->print_string);
	}

	va_start(ap, fmt);
	vfprintf(outfile, fmt, ap);
	va_end(ap);

	if (vrThisProc != NULL) {
		if (vrThisProc->print_color >= 0)
			fprintf(outfile, NORM_TEXT);
	}

#ifdef VRDELAY_DBG
	vrSleep(250000);
#endif

#ifdef PRINT_LOCK
	if (vrContext == NULL)
#  ifdef DEFAULT_NOPRINT
		printf("oy\n");
#  else
		;
#  endif
	else	if (vrContext->print_lock != NULL)
			vrLockWriteRelease(vrContext->print_lock);
#endif

}


/********************************************************************/
/* If I could pass variable arguments from this function to */
/*   vrDbgPrintfN(), this function would be a lot simpler.  */
void vrDbgPrintf(char *fmt,...)
{
	va_list ap;
	FILE	*outfile = stderr;

	/* First check to make sure FreeVR memory exists */
	if (vrContext == NULL || (vrShmemArena() == NULL && USE_SHMEM)) {
		/* If not, then use normal print routine */
		va_start(ap, fmt);
		vfprintf(outfile, fmt, ap);
		va_end(ap);
		return;
	}

	if (!vrDbgDo(DEFAULT_DBGLVL))
		return;

#ifdef PRINT_LOCK
	if (vrContext == NULL)
#  ifdef DEFAULT_NOPRINT
		printf("yo: no context yet ...");
#  else
		;
#  endif
	else	if (vrContext->print_lock != NULL)
			vrLockWriteSet(vrContext->print_lock);
#endif

#ifdef PRINT_WHICH_PROC
	fprintf(stderr, RED_TEXT "Process #%d (at %p)\n" NORM_TEXT, vrThisProc->id, vrThisProc);
#endif

	if (vrThisProc != NULL) {
		if (vrThisProc->print_file != NULL)
			outfile = vrThisProc->print_file;
		if (vrThisProc->print_color >= 0)
			fprintf(outfile, SET_TEXT, vrThisProc->print_color);
		fprintf(outfile, vrThisProc->print_string);
	}

	va_start(ap, fmt);
	vfprintf(outfile, fmt, ap);
	va_end(ap);

	if (vrThisProc != NULL) {
		if (vrThisProc->print_color >= 0)
			fprintf(outfile, NORM_TEXT);
	}

#ifdef VRDELAY_DBG
	vrSleep(250000);
#endif

#ifdef PRINT_LOCK
	if (vrContext == NULL)
#  ifdef DEFAULT_NOPRINT
		printf("oy\n");
#  else
		;
#  endif
	else	if (vrContext->print_lock != NULL)
			vrLockWriteRelease(vrContext->print_lock);
#endif

}


/********************************************************************/
/* The difference between vrErrPrintf & vrDbgPrintf[N] is that the */
/*   "Err" messages will always be printed, and the "Dbg" messages */
/*   can be selectively printed.                                   */
void vrErrPrintf(char *fmt,...)
{
	va_list ap;
	FILE	*outfile = stderr;

	/* First check to make sure FreeVR memory exists */
	if (vrContext == NULL || (vrShmemArena() == NULL && USE_SHMEM)) {
		/* If not, then use normal print routine */
		va_start(ap, fmt);
		vfprintf(outfile, fmt, ap);
		va_end(ap);
		return;
	}

#ifdef PRINT_LOCK
	if (vrContext == NULL)
#  ifdef DEFAULT_NOPRINT
		printf("yo: no context yet ...");
#  else
		;
#  endif
	else	if (vrContext->print_lock != NULL)
			vrLockWriteSet(vrContext->print_lock);
#endif

#ifdef PRINT_WHICH_PROC
	fprintf(stderr, RED_TEXT "Process #%d (at %p)\n" NORM_TEXT, vrThisProc->id, vrThisProc);
#endif

	if (vrThisProc != NULL) {
		if (vrThisProc->print_file != NULL)
			outfile = vrThisProc->print_file;
		if (vrThisProc->print_color >= 0)
			fprintf(outfile, SET_TEXT, vrThisProc->print_color);
		fprintf(outfile, vrThisProc->print_string);
	}

	va_start(ap, fmt);
	vfprintf(outfile, fmt, ap);
	va_end(ap);

	if (vrThisProc != NULL) {
		if (vrThisProc->print_color >= 0)
			fprintf(outfile, NORM_TEXT);
	}

#ifdef PRINT_LOCK
	if (vrContext == NULL)
#  ifdef DEFAULT_NOPRINT
		printf("oy\n");
#  else
		;
#  endif
	else	if (vrContext->print_lock != NULL)
			vrLockWriteRelease(vrContext->print_lock);
#endif

}


/********************************************************************/
/* vrMsgPrintf(): is basically the same as vrErrPrintf(), but */
/*   messages go to stdout by default rather than stderr.     */
/*   And they are not printed if debug level set to zero.     */
void vrMsgPrintf(char *fmt,...)
{
	va_list ap;
	FILE	*outfile = stderr;

	/* First check to make sure FreeVR memory exists */
	if (vrContext == NULL || (vrShmemArena() == NULL && USE_SHMEM)) {
		/* If not, then use normal print routine */
		va_start(ap, fmt);
		vfprintf(outfile, fmt, ap);
		va_end(ap);
		return;
	}

	if (!vrDbgDo(AALWAYS_DBGLVL))
		return;

#ifdef PRINT_LOCK
	if (vrContext == NULL)
#  ifdef DEFAULT_NOPRINT
		printf("yo: no context yet ...");
#  else
		;
#  endif
	else	if (vrContext->print_lock != NULL)
			vrLockWriteSet(vrContext->print_lock);
#endif

#ifdef PRINT_WHICH_PROC
	fprintf(stderr, RED_TEXT "Process #%d (at %p)\n" NORM_TEXT, vrThisProc->id, vrThisProc);
#endif

	if (vrThisProc != NULL) {
		if (vrThisProc->print_file != NULL)
			outfile = vrThisProc->print_file;
		if (vrThisProc->print_color >= 0)
			fprintf(outfile, SET_TEXT, vrThisProc->print_color);
		fprintf(outfile, vrThisProc->print_string);
	}

	va_start(ap, fmt);
	vfprintf(outfile, fmt, ap);
	va_end(ap);

	if (vrThisProc != NULL) {
		if (vrThisProc->print_color >= 0)
			fprintf(outfile, NORM_TEXT);
	}

#ifdef PRINT_LOCK
	if (vrContext == NULL)
#  ifdef DEFAULT_NOPRINT
		printf("oy\n");
#  else
		;
#  endif
	else	if (vrContext->print_lock != NULL)
			vrLockWriteRelease(vrContext->print_lock);
#endif

}


/********************************************************************/
void vrDbgFlush()
{
	FILE	*outfile = stderr;

	/* First check to make sure FreeVR memory exists */
	if (vrContext == NULL || (vrShmemArena() == NULL && USE_SHMEM)) {
		/* If not, then use normal print routine */
		fflush(outfile);
		return;
	}

	if (vrThisProc != NULL) {
		if (vrThisProc->print_file != NULL)
			outfile = vrThisProc->print_file;
	}

	fflush(outfile);
}


/********************************************************************/
void vrPrintf(char *fmt,...)
{
	va_list ap;
	FILE	*outfile = stderr;

	/* First check to make sure FreeVR memory exists */
	if (vrContext == NULL || (vrShmemArena() == NULL && USE_SHMEM)) {
		/* If not, then use normal print routine */
		va_start(ap, fmt);
		vfprintf(outfile, fmt, ap);
		va_end(ap);
		return;
	}

#ifdef PRINT_LOCK
	if (vrContext == NULL)
#  ifdef DEFAULT_NOPRINT
		printf("yo: no context yet ...");
#  else
		;
#  endif
	else	if (vrContext->print_lock != NULL)
			vrLockWriteSet(vrContext->print_lock);
#endif

#ifdef PRINT_WHICH_PROC
	fprintf(stderr, RED_TEXT "Process #%d (at %p)\n" NORM_TEXT, vrThisProc->id, vrThisProc);
#endif

	if (vrThisProc != NULL) {
		if (vrThisProc->print_file != NULL)
			outfile = vrThisProc->print_file;
		if (vrThisProc->print_color >= 0)
			fprintf(outfile, SET_TEXT, vrThisProc->print_color);
		fprintf(outfile, vrThisProc->print_string);
	}

	va_start(ap, fmt);
	vfprintf(outfile, fmt, ap);
	va_end(ap);

	if (vrThisProc != NULL) {
		if (vrThisProc->print_color >= 0)
			fprintf(outfile, NORM_TEXT);
	}

#ifdef PRINT_LOCK
	if (vrContext == NULL)
#  ifdef DEFAULT_NOPRINT
		printf("oy\n");
#  else
		;
#  endif
	else	if (vrContext->print_lock != NULL)
			vrLockWriteRelease(vrContext->print_lock);
#endif

}


/********************************************************************/
void vrFprintf(FILE *fp, char *fmt,...)
{
	va_list ap;
	FILE	*outfile = fp;

	/* First check to make sure FreeVR memory exists */
	if (vrContext == NULL || (vrShmemArena() == NULL && USE_SHMEM)) {
		/* If not, then use normal print routine */
		va_start(ap, fmt);
		vfprintf(outfile, fmt, ap);
		va_end(ap);
		return;
	}

#ifdef PRINT_LOCK
	if (vrContext == NULL)
#  ifdef DEFAULT_NOPRINT
		printf("yo: no context yet ...");
#  else
		;
#  endif
	else	if (vrContext->print_lock != NULL)
			vrLockWriteSet(vrContext->print_lock);
#endif

#ifdef PRINT_WHICH_PROC
	fprintf(stderr, RED_TEXT "Process #%d (at %p)\n" NORM_TEXT, vrThisProc->id, vrThisProc);
#endif

	/* don't print extra color/info stuff when proc not initialized or printing to a file */
	if (vrThisProc != NULL && (fp == stdout || fp == stderr)) {
		if (vrThisProc->print_file != NULL && (outfile == stdout || outfile == stderr))
			outfile = vrThisProc->print_file;
		if (vrThisProc->print_color >= 0)
			fprintf(outfile, SET_TEXT, vrThisProc->print_color);
		fprintf(outfile, vrThisProc->print_string);
	}

	va_start(ap, fmt);
	vfprintf(outfile, fmt, ap);
	va_end(ap);

	/* don't print extra color/info stuff when proc not initialized or printing to a file */
	if (vrThisProc != NULL && (fp == stdout || fp == stderr)) {
		if (vrThisProc->print_color >= 0)
			fprintf(outfile, NORM_TEXT);
	}

#ifdef PRINT_LOCK
	if (vrContext == NULL)
#  ifdef DEFAULT_NOPRINT
		printf("oy\n");
#  else
		;
#  endif
	else	if (vrContext->print_lock != NULL)
			vrLockWriteRelease(vrContext->print_lock);
#endif

}


/********************************************************************/
/* a Version of printf() that doesn't use any FreeVR variables */
/* NOTE: this is here because normally printf and fprintf are  */
/*    #define'd to use the FreeVR equivalents.                 */
void vrNormPrintf(char *fmt,...)
{
	va_list ap;
	FILE	*outfile = stderr;

	va_start(ap, fmt);
	vfprintf(outfile, fmt, ap);
	va_end(ap);
}


/********************************************************************/
void vrDbgInfo()
{
	vrPrintf("yo, test of debug print routine\n");
}


/********************************************************************/
int vrDbgStringValue(char *debug_name)
{
	int	count;
	int	num_codes =  (sizeof(_DebugCodes) / sizeof(vrCodeString));

	for (count = 0; count < num_codes; count++) {
		if (!strcasecmp(debug_name, _DebugCodes[count].string)) {
			return _DebugCodes[count].code;
		}
	}

	return -1;	/* no match */
}


/****************************************************************************/
void vrFatalError(const char *msg)
{
	vrErr(msg);
	vrErr("FATAL ERROR: Process is exiting now.\n");
	abort();
}


/********************************************************************/
/** TODO: is this really the proper file for the following? **/
/*
 *
 * ugly but functional
 *
 */
static char *sigint = "SIGINT";
static char *sighup = "SIGHUP";
static char *sigbus = "SIGBUS";
static char *sigsegv = "SIGSEGV";
static char *sigpipe = "SIGPIPE";
static char *othersig = "unknown signal";

char *vrSigName(int which)
{
	switch (which) {
	case SIGINT: return sigint;
	case SIGHUP: return sighup;
	case SIGBUS: return sigbus;
	case SIGSEGV: return sigsegv;
	case SIGPIPE: return sigpipe;
	default: return othersig;
	}
}

