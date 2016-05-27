/* ======================================================================
 *
 *  CCCCC          vr_input.static.c
 * CC   CC         Author(s): Bill Sherman, John Stone
 * CC              Created: October 24, 1998
 * CC   CC         Last Modified: August 29, 2012
 *  CCCCC
 *
 * Code file for FreeVR tracking from the Static input device.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/*************************************************************************

USAGE:
	Inputs are specified with the "input" option:
		input "<name>" = "2switch(constant[{ | 0 | 1 }])";
		input "<name>" = "2switch(toggle[<period> { , <phase shift> }])";
		input "<name>" = "valuator(constant[{ | 0.0-1.0 }])";
		input "<name>" = "valuator(sinewave[<period> {, <phase shift>}])";
		input "<name>" = "6sensor(constant[{  | id } { | , r2e}])";
		input "<name>" = "6sensor(sinewave[<period> {, <phase shift>}])";


	Controls are specified with the "control" option:
		control "<control option>" = "2switch(...)";

	Here is the available control options for FreeVR:
		"print_help" -- print info on how to use the input device
		"system_pause_toggle" -- toggle the system pause flag
		"print_context" -- print the overall FreeVR context data structure (for debugging)
		"print_config" -- print the overall FreeVR config data structure (for debugging)
		"print_input" -- print the overall FreeVR input data structure (for debugging)
		"print_struct" -- print the internal Static-input data structure (for debugging)


	Here are the FreeVR configuration options for the Static Device:
		"x" - set the default sensor X position value
		"y" - set the default sensor Y position value
		"z" - set the default sensor Z position value
		"azim" - set the default sensor azimuth value
		"elev" - set the default sensor elevation value
		"roll" - set the default sensor roll value
		"oob" - set the default sensor oob value

		"button" - set the default button value
		"valuator" - set the default valuator value
		"switch" - set the default Nswitch value

HISTORY:
	24 October 1998 -- Bill Sherman wrote the initial version.

	30 October 1999 (Bill Sherman) -- converted to new input creation method.

	29 December 1999 (Bill Sherman) -- brought the static device code
		up to date with the new format of input device source
		files, including the new CREATE section of _StaticFunction().

	28 January 2000 (Bill Sherman) -- Integrated new self-control creation
		method.  And wrote a routine and function for the "print_struct"
		device control.  Also created the "toggle" and "sinewave" type
		of inputs -- even though these aren't particularly static, they
		may come in handy for testing (and they were certainly helpful
		in testing the self-control code).

	3 May 2001 (Bill Sherman)
		I made a few minor changes to catch up to the general
		  vr_input.skeleton.c format.

	11 September 2002 (Bill Sherman)
		Moved the control callback array into the _StaticFunction()
		  callback.

	21-23 April 2003 (Bill Sherman)
		Updated to new input matching function code style.  Changed
		  "opaque" field to "aux_data".  Split _StaticFunction()
		  into 5 functions.  Added new vrPrintStyle argument to
		  _StaticPrintStruct() for the sake of the new "PrintAux"
		  callback.

	3 June 2003 (Bill Sherman)
		Added the address of the auxiliary data to the printout.
		Added the "system_pause_toggle" control callback.
		Added comments classifying the controls.
		Changed references of vrNow() to vrCurrentWallTime().

	21 March 2006 (Bill Sherman)
		Improved _StaticParseArgs() function.

	29 August 2012 (Bill Sherman)
		... (6-sensor sinewaves)
		- TODO: need a way to set the amplitude of the 6-sensor sinewave
			(really, I need a way to do a complicated formula!) --
			presently it's hardcoded.
		- TODO: really the whole darn thing is hardcoded except for
			the period.

	2 September 2013 (Bill Sherman) -- adopting a consistent usage
		of str<...>cmp() functions (using logical negation).

	14 September 2013 (Bill Sherman)
		I am making use of the newly created SELFCTRL_DBGLVL for all
		the self-control outputs.  I then added some reasonable output
		to the "print_help" callback to print out what all the device's
		inputs are doing.

	15 September 2013 (Bill Sherman)
		Changed the "0x%p" format to the improved "%#p" format. (once!)

TODO:
	- Figure out the proper formula for the sine waves.

	- Implement N-way switches and N-sensors.

*************************************************************************/
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <math.h>

#include "vr_parse.h"
#include "vr_input.h"
#include "vr_input.opts.h"
#include "vr_shmem.h"
#include "vr_debug.h"


/****************************************************************/
/*** auxiliary structure of the current data from the Static. ***/
typedef struct {
		int		def_button_value;
		int		def_Nswitch_value;
		float		def_valuator_value;
		float		def_6sensor_vals[6];
		vrMatrix	*def_6sensor;
		int		def_oob_value;

		/* TODO: any defaults for N-sensors that we need */

#define MAX_TOGGLES	10
#define	MAX_SINEWAVES	10
		int		num_toggles;
		vr2switch	*toggle_inputs[MAX_TOGGLES];
		int		toggle_values[MAX_TOGGLES];
		vrTime		toggle_period[MAX_TOGGLES];
		vrTime		toggle_pshift[MAX_TOGGLES];
		int		num_sinewaves;
		vrValuator	*sinewave_inputs[MAX_SINEWAVES];
		float		sinewave_values[MAX_SINEWAVES];
		vrTime		sinewave_period[MAX_SINEWAVES];
		vrTime		sinewave_pshift[MAX_SINEWAVES];

	} _StaticPrivateInfo;



	/***********************************/
	/*** General NON public routines ***/
	/***********************************/

/**************************************************************************/
static void _StaticPrintStruct(FILE *file, _StaticPrivateInfo *aux, vrPrintStyle style)
{
	int	count;

	vrFprintf(file, "Static device internal structure (%#p):\n", aux);
	vrFprintf(file, "\r\tdefault button value = %d\n", aux->def_button_value);
	vrFprintf(file, "\r\tdefault N-switch value = %d\n", aux->def_Nswitch_value);
	vrFprintf(file, "\r\tdefault valuator value = %f\n", aux->def_valuator_value);
	vrFprintf(file, "\r\tdefault 6-sensor values = %f, %f, %f : %f, %f, %f\n",
		aux->def_6sensor_vals[0],
		aux->def_6sensor_vals[1],
		aux->def_6sensor_vals[2],
		aux->def_6sensor_vals[3],
		aux->def_6sensor_vals[4],
		aux->def_6sensor_vals[5]);
	vrFprintf(file, "\r\tdefault oob value = %d\n", aux->def_oob_value);

	vrFprintf(file, "\r\t%d Toggles:\n", aux->num_toggles);
	for (count = 0; count < aux->num_toggles; count++) {
		vrFprintf(file, "\r\t\ttoggle %d: = %d, period = %.2f, pshift = %.2f\n",
			count,
			aux->toggle_values[count],
			aux->toggle_period[count],
			aux->toggle_pshift[count]);
	}

	vrFprintf(file, "\r\t%d Sine Waves:\n", aux->num_sinewaves);
	for (count = 0; count < aux->num_sinewaves; count++) {
		vrFprintf(file, "\r\t\tsinewave %d: = %.2f, period = %.2f, pshift = %.2f\n",
			count,
			aux->sinewave_values[count],
			aux->sinewave_period[count],
			aux->sinewave_pshift[count]);
	}
}


/**************************************************************************/
static void _StaticPrintHelp(FILE *file, _StaticPrivateInfo *aux)
{
	int	count;		/* looping variable */

#if 0 || defined(TEST_APP)
	vrFprintf(file, BOLD_TEXT "Sorry, Static device - print_help control not yet implemented.\n" NORM_TEXT);
#else
	vrFprintf(file, BOLD_TEXT "Static device - inputs:" NORM_TEXT "\n");
		vrFprintf(file, "\t%s -- %d\n", "default button_value", aux->def_button_value);
		vrFprintf(file, "\t%s -- %d\n", "default Nswitch_value", aux->def_Nswitch_value);
		vrFprintf(file, "\t%s -- %f\n", "default valuator_value", aux->def_valuator_value);
		vrFprintf(file, "\t%s -- %f %f %f   %f %f %f\n", "default 6sensor_vals",
			aux->def_6sensor_vals[VR_X],
			aux->def_6sensor_vals[VR_Y],
			aux->def_6sensor_vals[VR_Z],
			aux->def_6sensor_vals[VR_AZIM],
			aux->def_6sensor_vals[VR_ELEV],
			aux->def_6sensor_vals[VR_ROLL]);
	for (count = 0; count < aux->num_toggles; count++) {
		if (aux->toggle_inputs[count] != NULL) {
			vrFprintf(file, "\t%s -- %s%s\n",
				aux->toggle_inputs[count]->my_object->desc_str,
				(aux->toggle_inputs[count]->input_type == VRINPUT_CONTROL ? "control:" : ""),
				aux->toggle_inputs[count]->my_object->name);
		}
	}
	for (count = 0; count < aux->num_sinewaves; count++) {
		if (aux->sinewave_inputs[count] != NULL) {
			vrFprintf(file, "\t%s -- %s%s\n",
				aux->sinewave_inputs[count]->my_object->desc_str,
				(aux->sinewave_inputs[count]->input_type == VRINPUT_CONTROL ? "control:" : ""),
				aux->sinewave_inputs[count]->my_object->name);
		}
	}

	/* NOTE: Static device has no fluctuating 6sensor inputs */

#ifdef NOT_YET_IMPLEMENTED
	for (count = 0; count < MAX_6SENSORS; count++) {
		if (aux->sensor6_inputs[count] != NULL) {
			vrFprintf(file, "\t%s -- %s%s\n",
				aux->sensor6_inputs[count]->my_object->desc_str,
				(aux->sensor6_inputs[count]->input_type == VRINPUT_CONTROL ? "control:" : ""),
				aux->sensor6_inputs[count]->my_object->name);
		}
	}
#endif /* NOT_YET_IMPLEMENTED */
#endif
}



	/**********************************/
	/*** FreeVR NON public routines ***/
	/**********************************/


/**************************************************************/
static void _StaticParseArgs(_StaticPrivateInfo *aux, char *args)
{
	/* assign default default values */
	aux->def_button_value = 0;
	aux->def_Nswitch_value = 0;
	aux->def_valuator_value = 0.0;
	aux->def_6sensor_vals[VR_X] = 0.0;
	aux->def_6sensor_vals[VR_Y] = 0.0;
	aux->def_6sensor_vals[VR_Z] = 0.0;
	aux->def_6sensor_vals[VR_AZIM+3] = 0.0;
	aux->def_6sensor_vals[VR_ELEV+3] = 0.0;
	aux->def_6sensor_vals[VR_ROLL+3] = 0.0;
	aux->def_oob_value = 0;


	if (args != NULL || 0) {
		/*****************************************************/
		/** Argument format: { "x" | "y" | "z" } "=" number **/
		/*****************************************************/
		vrArgParseFloat(args, "x", &(aux->def_6sensor_vals[VR_X]));
		vrArgParseFloat(args, "y", &(aux->def_6sensor_vals[VR_Y]));
		vrArgParseFloat(args, "z", &(aux->def_6sensor_vals[VR_Z]));

		/**************************************************************/
		/** Argument format: { "azim" | "elev" | "roll" } "=" number **/
		/**************************************************************/
		vrArgParseFloat(args, "azim", &(aux->def_6sensor_vals[VR_AZIM+3]));
		vrArgParseFloat(args, "elev", &(aux->def_6sensor_vals[VR_ELEV+3]));
		vrArgParseFloat(args, "roll", &(aux->def_6sensor_vals[VR_ROLL+3]));


		/***************************************/
		/** Argument format: "oob" "=" number **/
		/***************************************/
		vrArgParseBool(args, "oob", &(aux->def_oob_value));

		/******************************************/
		/** Argument format: "button" "=" number **/
		/******************************************/
		vrArgParseBool(args, "button", &(aux->def_button_value));

		/********************************************/
		/** Argument format: "valuator" "=" number **/
		/********************************************/
		vrArgParseFloat(args, "valuator", &(aux->def_valuator_value));

		/******************************************/
		/** Argument format: "switch" "=" number **/
		/******************************************/
		vrArgParseInteger(args, "switch", &(aux->def_Nswitch_value));
	}

	aux->def_6sensor = vrMatrixSetXYZAERAf(vrMatrixCreate(), aux->def_6sensor_vals);
}


/**************************************************************/
void _StaticGetData(vrInputDevice *devinfo)
{
	_StaticPrivateInfo	*aux = (_StaticPrivateInfo *)devinfo->aux_data;
	vr2switch		*current_2switch;
	vrValuator		*current_valuator;
	int			*current_value;
	int			new_value;
	float			sine_value;
	int			count;

/* TODO: clean this up -- not sure if this is a good way to do this, but the whole 6-sensor sinewave code is really just in a test phase. */
static	vrEuler		sensor6_delta = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
static	vr6sensorConv	sensor6_options = { VR_Y, 0, 0, 0, 0, 0, 1, 0, 0, 0.001, 5.0, /* working volume: */ -5.0, 0.0, -5.0, 5.0, 10.0, 5.0 };


	/* only need to worry about the toggles and sinewaves */

	/***********/
	/* toggles */
	for (count = 0; count < aux->num_toggles; count++) {
		current_2switch = aux->toggle_inputs[count];
		current_value = &(aux->toggle_values[count]);

		new_value = (cos((vrCurrentWallTime() + aux->toggle_pshift[count]) /
			(aux->toggle_period[count] * M_2_PI)) >= 0 ? 0 : 1);

		if (*current_value != new_value) {
			*current_value = new_value;

			switch (current_2switch->input_type) {
			case VRINPUT_BINARY:
				vrDbgPrintfN(STATIC_DBGLVL, "_StaticGetData: "
					"Assigning a value of %d to static toggle %d\n", new_value, count);
				vrAssign2switchValue(current_2switch, new_value);
				break;

			case VRINPUT_CONTROL:
				vrDbgPrintfN(STATIC_DBGLVL, "_StaticGetData: "
					"Activating a callback with %d to static toggle %d\n", new_value, count);
				vrCallbackInvokeDynamic(((vrControl *)current_2switch)->callback, 1, new_value);
				break;

			default:
				vrErrPrintf(RED_TEXT "_StaticGetData: Unable to handle toggle inputs that aren't Binary or Control inputs\n" NORM_TEXT);
				break;
			}
		}
	}

	/*************/
	/* sinewaves */
	for (count = 0; count < aux->num_sinewaves; count++) {
		current_valuator = aux->sinewave_inputs[count];

#if 1
		sine_value = cos((vrCurrentWallTime() + aux->sinewave_pshift[count]) / (aux->sinewave_period[count] * M_PI_2));
#else
		sine_value = cos(((vrCurrentWallTime() * aux->sinewave_period[count]) + aux->sinewave_pshift[count]) / M_PI_2);
#endif

		switch (current_valuator->input_type) {
		case VRINPUT_VALUATOR:
			vrDbgPrintfN(STATIC_DBGLVL, "_StaticGetData: "
				"Assigning a value of %.2f to static sinewave %d\n", sine_value, count);
			vrAssignValuatorValue(current_valuator, sine_value);
			break;

		case VRINPUT_6SENSOR:
			vrDbgPrintfN(STATIC_DBGLVL, "_StaticGetData: "
				"Hey, got a 6-sensor with a sinewave!  (%.2f - %d)\n", sine_value, count);
			sensor6_delta.t[VR_X] = sine_value;
			vrAssign6sensorValueFromValuators((vr6sensor *)current_valuator, sensor6_delta.t, &sensor6_options, -1);
			break;

		case VRINPUT_CONTROL:
			vrDbgPrintfN(STATIC_DBGLVL, "_StaticGetData: "
				"Activating a callback with %.2f to static sinewave %d\n", sine_value, count);
			vrCallbackInvokeDynamic(((vrControl *)current_valuator)->callback, 1, (int)sine_value);
			break;

		default:
			vrErrPrintf(RED_TEXT "_StaticGetData: Unable to handle sinewave inputs that aren't Valuator or Control inputs\n" NORM_TEXT);
			break;
		}
	}

}


	/***************************************************************/
	/*    Function(s) for parsing Static "input" declarations.     */
	/*                                                             */
	/*  These _Static<type>Input() functions are called during the */
	/*  CREATE phase of the input interface.                       */

/**************************************************************************/
static vrInputMatch _StaticConstant2switchInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
	_StaticPrivateInfo  *aux = (_StaticPrivateInfo *)devinfo->aux_data;

	if (strlen(dti->instance) > 0)
		vrAssign2switchValue((vr2switch *)input, vrAtoI(dti->instance) /*, vrCurrentWallTime() */);
	else	vrAssign2switchValue((vr2switch *)input, aux->def_button_value /*, vrCurrentWallTime() */);

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/**************************************************************************/
static vrInputMatch _StaticConstantNswitchInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
	_StaticPrivateInfo  *aux = (_StaticPrivateInfo *)devinfo->aux_data;

	if (strlen(dti->instance) > 0)
		vrAssignNswitchValue((vrNswitch *)input, vrAtoI(dti->instance) /*, vrCurrentWallTime() */);
	else	vrAssignNswitchValue((vrNswitch *)input, aux->def_Nswitch_value /*, vrCurrentWallTime() */);

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/**************************************************************************/
static vrInputMatch _StaticConstantValuatorInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
	_StaticPrivateInfo  *aux = (_StaticPrivateInfo *)devinfo->aux_data;

	if (strlen(dti->instance) > 0)
		vrAssignValuatorValue((vrValuator *)input, atof(dti->instance) /*, vrCurrentWallTime() */);
	else	vrAssignValuatorValue((vrValuator *)input, aux->def_valuator_value /*, vrCurrentWallTime() */);

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/**************************************************************************/
static vrInputMatch _StaticConstant6sensorInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
	_StaticPrivateInfo  *aux = (_StaticPrivateInfo *)devinfo->aux_data;

	if (strlen(dti->instance) > 0) {
		if (!strncasecmp(dti->instance, "id", 2)) {
			/* do nothing, since it was already initialized to the Identity matrix */
		} else {
			/* do something, but as yet undetermined */
#if 0
			vrAssign...Value((vr6sensor *)input, atof(input_args) /*, vrCurrentWallTime() */);
#endif
		}
	} else {
		vrAssign6sensorValue((vr6sensor *)input, aux->def_6sensor, aux->def_oob_value /*, vrCurrentWallTime() */);
	}

	/* the vrAssign6sensorR2Exform() function does the parsing of the next token */
	vrAssign6sensorR2Exform((vr6sensor *)input, strchr(dti->instance, ','));

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/**************************************************************************/
static vrInputMatch _StaticToggle2switchInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
	_StaticPrivateInfo	*aux = (_StaticPrivateInfo *)devinfo->aux_data;
	char			*time_period;			/* period of the toggling */
	char			*time_pshift;			/* phase shift of toggling */
	int			toggle_num;

	if (aux->num_toggles == MAX_TOGGLES) {
		/* no room at the input */
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_StaticContactInput: "
			"Warning, too many ""toggle"" inputs, ignoring toggle['%s'].\n",
			dti->instance);

		return VRINPUT_MATCH_UNABLE;	/* input declaration match, but unable to create */
	} else {
		time_period = dti->instance;
		time_pshift = strchr(dti->instance, ',');

		/* add "toggle" as an input */
		toggle_num = aux->num_toggles;
		aux->num_toggles++;

		aux->toggle_inputs[toggle_num] = (vr2switch *)input;
		aux->toggle_period[toggle_num] = atof(time_period);
		if (aux->toggle_period[toggle_num] == 0)
			aux->toggle_period[toggle_num] = 10.0;		/* default to 10.0 second period */
		if (time_pshift != NULL)
			aux->toggle_pshift[toggle_num] = atof(time_pshift+1);
		else	aux->toggle_pshift[toggle_num] = 0.0;

		aux->toggle_values[toggle_num] =
			(cos((vrCurrentWallTime() + aux->toggle_pshift[toggle_num]) /
			     (aux->toggle_period[toggle_num] * M_2_PI)) >= 0 ? 0 : 1);

		/* assign initial value */
		vrAssign2switchValue((vr2switch *)input, aux->toggle_values[toggle_num] /*, vrCurrentWallTime() */);

		vrDbgPrintfN(STATIC_DBGLVL, "_StaticToggle2switchInput: "
			"assigned toggle '%s' to static 2switch #%d (type %d)\n",
			dti->instance, toggle_num, input->input_type);
	}

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/**************************************************************************/
static vrInputMatch _StaticSinewaveValuatorInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
	_StaticPrivateInfo	*aux = (_StaticPrivateInfo *)devinfo->aux_data;
	char			*time_period;			/* period of the toggling */
	char			*time_pshift;			/* phase shift of toggling */
	int			sinewave_num;

	if (aux->num_sinewaves == MAX_SINEWAVES) {
		/* no room at the input */
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_StaticContactInput: "
			"Warning, too many ""sinewave"" inputs, ignoring sinewave['%s'].\n",
			dti->instance);

		return VRINPUT_MATCH_UNABLE;	/* input declaration match, but unable to create */
	} else {
		time_period = dti->instance;
		time_pshift = strchr(dti->instance, ',');

		/* add "sinewave" as an input */
		sinewave_num = aux->num_sinewaves;
		aux->num_sinewaves++;

		aux->sinewave_inputs[sinewave_num] = (vrValuator *)input;
		aux->sinewave_period[sinewave_num] = atof(time_period);
		if (aux->sinewave_period[sinewave_num] == 0)
			aux->sinewave_period[sinewave_num] = 10.0;		/* default to 10.0 second period */
		if (time_pshift != NULL)
			aux->sinewave_pshift[sinewave_num] = atof(time_pshift+1);
		else	aux->sinewave_pshift[sinewave_num] = 0.0;

		aux->sinewave_values[sinewave_num] =
			cos((vrCurrentWallTime() + aux->sinewave_pshift[sinewave_num]) /
			    (aux->sinewave_period[sinewave_num] * M_2_PI));

		/* assign initial value */
		vrAssignValuatorValue((vrValuator *)input, aux->sinewave_values[sinewave_num] /*, vrCurrentWallTime() */);

		vrDbgPrintfN(STATIC_DBGLVL, "_StaticSinewave2switchInput: "
			"assigned sinewave '%s' to static 2switch #%d (type %d)\n",
			dti->instance, sinewave_num, input->input_type);
	}

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


	/************************************************************/
	/***************** FreeVR Callback routines *****************/
	/************************************************************/

	/********************************************************/
	/*    Callbacks for controlling the Static features.    */
	/*                                                      */

/************************************************************/
static void _StaticSystemPauseToggleCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

	devinfo->context->paused ^= 1;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Static Control: system_pause = %d.\n",
		devinfo->context->paused);
}

/************************************************************/
static void _StaticPrintContextStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintContext(stdout, devinfo->context, verbose);
}

/************************************************************/
static void _StaticPrintConfigStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintConfig(stdout, devinfo->context->config, verbose);
}

/************************************************************/
static void _StaticPrintInputStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintInput(stdout, devinfo->context->input, verbose);
}

/************************************************************/
static void _StaticPrintStructCallback(vrInputDevice *devinfo, int value)
{
	_StaticPrivateInfo  *aux = (_StaticPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	_StaticPrintStruct(stdout, aux, verbose);
}

/************************************************************/
static void _StaticPrintHelpCallback(vrInputDevice *devinfo, int value)
{
        _StaticPrivateInfo  *aux = (_StaticPrivateInfo *)devinfo->aux_data;

        if (value == 0)
                return;

        _StaticPrintHelp(stdout, aux);
}



	/********************************************************/
	/*   Callbacks for interfacing with the Static device.  */
	/*                                                      */

/**************************************************************************/
static void _StaticCreateFunction(vrInputDevice *devinfo)
{
	/*** List of possible inputs ***/
static	vrInputFunction	_StaticInputs[] = {
				{ "toggle", VRINPUT_2WAY, _StaticToggle2switchInput },
				{ "sinewave", VRINPUT_VALUATOR, _StaticSinewaveValuatorInput },

				{ "constant", VRINPUT_2WAY, _StaticConstant2switchInput },
				{ "constant", VRINPUT_NWAY, _StaticConstantNswitchInput },
				{ "constant", VRINPUT_VALUATOR, _StaticConstantValuatorInput },
				{ "constant", VRINPUT_6SENSOR, _StaticConstant6sensorInput },
				{ "sinewave", VRINPUT_6SENSOR, _StaticSinewaveValuatorInput },
				{ NULL, VRINPUT_UNKNOWN, NULL } };
	/*** List of control functions ***/
static	vrControlFunc	_StaticControlList[] = {
				/* overall system controls */
				{ "system_pause_toggle", _StaticSystemPauseToggleCallback },

				/* informational output controls */
				{ "print_context", _StaticPrintContextStructCallback },
				{ "print_config", _StaticPrintConfigStructCallback },
				{ "print_input", _StaticPrintInputStructCallback },
				{ "print_struct", _StaticPrintStructCallback },
				{ "print_help", _StaticPrintHelpCallback },

		/** TODO: other callback control functions go here **/
				/* simulated 6-sensor selection controls */
				/* simulated 6-sensor manipulation controls */
				/* other controls */

				/* end of the list */
				{ NULL, NULL } };

	_StaticPrivateInfo	*aux = NULL;

	/******************************************/
	/* allocate and initialize auxiliary data */
	devinfo->aux_data = (void *)vrShmemAlloc0(sizeof(_StaticPrivateInfo));
	aux = (_StaticPrivateInfo *)devinfo->aux_data;

	/******************/
	/* handle options */
	_StaticParseArgs(aux, devinfo->args);

	/***************************************/
	/* create the inputs and self-controls */
	vrInputCreateDataContainers(devinfo, _StaticInputs);
	vrInputCreateSelfControlContainers(devinfo, _StaticInputs, _StaticControlList);
	/* TODO: anything to do for NON self-controls?  Implement for Magellan first. */

	vrDbgPrintf("_StaticCreateFunction(): Done creating Static inputs for '%s'\n", devinfo->name);
	devinfo->created = 1;

	return;
}


/**************************************************************************/
static void _StaticOpenFunction(vrInputDevice *devinfo)
{
	vrTrace("_StaticOpenFunction", devinfo->name);

	/*******************/
	/* open the device */

	/* nothing to do */

	vrDbgPrintf("_StaticOpenFunction(): Done opening Static for input device '%s'\n", devinfo->name);
	devinfo->operating = 1;

	return;
}


/**************************************************************************/
static void _StaticCloseFunction(vrInputDevice *devinfo)
{
	return;
}


/**************************************************************************/
static void _StaticResetFunction(vrInputDevice *devinfo)
{
	_StaticPrivateInfo	*aux = (_StaticPrivateInfo *)devinfo->aux_data;

	_StaticParseArgs(aux, devinfo->args);
	return;
}


/**************************************************************************/
static void _StaticPollFunction(vrInputDevice *devinfo)
{
	if (devinfo->operating) {
		_StaticGetData(devinfo);
	}

	return;
}


	/**********************************************************/
	/***************** FreeVR public routines *****************/
	/**********************************************************/


/**************************************************************************/
void vrStaticInitInfo(vrInputDevice *devinfo)
{
	devinfo->version = (char *)vrShmemStrDup("The Static input device, version 0.2");
	devinfo->Create = vrCallbackCreateNamed("StaticInput:Create-Def", _StaticCreateFunction, 1, devinfo);
	devinfo->Open = vrCallbackCreateNamed("StaticInput:Open-Def", _StaticOpenFunction, 1, devinfo);
	devinfo->Close = vrCallbackCreateNamed("StaticInput:Close-Def", _StaticCloseFunction, 1, devinfo);
	devinfo->Reset = vrCallbackCreateNamed("StaticInput:Reset-Def", _StaticResetFunction, 1, devinfo);
	devinfo->PollData = vrCallbackCreateNamed("StaticInput:PollData-Def", _StaticPollFunction, 1, devinfo);
	devinfo->PrintAux = vrCallbackCreateNamed("StaticInput:PrintAux-Def", _StaticPrintStruct, 0);

	vrDbgPrintfN(STATIC_DBGLVL, "vrStaticInitInfo: callbacks created.\n");
}

