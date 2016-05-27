/* ======================================================================
 *
 *  CCCCC          vr_output.shmemd.c
 * CC   CC         Author(s): Bill Sherman
 * CC              Created: April 11, 2007
 * CC   CC         Last Modified: April 11, 2007
 *  CCCCC
 *
 * Code file for FreeVR outputs to generic Shared Memory device.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/*************************************************************************

USAGE:
	Outputs are specified with ... (to be determined -- hard coded for now, I'm in a hurry)

	Controls are specified in the freevrrc file:
		control "<control option>" = "2switch(...)";

	Here are the available control options for FreeVR:
	  :-(	"print_help" -- print info on how to use the output device
	  :-(	"system_pause_toggle" -- toggle the system pause flag
	  :-(	"print_context" -- print the overall FreeVR context data structure (for debugging)
	  :-(	"print_config" -- print the overall FreeVR config data structure (for debugging)
	  :-(	"print_input" -- print the overall FreeVR input data structure (for debugging)
	  :-(	"print_struct" -- print the internal Shared Memory data structure (for debugging)
	  :-(	"print_values" -- print just the input values of the memory


	Here are the FreeVR configuration options for the generic Shared Memory Device:
		"key" - set the shared memory location key identifier.
	  :-(	"scaleTrans" - scale the incoming location values (for unit conversion)

HISTORY:

	11 April, 2007 -- Bill Sherman began writing the initial version.
		Code is based on the shared memory input routines, but since
		this is the first (perhaps only) device of this type, there
		is much work to be done for proper configurability (ie. for
		the moment, I'm going to hard code stuff).  NOTE: the type
		is defined as an input device (at least for now), so that it
		can be included in the input process, and as part of the input
		sequence, write data to a shared memory segment.

		In this version, dual shared memory segments are used to
		accommodate systems that separate buttons and valuators
		from position sensors.  In the future ...

		(This was done quickly to get some old but cool applications
		running in preparation for a visit from Gonzo the Great.)

	15 September 2013 (Bill Sherman)
		Changed the "0x%p" format to the improved "%#p" format.
		[NOTE: I'm sure there were other cleanups between 2007 & 2013.]

TODO:
	- Need to make it configurable.

	- Once it is configurable, the example operation should be to
		have two of these "outputs" writing to separate shared
		memory segments.

*************************************************************************/
#include <stdio.h>
#include <string.h>		/* for strchr */

#include "vr_input.h"
#include "vr_input.opts.h"
#include "vr_shmem.h"
#include "vr_debug.h"

#if 0 /* this was to make dso work under linux */
#  define vrErrPrintf printf
#  define vrPrintf printf
#endif


	/***************************************************/
	/*** definitions for interfacing with the device ***/
	/***                                             ***/

/* list of types that can be found in the shared memory */
typedef enum {
		MEM_UNKNOWN = -1,
		MEM_CHAR,
		MEM_INT,
		MEM_LONG,
		MEM_FLOAT,
		MEM_DOUBLE,
		MEM_VREULER,
		MEM_VISBOXEULER,
		MEM_VISBOXTRANS,
		MEM_VRMATRIX,
		MEM_EULERDOUBLE,
		MEM_MATRIXFLOAT
	} _MemoryType;

/* These types are not used in FreeVR, but may be used by other */
/*   programs writing into shared memory.                       */
typedef struct {
		double	t[3];	/* translational component */
		double	r[3];	/* rotational component    */
	} vrEulerDouble;

typedef struct {
		float	v[16];
	} vrMatrixFloat;


/****************************************************************/
/*** auxiliary structure of the current data from the device. ***/
typedef struct {
		/* this is for printing extra info in _ShmemdPrintValues() */
		vrInputDevice	*devinfo;

		/* these are for shmemd interfacing */
		int		shmid;
		key_t		shmkey;
		key_t		shmkey_cont;
		int		open;
		char		*data;			/* pointer to the amorphous shared memory space */
		char		*data_cont;		/* pointer to the other amorphous shared memory space */
		int		segment_size;		/* size of the shared memory segment */


		/* TODO: most of these options were part of vr_input.shmemd.c, I suspect most can be deleted */

		/* these are pointers to the FreeVR inputs */
		int		num_buttons;
		vr2switch	**button_inputs;
		int		*button_values;		/* array of current values */
		int		*button_offsets;	/* array of offsets into the amorphous shared memory */
		_MemoryType	*button_types;		/* type of data to be found at the given offset */
		int		*button_dummy;		/* array of flags whether button data coming from dummy values */

		int		num_valuators;
		vrValuator	**valuator_inputs;
		float		*valuator_values;	/* array of current values */
		int		*valuator_offsets;	/* array of offsets into the amorphous shared memory */
		_MemoryType	*valuator_types;	/* type of data to be found at the given offset */
		int		*valuator_dummy;	/* array of flags whether valuator data coming from dummy values */

		int		num_6sensors;
		vr6sensor	**sensor6_inputs;
		vrMatrix	*sensor6_values;	/* array of current values */
		int		*sensor6_offsets;	/* array of offsets into the amorphous shared memory */
		_MemoryType	*sensor6_types;		/* type of data to be found at the given offset */
		int		*sensor6_dummy;		/* array of flags whether 6sensor data coming from dummy values */

		/* these are for storing dummy values when too many inputs are requested */
		int		dummy_button_value;
		int		dummy_switch_value;
		float		dummy_valuator_value;
		vrMatrix	dummy_6sensor_value;

	} _ShmemdPrivateInfo;



	/*********************************************/
	/*** General NON public interface routines ***/
	/*********************************************/

/************************************************************/
static int _MemoryTypeSize(_MemoryType type)
{
	switch (type) {
	case MEM_CHAR:
		return sizeof(char);
	case MEM_INT:
		return sizeof(int);
	case MEM_LONG:
		return sizeof(long);
	case MEM_FLOAT:
		return sizeof(float);
	case MEM_DOUBLE:
		return sizeof(double);
	case MEM_VREULER:
		return sizeof(vrEuler);
	case MEM_VISBOXEULER:
		return sizeof(vrEuler);
	case MEM_VISBOXTRANS:
		return sizeof(vrEuler);
	case MEM_VRMATRIX:
		return sizeof(vrMatrix);
	case MEM_EULERDOUBLE:
		return sizeof(vrEulerDouble);
	case MEM_MATRIXFLOAT:
		return sizeof(vrMatrixFloat);
	}

	return 0;
}


/************************************************************/
static char *_MemoryTypeName(_MemoryType type)
{
	switch (type) {
	case MEM_CHAR:
		return "char";
	case MEM_INT:
		return "int";
	case MEM_LONG:
		return "long";
	case MEM_FLOAT:
		return "float";
	case MEM_DOUBLE:
		return "double";
	case MEM_VREULER:
		return "vrEuler";
	case MEM_VISBOXEULER:
		return "visboxEuler";
	case MEM_VISBOXTRANS:
		return "visboxTrans";
	case MEM_VRMATRIX:
		return "vrMatrix";
	case MEM_EULERDOUBLE:
		return "vrEulerDouble";
	case MEM_MATRIXFLOAT:
		return "vrMatrixFloat";
	}

	return "unknown";
}


/************************************************************/
static _MemoryType _MemoryValue(char *name)
{
	if (!strcasecmp(name, "default"))		return MEM_INT;
	else if (!strcasecmp(name, "char"))		return MEM_CHAR;
	else if (!strcasecmp(name, "int"))		return MEM_INT;
	else if (!strcasecmp(name, "long"))		return MEM_LONG;
	else if (!strcasecmp(name, "float"))		return MEM_FLOAT;
	else if (!strcasecmp(name, "double"))		return MEM_DOUBLE;
	else if (!strcasecmp(name, "vreuler"))		return MEM_VREULER;
	else if (!strcasecmp(name, "visboxeuler"))	return MEM_VISBOXEULER;
	else if (!strcasecmp(name, "visboxtrans"))	return MEM_VISBOXTRANS;
	else if (!strcasecmp(name, "vrmatrix"))		return MEM_VRMATRIX;
	else if (!strcasecmp(name, "vreulerfloat"))	return MEM_VREULER;
	else if (!strcasecmp(name, "vrmatrixdouble"))	return MEM_VRMATRIX;
	else if (!strcasecmp(name, "vreulerdouble"))	return MEM_EULERDOUBLE;
	else if (!strcasecmp(name, "vrmatrixfloat"))	return MEM_MATRIXFLOAT;
	else {
		vrErrPrintf("Unknown memory type '%s' using \"int\" type\n", name);
		return MEM_UNKNOWN;
	}

	/* Can't get to this statement */
	return MEM_UNKNOWN;
}


/**************************************************************************/
static vrMatrix *_MatrixConvertFloatToDouble(vrMatrix *dst, vrMatrixFloat *src)
{
	dst->v[ 0] = src->v[ 0];
	dst->v[ 1] = src->v[ 1];
	dst->v[ 2] = src->v[ 2];
	dst->v[ 3] = src->v[ 3];
	dst->v[ 4] = src->v[ 4];
	dst->v[ 5] = src->v[ 5];
	dst->v[ 6] = src->v[ 6];
	dst->v[ 7] = src->v[ 7];
	dst->v[ 8] = src->v[ 8];
	dst->v[ 9] = src->v[ 9];
	dst->v[10] = src->v[10];
	dst->v[11] = src->v[11];
	dst->v[12] = src->v[12];
	dst->v[13] = src->v[13];
	dst->v[14] = src->v[14];
	dst->v[15] = src->v[15];

	return dst;
}


/**************************************************************************/
static vrEuler *_EulerConvertDoubleToFloat(vrEuler *dst, vrEulerDouble *src)
{
	dst->t[VR_X] = src->t[VR_X];
	dst->t[VR_Y] = src->t[VR_Y];
	dst->t[VR_Z] = src->t[VR_Z];
	dst->r[VR_AZIM] = src->r[VR_AZIM];
	dst->r[VR_ELEV] = src->r[VR_ELEV];
	dst->r[VR_ROLL] = src->r[VR_ROLL];

	return dst;
}


/**************************************************************************/
static void _ShmemdInitializeStruct(_ShmemdPrivateInfo *aux)
{
	aux->dummy_button_value = 0;
	aux->dummy_switch_value = 0;
	aux->dummy_valuator_value = 0.0;
	vrMatrixSetTranslation3d(&aux->dummy_6sensor_value, 0.0, 5.0, 0.0);
}


/**************************************************************************/
static void _ShmemdPrintStruct(FILE *file, _ShmemdPrivateInfo *aux, vrPrintStyle style)
{
	int	count;

	vrFprintf(file, "Shared Memory device internal structure (%#p):\n", aux);
	vrFprintf(file, "\r\tshmemd open = %d\n", aux->open);
	vrFprintf(file, "\r\tshmem id = %d (key = %d, key_cont = %d)\n", aux->shmid, aux->shmkey, aux->shmkey_cont);

	vrFprintf(file, "\r\t%d buttons:\n", aux->num_buttons);
	vrFprintf(file, "\r\tbutton_inputs = %#p, button_values = %#p, button_offsets = %#p, button_types = %#p\n",
		aux->button_inputs, aux->button_values, aux->button_offsets, aux->button_types);
	for (count = 0; count < aux->num_buttons; count++) {
		vrFprintf(file, "\r\t\tbutton %d: value = %d, offset = %d, type = %d, input = %#p (type = %d)\n",
			count,
			aux->button_values[count],
			aux->button_offsets[count],
			aux->button_types[count],
			aux->button_inputs[count],
			aux->button_inputs[count]->input_type);
	}

	vrFprintf(file, "\r\t%d valuators:\n", aux->num_valuators);
	vrFprintf(file, "\r\tvaluator_inputs = %#p, valuator_values = %#p, valuator_offsets = %#p, valuator_types = %#p\n",
		aux->valuator_inputs, aux->valuator_values, aux->valuator_offsets, aux->valuator_types);
	for (count = 0; count < aux->num_valuators; count++) {
		vrFprintf(file, "\r\t\tvaluator %d: value = %.2f, offset = %d, type = %d, input = %#p (type = %d)\n",
			count,
			aux->valuator_values[count],
			aux->valuator_offsets[count],
			aux->valuator_types[count],
			aux->valuator_inputs[count],
			aux->valuator_inputs[count]->input_type);
	}

	vrFprintf(file, "\r\t%d 6-sensors:\n", aux->num_6sensors);
	vrFprintf(file, "\r\t6sensor_inputs = %#p, 6sensor_values = %#p, 6sensor_offsets = %#p, 6sensor_types = %#p\n",
		aux->sensor6_inputs, aux->sensor6_values, aux->sensor6_offsets, aux->sensor6_types);
	for (count = 0; count < aux->num_6sensors; count++) {
		vrFprintf(file, "\r\t\t6-sensor %d: value = [... (TODO!)], offset = %d, type = %d, input = %#p (type = %d)\n",
			count,
		/* TODO: put matrix values here  -- perhaps convert to quat/trans */
			aux->sensor6_offsets[count],
			aux->sensor6_types[count],
			aux->sensor6_inputs[count],
			aux->sensor6_inputs[count]->input_type);
	}
}


/**************************************************************************/
static void _ShmemdPrintHelp(FILE *file, _ShmemdPrivateInfo *aux)
{
	vrFprintf(file, BOLD_TEXT "Sorry, Shmemd -- print_help control not yet implemented.\n" NORM_TEXT);
}


/**************************************************************************/
static void _ShmemdPrintValues(FILE *file, _ShmemdPrivateInfo *aux)
{
	int	count;

	vrFprintf(file, "Shared Memory output device values:\n");

	vrFprintf(file, "\r\t%d buttons\n", aux->num_buttons);
	for (count = 0; count < aux->num_buttons; count++) {
		if (aux->button_inputs[count]->input_type == VRINPUT_BINARY) {
			vrFprintf(file, "\r\t\tbutton %d ('%s') [offset = %d, type = %d]: = %d\n",
				count,
				aux->devinfo->switch2[count].my_object->name,
				aux->button_offsets[count],
				aux->button_types[count],
				aux->button_values[count]);
		} else {
			vrFprintf(file, "\r\t\tbutton %d ('%s') [offset = %d, type = %d]: not reported as VRINPUT_BINARY (%d instead)\n",
				count,
				aux->devinfo->switch2[count].my_object->name,
				aux->button_offsets[count],
				aux->button_types[count],
				aux->button_inputs[count]->input_type);
		}
	}

	vrFprintf(file, "\r\t%d valuators\n", aux->num_valuators);
	for (count = 0; count < aux->num_valuators; count++) {
		if (aux->valuator_inputs[count]->input_type == VRINPUT_VALUATOR) {
			vrFprintf(file, "\r\t\tvaluator %d ('%s') [offset = %d, type = %d]: = %.2f\n",
				count,
				aux->devinfo->valuator[count].my_object->name,
				aux->valuator_offsets[count],
				aux->valuator_types[count],
				aux->valuator_values[count]);
		} else {
			vrFprintf(file, "\r\t\tvaluator %d ('%s') [offset = %d, type = %d]: not reported as VRINPUT_VALUATOR (%d instead)\n",
				count,
				aux->devinfo->switch2[count].my_object->name,
				aux->valuator_offsets[count],
				aux->valuator_types[count],
				aux->valuator_inputs[count]->input_type);
		}
	}

	vrFprintf(file, "\r\t%d 6-sensors\n", aux->num_6sensors);
	for (count = 0; count < aux->num_6sensors; count++) {
		if (aux->sensor6_inputs[count]->input_type == VRINPUT_6SENSOR) {
			/* TODO: print the matrix */
			vrFprintf(file, "\r\t\t6sensor %d ('%s') [offset = %d, type = %d]: = ...\n",
				count,
				aux->devinfo->sensor6[count].my_object->name,
				aux->sensor6_offsets[count],
				aux->sensor6_types[count]);
		} else {
			vrFprintf(file, "\r\t\t6sensor %d ('%s') [offset = %d, type = %d]: not reported as VRINPUT_6SENSOR (%d instead)\n",
				count,
				aux->devinfo->switch2[count].my_object->name,
				aux->sensor6_offsets[count],
				aux->sensor6_types[count],
				aux->sensor6_inputs[count]->input_type);
		}
	}
}


	/************************************/
	/***  FreeVR NON public routines  ***/
	/************************************/


/***************************************************************/
static void _ShmemdParseArgs(_ShmemdPrivateInfo *aux, char *args)
{
	/* In the rare case of no arguments, just return */
	if (args == NULL)
		return;

	/***************************************/
	/** Argument format: "key" "=" number **/
	/***************************************/
	vrArgParseInteger(args, "key", &(aux->shmkey));
	vrArgParseInteger(args, "tkey", &(aux->shmkey));
	vrArgParseInteger(args, "ckey", &(aux->shmkey_cont));


	/* it would be reasonable to add some arguments for setting the dummy values here */
}


/***************************************************************/
/* NOTE: unlike the vr_input.shmemd.c counterpart, this version loops */
/*   through all the inputs of particular types, and copies the data  */
/*   into shared memory.                                              */
static void _ShmemdPutData(vrInputDevice *devinfo)
{
	_ShmemdPrivateInfo	*aux = (_ShmemdPrivateInfo *)devinfo->aux_data;
	int			count;

	/* TODO: I'd like these numbers to be determined in the _ShmemdCreateFunction() */
	/*   function, but that doesn't seem to be working like I expected.  It's not  */
	/*   a big time sink if it's done here though.                                 */
	aux->num_buttons = devinfo->context->input->num_2ways;
	aux->num_valuators = devinfo->context->input->num_valuators;
	aux->num_6sensors = devinfo->context->input->num_6sensors;

	/* This is what I need to emulate (when hardcoded):  */
	/*   args = "key = 4127;";                             */
	/*     input "2switch[0]" = "2switch(int[40])";        */
	/*     input "2switch[1]" = "2switch(int[44])";        */
	/*     input "2switch[2]" = "2switch(int[48])";        */
	/*     input "valuator[x]" = "valuator(float[168])";   */
	/*     input "valuator[y]" = "valuator(float[172])";	# ?? depends on size of float! */
	/*   args = "key = 4126;";                             */
	/*     input "6sensor[head]" = "6sensor(vreuler[28])"; */
	/*     input "6sensor[wand]" = "6sensor(vrEuler[68])"; */

	/*************/
	/** buttons **/
	*((int *)&(aux->data_cont[ 4])) =  40;			/* offset to button data */
	*((int *)&(aux->data_cont[12])) = aux->num_buttons-1;	/* number of buttons (not counting the escape key) */
	*((int *)&(aux->data_cont[32])) = aux->num_buttons-1;	/* number of buttons (in controller header) */

	/* NOTE: we skip button 0, which is the escape key */
	for (count = 1; count < aux->num_buttons; count++) {
		int		offset;				/* the location in the shared memory segment to store the information */

		offset = (count * 4) + 36;

		/* TODO: the following will have the side-effect of changing the "last_value"  */
		/*   of the input, but since we're probably only doing this in order to put    */
		/*   data into shared memory, we probably don't care, but I think it should be */
		/*   fixed anyway, since down the road we may care.                            */
		*((int *)&(aux->data_cont[offset])) = vrGet2switchValue(count);
	}

	/***************/
	/** valuators **/
	*((int *)&(aux->data_cont[ 8])) = 168;			/* offset to valuator data */
	*((int *)&(aux->data_cont[16])) = aux->num_valuators;	/* number of valuators (in main header) */
	*((int *)&(aux->data_cont[36])) = aux->num_valuators;	/* number of valuators (in controller header) */

	for (count = 0; count < aux->num_valuators; count++) {
		int		offset;				/* the location in the shared memory segment to store the information */

		offset = (count * 4) + 168;

		*((float *)&(aux->data_cont[offset])) = vrGetValuatorValue(count);
	}

	/***************/
	/** 6-sensors **/
	*((int *)&(aux->data[ 4])) = aux->num_6sensors;		/* number of 6-sensors */
	*((int *)&(aux->data[ 8])) =  28;			/* offset to 6-sensor data */
	*((int *)&(aux->data[12])) =  40;			/* size of each 6-sensors data */

	for (count = 0; count < aux->num_6sensors; count++) {
		int		offset;				/* the location in the shared memory segment to store the information */
		vrEuler		euler;				/* a transitional place to get the sensor data */

		offset = (count * 40) + 28;
#if 0
		vrMatrixSetEulerAzimaxis(new_value, new_value_euler, VR_Y);
#endif
		vrEulerGetRWFrom6sensor(&euler, count);
		*(vrEuler *)((void *)(&(aux->data[offset]))) = euler;
	}
#if 0
	/*************/
	/** buttons **/
	for (count = 0; count < aux->num_buttons; count++) {
		vr2switch	*current_2switch;
		int		*current_value;
		int		new_value;
		int		offset;
		_MemoryType	type;
		int		size;

		current_2switch = aux->button_inputs[count];
		current_value = &(aux->button_values[count]);
		offset = aux->button_offsets[count];
		type = aux->button_types[count];
		size = _MemoryTypeSize(type);

		if (aux->data == NULL) {
			if (!aux->button_dummy[count])
				vrDbgPrintfN(AALWAYS_DBGLVL, "_ShmemdPutData: "
					RED_TEXT "No shared memory data, so using a dummy for button %d\n" NORM_TEXT,
					count);
			aux->button_dummy[count] = 1;
			new_value = aux->dummy_button_value;
		} else if (offset+size > aux->segment_size) { /* checking for shared memory bound violation */
			vrDbgPrintfN(AALWAYS_DBGLVL, "_ShmemdPutData: "
				RED_TEXT "Shared memory offset is outside the memory segment, so using a dummy for button %d\n" NORM_TEXT,
				count);
			aux->button_dummy[count] = 1;
			new_value = aux->dummy_button_value;

		} else {
			/* we've made it past all the error checks, so now get the real data */
			switch (type) {
			case MEM_CHAR:
				new_value = *((char *)&(aux->data[offset]));
				break;

			case MEM_INT:
				new_value = *((int *)&(aux->data[offset]));
				break;

			case MEM_LONG:
				new_value = *((long *)&(aux->data[offset]));
				break;

			case MEM_FLOAT:
				new_value = *((float *)&(aux->data[offset]));
				break;

			case MEM_DOUBLE:
				new_value = *((double *)&(aux->data[offset]));
				break;

			case MEM_UNKNOWN:
			default:
				vrDbgPrintfN(AALWAYS_DBGLVL, "_ShmemdPutData: "
					RED_TEXT "Unknown memory type, so using a dummy for button %d\n" NORM_TEXT,
					count);
				aux->button_dummy[count] = 1;
				new_value = aux->dummy_button_value;
				break;
			}
		}

#ifdef DEBUG /* use this to see what's happening in the memory bytes */
		if (vrDbgDo(SHMEMD_DBGLVL)) {
			int byte;

			vrPrintf("_ShmemdPutData(): button %d, value = %d, type = %d, size = %d, offset = %2d, data = %#p:", count, new_value, (int)type, size, offset, aux->data);
			for (byte = 0; byte < size; byte++)
				vrPrintf(" 0x%02x", (unsigned int)aux->data[offset+byte]);
			vrPrintf(".\n");
		}
#endif

		if (*current_value != new_value) {
			*current_value = new_value;

			aux->button_dummy[count] = 0;

			switch (current_2switch->input_type) {
			case VRINPUT_BINARY:
				vrDbgPrintfN(SHMEMD_DBGLVL, "_ShmemdPutData: "
					"Assigning a value of %d to Shmemd button %d\n", new_value, count);
				vrAssign2switchValue(current_2switch, new_value /*, timestamp from daemon */);
				break;

			case VRINPUT_CONTROL:
				vrDbgPrintfN(SHMEMD_DBGLVL, "_ShmemdPutData: "
					"Activating a callback with %d to Shmemd button %d\n", new_value, count);
				vrCallbackInvokeDynamic(((vrControl *)current_2switch)->callback, 1, new_value);
				break;

			default:
				vrErrPrintf(RED_TEXT "_ShmemdPutData: Unable to handle toggle inputs that aren't Binary or Control inputs\n" NORM_TEXT);
				break;
			}
		}
	}

	/***************/
	/** valuators **/
	for (count = 0; count < aux->num_valuators; count++) {
		vrValuator	*current_valuator;
		float		*current_value;
		float		new_value;
		int		offset;
		_MemoryType	type;
		int		size;

		current_valuator = aux->valuator_inputs[count];
		current_value = &(aux->valuator_values[count]);
		offset = aux->valuator_offsets[count];
		type = aux->valuator_types[count];
		size = _MemoryTypeSize(type);

		if (aux->data == NULL) {
			if (!aux->valuator_dummy[count])
				vrDbgPrintfN(AALWAYS_DBGLVL, "_ShmemdPutData: "
					RED_TEXT "No shared memory data, so using a dummy for valuator %d\n" NORM_TEXT,
					count);
			aux->valuator_dummy[count] = 1;
			new_value = aux->dummy_valuator_value;
		} else if (offset+size > aux->segment_size) { /* checking for shared memory bound violation */
			vrDbgPrintfN(AALWAYS_DBGLVL, "_ShmemdPutData: "
				RED_TEXT "Shared memory offset is outside the memory segment, so using a dummy for valuator %d.\n" NORM_TEXT, count);
			aux->valuator_dummy[count] = 1;
			new_value = aux->dummy_valuator_value;

	/* TODO: some shared memory systems record the number of each type of input */
	/*   so it might be reasonable to have a config option that specified where */
	/*   that information is kept, and do a comparison.                         */
#ifdef OLD_CODE
		} else if (count >= *((int *)&(aux->data[num_valuator_offset]))) {
			if (!aux->valuator_dummy[count])
				vrDbgPrintfN(AALWAYS_DBGLVL, "_ShmemdPutData: "
					RED_TEXT "Not enough shmemd valuators, so using a dummy for valuator %d\n" NORM_TEXT,
					count);
			aux->valuator_dummy[count] = 1;
			new_value = aux->dummy_valuator_value;
#endif

		} else {
			/* we've made it past all the error checks, so now get the real data */
			switch (type) {
			case MEM_CHAR:
				/* TODO: for non-reals need a divisor */
				new_value = *((char *)&(aux->data[offset]));
				break;

			case MEM_INT:
				/* TODO: for non-reals need a divisor */
				new_value = *((int *)&(aux->data[offset]));
				break;

			case MEM_LONG:
				/* TODO: for non-reals need a divisor */
				new_value = *((long *)&(aux->data[offset]));
				break;

			case MEM_FLOAT:
				new_value = *((float *)&(aux->data[offset]));
				break;

			case MEM_DOUBLE:
				new_value = *((double *)&(aux->data[offset]));
				break;

			case MEM_UNKNOWN:
			default:
				vrDbgPrintfN(AALWAYS_DBGLVL, "_ShmemdPutData: "
					RED_TEXT "Unknown memory type, so using a dummy for valuator %d\n" NORM_TEXT,
					count);
				aux->valuator_dummy[count] = 1;
				new_value = aux->dummy_valuator_value;
				break;
			}
		}

#ifdef DEBUG /* use this to see what's happening in the memory bytes */
		if (vrDbgDo(SHMEMD_DBGLVL)) {
			int byte;

			vrPrintf("_ShmemdPutData(): valuator %d, value = %f, type = %d, size = %d, offset = %2d, data = %#p:", count, new_value, (int)type, size, offset, aux->data);
			for (byte = 0; byte < size; byte++)
				vrPrintf(" 0x%02x", (unsigned int)aux->data[offset+byte]);
			vrPrintf(".\n");
		}
#endif

		if (*current_value != new_value) {
			*current_value = new_value;

			aux->valuator_dummy[count] = 0;

			switch (current_valuator->input_type) {
			case VRINPUT_VALUATOR:
				vrDbgPrintfN(SHMEMD_DBGLVL, "_ShmemdPutData: "
					"Assigning a value of %.2f to shmemd valuator %d\n", new_value, count);
				vrAssignValuatorValue(current_valuator, new_value);
				break;

			case VRINPUT_CONTROL:
				vrDbgPrintfN(SHMEMD_DBGLVL, "_ShmemdPutData: "
					"Activating a callback with %.2f to shmemd valuator %d\n", new_value, count);
				vrCallbackInvokeDynamic(((vrControl *)current_valuator)->callback, 1, (int)new_value);
				break;

			default:
				vrErrPrintf(RED_TEXT "_ShmemdPutData: Unable to handle valuator inputs that aren't Valuator or Control inputs\n" NORM_TEXT);
				break;
			}
		}
	}

	/***************/
	/** 6-sensors **/
	for (count = 0; count < aux->num_6sensors; count++) {
		vr6sensor	*current_6sensor;
		vrMatrix	*new_value;
		vrEuler		*new_value_euler;
		vrEuler		tmp_euler;		/* used for converting double-Eulers to normal vrEulers */
		int		offset;
		_MemoryType	type;
		int		size;

		current_6sensor = aux->sensor6_inputs[count];
		offset = aux->sensor6_offsets[count];
		type = aux->sensor6_types[count];
		size = _MemoryTypeSize(type);

		if (aux->data == NULL) {
			if (!aux->sensor6_dummy[count])
				vrDbgPrintfN(AALWAYS_DBGLVL, "_ShmemdPutData: "
					RED_TEXT "No shared memory data, so using a dummy for 6-sensor %d\n" NORM_TEXT,
					count);
			aux->sensor6_dummy[count] = 1;
			new_value = &aux->dummy_6sensor_value;
		} else if (offset+size > aux->segment_size) { /* checking for shared memory bound violation */
			vrDbgPrintfN(AALWAYS_DBGLVL, "_ShmemdPutData: "
				RED_TEXT "Shared memory offset is outside the memory segment, so using a dummy for 6-sensor %d.\n" NORM_TEXT, count);
			aux->sensor6_dummy[count] = 1;
			*new_value = aux->dummy_6sensor_value;

	/* TODO: some shared memory systems record the number of each type of input */
	/*   so it might be reasonable to have a config option that specified where */
	/*   that information is kept, and do a comparison.                         */
#ifdef OLD_CODE
		} else if (count >= *((int *)&(aux->data[num_6sensor_offset]))) {
			if (!aux->sensor6_dummy[count])
				vrDbgPrintfN(AALWAYS_DBGLVL, "_ShmemdPutData: "
					RED_TEXT "Not enough shmemd 6-sensors, so using a dummy for 6-sensor %d\n" NORM_TEXT,
					count);
			aux->sensor6_dummy[count] = 1;
			new_value = &aux->dummy_6sensor_value;
#endif

		} else {
			new_value = &(aux->sensor6_values[count]);

			switch (type) {
			case MEM_VREULER:
				new_value_euler = (vrEuler *)((void *)(&(aux->data[offset])));
				vrMatrixSetEulerAzimaxis(new_value, new_value_euler, VR_Y);
				break;

			case MEM_VISBOXEULER:
				new_value_euler = (vrEuler *)((void *)(&(aux->data[offset])));
				tmp_euler = *new_value_euler;
				tmp_euler.r[VR_ROLL] *= -1.0;
				vrMatrixSetEulerAzimaxis(new_value, &tmp_euler, VR_Y);
				break;

			case MEM_VISBOXTRANS:
				new_value_euler = (vrEuler *)((void *)(&(aux->data[offset])));
				tmp_euler = *new_value_euler;
				tmp_euler.r[VR_AZIM] = 0.0;
				tmp_euler.r[VR_ELEV] = 0.0;
				tmp_euler.r[VR_ROLL] = 0.0;
				vrMatrixSetEulerAzimaxis(new_value, &tmp_euler, VR_Y);
				break;

			case MEM_VRMATRIX:
		/* TODO: test this -- I don't have a source for this yet */
				*new_value = *(vrMatrix *)((void *)(&(aux->data[offset])));
				break;

			case MEM_EULERDOUBLE:
		/* TODO: test this -- I don't have a source for this yet */
				_EulerConvertDoubleToFloat(&tmp_euler, (vrEulerDouble *)((void *)(&(aux->data[offset]))));
				vrMatrixSetEulerAzimaxis(new_value, &tmp_euler, VR_Y);
				break;

			case MEM_MATRIXFLOAT:
		/* TODO: test this -- I don't have a source for this yet */
				_MatrixConvertFloatToDouble(new_value, (vrMatrixFloat *)((void *)(&(aux->data[offset]))));
				break;

			case MEM_UNKNOWN:
			default:
				vrDbgPrintfN(AALWAYS_DBGLVL, "_ShmemdPutData: "
					RED_TEXT "Unknown memory type (%d), so using a dummy for 6-sensor %d\n" NORM_TEXT,
					type, count);
				aux->sensor6_dummy[count] = 1;
				*new_value = aux->dummy_6sensor_value;
				break;
			}
		}

#ifdef DEBUG /* use this to see what's happening in the memory bytes */
		if (vrDbgDo(SHMEMD_DBGLVL)) {
			int byte;

			vrPrintf("_ShmemdPutData(): 6-sensor %d, value = %d, type = %d, size = %d, offset = %2d, data = %#p:", count, new_value, (int)type, size, offset, aux->data);
			for (byte = 0; byte < size; byte++)
				vrPrintf(" 0x%02x", (unsigned int)aux->data[offset+byte]);
			vrPrintf(".\n");
		}
#endif

		/* since 6-sensors generally change rapidly, there is little point */
		/*   and often moderate expense in comparing for changes from the  */
		/*   current value to the new value.                               */
		/* although perhaps watching the timestamp might be a useful way   */
		/*   to do this.                                                   */
		vrAssign6sensorValue(current_6sensor, new_value, 0 /*, TODO: timestamp from daemon */);
	}
#endif
}


#if 0 /* I don't think I'm going to want this stuff for the "output" function, but I'll leave the code in place for now [04/11/2007] */

	/*****************************************************************/
	/*   Function(s) for parsing Shared Memory "input" declarations. */
	/*                                                               */
	/*  These _Shmemd<type>Input() functions are called during the   */
	/*  CREATE phase of the input interface.                         */

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

/**************************************************************************/
/* type might be "char", "int", or "long" (and in the future "float" or "double") */
/*    parsing:                                                                    */
/*       input "<name>" = "2switch(<type>[<offset>])";                            */
/*    thus:                                                                       */
/*       the device string is unused.                                             */
/*       the type string is <type> (eg. "char", "int", etc)                       */
/*       the instance string is <offset> (eg. 0, 4, 6)                            */
/**************************************************************************/
static vrInputMatch _ShmemdButtonInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
        _ShmemdPrivateInfo	*aux = (_ShmemdPrivateInfo *)devinfo->aux_data;
	int			button_num;
	int			offset;
	_MemoryType		memtype;

	/* determine current button in our private arrays of button data */
	button_num = aux->num_buttons;
	aux->num_buttons++;

	memtype = _MemoryValue(dti->type);
	offset = vrAtoI(dti->instance);

	vrDbgPrintfN(INPUT_DBGLVL, "_ShmemdButtonInput(): Assigned button event of button %d, to type '%s'(%d) with offset %d\n",
		button_num, _MemoryTypeName(memtype), (int)memtype, offset);

	if (memtype == MEM_UNKNOWN) {
		vrErrPrintf("_ShmemdValuatorInput: " RED_TEXT "Warning, type['%s'] did not match any known type\n" NORM_TEXT, dti->type);
		return VRINPUT_NOMATCH;	/* unacceptable input declaration match, perhaps keep searching */
	}

#if 0 /* NOTE: this check isn't possible here because we haven't connected with the memory yet to know the size */
	if (offset 'out of bounds') {
		...
		return VRINPUT_NOMATCH;	/* unacceptable input declaration match, perhaps keep searching */
	}
#endif

	aux->button_inputs[button_num] = (vr2switch *)input;
	aux->button_values[button_num] = 0;
	aux->button_dummy[button_num] = 0;
	aux->button_offsets[button_num] = offset;
	aux->button_types[button_num] = memtype;

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/**************************************************************************/
static vrInputMatch _ShmemdValuatorInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
        _ShmemdPrivateInfo	*aux = (_ShmemdPrivateInfo *)devinfo->aux_data;
	int			valuator_num;
	int			offset;
	_MemoryType		memtype;

	/* determine current valuator in our private arrays of valuator data */
	valuator_num = aux->num_valuators;
	aux->num_valuators++;

	memtype = _MemoryValue(dti->type);
	offset = vrAtoI(dti->instance);

	vrDbgPrintfN(INPUT_DBGLVL, "_ShmemdValuatorInput(): Assigned button event of button %d, to type '%s'(%d) with offset %d\n",
		valuator_num, _MemoryTypeName(memtype), (int)memtype, offset);

	if (memtype == MEM_UNKNOWN) {
		vrErrPrintf("_ShmemdValuatorInput: " RED_TEXT "Warning, type['%s'] did not match any known type\n" NORM_TEXT, dti->type);
		return VRINPUT_NOMATCH;	/* unacceptable input declaration match, perhaps keep searching */
	}

#if 0 /* NOTE: this check isn't possible here because we haven't connected with the memory yet to know the size */
	if (offset 'out of bounds') {
		...
		return VRINPUT_NOMATCH;	/* unacceptable input declaration match, perhaps keep searching */
	}
#endif

	aux->valuator_inputs[valuator_num] = (vrValuator *)input;
	aux->valuator_values[valuator_num] = 0;
	aux->valuator_dummy[valuator_num] = 0.0;
	aux->valuator_offsets[valuator_num] = offset;
	aux->valuator_types[valuator_num] = memtype;

	return VRINPUT_MATCH_ABLE;		/* input declaration match */
}


/**************************************************************************/
static vrInputMatch _Shmemd6sensorInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
        _ShmemdPrivateInfo	*aux = (_ShmemdPrivateInfo *)devinfo->aux_data;
	int			sensor6_num;
	int			offset;
	_MemoryType		memtype;

	/* determine current 6-sensor in our private arrays of 6-sensor data */
	sensor6_num = aux->num_6sensors;
	aux->num_6sensors++;

	memtype = _MemoryValue(dti->type);
	offset = vrAtoI(dti->instance);

	vrDbgPrintfN(INPUT_DBGLVL, "_Shmemd6sensorInput(): Assigned 6-sensor event of 6-sensor %d, to type '%s'(%d) with offset %d\n",
		sensor6_num, _MemoryTypeName(memtype), (int)memtype, offset);

	if (memtype == MEM_UNKNOWN) {
		vrErrPrintf("_ShmemdValuatorInput: " RED_TEXT "Warning, type['%s'] did not match any known type\n" NORM_TEXT, dti->type);
		return VRINPUT_NOMATCH;	/* unacceptable input declaration match, perhaps keep searching */
	}

#if 0 /* NOTE: this check isn't possible here because we haven't connected with the memory yet to know the size */
	if (offset 'out of bounds') {
		...
		return VRINPUT_NOMATCH;	/* unacceptable input declaration match, perhaps keep searching */
	}
#endif

	aux->sensor6_inputs[sensor6_num] = (vr6sensor *)input;
	vrMatrixSetIdentity(&aux->sensor6_values[sensor6_num]);
	aux->sensor6_dummy[sensor6_num] = 0;
	aux->sensor6_offsets[sensor6_num] = offset;
	aux->sensor6_types[sensor6_num] = memtype;

	vrAssign6sensorR2Exform(aux->sensor6_inputs[sensor6_num], strchr(dti->instance, ','));

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}

#endif


	/************************************************************/
	/***************** FreeVR Callback routines *****************/
	/************************************************************/

	/************************************************************/
	/*    Callbacks for controlling the device's features.      */
	/*                                                          */

/************************************************************/
static void _ShmemdSystemPauseToggleCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

	devinfo->context->paused ^= 1;
	vrDbgPrintfN(INPUT_DBGLVL, "Shmemd Control: system_pause = %d.\n",
		devinfo->context->paused);
}

/************************************************************/
static void _ShmemdPrintContextStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintContext(stdout, devinfo->context, verbose);
}

/************************************************************/
static void _ShmemdPrintConfigStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintConfig(stdout, devinfo->context->config, verbose);
}

/************************************************************/
static void _ShmemdPrintInputStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintInput(stdout, devinfo->context->input, verbose);
}

/************************************************************/
static void _ShmemdPrintStructCallback(vrInputDevice *devinfo, int value)
{
	_ShmemdPrivateInfo  *aux = (_ShmemdPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	_ShmemdPrintStruct(stdout, aux, verbose);
}

/************************************************************/
static void _ShmemdPrintHelpCallback(vrInputDevice *devinfo, int value)
{
        _ShmemdPrivateInfo  *aux = (_ShmemdPrivateInfo *)devinfo->aux_data;

        if (value == 0)
                return;

        _ShmemdPrintHelp(stdout, aux);
}

/************************************************************/
static void _ShmemdPrintValuesCallback(vrInputDevice *devinfo, int value)
{
	_ShmemdPrivateInfo  *aux = (_ShmemdPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	_ShmemdPrintValues(stdout, aux);
}




	/*************************************************/
	/*   Callbacks for interfacing with the device.  */
	/*                                               */

/**********************************************************/
static void _ShmemdCreateFunction(vrInputDevice *devinfo)
{
#if 0
	/*** List of possible inputs ***/
static	vrInputFunction	_ShmemdInputs[] = {
			/* TODO: ideally, we should be able to catch all 2-switches with: { NULL, VRINPUT_BINARY, _ShmemdButtonInput }, */
				{ "char", VRINPUT_BINARY, _ShmemdButtonInput },
				{ "int",  VRINPUT_BINARY, _ShmemdButtonInput },
				{ "long", VRINPUT_BINARY, _ShmemdButtonInput },
			/* TODO: a "float/double" binary is also conceivable, perhaps with an epsilon */
				{ "float",  VRINPUT_VALUATOR, _ShmemdValuatorInput },
				{ "double", VRINPUT_VALUATOR, _ShmemdValuatorInput },
			/* TODO: valuators based on integer values are also conceivable */
				{ "vreuler",       VRINPUT_6SENSOR, _Shmemd6sensorInput },
				{ "vrmatrix",      VRINPUT_6SENSOR, _Shmemd6sensorInput },
				{ "vreulerfloat",  VRINPUT_6SENSOR, _Shmemd6sensorInput },
				{ "vrmatrixdouble",VRINPUT_6SENSOR, _Shmemd6sensorInput },
				{ "vreulerdouble", VRINPUT_6SENSOR, _Shmemd6sensorInput },
				{ "vrmatrixfloat", VRINPUT_6SENSOR, _Shmemd6sensorInput },
			/* TODO: I'd rather do without these, but needed to get visbox working ASAP */
				{ "visboxeuler",   VRINPUT_6SENSOR, _Shmemd6sensorInput },
				{ "visboxtrans",   VRINPUT_6SENSOR, _Shmemd6sensorInput },
				{ NULL, VRINPUT_UNKNOWN, NULL } };
#endif
	/*** List of control functions ***/
static	vrControlFunc	_ShmemdControlList[] = {
				/* overall system controls */
				{ "system_pause_toggle", _ShmemdSystemPauseToggleCallback },

				/* informational output controls */
				{ "print_context", _ShmemdPrintContextStructCallback },
				{ "print_config",  _ShmemdPrintConfigStructCallback },
				{ "print_input",   _ShmemdPrintInputStructCallback },
				{ "print_struct",  _ShmemdPrintStructCallback },
				{ "print_help",    _ShmemdPrintHelpCallback },
				{ "print_values",  _ShmemdPrintValuesCallback },

		/** TODO: other callback control functions go here **/
				/* simulated 6-sensor selection controls */
				/* simulated 6-sensor manipulation controls */
				/* other controls */

				/* end of the list */
				{ NULL, NULL } };

	_ShmemdPrivateInfo	*aux = NULL;

	/******************************************/
	/* allocate and initialize auxiliary data */
	devinfo->aux_data = (void *)vrShmemAlloc0(sizeof(_ShmemdPrivateInfo));
	aux = (_ShmemdPrivateInfo *)devinfo->aux_data;
	_ShmemdInitializeStruct(aux);
	aux->devinfo = devinfo;		/* [1/31/00] -- we don't normally do this, but are experimenting */

	/******************/
	/* handle options */
	_ShmemdParseArgs(aux, devinfo->args);

#if 0 /* this stuff will probably be a lot different for the "output" version of the shared memory interface, but leaving the code for now [04/11/2007] */
	/***************************************/
	/* create the inputs and self-controls */

	/* Because shmemd can have a large unknown number of inputs,      */
	/*   we allocate memory for handling them here.  First the        */
	/*   vrInputCountDataContainers() function is called to determine */
	/*   how many of each type of input needs to be created.          */
	/*   Normally vrInputCountDataContainers() is called by           */
	/*   vrInputCreateDataContainers() (see below), but for this      */
	/*   circumstance we split the two operations.                    */
	vrInputCountDataContainers(devinfo);

	aux->num_buttons = 0;
	aux->button_inputs = (vr2switch **)vrShmemAlloc((devinfo->num_2ways + devinfo->num_scontrols) * sizeof(vr2switch *));
	aux->button_values = (int *)vrShmemAlloc((devinfo->num_2ways + devinfo->num_scontrols) * sizeof(int));
	aux->button_dummy = (int *)vrShmemAlloc((devinfo->num_2ways + devinfo->num_scontrols) * sizeof(int));
	aux->button_types = (int *)vrShmemAlloc((devinfo->num_2ways + devinfo->num_scontrols) * sizeof(int));
	aux->button_offsets = (int *)vrShmemAlloc((devinfo->num_2ways + devinfo->num_scontrols) * sizeof(int));

	aux->num_valuators = 0;
	aux->valuator_inputs = (vrValuator **)vrShmemAlloc((devinfo->num_valuators + devinfo->num_scontrols + 10) * sizeof(vrValuator *));
	aux->valuator_values = (float *)vrShmemAlloc((devinfo->num_valuators + devinfo->num_scontrols) * sizeof(float));
	aux->valuator_dummy = (int *)vrShmemAlloc((devinfo->num_valuators + devinfo->num_scontrols) * sizeof(int));
	aux->valuator_types = (int *)vrShmemAlloc((devinfo->num_valuators + devinfo->num_scontrols) * sizeof(int));
	aux->valuator_offsets = (int *)vrShmemAlloc((devinfo->num_valuators + devinfo->num_scontrols) * sizeof(int));

	aux->num_6sensors = 0;
	aux->sensor6_inputs = (vr6sensor **)vrShmemAlloc(devinfo->num_6sensors * sizeof(vr6sensor *));
	aux->sensor6_values = (vrMatrix *)vrShmemAlloc(devinfo->num_6sensors * sizeof(vrMatrix));
	aux->sensor6_dummy = (int *)vrShmemAlloc(devinfo->num_6sensors * sizeof(int));
	aux->sensor6_types = (int *)vrShmemAlloc(devinfo->num_6sensors * sizeof(int));
	aux->sensor6_offsets = (int *)vrShmemAlloc(devinfo->num_6sensors * sizeof(int));

	/* Here we return to the conventional way of creating the inputs */
	vrInputCreateDataContainers(devinfo, _ShmemdInputs);
	vrInputCreateSelfControlContainers(devinfo, _ShmemdInputs, _ShmemdControlList);
	/* TODO: anything to do for NON self-controls?  Implement for Magellan first. */
#endif

	/* Here is some hard-coding of what would typically be done in most setups. */
	/*   I.e. write to shared memory all the buttons (except button 0), all the */
	/*   valuators, and all the 6-sensors.                                      */
	/* TODO: these values don't seem to get filled in at this point -- I was */
	/*   thinking that since this "input" device would be last in the list,  */
	/*   that the values would be known by the time it is initialized, but I */
	/*   guess that's not the case, so I'm going to set the value at the top */
	/*   of the processing routine _ShmemdPutData().                         */
	aux->num_buttons = devinfo->context->input->num_2ways;
	aux->num_valuators = devinfo->context->input->num_valuators;
	aux->num_6sensors = devinfo->context->input->num_6sensors;

	vrDbgPrintf("OUTPUT:_ShmemdOpenFunction(): Done creating Shared Memory outputs for '%s'\n", devinfo->name);
	devinfo->created = 1;

	return;
}


/**********************************************************/
static void _ShmemdOpenFunction(vrInputDevice *devinfo)
{
	_ShmemdPrivateInfo	*aux = (_ShmemdPrivateInfo *)devinfo->aux_data;

	/*******************/
	/* open the device */
	aux->shmid = vrShmemGetKey(aux->shmkey, 0x4000, (void **)(&(aux->data)), 1);

	if (!aux->data) {
		vrDbgPrintf("OUTPUT:_ShmemdOpenFunction(): No shared memory segments for key %p.\n", aux->shmkey);
		sprintf(devinfo->version, "- unconnected shmemd -");
		aux->open = 0;
	} else {
		aux->open = 1;
	}

#if 1
	aux->segment_size = vrShmemIdGetSize(aux->shmid);
#else
	aux->segment_size = 2000;
#endif

	vrDbgPrintf("OUTPUT:_ShmemdOpenFunction(): data segment (id = %d, size = %d) at %#p for key %d(0x%x)\n",
		aux->shmid, aux->segment_size, aux->data, aux->shmkey, aux->shmkey);
	vrDbgPrintf("OUTPUT:_ShmemdOpenFunction(): Done opening Shmemd for output device '%s'\n", devinfo->name);
	devinfo->operating = aux->open;

	/*************************/
	/* open the other device */
	aux->shmid = vrShmemGetKey(aux->shmkey_cont, 0x4000, (void **)(&(aux->data_cont)), 1);

	if (!aux->data) {
		vrDbgPrintf("OUTPUT:_ShmemdOpenFunction(): No shared memory segments for key %p.\n", aux->shmkey_cont);
		sprintf(devinfo->version, "- unconnected shmemd -");
		aux->open = 0;
	} else {
		aux->open = 1;
	}

#if 1
	aux->segment_size = vrShmemIdGetSize(aux->shmid);
#else
	aux->segment_size = 2000;
#endif

	vrDbgPrintf("OUTPUT:_ShmemdOpenFunction(): data segment (id = %d, size = %d) at %#p for key %d(0x%x)\n",
		aux->shmid, aux->segment_size, aux->data, aux->shmkey_cont, aux->shmkey_cont);
	vrDbgPrintf("OUTPUT:_ShmemdOpenFunction(): Done opening Shmemd for output device '%s'\n", devinfo->name);
	devinfo->operating = aux->open;

	if (vrDbgDo(SHMEMD_DBGLVL)) {
		_ShmemdPrintValues(stdout, aux);
	}

	return;
}


/**********************************************************/
static void _ShmemdCloseFunction(vrInputDevice *devinfo)
{
	/* TODO: close the reference to the shared memory segment. */
	return;
}


/**********************************************************/
static void _ShmemdResetFunction(vrInputDevice *devinfo)
{
	/* TODO: a shmemd reset should verify that the shmemd is sending the */
	/*   same amount of data, and if not, update the vrInputDevice.  */

	return;
}


/**********************************************************/
static void _ShmemdPollFunction(vrInputDevice *devinfo)
{
	if (devinfo->operating) {
		_ShmemdPutData(devinfo);
	} else {
		/* TODO: try to open the device again */
	}

	return;
}



	/******************************/
	/*** FreeVR public routines ***/
	/******************************/


/**********************************************************/
void vrShmemdOutInitInfo(vrInputDevice *devinfo)
{
	devinfo->version = (char *)vrShmemStrDup("Generic Shared Memory out-dev 0.1");
	devinfo->Create = vrCallbackCreateNamed("Shmemd:Create-Def", _ShmemdCreateFunction, 1, devinfo);
	devinfo->Open = vrCallbackCreateNamed("Shmemd:Open-Def", _ShmemdOpenFunction, 1, devinfo);
	devinfo->Close = vrCallbackCreateNamed("Shmemd:Close-Def", _ShmemdCloseFunction, 1, devinfo);
	devinfo->Reset = vrCallbackCreateNamed("Shmemd:Reset-Def", _ShmemdResetFunction, 1, devinfo);
	devinfo->PollData = vrCallbackCreateNamed("Shmemd:PollData-Def", _ShmemdPollFunction, 1, devinfo);
	devinfo->PrintAux = vrCallbackCreateNamed("Shmemd:PrintAux-Def", _ShmemdPrintStruct, 0);
}

