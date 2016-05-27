/* ======================================================================
 *
 * HH   HH         vr_shmem.h
 * HH   HH         Author(s): Ed Peters, Bill Sherman
 * HHHHHHH         Created: June 4, 1998
 * HH   HH         Last Modified: May 6, 2005
 * HH   HH
 *
 * Header file for FreeVR shared memory.  Defines:
 *	- shmem init and generic allocation functions
 *	- type-specific shmem allocation functions (strings, int ary, etc.)
 *	- locking routines
 *	- barrier routines
 *
 * Copyright 2005, Bill Sherman & Friends, All Rights Reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#ifndef __VRSHMEM_H__
#define __VRSHMEM_H__

#include <stdlib.h>
#include <stdarg.h>

#include "vr_enums.h"
#include "vr_math.h"

#if defined(SHM_NONE)
#  define USE_SHMEM 0
#else
#  define USE_SHMEM 1
#endif

#ifdef __cplusplus
extern "C" {
#endif


		/*********************************************/
		/*** Shared Memory structure and functions ***/
		/*********************************************/

#ifdef VRSHMEM_USESTRUCT /* { */
/*************************************************************************/
/* A structure that contains all the information about the shared memory */
/*   system.  This allows all the shared memory info to be contained in  */
/*   one block that the vrContext can point to.                          */
typedef struct vrShmemInfo_st {
#ifdef VRTRACKMEM
		long		total_bytes_this_proc = 0;
		long		*total_bytes_overall = NULL;
		long		*total_freed_overall = NULL;
#endif
		long		total_bytes_too_many = 0;
		long		*arena_size = NULL;

#if defined(SHM_SVR4MMAP)
		long		shmem_fd = -1;
#endif

		void		*shmem_addr = NULL;
		size_t		abytes = 0;
		usptr_t		*usarena = NULL;
		void		*arena = NULL;

#if defined(SHM_SYSVIPC)
		int		shmid=0;
		int		shmkey=0;
		struct shmid_ds	shminfo;
#endif

#if defined(SEM_SYSVIPC)
		int		sysvsem_currentarray;		/* initialize to -1 */
		int		sysvsem_next_semnum;		/* initialize to  0 */
#endif

	} vrShmemInfo;
#endif /* } */


/****************************************************************************/
/* Generic functions to initialize arena (must be done before forking       */
/* off processes), allocate and free storage and cleanup on exit:           */
/*	- only the first call to vrShmemInit will have any effect           */
/*	- vrShmemAlloc0 will clear the buffer to zero for you (using memset)*/
/*	- arena ptr etc. are stored in static (file-scope) variables        */
/****************************************************************************/
long	vrShmemInit(size_t bytes);
void	*vrShmemAlloc(size_t bytes);
void	*vrShmemAlloc0(size_t bytes);
void	*vrShmemRealloc(void *data, size_t bytes);
void	vrShmemFree(void *data);
void	vrShmemExit();
void	*vrShmemArena();
long	vrShmemUsage();
long	vrShmemFreed();


/******************************************************************/
/* Type-specific shortcuts for shared memory allocation.  These   */
/* are mostly for programmer convenience.  They don't go through  */
/* vrShmemAlloc so they might be a little faster than doing it    */
/* by hand (ie. allocating the memory and then copying the data). */
/* For example:                                                   */
/*	char	*sh_string = vrShmemStrDup("foobar");             */
/*	int	*sh_ints = vrShmemIntArray(2, 0, 1);              */
/*	float	*sh_floats = vrShmemFloatArray(3, 1.0, 2.0, 3.0); */
/*	void	*sh_stuff[] = vrShmemPtrArray(3, sh_string, sh_ints, sh_floats); */
/******************************************************************/
char	*vrShmemStrDup(char *s);
char	*vrShmemStrCat(char *s1, char *s2);
void	*vrShmemMemDup(void *mem, int size);
int	*vrShmemIntArray(int count,...);
float	*vrShmemFloatArray(int count,...);
void	**vrShmemPtrArray(int count,...);


/**************************************/
/* Shared memory key access functions */
#if !defined(SEM_WIN32) && !defined(SEM_TCP) /* { */
#include <sys/types.h>		/* for key_t type */

int	vrShmemGetKey(key_t shmem_key, int memsize, void **memptr, int create);
int	vrShmemIdGetSize(int shmid);
void	vrShmemDetach(void *memory_address);
void	vrShmemRemoveKey(key_t shmem_key);
void	vrShmemRemoveId(int shmid);
#endif


		/********************************************/
		/*** Data Locking structure and functions ***/
		/********************************************/

/*********************************************************************/
/* The basic lock type is defined as a void pointer in this (public) */
/* header file, and will be cast into a structure inside the locking */
/* routines.  FreeVR users don't need to know what's in a lock.      */
/*                                                                   */
/* A lock will allow multiple readers at a time, or one writer at a  */
/* time, but NOT both.  If someone is busy reading, other readers    */
/* can get through but no writers.  If someone is busy writing, all  */
/* others (readers OR writers) have to wait.                         */
/*********************************************************************/
typedef void	*vrLock;

#if 0 /* this is going away */
vrLock	vrNCLockCreate();
#endif
#if 0
vrLock	vrLockCreate(struct vrContextInfo_st *context);
vrLock	vrLockCreateName(struct vrContextInfo_st *context, char *name);
vrLock	vrLockCreateTraced(struct vrContextInfo_st *context, char *name);
#else /* Okay, I have no idea why the above doesn't work, considering it works fine for Barriers */
vrLock	vrLockCreate(void *context);
vrLock	vrLockCreateName(void *context, char *name);
vrLock	vrLockCreateTraced(void *context, char *name);
#endif
void	vrLockFree(vrLock lock);
void	vrLockFreeAll(vrLock lock);
int	vrLockCheck(vrLock lock);
void	vrLockTrace(vrLock lock, int trace);

void	vrLockReadSet(vrLock lock);
void	vrLockReadRelease(vrLock lock);
void	vrLockReadToWrite(vrLock lock);
void	vrLockWriteSet(vrLock lock);
void	vrLockWriteRelease(vrLock lock);
char	*vrLockStatus(vrLock lock, char *string);
void	vrFprintLock(FILE *file, vrLock lock, vrPrintStyle style);
void	vrFprintLockList(FILE *file, vrLock lock, vrPrintStyle style);
int	vrCountLockList(vrLock lock);



		/*********************************************/
		/*** Code Barriers structure and functions ***/
		/*********************************************/

/***************************************************************/
/* Barriers use locks (defined above) and are also intimately  */
/* related to time.  There is a set of barriers for use by the */
/* system (one for each process group defined in the FreeVR    */
/* configuration), but you can also create your own barriers   */
/* for use in non-library processes.                           */
/*                                                             */
/* A group of processes that share a barrier will also share   */
/* a notion of time.  One of the actions in vrBarrierSync is   */
/* to set the current simulation time in the 'time' field of   */
/* the barrier struct.                                         */
/***************************************************************/

/* NOTE: we probably could also make the "public" barrier type a (void *) like vrLock */
typedef struct vrBarrier_st {
		/***************************/
		/* Generic Object field(s) */
		vrObject	object_type;	/* The type of this object (ie VROBJECT_BARRIER) */
		/* NOTE: the rest of the generic object fields not included with this structure  */
		/* TODO: consider adding the other object fields to this structure.              */

		/***************************/
		/* specific barrier fields */
	struct vrContextInfo_st	*context;	/* pointer to the vrContext memory structure */
		char		*name;		/* name to use with debugging output */

		vrTime		wtime;		/* shared wall-time for all procs using this barrier */
		int		num_clients;	/* total number of clients for this barrier */
		int		num_waiting;	/* number of clients currently waiting to sync */
		int		synchronizations;/* # of times this barrier synchronized */
		vrLock		lock;		/* used to control write access to this struct */
		vrLock		barrier_lock;	/* used to control synching */

	struct	vrBarrier_st	*next;		/* next barrier in the list of all barriers */

	} vrBarrier;

vrBarrier	*vrBarrierCreate(struct vrContextInfo_st *context, char *name, int num_clients);
void		vrBarrierIncrement(vrBarrier *barrier, int increment);
void		vrBarrierDecrement(vrBarrier *barrier, int decrement);
void		vrBarrierFree(vrBarrier *barrier);
int		vrBarrierCheck(vrBarrier *barrier);
int		vrBarrierSync(vrBarrier *barrier);
int		vrBarrierLastToSync(vrBarrier *barrier);
int		vrBarrierFirstToSync(vrBarrier *barrier);
void		vrBarrierReleaseOnce(vrBarrier *barrier);
void		vrBarrierReleaseAll(struct vrContextInfo_st *context);
void		vrFprintBarrier(FILE *file, vrBarrier *barrier, vrPrintStyle style);
void		vrFprintBarrierList(FILE *file, vrBarrier *barrier, vrPrintStyle style);
int		vrCountBarrierList(vrBarrier *barrier);


#ifdef __cplusplus
}
#endif

#endif /* __VRSHMEM_H__ */
