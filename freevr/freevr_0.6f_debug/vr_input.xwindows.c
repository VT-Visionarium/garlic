/* ======================================================================
 *
 *  CCCCC          vr_input.xwindows.c
 * CC   CC         Author(s): Bill Sherman, John Stone
 * CC              Created: October 14, 1999
 * CC   CC         Last Modified: June 13, 2006
 *  CCCCC
 *
 * Code file for FreeVR inputs from X-Windows events.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/*************************************************************************

FreeVR USAGE:
	Inputs that can be specified with the "input" option:
		input "<name>" = "2switch(keyboard:key[<keyname>])";
		input "<name>" = "2switch(mouse:button[{left|middle|right|<num>}])";
		input "<name>" = "valuator(pointer[{-| }{x|y|xscreen|yscreen}])";
		input "<name>" = "6sensor(sim6[<number>])";
		input "<name>" = "2switch(<device>:button[<num>])";
		input "<name>" = "valuator(<device>:valuator[<num>{, <scale>{, <shift>| }| }])";
	  :-(	input "<name>" = "6sensor(<device>:sim[<num>, {x|y|z|a|e|r}{, <scale>{, <shift>| }| }])";
	 	input "<name>" = "Nswitch(keyboard[0{ | , last|, release|, count}])";
	 	input "<name>" = "2switch(keyboard[0])";
	 	input "<name>" = "valuator(keyboard[0, <scale>])";


	Self-Controls are specified with the "control" option:
		control "<control option>" = "2switch(keyboard:key[<key name>])";
		control "<control option>" = "2switch(mouse:button[{left|middle|right|<num>}])";
		control "<control option>" = "2switch(<device>:button[<num>])";
		control "<control option>" = "valuator(<device>:valuator[<num>])";

	Here are the available control options for FreeVR:
		"print_help" -- print info on how to use the input device
		"system_pause_toggle" -- toggle the system pause flag
		"print_context" -- print the overall FreeVR context data structure (for debugging)
		"print_config" -- print the overall FreeVR config data structure (for debugging)
		"print_input" -- print the overall FreeVR input data structure (for debugging)
		"print_struct" -- print the internal Xwindows data structure (for debugging)

		"sensor_reset" -- reset the current 6-sensor
		"sensor_resetall" -- reset the all the 6-sensors
		"sensor_next" -- jump to the next 6-sensor on the list
		"setsensor(<num>)" -- set the simulated sensor to a particular one
		"sensor_left" -- move the current sensor to the left
		"sensor_right" -- move the current sensor to the right
		"sensor_up" -- move the current sensor up
		"sensor_down" -- move the current sensor down
		"sensor_in" -- move the current sensor in
		"sensor_out" -- move the current sensor out
		"sensor_pazim" -- rotate positively about the azimuth
		"sensor_nazim" -- rotate negatively about the azimuth
		"sensor_pelev" -- rotate positively about the elevation
		"sensor_nelev" -- rotate negatively about the elevation
		"sensor_cw" -- roll the current sensor clockwise
		"sensor_ccw" -- roll the current sensor counter-clockwise
		"sensor_swap_yz" -- temporarily swap up/down key movement for in/out movement
		"sensor_rotate_sensor" -- temporarily (key) rotate the sensor instead of translate
		"pointer_xy_override" -- temporarily use pointer movement for sensor translation
		"pointer_xz_override" -- temporarily use pointer movement for sensor translation
		"pointer_rot_override" -- temporarily use pointer movement for sensor rotation
		"pointer_valuator" -- temporarily activate use of pointer as valuators
		"toggle_relative" -- toggle whether movement is relative to sensor's position
		"toggle_space_limit" -- toggle whether 6-sensor can go outside working volume
		"toggle_return_to_zero" -- toggle whether return-to-zero operation is on
		"toggle_silence" -- toggle whether keyboard gives aural feedback to controls
		"keyval_sign(<num>)" -- toggle the sign of a keyboard valuator
		"keyval_zero(<num>)" -- zero the value of a keyboard valuator

		"bell" -- ring the keyboard bell
		"softbell" -- ring the keyboard bell softer (only on keyboards with variable volume)

	These controls would be nice, but non-integer callbacks need to be implemented first:
	  :-|	"sensor_leftright_val" -- move the current sensor left & right using a valuator
	  :-|	"sensor_updown_val" -- move the current sensor up & down using a valuator
		...


	Here are the FreeVR configuration argument options for the XWindows Device:
		"window[s]" <window name> [, <window name>]*; -- choose the window(s) from
			which to get events.  A special input-only window can also be
			specified by simply adding square-brackets after the name, with
			any window arguments inside the brackets.  These arguments are
			the same as those of a GLX visren window, with the only difference
			being that a comma must be used to separate arguments rather
			than a semicolon.
			NOTE: the primary restriction is that all input windows must be
			on the same Xdisplay device.  Any change to a different display
			will invalidate all the previous windows on the list.

			Here are the available arguments for input-only windows:
				"display=[host]:server[.screen]" -- set the display where the window should appear
				"geometry=[WxH][+X+Y]" -- set where on the display the window should appear
				"decoration={borders|title|minmax|window|none}*" -- set look of window
					(use "none" when windows will be joined with neighboring windows)
				"title=window title" -- set the title on the window title bar
				"cursor={default|blank|dot|bigdot}" -- set the name of the cursor
		"silent" -- boolean choice of whether keyboard gives aural feedback
			to control events
		"keyrepeat" -- boolean choice of whether keyboard keys will repeat when
			mouse pointer is inside the window (off by default, since most
			of the control functions are based on up/down events).
		"sensor_swap_yz" -- boolean choice of whether to start in in/out mode.
		"sensor_rotate_sensor" -- boolean choice of whether to start in rotate mode.
		"sensor_transScale" -- float value of the translational sensitivity scale
		"sensor_rotScale" -- float value of the rotational sensitivity scale
		"sensor_restrict" -- boolean choice of whether sensor can leave working volume
		"sensor_returnToZero" -- whether 6sensors return to zero when control is released
		"sensor_relativeRot" -- boolean choice of whether rotations should be about
			the 6-sensor's coordsys, or the world's coordsys.
		"sensor_workingVolume" -- 6 numbers that describe a parallelpiped in which
			6-sensors can roam.

	An Example FreeVR specification:
		TODO: ...

HISTORY:
	14-15 October 1999 -- First version written by Bill Sherman, with a
		little help in debugging from Stuart Levy.

	1 November 1999 (Bill Sherman) -- revised the FreeVR input interface
		to use a new method that will make the mapping of inputs
		in FreeVR far more flexible.

	29 December 1999 (Bill Sherman) -- brought the X-windows device code
		up to date with the new format of input device source
		files, including the new CREATE section of _XwindowsFunction().

	27 January 2000 (Bill Sherman) -- Integrated new self-control creation
		method.  And wrote a routine and function for the "print_struct"
		device control.

	2-11 February 2000 (Bill Sherman) -- Expanded the number of types of
		inputs possible with Xwindows.  Including mouse button presses,
		mouse motion events, control callbacks.  Implemented most of the
		control callbacks, and the argument parsing.  This device now
		performs nearly all the functions of the CAVE library simulator.

	25 February 2000 (Bill Sherman) -- Added new "keyrepeat", "sensor_swap_upin",
		and "sensor_rotate_sensor" argument options.  Plus allowed upper
		case characters to be used in place of lower case chars (ie. specifying
		a '?' will be treated as a '/' -- of course, this is probably specific
		to U.S. keyboards.  I began to implement the other-device usage, but
		ran into a few troubles, including the near total destruction of the
		OS on my Indigo 2.

	5-6 March 2000 (Bill Sherman) -- Finished (mostly) implementing the X
		extension devices for use as inputs.  It has certainly been
		sufficiently implemented for use where dial & button devices
		are used as the primary input (eg. CAVE[tm] systems).  In addition,
		controls can also be manipulated using buttons from these
		devices.  I implemented two test callbacks for using valuators
		as a means of controlling (special valuator-) callbacks.  However,
		the vr_callback.c routines aren't setup to handle non-integer
		callback values.  This will need to change before these can be
		implemented.  I also fixed the hardcoded references to window size
		to instead get the values from the window.

	8 February 2001 (Bill Sherman) -- Added the long anticipated "workingvolume"
		argument.

	3 May 2001 (Bill Sherman)
		I made a few minor changes to catch up to the general
		  vr_input.skeleton.c format.

	21 June 2001 (Bill Sherman)
		I set the oob flags of simulated 6-sensors to be in-bounds for
		  the sensor that is the "active_sim6sensor" (nee "active_sensor"),
		  and out-of-bounds for the others.

	20 April 2002 (Bill Sherman)
		I changed the last entry to use the new 6-sensor "active"
		  field rather than oob (since this is a cleaner thing to do).
		  Also set the new "error" flag to 0 everywhere -- I can't
		  think of any cases that would be in error at the moment.
		NOTE: the "oob" flag is now properly being set in the
		  vrAssign6sensorValueFromValuators() function in vr_input.c.

	17,20 May 2002 (Bill Sherman)
		Changed variable type names where possible (sensor6 to 6sensor,
		  etc), and renamed X,Y,Z,etc to VR_X, VR_Y, etc.

	11 September 2002 (Bill Sherman)
		Moved the control callback array into the _XwindowsFunction() callback.

	15 January 2003 (Bill Sherman)
		Fixed a seldom-seen bug that only happens when the visren process
		dies too soon.  Also renamed a variable.

	21-30 April 2003 (Bill Sherman)
		Updated to new input matching function code style.  Changed "opaque"
		  field to "aux_data".  Split _XwindowsFunction() into 5 functions.
		  Added new vrPrintStyle argument to _XwindowsPrintStruct() for the
		  sake of the new "PrintAux" callback.
		Added code to read a whole keyboard as an N-switch (individual key
		  buttons take precedence, so this gets the leftovers).  Can also
		  make binary and valuator inputs from the whole keyboard.
		Also began adding the X-input-only window code.
		As part of the new X-input-only changes, I implemented the ability to
		  connect with multiple input windows, be they input-output or input-only.

	21 May 2003 (Bill Sherman)
		Added the "system_pause_toggle" control.

	3 June 2003 (Bill Sherman)
		Added the address of the auxiliary data to the printout.
		Added comments classifying the controls.  Also re-ordered some
			of the callbacks to be more consistent with other files.
		Now use the "trans_scale" and "trans_rot" fields of vr6SensorConv
			instead of local copies.  Also use the "swap_transrot"
			field of vr6SensorConv instead of a local copy -- which
			meant the removal of a lot of code.
		Changed references of vrNow() to vrCurrentWallTime().

	10 February 2006 (Bill Sherman)
		Added a compile-time check to make the left-alt key function
			as expected on an Apple Mac w/ OS/X.

	10 March 2006 (Bill Sherman)
		Moved the use of the "sensor6_swap_upin" field of
			_XwindowsPrivateInfo into the new vr6sensorConv
			structure available for all input device drivers
			that require the creation of a simulated 6-sensor
			input type.  (Now known as "swap_yz".)

	16 October 2009 (Bill Sherman)
		A quick fix to the _GlxParseArgs() routine to handle the
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
	- fix problem caused by overloading of commas in input-only
		argument string (ie. the comma separates the decoration
		list, as well as all options)

	- DONE: implement "print_help" callback

	- implement N-switch inputs using pointer location, (divided
		into so-many sections).

	- implement the valuator control inputs (requires Dial & button box
		to be connected).

	- fix application of r2e transform to the sensor positions
		(this is probably a general vr_input.c problem)

*************************************************************************/
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>  /* needed for tolower() */

#include <X11/keysym.h>
#include <X11/extensions/XInput.h>	/* requires -lXi, needed by XListInputDevices(), XSelectExtensionEvent() and associated types */

#include "vr_config.h"
#include "vr_parse.h"
#include "vr_input.h"
#include "vr_input.opts.h"
#include "vr_visren.h"
#include "vr_visren.glx.h"		/* included exclusively for the Motif definitions */
#include "vr_shmem.h"
#include "vr_debug.h"

#define	KEY_TRANSL_DELTA	0.04
#define	KEY_ROTATE_DELTA	0.04
#define POINTER_TRANSL_DELTA	0.04	/* new-new value as of 05/24/2011 */ /* 0.01 was new value as of 03/26/2007 */
#define POINTER_ROTATE_DELTA	0.07	/* new-new value as of 05/24/2011 */ /* 0.05 was new value as of 03/26/2007 */

#define	MAX_WINDOWS	32

/******************************************************************/
/*** auxiliary structure of the current data from the XWindows. ***/
typedef struct {
		char		*window_list;			/* list of windows from which to get input * TODO: do we need to allocate some mem? */
		int		window_count;			/* number of windows from which to gather input */
		Display		*display;			/* pointer to the active Xwindows display */
		long		event_mask;			/* Xevents this device is interested in */

		Display		*display_array[MAX_WINDOWS];	/* pointer to the displays of all the specified windows */
		int		xscreen_num[MAX_WINDOWS];	/* number of the Xscreen of the display */
		Window		xwindow[MAX_WINDOWS];		/* handle to the Xwindows window */
		vrWindowInfo	*window[MAX_WINDOWS];		/* pointer to the FreeVR window structure */
		int		found_req_window[MAX_WINDOWS];	/* boolean indicating if using requested window */
#if 0 /* TODO: these don't seem to be used at the moment [4/24/03] */
		int		no_window_found[MAX_WINDOWS];	/* boolean indicating if we've found any window */
		int		selected;			/* flag to indicate XSelectInput() was done */
#endif

		/********************/
		/* standard devices */
#define MAX_KEYINPUTS		512
		vrGenericInput	**key_inputs;				/* array of pointers to key input controls */

#define	MAX_BUTTONINPUTS	 10
		vrGenericInput	*button_inputs[MAX_BUTTONINPUTS];	/* array of pointers to (mouse) button input controls */

#define	MAX_POINTERINPUTS	  4
		vrGenericInput	*pointer_inputs[MAX_POINTERINPUTS];	/* array of pointers to pointer input controls */
		float		pointer_sign[MAX_POINTERINPUTS];

#define	MAX_6SENSORS		 10
		vrGenericInput	*sensor6_inputs[MAX_6SENSORS];
#if 1
		vrEuler		sensor6_values[MAX_6SENSORS];		/* not really used much now */
#endif

#define MAX_KEYBOARDS		  1
		vrGenericInput	*keyboard_inputs[MAX_KEYBOARDS];	/* array of pointers to keyboard inputs */
#define KEYBOARD_LAST		1
#define KEYBOARD_RELEASE	2
#define KEYBOARD_COUNT		3
		int		keyboard_type[MAX_KEYBOARDS];		/* type of input function ("last", "release" or "count") */
		float		keyboard_scale[MAX_KEYBOARDS];		/* scale for value of each key in a valuator keyboard (also affects sign) */
		float		keyboard_count[MAX_KEYBOARDS];		/* current count of depressed keys of a keyboard */

		/*********************/
		/* extension devices */
#define MAX_EXDEVICES		 16
		int		num_exdevices;
		XDevice		*exdevice[MAX_EXDEVICES];
		char		*exdevice_names[MAX_EXDEVICES];
		XID		exdevice_id[MAX_EXDEVICES];
		XEventClass	exevents_list[32];
		int		exevents_count;

#define	MAX_EXTINPUTS		128
		int		num_extinputs;
		vrGenericInput	*extension_inputs[MAX_EXTINPUTS];	/* pointer to vrInput */
#define BUTTON_TYPE 1
#define MOTION_TYPE 2
		int		extinput_type[MAX_EXTINPUTS];		/* type of events to watch */
		float		extinput_scale[MAX_EXTINPUTS];		/* scale the valuator -- possible negation */
		float		extinput_shift[MAX_EXTINPUTS];		/* shift the valuator */
		int		extinput_edev[MAX_EXTINPUTS];		/* reference to exdevice list */
		int		extinput_event_val[MAX_EXTINPUTS];	/* valuator event type */
		int		extinput_event_press[MAX_EXTINPUTS];	/* button press event type*/
		int		extinput_event_release[MAX_EXTINPUTS];	/* button release event   */
		int		extinput_evnum[MAX_EXTINPUTS];		/* number of event (eg. button number) */
		char		extinput_simval[MAX_EXTINPUTS];		/* which value of simulated sensor to control */

		/******************************************/
		/* incoming data, and how to interpret it */

#define X_WINDOW	0
#define Y_WINDOW	1
#define X_SCREEN	2
#define Y_SCREEN	3
		float		pointer[4];		/* values of the four pointer choices */

		int		pointer_as_valuator;
		int		pointer_as_6sensorXY;
		int		pointer_as_6sensorXZ;
		int		pointer_as_6sensorRot;
		int		active_sim6sensor;
		vrEuler		sensor6_delta;
		vr6sensorConv	sensor6_options;

		int		silent;		/* boolean to determine noise factor */
		int		keyrepeat;	/* boolean to indicate if keys should repeat */

	} _XwindowsPrivateInfo;



	/***********************************/
	/*** General NON public routines ***/
	/***********************************/


/**************************************************************************/
/* typename is used to specify a particular device among many that */
/*   share (more or less) the same protocol.  The typename is then */
/*   used to determine what specific features are available with   */
/*   this particular type of device.  In the case of the X-windows */
/*   interface, there currently is only one type, so the argument  */
/*   is not present.                                               */
static void _XwindowsInitializeStruct(_XwindowsPrivateInfo *aux)
{
	int	count;

	/* allocate arrays based on number of possible inputs */
	aux->key_inputs = (vrGenericInput **)vrShmemAlloc0(MAX_KEYINPUTS * sizeof(vrGenericInput *));

	/* initialize each potential window source of X events */
	aux->window_count = 0;
	aux->display = NULL;
	for (count = 0; count < MAX_WINDOWS; count++) {
		aux->display_array[count] = NULL;
		aux->xscreen_num[count] = -1;
		aux->xwindow[count] = 0;
		aux->window[count] = NULL;
		aux->found_req_window[count] = 0;
#if 0 /* not currently used [4/24/03] */
		aux->no_window_found[count] = 0;
		aux->selected[count] = 0;
#endif
	}

	/* set input defaults */
	aux->silent = 1;				/* be quiet by default */
	aux->keyrepeat = 0;				/* disable key repeating by default */

	aux->pointer[0] = 0.0;
	aux->pointer[1] = 0.0;
	aux->pointer[2] = 0.0;
	aux->pointer[3] = 0.0;

	aux->pointer_as_valuator = 0;
	aux->pointer_as_6sensorXY = 0;
	aux->pointer_as_6sensorXZ = 0;
	aux->pointer_as_6sensorRot = 0;

	aux->active_sim6sensor = 0;
	aux->sensor6_options.azimuth_axis = VR_Y;	/* azimuth is about the Y axis */
	aux->sensor6_options.relative_axis = 0;		/* default to absolute rotations */
	aux->sensor6_options.return_to_zero = 0;	/* default to free floating */
	aux->sensor6_options.ignore_all = 0;		/* (unused by Xwindows input devices) */
	aux->sensor6_options.ignore_trans = 0;		/* (unused by Xwindows input devices) */
	aux->sensor6_options.restrict_space = 1;	/* default restricted to CAVE-space */
	aux->sensor6_options.trans_scale = 1.0;		/* default translational scaling is the identity */
	aux->sensor6_options.rot_scale = 5.0;		/* default rotational scaling. */
	aux->sensor6_options.swap_yz = 0;		/* default to standard Y & Z axes */
	aux->sensor6_options.swap_transrot = 0;		/* default to unswapped translation & rotation */

	/* the default working volume is that of the typical CAVE */
	aux->sensor6_options.working_volume_min[VR_X] = -5.0;
	aux->sensor6_options.working_volume_max[VR_X] =  5.0;
	aux->sensor6_options.working_volume_min[VR_Y] =  0.0;
	aux->sensor6_options.working_volume_max[VR_Y] = 10.0;
	aux->sensor6_options.working_volume_min[VR_Z] = -5.0;
	aux->sensor6_options.working_volume_max[VR_Z] =  5.0;

	aux->sensor6_delta.t[VR_X] = 0.0;
	aux->sensor6_delta.t[VR_Y] = 0.0;
	aux->sensor6_delta.t[VR_Z] = 0.0;
	aux->sensor6_delta.r[VR_AZIM] = 0.0;
	aux->sensor6_delta.r[VR_ELEV] = 0.0;
	aux->sensor6_delta.r[VR_ROLL] = 0.0;

	/* Setup the initial mask of events to watch for.  Others will */
	/*   be added later, as required by the given inputs.          */
	aux->event_mask =
#if 0 /* 06/13/2006 -- it was decided to use the FocusIn/Out events instead of Enter/LeaveNotify */
		EnterWindowMask | LeaveWindowMask |	/* these are to enable/disable key repeat */
#endif
		FocusChangeMask |			/* probably a better means of en/disabling key repeat */
		/* TODO: ?? ButtonMotionMask | */
		/* TODO: ?? FocusChangeMask | */
		0; /* TODO: ... */
}


/**************************************************************************/
static void _XwindowsPrintStruct(FILE *file, _XwindowsPrivateInfo *aux, vrPrintStyle style)
{
	char		charkey[] = "'x'";	/* NOTE: must be listed as an array or it's in fixed (read-only) memory space */
	char		*keyname;
	int		count;
#undef PRINT_S6_VALUES
#  ifdef PRINT_S6_VALUES
	vrVector	vector;
#  endif

	vrFprintf(file, "Xwindows device internal structure (%#p):\n", aux);
	vrFprintf(file, "\r\twindow list -- '%s' (%d initialized)\n", aux->window_list, aux->window_count);
	vrFprintf(file, "\r\tX-display active -- %#p\n", aux->display);
	vrFprintf(file, "\r\t\tX-event mask -- 0x%x\n", aux->event_mask);
	for (count = 0; count < aux->window_count; count++) {
		vrFprintf(file, "\r\tWindow[%d]:\n", count);
		vrFprintf(file, "\r\t\tX-display -- %#p\n", aux->display_array[count]);
		vrFprintf(file, "\r\t\tX-screen -- %d\n", aux->xscreen_num[count]);

		vrFprintf(file, "\r\t\tX-window -- %#p\n", aux->xwindow[count]);
		vrFprintf(file, "\r\t\tFreeVR-window -- %#p\n", aux->window[count]);

		vrFprintf(file, "\r\t\tfound_req_window -- %d\n", aux->found_req_window[count]);
#if 0 /* Not currently used [4/24/03] */
		vrFprintf(file, "\r\t\tno_window_found -- %d\n", aux->no_window_found[count]);
		vrFprintf(file, "\r\t\tXSelectInput() done -- %d\n", aux->selected[count]);
#endif
	}

	vrFprintf(file, "\r\tsilent -- %d\n", aux->silent);
	vrFprintf(file, "\r\tkeyrepeat -- %d\n", aux->keyrepeat);

	/* Unlike most other input devices, only print the keyinputs that are */
	/*   in use.  This is because there are just too darn many of them.   */
	vrFprintf(file, "\r\tkey inputs:\n");
	for (count = 0; count < MAX_KEYINPUTS; count++) {
		if (aux->key_inputs[count] != NULL) {
			if (count < 256 && isprint(count)) {
				charkey[1] = (char)count;
				keyname = charkey;
			} else
			   if (count < 256)
				keyname = XKeysymToString(count);
			else 	keyname = XKeysymToString(count + 0xFE00);

			vrFprintf(file, "\r\t\tkeyinput[%3x] (%-7s) = %#p (%s:%s)\n",
				count, keyname,
				aux->key_inputs[count],
				vrInputTypeName(aux->key_inputs[count]->input_type),
				aux->key_inputs[count]->my_object->name);
		}
	}

	vrFprintf(file, "\r\tmouse button inputs:\n");
	for (count = 0; count < MAX_BUTTONINPUTS; count++) {
		if (aux->button_inputs[count] != NULL) {
			vrFprintf(file, "\r\t\tmousebutton[%d] = %#p (%s:%s)\n",
				count,
				aux->button_inputs[count],
				vrInputTypeName(aux->button_inputs[count]->input_type),
				aux->button_inputs[count]->my_object->name);
		}
	}

	vrFprintf(file, "\r\tpointer valuator inputs:\n");
	for (count = 0; count < MAX_POINTERINPUTS; count++) {
		if (aux->pointer_inputs[count] != NULL) {
			vrFprintf(file, "\r\t\tpointer[%d] * %4.1f = %#p (%s:%s)\n",
				count,
				aux->pointer_sign[count],
				aux->pointer_inputs[count],
				vrInputTypeName(aux->pointer_inputs[count]->input_type),
				aux->pointer_inputs[count]->my_object->name);
		}
	}

	vrFprintf(file, "\r\tkeyboard (n-way) inputs:\n");
	for (count = 0; count < MAX_KEYBOARDS; count++) {
		if (aux->keyboard_inputs[count] != NULL) {
			vrFprintf(file, "\r\t\tkeyboard[%d] = %#p (%s:%s), type = %d, scale = %f\n",
				count,
				aux->keyboard_inputs[count],
				vrInputTypeName(aux->keyboard_inputs[count]->input_type),
				aux->keyboard_inputs[count]->my_object->name,
				aux->keyboard_type[count],
				aux->keyboard_scale[count]);
		}
	}

	vrFprintf(file, "\r\tX-extension devices (%d):\n", aux->num_exdevices);
	for (count = 0; count < aux->num_exdevices; count++) {
		vrFprintf(file, "\r\t\t%d: '%s'\n", count, aux->exdevice_names[count]);

		/* TODO: print the device info for each */

	}

	vrFprintf(file, "\r\textension device inputs:\n");
	for (count = 0; count < MAX_EXTINPUTS; count++) {
		if (aux->extension_inputs[count] != NULL) {
			switch (aux->extension_inputs[count]->input_type) {

			/* TODO: unless something is added here, the switch within */
			/*   the "case" of VRINPUT_CONTROL is sufficient here.     */

			case VRINPUT_BINARY:
				vrFprintf(file, "\r\t\tex-input[%d]: button[%d] = %#p (%s:%s-%s)\n",
					count,  /* the index */
					aux->extinput_evnum[count],
					aux->extension_inputs[count],
					vrInputTypeName(aux->extension_inputs[count]->input_type),
					aux->exdevice_names[aux->extinput_edev[count]],
					aux->extension_inputs[count]->my_object->name);
				break;

			case VRINPUT_VALUATOR:
				vrFprintf(file, "\r\t\tex-input[%d]: (valuator[%d]+%4.1f) * %4.1f = %#p (%s:%s-%s)\n",
					count,  /* the index */
					aux->extinput_evnum[count],
					aux->extinput_shift[count],
					aux->extinput_scale[count],
					aux->extension_inputs[count],
					vrInputTypeName(aux->extension_inputs[count]->input_type),
					aux->exdevice_names[aux->extinput_edev[count]],
					aux->extension_inputs[count]->my_object->name);
				break;

			case VRINPUT_6SENSOR:
				/* TODO: implement this -- once (if) 6-sensors are in use */
				break;

			case VRINPUT_CONTROL:
				switch (aux->extinput_type[count]) {
				case BUTTON_TYPE:
					vrFprintf(file, "\r\t\tex-input[%d]: button[%d] = %#p (%s:%s-%s)\n",
						count,  /* the index */
						aux->extinput_evnum[count],
						aux->extension_inputs[count],
						vrInputTypeName(aux->extension_inputs[count]->input_type),
						aux->exdevice_names[aux->extinput_edev[count]],
						aux->extension_inputs[count]->my_object->name);
					break;

				case MOTION_TYPE:
					vrFprintf(file, "\r\t\tex-input[%d]: (valuator[%d]+%4.1f) * %4.1f = %#p (%s:%s-%s)\n",
						count,  /* the index */
						aux->extinput_evnum[count],
						aux->extinput_shift[count],
						aux->extinput_scale[count],
						aux->extension_inputs[count],
						vrInputTypeName(aux->extension_inputs[count]->input_type),
						aux->exdevice_names[aux->extinput_edev[count]],
						aux->extension_inputs[count]->my_object->name);
					break;

				}
				break;

			default:
				vrFprintf(file, "\r\t\t%d: unknown type of extension input\n", count);
				break;
			}
		}
	}

	/* TODO: ?? more of the sensor-6 control values need to be printed */
#undef PRINT_S6_VALUES
	vrFprintf(file, "\r\tsimulated 6-sensor inputs (active = %d):\n", aux->active_sim6sensor);
	for (count = 0; count < MAX_6SENSORS; count++) {
		if (aux->sensor6_inputs[count] != NULL) {
#if 0 /* I don't use the sensor6_values really anymore */
			vrFprintf(file, "\r\t\t6sensor[%d] = %#p (%s:%s)"
#  ifdef PRINT_S6_VALUES
				" -- (%5.2f %5.2f %5.2f  %5.2f %5.2f %5.2f) "
#  endif
				"\n",
				count,
				aux->sensor6_inputs[count],
				vrInputTypeName(aux->sensor6_inputs[count]->input_type),
				aux->sensor6_inputs[count]->my_object->name
#  ifdef PRINT_S6_VALUES
				, aux->sensor6_values[count].t[VR_X],
				aux->sensor6_values[count].t[VR_Y],
				aux->sensor6_values[count].t[VR_Z],
				aux->sensor6_values[count].r[VR_AZIM],
				aux->sensor6_values[count].r[VR_ELEV],
				aux->sensor6_values[count].r[VR_ROLL],
#  endif
				);
#else

#  if PRINT_S6_VALUES
			vrVectorGetTransFromMatrix(&vector, ((vr6sensor *)(aux->sensor6_inputs[count]))->position);
#   endif
			vrFprintf(file, "\r\t\t6sensor[%d] = %#p (%s:%s)"
#  if PRINT_S6_VALUES
				" -- (%5.2f %5.2f %5.2f  -- -- --) "
#endif
				"\n",
				count,
				aux->sensor6_inputs[count],
				vrInputTypeName(aux->sensor6_inputs[count]->input_type),
				aux->sensor6_inputs[count]->my_object->name
#  if PRINT_S6_VALUES
				, vector.v[VR_X], vector.v[VR_Y], vector.v[VR_Z],
#  endif
				);
#  if 0 /* print details of each input when debugging */
			vrFprintInputObject(file, aux->sensor6_inputs[count]->my_object);
#  endif
#endif
		}
	}
	vrFprintf(file, "\r\tsimulated 6-sensor options:\n"
		"\t\tazimuth axis = %d\n"
		"\t\trelative axis = %d\n"
		"\t\treturn to zero = %d\n"
		"\t\trestrict space = %d\n"
		"\t\tworking volume[X] = % 6.2f -- % 6.2f\n"
		"\t\tworking volume[Y] = % 6.2f -- % 6.2f\n"
		"\t\tworking volume[Z] = % 6.2f -- % 6.2f\n",
		aux->sensor6_options.azimuth_axis,
		aux->sensor6_options.relative_axis,
		aux->sensor6_options.return_to_zero,
		aux->sensor6_options.restrict_space,
		aux->sensor6_options.working_volume_min[VR_X],
		aux->sensor6_options.working_volume_max[VR_X],
		aux->sensor6_options.working_volume_min[VR_Y],
		aux->sensor6_options.working_volume_max[VR_Y],
		aux->sensor6_options.working_volume_min[VR_Z],
		aux->sensor6_options.working_volume_max[VR_Z]);
}


/**************************************************************************/
static void _XwindowsPrintHelp(FILE *file, _XwindowsPrivateInfo *aux)
{
	int	count;			/* looping variable */
	char	charkey[] = "'x'";	/* NOTE: must be listed as an array or it's in fixed (read-only) memory space */
	char	*keyname;		/* X11-Windows name of a key */

	vrFprintf(file, BOLD_TEXT "\rXwindows default controls are:\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\tControlling the 2-way switches:\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\tMouse-left    -- 2-switch[1]\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\tMouse-middle  -- 2-switch[2]\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\tMouse-right   -- 2-switch[3]\n" NORM_TEXT);
	vrFprintf(file, "\n");

	vrFprintf(file, BOLD_TEXT "\r\tControlling the valuators:\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\tSpace key & mouse X   -- valuator[0]\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\tSpace key & mouse Y   -- valuator[1]\n" NORM_TEXT);
	vrFprintf(file, "\n");

	vrFprintf(file, BOLD_TEXT "\r\tControlling the simulated 6-sensors:\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\tArrow keys    -- left/right/forward/backward\n" NORM_TEXT);

	vrFprintf(file, BOLD_TEXT "\r\t\tLeft-Shift key-- change forward/backward movement to in/out\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\tLeft-Alt key  -- change translation movement to rotation\n" NORM_TEXT);
	vrFprintf(file, "\n");

	vrFprintf(file, BOLD_TEXT "\r\t\t'<' key       -- roll sensor counter-clockwise\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\t'>' key       -- roll sensor clockwise\n" NORM_TEXT);
	vrFprintf(file, "\n");

	vrFprintf(file, BOLD_TEXT "\r\t\t'n' key       -- change to next sensor\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\t's' key       -- change to head (ie. skull) sensor\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\t'w' key       -- change to hand (ie. wand) sensor\n" NORM_TEXT);
	vrFprintf(file, "\n");
	vrFprintf(file, BOLD_TEXT "\r\t\t'r' key       -- reset current sensor\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\t'*' key       -- reset all sensors\n" NORM_TEXT);


	vrFprintf(file, BOLD_TEXT "\r\t\t'h' key       -- ccw azimuth rotation of sensor\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\t'j' key       -- cw elevation rotation of sensor\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\t'k' key       -- ccw elevation rotation of sensor\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\t'l' key       -- cw azimuth rotation of sensor\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\t'f' key       -- translate sensor up\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\t'v' key       -- translate sensor down\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\ttab key       -- use MOUSE to rotate sensor\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\t'q' key       -- use MOUSE to move sensor left/right/up/down\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\t'a' key       -- use MOUSE to move sensor left/right/in/out\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\t'1' key       -- toggle use of sensor movement relative to own axis\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\t'2' key       -- toggle region limitation of movement\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\t'3' key       -- toggle return-to-zero sensor-6 movement method\n" NORM_TEXT);
	vrFprintf(file, "\n");

	vrFprintf(file, BOLD_TEXT "\r\tGeneral Controls:\n" NORM_TEXT);

	vrFprintf(file, BOLD_TEXT "\r\t\t'?' key       -- print this help info\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\tprint-screen  -- print the Xwindows-input data structure\n" NORM_TEXT);
	vrFprintf(file, "\n");
	vrFprintf(file, "\n");

	vrFprintf(file, BOLD_TEXT "\r\tSimulator Window Controls:\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t  (NOTE: these are technically not part of the input process)\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t  (NOTE also: key repeat is usually disabled)\n" NORM_TEXT);

	vrFprintf(file, BOLD_TEXT "\r\t\tkeypad '+'    -- move away from simulated CAVE\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\tkeypad '-'    -- move toward simulated CAVE\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\tkeypad '4'    -- rotate simulated CAVE\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\tkeypad '6'    -- rotate simulated CAVE\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\tkeypad '8'    -- rotate simulated CAVE\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\tkeypad '2'    -- rotate simulated CAVE\n" NORM_TEXT);
	vrFprintf(file, BOLD_TEXT "\r\t\tkeypad '5'    -- reset view of simulated CAVE\n" NORM_TEXT);
	vrFprintf(file, "\n");

	vrFprintf(file, "Input controls available from " BOLD_TEXT "X-windows input device:\n" NORM_TEXT);
	vrFprintf(file, "\r    Switch-2 key inputs:\n");

	for (count = 0; count < MAX_KEYINPUTS; count++) {
		if (aux->key_inputs[count] != NULL) {
			if (aux->key_inputs[count]->input_type == VRINPUT_BINARY || aux->key_inputs[count]->input_type == VRINPUT_CONTROL) {
				if (count < 256 && isprint(count)) {
					charkey[1] = (char)count;
					keyname = charkey;
				} else if (count < 256)
					keyname = XKeysymToString(count);
				else	keyname = XKeysymToString(count + 0xFE00);

				vrFprintf(file, "\t%s(%s) -- %s%s\n",
					aux->key_inputs[count]->my_object->desc_str,
					keyname,
					(aux->key_inputs[count]->input_type == VRINPUT_CONTROL ? "control:" : ""),
					aux->key_inputs[count]->my_object->name);
			}
		}
	}

	vrFprintf(file, "\r    Switch-2 button inputs:\n");
	for (count = 0; count < MAX_BUTTONINPUTS; count++) {
		if (aux->button_inputs[count] != NULL) {
			if (aux->button_inputs[count]->input_type == VRINPUT_BINARY || aux->button_inputs[count]->input_type == VRINPUT_CONTROL) {
				vrFprintf(file, "\t%s -- %s%s\n",
					aux->button_inputs[count]->my_object->desc_str,
					(aux->button_inputs[count]->input_type == VRINPUT_CONTROL ? "control:" : ""),
					aux->button_inputs[count]->my_object->name);
			}
		}
	}

	vrFprintf(file, "\r    Valuator pointer inputs:\n");
	for (count = 0; count < MAX_POINTERINPUTS; count++) {
		if (aux->pointer_inputs[count] != NULL) {
			if (aux->pointer_inputs[count]->input_type == VRINPUT_VALUATOR || aux->pointer_inputs[count]->input_type == VRINPUT_CONTROL) {
				vrFprintf(file, "\t%s -- %s%s\n",
					aux->pointer_inputs[count]->my_object->desc_str,
					(aux->pointer_inputs[count]->input_type == VRINPUT_CONTROL ? "control:" : ""),
					aux->pointer_inputs[count]->my_object->name);
			}
		}
	}
}


/**************************************************************************/
/* TODO: find out if there is an X-windows routine that basically does this */
/*   2/3/00 -- looks like XStringToKeysym() is basically what we want.      */
/*      (XKeysymToString() does the reverse.)                               */
static KeySym _XwindowsKeyValue(char *keyname)
{

#undef USE_XKeysymToString

#if defined(USE_XKeysymToString) /* { */

	/* This method works okay, and is much smaller than the other method */
	/*   but unfortunately doesn't do caseless string comparisons, so    */
	/*   would likely be the cause of much grief by people writing       */
	/*   config files, so we'll do it the long way.                      */
	/* NOTE: see also XKeysymToString() and XConvertCase() for other     */
	/*   potentially useful functions.                                   */

	KeySym	keysym;

	keysym = XStringToKeysym(keyname);
	if (keysym > 0xFF00)
		keysym -= 0xFE00;

	if (keysym > MAX_KEYINPUTS) {
		vrErrPrintf("_XwindowsKeyValue: Key symbol is out of bounds (%s).\n", keyname);
		keysym = NoSymbol;
	}

	vrPrintf("keyname = '%s', keysym = %d\n", keyname, keysym);

	return keysym;

#else /* } USE_XKeysymToString { */

	/* TODO: I have not tested every single key input, so we'll need to do that */

	/* character codes */
	if (strlen(keyname) == 1) {
		char	key = tolower(keyname[0]);

		/* continue converting key chars to non-shifted values          */
		/*  (since the shift-key modifier isn't used in the normal way) */
		/*  (note that some keyboards may be mapped differently)        */
		switch (key) {
		case '~':
		case '`':
			key = '`';
			break;
		case '!':
			key = '1';
			break;
		case '@':
			key = '2';
			break;
		case '#':
			key = '3';
			break;
		case '$':
			key = '4';
			break;
		case '%':
			key = '5';
			break;
		case '^':
			key = '6';
			break;
		case '&':
			key = '7';
			break;
		case '*':
			key = '8';
			break;
		case '(':
			key = '9';
			break;
		case ')':
			key = '0';
			break;
		case '_':
			key = '-';
			break;
		case '+':
			key = '=';
			break;
		case '{':
			key = '[';
			break;
		case '}':
			key = ']';
			break;
		case '|':
			key = '\\';
			break;
		case ':':
			key = ';';
			break;
		case '"':
			key = '\'';
			break;
		case '<':
			key = ',';
			break;
		case '>':
			key = '.';
			break;
		case '?':
			key = '/';
			break;
		}

		if (key >= 'a' && key <= 'z')
			return (XK_a + (key - 'a'));

		if (key >= ' ' && key <= '@')
			return (XK_space + (key - ' '));

#if 0 /* redundant with above */
		if (key >= '0' && key <= '9')
			return (XK_0 + (key - '0'));
#endif

		if (key >= '[' && key <= '\'')
			return (XK_0 + (key - '['));

	}

	if (!strcasecmp(keyname, "space"))		return (XK_space);
	else if (!strcasecmp(keyname, "exclam"))	return (XK_exclam);
	else if (!strcasecmp(keyname, "quotedbl"))	return (XK_quotedbl);
	else if (!strcasecmp(keyname, "hash"))		return (XK_numbersign);
	else if (!strcasecmp(keyname, "dollar"))	return (XK_dollar);
	else if (!strcasecmp(keyname, "percent"))	return (XK_percent);
	else if (!strcasecmp(keyname, "ampersand"))	return (XK_ampersand);
	else if (!strcasecmp(keyname, "apostrophe"))	return (XK_apostrophe);
	else if (!strcasecmp(keyname, "quoteright"))	return (XK_quoteright);
	else if (!strcasecmp(keyname, "parenleft"))	return (XK_parenleft);
	else if (!strcasecmp(keyname, "parenright"))	return (XK_parenright);
	else if (!strcasecmp(keyname, "asterisk"))	return (XK_asterisk);
	else if (!strcasecmp(keyname, "plus"))		return (XK_plus);
	else if (!strcasecmp(keyname, "comma"))		return (XK_comma);
	else if (!strcasecmp(keyname, "minus"))		return (XK_minus);
	else if (!strcasecmp(keyname, "period"))	return (XK_period);
	else if (!strcasecmp(keyname, "slash"))		return (XK_slash);
	else if (!strcasecmp(keyname, "colon"))		return (XK_colon);
	else if (!strcasecmp(keyname, "equal"))		return (XK_equal);
	else if (!strcasecmp(keyname, "grave"))		return (XK_grave);
	else if (!strcasecmp(keyname, "acute"))		return (XK_acute);

	/* TTY functions */
	else if (!strcasecmp(keyname, "escape"))	return (XK_Escape - 0xFE00);
	else if (!strcasecmp(keyname, "esc"))		return (XK_Escape - 0xFE00);
	else if (!strcasecmp(keyname, "backspace"))	return (XK_BackSpace - 0xFE00);
	else if (!strcasecmp(keyname, "tab"))		return (XK_Tab - 0xFE00);
	else if (!strcasecmp(keyname, "return"))	return (XK_Return - 0xFE00);
	else if (!strcasecmp(keyname, "enter"))		return (XK_Return - 0xFE00);
	else if (!strcasecmp(keyname, "pause"))		return (XK_Pause - 0xFE00);
	else if (!strcasecmp(keyname, "scroll_lock"))	return (XK_Scroll_Lock - 0xFE00);
	else if (!strcasecmp(keyname, "sys_req"))	return (XK_Sys_Req - 0xFE00);
	else if (!strcasecmp(keyname, "delete"))	return (XK_Delete - 0xFE00);

	/* Cursor control & motion */
	else if (!strcasecmp(keyname, "home"))		return (XK_Home - 0xFE00);
	else if (!strcasecmp(keyname, "left"))		return (XK_Left - 0xFE00);
	else if (!strcasecmp(keyname, "leftarrow"))	return (XK_Left - 0xFE00);
	else if (!strcasecmp(keyname, "up"))		return (XK_Up - 0xFE00);
	else if (!strcasecmp(keyname, "uparrow"))	return (XK_Up - 0xFE00);
	else if (!strcasecmp(keyname, "right"))		return (XK_Right - 0xFE00);
	else if (!strcasecmp(keyname, "rightarrow"))	return (XK_Right - 0xFE00);
	else if (!strcasecmp(keyname, "down"))		return (XK_Down - 0xFE00);
	else if (!strcasecmp(keyname, "downarrow"))	return (XK_Down - 0xFE00);
	else if (!strcasecmp(keyname, "prior"))		return (XK_Prior - 0xFE00);
	else if (!strcasecmp(keyname, "page_up"))	return (XK_Page_Up - 0xFE00);
	else if (!strcasecmp(keyname, "next"))		return (XK_Next - 0xFE00);
	else if (!strcasecmp(keyname, "page_down"))	return (XK_Page_Down - 0xFE00);
	else if (!strcasecmp(keyname, "end"))		return (XK_End - 0xFE00);
	else if (!strcasecmp(keyname, "begin"))		return (XK_Begin - 0xFE00);

	/* misc functions */
	else if (!strcasecmp(keyname, "print"))		return (XK_Print - 0xFE00);
	else if (!strcasecmp(keyname, "insert"))	return (XK_Insert - 0xFE00);
	else if (!strcasecmp(keyname, "num_lock"))	return (XK_Num_Lock - 0xFE00);

	/* keypad numbers and functions */
	else if (!strcasecmp(keyname, "kp0"))		return (XK_KP_Insert - 0xFE00);
	else if (!strcasecmp(keyname, "kp1"))		return (XK_KP_End - 0xFE00);
	else if (!strcasecmp(keyname, "kp2"))		return (XK_KP_Down - 0xFE00);
	else if (!strcasecmp(keyname, "kp3"))		return (XK_KP_Page_Down - 0xFE00);
	else if (!strcasecmp(keyname, "kp4"))		return (XK_KP_Left - 0xFE00);
	else if (!strcasecmp(keyname, "kp5"))		return (XK_KP_Begin - 0xFE00);
	else if (!strcasecmp(keyname, "kp6"))		return (XK_KP_Right - 0xFE00);
	else if (!strcasecmp(keyname, "kp7"))		return (XK_KP_Home - 0xFE00);
	else if (!strcasecmp(keyname, "kp8"))		return (XK_KP_Up - 0xFE00);
	else if (!strcasecmp(keyname, "kp9"))		return (XK_KP_Page_Up - 0xFE00);

	else if (!strcasecmp(keyname, "kp_enter"))	return (XK_KP_Enter - 0xFE00);
	else if (!strcasecmp(keyname, "kp_space"))	return (XK_KP_Space - 0xFE00);
	else if (!strcasecmp(keyname, "kp_tab"))	return (XK_KP_Tab - 0xFE00);
	else if (!strcasecmp(keyname, "kp_decimal"))	return (XK_KP_Decimal - 0xFE00);
	else if (!strcasecmp(keyname, "kp."))		return (XK_KP_Decimal - 0xFE00);
	else if (!strcasecmp(keyname, "kp_plus"))	return (XK_KP_Add - 0xFE00);
	else if (!strcasecmp(keyname, "kp+"))		return (XK_KP_Add - 0xFE00);
	else if (!strcasecmp(keyname, "kp_minus"))	return (XK_KP_Subtract - 0xFE00);
	else if (!strcasecmp(keyname, "kp-"))		return (XK_KP_Subtract - 0xFE00);
	else if (!strcasecmp(keyname, "kp_times"))	return (XK_KP_Multiply - 0xFE00);
	else if (!strcasecmp(keyname, "kp*"))		return (XK_KP_Multiply - 0xFE00);
	else if (!strcasecmp(keyname, "kp_divide"))	return (XK_KP_Divide - 0xFE00);
	else if (!strcasecmp(keyname, "kp/"))		return (XK_KP_Divide - 0xFE00);

	/* function keys */
	else if (!strcasecmp(keyname, "f1"))		return (XK_F1 - 0xFE00);
	else if (!strcasecmp(keyname, "f2"))		return (XK_F2 - 0xFE00);
	else if (!strcasecmp(keyname, "f3"))		return (XK_F3 - 0xFE00);
	else if (!strcasecmp(keyname, "f4"))		return (XK_F4 - 0xFE00);
	else if (!strcasecmp(keyname, "f5"))		return (XK_F5 - 0xFE00);
	else if (!strcasecmp(keyname, "f6"))		return (XK_F6 - 0xFE00);
	else if (!strcasecmp(keyname, "f7"))		return (XK_F7 - 0xFE00);
	else if (!strcasecmp(keyname, "f8"))		return (XK_F8 - 0xFE00);
	else if (!strcasecmp(keyname, "f9"))		return (XK_F9 - 0xFE00);
	else if (!strcasecmp(keyname, "f10"))		return (XK_F10 - 0xFE00);
	else if (!strcasecmp(keyname, "f11"))		return (XK_F11 - 0xFE00);
	else if (!strcasecmp(keyname, "f12"))		return (XK_F12 - 0xFE00);

	/* modifyer keys */
	else if (!strcasecmp(keyname, "shift_l"))	return (XK_Shift_L - 0xFE00);
	else if (!strcasecmp(keyname, "leftshift"))	return (XK_Shift_L - 0xFE00);
	else if (!strcasecmp(keyname, "shift_r"))	return (XK_Shift_R - 0xFE00);
	else if (!strcasecmp(keyname, "rightshift"))	return (XK_Shift_R - 0xFE00);
	else if (!strcasecmp(keyname, "control_l"))	return (XK_Control_L - 0xFE00);
	else if (!strcasecmp(keyname, "leftctrl"))	return (XK_Control_L - 0xFE00);
	else if (!strcasecmp(keyname, "control_r"))	return (XK_Control_R - 0xFE00);
	else if (!strcasecmp(keyname, "rightctrl"))	return (XK_Control_R - 0xFE00);
	else if (!strcasecmp(keyname, "caps_lock"))	return (XK_Caps_Lock - 0xFE00);
	else if (!strcasecmp(keyname, "shift_lock"))	return (XK_Shift_Lock - 0xFE00);
#ifdef __APPLE__	/* TODO: see whether this is just a Powerbook thing, or for all Apples. */
	else if (!strcasecmp(keyname, "alt_l"))		return (XK_Mode_switch - 0xFE00);
	else if (!strcasecmp(keyname, "leftalt"))	return (XK_Mode_switch - 0xFE00);
#else
	else if (!strcasecmp(keyname, "alt_l"))		return (XK_Alt_L - 0xFE00);
	else if (!strcasecmp(keyname, "leftalt"))	return (XK_Alt_L - 0xFE00);
#endif
	else if (!strcasecmp(keyname, "alt_r"))		return (XK_Alt_R - 0xFE00);
	else if (!strcasecmp(keyname, "rightalt"))	return (XK_Alt_R - 0xFE00);

	else {
		vrErrPrintf("_XwindowsKeyValue: Unknown (or niy) key type '%s'\n", keyname);
		return 0x00;
	}

	vrErrPrintf("\n\n_XwindowsKeyValue(): HMMM, we definitely shouldn't get here.\n");
#endif /* } (USE_XKeysymToString) */
}


/**************************************************************************/
static vrWindowInfo *_GetXWindowInfo(_XwindowsPrivateInfo *aux, Window window)
{
	int	count;

#if 0
	vrPrintf("Finding FreeVR-windowInfo for X-window %p\n", window);
	vrPrintf("aux->window_count = %d\n", aux->window_count);
#endif
	for (count = 0; count < aux->window_count; count++) {
		if (aux->xwindow[count] == window) {
			return (aux->window[count]);
		}
	}

	/* I think it's an error if we get here! */
	vrErrPrintf("_GetXWindowInfo(): " RED_TEXT "Warning: Unable to find FreeVR window that matches X-window %p.\n" NORM_TEXT, window);
	return NULL;
}


/**************************************************************************/
static unsigned int _XwindowsButtonValue(char *button_name)
{
	if (!strcasecmp(button_name, "left"))	return 1;

	if (!strcasecmp(button_name, "middle"))	return 2;

	if (!strcasecmp(button_name, "right"))	return 3;

	return 0;
}


/**************************************************************************/
static int _XwindowsPointerValue(char *pointer_name)
{
	if ((!strcasecmp(pointer_name, "x")) || (!strncasecmp(pointer_name, "xwin", 4)))
		return X_WINDOW;

	if ((!strcasecmp(pointer_name, "y")) || (!strncasecmp(pointer_name, "ywin", 4)))
		return Y_WINDOW;

	if ((!strcasecmp(pointer_name, "xroot")) || (!strcasecmp(pointer_name, "xscreen")))
		return X_SCREEN;

	if ((!strcasecmp(pointer_name, "yroot")) || (!strcasecmp(pointer_name, "yscreen")))
		return Y_SCREEN;

	return -1;
}


	/**********************************/
	/*** FreeVR NON public routines ***/
	/**********************************/


/*****************************************************************/
/* vrWaitForWindowMapping(): used by _GlxOpenFunc() to wait for  */
/*   a window to be mapped to an X-screen before continuing.     */
/*****************************************************************/
static Bool vrWaitForWindowMapping(Display *xdisplay, XEvent *event, XPointer data)
{
	Window	xwindow = (Window)data;

	if (event->xmap.window == xwindow && event->type == MapNotify)
		return True;
	else	return False;
}


/****************************************************************************/
/* NOTE: there are now three copies of this function in FreeVR -- ugh */
static void _GlxParseArgs(vrGlxPrivateInfo *aux, char *args)
{
	char	*str = NULL;

	/* In the rare case of no arguments, just return */
	if (args == NULL)
		return;

	/* TODO: see how much of the rest of this parsing stuff can be made generic */

	/**********************************************************************/
	/** Argument format: "display=" [host]:server[.screen] [(";" | ",")] **/
	/**********************************************************************/
	if (str = strstr(args, "display=")) {
		char	*host = strchr(str, '=') + 1;
		char	*colon = strchr(str, ':');

		if (colon == NULL) {
			aux->xhost = vrShmemStrDup("");
			colon = str-1;
		} else {
			*colon = '\0';
			aux->xhost = vrShmemStrDup(host);
			*colon = ':';
		}
		sscanf(colon, ":%d.%d", &(aux->xserver), &(aux->xscreen));
		vrDbgPrintfN(SELDOM_DBGLVL, "_GlxParseArgs(): parsed host = '%s', xserver = %d, screen = %d\n", host, aux->xserver, aux->xscreen);
	} else {
		aux->xhost = NULL;
		aux->xserver = -1;
	}

	/************************************************************/
	/** Argument format: "geometry=" [WxH][+X+Y] [(";" | ",")] **/
	/************************************************************/
	/* information is stored in the XSizeHints structure */
	if (str = strstr(args, "geometry=")) {
		char		*geom = strchr(str, '=');
		char		*space = strchr(geom, ' ');
		char		*semi = strchr(geom, ';');
		char		*comma = strchr(geom, ',');
		int		parsed;				/* return mask from XParseGeometry() */
		int		x, y;				/* return values from XParseGeometry () */
		unsigned int	width, height;			/* return values from XParseGeometry () */

		/* figure out which (non-NULL) terminator comes first, and end the string there */
		if (space == NULL) {
			if (semi != NULL)
				*semi = '\0';
			if (comma != NULL)
				*comma = '\0';
		} else {
			if (semi != NULL && semi < space)
				*semi = '\0';
			else	*space = '\0';
			if (comma != NULL && comma < space)
				*comma = '\0';
		}

		parsed = XParseGeometry(geom, &x, &y, &width, &height);
		aux->xsize_hints.flags = USPosition | PSize | PMinSize;
		if (parsed & XValue)
			aux->xsize_hints.x = x;
		if (parsed & YValue)
			aux->xsize_hints.y = y;
		if (parsed & WidthValue)
			aux->xsize_hints.width = width;
		aux->xsize_hints.min_width = 10;
		if (parsed & HeightValue)
			aux->xsize_hints.height = height;
		aux->xsize_hints.min_height = 10;

		/* restore the original string */
		if (space == NULL) {
			if (semi != NULL)
				*semi = ';';
			if (comma != NULL)
				*comma = ',';
		} else {
			if (semi != NULL && semi < space)
				*semi = ';';
			else	*space = ' ';
			if (comma != NULL && comma < space)
				*comma = ',';
		}
	} else {
		vrDbgPrintf("_GlxParseArgs(): No geometry field to parse: args = '%s'\n", args);
		aux->xsize_hints.flags = 0;
	}

	/**********************************************************/
	/** Argument format: "decoration=" {opts,} [(";" | ",")] **/
	/**********************************************************/
	/* where opts is a comma-separated string with one         */
	/* or more of these tokens in it:                          */
	/*   borders|border (resize handles)                       */
	/*   title|titlebar (title bar - for moving, not resizing) */
	/*   minmax (minimize and maximize buttons on titlebar)    */
	/* these two options set multiple resources:               */
	/*   all|window (full title bar and borders)               */
	/*   none (nothing -- the default)                         */

	/* set the decorations flag */
	if (str = strstr(args, "decoration=")) {
		char	hints[128];
		char	*hint_str = strchr(str, '=') + 1;
		char	*tok;

		aux->decorations = 0;

		sscanf(hint_str, "%s", hints);
		tok = strtok(hints, ", ");
		while (tok) {
			if (!strcasecmp(tok, "borders")) {
				aux->decorations |= DECORATION_BORDER;
			} else if (!strcasecmp(tok, "border")) {
				aux->decorations |= DECORATION_BORDER;
			} else if (!strcasecmp(tok, "minmax")) {
				aux->decorations |= DECORATION_MINMAX;
			} else if (!strcasecmp(tok, "title")) {
				aux->decorations |= DECORATION_TITLE;
			} else if (!strcasecmp(tok, "titlebar")) {
				aux->decorations |= DECORATION_TITLE;
			} else if (!strcasecmp(tok, "window")) {
				aux->decorations = DECORATION_ALL;
			} else if (!strcasecmp(tok, "all")) {
				aux->decorations = DECORATION_ALL;
			} else if (!strcasecmp(tok, "none")) {
				aux->decorations = 0;
			}
			tok = strtok(NULL, ", ");
		}
	}

	/**********************************************************/
	/** Argument format: "title=" <string> [(";" | ",")] **/
	/**********************************************************/
	/* where <string> is a string that will be used for the */
	/*   window title message.                              */

	/* set the title string */
	if (str = strstr(args, "title=")) {
		char	*title_str = strchr(str, '=') + 1;
		char	*tok;

		tok = strtok(title_str, ",;");
		aux->window_title = vrShmemStrDup(tok);
	}

	/******************************************************/
	/** Argument format: "cursor= <string> [(";" | ",")] **/
	/******************************************************/
	/* where <string> is a string that will be used for setting */
	/*   the name of the cursor that is used.                   */

	/* set the cursor_name string */
	if (str = strstr(args, "cursor=")) {
		char	*title_str = strchr(str, '=') + 1;
		char	*tok;

		tok = strtok(title_str, ",;");
		aux->cursor_name = vrShmemStrDup(tok);
	}

#if 0
	vrPrintf("================================================\n");
	vrPrintf("done parsing argument string `%s'\n", args);
	vrPrintf("aux->xhost = '%s'\n", aux->xhost);
	vrPrintf("aux->xserver = %d\n", aux->xserver);
	vrPrintf("aux->xscreen = %d\n", aux->xscreen);
	vrPrintf("aux->decoration = 0x%02x\n", aux->decorations);
	vrPrintf("geometry: %dx%d+%d+%d\n",
		aux->xsize_hints.width, aux->xsize_hints.height,
		aux->xsize_hints.x, aux->xsize_hints.y);
	vrPrintf("xsize_hints.flags = %d\n", aux->xsize_hints.flags);
	vrPrintf("aux->window_title = '%s'\n", aux->window_title);
	vrPrintf("aux->cursor_name = '%s'\n", aux->cursor_name);
	vrPrintf("================================================\n");
#endif
}


/**************************************************************************/
static vrGlxPrivateInfo *_XwindowsOpenInputWindow(Display *display, char *current_display, char *window_name, char *window_args)
{
#include XBM_BACK_FILE
#include XBM_ICON_FILE
	vrGlxPrivateInfo	*glx_aux;
	char			xdisplay_name[128];
	unsigned long		winattr_mask = 0;	/* window attributes set in the attribute structure */
	Atom			mwm_hints_atom;		/* used for hinting at the decorations to the WM */
	XColor			fg, bg;
	Pixmap			icon_pixmap;

	/****************************************************/
	/* allocate and initialize X-windows auxiliary data */
	glx_aux = (vrGlxPrivateInfo *)vrShmemAlloc0(sizeof(vrGlxPrivateInfo));

	glx_aux->xhost = NULL;
	glx_aux->xserver = -1;
	glx_aux->xscreen = -1;
	glx_aux->decorations = DECORATION_ALL;	/* default to all decorations */
	glx_aux->xsize_hints.x = 10;		/* TODO: ideally x, y, width & height would be part */
	glx_aux->xsize_hints.y = 10;		/*   of the overall window structure, and would be  */
	glx_aux->xsize_hints.width = 200;	/*   given initial values in vr_visren.c.           */
	glx_aux->xsize_hints.height = 200;

	glx_aux->cursor_name = vrShmemStrDup("default");	/* the default cursor (NOTE: different than for rendering windows) */

	glx_aux->window_title = vrShmemAlloc(30 + strlen(window_name));
	sprintf(glx_aux->window_title, "FreeVR Input: %s window", window_name);

	/* parse the arguments to override defaults            */
	/*   this will fill in aux->xserver, aux->xscreen, etc. */
	_GlxParseArgs(glx_aux, window_args);

	/****************************************************/
	/*** open connection to X server (one per window) ***/
	/****************************************************/

	if (glx_aux->xhost == NULL) {
		if (getenv("DISPLAY") != NULL)
			sprintf(xdisplay_name, "%s", getenv("DISPLAY"));
		else	sprintf(xdisplay_name, ":0.0");
	} else {
		sprintf(xdisplay_name, "%s:%d.%d", glx_aux->xhost, glx_aux->xserver, glx_aux->xscreen);
	}

	if (display == NULL || strcmp(current_display, xdisplay_name) != 0) {
		/* Only print re-open error if there was already an open display */
		if (display != NULL)
			vrErrPrintf("_XwindowsOpenInputWindow(): " RED_TEXT "Warning: X-display mismatch ('%s' vs. '%s'), previous display connections will be lost.\n" NORM_TEXT, current_display, xdisplay_name);

		if (!(glx_aux->xdisplay = XOpenDisplay(xdisplay_name))) {
			vrDbgPrintf("_XwindowsOpenInputWindow(): " RED_TEXT "Xdisplay -- Could not open display '%s'\n", xdisplay_name);
			return NULL;		/* returning without mapping the window */
		}
	} else {
		glx_aux->xdisplay = display;
	}

	glx_aux->xdisplay_string = vrShmemStrDup(DisplayString(glx_aux->xdisplay));
	if (glx_aux->xscreen < 0)
		glx_aux->xscreen = DefaultScreen(glx_aux->xdisplay);

	glx_aux->xscreen_size_x = DisplayWidth(glx_aux->xdisplay, glx_aux->xscreen);
	glx_aux->xscreen_size_y = DisplayHeight(glx_aux->xdisplay, glx_aux->xscreen);


	/*******************/
	/* Get an X-visual */
	glx_aux->xvisual = glXChooseVisual(glx_aux->xdisplay, glx_aux->xscreen, doub_buf_attribs);
	if (!glx_aux->xvisual) {
		vrErr("no RGB visual with depth buffer");
		return NULL;		/* returning without mapping the window */
	}

	/**************************************/
	/* create a colormap for the X window */
	/* NOTE: we don't need this for opening windows, but do */
	/*   need it for setting pixmap colors.                 */
	glx_aux->xcolormap = XCreateColormap(
		glx_aux->xdisplay,
		RootWindow(glx_aux->xdisplay, glx_aux->xscreen),
		glx_aux->xvisual->visual,
		AllocNone
		);
	glx_aux->xwindow_attr.colormap = glx_aux->xcolormap;	winattr_mask |= CWColormap;

	/******************/
	/* create pixmaps */

	/* first get the two colors we will use */
	if (glx_aux->xcolormap != None) {
		if (XParseColor(glx_aux->xdisplay, glx_aux->xcolormap, "orange", &bg)) {
			if (!XAllocColor(glx_aux->xdisplay, glx_aux->xcolormap, &bg))
				bg.pixel = BlackPixel(glx_aux->xdisplay, glx_aux->xscreen);
		} else {
				bg.pixel = BlackPixel(glx_aux->xdisplay, glx_aux->xscreen);
		}
		if (XParseColor(glx_aux->xdisplay, glx_aux->xcolormap, "blue", &fg)) {
			if (!XAllocColor(glx_aux->xdisplay, glx_aux->xcolormap, &fg))
				fg.pixel = WhitePixel(glx_aux->xdisplay, glx_aux->xscreen);
		} else {
				fg.pixel = WhitePixel(glx_aux->xdisplay, glx_aux->xscreen);
		}
	} else {
		bg.pixel = BlackPixel(glx_aux->xdisplay, glx_aux->xscreen);
		fg.pixel = WhitePixel(glx_aux->xdisplay, glx_aux->xscreen);
	}

	/* create the pixmap for the background */
	glx_aux->xwindow_attr.background_pixmap = XCreatePixmapFromBitmapData(
		glx_aux->xdisplay,
		RootWindow(glx_aux->xdisplay, glx_aux->xscreen),
		XBM_BACK_BITS, XBM_BACK_WIDTH, XBM_BACK_HEIGHT,
		fg.pixel, bg.pixel, glx_aux->xvisual->depth);	winattr_mask |= CWBackPixmap;

	/* create a pixmap for the icon */
	icon_pixmap = XCreateBitmapFromData(
		glx_aux->xdisplay,
		RootWindow(glx_aux->xdisplay, glx_aux->xscreen),
		XBM_ICON_BITS, XBM_ICON_WIDTH, XBM_ICON_HEIGHT);

	/************************************/
	/* create a cursor for the X window */
	glx_aux->cursor = vrXwindowsMakeCursor(glx_aux, glx_aux->cursor_name);
	glx_aux->xwindow_attr.cursor = glx_aux->cursor;		winattr_mask |= CWCursor;

	/**********************************************************************/
	/* set some additional window attributes prior to creating the window */
	vrTrace("_XwindowsOpenInputWindow", "about to open window");
	glx_aux->xwindow_attr.border_pixel = 0;			winattr_mask |= CWBorderPixel;
#if 0 /* setting the background_pixel will override the background pixmap */
	glx_aux->xwindow_attr.background_pixel = 0x009000;	winattr_mask |= CWBackPixel;
#endif
	glx_aux->xwindow_attr.event_mask =
#if 0					/* 06/13/2006: removed since we weren't using the "Expose" events */
		ExposureMask |
#endif
		StructureNotifyMask |
		DestroyNotify |		/* so we can cleanly go away when closed */
		UnmapNotify |		/* also so we can cleanly go away when closed */
		0;						winattr_mask |= CWEventMask;


	/**************************/
	/*** create the window! ***/
	/**************************/
	glx_aux->xwindow = XCreateWindow(
		glx_aux->xdisplay,		/* display to put the window on */
		RootWindow(glx_aux->xdisplay, glx_aux->xscreen),/* make the root window the parent*/
		0, 0,				/* default x & y location */
		100, 100,			/* default width & height -- this sets lower limit on window size */
		0,				/* default width of the border */
		glx_aux->xvisual->depth,	/* the desired color depth */
		InputOutput,			/* window class */
		glx_aux->xvisual->visual,	/* the desired visual type */
		winattr_mask,			/* attribute options -- mask */
		&(glx_aux->xwindow_attr));	/* window attributes */


	/****************************************************/
	/* set some window manager look and feel properties */
	glx_aux->mwm_hints.flags = MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS;
	glx_aux->mwm_hints.functions = MWM_FUNC_ALL;
	glx_aux->mwm_hints.decorations = 0;
	if (glx_aux->decorations == DECORATION_ALL) {
		glx_aux->mwm_hints.decorations = MWM_DECOR_ALL;
	} else {
		if (glx_aux->decorations & DECORATION_TITLE) {
			glx_aux->mwm_hints.decorations |= MWM_DECOR_TITLE;
		}
		if (glx_aux->decorations & DECORATION_MINMAX) {
			glx_aux->mwm_hints.decorations |= MWM_DECOR_MINIMIZE;
			glx_aux->mwm_hints.decorations |= MWM_DECOR_MAXIMIZE;
		}
		if (glx_aux->decorations & DECORATION_BORDER) {
			glx_aux->mwm_hints.decorations |= MWM_DECOR_BORDER;
			glx_aux->mwm_hints.decorations |= MWM_DECOR_RESIZEH;
		}
	}

	mwm_hints_atom = XInternAtom(glx_aux->xdisplay, _XA_MOTIF_WM_HINTS, False);
	XChangeProperty(glx_aux->xdisplay, glx_aux->xwindow, mwm_hints_atom, mwm_hints_atom,
		32, PropModeReplace, (unsigned char *)(&(glx_aux->mwm_hints)), PROP_MWM_HINTS_ELEMENTS);

	/************************************************/
	/* set standard window properties and pop it up */
	XSetStandardProperties(
		glx_aux->xdisplay,		/* Xwindow display pointer */
		glx_aux->xwindow,		/* Xwindow reference number */
		glx_aux->window_title,		/* window title */
		window_name,			/* icon title */
		icon_pixmap,			/* Pixmap icon_pixmap */
		NULL,				/* char **argv */
		0,				/* int argc */
		&(glx_aux->xsize_hints));	/* Xwindow geometry info */

	/* specify where the window should be placed */
	XMoveWindow(glx_aux->xdisplay, glx_aux->xwindow, glx_aux->xsize_hints.x, glx_aux->xsize_hints.y);

	if (glx_aux->xsize_hints.width == 0) {
		vrDbgPrintfN(AALWAYS_DBGLVL, "_XwindowsOpenInputWindow(): " RED_TEXT "window width is 0, setting to 100\n" NORM_TEXT);
		glx_aux->xsize_hints.width = 100;
	}
	if (glx_aux->xsize_hints.height == 0) {
		vrDbgPrintfN(AALWAYS_DBGLVL, "_XwindowsOpenInputWindow(): " RED_TEXT "window height is 0, setting to 100\n" NORM_TEXT);
		glx_aux->xsize_hints.height = 100;
	}

	/* specify the size of the window */
        XResizeWindow(glx_aux->xdisplay, glx_aux->xwindow, glx_aux->xsize_hints.width, glx_aux->xsize_hints.height);

	/* cause the window to appear on the screen */
	XMapWindow(glx_aux->xdisplay, glx_aux->xwindow);

	XFlush(glx_aux->xdisplay);
	glx_aux->mapped = 1;

	return glx_aux;
}


/**************************************************************************/
static void _XwindowsParseArgs(_XwindowsPrivateInfo *aux, char *args)
{
	float	volume_values[6];

	/****************************************/
	/** Argument format: "window" = string **/
	/****************************************/
	vrArgParseString(args, "window", &(aux->window_list));
	vrArgParseString(args, "windows", &(aux->window_list));	/* there might be more than one window */
	vrArgParseString(args, "screen", &(aux->window_list));	/* deprecated - backward compat for now */

	/*************************************************************/
	/** Argument format: "silent" "=" { "on" | "off" | number } **/
	/*************************************************************/
	vrArgParseBool(args, "silent", &(aux->silent));

	/****************************************************************/
	/** Argument format: "keyrepeat" "=" { "on" | "off" | number } **/
	/****************************************************************/
	vrArgParseBool(args, "keyrepeat", &(aux->keyrepeat));

	/*********************************************************************/
	/** Argument format: "sensor_swap_yz" "=" { "on" | "off" | number } **/
	/*********************************************************************/
	vrArgParseBool(args, "sensor_swap_yz", &(aux->sensor6_options.swap_yz));
	vrArgParseBool(args, "sensor_swap_upin", &(aux->sensor6_options.swap_yz));

	/**********************************************************************/
	/** Argument format: "sensor_rotate_sensor" "=" { "on" | "off" | number } **/
	/**********************************************************************/
	vrArgParseBool(args, "sensor_rotate_sensor", &(aux->sensor6_options.swap_transrot));

	/*****************************************************/
	/** Argument format: "sensor_transScale" "=" number **/
	/*****************************************************/
	vrArgParseFloat(args, "sensor_transscale", &(aux->sensor6_options.trans_scale));

	/***************************************************/
	/** Argument format: "sensor_rotScale" "=" number **/
	/***************************************************/
	vrArgParseFloat(args, "sensor_rotscale", &(aux->sensor6_options.rot_scale));

	/* NOTE: no valuator sensitivity, since the size of the */
	/*   window specifies the range of motion.              */

	/**********************************************************************/
	/** Argument format: "sensor_restrict" "=" { "on" | "off" | number } **/
	/**********************************************************************/
	vrArgParseBool(args, "sensor_restrict", &(aux->sensor6_options.restrict_space));

	/* NOTE: no valuatorOverride (like 6-dof simulated 6-sensors), */
	/*   because there are only two incoming valuators in X, so    */
	/*   a separate control callback is used.                      */

	/**************************************************************************/
	/** Argument format: "sensor_returnToZero" "=" { "on" | "off" | number } **/
	/**************************************************************************/
	vrArgParseBool(args, "sensor_returntozero", &(aux->sensor6_options.return_to_zero));

	/*************************************************************************/
	/** Argument format: "sensor_relativeRot" "=" { "on" | "off" | number } **/
	/*************************************************************************/
	vrArgParseBool(args, "sensor_relativerot", &(aux->sensor6_options.relative_axis));

	/******************************************************************************************/
	/** Argument format: "sensor_workingVolume" "=" <float> <float> <float> <float> <float> <float> **/
	/******************************************************************************************/
	if (vrArgParseFloatList(args, "sensor_workingvolume", volume_values, 6)) {
		aux->sensor6_options.working_volume_min[VR_X] = volume_values[0];
		aux->sensor6_options.working_volume_max[VR_X] = volume_values[1];
		aux->sensor6_options.working_volume_min[VR_Y] = volume_values[2];
		aux->sensor6_options.working_volume_max[VR_Y] = volume_values[3];
		aux->sensor6_options.working_volume_min[VR_Z] = volume_values[4];
		aux->sensor6_options.working_volume_max[VR_Z] = volume_values[5];
	}
}


/**************************************************************************/
static void _XwindowsGetData(vrInputDevice *devinfo)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;
	XEvent			event;					/* incoming x-event */

	/* handle all the events coming to the window selected for this input device */
	while (XPending(aux->display)) {
		XNextEvent(aux->display, &event);
		vrDbgPrintfN(XWIN_DBGLVL, "_XwindowsGetData: got event of type %d.\n", event.type);
		switch (event.type) {
		case KeyPress:
#if 0 /* print each keypress received for debugging */
vrPrintf("_XwindowsGetData(): Got a key press of type %X\n", XLookupKeysym((XKeyEvent *)&event, 0));
#endif
		case KeyRelease: {
			KeySym		key = XLookupKeysym((XKeyEvent *)&event, 0);
			unsigned int	keyvalue;

			if ((key & 0xFF00) == 0xFF00)
				keyvalue = key - 0xFE00;
			else if (key <= 0xFF)
				keyvalue = key;
			else {
				vrErrPrintf("_XwindowsGetData: not yet handling key 0x%02x\n", key);
				keyvalue = 0;
			}

			vrDbgPrintfN(INPUT_DBGLVL, "_XwindowsGetData: handling a key event of value 0x%02x (input pointer = %#p)\n", keyvalue, aux->key_inputs[keyvalue]);
			if (0) {
				char	keychar[32];
				KeySym	keysym;

				XLookupString((XKeyEvent *)&event, keychar, 31, &keysym, NULL);
				vrPrintf("XLookupString() returned with '%s'\n", keychar);
				/* NOTE: I was hoping keychar would return with something       */
				/*   reasonable for all keyboard inputs, but it only seems to   */
				/*   work for normal alpha/numeric/punctuation keys.  (The one  */
				/*   thing it does seem to do is make use of the modifier keys.)*/
			}
			if (aux->key_inputs[keyvalue] != NULL) {
				/********************************************************************/
				/* This section of the conditional is for keys for which a specific */
				/*   input was mapped.  E.g. a button mapped to just the 'Escape'   */
				/*   key.                                                           */
				/********************************************************************/
				int	new_value = (event.type == KeyPress);

				switch (aux->key_inputs[keyvalue]->input_type) {
				case VRINPUT_BINARY:
					vrDbgPrintfN(INPUT_DBGLVL, "_XwindowsGetData: "
                                                "Assigning a value of %d to key 0x%x\n", new_value, keyvalue);
					vrAssign2switchValue((vr2switch *)(aux->key_inputs[keyvalue]), new_value /*, vrCurrentWallTime() */);
					break;

				case VRINPUT_VALUATOR:
					/* TODO: ... */
					/* For keys associated with valuator inputs, pressing the */
					/*   key can add some fixed, or specified amount.  Fixed  */
					/*   is easier, so we'll start with a value like 0.2.     */
					break;

				case VRINPUT_CONTROL:
					vrDbgPrintfN(INPUT_DBGLVL, "_XwindowsGetData: "
						"Activating a callback (%#p) with %d to Xwindows key 0x%x\n",
						((vrControl *)(aux->key_inputs[keyvalue]))->callback,
						new_value, keyvalue);
					vrCallbackInvokeDynamic(((vrControl *)(aux->key_inputs[keyvalue]))->callback, 1, new_value);
					break;
				}
			} else {
				/****************************************************************/
				/* This section of the conditional is for keys that have not    */
				/*   been specifically mapped to an input, and therefore can be */
				/*   part of a generic keyboard input where all the remaining   */
				/*   key inputs are interpreted.                                */
				/****************************************************************/
				int	press_value = (event.type == KeyPress);
				int	keyboard_num;

				/* TODO: need to figure out which keyboard this input comes from. */
				/*   IE. for now, we're basically ignoring the "instance"         */
				/*   specification in the input specification.                    */
				/*	keyboard_num = ...;                                       */
				/*	if (keyboard_num >= MAX_KEYBOARDS) { ... } else { ... } */
				keyboard_num = 0;

				if (aux->keyboard_inputs[keyboard_num] == NULL)
					break;

				/* TODO: figure out the appropriate value for "new_value" */

				vrDbgPrintfN(INPUT_DBGLVL, "extra key event of value 0x%02x (input pointer = %#p)\n", keyvalue, aux->keyboard_inputs[keyboard_num]);
				/* If there is a general keyboard input associated with this     */
				/*   input, then put all other key inputs as the last keystroke. */

				switch (aux->keyboard_inputs[keyboard_num]->input_type) {
				case VRINPUT_BINARY:
					vrDbgPrintfN(INPUT_DBGLVL, "_XwindowsGetData: "
                                                "Assigning a binary value of %d to keyboard 0x%x\n", keyvalue, keyboard_num);
					vrAssign2switchValue((vr2switch *)(aux->keyboard_inputs[keyboard_num]), press_value /*, vrCurrentWallTime() */);
					break;

				case VRINPUT_NWAY:
					/* There are three types of n-way inputs: "release", "last", or */
					/*   count with the press_value needs to be used to determine   */
					/*   when to assign the value.                                  */
					vrDbgPrintfN(INPUT_DBGLVL, "_XwindowsGetData: "
                                                "Assigning new n-way value keyboard %d, from key %d\n", keyboard_num, keyvalue);

					switch (aux->keyboard_type[keyboard_num]) {
					default:
					case KEYBOARD_LAST:	/** only record the last-pressed key **/
						if (press_value)
							vrAssignNswitchValue((vrNswitch *)(aux->keyboard_inputs[keyboard_num]), keyvalue /*, vrCurrentWallTime() */);
						break;

					case KEYBOARD_RELEASE:	/** record the currently pressed key, or a 0 **/
						if (press_value)
							vrAssignNswitchValue((vrNswitch *)(aux->keyboard_inputs[keyboard_num]), keyvalue /*, vrCurrentWallTime() */);
						else	vrAssignNswitchValue((vrNswitch *)(aux->keyboard_inputs[keyboard_num]), 0 /*, vrCurrentWallTime() */);
						break;

					case KEYBOARD_COUNT:	/** record the number of currently depressed keys **/
						if (press_value)
							aux->keyboard_count[keyboard_num]++;
						else	aux->keyboard_count[keyboard_num]--;
						vrAssignNswitchValue((vrNswitch *)(aux->keyboard_inputs[keyboard_num]), aux->keyboard_count[keyboard_num] /*, vrCurrentWallTime() */);
						break;
					}
					break;

				case VRINPUT_VALUATOR:
					/* For keys associated with valuator inputs, pressing */
					/*   the key can add some fixed, or specified amount. */
					if (press_value)
						aux->keyboard_count[keyboard_num]++;
					else	aux->keyboard_count[keyboard_num]--;

					vrDbgPrintfN(INPUT_DBGLVL, "_XwindowsGetData: "
                                                "Assigning an valuator value of %.2f to keyboard %d\n", aux->keyboard_count[keyboard_num] * aux->keyboard_scale[keyboard_num], keyboard_num);
					vrAssignValuatorValue((vrValuator *)(aux->keyboard_inputs[keyboard_num]), aux->keyboard_count[keyboard_num] * aux->keyboard_scale[keyboard_num]);
					break;

				case VRINPUT_CONTROL:
					vrDbgPrintfN(INPUT_DBGLVL, "_XwindowsGetData: "
						"Activating a callback (%#p) with %d to Xwindows key 0x%x\n",
						((vrControl *)(aux->keyboard_inputs[keyboard_num]))->callback,
						keyvalue, keyboard_num);
					vrCallbackInvokeDynamic(((vrControl *)(aux->keyboard_inputs[keyboard_num]))->callback, 1, keyvalue);
					break;
				}
			}
			break;
		}

		case ButtonPress:
		case ButtonRelease: {
			XButtonEvent	*button_event = (XButtonEvent *)&event;
			int		button_num = button_event->button;

			if (button_num >= MAX_BUTTONINPUTS) {
				vrErrPrintf("_XwindowsGetData: not yet handling button %d\n", button_num);
			} else if (aux->button_inputs[button_num] != NULL) {
				int	new_value = (event.type == ButtonPress);

				switch (aux->button_inputs[button_num]->input_type) {
				case VRINPUT_BINARY:
					vrDbgPrintfN(INPUT_DBGLVL, "_XwindowsGetData: "
                                                "Assigning a value of %d to Xwindows button %d\n", new_value, button_num);
					vrAssign2switchValue((vr2switch *)(aux->button_inputs[button_num]), new_value /*, vrCurrentWallTime() */);
					break;

				case VRINPUT_CONTROL:
					vrDbgPrintfN(INPUT_DBGLVL, "_XwindowsGetData: "
						"Activating a callback with %d to Xwindows button %d\n", new_value, button_num);
                                        vrCallbackInvokeDynamic(((vrControl *)(aux->button_inputs[button_num]))->callback, 1, new_value);
					break;
				}
			}
			break;
		}

		case MotionNotify: {
			XMotionEvent	*motion_event = (XMotionEvent *)&event;
			Window		root_window;	/* unused, but needed as an argument */
			int		xwin, ywin;	/* location of window in parent window */
			unsigned int	xsize, ysize;	/* size of window */
			unsigned int	bordersize;	/* unused, but needed as an argument */
			unsigned int	depth;		/* unused, but needed as an argument */
			int		dummy;		/* for other unused arguments */
			unsigned int	udummy;		/* for other unused arguments */
			int		count;

			/********************************************************************/
			/* calculate all the pointer values if any pointer method is in use */
			if (aux->pointer_as_valuator || aux->pointer_as_6sensorXY || aux->pointer_as_6sensorXZ || aux->pointer_as_6sensorRot) {
				unsigned int	x_screensize, y_screensize;

				XGetGeometry(motion_event->display, motion_event->window, &root_window, &xwin, &ywin, &xsize, &ysize, &bordersize, &depth);
				XGetGeometry(motion_event->display, motion_event->root, &root_window, &dummy, &dummy, &x_screensize, &y_screensize, &udummy, &udummy);

				/* calculate pointer values for the four possibilities */
				aux->pointer[X_WINDOW] = (motion_event->x / ((xsize-1) / 2.0)) - 1.0;
				aux->pointer[Y_WINDOW] = (motion_event->y / ((ysize-1) / 2.0)) - 1.0;
				aux->pointer[X_SCREEN] = (motion_event->x_root / ((x_screensize-1) / 2.0)) - 1.0;
				aux->pointer[Y_SCREEN] = (motion_event->y_root / ((y_screensize-1) / 2.0)) - 1.0;
			}

			/****************************************************/
			/* use pointer values as valuator info if requested */
			if (aux->pointer_as_valuator) {

				/* check all the possible (mouse pointer) valuators */
				for (count = 0; count < MAX_POINTERINPUTS; count++) {
					if (aux->pointer_inputs[count] != NULL) {
						switch (aux->pointer_inputs[count]->input_type) {
						case VRINPUT_VALUATOR:
							vrAssignValuatorValue((vrValuator *)(aux->pointer_inputs[count]), aux->pointer[count] * aux->pointer_sign[count]);
							break;

						default:
							/* TODO: ... */
							break;
						}
					}
				}
			}

			if (aux->pointer_as_6sensorXY) {
				aux->sensor6_delta.t[VR_X] =  aux->pointer[X_WINDOW] * POINTER_TRANSL_DELTA;
				aux->sensor6_delta.t[VR_Y] = -aux->pointer[Y_WINDOW] * POINTER_TRANSL_DELTA;
			}

			if (aux->pointer_as_6sensorXZ) {
				aux->sensor6_delta.t[VR_X] =  aux->pointer[X_WINDOW] * POINTER_TRANSL_DELTA;
				aux->sensor6_delta.t[VR_Z] =  aux->pointer[Y_WINDOW] * POINTER_TRANSL_DELTA;
			}

			if (aux->pointer_as_6sensorRot) {
				aux->sensor6_delta.r[VR_AZIM] = -aux->pointer[X_WINDOW] * POINTER_ROTATE_DELTA;
				aux->sensor6_delta.r[VR_ELEV] = -aux->pointer[Y_WINDOW] * POINTER_ROTATE_DELTA;
			}

#if 0
			vrPrintf("TODO: handle motion events: %d (%d, %d)  (%d, %d): xsize = %d, ysize = %d\n",
				event.type,
				motion_event->x, motion_event->y,
				motion_event->x_root, motion_event->y_root,
				xsize, ysize);
#endif

			break;
		}

#if 0 /* 06/13/2006 -- it was decided to use the FocusIn/Out events instead of Enter/LeaveNotify */
		case EnterNotify:
			if (!aux->keyrepeat) {
				vrDbgPrintf("FreeVR: Mouse entered window '%s' -- Disabling Keyrepeat\n",
					(_GetXWindowInfo(aux, event.xany.window) == NULL ? "input window" : _GetXWindowInfo(aux, event.xany.window)->name));
				XAutoRepeatOff(aux->display);
			}
			break;

		case LeaveNotify:
			if (!aux->keyrepeat) {
				vrDbgPrintf("FreeVR: Mouse exitted window '%s' -- Enabling Keyrepeat\n",
					(_GetXWindowInfo(aux, event.xany.window) == NULL ? "input window" : _GetXWindowInfo(aux, event.xany.window)->name));
				XAutoRepeatOn(aux->display);
			}
			break;
#endif

		case FocusIn:
			if (!aux->keyrepeat) {
				vrDbgPrintf("FreeVR: Focus-in event for window '%s' -- Disabling Keyrepeat\n", (_GetXWindowInfo(aux, event.xany.window) == NULL ? "input window" : _GetXWindowInfo(aux, event.xany.window)->name));
				XAutoRepeatOff(aux->display);
			}
			break;

		case FocusOut:
			if (!aux->keyrepeat) {
				vrDbgPrintf("FreeVR: Focus-out event for window '%s' -- Enabling Keyrepeat\n", (_GetXWindowInfo(aux, event.xany.window) == NULL ? "input window" : _GetXWindowInfo(aux, event.xany.window)->name));
				XAutoRepeatOn(aux->display);
			}
			break;

		default: {
			/*** Here we check for events from the extension devices, which ***/
			/***   don't have hard-coded type numbers, and therefore can't  ***/
			/***   be checked in a switch() statement.                      ***/

			int	input_count;
			int	handled = 0;

			/* For some bizarre reason (I have to guess it's bizarre), there */
			/*   is no generic XDeviceEvent structure.  However, they all    */
			/*   have the deviceid field in the top group of fields that is  */
			/*   constant for all XDevice...Event types, so we choose one at */
			/*   random to get the ID of the device.                         */
			int	device_id = ((XDeviceMotionEvent *)&event)->deviceid;

			for (input_count = 0; input_count < aux->num_extinputs; input_count++) {

				/* find a device match */
				if (device_id == aux->exdevice_id[aux->extinput_edev[input_count]]){
					char	*device_name = aux->exdevice_names[aux->extinput_edev[input_count]];

					/*** This block is basically the same structure as the  ***/
					/***   switch structure we are in, in that we're looking **/
					/***   for cases that match the current event type.     ***/
					/*** However, the numeric representation of these events **/
					/***   is not a known constant for X-extension devices. ***/
					/***   Therefore we use if-statements instead of switch. **/

	/*	case DeviceButtonPress:   */
	/*	case DeviceButtonRelease: */
					if ((event.type == aux->extinput_event_press[input_count])||
						(event.type == aux->extinput_event_release[input_count])) {

						XDeviceButtonEvent	*button_event = (XDeviceButtonEvent *)&event;
						int			button_num = button_event->button;

						/* make sure it's the correct button */
						if (button_num == aux->extinput_evnum[input_count]) {
							int	new_value = (event.type == aux->extinput_event_press[input_count]);

							vrDbgPrintfN(ALMOSTNEVER_DBGLVL, "_XwindowsGetData: "
								RED_TEXT "found a %d event, and we're handling it with button %d (val = %d)\n" NORM_TEXT,
								event.type, button_num, new_value);

							switch (aux->extension_inputs[input_count]->input_type) {

							case VRINPUT_BINARY:
								vrDbgPrintfN(INPUT_DBGLVL, "_XwindowsGetData: "
									"Assigning a value of %d to Xwindows '%s' button %d\n",
									new_value, device_name, button_num);

								vrAssign2switchValue((vr2switch *)(aux->extension_inputs[input_count]), new_value /*, vrCurrentWallTime() */);
								handled = 1;
								break;

							case VRINPUT_CONTROL:
								vrDbgPrintfN(INPUT_DBGLVL, "_XwindowsGetData: "
									"Activating a callback with %d to Xwindows '%s' button %d\n",
									new_value, device_name, button_num);

								vrCallbackInvokeDynamic(((vrControl *)(aux->extension_inputs[input_count]))->callback, 1, new_value);
								handled = 1;
								break;
							}
						}

	/*	case DeviceMotionRelease: */
					} else if (event.type == aux->extinput_event_val[input_count]) {
						XDeviceMotionEvent	*motion_event = (XDeviceMotionEvent *)&event;
						int			valuator_num = motion_event->first_axis;

						/* make sure it's the correct valuator number */
						if (valuator_num == aux->extinput_evnum[input_count]) {
							float	new_value = ((float)motion_event->axis_data[0] + aux->extinput_shift[input_count]) * aux->extinput_scale[input_count];

							vrDbgPrintfN(ALMOSTNEVER_DBGLVL, "_XwindowsGetData: "
								RED_TEXT "found a %d event, and we're handling it with valuator %d (val = %f)\n" NORM_TEXT,
								event.type, valuator_num, new_value);

							switch (aux->extension_inputs[input_count]->input_type) {

							case VRINPUT_VALUATOR:
								vrDbgPrintfN(INPUT_DBGLVL, "_XwindowsGetData: "
									"Assigning a value of %f to Xwindows '%s' valuator %d\n",
									new_value, device_name, valuator_num);

								vrAssignValuatorValue((vrValuator *)(aux->extension_inputs[input_count]), new_value /*, vrCurrentWallTime() */);

								handled = 1;
								break;

							case VRINPUT_6SENSOR:
								/* TODO: ... (do we still want this?) */
								/* if input type is a sim-6, then affect the aux->sensor6_delta values */
vrPrintf(RED_TEXT "got a 6-sensor input from an ext-val-input (input type = %d)\n" NORM_TEXT, aux->extension_inputs[input_count]->input_type);
								break;

							case VRINPUT_CONTROL:
								vrDbgPrintfN(INPUT_DBGLVL, "_XwindowsGetData: "
									"Activating a callback with %f to Xwindows '%s' valuator %d\n",
									new_value, device_name, valuator_num);

								vrCallbackInvokeDynamic(((vrControl *)(aux->extension_inputs[input_count]))->callback, 1, new_value);
								handled = 1;
								break;

							default:
								vrMsgPrintf("_XwindowsGetData: " RED_TEXT
									"unknown input type for extension valuator input device.\n" NORM_TEXT);
								break;
							}
						}
					}
				}
			}

			if (!handled) {
				vrDbgPrintfN(INPUT_DBGLVL, "Ignoring an X-event of type %d, from device %d (input not specified).\n", event.type, device_id);
			}
		      } break;
		}
	}

	/* If a simulated 6-sensor is active, then add the current delta Euler to it */
	if (aux->active_sim6sensor >= 0) {
		vr6sensor	*sensor;

		/* only assign new sensor data if it changes */
		if (aux->sensor6_delta.t[VR_X] != 0.0 ||
			aux->sensor6_delta.t[VR_Y] != 0.0 ||
			aux->sensor6_delta.t[VR_Z] != 0.0 ||
			aux->sensor6_delta.r[VR_AZIM] != 0.0 ||
			aux->sensor6_delta.r[VR_ELEV] != 0.0 ||
			aux->sensor6_delta.r[VR_ROLL] != 0.0) {

			sensor = (vr6sensor *)(aux->sensor6_inputs[aux->active_sim6sensor]);
			vrAssign6sensorValueFromValuators(sensor, aux->sensor6_delta.t, &(aux->sensor6_options), -1);
			vrAssign6sensorActiveValue(sensor, 1);
		}
	}
}


	/*****************************************************************/
	/*    Function(s) for parsing Xwindows "input" declarations.     */
	/*                                                               */
	/*  These _Xwindows<type>Input() functions are called during the */
	/*  CREATE phase of the input interface.                         */

/**************************************************************************/
static vrInputMatch _XwindowsKeyInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;
	unsigned int		keyvalue;

	if (!strcasecmp(dti->device, "keyboard")) {
		keyvalue = _XwindowsKeyValue(dti->instance);
		if (keyvalue == 0) {
			vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_XwindowsKeyInput: " RED_TEXT "Warning, key['%s'] did not match any known key\n" NORM_TEXT, dti->instance);
			return VRINPUT_MATCH_UNABLE;	/* device match, but still unable to handle request */
		}

		if (aux->key_inputs[keyvalue] != NULL)
			vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_XwindowsKeyInput: " RED_TEXT "Warning, new input from %s:%s[%s] overwriting old value.\n" NORM_TEXT,
				dti->device, dti->type, dti->instance);

		aux->key_inputs[keyvalue] = input;
		vrDbgPrintfN(INPUT_DBGLVL, "mapped key event of value 0x%02x to input pointer = %#p)\n", keyvalue, aux->key_inputs[keyvalue]);

		/* enable Key Press/Release events */
		aux->event_mask |= KeyPressMask | KeyReleaseMask;

		return VRINPUT_MATCH_ABLE;	/* successful match */
	} else {
		/* TODO: all the X-devices other than the keyboard */
	}

	return VRINPUT_NOMATCH;	/* device mis-match */
}


/**************************************************************************/
static vrInputMatch _XwindowsMouseButtonInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;
	unsigned int		button_num;

	if (!strcasecmp(dti->device, "mouse")) {
		button_num = vrAtoI(dti->instance);
		if (button_num == 0)
			button_num = _XwindowsButtonValue(dti->instance);

		if (aux->button_inputs[button_num] != NULL)
			vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_XwindowsMouseButtonInput: Warning, new input from %s:%s[%s] overwriting old value.\n",
				dti->device, dti->type, dti->instance);
		if (button_num == 0)
			vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_XwindowsMouseButtonInput: Warning, button['%s'] did not match any known button\n", dti->instance);
		aux->button_inputs[button_num] = input;
		vrDbgPrintfN(INPUT_DBGLVL, "mapped button event of value %d to input pointer = %#p)\n", button_num, aux->button_inputs[button_num]);

		/* enable Mouse Button Press/Release events */
		aux->event_mask |= ButtonPressMask | ButtonReleaseMask;

		return VRINPUT_MATCH_ABLE;	/* successful match */
	}

	return VRINPUT_NOMATCH;	/* device mis-match */
}


/**************************************************************************/
static vrInputMatch _XwindowsPointerInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;
	unsigned int		pointer_num;
	float			sign;

	if (dti->instance[0] == '-') {
		pointer_num = _XwindowsPointerValue(&(dti->instance[1]));
		sign = -1.0;
	} else {
		pointer_num = _XwindowsPointerValue(dti->instance);
		sign =  1.0;
	}

	if (pointer_num > MAX_POINTERINPUTS) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_XwindowsPointerInput: Warning, pointer['%s'] did not match any known pointer\n", dti->instance);
		return VRINPUT_MATCH_UNABLE;	/* device match, but still unable to handle request */
	}

	if (aux->pointer_inputs[pointer_num] != NULL)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_XwindowsPointerInput: Warning, new input from %s:%s[%s] overwriting old value.\n",
			dti->device, dti->type, dti->instance);

	aux->pointer_inputs[pointer_num] = input;
	aux->pointer_sign[pointer_num] = sign;
	vrDbgPrintfN(INPUT_DBGLVL, "mapped pointer event of value %d (%f) to input pointer = %#p)\n",
		pointer_num, sign, aux->pointer_inputs[pointer_num]);


	/* enable Pointer movement events */
	aux->event_mask |= PointerMotionMask;

	return VRINPUT_MATCH_ABLE;	/* successful match */
}


/**************************************************************************/
static vrInputMatch _XwindowsExtButtonInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;
	unsigned int		button_num;		/* button number of the device */
	unsigned int		button_index;		/* where the button is store in the list*/
	int			device_num;		/* index of the x-extension device */
	int			count;

	button_num = vrAtoI(dti->instance);

	if (aux->num_extinputs == MAX_EXTINPUTS) {
		/* no room at the innput */
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_XwindowsExtButtonInput: "
			"Warning, too many ""button"" inputs, ignoring button[%s].\n",
			dti->instance);

		return VRINPUT_MATCH_UNABLE;	/* input declaration match, but unable to create */
	}
	/* else: add button to the list */


	/* TODO: perhaps have a method of seeing if this is a duplicate */
	/*   see _PinchgloveContactInput() for an example.              */


	/* Because "extension" X-inputs won't be known until the X display  */
	/*   is open, we can't verify the existence of the device at this   */
	/*   point.  So, we store a list of all requested inputs, and check */
	/*   if we a referring to an existing request, or a new one, in     */
	/*   which case we add it to the list of existing requests.         */
	/* These devices will then need to be opened when the Xwindows input*/
	/*   device is opened.  (Note: we're only at the CREATE stage now.) */

	/* check device list */
	device_num = -1;
	for (count = 0; count < aux->num_exdevices; count++) {
		if (!strcasecmp(dti->device, aux->exdevice_names[count]))
			device_num = count;
	}

	if (device_num == -1) {
		/* that device hasn't been requested yet, so add to list if possible */

		if (aux->num_exdevices == MAX_EXDEVICES) {
			/* ooops, we're already at the limit of devices */

			vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_XwindowsExtButtonInput: "
				"Warning, too many X-extension devices requested, can't add '%s'.\n",
				dti->device);

			return VRINPUT_MATCH_UNABLE;	/* input declaration match, but unable to create */
		}

		/* okay, it is possible, so add to the list */
		device_num = aux->num_exdevices;
		aux->num_exdevices++;

		aux->exdevice_names[device_num] = vrShmemStrDup(dti->device);

		vrDbgPrintfN(XWIN_DBGLVL, "_XwindowsExtButtonInput: added '%s' x-extension device as %d to list.\n", aux->exdevice_names[device_num], device_num);
	}

	/* now add the input information to the list of extension-inputs */
	button_index = aux->num_extinputs;
	aux->num_extinputs++;

	aux->extension_inputs[button_index] = input;
	aux->extinput_type[button_index] = BUTTON_TYPE;
	aux->extinput_scale[button_index] = 0.0;		/* unused by button inputs */
	aux->extinput_shift[button_index] = 0.0;		/* unused by button inputs */
	aux->extinput_edev[button_index] = device_num;
	aux->extinput_evnum[button_index] = button_num;
	vrDbgPrintfN(INPUT_DBGLVL, "mapped ex-device %d ext-input %d to input button = %#p)\n",
		device_num, button_index, aux->extension_inputs[button_index]);

vrPrintf(BOLD_TEXT "just created a button ext-input from device = '%s', type = '%s', instance = '%s', input = %#p\n" NORM_TEXT, dti->device, dti->type, dti->instance, aux->extension_inputs[button_index]);


	return VRINPUT_MATCH_ABLE;	/* successful match and creation */
}


/**************************************************************************/
static vrInputMatch _XwindowsKeyboardInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
static	char			*whitespace = " \t\r\b";
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;
	vrInputType		input_type = input->input_type;
	unsigned int		keyboard_value;
	char			*instance_args;

	/* No check for "dti->device" */

	keyboard_value = vrAtoI(dti->instance);			/* which keyboard? */

	if (keyboard_value >= MAX_KEYBOARDS) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_XwindowsKeyboardInput: " RED_TEXT "Warning, no space to allocate input for keyboard[%s] \n" NORM_TEXT, dti->instance);
		return VRINPUT_MATCH_UNABLE;	/* device match, but still unable to handle request */
	}

	if (aux->keyboard_inputs[keyboard_value] != NULL)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_XwindowsKeyboardInput: " RED_TEXT "Warning, new input from %s:%s[%s] overwriting old value.\n" NORM_TEXT,
			dti->device, dti->type, dti->instance);

	vrDbgPrintfN(INPUT_DBGLVL, "_XwindowsKeyboardInput(): Keyboard input is of type '%s'\n", vrInputTypeName(input_type));
	aux->keyboard_inputs[keyboard_value] = input;
	aux->keyboard_type[keyboard_value] = KEYBOARD_LAST;
	aux->keyboard_scale[keyboard_value] = 0.2;
	aux->keyboard_count[keyboard_value] = 0;

	/* special parsing of arguments based on the type of input */
	switch (input_type) {
	case VRINPUT_NWAY:
		/* For N-switches, determine value of the "type" of input: "last" or "release" */
		instance_args = strchr(dti->instance, ',');

		/* if no comma, then no arguments as go with the default: "last" */
		if (instance_args != NULL) {
			instance_args++;					/* skip comma */
			instance_args += strspn(instance_args, whitespace);	/* skip white */
			if (!strcasecmp(instance_args, "last"))
				aux->keyboard_type[keyboard_value] = KEYBOARD_LAST;
			else if (!strcasecmp(instance_args, "release"))
				aux->keyboard_type[keyboard_value] = KEYBOARD_RELEASE;
			else if (!strcasecmp(instance_args, "count"))
				aux->keyboard_type[keyboard_value] = KEYBOARD_COUNT;
			else
				vrDbgPrintfN(CONFIG_WARN_DBGLVL, "Warning: Invalid N-switch keyboard style: '%s'.\n", instance_args);
		}
		break;

	case VRINPUT_VALUATOR:
		instance_args = strchr(dti->instance, ',');
		if (instance_args != NULL)
			aux->keyboard_scale[keyboard_value] = atof(instance_args+1);
		break;

	default:
		break;
	}


	vrDbgPrintfN(INPUT_DBGLVL, "_XwindowsKeyboardInput(): Mapped leftover key events from keyboard value %d to input pointer = %#p, using method %d or with scale %.2f\n",
		keyboard_value, aux->keyboard_inputs[keyboard_value], aux->keyboard_type[keyboard_value], aux->keyboard_scale[keyboard_value]);

	/* enable Key Press/Release events */
	aux->event_mask |= KeyPressMask | KeyReleaseMask;

	return VRINPUT_MATCH_ABLE;	/* successful match */
}


/**************************************************************************/
static vrInputMatch _XwindowsExtValuatorInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;
	unsigned int		valuator_num;		/* valuator number of the device */
	unsigned int		valuator_index;		/* where the valuator is store in the list*/
	float			sign = 1.0;		/* default sign multiplier */
	float			scale = 1.0;		/* default valuator scale factor */
	float			shift = 0.0;		/* default valuator shift factor */
	char			*scale_instance;	/* pointer to scale portion of instance string */
	char			*shift_instance;	/* pointer to shift portion of instance string */
	int			device_num;		/* index of the x-extension device */
	int			count;

	if (dti->instance[0] == '-') {
		valuator_num = vrAtoI(&(dti->instance[1]));
		sign = -1.0;
	} else {
		valuator_num = vrAtoI(dti->instance);
		sign =  1.0;
	}

	/* check for non-default scale and shift values */
	scale_instance = strchr(dti->instance, ',');
	if (scale_instance != NULL) {
		scale = atof(scale_instance+1);

		shift_instance = strchr(scale_instance+1, ',');
		if (shift_instance != NULL)
			shift = atof(shift_instance+1);
	}

	if (aux->num_extinputs == MAX_EXTINPUTS) {
		/* no room at the innput */
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_XwindowsExtValuatorInput: "
			"Warning, too many ""valuator"" inputs, ignoring valuator[%s].\n",
			dti->instance);

		return VRINPUT_MATCH_UNABLE;	/* input declaration match, but unable to create */
	}
	/* else: add valuator to the list */


	/* TODO: perhaps have a method of seeing if this is a duplicate */
	/*   see _PinchgloveContactInput() for an example.              */


	/* Because "extension" X-inputs won't be known until the X display  */
	/*   is open, we can't verify the existence of the device at this   */
	/*   point.  So, we store a list of all requested inputs, and check */
	/*   if we a referring to an existing request, or a new one, in     */
	/*   which case we add it to the list of existing requests.         */
	/* These devices will then need to be opened when the Xwindows input*/
	/*   device is opened.  (Note: we're only at the CREATE stage now.) */

	/* check device list */
	device_num = -1;
	for (count = 0; count < aux->num_exdevices; count++) {
		if (!strcasecmp(dti->device, aux->exdevice_names[count]))
			device_num = count;
	}

	if (device_num == -1) {
		/* that device hasn't been requested yet, so add to list if possible */

		if (aux->num_exdevices == MAX_EXDEVICES) {
			/* ooops, we're already at the limit of devices */

			vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_XwindowsExtValuatorInput: "
				"Warning, too many X-extension devices requested, can't add '%s'.\n",
				dti->device);

			return VRINPUT_MATCH_UNABLE;	/* input declaration match, but unable to create */
		}

		/* okay, it is possible, so add to the list */
		device_num = aux->num_exdevices;
		aux->num_exdevices++;

		aux->exdevice_names[device_num] = vrShmemStrDup(dti->device);

		vrDbgPrintfN(XWIN_DBGLVL, "_XwindowsExtValuatorInput: added '%s' x-extension device as %d to list.\n", aux->exdevice_names[device_num], device_num);
	}

	/* now add the input information to the list of extension-inputs */
	valuator_index = aux->num_extinputs;
	aux->num_extinputs++;

	aux->extension_inputs[valuator_index] = input;
	aux->extinput_type[valuator_index] = MOTION_TYPE;
	aux->extinput_scale[valuator_index] = sign * scale;
	aux->extinput_shift[valuator_index] = sign * shift;
	aux->extinput_edev[valuator_index] = device_num;
	aux->extinput_evnum[valuator_index] = valuator_num;
	vrDbgPrintfN(INPUT_DBGLVL, "mapped ex-device %d ext-input %d (%f) to input valuator = %#p)\n",
		device_num, valuator_index, sign, aux->extension_inputs[valuator_index]);

	vrDbgPrintfN(INPUT_DBGLVL, BOLD_TEXT "just created a valuator ext-input from device = '%s', type = '%s', instance = '%s'\n" NORM_TEXT, dti->device, dti->type, dti->instance);


	return VRINPUT_MATCH_ABLE;	/* successful match and creation */
}


/**************************************************************************/
/* NOTE: that if (ie. once) non-self controls work, then the simulated 6-sensor */
/*   doesn't need to be part of the Xwindows device at all, but could be a      */
/*   separate input device, that just uses X inputs to callback it's controls.  */
/* 6/21/2001: I guess this initializes each of the simulated 6-sensor inputs. */
/*   (which at the moment means we should pass "-1" (nop) value as the oob flag */
static vrInputMatch _XwindowsSim6Input(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;
	vrEuler			*sensor6_value;
	int			sensor6_num;
	vr6sensor		*sensor6_input;
	vrMatrix		tmpmat;

	sensor6_num = vrAtoI(dti->instance);

	if (sensor6_num < 0 || sensor6_num > MAX_6SENSORS) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_XwindowsSim6Input: Warning, 6sensor['%s'] did not match any known 6sensor\n", dti->instance);
		return VRINPUT_MATCH_UNABLE;	/* device match, but still unable to handle request */
	}

	if (aux->sensor6_inputs[sensor6_num] != NULL)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_XwindowsSim6Input: Warning, new input from %s:%s[%s] overwriting old value.\n",
			dti->device, dti->type, dti->instance);

	aux->sensor6_inputs[sensor6_num] = input;
	sensor6_input = (vr6sensor *)(aux->sensor6_inputs[sensor6_num]);

	sensor6_value = &(aux->sensor6_values[sensor6_num]);
	sensor6_value->t[VR_X] = 0.0;
	sensor6_value->t[VR_Y] = 0.0;
	sensor6_value->t[VR_Z] = 0.0;
	sensor6_value->r[VR_AZIM] = 0.0;
	sensor6_value->r[VR_ELEV] = 0.0;
	sensor6_value->r[VR_ROLL] = 0.0;

	vrAssign6sensorR2Exform(sensor6_input, strchr(dti->instance, ','));
	vrAssign6sensorValue(sensor6_input, vrMatrixSetEulerAzimaxis(&tmpmat, sensor6_value, VR_Y), -1 /* , vrCurrentWallTime() */);
	vrAssign6sensorActiveValue(sensor6_input, 0);
	vrAssign6sensorErrorValue(sensor6_input, 0);

	vrDbgPrintfN(INPUT_DBGLVL, "mapped simulated 6-sensor number %d to input pointer = %#p)\n",
		sensor6_num, sensor6_input);

	return VRINPUT_MATCH_ABLE;	/* successful match */
}


	/************************************************************/
	/***************** FreeVR Callback routines *****************/
	/************************************************************/

	/**********************************************************/
	/*    Callbacks for controlling the Xwindows features.    */
	/*                                                        */

/************************************************************/
static void _XwindowsLoudBellCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	XBell(aux->display, 100);
}

/************************************************************/
static void _XwindowsSystemPauseToggleCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

        if (value == 0)
                return;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	devinfo->context->paused ^= 1;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Xwindows Control: system_pause = %d.\n",
		devinfo->context->paused);
}

/************************************************************/
static void _XwindowsPrintContextStructCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

        if (value == 0)
                return;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

        vrFprintContext(stdout, devinfo->context, verbose);
}

/************************************************************/
static void _XwindowsPrintConfigStructCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

        if (value == 0)
                return;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

        vrFprintConfig(stdout, devinfo->context->config, verbose);
}

/************************************************************/
static void _XwindowsPrintInputStructCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

        if (value == 0)
                return;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

        vrFprintInput(stdout, devinfo->context->input, verbose);
}

/************************************************************/
static void _XwindowsPrintStructCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	_XwindowsPrintStruct(stdout, aux, verbose);
}

/************************************************************/
static void _XwindowsPrintHelpCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	_XwindowsPrintHelp(stdout, aux);
}

/************************************************************/
/*  NOTE: not all X keyboards can change the volume of the bell */
static void _XwindowsSoftBellCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	XBell(aux->display, 10);
}

/************************************************************/
static void _XwindowsSensorNextCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	if (devinfo->num_6sensors == 0) {
		vrDbgPrintfN(SELFCTRL_DBGLVL, "Xwindows Control: next sensor -- no sensors available.\n");
		return;
	}

	vrAssign6sensorActiveValue(((vr6sensor *)aux->sensor6_inputs[aux->active_sim6sensor]), 0);
	do {
		aux->active_sim6sensor++;
	} while (aux->sensor6_inputs[aux->active_sim6sensor] == NULL && aux->active_sim6sensor < MAX_6SENSORS);

	if (aux->sensor6_inputs[aux->active_sim6sensor] == NULL || aux->active_sim6sensor >= MAX_6SENSORS)
{
		for (aux->active_sim6sensor = 0; aux->active_sim6sensor < MAX_6SENSORS && aux->sensor6_inputs[aux->active_sim6sensor] == NULL; aux->active_sim6sensor++);
	}

	if (aux->active_sim6sensor == MAX_6SENSORS) {
		vrDbgPrintfN(SELFCTRL_DBGLVL, "Xwindows Control: next sensor -- could not find any available sensors.\n");
		return;
	}
	vrAssign6sensorActiveValue(((vr6sensor *)aux->sensor6_inputs[aux->active_sim6sensor]), 1);

	vrDbgPrintfN(SELFCTRL_DBGLVL, "Xwindows Control: 6sensor[%d] now active.\n", aux->active_sim6sensor);
}

/* TODO: see if there is a way to call this as an N-switch */
/************************************************************/
static void _XwindowsSensorSetCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

	if (value == aux->active_sim6sensor)
		return;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	if (value < 0 || value >= MAX_6SENSORS) {
		vrDbgPrintfN(SELFCTRL_DBGLVL, "Xwindows Control: set sensor (%d) -- out of range.\n", value);
	}

	if (aux->sensor6_inputs[value] == NULL) {
		vrDbgPrintfN(SELFCTRL_DBGLVL, "Xwindows Control: set sensor (%d) -- no such sensor available.\n", value);
		return;
	}

	vrAssign6sensorActiveValue(((vr6sensor *)aux->sensor6_inputs[aux->active_sim6sensor]), 0);
	aux->active_sim6sensor = value;
	vrAssign6sensorActiveValue(((vr6sensor *)aux->sensor6_inputs[aux->active_sim6sensor]), 1);

	vrDbgPrintfN(SELFCTRL_DBGLVL, "Xwindows Control: 6sensor[%d] now active.\n", aux->active_sim6sensor);
}

/************************************************************/
static void _XwindowsSensorSet0Callback(vrInputDevice *devinfo, int value)
{	_XwindowsSensorSetCallback(devinfo, 0); }

/************************************************************/
static void _XwindowsSensorSet1Callback(vrInputDevice *devinfo, int value)
{	_XwindowsSensorSetCallback(devinfo, 1); }

/************************************************************/
static void _XwindowsSensorSet2Callback(vrInputDevice *devinfo, int value)
{	_XwindowsSensorSetCallback(devinfo, 2); }

/************************************************************/
static void _XwindowsSensorSet3Callback(vrInputDevice *devinfo, int value)
{	_XwindowsSensorSetCallback(devinfo, 3); }

/************************************************************/
static void _XwindowsSensorSet4Callback(vrInputDevice *devinfo, int value)
{	_XwindowsSensorSetCallback(devinfo, 4); }

/************************************************************/
static void _XwindowsSensorSet5Callback(vrInputDevice *devinfo, int value)
{	_XwindowsSensorSetCallback(devinfo, 5); }

/************************************************************/
static void _XwindowsSensorSet6Callback(vrInputDevice *devinfo, int value)
{	_XwindowsSensorSetCallback(devinfo, 6); }

/************************************************************/
static void _XwindowsSensorSet7Callback(vrInputDevice *devinfo, int value)
{	_XwindowsSensorSetCallback(devinfo, 7); }

/************************************************************/
static void _XwindowsSensorSet8Callback(vrInputDevice *devinfo, int value)
{	_XwindowsSensorSetCallback(devinfo, 8); }

/************************************************************/
static void _XwindowsSensorSet9Callback(vrInputDevice *devinfo, int value)
{	_XwindowsSensorSetCallback(devinfo, 9); }

/************************************************************/
static void _XwindowsSensorResetCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;
	int			sensor6_num = aux->active_sim6sensor;
	vrEuler			*sensor6_value = &(aux->sensor6_values[sensor6_num]);
	vrMatrix		tmpmat;
	vr6sensor		*sensor;

	if (value == 0 || sensor6_num < 0)
		return;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	/* reset the current delta values */
	aux->sensor6_delta.t[VR_X] = 0.0;
	aux->sensor6_delta.t[VR_Y] = 0.0;
	aux->sensor6_delta.t[VR_Z] = 0.0;
	aux->sensor6_delta.r[VR_AZIM] = 0.0;
	aux->sensor6_delta.r[VR_ELEV] = 0.0;
	aux->sensor6_delta.r[VR_ROLL] = 0.0;

	/* reset the actual sensor data */
	sensor6_value->t[VR_X] = 0.0;
	sensor6_value->t[VR_Y] = 0.0;
	sensor6_value->t[VR_Z] = 0.0;
	sensor6_value->r[VR_AZIM] = 0.0;
	sensor6_value->r[VR_ELEV] = 0.0;
	sensor6_value->r[VR_ROLL] = 0.0;

	sensor = (vr6sensor *)(aux->sensor6_inputs[sensor6_num]);
	vrAssign6sensorValue(sensor, vrMatrixSetEulerAzimaxis(&tmpmat, sensor6_value, VR_Y), 0 /* , vrCurrentWallTime() */);
	vrAssign6sensorActiveValue(sensor, -1);
	vrAssign6sensorErrorValue(sensor, 0);
}

/************************************************************/
static void _XwindowsSensorResetAllCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;
	vrMatrix		tmpmat;
	int			count;
	vr6sensor		*sensor;

	if (value == 0)
		return;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	/* reset the current delta values */
	aux->sensor6_delta.t[VR_X] = 0.0;
	aux->sensor6_delta.t[VR_Y] = 0.0;
	aux->sensor6_delta.t[VR_Z] = 0.0;
	aux->sensor6_delta.r[VR_AZIM] = 0.0;
	aux->sensor6_delta.r[VR_ELEV] = 0.0;
	aux->sensor6_delta.r[VR_ROLL] = 0.0;

	vrMatrixSetIdentity(&tmpmat);

	for (count = 0; count < MAX_6SENSORS; count++) {
		if (aux->sensor6_inputs[count] != NULL) {
			sensor = (vr6sensor *)(aux->sensor6_inputs[count]);
			vrAssign6sensorValue(sensor, &tmpmat, 0 /* , vrCurrentWallTime() */);
			vrAssign6sensorActiveValue(sensor, (count == aux->active_sim6sensor));
			vrAssign6sensorErrorValue(sensor, 0);
		}
	}
}

/************************************************************/
static void _XwindowsPointerAsValuator(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	aux->pointer_as_valuator = value;

	/* TODO: consider the option of resetting the current valuator */
	/*   values to 0.0 when no longer enabled.                     */

	/* Generate a MotionEvent in order to get the initial */
	/*   values set without waiting for the mouse to move.*/
#if 1
	XWarpPointer(aux->display, None, None, 0,0,0,0, /* x=*/0, /* y=*/0);
#else
	/* TODO: I thought this method might be a little cleaner than */
	/*   the XWarpPointer() method, but currently it doesn't work,*/
	/*   and the warp-pointer method does, so use it for now.     */
	{
		XMotionEvent	event;

		event.type = MotionNotify;
		event.x = 0;
		event.y = 0;

		XSendEvent(aux->display, PointerWindow, False, 0, &event);
	}
#endif
}

/************************************************************/
static void _XwindowsPointerAsXYCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	aux->pointer_as_6sensorXY = value;

	if (value) {
		/* Generate a MotionEvent in order to get the initial */
		/*   values set without waiting for the mouse to move.*/
		XWarpPointer(aux->display, None, None, 0,0,0,0, /* x=*/0, /* y=*/0);
	} else {
		/* turn off sensor movement */
		aux->sensor6_delta.t[VR_X] = 0.0;
		aux->sensor6_delta.t[VR_Y] = 0.0;
	}
}

/************************************************************/
static void _XwindowsPointerAsXZCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	aux->pointer_as_6sensorXZ = value;

	if (value) {
		/* Generate a MotionEvent in order to get the initial */
		/*   values set without waiting for the mouse to move.*/
		XWarpPointer(aux->display, None, None, 0,0,0,0, /* x=*/0, /* y=*/0);
	} else {
		/* turn off sensor movement */
		aux->sensor6_delta.t[VR_X] = 0.0;
		aux->sensor6_delta.t[VR_Z] = 0.0;
	}
}

/************************************************************/
static void _XwindowsPointerAsRotCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	aux->pointer_as_6sensorRot = value;

	if (value) {
		/* Generate a MotionEvent in order to get the initial */
		/*   values set without waiting for the mouse to move.*/
		XWarpPointer(aux->display, None, None, 0,0,0,0, /* x=*/0, /* y=*/0);
	} else {
		/* turn off sensor movement */
		aux->sensor6_delta.r[VR_AZIM] = 0.0;
		aux->sensor6_delta.r[VR_ELEV] = 0.0;
	}
}

/************************************************************/
static void _XwindowsToggleRelativeAxesCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	aux->sensor6_options.relative_axis ^= 1;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Xwindows Control: relative_axis = %d.\n",
		aux->sensor6_options.relative_axis);
}

/************************************************************/
/* TODO: this should probably also go through all the sensor6's  */
/*   and move them to be within the allowed workspace when space */
/*   restriction is turned on.                                   */
static void _XwindowsToggleRestrictSpaceCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	aux->sensor6_options.restrict_space ^= 1;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Xwindows Control: restrict_space = %d.\n",
		aux->sensor6_options.restrict_space);
}

/************************************************************/
static void _XwindowsToggleReturnToZeroCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	aux->sensor6_options.return_to_zero ^= 1;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Xwindows Control: return_to_zero = %d.\n",
		aux->sensor6_options.return_to_zero);
}

/************************************************************/
static void _XwindowsToggleSilenceCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	aux->silent ^= 1;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Xwindows Control: silent = %d.\n", aux->silent);

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

}

static void _XwindowsSensorLeftCallback();
static void _XwindowsSensorRightCallback();
static void _XwindowsSensorSwapYZCallback();
static void _XwindowsSensorOutDownCallback();
static void _XwindowsSensorInUpCallback();
static void _XwindowsSensorDownOutCallback();
static void _XwindowsSensorPAzimuthCallback();
static void _XwindowsSensorNAzimuthCallback();
static void _XwindowsSensorPElevationCallback();
static void _XwindowsSensorNElevationCallback();
static void _XwindowsSensorRollCwCallback();
static void _XwindowsSensorRollCcwCallback();

/************************************************************/
static void _XwindowsSensorLeftCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

#if 0 /* Nope, decided against this one.  Chose to use a delta instead. */
	/* Arrow key commands can repeat (unlike most of the Xwindows input keys) */
	/*   TODO: probably should set the repeating to be faster, for smoother movement */
	XAutoRepeatOn(aux->display);
#endif

	if (value)
		aux->sensor6_delta.t[VR_X] += -KEY_TRANSL_DELTA;
	else	aux->sensor6_delta.t[VR_X] -= -KEY_TRANSL_DELTA;

	vrDbgPrintfN(XWIN_DBGLVL, "left     (%d): delta = (%.2f %.2f %.2f  %.2f %.2f %.2f)\n", value, aux->sensor6_delta.t[VR_X], aux->sensor6_delta.t[VR_Y], aux->sensor6_delta.t[VR_Z], aux->sensor6_delta.r[VR_AZIM], aux->sensor6_delta.r[VR_ELEV], aux->sensor6_delta.r[VR_ROLL]);

	return;
}

/************************************************************/
static void _XwindowsSensorRightCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	if (value)
		aux->sensor6_delta.t[VR_X] +=  KEY_TRANSL_DELTA;
	else	aux->sensor6_delta.t[VR_X] -=  KEY_TRANSL_DELTA;

	vrDbgPrintfN(XWIN_DBGLVL, "right    (%d): delta = (%.2f %.2f %.2f  %.2f %.2f %.2f)\n", value, aux->sensor6_delta.t[VR_X], aux->sensor6_delta.t[VR_Y], aux->sensor6_delta.t[VR_Z], aux->sensor6_delta.r[VR_AZIM], aux->sensor6_delta.r[VR_ELEV], aux->sensor6_delta.r[VR_ROLL]);

	return;
}

/************************************************************/
static void _XwindowsSensorUpInCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	if (value)
		aux->sensor6_delta.t[VR_Y] +=  KEY_TRANSL_DELTA;
	else	aux->sensor6_delta.t[VR_Y] -=  KEY_TRANSL_DELTA;

	vrDbgPrintfN(XWIN_DBGLVL, "up/in    (%d): delta = (%.2f %.2f %.2f  %.2f %.2f %.2f)\n", value, aux->sensor6_delta.t[VR_X], aux->sensor6_delta.t[VR_Y], aux->sensor6_delta.t[VR_Z], aux->sensor6_delta.r[VR_AZIM], aux->sensor6_delta.r[VR_ELEV], aux->sensor6_delta.r[VR_ROLL]);

	return;
}

/************************************************************/
static void _XwindowsSensorDownOutCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	if (value)
		aux->sensor6_delta.t[VR_Y] += -KEY_TRANSL_DELTA;
	else	aux->sensor6_delta.t[VR_Y] -= -KEY_TRANSL_DELTA;

	vrDbgPrintfN(XWIN_DBGLVL, "down/out (%d): delta = (%.2f %.2f %.2f  %.2f %.2f %.2f)\n", value, aux->sensor6_delta.t[VR_X], aux->sensor6_delta.t[VR_Y], aux->sensor6_delta.t[VR_Z], aux->sensor6_delta.r[VR_AZIM], aux->sensor6_delta.r[VR_ELEV], aux->sensor6_delta.r[VR_ROLL]);

	return;
}

/************************************************************/
static void _XwindowsSensorInUpCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	if (value)
		aux->sensor6_delta.t[VR_Z] += -KEY_TRANSL_DELTA;
	else	aux->sensor6_delta.t[VR_Z] -= -KEY_TRANSL_DELTA;

	vrDbgPrintfN(XWIN_DBGLVL, "in/up    (%d): delta = (%.2f %.2f %.2f  %.2f %.2f %.2f)\n", value, aux->sensor6_delta.t[VR_X], aux->sensor6_delta.t[VR_Y], aux->sensor6_delta.t[VR_Z], aux->sensor6_delta.r[VR_AZIM], aux->sensor6_delta.r[VR_ELEV], aux->sensor6_delta.r[VR_ROLL]);

	return;
}

/************************************************************/
static void _XwindowsSensorOutDownCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	if (value)
		aux->sensor6_delta.t[VR_Z] +=  KEY_TRANSL_DELTA;
	else	aux->sensor6_delta.t[VR_Z] -=  KEY_TRANSL_DELTA;

	vrDbgPrintfN(XWIN_DBGLVL, "out/down (%d): delta = (%.2f %.2f %.2f  %.2f %.2f %.2f)\n", value, aux->sensor6_delta.t[VR_X], aux->sensor6_delta.t[VR_Y], aux->sensor6_delta.t[VR_Z], aux->sensor6_delta.r[VR_AZIM], aux->sensor6_delta.r[VR_ELEV], aux->sensor6_delta.r[VR_ROLL]);

	return;
}

/************************************************************/
/* NOTE: this doesn't quite work, as the callbacks don't pass non-integers well */
/* TODO: fix this, today (1/22/03) I figured out how to pass floats -- by reference (see joydev) */
static void _XwindowsSensorLeftRightValCallback(vrInputDevice *devinfo, int /* TODO: s/b float */ value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	if (value)
		aux->sensor6_delta.t[VR_X] += -KEY_TRANSL_DELTA;
	else	aux->sensor6_delta.t[VR_X] -= -KEY_TRANSL_DELTA;

	vrDbgPrintfN(XWIN_DBGLVL, "left     (%f): delta = (%.2f %.2f %.2f  %.2f %.2f %.2f)\n", value, aux->sensor6_delta.t[VR_X], aux->sensor6_delta.t[VR_Y], aux->sensor6_delta.t[VR_Z], aux->sensor6_delta.r[VR_AZIM], aux->sensor6_delta.r[VR_ELEV], aux->sensor6_delta.r[VR_ROLL]);

	aux->sensor6_delta.t[VR_X] = value;
	vrDbgPrintfN(XWIN_DBGLVL, "leftright(%f): delta = (%.2f %.2f %.2f  %.2f %.2f %.2f)\n", value, aux->sensor6_delta.t[VR_X], aux->sensor6_delta.t[VR_Y], aux->sensor6_delta.t[VR_Z], aux->sensor6_delta.r[VR_AZIM], aux->sensor6_delta.r[VR_ELEV], aux->sensor6_delta.r[VR_ROLL]);

	return;
}

/************************************************************/
/* NOTE: this doesn't quite work, as the callbacks don't pass non-integers well */
static void _XwindowsSensorUpDownValCallback(vrInputDevice *devinfo, int /* TODO: s/b float */ value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	aux->sensor6_delta.t[VR_Y] = value;
	vrDbgPrintfN(XWIN_DBGLVL, "up/down  (%f): delta = (%.2f %.2f %.2f  %.2f %.2f %.2f)\n", value, aux->sensor6_delta.t[VR_X], aux->sensor6_delta.t[VR_Y], aux->sensor6_delta.t[VR_Z], aux->sensor6_delta.r[VR_AZIM], aux->sensor6_delta.r[VR_ELEV], aux->sensor6_delta.r[VR_ROLL]);

	return;
}

/************************************************************/
/* also need to do a sign change, since Y-movement is negated */
static void _XwindowsSensorSwapYZCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;
	float			swap_temp;

	aux->sensor6_options.swap_yz ^= 1;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	vrDbgPrintfN(XWIN_DBGLVL, "swap ui  (%d): delta = (%.2f %.2f %.2f  %.2f %.2f %.2f)\n", value, aux->sensor6_delta.t[VR_X], aux->sensor6_delta.t[VR_Y], aux->sensor6_delta.t[VR_Z], aux->sensor6_delta.r[VR_AZIM], aux->sensor6_delta.r[VR_ELEV], aux->sensor6_delta.r[VR_ROLL]);
	return;
}

/************************************************************/
/* also need to do a sign change, since Y-movement is negated */
/* also need to adjust for the different rates of rotation & translation movement */
static void _XwindowsSensorSwapRotTransCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;
	aux->sensor6_options.swap_transrot ^= 1;

	vrDbgPrintfN(XWIN_DBGLVL, "swap rt  (%d): delta = (%.2f %.2f %.2f  %.2f %.2f %.2f)\n", value, aux->sensor6_delta.t[VR_X], aux->sensor6_delta.t[VR_Y], aux->sensor6_delta.t[VR_Z], aux->sensor6_delta.r[VR_AZIM], aux->sensor6_delta.r[VR_ELEV], aux->sensor6_delta.r[VR_ROLL]);
	return;
}

/************************************************************/
static void _XwindowsSensorPAzimuthCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	if (value)
		aux->sensor6_delta.r[VR_AZIM] += -KEY_ROTATE_DELTA;
	else	aux->sensor6_delta.r[VR_AZIM] -= -KEY_ROTATE_DELTA;

	return;
}

/************************************************************/
static void _XwindowsSensorNAzimuthCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	if (value)
		aux->sensor6_delta.r[VR_AZIM] +=  KEY_ROTATE_DELTA;
	else	aux->sensor6_delta.r[VR_AZIM] -=  KEY_ROTATE_DELTA;

	return;
}

/************************************************************/
static void _XwindowsSensorPElevationCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	if (value)
		aux->sensor6_delta.r[VR_ELEV] +=  KEY_ROTATE_DELTA;
	else	aux->sensor6_delta.r[VR_ELEV] -=  KEY_ROTATE_DELTA;

	return;
}

/************************************************************/
static void _XwindowsSensorNElevationCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	if (value)
		aux->sensor6_delta.r[VR_ELEV] += -KEY_ROTATE_DELTA;
	else	aux->sensor6_delta.r[VR_ELEV] -= -KEY_ROTATE_DELTA;

	return;
}

/************************************************************/
static void _XwindowsSensorRollCwCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	if (value)
		aux->sensor6_delta.r[VR_ROLL] += -KEY_ROTATE_DELTA;
	else	aux->sensor6_delta.r[VR_ROLL] -= -KEY_ROTATE_DELTA;

	return;
}

/************************************************************/
static void _XwindowsSensorRollCcwCallback(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	if (value)
		aux->sensor6_delta.r[VR_ROLL] +=  KEY_ROTATE_DELTA;
	else	aux->sensor6_delta.r[VR_ROLL] -=  KEY_ROTATE_DELTA;

	return;
}

/************************************************************/
/* reset the keys-pressed count for keyboard 0 to 0. */
static void _XwindowsKeyboard0ValuatorZero(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;
	int			keyboard_num = 0;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	if (value) {
		aux->keyboard_count[keyboard_num] = 0;
		vrAssignValuatorValue((vrValuator *)(aux->keyboard_inputs[keyboard_num]), aux->keyboard_count[keyboard_num] * aux->keyboard_scale[keyboard_num]);
	}
}

/************************************************************/
/* toggle the sign for keyboard 0's valuator input. */
static void _XwindowsKeyboard0ValuatorSign(vrInputDevice *devinfo, int value)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;
	int			keyboard_num = 0;

	if (!aux->silent)
		_XwindowsLoudBellCallback(devinfo, 1);

	if (value) {
		aux->keyboard_scale[keyboard_num] *= -1.0;
		vrAssignValuatorValue((vrValuator *)(aux->keyboard_inputs[keyboard_num]), aux->keyboard_count[keyboard_num] * aux->keyboard_scale[keyboard_num]);
	}
}



	/**********************************************************/
	/*   Callbacks for interfacing with the Xwindows device.  */
	/*                                                        */


/**************************************************************************/
static void _XwindowsCreateFunction(vrInputDevice *devinfo)
{
	/*** List of possible inputs ***/
static	vrInputFunction	_XwindowsInputs[] = {
				/* standard Xwindows input types */
				{ "key", VRINPUT_2WAY, _XwindowsKeyInput },
				{ "button", VRINPUT_2WAY, _XwindowsMouseButtonInput },
				{ "pointer", VRINPUT_VALUATOR, _XwindowsPointerInput },
				{ "sim6", VRINPUT_6SENSOR, _XwindowsSim6Input },
				{ "sim", VRINPUT_6SENSOR, _XwindowsSim6Input },		/* TODO: delete -- deprecated */

				/* three ways to interpret a keyboard */
				{ "keyboard", VRINPUT_NWAY, _XwindowsKeyboardInput },
				{ "keyboard", VRINPUT_2WAY, _XwindowsKeyboardInput },
				{ "keyboard", VRINPUT_VALUATOR, _XwindowsKeyboardInput },

				/* if we get here, check whether extended devices match for buttons or valuators */
				{ "button", VRINPUT_2WAY, _XwindowsExtButtonInput },
				{ "valuator", VRINPUT_VALUATOR, _XwindowsExtValuatorInput },

				/* end of the list */
				{ NULL, VRINPUT_UNKNOWN, NULL } };

	/*** List of control functions ***/
static	vrControlFunc	_XwindowsControlList[] = {
				/* overall system controls */
				{ "system_pause_toggle", _XwindowsSystemPauseToggleCallback },

				/* informational output controls */
				{ "print_context", _XwindowsPrintContextStructCallback },
				{ "print_config", _XwindowsPrintConfigStructCallback },
				{ "print_input", _XwindowsPrintInputStructCallback },
				{ "print_struct", _XwindowsPrintStructCallback },
				{ "print_help", _XwindowsPrintHelpCallback },

				/* simulated 6-sensor selection controls */
				{ "sensor_next", _XwindowsSensorNextCallback },
				{ "setsensor", _XwindowsSensorSetCallback },	/* NOTE: this is non-boolean */
				{ "setsensor(0)", _XwindowsSensorSet0Callback },
				{ "setsensor(1)", _XwindowsSensorSet1Callback },
				{ "setsensor(2)", _XwindowsSensorSet2Callback },
				{ "setsensor(3)", _XwindowsSensorSet3Callback },
				{ "setsensor(4)", _XwindowsSensorSet4Callback },
				{ "setsensor(5)", _XwindowsSensorSet5Callback },
				{ "setsensor(6)", _XwindowsSensorSet6Callback },
				{ "setsensor(7)", _XwindowsSensorSet7Callback },
				{ "setsensor(8)", _XwindowsSensorSet8Callback },
				{ "setsensor(9)", _XwindowsSensorSet9Callback },
				{ "sensor_reset", _XwindowsSensorResetCallback },
				{ "sensor_resetall", _XwindowsSensorResetAllCallback },

				/* simulated 6-sensor manipulation controls */
				{ "pointer_valuator", _XwindowsPointerAsValuator },
				{ "pointer_xy_override", _XwindowsPointerAsXYCallback },
				{ "pointer_xz_override", _XwindowsPointerAsXZCallback },
				{ "pointer_rot_override", _XwindowsPointerAsRotCallback },
				{ "toggle_relative", _XwindowsToggleRelativeAxesCallback },
				{ "toggle_space_limit", _XwindowsToggleRestrictSpaceCallback },
				{ "toggle_return_to_zero", _XwindowsToggleReturnToZeroCallback },
				{ "toggle_silence", _XwindowsToggleSilenceCallback },
				{ "sensor_left", _XwindowsSensorLeftCallback },
				{ "sensor_right", _XwindowsSensorRightCallback },
				{ "sensor_up", _XwindowsSensorUpInCallback },
				{ "sensor_in", _XwindowsSensorInUpCallback },
				{ "sensor_down", _XwindowsSensorDownOutCallback },
				{ "sensor_out", _XwindowsSensorOutDownCallback },
				{ "sensor_pazim", _XwindowsSensorPAzimuthCallback },
				{ "sensor_nazim", _XwindowsSensorNAzimuthCallback },
				{ "sensor_pelev", _XwindowsSensorPElevationCallback },
				{ "sensor_nelev", _XwindowsSensorNElevationCallback },
				{ "sensor_cw", _XwindowsSensorRollCwCallback },
				{ "sensor_ccw", _XwindowsSensorRollCcwCallback },
				{ "sensor_swap_yz", _XwindowsSensorSwapYZCallback },
				{ "sensor_swap_upin", _XwindowsSensorSwapYZCallback },
				{ "sensor_rotate_sensor", _XwindowsSensorSwapRotTransCallback },

				/* These two use non-integer values for the callback, so don't work correctly yet */
				/* TODO: fix this, today (1/22/03) I figured out how to pass floats -- by reference (see joydev) */
				{ "sensor_leftright_val", _XwindowsSensorLeftRightValCallback },
				{ "sensor_updown_val", _XwindowsSensorUpDownValCallback },

				{ "keyval_zero(0)", _XwindowsKeyboard0ValuatorZero },
				{ "keyval_sign(0)", _XwindowsKeyboard0ValuatorSign },

				/* other controls */
				{ "bell", _XwindowsLoudBellCallback },
				{ "softbell", _XwindowsSoftBellCallback },

				/* end of the list */
				{ NULL, NULL } };

	_XwindowsPrivateInfo	*aux = NULL;

	/******************************************/
	/* allocate and initialize auxiliary data */
	devinfo->aux_data = (void *)vrShmemAlloc0(sizeof(_XwindowsPrivateInfo));
	aux = (_XwindowsPrivateInfo *)devinfo->aux_data;
	_XwindowsInitializeStruct(aux);

	/******************/
	/* handle options */
	_XwindowsParseArgs(aux, devinfo->args);

	/***************************************/
	/* create the inputs and self-controls */
	vrInputCreateDataContainers(devinfo, _XwindowsInputs);
	vrInputCreateSelfControlContainers(devinfo, _XwindowsInputs, _XwindowsControlList);
	/* TODO: anything to do for NON self-controls?  Implement for Magellan first. */

	/* set the active flag for the active sensor" */
	if (aux->sensor6_inputs[aux->active_sim6sensor] != NULL) {
		vrAssign6sensorActiveValue(((vr6sensor *)aux->sensor6_inputs[aux->active_sim6sensor]), 1);
	}

	vrDbgPrintf("Done creating Xwindows inputs for '%s'\n", devinfo->name);
	devinfo->created = 1;

	return;
}


/**************************************************************************/
static void _XwindowsOpenFunction(vrInputDevice *devinfo)
{
static	char			*whitespace = " \t\r\b";
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;
	vrGlxPrivateInfo	*glx_aux;
	vrConfigInfo		*vrConfig;
	XDeviceInfoPtr		device_list;
	int			num_devices;
	int			exdev_type_val[32];
	int			exdev_type_press[32];
	int			exdev_type_release[32];
	int			exdev_class_val[32];
	int			exdev_class_press[32];
	int			exdev_class_release[32];
	int			req_count;
	int			exist_count;
	int			input_count;
	char			*current_xdisplay_string;	/* store the current display string to determine whether desired display is already open */

	/* variables for looping through the "aux->window_list" list */
	char			*list_dup;			/* a duplicate copy of aux->window_list, so we can be destructive */
	char			*next;				/* pointer to the next part of the list to parse */
	char			*window_name;			/* the name of the current window in the list */
	char			*end_name;			/* pointer to the terminator of the name string */
	char			*inputwindow_args;		/* the arguments of an input-only window */
	char			*end_args;			/* pointer to the terminator of the args string */

	vrTrace("_XwindowsOpenFunction", devinfo->name);

	vrConfig = devinfo->context->config;

	/*******************/
	/* open the device */

	/* loop over all the windows in the list */
	list_dup = vrShmemStrDup(aux->window_list);

	vrDbgPrintfN(XWIN_DBGLVL, "_XwindowsOpenFunction(): window_list = '%s'\n", list_dup);
	window_name = list_dup;
	do {
		window_name += strspn(window_name, whitespace);	/* skip white */
		end_name = strchr(window_name, ',');
		inputwindow_args = strchr(window_name, '[');

		/* If a '[' appears before the end of this string, then this window is an        */
		/*    input-only window with the arguments contained between the square-brackets */

		/* find the end of the current window name string */
			if ((inputwindow_args != NULL) && ((inputwindow_args < end_name) || end_name == NULL)) {
				/* this window name contains a '[' ']' pair, so it is an input-only window */
				inputwindow_args[0] = '\0';	/* end the name of the window */
				inputwindow_args++;
				end_args = strchr(inputwindow_args, ']');
				if (end_args == NULL) {
					/* This shouldn't happen because there should be a matching ']' */
					/*   so we'll set end_name to the end of the overall string.    */
					end_args = strchr(window_name, '\0'-1);
					vrErrPrintf("_XwindowsOpenFunction(): " RED_TEXT "Warning: no closing ']' in input-only specification for window '%s'.\n" NORM_TEXT, window_name);
				}
				end_name = strchr(end_args, ',');	/* check for commas after the arguments */
				end_args[0] = '\0';			/* put an end to the argument string */
				vrDbgPrintfN(XWIN_DBGLVL, "_XwindowsOpenFunction(): window args = '%s'\n", inputwindow_args);
			} else {
				inputwindow_args = NULL;
			}
		if (end_name != NULL) {
			next = end_name+1;
			end_name[0] = '\0';
		}
		vrDbgPrintfN(XWIN_DBGLVL, "_XwindowsOpenFunction(): window = '%s'\n", window_name);

		/* If the window_name is an empty string, then move on */
		if (window_name[0] == '\0') {
			window_name = next;
			continue;
		}

		/* If there are input window arguments, then create a special input-only window.*/
		/*   NOTE: an empty string still indicates to create the special window, but    */
		/*   with the default arguments.                                                */
		if (inputwindow_args != NULL) {
			vrDbgPrintfN(XWIN_DBGLVL, "_XwindowsOpenFunction(): " RED_TEXT "creating new input-only Xwindow '%s'\n" NORM_TEXT, window_name);
			glx_aux = _XwindowsOpenInputWindow(aux->display, current_xdisplay_string, window_name, inputwindow_args);
			current_xdisplay_string = glx_aux->xdisplay_string;

			/* TODO: these don't make a lot of sense, now that we can have multiple windows for input */
			aux->display = glx_aux->xdisplay;
			aux->window[aux->window_count] = NULL;		/* There is no visual rendering window for this input */
			aux->found_req_window[aux->window_count] = 1;

		} else {
			/* Otherwise, find the existing output window and use it for input */
			if (!strcasecmp(window_name, "default"))
				vrDbgPrintfN(XWIN_DBGLVL, "_XwindowsOpenFunction(): " RED_TEXT "looking for default output window -- ie. the first on the list.\n" NORM_TEXT);
			else	vrDbgPrintfN(XWIN_DBGLVL, "_XwindowsOpenFunction(): " RED_TEXT "looking for output window '%s'\n" NORM_TEXT, window_name);
			aux->window[aux->window_count] = vrObjectArraySearch(VROBJECT_WINDOW, (vrObjectInfo **)(vrConfig->windows), vrConfig->num_windows, window_name);
			if (aux->window[aux->window_count] == NULL) {
				/* Otherwise, find the first display window with GLX properties. */
				/* TODO: this should really find the first window with GLX/Performer properties, but for now just getting the first window. */
				aux->window[aux->window_count] = vrConfig->windows[0];
				if (!strcasecmp(window_name, "default"))
					vrMsgPrintf("FreeVR: X11 input device %s defaulting to window '%s'.\n",
						devinfo->name, aux->window[aux->window_count]->name);
				else	vrErrPrintf("_XwindowsOpenFunction(): Warning: X11 input device %s could not find window '%s', using '%s' instead.\n",
						devinfo->name, window_name, aux->window[aux->window_count]->name);
			} else {
				aux->found_req_window[aux->window_count] = 1;	/* TODO: this makes less sense now that we can have multiple windows */
			}

			/* get a pointer to the window's auxiliary data */
			glx_aux = (vrGlxPrivateInfo *)aux->window[aux->window_count]->aux_data;
			if (glx_aux == NULL) {
				vrMsgPrintf("FreeVR: X11 input device %s waiting for window '%s' to be initialized.\n",
					devinfo->name, aux->window[aux->window_count]->name);
				while (glx_aux == NULL)
					glx_aux = (vrGlxPrivateInfo *)aux->window[aux->window_count]->aux_data;
			}

			if (!glx_aux->mapped)
				vrMsgPrintf("FreeVR: X11 input device %s waiting for window '%s' to be opened.\n",
					devinfo->name, aux->window[aux->window_count]->name);
			while (!glx_aux->mapped);

			vrDbgPrintfN(XWIN_DBGLVL, "_XwindowsOpenFunction(): " RED_TEXT "done waiting for mapped window -- '%p'\n" NORM_TEXT, glx_aux->xwindow);
			vrDbgPrintfN(XWIN_DBGLVL, "_XwindowsOpenFunction(): " RED_TEXT "window name is -- '%s'\n" NORM_TEXT, aux->window[aux->window_count]->name);

			if (glx_aux->used_by_input) {
				vrMsgPrintf("X11 input device %s notes that window '%s' is already being used by another input device.\n",
					devinfo->name, aux->window[aux->window_count]->name);
			} else {
				glx_aux->used_by_input = 1;
				vrDbgPrintfN(COMMON_DBGLVL, "FreeVR: X11 input device %s is the first to tap X resources from window '%s'.\n",
					devinfo->name, aux->window[aux->window_count]->name);
			}

			/*************************************************************/
			/* connect with the window from which input will be garnered */
			/*  NOTE: it seems as though aux->display gets the same value as glx_aux->xdisplay */
			/*    but the call to XOpenDisplay() is still required, or the process will crash  */
			/* Only open the window if it hasn't already been opened, or when */
			/*    it has previously been opened to another display.           */
			if (aux->display != NULL && strcmp(current_xdisplay_string, glx_aux->xdisplay_string) != 0) {
				vrErrPrintf("_XwindowsOpenFunction(): " RED_TEXT "Warning: X-display mismatch ('%s' vs. '%s'), previous display connections will be lost.\n" NORM_TEXT, current_xdisplay_string, glx_aux->xdisplay_string);
				aux->display = NULL;
			}
			if (aux->display == NULL) {
				aux->display = XOpenDisplay(glx_aux->xdisplay_string);
				current_xdisplay_string = glx_aux->xdisplay_string;
			}
			vrDbgPrintfN(XWIN_DBGLVL, "_XwindowsOpenFunction(): " RED_TEXT "my display address is %p vs %p: display_string = '%s'\n" NORM_TEXT, aux->display, glx_aux->xdisplay, glx_aux->xdisplay_string);

		}

		/***********************************************************************/
		/* setup the input -- both input-output and input-only windows do this */
		aux->xscreen_num[aux->window_count] = glx_aux->xscreen;
		aux->xwindow[aux->window_count] = glx_aux->xwindow;
		XSetErrorHandler(vrXwindowsErrorHandler);
		XSynchronize(aux->display, True);

		XSelectInput(aux->display, aux->xwindow[aux->window_count], aux->event_mask);
		vrDbgPrintfN(XWIN_DBGLVL, "_XwindowsOpenFunction(): " RED_TEXT "Selected input for event_mask of 0x%x for input-output window 0x%x on display 0x%x\n" NORM_TEXT, aux->event_mask, aux->xwindow[aux->window_count], aux->display);

		/************************************************************/
		/* mark the device as ready -- or the inputs won't be read! */
		vrDbgPrintfN(XWIN_DBGLVL, "_XwindowsOpenFunction(): Done opening Xwindow '%s' for input device '%s'\n", window_name, devinfo->name);
		devinfo->operating = 1;

		/*******************************************************/
		/* get ready to parse the next window name in the list */
		window_name = next;

		aux->display_array[aux->window_count] = aux->display;
		aux->window_count++;
	} while (end_name != NULL);


	/* TODO: check whether this is true -- NOTE: only need to handle the X-extension devices once, not for each window */

	/***********************************************/
	/* now handle the X-extension devices (if any) */
	if (aux->num_exdevices > 0) {
#if 0 /* this was used to determine that X-extension devices doen't seem to work with VRPN */
		vrMsgPrintf("X11 input: Sleeping, feel free to use 'dbx -p %d' now to find out where\n", getpid()); vrSleep(20000000);
#endif

		/* NOTE: calling the function XListInputDevices() seems to break the VRPN server as an active FreeVR device */
		device_list = (XDeviceInfoPtr)XListInputDevices(aux->display, &num_devices);
	}
	for (req_count = 0; req_count < aux->num_exdevices; req_count++) {
		int	device_found = 0;

		for (exist_count = 0; exist_count < num_devices; exist_count++) {
			if (!strcmp(aux->exdevice_names[req_count], device_list[exist_count].name)) {
				/* found a requested device that exists! */
				aux->exdevice[req_count] = XOpenDevice(aux->display, device_list[exist_count].id);
				aux->exdevice_id[req_count] = aux->exdevice[req_count]->device_id;
				device_found = 1;

				/* find inputs that use this device */
				for (input_count = 0; input_count < aux->num_extinputs; input_count++) {
					if (aux->extinput_edev[input_count] == req_count) {
						/* found an input using this device */

						switch (aux->extinput_type[input_count]) {

						case BUTTON_TYPE:
							DeviceButtonPress(aux->exdevice[req_count], exdev_type_press[req_count], exdev_class_press[req_count]);
							aux->exevents_list[aux->exevents_count++] = exdev_class_press[req_count];
							aux->extinput_event_press[input_count] = exdev_type_press[req_count];

							DeviceButtonRelease(aux->exdevice[req_count], exdev_type_release[req_count], exdev_class_release[req_count]);
							aux->exevents_list[aux->exevents_count++] = exdev_class_release[req_count];
							aux->extinput_event_release[input_count] = exdev_type_release[req_count];

							aux->extinput_event_val[input_count] = -1;

							break;

						case MOTION_TYPE:

							DeviceMotionNotify(aux->exdevice[req_count], exdev_type_val[req_count], exdev_class_val[req_count]);
							aux->exevents_list[aux->exevents_count++] = exdev_class_val[req_count];

							aux->extinput_event_val[input_count] = exdev_type_val[req_count];

							aux->extinput_event_press[input_count] = -1;
							aux->extinput_event_release[input_count] = -1;
							break;

						default:
							vrErrPrintf(RED_TEXT "_XwindowsOpenFunction(): Unknown event type (%d) for X-extension device\n" NORM_TEXT,
								aux->extinput_type[input_count]);
							break;
						}
					}
				}
			}
		}

		if (!device_found) {
			vrErrPrintf(RED_TEXT "_XwindowsOpenFunction(): Unable to find requested X-device '%s', some inputs won't work.\n" NORM_TEXT,
				aux->exdevice_names[req_count]);
		} else {
			vrDbgPrintfN(XWIN_DBGLVL, "_XwindowsOpenFunction(): X device '%s' has been opened, with all the inputs created.\n",
				aux->exdevice_names[req_count]);
		}
	}

#if defined(__hpux) /* HP-UX doesn't provide dlopen(), dlsym(), etc. */
	vrDbgPrintfN(XWIN_DBGLVL, "_XwindowsOpenFunction(): " RED_TEXT "about to XSelectExtensionEvent() .. count = %d\n" NORM_TEXT, aux->exevents_count);
#else
	vrDbgPrintfN(XWIN_DBGLVL, "_XwindowsOpenFunction(): " RED_TEXT "about to XSelectExtensionEvent() .. count = %d function = %#p\n" NORM_TEXT,
		aux->exevents_count, dlsym(NULL, "XSelectInput"));
#endif

	/* we only need to call XSelectExtensionEvent() if we have some */
	/*    additional events that need to be read.  The library -lXi */
	/*    must be included to use this function.                    */
	/* NOTE: extension devices usually aren't associated with windows, */
	/*   so it isn't clear why a window argument is required here.     */
	if (aux->exevents_count != 0)
		XSelectExtensionEvent(aux->display, aux->xwindow[aux->window_count], aux->exevents_list, aux->exevents_count);


#if 0 /* TODO: 06/13/2006 -- testing whether this is necessary, since repeating is disabled when an input window gets focus */
	/* turn off auto repeating */
	/* TODO: unforuntately, this works fine if one of our input windows ends up with focus */
	/*   at the end of the FreeVR startup.  However (and this is the unfortunate part), if */
	/*   that is not the case (ie. an non-input window has focus), then there is no need   */
	/*   to turn off key-repeating, and it won't actually get enabled until the mouse goes */
	/*   through one of the input windows.  So, a better solution is being sought.         */
	if (!aux->keyrepeat)
		XAutoRepeatOff(aux->display);
#endif

#if 0 /* 06/13/2006 */
	/* NOTE: the following line moves focus to whereever the mouse pointer is, rather than */
	/*   this (or other recently-opened-by-FreeVR) window, which has the side effect of    */
	/*   matching the keyrepeat to the proper status.  The downside is that for people who */
	/*   want the new window to automatically have focus -- this won't do that (and in fact*/
	/*   will specifically not do that despite any window manager settings).               */
	XSetInputFocus(aux->display, aux->xwindow[aux->window_count], RevertToPointerRoot, CurrentTime);
#endif

#if 1 /* 06/13/2006 */
	/* NOTE: this technique finds out which window currently has focus, and searches through the */
	/*   list of FreeVR input-Xwindows to see whether there is a match.  If so, then it disables */
	/*   keyrepeating.  This is probably the best solution -- though I personally like it when   */
	/*   the focus is always under where the mouse currently resides.                            */
	if (!aux->keyrepeat) {
		Window	window_with_focus;
		int	focus_value;
		int	count;

		XGetInputFocus(aux->display, &window_with_focus, &focus_value);
		vrDbgPrintf("_XwindowsOpenFunction(): Hey, the focus is currently at window %p, with value %d\n", window_with_focus, focus_value);
		for (count = 0; count < aux->window_count; count++) {
			if (window_with_focus == aux->xwindow[count]) {
				vrDbgPrintf("_XwindowsOpenFunction(): Input window '%s' has focus -- Disabling Keyrepeat\n",
					(_GetXWindowInfo(aux, window_with_focus) == NULL ? "input window" : _GetXWindowInfo(aux, window_with_focus)->name));
				XAutoRepeatOff(aux->display);
			}
		}
	}
#endif

	vrDbgPrintf("_XwindowsOpenFunction(): Done opening Xwindows for input device '%s'\n", devinfo->name);
	devinfo->operating = 1;

	return;
}


/**************************************************************************/
static void _XwindowsCloseFunction(vrInputDevice *devinfo)
{
	_XwindowsPrivateInfo	*aux = (_XwindowsPrivateInfo *)devinfo->aux_data;
	int	count;

	/* close all the extended devices that were opened */
	for (count = 0; count < aux->num_exdevices; count++) {
		if (aux->exdevice[count] != NULL)
			XCloseDevice(aux->display, aux->exdevice[count]);
	}

	/* reenable key repeating if it had been disabled */
	if (!aux->keyrepeat)
		XAutoRepeatOn(aux->display);

	return;
}


/**************************************************************************/
static void _XwindowsResetFunction(vrInputDevice *devinfo)
{
	/* TODO: device reset code */
	return;
}


/**************************************************************************/
static void _XwindowsPollFunction(vrInputDevice *devinfo)
{
	if (devinfo->operating) {
		_XwindowsGetData(devinfo);
	} else {
		/* TODO: try to open the device again */
	}

	return;
}



	/******************************/
	/*** FreeVR public routines ***/
	/******************************/


/**************************************************************************/
void vrXwindowsInitInfo(vrInputDevice *devinfo)
{
	devinfo->version = (char *)vrShmemStrDup("The Xwindows input device, version 0.3");
	devinfo->Create = vrCallbackCreateNamed("XwindowInput:Create-Def", _XwindowsCreateFunction, 1, devinfo);
	devinfo->Open = vrCallbackCreateNamed("XwindowInput:Open-Def", _XwindowsOpenFunction, 1, devinfo);
	devinfo->Close = vrCallbackCreateNamed("XwindowInput:Close-Def", _XwindowsCloseFunction, 1, devinfo);
	devinfo->Reset = vrCallbackCreateNamed("XwindowInput:Reset-Def", _XwindowsResetFunction, 1, devinfo);
	devinfo->PollData = vrCallbackCreateNamed("XwindowInput:PollData-Def", _XwindowsPollFunction, 1, devinfo);
	devinfo->PrintAux = vrCallbackCreateNamed("XwindowInput:PrintAux-Def", _XwindowsPrintStruct, 0);

	vrDbgPrintfN(XWIN_DBGLVL, "vrXwindowsInitInfo: callbacks created.\n");
}

