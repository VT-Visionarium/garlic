/* A dummy c-file containing stubs to compile the library  */
/*   on a Linux system.  And, within certain restrictions, */
/*   run the compiled application on a Linux system using  */
/*   a Linux implementation of shared memory.              */
/*                                                         */
/* NOTE: this code is unnecessary on systems with a real   */
/*   shared memory implementation, or when using pthreads. */

/* Copyright 2014, Bill Sherman & Friends, All rights reserved.        */
/* With the intent to provide an open-source license to be named later.*/


/************************* toy-amalloc.c *************************/
/* Written (in about 15 minutes) by Stuart Levy 8/4/98           */
/* - mimics the API of general shared memory allocation routines */
/* - doesn't free memory, so requires a very large space to work */
/*   in.                                                         */
/*                                                               */
/* 1/6/99 Bill added amallocblksize().                           */
/* 2/4/02 Bill added two lines to afree().                       */
/* 08/18/2005 Bill renamed to vr_shmem.dummy.c                   */

#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>

/*
 * Code is now updated to allocate memory blocks which are
 * padded up to the next ALIGNSZ alignment, so that machines
 * (such as RISC boxes like SGI, Sun, etc) that do not support
 * unaligned memory accesses will be happy with the blocks of
 * memory that we return.  An ALIGNSZ of 16 ought to be more than
 * large enough for any machine we're going to be running on during
 * the time that this code still exists.
 */
#define ALIGNSZ 16

/*********************************************************/
/* Enough stubs to satisfy the freeVR library, for now.  */
static struct arena {
		int	room;
		int	avail;
		char	*last_avail;
		int	last_blocksize;
		char	*last_avail_butone;
		int	last_blocksize_butone;
	} *toy_arena;


/******************************************************/
void *usinit(const char *filename ) { return NULL; }

/******************************************************/
void *acreate(void *addr, size_t len, int flags, void *ushdr, void *(*grow)(size_t, void *))
{
        int	sa, sz;

	toy_arena = (struct arena *)addr;
	toy_arena->room = len;

        /* must align addresses to word boundary, ALIGNSZ-byte alignment   */
        /* ought to be more than adequate for all platforms we run on */
        sz = sizeof(struct arena);    		/* real size */
        sa = ((sz+(ALIGNSZ-1)) & ~(ALIGNSZ-1)); /* align to next boundary */
	toy_arena->avail = sa;        		/* use aligned size */

        return toy_arena;
}


/******************************************************/
void *amalloc(size_t size, struct arena *arena)
{
        size_t	sa;				/* aligned size */
	char	*p = (char *)(arena) + arena->avail;

        sa = ((size+(ALIGNSZ-1)) & ~(ALIGNSZ-1)); /* align to next boundary */
	if(arena->avail + sa > arena->room) {
		fprintf(stderr, RED_TEXT "Not enough room to amalloc(%d)!\n" NORM_TEXT, size);
		return NULL;
	}
#if 0
	fprintf(stderr, RED_TEXT "amallocing(%d), %d left -- address = %p.\n" NORM_TEXT, size, (arena->room - arena->avail), p);
#endif
	arena->last_avail_butone = arena->last_avail;
	arena->last_blocksize_butone = arena->last_blocksize;
	arena->last_avail = p;
	arena->last_blocksize = sa;
	arena->avail += sa;
	return p;
}


/******************************************************/
void afree(void *p, struct arena *arena)
{
	int	size_freed = 0;

#if 1 /* include this to back off the used memory if deleted last allocated */
	/* if we're just freeing the last thing allocated then */
	/*   simply step back on the pointers.                 */
	if (p == arena->last_avail) {
		size_freed = arena->last_blocksize;
		arena->avail -= arena->last_blocksize;

		arena->last_avail = arena->last_avail_butone;
		arena->last_blocksize = arena->last_blocksize_butone;

		arena->last_avail_butone = NULL;
		arena->last_blocksize_butone = 0;
	}
#endif
#if 0
	fprintf(stderr, RED_TEXT "freed(%d), %d left -- address = %p.\n" NORM_TEXT, size_freed, (arena->room - arena->avail), p);
#endif
}


/******************************************************/
void adelete(struct arena *arena) {}


/******************************************************/
void *arealloc(void *old, size_t size, struct arena *arena)
{
	char	*p = amalloc(size, arena);

	if(p == NULL)
		return NULL;

	memcpy(p, old, size);
	return p;
}


/**************************************************************/
/* check if the pointer is from the last allocation.  If so,  */
/*   then we know the size of the last allocation, and we can */
/*   return it.  Otherwise we're screwed.                     */
long amallocblksize(void *p, struct arena *arena)
{
	long	assumption = 64;	/* if indeterminate, assume 64. Usually correct. */

	if (p == arena->last_avail)
		return arena->last_blocksize;
	if (p == arena->last_avail_butone)
		return arena->last_blocksize_butone;

#if 0
	vrErrPrintf(RED_TEXT "unable to determine size of block %p\n" NORM_TEXT, p);
#endif

	return assumption;
}



/* ========================================================================== */
/* ========================================================================== */


/************************* toy-locks.c **************************/
/* stubs for locking/semaphore stuff for Linux version.         */
/* TODO: implement this -- since we now have locks implemented. */
/*   we're not really using the locking stuff yet, we don't yet */


/******************************************************/
int uspsema (int* sema) { return 0; }


/******************************************************/
int usvsema (usema_t *sema) { return 0; }


/******************************************************/
void usdetach (usptr_t *u) {}

