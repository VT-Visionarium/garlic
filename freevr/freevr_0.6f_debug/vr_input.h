/* ======================================================================
 *
 * HH   HH         vr_input.h
 * HH   HH         Author(s): Ed Peters, Bill Sherman
 * HHHHHHH         Created: June 4, 1998
 * HH   HH         Last Modified: February 20, 2014
 * HH   HH
 *
 * Header file for FreeVR inputs & input processes.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#ifndef __VRINPUT_H__
#define __VRINPUT_H__

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "vr_callback.h"
#include "vr_procs.h"
#include "vr_math.h"
#include "vr_context.h"		/* so we can point back to the context from the main input structure */

#define	INPUT_UIDESC_LEN	1024

#ifdef __cplusplus
extern "C" {
#endif


/****************************************************/
typedef enum {
		VRINPUT_UNKNOWN = -1,
		VRINPUT_BINARY = 0,
		VRINPUT_2WAY = VRINPUT_BINARY,
		VRINPUT_NWAY,
		VRINPUT_NARY = VRINPUT_NWAY,
		VRINPUT_VALUATOR,
		VRINPUT_KEYSTROKE,
		VRINPUT_TEXT,
		VRINPUT_6SENSOR,
		VRINPUT_NSENSOR,
		VRINPUT_POSITION,
		VRINPUT_POINTER = VRINPUT_POSITION,
		VRINPUT_PLANE,
		VRINPUT_CONTROL
	} vrInputType;


/****************************************************/
/* This is the return type from input device functions that indicate */
/*   whether they can handle an input specified by the config file.  */
typedef enum {
		VRINPUT_NOMATCH = -1,	/* Device does not match the given DTI specification */
		VRINPUT_MATCH_UNABLE,	/* Device matches DTI specification, but can't handle for other reasons */
		VRINPUT_MATCH_ABLE	/* Device matches DTI specification, and can/will handle it */
	} vrInputMatch;


/*******************************************************************************/
/* vrGenericInput:  A structure with the generic fields for all input types    */
/*   (aka "value containers").  By casting other input types to this structure */
/*   the code can reference the "input_type" field to verify the type.         */
/*******************************************************************************/
typedef struct vrGenericInput_st {
		/***************************/
		/* Generic Object field(s) */
		vrObject	object_type;	/* The type of this object (ie VROBJECT_INPUTDATA)*/
		/* NOTE: the rest of the generic object fields not included with individual inputs*/
		/* TODO: consider adding the other object fields to the input structures.         */

		/*****************************/
		/* generic input information */
		vrInputType	input_type;	/* enumerated value of the type of input */
		int		checksum;	/* a checksum of the structure (mostly for debugging */
		struct vrID_st	*my_device;	/* pointer back to the device giving this input */
		struct vrInput_st *my_object;	/* pointer back to the object holding this input */
		vrLock		lock;		/* mutual exclusion lock for the input data */
		int		queue_me;	/* flag whether this input s/b queued */
		vrTime		timestamp;	/* time of last update (type may change)*/
		int		dummy;		/* input is a dummy input */
	} vrGenericInput;


/***********************************************************/
/* vr2switch: A structure with the input information for a */
/*   binary (ie. 2-way) switch.                            */
/***********************************************************/
typedef struct {
		/***************************/
		/* Generic Object field(s) */
		vrObject	object_type;	/* The type of this object (ie VROBJECT_INPUTDATA)*/
		/* NOTE: the rest of the generic object fields not included with individual inputs*/
		/* TODO: consider adding the other object fields to the input structures.         */

		/*****************************/
		/* generic input information */
		vrInputType	input_type;	/* enumerated value of the type of input */
		int		checksum;	/* a checksum of the structure (mostly for debugging */
		struct vrID_st	*my_device;	/* pointer back to the device giving this input */
		struct vrInput_st *my_object;	/* pointer back to the object holding this input */
		vrLock		lock;		/* mutual exclusion lock for the input data */
		int		queue_me;	/* flag whether this input s/b queued */
		vrTime		timestamp;	/* time of last update (type may change)*/
		int		dummy;		/* input is a dummy input */

		/*******************************/
		/* binary specific information */
		int		value;		/* the current value of the input */
		int		last_value;	/* the previous value of the input */
		int		visren_value;	/* the value from a visren frame sync -- TODO: handle multiple sync-groups */

		int		num_measures;	/* the number of measurements to store in the "measures" array (if 0 or less, do no recording for this input) */
		int		current_measure;/* the most recent location in the "measures" array into which a value was placed */
		int		*measures;	/* array of values for last <N> frames */
	} vr2switch;


/***********************************************************/
/* vrNswitch: A structure with the input information for a */
/*   multiway (ie. N-way) switch.                          */
/***********************************************************/
typedef struct {
		/***************************/
		/* Generic Object field(s) */
		vrObject	object_type;	/* The type of this object (ie VROBJECT_INPUTDATA)*/
		/* NOTE: the rest of the generic object fields not included with individual inputs*/
		/* TODO: consider adding the other object fields to the input structures.         */

		/*****************************/
		/* generic input information */
		vrInputType	input_type;	/* enumerated value of the type of input */
		int		checksum;	/* a checksum of the structure (mostly for debugging */
		struct vrID_st	*my_device;	/* pointer back to the device giving this input */
		struct vrInput_st *my_object;	/* pointer back to the object holding this input */
		vrLock		lock;		/* mutual exclusion lock for the input data */
		int		queue_me;	/* flag whether this input s/b queued */
		vrTime		timestamp;	/* time of last update (type may change)*/
		int		dummy;		/* input is a dummy input */

		/*******************************/
		/* switch specific information */
		int		value;		/* the current value of the input */
		int		last_value;	/* the previous value of the input */
		int		visren_value;	/* the value from a visren frame sync -- TODO: handle multiple sync-groups */
	} vrNswitch;


/**********************************************************/
/* vrValuator: A structure with the input information for */
/*   continuous values.                                   */
/**********************************************************/
typedef struct {
		/***************************/
		/* Generic Object field(s) */
		vrObject	object_type;	/* The type of this object (ie VROBJECT_INPUTDATA)*/
		/* NOTE: the rest of the generic object fields not included with individual inputs*/
		/* TODO: consider adding the other object fields to the input structures.         */

		/*****************************/
		/* generic input information */
		vrInputType	input_type;	/* enumerated value of the type of input */
		int		checksum;	/* a checksum of the structure (mostly for debugging */
		struct vrID_st	*my_device;	/* pointer back to the device giving this input */
		struct vrInput_st *my_object;	/* pointer back to the object holding this input */
		vrLock		lock;		/* mutual exclusion lock for the input data */
		int		queue_me;	/* flag whether this input s/b queued */
		vrTime		timestamp;	/* time of last update (type may change)*/
		int		dummy;		/* input is a dummy input */

		/*********************************/
		/* valuator specific information */
		float		value;		/* the current value of the input */
		float		last_value;	/* the previous value of the input */
		float		visren_value;	/* the value from a visren frame sync -- TODO: handle multiple sync-groups */
		int		num_measures;	/* the number of measurements to store in the "measures" array (if 0 or less, do no recording for this input) */
		int		current_measure;/* the most recent location in the "measures" array into which a value was placed */
		float		*measures;	/* array of values for last <N> frames */
	} vrValuator;


/************************************************************/
/* vr6sensor: An 6-degree of freedom position input device. */
/************************************************************/
typedef struct vr6sensor_st {
		/***************************/
		/* Generic Object field(s) */
		vrObject	object_type;	/* The type of this object (ie VROBJECT_INPUTDATA)*/
		/* NOTE: the rest of the generic object fields not included with individual inputs*/
		/* TODO: consider adding the other object fields to the input structures.         */

		/*****************************/
		/* generic input information */
		vrInputType	input_type;	/* enumerated value of the type of input */
		int		checksum;	/* a checksum of the structure (mostly for debugging */
		struct vrID_st	*my_device;	/* pointer back to the device giving this input */
		struct vrInput_st *my_object;	/* pointer back to the object holding this input */
		vrLock		lock;		/* mutual exclusion lock for the input data */
		int		queue_me;	/* flag whether this input s/b queued */
		vrTime		timestamp;	/* time of last update (type may change)*/
		int		dummy;		/* input is a dummy input */

		/*******************************/
		/* sensor specific information */
		int		dof;		/* degrees of freedom of a sensor   */
		int		active;		/* sensor is active? boolean        */
		int		oob;		/* sensor is out-of-bounds? boolean */
		int		error;		/* sensor error (0 = no error)      */
#if 0 /* some other possibilities (ie. CAVElib has these) */
		int		calibrated;
		int		frame_of_reference;	/* real-world, virtual-world, tracker */ /* In FreeVR frame-of-reference is always real-world, and must be transformed to other FORs */
#endif
		vrMatrix	*raw_data;	/* raw sensor data              */
		vrMatrix	*position;	/* sensor data in real-world CS */
		vrMatrix	*last_position;	/* the previous sensor data in real-world CS */
		vrMatrix	*t2rw_xform;	/* transform from tracker CS (eg. transmitter) to real-world */
		vrMatrix	*r2e_xform;	/* transform from receiver to entity (eg. nose) */
		vrMatrix	*w_initxform;	/* initial transform in world coordinates */
		vrMatrix	*visren_position;/* the value from a visren frame sync -- TODO: handle multiple sync-groups */
	} vr6sensor;


/************************************************************/
/* vrNsensor: An n-degree of freedom input device.          */
/************************************************************/
typedef struct {
		/***************************/
		/* Generic Object field(s) */
		vrObject	object_type;	/* The type of this object (ie VROBJECT_INPUTDATA)*/
		/* NOTE: the rest of the generic object fields not included with individual inputs*/
		/* TODO: consider adding the other object fields to the input structures.         */

		/*****************************/
		/* generic input information */
		vrInputType	input_type;	/* enumerated value of the type of input */
		int		checksum;	/* a checksum of the structure (mostly for debugging */
		struct vrID_st	*my_device;	/* pointer back to the device giving this input */
		struct vrInput_st *my_object;	/* pointer back to the object holding this input */
		vrLock		lock;		/* mutual exclusion lock for the input data */
		int		queue_me;	/* flag whether this input s/b queued */
		vrTime		timestamp;	/* time of last update (type may change)*/
		int		dummy;		/* input is a dummy input */

#define MAX_NSENSOR_VALUES 100
		/*******************************/
		/* sensor specific information */
		int		dof;		/* ?? degrees of freedom of a sensor */
		int		active;		/* sensor is active? boolean         */
		int		oob;		/* sensor is out-of-bounds? boolean  */
		int		error;		/* sensor error (0 = no error)       */
		float		values[MAX_NSENSOR_VALUES];	/* sensor data                    */
		float		last_values[MAX_NSENSOR_VALUES];/* previous values of the sensor data */
#if 0 /* some other possibilities (ie. CAVElib has these) */
		int		calibrated;
#endif
		float		visren_values[MAX_NSENSOR_VALUES];/* the values from a visren frame sync -- TODO: handle multiple sync-groups */
	} vrNsensor;


/*********************************************************/
/* vrControl: A structure with the input information for */
/*   input device control callbacks.                     */
/*********************************************************/
typedef struct {
		/***************************/
		/* Generic Object field(s) */
		vrObject	object_type;	/* The type of this object (ie VROBJECT_INPUTDATA)*/
		/* NOTE: the rest of the generic object fields not included with individual inputs*/
		/* TODO: consider adding the other object fields to the input structures.         */

		/*****************************/
		/* generic input information */
		vrInputType	input_type;	/* enumerated value of the type of input */
		int		checksum;	/* a checksum of the structure (mostly for debugging */
		struct vrID_st	*my_device;	/* pointer back to the device giving this input */
		struct vrInput_st *my_object;	/* pointer back to the object holding this input */
		vrLock		lock;		/* mutual exclusion lock for the input data */
		int		queue_me;	/* flag whether this input s/b queued */
		vrTime		timestamp;	/* time of last update (type may change)*/
		int		dummy;		/* input is a dummy input */

		/*******************************/
		/* control specific information */
		int		callback_assigned;/* flag to indicate the device has assigned a callback */
		vrCallback	*callback;	/* function to control an input device */
	} vrControl;


/************************************************************/
/* vrInputDesc: a structure containing information about an */
/*   input description.                                     */
/************************************************************/
typedef struct vrInputDesc_st {
		vrInputType	type;
		char		*args;
	} vrInputDesc;


/***************************************************************/
/* vrInput: A structure containing all the details about       */
/*   a particular input.                                       */
/* Fields marked as "CONFIG" are filled in directly from the   */
/*   config file.  Other fields are determined in combination  */
/*   with other factors.                                       */
/* These are typically (perhaps only) created in the function  */
/*   _ConfigParseInputDescription() in vr_config.c             */
/***************************************************************/
typedef struct vrInput_st {

		/*************************/
		/* Generic Object fields */
		vrObject	object_type;	/* The type of this object (ie. VROBJECT_INPUT) */
		int		id;		/* index into the array of devices */
		char		*name;		/* CONFIG: name assigned to device in config file */
		int		malleable;	/* CONFIG: Whether the data of this device can be modified */
	struct	vrInput_st	*next;		/* next input device in a linked list */ /* TODO: is this right? shouldn't this be a pointer to a generic object? */
		vrContextInfo	*context;	/* pointer to the vrContext memory structure */
		int		line_created;	/* line number of where this object was created in config */
		int		line_lastmod;	/* line number of last time this object modified in config */
		char		file_created[512];/* file (or other) where this object was created*/
		char		file_lastmod[512];/* file (or other) where this object was created*/

		/* Fields specific to Inputs */

		int		map_refcount;	/* CONFIG: number of times this input is referenced in inputmap */
		char		*desc_str;	/* CONFIG: string description of the input */
		vrInputDesc	*desc;		/* &CONFIG: parsed description of the input */
		vrMatrix	*r2e_xform;	/* CONFIG: r2e transform at the time this object was created */

		char		desc_ui[INPUT_UIDESC_LEN];	/* A description about how this input is used as part of the user-interface */

		union {
			vrGenericInput	*generic;	/* generic information common to all types*/
			vr2switch	*switch2;	/* switch2 specific info (plus generic)   */
			vrNswitch	*switchN;	/* switchN specific info (plus generic)   */
			vrValuator	*valuator;	/* valuator specific info (plus generic)  */
			vr6sensor	*sensor6;	/* sensor6 specific info (plus generic)   */
			vrNsensor	*sensorN;	/* sensorN specific info (plus generic)   */
			vrControl	*control;	/* control specific info (plus generic)   */
		} container;			/* a pointer to the input value data container */

	} vrInput;


/***************************************************************/
/* vrInputDevice: A structure containing all the details about */
/*   a particular input device.                                */
/* Fields marked as "CONFIG" are filled in direction from the  */
/*   config file.  Other fields are determined in combination  */
/*   with other factors.                                       */
/***************************************************************/
typedef struct vrID_st {

		/*************************/
		/* Generic Object fields */
		vrObject	object_type;	/* The type of this object (ie. VROBJECT_INDEV) */
		int		id;		/* index into the array of devices */
		char		*name;		/* CONFIG: name assigned to device in config file */
		int		malleable;	/* CONFIG: Whether the data of this device can be modified */
	struct	vrID_st		*next;		/* next input device in a linked list */
		vrContextInfo	*context;	/* pointer to the vrContext memory structure */
		int		line_created;	/* line number of where this object was created in config */
		int		line_lastmod;	/* line number of last time this object modified in config */
		char		file_created[512];/* file (or other) where this object was created*/
		char		file_lastmod[512];/* file (or other) where this object was created*/

		/* Fields specific to Input Devices */

		/**************************************/
		/* General information about a device */
		char		*version;	/* version string from the device */
		vrProcessInfo	*proc;		/* pointer to the process handling this device */
		void		*aux_data;	/* auxiliary data, specific to the type of input */
		char		*type;		/* CONFIG: type of device -- used to choose functions */
		char		*dso_file;	/* CONFIG: filename for retrieving shared object code */
		char		*dso_func;	/* CONFIG: function name for DSO init-info routine */
		void		*dso_handle;	/* a handle for referencing the DSO object */
		char		*args;		/* CONFIG: config arguments for this device */
		vrInput		*inputs;	/* CONFIG: linked list of inputs to provide for this device */
		vrInput		*self_controls;	/* CONFIG: linked list of controls internal to this device */
		int		created;	/* whether the device's inputs have been created */
		int		operating;	/* whether the device is operating */
		vrMatrix	*t2rw_xform;	/* CONFIG: transform from tracker device CS to real-world CS */
		vrMatrix	*r2e_xform;	/* CONFIG: transform from tracker receiver to entity CS */

		/**************/
		/* The inputs */
		int		counted;	/* flag that indicates that the inputs types have been counted */

		int		num_6sensors;	/* number of position sensors provided by this device */
		vr6sensor	*sensor6;	/* data for position sensors */

		int		num_Nsensors;	/* number of general sensors provided by this device */
		vrNsensor	*sensorN;	/* data for general sensors */

		int		num_2ways;	/* number of 2way switches (aka buttons) */
		vr2switch	*switch2;	/* data for 2way switches */

		int		num_Nways;	/* number of Nway switches (aka switches) */
		vrNswitch	*switchN;	/* data for Nway switches */

		int		num_valuators;	/* number of valuators */
		vrValuator	*valuator;	/* data for valuators */

		int		num_texts;	/* number of text */
		void		*text;		/* data for text */

		int		num_controls;	/* number of input device controls */
		vrControl	*control;	/* data for controls */

		int		num_scontrols;	/* number of input device self-controls */

		/*****************/
		/* The callbacks */
		vrCallback	*Create;	/* function to create the inputs for the device */
		vrCallback	*Open;		/* function to initialize the device */
		vrCallback	*PollData;	/* function to Poll for device data */
		vrCallback	*Close;		/* function to shutdown the device */
		vrCallback	*Reset;		/* function to reset device unit */
		vrCallback	*PrintAux;	/* function to print the auxiliary data */

	} vrInputDevice;


/*******************************************************************/
/* vrInputOptsType: structure for adding new input device options. */
/*******************************************************************/
typedef struct {
		char	*option_name;
		void	(*info_func)(vrInputDevice *);
	} vrInputOptsType;


/********************************************************************/
/* vrInputDTI: a structure containing information about an          */
/*   input device,type,instance triplet.                            */
/*   These match the form in a configuration file:                  */
/*     {<sub-device>:}<type>[<arguments>]                           */
/*   So for example:                                                */
/*     input "6sensor[head]" = "6sensor(Tracker0:tracker[0, r2e])"; */
/*     - "vrpn" is the device -- as specified elsewhere             */
/*     - "Tracker0" is the sub-device                               */
/*     - "tracker" is the type                                      */
/*     - "0" is the tracker instance                                */
/*     - "r2e" is an argument applied to that instance              */
/********************************************************************/
typedef struct {
		char	*device;	/* really the "sub-device" of an overarching type -- eg. "Joystick0" sub-device of a VRPN device */
		char	*type;		/* this part is used to match against the vrInputFunction.type field */
		char	*instance;	/* a specific key/button/valuator -- NOTE: may also be followed by other arguments after a comma */
	} vrInputDTI;


/*************************************************************/
/* vrInputFunction: a structure containing information about */
/*   a function used to parse "input" configuration lines.   */
/*************************************************************/
typedef struct {
		char		*name;		/* NOTE: this is used to match against a vrInputDTI.type field! */
		vrInputType	type;
		vrInputMatch	(*func)(vrInputDevice *, vrGenericInput *, vrInputDTI *);
	} vrInputFunction;


/************************************************************/
/* vrControlFunc: a structure containing information about  */
/*   a function used to control an input device, based on a */
/*   name associated with the control.                      */
/************************************************************/
typedef struct {
		char		*name;
		void		(*func)();
	} vrControlFunc;


/***************************************************************************/
/* vr6sensorConv: structure for specifying Valuator to 6sensor Conversion. */
/* TODO: [6/3/03] consider a new field containing a ratio of translational scaling to rotational scaling */
/***************************************************************************/
typedef struct {
		int		azimuth_axis;	/* the axis about which azimuth is defined */
		int		ignore_all;	/* don't do the conversion */
		int		ignore_trans;	/* only do the rotation stuff */
		int		tmp_ignore_trans;/* only do the rotation stuff (on a temp basis) */
		int		relative_axis;	/* base movements on real world CS or sensor CS */
		int		return_to_zero;	/* base movements on previous sensor value, or not*/
		int		restrict_space;	/* restrict sensor location to a given volume */
		int		swap_yz;	/* swap movements between Y(up) & Z(in) axes */
		int		swap_transrot;	/* whether to swap translation and rotation values*/
		float		trans_scale;	/* translational scaling factor */
		float		rot_scale;	/* rotational scaling factor */
		float		working_volume_min[3];	/* minimum X,Y,Z values of working volume */
		float		working_volume_max[3];	/* maximum X,Y,Z values of working volume */
	} vr6sensorConv;


/******************************************************************/
/* vrInput: Overall input structure used by the app-dev to access */
/*   input data.                                                  */
/******************************************************************/
typedef struct vrInputInfo_st {
		/***************************/
		/* Generic Object field(s) */
		vrObject	object_type;	/* The type of this object (ie VROBJECT_INPUTINFO)*/
		/* NOTE: the rest of the generic object fields not included with individual inputs*/
		/* TODO: consider adding the other object fields to the input structures.         */

		/***************************/
		vrContextInfo	*context;	/* pointer to the vrContext memory structure */
#if 0
		/* TODO: create (and use) a queuing structure for inputs */
		vrQueue		*queue;
    or		vrInputQueue	*queue;
#endif
		char		*input_map_name;

		/******************************************/
		/* LAYER 1: pointers to the input devices */
		int		all_devices_initialized;
		int		num_input_devices;
		vrInputDevice	**input_devices;


		/*************************************/
		/* LAYER 2: pointers to the entities */
		/*          (not necessarily used)   */
		int		num_users;
	struct	vrUserInfo_st	**users;

		int		num_props;
	struct	vrPropInfo_st	**props;


		/*******************************************/
		/* LAYER 3: mappings to the logical inputs */
		/*          (ie. the useful layer)         */
		/*    (the instantiation of the input map) */
		int		num_2ways;
		vr2switch	**switch2;

		int		num_Nways;
		vrNswitch	**switchN;

		int		num_valuators;
		vrValuator	**valuator;

		int		num_6sensors;
		vr6sensor	**sensor6;

		int		num_Nsensors;
		vrNsensor	**sensorN;

		int		num_controls;
		vrControl	**control;

		/* TODO: ... text, positions/pointers, planes, {keys|keyboard} */

	} vrInputInfo;


/********************************************************/
/*** Function declarations for internal-use functions ***/

void		 vrCreateInputMap(vrInputInfo *input, char *mapname);
vrGenericInput	*vrInputAddDummyToInputMap(vrInputInfo *inputs, vrInputType type, int input_num);
void		 vrInputInitialize(vrContextInfo *context);
int		 vrInputsInitialized(vrContextInfo *context);
void		*vrInputCreateDataContainerArrayOfType(vrInputType type, int num, vrInputDevice *indev);
void		 vrInputCountDataContainers(vrInputDevice *devinfo);
void		 vrInputCreateDataContainers(vrInputDevice *devinfo, vrInputFunction *input_functions);
void		*vrGetFuncFromControlList(char *funcname, vrControlFunc *funclist); /* TODO: move elsewhere? */
int		 vrInputDeviceInList(char *device);
void		 vrInputCreateSelfControlContainers(vrInputDevice *devinfo, vrInputFunction *input_functions, vrControlFunc *funclist);
void		 vrFprintInputValue(FILE *file, vrGenericInput *input, vrPrintStyle style);
void		 vrInputInitProc(vrProcessInfo *);
void		 vrInputTermProc(vrProcessInfo *);
void		 vrInputOneFrame(vrProcessInfo *);
void		 vrInputMainLoop(vrProcessInfo *);
int		 vrInputCheckIfAllInputDevicesAreOpen(vrContextInfo *context);
void		 vrInputWaitForAllInputDevicesToBeOpen();
void		 vrInputWaitForAllInputsToBeCreated(vrContextInfo *context);
void		 vrGetDefaultInputDeviceInfo(vrInputDevice *info);
void		 vrGetInputDeviceInfo(vrInputDevice *);

void		 vrAssignGenericInput(vrGenericInput *input, char *value);
void		 vrAssign2switchValue(vr2switch *switch2, int newvalue /* , vrTime time */);
void		 vrAssignNswitchValue(vrNswitch *switchN, int newvalue /* , vrTime time */);
void		 vrAssignValuatorValue(vrValuator *valuator, float newvalue /* , vrTime time */);
void		 vrAssign6sensorValue(vr6sensor *sensor6, vrMatrix *new_mat, int oob /* , vrTime time */);
void		 vrAssign6sensorValueFromValuators(vr6sensor *sensor6, float *valuators, vr6sensorConv *conv_options, int oob /* , vrTime time */);
void		 vrAssign6sensorR2Exform(vr6sensor *sensor6, char *xform_info);
void		 vrAssign6sensorActiveValue(vr6sensor *sensor6, int active);
void		 vrAssign6sensorOobValue(vr6sensor *sensor6, int oob);
void		 vrAssign6sensorErrorValue(vr6sensor *sensor6, int error);
void		 vrAssignNsensorArray(vrNsensor *sensorN, float *new_data /* , vrTime time */);

/***********************************************************/
/*** Function declarations for application-use functions ***/

int		 vrInputSet2switchDescription(int input_num, char *input_desc);
int		 vrGet2switchValue(int input_num);
int		 vrGet2switchDelta(int input_num);
int		 vrGet2switchValueDirect(vr2switch *input);
int		 vrGet2switchDeltaDirect(vr2switch *input);
int		 vrGet2switchValueNoLastUpdate(int input_num);

int		 vrInputSetNswitchDescription(int input_num, char *input_desc);
int		 vrGetNswitchValue(int input_num);
int		 vrGetNswitchDelta(int input_num);
int		 vrGetNswitchValueNoLastUpdate(int input_num);

int		 vrInputSetValuatorDescription(int input_num, char *input_desc);
float		 vrGetValuatorValue(int input_num);
float		 vrGetValuatorDelta(int input_num);
float		 vrGetValuatorValueNoLastUpdate(int input_num);

int		 vrInputSet6sensorDescription(int input_num, char *input_desc);
int		 vrGet6sensorActiveValue(int input_num);
int		 vrGet6sensorOobValue(int input_num);
int		 vrGet6sensorErrorValue(int input_num);
int		 vrGet6sensorDummyValue(int input_num);
vr6sensor	*vrGet6sensor(int input_num);
vrMatrix	*vrMatrixGet6sensorValues(vrMatrix *mat, int input_num);
vrMatrix	*vrMatrixGet6sensorValuesNoLastUpdate(vrMatrix *mat, int input_num);
vrMatrix	*vrMatrixGet6sensorValuesDirect(vrMatrix *mat, vr6sensor *input);
vrMatrix	*vrMatrixGet6sensorValuesDirectNoLastUpdate(vrMatrix *mat, vr6sensor *input);
vrMatrix	*vrMatrixGet6sensorRawValuesDirect(vrMatrix *mat, vr6sensor *input);
float		 vrGet6sensorValueF(int input_num, int datum_num);
float		 vrGet6sensorValueDirectF(vr6sensor *input, int datum_num);
vrVector	*vrVectorGetRWFrom6sensorDir(vrVector *vector, int sensor_num, vrDirection direction);
vrVectorf	*vrVectorfGetRWFrom6sensorDir(vrVectorf *vector, int sensor_num, vrDirection direction);
vrVector	*vrVectorGetVWFromUser6sensorDir(vrVector *vector, int user_num, int sensor_num, vrDirection direction);
vrVectorf	*vrVectorfGetVWFromUser6sensorDir(vrVectorf *vector, int user_num, int sensor_num, vrDirection direction);
vrPoint		*vrPointGetRWFrom6sensor(vrPoint *real_point, int sensor_num);
vrPointf	*vrPointfGetRWFrom6sensor(vrPointf *real_point, int sensor_num);
vrPoint		*vrPointGetVWFromUser6sensor(vrPoint *virtual_point, int usernum, int sensor_num);
vrPointf	*vrPointfGetVWFromUser6sensor(vrPointf *virtual_point, int usernum, int sensor_num);
vrEuler		*vrEulerGetRWFrom6sensor(vrEuler *euler, int sensor_num);
vrEuler		*vrEulerGetVWFromUser6sensor(vrEuler *euler, int usernum, int sensor_num);

int		 vrInputSetNsensorDescription(int input_num, char *input_desc);
float		 vrGetNsensorValue(int input_num, int datum_num);
float		*vrGetNsensorArray(int input_num, float *array);
float		 vrGetNsensorValueDirect(vrNsensor *input, int datum_num);
float		*vrGetNsensorArrayDirect(vrNsensor *input, float *array);
float		 vrGetNsensorValueNoLastUpdate(int input_num, int datum_num);

#ifdef GFX_PERFORMER /* { */
/***********************************************************************************/
/* Utility functions that translate FreeVR coordinates in to Performer Z-up coords */
/***********************************************************************************/
vrPoint		*vrPfPointGetRWFrom6sensor(vrPoint *virtual_point, int sensor_num);
vrPoint		*vrPfPointGetVWFromUser6sensor(vrPoint *virtual_point, int usernum, int sensor_num);
vrVector	*vrPfVectorGetRWFrom6sensorDir(vrVector *vector, int sensor_num, vrDirection direction);
vrVector	*vrPfVectorGetVWFromUser6sensorDir(vrVector *vector, int usernum, int sensor_num, vrDirection direction);
vrMatrix	*vrPfMatrixGet6sensorValues(vrMatrix *mat, int input_num);
vrMatrix	*vrPfMatrixGet6sensorValuesDirect(vrMatrix *mat, vr6sensor *input);
vrMatrix	*vrPfMatrixGet6sensorValuesDirectNoLastUpdate(vrMatrix *mat, vr6sensor *input);
vrMatrix	*vrPfMatrixGet6sensorRawValuesDirect(vrMatrix *mat, vr6sensor *input);
vrVectorf	*vrPfVectorfGetRWFrom6sensorDir(vrVectorf *vector, int sensor_num, vrDirection direction);
vrVectorf	*vrPfVectorfGetVWFromUser6sensorDir(vrVectorf *vector, int user_num, int sensor_num, vrDirection direction);
vrPointf	*vrPfPointfGetRWFrom6sensor(vrPointf *real_point, int sensor_num);
vrPointf	*vrPfPointfGetVWFromUser6sensor(vrPointf *virtual_point, int usernum, int sensor_num);
vrEuler		*vrPfEulerGetRWFrom6sensor(vrEuler *euler, int sensor_num);
vrEuler		*vrPfEulerGetVWFromUser6sensor(vrEuler *euler, int usernum, int sensor_num);
#endif /* GFX_PERFORMER } */

int		 vrInputMakeChecksum(vrGenericInput *input);
int		 vrInputCheckChecksum(vrGenericInput *input);

void		 vrFprintInput(FILE *file, vrInputInfo *input, vrPrintStyle style);
char		*vrSprintInputUI(char *str, vrInputInfo *vrInputs, int str_size, vrPrintStyle style);

void		 vrInputObjectClear(vrInput *object);
void		 vrInputObjectCopy(vrInput *dest_object, vrInput *src_object);
char		*vrInputTypeName(vrInputType type);
vrGenericInput	*vrInputGetFromMapname(vrContextInfo *context, char *mapname);
vrGenericInput	*vrInputGetFromTypeIndex(vrContextInfo *context, vrInputType type, int input_num);
void		 vrFprintInputObject(FILE *file, vrInput *inobj, vrPrintStyle style);
void		 vrFprintInputValue(FILE *file, vrGenericInput *input, vrPrintStyle style);

void		 vrInputDeviceClear(vrInputDevice *object);
void		 vrInputDeviceCopy(vrContextInfo *context, vrInputDevice *dest_object, vrInputDevice *src_object);
void		 vrFprintInputDevice(FILE *file, vrInputDevice *device, vrPrintStyle style);


#ifdef __cplusplus
}
#endif

#endif

