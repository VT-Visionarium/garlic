/* ======================================================================
 *
 *  CCCCC          vr_input.spacetec.c
 * CC   CC         Author(s): John Stone, Bill Sherman
 * CC              Created: September 30, 1999 -- adapted from cave.spaceorb.c
 * CC   CC         Last Modified: June 7, 2003
 *  CCCCC
 *
 * Code file for FreeVR inputs from SpaceTec[tm] 6-DOF input devices.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/*************************************************************************

USAGE:
	There are a couple of ways to use the SpaceTec devices with the FreeVR
	library.  One is to use it like the Magellan & Spaceball as a complete
	tracker/controller input device.  The other is to use it only as the
	controller device (ie. just as a wand).

	Inputs are specified with the "input" option:
		input "<name>" = ...
			spaceorb:  "2switch(button[{A|B|C|D|E|F|Reset}])";
			spaceball: "2switch(button[{1|2|3|4|5|6|7|8|Pick}])";
		input "<name>" = "valuator(6dof[{-,}{tx|ty|tz|rx|ry|rz}])";
		input "<name>" = "6sensor(6dof[<sensor number>])";

	Controls are specified with the "control" option:
		control "<control option>" = "2switch(button[{A|B|C|D|E|F|Reset}])";
		control "<control option>" = "2switch(button[{1|2|3|4|5|6|7|8|Pick}])";

	Here is the available control options for FreeVR:
		"print_help" -- print info on how to use the input device
		"system_pause_toggle" -- toggle the system pause flag
		"print_context" -- print the overall FreeVR context data structure (for debugging)
		"print_config" -- print the overall FreeVR config data structure (for debugging)
		"print_input" -- print the overall FreeVR input data structure (for debugging)
		"print_struct" -- print the internal SpaceTec data structure (for debugging)

		"sensor_reset" -- reset the current 6-sensor
		"sensor_resetall" -- reset the all the 6-sensors
		"sensor_next" -- jump to the next 6-sensor on the list
		"setsensor(<num>)" -- set the simulated sensor to a particular one
		"toggle_valuator" -- toggle valuator values instead of 6-sensor
		"temp_valuator" -- temporarily disable 6-sensor for valuator values
		"toggle_valuator_only" -- toggle whether translation is saved for valuator
		"temp_valuator_only" -- temporarily use translation values for valuator
		"toggle_relative" -- toggle whether movement is relative to sensor's position
		"toggle_space_limit" -- toggle whether 6-sensor can go outside working volume
		"toggle_return_to_zero" -- toggle whether return-to-zero operation is on
		"toggle_use_null_region" -- toggle whether null-region is used

	Here are the FreeVR configuration argument options for the SpaceTec:
		"port" - serial port SpaceTec is connected to
			("/dev/input/spacetec" is the default)
		"baud" - baud rate of serial port connection (generally 9600)
			(9600 is the default)
		"orient" - choice of being held such that the orb points "up" or "away"
			("away" is the default)
		"transNull" - int value of the translational null region
		"rotNull" - int value of the rotational null region
		"useNull" - boolean choice of whether Null regions should be in effect
		"transScale" - float value of the translational sensitivity scale
		"rotScale" - float value of the rotational sensitivity scale
	 	"valScale" - float value of the valuator sensitivity scale
		"silent" - boolean choice of whether a SpaceBall gives aural
			feedback to button presses (2switch's give short beep,
			control operations give longer beep).

	 -- These options are for using the SpaceTec device as a 6sensor:
		"restrict" - boolean choice of whether sensor can leave
			working volume (no restriction is the default)
	 	"valuatoroverride" - whether the orb should act as a set of
			valuators and buttons rather than a 6sensor.
	 	"returnToZero" - whether 6sensors return to zero when orb is
			released
	 	"relativeRot" - boolean choice of whether rotations should be
			about the 6sensor's coordsys, or the world's coordsys.
		"workingVolume" - 6 numbers that describe a parallelepiped in
			which 6sensors can roam.


	----------------------------------------------------------------------

	Here is how the spaceorb was hardcoded:
			FreeVR			CAVElib
			------			-------
		'A' is 	button[0]		select the next sensor (debounced)
		'B' is 	button[1]		temporary valuator override
		'C' is 	button[2]		CAVEbutton2
		'D' is	toggle space restrict	CAVEbutton1
		'E' is	set to sensor0		CAVEbutton3
		'F' is	set to sensor1		toggle valuator override (debounced)
		'Re' is	toggle rel/abs rotation	reset the current sensor
			(none of which
			are debounced)
		N/A	toggle valuator override
		N/A	temporary valuator override
		N/A	reset the current sensor
		N/A	select the next sensor
		N/A	toggle return to zero mode
		N/A	use null region
		N/A	orient up or away
		N/A				toggle relative axis (ie. rel/abs rotation)
		N/A				toggle space restriction
		N/A				toggle return to zero
		N/A				use null region
		N/A				orient up or away

			rel/abs rotation defaults to "on"
			space restriction defaults to "off"
			valuator override defaults to "off"
			mixed valuator/sensor mode defaults to "off"
			return to zero mode defaults to "off"
			holding orientation defaults to pointing away from user ("away")

	The Spaceball (CAVElib) controls are hardcoded:
		'1' is				CAVEbutton1
		'2' is				CAVEbutton2
		'3' is				CAVEbutton3
		'4' is				select the next sensor (debounced)
		'5' is				reset the current sensor
		'6' is				toggle relative axis (ie. rel/abs rotation) (debounced)
		'7' is				toggle space restriction (debounced)
		'8' is				toggle return to zero (debounced)
		'Pick' is			tmp_valuator_override

HISTORY:
	13 January 1999 (Bill Sherman) -- combined John Stone's orb library
		files into a single cave.spaceorb.c file, added the routines
		to interface with the CAVE library, plus made several
		changes to make this file mimic the Magellan interface
		code -- although I went back to the dual valuator/sensor
		mode of tracking.

	[from the old cave.spaceball.c file prior to merger w/ vr_input.spaceorb.c]
	14 January 1999 (Bill Sherman) -- combined John Stone's sball library
		files into a single cave.spaceball.c file, added the routines
		to interface with the CAVE library, plus made several
		changes to make this file mimic the Magellan interface
		code -- although I went back to the dual valuator/sensor
		mode of tracking.

	30 September 1999 (Bill Sherman) -- created a FreeVR link to the
		existing interface code.

	4 November 1999 (Bill Sherman) -- redid the FreeVR method of defining
		inputs to the new flexible method that will allow more
		freedom in mapping inputs.

	23 November 1999 (Bill Sherman) -- revamped everything to use the
		new generic FreeVR serial code.  Plus a lot of general
		code cleanup.
		[Note that the CAVE side of things is probably broken at the
		moment, so will require some fixing the next time I go back
		to making the next enhanced version of the library.]

	29 November 1999 (Bill Sherman) -- completed the addition of the
		SpaceBall code to this file, allowing it to handle either
		Orbs or Balls -- again using John Stone's Spaceball code as
		a base, and then figuring out more of the Spaceball protocol.
		[Note, that we'll want to rename this from vr_input.spaceorb.c
		to vr_input.spacetec.c]

	29 December 1999 (Bill Sherman) -- brought the SpaceTec device code
		up to date with the new format of input device source
		files, including the new CREATE section of _SpaceorbFunction().
		And finally renamed this file to vr_input.spacetec.c.

	30 December 1999 (Bill Sherman) -- made a bunch of changes to
		the SpaceBall code to get it working with the changed
		vrSerialOpen() which now disables IXON.

	13 January 2000 (Bill Sherman) -- Reincorporated into CAVE library
		(it turns out I had made several changes to the CAVE portion
		of the CAVE library version [July 10, 1999] that weren't
		part of the combined version).
		Note that this now requires vr_serial.c, vr_serial.h & vr_debug.h
		to be compiled with the rest of the CAVE library.

	26 January 2000 (Bill Sherman) -- Integrated new self-control creation
		method.  And wrote a routine and function for the "print_struct"
		and "beep" device controls.

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
		  functions (by copying the similar code from vr_input.xwindows.c).

	20 April 2002 (Bill Sherman)
		I changed the usage of the "oob" flag for indicating the active
		  sensor to use the new "active" flag instead.

	17,20 May 2002 (Bill Sherman)
		Changed variable type names where possible (sensor6 to 6sensor,
		  etc), and renamed X,Y,Z,etc to VR_X, VR_Y, etc.

	11 September 2002 (Bill Sherman)
		Moved the control callback array into the _SpaceorbFunction()
		callback.

	21-23 April 2003 (Bill Sherman)
		Updated to new input matching function code style.  Changed
		  "opaque" field to "aux_data".  Split _SpacetecFunction()
		  into 5 functions.  Added new vrPrintStyle argument to
		  _SpacetecPrintStruct() for the sake of the new "PrintAux"
		  callback.

	3 June 2003 (Bill Sherman)
		Now include "vr_enums.h" for the TEST_APP code.
		Added the address of the auxiliary data to the printout.
		Added the "system_pause_toggle" control callback.
		Added comments classifying the controls.  Also moved some of
			the callbacks around to be in an order more consistent
			with the ControlList (and other input device files).
		Now use the "trans_scale" and "trans_rot" fields of
			vr6SensorConv instead of local copies.

	16 October 2009 (Bill Sherman)
		A quick fix to the _SpacetecParseArgs() routine to handle the
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
	- The call to _SpacetecInitiaizeDevice() is in a place that is not
		consistent with the rest of FreeVR input devices -- which
		is not hugely surprising since this input device would have
		been one of the earliest, and implemented when I hadn't
		settled on something approaching a standard.

	- The incoming data packet parsing should probably be changed to
		be more like the method used by the Magellan (and I guess
		probably the FAROarm too).  I think there are probably
		some bad packets caused by being split across reads, which
		is solvable.

	- The version string for the SpaceBall often has a couple of extra
		bytes on the end.  At one point this went away, but after
		a couple of other changes it came back.

	- DONE: Implement the "print_help" control-callback.


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
#include <ctype.h>  /* needed for isprint() etc */
#include "vr_utils.h"
#include "vr_serial.h"
#include "vr_debug.h"

#if defined (FREEVR)
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
#  include "vr_utils.c"
#  include "vr_enums.h"
#endif


#if defined(CAVE) /* { */
#  include <math.h>
#  include "cave.h"
#  include "cave.private.h"
#  include "cave.tracker.h"
#  include "cave.6dof.h"
static CAVE_SENSOR_ST initial_cave_sensor = {
		0,5,0,
		0,0,0,
		0,	/*timestamp*/
		FALSE,	/*calibrated*/
		CAVE_TRACKER_FRAME
	};
#endif /* } CAVE */


/*** local defines ***/

#define	VR_TX VR_X
#define	VR_TY VR_Y
#define	VR_TZ VR_Z
#define	VR_RX (VR_ELEV+3)
#define	VR_RY (VR_AZIM+3)
#define	VR_RZ (VR_ROLL+3)

#define	BUFSIZE 1024


	/*************************************************************/
	/*** definitions for interfacing with the SpaceTec devices ***/
	/***                                                       ***/

typedef enum {
		SPACETEC_UNKNOWN = -1,
		SPACETEC_DEFAULT = 0,
		SPACETEC_ORB,
		SPACETEC_BALL
	} SpaceDeviceType;

/* SpaceTec command sequences */
#define	BallShortBeepMsg	"Bcc\r"			/* Short Beep */
#define	BallLongBeepMsg		"Bccccc\r"		/* Long Beep */

/* SpaceOrb & SpaceBall Button bit-masks & indices */
#define ORB_BUTTON_A		 0x01
#define ORB_BUTTON_B		 0x02
#define ORB_BUTTON_C		 0x04
#define ORB_BUTTON_D		 0x08
#define ORB_BUTTON_E		 0x10
#define ORB_BUTTON_F		 0x20
#define ORB_BUTTON_RESET	 0x40

#define BALL_BUTTON_1		0x001
#define BALL_BUTTON_2		0x002
#define BALL_BUTTON_3		0x004
#define BALL_BUTTON_4		0x008
#define BALL_BUTTON_5		0x010
#define BALL_BUTTON_6		0x020
#define BALL_BUTTON_7		0x040
#define BALL_BUTTON_8		0x080
#define BALL_BUTTON_PICK	0x100

#define ORB_BUTTONINDEX_A	0x00
#define ORB_BUTTONINDEX_B	0x01
#define ORB_BUTTONINDEX_C	0x02
#define ORB_BUTTONINDEX_D	0x03
#define ORB_BUTTONINDEX_E	0x04
#define ORB_BUTTONINDEX_F	0x05
#define ORB_BUTTONINDEX_RESET	0x06

#define BALL_BUTTONINDEX_1	0x00
#define BALL_BUTTONINDEX_2	0x01
#define BALL_BUTTONINDEX_3	0x02
#define BALL_BUTTONINDEX_4	0x03
#define BALL_BUTTONINDEX_5	0x04
#define BALL_BUTTONINDEX_6	0x05
#define BALL_BUTTONINDEX_7	0x06
#define BALL_BUTTONINDEX_8	0x07
#define BALL_BUTTONINDEX_PICK	0x08


/* SpaceOrb base sensitivity values */
#define TRANS_SENSITIVITY	0.00005
#define ROT_SENSITIVITY		0.0025
#define VALUATOR_SENSITIVITY	0.002


/******************************************************************/
/*** auxiliary structure of the current data from the SpaceTec. ***/
typedef struct {
		/* these are for interfacing with the SpaceTec hardware */
		int		fd;		/* was commhandle */
		char		*port;		/* name of serial port */
		int		baud_enum;	/* communication rate as an enumerated value */
		int		baud_int;	/* communication rate as the real value */
		int		open;		/* flag with SpaceTec successfully open */

		/* these are for SpaceTec internal data parsing */
		unsigned char	buf[BUFSIZE];
		int		bufpos;
		unsigned char	packtype;
		int		packlen;
		long		bad_Dpackets;	/* count the number of bad 'D' packets */
		long		bad_Kpackets;	/* count the number of bad 'K' packets */
		long		bad_other_packets;/* count the number of other bad packets */
		SpaceDeviceType	device;		/* the type of SpaceTec device */
		int		ball_resolution;/* the number of bits for each value */
		char		version[256];	/* self-reported version of the device */
		char		op_params[256];	/* operating parameters of the device (according to it) */

		/* information about the current SpaceTec values */
		int		offset[6];	/* incoming translation & rotation offset info */
		int		buttons;	/* incoming button info */
		int		timer;		/* incoming timer info  TODO: or is it a checksum?*/
		int		button_change;	/* boolean indicator if button values have changed*/
		int		valuator_change;/* boolean indicator of change in valuator values */
#ifdef CAVE
		CAVE_SENSOR_ST	head, wand1, wand2; /* storage for multiple CAVE sensors */
		CAVE_CONTROLLER_ST controls;	/* storage for CAVE controller info */

		CAVE_SENSOR_ST	*active_sim6sensor;	/* points to {head, wand1, wand2} */

		/* these are now in a separate vr6sensorConv struct in FreeVR */
		int		restrict_space;	/* restrict movement to some display-specif region */
		int		relative_axis;	/* move relative to the current orientation? */
		int		valuator_only;	/* no 6sensor inputs are active at all? */
		int		valuator_override; /* control valuators rather than sensors translation */
#  if 1 /* 11/22/99 -- I'm pretty sure this is redundant with valuator_override */
	/* 1/13/00 -- actually, I think this was used when valuator override is on, but to  */
	/*   indicate that the rotational values of the sensor should continue to function. */
		int		mixed_valuator_sensor; /* continue to allow rotational sensor control */
#  endif
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
		int		active_sim6sensor;

		vr6sensorConv	sensor6_options;

#endif /* end library-specific fields */

		/* information about how to filter the data for use with the VR library (FreeVR or CAVE) */
		int		usenullregion;
		int		null_offset[6];

#if defined(TEST_APP) || defined(CAVE)	/* NOTE: these fields are needed by the CAVE and test versions of this input code */
		double		scale_trans;	/* scaling factor for translation */
		double		scale_rot;	/* scaling factor for rotation */
#endif
		double		scale_valuator;	/* scaling factor for use as a valuator */

		int		orient_up;	/* whether orb is held pointing up, or away */
		int		silent;		/* boolean to determine noise factor */

	} _SpacetecPrivateInfo;



	/******************************************************/
	/*** General NON public SpaceTec interface routines ***/
	/******************************************************/

/******************************************************/
/* typename is used to specify a particular device among many that  */
/*   share (more or less) the same protocol.  The typename is then  */
/*   used to determine what specific features are available with    */
/*   this particular type of device.  Possible types are "spaceorb" */
/*   and "spaceball".                                               */
static void _SpacetecInitializeStruct(_SpacetecPrivateInfo *orb, char *typename)
{
	orb->bad_Dpackets = 0;
	orb->bad_Kpackets = 0;
	orb->bad_other_packets = 0;

	if (!strcasecmp(typename, "spaceorb")) {
		orb->device = SPACETEC_ORB;
		orb->ball_resolution = 10;
		orb->silent = 1;
	} else if (!strcasecmp(typename, "spaceball")) {
		orb->device = SPACETEC_BALL;
		orb->ball_resolution = 16;
		orb->silent = 0;
	} else {
		/* default to SpaceOrb, since around here they are more prevalent */
		orb->device = SPACETEC_ORB;
		orb->ball_resolution = 10;
		orb->silent = 1;
	}

	orb->version[0] = '\0';
	orb->op_params[0] = '\0';
	orb->buf[0] = '\0';
	orb->bufpos = 0;
	orb->buttons = 0;
	orb->offset[VR_TX] = 0;
	orb->offset[VR_TY] = 0;
	orb->offset[VR_TZ] = 0;
	orb->offset[VR_RX] = 0;
	orb->offset[VR_RY] = 0;
	orb->offset[VR_RZ] = 0;

	orb->timer = 0;
	orb->button_change = 0;
	orb->valuator_change = 0;

#if defined(TEST_APP) || defined(CAVE)
	orb->scale_trans = TRANS_SENSITIVITY;
	orb->scale_rot = ROT_SENSITIVITY;
#endif
	orb->scale_valuator = VALUATOR_SENSITIVITY;
	orb->orient_up = 0;

	orb->usenullregion = 0;
	orb->null_offset[VR_TX] = 0;
	orb->null_offset[VR_TY] = 0;
	orb->null_offset[VR_TZ] = 0;
	orb->null_offset[VR_RX] = 0;
	orb->null_offset[VR_RY] = 0;
	orb->null_offset[VR_RZ] = 0;

#ifdef FREEVR /* { */
	orb->sensor6_options.azimuth_axis = VR_Y;
	orb->sensor6_options.relative_axis = 0;
	orb->sensor6_options.return_to_zero = 0;
	orb->sensor6_options.ignore_all = 0;
	orb->sensor6_options.ignore_trans = 0;
	orb->sensor6_options.restrict_space = 0;
#if !defined(TEST_APP) && !defined(CAVE)
	orb->sensor6_options.trans_scale = TRANS_SENSITIVITY;
	orb->sensor6_options.rot_scale = ROT_SENSITIVITY;
	orb->sensor6_options.swap_transrot = 0;
#endif

	/* The default working volume is that of the typical CAVE */
	orb->sensor6_options.working_volume_min[VR_X] = -5.0;
	orb->sensor6_options.working_volume_max[VR_X] =  5.0;
	orb->sensor6_options.working_volume_min[VR_Y] =  0.0;
	orb->sensor6_options.working_volume_max[VR_Y] = 10.0;
	orb->sensor6_options.working_volume_min[VR_Z] = -5.0;
	orb->sensor6_options.working_volume_max[VR_Z] =  5.0;
#endif /* } FREEVR */

#ifdef CAVE /* { */
	orb->restrict_space = 0;
	orb->relative_axis = 0;
	orb->return_to_zero = 0;
	orb->valuator_only = 0;
	orb->valuator_override = 0;
#if 0 /* redundant */
	orb->mixed_valuator_sensor = 0;
#endif

	/* initialize sensors from default tracker pos -- from Stuart Levy */
	initial_cave_sensor.x = CAVEConfig->DefaultTrackerPosition[0];
	initial_cave_sensor.y = CAVEConfig->DefaultTrackerPosition[1];
	initial_cave_sensor.z = CAVEConfig->DefaultTrackerPosition[2];
	initial_cave_sensor.azim = CAVEConfig->DefaultTrackerOrientation[0];
	initial_cave_sensor.elev = CAVEConfig->DefaultTrackerOrientation[1];
	initial_cave_sensor.roll = CAVEConfig->DefaultTrackerOrientation[2];


	CAVEGetTimestamp(&initial_cave_sensor.timestamp);
	orb->head  = initial_cave_sensor;
	orb->wand1 = initial_cave_sensor;
	orb->wand2 = initial_cave_sensor;

	orb->controls.button[0] = 0.0;
	orb->controls.button[1] = 0.0;
	orb->controls.button[2] = 0.0;
	orb->controls.valuator[SIXDOF_TX] = 0.0;
	orb->controls.valuator[SIXDOF_TY] = 0.0;
	orb->controls.valuator[SIXDOF_TZ] = 0.0;
	orb->controls.valuator[SIXDOF_RX] = 0.0;
	orb->controls.valuator[SIXDOF_RY] = 0.0;
	orb->controls.valuator[SIXDOF_RZ] = 0.0;

	orb->controls.valuator[SIXDOF_TYPE] = SIXDOF_ORB;
	orb->controls.valuator[SIXDOF_VERS] = ORB_CURVERSION;
	orb->controls.valuator[SIXDOF_KYBD] = 0.0;
	orb->controls.valuator[SIXDOF_MODE] = 0.0;
	orb->controls.valuator[SIXDOF_USE] = 0.0;

	orb->active_sim6sensor = &orb->wand1;
#endif /* } CAVE */

	/* everything else is zero'd by default */
}


/******************************************************/
static void _SpacetecPrintStruct(FILE *file, _SpacetecPrivateInfo *aux, vrPrintStyle style)
{
	int	count;

	vrFprintf(file, "Spacetec device internal structure (%#p):\n", aux);
	vrFprintf(file, "\r\ttype -- '%s'\n", (aux->device == SPACETEC_ORB ? "SpaceOrb" : (aux->device == SPACETEC_BALL ? "SpaceBall" : "Unknown")));
	vrFprintf(file, "\r\tversion -- '%s'\n", aux->version);
	vrFprintf(file, "\r\toperating parameters -- '%s'\n", aux->op_params);
	vrFprintf(file, "\r\tball resolution -- %d bits\n", aux->ball_resolution);
	vrFprintf(file, "\r\tbeep/silence -- '%s'\n", (aux->silent == 0 ? "beep" : "silent"));
	vrFprintf(file, "\r\tfd = %d\n\tport = '%s'\n\tbaud = %d (%d)\n\topen = %d\n",
		aux->fd,
		aux->port,
		aux->baud_int, aux->baud_enum,
		aux->open);
	vrFprintf(file, "\r\tbad D-packets = %d\n\tbad K-packets = %d\n\tbad other packets = %d\n",
		aux->bad_Dpackets,
		aux->bad_Kpackets,
		aux->bad_other_packets);

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
#  endif
		aux->scale_valuator);
	vrFprintf(file, "\r\tsilent = %d\n",
		aux->silent);

	vrFprintf(file, "\r\torientation -- '%s'\n", (aux->orient_up == 0 ? "away" : "up"));
	if (aux->device == SPACETEC_ORB) {
		vrFprintf(file, "\r\t%01X  %c%c%c%c%c%c%c  x =%4d y =%4d z =%4d a =%4d b =%4d c =%4d\n",
			aux->packtype,
			aux->buttons & 0x0001 ? 'A' : '-',
			aux->buttons & 0x0002 ? 'B' : '-',
			aux->buttons & 0x0004 ? 'C' : '-',
			aux->buttons & 0x0008 ? 'D' : '-',
			aux->buttons & 0x0010 ? 'E' : '-',
			aux->buttons & 0x0020 ? 'F' : '-',
			aux->buttons & 0x0040 ? 'r' : '-',
			aux->offset[VR_TX], aux->offset[VR_TY], aux->offset[VR_TZ], aux->offset[VR_RX], aux->offset[VR_RY], aux->offset[VR_RZ]);
	} else {
		vrFprintf(file, "\r\t%01X  %c%c%c%c%c%c%c%c%c  x =%4d y =%4d z =%4d a =%4d b =%4d c =%4d\n",
			aux->packtype,
			aux->buttons & 0x0001 ? '1' : '-',
			aux->buttons & 0x0002 ? '2' : '-',
			aux->buttons & 0x0004 ? '3' : '-',
			aux->buttons & 0x0008 ? '4' : '-',
			aux->buttons & 0x0010 ? '5' : '-',
			aux->buttons & 0x0020 ? '6' : '-',
			aux->buttons & 0x0040 ? '7' : '-',
			aux->buttons & 0x0080 ? '8' : '-',
			aux->buttons & 0x0100 ? 'p' : '-',
			aux->offset[VR_TX], aux->offset[VR_TY], aux->offset[VR_TZ], aux->offset[VR_RX], aux->offset[VR_RY], aux->offset[VR_RZ]);
	}
}


/**************************************************************************/
static void _SpacetecPrintHelp(FILE *file, _SpacetecPrivateInfo *aux)
{
	int	count;		/* looping variable */

#if 0 || defined(TEST_APP)
	vrFprintf(file, BOLD_TEXT "Sorry, Spacetec - print_help control not yet implemented.\n" NORM_TEXT);
#else
	vrFprintf(file, BOLD_TEXT "Spacetec - inputs:" NORM_TEXT "\n");
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



/********************************************************************/
/*  The following functions are from John Stone's Spaceorb driver:  */
/*       "A simple driver for the SpaceOrb 360 spaceball device     */
/*        manufactured by SpaceTec IMC (Nasdaq:SIMC).               */
/*                                                                  */
/*        Copyright 1997 John E. Stone (j.stone@acm.org)"           */
/*                                                                  */
/*  Specifically, the functions:                                    */
/*     _SpacetecRezero() -- nee orb_rezero()                        */
/*     _SpacetecSetNullRegion() -- nee orb_set_nullregion()         */
/*     _SpacetecCalcNullRegion() -- nee nullregion()                */
/*     _SpacetecDoNullRegion() -- nee orb_do_nullregion()           */
/*     _SpacetecReadInput() -- nee orb_update()                     */
/*     _SpacetecInitializeDevice() -- nee orb_init()                */
/*     _SpacetecOpen() -- nee orb_open()                            */
/*     _SpacetecClose() -- nee orb_close()                          */
/*                                                                  */
/*  The serial code from John's Spaceorb library is no longer used  */
/*    by FreeVR, in lieu of the generic serial interface code in    */
/*    vr_serial.c.                                                  */
/*                                                                  */
/*                                                                  */


/* *****************************************************
 * _SpacetecRezero()
 *   Forces the Orb to re-zero itself at the present twist/position.
 * All future event data is relative to this zero point.
 *
 * The 'Z\r' rezero command works on both the SpaceOrb and SpaceBall 2003
 */
static int _SpacetecRezero(_SpacetecPrivateInfo *orb)
{
	if (orb == NULL)
		return -1;

	vrSerialWriteString(orb->fd, "\rZ\r");

	return 0;
}


/* *****************************************************
 * _SpacetecSetNullRegion()
 *  Enables null-region processing on SpaceOrb output.
 * The null-region is the area (centered at 0) around which
 * each coordinate will report zero even when the SpaceOrb itself
 * reports a number whose absolute value is less than the null region
 * value for that coordinate.  For example, if the null region on the
 * X translation coordinate is set to 50, all SpaceorbGetInputs() would report
 * 0 if X is less than 50 and greater than -50.  If X is 51, SpaceorbGetInputs
 * would report 1.  If X is -51, SpaceorbGetInputs() would report -1.
 * Null-regions help novice users gradually become accustomed to the
 * incredible sensitivity of the SpaceOrb, and make some applications
 * significantly easier to control.  A reasonable default nullregion for all
 * six axes is 65.  Null regions should be tunable by the user, since its
 * likely that not all SpaceOrbs are quite identical, and it is guaranteed
 * that users have varying levels of manual dexterity.
 * Note that setting the null-region too high significantly reduces the
 * dynamic range of the output values from the SpaceOrb.
 */
static void _SpacetecSetNullRegion(_SpacetecPrivateInfo *orb, int ntx, int nty, int ntz, int nrx, int nry, int nrz)
{
	orb->null_offset[VR_TX] = abs(ntx);
	orb->null_offset[VR_TY] = abs(nty);
	orb->null_offset[VR_TZ] = abs(ntz);

	orb->null_offset[VR_RX] = abs(nrx);
	orb->null_offset[VR_RY] = abs(nry);
	orb->null_offset[VR_RZ] = abs(nrz);

	orb->usenullregion = 1;
}


/* **************************************************** */
static int _SpacetecCalcNullRegion(int null, int val)
{
	if (abs(val) > null) {
		return ((val > 0) ? (val - null) : (val + null));
	}
	return 0;
}


/* **************************************************** */
static void _SpacetecDoNullRegion(_SpacetecPrivateInfo *orb)
{
	orb->offset[VR_TX] = _SpacetecCalcNullRegion(orb->null_offset[VR_TX], orb->offset[VR_TX]);
	orb->offset[VR_TY] = _SpacetecCalcNullRegion(orb->null_offset[VR_TY], orb->offset[VR_TY]);
	orb->offset[VR_TZ] = _SpacetecCalcNullRegion(orb->null_offset[VR_TZ], orb->offset[VR_TZ]);
	orb->offset[VR_RX] = _SpacetecCalcNullRegion(orb->null_offset[VR_RX], orb->offset[VR_RX]);
	orb->offset[VR_RY] = _SpacetecCalcNullRegion(orb->null_offset[VR_RY], orb->offset[VR_RY]);
	orb->offset[VR_RZ] = _SpacetecCalcNullRegion(orb->null_offset[VR_RZ], orb->offset[VR_RZ]);
}


/* **************************************************** */
/* BS: I suspect all SpaceOrb data packets are of the form: */
/*    <type char><data bytes>+'\r' */
/*   where:                                                                 */
/*     <type char> is a single character in the normal ASCII printing range */
/*        (ie. it is a letter or symbol code for what data is to follow.)   */
/*     <data bytes> is a sequence of bytes with the high bit set, that      */
/*        can be decoded into the incoming data.  Note that some of these   */
/*        will be of fixed length, which is why the method John used to     */
/*        parse them succeeded -- and if everyone uses the same model of    */
/*        device, then even the fields that would otherwise vary in length  */
/*        will appear to be of fixed length.                                */
/*        Note also therefore that the top bit will need to be stripped     */
/*        from all the bytes in the sequence.                               */
/*     '\r' is a single carriage return *without* the top bit set, so it    */
/*        serves to quickly identify the end of a packet.                   */
/*                                                                          */
/*   The SpaceBall however doesn't seem to use the high bit to differentiate*/
/*     the data bytes from the separation characters.  So, for these, it is */
/*     the number of bytes that has to be used.                             */
/*   [12/30/99] On the other hand, the SpaceBall also uses the '\r' to      */
/*     mark the end of a packet, but that can't always be used, because     */
/*     the 'D' data packets can have a CR as part of the data.              */
/*                                                                          */
static int _SpacetecReadInput(_SpacetecPrivateInfo *orb)
/* Returns the number of inputs successfully read. */
{
static const char	xorme[20] = "D.SpaceWare!\0\0\0\0\0\0\0\0";
static	unsigned char	last_packtype = '\0';
	unsigned char   rawbuf[BUFSIZE];
	unsigned int    tx, ty, tz, rx, ry, rz;
	int             byte;		/* byte being processed within the buffer */
	int		bytes_read;	/* number of bytes read */
	int		packets_recieved;/* number of packets received during this update */
	int		bit_shift;	/* number of bits to shift incoming 'D' values */
#if defined(DEBUG) || 0
	int		count;		/* used to count through some buffer bytes */
#endif

	if (orb == NULL)
		return -1;

	orb->button_change = 0;
	orb->valuator_change = 0;
	packets_recieved = 0;		/* no packets received yet */

#if 0 /* 12/30/99 testing, this doesn't seem to hurt, though perhaps */
	/*  it causes a bit of pausiness in the movement.            */
	vrSerialAwaitData(orb->fd);
#endif
	bytes_read = vrSerialRead(orb->fd, (char *)rawbuf, BUFSIZE-1);

	for (byte = 0; byte < bytes_read; byte++) {

		/* If we're not in the middle of handling a packet, then         */
		/*   determine next packet type, and set parameters accordingly. */
		if (orb->bufpos == 0) {
			/* start copying a new packet */
			switch (rawbuf[byte]) {
			case 0x00:	/* nul: do nothing */
				orb->bufpos = 0;	/* skip bytes */
				orb->packtype = 0;	/* new packet */
				orb->packlen = 1;
				break;
#if 1 /* 12/30/99 testing */
			case 0x11:	/* dc1 (^Q): pause */
				orb->bufpos = 0;	/* skip bytes */
				orb->packtype = 0;	/* new packet */
				orb->packlen = 1;
				vrSleep(200000);
#if defined(DEBUG) || 0
				printf("\n^Q -- pausing\n");
#endif
				break;
			case 0x13:	/* dc3 (^S): unpause */
				orb->bufpos = 0;	/* skip bytes */
				orb->packtype = 0;	/* new packet */
				orb->packlen = 1;
#if defined(DEBUG) || 0
				printf("\n^S -- \"unpausing\"\n");
#endif
				break;
#endif
			case '\r':	/* CR: do nothing */
			case '\n':	/* LF: do nothing */
				orb->bufpos = 0;	/* skip bytes */
				orb->packtype = 0;	/* new packet */
				orb->packlen = 1;
				break;

			case '?':	/* SpaceBall error event -- not sure about Orb*/
				/* NOTE: this is different in that the length */
				/*   varies -- until the '\r' character.      */
				orb->bufpos = 0;
				orb->packtype = '?';
				orb->packlen = -1;	/* go until '\r' */
				break;

			case '!':	/* '!' response to spaceorb '?' query */
				orb->bufpos = 0;	/* skip bytes */
				/* This response exists only on the SpaceOrb */
				if (orb->device == SPACETEC_ORB) {
					orb->packtype = '!';
					orb->packlen = 83;
				} else	orb->packlen = 1;
				break;

			case '@':	/* '@' response to spaceball reset */
				orb->bufpos = 0;	/* skip bytes */
				/* This response exists only on the SpaceBall */
				if (orb->device == SPACETEC_BALL) {
					orb->packtype = '@';
					orb->packlen = 101;
				} else	orb->packlen = 1;
				break;

			case 'H':	/* 'H' help information for SpaceBall */
				/* This response exists only on the SpaceBall */
				if (orb->device == SPACETEC_BALL) {
					orb->packtype = 'H';
					orb->packlen = 622;
				} else	orb->packlen = 1;
				break;

			case 'D':	/* 'D' Displacement packet */
				orb->packtype = 'D';
				if (orb->device == SPACETEC_ORB)
					orb->packlen = 13;	/* Orb D packets are 12 bytes +CR */
				else if (orb->device == SPACETEC_BALL)
					orb->packlen = 16;	/* Ball D packets are 15 bytes +CR*/
				else {
					vrErrPrintf(RED_TEXT "SpaceTec device: Unknown device, assuming SpaceOrb\n" NORM_TEXT);
					orb->packlen = 13;	/* Orb D packets are 12 bytes +CR */
				}
				break;

			case 'K':	/* 'K' Button/Key packet */
				orb->packtype = 'K';
				if (orb->device == SPACETEC_ORB)
					orb->packlen = 6;	/* Orb K packets are 5 bytes + CR */
				else if (orb->device == SPACETEC_BALL)
					orb->packlen = 4;	/* Ball K packets are 3 bytes + CR*/
				else	orb->packlen = 6;
				break;

			case 'M':	/* 'M' SpaceBall Mode info */
				if (orb->device == SPACETEC_BALL) {
					orb->packtype = 'M';
					orb->packlen = 7;
				} else	orb->packlen = 1;
				break;

			case 'N':	/* 'N' null region info response packet */
				orb->packtype = 'N';
				orb->packlen = 3;	/* N packets are 3 bytes long */
				break;

			case 'P':	/* 'P' data rate info response packet */
				orb->packtype = 'P';
				if (orb->device == SPACETEC_ORB)
					orb->packlen = 4;	/* P packets are 4 bytes long */
				else	orb->packlen = 6;
				break;

#if 0
			case 'R':	/* orb reset packet, ball ? packet */
				orb->packtype = 'R';
				if (orb->device == SPACETEC_ORB)
					orb->packlen = 51;	/* reset packets are long */
				else	orb->packlen = 5;	/* Ball R packet is x bytes + CR */
				break;
#endif

			case 'F':	/* SpaceBall feel packet */
				if (orb->device == SPACETEC_BALL) {
					orb->packtype = 'F';
					orb->packlen = 4;	/* Ball F packets are 3 bytes + CR */
				} else {
					orb->packlen = 1;	/* Orb doesn't have 'F' packets */
				}
				break;

			case 'C':	/* SpaceBall character? mode packet */
				if (orb->device == SPACETEC_BALL) {
					orb->packtype = 'C';
					orb->packlen = 4;	/* Ball C packets are 3 bytes + CR */
				} else {
					orb->packlen = 1;	/* Orb doesn't have 'C' packets */
				}
				break;

			case 'E':	/* SpaceBall unknown-E packet */
				if (orb->device == SPACETEC_BALL) {
					orb->packtype = 'E';
					orb->packlen = 4;	/* Ball C packets are 3 bytes + CR */
				} else {
					orb->packlen = 1;	/* Orb doesn't have 'C' packets */
				}
				break;

			default:	/* Unknown packet! */
				if (getenv("DEBUG_BAD_PACKETS")) {
					printf("\nLast packet code: 0x%02x ('%c')      ",
						last_packtype, (isprint(last_packtype & 0x7f) ? last_packtype & 0x7f : '.'));
					printf("\nUnknown packet: 0x%02x ('%c'): %02x(%c) %02x(%c) %02x(%c) %02x(%c) %02x(%c) %02x(%c) %02x(%c) %02x(%c) %02x(%c) %02x(%c) %02x(%c) %02x(%c) %02x(%c) %02x(%c) %02x(%c) ",
						rawbuf[byte], (isprint(rawbuf[byte] & 0x7f) ? rawbuf[byte] & 0x7f : '.'),
						rawbuf[byte+1], (isprint(rawbuf[byte+1] & 0x7f) ? rawbuf[byte+1] & 0x7f : '.'),
						rawbuf[byte+2], (isprint(rawbuf[byte+2] & 0x7f) ? rawbuf[byte+2] & 0x7f : '.'),
						rawbuf[byte+3], (isprint(rawbuf[byte+3] & 0x7f) ? rawbuf[byte+3] & 0x7f : '.'),
						rawbuf[byte+4], (isprint(rawbuf[byte+4] & 0x7f) ? rawbuf[byte+4] & 0x7f : '.'),
						rawbuf[byte+5], (isprint(rawbuf[byte+5] & 0x7f) ? rawbuf[byte+5] & 0x7f : '.'),
						rawbuf[byte+6], (isprint(rawbuf[byte+6] & 0x7f) ? rawbuf[byte+6] & 0x7f : '.'),
						rawbuf[byte+7], (isprint(rawbuf[byte+7] & 0x7f) ? rawbuf[byte+7] & 0x7f : '.'),
						rawbuf[byte+8], (isprint(rawbuf[byte+8] & 0x7f) ? rawbuf[byte+8] & 0x7f : '.'),
						rawbuf[byte+9], (isprint(rawbuf[byte+9] & 0x7f) ? rawbuf[byte+9] & 0x7f : '.'),
						rawbuf[byte+10], (isprint(rawbuf[byte+10] & 0x7f) ? rawbuf[byte+10] & 0x7f : '.'),
						rawbuf[byte+11], (isprint(rawbuf[byte+11] & 0x7f) ? rawbuf[byte+11] & 0x7f : '.'),
						rawbuf[byte+12], (isprint(rawbuf[byte+12] & 0x7f) ? rawbuf[byte+12] & 0x7f : '.'),
						rawbuf[byte+13], (isprint(rawbuf[byte+13] & 0x7f) ? rawbuf[byte+13] & 0x7f : '.'),
						rawbuf[byte+14], (isprint(rawbuf[byte+14] & 0x7f) ? rawbuf[byte+14] & 0x7f : '.'),
						rawbuf[byte+15], (isprint(rawbuf[byte+15] & 0x7f) ? rawbuf[byte+15] & 0x7f : '.'));
				}
				orb->packtype = 0;
				orb->packlen = 1;
				orb->bad_other_packets++;
				break;
			}
		}

		/**************************************************************/
		/* Copy the current (single) rawbuf[byte] into the orb buffer */

		/* If we're processing an Orb 'D' packet, then */
		/*   decode each byte with the secret key.     */
		if (orb->packtype == 'D') {
			if (orb->device == SPACETEC_ORB)
				orb->buf[orb->bufpos] = rawbuf[byte] ^ xorme[orb->bufpos];
			else	orb->buf[orb->bufpos] = rawbuf[byte];
			orb->bufpos++;
		} else {
			orb->buf[orb->bufpos++] = rawbuf[byte] & 0x7F;
		}

		/***************************************************/
		/* If packlen is -1, then we're waiting for a '\r' */
		if (orb->packlen == -1) {
			if (orb->buf[orb->bufpos] == '\r') {
				orb->packlen = orb->bufpos;
			}
		}

		/****************************************************/
		/* If we haven't copied the entire packet into the  */
		/*   orb buffer, then continue with the 'for' loop. */
		if (orb->bufpos != orb->packlen) {
			continue;
		}

	/* TODO: I think this routine can be written a little more */
	/*   efficiently, and also more clearly (sorry John), by   */
	/*   just dealing with the packet in the above switch      */
	/*   statement, and then jumping to the first byte of the  */
	/*   next packet.                                          */

		/************************************************************/
		/* When we get here, the orb->buf will contain all the data */
		/*   for the given packet, so now we can process it.        */
		switch (orb->packtype) {
		case '?':	/* SpaceBall error event -- not sure about Orb*/
#if defined(DEBUG) || 0
			printf("\n'?' packet is: '");
			for (count = 0; count < orb->bufpos-1; count++) {
				printf("%c", orb->buf[count]);
			}
			printf("'\n");
#endif
			break;
		case '!':	/* SpaceOrb information event */
			if (orb->device == SPACETEC_ORB) {
				/* "!1 ..." is version and copyright information */
				memset(orb->version, 0, sizeof(orb->version));
				memcpy(orb->version, &(orb->buf[3]), 48);

				/* "!2 ..." is version and copyright information */
				memset(orb->op_params, 0, sizeof(orb->op_params));
				((char *)(memccpy(orb->op_params, &(orb->buf[55]), '\r', sizeof(orb->op_params))))[-1] = '\0';
			} else if (orb->device == SPACETEC_BALL) {
				vrPrintf("hmmm, Spaceball got a '!' packet.\n");
			}
			break;

		case '@':	/* SpaceBall information event */
			if (orb->device == SPACETEC_BALL) {
				/* "@2 ..." is version information */
				memset(orb->version, 0, sizeof(orb->version));
				sprintf(orb->version, "Spaceball: %44s", &(orb->buf[56]));
			}
			break;

		case 'H':	/* SpaceBall help event */
			if (orb->device == SPACETEC_BALL) {
#if defined(DEBUG) || 0
				char	*replace;
				while ((replace = strchr(orb->buf, '\r')) != NULL)
					*replace = '\n';

				vrPrintf(BOLD_TEXT "Ball info = '%s'\n\n" NORM_TEXT, orb->buf);
#endif
			}
			break;

		case 'D':	/* SpaceOrb & SpaceBall ball displacement event */
			/* packet error check */
			if (orb->buf[orb->packlen-1] != '\r') {
				orb->bufpos = 0;
				orb->packtype = 0;
				orb->packlen = 1;
				orb->bad_Dpackets++;
				if (orb->buf[orb->packlen-2] == '\r') {
					vrErrPrintf(RED_TEXT "SpaceTec 'D'ata packet apparently missing one byte.\n");
				}
				if (orb->buf[orb->packlen] == '\r') {
					vrErrPrintf(RED_TEXT "SpaceTec 'D'ata packet apparently has one extra byte.\n");
				}
				continue;
			}

			if (orb->device == SPACETEC_ORB) {
				tx = ((orb->buf[2] & 0x7F) << 3) |
					((orb->buf[3] & 0x70) >> 4);

				ty = ((orb->buf[3] & 0x0F) << 6) |
					((orb->buf[4] & 0x7E) >> 1);

				tz = ((orb->buf[4] & 0x01) << 9) |
					((orb->buf[5] & 0x7F) << 2) |
					((orb->buf[6] & 0x60) >> 5);

				rx = ((orb->buf[6] & 0x1F) << 5) |
					((orb->buf[7] & 0x7C) >> 2);

				ry = ((orb->buf[7] & 0x03) << 8) |
					((orb->buf[8] & 0x7F) << 1) |
					((orb->buf[9] & 0x40) >> 6);

				rz = ((orb->buf[9] & 0x3F) << 4) |
					((orb->buf[10] & 0x78) >> 3);

				orb->timer = ((orb->buf[10] & 0x07) << 7) |
					(orb->buf[11] & 0x7F);

				bit_shift = 22;	/* ie. 32 bit integer - 10 bit resolution */

			} else if (orb->device == SPACETEC_BALL) {
				tx = ((orb->buf[ 3]) << 8) | (orb->buf[ 4]);
				ty = ((orb->buf[ 5]) << 8) | (orb->buf[ 6]);
				tz = ((orb->buf[ 7]) << 8) | (orb->buf[ 8]);
				rx = ((orb->buf[ 9]) << 8) | (orb->buf[10]);
				ry = ((orb->buf[11]) << 8) | (orb->buf[12]);
				rz = ((orb->buf[13]) << 8) | (orb->buf[14]);

				orb->timer = ((orb->buf[1]) << 8) | (orb->buf[2]);

				bit_shift = 16;	/* ie. 32 bit integer - 16 bit resolution */
			}

			/* the bit shifting is done to make the number a proper 32-bit SIGNED integer */
			if (orb->orient_up) {
				/* translate values for pointing orb up */
				orb->offset[VR_TX] =   (((int) tx) << bit_shift) >> bit_shift;
				orb->offset[VR_TZ] =   (((int) ty) << bit_shift) >> bit_shift;
				orb->offset[VR_TY] = -((((int) tz) << bit_shift) >> bit_shift);
				orb->offset[VR_RX] =   (((int) rx) << bit_shift) >> bit_shift;
				orb->offset[VR_RZ] =   (((int) ry) << bit_shift) >> bit_shift;
				orb->offset[VR_RY] = -((((int) rz) << bit_shift) >> bit_shift);
			} else {
				/* direct translation for pointing away from user */
				orb->offset[VR_TX] =   (((int) tx) << bit_shift) >> bit_shift;
				orb->offset[VR_TY] =   (((int) ty) << bit_shift) >> bit_shift;
				orb->offset[VR_TZ] =   (((int) tz) << bit_shift) >> bit_shift;
				orb->offset[VR_RX] =   (((int) rx) << bit_shift) >> bit_shift;
				orb->offset[VR_RY] =   (((int) ry) << bit_shift) >> bit_shift;
				orb->offset[VR_RZ] =   (((int) rz) << bit_shift) >> bit_shift;
			}

#if 0 /* only need this when in left-handed coord sys */
			if (orb->device == SPACETEC_BALL) {
				/* The Z-axis on the ball is backward */
				orb->offset[VR_TZ] = -orb->offset[VR_TZ];
				orb->offset[VR_RZ] = -orb->offset[VR_RZ];
			}
#endif

			/* perform null region processing */
			if (orb->usenullregion)
				_SpacetecDoNullRegion(orb);

			orb->valuator_change = 1;
			break;

		case 'K':	/* SpaceOrb & SpaceBall keypress (ie button) event */
			/* packet error check */
			if (orb->buf[orb->packlen-1] != '\r') {
				orb->bufpos = 0;
				orb->packtype = 0;
				orb->packlen = 1;
				orb->bad_Kpackets++;
				continue;
			}

			if (orb->device == SPACETEC_ORB) {
				orb->buttons = orb->buf[2] & 0x7F;
				orb->timer = ((orb->buf[1] & 0x7f) << 8) | (orb->buf[4] & 0x7f);
			} else if (orb->device == SPACETEC_BALL) {
				orb->buttons = ((orb->buf[1] & 0x1F) << 4) | (orb->buf[2] & 0x0F);
#if defined(DEBUG) || 0
			printf("\nreceived key packet \n");
			for (count = 0; count < orb->bufpos+2; count++) {
				printf(" %02x ", orb->buf[count]);
			}
			printf("\n");
#endif
			}

			orb->button_change = 1;
			break;

		case 'N':	/* SpaceOrb and SpaceBall null region info */

#if defined(DEBUG) || 0
			printf("\n'N' packet: received response to null region query: '");
			for (count = 0; count < orb->bufpos-1; count++) {
				printf("%c", orb->buf[count]);
			}
			printf("'\n");
#endif
			break;

		case 'P':	/* SpaceOrb response to data rate query, SpaceBall pulse period */
			/* packet error check */
			if (orb->buf[orb->packlen-1] != '\r') {
				orb->bufpos = 0;
				orb->packtype = 0;
				orb->packlen = 1;
				orb->bad_other_packets++;
				continue;
			}
#if defined(DEBUG) || 0
			printf("\n'P' packet is: '");
			for (count = 0; count < orb->bufpos-1; count++) {
				printf("%c", orb->buf[count]);
			}
			printf("'\n");
#endif
			break;

		case 'R':	/* SpaceOrb (maybe SpaceBall) reset pack */
			/* packet error check */
			if (orb->buf[orb->packlen-1] != '\r') {
				orb->bufpos = 0;
				orb->packtype = 0;
				orb->packlen = 1;
				orb->bad_other_packets++;
				continue;
			}
			if (orb->device == SPACETEC_ORB) {
				printf("\n\nSpaceOrb: reset = '%s'\n\n", &(orb->buf[0]));
			} else {
				printf("\nSpaceBall 'R' packet: value = %c\n", orb->buf[1]);
			}
			break;

		case 'F':	/* SpaceBall feel packet */
			/* packet error check */
			if (orb->buf[orb->packlen-1] != '\r') {
				orb->bufpos = 0;
				orb->packtype = 0;
				orb->packlen = 1;
				orb->bad_other_packets++;
				continue;
			}
#if defined(DEBUG) || 0
			printf("\n'F' (Feel) packet is: '%c%c'\n", orb->buf[1], orb->buf[2]);
#endif
			break;

		case 'C':	/* SpaceBall character? mode packet */
			/* packet error check */
			if (orb->buf[orb->packlen-1] != '\r') {
				orb->bufpos = 0;
				orb->packtype = 0;
				orb->packlen = 1;
				orb->bad_other_packets++;
				continue;
			}
#if defined(DEBUG) || 0
			printf("\n'C' packet is: '%c%c'\n", orb->buf[1], orb->buf[2]);
#endif
			break;

		case 'E':	/* SpaceBall unknown-E packet */
			/* packet error check */
			if (orb->buf[orb->packlen-1] != '\r') {
				orb->bufpos = 0;
				orb->packtype = 0;
				orb->packlen = 1;
				orb->bad_other_packets++;
				continue;
			}
#if defined(DEBUG) || 0
			printf("\n'E' packet is: '%c%c'\n", orb->buf[1], orb->buf[2]);
#endif
			break;

		case 'M':	/* SpaceBall Mode packet */
			/* packet error check */
			if (orb->buf[orb->packlen-1] != '\r') {
				orb->bufpos = 0;
				orb->packtype = 0;
				orb->packlen = 1;
				orb->bad_other_packets++;
				continue;
			}
#if defined(DEBUG) || 0
			printf("\n'M' packet is: '%c%c%c%c%c'\n", orb->buf[1], orb->buf[2], orb->buf[3], orb->buf[4], orb->buf[5]);
#endif
			break;

		case 0:		/* a non-packet */
			break;


		default:
			orb->bad_other_packets++;
			if (getenv("DEBUG_BAD_PACKETS")) {
				printf("Unkpack: %2x \n ", orb->packtype);
				printf("   char:          %c \n", orb->packtype & 0x7f);
			}
			break;
		}

		/* reset */
		orb->bufpos = 0;
		if (orb->packtype != '\0')
			last_packtype = orb->packtype;
		orb->packtype = 0;
		orb->packlen = 1;
		packets_recieved++;
	}

	return packets_recieved;
}


/* **********************************************************/
/* _SpacetecInitializeDevice() is called in the OPEN phase  */
/*   of input interface -- after the types of inputs have   */
/*   been determined (during the CREATE phase).             */
/* This function performs a software re-initialization of   */
/*   the SpaceTec device, clearing all unprocessed events.  */
/*   A request for version/parameter information is sent to */
/*   the device, and after a one second delay, all packets  */
/*   are read.  Initialization then forces the Orb to       */
/*   re-zero itself.                                        */
static int _SpacetecInitializeDevice(_SpacetecPrivateInfo *orb)
{
	if (orb == NULL)
		return -1;

	switch (orb->device) {

	case SPACETEC_ORB:
		_SpacetecReadInput(orb);

		/********************/
		/* clear the palate */
		vrSerialWriteString(orb->fd, "\r");
		vrSleep(400000);

		/**********************************/
		/* request version and param info */
		vrSerialWriteString(orb->fd, "\r?\r");
		vrSleep(900000);	/* wait a little (.9 sec) for info response */

		/************************/
		/* read some input data */
		_SpacetecReadInput(orb);
		_SpacetecRezero(orb);

		/* Some codes that seem to generate a non-error response. */
		/*   "?\r" responds with an info string.                  */
		/*   "d\r" responds with (apparently) a 'D' packet.       */
		/*   "f\r" responds with "FF\r"                           */
		/*   "n\r" responds with "NZ\r"                           */
		/*   "p\r" responds with "P!q\r"                          */
		/*   "z\r" responds with "Z726^X54\r" -with some ctl bytes*/
		/*   "Z\r" responds with nothing, but does a rezero       */
		/*   "$\r" responds with "$Ignoredx\r"                    */
		/* other info:                                                */
		/*   upon receiving power, the following string is sent:      */
		/*    "DSpaceWare! (R) V4.34 19-Oct-96 Copyright (C) 1996r\r" */
		/*   after that, a 'K' packet is sent every one second.       */

		break;

	case SPACETEC_BALL:
		_SpacetecReadInput(orb);

		/* TODO: need to figure out if we want all these startup commands. */
		/********************/
		/* clear the palate */
		vrSerialWriteString(orb->fd, "\r\r");
		vrSleep(200000);

		/**********************************/
		/* request version and param info */
		while (orb->version[0] == '\0') {
			vrSerialWriteString(orb->fd, "\r@RESET\r");
			vrSleep(300000);
			_SpacetecReadInput(orb);
		}
		vrSleep(200000);	/* I'm not sure if this one is necessary */

		/********************************/
		/* send some device setup codes */
		vrSerialWriteString(orb->fd, "\rCB@\r");	/* "CB@\r" also works */
#if 0
		vrSerialWriteString(orb->fd, "CB\r");
		vrSerialWriteString(orb->fd, "NT\r");	/* T ?= 4 */
#endif
		vrSerialWriteString(orb->fd, "FT?\r");
		vrSerialWriteString(orb->fd, "FR?\r");
		vrSerialWriteString(orb->fd, "P@r@r\r");/* @ ?= 0 r ?= 2 */
		vrSerialWriteString(orb->fd, "MSSVR\r");
		vrSerialWriteString(orb->fd, "N\1\r");	/* orig was "NT" */
		_SpacetecRezero(orb);
#if 0
		vrSerialWriteString(orb->fd, "d\r");
		vrSerialWrite(orb->fd, "%D\0\0\0\0\0\0\0\0\0\0\0\0\0\0\r", 17);
		vrSerialWriteString(orb->fd, "BcCc\r");	/* c = 3 C = 3 */
			/* 'B' is the beep command, each 'c' is a beep of about .1 sec, */
			/*   each 'C' is a silence of about .1 sec -- max in buffer is  */
			/*   16 bytes, with one for the 'B', and one for the '\r'.      */
			/*   a 'z' beeps about .8 sec and a 'Z' is silent for about .8  */
#endif

		/************************/
		/* read some input data */
		_SpacetecReadInput(orb);

		/* Some other tests: */
		/*  "*\r" responds with "*PU422?36276(4)\r" on my Model 2003 */
		/*  "a\r"                             */
		/*  "b\r" responds with "B@\r"        */
		/*  "c\r" responds with "CBT\r"       */
		/*  "d\r" responds with nothing (apparently) vs. the "?d\r" as an error would */
		/*  "f\r" responds with FR?\r         */
		/*  "h\r" responds with a lot of info */
		/*  "m\r" responds with "MSSVLA\r"    */
		/*  "n\r" responds with "NT\r"        */
		/*  "o\r" responds with "OIPS\lfS\r"  */
		/*  "p\r" responds with "P@q@q\r"     */
		/*  "r\r" responds with "RPPP@\r"     */
		/*  "s\r" responds with "SpE\r"       */
		/*  "t\r" responds with "TPPP@\r"     */
		/*  "x\r" responds with "X@44@\r"     */
		/*  "z\r" responds with "Z442?35286(4)\r"          */
		/*  "Z\r" responds with nothing, but does a rezero */
		/*  "B<up to 14 chars>\r" responds with nothing, but does one or more beeps */
		/*  "N<char>\r" responds with "N<char>\r" (for letters, numbers behave different)*/
		/*    - 'N' is null region.                        */
		/*  "CB<char>\r" responds with "CB<char>\r"        */
		/*  "CB\r" responds with "CB<char>\r"  (where <char> is value from last time set)*/
		/*  "P@r@r\r" responds with "P@q@q|\r" */
		/*  "MSSV\r" responds with "DjVLA|\r" (ie. it sends a 'D' packet) */

		/* Okay, here's the lowdown from the device itself:   */
		/*   in response to an "h\r" command, the following is returned.
			HPackets:
			H'<SPACE>[?]' or '<TAB>[?]'     - ignored
			H'%[?]'                         - echoed
			H'@RESET'
			H'A<0><0><0><0><0><0>'          - obsolete
			H'B<? beep notes>'
			H'C<1 mode(B|b|P|p)>[1 xoff timeout]'
			H'F(T|R|B)<1 feel>
			H'h' 'help' 'HELP'
			H'hv'
			H'h?'
			H'h<1 error code>'
			H'k'
			H'L(+|-)'                       - click
			H'M<1 tran mode>[1 rot mode][1 rot format][1 handedness][A]'
			H'M<N|S|X>[N|S|X][V|X][L|R|X][A]'
			H'N<1 null region>'
			H'O<6 crm>'                     - obsolete
			H'P<2 max pulse period><2 min pulse period>'
			H'RPPP<40><00><00><00><00><00>' - obsolete
			H'SpE'  - obsolete
			H'TPPP<40><00><00><00><00><00>' - obsolete
			H'X<6 xyz sensitivities>'
			H'Z'                            - zero ball'
			H'Z[12 zero settings]'
		*/

		break;

	default:
		vrErrPrintf(RED_TEXT "Unknown SpaceTec device, unable to initialize data.\n" NORM_TEXT);
	}

	return 0;
}


/* *****************************************************
 * _SpacetecOpen()
 *   Open a named serial port which a SpaceTec device is attached to.
 * Returns the value of the file descriptor (which is less than
 *   zero when unable to open the orb port).
 */
static int _SpacetecOpen(_SpacetecPrivateInfo *orb, char *orbport, int baud)
{
	if (orbport == NULL)
		return -1;

	if (orb == NULL) {
		orb = (_SpacetecPrivateInfo *)malloc(sizeof(_SpacetecPrivateInfo));
		if (orb == NULL)
			return -1;

		/* clear all values in _SpacetecPrivateInfo to 0 */
		memset(orb, 0, sizeof(_SpacetecPrivateInfo));
	}

	orb->port = strdup(orbport);
	orb->baud_enum = baud;
	orb->packlen = 1;

	orb->fd = vrSerialOpen(orbport, baud);
	if (orb->fd < 0) {
		free(orb);
		fprintf(stderr, "Tracker process: Unable to open SpaceTec device.\n");
		return -1;
	}

	_SpacetecInitializeDevice(orb);		/* TODO: move this call to _SpacetecOpenFunction() */

	return orb->fd;		/* successful open */
}


/* *****************************************************
 * _SpacetecClose()
 *   Closes down the SpaceTec serial port, frees allocated resources and
 * discards any unprocessed orb messages.
 */
static int _SpacetecClose(_SpacetecPrivateInfo *orb)
{
	if (orb == NULL)
		return -1;

	vrSerialClose(orb->fd);
	free(orb);
	return 0;		/* successful close */
}


/******************************************************/
static unsigned int _SpacetecButtonValue(char *buttonname)
{
	switch (buttonname[0]) {
	case 'a':
	case 'A':
	case '1':
		return ORB_BUTTONINDEX_A;	/* same as BALL_BUTTONINDEX_1 */
	case 'b':
	case 'B':
	case '2':
		return ORB_BUTTONINDEX_B;	/* same as BALL_BUTTONINDEX_2 */
	case 'c':
	case 'C':
	case '3':
		return ORB_BUTTONINDEX_C;	/* same as BALL_BUTTONINDEX_3 */
	case 'd':
	case 'D':
	case '4':
		return ORB_BUTTONINDEX_D;	/* same as BALL_BUTTONINDEX_4 */
	case 'e':
	case 'E':
	case '5':
		return ORB_BUTTONINDEX_E;	/* same as BALL_BUTTONINDEX_5 */
	case 'f':
	case 'F':
	case '6':
		return ORB_BUTTONINDEX_F;	/* same as BALL_BUTTONINDEX_6 */
	case 'r':
	case 'R':
	case '7':
		return ORB_BUTTONINDEX_RESET;	/* same as BALL_BUTTONINDEX_7 */
	case '8':
		return BALL_BUTTONINDEX_8;
	case 'p':
	case 'P':
	case '9':
		return BALL_BUTTONINDEX_PICK;
	}

	return -1;
}


/******************************************************/
static unsigned int _SpacetecValuatorValue(char *valuatorname)
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
		vrErrPrintf("_SpacetecValuatorValue: unknown valuator name '%s'\n", valuatorname);
		return -1;
	}
}


	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/


#if defined(FREEVR) /* { */
/*****************************************************************/
/*** Functions for FreeVR access of SpaceTec devices for input ***/
/*****************************************************************/


	/*************************************/
	/***  FreeVR NON public routines   ***/
	/*************************************/


/* TODO: I don't think we want this function in this file, since multiple */
/*   input devices can be interfaced with in the same process.            */
/*********************************************************/
static void _SpacetecSigpipeHandler(int sig)
{
	vrDbgPrintf("Caught SIGPIPE error, process dying.\n");
}


/*********************************************************/
static void _SpacetecParseArgs(_SpacetecPrivateInfo *aux, char *args)
{
static	char	*orient_choices[] = { "up", "away", NULL };
static	int	orient_values[] = { 1, 0 };
	int	null_value = -1;
	float	scale_value = -1.0;
	float	volume_values[6];

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

	/*****************************************************/
	/** Argument format: "orient" "=" { "up" | "away" } **/
	/*****************************************************/
	vrArgParseChoiceInteger(args, "orient", &(aux->orient_up), orient_choices, orient_values);

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
static void _SpacetecGetData(vrInputDevice *devinfo)
{
	_SpacetecPrivateInfo	*aux = (_SpacetecPrivateInfo *)devinfo->aux_data;
#if defined(TEST_APP) || defined(CAVE)
	double			scale_trans = aux->scale_trans;
	double			scale_rot = aux->scale_rot;
#endif
#ifdef UNPICKY_COMPILER
	int			num_inputs;
#endif
static	int			buttons_last = 0;
	int			count;
	float			values[6];

	/*******************/
	/* gather the data */
#ifdef UNPICKY_COMPILER
	num_inputs = _SpacetecReadInput(aux);
#else
	_SpacetecReadInput(aux);
#endif

	/***************/
	/** valuators **/
	if (aux->valuator_change) {
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
	}

	/*************/
	/** buttons **/
	if (aux->button_change /* TODO: why was the pick button being excluded?  && !(aux->buttons & 0x0100) */) {
		for (count = 0; count < MAX_BUTTONS; count++) {
			if ((aux->buttons & (0x1 << count)) != (buttons_last & (0x1 << count))) {
				if (aux->button_inputs[count] != NULL) {
					switch (aux->button_inputs[count]->input_type) {
					case VRINPUT_BINARY:
						if (aux->device == SPACETEC_BALL && !aux->silent)
							vrSerialWriteString(aux->fd, BallShortBeepMsg);
						vrAssign2switchValue((vr2switch *)(aux->button_inputs[count]), ((aux->buttons & (0x1 << count)) != 0));
						break;
					case VRINPUT_CONTROL:
						vrCallbackInvokeDynamic(((vrControl *)(aux->button_inputs[count]))->callback, 1, ((aux->buttons & (0x1 << count)) != 0));
						break;
					default:
						vrErrPrintf(RED_TEXT "_SpacetecGetData: Unable to handle button inputs that aren't Binary or Control inputs\n" NORM_TEXT);
						break;
					}
				}
			}
		}

		buttons_last = aux->buttons;
	}
}


	/****************************************************************/
	/*    Function(s) for parsing SpaceTec "input" declarations.    */
	/*                                                              */
	/*  These _DEVICE<type>Input() functions are called during the  */
	/*  CREATE phase of the input interface.                        */

/**************************************************************************/
static vrInputMatch _SpacetecButtonInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
        _SpacetecPrivateInfo	*aux = (_SpacetecPrivateInfo *)devinfo->aux_data;
	int			button_num;

	button_num = _SpacetecButtonValue(dti->instance);
	if (button_num == -1)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_SpacetecButtonValue: Warning, button['%s'] did not match any known button\n", dti->instance);
	else if (aux->button_inputs[button_num] != NULL)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_SpacetecButtonValue: Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);
	aux->button_inputs[button_num] = (vr2switch *)input;
	vrDbgPrintfN(INPUT_DBGLVL, "assigned button event of value 0x%02x to input pointer = %#p)\n",
		button_num, aux->button_inputs[button_num]);

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/**************************************************************************/
static vrInputMatch _SpacetecOrbInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
        _SpacetecPrivateInfo	*aux = (_SpacetecPrivateInfo *)devinfo->aux_data;
	int			valuator_num;

	valuator_num = _SpacetecValuatorValue(dti->instance);
	if (valuator_num == -1)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_SpacetecOrbInput: Warning, valuator['%s'] did not match any known valuator\n", dti->instance);
	else if (aux->valuator_inputs[valuator_num] != NULL)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_SpacetecOrbInput: Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);
	aux->valuator_inputs[valuator_num] = (vrValuator *)input;
	if (dti->instance[0] == '-')
		aux->valuator_sign[valuator_num] = -1.0;
	else	aux->valuator_sign[valuator_num] =  1.0;
	vrDbgPrintfN(INPUT_DBGLVL, "assigned valuator event of value 0x%02x to input pointer = %#p)\n",
		valuator_num, aux->valuator_inputs[valuator_num]);

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/**************************************************************************/
static vrInputMatch _Spacetec6DofInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
        _SpacetecPrivateInfo	*aux = (_SpacetecPrivateInfo *)devinfo->aux_data;
	int			sensor_num;

	sensor_num = atoi(dti->instance);
	if (sensor_num < 0)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_Spacetec6DofInput: Warning, sensor number must be between %d and %d\n", 0, MAX_6SENSORS);
	else if (aux->sensor6_inputs[sensor_num] != NULL)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_Spacetec6DofInput: Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);
	aux->sensor6_inputs[sensor_num] = (vr6sensor *)input;
	vrAssign6sensorR2Exform(aux->sensor6_inputs[sensor_num], strchr(dti->instance, ','));
	vrDbgPrintfN(INPUT_DBGLVL, "assigned 6sensor event of value 0x%02x to input pointer = %#p)\n",
		sensor_num, aux->sensor6_inputs[sensor_num]);

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


	/************************************************************/
	/***************** FreeVR Callback routines *****************/
	/************************************************************/

	/************************************************************/
	/*    Callbacks for controlling the SpaceTec features.      */
	/*                                                          */

/************************************************************/
static void _SpacetecSystemPauseToggleCallback(vrInputDevice *devinfo, int value)
{
	_SpacetecPrivateInfo	*aux = (_SpacetecPrivateInfo *)devinfo->aux_data;

        if (value == 0)
                return;

	if (aux->device == SPACETEC_BALL && !aux->silent)
		vrSerialWriteString(aux->fd, BallLongBeepMsg);

	devinfo->context->paused ^= 1;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Spacetec Control: system_pause = %d.\n",
		devinfo->context->paused);
}

/************************************************************/
static void _SpacetecPrintContextStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintContext(stdout, devinfo->context, verbose);
}

/************************************************************/
static void _SpacetecPrintConfigStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintConfig(stdout, devinfo->context->config, verbose);
}

/************************************************************/
static void _SpacetecPrintInputStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintInput(stdout, devinfo->context->input, verbose);
}

/************************************************************/
static void _SpacetecPrintStructCallback(vrInputDevice *devinfo, int value)
{
        _SpacetecPrivateInfo  *aux = (_SpacetecPrivateInfo *)devinfo->aux_data;

        if (value == 0)
                return;

        _SpacetecPrintStruct(stdout, aux, verbose);
}

/************************************************************/
static void _SpacetecPrintHelpCallback(vrInputDevice *devinfo, int value)
{
        _SpacetecPrivateInfo  *aux = (_SpacetecPrivateInfo *)devinfo->aux_data;

        if (value == 0)
                return;

        _SpacetecPrintHelp(stdout, aux);
}

/************************************************************/
static void _SpacetecSensorNextCallback(vrInputDevice *devinfo, int value)
{
	_SpacetecPrivateInfo	*aux = (_SpacetecPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	if (aux->device == SPACETEC_BALL && !aux->silent)
		vrSerialWriteString(aux->fd, BallLongBeepMsg);

	if (devinfo->num_6sensors == 0) {
		vrDbgPrintfN(SELFCTRL_DBGLVL, "Orb Callback: next sensor -- no sensors available.\n",
			aux->active_sim6sensor);
		return;
	}

	vrAssign6sensorActiveValue(((vr6sensor *)aux->sensor6_inputs[aux->active_sim6sensor]), 0);
	do {
		aux->active_sim6sensor++;
	} while (aux->sensor6_inputs[aux->active_sim6sensor] == NULL && aux->active_sim6sensor < MAX_6SENSORS);

	if (aux->sensor6_inputs[aux->active_sim6sensor] == NULL || aux->active_sim6sensor >= MAX_6SENSORS) {
		for (aux->active_sim6sensor = 0; aux->active_sim6sensor < MAX_6SENSORS && aux->sensor6_inputs[aux->active_sim6sensor] == NULL; aux->active_sim6sensor++);
	}
	vrAssign6sensorActiveValue(((vr6sensor *)aux->sensor6_inputs[aux->active_sim6sensor]), 1);

	vrDbgPrintfN(SELFCTRL_DBGLVL, "Orb Callback: 6sensor[%d] now active.\n", aux->active_sim6sensor);
}

/* TODO: see if there is a way to call this as an N-switch */
/************************************************************/
static void _SpacetecSensorSetCallback(vrInputDevice *devinfo, int value)
{
	_SpacetecPrivateInfo	*aux = (_SpacetecPrivateInfo *)devinfo->aux_data;

	if (value == aux->active_sim6sensor)
		return;

	if (aux->device == SPACETEC_BALL && !aux->silent)
		vrSerialWriteString(aux->fd, BallLongBeepMsg);

	if (value < 0 || value >= MAX_6SENSORS) {
		vrDbgPrintfN(SELFCTRL_DBGLVL, "Spacetec Control: set sensor (%d) -- out of range.\n", value);
	}

	if (aux->sensor6_inputs[value] == NULL) {
		vrDbgPrintfN(SELFCTRL_DBGLVL, "Spacetec Control: set sensor (%d) -- no such sensor available.\n", value);
		return;
	}

	vrAssign6sensorActiveValue(((vr6sensor *)aux->sensor6_inputs[aux->active_sim6sensor]), 0);
	aux->active_sim6sensor = value;
	vrAssign6sensorActiveValue(((vr6sensor *)aux->sensor6_inputs[aux->active_sim6sensor]), 1);

	vrDbgPrintfN(SELFCTRL_DBGLVL, "Spacetec Control: 6sensor[%d] now active.\n", aux->active_sim6sensor);
}

/************************************************************/
static void _SpacetecSensorSet0Callback(vrInputDevice *devinfo, int value)
{	_SpacetecSensorSetCallback(devinfo, 0); }

/************************************************************/
static void _SpacetecSensorSet1Callback(vrInputDevice *devinfo, int value)
{	_SpacetecSensorSetCallback(devinfo, 1); }

/************************************************************/
static void _SpacetecSensorSet2Callback(vrInputDevice *devinfo, int value)
{	_SpacetecSensorSetCallback(devinfo, 2); }

/************************************************************/
static void _SpacetecSensorSet3Callback(vrInputDevice *devinfo, int value)
{	_SpacetecSensorSetCallback(devinfo, 3); }

/************************************************************/
static void _SpacetecSensorSet4Callback(vrInputDevice *devinfo, int value)
{	_SpacetecSensorSetCallback(devinfo, 4); }

/************************************************************/
static void _SpacetecSensorSet5Callback(vrInputDevice *devinfo, int value)
{	_SpacetecSensorSetCallback(devinfo, 5); }

/************************************************************/
static void _SpacetecSensorSet6Callback(vrInputDevice *devinfo, int value)
{	_SpacetecSensorSetCallback(devinfo, 6); }

/************************************************************/
static void _SpacetecSensorSet7Callback(vrInputDevice *devinfo, int value)
{	_SpacetecSensorSetCallback(devinfo, 7); }

/************************************************************/
static void _SpacetecSensorSet8Callback(vrInputDevice *devinfo, int value)
{	_SpacetecSensorSetCallback(devinfo, 8); }

/************************************************************/
static void _SpacetecSensorSet9Callback(vrInputDevice *devinfo, int value)
{	_SpacetecSensorSetCallback(devinfo, 9); }

/************************************************************/
static void _SpacetecSensorResetCallback(vrInputDevice *devinfo, int value)
{
static	vrMatrix		idmat = { { 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0 } };
	_SpacetecPrivateInfo	*aux = (_SpacetecPrivateInfo *)devinfo->aux_data;
	vr6sensor		*sensor;

	if (value == 0)
		return;

	if (aux->device == SPACETEC_BALL && !aux->silent)
		vrSerialWriteString(aux->fd, BallLongBeepMsg);

	sensor = &(devinfo->sensor6[aux->active_sim6sensor]);
	vrAssign6sensorValue(sensor, &idmat, 0 /* , timestamp */);
	vrAssign6sensorActiveValue(sensor, -1);
	vrAssign6sensorErrorValue(sensor, 0);
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Orb Callback: reset 6sensor[%d].\n", aux->active_sim6sensor);
}

/************************************************************/
static void _SpacetecSensorResetAllCallback(vrInputDevice *devinfo, int value)
{
static	vrMatrix		idmat = { { 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0 } };
	_SpacetecPrivateInfo	*aux = (_SpacetecPrivateInfo *)devinfo->aux_data;
	vr6sensor		*sensor;
	int			count;

	if (value == 0)
		return;

	if (aux->device == SPACETEC_BALL && !aux->silent)
		vrSerialWriteString(aux->fd, BallLongBeepMsg);

	for (count = 0; count < MAX_6SENSORS; count++) {
		if (aux->sensor6_inputs[count] != NULL) {
			sensor = &(devinfo->sensor6[count]);
			vrAssign6sensorValue(sensor, &idmat, 0 /* , timestamp */);
			vrAssign6sensorActiveValue(sensor, (count == aux->active_sim6sensor));
			vrAssign6sensorErrorValue(sensor, 0);
			vrDbgPrintfN(SELFCTRL_DBGLVL, "Orb Callback: reset 6sensor[%d].\n", count);
		}
	}
}

/************************************************************/
static void _SpacetecTempValuatorOverrideCallback(vrInputDevice *devinfo, int value)
{
	_SpacetecPrivateInfo	*aux = (_SpacetecPrivateInfo *)devinfo->aux_data;

	if (aux->device == SPACETEC_BALL && !aux->silent)
		vrSerialWriteString(aux->fd, BallLongBeepMsg);

	aux->sensor6_options.ignore_trans = value;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Orb Callback: ignore_trans = %d.\n",
		aux->sensor6_options.ignore_trans);
}

/************************************************************/
static void _SpacetecToggleValuatorOverrideCallback(vrInputDevice *devinfo, int value)
{
	_SpacetecPrivateInfo	*aux = (_SpacetecPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	if (aux->device == SPACETEC_BALL && !aux->silent)
		vrSerialWriteString(aux->fd, BallLongBeepMsg);

	aux->sensor6_options.ignore_trans ^= 1;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Orb Callback: ignore_trans = %d.\n",
		aux->sensor6_options.ignore_trans);
}

/************************************************************/
static void _SpacetecTempValuatorOnlyCallback(vrInputDevice *devinfo, int value)
{
	_SpacetecPrivateInfo	*aux = (_SpacetecPrivateInfo *)devinfo->aux_data;

	if (aux->device == SPACETEC_BALL && !aux->silent)
		vrSerialWriteString(aux->fd, BallLongBeepMsg);

	aux->sensor6_options.ignore_all = value;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Orb Callback: ignore_all = %d.\n",
		aux->sensor6_options.ignore_all);
}

/************************************************************/
static void _SpacetecToggleValuatorOnlyCallback(vrInputDevice *devinfo, int value)
{
	_SpacetecPrivateInfo	*aux = (_SpacetecPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	if (aux->device == SPACETEC_BALL && !aux->silent)
		vrSerialWriteString(aux->fd, BallLongBeepMsg);

	aux->sensor6_options.ignore_all ^= 1;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Orb Callback: ignore_all = %d.\n",
		aux->sensor6_options.ignore_all);
}

/************************************************************/
static void _SpacetecToggleRelativeAxesCallback(vrInputDevice *devinfo, int value)
{
	_SpacetecPrivateInfo	*aux = (_SpacetecPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	if (aux->device == SPACETEC_BALL && !aux->silent)
		vrSerialWriteString(aux->fd, BallLongBeepMsg);

	aux->sensor6_options.relative_axis ^= 1;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Orb Callback: relative_axis = %d.\n",
		aux->sensor6_options.relative_axis);
}

/************************************************************/
static void _SpacetecToggleRestrictSpaceCallback(vrInputDevice *devinfo, int value)
{
	_SpacetecPrivateInfo	*aux = (_SpacetecPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	if (aux->device == SPACETEC_BALL && !aux->silent)
		vrSerialWriteString(aux->fd, BallLongBeepMsg);

	aux->sensor6_options.restrict_space ^= 1;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Orb Callback: restrict_space = %d.\n",
		aux->sensor6_options.restrict_space);
}

/************************************************************/
static void _SpacetecToggleReturnToZeroCallback(vrInputDevice *devinfo, int value)
{
	_SpacetecPrivateInfo	*aux = (_SpacetecPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	if (aux->device == SPACETEC_BALL && !aux->silent)
		vrSerialWriteString(aux->fd, BallLongBeepMsg);

	aux->sensor6_options.return_to_zero ^= 1;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Orb Callback: return_to_zero = %d.\n",
		aux->sensor6_options.return_to_zero);
}

/************************************************************/
static void _SpacetecToggleUseNullRegionCallback(vrInputDevice *devinfo, int value)
{
	_SpacetecPrivateInfo	*aux = (_SpacetecPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	if (aux->device == SPACETEC_BALL && !aux->silent)
		vrSerialWriteString(aux->fd, BallLongBeepMsg);

	aux->usenullregion ^= 1;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Orb Callback: usenullregion = %d.\n", aux->usenullregion);
}

/************************************************************/
static void _SpacetecLongBeepCallback(vrInputDevice *devinfo, int value)
{
	_SpacetecPrivateInfo	*aux = (_SpacetecPrivateInfo *)devinfo->aux_data;

	if (aux->device == SPACETEC_BALL && !aux->silent)
		vrSerialWriteString(aux->fd, BallLongBeepMsg);
}



	/************************************************************/
	/*    Callbacks for interfacing with the SpaceTec device.   */
	/*                                                          */


/************************************************************/
static void _SpacetecCreateFunction(vrInputDevice *devinfo)
{
	/*** List of possible inputs ***/
static	vrInputFunction	_SpacetecInputs[] = {
				{ "button", VRINPUT_2WAY, _SpacetecButtonInput },
				{ "orb", VRINPUT_VALUATOR, _SpacetecOrbInput },
				{ "ball", VRINPUT_VALUATOR, _SpacetecOrbInput },
				{ "6dof", VRINPUT_VALUATOR, _SpacetecOrbInput },
				{ "6-dof", VRINPUT_VALUATOR, _SpacetecOrbInput },
				{ "6dof", VRINPUT_6SENSOR, _Spacetec6DofInput },
				{ "6-dof", VRINPUT_6SENSOR, _Spacetec6DofInput },
				{ NULL, VRINPUT_UNKNOWN, NULL } };
	/*** List of control functions ***/
static	vrControlFunc _SpacetecControlList[] = {
				/* overall system controls */
				{ "system_pause_toggle", _SpacetecSystemPauseToggleCallback },

				/* informational output controls */
				{ "print_context", _SpacetecPrintContextStructCallback },
				{ "print_config", _SpacetecPrintConfigStructCallback },
				{ "print_input", _SpacetecPrintInputStructCallback },
				{ "print_struct", _SpacetecPrintStructCallback },
				{ "print_help", _SpacetecPrintHelpCallback },

				/* simulated 6-sensor selection controls */
				{ "sensor_next", _SpacetecSensorNextCallback },
				{ "setsensor", _SpacetecSensorSetCallback },	/* NOTE: this is non-boolean */
				{ "setsensor(0)", _SpacetecSensorSet0Callback },
				{ "setsensor(1)", _SpacetecSensorSet1Callback },
				{ "setsensor(2)", _SpacetecSensorSet2Callback },
				{ "setsensor(3)", _SpacetecSensorSet3Callback },
				{ "setsensor(4)", _SpacetecSensorSet4Callback },
				{ "setsensor(5)", _SpacetecSensorSet5Callback },
				{ "setsensor(6)", _SpacetecSensorSet6Callback },
				{ "setsensor(7)", _SpacetecSensorSet7Callback },
				{ "setsensor(8)", _SpacetecSensorSet8Callback },
				{ "setsensor(9)", _SpacetecSensorSet9Callback },
				{ "sensor_reset", _SpacetecSensorResetCallback },
				{ "sensor_resetall", _SpacetecSensorResetAllCallback },

				/* simulated 6-sensor manipulation controls */
				{ "temp_valuator", _SpacetecTempValuatorOverrideCallback },
				{ "toggle_valuator", _SpacetecToggleValuatorOverrideCallback },
				{ "temp_valuator_only", _SpacetecTempValuatorOnlyCallback },
				{ "toggle_valuator_only", _SpacetecToggleValuatorOnlyCallback },
				{ "toggle_relative", _SpacetecToggleRelativeAxesCallback },
				{ "toggle_space_limit", _SpacetecToggleRestrictSpaceCallback },
				{ "toggle_return_to_zero", _SpacetecToggleReturnToZeroCallback },
				{ "toggle_use_null_region", _SpacetecToggleUseNullRegionCallback },

				/* other controls */
				{ "beep", _SpacetecLongBeepCallback },

				/* end of the list */
				{ NULL, NULL } };

	_SpacetecPrivateInfo	*aux = NULL;

#if 0 /* I don't think we want to do this on a per device basis -- s/b per process */
	signal(SIGPIPE, _SpacetecSigpipeHandler);
#endif

	/******************************************/
	/* allocate and initialize auxiliary data */
	devinfo->aux_data = (void *)vrShmemAlloc0(sizeof(_SpacetecPrivateInfo));
	aux = (_SpacetecPrivateInfo *)devinfo->aux_data;
	_SpacetecInitializeStruct(aux, devinfo->type);

	/******************/
	/* handle options */
	aux->port = vrShmemStrDup("/dev/input/spacetec");/* default, if no port given */
	aux->baud_int = 9600;				 /* default, if no baud given */
	aux->baud_enum = vrSerialBaudIntToEnum(aux->baud_int);
	_SpacetecParseArgs(aux, devinfo->args);

	/***************************************/
	/* create the inputs and self-controls */
	vrInputCreateDataContainers(devinfo, _SpacetecInputs);
	vrInputCreateSelfControlContainers(devinfo, _SpacetecInputs, _SpacetecControlList);
	/* TODO: anything to do for NON self-controls?  Implement for Magellan first. */

	/* set "active" flag for the active sensor */
	if (aux->sensor6_inputs[aux->active_sim6sensor] != NULL) {
		vrAssign6sensorActiveValue(((vr6sensor *)aux->sensor6_inputs[aux->active_sim6sensor]), 1);
	}

	if (vrDbgDo(INPUT_DBGLVL)) {
		int	count;
		for (count = 0; count < MAX_BUTTONS; count++)
			vrPrintf(BOLD_TEXT "button[%d] operation = %#p\n" NORM_TEXT, count, aux->button_inputs[count]);
	}

	vrDbgPrintf("Done creating SpaceTec inputs for '%s'\n", devinfo->name);
	devinfo->created = 1;

	return;
}


/************************************************************/
static void _SpacetecOpenFunction(vrInputDevice *devinfo)
{
	_SpacetecPrivateInfo	*aux = (_SpacetecPrivateInfo *)devinfo->aux_data;

	vrTrace("_SpacetecOpenFunction", devinfo->name);

	/*******************/
	/* open the device */
	vrDbgPrintfN(INPUT_DBGLVL, "_SpacetecOpenFunction(): SpaceTec device port is \"%s\"\n", aux->port);
	aux->fd = _SpacetecOpen(aux, aux->port, aux->baud_enum);
	if (aux->fd < 0) {
		aux->open = 0;
		vrErrPrintf("(%s::_SpacetecOpenFunction::%d) error: "
			"couldn't open serial port %s for %s\n",
			__FILE__, __LINE__, aux->port, devinfo->name);
		sprintf(aux->version, "- unconnected SpaceTec device -");
	} else {
		aux->open = 1;

		/* TODO: move the call to _SpacetecInitializeDevice() from _SpacetecOpen() to here */

		/* NOTE: for the SpaceTec devices, it is assumed that if we successfully */
		/*   open a connection that we will successfully initialize the device.  */
	}
	devinfo->version = aux->version;

	vrDbgPrintf("_SpacetecOpenFunction(): Done opening SpaceTec device for input device '%s'\n", devinfo->name);
	devinfo->operating = aux->open;

	return;
}


/************************************************************/
static void _SpacetecCloseFunction(vrInputDevice *devinfo)
{
	_SpacetecPrivateInfo	*aux = (_SpacetecPrivateInfo *)devinfo->aux_data;

	if (aux != NULL) {
		_SpacetecClose(aux);
		vrShmemFree(aux);	/* aka devinfo->aux_data */
	}

	return;
}


/************************************************************/
static void _SpacetecResetFunction(vrInputDevice *devinfo)
{
	/* TODO: reset code */
	return;
}


/************************************************************/
static void _SpacetecPollFunction(vrInputDevice *devinfo)
{
	if (devinfo->operating) {
		_SpacetecGetData(devinfo);
	} else {
		/* TODO: try to open the device again */
	}

	return;
}



	/******************************/
	/*** FreeVR public routines ***/
	/******************************/


/******************************************************/
void vrSpacetecInitInfo(vrInputDevice *devinfo)
{
	devinfo->version = (char *)vrShmemStrDup("-get from SpaceTec device-");
	devinfo->Create = vrCallbackCreateNamed("Spacetec:Create-Def", _SpacetecCreateFunction, 1, devinfo);
	devinfo->Open = vrCallbackCreateNamed("Spacetec:Open-Def", _SpacetecOpenFunction, 1, devinfo);
	devinfo->Close = vrCallbackCreateNamed("Spacetec:Close-Def", _SpacetecCloseFunction, 1, devinfo);
	devinfo->Reset = vrCallbackCreateNamed("Spacetec:Reset-Def", _SpacetecResetFunction, 1, devinfo);
	devinfo->PollData = vrCallbackCreateNamed("Spacetec:PollData-Def", _SpacetecPollFunction, 1, devinfo);
	devinfo->PrintAux = vrCallbackCreateNamed("Spacetec:PrintAux-Def", _SpacetecPrintStruct, 0);

	vrDbgPrintfN(INPUT_DBGLVL, "vrSpacetecInitInfo: callbacks created.\n");
}


#endif /* } FREEVR */



	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/



#if defined(TEST_APP) /* { */

/*******************************************************************/
/* A test program to communicate with a SpaceTec device and print the results. */
main(int argc, char *argv[])
{
	int			numevents;
	_SpacetecPrivateInfo	*aux;


	/******************************/
	/* setup the device structure */
	aux = (_SpacetecPrivateInfo *)malloc(sizeof(_SpacetecPrivateInfo));
	memset(aux, 0, sizeof(_SpacetecPrivateInfo));
	if (argv[0][0] == 'b')
		_SpacetecInitializeStruct(aux, "spaceball");	/* balltest */
	else	_SpacetecInitializeStruct(aux, "spaceorb");	/* orbtest */


	/*********************************************************/
	/* set default parameters based on environment variables */
	/* TODO: use of environment variables */
	aux->port = getenv("SPACETEC_TTY");
	if (aux->port == NULL)
		aux->port = "/dev/input/spacetec";	/* default, if no envvar */
	aux->baud_int = 9600;				/* default, if no baud given */
	aux->baud_enum = vrSerialBaudIntToEnum(aux->baud_int);


	/*****************************************************/
	/* adjust parameters based on command-line-arguments */
	/* TODO: parse CLAs for non-default serial port, baud, etc. */


	/**************************************************/
	/* open the serial port and initialize the device */
	aux->fd = _SpacetecOpen(aux, aux->port, aux->baud_enum);
	if (aux->fd < 0) {
		fprintf(stderr, "error: couldn't open serial port %s at %d baud.\n", aux->port, aux->baud_enum);
		exit (1);
	}
	printf(RED_TEXT "SpaceTec device open!\n\tversion = '%s'\n\tparams = '%s'\n\n" NORM_TEXT, aux->version, aux->op_params);

	/* TODO: after the call to _SpacetecInitializeDevice() is moved, will have to call it here */


	/**********************/
	/* display the output */
	while (aux->buttons != 0x41) {
		numevents = _SpacetecReadInput(aux);
		if (numevents > 0) {
			printf("tx:%6d ty:%6d tz:%6d rx:%6d ry:%6d rz:%6d  buttons:0x%03x",
				aux->offset[VR_TX], aux->offset[VR_TY], aux->offset[VR_TZ],
				aux->offset[VR_RX], aux->offset[VR_RY], aux->offset[VR_RZ],
				aux->buttons);
			if (getenv("SHOW_BAD_PACKETS"))
				printf("  DKO:%3d,%3d,%4d", aux->bad_Dpackets, aux->bad_Kpackets, aux->bad_other_packets);
			printf("    \r");
			fflush(stdout);
		}
	}


	/*****************/
	/* close up shop */
	if (aux != NULL) {
		vrSerialClose(aux->fd);
		free(aux);
	}
	printf("\nSpaceTec device closed\n");
}

#endif /* } TEST_APP */



	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/



#if defined(CAVE) /* { */

/*****************************************************************************/
/*** public functions for accessing the SpaceTec SpaceOrb as a             ***/
/***   tracking device:                                                    ***/
/***     void CAVEInitSpaceorbTracking(CAVE_ST *cave)                      ***/
/***     void CAVEInitSpaceballTracking(CAVE_ST *cave)                     ***/
/***     void CAVEResetSpaceorbTracking(CAVE_ST *cave)                     ***/
/***     void CAVEResetSpaceballTracking(CAVE_ST *cave)                    ***/
/***     int CAVEGetSpacetecTracking(CAVE_ST *cave, CAVE_SENSOR_ST *sensor)***/
/***     void CAVESetSpacetecModes(int mode, float sensitivity)            ***/
/***     -------                                                           ***/
/***     void CAVEReadSpacetecController(CAVE_ST *, CAVE_CONTROLLER_ST *)  ***/
/***     int CAVENumSpacetecButtons(CAVE_ST *cave)                         ***/
/***     int CAVENumSpacetecValuators(CAVE_ST *cave)                       ***/
/*****************************************************************************/

/*** routines from other CAVE files ***/
void	CAVEPreRotMatrix(Matrix mat,float angle,char axis);

/*** routines from this file ***/
void CAVESetSpacetecParameter(int parameter, float setting);


/*** local variables ***/
static	_SpacetecPrivateInfo	orb[1];

static _SpacetecPrivateInfo	*orb_tracker = NULL;
static _SpacetecPrivateInfo	*orb_controller = NULL;


/*****************************************************************/
void CAVEInitSpaceorbTracking(CAVE_ST *cave)
{
	_SpacetecPrivateInfo	*orb = orb_tracker;

	/* the orb is already open, then just return    */
	/*   this will happen when the controller tries */
	/*   to open the orb after the tracker already  */
	/*   has done so.                               */
	if (orb != NULL)
		return;

	if (!cave->config->TrackerSerialPort[0]) {
		cave->config->TrackerSerialPort[0] = "/dev/input/spaceorb";
		fprintf(stderr, "CAVE WARNING (CAVEInitSpaceorbTracking): "
			"Tracker serial port not specified"
			"trying %s.\n", cave->config->TrackerSerialPort[0]);
	}

#if 1
	if (cave->config->TrackerBaudRate != vrSerialBaudIntToEnum(9600)) {
		fprintf(stderr, "CAVE WARNING (CAVEInitSpaceorbTracking): "
			"Spaceorbs default to 9600 Baud.");
		cave->config->TrackerBaudRate = vrSerialBaudIntToEnum(9600);
	}
#endif

	orb = (_SpacetecPrivateInfo *)malloc(sizeof(_SpacetecPrivateInfo));
	orb_tracker = orb;
	_SpacetecInitializeStruct(orb, "spaceorb");
	orb->fd = _SpacetecOpen(orb, cave->config->TrackerSerialPort[0], cave->config->TrackerBaudRate);

	if (orb == NULL || orb->fd < 0) {
		fprintf(stderr, "CAVE ERROR (CAVEInitSpaceorbTracking): "
			"failed to open the serial port\n");
		exit(-1);
	}

	if (cave->config->SixDOFInitMode >= 0)
		CAVESetSpacetecParameter(SIXDOF_USE, cave->config->SixDOFInitMode);
	CAVESetSpacetecParameter(SIXDOF_TGAIN, cave->config->SixDOFInitTransSensitivity);
	CAVESetSpacetecParameter(SIXDOF_RGAIN, cave->config->SixDOFInitRotSensitivity);
	CAVESetSpacetecParameter(SIXDOF_VGAIN, cave->config->SixDOFInitValSensitivity);

	fprintf(stderr, "CAVE tracker (spaceorb): %s\n", orb->version);
}


/*****************************************************************/
void CAVEInitSpaceballTracking(CAVE_ST *cave)
{
	_SpacetecPrivateInfo	*orb = orb_tracker;

	/* the ball is already open, then just return   */
	/*   this will happen when the controller tries */
	/*   to open the ball after the tracker already */
	/*   has done so.                               */
	if (orb != NULL)
		return;

	if (!cave->config->TrackerSerialPort[0]) {
		cave->config->TrackerSerialPort[0] = "/dev/input/spaceball";
		fprintf(stderr, "CAVE WARNING (CAVEInitSpaceballTracking): "
			"Tracker serial port not specified"
			"trying %s.\n", cave->config->TrackerSerialPort[0]);
	}

#if 1
	if (cave->config->TrackerBaudRate != vrSerialBaudIntToEnum(9600)) {
		fprintf(stderr, "CAVE WARNING (CAVEInitSpaceballTracking): "
			"Spaceballs default to 9600 Baud.");
		cave->config->TrackerBaudRate = vrSerialBaudIntToEnum(9600);
	}
#endif

	orb = (_SpacetecPrivateInfo *)malloc(sizeof(_SpacetecPrivateInfo));
	orb_tracker = orb;
	_SpacetecInitializeStruct(orb, "spaceball");
	orb->fd = _SpacetecOpen(orb, cave->config->TrackerSerialPort[0], cave->config->TrackerBaudRate);

	if (orb == NULL || orb->fd < 0) {
		fprintf(stderr, "CAVE ERROR (CAVEInitSpaceballTracking): "
			"failed to open the serial port\n");
		exit(-1);
	}

	if (cave->config->SixDOFInitMode >= 0)
		CAVESetSpacetecParameter(SIXDOF_USE, cave->config->SixDOFInitMode);
	CAVESetSpacetecParameter(SIXDOF_TGAIN, cave->config->SixDOFInitTransSensitivity);
	CAVESetSpacetecParameter(SIXDOF_RGAIN, cave->config->SixDOFInitRotSensitivity);
	CAVESetSpacetecParameter(SIXDOF_VGAIN, cave->config->SixDOFInitValSensitivity);

	fprintf(stderr, "CAVE tracker (spaceball): %s\n", orb->version);
}


/*****************************************************************/
void CAVEInitSpaceorbController(CAVE_ST *cave)
{
	_SpacetecPrivateInfo	*orb = orb_controller;

	/* the orb is already open, then just return    */
	/*   this will happen when the controller tries */
	/*   to open the orb after the tracker already  */
	/*   has done so.                               */
	if (orb_tracker != NULL && CAVESameFile(cave->config->TrackerSerialPort[0], cave->config->ControllerSerialPort)) {
		orb_controller = orb_tracker;
		return;
	}

	if (!cave->config->ControllerSerialPort) {
		cave->config->ControllerSerialPort = "/dev/input/spaceorb";
		fprintf(stderr, "CAVE WARNING (CAVEInitSpaceorbController): "
			"Controller serial port not specified"
			"trying %s.\n", cave->config->ControllerSerialPort);
	}

#if 1
	if (cave->config->ControllerBaudRate != vrSerialBaudIntToEnum(9600)) {
		fprintf(stderr, "CAVE WARNING (CAVEInitSpaceorbController): "
			"Spaceorbs default to 9600 Baud.");
		cave->config->ControllerBaudRate = vrSerialBaudIntToEnum(9600);
	}
#endif

	_SpacetecInitializeStruct(orb, "spaceorb");
	orb->fd = _SpacetecOpen(orb, cave->config->ControllerSerialPort, cave->config->ControllerBaudRate);
	orb_controller = orb;

	if (orb == NULL || orb->fd < 0) {
		fprintf(stderr, "CAVE ERROR (CAVEInitSpaceorbController): "
			"failed to open the serial port\n");
		exit(-1);
	}

	if (cave->config->SixDOFInitMode >= 0)
		CAVESetSpacetecParameter(SIXDOF_USE, cave->config->SixDOFInitMode);
	CAVESetSpacetecParameter(SIXDOF_TGAIN, cave->config->SixDOFInitTransSensitivity);
	CAVESetSpacetecParameter(SIXDOF_RGAIN, cave->config->SixDOFInitRotSensitivity);
	CAVESetSpacetecParameter(SIXDOF_VGAIN, cave->config->SixDOFInitValSensitivity);

	fprintf(stderr, "CAVE tracker (spaceorb controller): %s\n", orb->version);
}


/*****************************************************************/
void CAVEInitSpaceballController(CAVE_ST *cave)
{
	_SpacetecPrivateInfo	*orb = orb_controller;

	/* the ball is already open, then just return   */
	/*   this will happen when the controller tries */
	/*   to open the ball after the tracker already */
	/*   has done so.                               */
	if (orb_tracker != NULL && CAVESameFile(cave->config->TrackerSerialPort[0], cave->config->ControllerSerialPort)) {
		orb_controller = orb_tracker;
		return;
	}

	if (!cave->config->ControllerSerialPort) {
		cave->config->ControllerSerialPort = "/dev/input/spaceball";
		fprintf(stderr, "CAVE WARNING (CAVEInitSpaceballController): "
			"Controller serial port not specified"
			"trying %s.\n", cave->config->ControllerSerialPort);
	}

#if 1
	if (cave->config->ControllerBaudRate != vrSerialBaudIntToEnum(9600)) {
		fprintf(stderr, "CAVE WARNING (CAVEInitSpaceballController): "
			"Spaceballs default to 9600 Baud.");
		cave->config->ControllerBaudRate = vrSerialBaudIntToEnum(9600);
	}
#endif

	_SpacetecInitializeStruct(orb, "spaceball");
	orb->fd = _SpacetecOpen(orb, cave->config->ControllerSerialPort, cave->config->ControllerBaudRate);
	orb_controller = orb;

	if (orb == NULL || orb->fd < 0) {
		fprintf(stderr, "CAVE ERROR (CAVEInitSpaceballController): "
			"failed to open the serial port\n");
		exit(-1);
	}

	if (cave->config->SixDOFInitMode >= 0)
		CAVESetSpacetecParameter(SIXDOF_USE, cave->config->SixDOFInitMode);
	CAVESetSpacetecParameter(SIXDOF_TGAIN, cave->config->SixDOFInitTransSensitivity);
	CAVESetSpacetecParameter(SIXDOF_RGAIN, cave->config->SixDOFInitRotSensitivity);
	CAVESetSpacetecParameter(SIXDOF_VGAIN, cave->config->SixDOFInitValSensitivity);

	fprintf(stderr, "CAVE tracker (spaceball controller): %s\n", orb->version);
}


/*****************************************************************/
int CAVENumSpacetecButtons(CAVE_ST *cave)
{
	return 3;
}


/*****************************************************************/
int CAVENumSpacetecValuators(CAVE_ST *cave)
{
	return SIXDOF_NVALUATORS;
}


/*****************************************************************/
void CAVEResetSpaceorbTracking(CAVE_ST *cave)
{
	_SpacetecPrivateInfo	*orb = orb_tracker;

	_SpacetecInitializeStruct(orb, "spaceorb");

	vrSleep(20000);
}


/*****************************************************************/
void CAVEResetSpaceballTracking(CAVE_ST *cave)
{
	_SpacetecPrivateInfo	*orb = orb_tracker;

	_SpacetecInitializeStruct(orb, "spaceball");

	vrSleep(20000);
}


/*****************************************************************/
void CAVESetSpacetecParameter(int parameter, float setting)
{
	int		int_setting = setting;

	switch (parameter) {
	case SIXDOF_MODE:
		fprintf(stderr, "CAVESetSpacetecParameter(): can't set the mode on the Spaceorb by software.\n");
		break;

	case SIXDOF_USE:
		/* interpret the usage settings */
		if (orb_tracker) {
			orb_tracker->valuator_override = ((int_setting & SIXDOF_UVAL) ? 1 : 0);
			orb_tracker->mixed_valuator_sensor = ((int_setting & SIXDOF_UMIXED) ? 1 : 0);
			if (int_setting & SIXDOF_UHEAD)
				orb_tracker->active_sim6sensor = &orb_tracker->head;
			if (int_setting & SIXDOF_UWAND)
				orb_tracker->active_sim6sensor = &orb_tracker->wand1;
			if (int_setting & SIXDOF_UWAND2)
				orb_tracker->active_sim6sensor = &orb_tracker->wand2;
			orb_tracker->restrict_space = ((int_setting & SIXDOF_UCLAMP) ? 1 : 0);
			orb_tracker->relative_axis = ((int_setting & SIXDOF_URELATIVE) ? 1 : 0);
			orb_tracker->return_to_zero = ((int_setting & SIXDOF_UREZERO) ? 1 : 0);
		}
		break;

	case SIXDOF_NULL:
		if (orb_tracker) {
			orb_tracker->null_offset[VR_TX] = setting;
			orb_tracker->null_offset[VR_TY] = setting;
			orb_tracker->null_offset[VR_TZ] = setting;
			orb_tracker->null_offset[VR_RX] = setting;
			orb_tracker->null_offset[VR_RY] = setting;
			orb_tracker->null_offset[VR_RZ] = setting;
		}
		if (orb_controller) {
			orb_controller->null_offset[VR_TX] = setting;
			orb_controller->null_offset[VR_TY] = setting;
			orb_controller->null_offset[VR_TZ] = setting;
			orb_controller->null_offset[VR_RX] = setting;
			orb_controller->null_offset[VR_RY] = setting;
			orb_controller->null_offset[VR_RZ] = setting;
		}
		break;

	case SIXDOF_RGAIN:
		if (orb_tracker) {
			orb_tracker->scale_rot = ROT_SENSITIVITY * setting;
		}
		if (orb_controller) {
			orb_controller->scale_rot = ROT_SENSITIVITY * setting;
		}
		break;

	case SIXDOF_TGAIN:
		if (orb_tracker) {
			orb_tracker->scale_trans = TRANS_SENSITIVITY * setting;
		}
		if (orb_controller) {
			orb_controller->scale_trans = TRANS_SENSITIVITY * setting;
		}
		break;

	case SIXDOF_VGAIN:
		if (orb_tracker) {
			orb_tracker->scale_valuator = VALUATOR_SENSITIVITY * setting;
		}
		if (orb_controller) {
			orb_controller->scale_valuator = VALUATOR_SENSITIVITY * setting;
		}
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
		fprintf(stderr, "CAVESetSpacetecParameter(): this parameter is not modifiable.\n");
		break;

	default:
		fprintf(stderr, "CAVESetSpacetecParameter(): No such parameter (%d).\n",parameter);
		break;
	}
}


#define RTOD(d) ((d)*57.295788f)
/*****************************************************************/
int CAVEGetSpacetecTracking(CAVE_ST *cave, CAVE_SENSOR_ST *sensor)
{
	_SpacetecPrivateInfo	*orb = orb_tracker;
	int		numevents;
static	int		buttons_last = 0;
	int		tmp_valuator_override;
	CAVE_SENSOR_ST	sensor_delta;
	float		vector[3];
	Matrix		mat = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

	numevents = _SpacetecReadInput(orb);

	/* If button B is pressed, then activate valuator mode while the */
	/*    button is pressed.                                         */
	if (orb->device == SPACETEC_ORB)
		tmp_valuator_override = (orb->buttons & ORB_BUTTON_B);
	else	tmp_valuator_override = (orb->buttons & BALL_BUTTON_PICK);

	sensor_delta.x = orb->offset[VR_TX] * orb->scale_trans;
	sensor_delta.y = orb->offset[VR_TY] * orb->scale_trans;
	sensor_delta.z = orb->offset[VR_TZ] * orb->scale_trans;

	sensor_delta.elev = orb->offset[VR_RX] * orb->scale_rot;
	sensor_delta.azim = orb->offset[VR_RY] * orb->scale_rot;
	sensor_delta.roll = orb->offset[VR_RZ] * orb->scale_rot;

	CAVEGetTimestamp(&sensor_delta.timestamp);

	orb->active_sim6sensor->timestamp = sensor_delta.timestamp;

	if (orb->return_to_zero) {
		*orb->active_sim6sensor = initial_cave_sensor;
		if (orb->active_sim6sensor == &(orb->wand1)) {
			orb->active_sim6sensor->y -= 1.0;
			orb->active_sim6sensor->z -= 1.5;
		}
	}

	/***************************************************/
	/*** handle the sensor translation (or valuator) ***/
	/***************************************************/
	if (orb->valuator_override || tmp_valuator_override) {
		/* make orb translation do valuator controls */
		orb->controls.valuator[SIXDOF_JOYX] = ( orb->offset[VR_TX] - 0.9*orb->offset[VR_RZ]) * orb->scale_valuator;
		orb->controls.valuator[SIXDOF_JOYY] = (-orb->offset[VR_TZ] - 1.2*orb->offset[VR_RX]) * orb->scale_valuator;
		orb->controls.valuator[SIXDOF_TX] = orb->offset[VR_TX] * orb->scale_valuator;
		orb->controls.valuator[SIXDOF_TY] = orb->offset[VR_TY] * orb->scale_valuator;
		orb->controls.valuator[SIXDOF_TZ] = orb->offset[VR_TZ] * orb->scale_valuator;
		orb->controls.valuator[SIXDOF_RX] = orb->offset[VR_RX] * orb->scale_valuator;
		orb->controls.valuator[SIXDOF_RY] = orb->offset[VR_RY] * orb->scale_valuator;
		orb->controls.valuator[SIXDOF_RZ] = orb->offset[VR_RZ] * orb->scale_valuator;
	} else {

		/* Not overridden.  Translate and rotate the active sensor */
		if (orb->relative_axis) {
			float cos_azim = cosf(DTOR(orb->head.azim));
			float sin_azim = sinf(DTOR(orb->head.azim));
			float cos_elev = cosf(DTOR(orb->head.elev));
			float sin_elev = sinf(DTOR(orb->head.elev));
			float cos_roll = cosf(DTOR(orb->head.roll));
			float sin_roll = sinf(DTOR(orb->head.roll));

			/* translate relative to current orientation.     */
			/* There is a choice of which orientation to move */
			/* in relation to: the manipulated sensor, or the */
			/* viewing sensor.                                */
#if 0
			/* move relative to manipulated sensor */
			CAVEGetSensorVector(orb->active_sim6sensor, CAVE_RIGHT, vector);
#endif

#if 0
			/* move relative to viewing (ie. head) sensor */
			CAVEGetSensorVector(&orb->head, CAVE_RIGHT, vector);
#else
			/* This mimics the non ZUP_COORD version of CAVEGetSensorVector..*/
			/*   (&orb->head,CAVE_RIGHT,vector), because the ZUP_COORDS stuff*/
			/*   in that is inappropriate for this circumstance.             */
			vector[0] =  cos_azim * cos_roll + sin_azim * sin_elev * sin_roll;
			vector[1] =  cos_elev * sin_roll;
			vector[2] = -sin_azim * cos_roll + cos_azim * sin_elev * sin_roll;
#endif
			orb->active_sim6sensor->x += sensor_delta.x * vector[0];
			orb->active_sim6sensor->y += sensor_delta.x * vector[1];
			orb->active_sim6sensor->z += sensor_delta.x * vector[2];

#if 0
			/* move relative to manipulated sensor */
			CAVEGetSensorVector(orb->active_sim6sensor, CAVE_UP, vector);
#endif

#if 0
			/* move relative to viewing (ie. head) sensor */
			CAVEGetSensorVector(&orb->head, CAVE_UP, vector);
#else
			vector[0] = -cos_azim * sin_roll + sin_azim * sin_elev * cos_roll;
			vector[1] =  cos_roll * cos_elev;
			vector[2] =  sin_azim * sin_roll + cos_azim * sin_elev * cos_roll;
#endif
			orb->active_sim6sensor->x += sensor_delta.y * vector[0];
			orb->active_sim6sensor->y += sensor_delta.y * vector[1];
			orb->active_sim6sensor->z += sensor_delta.y * vector[2];

#if 0
			/* move relative to manipulated sensor */
			CAVEGetSensorVector(orb->active_sim6sensor, CAVE_BACK, vector);
#endif

#if 0
			/* move relative to viewing (ie. head) sensor */
			CAVEGetSensorVector(&orb->head, CAVE_BACK, vector);
#else
			vector[0] =  cos_elev * sin_azim;
			vector[1] = -sin_elev;
			vector[2] =  cos_elev * cos_azim;
#endif
			orb->active_sim6sensor->x += sensor_delta.z * vector[0];
			orb->active_sim6sensor->y += sensor_delta.z * vector[1];
			orb->active_sim6sensor->z += sensor_delta.z * vector[2];
		} else {
			/* Not relative_axis; do translations in CAVE space */
			orb->active_sim6sensor->x += sensor_delta.x;
			orb->active_sim6sensor->y += sensor_delta.y;
			orb->active_sim6sensor->z += sensor_delta.z;
		}

		if (orb->restrict_space) {
			/* actually, the exact values of restriction should */
			/*   come from the specific configuration.          */
			if (orb->active_sim6sensor->x >  5.0) orb->active_sim6sensor->x =  5.0;
			if (orb->active_sim6sensor->x < -5.0) orb->active_sim6sensor->x = -5.0;
#ifndef ZUP_COORDS
			if (orb->active_sim6sensor->y > 10.0) orb->active_sim6sensor->y = 10.0;
			if (orb->active_sim6sensor->z >  5.0) orb->active_sim6sensor->z =  5.0;
			if (orb->active_sim6sensor->y <  0.0) orb->active_sim6sensor->y =  0.0;
			if (orb->active_sim6sensor->z < -5.0) orb->active_sim6sensor->z = -5.0;
#else
			if (orb->active_sim6sensor->z > 10.0) orb->active_sim6sensor->z = 10.0;
			if (orb->active_sim6sensor->y >  5.0) orb->active_sim6sensor->y =  5.0;
			if (orb->active_sim6sensor->z <  0.0) orb->active_sim6sensor->z =  0.0;
			if (orb->active_sim6sensor->y < -5.0) orb->active_sim6sensor->y = -5.0;
#endif
		}
	}

	if (!(orb->valuator_override || tmp_valuator_override) || orb->mixed_valuator_sensor) {
		/**********************************/
		/*** handle the sensor rotation ***/
		/**********************************/
		if (orb->relative_axis) {
			float	azim, elev, roll;
			float	azim_s, azim_c;

#if 1
			/* This seems to work well for the head display,  */
			/* but I'm not sure if it will under ZUP_coords.  */
			/* However, it's not the greatest interface for   */
			/* controlling wand rotation -- the rotation      */
			/* should probably be relative to the head_sensor */
			/* as with the wand translation.                  */

			CAVEPreRotMatrix(mat, orb->active_sim6sensor->azim,'y');
			CAVEPreRotMatrix(mat, orb->active_sim6sensor->elev,'x');
			CAVEPreRotMatrix(mat, orb->active_sim6sensor->roll,'z');
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

			orb->active_sim6sensor->azim =  RTOD(azim);
			orb->active_sim6sensor->elev = -RTOD(elev);
			orb->active_sim6sensor->roll =  RTOD(roll);
#else
			/* This one is close, but confuses roll & elev after an azim */
			/* twist.  I reversed the matrix indices, in an attempt to   */
			/* match the formulae from "Robotics."                       */
			CAVEPreRotMatrix(mat,-orb->active_sim6sensor->roll,'z');
			CAVEPreRotMatrix(mat,-orb->active_sim6sensor->elev,'x');
			CAVEPreRotMatrix(mat,-orb->active_sim6sensor->azim,'y');
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

			orb->active_sim6sensor->azim =  RTOD(azim);
			orb->active_sim6sensor->elev = -RTOD(elev);
			orb->active_sim6sensor->roll =  RTOD(roll);
#endif

		} else {
			/* do rotations around the sensor's axes */
			orb->active_sim6sensor->azim += sensor_delta.azim;
			orb->active_sim6sensor->elev += sensor_delta.elev;
			orb->active_sim6sensor->roll += sensor_delta.roll;

		}
	}

	/*****************************************************/
	/*** handle the button toggles related to tracking ***/
	/*****************************************************/
	/* select the next sensor (hard coded to two) */
	if (orb->device == SPACETEC_ORB) {
		if ((orb->buttons & ORB_BUTTON_A) && !(buttons_last & ORB_BUTTON_A)) {
			/* choose the next sensor */
			if (orb->active_sim6sensor == &orb->head) {
				orb->active_sim6sensor = &orb->wand1;
			} else if (orb->active_sim6sensor == &orb->wand1) {
				orb->active_sim6sensor = &orb->head;
			}
		}
	} else {
		if ((orb->buttons & BALL_BUTTON_4) && !(buttons_last & BALL_BUTTON_4)) {
			/* choose the next sensor */
			if (orb->active_sim6sensor == &orb->head) {
				orb->active_sim6sensor = &orb->wand1;
			} else if (orb->active_sim6sensor == &orb->wand1) {
				orb->active_sim6sensor = &orb->head;
			}
		}
	}

	/* reset the current sensor */
	if (orb->device == SPACETEC_ORB && orb->buttons & ORB_BUTTON_RESET) {
		*orb->active_sim6sensor = initial_cave_sensor;
	}
	if (orb->device == SPACETEC_BALL && orb->buttons & BALL_BUTTON_5) {
		*orb->active_sim6sensor = initial_cave_sensor;
	}

	/* toggle relative_axis (spaceball only) */
	if (orb->device == SPACETEC_BALL && orb->buttons & BALL_BUTTON_6 && !(buttons_last & BALL_BUTTON_6)) {
		orb->relative_axis ^= 1;
	}

	/* toggle restrict_space (spaceball only) */
	if (orb->device == SPACETEC_BALL && orb->buttons & BALL_BUTTON_7 && !(buttons_last & BALL_BUTTON_7)) {
		orb->restrict_space ^= 1;
	}

	/* toggle return_to_zero (spaceball only) */
	if (orb->device == SPACETEC_BALL && orb->buttons & BALL_BUTTON_8 && !(buttons_last & BALL_BUTTON_8)) {
		orb->return_to_zero ^= 1;
	}

	/* currently no way to toggle orient_up */

	/* currently no way to toggle use_null_region */


	/***********************************************/
	/*** copy the information to the cave sensor ***/
	/***********************************************/
	sensor[0] = orb->head;
	sensor[1] = orb->wand1;
	sensor[2] = orb->wand2;


	buttons_last = orb->buttons;

	return 1;
}



/*****************************************************************/
void CAVEReadSpacetecController(CAVE_ST *cave, volatile CAVE_CONTROLLER_ST *control)
{
	_SpacetecPrivateInfo	*orb = orb_controller;
	int		numevents;
static	int		buttons_last = 0;
	CAVE_SENSOR_ST	sensor_delta;

	numevents = _SpacetecReadInput(orb);

	/*****************************************/
	/*** handle the valuator (if selected) ***/
	/*****************************************/
	if (orb->valuator_override) {
		/* make orb translation do valuator controls */
		orb->controls.valuator[SIXDOF_JOYX] = ( orb->offset[VR_TX] - 0.9*orb->offset[VR_RZ]/2) * orb->scale_valuator;
		orb->controls.valuator[SIXDOF_JOYY] = (-orb->offset[VR_TZ] - 1.2*orb->offset[VR_RX]/2) * orb->scale_valuator;
		orb->controls.valuator[SIXDOF_TX] = orb->offset[VR_TX] * orb->scale_valuator;
		orb->controls.valuator[SIXDOF_TY] = orb->offset[VR_TY] * orb->scale_valuator;
		orb->controls.valuator[SIXDOF_TZ] = orb->offset[VR_TZ] * orb->scale_valuator;
		orb->controls.valuator[SIXDOF_RX] = orb->offset[VR_RX] * orb->scale_valuator;
		orb->controls.valuator[SIXDOF_RY] = orb->offset[VR_RY] * orb->scale_valuator;
		orb->controls.valuator[SIXDOF_RZ] = orb->offset[VR_RZ] * orb->scale_valuator;
	}
	control->valuator[SIXDOF_TYPE] = SIXDOF_ORB;
	control->valuator[SIXDOF_VERS] = ORB_CURVERSION;
	control->valuator[SIXDOF_KYBD] = orb->buttons;	/* bit-encoded buttons */

	/* spaceorb doesn't have bit-encoded enables (rot, trans, dominant), so hard code them */
	control->valuator[SIXDOF_MODE] = SIXDOF_MROT | SIXDOF_MTRAN;

	control->valuator[SIXDOF_USE] =
	  ((orb->valuator_override) ? SIXDOF_UVAL : 0) |		/* 0x01 orb->valuator_override [Fkey]*/
	  ((orb->mixed_valuator_sensor) ? SIXDOF_UMIXED : 0) |		/* 0x02 orb->mixed_valuator_sensor*/
	  ((orb->active_sim6sensor == &orb->head) ? SIXDOF_UHEAD : 0) |	/* 0x04 orb->head [Akey] */
	  ((orb->active_sim6sensor == &orb->wand1) ? SIXDOF_UWAND : 0) |	/* 0x08 orb->wand [Bkey] */
	  ((orb->active_sim6sensor == &orb->wand2) ? SIXDOF_UWAND2 : 0) |	/* 0x10 orb->wand        */
	  ((orb->restrict_space) ? SIXDOF_UCLAMP : 0) |			/* 0x20 rangerestrict      */
	  ((orb->relative_axis) ? SIXDOF_URELATIVE : 0);		/* 0x40 head/wand relative */
	  ((orb->return_to_zero) ? SIXDOF_UREZERO : 0);			/* 0x80 head/wand relative */

	control->valuator[SIXDOF_NULL] = 0.0;	/* not part of a spaceorb */
	control->valuator[SIXDOF_RGAIN] = 0.0;	/* not part of a spaceorb */
	control->valuator[SIXDOF_TGAIN] = 0.0;	/* not part of a spaceorb */
	control->valuator[SIXDOF_VGAIN] = 0.0;	/* not part of a spaceorb */

	/***********************************************************/
	/*** handle the button toggles related to the controller ***/
	/***********************************************************/
	if (orb->device == SPACETEC_ORB) {
		orb->controls.button[0] = (orb->buttons & ORB_BUTTON_D);
		orb->controls.button[1] = (orb->buttons & ORB_BUTTON_C);
		orb->controls.button[2] = (orb->buttons & ORB_BUTTON_E);
	} else {
		orb->controls.button[0] = (orb->buttons & BALL_BUTTON_1);
		orb->controls.button[1] = (orb->buttons & BALL_BUTTON_2);
		orb->controls.button[2] = (orb->buttons & BALL_BUTTON_3);
	}

	/* If button F was just pressed (ie. wasn't pushed last time) then */
	/*    toggle the state of valuator_override.  (spaceorb only)      */
	if (orb->device == SPACETEC_ORB && (orb->buttons & ORB_BUTTON_F) && !(buttons_last & ORB_BUTTON_F)) {
		orb->valuator_override ^= 1;
	}

	/***************************************************/
	/*** copy the information to the cave controller ***/
	/***************************************************/
	control->button[0] = orb->controls.button[0];
	control->button[1] = orb->controls.button[1];
	control->button[2] = orb->controls.button[2];

	control->valuator[SIXDOF_JOYX] = orb->controls.valuator[SIXDOF_JOYX]; /* x transl */
	control->valuator[SIXDOF_JOYY] = orb->controls.valuator[SIXDOF_JOYY]; /* y transl */
	control->valuator[SIXDOF_TX] = orb->controls.valuator[SIXDOF_TX]; /* x transl */
	control->valuator[SIXDOF_TY] = orb->controls.valuator[SIXDOF_TY]; /* y transl */
	control->valuator[SIXDOF_TZ] = orb->controls.valuator[SIXDOF_TZ]; /* z transl */
	control->valuator[SIXDOF_RX] = orb->controls.valuator[SIXDOF_RX]; /* elev [xrot]*/
	control->valuator[SIXDOF_RY] = orb->controls.valuator[SIXDOF_RY]; /* azim [yrot]*/
	control->valuator[SIXDOF_RZ] = orb->controls.valuator[SIXDOF_RZ]; /* roll [zrot]*/

	buttons_last = orb->buttons;
}


#endif /* } CAVE */

