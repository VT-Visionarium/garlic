/* ======================================================================
 *
 *  CCCCC          vr_input.c
 * CC   CC         Author(s): Ed Peters, Bill Sherman, John Stone
 * CC              Created: June 4, 1998
 * CC   CC         Last Modified: August 28, 2014
 *  CCCCC
 *
 * Code file for FreeVR input processes.  See vr_input.h for a
 * description of the relevant data structures.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */

#define	COREDUMP_IS_GOOD


#include <string.h>   /* needed for string functions */
#include <stdlib.h>   /* needed for atof(), and malloc, etc */
#include "vr_input.h"
#include "vr_callback.h"
#include "vr_config.h"
#include "vr_debug.h"
#include "vr_parse.h"
#include "vr_utils.h"
#include <signal.h>

#if !defined(__hpux)
#  include <dlfcn.h>	/* for DSO access */
#endif

#define VRINPUT_ONLY	/* needed by vr_input.opts.h */
#include "vr_input.opts.h"


/* static (file-scope) globals for the (TODO: each?) input process */
static	vrProcessInfo	*proc_info = NULL;	/* NOTE: currently only used by the signal handler */
static	int		num_devices = 0;
static	vrInputDevice	**devices = NULL;


/************************************************************************/
/* function declarations of local-only functions called before defined. */
vrInputDevice	*_MakeDummyInputDevice();
vrInput		*_MakeDummyInputObject(char *function, char *description);



/*****************************************************************/
/* vrInputSignalHandler(): on a SIGINT signal, this process will */
/*   close down all devices it's responsible for and exit.       */
/*****************************************************************/
static void vrInputSignalHandler(int which)
{
	switch (which) {
	case SIGUSR2:
		vrMsgPrintf("vrInputSignalHandler(): Hey, input process got a USR2 signal\n");
		vrCallbackInvoke(vrContext->callbacks->HandleUSR2);
		break;

	default:
		/* for all other signals, assume death */
		vrMsgPrintf("vrInputSignalHandler(): (input proc %d) " RED_TEXT "dying with %s.\n" NORM_TEXT, getpid(), vrSigName(which));

		vrMsgPrintf("vrInputSignalHandler(): Time of death = %f, number of frames = %d\n",
			proc_info->frame_wtime, proc_info->frame_count);

		vrMsgPrintf("vrInputSignalHandler(): Shared memory usage:%7ld (%ld freed)\n",
			vrShmemUsage(), vrShmemFreed());

		proc_info->end_proc = 1;
		vrFprintProcessInfo(stdout, proc_info, verbose);

#ifdef COREDUMP_IS_GOOD
		vrFprintf(stderr, "vrInputSignalHandler(): YO aborting in vr_input.c\n");
		abort();
#endif

#if 1
		vrInputTermProc(proc_info);	/* TODO: not sure we want to do this */
#endif

		/* PAUSE to allow debugger to attach to the process. */
		vrMsgPrintf("vrInputSignalHandler(): Process pausing, use 'dbx -p %d' to debug.\n", getpid());
		pause();
	}
}


/*****************************************************************/
void vrInputObjectClear(vrInput *object)
{
	object->object_type = VROBJECT_INPUT;

	object->map_refcount = 0;
	object->desc_str = NULL;
	object->desc = NULL;	
#if 1
	object->r2e_xform = NULL;
#endif

	object->desc_ui[0] = '\0';
	object->container.generic = NULL;
}


/*****************************************************************/
void vrInputObjectCopy(vrInput *dest_object, vrInput *src_object)
{
	void	*dest_mem;
	void	*src_mem;
	int	memlen;

	/* copy only the memory after the generic vrObjectInfo stuff */
	dest_mem = (char *)dest_object + sizeof(vrObjectInfo);
	src_mem = (char *)src_object + sizeof(vrObjectInfo);
	memlen = sizeof(vrInput) - sizeof(vrObjectInfo);
	memcpy(dest_mem, src_mem, memlen);

	/* make independent copy of some fields */
	dest_object->r2e_xform = vrShmemMemDup(src_object->r2e_xform, sizeof(vrMatrix));
}


/**********************************************************************/
char *vrInputTypeName(vrInputType type)
{
	switch (type) {
	case VRINPUT_BINARY:
		return "2-switch";
	case VRINPUT_NWAY:
		return "N-switch";
	case VRINPUT_VALUATOR:
		return "valuator";
	case VRINPUT_KEYSTROKE:
		return "keystroke";
	case VRINPUT_TEXT:
		return "text";
	case VRINPUT_6SENSOR:
		return "6-sensor";
	case VRINPUT_NSENSOR:
		return "N-sensor";
	case VRINPUT_POSITION:
		return "position";
	case VRINPUT_PLANE:
		return "plane";
	case VRINPUT_CONTROL:
		return "control";
	case VRINPUT_UNKNOWN:
	default:
		return "unknown";
	}
}


/*****************************************************************/
/* TODO: I'm not sure about the name of this function.  */
/*   for now I'm calling it "mapname" ... */
vrGenericInput *vrInputGetFromMapname(vrContextInfo *context, char *mapname)
{
	int		index;
	vrGenericInput	*return_value;

	/* TODO: should skip beginning whitespace */

	if (!strncmp(mapname, "2-way[", 6)) {
		index = vrAtoI(&mapname[6]);
		if (index < 0 || index >= context->input->num_2ways)
			return NULL;
		return_value = (vrGenericInput *)context->input->switch2[index];
	} else
	if (!strncmp(mapname, "2-switch[", 8)) {
		index = vrAtoI(&mapname[8]);
		if (index < 0 || index >= context->input->num_2ways)
			return NULL;
		return_value = (vrGenericInput *)context->input->switch2[index];
	} else
	if (!strncmp(mapname, "2switch[", 8)) {
		index = vrAtoI(&mapname[8]);
		if (index < 0 || index >= context->input->num_2ways)
			return NULL;
		return_value = (vrGenericInput *)context->input->switch2[index];
	} else
	if (!strncmp(mapname, "switch2[", 8)) {	/* deprecated */
		index = vrAtoI(&mapname[8]);
		if (index < 0 || index >= context->input->num_2ways)
			return NULL;
		return_value = (vrGenericInput *)context->input->switch2[index];
	} else
	if (!strncmp(mapname, "s2[", 3)) {
		index = vrAtoI(&mapname[3]);
		if (index < 0 || index >= context->input->num_2ways)
			return NULL;
		return_value = (vrGenericInput *)context->input->switch2[index];
	} else

	if (!strncmp(mapname, "N-way[", 6)) {
		index = vrAtoI(&mapname[6]);
		if (index < 0 || index >= context->input->num_Nways)
			return NULL;
		return_value = (vrGenericInput *)context->input->switchN[index];
	} else
	if (!strncmp(mapname, "Nswitch[", 8)) {
		index = vrAtoI(&mapname[8]);
		if (index < 0 || index >= context->input->num_Nways)
			return NULL;
		return_value = (vrGenericInput *)context->input->switchN[index];
	} else
	if (!strncmp(mapname, "N-switch[", 8)) {
		index = vrAtoI(&mapname[8]);
		if (index < 0 || index >= context->input->num_Nways)
			return NULL;
		return_value = (vrGenericInput *)context->input->switchN[index];
	} else
	if (!strncmp(mapname, "switchN[", 8)) {	/* deprecated */
		index = vrAtoI(&mapname[8]);
		if (index < 0 || index >= context->input->num_Nways)
			return NULL;
		return_value = (vrGenericInput *)context->input->switchN[index];
	} else

	if (!strncmp(mapname, "valuator[", 9)) {
		index = vrAtoI(&mapname[9]);
		if (index < 0 || index >= context->input->num_valuators)
			return NULL;
		return_value = (vrGenericInput *)context->input->valuator[index];
	} else
	if (!strncmp(mapname, "v[", 2)) {
		index = vrAtoI(&mapname[2]);
		if (index < 0 || index >= context->input->num_valuators)
			return NULL;
		return_value = (vrGenericInput *)context->input->valuator[index];
	} else

	if (!strncmp(mapname, "6-sensor[", 9)) {
		index = vrAtoI(&mapname[9]);
		if (index < 0 || index >= context->input->num_6sensors)
			return NULL;
		return_value = (vrGenericInput *)context->input->sensor6[index];
	} else
	if (!strncmp(mapname, "6sensor[", 8)) {
		index = vrAtoI(&mapname[8]);
		if (index < 0 || index >= context->input->num_6sensors)
			return NULL;
		return_value = (vrGenericInput *)context->input->sensor6[index];
	} else
	if (!strncmp(mapname, "sensor6[", 8)) {	/* deprecated */
		index = vrAtoI(&mapname[8]);
		if (index < 0 || index >= context->input->num_6sensors)
			return NULL;
		return_value = (vrGenericInput *)context->input->sensor6[index];
	} else

	if (!strncmp(mapname, "N-sensor[", 9)) {
		index = vrAtoI(&mapname[9]);
		if (index < 0 || index >= context->input->num_Nsensors)
			return NULL;
		return_value = (vrGenericInput *)context->input->sensorN[index];
	} else
	if (!strncmp(mapname, "Nsensor[", 8)) {
		index = vrAtoI(&mapname[8]);
		if (index < 0 || index >= context->input->num_Nsensors)
			return NULL;
		return_value = (vrGenericInput *)context->input->sensorN[index];
	} else
	if (!strncmp(mapname, "sensorN[", 8)) {	/* deprecated */
		index = vrAtoI(&mapname[8]);
		if (index < 0 || index >= context->input->num_Nsensors)
			return NULL;
		return_value = (vrGenericInput *)context->input->sensorN[index];
	} else

	if (!strncmp(mapname, "control[", 8)) {
		index = vrAtoI(&mapname[8]);
		if (index < 0 || index >= context->input->num_controls)
			return NULL;
		return_value = (vrGenericInput *)context->input->control[index];
	} else

	/* default: */ {
		return_value = NULL;
	}

	vrDbgPrintfN(PARSE_DBGLVL, "vrInputGetFromMapname(%s) returning %#p.\n", mapname, return_value);

	return return_value;
}


/*****************************************************************/
vrGenericInput *vrInputGetFromTypeIndex(vrContextInfo *context, vrInputType type, int input_num)
{
	vrGenericInput	*input = NULL;

	switch (type) {
	case VRINPUT_BINARY:
		if (input_num >= context->input->num_2ways) {
			if (vrDbgDo(CONFIG_WARN_DBGLVL)) {
				if (context->input->num_2ways == 0)
					vrErrPrintf("Invalid 2switch requested (%d) -- none available.\n", input_num);
				else	vrErrPrintf("Invalid 2switch requested (%d) -- valid range is [0..%d].\n",
					input_num, context->input->num_2ways-1);

				/* Now Create a new dummy input to handle the request */
				vrDbgPrintfN(AALWAYS_DBGLVL, RED_TEXT "Creating dummy 2switch %d.\n" NORM_TEXT, input_num);
				input = vrInputAddDummyToInputMap(context->input, VRINPUT_BINARY, input_num);
			}
		} else {
			input = (vrGenericInput *)context->input->switch2[input_num];

			/* Ideally this should never happen, but until the vrInputWaitForAllInputsToBeCreated() */
			/*   routine creates dummy inputs for devices that don't initiate, this could happen.   */
			if (input == NULL) {
				vrDbgPrintfN(AALWAYS_DBGLVL, RED_TEXT "NULL input!  Creating dummy w-switch %d.\n" NORM_TEXT, input_num);
				input = vrInputAddDummyToInputMap(context->input, VRINPUT_BINARY, input_num);
				context->input->switch2[input_num] = (vr2switch *)input;
			}
		}
		break;

	case	VRINPUT_NWAY:
		if (input_num >= context->input->num_Nways) {
			if (vrDbgDo(CONFIG_WARN_DBGLVL)) {
				if (context->input->num_Nways == 0)
					vrErrPrintf("Invalid Nswitch requested (%d) -- none available.\n", input_num);
				else	vrErrPrintf("Invalid Nswitch requested (%d) -- valid range is [0..%d].\n",
					input_num, context->input->num_Nways-1);

				/* Now Create a new dummy input to handle the request */
				vrDbgPrintfN(AALWAYS_DBGLVL, RED_TEXT "Creating dummy N-switch %d.\n" NORM_TEXT, input_num);
				input = vrInputAddDummyToInputMap(context->input, VRINPUT_NWAY, input_num);
			}
		} else {
			input = (vrGenericInput *)context->input->switchN[input_num];

			/* Ideally this should never happen, but until the vrInputWaitForAllInputsToBeCreated() */
			/*   routine creates dummy inputs for devices that don't initiate, this could happen.   */
			if (input == NULL) {
				vrDbgPrintfN(AALWAYS_DBGLVL, RED_TEXT "NULL input!  Creating dummy N-switch %d.\n" NORM_TEXT, input_num);
				input = vrInputAddDummyToInputMap(context->input, VRINPUT_NWAY, input_num);
				context->input->switchN[input_num] = (vrNswitch *)input;
			}
		}
		break;

	case	VRINPUT_VALUATOR:
		if (input_num >= context->input->num_valuators) {
			if (vrDbgDo(CONFIG_WARN_DBGLVL)) {
				if (context->input->num_valuators == 0)
					vrErrPrintf("Invalid Valuator requested (%d) -- none available.\n", input_num);
				else	vrErrPrintf("Invalid Valuator requested (%d) -- valid range is [0..%d].\n",
					input_num, context->input->num_valuators-1);

				/* Now Create a new dummy input to handle the request */
				vrDbgPrintfN(AALWAYS_DBGLVL, RED_TEXT "Creating dummy Valuator %d.\n" NORM_TEXT, input_num);
				input = vrInputAddDummyToInputMap(context->input, VRINPUT_VALUATOR, input_num);
			}
		} else {
			input = (vrGenericInput *)context->input->valuator[input_num];

			/* Ideally this should never happen, but until the vrInputWaitForAllInputsToBeCreated() */
			/*   routine creates dummy inputs for devices that don't initiate, this could happen.   */
			if (input == NULL) {
				vrDbgPrintfN(AALWAYS_DBGLVL, RED_TEXT "NULL input!  Creating dummy Valuator %d.\n" NORM_TEXT, input_num);
				input = vrInputAddDummyToInputMap(context->input, VRINPUT_VALUATOR, input_num);
				context->input->valuator[input_num] = (vrValuator *)input;
			}
		}
		break;

	case	VRINPUT_6SENSOR:
		if (input_num >= context->input->num_6sensors) {
			if (vrDbgDo(CONFIG_WARN_DBGLVL)) {
				if (context->input->num_6sensors == 0)
					vrErrPrintf("Invalid Position Sensor requested (%d) -- none available.\n", input_num);
				else	vrErrPrintf("Invalid Position Sensor requested (%d) -- valid range is [0..%d].\n",
					input_num, context->input->num_6sensors-1);

				/* Now Create a new dummy input to handle the request */
				vrDbgPrintfN(AALWAYS_DBGLVL, RED_TEXT "Creating dummy Position Sensor %d.\n" NORM_TEXT, input_num);
				input = vrInputAddDummyToInputMap(context->input, VRINPUT_6SENSOR, input_num);
			}
		} else {
			input = (vrGenericInput *)context->input->sensor6[input_num];

			/* Ideally this should never happen, but until the vrInputWaitForAllInputsToBeCreated() */
			/*   routine creates dummy inputs for devices that don't initiate, this could happen.   */
			if (input == NULL) {
				vrDbgPrintfN(AALWAYS_DBGLVL, RED_TEXT "NULL input!  Creating dummy Position Sensor %d.\n" NORM_TEXT, input_num);
				input = vrInputAddDummyToInputMap(context->input, VRINPUT_6SENSOR, input_num);
				context->input->sensor6[input_num] = (vr6sensor *)input;
			}
		}
		break;

	case	VRINPUT_NSENSOR:
		if (input_num >= context->input->num_Nsensors) {
			if (vrDbgDo(CONFIG_WARN_DBGLVL)) {
				if (context->input->num_Nsensors == 0)
					vrErrPrintf("Invalid N-Sensor requested (%d) -- none available.\n", input_num);
				else	vrErrPrintf("Invalid N-Sensor requested (%d) -- valid range is [0..%d].\n",
					input_num, context->input->num_Nsensors-1);

				/* Now Create a new dummy input to handle the request */
				vrDbgPrintfN(AALWAYS_DBGLVL, RED_TEXT "Creating dummy N-Sensor %d.\n" NORM_TEXT, input_num);
				input = vrInputAddDummyToInputMap(context->input, VRINPUT_NSENSOR, input_num);
			}
		} else {
			input = (vrGenericInput *)context->input->sensorN[input_num];

			/* Ideally this should never happen, but until the vrInputWaitForAllInputsToBeCreated() */
			/*   routine creates dummy inputs for devices that don't initiate, this could happen.   */
			if (input == NULL) {
				vrDbgPrintfN(AALWAYS_DBGLVL, RED_TEXT "NULL input!  Creating dummy N-Sensor %d.\n" NORM_TEXT, input_num);
				input = vrInputAddDummyToInputMap(context->input, VRINPUT_NSENSOR, input_num);
				context->input->sensorN[input_num] = (vrNsensor *)input;
			}
		}
		break;

	case	VRINPUT_KEYSTROKE:
	case	VRINPUT_TEXT:
	case	VRINPUT_POSITION:
	case	VRINPUT_PLANE:
	case	VRINPUT_CONTROL:
		vrErrPrintf("vrInputGetFromTypeIndex(): Unimplemented input type requested.\n");
		break;

	default:
		vrErrPrintf("vrInputGetFromTypeIndex(): Unknown input type requested.\n");
		break;
	}

	return input;
}


/*****************************************************************/
void vrFprintInputObject(FILE *file, vrInput *inobj, vrPrintStyle style)
{

	switch (style) {

	case brief:
	case one_line:
		/* Take advantage of the fact that we can at least print the value in one line */
		/* TODO: consider prepending the object name or something */
#if 0
		vrFprintf(file, "Object_type = %s (%d)\n\tid = %d\n\tname = \"%s\"\n",
			vrObjectTypeName(inobj->object_type),
			inobj->object_type,
			inobj->id,
			inobj->name);
#endif
		vrFprintInputValue(file, inobj->container.generic, style);
		break;

	case machine:
		/* TODO: not yet implemented -- output easily machine parsed */
		vrFprintf(file, "TODO: put special version of input object info here\n");
		break;

	default:
	case verbose:
		vrFprintf(file, "{\n");
		vrFprintf(file, "\r"
			"\tObject_type = %s (%d)\n\tid = %d\n\tname = \"%s\"\n"
			"\tmalleable = %d\n\tnext = %#p\n"
			"\tCreated at %s, line %d\n"
			"\tLast modified at %s, line %d\n",
			vrObjectTypeName(inobj->object_type),
			inobj->object_type,
			inobj->id,
			inobj->name,
			inobj->malleable,
			inobj->next,
			inobj->file_created,
			inobj->line_created,
			inobj->file_lastmod,
			inobj->line_lastmod);
		vrFprintf(file,
			"\r\tmap_refcount = %d\n"
			"\tdesc_str (%#p) = \"%s\"\n"
			"\tdesc_ui = \"%s\"\n"
			"\tvalue container = %#p\n",
			inobj->map_refcount,
			inobj->desc,
			inobj->desc_str,
			inobj->desc_ui,
			inobj->container.generic);
		if (inobj->r2e_xform == NULL)
			vrFprintf(file, "\r\tr2e_xform: (--)\n");
		else	vrFprintf(file, "\r\tr2e_xform: (%3.1f %3.1f %3.1f %3.1f/%3.1f %3.1f %3.1f %3.1f/%3.1f %3.1f %3.1f %3.1f/%3.1f %3.1f %3.1f %3.1f)\n",
				inobj->r2e_xform[ 0],
				inobj->r2e_xform[ 4],
				inobj->r2e_xform[ 8],
				inobj->r2e_xform[12],
				inobj->r2e_xform[ 1],
				inobj->r2e_xform[ 5],
				inobj->r2e_xform[ 9],
				inobj->r2e_xform[13],
				inobj->r2e_xform[ 2],
				inobj->r2e_xform[ 6],
				inobj->r2e_xform[10],
				inobj->r2e_xform[14],
				inobj->r2e_xform[ 3],
				inobj->r2e_xform[ 7],
				inobj->r2e_xform[11],
				inobj->r2e_xform[15]);

		vrFprintInputValue(file, inobj->container.generic, style);
		vrFprintf(file, "}\n");
		break;

	case file_format:
		vrFprintf(file, "#inputobject \"%s\" = {\n", inobj->name);

		if (!inobj->malleable)
			vrFprintf(file, "#\tmalleable = %d;\n", inobj->malleable);

		/* TODO: */

		vrFprintf(file, "}#\n\n");
		break;
	}
}


/*****************************************************************/
void vrInputDeviceClear(vrInputDevice *object)
{
	object->type = NULL;
	object->dso_file = NULL;
	object->dso_func = NULL;
	object->dso_handle = NULL;
	object->version = NULL;
	object->args = NULL;
	object->operating = 0;
	object->t2rw_xform = vrMatrixCreateIdentity();
	object->inputs = NULL;
	object->self_controls = NULL;
	object->counted = 0;
}


/*****************************************************************/
void vrInputDeviceCopy(vrContextInfo *context, vrInputDevice *dest_object, vrInputDevice *src_object)
{
	void	*dest_mem;
	void	*src_mem;
	int	memlen;

	/* copy only the memory after the generic vrObjectInfo stuff */
	dest_mem = (void *)dest_object + sizeof(vrObjectInfo);		/* 03/08/2006: "(char *)" changed to "(void *)" -- see whether this breaks anything (it seems cleaner) */
	src_mem = (void *)src_object + sizeof(vrObjectInfo);		/* 03/08/2006: ditto */
	memlen = sizeof(vrInputDevice) - sizeof(vrObjectInfo);
	memcpy(dest_mem, src_mem, memlen);

	/* make independent copy of some fields */
	dest_object->t2rw_xform = vrShmemMemDup(src_object->t2rw_xform, sizeof(vrMatrix));
	dest_object->r2e_xform = vrShmemMemDup(src_object->r2e_xform, sizeof(vrMatrix));

	dest_object->inputs = vrObjectCopyLinkedList(context, src_object->inputs);
	dest_object->self_controls = vrObjectCopyLinkedList(context, src_object->self_controls);
}


/*****************************************************************/
vrInputDevice *_MakeDummyInputDevice()
{
	vrInputDevice	*new_indev = vrShmemAlloc0(sizeof(vrInputDevice));

	vrInputDeviceClear(new_indev);
	new_indev->name = vrShmemStrDup("Dummy Input Device");	/* under normal (non-dummy) circumstances, this would be created in vrObjectNew during the configuration parsing process */

	return new_indev;
}


/*****************************************************************/
vrInput *_MakeDummyInputObject(char *function, char *description)
{
static	vrInput 	*dummy_list = NULL;		/* the linked list of all the created input objects */
	vrInput		*input_object;

	/* create a new input object definition (adding it to the dummy inputdevice's private list of input objects) */
	input_object = (vrInput *)vrObjectListNew(vrContext, VROBJECT_INPUT, (vrObjectInfo **)(&dummy_list), "unassigned input");
	input_object->line_created = __LINE__ - 1;
	snprintf(input_object->file_created, sizeof(input_object->file_created), "on the fly in %s()", function);

	input_object->desc_str = vrShmemStrDup(description);
	input_object->desc_ui[0] = '\0';
	vrDbgPrintfN(COMMON_DBGLVL, "Created new %s: '%s' (%d) -- %s\n", "input object", input_object->name, input_object->id, description);

	return (input_object);
}


/*****************************************************************/
void vrFprintInputDevice(FILE *file, vrInputDevice *device, vrPrintStyle style)
{
	vrInput		*input;
	vrMatrix	*last_r2e;	/* used for the file_format output of inputs */

	switch (style) {
	case one_line:
	case brief:
		vrFprintf(file, "id = %d, name = '%s', type = '%s', operating = %d\n",
			device->id,
			device->name,
			device->type,
			device->operating);
		break;

	default:
	case verbose:
		vrFprintf(file, "{\n"
			"\tObject_type = %s (%d)\n\tid = %d\n\tname = \"%s\"\n"
			"\tmalleable = %d\n\tnext = %#p\n"
			"\tCreated at %s, line %d\n"
			"\tLast modified at %s, line %d\n",
			vrObjectTypeName(device->object_type),
			device->object_type,
			device->id,
			device->name,
			device->malleable,
			device->next,
			device->file_created,
			device->line_created,
			device->file_lastmod,
			device->line_lastmod);
		vrFprintf(file, "\r"
			"\tproc = %#p (pid %d)\n"
			"\ttype = '%s'\n\tdso file = '%s'\n\tdso function = '%s'\n"
			"\tdso handle = %#p\n\tversion = '%s'\n\targs = '%s'\n",
			device->proc,
			(device->proc == NULL) ? -1 : device->proc->pid,
			device->type,
			(device->dso_file == NULL) ? "" : device->dso_file,
			(device->dso_func == NULL) ? "" : device->dso_func,
			device->dso_handle,
			device->version,
			device->args);
		if (device->t2rw_xform == NULL)
			vrFprintf(file, "\r\tt2rw_xform: (--)\n");
		else	vrFprintf(file, "\r"
				"\tt2rw_xform: (%3.1lf %3.1lf %3.1lf %3.1lf/%3.1lf %3.1lf %3.1lf %3.1lf/%3.1lf %3.1lf %3.1lf %3.1lf/%3.1lf %3.1lf %3.1lf %3.1lf)\n",
				device->t2rw_xform->v[ 0],
				device->t2rw_xform->v[ 4],
				device->t2rw_xform->v[ 8],
				device->t2rw_xform->v[12],
				device->t2rw_xform->v[ 1],
				device->t2rw_xform->v[ 5],
				device->t2rw_xform->v[ 9],
				device->t2rw_xform->v[13],
				device->t2rw_xform->v[ 2],
				device->t2rw_xform->v[ 6],
				device->t2rw_xform->v[10],
				device->t2rw_xform->v[14],
				device->t2rw_xform->v[ 3],
				device->t2rw_xform->v[ 7],
				device->t2rw_xform->v[11],
				device->t2rw_xform->v[15]);
		if (device->r2e_xform == NULL)
			vrFprintf(file, "\r\tr2e_xform: (--)\n");
		else	vrFprintf(file, "\r"
				"\tr2e_xform: (%3.1lf %3.1lf %3.1lf %3.1lf/%3.1lf %3.1lf %3.1lf %3.1lf/%3.1lf %3.1lf %3.1lf %3.1lf/%3.1lf %3.1lf %3.1lf %3.1lf)\n",
				device->r2e_xform->v[ 0],
				device->r2e_xform->v[ 4],
				device->r2e_xform->v[ 8],
				device->r2e_xform->v[12],
				device->r2e_xform->v[ 1],
				device->r2e_xform->v[ 5],
				device->r2e_xform->v[ 9],
				device->r2e_xform->v[13],
				device->r2e_xform->v[ 2],
				device->r2e_xform->v[ 6],
				device->r2e_xform->v[10],
				device->r2e_xform->v[14],
				device->r2e_xform->v[ 3],
				device->r2e_xform->v[ 7],
				device->r2e_xform->v[11],
				device->r2e_xform->v[15]);

		vrFprintf(file, "\r"
			"\toperating = %d\n"
			"\tinputs counted? = %d\n"
			"\tinput objects (%#p) [",
			device->operating,
			device->counted,
			device->inputs);
		for (input = device->inputs; input != NULL; input = (vrInput *)input->next)
			vrFprintf(file, " \"%s\"", input->name);

#if 0 /* Print all the input objects associated with this device */
		for (input = device->inputs; input != NULL; input = (vrInput *)input->next)
			vrFprintInputObject(file, input, style);
#endif

		vrFprintf(file, " ]\n\r"
			"\tself control input objects (%#p) [",
			device->self_controls);
		if (device->self_controls != NULL) {
			for (input = device->self_controls; input != NULL; input = (vrInput *)input->next)
				vrFprintf(file, " \"%s\"", input->name);
		}
		vrFprintf(file, " ]\n\r"
			"\tnum_6sensors = %d\n\t6sensor (data ptr) = %#p\n"
			"\tnum_Nsensors = %d\n\tNsensor (data ptr) = %#p\n"
			"\tnum_2ways = %d\n\tswitch2 (data ptr) = %#p\n"
			"\tnum_Nways = %d\n\tswitchN (data ptr) = %#p\n"
			"\tnum_valuators = %d\n\tvaluator (data ptr) = %#p\n"
			"\tnum_texts = %d\n\ttext (data ptr) = %#p\n"
			"\tnum_controls = %d\n\tcontrol (data ptr) = %#p\n",
			device->num_6sensors,	device->sensor6,
			device->num_Nsensors,	device->sensorN,
			device->num_2ways,	device->switch2,
			device->num_Nways,	device->switchN,
			device->num_valuators,	device->valuator,
			device->num_texts,	device->text,
			device->num_controls,	device->control);

		/* If device->Create exists, the rest should exist */
		if (device->Create != NULL) {
			vrFprintf(file, "\r"
				"\tCreate = %#p (%s)\n\tOpen = %#p (%s)\n\tClose = %#p (%s)\n"
				"\tReset = %#p (%s)\n\tPollData = %#p (%s)\n\tPrintAux = %#p (%s)\n",
				device->Create,		(device->Create->name != NULL ? device->Create->name : "-"),
				device->Open,		(device->Open->name != NULL ? device->Open->name : "-"),
				device->Close,		(device->Close->name != NULL ? device->Close->name : "-"),
				device->Reset,		(device->Reset->name != NULL ? device->Reset->name : "-"),
				device->PollData,	(device->PollData->name != NULL ? device->PollData->name : "-"),
				device->PrintAux,	(device->PrintAux->name != NULL ? device->PrintAux->name : "-")
				);
		}
		vrFprintf(file, "\r"
			"\taux_data = %#p\n"
			"}\n",
			device->aux_data);

		/* Now print the auxillary data, if available */
		if (device->PrintAux != NULL)
			vrCallbackInvokeDynamic(device->PrintAux, 3, file, device->aux_data, style);
		break;

	case file_format:
		vrFprintf(file, "inputdevice \"%s\" = {\n", device->name);

		if (!device->malleable)
			vrFprintf(file, "\tmalleable = %d;\n", device->malleable);
		vrFprintf(file, "\ttype = \"%s\";\n", device->type);
		if (device->args != NULL)
			vrFprintf(file, "\targs = \"%s\";\n", device->args);
		if (device->version != NULL)
			vrFprintf(file, "\t#version = \"%s\";\n", device->version);
		if (device->dso_file != NULL)
			vrFprintf(file, "\tdsofile = \"%s\";\n", device->dso_file);
		if (device->dso_func != NULL)
			vrFprintf(file, "\tdsofunc = \"%s\";\n", device->dso_file);
		vrFprintf(file, "\n");

		vrFprintf(file, "\t# WARNING: the order of the t2rw and r2e matrices is untested, and may very well be incorrect.\n");
		if (device->t2rw_xform == NULL)
			vrFprintf(file, "\t# No t2rw_transform defined.\n");
		else	vrFprintf(file, "\tt2rw_transform =%6.3f,%6.3f,%6.3f,%6.3f,  %6.3f,%6.3f,%6.3f,%6.3f,  %6.3f,%6.3f,%6.3f,%6.3f,  %6.3f,%6.3f,%6.3f,%6.3f;\n",
				device->t2rw_xform[ 0],
				device->t2rw_xform[ 4],
				device->t2rw_xform[ 8],
				device->t2rw_xform[12],
				device->t2rw_xform[ 1],
				device->t2rw_xform[ 5],
				device->t2rw_xform[ 9],
				device->t2rw_xform[13],
				device->t2rw_xform[ 2],
				device->t2rw_xform[ 6],
				device->t2rw_xform[10],
				device->t2rw_xform[14],
				device->t2rw_xform[ 3],
				device->t2rw_xform[ 7],
				device->t2rw_xform[11],
				device->t2rw_xform[15]);
		last_r2e = device->r2e_xform;
		/* NOTE: there is almost no point in printing the r2e matrix here */
		/*   because the "device" copy will almost always be the one used */
		/*   with the last "input" copy, so if there are more than one    */
		/*   r2e matrices, the first input to use it will always override */
		/*   this one, and then eventually this one will be printed again.*/
		/*   Of course, if there is only one, then it looks nicer here!   */
		if (device->r2e_xform == NULL)
			vrFprintf(file, "\t# No r2e_transform defined.\n");
		else	vrFprintf(file, "\tr2e_transform =%6.3f,%6.3f,%6.3f,%6.3f,  %6.3f,%6.3f,%6.3f,%6.3f,  %6.3f,%6.3f,%6.3f,%6.3f,  %6.3f,%6.3f,%6.3f,%6.3f;\n",
				device->r2e_xform[ 0],
				device->r2e_xform[ 4],
				device->r2e_xform[ 8],
				device->r2e_xform[12],
				device->r2e_xform[ 1],
				device->r2e_xform[ 5],
				device->r2e_xform[ 9],
				device->r2e_xform[13],
				device->r2e_xform[ 2],
				device->r2e_xform[ 6],
				device->r2e_xform[10],
				device->r2e_xform[14],
				device->r2e_xform[ 3],
				device->r2e_xform[ 7],
				device->r2e_xform[11],
				device->r2e_xform[15]);
		vrFprintf(file, "\n");

		for (input = device->inputs; input != NULL; input = (vrInput *)input->next) {
			/* Print r2e_xform for each input that uses it, and no   */
			/*   more.  Or maybe each time it's changed from before. */
			/* By storing the last printed r2e matrix, we only need */
			/*   to print it again when it changes for a new input. */
#if 0
			vrFprintf(file, "\t# r2e matrix is %p, last is %p, compare is %d\n",
				input->r2e_xform,
				last_r2e,
				vrMatrixEqual(last_r2e, input->r2e_xform));
#endif
			if (!vrMatrixEqual(last_r2e, input->r2e_xform) && input->r2e_xform != NULL) {
				vrFprintf(file, "\tr2e_transform =%6.3f,%6.3f,%6.3f,%6.3f,  %6.3f,%6.3f,%6.3f,%6.3f,  %6.3f,%6.3f,%6.3f,%6.3f,  %6.3f,%6.3f,%6.3f,%6.3f;\n",
					input->r2e_xform[ 0],
					input->r2e_xform[ 4],
					input->r2e_xform[ 8],
					input->r2e_xform[12],
					input->r2e_xform[ 1],
					input->r2e_xform[ 5],
					input->r2e_xform[ 9],
					input->r2e_xform[13],
					input->r2e_xform[ 2],
					input->r2e_xform[ 6],
					input->r2e_xform[10],
					input->r2e_xform[14],
					input->r2e_xform[ 3],
					input->r2e_xform[ 7],
					input->r2e_xform[11],
					input->r2e_xform[15]);
				last_r2e = input->r2e_xform;
			}
			vrFprintf(file, "\tinput \"%s\" = \"%s\";\n", input->name, input->desc_str);
		}
		vrFprintf(file, "\n");

		for (input = device->self_controls; input != NULL; input = (vrInput *)input->next) {
			vrFprintf(file, "\tcontrol \"%s\" = \"%s\";\n", input->name, input->desc_str);
		}

		vrFprintf(file, "}\n\n");
		break;
	}
}


/*****************************************************************/
/* create the structures in vrInputs that applications refer to */
/*   for input values (ie. button1, button2, valuator1, etc.). */
/* -- this replaces tmpConfigMapInputs() in config.c --        */
/*                                                        */
/* NOTE: it is important to note that this routine is     */
/*   called in the main process, not the input process.   */
/*   This is done after all the input devices have        */
/*   reported an operating-status.                        */
void vrCreateInputMap(vrInputInfo *vrInputs, char *mapname)
{
	int	count;
	int	devcount;
	int	num_2ways_assigned;
	int	num_Nways_assigned;
	int	num_valuators_assigned;
	int	num_6sensors_assigned;
	int	num_Nsensors_assigned;
	int	num_controls_assigned;

	vrTrace("vrCreateInputMap():", BOLD_TEXT "entering." NORM_TEXT);
	if (vrInputs->num_input_devices == 0) {
		vrErrPrintf("vrCreateInputMap(): " RED_TEXT "Warning, no input devices have been defined!\n" NORM_TEXT);
		return;
	}

	if (mapname == NULL) {
		vrErrPrintf("vrCreateInputMap(): " RED_TEXT "Warning, NULL InputMap string!\n" NORM_TEXT);
		return;
	}

	if (!strcasecmp(mapname, "olddefault")) {

		/* TODO: this hardwired monstrosity obviously needs to go away. */
		/*   That'll probably have to wait until most of the rest of    */
		/*   the config file parsing stuff is done, though.             */
		/* [10/13/99] -- actually, go away may be a bit strong.  It     */
		/*   might be wise to keep a "default" input map, but it        */
		/*   probably should do something like loop over all the input  */
		/*   devices and map all their inputs (in order) to the list of */
		/*   inputs in the configuration.  Then, make sure there is     */
		/*   some appropriate minimal number of inputs for each type,   */
		/*   and add dummy inputs where necessary.                      */

		vrDbgPrintfN(SELDOM_DBGLVL, "vrInputs data structure is at %#p w/ %d devices\n",
			vrInputs, vrInputs->num_input_devices);

		vrInputs->num_2ways += vrInputs->input_devices[0]->num_2ways;
		vrDbgPrintfN(SELDOM_DBGLVL, "vrInputs->num_2ways = %d\n", vrInputs->num_2ways);
		if (vrInputs->switch2 != NULL)
			vrShmemFree(vrInputs->switch2);
		if (vrInputs->input_devices[0]->num_2ways >= 3) {
			vrInputs->switch2 = (vr2switch **)vrShmemAlloc0(vrInputs->num_2ways * sizeof(vr2switch *));
			vrInputs->switch2[0] = &(vrInputs->input_devices[0]->switch2[0]);
			vrInputs->switch2[1] = &(vrInputs->input_devices[0]->switch2[1]);
			vrInputs->switch2[2] = &(vrInputs->input_devices[0]->switch2[2]);
		}

		vrInputs->num_Nways += vrInputs->input_devices[0]->num_Nways;
		vrDbgPrintfN(SELDOM_DBGLVL, "vrInputs->num_Nways = %d\n", vrInputs->num_Nways);
		if (vrInputs->switchN != NULL)
			vrShmemFree(vrInputs->switchN);
		if (vrInputs->input_devices[0]->num_Nways >= 2) {
			vrInputs->switchN = (vrNswitch **)vrShmemAlloc0(vrInputs->num_Nways * sizeof(vrNswitch *));
			vrInputs->switchN[0] = &(vrInputs->input_devices[0]->switchN[0]);
			vrInputs->switchN[1] = &(vrInputs->input_devices[0]->switchN[1]);
			vrInputs->switchN[2] = &(vrInputs->input_devices[0]->switchN[2]);
		}

		vrInputs->num_valuators += vrInputs->input_devices[0]->num_valuators;
		vrDbgPrintfN(SELDOM_DBGLVL, "vrInputs->num_valuators = %d\n", vrInputs->num_valuators);
		if (vrInputs->valuator != NULL)
			vrShmemFree(vrInputs->valuator);
		if (vrInputs->input_devices[0]->num_valuators >= 2) {
			vrInputs->valuator = (vrValuator **)vrShmemAlloc0(vrInputs->num_valuators * sizeof(vrValuator *));
			vrInputs->valuator[0] = &(vrInputs->input_devices[0]->valuator[0]);
			vrInputs->valuator[1] = &(vrInputs->input_devices[0]->valuator[1]);
			/* magellan & spaceorb have a third valuator (unused for now) */
			if (vrInputs->num_valuators >= 3)
				vrInputs->valuator[2] = &(vrInputs->input_devices[0]->valuator[2]);
		}

		vrInputs->num_6sensors += vrInputs->input_devices[0]->num_6sensors;
		vrDbgPrintfN(SELDOM_DBGLVL, "vrInputs->num_6sensors = %d\n", vrInputs->num_6sensors);
		if (vrInputs->sensor6 != NULL)
			vrShmemFree(vrInputs->sensor6);
		if (vrInputs->input_devices[0]->num_6sensors >= 2) {
			vrInputs->sensor6 = (vr6sensor **)vrShmemAlloc0(vrInputs->num_6sensors * sizeof(vr6sensor *));
			vrInputs->sensor6[0] = &(vrInputs->input_devices[0]->sensor6[0]);
			vrInputs->sensor6[1] = &(vrInputs->input_devices[0]->sensor6[1]);
			vrDbgPrintfN(SELDOM_DBGLVL, "vrInputs->sensor6[0] is at %#p -> %#p\n",
				&vrInputs->sensor6[0], vrInputs->sensor6[0]);
			vrDbgPrintfN(SELDOM_DBGLVL, "vrInputs->sensor6[1] is at %#p -> %#p\n",
				&vrInputs->sensor6[1], vrInputs->sensor6[1]);
			/* magellan has a third sensor6 (unused for now) */
			if (vrInputs->num_6sensors >= 3) {
				vrInputs->sensor6[2] = &(vrInputs->input_devices[0]->sensor6[2]);
				vrDbgPrintfN(SELDOM_DBGLVL, "vrInputs->sensor6[2] is at %#p -> %#p\n",
					&vrInputs->sensor6[2], vrInputs->sensor6[2]);
			}
		}

		/* assign the user's head to sensor6 0 */
		vrInputs->users[0]->head = vrInputs->sensor6[0];


	} else if (!strcasecmp(mapname, "default")) {

		/* This version of default, simply puts all the available inputs */
		/*   into the map, in a first found, first added ordering.       */

		vrDbgPrintfN(SELDOM_DBGLVL, "vrInputs data structure is at %#p w/ %d devices\n",
			vrInputs, vrInputs->num_input_devices);

		/*************************************************************/
		/* first, we need to count all the different types of inputs */
		/*   from all the devices.                                   */
		vrInputs->num_2ways = 0;
		vrInputs->num_Nways = 0;
		vrInputs->num_valuators = 0;
		vrInputs->num_6sensors = 0;
		vrInputs->num_Nsensors = 0;
		vrInputs->num_controls = 0;
		for (devcount = 0; devcount < vrInputs->num_input_devices; devcount++) {
			vrInputs->num_2ways += vrInputs->input_devices[devcount]->num_2ways;
			vrInputs->num_Nways += vrInputs->input_devices[devcount]->num_Nways;
			vrInputs->num_valuators += vrInputs->input_devices[devcount]->num_valuators;
			vrInputs->num_6sensors += vrInputs->input_devices[devcount]->num_6sensors;
			vrInputs->num_Nsensors += vrInputs->input_devices[devcount]->num_Nsensors;
			vrInputs->num_controls += vrInputs->input_devices[devcount]->num_controls;
		}

		/***************************************************************/
		/* Now allocate the memory for the array of each type of input */

		/*** VRINPUT_BINARY ***/
		if (vrInputs->switch2 != NULL)
			vrShmemFree(vrInputs->switch2);
		vrInputs->switch2 = (vr2switch **)vrShmemAlloc0(vrInputs->num_2ways * sizeof(vr2switch *));
		num_2ways_assigned = 0;

		/*** VRINPUT_NWAY ***/
		if (vrInputs->switchN != NULL)
			vrShmemFree(vrInputs->switchN);
		vrInputs->switchN = (vrNswitch **)vrShmemAlloc0(vrInputs->num_Nways * sizeof(vrNswitch *));
		num_Nways_assigned = 0;

		/*** VRINPUT_VALUATOR ***/
		if (vrInputs->valuator != NULL)
			vrShmemFree(vrInputs->valuator);
		vrInputs->valuator = (vrValuator **)vrShmemAlloc0(vrInputs->num_valuators * sizeof(vrValuator *));
		num_valuators_assigned = 0;

		/*** VRINPUT_6SENSOR ***/
		if (vrInputs->sensor6 != NULL)
			vrShmemFree(vrInputs->sensor6);
		vrInputs->sensor6 = (vr6sensor **)vrShmemAlloc0(vrInputs->num_6sensors * sizeof(vr6sensor *));
		num_6sensors_assigned = 0;

		/*** VRINPUT_NSENSOR ***/
		if (vrInputs->sensorN != NULL)
			vrShmemFree(vrInputs->sensorN);
		vrInputs->sensorN = (vrNsensor **)vrShmemAlloc0(vrInputs->num_Nsensors * sizeof(vrNsensor *));
		num_Nsensors_assigned = 0;

		/*** VRINPUT_CONTROL ***/
		if (vrInputs->control != NULL)
			vrShmemFree(vrInputs->control);
		vrInputs->control = (vrControl **)vrShmemAlloc0(vrInputs->num_controls * sizeof(vrControl *));
		num_controls_assigned = 0;


		/***************************************************************************/
		/* Now, go through all the devices and add their inputs to the global list */
		for (devcount = 0; devcount < vrInputs->num_input_devices; devcount++) {

			/*** VRINPUT_BINARY ***/
			for (count = 0; count < vrInputs->input_devices[devcount]->num_2ways; count++) {
				vrInputs->switch2[num_2ways_assigned] = &(vrInputs->input_devices[devcount]->switch2[count]);
				vrInputs->switch2[num_2ways_assigned]->my_object->map_refcount++;
				num_2ways_assigned++;
			}

			/*** VRINPUT_NWAY ***/
			for (count = 0; count < vrInputs->input_devices[devcount]->num_Nways; count++) {
				vrInputs->switchN[num_Nways_assigned] = &(vrInputs->input_devices[devcount]->switchN[count]);
				vrInputs->switchN[num_Nways_assigned]->my_object->map_refcount++;
				num_Nways_assigned++;
			}

			/*** VRINPUT_VALUATOR ***/
			for (count = 0; count < vrInputs->input_devices[devcount]->num_valuators; count++) {
				vrInputs->valuator[num_valuators_assigned] = &(vrInputs->input_devices[devcount]->valuator[count]);
				vrInputs->valuator[num_valuators_assigned]->my_object->map_refcount++;
				num_valuators_assigned++;
			}

			/*** VRINPUT_6SENSOR ***/
			for (count = 0; count < vrInputs->input_devices[devcount]->num_6sensors; count++) {
				vrInputs->sensor6[num_6sensors_assigned] = &(vrInputs->input_devices[devcount]->sensor6[count]);
				vrInputs->sensor6[num_6sensors_assigned]->my_object->map_refcount++;
				num_6sensors_assigned++;
			}

			/*** VRINPUT_NSENSOR ***/
			for (count = 0; count < vrInputs->input_devices[devcount]->num_Nsensors; count++) {
				vrInputs->sensorN[num_Nsensors_assigned] = &(vrInputs->input_devices[devcount]->sensorN[count]);
				vrInputs->sensorN[num_Nsensors_assigned]->my_object->map_refcount++;
				num_Nsensors_assigned++;
			}

			/*** VRINPUT_CONTROL ***/
			for (count = 0; count < vrInputs->input_devices[devcount]->num_controls; count++) {
				vrInputs->control[num_controls_assigned] = &(vrInputs->input_devices[devcount]->control[count]);
				vrInputs->control[num_controls_assigned]->my_object->map_refcount++;
				num_controls_assigned++;
			}
		}


	} else if (!strcasecmp(mapname, "xtest")) {

		/* TODO: okay, this hardwired monstrosity obviously does need to go away. */

		vrDbgPrintfN(SELDOM_DBGLVL, "vrInputs data structure is at %#p w/ %d devices\n",
			vrInputs, vrInputs->num_input_devices);

		vrInputs->num_2ways += vrInputs->input_devices[0]->num_2ways;
		vrDbgPrintfN(SELDOM_DBGLVL, "vrInputs->num_2ways = %d\n", vrInputs->num_2ways);
		if (vrInputs->switch2 != NULL)
			vrShmemFree(vrInputs->switch2);
		if (vrInputs->input_devices[0]->num_2ways >= 3) {
			vrInputs->switch2 = (vr2switch **)vrShmemAlloc0(vrInputs->num_2ways+1 * sizeof(vr2switch *));
			vrInputs->switch2[1] = &(vrInputs->input_devices[0]->switch2[0]);
			vrInputs->switch2[2] = &(vrInputs->input_devices[0]->switch2[1]);
			vrInputs->switch2[3] = &(vrInputs->input_devices[0]->switch2[2]);
		}

		/***** NEW stuff for testing X windows *****/
		/* we assume that the X device is the second input device */
		if (vrInputs->input_devices[1]->num_2ways > 0) {
			/* Notice I allocated an extra slot above */
			/* Also, I made this one the first one, just so it would */
			/*   be easier to see in the on-screen text output.      */
			vrInputs->switch2[0] = &(vrInputs->input_devices[1]->switch2[0]);
vrPrintf("Okay, xwin device is button[0]\n");
		} else {
			vrErr("YO: for some reason there wasn't a device\n");
			vrInputs->switch2[0] = &(vrInputs->input_devices[0]->switch2[0]);
		}

		vrInputs->num_Nways += vrInputs->input_devices[0]->num_Nways;
		vrDbgPrintfN(SELDOM_DBGLVL, "vrInputs->num_Nways = %d\n", vrInputs->num_Nways);
		if (vrInputs->switchN != NULL)
			vrShmemFree(vrInputs->switchN);
		if (vrInputs->input_devices[0]->num_Nways >= 2) {
			vrInputs->switchN = (vrNswitch **)vrShmemAlloc0(vrInputs->num_Nways * sizeof(vrNswitch *));
			vrInputs->switchN[0] = &(vrInputs->input_devices[0]->switchN[0]);
			vrInputs->switchN[1] = &(vrInputs->input_devices[0]->switchN[1]);
			vrInputs->switchN[2] = &(vrInputs->input_devices[0]->switchN[2]);
		}

		vrInputs->num_valuators += vrInputs->input_devices[0]->num_valuators;
		vrDbgPrintfN(SELDOM_DBGLVL, "vrInputs->num_valuators = %d\n", vrInputs->num_valuators);
		if (vrInputs->valuator != NULL)
			vrShmemFree(vrInputs->valuator);
		if (vrInputs->input_devices[0]->num_valuators >= 2) {
			vrInputs->valuator = (vrValuator **)vrShmemAlloc0(vrInputs->num_valuators * sizeof(vrValuator *));
			vrInputs->valuator[0] = &(vrInputs->input_devices[0]->valuator[0]);
			vrInputs->valuator[1] = &(vrInputs->input_devices[0]->valuator[1]);
			/* magellan & spaceorb have a third valuator (unused for now) */
			if (vrInputs->num_valuators >= 3)
				vrInputs->valuator[2] = &(vrInputs->input_devices[0]->valuator[2]);
		}

		vrInputs->num_6sensors += vrInputs->input_devices[0]->num_6sensors;
		vrDbgPrintfN(SELDOM_DBGLVL, "vrInputs->num_6sensors = %d\n", vrInputs->num_6sensors);
		if (vrInputs->sensor6 != NULL)
			vrShmemFree(vrInputs->sensor6);
		if (vrInputs->input_devices[0]->num_6sensors >= 2) {
			vrInputs->sensor6 = (vr6sensor **)vrShmemAlloc0(vrInputs->num_6sensors * sizeof(vr6sensor *));
			vrInputs->sensor6[0] = &(vrInputs->input_devices[0]->sensor6[0]);
			vrInputs->sensor6[1] = &(vrInputs->input_devices[0]->sensor6[1]);
			vrDbgPrintfN(SELDOM_DBGLVL, "vrInputs->sensor6[0] is at %#p -> %#p\n",
				&vrInputs->sensor6[0], vrInputs->sensor6[0]);
			vrDbgPrintfN(SELDOM_DBGLVL, "vrInputs->sensor6[1] is at %#p -> %#p\n",
				&vrInputs->sensor6[1], vrInputs->sensor6[1]);
			/* magellan has a third sensor6 (unused for now) */
			if (vrInputs->num_6sensors >= 3) {
				vrInputs->sensor6[2] = &(vrInputs->input_devices[0]->sensor6[2]);
				vrDbgPrintfN(SELDOM_DBGLVL, "vrInputs->sensor6[2] is at %#p -> %#p\n",
					&vrInputs->sensor6[2], vrInputs->sensor6[2]);
			}
		}

		/* assign the user's head to sensor6 0 */
		vrInputs->users[0]->head = vrInputs->sensor6[0];


	} else {
		vrErrPrintf("vrCreateInputMap(): " RED_TEXT "input map other than 'default' specified -- don't know what to do!\n" NORM_TEXT);
	}

	vrTrace("vrCreateInputMap():", BOLD_TEXT "exiting." NORM_TEXT);
}


/***********************************************************************************/
/* vrInputAddDummyToInputMap(): The intent of this function is to allow the system */
/*   the ability to add new inputs that the application program has requested, but */
/*   that don't exist in the configuration.  This way at least things can function */
/*   in a limited fashion.                                                         */
/*                                                                                 */
/* TODO: I kind of put this together in a hurry, so it probably could use some     */
/*   rethinking and redesign.  [05/24/2006]                                        */
vrGenericInput *vrInputAddDummyToInputMap(vrInputInfo *inputs, vrInputType type, int input_num)
{
	vrGenericInput	*input = NULL;
	vrGenericInput	**old_list;
	int		count;
	vrInput		*input_object;		/* a dummy input object */
#if 0
static	vrInput 	*dummy_list;		/* the linked list of all the created input objects */
#endif
static	char		buffer[512];		/* a place to sprintf into */

	switch (type) {
	case VRINPUT_BINARY:
		if (input_num >= inputs->num_2ways) {
			/* If we get here, then we need to extend the number of 2-way inputs */
			/* NOTE: if this condition isn't true, then we've previously extended the */
			/*   number of inputs, leaving some holes to be filled.                   */
			old_list = (vrGenericInput **)inputs->switch2;
			inputs->switch2 = (vr2switch **)vrShmemAlloc0((input_num+1) * sizeof(vr2switch *));
			for (count = 0; count < inputs->num_2ways; count++) {
				inputs->switch2[count] = ((vr2switch **)old_list)[count];
			}
			inputs->num_2ways = input_num + 1;
		}

		if (inputs->switch2[input_num] != NULL) {
			/* If we get here, there is an error in the library! */
			vrDbgPrintfN(ALWAYS_DBGLVL, RED_TEXT "vrInputAddDummyToInputMap(): Library Error: Created a new input that is non-NULL!" NORM_TEXT);
		}

		/* make the dummy input data container */
		input = vrInputCreateDataContainerArrayOfType(type, 1, _MakeDummyInputDevice());
		inputs->switch2[input_num] = (vr2switch *)input;

		snprintf(buffer, sizeof(buffer), "switch2(dummy[%d])", input_num);

		break;

	case VRINPUT_NWAY:
		if (input_num >= inputs->num_Nways) {
			/* If we get here, then we need to extend the number of N-way inputs */
			/* NOTE: if this condition isn't true, then we've previously extended the */
			/*   number of inputs, leaving some holes to be filled.                   */
			old_list = (vrGenericInput **)inputs->switchN;
			inputs->switchN = (vrNswitch **)vrShmemAlloc0((input_num+1) * sizeof(vrNswitch *));
			for (count = 0; count < inputs->num_Nways; count++) {
				inputs->switchN[count] = ((vrNswitch **)old_list)[count];
			}
			inputs->num_Nways = input_num + 1;
		}

		if (inputs->switchN[input_num] != NULL) {
			/* If we get here, there is an error in the library! */
			vrDbgPrintfN(ALWAYS_DBGLVL, RED_TEXT "vrInputAddDummyToInputMap(): Library Error: Created a new input that is non-NULL!" NORM_TEXT);
		}

		/* make the dummy input data container */
		input = vrInputCreateDataContainerArrayOfType(type, 1, _MakeDummyInputDevice());
		inputs->switchN[input_num] = (vrNswitch *)input;

		snprintf(buffer, sizeof(buffer), "switchN(dummy[%d])", input_num);

		break;

	case VRINPUT_VALUATOR:
		if (input_num >= inputs->num_valuators) {
			/* If we get here, then we need to extend the number of valuator inputs */
			/* NOTE: if this condition isn't true, then we've previously extended the */
			/*   number of inputs, leaving some holes to be filled.                   */
			old_list = (vrGenericInput **)inputs->valuator;
			inputs->valuator = (vrValuator **)vrShmemAlloc0((input_num+1) * sizeof(vrValuator *));
			for (count = 0; count < inputs->num_valuators; count++) {
				inputs->valuator[count] = ((vrValuator **)old_list)[count];
			}
			inputs->num_valuators = input_num + 1;
		}

		if (inputs->valuator[input_num] != NULL) {
			/* If we get here, there is an error in the library! */
			vrDbgPrintfN(ALWAYS_DBGLVL, RED_TEXT "vrInputAddDummyToInputMap(): Library Error: Created a new input that is non-NULL!" NORM_TEXT);
		}

		/* make the dummy input data container */
		input = vrInputCreateDataContainerArrayOfType(type, 1, _MakeDummyInputDevice());
		inputs->valuator[input_num] = (vrValuator *)input;

		snprintf(buffer, sizeof(buffer), "valuator(dummy[%d])", input_num);

		break;

	case VRINPUT_6SENSOR:
		if (input_num >= inputs->num_6sensors) {
			/* If we get here, then we need to extend the number of 6-sensor inputs   */
			/* NOTE: if this condition isn't true, then we've previously extended the */
			/*   number of inputs, leaving some holes to be filled.                   */
			old_list = (vrGenericInput **)inputs->sensor6;
			inputs->sensor6 = (vr6sensor **)vrShmemAlloc0((input_num+1) * sizeof(vr6sensor *));
			for (count = 0; count < inputs->num_6sensors; count++) {
				inputs->sensor6[count] = ((vr6sensor **)old_list)[count];
			}
			inputs->num_6sensors = input_num + 1;
		}

		if (inputs->sensor6[input_num] != NULL) {
			/* If we get here, there is an error in the library! */
			vrDbgPrintfN(ALWAYS_DBGLVL, RED_TEXT "vrInputAddDummyToInputMap(): Library Error: Created a new input that is non-NULL!" NORM_TEXT);
		}

		/* make the dummy input data container */
		input = vrInputCreateDataContainerArrayOfType(type, 1, _MakeDummyInputDevice());
		inputs->sensor6[input_num] = (vr6sensor *)input;

		snprintf(buffer, sizeof(buffer), "sensor6(dummy[%d])", input_num);

		break;

	case VRINPUT_NSENSOR:
		if (input_num >= inputs->num_Nsensors) {
			/* If we get here, then we need to extend the number of N-sensor inputs   */
			/* NOTE: if this condition isn't true, then we've previously extended the */
			/*   number of inputs, leaving some holes to be filled.                   */
			old_list = (vrGenericInput **)inputs->sensorN;
			inputs->sensorN = (vrNsensor **)vrShmemAlloc0((input_num+1) * sizeof(vrNsensor *));
			for (count = 0; count < inputs->num_Nsensors; count++) {
				inputs->sensorN[count] = ((vrNsensor **)old_list)[count];
			}
			inputs->num_Nsensors = input_num + 1;
		}

		if (inputs->sensorN[input_num] != NULL) {
			/* If we get here, there is an error in the library! */
			vrDbgPrintfN(ALWAYS_DBGLVL, RED_TEXT "vrInputAddDummyToInputMap(): Library Error: Created a new input that is non-NULL!" NORM_TEXT);
		}

		/* make the dummy input data container */
		input = vrInputCreateDataContainerArrayOfType(type, 1, _MakeDummyInputDevice());
		inputs->sensorN[input_num] = (vrNsensor *)input;

		snprintf(buffer, sizeof(buffer), "sensorN(dummy[%d])", input_num);

		break;

	default:
		/* Not implemented yet */
		break;
	}

	input->my_object = _MakeDummyInputObject("vrInputAddDummyToInputMap", buffer);
	input->my_object->container.generic = input;
	input->dummy = 1;			/* flag this as being a dummy input */

	return input;
}


/***************************************************************/
/* vrInputInitialize(): initialize the vrContext->input data structure */
/***************************************************************/
/* setup the vrContext->input structure that the AP will typically */
/* use to access input data.                               */
/*                                                         */
/* NOTE: it is important to note that this routine is      */
/*   called in the main process, not the input process.    */
/*   This is done after all the input devices have         */
/*   reported an operating-status.                         */
/**************************************************************/
void vrInputInitialize(vrContextInfo *context)
{
	vrInputInfo	*vrInputs = context->input;
	vrConfigInfo	*vrConfig = context->config;

	vrTrace("vrInputInitialize():", BOLD_TEXT "entering." NORM_TEXT);

	/* NOTE: I had vrContext->input allocated here, but that needs     */
	/*   to happen before the processes are forked in vrStart.  */

	vrInputs->users = vrConfig->users;
	vrInputs->num_users = vrConfig->num_users;
	if (vrInputs->num_users > 0)
		vrDbgPrintfN(SELDOM_DBGLVL, "vrInputs->num_users = %d, vrInputs->user[0] at %#p\n", vrInputs->num_users, vrInputs->users[0]);
	else	vrDbgPrintfN(SELDOM_DBGLVL, "vrInputs->num_users = %d\n", vrInputs->num_users);

	vrInputs->props = vrConfig->props;
	vrInputs->num_props = vrConfig->num_props;

	vrInputs->input_devices = vrConfig->input_devices;
	vrInputs->num_input_devices = vrConfig->num_input_devices;

	/* Map all the physical inputs and fixed values to the arrays */
	/*   of inputs referred to in the applications here.          */
	vrInputs->input_map_name = vrConfig->input_map_name;
	vrCreateInputMap(vrInputs, vrInputs->input_map_name);

	vrConfig->inputs_init = 1;

	vrTrace("vrInputInitialize():", BOLD_TEXT "exiting." NORM_TEXT);
	return;
}


/***********************************************************************/
int vrInputsInitialized(vrContextInfo *context)
{
	return context->config->inputs_init;
}


/******************************************************************************/
/* vrInputCreateDataContainerArrayOfType(): makes the memory arrangements for */
/*   adding an array of input data containers of the given type to the global */
/*   structure, and returns a pointer to the memory where the first input of  */
/*   that particular type is stored.                                          */
/* Eg. A magellan will request to add 3 2-way input containers.               */
/******************************************************************************/
void *vrInputCreateDataContainerArrayOfType(vrInputType type, int num, vrInputDevice *indev)
{
static	char		trace_msg[256];			/* space to create a string for vrTrace */
	int		count;				/* loop counter */
	vrGenericInput	*newinput;
	char		*memptr;
	float		*float_array;			/* memory for allocating Nsensor memory */

	vrTrace("vrInputCreateDataContainerArrayOfType", "beginning");

	/* TODO: this entire section (or each case separately) MUST be    */
	/*   mutex locked so only one change (per case) is being modified */
	/*   at any instance.  (We're safe with just one input proc.)     */
	/* Hmmm, now (12/16/98) I'm not so sure that we need to lock this code. */
	switch (type) {

	/******************/
	case VRINPUT_BINARY:	/* equivalent to VRINPUT_2WAY */
		sprintf(trace_msg, "  creating %d 2-switches", num);
		vrTrace("vrInputCreateDataContainerArrayOfType", trace_msg);
		memptr = (char *)vrShmemAlloc0(num * sizeof(vr2switch));
		for (count = 0; count < num; count++) {
			newinput = (vrGenericInput *)(memptr + sizeof(vr2switch) * count);
			newinput->object_type = VROBJECT_INPUTDATA;
			newinput->input_type = VRINPUT_BINARY;
			newinput->my_device = indev;
			newinput->queue_me = 0;
			newinput->dummy = 0;
			newinput->lock = vrLockCreateName(vrContext, "binary input");
			newinput->my_object = _MakeDummyInputObject("vrInputCreateDataContainerArrayOfType", "dummy 2-switch"); /* assign a dummy object for now -- the real object is assigned later, after vrInputCreateDataContainerArrayOfType() completes and returns back to vrInputCreateDataContainers(). */
			newinput->my_object->container.generic = newinput;

			/* allocate the historical measurement array and initialize */
			((vr2switch *)newinput)->num_measures = 50;
			((vr2switch *)newinput)->current_measure = 0;
			((vr2switch *)newinput)->measures = vrShmemAlloc0(((vr2switch *)newinput)->num_measures * sizeof(int));

			/* make the initial assignment, and set last_value the same */
			vrAssign2switchValue((vr2switch *)(newinput), 0 /* , vrTime time */);
			((vr2switch *)newinput)->last_value = ((vr2switch *)newinput)->value;

			newinput->checksum = vrInputMakeChecksum(newinput);

			if (vrDbgDo(INPUT_DBGLVL)) {
				vrPrintf("Created: ");
				vrFprintInputValue(stdout, newinput, verbose);
			}
		}
		break;

	/****************/
	case VRINPUT_NWAY:	/* equivalent to VRINPUT_NARY */
		sprintf(trace_msg, "  creating %d N-switches", num);
		vrTrace("vrInputCreateDataContainerArrayOfType", trace_msg);
		memptr = (char *)vrShmemAlloc0(num * sizeof(vrNswitch));
		for (count = 0; count < num; count++) {
			newinput = (vrGenericInput *)(memptr + sizeof(vrNswitch) * count);
			newinput->object_type = VROBJECT_INPUTDATA;
			newinput->input_type = VRINPUT_NWAY;
			newinput->my_device = indev;
			newinput->queue_me = 0;
			newinput->dummy = 0;
			newinput->lock = vrLockCreateName(vrContext, "n-ary input");

			/* make the initial assignment, and set last_value the same */
			vrAssignNswitchValue((vrNswitch *)(newinput), 0 /* , vrTime time */);
			((vrNswitch *)newinput)->last_value = ((vrNswitch *)newinput)->value;

			newinput->checksum = vrInputMakeChecksum(newinput);

			if (vrDbgDo(INPUT_DBGLVL)) {
				vrPrintf("Created: ");
				vrFprintInputValue(stdout, newinput, verbose);
			}
		}
		break;

	/*******************/
	case VRINPUT_VALUATOR:
		sprintf(trace_msg, "  creating %d Valuators", num);
		vrTrace("vrInputCreateDataContainerArrayOfType", trace_msg);
		memptr = (char *)vrShmemAlloc0(num * sizeof(vrValuator));
		for (count = 0; count < num; count++) {
			newinput = (vrGenericInput *)(memptr + sizeof(vrValuator) * count);
			newinput->object_type = VROBJECT_INPUTDATA;
			newinput->input_type = VRINPUT_VALUATOR;
			newinput->my_device = indev;
			newinput->queue_me = 0;
			newinput->dummy = 0;
			newinput->lock = vrLockCreateName(vrContext, "valuator input");
			newinput->my_object = _MakeDummyInputObject("vrInputCreateDataContainerArrayOfType", "dummy valuator"); /* assign a dummy object for now -- the real object is assigned later, after vrInputCreateDataContainerArrayOfType() completes and returns back to vrInputCreateDataContainers(). */
			newinput->my_object->container.generic = newinput;

			/* allocate the historical measurement array and initialize */
			((vrValuator *)newinput)->num_measures = 50;
			((vrValuator *)newinput)->current_measure = 0;
			((vrValuator *)newinput)->measures = vrShmemAlloc0(((vrValuator *)newinput)->num_measures * sizeof(float));

			/* make the initial assignment, and set last_value the same */
			vrAssignValuatorValue((vrValuator *)(newinput), 0.0 /* , vrTime time */);
			((vrValuator *)newinput)->last_value = ((vrValuator *)newinput)->value;

			newinput->checksum = vrInputMakeChecksum(newinput);

			if (vrDbgDo(INPUT_DBGLVL)) {
				vrPrintf("Created: ");
				vrFprintInputValue(stdout, newinput, verbose);
			}
		}
		break;

	/*******************/
	case VRINPUT_6SENSOR:
		sprintf(trace_msg, "  creating %d 6-sensors", num);
		vrTrace("vrInputCreateDataContainerArrayOfType", trace_msg);
		memptr = (char *)vrShmemAlloc0(num * sizeof(vr6sensor));
		for (count = 0; count < num; count++) {
			newinput = (vrGenericInput *)(memptr + sizeof(vr6sensor) * count);
			newinput->object_type = VROBJECT_INPUTDATA;
			newinput->input_type = VRINPUT_6SENSOR;
			newinput->my_device = indev;
			newinput->queue_me = 0;
			newinput->dummy = 0;
			newinput->lock = vrLockCreateName(vrContext, "6sensor input");
#if defined(LOCK_DBG) || 0
vrPrintf("Just created lock %p for sensor6 %p\n", newinput->lock, newinput);
#endif

			((vr6sensor *)newinput)->dof = 9;
			((vr6sensor *)newinput)->raw_data = vrMatrixCreateIdentity();
			((vr6sensor *)newinput)->r2e_xform = vrMatrixCreateIdentity();
			((vr6sensor *)newinput)->position = vrMatrixCreateIdentity();
			((vr6sensor *)newinput)->last_position = vrMatrixCreateIdentity();
			((vr6sensor *)newinput)->visren_position = vrMatrixCreateIdentity();
			if (indev == NULL)
				((vr6sensor *)newinput)->t2rw_xform = vrMatrixCreateIdentity();
			else	((vr6sensor *)newinput)->t2rw_xform = indev->t2rw_xform;

			/* make the initial assignment, and set last_value the same */
			vrAssign6sensorValue((vr6sensor *)newinput, vrMatrixCreateIdentity(), 0 /* , vrTime time */);
			vrMatrixCopy(((vr6sensor *)newinput)->last_position, ((vr6sensor *)newinput)->position);

			newinput->checksum = vrInputMakeChecksum(newinput);

			if (vrDbgDo(INPUT_DBGLVL)) {
				vrPrintf("Created: ");
				vrFprintInputValue(stdout, newinput, verbose);
			}
		}
		break;

	/*******************/
	case VRINPUT_NSENSOR:
		sprintf(trace_msg, "  creating %d N-sensors", num);
		vrTrace("vrInputCreateDataContainerArrayOfType", trace_msg);
		memptr = (char *)vrShmemAlloc0(num * sizeof(vrNsensor));
		for (count = 0; count < num; count++) {
			newinput = (vrGenericInput *)(memptr + sizeof(vrNsensor) * count);
			newinput->object_type = VROBJECT_INPUTDATA;
			newinput->input_type = VRINPUT_NSENSOR;
			newinput->my_device = indev;
			newinput->queue_me = 0;
			newinput->dummy = 0;
			newinput->lock = vrLockCreateName(vrContext, "n-sensor input");

			((vrNsensor *)newinput)->dof = MAX_NSENSOR_VALUES;	/* TODO: this needs to be set somewhere */
			float_array = vrShmemAlloc0(MAX_NSENSOR_VALUES * sizeof(float));

			/* make the initial assignment, and set last_value the same */
			vrAssignNsensorArray((vrNsensor *)newinput, float_array /* , vrTime time */);
			memcpy(((vrNsensor *)newinput)->values, ((vrNsensor *)newinput)->last_values, MAX_NSENSOR_VALUES * sizeof(float));

			newinput->checksum = vrInputMakeChecksum(newinput);

			if (vrDbgDo(INPUT_DBGLVL)) {
				vrPrintf("Created: ");
				vrFprintInputValue(stdout, newinput, verbose);
			}
		}
		break;

	/*******************/
	case VRINPUT_CONTROL:
		sprintf(trace_msg, "  creating %d Controls", num);
		vrTrace("vrInputCreateDataContainerArrayOfType", trace_msg);
		memptr = (char *)vrShmemAlloc0(num * sizeof(vrControl));
		for (count = 0; count < num; count++) {
			newinput = (vrGenericInput *)(memptr + sizeof(vrControl) * count);
			newinput->object_type = VROBJECT_INPUTDATA;
			newinput->input_type = VRINPUT_CONTROL;
			newinput->my_device = indev;
			newinput->queue_me = 0;
			newinput->dummy = 0;
			newinput->lock = vrLockCreateName(vrContext, "control input");

			/* assign the Null-callback until changed by the input device */
			((vrControl *)newinput)->callback_assigned = 0;
			((vrControl *)newinput)->callback = vrCallbackCreateNamed("CreateType:Control-DN", vrDoNothing, 0);

			newinput->checksum = vrInputMakeChecksum(newinput);

			if (vrDbgDo(INPUT_DBGLVL)) {
				vrPrintf("Created: ");
				vrFprintInputValue(stdout, newinput, verbose);
			}
		}
		break;

	/*********************/
	case VRINPUT_KEYSTROKE:
		/* TODO: implement VRINPUT_KEYSTROKE section */
		break;

	/****************/
	case VRINPUT_TEXT:
		/* TODO: implement VRINPUT_TEXT section */
		break;

	/********************/
	case VRINPUT_POSITION:	/* equivalent to VRINPUT_POINTER */
		/* TODO: implement VRINPUT_POSITION section */
		break;

	/*****************/
	case VRINPUT_PLANE:
		/* TODO: implement VRINPUT_PLANE section */
		break;

	/******/
	default:
		vrDbgPrintf("Unknown input type\n");
	}

	vrTrace("vrInputCreateDataContainerArrayOfType", "returning");

	return (void *)memptr;
}


/***************************************************************************/
/* vrInputCountDataContainers(): uses the linked list of input-objects for */
/*   the given device to count the inputs.                                 */
/***************************************************************************/
void vrInputCountDataContainers(vrInputDevice *devinfo)
{
	vrInput		*input_object;

	for (input_object = devinfo->inputs; input_object != NULL; input_object = (vrInput *)(input_object->next)) {
		input_object->desc = vrParseInputDescription(input_object->desc_str);
		switch (input_object->desc->type) {
		case VRINPUT_BINARY:
			devinfo->num_2ways++;
			break;
		case VRINPUT_NWAY:
			devinfo->num_Nways++;
			break;
		case VRINPUT_VALUATOR:
			devinfo->num_valuators++;
			break;
		case VRINPUT_6SENSOR:
			devinfo->num_6sensors++;
			break;
		case VRINPUT_NSENSOR:
			devinfo->num_Nsensors++;
			break;
		case VRINPUT_CONTROL:
			devinfo->num_controls++;
			break;

		case VRINPUT_KEYSTROKE:
		case VRINPUT_TEXT:
		case VRINPUT_POSITION:
		case VRINPUT_PLANE:
		default:
			vrErrPrintf(RED_TEXT "vrInputCountDataContainers(): encountered not-yet-implemented input type.\n" NORM_TEXT);
			break;

		}
	}

	for (input_object = devinfo->self_controls; input_object != NULL; input_object = (vrInput *)input_object->next) {
		devinfo->num_scontrols++;
	}

	devinfo->counted = 1;

	vrDbgPrintfN(INPUT_DBGLVL, "vrInputCountDataContainers(): " RED_TEXT "Info: just counted inputs for '%s' -- (2:%d, N:%d, V:%d, 6:%d, N:%d, C:%d)\n" NORM_TEXT,
		devinfo->name,
		devinfo->num_2ways,
		devinfo->num_Nways,
		devinfo->num_valuators,
		devinfo->num_6sensors,
		devinfo->num_Nsensors,
		devinfo->num_controls);
}


/**********************************************************************************/
/* vrInputCreateDataContainers(): uses the linked list of input-objects for       */
/*   the given device to create the inputs, assign initial values, and            */
/*   then ... (TODO: fill in the ...) using the list of Input Functions for each  */
/*                                                                                */
/* The list/array of vrInputFunction pointers is provided by each input device    */
/*   definition at the top of the _<DEVICE>CreateFunction() function as a         */
/*   "List of possible inputs".
/**********************************************************************************/
void vrInputCreateDataContainers(vrInputDevice *devinfo, vrInputFunction *input_functions)
{
	vrInput		*input_object;
	vrGenericInput	*input;
	vrInputFunction	*infunc;
	vrInputDTI	*dti;			/* the device-type-instance information of an input description */
	int		mapped;			/* flag for indicating if a successful map found for a declared input */
	int		map_count = 0;		/* count the number of mappings (NOTE: includes unable-to-handle matches) */
	int		num_2ways = 0;
	int		num_Nways = 0;
	int		num_valuators = 0;
	int		num_6sensors = 0;
	int		num_Nsensors = 0;
	int		num_controls = 0;

#if 0 /* TODO: decide which is better -- or spend effort to concatenate strings */
	vrTrace("vrInputCreateDataContainers", "beginning");
#else
	vrTrace("vrInputCreateDataContainers", devinfo->name);
#endif

	/************************************************/
	/* First count the input types for this device. */
	if (!devinfo->counted)
		vrInputCountDataContainers(devinfo);

	/*************************/
	/* Now create the inputs */

	/* create an array of each type of input */
	devinfo->switch2  = (vr2switch  *)vrInputCreateDataContainerArrayOfType(VRINPUT_BINARY, devinfo->num_2ways, devinfo);
	devinfo->switchN  = (vrNswitch  *)vrInputCreateDataContainerArrayOfType(VRINPUT_NWAY, devinfo->num_Nways, devinfo);
	devinfo->valuator = (vrValuator *)vrInputCreateDataContainerArrayOfType(VRINPUT_VALUATOR, devinfo->num_valuators, devinfo);
	devinfo->sensor6  = (vr6sensor  *)vrInputCreateDataContainerArrayOfType(VRINPUT_6SENSOR, devinfo->num_6sensors, devinfo);
	devinfo->sensorN  = (vrNsensor  *)vrInputCreateDataContainerArrayOfType(VRINPUT_NSENSOR, devinfo->num_Nsensors, devinfo);
	devinfo->control  = (vrControl  *)vrInputCreateDataContainerArrayOfType(VRINPUT_CONTROL, devinfo->num_controls, devinfo);

	vrDbgPrintfN(INPUT_DBGLVL, "vrInputCountDataContainers(): " RED_TEXT "Info: just created inputs for '%s' -- (2:%d, N:%d, V:%d, 6:%d, N:%d, C:%d)\n" NORM_TEXT,
		devinfo->name,
		devinfo->num_2ways,
		devinfo->num_Nways,
		devinfo->num_valuators,
		devinfo->num_6sensors,
		devinfo->num_Nsensors,
		devinfo->num_controls);

	/*****************************************************/
	/* Now initialize the input types for this device.   */
	/*   - For each input config object in the list      */
	/*     assign it to an input of the particular type. */
	/*   - Find a matching input mapping function based  */
	/*     on the input type, and type name in the list  */
	/*     of functions given by "input_functions".      */
	for (input_object = devinfo->inputs; input_object != NULL; input_object = (vrInput *)(input_object->next)) {
		switch (input_object->desc->type) {
		case VRINPUT_BINARY:
			input = (vrGenericInput *)&devinfo->switch2[num_2ways];
			num_2ways++;
			break;

		case VRINPUT_NWAY:
			input = (vrGenericInput *)&devinfo->switchN[num_Nways];
			num_Nways++;
			break;

		case VRINPUT_VALUATOR:
			input = (vrGenericInput *)&devinfo->valuator[num_valuators];
			num_valuators++;
			break;

		case VRINPUT_6SENSOR:
			input = (vrGenericInput *)&devinfo->sensor6[num_6sensors];
			num_6sensors++;
			break;

		case VRINPUT_NSENSOR:
			input = (vrGenericInput *)&devinfo->sensorN[num_Nsensors];
			num_Nsensors++;
			break;

		case VRINPUT_CONTROL:
			input = (vrGenericInput *)&devinfo->control[num_controls];
			num_controls++;
			break;


		case VRINPUT_KEYSTROKE:
		case VRINPUT_TEXT:
		case VRINPUT_POSITION:
		case VRINPUT_PLANE:
		default:
			vrErrPrintf(RED_TEXT "vrInputCreateDataContainers(): encountered not-yet-implemented input type.\n" NORM_TEXT);
			break;
		}

		input->my_object = input_object;
		input_object->desc_ui[0] = '\0';
		input_object->container.generic = input;

		/* parse the input arguments from the format: '<device>:type[instance]' */
		dti = vrParseInputDTI(input_object->desc->args);

#if 1 /* I'm including this because sometimes dti is corrupted, and I don't know why! [09/05/13] */
		while (dti == NULL || dti->device == NULL || dti->type == NULL || dti->instance == NULL) {
			vrErrPrintf(RED_TEXT "vrInputCreateDataContainers(): Parsing of the device type returned a corrupted value!  Trying again.\n" NORM_TEXT);
			dti = vrParseInputDTI(input_object->desc->args);
		}
#endif

		/* loop through the list of possible input functions */
		mapped = 0;
		for (infunc = input_functions; infunc->name != NULL && !mapped; infunc++) {
			vrDbgPrintfN(INPUT_DBGLVL, "vrInputCreateDataContainers(): trying to match input '%s' with input func '%s' (%s)\n",
				input_object->desc->args, infunc->name, vrInputTypeName(infunc->type));
			if (infunc->type == input->input_type && !strcasecmp(dti->type, infunc->name)) {
				vrDbgPrintfN(INPUT_DBGLVL, "vrInputCreateDataContainers(): ... possible match -- type = '%s' (%s) ... \n\t",
					dti->type, vrInputTypeName(input->input_type));
				switch ((infunc->func)(devinfo, input, dti)) {
				case VRINPUT_NOMATCH:
					vrDbgPrintfN(INPUT_DBGLVL, "vrInputCreateDataContainers(): not a good match, try again\n");
					break;

				case VRINPUT_MATCH_UNABLE:	/* matches, but unable to handle request */
					/* We'll just march on without this input then, and claim it's mapped. */
					vrDbgPrintfN(INPUT_DBGLVL, "vrInputCreateDataContainers(): can't handle this ... but pretending to be a ");/* ... "successful match\n" */

				case VRINPUT_MATCH_ABLE:	/* matches, and able to handle request */
					mapped = 1;
					map_count++;
					vrDbgPrintfN(INPUT_DBGLVL, "vrInputCreateDataContainers(): successful match\n");
					break;
				}
			}
else { vrDbgPrintfN(INPUT_DBGLVL, "vrInputCreateDataContainers(): type mismatch '%s' != '%s'\n", dti->type, vrInputTypeName(input->input_type)); }
		}

		if (!mapped)
			vrDbgPrintfN(AALWAYS_DBGLVL, "vrInputCreateDataContainers(): "
				RED_TEXT "Warning, no mapping found for device '%s' input '%s', a %s of type '%s'\n" NORM_TEXT,
				devinfo->name, input_object->name, vrInputTypeName(input->input_type), input_object->desc->args);
	}
	vrDbgPrintfN(INPUT_DBGLVL, "vrInputCreateDataContainers(): " RED_TEXT "Info: just mapped %d inputs for '%s'\n" NORM_TEXT,
		map_count, devinfo->name);

	vrTrace("vrInputCreateDataContainers", "ending");
}


/*******************************************************************/
void vrDeleteInputs(vrInputDevice *devinfo)
{
	int		count;
	vrGenericInput	*input;

	/* Loop through each type of input for the device delete the lock, and then the input. */
	for (count = 0; count < devinfo->num_2ways; count++) {
		input = (vrGenericInput *)&devinfo->switch2[count];
		vrLockFree(input->lock);
		vrShmemFree(input);
	}
	for (count = 0; count < devinfo->num_Nways; count++) {
		input = (vrGenericInput *)&devinfo->switchN[count];
		vrLockFree(input->lock);
		vrShmemFree(input);
	}
	for (count = 0; count < devinfo->num_valuators; count++) {
		input = (vrGenericInput *)&devinfo->valuator[count];
		vrLockFree(input->lock);
		vrShmemFree(input);
	}
	for (count = 0; count < devinfo->num_6sensors; count++) {
		input = (vrGenericInput *)&devinfo->sensor6[count];
		vrLockFree(input->lock);
		vrShmemFree(input);
	}
	for (count = 0; count < devinfo->num_Nsensors; count++) {
		input = (vrGenericInput *)&devinfo->sensorN[count];
		vrLockFree(input->lock);
		vrShmemFree(input);
	}
	for (count = 0; count < devinfo->num_controls; count++) {
		input = (vrGenericInput *)&devinfo->control[count];
		vrLockFree(input->lock);
		vrShmemFree(input);
	}
}


/*******************************************************************/
void *vrGetFuncFromControlList(char *funcname, vrControlFunc *funclist)
{
	vrControlFunc	*cfunc;

	for (cfunc = funclist; cfunc->name != NULL; cfunc++) {
		if (!strcasecmp(funcname, cfunc->name))
			return (void *) cfunc->func;
	}

	return NULL;
}


/*******************************************************************/
int vrInputDeviceInList(char *device)
{
	vrInputOptsType	*option;	/* for looping through the list of possible input devices */

	/* Loop through the input device options specified in vr_input.opts.h     */
	/*   until a string match is made with the "device" string, then call the */
	/*   associated initialization function to fill in the info structure.    */
	option = vrInputOpts;
	while (option->option_name != NULL) {
		if (!strcasecmp(option->option_name, device)) {
			return (1);
		}
		option++;
	}

	return (0);
}


/*******************************************************************************/
/* vrInputCreateSelfControlContainers(): uses the linked list of self-controls */
/*   listed with the input object of the given device to create and setup each */
/*   self-control, assign initial values, and then ... (fill this in) using    */
/*   the list of Input Functions for each                                      */
/*******************************************************************************/
void vrInputCreateSelfControlContainers(vrInputDevice *devinfo, vrInputFunction *input_functions, vrControlFunc *funclist)
{
static	char		string[256];		/* temp space for creating strings */
	vrInput		*scontrol;
	vrInputFunction	*infunc;
	vrInputDTI	*dti;
	char		*input_args;
	vrInputType	 input_type;
	int		 mapped;		/* flag for indicating if a successful map found for a declared input */
	int		 map_count = 0;		/* count the number of mappings (NOTE: includes unable-to-handle matches) */
	void		(*callback_function)();

	/* this part originally from vr_input.magellan.c */

	/* initialize all the input device (self) controls */
	for (scontrol = devinfo->self_controls; scontrol != NULL; scontrol = (vrInput *)scontrol->next) {
		scontrol->desc = vrParseInputDescription(scontrol->desc_str);
		input_type = scontrol->desc->type;
		input_args = scontrol->desc->args;
		vrDbgPrintfN(DEFAULT_DBGLVL, "Handling '%s' self control '%s' with '%s'\n",
			devinfo->name, scontrol->name, input_args);

		scontrol->container.control = (vrControl *)vrInputCreateDataContainerArrayOfType(VRINPUT_CONTROL, 1, devinfo);
		scontrol->container.control->my_object = scontrol;

		/* assign the control callback */
		callback_function = (void (*)())vrGetFuncFromControlList(scontrol->name, funclist);
		if (callback_function == NULL) {
			scontrol->container.control->callback_assigned = -1;
			vrDbgPrintfN(AALWAYS_DBGLVL, "vrInputCreateSelfControlContainers(): "
				RED_TEXT "Warning, no callback function ('%s') found for device '%s' input '%s'\n" NORM_TEXT,
				scontrol->name, devinfo->name, scontrol->desc->args);
			/* scontrol->container.control->callback was set to a Do-nothing callback when created */
		} else {
			sprintf(string, "SelfControl: '%s' of device '%s", scontrol->name, devinfo->name);
			scontrol->container.control->callback = vrCallbackCreateNamed(string, callback_function, 1, devinfo);
			scontrol->container.control->callback_assigned =  1;
		}

		/* parse the input arguments from the format: '<device>:type[instance]' */
		dti = vrParseInputDTI(input_args);

		/* this code from vr_input.c::vrInputCreateDataContainers() */

		/* loop through the list of possible input functions */
		mapped = 0;
		for (infunc = input_functions; infunc->name != NULL && !mapped; infunc++) {
			vrDbgPrintfN(INPUT_DBGLVL, "vrInputCreateSelfControlContainers(): trying to match input '%s' with input func '%s' (%d)\n",
				scontrol->desc->args, infunc->name, infunc->type);
			if (infunc->type == input_type && !strcasecmp(dti->type, infunc->name)) {
				vrDbgPrintfN(INPUT_DBGLVL, "vrInputCreateSelfControlContainers(): ... possible match -- type = '%s' (%d) ... \n\t",
					dti->type, input_type);
				switch ((infunc->func)(devinfo, scontrol->container.generic, dti)) {
				case VRINPUT_NOMATCH:
					vrDbgPrintfN(INPUT_DBGLVL, "vrInputCreateSelfControlContainers(): not a good match, try again\n");
					break;

				case VRINPUT_MATCH_UNABLE:	/* matches, but unable to handle request */
					/* We'll just march on without this input then, and claim it's mapped. */
					vrDbgPrintfN(INPUT_DBGLVL, "vrInputCreateSelfControlContainers(): can't handle this ... ");

				case VRINPUT_MATCH_ABLE:	/* matches, and able to handle request */
					mapped = 1;
					map_count++;
					vrDbgPrintfN(INPUT_DBGLVL, "vrInputCreateSelfControlContainers(): successful match\n");
					break;
				}
			}
		}
		if (!mapped)
			vrDbgPrintfN(AALWAYS_DBGLVL, "vrInputCreateSelfControlContainers(): "
				RED_TEXT "Warning, no mapping found for device '%s' input '%s'\n" NORM_TEXT,
				devinfo->name, scontrol->desc->args);
	}
	vrDbgPrintfN(SELFCTRL_DBGLVL, "vrInputCreateSelfControlContainers(): " RED_TEXT "Info: just mapped %d self-controls for '%s'\n" NORM_TEXT,
		map_count, devinfo->name);
}


/**********************************************************************/
/* vrFprintInputValue(): ...   */
/**********************************************************************/
void vrFprintInputValue(FILE *file, vrGenericInput *input, vrPrintStyle style)
{
	vrMatrix	*tmpmat;

	if (input == NULL) {
		vrFprintf(file, "NULL Input = { }\n");
		return;
	}

	switch (style) {
	case none:
		break;

	case brief:
	case one_line:
		vrFprintf(file, "Input at %#p, object_name = '%s', ",
			input,
			(input->my_object != NULL ? input->my_object->name : "-"));
		switch(input->input_type) {

		case VRINPUT_BINARY:	/* equivalent to VRINPUT_2WAY */
			vrFprintf(file, "value = %d", ((vr2switch *)input)->value);
			break;
		case VRINPUT_NWAY:	/* equivalent to VRINPUT_NARY */
			vrFprintf(file, " NYI");	/* TODO: implement this */
			break;
		case VRINPUT_VALUATOR:
			vrFprintf(file, "value = %.2f", ((vrValuator *)input)->value);
			break;
		case VRINPUT_6SENSOR:
			tmpmat = ((vr6sensor *)input)->position;
			vrFprintf(file, "active = %d, oob = %d, error = %d, position = (%3.1f %3.1f %3.1f %3.1f/%3.1f %3.1f %3.1f %3.1f/%3.1f %3.1f %3.1f %3.1f/%3.1f %3.1f %3.1f %3.1f)",
				((vr6sensor *)input)->active,
				((vr6sensor *)input)->oob,
				((vr6sensor *)input)->error,
				tmpmat->v[ 0], tmpmat->v[ 4], tmpmat->v[ 8], tmpmat->v[12],
				tmpmat->v[ 1], tmpmat->v[ 5], tmpmat->v[ 9], tmpmat->v[13],
				tmpmat->v[ 2], tmpmat->v[ 6], tmpmat->v[10], tmpmat->v[14],
				tmpmat->v[ 3], tmpmat->v[ 7], tmpmat->v[11], tmpmat->v[15]);
			break;
		case VRINPUT_NSENSOR:
			vrFprintf(file, " NYI");	/* TODO: implement this */
			break;
		case VRINPUT_CONTROL:
			vrFprintf(file, "callback_assigned = %d, callback = %#p",
				((vrControl *)input)->callback_assigned,
				((vrControl *)input)->callback);
			break;

		default:
			vrFprintf(file, " NYI");	/* TODO: implement this */
			break;
		}
		vrFprintf(file, "\n");

		break;

	case file_format:
		/* TODO: or at least determine if this means anything -- probably not */
	case machine:
		/* TODO: not yet implemented -- output easily machine parsed */
		vrFprintf(file, "TODO: put special version of input value info here\n");
		break;

	default:
	case verbose:
	case everything:
		vrFprintf(file, "Input at %#p = {\n"
			"\r\tInput_type = %d\n\tchecksum = %d\n\tmy_device = %#p ('%s')\n"
			"\tmy_object = %#p ('%s')\n"
			"\tlock = %#p\n\tqueue_me = %d\n\tdummy = %d\n\ttimestamp = %.2f\n",
			input,
			input->input_type,
			input->checksum,
			input->my_device,
			(input->my_device != NULL ? input->my_device->name : "-"),
			input->my_object,
			(input->my_object != NULL ? input->my_object->name : "-"),
			input->lock,
			input->queue_me,
			input->dummy,
			input->timestamp);

		switch(input->input_type) {
		case VRINPUT_BINARY:	/* equivalent to VRINPUT_2WAY */
			vrFprintf(file, "\r"
				"\r\tvalue = %d\n\tlast_value = %d\n\tvisren_value = %d\n",
				((vr2switch *)input)->value,
				((vr2switch *)input)->last_value,
				((vr2switch *)input)->visren_value);
			break;
		case VRINPUT_NWAY:	/* equivalent to VRINPUT_NARY */
			vrFprintf(file, "\r"
				"\r\tvalue = %d\n\tlast_value = %d\n\tvisren_value = %d\n",
				((vrNswitch *)input)->value,
				((vrNswitch *)input)->last_value,
				((vrNswitch *)input)->visren_value);
			break;
		case VRINPUT_VALUATOR:
			vrFprintf(file, "\r"
				"\r\tvalue = %f\n\tlast_value = %f\n\tvisren_value = %f\n",
				((vrValuator *)input)->value,
				((vrValuator *)input)->last_value,
				((vrValuator *)input)->visren_value);
			break;
		case VRINPUT_6SENSOR:
			tmpmat = ((vr6sensor *)input)->position;
			vrFprintf(file, "\r"
				"\r\tdof = %d\n\tactive = %d\n\toob = %d\n\terror = %d\n"
				"\tposition  = [%#p] (%3.1lf %3.1lf %3.1lf %3.1lf/%3.1lf %3.1lf %3.1lf %3.1lf/%3.1lf %3.1lf %3.1lf %3.1lf/%3.1lf %3.1lf %3.1lf %3.1lf)\n",
				((vr6sensor *)input)->dof,
				((vr6sensor *)input)->active,
				((vr6sensor *)input)->oob,
				((vr6sensor *)input)->error,
				tmpmat,
				tmpmat->v[ 0],
				tmpmat->v[ 4],
				tmpmat->v[ 8],
				tmpmat->v[12],
				tmpmat->v[ 1],
				tmpmat->v[ 5],
				tmpmat->v[ 9],
				tmpmat->v[13],
				tmpmat->v[ 2],
				tmpmat->v[ 6],
				tmpmat->v[10],
				tmpmat->v[14],
				tmpmat->v[ 3],
				tmpmat->v[ 7],
				tmpmat->v[11],
				tmpmat->v[15]);
			tmpmat = ((vr6sensor *)input)->r2e_xform;
			vrFprintf(file, "\r"
				"\tr2e_xform = [%#p] (%3.1lf %3.1lf %3.1lf %3.1lf/%3.1lf %3.1lf %3.1lf %3.1lf/%3.1lf %3.1lf %3.1lf %3.1lf/%3.1lf %3.1lf %3.1lf %3.1lf)\n",
				tmpmat,
				tmpmat->v[ 0],
				tmpmat->v[ 4],
				tmpmat->v[ 8],
				tmpmat->v[12],
				tmpmat->v[ 1],
				tmpmat->v[ 5],
				tmpmat->v[ 9],
				tmpmat->v[13],
				tmpmat->v[ 2],
				tmpmat->v[ 6],
				tmpmat->v[10],
				tmpmat->v[14],
				tmpmat->v[ 3],
				tmpmat->v[ 7],
				tmpmat->v[11],
				tmpmat->v[15]);
			tmpmat = ((vr6sensor *)input)->visren_position;
			vrFprintf(file, "\r"
				"\tvisren_pos= [%#p] (%3.1f %3.1lf %3.1lf %3.1lf/%3.1lf %3.1lf %3.1lf %3.1lf/%3.1lf %3.1lf %3.1lf %3.1lf/%3.1lf %3.1lf %3.1lf %3.1lf)\n",
				tmpmat,
				tmpmat->v[ 0],
				tmpmat->v[ 4],
				tmpmat->v[ 8],
				tmpmat->v[12],
				tmpmat->v[ 1],
				tmpmat->v[ 5],
				tmpmat->v[ 9],
				tmpmat->v[13],
				tmpmat->v[ 2],
				tmpmat->v[ 6],
				tmpmat->v[10],
				tmpmat->v[14],
				tmpmat->v[ 3],
				tmpmat->v[ 7],
				tmpmat->v[11],
				tmpmat->v[15]);
			break;
		case VRINPUT_NSENSOR:
			vrFprintf(file, " NYI");	/* TODO: implement this */
			break;
		case VRINPUT_CONTROL:
			vrFprintf(file, "\r"
				"\r\tcallback_assigned = %d\n\tcallback = %#p\n",
				((vrControl *)input)->callback_assigned,
				((vrControl *)input)->callback);
			break;
		default:
			vrFprintf(file, "\r\tvalue = ... [value printing not yet implemented for this type]\n");
			break;
		}
		vrFprintf(file, "\r}\n");

		break;
	}
}


/******************************************************************/
/* vrInputInitProc(): Initialize the input devices associated with */
/*   this process.                                                 */
/******************************************************************/
void vrInputInitProc(vrProcessInfo *myproc_info)
{
	int		count;

	vrTrace("vrInputInitProc", "beginning");
	proc_info = myproc_info;

#ifdef COREDUMP_IS_GOOD
	vrSetSignalHandler(vrInputSignalHandler);
#endif

	if (vrDbgDo(DEFAULT_DBGLVL+1)) {
		vrMsgPrintf("Input proc is: ");
		vrFprintProcessInfo(stdout, myproc_info, verbose);
	}

	/* copy data into static variables to avoid latency from indirection */
	num_devices = myproc_info->num_things;
	devices = (vrInputDevice **)(myproc_info->things);

	/**************************************************************/
	/*** Create inputs for all input devices (for this process) ***/
	/**************************************************************/
	vrTrace("vrInputInitProc", "creating inputs");
	for (count = 0; count < num_devices; count++) {
		if (devices[count] != NULL) {
			/* create the device's inputs */
			vrCallbackInvoke(devices[count]->Create);
		} else {
			vrErrPrintf("vrInputInitProc()-creating: " RED_TEXT "Device '%s' is null.  Something is wrong.\n" NORM_TEXT, myproc_info->thing_names[count]);
		}
	}


	/*************************************************/
	/*** Open all input devices (for this process) ***/
	/*************************************************/
	vrTrace("vrInputInitProc", "opening devices");
	for (count = 0; count < num_devices; count++) {
		if (devices[count] != NULL) {
			/* start the input device */
			vrTrace("vrInputInitProc", devices[count]->name);
			vrCallbackInvoke(devices[count]->Open);
		} else {
			vrErrPrintf("vrInputInitProc()-opening: " RED_TEXT "Device '%s' is null.  Something is wrong.\n" NORM_TEXT, myproc_info->thing_names[count]);
		}
	}

	/************************************/
	/*** Initialize timing statistics ***/
	/************************************/
	/* NOTE: here are the meanings of all the stats values:        */
	/*    <1 to num_devices-1> is time spent polling that device   */
	/*    <num_devices> is time spent in sleep.                    */
	/*    <num_devices+1> is time spent waiting for sync barrier   */
	/*    <num_devices+2> is time spent waiting for freeze barrier */
	if (myproc_info->stats_args) {
		vrTrace("vrInputInitProc", "initializing process statistics");
		myproc_info->stats = vrProcessStatsCreate(myproc_info->name, num_devices+3, myproc_info->stats_args);

		/* if no y-location was assigned then change the default */
		if (!strstr(myproc_info->stats_args, "yloc"))
			myproc_info->stats->yloc = 0.667;

		/* set the element labels */
		for (count = 0; count < num_devices; count++)
			myproc_info->stats->elem_labels[count] = vrShmemStrDup(devices[count]->name);
		myproc_info->stats->elem_labels[num_devices+0] = vrShmemStrDup("sleep");
		myproc_info->stats->elem_labels[num_devices+1] = vrShmemStrDup("sync");
		myproc_info->stats->elem_labels[num_devices+2] = vrShmemStrDup("freeze");
	}

	return;
}


/*************************************************************/
/* vrInputTermProc(): Close the input devices associated with */
/*   this process.                                            */
/*************************************************************/
void vrInputTermProc(vrProcessInfo *myproc_info)
{
#if 1
	int		count;
#endif

	vrTrace("vrInputTermProc", "beginning");

#if 1 /* 7/3/01: this sometimes hangs during the close-down phase (during a shmem-free call) */
	/**************************************************************/
	/*** Create inputs for all input devices (for this process) ***/
	/**************************************************************/
	for (count = 0; count < num_devices; count++) {
		if (devices[count] != NULL) {
#if 0 /* 11/15/01: instead of doing nothing, we at least need to close the device -- I think, I know xwindows should be closed to reenable keyrepeating */
			/* delete the device's inputs */
			vrDeleteInputs(devices[count]);		/* TODO: Need to try doing this, but testing input existance when calculating perspective matrices */

			/* close the device */
			vrCallbackInvoke(devices[count]->Close);

			vrShmemFree(devices[count]);		/* TODO: can't do this until the vrDeleteInputs() stuff works */
#else
			/* close the device */
			vrCallbackInvoke(devices[count]->Close);
#endif
		} else {
			vrErrPrintf("vrInputTermProc(): " RED_TEXT "Null device.  Something is wrong.\n" NORM_TEXT);
		}
	}
#endif

	return;
}


/*****************************************************************************/
/* vrInputOneFrame(): handle all the work for a single input frame.  Which   */
/*   basically just means poll each input device.  Since num_devices and     */
/*   devices are file-scope globals, there are no arguments to this function.*/
/*   We may want to change this, and have these values passed as arguments.  */
/*****************************************************************************/
void vrInputOneFrame(vrProcessInfo *myproc_info)
{
static	char	trace_msg[256];				/* space to create a string for vrTrace */
	int	count;					/* loop counter */

	/* calculate frame rates and set the process time values */
	myproc_info->frame_count++;
	vrProcessCalcFrameRate(myproc_info);

	/* measure: update time measurement array index */
	vrProcessStatsNextFrame(myproc_info->stats);

	/* do the input polling */
	for (count = 0; count < num_devices; count++) {
		if (devices[count]) {
			sprintf(trace_msg, "about to poll inputs from device %s", devices[count]->name);
			vrTrace("vrInputOneFrame", trace_msg);
			vrCallbackInvoke(devices[count]->PollData);
		}
		vrProcessStatsMark(myproc_info->stats, count, 0);
	}
}


/**********************************************************************/
/* vrInputMainLoop(): this process will just loop, invoking a polling */
/*   function for each of the devices it's responsible for.           */
/**********************************************************************/
void vrInputMainLoop(vrProcessInfo *myproc_info)
{
#if 0
static	char	trace_msg[256];				/* space to create a string for vrTrace */
	int	count;					/* loop counter */
#endif
	int	sync_order;				/* the order of hitting the barrier */
	vrTime	loop_wtime = vrCurrentWallTime();	/* time of the beginning of each loop */

	vrTrace("vrInputMainLoop", "beginning");

	/* At this point, the inputs for all the devices _of this process_ */
	/*   have been created.  Some of the devices may not be fully open */
	/*   yet, and for those, more attempts to reopen can be made while */
	/*   the device is polled.                                         */

	/******************************/
	/*** Poll all input devices ***/
	/******************************/
	/* enter a (seemingly infinite) loop, polling each device */

	while (!myproc_info->end_proc) {
		vrTrace("vrInputMainLoop", BOLD_TEXT "*** top of input loop ***" NORM_TEXT);

		/* do minimal frame delay to allow other processes to get CPU time */
		/* NOTE: Delays of 10,000us or less allow 50fps frame rate for the input process (at least on my Thinkpad 770Z running Linux) */
		vrSleep(myproc_info->usec_min - (long)((vrCurrentWallTime() - loop_wtime) * 1000000.0));
		vrProcessStatsMark(myproc_info->stats, num_devices, 0);	/* time spent in sleep */

		vrProcessSync(myproc_info, num_devices+1, num_devices+2);

		/* start measuring for minimal loop time */
		loop_wtime = vrCurrentWallTime();

		vrInputOneFrame(myproc_info);
	}

	vrTrace("vrInputMainLoop", "ending");
	return;
}


/******************************************************************/
/* vrInputCheckIfAllInputDevicesAreOpen():                        */
/******************************************************************/
int vrInputCheckIfAllInputDevicesAreOpen(vrContextInfo *context)
{
	vrConfigInfo	*vrConfig = context->config;
	int		num_devices = vrConfig->num_input_devices;
	int		devnum;

	for (devnum = 0; devnum < num_devices; devnum++) {
		if (vrConfig->input_devices[devnum]->operating == 0) {
			return 0;
		}
	}

	return 1;
}


/*******************************************************************/
/* vrInputWaitForAllInputDevicesToBeOpen():                        */
/*   waits at a barrier until all the input devices are open -- is */
/*   (currently at least) necessary for some functions that assume */
/*   the input data structures are all taken care of.              */
/*                                                                 */
/* NOTE: at the moment (12/10/2002) this function is not actually  */
/*   called by the library.  The travel.c test program calls it    */
/*   before beginning the world simulation in earnest, however     */
/*   most programs don't call it, and they seem to run fine.  The  */
/*   reason the library doesn't call it is to allow the world      */
/*   simulation to begin, even when the system isn't fully engaged.*/
/*******************************************************************/
void vrInputWaitForAllInputDevicesToBeOpen()
{
	vrConfigInfo	*vrConfig = vrContext->config;
	int		num_devices = vrConfig->num_input_devices;
	int		devnum;

	for (devnum = 0; devnum < num_devices; devnum++) {
		vrDbgPrintfN(ALWAYS_DBGLVL, "FreeVR: Input device '%s' is %s\n", vrConfig->input_devices[devnum]->name,
			(vrConfig->input_devices[devnum]->operating ? "operating." : RED_TEXT "NOT YET operating." NORM_TEXT));
	}
	for (devnum = 0; devnum < num_devices; devnum++) {
		while(vrConfig->input_devices[devnum]->operating == 0) {
			vrDbgPrintfN(ALWAYS_DBGLVL, "FreeVR: " BOLD_TEXT "Still waiting for '%s' to be operating.\n" NORM_TEXT,
				vrConfig->input_devices[devnum]->name);
			vrSleep(1000000);
		}
	}
	vrDbgPrintfN(ALWAYS_DBGLVL, "FreeVR: " BOLD_TEXT ">>>>>>>>>>>>>> All %d input devices reporting operating. <<<<<<<<<<<<<<\n"
		NORM_TEXT, num_devices);
}


/*********************************************************************/
/* vrInputWaitForAllInputsToBeCreated():                             */
/*   Waits until the inputs for all the input devices to be created  */
/*   This is (currently at least) necessary for some functions that  */
/*   assume the input data structures are all taken care of.         */
/* 03/08/2006: added the concept of a maximum number of loops before */
/*   ending the waiting.  This may be bad in some circumstances, but */
/*   for optional input devices, at least we can continue getting    */
/*   FreeVR up and running.                                          */
/*********************************************************************/
void vrInputWaitForAllInputsToBeCreated(vrContextInfo *context)
{
	vrConfigInfo	*vrConfig = context->config;
	int		devnum;
	int		num_devices = vrConfig->num_input_devices;
	int		wait_count;
	int		max_wait = 10;			/* TODO: probably want this to be configurable -- perhaps on a device by device basis */
	int		gave_up_flag = 0;

	for (devnum = 0; devnum < num_devices; devnum++) {
		vrDbgPrintfN(ALWAYS_DBGLVL, "FreeVR: Inputs for device '%s' are %s\n", vrConfig->input_devices[devnum]->name,
			(vrConfig->input_devices[devnum]->created ? "created." : RED_TEXT "NOT YET created." NORM_TEXT));
	}
	for (devnum = 0; devnum < num_devices; devnum++) {
		wait_count = 0;
		while (vrConfig->input_devices[devnum]->created == 0 && wait_count < max_wait) {
			vrDbgPrintfN(ALWAYS_DBGLVL, "FreeVR: " BOLD_TEXT "Still waiting for '%s' inputs to be created.\n" NORM_TEXT,
				vrConfig->input_devices[devnum]->name);
			vrSleep(1000000);
			wait_count++;
		}
		if (vrConfig->input_devices[devnum]->created == 0) {
			vrDbgPrintfN(ALWAYS_DBGLVL, "FreeVR: " BOLD_TEXT "*** Giving up on the creation of input '%s' ***.\n" NORM_TEXT,
					vrConfig->input_devices[devnum]->name);
			gave_up_flag++;

			/* TODO: Create dummy inputs for all the inputs that were anticipated for this device! */
		}
	}
	if (gave_up_flag) {
		vrDbgPrintfN(ALWAYS_DBGLVL, "FreeVR: " BOLD_TEXT ">>>>>> Inputs for all but %d of the %d devices have been created. <<<<<<\n" NORM_TEXT, gave_up_flag, num_devices);
	} else {
		vrDbgPrintfN(ALWAYS_DBGLVL, "FreeVR: " BOLD_TEXT ">>>>>> Inputs for all %d devices created. <<<<<<\n" NORM_TEXT, num_devices);
	}
}


/**********************************************************************/
void vrGetDefaultInputDeviceInfo(vrInputDevice *info)
{
	info->version = (char *)vrShmemStrDup("Default (null) input device");
	info->Create = vrCallbackCreateNamed("Default Indev:Create-DN", vrDoNothing, 2, info);
	info->Open = vrCallbackCreateNamed("Default Indev:Open-DN", vrDoNothing, 2, info);
	info->Close = vrCallbackCreateNamed("Default Indev:Close-DN", vrDoNothing, 2, info);
	info->Reset = vrCallbackCreateNamed("Default Indev:Reset-DN", vrDoNothing, 2, info);
	info->PollData = vrCallbackCreateNamed("Default Indev:PollData-DN", vrDoNothing, 2, info);

	info->operating = 1;

	return;
}


/**********************************************************************/
/* Fill in the info structure callbacks with the functions associated */
/*   with the "type" field string indicating the type of the input    */
/*   devices.  First checks for DSO functions, then searches the list */
/*   specified in the vr_input.opts.h header file.                    */
/**********************************************************************/
void vrGetInputDeviceInfo(vrInputDevice *info)
{
	char		*type = info->type;	/* string name that indicates type of the input device */

	/******************************************************************/
	/*** First check DSO values for initializing the info structure ***/

	/* NOTE: relies on lazy boolean evaluation.  The strlen operation is here */
	/*   to allow a configuration file to cancel a dso setting by giving the  */
	/*   empty string as an argument.                                         */
	if (info->dso_file && strlen(info->dso_file) > 0) {
		char		*dso_func = info->dso_func;
		char		*dso_file = info->dso_file;
		void		*dso_handle = NULL;
		void		(*dso_info_func)(vrInputDevice *) = NULL;

#if defined(__hpux)
		 vrErrPrintf("vrGetInputDeviceInfo: "
                             RED_TEXT "This platform does not support DSO loading (yet)\n" NORM_TEXT);
#else
		/* If a DSO file has been named, then first attempt to use it. */

		/* give a default function name if none given */
		if (!dso_func)
			dso_func = "initializeDeviceInfo";

vrPrintf(RED_TEXT "LD_LIBRARY_PATH is '%s'\n", getenv("LD_LIBRARY_PATH"));
vrPrintf(RED_TEXT "LD_LIBRARYN32_PATH is '%s'\n", getenv("LD_LIBRARYN32_PATH"));
		dso_handle = dlopen(dso_file, RTLD_LAZY);
		if (dso_handle == NULL) {
			vrErrPrintf("vrGetInputDeviceInfo: "
				RED_TEXT "Unable to load DSO input device file ('%s').\n" NORM_TEXT,
				dso_file);
			vrPrintf("dlerror: " BOLD_TEXT "'%s'\n" NORM_TEXT, dlerror());
		} else {
			info->dso_handle = dso_handle;	/* store the handle for removing the dso when the device is closed */
			dso_info_func = (void (*)(vrInputDevice *))dlsym(dso_handle, dso_func);
			if (dso_info_func == NULL) {
				vrErrPrintf("vrGetInputDeviceInfo: "
					RED_TEXT "No \"%s\" function in DSO input device file ('%s').\n" NORM_TEXT,
					dso_func, dso_file);
				vrPrintf(BOLD_TEXT "dlerror: '%s'\n" NORM_TEXT, dlerror());
			} else {
				/* If the dso operations worked, then fill in the info structure */
				/*   by calling the initialization function.                     */
				(*dso_info_func)(info);
				return;
			}
		}
#endif
	}

	/**********************************************************************************/
	/*** Next search the list of possible input devices listed in vr_inputs.opts.h. ***/

	/* an implied "else" here, since we'll have returned if the DSO worked */
	/*   (though if it didn't work, it will report an error, but still     */
	/*   attempt to function using the built-in version of the input).     */
	if (!type) {
		/* If no "type" string is given (ie. it's NULL), then use */
		/*   a default input device for this device.              */
		vrGetDefaultInputDeviceInfo(info);
		return;

	} else {
		vrInputOptsType	*option;	/* for looping through the list of possible input devices */

		/* Loop through the input device options specified in vr_input.opts.h   */
		/*   until a string match is made with the "type" string, then call the */
		/*   associated initialization function to fill in the info structure.  */
		option = vrInputOpts;
		while (option->option_name != NULL) {
			if (!strcasecmp(option->option_name, type)) {
				(*option->info_func)(info);
				return;
			}
			option++;
		}
	}

	/***************************************************/
	/*** Last option is the default nop input device ***/

	/* If we get here, then neither the DSO nor the input-option list provided */
	/*   a function for initializing the info structure, so use the default    */
	/*   function (which basically setups up a bunch of nop callbacks).        */
	vrGetDefaultInputDeviceInfo(info);
	return;
}


	/************************************************************/
	/************************************************************/
	/************ Generic input type access function ************/


/**************************************************************************/
/* TODO: for all the input assignment functions, we could have a third    */
/*   argument (vrTime) that allows the time to be specific by the calling */
/*   party, or if a value of 0 time is given then determine the time here.*/
/*   I suggest this because for devices like the tracker daemon, a time   */
/*   is already given for when the value was read by the remote process.  */
/*                                                                         */
/* TODO: do we also want get-functions to return the timestamp value/delta?*/


/****************************************************************/
/* TODO: add features to the 6-sensor assignments */
void vrAssignGenericInput(vrGenericInput *input, char *value /* , vrTime time */)
{
static	char		*whitespace = " \t\r\b\n";
#if 0 /* currently in unused code */
	vrMatrix	mat_data;
	vrMatrix	*mat = &mat_data;
#endif

#if 0
	float	float_value_array[6];	/* for assigning a 6-sensor from 6 values */
#endif
	if (value == NULL) {
		vrDbgPrintfN(AALWAYS_DBGLVL, "vrAssignGenericInput(): Null assignment string given.\n");
		return;
	}

	if (input == NULL) {
		vrDbgPrintfN(AALWAYS_DBGLVL, "vrAssignGenericInput(): Null input pointer given.\n");
		return;
	}

	value += strspn(value, whitespace);		/* skip white */

	switch (input->input_type) {
	case VRINPUT_BINARY:	/* equivalent to VRINPUT_2WAY */
		vrAssign2switchValue((vr2switch *)input, vrAtoI(value) /* , vrTime time */);
		break;

	case VRINPUT_NWAY:	/* equivalent to VRINPUT_NARY */
		vrAssignNswitchValue((vrNswitch *)input, vrAtoI(value) /* , vrTime time */);
		break;

	case VRINPUT_VALUATOR:
		vrAssignValuatorValue((vrValuator *)input, atof(value) /* , vrTime time */);
		break;

	case VRINPUT_6SENSOR:
vrPrintf("vrAssignGenericInput(): 6-sensor assignment: '%s' (%#p)\n", value, value);
		if (!strncasecmp(value, "id", 2)) {
			vrMatrix	mat;
			/* the "id" command resets 6-sensor to the identity matrix */
			vrMatrixSetIdentity(&mat);
			vrAssign6sensorValue((vr6sensor *)input, &mat, 0 /* , vrTime time */);
		} else if (!strncasecmp(value, "loc", 3)) {
			/* the "loc" command repositions the sensor at x,y,z */
			vrMatrix	mat;

			vrMatrixGet6sensorValuesDirect(&mat, (vr6sensor *)input);

			/* replace the X, Y & Z values that are given -- using a "*" will leave it as is */
			value = strchr(&value[3], ' ');				/* skip to a space */
vrPrintf("vrAssignGenericInput(): 6-sensor assignment: '%s' (%#p)\n", value, value);
			if (value != NULL && strlen(value) > 0) {		/* done if no more spaces */
				value += strspn(value, whitespace);     		/* skip white */
				if (value[0] != '*') {
	vrPrintf("Setting X to %f (%s)\n", atof(value), value);
					VRMAT_ROWCOL(&mat, VR_X, VR_W) = atof(value);	/* set 'x' value */
				}
				value = strchr(value, ' ');				/* skip to next space */
			}

			if (value != NULL && strlen(value) > 0) {		/* done if no more spaces */
				value += strspn(value, whitespace);     		/* skip white */
				if (value[0] != '*') {
	vrPrintf("Setting Y to %f (%s)\n", atof(value), value);
					VRMAT_ROWCOL(&mat, VR_Y, VR_W) = atof(value);	/* set 'y' value */
				}
				value = strchr(value, ' ');				/* skip to next space */
			}

			if (value != NULL && strlen(value) > 0) {		/* done if no more spaces */
				value += strspn(value, whitespace);     		/* skip white */
				if (value[0] != '*') {
	vrPrintf("Setting Z to %f (%s)\n", atof(value), value);
					VRMAT_ROWCOL(&mat, VR_Z, VR_W) = atof(value);	/* set 'z' value */
				}
			}

			vrAssign6sensorValue((vr6sensor *)input, &mat, 0 /* , vrTime time */);
		} else if (!strncasecmp(value, "move", 4)) {
			/* the "move" command does a movement based on the X,Y,Z,AZIM,ELEV,ROLL values given */
			vr6sensorConv	default_options = {
						VR_Y,		/* azimuth_axis */
						0,		/* ignore_all */
						0,		/* ignore_trans */
						0,		/* tmp_ignore_trans */
						0,		/* relative_axis */
						0,		/* return_to_zero */
						0,		/* restrict_space */
						0,		/* swap_yz */
						0,		/* swap_transrot */
						1.0, 1.0,	/* trans_scale & rot_scale */
						-5.0, 0.0, -5.0,/* working-volume minimums */
						5.0, 10.0, 5.0	/* working-volume maximums */
					};
			float		valuators[6] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
			value = strchr(&value[4], ' ');				/* skip past next space */
			if (value == NULL || strlen(value) == 0) break;		/* done if no more spaces */
			value += strspn(value, whitespace);     		/* skip white */
			valuators[0] = atof(value);

			value = strchr(value, ' ');				/* skip to next space */
			if (value == NULL || strlen(value) == 0) break;		/* done if no more spaces */
			value += strspn(value, whitespace);     		/* skip white */
			valuators[1] = atof(value);

			value = strchr(value, ' ');				/* skip to next space */
			if (value == NULL || strlen(value) == 0) break;		/* done if no more spaces */
			value += strspn(value, whitespace);     		/* skip white */
			valuators[2] = atof(value);

			value = strchr(value, ' ');				/* skip to next space */
			if (value == NULL || strlen(value) == 0) break;		/* done if no more spaces */
			value += strspn(value, whitespace);     		/* skip white */
			valuators[3] = atof(value);

			value = strchr(value, ' ');				/* skip to next space */
			if (value == NULL || strlen(value) == 0) break;		/* done if no more spaces */
			value += strspn(value, whitespace);     		/* skip white */
			valuators[4] = atof(value);

			value = strchr(value, ' ');				/* skip to next space */
			if (value == NULL || strlen(value) == 0) break;		/* done if no more spaces */
			value += strspn(value, whitespace);     		/* skip white */
			valuators[5] = atof(value);

			vrAssign6sensorValueFromValuators((vr6sensor *)input, valuators, &default_options, -1 /* , vrTime time */);
		} else if (!strncasecmp(value, "nr", 2)) {
			/* the "nudger2e" command allows for adjusting the 6-sensors calibration */
			/* Form: "nudger2e <x-rot> <y-rot> <z-rot>"                              */
			vrMatrix	*tmpmat;
			double		xrot,yrot,zrot = 0.0;			/* rotation values */

			value = strchr(&value[2], ' ');				/* skip to a space */
vrPrintf("vrAssignGenericInput(): 6-sensor r2e nudge: '%s' (%#p)\n", value, value);
			if (value != NULL && strlen(value) > 0) {		/* done if no more spaces */
				value += strspn(value, whitespace);     		/* skip white */
				xrot = atof(value);
				value = strchr(value, ' ');				/* skip to next space */
			}
			if (value != NULL && strlen(value) > 0) {		/* done if no more spaces */
				value += strspn(value, whitespace);     		/* skip white */
				yrot = atof(value);
				value = strchr(value, ' ');				/* skip to next space */
			}
			if (value != NULL && strlen(value) > 0) {		/* done if no more spaces */
				value += strspn(value, whitespace);     		/* skip white */
				zrot = atof(value);
				value = strchr(value, ' ');				/* skip to next space */
			}

			/* Now do the rotation in-place */
			tmpmat = ((vr6sensor *)input)->r2e_xform;
			vrMatrixPostMult(tmpmat, vrMatrixSetRotationId(vrMatrixCreate(), VR_X, xrot));
			vrMatrixPostMult(tmpmat, vrMatrixSetRotationId(vrMatrixCreate(), VR_Y, yrot));
			vrMatrixPostMult(tmpmat, vrMatrixSetRotationId(vrMatrixCreate(), VR_Z, zrot));
		}
		break;

	default:
		break;

	}
}


	/**************************************************************/
	/**************************************************************/
	/************ Switch 2 input type access functions ************/


/****************************************************************/
/* NOTE: 1/16/2003: I'm now forcing the stored value to be binary. */
/*   Previously it was really just any integer.                    */
void vrAssign2switchValue(vr2switch *switch2, int newvalue /* , vrTime time */)
{
	int	assign_value;

	if (switch2->object_type != VROBJECT_INPUTDATA) {
		vrDbgPrintf("Attempt to assign a binary value to a non-input object type %d at %#p\n", switch2->object_type, switch2);
		return;
	}

	if (switch2->input_type != VRINPUT_BINARY) {
		vrDbgPrintf("Attempt to assign a binary value to an incorrect input type %d at %#p\n", switch2->input_type, switch2);
		return;
	}

	/* force value to be binary */
	assign_value = (newvalue != 0);

	/* store the value if it's changed */
	if (switch2->value != assign_value) {
#if 0
vrPrintf("switch %p is getting new value %d\n", switch2, assign_value);
#endif
		/* value has changed */
		vrLockWriteSet(switch2->lock);
		switch2->timestamp = 0.0;		/* TODO: assign timestamp */
		switch2->value = assign_value;
		vrLockWriteRelease(switch2->lock);

		if (switch2->queue_me) {
			/* TODO: put me in the queue */
		}
	}

	/* store historical measurement value */
	/* TODO: determine whether historical value measurements should only be recorded */
	/*   when values change, or always.  For the moment, we will do the "always"     */
	/*   method -- for the "when-change" method this section of code should be moved */
	/*   to be within the above if-statement.                                        */
	if (switch2->num_measures > 0) {
		switch2->current_measure++;
		switch2->current_measure %= switch2->num_measures;

		switch2->measures[switch2->current_measure] = switch2->value;
#if 0
printf("storing historical data for '%s', measure num %d is %d\n", switch2->my_object->name, switch2->current_measure, switch2->measures[switch2->current_measure]);
#endif
	}
}


/****************************************************************/
/* NOTE: returns 1 if successful, 0 if doesn't exist in current configuration */
int vrInputSet2switchDescription(int input_num, char *input_desc)
{
	vrGenericInput	*input;

	input = vrInputGetFromTypeIndex(vrContext, VRINPUT_BINARY, input_num);
	if (input == NULL) {
		/* Create a new dummy input to handle the request */

		vrDbgPrintfN(AALWAYS_DBGLVL, RED_TEXT "No button (%d) available to '%s' (Creating a dummy)\n" NORM_TEXT, input_num, input_desc);
		input = vrInputAddDummyToInputMap(vrContext->input, VRINPUT_BINARY, input_num);

		strncpy(input->my_object->desc_ui, input_desc, INPUT_UIDESC_LEN);

		return 0;
	}

	strncpy(input->my_object->desc_ui, input_desc, INPUT_UIDESC_LEN);

	return 1;
}


/****************************************************************/
int vrGet2switchValue(int input_num)
{
	vr2switch	*input;
	int		value;

	input = (vr2switch *)vrInputGetFromTypeIndex(vrContext, VRINPUT_BINARY, input_num);
	if (input == NULL) {
		/* TODO: this is where we can create new dummy inputs to handle the request */
		return 0;
	}

#if 0
vrTrace("vrGet2switchValue", "pre-read");
#endif
	vrLockReadSet(input->lock);
	value = input->value;
#if 0
vrTrace("vrGet2switchValue", "pre-read2write");
#endif
	vrLockReadToWrite(input->lock);
	input->last_value = value;
#if 0
vrTrace("vrGet2switchValue", "pre-write-rel");
#endif
	vrLockWriteRelease(input->lock);
#if 0
vrTrace("vrGet2switchValue", "end");
#endif

	return value;
}


/****************************************************************/
/* This version is intended for Rendering processes, so doesn't update */
/*   "last_value" and therefore won't disrupt the Delta calculations.  */
int vrGet2switchValueNoLastUpdate(int input_num)
{
	vr2switch	*input;
	int		value;

	input = (vr2switch *)vrInputGetFromTypeIndex(vrContext, VRINPUT_BINARY, input_num);
	if (input == NULL) {
		/* TODO: this is where we can create new dummy inputs to handle the request */
		return 0;
	}

	vrLockReadSet(input->lock);
	value = input->value;
	vrLockReadRelease(input->lock);

	return value;
}


/**********************************************************************/
int vrGet2switchDelta(int input_num)
{
	vr2switch	*input;
	int		delta;

	input = (vr2switch *)vrInputGetFromTypeIndex(vrContext, VRINPUT_BINARY, input_num);
	if (input == NULL) {
		/* TODO: this is where we can create new dummy inputs to handle the request */
		return 0;
	}

	vrLockReadSet(input->lock);
	delta = input->value - input->last_value;
	vrLockReadToWrite(input->lock);
	input->last_value = input->value;
	vrLockWriteRelease(input->lock);

	return delta;
}


/**********************************************************************/
int vrGet2switchValueDirect(vr2switch *input)
{
	int		value;

	if (input == NULL) {
		if (vrDbgDo(CONFIG_WARN_DBGLVL)) {
			vrErrPrintf("vrGet2switchValueDirect(): Invalid NULL Binary input.\n");
		}
		return 0;
	}
	vrLockReadSet(input->lock);
	value = input->value;
	vrLockReadToWrite(input->lock);
	input->last_value = value;
	vrLockWriteRelease(input->lock);

	return value;
}


/**********************************************************************/
int vrGet2switchDeltaDirect(vr2switch *input)
{
	int		delta;

	if (input == NULL) {
		if (vrDbgDo(CONFIG_WARN_DBGLVL)) {
			vrErrPrintf("vrGet2switchDeltaDirect(): Invalid NULL Binary input.\n");
		}
		return 0;
	}
	vrLockReadSet(input->lock);
	delta = input->value - input->last_value;
	vrLockReadToWrite(input->lock);
	input->last_value = input->value;
	vrLockWriteRelease(input->lock);

	return delta;
}


	/**************************************************************/
	/**************************************************************/
	/************ Switch N input type access functions ************/


/**********************************************************************/
void vrAssignNswitchValue(vrNswitch *switchN, int newvalue /* , vrTime time */)
{
	if (switchN->object_type != VROBJECT_INPUTDATA) {
		vrDbgPrintf("Attempt to assign a N-switch value to a non-input object type %d at %#p\n", switchN->object_type, switchN);
		return;
	}

	if (switchN->input_type != VRINPUT_NARY) {
		vrDbgPrintf("Attempt to assign a N-switch value to an incorrect input type %d at %#p\n", switchN->input_type, switchN);
		return;
	}

	/* store the value if it's changed */
	if (switchN->value != newvalue) {
		/* value has changed */
		vrLockWriteSet(switchN->lock);
		switchN->timestamp = 0.0;		/* TODO: assign timestamp */
		switchN->value = newvalue;
		vrLockWriteRelease(switchN->lock);

		if (switchN->queue_me) {
			/* TODO: put me in the queue */
		}
	}
}


/**********************************************************************/
/* NOTE: returns 1 if successful, 0 if doesn't exist in current configuration */
int vrInputSetNswitchDescription(int input_num, char *input_desc)
{
	vrGenericInput	*input;

	input = vrInputGetFromTypeIndex(vrContext, VRINPUT_NWAY, input_num);
	if (input == NULL) {
		/* Create a new dummy input to handle the request */

		vrDbgPrintfN(AALWAYS_DBGLVL, RED_TEXT "No N-switch (%d) available to '%s' (Creating a dummy)\n" NORM_TEXT, input_num, input_desc);
		input = vrInputAddDummyToInputMap(vrContext->input, VRINPUT_NWAY, input_num);

		strncpy(input->my_object->desc_ui, input_desc, INPUT_UIDESC_LEN);

		return 0;
	}

	strncpy(input->my_object->desc_ui, input_desc, INPUT_UIDESC_LEN);

	return 1;
}


/**********************************************************************/
int vrGetNswitchValue(int input_num)
{
	vrNswitch	*input;
	int		value;

	input = (vrNswitch *)vrInputGetFromTypeIndex(vrContext, VRINPUT_NWAY, input_num);
	if (input == NULL) {
		/* TODO: this is where we can create new dummy inputs to handle the request */
		return 0;
	}

	vrLockReadSet(input->lock);
	value = input->value;
	vrLockReadToWrite(input->lock);
	input->last_value = value;
	vrLockWriteRelease(input->lock);

	return value;
}

/**********************************************************************/
/* This version is intended for Rendering processes, so doesn't update */
/*   "last_value" and therefore won't disrupt the Delta calculations.  */
int vrGetNswitchValueNoLastUpdate(int input_num)
{
	vrNswitch	*input;
	int		value;

	input = (vrNswitch *)vrInputGetFromTypeIndex(vrContext, VRINPUT_NWAY, input_num);
	if (input == NULL) {
		/* TODO: this is where we can create new dummy inputs to handle the request */
		return 0;
	}

	vrLockReadSet(input->lock);
	value = input->value;
	vrLockReadRelease(input->lock);

	return value;
}

/**********************************************************************/
int vrGetNswitchDelta(int input_num)
{
	vrNswitch	*input;
	int		delta;

	input = (vrNswitch *)vrInputGetFromTypeIndex(vrContext, VRINPUT_NWAY, input_num);
	if (input == NULL) {
		/* TODO: this is where we can create new dummy inputs to handle the request */
		return 0;
	}

	vrLockReadSet(input->lock);
	delta = input->value - input->last_value;
	vrLockReadToWrite(input->lock);
	input->last_value = input->value;
	vrLockWriteRelease(input->lock);

	return delta;
}

/**********************************************************************/
int vrGetNswitchValueDirect(vr2switch *input) { return -10; }
/**********************************************************************/
int vrGetNswitchDeltaDirect(vr2switch *input) { return -10; }


	/**************************************************************/
	/**************************************************************/
	/************ Valuator input type access functions ************/


/**********************************************************************/
void vrAssignValuatorValue(vrValuator *valuator, float newvalue /* , vrTime time */)
{
	if (valuator->object_type != VROBJECT_INPUTDATA) {
		vrDbgPrintf("Attempt to assign a valuator value to a non-input object type %d at %#p\n", valuator->object_type, valuator);
		return;
	}

	if (valuator->input_type != VRINPUT_VALUATOR) {
		vrDbgPrintf("Attempt to assign a valuator value to an incorrect input type %d at %#p\n", valuator->input_type, valuator);
		return;
	}

	/* store the value if it's changed */
	if (valuator->value != newvalue) {
		/* value has changed */
		vrLockWriteSet(valuator->lock);
		valuator->timestamp = 0.0;		/* TODO: assign timestamp */
		valuator->value = newvalue;
		vrLockWriteRelease(valuator->lock);

		if (valuator->queue_me) {
			/* TODO: put me in the queue */
		}
	}

	/* store historical measurement value */
	/* TODO: determine whether historical value measurements should only be recorded */
	/*   when values change, or always.  For the moment, we will do the "always"     */
	/*   method -- for the "when-change" method this section of code should be moved */
	/*   to be within the above if-statement.                                        */
	if (valuator->num_measures > 0) {
		valuator->current_measure++;
		valuator->current_measure %= valuator->num_measures;

		valuator->measures[valuator->current_measure] = valuator->value;
#if 0
printf("storing historical data for '%s', measure num %d is %d\n", valuator->my_object->name, valuator->current_measure, valuator->measures[valuator->current_measure]);
#endif
	}
}


/**********************************************************************/
/* NOTE: returns 1 if successful, 0 if doesn't exist in current configuration */
int vrInputSetValuatorDescription(int input_num, char *input_desc)
{
	vrGenericInput	*input;

	input = vrInputGetFromTypeIndex(vrContext, VRINPUT_VALUATOR, input_num);
	if (input == NULL) {
		/* Create a new dummy input to handle the request */

		vrDbgPrintfN(AALWAYS_DBGLVL, RED_TEXT "No Valuator (%d) available to '%s' (Creating a dummy)\n" NORM_TEXT, input_num, input_desc);
		input = vrInputAddDummyToInputMap(vrContext->input, VRINPUT_VALUATOR, input_num);

		strncpy(input->my_object->desc_ui, input_desc, INPUT_UIDESC_LEN);

		return 0;
	}

	strncpy(input->my_object->desc_ui, input_desc, INPUT_UIDESC_LEN);

	return 1;
}


/**********************************************************************/
float vrGetValuatorValue(int input_num)
{
	vrValuator	*input;
	float		value;

	input = (vrValuator *)vrInputGetFromTypeIndex(vrContext, VRINPUT_VALUATOR, input_num);
	if (input == NULL) {
		/* TODO: this is where we can create new dummy inputs to handle the request */
		return 0;
	}

	vrLockReadSet(input->lock);
	value = input->value;
	vrLockReadToWrite(input->lock);
	input->last_value = value;
	vrLockWriteRelease(input->lock);

	return value;
}


/**********************************************************************/
/* This version is intended for Rendering processes, so doesn't update */
/*   "last_value" and therefore won't disrupt the Delta calculations.  */
float vrGetValuatorValueNoLastUpdate(int input_num)
{
	vrValuator	*input;
	float		value;

	input = (vrValuator *)vrInputGetFromTypeIndex(vrContext, VRINPUT_VALUATOR, input_num);
	if (input == NULL) {
		/* TODO: this is where we can create new dummy inputs to handle the request */
		return 0;
	}

	vrLockReadSet(input->lock);
	value = input->value;
	vrLockReadRelease(input->lock);

	return value;
}


/**********************************************************************/
float vrGetValuatorDelta(int input_num)
{
	vrValuator	*input;
	float		delta;

	input = (vrValuator *)vrInputGetFromTypeIndex(vrContext, VRINPUT_VALUATOR, input_num);
	if (input == NULL) {
		/* TODO: this is where we can create new dummy inputs to handle the request */
		return 0;
	}

	vrLockReadSet(input->lock);
	delta = input->value - input->last_value;
	vrLockReadToWrite(input->lock);
	input->last_value = input->value;
	vrLockWriteRelease(input->lock);

	return delta;
}


/**********************************************************************/
float vrGetValuatorValueDirect(vrValuator *input) { return -10.0; }
/**********************************************************************/
float vrGetValuatorDeltaDirect(vrValuator *input) { return -10.0; }


	/**************************************************************/
	/**************************************************************/
	/************ Sensor 6 input type access functions ************/


/**********************************************************************/
/*  ... */
/*   set the r2e matrix to either the identity matrix, the current r2e value */
/*   or a matrix specified by 16 values (the latter not yet implemented).    */
/*   For the latter two options (ie. the non-identity matrix ones), the new  */
/*   r2e matrix is used to reassign the current matrix raw values.           */
void vrAssign6sensorR2Exform(vr6sensor *sensor6, char *xform_info)
{
static	char		*skip_chars = " \t\n\r\b,";
	vrMatrix	mat;


	/* If no string provided, then just leave the identity matrix in place */
	if (xform_info == NULL)
		return;

	/* If string is zero-length, then just leave the identity matrix in place */
	if (strlen(xform_info) == 0)
		return;

	/* skip any commas and whitespace at the beginning of the string */
	xform_info += strspn(xform_info, skip_chars);

	/* If value is "id" then just leave the identity matrix in place */
	if (!strncasecmp(xform_info, "id", 2))
		return;

	if (!strcasecmp(xform_info, "r2e")) {
		vrMatrixCopy(sensor6->r2e_xform, sensor6->my_object->r2e_xform);
		vrAssign6sensorValue(sensor6, vrMatrixGet6sensorRawValuesDirect(&mat, sensor6), -1 /* , vrCurrentWallTime() */);
		return;
	}

	if (!strncasecmp(xform_info, "xform:", 6)) {
		vrErrPrintf("vrAssign6sensorR2Exform(): " RED_TEXT "Sorry, 'xform:' option not yet implemented, using identity matrix.\n" NORM_TEXT);
		vrAssign6sensorValue(sensor6, vrMatrixGet6sensorRawValuesDirect(&mat, sensor6), -1 /* , vrCurrentWallTime() */);
		return;
	}
}


/***********************************************************************/
/* new_mat is in the input device's (tracker's) coordinate system.     */
/*   It is the responsibility of this function to map that matrix      */
/*   into the coordinate system of the VR system (ie. the "real world" */
/*   coordinate system).                                               */
/* values of oob less than zero are treated as a nop (ie. do not change) */
/*   flag.                                                               */
void vrAssign6sensorValue(vr6sensor *sensor6, vrMatrix *new_mat, int oob /* , vrTime time */)
{
	if (sensor6->object_type != VROBJECT_INPUTDATA) {
		vrDbgPrintf("Attempt to assign a position sensor value to a non-input object type %d at %#p\n", sensor6->object_type, sensor6);
		return;
	}

	if (sensor6->input_type != VRINPUT_6SENSOR) {
		vrDbgPrintf("Attempt to assign a position sensor value to an incorrect input type %d at %#p\n",
			sensor6->input_type, sensor6);
		return;
	}

	if (new_mat == NULL) {
		vrErr("vrAssign6sensorValue: Attempted to assign a Null pointer.\n");
		vrPrintf("Pausing: Feel free to use 'dbx -p %d' now to find out where\n", getpid());
		pause();
	}

	/* TODO: checksum check here -- I fixed the bug I was looking for, */
	/*   so I didn't implement this, but we may want to in the future. */

	if (1) {
		/* TODO: If there is a quick memory-checksum function, we could */
		/*   add a checksum field for quickly determining if data has   */
		/*   changed.  If the checksums match, then a slightly slower,  */
		/*   brute force check should be done.                          */

		/* assume (for now) value has changed */
		vrLockWriteSet(sensor6->lock);
		if (oob >= 0)
			sensor6->oob = oob;
		sensor6->timestamp = 0.0;		/* TODO: assign timestamp */
		vrMatrixCopy(sensor6->raw_data, new_mat);

#if 0
		incoming_sensor6.calibrated = 0;
		/* now calibrate the data */
		/* TODO: the calibration stuff */

		/* we may want to use callbacks for the calibration function */
#endif

		/* now transform the data into world space */
		vrMatrixCopy(sensor6->position, sensor6->raw_data);
#if 1 /* 2/2/01 test -- top version is original -- after testing, I'm fairly certain the original was correct */
		vrMatrixPreMult(sensor6->position, sensor6->t2rw_xform); /* TODO: I prefer using sensor6->my_device->t2rw_xform   */
#else
		vrMatrixPostMult(sensor6->position, sensor6->t2rw_xform); /* TODO: I prefer using sensor6->my_device->t2rw_xform  */
#endif
									/*   because then we can change the device's xform w/o    */
									/*   needing to copy it to all the inputs.  However,      */
									/*   because the pre-config, dummy user creates a 6sensor */
									/*   without an inputdevice, my_device is NULL for this   */
									/* TODO: use the newly created (02/22/2008) _MakeDummyInputDevice() */
									/*   function to have a dummy input device for the dummy  */
									/*   6-sensor of the dummy user.                          */

#if 0 /* this first test is wrong -- 2/2/01 testing original again, still doesn't seem to work */
		vrMatrixPreMult(sensor6->position, sensor6->r2e_xform); /* TODO: the correctness of this is in question -- always tested with ident mat */
#else /* this seems to work fine. */
	/* 2/11/00 -- actually, this doesn't work either. rotations about the object */
	/*   (specifically xwindows sim-6), rotation about the origin, not the sensor */
		vrMatrixPostMult(sensor6->position, sensor6->r2e_xform); /* TODO: the correctness of this is in question -- always tested with ident mat */
#endif
#if 0
		incoming_sensor6.frame_of_reference = 0;  /* TODO: set this to world space */
#endif

		vrLockWriteRelease(sensor6->lock);

		if (sensor6->queue_me) {
			/* TODO: put me in the queue */
		}
	}
}


/**********************************************************************/
/* NOTE: the array of valuators is of type (float) in order to work   */
/*   in conjunction with the vrEuler type.  If vrEuler changes to     */
/*   use doubles, then this function should also be similarly changed.*/
/* Values of oob less than zero are treated as a nop (ie. do not change) */
/*   flag.                                                               */
void vrAssign6sensorValueFromValuators(vr6sensor *sensor6, float *valuators, vr6sensorConv *conv_options, int oob /* , vrTime time */)
{
static	vr6sensorConv	default_options = { VR_Y, 0, 0, 0, 0, 0, 0, 0, 0, 1.0, 1.0, { -5.0, 0.0, -5.0 }, { 5.0, 10.0, 5.0 } };
	vrMatrix	mat_data;
	vrMatrix	*mat = &mat_data;
	float		*valuators_trans;	/* pointer to the translational data */
	float		*valuators_rot;		/* pointer to the rotational data */
	float		valuators_tmp[6];	/* holder for valuator values to move them around a bit */
	float		swapper;		/* place to hold a value while swapping */
	float		ts;			/* local copy of translational scaling factor */
	float		rs;			/* local copy of rotational scaling factor */

	if (conv_options == NULL)
		conv_options = &default_options;

	if (conv_options->ignore_all)
		return;

	/* make local copies of trans & rot _scale, use 1.0 if scale factor is 0.0 */
	/* TODO: once all the input devices have been updated to use the trans & rot factors, we can get rid of the check for zero */
	ts = conv_options->trans_scale;
	if (ts == 0.0)
		ts = 1.0;

	rs = conv_options->rot_scale;
	if (rs == 0.0)
		rs = 1.0;

	if (conv_options->swap_transrot) {
		/* swapped */
#if 0
		valuators_trans = &valuators[3];
		valuators_rot   = &valuators[0];
#else
		/* this is a kludge swap that works well for the way I've configured a joydev device */
		/* TODO: we need to use input maps for a more generic way of accomplishing this */
		valuators_tmp[VR_X] = -valuators[VR_AZIM+3];
		valuators_tmp[VR_Z] =  valuators[VR_ELEV+3];
		valuators_tmp[VR_Y] = -valuators[VR_ROLL+3];
		valuators_tmp[VR_AZIM+3] = -valuators[VR_X];
		valuators_tmp[VR_ELEV+3] =  valuators[VR_Z];
		valuators_tmp[VR_ROLL+3] = -valuators[VR_Y];
#endif
	} else {
		valuators_tmp[VR_X] = valuators[VR_X];
		valuators_tmp[VR_Y] = valuators[VR_Y];
		valuators_tmp[VR_Z] = valuators[VR_Z];
		valuators_tmp[VR_AZIM+3] = valuators[VR_AZIM+3];
		valuators_tmp[VR_ELEV+3] = valuators[VR_ELEV+3];
		valuators_tmp[VR_ROLL+3] = valuators[VR_ROLL+3];
	}

	if (conv_options->swap_yz) {
		swapper = valuators[VR_Z];
		valuators_tmp[VR_Z] = -valuators[VR_Y];
		valuators_tmp[VR_Y] = -swapper;

		swapper = valuators[VR_ELEV+3];
		valuators_tmp[VR_ELEV+3] = valuators[VR_ROLL+3];
		valuators_tmp[VR_ROLL+3] = swapper;
	}

	valuators_trans = &valuators_tmp[0];
	valuators_rot   = &valuators_tmp[3];

	/************************************************************/
	/*** Set the starting matrix based on return_to_zero flag ***/

	/* The non-raw matrix has the tracker xform incorporated */
	/*   so using that causes that xform to be added to the  */
	/*   position every time.  And that's bad.               */
	if (conv_options->return_to_zero)
		vrMatrixSetIdentity(mat);
	else	vrMatrixGet6sensorRawValuesDirect(mat, sensor6);


	/****************************************/
	/*** calculate the sensor translation ***/
	if (!conv_options->ignore_trans) {
		/* only map the translation to the sensor when not doing valuator_override */
		if (conv_options->relative_axis) {
			/* do translation using the sensor's axes */
			vrMatrixPostTranslate3d(mat, valuators_trans[VR_X]*ts , valuators_trans[VR_Y]*ts, valuators_trans[VR_Z]*ts);
		} else {
			/* do translation using the world's axes */
			vrMatrixPreTranslate3d(mat, valuators_trans[VR_X]*ts, valuators_trans[VR_Y]*ts, valuators_trans[VR_Z]*ts);
		}
		if (conv_options->restrict_space) {
			/* compare the location with the given working volume, and move inside if nece */
			if (VRMAT_ROWCOL(mat, VR_X, VR_W) > conv_options->working_volume_max[VR_X])
				VRMAT_ROWCOL(mat, VR_X, VR_W) = conv_options->working_volume_max[VR_X];
			if (VRMAT_ROWCOL(mat, VR_X, VR_W) < conv_options->working_volume_min[VR_X])
				VRMAT_ROWCOL(mat, VR_X, VR_W) = conv_options->working_volume_min[VR_X];
			if (VRMAT_ROWCOL(mat, VR_Y, VR_W) > conv_options->working_volume_max[VR_Y])
				VRMAT_ROWCOL(mat, VR_Y, VR_W) = conv_options->working_volume_max[VR_Y];
			if (VRMAT_ROWCOL(mat, VR_Y, VR_W) < conv_options->working_volume_min[VR_Y])
				VRMAT_ROWCOL(mat, VR_Y, VR_W) = conv_options->working_volume_min[VR_Y];
			if (VRMAT_ROWCOL(mat, VR_Z, VR_W) > conv_options->working_volume_max[VR_Z])
				VRMAT_ROWCOL(mat, VR_Z, VR_W) = conv_options->working_volume_max[VR_Z];
			if (VRMAT_ROWCOL(mat, VR_Z, VR_W) < conv_options->working_volume_min[VR_Z])
				VRMAT_ROWCOL(mat, VR_Z, VR_W) = conv_options->working_volume_min[VR_Z];

			sensor6->oob = 0; /* it must be in-bounds, since we forced it to be */
		} else {
			/* determine whether location is out-of-bounds */
			if ((VRMAT_ROWCOL(mat, VR_X, VR_W) > conv_options->working_volume_max[VR_X])
				|| (VRMAT_ROWCOL(mat, VR_X, VR_W) < conv_options->working_volume_min[VR_X])
				|| (VRMAT_ROWCOL(mat, VR_Y, VR_W) > conv_options->working_volume_max[VR_Y])
				|| (VRMAT_ROWCOL(mat, VR_Y, VR_W) < conv_options->working_volume_min[VR_Y])
				|| (VRMAT_ROWCOL(mat, VR_Z, VR_W) > conv_options->working_volume_max[VR_Z])
				|| (VRMAT_ROWCOL(mat, VR_Z, VR_W) < conv_options->working_volume_min[VR_Z])) {
				sensor6->oob = 1;
			} else {
				sensor6->oob = 0;
			}
		}
	}


	/*************************************/
	/*** calculate the sensor rotation ***/
	if (conv_options->relative_axis) {
		/* This seems to work well for the head sensor.   */
		/* TODO: However, it's not the greatest interface */
		/* for controlling wand rotation -- wand rotation */
		/* should probably be relative to the head_sensor */
		/* as with the wand translation.                  */

		/* do rotations around the sensor's axes                  */
		/*   NOTE: in this case, order of rotation doesn't matter */
#define vrMatrixPostRotateIdNew vrMatrixPostRotateId	/* 1/21/03: test whether it works without the "New" */
		switch (conv_options->azimuth_axis) {
		case VR_X:
			vrMatrixPostRotateIdNew(mat, VR_X, valuators_rot[VR_AZIM]*rs);
			vrMatrixPostRotateIdNew(mat, VR_Z, valuators_rot[VR_ELEV]*rs);
			vrMatrixPostRotateIdNew(mat, VR_Y, valuators_rot[VR_ROLL]*rs);
			break;
		case VR_Y:
			vrMatrixPostRotateIdNew(mat, VR_Y, valuators_rot[VR_AZIM]*rs);
			vrMatrixPostRotateIdNew(mat, VR_X, valuators_rot[VR_ELEV]*rs);
			vrMatrixPostRotateIdNew(mat, VR_Z, valuators_rot[VR_ROLL]*rs);
			break;
		case VR_Z:
			vrMatrixPostRotateIdNew(mat, VR_Z, valuators_rot[VR_AZIM]*rs);
			vrMatrixPostRotateIdNew(mat, VR_Y, valuators_rot[VR_ELEV]*rs);
			vrMatrixPostRotateIdNew(mat, VR_X, valuators_rot[VR_ROLL]*rs);
			break;
		}
	} else {
		vrVector	vector;

		/* do rotations around the world's axes */
#if 0 /* TODO: 1/21/03: delete this, I'm pretty sure these comments are out of date */
		/* TODO: need to translate to the origin & back to make   */
		/*   rotation be about the sensor's origin, and not */
		/*   the world origin.                              */
		/* NOTE: There is a function to do this -- vrMatrixPreRotateAboutOriginId() */
		/* 9/6/02 -- hmmm, it looks like this TODO has been taken care of with the */
		/*   vrMatrixGetResetTranslationAd() and vrMatrixSetTranslationOnlyAd() functions */
#else
		/* NOTE: the vrMatrixGetResetTranslationAd() and vrMatrixSetTranslationOnlyAd()    */
		/*   functions are effectively used to translate the matrix to the origin, do the  */
		/*   rotation, and then translate back again.  This way saves a few operations.    */
#endif
		vrMatrixGetResetTranslationAd(mat, vector.v);

		switch (conv_options->azimuth_axis) {
		case VR_X:
			vrMatrixPreRotateIdNew(mat, VR_X, valuators_rot[VR_AZIM]*rs);
			vrMatrixPreRotateIdNew(mat, VR_Z, valuators_rot[VR_ELEV]*rs);
			vrMatrixPreRotateIdNew(mat, VR_Y, valuators_rot[VR_ROLL]*rs);
			break;
		case VR_Y:
			vrMatrixPreRotateIdNew(mat, VR_Y, valuators_rot[VR_AZIM]*rs);
			vrMatrixPreRotateIdNew(mat, VR_X, valuators_rot[VR_ELEV]*rs);
			vrMatrixPreRotateIdNew(mat, VR_Z, valuators_rot[VR_ROLL]*rs);
			break;
		case VR_Z:
			vrMatrixPreRotateIdNew(mat, VR_Z, valuators_rot[VR_AZIM]*rs);
			vrMatrixPreRotateIdNew(mat, VR_Y, valuators_rot[VR_ELEV]*rs);
			vrMatrixPreRotateIdNew(mat, VR_X, valuators_rot[VR_ROLL]*rs);
			break;
		}
		vrMatrixSetTranslationOnlyAd(mat, vector.v);
	}

	vrAssign6sensorValue(sensor6, mat, oob /* , TODO: time */);
}


/**********************************************************************/
/* NOTE: returns 1 if successful, 0 if doesn't exist in current configuration */
int vrInputSet6sensorDescription(int input_num, char *input_desc)
{
	vrGenericInput	*input;

	input = vrInputGetFromTypeIndex(vrContext, VRINPUT_6SENSOR, input_num);
	if (input == NULL) {
		/* Create a new dummy input to handle the request */

		vrDbgPrintfN(AALWAYS_DBGLVL, RED_TEXT "No Position Sensor (%d) available to '%s' (Creating a dummy)\n" NORM_TEXT, input_num, input_desc);
		input = vrInputAddDummyToInputMap(vrContext->input, VRINPUT_6SENSOR, input_num);

		strncpy(input->my_object->desc_ui, input_desc, INPUT_UIDESC_LEN);

		return 0;
	}

	strncpy(input->my_object->desc_ui, input_desc, INPUT_UIDESC_LEN);

	return 1;
}


/**********************************************************************/
vr6sensor *vrGet6sensor(int input_num)
{
	return ((vr6sensor *)vrInputGetFromTypeIndex(vrContext, VRINPUT_6SENSOR, input_num));
}


/**********************************************************************/
vrMatrix *vrMatrixGet6sensorValues(vrMatrix *mat, int input_num)
{
	vr6sensor	*input;

	input = vrGet6sensor(input_num);
	if (input == NULL) {
		vrErrPrintf("vrMatrixGet6sensorValues(): Hmmm, encountered a bad Position Sensor (%d) -- this shouldn't happen.\n", input_num);
		vrMatrixSetIdentity(mat);
		return mat;
	}

	vrLockReadSet(input->lock);
	vrMatrixCopy(mat, input->position);
	vrLockReadToWrite(input->lock);
	vrMatrixCopy(input->last_position, input->position);
	vrLockWriteRelease(input->lock);

	return mat;
}


/**********************************************************************/
/* This version is intended for Rendering processes, so doesn't update */
/*   "last_value" and therefore won't disrupt the Delta calculations.  */
vrMatrix *vrMatrixGet6sensorValuesNoLastUpdate(vrMatrix *mat, int input_num)
{
	vr6sensor	*input;

	input = vrGet6sensor(input_num);
	if (input == NULL) {
		vrErrPrintf("vrMatrixGet6sensorValues(): Hmmm, encountered a bad Position Sensor (%d) -- this shouldn't happen.\n", input_num);
		vrMatrixSetIdentity(mat);
		return mat;
	}

	vrLockReadSet(input->lock);
	vrMatrixCopy(mat, input->position);
	vrLockReadRelease(input->lock);

	return mat;
}


/**********************************************************************/
/* values of oob less than zero are treated as a nop (ie. do not change) */
/*   flag.                                                               */
void vrAssign6sensorOobValue(vr6sensor *sensor6, int oob)
{
	if (sensor6->object_type != VROBJECT_INPUTDATA) {
		vrDbgPrintf("Attempt to assign a position sensor value to a non-input object type %d at %#p\n", sensor6->object_type, sensor6);
		return;
	}

	if (sensor6->input_type != VRINPUT_6SENSOR) {
		vrDbgPrintf("Attempt to assign a position sensor value to an incorrect input type %d at %#p\n",
			sensor6->input_type, sensor6);
		return;
	}

	if (oob >= 0) {
		vrLockWriteSet(sensor6->lock);
		sensor6->oob = oob;
		vrLockWriteRelease(sensor6->lock);
	}

	return;
}


/**********************************************************************/
/* -1 is returned if input_num is not a valid 6-sensor. */
int vrGet6sensorOobValue(int input_num)
{
	vr6sensor	*input;
	int		oob;

	input = vrGet6sensor(input_num);
	if (input == NULL) {
		vrErrPrintf("vrGet6sensorOobValue(): Hmmm, encountered a bad Position Sensor (%d) -- this shouldn't happen.\n", input_num);
		return -1;
	}

	vrLockReadSet(input->lock);
	oob = input->oob;
	vrLockReadRelease(input->lock);

	return oob;
}


/**********************************************************************/
/* values of active less than zero are treated as a nop (ie. do not change) */
/*   flag.                                                               */
void vrAssign6sensorActiveValue(vr6sensor *sensor6, int active)
{
	if (sensor6->object_type != VROBJECT_INPUTDATA) {
		vrDbgPrintf("Attempt to assign a position sensor value to a non-input object type %d at %#p\n", sensor6->object_type, sensor6);
		return;
	}

	if (sensor6->input_type != VRINPUT_6SENSOR) {
		vrDbgPrintf("Attempt to assign a position sensor value to an incorrect input type %d at %#p\n",
			sensor6->input_type, sensor6);
		return;
	}

	if (active >= 0) {
		vrLockWriteSet(sensor6->lock);
		sensor6->active = active;
		vrLockWriteRelease(sensor6->lock);
	}

	return;
}


/**********************************************************************/
/* -1 is returned if input_num is not a valid 6-sensor. */
int vrGet6sensorActiveValue(int input_num)
{
	vr6sensor	*input;
	int		active;

	input = vrGet6sensor(input_num);
	if (input == NULL) {
		vrErrPrintf("vrGet6sensorActiveValue(): Hmmm, encountered a bad Position Sensor (%d) -- this shouldn't happen.\n", input_num);
		return -1;
	}

	vrLockReadSet(input->lock);
	active = input->active;
	vrLockReadRelease(input->lock);

	return active;
}


/**********************************************************************/
/* values of error less than zero are treated as a nop (ie. do not change) */
/*   flag.                                                               */
void vrAssign6sensorErrorValue(vr6sensor *sensor6, int error)
{
	if (sensor6->object_type != VROBJECT_INPUTDATA) {
		vrDbgPrintf("Attempt to assign a position sensor value to a non-input object type %d at %#p\n", sensor6->object_type, sensor6);
		return;
	}

	if (sensor6->input_type != VRINPUT_6SENSOR) {
		vrDbgPrintf("Attempt to assign a position sensor value to an incorrect input type %d at %#p\n",
			sensor6->input_type, sensor6);
		return;
	}

	if (error >= 0) {
		vrLockWriteSet(sensor6->lock);
		sensor6->error = error;
		vrLockWriteRelease(sensor6->lock);
	}

	return;
}


/**********************************************************************/
/* -1 is returned if input_num is not a valid 6-sensor. */
int vrGet6sensorErrorValue(int input_num)
{
	vr6sensor	*input;
	int		error;

	input = vrGet6sensor(input_num);
	if (input == NULL) {
		vrErrPrintf("vrGet6sensorErrorValue(): Hmmm, encountered a bad Position Sensor (%d) -- this shouldn't happen.\n", input_num);
		return -1;
	}

	vrLockReadSet(input->lock);
	error = input->error;
	vrLockReadRelease(input->lock);

	return error;
}


/**********************************************************************/
/* -1 is returned if input_num is not a valid 6-sensor. */
int vrGet6sensorDummyValue(int input_num)
{
	vr6sensor	*input;
	int		dummy;

	input = vrGet6sensor(input_num);
	if (input == NULL) {
		vrErrPrintf("vrGet6sensorDummyValue(): Hmmm, encountered a bad Position Sensor (%d) -- this shouldn't happen.\n", input_num);
		return -1;
	}

	vrLockReadSet(input->lock);
	dummy = input->dummy;
	vrLockReadRelease(input->lock);

	return dummy;
}


/**********************************************************************/
vrMatrix *vrMatrixGet6sensorValuesDirect(vrMatrix *mat, vr6sensor *input)
{

	if (input == NULL) {
		if (vrDbgDo(CONFIG_WARN_DBGLVL)) {
			vrErrPrintf("vrMatrixGet6sensorValuesDirect(): Invalid NULL Position Sensor.\n");
		}
		vrMatrixSetIdentity(mat);
		return mat;
	}

	vrLockReadSet(input->lock);
	vrMatrixCopy(mat, input->position);
	vrLockReadToWrite(input->lock);
	vrMatrixCopy(input->last_position, input->position);
	vrLockWriteRelease(input->lock);

	return mat;
}


/**********************************************************************/
vrMatrix *vrMatrixGet6sensorValuesDirectNoLastUpdate(vrMatrix *mat, vr6sensor *input)
{

	if (input == NULL) {
		vrErrPrintf("vrMatrixGet6sensorValuesDirectNoLastUpdate(): Invalid NULL Position Sensor.\n");
vrErrPrintf("aborting until a better solution is determined\n"); abort();
		vrMatrixSetIdentity(mat);
		return mat;
	}

	vrLockReadSet(input->lock);
	vrMatrixCopy(mat, input->position);
	vrLockReadRelease(input->lock);

	return mat;
}


/**********************************************************************/
vrMatrix *vrMatrixGet6sensorRawValuesDirect(vrMatrix *mat, vr6sensor *input)
{

	if (input == NULL) {
		vrErrPrintf("vrMatrixGet6sensorRawValuesDirect(): Invalid NULL Position Sensor.\n");
		vrMatrixSetIdentity(mat);
		return mat;
	}

	vrLockReadSet(input->lock);
	/* TODO: this will be different for generic non-6DOF sensors */
	vrMatrixCopy(mat, input->raw_data);
	/* TODO: copy raw_data to last_value? */
	vrLockReadRelease(input->lock);

	return mat;
}


/*********************************************************************/
/* vrVectorGetRWFrom6sensorDir(): Get the requested vector emanating */
/*   from the given sensor as it is located in the real world.       */
/* DONE: s/b vrVectorGet6sensorRW(<vector>, <sensor num>, <dir>) */
/* eg. to get the head's forward vector: */
/*	vrVectorGet6sensorRW(head_forward_vector, <head sensor>, VRDIR_FORE); */
/* NOTE: used to be: vrVector *vrGetSensor6VectorRW(vrDirection direction, int sensor_num, vrVector *vector) */
vrVector *vrVectorGetRWFrom6sensorDir(vrVector *vector, int sensor_num, vrDirection direction)
{
static	vrMatrix	mat;				/* used as storage space */
static	vrVector	fore_vec = { {  0.0,  0.0, -1.0 } };
static	vrVector	back_vec = { {  0.0,  0.0,  1.0 } };
static	vrVector	up_vec =   { {  0.0,  1.0,  0.0 } };
static	vrVector	down_vec = { {  0.0, -1.0,  0.0 } };
static	vrVector	left_vec = { { -1.0,  0.0,  0.0 } };
static	vrVector	right_vec= { {  1.0,  0.0,  0.0 } };
	vrVector	*unit_vec;

	switch (direction) {
	default:
		vrErrPrintf("vrVectorGetRWFrom6sensorDir(): " RED_TEXT "Unknown vector direction, using FORE\n" NORM_TEXT);
	case VRDIR_FORE:
		unit_vec = &fore_vec;
		break;
	case VRDIR_BACK:
		unit_vec = &back_vec;
		break;
	case VRDIR_UP:
		unit_vec = &up_vec;
		break;
	case VRDIR_DOWN:
		unit_vec = &down_vec;
		break;
	case VRDIR_LEFT:
		unit_vec = &left_vec;
		break;
	case VRDIR_RIGHT:
		unit_vec = &right_vec;
		break;
	}

	vrVectorTransformByMatrix(vector, unit_vec, vrMatrixGet6sensorValues(&mat, sensor_num));

	return vector;
}


/*********************************************************************/
vrVectorf *vrVectorfGetRWFrom6sensorDir(vrVectorf *vector, int sensor_num, vrDirection direction)
{
	vrVector	double_vector;

	vrVectorGetRWFrom6sensorDir(&double_vector, sensor_num, direction);
	vector->v[VR_X] = double_vector.v[VR_X];
	vector->v[VR_Y] = double_vector.v[VR_Y];
	vector->v[VR_Z] = double_vector.v[VR_Z];

	return vector;
}


/* DONE: rename: see above */
/**********************************************************************/
/* NOTE: used to be: vrVector *vrGetSensor6VectorVW(int user_num, vrDirection direction, int sensor_num, vrVector *vector) */
/* TODO: I don't know whether this function is correct. */
/*   6/2/03: I think this is now fixed because the vrVectorUserVWFromRW() was fixed */
vrVector *vrVectorGetVWFromUser6sensorDir(vrVector *vector, int user_num, int sensor_num, vrDirection direction)
{
	vrVectorGetRWFrom6sensorDir(vector, sensor_num, direction);
	vrVectorUserVWFromRW(user_num, vector, vector);

	return vector;
}


/*********************************************************************/
vrVectorf *vrVectorfGetVWFromUser6sensorDir(vrVectorf *vector, int user_num, int sensor_num, vrDirection direction)
{
	vrVector	double_vector;

	vrVectorGetVWFromUser6sensorDir(&double_vector, user_num, sensor_num, direction);
	vector->v[VR_X] = double_vector.v[VR_X];
	vector->v[VR_Y] = double_vector.v[VR_Y];
	vector->v[VR_Z] = double_vector.v[VR_Z];

	return vector;
}


/**********************************************************************/
/* NOTE: used to be: vrPoint *vrGetSensor6LocationRW(int sensor_num, vrPoint *real_point) */
vrPoint *vrPointGetRWFrom6sensor(vrPoint *real_point, int sensor_num)
{
	vrMatrix	matrix_data;
	vrMatrix	*mat = &matrix_data;

	return (vrPoint *)vrVectorGetTransFromMatrix((vrVector *)real_point, vrMatrixGet6sensorValues(mat, sensor_num));
}


/**********************************************************************/
vrPointf *vrPointfGetRWFrom6sensor(vrPointf *real_point, int sensor_num)
{
	vrPoint	double_point;

	vrPointGetRWFrom6sensor(&double_point, sensor_num);
	real_point->v[VR_X] = double_point.v[VR_X];
	real_point->v[VR_Y] = double_point.v[VR_Y];
	real_point->v[VR_Z] = double_point.v[VR_Z];

	return real_point;
}


/**********************************************************************/
/* NOTE: used to be: vrPoint *vrGetSensor6LocationVW(int usernum, int sensor_num, vrPoint *virtual_point) */
vrPoint *vrPointGetVWFromUser6sensor(vrPoint *virtual_point, int usernum, int sensor_num)
{
	vrMatrix	matrix_data;
	vrMatrix	*mat = &matrix_data;
	vrPoint		point_data;
	vrPoint		*real_point = &point_data;

	vrVectorGetTransFromMatrix((vrVector *)real_point, vrMatrixGet6sensorValues(mat, sensor_num));

	return vrPointGetVWUserFromRWPoint(virtual_point, usernum, real_point);
}


/**********************************************************************/
vrPointf *vrPointfGetVWFromUser6sensor(vrPointf *virtual_point, int usernum, int sensor_num)
{
	vrPoint	double_point;

	vrPointGetVWFromUser6sensor(&double_point, usernum, sensor_num);
	virtual_point->v[VR_X] = double_point.v[VR_X];
	virtual_point->v[VR_Y] = double_point.v[VR_Y];
	virtual_point->v[VR_Z] = double_point.v[VR_Z];

	return virtual_point;
}


/**********************************************************************/
vrEuler *vrEulerGetRWFrom6sensor(vrEuler *euler, int sensor_num)
{
	vrMatrix	matrix_data;
	vrMatrix	*mat = &matrix_data;

	return vrEulerSetFromMatrix(euler, vrMatrixGet6sensorValues(mat, sensor_num));
}


/**********************************************************************/
vrEuler *vrEulerGetVWFromUser6sensor(vrEuler *euler, int usernum, int sensor_num)
{
	vrMatrix	matrix_data;
	vrMatrix	*mat = &matrix_data;

#if 1
	return vrEulerSetFromMatrix(euler, vrMatrixPreMult(vrMatrixGet6sensorValues(mat, sensor_num), vrContext->input->users[usernum]->vw2rw_xform));
#else
	return vrEulerSetFromMatrix(euler, vrMatrixPreMult(vrMatrixGet6sensorValues(mat, sensor_num), vrContext->input->users[usernum]->rw2vw_xform));
#endif
}



	/**************************************************************/
	/**************************************************************/
	/************ Sensor N input type access functions ************/


/******************/
/* TODO: implement this function */
void vrAssignNsensorArray(vrNsensor *sensorN, float *new_data /* , vrTime time */)
{
	if (sensorN->object_type != VROBJECT_INPUTDATA) {
		vrDbgPrintf("Attempt to assign a N-sensor value to a non-input object type %d at %#p\n", sensorN->object_type, sensorN);
		return;
	}

	if (sensorN->input_type != VRINPUT_NSENSOR) {
		vrDbgPrintf("Attempt to assign an N-sensor value to an incorrect input type %d at %#p\n", sensorN->input_type, sensorN);
		return;
	}

	if (new_data == NULL) {
		vrErr("vrAssignNsensorArray: Attempted to assign a Null pointer.\n");
		vrPrintf("Pausing: Feel free to use 'dbx -p %d' now to find out where\n", getpid());
		pause();
	}

	/* TODO: checksum check here -- I fixed the bug I was looking for, */
	/*   so I didn't implement this, but we may want to in the future. */

	if (1) {
		/* TODO: If there is a quick memory-checksum function, we could */
		/*   add a checksum field for quickly determining if data has   */
		/*   changed.  If the checksums match, then a slightly slower,  */
		/*   brute force check should be done.                          */

		/* assume (for now) value has changed */
		vrLockWriteSet(sensorN->lock);
		sensorN->timestamp = 0.0;		/* TODO: assign timestamp */
		memcpy(sensorN->values, new_data, sensorN->dof * sizeof(float));

#if 0
		sensorN.calibrated = 0;
		/* now calibrate the data */
		/* TODO: the calibration stuff */

		/* we may want to use callbacks for the calibration function */
#endif

#if 0
		sensorN.frame_of_reference = 0;  /* ?? TODO: set this to world space */
#endif

		vrLockWriteRelease(sensorN->lock);

		if (sensorN->queue_me) {
			/* TODO: put me in the queue */
		}
	}
}


/**********************************************************************/
/* NOTE: returns 1 if successful, 0 if doesn't exist in current configuration */
int vrInputSetNsensorDescription(int input_num, char *input_desc)
{
	vrGenericInput	*input;

	input = vrInputGetFromTypeIndex(vrContext, VRINPUT_NSENSOR, input_num);
	if (input == NULL) {
		/* Create a new dummy input to handle the request */

		vrDbgPrintfN(AALWAYS_DBGLVL, RED_TEXT "No N-Sensor (%d) available to '%s' (Creating a dummy)\n" NORM_TEXT, input_num, input_desc);
		input = vrInputAddDummyToInputMap(vrContext->input, VRINPUT_NSENSOR, input_num);

		strncpy(input->my_object->desc_ui, input_desc, INPUT_UIDESC_LEN);

		return 0;
	}

	strncpy(input->my_object->desc_ui, input_desc, INPUT_UIDESC_LEN);

	return 1;
}


/**********************************************************************/
/* NOTE: TODO: we're not currently setting the "last_value" to make the Delta() version work! */
float vrGetNsensorValue(int input_num, int datum_num)
{
	vrNsensor	*input;
	float		value;

	if (datum_num >= MAX_NSENSOR_VALUES) {
		vrErrPrintf("vrGetNsensorValue(): Invalid N Sensor datum requested (%d) -- valid range for an N-Sensor datum is [0..%d].\n", datum_num, MAX_NSENSOR_VALUES-1);
		return 0;
	}

	input = (vrNsensor *)vrInputGetFromTypeIndex(vrContext, VRINPUT_NSENSOR, input_num);
	if (input == NULL) {
		/* TODO: this is where we can create new dummy inputs to handle the request */
		vrErrPrintf("vrGetNsensorValue(): Hmmm, encountered a bad N-Sensor -- this shouldn't happen.\n");
		return 0;
	}


	vrLockReadSet(input->lock);
	value = input->values[datum_num];
#if 0
	/* TODO: decide if we want to do this for the single datum */
	vrLockReadToWrite(input->lock);
	input->last_values[datam_num] = value;
	vrLockWriteRelease(input->lock);
#else
	vrLockReadRelease(input->lock);
#endif

	return value;
}


/**********************************************************************/
/* This version is intended for Rendering processes, so doesn't update */
/*   "last_value" and therefore won't disrupt the Delta calculations.  */
float vrGetNsensorValueNoLastUpdate(int input_num, int datum_num)
{
	vrNsensor	*input;
	float		value;

	if (datum_num >= MAX_NSENSOR_VALUES) {
		vrErrPrintf("vrGetNsensorValue(): Invalid N Sensor datum requested (%d) -- valid range for an N-Sensor datum is [0..%d].\n", datum_num, MAX_NSENSOR_VALUES-1);
		return 0;
	}

	input = (vrNsensor *)vrInputGetFromTypeIndex(vrContext, VRINPUT_NSENSOR, input_num);
	if (input == NULL) {
		/* TODO: this is where we can create new dummy inputs to handle the request */
		vrErrPrintf("vrGetNsensorValue(): Hmmm, encountered a bad N-Sensor -- this shouldn't happen.\n");
		return 0;
	}


	vrLockReadSet(input->lock);
	value = input->values[datum_num];
	vrLockReadRelease(input->lock);

	return value;
}


/**********************************************************************/
float *vrGetNsensorArray(int input_num, float *array)
{
	vrNsensor	*input;
	int		array_size;

	input = (vrNsensor *)vrInputGetFromTypeIndex(vrContext, VRINPUT_NSENSOR, input_num);
	if (input == NULL) {
		/* TODO: this is where we can create new dummy inputs to handle the request */
		vrErrPrintf("vrGetNsensorArray(): Hmmm, encountered a bad N-Sensor -- this shouldn't happen.\n");
		return array;
	}

	vrLockReadSet(input->lock);
	array_size = input->dof * sizeof(float);
	memcpy(array, input->values, array_size);
	vrLockReadToWrite(input->lock);
	memcpy(input->last_values, input->values, array_size);
	vrLockWriteRelease(input->lock);

	return array;
}


/**********************************************************************/
float vrGetNsensorValueDirect(vrNsensor *input, int datum_num)
{
	float		value;

	if (input == NULL) {
		vrErrPrintf("vrGetNsensorValueDirect(): Invalid NULL N Sensor.\n");
		return 0;
	}

	if (datum_num >= MAX_NSENSOR_VALUES) {
		vrErrPrintf("vrGetNsensorValueDirect(): Invalid N Sensor datum requested (%d) -- valid range for N-Sensor x0%p is [0..%d].\n", datum_num, input, MAX_NSENSOR_VALUES-1);
		return 0;
	}

	vrLockReadSet(input->lock);
	value = input->values[datum_num];
#if 0
	/* TODO: decide if we want to do this for the single datum */
	vrLockReadToWrite(input->lock);
	input->last_values[datam_num] = value;
	vrLockWriteRelease(input->lock);
#else
	vrLockReadRelease(input->lock);
#endif

	return value;
}


/**********************************************************************/
float *vrGetNsensorArrayDirect(vrNsensor *input, float *array)
{
	int		array_size;

	if (input == NULL) {
		vrErrPrintf("vrGetNsensorValueDirect(): Invalid NULL N Sensor.\n");
		return 0;
	}

	array_size = input->dof * sizeof(float);
	vrLockReadSet(input->lock);
	memcpy(array, input->values, array_size);
	vrLockReadToWrite(input->lock);
	memcpy(input->last_values, input->values, array_size);
	vrLockWriteRelease(input->lock);

	return array;
}


/**********************************************************************/
float vrGetNsensorDelta(int input_num, int datum_num) { return -10.0; }
/**********************************************************************/
float vrGetNsensorDeltaDirect(vrNsensor *input, int datum_num) { return -10.0; }


	/**************************************************************/
	/**************************************************************/

#ifdef GFX_PERFORMER /* { */
/***********************************************************************************/
/* Utility functions that translate FreeVR coordinates in to Performer Z-up coords */
/***********************************************************************************/
/* TODO: in the future we probably will just have a conversion matrix stored */
/*   with the context, and we'll use that matrix in the original functions   */
/*   to convert to whatever coordinate system the graphics system uses.      */


/**********************************************************************/
vrPoint *vrPfPointGetRWFrom6sensor(vrPoint *virtual_point, int sensor_num)
{
static	vrMatrix	mat_pos90x = { 1.0,0.0,0.0,0.0,  0.0,0.0,1.0,0.0,  0.0,-1.0,0.0,0.0,  0.0,0.0,0.0,1.0 };
#if 0
static	vrMatrix	mat_neg90x = { 1.0,0.0,0.0,0.0,  0.0,0.0,-1.0,0.0,  0.0,1.0,0.0,0.0,  0.0,0.0,0.0,1.0 };
#endif

	vrPointGetRWFrom6sensor(virtual_point, sensor_num);
	vrPointTransformByMatrix(virtual_point, virtual_point, &mat_pos90x);

	return virtual_point;
}


/**********************************************************************/
vrPoint *vrPfPointGetVWFromUser6sensor(vrPoint *virtual_point, int usernum, int sensor_num)
{
#if 0 /* for some reason, I can't declare these as static, even though it works in PF examples */
vrMatrix	*mat_neg90x = vrMatrixSetRotationIdNew(vrMatrixCreate(), VR_X, -90.0);	/* convert from Pf to OpenGL space */
vrMatrix	*mat_pos90x = vrMatrixSetRotationIdNew(vrMatrixCreate(), VR_X,  90.0);	/* convert from OpenGL to Pf space  (ogl2pf) */

vrPrintMatrix("neg", mat_neg90x);
vrPrintMatrix("pos", mat_pos90x);
#else
static	vrMatrix	mat_pos90x = { 1.0,0.0,0.0,0.0,  0.0,0.0,1.0,0.0,  0.0,-1.0,0.0,0.0,  0.0,0.0,0.0,1.0 };
#  if 0
	vrMatrix	*yo = vrMatrixSetRotationIdNew(vrMatrixCreate(), VR_X,  90.0);	/* convert from OpenGL to Pf space  (ogl2pf) */
	vrPrintMatrix("pos", &mat_pos90x);
	vrPrintMatrix("yo ", yo);
#  endif
#endif

	vrPointGetVWFromUser6sensor(virtual_point, usernum, sensor_num);
	vrPointTransformByMatrix(virtual_point, virtual_point, &mat_pos90x);

	return virtual_point;
}


/**********************************************************************/
vrVector *vrPfVectorGetRWFrom6sensorDir(vrVector *vector, int sensor_num, vrDirection direction)
{
static	vrMatrix	mat_pos90x = { 1.0,0.0,0.0,0.0,  0.0,0.0,1.0,0.0,  0.0,-1.0,0.0,0.0,  0.0,0.0,0.0,1.0 };

	vrVectorGetRWFrom6sensorDir(vector, sensor_num, direction);
	vrVectorTransformByMatrix(vector, vector, &mat_pos90x);

	return vector;
}


/**********************************************************************/
vrVector *vrPfVectorGetVWFromUser6sensorDir(vrVector *vector, int usernum, int sensor_num, vrDirection direction)
{
static	vrMatrix	mat_pos90x = { 1.0,0.0,0.0,0.0,  0.0,0.0,1.0,0.0,  0.0,-1.0,0.0,0.0,  0.0,0.0,0.0,1.0 };

	vrVectorGetVWFromUser6sensorDir(vector, usernum, sensor_num, direction);
	vrVectorTransformByMatrix(vector, vector, &mat_pos90x);

	return vector;
}


/**********************************************************************/
vrMatrix *vrPfMatrixGet6sensorValues(vrMatrix *mat, int input_num)
{
static	vrMatrix	mat_pos90x = { 1.0,0.0,0.0,0.0,  0.0,0.0,1.0,0.0,  0.0,-1.0,0.0,0.0,  0.0,0.0,0.0,1.0 };

	vrMatrixGet6sensorValues(mat, input_num);
	vrMatrixPreMult(mat, &mat_pos90x);

	return mat;
}


/**********************************************************************/
vrMatrix *vrPfMatrixGet6sensorValuesDirect(vrMatrix *mat, vr6sensor *input)
{
static	vrMatrix	mat_pos90x = { 1.0,0.0,0.0,0.0,  0.0,0.0,1.0,0.0,  0.0,-1.0,0.0,0.0,  0.0,0.0,0.0,1.0 };

	vrMatrixGet6sensorValuesDirect(mat, input);
	vrMatrixPreMult(mat, &mat_pos90x);

	return mat;
}


/**********************************************************************/
vrMatrix *vrPfMatrixGet6sensorValuesDirectNoLastUpdate(vrMatrix *mat, vr6sensor *input)
{
static	vrMatrix	mat_pos90x = { 1.0,0.0,0.0,0.0,  0.0,0.0,1.0,0.0,  0.0,-1.0,0.0,0.0,  0.0,0.0,0.0,1.0 };

	vrMatrixGet6sensorValuesDirectNoLastUpdate(mat, input);
	vrMatrixPreMult(mat, &mat_pos90x);

	return mat;
}


/**********************************************************************/
vrMatrix *vrPfMatrixGet6sensorRawValuesDirect(vrMatrix *mat, vr6sensor *input)
{
static	vrMatrix	mat_pos90x = { 1.0,0.0,0.0,0.0,  0.0,0.0,1.0,0.0,  0.0,-1.0,0.0,0.0,  0.0,0.0,0.0,1.0 };

	vrMatrixGet6sensorRawValuesDirect(mat, input);
	vrMatrixPreMult(mat, &mat_pos90x);

	return mat;
}


/**********************************************************************/
vrVectorf *vrPfVectorfGetRWFrom6sensorDir(vrVectorf *vector, int sensor_num, vrDirection direction)
{
static	vrMatrix	mat_pos90x = { 1.0,0.0,0.0,0.0,  0.0,0.0,1.0,0.0,  0.0,-1.0,0.0,0.0,  0.0,0.0,0.0,1.0 };
	vrVector	vectord;

	vrVectorfGetRWFrom6sensorDir(vector, sensor_num, direction);
	vectord.v[VR_X] = vector->v[VR_X];
	vectord.v[VR_Y] = vector->v[VR_Y];
	vectord.v[VR_Z] = vector->v[VR_Z];
	vrVectorTransformByMatrix(&vectord, &vectord, &mat_pos90x);
	vector->v[VR_X] = vectord.v[VR_X];
	vector->v[VR_Y] = vectord.v[VR_Y];
	vector->v[VR_Z] = vectord.v[VR_Z];

	return vector;
}


/**********************************************************************/
vrVectorf *vrPfVectorfGetVWFromUser6sensorDir(vrVectorf *vector, int user_num, int sensor_num, vrDirection direction)
{
static	vrMatrix	mat_pos90x = { 1.0,0.0,0.0,0.0,  0.0,0.0,1.0,0.0,  0.0,-1.0,0.0,0.0,  0.0,0.0,0.0,1.0 };
	vrVector	vectord;

	vrVectorfGetVWFromUser6sensorDir(vector, user_num, sensor_num, direction);
	vectord.v[VR_X] = vector->v[VR_X];
	vectord.v[VR_Y] = vector->v[VR_Y];
	vectord.v[VR_Z] = vector->v[VR_Z];
	vrVectorTransformByMatrix(&vectord, &vectord, &mat_pos90x);
	vector->v[VR_X] = vectord.v[VR_X];
	vector->v[VR_Y] = vectord.v[VR_Y];
	vector->v[VR_Z] = vectord.v[VR_Z];

	return vector;
}


/**********************************************************************/
vrPointf *vrPfPointfGetRWFrom6sensor(vrPointf *real_point, int sensor_num)
{
static	vrMatrix	mat_pos90x = { 1.0,0.0,0.0,0.0,  0.0,0.0,1.0,0.0,  0.0,-1.0,0.0,0.0,  0.0,0.0,0.0,1.0 };
	vrPoint		pointd;

	vrPointfGetRWFrom6sensor(real_point, sensor_num);
	pointd.v[VR_X] = real_point->v[VR_X];
	pointd.v[VR_Y] = real_point->v[VR_Y];
	pointd.v[VR_Z] = real_point->v[VR_Z];
	vrPointTransformByMatrix(&pointd, &pointd, &mat_pos90x);
	real_point->v[VR_X] = pointd.v[VR_X];
	real_point->v[VR_Y] = pointd.v[VR_Y];
	real_point->v[VR_Z] = pointd.v[VR_Z];

	return real_point;
}


/**********************************************************************/
vrPointf *vrPfPointfGetVWFromUser6sensor(vrPointf *virtual_point, int usernum, int sensor_num)
{
static	vrMatrix	mat_pos90x = { 1.0,0.0,0.0,0.0,  0.0,0.0,1.0,0.0,  0.0,-1.0,0.0,0.0,  0.0,0.0,0.0,1.0 };
	vrPoint		pointd;

	vrPointfGetVWFromUser6sensor(virtual_point, usernum, sensor_num);
	pointd.v[VR_X] = virtual_point->v[VR_X];
	pointd.v[VR_Y] = virtual_point->v[VR_Y];
	pointd.v[VR_Z] = virtual_point->v[VR_Z];
	vrPointTransformByMatrix(&pointd, &pointd, &mat_pos90x);
	virtual_point->v[VR_X] = pointd.v[VR_X];
	virtual_point->v[VR_Y] = pointd.v[VR_Y];
	virtual_point->v[VR_Z] = pointd.v[VR_Z];

	return virtual_point;
}


/**********************************************************************/
/* NOTE: unlike the other -Pf- versions of the input routines, this */
/*   one actually mimics the non-Pf version rather than calling the */
/*   non-Pf version and then converting the result.                 */
vrEuler *vrPfEulerGetRWFrom6sensor(vrEuler *euler, int sensor_num)
{
static	vrMatrix	mat_pos90x = { 1.0,0.0,0.0,0.0,  0.0,0.0,1.0,0.0,  0.0,-1.0,0.0,0.0,  0.0,0.0,0.0,1.0 };
	vrMatrix	matrix_data;
	vrMatrix	*mat = &matrix_data;

	vrMatrixGet6sensorValues(mat, sensor_num);
	return vrEulerSetFromMatrix(euler, vrMatrixPreMult(mat, &mat_pos90x));
}


/**********************************************************************/
vrEuler *vrPfEulerGetVWFromUser6sensor(vrEuler *euler, int usernum, int sensor_num)
{
static	vrMatrix	mat_pos90x = { 1.0,0.0,0.0,0.0,  0.0,0.0,1.0,0.0,  0.0,-1.0,0.0,0.0,  0.0,0.0,0.0,1.0 };
	vrMatrix	matrix_data;
	vrMatrix	*mat = &matrix_data;

	vrMatrixPreMult(vrMatrixGet6sensorValues(mat, sensor_num), vrContext->input->users[usernum]->vw2rw_xform);
	return vrEulerSetFromMatrix(euler, vrMatrixPreMult(mat, &mat_pos90x));
}

#endif /* GFX_PERFORMER } */


	/**************************************************************/
	/**************************************************************/


/**********************************************************************/
/* Copy all the current input values into a secondary storage location (also part of each */
/*   inputs data structure). ... */
/* TODO: it would be more efficient to use a local variable to store the pointer to each */
/*   input, as it is referenced 4 times per loop, and requires 2 dereferences and an     */
/*   addition.                                                                           */
void vrInputFreezeVisren(vrContextInfo *context)
{
	vrInputInfo	*vrInputs = context->input;
	int		count;

	/*************************/
	/* freeze all the 2-ways */
	for (count = 0; count < vrInputs->num_2ways; count++) {
		if (vrInputs->switch2[count] != NULL) {
			vrLockWriteSet(vrInputs->switch2[count]->lock);
			vrInputs->switch2[count]->visren_value = vrInputs->switch2[count]->value;
			vrLockWriteRelease(vrInputs->switch2[count]->lock);
		}
	}

	/*************************/
	/* freeze all the N-ways */
	for (count = 0; count < vrInputs->num_Nways; count++) {
		if (vrInputs->switchN[count] != NULL) {
			vrLockWriteSet(vrInputs->switchN[count]->lock);
			vrInputs->switchN[count]->visren_value = vrInputs->switchN[count]->value;
			vrLockWriteRelease(vrInputs->switchN[count]->lock);
		}
	}

	/****************************/
	/* freeze all the valuators */
	for (count = 0; count < vrInputs->num_valuators; count++) {
		if (vrInputs->valuator[count] != NULL) {
			vrLockWriteSet(vrInputs->valuator[count]->lock);
			vrInputs->valuator[count]->visren_value = vrInputs->valuator[count]->value;
			vrLockWriteRelease(vrInputs->valuator[count]->lock);
		}
	}

	/****************************/
	/* freeze all the 6-sensors */
	for (count = 0; count < vrInputs->num_6sensors; count++) {
		if (vrInputs->sensor6[count] != NULL) {
			vrLockWriteSet(vrInputs->sensor6[count]->lock);
			*(vrInputs->sensor6[count]->visren_position) = *(vrInputs->sensor6[count]->position);
			vrLockWriteRelease(vrInputs->sensor6[count]->lock);
		}
	}

	/****************************/
	/* freeze all the N-sensors */
	for (count = 0; count < vrInputs->num_Nsensors; count++) {
		if (vrInputs->sensorN[count] != NULL) {
			vrLockWriteSet(vrInputs->sensorN[count]->lock);
			*(vrInputs->sensorN[count]->visren_values) = *(vrInputs->sensorN[count]->values);
			vrLockWriteRelease(vrInputs->sensorN[count]->lock);
		}
	}
}


	/**************************************************************/
	/**************************************************************/



/**********************************************************************/
int vrInputMakeChecksum(vrGenericInput *input)
{
#ifdef UNPICKY_COMPILER
	int	bytes_to_check;
#endif

	/* TODO: I didn't get around to implementing this before finding the bug */
	/*   that made me want to have this -- not sure if we will implement it. */

	return -10;	/* return a dummy value for now */
}


/**********************************************************************/
int vrInputCheckChecksum(vrGenericInput *input)
{
#ifdef UNPICKY_COMPILER
	int	bytes_to_check;
#endif

	/* TODO: I didn't get around to implementing this before finding the bug */
	/*   that made me want to have this -- not sure if we will implement it. */

	return 0;
}


/**************************************************************************/
/* The vrFprintInput() function prints information about the entire input */
/*   structure of FreeVR -- the vrInputInfo structure, which has pointers */
/*   to all aspects of the running FreeVR input system.                   */
void vrFprintInput(FILE *file, vrInputInfo *vrInputs, vrPrintStyle style)
{
	int		count;

	if (vrInputs == NULL) {
		vrErrPrintf("vrFprintInput(): " RED_TEXT "memory argument is NULL!\n" NORM_TEXT);
		return;
	}

	if (vrInputs->object_type != VROBJECT_INPUTINFO) {
		vrErrPrintf("vrFprintInput(): " RED_TEXT "memory argument does not point to the vrInputInfo structure!\n" NORM_TEXT);
		return;
	}

	switch(style) {
	case none:
		break;

	case brief:
		vrFprintf(file, "==================================================\n");
		vrFprintf(file, "vrInputs at %#p\n", vrInputs);
#if 0 /* TODO: when Queue stuff is implemented */
		vrFprintf(file, "\tQueue at %#p\n", vrInputs->queue);
#endif
		vrFprintf(file, "%d input devices\n", vrInputs->num_input_devices);
		vrFprintf(file, "%d users\n", vrInputs->num_users);
		vrFprintf(file, "%d props\n", vrInputs->num_props);
		vrFprintf(file, "%d 2-way switches\n", vrInputs->num_2ways);
		vrFprintf(file, "%d N-way switches\n", vrInputs->num_Nways);
		vrFprintf(file, "%d valuators\n", vrInputs->num_valuators);
		vrFprintf(file, "%d 6-sensors\n", vrInputs->num_6sensors);
		vrFprintf(file, "%d N-sensors\n", vrInputs->num_Nsensors);
		vrFprintf(file, "%d controls\n", vrInputs->num_controls);
		vrFprintf(file, "==================================================\n");
		break;

	case everything:
		/* here, this is the same as verbose -- it also means more info */
		/* during the read phase of configuration.                      */
		/* TODO: decide if it should be more.                           */

	case verbose:
		vrFprintf(file, "==================================================\n");
		vrFprintf(file, "vrInputs at %#p\n", vrInputs);
#if 0 /* TODO: when Queue stuff is implemented */
		vrFprintf(file, "\tQueue at %#p\n", vrInputs->queue);
#endif
		vrFprintf(file, "input map name = \"%s\"\n", vrInputs->input_map_name);

		vrFprintf(file, "%d input devices:\n", vrInputs->num_input_devices);
		for (count = 0; count < vrInputs->num_input_devices; count++) {
			if (vrInputs->input_devices[count] == NULL) {
				vrFprintf(file, "\tdevice[%d] is NULL\n", count);
			} else {
				vrFprintf(file, "\tdevice[%d] at %#p id:%d named: \"%s\" (%s)\n",
					count,
					vrInputs->input_devices[count],
					vrInputs->input_devices[count]->id,
					vrInputs->input_devices[count]->name,
					(vrInputs->input_devices[count]->operating ? "operating" : RED_TEXT "NOT OPERATING" NORM_TEXT) );
			}
		}
		vrFprintf(file, "%d users:\n", vrInputs->num_users);
		for (count = 0; count < vrInputs->num_users; count++) {
			if (vrInputs->users[count] == NULL) {
				vrFprintf(file, "\tuser[%d] is NULL\n", count);
			} else {
				vrFprintf(file, "\tuser[%d] at %#p, named \"%s\", head sensor is \"%s\"\n",
					count,
					vrInputs->users[count],
					vrInputs->users[count]->name,
					(vrInputs->users[count]->head->my_object != NULL ?
						vrInputs->users[count]->head->my_object->name : "-no object-"));
			}
		}
		vrFprintf(file, "%d props:\n", vrInputs->num_props);
		for (count = 0; count < vrInputs->num_props; count++) {
			if (vrInputs->props[count] == NULL) {
				vrFprintf(file, "\tprop[%d] is NULL\n", count);
			} else {
				vrFprintf(file, "\tprop[%d] at %#p, named \"%s\"\n",
					count,
					vrInputs->props[count],
					vrInputs->props[count]->name);
			}
		}
		vrFprintf(file, "%d 2-way switches (at %#p):\n", vrInputs->num_2ways, vrInputs->switch2);
		for (count = 0; count < vrInputs->num_2ways; count++) {
			if (vrInputs->switch2[count] == NULL) {
				vrFprintf(file, "\t2-way[%d] is NULL\n", count);
			} else {
				vrFprintf(file, "\t2-way[%d] at %#p, from \"%s\":\"%s\" (%d:%d) -- %d\n",
					count,
					 vrInputs->switch2[count],
					(vrInputs->switch2[count]->my_device != NULL ? vrInputs->switch2[count]->my_device->name : "-"),
					(vrInputs->switch2[count]->my_object != NULL ? vrInputs->switch2[count]->my_object->name : "-"),
					(vrInputs->switch2[count]->my_device != NULL ? vrInputs->switch2[count]->my_device->id : -1),
					(vrInputs->switch2[count]->my_object != NULL ? vrInputs->switch2[count]->my_object->id : -1),
					 vrInputs->switch2[count]->value);
			}
		}
		vrFprintf(file, "%d N-way switches (at %#p):\n", vrInputs->num_Nways, vrInputs->switchN);
		for (count = 0; count < vrInputs->num_Nways; count++) {
			if (vrInputs->switchN[count] == NULL) {
				vrFprintf(file, "\tN-way[%d] is NULL\n", count);
			} else {
				vrFprintf(file, "\tN-way[%d] at %#p, from \"%s\":\"%s\" (%d:%d) -- %d\n",
					count,
					 vrInputs->switchN[count],
					(vrInputs->switchN[count]->my_device != NULL ? vrInputs->switchN[count]->my_device->name : "-"),
					(vrInputs->switchN[count]->my_object != NULL ? vrInputs->switchN[count]->my_object->name : "-"),
					(vrInputs->switchN[count]->my_device != NULL ? vrInputs->switchN[count]->my_device->id : -1),
					(vrInputs->switchN[count]->my_object != NULL ? vrInputs->switchN[count]->my_object->id : -1),
					 vrInputs->switchN[count]->value);
			}
		}
		vrFprintf(file, "%d valuators (at %#p):\n", vrInputs->num_valuators, vrInputs->valuator);
		for (count = 0; count < vrInputs->num_valuators; count++) {
			if (vrInputs->valuator[count] == NULL) {
				vrFprintf(file, "\tvaluator[%d] is NULL\n", count);
			} else {
				vrFprintf(file, "\tvaluator[%d] at %#p, from \"%s\":\"%s\" (%d:%d) -- %.2f\n",
					count,
					 vrInputs->valuator[count],
					(vrInputs->valuator[count]->my_device != NULL ? vrInputs->valuator[count]->my_device->name : "-"),
					(vrInputs->valuator[count]->my_object != NULL ? vrInputs->valuator[count]->my_object->name : "-"),
					(vrInputs->valuator[count]->my_device != NULL ? vrInputs->valuator[count]->my_device->id : -1),
					(vrInputs->valuator[count]->my_object != NULL ? vrInputs->valuator[count]->my_object->id : -1),
					 vrInputs->valuator[count]->value);
			}
		}
		vrFprintf(file, "%d 6-sensors (at %#p):\n", vrInputs->num_6sensors, vrInputs->sensor6);
		for (count = 0; count < vrInputs->num_6sensors; count++) {
			if (vrInputs->sensor6[count] == NULL) {
				vrFprintf(file, "\t6-sensor[%d] is NULL\n", count);
			} else {
				vrFprintf(file, "\t6-sensor[%d] at %#p, from \"%s\":\"%s\" (%d:%d) -- [tr: %.2f %.2f %.2f]\n",
					count,
					 vrInputs->sensor6[count],
					(vrInputs->sensor6[count]->my_device != NULL ? vrInputs->sensor6[count]->my_device->name : "-"),
					(vrInputs->sensor6[count]->my_object != NULL ? vrInputs->sensor6[count]->my_object->name : "-"),
					(vrInputs->sensor6[count]->my_device != NULL ? vrInputs->sensor6[count]->my_device->id : -1),
					(vrInputs->sensor6[count]->my_object != NULL ? vrInputs->sensor6[count]->my_object->id : -1),
					 VRMAT_ROWCOL(vrInputs->sensor6[count]->position, VR_X, VR_W),
					 VRMAT_ROWCOL(vrInputs->sensor6[count]->position, VR_Y, VR_W),
					 VRMAT_ROWCOL(vrInputs->sensor6[count]->position, VR_Z, VR_W));
			}
		}
		vrFprintf(file, "%d N-sensors (at %#p):\n", vrInputs->num_Nsensors, vrInputs->sensorN);
		for (count = 0; count < vrInputs->num_Nsensors; count++) {
			if (vrInputs->sensorN[count] == NULL) {
				vrFprintf(file, "\tN-sensor[%d] is NULL\n", count);
			} else {
				vrFprintf(file, "\tN-sensor[%d] at %#p, from \"%s\":\"%s\" (%d:%d)\n",
					count,
					 vrInputs->sensorN[count],
					(vrInputs->sensorN[count]->my_device != NULL ? vrInputs->sensorN[count]->my_device->name : "-"),
					(vrInputs->sensorN[count]->my_object != NULL ? vrInputs->sensorN[count]->my_object->name : "-"),
					(vrInputs->sensorN[count]->my_device != NULL ? vrInputs->sensorN[count]->my_device->id : -1),
					(vrInputs->sensorN[count]->my_object != NULL ? vrInputs->sensorN[count]->my_object->id : -1));
			}
		}
		vrFprintf(file, "%d controls (at %#p):\n", vrInputs->num_controls, vrInputs->control);
		for (count = 0; count < vrInputs->num_controls; count++) {
			if (vrInputs->control[count] == NULL) {
				vrFprintf(file, "\tcontrol[%d] is NULL\n", count);
			} else {
				vrFprintf(file, "\tcontrol[%d] at %#p, from \"%s\":\"%s\" (%d:%d)\n",
					count,
					 vrInputs->control[count],
					(vrInputs->control[count]->my_device != NULL ? vrInputs->control[count]->my_device->name : "-"),
					(vrInputs->control[count]->my_object != NULL ? vrInputs->control[count]->my_object->name : "-"),
					(vrInputs->control[count]->my_device != NULL ? vrInputs->control[count]->my_device->id : -1),
					(vrInputs->control[count]->my_object != NULL ? vrInputs->control[count]->my_object->id : -1));
			}
		}
		vrFprintf(file, "==================================================\n");
		break;

	case file_format:
		/* TODO: not yet implemented -- output that can be reread. */
	case one_line:
		/* TODO: not yet implemented -- very brief output */
	case machine:
		/* TODO: not yet implemented -- output easily machine parsed */
		vrFprintf(file, "TODO: put special version of input info here\n");
		break;
	}
}


/******************************************************************************/
/* The vrSprintInputUI() function prints into a given character string array  */
/*   information about the FreeVR input system, as further augmented by the   */
/*   application program through the assignment of user-interface description */
/*   strings to particular inputs.                                            */
/*                                                                            */
/* We are currently ignoring the "style" argument, but we may want to make    */
/*   use of this in the future.                                               */
/*                                                                            */
/* It returns a copy of the pointer to the string.                            */
/*                                                                            */
/* NOTE: a warning about the use of snprintf() -- the return value is the     */
/*   number of bytes that ** would have been written ** were there enough     */
/*   space in the string -- which forces us to check whether we've hit the    */
/*   limit or not!  I think this is a bad return value, but nothing I can     */
/*   do about that!                                                           */
char *vrSprintInputUI(char *str, vrInputInfo *vrInputs, int str_size, vrPrintStyle style)
{
	int	bytes_remaining = str_size;
	int	bytes_used = 0;
	int	bytes;			/* return result from snprintf() calls -- usually # of bytes written */
	int	first_of_type;		/* has the first input of the current type been output yet? */
	int	count;
	vrInput	*input_object;

	str[0] = '\0';

	/********************/
	/* Application name */
	/********************/
	if (vrInputs->context->name != NULL) {
		/* NOTE: add an extra '\n' if there is no 'authors' line (and the space is necessary because of the strtok() method of string parsing) */
		bytes = snprintf(str, bytes_remaining, "Program: %s%s\n", vrInputs->context->name, (vrInputs->context->authors == NULL ? "\n " : ""));
		bytes_used += bytes;
		bytes_remaining -= bytes;
		if ((bytes < 1) || (bytes_remaining < 1)) {
			str[str_size] = '\0';		/* for safety */
			return str;
		}
	}

	/***********************/
	/* Application authors */
	/***********************/
	if (vrInputs->context->authors != NULL) {
		bytes = snprintf(&str[bytes_used], bytes_remaining, "Author(s): %s\n", vrInputs->context->authors);
		bytes_used += bytes;
		bytes_remaining -= bytes;
		if ((bytes < 1) || (bytes_remaining < 1)) {
			str[str_size] = '\0';		/* for safety */
			return str;
		}
	}


	/******************/
	/* FreeVR version */
	/******************/
	if (vrInputs->context->authors != NULL) {
		bytes = snprintf(&str[bytes_used], bytes_remaining, "FreeVR version: %s\n \n", &(vrInputs->context->version[15]));	/* adding a colon in the middle */
		bytes_used += bytes;
		bytes_remaining -= bytes;
		if ((bytes < 1) || (bytes_remaining < 1)) {
			str[str_size] = '\0';		/* for safety */
			return str;
		}
	}


	/*****************************/
	/*** 2-way's (aka buttons) ***/
	first_of_type = 1;
	for (count = 0; count < vrInputs->num_2ways; count++) {
		if (vrInputs->switch2[count] != NULL) {
			input_object = vrInputs->switch2[count]->my_object;

			if ((input_object != NULL) && (input_object->desc_ui[0] != '\0')) {
				/* print the section header just before the first button */
				if (first_of_type) {
					bytes = snprintf(&str[bytes_used], bytes_remaining, "Buttons:\n");
					bytes_used += bytes;
					bytes_remaining -= bytes;
					if ((bytes < 1) || (bytes_remaining < 1)) {
						str[str_size] = '\0';		/* for safety */
						return str;
					}

					first_of_type = 0;
				}

				/* now print the button information */
				bytes = snprintf(&str[bytes_used], bytes_remaining, "  button[%d] -- \"%s\" performs: %s\n",
						count,
						input_object->name,
						input_object->desc_ui);

				bytes_used += bytes;
				bytes_remaining -= bytes;
				if ((bytes < 1) || (bytes_remaining < 1)) {
					str[str_size] = '\0';		/* for safety */
					return str;
				}
			}
		}
	}

	/***************/
	/*** N-way's ***/
	first_of_type = 1;
	for (count = 0; count < vrInputs->num_Nways; count++) {
		if (vrInputs->switchN[count] != NULL) {
			input_object = vrInputs->switchN[count]->my_object;

			if ((input_object != NULL) && (input_object->desc_ui[0] != '\0')) {
				/* print the section header just before the first button */
				if (first_of_type) {
					bytes = snprintf(&str[bytes_used], bytes_remaining, "N-way switches:\n");
					bytes_used += bytes;
					bytes_remaining -= bytes;
					if ((bytes < 1) || (bytes_remaining < 1)) {
						str[str_size] = '\0';		/* for safety */
						return str;
					}

					first_of_type = 0;
				}

				/* now print the button information */
				bytes = snprintf(&str[bytes_used], bytes_remaining, "  switch[%d] -- \"%s\" performs: %s\n",
						count,
						input_object->name,
						input_object->desc_ui);

				bytes_used += bytes;
				bytes_remaining -= bytes;
				if ((bytes < 1) || (bytes_remaining < 1)) {
					str[str_size] = '\0';		/* for safety */
					return str;
				}
			}
		}
	}

	/*****************/
	/*** Valuators ***/
	first_of_type = 1;
	for (count = 0; count < vrInputs->num_valuators; count++) {
		if (vrInputs->valuator[count] != NULL) {
			input_object = vrInputs->valuator[count]->my_object;

			if ((input_object != NULL) && (input_object->desc_ui[0] != '\0')) {
				/* print the section header just before the first button */
				if (first_of_type) {
					bytes = snprintf(&str[bytes_used], bytes_remaining, "Valuators:\n");
					bytes_used += bytes;
					bytes_remaining -= bytes;
					if ((bytes < 1) || (bytes_remaining < 1)) {
						str[str_size] = '\0';		/* for safety */
						return str;
					}

					first_of_type = 0;
				}

				/* now print the button information */
				bytes = snprintf(&str[bytes_used], bytes_remaining, "  valuator[%d] -- \"%s\" performs: %s\n",
						count,
						input_object->name,
						input_object->desc_ui);

				bytes_used += bytes;
				bytes_remaining -= bytes;
				if ((bytes < 1) || (bytes_remaining < 1)) {
					str[str_size] = '\0';		/* for safety */
					return str;
				}
			}
		}
	}

	/*****************/
	/*** 6-Sensors ***/
	first_of_type = 1;
	for (count = 0; count < vrInputs->num_6sensors; count++) {
		if (vrInputs->sensor6[count] != NULL) {
			input_object = vrInputs->sensor6[count]->my_object;

			if ((input_object != NULL) && (input_object->desc_ui[0] != '\0')) {
				/* print the section header just before the first button */
				if (first_of_type) {
					bytes = snprintf(&str[bytes_used], bytes_remaining, "6DOF sensors:\n");
					bytes_used += bytes;
					bytes_remaining -= bytes;
					if ((bytes < 1) || (bytes_remaining < 1)) {
						str[str_size] = '\0';		/* for safety */
						return str;
					}

					first_of_type = 0;
				}

				/* now print the button information */
				bytes = snprintf(&str[bytes_used], bytes_remaining, "  6-sensor[%d] -- \"%s\" performs: %s\n",
						count,
						input_object->name,
						input_object->desc_ui);

				bytes_used += bytes;
				bytes_remaining -= bytes;
				if ((bytes < 1) || (bytes_remaining < 1)) {
					str[str_size] = '\0';		/* for safety */
					return str;
				}
			}
		}
	}

	/*****************/
	/*** N-Sensors ***/
	first_of_type = 1;
	for (count = 0; count < vrInputs->num_Nsensors; count++) {
		if (vrInputs->sensorN[count] != NULL) {
			input_object = vrInputs->sensorN[count]->my_object;

			if ((input_object != NULL) && (input_object->desc_ui[0] != '\0')) {
				/* print the section header just before the first button */
				if (first_of_type) {
					bytes = snprintf(&str[bytes_used], bytes_remaining, "N-sensors:\n");
					bytes_used += bytes;
					bytes_remaining -= bytes;
					if ((bytes < 1) || (bytes_remaining < 1)) {
						str[str_size] = '\0';		/* for safety */
						return str;
					}

					first_of_type = 0;
				}

				/* now print the button information */
				bytes = snprintf(&str[bytes_used], bytes_remaining, "  N-sensor[%d] -- \"%s\" performs: %s\n",
						count,
						input_object->name,
						input_object->desc_ui);

				bytes_used += bytes;
				bytes_remaining -= bytes;
				if ((bytes < 1) || (bytes_remaining < 1)) {
					str[str_size] = '\0';		/* for safety */
					return str;
				}
			}
		}
	}

	/*********************************/
	/* Application Extra-Information */
	/*********************************/
	if (vrInputs->context->extra_info != NULL) {
		bytes = snprintf(&str[bytes_used], bytes_remaining, " \nAdditional information:\n  %s\n", vrInputs->context->extra_info);
		bytes_used += bytes;
		bytes_remaining -= bytes;
		if ((bytes < 1) || (bytes_remaining < 1)) {
			str[str_size] = '\0';		/* for safety */
			return str;
		}
	}

	/**********************************/
	/* Application Status Information */
	/**********************************/
	if (vrInputs->context->status_info != NULL) {
		bytes = snprintf(&str[bytes_used], bytes_remaining, " \nApplication status:\n  %s\n", vrInputs->context->status_info);
		bytes_used += bytes;
		bytes_remaining -= bytes;
		if ((bytes < 1) || (bytes_remaining < 1)) {
			str[str_size] = '\0';		/* for safety */
			return str;
		}
	}

	return str;
}

