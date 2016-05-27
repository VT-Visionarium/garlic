/* ======================================================================
 *
 *  CCCCC          vr_objects.c
 * CC   CC         Author(s): Bill Sherman, John Stone
 * CC              Created: October 21, 1998
 * CC   CC         Last Modified: March 3, 2003 (03/03/03!)
 *  CCCCC
 *
 * Code file for FreeVR object structures.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#include <string.h>  /* needed for string functions */
#include "vr_objects.h"
#include "vr_config.h"
#include "vr_input.h"
#include "vr_procs.h"
#include "vr_entity.h"		/* needed for declaration of vrFprintUserInfo() and vrFprintPropInfo() */
#include "vr_debug.h"


/**********************************************************************/
void vrObjectListsInitialize(vrObjectLists *lists)
{
	lists->systemsDefined = 0;		/* Number of systems defined so far.             */
	lists->systemDefinitions = NULL;	/* Linked list for storing possible system defs. */
	lists->systemLast = NULL;		/* The last system on the linked list.           */

	lists->procsDefined = 0;		/* Number of procs defined so far.               */
	lists->procDefinitions = NULL;	/* Linked list for storing possible proc defs.   */
	lists->procLast = NULL;		/* The last process on the linked list.          */

	lists->windowsDefined = 0;		/* Number of windows defined so far.             */
	lists->windowDefinitions = NULL;	/* Linked list for storing possible window defs. */
	lists->windowLast = NULL;		/* The last window on the linked list.           */

	lists->eyelistsDefined = 0;		/* Number of eyelists defined so far.            */
	lists->eyelistDefinitions = NULL;	/* Linked list for storing possible eyelist defs.*/
	lists->eyelistLast = NULL;		/* The last eyelist on the linked list.          */

	lists->usersDefined = 0;		/* Number of users defined so far.               */
	lists->userDefinitions = NULL;	/* Linked list for storing possible user defs.   */
	lists->userLast = NULL;		/* The last user on the linked list.             */

	lists->propsDefined = 0;		/* Number of props defined so far.               */
	lists->propDefinitions = NULL;	/* Linked list for storing possible prop defs.   */
	lists->propLast = NULL;		/* The last prop on the linked list.             */

	lists->indevsDefined = 0;		/* Number of Indevs defined so far.              */
	lists->indevDefinitions = NULL;	/* Linked list for storing possible Indev defs.  */
	lists->indevLast = NULL;		/* The last Input device on the linked list.     */

	lists->inputsDefined = 0;		/* Number of Inputs defined so far.              */
	lists->inputDefinitions = NULL;	/* Linked list for storing possible Input defs.  */
	lists->inputLast = NULL;		/* The last Input on the linked list.            */
}


/**********************************************************************/
char *vrObjectTypeName(vrObject type)
{
	switch (type) {
	case VROBJECT_CALLBACK:
		return "Callback";
	case VROBJECT_SYSTEM:
		return "System";
	case VROBJECT_PROCESS:
		return "Process";
	case VROBJECT_WINDOW:
		return "Window";
	case VROBJECT_EYELIST:
		return "Eyelist";
	case VROBJECT_USER:
		return "User";
	case VROBJECT_PROP:
		return "Prop";
	case VROBJECT_INDEV:
		return "InputDevice";
	case VROBJECT_INPUT:
		return "Input";

	case VROBJECT_ADDRESS:
		return "Address";
	case VROBJECT_CONTEXT:
		return "Context";
	case VROBJECT_CONFIGINFO:
		return "ConfigInfo";
	case VROBJECT_INPUTINFO:
		return "InputInfo";
	case VROBJECT_INPUTDATA:
		return "InputData";
	case VROBJECT_EYEINFO:
		return "EyeInfo";
	case VROBJECT_LOCK:
		return "Lock";
	case VROBJECT_BARRIER:
		return "Barrier";

	case VROBJECT_NONE:
	case VROBJECT_EOL_OBJECTS:
	case VROBJECT_ENDOFLIST:
	default:
		return "unknown";
	}
}


/**********************************************************************/
vrObject vrObjectType(char *name)
{
	if (!strncasecmp(name, "ca", 2))	return VROBJECT_CALLBACK;
	if (!strncasecmp(name, "sys", 3))	return VROBJECT_SYSTEM;
	if (!strncasecmp(name, "proc", 4))	return VROBJECT_PROCESS;
	if (!strncasecmp(name, "win", 3))	return VROBJECT_WINDOW;
	if (!strncasecmp(name, "eyel", 4))	return VROBJECT_EYELIST;
	if (!strcasecmp(name, "user"))		return VROBJECT_USER;
	if (!strcasecmp(name, "prop"))		return VROBJECT_PROP;
	if (!strncasecmp(name, "ind", 3))	return VROBJECT_INDEV;
	if (!strncasecmp(name, "dev", 3))	return VROBJECT_INDEV;
	if (!strncasecmp(name, "inp", 3))	return VROBJECT_INPUT;

	if (!strncasecmp(name, "a", 1))		return VROBJECT_ADDRESS;
	if (!strncasecmp(name, "cont", 4))	return VROBJECT_CONTEXT;
	if (!strncasecmp(name, "conf", 4))	return VROBJECT_CONFIGINFO;
	if (!strncasecmp(name, "inputinfo", 9))	return VROBJECT_INPUTINFO;
	if (!strncasecmp(name, "inputdata", 9))	return VROBJECT_INPUTDATA;
	if (!strncasecmp(name, "eyei", 4))	return VROBJECT_EYEINFO;
	if (!strncasecmp(name, "b", 1))		return VROBJECT_BARRIER;
	if (!strncasecmp(name, "l", 1))		return VROBJECT_LOCK;

	return VROBJECT_NONE;
}


/**********************************************************************/
void vrRemoveAllObjects(vrContextInfo *context)
{
	vrObjectLists	*lists = context->object_lists;

	lists->systemsDefined = 0;
	lists->procsDefined = 0;
	lists->windowsDefined = 0;
	lists->eyelistsDefined = 0;
	lists->usersDefined = 0;
	lists->propsDefined = 0;
	lists->indevsDefined = 0;
	lists->inputsDefined = 0;
}


/**********************************************************************/
/* Print the information of an object of any type -- type is encoded  */
/*   in the object structure, so no need to know what the type is.    */
void vrFprintObjectInfo(FILE *fp, vrObjectInfo *object, vrPrintStyle style)
{
	if (object == NULL) {
		vrFprintf(fp, "Warning: " RED_TEXT "attempt to print NULL object.\n" NORM_TEXT);
		return;
	}

	switch (object->object_type) {

	case VROBJECT_CALLBACK:
		vrFprintCallback(fp, (void *)object, style);
		break;

	case VROBJECT_SYSTEM:
		vrFprintSystemInfo(fp, (void *)object, style);
		break;

	case VROBJECT_PROCESS:
		vrFprintProcessInfo(fp, (void *)object, style);
		break;

	case VROBJECT_WINDOW:
		vrFprintWindowInfo(fp, (void *)object, style);
		break;

	case VROBJECT_EYELIST:
		vrFprintEyelistInfo(fp, (void *)object, style);
		break;

	case VROBJECT_EYEINFO:
		vrFprintEyeInfo(fp, (void *)object, style);
		break;

	case VROBJECT_USER:
		vrFprintUserInfo(fp, (void *)object, style);
		break;

	case VROBJECT_PROP:
		vrFprintPropInfo(fp, (void *)object, style);
		break;

	case VROBJECT_INDEV:
		vrFprintInputDevice(fp, (void *)object, style);
		break;

	case VROBJECT_INPUT:
		vrFprintInputObject(fp, (void *)object, style);
		break;

	case VROBJECT_INPUTDATA:
		vrFprintInputValue(fp, (void *)object, style);
		break;

	case VROBJECT_INPUTINFO:
		vrFprintInput(fp, (void *)object, style);
		break;

	case VROBJECT_CONTEXT:
		vrFprintContext(fp, (void *)object, style);
		break;

	case VROBJECT_CONFIGINFO:
		vrFprintConfig(fp, (void *)object, style);
		break;

	case VROBJECT_LOCK:
		vrFprintLock(fp, (void *)object, style);
		break;

	case VROBJECT_BARRIER:
		vrFprintBarrier(fp, (void *)object, style);
		break;

	case VROBJECT_NONE:
	case VROBJECT_EOL_OBJECTS:
	case VROBJECT_ENDOFLIST:
	default:
		vrErrPrintf("vrFprintObjectInfo(): " RED_TEXT "Unknown or no object type given by object %#p.\n" NORM_TEXT, object);
		vrFprintf(fp, RED_TEXT "Unknown or no object type given by object %#p.\n" NORM_TEXT, object);
		break;
	}
}


/**********************************************************************/
/* Print the information of an object of the given type -- because    */
/*   objects of different types can have the same name, the name is   */
/*   not sufficient and the type must also be specified.              */
/* If "list" is given as the value of <name>, then list all the       */
/*   objects of the given type rather than print a specific object.   */
void vrFprintObjectTypeInfo(FILE *fp, vrContextInfo *context, char *typestr, char *name, vrPrintStyle style)
{
	vrObject	type;
	vrObjectInfo	*object = NULL;
	int		count;

	/* First check whether "list" is given as object type, if so print list of object types */
	if (!strcasecmp(typestr, "list")) {
		for (count = VROBJECT_NONE+1; count < VROBJECT_ENDOFLIST; count++) {
			if (count != VROBJECT_INPUT)
				/* Input objects aren't part of the configuration */
				vrFprintf(fp, "  %s\n", vrObjectTypeName((vrObject)count));
		}
		return;
	}

	/* verify whether a valid type was given */
	type = vrObjectType(typestr);
	if (type == VROBJECT_NONE) {
		vrFprintf(fp, "Warning: " RED_TEXT "invalid object type given ('%s').\n" NORM_TEXT, typestr);
		for (count = VROBJECT_NONE+1; count < VROBJECT_ENDOFLIST; count++) {
			vrFprintf(fp, "  %s\n", vrObjectTypeName((vrObject)count));
		}
		return;
	}

	if (type == VROBJECT_INPUT) {
		vrFprintf(fp, "Warning: " RED_TEXT "Input objects aren't separate configuration objects.\n" NORM_TEXT, typestr);
		return;
	}

	if (!strcasecmp(name, "list")) {
		/* For the special "list" name, print a list of all <type> objects */
		object = vrObjectFirst(context, type);
		if (object == NULL) {
			vrFprintf(fp, "No '%s' objects in configuration.\n", typestr);
			return;
		}
		count = 0;
		while (object != NULL) {
			vrFprintf(fp, "%s object %2d: '%s'\n", typestr, count, object->name);
			object = object->next;
			count++;
		}
		return;
	}

	object = vrObjectSearch(context, type, name);
	if (object == NULL) {
		vrFprintf(fp, "Warning: " RED_TEXT "invalid object given -- NULL.\n" NORM_TEXT);
		return;
	}
	vrFprintObjectInfo(fp, object, style);

	return;
}


/****************************************************************/
/* Routines for creating new (empty) objects on the linked lists */
/****************************************************************/


/**********************************************************************/
vrObjectInfo *vrObjectFirst(vrContextInfo *context, vrObject objtype)
{
	vrObjectLists	*lists = context->object_lists;
	vrObjectInfo	*object = NULL;

	switch (objtype) {

	case VROBJECT_CALLBACK:
		object = (vrObjectInfo *)lists->callbackDefinitions;
		break;

	case VROBJECT_SYSTEM:
		object = (vrObjectInfo *)lists->systemDefinitions;
		break;

	case VROBJECT_PROCESS:
		object = (vrObjectInfo *)lists->procDefinitions;
		break;

	case VROBJECT_WINDOW:
		object = (vrObjectInfo *)lists->windowDefinitions;
		break;

	case VROBJECT_EYELIST:
		object = (vrObjectInfo *)lists->eyelistDefinitions;
		break;

	case VROBJECT_USER:
		object = (vrObjectInfo *)lists->userDefinitions;
		break;

	case VROBJECT_PROP:
		object = (vrObjectInfo *)lists->propDefinitions;
		break;

	case VROBJECT_INDEV:
		object = (vrObjectInfo *)lists->indevDefinitions;
		break;

	case VROBJECT_INPUT:
		object = (vrObjectInfo *)lists->inputDefinitions;
		break;

	case VROBJECT_NONE:
	default:
		vrErrPrintf(RED_TEXT "Sorry, no such object type.\n" NORM_TEXT);
		break;

	}

	return object;
}


/**********************************************************************/
/* Make a new object of the given type, and add that object to the global */
/*   linked list.                                                         */
void *vrObjectNew(vrContextInfo *context, vrObject objtype, char *name)
{
	vrObjectLists	*lists = context->object_lists;
	vrObjectInfo	*object;
	int		*objects_defined;
	vrObjectInfo	**object_definitions;
	vrObjectInfo	**object_last;

	switch (objtype) {

	case VROBJECT_CALLBACK:
		object = (vrObjectInfo *)vrShmemAlloc0(sizeof(vrCallback));
		objects_defined = &lists->callbacksDefined;
		object_definitions = (vrObjectInfo **)&lists->callbackDefinitions;
		object_last = (vrObjectInfo **)&lists->callbackLast;
		break;

	case VROBJECT_SYSTEM:
		object = (vrObjectInfo *)vrShmemAlloc0(sizeof(vrSystemInfo));
		objects_defined = &lists->systemsDefined;
		object_definitions = (vrObjectInfo **)&lists->systemDefinitions;
		object_last = (vrObjectInfo **)&lists->systemLast;
		break;

	case VROBJECT_PROCESS:
		object = (vrObjectInfo *)vrShmemAlloc0(sizeof(vrProcessInfo));
		objects_defined = &lists->procsDefined;
		object_definitions = (vrObjectInfo **)&lists->procDefinitions;
		object_last = (vrObjectInfo **)&lists->procLast;
		break;

	case VROBJECT_WINDOW:
		object = (vrObjectInfo *)vrShmemAlloc0(sizeof(vrWindowInfo));
		objects_defined = &lists->windowsDefined;
		object_definitions = (vrObjectInfo **)&lists->windowDefinitions;
		object_last = (vrObjectInfo **)&lists->windowLast;
		break;

	case VROBJECT_EYELIST:
		object = (vrObjectInfo *)vrShmemAlloc0(sizeof(vrEyelistInfo));
		objects_defined = &lists->eyelistsDefined;
		object_definitions = (vrObjectInfo **)&lists->eyelistDefinitions;
		object_last = (vrObjectInfo **)&lists->eyelistLast;
		break;

	case VROBJECT_USER:
		object = (vrObjectInfo *)vrShmemAlloc0(sizeof(vrUserInfo));
		objects_defined = &lists->usersDefined;
		object_definitions = (vrObjectInfo **)&lists->userDefinitions;
		object_last = (vrObjectInfo **)&lists->userLast;
		break;

	case VROBJECT_PROP:
		object = (vrObjectInfo *)vrShmemAlloc0(sizeof(vrPropInfo));
		objects_defined = &lists->propsDefined;
		object_definitions = (vrObjectInfo **)&lists->propDefinitions;
		object_last = (vrObjectInfo **)&lists->propLast;
		break;

	case VROBJECT_INDEV:
		object = (vrObjectInfo *)vrShmemAlloc0(sizeof(vrInputDevice));
		objects_defined = &lists->indevsDefined;
		object_definitions = (vrObjectInfo **)&lists->indevDefinitions;
		object_last = (vrObjectInfo **)&lists->indevLast;
		break;

	case VROBJECT_INPUT:
		object = (vrObjectInfo *)vrShmemAlloc0(sizeof(vrInput));
		objects_defined = &lists->inputsDefined;
		object_definitions = (vrObjectInfo **)&lists->inputDefinitions;
		object_last = (vrObjectInfo **)&lists->inputLast;
		break;

	case VROBJECT_NONE:
	default:
		vrErrPrintf(RED_TEXT "Sorry, no such object.\n" NORM_TEXT);
		break;

	}

	/* set some initial values */
	object->object_type = objtype;
	object->id = *objects_defined;
	(*objects_defined)++;

	object->name = vrShmemStrDup(name);
	object->malleable = 1;
	object->context = context;
	object->line_created = -1;
	object->line_lastmod = -1;
	object->file_created[0] = '\0';
	object->file_lastmod[0] = '\0';
	vrObjectClear(object);

	/* add to the linked list */
	object->next = NULL;
	if (*object_definitions == NULL)
		*object_definitions = object;
	else	(*object_last)->next = object;
	*object_last = object;

	if (vrDbgDo(SELDOM_DBGLVL+1)) {
		vrMsgPrintf("Created a new Object: ");
		vrFprintObjectInfo(stdout, object, verbose);
	}

	return (object);
}


/*******************************************************************************/
/* Make a new object of the given type, but instead of adding it to the global */
/*   linked list, add it to a private list.                                    */
void *vrObjectListNew(vrContextInfo *context, vrObject objtype, vrObjectInfo **object_linked_list, char *name)
{
	vrObjectLists	*lists = context->object_lists;
	vrObjectInfo	*object;
	int		*objects_defined;

	switch (objtype) {

	case VROBJECT_CALLBACK:
		object = (vrObjectInfo *)vrShmemAlloc0(sizeof(vrCallback));
		objects_defined = &lists->callbacksDefined;
		break;

	case VROBJECT_SYSTEM:
		object = (vrObjectInfo *)vrShmemAlloc0(sizeof(vrSystemInfo));
		objects_defined = &lists->systemsDefined;
		break;

	case VROBJECT_PROCESS:
		object = (vrObjectInfo *)vrShmemAlloc0(sizeof(vrProcessInfo));
		objects_defined = &lists->procsDefined;
		break;

	case VROBJECT_WINDOW:
		object = (vrObjectInfo *)vrShmemAlloc0(sizeof(vrWindowInfo));
		objects_defined = &lists->windowsDefined;
		break;

	case VROBJECT_EYELIST:
		object = (vrObjectInfo *)vrShmemAlloc0(sizeof(vrEyelistInfo));
		objects_defined = &lists->eyelistsDefined;
		break;

	case VROBJECT_USER:
		object = (vrObjectInfo *)vrShmemAlloc0(sizeof(vrUserInfo));
		objects_defined = &lists->usersDefined;
		break;

	case VROBJECT_PROP:
		object = (vrObjectInfo *)vrShmemAlloc0(sizeof(vrPropInfo));
		objects_defined = &lists->propsDefined;
		break;

	case VROBJECT_INDEV:
		object = (vrObjectInfo *)vrShmemAlloc0(sizeof(vrInputDevice));
		objects_defined = &lists->indevsDefined;
		break;

	case VROBJECT_INPUT:
		object = (vrObjectInfo *)vrShmemAlloc0(sizeof(vrInput));
		objects_defined = &lists->inputsDefined;
		break;

	case VROBJECT_NONE:
	default:
		vrErrPrintf(RED_TEXT "Sorry, no such object.\n" NORM_TEXT);
		break;

	}

	/* set some initial values */
	object->object_type = objtype;
	object->id = *objects_defined;
	(*objects_defined)++;

	object->name = vrShmemStrDup(name);
	object->malleable = 1;
	vrObjectClear(object);

	/* add to the linked list */
	vrObjectsAddToList(object_linked_list, object);

	if (vrDbgDo(SELDOM_DBGLVL+1)) {
		vrMsgPrintf("Created a new Object: ");
		vrFprintObjectInfo(stdout, object, verbose);
	}

	return (object);
}


/**************************************************************************/
/* Add an object to the end of a linked list specified by "list".  If     */
/*   the given list is NULL, then make the "object" the first node on     */
/*   the list.  NOTE: the reason "list" is a doubly dereferenced argument */
/*   is to enable us to make the object the first node of the list.       */
void vrObjectsAddToList(vrObjectInfo **list, vrObjectInfo *object)
{
	vrObjectInfo	*last_object;

	object->next = NULL;
	if (*list == NULL) {
		*list = object;
		return;
	}

	/* step through the list to the last object */
	for (last_object = *list; last_object->next != NULL; last_object = last_object->next);

	/* add to the linked list */
	last_object->next = object;

	return;
}


/*******************************************************************************/
/* vrObjectSearch(): search for an object of type <name> in the global object  */
/*   space for the given type.                                                 */
void *vrObjectSearch(vrContextInfo *context, vrObject objtype, char *name)
{
	vrObjectInfo	*object;
	vrObjectInfo	*retobj = NULL;

	/******************************************/
	/** handle address of objects separately **/
	if (objtype == VROBJECT_ADDRESS) {
		object = vrAtoP(name);
		return object;
	}

	/***************************************************/
	/** for non-address types, search the linked list **/
	object = vrObjectFirst(context, objtype);

	while (object != NULL) {
		vrDbgPrintfN(OBJSEARCH_DBGLVL, "object(%d) search: comparing '%s' with '%s'\n", object->object_type, name, object->name);
		if (!strcasecmp(name, object->name)) {
			if (retobj == NULL) {
				vrTrace("vrObjectSearch", "found an object name match");
				retobj = object;
			} else {
				vrErrPrintf(RED_TEXT "Warning: extra definition of %s object \"%s\" -- THIS SHOULD NOT HAPPEN!\n" NORM_TEXT, vrObjectTypeName(objtype), name);
printf("retobj = %p, object = %p\n", retobj, object);
			}
		}
		object = object->next;
	}

	return retobj;
}


/*********************************************************************/
/* vrObjectListSearch(): same as vrObjectSearch(), but not from the  */
/*   global object space.                                            */
void *vrObjectListSearch(vrObject objtype, vrObjectInfo *object_linked_list, char *name)
{
	vrObjectInfo	*object;
	vrObjectInfo	*retobj = NULL;

	object = object_linked_list;

	while (object != NULL) {
		vrDbgPrintfN(OBJSEARCH_DBGLVL, "object(%d) search: comparing '%s' with '%s'\n",
			object->object_type, name, object->name);
		if (!strcasecmp(name, object->name)) {
			if (retobj == NULL) {
				vrTrace("vrObjectSearch", "found an object name match");
				retobj = object;
			} else {
				vrErrPrintf(RED_TEXT "Warning: extra definition of %s object \"%s\" -- THIS SHOULD NOT HAPPEN!\n" NORM_TEXT, vrObjectTypeName(objtype), name);
			}
		}
		object = object->next;
	}

	return retobj;
}


/**********************************************************************/
void *vrObjectArraySearch(vrObject objtype, vrObjectInfo *objarray[], int numobjs, char *name)
{
	vrObjectInfo	*retobj = NULL;
	int		count;

	for (count = 0; count < numobjs; count++) {
		vrDbgPrintfN(OBJSEARCH_DBGLVL, "object(%d) array search: comparing '%s' with '%s'\n", objarray[count]->name, name);
		if (!strcmp(objarray[count]->name, name)) {
			if (retobj == NULL) {
				vrTrace("vrObjectSearch", "found an object name match");
				retobj = objarray[count];
			} else {
				vrErrPrintf(RED_TEXT "Warning: extra definition of %s object \"%s\" -- THIS SHOULD NOT HAPPEN!\n" NORM_TEXT, vrObjectTypeName(objtype), name);
			}
		}
	}

	return retobj;
}


/**********************************************************************/
void vrObjectClear(void *object)
{
	switch (((vrObjectInfo *)(object))->object_type) {

	case VROBJECT_SYSTEM:
		vrSystemClear(object);
		break;

	case VROBJECT_PROCESS:
		vrProcessClear(object);
		break;

	case VROBJECT_WINDOW:
		vrWindowClear(object);
		break;

	case VROBJECT_EYELIST:
		vrEyelistClear(object);
		break;

	case VROBJECT_USER:
		vrUserClear(object);
		break;

	case VROBJECT_PROP:
		vrPropClear(object);
		break;

	case VROBJECT_INDEV:
		vrInputDeviceClear(object);
		break;

	case VROBJECT_INPUT:
		vrInputObjectClear(object);
		break;

	case VROBJECT_CALLBACK:
		/* nothing to do, vrCallbackCreate() will take care of it */
		break;

	case VROBJECT_NONE:
	default:
		vrErrPrintf(RED_TEXT "Sorry, no such object type.\n" NORM_TEXT);
		break;

	}
}


/**********************************************************************/
/* Note: all the generic fields of vrObjectInfo are unique for  */
/*   the duplicated object -- they are not copies of the source */
/*   object (except the pointer to the system context.          */
void vrObjectCopy(vrContextInfo *context, void *dest_object, void *source_object)
{
	switch (((vrObjectInfo *)(source_object))->object_type) {

	case VROBJECT_SYSTEM:
		vrSystemCopy(dest_object, source_object);
		break;

	case VROBJECT_PROCESS:
		vrProcessCopy(dest_object, source_object);
		break;

	case VROBJECT_WINDOW:
		vrWindowCopy(dest_object, source_object);
		break;

	case VROBJECT_EYELIST:
		vrEyelistCopy(dest_object, source_object);
		break;

	case VROBJECT_USER:
		vrUserCopy(dest_object, source_object);
		break;

	case VROBJECT_PROP:
		vrPropCopy(dest_object, source_object);
		break;

	case VROBJECT_INDEV:
		vrInputDeviceCopy(context, dest_object, source_object);
		break;

	case VROBJECT_INPUT:
		vrInputObjectCopy(dest_object, source_object);
		break;

	case VROBJECT_CALLBACK:
		/* nothing to do, no reason to copy callbacks */
		break;

	case VROBJECT_NONE:
	default:
		vrErrPrintf(RED_TEXT "Sorry, no such object type.\n" NORM_TEXT);
		break;

	}

	((vrObjectInfo *)dest_object)->context = ((vrObjectInfo *)source_object)->context;

	return;
}


/**********************************************************************/
/* [03/08/2006 -- currently this function is only used by vrObjectCopyLinkedList. */
/*   The reason I mention this is that I was thinking of now creating a vrObjectDup() */
/*   function that is basically like what vrObjectCopy() now is, but that also does   */
/*   the allocation of the memory, etc -- probably just using vrObjectNew().          */
void *vrObjectDup(vrContextInfo *context, void **object_linked_list, void *source_object)
{
	void	*dest_object;

	/* create a new object (vrObjectListNew() does the allocation), */
	/*   and then add that object to the given linked list.         */
	dest_object = vrObjectListNew(context,
		(((vrObjectInfo *)(source_object))->object_type),	/* make new object same type as source object */
		  (vrObjectInfo **)object_linked_list,			/* put new object on the given linked list    */
		 ((vrObjectInfo *)(source_object))->name);		/* give new object same name as source object */

	/* now copy type-related details from source object to new object */
	vrObjectCopy(context, dest_object, source_object);

	return dest_object;
}


/**********************************************************************/
/* [03/08/2006 -- currently this function is only used by the vrInputDeviceCopy() */
/*   function in order to make duplicate lists of the inputs and self-controls.]  */
void *vrObjectCopyLinkedList(vrContextInfo *context, void *object_list_to_copy)
{
	vrObjectInfo	*dest_object;
	vrObjectInfo	*object_linked_list = NULL;	/* an empty linked list that we will add objects onto */

	if (object_list_to_copy == NULL)
		return NULL;

	/* duplicate the first object on the list */
	/* [03/08/2006: Hmmm, I'm not sure this is necessary since we check for NULL of  */
	/*    the initial list above -- ie. I think the loop alone might be sufficient.] */
	dest_object = vrObjectDup(context, (void **)(&object_linked_list), (void *)object_list_to_copy);

	/* now loop through the list duplicating the rest of the objects */
	while ((((vrObjectInfo *)object_list_to_copy)->next) != NULL) {
		object_list_to_copy = ((vrObjectInfo *)object_list_to_copy)->next;
		dest_object->next = vrObjectDup(context, (void **)(&object_linked_list), (void *)object_list_to_copy);
		dest_object = dest_object->next;
	}

	return object_linked_list;
}

