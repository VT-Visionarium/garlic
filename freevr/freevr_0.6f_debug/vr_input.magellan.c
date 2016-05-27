/* ======================================================================
 *
 *  CCCCC          vr_input.magellan.c
 * CC   CC         Author(s): Bill Sherman
 * CC              Created: June 20, 1998 -- adapted from my cave.magellan.c code
 * CC   CC         Last Modified: June 7, 2003
 *  CCCCC
 *
 * Code file for FreeVR tracking from Magellan[tm] 6-DOF input device.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/*************************************************************************

FreeVR USAGE:

	Inputs that can be specified with the "input" option:
		input "<name>" = "2switch(button[{1|2|3|4|5|6|7|8|Star}])";
		input "<name>" = "valuator(6dof[{-,}{tx|ty|tz|rx|ry|rz}])";
		input "<name>" = "6sensor(6dof[<sensor number>])";

	Controls are specified with the "control" option:
		control "<control option>" = "2switch(button[{1|2|3|4|5|6|7|8|Star}])";

	Available control options are:
		"print_help" -- print info on how to use the input device
		"system_pause_toggle" -- toggle the system pause flag
		"print_context" -- print the overall FreeVR context data structure (for debugging)
		"print_config" -- print the overall FreeVR config data structure (for debugging)
		"print_input" -- print the overall FreeVR input data structure (for debugging)
		"print_struct" -- print the internal Magellan data structure (for debugging)

		"sensor_next" -- jump to the next 6-sensor on the list
		"setsensor(<num>)" -- set the simulated sensor to a particular one
		"sensor_reset" -- reset the current 6-sensor
		"sensor_resetall" -- reset the all the 6-sensors
		"toggle_valuator" -- toggle valuator values instead of 6-sensor
		"temp_valuator" -- temporarily disable 6-sensor for valuator values
		"toggle_valuator_only" -- toggle whether translation is saved for valuator
		"temp_valuator_only" -- temporarily use translation values for valuator
		"toggle_relative" -- toggle whether movement is relative to sensor's position
		"toggle_space_limit" -- toggle whether 6-sensor can go outside working volume
		"toggle_return_to_zero" -- toggle whether return-to-zero operation is on
		"toggle_use_null_region" -- toggle whether null-region is used

		"beep" -- cause the Magellan to beep


	Here are the FreeVR configuration options for the Magellan:
		"port" - serial port Magellan is connected to
			("/dev/input/magellan" is the default)
		"baud" - baud rate of serial port connection (generally 9600)
			(9600 is the default)
		"transNull" - int value of the translational null region
		"rotNull" - int value of the rotational null region
		"useNull" - boolean choice of whether Null regions should be in effect
		"transScale" - float value of the translational sensitivity scale
		"rotScale" - float value of the rotational sensitivity scale
	 	"valScale" - float value of the valuator sensitivity scale
		"silent" - boolean choice of whether magellan gives aural
			feedback to button presses (2switch's give short beep,
			control operations give longer beep).

	  -- These options are for using the Magellan as a 6sensor
		"restrict" - boolean choice of whether sensor can leave
			working volume (no restriction is the default)
	 	"valuatoroverride" - whether the Magellan should act as a set
			of valuators and buttons rather than a 6sensor.
	 	"returnToZero" - whether 6sensors return to zero when Magellan
			is released
	 	"relativeRot" - boolean choice of whether rotations should be
			about the 6sensor's coordsys, or the world's coordsys.
		"workingVolume" - 6 numbers that describe a parallelepiped in
			which 6sensors can roam.

#if defined(CAVE)
CAVE USAGE:
	Some possible options (modes):
	   Choice of which sensor is active (default to first wand (1)
	   Movement restricted to remain inside the CAVE (or not)
	   Rotations relative to the sensor's axes, or the viewer's

	Current button effects:
	   1 - controller: input button (2way switch) / CAVE button 1
	   2 - controller: input button (2way switch) / CAVE button 2
	   3 - controller: input button (2way switch) / CAVE button 3
	   4 - toggle space restriction (FreeVR and CAVE)
	   5 - control sensor 1 (eg. head)
	   6 - control sensor 2 (eg. wand1)
	   5+6 - reset sensors (eg. CAVE: cave center, facing forward)
	   7 - toggle between abs/rel rotations
	   8 - controller: toggle valuator mode (toggle translation motions)

	Calling CAVEResetTracker will reinitialize the sensors to their
		original locations.
#endif

HISTORY:
	28 August 1997 (Bill Sherman) -- wrote the first version of
		cave.magellan.c based on some code found on the net.

	10 October 1998 (Stuart Levy) -- Modifications made to make it
		work in non-blocking mode.

	5 January 1999 (Bill Sherman) -- did some touch up work, including
		the inclusion of the TEST_APP code for testing the Magellan
		independent of the CAVE library -- this code came from the
		FreeVR library.

	5 January 1999 (Stuart Levy) -- figured out all the read/write timing
		stuff (basically the device operates in half-duplex), plus
		converted the serial port stuff to be POSIX compliant -- with
		Bill Sherman sitting around making comments.

	5 January 1999 (Bill Sherman) -- More cleanup and debugging.

	x February 1999 (Bill Sherman) -- integrated into version 2.6g+ version
		of cave library

	6 October 1999 (Bill Sherman) -- brought all the changes made to the
		CAVE version of the Magellan code last February into the FreeVR
		code (which ideally can be used to compile with either library,
		though that needs to be tested).

	1 December 1999 (Bill Sherman) -- Converted to new vr_serial.c serial
		interface code.  Did the big input style changeover for FreeVR,
		to make the method of specifying inputs much more flexible.

	29 December 1999 (Bill Sherman) -- brought the Magellan device code
		up to date with the new format of input device source
		files, including the new CREATE section of _MagellanFunction().

	3 January 2000 (Bill Sherman) -- Implemented the vrInputFunction method
		of parsing "input" configuration lines.  And added the
		"print_struct" control callback to help do some debugging.

	12 January 2000 (Bill Sherman) -- Reincorporated into CAVE library (it
		turns out I had made several changes to the CAVE portion of the
		CAVE library version [July 10, 1999] that weren't part of the
		combined version).  Note that this now requires vr_serial.c,
		vr_serial.h & vr_debug.h to be compiled with the rest of the
		CAVE library.

	24 January 2000 (Bill Sherman) -- Integrated new self-control creation
		method.  (and cleaned out some of the old, unused code.)

	8 February 2001 (Bill Sherman) -- Added the long anticipated
		"workingvolume" argument.

	3 May 2001 (Bill Sherman)
		I made a few minor changes to catch up to the general
		  vr_input.skeleton.c format.

	21 June 2001 (Bill Sherman)
		I set the oob flags of simulated 6-sensors to be in-bounds for
		  the sensor that is the "active_sim6sensor" (nee "active_sensor"),
		  and out-of-bounds for the others.

	25 June 2001 (Bill Sherman)
		I added the "setsensor(<num>)" control option and associated
		  functions (by copying similar code from vr_input.xwindows.c).

	20 April 2002 (Bill Sherman)
		I changed the usage of the "oob" flag for indicating the active
		  sensor to use the new "active" flag instead.

	11 September 2002 (Bill Sherman)
		Moved the control callback array into the _MagellanFunction()
		  callback.

	21-23 April 2003 (Bill Sherman)
		Updated to new input matching function code style.  Changed
		  "opaque" field to "aux_data".  Split _MagellanFunction()
		  into 5 functions.  Added new vrPrintStyle argument to
		  _MagellanPrintStruct() for the sake of the new "PrintAux"
		  callback.

	3 June 2003 (Bill Sherman)
		Now include "vr_enums.h" for the TEST_APP code.
		Added the address of the auxiliary data to the printout.
		Added the "system_pause_toggle" control callback.
		Now use the "trans_scale" and "trans_rot" fields of
			vr6SensorConv instead of local copies.

	16 October 2009 (Bill Sherman)
		A quick fix to the _MagellanParseArgs() routine to handle the
		no-arguments case.

	2 September 2013 (Bill Sherman) -- adopting a consistent usage
		of str<...>cmp() functions (using logical negation).

	14 September 2013 (Bill Sherman)
		I am making use of the newly created SELFCTRL_DBGLVL for all
		the self-control outputs.  I then added some reasonable output
		to the "print_help" callback to print out what all the device's
		inputs are doing.

	15 September 2013 (Bill Sherman)
		Renaming "active_sensor" to "active_sim6sensor".
		Changed the "0x%p" format to the improved "%#p" format.

TODO:
	- Determine and Return an error value when unable to initialize in
		_MagellanInitializeDevice().  (and use it in _MagellanOpenFunction())

	- Bring in some of the improved comments, etc. from VruiDD (and the new skeleton) [10/19/2009]

	- figure out why the Magellan doesn't work on Solaris for John Stone

	- an option to have the wand (or any sensor) follow along with the head
		(or any other sensor) when it moves might be good

		Here's a thought:
			handle the (internal notion of) wand movement as always
			being relative to the head.  After every movement,
			send the addition of the internal wand position to
			the head position as the external wand position -- this
			may clean up some of the other problems with Eulers.

	- add rotations about the head/wand sensors (ie. relative)
		incorporate into the controller code (ie. buttons & joystick)

	- provide an option to use blocking reads rather than non-blocking -- this
		may be useful to avoid wasted system time polling for inputs.


**************************************************************************/
#if !defined(FREEVR) && !defined(TEST_APP) && !defined(CAVE)
/* Choose how this code should be compiled (ie. for CAVE, standalone, etc). */
#  define	FREEVR
#  undef	TEST_APP
#  undef	CAVE
#endif

#include <stdio.h>
#include <stdlib.h>	/* used by getenv() */
#include <string.h>
#include <signal.h>

#include "vr_debug.h"
#include "vr_serial.h"

#if defined(FREEVR)
#  include "vr_input.h"
#  include "vr_input.opts.h"
#  include "vr_parse.h"
#  include "vr_shmem.h"
#endif


#if defined(TEST_APP) || defined(CAVE)
#  define	VR_X	0
#  define	VR_Y	1
#  define	VR_Z	2
#  define	VR_AZIM	0
#  define	VR_ELEV	1
#  define	VR_ROLL	2
#endif

#if defined(TEST_APP)
#  include "vr_serial.c"
#  include "vr_enums.h"
#endif


#if defined(CAVE)
#  include <sys/time.h>
#  include <math.h>
#  include "cave.h"
#  include "cave.private.h"
#  include "cave.tracker.h"
#  include "cave.6dof.h"
static CAVE_SENSOR_ST initial_sensor = {
		0,5,0,
		0,0,0,
		0,	/*timestamp*/
		FALSE,	/*calibrated*/
		CAVE_TRACKER_FRAME
	};

#endif /* CAVE */


/*** local defines ***/

#define	VR_TX VR_X
#define	VR_TY VR_Y
#define	VR_TZ VR_Z
#define	VR_RX (VR_ELEV+3)
#define	VR_RY (VR_AZIM+3)
#define	VR_RZ (VR_ROLL+3)

#undef	MAG_DEBUG


	/*****************************************************/
	/*** definitions for interfacing with the Magellan ***/
	/***                                               ***/

/*** protocol information ***/
	/* Magellan star (*) commands:                              */
	/*  "*1" 0010 - 2 report translation                        */
	/*  "*2" 0001 - 1 report rotation                           */
	/*  "*3" 0100 - 4 report only one value (most offset)       */
	/*  "*4" - set the zero position                            */
	/*  "*5" - step through trans gain value (0-7)              */
	/*  "*6" - step through rotate gain value (0-7)             */
	/*  "*7" - ??                                               */
	/*  "*8" - reset the magellan (performs a q(0,0) and n(13)) */

	/* Magellan commands & responses:                               */
	/*  'v'(nnn):- get the device version                           */
	/*  'z'(0):  - set the zero position                            */
	/*  'd'(24): - decode the dial/puck data                        */
	/*  'k'(3):  - decode the keyboard data                         */
	/*  'b'(1/0):- send a beep (1 value for send, 0 for response)   */
	/*            I think values 0-7 are for short to long pauses   */
	/*              and 8-15 are for short to long beeps.           */
	/*  'm'(1):  - set the puck mode:  (range 0-7, default=0)       */
	/*                 0000 - 0 report nothing                      */
	/*            "*2" 0001 - 1 report rotation                     */
	/*            "*1" 0010 - 2 report translation                  */
	/*            "*3" 0100 - 4 report only one value (most offset) */
	/*  'n'(1):  - set the null radius  (range 0-15, default=13)    */
	/*  'p'(2):  -  ?? set poll/x mode  - 2 values?                 */
	/*  'q'(2):  -  set the trans & rotate qualities                */
	/*            effectively scales the puck movements (0=linear?) */
	/*                                                              */
	/* Instead of setting the value, many of the commands will      */
	/*   take a 'Q' as part of the string, and return the current   */
	/*   setting.                                                   */

/*** Magellan command sequences ***/
#define	StartupMsg	"\rz\r"
#define	ModeOffMsg	"m0\r"		/* Translation and Rotation Mode OFF */
#define	ModeMsg		"m3\r"		/* Translation and Rotation Mode ON */
#define	DataRateMsg	"p00\r"		/* 100 msec Date Rate */
#define	QualityMsg	"q00\r"		/* Linear */
#define	NullRadiusMsg	"n?\r"		/* Null Radius at 5 */
#define	ShortBeepMsg	"b9\r"		/* Short Beep */
#define	LongBeepMsg	"bK\r"		/* Long Beep */
#define	ZeroMsg		"z\r"		/* Zeroing */
#define	VersionMsg	"vQ\r"		/* Query for the Magellan version string */
#define	KeyboadQuery	"kQ\r"		/* Poll keyboard */
#define	DataQuestion	"dQ\r"		/* Data Request (Polling mode) -- this doesn't seem to do much */

/* Magellan Button bit-masks & indices */
#define MAG_BUTTON_1		0x001
#define MAG_BUTTON_2		0x002
#define MAG_BUTTON_3		0x004
#define MAG_BUTTON_4		0x008
#define MAG_BUTTON_5		0x010
#define MAG_BUTTON_6		0x020
#define MAG_BUTTON_7		0x040
#define MAG_BUTTON_8		0x080
#define MAG_BUTTON_STAR		0x100

#define MAG_BUTTONINDEX_1	0x00
#define MAG_BUTTONINDEX_2	0x01
#define MAG_BUTTONINDEX_3	0x02
#define MAG_BUTTONINDEX_4	0x03
#define MAG_BUTTONINDEX_5	0x04
#define MAG_BUTTONINDEX_6	0x05
#define MAG_BUTTONINDEX_7	0x06
#define MAG_BUTTONINDEX_8	0x07
#define MAG_BUTTONINDEX_STAR	0x08

/* Magellan sensitivity values */
#define TRANS_SENSITIVITY	0.001
#define ROT_SENSITIVITY		0.02
#define VALUATOR_SENSITIVITY	0.002 /* ?? */


/******************************************************************/
/*** auxiliary structure of the current data from the Magellan. ***/
typedef struct {
		/* these are for magellan interfacing */
		int		fd;		/* file descriptor */
		char		*port;		/* name of serial port */
		int		baud_enum;	/* communication rate as an enumerated value */
		int		baud_int;	/* communication rate as the real value */
		int		open;		/* flag when magellan successfully open */

		/* these are for magellan data parsing */
		char		version[1025];	/* string containing Magellan version  */
		char		lo_buf[512];	/* buffer of left-over bytes           */
		int		lo_buflen;	/* length of lo_buf                    */
		long		bad_Dpackets;	/* count the number of bad 'D' packets */
		long		bad_Kpackets;	/* count the number of bad 'K' packets */
		long		bad_other_packets;/* count the number of other bad packets */

		/* Information about the internal state of the device */
		int             offset[6];	/* incoming translation & rotation offset info */
	unsigned int		mode;		/* the mode of the device itself */
	unsigned int		buttons;	/* the keyboard values from the device */
	unsigned int		beep;
	unsigned int		null_radius;	/* the null radius value in the device itself */
	unsigned int		trans_gain;	/* the translation gain value in the device itself */
	unsigned int		rotate_gain;	/* the rotation gain value in the device itself */
	unsigned int		datarate1;	/* the (MSB?) of the devices datarate */
	unsigned int		datarate2;	/* the (LSB?) of the devices datarate */

#ifdef CAVE
		CAVE_SENSOR_ST	head, wand1, wand2;	/* storage for multiple CAVE sensors */
		CAVE_CONTROLLER_ST controls;		/* storage for CAVE controller info */

		CAVE_SENSOR_ST	*active_sim6sensor;	/* points to {head, wand1, wand2} */

		/* FreeVR holds these values in a separate vr6sensorConv struct */
		int		restrict_space;	/* restrict movement to some device-specific region? */
		int		relative_axis;	/* move relative to the current orientation? */
		int		valuator_only;	/* no 6sensor inputs are active at all? */
		int		valuator_override; /* control valuators rather than sensors? */
		int		return_to_zero;	/* return the sensor to zero when the inputs do */
#elif defined(FREEVR)
	/* FREEVR stuff */

#  define MAX_BUTTONS    9
#  define MAX_VALUATORS  6
#  define MAX_6SENSORS  10
		vr2switch	*button_inputs[MAX_BUTTONS];
		vrValuator	*valuator_inputs[MAX_VALUATORS];
		float		valuator_sign[MAX_VALUATORS];
		vr6sensor	*sensor6_inputs[MAX_6SENSORS];
#  if 0 /* TODO: not sure if we want/need this */
#    define MAX_CONTROLS  10
		vrControl	*control_inputs[MAX_CONTROLS];
#  endif
		int		active_sim6sensor;	/* The simulated 6-sensor that is being actively controlled */
		vr6sensorConv	sensor6_options;	/* Structure of settings that affect how a 6-sensor is simulated */

#endif /* end library-specific fields */

		/* information about how to filter the data for use with the VR library (FreeVR or CAVE) */
		int		usenullregion;
		int		null_offset[6];

#if defined(TEST_APP) || defined(CAVE)	/* NOTE: these fields are needed by the CAVE and test versions of this input code */
		float		scale_trans;	/* scaling factor for translation */
		float		scale_rot;	/* scaling factor for rotation */
#endif
		float		scale_valuator;	/* scaling factor for valuators */

		int		silent;		/* boolean to determine noise factor */
	} _MagellanPrivateInfo;



	/******************************************************/
	/*** General NON public Magellan interface routines ***/
	/******************************************************/

/*******************************************************************/
/* typename is used to specify a particular device among many that */
/*   share (more or less) the same protocol.  The typename is then */
/*   used to determine what specific features are available with   */
/*   this particular type of device.  Currently there is only one  */
/*   type of device handled by this file, so there is no typename  */
/*   argument.                                                     */
/*                                                                 */
/* TODO: at the moment, this is also being used to reset the sensors   */
/*   by the CAVE library routines.  And while the CAVE reset option    */
/*   is seldom used, this isn't correct, so there should be a separate */
/*   function for resetting the input state, rather than the filter    */
/*   state -- which may also be desired by FreeVR in the future.       */
static void _MagellanInitializeStruct(_MagellanPrivateInfo *mag)
{
	mag->version[0] = '\0';
	mag->lo_buf[0] = '\0';
	mag->lo_buflen = 0;
	mag->buttons = 0;
	mag->offset[VR_TX] = 0;
	mag->offset[VR_TY] = 0;
	mag->offset[VR_TZ] = 0;
	mag->offset[VR_RX] = 0;
	mag->offset[VR_RY] = 0;
	mag->offset[VR_RZ] = 0;

#if defined(TEST_APP) || defined(CAVE)
	mag->scale_trans = TRANS_SENSITIVITY;
	mag->scale_rot = ROT_SENSITIVITY;
#endif
	mag->scale_valuator = VALUATOR_SENSITIVITY;

	mag->silent = 0;

	mag->usenullregion = 0;
	mag->null_offset[VR_TX] = 0;
	mag->null_offset[VR_TY] = 0;
	mag->null_offset[VR_TZ] = 0;
	mag->null_offset[VR_RX] = 0;
	mag->null_offset[VR_RY] = 0;
	mag->null_offset[VR_RZ] = 0;

	mag->bad_Dpackets = 0;
	mag->bad_Kpackets = 0;
	mag->bad_other_packets = 0;

#ifdef FREEVR /* { */
	mag->active_sim6sensor = 1;			/* start with the "wand" active  -- potential bug */
	mag->active_sim6sensor = 0;			/* TODO: go back to wand as default -- currently head */

	mag->sensor6_options.azimuth_axis = VR_Y;	/* azimuth is about the Y axis */
	mag->sensor6_options.relative_axis = 1;		/* default to relative rotations */
	mag->sensor6_options.return_to_zero = 0;	/* default to free floating */
	mag->sensor6_options.ignore_all = 0;		/* default to joystick off */
	mag->sensor6_options.ignore_trans = 0;		/* default to joystick off */
	mag->sensor6_options.restrict_space = 1;	/* default restricted to CAVE-space */
#  if !defined(TEST_APP) && !defined(CAVE)
	mag->sensor6_options.trans_scale = TRANS_SENSITIVITY;
	mag->sensor6_options.rot_scale = ROT_SENSITIVITY;
	mag->sensor6_options.swap_transrot = 0;
#  endif

	/* The default working volume is that of the typical CAVE */
	mag->sensor6_options.working_volume_min[VR_X] = -5.0;
	mag->sensor6_options.working_volume_max[VR_X] =  5.0;
	mag->sensor6_options.working_volume_min[VR_Y] =  0.0;
	mag->sensor6_options.working_volume_max[VR_Y] = 10.0;
	mag->sensor6_options.working_volume_min[VR_Z] = -5.0;
	mag->sensor6_options.working_volume_max[VR_Z] =  5.0;
#endif /* } FREEVR */

#ifdef CAVE /* { */
	CAVEGetTimestamp(&initial_sensor.timestamp);

	mag->head = initial_sensor;

	mag->wand1 = mag->head;
	mag->wand1.y -= 1.0;
	mag->wand1.z -= 1.5;

	mag->wand2 = mag->head;
	mag->wand2.x += 1.0;
	mag->wand2.y -= 1.0;
	mag->wand2.z -= 1.5;

	mag->controls.button[0] = 0.0;
	mag->controls.button[1] = 0.0;
	mag->controls.button[2] = 0.0;
	mag->controls.valuator[SIXDOF_JOYX] = 0.0;
	mag->controls.valuator[SIXDOF_JOYY] = 0.0;
	mag->controls.valuator[SIXDOF_TX] = 0.0;
	mag->controls.valuator[SIXDOF_TY] = 0.0;
	mag->controls.valuator[SIXDOF_TZ] = 0.0;
	mag->controls.valuator[SIXDOF_RX] = 0.0;
	mag->controls.valuator[SIXDOF_RY] = 0.0;
	mag->controls.valuator[SIXDOF_RZ] = 0.0;

	mag->controls.valuator[SIXDOF_TYPE] = SIXDOF_MAG;
	mag->controls.valuator[SIXDOF_VERS] = MAG_CURVERSION;
	mag->controls.valuator[SIXDOF_KYBD] = 0.0;
	mag->controls.valuator[SIXDOF_MODE] = 0.0;
	mag->controls.valuator[SIXDOF_USE] = 0.0;

	mag->active_sim6sensor = &mag->wand1;

	mag->restrict_space = 1;	/* start restricted to CAVE-space */
	mag->relative_axis = 1;		/* start with relative rotations */
	mag->return_to_zero = 0;	/* TODO: implement this */
	mag->valuator_only = 0;		/* start with joystick off */
	mag->valuator_override = 0;	/* start with joystick off */

#endif /* } CAVE */
}


/*******************************************************************/
static void _MagellanPrintStruct(FILE *file, _MagellanPrivateInfo *aux, vrPrintStyle style)
{
	int	count;

	vrFprintf(file, "Magellan device internal structure (%#p):\n", aux);
	vrFprintf(file, "\r\tversion --'%s'\n", aux->version);
	vrFprintf(file, "\r\tfd = %d\n\tport = '%s'\n\tbaud = %d (%d)\n\topen = %d\n",
		aux->fd,
		aux->port,
		aux->baud_int, aux->baud_enum,
		aux->open);
	vrFprintf(file, "\r\tleftover buffer:\n\t\t'%s'\n", aux->lo_buf);
	vrFprintf(file, "\r\tbad D-packets = %d\n\tbad K-packets = %d\n\tbad other packets = %d\n",
		aux->bad_Dpackets,
		aux->bad_Kpackets,
		aux->bad_other_packets);
	vrFprintf(file, "\r\tbeep/silence -- '%s'\n", (aux->silent == 0 ? "beep" : "silent"));

#ifdef FREEVR /* { */
	vrFprintf(file, "\r\tbutton inputs:\n");
	for (count = 0; count < MAX_BUTTONS; count++)
		vrFprintf(file, "\r\t\tbutton_input[%d] = %#p\n", count, aux->button_inputs[count]);

	vrFprintf(file, "\r\tvaluator inputs:\n");
	for (count = 0; count < MAX_VALUATORS; count++)
		vrFprintf(file, "\r\t\tvaluator_inputs[%d] = (%.0f) %#p\n",
			count,
			aux->valuator_sign[count],
			aux->valuator_inputs[count]);

	vrFprintf(file, "\r\t6sensor inputs (active = %d):\n", aux->active_sim6sensor);
	for (count = 0; count < MAX_6SENSORS; count++)
		vrFprintf(file, "\r\t\t6sensor_inputs[%d] = %#p\n", count, aux->sensor6_inputs[count]);
	vrFprintf(file, "\r\tsimulated 6-sensor options:\n"
		"\t\tazimuth axis = %d\n"
		"\t\tignore all values = %d\n"
		"\t\tignore translation values = %d\n"
		"\t\ttemp ignore translation values = %d\n"
		"\t\trelative axis = %d\n"
		"\t\treturn to zero = %d\n"
		"\t\trestrict space = %d\n"
		"\t\tworking volume[VR_X] = % 6.2f -- % 6.2f\n"
		"\t\tworking volume[VR_Y] = % 6.2f -- % 6.2f\n"
		"\t\tworking volume[VR_Z] = % 6.2f -- % 6.2f\n",
		aux->sensor6_options.azimuth_axis,
		aux->sensor6_options.ignore_all,
		aux->sensor6_options.ignore_trans,
		aux->sensor6_options.tmp_ignore_trans,
		aux->sensor6_options.relative_axis,
		aux->sensor6_options.return_to_zero,
		aux->sensor6_options.restrict_space,
		aux->sensor6_options.working_volume_min[VR_X],
		aux->sensor6_options.working_volume_max[VR_X],
		aux->sensor6_options.working_volume_min[VR_Y],
		aux->sensor6_options.working_volume_max[VR_Y],
		aux->sensor6_options.working_volume_min[VR_Z],
		aux->sensor6_options.working_volume_max[VR_Z]);

#if 0 /* TODO: not sure if we want/need this */
	vrFprintf(file, "\r\tcontrol inputs:\n");
	for (count = 0; count < MAX_CONTROLS; count++)
		vrFprintf(file, "\r\t\tcontrol_inputs[%d] = %#p\n", count, aux->control_inputs[count]);
#endif
#endif /* } FREEVR */

	vrFprintf(file, "\r\tusenullregion = %d\n",
		aux->usenullregion);
	vrFprintf(file, "\r\tnull_offset[6] = %d %d %d %d %d %d\n",
		aux->null_offset[0],
		aux->null_offset[1],
		aux->null_offset[2],
		aux->null_offset[3],
		aux->null_offset[4],
		aux->null_offset[5]);
	vrFprintf(file, "\r\tscale_trans = %f\n\tscale_rot = %f\n\tscale_valuator = %f\n",
#  if defined(FREEVR)
		aux->sensor6_options.trans_scale,
		aux->sensor6_options.rot_scale,
#  else
		aux->scale_trans,
		aux->scale_rot,
#endif
		aux->scale_valuator);
	vrFprintf(file, "\r\tsilent = %d\n",
		aux->silent);

	vrFprintf(file, "\r\t%01X %c  (%d, %d,%d) %c%c%c%c%c%c%c%c%c  x =%4d y =%4d z =%4d a =%4d b =%4d c =%4d\n",
		aux->mode,
		aux->lo_buf[0],
		aux->null_radius, aux->trans_gain, aux->rotate_gain,
		aux->buttons & 0x0001 ? '1' : '-',
		aux->buttons & 0x0002 ? '2' : '-',
		aux->buttons & 0x0004 ? '3' : '-',
		aux->buttons & 0x0008 ? '4' : '-',
		aux->buttons & 0x0010 ? '5' : '-',
		aux->buttons & 0x0020 ? '6' : '-',
		aux->buttons & 0x0040 ? '7' : '-',
		aux->buttons & 0x0080 ? '8' : '-',
		aux->buttons & 0x0100 ? '*' : '-',
		aux->offset[VR_TX], aux->offset[VR_TY], aux->offset[VR_TZ], aux->offset[VR_RX], aux->offset[VR_RY], aux->offset[VR_RZ]);
}


/**************************************************************************/
static void _MagellanPrintHelp(FILE *file, _MagellanPrivateInfo *aux)
{
	int	count;		/* looping variable */

#if 0 || defined(TEST_APP)
	vrFprintf(file, BOLD_TEXT "Sorry, Magellan - print_help control not yet implemented.\n" NORM_TEXT);
#else
	vrFprintf(file, BOLD_TEXT "Magellan - inputs:" NORM_TEXT "\n");
	for (count = 0; count < MAX_BUTTONS; count++) {
		if (aux->button_inputs[count] != NULL) {
			vrFprintf(file, "\t%s -- %s%s\n",
				aux->button_inputs[count]->my_object->desc_str,
				(aux->button_inputs[count]->input_type == VRINPUT_CONTROL ? "control:" : ""),
				aux->button_inputs[count]->my_object->name);
		}
	}
	for (count = 0; count < MAX_VALUATORS; count++) {
		if (aux->valuator_inputs[count] != NULL) {
			vrFprintf(file, "\t%s -- %s%s\n",
				aux->valuator_inputs[count]->my_object->desc_str,
				(aux->valuator_inputs[count]->input_type == VRINPUT_CONTROL ? "control:" : ""),
				aux->valuator_inputs[count]->my_object->name);
		}
	}
	for (count = 0; count < MAX_6SENSORS; count++) {
		if (aux->sensor6_inputs[count] != NULL) {
			vrFprintf(file, "\t%s -- %s%s\n",
				aux->sensor6_inputs[count]->my_object->desc_str,
				(aux->sensor6_inputs[count]->input_type == VRINPUT_CONTROL ? "control:" : ""),
				aux->sensor6_inputs[count]->my_object->name);
		}
	}
#endif
}


/*******************************************************************/
static void _MagellanPrintIncoming(FILE *file, _MagellanPrivateInfo *aux)
{
	vrFprintf(file, "version --'%s'\n", aux->version);
	vrFprintf(file, "\t%01X %c  (%d, %d,%d) %c%c%c%c%c%c%c%c%c  x =%4d y =%4d z =%4d a =%4d b =%4d c =%4d\n",
		aux->mode,
		aux->lo_buf[0],
		aux->null_radius, aux->trans_gain, aux->rotate_gain,
		aux->buttons & 0x0001 ? '1' : '-',
		aux->buttons & 0x0002 ? '2' : '-',
		aux->buttons & 0x0004 ? '3' : '-',
		aux->buttons & 0x0008 ? '4' : '-',
		aux->buttons & 0x0010 ? '5' : '-',
		aux->buttons & 0x0020 ? '6' : '-',
		aux->buttons & 0x0040 ? '7' : '-',
		aux->buttons & 0x0080 ? '8' : '-',
		aux->buttons & 0x0100 ? '*' : '-',
		aux->offset[VR_TX], aux->offset[VR_TY], aux->offset[VR_TZ], aux->offset[VR_RX], aux->offset[VR_RY], aux->offset[VR_RZ]);
}


/*********************************************************************/
/* Convert magic character to 4-bit quantity, using the table of     */
/*   magic characters the Magellan offers.  Why didn't they just use */
/*   hex digits?  Maybe for error checking?  Otherwise, who knows?   */
/*********************************************************************/

static	char	_MagellanNibbleTable[] = "0AB3D56GH9:K<MN?";


/*******************************************************************/
static signed int _MagellanCharToNibble(char Value)
{
	int	value;

	value = Value & 0x000F;
	if (_MagellanNibbleTable[value] == Value)
		return value;

	vrFprintf(stderr, "\n\rMagellan: Nibble Error %02X %02X %d \n\r",
		Value, _MagellanNibbleTable[value], value);
	return value;
}


/*******************************************************************/
static int _MagellanNibbleToChar(int nibble)
{
	if (nibble < 0 || nibble > 15)
		vrFprintf(stderr, "Magellan: _MagellanNibbleToChar(%d) only accepts values in 0..15\n", nibble);

	return _MagellanNibbleTable[nibble & 0x0F];
}


/*******************************************************************/
static int _Magellan4BytesToInt(char *bp)
{
	return (_MagellanCharToNibble(bp[0]) << 12)
		+ (_MagellanCharToNibble(bp[1]) << 8)
		+ (_MagellanCharToNibble(bp[2]) << 4)
		+  _MagellanCharToNibble(bp[3])
		- 32768;
}

/*******************************************************************/
/*******************************************************************/


/*******************************************************************/
static int _MagellanCalcNullRegion(int null, int val)
{
	if (abs(val) > null) {
		return ((val > 0) ? (val - null) : (val + null));
	}
	return 0;
}


/*******************************************************************/
static void _MagellanDoNullRegion(_MagellanPrivateInfo *mag)
{
	mag->offset[VR_TX] = _MagellanCalcNullRegion(mag->null_offset[VR_TX], mag->offset[VR_TX]);
	mag->offset[VR_TY] = _MagellanCalcNullRegion(mag->null_offset[VR_TY], mag->offset[VR_TY]);
	mag->offset[VR_TZ] = _MagellanCalcNullRegion(mag->null_offset[VR_TZ], mag->offset[VR_TZ]);
	mag->offset[VR_RX] = _MagellanCalcNullRegion(mag->null_offset[VR_RX], mag->offset[VR_RX]);
	mag->offset[VR_RY] = _MagellanCalcNullRegion(mag->null_offset[VR_RY], mag->offset[VR_RY]);
	mag->offset[VR_RZ] = _MagellanCalcNullRegion(mag->null_offset[VR_RZ], mag->offset[VR_RZ]);
}



/*******************************************************************/
static void _MagellanDecodeBuffer(_MagellanPrivateInfo *mag, char *buffer)
{
	int	buflen = strlen(buffer);

#if MAG_DEBUG>1
	vrPrintf("set to decode %x(%c)(%c)(%c)(%c)\n", buffer[0],buffer[0], buffer[1], buffer[2], buffer[3]);
#endif
	switch (buffer[0]) {

	case 'v':  /* hardware version */
		strncpy(mag->version, &buffer[1], sizeof(mag->version));
		mag->version[strlen(mag->version)-1] = '\0';	/* remove trailing CR */
		break;

	case 'k':  /* keyboard */
		if (buflen == 5) {
			mag->buttons = _MagellanCharToNibble(buffer[1]) +
				_MagellanCharToNibble(buffer[2]) * 16 +
				_MagellanCharToNibble(buffer[3]) * 256;
		} else {
			mag->bad_Kpackets++;
		}
		break;

	case 'd':  /* dial movement */
		if (buflen == 26) {
			mag->offset[VR_TX] = _Magellan4BytesToInt(&buffer[ 1]);
			mag->offset[VR_TY] = _Magellan4BytesToInt(&buffer[ 5]);
			mag->offset[VR_TZ] = _Magellan4BytesToInt(&buffer[ 9]);
			mag->offset[VR_RX] = _Magellan4BytesToInt(&buffer[13]);
			mag->offset[VR_RY] = _Magellan4BytesToInt(&buffer[17]);
			mag->offset[VR_RZ] = _Magellan4BytesToInt(&buffer[21]);

#if 1
			if (mag->usenullregion)
				_MagellanDoNullRegion(mag);
#endif
		} else {
			mag->bad_Dpackets++;
		}
		break;

	case 'm':  /* magellan mode */
		if (buflen == 3) {
			mag->mode = _MagellanCharToNibble(buffer[1]);
		} else {
			mag->bad_other_packets++;
		}
		break;

	case 'n':  /* null radius */
		if (buflen == 3) {
			mag->null_radius = _MagellanCharToNibble(buffer[1]);
		} else {
			mag->bad_other_packets++;
		}
		break;

	case 'q':  /* quality values */
		if (buflen == 4) {
			mag->trans_gain = _MagellanCharToNibble(buffer[1]);
			mag->rotate_gain = _MagellanCharToNibble(buffer[2]);
		} else {
			mag->bad_other_packets++;
		}
		break;

	case 'p':  /* datarate? values */
		if (buflen == 4) {
			mag->datarate1 = _MagellanCharToNibble(buffer[1]);
			mag->datarate2 = _MagellanCharToNibble(buffer[2]);
		} else {
			mag->bad_other_packets++;
		}
		break;

	case 'z':
	case 'b':
		break;
	default:
#ifdef MAG_DEBUG
		vrFprintf(stderr, "_MagellanDecodeBuffer(): unable to decode response -- '%s'\n", buffer);
#endif
		mag->bad_other_packets++;
		break;
	}


#ifdef MAG_DEBUG
	_MagellanPrintIncoming(stderr, mag);
	vrFprintf(stderr, "\n");
#endif
}


/*******************************************************************/
/*   This function returns a character based on the most recently received   */
/*   message type (i.e. the first character of the message).                 */
static char _MagellanReadInput(_MagellanPrivateInfo *mag)
{

#if 1
	char	buffer[512];

	if (vrSerialReadToCR(mag->fd, mag->lo_buf, sizeof(mag->lo_buf), mag->lo_buflen, buffer, sizeof(buffer)) == NULL) {
		return '\0';
	}
#elif 1
	char	*buffer;	/* NOTE: if there version were used the memory allocated */
				/*   for buffer would later need to be freed.            */

	if ((buffer = vrSerialReadToCR_Alt(mag->fd)) == NULL) {
		return '\0';
	}
#else
	char	buffer[512];

	if (MAG_Read(mag, buffer, sizeof(buffer)) == NULL) {
		return '\0';
	}
#endif

	_MagellanDecodeBuffer(mag, buffer);

	return (buffer[0]);
}


/*******************************************************************/
/* _MagellanInitializeDevice() is called in the OPEN phase  */
/*   of input interface -- after the types of inputs have   */
/*   been determined (during the CREATE phase).             */
static void _MagellanInitializeDevice(_MagellanPrivateInfo *mag)
{
	do {
		vrSerialWriteString(mag->fd, StartupMsg);
		vrSerialAwaitData(mag->fd);
	} while(_MagellanReadInput(mag) != 'z');


	do {
		vrSerialWriteString(mag->fd, DataRateMsg);
		vrSerialAwaitData(mag->fd);
	} while (_MagellanReadInput(mag) != 'p');

	do {
		vrSerialWriteString(mag->fd, VersionMsg);
		vrSerialAwaitData(mag->fd);
	} while (_MagellanReadInput(mag) != 'v');

	do {
		vrSerialWriteString(mag->fd, QualityMsg);
		vrSerialAwaitData(mag->fd);
	} while (_MagellanReadInput(mag) != 'q');

	do {
		vrSerialWriteString(mag->fd, NullRadiusMsg);
		vrSerialAwaitData(mag->fd);
	} while (_MagellanReadInput(mag) != 'n');

	do {
		vrSerialWriteString(mag->fd, ZeroMsg);
		vrSerialAwaitData(mag->fd);
	} while (_MagellanReadInput(mag) != 'z');

	if (!mag->silent) {
		do {
			vrSerialWriteString(mag->fd, ShortBeepMsg);
			vrSerialAwaitData(mag->fd);
		} while (_MagellanReadInput(mag) != 'b');
	}

	do {
		vrSerialWriteString(mag->fd, ModeMsg);
		vrSerialAwaitData(mag->fd);
	} while (_MagellanReadInput(mag) != 'm');
}


/************************************************************/
static unsigned int _MagellanButtonValue(char *buttonname)
{
	switch (buttonname[0]) {
	case '1':
		return MAG_BUTTONINDEX_1;
	case '2':
		return MAG_BUTTONINDEX_2;
	case '3':
		return MAG_BUTTONINDEX_3;
	case '4':
		return MAG_BUTTONINDEX_4;
	case '5':
		return MAG_BUTTONINDEX_5;
	case '6':
		return MAG_BUTTONINDEX_6;
	case '7':
		return MAG_BUTTONINDEX_7;
	case '8':
		return MAG_BUTTONINDEX_8;
	case '*':
	case 's':
	case '9':
		return MAG_BUTTONINDEX_STAR;
	}

	return -1;
}


/************************************************************/
static unsigned int _MagellanValuatorValue(char *valuatorname)
{
	if      (!strcasecmp(valuatorname,  "tx"))	return VR_TX;
	else if (!strcasecmp(valuatorname, "-tx"))	return VR_TX;
	else if (!strcasecmp(valuatorname,  "ty"))	return VR_TY;
	else if (!strcasecmp(valuatorname, "-ty"))	return VR_TY;
	else if (!strcasecmp(valuatorname,  "tz"))	return VR_TZ;
	else if (!strcasecmp(valuatorname, "-tz"))	return VR_TZ;
	else if (!strcasecmp(valuatorname,  "rx"))	return VR_RX;
	else if (!strcasecmp(valuatorname, "-rx"))	return VR_RX;
	else if (!strcasecmp(valuatorname,  "ry"))	return VR_RY;
	else if (!strcasecmp(valuatorname, "-ry"))	return VR_RY;
	else if (!strcasecmp(valuatorname,  "rz"))	return VR_RZ;
	else if (!strcasecmp(valuatorname, "-rz"))	return VR_RZ;

	else {
		vrErrPrintf("_MagellanValuatorValue: unknown valuator name '%s'\n", valuatorname);
		return -1;
	}
}


	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/


#if defined(FREEVR) /* { */
/********************************************************************/
/*** Functions for FreeVR access of Magellan as a tracking device ***/
/********************************************************************/


	/************************************/
	/***  FreeVR NON public routines  ***/
	/************************************/


/* TODO: I don't think we want this function in this file, since multiple */
/*   input devices can be interfaced with in the same process.            */
/******************************************************/
static void _MagellanSigpipeHandler(int sig)
{
	vrDbgPrintf("Caught SIGPIPE error, process dying.\n");
}


/******************************************************/
static void _MagellanParseArgs(_MagellanPrivateInfo *aux, char *args)
{
	int 	null_value;		/* for reading one of the null region values */
	float	scale_value;		/* for reading one of the scaling factor values */
	float	volume_values[6];	/* for reading the working volume array */

	/* In the rare case of no arguments, just return */
	if (args == NULL)
		return;

	/**************************************/
	/** Argument format: "port" "=" file **/
	/**************************************/
	vrArgParseString(args, "port", &(aux->port));

	/****************************************/
	/** Argument format: "baud" "=" number **/
	/****************************************/
	if (vrArgParseInteger(args, "baud", &(aux->baud_int))) {
		aux->baud_enum = vrSerialBaudIntToEnum(aux->baud_int);
	}

	/*********************************************/
	/** Argument format: "transNull" "=" number **/
	/*********************************************/
	if (vrArgParseInteger(args, "transnull", &null_value)) {
		aux->null_offset[VR_TX] = null_value;
		aux->null_offset[VR_TY] = null_value;
		aux->null_offset[VR_TZ] = null_value;
	}

	/*******************************************/
	/** Argument format: "rotNull" "=" number **/
	/*******************************************/
	if (vrArgParseInteger(args, "rotnull", &null_value)) {
		aux->null_offset[VR_RX] = null_value;
		aux->null_offset[VR_RY] = null_value;
		aux->null_offset[VR_RZ] = null_value;
	}

	/**************************************************************/
	/** Argument format: "useNull" "=" { "on" | "off" | number } **/
	/**************************************************************/
	vrArgParseBool(args, "usenull", &(aux->usenullregion));

	/**********************************************/
	/** Argument format: "transScale" "=" number **/
	/**********************************************/
	if (vrArgParseFloat(args, "transscale", &scale_value)) {
		aux->sensor6_options.trans_scale = scale_value * TRANS_SENSITIVITY;
	}

	/********************************************/
	/** Argument format: "rotScale" "=" number **/
	/********************************************/
	if (vrArgParseFloat(args, "rotscale", &scale_value)) {
		aux->sensor6_options.rot_scale = scale_value * ROT_SENSITIVITY;
	}

	/********************************************/
	/** Argument format: "valScale" "=" number **/
	/********************************************/
	if (vrArgParseFloat(args, "valscale", &scale_value)) {
		aux->scale_valuator = scale_value * VALUATOR_SENSITIVITY;
	}

	/*************************************************************/
	/** Argument format: "silent" "=" { "on" | "off" | number } **/
	/*************************************************************/
	vrArgParseBool(args, "silent", &(aux->silent));

	/***************************************************************/
	/** Argument format: "restrict" "=" { "on" | "off" | number } **/
	/***************************************************************/
	vrArgParseBool(args, "restrict", &(aux->sensor6_options.restrict_space));

	/***********************************************************************/
	/** Argument format: "valuatorOverride" "=" { "on" | "off" | number } **/
	/***********************************************************************/
	vrArgParseBool(args, "valuatoroverride", &(aux->sensor6_options.ignore_trans));

	/*******************************************************************/
	/** Argument format: "returnToZero" "=" { "on" | "off" | number } **/
	/*******************************************************************/
	vrArgParseBool(args, "returntozero", &(aux->sensor6_options.return_to_zero));

	/******************************************************************/
	/** Argument format: "relativeRot" "=" { "on" | "off" | number } **/
	/******************************************************************/
	vrArgParseBool(args, "relativerot", &(aux->sensor6_options.relative_axis));

	/******************************************************************************************/
	/** Argument format: "workingVolume" "=" <float> <float> <float> <float> <float> <float> **/
	/******************************************************************************************/
	if (vrArgParseFloatList(args, "workingvolume", volume_values, 6)) {
		aux->sensor6_options.working_volume_min[VR_X] = volume_values[0];
		aux->sensor6_options.working_volume_max[VR_X] = volume_values[1];
		aux->sensor6_options.working_volume_min[VR_Y] = volume_values[2];
		aux->sensor6_options.working_volume_max[VR_Y] = volume_values[3];
		aux->sensor6_options.working_volume_min[VR_Z] = volume_values[4];
		aux->sensor6_options.working_volume_max[VR_Z] = volume_values[5];
	}
}


/************************************************************/
static void _MagellanGetData(vrInputDevice *devinfo)
{
	_MagellanPrivateInfo	*aux = (_MagellanPrivateInfo *)devinfo->aux_data;
#if defined(TEST_APP) || defined(CAVE)
	double			scale_trans = aux->scale_trans;
	double			scale_rot = aux->scale_rot;
#endif
	char			msg_type;
static	int			buttons_last = 0;
	int			count;
	float			values[6];

    do {
		/* Loop: consume all data received so far in one gulp */

	/*******************/
	/* gather the data */
	msg_type = _MagellanReadInput(aux);

	switch(msg_type) {

	/***************/
	/** valuators **/
	case 'd':	/* ie. valuators changed */
		if (aux->sensor6_options.ignore_all || aux->sensor6_options.ignore_trans) {
			for (count = 0; count < (aux->sensor6_options.ignore_all ? 6 : 3); count++) {
				if (aux->valuator_inputs[count] != NULL) {
					switch (aux->valuator_inputs[count]->input_type) {
					case VRINPUT_VALUATOR:
						vrAssignValuatorValue((vrValuator *)(aux->valuator_inputs[count]), aux->offset[count] * aux->scale_valuator * aux->valuator_sign[count]);
						break;
					case VRINPUT_CONTROL:
						/* TODO: ... */
						break;
					default:
						/* TODO: ... */
						break;
					}
				}
			}
		}

		if (!aux->sensor6_options.ignore_all && devinfo->num_6sensors > 0) {
			vr6sensor	*sensor;

#if defined(TEST_APP) || defined(CAVE)
			values[VR_X] = aux->offset[VR_X] * scale_trans;
			values[VR_Y] = aux->offset[VR_Y] * scale_trans;
			values[VR_Z] = aux->offset[VR_Z] * scale_trans;
			values[VR_AZIM+3] = aux->offset[VR_AZIM+3] * scale_rot;
			values[VR_ELEV+3] = aux->offset[VR_ELEV+3] * scale_rot;
			values[VR_ROLL+3] = aux->offset[VR_ROLL+3] * scale_rot;
#else
			values[VR_X] = aux->offset[VR_X];
			values[VR_Y] = aux->offset[VR_Y];
			values[VR_Z] = aux->offset[VR_Z];
			values[VR_AZIM+3] = aux->offset[VR_AZIM+3];
			values[VR_ELEV+3] = aux->offset[VR_ELEV+3];
			values[VR_ROLL+3] = aux->offset[VR_ROLL+3];
#endif

			sensor = &(devinfo->sensor6[aux->active_sim6sensor]);
			vrAssign6sensorValueFromValuators(sensor, values, &(aux->sensor6_options), -1);
			vrAssign6sensorActiveValue(sensor, 1);
		}

		break;

	/*************/
	/** buttons **/
	case 'k':	/* ie. buttons changed */

		for (count = 0; count < MAX_BUTTONS; count++) {
			if ((aux->buttons & (0x1 << count)) != (buttons_last & (0x1 << count))) {
				if (aux->button_inputs[count] != NULL) {
					switch (aux->button_inputs[count]->input_type) {
					case VRINPUT_BINARY:
						if (!aux->silent)
							vrSerialWriteString(aux->fd, ShortBeepMsg);
						vrAssign2switchValue((vr2switch *)(aux->button_inputs[count]), ((aux->buttons & (0x1 << count)) != 0));
						break;
					case VRINPUT_CONTROL:
						vrCallbackInvokeDynamic(((vrControl *)(aux->button_inputs[count]))->callback, 1, ((aux->buttons & (0x1 << count)) != 0));
						break;
					default:
						vrErrPrintf(RED_TEXT "_MagellanGetData: Unable to handle button inputs that aren't Binary or Control inputs\n" NORM_TEXT);
						break;
					}
				}
			}
		}

		buttons_last = aux->buttons;

		break;
	}
     } while (aux->lo_buflen > 0 && msg_type != '\0');
}


	/*****************************************************************/
	/*    Function(s) for parsing Magellan "input" declarations.     */
	/*                                                               */
	/*  These _Magellan<type>Input() functions are called during the */
	/*  CREATE phase of the input interface.                         */

/************************************************************/
static vrInputMatch _MagellanButtonInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
	_MagellanPrivateInfo	*aux = (_MagellanPrivateInfo *)devinfo->aux_data;
	int			button_num;

	button_num = _MagellanButtonValue(dti->instance);
	if (button_num == -1)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_MagellanButtonInput: Warning, key['%s'] did not match any known key\n", dti->instance);
	else if (aux->button_inputs[button_num] != NULL)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_MagellanButtonInput: Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);
	aux->button_inputs[button_num] = (vr2switch *)input;
	vrDbgPrintfN(INPUT_DBGLVL, "_MagellanButtonInput: Assigned button event of value 0x%02x to input pointer = %#p)\n",
		button_num, aux->button_inputs[button_num]);

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/************************************************************/
static vrInputMatch _MagellanMagInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
	_MagellanPrivateInfo	*aux = (_MagellanPrivateInfo *)devinfo->aux_data;
	int			valuator_num;

	valuator_num = _MagellanValuatorValue(dti->instance);
	if (valuator_num == -1)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_MagellanMagInput: Warning, key['%s'] did not match any known key\n", dti->instance);
	else if (aux->valuator_inputs[valuator_num] != NULL)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_MagellanMagInput: Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);
	aux->valuator_inputs[valuator_num] = (vrValuator *)input;
	if (dti->instance[0] == '-')
		aux->valuator_sign[valuator_num] = -1.0;
	else	aux->valuator_sign[valuator_num] =  1.0;
	vrDbgPrintfN(INPUT_DBGLVL, "_MagellanMagInput: Assigned valuator event of value 0x%02x to input pointer = %#p)\n",
		valuator_num, aux->valuator_inputs[valuator_num]);

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/************************************************************/
static vrInputMatch _Magellan6DofInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
	_MagellanPrivateInfo	*aux = (_MagellanPrivateInfo *)devinfo->aux_data;
	int			sensor_num;

	sensor_num = vrAtoI(dti->instance);
	if (sensor_num < 0)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_Magellan6DofInput: Warning, sensor number must be between %d and %d\n", 0, MAX_6SENSORS);
	else if (aux->sensor6_inputs[sensor_num] != NULL)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_Magellan6DofInput: Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);
	aux->sensor6_inputs[sensor_num] = (vr6sensor *)input;
	vrAssign6sensorR2Exform(aux->sensor6_inputs[sensor_num], strchr(dti->instance, ','));
	vrDbgPrintfN(INPUT_DBGLVL, "_Magellan6DofInput: Assigned 6sensor event of value 0x%02x to input pointer = %#p)\n",
		sensor_num, aux->sensor6_inputs[sensor_num]);

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


	/************************************************************/
	/***************** FreeVR Callback routines *****************/
	/************************************************************/

	/************************************************************/
	/*   Callbacks for controlling the Magellan features.       */
	/*                                                          */

/************************************************************/
static void _MagellanSystemPauseToggleCallback(vrInputDevice *devinfo, int value)
{
	_MagellanPrivateInfo	*aux = (_MagellanPrivateInfo *)devinfo->aux_data;

        if (value == 0)
                return;

	if (!aux->silent)
		vrSerialWriteString(aux->fd, LongBeepMsg);

	devinfo->context->paused ^= 1;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Magellan Control: system_pause = %d.\n",
		devinfo->context->paused);
}

/************************************************************/
static void _MagellanPrintContextStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintContext(stdout, devinfo->context, verbose);
}

/************************************************************/
static void _MagellanPrintConfigStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintConfig(stdout, devinfo->context->config, verbose);
}

/************************************************************/
static void _MagellanPrintInputStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintInput(stdout, devinfo->context->input, verbose);
}

/************************************************************/
static void _MagellanPrintHelpCallback(vrInputDevice *devinfo, int value)
{
        _MagellanPrivateInfo  *aux = (_MagellanPrivateInfo *)devinfo->aux_data;

        if (value == 0)
                return;

        _MagellanPrintHelp(stdout, aux);
}

/************************************************************/
static void _MagellanPrintStructCallback(vrInputDevice *devinfo, int value)
{
	_MagellanPrivateInfo	*aux = (_MagellanPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	if (!aux->silent)
		vrSerialWriteString(aux->fd, LongBeepMsg);

	_MagellanPrintStruct(stdout, aux, verbose);
}

/************************************************************/
static void _MagellanSensorNextCallback(vrInputDevice *devinfo, int value)
{
	_MagellanPrivateInfo	*aux = (_MagellanPrivateInfo *)devinfo->aux_data;

#if 0
	if (vrDbgDo(SELFCTRL_DBGLVL))
		_MagellanPrintStruct(stdout, aux, verbose);
#endif

	if (value == 0)
		return;

	if (!aux->silent)
		vrSerialWriteString(aux->fd, LongBeepMsg);

	if (devinfo->num_6sensors == 0) {
		vrDbgPrintfN(SELFCTRL_DBGLVL, "Magellan Control: next sensor -- no sensors available.\n",
			aux->active_sim6sensor);
		return;
	}

	/* set the current sensor as non-active */
	vrAssign6sensorActiveValue(((vr6sensor *)aux->sensor6_inputs[aux->active_sim6sensor]), 0);

	/* search from the next possible sensor to the end of the list */
	do {
		aux->active_sim6sensor++;
	} while (aux->sensor6_inputs[aux->active_sim6sensor] == NULL && aux->active_sim6sensor < MAX_6SENSORS);

	/* if none found (ie. we were basically already at the end of the list), then search from the beginning */
	if (aux->sensor6_inputs[aux->active_sim6sensor] == NULL || aux->active_sim6sensor >= MAX_6SENSORS) {
		for (aux->active_sim6sensor = 0; aux->active_sim6sensor < MAX_6SENSORS && aux->sensor6_inputs[aux->active_sim6sensor] == NULL; aux->active_sim6sensor++);
	}

	/* set the newly found sensor as active */
	vrAssign6sensorActiveValue(((vr6sensor *)aux->sensor6_inputs[aux->active_sim6sensor]), 1);

	vrDbgPrintfN(SELFCTRL_DBGLVL, "Magellan Control: 6sensor[%d] now active.\n", aux->active_sim6sensor);
}

/* TODO: see if there is a way to call this as an N-switch */
/************************************************************/
static void _MagellanSensorSetCallback(vrInputDevice *devinfo, int value)
{
	_MagellanPrivateInfo	*aux = (_MagellanPrivateInfo *)devinfo->aux_data;

	if (value == aux->active_sim6sensor)
		return;

	if (!aux->silent)
		vrSerialWriteString(aux->fd, LongBeepMsg);

	if (value < 0 || value >= MAX_6SENSORS) {
		vrDbgPrintfN(SELFCTRL_DBGLVL, "Magellan Control: set sensor (%d) -- out of range.\n", value);
	}

	if (aux->sensor6_inputs[value] == NULL) {
		vrDbgPrintfN(SELFCTRL_DBGLVL, "Magellan Control: set sensor (%d) -- no such sensor available.\n", value);
		return;
	}

	vrAssign6sensorActiveValue(((vr6sensor *)aux->sensor6_inputs[aux->active_sim6sensor]), 0);
	aux->active_sim6sensor = value;
	vrAssign6sensorActiveValue(((vr6sensor *)aux->sensor6_inputs[aux->active_sim6sensor]), 1);

	vrDbgPrintfN(SELFCTRL_DBGLVL, "Magellan Control: 6sensor[%d] now active.\n", aux->active_sim6sensor);
}

/************************************************************/
static void _MagellanSensorSet0Callback(vrInputDevice *devinfo, int value)
{	_MagellanSensorSetCallback(devinfo, 0); }

/************************************************************/
static void _MagellanSensorSet1Callback(vrInputDevice *devinfo, int value)
{	_MagellanSensorSetCallback(devinfo, 1); }

/************************************************************/
static void _MagellanSensorSet2Callback(vrInputDevice *devinfo, int value)
{	_MagellanSensorSetCallback(devinfo, 2); }

/************************************************************/
static void _MagellanSensorSet3Callback(vrInputDevice *devinfo, int value)
{	_MagellanSensorSetCallback(devinfo, 3); }

/************************************************************/
static void _MagellanSensorSet4Callback(vrInputDevice *devinfo, int value)
{	_MagellanSensorSetCallback(devinfo, 4); }

/************************************************************/
static void _MagellanSensorSet5Callback(vrInputDevice *devinfo, int value)
{	_MagellanSensorSetCallback(devinfo, 5); }

/************************************************************/
static void _MagellanSensorSet6Callback(vrInputDevice *devinfo, int value)
{	_MagellanSensorSetCallback(devinfo, 6); }

/************************************************************/
static void _MagellanSensorSet7Callback(vrInputDevice *devinfo, int value)
{	_MagellanSensorSetCallback(devinfo, 7); }

/************************************************************/
static void _MagellanSensorSet8Callback(vrInputDevice *devinfo, int value)
{	_MagellanSensorSetCallback(devinfo, 8); }

/************************************************************/
static void _MagellanSensorSet9Callback(vrInputDevice *devinfo, int value)
{	_MagellanSensorSetCallback(devinfo, 9); }

/************************************************************/
static void _MagellanSensorResetCallback(vrInputDevice *devinfo, int value)
{
static	vrMatrix		idmat = { { 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0 } };
	_MagellanPrivateInfo	*aux = (_MagellanPrivateInfo *)devinfo->aux_data;
	vr6sensor		*sensor;

	if (value == 0)
		return;

	if (!aux->silent)
		vrSerialWriteString(aux->fd, LongBeepMsg);

	sensor = &(devinfo->sensor6[aux->active_sim6sensor]);
	vrAssign6sensorValue(sensor, &idmat, 0 /* , timestamp */);
	vrAssign6sensorActiveValue(sensor, -1);
	vrAssign6sensorErrorValue(sensor, 0);
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Magellan Control: reset 6sensor[%d].\n", aux->active_sim6sensor);
}

/************************************************************/
static void _MagellanSensorResetAllCallback(vrInputDevice *devinfo, int value)
{
static	vrMatrix		idmat = { { 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0 } };
	_MagellanPrivateInfo	*aux = (_MagellanPrivateInfo *)devinfo->aux_data;
	vr6sensor		*sensor;
	int			count;

	if (value == 0)
		return;

	if (!aux->silent)
		vrSerialWriteString(aux->fd, LongBeepMsg);

	for (count = 0; count < MAX_6SENSORS; count++) {
		if (aux->sensor6_inputs[count] != NULL) {
			sensor = &(devinfo->sensor6[count]);
			vrAssign6sensorValue(sensor, &idmat, 0 /* , timestamp */);
			vrAssign6sensorActiveValue(sensor, (count == aux->active_sim6sensor));
			vrAssign6sensorErrorValue(sensor, 0);
			vrDbgPrintfN(SELFCTRL_DBGLVL, "Magellan Control: reset 6sensor[%d].\n", count);
		}
	}
}

/************************************************************/
static void _MagellanTempValuatorOverrideCallback(vrInputDevice *devinfo, int value)
{
	_MagellanPrivateInfo	*aux = (_MagellanPrivateInfo *)devinfo->aux_data;

	if (!aux->silent)
		vrSerialWriteString(aux->fd, LongBeepMsg);

	/* set the field to the current state of value (ie. 1 when depressed, 0 when released) */
	aux->sensor6_options.ignore_trans = value;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Magellan Control: ignore_trans = %d.\n",
		aux->sensor6_options.ignore_trans);
}

/************************************************************/
static void _MagellanToggleValuatorOverrideCallback(vrInputDevice *devinfo, int value)
{
	_MagellanPrivateInfo	*aux = (_MagellanPrivateInfo *)devinfo->aux_data;

	/* take no action when the button is released */
	if (value == 0)
		return;

	if (!aux->silent)
		vrSerialWriteString(aux->fd, LongBeepMsg);

	/* toggle the field (NOTE: we only get here when button is depressed) */
	aux->sensor6_options.ignore_trans ^= 1;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Magellan Control: ignore_trans = %d.\n",
		aux->sensor6_options.ignore_trans);
}

/************************************************************/
static void _MagellanTempValuatorOnlyCallback(vrInputDevice *devinfo, int value)
{
	_MagellanPrivateInfo	*aux = (_MagellanPrivateInfo *)devinfo->aux_data;

	if (!aux->silent)
		vrSerialWriteString(aux->fd, LongBeepMsg);

	/* set the field to the current state of value (ie. 1 when depressed, 0 when released) */
	aux->sensor6_options.ignore_all = value;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Magellan Control: ignore_all = %d.\n",
		aux->sensor6_options.ignore_all);
}

/************************************************************/
static void _MagellanToggleValuatorOnlyCallback(vrInputDevice *devinfo, int value)
{
	_MagellanPrivateInfo	*aux = (_MagellanPrivateInfo *)devinfo->aux_data;

	/* take no action when the button is released */
	if (value == 0)
		return;

	if (!aux->silent)
		vrSerialWriteString(aux->fd, LongBeepMsg);

	/* toggle the field (NOTE: we only get here when button is depressed) */
	aux->sensor6_options.ignore_all ^= 1;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Magellan Control: ignore_all = %d.\n",
		aux->sensor6_options.ignore_all);
}

/************************************************************/
static void _MagellanToggleRelativeAxesCallback(vrInputDevice *devinfo, int value)
{
	_MagellanPrivateInfo	*aux = (_MagellanPrivateInfo *)devinfo->aux_data;

	/* take no action when the button is released */
	if (value == 0)
		return;

	if (!aux->silent)
		vrSerialWriteString(aux->fd, LongBeepMsg);

	/* toggle the field (NOTE: we only get here when button is depressed) */
	aux->sensor6_options.relative_axis ^= 1;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Magellan Control: relative_axis = %d.\n",
		aux->sensor6_options.relative_axis);
}


/************************************************************/
/* TODO: this should probably also go through all the 6sensor's  */
/*   and move them to be within the allowed workspace when space */
/*   restriction is turned on.                                   */
static void _MagellanToggleRestrictSpaceCallback(vrInputDevice *devinfo, int value)
{
	_MagellanPrivateInfo	*aux = (_MagellanPrivateInfo *)devinfo->aux_data;

	/* take no action when the button is released */
	if (value == 0)
		return;

	if (!aux->silent)
		vrSerialWriteString(aux->fd, LongBeepMsg);

	/* toggle the field (NOTE: we only get here when button is depressed) */
	aux->sensor6_options.restrict_space ^= 1;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Magellan Control: restrict_space = %d.\n",
		aux->sensor6_options.restrict_space);
}

/************************************************************/
static void _MagellanToggleReturnToZeroCallback(vrInputDevice *devinfo, int value)
{
	_MagellanPrivateInfo	*aux = (_MagellanPrivateInfo *)devinfo->aux_data;

	/* take no action when the button is released */
	if (value == 0)
		return;

	if (!aux->silent)
		vrSerialWriteString(aux->fd, LongBeepMsg);

	/* toggle the field (NOTE: we only get here when button is depressed) */
	aux->sensor6_options.return_to_zero ^= 1;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Magellan Control: return_to_zero = %d.\n",
		aux->sensor6_options.return_to_zero);
}

/************************************************************/
static void _MagellanToggleUseNullRegionCallback(vrInputDevice *devinfo, int value)
{
	_MagellanPrivateInfo	*aux = (_MagellanPrivateInfo *)devinfo->aux_data;

	/* take no action when the button is released */
	if (value == 0)
		return;

	if (!aux->silent)
		vrSerialWriteString(aux->fd, LongBeepMsg);

	/* toggle the field (NOTE: we only get here when button is depressed) */
	aux->usenullregion ^= 1;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Magellan Control: usenullregion = %d.\n", aux->usenullregion);
}

/************************************************************/
static void _MagellanLongBeepCallback(vrInputDevice *devinfo, int value)
{
	_MagellanPrivateInfo	*aux = (_MagellanPrivateInfo *)devinfo->aux_data;

	if (!aux->silent)
		vrSerialWriteString(aux->fd, LongBeepMsg);
}



	/************************************************************/
	/*   Callbacks for interfacing with the Magellan device.    */
	/*                                                          */


/************************************************************/
static void _MagellanCreateFunction(vrInputDevice *devinfo)
{
	/*** List of possible inputs ***/
static	vrInputFunction	_MagellanInputs[] = {
				{ "button", VRINPUT_2WAY, _MagellanButtonInput },
				{ "mag", VRINPUT_VALUATOR, _MagellanMagInput },
				{ "6dof", VRINPUT_VALUATOR, _MagellanMagInput },
				{ "6-dof", VRINPUT_VALUATOR, _MagellanMagInput },
				{ "6dof", VRINPUT_6SENSOR, _Magellan6DofInput },
				{ "6-dof", VRINPUT_6SENSOR, _Magellan6DofInput },
				{ "sim6", VRINPUT_6SENSOR, _Magellan6DofInput },	/* "sim6" will better match other inputs that simulate a 6-sensor from other inputs */
				{ NULL, VRINPUT_UNKNOWN, NULL } };
	/*** List of control functions ***/
static	vrControlFunc	_MagellanControlList[] = {
				/* overall system controls */
				{ "system_pause_toggle", _MagellanSystemPauseToggleCallback },

				/* informational output controls */
				{ "print_context", _MagellanPrintContextStructCallback },
				{ "print_config", _MagellanPrintConfigStructCallback },
				{ "print_input", _MagellanPrintInputStructCallback },
				{ "print_struct", _MagellanPrintStructCallback },
				{ "print_help", _MagellanPrintHelpCallback },

				/* simulated 6-sensor selection controls */
				{ "sensor_next", _MagellanSensorNextCallback },
				{ "setsensor", _MagellanSensorSetCallback },	/* NOTE: this is non-boolean */
				{ "setsensor(0)", _MagellanSensorSet0Callback },
				{ "setsensor(1)", _MagellanSensorSet1Callback },
				{ "setsensor(2)", _MagellanSensorSet2Callback },
				{ "setsensor(3)", _MagellanSensorSet3Callback },
				{ "setsensor(4)", _MagellanSensorSet4Callback },
				{ "setsensor(5)", _MagellanSensorSet5Callback },
				{ "setsensor(6)", _MagellanSensorSet6Callback },
				{ "setsensor(7)", _MagellanSensorSet7Callback },
				{ "setsensor(8)", _MagellanSensorSet8Callback },
				{ "setsensor(9)", _MagellanSensorSet9Callback },
				{ "sensor_reset", _MagellanSensorResetCallback },
				{ "sensor_resetall", _MagellanSensorResetAllCallback },

				/* simulated 6-sensor manipulation controls */
				{ "temp_valuator", _MagellanTempValuatorOverrideCallback },
				{ "toggle_valuator", _MagellanToggleValuatorOverrideCallback },
				{ "temp_valuator_only", _MagellanTempValuatorOnlyCallback },
				{ "toggle_relative", _MagellanToggleRelativeAxesCallback },
				{ "toggle_space_limit", _MagellanToggleRestrictSpaceCallback },
				{ "toggle_return_to_zero", _MagellanToggleReturnToZeroCallback },

				/* other controls */
				{ "toggle_use_null_region", _MagellanToggleUseNullRegionCallback },
				{ "beep", _MagellanLongBeepCallback },

				/* end of the list */
				{ NULL, NULL } };

	_MagellanPrivateInfo	*aux = NULL;
	int			count;


#if 0 /* I don't think we want to do this on a per device basis -- s/b per process */
	signal(SIGPIPE, _MagellanSigpipeHandler);
#endif

	/******************************************/
	/* allocate and initialize auxiliary data */
	devinfo->aux_data = (void *)vrShmemAlloc0(sizeof(_MagellanPrivateInfo));
	aux = (_MagellanPrivateInfo *)devinfo->aux_data;
	_MagellanInitializeStruct(aux);

	/******************/
	/* handle options */
	aux->port = vrShmemStrDup("/dev/input/magellan");/* default, if no port given */
	aux->baud_int = 9600;				 /* default, if no baud given */
	aux->baud_enum = vrSerialBaudIntToEnum(aux->baud_int);
	_MagellanParseArgs(aux, devinfo->args);

	/***************************************/
	/* create the inputs and self-controls */
	vrInputCreateDataContainers(devinfo, _MagellanInputs);
	vrInputCreateSelfControlContainers(devinfo, _MagellanInputs, _MagellanControlList);
	/* TODO: anything to do for NON self-controls?  Perhaps implement here first. */

	/* set the active flag for the active sensor */
	if (aux->sensor6_inputs[aux->active_sim6sensor] != NULL) {
		vrAssign6sensorActiveValue(((vr6sensor *)aux->sensor6_inputs[aux->active_sim6sensor]), 1);
	}

	if (vrDbgDo(INPUT_DBGLVL))
		for (count = 0; count < MAX_BUTTONS; count++)
			vrPrintf(BOLD_TEXT "button[%d] operation = %#p\n" NORM_TEXT, count, aux->button_inputs[count]);

	vrDbgPrintf("Done creating Magellan inputs for '%s'\n", devinfo->name);
	devinfo->created = 1;

	return;
}


/************************************************************/
static void _MagellanOpenFunction(vrInputDevice *devinfo)
{
	_MagellanPrivateInfo	*aux = (_MagellanPrivateInfo *)devinfo->aux_data;

	vrTrace("_MagellanOpenFunction", devinfo->name);

	/*******************/
	/* open the device */
	vrDbgPrintfN(MAG_DBGLVL, "_MagellanOpenFunction(): Magellan port is \"%s\"\n", aux->port);
	aux->fd = vrSerialOpen(aux->port, aux->baud_enum);
	if (aux->fd < 0) {
		aux->open = 0;
		vrErrPrintf("(%s::_MagellanOpenFunction::%d) error: "
			"couldn't open serial port %s for %s\n",
			__FILE__, __LINE__, aux->port, devinfo->name);
		perror("_MagellanOpenFunction():");
		sprintf(aux->version, "- unconnected magellan -");
	} else {
		aux->open = 1;
		_MagellanInitializeDevice(aux);

		/* NOTE: for the Magellan, it is assumed that if we successfully open */
		/*   a connection that we will successfully initialize the device.    */
	}
	devinfo->version = aux->version;

	vrDbgPrintf("_MagellanOpenFunction(): Done opening Magellan for input device '%s'\n", devinfo->name);
	devinfo->operating = aux->open;

	return;
}


/************************************************************/
static void _MagellanCloseFunction(vrInputDevice *devinfo)
{
	_MagellanPrivateInfo	*aux = (_MagellanPrivateInfo *)devinfo->aux_data;

	if (aux != NULL) {
		vrSerialClose(aux->fd);
		vrShmemFree(aux);	/* aka devinfo->aux_data */
	}

	return;
}


/************************************************************/
static void _MagellanResetFunction(vrInputDevice *devinfo)
{
	/* TODO: reset code */
	return;
}


/************************************************************/
static void _MagellanPollFunction(vrInputDevice *devinfo)
{
	if (devinfo->operating) {
		_MagellanGetData(devinfo);
	} else {
		/* TODO: try to open the device again */
	}

	return;
}



	/******************************/
	/*** FreeVR public routines ***/
	/******************************/


/**********************************************************/
void vrMagellanInitInfo(vrInputDevice *devinfo)
{
	devinfo->version = (char *)vrShmemStrDup("-get from Magellan-");
	devinfo->Create = vrCallbackCreateNamed("Magellan:Create-Def", _MagellanCreateFunction, 1, devinfo);
	devinfo->Open = vrCallbackCreateNamed("Magellan:Open-Def", _MagellanOpenFunction, 1, devinfo);
	devinfo->Close = vrCallbackCreateNamed("Magellan:Close-Def", _MagellanCloseFunction, 1, devinfo);
	devinfo->Reset = vrCallbackCreateNamed("Magellan:Reset-Def", _MagellanResetFunction, 1, devinfo);
	devinfo->PollData = vrCallbackCreateNamed("Magellan:PollData-Def", _MagellanPollFunction, 1, devinfo);
	devinfo->PrintAux = vrCallbackCreateNamed("Magellan:PrintAux-Def", _MagellanPrintStruct, 0);

	vrDbgPrintfN(MAG_DBGLVL, "vrMagellanInitInfo: callbacks created.\n");
}


#endif /* } FREEVR */



	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/



#if defined(TEST_APP) /* { */

/**********************************************************************/
/*** Main function of an application to TEST the Magellan interface ***/
/**********************************************************************/
/* A test program to communicate with a magellan and print the results. */
main(int argc, char *argv[])
{
	char			msg_type;
	_MagellanPrivateInfo	*aux;
	int			baud;


	/******************************/
	/* setup the device structure */
	aux = (_MagellanPrivateInfo *)malloc(sizeof(_MagellanPrivateInfo));
	memset(aux, 0, sizeof(_MagellanPrivateInfo));
	_MagellanInitializeStruct(aux);


	/*********************************************************/
	/* set default parameters based on environment variables */
	aux->port = getenv("MAGELLAN_TTY");
	if (aux->port == NULL)
		aux->port = "/dev/input/magellan";	/* default, if no envvar */
	aux->baud_int = 9600;				/* default, if no baud given (and none can) */
	aux->baud_enum = vrSerialBaudIntToEnum(aux->baud_int);

#if 0 /* use enumerated value for baud */
	baud = aux->baud_int;
#else
	baud = aux->baud_enum;		/* use enumerated value for baud */
#endif


	/*****************************************************/
	/* adjust parameters based on command-line-arguments */
	/* TODO: parse CLAs for non-default serial port, baud, etc. */


	/**************************************************/
	/* open the serial port and initialize the device */
	aux->fd = vrSerialOpen(aux->port, baud);
	if (aux->fd < 0) {
		fprintf(stderr, "error: couldn't open serial port %s at %d baud.\n", aux->port, baud);
		exit(1);
	}

	_MagellanInitializeDevice(aux);
	printf("Magellan open.\n");

	if (getenv("MAG_DEBUG"))
		printf("\n");
	printf("Magellan: Device now initialized.\n");
	printf("Magellan version =%53s\n", aux->version);

#if 0 /* this doesn't seem to do much */
	/* put Magellan into polling mode. */
	vrSerialWriteString(aux->fd, DataQuestion);
#endif

#if 0
	/* I don't know what I thought this would do, but it */
	/*   totally hangs the program, though without this  */
	/*   it works fine (caveat on my SGI, I'm sure I added */
	/*   it when doing linux testing, but it's a bad thing.*/
	vrSerialWriteString(aux->fd, VersionMsg);
#endif
	/**********************/
	/* display the output */
	while (aux->buttons != (0x0001 | 0x0080)) {
		msg_type = _MagellanReadInput(aux);
		switch(msg_type) {
		case 'd' :
		case 'k' :
			printf("%02X %c  x=%6d y=%6d z=%6d a=%6d b=%6d c=%6d  %c%c%c%c%c%c%c%c%c",
				msg_type, msg_type,
				aux->offset[VR_TX], aux->offset[VR_TY], aux->offset[VR_TZ],
				aux->offset[VR_RX], aux->offset[VR_RY], aux->offset[VR_RZ],
				aux->buttons & 0x0001 ? '1':'-',
				aux->buttons & 0x0002 ? '2':'-',
				aux->buttons & 0x0004 ? '3':'-',
				aux->buttons & 0x0008 ? '4':'-',
				aux->buttons & 0x0010 ? '5':'-',
				aux->buttons & 0x0020 ? '6':'-',
				aux->buttons & 0x0040 ? '7':'-',
				aux->buttons & 0x0080 ? '8':'-',
				aux->buttons & 0x0100 ? '*':'-');
			if (getenv("SHOW_BAD_PACKETS"))
				printf("  DKO:%3d,%3d,%4d", aux->bad_Dpackets, aux->bad_Kpackets, aux->bad_other_packets);
			printf("%c%s", (getenv("MAG_DEBUG") ? '\n' : '\r'), "");
			fflush(stdout);
			break;
		case 'm' :
			printf("%02X %c  mode = %d                                                        \n", msg_type, msg_type, aux->mode);
			break;
		case 'n' :
			printf("%02X %c  null radius = %2d                                                 \n", msg_type, msg_type, aux->null_radius);
			break;
		case 'z' :
			printf("%02X %c  zeroed                                                          \n", msg_type, msg_type);
			break;
		case 'b' :
			printf("%02X %c  beep                                                           \n", msg_type, msg_type);
			break;
		case 'q' :
			printf("%02X %c  quality: trans gain = %d  rot gain = %d                           \n", msg_type, msg_type, aux->trans_gain, aux->rotate_gain);
			break;
		case 'p' :
			printf("%02X %c  datarate = %d %d                                                 \n", msg_type, msg_type, aux->datarate1, aux->datarate2);
			break;
		case 'v' :
			printf("%02X %c  version =%53s\n", msg_type, msg_type, aux->version);
			break;
		case '\0' :
#if 0
			/* don't do this for non-blocking read */
			printf("%02X %c  NULL response                                                  \n", msg_type, msg_type);
#endif
			break;
		default:
			printf("%02X %c  unknown response                                               \n", msg_type, msg_type);
		}
	}


	/*****************/
	/* close up shop */
	if (aux != NULL) {
		vrSerialClose(aux->fd);
		free(aux);
	}
	printf("\nMagellan closed\n");
}

#endif /* } TEST_APP */



	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/



#if defined(CAVE) /* { */

/*****************************************************************************/
/*** public functions for accessing the magellan space mouse as a          ***/
/***   tracking device:                                                    ***/
/***     void CAVEInitMagellanTracking(CAVE_ST *cave)                      ***/
/***     void CAVEResetMagellanTracking(CAVE_ST *cave)                     ***/
/***     int CAVEGetMagellanTracking(CAVE_ST *cave, CAVE_SENSOR_ST *sensor)***/
/***     -------                                                           ***/
/***     void CAVEReadMagellanController(CAVE_ST *, CAVE_CONTROLLER_ST *)  ***/
/***     int CAVENumMagellanButtons(CAVE_ST *cave)                         ***/
/***     int CAVENumMagellanValuators(CAVE_ST *cave)                       ***/
/*****************************************************************************/

/*** routines from other CAVE files ***/
void	CAVEPreRotMatrix(Matrix mat,float angle,char axis);

/*** routines from this file ***/
void	CAVEMagellanToCaveSensor(_MagellanPrivateInfo *mag);
void	CAVESetMagellanParameter(int parameter, float setting);


/*** local variables ***/
static	_MagellanPrivateInfo	mag_data[1];
static  _MagellanPrivateInfo	*cur_mag = &mag_data[0];


/*****************************************************************/
/* void CAVEInitMagellanTracking(CAVE_ST *cave)                  */
/*    Initializes the Magellan SpaceMouse as a tracking device   */
/* for the CAVE library.                                         */
/*                                                               */
/* The number of trackers specified is (will be) based on the    */
/* sensor count.                                                 */
/*                                                               */
/* CAVE structure variables accessed:                            */
/*                                                               */
/*    config->TrackerSerialPort[0]                               */
/*    config->TrackerBaudRate                                    */
/*  to do:                                                       */
/*    config->SensorActive[*]                                    */
/*    config->TrackerSerialPort[1]                               */
/*                                                               */
/*****************************************************************/
void CAVEInitMagellanTracking(CAVE_ST *cave)
{
	_MagellanPrivateInfo *mag;

	cur_mag = &mag_data[0];		/* Assumes there's just 1 Magellan! */
	mag = cur_mag;

	_MagellanInitializeStruct(mag);
	if (!cave->config->TrackerSerialPort[0]) {
		cave->config->TrackerSerialPort[0] = "/dev/input/magellan";
		fprintf(stderr, "CAVE WARNING (CAVEInitMagellanTracking): "
			"Tracker serial port not specified"
			"trying %s.\n", cave->config->TrackerSerialPort[0]);
	}

	if (cave->config->TrackerBaudRate != vrSerialBaudIntToEnum(9600)) {
		fprintf(stderr, "CAVE WARNING (CAVEInitMagellanTracking): "
			"Some Magellans default to 9600 Baud.  TrackerBaud code %d, not %d\n", cave->config->TrackerBaudRate, vrSerialBaudIntToEnum(9600));
		cave->config->TrackerBaudRate = vrSerialBaudIntToEnum(9600);
	}

	mag->fd = vrSerialOpen(cave->config->TrackerSerialPort[0],
		cave->config->TrackerBaudRate);

	if (mag->fd < 0) {
		fprintf(stderr, "CAVE ERROR (CAVEInitMagellanTracking): "
			"failed to open the serial port\n");
		exit(-1);
	}

	_MagellanInitializeDevice(mag);
	if (cave->config->SixDOFInitMode >= 0)
		CAVESetMagellanParameter(SIXDOF_USE, cave->config->SixDOFInitMode);
	CAVESetMagellanParameter(SIXDOF_TGAIN, cave->config->SixDOFInitTransSensitivity);
	CAVESetMagellanParameter(SIXDOF_RGAIN, cave->config->SixDOFInitRotSensitivity);
	CAVESetMagellanParameter(SIXDOF_VGAIN, cave->config->SixDOFInitValSensitivity);

	fprintf(stderr, "CAVE tracker (magellan):%s\n", mag->version);

#if 0 /* this doesn't seem to do much */
	/* put Magellan into polling mode. */
	vrSerialWriteString(mag->fd, DataQuestion);
#endif
}


/*******************************************************************/
void CAVEResetMagellanTracking(CAVE_ST *cave)
{
	_MagellanPrivateInfo *mag = cur_mag;

	_MagellanInitializeStruct(mag);

	vrSleep(20000);
}


/*******************************************************************/
void CAVESetMagellanParameter(int parameter, float setting)
{
	int			int_setting = setting;
	_MagellanPrivateInfo	*mag = cur_mag;

	if (!mag)
		return;

	switch (parameter) {
	case SIXDOF_MODE:
		fprintf(stderr, "CAVESetMagellanParameter(): can't set the mode on the Magellan by software.\n");
		break;

	case SIXDOF_USE:
		/* interpret the usage settings */
		mag->valuator_override = ((int_setting & SIXDOF_UVAL) ? 1 : 0);
#if 0
		mag->mixed_valuator_sensor = ((int_setting & SIXDOF_UMIXED) ? 1 : 0);
#endif
		if (int_setting & SIXDOF_UHEAD)
			mag->active_sim6sensor = &mag->head;
		if (int_setting & SIXDOF_UWAND)
			mag->active_sim6sensor = &mag->wand1;
		if (int_setting & SIXDOF_UWAND2)
			mag->active_sim6sensor = &mag->wand2;
		mag->restrict_space = ((int_setting & SIXDOF_UCLAMP) ? 1 : 0);
		mag->relative_axis = ((int_setting & SIXDOF_URELATIVE) ? 1 : 0);
		mag->return_to_zero = ((int_setting & SIXDOF_UREZERO) ? 1 : 0);
		break;

	case SIXDOF_NULL:
		/* note: these are currently not used */
		mag->null_offset[VR_TX] = setting;
		mag->null_offset[VR_TY] = setting;
		mag->null_offset[VR_TZ] = setting;
		mag->null_offset[VR_RX] = setting;
		mag->null_offset[VR_RY] = setting;
		mag->null_offset[VR_RZ] = setting;
		break;

	case SIXDOF_RGAIN:
		mag->scale_rot = ROT_SENSITIVITY * setting;
		break;

	case SIXDOF_TGAIN:
		mag->scale_trans = TRANS_SENSITIVITY * setting;
		break;

	case SIXDOF_VGAIN:
		mag->scale_valuator = VALUATOR_SENSITIVITY * setting;
		break;

	case SIXDOF_JOYX:
	case SIXDOF_JOYY:
	case SIXDOF_TX:
	case SIXDOF_TY:
	case SIXDOF_TZ:
	case SIXDOF_RX:
	case SIXDOF_RY:
	case SIXDOF_RZ:
	case SIXDOF_TYPE:
	case SIXDOF_VERS:
	case SIXDOF_KYBD:
		fprintf(stderr, "CAVESetMagellanParameter(): this parameter is not modifiable.\n");
		break;

	default:
		fprintf(stderr, "CAVESetMagellanParameter(): No such parameter (%d).\n",parameter);
		break;
	}
}


/*******************************************************************/
/* Some possible options (modes):                                  */
/*    Choice of which sensor is active (default to first wand (1)  */
/*    Movement restricted to remain inside the CAVE (or not)       */
/*    Rotations relative to the sensor's axes, or the viewer's     */
/*                                                                 */
/* Current thoughts on button commands:                            */
/* :) 1 - controller: cave button 1                                */
/* :) 2 - controller: cave button 2                                */
/* :) 3 - controller: cave button 3                                */
/* :) 4 - toggle cave space restriction                            */
/* :) 5 - control head                                             */
/* :) 6 - control wand1                                            */
/*    7 - control wand2 (for now, toggle between abs/rel rotations)*/
/* :) 8 - controller: joystick                                     */
/*******************************************************************/
int CAVEGetMagellanTracking(CAVE_ST *cave, CAVE_SENSOR_ST *sensor)
{
#define RTOD(d) ((d)*57.295788f)

	char			msg_type;
	_MagellanPrivateInfo	*mag;

	mag = cur_mag;

	do {
		/* Loop: consume all data received so far in one gulp */

		msg_type = _MagellanReadInput(mag);

		switch(msg_type) {
		/***********************************************/
		case 'd':
			CAVEMagellanToCaveSensor(mag);
			break;

		/***********************************************/
		case 'k':
			if (getenv("MAG_DEBUG")) {
				fprintf(stderr, "Magellan: Got a keypress -- 0x%0x\n", mag->buttons);
			}

			if (mag->buttons & 0x0100)
				/* "*" keypress - do nothing (allow meta commands) */
				break;

			/* "1", "2", "3" keypresses -> buttons 1, 2, 3 */
			mag->controls.button[0] = (mag->buttons & 0x0001);
			mag->controls.button[1] = (mag->buttons & 0x0002);
			mag->controls.button[2] = (mag->buttons & 0x0004);
			if (mag->buttons & 0x0008) {
				/* "4" keypress - toggle movement restriction */
				mag->restrict_space ^= 1;
			}
			if ((mag->buttons & 0x0030) == 0x0030) {
				/* "5" + "6" -- reset head and wand */
				mag->head = initial_sensor;
				mag->wand1 = initial_sensor;
#if 0
				mag->wand2 = initial_sensor;
#endif
			}
			if (mag->buttons & 0x0010) {
				/* "5" keypress - control movement of head */
				mag->active_sim6sensor = &mag->head;
				mag->valuator_override = 0;
			}
			if (mag->buttons & 0x0020) {
				/* "6" keypress - control movement of wand1 */
				mag->active_sim6sensor = &mag->wand1;
				mag->valuator_override = 0;
			}
			if (mag->buttons & 0x0040) {
				/* "7" keypress - (for now) toggle rel/abs rotation */
				mag->relative_axis ^= 1;
			}
			if (mag->buttons & 0x0080) {
				/* "8" keypress - toggle valuator_override (ie. magellan is joystick) */
				mag->valuator_override ^= 1;
			}
			break;
		}

#if 0
		printf("loc = (%f,%f,%f)\n", mag->active_sim6sensor->x, mag->active_sim6sensor->y, mag->active_sim6sensor->z);
#endif

	} while (mag->lo_buflen > 0 && msg_type != '\0');


	sensor[0] = mag->head;
	sensor[1] = mag->wand1;
	sensor[2] = mag->wand2;
	return 1;
}


	/*--------------------------------------*/
	/*       non public CAVE Routines       */
	/*--------------------------------------*/

/*******************************************************************/
void CAVEMagellanToCaveSensor(_MagellanPrivateInfo *mag)
{
	float		vector[3];
	Matrix		mat = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
	CAVE_SENSOR_ST	sensor_delta;

	sensor_delta.x = mag->offset[VR_TX] * mag->scale_trans;
	sensor_delta.y = mag->offset[VR_TY] * mag->scale_trans;
	sensor_delta.z = mag->offset[VR_TZ] * mag->scale_trans;

	sensor_delta.azim = mag->offset[VR_RY] * mag->scale_rot;
	sensor_delta.elev = mag->offset[VR_RX] * mag->scale_rot;
	sensor_delta.roll = mag->offset[VR_RZ] * mag->scale_rot;

	CAVEGetTimestamp(&sensor_delta.timestamp);

	mag->active_sim6sensor->timestamp = sensor_delta.timestamp;

	if (mag->return_to_zero) {
		*mag->active_sim6sensor = initial_sensor;
		if (mag->active_sim6sensor == &(mag->wand1)) {
			mag->active_sim6sensor->y -= 1.0;
			mag->active_sim6sensor->z -= 1.5;
		}
	}

	/***************************************************/
	/*** handle the sensor translation (or valuator) ***/
	/***************************************************/
	if (mag->valuator_override) {
		/* make magellan translation do valuator controls */
		mag->controls.valuator[SIXDOF_JOYX] =  mag->offset[VR_TX] * mag->scale_valuator;
		mag->controls.valuator[SIXDOF_JOYY] = -mag->offset[VR_TZ] * mag->scale_valuator;
		mag->controls.valuator[SIXDOF_TX] = mag->offset[VR_TX] * mag->scale_valuator;
		mag->controls.valuator[SIXDOF_TY] = mag->offset[VR_TY] * mag->scale_valuator;
		mag->controls.valuator[SIXDOF_TZ] = mag->offset[VR_TZ] * mag->scale_valuator;
		mag->controls.valuator[SIXDOF_RX] = mag->offset[VR_RX] * mag->scale_valuator;
		mag->controls.valuator[SIXDOF_RY] = mag->offset[VR_RY] * mag->scale_valuator;
		mag->controls.valuator[SIXDOF_RZ] = mag->offset[VR_RZ] * mag->scale_valuator;
	} else {

		/* Not overridden.  Translate and rotate the active sensor */
		if (mag->relative_axis) {
			float cos_azim = cosf(DTOR(mag->head.azim));
			float sin_azim = sinf(DTOR(mag->head.azim));
			float cos_elev = cosf(DTOR(mag->head.elev));
			float sin_elev = sinf(DTOR(mag->head.elev));
			float cos_roll = cosf(DTOR(mag->head.roll));
			float sin_roll = sinf(DTOR(mag->head.roll));

			/* translate relative to current orientation.     */
			/* There is a choice of which orientation to move */
			/* in relation to: the manipulated sensor, or the */
			/* viewing sensor.                                */

			/**********************************************/
			/* move relative to viewing (ie. head) sensor */

			/* This mimics the non ZUP_COORD version of CAVEGetSensorVector..*/
			/*   (&mag->head,CAVE_RIGHT,vector), because the ZUP_COORDS stuff*/
			/*   in that is inappropriate for this circumstance.             */

			/* vector = head->CAVE_RIGHT vector (correct for Z-UP or Z-OUT) */
			vector[0] =  cos_azim * cos_roll + sin_azim * sin_elev * sin_roll;
			vector[1] =  cos_elev * sin_roll;
			vector[2] = -sin_azim * cos_roll + cos_azim * sin_elev * sin_roll;
			mag->active_sim6sensor->x += sensor_delta.x * vector[0];
			mag->active_sim6sensor->y += sensor_delta.x * vector[1];
			mag->active_sim6sensor->z += sensor_delta.x * vector[2];

			/* vector = head->CAVE_UP vector (correct for Z-UP or Z-OUT) */
			vector[0] = -cos_azim * sin_roll + sin_azim * sin_elev * cos_roll;
			vector[1] =  cos_roll * cos_elev;
			vector[2] =  sin_azim * sin_roll + cos_azim * sin_elev * cos_roll;
			mag->active_sim6sensor->x += sensor_delta.y * vector[0];
			mag->active_sim6sensor->y += sensor_delta.y * vector[1];
			mag->active_sim6sensor->z += sensor_delta.y * vector[2];

			/* vector = head->CAVE_BACK vector (correct for Z-UP or Z-OUT) */
			vector[0] =  cos_elev * sin_azim;
			vector[1] = -sin_elev;
			vector[2] =  cos_elev * cos_azim;
			mag->active_sim6sensor->x += sensor_delta.z * vector[0];
			mag->active_sim6sensor->y += sensor_delta.z * vector[1];
			mag->active_sim6sensor->z += sensor_delta.z * vector[2];
		} else {
			/* Not relative_axis; do translations in CAVE space */
			mag->active_sim6sensor->x += sensor_delta.x;
			mag->active_sim6sensor->y += sensor_delta.y;
			mag->active_sim6sensor->z += sensor_delta.z;
		}

		if (mag->restrict_space) {
			/* actually, the exact values of restriction should */
			/*   come from the specific configuration.          */
			if (mag->active_sim6sensor->x >  5.0) mag->active_sim6sensor->x =  5.0;
			if (mag->active_sim6sensor->y > 10.0) mag->active_sim6sensor->y = 10.0;
			if (mag->active_sim6sensor->z >  5.0) mag->active_sim6sensor->z =  5.0;
			if (mag->active_sim6sensor->x < -5.0) mag->active_sim6sensor->x = -5.0;
			if (mag->active_sim6sensor->y <  0.0) mag->active_sim6sensor->y =  0.0;
			if (mag->active_sim6sensor->z < -5.0) mag->active_sim6sensor->z = -5.0;
		}

		/**********************************/
		/*** handle the sensor rotation ***/
		/**********************************/
		if (mag->relative_axis) {
			float	azim, elev, roll;
			float	azim_s, azim_c;

#if 1
			/* This seems to work well for the head display,  */
			/* but I'm not sure if it will under ZUP_coords.  */
			/* However, it's not the greatest interface for   */
			/* controlling wand rotation -- the rotation      */
			/* should probably be relative to the head_sensor */
			/* as with the wand translation.                  */

			CAVEPreRotMatrix(mat, mag->active_sim6sensor->azim,'y');
			CAVEPreRotMatrix(mat, mag->active_sim6sensor->elev,'x');
			CAVEPreRotMatrix(mat, mag->active_sim6sensor->roll,'z');
			CAVEPreRotMatrix(mat, sensor_delta.azim,'y');
			CAVEPreRotMatrix(mat, sensor_delta.elev,'x');
			CAVEPreRotMatrix(mat, sensor_delta.roll,'z');

			/*** This section of code calculates the proper Euler angles from ***/
			/***   the 4x4 matrix using techniques from "Robotics," Fu et.al. ***/
			/***   Actually, the matrix order seems backward, and I lucked    ***/
			/***   into getting them backward - figure it out later, it works!***/
			azim =  atan2(mat[2][0], mat[2][2]);
			azim_s = sin(azim);
			azim_c = cos(azim);
			elev =  atan2(mat[2][1], azim_s*mat[2][0] + azim_c*mat[2][2]);
			roll =  atan2(azim_s*mat[1][2] - azim_c*mat[1][0], azim_c*mat[0][0] - azim_s*mat[0][2]);

			mag->active_sim6sensor->azim =  RTOD(azim);
			mag->active_sim6sensor->elev = -RTOD(elev);
			mag->active_sim6sensor->roll =  RTOD(roll);
#else
			/* This one is close, but confuses roll & elev after an azim */
			/* twist.  I reversed the matrix indices, in an attempt to   */
			/* match the formulae from "Robotics."                       */
			CAVEPreRotMatrix(mat,-mag->active_sim6sensor->roll,'z');
			CAVEPreRotMatrix(mat,-mag->active_sim6sensor->elev,'x');
			CAVEPreRotMatrix(mat,-mag->active_sim6sensor->azim,'y');
			CAVEPreRotMatrix(mat,-sensor_delta.roll,'z');
			CAVEPreRotMatrix(mat,-sensor_delta.elev,'x');
			CAVEPreRotMatrix(mat,-sensor_delta.azim,'y');

			/*** This section of code calculates the proper Euler angles from ***/
			/***   the 4x4 matrix using techniques from Robotics, Fu et.al.   ***/
			azim =  atan2(mat[0][2], mat[2][2]);
			azim_s = sin(azim);
			azim_c = cos(azim);
			elev =  atan2(mat[1][2], azim_s*mat[0][2] + azim_c*mat[2][2]);
			roll =  atan2(azim_s*mat[2][1] - azim_c*mat[0][1], azim_c*mat[0][0] - azim_s*mat[2][0]);

			mag->active_sim6sensor->azim =  RTOD(azim);
			mag->active_sim6sensor->elev = -RTOD(elev);
			mag->active_sim6sensor->roll =  RTOD(roll);
#endif

		} else {
			/* do rotations around the sensor's axes */
			mag->active_sim6sensor->azim += sensor_delta.azim;
			mag->active_sim6sensor->elev += sensor_delta.elev;
			mag->active_sim6sensor->roll += sensor_delta.roll;

		}
	}
}


/*******************************************************************/
void CAVEReadMagellanController(CAVE_ST *cave,volatile CAVE_CONTROLLER_ST *control)
{
	_MagellanPrivateInfo *mag;

	mag = cur_mag;		/* Well, where else do we get it from? */

	if (mag == NULL || mag->fd < 0) {
		fprintf(stderr, "CAVE tracker: Magellan not initialized yet.\n");
		return;
	}

	control->button[0] = mag->controls.button[0];
	control->button[1] = mag->controls.button[1];
	control->button[2] = mag->controls.button[2];

	control->valuator[SIXDOF_JOYX] = mag->controls.valuator[SIXDOF_JOYX]; /* x transl */
	control->valuator[SIXDOF_JOYY] = mag->controls.valuator[SIXDOF_JOYY]; /* y transl */
	control->valuator[SIXDOF_TX] = mag->controls.valuator[SIXDOF_TX]; /* x transl */
	control->valuator[SIXDOF_TY] = mag->controls.valuator[SIXDOF_TY]; /* y transl */
	control->valuator[SIXDOF_TZ] = mag->controls.valuator[SIXDOF_TZ]; /* z transl */
	control->valuator[SIXDOF_RX] = mag->controls.valuator[SIXDOF_RX]; /* elev [xrot] */
	control->valuator[SIXDOF_RY] = mag->controls.valuator[SIXDOF_RY]; /* azim [yrot] */
	control->valuator[SIXDOF_RZ] = mag->controls.valuator[SIXDOF_RZ]; /* roll [zrot] */

	control->valuator[SIXDOF_TYPE] = SIXDOF_MAG;
	control->valuator[SIXDOF_VERS] = MAG_CURVERSION;

	control->valuator[SIXDOF_KYBD] = mag->buttons;	/* bit-encoded keyboard */
	control->valuator[SIXDOF_MODE] = mag->mode;	/* bit-encoded enables (rot, trans, dominant) */
	control->valuator[SIXDOF_USE] =
	  ((mag->valuator_override) ? SIXDOF_UVAL : 0) |		/* 0x01 mag->valuator_override [8key]*/
#if 0
	  ((mag->mixed_valuator_sensor) ? SIXDOF_UMIXED : 0) |		/* 0x02 mag->mixed_valuator_sensor*/
#endif
	  ((mag->active_sim6sensor == &mag->head) ? SIXDOF_UHEAD : 0) |	/* 0x04 mag->head [5key] */
	  ((mag->active_sim6sensor == &mag->wand1) ? SIXDOF_UWAND : 0) |	/* 0x08 mag->wand [6key] */
	  ((mag->active_sim6sensor == &mag->wand2) ? SIXDOF_UWAND2 : 0) |	/* 0x10 mag->wand        */
	  ((mag->restrict_space) ? SIXDOF_UCLAMP : 0) |			/* 0x20 rangerestrict [4key]*/
	  ((mag->relative_axis) ? SIXDOF_URELATIVE : 0);		/* 0x40 head/wand relative [7key]*/
	  ((mag->return_to_zero) ? SIXDOF_UREZERO : 0);			/* 0x80 head/wand relative [7key]*/

	control->valuator[SIXDOF_NULL] = mag->null_radius;
	control->valuator[SIXDOF_RGAIN] = mag->rotate_gain;
	control->valuator[SIXDOF_TGAIN] = mag->trans_gain;
	control->valuator[SIXDOF_VGAIN] = 0.0;	/* not part of a magellan */
}


/*******************************************************************/
int CAVENumMagellanButtons(CAVE_ST *cave)
{
	return 3;
}

/*******************************************************************/
int CAVENumMagellanValuators(CAVE_ST *cave)
{
	return SIXDOF_NVALUATORS;
}

#endif /* } CAVE */

