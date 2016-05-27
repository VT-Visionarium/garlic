/* ======================================================================
 * 
 *  CCCCC          vr_input.vrpn.cxx
 * CC   CC         Author(s): Bill Sherman
 * CC              Created: March 15, 2000
 * CC   CC         Last Modified: May 4, 2001
 *  CCCCC
 * 
 * Code file for FreeVR inputs from the VRPN input device.
 * 
 * Copyright 2001, Bill Sherman & Friends, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/*************************************************************************

USAGE:
	Of course, to use VRPN as an input device, the VRPN server must
	be up and running prior to starting the FreeVR application.

	Here is an example of how to run VRPN on cassatt:
		(from the "/vr/apps/vrpn/current/vrpn" directory)
			A% server_src/sgi_irix.n32.mips3/vrpn_server -f vrpn.cassatt_full.cfg
			B% client_src/sgi_irix.n32.mips3/printvals cassatt
		  or
			B% client_src/sgi_irix.n32.mips3/printvals localhost


	Inputs are specified with the "input" option:
		Here are two possible configuration styles:
			(going with style 1 for now)
			[though today (5/4/01), while working on the r2e stuff
			 it seems as though we might be better off with style 2.]

	     style 1:
		input "<name>" = "<freevr-type>(<VRPN-type>[<device>@<host>, <num>])";

		input "<name>" = "2switch(button[<device>@<host>, <num>])";
		input "<name>" = "valuator(analog[<device>@<host>, <num>{, scale}])";
		input "<name>" = "6sensor(tracker[<device>@<host>, <num>{, {id|r2e|xform}}])";
	 ? :-(	input "<name>" = "6sensor(phantom[<device>@<host>, <num>])";
	  :-(	input "<name>" = "6sensor(analog[<device>@<host>, <num>])";

	     style 2:
		input "<name>" = "<freevr-type>(<VRPN-type>:<device>@<host>[<num>])";

		input "<name>" = "2switch(button:<device>@<host>[<num>])";
		input "<name>" = "valuator(analog:DBBox@<host>[<num>{, scale]})";
		input "<name>" = "6sensor(tracker:<device>@<host>[<num>{, {id|r2e|xform}}])";
		input "<name>" = "6sensor(phantom:<device>@<host>[<num>])";
		input "<name>" = "6sensor(analog:DBBox@<host>[<num>])";


		future possibilities:

	  :-(	input "<name>" = "Nswitch(switch[<number>])";
	  :-(	input "<name>" = "Nsensor(glove[<number>])";


	Controls are specified in the freevrrc file:
		control "<control option>" = "2switch(button[<device>@<host>, <num>])";
	  ...

	Here are the available control options for FreeVR:
		"print_context" -- print the overall FreeVR context data structure (for debugging)
		"print_config" -- print the overall FreeVR config data structure (for debugging)
		"print_input" -- print the overall FreeVR input data structure (for debugging)
		"print_struct" -- print the internal VRPN data structure (for debugging)
	  :-(	"print_help" -- print info on how to use the input device
	  ...

	Here are the FreeVR configuration argument options for the VRPN:
	 	"valScale" - float value of the valuator sensitivity scale
	?  :-(	"port" - socket port VRPN is connected to
			(4500 is the default)
	  ...


	NOTE: VRPN inputs cannot be combined with an X-windows configuration that
		uses extension devices of X.  It's not clear why this is, it just
		is.

HISTORY:
	15 March 2000 (Bill Sherman)
		I used (newly created) vr_input.skeleton.c to help create the
		  initial version.

	12 April 2001 (Bill Sherman) -- mid mod
		I began getting all the proper variables, etc. laid out.  At
		  this point, it compiles, and calls the input creation
		  routines for the inputs -- though doesn't really create
		  them yet.  But at least it works with the mixed C++ dso and
		  normal C application (using travel_n32), and it does call
		  the input creation routines.

	30 April 2001 (Bill Sherman) -- mid mod
		I managed to get the Button and Analog inputs working.  First
		  problem solved was find the cause of the VRPN server hangs --
		  oddly enough an Xwindows function: XListInputDevices().  By
		  not calling this, things work pretty well.

	 3 May 2001 (Bill Sherman) -- mid mod
		I now have tracker sensor inputs working, and have added
		  new 'print_context' and 'print_config' control options
		  not only here, but to all input devices.

		The BIG NEWS is that after getting the basics to work, I
		  decided to try getting the dso to work with a normally
		  compiled C application.  And it now works!  Hooray for
		  Captain Spaulding!

	 4 May 2001 (Bill Sherman)
		I added the r2e transform option to the 6-sensor inputs, and
		  a scale factor argument for valuators.
		I also did some general cleanup, since this file is mostly
		  working now -- though still need to add Phantom, etc.

TODO:

	- test with an operating system other than SGI/IRIX  (currently
		verified to work on both an Indigo 2, and an Onyx 2)

	- DONE: need to allow scaling of valuator inputs to be specified

	- DONE: need to allow transforms to be applied to sensor inputs

	- make calls to VRPN shut down operations when device is closed

	- figure out what values to assign the 'version' and 'operating parameter' fields

	- delete old DONOTMOVETO_INITDEV code

	- add "phantom" type ??

	- allow a FreeVR-created 6-sensor from valuator inputs to be created.
		(ie. can either use the VRPN server options for creating a
		"tracker" from analog inputs, or can use the standard FreeVR
		method such as is available in the raw Magellan code).

	- determine whether old analog values can be saved, and compared with
		incoming analog values, reporting only those that have changed.

	- test for the existance of the VRPN library functions before
		calling them (to prevent this process from crashing).
		At some point we can try to load VRPN as a DSO if the
		function isn't found.
		(Put in _VrpnInitializeDevice().)
		I tried this, but can't seem to get it to work -- 5/3/01.

	- get TEST_APP working

	- figure out why calling XListInputDevices() messes up the
		VRPN server.


**************************************************************************/
#if !defined(FREEVR) && !defined(TEST_APP) && !defined(CAVE)
/* Choose how this code should be compiled (ie. for CAVE, standalone, etc). */
#  define	FREEVR
#  undef	TEST_APP
#  undef	CAVE
#endif

#undef	DEBUG


#include <stdio.h>
#include <string.h>
#include <signal.h>

#if defined(__cplusplus)
extern "C" {
#endif

#include "vr_serial.h"
#include "vr_debug.h"
#include "vr_utils.h"

#if defined (FREEVR)
#if 1
#  include "vr_input.h"
#  include "vr_input.opts.h"
#endif
#  include "vr_parse.h"
#  include "vr_shmem.h"
#endif

#if defined(__cplusplus)
}
#endif


#if defined(TEST_APP) || defined(CAVE)
#  define	X	0
#  define	Y	1
#  define	Z	2
#  define	AZIM	3
#  define	ELEV	4
#  define	ROLL	5
#endif

#if defined(TEST_APP)
/* serial stuff not needed for VRPN */
#endif


#if defined(CAVE) /* { */
#  include <math.h>
#  include "cave.h"
#  include "cave.private.h"
#  include "cave.tracker.h"
static CAVE_SENSOR_ST initial_cave_sensor = {
		0,5,0,
		0,0,0,
		0,	/*timestamp*/
		FALSE,	/*calibrated*/
		CAVE_TRACKER_FRAME
	};
#endif /* } CAVE */


/*** special VRPN includes ***/
#include "vrpn/vrpn_Tracker.h"
#include "vrpn/vrpn_Button.h"
#include "vrpn/vrpn_Analog.h"

#undef DO_EXTERN_C
#if defined(__cplusplus) && defined(DO_EXTERN_C)
extern "C" {
#endif


	/******************************************************************/
	/***      definitions for interfacing with the VRPN device      ***/
	/***                                                            ***/

/* VRPN sensitivity values */
#define VALUATOR_SENSITIVITY	17.0	/* empiracally determined */


/*************************************************************/
/*** private structure of the current data from the VRPN. ***/
typedef struct {
		/* these are for interfacing with the hardware */

#define MULTI_DEVS 8 /* there can be <n> multiple devices of each type */

		int		num_button_devices;		/* num of button devices used */
		char		button_names[MULTI_DEVS][256];	/* name of the button devices */
	   vrpn_Button_Remote	*vrpn_button[MULTI_DEVS];	/* VRPN button class */

		int		num_analog_devices;		/* num of analog devices used */
		char		analog_names[MULTI_DEVS][256];	/* name of the analog devices */
	   vrpn_Analog_Remote	*vrpn_analog[MULTI_DEVS];	/* VRPN analog class */

		int		num_tracker_devices;		/* num of tracker devices used */
		char		tracker_names[MULTI_DEVS][256];	/* names of the tracker devices */
	   vrpn_Tracker_Remote	*vrpn_tracker[MULTI_DEVS];	/* VRPN tracker class */

		/* TODO: other VRPN classes include Text, Phantom?, ... */

		int		open;		/* flag with VRPN successfully open */

		/* these are for internal data parsing */
		char		version[512];	/* self-reported version of the device */
		char		op_params[256];	/* operating parameters of the device (according to it) */

#define MAX_BUTTONS	16
#define MAX_VALUATORS	16
#define MAX_6SENSORS	16
#ifdef CAVE
		/* CAVE specific stuff here */

#elif defined(FREEVR)
	/* FREEVR stuff */

		vr2switch	*button_inputs[MULTI_DEVS][MAX_BUTTONS];	/* each button device can have <n> buttons */
		vrValuator	*valuator_inputs[MULTI_DEVS][MAX_VALUATORS];	/* ditto for analog */
		vr6sensor	*sensor6_inputs[MULTI_DEVS][MAX_6SENSORS];	/* ditto for trackers */

#if 0 /* TODO: not sure if we want/need this */
#define MAX_CONTROLS  10
		vrControl	*control_inputs[MAX_CONTROLS];
#endif

#endif /* end library-specific fields */

		/* information about the current values */
#if 0
		int		data[6];	/* incoming data */
		int		buttons[MAX_BUTTONS];/* incoming button info */
		int		data_change;	/* boolean indicator if any values have changed*/
#endif
		int		button_change;	/* (specific) boolean indicator if button values have changed*/
		int		valuator_change;/* (specific) boolean indicator of change in valuator values */
		int		receiver_change;/* (specific) boolean indicator of change in receiver values */

		/* information about how to filter the data for use with the VR library (FreeVR or CAVE) */
		float		scale_valuator;	/* scaling factor for valuators */

	} _VrpnPrivateInfo;


/***********************************************************/
/*** structure for the userdata of the callback handles. ***/
typedef struct {
		int		  num;		/* the number in the device list of this type */
		_VrpnPrivateInfo *aux;		/* a pointer to the overall private info structure */
	} _VrpnUserdata;



	/******************************************************/
	/*** General NON public VRPN interface routines ***/
	/******************************************************/

/******************************************************/
static void _VrpnInitializeStruct(_VrpnPrivateInfo *aux)
{
	aux->version[0] = '\0';
	aux->op_params[0] = '\0';

	aux->num_button_devices = 0;
	aux->num_analog_devices = 0;
	aux->num_tracker_devices = 0;

	aux->button_change = 0;
	aux->valuator_change = 0;
	aux->receiver_change = 0;

	aux->scale_valuator = VALUATOR_SENSITIVITY;

	/* everything else is zero'd by default */
}


/******************************************************/
static void _VrpnPrintStruct(FILE *file, _VrpnPrivateInfo *aux)
{
	int		count;
	int		count2;
	vrGenericInput	*input;

	vrFprintf(file, "VRPN device internal structure:\n");
	vrFprintf(file, "\r\tversion -- '%s'\n", aux->version);
	vrFprintf(file, "\r\toperating parameters -- '%s'\n", aux->op_params);
	vrFprintf(file, "\r\topen = %d\n", aux->open);

	vrFprintf(file, "\r\tscale_valuator = %f\n", aux->scale_valuator);

#ifdef FREEVR /* { */
	/* button inputs */
	vrFprintf(file, "\r\tbutton_devices = %d:\n", aux->num_button_devices);
	for (count = 0; count < aux->num_button_devices; count++) {
		vrFprintf(file, "\r\t\tbutton[%d] = '%s' (0x%p)\n",
			count,
			aux->button_names[count],
			aux->vrpn_button[count]);
		for (count2 = 0; count2 < MAX_BUTTONS; count2++) {
			input = (vrGenericInput *)aux->button_inputs[count][count2];
			if (input != NULL) {
				vrFprintf(file, "\r\t\t\tbutton %d: input = 0x%p (%s:%s)\n",
					count2, input, vrInputTypeName(input->input_type), input->my_object->name);
			}
		}
	}

	/* analog inputs */
	vrFprintf(file, "\r\tanalog_devices = %d:\n", aux->num_analog_devices);
	for (count = 0; count < aux->num_analog_devices; count++) {
		vrFprintf(file, "\r\t\tanalog[%d] = '%s' (0x%p)\n",
			count,
			aux->analog_names[count],
			aux->vrpn_analog[count]);
		for (count2 = 0; count2 < MAX_VALUATORS; count2++) {
			input = (vrGenericInput *)aux->valuator_inputs[count][count2];
			if (input != NULL) {
				vrFprintf(file, "\r\t\t\tanalog %d: input = 0x%p (%s:%s)\n",
					count2, input, vrInputTypeName(input->input_type), input->my_object->name);
			}
		}
	}

	/* tracker inputs */
	vrFprintf(file, "\r\ttracker_devices = %d:\n", aux->num_tracker_devices);
	for (count = 0; count < aux->num_tracker_devices; count++) {
		vrFprintf(file, "\r\t\ttracker[%d] = '%s' (0x%p)\n",
			count,
			aux->tracker_names[count],
			aux->vrpn_tracker[count]);
		for (count2 = 0; count2 < MAX_6SENSORS; count2++) {
			input = (vrGenericInput *)aux->sensor6_inputs[count][count2];
			if (input != NULL) {
				vrFprintf(file, "\r\t\t\ttracker %d: input = 0x%p (%s:%s)\n",
					count2, input, vrInputTypeName(input->input_type), input->my_object->name);
			}
		}
	}
#endif /* } FREEVR */

	/* TODO: print some info about the current values */

}


/**************************************************************************/
static void _VrpnPrintHelp(FILE *file, _VrpnPrivateInfo *aux)
{
	vrFprintf(file, BOLD_TEXT "Sorry, VRPN - print_help control not yet implemented.\n" NORM_TEXT);
}


/******************************************************/
static char _VrpnReadInput(_VrpnPrivateInfo *aux)
{
	char	return_code = '\1';	/* currently not watching for any problems */
	int	count;

	for (count = 0; count < aux->num_button_devices; count++) {
		if (aux->vrpn_button[count] != NULL)
			aux->vrpn_button[count]->mainloop();
		else	vrErrPrintf(RED_TEXT "_VrpnReadInput: aux->vrpn_button[%d] shouldn't be NULL\n" NORM_TEXT, count);
	}

	for (count = 0; count < aux->num_analog_devices; count++) {
		if (aux->vrpn_analog[count] != NULL)
			aux->vrpn_analog[count]->mainloop();
		else	vrErrPrintf(RED_TEXT "_VrpnReadInput: aux->vrpn_analog[%d] shouldn't be NULL\n" NORM_TEXT, count);
	}

	for (count = 0; count < aux->num_tracker_devices; count++) {
		if (aux->vrpn_tracker[count] != NULL)
			aux->vrpn_tracker[count]->mainloop();
		else	vrErrPrintf(RED_TEXT "_VrpnReadInput: aux->vrpn_tracker[%d] shouldn't be NULL\n" NORM_TEXT, count);
	}

	return (return_code);
}


/******************************************************/
static void _VrpnHandleButton(_VrpnUserdata *userdata, const vrpn_BUTTONCB but_vrpn)
{
	_VrpnPrivateInfo	*aux;
	vr2switch		*input;			
	int			vrpn_device_num;

#if 0
	vrPrintf("_VrpnHandleButton(): Button %3d is in state: %d\n", but_vrpn.button, but_vrpn.state);
#endif

	if (but_vrpn.button >= MAX_BUTTONS) {
		/* we've gone above the arbitrarily set maximum number of button inputs from any given device */
		fprintf(stderr, "_VrpnHandleButton(): invalid button number from VRPN (%d)\n", but_vrpn.button);
		return;
	}

	aux = userdata->aux;
	vrpn_device_num = userdata->num;

	input = aux->button_inputs[vrpn_device_num][but_vrpn.button];

	vrDbgPrintfN(INPUT_DBGLVL,
		"_VrpnHandleButton(): device = %d, button = %d, state = %d, input = 0x%p\n",
		vrpn_device_num, but_vrpn.button, but_vrpn.state, input);

	if (input != NULL) {
		switch (input->input_type) {
		case VRINPUT_BINARY:
			vrAssign2switchValue(input, but_vrpn.state);
			break;
		case VRINPUT_CONTROL:
			vrInvokeDynamicCallback(((vrControl *)(input))->callback, 1, but_vrpn.state);
			break;
		default:
			vrErrPrintf(RED_TEXT "_VrpnHandleButton: Unable to handle button inputs that aren't Binary or Control inputs\n" NORM_TEXT);
			break;
		}
	}


#ifdef CAVE
	vrpn_controls.button[but_vrpn.button] = but_vrpn.state;
#endif
}

/******************************************************/
static void _VrpnHandleAnalog(_VrpnUserdata *userdata, const vrpn_ANALOGCB analog_vrpn)
{
	_VrpnPrivateInfo	*aux;
	vrValuator		*input;			
	int			vrpn_device_num;
	int			count;			/* for looping over each analog channel */

#if 0
	vrPrintf("_VrpnHandleAnalog: Analog has %3d channels w/ values: %lf %lf %lf ...\n",
		analog_vrpn.num_channel, analog_vrpn.channel[0], analog_vrpn.channel[1], analog_vrpn.channel[2]);
#endif

	aux = userdata->aux;
	vrpn_device_num = userdata->num;

	for (count = 0; count < analog_vrpn.num_channel && count < MAX_VALUATORS; count++) {
		input = aux->valuator_inputs[vrpn_device_num][count];

		vrDbgPrintfN(INPUT_DBGLVL,
			"_VrpnHandleAnalog(): device = %d, analog channel = %d, value = %f, input = 0x%p\n",
			vrpn_device_num, count, analog_vrpn.channel[count], input);

		if (input != NULL) {
			switch (input->input_type) {
			case VRINPUT_VALUATOR:
				vrAssignValuatorValue(input, analog_vrpn.channel[count] * aux->scale_valuator);
				break;

			default:
				/* TODO: ... */
				break;
			}
		}
	}

#ifdef CAVE
	vrpn_controls.valuator[0] = analog_vrpn.channel[0] * 3.5;
	vrpn_controls.valuator[1] = analog_vrpn.channel[1] * 3.5;
#endif
}


/******************************************************/
static void _VrpnHandleTracker(_VrpnUserdata *userdata, const vrpn_TRACKERCB tracker_vrpn)
{
	_VrpnPrivateInfo	*aux;
	vr6sensor		*input;
	vrMatrix		tmpmat;
	vrQuat			tmpquat;
	int			vrpn_device_num;

#if 0
	vrPrintf("_VrpnHandleTracker: Sensor %3d is now at (%11f, %11f, %11f)\n", 
        	tracker_vrpn.sensor, tracker_vrpn.pos[0], tracker_vrpn.pos[1], tracker_vrpn.pos[2]);
#endif

	if (tracker_vrpn.sensor >= MAX_6SENSORS) {
		/* we've gone above the arbitrarily set maximum number of tracker inputs from any given device */
		fprintf(stderr, "_VrpnHandleTracker(): invalid sensor number from VRPN (%d)\n", tracker_vrpn.sensor);
		return;
	}

	aux = userdata->aux;
	vrpn_device_num = userdata->num;

	input = aux->sensor6_inputs[vrpn_device_num][tracker_vrpn.sensor];

	vrDbgPrintfN(INPUT_DBGLVL,
		"_VrpnHandleTracker(): device = %d, sensor# = %d, loc = (%5.3f %5.3f %5.3f), input = 0x%p\n",
		vrpn_device_num, tracker_vrpn.sensor,
		tracker_vrpn.pos[0], tracker_vrpn.pos[1], tracker_vrpn.pos[2],
		input);


	if (input != NULL) {
		switch (input->input_type) {
		case VRINPUT_6SENSOR:
			vrMatrixSetTranslation3d(&tmpmat, tracker_vrpn.pos[0], tracker_vrpn.pos[1], tracker_vrpn.pos[2]);	/* this sets rot to ID */

			tmpquat.v[0] = tracker_vrpn.quat[0];
			tmpquat.v[1] = tracker_vrpn.quat[1];
			tmpquat.v[2] = tracker_vrpn.quat[2];
			tmpquat.v[3] = tracker_vrpn.quat[3];
			vrMatrixSetRotationFromQuat(&tmpmat, &tmpquat);

			vrAssign6sensorValue(input, &tmpmat, 0 /*, TODO: timestamp */);
			break;
		default:
			vrErrPrintf(RED_TEXT "_VrpnHandleTracker(): Unable to handle VRPN tracker inputs that aren't 6-sensor inputs\n" NORM_TEXT);
			break;
		}
	}

#ifdef CAVE
	/* raise value of num_sensors when info from a larger sensor number is recieved */
	if (tracker_dev.sensor > num_sensors) {
		num_sensors = tracker_dev.sensor+1;
		printf("CAVE VRPN-tracker: num_sensors is now %d\n", num_sensors);
	}

	vrpn_sensor[tracker_vrpn.sensor].x = tracker_vrpn.pos[0];
	vrpn_sensor[tracker_vrpn.sensor].y = tracker_vrpn.pos[1];
	vrpn_sensor[tracker_vrpn.sensor].z = tracker_vrpn.pos[2];

	... = tracker_vrpn.quat[0]; (qx)
	... = tracker_vrpn.quat[1]; (qy)
	... = tracker_vrpn.quat[2]; (qz)
	... = tracker_vrpn.quat[3]; (qw)
	vrpn_sensor[tracker_vrpn.sensor].azim = 0.0;
	vrpn_sensor[tracker_vrpn.sensor].elev = 0.0;
	vrpn_sensor[tracker_vrpn.sensor].roll = 0.0;

	CAVEGetTimestamp(&vrpn_sensor[tracker_vrpn.sensor].timestamp);
#endif
}


/******************************************************/
static int _VrpnInitializeDevice(_VrpnPrivateInfo *aux)
{
	int		device_count;		/* for looping through list of each device type */
	_VrpnUserdata	*userdata;		/* struct for passing info to the callback */

	if (aux == NULL)
		return -1;

	/* TODO: verify that the VRPN functions exist before calling them */
	/*   if they don't exist, print a warning and return (-1).        */

#ifndef DONOTMOVETO_INITDEV
	/************************************************/
	/* make the connection(s) to the VRPN server(s) */
	for (device_count = 0; device_count < aux->num_button_devices; device_count++) {
		/* create the userdata for the callback */
		userdata = (_VrpnUserdata *)malloc(sizeof(_VrpnUserdata));
		userdata->num = device_count;
		userdata->aux = aux;

		/* open the VRPN Button connection and set the incoming data callback */
#if 0 /* attempt to test for the existence of the function */
		vrPrintf("About to create new vrpn Button device '%s' (first check for function existance)\n", aux->button_names[device_count]);
		if ((void *)vrpn_Button_Remote == NULL)
			vrPrintf(RED_TEXT "cannot find function 'vrpn_Button_Remote'\n" NORM_TEXT);
#endif
		aux->vrpn_button[device_count] = new vrpn_Button_Remote(aux->button_names[device_count]);
		aux->vrpn_button[device_count]->register_change_handler((void *)userdata, (vrpn_BUTTONCHANGEHANDLER)_VrpnHandleButton);
	}

	for (device_count = 0; device_count < aux->num_analog_devices; device_count++) {
		/* create the userdata for the callback */
		userdata = (_VrpnUserdata *)malloc(sizeof(_VrpnUserdata));
		userdata->num = device_count;
		userdata->aux = aux;

		/* open the VRPN Analog connection and set the incoming data callback */
		aux->vrpn_analog[device_count] = new vrpn_Analog_Remote(aux->analog_names[device_count]);
		aux->vrpn_analog[device_count]->register_change_handler((void *)userdata, (vrpn_ANALOGCHANGEHANDLER)_VrpnHandleAnalog);
	}

	for (device_count = 0; device_count < aux->num_tracker_devices; device_count++) {
		/* create the userdata for the callback */
		userdata = (_VrpnUserdata *)malloc(sizeof(_VrpnUserdata));
		userdata->num = device_count;
		userdata->aux = aux;

		/* open the VRPN Tracker connection and set the incoming data callback */
		aux->vrpn_tracker[device_count] = new vrpn_Tracker_Remote(aux->tracker_names[device_count]);
		aux->vrpn_tracker[device_count]->register_change_handler((void *)userdata, (vrpn_TRACKERCHANGEHANDLER)_VrpnHandleTracker);
	}
#endif /* DONOTMOVETO_INITDEV */

	/********************/
	/* clear the palate */
	_VrpnReadInput(aux);

	/**********************************/
	/* request version and param info */
	/* TODO: this */

	/********************************/
	/* send some device setup codes */
	/* TODO: this (if necessary) */

#if 0
	/************************/
	/* read some input data */
	_VrpnReadInput(aux);
#endif

	return 0;
}





	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/


#if defined(FREEVR) /* { */
/*****************************************************************/
/***   Functions for FreeVR access of VRPN devices for input   ***/
/*****************************************************************/


	/*************************************/
	/***  FreeVR NON public routines   ***/
	/*************************************/


/*********************************************************/
static void _VrpnParseArgs(_VrpnPrivateInfo *aux, char *args)
{
#if 0
static	char	*XX_choices[] = { "up", "away", NULL };
static	int	XX_values[] = { 1, 0 };
	int	null_value = -1;
#endif
	float	scale_value = -1.0;

#if 0
	/**************************************/
	/** Argument format: "port" "=" file **/
	/**************************************/
	vrArgParseString(args, "port", &(aux->port));
#endif

	/********************************************/
	/** Argument format: "valScale" "=" number **/
	/********************************************/
	if (vrArgParseFloat(args, "valscale", &scale_value)) {
		aux->scale_valuator = scale_value * VALUATOR_SENSITIVITY;
	}

	/** TODO: other arguments to parse go here **/
}


/************************************************************/
static void _VrpnGetData(vrInputDevice *devinfo)
{
	_VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->opaque;

	_VrpnReadInput(aux);

}


	/****************************************************************/
	/*      Function(s) for parsing VRPN "input" declarations.      */
	/*                                                              */

/**************************************************************************/
/*  for example: input "2switch[0]" = "2switch(button[DBBox@localhost, 0])"; */
static int _VrpnButtonInput(vrInputDevice *devinfo, vrGenericInput *input, char *device, char *type, char *instance)
{
        _VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->opaque;
	int			button_num;
	char			vrpn_device[256];
	char			*device_end;
	int			vrpn_device_num;
#ifdef DONOTMOVETO_INITDEV
	_VrpnUserdata		*userdata;		/* struct for passing info to the callback */
#endif

	/* A comma separates the VRPN device name, and the number */
	/*   of the desired input from that device.               */
	device_end = strchr(instance, ',');

	/* If no comma, then report an error in the input specification */
	/*   otherwise add this VRPN field as an input.                 */
	if (device_end != NULL) {
		/* copy just the name of the device into vrpn_device */
		strncpy(vrpn_device, instance, device_end - instance);
		vrpn_device[device_end - instance] = '\0';

		/* the rest of the string should just be a number of the particular sensor */
		button_num = vrAtoI(device_end+1);

		/* now see if the given device name is already on */
		/*   the list of specified button devices.       */
		vrpn_device_num = vrSearch256StringList(vrpn_device, aux->num_button_devices, aux->button_names);

		/* if not, then make a VRPN connection to this button device and add to the list */
		if (vrpn_device_num < 0) {
			vrpn_device_num = aux->num_button_devices;
			vrDbgPrintfN(1 /* TODO: delete this or make INPUT_DBGLVL */, "adding new button device to list: number %d\n", vrpn_device_num);

			/* add device name to the list of VRPN button devices */
			strcpy(aux->button_names[vrpn_device_num], vrpn_device);
			aux->num_button_devices++;
			vrDbgPrintfN(1 /* TODO: make INPUT_DBGLVL */, "aux->button_names[%d] is now '%s'\n", vrpn_device_num, aux->button_names[vrpn_device_num]);

#ifdef DONOTMOVETO_INITDEV
			/* create the userdata for the callback (TODO: or move this to _VrpnInitializeDevice()) */
			userdata = (_VrpnUserdata *)malloc(sizeof(_VrpnUserdata));
			userdata->num = vrpn_device_num;
			userdata->aux = aux;

			/* open the VRPN connection and set the incoming data callback (TODO: or do this in _VrpnInitializeDevice()) */
vrPrintf("About to create new vrpn Button device '%s'\n", vrpn_device);
			aux->vrpn_button[vrpn_device_num] = new vrpn_Button_Remote(vrpn_device);
vrPrintf("About to register Button change handler\n");
			aux->vrpn_button[vrpn_device_num]->register_change_handler((void *)userdata, (vrpn_BUTTONCHANGEHANDLER)_VrpnHandleButton);
vrPrintf("Okay, button now registered\n");
#endif
		}

		vrDbgPrintfN(1 /* TODO: make INPUT_DBGLVL */, "doing button-sensor device '%s' (button %d), %d\n", vrpn_device, vrpn_device_num, button_num);

		/* now assign given input to the existing or new button device */
		aux->button_inputs[vrpn_device_num][button_num] = (vr2switch *)input;

		/* TODO: make initial button assignment */
#if 0 /* still need to make sure the instance stuff is okay */
		vrAssign2switch(aux->button_inputs[button_num], strchr(instance, ','));
#endif
		vrDbgPrintfN(INPUT_DBGLVL, "assigned 2switch event of value 0x%02x to input pointer = 0x%p)\n",
			button_num, aux->button_inputs[vrpn_device_num][button_num]);

	} else {
		vrErrPrintf(RED_TEXT "_VrpnButtonInput: Warning, invalid input specification (%s, %s).\n" NORM_TEXT,
			type, instance);
	}

	return 1;	/* input declaration match */
}


/**************************************************************************/
static int _VrpnAnalogInput(vrInputDevice *devinfo, vrGenericInput *input, char *device, char *type, char *instance)
{
        _VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->opaque;
	int			valuator_num;
	char			vrpn_device[256];
	char			*device_end;
	int			vrpn_device_num;
#ifdef DONOTMOVETO_INITDEV
	_VrpnUserdata		*userdata;		/* struct for passing info to the callback */
#endif

	/* A comma separates the VRPN device name, and the number */
	/*   of the desired input from that device.               */
	device_end = strchr(instance, ',');

	/* If no comma, then report an error in the input specification */
	/*   otherwise add this VRPN field as an input.                 */
	if (device_end != NULL) {
		/* copy just the name of the device into vrpn_device */
		strncpy(vrpn_device, instance, device_end - instance);
		vrpn_device[device_end - instance] = '\0';

		/* the rest of the string should just be a number of the particular sensor */
		valuator_num = vrAtoI(device_end+1);

		/* now see if the given device name is already on */
		/*   the list of specified analog devices.       */
		vrpn_device_num = vrSearch256StringList(vrpn_device, aux->num_analog_devices, aux->analog_names);

		/* if not, then make a VRPN connection to this analog device and add to the list */
		if (vrpn_device_num < 0) {
			vrpn_device_num = aux->num_analog_devices;
			vrDbgPrintfN(1 /* TODO: delete this or make INPUT_DBGLVL */, "adding new analog device to list: number %d\n", vrpn_device_num);

			/* add device name to the list of VRPN analog devices */
			strcpy(aux->analog_names[vrpn_device_num], vrpn_device);
			aux->num_analog_devices++;
			vrDbgPrintfN(1 /* TODO: make INPUT_DBGLVL */, "aux->analog_names[%d] is now '%s'\n", vrpn_device_num, aux->analog_names[vrpn_device_num]);

#ifdef DONOTMOVETO_INITDEV
			/* create the userdata for the callback (TODO: or move this to _VrpnInitializeDevice()) */
			userdata = (_VrpnUserdata *)malloc(sizeof(_VrpnUserdata));
			userdata->num = vrpn_device_num;
			userdata->aux = aux;

			/* open the VRPN connection and set the incoming data callback (TODO: or do this in _VrpnInitializeDevice()) */
vrPrintf("About to create new vrpn Analog device '%s'\n", vrpn_device);
			aux->vrpn_analog[vrpn_device_num] = new vrpn_Analog_Remote(vrpn_device);
vrPrintf("About to register analog change handler\n");
			aux->vrpn_analog[vrpn_device_num]->register_change_handler((void *)userdata, (vrpn_ANALOGCHANGEHANDLER)_VrpnHandleAnalog);
vrPrintf("Okay, analog now registered\n");
#endif
		}

		vrDbgPrintfN(1 /* TODO: make INPUT_DBGLVL */, "doing valuator device '%s', %d\n", vrpn_device, valuator_num);

		/* now assign given input to the existing or new analog device */
		aux->valuator_inputs[vrpn_device_num][valuator_num] = (vrValuator *)input;

		/* TODO: make initial valuator assignment */
#if 0 /* still need to make sure the instance stuff is okay */
		vrAssignValuator(aux->valuator_inputs[valuator_num], strchr(instance, ','));
#endif
		vrDbgPrintfN(INPUT_DBGLVL, "assigned Valuator event of value 0x%02x to input pointer = 0x%p)\n",
			valuator_num, aux->valuator_inputs[vrpn_device_num][valuator_num]);

	} else {
		vrErrPrintf(RED_TEXT "_VrpnAnalogInput: Warning, invalid input specification (%s, %s).\n" NORM_TEXT,
			type, instance);
	}

	return 1;	/* input declaration match */
}


/**************************************************************************/
static int _VrpnTrackerInput(vrInputDevice *devinfo, vrGenericInput *input, char *device, char *type, char *instance)
{
        _VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->opaque;
	int			sensor_num;
	char			vrpn_device[256];
	char			*device_end;
	int			vrpn_device_num;
#ifdef DONOTMOVETO_INITDEV
	_VrpnUserdata		*userdata;		/* struct for passing info to the callback */
#endif

	/* A comma separates the VRPN device name, and the number */
	/*   of the desired input from that device.               */
	device_end = strchr(instance, ',');

	/* If no comma, then report an error in the input specification */
	/*   otherwise add this VRPN field as an input.                 */
	if (device_end != NULL) {
		/* copy just the name of the device into vrpn_device */
		strncpy(vrpn_device, instance, device_end - instance);
		vrpn_device[device_end - instance] = '\0';

		/* the rest of the string should just be a number of the particular sensor */
		sensor_num = vrAtoI(device_end+1);

		/* now see if the given device name is already on */
		/*   the list of specified tracker devices.       */
		vrpn_device_num = vrSearch256StringList(vrpn_device, aux->num_tracker_devices, aux->tracker_names);

		/* if not, then make a VRPN connection to this tracker device and add to the list */
		if (vrpn_device_num < 0) {
			vrpn_device_num = aux->num_tracker_devices;
			vrDbgPrintfN(1 /* TODO: delete this or make INPUT_DBGLVL */, "adding new tracker device to list: number %d\n", vrpn_device_num);

			/* add device name to the list of VRPN tracker devices */
			strcpy(aux->tracker_names[vrpn_device_num], vrpn_device);
			aux->num_tracker_devices++;
			vrDbgPrintfN(1 /* TODO: make INPUT_DBGLVL */, "aux->tracker_names[%d] is now '%s'\n", vrpn_device_num, aux->tracker_names[vrpn_device_num]);

#ifdef DONOTMOVETO_INITDEV
			/* create the userdata for the callback (TODO: or move this to _VrpnInitializeDevice()) */
			userdata = (_VrpnUserdata *)malloc(sizeof(_VrpnUserdata));
			userdata->num = vrpn_device_num;
			userdata->aux = aux;

			/* open the VRPN connection and set the incoming data callback (TODO: or do this in _VrpnInitializeDevice()) */
			aux->vrpn_tracker[vrpn_device_num] = new vrpn_Tracker_Remote(vrpn_device);
			aux->vrpn_tracker[vrpn_device_num]->register_change_handler((void *)userdata, (vrpn_TRACKERCHANGEHANDLER)_VrpnHandleTracker);
#endif
		}

		vrDbgPrintfN(1 /* TODO: make INPUT_DBGLVL */, "doing tracker-sensor device '%s' (tracker %d), %d\n", vrpn_device, vrpn_device_num, sensor_num);

		/* now assign given input to the existing or new tracker device */
		aux->sensor6_inputs[vrpn_device_num][sensor_num] = (vr6sensor *)input;

		/* the vrAssign6sensorR2Exform() function does the parsing of the next token */
		vrAssign6sensorR2Exform((vr6sensor *)input, strchr(device_end+1, ','));

		vrDbgPrintfN(INPUT_DBGLVL, "assigned 6sensor event of value 0x%02x to input pointer = 0x%p)\n",
			sensor_num, aux->sensor6_inputs[vrpn_device_num][sensor_num]);

	} else {
		vrErrPrintf(RED_TEXT "_VrpnTrackerInput: Warning, invalid input specification (%s, %s).\n" NORM_TEXT,
			type, instance);
	}

	return 1;	/* input declaration match */
}


	/************************************************************/
	/***************** FreeVR Callback routines *****************/
	/************************************************************/

	/************************************************************/
	/*      Callbacks for controlling the VRPN features.        */
	/*                                                          */

/************************************************************/
static void _VrpnPrintContextStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrPrintContext(stdout, verbose);
}

/************************************************************/
static void _VrpnPrintConfigStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrPrintConfig(stdout, verbose);
}

/************************************************************/
static void _VrpnPrintInputStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrPrintInput(stdout, verbose);
}

/************************************************************/
static void _VrpnPrintStructCallback(vrInputDevice *devinfo, int value)
{
        _VrpnPrivateInfo  *aux = (_VrpnPrivateInfo *)devinfo->opaque;

        if (value == 0)
                return;

        _VrpnPrintStruct(stdout, aux);
}

/************************************************************/
static void _VrpnPrintHelpCallback(vrInputDevice *devinfo, int value)
{
        _VrpnPrivateInfo  *aux = (_VrpnPrivateInfo *)devinfo->opaque;

        if (value == 0)
                return;

        _VrpnPrintHelp(stdout, aux);
}


/*********************************/
/*** List of control functions ***/
static vrControlFunc _VrpnControlList[] = {
                { "print_context", (void (*)())_VrpnPrintContextStructCallback },
                { "print_config", (void (*)())_VrpnPrintConfigStructCallback },
                { "print_input", (void (*)())_VrpnPrintInputStructCallback },
                { "print_struct", (void (*)())_VrpnPrintStructCallback },
                { "print_help", (void (*)())_VrpnPrintHelpCallback },
	/** TODO: other callback control functions go here **/
                { NULL, NULL } };



	/************************************************************/
	/*      Callbacks for interfacing with the VRPN device.     */
	/*                                                          */


/************************************************************/
static void _VrpnFunction(vrInputDevice *devinfo, int which_operation)
{

	_VrpnPrivateInfo	*aux = NULL;
	vrInputFunction		_VrpnInputs[] = {
					{ "button", VRINPUT_2WAY, _VrpnButtonInput },
					{ "analog", VRINPUT_VALUATOR, _VrpnAnalogInput },
					{ "tracker", VRINPUT_6SENSOR, _VrpnTrackerInput },
					{ NULL, VRINPUT_UNKNOWN, NULL } };

	switch (which_operation) {

#define CREATE	1
#define OPEN	2
#define CLOSE	3
#define RESET	4
#define POLL	5

	case CREATE:

		devinfo->opaque = (void *)vrShmemAlloc0(sizeof(_VrpnPrivateInfo));
		aux = (_VrpnPrivateInfo *)devinfo->opaque;
		_VrpnInitializeStruct(aux);

		/******************/
		/* handle options */
#if 0
		aux->port = vrShmemStrDup(DEFAULT_PORT);	/* default, if no port given */
#endif
		_VrpnParseArgs(aux, devinfo->args);

		/***************************************/
		/* create the inputs and self-controls */
		vrCreateInputs(devinfo, _VrpnInputs);
		vrCreateSelfcontrols(devinfo, _VrpnInputs, _VrpnControlList);
		/* TODO: anything to do for NON self-controls?  Implement for Magellan first. */

		vrDbgPrintf("Done creating VRPN inputs for '%s'\n", devinfo->name);
		devinfo->created = 1;

		break;

	case OPEN:

		aux = (_VrpnPrivateInfo *)devinfo->opaque;

		/*******************/
		/* open the device */
		/* TODO: perhaps this is where the check for VRPN functions should be. */
		if (_VrpnInitializeDevice(aux) < 0) {
			vrErrPrintf("_VrpnFunction-OPEN: "
				RED_TEXT "Warning, no XXX to '%s' VRPN.\n" NORM_TEXT,
				devinfo->name);
		}
		aux->open = 1;

		vrDbgPrintf("Done opening VRPN input device '%s'.\n", devinfo->name);
		devinfo->ready = 1;	/* TODO: s/b = aux->open */

		break;

	case CLOSE:

		aux = (_VrpnPrivateInfo *)devinfo->opaque;

		/* TODO: add VRPN shut down operations */
		if (aux != NULL) {
			vrShmemFree(aux);	/* aka devinfo->opaque */
		}
		break;

	case RESET:
		/* TODO: reset code */
		break;

	case POLL:
		if (devinfo->ready) {
			_VrpnGetData(devinfo);
		} else {
			/* TODO: try to open the device again */
		}
		break;
	}
}



	/******************************/
	/*** FreeVR public routines ***/
	/******************************/


#if defined(__cplusplus) && !defined(DO_EXTERN_C)
extern "C" {
#endif
/******************************************************/
void vrVRPNInitInfo(vrInputDevice *devinfo)
{
	devinfo->version = (char *)vrShmemStrDup("-get from VRPN device-");
	devinfo->Create = vrCreateCallback(_VrpnFunction, 2, devinfo, CREATE);
	devinfo->Open = vrCreateCallback(_VrpnFunction, 2, devinfo, OPEN);
	devinfo->Close = vrCreateCallback(_VrpnFunction, 2, devinfo, CLOSE);
	devinfo->Reset = vrCreateCallback(_VrpnFunction, 2, devinfo, RESET);
	devinfo->PollData = vrCreateCallback(_VrpnFunction, 2, devinfo, POLL);

	vrDbgPrintfN(INPUT_DBGLVL, "vrVrpnInitInfo: callbacks created.\n");
}
#if defined(__cplusplus) && !defined(DO_EXTERN_C)
}
#endif


#endif /* } FREEVR */



	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/



#if defined(TEST_APP) /* { */

/***************************************************************************/
/* A test program to communicate with a VRPN device and print the results. */
main(int argc, char *argv[])
{
	_VrpnPrivateInfo	*aux;
	char			*hostname = "localhost";
	char			*tracker_name = "Tracker0"
	char			tracker_dev[256];


	/******************************/
	/* setup the device structure */
	aux = (_VrpnPrivateInfo *)malloc(sizeof(_VrpnPrivateInfo));
	memset(aux, 0, sizeof(_VrpnPrivateInfo));
	_VrpnInitializeStruct(aux);


	/****************************************************/
	/* adjust parameters based on environment variables */
#if 0
	aux->port = DEFAULT_PORT;			/* default, if no file given */
#endif

	if (getenv("VRPN_HOST") != NULL)
		hostname = getenv("VRPN_HOST");

	if (getenv("VRPN_TRACKER") != NULL)
		tracker_name = getenv("VRPN_TRACKER");


	/*****************************************************/
	/* adjust parameters based on command-line-arguments */
	/* TODO: parse CLAs for non-default socket, server machine, etc. */


	/**************************************************/
	/* open and initialize the device */
	aux->open = 0;

	sprintf(tracker_dev, "%s@%s", tracker_name, hostname);
	printf("Opening device: %s\n", tracker_dev);
	aux->vrpn_tracker = new vrpn_Tracker_Remote(tracker_dev);
	if (aux->vrpn_tracker == NULL) {
		fprintf(stderr, RED_TEXT "couldn't open tracker port %s\n" NORM_TEXT, tracker_dev);
		sprintf(aux->version, "- unconnected VRPN -");
	} else {
		aux->vrpn_tracker->register_change_handler(NULL, _VrpnHandleTracker);
		aux->open = 1;	/* ie. something is open */
	}

	if (_VrpnInitializeDevice(aux) < 0) {
		vrErrPrintf("main: "
			RED_TEXT "Warning, unable to initialize VRPN.\n" NORM_TEXT);
		aux->open = 0;
	}

	_VrpnPrintStruct(stdout, aux);
	fprintf(stdout, "-------------------\n");


	/**********************/
	/* display the output */
	while (1) {
		if (_VrpnReadInput(aux) > 0) {
#ifdef NOT_YET_IMPLEMENTED
			printf("tx:%6d ty:%6d tz:%6d rx:%6d ry:%6d rz:%6d  buttons:0x%03x",
				aux->data[TX], aux->data[TY], aux->data[TZ],
				aux->data[RX], aux->data[RY], aux->data[RZ],
				aux->buttons);
#else
			printf("got some data");
#endif /* NOT_YET_IMPLEMENTED */
			printf("    \r");
			fflush(stdout);
		}
	}


	/*****************/
	/* close up shop */
	if (aux != NULL) {
		/* TODO: close the VRPN connections */
#ifdef NOT_YET_IMPLEMENTED
		vrSerialClose(aux->fd);
		free(aux);			/* aka devinfo->opaque */
#endif /* NOT_YET_IMPLEMENTED */
	}

	printf("\nVRPN device closed\n");
}

#endif /* } TEST_APP */



	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/



#if defined(CAVE) /* { */


	/* ... CAVE stuff here if to also work with CAVElib */


#endif /* } CAVE */

#if defined(__cplusplus) && defined(DO_EXTERN_C)
}
#endif

