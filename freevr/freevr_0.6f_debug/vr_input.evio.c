/* ======================================================================
 *
 *  CCCCC          vr_input.evio.c
 * CC   CC         Author(s): Bill Sherman
 * CC              Created: November 23, 2004
 * CC   CC         Last Modified: September 1, 2013
 *  CCCCC
 *
 * Code file for FreeVR inputs from the Linux Event I/O (EVIO) input interface.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/*************************************************************************

USAGE:
	This device driver currently only works on Linux systems (because those
	are where the "event I/O" (aka EVIO, aka evdev) interface exist -- though
	it may also be available on FreeBSD and similar systems).


	The configuration of the inputs is similar to all input devices:

	Inputs are specified with the "input" option:
		input "<name>" = "2switch(key[<name>])";
		input "<name>" = "2switch(button[<name>])";
		input "<name>" = "2switch(code[<number>])";
		input "<name>" = "valuator(abs[<name>|<number>])";
		input "<name>" = "valuator(rel[<name>|<number>])";
		input "<name>" = "6sensor(sim6[<number>])";
	 :-?	input "<name>" = "valuator(6dof[{-,}{tx|ty|tz|rx|ry|rz}])";

	Controls are specified with the "control" option:
		control "<control option>" = "2switch(button|key|code[<name>|<number>])";
		control "<control option>" = "valuator(abs|rel[<name>|<number>])";

	Here are the available (2switch) control options for FreeVR:
		"print_help" -- print info on how to use the input device
		"system_pause_toggle" -- toggle the system pause flag
		"print_context" -- print the overall FreeVR context data structure (for debugging)
		"print_config" -- print the overall FreeVR config data structure (for debugging)
		"print_input" -- print the overall FreeVR input data structure (for debugging)
		"print_struct" -- print the internal EVIO data structure (for debugging)
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

	Here are the FreeVR configuration argument options for the EVIO:
		"devname" - /dev/input/event<num> port EVIO is connected to
			("/dev/input/event3" is the default)
		"restrict" - limit simulated 6-sensors to move within the working-volume
		"valuatoroverride" - produce valuator values instead of moving the 6-sensor
		"returntozero" - valuator offsets map to absolute or relative position of simulated 6-sensor
		"relativerot" - rotation of simulated 6-sensor is based on sensor's or world's coord-sys
	  	"swaptransrot" -- swap the translational and rotational inputs
		"workingvolume" - orthogonally defined volume specifying the valid range of simulated 6-sensor movement
		"transscale" - scaling factor used to tune the translational movement of a simulated 6-sensor
		"rotscale" - scaling factor used to tune the rotational movement of a simulated 6-sensor
		"valscale" - scaling factor used to tune the valuator input range


NOTES on the EVIO interface:

	Availability:
		As far as I can tell, it is available on Linux systems, not
		sure about BSD-based OSes (will have to check).  It is not
		available on Mac OSX.

	Devices:
		Different devices report specific input types, and for absolute
		axes, they further report about the range of inputs.  Of note,
		on an XBox controller the right and left "Z" inputs have lower
		resolution, but also go only from 0 to positive-max whereas the
		joysticks go from negative-max to positive-max.  In the "eviotest"
		program this all gets washed away as it will spread the range to
		map to [-1:1].  When it comes to FreeVR interfaces though, that
		suggests that there ought to be a configuration feature that
		allows one to just provide half of the domain (probably [0:1]).

		Also, on a Logitech Wireless RumblePad 2 that I am testing, the
		X-axis of the right joystick only goes through half of the range
		that it self reports -- therefore "eviotest" only gives values
		in the domain [0:1].

		My first opportunity to test relative valuator inputs is when
		on of my machines (running Fedora-11) automatically grabs a
		SpaceNavigator as part of the X11 input system, and toggles it
		into relative mode -- and uses it as a mouse!

		My second opportunity for relative valuator inputs is more
		consistent -- a Shuttle Pro video editor controller input device.


		Using third-party software it is possible to generate EVIO/evdev
		events from a Nintendo Wii Controller (aka Wiimote).
		Specifically, the cwiid library in conjunction with the
		"wminput" application will create a (configurable)
		/dev/input/event<N> EVIO/evdev device that can then be
		directly interfaced with this system.

		Many Linux distributions include a "cwiid" package as well as
		the "wminput" application as a package.

	XInput devices:
		NOTE: sometimes (as mentioned above), the X-11 input system will
		grab devices as they are hot-plugged, and will use them as part
		of the Xwindows system.  To disable an input device from
		providing Xwindows inputs, use the "xinput" command.  For
		example:
			% xinput list --short | grep Navigator
			"3Dconnexion SpaceNavigator"    id=7    [XExtensionPointer]
			% xinput list-props 7 | grep "Device Enabled"
				Device Enabled (93):    1

		now take the device id (7) and the enable property (93), and
		knowing the bit length to be 8, set the enable flag to "0" to
		disable:
			% xinput set-int-prop 7 93 8 0
			% xinput list-props 7 | grep "Device Enabled"
				Device Enabled (93):    0

		(and how do we know the bit length to be 8?  I'm not sure,
		other than by looking in the X-windows source code, or by
		trial and error -- X will return a "BadValue" error when
		using 16 or 32 for that particular property.)
			% xinput set-int-prop 7 93 16 0
			X Error of failed request:  BadValue (integer parameter out of range for operation)
			  Major opcode of failed request:  143 (XInputExtension)
			  Minor opcode of failed request:  37 ()
			  Value in failed request:  0x5d
			  Serial number of failed request:  14
			  Current serial number in output stream:  16

	O/S Interface:
		Modern Linux systems (ie. at least after 2.6.28, probably
		earlier), will automatically create a new EVIO (aka "event")
		input device when the hardware is connected to the system --
		typically by plugging in via USB connector.  Other means of
		connection are via RS-232 serial interfaces, along with
		device-specific driver software, plus bluetooth devices using
		the HID protocol (verify -- may also require additional drivers
		to be engaged).

		Once an interface has been established, a new file under the
		"/dev" directory will be created.  The name will be of the form:
			/dev/input/event<N>
		where <N> is a numeric value assigned based on the next
		available number.

	IOCTL interface:
		To access data about a particular input device, the IOCTL
		system calls are used.  On Linux systems, the file
		"/usr/include/linux/input.h" provides the needed software
		interface data.

		The possible event types available through the EVIO interface
		are:
			EV_SYN -- synchronization
			EV_KEY -- keyboard or button press
			EV_REL -- Relative axis movement (e.g. mouse)
			EV_ABS -- Absolute axis movement (e.g. game controller)
			EV_MSC -- Miscellaneous things (e.g. gestures)
			EV_LED -- LED related events

	Places on the web with documentation and information (not all of it current):
		- https://www.kernel.org/doc/Documentation/input/event-codes.txt
		- http://www.linuxjournal.com/article/6396 (part I)
		- http://www.linuxjournal.com/article/6429 (part II)

		- http://texas.funkfeuer.at/~harald/slu/ (Kernel input utilities)

		- http://www.einfochips.com/download/dash_jan_tip.pdf (putting new inputs into the system in user-space)

HISTORY:
	23 November 2004 (Bill Sherman) -- wrote initial version by adapting
		vr_input.joydev.c code with some methods adopted from
		vr_input.xwindows.c (specifically the notion that there
		is a potential input for each key, axis, etc.).

	16 October 2009 (Bill Sherman)
		A quick fix to the _EvioParseArgs() routine to handle the
		  no-arguments case.

	07 August 2013 (Bill Sherman)
		Fixed some minor bugs to now allow the "eviotest" program to
		  operate (which it does very similarly to the "evRead" program).

	10-12 August 2013 (Bill Sherman)
		Enhanced the "eviotest" side of things such that it works very
		close to the "joytest" program.  This brings us close to
		where we need to be to implement the FreeVR aspects of EVIO
		interfacing.  I also added a label to the "eviotest" output
		to make it easier to read the inputs.

	13-14 August 2013 (Bill Sherman)
		Added two command line arguments to the "eviotest" application:
			-list -- list all active /dev/input/event<N> devices
			-nodata -- just show information about the device, but
				don't print a stream of input values
			-repunk -- report unknown input event types

	14 August 2013 (Bill Sherman)
		I made a first pass at integrating button presses into FreeVR.
		This required implementing _EvioButtonInput(), and changing
		_EvioButtonValue() to return actual button values -- at this
		point only ones specified to numerically match the EVIO button
		value.

		I also enabled a bit of code in _EvioPrintStruct() to print
		out the button/key inputs that are active.  Plus I did some
		cleanup, including some spell checking.

	19-21 August 2013 (Bill Sherman)
		Okay, it's basically working now.  I've completed the button
		inputs, and then completed the valuator inputs -- well, I don't
		have anything for the relative inputs (relaxis) yet.  I didn't
		do anything for the simulated 6-sensor inputs (sim6), but I
		think they may just work with all the rest of the standard
		code -- but I haven't tested them yet.

		I then began doing some other cleanups (basically began working
		to make the EVIO code more closely match the Joydev code).

	26-28 August 2013 (Bill Sherman)
		I did more cleanup work and work to better match vr_input.joydev.c.

	1 September 2013 (Bill Sherman)
		As with the Joydev input device, I've added some features to
		enhance the self-control capabilities and usage.

	4 September 2013 (Bill Sherman)
		Fixed some bugs where string comparisons could potentially be
		done against NULL string pointers from key_names[], abs_names[]
		or rel_names[] arrays -- and in fact would happen when there
		was an unmatched string since KEY_MAX grew past what I was
		allocating names for!

		Also made it such that specifying relative axes valuator inputs
		now looks to the proper array of names.

		I added a hard-coded threshold check on valuator inputs.  It
		turns out that with 8 bit A-D inputs the lack of a zero-point
		stands out.  (Of course, in my tutorials I handle this for the
		joytstick inputs, but maybe it is just smarter to handle it in
		the library -- plus when using the values for the simulated
		6-sensor inputs, it's all within the library, so that's the

	14 September 2013 (Bill Sherman)
		I added the "eviotest" man page to the end of this document.

	15 September 2013 (Bill Sherman)
		Renaming "active_sensor" to "active_sim6sensor".

	5 December 2013 (Bill Sherman)
		Fixed a couple bugs where the "aux->key_inputs" field was
		exposed outside of defined(FREEVR) sections of code.  This
		happened because of a mal-formed #if expression and also
		because a vrDbgPrintfN() expression containing the above
		was outside a defined(FREEVR) section, but for some compilers
		the internals are ignored based on how I trick the compiler
		to ignoring the print statement when compiling the test
		application.

TODO:
	- DONE: Complete the FreeVR interface to EVIO devices.

	- DONE: Consider printing the self-controls as part of the _EvioPrintHelp() function
		(which currently does nothing of interest).

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

	- Consider moving the vrAssign...() calls to _EvioGetData() to
		match other input interfaces (from _EvioReadInput())
		(I'm still debating this, as per my journal entry from 08/xx/13,
		there is a valid reason to leave things as they are -- in the
		_EvioReadInput() function I already am aware of what inputs have
		changed and so don't have to pass this information along, and
		then loop through that information to make the assignments.)

	- DONE: add "swap_yz" to all sensor6_options usage

	- DONE: Write a man page for "eviotest"

	- Make sure error handling of controls & input type mismatches is
		reasonable -- eg. trying to use a valuator control with a
		button input & vice versa.

	- When a button goes to a valuator, allow sign change operation.
		(ie. allow the sign to be specified in the "control" config
		line.)

	- Allow multiple inputs to same control, so for example set_roll can
		be implemented by two opposing button presses.

	- Add a command-line option to control the style of valuator output
		in "eviotest".

	- investigate LED control

	- investigate force-feedback (FF) controls

	- make the output controls (LEDs, sounds, forces) work -- once the
		Linux kernel can handle it

	- Should check to see whether some other free Unixii have the
		evio interface.  (Mac OS/X (OSX) does not)

	- (perhaps) make use of incoming time data

**************************************************************************/
#if defined(__linux)	/* Can only be compiled on Linux systems { */
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
#  include <linux/input.h>	/* needed for Event I/O (EVIO/evdev) flags and masks */
	/* Some addendum types and defines that aren't including in many Linux distributions */

	/* NOTE: "input_devinfo" has been renamed "input_id" in linux/input.h */
	struct	input_devinfo {
		uint16_t	bustype;
		uint16_t	vendor;
		uint16_t	product;
		uint16_t	version;
	};

#ifdef EV_RST	/* EV_RST only seems to be defined in older versions of input.h */
	/* Older versions of linux/input.h are missing this definition */
	struct	input_absinfo {
		uint32_t	value;
		uint32_t	miniumum;
		uint32_t	maxiumum;
		uint32_t	fuzz;
		uint32_t	flat;
	};
#endif

#	define REL_RX	0x03
#	define REL_RY	0x04
#	define REL_RZ	0x05
	/* end of linux/input.h addendum */
#endif


#if defined(__CYGWIN__)
#	define KEY_MAX 0777
#	define REL_MAX 0x1F
#	define ABS_MAX 0x1F
#endif

/* The "test_bit" macro is for extracting active inputs from a bitmask. */
#define	test_bit(bit, array)	(array[bit/8] & (1<<(bit%8)))

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
#  define	vrShmemStrDup	strdup
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

#define	DEFAULT_DEVICE	"/dev/input/event3"	/* events 0 & 1 are usually mouse and keyboard */

/* TODO: determine whether we still want these */
/* sensitivity values */
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
		int		open;			/* flag that EVIO successfully open */
		char		name[256];		/* name reported by the device */

		/* these are for internal data parsing */
		char		version[256];		/* self-reported version of the device */

		int		report_unknown;		/* flag of whether to report unknown events */

#ifdef CAVE /* { */
		/* CAVE specific fields here */

#elif defined(FREEVR) /* } { */
		/* FREEVR specific fields here */

		vrGenericInput	*key_inputs[KEY_MAX+1];		/* allocate KEY_MAX+1 */
		vrGenericInput	*abs_inputs[ABS_MAX+1];		/* allocate ABS_MAX+1 */
		float		absaxis_sign[ABS_MAX+1];
		vrGenericInput	*rel_inputs[REL_MAX+1];		/* allocate REL_MAX+1 */
		float		relaxis_sign[REL_MAX+1];
#  define MAX_6SENSORS  16
		vr6sensor	*sensor6_inputs[MAX_6SENSORS];
		int		sim6sensor_change;	/* flag that indicates whether any of the controls to a simulated 6-sensor have changed */

		/* for the FreeVR simulated 6-sensors */
		int		active_sim6sensor;	/* The simulated 6-sensor that is being actively controlled */
		vr6sensorConv	sensor6_options;	/* Structure of settings that affect how a 6-sensor is simulated */
		float		sim_values[6];		/* Array of 6 valuator values used to move the simulated 6-sensor */
#endif /* } end library-specific fields */

		/* information about the current values */
		uint8_t		button_bitmask[KEY_MAX/8 + 1];	/* active buttons for this device */
		uint8_t		num_buttons;		/* number of buttons reported by the device */
		uint8_t		button[KEY_MAX];	/* array of button values -- length is KEY_MAX*/
		uint8_t		button_change[KEY_MAX];	/* array of flags to indicate new button value -- length is KEY_MAX */
#if 0 /* TODO: consider this -- I'm currently leaning against it, to just loop through them (in the FreeVR version, we'll already have a mapping, and for the test version I can just loop through them all */
		int		*button_map;		/* mapping of EVIO code to condensed list of possible buttons */
#endif

		uint8_t		absaxis_bitmask[ABS_MAX/8 + 1];	/* active absolute axes for this device */
		uint8_t		num_absaxes;		/* number of valuators (aka axes) reported by the device */
		float		absaxis[ABS_MAX];	/* array of axis values */
		uint8_t		absaxis_change[ABS_MAX];/* array of flags to indicate new valuator value */
		struct input_absinfo	*absaxis_info[ABS_MAX];	/* array of pointers to details for an absolute axis input */

		uint8_t		relaxis_bitmask[REL_MAX/8 + 1];	/* active relative axes for this device */
		uint8_t		num_relaxes;		/* number of valuators (aka axes) reported by the device */
		float		relaxis[REL_MAX];	/* array of axis values */
		uint8_t		relaxis_change[REL_MAX];/* array of flags to indicate new valuator value */

		/* information about how to filter the data for use with the VR library (FreeVR or CAVE) */
		float		scale_valuator;		/* scaling factor for valuators */
		/* TODO: determine whether any filter values/settings are necessary */

	} _EvioPrivateInfo;


/**************************************************************************/
/* arrays of strings naming all the specific inputs from the EVIO devices */
static	char	*key_names[KEY_MAX+1] = {
			"Reserved",
			"Escape",
			"1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
			"Minus", "Equal", "Backspace", "Tab",
			"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P",
			"LeftBrace", "RightBrace", "Enter", "LeftCtrl",
			"A", "S", "D", "F", "G", "H", "J", "K", "L",
			"Semicolon", "Apostrophe", "Grave", "LeftShift", "BackSlash",
			"Z", "X", "C", "V", "B", "N", "M",
			"Comma", "Dot", "Slash", "RightShift",
			"KPAsterisk", "LeftAlt", "Space", "CapsLock",
			"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
			"NumLock", "ScrollLock",
			"KP7", "KP8", "KP9", "KPMinus",
			"KP4", "KP5", "KP6", "KPPlus",
			"KP1", "KP2", "KP3", "KP0", "KPDot",
			"103RD", "F13", "102RD", "F11",
			"F12", "F14", "F15", "F16",
			"F17", "F18", "F19", "F20",
			"KPEnter", "RightCtl", "KPSlash", "SysRQ",
			"RightAlt", "Linefeed",
			"Home", "Up", "PageUp", "Left", "Right", "End",
			"Down", "PageDown", "Insert", "Delete",
			"Macro", "Mute", "VolumeDown", "VolumeUp", "Power",
			"KPEqual", "KPPlusMinus", "Pause",
			"F21", "F22", "F23", "F24",
			"KPComma", "LeftMeta", "RightMeta", "Compose",
			"Stop", "Again", "Props", "Undo", "Front",
			"Copy", "Open", "Paste", "Find", "Cut",
			"Help", "Menu", "Calc", "Setup", "Sleep",
			"Wakeup", "File", "Sendfile", "Deletefile",
			"Xfer", "Prog1", "Prog2", "WWW", "MsDOS",
			"Coffee", "Direction", "Cyclewindows",
			"Mail", "Bookmarks", "Computer", "Back", "Forward",
			"CloseCD", "EjectCD", "EjectCloseCD",
			"Nextsong", "PlayPause", "PreviousSong", "StopCD",
			"Record", "Rewind", "Phone", "ISO",
			"Config", "HomePage", "Refresh", "Exit",
			"Move", "Edit", "ScrollUp", "ScrollDown",
			"KPLeftParen", "KPRightParen",
			"Intl1", "Intl2", "Intl3", "Intl4",
			"Intl5", "Intl6", "Intl7", "Intl8", "Intl9",
			"Lang1", "Lang2", "Lang3", "Lang4", "Lang5",
			"Lang6", "Lang7", "Lang8", "Lang9", "0x0c7",
			"PlayCD", "PauseCD", "Prog3", "Prog4",
			"Suspend", "Close",
			"0x0ce", "0x0cf",
			"0x0d0", "0x0d1", "0x0d2", "0x0d3",
			"0x0d4", "0x0d5", "0x0d6", "0x0d7",
			"0x0d8", "0x0d9", "0x0da", "Unknown",
			"0x0dc", "0x0dd", "0x0de", "0x0df",
			"0x0e0", "0x0e1", "0x0e2", "0x0e3",
			"0x0e4", "0x0e5", "0x0e6", "0x0e7",
			"0x0e8", "0x0e9", "0x0ea", "0x0eb",
			"0x0ec", "0x0ed", "0x0ee", "0x0ef",
			"0x0f0", "0x0f1", "0x0f2", "0x0f3",
			"0x0f4", "0x0f5", "0x0f6", "0x0f7",
			"0x0f8", "0x0f9", "0x0fa", "0x0fb",
			"0x0fc", "0x0fd", "0x0fe", "0x0ff",
			"Btn:0",	/* also Btn:Misc */
			"Btn:1", "Btn:2", "Btn:3", "Btn:4",
			"Btn:5", "Btn:6", "Btn:7", "Btn:8", "Btn:9",
			"0x10a", "0x10b", "0x10c",
			"0x10d", "0x10e", "0x10f",
			"Btn:Left",	/* also Btn:Mouse */
			"Btn:Right", "Btn:Middle", "Btn:Side",
			"Btn:Extra", "Btn:Forward", "Btn:Back",
			"0x117", "0x118", "0x119", "0x11a",
			"0x11b", "0x11c", "0x11d", "0x11e", "0x11f",
			"Btn:Trigger",	/* also Btn:Joystick */
			"Btn:Thumb", "Btn:Thumb2",
			"Btn:Top", "Btn:Top2",
			"Btn:Pinkie",
			"Btn:Base", "Btn:Base2", "Btn:Base3",
			"Btn:Base4", "Btn:Base5", "Btn:Base6",
			"0x12c", "0x12d", "0x12e", "Btn:Dead",
			"Btn:A",	/* also Btn:Gamepad */
			"Btn:B", "Btn:C", "Btn:X", "Btn:Y", "Btn:Z",
			"Btn:TL", "Btn:TR", "Btn:TL2", "Btn:TR2",
			"Btn:Select", "Btn:Start", "Btn:Mode",
			"Btn:Thumbl", "Btn:Thumbr",		/* NOTE: on XBOX at least, these are better labled "Btn:JoyL" & "Bth:JoyR" (TODO: consider renaming) */
			"0x13f",
			"Btn:ToolPen",	/* also Btn:Digi */
			"Btn:ToolRubber", "Btn:ToolBrush", "Btn:ToolPencil",
			"Btn:ToolAirbrush", "Btn:ToolFinger", "Btn:ToolMouse",
			"Btn:ToolLens", "0x148", "0x149",
			"Btn:Touch", "Btn:Stylus", "Btn:Stylus2",
			"0x14d", "0x14e", "0x14f",
			"0x150", "0x151", "0x152", "0x153",
			"0x154", "0x155", "0x156", "0x157",
			"0x158", "0x159", "0x15a", "0x15b",
			"0x15c", "0x15d", "0x15e", "0x15f",
			"0x160", "0x161", "0x162", "0x163",
			"0x164", "0x165", "0x166", "0x167",
			"0x168", "0x169", "0x16a", "0x16b",
			"0x16c", "0x16d", "0x16e", "0x16f",
			"0x170", "0x171", "0x172", "0x173",
			"0x174", "0x175", "0x176", "0x177",
			"0x178", "0x179", "0x17a", "0x17b",
			"0x17c", "0x17d", "0x17e", "0x17f",
			"0x180", "0x181", "0x182", "0x183",
			"0x184", "0x185", "0x186", "0x187",
			"0x188", "0x189", "0x18a", "0x18b",
			"0x18c", "0x18d", "0x18e", "0x18f",
			"0x190", "0x191", "0x192", "0x193",
			"0x194", "0x195", "0x196", "0x197",
			"0x198", "0x199", "0x19a", "0x19b",
			"0x19c", "0x19d", "0x19e", "0x19f",
			"0x1a0", "0x1a1", "0x1a2", "0x1a3",
			"0x1a4", "0x1a5", "0x1a6", "0x1a7",
			"0x1a8", "0x1a9", "0x1aa", "0x1ab",
			"0x1ac", "0x1ad", "0x1ae", "0x1af",
			"0x1b0", "0x1b1", "0x1b2", "0x1b3",
			"0x1b4", "0x1b5", "0x1b6", "0x1b7",
			"0x1b8", "0x1b9", "0x1ba", "0x1bb",
			"0x1bc", "0x1bd", "0x1be", "0x1bf",
			"0x1c0", "0x1c1", "0x1c2", "0x1c3",
			"0x1c4", "0x1c5", "0x1c6", "0x1c7",
			"0x1c8", "0x1c9", "0x1ca", "0x1cb",
			"0x1cc", "0x1cd", "0x1ce", "0x1cf",
			"0x1d0", "0x1d1", "0x1d2", "0x1d3",
			"0x1d4", "0x1d5", "0x1d6", "0x1d7",
			"0x1d8", "0x1d9", "0x1da", "0x1db",
			"0x1dc", "0x1dd", "0x1de", "0x1df",
			"0x1e0", "0x1e1", "0x1e2", "0x1e3",
			"0x1e4", "0x1e5", "0x1e6", "0x1e7",
			"0x1e8", "0x1e9", "0x1ea", "0x1eb",
			"0x1ec", "0x1ed", "0x1ee", "0x1ef",
			"0x1f0", "0x1f1", "0x1f2", "0x1f3",
			"0x1f4", "0x1f5", "0x1f6", "0x1f7",
			"0x1f8", "0x1f9", "0x1fa", "0x1fb",
			"0x1fc", "0x1fd", "0x1fe",
			"0x1ff"			/* KEY_MAX (perhaps -- ie. at least was once) */
		};

static	char	*rel_names[REL_MAX+1] = {
			"X", "Y", "Z",
			"RX",			/* not specified in linux/input.h" */
			"RY",			/* not specified in linux/input.h" */
			"RZ",			/* not specified in linux/input.h" */
			"HWheel", "Dial", "Wheel", "Misc",
			"0x0a", "0x0b", "0x0c", "0x0d", "0x0e",
			"0x0f"			/* REL_MAX (perhaps -- ie. at least was once) */
		};

static	char	*abs_names[ABS_MAX+1] = {
			"X", "Y", "Z",
			"RX", "RY", "RZ",
			"Throttle", "Rudder", "Wheel", "Gas", "Brake",
			"0x0b", "0x0c", "0x0d", "0x0e", "0x0f",
			"Hat0X", "Hat0Y",
			"Hat1X", "Hat1Y",
			"Hat2X", "Hat2Y",
			"Hat3X", "Hat3Y",
			"Pressure", "Distance",
			"Tilt_X", "Tilt_Y", "Misc",
			"0x1d", "0x1e",
			"0x1f"			/* ABS_MAX (perhaps -- ie. at least was once) */
		};



	/*********************************************/
	/*** General NON public interface routines ***/
	/*********************************************/

/***************************************************/
char *_EvioEventTypeName(int type)
{
	switch (type) {
	case EV_SYN:
		return "EV_SYN";
	case EV_KEY:
		return "EV_KEY";
	case EV_REL:
		return "EV_REL";
	case EV_ABS:
		return "EV_ABS";
	case EV_MSC:
		return "EV_MSC";
	case EV_SW:
		return "EV_SW";
	case EV_LED:
		return "EV_LED";
	case EV_SND:
		return "EV_SND";
	case EV_REP:
		return "EV_REP";
	case EV_FF:
		return "EV_FF";
	case EV_PWR:
		return "EV_PWR";
	case EV_FF_STATUS:
		return "EV_FF_STATUS";
	}

	return "<Unknown EV Type>";
}


/******************************************************/
/* typename is used to specify a particular device among many that */
/*   share (more or less) the same protocol.  The typename is then */
/*   used to determine what specific features are available with   */
/*   this particular type of device.                               */
static void _EvioInitializeStruct(_EvioPrivateInfo *aux, char *typename)
{
	int	count;

	/* set input defaults */
	aux->version[0] = '\0';
	aux->name[0] = '\0';

	aux->report_unknown = 0;

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

	/***************************************/
	/* Fill holes in the input name arrays */
	/*   (Since the number of KEY/ABS/REL inputs can change based on the Linux */
	/*   distribution, we need to ensure that each element of the array has an */
	/*   actual string rather than a NULL pointer -- at least not without      */
	/*   modifying the code each place where the arrays are accessed and       */
	/*   assumed to have valid strings in each element.                        */
	/* NOTE: this isn't exactly initializing the private info structure, but   */
	/*   this does seem like the best place for this -- at first I had it in   */
	/*   _EvioInitializeDevice(), but in fact I need this correction made      */
	/*   prior to the creation of the inputs, etc, and that happens before the */
	/*   device is opened (and _EvioInitializeDevice() is called as part of    */
	/*   the opening of the device -- so it's too late!  The other viable      */
	/*   option is to just put this code right after or before the call to     */
	/*   _EvioInitializeStructure(), but then I'd have to put it in two places:*/
	/*   once for the FreeVR app, and once for the TEST_APP, so I just put it  */
	/*   here instead.                                                         */
	for (count = 0; count < KEY_MAX; count++) {
		if (key_names[count] == NULL) {
			key_names[count] = vrShmemStrDup("<unknown>");
		}
	}

	for (count = 0; count < ABS_MAX; count++) {
		if (abs_names[count] == NULL) {
			abs_names[count] = vrShmemStrDup("<unknown>");
		}
	}

	for (count = 0; count < REL_MAX; count++) {
		if (rel_names[count] == NULL) {
			rel_names[count] = vrShmemStrDup("<unknown>");
		}
	}
}


#ifdef FREEVR /* { */
/******************************************************/
/* ... */
/* NOTE: simulated 6-sensors are not part of the test application */
static void _EvioPrint6sensorOptions(FILE *file, _EvioPrivateInfo *aux, vrPrintStyle style)
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
		vrFprintf(file, BOLD_TEXT "EVIO - 6-sensor settings:" NORM_TEXT "\n");
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
static void _EvioPrintStruct(FILE *file, _EvioPrivateInfo *aux, vrPrintStyle style)
{
	char	*input_name;
	int	count;

	vrFprintf(file, "EVIO device internal structure (%#p):\n", aux);
	vrFprintf(file, "\r\tversion -- '%s'\n", aux->version);
	vrFprintf(file, "\r\tname -- '" BOLD_TEXT "%s" NORM_TEXT "'\n", aux->name);
	vrFprintf(file, "\r\tfd = %d\n\tdevfile = '%s'\n\topen = %d\n",
		aux->fd,
		aux->devfile,
		aux->open);

	/* print the stored values */
	vrFprintf(file, "\r\tnum_buttons = %d\n", aux->num_buttons);
	for (count = 0; count < KEY_MAX; count++)
		if (test_bit(count, aux->button_bitmask)) {
			vrFprintf(file, "\r\t\tbutton[%2d](%-10s) = %d  (changed? %d)\n", count, key_names[count], aux->button[count], aux->button_change[count]);
		}

	vrFprintf(file, "\r\tnum_absaxes = %d\n", aux->num_absaxes);
	for (count = 0; count < ABS_MAX; count++) {
		if (test_bit(count, aux->absaxis_bitmask)) {
			vrFprintf(file, "\r\t\tabsaxis[%2d](%-10s) = %+f  (changed? %d)", count, abs_names[count], aux->absaxis[count], aux->absaxis_change[count]);
			vrFprintf(file, " -- (min/max = [%d:%d], flatness = %d, fuzz = %d)\n", aux->absaxis_info[count]->minimum, aux->absaxis_info[count]->maximum, aux->absaxis_info[count]->flat, aux->absaxis_info[count]->fuzz);
		}
	}

	vrFprintf(file, "\r\tnum_relaxes = %d\n", aux->num_relaxes);
	for (count = 0; count < REL_MAX; count++) {
		if (test_bit(count, aux->relaxis_bitmask)) {
			vrFprintf(file, "\r\t\trelaxis[%2d](%-10s) = %+f  (changed? %d)\n", count, rel_names[count], aux->relaxis[count], aux->relaxis_change[count]);
		}
	}

#ifdef FREEVR /* { */
	/* print the FreeVR input objects */

	/* Only print the keyinputs that are in use -- because otherwise */
	/*   there are just too darn many of them.                       */

	/* button inputs */
	vrFprintf(file, "\r\tkey/button inputs:\n");
	for (count = 0; count < KEY_MAX+1; count++) {
		if (aux->key_inputs[count] != NULL) {
			vrFprintf(file, "\r\t\tkey/button input[%d] (%-7s) = %d:  %#p (%s -- %s:%s)\n",
				count,
				key_names[count],
				aux->button[count],
				aux->key_inputs[count],
				aux->key_inputs[count]->my_object->desc_str,
				vrInputTypeName(aux->key_inputs[count]->input_type),
				aux->key_inputs[count]->my_object->name);
		}
	}

	/* absolute valuator inputs */
	vrFprintf(file, "\r\tabsolute axis inputs:\n");
	for (count = 0; count < ABS_MAX+1; count++) {
		if (aux->abs_inputs[count] != NULL) {
			vrFprintf(file, "\r\t\tabsaxis input[%d] (%-7s) = %+6.3f:  %#p (%s -- %s:%s)\n",
				count,
				abs_names[count],
				aux->absaxis[count],
				aux->abs_inputs[count],
				aux->abs_inputs[count]->my_object->desc_str,
				vrInputTypeName(aux->abs_inputs[count]->input_type),
				aux->abs_inputs[count]->my_object->name);
		}
	}

	/* relative valuator inputs */
	vrFprintf(file, "\r\trelative axis inputs:\n");
	for (count = 0; count < REL_MAX+1; count++) {
		if (aux->rel_inputs[count] != NULL) {
			vrFprintf(file, "\r\t\trelaxis input[%d] (%-7s) = %f:  %#p (%s -- %s:%s)\n",
				count,
				rel_names[count],
				aux->relaxis[count],
				aux->rel_inputs[count],
				aux->rel_inputs[count]->my_object->desc_str,
				vrInputTypeName(aux->rel_inputs[count]->input_type),
				aux->rel_inputs[count]->my_object->name);
		}
	}

	/* 6-sensor inputs */
	vrFprintf(file, "\r\t6sensor inputs (active = %d):\n", aux->active_sim6sensor);
	for (count = 0; count < MAX_6SENSORS; count++) {
		if (aux->sensor6_inputs[count] != NULL) {
			vrFprintf(file, "\r\t\t6sensor_inputs[%d] (%-7s):  %#p (%s -- %s:%s)\n",
				count,
				"sim6",
				aux->sensor6_inputs[count],
				aux->sensor6_inputs[count]->my_object->desc_str,
				vrInputTypeName(aux->sensor6_inputs[count]->input_type),
				aux->sensor6_inputs[count]->my_object->name);
		}
	}

	/* simulated 6-sensor settings */
	vrFprintf(file, "\r");
	_EvioPrint6sensorOptions(file, aux, brief);

#endif /* } FREEVR */

}


/**************************************************************************/
static void _EvioPrintHelp(FILE *file, _EvioPrivateInfo *aux)
{
	int	count;		/* looping variable */

#if 0 || defined(TEST_APP)
	vrFprintf(file, BOLD_TEXT "Sorry, EVIO - print_help control not yet implemented." NORM_TEXT "\n");
#else
	vrFprintf(file, BOLD_TEXT "EVIO - inputs:" NORM_TEXT "\n");
	for (count = 0; count < KEY_MAX; count++) {
		if (aux->key_inputs[count] != NULL) {
			vrFprintf(file, "\t%s -- %s%s\n",
				aux->key_inputs[count]->my_object->desc_str,
				(aux->key_inputs[count]->input_type == VRINPUT_CONTROL ? "control:" : ""),
				aux->key_inputs[count]->my_object->name);
		}
	}
	for (count = 0; count < ABS_MAX; count++) {
		if (aux->abs_inputs[count] != NULL) {
			vrFprintf(file, "\t%s -- %s%s\n",
				aux->abs_inputs[count]->my_object->desc_str,
				(aux->abs_inputs[count]->input_type == VRINPUT_CONTROL ? "control:" : ""),
				aux->abs_inputs[count]->my_object->name);
		}
	}
	for (count = 0; count < REL_MAX; count++) {
		if (aux->rel_inputs[count] != NULL) {
			vrFprintf(file, "\t%s -- %s%s\n",
				aux->rel_inputs[count]->my_object->desc_str,
				(aux->rel_inputs[count]->input_type == VRINPUT_CONTROL ? "control:" : ""),
				aux->rel_inputs[count]->my_object->name);
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


/******************************************************/
/* Returns the number of inputs successfully read. */
static int _EvioReadInput(_EvioPrivateInfo *aux)
{
	char			inputs_read = 0;
#ifdef __linux /* { */
static	struct timeval		no_block = { 0 , 0 };
static	struct input_event	event;
	int			code;
#ifdef FREEVR
	float			valuator_value;
#endif

	if (aux->open == 0) {
		return (0);
	}

	/* set the file descriptor list for select() to watch the EVIO/event device */
	FD_ZERO(&aux->read_fds);
	FD_SET(aux->fd, &aux->read_fds);

	/* loop until there is no more data on the device */
	while (select(aux->fd + 1, &aux->read_fds, NULL, NULL, &no_block)) {

		/* read the data from the device */
		read(aux->fd, &event, sizeof(event));
		code = event.code;

#ifdef DEBUG
		if (!(event.type == EV_SYN && code == 0)) {
			/* don't print the sync events with code 0 */
			vrPrintf("Event: time = %d:%06d, type = %d (%s), code = %d, value = %d\n", event.time.tv_sec, event.time.tv_usec, event.type, _EvioEventTypeName(event.type), code, event.value);
		}
#endif

		/* parse the event */
		switch (event.type) {
#ifdef EV_RST /* not all versions of Linux have this case -- perhaps deprecated */
		case EV_RST:	/* Reset event?  currently nop */
			break;
#endif
#ifdef EV_SYN /* not all versions of Linux have this case -- perhaps new */
		case EV_SYN:	/* Sync event?  currently nop */
			break;
#endif

		case EV_KEY:	/* Key or Button event */
			/* TODO: get key/button name */
			inputs_read++;
#ifdef DEBUG
			vrPrintf("EV_KEY: code = %d (%s), KEY_MAX = %d\n", code, key_names[code], KEY_MAX);
#endif
			aux->button[code] = event.value;		/* I may only need this for the TEST_APP */
			aux->button_change[code] = 1;
#ifdef FREEVR
			vrDbgPrintfN(INPUT_DBGLVL, "_EvioReadInput: handling a key/button event code of 0x%02x ('%s') (value = %d) (input pointer = %#p)\n", code, key_names[code], aux->button[code], aux->key_inputs[code]);
			if (aux->key_inputs[code] != NULL) {
				/* Specifically handle mapped keys & buttons */
				switch (aux->key_inputs[code]->input_type) {
				case VRINPUT_BINARY:
					vrDbgPrintfN(INPUT_DBGLVL, "_EvioGetData: Assigning a value of %d to key 0x%x\n", event.value, code);
					vrAssign2switchValue((vr2switch *)(aux->key_inputs[code]), event.value /*, vrCurrentWallTime() */);
					break;
				case VRINPUT_VALUATOR:
					/* TODO: ... */
					/* For keys associated with valuator inputs, pressing the */
					/*   key can add some fixed, or specified amount.  Fixed  */
					/*   is easier, so we'll start with a value like 0.2.     */
					break;

				case VRINPUT_CONTROL:
					vrDbgPrintfN(INPUT_DBGLVL, "_EvioGetData: Activating a callback (%#p) with %d to EVIO key 0x%x\n",
						((vrControl *)(aux->key_inputs[code]))->callback,
						event.value, code);
					vrCallbackInvokeDynamic(((vrControl *)(aux->key_inputs[code]))->callback, 1, event.value);
					break;
				}

			} else {
				/* allow handling of "keyboard" input types here */
				/* TODO: see vr_input.xwindows.c for how to implement this */
			}
#endif
			break;

		case EV_REL:	/* Relative Axis event */
			inputs_read++;
			aux->relaxis[code] = event.value;		/* I may only need this for the TEST_APP */
#ifdef DEBUG
			vrPrintf("EV_REL: code = %d (%s), REL_MAX = %d\n", code, rel_names[code], REL_MAX);
#endif
#ifdef FREEVR
			vrDbgPrintfN(INPUT_DBGLVL, "_EvioReadInput: handling a relative-axis event code of 0x%02x ('%s') (input pointer = %#p)\n", code, rel_names[code], aux->rel_inputs[code]);
#endif
			break;

		case EV_ABS:	/* Absolute Axis event */
			inputs_read++;
#if 0
			/* This version was for the XBox controller, but was too specific */
			if (code == 2 || code == 5)
				aux->absaxis[code] = event.value / 255.0;	/* xbox valuator triggers -- TODO: make use of calibration data */
			else if (code == 16 || code == 17)
				aux->absaxis[code] = event.value;		/* xbox valuator china-hat -- TODO: make use of calibration data */
			else	aux->absaxis[code] = event.value / 32767.0;
#else
			/* NOTE: this hard-codes for the [-1.0, 1.0] range of values -- we need a setting for this instead */
			aux->absaxis[code] = 2.0 * ((float)event.value - (float)aux->absaxis_info[code]->minimum) / ((float)aux->absaxis_info[code]->maximum - (float)aux->absaxis_info[code]->minimum) - 1.0;

			/* NOTE: for now I'm hard-coding a threshold (assuming 8-bit precision), but TODO: this needs to be configurable */
			/* NOTE: I'm not using fabs() here to avoid having to include the math library.  Plus, this is probably less cycles. */
			if (aux->absaxis[code] > -0.01 && aux->absaxis[code] < 0.01) {
				aux->absaxis[code] = 0.0;
			}
#endif
			aux->absaxis_change[code] = 1;
#ifdef DEBUG
			vrPrintf("EV_ABS: code = %d (%s), ABS_MAX = %d\n", code, abs_names[code], ABS_MAX);
#endif
#ifdef FREEVR
			vrDbgPrintfN(INPUT_DBGLVL, "_EvioReadInput: handling an absolute-axis event code of 0x%02x ('%s') (value = %.3f) (input pointer = %#p)\n", code, abs_names[code], aux->absaxis[code], aux->abs_inputs[code]);
			if (aux->abs_inputs[code] != NULL) {
				valuator_value = aux->absaxis[code] * aux->scale_valuator * aux->absaxis_sign[code];

				/* Specifically handle mapped keys & buttons */
				switch (aux->abs_inputs[code]->input_type) {
				case VRINPUT_BINARY:
					/* TODO: I don't know if this bit of code works -- not tested */
					vrDbgPrintfN(INPUT_DBGLVL, "_EvioGetData: Binary input for valuator value??\n", event.value, code);
					vrAssign2switchValue((vr2switch *)(aux->abs_inputs[code]), event.value /*, vrCurrentWallTime() */);
					break;
				case VRINPUT_VALUATOR:
					vrAssignValuatorValue((vrValuator *)(aux->abs_inputs[code]), valuator_value);
					break;

				case VRINPUT_CONTROL:
					vrDbgPrintfN(INPUT_DBGLVL, "_EvioGetData: Activating a callback (%#p) with %d to EVIO key 0x%x\n",
						((vrControl *)(aux->abs_inputs[code]))->callback,
						event.value, code);
					vrCallbackInvokeDynamic(((vrControl *)(aux->abs_inputs[code]))->callback, 1, &valuator_value);
					break;
				}

			} else {
				/* allow handling of "keyboard" input types here */
				/* TODO: see vr_input.xwindows.c for how to implement this */
			}
#endif
			break;

		case EV_MSC:	/* Miscellaneous event */
			if (code == MSC_SCAN) {
				/* don't do anything -- the Logitech Cordless RumblePad 2 reports these, ignore */
			} else {
				if (aux->report_unknown) {
					vrPrintf("\n_EvioReadInput: unknown event of type %d(%s) -- code = %d, value = %d\n", event.type, _EvioEventTypeName(event.type), code, event.value);
				}
			}
			break;

		default:	/* all other reported event types */
			/* TODO: print a warning -- I don't think there should be other reported types, and if there are I'd like to know about them. */
			vrDbgPrintfN(COMMON_DBGLVL, "_EvioReadInput: WARNING: unanticipated event of type 0x%02x (%s) (code=%d) -- please report to library maintainer.\n", event.type, _EvioEventTypeName(event.type), code);
			if (aux->report_unknown) {
				vrPrintf("\n_EvioReadInput: unknown event of type %d(%s) -- code = %d, value = %d\n", event.type, _EvioEventTypeName(event.type), code, event.value);
			}
			break;
		}
	}
#endif /* } (__linux) */

	return (inputs_read);
}


/**********************************************************/
/* _EvioInitializeDevice() is called in the OPEN phase  */
/*   of input interface -- after the types of inputs have */
/*   been determined (during the CREATE phase).           */
static int _EvioInitializeDevice(_EvioPrivateInfo *aux)
{
#ifdef __linux /* { */
	int		count;
	uint32_t	ev_version;
#endif /* } */

	if (aux == NULL)
		return -1;

#ifdef __linux /* { */
	/**************************************/
	/* read name and parameters of device */
	ioctl(aux->fd, EVIOCGVERSION, &ev_version);
	ioctl(aux->fd, EVIOCGNAME(255), &aux->name);

	/* NOTE: EVIOCGPHYS & EVIOCGUNIQ return strings similar to those listed in */
	/*   /dev/input/by-path, though interestingly, the string doesn't exactly  */
	/*   match any of the devices there.                                       */

	sprintf(aux->version, "Linux EVIO/evdev Driver version %u.%u.%u", 
		((ev_version & 0xff0000) >> 16), ((ev_version & 0x00ff00) >> 8), ((ev_version & 0x0000ff) >> 1));
	vrDbgPrintfN(INPUT_DBGLVL, "Initializing %s, name = '%s', aux->fd = %d\n", aux->version, aux->name, aux->fd);

	/******************************/
	/* get the button/key bitmask */
	aux->num_buttons = 0;
	memset(aux->button_bitmask, 0, sizeof(aux->button_bitmask));
	/* NOTE: ioctl(EVIOCGBIT...) does not return a number of matches -- has to be calculated */
	if (ioctl(aux->fd, EVIOCGBIT(EV_KEY, sizeof(aux->button_bitmask)), aux->button_bitmask) < 0) {
		perror("_EvioInitializeDevice(): getting keys & buttons bitmask");
	}

	for (count = 0; count < KEY_MAX; count++) {
		if (test_bit(count, aux->button_bitmask)) {
			aux->num_buttons++;
			aux->button_change[count] = 0;
		}
	}

	/*************************************/
	/* get the absolute valuator bitmask */
	aux->num_absaxes = 0;
	memset(aux->absaxis_bitmask, 0, sizeof(aux->absaxis_bitmask));
	/* NOTE: ioctl(EVIOCGBIT...) does not return a number of matches -- has to be calculated */
	if (ioctl(aux->fd, EVIOCGBIT(EV_ABS, sizeof(aux->absaxis_bitmask)), aux->absaxis_bitmask) < 0) {
		perror("_EvioInitializeDevice(): getting absolute axis bitmask");
	}

	for (count = 0; count < ABS_MAX; count++) {
		if (test_bit(count, aux->absaxis_bitmask)) {
			aux->num_absaxes++;
			aux->absaxis_info[count] = vrShmemAlloc0(sizeof(*aux->absaxis_info[count]));
			if (ioctl(aux->fd, EVIOCGABS(count), aux->absaxis_info[count]))
				perror("_EvioInitializeDevice(): getting absolute axis details");
			aux->absaxis_change[count] = 0;
		}
	}

	/*************************************/
	/* get the relative valuator bitmask */
	aux->num_relaxes = 0;
	memset(aux->relaxis_bitmask, 0, sizeof(aux->relaxis_bitmask));
	/* NOTE: ioctl(EVIOCGBIT...) does not return a number of matches -- has to be calculated */
	if (ioctl(aux->fd, EVIOCGBIT(EV_REL, sizeof(aux->relaxis_bitmask)), aux->relaxis_bitmask) < 0) {
		perror("_EvioInitializeDevice(): getting relative axis bitmask");
	}

	for (count = 0; count < REL_MAX; count++) {
		if (test_bit(count, aux->relaxis_bitmask)) {
			aux->num_relaxes++;
			aux->relaxis_change[count] = 0;
		}
	}
#else /* } __linux { */
	sprintf(aux->version, "Non-Linux system: No Linux EVIO Driver version available");
	sprintf(aux->name, "Non-Linux system: Unable to obtain device name");
	aux->num_buttons = 0;
	aux->num_absaxes = 0;
	aux->num_relaxes = 0;
#endif /* } !__linux */

	vrDbgPrintfN(INPUT_DBGLVL, "Initialized EVIO Driver with %d buttons, %d absolute axes and %d relative axes\n", aux->num_buttons, aux->num_absaxes, aux->num_relaxes);

	/* read some initial data */
	_EvioReadInput(aux);

	return 0;
}


/******************************************************/
/* Translate a string name of a button (the "instance" config) into a numeric value */
static unsigned int _EvioButtonValue(char *buttonname)
{
	int	count;

	/* look for a matching string */
	for (count = 0; count < KEY_MAX; count++) {
#if 0
		/* NOTE: the value of KEY_MAX can vary, and might be higher than the key_names array pre-sets, so check for a valid pointer first  */
		if (key_names[count] != NULL && !strcasecmp(buttonname, key_names[count])) {
			return count;
		}
#else /* a different (and more encompassing) solution was to just fill in all the array elements with strings */
		if (!strcasecmp(buttonname, key_names[count])) {
			return count;
		}
#endif
	}

	/* if we didn't get a match with the key_names[] array, then try some alternatives */
	switch (buttonname[0]) {
	case '0':
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
	}

	return -1;
}


/******************************************************/
/* Translate a string name of an absolute axis valuator (the "instance" config) into a numeric value */
static unsigned int _EvioAbsaxisValue(char *valuatorname)
{
	int	count;

	/* skip an opening sign character (is handled in _EvioAbsValuatorInput) */
	if (valuatorname[0] == '-' || valuatorname[0] == '+')
		valuatorname++;

	/* look for a matching string */
	for (count = 0; (count < ABS_MAX) && (abs_names[count] != NULL); count++) {
#if 0
		/* NOTE: the value of KEY_MAX can vary, and might be higher than the key_names array pre-sets, so check for a valid pointer first  */
		if (abs_names[count] != NULL && !strcasecmp(valuatorname, abs_names[count])) {
			return count;
		}
#else /* a different (and more encompassing) solution was to just fill in all the array elements with strings */
		if (!strcasecmp(valuatorname, abs_names[count])) {
			return count;
		}
#endif
	}

	/* if we didn't get a match with the abs_names[] array, then try some alternatives */
	switch (valuatorname[0]) {
	case '0':
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
	}

	return -1;
}


/******************************************************/
/* Translate a string name of an relative axis valuator (the "instance" config) into a numeric value */
static unsigned int _EvioRelaxisValue(char *valuatorname)
{
	int	count;

	/* skip an opening sign character (is handled in _EvioRelValuatorInput) */
	if (valuatorname[0] == '-' || valuatorname[0] == '+')
		valuatorname++;

	/* look for a matching string */
	for (count = 0; (count < REL_MAX) && (rel_names[count] != NULL); count++) {
#if 0
		/* NOTE: the value of KEY_MAX can vary, and might be higher than the key_names array pre-sets, so check for a valid pointer first  */
		if (rel_names[count] != NULL && !strcasecmp(valuatorname, rel_names[count])) {
			return count;
		}
#else /* a different (and more encompassing) solution was to just fill in all the array elements with strings */
		if (!strcasecmp(valuatorname, rel_names[count])) {
			return count;
		}
#endif
	}

	/* if we didn't get a match with the rel_names[] array, then try some alternatives */
	switch (valuatorname[0]) {
	case '0':
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
	}

	return -1;
}


	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/


#if defined(FREEVR) /* { */
/*****************************************************************/
/*** Functions for FreeVR access of EVIO devices for input ***/
/*****************************************************************/


	/************************************/
	/***  FreeVR NON public routines  ***/
	/************************************/


/*********************************************************/
static void _EvioParseArgs(_EvioPrivateInfo *aux, char *args)
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
static void _EvioGetData(vrInputDevice *devinfo)
{
	_EvioPrivateInfo	*aux = (_EvioPrivateInfo *)devinfo->aux_data;
	int			num_inputs;
	int			count;
	float			valuator_value;
#if 0 /* used for the simulated 6-sensor */
	float			values[6];
#endif

	/*******************/
	/* gather the data */
	num_inputs = _EvioReadInput(aux);

#if 0 /* now will be handled in _EvioReadInput() -- which is different than most of the other FreeVR interfaces [see notes on 08/xx/13] */
	if (num_inputs > 0) {

		/*************/
		/** buttons **/
		/* handle button inputs as buttons or self-controls */
		for (count = 0; count < MAX_BUTTONS; count++) {
			if ((aux->button_change[count] != 0)) {
				if (aux->key_inputs[count] != NULL) {
					switch (aux->key_inputs[count]->input_type) {
					case VRINPUT_BINARY:
						vrAssign2switchValue((vr2switch *)(aux->key_inputs[count]), ((aux->button[count]) != 0));
						break;
					case VRINPUT_CONTROL:
						vrCallbackInvokeDynamic(((vrControl *)(aux->key_inputs[count]))->callback, 1, ((aux->button[count]) != 0));
						break;
					default:
						vrErrPrintf(RED_TEXT "_EvioGetData: Unable to handle button inputs that aren't Binary or Control inputs\n" NORM_TEXT);
						break;
					}

				}
				aux->button_change[count] = 0;
			}
		}

		/***************/
		/** valuators **/
		/* handle valuator inputs as valuators or self-controls */
		for (count = 0; count < MAX_VALUATORS; count++) {
			if ((aux->absaxis_change[count] != 0)) {
				if (aux->valuator_inputs[count] != NULL) {
					valuator_value = aux->absaxis[count] * aux->scale_valuator * aux->valuator_sign[count];
					switch (aux->valuator_inputs[count]->input_type) {
					case VRINPUT_VALUATOR:
						vrAssignValuatorValue((vrValuator *)(aux->valuator_inputs[count]), valuator_value);
						break;
					case VRINPUT_CONTROL:
						vrCallbackInvokeDynamic(((vrControl *)(aux->valuator_inputs[count]))->callback, 1, &valuator_value);
						break;
					default:
						vrErrPrintf(RED_TEXT "_EvioGetData: Unable to handle valuator inputs that aren't Floating or Control inputs\n" NORM_TEXT);
						break;
					}
				}
				aux->absaxis_change[count] = 0;
			}
		}
	}
#endif

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
	/*    Function(s) for parsing EVIO "input" declarations.      */
	/*                                                              */
	/*  These _Evio<type>Input() functions are called during the  */
	/*  CREATE phase of the input interface.                        */

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

/**************************************************************************/
/* NOTE: There are three anticipated values for the DTI->type:            */
/*   * "key" -> keyboard key, etc. -- literal match to key_names[]        */
/*   * "button" -> game buttons, etc.  -- prepend "Btn:" to string match  */
/*   * "code" -> anything -- use the number as direct index to key_names[]*/
static vrInputMatch _EvioButtonInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
	_EvioPrivateInfo	*aux = (_EvioPrivateInfo *)devinfo->aux_data;
	int			button_num = -1;
	char			button_name[128] = "\0";

	/* get a numeric value for the given instance */
	if (!strcmp(dti->type, "code"))
		button_num = vrAtoI(dti->instance);
	else if (!strcmp(dti->type, "key"))
		button_num = _EvioButtonValue(dti->instance);
	else if (!strcmp(dti->type, "button")) {
		snprintf(button_name, sizeof(button_name), "Btn:%s", dti->instance);
		button_num = _EvioButtonValue(button_name);
	}

	/* check the selected choice */
	if (button_num == -1) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_EvioButtonInput(): " RED_TEXT "Warning, key['%s'] did not match any known key or button\n" NORM_TEXT, dti->instance);
		return VRINPUT_MATCH_UNABLE;	/* device match, but still unable to handle request */
	} else if (aux->key_inputs[button_num] != NULL)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_EvioButtonInput(): " RED_TEXT "Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);

	/* make the mapping -- assign the input argument to the inputs array */
#if 0 /* I'm debatting which way to go here [08/21/13] */
	aux->key_inputs[button_num] = (vr2switch *)input;
#else
	aux->key_inputs[button_num] = (vrGenericInput *)input;
#endif
	vrDbgPrintfN(INPUT_DBGLVL, "_EvioButtonInput(): assigned button event of value 0x%02x to input pointer = %#p)\n",
		button_num, aux->key_inputs[button_num]);

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/**************************************************************************/
static vrInputMatch _EvioAbsValuatorInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
	_EvioPrivateInfo	*aux = (_EvioPrivateInfo *)devinfo->aux_data;
	int			valuator_num;

	/* select a valuator (and sign) */
	valuator_num = _EvioAbsaxisValue(dti->instance);
	if (dti->instance[0] == '-')
		aux->absaxis_sign[valuator_num] = -1.0;
	else	aux->absaxis_sign[valuator_num] =  1.0;

	/* check the selected valuator */
	if (valuator_num == -1) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_EvioAbsValuatorInput(): " RED_TEXT "Warning, valuator['%s'] did not match any known valuator\n" NORM_TEXT, dti->instance);
		return VRINPUT_MATCH_UNABLE;	/* device match, but still unable to handle request */
	} else if (aux->abs_inputs[valuator_num] != NULL) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_EvioAbsValuatorInput(): " RED_TEXT "Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);
	}

	/* make the mapping */
#if 0 /* I'm debatting which way to go here [08/21/13] */
	aux->abs_inputs[valuator_num] = (vrValuator *)input;
#else
	aux->abs_inputs[valuator_num] = (vrGenericInput *)input;
#endif
	vrDbgPrintfN(INPUT_DBGLVL, "_EvioAbsValuatorInput(): assigned valuator event of value 0x%02x to input pointer = %#p)\n",
		valuator_num, aux->abs_inputs[valuator_num]);

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/**************************************************************************/
static vrInputMatch _EvioRelValuatorInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
	_EvioPrivateInfo	*aux = (_EvioPrivateInfo *)devinfo->aux_data;
	int			valuator_num;

	/* select a valuator (and sign) */
	valuator_num = _EvioRelaxisValue(dti->instance);
	if (dti->instance[0] == '-')
		aux->relaxis_sign[valuator_num] = -1.0;
	else	aux->relaxis_sign[valuator_num] =  1.0;

	/* check the selected valuator */
	if (valuator_num == -1) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_EvioRelValuatorInput(): " RED_TEXT "Warning, valuator['%s'] did not match any known valuator\n" NORM_TEXT, dti->instance);
		return VRINPUT_MATCH_UNABLE;	/* device match, but still unable to handle request */
	} else if (aux->rel_inputs[valuator_num] != NULL) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_EvioRelValuatorInput(): " RED_TEXT "Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);
	}

	/* make the mapping */
#if 0 /* I'm debatting which way to go here [08/21/13] */
	aux->rel_inputs[valuator_num] = (vrValuator *)input;
#else
	aux->rel_inputs[valuator_num] = (vrGenericInput *)input;
#endif
	vrDbgPrintfN(INPUT_DBGLVL, "_EvioRelValuatorInput(): assigned valuator event of value 0x%02x to input pointer = %#p)\n",
		valuator_num, aux->rel_inputs[valuator_num]);

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/**************************************************************************/
static vrInputMatch _Evio6sensorInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
	_EvioPrivateInfo	*aux = (_EvioPrivateInfo *)devinfo->aux_data;
	int			sensor_num;

	/* select a sensor */
	sensor_num = vrAtoI(dti->instance);

	/* check the selected sensor */
	if (sensor_num < 0) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_Evio6sensorInput(): " RED_TEXT "Warning, sensor number must be between %d and %d\n" NORM_TEXT, 0, MAX_6SENSORS);
		return VRINPUT_MATCH_UNABLE;	/* device match, but still unable to handle request */
	} else if (aux->sensor6_inputs[sensor_num] != NULL)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_Evio6sensorInput(): " RED_TEXT "Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);

	/* make the mapping */
	aux->sensor6_inputs[sensor_num] = (vr6sensor *)input;

	/* the vrAssign6sensorR2Exform() function does the parsing of the next token */
	vrAssign6sensorR2Exform(aux->sensor6_inputs[sensor_num], strchr(dti->instance, ','));

	vrDbgPrintfN(INPUT_DBGLVL, "_Evio6sensorInput(): assigned 6sensor event of value 0x%02x to input pointer = %#p)\n",
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
static void _EvioSystemPauseToggleCallback(vrInputDevice *devinfo, int value)
{
	if (value == 0)
		return;

	devinfo->context->paused ^= 1;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "EVIO Control: system_pause = %d.\n",
		devinfo->context->paused);
}

/************************************************************/
static void _EvioPrintContextStructCallback(vrInputDevice *devinfo, int value)
{
	if (value == 0)
		return;

	vrFprintContext(stdout, devinfo->context, verbose);
}

/************************************************************/
static void _EvioPrintConfigStructCallback(vrInputDevice *devinfo, int value)
{
	if (value == 0)
		return;

	vrFprintConfig(stdout, devinfo->context->config, verbose);
}

/************************************************************/
static void _EvioPrintInputStructCallback(vrInputDevice *devinfo, int value)
{
	if (value == 0)
		return;

	vrFprintInput(stdout, devinfo->context->input, verbose);
}

/************************************************************/
static void _EvioPrintStructCallback(vrInputDevice *devinfo, int value)
{
	_EvioPrivateInfo  *aux = (_EvioPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	_EvioPrintStruct(stdout, aux, verbose);
}

/************************************************************/
static void _EvioPrint6sensorOptionsCallback(vrInputDevice *devinfo, int value)
{
	_EvioPrivateInfo  *aux = (_EvioPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	_EvioPrint6sensorOptions(stdout, aux, verbose);
}

/************************************************************/
static void _EvioPrintHelpCallback(vrInputDevice *devinfo, int value)
{
	_EvioPrivateInfo  *aux = (_EvioPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	_EvioPrintHelp(stdout, aux);
}

/************************************************************/
static void _EvioSensorNextCallback(vrInputDevice *devinfo, int value)
{
	_EvioPrivateInfo	*aux = (_EvioPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	if (devinfo->num_6sensors == 0) {
		vrDbgPrintfN(SELFCTRL_DBGLVL, "EVIO Control: next sensor -- no sensors available.\n",
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

	vrDbgPrintfN(SELFCTRL_DBGLVL, "EVIO Control: 6sensor[%d] now active.\n", aux->active_sim6sensor);
}

/* TODO: see if there is a way to call this as an N-switch */
/************************************************************/
static void _EvioSensorSetCallback(vrInputDevice *devinfo, int value)
{
	_EvioPrivateInfo	*aux = (_EvioPrivateInfo *)devinfo->aux_data;

	if (value == aux->active_sim6sensor)
		return;

	if (value < 0 || value >= MAX_6SENSORS) {
		vrDbgPrintfN(SELFCTRL_DBGLVL, "EVIO Control: set sensor (%d) -- out of range.\n", value);
	}

	if (aux->sensor6_inputs[value] == NULL) {
		vrDbgPrintfN(SELFCTRL_DBGLVL, "EVIO Control: set sensor (%d) -- no such sensor available.\n", value);
		return;
	}

	vrAssign6sensorActiveValue(((vr6sensor *)aux->sensor6_inputs[aux->active_sim6sensor]), 0);
	aux->active_sim6sensor = value;
	vrAssign6sensorActiveValue(((vr6sensor *)aux->sensor6_inputs[aux->active_sim6sensor]), 1);

	vrDbgPrintfN(SELFCTRL_DBGLVL, "EVIO Control: 6sensor[%d] now active.\n", aux->active_sim6sensor);
}

/************************************************************/
static void _EvioSensorSet0Callback(vrInputDevice *devinfo, int value)
{	_EvioSensorSetCallback(devinfo, 0); }

/************************************************************/
static void _EvioSensorSet1Callback(vrInputDevice *devinfo, int value)
{	_EvioSensorSetCallback(devinfo, 1); }

/************************************************************/
static void _EvioSensorSet2Callback(vrInputDevice *devinfo, int value)
{	_EvioSensorSetCallback(devinfo, 2); }

/************************************************************/
static void _EvioSensorSet3Callback(vrInputDevice *devinfo, int value)
{	_EvioSensorSetCallback(devinfo, 3); }

/************************************************************/
static void _EvioSensorSet4Callback(vrInputDevice *devinfo, int value)
{	_EvioSensorSetCallback(devinfo, 4); }

/************************************************************/
static void _EvioSensorSet5Callback(vrInputDevice *devinfo, int value)
{	_EvioSensorSetCallback(devinfo, 5); }

/************************************************************/
static void _EvioSensorSet6Callback(vrInputDevice *devinfo, int value)
{	_EvioSensorSetCallback(devinfo, 6); }

/************************************************************/
static void _EvioSensorSet7Callback(vrInputDevice *devinfo, int value)
{	_EvioSensorSetCallback(devinfo, 7); }

/************************************************************/
static void _EvioSensorSet8Callback(vrInputDevice *devinfo, int value)
{	_EvioSensorSetCallback(devinfo, 8); }

/************************************************************/
static void _EvioSensorSet9Callback(vrInputDevice *devinfo, int value)
{	_EvioSensorSetCallback(devinfo, 9); }

/************************************************************/
static void _EvioSensorResetCallback(vrInputDevice *devinfo, int value)
{
static	vrMatrix		idmat = { { 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0 } };
	_EvioPrivateInfo	*aux = (_EvioPrivateInfo *)devinfo->aux_data;
	vr6sensor		*sensor;

	if (value == 0)
		return;

	sensor = &(devinfo->sensor6[aux->active_sim6sensor]);
	vrAssign6sensorValue(sensor, &idmat, 0 /* , timestamp */);
	vrAssign6sensorActiveValue(sensor, -1);
	vrAssign6sensorErrorValue(sensor, 0);
	vrDbgPrintfN(SELFCTRL_DBGLVL, "EVIO Control: reset 6sensor[%d].\n", aux->active_sim6sensor);
}

/************************************************************/
static void _EvioSensorResetAllCallback(vrInputDevice *devinfo, int value)
{
static	vrMatrix		idmat = { { 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0 } };
	_EvioPrivateInfo	*aux = (_EvioPrivateInfo *)devinfo->aux_data;
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
			vrDbgPrintfN(SELFCTRL_DBGLVL, "EVIO Control: reset 6sensor[%d].\n", count);
		}
	}
}

/************************************************************/
static void _EvioTempValuatorOverrideCallback(vrInputDevice *devinfo, int value)
{
	_EvioPrivateInfo	*aux = (_EvioPrivateInfo *)devinfo->aux_data;

	/* set the field to the current state of value (ie. 1 when depressed, 0 when released) */
	aux->sensor6_options.ignore_trans = value;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "EVIO Control: ignore_trans = %d.\n",
		aux->sensor6_options.ignore_trans);
}

/************************************************************/
static void _EvioToggleValuatorOverrideCallback(vrInputDevice *devinfo, int value)
{
	_EvioPrivateInfo	*aux = (_EvioPrivateInfo *)devinfo->aux_data;

	/* take no action when the button is released */
	if (value == 0)
		return;

	/* toggle the field (NOTE: we only get here when button is depressed) */
	aux->sensor6_options.ignore_trans ^= 1;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "EVIO Control: ignore_trans = %d.\n",
		aux->sensor6_options.ignore_trans);
}

/************************************************************/
static void _EvioTempValuatorOnlyCallback(vrInputDevice *devinfo, int value)
{
	_EvioPrivateInfo	*aux = (_EvioPrivateInfo *)devinfo->aux_data;

	/* set the field to the current state of value (ie. 1 when depressed, 0 when released) */
	aux->sensor6_options.ignore_all = value;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "EVIO Control: ignore_all = %d.\n",
		aux->sensor6_options.ignore_all);
}

/************************************************************/
static void _EvioToggleValuatorOnlyCallback(vrInputDevice *devinfo, int value)
{
	_EvioPrivateInfo	*aux = (_EvioPrivateInfo *)devinfo->aux_data;

	/* take no action when the button is released */
	if (value == 0)
		return;

	/* toggle the field (NOTE: we only get here when button is depressed) */
	aux->sensor6_options.ignore_all ^= 1;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "EVIO Control: ignore_all = %d.\n",
		aux->sensor6_options.ignore_all);
}

/************************************************************/
static void _EvioToggleRelativeAxesCallback(vrInputDevice *devinfo, int value)
{
	_EvioPrivateInfo	*aux = (_EvioPrivateInfo *)devinfo->aux_data;

	/* take no action when the button is released */
	if (value == 0)
		return;

	/* toggle the field (NOTE: we only get here when button is depressed) */
	aux->sensor6_options.relative_axis ^= 1;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "EVIO Control: relative_axis = %d.\n",
		aux->sensor6_options.relative_axis);
}

/************************************************************/
/* TODO: this should probably also go through all the 6sensor's  */
/*   and move them to be within the allowed workspace when space */
/*   restriction is turned on.                                   */
static void _EvioToggleRestrictSpaceCallback(vrInputDevice *devinfo, int value)
{
	_EvioPrivateInfo	*aux = (_EvioPrivateInfo *)devinfo->aux_data;

	/* take no action when the button is released */
	if (value == 0)
		return;

	/* toggle the field (NOTE: we only get here when button is depressed) */
	aux->sensor6_options.restrict_space ^= 1;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "EVIO Control: restrict_space = %d.\n",
		aux->sensor6_options.restrict_space);
}

/************************************************************/
static void _EvioToggleReturnToZeroCallback(vrInputDevice *devinfo, int value)
{
	_EvioPrivateInfo	*aux = (_EvioPrivateInfo *)devinfo->aux_data;

	/* take no action when the button is released */
	if (value == 0)
		return;

	/* toggle the field (NOTE: we only get here when button is depressed) */
	aux->sensor6_options.return_to_zero ^= 1;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "EVIO Control: return_to_zero = %d.\n",
		aux->sensor6_options.return_to_zero);
}

/************************************************************/
static void _EvioToggleSwapTransRotCallback(vrInputDevice *devinfo, int value)
{
	_EvioPrivateInfo	*aux = (_EvioPrivateInfo *)devinfo->aux_data;

	/* take no action when the button is released */
	if (value == 0)
		return;

	/* toggle the field (NOTE: we only get here when button is depressed) */
	aux->sensor6_options.swap_transrot ^= 1;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "EVIO Control: swap_transrot = %d.\n",
		aux->sensor6_options.swap_transrot);
}

/************************************************************/
static void _EvioToggleSwapYZCallback(vrInputDevice *devinfo, int value)
{
	_EvioPrivateInfo	*aux = (_EvioPrivateInfo *)devinfo->aux_data;

	/* take no action when the button is released */
	if (value == 0)
		return;

	/* toggle the field (NOTE: we only get here when button is depressed) */
	aux->sensor6_options.swap_yz ^= 1;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "EVIO Control: swap_yz = %d.\n",
		aux->sensor6_options.swap_yz);
}

/************************************************************/
static void _EvioSetTransXCallback(vrInputDevice *devinfo, float *value)
{
	_EvioPrivateInfo	*aux = (_EvioPrivateInfo *)devinfo->aux_data;

	aux->sim_values[VR_X] = *value;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "EVIO Control: set trans_x = %f.\n", *value);
}

/************************************************************/
static void _EvioSetTransYCallback(vrInputDevice *devinfo, float *value)
{
	_EvioPrivateInfo	*aux = (_EvioPrivateInfo *)devinfo->aux_data;

	aux->sim_values[VR_Y] = *value;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "EVIO Control: set trans_y = %f.\n", *value);
}

/************************************************************/
static void _EvioSetTransZCallback(vrInputDevice *devinfo, float *value)
{
	_EvioPrivateInfo	*aux = (_EvioPrivateInfo *)devinfo->aux_data;

	aux->sim_values[VR_Z] = *value;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "EVIO Control: set trans_z = %f.\n", *value);
}

/************************************************************/
static void _EvioSetAzimuthCallback(vrInputDevice *devinfo, float *value)
{
	_EvioPrivateInfo	*aux = (_EvioPrivateInfo *)devinfo->aux_data;

	aux->sim_values[VR_AZIM+3] = *value;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "EVIO Control: set azimuth = %f.\n", *value);
}

/************************************************************/
static void _EvioSetElevationCallback(vrInputDevice *devinfo, float *value)
{
	_EvioPrivateInfo	*aux = (_EvioPrivateInfo *)devinfo->aux_data;

	aux->sim_values[VR_ELEV+3] = *value;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "EVIO Control: set elevation = %f.\n", *value);
}

/************************************************************/
static void _EvioSetRollCallback(vrInputDevice *devinfo, float *value)
{
	_EvioPrivateInfo	*aux = (_EvioPrivateInfo *)devinfo->aux_data;

	aux->sim_values[VR_ROLL+3] = *value;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "EVIO Control: set roll = %f.\n", *value);
}



	/*************************************************/
	/*   Callbacks for interfacing with the device.  */
	/*                                               */


/************************************************************/
static void _EvioCreateFunction(vrInputDevice *devinfo)
{
	/*** List of possible inputs ***/
static	vrInputFunction	_EvioInputs[] = {
				/* tuple: "DTI.type-name", input-type, match-function */
				{ "button", VRINPUT_2WAY, _EvioButtonInput },		/* prepends "Btn:" to match name     */
				{ "key", VRINPUT_2WAY, _EvioButtonInput },		/* matches exactly as in key_names[] */
				{ "code", VRINPUT_2WAY, _EvioButtonInput },		/* uses the number as a direct index */
				{ "abs", VRINPUT_VALUATOR, _EvioAbsValuatorInput },
				{ "absaxis", VRINPUT_VALUATOR, _EvioAbsValuatorInput },
				{ "rel", VRINPUT_VALUATOR, _EvioRelValuatorInput },
				{ "relaxis", VRINPUT_VALUATOR, _EvioRelValuatorInput },
				{ "sim6", VRINPUT_6SENSOR, _Evio6sensorInput },
				{ NULL, VRINPUT_UNKNOWN, NULL } };
	/*** List of control functions ***/
static	vrControlFunc	_EvioControlList[] = {
				/* overall system controls */
				{ "system_pause_toggle", _EvioSystemPauseToggleCallback },

				/* informational output controls */
				{ "print_context", _EvioPrintContextStructCallback },
				{ "print_config", _EvioPrintConfigStructCallback },
				{ "print_input", _EvioPrintInputStructCallback },
				{ "print_struct", _EvioPrintStructCallback },
				{ "print_sim6opts", _EvioPrint6sensorOptionsCallback },
				{ "print_help", _EvioPrintHelpCallback },

				/* simulated 6-sensor selection controls */
				{ "sensor_next", _EvioSensorNextCallback },
				{ "setsensor", _EvioSensorSetCallback },	/* NOTE: this is non-boolean */
				{ "setsensor(0)", _EvioSensorSet0Callback },
				{ "setsensor(1)", _EvioSensorSet1Callback },
				{ "setsensor(2)", _EvioSensorSet2Callback },
				{ "setsensor(3)", _EvioSensorSet3Callback },
				{ "setsensor(4)", _EvioSensorSet4Callback },
				{ "setsensor(5)", _EvioSensorSet5Callback },
				{ "setsensor(6)", _EvioSensorSet6Callback },
				{ "setsensor(7)", _EvioSensorSet7Callback },
				{ "setsensor(8)", _EvioSensorSet8Callback },
				{ "setsensor(9)", _EvioSensorSet9Callback },
				{ "sensor_reset", _EvioSensorResetCallback },
				{ "sensor_resetall", _EvioSensorResetAllCallback },

				/* simulated 6-sensor manipulation controls */
				{ "temp_valuator", _EvioTempValuatorOverrideCallback },
				{ "toggle_valuator", _EvioToggleValuatorOverrideCallback },
				{ "temp_valuator_only", _EvioTempValuatorOnlyCallback },
				{ "toggle_relative", _EvioToggleRelativeAxesCallback },
				{ "toggle_space_limit", _EvioToggleRestrictSpaceCallback },
				{ "toggle_return_to_zero", _EvioToggleReturnToZeroCallback },
				{ "toggle_swap_transrot", _EvioToggleSwapTransRotCallback },
				{ "toggle_swap_yz", _EvioToggleSwapYZCallback },
				{ "set_transx", _EvioSetTransXCallback },
				{ "set_transy", _EvioSetTransYCallback },
				{ "set_transz", _EvioSetTransZCallback },
				{ "set_azim", _EvioSetAzimuthCallback },
				{ "set_elev", _EvioSetElevationCallback },
				{ "set_roll", _EvioSetRollCallback },

				/* other controls */
				/*   NONE   */

				/* end of the list */
				{ NULL, NULL } };

	_EvioPrivateInfo	*aux = NULL;

	/******************************************/
	/* allocate and initialize auxiliary data */
	devinfo->aux_data = (void *)vrShmemAlloc0(sizeof(_EvioPrivateInfo));
	aux = (_EvioPrivateInfo *)devinfo->aux_data;
	_EvioInitializeStruct(aux, devinfo->type);

	/******************/
	/* handle options */
	aux->devfile = vrShmemStrDup(DEFAULT_DEVICE);	/* default, if no device file given */
	_EvioParseArgs(aux, devinfo->args);

	/***************************************/
	/* create the inputs and self-controls */
	vrInputCreateDataContainers(devinfo, _EvioInputs);
	vrInputCreateSelfControlContainers(devinfo, _EvioInputs, _EvioControlList);
	/* TODO: anything to do for NON self-controls?  Implement for Magellan first. */

	vrDbgPrintf("_EvioCreateFunction(): Done creating EVIO inputs for '%s'\n", devinfo->name);
	devinfo->created = 1;

	return;
}


/************************************************************/
static void _EvioOpenFunction(vrInputDevice *devinfo)
{
	_EvioPrivateInfo	*aux = NULL;

	vrTrace("_EvioOpenFunction", devinfo->name);

	aux = (_EvioPrivateInfo *)devinfo->aux_data;

	/*******************/
	/* open the device */
#ifdef __linux /* { */
	aux->fd = open(aux->devfile, O_RDONLY);
	vrDbgPrintfN(INPUT_DBGLVL, "_EvioOpenFunction(): just opened aux->fd = %d, devfile = '%s'\n", aux->fd, aux->devfile);
	if (aux->fd < 0) {
		aux->open = 0;
		vrErrPrintf("_EvioOpenFunction(): EVIO device '%s' error: " RED_TEXT "couldn't open EVIO device '%s'\n" NORM_TEXT,
			devinfo->name, aux->devfile);
		sprintf(aux->version, "- unconnected EVIO -");
	} else {
		aux->open = 1;
		if (_EvioInitializeDevice(aux) < 0) {
			vrErrPrintf("_EvioOpenFunction: "
				RED_TEXT "Warning, unable to initialize EVIO '%s'.\n" NORM_TEXT, devinfo->name);
		} else {
			devinfo->operating = 1;
			vrDbgPrintf("_EvioOpenFunction(): Done opening EVIO input device '%s'.\n", devinfo->name);
		}
	}
#else /* } { ! __linux */
	vrErrPrintf("_EvioOpenFunction(): "
		RED_TEXT "Warning, unable to use EVIO devices on non-Linux systems. ('%s' inputs exist, but without hardware).\n" NORM_TEXT, devinfo->name);
	aux->open = 0;
	devinfo->operating = 0;
#endif /* } */

	return;
}


/************************************************************/
static void _EvioCloseFunction(vrInputDevice *devinfo)
{
	_EvioPrivateInfo	*aux = NULL;

	aux = (_EvioPrivateInfo *)devinfo->aux_data;

	if (aux != NULL) {
		vrSerialClose(aux->fd);
		vrShmemFree(aux);	/* aka devinfo->aux_data */
	}

	return;
}


/************************************************************/
static void _EvioResetFunction(vrInputDevice *devinfo)
{
#if 0 /* not yet used */
	_EvioPrivateInfo	*aux = NULL;

	aux = (_EvioPrivateInfo *)devinfo->aux_data;
#endif

	return;
}


/************************************************************/
static void _EvioPollFunction(vrInputDevice *devinfo)
{
#if 0 /* not yet used */
	_EvioPrivateInfo	*aux = NULL;

	aux = (_EvioPrivateInfo *)devinfo->aux_data;
#endif

	if (devinfo->operating) {
		_EvioGetData(devinfo);
	} else {
		/* TODO: try to open the device again */
	}

	return;
}



	/******************************/
	/*** FreeVR public routines ***/
	/******************************/


/******************************************************/
void vrEvioInitInfo(vrInputDevice *devinfo)
{
	devinfo->version = (char *)vrShmemStrDup("-get from Evio device-");
	devinfo->Create = vrCallbackCreateNamed("EvioInput:Create-Def", _EvioCreateFunction, 1, devinfo);
	devinfo->Open = vrCallbackCreateNamed("EvioInput:Open-Def", _EvioOpenFunction, 1, devinfo);
	devinfo->Close = vrCallbackCreateNamed("EvioInput:Close-Def", _EvioCloseFunction, 1, devinfo);
	devinfo->Reset = vrCallbackCreateNamed("EvioInput:Reset-Def", _EvioResetFunction, 1, devinfo);
	devinfo->PollData = vrCallbackCreateNamed("EvioInput:PollData-Def", _EvioPollFunction, 1, devinfo);
	devinfo->PrintAux = vrCallbackCreateNamed("EvioInput:PrintAux-Def", _EvioPrintStruct, 0);

	vrDbgPrintfN(INPUT_DBGLVL, "vrEvioInitInfo: callbacks created.\n");
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


/********************************************************************************/
char *BUSname(unsigned int bustype)
{
	switch (bustype) {
	case BUS_PCI:
		return("PCI bus");
	case BUS_ISAPNP:
		return("Plug-n-pray ISA bus");
	case BUS_USB:
		return("Universal Serial Bus");
	case BUS_ISA:
		return("legacy ISA bus");
	case BUS_I8042:
		return("I8042 (or similar) controller");
	case BUS_XTKBD:
		return("IBM XT bus");
	case BUS_RS232:
		return("RS232 serial bus");
	case BUS_GAMEPORT:
		return("gameport");
	case BUS_PARPORT:
		return("parallel port");
	case BUS_AMIGA:
		return("Amiga unique interface");
	case BUS_ADB:
		return("Apple Desktop Bus");
	case BUS_I2C:
		return("inter-integrated circuit bus");
	default:
		return("unknown bus type");
	}
}



/******************************************************************************/
/* report_nofile -- boolean flag to indicate whether an argument points to a  */
/*   non-existent file should be reported -- ie. for looping through specific */
/*   file names we want it, but looping over a range, we don't.               */
int _EVIOinfo(char *device_filename, int report_nofile)
{
	int		fd = -1;
	char		name[256]= "Unknown";
	int		returnval;		/* a value returned by a system call */
	int		toreturn = 0;		/* the value this function will return */
struct	input_id	device_info;		/* input_id defined in "linux/input.h" */

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
		if ((returnval = ioctl(fd, EVIOCGNAME(sizeof(name)), name)) < 0) {
#if 0
			perror("evdev ioctl");
			printf("ioctl return value = %d, errno = %d\n", returnval, errno);
#endif
			/* errno = 22 -> Invalid Argument (EINVAL)  -- from wrong type of device (e.g. joydev device) */
			/* errno = 25 -> Not an EVIO device (ENOTTY) -- or really not an input device */
			switch (errno) {
			case ENOTTY:
				strcpy(name, "is not an input device");
				break;
			case EINVAL:
				strcpy(name, "is not an EVIO input device");
				break;
			}
			toreturn = errno;
		} else {
			/* If we're here then we've successfully opened an EVIO device */

			/* request device information */
			if(ioctl(fd, EVIOCGID, &device_info)) {
				perror("evdev ioctl");
			}
#if 0
			printf("vendor 0x%04hx product 0x%04hx version 0x%04hx is on", device_info.vendor, device_info.product, devic_info.version);
#endif
		}
	}

	if (errno == 0) {
		printf("The device on %s says is a '%s' (%04x:%04x) on bus: %s\n", device_filename, name, device_info.vendor, device_info.product, BUSname(device_info.bustype));
	} else if (errno != ENOENT || report_nofile == 1) {
		printf(RED_TEXT "The device on %s %s\n" NORM_TEXT, device_filename, name);
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
/* A test program to communicate with a Evio device and print the results. */
main(int argc, char *argv[])
{
	_EvioPrivateInfo	*aux;
	char			*progname;		/* name of the program executable */
	int			count;
	int			nodata_flag = 0;	/* whether or not to actual show input data */
	int			button1 = -1;		/* the first actual button in the system (used for chorded-quit) */
	int			button2 = -1;		/* the second actual button in the system (used for chorded-quit) */
	int			button3 = -1;		/* the third actual button in the system (used for chorded-quit) */

	done = 0;
	signal(SIGINT, exit_testapp);


	/******************************/
	/* setup the device structure */
	aux = (_EvioPrivateInfo *)malloc(sizeof(_EvioPrivateInfo));
	memset(aux, 0, sizeof(_EvioPrivateInfo));
	_EvioInitializeStruct(aux, "Eviosubtype");


	/*********************************************************/
	/* set default parameters based on environment variables */
	aux->devfile = getenv("EVIO_DEVICE");
	if (aux->devfile == NULL)
		aux->devfile = DEFAULT_DEVICE;		/* default, if no file given */


	/*****************************************************/
	/* adjust parameters based on command-line-arguments */
	progname = argv[0];
	while ((argc > 1) && (argv[1][0] == '-')) {
		/* Just list all the /dev/input/event<N> devices with their names */
		if (!strcmp(argv[1], "-list")) {
			char	file[128];

			for (count = 0; count < 512; count++) {
				sprintf(file, "/dev/input/event%d", count);
				_EVIOinfo(file, 0);
			}
			exit(0);
		}

		/* Report information about the device and then quit */
		else if (!strcmp(argv[1], "-nodata")) {
			nodata_flag = 1;
			argv++; argc--;
		}

		/* Report unknown input events -- or punk something again */
		else if (!strcmp(argv[1], "-repunk")) {
			aux->report_unknown = 1;
			argv++; argc--;
		}

		/* Unknown option */
		else {
			/* There are currently no other "-" options, so this is an error */
			fprintf(stderr, RED_TEXT "Usage:" NORM_TEXT " %s [-list] | [-nodata] [-repunk] [<event device> (default = '%s')]\n", progname, aux->devfile);	/* NOTE: I'm reporting the default based on what might be changed by the environment variable */
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
		fprintf(stderr, RED_TEXT "couldn't open EVIO device %s\n" NORM_TEXT, aux->devfile);
		aux->open = 0;
		sprintf(aux->version, "- unconnected EVIO -");
	} else {
		if (_EvioInitializeDevice(aux) < 0) {
			vrErrPrintf("main: " RED_TEXT "Warning, unable to initialize EVIO.\n" NORM_TEXT);
		}
		aux->open = 1;
	}

	_EvioPrintStruct(stdout, aux, verbose);

	/* quit if flagged to print the device info but not data */
	if (nodata_flag)
		exit(0);

	/********************/
	/* display a header */
	printf("\n");
	if (aux->num_buttons > 0) {
		printf("         ");
		for (count = 0; count < KEY_MAX; count++) {
			if (test_bit(count, aux->button_bitmask)) {
				printf("%c", key_names[count][strlen(key_names[count])-1]);
				if (button1 < 0)
					button1 = count;
				else if (button2 < 0)
					button2 = count;
				else if (button3 < 0)
					button3 = count;
			}
		}
	}
	if (aux->num_absaxes > 0) {
		printf("           |");
		for (count = 0; count < ABS_MAX; count++) {
			if (test_bit(count, aux->absaxis_bitmask)) {
				printf("%-7s|", abs_names[count]);
			}
		}
	}
	if (aux->num_relaxes > 0) {
		printf("           |");
		for (count = 0; count < REL_MAX; count++) {
			if (test_bit(count, aux->relaxis_bitmask)) {
				printf("%-7s|", rel_names[count]);
			}
		}
	}
	printf("\n");

	/**********************/
	/* display the output */
	while(aux->open && !done) {
		if (_EvioReadInput(aux) > 0) {
			if (aux->num_buttons > 0) {
				printf("buttons: ");
				for (count = 0; count < KEY_MAX; count++) {
					if (test_bit(count, aux->button_bitmask)) {
						printf("%d", aux->button[count]);
					}
				}
			}
			if (aux->num_absaxes > 0) {
				printf("  absaxes: |");
				for (count = 0; count < ABS_MAX; count++) {
					if (test_bit(count, aux->absaxis_bitmask)) {
/* Four choices of how to render axes (5 vs. 7 width and dash vs. numeric representation) */
#if 0	/* 5 wide dash representation */
						printf("%5s|",
							(aux->absaxis[count] < -0.6 ?    "-    " :
							 (aux->absaxis[count] < -0.3 ?   " -   " :
							  (aux->absaxis[count] <  0.3 ?  "  +  " :
							   (aux->absaxis[count] <  0.6 ? "   - " :
											 "    -" )))))));
#elif 1	/* 7 wide dash representation */
						printf("%7s|",
							(aux->absaxis[count] < -0.7 ?      "-      " :
							 (aux->absaxis[count] < -0.4 ?     " -     " :
							  (aux->absaxis[count] < -0.1 ?    "  -    " :
							   (aux->absaxis[count] <  0.1 ?   "   +   " :
							    (aux->absaxis[count] <  0.4 ?  "    -  " :
							     (aux->absaxis[count] <  0.7 ? "     - " :
											   "      -" )))))));
#elif 1	/* 7 wide numeric representation */
						printf("%7.4f|", aux->absaxis[count]);
#else	/* 5 wide numeric representation */
						printf("%5.2f|", aux->absaxis[count]);
#endif
					}
				}
			}
			if (aux->num_relaxes > 0) {
				printf("  relaxes: |");
				for (count = 0; count < REL_MAX; count++) {
					if (test_bit(count, aux->relaxis_bitmask)) {
						printf("%7.2f|", aux->relaxis[count]);
					}
				}
			}
			printf("    \r");
			fflush(stdout);

			/* Alternate mode of quitting is to press buttons 1 & 2 */
			if (aux->num_buttons >= 2) {
				if (aux->button[button1] && aux->button[button2])
					done = 1;
			}

			/* For printing the internal data in the middle of testing */
			/*   use the chorded input of buttons 1 & 3                */
			if (aux->num_buttons >= 3) {
				if (aux->button[button1] && aux->button[button3]) {
					printf("\n");
					_EvioPrintStruct(stdout, aux, verbose);
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
	_EvioCloseDevice(aux);
	vrPrintf(BOLD_TEXT "\nEVIO device closed\n" NORM_TEXT);
#endif
}

#endif /* } TEST_APP */

#endif /* } __linux */

#if defined(MAN_PAGE) /* {  :set syntax=nroff  */
.\"* ======================================================================= "
.\"*                                                                         "
.\"*   11            eviotest.1                                              "
.\"* .111            Author(s): Bill Sherman                                 "
.\"*   11            Created: August 19, 2013                                "
.\"*   11            Last Modified: August 19, 2013                          "
.\"* 111111                                                                  "
.\"*                                                                         "
.\"* Man page for the FreeVR test program for the EVIO (aka evdev) Linux     "
.\"*   input system.                                                         "
.\"*                                                                         "
.\"* Copyright 2013, Bill Sherman, All rights reserved.                      "
.\"* With the intent to provide an open-source license to be named later.    "
.\"* ======================================================================= "
.\"********************************* TITLE ********************************* "
.\" the ".TH" title line must be the first non-comment line in the file      "
.\" .TH <title> <section> <date> <source> <manual>                           "
.TH EVIOTEST 1 "19 August 2013" "FreeVR 0.6d" "FreeVR Commands"

.\" ********************************* NAME *********************************     "
.\" The NAME section should have the name in bold followed by a dash, followed   "
.\"   by a one-line description which can be used in the whatis/apropos database "
.\" .SH <section header name>                                                    "
.SH NAME

.B eviotest
\- test the setup of an input device connected via the Linux
input event systems (aka evdev or EVIO).
.\" ******************************* SYNOPSIS ******************************* "
.\" .SH <section header name>                                                "
.SH SYNOPSIS

\fBeviotest\fI [-list] | [-nodata] [-repunk] [<\fIevent device\fP>]
.\" ****************************** DESCRIPTION ***************************** "
.\" .SH <section header name>                                                "
.SH DESCRIPTION

The \fBeviotest\fP program is used to interface with devices connected via
the Linux input event system.  This system is sometimes called "\fIevdev\fP",
or can be referenced by the IOCTL code of "\fIEVIO\fP".  The \fBeviotest\fP
program can be used to list input devices and provide their self-reported
names, or to provide a live report of the inputs.
.PP
Before rendering the input stream, \fBeviotest\fP will output information
specific to the input device such as the number and name of all button
inputs, absolute axis (aka valuator) inputs and all relative axis inputs.
.PP
The program is terminated by pressing the interrupt key (usually ^C).
Note that sometimes this doesn´t fully work the first time, but it will
work the second time.
.\" ******************************* OPTIONS ******************************** "
.\" .SH <section header name>                                                "
.SH OPTIONS

.TP 0.5i
.B -list
The \fB-list\fP option lists all Linux event devices of the form
"\fI/dev/input/event<N>\fP", where N is from [0:512].
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
.br
.TP 0.5i
.B -repunk
The \fB-repunk\fP option will \fIrep\fPort all \fIunk\fPnown input events.
Typically, unknown events will go unreported so as to not sully the event
output with undesired information.
However, for debugging purposes it may be of interest to know whether there
are other input events being received, and what they might be.
.\" ******************************* ARGUMENTS ****************************** "
.\" .SH <section header name>                                                "
.SH ARGUMENTS

.TP 0.5i
.B <event device>
The \fI<event device>\fP argument is a filesystem path pointing to a specific
Linux input event device \- for example "/dev/input/event5".
.\" ************************* ENVIRONMENT VARIABLES ************************ "
.\" .SH <section header name>                                                "
.SH ENVIRONMENT VARIABLES

.TP 0.5i
.B EVIO_DEVICE
Set the path of the default input device to read when no device argument
is provided.
.\" ******************************* EXAMPLES ******************************* "
.\" .SH <section header name>                                                "
.SH EXAMPLES

.TP 0.5i
List all Linux input event devices:
% \fBeviotest\fP -list
.br
.TP 0.5i
Report inputs from the device at "/dev/input/event5":
% \fBeviotest\fP /dev/input/event5
.br
.TP 0.5i
Set the default input device to be "/dev/input/event6" and then report those events:
% setenv EVIO_DEVICE /dev/input/event6
.br
% \fBeviotest\fP
.\" ********************************* BUGS ********************************* "
.\" .SH <section header name>                                                "
.SH BUGS

The fact that \fBeviotest\fP does not always terminate when receiving the
first interrupt signal (ie. ^C) may be considered a bug.  The workaround
is already coded in the program \- just send a second interrupt.
.\" ********************************* TODO ********************************* "
.\" .SH <section header name>                                                "
.SH TODO

.HP 0.5i
\fB*\fP It would be nice to manipulate device LEDs
.HP 0.5i
\fB*\fP It would be nice to manipulate device force feedback events
.HP 0.5i
\fB*\fP Implement a screen-rendering option
.\" ******************************* SEE ALSO ******************************* "
.\" .SH <section header name>                                                "
.SH SEE ALSO

joytest(1), fobtest(1), ...
.\" TODO: finish "see also"                                                  "
.\" ******************************* COPYRIGHT ****************************** "
.\" .SH <section header name>                                                "
.SH COPYRIGHT

Copyright 2013, Bill Sherman, All rights reserved.
.\"With the intent to provide an open-source license to be named later.      "
.\" ****************************** OTHER NOTES ***************************** "
.\" .SH <section header name>                                                "
.SH OTHER NOTES

The source code for \fBeviotest\fP is in the "\fIvr_input.evio.c\fP" file,
which also handles the EVIO input interface to the \fBFreeVR\fP library.


.\"* ======================================================================= "
#endif /* } MAN_PAGE */

