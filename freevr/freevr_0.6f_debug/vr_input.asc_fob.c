/* ======================================================================
 *
 *  CCCCC          vr_input.asc_fob.c
 * CC   CC         Author(s): Bill Sherman, Albert Khakshour
 * CC              Created: February 14, 2000
 * CC   CC         Last Modified: September 28, 2005
 *  CCCCC
 *
 * Code file for FreeVR inputs from the Ascension Flock of Birds[tm]
 *   input device.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/*************************************************************************

USAGE:
	...

	Inputs are specified with the "input" option:
		input "<name>" = "2switch(<sensor number>, {L|M|R|Z})";  (aka "birdmouse" & "button")
		input "<name>" = "2switch(zerorot[<sensor number>])";
		input "<name>" = "6sensor(<sensor number>)"; ("bird" and "receiver" also work)
	  :-?	input "<name>" = "valuator(??[<sensor number>, <position portion>])";
			(where "<position portion>" is {azim,elev,roll,x,y,z}).

		The "zerorot" 2switch input is really just for testing.  It sets
			the button to be one when all the orientations (currently
			just azimuth) are less than 20.0 degrees.

		TODO: this might be a good place to rant about the birdmouse
			buttons not being independent.  Of course, there is
			more to rant about with the FoB design.  Perhaps a
			new RANT: section could be added after USAGE.

	Controls are specified with the "control" option:
		control "<control option>" = "2switch(birdmouse[<sensor number>, {L|M|R|Z}])";
			(aka "" and "button")

	Here is the available control options for FreeVR:
		"print_help" -- print info on how to use the input device
		"system_pause_toggle" -- toggle the system pause flag
		"print_context" -- print the overall FreeVR context data structure (for debugging)
		"print_config" -- print the overall FreeVR config data structure (for debugging)
	  	"print_input" -- print the overall FreeVR input data structure (for debugging)
		"print_struct" -- print the internal FoB data structure (for debugging)
	  :-(	?? -- set the filters to use
			This will allow VR-admins to test whether filters
			are necessary or not -- adding filters increases
			the data reporting lag.  [See App Note #6, 6 pg. 144]

	Here are the FreeVR configuration argument options for the Flock of Birds:
		"port" - serial port FoB is connected to
			("/dev/input/fob" is the default -- or will be once debugged)
		"baud" - baud rate of serial port connection
			(38400 is the default)
		"hemisphere" - the portion (half) of the world in which the receivers
			should be assumed to be located
			(default to "lower")
		"scale" - convert from native units of FoB (inches) to units of the VR system
	  :-(	"angles" - specify how the angles that will be converted into the
			6-sensor matrix should be retrieved from the flock { "euler" "quad" "3x3" }
		"stream" - select whether stream mode is used (vs. "polling")
		"group" - select whether all units should report as a group
		"crtsync" - turn CRT syncing "off", or "on" (or "2" for 2nd on mode)
	  	"filter" { off| <filter number> } - whether to filter, and filter parameter
	  :-(	"syncmode" { off | "samefreq" | "doubFreq" } - whether to sync, and at what rate
	  :-(	"calibration" <calibration file ??> - a calibration lookup table


NOTES on the Flock of Birds:

	Terminology:
		"Bird unit"  (sometimes referred to as just "bird" or just "unit"):
			- a sensor or transmitter controller box

		"Master unit"
			- the unit with the serial connection to the host computer.
				It must be unit #1 (based on experience, not the manual).

		"Addressing mode"
			-

	TODO: figure out what the word "Master" means in the Ascension manual
		10/19/00 -- I think it just means the one unit with the serial
			connection to the computer.  And does not imply that
			it must be unit #1.
		10/19/00 -- though on page 90, it does say:
			 "The Bird at address = 1 (the default Master)"

	TODO: put information on how to setup FOB dip switches here,
		including how to attach the CRT sync, and the fact that
		that requires making unit 1 a receiver.

	DIP-switch settings:

		NORMAL addressing mode:
		bits (from left)= value
			   123	= uuu -  N/A
				= uud -   2400
				= udu -   4800
				= udd -   9600
				= duu -  19200
				= dud -  38400
				= ddu -  57600
				= ddd - 115200
			   ----------
			   4567	= uuuu - indicates a single unit standalone setup
				= uuud - unit #1
				= uudu - unit #2
				= uudd - unit #3
				= uduu - unit #4
				...
				= dddu - unit #14
				= dddd -  N/A (used for broadcasting to all units)
			   ----------
			   8	= u - fly mode
				= d - test mode

	Single serial connection should be made to unit #1.
		This can be a transmitter or sensor, but is often a sensor
		in order to take advantage of the CRT sync connection.
		[10/19/00 -- I'm not sure this is accurate]

		[10/25/00 -- When I set the serial unit to something other than
		unit #1, the system status only finds that unit. -- This implies
		that the master unit (ie. the one with the serial connection)
		*must* be unit #1.

	Address mode of the Flock can be determined by counting the flashes
		during power-up:
		5) Normal Address Mode		(up to  14 units)
		2) Expanded Address Mode	(up to  30 units)
		1) Super-Expanded Address Mode	(up to 126 units)

	Example dip switch settings:
		standalone unit (115200 baud):   ddduuuuu
		single unit flock (115200 baud): ddduuudu
		dual unit (small transmitter):
			unit 1 (w/ serial cable): ddduuudu
			unit 2: ddduuduu
				[does baud matter for 2nd unit?]

	NOTE: a "Bird Reset" is issued to the Flock by "asserting the RTS signal"
		TODO: figure out how to do this.

	NOTE: the "master" unit is the one with the single serial line
		connecting the flock to the computer.
		TODO: determine if this MUST be unit #1 (probably, but not sure)
			10/19/00 -- Actually, it seems not. (based on the manual)
			10/25/00 -- Actually, based on experience, it seems so.

	NOTE: it appears as though requesting button data from a receiver
		without a button causes a "Watch Dog Timer" error.  Of course,
		this may just be for older units.  The unit on which this
		behavior occurred is a '6DFOB', rev 3.64.

	Procedure for "normal" FoB setup:
		(Where "normal" = Normal Address mode, ERC as unit #E, and
		sensor units #S1, S2, ... Sn, with a single RS-232 connection
		to sensor unit #S1 (because of usual need for CRT syncing).)

		TODO: is more needed for this "procedure?"


	Here is how the Flock of Birds coordinate system is laid out, with
		the cord coming out the -x side of the transmitter:
				_______
			       /:     /|
			      / :    / |         o----(+x)
			     /______/  |        /|
			     |  :...|..|       / |
			*****|o;    | /     (+y) |
		       *     |;     |/           (+z)
		  *****      |______|


HISTORY:
	14 February 2000 (Bill Sherman) -- copied vr_input.spacetec.c and began
		editing.  I took some code hints from the VRPN library, and
		some from the Clemson Bird library.

	23 February 2000 (Albert Khakshour)
		Added _FobSendCommandAddr function to send a command to a
		  particular bird.
		Added serial, rev_number, and model fields to _FobUnit struct.
		Modified _FobPrintStruct to print out a text description of
		  the bits in the bird_status field and the status field for
		  the individual units.

	28 February 2000 (Albert Khakshour)
		Added _FobPrintUnitStatus function (nee _FobPrintUnitBirdStatus).
		Added _FobPrintUnitConfig function (nee _FobPrintUnitStatus).
		Added _FobPrintFlockStatus function.
		Added _FobGetFlockStatus function.
		Added _FobReadInput function. (nee _FobUpdateSensors)
		Added _FobConfigMask enum (nee _FobStatus).
		Added FOB_SET_OUTPUT_* to _FobCommand enum.
		Added sn_sensor, sn_xmtr, rev_major, rev_minor, crystal_rate,
		  status, and data fields to _FobUnit struct.
		Added num_units (nee numBirds) and max_units (nee maxBirds)
		  to _FobPrivateInfo struct.
		We're now getting real data out of the new tracker, but the
		  ones on picasso and cassatt aren't playing nice.

	3-5 October 2000 (Bill Sherman) -- mid mod
		Begun the process of reintegrating Albert's additions to
		the core FreeVR code.  Also fixing some of the (hopefully
		few) remaining bugs.

	18-26+ October 2000 (Bill Sherman) -- mid mod still (just want to
		clean up stream mode).
		At this point, the serial interface with the Flock of Birds
		is more or less running well.  (I.e. the test application runs
		well).

	5-8 November 2000 (Bill Sherman)
		All modes (stream/poll, group/non-group) all work great, and
		  can be selected by fields in the _FobPrivateInfo structure,
		  and in the fobtest application by setting environment variables.
		I had to add a validity test of the _FobMessageList[] array
		  after spending hours trying to debug a problem that was
		  caused just by having the command names out of sync with
		  the byte sequences.
		I got buttons working in the test app.
		I tested an RTS line adjuster on IRIX based on some code I found
		  for Linux -- to use to reset the Flock.  But IRIX doesn't
		  allow the operation.

	30, January 2001 (Bill Sherman)
		Wrote a man page for 'fobtest' -- the first man page for FreeVR!

	31, January 2001 (Bill Sherman) -- mid mod
		Added a compile time switch to trust the sequential ordering of
		  unit numbers, or not.  For now we trust them.
		Got the beginnings of the FreeVR library interface working.
		  The receiver data is now successfully placed into the proper
		  6-sensor FreeVR inputs.  However, the scale is incorrect,
		  and the buttons aren't active.  Also there are a few more
		  arguments that need to be parsed.

	1, February 2001 (Bill Sherman)
		The basic functionality now works.  Still need to add a
		  "valuator" input type, and fix all the print statements to
		  work at a reasonable debug level.
		One other problem is that I haven't figured out a good
		  configuration to get the head in the proper orientation --
		  location works fine.

	2, February 2001 (Bill Sherman)
		- debug level printing
		- todo: valuator inputs

	7-8, February 2001 (Bill Sherman, w/ help from Stuart)
		- fixed the Euler conversion to work in normal CAVE setup
		- reversed the direction that markers go in the test app
			(up is now positive)

	3 May 2001 (Bill Sherman)
		I made a few minor changes to catch up to the general
		  vr_input.skeleton.c format.

...
	21 February 2002 (Bill Sherman)
		Some cleanups, so the code will compile without warnings.
		  (even some of the stuff that will need to change when I begin
		  implementing some items on the TODO list).

	17,20 May 2002 (Bill Sherman)
		Changed variable type names where possible (sensor6 to 6sensor,
		  etc), and renamed X,Y,Z,etc to VR_X, VR_Y, etc.

	11 September 2002 (Bill Sherman)
		Moved the control callback array into the _FobFunction()
		  callback.

	21-23 April 2003 (Bill Sherman)
		Updated to new input matching function code style.  Changed
		  "opaque" field to "aux_data".  Split _FobFunction()
		  into 5 functions.  Added new vrPrintStyle argument to
		  _FobPrintStruct() for the sake of the new "PrintAux" callback.

	22-24 May 2003 (Bill Sherman)
		Added code for setting the CRT-SYNC mode of the flock.  Added
		  parsing of three new arguments to set the "crtsync", "stream"
		  and "group" values.
		NOTE: the primary reason for adding the last two arguments is
		  that the default streamed/grouped mode which had generally
		  been working well on tested system did not work very well on
		  the Linux system currently being tested on.

	3 June 2003 (Bill Sherman)
		Now include "vr_enums.h" for the TEST_APP code.
		Added the address of the auxiliary data to the printout.
		Added the "system_pause_toggle" control callback.
		Added comments classifying the controls.

	2-4 February 2005 (Bill Sherman)
		Got standalone mode to work, and also single unit mode with both
		sensor and small transmitter on unit #1.  Standalone mode is
		determined from the FOB_EXAMINE_CONFIG return data, and when
		this is true, the AUTOCONFIG process is not necessary -- and
		the single unit has address 0 instead of 1.

		Unfortunately, I'm not at a place where I can now test ERT-based
		systems, much less multiple-ERT systems.  I also added a new
		"FOB_BAUD" environment variable for the test-app.  I added 
		"filters" command for FreeVR, and a "FOB_FILTERS" environment
		variable for the test application.

	18 September 2005 (Bill Sherman)
		Cleaned up some things to be consistent with vr_input.skeleton.c
		(and vr_input.fastrak.c).

	16 October 2009 (Bill Sherman)
		A quick fix to the _FobParseArgs() routine to handle the
		no-arguments case.

	2 September 2013 (Bill Sherman) -- adopting a consistent usage
		of str<...>cmp() functions (using logical negation).

	14 September 2013 (Bill Sherman)
		I added the "fobtest" man page to the end of this document.

		I am making use of the newly created SELFCTRL_DBGLVL for all
		the self-control outputs.  I then added some reasonable output
		to the "print_help" callback to print out what all the device's
		inputs are doing.

	15 September 2013 (Bill Sherman)
		Changed the "0x%p" format to the improved "%#p" format.

TODO:
	- implement the "angles" argument, which allows for quaternions or 3x3
		matrices to be used to pass the orientation from the Flock to
		FreeVR.

	- do Stuart's await_data trick in _FobReadOneUnit() to reduce CPU usage.
		[2/4/05: actually, running "fobtest" produced virtually no CPU
		usage at all,though interrupt-4 seemed to be firing constantly.]
		[2/4/05: and it seems I can't find any code containing
		"await_data" in my VR code archives.]

	- implement FreeVR arguments for the above new commands


	- I don't think this version is written to handle multiple transmitters.
		In particular, _FobAutoSetup() sets a "transmitter_num" for the
		global FOB structure, but this should probably be a per-unit
		field, as when there are multiple transmitter units, then each
		will have a different "transmitter_num" value.

	- determine a good (i.e. working) configuration, and perhaps also
		a step-by-step process others can follow.

	- Need to be able to handle address modes other than "normal"
		NOTE: first have to use hardware revision to determine if
		beyond-normal addressing is even possible (system will have
		trouble otherwise)

	- Some other commands that need to be implemented/tested:
		DONE: CRT Sync (pg 117-119)
		Filter mode (pg 74)
		DC Filter (pg 80-81)
		Bird Measurement Rate (pg 76)
		Position scaling (pg 74)
		FBB Reset (pg 95)

	- Need to implement _FobRTSReset().  Although, I'm not sure it will be
		possible under IRIX.  Even if not, it would be good to have
		Linux versions use their ability.

	- Need CLA's for the TEST_APP that allow setting parameters
		[currently have some environment variables]

	- Probably should redo any blocking serial reads such that they
		don't block, but do something reasonable if nothing
		received.  (perhaps using the aux->buf and aux->bufpos
		fields that are currently unused.)

	- Consider a function _FobReadCommandResponse(aux, command) that
		parses the incoming data based on the command that was
		sent.  Some of the commands will wait until the proper
		number of bytes are received.  Other commands will wait
		for a few attempts and then give up and return a failure
		code, while other commands will try a couple of times,
		and then just give up and move on.

RANT:
	Whose bright idea was it to use C for this thing?  Look at all those
	underscores!  Look at all those \ *  * \ blocks!  Look at all the #if 0
	thingys!  I think the person in charge of this project needs to learn a
	real programming language.

RANT REBUTTAL:
	I'm not sure what language that doesn't use underscores you'd be
	referring to, but whatever it is it probably wouldn't allow this
	library to compile on the ten or so OSes it now does.  And I can't
	believe one would prefer adding and deleting '//' to every line
	that is being conditionally compiled during development.  Good
	programmers write readable code, and can do good object-oriented
	programming without a crutch.


**************************************************************************/


#if !defined(FREEVR) && !defined(TEST_APP) && !defined(CAVE)
/* Choose how this code should be compiled (ie. for CAVE, standalone, etc). */
#  define	FREEVR
#  undef	TEST_APP
#  undef	CAVE
#endif

#undef	DEBUG_PRINT


#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "vr_serial.h"
#include "vr_debug.h"
#include "vr_utils.h"


#if defined (FREEVR)
#  include <math.h>		/* needed for fabs() */
#  include "vr_input.h"
#  include "vr_input.opts.h"
#  include "vr_math.h"		/* needed for vrEuler and vrAtoI() */
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
#  include <stdlib.h>		/* needed for getenv() & atoi() */
#  include "vr_serial.c"
#  include "vr_utils.c"
#  include "vr_enums.h"
   char *vrBinaryToString(unsigned int val, int len, char *str); /* from vr_math.h */

#  undef	vrDbgDo
#  define	vrDbgDo(n)	1
#endif

#ifdef DEBUG_PRINT		/* adjust the normal debugging output level */
#  undef	ASCFOB_DBGLVL
#  define	ASCFOB_DBGLVL	1
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


/*** local defines ***/

#define	DEFAULT_PORT	"/dev/input/fob"
#define	DEFAULT_BAUD	38400

#define	BUFSIZE		1024


	/******************************************************************/
	/*** definitions for interfacing with the Flock of Birds device ***/
	/***                                                            ***/

/* some of the timing delay values */
#define FOB_EXAM_DELAY		 50000
#define FOB_SET_DELAY		 30000	/* 100000 & 50000 definitely seem to work, trying lower */
#define	FOB_CLEARPAL_DELAY	 50000
#define	FOB_AUTOCONF_PREDELAY	300000
#define	FOB_RUN_PREDELAY	 50000


/* Flock of Birds BirdMouse bit-masks & indices */
/*   NOTE: the FoB doesn't really use masks, so check for equality */
#define DEVICE_BUTTON_x1x	0x10
#define DEVICE_BUTTON_x2x	0x30
#define DEVICE_BUTTON_x3x	0x70

#define DEVICE_BUTTONINDEX_x1x	0x00
#define DEVICE_BUTTONINDEX_x2x	0x01
#define DEVICE_BUTTONINDEX_x3x	0x02
#define DEVICE_BUTTONINDEX_x4x	0x03	/* this is a pseudo-button */

/* These masks are used to decode the unit configurations returned by FOB_EXAMINE_SYSTEMSTATUS */
typedef enum {
		FOB_CONFIG_FLY		= 0x80,
		FOB_CONFIG_RUN		= 0x40,
		FOB_CONFIG_SENSOR	= 0x20,
		FOB_CONFIG_ERT		= 0x10,
		FOB_CONFIG_XMTR3	= 0x08,
		FOB_CONFIG_XMTR2	= 0x04,
		FOB_CONFIG_XMTR1	= 0x02,
		FOB_CONFIG_XMTR0	= 0x01,
		FOB_CONFIG_NOUNIT	= 0x00
	} _FobConfigMask;

/* These masks are used to decode the unit status values returned by FOB_EXAMINE_BIRDSTATUS */
typedef enum {
		/* Mask for byte 0 */
		FOB_STATUS_MASTER	= 0x80,
		FOB_STATUS_INIT		= 0x40,
		FOB_STATUS_ERROR	= 0x20,
		FOB_STATUS_RUN		= 0x10,
		FOB_STATUS_HOSTSYNC	= 0x08,
		FOB_STATUS_EXPADR	= 0x04,
		FOB_STATUS_CRTSYNC	= 0x02,
		FOB_STATUS_NOSYNC	= 0x01,

		/* Mask for byte 1 */
		FOB_STATUS_FACTEST	= 0x80,
		FOB_STATUS_XOFF		= 0x40,
		FOB_STATUS_SLEEP	= 0x20,
		/* NOTE: bits 1-4 used for type of data to send */
		FOB_STATUS_STREAM	= 0x01
	} _FobStatusMask;


/***************************************************************/
/* Flock of Birds error codes                                  */
/*	NONE = no error occurred                               */
/*	FATAL = "Error is posted in system status, panel light */
/*		continuously blinks the error code, the Flock  */
/*		stops running."                                */
/*	WARNING1 = "Error is posted in system status, panel    */
/*		light blinks the error code once, the Flock    */
/*		resumes operation after the blinking stops."   */
/*	WARNING2 = "Error is posted in system status, no light */
/*		blinking, the Flock continues to run."         */
typedef struct {
		enum { NONE, FATAL, WARNING1, WARNING2 }	type;
		char						*desc;
	} _FobError;

static	_FobError	error_codes[] = {
		{ /*  0 */  NONE,	"no error" },
		{ /*  1 */  FATAL,	"System Ram Failure" },
		{ /*  2 */  FATAL,	"Non-Volatile Storage Write Failure" },
		{ /*  3 */  WARNING1,	"PCB Configuration Data Corrupt" },
		{ /*  4 */  WARNING1,	"Bird Transmitter Calibration Data Corrupt or Not Connected" },
		{ /*  5 */  WARNING1,	"Bird Sensor Calibration Data Corrupt or Not Connected" },
		{ /*  6 */  WARNING2,	"Invalid RS232 Command" },
		{ /*  7 */  WARNING2,	"Not an FBB master" },
		{ /*  8 */  WARNING2,	"No Birds accessible in Device List" },
		{ /*  9 */  WARNING2,	"Bird is Not Initialized" },
		{ /* 10 */  WARNING1,	"FBB Serial Port Receive Error - Intra Bird Bus" },
		{ /* 11 */  WARNING1,	"RS232 Serial Port Receive Error" },
		{ /* 12 */  WARNING1,	"FBB Serial Port Receive Error - FBB Host Bus" },
		{ /* 13 */  WARNING1,	"No FBB Command Response" },
		{ /* 14 */  WARNING1,	"Invalid FBB Host Command" },
		{ /* 15 */  FATAL,	"FBB Run Time Error" },
		{ /* 16 */  FATAL,	"Invalid CPU speed" },
		{ /* 17 */  WARNING1,	"No FBB Data" },
		{ /* 18 */  WARNING1,	"Illegal Baud Rate" },
		{ /* 19 */  WARNING1,	"Slave Acknowledge Error" },
		{ /* 20 */  FATAL,	"Intel 80186 CPU Errors (20)" },
		{ /* 21 */  FATAL,	"Intel 80186 CPU Errors (21)" },
		{ /* 22 */  FATAL,	"Intel 80186 CPU Errors (22)" },
		{ /* 23 */  FATAL,	"Intel 80186 CPU Errors (23)" },
		{ /* 24 */  FATAL,	"Intel 80186 CPU Errors (24)" },
		{ /* 25 */  FATAL,	"Intel 80186 CPU Errors (25)" },
		{ /* 26 */  FATAL,	"Intel 80186 CPU Errors (26)" },
		{ /* 27 */  FATAL,	"Intel 80186 CPU Errors (27)" },
		{ /* 28 */  WARNING1,	"CRT Synchronization" },
		{ /* 29 */  WARNING1,	"Transmitter Not accessible" },
		{ /* 30 */  WARNING1,	"Extended Range Transmitter Not Attached" },
		{ /* 31 */  WARNING2,	"CPU Time Overflow" },
		{ /* 32 */  WARNING1,	"Sensor Saturated" },
		{ /* 33 */  WARNING1,	"Slave Configuration" },
		{ /* 34 */  WARNING1,	"Watch Dog Timer" },
		{ /* 35 */  WARNING1,	"Over Temperature" },
		{ /* -- */  NONE,	(char *)NULL }
	};

#define MAX_ERRORS (sizeof(error_codes)/sizeof(_FobError)-1)


/***************************************/
/* Flock of Birds communications codes */
typedef	enum {
		FOB_POINT_MODE,		/* Albert renamed from FOB_POLL_MODE -- find out why */
		FOB_STREAM_MODE,
		FOB_RUN,
		FOB_SLEEP,

		FOB_BUTTON_READ,
		FOB_SET_BUTTON_MODE_ON,
		FOB_SET_BUTTON_MODE_OFF,

		FOB_EXAMINE_BIRDSTATUS,
		FOB_EXAMINE_REVNUM,
		FOB_EXAMINE_CRYSTAL,
		FOB_EXAMINE_FILTERS,
		FOB_EXAMINE_ERROR_CODE,
		FOB_EXAMINE_MODEL,
		FOB_EXAMINE_SERIAL_BIRD,
		FOB_EXAMINE_SERIAL_SENSOR,
		FOB_EXAMINE_SERIAL_TRANS,
		FOB_EXAMINE_SYSTEMSTATUS,
		FOB_EXAMINE_GROUP_MODE,
		FOB_EXAMINE_CONFIG,
		FOB_EXAMINE_XMTR_MODE,
		FOB_EXAMINE_HEMISPHERE,
		FOB_EXAMINE_ADDRESSING,

		FOB_NEXT_TRANSMITTER,
		FOB_AUTOCONFIG,

		FOB_SET_GROUP_MODE,
		FOB_SET_GROUP_MODE_ON,
		FOB_SET_GROUP_MODE_OFF,

		FOB_CRTSYNC_OFF,
		FOB_CRTSYNC_ON1,		/* this is for <= 72 Hz */
		FOB_CRTSYNC_ON2,		/* this is for >= 72 Hz */

		FOB_SET_HEMISPHERE_FORE,
		FOB_SET_HEMISPHERE_AFT,
		FOB_SET_HEMISPHERE_RIGHT,
		FOB_SET_HEMISPHERE_LEFT,
		FOB_SET_HEMISPHERE_LOWER,
		FOB_SET_HEMISPHERE_UPPER,

		FOB_SET_REPORT_RATE_ALL,
		FOB_SET_REPORT_RATE_HALF,
		FOB_SET_REPORT_RATE_EIGHTH,
		FOB_SET_REPORT_RATE_THIRTSCNTH,

		FOB_SET_OUTPUT_ANGLES,
		FOB_SET_OUTPUT_MATRIX,
		FOB_SET_OUTPUT_POSITION,
		FOB_SET_OUTPUT_POSITION_ANGLES,
		FOB_SET_OUTPUT_POSITION_MATRIX,
		FOB_SET_OUTPUT_POSITION_QUATERNION,
		FOB_SET_OUTPUT_QUATERNION,

		FOB_LASTCOMMAND		/* used to check bounds on array */
	} _FobCommand;


/**********************************************/
/* _FobComandInfo structure associates the command names with the */
/*    Flock of Birds command byte sequence, (incl. the number of  */
/*    bytes, a post-command delay, and a string with the name.    */
typedef struct {
		char	name[128];
		int	len;
		int	us_delay;
		char	msg[10];
	} _FobCommandInfo;

	/* NOTE: these are not proper C-strings, in that '\0' is a valid character,   */
	/*   So some of the character arrays require the individual characters to be  */
	/*   specified as an array of chars, rather than a C-string -- in particular, */
	/*   strings that contain the NUL value.                                      */

	/* NOTE: (also) that this list is not comprehensive.  It contains all the     */
	/*   FoB commands that are deemed (or were once deemed) necessary for normal  */
	/*   operation of the Flock.                                                  */
static	_FobCommandInfo	_FobMsgList[] = {
		{ "FOB_POINT_MODE",		1,	   20000,	"B" },	/* pg 102 -- 20000 is as low as it will work */
		{ "FOB_STREAM_MODE",		1,	   30000,	"@" },	/* pg 116 */
		{ "FOB_RUN",			1,	  200000,	"F" },	/* pg 114 -- 100000 wasn't long enough -- test after power-cycle */
		{ "FOB_SLEEP",			1,	  300000,	"G" },	/* pg 115 */

		{ "FOB_BUTTON_READ",		1,	   10000,	"N" },	/* page 68 */
		{ "FOB_SET_BUTTON_MODE_ON",	2,	FOB_SET_DELAY,	"M\x01" }, /* pg 67 */
		{ "FOB_SET_BUTTON_MODE_OFF",	2,	FOB_SET_DELAY,	"M\x00" },

		{ "FOB_EXAMINE_BIRDSTATUS",	2,	FOB_EXAM_DELAY,	{ 'O', '\x00' } }, /*pg 72*/
		{ "FOB_EXAMINE_REVNUM",		2,	FOB_EXAM_DELAY,	"O\x01" },
		{ "FOB_EXAMINE_CRYSTAL",	2,	FOB_EXAM_DELAY,	"O\x02" },
		{ "FOB_EXAMINE_FILTERS",	2,	FOB_EXAM_DELAY,	"O\x04" },
		{ "FOB_EXAMINE_ERROR_CODE",	2,	FOB_EXAM_DELAY,	"O\x0a" },
		{ "FOB_EXAMINE_MODEL",		2,	FOB_EXAM_DELAY,	"O\x0f" }, /* O^O */
		{ "FOB_EXAMINE_SERIAL_BIRD",	2,	FOB_EXAM_DELAY,	"O\x19" },
		{ "FOB_EXAMINE_SERIAL_SENSOR",	2,	FOB_EXAM_DELAY,	"O\x1a" },
		{ "FOB_EXAMINE_SERIAL_TRANS",	2,	FOB_EXAM_DELAY,	"O\x1b" },
		{ "FOB_EXAMINE_SYSTEMSTATUS",	2,	2*400000,	"O\x24" }, /* page 89 */
		{ "FOB_EXAMINE_GROUP_MODE",	2,	FOB_EXAM_DELAY,	"O\x23" }, /* page 88 */
		{ "FOB_EXAMINE_CONFIG",		2,	FOB_EXAM_DELAY,	"O\x32" },
		{ "FOB_EXAMINE_XMTR_MODE",	2,	FOB_EXAM_DELAY,	"O\x12" },
		{ "FOB_EXAMINE_HEMISPHERE",	2,	FOB_EXAM_DELAY,	"O\x16" },
		{ "FOB_EXAMINE_ADDRESSING",	2,	FOB_EXAM_DELAY,	"O\x13" }, /* page 85, >= v3.67 */

		{ "FOB_NEXT_TRANSMITTER",	2,	  100000,	{ '0', '\x10' }  }, /* was 12000 */
		{ "FOB_AUTOCONFIG",		3,	  400000,	{ 'P', '\x32', '\x00' }  }, /* minimal 600000 (or 300000?) us delay */
		{ "FOB_SET_GROUP_MODE",		3,	   50000,	"P\x23\x01" },	/* last byte changeable */
		{ "FOB_SET_GROUP_MODE_ON",	3,	   20000,	"P\x23\x01" },
		{ "FOB_SET_GROUP_MODE_OFF",	3,	   20000,	"P\x23\x00" },

		{ "FOB_CRTSYNC_OFF",		2,	FOB_SET_DELAY,	"A\x00" },
		{ "FOB_CRTSYNC_ON1",		2,	FOB_SET_DELAY,	"A\x01" },
		{ "FOB_CRTSYNC_ON2",		2,	FOB_SET_DELAY,	"A\x02" },

		{ "FOB_SET_HEMISPHERE_FORE",	3,	FOB_SET_DELAY,	{ 'L', '\x00', '\x00' } },
		{ "FOB_SET_HEMISPHERE_AFT",	3,	FOB_SET_DELAY,	{ 'L', '\x00', '\x01' } },
		{ "FOB_SET_HEMISPHERE_RIGHT",	3,	FOB_SET_DELAY,	{ 'L', '\x06', '\x00' } },
		{ "FOB_SET_HEMISPHERE_LEFT",	3,	FOB_SET_DELAY,	"L\x06\x01" },
		{ "FOB_SET_HEMISPHERE_LOWER",	3,	FOB_SET_DELAY,	{ 'L', '\x0C', '\x00' } },
		{ "FOB_SET_HEMISPHERE_UPPER",	3,	FOB_SET_DELAY,	"L\x0C\x01" },

		{ "FOB_SET_REPORT_RATE_ALL",	1,	FOB_SET_DELAY,	"Q" },	/* pg 111 */
		{ "FOB_SET_REPORT_RATE_HALF",	1,	FOB_SET_DELAY,	"R" },	/* pg 111 */
		{ "FOB_SET_REPORT_RATE_EIGHTH",	1,	FOB_SET_DELAY,	"S" },	/* pg 111 */
		{ "FOB_SET_REPORT_RATE_THIRTSCNTH",1,	FOB_SET_DELAY,	"T" },	/* pg 111 */

		{ "FOB_SET_OUTPUT_ANGLES",	1,	FOB_SET_DELAY,	"W"},	/* pg 61, 52 */
		{ "FOB_SET_OUTPUT_MATRIX",	1,	FOB_SET_DELAY,	"X"},	/* pp 98-99, 52 */
		{ "FOB_SET_OUTPUT_POSITION",	1,	FOB_SET_DELAY,	"V"},	/* pg 103, 52 */
		{ "FOB_SET_OUTPUT_POSITION_ANGLES",1,	FOB_SET_DELAY,	"Y"},	/* pg 104, 52 */
		{ "FOB_SET_OUTPUT_POSITION_MATRIX",1,	FOB_SET_DELAY,	"Z"},	/* pg 105, 52 */
		{ "FOB_SET_OUTPUT_POSITION_QUAT",1,	FOB_SET_DELAY,	"]"},	/* pg 106, 52 */
		{ "FOB_SET_OUTPUT_QUATERNION",	1,	FOB_SET_DELAY,	"\\"},	/* pg 107, 52 */

		{ "FOB_LASTCOMMAND",		0,	0, "" }
	};



/****************************************************************/
/*** auxiliary structures of the current data from the Flock. ***/

	/* _FobUnit: contains data for a single unit of the FoB system */
typedef struct {
		unsigned char	config;		/* configuration byte of the sensor */
		unsigned char	status[2];	/* from bird status */
		unsigned char	error;		/* from examine error */
		unsigned int	filter;		/* filter values (bit flags) */
		int		data_sensor;	/* number of the sensor from which data is reportedly from (should correspond directly to unit number) */
		float		data_pos[32];	/* incoming position data of the sensor */
		unsigned char	data_button;	/* incoming button data of the sensor */
		int		serial_sensor;	/* serial number of the unit's sensor */
		int		serial_trans;	/* serial number of the unit's transmitter */

		int		sn;		/* serial number of the box    (rev 3.67+) */
		int		sn_sensor;	/* serial number of the sensor (rev 3.71+) */
		int		sn_xmtr;	/* serial number of the xmtr   (rev 3.71+) */
		float		rev_number;		/* don't need both styles */
		int		rev_major;
		int		rev_minor;
		char		model[11];	/* max 10 bytes plus null */
		int		crystal_rate;

		_FobCommand	data_type;	/* type of information the flock will send */
		int		has_button;	/* whether a button is attached to this unit */
	} _FobUnit;

typedef struct {
		/* these are for interfacing with the hardware */
		int		fd;		/* was commhandle */
		char		*port;		/* name of serial port */
		int		baud_enum;	/* communication rate as an enumerated value */
		int		baud_int;	/* communication rate as the real value */
		int		open;		/* flag with Flock of Birds successfully open */

		/* these are for internal data parsing */
		unsigned char	buf[BUFSIZE];	/* TODO: this is not used in this code */
		int		bufpos;		/* TODO: this is not used in this code */
		char		version[256];	/* self-reported version of the device */
		char		op_params[256];	/* operating parameters of the device (according to it) */

		/* general information obtained from the system */
		unsigned char	address_mode;	/* the addressing mode of the flock */
		int		num_units;	/* number of birds that return status > 0 */
		int		max_units;	/* max number of birds based on addressing mode */
#define MAX_FOB_UNITS	30 /* make 126 for super-expanded address mode */
		_FobUnit	units[MAX_FOB_UNITS];

		int		auto_setup;	/* whether to use reported status to for config */
		int		standalone;	/* Flag indicating whether using a single unit in standalone mode */
		int		transmitter_unit;/* Bird unit number of the (single) transmitter */
		int		transmitter_num;/* Transmitter number in the Flock 0-3 */	/* TODO: shouldn't this a per-unit value??? */
		int		xxx_transmitter_count;/* number of short-range transmitters in the Flock */
		int		ert_transmitter_count;/* number of ERT transmitters in the Flock */
		int		sensor_count;	/* number of sensors in the Flock */
		unsigned short	transmitter_param;/* parameter for NEXT_TRANSMITTER Flock command */
		unsigned short	autoconfig_param;/* parameter for AUTOCONFIG Flock command */
		int		group_mode;	/* whether group mode should be used to get data */
		int		stream_mode;	/* whether stream mode should be used to get data */
		int		crt_sync;	/* requested CRT sync mode */
		int		filter_mode;	/* requested filter mode that the system should be set to */
		_FobCommand	hemisphere;	/* code of which hemisphere to use */
		_FobCommand	group_data_type;/* type of data flock is to report (when using gm)*/
				/* TODO: determine if the "group_..." fields are necessary/desirable */

		/* information about the current values */
		int		buttons;	/* TODO: this is not used -- incoming button info */
		int		timer;		/* TODO: this is not used -- incoming timer info  TODO: or is it a checksum?*/
		int		button_change;	/* TODO: this is not used -- boolean indicator if button values have changed*/
		int		receiver_change;/* TODO: this is not used -- boolean indicator of change in receiver values */

		/* information about how to filter the data for use with the VR library (FreeVR or CAVE) */
		float		scale_trans;	/* multiplier to scale from the FoB units (inches) */

#if defined(FREEVR)
		int		num_6sensors;	/* the number of 6-dof input sensors in config */
		vr6sensor	**sensor6_inputs;/* An array of pointers to the input structures */
		int		*sensor6_map;	/* An array of unit numbers from which the data should come */

		int		num_buttons;	/* the number of buttons in the configuration */
		vr2switch	**button_inputs;/* An array of pointers to the input structures */
		int		*button_map_unit;/* An array of unit numbers from which the data should come */
		int		*button_map_button;/* An array of numbers indicating the particular button */
		int		*button_values;	/* An array containing the current value of each (used for debouncing) */
#endif

	} _FobPrivateInfo;




	/************************************************************/
	/*** General NON public Flock of Birds interface routines ***/
	/************************************************************/


/******************************************************/
/* give the number of bytes to expect for a particular type of */
/*   output that the flock will be reporting.                  */
static int _FobOutputBytes(int output_type)
{
	int	data_bytes;

	switch (output_type) {
	case FOB_SET_OUTPUT_ANGLES:
		data_bytes = 6;
		break;

	case FOB_SET_OUTPUT_MATRIX:			/* TODO: IMPLEMENTED??? */
		data_bytes = 18;
		break;

	case FOB_SET_OUTPUT_POSITION:
		data_bytes = 6;
		break;

	case FOB_SET_OUTPUT_POSITION_ANGLES:
		data_bytes = 12;
		break;

	case FOB_SET_OUTPUT_POSITION_MATRIX:		/* TODO: IMPLEMENTED??? */
		data_bytes = 24;
		break;

	case FOB_SET_OUTPUT_POSITION_QUATERNION:	/* TODO: UNFINISHED!!! */
		data_bytes = 14;
		break;

	case FOB_SET_OUTPUT_QUATERNION:			/* TODO: UNFINISHED!!! */
		data_bytes = 8;
		break;

	default:
		data_bytes = 0;
		break;
	}

	return(data_bytes);
}


/******************************************************/
/* typename is used to specify a particular device among many that */
/*   share (more or less) the same protocol.  The typename is then */
/*   used to determine what specific features are available with   */
/*   this particular type of device.  In the case of the Flock of  */
/*   Birds, there currently is only one type: "fob".               */
static void _FobInitializeStruct(_FobPrivateInfo *aux, char *typename)
{
	aux->open = 0;

	aux->version[0] = '\0';
	aux->op_params[0] = '\0';

	aux->standalone = 0;
#if 1
	aux->auto_setup = 1;			/* use status info to determine setup */
#else
	aux->transmitter_unit = 2;		/* this is the default EVL CAVE setup */
	aux->transmitter_num = 0;
	aux->ert_transmitter_count = 1;
	aux->sensor_count = 3;
#endif

	aux->stream_mode = 1;			/* use stream mode by default */
	if (aux->stream_mode)
		aux->group_mode = 1;		/* use group mode when using stream mode */
	aux->crt_sync = -1;			/* don't set sync by default */
	aux->filter_mode = -1;			/* don't change filters by default */
	aux->hemisphere = FOB_SET_HEMISPHERE_LOWER;/* use lower hemisphere by default */
	aux->scale_trans = 1.0/12.0;		/* convert from inches to feet by default */


	aux->buf[0] = '\0';			/* TODO: this is not used in this code */
	aux->bufpos = 0;			/* TODO: this is not used in this code */
	aux->buttons = 0;			/* TODO: this is not used in this code */

	aux->timer = 0;				/* TODO: this is not used in this code */
	aux->button_change = 0;			/* TODO: this is not used in this code */

	aux->num_units = 0;
	aux->max_units = 14;

	/* everything else is zero'd by default */
}


/******************************************************/
static void _FobPrintStruct(FILE *file, _FobPrivateInfo *aux, vrPrintStyle style)
{
	int		count;

	vrFprintf(file, "FOB: Flock of Birds device internal structure (%#p):\n", aux);
	vrFprintf(file, "\r\tversion -- '%s'\n", aux->version);
	vrFprintf(file, "\r\toperating parameters -- '%s'\n", aux->op_params);
	vrFprintf(file, "\r\tfd = %d\n\tport = '%s'\n\tbaud = %d (%d)\n\topen = %d\n",
		aux->fd,
		aux->port,
		aux->baud_int, aux->baud_enum,
		aux->open);

	/* TODO: add some of the newly added fields (e.g. auto_setup, ..._param, etc.) */

#ifdef FREEVR /* { */

	vrFprintf(file, "\r\t%d buttons:\n", aux->num_buttons);
	vrFprintf(file, "\r\tbutton_values = %#p, button_map_unit = %#p, button_map_button = %#p, button_inputs = %#p\n",
		aux->button_values,
		aux->button_map_unit,
		aux->button_map_button,
		aux->button_inputs);
	for (count = 0; count < aux->num_buttons; count++) {
		vrFprintf(file, "\r\t\tbutton %d: value = %d, map_unit = %d, map_button = %d, input = %#p (type = %d)\n",	/* TODO: should map_unit have a '+1' after it?  -- See vr_input.asc_ms.c */
			count,
			aux->button_values[count],
			aux->button_map_unit[count],
			aux->button_map_button[count],
			aux->button_inputs[count],
			aux->button_inputs[count]->input_type);
	}

	vrFprintf(file, "\r\t%d 6-sensors:\n", aux->num_6sensors);
	vrFprintf(file, "\r\t6sensor_values = %#p, 6sensor_map = %#p, 6sensor_inputs = %#p\n",
#if 0 /* TODO: why are we not printing the actual values here? [10/16/2009] */
		aux->sensor6_values,
#else
		0x0,
#endif
		aux->sensor6_map,
		aux->sensor6_inputs);
	for (count = 0; count < aux->num_6sensors; count++) {
		vrFprintf(file, "\r\t\t6-sensor %d: value = [%.2f %.2f %.2f  %.2f %.2f %.2f], map_unit = %d, input = %#p (type = %d)\n",	/* TODO: should map_unit have a '+1' after it?  -- See vr_input.asc_ms.c */
			count,
#if 0 /* TODO: why are we not printing the actual values here? [10/16/2009] */
			aux->sensor6_values[count].t[VR_X],
			aux->sensor6_values[count].t[VR_Y],
			aux->sensor6_values[count].t[VR_Z],
			aux->sensor6_values[count].r[VR_AZIM],
			aux->sensor6_values[count].r[VR_ELEV],
			aux->sensor6_values[count].r[VR_ROLL],
#else
			-1.0, -1.0, -1.0, -1.0, -1.0, -1.0,
#endif
			aux->sensor6_map[count],
			aux->sensor6_inputs[count],
			aux->sensor6_inputs[count]->input_type);
	}

#endif /* } FREEVR */

	vrFprintf(file, "\r\tscale_trans = %f\n", aux->scale_trans);
	vrFprintf(file, "\r\themisphere = %d\n", aux->hemisphere);
}


/******************************************************/
/* Info that indicates the Flock is not operating is printed in Red */
static void _FobPrintUnitStatus(FILE *file, unsigned char* status_bytes)
{
static	char		print_buf[128];
	unsigned char	status;
	int		runMode;

	status = status_bytes[1];
	vrFprintf(file, "    %s : ", vrBinaryToString((unsigned int)(status), 8, print_buf));

	runMode = status & FOB_STATUS_RUN;

	if (status & FOB_STATUS_MASTER)	{
					vrFprintf(file, "master ");
		/* init+ is normal for the master */
		if (status & FOB_STATUS_INIT)	vrFprintf(file, "init+ ");
		else				vrFprintf(file, RED_TEXT "init- " NORM_TEXT);
	} else {
					vrFprintf(file, "slave ");
		/* init- is normal for the slave */
		if (status & FOB_STATUS_INIT)	vrFprintf(file, RED_TEXT "init+ " NORM_TEXT);
		else				vrFprintf(file, "init- ");
	}


	if (status & FOB_STATUS_ERROR)	vrFprintf(file, RED_TEXT "err+ " NORM_TEXT);
	else				vrFprintf(file, "err- ");

	if (status & FOB_STATUS_RUN)	vrFprintf(file, "run+ ");
	else				vrFprintf(file, RED_TEXT "run- " NORM_TEXT);

	if (status & FOB_STATUS_HOSTSYNC)vrFprintf(file, "hostsync+ ");
	else				 vrFprintf(file, "hostsync- ");

	if (status & FOB_STATUS_EXPADR)	vrFprintf(file, BOLD_TEXT "expand_addr " NORM_TEXT);
	else				vrFprintf(file, "normal_addr ");

	if (status & FOB_STATUS_CRTSYNC)vrFprintf(file, "crtsync+ ");
	else				vrFprintf(file, "crtsync- ");

	if (status & FOB_STATUS_NOSYNC)	vrFprintf(file, "anysync- ");
	else				vrFprintf(file, "anysync+ ");

	status = status_bytes[0];

	vrFprintf(file, "\n    %s : ", vrBinaryToString((unsigned int)(status), 8, &print_buf[8]));

	if (status & FOB_STATUS_FACTEST)vrFprintf(file, "test&bird ");
	else				vrFprintf(file, "birdonly ");

	if (status & FOB_STATUS_XOFF)	vrFprintf(file, "XOFF ");
	else				vrFprintf(file, "XON ");

	if (runMode) {
		if (status & FOB_STATUS_SLEEP)	vrFprintf(file, BOLD_TEXT "sleep " NORM_TEXT);
		else				vrFprintf(file, "run ");
	}

	/* shifts bits 1-4 down by one and zero out 4-7 */
	switch ((status >> 1) & 0x0F) {
	case 1:	vrFprintf(file, "pos ");	break;
	case 2:	vrFprintf(file, "angle ");	break;
	case 3:	vrFprintf(file, "matrix ");	break;
	case 4:	vrFprintf(file, "pos/angle ");	break;
	case 5:	vrFprintf(file, "pos/matrix ");	break;
	case 6:	vrFprintf(file, "factory ");	break;
	case 7:	vrFprintf(file, "quat ");	break;
	case 8:	vrFprintf(file, "pos/quat ");	break;
	}

	if (status & FOB_STATUS_STREAM)	vrFprintf(file, "stream ");
	else				vrFprintf(file, "point ");

	vrFprintf(file, "\n");
}


/******************************************************/
/* info that indicates the presence of a transmitter is printed in Bold */
/* info that indicates the Flock is not operating is printed in Red */
static void _FobPrintUnitConfig(FILE *file, unsigned char config)
{
	char	buffer[9];	/* NOTE: must be one more than the number of bits to print */

	vrFprintf(file, "%s : ", vrBinaryToString((unsigned int)(config), 8, buffer));

	/* flying */
	if (config & FOB_CONFIG_FLY)	vrFprintf(file, "fly+ ");
	else				vrFprintf(file, RED_TEXT "fly- " NORM_TEXT);

	 /* running */
	if (config & FOB_CONFIG_RUN)	vrFprintf(file, "run+ ");
	else				vrFprintf(file, RED_TEXT "run- " NORM_TEXT);

	/* sensor */
	if (config & FOB_CONFIG_SENSOR)	vrFprintf(file, "sensor+ ");
	else				vrFprintf(file, BOLD_TEXT "sensor- " NORM_TEXT);

	/* extended range transmitter  */
	if (config & FOB_CONFIG_ERT)	vrFprintf(file, BOLD_TEXT "ert+ " NORM_TEXT);
	else				vrFprintf(file, "ert- ");

	/* transmitter 3 */
	if (config & FOB_CONFIG_XMTR3)	vrFprintf(file, BOLD_TEXT "xmtr3+ " NORM_TEXT);
	else				vrFprintf(file, "xmtr3- ");

	/* transmitter 2 */
	if (config & FOB_CONFIG_XMTR2)	vrFprintf(file, BOLD_TEXT "xmtr2+ " NORM_TEXT);
	else				vrFprintf(file, "xmtr2- ");

	/* transmitter 1 */
	if (config & FOB_CONFIG_XMTR1)	vrFprintf(file, BOLD_TEXT "xmtr1+ " NORM_TEXT);
	else				vrFprintf(file, "xmtr1- ");

	/* transmitter 0 */
	if (config & FOB_CONFIG_XMTR0)	vrFprintf(file, BOLD_TEXT "xmtr0+ " NORM_TEXT);
	else				vrFprintf(file, "xmtr0- ");

	vrFprintf(file, "\n");
}


/******************************************************/
static void _FobPrintFlockStatus(FILE *file, _FobPrivateInfo *aux)
{
	int	unit;
	int	error_code;

	vrPrintf("address mode: ");
	switch (aux->address_mode) {
	case 0x00:
		vrPrintf("normal(14)\n");
		break;
	case 0x01:
		vrPrintf("expanded(30)\n");
		break;
	case 0x03:
		vrPrintf("super-expanded(126)\n");
		break;
	default:
		vrPrintf(RED_TEXT "unknown(0)\n" NORM_TEXT);
		break;
	}

	for (unit = 0; unit < aux->max_units; unit++) {
		if (!aux->units[unit].config) continue;

		vrFprintf(file, "Bird unit %d (sn: %d, rev: %4.2f, model: '%s', crystal: %dmhz, filter: 0x%04x)\n",
			(aux->standalone ? 0 : unit+1), aux->units[unit].sn, aux->units[unit].rev_number,
			aux->units[unit].model, aux->units[unit].crystal_rate, aux->units[unit].filter);

		_FobPrintUnitConfig(file, aux->units[unit].config);
		_FobPrintUnitStatus(file, aux->units[unit].status);
		error_code = (int)(aux->units[unit].error);
		if (error_code > MAX_ERRORS || error_code < 0)
			vrFprintf(file, "    Bad value for last reported error (%d), something is wrong.\n" NORM_TEXT, error_code);
		else	vrFprintf(file, "    Last Reported Error: %d -- %s%s\n" NORM_TEXT, error_code, (error_code > 0 ? RED_TEXT : ""), error_codes[error_code].desc);

		vrFprintf(file, "\n");
	}
	vrFprintf(file, "\n");
}


/**************************************************************************/
static void _FobPrintHelp(FILE *file, _FobPrivateInfo *aux)
{
	int	count;		/* looping variable */

	/* TODO: mention the lack of button independence here. */

#if 0 || defined(TEST_APP)
	vrFprintf(file, BOLD_TEXT "FOB: Sorry, Flock of Birds - print_help control not yet implemented.\n" NORM_TEXT);
#else
	vrFprintf(file, BOLD_TEXT "FOB - inputs:" NORM_TEXT "\n");
	for (count = 0; count < aux->num_buttons; count++) {
		if (aux->button_inputs[count] != NULL) {
			vrFprintf(file, "\t%s -- %s%s\n",
				aux->button_inputs[count]->my_object->desc_str,
				(aux->button_inputs[count]->input_type == VRINPUT_CONTROL ? "control:" : ""),
				aux->button_inputs[count]->my_object->name);
		}
	}

	/* NOTE: Flock of birds has no valuator inputs */

#ifdef NOT_YET_IMPLEMENTED
	for (count = 0; count < MAX_VALUATORS; count++) {
		if (aux->valuator_inputs[count] != NULL) {
			vrFprintf(file, "\t%s -- %s%s\n",
				aux->valuator_inputs[count]->my_object->desc_str,
				(aux->valuator_inputs[count]->input_type == VRINPUT_CONTROL ? "control:" : ""),
				aux->valuator_inputs[count]->my_object->name);
		}
	}
#endif /* NOT_YET_IMPLEMENTED */
	for (count = 0; count < aux->num_6sensors; count++) {
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
/* TODO: implement this function */
static int _FobRTSReset(_FobPrivateInfo *aux)
{
	vrFprintf(stderr, BOLD_TEXT "FOB: Flock is not responding.\n" NORM_TEXT);
	vrFprintf(stderr, BOLD_TEXT "Performing an RTS Reset " NORM_TEXT);
	vrFprintf(stderr, RED_TEXT "-- NOT!  TODO: Still needs to be implemented.\n" NORM_TEXT);

	return 0;
}


/******************************************************/
static int _FobClearPalate(_FobPrivateInfo *aux)
{
static	char	buffer[BUFSIZE];
	int	bytes_read;
#ifdef DEBUG_PRINT
	int	count;
#endif

	bytes_read = vrSerialRead(aux->fd, buffer, BUFSIZE);
#if defined(DEBUG_PRINT) || 0/*TODO: this should be a debug level -- what about for the test app? */
	if (vrDbgDo(ASCFOB_DBGLVL + RARE_DBGLVL)) {
		vrPrintf("FOB: clear palate -- bytes_read = %2d", bytes_read);
		if (bytes_read > 0) {
			vrPrintf(" -- ");
			for (count = 0; count < bytes_read; count++) {
				vrPrintf(" %02x",  buffer[count]);
			}
		}
		vrPrintf("\n");
	}
#endif

	vrSleep(FOB_CLEARPAL_DELAY);

	return bytes_read;
}


/******************************************************/
static char _FobSendCommand(_FobPrivateInfo *aux, _FobCommand command)
{
	_FobCommandInfo	*command_info;
	char		return_code = '\0';

	/* if command doesn't exist, then return immediately */
	if (command >= FOB_LASTCOMMAND) {
		vrPrintf(RED_TEXT "No command (%d), nothing sent to FoB\n" NORM_TEXT, (int)command);
		return (return_code);
	}

	command_info = &(_FobMsgList[command]);

#ifdef DEBUG_PRINT	/* TODO: this should be a debug level -- what about for test app? */
	vrPrintf("FOB: Sending command '%s'\n", command_info->name);
#else
	vrDbgPrintfN(ASCFOB_DBGLVL + RARE_DBGLVL, "FOB: Sending command '%s'\n", command_info->name);
#endif

	vrSerialWrite(aux->fd, command_info->msg, command_info->len);
#if 1
	vrSerialDrain(aux->fd);
#endif
	vrSleep(command_info->us_delay);

	/* TODO: should we check for error status after every command? */
	/*   Well, we certainly shouldn't do it after a stream command. */
	/*   Probably best to require specific code to do error checking. */

	return (return_code);
}


/******************************************************/
static char _FobSendCommandAddr(_FobPrivateInfo *aux, _FobCommand command, int addr)
{
static	char		newMsg[32];
	_FobCommandInfo	*command_info;
	char		return_code = '\0';

	/* if command doesn't exist, then return immediately */
	if (command >= FOB_LASTCOMMAND) {
		vrPrintf(RED_TEXT "No command (%d), nothing sent to FoB\n" NORM_TEXT, (int)command);
		return (return_code);
	}

	command_info = &(_FobMsgList[command]);

#ifdef DEBUG_PRINT	/* TODO: this should be a debug level -- what about for test app? */
	vrPrintf("FOB: Sending command '%s' to unit #%d\n", command_info->name, addr);
#else
	vrDbgPrintfN(ASCFOB_DBGLVL + RARE_DBGLVL, "FOB: Sending command '%s' to unit #%d\n", command_info->name, addr);
#endif

	newMsg[0] = 0xF0 + (unsigned char)addr;		/* RS232TOFBB command */
	memcpy(newMsg+1, command_info->msg, command_info->len);

	vrSerialWrite(aux->fd, newMsg, command_info->len+1);
#if 1
	vrSerialDrain(aux->fd);
#endif
	vrSleep(command_info->us_delay);

	/* TODO: should we check for error status after every command? */
	/*   Well, we certainly shouldn't do it after a stream command. */
	/*   Probably best to require specific code to do error checking. */

	return (return_code);
}


/******************************************************/
/* TODO: if error is #13, then perhaps do an expanded-error-code check (1999, pg. 82) */
static int _FobGetError(_FobPrivateInfo *aux, int unit)
{
static	char	buffer[10];
	int	bytes_read;
	int	error_code;

	/* TODO: also send to other units? */
	if (unit == 0) {
		/* send to Master */
		_FobSendCommand(aux, FOB_EXAMINE_ERROR_CODE);
	} else {
		/* send to specified unit */
		_FobSendCommandAddr(aux, FOB_EXAMINE_ERROR_CODE, unit);
	}
	bytes_read = vrSerialRead(aux->fd, buffer, 1);

	if (bytes_read == 1) {
		error_code = buffer[0];
		if (error_code == 0) {
#ifdef DEBUG_PRINT	/* TODO: this should be a debug level -- what about for test app? */
			vrPrintf(RED_TEXT "FOB: Error code (unit %d):%3d (?)\n" NORM_TEXT, unit, error_code);	/* TODO: for some reason, a second '?' screws up the output. */
#else
			vrDbgPrintfN(ASCFOB_DBGLVL + RARE_DBGLVL, RED_TEXT "FOB: Error code (unit %d):%3d (?)\n" NORM_TEXT, unit, error_code);	/* TODO: for some reason, a second '?' screws up the output. */
#endif
		} else if (error_code > MAX_ERRORS) {
			error_code = -2;
			vrPrintf(RED_TEXT "FOB: Error code (unit %d):%3d (?)\n" NORM_TEXT, unit, error_code);
		} else {
			vrPrintf(RED_TEXT "FOB: Error code (unit %d):%3d (%s)\n" NORM_TEXT, unit, error_code, error_codes[error_code].desc);
		}
	} else {
		error_code = -1;
		vrPrintf(RED_TEXT "FOB: Error code (unit %d): --\n" NORM_TEXT, unit);
	}

	return error_code;
}


/******************************************************/
static void _FobGetUnitStatus(_FobPrivateInfo* aux, int unit)
{
static	char	buffer[128];
	int	bytes_read;

	/* Bird Status -- see pages 72-73 of the 1999 FoB Guide */
	if (aux->standalone)
		_FobSendCommandAddr(aux, FOB_EXAMINE_BIRDSTATUS, 0);
	else	_FobSendCommandAddr(aux, FOB_EXAMINE_BIRDSTATUS, unit+1);
	bytes_read = vrSerialReadNbytes_Block(aux->fd, buffer, 2);

	if (bytes_read < 2) {
		aux->units[unit].status[0] = 0x00;
		aux->units[unit].status[1] = 0x00;
	} else {
		aux->units[unit].status[0] = buffer[0];
		aux->units[unit].status[1] = buffer[1];
	}

	if (aux->standalone)
		_FobSendCommandAddr(aux, FOB_EXAMINE_ERROR_CODE, 0);
	else	_FobSendCommandAddr(aux, FOB_EXAMINE_ERROR_CODE, unit+1);
	bytes_read = vrSerialReadNbytes_Block(aux->fd, buffer, 1);
	aux->units[unit].error = buffer[0];
}


/******************************************************/
/* TODO: WARNING, this currently puts the flock in point (vs. stream) mode, */
/*    and doesn't bother to put things back if previously in stream mode.   */
static int _FobGetFlockStatus(_FobPrivateInfo* aux)
{
static	char	buffer[BUFSIZE];
	int	bytes_read;
	int	count;
	int	unit;
	int	unit_address;		/* command address for the unit -- except for standalone is unit+1 */

	_FobClearPalate(aux);

	/********************/
	/* Clear the palate */
	_FobClearPalate(aux);

	/**************************/
	/* Get system information */

#if 1 /* TODO: this code should only be executed on systems with late-model firmware */
	/* address mode */
	_FobSendCommand(aux, FOB_EXAMINE_ADDRESSING);
#  if 1
	bytes_read = vrSerialRead(aux->fd, buffer, 1);
	if (bytes_read < 1) {
		vrPrintf(RED_TEXT "FOB: Warning: No response to address mode request, assuming normal.\n" NORM_TEXT);
		buffer[0] = 0x00;
	}
#  else
	/* NOTE: this will hang when the unit doesn't understand this command */
	bytes_read = vrSerialReadNbytes_Block(aux->fd, buffer, 1);
#  endif
	aux->address_mode = buffer[0];
	switch (aux->address_mode) {
	case 0x00:
		aux->max_units = 14;
		vrPrintf(RED_TEXT "FOB: address mode normal.\n" NORM_TEXT);
		break;
	case 0x01:
		aux->max_units = 30;
		vrPrintf(RED_TEXT "FOB: address mode extended.\n" NORM_TEXT);
		break;
	case 0x03:
		aux->max_units = 126;
		vrPrintf(RED_TEXT "FOB: address mode super-extended.\n" NORM_TEXT);
		break;
	default:
		vrPrintf(RED_TEXT "FOB: Warning: invalid response to address mode request (%d), assuming normal.\n" NORM_TEXT, aux->address_mode);

		aux->max_units = 14;
		break;
	}
#else
	vrPrintf(RED_TEXT "FOB: Warning: Currently not requesting address mode, assuming normal.\n" NORM_TEXT);
	aux->address_mode = 0x00;
	aux->max_units = 14;
#endif

	if (aux->max_units > MAX_FOB_UNITS) {
		vrPrintf(RED_TEXT "FOB: Warning: addressing mode is greater than hardcoded unit maximum -- change value for MAX_FOB_UNITS.\n" NORM_TEXT);
		aux->max_units = MAX_FOB_UNITS;
	}


	/***********************************/
	/* Check the initial configuration */
	_FobSendCommand(aux, FOB_EXAMINE_CONFIG); /* NOTE: returns 5 bytes in Normal address mode, 7 for Extended, and 19 for Super-Extended*/
	switch (aux->address_mode) {
	case 0x00:	bytes_read = vrSerialReadNbytes_Block(aux->fd, buffer,  5); break;	/* normal address mode */
	case 0x01:	bytes_read = vrSerialReadNbytes_Block(aux->fd, buffer,  7); break;	/* extended address mode */
	case 0x03:	bytes_read = vrSerialReadNbytes_Block(aux->fd, buffer, 19); break;	/* super-extended address mode */
	}

	if (buffer[0] == 0) {
		aux->standalone = 1;
		vrPrintf(RED_TEXT "FOB System is a standalone unit.\n" NORM_TEXT);
	}


	/**************************/
	/* Get unit system status * (see page 89 of the 1999 FoB Guide) */
	/* This reads the status for all possible units.  The number is */
	/*   based on the addressing mode that has been selected.       */

	/* Get Status from all units in System */
	_FobClearPalate(aux);
	_FobSendCommand(aux, FOB_EXAMINE_SYSTEMSTATUS);
	bytes_read = vrSerialReadNbytes_Block(aux->fd, buffer, aux->max_units);
	vrPrintf("FOB System status:\n");

	/* Loop over the maximum number of units (the lesser of MAX_FOB_UNITS and those allowed by the addressing mode), and determine which ones exist and are active */
	aux->num_units = 0;
	for (unit = 0; unit < aux->max_units; unit++) {
		aux->units[unit].config = buffer[unit];
		if (buffer[unit]) {
			aux->num_units++;
			vrFprintf(stderr, "    unit %2d: ", unit+1);
			_FobPrintUnitConfig(stderr, buffer[unit]);
		}
	}
	if ((aux->num_units == 0 && !aux->standalone) || aux->num_units == aux->max_units) {
		vrFprintf(stderr, RED_TEXT "FOB: %d units?  This usually means an error occurred.  Will retry status request.\n" NORM_TEXT, aux->num_units);
		_FobClearPalate(aux);
		_FobSendCommand(aux, FOB_EXAMINE_SYSTEMSTATUS);
		bytes_read = vrSerialReadNbytes_Block(aux->fd, buffer, aux->max_units);
		vrPrintf("System status:\n");
		aux->num_units = 0;
		for (unit = 0; unit < aux->max_units; unit++) {
			aux->units[unit].config = buffer[unit];
			if (buffer[unit]) {
				aux->num_units++;
				vrFprintf(stderr, "    unit %2d: ", unit+1);
				_FobPrintUnitConfig(stderr, buffer[unit]);
			}
		}
	}


	/*************************************/
	/* configure standalone special case */
	if (aux->standalone) {
		if (aux->num_units != 0) {
			vrFprintf(stderr, RED_TEXT "FOB in standalone mode > 0 units (%d)?  This usually means an error occurred.  Will forge ahead anyway.\n" NORM_TEXT, aux->num_units);
		}

		aux->num_units = 1;

		/* Hard-code the configuration value for a standalone unit */
		aux->units[0].config = 0x61;			/* Binary: 01100001 -- running, has a sensor, transmitter #0 */
		vrFprintf(stderr, "    standalone unit: ");
		_FobPrintUnitConfig(stderr, aux->units[0].config);
	}

	/* If we still only have 0 units, something is definitely wrong! */
	if (aux->num_units == 0)
		return (-1);

	/**********************************/
	/* Get each existing unit's setup */
	/* TODO: this needs to be changed to support expanded addressing */
	vrFprintf(stderr, "FOB: Getting configuration data from the %d birds... (this might take a moment)...\n", aux->num_units);
	for (unit = 0; unit < aux->max_units; unit++) {

		vrFprintf(stderr, "%d", (unit+1)%10);

		if (!aux->units[unit].config && !aux->standalone)
			/* no bird at that address */
			continue;

		if (aux->standalone)
			unit_address = 0;	/* standalone unit is 0 */
		else	unit_address = unit + 1;

		/* Revision -- see page 73 of the 1999 FoB Guide */
		_FobSendCommandAddr(aux, FOB_EXAMINE_REVNUM, unit_address);
		bytes_read = vrSerialReadNbytes_Block(aux->fd, buffer, 2);
		if (bytes_read < 2) {
			aux->units[unit].rev_number = 0.0f;
			aux->units[unit].rev_major = 0;
			aux->units[unit].rev_minor = 0;
		} else {
			aux->units[unit].rev_major = buffer[0];
			aux->units[unit].rev_minor = buffer[1];
			aux->units[unit].rev_number = (float)aux->units[unit].rev_major + (float)aux->units[unit].rev_minor/100.0f;
		}
		vrFprintf(stderr, ".");

		/* Model type -- see page 82 of the 1999 FoB Guide */
		_FobSendCommandAddr(aux, FOB_EXAMINE_MODEL, unit_address);
		bytes_read = vrSerialReadNbytes_Block(aux->fd, aux->units[unit].model, 10);
		for (count = bytes_read-1; count >= 0; count--) {
			if (aux->units[unit].model[count] != ' ' &&
			    aux->units[unit].model[count] != '\0')
				break;
		}
		aux->units[unit].model[count+1] = '\0';
		vrFprintf(stderr, ".");

		/* Crystal Speed -- see page 73 of the 1999 FoB Guide */
		_FobSendCommandAddr(aux, FOB_EXAMINE_CRYSTAL, unit_address);
		bytes_read = vrSerialReadNbytes_Block(aux->fd, buffer, 2);
		if (bytes_read < 2)
			aux->units[unit].crystal_rate = -1;
		else	aux->units[unit].crystal_rate = buffer[0] + buffer[1] * 256;
		vrFprintf(stderr, ".");

		/* Filter On/Off Status -- see page 114 of the 2002 PDF FoB Guide */
		_FobSendCommandAddr(aux, FOB_EXAMINE_FILTERS, unit_address);
		bytes_read = vrSerialReadNbytes_Block(aux->fd, buffer, 2);
		if (bytes_read < 2)
			aux->units[unit].filter = -1;
		else	aux->units[unit].filter = buffer[0] + buffer[1] * 256;
		vrFprintf(stderr, ".");

		/* Serial Number -- see page 87 of the 1999 FoB Guide */
		if (aux->units[unit].rev_minor >= 67) {
			_FobSendCommandAddr(aux, FOB_EXAMINE_SERIAL_BIRD, unit_address);
			bytes_read = vrSerialRead(aux->fd, buffer, 2);
			if (bytes_read < 2)
				aux->units[unit].sn = -1;
			else	aux->units[unit].sn = buffer[0] + buffer[1] * 256;
		}
		vrFprintf(stderr, ".");

		/* Transmitter & Sensor Serial Number -- see page 87 of the 1999 FoB Guide */
		if (aux->units[unit].rev_minor >= 71) {
			/* Sensor Serial Number -- see page 87 of the 1999 FoB Guide */
			_FobSendCommandAddr(aux, FOB_EXAMINE_SERIAL_SENSOR, unit_address);
			bytes_read = vrSerialRead(aux->fd, buffer, 2);
			if (bytes_read < 2)
				aux->units[unit].sn_sensor = -1;
			else	aux->units[unit].sn_sensor = buffer[0] + buffer[1] * 256;

			/* Transmitter Serial Number -- see page 87 of the 1999 FoB Guide */
			_FobSendCommandAddr(aux, FOB_EXAMINE_SERIAL_TRANS, unit_address);
			bytes_read = vrSerialRead(aux->fd, buffer, 2);
			if (bytes_read < 2)
				aux->units[unit].sn_xmtr = -1;
			else	aux->units[unit].sn_xmtr = buffer[0] + buffer[1] * 256;

			vrFprintf(stderr, ".");
		}

#if 0 /* I don't have access to any brand new systems, but probably need */
	/*   to check the firmware version before issueing this command. */
		/* Hemisphere -- see page 86 of the 1999 FoB Guide */
		/* TODO: does this matter for non-transmitters? */
		/* NOTE: this may only be available for firmware versions >= 3.67 */

		_FobSendCommandAddr(aux, FOB_EXAMINE_HEMISPHERE, 2);	/* TODO: currently hardcoded to EVL-style setup */
		vrSleep(400000);	/* BS: added 10/3/2000 */
		bytes_read = vrSerialReadNbytes_Block(aux->fd, buffer, 2);

		vrPrintf("hemisphere: ");
		switch (buffer[0]) {
		case 0x00: if (buffer[1]) vrPrintf("back\n");	else vrPrintf("front\n");	break;
		case 0x0c: if (buffer[1]) vrPrintf("upper\n");	else vrPrintf("lower\n");	break;
		case 0x06: if (buffer[1]) vrPrintf("left\n");	else vrPrintf("right\n");	break;
		}
#endif

		/* Bird Status -- see pages 72-73 of the 1999 FoB Guide */
		_FobGetUnitStatus(aux, unit);
		vrFprintf(stderr, ".");

		if (aux->standalone)
			break;
	}
	vrFprintf(stderr, "\n\n");

	return(0);
}


/****************************************************************************/
/* This function determines the setup for the flock based on the previously */
/*   obtained status bytes.                                                 */
/* The "transmitter_param" field will contain the value of the parameter    */
/*   needed for the NEXT_TRANSMITTER command to the Flock of Birds.         */
/* NOTE: this function will require some changes in order to handle the     */
/*   multiple-transmitter configurations.                                   */
static void _FobAutoSetup(_FobPrivateInfo* aux)
{
	int		unit;
	unsigned char	config;

	aux->transmitter_num = -1;
	aux->transmitter_unit = -1;
	aux->xxx_transmitter_count = 0;	/* I forget what code is used for non-ERT transmitters */
	aux->ert_transmitter_count = 0;
	aux->sensor_count = 0;

	for (unit = 0; unit < aux->max_units; unit++) {
		config = aux->units[unit].config;

		if (config & FOB_CONFIG_SENSOR)
			aux->sensor_count++;

		if (config & (FOB_CONFIG_ERT)) {
			aux->ert_transmitter_count++;

			if (aux->ert_transmitter_count == 1) {
				aux->transmitter_unit = unit+1;
			} else {
				vrFprintf(stderr, RED_TEXT "FOB: Warning, a second transmitter found, and at the moment, we can only handle one!\n" NORM_TEXT);
			}
		}

		if (config & FOB_CONFIG_XMTR0) {
			aux->transmitter_num = 0;

			/* if unit is not an ERT unit, then it's a non-ERT unit */
			if (!(config & (FOB_CONFIG_ERT))) {
				aux->transmitter_unit = unit+1;
				aux->xxx_transmitter_count++;
			}
		}

		if (config & FOB_CONFIG_XMTR1) {
			aux->transmitter_num = 1;

			/* if unit is not an ERT unit, then it's a non-ERT unit */
			if (!(config & (FOB_CONFIG_ERT))) {
				aux->transmitter_unit = unit+1;
				aux->xxx_transmitter_count++;
			}
		}

		if (config & FOB_CONFIG_XMTR2) {
			aux->transmitter_num = 2;

			/* if unit is not an ERT unit, then it's a non-ERT unit */
			if (!(config & (FOB_CONFIG_ERT))) {
				aux->transmitter_unit = unit+1;
				aux->xxx_transmitter_count++;
			}
		}

		if (config & FOB_CONFIG_XMTR3) {
			aux->transmitter_num = 3;

			/* if unit is not an ERT unit, then it's a non-ERT unit */
			if (!(config & (FOB_CONFIG_ERT))) {
				aux->transmitter_unit = unit+1;
				aux->xxx_transmitter_count++;
			}
		}
	}

	if (aux->xxx_transmitter_count > 1) {
		vrFprintf(stderr, RED_TEXT "FOB: Warning, multiple short-range transmitters found, and at the moment, we can only handle one!\n" NORM_TEXT);
	}

	if (aux->xxx_transmitter_count > 0 && aux->ert_transmitter_count > 0) {
		vrFprintf(stderr, RED_TEXT "FOB: Warning, both short-range and extended range transmitters found, and at the moment, we can only handle one!\n" NORM_TEXT);
	}
}


/******************************************************/
static void _FobConvertBytesToFloats(int numBytes, unsigned char* src, short* dst)
{
	int	count;

#ifdef DEBUG_PRINT
static	char	print_buf[17];

	if (vrDbgDo(ASCFOB_DBGLVL + RARE_DBGLVL)) {
		vrFprintf(stderr, "_FobConvertBytesToFloats:\n    ");

		for (count = 0; count < numBytes; count++)
			vrFprintf(stderr, "   %2x    ", src[count]);
		vrFprintf(stderr, "\n    ");

		for (count = 0; count < numBytes; count++)
			vrFprintf(stderr, "%s ", vrBinaryToString((unsigned int)(src[count]), 8, print_buf));
		vrFprintf(stderr, "\n    ");
	}
#endif

	/* change the phasing bit to a 0 */
	src[0] &= 0x7f;

#ifdef DEBUG_PRINT
	if (vrDbgDo(ASCFOB_DBGLVL + RARE_DBGLVL)) {
		for (count = 0; count < numBytes; count++)
			vrFprintf(stderr, "%s ", vrBinaryToString((unsigned int)(src[count]), 8, print_buf));
		vrFprintf(stderr, "\n    ");
	}
#endif

	/* shift each LS byte left one */
	for (count = 0; count < numBytes; count += 2)
		src[count] <<= 1;

#ifdef DEBUG_PRINT
	if (vrDbgDo(ASCFOB_DBGLVL + RARE_DBGLVL)) {
		for (count = 0; count < numBytes; count++)
			vrFprintf(stderr, "%s ", vrBinaryToString((unsigned int)(src[count]), 8, print_buf));
		vrFprintf(stderr, "\n    ");
	}
#endif

	/* combine each byte pair into a word and left shift the word */
	for (count = 0; count < numBytes/2; count++)
		dst[count] = (((unsigned short)src[2*count+1]<<8) + (unsigned short)src[2*count]) << 1;

#ifdef DEBUG_PRINT
	if (vrDbgDo(ASCFOB_DBGLVL + RARE_DBGLVL)) {
		for (count = 0; count<3; count++)
			vrFprintf(stderr, "%s  ", vrBinaryToString((unsigned int)(dst[count]), 16, print_buf));
		vrFprintf(stderr, "\n    ");

		for (count = 0; count<3; count++)
			vrFprintf(stderr, "%d ", (dst[count]));
		vrFprintf(stderr, "\n");
	}
#endif

}


/******************************************************/
static void _FobReadOneUnit(_FobPrivateInfo *aux, int unit, _FobCommand data_type)
{
static	char		buffer[BUFSIZE];
static	short		raw_data[12];

static	float		trans_scale = 144.0f / 32768.0f; /* 144 is for ERT */
static	float		angle_scale = 180.0f / 32768.0f;
static	float		value_scale =   1.0f / 32768.0f;

	_FobUnit	*read_unit;		/* the unit in which to put the incoming data */
	int		bytes_read;
	int		data_bytes;		/* number of bytes for 'data_type'  */
	int		reported_unit;		/* sensor from which data says its from */
	int		button_value;		/* value of a button (if one exists) */
	int		count;

	/*********************************/
	/* get number of bytes to expect */
#define DATATYPETEST /* I think we can just use the unit's data type info, in group and non-group modes */

#ifdef DATATYPETEST
	/* NOTE: The danger in this is that we assume we known which sensor */
	/*   we're reading before getting the confirmation from the last    */
	/*   byte.  Of course, there's nothing that can be done about this  */
	/*   short of having the folks at ascension create a protocol that  */
	/*   makes sense (ie. having the unit number be the first byte),    */
	/*   that would be a lot to ask.                                    */
	data_type = aux->units[unit].data_type;
#endif

	data_bytes = _FobOutputBytes(data_type);


	/* TODO: should do Stuart's await_data trick here to reduce CPU usage */

	/****************************/
	/* wait for start-byte flag */
	do {
		bytes_read = vrSerialReadNbytes_Block(aux->fd, buffer, 1);
	} while ((bytes_read > 0) && !(buffer[0] & 0x80));


	/******************************************************************/
	/* read up to the expected number of bytes, and convert to floats */
	bytes_read += vrSerialReadNbytes_Block(aux->fd, &(buffer[bytes_read]), data_bytes-bytes_read);

	/* units with activated button mode send an extra byte for the button values */
	/* NOTE: Again dangerous if it's possible for sensor order to be */
	/*   unpredictable -- fortunately it doesn't seem to be, which   */
	/*   begs the question of why waste bandwidth on the extra byte? */
	if (aux->units[unit].has_button) {
		bytes_read += vrSerialReadNbytes_Block(aux->fd, &(buffer[bytes_read]), 1);
		button_value = (int)buffer[bytes_read-1];
	}

	/* group mode sends an extra byte for the unit number */
	if (aux->group_mode) {
		bytes_read += vrSerialReadNbytes_Block(aux->fd, &(buffer[bytes_read]), 1);
		reported_unit = (int)buffer[bytes_read-1];
#define	TRUST_SEQUENTIAL_UNITS /* NOTE: this is related to the DATATYPETEST switch above */
#ifdef TRUST_SEQUENTIAL_UNITS
		read_unit = &(aux->units[unit]);
#else
		if (unit != reported_unit-1) {
			/* In this case, the reported unit didn't come in the expected sequence. */
			/*   This is more likely to be an indication that there is some other    */
			/*   error with the incoming data and/or parsing.                        */
			vrErrPrintf("FOB: Warning: unexpected unit number received (%d, expected %d)\n",
				reported_unit, unit+1);
		} else {
			/* If we were to assume the reported unit is correct, and it isn't */
			/*   then we could end up accessing invalid memory.                */
			read_unit = &(aux->units[reported_unit-1]);	/* subtract 1 to be zero-based */
		}
#endif
	} else {
		read_unit = &(aux->units[unit]);
		reported_unit = -1;
	}
	read_unit->data_sensor = reported_unit;		/* i.e. in group mode, this should always match */
#ifdef DEBUG_PRINT
	vrDbgPrintfN(ASCFOB_DBGLVL + RARE_DBGLVL, "  Unit = %d, reported_unit = %d\n", unit+1, reported_unit);
#endif


	_FobConvertBytesToFloats(data_bytes, (unsigned char *)buffer, raw_data);


	/*****************************************************************/
	/* put the scaled, converted floats into the data position array */
	switch (data_type) {
	case FOB_SET_OUTPUT_ANGLES:
		for (count = 0; count < 3; count++)
			read_unit->data_pos[count] = raw_data[count] * angle_scale;
		break;

	case FOB_SET_OUTPUT_MATRIX:
		for (count = 0; count < 9; count++)
			read_unit->data_pos[count] = raw_data[count] * value_scale;
		break;

	case FOB_SET_OUTPUT_POSITION:
		for (count = 0; count < 3; count++)
			read_unit->data_pos[count] = raw_data[count] * trans_scale;
		break;

	case FOB_SET_OUTPUT_POSITION_ANGLES:
		for (count = 0; count < 3; count++)
			read_unit->data_pos[count] = raw_data[count] * trans_scale;
		for (count = 3; count < 6; count++)
			read_unit->data_pos[count] = raw_data[count] * angle_scale;
		break;

	case FOB_SET_OUTPUT_POSITION_MATRIX:
		for (count = 0; count < 3; count++)
			read_unit->data_pos[count] = raw_data[count] * trans_scale;
		for (count = 3; count < 12; count++)
			read_unit->data_pos[count] = raw_data[count] * value_scale;
		break;

	case FOB_SET_OUTPUT_POSITION_QUATERNION: /* TODO: UNFINISHED!!! */
		for (count = 0; count < 3; count++)
			read_unit->data_pos[count] = raw_data[count] * trans_scale;
		break;

	case FOB_SET_OUTPUT_QUATERNION: /* TODO: UNFINISHED!!! */
		break;

	default:
		vrPrintf("FOB: Bird unit %d has unknown datatype (%d).\n", unit+1, read_unit->data_type);
		break;
	}


	/**********************/
	/* Read button values */
	if (read_unit->has_button) {
		if (aux->group_mode) {
			/* In group mode, we've already read the value */
			read_unit->data_button = button_value;
		} else {
			/* In polled, non-group mode we need to request it */
			/* See page 68 of the 1999 Fob Guide */
			if (aux->standalone)
				_FobSendCommandAddr(aux, FOB_BUTTON_READ, 0);
			else	_FobSendCommandAddr(aux, FOB_BUTTON_READ, unit+1);
			bytes_read = vrSerialReadNbytes_Block(aux->fd, buffer, 1);
			read_unit->data_button = buffer[0];
		}
	}

	/*************************************/
	/* Read the Error code for this unit */
	/*   (non-group, poll-mode only) */
	if (!aux->group_mode && !aux->stream_mode) {
		_FobClearPalate(aux);
		_FobGetError(aux, 0);
	}
}


/******************************************************/
static void _FobReadInput(_FobPrivateInfo *aux)
{
	int		unit;
	_FobCommand	data_type;		/* type of data expected from Flock */

#ifndef DATATYPETEST
	if (aux->group_mode)
		data_type = aux->group_data_type;
#endif

	/* send request for group of data when in group-polled mode */
	if (aux->group_mode && !aux->stream_mode) {
		_FobSendCommand(aux, FOB_POINT_MODE);
	}

	for (unit = 0; unit < aux->max_units; unit++) {
		/* skip unit if no sensor (there is no point in sending the    */
		/*    POINT_MODE command to or reading data from transmitters) */
		if ((aux->units[unit].config & FOB_CONFIG_SENSOR) == 0)
			continue;

		/* send request for data when in non-group polled mode */
		if (!aux->group_mode && !aux->stream_mode) {
#ifndef DATATYPETEST
			data_type = aux->units[unit].data_type;
#endif
			if (aux->standalone)
				_FobSendCommandAddr(aux, FOB_POINT_MODE, 0);
			else	_FobSendCommandAddr(aux, FOB_POINT_MODE, unit+1);
		}

		/* now read and parse the data for one unit */
		_FobReadOneUnit(aux, unit, data_type);
	}

	return;
}


/******************************************************/
/* _FobInitializeDevice() is called in the OPEN phase     */
/*   of input interface -- after the types of inputs have */
/*   been determined (during the CREATE phase).           */
static int _FobInitializeDevice(_FobPrivateInfo *aux)
{
static	char		buffer[BUFSIZE];	/* where we read the serial data into */
static	char		print_buf[256];		/* used for making binary strings */
	_FobUnit	*transunit;		/* used for making the version & param strings */
	int		bytes_read;		/* number of bytes read from serial port */
	int		unit;			/* counter for looping through the bird units */
	int		unit_address;		/* command address for the unit -- except for standalone is unit+1 */

	if (aux == NULL)
		return(-1);

	/******************************************************************************/
	/* First, verify that (some of) the underlying data structures are consistent */
	if (strcmp(_FobMsgList[FOB_LASTCOMMAND].name, "FOB_LASTCOMMAND") != 0) {
		/* NOTE: this generally happens when a new command is added to */
		/*   the _FobMsgList[] array, but the corresponding enumerated */
		/*   value is not added to the _FobCommand type definition.    */
		vrErrPrintf(RED_TEXT "The Flock of Birds source code is not consistent -- '_FobMsgList[]'\n" NORM_TEXT);
		return(-1);
	}

vrPrintf("2:trans unit %02x, num %02x\n", aux->transmitter_unit, aux->transmitter_num);

	/***************************************/
	/* Make initial contact with the Flock */

	/* Get any error codes that the device is currently reporting. */
	/* NOTE: if first request for error returns -1, then the flock */
	/*   is not responding, so we should do an RTS reset.  If the  */
	/*   Flock doesn't respond on the second request, then assume  */
	/*   we can't fix things in software, and suggest a powercycle.*/
	if (_FobGetError(aux, 0) == -1) {
		_FobRTSReset(aux);
		/* TODO: perhaps also try an FBBreset command */
	}
	if (_FobGetError(aux, 0) == -1) {
		vrErrPrintf(RED_TEXT "FOB: Flock is not responding.  Try a powercycle and try again!\n" NORM_TEXT);
		return(-1);
	}


	/* Go into "polled mode" for communicating with the device */
	vrPrintf("FOB: Going into 'polled mode'\n");
	_FobSendCommand(aux, FOB_POINT_MODE);
	_FobClearPalate(aux);		/* This is a must after a POINT_MODE command */

	_FobSendCommand(aux, FOB_SET_GROUP_MODE_OFF);
	_FobSendCommand(aux, FOB_SLEEP);	/* only send to master */


	/* This is where most of the information about the FOB setup is gathered */
	/* TODO: we may want to do an _FobRTSReset(aux) here too, if an error is returned */
	if (_FobGetFlockStatus(aux) < 0) {
		vrPrintf("_FobInitializeDevice(): Flock Status < 0, returning.\n");
		return(-1);
	}

vrPrintf("3:trans unit %02x, num %02x\n", aux->transmitter_unit, aux->transmitter_num);

	/* print out the birds' status */
	if (vrDbgDo(ASCFOB_DBGLVL))
		_FobPrintFlockStatus(stderr, aux);


	if (!aux->standalone) {
		/***********************************************/
		/* Once we have the status information, we can */
		/*   automatically determine the Flock setup   */
		if (aux->auto_setup)
			_FobAutoSetup(aux);

		/* value = aaaa00nn, aaaa = transmitter unit, nn = transmitter number (0-3) */
vrPrintf("4:trans unit %02x, num %02x\n", aux->transmitter_unit, aux->transmitter_num);
		aux->transmitter_param = ((aux->transmitter_unit & 0x0F) << 4) | (aux->transmitter_num & 0x03);
vrPrintf("just set transmitter_param to %02x\n", aux->transmitter_param);

		/* value = number of bird units (sensors plus transmitters) */
		aux->autoconfig_param = aux->ert_transmitter_count + aux->sensor_count;

		if (vrDbgDo(ASCFOB_DBGLVL)) {
			vrPrintf(BOLD_TEXT "FOB: %d ERT transmitters (unit %d), %d sensors\n" NORM_TEXT,
				aux->ert_transmitter_count, aux->transmitter_unit, aux->sensor_count);
			vrPrintf(BOLD_TEXT "FOB: tranmistter_param = %02x, autoconfig = %d\n" NORM_TEXT,
				aux->transmitter_param, aux->autoconfig_param);
		}

		/********************************/
		/* Configure the Flock of Birds */

		/* Set the transmitter (See page 100 of 1999 Flock Guide) */
		/* NOTE: if we don't do this, then we're assuming the transmitter is unit #1 (bad!) */
		/* TODO: figure out what needs to be done for multiple transmitters */
		_FobMsgList[FOB_NEXT_TRANSMITTER].msg[1] = aux->transmitter_param;
		/* only send Next Transmitter command this when trans is not master */
		if (_FobMsgList[FOB_NEXT_TRANSMITTER].msg[1] != 0x00 && _FobMsgList[FOB_NEXT_TRANSMITTER].msg[1] != 0x10) {
			vrPrintf("Sending NEXT_TRANSMITTER command %02x\n", _FobMsgList[FOB_NEXT_TRANSMITTER].msg[1]);
			_FobSendCommand(aux, FOB_NEXT_TRANSMITTER);
			_FobGetError(aux, 0);
		}

		/* NOTE: auto config requires a 300ms pre and post command wait period */
		vrSleep(FOB_AUTOCONF_PREDELAY);
		_FobMsgList[FOB_AUTOCONFIG].msg[2] = aux->autoconfig_param;
		vrDbgPrintfN(AALWAYS_DBGLVL, BOLD_TEXT "FOB: Doing Flock of Birds Auto-configuration.\n" NORM_TEXT);
		_FobSendCommand(aux, FOB_AUTOCONFIG);	/* send the autoconfig command to the Master */
		/* (TODO: does Master = unit #1 or Transmitter?) -- probably the latter in which case the Addr stuff is unnece [10/19/2000 -- neither actually, it's the one with the serial connection] */

		_FobGetError(aux, 0);
		_FobClearPalate(aux);
	}

	/***************************/
	/* Check the configuration */
	/* TODO: I'm not sure this changes because of an executed autoconfig command, so perhaps */
	/*   this should just be done initially (including the debug printing), and not here.    */
	_FobSendCommand(aux, FOB_EXAMINE_CONFIG);
	/* NOTE: returns 5 bytes in Normal address mode, 7 for Extended, and 19 for Super-Extended*/

	switch (aux->address_mode) {
	case 0x00:	/* normal address mode */
		bytes_read = vrSerialReadNbytes_Block(aux->fd, buffer, 5);
		/*vrDbgPrintfN(ASCFOB_DBGLVL, */vrPrintf("FOB: autoconfig (%d bytes): %02x %s:%s %s:%s\n", bytes_read, buffer[0],
			vrBinaryToString((unsigned int)(buffer[2]), 8, print_buf),
			vrBinaryToString((unsigned int)(buffer[1]), 8, print_buf+10),
			vrBinaryToString((unsigned int)(buffer[4]), 8, print_buf+20),
			vrBinaryToString((unsigned int)(buffer[3]), 8, print_buf+30));
		/* if buffer[0-4] is 0x00 here, then we are connected to a unit in standalone mode. */
		break;
	case 0x01:	/* extended address mode */
		bytes_read = vrSerialReadNbytes_Block(aux->fd, buffer, 7);
		/*vrDbgPrintfN(ASCFOB_DBGLVL, */vrPrintf("FOB: autoconfig (%d bytes): %02x %s:%s %s:%s %s:%s\n", bytes_read, buffer[0],
			vrBinaryToString((unsigned int)(buffer[2]), 8, print_buf),
			vrBinaryToString((unsigned int)(buffer[1]), 8, print_buf+10),
			vrBinaryToString((unsigned int)(buffer[4]), 8, print_buf+20),
			vrBinaryToString((unsigned int)(buffer[3]), 8, print_buf+30),
			vrBinaryToString((unsigned int)(buffer[6]), 8, print_buf+40),
			vrBinaryToString((unsigned int)(buffer[5]), 8, print_buf+50));
		/* if buffer[0-4] is 0x00 here, then we are connected to a unit in standalone mode. */
		break;
	case 0x03:	/* super-extended address mode */
		bytes_read = vrSerialReadNbytes_Block(aux->fd, buffer, 19);
		break;
	}

#if 0
	/******************************************************/
	/* Get the mode in which the transmitter is operating */

	/* NOTE: I haven't gotten this to work on 4.67 or 4.64 models of the ERC */
	vrSleep(600000);
	_FobSendCommandAddr(aux, FOB_EXAMINE_XMTR_MODE, aux->transmitter_unit);
	vrSleep( 5000000);
	_FobGetError(aux, 0);
	_FobClearPalate(aux);
	_FobGetError(aux, aux->transmitter_unit);
	_FobClearPalate(aux);
	vrSleep(15000000);
	bytes_read = vrSerialRead(aux->fd, buffer, BUFSIZE);
	if (bytes_read == 0)
		buffer[0] = 0xFF;

	if (vrDbgDo(ASCFOB_DBGLVL) {
		vrPrintf("FOB: xmtr mode: ");
		switch (buffer[0]) {
		case 0x00:	vrPrintf("non-pulsed\n");	break;
		case 0x01:	vrPrintf("pulsed\n");		break;
		case 0x02:	vrPrintf("cool-down\n");	break;
		case 0xFF:	vrPrintf("no response\n");	break;
		default:	vrPrintf("unknown response\n");	break;
		}
	}
#endif


	/****************************************/
	/* Set the parameters for all the units */

	for (unit = 0; unit < MAX_FOB_UNITS; unit++) {

		if (aux->standalone)
			unit_address = 0;	/* standalone unit is 0 */
		else	unit_address = unit + 1;

		/* only handle units that are in fly mode */
		/* TODO: we may want to be more specific later -- ie not include transmitters */
		if (aux->units[unit].config /* & FOB_CONFIG_FLY */) {
			aux->units[unit].data_type = FOB_SET_OUTPUT_POSITION_ANGLES;

			/* set type of data to report */
			_FobSendCommandAddr(aux, aux->units[unit].data_type, unit_address);

			/* set hemisphere to use */
			_FobSendCommandAddr(aux, aux->hemisphere, unit_address);

			_FobClearPalate(aux);

			/* turn on button mode */
			/* NOTE: do for group mode only, though it probably wouldn't do harm when not */
			if (aux->group_mode) {
				if (aux->units[unit].has_button)
					_FobSendCommandAddr(aux, FOB_SET_BUTTON_MODE_ON, unit_address);
				else	_FobSendCommandAddr(aux, FOB_SET_BUTTON_MODE_OFF, unit_address);
				_FobGetError(aux, 0);
			}
		}
	}

	/* setup group mode if specified, otherwise in poll mode */
	if (aux->group_mode) {
		vrPrintf("FOB: Going into 'group mode'\n");
		_FobSendCommand(aux, FOB_SET_GROUP_MODE_ON);
		_FobGetError(aux, 0);

		aux->group_data_type = FOB_SET_OUTPUT_POSITION_ANGLES;
	}


	/*******************/
	/* Begin Operation */

	vrSleep(FOB_RUN_PREDELAY);
	_FobSendCommand(aux, FOB_RUN); /* only send to master */
	vrDbgPrintfN(ASCFOB_DBGLVL, "FOB: Post FOB_RUN\n");

	/******************************************************************/
	/* Reget the current status of the system to verify it is running */
	_FobGetError(aux, 0);
	_FobClearPalate(aux);

	for (unit = 0; unit < aux->max_units; unit++) {
		vrDbgPrintfN(ASCFOB_DBGLVL, "%d", (unit+1)%10);

		if (!aux->units[unit].config)
			/* no bird unit at that address */
			continue;

		/* Bird Status -- see pages 72-73 of the 1999 FoB Guide */
		_FobGetUnitStatus(aux, unit);
	}
	vrDbgPrintfN(ASCFOB_DBGLVL, "\n");
	if (vrDbgDo(ASCFOB_DBGLVL))
		_FobPrintFlockStatus(stderr, aux);


	/****************************************/
	/* Create the version and param strings */

	/* TODO: it might be better to have something like:   */
	/*	ERC:  model 6DERC, revision 4.64  FOB: model 6DFOB, revision 3.64 */
	/*   and as part of this print a warning message if all the sensors don't */
	/*   agree -- and same goes for the crystal rate w/ both sensors and xmtrs*/
	if (aux->standalone)
		transunit = &(aux->units[0]);				/* use the transmitter unit */
	else	transunit = &(aux->units[aux->transmitter_unit-1]);	/* use the transmitter unit */
	sprintf(aux->version, "model: %s, revision: %d.%d",
		transunit->model, transunit->rev_major, transunit->rev_minor);
	sprintf(aux->op_params, "Crystal: %dMHz", transunit->crystal_rate);


	/*****************************/
	/* Set the system parameters */

	/* TODO: set the CRT Sync configuration */
	switch (aux->crt_sync) {
	case 0x0:
		_FobSendCommand(aux, FOB_CRTSYNC_OFF);
		break;
	case 0x1:
		_FobSendCommand(aux, FOB_CRTSYNC_ON1);
		break;
	case 0x2:
		_FobSendCommand(aux, FOB_CRTSYNC_ON2);
		break;
	default:
		break;
	}

	/* Set the filter mode */
	if (aux->filter_mode >= 0) {
		vrPrintf("FOB: setting Filter mode to 0x%02x\n", aux->filter_mode);
		_FobSendCommand(aux, aux->filter_mode);
	}


	/***************************************/
	/* Go into stream mode -- if specified */
	if (aux->stream_mode) {
		/* Set the report rate */
		vrDbgPrintfN(ASCFOB_DBGLVL, BOLD_TEXT "FOB: Setting report rate\n" NORM_TEXT);
		/* TODO: the report rate should be a config-parameter */
		_FobSendCommand(aux, FOB_SET_REPORT_RATE_HALF);

		vrDbgPrintfN(ASCFOB_DBGLVL, BOLD_TEXT "FOB: Going into Stream Mode\n" NORM_TEXT);
		/* Go into stream mode -- just send to Master unit */
		_FobSendCommand(aux, FOB_STREAM_MODE);
		vrDbgPrintfN(ASCFOB_DBGLVL, BOLD_TEXT "FOB: Post Stream Mode\n" NORM_TEXT);
	}


	/*****************************/
	/* read the first input data */
	_FobReadInput(aux);

	return 0;
}


/******************************************************/
static int _FobResetDevice(_FobPrivateInfo *aux)
{
	if (aux == NULL)
		return -1;

	/* NOTE: command sequence is basically from the VRPN open-source code */

	/* TODO: go into Poll mode "B" */

	/* TODO: FBB auto configure "P,0x32,<num units>" */

	/* (perhaps) TODO: (ie. VRPN does this) set data request for each sensor */

	/* (perhaps) TODO: (ie. VRPN does this) set hemisphere for each sensor */

	/* TODO: Get system status "O,0x24" */
	/*	and then print out status -- see vrpn_Tracker_Flock::reset() */

	/* TODO: Go into stream mode "@" */

	/*------------------------------*/
	/* NOTE: here is another possible sequence to try: */
	/*   RTS reset     */
	/*   poll-mode 'B' */
	/*   FBB reset     */
	/*   probably just re-initialize everything: */
	/*  	(perhaps next-transmitter) */
	/*	auto-configure */
	/*	position data */
	/*	hemisphere */
	/*	... */
	/*	stream mode */

	return 0;
}


/******************************************************/
static int _FobCloseDevice(_FobPrivateInfo *aux)
{
	if (aux == NULL)
		return -1;

	/* NOTE: command sequence is basically from the VRPN open-source code */

	/* go into Poll mode "B" */
	_FobSendCommand(aux, FOB_POINT_MODE);

	/* go into sleep mode "G" */
	_FobSendCommand(aux, FOB_SLEEP);

	/* close the serial port */
	vrSerialClose(aux->fd);

	return 0;
}


/******************************************************/
static unsigned int _FobButtonValue(char *buttonname)
{
	switch (buttonname[0]) {
	case 'l':
	case 'L':
	case '1':
		return DEVICE_BUTTONINDEX_x1x;
	case 'm':
	case 'M':
	case '2':
		return DEVICE_BUTTONINDEX_x2x;
	case 'r':
	case 'R':
	case '3':
		return DEVICE_BUTTONINDEX_x3x;
	case 'z':
	case 'Z':
	case '4':
		/* This is the pseudo "zero-rotation" button */
		return DEVICE_BUTTONINDEX_x4x;
	}

	return -1;
}


	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/


#if defined(FREEVR) /* { */
/***********************************************************************/
/*** Functions for FreeVR access of Flock of Birds devices for input ***/
/***********************************************************************/


	/*************************************/
	/***  FreeVR NON public routines   ***/
	/*************************************/


/*********************************************************/
static void _FobParseArgs(_FobPrivateInfo *aux, char *args)
{
static	char	*hemi_choices[] = { "fore", "aft", "right", "left", "lower", "upper" };
static	int	hemi_values[] = {
			FOB_SET_HEMISPHERE_FORE,
			FOB_SET_HEMISPHERE_AFT,
			FOB_SET_HEMISPHERE_RIGHT,
			FOB_SET_HEMISPHERE_LEFT,
			FOB_SET_HEMISPHERE_LOWER,
			FOB_SET_HEMISPHERE_UPPER  };
	float	scale_value = -1.0;

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

	/****************************************************************************************/
	/** Format: "hemisphere" "=" { "fore" | "aft" | "right" | "left" | "lower" | "upper" } **/
	/****************************************************************************************/
	vrArgParseChoiceInteger(args, "hemisphere", (int *)&(aux->hemisphere), hemi_choices, hemi_values);

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

	/*******************************************/
	/** Argument format: "filter" "=" number **/
	/*******************************************/
	vrArgParseInteger(args, "filter", &(aux->filter_mode));

	/*******************************************/
	/** Argument format: "crtsync" "=" number **/
	/*******************************************/
	vrArgParseInteger(args, "crtsync", &(aux->crt_sync));

	/******************************************/
	/** Argument format: "stream" "=" number **/
	/******************************************/
	vrArgParseInteger(args, "stream", &(aux->stream_mode));

	/*****************************************/
	/** Argument format: "group" "=" number **/
	/*****************************************/
	vrArgParseInteger(args, "group", &(aux->group_mode));


	/** TODO: other arguments to parse go here **/
}


/************************************************************/
static void _FobGetData(vrInputDevice *devinfo)
{
	_FobPrivateInfo	*aux = (_FobPrivateInfo *)devinfo->aux_data;
	int		count;
	float		scale_trans = aux->scale_trans;


	/*******************/
	/* gather the data */
	_FobReadInput(aux);


	/*************/
	/** buttons **/
	for (count = 0; count < aux->num_buttons; count++) {
		vr2switch	*current_2switch;
		_FobUnit	*current_fobunit;
		int		mapped_6sensor;
		int		new_value;

		current_2switch = aux->button_inputs[count];
		current_fobunit = &(aux->units[aux->button_map_unit[count]]);
		mapped_6sensor = aux->button_map_unit[count];

		if (current_fobunit == NULL) {
			vrErrPrintf("_FobGetData: Attempt to read button from a non existent FobUnit (%d)\n", aux->button_map_unit[count]);
		} else {
			switch(aux->button_map_button[count]) {
			case DEVICE_BUTTONINDEX_x1x:
				new_value = (current_fobunit->data_button == DEVICE_BUTTON_x1x);
				break;
			case DEVICE_BUTTONINDEX_x2x:
				new_value = (current_fobunit->data_button == DEVICE_BUTTON_x2x);
				break;
			case DEVICE_BUTTONINDEX_x3x:
				new_value = (current_fobunit->data_button == DEVICE_BUTTON_x3x);
				break;

			case DEVICE_BUTTONINDEX_x4x:
				/* This is the zero-rotation "button" */
#define ZR_THRESHOLD 20.0
				new_value = (fabs(aux->units[mapped_6sensor].data_pos[3]) < ZR_THRESHOLD) &&
					1; /* TODO && with datum 4 and 5 */
				break;

			default:
				/* TODO: print a warning message here */
				break;
			}
		}

		/* If value has changed, do some work */
		if (new_value != aux->button_values[count]) {
			aux->button_values[count] = new_value;
			if (current_2switch != NULL) {
				switch (current_2switch->input_type) {
				case VRINPUT_BINARY:
					vrAssign2switchValue(current_2switch, new_value);
					break;
				case VRINPUT_CONTROL:
					vrCallbackInvokeDynamic(((vrControl *)(current_2switch))->callback, 1, new_value);
					break;
				default:
					vrErrPrintf(RED_TEXT "_FobGetData: Unable to handle button inputs that aren't Binary or Control inputs\n" NORM_TEXT);
					break;
				}
			}
		}
	}

	/***************/
	/** 6-sensors **/
	for (count = 0; count < aux->num_6sensors; count++) {
		vr6sensor	*current_6sensor;
		vrEuler		new_value;
		int		mapped_6sensor;
		vrMatrix	tmpmat;

		current_6sensor = aux->sensor6_inputs[count];
		mapped_6sensor = aux->sensor6_map[count];
		new_value.t[VR_X] = aux->units[mapped_6sensor].data_pos[VR_X] * scale_trans;
		new_value.t[VR_Y] = aux->units[mapped_6sensor].data_pos[VR_Y] * scale_trans;
		new_value.t[VR_Z] = aux->units[mapped_6sensor].data_pos[VR_Z] * scale_trans;
		new_value.r[VR_AZIM] = aux->units[mapped_6sensor].data_pos[VR_AZIM+3];
		new_value.r[VR_ELEV] = aux->units[mapped_6sensor].data_pos[VR_ELEV+3];
		new_value.r[VR_ROLL] = aux->units[mapped_6sensor].data_pos[VR_ROLL+3];

		vrAssign6sensorValue(current_6sensor, vrMatrixSetEulerAzimaxis(&tmpmat, &new_value, VR_Z), 0 /*, TODO: timestamp */);
	}
}


	/**********************************************************************/
	/*    Function(s) for parsing Flock of Birds "input" declarations.    */
	/*                                                                    */
	/*  These _Fob<type>Input() functions are called during the CREATE    */
	/*  phase of the input interface.                                     */

/**************************************************************************/
/* Here is a sample definition for a flock button:                        */
/*     input "2switch[bmouse-L]" = "2switch(3, L)";                       */
/* And since the numbers are 1-based, this means the left button of the   */
/* bird-mouse on receiver number 3.                                       */
static vrInputMatch _FobButtonInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
static	char		*whitespace = " \t\r\b\n";
	_FobPrivateInfo	*aux = (_FobPrivateInfo *)devinfo->aux_data;
	char		*button_name_instance;		/* pointer to the 2nd half of the instance string -- after the comma */
	int		button_num;			/* the current count of FreeVR button inputs */
	int		unit_number;			/* the number of the receiver (eg. the 2nd receiver is a bird-mouse) */
	int		button_key;			/* the mapping from an alpha code to a numeric value for the button  */

	vrDbgPrintfN(ASCFOB_DBGLVL, "_FobButtonInput(): instance = '%s'\n" NORM_TEXT, dti->instance);

	button_num = aux->num_buttons;
	aux->num_buttons++;
	unit_number = vrAtoI(dti->instance);
	button_name_instance = strchr(dti->instance, ',') + 1;			/* jump past the comma */

	if (button_name_instance != NULL) {
		button_name_instance += strspn(button_name_instance, whitespace);/* skip white */
		button_key = _FobButtonValue(button_name_instance);
	} else {
		button_key = -1;
	}

	/*************************************************/
	/* verify that the requested 'instance' is valid */
	if (unit_number < 1 || unit_number > MAX_FOB_UNITS)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_FobButtonInput: Warning, sensor number must be between %d and %d\n", 1, MAX_FOB_UNITS); /* TODO: determine better (more specific) last argument */
	else {
		aux->units[unit_number].has_button = 1;
		if (aux->button_inputs[button_num] != NULL)
			vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_FobButtonInput: Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);
	}

	if (button_key == -1)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_FobButtonInput: Warning, button['%s'] did not match any known button\n", dti->instance);
	else if (aux->button_inputs[button_num] != NULL)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_FobButtonInput: Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);

	vrDbgPrintfN(INPUT_DBGLVL, "_FobButtonInput: assigned button #%d -- type %d of sensor %d\n",
		button_num, button_key, unit_number);

	/***********************/
	/* now setup the input */
	aux->button_inputs[button_num] = (vr2switch *)input;
	aux->button_map_unit[button_num] = unit_number - 1;	/* change to 1-based unit code */
	aux->button_map_button[button_num] = button_key;
	aux->button_values[button_num] = 0;	/* TODO: should vrAssign2switch() be used here?  probably */

	vrDbgPrintfN(INPUT_DBGLVL, "assigned button event of value 0x%02x to input pointer = %#p)\n",
		button_num, aux->button_inputs[button_num]);

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/****************************************************************************/
/* This is a special "button" input that allows one to have a means of      */
/*   giving a discrete 2-switch input when using a flock with no bird-mouse */
/*   present.  Which is particularly useful when one wants to initiate a    */
/*   control to the device.                                                 */
/*	control "print_struct" = "2switch(2, Z)";                           */
static vrInputMatch _FobButtonZerorotInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
static	char		new_instance[128];
static	vrInputDTI	new_dti;

	if (strlen(dti->instance) < 125) {
		strcpy(new_instance, dti->instance);
		strcat(new_instance, ",Z");

		new_dti.device = dti->device;
		new_dti.type = dti->type;
		new_dti.instance = vrShmemStrDup(new_instance);

		return _FobButtonInput(devinfo, input, &new_dti);
	} else {
		/* TODO: print a warning here that we're not making this input & why */
		return VRINPUT_MATCH_UNABLE;
	}
}


/**************************************************************************/
/* NOTE: there are no valuator inputs for flock of bird systems (at least */
/*   not at the time this interface was programmed).  So this is just     */
/*   placeholder code.                                                    */
static vrInputMatch _FobValuatorInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
#ifdef NOT_YET_IMPLEMENTED
	_FobPrivateInfo	*aux = (_FobPrivateInfo *)devinfo->aux_data;
	int		valuator_num;
	int		unit_number;
#endif /* NOT_YET_IMPLEMENTED */

	vrDbgPrintfN(ASCFOB_DBGLVL, "NYI: _FobValuatorInput(): instance = '%s'\n" NORM_TEXT, dti->instance);

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/**************************************************************************/
/* Here is a sample definition for a flock button:                        */
/*     input "6sensor[head]" = "6sensor(1, r2e)";                         */
/* And since the numbers are 1-based, this means the first receiver is    */
/* the input referred to as "6sensor[head]", and it will have the current */
/* receiver-to-entity matrix applied to it.                               */
static vrInputMatch _FobReceiverInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
	_FobPrivateInfo		*aux = (_FobPrivateInfo *)devinfo->aux_data;
	int			sensor_num;
	int			unit_number;

	vrDbgPrintfN(ASCFOB_DBGLVL, "_FobReceiverInput(): instance = '%s'\n" NORM_TEXT, dti->instance);

	sensor_num = aux->num_6sensors;
	aux->num_6sensors++;
	unit_number = vrAtoI(dti->instance);

	/*************************************************/
	/* verify that the requested 'instance' is valid */
	if (unit_number < 1)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_FobReceiverInput: Warning, sensor number must be between %d and %d\n", 1, MAX_FOB_UNITS); /* TODO: determine better (more specific) last argument */
	else if (aux->sensor6_inputs[sensor_num] != NULL)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_FobReceiverInput: Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);

	/***********************/
	/* now setup the input */
	aux->sensor6_inputs[sensor_num] = (vr6sensor *)input;
#if 0 /* TODO: determine whether this code to clear the values is necessary */
	aux->sensor6_values[sensor_num].t[VR_X] = 0.0;
	aux->sensor6_values[sensor_num].t[VR_Y] = 0.0;
	aux->sensor6_values[sensor_num].t[VR_Z] = 0.0;
	aux->sensor6_values[sensor_num].r[VR_AZIM] = 0.0;
	aux->sensor6_values[sensor_num].r[VR_ELEV] = 0.0;
	aux->sensor6_values[sensor_num].r[VR_ROLL] = 0.0;
	aux->sensor6_dummy[sensor_num] = 0;
#endif
	aux->sensor6_map[sensor_num] = unit_number - 1; /* change from one based (unit_number) to zero based (sensor_num) */

	/* the vrAssign6sensorR2Exform() function does the parsing of the next token */
	vrAssign6sensorR2Exform(aux->sensor6_inputs[sensor_num], strchr(dti->instance, ','));

	vrDbgPrintfN(INPUT_DBGLVL, "assigned 6sensor event of value 0x%02x to input pointer = %#p)\n",
		unit_number, aux->sensor6_inputs[sensor_num]);

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


	/************************************************************/
	/***************** FreeVR Callback routines *****************/
	/************************************************************/

	/******************************************************************/
	/*    Callbacks for controlling the Flock of Birds features.      */
	/*                                                                */

/************************************************************/
static void _FobSystemPauseToggleCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

	devinfo->context->paused ^= 1;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Fob Control: system_pause = %d.\n",
		devinfo->context->paused);
}

/************************************************************/
static void _FobPrintContextStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintContext(stdout, devinfo->context, verbose);
}

/************************************************************/
static void _FobPrintConfigStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintConfig(stdout, devinfo->context->config, verbose);
}

/************************************************************/
static void _FobPrintInputStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintInput(stdout, devinfo->context->input, verbose);
}


/************************************************************/
static void _FobPrintStructCallback(vrInputDevice *devinfo, int value)
{
	_FobPrivateInfo	*aux = (_FobPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	_FobPrintStruct(stdout, aux, verbose);
}


/************************************************************/
static void _FobPrintHelpCallback(vrInputDevice *devinfo, int value)
{
	_FobPrivateInfo	*aux = (_FobPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	_FobPrintHelp(stdout, aux);
}



	/******************************************************************/
	/*    Callbacks for interfacing with the Flock of Birds device.   */
	/*                                                                */


/************************************************************/
static void _FobCreateFunction(vrInputDevice *devinfo)
{
	/*** List of possible inputs ***/
static	vrInputFunction	_FobInputs[] = {
				{ "", VRINPUT_2WAY, _FobButtonInput },
				{ "button", VRINPUT_2WAY, _FobButtonInput },
				{ "birdmouse", VRINPUT_2WAY, _FobButtonInput },
				{ "zerorot", VRINPUT_2WAY, _FobButtonZerorotInput },
				{ "slider", VRINPUT_VALUATOR, _FobValuatorInput },
				{ "", VRINPUT_6SENSOR, _FobReceiverInput },
				{ "receiver", VRINPUT_6SENSOR, _FobReceiverInput },
				{ "bird", VRINPUT_6SENSOR, _FobReceiverInput },
				{ NULL, VRINPUT_UNKNOWN, NULL } };
	/*** List of control functions ***/
static	vrControlFunc	_FobControlList[] = {
				/* overall system controls */
				{ "system_pause_toggle", _FobSystemPauseToggleCallback },

				/* informational output controls */
				{ "print_context", _FobPrintContextStructCallback },
				{ "print_config", _FobPrintConfigStructCallback },
				{ "print_input", _FobPrintInputStructCallback },
				{ "print_struct", _FobPrintStructCallback },
				{ "print_help", _FobPrintHelpCallback },

		/** TODO: other callback control functions go here **/
				/* simulated 6-sensor selection controls */
				/* simulated 6-sensor manipulation controls */
				/* other controls */

				/* end of the list */
				{ NULL, NULL } };

	_FobPrivateInfo	*aux = NULL;

	/******************************************/
	/* allocate and initialize auxiliary data */
	devinfo->aux_data = (void *)vrShmemAlloc0(sizeof(_FobPrivateInfo));
	aux = (_FobPrivateInfo *)devinfo->aux_data;
	_FobInitializeStruct(aux, devinfo->type);

	/******************/
	/* handle options */
	aux->port = vrShmemStrDup(DEFAULT_PORT);	/* default, if no port given */
	aux->baud_int = DEFAULT_BAUD;			/* default, if no baud given */
	aux->baud_enum = vrSerialBaudIntToEnum(aux->baud_int);
	_FobParseArgs(aux, devinfo->args);

	/***************************************/
	/* create the inputs and self-controls */

	/* Because an FOB can have a large unknown number of inputs, we allocate     */
	/*   memory for handling them here.  First the vrInputCreateDataContainers() */
	/*   function is called to determine how many of each type of input needs to */
	/*   be created.  Normally vrInputCreateDataContainers() is called by        */
	/*   vrInputCreateDataContainers() (see below), but for this circumstance we */
	/*   split the two operations.                                               */
	vrInputCountDataContainers(devinfo);

	aux->num_buttons = 0;
	aux->button_inputs = (vr2switch **)vrShmemAlloc0((devinfo->num_2ways + devinfo->num_scontrols) * sizeof(vr2switch *));
	aux->button_map_unit = (int *)vrShmemAlloc0((devinfo->num_2ways + devinfo->num_scontrols) * sizeof(int));
	aux->button_map_button = (int *)vrShmemAlloc0((devinfo->num_2ways + devinfo->num_scontrols) * sizeof(int));
	aux->button_values = (int *)vrShmemAlloc0((devinfo->num_2ways + devinfo->num_scontrols) * sizeof(int));

	aux->num_6sensors = 0;
	aux->sensor6_inputs = (vr6sensor **)vrShmemAlloc0(devinfo->num_6sensors * sizeof(vr6sensor *));
#if 0
	aux->sensor6_values = (vrEuler *)vrShmemAlloc0(devinfo->num_6sensors * sizeof(vrEuler));
#endif
	aux->sensor6_map = (int *)vrShmemAlloc0(devinfo->num_6sensors * sizeof(int));

	/* Here we return to the conventional way of creating the inputs */
	vrInputCreateDataContainers(devinfo, _FobInputs);
	vrInputCreateSelfControlContainers(devinfo, _FobInputs, _FobControlList);
	/* TODO: anything to do for NON self-controls?  Implement for Magellan first. */

	vrDbgPrintf("Done creating Flock of Birds inputs for '%s'\n", devinfo->name);
	devinfo->created = 1;

	return;
}


/************************************************************/
static void _FobOpenFunction(vrInputDevice *devinfo)
{
	vrTrace("_FobOpenFunction", devinfo->name);

	_FobPrivateInfo	*aux = (_FobPrivateInfo *)devinfo->aux_data;

	/*******************/
	/* open the device */
	aux->fd = vrSerialOpen(aux->port, aux->baud_enum);
	if (aux->fd < 0) {
		aux->open = 0;
		vrErrPrintf("(%s::_FobFunction::%d) error: "
			RED_TEXT "couldn't open serial port %s for %s\n" NORM_TEXT,
			__FILE__, __LINE__, aux->port, devinfo->name);
		sprintf(aux->version, "- unconnected Flock of Birds -");
	} else {
		aux->open = 1;
		if (_FobInitializeDevice(aux) < 0) {
			vrErrPrintf("_FobOpenFunction(): "
				RED_TEXT "Warning, left Flock not or improperly connected to '%s' Flock.\n" NORM_TEXT, devinfo->name);
			/* TODO: ideally we'd do something to try to correct the situation here */
		} else {
			vrDbgPrintf("_FobOpenFunction(): Done opening Flock of Birds input device '%s'.\n", devinfo->name);
			devinfo->operating = 1;
		}
	}

	return;
}


/************************************************************/
static void _FobCloseFunction(vrInputDevice *devinfo)
{
	_FobPrivateInfo	*aux = (_FobPrivateInfo *)devinfo->aux_data;

	if (aux != NULL) {
		_FobCloseDevice(aux);
		vrSerialClose(aux->fd);
		vrShmemFree(aux);	/* aka devinfo->aux_data */
	}

	return;
}


/************************************************************/
static void _FobResetFunction(vrInputDevice *devinfo)
{
	_FobPrivateInfo	*aux = (_FobPrivateInfo *)devinfo->aux_data;

	if (aux != NULL) {
		_FobResetDevice(aux);
	}

	return;
}


/************************************************************/
static void _FobPollFunction(vrInputDevice *devinfo)
{
	if (devinfo->operating) {
		_FobGetData(devinfo);
	} else {
		/* TODO: try to open the device again */
	}

	return;
}



	/******************************/
	/*** FreeVR public routines ***/
	/******************************/


/******************************************************/
void vrFobInitInfo(vrInputDevice *devinfo)
{
	devinfo->version = (char *)vrShmemStrDup("-get from Flock of Birds device-");
	devinfo->Create = vrCallbackCreateNamed("FOBInput:Create-Def", _FobCreateFunction, 1, devinfo);
	devinfo->Open = vrCallbackCreateNamed("FOBInput:Open-Def", _FobOpenFunction, 1, devinfo);
	devinfo->Close = vrCallbackCreateNamed("FOBInput:Close-Def", _FobCloseFunction, 1, devinfo);
	devinfo->Reset = vrCallbackCreateNamed("FOBInput:Reset-Def", _FobResetFunction, 1, devinfo);
	devinfo->PollData = vrCallbackCreateNamed("FOBInput:PollData-Def", _FobPollFunction, 1, devinfo);
	devinfo->PrintAux = vrCallbackCreateNamed("FOBInput:PrintAux-Def", _FobPrintStruct, 0);
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
static	int	done = -1;

#define RENDERLINES	0
#define RENDERCSV	1	/* NOTE: not yet implemented here -- see vr_input.fastrak.c for example */
#define RENDERSCREEN	2


/******************/
/* from vr_math.c */
/*******************************************************************/
/* vrBinaryToString(): takes an integer, a number of bits, and a pointer */
/*   to memory in which to store the result.  The resultant string    */
/*   is placed in the memory, and returned.  The string is made up */
/*   of the least significant N bits, based on the value of len.   */
/*******************************************************************/
char *vrBinaryToString(unsigned int val, int len, char *str)
{
	int	count;

	str[len] = '\0';
	for (count = 0; count < len; count++) {
		str[len-count-1] = (((1 << count) & val) ? '1' : '0');
	}
	return(str);
}


/*******************************************************************/
void _RenderValuesLine(_FobPrivateInfo *aux)
{
	int		unit;
	_FobUnit	*unitdata;		/* pointer to the current unit */

	for (unit = 0; unit < MAX_FOB_UNITS; unit++) {
		unitdata = &(aux->units[unit]);
		/* nothing to print for non existent sensors */
		if ((unitdata->config & FOB_CONFIG_SENSOR) == 0)
			continue;

		if (unitdata->has_button) {
			vrPrintf("  Bird %d(%d): b=%02x %6.1f %6.1f %6.1f %6.1f %6.1f %6.1f",
				(aux->standalone ? 0 : unit+1),
				unitdata->data_sensor,
				unitdata->data_button,
				unitdata->data_pos[0],	/* X */
				unitdata->data_pos[1],	/* Y */
				unitdata->data_pos[2],	/* Z */
				unitdata->data_pos[3],	/* AZIM */
				unitdata->data_pos[4],	/* ELEV */
				unitdata->data_pos[5]);	/* ROLL */
		} else {
			vrPrintf("  Bird %d(%d): %6.1f %6.1f %6.1f %6.1f %6.1f %6.1f",
				(aux->standalone ? 0 : unit+1),
				unitdata->data_sensor,
				unitdata->data_pos[0],	/* X */
				unitdata->data_pos[1],	/* Y */
				unitdata->data_pos[2],	/* Z */
				unitdata->data_pos[3],	/* AZIM */
				unitdata->data_pos[4],	/* ELEV */
				unitdata->data_pos[5]);	/* ROLL */
		}
	}
	vrPrintf("\n");
}


/*******************************************************************/
/* NOTE: this routine relies on using the ANSI terminal character */
/*   codes.  Since this is just a test application, I decided to  */
/*   forgo the requirement of the curses library, and instead     */
/*   limit the terminal on which this version would work.         */
/*   On Linux do "man console_codes" to get the sequence codes    */
/*   for other operations.                                        */
void _RenderValuesScreen(_FobPrivateInfo *aux)
{
	int		unit;
	_FobUnit	*unitdata;		/* pointer to the current unit */
	int		num_lines = 40;		/* number of lines for showing chart */
	float		max_trans = 96.0;	/* 8 feet in one direction */
	float		max_rot = 180.0;	/* in one direction */

	int		center = num_lines / 2.0 + 4;	/* shift for header */
	float		trans_scale = num_lines * -0.5 / max_trans;
	float		rot_scale = num_lines * -0.5 / max_rot;

	/* save current cursor position */
	vrPrintf("7");

	/* move to upper left corner of screen */
	vrPrintf("[0;0H");

	/* clear to end of screen */
	vrPrintf("[J");

	for (unit = 0; unit < MAX_FOB_UNITS; unit++) {
		/* nothing to print for non existent sensors */
		if ((aux->units[unit].config & FOB_CONFIG_SENSOR) == 0)
			continue;

		unitdata = &(aux->units[unit]);

		/* move to top of column and print unit # (underlined) */
		vrPrintf("[2;%dH[4mBird_%d:%d[24m", unit*10 + 5, (aux->standalone ? 0 : unit+1), unitdata->data_sensor);

		/* print button value (if appropriate) */
		if (unitdata->has_button)
			vrPrintf("[3;%dH (%02x) ", unit*10 + 5, unitdata->data_button);

		/* now print info headers */
		vrPrintf("[4;%dHXYZAER", unit*10 + 5);

		/* print a *zero* line */
		vrPrintf("[%d;%dH------", center, unit*10 + 5);

		/* print the X value */
		vrPrintf("[%d;%dHX", (int)(unitdata->data_pos[VR_X]*trans_scale)+center, unit*10 + 5);

		/* print the Y value */
		vrPrintf("[%d;%dHY", (int)(unitdata->data_pos[VR_Y]*trans_scale)+center, unit*10 + 6);

		/* print the Z value */
		vrPrintf("[%d;%dHZ", (int)(unitdata->data_pos[VR_Z]*trans_scale)+center, unit*10 + 7);

		/* print the A value */
		vrPrintf("[%d;%dHA", (int)(unitdata->data_pos[VR_AZIM+3]*rot_scale)+center, unit*10 + 8);

		/* print the E value */
		vrPrintf("[%d;%dHE", (int)(unitdata->data_pos[VR_ELEV+3]*rot_scale)+center, unit*10 + 9);

		/* print the R value */
		vrPrintf("[%d;%dHR", (int)(unitdata->data_pos[VR_ROLL+3]*rot_scale)+center, unit*10 + 10);

	}

	/* return to the original cursor position */
	vrPrintf("8");
	fflush(stdout);
}


/*******************************************************************/
void exit_testapp()
{
	if (done < 0) {
		/* if this is the case, we haven't even begun the reading loop, so just exit */
		vrPrintf("\nInterrupted prior to entering the read-loop.\n");
		exit(1);
	}

	done = 1;
}


/*******************************************************************/
/* A test program to communicate with a Flock of Birds device and print the results. */
main(int argc, char *argv[])
{
	_FobPrivateInfo		*aux;
	int			count;
	int			numevents;
	int			rendermode = RENDERSCREEN;
#if 0 /* not yet implemented */
	unsigned int		transmitter_param = -1;
#endif
	int			justonce = 0;		/* quit after just one output */

	done = -1;
	signal(SIGINT, exit_testapp);


	/******************************/
	/* setup the device structure */
	aux = (_FobPrivateInfo *)malloc(sizeof(_FobPrivateInfo));
	memset(aux, 0, sizeof(_FobPrivateInfo));
	_FobInitializeStruct(aux, "fob");


	/****************************************************/
	/* adjust parameters based on environment variables */
	aux->port = getenv("FOB_TTY");
	if (aux->port == NULL)
		aux->port = DEFAULT_PORT;		/* default, if no file given */
	aux->baud_int = DEFAULT_BAUD;			/* default, if no baud given */
	aux->baud_enum = vrSerialBaudIntToEnum(aux->baud_int);

	if (getenv("FOB_BAUD")) {
		aux->baud_int = atoi(getenv("FOB_BAUD"));
		if (vrSerialBaudIntToEnum(aux->baud_int) > 0)
			aux->baud_enum = vrSerialBaudIntToEnum(aux->baud_int);
		else	vrPrintf("Invalid baud (%d), using default (%d)\n", aux->baud_int, DEFAULT_BAUD);
	}
	if (getenv("FOB_POLL") != NULL) {
		vrPrintf("-->Turning off stream and group modes\n");
		aux->stream_mode = 0;
		aux->group_mode = 0;
	}

	if (getenv("FOB_GROUP_ON") != NULL) {
		vrPrintf("-->Turning on group mode\n");
		aux->group_mode = 1;
	}
	if (getenv("FOB_GROUP_OFF") != NULL) {
		vrPrintf("-->Turning off group mode\n");
		aux->group_mode = 0;
	}

	if (getenv("FOB_MOUSE") != NULL) {
		int	button_unit = atoi(getenv("FOB_MOUSE"));
		vrPrintf("-->Turning on button mode for unit %d\n", button_unit);
		if (button_unit >= 0 && button_unit <= MAX_FOB_UNITS)
			aux->units[button_unit].has_button = 1;
	}

	if (getenv("FOB_FILTERS") != NULL) {
		aux->filter_mode = atoi(getenv("FOB_FILTERS"));
		vrPrintf("-->Setting filter mode to 0x%0x\n", aux->filter_mode);
	}

	if (getenv("FOB_LINERENDER") != NULL)
		rendermode = RENDERLINES;

	if (getenv("FOB_TRANSUNIT") != NULL)
		aux->transmitter_unit = atoi(getenv("FOB_TRANSUNIT"));

	if (getenv("FOB_TRANSNUM") != NULL)
		aux->transmitter_num = atoi(getenv("FOB_TRANSNUM"));

vrPrintf("trans unit %02x, num %02x\n", aux->transmitter_unit, aux->transmitter_num);

	/* terminate after acquiring and rendering just one set of inputs */
	if (getenv("FOB_JUSTONCE") != NULL)
		justonce = 1;


	/*****************************************************/
	/* adjust parameters based on command-line-arguments */
	/* TODO: parse CLAs for non-default serial port, baud, etc. */


	/**************************************************/
	/* open the serial port and initialize the device */
	/* TODO: probably should set TIOCNOTTY for the FoB port */
	aux->fd = vrSerialOpen(aux->port, aux->baud_enum);
	if (aux->fd < 0) {
		aux->open = 0;
		vrFprintf(stderr, RED_TEXT "couldn't open serial port %s -- use FOB_TTY to change port.\n" NORM_TEXT, aux->port);
		sprintf(aux->version, "- unconnected Flock of Birds -");

		exit (1);
	} else {
		aux->open = 1;
		if (_FobInitializeDevice(aux) < 0) {
			vrErrPrintf("main: " RED_TEXT "Warning, unable to establish communications with Flock of Birds.\n" NORM_TEXT);
		}
	}

	vrPrintf("\n");
	_FobPrintStruct(stdout, aux, verbose);

	if (aux->open == 0) {
		vrPrintf(RED_TEXT "Quitting due to lack of communication with the Flock.\n" NORM_TEXT);
		exit(1);
	}


	/**********************/
	/* display the output */

	/* for screen-rendering, scroll everything up so it won't be erased */
	if (rendermode == RENDERSCREEN && !justonce) {
		for (count = 0; count < 60; count++)
			vrPrintf("\n");
	}

	/* we're done already if just one printout */
	if (justonce)
		done = 1;

	/* just loop and print values */
	done = 0; /* indicate that we've entered the loop */
	while(aux->open && !done) {
		_FobReadInput(aux);

		switch(rendermode) {
		default:
		case RENDERLINES:
			_RenderValuesLine(aux);
			break;
		case RENDERSCREEN:
			_RenderValuesScreen(aux);
			break;
		}
	}


	/*****************/
	/* close up shop */
	_FobCloseDevice(aux);
	vrPrintf(BOLD_TEXT "\nFlock of Birds device closed\n" NORM_TEXT);
}

#endif /* } TEST_APP */

#if defined(MAN_PAGE) /* {  :set syntax=nroff  */
.\"* ======================================================================= "
.\"*                                                                         "
.\"*   11            fobtest.1                                               "
.\"* .111            Author(s): Bill Sherman                                 "
.\"*   11            Created: January 30, 2001                               "
.\"*   11            Last Modified: January 30, 2001                         "
.\"* 111111                                                                  "
.\"*                                                                         "
.\"* Man page for the FreeVR test program for the Ascension Flock of         "
.\"*   Birds[tm] input device.                                               "
.\"*                                                                         "
.\"* Copyright 2001, Bill Sherman & Friends, All rights reserved.            "
.\"* With the intent to provide an open-source license to be named later.    "
.\"* ======================================================================= "
.\"********************************* TITLE ********************************* "
.\" the ".TH" title line must be the first non-comment line in the file      "
.\" .TH <title> <section> <date> <source> <manual>                           "
.TH FOBTEST 1 "30 January 2001" "FreeVR 0.3c" "FreeVR Commands"

.\" ********************************* NAME ********************************* "
.\" .SH <section header name>                                                "
.SH NAME

fobtest \- test the connection and setup of an Ascension Flock of Birds\*(Tm
system
.\" ******************************* SYNOPSIS ******************************* "
.\" .SH <section header name>                                                "
.SH SYNOPSIS

fobtest
.\" ****************************** DESCRIPTION ***************************** "
.\" .SH <section header name>                                                "
.SH DESCRIPTION

blah blah blah
.PP
Quit by pressing the interrupt key (usually ^C).
.PP
See the asc_fob(5|7) man page for information on setting up
the Flock of Birds hardware.

.\" ******************************* OPTIONS ******************************** "
.\" .SH <section header name>                                                "
.SH OPTIONS

.TP 0.5i
none (not yet anyway)
.\" ******************************* ARGUMENTS ***************************** "
.\" .SH <section header name>                                                "
.SH ARGUMENTS

.TP 0.5i
none
.\" ************************* ENVIRONMENT VARIABLES *********************** "
.\" .SH <section header name>                                                "
.SH ENVIRONMENT VARIABLES

.TP 0.5i
.B FOB_TTY
Set the serial port to which the Flock is connected.
.br
.TP 0.5i
.B FOB_POLL
When set will go into ungrouped polling mode.  I.e. it will
disable the default stream and group settings.  However, it
is possible to use grouped poll mode by also setting the
FOB_GROUP_ON environment variable.
.br
.TP 0.5i
.B FOB_GROUP_ON
Enable group mode when set.
Group mode sends all the sensor data as a single
packet greatly increasing the update rate.  Group mode is on
by default in Stream mode, and off by default in Poll mode.
.br
.TP 0.5i
.B FOB_GROUP_OFF
Disable group mode when set (overrides FOB_GROUP_ON variable).
.br
.TP 0.5i
.B FOB_MOUSE
When set, indicates that an Ascension mouse is active on the
given reciever. 
.br
.TP 0.5i
.B FOB_LINERENDER
When set changes from the default full-screen rendering mode of
the current position data from the Flock to a line based,
scrolling output.

.\" ******************************* EXAMPLES ******************************* "
.\" .SH <section header name>                                                "
.SH EXAMPLES

% fobtest

% setenv FOB_POLL
.br
% fobtest

% setenv FOB_MOUSE 3
.br
% fobtest
.\" ********************************* BUGS ********************************* "
.\" .SH <section header name>                                                "
.SH BUGS

none reported
.\" ********************************* TODO ********************************* "
.\" .SH <section header name>                                                "
.SH TODO

Report the frequency of reports (ie. input frame rate) in the screen
render mode.
.PP
Add "reported_unit" as a field to FobUnit, and don´t use it to determine
where to put the data (since that might cause a core dump).  However,
we might want to display some warning text when a mismatch occurs.
.PP
Change FOB_BUTTON to FOB_MOUSE, and use the value to set which reciever
has the Ascension Mouse attatched.
.PP
Add an option to fobtest that signifies a limit to how many position inputs
should be read before quitting.  This may be useful for requestion a
data output sample from someone having trouble getting their Flock to run.

.\" ******************************* SEE ALSO ******************************* "
.\" .SH <section header name>                                                "
.SH SEE ALSO

asc_fob(5|7), magtest(1), ...
.\" TODO: finish "see also"                                                  "
.\" ******************************* COPYRIGHT ****************************** "
.\" .SH <section header name>                                                "
.SH COPYRIGHT

Copyright 2001, Bill Sherman & Friends, All rights reserved.
With the intent to provide an open-source license to be named later.
.\" ****************************** OTHER NOTES ***************************** "
.\" .SH <section header name>                                                "
.SH OTHER NOTES

Flock of Birds is a trademark of Ascension ... Inc.[?]
.\" TODO: finish company name                                                "
.\"* ======================================================================= "
#endif /* } MAN_PAGE */

