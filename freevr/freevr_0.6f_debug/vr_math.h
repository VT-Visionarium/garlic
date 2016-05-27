/* ======================================================================
 *
 * HH   HH         vr_math.h
 * HH   HH         Author(s): Ed Peters, Stuart Levy, Bill Sherman, John Stone
 * HHHHHHH         Created: June 4, 1998
 * HH   HH         Last Modified: July 15, 2013
 * HH   HH
 *
 * Header file for FreeVR math routines.
 *
 * Copyright 2013, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/* NOTE: FreeVR operates in the OpenGL Y-UP coordinate system. */
#ifndef __VRMATH_H__
#define __VRMATH_H__

#include <stdio.h>		/* for FILE type definition */

#ifdef __cplusplus
extern "C" {
#endif


/* translational / homogeneous matrix elements (and standard quaternions) */
#define VR_X 0
#define VR_Y 1
#define VR_Z 2
#define VR_W 3

/* Euler ego-centric rotational elements */
#define	VR_AZIM 0	/* rotation about Y (upward) axis     */
#define	VR_ELEV 1	/* rotation about X (horizontal) axis */
#define	VR_ROLL 2	/* rotation about Z (inward) axis     */

/* quaternion elements -- when following translational elements in a sequence of 7 values */
#define VR_QX (VR_X + 3)
#define VR_QY (VR_Y + 3)
#define VR_QZ (VR_Z + 3)
#define VR_QW (VR_W + 3)

#if !defined(PI)
#define PI	3.14159265358979323846
#endif

#define DTOR(x)	((x) * PI / 180.0)
#define RTOD(x)	((x) * 180.0 / PI)

#define VR_EPSILON 0.00000000001


/****************************************/
typedef enum {
		VRDIR_UNKNOWN = -1,
		VRDIR_FORE,
		VRDIR_BACK,
		VRDIR_UP,
		VRDIR_DOWN,
		VRDIR_LEFT,
		VRDIR_RIGHT
	} vrDirection;


/***************************************************************************/
/* vrMatrix: a 4x4 matrix stored as a simple 16 element array of doubles.  */
/*   The VRMAT_ROWCOL (primarily) and VRMAT_COLROW macros can be used to   */
/*   access specific elements based on the column and row coordinates.     */
/* NOTE: The layout of the array in matrix form is:                        */
/*                                                                         */
/*   v[0]   v[4]   v[8]  v[12]         rc(x,x)  rc(x,y)  rc(x,z)  rc(x,w)  */
/*   v[1]   v[5]   v[9]  v[13]    =    rc(y,x)  rc(y,y)  rc(y,z)  rc(y,w)  */
/*   v[2]   v[6]  v[10]  v[14]         rc(z,x)  rc(z,y)  rc(z,z)  rc(z,w)  */
/*   v[3]   v[7]  v[11]  v[15]         rc(w,x)  rc(w,y)  rc(w,z)  rc(w,w)  */
/*                                                                         */
/* which is determined by the VRMAT_ROWCOL and VRMAT_COLROW macros.        */
typedef struct {
		double	v[16];
	} vrMatrix;
#define VRMAT_MAXDIM 4
#define VRMAT_ROWCOL(m,r,c) ((m)->v[(r)+VRMAT_MAXDIM*(c)])
#define VRMAT_COLROW(m,c,r) ((m)->v[(r)+VRMAT_MAXDIM*(c)])


/*********************************************************/
/* This structure is used for breaking down a 4x4 matrix */
/*   into its component operations                       */
typedef struct {
		double	scale[3];
		double	shear[3];	/* XY, XZ, YZ */
		double	rotate[3];
		double	translate[3];
		double	perspective[4];	/* X, Y, Z, W */
	} vrMatrixComposition;


/****************************************/
/* data for storing a rendering frustum */
typedef struct {
		double	left;
		double	right;
		double	bottom;
		double	top;
		double	near_clip;	/* NOTE: I prefer "near" but Cygwin compiler can't handle it */
		double	far_clip;	/* NOTE: I prefer "far" but Cygwin compiler can't handle it */
	} vrFrustum;


/*************************************************************/
/* vrPoint & vrPointf : ...   */
typedef struct {
		double	v[3];
	} vrPoint;
typedef struct {
		float	v[3];
	} vrPointf;

#define vrPointSet3d(pnt,x,y,z) \
	if ((pnt)!=NULL) { (pnt)->v[VR_X]=(x); (pnt)->v[VR_Y]=(y); (pnt)->v[VR_Z]=(z); }
#define vrPointSetAd(pnt,a) \
	if ((pnt)!=NULL) { (pnt)->v[VR_X]=(a)[VR_X]; (pnt)->v[VR_Y]=(a)[VR_Y]; (pnt)->v[VR_Z]=(a)[VR_Z]; }


/*************************************************************/
/* vrVector, vrVectorf & vrVector4: ...   */
typedef struct {
		double	v[3];
	} vrVector;
typedef struct {
		float	v[3];
	} vrVectorf;

#define vrVectorSet3d(vec,x,y,z) \
	if ((vec)!=NULL) { (vec)->v[VR_X]=(x); (vec)->v[VR_Y]=(y); (vec)->v[VR_Z]=(z); }
#define vrVectorSetAd(vec,a) \
	if ((vec)!=NULL) { (vec)->v[VR_X]=(a)[VR_X]; (vec)->v[VR_Y]=(a)[VR_Y]; (vec)->v[VR_Z]=(a)[VR_Z]; }


typedef struct {
		double	v[4];
	} vrVector4;

#define vrVector4Set4d(vec,x,y,z,w) \
	if ((vec)!=NULL) { (vec)->v[VR_X]=(x); (vec)->v[VR_Y]=(y); (vec)->v[VR_Z]=(z); (vec)->v[VR_W]=(w); }
#define vrVector4SetAd(vec,a) \
	if ((vec)!=NULL) { (vec)->v[VR_X]=(a)[VR_X]; (vec)->v[VR_Y]=(a)[VR_Y]; (vec)->v[VR_Z]=(a)[VR_Z]; (vec)->v[VR_W]=(a)[VR_W]; }


/*************************************************************/
/* vrQuat: ...   */
typedef struct {
		double v[4];
	} vrQuat;

/* vrQuatLengthSq() -- return the length-squared of the quaternion.  Useful for normalizing a quaternion */
#define vrQuatLengthSq(q) \
	( ((q)==NULL) ? -1 : \
	(((q)->v[VR_X]*(q)->v[VR_X])+((q)->v[VR_Y]*(q)->v[VR_Y])+((q)->v[VR_Z]*(q)->v[VR_Z])+((q)->v[VR_W]*(q)->v[VR_W])))


/*********************************************************************/
/* vrTransform: a structure that uses UNC's VQS method of holding    */
/*   transform information.  A VQS transform combines the operations */
/*   of translation, rotation and scaling.                           */
/* NOTE: at the moment FreeVR does not use this structure, but if    */
/*   feasible, it might be beneficial.                               */
/*********************************************************************/
typedef struct {
		vrVector	v;
		vrQuat		q;
		double		s;
	} vrTransform;


/**************************************************************/
/* vrEuler:  a type containing 6 Euler values that give       */
/*   a particular positional value.  In FreeVR, their use     */
/*   is intentionally limited to only be used when necessary, */
/*   which is generally when some input device reports its    */
/*   values in Euler angles.                                  */
/*                                                            */
/* NOTE: vrEuler uses floats in order to match how information*/
/*   is stored in shared memory by the CAVE tracker daemon.   */
/**************************************************************/
typedef struct {
		float	t[3];	/* translational component */
		float	r[3];	/* rotational component    */
	} vrEuler;



/*
 *
 * TIME
 * These are the time-related definitions.  Most user code should be
 * able to get by with the frame_time field in the process info
 * structure, but if you _really_ want to make timing calls here's
 * how to do it.
 * vrStartTime will be the wall-clock time when the simulation
 * started.  vrCurrentTime will be the current wall-clock time.
 * vrTimeSince will be the time elapsed since the specified time.
 * (All of these will be in seconds.)
 *
 */
typedef	double	vrTime;

vrTime		vrStartTime();
vrTime		vrCurrentWallTime();
vrTime		vrSimTimeOf(vrTime time);
vrTime		vrCurrentSimTime();
vrTime		vrTimeSince(vrTime then);



/*
 *
 * MATRICES
 * Defined are a series of operations to work on 4x4 matrices,
 * laid out as a 16-element float vector (defined as column-major
 * for compatibility * with GL).  The API includes functions to
 * create and destroy matrices, initialize their values and compose
 * them with multiplication.
 *
 */
vrMatrix	*vrMatrixCreate(void);
void		 vrMatrixDelete(vrMatrix *mat);
int		 vrMatrixEqual(vrMatrix *mat_l, vrMatrix *mat_r);
vrMatrix	*vrMatrixCopy(vrMatrix *mat_l, vrMatrix *mat_r);
vrMatrix	*vrMatrixSetIdentity(vrMatrix *mat);
vrMatrix	*vrMatrixCreateIdentity(void);
vrMatrix	*vrMatrixSetAd(vrMatrix *mat, double *array);
vrMatrix	*vrMatrixSetXYZAERAf(vrMatrix *mat, float *array);
vrMatrix	*vrMatrixSetXYZAERAd(vrMatrix *mat, double *array);
vrMatrix	*vrMatrixSetXYZAERE(vrMatrix *mat, vrEuler *euler);
vrMatrix	*vrMatrixSetEulerAzimaxis(vrMatrix *mat, vrEuler *euler, int axis);
vrEuler		*vrEulerSetFromMatrix(vrEuler *euler, vrMatrix *mat);
vrMatrix	*vrMatrixSetFromCoords(vrMatrix *mat, double *coords);
vrMatrix	*vrMatrixSetTranslation3d(vrMatrix *mat, double x, double y, double z);
vrMatrix	*vrMatrixSetTranslationAd(vrMatrix *mat, double *array);
vrMatrix	*vrMatrixSetTranslationOnlyAd(vrMatrix *mat, double *array);
vrMatrix	*vrMatrixGetResetTranslationAd(vrMatrix *mat, double *array);
int		 vrMatrixCompositionFromMatrix(vrMatrixComposition *comp, vrMatrix *mat);
vrMatrix	*vrMatrixSetRotationId(vrMatrix *mat, int axis, double theta);
vrMatrix	*vrMatrixSetRotationIdNew(vrMatrix *mat, int axis, double theta);
vrMatrix	*vrMatrixSetRotation4d(vrMatrix *mat, double x, double y, double z, double theta);
vrMatrix	*vrMatrixSetRotationAd(vrMatrix *mat, double *array);
vrMatrix	*vrMatrixSetScale3d(vrMatrix *mat, double sx, double sy, double sz);

vrMatrix	*vrMatrixProduct(vrMatrix *product, vrMatrix *mat_l, vrMatrix *mat_r);

vrMatrix	*vrMatrixPreMult(vrMatrix *mat_l, vrMatrix *mat_r);
vrMatrix	*vrMatrixPreTranslate3d(vrMatrix *mat_l, double x, double y, double z);
vrMatrix	*vrMatrixPreTranslateAd(vrMatrix *mat_l, double *array);
vrMatrix	*vrMatrixPreRotateId(vrMatrix *mat_l, int axis, double theta);
vrMatrix	*vrMatrixPreRotateAboutOriginId(vrMatrix *mat_l, int axis, double theta);
vrMatrix	*vrMatrixPreRotateAboutPointId(vrMatrix *mat_l, vrPoint *point, int axis, double theta);
vrMatrix	*vrMatrixPreRotateIdNew(vrMatrix *mat_l, int axis, double theta);
vrMatrix	*vrMatrixPreRotate4d(vrMatrix *mat_l, double x, double y, double z, double theta);
vrMatrix	*vrMatrixPreScale3d(vrMatrix *mat_l, double sx, double sy, double sz);

vrMatrix	*vrMatrixPostMult(vrMatrix *mat_l, vrMatrix *mat_r);
vrMatrix	*vrMatrixPostTranslate3d(vrMatrix *mat_l, double x, double y, double z);
vrMatrix	*vrMatrixPostTranslateAd(vrMatrix *mat_l, double *array);
vrMatrix	*vrMatrixPostRotateId(vrMatrix *mat_l, int axis, double theta);
vrMatrix	*vrMatrixPostRotateAboutOriginId(vrMatrix *mat_l, int axis, double theta);
vrMatrix	*vrMatrixPostRotateAboutPointId(vrMatrix *mat_l, vrPoint *point, int axis, double theta);
vrMatrix	*vrMatrixPostRotateIdNew(vrMatrix *mat_l, int axis, double theta);
vrMatrix	*vrMatrixPostRotate4d(vrMatrix *mat_l, double x, double y, double z, double theta);
vrMatrix	*vrMatrixPostScale3d(vrMatrix *mat_l, double sx, double sy, double sz);

vrMatrix	*vrMatrixTranspose(vrMatrix *dst, vrMatrix *src);
vrMatrix	*vrMatrixInvert(vrMatrix *dst, vrMatrix *src);
vrMatrix	*vrMatrixInvertInPlace(vrMatrix *mat);
int		 vrMatrixIsEuclidean(vrMatrix *mat);
vrMatrix	*vrMatrixMakeEuclidean(vrMatrix *dst, vrMatrix *src);
vrMatrix	*vrMatrixInvertEuclidean(vrMatrix *dst, vrMatrix *src);
double		 vrMatrixGetScale(vrMatrix *mat);
vrMatrix	*vrMatrixSetScale(vrMatrix *dst, vrMatrix *src, double scale);
double		 vrMatrixDeterminant(vrMatrix *mat);
double		 vr3x3Determinant(double a1, double a2, double a3, double b1, double b2, double b3, double c1, double c2, double c3);
double		 vr2x2Determinant(double a, double b, double c, double d);



/*
 *
 * QUATERNIONS
 * Defined are routines for creating and manipulating quaternions,
 * laid out as an array of 4 doubles [X,Y,Z,W].  Quaternions are an
 * especially succinct way of representing 3d rotations, with many
 * advantages over traditional matrices and Euler angles.  A full
 * description of quaternions is beyond the scope of this comment.
 *
 */
vrQuat		*vrQuatCreate(void);
vrQuat		*vrQuatSet4d(vrQuat *quat, double theta, double x, double y, double z);
vrQuat		*vrQuatProduct(vrQuat *prod, vrQuat *ql, vrQuat *qr);
vrQuat		*vrQuatProductEquals(vrQuat *prod, vrQuat *ql, vrQuat *qr);
vrQuat		*vrQuatCopy(vrQuat *ql, vrQuat *qr);
void		 vrQuatDelete(vrQuat *q);


/* Vector operations */

vrVector	*vrVectorCopy(vrVector *vec1, vrVector *vec2);
vrVector	*vrVectorFromTwoPoints(vrVector *dst, vrPoint *pnt1, vrPoint *pnt2);
vrVector	*vrVectorNormalize(vrVector *dst, vrVector *src);
double		 vrVectorLength(vrVector *vec);
double		 vrVectorDotProduct(vrVector *vec1, vrVector *vec2);
vrVector	*vrVectorCrossProduct(vrVector *dst, vrVector *vec1, vrVector *vec2);
vrVector	*vrVectorScale(vrVector *vec, double scale);
vrVector	*vrVectorSubtract(vrVector *result_vec, vrVector *vec1, vrVector *vec2);
vrVector	*vrVectorTransformByMatrix(vrVector *dst_vec, vrVector *src_vec, vrMatrix *src_mat);
vrVector4	*vrVector4TransformByMatrix(vrVector4 *dst_vec, vrVector4 *src_vec, vrMatrix *src_mat);
vrVector	*vrVectorAddScaledVectors(vrVector *result_vec, vrVector *vec1, vrVector *vec2, double scale1, double scale2);


/*
 *
 * POINTS
 * These are really defined for the sake of completeness;  it's
 * probably more of a pain in the ass to use them than not.
 *
 */
vrPoint 	*vrPointCreate(void);
vrPoint		*vrPointCreate3d(double x, double y, double z);
vrPoint		*vrPointCopy(vrPoint *p1, vrPoint *p2);
void		 vrPointDelete(vrPoint *pnt);
vrPoint		*vrPointAddScaledVector(vrPoint *result_pnt, vrPoint *pnt, vrVector *vec, double scale);



/*
 *
 * TRANSFORMATIONS AND CONVERSIONS
 * Converting quats to mats, vice versa, and applying these
 * transformations to points.
 *
 */
vrMatrix	*vrMatrixSetRotationFromQuatRC(vrMatrix *mat, vrQuat *quat);
#if 1 /* it's time to get rid of this mistake (changed my mind back) */
vrMatrix	*vrMatrixSetRotationFromQuat(vrMatrix *mat, vrQuat *quat);
#endif
vrMatrix	*vrMatrixSetFromQuatRC(vrMatrix *mat, vrQuat *quat);
vrQuat		*vrQuatSetFromMatrix(vrQuat *quat, vrMatrix *mat);

vrPoint		*vrPointTransformByQuat(vrPoint *dst_pnt, vrPoint *src_pnt, vrQuat *quat);
vrPoint		*vrPointTransformByMatrix(vrPoint *dst_pnt, vrPoint *src_pnt, vrMatrix *mat);
vrPoint		*vrPointGetTransFromMatrix(vrPoint *dst_pnt, vrMatrix *src_mat);
vrMatrix	*vrMatrixSetTransFromVector(vrMatrix *mat, vrVector *vec);
vrVector	*vrVectorGetTransFromMatrix(vrVector *dst_vec, vrMatrix *src_mat);





/*
 *
 * DEBUGGING AND UTILITY ROUTINES
 *
 */
void		vrFprintMatrix(FILE *file, char *before, vrMatrix *mat);
void 		vrPrintMatrix(char *before, vrMatrix *mat);
void		vrFprintMatrixT(FILE *file, char *before, vrMatrix *mat);
void 		vrPrintMatrixT(char *before, vrMatrix *mat);
void 		vrPrintQuat(char *before, vrQuat *quat);
void 		vrPrintPoint(char *before, vrPoint *pnt);
void		vrPrintVector(char *before, vrVector *vec);
void		vrPrintVector4(char *before, vrVector4 *vec);
void		vrPrintEuler(char *before, vrEuler *euler);

double		vrDegreesToRadians(double deg);
double		vrRadiansToDegrees(double rad);

#ifndef vrAtoI /* don't declare vrAtoI if it has been defined as an alias for atoi() */
long		vrAtoI(char *str);
#endif
void		*vrAtoP(char *str);
char		*vrBinaryToString(unsigned int val, int len, char *str);

#ifdef __cplusplus
}
#endif

#endif /* __VRMATH_H__ */
