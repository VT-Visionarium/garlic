/* ======================================================================
 *
 * HH   HH         vr_objects.h
 * HH   HH         Author(s): Bill Sherman
 * HHHHHHH         Created: October 21, 1998
 * HH   HH         Last Modified: April 3, 2003
 * HH   HH
 *
 * Header file for FreeVR objects structures.  Defines the
 * generic object structures, and declares the related functions.
 *
 * Copyright 2014, Bill Sherman & Friends, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#ifndef __VROBJECTS_H__
#define __VROBJECTS_H__

#include <stdio.h>	/* for FILE typedef */

#include "vr_context.h"	/* for vrContextInfo type definition */
#include "vr_entity.h"	/* for vrObject type definition */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vrObjectInfo_st {
		/*************************/
		/* Generic Object fields */
		vrObject	object_type;	/* The type of this object (eg. VROBJECT_INDEV) */
		int		id;		/* index into the array of this-object-type */
		char		*name;		/* CONFIG: name assigned to this-object-type in config file */
		int		malleable;	/* CONFIG: Whether the data of this this-object-type can be modified */
	struct	vrObjectInfo_st	*next;		/* next input this-object-type in a linked list */
		vrContextInfo	*context;	/* pointer to the vrContext memory structure */
		int		line_created;	/* line number of where this object was created in config */
		int		line_lastmod;	/* line number of last time this object modified in config */
		char		file_created[512];/* file (or other) where this object was created */
		char		file_lastmod[512];/* file (or other) where this object was last modified */

		/* other fields are added here based on the specific type of object */
	} vrObjectInfo;


/**********************************************************************/
/* System object definitions:  This is where info is stored as it's   */
/*   read from the config file.  When all the config info is read,    */
/*   the config process figures out which of this info is used, and   */
/*   copies/creates the necessary stuff into the vrConfig and vrInputs*/
/*   structures.                                                      */
/* Basic linked lists are used to store the objects as they wait to   */
/*   be used in the configuration process -- afterwhich they'll be    */
/*   referenced from the vrConfig and vrInput structures.             */
typedef struct vrObjectLists_st {
		int		callbacksDefined;	/* Number of callbacks defined so far.    */
	struct	vrCallback_st	*callbackDefinitions;	/* Linked list for storing possible defs. */
	struct	vrCallback_st	*callbackLast;		/* The last callback on the linked list.  */

		int		systemsDefined;		/* Number of systems defined so far.      */
	struct	vrSystemInfo_st	*systemDefinitions;	/* Linked list for storing possible defs. */
	struct	vrSystemInfo_st	*systemLast;		/* The last system on the linked list.    */

		int		procsDefined;		/* Number of procs defined so far.        */
	struct	vrProcessInfo_st *procDefinitions;	/* Linked list for storing possible defs. */
	struct	vrProcessInfo_st *procLast;		/* The last process on the linked list.   */

		int		windowsDefined;		/* Number of windows defined so far.      */
	struct	vrWindowInfo_st	*windowDefinitions;	/* Linked list for storing possible defs. */
	struct	vrWindowInfo_st	*windowLast;		/* The last window on the linked list.    */

		int		eyelistsDefined;	/* Number of eyelists defined so far.     */
	struct	vrEyelistInfo_st *eyelistDefinitions;	/* Linked list for storing possible defs. */
	struct	vrEyelistInfo_st *eyelistLast;		/* The last eyelist on the linked list.   */

		int		usersDefined;		/* Number of users defined so far.        */
	struct	vrUserInfo_st	*userDefinitions;	/* Linked list for storing possible defs. */
	struct	vrUserInfo_st	*userLast;		/* The last user on the linked list.      */

		int		propsDefined;		/* Number of props defined so far.        */
	struct	vrPropInfo_st	*propDefinitions;	/* Linked list for storing possible defs. */
	struct	vrPropInfo_st	*propLast;		/* The last prop on the linked list.      */

		int		indevsDefined;		/* Number of Indevs defined so far.       */
	struct	vrID_st		*indevDefinitions;	/* Linked list for storing possible defs. */
	struct	vrID_st		*indevLast;		/* The last device on the linked list.    */

		int		inputsDefined;		/* Number of Inputs defined so far.       */
	struct	vrInput_st	*inputDefinitions;	/* Linked list for storing possible defs. */
	struct	vrInput_st	*inputLast;		/* The last Input on the linked list.     */

	} vrObjectLists;



void		 vrObjectListsInitialize(vrObjectLists *lists);
char		*vrObjectTypeName(vrObject type);
void		vrRemoveAllObjects(vrContextInfo *context);
void		vrFprintObjectInfo(FILE *fp, vrObjectInfo *object, vrPrintStyle style);
void		vrFprintObjectTypeInfo(FILE *fp, vrContextInfo *context, char *typestr, char *name, vrPrintStyle style);
vrObjectInfo	*vrObjectFirst(vrContextInfo *context, vrObject objtype);
void		*vrObjectNew(vrContextInfo *context, vrObject objtype, char *name);
void		*vrObjectListNew(vrContextInfo *context, vrObject objtype, vrObjectInfo **first_object, char *name);
void		 vrObjectsAddToList(vrObjectInfo **list, vrObjectInfo *object);
void		*vrObjectSearch(vrContextInfo *context, vrObject objtype, char *name);
void		*vrObjectListSearch(vrObject objtype, vrObjectInfo *first_object, char *name);
void		*vrObjectArraySearch(vrObject objtype, vrObjectInfo *objarray[], int numobjs, char *name);
void		 vrObjectClear(void *object);
void		 vrObjectCopy(vrContextInfo *context, void *dest_object, void *source_object);
void		*vrObjectDup(vrContextInfo *context, void **first_object, void *source_object);
void		*vrObjectCopyLinkedList(vrContextInfo *context, void *src_object);

#ifdef __cplusplus
}
#endif

#endif
