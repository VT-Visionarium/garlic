/* ======================================================================
 *
 *  CCCCC          vr_shmem.c
 * CC   CC         Author(s): Ed Peters, Bill Sherman, John Stone, Sukru Tikves (SEM_TCP stuff)
 * CC              Created: June 4, 1998
 * CC   CC         Last Modified: December 2, 2013
 *  CCCCC
 *
 * Code file for FreeVR shared memory.  Defines routines to initialize
 * and allocated from shared memory.  The pointers to the shared memory
 * arena are stored in static (file-scope) variables declared in this
 * file.  The file also contains definitions of the locking and barrier
 * routines, as well as the "private" locking and barrier structures.
 *
 * The code for initializing IRIX shared memory was mostly lifted from
 * Vijay Jaswal's cxMem.{h, C}, and Bill Sherman's shared memory code.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/*************************************************************************

TODO:
	- do more error checking and reporting using the errno facility.

	- figure out how to determine the number of processes waiting on
		write & read locks for Linux/POSIX implementation
		[see vrLockStatus() for one case.]

*************************************************************************/


/*****************************************************************/
/* Some options to how the shared memory code should be compiled */
/*   (mostly options for use in debugging the library.)          */
#undef	VRDBXPAUSE_ON_ERROR	/* define to use debugger to analyze why no memory */
#define	VRTRACKMEM		/* define this to keep track of memory allocations */
#define	VRSHMEMINITDBG		/* define this to print debugging statements during initialization (ie. before Context is set) */
#define	VRTRACKLOCKS		/* define this to keep track of locking operations  -- DO NOT USE w/ vr_debug.c:PRINT_LOCK */
#undef	VRSHMEM_USESTRUCT	/* define this to keep all the shared mem info in a struct -- NYI */

#define VRSHMEM_ARENANAME "/tmp/freevr.arena"

#define	VRTRACE_SHMEM	/* define this to enable tracing within vrShmem...() calls */
#undef	VRTRACE_LOCK	/* define this to enable tracing within vrLock...() calls */
#define	VRTRACE_BARRIER	/* define this to enable tracing within vrBarrier...() calls */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>		/* needed for memset(), strlen() & strcpy() */
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#if defined(__sgi)
#  include <malloc.h> /* needed for amalloc/afree etc -- TODO: only include when amalloc/afree, etc. are needed */
#endif
#if 0 /* 07/06/2006: I'm not sure we ended up needing this */
#if defined(GFX_PERFORMER)
#  include <amalloc.h>
#endif
#endif



		/*********************************************/
		/*** Shared Memory structure and functions ***/
		/*********************************************/

#ifdef VRTRACE_SHMEM
#  define vrTraceOpt		vrTrace
#else
#  define vrTraceOpt(a,b)	;
#endif

#ifdef SHM_PF_ARENA /* use Performer's memory arena { */
#  include <Performer/pf.h>		/* needed for pfGetSharedArena() */
#  define PERFORMER_2_3_BUG  0		/* version 2.3 of Linux-Performer doesn't have pfGetSharedArena() */
#  if (PERFORMER_2_3_BUG == 1) || 1	/* Linux on SGI's Prism doesn't have working amalloc, etc */
#    define amalloc			pfMalloc
#    define afree(ptr, ar)		pfFree((ptr))
#    define arealloc(mem, bytes, ar)	pfRealloc((mem), (bytes))
#  endif
#else
#  define PERFORMER_2_3_BUG  0		/* When not using Performer, then we're not using pfGetSharedArena() */
#endif /* } */

#if defined(SHM_SYSVIPC)
#  include <sys/ipc.h>
#  include <sys/shm.h>
/* perhaps also #include <sys/sem.h>, but probably not -- the only time SHM_SYSVIPC is defined w/o SEM_SYSVIPC is Linux w/ pf2.3 */
#endif

#if defined(SEM_SYSVIPC)
#  include <sys/ipc.h>
#  include <sys/shm.h>
#  include <sys/sem.h>
#  if defined(__linux)
#    define SEMMSL 250		/* this is #defined in linux/sem.h, but including that causes compilation problems */
#  elif defined(__sgi)
#    define SEMMSL 100		/* this value can be found in "/var/sysgen/mtune/sem", but not in a header file */
#  else
#    define SEMMSL 1		/* "1" by default -- that should never be a problem */
#  endif
#endif

#if defined(SEM_IRIX)
#  include <ulocks.h>
#endif

#if defined(SEM_POSIX)
#  include <semaphore.h>
#endif

#if defined(SEM_WIN32)
#  include <windows.h>
#endif

#if defined(SEM_TCP)
#  include "vr_sem_tcp.h"
#endif

#include "vr_shmem.h"
#include "vr_debug.h"
#include "vr_context.h"

#if defined(SHM_DUMMY) /* { */
   /* Stuart's simple wrappers to implement a dumb, but */
   /*   quick-to-write shared memory allocation scheme. */
   /*   (He wrote it in about 15 minutes.)              */

#include "vr_math.h"

#  include <sys/mman.h>
#  ifndef SHM_PF_ARENA
     typedef void	usptr_t;
     typedef void	usema_t;
#    include "vr_shmem.dummy.c"		/* Kids, don't look -- this is not kosher C programming */
#  endif /* !SHM_PF_ARENA */
#  define MEM_SHARED 0x0001
#endif /* } SHM_DUMMY */

/******************************************************************/
/* File-scope variables for keeping track of the shared memory    */
/* arena and whatnot.  It would be nice to have them in a struct. */
#ifdef VRTRACKMEM
#  define TRACK_SPACER	"                                                       "
#  ifndef VRSHMEM_USESTRUCT
	static	long		total_bytes_this_proc = 0;
	static	long		*total_bytes_overall = NULL;
	static	long		*total_freed_overall = NULL;
#  endif
#endif
#ifndef VRSHMEM_USESTRUCT
	static	long		total_bytes_too_many = 0;
	static	long		*arena_size = NULL;
#endif

#ifndef VRSHMEM_USESTRUCT /* { */
#  if !defined(SHM_PF_ARENA) && USE_SHMEM /* { */
#    if defined(SHM_SVR4MMAP)
	static	long		shmem_fd = -1;
#    endif

	static	void		*shmem_addr = NULL;
	static	usptr_t		*usarena = NULL;
#  endif /* } !SHM_PF_ARENA  && USE_SHMEM */
	static	void		*arena = NULL;

#  if defined(SHM_SYSVIPC)
	static	int		shmid = 0;
	static	int		shmkey = 0;
	static	struct shmid_ds	shminfo;
#  endif

#if 0
#  if defined(SEM_SYSVIPC)
	static	int		sysvsem_current_semset;		/* initialize to NULL */
	static	int		sysvsem_next_semnum;		/* initialize to  0 */
#  endif
#endif
#endif /* } !VRSHMEM_USESTRUCT */

#ifdef VRSHMEM_USESTRUCT
  static	vrShmemInfo	shmem_info;
#endif


/***************************************************/
/* NOTE: currently the argument shmeminfo is a dummy placeholder for */
/*   a future time when we might keep all the info in a structure    */
void vrFprintShmemInfo(FILE *file, void *shmeminfo, vrPrintStyle style)
{
	switch (style) {
	case brief:
	case one_line:
		/* TODO: not yet implemented -- very brief output */
	case machine:
		/* TODO: not yet implemented -- output easily machine parsed */
		vrFprintf(file, "TODO: put special version of shared-memory info here\n");
		break;

	default:
	case file_format:	/* actually, this doesn't make sense, but we should print something */
	verbose:
		vrFprintf(file, "{\n");
#ifdef VRTRACKMEM
		vrFprintf(file, "\r"
			"\tmemory size = %d\n\tused memory = %d\n\tfreed memory = %d\n",
			*arena_size,
			vrShmemUsage(),
			vrShmemFreed());
#else
		vrFprintf(file, "\rvr_shmem.c must be compiled with the VRTRACKMEM option to get memory usage values\n");
#endif
		vrFprintf(file, "}\n");
	}
}


/*********************************************************************/
/* set up the shared memory arena which will be shareable by         */
/*   processes forked from this one                                  */
/* returns the size of the arena, which is especially useful if this */
/*   isn't the first call the vrShmemInit(), and thus the requested  */
/*   size won't match the actual size.                               */
long vrShmemInit(size_t bytes)
{
#ifndef SHM_PF_ARENA
	size_t	request_size;		/* used to adjust the bytes specified to be on a page boundary */
#endif

#ifdef VRSHMEMINITDBG
	vrTraceOpt("vrShmemInit", "beginning");
#endif

#if USE_SHMEM
	if (arena) {
		vrErrPrintf("vrShmemInit(): " RED_TEXT "Warning: shared memory already initialized (%d bytes), additional attempt ignored.\n" NORM_TEXT, *arena_size);
		return *arena_size;
	}
#elif 0	/* We only need to include this when debugging the library, so not included for now 07/06/2006 */
	vrErrPrintf("vrShmemInit(): " RED_TEXT "Info: Pthread version, no need for shared memory.\n" NORM_TEXT);
#endif

	/******************************************************/
	/* print some warnings on systems with known problems */
#if defined(SEM_POSIX) && !defined(_POSIX_THREAD_PROCESS_SHARED) && !defined(MP_PTHREADS) && !defined(MP_PTHREADS2) /* TODO: need to verify this is not broken when actually using pthreads -- it seems things are fine. */
	/* NOTE: Actually, on my FC-2 Linux system, locks seem to work just  */
	/*   fine, even when not using pthreads, but using POSIX semaphores. */
	vrMsgPrintf("FreeVR: " RED_TEXT
			"WARNING, it is possible that POSIX threads are not shared across processes.\n"
		"        on this system.  If so, features such as locking operations will not work.\n"
		"        You may need to compile with a different choice as the method of semaphore\n"
		"        implementation.  See the library development guide and the Make-arch file\n"
		"        for details.  Anecdotally, this failure may occur once every 10 or 20 runs.\n" NORM_TEXT);
		/* See diary entries on 3/9/25 and 3/25/05. */
//	vrSleep(2000000);
#endif
#if defined(SEM_SYSVIPC)    && 0 /* I think these are working [3/25/05] */
	vrMsgPrintf("FreeVR: " RED_TEXT
			"WARNING, barriers based on SYS-V IPC semaphores are not fully functional.\n" NORM_TEXT);
	vrSleep(2000000);
#endif
	/* set the initial barrier values */

        /*********************************************************/
        /* create the address space for amalloc to allocate from */

	arena = NULL;

#if USE_SHMEM /* { */

/***********************/
#ifdef SHM_PF_ARENA /* use Performer's memory arena { */
	/* NOTE: If a non-default arena size is desired, vrShmemInit() must be */
	/*   called before pfInit() -- otherwise the call to pfInitArenas()    */
	/*   will already have been made.                                      */
	pfSharedArenaSize(bytes);
	pfInitArenas();

	arena = pfGetSharedArena();

	/**************************************************************/
	/* The first thing in the arena will be the size of the arena */
	arena_size = (long *)pfMalloc(sizeof(long), arena);
	*arena_size = pfGetSharedArenaSize();

	vrNormPrintf("FreeVR: Just allocated an arena (%p) from Performer of size %ld\n", arena, *arena_size);

	if (arena == NULL) {
		vrNormPrintf(RED_TEXT "ERROR: if using Performer 2.3 on Linux, try recompiling library\n"
			"  with target \"linux-pf2.3\"\n" NORM_TEXT);
		exit(-1);
	}

#else /* } SHM_PF_ARENA vs. Use FreeVR Shared Memory Arena { */

        /* round request_size up to next full page of memory, required */
        /* by most mmap() shared memory implementations.               */
	request_size = ((bytes+getpagesize()-1) & ~(getpagesize()-1));

/***********************/
#if defined(SHM_SVR4MMAP)
        /* SVR4 style shared memory via mmap() of /dev/zero */
        shmem_fd = open("/dev/zero", O_RDWR );
        if (shmem_fd < 0) {
            vrErr("Couldn't open shmem file.");
            return 0;
        }
#  ifdef VRSHMEMINITDBG
        vrTraceOpt("vrShmemInit", "before mmap");
#  endif
#  if defined(__sgi)
        /* SGI's mmap lets us grow the mapped region on the fly if we like */
        shmem_addr = mmap(0, request_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_AUTOGROW | MAP_AUTORESRV, shmem_fd, 0);
#  else
        /* non-SGI mmap doesn't let us grow the mapped region on the fly */
        shmem_addr = mmap(0, request_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmem_fd, 0);
#  endif

	/* TODO: we can probably close(shmem_fd) here -- since the memory mapping has been done. */

#  if defined(__sgi)
        /* This code is only functional on IRIX, emulated everywhere else */
        /* So this initialization sequence doesn't make sense elsewhere   */
	/*****************************************/
	/* create an arena to be used by acreate */
#  ifdef VRSHMEMINITDBG
	vrTraceOpt("vrShmemInit", "before usinit");
#  endif
	usarena = usinit(VRSHMEM_ARENANAME);
	if (!usarena) {
		vrErr("Couldn't init arena.");
		return 0;
	}
#  else
        usarena = NULL; /* unused, but passed into emulation code */
#  endif

/***********************/
#elif defined(SHM_BSDANONMMAP)
        /* BSD4.4 style shared memory via mmap() and MAP_ANON */
        vrTraceOpt("vrShmemInit", "before mmap");
#  if defined(__hpux)
#     define MAP_ANON MAP_ANONYMOUS
#  endif
        shmem_addr = mmap(0, request_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);

/***********************/
#elif defined(SHM_SYSVIPC)
        /* System V style shared memory via shmget() */
#  ifdef VRSHMEMINITDBG
        vrTraceOpt("vrShmemInit", "before shmget");
#  endif

	/* setup the shared memory, using the pid as the key */
        shmkey = getpid(); /* TODO: maybe this should be configurable */
        shmid = shmget(shmkey, request_size, IPC_CREAT | SHM_R |SHM_W);
	if (shmid < 0) {
		switch (errno) {
		case ENOENT:
			/* NOTE: actually this shouldn't happen since we're creating the arena */
			vrErrPrintf("vrShmemInit(): " RED_TEXT "ERROR: no such shared memory key (%d)\n" NORM_TEXT, shmkey);
			break;
		case EINVAL:
			/* outside the bounds of possible shared memory. */
			vrErrPrintf("vrShmemInit(): " RED_TEXT "ERROR: outside memory bounds (%ld, %ld)\n" NORM_TEXT, request_size, bytes);
			break;
		case ENOMEM:
			/* not enough shared memory available */
			vrErrPrintf("vrShmemInit(): " RED_TEXT "ERROR: not enough memory available\n" NORM_TEXT);
			break;
		case ENOSPC:
			/* this only occurs when creating new keys -- not */
			/*   enough system resources for another key.     */
			vrErrPrintf("vrShmemInit(): " RED_TEXT "ERROR: no more shared memory segments allowed\n" NORM_TEXT);
			break;
		case EEXIST:
			/* a shared memory segment with that key already exists
	*/
			vrErrPrintf("vrShmemInit(): " RED_TEXT "ERROR: key %x already exists\n" NORM_TEXT, shmkey);
			break;

		}
		vrSleep(10000000);	/* 10 seconds: give the user a chance to notice we've hit a serious error */

		return 0;
	}

	/* set the permissions for the arena */
        shmctl(shmid, IPC_STAT, &shminfo);
        shminfo.shm_perm.mode = 0666;
        shmctl(shmid, IPC_SET, &shminfo);

	/* now get the shared memory address */
        shmem_addr = shmat(shmid, NULL, SHM_RND | SHM_R | SHM_W);
        if (shmem_addr == NULL || shmem_addr == (void *)-1) {
		shmctl(shmid, IPC_RMID, &shminfo);
        }
#endif

        if (shmem_addr == NULL || shmem_addr == (void *)-1) {
                vrErr("Couldn't acquire shared memory from system.");
                return 0;
        }

	/**********************************************************/
	/* Remove the arena file, so only process descending from */
	/* this one can connect to our shared memory arena.       */
#ifdef VRSHMEMINITDBG
	vrTraceOpt("vrShmemInit", "before arena file unlink");
#endif
	unlink(VRSHMEM_ARENANAME);

	/************************************/
	/* now do the actual arena creation */
#ifdef VRSHMEMINITDBG
	vrTraceOpt("vrShmemInit", "before acreate");
#endif
	arena = acreate(shmem_addr, bytes, MEM_SHARED, usarena, NULL);
	if (!arena) {
		vrErr("Couldn't create arena.");
		return 0;
	}

	/**************************************************************/
	/* The first thing in the arena will be the size of the arena */
	arena_size = (long *)amalloc(sizeof(long), arena);
	*arena_size = bytes;
#endif /* } !SHM_PF_ARENA */ /* TODO: should this come after the VRTRACKMEM stuff? */
#else /* } USE_SHMEM { */
	arena_size = (long *)malloc(sizeof(long));
	*arena_size = bytes;
#endif /* } USE_SHMEM */


#if 0
	/*************************************/
	/*** Now initialize the semaphores ***/
	/*************************************/

#if defined(SEM_SYSVIPC) /* { */
	sysvsem_current_semset = 0;	/* no current semaphore set -- one needs to be created */
	sysvsem_next_semnum = 0;	/* the first semaphore will be number 0 */
#endif /* } SEM_SYSVIPC */
#endif


	/***********************************************************/
	/*** Now allocate and store data about the shared memory ***/
	/***********************************************************/

#ifdef VRTRACKMEM
#  if USE_SHMEM
	if (arena || PERFORMER_2_3_BUG) {
		total_bytes_overall = (long *)amalloc(sizeof(long), arena);
		total_freed_overall = (long *)amalloc(sizeof(long), arena);
	} else {
		vrErr("No shmem arena to allocate from.");
	}
#  else
	total_bytes_overall = (long *)malloc(sizeof(long));
	total_freed_overall = (long *)malloc(sizeof(long));
#  endif

	if (total_bytes_overall == NULL) {
		total_bytes_too_many += sizeof(long);
		vrErrPrintf("vrShmemInit(%d): "
			RED_TEXT "No shmem allocated!  IE. out of shared memory. (%ld too much, %ld allocated)\n" NORM_TEXT,
			bytes, total_bytes_too_many, *total_bytes_overall);
#  ifdef VRDBXPAUSE_ON_ERROR
		vrErrPrintf(BOLD_TEXT "pausing, use 'dbx -p %d' to debug\n" NORM_TEXT, getpid());
		pause();
#  endif
	}

	*total_bytes_overall = 3 * sizeof(long);	/* 3 longs allocated so far */
	*total_freed_overall = 0;
	total_bytes_this_proc = *total_bytes_overall;
	vrDbgPrintfN(TRACKMEM_DBGLVL, TRACK_SPACER "(shmem) allocated %d bytes so far\r", *total_bytes_overall);
#endif  /* VRTRACKMEM */
#ifdef VRSHMEMINITDBG
	vrTraceOpt("vrShmemInit", "done");
#endif

	return *arena_size;
}


/*****************************************************************/
/*
 * Detach shared memory resources, delete any allocated shared memory
 * keys, file handles, etc so that we don't unintentionally consume
 * finite system resources (such as keys, files, etc that last beyond
 * process exit) with each run.
 */
void vrShmemExit()
{
#if defined(SHM_SVR4MMAP) || defined(SHM_BSDANONMMAP)
	int	size = *arena_size;
#endif

#  if !defined(SHM_PF_ARENA) && USE_SHMEM /* if using Performer's memory system, let it clean itself up { */
	adelete(arena);
	usdetach(usarena);
#  if defined(SHM_SVR4MMAP)
	munmap(shmem_addr, size);		/* detach shared memory          */
	close(shmem_fd);			/* close memory mapped /dev/zero */
#  elif defined(SHM_BSDANONMMAP)
	munmap(shmem_addr, size);		/* detach shared memory          */
#  elif defined(SHM_SYSVIPC)
        shmdt(shmem_addr);			/* detach shared memory          */
        shmctl(shmid, IPC_RMID, &shminfo);	/* delete shared memory key      */

	shmid = 0;
	shmkey = 0;
#    if 0 /* this isn't a pointer, so setting to NULL isn't correct */
	shminfo = NULL;
#    endif
#  endif
	shmem_addr = NULL;
	usarena = NULL;
#endif /* } !SHM_PF_ARENA */
	arena = NULL;		/* after this FreeVR will no longer be able to use shared memory */
}


/*****************************************************************/
void *vrShmemArena()
{
	return arena;
}


/*****************************************************************/
long vrShmemUsage()
{
#ifdef VRTRACKMEM
	return *total_bytes_overall;
#else
	return -1L;
#endif
}


/*****************************************************************/
long vrShmemFreed()
{
#ifdef VRTRACKMEM
	return *total_freed_overall;
#else
	return -1L;
#endif
}


/*****************************************************************/
/*
 * Once the arena has been initialized, these are the simplest
 * functions in the world (just wrappers around amalloc and afree).
 */
void *vrShmemAlloc(size_t bytes)
{
	void	*mem = NULL;

	if (bytes == 0) {
		vrDbgPrintfN(DEFAULT_DBGLVL+1, "vrShmemAlloc(): Requested allocation of 0 bytes.");
		return mem;
	}

	if (bytes < 0) {
		vrDbgPrintfN(ALWAYS_DBGLVL, "vrShmemAlloc(): " RED_TEXT "Requested allocation of negative memory block -- aborting." NORM_TEXT);
		abort();
	}

#if USE_SHMEM
	if (arena || PERFORMER_2_3_BUG)
		mem = amalloc(bytes, arena);
	else	vrErr("No shmem arena to allocate from.");
#else
	mem = malloc(bytes);
#endif

	if (mem == NULL) {
		total_bytes_too_many += bytes;
		vrErrPrintf("vrShmemAlloc(%d): "
			RED_TEXT "No shmem allocated!  IE. out of shared memory. (%ld too much, %ld allocated)\n" NORM_TEXT,
			bytes, total_bytes_too_many, *total_bytes_overall);
#ifdef VRDBXPAUSE_ON_ERROR
		vrErrPrintf(BOLD_TEXT "pausing, use 'dbx -p %d' to debug\n" NORM_TEXT, getpid());
		pause();
#endif
	}

#ifdef VRTRACKMEM
	if (mem) {
#  if USE_SHMEM && !defined(SHM_PF_ARENA)
		*total_bytes_overall += amallocblksize(mem, arena);
		total_bytes_this_proc += amallocblksize(mem, arena);
#  else
		/* we can only assume that the size of the allocated memory matches the request */
		*total_bytes_overall += bytes;
		total_bytes_this_proc += bytes;
#  endif
		vrDbgPrintfN(TRACKMEM_DBGLVL, TRACK_SPACER "(shmem) allocated %d bytes so far\r", *total_bytes_overall);
	}
#endif

	return mem;
}


/*****************************************************************/
/* NOTE: when 0 bytes are requested then the NULL pointer is returned */
void *vrShmemAlloc0(size_t bytes)
{
	void	*mem = NULL;

	if (bytes == 0) {
		vrDbgPrintfN(DEFAULT_DBGLVL+1, "vrShmemAlloc0(): Requested allocation of 0 bytes.");
		return mem;
	}

	if (bytes < 0) {
		vrDbgPrintfN(ALWAYS_DBGLVL, "vrShmemAlloc0(): " RED_TEXT "Requested allocation of negative memory block -- aborting." NORM_TEXT);
		abort();
	}

#if USE_SHMEM
	if (arena || PERFORMER_2_3_BUG) {
		mem = amalloc(bytes, arena);
	} else	vrErr("No shmem arena to allocate from.");
#else
	mem = malloc(bytes);
#endif

	if (mem == NULL) {
		total_bytes_too_many += bytes;
		vrErrPrintf("vrShmemAlloc0(%d): "
			RED_TEXT "No shmem allocated!  IE. out of shared memory. (%ld too much, %ld allocated)\n" NORM_TEXT,
			bytes, total_bytes_too_many, *total_bytes_overall);
#if defined(VRDBXPAUSE_ON_ERROR) || 1
		vrErrPrintf(BOLD_TEXT "pausing, use 'dbx -p %d' to debug\n" NORM_TEXT, getpid());
		pause();
#endif
	} else {
		memset(mem, 0, bytes);
	}

#ifdef VRTRACKMEM
	if (mem) {
#  if USE_SHMEM && !defined(SHM_PF_ARENA)
		*total_bytes_overall += amallocblksize(mem, arena);
		total_bytes_this_proc += amallocblksize(mem, arena);
#  else
		/* we can only assume that the size of the allocated memory matches the request */
		*total_bytes_overall += bytes;
		total_bytes_this_proc += bytes;
#  endif
		vrDbgPrintfN(TRACKMEM_DBGLVL, TRACK_SPACER "(shmem) allocated %d bytes so far\r", *total_bytes_overall);
	}
#endif

	return mem;
}


/*****************************************************************/
void *vrShmemRealloc(void *mem, size_t bytes)
{
#ifdef VRTRACKMEM
#  if USE_SHMEM && !defined(SHM_PF_ARENA)
	long old_size = (mem ? amallocblksize(mem, arena) : 0);
#  else
	long old_size = 0;	/* it's hard to guess what the actual value should be */
#  endif
#endif

#if USE_SHMEM
	if (arena || PERFORMER_2_3_BUG)
		mem = arealloc(mem, bytes, arena);
	else	vrErr("No shmem arena to reallocate from.");
#else
	mem = realloc(mem, bytes);
#endif

	if (mem == NULL) {
		total_bytes_too_many += bytes;
		vrErrPrintf("vrShmemRealloc(%d): "
			RED_TEXT "No shmem allocated!  IE. out of shared memory. (%ld too much, %ld allocated)\n" NORM_TEXT,
			bytes, total_bytes_too_many, *total_bytes_overall);
#ifdef VRDBXPAUSE_ON_ERROR
		vrErrPrintf(BOLD_TEXT "pausing, use 'dbx -p %d' to debug\n" NORM_TEXT, getpid());
		pause();
#endif
	}

#ifdef VRTRACKMEM
	if (mem) {
#  if USE_SHMEM && !defined(SHM_PF_ARENA)
		*total_bytes_overall += (amallocblksize(mem, arena) - old_size);
		total_bytes_this_proc += (amallocblksize(mem, arena) - old_size);
#  else
		/* we can only assume that the size of the allocated memory matches the request */
		*total_bytes_overall += bytes - old_size;
		total_bytes_this_proc += bytes - old_size;
#  endif
		vrDbgPrintfN(TRACKMEM_DBGLVL, TRACK_SPACER "(shmem) allocated %d bytes so far\r", *total_bytes_overall);
	}
#endif

	return mem;
}


/*****************************************************************/
void vrShmemFree(void *mem)
{
#ifdef VRTRACKMEM
	if (mem) {
#  if USE_SHMEM && !defined(SHM_PF_ARENA)
		*total_bytes_overall -= amallocblksize(mem, arena);
		*total_freed_overall += amallocblksize(mem, arena);
		total_bytes_this_proc -= amallocblksize(mem, arena);
#  else
		/* we can only assume that the size of the allocated memory matches the request */
		*total_bytes_overall -= 0;
		*total_freed_overall += 0;
		total_bytes_this_proc -= 0;
#  endif
		vrDbgPrintfN(TRACKMEM_DBGLVL, TRACK_SPACER "(shmem) allocated %d bytes so far -- freed %d\r", *total_bytes_overall, *total_freed_overall);
	}
#endif
#if USE_SHMEM
	if ((arena || PERFORMER_2_3_BUG) && mem)
		afree(mem, arena);
#else
	if (mem)
		free(mem);
#endif
}



/*****************************************************************/
/* Some functions for creating memory for specific types of things */


/*****************************************************************/
char *vrShmemStrDup(char *s)
{
	char	*mem = NULL;
	int	size;

	if (s == NULL)
		return NULL;

	size = strlen(s) + 1;

#if USE_SHMEM
	if (arena || PERFORMER_2_3_BUG) {
		mem = (char *)amalloc(size, arena);
#else
	if (1) {
		mem = (char *)malloc(size);
#endif
		if (mem == NULL) {
			total_bytes_too_many += size;
			vrErrPrintf("vrShmemStrDup(%s): "
				RED_TEXT "No shmem allocated!  IE. out of shared memory. (%ld too much, %ld allocated)\n" NORM_TEXT,
				s, total_bytes_too_many, *total_bytes_overall);
#ifdef VRDBXPAUSE_ON_ERROR
		vrErrPrintf(BOLD_TEXT "pausing, use 'dbx -p %d' to debug\n" NORM_TEXT, getpid());
		pause();
#endif
			return(mem);
		}

		strcpy(mem, s);
	} else {
		vrErr("No shmem arena to allocate from.");
	}

#ifdef VRTRACKMEM
	if (mem) {
#  if USE_SHMEM && !defined(SHM_PF_ARENA)
		*total_bytes_overall += amallocblksize(mem, arena);
		total_bytes_this_proc += amallocblksize(mem, arena);
#  else
		/* we can only assume that the size of the allocated memory matches the request */
		*total_bytes_overall += size;
		total_bytes_this_proc += size;
#  endif
		vrDbgPrintfN(TRACKMEM_DBGLVL, TRACK_SPACER "(shmem) allocated %d bytes so far\r", *total_bytes_overall);
	}
#endif
	return mem;
}


/*****************************************************************/
char *vrShmemStrCat(char *s1, char *s2)
{
	char	*mem = NULL;
	int	size = strlen(s1) + strlen(s2) + 1;

#if USE_SHMEM
	if (arena || PERFORMER_2_3_BUG) {
		mem = (char *)amalloc(size, arena);
#else
	if (1) {
		mem = (char *)malloc(size);
#endif
		if (mem == NULL) {
			total_bytes_too_many += size;
			vrErrPrintf("vrShmemStrCat(%s, %s): "
				RED_TEXT "No shmem allocated!  IE. out of shared memory. (%ld too much, %ld allocated)\n" NORM_TEXT,
				s1, s2, total_bytes_too_many, *total_bytes_overall);
#ifdef VRDBXPAUSE_ON_ERROR
		vrErrPrintf(BOLD_TEXT "pausing, use 'dbx -p %d' to debug\n" NORM_TEXT, getpid());
		pause();
#endif
			return(mem);
		}

		strcpy(mem, s1);
		strcpy(mem+strlen(s1), s2);
	} else {
		vrErr("No shmem arena to allocate from.");
	}

#ifdef VRTRACKMEM
	if (mem) {
#  if USE_SHMEM && !defined(SHM_PF_ARENA)
		*total_bytes_overall += amallocblksize(mem, arena);
		total_bytes_this_proc += amallocblksize(mem, arena);
#  else
		/* we can only assume that the size of the allocated memory matches the request */
		*total_bytes_overall += size;
		total_bytes_this_proc += size;
#  endif
		vrDbgPrintfN(TRACKMEM_DBGLVL, TRACK_SPACER "(shmem) allocated %d bytes so far\r", *total_bytes_overall);
	}
#endif
	return mem;
}


/*****************************************************************/
void *vrShmemMemDup(void *mem, int size)
{
	void	*newmem = NULL;

	if (mem == NULL)
		return NULL;

#if USE_SHMEM
	if (arena || PERFORMER_2_3_BUG) {
		newmem = (void *)amalloc(size, arena);
#else
	if (1) {
		newmem = (void *)malloc(size);
#endif
		if (newmem == NULL) {
			total_bytes_too_many += size;
			vrErrPrintf("vrShmemMemDup(%p, %d): "
				RED_TEXT "No shmem allocated!  IE. out of shared memory. (%ld too much, %ld allocated)\n" NORM_TEXT,
				mem, size, total_bytes_too_many, *total_bytes_overall);
#ifdef VRDBXPAUSE_ON_ERROR
		vrErrPrintf(BOLD_TEXT "pausing, use 'dbx -p %d' to debug\n" NORM_TEXT, getpid());
		pause();
#endif
			return(mem);
		}

		memcpy(newmem, mem, size);
	} else {
		vrErr("No shmem arena to allocate from.");
	}

#ifdef VRTRACKMEM
	if (newmem) {
#  if USE_SHMEM && !defined(SHM_PF_ARENA)
		*total_bytes_overall += amallocblksize(newmem, arena);
		total_bytes_this_proc += amallocblksize(newmem, arena);
#  else
		/* we can only assume that the size of the allocated memory matches the request */
		*total_bytes_overall += size;
		total_bytes_this_proc += size;
#  endif
		vrDbgPrintfN(TRACKMEM_DBGLVL, TRACK_SPACER "(shmem) allocated %d bytes so far\r", *total_bytes_overall);
	}
#endif
	return newmem;
}


/*****************************************************************/
/* 6/12/01: this function currently not used -- was used in old config method */
int *vrShmemIntArray(int count,...)
{
	int	*mem = NULL;
	int	size = count * sizeof(int);

	if (arena || PERFORMER_2_3_BUG) {
		if (count) {
			int i = 0;
			va_list ap;
#if USE_SHMEM
			mem = (int *)amalloc(size, arena);
#else
			mem = (int *)malloc(size);
#endif
			if (mem == NULL) {
				total_bytes_too_many += size;
				vrErrPrintf("vrShmemIntArray(%d, ...): "
					RED_TEXT "No shmem allocated!  IE. out of shared memory. (%ld too much, %ld allocated)\n" NORM_TEXT,
					count, total_bytes_too_many, *total_bytes_overall);
#ifdef VRDBXPAUSE_ON_ERROR
		vrErrPrintf(BOLD_TEXT "pausing, use 'dbx -p %d' to debug\n" NORM_TEXT, getpid());
		pause();
#endif
				return(mem);
			}

			va_start(ap, count);
			for (i = 0; i < count; i++)
				mem[i] = va_arg(ap, int);

			va_end(ap);
		}
	} else {
		vrErr("No shmem arena to allocate from.");
	}

#ifdef VRTRACKMEM
	if (mem) {
#  if USE_SHMEM && !defined(SHM_PF_ARENA)
		*total_bytes_overall += amallocblksize(mem, arena);
		total_bytes_this_proc += amallocblksize(mem, arena);
#  else
		/* we can only assume that the size of the allocated memory matches the request */
		*total_bytes_overall += size;
		total_bytes_this_proc += size;
#  endif
		vrDbgPrintfN(TRACKMEM_DBGLVL, TRACK_SPACER "(shmem) allocated %d bytes so far\r", *total_bytes_overall);
	}
#endif
	return mem;
}


/*****************************************************************/
/* 6/12/01: this function currently not used -- was used in old config method */
float *vrShmemFloatArray(int count,...)
{
	float	*mem = NULL;
	int	size = count * sizeof(float);

	if (arena || PERFORMER_2_3_BUG) {
		if (count) {
			int i = 0;
			va_list ap;
			mem = (float *)vrShmemAlloc(size);
			if (mem == NULL) {
				total_bytes_too_many += size;
				vrErrPrintf("vrShmemFloatArray(%d, ...): "
					RED_TEXT "No shmem allocated!  IE. out of shared memory. (%ld too much, %ld allocated)\n" NORM_TEXT,
					count, total_bytes_too_many, *total_bytes_overall);
#ifdef VRDBXPAUSE_ON_ERROR
		vrErrPrintf(BOLD_TEXT "pausing, use 'dbx -p %d' to debug\n" NORM_TEXT, getpid());
		pause();
#endif
				return(mem);
			}

			va_start(ap, count);
			for (i = 0; i < count; i++)
                                mem[i] = va_arg(ap, double); /* the va_arg _must_ be double, even though */
                                                             /* we're working with floats.               */

			va_end(ap);
		}
	} else {
		vrErr("No shmem arena to allocate from.");
	}

#if defined(VRTRACKMEM) || 0 /* since vrShmemAlloc() is used above, this would be redundant */
	if (mem) {
#  if USE_SHMEM && !defined(SHM_PF_ARENA)
		*total_bytes_overall += amallocblksize(mem, arena);
		total_bytes_this_proc += amallocblksize(mem, arena);
#  else
		/* we can only assume that the size of the allocated memory matches the request */
		*total_bytes_overall += size;
		total_bytes_this_proc += size;
#  endif
		vrDbgPrintfN(TRACKMEM_DBGLVL, TRACK_SPACER "(shmem) allocated %d bytes so far\r", *total_bytes_overall);
	}
#endif
	return mem;
}


/*****************************************************************/
/* 6/12/01: this function currently not used -- was used in old config method */
void **vrShmemPtrArray(int count,...)
{
	void	**mem = NULL;
	int	size = count * sizeof(void *);

	if (arena || PERFORMER_2_3_BUG) {
		if (count) {
			int i = 0;
			va_list ap;
			mem = (void **)vrShmemAlloc(size);
			if (mem == NULL) {
				total_bytes_too_many += size;
				vrErrPrintf("vrShmemPtrArray(%d, ...): "
					RED_TEXT "No shmem allocated!  IE. out of shared memory. (%ld too much, %ld allocated)\n" NORM_TEXT,
					count, total_bytes_too_many, *total_bytes_overall);
#ifdef VRDBXPAUSE_ON_ERROR
		vrErrPrintf(BOLD_TEXT "pausing, use 'dbx -p %d' to debug\n" NORM_TEXT, getpid());
		pause();
#endif
				return(mem);
			}

			va_start(ap, count);
			for (i = 0; i < count; i++)
				mem[i] = va_arg(ap, void *);

			va_end(ap);
		}
	} else {
		vrErr("No shmem arena to allocate from.");
	}

#if defined(VRTRACKMEM) || 0 /* since vrShmemAlloc() is used above, this would be redundant */
	if (mem) {
#  if USE_SHMEM && !defined(SHM_PF_ARENA)
		*total_bytes_overall += amallocblksize(mem, arena);
		total_bytes_this_proc += amallocblksize(mem, arena);
#  else
		/* we can only assume that the size of the allocated memory matches the request */
		*total_bytes_overall += size;
		total_bytes_this_proc += size;
#  endif
		vrDbgPrintfN(TRACKMEM_DBGLVL, TRACK_SPACER "(shmem) allocated %d bytes so far\r", *total_bytes_overall);
	}
#endif
	return mem;
}



		/********************************************/
		/*** Data Locking structure and functions ***/
		/********************************************/

/*
 * locking primitives --
 * for the most part, this code just apes stuff I (elp) found in the
 * dinosaur book (Silberschatz and Galvin's "Operating System
 * Concepts", 4th edition, pp 183-4)
 */

#undef vrTraceOpt
#ifdef VRTRACE_LOCK
#  define vrTraceOpt		vrTrace
#else
#  define vrTraceOpt(a,b)	;
#endif

/*************************/
#if defined(SEM_SYSVIPC) /* { */
	/* may have to be defined manually on some systems */

   /***************/
#  if defined(__sgi) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__APPLE__)
union semun semarg;
#  elif defined(__sun) || defined(_AIX)
union semun {
		int             val;
		struct semid_ds *buf;
		ushort_t        *array;
	} semarg;

   /***************/
#  elif defined(__linux)
union semun {
		int		val;		/* value for SETVAL */
		struct semid_ds	*buf;		/* buffer for IPC_STAT, IPC_SET */
	unsigned short int	*array;		/* array for GETALL, SETALL */
		struct seminfo	*__buf;		/* buffer for IPC_INFO */
	} semarg;

   /***************/
#  elif defined(__hpux)
union semun {
		int             val;
		struct semid_ds *buf;
	} semarg;
#  endif

#  if !defined(SEM_R)
#     define SEM_R 0400
#  endif

#  if !defined(SEM_A)
#     define SEM_A 0200
#  endif
#endif /* } SEM_SYSVIPC */


/*************************************************/
/* NOTE: the public vrLock datatype is just a (void *) type, */
/*   which can then be passed to the routines here, which    */
/*   treat the type as this fully-blown defined structure.   */
typedef struct vrLock_st {
		/***************************/
		/* Generic Object field(s) */
		vrObject	object_type;	/* The type of this object (ie VROBJECT_LOCK) */
		/* NOTE: the rest of the generic object fields not included with this struct  */
		/* TODO: consider adding the other object fields to this structure.           */

		/***************************/
		/* specific lock fields */
		char		*name;		/* name to use with debugging output */
	struct	vrLock_st	*next;		/* pointer to the next lock in the list */
		int		trace;		/* flag to do tracing debug output */
		int		disabled;	/* flag to indicate that this lock is disabled (usually because it's bad) */
		int		readcount;	/* counter of number of readers locking this lock */
		int		opcount;	/* counter of number of operations done on this lock */
		int		total_readsets;	/* counter of total number read-sets on this lock */
		int		total_readrels;	/* counter of total number read-releases on this lock */
		int		total_writesets;/* counter of total number write-sets on this lock */
		int		total_writerels;/* counter of total number write-releases on this lock */

/***********************/
#if defined(SEM_IRIX)
		usema_t		*wmutex;	/* semaphore ID of write semaphore */
		usema_t		*rmutex;	/* semaphore ID of readcount/opcount access semaphore */

/***********************/
#elif defined(SEM_POSIX)
		sem_t		*wmutex;	/* semaphore ID of write semaphore */
		sem_t		*rmutex;	/* semaphore ID of readcount/opcount access semaphore */

/***********************/
#elif defined(SEM_SYSVIPC)
		struct sembuf	w_waitop;	/* wait operation on write mutex*/
		struct sembuf	w_postop;	/* post operation on write mutex*/
		struct sembuf	r_waitop;	/* wait operation on read mutex*/
		struct sembuf	r_postop;	/* post operation on read mutex*/
                int		wmutex_semset;	/* semaphore set ID of write semaphore */
                int		wmutex_semnum;	/* semaphore ID of write semaphore (in the set) */
                int		rmutex_semset;	/* semaphore set ID of readcount/opcount access semaphore */
                int		rmutex_semnum;	/* semaphore ID of readcount/opcount access semaphore (in the set) */

/***********************/
#elif defined(SEM_WIN32)
		HANDLE		wmutex;		/* semaphore ID of write semaphore */
		HANDLE		rmutex;		/* semaphore ID of readcount/opcount access semaphore */

/***********************/
#elif defined(SEM_TCP)
		MUTEX_HANDLE	wmutex;		/* semaphore ID of write semaphore */
		MUTEX_HANDLE	rmutex;		/* semaphore ID of readcount/opcount access semaphore */

/***********************/
#else
		/* TODO: is this good enough, or is there more? */
		void		*wmutex;	/* semaphore ID of write semaphore */
		void		*rmutex;	/* semaphore ID of readcount/opcount access semaphore */
#endif
	} vrPrivateLock;


#if 1
/*****************************************************************/
/* This is just for versions 0.4f 0.5<l>, since I've changed the arguments  */
/*   for for vrLockCreate(), and will be making more changes in version 0.6 */
vrLock vrNCLockCreate()
{
	return vrLockCreate(vrContext);
}
#endif


/*****************************************************************/
/*
 * the private lock structure will have two semaphores -- one
 * to control access to a count of readers and one to control
 * write access (only one writer allowed at a time)
 */
/* TODO: I don't know why we're forced to use "void *" here, when it works fine for vrBarrierCreate() -- figure this out! */
vrLock vrLockCreate(void *void_context)
{
	vrContextInfo		*context = (vrContextInfo *)void_context;
	vrPrivateLock		*lock = (vrPrivateLock *)vrShmemAlloc(sizeof(vrPrivateLock));
#if defined(SEM_POSIX)
	int			werrno = 0;	/* errno from an operation on the write semaphore */
	int			rerrno = 0;	/* errno from an operation on the read semaphore */
#endif
#if defined(SEM_WIN32)
	SECURITY_ATTRIBUTES	sec;
#endif

	while (context == NULL) {
		/* we should not get here */
		vrPrintf("vrLockCreate(): Without a context pointer, locks should not be created!\n");
		abort();
	}

	/**************************************/
	/* add this lock to the list of locks */
	if (context->head_lock != NULL) {
		/* we don't do this part for the first lock -- the lock lock */
		vrLockWriteSet(context->head_lock);	/* the head_lock is the lock for the lock list */
		((vrPrivateLock *)(context->tail_lock))->next = lock;
		context->tail_lock = lock;
		vrLockWriteRelease(context->head_lock);
	}

	/******************************/
	/* assign some initial values */
	lock->object_type = VROBJECT_LOCK;
	lock->name = vrShmemStrDup("");
	lock->next = NULL;
	lock->trace = 0;
	lock->disabled = 0;
	lock->readcount = 0;
	lock->opcount = 0;
	lock->total_readsets = 0;
	lock->total_readrels = 0;
	lock->total_writesets = 0;
	lock->total_writerels = 0;

	/***********************************************************/
	/* setup the mutual-exclusion semaphores (system dependent) */

/***********************/
#if defined(SEM_IRIX)
	lock->wmutex = usnewsema(usarena, 1);
	lock->rmutex = usnewsema(usarena, 1);

/***********************/
#elif defined(SEM_POSIX)
        lock->wmutex = vrShmemAlloc(sizeof(sem_t));
        lock->rmutex = vrShmemAlloc(sizeof(sem_t));
#  if !defined(_POSIX_THREAD_PROCESS_SHARED)
        if (sem_init(lock->wmutex, 0, 1) < 0)
		werrno = errno;
        if (sem_init(lock->rmutex, 0, 1) < 0)
		rerrno = errno;

#  else
        if (sem_init(lock->wmutex, 1, 1) < 0)
		werrno = errno;
        if (sem_init(lock->rmutex, 1, 1) < 0)
		rerrno = errno;
#  endif
#  if 1 || (!defined(MP_PTHREADS) && !defined(MP_PTHREADS2)) /* TODO: (test) POSIX semaphores may be working in this config -- not sure */
	/* thus far, tested with -DSHM_SVR4MMAP -DSEM_POSIX on a Linux system, and barriers were working [3/25/05] */
	if (werrno != 0 && rerrno != 0) {
		vrDbgPrintfN(AALWAYS_DBGLVL, "vrLockCreate(): SEM_POSIX sem_init errnos: %d %d -- _POSIX_THREAD_PROCESS_SHARED is %sset.\n", werrno, rerrno,
#    if defined(_POSIX_THREAD_PROCESS_SHARED)
		""
#    else
		"NOT "
#    endif
		);
	}
#  endif

/***********************/
#elif defined(SEM_SYSVIPC)

	/* Because the values for semset and semnum are stored in the context */
	/*   structure, and we plan to modify them, we must use the lock-lock */
	/*   to protect multiple accesses.                                    */

	vrLockWriteSet(context->head_lock);	/* the head_lock is the lock for lock information */
	/* If we don't have a semaphore set to allocated from, or we've  */
	/*    used up all the existing semaphores, then create a new one.*/
	/* NOTE: SEMMSL-2 is used because we'll need two semaphores for a new lock. */
	if ((context->lock_semset <= 0) || (context->lock_semnum > SEMMSL-2)) {
		errno = 0;
		context->lock_semnum = 0;
		context->lock_semset = semget(IPC_PRIVATE, SEMMSL, IPC_CREAT | IPC_EXCL | SEM_A | SEM_R);
		if (context->lock_semset < 0) {
			/* failed to acquire semaphore set */
			vrErrPrintf("vrLockCreate(): " RED_TEXT "Can't get the necessary semaphore set (%p).\n" NORM_TEXT, lock);
			perror("vrLockCreate()");
			context->startup_error |= VRSTARTUP_NOLOCKSEM;	/* TODO: I guess we should try creating the semaphore set during initialization -- otherwise setting this value here makes no sense */

			vrErrPrintf("vrLockCreate(): " RED_TEXT "NOTE: This likely means FreeVR will not operate correctly.\n" NORM_TEXT);
			vrErrPrintf("vrLockCreate(): " RED_TEXT "      A probable solution is to run the 'ipcrms' program\n" NORM_TEXT);
			vrErrPrintf("vrLockCreate(): " RED_TEXT "      that comes with the FreeVR distribution.\n" NORM_TEXT);
			vrSleep(2000000);

			/* set some values for a lock with no semaphore segment */
			lock->wmutex_semset = -1;
			lock->rmutex_semset = -1;
			lock->wmutex_semnum = -1;
			lock->rmutex_semnum = -1;
			vrLockWriteRelease(context->head_lock);

			return (vrLock)lock;
		}
	}

        lock->wmutex_semset = context->lock_semset;
        lock->rmutex_semset = context->lock_semset;
	lock->wmutex_semnum = context->lock_semnum++;
	lock->rmutex_semnum = context->lock_semnum++;
	vrLockWriteRelease(context->head_lock);

        lock->w_waitop.sem_num = lock->wmutex_semnum;
        lock->w_waitop.sem_op  = -1;
        lock->w_waitop.sem_flg = 0;
        lock->r_waitop.sem_num = lock->rmutex_semnum;
        lock->r_waitop.sem_op  = -1;
        lock->r_waitop.sem_flg = 0;
        lock->w_postop.sem_num = lock->wmutex_semnum;
        lock->w_postop.sem_op  = 1;
        lock->w_postop.sem_flg = 0;
        lock->r_postop.sem_num = lock->rmutex_semnum;
        lock->r_postop.sem_op  = 1;
        lock->r_postop.sem_flg = 0;

	/* initialize SYSV semaphore */
	semarg.val = 0;
	if (semctl(lock->wmutex_semset, lock->wmutex_semnum, SETVAL, semarg) < 0) {
		/* failed to set value for semaphore */
		vrErrPrintf("vrLockCreate(): " RED_TEXT "Can't set semaphore value for w-mutex.\n" NORM_TEXT);
		perror("vrLockCreate()");
	}
	if (semctl(lock->rmutex_semset, lock->rmutex_semnum, SETVAL, semarg) < 0) {
		/* failed to set value for semaphore */
		vrErrPrintf("vrLockCreate(): " RED_TEXT "Can't set semaphore value for r-mutex.\n" NORM_TEXT);
		perror("vrLockCreate()");
	}
	if (semop(lock->wmutex_semset, &lock->w_postop, 1) < 0) {
		/* failed to perform semaphore operation */
		vrErrPrintf("vrLockCreate(): " RED_TEXT "Can't perform semaphore operation for w-mutex.\n" NORM_TEXT);
		perror("vrLockCreate()");
	}
	if (semop(lock->rmutex_semset, &lock->r_postop, 1) < 0) {
		/* failed to perform semaphore operation */
		vrErrPrintf("vrLockCreate(): " RED_TEXT "Can't perform semaphore operation for r-mutex.\n" NORM_TEXT);
		perror("vrLockCreate()");
	}

/***********************/
#elif defined(SEM_WIN32)
	/* set the security attributes */
	memset(&sec, 0, sizeof(sec));
	sec.nLength = sizeof(sec);
	sec.lpSecurityDescriptor = NULL;
	sec.bInheritHandle = 1;

	lock->wmutex = CreateSemaphore(&sec, 1, 1, NULL);
	lock->rmutex = CreateSemaphore(&sec, 1, 1, NULL);

/***********************/
#elif defined(SEM_TCP)
	lock->wmutex = vrTcpNewMutex();
	lock->rmutex = vrTcpNewMutex();

/***********************/
#else
	/* There is no default lock scheme, so choose one of the above, or make a new one. */
	/* Without more information, basically make a no-op lock */
	lock->wmutex = NULL;
	lock->rmutex = NULL;
#endif

#ifdef VRTRACKLOCKS
	vrDbgPrintfN(TRACKLOCKS_DBGLVL, "(lock) created lock %p\n", lock);
#endif

	return (vrLock)lock;
}


/*****************************************************************/
vrLock vrLockCreateName(void *context, char *name)
{
	vrLock		lock = vrLockCreate(context);
	vrPrivateLock	*plock = (vrPrivateLock *)lock;

	if (name && *name)
		plock->name = vrShmemStrDup(name);
	else	plock->name = "";

	return lock;
}


/*****************************************************************/
vrLock vrLockCreateTraced(void *context, char *name)
{
static	char		string[128];
	vrLock		lock = vrLockCreate(context);
	vrPrivateLock	*plock = (vrPrivateLock *)lock;

	plock->trace = 1;
	if (name && *name)
		plock->name = vrShmemStrDup(name);
	else	plock->name = "";

	vrDbgPrintfN(ALWAYS_DBGLVL, "LOCK: Tracing new lock '%s' [%p], status %s\n",
			plock->name, plock, vrLockStatus(lock, string));
	vrFprintLock(stdout, plock, verbose);

	return lock;
}


/*****************************************************************/
/*
 * a freed lock will have both semaphores set to NULL;
 * it's never an error to release a lock
 */
void vrLockFree(vrLock lock)
{
static	char		string[128];
	vrContextInfo	*context = (vrContextInfo *)vrContext;	/* in the near future, "context" will be an argument */
	vrPrivateLock	*plock = (vrPrivateLock *)lock;

	if (plock->trace || vrDbgDo(SHMEMD_DBGLVL)) {
		vrDbgPrintfN(ALWAYS_DBGLVL, "LOCK: Deleting lock '%s' [%p], pre-status %s\n",
				plock->name, plock, vrLockStatus(lock, string));
		vrFprintLock(stdout, plock, verbose);
	}

	/* First, remove this lock from the linked list of locks (except for the head-lock) */
	if (context != NULL && lock != context->head_lock) {
		vrPrivateLock	*search_lock;
		vrPrivateLock	*prev = ((vrPrivateLock *)(context->head_lock))->next;	/* start with the second lock -- we always want the first lock on the list */

		vrLockWriteSet(context->head_lock);	/* the head_lock is the lock for the lock list */
		for (search_lock = prev; search_lock != NULL; search_lock = search_lock->next) {
			if (plock == search_lock) {
				/* remove this one from the list */
				prev->next = search_lock->next;

				if (search_lock == context->tail_lock) {
					/* if this was the last lock on the list, then fix the list's tail pointer */
					context->tail_lock = prev;
				}
			}
			prev = search_lock;
		}

		vrLockWriteRelease(context->head_lock);
	}

	/* Now release anything the lock is holding */
	vrLockWriteRelease(lock);
	vrLockReadRelease(lock);
	vrLockWriteSet(lock);

/***********************/
#if defined(SEM_IRIX)
	if (plock->wmutex != NULL)
		usfreesema(plock->wmutex, usarena);
	if (plock->rmutex != NULL)
		usfreesema(plock->rmutex, usarena);
	plock->wmutex = plock->rmutex = NULL;

/***********************/
#elif defined(SEM_POSIX)
	if (plock->wmutex != NULL) {
		sem_destroy(plock->wmutex);
		vrShmemFree(plock->wmutex);
		plock->wmutex = NULL;
	}
	if (plock->rmutex != NULL) {
		sem_destroy(plock->rmutex);
		vrShmemFree(plock->rmutex);
		plock->rmutex = NULL;
	}

/***********************/
#elif defined(SEM_SYSVIPC)
	if (plock->wmutex_semset != -1) {
		semctl(plock->wmutex_semset, plock->wmutex_semnum, IPC_RMID);
		plock->wmutex_semset = -1;
	}
	if (plock->rmutex_semset != -1) {
		semctl(plock->rmutex_semset, plock->rmutex_semnum, IPC_RMID);
		plock->rmutex_semset = -1;
	}

/***********************/
#elif defined(SEM_WIN32)
	if (plock->wmutex != NULL)
		CloseHandle(plock->wmutex);
	if (plock->rmutex != NULL)
		CloseHandle(plock->rmutex);
	plock->wmutex = plock->rmutex = NULL;

/***********************/
#elif defined(SEM_TCP)
	vrTcpDeleteMutex(plock->wmutex);
	vrTcpDeleteMutex(plock->rmutex);

/***********************/
#else
	/* There is no default lock scheme, so choose one of the above, or make a new one. */
#endif

#ifdef VRTRACKLOCKS
	vrDbgPrintfN(TRACKLOCKS_DBGLVL, "(lock) freed lock %p\n", lock);
#endif
	return;
}


/*****************************************************************/
/* Loop over the linked list, calling vrLockFree() for each */
void vrLockFreeAll(vrLock lock)
{
	vrContextInfo	*context = (vrContextInfo *)vrContext;	/* in the near future, "context" will be an argument */
	vrPrivateLock	*first = (vrPrivateLock *)lock;
	vrPrivateLock	*plock;
	vrPrivateLock	*next;

	for (plock = first; plock != NULL; plock = next) {
		next = plock->next;
		/* don't delete the head-lock (not yet anyway) */
		if (plock != context->head_lock)
			vrLockFree(plock);
	}

	/* if the head-lock was passed as the argument, then free it last */
	if (lock == context->head_lock)
		vrLockFree(lock);
}


/********************************/
/* Verify that "lock" is a lock */
int vrLockCheck(vrLock lock)
{
	vrPrivateLock	*plock = (vrPrivateLock *)lock;

	if (plock == NULL)
		return 0;

	return (plock->object_type == VROBJECT_LOCK);
}


/*****************************************************************/
/*
 * a new reader will increment readcount
 * if readcount was previously 0, we'll want to get a write lock as well
 *
 * a finished reader will decrement the readcount
 * if readcount is decremented to 0, then release the writelock
 *
 * opcount will also be incremented within the semaphore blocking for readcount
 */
void vrLockReadSet(vrLock lock)
{
static	char		string[128];
	vrPrivateLock	*plock = (vrPrivateLock *)lock;
	int		local_readcount = -1;
	int		local_opcount = -1;
	int		local_exclude = 0;

	/* skip disabled locks */
	if (plock->disabled) {
		vrTraceOpt("vrLockWriteRelease", "skipping disabled lock");
		return;
	}

	/* report info if this lock is in "trace" mode */
	if (plock->trace)
		vrDbgPrintfN(ALWAYS_DBGLVL, "LOCK: Acquiring read lock '%s' [%p], pre-status %s\n",
				plock->name, plock, vrLockStatus(lock, string));

#ifdef VRTRACKLOCKS
	vrDbgPrintfN(TRACKLOCKS_DBGLVL, "(lock) setting lock %p for reading\n", lock);
#endif

	/* verify that this is a good lock */
#ifdef SEM_SYSVIPC
	if (plock->rmutex_semset <= 0)
#else
	if (plock->rmutex <= 0)
#endif
	{
		vrErrPrintf("vrLockReadSet(): " RED_TEXT "Can't set read lock: bad lock [%p] -- disabling.\n" NORM_TEXT, lock);

#ifdef VRDBXPAUSE_ON_ERROR
		vrPrintf(BOLD_TEXT "use 'dbx -p %d' to debug\n", getpid());
		pause();
#endif
		plock->disabled = 1;	/* mark this lock as being disabled */
		return;
	}

	/* handle the lock operation */
#ifdef VRTRACE_LOCK
	vrDbgPrintfN(TRACE_DBGLVL, "(%s::%d) %s -> %s '%s'\n", __FILE__, __LINE__, "vrLockReadSet", "about to lock", plock->name);
#endif

/***********************/
#if defined(SEM_IRIX)
	uspsema(plock->rmutex);					/* set lock on readcount data */

	plock->total_readsets++;
	plock->readcount++;
	local_readcount = plock->readcount;
	plock->opcount++;
	local_opcount = plock->opcount;
	if (plock->readcount == 1) {				/* first reader sets exclusion lock */
		uspsema(plock->wmutex);
		local_exclude = 1;
	}

	usvsema(plock->rmutex);					/* release lock on readcount data */

/***********************/
#elif defined(SEM_POSIX)
        sem_wait(plock->rmutex);				/* set lock on readcount data */

	plock->total_readsets++;
	plock->readcount++;
	local_readcount = plock->readcount;
	plock->opcount++;
	local_opcount = plock->opcount;
	if (plock->readcount == 1) {				/* first reader sets exclusion lock */
		sem_wait(plock->wmutex);
		local_exclude = 1;
	}

        sem_post(plock->rmutex);				/* release lock on readcount data */

/***********************/
#elif defined(SEM_WIN32)
	_WaitForSemaphore(plock->rmutex);			/* set lock on readcount data */

	plock->total_readsets++;
	plock->readcount++;
	local_readcount = plock->readcount;
	plock->opcount++;
	local_opcount = plock->opcount;
	if (plock->readcount == 1) {				/* first reader sets exclusion lock */
		_WaitForSemaphore(plock->wmutex);
		local_exclude = 1;
	}

	ReleaseSemaphore(plock->rmutex, 1, NULL);		/* release lock on readcount data */

/***********************/
#elif defined(SEM_SYSVIPC)
        semop(plock->rmutex_semset, &plock->r_waitop, 1);	/* set lock on readcount data */

	plock->total_readsets++;
	plock->readcount++;
	local_readcount = plock->readcount;
	plock->opcount++;
	local_opcount = plock->opcount;
	if (plock->readcount == 1) {				/* first reader sets exclusion lock */
		semop(plock->wmutex_semset, &plock->w_waitop, 1);
		local_exclude = 1;
	}

        semop(plock->rmutex_semset, &plock->r_postop, 1);	/* release lock on readcount data */

/***********************/
#elif defined(SEM_TCP)
	vrTcpAcquireMutex(plock->rmutex);			/* set lock on readcount data */

	plock->total_readsets++;
	plock->readcount++;
	local_readcount = plock->readcount;
	plock->opcount++;
	local_opcount = plock->opcount;
	if (plock->readcount == 1) {				/* first reader sets exclusion lock */
		vrTcpAcquireMutex(plock->wmutex);
		local_exclude = 1;
	}

	vrTcpReleaseMutex(plock->rmutex);			/* release lock on readcount data */

/***********************/
#else
	/* TODO: No default lock scheme -- should we print a warning? */
#endif
	if (plock->trace)
		vrDbgPrintfN(ALWAYS_DBGLVL, "LOCK: Acquired  read lock '%s' [%p], new-status %s, rc = %d, oc = %d, ex = %d\n",
				plock->name, plock, vrLockStatus(lock, string),
				local_readcount, local_opcount, local_exclude);

	vrTraceOpt("vrLockReadSet", "locked");
}


/*****************************************************************/
void vrLockReadRelease(vrLock lock)
{
static	char		string[128];
	vrPrivateLock	*plock = (vrPrivateLock *)lock;
	int		local_readcount = -1;
	int		local_opcount = -1;
	int		local_exclude = 0;

	/* skip disabled locks */
	if (plock->disabled) {
		vrTraceOpt("vrLockWriteRelease", "skipping disabled lock");
		return;
	}

	/* report info if this lock is in "trace" mode */
	if (plock->trace)
		vrDbgPrintfN(ALWAYS_DBGLVL, "LOCK: Releasing read lock '%s' [%p], pre-status %s\n",
				plock->name, plock, vrLockStatus(lock, string));

#ifdef VRTRACKLOCKS
	vrDbgPrintfN(TRACKLOCKS_DBGLVL, "(lock) freeing lock %p for reading\n", lock);
#endif

	/* report info if this lock is in "trace" mode */
#ifdef SEM_SYSVIPC
	if (plock->rmutex_semset <= 0)
#else
	if (plock->rmutex <= 0)
#endif
	{
		vrErrPrintf("vrLockReadRelease(): " RED_TEXT "Can't release read lock: bad lock [%p] -- disabling.\n" NORM_TEXT, lock);
		plock->disabled = 1;	/* mark this lock as being disabled */
		return;
	}

	/* handle the lock operation */
#ifdef VRTRACE_LOCK
	vrDbgPrintfN(TRACE_DBGLVL, "(%s::%d) %s -> %s '%s'\n", __FILE__, __LINE__, "vrLockReadRelease", "about to release", plock->name);
#endif

/***********************/
#if defined(SEM_IRIX)
	uspsema(plock->rmutex);					/* set lock on readcount data */

	plock->total_readrels++;
	plock->readcount--;
	local_readcount = plock->readcount;
	plock->opcount++;
	local_opcount = plock->opcount;
	if (plock->readcount == 0) {				/* last readlock releases exclusion lock */
		usvsema(plock->wmutex);
		local_exclude = 1;
	}

	usvsema(plock->rmutex);					/* release lock on readcount data */

/***********************/
#elif defined(SEM_POSIX)
	sem_wait(plock->rmutex);				/* set lock on readcount data */

	plock->total_readrels++;
	plock->readcount--;
	local_readcount = plock->readcount;
	plock->opcount++;
	local_opcount = plock->opcount;
	if (plock->readcount == 0) {				/* last readlock releases exclusion lock */
		sem_post(plock->wmutex);
		local_exclude = 1;
	}

	sem_post(plock->rmutex);				/* release lock on readcount data */

/***********************/
#elif defined(SEM_WIN32)
	_WaitForSemaphore(plock->rmutex);			/* set lock on readcount data */

	plock->total_readrels++;
	plock->readcount--;
	local_readcount = plock->readcount;
	plock->opcount++;
	local_opcount = plock->opcount;
	if (plock->readcount == 0) {				/* last readlock releases exclusion lock */
		ReleaseSemaphore(plock->wmutex, 1, NULL);
		local_exclude = 1;
	}

	ReleaseSemaphore(plock->rmutex, 1, NULL);		/* release lock on readcount data */

/***********************/
#elif defined(SEM_SYSVIPC)
	semop(plock->rmutex_semset, &plock->r_waitop, 1);	/* set lock on readcount data */

	plock->total_readrels++;
	plock->readcount--;
	local_readcount = plock->readcount;
	plock->opcount++;
	local_opcount = plock->opcount;
	if (plock->readcount == 0) {				/* last readlock releases exclusion lock */
		semop(plock->wmutex_semset, &plock->w_postop, 1);
		local_exclude = 1;
	}

        semop(plock->rmutex_semset, &plock->r_postop, 1);	/* release lock on readcount data */

/***********************/
#elif defined(SEM_TCP)
	vrTcpAcquireMutex(plock->rmutex);			/* set lock on readcount data */

	plock->total_readrels++;
	plock->readcount--;
	local_readcount = plock->readcount;
	plock->opcount++;
	local_opcount = plock->opcount;
	if (plock->readcount == 0) {				/* last readlock releases exclusion lock */
		vrTcpReleaseMutex(plock->wmutex);
		local_exclude = 1;
	}

	vrTcpReleaseMutex(plock->rmutex);			/* release lock on readcount data */

/***********************/
#else
	/* TODO: No default lock scheme -- should we print a warning? */
#endif
	if (plock->trace)
		vrDbgPrintfN(ALWAYS_DBGLVL, "LOCK: Released  read lock '%s' [%p], new-status %s, rc = %d, oc = %d, ex = %d\n",
				plock->name, plock, vrLockStatus(lock, string),
				local_readcount, local_opcount, local_exclude);


	vrTraceOpt("vrLockReadRelease", "released");
}


/*****************************************************************/
/* TODO: this should probably be combined into one atomic operation */
void vrLockReadToWrite(vrLock lock)
{
	vrLockReadRelease(lock);
	vrLockWriteSet(lock);
}


/*****************************************************************/
/*
 * a new writer waits to acquire the write lock
 * and frees it when they're done
 */
void vrLockWriteSet(vrLock lock)
{
static	char		string[128];
	vrPrivateLock	*plock = (vrPrivateLock *)lock;
	int		local_opcount;

	/* plock should only be NULL when creating the first lock (context->head_lock) */
	if (plock == NULL) {
		if (vrContext->tail_lock == NULL) {
			return;
		} else {
			vrErrPrintf("vrLockWriteSet() called with a NULL lock!\n");
			return;
		}
	}

	/* skip disabled locks */
	if (plock->disabled) {
		vrTraceOpt("vrLockWriteRelease", "skipping disabled lock");
		return;
	}

	/* report info if this lock is in "trace" mode */
	if (plock->trace)
		vrDbgPrintfN(ALWAYS_DBGLVL, "LOCK: Acquiring *write* lock '%s' [%p], pre-status %s\n",
				plock->name, plock, vrLockStatus(lock, string));

#ifdef VRTRACKLOCKS
	vrDbgPrintfN(TRACKLOCKS_DBGLVL, "(lock) setting lock %p for writing\n", lock);
#endif

	/* report info if this lock is in "trace" mode */
#ifdef SEM_SYSVIPC
	if (plock->wmutex_semset <= 0)
#else
	if (plock->wmutex <= 0)
#endif
	{
		vrErrPrintf("vrLockWriteSet(): " RED_TEXT "Can't set write lock: bad lock [%p] -- disabling.\n" NORM_TEXT, lock);
		plock->disabled = 1;	/* mark this lock as being disabled */
		return;
	}

	/* handle the lock operation */
#ifdef VRTRACE_LOCK
	vrDbgPrintfN(TRACE_DBGLVL, "(%s::%d) %s -> %s '%s'\n", __FILE__, __LINE__, "vrLockWriteSet", "about to lock", plock->name);
#endif

/***********************/
#if defined(SEM_IRIX)
	uspsema(plock->wmutex);

/***********************/
#elif defined(SEM_POSIX)
	sem_wait(plock->wmutex);

/***********************/
#elif defined(SEM_WIN32)
	_WaitForSemaphore(plock->wmutex);

/***********************/
#elif defined(SEM_SYSVIPC)
	semop(plock->wmutex_semset, &plock->w_waitop, 1);

/***********************/
#elif defined(SEM_TCP)
	vrTcpAcquireMutex(plock->wmutex);

/***********************/
#else
	/* TODO: No default lock scheme -- should we print a warning? */
#endif
	plock->total_writesets++;
	plock->opcount++;
	local_opcount = plock->opcount;

	if (plock->trace)
		vrDbgPrintfN(ALWAYS_DBGLVL, "LOCK: Acquired  *write* lock '%s' [%p], new-status %s, oc = %d\n",
				plock->name, plock, vrLockStatus(lock, string),
				local_opcount);

	vrTraceOpt("vrLockWriteSet", "locked");
}


/*****************************************************************/
void vrLockWriteRelease(vrLock lock)
{
static	char		string[128];
	vrPrivateLock	*plock = (vrPrivateLock *)lock;
	int		local_opcount;

	/* plock should only be NULL when creating the first lock (context->head_lock) */
	if (plock == NULL) {
		if (vrContext->tail_lock == NULL) {
			return;
		} else {
			vrErrPrintf("vrLockWriteSet() called with a NULL lock!\n");
			return;
		}
	}

	/* skip disabled locks */
	if (plock->disabled) {
		vrTraceOpt("vrLockWriteRelease", "skipping disabled lock");
		return;
	}

	/* report info if this lock is in "trace" mode */
	if (plock->trace)
		vrDbgPrintfN(ALWAYS_DBGLVL, "LOCK: Releasing *write* lock '%s' [%p], pre-status %s\n",
				plock->name, plock, vrLockStatus(lock, string));

#ifdef VRTRACKLOCKS
	vrDbgPrintfN(TRACKLOCKS_DBGLVL, "(lock) freeing lock %p for writing\n", lock);
#endif

	/* report info if this lock is in "trace" mode */
#ifdef SEM_SYSVIPC
	if (plock->wmutex_semset <= 0)
#else
	if (plock->wmutex <= 0)
#endif
	{
		vrErrPrintf("vrLockWriteSet(): " RED_TEXT "Can't set write lock: bad lock [%p] -- disabling.\n" NORM_TEXT, lock);
		plock->disabled = 1;	/* mark this lock as being disabled */
		return;
	}

	/* handle the lock operation */
	plock->total_writerels++;
	plock->opcount++;
	local_opcount = plock->opcount;
#ifdef VRTRACE_LOCK
	vrDbgPrintfN(TRACE_DBGLVL, "(%s::%d) %s -> %s '%s'\n", __FILE__, __LINE__, "vrLockWriteRelease", "about to release", plock->name);
#endif

/***********************/
#if defined(SEM_IRIX)
	usvsema(plock->wmutex);

/***********************/
#elif defined(SEM_POSIX)
	sem_post(plock->wmutex);

/***********************/
#elif defined(SEM_WIN32)
	ReleaseSemaphore(plock->wmutex, 1, NULL);

/***********************/
#elif defined(SEM_SYSVIPC)
	semop(plock->wmutex_semset, &plock->w_postop, 1);

/***********************/
#elif defined(SEM_TCP)
	vrTcpReleaseMutex(plock->wmutex);

/***********************/
#else
	/* TODO: No default lock scheme -- should we print a warning? */
#endif

	if (plock->trace)
		vrDbgPrintfN(ALWAYS_DBGLVL, "LOCK: Released  *write* lock '%s' [%p], new-status %s, oc = %d\n",
				plock->name, plock, vrLockStatus(lock, string),
				local_opcount);

	vrTraceOpt("vrLockWriteRelease", "released");
}


/*****************************************************************/
void vrLockTrace(vrLock lock, int trace)
{
static	char		string[128];
	vrPrivateLock	*plock = (vrPrivateLock *)lock;

	if (vrLockCheck(plock)) {
		plock->trace = trace;
	}

	vrDbgPrintfN(ALWAYS_DBGLVL, "LOCK: Tracing lock '%s' [%p] now " BOLD_TEXT "%s" NORM_TEXT ", pre-status %s\n",
		plock->name, plock, (trace ? "on" : "off"), vrLockStatus(lock, string));
}


/*****************************************************************/
/* vrLockStatus() returns a string with a summary of the lock's  */
/*   current status.  It is the same format as the one_line print*/
/*   style status given for a lock.  This function is intended   */
/*   for use by the lock-tracing routines.                       */
/* NOTE: string should have memory allocated, which is where the */
/*   information will be placed.                                 */
char *vrLockStatus(vrLock lock, char *string)
{
	vrPrivateLock	*plock = (vrPrivateLock *)lock;
	int		wvalue;
	int		rvalue;
#if defined(SEM_SYSVIPC)
	int		wncnt, rncnt;
	int		wzcnt, rzcnt;
#endif
#if defined(SEM_IRIX) || defined(SEM_POSIX) || defined(SEM_SYSVIPC)
	int		lock_held, read_held, write_held;
	int		read_procs_holding;
	int		procs_waiting;
	int		read_procs_waiting;
	int		write_procs_waiting;
#endif

	/* if null pointer given, print an empty shell and return */
	if (plock == NULL) {
		sprintf(string, "<nil> lock");
		return string;
	}

	/****************************************************************/
	/* Get the values of the semaphores based on the defined system */
/***********************/
#if defined(SEM_IRIX)
		wvalue = ustestsema(plock->wmutex);
		rvalue = ustestsema(plock->rmutex);

/***********************/
#elif defined(SEM_POSIX)
		sem_getvalue(plock->wmutex, &wvalue);
		sem_getvalue(plock->rmutex, &rvalue);

/***********************/
#elif defined(SEM_SYSVIPC)
		wvalue = semctl(plock->wmutex_semset, plock->wmutex_semnum, GETVAL);
		wncnt  = semctl(plock->wmutex_semset, plock->wmutex_semnum, GETNCNT);
		rvalue = semctl(plock->rmutex_semset, plock->rmutex_semnum, GETVAL);
		rncnt  = semctl(plock->rmutex_semset, plock->rmutex_semnum, GETNCNT);
		wzcnt  = semctl(plock->wmutex_semset, plock->wmutex_semnum, GETZCNT);
		rzcnt  = semctl(plock->rmutex_semset, plock->rmutex_semnum, GETZCNT);
/***********************/
#elif defined(SEM_WIN32) && 0 /* disabled until the correct code is written */
		/* TODO: write the correct code */

/***********************/
#elif defined(SEM_TCP) && 0 /* disabled until the correct code is written */
		/* TODO: write the correct code */

/***********************/
#else
		wvalue = -1;
		rvalue = -1;
#endif

/******************************************************************/
#if defined(SEM_IRIX) || defined(SEM_POSIX) || defined(SEM_SYSVIPC)
	/************************************/
	/* calculate some interesting facts */
	/* NOTE: can only do so if value wvalues and rvalues are known */
	/* TODO: Warning, these values probably aren't being calculated correctly -- this */
	/*    will need to match the similar code at the top of vrFprintLock()            */
	/* NOTE: 07/06/2006 -- I can't seem to get a good write-waiting-count for Linux POSIX semaphores (it seems the value of wvalue isn't really correct) */
	lock_held = (wvalue < 1);
	write_held = lock_held && ((plock->readcount < 1) || (rvalue < 1));
	read_held = lock_held && !write_held;
	if (read_held)
		read_procs_holding = plock->readcount;
	else	read_procs_holding = 0;

	procs_waiting = -wvalue;
	read_procs_waiting = -(rvalue - 1);
#  if defined(SEM_SYSVIPC)
	procs_waiting += wncnt;
	read_procs_waiting += rncnt;
#  endif
	write_procs_waiting = procs_waiting - read_procs_waiting + !lock_held;
	/* Test: */ write_procs_waiting = -(wvalue - 1) - (read_procs_waiting > 0) - lock_held;	/* NOTE: the first waiting read also gets counted in this value */
#  if defined(SEM_SYSVIPC)
	write_procs_waiting += wncnt;
#  endif
	/* Test: */ procs_waiting = read_procs_waiting + write_procs_waiting;
#endif


	/**********************************/
	/* put the status into the string */
/******************************************************************/
#if defined(SEM_IRIX) || defined(SEM_POSIX) || defined(SEM_SYSVIPC)
	sprintf(string,
#  if defined(SEM_POSIX) && defined(__linux)	/* put '?' in place of the numbers to indicate that we don't currently know how to determine the numbers. */
		"%1$s (%2$s?r%4$s %5$s?w%7$s)",
#  else
		"%s (%s%dr%s %s%dw%s)",
#  endif
		(lock_held ?
			(read_held ? RED_TEXT "%dread" NORM_TEXT :
				     RED_TEXT "write" NORM_TEXT) :
			"none "),
		(read_procs_waiting > 0 ? RED_TEXT : ""),
		read_procs_waiting,
		(read_procs_waiting > 0 ? NORM_TEXT : ""),
		(write_procs_waiting > 0 ? RED_TEXT : ""),
		write_procs_waiting,
		(write_procs_waiting > 0 ? NORM_TEXT : ""));
	if (read_held)
		sprintf(string, string, read_procs_holding);

if (write_procs_waiting < 0) {
	vrFprintLock(stdout, lock, verbose);
}
#else /* do something simple if mutex values unknown */
	sprintf(string,
		"unknown lock status");
#endif

#if defined(SEM_SYSVIPC)
sprintf(string, " v = %d/%d, n = %d/%d, z = %d/%d", wvalue,rvalue, wncnt,rncnt, wzcnt,rzcnt);
#endif
	return string;
}


/*****************************************************************/
void vrFprintLock(FILE *file, vrLock lock, vrPrintStyle style)
{
	vrPrivateLock	*plock = (vrPrivateLock *)lock;
	int		wvalue;		/* sem-value of the 'w' semaphore (non-negative integer) */
	int		rvalue;		/* sem-value of the 'r' semaphore (non-negative integer) */
#if defined(SEM_SYSVIPC)
	int		wpid, rpid;
	int		wncnt, rncnt;	/* n-count is num procs waiting for sem-value to increase */
	int		wzcnt, rzcnt;	/* z-count is num procs waiting for sem-value to become 0 */
#endif
#if defined(SEM_IRIX) || defined(SEM_POSIX) || defined(SEM_SYSVIPC)
	int		lock_held, read_held, write_held;
	int		read_procs_holding;
	int		procs_waiting;
	int		read_procs_waiting;
	int		write_procs_waiting;
#endif

	/* TODO: print different things for different styles */
	vrTraceOpt("vrFprintLock", "beginning");

	/* if null pointer given, print an empty shell and return */
	if (plock == NULL) {
		vrFprintf(file, "Lock = { <nil> }\n");
		return;
	}

	/****************************************************************/
	/* Get the values of the semaphores based on the defined system */
/***********************/
#if defined(SEM_IRIX)
		wvalue = ustestsema(plock->wmutex);
		rvalue = ustestsema(plock->rmutex);

/***********************/
#elif defined(SEM_POSIX)
		sem_getvalue(plock->wmutex, &wvalue);
		sem_getvalue(plock->rmutex, &rvalue);

/***********************/
#elif defined(SEM_SYSVIPC)
		wvalue = semctl(plock->wmutex_semset, plock->wmutex_semnum, GETVAL);
		wpid   = semctl(plock->wmutex_semset, plock->wmutex_semnum, GETPID);
		wncnt  = semctl(plock->wmutex_semset, plock->wmutex_semnum, GETNCNT);
		wzcnt  = semctl(plock->wmutex_semset, plock->wmutex_semnum, GETZCNT);
		rvalue = semctl(plock->rmutex_semset, plock->rmutex_semnum, GETVAL);
		rpid   = semctl(plock->rmutex_semset, plock->rmutex_semnum, GETPID);
		rncnt  = semctl(plock->rmutex_semset, plock->rmutex_semnum, GETNCNT);
		rzcnt  = semctl(plock->rmutex_semset, plock->rmutex_semnum, GETZCNT);
/***********************/
#elif defined(SEM_WIN32) && 0 /* disabled until the correct code is written */
		/* TODO: write the correct code */

/***********************/
#elif defined(SEM_TCP) && 0 /* disabled until the correct code is written */
		/* TODO: write the correct code */

/***********************/
#else
		wvalue = -1;
		rvalue = -1;
#endif

/******************************************************************/
#if defined(SEM_IRIX) || defined(SEM_POSIX) || defined(SEM_SYSVIPC)
	/************************************/
	/* calculate some interesting facts */
	/* NOTE: can only do so if value wvalues and rvalues are known */
	/* TODO: the calculations for <x>procs_waiting are not validated on all systems. */
	/*   In particular, it seems that on the SGI wvalue is <=0, whereas on Linux it  */
	/*   is >= 0.  So currently, I'm getting negative numbers on Linux systems.      */
	/*   Actually, in checking with the man pages on Linux and SGI, it seems none of */
	/*   them should be returning negative numbers except for errors.                */
	lock_held = (wvalue < 1);
	write_held = lock_held && ((plock->readcount < 1) || (rvalue < 1));
	read_held = lock_held && !write_held;
	if (read_held)
		read_procs_holding = plock->readcount;
	else	read_procs_holding = 0;

	procs_waiting = -wvalue;
	read_procs_waiting = -(rvalue - 1);
#  if defined(SEM_SYSVIPC)
	procs_waiting += wncnt;
	read_procs_waiting += rncnt;
#  endif
	write_procs_waiting = procs_waiting - read_procs_waiting + !lock_held;
	/* Test: */ write_procs_waiting = -(wvalue - 1) - (read_procs_waiting > 0) - lock_held;	/* NOTE: the first waiting read also gets counted in this value */
#  if defined(SEM_SYSVIPC)
	write_procs_waiting += wncnt;
#  endif
	/* Test: */ procs_waiting = read_procs_waiting + write_procs_waiting;
#endif

	/********************/
	/* print the values */
	switch (style) {

	case file_format:
		/* TODO: not yet implemented -- output that can be reread. */
		vrFprintf(file, "TODO: put special version of lock info here\n");
		break;

	case machine:
		/* TODO: not yet implemented -- output easily machine parsed */
		vrFprintf(file,
			"%p:%s:%s:%d:%d\n",
			plock,
			(plock->name == NULL) ? "" : plock->name,
			(lock_held ?  (read_held ? "r" : "w" ) : "n"),
			read_procs_waiting,
			write_procs_waiting);

		break;

	case one_line:
/******************************************************************/
#if defined(SEM_IRIX) || defined(SEM_POSIX) || defined(SEM_SYSVIPC)
		vrTraceOpt("vrFprintLock", "printing one_line");
		vrFprintf(file,
			"Lock at %p, name = '%-20.20s', Status: %s (%s%dr%s %s%dw%s)\n",
			plock,
			(plock->name == NULL) ? "" : plock->name,
			(lock_held ?
				(read_held ? RED_TEXT "read " NORM_TEXT :
					     RED_TEXT "write" NORM_TEXT) :
				"none "),
			(read_procs_waiting > 0 ? RED_TEXT : ""),
			read_procs_waiting,
			(read_procs_waiting > 0 ? NORM_TEXT : ""),
			(write_procs_waiting > 0 ? RED_TEXT : ""),
			write_procs_waiting,
			(write_procs_waiting > 0 ? NORM_TEXT : ""));

if (write_procs_waiting >= 0) {
		break;
}
#else /* do something simple if mutex values unknown */
		vrFprintf(file,
			"Lock at %p, name = '%s'\n",
			plock,
			plock->name);
#endif

	default:
	case verbose:
		vrTraceOpt("vrFprintLock", "printing verbose");
		vrFprintf(file, "{\n");
		vrFprintf(file,
			"\r\tObject_type = %s (%d)\n"
			"\tname = '%s'\n"
			"\ttrace = %d\n"
			"\tnext = %p\n"
			"\treadcount = %d\n"
			"\topcount = %d\n"
			"\ttotal read:  sets = %d releases = %d\n"
			"\ttotal write: sets = %d releases = %d\n",
			vrObjectTypeName(plock->object_type),
			plock->object_type,
#if !defined(__ia64__) && defined(__GNUC__)	/* TODO: This bug appears on the SGI Prisms when using gcc -- so the IA64 specifier may not be 100% correct, but it's the only way I have of inferring that I'm on a Prism system.  Of course, the problem may actually be an Itanium issue in which case this is exactly correct.  (As it happens this is version 3.3.3 of Gcc) */
			(plock->name == NULL) ? "" : plock->name,
#else
			"Unable to report lock name due to gcc bug",
#endif
			plock->trace,
			plock->next,
			plock->readcount,
			plock->opcount,
			plock->total_readsets, plock->total_readrels,
			plock->total_writesets, plock->total_writerels);

/***********************/
#if defined(SEM_IRIX)
		vrFprintf(file,
			"\r\twmutex = %p (IRIX semvalue = %d)\n"
			  "\trmutex = %p (IRIX semvalue = %d)\n",
			plock->wmutex, wvalue,
			plock->rmutex, rvalue);

/***********************/
#elif defined(SEM_POSIX)
		sem_getvalue(plock->wmutex, &wvalue);
		sem_getvalue(plock->rmutex, &rvalue);
		vrFprintf(file,
			"\r\twmutex = %p (POSIX semvalue = %d)\n"
			  "\trmutex = %p (POSIX semvalue = %d)\n",
			plock->wmutex, wvalue,
			plock->rmutex, rvalue);

/***********************/
#elif defined(SEM_SYSVIPC)
		vrFprintf(file,
			"\r\twmutex_semset = %p, semnum = %d (SYSVIPC semvalue = %d, pid = %d, n = %d, z = %d)\n"
			  "\trmutex_semset = %p, semnum = %d (SYSVIPC semvalue = %d, pid = %d, n = %d, z = %d)\n",
			plock->wmutex_semset, plock->wmutex_semnum, wvalue, wpid, wncnt, wzcnt,
			plock->rmutex_semset, plock->rmutex_semnum, rvalue, rpid, rncnt, rzcnt);

/***********************/
#elif defined(SEM_WIN32) && 0 /* disabled until the correct code is written */
		/* TODO: write the correct code */

/***********************/
#elif defined(SEM_TCP) && 0 /* disabled until the correct code is written */
		/* TODO: write the correct code */

/***********************/
#else
		vrFprintf(file,
			"\r\twmutex = %p\n\trmutex = %p\n",
			plock->wmutex,
			plock->rmutex);
#endif

/******************************************************************/
#if defined(SEM_IRIX) || defined(SEM_POSIX) || defined(SEM_SYSVIPC)
		if (lock_held) {
			if (read_held)
				vrFprintf(file, "\r\tStatus: held as " RED_TEXT "%dread" NORM_TEXT ", ", read_procs_holding);
			else	vrFprintf(file, "\r\tStatus: held as " RED_TEXT "write" NORM_TEXT ", ");
			if (read_procs_waiting != 0)
				vrFprintf(file, RED_TEXT "%d" NORM_TEXT " reads waiting, ", read_procs_waiting);
			else	vrFprintf(file, "%d reads waiting, ", read_procs_waiting);
			if (write_procs_waiting != 0)
				vrFprintf(file, RED_TEXT "%d" NORM_TEXT " writes waiting.\n", write_procs_waiting);
			else	vrFprintf(file, "%d writes waiting\n", write_procs_waiting);
		} else {
			vrFprintf(file, "\r\tStatus: no lock held\n");
		}
#endif
		vrFprintf(file, "}\n");

		break;
	}
}


/*****************************************************************/
/* Print not just the lock given, but all the subsequent locks in the linked list */
/* NOTE: we could do some tail recursion here, but to be a little more efficient, */
/*   we'll do the loop right here.                                                */
void vrFprintLockList(FILE *file, vrLock lock, vrPrintStyle style)
{
	vrPrivateLock	*plock = (vrPrivateLock *)lock;
	vrPrivateLock	*next;

	for (next = plock; next != NULL; next = next->next) {
		vrFprintLock(file, next, one_line);
	}
}


/*****************************************************************/
/* Count all the locks in the list from the given lock to the end */
int vrCountLockList(vrLock lock)
{
	vrPrivateLock	*plock = (vrPrivateLock *)lock;
	vrPrivateLock	*next;
	int		count = 0;

	for (next = plock; next != NULL; next = next->next) {
		count++;
	}

	return count;
}




		/*********************************************/
		/*** Code Barriers structure and functions ***/
		/*********************************************/

/************************************************************************/
/*                BARRIERS                                              */
/*                Added by Ed, 11/20/1998                               */
/*                Modified by Bill, 02/28/2003                          */
/************************************************************************/

#define PRINT_FIRST_N_SYNCS	-1

#undef vrTraceOpt
#ifdef VRTRACE_BARRIER
#  define vrTraceOpt		vrTrace
#else
#  define vrTraceOpt(a,b)	;
#endif

/*****************************************************************/
/*
 * Barriers are based on the locks implemented above.  In order
 * to avoid busywaiting, a barrier is created with a dummy lock
 * (barrier_lock) to control synchronization.  This lock is initialized
 * to be writelocked.  See the comment for vrBarrierSync for more
 * details.
 *
 * The context argument is passed for two reasons:
 *      - so each barrier has the ability to access the system context information
 *      - so each barrier will be added to the linked list of barriers for this system
 */
vrBarrier *vrBarrierCreate(vrContextInfo *context, char *name, int num_clients)
{
static	char		string[128];
	vrBarrier	*barrier = (vrBarrier *)vrShmemAlloc0(sizeof(vrBarrier));

#if defined(SEM_SYSVIPC)    && 0 /* I think these are working [3/25/05] */
	vrMsgPrintf("FreeVR: " RED_TEXT "Reminder, barriers based on SYS-V IPC semaphores are not fully functional.\n" NORM_TEXT);
	context->startup_error |= VRSTARTUP_NOBARRSEM;
	vrSleep(1000000);
#endif
	/* set the initial barrier values */
	barrier->object_type = VROBJECT_BARRIER;
	barrier->context = context;
	barrier->num_clients = num_clients;
	barrier->num_waiting = 0;
	barrier->synchronizations = 0;
	barrier->wtime = -1.0;
	barrier->next = NULL;
	snprintf(string, sizeof(string), "barrier %p data lock", barrier);
	barrier->lock = vrLockCreateName(context, string);
	snprintf(string, sizeof(string), "barrier %p barrier lock", barrier);
	barrier->barrier_lock = vrLockCreateName(context, string);
	if (name && *name)
		barrier->name = vrShmemStrDup(name);
	else	barrier->name = "";

#if 0 /* set to 1 to trace the barrier's locks */
	vrLockTrace(barrier->lock, 1);
	vrLockTrace(barrier->barrier_lock, 1);
#endif

	/* This sets up the barrier -- all associated barriers will wait for this lock */
	vrLockWriteSet(barrier->barrier_lock);

	/* add this barrier to a linked list in the context */
	if (context->head_barrier == NULL) {
		/* start the list */
		context->head_barrier = barrier;
		context->tail_barrier = barrier;
	} else {
		/* append to the list */
		/* NOTE: no need to write-lock this because the call to vrBarrierCreate() */
		/*   is already inside a write lock on "context->barrier_lock"            */
		context->tail_barrier->next = barrier;
		context->tail_barrier = barrier;
	}

	vrDbgPrintfN(BARRIER_DBGLVL, "Created Barrier %p, with %d clients\n", barrier, barrier->num_clients);

	return barrier;
}


/************************************************************/
/* Increase the number of clients associated with a barrier */
void vrBarrierIncrement(vrBarrier *barrier, int increment)
{
	vrLockWriteSet(barrier->lock);	/* lock on the data in the barrier's own struct */
	barrier->num_clients += increment;
	vrLockWriteRelease(barrier->lock);

	vrDbgPrintfN(BARRIER_DBGLVL, "Barrier %p incremented to %d clients\n", barrier, barrier->num_clients);
}


/************************************************************/
/* Decrease the number of clients associated with a barrier */
void vrBarrierDecrement(vrBarrier *barrier, int decrement)
{
	vrLockWriteSet(barrier->lock);	/* lock on the data in the barrier's own struct */
	if (barrier->num_clients < decrement) {
		vrErrPrintf("vrBarrierDecrement(): " RED_TEXT "Attempt to decrement barrier %p failed -- not enough clients, has %d, decrement of %d requested" NORM_TEXT, barrier, barrier->num_clients, decrement);
	} else {
		barrier->num_clients -= decrement;
	}
	vrLockWriteRelease(barrier->lock);

	vrDbgPrintfN(BARRIER_DBGLVL, "Barrier %p decremented to %d clients\n", barrier, barrier->num_clients);
}


/*****************************************************************/
/* NOTE: It isn't clear just how appropriate this really is.  */
/*   Freeing a barrier could be a really bad idea.            */
void vrBarrierFree(vrBarrier *barrier)
{
	vrDbgPrintfN(BARRIER_DBGLVL, "Freeing barrier %p with %d clients, %d waiting\n",
		barrier, barrier->num_clients, barrier->num_waiting);

	vrLockWriteSet(barrier->lock);	/* lock on the data in the barrier's own struct */
	vrLockFree(barrier->barrier_lock);
	vrLockFree(barrier->lock);
	vrShmemFree(barrier);
}


/**************************************/
/* Verify that "barrier" is a barrier */
int vrBarrierCheck(vrBarrier *barrier)
{
	vrContextInfo	*context = (vrContextInfo *)vrContext;	/* in the near future, "context" will be an argument */

	if (barrier == NULL)
		return 0;

	if ((void *)barrier < (void *)context)
		vrMsgPrintf("vrBarrierCheck(): Warning, %p is probably an invalid barrier address, something bad may happen next.\n", barrier);

	return (barrier->object_type == VROBJECT_BARRIER);
}


/*****************************************************************/
/*
 * In normal operation, the dummy lock (barrier_lock) in a barrier
 *	is writelocked.
 * Clients wanting to sync will do one of two things:
 *	(a) all but the last client will try to readlock the barrier_lock,
 *		resulting in being suspended until that write-lock is released
 *	(b) the last client will unwritelock the barrier_lock, resulting in
 *		all the processes from (a) being freed.
 *
 * After synching, the current (wall) time is determined and placed in the
 * barrier structure so all processes can set the frame_wtime field in their
 * info structure.  The return value is the place in line this process was
 * in calling the barrier -- the "sync order".
 *
 * If called with a NULL barrier, then the only thing done is to return
 * a value of 0 (which will never be the number of the sync-order from an
 * actual barrier operation).
 */
int vrBarrierSync(vrBarrier *barrier)
{
	int	mynum;		/* the order in which this process hit the barrier */

	/* if the barrier doesn't exist, then just return 0. */
	if (barrier != NULL) {
#ifdef VRTRACE_BARRIER
		vrDbgPrintfN(TRACE_DBGLVL, "(%s::%d) %s -> %s %p at time %f\n", __FILE__, __LINE__, "vrBarrierSync", "syncing on barrier", barrier, (vrCurrentWallTime() - barrier->context->time_immemorial));
#endif
		/* NOTE: the following really should be in a read-lock, but */
		/*   since it's just debugging stuff, we'll let it slide.   */
		if (barrier->synchronizations < PRINT_FIRST_N_SYNCS)
			vrDbgPrintfN(BARRIER_DBGLVL, "Synchronizing on barrier %p with %d clients, %d waiting + one more\n",
				barrier, barrier->num_clients, barrier->num_waiting);
		else if (barrier->synchronizations == PRINT_FIRST_N_SYNCS) {
			vrLockTrace(barrier->lock, 0);
			vrLockTrace(barrier->barrier_lock, 0);
		}

		vrLockWriteSet(barrier->lock);	/* lock on the data in the barrier's own struct */
		barrier->num_waiting++;
		mynum = barrier->num_waiting;
#ifdef VRTRACE_BARRIER
		vrDbgPrintfN(TRACE_DBGLVL, "(%s::%d) %s -> %s, num_waiting = %d, num_clients = %d\n", __FILE__, __LINE__, "vrBarrierSync", "checking", barrier->num_waiting, barrier->num_clients);
#endif

		if (barrier->num_waiting >= barrier->num_clients) {
			/* This is the last process to hit the barrier, so we're synced */

			if (barrier->synchronizations < PRINT_FIRST_N_SYNCS)
				vrDbgPrintfN(BARRIER_DBGLVL, "Barrier %p has all clients, so releasing them.\n", barrier);

#define	BARRIER_DECREMENT	/* 6/3/03: either should work, and I think "undef" is preferred, but not sure */
#ifndef BARRIER_DECREMENT /* 3/5/3 test */
			barrier->num_waiting = 0;
#else
			barrier->num_waiting--;
#endif
			barrier->wtime = vrCurrentWallTime();
			barrier->synchronizations++;
			vrLockWriteRelease(barrier->lock);

#ifdef VRTRACE_BARRIER
			vrDbgPrintfN(TRACE_DBGLVL, "(%s::%d) %s -> %s %f\n", __FILE__, __LINE__, "vrBarrierSync", "freeing at time", (barrier->wtime - barrier->context->time_immemorial));
#endif
			vrLockWriteRelease(barrier->barrier_lock);	/* this frees all the waiting processes */
			vrLockWriteSet(barrier->barrier_lock);	/* this sets the barrier for the next pass */
		} else {
			/* We still need to wait for some other processes to check in, so wait */
			vrLockWriteRelease(barrier->lock);

#ifdef VRTRACE_BARRIER
			vrDbgPrintfN(TRACE_DBGLVL, "(%s::%d) %s -> %s\n", __FILE__, __LINE__, "vrBarrierSync", "waiting, using read-lock on barrier lock");
#endif
			vrLockReadSet(barrier->barrier_lock);		/* this is where the waiting is done */
#ifdef VRTRACE_BARRIER
			vrDbgPrintfN(TRACE_DBGLVL, "(%s::%d) %s -> %s\n", __FILE__, __LINE__, "vrBarrierSync", "done waiting");
#endif
#ifdef BARRIER_DECREMENT
			vrLockWriteSet(barrier->lock);
			barrier->num_waiting--;
			vrLockWriteRelease(barrier->lock);
#endif
			vrLockReadRelease(barrier->barrier_lock);	/* we really just wanted the blocking side-effect of the lock, so release it immediately */
		}

#if 1
		/* TODO: 2/28/03 -- I don't know what these two lines are all about */
		vrLockReadSet(barrier->lock);
		vrLockReadRelease(barrier->lock);
#endif
	} else {
#ifdef VRTRACE_BARRIER
		vrDbgPrintfN(TRACE_DBGLVL, "(%s::%d) %s -> %s\n", __FILE__, __LINE__, "vrBarrierSync", "Null barrier -- no work to do");
#endif
		vrDbgPrintfN(BARRIER_DBGLVL+100, "NULL Barrier %p so just returning 0.\n", barrier);
		mynum = 0;
	}

#ifdef VRTRACE_BARRIER
	vrDbgPrintfN(TRACE_DBGLVL, "(%s::%d) %s -> %s\n", __FILE__, __LINE__, "vrBarrierSync", "ending");
#endif
	return mynum;
}


/*****************************************************************/
/* This function returns true when all the other processes are */
/*   already waiting at this barrier, and thus the calling     */
/*   process will be the last to sync -- but might first want  */
/*   to do a little extra work just before synchronization.    */
int vrBarrierLastToSync(vrBarrier *barrier)
{
	int	last;

	if (barrier == NULL)
		return -1;

	vrLockReadSet(barrier->lock);

	last = barrier->num_waiting + 1 == barrier->num_clients;

	vrLockReadRelease(barrier->lock);

	if (barrier->synchronizations < PRINT_FIRST_N_SYNCS)
		vrDbgPrintfN(BARRIER_DBGLVL, "Barrier %p returning last-to-sync %d.\n", barrier, last);
	return last;
}


/*************************************************************/
/* This function returns true when no other processes are    */
/*   currently waiting at this barrier, and thus the calling */
/*   process will be the first to sync -- perhaps it might   */
/*   first want to do a little extra work just before        */
/*   synchronization.                                        */
int vrBarrierFirstToSync(vrBarrier *barrier)
{
	int	first;

	if (barrier == NULL)
		return -1;

	vrLockReadSet(barrier->lock);

	first = (barrier->num_waiting == 0);

	vrLockReadRelease(barrier->lock);

	if (barrier->synchronizations < PRINT_FIRST_N_SYNCS)
		vrDbgPrintfN(BARRIER_DBGLVL, "Barrier %p returning first-to-sync %d.\n", barrier, first);
	return first;
}


/*****************************************************************/
/* NOTE: this function is primarily for debugging purposes, to allow */
/*   waiting processes to continue, and then see what happens next.  */
void vrBarrierReleaseOnce(vrBarrier *barrier)
{
	/* make sure we're given a good barrier before doing anything */
	if (barrier == NULL) {
		vrPrintf("vrBarrierReleaseOnce(): NULL address given.\n");
		return;
	}

	if (barrier->object_type != VROBJECT_BARRIER) {
		vrPrintf("vrBarrierReleaseOnce(): address given is type %d, not a barrier.\n", barrier->object_type);
		return;
	}

	/* now release the barrier */
	vrLockWriteSet(barrier->lock);	/* lock on the data in the barrier's own struct */

	barrier->num_waiting = 0;
	/* NOTE: we won't increment "synchronizations" since we didn't really sync this time */
	vrLockWriteRelease(barrier->lock);

	vrLockWriteRelease(barrier->barrier_lock);	/* this frees all the waiting processes */
/* TODO: 3/9/2005 -- what are these two lines all about?  I.e. why aren't they indented, were they for testing? */
vrLockReadSet(barrier->lock);
vrLockReadRelease(barrier->lock);
	vrLockWriteSet(barrier->barrier_lock);	/* this sets the barrier for the next pass */

	return;
}


/*****************************************************************/
void vrBarrierReleaseAll(vrContextInfo *context)
{
	vrBarrier	*barrier;

	vrDbgPrintfN(BARRIER_DBGLVL, "Releasing all barriers.\n");

	for (barrier = context->head_barrier; barrier != NULL; barrier = barrier->next) {
		vrLockWriteSet(barrier->lock);
		barrier->num_clients = 0;			/* with 0 clients, this barrier will never again bar */
		vrLockWriteRelease(barrier->barrier_lock);	/* this frees all the waiting processes */
		/* NOTE: we also don't set the write-lock on barrier_lock again */
		/*   to prevent this barrier from doing any barriering.         */

		vrLockWriteRelease(barrier->lock);
		/* NOTE: we can't Free the memory for this barrier inside the loop */
		/*   because we need the next pointer.                             */
	}
	vrDbgPrintfN(BARRIER_DBGLVL, "All barriers released.\n");
}


/*****************************************************************/
/* NOTE: we currently ignore the barrier's own lock -- we may want */
/*   to set the lock for reading, although considering this is     */
/*   just a debugging aid, it probably isn't necessary.            */
void vrFprintBarrier(FILE *file, vrBarrier *barrier, vrPrintStyle style)
{
	/* TODO: print different things for different styles */

	/* if null pointer given, print an empty shell and return */
	if (barrier == NULL) {
		vrFprintf(file, "Barrier = { <nil> }\n");
		return;
	}

	switch (style) {

	case file_format:
		/* TODO: not yet implemented -- output that can be reread. */
		vrFprintf(file, "TODO: put special version of barrier info here\n");
		break;

	case machine:
		/* TODO: not yet implemented -- output easily machine parsed */
		vrFprintf(file, "TODO: put special version of barrier info here\n");
		break;

	case brief:
	case one_line:
		vrFprintf(file,
			"Barrier at %p, name = '%-24.24s', %d clients, %d syncs, Status: %d waiting, first? %d, last? %d\n",
			barrier,
			barrier->name,
			barrier->num_clients,
			barrier->synchronizations,
			barrier->num_waiting,
			(barrier->num_waiting == 0),
			barrier->num_waiting + 1 == barrier->num_clients);

		break;

	default:
	case verbose:
		vrFprintf(file, "{\n");
		vrFprintf(file,
			"\r\tObject_type = %s (%d)\n"
			"\tname = '%s'\n"
			"\tnext = %p\n",
			vrObjectTypeName(barrier->object_type),
			barrier->object_type,
			barrier->name,
			barrier->next);
		vrFprintf(file,
			"\r\tnum_clients = %d\n\tnum_waiting = %d\n\tsynchronizations = %d\n",
			barrier->num_clients,
			barrier->num_waiting,
			barrier->synchronizations);
		vrFprintf(file,
			"\r\twtime = %.3lf\n\tlock = %p\n\tbarrier_lock = %p\n",
			(double)barrier->wtime,
			barrier->lock,
			barrier->barrier_lock);
		vrFprintf(file, "}\n");

		break;
	}
}


/*****************************************************************/
/* Print not just the barrier given, but all the subsequent barriers in the linked list */
/* NOTE: we could do some tail recursion here, but to be a little more efficient, */
/*   we'll do the loop right here.                                                */
void vrFprintBarrierList(FILE *file, vrBarrier *barrier, vrPrintStyle style)
{
	vrBarrier	*next;

	for (next = barrier; next != NULL; next = next->next) {
		if (style == machine) {
			vrFprintf(file, "%p:", next);
		} else {
			vrFprintBarrier(file, next, one_line);
		}
	}
	if (style == machine) {
		vrFprintf(file, "\n");
	}
}

/*****************************************************************/
/* Count all the barriers in the list from the given lock to the end */
int vrCountBarrierList(vrBarrier *barrier)
{
	vrBarrier	*next;
	int		count = 0;

	for (next = barrier; next != NULL; next = next->next) {
		count++;
	}

	return count;
}


#ifdef SEM_WIN32 /* { */
/*****************************************************************/
static void _WaitForSemaphore(HANDLE handle)
{
	if (WaitForSingleObject(handle, INFINITE) == WAIT_FAILED) {
		LPVOID	lpMsgBuf;

		if (!FormatMessage(
					FORMAT_MESSAGE_ALLOCATE_BUFFER |
					FORMAT_MESSAGE_FROM_SYSTEM |
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					GetLastError(),
					MAKELANGID(LANG_NEUTRAL,
						SUBLANG_DEFAULT),
					(LPTSTR) &lpMsgBuf,
					0,
					NULL)) {
			vrErr("FormatMessage() failed");
			vrErrPrintf("Waiting failed for lock (%p)\n", (void *)handle);
		} else {
			vrErrPrintf("Waiting failed for lock (%p)\n"
					"GetLastError() = %s\n",
					(void *)handle,
					(LPCTSTR)lpMsgBuf);
			LocalFree(lpMsgBuf);
		}
	}
}
#endif /* } */



#if !defined(SEM_WIN32) && !defined(SEM_TCP) /* { */
/* The following functions break the CYGWIN build */

/* TODO: these functions should probably be moved to another file */

/*****************************************************************/
/*===============================================================*/
/*****************************************************************/
/* The following routines are for key-based shared memory access.*/


#include <sys/types.h>
#if !(defined(SHM_SYSVIPC) || defined(SEM_SYSVIPC)) /* ie. if not already included */
#  include <sys/ipc.h>			/* IRIX 6.x includes this in sys/shm.h, but not 5.3 */
#  include <sys/shm.h>
#endif


#define PERMS 0666


/*************************************************************************
 int vrShmemGetKey()
	Gets a shared memory segment -- creates if necessary.
**************************************************************************/
int vrShmemGetKey(key_t shmem_key, int memsize, void **memptr, int create)
{
	int	shmid;
unsigned int	sh_flags;
	void	*memory;

	if (create)
		sh_flags = PERMS | IPC_CREAT;
	else	sh_flags = 0x0;

printf("memsize is %d flags is 0x%x\n", memsize, sh_flags);
	shmid = shmget(shmem_key, memsize, sh_flags);
	if (shmid < 0) {
		/* TODO: need to check for cause of failure -- e.g. permissions, ... */
		if (create) {
			vrErrPrintf("vrShmemGetKey(): " RED_TEXT "Can't get shared memory: key %d\n" NORM_TEXT, shmem_key);
			perror("vrShmemGetKey() response from shmget");
#if 0
			exit(-1);	/* TODO: this seems a bit harsh -- let the program decide */
#else
			return 0;
#endif
		} else {
			*memptr = NULL;
			return shmid;
		}
	}

	memory = (void *)shmat(shmid, (char *)0, 0);
	if (memory == (void *)-1) {
		vrErrPrintf("vrShmemGetKey(): " RED_TEXT "Can't attach shared memory: key %d\n" NORM_TEXT, shmem_key);
		perror("vrShmemGetKey() response from shmat");
#if 0
		exit(-1);	/* TODO: this seems a bit harsh -- let the program decide */
#else
		return 0;
#endif
	}
	*memptr = memory;

	/* TODO: it might be wise to print a warning message if the requested */
	/*   memory size doesn't match the memory size of the given segment.  */
	/* TODO: for debugging it might be nice to have a vrShmemPrintId()  */
	/*   function that prints the data associated with a given segment. */

	return shmid;
}


/*************************************************************************/
int vrShmemIdGetSize(int shmid)
{
	struct shmid_ds		shm_data;	/* structure with info about shared memory segment */
	int			ctl;		/* for interfacing with shared memory controls     */

	/* get the statistics on the shared memory segment */
	errno = 0;
	ctl = shmctl(shmid, IPC_STAT, &shm_data);

	if (ctl < 0) {
		/* print warnings/error messages for possible errors */
		switch (errno) {
		case EACCES:	/* permission denied */
			vrErrPrintf("vrShmemIdGetSize(): " RED_TEXT "ERROR: permission denied to shared memory id %d\n" NORM_TEXT, shmid);
			break;

		case EINVAL:	/* shmid is not valid */
			vrErrPrintf("vrShmemIdGetSize(): " RED_TEXT "ERROR: invalid shared memory id %d\n" NORM_TEXT, shmid);
			break;

#if defined(EOVERFLOW)
		case EOVERFLOW:	/* status cannot be stored */
			vrErrPrintf("vrShmemIdGetSize(): " RED_TEXT "ERROR: overflow error -- status cannot be acquired for shmid %d\n" NORM_TEXT, shmid);
			break;
#endif

		case EFAULT:	/* illegal data address (ie. of shm_data, which shouldn't happen) */
			vrErrPrintf("vrShmemIdGetSize(): " RED_TEXT "ERROR: illegal data address while reading shmid %d\n" NORM_TEXT, shmid);
			break;
		}

		return -1;
	}

	return shm_data.shm_segsz;
}


/*************************************************************************
 void vrShmemDetach()
	Get the shared memory id and remove it.
**************************************************************************/
void vrShmemDetach(void *memory_address)
{
	if (shmdt(memory_address) < 0)
		perror("vrShmemDetach(): Detaching shared memory");
}


/*************************************************************************
 void vrShmemRemoveKey()
	Get the shared memory id and remove it.
**************************************************************************/
void vrShmemRemoveKey(key_t shmem_key)
{
	int	shmid;

	shmid = shmget(shmem_key, 0, 0x0);
	if (shmid > -1) {
		/* only Linux needs the third argument to shmctl */
		if (shmctl(shmid, IPC_RMID, (struct shmid_ds *)NULL) < 0)
			perror("vrShmemRemoveKey(): Removing shmid");
	}
}


/*************************************************************************
 void vrShmemRemoveId()
	Remove shared memory by id.
**************************************************************************/
void vrShmemRemoveId(int shmid)
{
	if (shmid > -1) {
		if (shmctl(shmid, IPC_RMID, (struct shmid_ds *)NULL) < 0)
			perror("vrShmemRemoveId(): Removing shmid");
	}
}

#endif /* } */

