/* ======================================================================
 * 
 *  CCCCC          vr_input.vrpn.c
 * CC   CC         Author(s): Bill Sherman
 * CC              Created: May 11, 2013 (copied from vr_input.vrpn.cxx)
 * CC   CC         Last Modified: September 19, 2013
 *  CCCCC
 * 
 * Code file for FreeVR inputs from the VRPN input device.
 * 
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/*************************************************************************

NOTE: this is the C version of the VRPN FreeVR interface, and unlike the C++
	version, it does not require the use of the VRPN library.  Instead it
	directly interprets the incoming VRPN stream.  But otherwise the
	end-user experience should be exactly the same.

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
		input "<name>" = "<freevr-type>(<device-name>:<VRPN-type>[<input-num>])";

		input "2switch[left]" = "2switch(Joystick0:button[0])";
	  :-(	input "2switch[hat-x]"= "2switch(Joystick0:analog[4], threshold)";
		input "valuator[x]"   = "valuator(Joystick0:analog[3])";
		input "valuator[y]"   = "valuator(Joystick0:analog[-4])";
		input "6sensor[head]" = "6sensor(Tracker0:tracker[0, r2e])";
		input "6sensor[wand]" = "6sensor(Tracker0:tracker[1, r2e])";
		input "6sensor[wand2]" = "6sensor(sim6[1])";

		input "2switch[yo]"   = "2switch(Tracker0:button[1])";

		future possibilities:

	  :-(	input "<name>" = "Nswitch(<device>:switch[<number>])";
	  :-(	input "<name>" = "Nsensor(<device>:glove[<number>])";


	Controls are specified with the "control" option:
		control "<control option>" = "2switch(<device-name>:<VRPN-type>[<num>])";

		control "print_struct" = "2switch(Joystick0:button[10])";
		control "sensor_next" = "2switch(Joystick0:button[11])";
	  ...

	Here are the available (2switch) control options for FreeVR:
		"print_help" -- print info on how to use the input device
		"print_context" -- print the overall FreeVR context data structure (for debugging)
		"print_config" -- print the overall FreeVR config data structure (for debugging)
		"print_input" -- print the overall FreeVR input data structure (for debugging)
		"print_struct" -- print the internal VRPN data structure (for debugging)
		"print_sim6opts" -- print the internal VRPN data for simulated 6-sensors (for debugging)

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

	Here are the FreeVR configuration argument options for the VRPN server:
		"host" (or "hostname") - string of the machine running the VRPN server.
		"port" - socket port of the VRPN server (3883 is the VRPN default).
	 	"valScale" - float value of the valuator sensitivity scale
		"scale" (or "transscale") - float value by which to scale the
			tracking location data (defaults to 3.28084 to convert
			from meters to feet).


	NOTE: VRPN inputs cannot be combined with an X-windows configuration
		that uses extension devices of X.  It's not clear why this is,
		it just is.  [subnote: probably this only pertains to the
		VRPN-library version.]

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

	------------------------------------------------------------------
	11 May 2013 (Bill Sherman)
		After an initial foray into creating from scratch a new VRPN
		reader written in C and directly interpreting the VRPN protocol
		based on my VruiDD input device, today I decided that I could
		reuse most of what already was written for the C++ VRPN reader
		based on the actual VRPN library.

	17 May 2013 (Bill Sherman)
		In comparing the original VRPN C++ code with the input-skeleton
		code, I found that there was some updating that needed to be
		done.  So I did it.

		Also began working on bringing in my non-library Vrpn
		communication code.

	17-23 May 2013 (Bill Sherman)
		At this stage, enough has been done to initiate communications
		with a VRPN server and exchange version numbers.  I am doing
		things a little differently with this input device in that
		I'm using the 5 standard device callbacks.  As this requires
		a vrInputDevice type, I've created a shell version of
		vrInputDevice that has the bare minimum information for the
		test application.

	28-29 May 2013 (Bill Sherman)
		I copied the "_VrpnReadInput()" routine from the stand-alone
		"vrpntest.c" program into here, and from that got the basic
		communication working.  Still need to convert it into FreeVR
		format.

	12-16 July 2013 (Bill Sherman)
		I added storage of the incoming data into the auxiliary data
		structure.  Tracking "posquat" data, button data and
		analog/valuator data are now stored in a VRPN_Device structure,
		and an array of those structures is part of the auxiliary
		structure.

		I added flags for each input to indicate whether it has been
		received since the last input processing operation, or has been
		received at all since the beginning of the program.

		I also now store the name of devices as reported by the VRPN
		server, and I now store all the types of messages reported by
		the server, not just tags for the 5 messages I'm especially
		looking for.

		The "vrpntest" application now basically works.

	22-25 July 2013 (Bill Sherman)
		I revamped the FreeVR side of the device handler (ie. for the
		no VRPN client library version).  First, just to get it to
		compile, then to get it to print out all the incoming values.
		Presently it now handles button presses as inputs and as
		self-controls.

	30 July 2013 (Bill Sherman)
		Tracker (aka 6sensor) inputs have now been implemented!  Thus
		we now have a fully working version of the VRPN input system.
		I also added some robustness to device-name access (making sure
		there's not a NULL pointer first).  [NOTE: though there still
		is some robustness that needs to be added in other areas.]
		I also added some new arguments including the surprisingly
		missing "host" argument (which can now also be referred to as
		"hostname"), plus "scale"/"transscale" and "valscale".

	9 August 2013 (Bill Sherman)
		I improved the robustness of the FreeVR VRPN interface.
		Specifically, I now do a better job of handling the case where
		an unknown host is provided as the VRPN server.  Also I better
		handle the situation where the FreeVR configuration requests a
		VRPN device that doesn't exist (and thus resulting in a case
		where aux->button_map_device == NULL, which obviously can't be
		dereferenced).  So I now print warnings when this happens.

	2-5 September 2013 (Bill Sherman) -- adopting a consistent usage
		of str<...>cmp() functions (using logical negation).

		Also, implemented simulated 6-sensors for VRPN.  This is the
		first time there is a device that can have both actual 6-sensor
		inputs as well as simulated inputs.  Thus there is a hack-ish
		method used to differentiate between them.  There already was
		a VRPNdevice[] array that listed all the devices coming from
		the vrpn_server, so device 0 (VRPNdevice[0]) is now reserved
		as the internal "simulator" device and thus does not come from
		the vrpn_server inputs.

		However, apart from the "name" and "newly_reported_posquat"
		fields, it doesn't really use the contents of VRPNdevice, but
		is more of a placeholder.  Because simulated 6-sensors are
		(presently!) designed to just have one per device (with a
		selection of active 6-sensors), and generally don't have button
		or valuator values -- because those don't need to be simulated!

		I also added the ability to quit "vrpntest" when pressing
		buttons 1 & 2 of any VRPN device -- was easier than watching
		for exactly the first two overall buttons, and in reality will
		work better too.

	15 September 2013 (Bill Sherman)
		Renaming "active_sensor" to "active_sim6sensor".
		Changed the "0x%p" format to the improved "%#p" format.

	19 September 2013 (Bill Sherman)
		Added "-p"/"-port" command line argument to "vrpntest".
		Wrote the man page.

	10 November 2013 (Bill Sherman)
		Adjusted the inclusion of "endian.h" for APPLE compilations.

TODO:
	- Make handling of lack of available simulated 6-sensors robust!  Basically
		what this means is adding a conditional to all of the self-control
		callbacks that make use of the "active_sim6sensor" value.

	- I'm concerned that the instance numbers are not being handled correctly.
		Specifically, they are meant to indicate the number of the
		button/analog/tracker on the VRPN server side, but they may
		also be inadvertently being used as the overall button/analog/etc
		number on the FreeVR side, and that's not correct -- it needs to
		be handled on a device by device basis.

		So first thing to do is some testing.

	- DONE: I may need to handle simulated 6-sensors separate from trackers
		coming from the VRPN server.  (In fact, this may be a bigger
		problem even if I have two joystick devices on the server!)

	- DONE: robustly handle the unknown host circumstance

	- DONE: robustly handle the situation of (aux->button_map_device == NULL)

	- DONE: print a stern warning when VRPN devices listed in ".freevrrc"
		are not being sent from the VRPN server (which is the cause of
		the above problem).

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

	- find out if "vrpn_Forwarder_Brain" is always the last device sent by
		the server, and if so, use that as a flag (see questions, below)

	- consider how I might use the mapped_button/mapped_analog/mapped_tracker
		VRPNdevice fields (or get rid of them).  (I should at least be
		printing them somewhere.)

	- DONE: Add a "-nodata" CLA to "vrpntest" that allows one to just get a report
		of what the VRPN server is serving.

	- Consider adding a "-repunk" CLA to "vrpntest" to REPort UNKnown inputs.

	- Add a screen-based (curses) rendering option of the tracking data
		to "vrpntest" (might be good to have a means of rendering a
		quaternion representation)

	- Update the _VrpnPrintStruct() routine to report the stored input data.

	- Test whether there are VRPN messages that can be sent back to the
		server!

	- figure out what values to assign the 'version' and
		'operating parameter' fields

	- attempt to get the VRPN_LIB version working as well!

	- delete old DONOTMOVETO_INITDEV code

	- DONE: determine whether the _VrpnUserdata structure (and affiliated
		code) are required for the non VRPN_LIB version.  [Answer:
		pretty sure that it is only for the VRPN_LIB version, so now
		conditionally removed.]

	- add "phantom" type ??

	- PARTIAL: allow a FreeVR-created 6-sensor from valuator inputs to be created.
		(ie. can either use the VRPN server options for creating a
		"tracker" from analog inputs, or can use the standard FreeVR
		method such as is available in the raw Magellan code).  In the
		X11-windows input this is the "sim6" input type.

	- determine whether old analog values can be saved, and compared with
		incoming analog values, reporting only those that have changed.
		NOTE: I do make use of the fact that VRPN only sends data as it
		determines they are new, so I may not need to do anything beyond
		flagging which data has recently been received -- which I now do.

	- Add information about the VRPN protocol to the top-comments.

TOASK:
	(ie. questions to ask the VRPN community)

	- What is the 8th float value reported by VelQuat?

	- What does the "VRPN Control" device do?
		(and is there an example tool that generates these device events?)

	- What does the "vrpn_Forwarder_Brain" device do?
		(and is there an example tool that generates these device
		events?)
		- and perhaps most importantly!  Will this always be the last
			device sent by the server?  (If so we can use this as
			a flag to know that we've received all the devices.)

	- What interesting messages can be sent from the client to the server?
		(or to specific devices configured in a server)

**************************************************************************/
#if !defined(FREEVR) && !defined(TEST_APP) && !defined(CAVE)
/* Choose how this code should be compiled (ie. for CAVE, standalone, etc). */
#  define	FREEVR
#  undef	TEST_APP
#  undef	CAVE
#endif

#undef	VRPN_LIB	/* The #undef version does it's own parsing of the data stream, and does not require linking with the VRPN library */
#undef	DEBUG


#include <stdio.h>
#include <string.h>		/* needed for strcmp(), strncmp(), strncpy() & strchr() */
#include <stdint.h>		/* needed for uint16_t & uint32_t types */
#include <errno.h>		/* for errno global and perror() */
#include <fcntl.h>		/* for fcntl() to set socket parameters */
#ifdef __APPLE__
#  include </usr/include/machine/endian.h>/* set __BYTE_ORDER value */
#else
#  include <endian.h>		/* set __BYTE_ORDER value */
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#include "vr_serial.h"
#include "vr_debug.h"
#include "vr_utils.h"

#if defined (FREEVR)
#  include "vr_input.h"
#  include "vr_input.opts.h"
#  include "vr_parse.h"
#  include "vr_shmem.h"
#endif

#if defined(__cplusplus)
}
#endif

#if defined(TEST_APP)
#  include <stdlib.h>		/* needed for malloc(), getenv() & atoi() */
#  include <signal.h>		/* needed for signal() */
#  include "vr_socket.c"	/* needed for vrSocketCall */
#  include "vr_serial.c"	/* needed just for vrSerialAwaitData()  -- which may now be misnamed */
#  include "vr_utils.c"		/* needed for vrEndianSwap4() & vrEndianSwap8() */
#  define vrShmemAlloc0 malloc
#  define vrShmemFree free


/*****************************************************************************/
/* vrInputDevice is a structure type used in the test application (TEST_APP) */
/*   to mimic the type from vr_input.h, and allowing the test app to make    */
/*   use of the FreeVR input callback functions.                             */
typedef struct vrID_st {
		char		*name;		/* CONFIG: name assigned to device in config file */
		int		operating;	/* whether the device is operating */
		void		*aux_data;	/* auxiliary data, specific to the type of input */
	} vrInputDevice;
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


#ifdef VRPN_LIB /* { */
/*** special VRPN includes ***/
#include "vrpn/vrpn_Tracker.h"
#include "vrpn/vrpn_Button.h"
#include "vrpn/vrpn_Analog.h"

#undef DO_EXTERN_C
#if defined(__cplusplus) && defined(DO_EXTERN_C)
extern "C" {
#endif

#endif /* } VRPN_LIB */

	/******************************************************************/
	/***      definitions for interfacing with the VRPN device      ***/
	/***                                                            ***/

#define DEFAULT_HOST	"localhost"
#define DEFAULT_PORT	3883
#define	HEADER_SIZE	24
#define BUFSIZE		2048

/* TODO: determine whether we still want these */
/* VRPN sensitivity values */
#define TRANS_SENSITIVITY	0.1
#define ROT_SENSITIVITY		1.0
#define VALUATOR_SENSITIVITY	17.0	/* empirically determined */


/****************************************************************************/
/* NOTE: only the "RELIABLE" flag is presently used */
typedef enum {
		RELIABLE	 = (1<<0),
		FIXED_LATENCY	 = (1<<1),
		LOW_LATENCY	 = (1<<2),
		FIXED_THROUGHPUT = (1<<3),
		HIGH_THROUGHPUT	 = (1<<4)
	} VRPNServiceFlags;


/****************************************************************************/
typedef enum {
		SENDER_DESCRIPTION	= (-1),
		TYPE_DESCRIPTION	= (-2),
		UDP_DESCRIPTION		= (-3),
		LOG_DESCRIPTION		= (-4),
		DISCONNECT_MESSAGE	= (-5)
		/* ...  -- the non-negative message-type values are mapped to   */
		/*         ASCII strings immediately after a client-server      */
		/*         connection is established, and then used to indicate */
		/*         what type of data is arriving (like tracker data or  */
		/*         button data.                                         */
	} VRPNMessageType;


/****************************************************************************/
typedef enum {
		LOG_NOTHING		= (0),
		LOG_INCOMING		= (1<<0),
		LOG_OUTGOING		= (1<<1)
	} VRPNLogFlags;


/****************************************************************************/
/* VRPNheader -- data packet header for communication between client and server */
typedef struct {
		uint32_t	message_size;			/* header-length (24) + unpadded message length (4 + string_length) */
		uint32_t	time_seconds;			/* seconds component of time */
		uint32_t	time_useconds;			/* micro-seconds component of time */
		uint32_t	code;				/* numeric encoding describing message source (source code :-) */
								/*   code is "sender" when (type == SENDER_DESCRIPTION)        */
								/*   code is "message-number" when (type == TYPE_DESCRIPTION)  */
		int32_t		message_type;			/* type of message (from SystemMessageType) */
		int32_t		padding;			/* need 4 more bytes to be on an 8-byte boundary */
	} VRPNheader;

/****************************************************************************/
/* VRPNdevice -- structure containing input data from a particular VRPN device */
#define MAX_BUTTONS	16
#define MAX_VALUATORS	16
#define MAX_6SENSORS	16
typedef struct {
		char	name[256];				/* name of this device -- according to VRPN server */

		uint8_t	ever_reported_button[MAX_BUTTONS];	/* flag indicating whether the VRPN server has ever sent input numbered such */
		uint8_t	newly_reported_button[MAX_BUTTONS];	/* flag indicating whether the VRPN server just sent input numbered_such */
		uint8_t	mapped_button[MAX_BUTTONS];		/* flag indicating whether the given button has been mapped to a FreeVR input */
		uint8_t	button[MAX_BUTTONS];			/* values of VRPN button inputs */
		double	button_time[MAX_BUTTONS];		/* time of the last button event */

		uint8_t	ever_reported_analog[MAX_VALUATORS];	/* flag indicating whether the VRPN server has ever sent input numbered such */
		uint8_t	newly_reported_analog[MAX_VALUATORS];	/* flag indicating whether the VRPN server just sent input numbered_such */
		uint8_t	mapped_analog[MAX_VALUATORS];		/* flag indicating whether the given analog has been mapped to a FreeVR input */
		double	analog[MAX_VALUATORS];			/* values of VRPN valuator inputs */
		double	analog_time[MAX_VALUATORS];		/* time of the last valuator event */

		uint8_t	ever_reported_posquat[MAX_6SENSORS];	/* flag indicating whether the VRPN server has ever sent input numbered such */
		uint8_t	newly_reported_posquat[MAX_6SENSORS];	/* flag indicating whether the VRPN server just sent input numbered_such */
		uint8_t	mapped_tracker[MAX_6SENSORS];		/* flag indicating whether the given tracker has been mapped to a FreeVR input */
		double	posquat[MAX_6SENSORS][7];		/* values of VRPN 6-sensor inputs */
		double	posquat_time[MAX_6SENSORS];		/* time of the last 6-sensor event */

		uint8_t	ever_reported_velquat[MAX_6SENSORS];	/* flag indicating whether the VRPN server has ever sent input numbered such */
		uint8_t	newly_reported_velquat[MAX_6SENSORS];	/* flag indicating whether the VRPN server just sent input numbered_such */
		double	velquat[MAX_6SENSORS][7];		/* values of VRPN 6-sensor input velocity values (currently unused) */
		double	velquat_time[MAX_6SENSORS];		/* time of the last 6-sensor velocity event */
	} VRPNdevice;

/*************************************************************/
/*** private structure of the current data from the VRPN. ***/
typedef struct {
		/* these are for interfacing with the daemon */
		int		vrpn_socket;			/* communication file descriptor */
		char		*vrpn_host;			/* name of daemon's host */
		int		vrpn_port;			/* port on host listening for application connections */
		int		open;				/* flag with VRPN successfully open */

#ifndef VRPN_LIB /* { */
		/* these are for internal data parsing */
		unsigned char	buf[BUFSIZE];			/* the incoming data stream */
		int		eobuf_pos;			/* the last read byte in the buffer */
#endif /* } !VRPN_LIB */

#define MAX_DEVICES 8 /* there can be <n> VRPN devices */

/* TODO: much of the next few lines are currently only used for the VRPN_LIB version of this code -- need to clarify which is still needed and where */
		int		num_button_devices;		/* num of button devices used */
		char		button_names[MAX_DEVICES][256];	/* name of the button devices */
#ifdef VRPN_LIB /* { */
	   vrpn_Button_Remote	*vrpn_button[MAX_DEVICES];	/* VRPN button class */
#endif /* } VRPN_LIB */

		int		num_analog_devices;		/* num of analog devices used */
		char		analog_names[MAX_DEVICES][256];	/* name of the analog devices */
#ifdef VRPN_LIB /* { */
	   vrpn_Analog_Remote	*vrpn_analog[MAX_DEVICES];	/* VRPN analog class */
#endif /* } VRPN_LIB */

		int		num_tracker_devices;		/* num of tracker devices used */
		char		tracker_names[MAX_DEVICES][256];	/* names of the tracker devices */
#ifdef VRPN_LIB /* { */
	   vrpn_Tracker_Remote	*vrpn_tracker[MAX_DEVICES];	/* VRPN tracker class */
#endif /* } VRPN_LIB */

#ifndef VRPN_LIB /* { */
#  define MAX_CODES 128 /* the upper bounds on number of codes in the array */
		int		biggest_code;			/* highest code value recorded */
		char		message_codes[MAX_CODES][256];	/* string names for each message code VRPN server is reporting */
		int		MessageSensorPos;		/* VRPN code for sensor location data */
		int		MessageSensorVel;		/* VRPN code for sensor velocity data */
		int		MessageSensorAcc;		/* VRPN code for sensor acceleration data */
		int		MessageButton;			/* VRPN code for sensor button data */
		int		MessageValuator;		/* VRPN code for sensor velocity data */
#endif /* } !VRPN_LIB */
		/* TODO: other VRPN classes include Text, Phantom?, ... */


		/* these are for internal data parsing */
		char		version[512];			/* self-reported version of the device */
		char		op_params[256];			/* operating parameters of the device (according to it) */
		int		server_ver_major;		/* Major version code of the VRPN server */
		int		server_ver_minor;		/* Minor version code of the VRPN server */

		/* information about how to filter the data for use with the VR library (FreeVR or CAVE) */
		float		scale_valuator;			/* scaling factor for valuators */
		float		scale_trans;			/* multiplier to scale from the VRPN location units (meters) */

#ifdef CAVE
		/* CAVE specific fields here */
		/* ... */

#elif defined(FREEVR)
		/* FREEVR specific fields here */

		int		mapped;				/* flag to indicate when the VRPN devices have been mapped to FreeVR devices */

		/* 2switch/button mappings */
		int		num_buttons;			/* it is often wise to store the number of button inputs in addition to knowing the maximum possible */
		vr2switch	**button_inputs;		/* the maximum number of buttons is devinfo->num_2ways + devinfo->num_scontrols (all the self-controls are not necessarily buttons) */
		int		*button_map_button;		/* An array of numbers indicating the particular button */
		char		**button_map_devicename;	/* An array of VRPN device names for each button input */
		VRPNdevice	**button_map_device;		/* An array of VRPN device pointers linking to the specific device structure */

		/* Valuator/analog mappings */
		int		num_valuators;			/* it is often wise to store the number of valuator inputs in addition to knowing the maximum possible */
		vrValuator	**valuator_inputs;		/* the maximum number of valuators is devinfo->num_valuators + devinfo->num_scontrols (all the self-controls are not necessarily valuators) */
		int		*valuator_map_valuator;		/* An array of numbers indicating the particular valuator */
		float		*valuator_sign;			/* An array of numbers indicating the sign of each valuator input */
		char		**valuator_map_devicename;	/* An array of VRPN device names for each valuator input */
		VRPNdevice	**valuator_map_device;		/* An array of VRPN device pointers linking to the specific device structure */

		/* 6-sensor/tracker mappings */
		int		num_6sensors;			/* it is often wise to store the number of 6-sensor inputs in addition to knowing the maximum possible */
		vr6sensor	**tracker_inputs;		/* the maximum number of 6sensors is devinfo->num_6sensors currently no self-control options) */
		int		*tracker_map_tracker;		/* An array of numbers indicating the particular 6sensor */
		char		**tracker_map_devicename;	/* An array of VRPN device names for each 6sensor input */
		VRPNdevice	**tracker_map_device;		/* An array of VRPN device pointers linking to the specific device structure */
		int		sim6sensor_change;		/* flag that indicates whether any of the controls to a simulated 6-sensor have changed */

#  if 0 /* Question: were these used by the VRPN_LIB version of this file? */
		vr2switch	*button_inputs[MAX_DEVICES][MAX_BUTTONS];	/* each button device can have <n> buttons */
		vrValuator	*valuator_inputs[MAX_DEVICES][MAX_VALUATORS];	/* ditto for analog */
		vr6sensor	*tracker_inputs[MAX_DEVICES][MAX_6SENSORS];	/* ditto for trackers */
#  endif

#  if 0 /* TODO: not sure if we want/need this */
#    define MAX_CONTROLS  10
		vrControl	*control_inputs[MAX_CONTROLS];
#  endif

		/* for the FreeVR simulated 6-sensors */
		int		active_sim6sensor;		/* The simulated 6-sensor that is being actively controlled */
		vr6sensorConv	sensor6_options;		/* Structure of settings that affect how a 6-sensor is simulated */
		float		sim_values[6];			/* Array of 6 valuator values used to move the simulated 6-sensor */
#endif /* end library-specific fields */

		/* information about the current (raw/unprocessed) values */
		int		num_devices;			/* just a counter to know how many have been mentioned by the VRPN server */
		VRPNdevice	device[MAX_DEVICES];		/* array of possible VRPN input devices */

	} _VrpnPrivateInfo;


#ifdef VRPN_LIB /* { */
/***********************************************************/
/*** structure for the userdata of the callback handles. ***/
typedef struct {
		int		  num;		/* the number in the device list of this type */
		_VrpnPrivateInfo *aux;		/* a pointer to the overall private info structure */
	} _VrpnUserdata;
#endif /* } VRPN_LIB */



	/**************************************************/
	/*** General NON public VRPN interface routines ***/
	/**************************************************/

/******************************************************/
static void _VrpnInitializeStruct(_VrpnPrivateInfo *aux)
{
	int	count;

	aux->version[0] = '\0';
	aux->op_params[0] = '\0';

	aux->biggest_code = -1;
	aux->MessageSensorPos = -1;
	aux->MessageSensorVel = -1;
	aux->MessageSensorAcc = -1;
	aux->MessageButton = -1;
	aux->MessageValuator = -1;

	aux->num_button_devices = 0;
	aux->num_analog_devices = 0;
	aux->num_tracker_devices = 0;

	aux->num_devices = 1;				/* device 0 is reserved for a simulator device! */
	strcpy(aux->device[0].name, "simulator");	/* The simulator device has the empty name */

	aux->scale_valuator = 1.0;			/* set the default valuator scaling factor */
	aux->scale_trans = 3.28084;			/* convert from meters to feet by default */

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
static void _VrpnPrint6sensorOptions(FILE *file, _VrpnPrivateInfo *aux, vrPrintStyle style)
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
		vrFprintf(file, BOLD_TEXT "VRPN - 6-sensor settings:" NORM_TEXT "\n");
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
static void _VrpnPrintStruct(FILE *file, _VrpnPrivateInfo *aux, vrPrintStyle style)
{
	int		count;
	int		count2;
#ifdef FREEVR /* { */
	vrGenericInput	*input;
#endif /* } FREEVR */

	vrFprintf(file, "VRPN device internal structure:\n");
	vrFprintf(file, "\r\tversion -- '%s'\n", aux->version);
	vrFprintf(file, "\r\tserver connection -- TCP@%s:%d\n", aux->vrpn_host, aux->vrpn_port);
	vrFprintf(file, "\r\toperating parameters -- '%s'\n", aux->op_params);
	vrFprintf(file, "\r\topen = %d\n", aux->open);
#ifdef FREEVR /* { */
	vrFprintf(file, "\r\tmapped = %d\n", aux->mapped);
#endif /* } FREEVR */

	vrFprintf(file, "\r\tscale_valuator = %f\n", aux->scale_valuator);
	vrFprintf(file, "\r\tscale_trans = %f\n", aux->scale_trans);

	/* NOTE: count starts at 1 since device[0] is the simulator and is not from the VRPN server */
	vrFprintf(file, "\tVRPN Server reported devices:\n");
	for (count = 1; count < MAX_DEVICES; count++) {
		if (aux->device[count].name[0] != '\0')
			vrFprintf(file, "\t\tDevice %d is '%s'\n", count, aux->device[count].name);
	}
	vrFprintf(file, "\tVRPN Server reported message types:\n");
	for (count = 0; count < aux->biggest_code; count++) {
		vrFprintf(file, "\t\tMessage Code %2d is '%s'\n", count, aux->message_codes[count]);
	}

	vrFprintf(file, "\tWatch Messages:\n");
	vrFprintf(file, "\t\t%2d: Tracker position\n", aux->MessageSensorPos);
	vrFprintf(file, "\t\t%2d: Tracker velocity\n", aux->MessageSensorVel);
	vrFprintf(file, "\t\t%2d: Tracker acceleration\n", aux->MessageSensorAcc);
	vrFprintf(file, "\t\t%2d: Button event\n", aux->MessageButton);
	vrFprintf(file, "\t\t%2d: Valuator event\n", aux->MessageValuator);

#if 0 /* 09/04/13 -- removing this for now since it's a duplicate of "VRPN Server reported devices above */
/* TODO: print all the active devices & inputs -- or maybe just all of them -- or all of them that have been "touched" */
	/* print information about the array of VRPNdevices */
	vrFprintf(file, "\tVRPN Devices:\n");
	for (count = 0; count < aux->num_devices; count++) {
		vrFprintf(file, "\t\t%2d: %s\n", count, aux->device[count].name);
	}
#endif

#ifdef FREEVR /* { */
	/* button inputs */
	vrFprintf(file, "\n\tbutton inputs:\n");
	for (count = 0; count < aux->num_buttons; count++) {
		vrFprintf(file, "\t\t2switch_input[%d] = %p (%s:%s) -- mapped to VRPN device '%s':button[%d] %s\n",
			count,
			aux->button_inputs[count],
			vrInputTypeName(aux->button_inputs[count]->input_type),
			aux->button_inputs[count]->my_object->name,
			aux->button_map_devicename[count],
			aux->button_map_button[count],
			(aux->button_map_device[count] == NULL ? RED_TEXT "(nodev)" NORM_TEXT : aux->button_map_device[count]->ever_reported_button[aux->button_map_button[count]] ? "(used)" : ""));
	}

	/* valuator inputs */
	vrFprintf(file, "\n\tvaluator inputs:\n");
	for (count = 0; count < aux->num_valuators; count++) {
		vrFprintf(file, "\t\tvaluator_input[%d] = %p (%s:%s) -- mapped to VRPN device '%s':analog[%d] %s\n",
			count,
			aux->valuator_inputs[count],
			vrInputTypeName(aux->valuator_inputs[count]->input_type),
			aux->valuator_inputs[count]->my_object->name,
			aux->valuator_map_devicename[count],
			aux->valuator_map_valuator[count],
			(aux->valuator_map_device[count] == NULL ? RED_TEXT "(nodev)" NORM_TEXT : aux->valuator_map_device[count]->ever_reported_analog[aux->valuator_map_valuator[count]] ? "(used)" : ""));
	}

	/* 6sensor inputs */
	vrFprintf(file, "\n\t6sensor inputs:\n");
	for (count = 0; count < aux->num_6sensors; count++) {
		vrFprintf(file, "\t\t6sensor_input[%d] = %p (%s:%s) -- mapped to VRPN device '%s':tracker[%d] %s\n",
			count,
			aux->tracker_inputs[count],
			vrInputTypeName(aux->tracker_inputs[count]->input_type),
			aux->tracker_inputs[count]->my_object->name,
			aux->tracker_map_devicename[count],
			aux->tracker_map_tracker[count],
			(aux->tracker_map_device[count] == NULL ? RED_TEXT "(nodev)" NORM_TEXT : aux->tracker_map_device[count]->ever_reported_analog[aux->tracker_map_tracker[count]] ? "(used)" : ""));
	}

	/* simulated 6-sensor settings */
	vrFprintf(file, "\r");
	_VrpnPrint6sensorOptions(file, aux, brief);
#endif /* } FREEVR */

#ifdef FREEVRx /* { */
	/* button inputs */
	vrFprintf(file, "\r\tbutton_devices = %d:\n", aux->num_button_devices);
	for (count = 0; count < aux->num_button_devices; count++) {
#  ifdef VRPN_LIB /* { */
		vrFprintf(file, "\r\t\tbutton[%d] = '%s' (%#p)\n",
			count,
			aux->button_names[count],
			aux->vrpn_button[count]);
#  endif /* } */
		/* TODO: see if the VRPN_LIB version can use the above (improved) code instead of this */
		for (count2 = 0; count2 < MAX_BUTTONS; count2++) {
			input = (vrGenericInput *)aux->button_inputs[count][count2];
			if (input != NULL) {
				vrFprintf(file, "\r\t\t\tbutton %d: input = %#p (%s:%s)\n",
					count2, input, vrInputTypeName(input->input_type), input->my_object->name);
			}
		}
	}

	/* analog inputs */
	vrFprintf(file, "\r\tanalog_devices = %d:\n", aux->num_analog_devices);
	for (count = 0; count < aux->num_analog_devices; count++) {
#  ifdef VRPN_LIB /* { */
		vrFprintf(file, "\r\t\tanalog[%d] = '%s' (%#p)\n",
			count,
			aux->analog_names[count],
			aux->vrpn_analog[count]);
#  endif /* } */
		for (count2 = 0; count2 < MAX_VALUATORS; count2++) {
			input = (vrGenericInput *)aux->valuator_inputs[count][count2];
			if (input != NULL) {
				vrFprintf(file, "\r\t\t\tanalog %d: input = %#p (%s:%s)\n",
					count2, input, vrInputTypeName(input->input_type), input->my_object->name);
			}
		}
	}

	/* tracker inputs */
	vrFprintf(file, "\r\ttracker_devices = %d:\n", aux->num_tracker_devices);
	for (count = 0; count < aux->num_tracker_devices; count++) {
#  ifdef VRPN_LIB /* { */
		vrFprintf(file, "\r\t\ttracker[%d] = '%s' (%#p)\n",
			count,
			aux->tracker_names[count],
			aux->vrpn_tracker[count]);
#  endif /* } */
		for (count2 = 0; count2 < MAX_6SENSORS; count2++) {
			input = (vrGenericInput *)aux->tracker_inputs[count][count2];
			if (input != NULL) {
				vrFprintf(file, "\r\t\t\ttracker %d: input = %#p (%s:%s)\n",
					count2, input, vrInputTypeName(input->input_type), input->my_object->name);
			}
		}
	}
#endif /* } FREEVRx */
}


/**************************************************************************/
static void _VrpnPrintHelp(FILE *file, _VrpnPrivateInfo *aux)
{
	int	count;		/* looping variable */

#if 0 || defined(TEST_APP)
	vrFprintf(file, BOLD_TEXT "Sorry, VRPN - print_help control not yet implemented." NORM_TEXT "\n");
#else
	vrFprintf(file, BOLD_TEXT "VRPN - inputs:" NORM_TEXT "\n");
	for (count = 0; count < aux->num_buttons; count++) {
		if (aux->button_inputs[count] != NULL) {
			vrFprintf(file, "\t%s -- %s%s\n",
				aux->button_inputs[count]->my_object->desc_str,
				(aux->button_inputs[count]->input_type == VRINPUT_CONTROL ? "control:" : ""),
				aux->button_inputs[count]->my_object->name);
		}
	}
	for (count = 0; count < aux->num_valuators; count++) {
		if (aux->valuator_inputs[count] != NULL) {
			vrFprintf(file, "\t%s -- %s%s\n",
				aux->valuator_inputs[count]->my_object->desc_str,
				(aux->valuator_inputs[count]->input_type == VRINPUT_CONTROL ? "control:" : ""),
				aux->valuator_inputs[count]->my_object->name);
		}
	}
#  if 0
	for (count = 0; count < MAX_6SENSORS; count++) {
		if (aux->tracker_inputs[count] != NULL) {
			vrFprintf(file, "\t%s -- %s%s\n",
				aux->tracker_inputs[count]->my_object->desc_str,
				(aux->tracker_inputs[count]->input_type == VRINPUT_CONTROL ? "control:" : ""),
				aux->tracker_inputs[count]->my_object->name);
		}
	}
#  endif
#endif
}


/******************************************************/
static int _VrpnReadInput(_VrpnPrivateInfo *aux)
{
#ifdef VRPN_LIB /* { */
	/************************************************************/
	/* This section reads the VRPN data using the VRPN library. */
	/************************************************************/
	char	return_code = 1;	/* currently not watching for any problems */
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
#else /* } VRPN_LIB { */
	/************************************************************/
	/* This section reads the VRPN data by parsing the raw data */
	/************************************************************/
	VRPNheader	VRPNdata;		/* storage of the incoming VRPN header */
	int		read_result;		/* return value of the call to "read()" */
	int		packets_decoded = 0;	/* return the number of packets that are decoded */
	int		packet_size;		/* size of the currently being parsed data packet -- adjusted as new information is determined */

	uint32_t	message_length;		/* for string-type messages */
	char		*message;		/* for string-type messages */
	int		padded_message_size;	/* the complete length of the message portion of the packet (or does it include header too?) */

	double		vrpntime;		/* time stamp of the incoming VRPN data */
	VRPNdevice	*vrpn_dev;		/* pointer to a device in the VRPN device list */
	/*******************************************************/
	/*** read available data into available buffer space ***/
	do {
		read_result = (ssize_t)read(aux->vrpn_socket, &aux->buf[aux->eobuf_pos], BUFSIZE - aux->eobuf_pos);

		/* NOTE: if read_result == 0, the socket was closed from the other side.  This condition is handled at the end of the function */
		if (read_result > 0) {
			aux->eobuf_pos += read_result;
			/* debug statement here */
		}
#if 1
		if (read_result < -1) {
			/* we don't care about a -1 return value */
			/* error ? */
			vrErrPrintf("read_result = %d, errno = %d\n", read_result, errno);
			perror("socket read:");
		}
#endif
	} while (read_result > 0 && aux->eobuf_pos < BUFSIZE);

	/*******************************************/
	/*** decode data contained in the buffer ***/

	/* decode until no more complete messages are contained in the buffer */
	packet_size = HEADER_SIZE + 8;	/* There at least must be data for the first header */  /* NOTE: in reality, we'll always need the first number as well */
	while (aux->eobuf_pos >= packet_size) {
		/* point the VRPN interpretation structure (aka "header") to the beginning of the buffer */
		VRPNdata = *(VRPNheader *)aux->buf;

		/* interpret the header */
#if __BYTE_ORDER == __LITTLE_ENDIAN
		vrEndianSwap4(&VRPNdata.message_size);
		vrEndianSwap4(&VRPNdata.time_seconds);
		vrEndianSwap4(&VRPNdata.time_useconds);
		vrEndianSwap4(&VRPNdata.code);
		vrEndianSwap4(&VRPNdata.message_type);
#endif

		/* now make sure the entire packet is in the buffer */
		padded_message_size = VRPNdata.message_size;
		padded_message_size = (padded_message_size+7)&(~7);	/* use bit-wise operation to raise size to next 8-byte number */
		if (aux->eobuf_pos < padded_message_size) {
			/* debug statement here */
			vrDbgPrintf("************** YO: need to drop out of the while loop here! *********\n");
			vrDbgPrintf("message_size = %d, padded size = %d, bytes in the buffer = %d\n", VRPNdata.message_size, padded_message_size, aux->eobuf_pos);
			packet_size = padded_message_size;	/* skip the complete packet */
			continue;	/* jump out of the loop if we don't have enough data */
		}

		/* now interpret the data in the message container */
		switch (VRPNdata.message_type) {
		case SENDER_DESCRIPTION:
			vrDbgPrintfN(VRPN_DBGLVL, "SENDER_DESCRIPTION\n");

			/* extract the string message -- starting with the size */
			packet_size = HEADER_SIZE + sizeof(message_length);		/* NOTE: this isn't really necessary */
			memcpy(&message_length, &aux->buf[HEADER_SIZE], sizeof(message_length));
#if __BYTE_ORDER == __LITTLE_ENDIAN
			vrEndianSwap4(&message_length);
#endif

			/* now calculate the overall packet size */
			packet_size = HEADER_SIZE + sizeof(message_length) + message_length;
			packet_size = (packet_size+7)&(~7);	/* use bit-wise operation to raise size to next 8-byte number */
			if (aux->eobuf_pos < packet_size) {
				/* debug statement here */
				/* 01/12/13 NOTE: we should now never get here since we did a check before the switch */
				vrErrPrintf("\nNot enough data in the buffer\n\n\n", packet_size);
				vrErrPrintf("************** SHOULDN'T GET HERE! need to drop out of the while loop here! *********\n");
				continue;	/* jump out of the loop if we don't have enough data */
			}

			/* store the name of the device */
			message = &aux->buf[HEADER_SIZE + sizeof(message_length)];
			if (VRPNdata.code < MAX_DEVICES) {
				strncpy(aux->device[VRPNdata.code+1].name, message, sizeof(aux->device[0].name));
				vrDbgPrintfN(VRPN_DBGLVL, "Device number (%d) has the name '%s'.\n", (int)(VRPNdata.code), message);
				aux->num_devices++;
				/* TODO: do a check here to see whether we've already initialized the FreeVR inputs and if so print a warning */
			} else {
				vrDbgPrintfN(AALWAYS_DBGLVL, "Device number (%d) exceeds maximum (%d) -- '%s'.\n", (int)(VRPNdata.code), MAX_DEVICES, message);
			}
			vrDbgPrintfN(VRPN_DBGLVL, "Sender: bytes = %d, mes-size = %u, time = %u:%06u, code = %u, mes-type = %d, mes-len = %u, message = '%s'\n", aux->eobuf_pos, VRPNdata.message_size, VRPNdata.time_seconds, VRPNdata.time_useconds, VRPNdata.code, VRPNdata.message_type, message_length, message);
			break;

		case TYPE_DESCRIPTION:
			vrDbgPrintfN(VRPN_DBGLVL, "TYPE_DESCRIPTION\n");

			/* extract the string message -- starting with the size */
			packet_size = HEADER_SIZE + sizeof(message_length);		/* NOTE: this isn't really necessary */
			memcpy(&message_length, &aux->buf[HEADER_SIZE], sizeof(message_length));
#if __BYTE_ORDER == __LITTLE_ENDIAN
			vrEndianSwap4(&message_length);
#endif

			/* now calculate the overall packet size */
			packet_size = HEADER_SIZE + sizeof(message_length) + message_length;
			packet_size = (packet_size+7)&(~7);	/* use bit-wise operation to raise size to next 8-byte number */
			if (aux->eobuf_pos < packet_size) {
				/* debug statement here */
				/* 01/12/13 NOTE: we should now never get here since we did a check before the switch */
				vrErrPrintf("\nNot enough data in the buffer\n\n\n", packet_size);
				vrErrPrintf("************** SHOULDN'T GET HERE! need to drop out of the while loop here! *********\n");
				continue;	/* jump out of the loop if we don't have enough data */
			}

			/* now do something with the data */
			if (!strncmp(message, "vrpn_Tracker Pos_Quat", message_length)) {
				aux->MessageSensorPos = VRPNdata.code;
				vrDbgPrintfN(VRPN_DBGLVL, "assigned MessageSensorPos to %d\n", aux->MessageSensorPos);
			} else if (!strncmp(message, "vrpn_Tracker Velocity", message_length)) {
				aux->MessageSensorVel = VRPNdata.code;
				vrDbgPrintfN(VRPN_DBGLVL, "assigned MessageSensorVel to %d\n", aux->MessageSensorVel);
			} else if (!strncmp(message, "vrpn_Tracker Acceleration", message_length)) {
				aux->MessageSensorAcc = VRPNdata.code;
				vrDbgPrintfN(VRPN_DBGLVL, "assigned MessageSensorAcc to %d\n", aux->MessageSensorAcc);
			} else if (!strncmp(message, "vrpn_Button Change", message_length)) {
				aux->MessageButton = VRPNdata.code;
				vrDbgPrintfN(VRPN_DBGLVL, "assigned MessageButton to %d\n", aux->MessageButton);
			} else if (!strncmp(message, "vrpn_Analog Channel", message_length)) {
				aux->MessageValuator = VRPNdata.code;
				vrDbgPrintfN(VRPN_DBGLVL, "assigned MessageValuator to %d\n", aux->MessageValuator);
			}
			if ((int32_t)(VRPNdata.code) < MAX_CODES) {
				if ((int32_t)(VRPNdata.code) > aux->biggest_code)
					aux->biggest_code = (int32_t)(VRPNdata.code);
				else	vrDbgPrintf("Warning: duplicate message code value received: %u\n", VRPNdata.code);
				strncpy(aux->message_codes[VRPNdata.code], message, sizeof(aux->message_codes[0]));
			}
			vrDbgPrintfN(VRPN_DBGLVL, "Type: bytes = %d, mes-size = %u, time = %u:%06u, code = %u, mes-type = %d, mes-len = %u, message = '%s'\n", aux->eobuf_pos, VRPNdata.message_size, VRPNdata.time_seconds, VRPNdata.time_useconds, VRPNdata.code, VRPNdata.message_type, message_length, message);
			break;

		case UDP_DESCRIPTION:
			vrDbgPrintfN(VRPN_DBGLVL, RED_TEXT "UDP_DESCRIPTION -- not parsed! (code = %u)\n" NORM_TEXT, VRPNdata.code);
			packet_size = padded_message_size;	/* skip the complete packet */
			break;

		case LOG_DESCRIPTION:
			vrDbgPrintfN(VRPN_DBGLVL, RED_TEXT "LOG_DESCRIPTION -- not parsed! (code = %u)\n" NORM_TEXT, VRPNdata.code);
			packet_size = padded_message_size;	/* skip the complete packet */
			break;

		case DISCONNECT_MESSAGE:
			vrDbgPrintfN(VRPN_DBGLVL, RED_TEXT "DISCONNECT_MESSAGE -- not parsed! (code = %u)\n" NORM_TEXT, VRPNdata.code);
			packet_size = padded_message_size;	/* skip the complete packet */
			break;

		default:
			/************************************************************************/
			/* The default message is that we have some actual input data to parse. */
			/************************************************************************/

			/******************************/
			/*** Tracker Position Data? ***/
			if (VRPNdata.message_type == aux->MessageSensorPos) {
				/* tracking data = 64bit sensor number, 3*64 bit float location, 4*64bit float orientation (64 bytes) */
				int		count;
				uint64_t	sensor;		/* the 8-byte (64bit) sensor value in the VRPN data packet */
				int32_t		int_sensor;	/* the unsigned 64 bit sensor value wreaks havoc with array access */

				/* redetermine the packet size now that we know the type of data */
				packet_size = HEADER_SIZE + sizeof(sensor) + 7 * sizeof(vrpn_dev->posquat[0][0]);
				packet_size = (packet_size+7)&(~7);	/* use bit-wise operation to raise size to next 8-byte number */

				/* now determine whether there is enough data in the buffer! */
				if (aux->eobuf_pos < packet_size) {
					/* debug statement here */
					/* 01/12/13 NOTE: we should now never get here since we did a check before the switch */
					vrErrPrintf("\nNot enough data in the buffer\n\n\n", packet_size);
					vrErrPrintf("************** SHOULDN'T GET HERE! need to drop out of the while loop here! *********\n");
					continue;	/* jump out of the loop if we don't have enough data */
				}


				/* now parse the data! */
				memmove(&sensor, &aux->buf[HEADER_SIZE], sizeof(sensor));
				vrpntime = VRPNdata.time_seconds + (VRPNdata.time_useconds * 0.000001);
				vrpn_dev = &(aux->device[VRPNdata.code+1]);
#if __BYTE_ORDER == __LITTLE_ENDIAN
				vrEndianSwap8(&sensor);
#endif
				int_sensor = (int32_t)sensor;		/* convert to an integer to avoid gcc array indexing bug */
				if (int_sensor < MAX_6SENSORS) {
					vrpn_dev->ever_reported_posquat[int_sensor] = 1u;
					vrpn_dev->newly_reported_posquat[int_sensor] = 1u;
					for (count = 0; count < 7; count++) {
						memmove(&(vrpn_dev->posquat[int_sensor][count]), &aux->buf[HEADER_SIZE + sizeof(sensor) + (count * sizeof(vrpn_dev->posquat[0][0]))], sizeof(vrpn_dev->posquat[0][0]));
#if __BYTE_ORDER == __LITTLE_ENDIAN
						vrEndianSwap8(&(vrpn_dev->posquat[int_sensor][count]));
#endif
					}
					vrDbgPrintfN(VRPN_DBGLVL, BOLD_TEXT "\rTime = %0.6f" NORM_TEXT " -- PosQuat data: device = %u sensor = %d, xyz = %.3f %.3f %.3f, quat = %.3f %.3f %.3f %.3f", vrpntime, VRPNdata.code, int_sensor, vrpn_dev->posquat[int_sensor][0], vrpn_dev->posquat[int_sensor][1], vrpn_dev->posquat[int_sensor][2], vrpn_dev->posquat[int_sensor][3], vrpn_dev->posquat[int_sensor][4], vrpn_dev->posquat[int_sensor][5], vrpn_dev->posquat[int_sensor][6], vrpn_dev->posquat[int_sensor][7]);
				} else {
					vrDbgPrintf("Number of 6-sensor (%d) exceeds maximum (%d).\n", int_sensor+1, MAX_6SENSORS);
				}

			}
			/**********************/
			/*** Velocity Data? ***/
			else if (VRPNdata.message_type == aux->MessageSensorVel) {
				/* tracking data = 64bit sensor number, 3*64 bit float location vel, 4*64bit float orientation vel + one more unknown double! (72 bytes) */
				int		count;
				uint64_t	sensor;
				double		velquat[7];
				double		unknown_quantity;

				/* redetermine the packet size now that we know the type of data */
				packet_size = HEADER_SIZE + sizeof(sensor) + 7 * sizeof(velquat[0]) + sizeof(unknown_quantity);
				packet_size = (packet_size+7)&(~7);	/* use bit-wise operation to raise size to next 8-byte number */

				/* now determine whether there is enough data in the buffer! */
				if (aux->eobuf_pos < packet_size) {
					/* debug statement here */
					/* 01/12/13 NOTE: we should now never get here since we did a check before the switch */
					vrErrPrintf("\nNot enough data in the buffer\n\n\n", packet_size);
					vrErrPrintf("************** SHOULDN'T GET HERE! need to drop out of the while loop here! *********\n");
					continue;	/* jump out of the loop if we don't have enough data */
				}


				/* now parse the data! */
				memmove(&sensor, &aux->buf[HEADER_SIZE], sizeof(sensor));
				memmove(&unknown_quantity, &aux->buf[packet_size - sizeof(unknown_quantity)], sizeof(unknown_quantity));
#if __BYTE_ORDER == __LITTLE_ENDIAN
				vrEndianSwap8(&sensor);
				vrEndianSwap8(&unknown_quantity);
#endif
				for (count = 0; count < 7; count++) {
					memmove(&velquat[count], &aux->buf[HEADER_SIZE + sizeof(sensor) + (count * sizeof(velquat[0]))], sizeof(velquat[0]));
#if __BYTE_ORDER == __LITTLE_ENDIAN
					vrEndianSwap8(&velquat[count]);
#endif
				}

				vrDbgPrintfN(VRPN_DBGLVL, BOLD_TEXT "\nTime = %u:%06u" NORM_TEXT " -- VelQuat data: device = %u sensor = %d, xyz = %.3f %.3f %.3f, quat = %.3f %.3f %.3f %.3f, unknown = %.3f\n", VRPNdata.time_seconds, VRPNdata.time_useconds, VRPNdata.code, sensor, velquat[0], velquat[1], velquat[2], velquat[3], velquat[4], velquat[5], velquat[6], velquat[7], unknown_quantity);

			}
			/**************************/
			/*** Acceleration Data? ***/
			else if (VRPNdata.message_type == aux->MessageSensorAcc) {
				packet_size = HEADER_SIZE + 72;
			}
			/**********************/
			/*** Valuator Data? ***/
			else if (VRPNdata.message_type == aux->MessageValuator) {
				/* valuator data = 64bit number of channels, num-channels * 64bit values */
				int		count;
				double		num_valuators;		/* Oddly this is sent as a double precision float */
				double		valuators[MAX_VALUATORS];
				packet_size = HEADER_SIZE + sizeof(num_valuators);
				memmove(&num_valuators, &aux->buf[HEADER_SIZE], sizeof(num_valuators));
#if __BYTE_ORDER == __LITTLE_ENDIAN
				vrEndianSwap8(&num_valuators);
#endif
				packet_size = HEADER_SIZE + sizeof(num_valuators) + sizeof(valuators[0]) * num_valuators;
				/* now determine whether there is enough data in the buffer! */
				if (aux->eobuf_pos < packet_size) {
					/* debug statement here */
					/* 01/12/13 NOTE: we should now never get here since we did a check before the switch */
					vrErrPrintf("\nNot enough data in the buffer\n\n\n", packet_size);
					vrErrPrintf("************** SHOULDN'T GET HERE! need to drop out of the while loop here! *********\n");
					continue;	/* jump out of the loop if we don't have enough data */
				}

				vrpntime = VRPNdata.time_seconds + (VRPNdata.time_useconds * 0.000001);
				vrpn_dev = &(aux->device[VRPNdata.code+1]);
				if (num_valuators > MAX_VALUATORS) {
					vrDbgPrintf("Number of valuators available (%d) exceeds maximum (%d).\n", ((int)num_valuators), MAX_VALUATORS);
					num_valuators = MAX_VALUATORS;		/* cap number to maximum allowable */
				}

				vrDbgPrintfN(VRPN_DBGLVL, BOLD_TEXT "\nTime = %u:%06u" NORM_TEXT " -- (device = %u) Valuators (%d): ", VRPNdata.time_seconds, VRPNdata.time_useconds, VRPNdata.code, (int)num_valuators);
				for (count = 0; count < num_valuators && count < num_valuators; count++) {
					memmove(&valuators[count], &aux->buf[HEADER_SIZE + sizeof(num_valuators) + (count * sizeof(valuators[0]))], sizeof(valuators[0]));
#if __BYTE_ORDER == __LITTLE_ENDIAN
					vrEndianSwap8(&valuators[count]);
#endif
					vrpn_dev->analog[count] = valuators[count];
					vrpn_dev->ever_reported_analog[count] = 1u;
					vrpn_dev->newly_reported_analog[count] = 1u;
					vrpn_dev->analog_time[count] = vrpntime;

					vrDbgPrintfN(VRPN_DBGLVL, "% lf ", valuators[count]);
				}
				vrDbgPrintfN(VRPN_DBGLVL, "\n");
			}
			/********************/
			/*** Button Data? ***/
			else if (VRPNdata.message_type == aux->MessageButton) {
				/* button change data = 32bit button number, 32bit button value (8 bytes) */
				uint32_t	button_number;
				uint32_t	button_value;

				packet_size = HEADER_SIZE + 8;
				if (aux->eobuf_pos < packet_size) {		/* NOTE: we won't need this once I assume there's at least 8 bytes after the header */
					/* debug statement here */
					/* 01/12/13 NOTE: we should now never get here since we did a check before the switch */
					vrErrPrintf("\nNot enough data in the buffer\n\n\n", packet_size);
					vrErrPrintf("************** SHOULDN'T GET HERE! need to drop out of the while loop here! *********\n");
					continue;	/* jump out of the loop if we don't have enough data */
				}
				memmove(&button_number, &aux->buf[HEADER_SIZE], sizeof(button_number));
				memmove(&button_value, &aux->buf[HEADER_SIZE + sizeof(button_number)], sizeof(button_value));
#if __BYTE_ORDER == __LITTLE_ENDIAN
				vrEndianSwap4(&button_number);
				vrEndianSwap4(&button_value);
#endif
				vrpntime = VRPNdata.time_seconds + (VRPNdata.time_useconds * 0.000001);
				vrpn_dev = &(aux->device[VRPNdata.code+1]);

				if (button_number < MAX_BUTTONS) {
					vrpn_dev->button[button_number] = button_value;
					vrpn_dev->ever_reported_button[button_number] = 1u;
					vrpn_dev->newly_reported_button[button_number] = 1u;
					vrpn_dev->button_time[button_number] = vrpntime;
				} else {
					vrDbgPrintf("Number of button (%d) exceeds maximum (%d).\n", button_number+1, MAX_BUTTONS);
				}

				vrDbgPrintfN(VRPN_DBGLVL, BOLD_TEXT "\nTime = %u:%06u" NORM_TEXT " -- (device = %u) Button %d is now %d\n", VRPNdata.time_seconds, VRPNdata.time_useconds, VRPNdata.code, button_number, button_value);
			}
#if 1
			/************************/
			/*** Unknown Datatype ***/
			else {
				memcpy(&message_length, &aux->buf[HEADER_SIZE], sizeof(message_length));
#if __BYTE_ORDER == __LITTLE_ENDIAN
				vrEndianSwap4(&message_length);
#endif
				message = &aux->buf[HEADER_SIZE + sizeof(message_length)];
				packet_size = padded_message_size;	/* skip the complete packet */

				vrDbgPrintf("\n -- got a different type of data -- %d (code = %u)\n\n", VRPNdata.message_type, VRPNdata.code);
				vrDbgPrintf("bytes = %d, mes-size = %u, time = %u:%06u, code = %u, mes-type = %d, string? = '%s'\n", aux->eobuf_pos, VRPNdata.message_size, VRPNdata.time_seconds, VRPNdata.time_useconds, VRPNdata.code, VRPNdata.message_type, message);
				int		count;
				/* ... print out all the bytes */
			}
#endif
		}

		/* do a sanity check!  (the reported message size should match the actual message size) */
		if (padded_message_size != packet_size) {
			vrDbgPrintf(RED_TEXT "SANITY FAIL: padded_message_size = %d, packet_size = %d -- bytes = %d, mes-size = %u, time = %u:%06u, code = %u, mes-type = %d\n" NORM_TEXT, padded_message_size, packet_size, aux->eobuf_pos, VRPNdata.message_size, VRPNdata.time_seconds, VRPNdata.time_useconds, VRPNdata.code, VRPNdata.message_type);
			abort();
		}

		/* now shift the buffer by the message size (padded to 8 byte boundaries) */
		memmove(aux->buf, &aux->buf[packet_size], (aux->eobuf_pos - packet_size));	/* shift the data */
		aux->eobuf_pos -= packet_size;
		packets_decoded++;

		/* reset the packet size to the minimal size required for parsing */
		packet_size = HEADER_SIZE;	/* There at least must be data for the first header */  /* NOTE: in reality, we'll always need the first number as well */
	}

	/* return -1 if the socket connection is closed */
	if (read_result == 0) {
		vrDbgPrintf("\n\n_VRPNReadInput(): socket connection closed -- %d packets decoded in this pass\n", packets_decoded);
		return (-1);
	}
	return (packets_decoded);
#endif /* } !VRPN_LIB */
}


#ifdef VRPN_LIB /* { */
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
		vrErrPrintf("_VrpnHandleButton(): invalid button number from VRPN (%d)\n", but_vrpn.button);
		return;
	}

	aux = userdata->aux;
	vrpn_device_num = userdata->num;

	input = aux->button_inputs[vrpn_device_num][but_vrpn.button];

	vrDbgPrintfN(INPUT_DBGLVL,
		"_VrpnHandleButton(): device = %d, button = %d, state = %d, input = %#p\n",
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
			"_VrpnHandleAnalog(): device = %d, analog channel = %d, value = %f, input = %#p\n",
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
		vrErrPrintf("_VrpnHandleTracker(): invalid sensor number from VRPN (%d)\n", tracker_vrpn.sensor);
		return;
	}

	aux = userdata->aux;
	vrpn_device_num = userdata->num;

	input = aux->tracker_inputs[vrpn_device_num][tracker_vrpn.sensor];

	vrDbgPrintfN(INPUT_DBGLVL,
		"_VrpnHandleTracker(): device = %d, sensor# = %d, loc = (%5.3f %5.3f %5.3f), input = %#p\n",
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
	/* raise value of num_sensors when info from a larger sensor number is received */
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

#endif /* } VRPN_LIB */

/******************************************************/
/* NOTE: _VrpnInitializeDevice() is called by _VrpnOpenFunction() after the socket */
/*   connection has been made and the version number obtained.                     */
static int _VrpnInitializeDevice(_VrpnPrivateInfo *aux)
{
	int		device_count;		/* for looping through list of each device type */
	int		count;			/* for looping over inputs of each type */
#ifdef VRPN_LIB /* { */
	_VrpnUserdata	*userdata;		/* struct for passing info to the VRPN library callback */

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
#  if 0 /* attempt to test for the existence of the function */
		vrPrintf("About to create new vrpn Button device '%s' (first check for function existance)\n", aux->button_names[device_count]);
		if ((void *)vrpn_Button_Remote == NULL)
			vrPrintf(RED_TEXT "cannot find function 'vrpn_Button_Remote'\n" NORM_TEXT);
#  endif
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
#else /* } VRPN_LIB { */
	int	retval;		/* value returned by _VrpnReadInput() */

	/********************/
	/* clear the palate */
	retval = _VrpnReadInput(aux);		/* NOTE: the first reading of VRPN data will often contain all the setup information */
	/* if we didn't get any data, try again for 10 times */
	for (count = 0; count < 10 && retval <= 0; count++) {
		vrDbgPrintfN(ALWAYS_DBGLVL, "_VrpnInitializeDevice(): " RED_TEXT "VRPN Server is not yet sending data.  Trying again...\n" NORM_TEXT);
		vrSleep(100000);
		retval = _VrpnReadInput(aux);
		if (retval > 0)
			vrDbgPrintfN(ALWAYS_DBGLVL, "_VrpnInitializeDevice(): " RED_TEXT "... Success!\n" NORM_TEXT);
	}

#ifdef FREEVR /* { */
	/* set each VRPN input's device pointer to device matching device named in string */
	if (aux->num_devices > 0) {
		/* buttons */
		for (count = 0; count < aux->num_buttons; count++) {
			/* find an existing device with the matching name */
			for (device_count = 0; device_count < aux->num_devices; device_count++) {
				if (!strcmp(aux->button_map_devicename[count], aux->device[device_count].name)) {
					aux->button_map_device[count] = &(aux->device[device_count]);
					aux->button_map_device[count]->mapped_button[aux->button_map_button[count]] = 1;
				}
			}
			vrDbgPrintfN(VRPN_DBGLVL, "_VrpnInitializeDevice(): Just mapped button[%d](%s) to %p\n", count, aux->button_map_devicename[count], aux->button_map_device[count]);
			if (aux->button_map_device[count] == NULL) {
				vrDbgPrintfN(ALWAYS_DBGLVL, "_VrpnInitializeDevice(): " RED_TEXT BOLD_TEXT "Alert! VRPN Server is not sending the requested device '%s' for button input %d.\n" NORM_TEXT, aux->button_map_devicename[count], count);
			}
		}

		/* valuators */
		for (count = 0; count < aux->num_valuators; count++) {
			/* find an existing device with the matching name */
			for (device_count = 0; device_count < aux->num_devices; device_count++) {
				if (!strcmp(aux->valuator_map_devicename[count], aux->device[device_count].name)) {
					aux->valuator_map_device[count] = &(aux->device[device_count]);
					aux->valuator_map_device[count]->mapped_analog[aux->valuator_map_valuator[count]] = 1;
				}
			}
			vrDbgPrintfN(VRPN_DBGLVL, "_VrpnInitializeDevice(): Just mapped valuator[%d](%s) to %p\n", count, aux->valuator_map_devicename[count], aux->valuator_map_device[count]);
			if (aux->valuator_map_device[count] == NULL) {
				vrDbgPrintfN(ALWAYS_DBGLVL, "_VrpnInitializeDevice(): " RED_TEXT BOLD_TEXT "Alert! VRPN Server is not sending the requested device '%s' for valuator input %d.\n" NORM_TEXT, aux->valuator_map_devicename[count], count);
			}
		}

		/* 6sensors */
		for (count = 0; count < aux->num_6sensors; count++) {
			/* find an existing device with the matching name */
			for (device_count = 0; device_count < aux->num_devices; device_count++) {
				if (!strcmp(aux->tracker_map_devicename[count], aux->device[device_count].name)) {
					aux->tracker_map_device[count] = &(aux->device[device_count]);
					aux->tracker_map_device[count]->mapped_tracker[aux->tracker_map_tracker[count]] = 1;
				}
			}
			vrDbgPrintfN(VRPN_DBGLVL, "_VrpnInitializeDevice(): Just mapped tracker[%d](%s) to %p\n", count, aux->tracker_map_devicename[count], aux->tracker_map_device[count]);
			if (aux->tracker_map_device[count] == NULL) {
				vrDbgPrintfN(ALWAYS_DBGLVL, "_VrpnInitializeDevice(): " RED_TEXT BOLD_TEXT "Alert! VRPN Server is not sending the requested device '%s' for tracker input %d.\n" NORM_TEXT, aux->tracker_map_devicename[count], count);
			}
		}

		aux->mapped = 1;
	}

	/* find the first simulator-sensor */
	for (aux->active_sim6sensor = 0
		; /* conditional */							/* keep looping if: */
		   (aux->active_sim6sensor < aux->num_6sensors)				/*   still within the list bounds */
		&& (aux->tracker_inputs[aux->active_sim6sensor] != NULL)			/*   tracker does not exist! */
		&& (aux->tracker_map_device[aux->active_sim6sensor] != &(aux->device[0]))	/*   tracker is not from device[0] (ie. simulator) */
		; /* increment */
		aux->active_sim6sensor++)
#if 0
vrPrintf("!!!!!! for: active_sim6sensor=%d, num_6=%d, aux->tracker_inputs[%d]=%p, aux->tracker_map_device[aux->active_sim6sensor]->name='%s'\n", aux->active_sim6sensor, aux->num_6sensors, aux->active_sim6sensor, aux->tracker_inputs[aux->active_sim6sensor], aux->tracker_map_device[aux->active_sim6sensor]->name)
#endif
		; /* no-op -- the looping is the reward */
	if (aux->active_sim6sensor >= aux->num_6sensors) {
		vrDbgPrintfN(ALWAYS_DBGLVL, "_VrpnInitializeDevice(): " RED_TEXT "Warning, there is no simulated 6-sensor (not a problem if not planning to use one).\n" NORM_TEXT);
	} else {
		vrAssign6sensorActiveValue(((vr6sensor *)aux->tracker_inputs[aux->active_sim6sensor]), 1);
		vrDbgPrintfN(SELFCTRL_DBGLVL, "VRPN Control: 6sensor[%d] now active.\n", aux->active_sim6sensor);
	}
#endif /* } FREEVR */

	if (vrDbgDo(VRPN_DBGLVL)) {
		_VrpnPrintStruct(stdout, aux, verbose);
	}

	/**********************************/
	/* request version and param info */
	/* TODO: this */

	/********************************/
	/* send some device setup codes */
	/* TODO: this (if necessary) */
#if 0
        /* At this point for VRPN servers compiled with the ??? option, we would     */
	/*   need to inform the server what tracker/input data we are interested in. */
	/*   But typically VRPN servers are not configured that way and just send    */
	/*   all available data.                                                     */
        _VRPNSendSenderMessage(fd_socket, 0, "head", RELIABLE);
        _VRPNSendSenderMessage(fd_socket, 1, "wand", RELIABLE);
#endif

#if 0
	/************************/
	/* read some input data */
	_VrpnReadInput(aux);
#endif
#endif /* } */

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


	/************************************/
	/***  FreeVR NON public routines  ***/
	/************************************/


/*********************************************************/
static void _VrpnParseArgs(_VrpnPrivateInfo *aux, char *args)
{
	float	volume_values[6];		/* for reading the working volume array */
	float	scale_value = -1.0;		/* for reading one of the scaling factor values */

	/* In the case of no arguments, just return */
	if (args == NULL)
		return;

	/******************************************/
	/** Argument format: "host" "=" hostname **/
	/******************************************/
	vrArgParseString(args, "host", &(aux->vrpn_host));

	/**********************************************/
	/** Argument format: "hostname" "=" hostname **/
	/**********************************************/
	vrArgParseString(args, "hostname", &(aux->vrpn_host));

	/**************************************/
	/** Argument format: "port" "=" file **/
	/**************************************/
	vrArgParseInteger(args, "port", &(aux->vrpn_port));

#if 1 /* not sure whether we'll want this */
	/********************************************/
	/** Argument format: "valScale" "=" number **/
	/********************************************/
	vrArgParseFloat(args, "valscale", &(aux->scale_valuator));
#endif
#if 0 /* old method */
	/********************************************/
	/** Argument format: "valScale" "=" number **/
	/********************************************/
	if (vrArgParseFloat(args, "valscale", &scale_value)) {
		aux->scale_valuator = scale_value * VALUATOR_SENSITIVITY;
	}
#endif

	/**********************************************/
	/** Argument format: "transScale" "=" number **/
	/**********************************************/
	if (vrArgParseFloat(args, "transscale", &scale_value)) {
		aux->scale_trans = scale_value;
	}

	/*****************************************/
	/** Argument format: "scale" "=" number **/ /* (redundant with above) */
	/*****************************************/
	if (vrArgParseFloat(args, "scale", &scale_value)) {
		aux->scale_trans = scale_value;
	}

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


	/** TODO: other arguments to parse go here **/
}


/************************************************************/
static void _VrpnGetData(vrInputDevice *devinfo)
{
	_VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->aux_data;
	int			count;			/* for looping over things */
	VRPNdevice		*vrpn_dev;		/* pointer to a device in the VRPN device list */
	int			input;
	int			vrpn_map;		/* local storage of the mapping code from FreeVR input number to VRPN input number */
	float			valuator_value;		/* holder for compositing a value together from components (e.g. sign & value) */

	/*******************/
	/* gather the data */
	_VrpnReadInput(aux);

	/* TODO: perhaps check to see whether the input structure has been initialized yet, and if not, call it AGAIN here */
	/* If the input mapping hasn't taken place yet, try again */
	if (!aux->mapped) {
		vrDbgPrintfN(1, "_VrpnGetData(): trying again to initialize the input mapping!\n");
		vrSleep(10000);				/* allow a short amount of time to pass */
		_VrpnInitializeDevice(aux);
	}

	/* If still not mapped, return so other inputs can do some work */
	if (!aux->mapped) {
		vrDbgPrintfN(1, "_VrpnGetData(): VRPN inputs not yet mapped, skipping!\n");
		return;
	}

	/****************************************/
	/* Handle button (2-switch) information */
#if 0 /* really we shouldn't be here until the device is ready, so I may just get rid of this bit */
	if (aux->button_map_button == NULL) {
		vrDbgPrintfN(1, "_VrpnGetData(): button_map_button not yet initialized!\n");
		vrSleep(10000);				/* slow things down since we're not ready to go yet anyway */
		return;
	}
#endif
	for (count = 0; count < aux->num_buttons; count++) {	/* NOTE: was devinfo->num_2ways, but that doesn't count self-controls! */
		vrpn_map = aux->button_map_button[count];
		vrpn_dev = aux->button_map_device[count];

		/* verify we have a good button input */
		if ((aux->button_inputs[count] != NULL) && (vrpn_dev != NULL)) {

			/* only handle buttons with a change of state (ie. new data from the VRPN server) */
			if (vrpn_dev->newly_reported_button[vrpn_map]) {
#if 0
printf("button change on device '%s', num %d\n", vrpn_dev->name, vrpn_map);
#endif
				switch (aux->button_inputs[count]->input_type) {
				case VRINPUT_BINARY:
					vrAssign2switchValue((vr2switch *)(aux->button_inputs[count]), (vrpn_dev->button[vrpn_map] != 0));
					vrpn_dev->newly_reported_button[vrpn_map] = 0u;
#if 0
vrPrintf("Assignment: VRPN button %d maps to FreeVR button %d, max = %d, val = %d, prev = %d\n", vrpn_map, count, aux->num_dd_buttons, aux->incoming_buttons[vrpn_map], aux->incoming_buttons_prev[vrpn_map]);
#endif
					break;
				case VRINPUT_CONTROL:
					vrCallbackInvokeDynamic(((vrControl *)(aux->button_inputs[count]))->callback, 1, (vrpn_dev->button[vrpn_map] != 0));
					vrpn_dev->newly_reported_button[vrpn_map] = 0u;
					break;
				default:
					vrErrPrintf(RED_TEXT "_VruiDDGetData(): Unable to handle button inputs that aren't Binary or Control inputs\n" NORM_TEXT);
					break;
				}
			}
		}
	}

	/****************************************/
	/* Handle analog (valuator) information */
	for (count = 0; count < aux->num_valuators; count++) {
		vrpn_map = aux->valuator_map_valuator[count];
		vrpn_dev = aux->valuator_map_device[count];

		/* verify we have a good valuator input */
		if ((aux->valuator_inputs[count] != NULL) && (vrpn_dev != NULL)) {

			/* only handle valuators with a change of state (ie. new data from the VRPN server) */
			if (vrpn_dev->newly_reported_analog[vrpn_map]) {
#if 0
printf("valuator change on device '%s', num %d\n", vrpn_dev->name, vrpn_map);
#endif
				/* TODO: add aux->scale_valuator feature */
				valuator_value = vrpn_dev->analog[vrpn_map] * aux->scale_valuator * aux->valuator_sign[count];
				switch (aux->valuator_inputs[count]->input_type) {
				case VRINPUT_VALUATOR:
					vrAssignValuatorValue((vrValuator *)(aux->valuator_inputs[count]), valuator_value);
					vrpn_dev->newly_reported_analog[vrpn_map] = 0u;
#if 0
vrPrintf("Assignment: VRPN valuator %d maps to FreeVR valuator %d, max = %d, val = %d, prev = %d\n", vrpn_map, count, aux->num_dd_valuators, aux->incoming_valuators[vrpn_map], aux->incoming_valuators_prev[vrpn_map]);
#endif
					break;
				case VRINPUT_CONTROL:
					vrCallbackInvokeDynamic(((vrControl *)(aux->valuator_inputs[count]))->callback, 1, &valuator_value);
					vrpn_dev->newly_reported_analog[vrpn_map] = 0u;
					break;
				default:
					vrErrPrintf(RED_TEXT "_VruiDDGetData(): Unable to handle valuator inputs that aren't Binary or Control inputs\n" NORM_TEXT);
					break;
				}
			}
		}
	}

	/****************************************/
	/* Handle tracker (6sensor) information */
	for (count = 0; count < aux->num_6sensors; count++) {
		vrMatrix	new_position;
		vrVector	new_location;
		vrQuat		new_orientation;
		float		scale_trans = aux->scale_trans;

		vrpn_map = aux->tracker_map_tracker[count];
		vrpn_dev = aux->tracker_map_device[count];

		/* verify we have a good tracker and that there is new data! */
		if ((aux->tracker_inputs[count] != NULL) && (vrpn_dev != NULL) && vrpn_dev->newly_reported_posquat[vrpn_map]) {
			/* Copy the incoming tracker data into the FreeVR structure */
			new_location.v[VR_X] = vrpn_dev->posquat[vrpn_map][VR_X] * scale_trans;
			new_location.v[VR_Y] = vrpn_dev->posquat[vrpn_map][VR_Y] * scale_trans;
			new_location.v[VR_Z] = vrpn_dev->posquat[vrpn_map][VR_Z] * scale_trans;

			/* [07/01/2014] Is there a defacto standard coordinate system for VRPN?  The use of vrMatrixSetRotationFromQuatCR() below implies that we need to convert from Z-up to Y-up for FreeVR */
			new_orientation.v[VR_X] = vrpn_dev->posquat[vrpn_map][VR_QX];
			new_orientation.v[VR_Y] = vrpn_dev->posquat[vrpn_map][VR_QY];
			new_orientation.v[VR_Z] = vrpn_dev->posquat[vrpn_map][VR_QZ];
			new_orientation.v[VR_W] = vrpn_dev->posquat[vrpn_map][VR_QW];
#if 0
vrPrintf("assigning tracker %d to location %f %f %f\n", vrpn_map, new_location.v[VR_X], new_location.v[VR_Y], new_location.v[VR_Z]);
#endif

			vrMatrixSetIdentity(&new_position);
			vrMatrixSetTransFromVector(&new_position, &new_location);
#if 1	/* [03/18/2015: I've gone back to the original name, but in fact the functionality is what the QuatCR version did, because it seems that that was the correct one all along.] */
			vrMatrixSetRotationFromQuat(&new_position, &new_orientation);
#else
			vrMatrixSetRotationFromQuatCR(&new_position, &new_orientation);		/* [07/01/2014] I suspect that using this routine is unnecessary, and that it's here because it happens to cause a transition between Y-up and Z-up coordinate systems.  Probably should do an actual Quaternion rotation (or mangling -- see vr_output.vruidd.c) instead. */
#endif

			/* since 6-sensors generally change rapidly, there is little point */
			/*   and often moderate expense in comparing for changes from the  */
			/*   current value to the new value.                               */
			/* although perhaps watching the timestamp might be a useful way   */
			/*   to do this.                                                   */
			/* TODO: perhaps verify that this is indeed a VRINPUT_6SENSOR */
			if (vrpn_dev == &(aux->device[0])) {
				/* device[0] is the simulated 6-sensor device */
				if (!aux->sensor6_options.ignore_all && aux->sim6sensor_change) {
					vrAssign6sensorValueFromValuators(aux->tracker_inputs[count], aux->sim_values, &(aux->sensor6_options), -1 /* (ie. no change to oob flag) */);
					if ((aux->sim_values[VR_X] == 0)
					   && (aux->sim_values[VR_Y] == 0)
					   && (aux->sim_values[VR_Z] == 0)
					   && (aux->sim_values[VR_AZIM+3] == 0)
					   && (aux->sim_values[VR_ELEV+3] == 0)
					   && (aux->sim_values[VR_ROLL+3] == 0)) {
						aux->sim6sensor_change = 0;
						vrpn_dev->newly_reported_posquat[vrpn_map] = 0u;
					}
				}
			} else {
				/* all other devices are actually from the VRPN server */
				vrAssign6sensorValue(aux->tracker_inputs[count], &new_position, 0 /* TODO: timestamp: , vrpn_dev->posquat_time[vrpn_map] */);
				vrpn_dev->newly_reported_posquat[vrpn_map] = 0u;
			}

		}
	}

#if 0 /* this just prints all data -- for code development only { */
	/****************************************/
	/* Handle button (2-switch) information */
	for (count = 0; count < MAX_DEVICES; count++) {
		vrpn_dev = &(aux->device[count]);
		for (input = 0; input < MAX_BUTTONS; input++) {
			if (vrpn_dev->newly_reported_button[input]) {
				printf("dev[%s]->button[%d] = %d\n", vrpn_dev->name, input, vrpn_dev->button[input]);
				vrpn_dev->newly_reported_button[input] = 0u;
			}
		}
	}

	/****************************************/
	/* Handle analog (valuator) information */
	for (count = 0; count < MAX_DEVICES; count++) {
		int	print_flag = 0;		/* temporary */
		vrpn_dev = &(aux->device[count]);
		for (input = 0; input < MAX_VALUATORS; input++) {
			if (vrpn_dev->newly_reported_analog[input]) {
				printf("dev[%s]->val[%d] = % 6.3f  ", vrpn_dev->name, input, vrpn_dev->analog[input]);
				vrpn_dev->newly_reported_analog[input] = 0u;
				print_flag = 1;	/* temporary */
			}
		}
		if (print_flag)		/* temporary */
			printf("\n");	/* temporary */
	}

	/*****************************************/
	/* Handle tracker (6-sensor) information */
	for (count = 0; count < MAX_DEVICES; count++) {
		vrpn_dev = &(aux->device[count]);
		for (input = 0; input < MAX_6SENSORS; input++) {
			if (vrpn_dev->newly_reported_posquat[input]) {
				printf("dev[%s]->s%d x:%.3f y:%.3f z:%.3f  qx:%.3f qy:%.3f qz:%.3f qw:%.3f\n",
					vrpn_dev->name, input,
					vrpn_dev->posquat[input][VR_X], vrpn_dev->posquat[input][VR_Y], vrpn_dev->posquat[input][VR_Z],
					vrpn_dev->posquat[input][VR_QX], vrpn_dev->posquat[input][VR_QY], vrpn_dev->posquat[input][VR_QZ], vrpn_dev->posquat[input][VR_QW]);
				vrpn_dev->newly_reported_posquat[input] = 0u;
			}
		}
	}
#endif /* } */
}


	/****************************************************************/
	/*      Function(s) for parsing VRPN "input" declarations.      */
	/*                                                              */

/**************************************************************************/
/*  for example: input "2switch[left]" = "2switch(Joystick0:button[0])"; */
static vrInputMatch _VrpnButtonInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
        _VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->aux_data;
	int			button_num;		/* the current count of FreeVR button inputs */
	int			button_choice;		/* the chosen number for this button */

#if 0 /* old style configuration */
	char			vrpn_device[256];
	char			*device_end;
	int			vrpn_device_num;
#ifdef DONOTMOVETO_INITDEV
	_VrpnUserdata		*userdata;		/* struct for passing info to the callback */
#endif

	/* A comma separates the VRPN device name, and the number */
	/*   of the desired input from that device.               */
	device_end = strchr(dti->instance, ',');

	/* If no comma, then report an error in the input specification */
	/*   otherwise add this VRPN field as an input.                 */
	if (device_end != NULL) {
		/* copy just the name of the device into vrpn_device */
		strncpy(vrpn_device, dti->instance, device_end - dti->instance);
		vrpn_device[device_end - dti->instance] = '\0';

		/* the rest of the string should just be a number of the particular sensor */
		button_num = vrAtoI(dti->instance);

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

#  ifdef VRPN_LIB /* { */
			/* open the VRPN connection and set the incoming data callback (TODO: or do this in _VrpnInitializeDevice()) */
vrPrintf("About to create new vrpn Button device '%s'\n", vrpn_device);
			aux->vrpn_button[vrpn_device_num] = new vrpn_Button_Remote(vrpn_device);
vrPrintf("About to register Button change handler\n");
			aux->vrpn_button[vrpn_device_num]->register_change_handler((void *)userdata, (vrpn_BUTTONCHANGEHANDLER)_VrpnHandleButton);
#  else /* } { */
#  endif /* } */
vrPrintf("Okay, button now registered\n");
#endif
		}

		vrDbgPrintfN(1 /* TODO: make INPUT_DBGLVL */, "doing button-sensor device '%s' (button %d), %d\n", vrpn_device, vrpn_device_num, button_num);

		/* now assign given input to the existing or new button device */
		aux->button_inputs[vrpn_device_num][button_num] = (vr2switch *)input;

		/* TODO: make initial button assignment */
#if 0 /* still need to make sure the instance stuff is okay */
		vrAssign2switch(aux->button_inputs[button_num], strchr(dti->instance, ','));
#endif
		vrDbgPrintfN(INPUT_DBGLVL, "assigned 2switch event of value 0x%02x to input pointer = %#p)\n",
			button_num, aux->button_inputs[vrpn_device_num][button_num]);

	} else {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_VrpnButtonInput: Warning, invalid input specification (%s, %s).\n" NORM_TEXT,
			dti->type, dti->instance);
	}
#else
	/* select a button */
	button_num = aux->num_buttons;
	aux->num_buttons++;
	button_choice = vrAtoI(dti->instance);

	/* check the selected button */
	if (button_choice == -1) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_VrpnButtonInput(): Warning, button['%s'] did not match any known button\n", dti->instance);
		return VRINPUT_MATCH_UNABLE;	/* device match, but still unable to handle request */
	} else if (aux->button_inputs[button_num] != NULL) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_VrpnButtonInput(): Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);
	}

	/* make the mapping */
	aux->button_inputs[button_num] = (vr2switch *)input;
	aux->button_map_button[button_num] = button_choice;
	aux->button_map_devicename[button_num] = vrShmemStrDup(dti->device);
	vrDbgPrintfN(INPUT_DBGLVL, "_VrpnButtonInput(): assigned button event of value %d (VRPN button %d) to input pointer = %#p)\n",
		button_num, button_choice, aux->button_inputs[button_num]);
#endif
	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/**************************************************************************/
/* _VrpnValuatorInput() was copied from _VruiDDValuatorInput() */
static vrInputMatch _VrpnValuatorInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
        _VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->aux_data;
	int			valuator_num;		/* the current count of FreeVR valuator inputs */
	int			valuator_choice;	/* the chosen number for this valuator */

	/* The Vrpn method of handling input arrays was taken from how the VruiDD handler works. */

	/* select a valuator (and sign) */
	valuator_num = aux->num_valuators;
	aux->num_valuators++;
	if (dti->instance[0] == '-') {
		aux->valuator_sign[valuator_num] = -1.0;
		valuator_choice = vrAtoI(&(dti->instance[1]));
	} else {
		aux->valuator_sign[valuator_num] =  1.0;
		valuator_choice = vrAtoI(dti->instance);
	}

	/* check the selected valuator */
	if (valuator_choice == -1) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_VrpnValuatorInput(): Warning, valuator['%s'] did not match any known valuator\n", dti->instance);
		return VRINPUT_MATCH_UNABLE;	/* device match, but still unable to handle request */
	} else if (aux->valuator_inputs[valuator_num] != NULL) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_VrpnValuatorInput(): Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);
	}

	/* make the mapping */
	aux->valuator_inputs[valuator_num] = (vrValuator *)input;
	aux->valuator_map_valuator[valuator_num] = valuator_choice;
	aux->valuator_map_devicename[valuator_num] = vrShmemStrDup(dti->device);
	vrDbgPrintfN(INPUT_DBGLVL, "_VrpnValuatorInput(): assigned valuator event of value %d (Vrpn valuator %d) to input pointer = %#p)\n",
		valuator_num, valuator_choice, aux->valuator_inputs[valuator_num]);

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}
/**************************************************************************/
/* NOTE: _VrpnAnalogInput() is old and deprecated */
static int _VrpnAnalogInput(vrInputDevice *devinfo, vrGenericInput *input, char *device, char *type, char *instance)
{
        _VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->aux_data;
	int			valuator_num;
	char			vrpn_device[256];
	char			*device_end;
	int			vrpn_device_num;
#ifdef DONOTMOVETO_INITDEV
	_VrpnUserdata		*userdata;		/* struct for passing info to the callback */
#endif

#if 0 /* old style configuration */
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
#  ifdef VRPN_LIB /* { */

			/* open the VRPN connection and set the incoming data callback (TODO: or do this in _VrpnInitializeDevice()) */
vrPrintf("About to create new vrpn Analog device '%s'\n", vrpn_device);
			aux->vrpn_analog[vrpn_device_num] = new vrpn_Analog_Remote(vrpn_device);
vrPrintf("About to register analog change handler\n");
			aux->vrpn_analog[vrpn_device_num]->register_change_handler((void *)userdata, (vrpn_ANALOGCHANGEHANDLER)_VrpnHandleAnalog);
#  else /* } { */
#  endif /* } */
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
		vrDbgPrintfN(INPUT_DBGLVL, "assigned Valuator event of value 0x%02x to input pointer = %#p)\n",
			valuator_num, aux->valuator_inputs[vrpn_device_num][valuator_num]);

	} else {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_VrpnAnalogInput: Warning, invalid input specification (%s, %s).\n" NORM_TEXT,
			type, instance);
	}
#endif

	return 1;	/* input declaration match */
}


/**************************************************************************/
/* NOTE: this is an exact copy of the VruiDD version (except the name) */
static vrInputMatch _Vrpn6sensorInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
        _VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->aux_data;
	int			tracker_num;		/* the current count of FreeVR tracker inputs */
	int			tracker_choice;		/* the chosen number for this tracker */

	/* The Vrpn method of handling input arrays may be a combination of the way */
	/*   suggested by vr_input.skeleton.c as the FoB, MS & Fastrak input drivers. */

	/* select a tracker */
	tracker_num = aux->num_6sensors;
	aux->num_6sensors++;
	tracker_choice = vrAtoI(dti->instance);

	/* check the selected tracker */
	if (tracker_choice == -1) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_Vrpn6sensorInput(): Warning, tracker['%s'] did not match any known tracker\n", dti->instance);
		return VRINPUT_MATCH_UNABLE;	/* device match, but still unable to handle request */
	} else if (aux->tracker_inputs[tracker_num] != NULL) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_Vrpn6sensorInput(): Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);
	}

	/* make the mapping */
	aux->tracker_inputs[tracker_num] = (vr6sensor *)input;
	aux->tracker_map_tracker[tracker_num] = tracker_choice;
	if (!strcasecmp(dti->type, "sim6")) {
		/* force this to use the simulator device if the type implies it */
		aux->tracker_map_devicename[tracker_num] = vrShmemStrDup("simulator");
	} else {
		aux->tracker_map_devicename[tracker_num] = vrShmemStrDup(dti->device);
	}
	vrDbgPrintfN(INPUT_DBGLVL, "_Vrpn6sensorInput(): assigned tracker event of value %d (Vrpn tracker %d) to input pointer = %#p)\n",
		tracker_num, tracker_choice, aux->tracker_inputs[tracker_num]);

	/* the vrAssign6sensorR2Exform() function does the parsing of the next token */
	vrAssign6sensorR2Exform(aux->tracker_inputs[tracker_num], strchr(dti->instance, ','));

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}

#if 0 /* old style configuration */
/**************************************************************************/
static int _VrpnTrackerInput(vrInputDevice *devinfo, vrGenericInput *input, char *device, char *type, char *instance)
{
        _VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->aux_data;
	int			sensor_num;
	char			vrpn_device[256];
	char			*device_end;
	int			vrpn_device_num;
#ifdef DONOTMOVETO_INITDEV
	_VrpnUserdata		*userdata;		/* struct for passing info to the callback */
#endif

#if 0 /* old style configuration */
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

#  ifdef VRPN_LIB /* { */
			/* open the VRPN connection and set the incoming data callback (TODO: or do this in _VrpnInitializeDevice()) */
			aux->vrpn_tracker[vrpn_device_num] = new vrpn_Tracker_Remote(vrpn_device);
			aux->vrpn_tracker[vrpn_device_num]->register_change_handler((void *)userdata, (vrpn_TRACKERCHANGEHANDLER)_VrpnHandleTracker);
#  else /* } { */
#  endif /* } */
#endif
		}

		vrDbgPrintfN(1 /* TODO: make INPUT_DBGLVL */, "doing tracker-sensor device '%s' (tracker %d), %d\n", vrpn_device, vrpn_device_num, sensor_num);

		/* now assign given input to the existing or new tracker device */
		aux->tracker_inputs[vrpn_device_num][sensor_num] = (vr6sensor *)input;

		/* the vrAssign6sensorR2Exform() function does the parsing of the next token */
		vrAssign6sensorR2Exform((vr6sensor *)input, strchr(device_end+1, ','));

		vrDbgPrintfN(INPUT_DBGLVL, "assigned 6sensor event of value 0x%02x to input pointer = %#p)\n",
			sensor_num, aux->tracker_inputs[vrpn_device_num][sensor_num]);

	} else {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_VrpnTrackerInput: Warning, invalid input specification (%s, %s).\n" NORM_TEXT,
			type, instance);
	}
#endif

	return 1;	/* input declaration match */
}
#endif


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

        vrFprintContext(stdout, devinfo->context, verbose);	/* NOTE: this effectively requires the inclusion of "vr_system.h" */
}

/************************************************************/
static void _VrpnPrintConfigStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintConfig(stdout, devinfo->context->config, verbose);	/* NOTE: this effectively requires the inclusion of "vr_config.h" */
}

/************************************************************/
static void _VrpnPrintInputStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintInput(stdout, devinfo->context->input, verbose);
}

/************************************************************/
static void _VrpnPrintStructCallback(vrInputDevice *devinfo, int value)
{
        _VrpnPrivateInfo  *aux = (_VrpnPrivateInfo *)devinfo->aux_data;

        if (value == 0)
                return;

        _VrpnPrintStruct(stdout, aux, verbose);
}

/************************************************************/
static void _VrpnPrint6sensorOptionsCallback(vrInputDevice *devinfo, int value)
{
	_VrpnPrivateInfo  *aux = (_VrpnPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	_VrpnPrint6sensorOptions(stdout, aux, verbose);
}

/************************************************************/
static void _VrpnPrintHelpCallback(vrInputDevice *devinfo, int value)
{
        _VrpnPrivateInfo  *aux = (_VrpnPrivateInfo *)devinfo->aux_data;

        if (value == 0)
                return;

        _VrpnPrintHelp(stdout, aux);
}

/************************************************************/
static void _VrpnSensorNextCallback(vrInputDevice *devinfo, int value)
{
	_VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	if (devinfo->num_6sensors == 0) {
		vrDbgPrintfN(SELFCTRL_DBGLVL, "VRPN Control: next sensor -- no sensors available.\n",
			aux->active_sim6sensor);
		return;
	}
#if 0
vrPrintf("sim6 mapped? %d %d %d %d %d\n", aux->device[0].mapped_tracker[0], aux->device[0].mapped_tracker[1], aux->device[0].mapped_tracker[2], aux->device[0].mapped_tracker[3], aux->device[0].mapped_tracker[4]);
#endif

	/* set the current sensor as non-active */
	if (aux->active_sim6sensor >= aux->num_6sensors) {
		/* there is no active sensor */
		vrDbgPrintfN(ALWAYS_DBGLVL, "_VrpnInitializeDevice(): " RED_TEXT "Warning, attempting to change simulated 6-sensor, but none available.\n" NORM_TEXT);
	} else {
		vrAssign6sensorActiveValue(((vr6sensor *)aux->tracker_inputs[aux->active_sim6sensor]), 0);
	}

	/* search from the next possible sensor to the end of the list */
	/* NOTE: VRPN, unlike other devices, has a select set of trackers that are */
	/*   part of the simulation group, so only search those.  (which in practice */
	/*   means keep searching the list if a near-match isn't from the simulation */
	/*   group.)                                                                 */
	do {
		aux->active_sim6sensor++;
#if 0
vrPrintf("@@@@@@ do-while: active_sim6sensor=%d, num_6=%d, aux->tracker_inputs[%d]=%p, aux->tracker_map_device[aux->active_sim6sensor]->name='%s'\n", aux->active_sim6sensor, aux->num_6sensors, aux->active_sim6sensor, aux->tracker_inputs[aux->active_sim6sensor], (aux->active_sim6sensor < aux->num_6sensors ? aux->tracker_map_device[aux->active_sim6sensor]->name : "<void>"));
#endif
												/* keep looping if: */
	} while (	   (aux->active_sim6sensor < aux->num_6sensors)				/*   we haven't gone off the end of the list */
			&& (aux->tracker_inputs[aux->active_sim6sensor] != NULL)			/*   tracker does not exist! */
			&& (aux->tracker_map_device[aux->active_sim6sensor] != &(aux->device[0])));	/*   tracker is not from device[0] (ie. simulator) */

	/* if none found (ie. we were basically already at the end of the list), then search from the beginning */
#if 0
vrPrintf("@@@@@@ pre-for: active_sim6sensor=%d, num_6=%d, aux->tracker_inputs[%d]=%p, aux->tracker_map_device[aux->active_sim6sensor]->name='%s'\n", aux->active_sim6sensor, aux->num_6sensors, aux->active_sim6sensor, aux->tracker_inputs[aux->active_sim6sensor], (aux->active_sim6sensor < aux->num_6sensors ? aux->tracker_map_device[aux->active_sim6sensor]->name : "<void>"));
#endif
	if (aux->active_sim6sensor >= aux->num_6sensors
			|| (aux->tracker_inputs[aux->active_sim6sensor] == NULL)
			|| (aux->tracker_map_device[aux->active_sim6sensor] != &(aux->device[0]))) {
		for (aux->active_sim6sensor = 0
			; /* conditional */							/* keep looping if: */
			   (aux->active_sim6sensor < aux->num_6sensors)				/*   still within the list bounds */
			&& (aux->tracker_inputs[aux->active_sim6sensor] != NULL)			/*   tracker does not exist! */
			&& (aux->tracker_map_device[aux->active_sim6sensor] != &(aux->device[0]))	/*   tracker is not from device[0] (ie. simulator) */
			; /* increment */
			aux->active_sim6sensor++)
#if 0
vrPrintf("@@@@@@ for: active_sim6sensor=%d, num_6=%d, aux->tracker_inputs[%d]=%p, aux->tracker_map_device[aux->active_sim6sensor]->name='%s'\n", aux->active_sim6sensor, aux->num_6sensors, aux->active_sim6sensor, aux->tracker_inputs[aux->active_sim6sensor], aux->tracker_map_device[aux->active_sim6sensor]->name)
#endif
			; /* no-op -- the looping is the reward */
	}
#if 0
vrPrintf(BOLD_TEXT "New active sensor is %d\n" NORM_TEXT, aux->active_sim6sensor);
#endif

	/* set the newly found sensor as active */
	if (aux->active_sim6sensor >= aux->num_6sensors) {
		/* again, no active sensor */
		vrDbgPrintfN(SELFCTRL_DBGLVL, "VRPN Control: No simulated 6-sensor available to make active.\n", aux->active_sim6sensor);
	} else {
		vrAssign6sensorActiveValue(((vr6sensor *)aux->tracker_inputs[aux->active_sim6sensor]), 1);
		aux->device[0].newly_reported_posquat[aux->tracker_map_tracker[aux->active_sim6sensor]] = 1u;	/* VRPN new data flag */

		vrDbgPrintfN(SELFCTRL_DBGLVL, "VRPN Control: 6sensor[%d] now active.\n", aux->active_sim6sensor);
	}
}

/* TODO: see if there is a way to call this as an N-switch */
/************************************************************/
static void _VrpnSensorSetCallback(vrInputDevice *devinfo, int value)
{
	_VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->aux_data;

	if (value == aux->active_sim6sensor)
		return;

	if (value < 0 || value >= MAX_6SENSORS) {
		vrDbgPrintfN(SELFCTRL_DBGLVL, "VRPN Control: set sensor (%d) -- out of range.\n", value);
	}

	if (aux->tracker_inputs[value] == NULL) {
		vrDbgPrintfN(SELFCTRL_DBGLVL, "VRPN Control: set sensor (%d) -- no such sensor available.\n", value);
		return;
	}

	vrAssign6sensorActiveValue(((vr6sensor *)aux->tracker_inputs[aux->active_sim6sensor]), 0);
	aux->active_sim6sensor = value;
	vrAssign6sensorActiveValue(((vr6sensor *)aux->tracker_inputs[aux->active_sim6sensor]), 1);

	vrDbgPrintfN(SELFCTRL_DBGLVL, "VRPN Control: 6sensor[%d] now active.\n", aux->active_sim6sensor);
}

/************************************************************/
static void _VrpnSensorSet0Callback(vrInputDevice *devinfo, int value)
{	_VrpnSensorSetCallback(devinfo, 0); }

/************************************************************/
static void _VrpnSensorSet1Callback(vrInputDevice *devinfo, int value)
{	_VrpnSensorSetCallback(devinfo, 1); }

/************************************************************/
static void _VrpnSensorSet2Callback(vrInputDevice *devinfo, int value)
{	_VrpnSensorSetCallback(devinfo, 2); }

/************************************************************/
static void _VrpnSensorSet3Callback(vrInputDevice *devinfo, int value)
{	_VrpnSensorSetCallback(devinfo, 3); }

/************************************************************/
static void _VrpnSensorSet4Callback(vrInputDevice *devinfo, int value)
{	_VrpnSensorSetCallback(devinfo, 4); }

/************************************************************/
static void _VrpnSensorSet5Callback(vrInputDevice *devinfo, int value)
{	_VrpnSensorSetCallback(devinfo, 5); }

/************************************************************/
static void _VrpnSensorSet6Callback(vrInputDevice *devinfo, int value)
{	_VrpnSensorSetCallback(devinfo, 6); }

/************************************************************/
static void _VrpnSensorSet7Callback(vrInputDevice *devinfo, int value)
{	_VrpnSensorSetCallback(devinfo, 7); }

/************************************************************/
static void _VrpnSensorSet8Callback(vrInputDevice *devinfo, int value)
{	_VrpnSensorSetCallback(devinfo, 8); }

/************************************************************/
static void _VrpnSensorSet9Callback(vrInputDevice *devinfo, int value)
{	_VrpnSensorSetCallback(devinfo, 9); }

/************************************************************/
static void _VrpnSensorResetCallback(vrInputDevice *devinfo, int value)
{
static	vrMatrix		idmat = { { 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0 } };
	_VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->aux_data;
	vr6sensor		*sensor;

	if (value == 0)
		return;

	sensor = &(devinfo->sensor6[aux->active_sim6sensor]);
	vrAssign6sensorValue(sensor, &idmat, 0 /* , timestamp */);
	vrAssign6sensorActiveValue(sensor, -1);
	vrAssign6sensorErrorValue(sensor, 0);
	vrDbgPrintfN(SELFCTRL_DBGLVL, "VRPN Control: reset 6sensor[%d].\n", aux->active_sim6sensor);
}

/************************************************************/
static void _VrpnSensorResetAllCallback(vrInputDevice *devinfo, int value)
{
static	vrMatrix		idmat = { { 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0 } };
	_VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->aux_data;
	vr6sensor		*sensor;
	int			count;

	if (value == 0)
		return;

	for (count = 0; count < MAX_6SENSORS; count++) {
		if (aux->tracker_inputs[count] != NULL) {
			sensor = &(devinfo->sensor6[count]);
			vrAssign6sensorValue(sensor, &idmat, 0 /* , timestamp */);
			vrAssign6sensorActiveValue(sensor, (count == aux->active_sim6sensor));
			vrAssign6sensorErrorValue(sensor, 0);
			vrDbgPrintfN(SELFCTRL_DBGLVL, "VRPN Control: reset 6sensor[%d].\n", count);
		}
	}
}

/************************************************************/
static void _VrpnTempValuatorOverrideCallback(vrInputDevice *devinfo, int value)
{
	_VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->aux_data;

	/* set the field to the current state of value (ie. 1 when depressed, 0 when released) */
	aux->sensor6_options.ignore_trans = value;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	aux->device[0].newly_reported_posquat[aux->tracker_map_tracker[aux->active_sim6sensor]] = 1u;	/* VRPN new data flag */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "VRPN Control: ignore_trans = %d.\n",
		aux->sensor6_options.ignore_trans);
}

/************************************************************/
static void _VrpnToggleValuatorOverrideCallback(vrInputDevice *devinfo, int value)
{
	_VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->aux_data;

	/* take no action when the button is released */
	if (value == 0)
		return;

	/* toggle the field (NOTE: we only get here when button is depressed) */
	aux->sensor6_options.ignore_trans ^= 1;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	aux->device[0].newly_reported_posquat[aux->tracker_map_tracker[aux->active_sim6sensor]] = 1u;	/* VRPN new data flag */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "VRPN Control: ignore_trans = %d.\n",
		aux->sensor6_options.ignore_trans);
}

/************************************************************/
static void _VrpnTempValuatorOnlyCallback(vrInputDevice *devinfo, int value)
{
	_VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->aux_data;

	/* set the field to the current state of value (ie. 1 when depressed, 0 when released) */
	aux->sensor6_options.ignore_all = value;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	aux->device[0].newly_reported_posquat[aux->tracker_map_tracker[aux->active_sim6sensor]] = 1u;	/* VRPN new data flag */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "VRPN Control: ignore_all = %d.\n",
		aux->sensor6_options.ignore_all);
}

/************************************************************/
static void _VrpnToggleValuatorOnlyCallback(vrInputDevice *devinfo, int value)
{
	_VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->aux_data;

	/* take no action when the button is released */
	if (value == 0)
		return;

	/* toggle the field (NOTE: we only get here when button is depressed) */
	aux->sensor6_options.ignore_all ^= 1;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	aux->device[0].newly_reported_posquat[aux->tracker_map_tracker[aux->active_sim6sensor]] = 1u;	/* VRPN new data flag */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "VRPN Control: ignore_all = %d.\n",
		aux->sensor6_options.ignore_all);
}

/************************************************************/
static void _VrpnToggleRelativeAxesCallback(vrInputDevice *devinfo, int value)
{
	_VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->aux_data;

	/* take no action when the button is released */
	if (value == 0)
		return;

	/* toggle the field (NOTE: we only get here when button is depressed) */
	aux->sensor6_options.relative_axis ^= 1;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	aux->device[0].newly_reported_posquat[aux->tracker_map_tracker[aux->active_sim6sensor]] = 1u;	/* VRPN new data flag */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "VRPN Control: relative_axis = %d.\n",
		aux->sensor6_options.relative_axis);
}

/************************************************************/
/* TODO: this should probably also go through all the 6sensor's  */
/*   and move them to be within the allowed workspace when space */
/*   restriction is turned on.                                   */
static void _VrpnToggleRestrictSpaceCallback(vrInputDevice *devinfo, int value)
{
	_VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->aux_data;

	/* take no action when the button is released */
	if (value == 0)
		return;

	/* toggle the field (NOTE: we only get here when button is depressed) */
	aux->sensor6_options.restrict_space ^= 1;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	aux->device[0].newly_reported_posquat[aux->tracker_map_tracker[aux->active_sim6sensor]] = 1u;	/* VRPN new data flag */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "VRPN Control: restrict_space = %d.\n",
		aux->sensor6_options.restrict_space);
}

/************************************************************/
static void _VrpnToggleReturnToZeroCallback(vrInputDevice *devinfo, int value)
{
	_VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->aux_data;

	/* take no action when the button is released */
	if (value == 0)
		return;

	/* toggle the field (NOTE: we only get here when button is depressed) */
	aux->sensor6_options.return_to_zero ^= 1;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	aux->device[0].newly_reported_posquat[aux->tracker_map_tracker[aux->active_sim6sensor]] = 1u;	/* VRPN new data flag */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "VRPN Control: return_to_zero = %d.\n",
		aux->sensor6_options.return_to_zero);
}

/************************************************************/
static void _VrpnToggleSwapTransRotCallback(vrInputDevice *devinfo, int value)
{
	_VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->aux_data;

	/* take no action when the button is released */
	if (value == 0)
		return;

	/* toggle the field (NOTE: we only get here when button is depressed) */
	aux->sensor6_options.swap_transrot ^= 1;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	aux->device[0].newly_reported_posquat[aux->tracker_map_tracker[aux->active_sim6sensor]] = 1u;	/* VRPN new data flag */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "VRPN Control: swap_transrot = %d.\n",
		aux->sensor6_options.swap_transrot);
}

/************************************************************/
static void _VrpnToggleSwapYZCallback(vrInputDevice *devinfo, int value)
{
	_VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->aux_data;

	/* take no action when the button is released */
	if (value == 0)
		return;

	/* toggle the field (NOTE: we only get here when button is depressed) */
	aux->sensor6_options.swap_yz ^= 1;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	aux->device[0].newly_reported_posquat[aux->tracker_map_tracker[aux->active_sim6sensor]] = 1u;	/* VRPN new data flag */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "VRPN Control: swap_yz = %d.\n",
		aux->sensor6_options.swap_yz);
}

/************************************************************/
static void _VrpnSetTransXCallback(vrInputDevice *devinfo, float *value)
{
	_VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->aux_data;

	aux->sim_values[VR_X] = *value;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	aux->device[0].newly_reported_posquat[aux->tracker_map_tracker[aux->active_sim6sensor]] = 1u;	/* VRPN new data flag */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "VRPN Control: set trans_x = %f.\n", *value);
}

/************************************************************/
static void _VrpnSetTransYCallback(vrInputDevice *devinfo, float *value)
{
	_VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->aux_data;
	int			simulated_tracker_num;

	aux->sim_values[VR_Y] = *value;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	simulated_tracker_num = aux->tracker_map_tracker[aux->active_sim6sensor];
	aux->device[0].newly_reported_posquat[simulated_tracker_num] = 1u;				/* VRPN new data flag */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "VRPN Control: set trans_y = %f.\n", *value);
}

/************************************************************/
static void _VrpnSetTransZCallback(vrInputDevice *devinfo, float *value)
{
	_VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->aux_data;

	aux->sim_values[VR_Z] = *value;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	aux->device[0].newly_reported_posquat[aux->tracker_map_tracker[aux->active_sim6sensor]] = 1u;	/* VRPN new data flag */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "VRPN Control: set trans_z = %f.\n", *value);
}

/************************************************************/
static void _VrpnSetAzimuthCallback(vrInputDevice *devinfo, float *value)
{
	_VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->aux_data;

	aux->sim_values[VR_AZIM+3] = *value;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	aux->device[0].newly_reported_posquat[aux->tracker_map_tracker[aux->active_sim6sensor]] = 1u;	/* VRPN new data flag */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "VRPN Control: set azimuth = %f.\n", *value);
}

/************************************************************/
static void _VrpnSetElevationCallback(vrInputDevice *devinfo, float *value)
{
	_VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->aux_data;

	aux->sim_values[VR_ELEV+3] = *value;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	aux->device[0].newly_reported_posquat[aux->tracker_map_tracker[aux->active_sim6sensor]] = 1u;	/* VRPN new data flag */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "VRPN Control: set elevation = %f.\n", *value);
}

/************************************************************/
static void _VrpnSetRollCallback(vrInputDevice *devinfo, float *value)
{
	_VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->aux_data;

	aux->sim_values[VR_ROLL+3] = *value;
	aux->sim6sensor_change = 1;		/* indicate new data for simulated 6-sensor */
	aux->device[0].newly_reported_posquat[aux->tracker_map_tracker[aux->active_sim6sensor]] = 1u;	/* VRPN new data flag */
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Vrpn Control: set roll = %f.\n", *value);
}


	/*************************************************/
	/*   Callbacks for interfacing with the device.  */
	/*                                               */


/************************************************************/
static void _VrpnCreateFunction(vrInputDevice *devinfo)
{
	/*** List of possible inputs ***/
static	vrInputFunction	_VrpnInputs[] = {
				{ "button", VRINPUT_2WAY, _VrpnButtonInput },
				{ "analog", VRINPUT_VALUATOR, _VrpnValuatorInput },
				{ "tracker", VRINPUT_6SENSOR, _Vrpn6sensorInput },
				{ "sim6", VRINPUT_6SENSOR, _Vrpn6sensorInput },
				{ NULL, VRINPUT_UNKNOWN, NULL } };
	/*** List of control functions ***/
static	vrControlFunc	_VrpnControlList[] = {
				/* informational output controls */
				{ "print_context", _VrpnPrintContextStructCallback },
				{ "print_config", _VrpnPrintConfigStructCallback },
				{ "print_input", _VrpnPrintInputStructCallback },
				{ "print_struct", _VrpnPrintStructCallback },
				{ "print_sim6opts", _VrpnPrint6sensorOptionsCallback },
				{ "print_help", _VrpnPrintHelpCallback },

				/* simulated 6-sensor selection controls */
				{ "sensor_next", _VrpnSensorNextCallback },
				{ "setsensor", _VrpnSensorSetCallback },	/* NOTE: this is non-boolean */
				{ "setsensor(0)", _VrpnSensorSet0Callback },
				{ "setsensor(1)", _VrpnSensorSet1Callback },
				{ "setsensor(2)", _VrpnSensorSet2Callback },
				{ "setsensor(3)", _VrpnSensorSet3Callback },
				{ "setsensor(4)", _VrpnSensorSet4Callback },
				{ "setsensor(5)", _VrpnSensorSet5Callback },
				{ "setsensor(6)", _VrpnSensorSet6Callback },
				{ "setsensor(7)", _VrpnSensorSet7Callback },
				{ "setsensor(8)", _VrpnSensorSet8Callback },
				{ "setsensor(9)", _VrpnSensorSet9Callback },
				{ "sensor_reset", _VrpnSensorResetCallback },
				{ "sensor_resetall", _VrpnSensorResetAllCallback },

				/* simulated 6-sensor manipulation controls */
				{ "temp_valuator", _VrpnTempValuatorOverrideCallback },
				{ "toggle_valuator", _VrpnToggleValuatorOverrideCallback },
				{ "temp_valuator_only", _VrpnTempValuatorOnlyCallback },
				{ "toggle_valuator_only", _VrpnToggleValuatorOnlyCallback },
				{ "toggle_relative", _VrpnToggleRelativeAxesCallback },
				{ "toggle_space_limit", _VrpnToggleRestrictSpaceCallback },
				{ "toggle_return_to_zero", _VrpnToggleReturnToZeroCallback },
				{ "toggle_swap_transrot", _VrpnToggleSwapTransRotCallback },
				{ "toggle_swap_yz", _VrpnToggleSwapYZCallback },
				{ "set_transx", _VrpnSetTransXCallback },
				{ "set_transy", _VrpnSetTransYCallback },
				{ "set_transz", _VrpnSetTransZCallback },
				{ "set_azim", _VrpnSetAzimuthCallback },
				{ "set_elev", _VrpnSetElevationCallback },
				{ "set_roll", _VrpnSetRollCallback },

		/** TODO: other callback control functions go here **/
				/* other controls */

				/* end of the list */
				{ NULL, NULL } };

	_VrpnPrivateInfo	*aux = NULL;

	/******************************************/
	/* allocate and initialize auxiliary data */
	devinfo->aux_data = (void *)vrShmemAlloc0(sizeof(_VrpnPrivateInfo));
	aux = (_VrpnPrivateInfo *)devinfo->aux_data;
	_VrpnInitializeStruct(aux);

	/******************/
	/* handle options */
	aux->vrpn_host = vrShmemStrDup(DEFAULT_HOST);	/* initialize to the default host value */
	aux->vrpn_port = DEFAULT_PORT;			/* initialize to the default port value */
	_VrpnParseArgs(aux, devinfo->args);


	/***************************************/
	/* create the inputs and self-controls */
	vrInputCountDataContainers(devinfo);

	/* Allocate memory for specific input types */
	/*   NOTE: something to be aware of -- when a 0 byte allocation is requested, vrShmemAlloc0() will return NULL */
	aux->button_inputs = (vr2switch **)vrShmemAlloc((devinfo->num_2ways + devinfo->num_scontrols) * sizeof(vr2switch *));
	aux->button_map_button = (int *)vrShmemAlloc0((devinfo->num_2ways + devinfo->num_scontrols) * sizeof(int));
	aux->button_map_devicename = (char **)vrShmemAlloc0((devinfo->num_2ways + devinfo->num_scontrols) * sizeof(char *));
	aux->button_map_device = (VRPNdevice **)vrShmemAlloc0((devinfo->num_2ways + devinfo->num_scontrols) * sizeof(VRPNdevice *));

	aux->valuator_inputs = (vrValuator **)vrShmemAlloc0((devinfo->num_valuators + devinfo->num_scontrols) * sizeof(vrValuator *));
	aux->valuator_map_valuator = (int *)vrShmemAlloc0((devinfo->num_valuators + devinfo->num_scontrols) * sizeof(int));
	aux->valuator_sign = (float *)vrShmemAlloc0((devinfo->num_valuators + devinfo->num_scontrols) * sizeof(float));
	aux->valuator_map_devicename = (char **)vrShmemAlloc0((devinfo->num_valuators + devinfo->num_scontrols) * sizeof(char *));
	aux->valuator_map_device = (VRPNdevice **)vrShmemAlloc0((devinfo->num_valuators + devinfo->num_scontrols) * sizeof(VRPNdevice *));

	aux->tracker_inputs = (vr6sensor **)vrShmemAlloc0((devinfo->num_6sensors) * sizeof(vr6sensor *));
	aux->tracker_map_tracker = (int *)vrShmemAlloc0((devinfo->num_6sensors) * sizeof(int));
	aux->tracker_map_devicename = (char **)vrShmemAlloc0((devinfo->num_6sensors) * sizeof(char *));
	aux->tracker_map_device = (VRPNdevice **)vrShmemAlloc0((devinfo->num_6sensors) * sizeof(VRPNdevice *));

	/* Create the inputs within the FreeVR system */
	vrInputCreateDataContainers(devinfo, _VrpnInputs);
	vrInputCreateSelfControlContainers(devinfo, _VrpnInputs, _VrpnControlList);

	/* TODO: anything to do for NON self-controls?  Implement for Magellan first. */

	vrDbgPrintf("Done creating VRPN inputs for '%s'\n", devinfo->name);
	devinfo->created = 1;

	return;
}


#endif /* } FREEVR */
/************************************************************/
static void _VrpnOpenFunction(vrInputDevice *devinfo)
{
	_VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->aux_data;
static	char	*version = "vrpn: ver. 07.28  0\0\0\0\0\0\0\0\0";	/* Using the current version number */
	int	read_result;
	int	rval;				/* return value */

	vrTrace("_VrpnOpenFunction", devinfo->name);

	/* TODO: consider putting a check for VRPN functions here (ie. is the DSO loaded). */

	/*******************/
	/* open the device */

	/* open the "incoming" port (ie. port connected to the device) */
	aux->vrpn_socket = vrSocketCall(aux->vrpn_host, aux->vrpn_port);
	if (aux->vrpn_socket < 0) {
		vrDbgPrintfN(ALWAYS_DBGLVL, "_VrpnOpenFunction(): " RED_TEXT "Error, unable to open connection to socket '%s:%d'!\n" NORM_TEXT, aux->vrpn_host, aux->vrpn_port);
		return;		/* return w/o setting the open flag */
	}

	if (fcntl(aux->vrpn_socket, F_SETFL, O_RDWR | O_NONBLOCK | O_NOCTTY) < 0) {
		vrDbgPrintfN(AALWAYS_DBGLVL, "_VrpnOpenFunction(): " RED_TEXT "An error occurred while trying to set the file controls.\n" NORM_TEXT);
		return;		/* return w/o setting the open flag */
	}

	aux->open = 1;		/* communication line has been established */

	/* establish connection with the protocol */
	rval = write(aux->vrpn_socket, version, 24);	/* NOTE: not using vrSocketSendMsg() to avoid sending the NULL terminator */
	if (rval != 24) {
		vrDbgPrintfN(AALWAYS_DBGLVL, "_VrpnOpenFunction(): " RED_TEXT "An error occurred while trying to send VRPN hail message - %d bytes send instead of %d.\n" NORM_TEXT, rval, 24);
		if (rval < 0)
			perror("_VrpnOpenFunction(): write()");
	}
	vrSerialAwaitData(aux->vrpn_socket);		/* NOTE: currently hard-coded to wait .15 second */
	read_result = (ssize_t)read(aux->vrpn_socket, &aux->buf[aux->eobuf_pos], sizeof(aux->buf)-aux->eobuf_pos);
	if (read_result <= 0) {
		return;		/* didn't get a quick response -- return w/o setting the open flag */
	}
	aux->eobuf_pos = read_result;

	/* parse the version number */
	sscanf(aux->buf, "vrpn: ver. %d.%d  0", &aux->server_ver_major, &aux->server_ver_minor);
	vrDbgPrintf("got (%d) '%s' -- major = %d, minor = %d\n", read_result, aux->buf, aux->server_ver_major, aux->server_ver_minor);
	memmove(aux->buf, &aux->buf[24], (sizeof(aux->buf)-24));	/* shift the data */
	aux->eobuf_pos -= 24;

	/* initialize the device */
	if (_VrpnInitializeDevice(aux) < 0) {
		devinfo->operating = 0;
		vrErrPrintf("_VrpnOpenFunction(): "
			RED_TEXT "Warning, unable to initialize '%s' DEVICE.\n" NORM_TEXT,
			devinfo->name);
	} else {
		devinfo->operating = 1;
		vrDbgPrintfN(AALWAYS_DBGLVL, "_VrpnOpenFunction(): Done opening and initializing VRPN input device '%s'.\n", devinfo->name);
	}

	vrTrace("_VrpnOpenFunction", "returning");

	return;
}


/************************************************************/
static void _VrpnCloseFunction(vrInputDevice *devinfo)
{
	_VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->aux_data;

	/* TODO: add VRPN shut down operations */

	/* TODO: close the socket */

	if (aux != NULL) {
		vrShmemFree(aux);	/* aka devinfo->aux_data */
	}

	return;
}


#if defined(FREEVR) /* { */
/************************************************************/
static void _VrpnResetFunction(vrInputDevice *devinfo)
{
	_VrpnPrivateInfo	*aux = (_VrpnPrivateInfo *)devinfo->aux_data;
	/* TODO: reset code */

	return;
}


/************************************************************/
static void _VrpnPollFunction(vrInputDevice *devinfo)
{
	if (devinfo->operating) {
		_VrpnGetData(devinfo);
	} else {
		/* TODO: try to open the device again */
	}

	return;
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
	devinfo->Create = vrCallbackCreateNamed("Vrpn:Create-Def", _VrpnCreateFunction, 1, devinfo);
	devinfo->Open = vrCallbackCreateNamed("Vrpn:Open-Def", _VrpnOpenFunction, 1, devinfo);
	devinfo->Close = vrCallbackCreateNamed("Vrpn:Close-Def", _VrpnCloseFunction, 1, devinfo);
	devinfo->Reset = vrCallbackCreateNamed("Vrpn:Reset-Def", _VrpnResetFunction, 1, devinfo);
	devinfo->PollData = vrCallbackCreateNamed("Vrpn:PollData-Def", _VrpnPollFunction, 1, devinfo);
	devinfo->PrintAux = vrCallbackCreateNamed("Vrpn:PrintAux-Def", _VrpnPrintStruct, 0);

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

/******************************************************************/
/* Ugh, I hate globals, but I don't know a better way to get the  */
/*   aux value into the interrupt signal function exit_testapp(). */
static	int	done = 0;


/*******************************************************************/
void exit_testapp()
{
	done = 1;
	printf("Exiting application\n");
}


/***************************************************************************/
/* A test program to communicate with a VRPN device and print the results. */
main(int argc, char *argv[])
{
static	char			*err_missing_arg = "%s: missing argument to '%s'.\n";
	_VrpnPrivateInfo	*aux;
	vrInputDevice		device;
	char			*progname;			/* name of the program executable */
#ifdef VRPN_LIB /* { */
	char			*hostname = "149.166.144.248";
	char			*tracker_name = "Tracker0";
	char			tracker_dev[256];
#endif /* } VRPN_LIB */
	int			count;				/* for looping over things */
	int			nodata_flag = 0;		/* whether or not to actual show input data */
	int			button1_value = -1;		/* value of the first actual button in the system (used for chorded-quit) */
	int			button2_value = -1;		/* value of the second actual button in the system (used for chorded-quit) */

	done = 0;
	signal(SIGINT, exit_testapp);


	/******************************/
	/* setup the device structure */
	aux = (_VrpnPrivateInfo *)malloc(sizeof(_VrpnPrivateInfo));
	memset(aux, 0, sizeof(_VrpnPrivateInfo));
	_VrpnInitializeStruct(aux);

	device.name = "VRPN test device";
	device.operating = 0;
	device.aux_data = aux;


	/*********************************************************/
	/* set default parameters based on environment variables */

	if (getenv("VRPN_HOST") != NULL)
		aux->vrpn_host = getenv("VRPN_HOST");
	else	aux->vrpn_host = DEFAULT_HOST;			/* default, if no host given */

	aux->vrpn_port = ((getenv("VRPN_PORT")==NULL) ? DEFAULT_PORT : atoi(getenv("VRPN_PORT")));

#ifdef VRPN_LIB /* { */
	if (getenv("VRPN_TRACKER") != NULL)
		tracker_name = getenv("VRPN_TRACKER");
#endif /* } VRPN_LIB */


	/*****************************************************/
	/* adjust parameters based on command-line-arguments */
	progname = argv[0];
	while ((argc > 1) && (argv[1][0] == '-')) {
		/* Report information about the device and then quit */
		/* NOTE: the "-nodata" option already basically does a "list" of what's coming from the vrpn_server */
		if (!strcmp(argv[1], "-nodata") || !strcmp(argv[1], "-list")) {
			nodata_flag = 1;
			argv++; argc--;
		}

		/* Set the port */
		if (!strcmp(argv[1], "-p") || !strcmp(argv[1], "-port")) {
			argv++; argc--;
			if (argc > 1) {
				aux->vrpn_port = atoi(argv[1]);
				argv++; argc--;
			} else {
				fprintf(stderr, err_missing_arg, progname, argv[0]);
				fprintf(stderr, RED_TEXT "Usage:" NORM_TEXT " %s [-list | -nodata] [-p <port>(%d)] [<VRPN host> (default = '%s')]\n",
					progname, aux->vrpn_port, aux->vrpn_host);
				exit(1);
			}
		}

		/* Unknown option */
		else {
			/* There are currently no other "-" options, so this is an error */
			fprintf(stderr, RED_TEXT "Usage:" NORM_TEXT " %s [-list | -nodata] [-p <port>(%d)] [<VRPN host> (default = '%s')]\n",
				progname, aux->vrpn_port, aux->vrpn_host); /* NOTE: reported defaults might be effected by environment variables */
			exit(1);
		}
	}

	/* if there are any arguments left, use the first as the daemon hostname */
	if (argc > 1) {
		aux->vrpn_host = strdup(argv[1]);
	}


	/**********************************/
	/* open and initialize the device */
	aux->open = 0;
#ifdef VRPN_LIB /* { */
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
#else /* Do it the brute-force way */
	_VrpnOpenFunction(&device);
#endif /* } VRPN_LIB */

	if (_VrpnInitializeDevice(aux) < 0) {
		vrErrPrintf("main: " RED_TEXT "Warning, unable to initialize VRPN.\n" NORM_TEXT);
		aux->open = 0;
	}

	_VrpnPrintStruct(stdout, aux, verbose);
	fprintf(stdout, "-------------------\n");
	for (count = 1; count < MAX_DEVICES; count++) {
		if (aux->device[count].name[0] != '\0')
			fprintf(stdout, "Device %d is '%s'\n", count, aux->device[count].name);
	}
	fprintf(stdout, "-------------------\n");
	for (count = 0; count < aux->biggest_code; count++) {
		fprintf(stdout, "Message Code %2d is '%s'\n", count, aux->message_codes[count]);
	}
	fprintf(stdout, "-------------------\n");

	/* quit if flagged to print the device info but not data */
	if (nodata_flag)
		exit(0);

	/****************************************/
	/* read the data and display the output */
	while(aux->open && !done) {
		VRPNdevice	*vrpn_dev;		/* pointer to a device in the VRPN device list */
		int		input;
		int		devname_flag;		/* flag of whether a device's name has been printed yet */

		/* if any new data was received then display it */
		if (_VrpnReadInput(aux) > 0) {
			/* Display tracker (6-sensor) information */
			for (count = 0; count < MAX_DEVICES; count++) {
				vrpn_dev = &(aux->device[count]);
				devname_flag = 0;
				for (input = 0; input < MAX_6SENSORS; input++) {
					if (vrpn_dev->newly_reported_posquat[input]) {
						if (!devname_flag) {
							printf("Device %d(%s):", count, vrpn_dev->name);
							devname_flag = 1;
						}
						printf("s%d x:%.3f y:%.3f z:%.3f  qx:%.3f qy:%.3f qz:%.3f qw:%.3f  ",
							input,
							vrpn_dev->posquat[input][VR_X], vrpn_dev->posquat[input][VR_Y], vrpn_dev->posquat[input][VR_Z],
							vrpn_dev->posquat[input][VR_QX], vrpn_dev->posquat[input][VR_QY], vrpn_dev->posquat[input][VR_QZ], vrpn_dev->posquat[input][VR_QW]);
						vrpn_dev->newly_reported_posquat[input] = 0u;
					}
				}
			}

			/* Display analog (valuator) information */
			for (count = 0; count < MAX_DEVICES; count++) {
				vrpn_dev = &(aux->device[count]);
				devname_flag = 0;
				for (input = 0; input < MAX_VALUATORS; input++) {
					if (vrpn_dev->newly_reported_analog[input]) {
#if 0 /* old, more verbose method */
						if (!devname_flag) {
							printf("Device %d(%s):", count, vrpn_dev->name);
							devname_flag = 1;
						}
						printf("val[%d] = % 6.3f  ", input, vrpn_dev->analog[input]);
#else /* print more concisely so we can see more values */
						if (!devname_flag) {
							printf("Device %d(%s): valuators: ", count, vrpn_dev->name);
							devname_flag = 1;
						}
						printf("%d:%8.3lf ", input, vrpn_dev->analog[input]);
#endif
						vrpn_dev->newly_reported_analog[input] = 0u;
					}
				}
			}

			/* Display button (2-switch) information */
			for (count = 0; count < MAX_DEVICES; count++) {
				vrpn_dev = &(aux->device[count]);
				devname_flag = 0;
				button1_value = -1;	/* reset back to -1 for each device */
				button2_value = -1;	/* reset back to -1 for each device */
				for (input = 0; input < MAX_BUTTONS; input++) {
					if (vrpn_dev->newly_reported_button[input]) {
						if (!devname_flag) {
							printf("Device %d(%s):", count, vrpn_dev->name);
							devname_flag = 1;
						}
						printf("button[%d] = %d  ", input, vrpn_dev->button[input]);
						vrpn_dev->newly_reported_button[input] = 0u;
					}

					/* Alternate mode of quitting is to press buttons 1 & 2 of any VRPN device */
					if (button1_value < 0)
						button1_value = vrpn_dev->button[input];
					else if (button2_value < 0)
						button2_value = vrpn_dev->button[input];
					if ((button1_value > 0) && (button2_value> 0))
						done = 1;
				}
			}
			printf("\n");
			fflush(stdout);

		}
	}


	/*****************/
	/* close up shop */
	if (aux != NULL) {
#ifdef VRPN_LIB /* { */
		/* TODO: close the VRPN library connections */
#else /* Do it the brute-force way */
		_VrpnCloseFunction(&device);
#endif /* } VRPN_LIB */
	}

	vrPrintf(BOLD_TEXT "\nVRPN device closed\n" NORM_TEXT);
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

