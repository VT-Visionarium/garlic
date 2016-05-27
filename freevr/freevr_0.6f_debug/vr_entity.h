/* ======================================================================
 *
 * HH   HH         vr_entity.h
 * HH   HH         Author(s): Bill Sherman
 * HHHHHHH         Created: December 21, 1998
 * HH   HH         Last Modified: June 25, 2012
 * HH   HH
 *
 * Header file for FreeVR entity structures (ie. Eyes -- this would
 * normally also include Users & Props, but I tried to make a vr_entity.h
 * file a couple months ago and move that stuff from vr_input.h, but I
 * recall that didn't go very well and I had to move everything back).
 * So, right now Eyes and EyeLists are declared.
 *
 * Copyright 2013, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#ifndef __VRENTITY_H__
#define __VRENTITY_H__

#include "vr_math.h"
#include "vr_context.h"		/* which includes vr_shmem.h and vr_enums.h */
#include "vr_system.h"		/* Needed for vrSystemSettings typedef (also includes vr_context.h and vr_shmem.h (already both included)) */
#include "vr_callback.h"

#ifdef __cplusplus
extern "C" {
#endif


/***********************************************************************/
/* The following data is maintained for each user:                     */
/*   - their own inter-ocular distance (used in the display function)  */
/*   - number of eyes to consider (right now, 1 means only left and    */
/*     two means both; could be generalized to N-eye beings later)     */
/*   - an array of SensorAssociation info for their tracked points     */
/*   - a set of pointers to user-specific visual rendering callbacks   */
/*     (see vr_callback.h for a description of these)                  */
/*   - specific position data                                          */
/*                                                                     */
/* Fields marked as "CONFIG" are filled in direction from the          */
/*   config file.  Other fields are determined in combination          */
/*   with other factors.                                               */
/***********************************************************************/
typedef struct vrUserInfo_st {

		/* Generic Object fields */
		vrObject	object_type;	/* The type of this object (ie. VROBJECT_USER) */
		int		id;		/* id number of this user */
		char		*name;		/* CONFIG: name associated with this user */
		int		malleable;	/* CONFIG: Whether the data of this user can be modified */
	struct	vrUserInfo_st	*next;		/* The next user in a linked list         */
		vrContextInfo	*context;	/* pointer to the vrContext memory structure */
		int		line_created;	/* line number of where this object was created in config */
		int		line_lastmod;	/* line number of last time this object modified in config */
		char		file_created[512];/* file (or other) where this object was created*/
		char		file_lastmod[512];/* file (or other) where this object was created*/

		/* Fields specific to Users */
		vrLock		simdata_lock;	/* mutual exclusion lock for the user data */
		vrLock		visren_lock;	/* lock for the visren sync'd copy of data -- this may now be redundant with app_lock */
		vrLock		app_lock;	/* lock for making application travel adjustments atomic */

		/*************************************************************/
		/* specific information necessary for visual/aural rendering */
		vrSystemSettings settings;	/* CONFIG: values for many inheritable fields */
		int		num;		/* the number of this user in running system (1-based) */
		float		iod;		/* CONFIG: The interocular distance of this user */
		int		num_eyes;	/* CONFIG: The number of eyes the user requires rendering for -- redundant with window */
		float		color[3];	/* CONFIG: The default color for representing the user. */
#undef DATA_SPACER	/* 16384 */
#ifdef DATA_SPACER
		char	tmp1[DATA_SPACER];
#endif
	struct	vr6sensor_st	*head;		/* The sensor used for calculating the visual rendering perspective matrix. */
#ifdef DATA_SPACER
		char	tmp2[DATA_SPACER];
#endif
		vrMatrix	*rw2vw_xform;	/* transform from real-world (user) to virtual-world CS */
#ifdef DATA_SPACER
		char	tmp3[DATA_SPACER];
#endif
		vrMatrix	*vw2rw_xform;	/* transform from virtual-world to real-world (user) CS */
#ifdef DATA_SPACER
		char	tmp4[DATA_SPACER];
#endif

		vrMatrix	*visren_headpos;/* visren sync'd value of head position */
#ifdef DATA_SPACER
		char	tmp5[DATA_SPACER];
#endif
		vrMatrix	*visren_rw2vw;	/* visren sync'd copy of rw2vw_xform */
#ifdef DATA_SPACER
		char	tmp6[DATA_SPACER];
#endif
		vrMatrix	*visren_vw2rw;	/* visren sync'd copy of vw2rw_xform */

		/**************************************/
		/* generic information about the user */
		int		num_inputs;	/* CONFIG: The number of inputs associated with this user. */
		char		**input_names;	/* CONFIG: The names of the inputs associated with this user. */
	struct	vrGenericInput_st **input_data;	/* pointers to the inputs determined from sensor_names */

		/*******************************************************/
		/* callback routines for rendering for a specific user */
		vrCallback	*VisrenInit;
		vrCallback	*VisrenFrame;
		vrCallback	*VisrenWorld;
		vrCallback	*VisrenSim;
		vrCallback	*VisrenExit;
	} vrUserInfo;


/**************************************************************/
/* Each prop maintains the following data:                    */
/**************************************************************/
typedef struct vrPropInfo_st {

		/* Generic Object fields */
		vrObject	object_type;	/* The type of this object (ie. VROBJECT_PROP) */
		int		id;		/* id number of this prop */
		char		*name;		/* CONFIG: name associated with this prop */
		int		malleable;	/* CONFIG: Whether the data of this prop can be modified */
	struct	vrPropInfo_st	*next;		/* The next prop in a linked list         */
		vrContextInfo	*context;	/* pointer to the vrContext memory structure */
		int		line_created;	/* line number of where this object was created in config */
		int		line_lastmod;	/* line number of last time this object modified in config */
		char		file_created[512];/* file (or other) where this object was created*/
		char		file_lastmod[512];/* file (or other) where this object was created*/

		/* Fields specific to Props */
		void		*nothing;	/* otherwise, not much associated with props yet. */
	} vrPropInfo;



typedef struct vrEyeInfo_st {
		/***************************/
		/* Generic Object field(s) */
		vrObject	object_type;	/* The type of this object (ie. VROBJECT_EYEINFO) */
		/* NOTE: the rest of the generic object fields not included with individual inputs*/
		/* TODO: consider adding the other object fields to the input structures.         */

		/***************************/
		vrEyeType	type;
		int		num;		/* the number of this eye in running system (1-based) */
	struct	vrUserInfo_st	*user;
		vrAnaglyphType	color;
		vrFrameBufferType render_framebuffer;	/* the frame buffer to render in */
		char		*name;		/* CONFIG: this is the string parsed into the other fields */
	struct	vrEyeInfo_st	*next;		/* the next eye in a linked list */

		vrPoint		loc;		/* location of the eye */
	} vrEyeInfo;


typedef struct vrEyelistInfo_st {

		/* Generic Object fields */
		vrObject	object_type;	/* The type of this object (ie. VROBJECT_EYELIST) */
		int		id;		/* numeric id for this eyelist. */
		char		*name;		/* CONFIG: The name assigned to this eyelist. */
		int		malleable;	/* CONFIG: Whether the data of this eyelist can be modified */
	struct vrEyelistInfo_st	*next;		/* the next eyelist in a linked list */
		vrContextInfo	*context;	/* pointer to the vrContext memory structure */
		int		line_created;	/* line number of where this object was created in config */
		int		line_lastmod;	/* line number of last time this object modified in config */
		char		file_created[512];/* file (or other) where this object was created*/
		char		file_lastmod[512];/* file (or other) where this object was created*/

		/* Fields specific to EyeLists */
		char		**monofb_names;	/* list of eye names -- to be parsed */
		int		num_monofb_names;/* number in list of names */
		vrEyeInfo	*monofb;	/* or this could be "single" */
		char		**leftfb_names;	/* list of eye names -- to be parsed */
		int		num_leftfb_names;/* number in list of names */
		vrEyeInfo	*leftfb;	/* or this could be "dualfb_left" */
		char		**rightfb_names;/* list of eye names -- to be parsed */
		int		num_rightfb_names;/* number in list of names */
		vrEyeInfo	*rightfb;	/* or this could be "dualfb_right" */
		char		**leftvp_names;	/* list of eye names -- to be parsed */
		int		num_leftvp_names;/* number in list of names */
		vrEyeInfo	*leftvp;	/* or this could be "dualvp_left" */
		char		**rightvp_names;/* list of eye names -- to be parsed */
		int		num_rightvp_names;/* number in list of names */
		vrEyeInfo	*rightvp;	/* or this could be "dualvp_right" */
		char		**anaglfb_names;/* list of eye names -- to be parsed */
		int		num_anaglfb_names;/* number in list of names */
		vrEyeInfo	*anaglfb;	/* or this could be "single" */

	} vrEyelistInfo;


/***** Functions *****/

char		*vrEyeTypeName(vrEyeType type);
vrEyeType	vrEyeValue(char *name);
char		*vrAnaglyphTypeName(vrAnaglyphType type);
vrAnaglyphType	vrAnaglyphValue(char *name);
char		*vrPrintStyleName(vrPrintStyle style);
vrPrintStyle	vrPrintStyleValue(char *name);
vrEyeInfo	*vrMakeEye(vrContextInfo *context, char *eyestring);
void		vrFprintEyeInfo(FILE *file, vrEyeInfo *eyeinfo, vrPrintStyle style);
void		vrEyelistInitialize(vrContextInfo *context, vrEyelistInfo *eyelistinfo);

void		vrUserInitialize(vrContextInfo *context);
int		vrUsersInitialized(vrContextInfo *context);
void		vrUserClear(vrUserInfo *object);
void		vrUserCopy(vrUserInfo *dest_object, vrUserInfo *src_object);
void		vrFprintUserInfo(FILE *file, vrUserInfo *userinfo, vrPrintStyle style);

void		vrPropClear(vrPropInfo *object);
void		vrPropCopy(vrPropInfo *dest_object, vrPropInfo *src_object);
void		vrPropFreezeVisren(vrContextInfo *context);
void		vrFprintPropInfo(FILE *file, vrPropInfo *propinfo, vrPrintStyle style);

vrMatrix	*vrMatrixGetRWFromUserHead(vrMatrix *headmat, int usernum);
vrMatrix	*vrMatrixGetVWUserFromRWMatrix(vrMatrix *virtual_mat, int usernum, vrMatrix *real_mat);
vrPoint		*vrPointGetVWUserFromRWPoint(vrPoint *virtual_pnt, int usernum, vrPoint *real_pnt);
vrPoint		*vrPointGetRWFromVWUserPoint(vrPoint *real_pnt, int usernum, vrPoint *virtual_pnt);
vrPointf	*vrPointfGetRWFromVWUserPointf(vrPointf *real_pnt, int usernum, vrPointf *virtual_pnt);
vrVector	*vrVectorUserVWFromRW(int usernum, vrVector *virtual_vec, vrVector *real_vec);
vrPoint		*vrPointGetRWLocationFromMatrix(vrPoint *real_point, vrMatrix *mat);
vrPoint		*vrPointGetVWFromUserMatrix(vrPoint *virtual_point, int usernum, vrMatrix *mat);

vrMatrix	*vrMatrixGetUserTravel(vrMatrix *mat, int usernum);
vrMatrix	*vrMatrixGetUserTravelInv(vrMatrix *mat, int usernum);
int		vrUserTravelReset(int usernum);
int		vrUserTravelTranslate3d(int usernum, double x, double y, double z);
int		vrUserTravelTranslateAd(int usernum, double *array);
int		vrUserTravelTranslateVec(int usernum, vrVector *vec);
int		vrUserTravelRotateId(int usernum, int axis, double theta);
int		vrUserTravelScale(int usernum, double scale);
int		vrUserTravelTransformMatrix(int usernum, vrMatrix *mat);
int		vrUserTravelLockSet(int usernum);
int		vrUserTravelLockRelease(int usernum);
void		vrUserTravelFreezeVisren(vrContextInfo *context);

#ifdef GFX_PERFORMER /* { */
/***********************************************************************************/
/* Utility functions that translate FreeVR coordinates in to Performer Z-up coords */
/***********************************************************************************/
int		vrPfUserTravelTranslate3d(int usernum, double x, double y, double z);
int		vrPfUserTravelReset(int usernum);
int		vrPfUserTravelTranslate3d(int usernum, double x, double y, double z);
int		vrPfUserTravelTranslateAd(int usernum, double *array);
int		vrPfUserTravelTranslateVec(int usernum, vrVector *vec);
int		vrPfUserTravelRotateId(int usernum, int axis, double theta);
int		vrPfUserTravelScale(int usernum, double scale);
int		vrPfUserTravelTransformMatrix(int usernum, vrMatrix *mat);
#endif /* GFX_PERFORMER } */

void		vrEyelistClear(vrEyelistInfo *object);
void		vrEyelistCopy(vrEyelistInfo *dest_object, vrEyelistInfo *src_object);
void		vrFprintEyelistInfo(FILE *file, vrEyelistInfo *eyelistinfo, vrPrintStyle style);


#ifdef __cplusplus
}
#endif

#endif  /* __VRENTITY_H__ */
