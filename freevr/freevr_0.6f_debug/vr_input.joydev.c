/* ======================================================================
 *
 *  CCCCC          vr_input.joydev.c
 * CC   CC         Author(s): Bill Sherman
 * CC              Created: January 17, 2003
 * CC   CC         Last Modified: September 4, 2013
 *  CCCCC
 *
 * Code file for FreeVR inputs from the Linux Joystick input interface.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/*************************************************************************

USAGE:
	This device driver currently only works on Linux systems (because
	those are where the "joydev" interface exist -- though it may also
	be available on FreeBSD and similar systems).

	To use the "joydev" interface, the "joydev" module must be loaded.
	For most modern editions of the Linux kernel, this should be handled
	automatically, but you can verify it has been loaded with:
		% lsmod | grep joydev

	And if it isn't there, attempt to load it with:
		% modprobe joydev

	To test a joystick device, you can either use the Linux tool "jstest",
	or the FreeVR tool "joytest".  Presently (2013), the "jstest" application
	is often not installed by default.  Package names differ ...
		Debian/Ubuntu% sudo apt-get install joystick
		Fedora% sudo yum install joystick

	Then, to test the device, the following command can be used (or similar
	if there is more than one USB human-interface device connected):
		% jstest --event /dev/input/js0


	The FreeVR "joytest" program is compiled from this source file with:
		% make joytest

	Then, to test the device:
		% joytest /dev/input/js0

	See the "joytest.1" man page for additional information (assuming FreeVR
	has been installed):
		% man joytest


	The FreeVR configuration of the inputs is similar to all input devices:

	Inputs are specified with the "input" option:
		input "<name>" = "2switch(button[<number>])";
		input "<name>" = "valuator(axis[<number>])";
		input "<name>" = "6sensor(sim6[<number>])";

	Controls are specified with the "control" option:
		control "<control option>" = "2switch(button[<number>])";
		control "<control option>" = "valuator(axis[<number>])";

	Here are the available (2switch) control options for FreeVR:
		"print_help" -- print info on how to use the input device
		"system_pause_toggle" -- toggle the system pause flag
		"print_context" -- print the overall FreeVR context data structure (for debugging)
		"print_config" -- print the overall FreeVR config data structure (for debugging)
		"print_input" -- print the overall FreeVR input data structure (for debugging)
		"print_struct" -- print the internal Joydev data structure (for debugging)
		"print_sim6opts" -- print the internal Joydev data for simulated 6-sensors (for debugging)

		"sensor_next" -- jump to the next 6-sensor on the list
		"setsensor(<num>)" -- set the simulated sensor to a particular one
		"sensor_reset" -- reset the current 6-sensor
		"sensor_resetall" -- reset the all the 6-sensors
		"temp_valuator" -- temporarily disable 6-sensor for valuator values
		"toggle_valuator" -- toggle valuator values instead of 6-sensor
		"temp_valuator_only" -- temporarily use translation values for valuator
		"toggle_valuator_only" -- toggle whether translation is saved for valuator
		"toggle_relative" -- toggle whether movement is relative to sensor's position
		"toggle_space_limit" -- toggle whether 6-sensor can go outside working volume
		"toggle_return_to_zero" -- toggle whether return-to-zero operation is on
		"toggle_swap_transrot" -- toggle whether to swap translation and rotation inputs
		"toggle_swap_yz" -- toggle the swapping of Y and Z translational movements

	Here are the available (valuator) control options for FreeVR:
		"set_transx" -- set the translation value for the simulated 6-sensor
		"set_transy" -- set the translation value for the simulated 6-sensor
		"set_transz" -- set the translation value for the simulated 6-sensor
		"set_azim" -- set the azimuth rotation value for the simulated 6-sensor
		"set_elev" -- set the elevation rotation value for the simulated 6-sensor
		"set_roll" -- set the roll rotation value for the simulated 6-sensor

	Here are the FreeVR configuration argument options for the Joydev:
		"devname" - /dev/input/js<num> port Joydev is connected to
			("/dev/input/js0" is the default)
		"restrict" - limit simulated 6-sensors to move within the working-volume
		"valuatoroverride" - produce valuator values instead of moving the 6-sensor
		"returntozero" - valuator offsets map to absolute or relative position of simulated 6-sensor
		"relativerot" - rotation of simulated 6-sensor is based on sensor's or world's coord-sys
	  	"swaptransrot" -- swap the translational and rotational inputs
		"workingvolume" - orthogonally defined volume specifying the valid range of simulated 6-sensor movement
		"transscale" - scaling factor used to tune the translational movement of a simulated 6-sensor
		"rotscale" - scaling factor used to tune the rotational movement of a simulated 6-sensor
		"valscale" - scaling factor used to tune the valuator input range

	NOTE: for the "joytest" test application, because the first connection
		with the joydev device grabs a value for each input, the
		"changed?" flag will always be "1" after the first pass.
		(and because the test application doesn't reset the "changed?"
		values.)

HISTORY:
	17 January 2003 (Bill Sherman) -- wrote initial version using some
		example code from the net and Albert.

	21 January 2003 (Bill Sherman) -- wrote interface to FreeVR.  Added
		compile time flags to make a version that will compile on
		non-Linux systems, but without providing any real input.

	22 January 2003 (Bill Sherman) -- wrote the simulated 6-sensor
		interface.

	21-23 April 2003 (Bill Sherman)
		Updated to new input matching function code style.  Changed
		"opaque" field to "aux_data".  Split _JoydevFunction() into 5
		functions.
		Added new vrPrintStyle argument to _JoydevPrintStruct() for the
		sake of the new "PrintAux" callback.

	1 May 2003 (Bill Sherman)
		A simulated 6-sensor input is now only assigned when the values
		change.

	3 June 2003 (Bill Sherman)
		Now include "vr_enums.h" for the TEST_APP code.
		Added the address of the auxiliary data to the printout.
		Added the "system_pause_toggle" control callback.

	9 October 2003 (Bill Sherman)
		Added inclusion of "sys/time.h" when compiling under Linux
		because *older* versions of Linux require it -- it seems
		to be included in some of the other header files on newer
		Linuxii.

       13 October 2003 (John Stone)
		Added inclusion of "sys/ioctl.h" when compiling under Linux
		because *older* versions of Linux require it -- it seems
		to be included in some of the other header files on newer
		Linuxii.

       16 November 2004 (Bill Sherman & Jeff Stuart)
		Fixed some null dereferencing bugs for which it isn't clear
		how it worked in the past.

       22 November 2004 (Bill Sherman & Dave Zielinski)
		I added some protection against getting event inputs outside
		the bounds allocated for buttons or axes.  I also added a
		fix sent to me by Dave on 9/22/2004 (had "num_axes", s/b
		"num_buttons").

	16 October 2009 (Bill Sherman)
		A quick fix to the _JoydevParseArgs() routine to handle the
		no-arguments case.

	30 March 2010 (Bill Sherman)
		A quick fix to prevent the "valuator_inputs" field from being
		written beyond its bounds. Also changed references to "/dev/js0"
		to be "/dev/input/js0" (including the default port) since that
		seems to be how modern Linuxii have moved.

	10 August 2013 (Bill Sherman)
		I cleaned up the "joytest" test application.  Specifically to
		allow it to be killed with two interrupts (was essentially
		ignoring them), or by pressing buttons 1 & 2 together.

	20-21 August 2013 (Bill Sherman)
		Added the "-list" and "-nodata" options to the "joytest"
		standalone application (based on code from vr_input.evio.c).
		Fixed the type for "num_buttons" and "num_axes", along with
		some minor cleanups.  Wrote the "joytest.1" man-page.

	1 September 2013 (Bill Sherman)
		I added a new __JoydevPrint6sensorOptions() function to
		specifically just print information about simulated 6-sensors.
		Also added a callback to use that as a self-control.
		I am making use of the newly created SELFCTRL_DBGLVL for all
		the self-control outputs.  I then added some reasonable output
		to the "print_help" callback to print out what all the device's
		inputs are doing.

		(Also converted some 8-spaces to tabs -- not in comments.)

	4 September 2013 (Bill Sherman)
		I added a hard-coded threshold check on valuator inputs.  It
		turns out that with 8 bit A-D inputs the lack of a zero-point
		stands out.  (Of course, in my tutorials I handle this for the
		joystick inputs, but maybe it is just smarter to handle it in
		the library -- plus when using the values for the simulated
		6-sensor inputs, it's all within the library, so that's the
		only place to handle it.

	14 September 2013 (Bill Sherman)
		I added the "joytest" man page to the end of this document.

	15 September 2013 (Bill Sherman)
		Renaming "active_sensor" to "active_sim6sensor".
		Changed the "0x%p" format to the improved "%#p" format.

TODO:
	- DONE: Write a man page for "joytest"

	- DONE: Consider printing the self-controls as part of the _JoydevPrintHelp() function
		(which currently does nothing of interest).

	- Read and report the joydev calibration data.  See this page
		for examples:
		- http://forum.egosoft.com/viewtopic.php?t=332757

	- Add the capability of having dual-purpose for valuators as both
		normal valuator input or as simulated 6-sensor controls.
		(See "ignore_trans" and "ignore_all" in the Magellan and
		Spacetec drivers.)

		Perhaps I would have a configuration line:
			"val[x]" = "valuator(sim6[transx|VR_X|X]);"
		which would mean that valuator arbitrarily named "val[x]" gets
		it's value from the VR_X component of the simulated 6-sensor
		values in "aux->sim_values[]" -- of course only when the
		temp_valuator flag is set (aka "ignore_trans" or "ignore_all").

		Actually, looking at the code, for the Magellan and Spacetec
		devices it's always just mapped to VR_X and VR_Y.  But this way
		would be more flexible.

	- Bring in some of the improved comments, etc. from VruiDD (and the new skeleton) [10/19/2009]

	- Make sure error handling of controls & input type mismatches is
		reasonable -- eg. trying to use a valuator control with a
		button input & vice versa.

	- When a button goes to a valuator, allow sign change operation.
		(ie. allow the sign to be specified in the "control" config line.)

	- Allow multiple inputs to same control, so for example set_roll can
		be implemented by two opposing button presses.

	- Add a command-line option to control the style of valuator output
		in "eviotest".

	- enable forecfeedback (rumble) in devices so-equipped.  (perhaps not
		possible via the joydev interface, and requires the EVIO/evdev
		interface.)

	- (perhaps) make use of incoming time data

	- Should check to see whether some other free Unixii have the
		joydev interface.

**************************************************************************/
#if !defined(FREEVR) && !defined(TEST_APP) && !defined(CAVE)
/* Choose how this code should be compiled (ie. for CAVE, standalone, etc). */
#  define	FREEVR
#  undef	TEST_APP
#  undef	CAVE
#endif

#undef	DEBUG


#include <stdio.h>
#include <string.h>		/* for strchr, strlen, strdup & strcmp */
#include <signal.h>		/* needed for signal() and associated #defines */
#include <stdint.h>		/* needed for uint16_t & uint32_t types */
#include <fcntl.h>		/* needed for open() and fcntl() */
#ifdef __linux
#  include <sys/ioctl.h>	/* needed for ioctl() */
#  include <sys/time.h>		/* needed for struct timeval */
#  include <linux/joystick.h>	/* needed for Joydev flags and masks */
#endif

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
#  define	VR_W	3
#  define	VR_AZIM	0
#  define	VR_ELEV	1
#  define	VR_ROLL	2
#endif

#if defined(TEST_APP)
#  include "vr_serial.c"
#  include "vr_enums.h"
#  define	vrShmemAlloc	malloc
#  define	vrShmemAlloc0	malloc
#  define	vrAtoI		atoi
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


	/***************************************************/
	/*** definitions for interfacing with the device ***/
	/***                                             ***/

#define	DEFAULT_DEVICE	"/dev/input/js0"

/** Joydev indices **/
/* NOTE: These labels are based on the Logitech Wingman USB controller, they may not reflect other controllers */
/*   Additional comments are to compare with other devices */
						/* 4-but analog		LT-RumblePad2	LT-F710-generic	LT-F710 as Xbox	X-box		N64-Adaptoid	GrIP	*/
#define Joydev_BUTTONINDEX_A	 0		/* red			1		x/blue		a/green		a/green		a/blue		green	*/
#define Joydev_BUTTONINDEX_B	 1		/* blue			2		a/green		b/red		b/red		cpad-down	yellow	*/
#define Joydev_BUTTONINDEX_C	 2		/* yellow		3		b/red		x/blue		x/blue		cpad-right	red	*/
#define Joydev_BUTTONINDEX_X	 3		/* green		4		y/yellow	y/yellow	y/yellow	b/green		blue	*/
#define Joydev_BUTTONINDEX_Y	 4		/*			5/L1		left-button	left-button	left-button	cpad-left	L1	*/
#define Joydev_BUTTONINDEX_Z	 5		/*			6/R1		right-button	right-button	right-button	cpad-up		R1	*/
#define Joydev_BUTTONINDEX_L1	 6		/*			7/L2		left-trigger*	start		start		L		L2	*/
#define Joydev_BUTTONINDEX_R1	 7		/*			8/R2		right-trigger*	Logitech	X-box		R		R2	*/
#define Joydev_BUTTONINDEX_START 8		/*			9/back		back		left-joystick	left-joystick	Start/red	Start	*/
#define Joydev_BUTTONINDEX_L2	 9		/*			10/start	start		right-joystick	right-joystick	Z/Trigger	Select	*/
#define Joydev_BUTTONINDEX_R2	10		/*			left-joystick 	left-joystick	back		back		dpad-up			*/
#define Joydev_BUTTONINDEX_MODE	11		/*			right-joystick	right-joystick					dpad-down		*/
						/*											dpad-left		*/
						/*											dpad-right		*/
						/*  * For the Logitech F710 operating as a generic controller, the left & right triggers have to hit a threshold to activate, and the Logitech button does nothing */

#define	Joydev_VALUATORINDEX_LEFT_X	0	/*			left-x	 	left-x	 	left-x		left-x		joy-x			*/
#define	Joydev_VALUATORINDEX_LEFT_Y	1	/*			left-y	  	left-y		left-y		left-y		joy-y			*/
#define	Joydev_VALUATORINDEX_SLIDER	2	/*			right-x	  	right-x		left-trigger	left-trigger				*/
#define	Joydev_VALUATORINDEX_RIGHT_X	3	/*			right-y	  	right-y		right-x		right-x					*/
#define	Joydev_VALUATORINDEX_RIGHT_Y	4	/*			dig-x	  	dig-x		right-y		right-y					*/
#define	Joydev_VALUATORINDEX_DIGITAL_X	5	/* joystick-X		dig-y	  	dig-y		right-trigger	right-trigger			joy-X	*/
#define	Joydev_VALUATORINDEX_DIGITAL_Y	6	/* joystick-Y			  			dig-x		dig-x				joy-Y	*/
						/*				  			dig-y		dig-y					*/

/* Joydev sensitivity values */
#define TRANS_SENSITIVITY	0.1
#define ROT_SENSITIVITY		1.0
#define VALUATOR_SENSITIVITY	1.0


/****************************************************************/
/*** auxiliary structure of the current data from the device. ***/
typedef struct {
		/* these are for interfacing with the hardware */
		int		fd;			/* communication file descriptor */
		fd_set		read_fds;		/* list of file descriptors for select() -- will only use one */
		char		*devfile;		/* name of device file */
		int		open;			/* flag with Joydev successfully open */
		char		name[256];		/* name reported by the device */

		/* these are for internal data parsing */
		char		version[256];		/* self-reported version of the device */

#ifdef CAVE /* { */
		/* CAVE specific fields here */

#elif defined(FREEVR) /* } { */
		/* FREEVR specific fields here */

#  define MAX_BUTTONS    32
		vr2switch	*button_inputs[MAX_BUTTONS];
#  define MAX_VALUATORS  16
		vrValuator	*valuator_inputs[MAX_VALUATORS];
		float		valuator_sign[MAX_VALUATORS];
#  define MAX_6SENSORS  16
		vr6sensor	*sensor6_inputs[MAX_6SENSORS];
		int		sim6sensor_change;	/* flag that indicates whether any of the controls to a simulated 6-sensor have changed */

		/* for the FreeVR simulated 6-sensors */
		int		active_sim6sensor;	/* The simulated 6-sensor that is being actively controlled */
		vr6sensorConv	sensor6_options;	/* Structure of settings that affect how a 6-sensor is simulated */
		float		sim_values[6];		/* Array of 6 valuator values used to move the simulated 6-sensor */
#endif /* } end library-specific fields */

		/* information about the current values */
		uint8_t		num_buttons;		/* number of buttons reported by the device */
		uint8_t		*button;		/* array of button values */
		uint8_t		*button_change;		/* array of flags to indicate new button value */
		uint8_t		num_axes;		/* number of valuators (aka axes) reported by the device */
		float		*axis;			/* array of axis values */
		uint8_t		*axis_change;		/* array of flags to indicate new valuator value */

		/* information about how to filter the data for use with the VR library (FreeVR or CAVE) */
		float		scale_valuator;		/* scaling factor for valuators */
		/* TODO: determine whether any filter values/settings are necessary */

	} _JoydevPrivateInfo;



	/*********************************************/
	/*** General NON public interface routines ***/
	/*********************************************/

/******************************************************/
/* typename is used to specify a particular device among many that */
/*   share (more or less) the same protocol.  The typename is then */
/*   used to determine what specific features are available with   */
/*   this particular type of device.                               */
static void _JoydevInitializeStruct(_JoydevPrivateInfo *aux, char *typename)
{
	/* set input defaults */
	aux->version[0] = '\0';
	aux->name[0] = '\0';

	aux->scale_valuator = VALUATOR_SENSITIVITY;	/* set the default valuator scaling factor */

#ifndef TEST_APP /* {  (we have no simulated inputs for the test app) */
	/* set values for simulated 6-sensor */
	aux->sim_values[0] = 0.0;			/* start with no movement values in the valuators holders */
	aux->sim_values[1] = 0.0;
	aux->sim_values[2] = 0.0;
	aux->sim_values[3] = 0.0;
	aux->sim_values[4] = 0.0;
	aux->sim_values[5] = 0.0;

	aux->active_sim6sensor = 1;			/* start with the "wand" active  -- potential bug */
	aux->active_sim6sensor = 0;			/* TODO: go back to wand as default -- currently head (ie. delete this line) */

	aux->sensor6_options.azimuth_axis = VR_Y;	/* azimuth is about the Y axis */
	aux->sensor6_options.relative_axis = 1;		/* default to relative rotations */
	aux->sensor6_options.return_to_zero = 0;	/* default to free floating */
	aux->sensor6_options.ignore_all = 0;		/* default to joystick off */
	aux->sensor6_options.ignore_trans = 0;		/* default to joystick off */
	aux->sensor6_options.restrict_space = 1;	/* default restricted to working-volume */
	aux->sensor6_options.swap_yz = 0;		/* default to standard Y & Z axes */
	aux->sensor6_options.swap_transrot = 0;		/* default to unswapped translation & rotation */
	aux->sensor6_options.trans_scale = TRANS_SENSITIVITY;	/* set the default translational scaling */
	aux->sensor6_options.rot_scale = ROT_SENSITIVITY;	/* set the default rotational scaling */

	/* The default working-volume is that of the typical CAVE */
	aux->sensor6_options.working_volume_min[VR_X] = -5.0;
	aux->sensor6_options.working_volume_max[VR_X] =  5.0;
	aux->sensor6_options.working_volume_min[VR_Y] =  0.0;
	aux->sensor6_options.working_volume_max[VR_Y] = 10.0;
	aux->sensor6_options.working_volume_min[VR_Z] = -5.0;
	aux->sensor6_options.working_volume_max[VR_Z] =  5.0;
#endif /* } */

	/* everything else is zero'd by default */
}


#ifdef FREEVR /* { */
/******************************************************/
/* ... */
/* NOTE: simulated 6-sensors are not part of the test application */
static void _JoydevPrint6sensorOptions(FILE *file, _JoydevPrivateInfo *aux, vrPrintStyle style)
{
	switch (style) {
		case brief:
		vrFprintf(file, "\r"
			"\t6sensor_options = %d, %d, %d, %d, %d, %d, %d, %d, %d, %.2f %.2f (%.2f--%.2f %.2f--%.2f %.2f--%.2f)\n",
			aux->sensor6_options.azimuth_axis,
			aux->sensor6_options.ignore_all,
			aux->sensor6_options.ignore_trans,
			aux->sensor6_options.tmp_ignore_trans,
			aux->sensor6_options.relative_axis,
			aux->sensor6_options.return_to_zero,
			aux->sensor6_options.restrict_space,
			aux->sensor6_options.swap_yz,
			aux->sensor6_options.swap_transrot,
			aux->sensor6_options.trans_scale,
			aux->sensor6_options.rot_scale,
			aux->sensor6_options.working_volume_min[0],
			aux->sensor6_options.working_volume_max[0],
			aux->sensor6_options.working_volume_min[1],
			aux->sensor6_options.working_volume_max[1],
			aux->sensor6_options.working_volume_min[2],
			aux->sensor6_options.working_volume_max[2]);

		vrFprintf(file, "\r"
			"\tsim_values = (%.2f, %.2f, %.2f,  %.2f, %.2f, %.2f)\n"
			"\tsim6sensor_change = %d\n",
			aux->sim_values[VR_X],
			aux->sim_values[VR_Y],
			aux->sim_values[VR_Z],
			aux->sim_values[VR_AZIM+3],
			aux->sim_values[VR_ELEV+3],
			aux->sim_values[VR_ROLL+3],
			aux->sim6sensor_change);
		break;

		case verbose:
		default:
		vrFprintf(file, BOLD_TEXT "Joydev - 6-sensor settings:" NORM_TEXT "\n");
		vrFprintf(file, "\tActive 6-sensor = %d\n", aux->active_sim6sensor);
		vrFprintf(file, "\t6sensor_options = {\n");
		vrFprintf(file, "\r"
			"\t\tazimuth-axis = '%c'\n"
			"\t\tignore-all = %d\n"
			"\t\tignore-trans = %d\n"
			"\t\ttmp_ignore-trans = %d\n"
			"\t\trelative-axis = %d\n"
			"\t\treturn-to-zero = %d\n"
			"\t\trestrict-space = %d\n"
			"\t\tswap-yz = %d\n"
			"\t\tswap-transrot = %d\n"
			"\t\ttrans-scale = %.2f\n"
			"\t\trot-scale = %.2f\n"
			"\t\tworking-volume = (%.2f--%.2f %.2f--%.2f %.2f--%.2f)\n",
			(aux->sensor6_options.azimuth_axis == VR_X ? 'X' :
				(aux->sensor6_options.azimuth_axis == VR_Y ? 'Y' :
				(aux->sensor6_options.azimuth_axis == VR_Z ? 'Z' :
				'?'))),
			aux->sensor6_options.ignore_all,
			aux->sensor6_options.ignore_trans,
			aux->sensor6_options.tmp_ignore_trans,
			aux->sensor6_options.relative_axis,
			aux->sensor6_options.return_to_zero,
			aux->sensor6_options.restrict_space,
			aux->sensor6_options.swap_yz,
			aux->sensor6_options.swap_transrot,
			aux->sensor6_options.trans_scale,
			aux->sensor6_options.rot_scale,
			aux->sensor6_options.working_volume_min[0],
			aux->sensor6_options.working_volume_max[0],
			aux->sensor6_options.working_volume_min[1],
			aux->sensor6_options.working_volume_max[1],
			aux->sensor6_options.working_volume_min[2],
			aux->sensor6_options.working_volume_max[2]);

		vrFprintf(file, "\r"
			"\t\tsim_values = (%.2f, %.2f, %.2f,  %.2f, %.2f, %.2f)\n"
			"\t\tsim6sensor_change = %d\n",
			aux->sim_values[VR_X],
			aux->sim_values[VR_Y],
			aux->sim_values[VR_Z],
			aux->sim_values[VR_AZIM+3],
			aux->sim_values[VR_ELEV+3],
			aux->sim_values[VR_ROLL+3],
			aux->sim6sensor_change);
		vrFprintf(file, "\t}\n");
		break;
	}
}
#endif /* } FREEVR */


/******************************************************/
static void _JoydevPrintStruct(FILE *file, _JoydevPrivateInfo *aux, vrPrintStyle style)
{
	int	count;

	vrFprintf(file, "Joydev device internal structure (%#p):\n", aux);
	vrFprintf(file, "\r\tversion -- '%s'\n", aux->version);
	vrFprintf(file, "\r\tname -- '" BOLD_TEXT "%s" NORM_TEXT "'\n", aux->name);
	vrFprintf(file, "\r\tfd = %d\n\tdevfile = '%s'\n\topen = %d\n",
		aux->fd,
		aux->devfile,
		aux->open);

	/* print the raw values */
	vrFprintf(file, "\r\tnum_buttons = %d\n", aux->num_buttons);
	for (count = 0; count < aux->num_buttons; count++)
		vrFprintf(file, "\r\t\tbutton[%2d] = %d  (changed? %d)\n", count, aux->button[count], aux->button_change[count]);

	vrFprintf(file, "\r\tnum_valuators = %d\n", aux->num_axes);
	for (count = 0; count < aux->num_axes; count++)
		vrFprintf(file, "\r\t\tvaluator[%d] = %+f  (changed? %d)\n", count, aux->axis[count], aux->axis_change[count]);

#ifdef FREEVR /* { */
	/* print the FreeVR input objects */
	vrFprintf(file, "\r\tbutton inputs:\n");
	for (count = 0; count < MAX_BUTTONS; count++) {
		vrFprintf(file, "\r\t\tbutton_input[%d] = %#p", count, aux->button_inputs[count]);
		if (aux->button_inputs[count] != NULL)
			vrFprintf(file, " type: %s, name: %s\n", vrInputTypeName(aux->button_inputs[count]->input_type), aux->button_inputs[count]->my_object->name);
		else	vrFprintf(file, "\n");
	}

	vrFprintf(file, "\r\tvaluator inputs:\n");
	for (count = 0; count < MAX_VALUATORS; count++) {
		vrFprintf(file, "\r\t\tvaluator_input[%d] = %#p", count, aux->valuator_inputs[count]);
		if (aux->valuator_inputs[count] != NULL)
			vrFprintf(file, " type: %s, name: %s\n", vrInputTypeName(aux->valuator_inputs[count]->input_type), aux->valuator_inputs[count]->my_object->name);
		else	vrFprintf(file, "\n");
	}

	vrFprintf(file, "\r\t6sensor inputs (active = %d):\n", aux->active_sim6sensor);
	for (count = 0; count < MAX_6SENSORS; count++) {
		vrFprintf(file, "\r\t\t6sensor_inputs[%d] = %#p", count, aux->sensor6_inputs[count]);
		if (aux->sensor6_inputs[count] != NULL)
			vrFprintf(file, " type: %s, name: %s\n", vrInputTypeName(aux->sensor6_inputs[count]->input_type), aux->sensor6_inputs[count]->my_object->name);
		else	vrFprintf(file, "\n");
	}

	vrFprintf(file, "\r");
	_JoydevPrint6sensorOptions(file, aux, brief);

#endif /* } FREEVR */

}


/**************************************************************************/
static void _JoydevPrintHelp(FILE *file, _JoydevPrivateInfo *aux)
{
	int	count;		/* looping variable */

#if 0 || defined(TEST_APP)
	vrFprintf(file, BOLD_TEXT "Sorry, Joydev - print_help control not yet implemented." NORM_TEXT "\n");
#else
	vrFprintf(file, BOLD_TEXT "Joydev - inputs:" NORM_TEXT "\n");
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


/***************************************************/
/* Returns the number of inputs successfully read. */
static int _JoydevReadInput(_JoydevPrivateInfo *aux)
{
	char			inputs_read = 0;
#ifdef __linux /* { */
static	struct timeval		no_block = { 0 , 0 };
static	struct js_event		event;

	if (aux->open == 0) {
		return (0);
	}

	/* set the file descriptor list for select() to watch the joydev device */
	FD_ZERO(&aux->read_fds);
	FD_SET(aux->fd, &aux->read_fds);

	/* loop until there is no more data on the device */
	while (select(aux->fd + 1, &aux->read_fds, NULL, NULL, &no_block)) {

		/* read the data from the device */
		read(aux->fd, &event, sizeof(event));

#ifdef DEBUG
		vrPrintf("Event: time = %u, type = %u, code = %u, value = %d\n", event.time, event.type, event.number, event.value);
#endif

		/* parse the event */
		if (event.type & JS_EVENT_INIT) {
			/* got an initial value */
#ifdef DEBUG
			vrPrintf("Yo: initial value received\n");
#endif
		}
		switch (event.type & ~JS_EVENT_INIT) {
		case JS_EVENT_AXIS:
			if (event.number+1 > aux->num_axes) {
				vrDbgPrintfN(AALWAYS_DBGLVL,
					RED_TEXT "Unexpected axis %d (only %d available) -- ignoring input.\n" NORM_TEXT,
					event.number, aux->num_axes);
			} else {
				vrDbgPrintfN(INPUT_DBGLVL, "_JoydevReadInput(): got axis event: number = %d, value = %d\n",
					event.number, event.value);
				aux->axis[event.number] = event.value / 32767.0; /* NOTE: this hard-codes for the [-1.0, 1.0] range of values -- we need a setting for this instead */
				/* NOTE: for now I'm hard-coding a threshold (assuming 8-bit precision), but TODO: this needs to be configurable */
				/* NOTE: I'm not using fabs() here to avoid having to include the math library.  Plus, this is probably less cycles. */
				if (aux->axis[event.number] > -0.01 && aux->axis[event.number] < 0.01) {
					aux->axis[event.number] = 0.0;
				}
				aux->axis_change[event.number] = 1;
				inputs_read++;
			}
			break;
		case JS_EVENT_BUTTON:
			if (event.number+1 > aux->num_buttons) {
				vrDbgPrintfN(AALWAYS_DBGLVL,
					RED_TEXT "Unexpected button %d (only %d available) -- ignoring input.\n" NORM_TEXT,
					event.number, aux->num_buttons);
			} else {
				vrDbgPrintfN(INPUT_DBGLVL, "_JoydevReadInput(): got button event: number = %d, value = %d\n",
					event.number, event.value);
				aux->button[event.number] = event.value;
				aux->button_change[event.number] = 1;
				inputs_read++;
			}
			break;
		}
	}
#endif /* } (__linux) */

	return (inputs_read);
}


/**********************************************************/
/* _JoydevInitializeDevice() is called in the OPEN phase  */
/*   of input interface -- after the types of inputs have */
/*   been determined (during the CREATE phase).           */
static int _JoydevInitializeDevice(_JoydevPrivateInfo *aux)
{
#ifdef __linux /* { */
	uint32_t	js_version;
#endif /* } */

	if (aux == NULL)
		return -1;

#ifdef __linux /* { */
	/* read name and parameters of device */
	ioctl(aux->fd, JSIOCGVERSION, &js_version);
	ioctl(aux->fd, JSIOCGNAME(255), &aux->name);
	ioctl(aux->fd, JSIOCGAXES, &aux->num_axes);
	ioctl(aux->fd, JSIOCGBUTTONS, &aux->num_buttons);

	sprintf(aux->version, "Linux Joystick Driver version %u.%u.%u", 
		((js_version & 0xff0000) >> 16), ((js_version & 0x00ff00) >> 8), ((js_version & 0x0000ff) >> 1));
	vrDbgPrintfN(INPUT_DBGLVL,
		"Initializing for Joystick Driver version 0x%x, name = '%s', with %d axes and %d buttons, aux->fd = %d\n",
		js_version, aux->name, aux->num_axes, aux->num_buttons, aux->fd);
#else /* } __linux { */
	sprintf(aux->version, "Non-Linux system: No Linux Joystick Driver version available");
	sprintf(aux->name, "Non-Linux system: Unable to obtain device name");
	aux->num_buttons = 0;
	aux->num_axes = 0;
#endif /* } */

	/* allocate memory for reading values from the device */
	aux->axis = vrShmemAlloc0(aux->num_axes * sizeof(*aux->axis));
	aux->axis_change = vrShmemAlloc0(aux->num_axes * sizeof(*aux->axis_change));
	aux->button = vrShmemAlloc0(aux->num_buttons * sizeof(*aux->button));
	aux->button_change = vrShmemAlloc0(aux->num_buttons * sizeof(*aux->button_change));

	/* read some initial data */
	_JoydevReadInput(aux);

	return 0;
}


/******************************************************/
/* Translate a string name of a button (the "instance" config) into a numeric value */
static unsigned int _JoydevButtonValue(char *buttonname)
{
	switch (buttonname[0]) {
	case '0':		/* for buttons that start with a digit, assume it's just a numeric reference */
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		return vrAtoI(buttonname);

	/* NOTE: these codes are not always consistent between devices, so generally it */
	/*   is better to use the numeric representation -- and you can use "joytest"   */
	/*   to determine the mapping between physical buttons and the numbers.         */
	case 'a':
	case 'A':
		return Joydev_BUTTONINDEX_A;

	case 'b':
	case 'B':
		return Joydev_BUTTONINDEX_B;

	case 'c':
	case 'C':
		return Joydev_BUTTONINDEX_C;

	case 'x':
	case 'X':
		return Joydev_BUTTONINDEX_X;

	case 'y':
	case 'Y':
		return Joydev_BUTTONINDEX_Y;

	case 'z':
	case 'Z':
		return Joydev_BUTTONINDEX_Z;

	case 'l':
	case 'L':
		switch (buttonname[1]) {
		case '1':
			return Joydev_BUTTONINDEX_L1;
		case '2':
			return Joydev_BUTTONINDEX_L2;
		default:
			return -1;
		}

	case 's':	/* select and start */
	case 'r':	/* 1 and 2 */
	case 'R':
		switch (buttonname[1]) {
		case '1':
			return Joydev_BUTTONINDEX_R1;
		case '2':
			return Joydev_BUTTONINDEX_R2;
		default:
			return -1;
		}

	case 'S':
		return Joydev_BUTTONINDEX_START;

	case 'm':
	case 'M':
		return Joydev_BUTTONINDEX_MODE;
	}

	return -1;
}


/******************************************************/
/* Translate a string name of a valuator (the "instance" config) into a numeric value */
static unsigned int _JoydevValuatorValue(char *valuatorname)
{
	/* skip an opening sign character (is handled in _JoydevValuatorInput) */
	if (valuatorname[0] == '-' || valuatorname[0] == '+')
		valuatorname++;

	switch (valuatorname[0]) {
	case '0':		/* for valuators that start with a digit, assume it's just a numeric reference */
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		return vrAtoI(valuatorname);

	case 's':	/* slider */
	case 'S':
		return Joydev_VALUATORINDEX_SLIDER;

	case 'l':	/* Left Joystick, X and Y */
	case 'L':
		switch (valuatorname[strlen(valuatorname)-1]) {
		case 'x':
		case 'X':
			return Joydev_VALUATORINDEX_LEFT_X;
		case 'y':
		case 'Y':
			return Joydev_VALUATORINDEX_LEFT_Y;
		default:
			return -1;
		}

	case 'r':	/* Right Joystick, X and Y */
	case 'R':
		switch (valuatorname[strlen(valuatorname)-1]) {
		case 'x':
		case 'X':
			return Joydev_VALUATORINDEX_RIGHT_X;
		case 'y':
		case 'Y':
			return Joydev_VALUATORINDEX_RIGHT_Y;
		default:
			return -1;
		}

	case 'd':	/* Digital Joystick, X and Y */
	case 'D':
		switch (valuatorname[strlen(valuatorname)-1]) {
		case 'x':
		case 'X':
			return Joydev_VALUATORINDEX_DIGITAL_X;
		case 'y':
		case 'Y':
			return Joydev_VALUATORINDEX_DIGITAL_Y;
		default:
			return -1;
		}
	}

	return -1;
}


	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/


#if defined(FREEVR) /* { */
/*****************************************************************/
/*** Functions for FreeVR access of Joydev devices for input ***/
/*****************************************************************/


	/************************************/
	/***  FreeVR NON public routines  ***/
	/************************************/


/*********************************************************/
static void _JoydevParseArgs(_JoydevPrivateInfo *aux, char *args)
{
	float	volume_values[6];	/* for reading the working volume array */

	/* In the rare case of no arguments, just return */
	if (args == NULL)
		return;

	/*****************************************/
	/** Argument format: "devname" "=" file **/
	/**    or "devfile" "=" file            **/
	/*****************************************/
	vrArgParseString(args, "devname", &(aux->devfile));
	vrArgParseString(args, "devfile", &(aux->devfile));

	/* arguments for simulated 6-sensors */

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

	/*******************************************************************/
	/** Argument format: "swapTransRot" "=" { "on" | "off" | number } **/
	/*******************************************************************/
	vrArgParseBool(args, "swaptransrot", &(aux->sensor6_options.swap_transrot));

	/*********************************************************************/
	/** Argument format: "sensor_swap_yz" "=" { "on" | "off" | number } **/
	/*********************************************************************/
	vrArgParseBool(args, "sensor_swap_yz", &(aux->sensor6_options.swap_yz));
	vrArgParseBool(args, "sensor_swap_upin", &(aux->sensor6_options.swap_yz));

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

	/**********************************************/
	/** Argument format: "transScale" "=" number **/
	/**********************************************/
	if (vrArgParseFloat(args, "transscale", &(aux->sensor6_options.trans_scale))) {
		aux->sensor6_options.trans_scale *= TRANS_SENSITIVITY;
	}

	/********************************************/
	/** Argument format: "rotScale" "=" number **/
	/********************************************/
	if (vrArgParseFloat(args, "rotscale", &(aux->sensor6_options.rot_scale))) {
		aux->sensor6_options.rot_scale *= ROT_SENSITIVITY;
	}

	/********************************************/
	/** Argument format: "valScale" "=" number **/
	/********************************************/
	if (vrArgParseFloat(args, "valscale", &(aux->scale_valuator))) {
		aux->scale_valuator *= VALUATOR_SENSITIVITY;
	}

	/** TODO: other arguments to parse go here **/
}


/************************************************************/
static void _JoydevGetData(vrInputDevice *devinfo)
{
	_JoydevPrivateInfo	*aux = (_JoydevPrivateInfo *)devinfo->aux_data;
	int			num_inputs;
	int			count;
	float			valuator_value;
 
	/*******************/
	/* gather the data */
	num_inputs = _JoydevReadInput(aux);

	if (num_inputs > 0) {

		/*************/
		/** buttons **/
		/* handle button inputs as buttons or self-controls */
		for (count = 0; count < aux->num_buttons; count++) {
			if ((aux->button_change[count] != 0)) {
				if (aux->button_inputs[count] != NULL) {
					switch (aux->button_inputs[count]->input_type) {
					case VRINPUT_BINARY:
						vrAssign2switchValue((vr2switch *)(aux->button_inputs[count]), ((aux->button[count]) != 0));
						break;
					case VRINPUT_CONTROL:
						vrCallbackInvokeDynamic(((vrControl *)(aux->button_inputs[count]))->callback, 1, ((aux->button[count]) != 0));
						break;
					default:
						vrErrPrintf(RED_TEXT "_JoydevGetData: Unable to handle button inputs that aren't Binary or Control inputs\n" NORM_TEXT);
						break;
					}

				}
				aux->button_change[count] = 0;
			}
		}

		/***************/
		/** valuators **/
		/* handle valuator inputs as valuators or self-controls */
		for (count = 0; count < aux->num_axes; count++) {
			if ((aux->axis_change[count] != 0)) {
				if (aux->valuator_inputs[count] != NULL) {
					valuator_value = aux->axis[count] * aux->scale_valuator * aux->valuator_sign[count];
					switch (aux->valuator_inputs[count]->input_type) {
					case VRINPUT_VALUATOR:
						vrAssignValuatorValue((vrValuator *)(aux->valuator_inputs[count]), valuator_value);
						break;
					case VRINPUT_CONTROL:
						vrCallbackInvokeDynamic(((vrControl *)(aux->valuator_inputs[count]))->callback, 1, &valuator_value);
						break;
					default:
						vrErrPrintf(RED_TEXT "_JoydevGetData: Unable to handle valuator inputs that aren't Floating or Control inputs\n" NORM_TEXT);
						break;
					}
				}
				aux->axis_change[count] = 0;
			}
		}
	}

	/* handle valuator inputs to be converted to simulated 6-sensor inputs */
	if (!aux->sensor6_options.ignore_all && devinfo->num_6sensors > 0 && aux->sim6sensor_change) {
		vrAssign6sensorValueFromValuators(&(devinfo->sensor6[aux->active_sim6sensor]), aux->sim_values, &(aux->sensor6_options), -1 /* (ie. no change to oob flag) */);
		if ((aux->sim_values[VR_X] == 0)
		   && (aux->sim_values[VR_Y] == 0)
		   && (aux->sim_values[VR_Z] == 0)
		   && (aux->sim_values[VR_AZIM+3] == 0)
		   && (aux->sim_values[VR_ELEV+3] == 0)
		   && (aux->sim_values[VR_ROLL+3] == 0)) {
			aux->sim6sensor_change = 0;
		}
	}
}


	/****************************************************************/
	/*    Function(s) for parsing Joydev "input" declarations.      */
	/*                                                              */
	/*  These _Joydev<type>Input() functions are called during the  */
	/*  CREATE phase of the input interface.                        */

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

/**************************************************************************/
static vrInputMatch _JoydevButtonInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
	_JoydevPrivateInfo	*aux = (_JoydevPrivateInfo *)devinfo->aux_data;
	int			button_num;

	/* select a button */
	button_num = _JoydevButtonValue(dti->instance);

	/* check the selected choice */
	if (button_num == -1) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_JoydevButtonInput(): Warning, button['%s'] did not match any known button\n", dti->instance);
		return VRINPUT_MATCH_UNABLE;	/* device match, but still unable to handle request */
	} else if (aux->button_inputs[button_num] != NULL)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_JoydevButtonInput(): Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);

	/* make the mapping -- assign the input argument to the inputs array */
	aux->button_inputs[button_num] = (vr2switch *)input;
	vrDbgPrintfN(INPUT_DBGLVL, "_JoydevButtonInput(): assigned button event of value 0x%02x to input pointer = %#p)\n",
		button_num, aux->button_inputs[button_num]);

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/**************************************************************************/
static vrInputMatch _JoydevValuatorInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
	_JoydevPrivateInfo	*aux = (_JoydevPrivateInfo *)devinfo->aux_data;
	int			valuator_num;

	/* select a valuator (and sign) */
	valuator_num = _JoydevValuatorValue(dti->instance);
	if (dti->instance[0] == '-')
		aux->valuator_sign[valuator_num] = -1.0;
	else	aux->valuator_sign[valuator_num] =  1.0;

	/* check the selected valuator */
	if (valuator_num == -1) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_JoydevValuatorInput(): Warning, valuator['%s'] did not match any known valuator\n", dti->instance);
		return VRINPUT_MATCH_UNABLE;	/* device match, but still unable to handle request */
	} else if (aux->valuator_inputs[valuator_num] != NULL) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_JoydevValuatorInput(): Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);
	}

	/* make the mapping */
	aux->valuator_inputs[valuator_num] = (vrValuator *)input;
	vrDbgPrintfN(INPUT_DBGLVL, "_JoydevValuatorInput(): assigned valuator event of value 0x%02x to input pointer = %#p)\n",
		valuator_num, aux->valuator_inputs[valuator_num]);

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/**************************************************************************/
static vrInputMatch _Joydev6sensorInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
	_JoydevPrivateInfo	*aux = (_JoydevPrivateInfo *)devinfo->aux_data;
	int			sensor_num;

	/* select a sensor */
	sensor_num = vrAtoI(dti->instance);

	/* check the selected sensor */
	if (sensor_num < 0) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_Joydev6sensorInput(): Warning, sensor number must be between %d and %d\n", 0, MAX_6SENSORS);
		return VRINPUT_MATCH_UNABLE;	/* device match, but still unable to handle request */
	} else if (aux->sensor6_inputs[sensor_num] != NULL)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_Joydev6sensorInput(): Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);

	/* make the mapping */
	aux->sensor6_inputs[sensor_num] = (vr6sensor *)input;

	/* the vrAssign6sensorR2Exform() function does the parsing of the next token */
	vrAssign6sensorR2Exform(aux->sensor6_inputs[sensor_num], strchr(dti->instance, ','));

	vrDbgPrintfN(INPUT_DBGLVL, "_Joydev6sensorInput(): assigned 6sensor event of value 0x%02x to input pointer = %#p)\n",
		sensor_num, aux->sensor6_inputs[sensor_num]);

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}



	/************************************************************/
	/***************** FreeVR Callback routines *****************/
	/************************************************************/

	/************************************************************/
	/*    Callbacks for controlling the device's features.      */
	/*                                                          */

/* TODO: describe the consistent arguments of the callbacks. */
/*   ... devinfo -- same as "input", but also sometimes used to get */
/*     information about the device so it can be altered -- eg. _SpacetecSensorResetCallback()  and _XwindowsSensorNextCallback() */
/*       it is also frequently passed on to other callback routines which in */
/*       turn generally use it to access the "aux" data.  */

/************************************************************/
static void _JoydevSystemPauseToggleCallback(vrInputDevice *devinfo, int value)
{
	if (value == 0)
		return;

	devinfo->context->paused ^= 1;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Joydev Control: system_pause = %d.\n",
		devinfo->context->paused);
}

/************************************************************/
static void _JoydevPrintContextStructCallback(vrInputDevice *devinfo, int value)
{
	if (value == 0)
		return;

	vrFprintContext(stdout, devinfo->context, verbose);
}

/************************************************************/
static void _JoydevPrintConfigStructCallback(vrInputDevice *devinfo, int value)
{
	if (value == 0)
		return;

	vrFprintConfig(stdout, devinfo->context->config, verbose);
}

/************************************************************/
static void _JoydevPrintInputStructCallback(vrInputDevice *devinfo, int value)
{
	if (value == 0)
		return;

	vrFprintInput(stdout, devinfo->context->input, verbose);
}

/************************************************************/
static void _JoydevPrintStructCallback(vrInputDevice *devinfo, int value)
{
	_JoydevPrivateInfo  *aux = (_JoydevPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	_JoydevPrintStruct(stdout, aux, verbose);
}

/************************************************************/
static void _JoydevPrint6sensorOptionsCallback(vrInputDevice *devinfo, int value)
{
	_JoydevPrivateInfo  *aux = (_JoydevPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	_JoydevPrint6sensorOptions(stdout, aux, verbose);
}

/************************************************************/
static void _JoydevPrintHelpCallback(vrInputDevice *devinfo, int value)
{
	_JoydevPrivateInfo  *aux = (_JoydevPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	_JoydevPrintHelp(stdout, aux);
}

/************************************************************/
static void _JoydevSensorNextCallback(vrInputDevice *devinfo, int value)
{
	_JoydevPrivateInfo	*aux = (_JoydevPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	if (devinfo->num_6sensors == 0) {
		vrDbgPrintfN(SELFCTRL_DBGLVL, "Joydev Control: next sensor -- no sensors available.\n",
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

	vrDbgPrintfN(SELFCTRL_DBGLVL, "Joydev Control: 6sensor[%d] now active.\n", aux->active_sim6sensor);
}

/* TODO: see if there is a way to call this as an N-switch */
/************************************************************/
static void _JoydevSensorSetCallback(vrInputDevice *devinfo, int value)
{
	_JoydevPrivateInfo	*aux = (_JoydevPrivateInfo *)devinfo->aux_data;

	if (value == aux->active_sim6sensor)
		return;

	if (value < 0 || value >= MAX_6SENSORS) {
		vrDbgPrintfN(SELFCTRL_DBGLVL, "Joydev Control: set sensor (%d) -- out of range.\n", value);
	}

	if (aux->sensor6_inputs[value] == NULL) {
		vrDbgPrintfN(SELFCTRL_DBGLVL, "Joydev Control: set sensor (%d) -- no such sensor available.\n", value);
		return;
	}

	vrAssign6sensorActiveValue(((vr6sensor *)aux->sensor6_inputs[aux->active_sim6sensor]), 0);
	aux->active_sim6sensor = value;
	vrAssign6sensorActiveValue(((vr6sensor *)aux->sensor6_inputs[aux->active_sim6sensor]), 1);

	vrDbgPrintfN(SELFCTRL_DBGLVL, "Joydev Control: 6sensor[%d] now active.\n", aux->active_sim6sensor);
}

/************************************************************/
static void _JoydevSensorSet0Callback(vrInputDevice *devinfo, int value)
{	_JoydevSensorSetCallback(devinfo, 0); }

/************************************************************/
static void _JoydevSensorSet1Callback(vrInputDevice *devinfo, int value)
{	_JoydevSensorSetCallback(devinfo, 1); }

/************************************************************/
static void _JoydevSensorSet2Callback(vrInputDevice *devinfo, int value)
{	_JoydevSensorSetCallback(devinfo, 2); }

/************************************************************/
static void _JoydevSensorSet3Callback(vrInputDevice *devinfo, int value)
{	_JoydevSensorSetCallback(devinfo, 3); }

/************************************************************/
static void _JoydevSensorSet4Callback(vrInputDevice *devinfo, int value)
{	_JoydevSensorSetCallback(devinfo, 4); }

/************************************************************/
static void _JoydevSensorSet5Callback(vrInputDevice *devinfo, int value)
{	_JoydevSensorSetCallback(devinfo, 5); }

/************************************************************/
static void _JoydevSensorSet6Callback(vrInputDevice *devinfo, int value)
{	_JoydevSensorSetCallback(devinfo, 6); }

/************************************************************/
static void _JoydevSensorSet7Callback(vrInputDevice *devinfo, int value)
{	_JoydevSensorSetCallback(devinfo, 7); }

/************************************************************/
static void _JoydevSensorSet8Callback(vrInputDevice *devinfo, int value)
{	_JoydevSensorSetCallback(devinfo, 8); }

/************************************************************/
static void _JoydevSensorSet9Callback(vrInputDevice *devinfo, int value)
{	_JoydevSensorSetCallback(devinfo, 9); }

/************************************************************/
static void _JoydevSensorResetCallback(vrInputDevice *devinfo, int value)
{
static	vrMatrix		idmat = { { 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0 } };
	_JoydevPrivateInfo	*aux = (_JoydevPrivateInfo *)devinfo->aux_data;
	vr6sensor		*sensor;

	if (value == 0)
		return;

	sensor = &(devinfo->sensor6[aux->active_sim6sensor]);
	vrAssign6sensorValue(sensor, &idmat, 0 /* , timestamp */);
	vrAssign6sensorActiveValue(sensor, -1);
	vrAssign6sensorErrorValue(sensor, 0);
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Joydev Control: reset 6sensor[%d].\n", aux->active_sim6sensor);
}

/************************************************************/
static void _JoydevSensorResetAllCallback(vrInputDevice *devinfo, int value)
{
static	vrMatrix		idmat = { { 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0 } };
	_JoydevPrivateInfo	*aux = (_JoydevPrivateInfo *)devinfo->aux_data;
	vr6sensor		*sensor;
	int			count;

	if (value == 0)
		return;

	for (count = 0; count < MAX_6SENSORS; count++) {
		if (aux->sensor6_inputs[count] != NULL) {
			sensor = &(devinfo->sensor6[count]);
			vrAssign6sensorValue(sensor, &idmat, 0 /* , timestamp */);
			vrAssign6sensorActiveValue(sensor, (count == aux->active_sim6sensor));
			vrAssign6sensorErrorValue(sensor, 0);
			vrDbgPrintfN(SELFCTRL_DBGLVL, "Joydev Control: reset 6sensor[%d].\n", count);
		}
	}
}

/************************************************************/
static void _JoydevTempValuatorOverrideCallback(vrInputDevice *devinfo, int value)
{
	_JoydevPrivateInfo	*aux = (_JoydevPrivateInfo *)devinfo->aux_data;

	/* set the field to the current state of value (ie. 1 when depressed, 0 when released) */
	aux->sensor6_options.ignore_trans = value;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Joydev Control: ignore_trans = %d.\n",
		aux->sensor6_options.ignore_trans);
}

/************************************************************/
static void _JoydevToggleValuatorOverrideCallback(vrInputDevice *devinfo, int value)
{
	_JoydevPrivateInfo	*aux = (_JoydevPrivateInfo *)devinfo->aux_data;

	/* take no action when the button is released */
	if (value == 0)
		return;

	/* toggle the field (NOTE: we only get here when button is depressed) */
	aux->sensor6_options.ignore_trans ^= 1;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Joydev Control: ignore_trans = %d.\n",
		aux->sensor6_options.ignore_trans);
}

/************************************************************/
static void _JoydevTempValuatorOnlyCallback(vrInputDevice *devinfo, int value)
{
	_JoydevPrivateInfo	*aux = (_JoydevPrivateInfo *)devinfo->aux_data;

	/* set the field to the current state of value (ie. 1 when depressed, 0 when released) */
	aux->sensor6_options.ignore_all = value;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Joydev Control: ignore_all = %d.\n",
		aux->sensor6_options.ignore_all);
}

/************************************************************/
static void _JoydevToggleValuatorOnlyCallback(vrInputDevice *devinfo, int value)
{
	_JoydevPrivateInfo	*aux = (_JoydevPrivateInfo *)devinfo->aux_data;

	/* take no action when the button is released */
	if (value == 0)
		return;

	/* toggle the field (NOTE: we only get here when button is depressed) */
	aux->sensor6_options.ignore_all ^= 1;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Joydev Control: ignore_all = %d.\n",
		aux->sensor6_options.ignore_all);
}

/************************************************************/
static void _JoydevToggleRelativeAxesCallback(vrInputDevice *devinfo, int value)
{
	_JoydevPrivateInfo	*aux = (_JoydevPrivateInfo *)devinfo->aux_data;

	/* take no action when the button is released */
	if (value == 0)
		return;

	/* toggle the field (NOTE: we only get here when button is depressed) */
	aux->sensor6_options.relative_axis ^= 1;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Joydev Control: relative_axis = %d.\n",
		aux->sensor6_options.relative_axis);
}

/************************************************************/
/* TODO: this should probably also go through all the 6sensor's  */
/*   and move them to be within the allowed workspace when space */
/*   restriction is turned on.                                   */
static void _JoydevToggleRestrictSpaceCallback(vrInputDevice *devinfo, int value)
{
	_JoydevPrivateInfo	*aux = (_JoydevPrivateInfo *)devinfo->aux_data;

	/* take no action when the button is released */
	if (value == 0)
		return;

	/* toggle the field (NOTE: we only get here when button is depressed) */
	aux->sensor6_options.restrict_space ^= 1;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Joydev Control: restrict_space = %d.\n",
		aux->sensor6_options.restrict_space);
}

/************************************************************/
static void _JoydevToggleReturnToZeroCallback(vrInputDevice *devinfo, int value)
{
	_JoydevPrivateInfo	*aux = (_JoydevPrivateInfo *)devinfo->aux_data;

	/* take no action when the button is released */
	if (value == 0)
		return;

	/* toggle the field (NOTE: we only get here when button is depressed) */
	aux->sensor6_options.return_to_zero ^= 1;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Joydev Control: return_to_zero = %d.\n",
		aux->sensor6_options.return_to_zero);
}

/************************************************************/
static void _JoydevToggleSwapTransRotCallback(vrInputDevice *devinfo, int value)
{
	_JoydevPrivateInfo	*aux = (_JoydevPrivateInfo *)devinfo->aux_data;

	/* take no action when the button is released */
	if (value == 0)
		return;

	/* toggle the field (NOTE: we only get here when button is depressed) */
	aux->sensor6_options.swap_transrot ^= 1;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Joydev Control: swap_transrot = %d.\n",
		aux->sensor6_options.swap_transrot);
}

/************************************************************/
static void _JoydevToggleSwapYZCallback(vrInputDevice *devinfo, int value)
{
	_JoydevPrivateInfo	*aux = (_JoydevPrivateInfo *)devinfo->aux_data;

	/* take no action when the button is released */
	if (value == 0)
		return;

	/* toggle the field (NOTE: we only get here when button is depressed) */
	aux->sensor6_options.swap_yz ^= 1;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Joydev Control: swap_yz = %d.\n",
		aux->sensor6_options.swap_yz);
}

/************************************************************/
static void _JoydevSetTransXCallback(vrInputDevice *devinfo, float *value)
{
	_JoydevPrivateInfo	*aux = (_JoydevPrivateInfo *)devinfo->aux_data;

	aux->sim_values[VR_X] = *value;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Joydev Control: set trans_x = %f.\n", *value);
}

/************************************************************/
static void _JoydevSetTransYCallback(vrInputDevice *devinfo, float *value)
{
	_JoydevPrivateInfo	*aux = (_JoydevPrivateInfo *)devinfo->aux_data;

	aux->sim_values[VR_Y] = *value;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Joydev Control: set trans_y = %f.\n", *value);
}

/************************************************************/
static void _JoydevSetTransZCallback(vrInputDevice *devinfo, float *value)
{
	_JoydevPrivateInfo	*aux = (_JoydevPrivateInfo *)devinfo->aux_data;

	aux->sim_values[VR_Z] = *value;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Joydev Control: set trans_z = %f.\n", *value);
}

/************************************************************/
static void _JoydevSetAzimuthCallback(vrInputDevice *devinfo, float *value)
{
	_JoydevPrivateInfo	*aux = (_JoydevPrivateInfo *)devinfo->aux_data;

	aux->sim_values[VR_AZIM+3] = *value;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Joydev Control: set azimuth = %f.\n", *value);
}

/************************************************************/
static void _JoydevSetElevationCallback(vrInputDevice *devinfo, float *value)
{
	_JoydevPrivateInfo	*aux = (_JoydevPrivateInfo *)devinfo->aux_data;

	aux->sim_values[VR_ELEV+3] = *value;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Joydev Control: set elevation = %f.\n", *value);
}

/************************************************************/
static void _JoydevSetRollCallback(vrInputDevice *devinfo, float *value)
{
	_JoydevPrivateInfo	*aux = (_JoydevPrivateInfo *)devinfo->aux_data;

	aux->sim_values[VR_ROLL+3] = *value;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Joydev Control: set roll = %f.\n", *value);
}



	/*************************************************/
	/*   Callbacks for interfacing with the device.  */
	/*                                               */


/************************************************************/
static void _JoydevCreateFunction(vrInputDevice *devinfo)
{
	/*** List of possible inputs ***/
static	vrInputFunction	_JoydevInputs[] = {
				/* tuple: "DTI.type-name", input-type, match-function */
				{ "button", VRINPUT_2WAY, _JoydevButtonInput },
				{ "axis", VRINPUT_VALUATOR, _JoydevValuatorInput },
				{ "valuator", VRINPUT_VALUATOR, _JoydevValuatorInput },
				{ "slider", VRINPUT_VALUATOR, _JoydevValuatorInput },
				{ "sim6", VRINPUT_6SENSOR, _Joydev6sensorInput },
				{ NULL, VRINPUT_UNKNOWN, NULL } };
	/*** List of control functions ***/
static	vrControlFunc	_JoydevControlList[] = {
				/* overall system controls */
				{ "system_pause_toggle", _JoydevSystemPauseToggleCallback },

				/* informational output controls */
				{ "print_context", _JoydevPrintContextStructCallback },
				{ "print_config", _JoydevPrintConfigStructCallback },
				{ "print_input", _JoydevPrintInputStructCallback },
				{ "print_struct", _JoydevPrintStructCallback },
				{ "print_sim6opts", _JoydevPrint6sensorOptionsCallback },
				{ "print_help", _JoydevPrintHelpCallback },

				/* simulated 6-sensor selection controls */
				{ "sensor_next", _JoydevSensorNextCallback },
				{ "setsensor", _JoydevSensorSetCallback },	/* NOTE: this is non-boolean */
				{ "setsensor(0)", _JoydevSensorSet0Callback },
				{ "setsensor(1)", _JoydevSensorSet1Callback },
				{ "setsensor(2)", _JoydevSensorSet2Callback },
				{ "setsensor(3)", _JoydevSensorSet3Callback },
				{ "setsensor(4)", _JoydevSensorSet4Callback },
				{ "setsensor(5)", _JoydevSensorSet5Callback },
				{ "setsensor(6)", _JoydevSensorSet6Callback },
				{ "setsensor(7)", _JoydevSensorSet7Callback },
				{ "setsensor(8)", _JoydevSensorSet8Callback },
				{ "setsensor(9)", _JoydevSensorSet9Callback },
				{ "sensor_reset", _JoydevSensorResetCallback },
				{ "sensor_resetall", _JoydevSensorResetAllCallback },

				/* simulated 6-sensor manipulation controls */
				{ "temp_valuator", _JoydevTempValuatorOverrideCallback },
				{ "toggle_valuator", _JoydevToggleValuatorOverrideCallback },
				{ "temp_valuator_only", _JoydevTempValuatorOnlyCallback },
				{ "toggle_relative", _JoydevToggleRelativeAxesCallback },
				{ "toggle_space_limit", _JoydevToggleRestrictSpaceCallback },
				{ "toggle_return_to_zero", _JoydevToggleReturnToZeroCallback },
				{ "toggle_swap_transrot", _JoydevToggleSwapTransRotCallback },
				{ "toggle_swap_yz", _JoydevToggleSwapYZCallback },
				{ "set_transx", _JoydevSetTransXCallback },
				{ "set_transy", _JoydevSetTransYCallback },
				{ "set_transz", _JoydevSetTransZCallback },
				{ "set_azim", _JoydevSetAzimuthCallback },
				{ "set_elev", _JoydevSetElevationCallback },
				{ "set_roll", _JoydevSetRollCallback },

				/* other controls */
				/*   NONE   */

				/* end of the list */
				{ NULL, NULL } };

	_JoydevPrivateInfo	*aux = NULL;

	/******************************************/
	/* allocate and initialize auxiliary data */
	devinfo->aux_data = (void *)vrShmemAlloc0(sizeof(_JoydevPrivateInfo));
	aux = (_JoydevPrivateInfo *)devinfo->aux_data;
	_JoydevInitializeStruct(aux, devinfo->type);

	/******************/
	/* handle options */
	aux->devfile = vrShmemStrDup(DEFAULT_DEVICE);	/* default, if no device file given */
	_JoydevParseArgs(aux, devinfo->args);

	/***************************************/
	/* create the inputs and self-controls */
	vrInputCreateDataContainers(devinfo, _JoydevInputs);
	vrInputCreateSelfControlContainers(devinfo, _JoydevInputs, _JoydevControlList);
	/* TODO: anything to do for NON self-controls?  Implement for Magellan first. */

	vrDbgPrintf("_JoydevCreateFunction(): Done creating Joydev inputs for '%s'\n", devinfo->name);
	devinfo->created = 1;

	return;
}


/************************************************************/
static void _JoydevOpenFunction(vrInputDevice *devinfo)
{
	_JoydevPrivateInfo	*aux = NULL;

	vrTrace("_JoydevOpenFunction", devinfo->name);

	aux = (_JoydevPrivateInfo *)devinfo->aux_data;

	/*******************/
	/* open the device */
#ifdef __linux /* { */
	aux->fd = open(aux->devfile, O_RDONLY);
	vrDbgPrintfN(INPUT_DBGLVL, "_JoydevOpenFunction(): just opened aux->fd = %d, devfile = '%s'\n", aux->fd, aux->devfile);
	if (aux->fd < 0) {
		aux->open = 0;
		vrErrPrintf("_JoydevOpenFunction(): Joydev device '%s' error: " RED_TEXT "couldn't open joydev device '%s'\n" NORM_TEXT,
			devinfo->name, aux->devfile);
		sprintf(aux->version, "- unconnected Joydev -");
	} else {
		aux->open = 1;
		if (_JoydevInitializeDevice(aux) < 0) {
			vrErrPrintf("_JoydevOpenFunction(): "
				RED_TEXT "Warning, unable to initialize Joydev '%s'.\n" NORM_TEXT, devinfo->name);
		} else {
			devinfo->operating = 1;
			vrDbgPrintf("_JoydevOpenFunction(): Done opening Joydev input device '%s'.\n", devinfo->name);
		}
	}
#else /* } { ! __linux */
	vrErrPrintf("_JoydevOpenFunction(): "
		RED_TEXT "Warning, unable to use Joydev devices on non-Linux systems. ('%s' inputs exist, but without hardware).\n" NORM_TEXT, devinfo->name);
	aux->open = 0;
	devinfo->operating = 0;
#endif /* } */

	return;
}


/************************************************************/
static void _JoydevCloseFunction(vrInputDevice *devinfo)
{
	_JoydevPrivateInfo	*aux = NULL;

	aux = (_JoydevPrivateInfo *)devinfo->aux_data;

	if (aux != NULL) {
		vrSerialClose(aux->fd);
		vrShmemFree(aux);	/* aka devinfo->aux_data */
	}

	return;
}


/************************************************************/
static void _JoydevResetFunction(vrInputDevice *devinfo)
{
#if 0 /* not yet used */
	_JoydevPrivateInfo	*aux = NULL;

	aux = (_JoydevPrivateInfo *)devinfo->aux_data;
#endif

	return;
}


/************************************************************/
static void _JoydevPollFunction(vrInputDevice *devinfo)
{
#if 0 /* not yet used */
	_JoydevPrivateInfo	*aux = NULL;

	aux = (_JoydevPrivateInfo *)devinfo->aux_data;
#endif

	if (devinfo->operating) {
		_JoydevGetData(devinfo);
	} else {
		/* TODO: try to open the device again */
	}

	return;
}



	/******************************/
	/*** FreeVR public routines ***/
	/******************************/


/******************************************************/
void vrJoydevInitInfo(vrInputDevice *devinfo)
{
	devinfo->version = (char *)vrShmemStrDup("-get from Joydev device-");
	devinfo->Create = vrCallbackCreateNamed("JoydevInput:Create-Def", _JoydevCreateFunction, 1, devinfo);
	devinfo->Open = vrCallbackCreateNamed("JoydevInput:Open-Def", _JoydevOpenFunction, 1, devinfo);
	devinfo->Close = vrCallbackCreateNamed("JoydevInput:Close-Def", _JoydevCloseFunction, 1, devinfo);
	devinfo->Reset = vrCallbackCreateNamed("JoydevInput:Reset-Def", _JoydevResetFunction, 1, devinfo);
	devinfo->PollData = vrCallbackCreateNamed("JoydevInput:PollData-Def", _JoydevPollFunction, 1, devinfo);
	devinfo->PrintAux = vrCallbackCreateNamed("JoydevInput:PrintAux-Def", _JoydevPrintStruct, 0);

	vrDbgPrintfN(INPUT_DBGLVL, "vrJoydevInitInfo: callbacks created.\n");
}


#endif /* } FREEVR */



	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/



#if defined(CAVE) /* { */


	/* ... CAVE stuff here if to also work with CAVElib */


#endif /* } CAVE */



	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/



#if defined(TEST_APP) /* { */

/******************************************************************/
/* Ugh, I hate globals, but I don't know a better way to get the  */
/*   aux value into the interrupt signal function exit_testapp(). */
static	int	done = 0;


/******************************************************************************/
/* report_nofile -- boolean flag to indicate whether an argument points to a  */
/*   non-existent file should be reported -- ie. for looping through specific */
/*   file names we want it, but looping over a range, we don't.               */
int JoydevInfo(char *device_filename, int report_nofile)
{
	int		fd = -1;		/* file-descriptor for communicating with the device */
	char		name[256]= "Unknown";	/* name of the joystick device */
	uint8_t		num_buttons;		/* number of buttons of the device */
	uint8_t		num_axes;		/* number of axes of the device */
	int		returnval;		/* a value returned by a system call */
	int		toreturn = 0;		/* the value this function will return */

	errno = 0;
	if ((fd = open(device_filename, O_RDONLY)) < 0) {
		/* errno = 2 -> no such file (ENOENT) */
		/* errno = 13 -> permission denied (EACCES) */
#if 0
		printf("open return value = %d, errno = %d\n", fd, errno);
#endif
		switch (errno) {
		case ENOENT:
			strcpy(name, "does not exist");
			break;
		case EACCES:
			strcpy(name, "is unavailable due to permission restrictions");
			break;
		}
		toreturn = errno;
	} else {

		/* request the name information -- * return value is the     */
		/*   length of the name, for success or -EFAULT for failure. */
		if ((returnval = ioctl(fd, JSIOCGNAME(sizeof(name)), name)) < 0) {
#if 0
			perror("joydev ioctl");
			printf("ioctl return value = %d, errno = %d\n", returnval, errno);
#endif
			/* errno = 22 -> Invalid Argument (EINVAL)  -- from wrong type of device (e.g. joydev device) */
			/* errno = 25 -> Not an JS device (ENOTTY) -- or really not an input device */
			switch (errno) {
			case ENOTTY:
				strcpy(name, "is not an input device");
				break;
			case EINVAL:
				strcpy(name, "is not a joydev input device");
				break;
			}
			toreturn = errno;
		} else {
			/* If we're here then we've successfully opened an JS device */
			if (ioctl(fd, JSIOCGBUTTONS, &num_buttons)) {
				perror("joydev ioctl for buttons");
			}
			if (ioctl(fd, JSIOCGAXES, &num_axes)) {
				perror("joydev ioctl for axes");
			}
		}
	}

	if (errno == 0) {
		printf("The device on " BOLD_TEXT "%s" NORM_TEXT " is a '" BOLD_TEXT "%s" NORM_TEXT "' with %d/%d buttons/axes.\n",
			device_filename, name, num_buttons, num_axes);
	} else if (errno != ENOENT || report_nofile == 1) {
		printf(RED_TEXT "The device on " BOLD_TEXT "%s" NORM_TEXT " %s\n" NORM_TEXT, device_filename, name);
	} else {
		/* For some cases we'll want to print file-doesn't-exist messages, but not others */
	}

	close(fd);

	return toreturn;
}


/*******************************************************************/
void exit_testapp()
{
#if 0 /* NOTE: I'm not sure which method is more desireable */
	signal(SIGINT, SIG_DFL);	/* set the signal action back to the default -- interruption. */
#else
	if (done == 1) {
		/* if we've already been here, this is a sign to force quit */
		printf("\nPersistent interruption... terminating.\n");

		exit(0);
	}
#endif

	done = 1;
	printf("\nInterruption -- if program doesn't close cleanly, try again.\n");
}


/*******************************************************************/
/* A test program to communicate with a Joydev device and print the results. */
main(int argc, char *argv[])
{
	_JoydevPrivateInfo	*aux;
	char			*progname;		/* name of the program executable */
	int			count;
	int			loop = 0;
	int			nodata_flag = 0;	/* whether or not to actual show input data */

	done = 0;
	signal(SIGINT, exit_testapp);


	/******************************/
	/* setup the device structure */
	aux = (_JoydevPrivateInfo *)malloc(sizeof(_JoydevPrivateInfo));
	memset(aux, 0, sizeof(_JoydevPrivateInfo));
	_JoydevInitializeStruct(aux, "Joydevsubtype");


	/*********************************************************/
	/* set default parameters based on environment variables */
	aux->devfile = getenv("JOYDEV_DEVICE");
	if (aux->devfile == NULL)
		aux->devfile = DEFAULT_DEVICE;		/* default, if no file given */


	/*****************************************************/
	/* adjust parameters based on command-line-arguments */
	progname = argv[0];
	while ((argc > 1) && (argv[1][0] == '-')) {
		/* Just list all the /dev/input/js<N> devices with their names */
		if (!strcmp(argv[1], "-list")) {
			char	file[128];

			for (count = 0; count < 512; count++) {
				sprintf(file, "/dev/input/js%d", count);
				JoydevInfo(file, 0);
			}
			exit(0);
		}

		/* Report information about the device and then quit */
		else if (!strcmp(argv[1], "-nodata")) {
			nodata_flag = 1;
			argv++; argc--;
		}

		/* Unknown option */
		else {
			/* There are currently no other "-" options, so this is an error */
			fprintf(stderr, RED_TEXT "Usage:" NORM_TEXT " %s [-list] | [-nodata] [<joydev device> (default = '%s')]\n", progname, aux->devfile);	/* NOTE: I'm reporting the default based on what might be changed by the environment variable */
			exit(1);
		}
	}

	/* if there are any arguments left, use the first as the device file path */
	if (argc > 1) {
		aux->devfile = strdup(argv[1]);
	}


	/**************************************************/
	/* open the device file and initialize the device */
	aux->fd = open(aux->devfile, O_RDONLY);
	if (aux->fd < 0) {
		fprintf(stderr, RED_TEXT "couldn't open joydev device %s\n" NORM_TEXT, aux->devfile);
		aux->open = 0;
		sprintf(aux->version, "- unconnected Joydev -");
	} else {
		if (_JoydevInitializeDevice(aux) < 0) {
			vrErrPrintf("main: " RED_TEXT "Warning, unable to initialize Joydev.\n" NORM_TEXT);
		}
		aux->open = 1;
	}

	_JoydevPrintStruct(stdout, aux, verbose);

	/* quit if flagged to print the device info but not data */
	if (nodata_flag)
		exit(0);

	/**********************/
	/* display the output */
	while(aux->open && !done) {
		if (_JoydevReadInput(aux) > 0) {
			loop++;
			printf("buttons: ");
			for (count = 0; count < aux->num_buttons; count++)
				printf("%d", aux->button[count]);
#if 1 /* text output of all the values */
			printf("  axes: ");
			for (count = 0; count < aux->num_axes; count++)
				printf("%6.2f", aux->axis[count]);
#else /* a little fancier text output */
			printf("  axes: |");
			for (count = 0; count < aux->num_axes; count++)
				printf("%5s|",
					(aux->axis[count] < -0.6 ? "-    " :
					 (aux->axis[count] < -0.3 ? " -   " :
					  (aux->axis[count] <  0.3 ? "  +  " :
					   (aux->axis[count] <  0.6 ? "   - " :
					    "    -")))));
#endif
			printf("    \r");
			fflush(stdout);

			/* Alternate mode of quitting is to press buttons 1 & 2 */
			/* NOTE: for some reason the Linux joydev interface */
			/*   reports the previously held buttons as the     */
			/*   "initial value", and so when running "joytest" */
			/*   after having quit, it would instantly quit     */
			/*   without the loop count test.                   */
			if ((aux->num_buttons >= 2) && (loop > 1)) {
				if (aux->button[0] && aux->button[1])
					done = 1;
			}

			/* For printing the internal data in the middle of testing */
			/*   use the chorded input of buttons 1 & 3                */
			if (aux->num_buttons >= 3) {
				if (aux->button[0] && aux->button[2]) {
					printf("\n");
					_JoydevPrintStruct(stdout, aux, verbose);
				}
			}
		}
	}

	printf("\n");

	/*****************/
	/* close up shop */
	if (aux != NULL) {
		close(aux->fd);
		free(aux);			/* aka devinfo->aux_data */
	}

#if 0
	_JoydevCloseDevice(aux);
	vrPrintf(BOLD_TEXT "\nJoydev device closed\n" NORM_TEXT);
#endif
}

#endif /* } TEST_APP */

#if defined(MAN_PAGE) /* {  :set syntax=nroff  */
.\"* ======================================================================= "
.\"*                                                                         "
.\"*   11            joytest.1                                               "
.\"* .111            Author(s): Bill Sherman                                 "
.\"*   11            Created: August 19, 2013                                "
.\"*   11            Last Modified: August 19, 2013                          "
.\"* 111111                                                                  "
.\"*                                                                         "
.\"* Man page for the FreeVR test program for the joydev Linux input system. "
.\"*                                                                         "
.\"* Copyright 2013, Bill Sherman, All rights reserved.                      "
.\"* With the intent to provide an open-source license to be named later.    "
.\"* ======================================================================= "
.\"********************************* TITLE ********************************* "
.\" the ".TH" title line must be the first non-comment line in the file      "
.\" .TH <title> <section> <date> <source> <manual>                           "
.TH JOYTEST 1 "21 August 2013" "FreeVR 0.6d" "FreeVR Commands"

.\" ********************************* NAME ********************************* "
.\" The NAME section should have the name in bold followed by a dash, followed   "
.\"   by a one-line description which can be used in the whatis/apropos database "
.\" .SH <section header name>                                                "
.SH NAME

.B joytest
\- test the setup of an joystick device connected via the Linux
\fIjoydev\fP input system.
.\" ******************************* SYNOPSIS ******************************* "
.\" .SH <section header name>                                                "
.SH SYNOPSIS

\fBjoytest\fI [-list] | [-nodata] [-repunk] [<\fIevent device\fP>]
.\" ****************************** DESCRIPTION ***************************** "
.\" .SH <section header name>                                                "
.SH DESCRIPTION

The \fBjoytest\fP program is used to interface with devices connected via
the Linux joydev input system.
The \fBjoytest\fP program can be used to list joystick devices and provide
their self-reported names, or to provide a live report of the inputs.
.PP
Before rendering the input stream, \fBjoytest\fP will output information
specific to the input device such as the number of all button
inputs, axis (aka valuator) inputs.
.PP
The program is terminated by pressing the interrupt key (usually ^C).
Note that sometimes this doesn´t fully work the first time, but it will
work the second time.
.\" ******************************* OPTIONS ******************************** "
.\" .SH <section header name>                                                "
.SH OPTIONS

.TP 0.5i
.B -list
The \fB-list\fP option lists all Linux joystick devices of the form
"\fI/dev/input/js<N>\fP", where N is from [0:512].
It will only list devices that exist.
For devices that exist, but for which the current user does not have
permission to access, then this information will be reported.
All other arguments are ignored when "\fB-list\fP" is specified.
.br
.TP 0.5i
.B -nodata
The \fB-nodata\fP option reports detailed information about the device and
then exits.
The reported data includes the name of the device, the manufacturer id,
and item code as well as the name and number of all inputs.
.\" ******************************* ARGUMENTS ****************************** "
.\" .SH <section header name>                                                "
.SH ARGUMENTS

.TP 0.5i
.B <joydev device>
The \fI<joydev device>\fP argument is a filesystem path pointing to a specific
Linux input event device \- for example "/dev/input/js0".
.\" ************************* ENVIRONMENT VARIABLES ************************ "
.\" .SH <section header name>                                                "
.SH ENVIRONMENT VARIABLES

.TP 0.5i
.B JOYDEV_DEVICE
Set the path of the default joystick device to read when no device argument
is provided.
.\" ******************************* EXAMPLES ******************************* "
.\" .SH <section header name>                                                "
.SH EXAMPLES

.TP 0.5i
List all Linux input event devices:
% \fBjoytest\fP -list
.br
.TP 0.5i
Report inputs from the device at "/dev/input/js1":
% \fBjoytest\fP /dev/input/js1
.br
.TP 0.5i
Set the default input device to be "/dev/input/js1" and then report those events:
% setenv JOYDEV_DEVICE /dev/input/js1
.br
% \fBjoytest\fP
.\" ********************************* BUGS ********************************* "
.\" .SH <section header name>                                                "
.SH BUGS

The fact that \fBjoytest\fP does not always terminate when receiving the
first interrupt signal (ie. ^C) may be considered a bug.  The workaround
is already coded in the program \- just send a second interrupt.
.\" ********************************* TODO ********************************* "
.\" .SH <section header name>                                                "
.SH TODO

.HP 0.5i
\fB*\fP Report the joystick calibration information
.HP 0.5i
\fB*\fP It would be nice to manipulate device force feedback events
.HP 0.5i
\fB*\fP Implement a screen-rendering option
.\" ******************************* SEE ALSO ******************************* "
.\" .SH <section header name>                                                "
.SH SEE ALSO

eviotest(1), fobtest(1), ...
.\" TODO: finish "see also"                                                  "
.\" ******************************* COPYRIGHT ****************************** "
.\" .SH <section header name>                                                "
.SH COPYRIGHT

Copyright 2013, Bill Sherman, All rights reserved.
.\"With the intent to provide an open-source license to be named later.      "
.\" ****************************** OTHER NOTES ***************************** "
.\" .SH <section header name>                                                "
.SH OTHER NOTES

The source code for \fBjoytest\fP is in the "\fIvr_input.joydev.c\fP" file,
which also handles the joydev input interface to the \fBFreeVR\fP library.

.\"* ======================================================================= "
#endif /* } MAN_PAGE */

