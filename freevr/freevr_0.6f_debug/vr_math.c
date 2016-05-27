/* ======================================================================
 *
 *  CCCCC          vr_math.c
 * CC   CC         Author(s): Ed Peters, Stuart Levy, Bill Sherman, John Stone
 * CC              Created: (probably) June 4, 1998
 * CC   CC         Last Modified: February 8, 2013
 *  CCCCC
 *
 * Source file for FreeVR math routines.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <sys/time.h>
#include <memory.h>
#include <unistd.h>  /* needed by getpid() */

#if !defined(FREEVR) && !defined(TEST_APP) && !defined(MATH_ONLY)
#  define	FREEVR
#  undef	TEST_APP
#endif

#include "vr_math.h"

#if defined(FREEVR)
#  include "vr_debug.h"
#  include "vr_shmem.h"
#  include "vr_system.h"
#endif

#if defined(TEST_APP) || defined(MATH_ONLY) /* { */
/* declare some alternate and dummy functions, macros and structures to   */
/*   allow the "mathtest" application to be compiled with out complaints. */

struct temp_struct {
		vrTime	time_immemorial;
		vrTime	paused_time;
		vrTime	pause_wtime;
	} *vrContext;

void *vrShmemAlloc0(size_t size)
{
	void *buf = malloc(size);
	memset(buf, 0, size);
	return buf;
}
void vrShmemFree(void *buf) { free(buf); }

#  define	MATH_DBGLVL	260
#  define	vrDbgDo(n)	1
#  define	vrPrintf	printf
#  define	vrFprintf	fprintf
#  define	vrDbgPrintf	printf
#  define	AltvrDbgPrintfN(a,b,c,d,e,f,g,h,i,j,k)	/* print nothing (for different compilers, different forms) */;
#  define	vrDbgPrintfN(...)			/* print nothing (for different compilers, different forms) */;
#  define	vrMsgPrintf	printf
#  define	vrErrPrintf	printf
#  define	vrErr(s)	{ printf(s); printf("\n"); }

#  define	NORM_TEXT	"[0m"
#  define	BOLD_TEXT	"[1m"
#  define	RED_TEXT	"[31m"
#  define	SET_TEXT	"[%dm"

#endif /* } TEST_APP || MATH_ONLY */


/*****************************************************************/
/*                       MISC routines                           */
/*****************************************************************/

/*********************************************************************/
/* NOTE: needs to skip leading whitespace to function like atoi(),   */
/*   and make the test for other bases (eg. "0x") work properly.     */
long vrAtoI(char *str)
{
static	char	*whitespace = " \t\r\b\n";
	int	sign = 1;

	str += strspn(str, whitespace);		/* skip leading whitespace */

	/* check for minus sign */
	if (strlen(str) > 1) {
		if (str[0] == '-') {
			sign = -1;		/* set the sign to negative */
			str++;			/* now skip the minus sign */
		}
	}

	/* check for radix */
	if (strlen(str) > 2) {
		if (str[0] == '0') {
			switch(str[1]) {
			case 'b':
				return (sign * strtol(&str[2], (char **)NULL, 2));
			case 'o':
				return (sign * strtol(&str[2], (char **)NULL, 8));
			case 'd':
				return (sign * strtol(&str[2], (char **)NULL, 10));
			case 'x':
				return (sign * strtol(&str[2], (char **)NULL, 16));
			default:
				return (sign * strtol(str, (char **)NULL, 0));
			}
		}
	}

	/* otherwise return with the default strtol() behavior */
	return (sign * strtol(str, (char **)NULL, 0));
}


/*********************************************************************/
/* This version returns an integer to be used as a pointer -- which */
/*   may require larger integers.                                   */
/* NOTE: on Linux, strtol() works on both 32 and 64 bit architectures */
/*   the compiler does the right thing.  I'm not sure whether this is */
/*   true for other machines -- I was using strtoll(), and I suspect  */
/*   that was for a good reason.                                      */
/*   And my notes from 03/13/03 indicate that it was tests on a big   */
/*   Sun machine that required strtoll(), or it would truncate the    */
/*   number -- now, this stuff only matters when entering addresses   */
/*   via the telnet interface.                                        */
void *vrAtoP(char *str)
{
static	char	*whitespace = " \t\r\b\n";

	str += strspn(str, whitespace);		/* skip leading whitespace */

	if (strlen(str) > 2) {
		if (str[0] == '0') {
			switch(str[1]) {
			case 'b':
				return ((void *)strtol(&str[2], (char **)NULL, 2));
			case 'o':
				return ((void *)strtol(&str[2], (char **)NULL, 8));
			case 'd':
				return ((void *)strtol(&str[2], (char **)NULL, 10));
			case 'x':
				return ((void *)strtol(&str[2], (char **)NULL, 16));
			default:
				return ((void *)strtol(str, (char **)NULL, 0));
			}
		} else {
			return ((void *)strtol(str, (char **)NULL, 0));
		}
	} else {
		return ((void *)strtol(str, (char **)NULL, 0));
	}
}


/*************************************************************************/
/* vrBinaryToString(): takes an integer, a number of bits, and a pointer */
/*   to memory in which to store the result.  The resultant string       */
/*   is placed in the memory, and returned.  The string is made up       */
/*   of the least significant N bits, based on the value of len.         */
/*************************************************************************/
char *vrBinaryToString(unsigned int val, int len, char *str)
{
	int     count;

	str[len] = '\0';
	for (count = 0; count < len; count++) {
		str[len-count-1] = (((1 << count) & val) ? '1' : '0');
	}
	return (str);
}



/*****************************************************************/
/*                       TIME routines                           */
/*****************************************************************/

/*********************************************************************/
vrTime vrStartTime()
{
	if (vrContext == NULL)
		return 0.0;

	return vrContext->time_immemorial;
}


/*********************************************************************/
vrTime vrCurrentWallTime()
{
	vrTime		return_now;
#if 1
	struct timeval	now;

	gettimeofday(&now, NULL);
	return_now = ((vrTime)now.tv_sec + (vrTime)now.tv_usec / 1000000.0);
#else
	struct timespec	now;

	clock_gettime(CLOCK_SGI_CYCLE, &now);
	return_now = ((vrTime)now.tv_sec + (vrTime)now.tv_nsec / 1000000000.0);
#endif

	return (return_now);
}


/*********************************************************************/
vrTime vrSimTimeOf(vrTime wtime)
{
	if (vrContext->pause_wtime > 0.0)
		return (vrContext->pause_wtime - vrStartTime() - vrContext->paused_time);
	else	return (                 wtime - vrStartTime() - vrContext->paused_time);
}


/*********************************************************************/
vrTime vrCurrentSimTime()
{
	return vrSimTimeOf(vrCurrentWallTime());
}


/*********************************************************************/
vrTime vrTimeSince(vrTime then)
{
	return vrCurrentWallTime() - then;
}



/* ==================================================== */
/* MATRIX OPERATIONS                                    */
/* ==================================================== */


/**************************************/
/****** Matrix Creation Routines ******/
/**************************************/

/*********************************************************************/
/* create an empty (all zeros) 4x4 matrix                            */
vrMatrix *vrMatrixCreate(void)
{
	return (vrMatrix *)vrShmemAlloc0(sizeof(vrMatrix));
}


/*********************************************************************/
/* delete the given matrix                                           */
void vrMatrixDelete(vrMatrix *mat)
{
	vrShmemFree(mat);
}


/**************************************************************************/
/* compare the values of two matrices                                     */
/* NOTE: if either matrix is NULL, return a non-equivalency value (ie. 0) */
/*   We may want to consider a third result for this: -1.                 */
/*   Just added a new check, such that if both matrices are NULL, then    */
/*   they are considered to be the same.                                  */
int vrMatrixEqual(vrMatrix *mat_l, vrMatrix *mat_r)
{
	int	count;

	if (mat_l == NULL && mat_r == NULL)
		return 1;

	if (mat_l == NULL)
		return 0;

	if (mat_r == NULL)
		return 0;

	for (count = 0; count < 16; count++) {
		if (mat_l->v[count] != mat_r->v[count])
			return 0;
	}

	return 1;
}


/********************************************************************/
/* copy the given matrix; ie, mat_l = mat_r                         */
vrMatrix *vrMatrixCopy(vrMatrix *mat_l, vrMatrix *mat_r)
{
#if 1 /* for debugging on machines that don't crash when mat_l == NULL */
	if (mat_l == NULL) {
		vrErrPrintf("vrMatrixCopy(): asked to copy to NULL memory.\n"
			" -- pausing -- use 'dbx -p %d'\n", getpid());
		pause();
	}
#endif

	if (mat_r == NULL) {
#if 0 /* for more debugging on machines that don't crash when mat_r == NULL */
		vrErrPrintf("vrMatrixCopy(): asked to copy to NULL memory.\n"
			" -- pausing -- use 'dbx -p %d'\n", getpid());
		pause();
#endif
		/* when not debugging, just return the identity matrix */
		vrMatrixSetScale3d(mat_l, 1.0f, 1.0f, 1.0f);
	} else {
		memcpy(mat_l, mat_r, sizeof(vrMatrix));
	}

	return mat_l;
}


/*********************************************************************/
/* set an existing 4x4 matrix to the identity matrix               */
vrMatrix *vrMatrixSetIdentity(vrMatrix *mat)
{
	return vrMatrixSetScale3d(mat, 1.0f, 1.0f, 1.0f);
}


/*********************************************************************/
/* create a 4x4 identity matrix                                      */
vrMatrix *vrMatrixCreateIdentity(void)
{
	return vrMatrixSetScale3d(vrMatrixCreate(), 1.0f, 1.0f, 1.0f);
}


/******************************************************************************/
/* set a 4x4 transform matrix from an array of 16 doubles                     */
/* Here is how the OpenGL-style matrix corresponds to                         */
/*   the matrix array elements:                                               */
/*        v[0]   v[4]   v[8]  v[12]                                           */
/*        v[1]   v[5]   v[9]  v[13]                                           */
/*        v[2]   v[6]  v[10]  v[14]                                           */
/*        v[3]   v[7]  v[11]  v[15]                                           */
/* The transpose of this looks more familiar:                                 */
/*        a  b  c  0                                                          */
/*        d  e  f  0                                                          */
/*        g  h  i  0                                                          */
/*        x  y  z  1                                                          */
/* where a-i is the 3x3 rotation matrix and [x,y,z] is the translation vector */
vrMatrix *vrMatrixSetAd(vrMatrix *mat, double *array)
{
	mat->v[ 0] = array[ 0];		/* a */
	mat->v[ 1] = array[ 1];		/* b */
	mat->v[ 2] = array[ 2];		/* c */
	mat->v[ 3] = array[ 3];		/* 0 */
	mat->v[ 4] = array[ 4];		/* d */
	mat->v[ 5] = array[ 5];		/* e */
	mat->v[ 6] = array[ 6];		/* f */
	mat->v[ 7] = array[ 7];		/* 0 */
	mat->v[ 8] = array[ 8];		/* g */
	mat->v[ 9] = array[ 9];		/* h */
	mat->v[10] = array[10];		/* i */
	mat->v[11] = array[11];		/* 0 */
	mat->v[12] = array[12];		/* x */
	mat->v[13] = array[13];		/* y */
	mat->v[14] = array[14];		/* z */
	mat->v[15] = array[15];		/* 1 */

	return mat;
}


/*********************************************************************/
/* set matrix to be a 4x4 translation matrix from 3 doubles          */
vrMatrix *vrMatrixSetTranslation3d(vrMatrix *mat, double x, double y, double z)
{
	vrMatrixSetIdentity(mat);
	VRMAT_ROWCOL(mat, VR_X, VR_W) = x;
	VRMAT_ROWCOL(mat, VR_Y, VR_W) = y;
	VRMAT_ROWCOL(mat, VR_Z, VR_W) = z;

	return mat;
}


/*********************************************************************/
/* set a 4x4 translation matrix from an array of 3 doubles           */
vrMatrix *vrMatrixSetTranslationAd(vrMatrix *mat, double *array)
{
	vrMatrixSetIdentity(mat);
	VRMAT_ROWCOL(mat, VR_X, VR_W) = array[VR_X];
	VRMAT_ROWCOL(mat, VR_Y, VR_W) = array[VR_Y];
	VRMAT_ROWCOL(mat, VR_Z, VR_W) = array[VR_Z];

	return mat;
}


/**************************************************************************/
/* set only translation values of a 4x4 matrix from an array of 3 doubles */
vrMatrix *vrMatrixSetTranslationOnlyAd(vrMatrix *mat, double *array)
{
	VRMAT_ROWCOL(mat, VR_X, VR_W) = array[VR_X];
	VRMAT_ROWCOL(mat, VR_Y, VR_W) = array[VR_Y];
	VRMAT_ROWCOL(mat, VR_Z, VR_W) = array[VR_Z];

	return mat;
}


/************************************************************************************/
/* get the translation values into an array of 3 doubles from a 4x4 mat,            */
/*   and then reset the current matrix's translation values to zero.                */
/*   TODO: usage of this should go away once we fully adopt the vrVector operations */
/*   [1/16/02] Hmmm, actually, this can be used by the vector class by passing the  */
/*   "v" field.  And is an efficient way of translating to origin and back for      */
/*    rotations about  the origin.                                                  */
vrMatrix *vrMatrixGetResetTranslationAd(vrMatrix *mat, double *array)
{
	array[VR_X] = VRMAT_ROWCOL(mat, VR_X, VR_W);
	array[VR_Y] = VRMAT_ROWCOL(mat, VR_Y, VR_W);
	array[VR_Z] = VRMAT_ROWCOL(mat, VR_Z, VR_W);

	VRMAT_ROWCOL(mat, VR_X, VR_W) = 0.0;
	VRMAT_ROWCOL(mat, VR_Y, VR_W) = 0.0;
	VRMAT_ROWCOL(mat, VR_Z, VR_W) = 0.0;

	return mat;
}


/*********************************************************************/
/* NOTE: this function doesn't work -- the intent is to decompose a  */
/*   4x4 matrix into the component parts (scale, shear, rotation,    */
/*   translation, and perspective).                                  */
/* The function returns '1' when it can successfully decompose the   */
/*   matrix, 0 when it cannot.                                       */
int vrMatrixCompositionFromMatrix(vrMatrixComposition *comp, vrMatrix *mat)
{
	int		count, count2;
	vrMatrix	local_mat;
	vrMatrix	pmat;
	vrMatrix	invpmat;
	vrVector4	prhs;
	vrVector	row[3];
	vrVector	tmpvec;

	local_mat = *mat;

	/* Normalize the matrix */
	if (VRMAT_COLROW(&local_mat, VR_W, VR_W) != 0) {
		for (count = VR_X; count <= VR_W; count++)
			for (count2 = VR_X; count2 <= VR_W; count2++)
				VRMAT_COLROW(&local_mat, count, count2) /= VRMAT_COLROW(&local_mat, VR_W, VR_W);
	}

	/* Initialize pmat and check for 3x3 singularity */
	pmat = local_mat;
	for (count = VR_X; count <= VR_Z; count++)
		VRMAT_COLROW(&pmat, count, VR_W) = 0.0;
	VRMAT_COLROW(&pmat, VR_W, VR_Z) = 1.0;

	if (vrMatrixDeterminant(&pmat) == 0.0)
		return 0;

	/* Find the perspective values (this is the messiest) */
	if (VRMAT_COLROW(&local_mat, VR_X, VR_W) != 0 ||
		VRMAT_COLROW(&local_mat, VR_Y, VR_W) != 0 ||
		VRMAT_COLROW(&local_mat, VR_Z, VR_W) != 0) {

		/* prhs is the right hand side of the equation */
		prhs.v[VR_X] = VRMAT_COLROW(&local_mat, VR_X, VR_W);
		prhs.v[VR_Y] = VRMAT_COLROW(&local_mat, VR_Y, VR_W);
		prhs.v[VR_Z] = VRMAT_COLROW(&local_mat, VR_Z, VR_W);
		prhs.v[VR_W] = VRMAT_COLROW(&local_mat, VR_Z, VR_W);

		/* solve the equation by inverting pmat and multiplying prhs by the */
		/*   inverse.  (This is the easiest way, not necessarily the best.) */
		vrMatrixInvert(&invpmat, &pmat);
		vrVector4TransformByMatrix((vrVector4 *)comp->perspective, &prhs, &invpmat);

		/* Clear the perspective partition */
		VRMAT_COLROW(&local_mat, VR_X, VR_W) = 0.0;
		VRMAT_COLROW(&local_mat, VR_Y, VR_W) = 0.0;
		VRMAT_COLROW(&local_mat, VR_Z, VR_W) = 0.0;
		VRMAT_COLROW(&local_mat, VR_W, VR_W) = 1.0;

	} else {
		/* There isn't any perspective */
		comp->perspective[VR_X] = 0.0;
		comp->perspective[VR_Y] = 0.0;
		comp->perspective[VR_Z] = 0.0;
		comp->perspective[VR_W] = 0.0;
	}

	/* Take care of translation (easy) */
	for (count = VR_X; count <= VR_Z; count++) {
		comp->translate[count] = VRMAT_COLROW(&local_mat, VR_W, count);
		VRMAT_COLROW(&local_mat, VR_W, count) = 0.0;
	}

	/* Get the scale and shear */
	for (count = VR_X; count <= VR_Z; count++) {
		row[count].v[VR_X] = VRMAT_COLROW(&local_mat, count, VR_X);
		row[count].v[VR_Y] = VRMAT_COLROW(&local_mat, count, VR_Y);
		row[count].v[VR_Z] = VRMAT_COLROW(&local_mat, count, VR_Z);
	}

	/* Compute X scale factor and normalize first row */
	comp->scale[VR_X] = vrVectorLength(&row[VR_X]);
	vrVectorNormalize(&row[VR_X], &row[VR_X]);

	/* Compute XY shear factor and make 2nd row orthogonal to 1st */
	comp->shear[0] = vrVectorDotProduct(&row[VR_X], &row[VR_Y]);
	vrVectorAddScaledVectors(&row[VR_Y], &row[VR_X], &row[VR_Y], 1.0, -comp->shear[0]);

	/* Compute Y scale and normalize 2nd row */
	comp->scale[VR_Y] = vrVectorLength(&row[VR_Y]);
	vrVectorNormalize(&row[VR_Y], &row[VR_Y]);
	comp->shear[0] /= comp->scale[VR_Y];

	/* Compute XZ and YZ shears, orthogonalize 3rd row */
	comp->shear[1] = vrVectorDotProduct(&row[VR_X], &row[VR_Z]);
	vrVectorAddScaledVectors(&row[VR_Z], &row[VR_X], &row[VR_Z], 1.0, -comp->shear[1]);
	comp->shear[2] = vrVectorDotProduct(&row[VR_Y], &row[VR_Z]);
	vrVectorAddScaledVectors(&row[VR_Z], &row[VR_Y], &row[VR_Z], 1.0, -comp->shear[2]);

	/* Compute Z scale and normalize 3rd row */
	comp->scale[VR_Z] = vrVectorLength(&row[VR_Z]);
	vrVectorNormalize(&row[VR_Z], &row[VR_Z]);
	comp->shear[1] /= comp->scale[VR_Z];
	comp->shear[2] /= comp->scale[VR_Z];

	/* At this point, the matrix (in rows[]) is orthonormal.  Check */
	/*   for a coordinate system flip.  If the determinant is -1,   */
	/*   then negate the matrix and the scaling factors.            */
	if (vrVectorDotProduct(&row[VR_X], vrVectorCrossProduct(&tmpvec, &row[VR_Y], &row[VR_Z])) < 0) {
		for (count = 0; count <= VR_Z; count++) {
			comp->scale[count] *= -1.0;
			row[count].v[VR_X] *= -1.0;
			row[count].v[VR_Y] *= -1.0;
			row[count].v[VR_Z] *= -1.0;
		}
	}

	/* Now get the rotation out */
	comp->rotate[VR_Y] = asin(-row[VR_X].v[VR_Z]);
	if (cos(comp->rotate[VR_Y]) != 0.0) {
		comp->rotate[VR_X] = atan2(row[VR_Y].v[VR_Z], row[VR_Z].v[VR_Z]);
		comp->rotate[VR_Z] = atan2(row[VR_X].v[VR_Y], row[VR_X].v[VR_X]);
	} else {
		comp->rotate[VR_X] = atan2(row[VR_Y].v[VR_X], row[VR_Y].v[VR_Y]);
		comp->rotate[VR_Z] = 0.0;
	}

	return 1;
}


/*********************************************************************/
/* set an existing 4x4 translation matrix from an array of 6 doubles */
/*   from X,Y,Z location and Azim, Elev, Roll orientation.           */
vrMatrix *vrMatrixSetXYZAERAd(vrMatrix *mat, double *array)
{
	vrMatrixSetIdentity(mat);

	VRMAT_ROWCOL(mat, VR_X, VR_W) = array[VR_X];
	VRMAT_ROWCOL(mat, VR_Y, VR_W) = array[VR_Y];
	VRMAT_ROWCOL(mat, VR_Z, VR_W) = array[VR_Z];

	vrMatrixPostRotateId(mat, VR_Y, array[VR_AZIM+3]);
	vrMatrixPostRotateId(mat, VR_X, array[VR_ELEV+3]);
	vrMatrixPostRotateId(mat, VR_Z, array[VR_ROLL+3]);

	return mat;
}


/*********************************************************************/
/* set an existing 4x4 translation matrix from an array of 6 floats  */
/*   from X,Y,Z location and Azim, Elev, Roll orientation.           */
vrMatrix *vrMatrixSetXYZAERAf(vrMatrix *mat, float *array)
{
	vrMatrixSetIdentity(mat);

	VRMAT_ROWCOL(mat, VR_X, VR_W) = array[VR_X];
	VRMAT_ROWCOL(mat, VR_Y, VR_W) = array[VR_Y];
	VRMAT_ROWCOL(mat, VR_Z, VR_W) = array[VR_Z];

	vrMatrixPostRotateId(mat, VR_Y, (double)(array[VR_AZIM+3]));
	vrMatrixPostRotateId(mat, VR_X, (double)(array[VR_ELEV+3]));
	vrMatrixPostRotateId(mat, VR_Z, (double)(array[VR_ROLL+3]));

	return mat;
}


/*********************************************************************/
/* set an existing 4x4 translation matrix from a vrEuler.            */
vrMatrix *XXvrMatrixSetXYZAERE(vrMatrix *mat, vrEuler *euler)
{
	vrMatrixSetIdentity(mat);

	VRMAT_ROWCOL(mat, VR_X, VR_W) = euler->t[VR_X];
	VRMAT_ROWCOL(mat, VR_Y, VR_W) = euler->t[VR_Y];
	VRMAT_ROWCOL(mat, VR_Z, VR_W) = euler->t[VR_Z];

	vrMatrixPostRotateId(mat, VR_Y, euler->r[VR_AZIM]);
	vrMatrixPostRotateId(mat, VR_X, euler->r[VR_ELEV]);
	vrMatrixPostRotateId(mat, VR_Z, euler->r[VR_ROLL]);

	return mat;
}


/***********************************************************************/
/* set an existing 4x4 translation matrix from a vrEuler, with the     */
/*   axis about which the azimuth is derived specified.  The elevation */
/*   and roll axes are assumed to follow in logical fashion.           */
/* TODO: One day, and that day may never come, we may need an Euler conversion   */
/*   function that is more general than this, allowing any axis to be associated */
/*   with any rotation about a body, and in either direction.  However, until    */
/*   that days comes, or someone is sufficiently bored, that function won't be   */
/*   written.                                                                    */
vrMatrix *vrMatrixSetEulerAzimaxis(vrMatrix *mat, vrEuler *euler, int axis)
{
	vrMatrixSetIdentity(mat);

	VRMAT_ROWCOL(mat, VR_X, VR_W) = euler->t[VR_X];
	VRMAT_ROWCOL(mat, VR_Y, VR_W) = euler->t[VR_Y];
	VRMAT_ROWCOL(mat, VR_Z, VR_W) = euler->t[VR_Z];

	switch (axis) {
	case VR_X:	/* ie. an X-up coordinate system (an unlikely choice) */
		vrMatrixPostRotateId(mat, VR_X, euler->r[VR_AZIM]);
		vrMatrixPostRotateId(mat, VR_Z, euler->r[VR_ELEV]);
		vrMatrixPostRotateId(mat, VR_Y, euler->r[VR_ROLL]);
		break;
	case VR_Y:	/* ie. a Y-up coordinate system */
		vrMatrixPostRotateId(mat, VR_Y, euler->r[VR_AZIM]);
		vrMatrixPostRotateId(mat, VR_X, euler->r[VR_ELEV]);
		vrMatrixPostRotateId(mat, VR_Z, euler->r[VR_ROLL]);
		break;
	default:
		vrMsgPrintf("Warning, incorrect azimuth axis specified in Euler conversion, using 'Z'.\n");
	case VR_Z:	/* ie. a Z-up coordinate system */
		vrMatrixPostRotateId(mat, VR_Z, euler->r[VR_AZIM]);
		vrMatrixPostRotateId(mat, VR_Y, euler->r[VR_ELEV]);
		vrMatrixPostRotateId(mat, VR_X, euler->r[VR_ROLL]);
		break;
	}

	return mat;
}


/*********************************************************************/
vrEuler *vrEulerSetFromMatrix(vrEuler *euler, vrMatrix *mat)
{
	double	azim;
	double	elev;
	double	roll;
	double	azim_sin;
	double	azim_cos;

	/* first handle the translational values -- very easy */
	euler->t[VR_X] = VRMAT_ROWCOL(mat, VR_X, VR_W);
	euler->t[VR_Y] = VRMAT_ROWCOL(mat, VR_Y, VR_W);
	euler->t[VR_Z] = VRMAT_ROWCOL(mat, VR_Z, VR_W);

	/* set the rotational values */
	/*** This section of code calculates the proper Euler angles from ***/
	/***   the 4x4 matrix using techniques from Robotics, Fu et.al.   ***/
	/***   Actually, the matrix order seems backward, and I lucked    ***/
	/***   into getting them backward - figure it out later, it works!***/
	azim = atan2(VRMAT_ROWCOL(mat, VR_X, VR_Z), VRMAT_ROWCOL(mat, VR_Z, VR_Z));
	azim_sin = sin(azim);
	azim_cos = cos(azim);

	elev = atan2(VRMAT_ROWCOL(mat, VR_Y, VR_Z), azim_sin*VRMAT_ROWCOL(mat, VR_X, VR_Z) + azim_cos*VRMAT_ROWCOL(mat, VR_Z, VR_Z));
	roll = atan2(azim_sin*VRMAT_ROWCOL(mat, VR_Z, VR_Y) - azim_cos*VRMAT_ROWCOL(mat, VR_X, VR_Y), azim_cos*VRMAT_ROWCOL(mat, VR_X, VR_X) - azim_sin*VRMAT_ROWCOL(mat, VR_Z, VR_X));

#if 1 /* NOTE: 5/2/03: it seems as though the Azim and Elev values are swapped from the other lib */ /* 01/29/13 -- changed to "1" for testing */ /* 03/18/15 considered changing to "0", but issue was more with knowing that elevation is the X-rotation */
	euler->r[VR_AZIM] =  RTOD(azim);
	euler->r[VR_ELEV] = -RTOD(elev);
#else
	euler->r[VR_ELEV] =  RTOD(azim);
	euler->r[VR_AZIM] = -RTOD(elev);
#endif
	euler->r[VR_ROLL] =  RTOD(roll);

	return euler;
}


/*********************************************************************/
/* Set an existing 4x4 translation matrix from a coordinate triplet. */
/*   This is primarily used to configure the windows based on three  */
/*   locations in space (lower-left point, lower-right point, upper- */
/*   left point.                                                     */
vrMatrix *vrMatrixSetFromCoords(vrMatrix *mat, double *coords)
{
	vrPoint		P0w, P1w, P2w;		/* three points that describe a rectangular window*/
	vrVector	x;			/* vector from P0w to P1w */
	vrVector	y_basis;		/* vector from P0w to P2w */
	vrVector	y;			/* y_basis vector with x vector component trimmed */
	vrVector	xhat, yhat, zhat;	/* normalized x, y & z vectors (orthogonal) */
	vrMatrix	Tw2rw;			/* Transform matrix from window to real-world */
	vrVector	tmpvec;

	/* step1: get the three coordinates from the configuration */
	vrPointSet3d(&P0w, coords[0], coords[1], coords[2]);
	vrPointSet3d(&P1w, coords[3], coords[4], coords[5]);
	vrPointSet3d(&P2w, coords[6], coords[7], coords[8]);
	vrDbgPrintfN(MATH_DBGLVL, "P0w = (%lf %lf %lf), P1w = (%lf %lf %lf), P2w = (%lf %lf %lf)\n",
		P0w.v[0], P0w.v[1], P0w.v[2],
		P1w.v[0], P1w.v[1], P1w.v[2],
		P2w.v[0], P2w.v[1], P2w.v[2]);

	/* step2: calculate the x & y vectors */
	vrVectorFromTwoPoints(&x, &P0w, &P1w);
	vrVectorFromTwoPoints(&y_basis, &P0w, &P2w);
	/* from Lengyel pg 9, equation 1.19 -- perp-q(P), ie. the perpendicular-to-Q element of P */
	/* Y = Yb - X*(X dot Yb / X dot X)  -- ie. subtract the projection of Yb onto X from Yb resulting in a Y orthogonal to X */
	vrVectorSubtract(&y, &y_basis, vrVectorScale(vrVectorCopy(&tmpvec, &x), (vrVectorDotProduct(&x, &y_basis) / vrVectorDotProduct(&x, &x))));
	vrDbgPrintfN(MATH_DBGLVL, "x = (%lf %lf %lf), y_basis = (%lf %lf %lf), y = (%lf %lf %lf)\n",
		x.v[0], x.v[1], x.v[2],
		y_basis.v[0], y_basis.v[1], y_basis.v[2],
		y.v[0], y.v[1], y.v[2]);


	/* step3: calculate the unit X & Y vectors */
	vrVectorNormalize(&xhat, &x);
	vrVectorNormalize(&yhat, &y);

	/* step4: calculate the unit Z vector */
	vrVectorCrossProduct(&zhat, &xhat, &yhat);
	vrDbgPrintfN(MATH_DBGLVL, "x^ = (%lf %lf %lf), y^ = (%lf %lf %lf), z^ = (%lf %lf %lf)\n",
		xhat.v[0], xhat.v[1], xhat.v[2],
		yhat.v[0], yhat.v[1], yhat.v[2],
		zhat.v[0], zhat.v[1], zhat.v[2]);

	/* step5: Make the Tw2rw matrix */
	vrMatrixSetIdentity(&Tw2rw);
	/* remember to use coordinates based on OpenGL documentation */
	VRMAT_ROWCOL(&Tw2rw, 0, 0) = xhat.v[0];
	VRMAT_ROWCOL(&Tw2rw, 1, 0) = xhat.v[1];
	VRMAT_ROWCOL(&Tw2rw, 2, 0) = xhat.v[2];
	VRMAT_ROWCOL(&Tw2rw, 0, 1) = yhat.v[0];
	VRMAT_ROWCOL(&Tw2rw, 1, 1) = yhat.v[1];
	VRMAT_ROWCOL(&Tw2rw, 2, 1) = yhat.v[2];
	VRMAT_ROWCOL(&Tw2rw, 0, 2) = zhat.v[0];
	VRMAT_ROWCOL(&Tw2rw, 1, 2) = zhat.v[1];
	VRMAT_ROWCOL(&Tw2rw, 2, 2) = zhat.v[2];
	/* the translation is by the lower-left corner */
	VRMAT_ROWCOL(&Tw2rw, 0, 3) = P0w.v[0];
	VRMAT_ROWCOL(&Tw2rw, 1, 3) = P0w.v[1];
	VRMAT_ROWCOL(&Tw2rw, 2, 3) = P0w.v[2];
	if (vrDbgDo(MATH_DBGLVL))
		vrPrintMatrix("Tw2rw -- matrix", &Tw2rw);

	/* step5: Calculate Trw2w from Tw2rw */
	vrMatrixInvertEuclidean(mat, &Tw2rw);
	if (vrDbgDo(MATH_DBGLVL))
		vrPrintMatrix("Trw2w -- matrix", mat);

	return mat;
}


/**********************************************************************/
/* create a 4x4 matrix for rotation around one of the coordinate axes */
vrMatrix *vrMatrixSetRotationId(vrMatrix *mat, int axis, double theta)
{
	double	cost = cos(vrDegreesToRadians(theta));
	double	sint = sin(vrDegreesToRadians(theta));

	vrMatrixSetIdentity(mat);

	switch (axis) {
	case VR_X:
		VRMAT_ROWCOL(mat, VR_Y, VR_Y) = cost;
		VRMAT_ROWCOL(mat, VR_Y, VR_Z) = -sint;
		VRMAT_ROWCOL(mat, VR_Z, VR_Y) = sint;
		VRMAT_ROWCOL(mat, VR_Z, VR_Z) = cost;
		break;
	case VR_Y:
		VRMAT_ROWCOL(mat, VR_X, VR_X) = cost;
		VRMAT_ROWCOL(mat, VR_Z, VR_X) = -sint;
		VRMAT_ROWCOL(mat, VR_X, VR_Z) = sint;
		VRMAT_ROWCOL(mat, VR_Z, VR_Z) = cost;
		break;
	case VR_Z:
		VRMAT_ROWCOL(mat, VR_X, VR_X) = cost;
		VRMAT_ROWCOL(mat, VR_X, VR_Y) = -sint;
		VRMAT_ROWCOL(mat, VR_Y, VR_X) = sint;
		VRMAT_ROWCOL(mat, VR_Y, VR_Y) = cost;
		break;
	default:
		vrErr("unknown axis specified");
		break;
	}
	return mat;
}


/*************************************************************************/
/* the "old" version of this routine may have had the row,column         */
/*   values swapped -- the result of this is merely a sign change        */
/*   on the sin() operation.  I'm testing this in some test market       */
/*   cities to see how it plays.                                         */
/* 9/6/02 -- which means, that this function is now (and apparently has  */
/*   been for a while) exactly the same as vrMatrixSetRotationId() above */
/*   just with some of the lines in a slightly different order.  So, now,*/
/*   TODO: get rid of this function.                                     */
vrMatrix *vrMatrixSetRotationIdNew(vrMatrix *mat, int axis, double theta)
{
	double	cost = cos(vrDegreesToRadians(theta));
	double	sint = sin(vrDegreesToRadians(theta));

	vrMatrixSetIdentity(mat);

	switch (axis) {
	case VR_X:
		VRMAT_ROWCOL(mat, VR_Y, VR_Y) = cost;
		VRMAT_ROWCOL(mat, VR_Z, VR_Y) = sint;
		VRMAT_ROWCOL(mat, VR_Y, VR_Z) = -sint;
		VRMAT_ROWCOL(mat, VR_Z, VR_Z) = cost;
		break;
	case VR_Y:
		VRMAT_ROWCOL(mat, VR_X, VR_X) = cost;
		VRMAT_ROWCOL(mat, VR_X, VR_Z) = sint;
		VRMAT_ROWCOL(mat, VR_Z, VR_X) = -sint;
		VRMAT_ROWCOL(mat, VR_Z, VR_Z) = cost;
		break;
	case VR_Z:
		VRMAT_ROWCOL(mat, VR_X, VR_X) = cost;
		VRMAT_ROWCOL(mat, VR_Y, VR_X) = sint;
		VRMAT_ROWCOL(mat, VR_X, VR_Y) = -sint;
		VRMAT_ROWCOL(mat, VR_Y, VR_Y) = cost;
		break;
	default:
		vrErr("unknown axis specified");
		break;
	}
	return mat;
}


/*********************************************************************/
/* create a 4x4 matrix for rotation around an arbitrary axis         */
vrMatrix *vrMatrixSetRotation4d(vrMatrix *mat, double x, double y, double z, double theta)
{
	vrQuat	quat;

	vrQuatSet4d(&quat, theta, x, y, z);
	mat = vrMatrixSetFromQuatRC(mat, &quat);

	return mat;
}


/*********************************************************************/
/* create a 4x4 matrix for rotation around an arbitrary axis         */
/* NOTE: order of the array is X, Y, Z, theta (in degrees)           */
vrMatrix *vrMatrixSetRotationAd(vrMatrix *mat, double *array)
{
	vrQuat	quat;

	vrQuatSet4d(&quat, array[VR_W], array[VR_X], array[VR_Y], array[VR_Z]);
	mat = vrMatrixSetFromQuatRC(mat, &quat);

	return mat;
}


/*********************************************************************/
/* set an existing 4x4 matrix to the scaling transformation          */
vrMatrix *vrMatrixSetScale3d(vrMatrix *mat, double sx, double sy, double sz)
{
	memset(mat, 0, sizeof(vrMatrix));
	VRMAT_ROWCOL(mat, VR_X, VR_X) = sx;
	VRMAT_ROWCOL(mat, VR_Y, VR_Y) = sy;
	VRMAT_ROWCOL(mat, VR_Z, VR_Z) = sz;
	VRMAT_ROWCOL(mat, VR_W, VR_W) = 1.0f;

	return mat;
}


/****************************************************************/
/* Multiply two 4x4 matrices (product = mat_l * mat_r).         */
/*   When working in the OpenGL documentation convention, then  */
/*   consider the operation to be:                              */
/*       product * point = mat_l * mat_r * point                */
/*                                                              */
/*   When working in the C-style matrix convention, then:       */
/*       point * product = point * mat_r * mat_l                */
vrMatrix *vrMatrixProduct(vrMatrix *product, vrMatrix *mat_l, vrMatrix *mat_r)
{
	int		count;
	int		row;
	int		col;
	double		sum;

	for (row = 0; row < VRMAT_MAXDIM; row++) {
		for (col = 0; col < VRMAT_MAXDIM; col++) {
			sum = 0.0f;
			for (count = 0; count < VRMAT_MAXDIM; count++)
				sum += VRMAT_ROWCOL(mat_l, row, count) * VRMAT_ROWCOL(mat_r, count, col);
			VRMAT_ROWCOL(product, row, col) = sum;
		}
	}

	return product;
}


/*********************************************************************/
/* transpose the given (src) matrix, putting the result in the (dst) */
/*   destination matrix.  A pointer to the resultant dst matrix is   */
/*   also returned as an argument.  It is safe to pass the same      */
/*   matrix-pointer as both the source and destination matrices, in  */
/*   which case that matrix is transposed in place.                  */
/*      Eg.  t = vrMatrixTranspose(m, m);                            */
vrMatrix *vrMatrixTranspose(vrMatrix *dst_mat, vrMatrix *src_mat)
{
	int	row;
	int	col;
	double	tmp;

	for (row = 0; row < VRMAT_MAXDIM-1; row++) {
		for (col = row + 1; col < VRMAT_MAXDIM; col++) {
			tmp = VRMAT_ROWCOL(dst_mat, row, col);
			VRMAT_ROWCOL(dst_mat, row, col) = VRMAT_ROWCOL(src_mat, col, row);
			VRMAT_ROWCOL(src_mat, col, row) = tmp;
		}
	}

	return dst_mat;
}


/************************************************************/
double vrMatrixGetScale(vrMatrix *mat)
{
	vrVector	*row1;		/* row-1 of the 3x3 rotation matrix */
	vrVector	*row2;		/* row-2 of the 3x3 rotation matrix */
	vrVector	*row3;		/* row-3 of the 3x3 rotation matrix */
	double		scale_sq1;	/* the scales of row-1 of the rotation portion */
	double		scale_sq2;	/* the scales of row-2 of the rotation portion */
	double		scale_sq3;	/* the scales of row-3 of the rotation portion */

	row1 = (vrVector *)&mat->v[0];	/* NOTE: these are actually columns of the OpenGL style */
	row2 = (vrVector *)&mat->v[4];	/*   matrix, which makes them rows of the transpose.    */
	row3 = (vrVector *)&mat->v[8];

	scale_sq1 = row1->v[0]*row1->v[0] + row1->v[1]*row1->v[1] + row1->v[2]*row1->v[2];
	scale_sq2 = row2->v[0]*row2->v[0] + row2->v[1]*row2->v[1] + row2->v[2]*row2->v[2];
	scale_sq3 = row3->v[0]*row3->v[0] + row3->v[1]*row3->v[1] + row3->v[2]*row3->v[2];

	/* check whether all the scale factors are the same (within a reasonable epsilon) */
	if ((fabs(scale_sq1 - scale_sq2) > 10000*VR_EPSILON) || (fabs(scale_sq1 - scale_sq3) > 10000*VR_EPSILON)) {
		/* if they are not the same, print an error and return the identity value */
#if 0
		vrErrPrintf("vrMatrixGetScale(): " RED_TEXT "Warning: an asymmetrically scaled matrix!\n" NORM_TEXT);
#else
		vrErrPrintf("vrMatrixGetScale(): " RED_TEXT "Warning: an asymmetrically scaled matrix! (%.9f %.9f %.9f)\n" NORM_TEXT, scale_sq1, scale_sq2, scale_sq3);
#endif
		return (1.0);		/* NOTE: 1.0 is the identity operation for scaling */
	}

	return (sqrt(scale_sq1));
}


/************************************************************/
vrMatrix *vrMatrixSetScale(vrMatrix *dst, vrMatrix *src, double scale)
{
	vrMatrix	tmpmat;		/* memory holder for src matrix to allow same value for dst and src */
	vrVector	*row1;		/* row-1 of the 3x3 rotation matrix */
	vrVector	*row2;		/* row-2 of the 3x3 rotation matrix */
	vrVector	*row3;		/* row-3 of the 3x3 rotation matrix */
	double		scale_sq;	/* the square of the given scale */

	if(src == dst) {	/* if modifying in-place: */
		tmpmat = *src;	/*   copy source data and */
		src = &tmpmat;	/*   change src arg to point to local copy */
	} else {
		*dst = *src;	/* start by copying the source matrix to the destination */
	}

	scale_sq = scale * scale;

	row1 = (vrVector *)&dst->v[0];	/* NOTE: these are actually columns of the OpenGL style */
	row2 = (vrVector *)&dst->v[4];	/*   matrix, which makes them rows of the transpose.    */
	row3 = (vrVector *)&dst->v[8];

	/* now rescale all the rows to have the average scale */
	vrVectorScale(row1, sqrt(scale_sq / vrVectorDotProduct(row1, row1)));
	vrVectorScale(row2, sqrt(scale_sq / vrVectorDotProduct(row2, row2)));
	vrVectorScale(row3, sqrt(scale_sq / vrVectorDotProduct(row3, row3)));

	return (dst);
}


/************************************************************/
/* Report whether the given matrix conforms to the rules of */
/*   a Euclidean matrix.  A Euclidean transformation matrix */
/*   must consist only of rotations, uniform scales, and    */
/*   translations.                                          */
int vrMatrixIsEuclidean(vrMatrix *mat)
{
	vrVector	*row1;		/* row-1 of the 3x3 rotation matrix */
	vrVector	*row2;		/* row-2 of the 3x3 rotation matrix */
	vrVector	*row3;		/* row-3 of the 3x3 rotation matrix */
	double		scale_sq1;	/* the scales of row-1 of the rotation portion */
	double		scale_sq2;	/* the scales of row-2 of the rotation portion */
	double		scale_sq3;	/* the scales of row-3 of the rotation portion */
	int		result = 1;

	row1 = (vrVector *)&mat->v[0];	/* NOTE: these are actually columns of the OpenGL style */
	row2 = (vrVector *)&mat->v[4];	/*   matrix, which makes them rows of the transpose.    */
	row3 = (vrVector *)&mat->v[8];

	scale_sq1 = row1->v[0]*row1->v[0] + row1->v[1]*row1->v[1] + row1->v[2]*row1->v[2];
	scale_sq2 = row2->v[0]*row2->v[0] + row2->v[1]*row2->v[1] + row2->v[2]*row2->v[2];
	scale_sq3 = row3->v[0]*row3->v[0] + row3->v[1]*row3->v[1] + row3->v[2]*row3->v[2];

	/* NOTE: these operations seem to be below the desired EPSILON value so need to soften the requirement */
	if ((fabs(scale_sq1 - scale_sq2) > 1000000*VR_EPSILON) || (fabs(scale_sq1 - scale_sq3) > 1000000*VR_EPSILON)) {
		vrErrPrintf("vrMatrixIsEuclidean(): " RED_TEXT "Warning: an unsymetrically scaled matrix is not Euclidean!  Scales: %.18lf, %.18lf, %.18lf\n" NORM_TEXT, scale_sq1, scale_sq2, scale_sq3);
		result = 0;
	}

	if (fabs(vrVectorDotProduct(row1, row2)) > 1000000*VR_EPSILON) {
		vrErrPrintf("vrMatrixIsEuclidean(): " RED_TEXT "Warning: Row 1 and 2 of matrix do not give a 0 dot product as expected from a Euclidean matrix! (%.18lf)\n" NORM_TEXT, vrVectorDotProduct(row1, row2));
		result = 0;
	}

	if (fabs(vrVectorDotProduct(row1, row3)) > 1000000*VR_EPSILON) {
		vrErrPrintf("vrMatrixIsEuclidean(): " RED_TEXT "Warning: Row 1 and 3 of matrix do not give a 0 dot product as expected from a Euclidean matrix! (%.18lf)\n" NORM_TEXT, vrVectorDotProduct(row1, row3));
		result = 0;
	}

	if (fabs(vrVectorDotProduct(row2, row3)) > 1000000*VR_EPSILON) {
		vrErrPrintf("vrMatrixIsEuclidean(): " RED_TEXT "Warning: Row 2 and 3 of matrix do not give a 0 dot product as expected from a Euclidean matrix! (%.18lf)\n" NORM_TEXT, vrVectorDotProduct(row2, row3));
		result = 0;
	}

	return result;
}


/**************************************************************/
/* Transform the given matrix into one that conforms to the   */
/*   rules of a Euclidean matrix.  A Euclidean transformation */
/*   matrix must consist only of rotations, uniform scales,   */
/*   and translations.                                        */
/* We will use the algorithm expressed by Stuart in an email. */
vrMatrix *vrMatrixMakeEuclidean(vrMatrix *dst, vrMatrix *src)
{
	vrMatrix	tmpmat;		/* memory holder for src matrix to allow same value for dst and src */
	vrVector	tmpvec;		/* memory holder for copying a vector */
	vrVector	*row1;		/* row-1 of the 3x3 rotation matrix */
	vrVector	*row2;		/* row-2 of the 3x3 rotation matrix */
	vrVector	*row3;		/* row-3 of the 3x3 rotation matrix */
	double		scale_sq1;	/* the scales of row-1 of the rotation portion */
	double		scale_sq2;	/* the scales of row-2 of the rotation portion */
	double		scale_sq3;	/* the scales of row-3 of the rotation portion */
	double		avg_scale_sq;	/* square of the average scaling factor */
	double		row1dot1;	/* dot product of row-1 & itself */
	double		row1dot2;	/* dot product of row-1 & row-2 */
	double		row1dot3;	/* dot product of row-1 & row-3 */
	double		row2dot2;	/* dot product of row-2 & row-2 (after first normalization) */
	double		row2dot3;	/* dot product of row-2 & row-3 (after first normalization) */

	if(src == dst) {	/* if modifying in-place: */
		tmpmat = *src;	/*   copy source data and */
		src = &tmpmat;	/*   change src arg to point to local copy */
	} else {
		*dst = *src;	/* start by copying the source matrix to the destination */
	}

	/* assign the three row vectors to their location in the 4x4 matrix */
	row1 = (vrVector *)&dst->v[0];	/* NOTE: these are actually columns of the OpenGL style */
	row2 = (vrVector *)&dst->v[4];	/*   matrix, which makes them rows of the transpose.    */
	row3 = (vrVector *)&dst->v[8];

	/* calculate the squared scale values of each row and their average */
	scale_sq1 = row1->v[0]*row1->v[0] + row1->v[1]*row1->v[1] + row1->v[2]*row1->v[2];
	scale_sq2 = row2->v[0]*row2->v[0] + row2->v[1]*row2->v[1] + row2->v[2]*row2->v[2];
	scale_sq3 = row3->v[0]*row3->v[0] + row3->v[1]*row3->v[1] + row3->v[2]*row3->v[2];
	avg_scale_sq = (scale_sq1 + scale_sq2 + scale_sq3) / 3.0;

	/* calculate the dot product of row-1 with each row */
	row1dot1 = vrVectorDotProduct(row1, row1);
	row1dot2 = vrVectorDotProduct(row1, row2);
	row1dot3 = vrVectorDotProduct(row1, row3);

	/* assume row-1 is pointing in the correct direction, */
	/*   and subtract its component from row-2 and row-3. */
	vrVectorSubtract(row2, row2, vrVectorScale(vrVectorCopy(&tmpvec, row1), (row1dot2 / row1dot1)));
	vrVectorSubtract(row3, row3, vrVectorScale(vrVectorCopy(&tmpvec, row1), (row1dot3 / row1dot1)));

	/* now calculate the dot products of row-2 with itself and row-3 */
	/*   and then normalize row-3 with respect to row-2.             */
	row2dot2 = vrVectorDotProduct(row2, row2);
	row2dot3 = vrVectorDotProduct(row2, row3);
	vrVectorSubtract(row3, row3, vrVectorScale(vrVectorCopy(&tmpvec, row2), (row2dot3 / row2dot2)));

	/* now rescale all the rows to have the average scale */
	vrVectorScale(row1, sqrt(avg_scale_sq / vrVectorDotProduct(row1, row1)));
	vrVectorScale(row2, sqrt(avg_scale_sq / vrVectorDotProduct(row2, row2)));
	vrVectorScale(row3, sqrt(avg_scale_sq / vrVectorDotProduct(row3, row3)));

	/* Just to be safe, we'll check for the Euclideanness */
	if (!vrMatrixIsEuclidean(dst)) {
		vrErrPrintf("vrMatrixMakeEuclidean(): " RED_TEXT "Warning: attempt to Euclideanize the matrix failed!\n" NORM_TEXT);
	}
	return dst;
}


/***********************************************************************/
/* Calculate the inverse of a matrix that is of the Euclidean variety. */
/* Stuart: so long as the 4x4 is a Euclidean transformation (ie.       */
/*   consisting only of a combination of rotations, uniform scales and */
/*   translations, this works.  Matrix operations that cannot be       */
/*   accommodated are non-uniform scales, shears and projections.      */
/*   "Euclidean" is slightly too broad a term, geometers might call    */
/*   the class of matrices this works on: "Euclidean Similarity" is    */
/*   probably a more precise term.                                     */
vrMatrix *vrMatrixInvertEuclidean(vrMatrix *dst, vrMatrix *src)
{
	vrMatrix	tmp;		/* memory holder for src matrix to allow same value for dst and src */
	int		col, row;
	vrVector	trans;		/* the translation part of the matrix */
	vrVector	old_trans;	/* the translation part of the matrix */
	double		inv_scale_factor;/* reciprocal of the scale part of the matrix */

	if(src == dst) {	/* if inverting in-place: */
		tmp = *src;	/*   copy source data and */
		src = &tmp;	/*   change src arg to point to local copy */
	}

	/* Measure square-of-scaling from the first row.  If this matrix is */
	/*   actually a uniform scaling (plus rotation plus translation, as */
	/*   described above), then any row or column should give the same  */
	/*   result.                                                        */
	inv_scale_factor = 1.0 / (src->v[0]*src->v[0] + src->v[1]*src->v[1] + src->v[2]*src->v[2]);

	/* transpose 3x3 rotation sub matrix into a 4x4 matrix */
	for (col = 0; col < 3; col++) {
		for (row = 0; row < 3; row++) {
			VRMAT_ROWCOL(dst, col, row) = VRMAT_ROWCOL(src, row, col) * inv_scale_factor;
		}
		VRMAT_ROWCOL(dst, VR_W, col) = 0.0;
	}

	/* now calculate the new translation portion of the inversion */
	vrVectorGetTransFromMatrix(&old_trans, src);
	vrVectorTransformByMatrix(&trans, &old_trans, dst);	/* NOTE: this operation does not use the 4th row of the 4x4 matrix (which is not presently set) */
	vrVectorScale(&trans, -1.0);
	vrMatrixSetTransFromVector(dst, &trans);

	VRMAT_ROWCOL(dst, VR_W, VR_W) = 1.0;			/* NOTE: we haven't yet set this element of the matrix */

	return dst;
}


/*****************************************************************************/
double vrMatrixDeterminant(vrMatrix *mat)
{
	double	exx, eyx, ezx, ewx;	/* a1 - a4 */
	double	exy, eyy, ezy, ewy;	/* b1 - b4 */
	double	exz, eyz, ezz, ewz;	/* c1 - c4 */
	double	exw, eyw, ezw, eww;	/* d1 - d4 */
	double	result;

	exx = VRMAT_ROWCOL(mat, VR_X, VR_X);	/* a1 */
	eyx = VRMAT_ROWCOL(mat, VR_Y, VR_X);	/* a2 */
	ezx = VRMAT_ROWCOL(mat, VR_Z, VR_X);	/* a3 */
	ewx = VRMAT_ROWCOL(mat, VR_W, VR_X);	/* a4 */

	exy = VRMAT_ROWCOL(mat, VR_X, VR_Y);	/* b1 */
	eyy = VRMAT_ROWCOL(mat, VR_Y, VR_Y);	/* b2 */
	ezy = VRMAT_ROWCOL(mat, VR_Z, VR_Y);	/* b3 */
	ewy = VRMAT_ROWCOL(mat, VR_W, VR_Y);	/* b4 */

	exz = VRMAT_ROWCOL(mat, VR_X, VR_Z);	/* c1 */
	eyz = VRMAT_ROWCOL(mat, VR_Y, VR_Z);	/* c2 */
	ezz = VRMAT_ROWCOL(mat, VR_Z, VR_Z);	/* c3 */
	ewz = VRMAT_ROWCOL(mat, VR_W, VR_Z);	/* c4 */

	exw = VRMAT_ROWCOL(mat, VR_X, VR_W);	/* d1 */
	eyw = VRMAT_ROWCOL(mat, VR_Y, VR_W);	/* d2 */
	ezw = VRMAT_ROWCOL(mat, VR_Z, VR_W);	/* d3 */
	eww = VRMAT_ROWCOL(mat, VR_W, VR_W);	/* d4 */

	result = exx * vr3x3Determinant(eyy, ezy, ewy, eyz, ezz, ewz, eyw, ezw, eww)
		- exy * vr3x3Determinant(eyx, ezx, ewx, eyz, ezz, ewz, eyw, ezw, eww)
		+ exz * vr3x3Determinant(eyx, ezx, ewx, eyy, ezy, ewy, eyw, ezw, eww)
		- exw * vr3x3Determinant(eyx, ezx, ewx, eyy, ezy, ewy, eyz, ezz, ewz);

	return result;
}


/**************************************************************************/
/* calculate the determinant of a 3x3 matrix.                             */
/*     | a1,  b1,  c1 |                                                   */
/*     | a2,  b2,  c2 |                                                   */
/*     | a3,  b3,  c3 |                                                   */
double vr3x3Determinant(double a1, double a2, double a3, double b1, double b2, double b3, double c1, double c2, double c3)
{
	double	result;

	result = a1 * vr2x2Determinant(b2, b3, c2, c3)
		- b1 * vr2x2Determinant(a2, a3, c2, c3)
		+ c1 * vr2x2Determinant(a2, a3, b2, b3);

	return result;
}


/********************************************************************/
double vr2x2Determinant(double a, double b, double c, double d)
{
	return (a * d - b * c);
}



/**************************************/
/****** Matrix Pre-Mult Routines ******/
/**************************************/


/******************************************************/
/* compute the matrix product, mat_l' = mat_r * mat_l */
/*   where "pre" is in C-style.                       */
/*   ie.  point * mat_l' = point * mat_r * mat_l      */
vrMatrix *vrMatrixPreMult(vrMatrix *mat_l, vrMatrix *mat_r)
{
	vrMatrix	mat_tmpmem;
	vrMatrix	*product = vrMatrixProduct(&mat_tmpmem, mat_r, mat_l);

	vrMatrixCopy(mat_l, product);

	return (mat_l);
}


/*************************************************************************/
/* translate the given matrix by composing it with a translation matrix. */
/*   ie, mat_l = transmat * mat_l                                        */
vrMatrix *vrMatrixPreTranslate3d(vrMatrix *mat_l, double x, double y, double z)
{
	vrMatrix	mat_tmpmem;
	vrMatrix	*tmat = vrMatrixSetTranslation3d(&mat_tmpmem, x, y, z);

	vrMatrixPreMult(mat_l, tmat);

	return (mat_l);
}


/*************************************************************************/
/* translate the given matrix by composing it with a translation matrix. */
/*   ie, mat_l = transmat * mat_l                                        */
vrMatrix *vrMatrixPreTranslateAd(vrMatrix *mat_l, double *array)
{
	vrMatrix	mat_tmpmem;
	vrMatrix	*tmat = vrMatrixSetTranslationAd(&mat_tmpmem, array);

	vrMatrixPreMult(mat_l, tmat);

	return (mat_l);
}


/*******************************************************************/
/* rotate the given matrix by composing it with a rotation matrix. */
/*    ie, mat_l = rotmat * mat_l                                   */
vrMatrix *vrMatrixPreRotateId(vrMatrix *mat_l, int axis, double theta)
{
	vrMatrix	mat_tmpmem;
	vrMatrix	*rmat = vrMatrixSetRotationId(&mat_tmpmem, axis, theta);

	vrMatrixPreMult(mat_l, rmat);

	return (mat_l);
}


/************************************************************************************/
/* TODO: 9/6/02 -- there is no need for this function, as the vrMatrixPreRotateId() */
/*   does the same as this, because vrMatrixSetRotationIdNew() is now the same as   */
/*   vrMatrixSetRotationId().                                                       */
vrMatrix *vrMatrixPreRotateIdNew(vrMatrix *mat_l, int axis, double theta)
{
	vrMatrix	mat_tmpmem;
	vrMatrix	*rmat = vrMatrixSetRotationIdNew(&mat_tmpmem, axis, theta);

	vrMatrixPreMult(mat_l, rmat);

	return (mat_l);
}


/*******************************************************************/
/* rotate the given matrix by composing it with a rotation matrix. */
/*    ie, mat_l = m2o * rotmat * mat_l * m2o-1                     */
/*     (or visa versa on the translation to and from the origin)   */
vrMatrix *vrMatrixPreRotateAboutOriginId(vrMatrix *mat_l, int axis, double theta)
{
	vrMatrix	mat_tmpmem;
	vrMatrix	*rmat = vrMatrixSetRotationId(&mat_tmpmem, axis, theta);
	vrVector	trans_vec;

	/* move matrix to origin and store offset */
	vrMatrixGetResetTranslationAd(mat_l, trans_vec.v);

	vrMatrixPreMult(mat_l, rmat);

	/* return to original translation */
	vrMatrixSetTranslationOnlyAd(mat_l, trans_vec.v);

	return (mat_l);
}


/*******************************************************************/
/* rotate the given matrix by composing it with a rotation matrix. */
/*    ie, mat_l = m2p * rotmat * mat_l * m2p-1                     */
/*     (or visa versa on the translation to and from the origin)   */
vrMatrix *vrMatrixPreRotateAboutPointId(vrMatrix *mat_l, vrPoint *point, int axis, double theta)
{
	vrMatrix	mat_tmpmem;
	vrMatrix	*rmat = vrMatrixSetRotationId(&mat_tmpmem, axis, theta);
	vrVector	trans_vec;

	/* move matrix to origin and store offset */
	vrMatrixGetResetTranslationAd(mat_l, trans_vec.v);

	/* now jump to the point to rotate about */
	vrMatrixSetTranslationOnlyAd(mat_l, point->v);

	vrMatrixPreMult(mat_l, rmat);

	/* return to original translation */
	vrMatrixSetTranslationOnlyAd(mat_l, trans_vec.v);

	return (mat_l);
}


/*******************************************************************/
/* rotate the given matrix by composing it with a rotation matrix. */
/*    ie, mat_l = rotmat * mat_l                                   */
vrMatrix *vrMatrixPreRotate4d(vrMatrix *mat_l, double x, double y, double z, double theta)
{
	vrMatrix	mat_tmpmem;
	vrMatrix	*rmat = vrMatrixSetRotation4d(&mat_tmpmem, x, y, z, theta);

	vrMatrixPreMult(mat_l, rmat);

	return (mat_l);
}


/******************************************************************/
/* scale the given matrix by composing it with a scaling matrix.  */
/*     ie, mat_l = scalemat * mat_l                               */
vrMatrix *vrMatrixPreScale3d(vrMatrix *mat_l, double sx, double sy, double sz)
{
	vrMatrix	mat_tmpmem;
	vrMatrix	*smat = vrMatrixSetScale3d(&mat_tmpmem, sx, sy, sz);

	vrMatrixPreMult(mat_l, smat);

	return (mat_l);
}

/***************************************/
/****** Matrix Post-Mult Routines ******/
/***************************************/


/********************************************************/
/* compute the matrix product, mat_l' = mat_l * mat_r   */
/*   ie. point * mat_l' = point * mat_l * mat_r         */
vrMatrix *vrMatrixPostMult(vrMatrix *mat_l, vrMatrix *mat_r)
{
	vrMatrix	mat_tmpmem;
	vrMatrix	*product = vrMatrixProduct(&mat_tmpmem, mat_l, mat_r);

	vrMatrixCopy(mat_l, product);

	return (mat_l);
}


/*************************************************************************/
/* translate the given matrix by composing it with a translation matrix. */
/*     ie, mat_l = mat_l * transmat                                      */
vrMatrix *vrMatrixPostTranslate3d(vrMatrix *mat_l, double x, double y, double z)
{
	vrMatrix	mat_tmpmem;
	vrMatrix	*tmat = vrMatrixSetTranslation3d(&mat_tmpmem, x, y, z);

	vrMatrixPostMult(mat_l, tmat);

	return (mat_l);
}


/*************************************************************************/
/* translate the given matrix by composing it with a translation matrix. */
/*     ie, mat_l = mat_l * transmat                                      */
vrMatrix *vrMatrixPostTranslateAd(vrMatrix *mat_l, double *array)
{
	vrMatrix	mat_tmpmem;
	vrMatrix	*tmat = vrMatrixSetTranslationAd(&mat_tmpmem, array);

	vrMatrixPostMult(mat_l, tmat);

	return (mat_l);
}


/*******************************************************************/
/* rotate the given matrix by composing it with a rotation matrix. */
/* ie, mat_l = mat_l * rotmat                                      */
vrMatrix *vrMatrixPostRotateId(vrMatrix *mat_l, int axis, double theta)
{
	vrMatrix	mat_tmpmem;
	vrMatrix	*rmat = vrMatrixSetRotationId(&mat_tmpmem, axis, theta);

	vrMatrixPostMult(mat_l, rmat);

	return (mat_l);
}


/************************************************************************************/
/* TODO: 9/6/02 -- there is no need for this function, as the vrMatrixPostRotateId()*/
/*   does the same as this, because vrMatrixSetRotationIdNew() is now the same as   */
/*   vrMatrixSetRotationId().                                                       */
vrMatrix *vrMatrixPostRotateIdNew(vrMatrix *mat_l, int axis, double theta)
{
	vrMatrix	mat_tmpmem;
	vrMatrix	*rmat = vrMatrixSetRotationIdNew(&mat_tmpmem, axis, theta);

	vrMatrixPostMult(mat_l, rmat);

	return (mat_l);
}


/*******************************************************************/
/* rotate the given matrix by composing it with a rotation matrix. */
/*    ie, mat_l = m2o * mat_l * rotmat * m2o-1                     */
/*     (or visa versa on the translation to and from the origin)   */
vrMatrix *vrMatrixPostRotateAboutOriginId(vrMatrix *mat_l, int axis, double theta)
{
	vrMatrix	mat_tmpmem;
	vrMatrix	*rmat = vrMatrixSetRotationId(&mat_tmpmem, axis, theta);
	vrVector	trans_vec;

	/* move matrix to origin and store offset */
	vrMatrixGetResetTranslationAd(mat_l, trans_vec.v);

	vrMatrixPostMult(mat_l, rmat);

	/* return to original translation */
	vrMatrixSetTranslationOnlyAd(mat_l, trans_vec.v);

	return (mat_l);
}


/*******************************************************************/
/* rotate the given matrix by composing it with a rotation matrix. */
/*    ie, mat_l = m2p * mat_l * rotmat * m2p-1                     */
/*     (or visa versa on the translation to and from the origin)   */
vrMatrix *vrMatrixPostRotateAboutPointId(vrMatrix *mat_l, vrPoint *point, int axis, double theta)
{
	vrMatrix	mat_tmpmem;
	vrMatrix	*rmat = vrMatrixSetRotationId(&mat_tmpmem, axis, theta);
	vrVector	trans_vec;

	/* move matrix to origin and store offset */
	vrMatrixGetResetTranslationAd(mat_l, trans_vec.v);

	/* now jump to the point to rotate about */
	vrMatrixSetTranslationOnlyAd(mat_l, point->v);

	vrMatrixPostMult(mat_l, rmat);

	/* return to original translation */
	vrMatrixSetTranslationOnlyAd(mat_l, trans_vec.v);

	return (mat_l);
}


/*******************************************************************/
/* rotate the given matrix by composing it with a rotation matrix. */
/*     ie, mat_l = mat_l * rotmat                                  */
vrMatrix *vrMatrixPostRotate4d(vrMatrix *mat_l, double x, double y, double z, double theta)
{
	vrMatrix	mat_tmpmem;
	vrMatrix	*rmat = vrMatrixSetRotation4d(&mat_tmpmem, x, y, z, theta);

	vrMatrixPostMult(mat_l, rmat);

	return (mat_l);
}


/*****************************************************************/
/* scale the given matrix by composing it with a scaling matrix. */
/*      ie, mat_l = mat_l * scalemat                             */
vrMatrix *vrMatrixPostScale3d(vrMatrix *mat_l, double sx, double sy, double sz)
{
	vrMatrix	mat_tmpmem;
	vrMatrix	*smat = vrMatrixSetScale3d(&mat_tmpmem, sx, sy, sz);

	vrMatrixPostMult(mat_l, smat);

	return (mat_l);
}



/* ==================================================== */
/* QUATERNION OPERATIONS                                */
/* ==================================================== */

/*********************************************************************/
/* create an empty quaternion                                        */
vrQuat *vrQuatCreate(void)
{
	return (vrQuat *)vrShmemAlloc0(sizeof(vrQuat));
}


/*********************************************************************/
/* create a quaternion representing the specified rotation           */
vrQuat *vrQuatSet4d(vrQuat *quat, double theta, double x, double y, double z)
{
	double	theta_2 = theta / 2.0f;
	double	sint = sin(vrDegreesToRadians(theta_2));
	double	cost = cos(vrDegreesToRadians(theta_2));

	quat->v[VR_X] = x * sint;
	quat->v[VR_Y] = y * sint;
	quat->v[VR_Z] = z * sint;
	quat->v[VR_W] = cost;

	return quat;
}


/*********************************************************************/
/* create the product of the two given quaternions, p = ql * qr      */
/*   to create a new product from quats ql & qr:                     */
/*      p = vrQuatProduct(vrQuatCreate(), ql, qr)                    */
vrQuat *vrQuatProduct(vrQuat *prod, vrQuat *ql, vrQuat *qr)
{
	prod->v[VR_X] = (ql->v[VR_W]*qr->v[VR_X]) + (ql->v[VR_X]*qr->v[VR_W]) +
			(ql->v[VR_Y]*qr->v[VR_Z]) - (ql->v[VR_Z]*qr->v[VR_Y]);
	prod->v[VR_Y] = (ql->v[VR_W]*qr->v[VR_Y]) + (ql->v[VR_Y]*qr->v[VR_W]) +
			(ql->v[VR_Z]*qr->v[VR_X]) - (ql->v[VR_X]*qr->v[VR_Z]);
	prod->v[VR_Z] = (ql->v[VR_W]*qr->v[VR_Z]) + (ql->v[VR_Z]*qr->v[VR_W]) +
			(ql->v[VR_X]*qr->v[VR_Y]) - (ql->v[VR_Y]*qr->v[VR_X]);
	prod->v[VR_W] = (ql->v[VR_W]*qr->v[VR_W]) - (ql->v[VR_X]*qr->v[VR_X]) -
			(ql->v[VR_Y]*qr->v[VR_Y]) - (ql->v[VR_Z]*qr->v[VR_Z]);

	return prod;
}


/*********************************************************************/
/* create the product of the two given quaternions, ql *= qr        */
vrQuat *vrQuatProductEquals(vrQuat *tmpquat, vrQuat *ql, vrQuat *qr)
{
	vrQuatCopy(ql, vrQuatProduct(tmpquat, ql, qr));

	return ql;
}


/**************************************************************************************/
/* Convert the given quaternion from Y-UP coordinate system to Z-UP: ie, ql = Y2Z(qr) */
vrQuat *vrQuatConvertYUPtoZUP(vrQuat *dst, vrQuat *src)
{
	vrQuat	swap;	/* used to make it safe to give the same value for both arguments */
	swap.v[VR_X] = src->v[VR_X];
	swap.v[VR_Y] = src->v[VR_Y];
	swap.v[VR_Z] = src->v[VR_Z];
	swap.v[VR_W] = src->v[VR_W];

	dst->v[VR_X] = -swap.v[VR_X];
	dst->v[VR_Y] =  swap.v[VR_Z];
	dst->v[VR_Z] = -swap.v[VR_Y];
	dst->v[VR_W] =  swap.v[VR_W];

	return (dst);
}


/**************************************************************************************/
/* Convert the given quaternion from Y-UP coordinate system to Z-UP: ie, ql = Z2Y(qr) */
vrQuat *vrQuatConvertZUPtoYUP(vrQuat *dst, vrQuat *src)
{
	vrQuat	swap;	/* used to make it safe to give the same value for both arguments */
	swap.v[VR_X] = src->v[VR_X];
	swap.v[VR_Y] = src->v[VR_Y];
	swap.v[VR_Z] = src->v[VR_Z];
	swap.v[VR_W] = src->v[VR_W];

	dst->v[VR_X] = -swap.v[VR_X];
	dst->v[VR_Y] = -swap.v[VR_Z];
	dst->v[VR_Z] =  swap.v[VR_Y];
	dst->v[VR_W] =  swap.v[VR_W];

	return (dst);
}


/*********************************************************************/
/* copy the given quaternion: ie, ql = qr                            */
vrQuat *vrQuatCopy(vrQuat *ql, vrQuat *qr)
{
	return ((vrQuat *)memcpy(ql, qr, sizeof(vrQuat)));
}


/*********************************************************************/
/* delete the given quaternion                                       */
void vrQuatDelete(vrQuat *quat)
{
	vrShmemFree(quat);
}



/* ==================================================== */
/* VECTOR OPERATIONS                                    */
/* ==================================================== */

/*********************************************************************/
vrVector *vrVectorCopy(vrVector *vec1, vrVector *vec2)
{
	return ((vrVector *)memcpy(vec1, vec2, sizeof(vrVector)));
}


/*********************************************************************/
/* calculate the vector from point-1 to point-2                      */
vrVector *vrVectorFromTwoPoints(vrVector *dst, vrPoint *pnt1, vrPoint *pnt2)
{
	dst->v[VR_X] = pnt2->v[VR_X] - pnt1->v[VR_X];
	dst->v[VR_Y] = pnt2->v[VR_Y] - pnt1->v[VR_Y];
	dst->v[VR_Z] = pnt2->v[VR_Z] - pnt1->v[VR_Z];

	return dst;
}


/*********************************************************************/
/* NOTE: src and dst can point to the same point.                    */
vrVector *vrVectorNormalize(vrVector *dst, vrVector *src)
{
	double	magnitude;

	magnitude = sqrt(src->v[VR_X]*src->v[VR_X] + src->v[VR_Y]*src->v[VR_Y] + src->v[VR_Z]*src->v[VR_Z]);

	dst->v[VR_X] = src->v[VR_X] / magnitude;
	dst->v[VR_Y] = src->v[VR_Y] / magnitude;
	dst->v[VR_Z] = src->v[VR_Z] / magnitude;

	return dst;
}


/*********************************************************************/
double vrVectorLength(vrVector *vec)
{
	double	magnitude;

	magnitude = sqrt(vec->v[VR_X]*vec->v[VR_X] + vec->v[VR_Y]*vec->v[VR_Y] + vec->v[VR_Z]*vec->v[VR_Z]);

	return magnitude;
}


/*********************************************************************/
double vrVectorDotProduct(vrVector *vec1, vrVector *vec2)
{
	double	result;

	result  = vec1->v[VR_X] * vec2->v[VR_X];
	result += vec1->v[VR_Y] * vec2->v[VR_Y];
	result += vec1->v[VR_Z] * vec2->v[VR_Z];

	return result;
}


/*********************************************************************/
/* NOTE: unlike many of the other vector operations, it is NOT SAFE  */
/*   to use the same memory pointer for the result (dst) and either  */
/*   of the operands.  We may want to consider fixing this.          */
vrVector *vrVectorCrossProduct(vrVector *dst, vrVector *vec1, vrVector *vec2)
{
	vrVector	result;

	result.v[VR_X] = vec1->v[VR_Y]*vec2->v[VR_Z] - vec1->v[VR_Z]*vec2->v[VR_Y];
	result.v[VR_Y] = vec1->v[VR_Z]*vec2->v[VR_X] - vec1->v[VR_X]*vec2->v[VR_Z];
	result.v[VR_Z] = vec1->v[VR_X]*vec2->v[VR_Y] - vec1->v[VR_Y]*vec2->v[VR_X];

	*dst = result;

	return dst;
}


/*********************************************************************/
/* NOTE: this changes the vector passed as the (first) argument      */
vrVector *vrVectorScale(vrVector *vec, double scale)
{
	vec->v[VR_X] *= scale;
	vec->v[VR_Y] *= scale;
	vec->v[VR_Z] *= scale;

	return vec;
}


/********************************************************************************************/
/* NOTE: this should be safe for result_vec to be the same as either vec1 or vec2 (or both) */
vrVector *vrVectorSubtract(vrVector *result_vec, vrVector *vec1, vrVector *vec2)
{
	result_vec->v[VR_X] = vec1->v[VR_X] - vec2->v[VR_X];
	result_vec->v[VR_Y] = vec1->v[VR_Y] - vec2->v[VR_Y];
	result_vec->v[VR_Z] = vec1->v[VR_Z] - vec2->v[VR_Z];

	return result_vec;
}


/**********************************************************************/
/* it is safe to pass the source and destination vectors as the same. */
/* NOTE: this is the equivalent of extending the 3-tuple vector by    */
/*   one element with a value of 0, and then multiplying the 4-tuple  */
/*   by the 4x4 matrix.                                               */
/* NOTE: this is a Vector operation, so translations have no effect!  */
vrVector *vrVectorTransformByMatrix(vrVector *dst_vec, vrVector *src_vec, vrMatrix *src_mat)
{
	int		row;
	vrVector	src_tmp;		/* store here in case src and dst are the same */

	src_tmp = *src_vec;
	for (row = VR_X; row <= VR_Z; row++) {
		dst_vec->v[row] = src_tmp.v[VR_X] * VRMAT_ROWCOL(src_mat, row, VR_X) +
				  src_tmp.v[VR_Y] * VRMAT_ROWCOL(src_mat, row, VR_Y) +
				  src_tmp.v[VR_Z] * VRMAT_ROWCOL(src_mat, row, VR_Z);
	}

	return dst_vec;
}


/*************************************************************************************/
/* it is (currently) NOT safe to pass the source and dest as the same vector memory  */
/* NOTE: this function is (currently) only called by vrMatrixCompositionFromMatrix() */
/*   which also is using VRMAT_COLROW -- probably in an attempt to fix the latter.   */
vrVector4 *vrVector4TransformByMatrix(vrVector4 *dst_vec, vrVector4 *src_vec, vrMatrix *src_mat)
{
	dst_vec->v[VR_X] = (src_vec->v[VR_X] * VRMAT_COLROW(src_mat, VR_X, VR_X)) +
			   (src_vec->v[VR_Y] * VRMAT_COLROW(src_mat, VR_Y, VR_X)) +
			   (src_vec->v[VR_Z] * VRMAT_COLROW(src_mat, VR_Z, VR_X)) +
			   (src_vec->v[VR_W] * VRMAT_COLROW(src_mat, VR_W, VR_X));

	dst_vec->v[VR_Y] = (src_vec->v[VR_X] * VRMAT_COLROW(src_mat, VR_X, VR_Y)) +
			   (src_vec->v[VR_Y] * VRMAT_COLROW(src_mat, VR_Y, VR_Y)) +
			   (src_vec->v[VR_Z] * VRMAT_COLROW(src_mat, VR_Z, VR_Y)) +
			   (src_vec->v[VR_W] * VRMAT_COLROW(src_mat, VR_W, VR_Y));

	dst_vec->v[VR_Z] = (src_vec->v[VR_X] * VRMAT_COLROW(src_mat, VR_X, VR_Z)) +
			   (src_vec->v[VR_Y] * VRMAT_COLROW(src_mat, VR_Y, VR_Z)) +
			   (src_vec->v[VR_Z] * VRMAT_COLROW(src_mat, VR_Z, VR_Z)) +
			   (src_vec->v[VR_W] * VRMAT_COLROW(src_mat, VR_W, VR_Z));

	dst_vec->v[VR_W] = (src_vec->v[VR_X] * VRMAT_COLROW(src_mat, VR_X, VR_W)) +
			   (src_vec->v[VR_Y] * VRMAT_COLROW(src_mat, VR_Y, VR_W)) +
			   (src_vec->v[VR_Z] * VRMAT_COLROW(src_mat, VR_Z, VR_W)) +
			   (src_vec->v[VR_W] * VRMAT_COLROW(src_mat, VR_W, VR_W));


	return dst_vec;
}


/*********************************************************************/
vrVector *vrVectorAddScaledVectors(vrVector *result_vec, vrVector *vec1, vrVector *vec2, double scale1, double scale2)
{
	result_vec->v[VR_X] = vec1->v[VR_X] * scale1 + vec2->v[VR_X] * scale2;
	result_vec->v[VR_Y] = vec1->v[VR_Y] * scale1 + vec2->v[VR_Y] * scale2;
	result_vec->v[VR_Z] = vec1->v[VR_Z] * scale1 + vec2->v[VR_Z] * scale2;

	return result_vec;
}



/* ==================================================== */
/* POINT OPERATIONS                                     */
/* ==================================================== */

/*********************************************************************/
vrPoint *vrPointCreate(void)
{
	return (vrPoint *)vrShmemAlloc0(sizeof(vrPoint));
}


/*********************************************************************/
vrPoint *vrPointCreate3d(double x, double y, double z)
{
	vrPoint	*pt = vrPointCreate();

	vrPointSet3d(pt, x, y, z);

	return pt;
}


/*********************************************************************/
vrPoint *vrPointCopy(vrPoint *p1, vrPoint *p2)
{
	return ((vrPoint *)memcpy(p1, p2, sizeof(vrPoint)));
}


/*********************************************************************/
void vrPointDelete(vrPoint *pnt)
{
	vrShmemFree(pnt);
}


/*********************************************************************/
vrPoint	*vrPointAddScaledVector(vrPoint *result_pnt, vrPoint *pnt, vrVector *vec, double scale)
{
	result_pnt->v[VR_X] = pnt->v[VR_X] + vec->v[VR_X] * scale;
	result_pnt->v[VR_Y] = pnt->v[VR_Y] + vec->v[VR_Y] * scale;
	result_pnt->v[VR_Z] = pnt->v[VR_Z] + vec->v[VR_Z] * scale;

	return result_pnt;
}


/* ==================================================== */
/* TRANSFORMATIONS AND CONVERSIONS                      */
/* ==================================================== */


/*********************************************************************/
/* vrMatrixSetRotationFromQuatRC() takes a quaternion and a pointer to */
/*   a matrix and inserts into (overwrites) the matrix the rotation  */
/*   portion of the matrix with the rotation represented by the      */
/*   quaternion.                                                     */
/* Note: this function does not alter the translation portion of the */
/*   4x4 homogeneous matrix.                                         */
/* NOTE: [03/18/2015: I've determined today that this is the wrong way to do it and the "CR" version is correct, so I need to begin the process of correcting that. */
vrMatrix *vrMatrixSetRotationFromQuatRC(vrMatrix *mat, vrQuat *quat)
{
	double		xs,ys,zs,wx,wy,wz,xx,xy,xz,yy,yz,zz;

#if 0
	vrPrintQuat("vrMatrixSetRotationFromQuatRC() -- quat", quat);
#endif

	xs = quat->v[VR_X] * 2.0;
	ys = quat->v[VR_Y] * 2.0;
	zs = quat->v[VR_Z] * 2.0;

	wx = quat->v[VR_W] * xs;
	wy = quat->v[VR_W] * ys;
	wz = quat->v[VR_W] * zs;
	xx = quat->v[VR_X] * xs;
	xy = quat->v[VR_X] * ys;
	xz = quat->v[VR_X] * zs;
	yy = quat->v[VR_Y] * ys;
	yz = quat->v[VR_Y] * zs;
	zz = quat->v[VR_Z] * zs;

#if 0 /* when set to "0", this uses row-column ordering, which i think (as of 03/18/2015) is incorrect, but at this point there are other parts of the library that rely on it -- so I need to do a global fix, which may also involve fixing configuration files! */
	/* NOTE: we're using COLROW here instead of our usual ROWCOL, because  */
	/*   the forumula we're using to convert from a Quaternion to a matrix */
	/*   assumes the normal matrix ordering, which is to say, not the      */
	/*   OpenGL ordering, which is what FreeVR uses.                       */
	VRMAT_COLROW(mat, VR_X, VR_X) = 1.0 - yy - zz;
	VRMAT_COLROW(mat, VR_X, VR_Y) = xy + wz;
	VRMAT_COLROW(mat, VR_X, VR_Z) = xz - wy;

	VRMAT_COLROW(mat, VR_Y, VR_X) = xy - wz;
	VRMAT_COLROW(mat, VR_Y, VR_Y) = 1.0 - xx - zz;
	VRMAT_COLROW(mat, VR_Y, VR_Z) = yz + wx;

	VRMAT_COLROW(mat, VR_Z, VR_X) = xz + wy;
	VRMAT_COLROW(mat, VR_Z, VR_Y) = yz - wx;
	VRMAT_COLROW(mat, VR_Z, VR_Z) = 1.0 - xx - yy;
#else
	/* 05/26/2006: NOTE further (or instead), when I use the COLROW, my */
	/*   configuration is backward! (ie. I get t2r_xform & r2e_xform    */
	/*   values that don't work with the configuration I've already     */
	/*   worked out.)  So, what is the solution???                      */
	VRMAT_ROWCOL(mat, VR_X, VR_X) = 1.0 - yy - zz;
	VRMAT_ROWCOL(mat, VR_X, VR_Y) = xy + wz;
	VRMAT_ROWCOL(mat, VR_X, VR_Z) = xz - wy;

	VRMAT_ROWCOL(mat, VR_Y, VR_X) = xy - wz;
	VRMAT_ROWCOL(mat, VR_Y, VR_Y) = 1.0 - xx - zz;
	VRMAT_ROWCOL(mat, VR_Y, VR_Z) = yz + wx;

	VRMAT_ROWCOL(mat, VR_Z, VR_X) = xz + wy;
	VRMAT_ROWCOL(mat, VR_Z, VR_Y) = yz - wx;
	VRMAT_ROWCOL(mat, VR_Z, VR_Z) = 1.0 - xx - yy;
#endif

#if 0
	vrPrintMatrix("vrMatrixSetRotationFromQuatRC() -- matrix", mat);
#endif
	return mat;
}


#if 1 /* it's time to get rid of this mistake [03/18/2015 -- actually it's time to make this the real version!] */
/* same as above, but using the COLROW method */
/* 07/01/2014: I suspect that I should get rid of this function.  It seems to be */
/*   used as a means to convert data from Z-up to Y-up coordinate systems,       */
/*   though without any real understanding that that's what was going on.        */
/* 03/18/2015: And today I suspect it's the other version that we can get rid of */
/*   so I've renamed this one to have the normal name, and the other one to be   */
/*   oddly named, and now destined for deprecation.                              */
vrMatrix *vrMatrixSetRotationFromQuat(vrMatrix *mat, vrQuat *quat)
{
	double		xs,ys,zs,wx,wy,wz,xx,xy,xz,yy,yz,zz;

#if 0
	vrPrintQuat("vrMatrixSetRotationFromQuatCR() -- quat", quat);
#endif

	xs = quat->v[VR_X] * 2.0;
	ys = quat->v[VR_Y] * 2.0;
	zs = quat->v[VR_Z] * 2.0;

	wx = quat->v[VR_W] * xs;
	wy = quat->v[VR_W] * ys;
	wz = quat->v[VR_W] * zs;
	xx = quat->v[VR_X] * xs;
	xy = quat->v[VR_X] * ys;
	xz = quat->v[VR_X] * zs;
	yy = quat->v[VR_Y] * ys;
	yz = quat->v[VR_Y] * zs;
	zz = quat->v[VR_Z] * zs;

#if 1
	/* NOTE: we're using COLROW here instead of our usual ROWCOL, because */
	/*   for formula we're using to convert from a Quaternion to a matrix */
	/*   assumes the normal matrix ordering, which is to say, not the     */
	/*   OpenGL ordering, which is what FreeVR uses.                      */
	VRMAT_COLROW(mat, VR_X, VR_X) = 1.0 - yy - zz;
	VRMAT_COLROW(mat, VR_X, VR_Y) = xy + wz;
	VRMAT_COLROW(mat, VR_X, VR_Z) = xz - wy;

	VRMAT_COLROW(mat, VR_Y, VR_X) = xy - wz;
	VRMAT_COLROW(mat, VR_Y, VR_Y) = 1.0 - xx - zz;
	VRMAT_COLROW(mat, VR_Y, VR_Z) = yz + wx;

	VRMAT_COLROW(mat, VR_Z, VR_X) = xz + wy;
	VRMAT_COLROW(mat, VR_Z, VR_Y) = yz - wx;
	VRMAT_COLROW(mat, VR_Z, VR_Z) = 1.0 - xx - yy;
#else
	/* 05/26/2006: NOTE further (or instead), when I use the COLROW, my */
	/*   configuration is backward! (ie. I get t2r_xform & r2e_xform    */
	/*   values that don't work with the configuration I've already     */
	/*   worked out.)  So, what is the solution???                      */
	VRMAT_ROWCOL(mat, VR_X, VR_X) = 1.0 - yy - zz;
	VRMAT_ROWCOL(mat, VR_X, VR_Y) = xy + wz;
	VRMAT_ROWCOL(mat, VR_X, VR_Z) = xz - wy;

	VRMAT_ROWCOL(mat, VR_Y, VR_X) = xy - wz;
	VRMAT_ROWCOL(mat, VR_Y, VR_Y) = 1.0 - xx - zz;
	VRMAT_ROWCOL(mat, VR_Y, VR_Z) = yz + wx;

	VRMAT_ROWCOL(mat, VR_Z, VR_X) = xz + wy;
	VRMAT_ROWCOL(mat, VR_Z, VR_Y) = yz - wx;
	VRMAT_ROWCOL(mat, VR_Z, VR_Z) = 1.0 - xx - yy;
#endif

#if 0
	vrPrintMatrix("vrMatrixSetRotationFromQuatCR() -- matrix", mat);
#endif
	return mat;
}
#endif


/*********************************************************************/
vrMatrix *vrMatrixSetFromQuatRC(vrMatrix *mat, vrQuat *quat)
{

	vrMatrixSetRotationFromQuatRC(mat, quat);

	/* reset translation to none */
	VRMAT_ROWCOL(mat, VR_W, VR_X) = VRMAT_ROWCOL(mat, VR_W, VR_Y) = VRMAT_ROWCOL(mat, VR_W, VR_Z) = 0.0f;

	/* reset the 4th column to normal */
	VRMAT_ROWCOL(mat, VR_X, VR_W) = 0.0f;
	VRMAT_ROWCOL(mat, VR_Y, VR_W) = 0.0f;
	VRMAT_ROWCOL(mat, VR_Z, VR_W) = 0.0f;
	VRMAT_ROWCOL(mat, VR_W, VR_W) = 1.0f;

	return mat;
}


/*********************************************************************/
vrQuat *vrQuatSetFromMatrix(vrQuat *quat, vrMatrix *mat)
{
#if 0 /* this is Ed's method (set to '1'), for new method, set to '0' */
	double	w_2, x_2, y_2;
	double	s;
	double	len;

#if 0
	vrPrintMatrix("vrQuatSetFromMatrix() -- matrix", mat);
#endif

	w_2 = (1.0 + VRMAT_ROWCOL(mat, VR_X, VR_X) + VRMAT_ROWCOL(mat, VR_Y, VR_Y) + VRMAT_ROWCOL(mat, VR_Z, VR_Z)) * .25;

	if (w_2 > FLT_EPSILON) {
		quat->v[VR_W] = sqrt(w_2);
		s = 4.0 * quat->v[VR_W];
		quat->v[VR_X] = (VRMAT_ROWCOL(mat, VR_Y, VR_Z) - VRMAT_ROWCOL(mat, VR_Z, VR_Y)) / s;
		quat->v[VR_Y] = (VRMAT_ROWCOL(mat, VR_Z, VR_X) - VRMAT_ROWCOL(mat, VR_X, VR_Z)) / s;
		quat->v[VR_Z] = (VRMAT_ROWCOL(mat, VR_X, VR_Y) - VRMAT_ROWCOL(mat, VR_Y, VR_X)) / s;
	} else {
		quat->v[VR_W] = 0.0;
		x_2 = (VRMAT_ROWCOL(mat, VR_Y, VR_Y) + VRMAT_ROWCOL(mat, VR_Z, VR_Z)) * -0.5;

		if (x_2 > FLT_EPSILON) {
			quat->v[VR_X] = sqrt(x_2);
			s = 2.0 * quat->v[VR_X];
			quat->v[VR_Y] = VRMAT_ROWCOL(mat, VR_X, VR_Y) / s;
			quat->v[VR_Z] = VRMAT_ROWCOL(mat, VR_X, VR_Z) / s;
		} else {
			quat->v[VR_X] = 0.0;
			y_2 = (1.0 - VRMAT_ROWCOL(mat, VR_Z, VR_Z)) * 0.5;

			if (y_2 > FLT_EPSILON) {

				quat->v[VR_Y] = sqrt(y_2);
				s = 2.0 * quat->v[VR_Y];
				quat->v[VR_Z] = VRMAT_ROWCOL(mat, VR_Y, VR_Z) / s;

			} else {

				quat->v[VR_Y] = 0.0;
				quat->v[VR_Z] = 1.0;

			}
		}
	}

	/* Normalize the quaternion */
	len = sqrt(vrQuatLengthSq(quat));

	quat->v[VR_X] = quat->v[VR_X] / len;
	quat->v[VR_Y] = quat->v[VR_Y] / len;
	quat->v[VR_Z] = quat->v[VR_Z] / len;
	quat->v[VR_W] = quat->v[VR_W] / len;

#else

	/* This version of the calculation come from the webpage:   */
	/*   http://skal.planet-d.net/demo/matrixfaq.htm#Q55        */
	double	trace;
	double	scale;

	trace = 1.0 + VRMAT_ROWCOL(mat, VR_X, VR_X) + VRMAT_ROWCOL(mat, VR_Y, VR_Y) + VRMAT_ROWCOL(mat, VR_Z, VR_Z);

	/* test for epsilon above zero to avoid distortions that can arise when near zero */
	if (trace > FLT_EPSILON) {
		scale = sqrt(trace) * 2.0;
		quat->v[VR_W] = scale * 0.25;
		quat->v[VR_X] = (VRMAT_ROWCOL(mat, VR_Y, VR_Z) - VRMAT_ROWCOL(mat, VR_Z, VR_Y)) / scale;
		quat->v[VR_Y] = (VRMAT_ROWCOL(mat, VR_Z, VR_X) - VRMAT_ROWCOL(mat, VR_X, VR_Z)) / scale;
		quat->v[VR_Z] = (VRMAT_ROWCOL(mat, VR_X, VR_Y) - VRMAT_ROWCOL(mat, VR_Y, VR_X)) / scale;
	} else {
		/* If the trace of the matrix is equal to zero (or nearly) then identify */
		/*   which major diagonal element has the greatest value.                */
		/* Depending on this, calculate the following:                           */

		if (VRMAT_ROWCOL(mat, VR_X, VR_X) > VRMAT_ROWCOL(mat, VR_Y, VR_Y) &&
			VRMAT_ROWCOL(mat, VR_X, VR_X) > VRMAT_ROWCOL(mat, VR_Z, VR_Z))  {

			/* X,X is greatest */
			scale  = sqrt(1.0 + VRMAT_ROWCOL(mat, VR_X, VR_X) - VRMAT_ROWCOL(mat, VR_Y, VR_Y) - VRMAT_ROWCOL(mat, VR_Z, VR_Z)) * 2;
			quat->v[VR_W] = (VRMAT_ROWCOL(mat, VR_Y, VR_Z) - VRMAT_ROWCOL(mat, VR_Z, VR_Y)) / scale;
			quat->v[VR_X] = 0.25 * scale;
			quat->v[VR_Y] = (VRMAT_ROWCOL(mat, VR_X, VR_Y) + VRMAT_ROWCOL(mat, VR_X, VR_Y)) / scale;
			quat->v[VR_Z] = (VRMAT_ROWCOL(mat, VR_X, VR_Z) + VRMAT_ROWCOL(mat, VR_X, VR_Z)) / scale;
		} else if (VRMAT_ROWCOL(mat, VR_Y, VR_Y) > VRMAT_ROWCOL(mat, VR_Z, VR_Z)) {
			/* Y,Y is greatest */
			scale  = sqrt(1.0 + VRMAT_ROWCOL(mat, VR_Y, VR_Y) - VRMAT_ROWCOL(mat, VR_X, VR_X) - VRMAT_ROWCOL(mat, VR_Z, VR_Z)) * 2;
			quat->v[VR_W] = (VRMAT_ROWCOL(mat, VR_X, VR_Z) - VRMAT_ROWCOL(mat, VR_X, VR_Z)) / scale;
			quat->v[VR_X] = (VRMAT_ROWCOL(mat, VR_X, VR_Y) + VRMAT_ROWCOL(mat, VR_X, VR_Y)) / scale;
			quat->v[VR_Y] = 0.25 * scale;
			quat->v[VR_Z] = (VRMAT_ROWCOL(mat, VR_Y, VR_Z) + VRMAT_ROWCOL(mat, VR_Z, VR_Y)) / scale;
		} else {
			/* Z,Z is greatest  */
			scale  = sqrt(1.0 + VRMAT_ROWCOL(mat, VR_Z, VR_Z) - VRMAT_ROWCOL(mat, VR_X, VR_X) - VRMAT_ROWCOL(mat, VR_Y, VR_Y)) * 2;
			quat->v[VR_W] = (VRMAT_ROWCOL(mat, VR_X, VR_Y) - VRMAT_ROWCOL(mat, VR_X, VR_Y)) / scale;
			quat->v[VR_X] = (VRMAT_ROWCOL(mat, VR_X, VR_Z) + VRMAT_ROWCOL(mat, VR_X, VR_Z)) / scale;
			quat->v[VR_Y] = (VRMAT_ROWCOL(mat, VR_Y, VR_Z) + VRMAT_ROWCOL(mat, VR_Z, VR_Y)) / scale;
			quat->v[VR_Z] = 0.25 * scale;
		}
	}
#endif

#if 0
	vrPrintQuat("vrQuatSetFromMatrix() -- quat", quat);
#endif

	return quat;
}


/*********************************************************************/
/* p = q p q^(-1)                                                    */
vrPoint *vrPointTransformByQuat(vrPoint *dst_pnt, vrPoint *src_pnt, vrQuat *quat)
{
	vrErr("vrPointTransformByQuat() -- not implemented yet");
	return src_pnt;
}


/*********************************************************************/
/* dp = m * sp                                                       */
/* NOTE: src_pnt and dst_pnt can point to the same point.            */
/* NOTE: this is the equivalent of extending the 3-tuple vector by   */
/*   one element with a value of 1, and then multiplying the 4-tuple */
/*   by the 4x4 matrix.                                              */
vrPoint *vrPointTransformByMatrix(vrPoint *dst_pnt, vrPoint *src_pnt, vrMatrix *mat)
{
	double	tmp[4];
	int	row;
	int	col;

	for (row = VR_X; row <= VR_W; row++) {
		tmp[row] = 0.0f;
		for (col = VR_X; col <= VR_Z; col++)
			tmp[row] += VRMAT_ROWCOL(mat, row, col) * src_pnt->v[col];
		tmp[row] += VRMAT_ROWCOL(mat, row, VR_W);
	}
	for (row = VR_X; row <= VR_Z; row++)
		dst_pnt->v[row] = tmp[row] / tmp[VR_W];

#ifndef MATH_ONLY
	if (vrDbgDo(ALMOSTNEVER_DBGLVL/2))
		vrPrintMatrix("vrMatrixXformPoint():", mat);
#endif

	return dst_pnt;
}


/*********************************************************************/
/* get the translation values into a vrPoint from a 4x4 mat          */
/*   TODO: usage of this should go away once we fully adopt the vrVector operations */
/*      9/7/02 -- why?  It appears vrVectorGetTransFromMatrix() is even being used  */
/*        in vr_entity.c where this function is more appropriate.                   */
/*      9/25/02 -- now vr_entity.c is using this function.                          */
vrPoint *vrPointGetTransFromMatrix(vrPoint *dst_pnt, vrMatrix *src_mat)
{
	dst_pnt->v[VR_X] = VRMAT_ROWCOL(src_mat, VR_X, VR_W);
	dst_pnt->v[VR_Y] = VRMAT_ROWCOL(src_mat, VR_Y, VR_W);
	dst_pnt->v[VR_Z] = VRMAT_ROWCOL(src_mat, VR_Z, VR_W);

	return dst_pnt;
}


/*********************************************************************/
vrVector *vrVectorGetTransFromMatrix(vrVector *dst_vec, vrMatrix *src_mat)
{
	dst_vec->v[VR_X] = VRMAT_ROWCOL(src_mat, VR_X, VR_W);
	dst_vec->v[VR_Y] = VRMAT_ROWCOL(src_mat, VR_Y, VR_W);
	dst_vec->v[VR_Z] = VRMAT_ROWCOL(src_mat, VR_Z, VR_W);

	return dst_vec;
}


/*********************************************************************/
vrMatrix *vrMatrixSetTransFromVector(vrMatrix *dst_mat, vrVector *src_vec)
{
	VRMAT_ROWCOL(dst_mat, VR_X, VR_W) = src_vec->v[VR_X];
	VRMAT_ROWCOL(dst_mat, VR_Y, VR_W) = src_vec->v[VR_Y];
	VRMAT_ROWCOL(dst_mat, VR_Z, VR_W) = src_vec->v[VR_Z];

	return dst_mat;
}


/*********************************************************************/
double vrDegreesToRadians(double deg)
{
	return (deg * M_PI) / 180.0f;
}


/*********************************************************************/
double vrRadiansToDegrees(double rad)
{
	return (rad * 180.0) / M_PI;
}



/* ==================================================== */
/* DEBUGGING PRINTERS                                   */
/* ==================================================== */


/*********************************************************************/
/* NOTE: this prints the matrix in OpenGL-style -- ie. with          */
/*   the translation values down the right column.                   */
/* Here is how the OpenGL-style matrix corresponds to                */
/*   the matrix array elements:                                      */
/*        v[0]   v[4]   v[8]  v[12]                                  */
/*        v[1]   v[5]   v[9]  v[13]                                  */
/*        v[2]   v[6]  v[10]  v[14]                                  */
/*        v[3]   v[7]  v[11]  v[15]                                  */
void vrFprintMatrix(FILE *file, char *before, vrMatrix *mat)
{
	int	row;

	if (before)
		vrFprintf(file, "%s\n", before);

	for (row = 0; row < VRMAT_MAXDIM; row++) {
		vrFprintf(file, "\t%6.3f  %6.3f  %6.3f  %6.3f\n",
			VRMAT_ROWCOL(mat, row, VR_X), VRMAT_ROWCOL(mat, row, VR_Y), VRMAT_ROWCOL(mat, row, VR_Z), VRMAT_ROWCOL(mat, row, VR_W));
	}
}


/*********************************************************************/
void vrPrintMatrix(char *before, vrMatrix *mat)
{
	vrFprintMatrix(stdout, before, mat);
}


/*********************************************************************/
/* print the transpose of the matrix                                 */
/* NOTE: this is often done to print a 4x4 matrix with the           */
/*   translation values in the bottom row (C-style).                 */
void vrFprintMatrixT(FILE *file, char *before, vrMatrix *mat)
{
	int	row;

	if (before)
		vrFprintf(file, "%s\n", before);

	for (row = 0; row < VRMAT_MAXDIM; row++) {
		vrFprintf(file, "\t%6.3f  %6.3f  %6.3f  %6.3f\n",
			VRMAT_ROWCOL(mat, VR_X, row), VRMAT_ROWCOL(mat, VR_Y, row), VRMAT_ROWCOL(mat, VR_Z, row), VRMAT_ROWCOL(mat, VR_W, row));
	}
}


/*********************************************************************/
/* print the transpose of the matrix */
void vrPrintMatrixT(char *before, vrMatrix *mat)
{
	vrFprintMatrixT(stdout, before, mat);
}


/*********************************************************************/
void vrPrintQuat(char *before, vrQuat *quat)
{
	if (before)
		vrPrintf("%s [(%2.3f, %2.3f, %2.3f), %2.3f]\n",
			before, quat->v[VR_X], quat->v[VR_Y], quat->v[VR_Z], quat->v[VR_W]);
	else
		vrPrintf(   "[(%2.3f, %2.3f, %2.3f), %2.3f]\n",
			        quat->v[VR_X], quat->v[VR_Y], quat->v[VR_Z], quat->v[VR_W]);
}


/*********************************************************************/
void vrPrintPoint(char *before, vrPoint *pnt)
{
	if (before)
		vrPrintf("%s (%2.2f, %2.2f, %2.2f)\n", before, pnt->v[VR_X], pnt->v[VR_Y], pnt->v[VR_Z]);
	else	vrPrintf(   "(%2.2f, %2.2f, %2.2f)\n",         pnt->v[VR_X], pnt->v[VR_Y], pnt->v[VR_Z]);
}


/*********************************************************************/
void vrPrintVector(char *before, vrVector *vec)
{
	if (before)
		vrPrintf("%s (%2.2f, %2.2f, %2.2f)\n", before, vec->v[VR_X], vec->v[VR_Y], vec->v[VR_Z]);
	else	vrPrintf(   "(%2.2f, %2.2f, %2.2f)\n",         vec->v[VR_X], vec->v[VR_Y], vec->v[VR_Z]);
}


/*********************************************************************/
void vrPrintVector4(char *before, vrVector4 *vec)
{
	if (before)
		vrPrintf("%s (%2.2f, %2.2f, %2.2f, %2.2f)\n", before, vec->v[VR_X], vec->v[VR_Y], vec->v[VR_Z], vec->v[VR_W]);
	else	vrPrintf(   "(%2.2f, %2.2f, %2.2f, %2.2f)\n",         vec->v[VR_X], vec->v[VR_Y], vec->v[VR_Z], vec->v[VR_W]);
}


/*********************************************************************/
/* NOTE: It's important to remember that AZIM is a rotation about the up-axis, */
/*    and ELEV is a rotation about what is normally the X-axis.                */
void vrPrintEuler(char *before, vrEuler *euler)
{
	if (before)
		vrPrintf("%s (%2.2f, %2.2f, %2.2f), (%2.2f, %2.2f, %2.2f)\n", before,
			euler->t[VR_X], euler->t[VR_Y], euler->t[VR_Z],
			euler->r[VR_AZIM], euler->r[VR_ELEV], euler->r[VR_ROLL]);
	else	vrPrintf(   "(%2.2f, %2.2f, %2.2f), (%2.2f, %2.2f, %2.2f)\n",
			euler->t[VR_X], euler->t[VR_Y], euler->t[VR_Z],
			euler->r[VR_AZIM], euler->r[VR_ELEV], euler->r[VR_ROLL]);
}


/*
 * ===================================================
 *
 * DEFINITIONS FOR MATRIX INVERSION
 * I'm doing matrix inversion with Gauss-Jordan reduction,
 * so I need to define some auxiliary data structures
 * ===================================================
 */
#define VRAUG_ROWCOL(a, row, col)	((a)[(8*(row) + (col))])
typedef double *vrAugMat;	/* TODO: make this a structure like vrMatrix */


/*********************************************************************/
vrAugMat vrAugMatCreate(vrMatrix *m)
{
	int		row;
	int		col;
	vrAugMat	a = vrShmemAlloc0(32*sizeof(double));

	for (row = 0; row < 4; row++) {
		for (col = 0; col < 4; col++) {
			VRAUG_ROWCOL(a, row, col) = VRMAT_ROWCOL(m, row, col);
		}
		VRAUG_ROWCOL(a, row, 4+row) = 1.0f;
	}
	return a;
}


/*********************************************************************/
void vrAugMatDelete(vrAugMat mat) { vrShmemFree(mat); }


/*********************************************************************/
void vrAugMatPrint(char *before, vrAugMat mat)
{
	int	row;
	int	col;

	if (before) vrPrintf("%s\n", before);
	for (row = 0; row < 4; row++) {
		vrPrintf("\t");
		for (col=0; col<8; col++) vrDbgPrintf("%2.2f  ", VRAUG_ROWCOL(mat,row,col));
		vrPrintf("\n");
	}
}


/*********************************************************************/
/* elementary row operations on an augmented matrix                  */
void swap_rows(vrAugMat mat, int row1, int row2)
{
	int	col;
	double	tmp;

	for (col = 0; col < 8; col++) {
		tmp = VRAUG_ROWCOL(mat, row1, col);
		VRAUG_ROWCOL(mat, row1, col) = VRAUG_ROWCOL(mat, row2, col);
		VRAUG_ROWCOL(mat, row2, col) = tmp;
	}
}


/*********************************************************************/
void scale_row(vrAugMat mat, int row, double s)
{
	int	col;

	for (col = 0; col < 8; col++)
		VRAUG_ROWCOL(mat, row, col) *= s;
}


/*********************************************************************/
void combine_rows(vrAugMat mat, int row1, int row2, double s)
{
	int	col;

	for (col = 0; col < 8; col++)
		VRAUG_ROWCOL(mat, row1, col) += VRAUG_ROWCOL(mat, row2, col) * s;
}


/*********************************************************************/
/*  helper routines for reduction                                    */
#include <limits.h>
int fewest_zeros(vrAugMat mat, int start)
{
	int	row;
	int	best = INT_MAX;
	int	best_idx = -1;

	for (row = start; row < 4; row++) {
		int zero_cnt = 0;
		int col = 0;
		while (VRAUG_ROWCOL(mat, row, col++) == 0)
			zero_cnt++;
		if (zero_cnt < best) {
			best = zero_cnt;
			best_idx = row;
		}
	}
	return best_idx;
}


/*********************************************************************/
/*  do the reduction                                                 */
void vrAugMatReduce(vrAugMat mat)
{
	int	row;
	double	factor;

	for (row = 0; row < 4; row++) {
		int	row2;
		int	best;

		/* adjust rows if necessary */
		best = fewest_zeros(mat, row);

		if (best > row) {
			swap_rows(mat, row, best);
		}

		/* scale the row if necessary */
		if (VRAUG_ROWCOL(mat, row, row) != 1.0f &&
			VRAUG_ROWCOL(mat, row, row) != 0.0f) {
			factor = 1.0f/VRAUG_ROWCOL(mat, row, row);
			scale_row(mat, row, factor);
		}

		/* remove nonzero entries in the following rows */
		/* we're going to cheat a little here and use   */
		/* the fact that we're doing this on a 4x4 matrix */
		for (row2=0; row2<4; row2++) {
			factor = - VRAUG_ROWCOL(mat, row2, row);
			if (factor != 0.0f && row2 != row) {
				combine_rows(mat, row2, row, factor);
			}
		}
	}
}


/*********************************************************************/
void vrGetInverseFromAug(vrAugMat mat, vrMatrix *inv)
{
	int	row;
	int	col;

	for (row = 0; row < 4; row++)
		for (col = 0; col < 4; col++)
			VRMAT_ROWCOL(inv, row, col) = VRAUG_ROWCOL(mat, row, col+4);
}


/*********************************************************************/
vrMatrix *vrMatrixInvert(vrMatrix *dst, vrMatrix *src)
{
	vrAugMat	aug = vrAugMatCreate(src);

	vrAugMatReduce(aug);
	vrGetInverseFromAug(aug, dst);
	vrAugMatDelete(aug);

	return dst;
}


/*********************************************************************/
vrMatrix *vrMatrixInvertInPlace(vrMatrix *mat)
{
	vrAugMat	aug = vrAugMatCreate(mat);

	vrAugMatReduce(aug);
	vrGetInverseFromAug(aug, mat);
	vrAugMatDelete(aug);

	return (mat);
}


/***********************************************************************/
/* This section create a program that tests some of the math routines, */
/*   printing the result of various operations.  Tests are added on an */
/*   as-needed basis (ie. I think there might be a problem with either */
/*   one routine, or a section of routines.                            */
/*                                                                     */
/* The included Makefile will compile this program with the target:    */
/*   % make mathtest                                                   */
/***********************************************************************/
#ifdef TEST_APP
int main(int argc, char **argv)
{
	vrPrintf("NOTE: cos(30) = %2.3f, sin(30) = %2.3f\n", cos(vrDegreesToRadians(30.0)), sin(vrDegreesToRadians(30.0)));
	vrPrintf("NOTE: cos(45) = %2.3f, sin(45) = %2.3f\n", cos(vrDegreesToRadians(45.0)), sin(vrDegreesToRadians(45.0)));
	vrPrintf("NOTE: cos(90) = %2.3f, sin(90) = %2.3f\n", cos(vrDegreesToRadians(90.0)), sin(vrDegreesToRadians(90.0)));
#if 0
	vrPrintf(BOLD_TEXT "\nSome basic matrix operation tests:\n\n" NORM_TEXT);

	vrMatrix	*m = vrMatrixSetRotationId(vrMatrixCreate(), VR_X, 30.0f);
	vrMatrix	*i = vrMatrixInvert(vrMatrixCreate(), m);
	vrMatrix	*p = vrMatrixProduct(vrMatrixCreate(), i, m);

	vrPrintMatrix("30d rot about X = ", m);
	vrPrintMatrix("inverse of above = ", i);
	vrPrintMatrix("product of these two (s/b id) = ", p);


  {
	/* tests of the Euclidean inversion stuff with travel matrices from Kalev */
	vrMatrix	cl;
	vrMatrix	clinv;
	double		clvals[16] = { -0.964685, 0.000000, -0.165759, 0.000000,
					0.000000, 1.000000, 0.000000, 0.000000,
					0.165759, 0.000000, -0.964685, 0.000000,
					5.373073, -23.577421, -20.114393, 1.000000 };

	vrPrintf(BOLD_TEXT "\ntests of the Euclidean inversion stuff:\n\n" NORM_TEXT);
	vrMatrixSetAd(&cl, clvals);
	vrMatrixInvertEuclidean(&clinv, &cl);
	vrPrintMatrix("Supposed Euclidean matrix", &cl);
	vrPrintMatrix("Inverted Euclidean matrix", &clinv);

	if (!vrMatrixIsEuclidean(&cl)) {
		vrPrintf("Actually it wasn't Euclidean\n");
		vrMatrixMakeEuclidean(&cl, &cl);
		vrMatrixInvertEuclidean(&clinv, &cl);
		vrPrintMatrix("Converted Euclidean matrix", &cl);
		vrPrintMatrix("Inverted Euclidean matrix", &clinv);
	}

  }
  {
	/* further tests of the Euclidean inversion stuff with travel matrices from Kalev */
	vrMatrix	tmpmat;
	vrMatrix	cl;
	vrMatrix	cleuc;
	double		clvals[16] = { 0.040264,   0.000000,  -0.978011, 0.000000,
					 0.000000,   1.000000,   0.000000, 0.000000,
					 0.978011,   0.000000,   0.040264, 0.000000,
					20.632961, -14.370701, -36.058109, 1.000000 };
	double		scale;

	vrPrintf(BOLD_TEXT "\nFurther tests of the Euclidean inversion stuff:\n\n" NORM_TEXT);
	vrMatrixSetAd(&cl, clvals);
	vrPrintMatrix("initial matrix", &cl);

	vrMatrixMakeEuclidean(&cleuc, &cl);
	vrPrintMatrix("first conversion", &cleuc);

	vrMatrixMakeEuclidean(&cl, &cleuc);
	vrPrintMatrix("second conversion", &cl);

	vrMatrixMakeEuclidean(&cleuc, &cl);
	vrPrintMatrix("third conversion", &cleuc);

	scale = vrMatrixGetScale(&cleuc);
	vrPrintf("The scale of the latest matrix is %f\n", scale);
	vrMatrixSetScale3d(&tmpmat, 1.0f/scale, 1.0f/scale, 1.0f/scale);
	vrMatrixPostMult(&cleuc, &tmpmat);
	vrPrintMatrix("rescaled matrix is", &cleuc);

	vrMatrixSetScale(&tmpmat, &cleuc, 1.0);
	vrPrintMatrix("rescaled via vrMatrixSetScale() matrix is", &cleuc);

	scale = vrMatrixGetScale(&cleuc);
	vrPrintf("The scale of the rescaled matrix is %f\n", scale);

  }
  {
	/* tests of the Quaternion functions */
	vrMatrix	*mid = vrMatrixCreateIdentity();
	vrMatrix	*m90x = vrMatrixSetRotationId(vrMatrixCreate(), VR_X, 90.0f);
	vrMatrix	*m30x = vrMatrixSetRotationId(vrMatrixCreate(), VR_X, 30.0f);
	vrQuat		quat;
	vrQuat		*q30x = vrQuatSet4d(vrQuatCreate(), 30.0, 1.0, 0.0, 0.0);

	vrPrintf(BOLD_TEXT "\n\nSome Quaternion function tests:\n\n" NORM_TEXT);
	vrPrintMatrix("identity matrix is", mid);
	vrQuatSetFromMatrix(&quat, mid);
	vrPrintQuat("vrQuatSetFromMatrix() -- identity", &quat);
	vrMatrixSetRotationFromQuat(mid, &quat);
	vrPrintMatrix("and converted back to matrix again is", mid);

	vrPrintMatrix("\n90 about X matrix is", m90x);
	vrQuatSetFromMatrix(&quat, m90x);
	vrPrintQuat("vrQuatSetFromMatrix() -- m90x", &quat);
	vrMatrixSetRotationFromQuat(m90x, &quat);
	vrPrintMatrix("and converted back to matrix again is", m90x);

	vrPrintMatrix("\n30 about X matrix is", m30x);
	vrQuatSetFromMatrix(&quat, m30x);
	vrPrintQuat("vrQuatSetFromMatrix() -- m30x", &quat);
	vrMatrixSetRotationFromQuat(m30x, &quat);
	vrPrintMatrix("and converted back to matrix again is", m30x);

	vrPrintQuat("\nvrQuatSet4d() -- q30x", q30x);
	vrMatrixSetRotationFromQuat(m30x, q30x);
	vrPrintMatrix("and converted to matrix is", m30x);
	vrPrintMatrix("with the transpose of:", vrMatrixTranspose(m30x, m30x));

  }
#elif 1
  {
	vrPrintf(BOLD_TEXT "\n\nMore Quaternion function tests (Z-up vs. Y-up):\n\n" NORM_TEXT);
	/* tests of the Quaternion Y-UP and Z-UP conversion routines */
	vrPoint		*point1 = vrPointCreate3d(10.0, 20.0, 30.0);
	vrQuat		*p90x = vrQuatSet4d(vrQuatCreate(),  90.0, 1.0, 0.0, 0.0);
	vrQuat		*n90x = vrQuatSet4d(vrQuatCreate(), -90.0, 1.0, 0.0, 0.0);
	vrQuat		quat;
	vrMatrix	mat;

	vrQuatSet4d(&quat, 120.0, 0.0, 0.0, 1.0);
	vrPrintf("------------\n");
	vrPrintQuat("Original quaternion", &quat);
	vrPrintQuat("Converted from Z-up to Y-up:", vrQuatConvertZUPtoYUP(&quat, &quat));
	vrPrintQuat("Converted back to Z-up from Y-up:", vrQuatConvertYUPtoZUP(&quat, &quat));

	/* now do the same with multiplies */
	vrQuatSet4d(&quat, 45, 0.0, 1.0, 0.0);
	vrQuatSet4d(&quat, 120.0, 0.0, 0.0, 1.0);
	vrQuatSet4d(&quat, -30.0, 1.0, 0.0, 0.0);
	vrPrintf("------------\n");
	vrPrintQuat("Original quaternion", &quat);
	vrQuatProduct(&quat, n90x, &quat);
	vrPrintQuat("Converted from Z-up to Y-up:", &quat);
	vrQuatProduct(&quat, p90x, &quat);
	vrPrintQuat("Converted back to Z-up from Y-up:", &quat);
	vrPrintf("------------\n");
	vrQuatSet4d(&quat, 90.0, 1.0, 0.0, 0.0);
	vrPrintQuat("+90 x-rot as Quat:", &quat);
	//vrMatrixSetIdentity(&mat);
	//vrMatrixSetRotation
	vrMatrixSetTranslation3d(&mat, 30.0, 20.0, 10.0);
	vrMatrixSetRotationFromQuat(&mat, &quat);
	vrPrintMatrix("+90 x-rot as Matrix", &mat);
	vrEuler	euler;
	vrEulerSetFromMatrix(&euler, &mat);
	vrPrintEuler("+90 x-rot as Euler (a,e,r)", &euler);

	/* now the same with matrix multiplies */
	vrQuatSet4d(&quat, 90.0, 0.0, 1.0, 0.0);
	vrPrintf("------------\n");
	vrPrintQuat("Original quaternion", &quat);
	vrPrintPoint("test point is: ", point1);
	vrPointTransformByQuat(point1, point1, &quat);
	vrPrintPoint("test point by quat: ", point1);

/* Other quaternion stuff to test:

vrQuat		*vrQuatProduct(vrQuat *prod, vrQuat *ql, vrQuat *qr);
vrQuat		*vrQuatProductEquals(vrQuat *prod, vrQuat *ql, vrQuat *qr);
DONE: vrMatrix	*vrMatrixSetFromQuat(vrMatrix *mat, vrQuat *quat);
vrPoint		*vrPointTransformByQuat(vrPoint *dst_pnt, vrPoint *src_pnt, vrQuat *quat);
	[and when I add it: vrQuatSetFromVector()]
	[and when I add it: vrVectorTransformByQuat() -- verify that this would be different from vrPointTransformByquat -- or should this be vrVectorRotateByQuat()?]
*/
  }
#else
	vrMatrix	*m = vrMatrixSetRotationId(vrMatrixCreate(), VR_X, -10.0f);
	vrPrintMatrix("-10d rot about X = ", m);
#endif
}
#endif



#if 0 /* { */
/*******************************************************************/
/*******************************************************************/
/*******************************************************************/
/*******************************************************************/
/* Stuff from cave.boom.c (written by Bill Sherman) ****************/
/*   This old code has some routines used to convert a 4x4 matrix  */
/*   into 6 euler values.  At the moment we don't need this code,  */
/*   but just in case it's needed in the future, I figure the math */
/*   source file is the best place to store it.  I (Bill) wrote    */
/*   this to integrate a Fakespace BOOM-2 into the CAVE library -- */
/*   which uses Eulers for dealing with all 6-sensors.             */


/******************************/
/*** CAVEGetBoomTracking(): ***/
/******************************/
int CAVEGetBoomTracking(CAVE_ST *cave,CAVE_SENSOR_ST *sensor)
{
static	CAVE_SENSOR_ST	head = { 0.0, 0.0, 0.0,   0.0, 0.0, 0.0 };
	float		mat[4][4];
	float		*lmat;
	float		azi, elev, roll;
	float		azi_s, azi_c;

	if (boom_fd < 0) {
		fprintf(stderr,"CAVE tracking: no boom communications port.\n");
		return 0;
	}

	lmat = BOOM_read(boom_fd);
	CAVEGetTimestamp(&head.timestamp);
	memcpy(mat, lmat, 16*sizeof(float));

	head.x = mat[3][0]*3.0;
	head.y = mat[3][1]*3.0 + 5.0;
	head.z = mat[3][2]*3.0;

	/*** This section of code calculates the proper Euler angles from ***/
	/***   the 4x4 matrix using techniques from Robotics, Fu et.al.   ***/
	/***   Actually, the matrix order seems backward, and I lucked    ***/
	/***   into getting them backward - figure it out later, it works!***/
	azi  = atan2(mat[2][0], mat[2][2]);
	azi_s = sin(azi);
	azi_c = cos(azi);
	elev = atan2(mat[2][1], azi_s*mat[2][0] + azi_c*mat[2][2]);
	roll = atan2(azi_s*mat[1][2] - azi_c*mat[1][0], azi_c*mat[0][0] - azi_s*mat[0][2]);
	head.azim =  RTOD(azi);
	head.elev = -RTOD(elev);
	head.roll =  RTOD(roll);

	/*** put information into return structs ***/
	sensor[0] = head;
	sensor[1] = head;

	return 1;
}


/*****************************/
/*** BOOM_angles2matrix(): ***/
/*****************************/
static void BOOM_angles2matrix(float angles[6],float matrix[4][4])
{
	int	i, j;	/* loop counters              */

	/*** initialize matrix to 4x4 identity matrix ***/
	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			matrix[i][j] = (i == j) ? 1 : 0;

	matrix_rotate(matrix, angles[0] * (-1.0), 'y');
	matrix_rotate(matrix, angles[1],          'x');
	matrix_translate(matrix, 0.0, 1.0, 0.0);
	matrix_rotate(matrix, angles[2] - M_PI_2, 'x');
	matrix_translate(matrix, 0.0, 0.0, 1.0);
	matrix_rotate(matrix, angles[3] - M_PI_2, 'x');
	matrix_translate(matrix, 0.0, -0.5, 0.0);
	matrix_rotate(matrix, angles[4],          'y');
	matrix_rotate(matrix, angles[5],          'x');

	matrix[3][1] -= 0.5;	/* display is at y=0	      */
}


/*****************************************************************/
/*** BOOM_matrix2euler(): convert a 4x4 matrix to euler angles ***/
/***   plus a cartesian 3-tuple.                               ***/
/***                                                           ***/
/*** NOTE: this code isn't necessarily correct, I don't use it ***/
/***   It has the essence of solving for the Euler angles, but ***/
/***   to see how it is really done, refer to "Robotics" by Fu ***/
/***   Gonzales & Lee.                                         ***/
/*****************************************************************/
static void BOOM_matrix2euler(float matrix[4][4], float eulers[6])
{
static	float	epsilon = (1e-10);
	float	sinPitch, cosPitch;
	float	sinRoll, cosRoll;
	float	sinYaw, cosYaw;

	/* cartesian postition is just copied from matrix */
	eulers[3] = matrix[3][0];
	eulers[4] = matrix[3][1];
	eulers[5] = matrix[3][2];

	/* determine euler angles from matrix - a little harder */
	sinPitch = -matrix[2][0];
	cosPitch = sqrt(1 - sinPitch*sinPitch);
	if (matrix[0][2] < 0) cosPitch = -cosPitch;

	if (fabs(cosPitch) > epsilon) {
		sinRoll = matrix[2][1] / cosPitch;
		cosRoll = matrix[2][2] / cosPitch;
		sinYaw = matrix[1][0] / cosPitch;
		cosYaw = matrix[0][0] / cosPitch;
	} else {
		sinRoll = -matrix[1][2];
		cosRoll = matrix[1][1];
		sinYaw = 0;
		cosYaw = 1;
	}

	eulers[0] = atan2(sinYaw, cosYaw);
	eulers[1] = atan2(sinPitch, cosPitch);
	eulers[2] = atan2(sinRoll, cosRoll);
}


/************************/
/*** matrix_rotate(): ***/
/************************/
static void matrix_rotate(float mat[4][4],float angle,char axis)
{
	float	sin_angle, cos_angle, temp;
	int	i, j, k;

	switch (axis) {
	case 'x':
		i = 1;
		j = 2;
		break;
	case 'y':
		i = 2;
		j = 0;
		break;
	case 'z':
		i = 0;
		j = 1;
		break;
	}

	sin_angle = sinf(angle);
	cos_angle = cosf(angle);

	for (k = 0; k < 4; k++) {
		temp = cos_angle * mat[j][k] - sin_angle * mat[i][k];
		mat[i][k] = sin_angle * mat[j][k] + cos_angle * mat[i][k];
		mat[j][k] = temp;
	}
}


/***************************/
/*** matrix_translate(): ***/
/***************************/
static void matrix_translate(float mat[4][4], float x, float y, float z)
{
	int	count;

	for (count = 0; count < 3; count++)
		mat[3][count] += mat[0][count] * x + mat[1][count] * y + mat[2][count] * z;
}

#endif /* } */

