/* ======================================================================
 *
 *  CCCCC          vr_input.asc_ms.c
 * CC   CC         Author(s): Bill Sherman
 * CC              Created: May 14, 2001
 * CC   CC         Last Modified: June 7, 2003
 *  CCCCC
 *
 * Code file for FreeVR inputs from the Ascension MotionStar[tm]
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
			should be assumed to be located.
			(default to "lower")
		"scale" - convert from native units of FoB (inches) to units of the VR system
	  :-(	"angles" - specify how the angles that will be converted into the
			6-sensor matrix should be retrieved from the flock { "euler" "quad" "3x3" }
	  :-(	"filter" { off| <filter number> } - whether to filter, and filter paramter
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

	NOTE: it appears as though requesting button data from a receiver
		without a button causes a "Watch Dog Timer" error.  Of course,
		this may just be for older units.  The unit on which this
		behavior occurred is a '6DFOB', rev 3.64.


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
	14 May 2001 (Bill Sherman) -- copied vr_input.asc_fob.c and began
		editing.  I took some code hints from the University of
		Stuttgart CAVE code.

	15 May 2001 (Bill Sherman)
		First I got the test application running.
		Next, I got the CAVE dso version running.
		Then, I got the stand-alone Tracker daemon version running.

	16 May 2001 (Bill Sherman)
		Worked on getting a hardcoded version to work on the Fraunhofer
		IAO Six-sided CAVE.  I managed to get it somewhat working, but
		the results aren't great.  There is far too much noise.  So, I
		implemented a crude filter, but the results were still not quite
		satisfactory.  It is possible that the hardcoded transmitter
		frequency was sub-optimal.

...
	21 February 2002 (Bill Sherman)
		Some cleanups, so the code will compile without warnings.
		  (even some of the stuff will need to change when I begin
		  implementing some items on the TODO list).

	11 September 2002 (Bill Sherman)
		Moved the control callback array into the _MsFunction() callback.

	21-23 April 2003 (Bill Sherman)
		Updated to new input matching function code style.  Changed
		  "opaque" field to "aux_data".  Split _MsFunction() into 5
		  functions.
		  Added new vrPrintStyle argument to _FobPrintStruct() for the
		  sake of the new "PrintAux" callback.

	3 June 2003 (Bill Sherman)
		Now include "vr_enums.h" for the TEST_APP code.
		Added the address of the auxiliary data to the printout.
		Added the "system_pause_toggle" control callback.
		Added comments classifying the controls.

	16 October 2009 (Bill Sherman)
		A quick fix to the _FobParseArgs() routine to handle the
		  no-arguments case.

	2 September 2013 (Bill Sherman) -- adopting a consistent usage
		of str<...>cmp() functions (using logical negation).

	14 September 2013 (Bill Sherman)
		I am making use of the newly created SELFCTRL_DBGLVL for all
		the self-control outputs.  I then added some reasonable output
		to the "print_help" callback to print out what all the device's
		inputs are doing.

	15 September 2013 (Bill Sherman)
		Changed the "0x%p" format to the improved "%#p" format.

TODO:
	- massive cleanup
		- much of which is deleting Flock of Birds code
		- also should make the different versions cleaner
			(FreeVR, vs. CAVE vs. CAVE DSO, vs. Daemon Standalone)
		- probably rename functions that still begin "_Fob" with "_Ms"

	- implement the "angles" argument, which allows for quaternions or 3x3
		matrices to be used to pass the orientation from the Flock to
		FreeVR.

	- determine a good (i.e. working) configuration, and perhaps also
		a step-by-step process others can follow.

	- Some other commands that need to be implemented/tested:
		CRT Sync (pg xx)
		Filter mode (pg xx)
		DC Filter (pg xx)
		Bird Measurement Rate (pg xx)
		Position scaling (pg xx)
		FBB Reset (pg xx)

	- implement FreeVR arguments for the above new commands

	- Need CLA's for the TEST_APP that allow setting parameters
		[currently have some environment variables]

	- Probably should redo any blocking socket reads such that they
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

**************************************************************************/


#if !defined(FREEVR) && !defined(TEST_APP) && !defined(CAVE) && !defined(TRACKD_SA)
/* Choose how this code should be compiled (ie. for CAVE, standalone, etc). */
#  define	FREEVR
#  undef	TEST_APP
#  undef	CAVE
#  undef	TRACKD_SA
#endif

#define IAO

#undef	DEBUG_PRINT
#undef	MS_DEBUG_PRINT


#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>		/* most systems need this for struct sockaddr */
#include <netinet/in.h>
#include <arpa/inet.h>

#if 0 /* not for ms */
#include "vr_serial.h"
#endif
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


#if defined(TEST_APP) || defined(CAVE) || defined(TRACKD_SA)
#  define	VR_X	0
#  define	VR_Y	1
#  define	VR_Z	2
#  define	VR_AZIM	0
#  define	VR_ELEV	1
#  define	VR_ROLL	2
#endif

#if defined(TEST_APP)
#  include <stdlib.h>		/* needed for getenv() & atoi() */
#  if 0 /* not for ms */
#    include "vr_serial.c"
#  endif
#  include "vr_enums.h"
#endif

#if defined(TEST_APP) || defined(CAVE) || defined(TRACKD_SA)
#  include "vr_utils.c"
   char *vrBinaryToString(unsigned int val, int len, char *str); /* from vr_math.h */
#endif

#if defined(CAVE) || defined(TRACKD_SA) /* { */
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

#if defined(TRACKD_SA) /* { */
#  include "trackd.h"
#endif /* } TRACK_SA */


/*** local defines ***/

#define	DEFAULT_PORT	"/dev/input/fob"
#define	DEFAULT_BAUD	38400

#define	BUFSIZE 1024


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

/* new for ms */
/* TODO: convert to enum */
#define SYSTEM_RUNNING		128
#define SYSTEM_ERROR		64
#define SYSTEM_FBB_ERROR	32
#define SYSTEM_LOCAL_ERROR	16
#define SYSTEM_LOCAL_POWER	8
#define SYSTEM_MASTER		4
#define SYSTEM_CRTSYNC_TYPE	2
#define SYSTEM_CRTSYNC		1


#define DEVICE_IS_ACCESSIBLE	128
#define DEVICE_IS_RUNNING	64
#define DEVICE_IS_RECEIVER	32
#define DEVICE_IS_ERC		16
#define DEVICE_IS_ERC3		8
#define DEVICE_IS_ERC2		4
#define DEVICE_IS_ERC1		2
#define DEVICE_IS_ERC0		1
#define	DEVICE_IS_SRT		1	/* if not DEVICE_IS_ERC, then standard range transmitter */

#define FLOCK_IS_RECEIVER	4
#define FLOCK_HAS_BUTTONS	8

#define	MS_FRONT_HEMISPHERE	0
#define	MS_REAR_HEMISPHERE	1
#define	MS_UPPER_HEMISPHERE	2
#define	MS_LOWER_HEMISPHERE	3
#define	MS_LEFT_HEMISPHERE	4
#define	MS_RIGHT_HEMISPHERE	5



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
#undef FATAL	/* TODO: figure out where this comes from */
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

/* new for ms */
typedef enum {
		MSG_WAKE_UP = 10,
		MSG_SHUT_DOWN = 11,
		MSG_GET_STATUS = 101,
		MSG_SEND_SETUP = 102,
		MSG_SINGLE_SHOT = 103,
		MSG_RUN_CONTINUOUS = 104,
		MSG_STOP_DATA = 105,
		MSG_SEND_DATA = 106,
		MSG_SYNC_SEQUENCE = 30,
		RSP_WAKE_UP = 20,
		RSP_SHUT_DOWN = 21,
		RSP_GET_STATUS = 201,
		RSP_SEND_SETUP = 202,
		RSP_RUN_CONTINUOUS = 204,
		RSP_STOP_DATA = 205,
		RSP_ILLEGAL = 40,
		RSP_UNKNOWN = 50,
		RSP_SYNC_SEQUENCE = 31,
		DATA_PACKET_MULTI = 210,
		DATA_PACKET_ACK = 211,
		DATA_PACKET_SINGLE = 212,
		MSG_SLEEP = 71
	} _MsCommand;

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
		FOB_EXAMINE_MODEL,
		FOB_EXAMINE_SERIAL_BIRD,
		FOB_EXAMINE_SERIAL_SENSOR,
		FOB_EXAMINE_SERIAL_TRANS,
		FOB_EXAMINE_SYSTEMSTATUS,
		FOB_EXAMINE_GROUP_MODE,
		FOB_EXAMINE_AUTOCONFIG,
		FOB_EXAMINE_ERROR_CODE,
		FOB_EXAMINE_XMTR_MODE,
		FOB_EXAMINE_HEMISPHERE,
		FOB_EXAMINE_ADDRESSING,

		FOB_NEXT_TRANSMITTER,
		FOB_AUTOCONFIG,

		FOB_SET_GROUP_MODE,
		FOB_SET_GROUP_MODE_ON,
		FOB_SET_GROUP_MODE_OFF,

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

#if 0 /* non ms { */
	/* NOTE: (also) that this list is not comprehensive.  It contains all the     */
	/*   FoB commands that are deemed (or were once deemed) necessary for normal  */
	/*   operation of the Flock.                                                  */
static	_FobCommandInfo	_FobMsgList[] = {
		{ "FOB_POINT_MODE",		1,	   20000,	"B" },	/* pg 102 -- 20000 is as low as it will work */
		{ "FOB_STREAM_MODE",		1,	   30000,	"@" },	/* pg 116 */
		{ "FOB_RUN",			1,	  200000,	"F" },	/* pg 114 -- 100000 wan't long enough -- test after power-cycle */
		{ "FOB_SLEEP",			1,	  300000,	"G" },	/* pg 115 */

		{ "FOB_BUTTON_READ",		1,	   10000,	"N" },	/* page 68 */
		{ "FOB_SET_BUTTON_MODE_ON",	2,	FOB_SET_DELAY,	"M\x01" }, /* pg 67 */
		{ "FOB_SET_BUTTON_MODE_OFF",	2,	FOB_SET_DELAY,	"M\x00" },

		{ "FOB_EXAMINE_BIRDSTATUS",	2,	FOB_EXAM_DELAY,	{ 'O', '\x00' } }, /*pg 72*/
		{ "FOB_EXAMINE_REVNUM",		2,	FOB_EXAM_DELAY,	"O\x01" },
		{ "FOB_EXAMINE_CRYSTAL",	2,	FOB_EXAM_DELAY,	"O\x02" },
		{ "FOB_EXAMINE_MODEL",		2,	FOB_EXAM_DELAY,	"O\x0f" }, /* O^O */
		{ "FOB_EXAMINE_SERIAL_BIRD",	2,	FOB_EXAM_DELAY,	"O\x19" },
		{ "FOB_EXAMINE_SERIAL_SENSOR",	2,	FOB_EXAM_DELAY,	"O\x1a" },
		{ "FOB_EXAMINE_SERIAL_TRANS",	2,	FOB_EXAM_DELAY,	"O\x1b" },
		{ "FOB_EXAMINE_SYSTEMSTATUS",	2,	2*400000,	"O\x24" }, /* page 89 */
		{ "FOB_EXAMINE_GROUP_MODE",	2,	FOB_EXAM_DELAY,	"O\x23" }, /* page 88 */
		{ "FOB_EXAMINE_AUTOCONFIG",	2,	FOB_EXAM_DELAY,	"O\x32" },
		{ "FOB_EXAMINE_ERROR_CODE",	2,	FOB_EXAM_DELAY,	"O\x0a" },
		{ "FOB_EXAMINE_XMTR_MODE",	2,	FOB_EXAM_DELAY,	"O\x12" },
		{ "FOB_EXAMINE_HEMISPHERE",	2,	FOB_EXAM_DELAY,	"O\x16" },
		{ "FOB_EXAMINE_ADDRESSING",	2,	FOB_EXAM_DELAY,	"O\x13" }, /* page 85, >= v3.67 */

		{ "FOB_NEXT_TRANSMITTER",	2,	  100000,	{ '0', '\x10' }  }, /* was 12000 */
		{ "FOB_AUTOCONFIG",		3,	  400000,	{ 'P', '\x32', '\x00' }  }, /* minimal 600000 (or 300000?) us delay */
		{ "FOB_SET_GROUP_MODE",		3,	   50000,	"P\x23\x01" },	/* last byte changable */
		{ "FOB_SET_GROUP_MODE_ON",	3,	   20000,	"P\x23\x01" },
		{ "FOB_SET_GROUP_MODE_OFF",	3,	   20000,	"P\x23\x00" },

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
#endif /* } */



/****************************************************************/
/*** auxiliary structures of the current data from the Flock. ***/

typedef struct {
		unsigned short int 	sequence;
		unsigned short int	milliseconds;
		unsigned int		time;
		unsigned char		type;
		unsigned char		xtype;
		unsigned char		protocol;
		unsigned char		error_code;
		unsigned short int	error_code_extension;
		unsigned short int	number_bytes;
		unsigned char		data[2048];	/* TODO: [2/21/02: not sure if this should be unsigned or not, but making it unsigned to eliminate compiler warnings */
	} _MsPacket;



	/* _FobUnit: contains data for a single unit of the FoB system */
typedef struct {
		unsigned char	config;		/* configuration byte of the sensor */
		unsigned char	status[2];	/* from bird status */
		unsigned char	error;		/* from examine error */
		int		data_sensor;	/* number of the sensor from which data is reportedly from (should correspond directly to unit number) */
		float		data_pos[32];	/* incoming position data of the sensor */
		float		old_dp[32];	/* some hack code for doing a simple filter */
		float		old_dp2[32];	/* some hack code for doing a simple filter */
		float		old_dp3[32];	/* some hack code for doing a simple filter */
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

		int		data_type;	/* type of information the flock will send */
		int		has_button;	/* whether a button is attached to this unit */

		/* new for ms */
		/* TODO: reorganize this structure for MotionStar */
		int		is_receiver;	/* whether or not this is a receiver unit -- TODO: probably this should just be in the "config" field */
	} _FobUnit;

typedef struct {
		/* these are for interfacing with the hardware */
		int		fd;		/* was commhandle */
		char		*port;		/* name of serial port */
		int		baud_enum;	/* communication rate as an enumerated value */
		int		baud_int;	/* communication rate as the real value */
		int		open;		/* flag with Flock of Birds successfully open */

		/* new for ms */
		int		sock_id;	/* deprecated, using old "fd" field */
		int		connected;
	struct	sockaddr_in	server;			/* TODO: should this be "sockaddr" type? */
		int		bird_port;
		int		button_system;
		unsigned char	data_button;	/* incoming button data of the system */
		int		dual_erc;
		_MsPacket	packet;		/* packet to/from the MotionStar */
		int		rate;

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
		int		transmitter_unit;/* Bird unit number of the (single) transmitter */
		int		transmitter_num;/* Transmitter number in the Flock 0-3 */
		int		transmitter_count;/* number of transmitters in the Flock */
		int		sensor_count;	/* number of sensors in the Flock */
		unsigned short	transmitter_param;/* parameter for NEXT_TRANSMITTER Flock command */
		unsigned short	autoconfig_param;/* parameter for AUTOCONFIG Flock command */
		int		group_mode;	/* whether group mode should be used to get data */
		int		stream_mode;	/* whether stream mode should be used to get data */
		int		crt_sync;
		int		filter_mode;
		int		hemisphere;	/* code of which hemisphere to use */
		int		group_data_type;/* type of data flock is to report (when using gm)*/
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

#if defined(CAVE) || defined(TRACKD_SA)
		CAVE_SENSOR_ST	head, wand1, wand2;	/* storage for multiple CAVE sensors */
		CAVE_CONTROLLER_ST controls;		/* storage for CAVE controller info */
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

#if 1
	aux->auto_setup = 1;			/* use status info to determine setup */
#else
	aux->transmitter_unit = 2;		/* this is the default EVL CAVE setup */
	aux->transmitter_num = 0;
	aux->transmitter_count = 1;
	aux->sensor_count = 3;
#endif

	/* new for ms */
	aux->connected = 0;
	aux->bird_port = 6000;
	aux->button_system = 0;
	bzero(&(aux->packet), sizeof(aux->packet));
#ifdef IAO
	aux->dual_erc = 1;
	aux->rate = 100;
	aux->button_system = 1;		/* feedthrough button */
#else
	aux->dual_erc = 0;
	aux->rate = 100;
	aux->button_system = 0;		/* non feedthrough button */
#endif


	aux->stream_mode = 1;			/* use stream mode by default */
	if (aux->stream_mode)
		aux->group_mode = 1;		/* use group mode when using stream mode */
	aux->crt_sync = 0;			/* turn off sync by default */
	aux->filter_mode = 0;			/* turn off filters by default */
	aux->hemisphere = MS_FRONT_HEMISPHERE;	/* use lower hemisphere by default (front at US) */
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
static void _MsPrintPacket(FILE *file, _MsPacket *packet)
{
	vrFprintf(file, "MS: MotionStar packet info\n");
	vrFprintf(file, "\r    sequence = %hd\n", packet->sequence);
	vrFprintf(file, "\r    milliseconds = %hd\n", packet->milliseconds);
	vrFprintf(file, "\r    time = %d\n", packet->time);
	vrFprintf(file, "\r    type = %d\n", (int)(packet->type));
	vrFprintf(file, "\r    xtype = %d\n", (int)(packet->xtype));
	vrFprintf(file, "\r    protocol = %d\n", (int)(packet->protocol));
	vrFprintf(file, "\r    error_code = %d\n", (int)(packet->error_code));
	vrFprintf(file, "\r    error_code_extension = %hd\n", packet->error_code_extension);
	vrFprintf(file, "\r    number_bytes = %hd\n", packet->number_bytes);
	vrFprintf(file, "\r    data = %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x ...\n", packet->data[0], packet->data[1], packet->data[2], packet->data[3], packet->data[4], packet->data[5], packet->data[6], packet->data[7], packet->data[8], packet->data[9], packet->data[10], packet->data[11]);

}


/******************************************************/
static void _FobPrintStruct(FILE *file, _FobPrivateInfo *aux, vrPrintStyle style)
{
	int	count;

	vrFprintf(file, "MS: MotionStar device internal structure (%#p):\n", aux);
	vrFprintf(file, "\r    version -- '%s'\n", aux->version);
	vrFprintf(file, "\r    operating parameters -- '%s'\n", aux->op_params);
	vrFprintf(file, "\r    fd = %d\n    port = '%s'\n    baud = %d (%d)\n    open = %d\n",
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
		vrFprintf(file, "\r\t\tbutton %d: value = %d, map_unit = %d+1, map_button = %d, input = %#p (type = %d)\n",
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
		vrFprintf(file, "\r\t\t6-sensor %d: value = [%.2f %.2f %.2f  %.2f %.2f %.2f], map_unit = %d+1, input = %#p (type = %d)\n",
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

	if (status & FOB_STATUS_MASTER)	vrFprintf(file, "master ");
	else				vrFprintf(file, "slave ");

	if (status & FOB_STATUS_INIT)	vrFprintf(file, "init+ ");
	else				vrFprintf(file, RED_TEXT "init- " NORM_TEXT);

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

		vrFprintf(file, "Bird unit %d (sn: %d, rev: %4.2f, model: '%s', crystal: %dmhz)\n",
			unit+1, aux->units[unit].sn, aux->units[unit].rev_number,
			aux->units[unit].model, aux->units[unit].crystal_rate);

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
	vrFprintf(file, BOLD_TEXT "MS: Sorry, MotionStar - print_help control not yet implemented.\n" NORM_TEXT);
#else
	vrFprintf(file, BOLD_TEXT "MotionStar - inputs:" NORM_TEXT "\n");
	for (count = 0; count < aux->num_buttons; count++) {
		if (aux->button_inputs[count] != NULL) {
			vrFprintf(file, "\t%s -- %s%s\n",
				aux->button_inputs[count]->my_object->desc_str,
				(aux->button_inputs[count]->input_type == VRINPUT_CONTROL ? "control:" : ""),
				aux->button_inputs[count]->my_object->name);
		}
	}

	/* NOTE: MotionStar has no valuator inputs */

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


#if 0 /* non ms { */
/******************************************************/
static int _FobClearPalate(_FobPrivateInfo *aux)
{
static	unsigned char	buffer[BUFSIZE];
	int		bytes_read;
	int		count;

#if 0
	bytes_read = vrSerialRead(aux->fd, buffer, BUFSIZE);
#endif
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
#endif /* } */


/******************************************************/
static char _FobSendCommandAddr(_FobPrivateInfo *aux, _MsCommand command, int addr)
{
#if 0 /* non ms { */
	_FobCommandInfo	*command_info;
#endif /* } */
	char		return_code = '\0';
static	int		packet_seq = 0;

#ifdef MS_DEBUG_PRINT /* TODO: this could be a debug level print -- doesn't print too often */
	vrFprintf(stderr, "_FobSendCommand(), sending command %d\n", (int)command);
#endif

#if 0
	/* if command doesn't exist, then return immediately */
	if (command >= FOB_LASTCOMMAND) {
		vrPrintf(RED_TEXT "No command (%d), nothing sent to FoB\n" NORM_TEXT, (int)command);
		return (return_code);
	}
#endif

#if 0
	command_info = &(_FobMsgList[command]);
#else
	packet_seq++;
	aux->packet.sequence = packet_seq;
	aux->packet.type = command;
	aux->packet.xtype = (unsigned char)addr;
	aux->packet.time = 0;
	aux->packet.milliseconds = 0;
	aux->packet.protocol = 3;
	if (command != MSG_SEND_SETUP)
		aux->packet.number_bytes = 0;
#endif

#ifdef DEBUG_PRINT	/* TODO: this should be a debug level -- what about for test app? */
	vrDbgPrintN(ASCFOB_DBGLVL + RARE_DBGLVL, "FOB: Sending command '%s'\n", command_info->name);
#endif

#if 0
	vrSerialWrite(aux->fd, command_info->msg, command_info->len);
#  if 1
	vrSerialDrain(aux->fd);
#  endif
	vrSleep(command_info->us_delay);
#else
	/* new for ms */
#  ifdef MS_DEBUG_PRINT
	printf(BOLD_TEXT "<<< sending packet:\n" NORM_TEXT);
	_MsPrintPacket(stderr, &(aux->packet));
#  endif
#  if defined(__linux) || defined(__APPLE__) || defined(__CYGWIN__)
	return (sendto(aux->fd, &(aux->packet), aux->packet.number_bytes+16, 0, (struct sockaddr *)&aux->server, sizeof(aux->server))); /* TODO: why is this "sockaddr", when the type is "sockaddr_in" ?? */
#  else
	return (sendto(aux->fd, &(aux->packet), aux->packet.number_bytes+16, 0, &aux->server, sizeof(aux->server)));
#    if 0
	flush(aux->fd);
#    endif
#  endif
#endif

	/* TODO: should we check for error status after every command? */
	/*   Well, we certainly shouldn't do it after a stream command. */
	/*   Probably best to require specific code to do error checking. */

	return (return_code);
}


/******************************************************/
static char _FobSendCommand(_FobPrivateInfo *aux, _MsCommand command)
{
	return (_FobSendCommandAddr(aux, command, 0));
}


/******************************************************/
/* TODO: if error is #13, then perhaps do an expanded-error-code check (1999, pg. 82) */
static int _MsGetPacket(_FobPrivateInfo *aux)
{
	void	*buf = &(aux->packet);
	int	toReceive = 16;
	int	headerBytesReceived = 0;
	int	bytesReceived;

	/* loop until all of the packet data was read */
	/*   start by reading the 16 header bytes, then using */
	/*   the header information, keep reading until all   */
	/*   the data is read.                                */
	while (headerBytesReceived != toReceive) {
#ifdef MS_DEBUG_PRINT
		vrFprintf(stderr, "Hey: about to call recvfrom(%d, %#p, %d, %d)\n", aux->fd, buf, toReceive-headerBytesReceived, 0);
#endif
		bytesReceived = recvfrom(aux->fd, buf, toReceive-headerBytesReceived, 0, NULL, NULL);
#ifdef MS_DEBUG_PRINT
		vrFprintf(stderr, "yo: just read %d bytes\n", bytesReceived);
#endif

		if (bytesReceived < 0)
			return -1;

		headerBytesReceived += bytesReceived;

		if (headerBytesReceived==16)
		{
#ifdef MS_DEBUG_PRINT
			printf(BOLD_TEXT "reading packet <<<:\n" NORM_TEXT);
			_MsPrintPacket(stderr, &(aux->packet));
#endif
			/* got the header, now read the rest */
			toReceive = aux->packet.number_bytes + 16;
#ifdef MS_DEBUG_PRINT
			vrFprintf(stderr, "now get %d more bytes into data\n", toReceive-headerBytesReceived);
#endif
		}

		/* add received data to packet */
		buf = (void *)((char *)buf + bytesReceived );
	}

	return (headerBytesReceived);
}


#if 0 /* non ms { */
/******************************************************/
/* TODO: if error is #13, then perhaps do an expanded-error-code check (1999, pg. 82) */
static int _FobGetError(_FobPrivateInfo *aux, int unit)
{
static	unsigned char	buffer[10];
	int		bytes_read;
	int		error_code;

	/* TODO: also send to other units? */
	if (unit == 0) {
		/* send to Master */
		_FobSendCommand(aux, FOB_EXAMINE_ERROR_CODE);
	} else {
		/* send to specified unit */
		_FobSendCommandAddr(aux, FOB_EXAMINE_ERROR_CODE, unit);
	}
#if 0
	bytes_read = vrSerialRead(aux->fd, buffer, 1);
#endif

	if (bytes_read == 1) {
		error_code = buffer[0];
		if (error_code == 0) {
#ifdef DEBUG_PRINT	/* TODO: this should be a debug level -- what about for test app? */
			vrDbgPrintN(ASCFOB_DBGLVL + RARE_DBGLVL, RED_TEXT "FOB: Error code (unit %d):%3d (?)\n" NORM_TEXT, unit, error_code);	/* TODO: for some reason, a second '?' screws up the output. */
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
#endif /* } */


#if 0 /* non ms { */
/******************************************************/
static int _FobGetUnitStatus(_FobPrivateInfo* aux, int unit)
{
static	unsigned char	buffer[128];
	int		bytes_read;

	/* Bird Status -- see pages 72-73 of the 1999 FoB Guide */
	_FobSendCommandAddr(aux, FOB_EXAMINE_BIRDSTATUS, unit+1);
#if 0
	bytes_read = vrSerialReadNbytes_Block(aux->fd, buffer, 2);
#endif

	if (bytes_read < 2) {
		aux->units[unit].status[0] = 0x00;
		aux->units[unit].status[1] = 0x00;
	} else {
		aux->units[unit].status[0] = buffer[0];
		aux->units[unit].status[1] = buffer[1];
	}

	_FobSendCommandAddr(aux, FOB_EXAMINE_ERROR_CODE, unit+1);
#if 0
	bytes_read = vrSerialReadNbytes_Block(aux->fd, buffer, 1);
#endif
	aux->units[unit].error = buffer[0];
}
#endif /* } */


/******************************************************/
/* TODO: WARNING, this currently puts the flock in point (vs. stream) mode, */
/*    and doesn't bother to put things back if previously in stream mode.   */
static int _FobGetFlockStatus(_FobPrivateInfo* aux)
{
static	char		print_buf[128];
#if 0 /* non ms { */
static	unsigned char	buffer[BUFSIZE];
	int		bytes_read;
	int		count;
#endif /* } */
	int		unit;
	/* new for ms */
	unsigned int	status;
	unsigned int	error;
	int		numReceivers;

	/**************************/
	/* Get system information */

#if 0 /* non ms stuff { */
#if 0 /* TODO: this code should only be executed on systems with late-model firmware */
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
#if 0
	bytes_read = vrSerialReadNbytes_Block(aux->fd, buffer, 1);
#endif
#  endif
	aux->address_mode = buffer[0];
	switch (aux->address_mode) {
	case 0x00:
		aux->max_units = 14;
		break;
	case 0x01:
		aux->max_units = 30;
		break;
	case 0x03:
		aux->max_units = 126;
		break;
	default:
		vrPrintf(RED_TEXT "FOB: Warning: invalid response to address mode request, assuming normal.\n" NORM_TEXT);

		aux->max_units = 14;
		break;
	}
#else
	vrPrintf(RED_TEXT "FOB: Warning: Currently not requesting address mode, assuming normal.\n" NORM_TEXT);
	aux->address_mode = 0x00;
	aux->max_units = 14;
#endif
#endif /* } non ms */


	/**************************/
	/* Get unit system status * (see page 89 of the 1999 FoB Guide) */
	/* This reads the status for all possible units.  The number is */
	/*   based on the addressing mode that has been selected.       */

#if 0 /* non ms { */
	/* Get Status from all units in System */
	_FobClearPalate(aux);
	_FobSendCommand(aux, FOB_EXAMINE_SYSTEMSTATUS);
	bytes_read = vrSerialReadNbytes_Block(aux->fd, buffer, aux->max_units);
	vrPrintf("FOB System status:\n");

	/* NOTE: more than 14 for expanded address modes */
	aux->num_units = 0;
	for (unit = 0; unit < aux->max_units; unit++) {
		aux->units[unit].config = buffer[unit];
		if (buffer[unit]) {
			aux->num_units++;
			vrFprintf(stderr, "    unit %2d: ", unit+1);
			_FobPrintUnitConfig(stderr, buffer[unit]);
		}
	}
	if (aux->num_units == 0 || aux->num_units == aux->max_units) {
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
#else /* } non ms vs. ms { */
   {
	/* TODO: this part (get status) may not work in IAO cave (w/ wireless MotionStar) */
	vrFprintf(stderr, "MS: Getting configuration data from the birds...\n");
	_FobSendCommand(aux, MSG_GET_STATUS);
	_MsGetPacket(aux);	/* TODO: check that no error occurred */

	status = aux->packet.data[0];
	error = aux->packet.data[1];
	/* TODO: other bytes: */
	/*	flock_number = data[2] */
	/*	server_number = data[3] */
	/*	transmitter_number = data[4] */
	/*	measurement_rate = data[5]...data[10] (ASCII number, samples per 1000sec) */
	/*	chassis_number = data[11] (wireless always = 1) */
	/*	chassis_devices = data[12] */
	/*	first_address = data[13] */
	/*	software_revision = data[14] & data[15] */

	/* check status for any error */
	vrFprintf(stderr, "System Status %02x (84 = running & master): %s\n", status, vrBinaryToString((unsigned int)(status), 8, print_buf));
	if (status&(SYSTEM_ERROR|SYSTEM_FBB_ERROR|SYSTEM_LOCAL_ERROR|SYSTEM_LOCAL_POWER)) {
		fprintf(stderr, "birdTracker: ERROR - system error (status %d, error %d)\a\n", status, error);
		return (-1);
	}

	/* see if the system is up */
	if (!(status&SYSTEM_RUNNING)) {
		vrFprintf(stderr, "birdTracker: ERROR - system not running (status %d, error %d)\a\n", status, error);
		return (-1);
	}

	/* now get number of connected devices */
	numReceivers = 0;
	for (unit = 0; unit < 8 /* TODO: theoretically, this could be 120 (more?) */; unit++) {
		vrFprintf(stderr, "Status for bird %2d: %s", unit+1, vrBinaryToString((unsigned int)(aux->packet.data[16+unit]), 8, print_buf));
		vrFprintf(stderr, "  %s  %s  %s  %s  %s  %s  %s  %s\n",
			((aux->packet.data[16+unit] & DEVICE_IS_ACCESSIBLE) ? "acc" : "   "),
			((aux->packet.data[16+unit] & DEVICE_IS_RECEIVER) ? "rec" : "   "),
			((aux->packet.data[16+unit] & DEVICE_IS_RUNNING) ? "run" : "   "),
			((aux->packet.data[16+unit] & DEVICE_IS_ERC) ? "erc" : "   "),
			((aux->packet.data[16+unit] & DEVICE_IS_ERC3) ? "#3" : "  "),
			((aux->packet.data[16+unit] & DEVICE_IS_ERC2) ? "#2" : "  "),
			((aux->packet.data[16+unit] & DEVICE_IS_ERC1) ? "#1" : "  "),
			((aux->packet.data[16+unit] & DEVICE_IS_ERC0) ? "#0" : "  "));
		if ((aux->packet.data[16+unit]&(DEVICE_IS_ACCESSIBLE|DEVICE_IS_RECEIVER|DEVICE_IS_RUNNING)) == (DEVICE_IS_ACCESSIBLE|DEVICE_IS_RECEIVER|DEVICE_IS_RUNNING)) {
			numReceivers++;
			aux->units[unit].is_receiver = 1;
			vrFprintf(stderr, BOLD_TEXT "Setting bird %d as a receiver.\n" NORM_TEXT, unit);
		} else {
			aux->units[unit].is_receiver = 0;
		}
	}

	aux->num_units = numReceivers;
	vrFprintf(stderr, "Found %d bird units\n", aux->num_units);

   }

#endif /* } */

	/* If we still only have 0 units, something is definitely wrong! */
	if (aux->num_units == 0) {
		vrFprintf(stderr, RED_TEXT "Perhaps the wireless backpack is turned off!\n" NORM_TEXT);
		/* TODO: if this is the case, then we should really keep trying until the backpack is turned on. */
		return (-1);
	}

#if 0 /* non ms { */
	/**********************************/
	/* Get each existing unit's setup */
	/* TODO: this needs to be changed to support expanded addressing */
	vrFprintf(stderr, "FOB: Getting configuration data from the %d birds... (this might take a moment)...\n", aux->num_units);
	for (unit = 0; unit < aux->max_units; unit++) {
		vrFprintf(stderr, "%d", (unit+1)%10);

		if (!aux->units[unit].config)
			/* no bird at that address */
			continue;

		/* Revision -- see page 73 of the 1999 FoB Guide */
		_FobSendCommandAddr(aux, FOB_EXAMINE_REVNUM, unit+1);
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
		_FobSendCommandAddr(aux, FOB_EXAMINE_MODEL, unit+1);
		bytes_read = vrSerialReadNbytes_Block(aux->fd, aux->units[unit].model, 10);
		for (count = bytes_read-1; count >= 0; count--) {
			if (aux->units[unit].model[count] != ' ' &&
			    aux->units[unit].model[count] != '\0')
				break;
		}
		aux->units[unit].model[count+1] = '\0';
		vrFprintf(stderr, ".");

		/* Crystal Speed -- see page 73 of the 1999 FoB Guide */
		_FobSendCommandAddr(aux, FOB_EXAMINE_CRYSTAL, unit+1);
		bytes_read = vrSerialReadNbytes_Block(aux->fd, buffer, 2);
		if (bytes_read < 2)
			aux->units[unit].crystal_rate = -1;
		else	aux->units[unit].crystal_rate = buffer[0] + buffer[1] * 256;
		vrFprintf(stderr, ".");

		/* Serial Number -- see page 87 of the 1999 FoB Guide */
		if (aux->units[unit].rev_minor >= 67) {
			_FobSendCommandAddr(aux, FOB_EXAMINE_SERIAL_BIRD, unit+1);
			bytes_read = vrSerialRead(aux->fd, buffer, 2);
			if (bytes_read < 2)
				aux->units[unit].sn = -1;
			else	aux->units[unit].sn = buffer[0] + buffer[1] * 256;
		}
		vrFprintf(stderr, ".");

		/* Transmitter & Sensor Serial Number -- see page 87 of the 1999 FoB Guide */
		if (aux->units[unit].rev_minor >= 71) {
			/* Sensor Serial Number -- see page 87 of the 1999 FoB Guide */
			_FobSendCommandAddr(aux, FOB_EXAMINE_SERIAL_SENSOR, unit+1);
			bytes_read = vrSerialRead(aux->fd, buffer, 2);
			if (bytes_read < 2)
				aux->units[unit].sn_sensor = -1;
			else	aux->units[unit].sn_sensor = buffer[0] + buffer[1] * 256;

			/* Transmitter Serial Number -- see page 87 of the 1999 FoB Guide */
			_FobSendCommandAddr(aux, FOB_EXAMINE_SERIAL_TRANS, unit+1);
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

		_FobSendCommandAddr(aux, FOB_EXAMINE_HEMISPHERE, 2);
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
	}
	vrFprintf(stderr, "\n\n");
#endif /* } */

	return (0);
}


/******************************************************/
/* This function determines the setup for the flock based on the previously */
/*   obtained status bytes.                                                 */
/* The "transmitter_param" field will contain the value of the parameter */
/*   needed for the NEXT_TRANSMITTER command to the Flock of Birds.      */
static void _FobAutoSetup(_FobPrivateInfo* aux)
{
	int		unit;
	unsigned char	config;

	aux->transmitter_num = -1;
	aux->transmitter_unit = -1;
	aux->transmitter_count = 0;
	aux->sensor_count = 0;

	for (unit = 0; unit < aux->max_units; unit++) {
		config = aux->units[unit].config;

		if (config & FOB_CONFIG_SENSOR)
			aux->sensor_count++;

		if (config & FOB_CONFIG_ERT) {
			aux->transmitter_count++;

			if (aux->transmitter_count == 1) {
				aux->transmitter_unit = unit+1;
			} else {
				vrFprintf(stderr, RED_TEXT "FOB: Warning, a second transmitter found, and at the moment, we can only handle one!\n" NORM_TEXT);
			}
		}

		if (config & FOB_CONFIG_XMTR0)
			aux->transmitter_num = 0;

		if (config & FOB_CONFIG_XMTR1)
			aux->transmitter_num = 1;

		if (config & FOB_CONFIG_XMTR2)
			aux->transmitter_num = 2;

		if (config & FOB_CONFIG_XMTR3)
			aux->transmitter_num = 3;
	}
}


/******************************************************/
static void _MsConvertBytesToShorts(int numBytes, unsigned char* src, short* dst)
{
	int		count;

	for (count = 0; count < numBytes/2; count++)
		dst[count] = (((unsigned short)src[2*count]<<8) + (unsigned short)src[2*count+1]);

	return;
}


/******************************************************/
static void _FobConvertBytesToFloats(int numBytes, unsigned char* src, short* dst)
{
	int		count;

#ifdef DEBUG_PRINT
static	unsigned char	print_buf[17];

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


#if 0 /* non ms { */
/******************************************************/
static int _FobReadOneUnit(_FobPrivateInfo *aux, int unit, int data_type)
{
static	unsigned char	buffer[BUFSIZE];
static	unsigned char	print_buf[8];
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
#if 0
	do {
		bytes_read = vrSerialReadNbytes_Block(aux->fd, buffer, 1);
	} while ((bytes_read > 0) && !(buffer[0] & 0x80));
#endif


	/******************************************************************/
	/* read up to the expected number of bytes, and convert to floats */
#if 0
	bytes_read += vrSerialReadNbytes_Block(aux->fd, &(buffer[bytes_read]), data_bytes-bytes_read);
#endif

	/* units with activated button mode send an extra byte for the button values */
	/* NOTE: Again dangerous if it's possible for sensor order to be */
	/*   unpredictable -- fortunately it doesn't seem to be, which   */
	/*   begs the question of why waste bandwidth on the extra byte? */
	if (aux->units[unit].has_button) {
#if 0
		bytes_read += vrSerialReadNbytes_Block(aux->fd, &(buffer[bytes_read]), 1);
		button_value = (int)buffer[bytes_read-1];
#endif
	}

	/* group mode sends an extra byte for the unit number */
	if (aux->group_mode) {
#if 0
		bytes_read += vrSerialReadNbytes_Block(aux->fd, &(buffer[bytes_read]), 1);
#endif
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


	_FobConvertBytesToFloats(data_bytes, buffer, raw_data);


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
			_FobSendCommandAddr(aux, FOB_BUTTON_READ, unit+1);
#if 0
			bytes_read = vrSerialReadNbytes_Block(aux->fd, buffer, 1);
#endif
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
#endif /* } */


/******************************************************/
static void _FobReadInput(_FobPrivateInfo *aux)
{
#if 0 /* non ms { */
	int		unit;
	int		data_type;		/* type of data expected from Flock */
#endif /* } */

	/* new for ms */
	unsigned int	fbb_addr;
	unsigned int	button_flag;
	unsigned int	data_size;
	unsigned int	data_format;
	unsigned char	*data;
	int		bytes_processed;
	int		bytes_incoming;
	short		button_value;

	/* these are from _FobReadOneUnit() */
static	short		raw_data[12];
	_FobUnit	*read_unit;		/* the unit in which to put the incoming data */
	int		count;
#if 0	/* ms test */
static	float		trans_scale = 144.0f / 32768.0f; /* 144 is for ERT */
#else
#ifdef IAO
static	float		trans_scale = 24.0f / 32768.0f; /* 144 is for ERT */
#else
static	float		trans_scale = 12.0f / 32768.0f; /* 144 is for ERT */
#endif
#endif
static	float		angle_scale = 180.0f / 32768.0f;
#ifdef NOT_YET_IMPLEMENTED
static	float		value_scale =   1.0f / 32768.0f;
#endif

#if 0 /* non ms { */
#ifndef DATATYPETEST
	if (aux->group_mode)
		data_type = aux->group_data_type;
#endif
#endif /* } */

#if 0 /* non ms { */

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
			_FobSendCommandAddr(aux, FOB_POINT_MODE, unit+1);
		}

		/* now read and parse the data for one unit */
		_FobReadOneUnit(aux, unit, data_type);
	}
#else /* } non ms vs. ms { */

	/* NOTE: the MotionStar gets all the units in one packet */
	_MsGetPacket(aux);
	bytes_processed = 0;
	bytes_incoming = aux->packet.number_bytes;
	data = aux->packet.data;

	while (bytes_processed < bytes_incoming) {
		fbb_addr = ((unsigned int)(data[0]&0x7F));

		/* is there button-information available ? (highest bit) */
		button_flag = ((unsigned int)(data[0]&0x80))>>7;

		/** NOTE: bizarrely, the size/format nibbles of data[1] are reversed from how **/
		/**   a sensor is configured.  Par for the course I guess for Ascension.      **/
		/* get number of words (2bytes) in this record */
		data_size = ((unsigned int)(data[1]&0x0F));

		/* and get the format of the data */
		data_format = ((unsigned int)(data[1]&0x0F0))>>4;

#ifdef MS_DEBUG_PRINT
		vrFprintf(stderr, RED_TEXT "fbb = %d, format = %d, data_size = %d, bytes_proc = %d\n" NORM_TEXT, fbb_addr, data_format, data_size, bytes_processed);
#endif

		switch (data_format) {

		case 0:
#if defined(MS_DEBUG_PRINT) || 0
			vrFprintf(stderr, RED_TEXT "Hmmm, data_format is 0\n" NORM_TEXT);
#endif
			break;

		case 4: /* FLOCK_POSITIONANGLES */

			read_unit = &(aux->units[fbb_addr-1]);

			_MsConvertBytesToShorts(data_size*2, &(data[2]), raw_data);

			if (button_flag) {
				_MsConvertBytesToShorts(2, &(data[2 + 2*data_size]), &button_value);
			}

#ifdef MS_DEBUG_PRINT
			vrFprintf(stderr, "Raw data = (button flag = %d, value = %d) %d %d %d %d %d %d ...\n",
				button_flag, button_value,
				raw_data[0],
				raw_data[1],
				raw_data[2],
				raw_data[3],
				raw_data[4],
				raw_data[5]);
#endif
			for (count = 0; count < 3; count++)
				read_unit->data_pos[count] = raw_data[count] * trans_scale;
			for (count = 3; count < 6; count++)
				read_unit->data_pos[count] = raw_data[count] * angle_scale;

			read_unit->data_button = button_value;

			break;

		case 14: /* FLOCK_FEEDTHROUGHDATA */
			button_value = (*(unsigned short int *)(data+(data_size)*2) >> 8) & (~0x80);
#if 0
			/* NOTE: this is not really "read_unit" it is just a general button value for the system */
			read_unit->data_button = button_value;
#else
			aux->data_button = button_value;
#endif
			vrFprintf(stderr, RED_TEXT "button_value is %d\n" NORM_TEXT, button_value);
			break;

		default:
			vrFprintf(stderr, RED_TEXT "not yet implemented format %d\n" NORM_TEXT, data_format);
			break;

		}

		bytes_processed += data_size*2 + button_flag*2 + 2;
		data += data_size*2 + button_flag*2 + 2;

	}
#endif /* } */

	return;
}


/******************************************************/
/* _FobInitializeDevice() is called in the OPEN phase     */
/*   of input interface -- after the types of inputs have */
/*   been determined (during the CREATE phase).           */
static int _FobInitializeDevice(_FobPrivateInfo *aux)
{
static	char		print_buf[256];		/* used for making binary strings */
#if 0 /* non ms { */
static	unsigned char	buffer[BUFSIZE];	/* where we read the serial data into */
	_FobUnit	*transunit;		/* used for making the version & param strings */
	int		bytes_read;		/* number of bytes read from serial port */
#endif /* } */
	int		unit;			/* counter for looping through the bird units */
	int		count;
	unsigned char	*dataptr;

	if (aux == NULL)
		return(-1);

#if 0
	/******************************************************************************/
	/* First, verify that (some of) the underlying data structures are consistent */
	if (strcmp(_FobMsgList[FOB_LASTCOMMAND].name, "FOB_LASTCOMMAND") != 0) {
		/* NOTE: this generally happens when a new command is added to */
		/*   the _FobMsgList[] array, but the corresponding enumerated */
		/*   value is not added to the _FobCommand type definition.    */
		vrErrPrintf(RED_TEXT "The Flock of Birds source code is not consistent -- '_FobMsgList[]'\n" NORM_TEXT);
		return(-1);
	}
#endif

	/***************************************/
	/* Make initial contact with the Flock */

	/* new for ms */
	_FobSendCommand(aux, MSG_WAKE_UP);
	_MsGetPacket(aux);
	if (_FobGetFlockStatus(aux) < 0)
		return(-1);

	/* print out the birds' status */
	if (vrDbgDo(ASCFOB_DBGLVL))
		_FobPrintFlockStatus(stderr, aux);

#if 0 /* non ms { */
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
	if (_FobGetFlockStatus(aux) < 0)
		return(-1);

	/* print out the birds' status */
	if (vrDbgDo(ASCFOB_DBGLVL))
		_FobPrintFlockStatus(stderr, aux);


	/***********************************************/
	/* Once we have the status information, we can */
	/*   automatically determine the Flock setup   */
	if (aux->auto_setup)
		_FobAutoSetup(aux);

	/* value = aaaa00nn, aaaa = transmitter unit, nn = transmitter number (0-3) */
	aux->transmitter_param = ((aux->transmitter_unit & 0x0F) << 4) | (aux->transmitter_num & 0x03);

	/* value = number of bird units (sensors plus transmitters) */
	aux->autoconfig_param = aux->transmitter_count + aux->sensor_count;

	if (vrDbgDo(ASCFOB_DBGLVL)) {
		vrPrintf(BOLD_TEXT "FOB: %d transmitters (unit %d), %d sensors\n" NORM_TEXT,
			aux->transmitter_count, aux->transmitter_unit, aux->sensor_count);
		vrPrintf(BOLD_TEXT "FOB: tranmistter_parm = %02x, autoconfig = %d\n" NORM_TEXT,
			aux->transmitter_param,  aux->autoconfig_param);
	}

	/********************************/
	/* Configure the Flock of Birds */

	/* Set the transmitter (See page 100 of 1999 Flock Guide) */
	/* NOTE: if we don't do this, then we're assuming the transmitter is unit #1 (bad!) */
	/* TODO: figure out what needs to be done for multiple transmitters */
	_FobMsgList[FOB_NEXT_TRANSMITTER].msg[1] = aux->transmitter_param;
	/* only send Next Transmitter command this when trans is not master */
	if (_FobMsgList[FOB_NEXT_TRANSMITTER].msg[1] != 0x00 && _FobMsgList[FOB_NEXT_TRANSMITTER].msg[1] != 0x10) {
		_FobSendCommand(aux, FOB_NEXT_TRANSMITTER);
		_FobGetError(aux, 0);
	}

	/* NOTE: auto config requires a 300ms pre and post command wait period */
	vrSleep(FOB_AUTOCONF_PREDELAY);
	_FobMsgList[FOB_AUTOCONFIG].msg[2] = aux->autoconfig_param;
	vrDbgPrintfN(AALWAYS_DBGLVL, BOLD_TEXT "FOB: Doing Flock of Birds Auto-configuration.\n" NORM_TEXT);
	_FobSendCommand(aux, FOB_AUTOCONFIG);	/* send the autoconfig command to the Master */
	/* (TODO: does Master = unit #1 or Transmitter?) -- probably the latter in which case the Addr stuff is unnece [10/19/2000 -- neither actually, it's the one with the serial connnection] */

	_FobGetError(aux, 0);
	_FobClearPalate(aux);

	/* Check the configuration */
	_FobSendCommand(aux, FOB_EXAMINE_AUTOCONFIG);
	/* TODO: currently hardcoded for normal address mode */
	/* NOTE: returns 5 bytes in Normal address mode, 7 for Extended, and 19 for Super-Extended*/
	bytes_read = vrSerialReadNbytes_Block(aux->fd, buffer, 5);
	vrDbgPrintfN(ASCFOB_DBGLVL, "FOB: autoconfig: %02x %s:%s %s:%s\n", buffer[0],
		vrBinaryToString((unsigned int)(buffer[2]), 8, print_buf),
		vrBinaryToString((unsigned int)(buffer[1]), 8, print_buf+10),
		vrBinaryToString((unsigned int)(buffer[4]), 8, print_buf+20),
		vrBinaryToString((unsigned int)(buffer[3]), 8, print_buf+30));
	/* TODO: if buffer[0-4] is 0x00 here, then there is probably a problem, */
	/*   so should return with an error code. */
#else /* } non ms vs ms { */

	/* TODO: get any error codes */

	/* TODO: determine the MotionStar setup from info it provides */

	/****************************/
	/* Configure the MotionStar */
	/* TODO: this was the auto-config stuff in the Flock code */

	/* configuring the MotionStar is done by getting the status, changing values and sending a "setup" command */
	_FobSendCommand(aux, MSG_GET_STATUS);
	_MsGetPacket(aux);	/* TODO: check that no error occurred */

	/* set the rate */
	sprintf(print_buf, "%06d", (int)(aux->rate*1000));
	for (count = 0; count < 6; count++)
		aux->packet.data[5+count] = print_buf[count];

	/* send the modified setup & get response */
	_FobSendCommand(aux, MSG_SEND_SETUP);
	_MsGetPacket(aux);	/* TODO: check that no error occurred */

	/* TODO: Get the mode in which the transmitter is operating */

#endif /* } */

#if 0 /* disabled in flock and ms code { */
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
#endif /* } */


	/****************************************/
	/* Set the parameters for all the units */

	for (unit = 0; unit < MAX_FOB_UNITS; unit++) {

#if 0 /* non ms { */
		/* only handle units that are in fly mode */
		/* TODO: we may want to be more specific later -- ie not include transmitters */
		if (aux->units[unit].config /* & FOB_CONFIG_FLY */) {
			aux->units[unit].data_type = FOB_SET_OUTPUT_POSITION_ANGLES;

			/* set type of data to report */
			_FobSendCommandAddr(aux, aux->units[0].data_type, unit+1);

			/* set hemisphere to use */
			_FobSendCommandAddr(aux, aux->hemisphere, unit+1);

			_FobClearPalate(aux);

			/* turn on button mode */
			/* NOTE: do for group mode only, though it probably wouldn't do harm when not */
			if (aux->group_mode) {
				if (aux->units[unit].has_button)
					_FobSendCommandAddr(aux, FOB_SET_BUTTON_MODE_ON, unit+1);
				else	_FobSendCommandAddr(aux, FOB_SET_BUTTON_MODE_OFF, unit+1);
				_FobGetError(aux, 0);
			}
		}
#else /* } non ms vs. ms { */
		/* only handle units that are receivers -- TODO: change to use "config" */
		if (aux->units[unit].is_receiver) {
			/* configuring the MotionStar receivers is done by getting the status, changing values and sending a "setup" command */
			_FobSendCommandAddr(aux, MSG_GET_STATUS, unit+1);
			_MsGetPacket(aux);	/* TODO: check that no error occurred */

			dataptr = aux->packet.data;
			/* receiver packet data values are: */
			/*	data[0] -- status */
			/*	data[1] -- id */
			/*	data[2-3] -- software revision */
			/*	data[4] -- error code */
			/*	data[5] -- setup */
			/*	data[6] -- data format */
			/*	data[7] -- report rate */
			/*	data[8-9] -- scaling */
			/*	data[10] -- hemisphere */
			/*	data[11] -- FBB address of this device */
			/*	data[12] -- transmitter type */
			/*	data[13] -- reserved */
			/*	data[14-15] -- reserved */

			dataptr[6] = 0x64;	/* TODO: unhardcode -- 6 words, 4=position/angles */
			dataptr[10] = aux->hemisphere;

			/* added for testing at IAO */
			dataptr[8] = 0;
			dataptr[9] = 144;

			/* The filter code comes from IAO */
			dataptr[5] = 0x05;	/* this turns some filtering on */
			dataptr[7] = 0x02;	/* report info every cycle */
			dataptr[5] = 0x07;	/* uni-st turns on more filtering */

			  /* alpha min */
			  for (count = 0; count < 7; count ++) {
			    dataptr[16+count *2]=0x02;
			    dataptr[16+count *2+1]=0x8F; /* filter coeff */
			  }
			  /* alpha max */
			  for (count = 0; count < 7; count++) {
			    dataptr[30+count*2]=0x73;
			    dataptr[30+count*2+1]=0x33; /* filter coeff */
			  }

			  /* Vm table */
			  dataptr[44]=0x02; /* filter coeff */
			  dataptr[45]=0x0; /* filter coeff */
			  dataptr[46]=0x04; /* was 0x02; *//* filter coeff */
			  dataptr[47]=0x0; /* filter coeff */
			  dataptr[48]=0x08; /* was 0x02; *//* filter coeff */
			  dataptr[49]=0x0; /* filter coeff */
			  dataptr[50]=0x20; /* was 0x10; *//* filter coeff */
			  dataptr[51]=0x0; /* filter coeff */
			  dataptr[52]=0x40; /* was 0x10; *//* filter coeff */
			  dataptr[53]=0x0; /* filter coeff */
			  dataptr[54]=0x00; /* was 0x40; *//* filter coeff */
			  dataptr[55]=0x01; /* was 0x0; *//* filter coeff */
			  dataptr[56]=0x0; /* filter coeff */
			  dataptr[57]=0x02; /* filter coeff */

			/* end of filter code from IAO */



			/* NOTE: uni-Stuttgart code stores the range value from each receiver unit */

			/* setup the button */
			/* TODO: unhardcode */
vrFprintf(stderr, RED_TEXT "unit %d: (status = %s) has a button? -- %d\n" NORM_TEXT, unit, vrBinaryToString((unsigned int)(dataptr[0]), 8, print_buf), dataptr[0] & FLOCK_HAS_BUTTONS);
			if (dataptr[0] & FLOCK_HAS_BUTTONS || unit == 2 /* hardcode for uni-Stuttgart */
#ifdef IAO
									&& 0
#endif
											) {
				aux->units[unit].has_button = 1;
				dataptr[5] |= 0x08;	/* if button, then send the button values */
			} else {
				aux->units[unit].has_button = 0;
				dataptr[5] &= !0x08;	/* if no button, then don't send the button values */
			}

			/* send the modified setup & get response */
			_FobSendCommandAddr(aux, MSG_SEND_SETUP, unit+1);
			_MsGetPacket(aux);	/* TODO: check that no error occurred */

		}
#endif /* } */
	}

#  if defined(IAO) && 0 /* test */
		_FobSendCommandAddr(aux, MSG_GET_STATUS, 4);
		_MsGetPacket(aux);	/* TODO: check that no error occurred */
		dataptr = aux->packet.data;
		dataptr[6] = 0x00;	/* TODO: unhardcode -- 6 words, 4=position/angles */
		dataptr[5] &= !0x08;	/* if no button, then don't send the button values */

		/* send the modified setup & get response */
		_FobSendCommandAddr(aux, MSG_SEND_SETUP, 4);
		_MsGetPacket(aux);	/* TODO: check that no error occurred */

		_FobSendCommandAddr(aux, MSG_GET_STATUS, 1);
		_MsGetPacket(aux);	/* TODO: check that no error occurred */
		dataptr = aux->packet.data;
		dataptr[6] = 0x64;	/* TODO: unhardcode -- 6 words, 4=position/angles */
		dataptr[5] &= !0x08;	/* if no button, then don't send the button values */

		/* send the modified setup & get response */
		_FobSendCommandAddr(aux, MSG_SEND_SETUP, 1);
		_MsGetPacket(aux);	/* TODO: check that no error occurred */

#  endif

	/* perhaps using group mode, otherwise read one at a time */
	if (aux->group_mode) {
#if 0 /* non ms { */
		_FobSendCommand(aux, FOB_SET_GROUP_MODE_ON);
		_FobGetError(aux, 0);
#endif /* } */

		aux->group_data_type = FOB_SET_OUTPUT_POSITION_ANGLES;
	}


	/*******************/
	/* Begin Operation */

#if 0 /* non ms { */
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
	transunit = &(aux->units[aux->transmitter_unit-1]);	/* use the transmitter unit */
	sprintf(aux->version, "model: %s, revision: %d.%d",
		transunit->model, transunit->rev_major, transunit->rev_minor);
	sprintf(aux->op_params, "Crystal: %dMHz", transunit->crystal_rate);
#endif /* } */


	/*****************************/
	/* Set the system parameters */

#if 0
	/* TODO: set the CRT Sync configuration */
	if (aux->crt_sync)
		_FobSendCommand(aux, FOB_CRTSYNC_ON);
	else	_FobSendCommand(aux, FOB_CRTSYNC_OFF);

	/* TODO: set filter mode */
	switch (aux->filter_mode) {
	default:
		_FobSendCommand(aux, FOB_FILTERS_OFF);
		break;
	}
#endif

	/***************************************/
	/* Go into stream mode -- if specified */
	if (aux->stream_mode) {
#if 0 /* non ms { */
		/* Set the report rate */
		vrDbgPrintfN(ASCFOB_DBGLVL, BOLD_TEXT "FOB: Setting report rate\n" NORM_TEXT);
		/* TODO: the report rate should be a config-parameter */
		_FobSendCommand(aux, FOB_SET_REPORT_RATE_HALF);

		vrDbgPrintfN(ASCFOB_DBGLVL, BOLD_TEXT "FOB: Going into Stream Mode\n" NORM_TEXT);
		/* Go into stream mode -- just send to Master unit */
		_FobSendCommand(aux, FOB_STREAM_MODE);
		vrDbgPrintfN(ASCFOB_DBGLVL, BOLD_TEXT "FOB: Post Stream Mode\n" NORM_TEXT);
#else /* } { */
		vrFprintf(stderr, RED_TEXT "Going into continuous mode, this may take a few moments.\n" NORM_TEXT);
		_FobSendCommand(aux, MSG_RUN_CONTINUOUS);
		_MsGetPacket(aux);	/* TODO: check that no error occurred */
#endif /* } */

	}


	/************************/
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

#if 0 /* non ms { */
	/* NOTE: command sequence is basically from the VRPN open-source code */

	/* go into Poll mode "B" */
	_FobSendCommand(aux, FOB_POINT_MODE);

	/* go into sleep mode "G" */
	_FobSendCommand(aux, FOB_SLEEP);

	/* close the serial port */
#if 0
	vrSerialClose(aux->fd);
#endif

#else /* } non ms vs. ms { */
	vrFprintf(stderr, "MS: Closing the MotionStar connection...\n");
	_FobSendCommand(aux, MSG_STOP_DATA);	/* TODO: should really only do this if we know we've begun continuous mode operation */
	_MsGetPacket(aux);	/* TODO: check that no error occurred */

	_FobSendCommand(aux, MSG_SHUT_DOWN);
	_MsGetPacket(aux);	/* TODO: check that no error occurred */

	close(aux->fd);
#endif /* } */

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
#ifdef NOT_YET_IMPLEMENTED
	int	null_value = -1;
#endif /* NOT_YET_IMPLEMENTED */
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
#if 0
	if (vrArgParseInteger(args, "baud", &(aux->baud_int))) {
		aux->baud_enum = vrSerialBaudIntToEnum(aux->baud_int);
	}
#endif

	/****************************************************************************************/
	/** Format: "hemisphere" "=" { "fore" | "aft" | "right" | "left" | "lower" | "upper" } **/
	/****************************************************************************************/
	vrArgParseChoiceInteger(args, "hemisphere", &(aux->hemisphere), hemi_choices, hemi_values);

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
		new_value.t[VR_X] = aux->units[mapped_6sensor].data_pos[0] * scale_trans;
		new_value.t[VR_Y] = aux->units[mapped_6sensor].data_pos[1] * scale_trans;
		new_value.t[VR_Z] = aux->units[mapped_6sensor].data_pos[2] * scale_trans;
		new_value.r[VR_AZIM] = aux->units[mapped_6sensor].data_pos[3];
		new_value.r[VR_ELEV] = aux->units[mapped_6sensor].data_pos[4];
		new_value.r[VR_ROLL] = aux->units[mapped_6sensor].data_pos[5];

		vrAssign6sensorValue(current_6sensor, vrMatrixSetEulerAzimaxis(&tmpmat, &new_value, VR_Z), 0 /*, TODO: timestamp */);
	}
}


	/**********************************************************************/
	/*    Function(s) for parsing Flock of Birds "input" declarations.    */
	/*                                                                    */
	/*  These _Fob<type>Input() functions are called during the CREATE    */
	/*  phase of the input interface.                                     */

/**************************************************************************/
static vrInputMatch _FobButtonInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
static	char		*whitespace = " \t\r\b\n";
	_FobPrivateInfo	*aux = (_FobPrivateInfo *)devinfo->aux_data;
	char		*button_name_instance;
	int		button_num;
	int		unit_number;
	int		button_key;

	vrDbgPrintfN(ASCFOB_DBGLVL, "_FobButtonInput(): instance = '%s'\n" NORM_TEXT, dti->instance);

	button_num = aux->num_buttons;
	aux->num_buttons++;
	unit_number = vrAtoI(dti->instance);
	button_name_instance = strchr(dti->instance, ',') + 1;			/* jump past comma*/

	if (button_name_instance != NULL) {
		button_name_instance += strspn(button_name_instance, whitespace);/* skip white */
		button_key = _FobButtonValue(button_name_instance);
	} else {
		button_key = -1;
	}

	/*************************************************/
	/* verify that the requested 'instance' is valid */
	if (unit_number < 0 || unit_number > MAX_FOB_UNITS)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_FobButtonInput: Warning, sensor number must be between %d and %d\n", 0, MAX_FOB_UNITS); /* TODO: determine better (more specific) last argument */
	else {
		aux->units[unit_number].has_button = 1;
		if (aux->button_inputs[button_num] != NULL)
			vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_FobButtonValue: Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);
	}

	if (button_key == -1)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_FobButtonValue: Warning, button['%s'] did not match any known button\n", dti->instance);
	else if (aux->button_inputs[button_num] != NULL)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_FobButtonValue: Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);

	vrDbgPrintfN(INPUT_DBGLVL, "_FobButtonValue: assigned button #%d -- type %d of sensor %d\n",
		button_num, button_key, unit_number);

	/***********************/
	/* now setup the input */
	aux->button_inputs[button_num] = (vr2switch *)input;
	aux->button_map_unit[button_num] = unit_number;
	aux->button_map_button[button_num] = button_key;
	aux->button_values[button_num] = 0;	/* TODO: should vrAssign2switch() be used here?  probably */

	vrDbgPrintfN(INPUT_DBGLVL, "assigned button event of value 0x%02x to input pointer = %#p)\n",
		button_num, aux->button_inputs[button_num]);

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/**************************************************************************/
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
		/* TODO: print a warning here that we're not making this input */
		return VRINPUT_MATCH_UNABLE;
	}
}

/**************************************************************************/
static vrInputMatch _FobValInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
#ifdef NOT_YET_IMPLEMENTED
	_FobPrivateInfo	*aux = (_FobPrivateInfo *)devinfo->aux_data;
	int		valuatornum;
#endif /* NOT_YET_IMPLEMENTED */

	vrDbgPrintfN(ASCFOB_DBGLVL, "NYI: _FobValInput(): instance = '%s'\n" NORM_TEXT, dti->instance);

#ifdef NOT_YET_IMPLEMENTED
	valuatornum = _FobValuatorValue(dti->instance);
	if (valuatornum == -1)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_FobValInput: Warning, valuator['%s'] did not match any known valuator\n", dti->instance);
	else if (aux->valuator_inputs[valuatornum] != NULL)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_FobValInput: Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);
	aux->valuator_inputs[valuatornum] = (vrValuator *)input;
	if (dti->instance[0] == '-')
		aux->valuator_sign[valuatornum] = -1.0;
	else	aux->valuator_sign[valuatornum] =  1.0;
	vrDbgPrintfN(INPUT_DBGLVL, "assigned valuator event of value 0x%02x to input pointer = %#p)\n",
		valuatornum, aux->valuator_inputs[valuatornum]);
#endif /* NOT_YET_IMPLEMENTED */

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/**************************************************************************/
static vrInputMatch _FobReceiverInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
	_FobPrivateInfo		*aux = (_FobPrivateInfo *)devinfo->aux_data;
	int			sensor6_num;
	int			unit_number;

	vrDbgPrintfN(ASCFOB_DBGLVL, "_FobReceiverInput(): instance = '%s'\n" NORM_TEXT, dti->instance);

	sensor6_num = aux->num_6sensors;
	aux->num_6sensors++;
	unit_number = vrAtoI(dti->instance);

	/*************************************************/
	/* verify that the requested 'instance' is valid */
	if (unit_number < 0)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_FobReceiverInput: Warning, sensor number must be between %d and %d\n", 0, MAX_FOB_UNITS); /* TODO: determine better (more specific) last argument */
	else if (aux->sensor6_inputs[sensor6_num] != NULL)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_FobReceiverInput: Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);

	/***********************/
	/* now setup the input */
	aux->sensor6_inputs[sensor6_num] = (vr6sensor *)input;
#if 0
	aux->sensor6_values[sensor6_num].t[VR_X] = 0.0;
	aux->sensor6_values[sensor6_num].t[VR_Y] = 0.0;
	aux->sensor6_values[sensor6_num].t[VR_Z] = 0.0;
	aux->sensor6_values[sensor6_num].r[VR_AZIM] = 0.0;
	aux->sensor6_values[sensor6_num].r[VR_ELEV] = 0.0;
	aux->sensor6_values[sensor6_num].r[VR_ROLL] = 0.0;
	aux->sensor6_dummy[sensor6_num] = 0;
#endif
	aux->sensor6_map[sensor6_num] = unit_number;

	/* the vrAssign6sensorR2Exform() function does the parsing of the next token */
	vrAssign6sensorR2Exform(aux->sensor6_inputs[sensor6_num], strchr(dti->instance, ','));

	vrDbgPrintfN(INPUT_DBGLVL, "assigned 6sensor event of value 0x%02x to input pointer = %#p)\n",
		unit_number, aux->sensor6_inputs[sensor6_num]);

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
static void _MsCreateFunction(vrInputDevice *devinfo)
{
	/*** List of possible inputs ***/
static	vrInputFunction	_FobInputs[] = {
				{ "", VRINPUT_2WAY, _FobButtonInput },
				{ "button", VRINPUT_2WAY, _FobButtonInput },
				{ "birdmouse", VRINPUT_2WAY, _FobButtonInput },
				{ "zerorot", VRINPUT_2WAY, _FobButtonZerorotInput },
				{ "slider", VRINPUT_VALUATOR, _FobValInput },
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
#if 0
	aux->baud_enum = vrSerialBaudIntToEnum(aux->baud_int);
#endif
	_FobParseArgs(aux, devinfo->args);

	/***************************************/
	/* create the inputs and self-controls */

	/* Because a MotionStar can have a large unknown number of inputs, we allocate  */
	/*   memory for handling them here.  First the vrInputCreateDataContainers()    */
	/*   function is called to determine how many of each type of input needs to be */
	/*   created.  Normally vrInputCreateDataContainers() is called by              */
	/*   vrInputCreateDataContainers() (see below), but for this circumstance we    */
	/*   split the two operations.                                                  */
	vrInputCountDataContainers(devinfo);

	aux->num_buttons = 0;
	aux->button_inputs = (vr2switch **)vrShmemAlloc((devinfo->num_2ways + devinfo->num_scontrols) * sizeof(vr2switch *));
	aux->button_map_unit = (int *)vrShmemAlloc((devinfo->num_2ways + devinfo->num_scontrols) * sizeof(int));
	aux->button_map_button = (int *)vrShmemAlloc((devinfo->num_2ways + devinfo->num_scontrols) * sizeof(int));
	aux->button_values = (int *)vrShmemAlloc((devinfo->num_2ways + devinfo->num_scontrols) * sizeof(int));

	aux->num_6sensors = 0;
	aux->sensor6_inputs = (vr6sensor **)vrShmemAlloc(devinfo->num_6sensors * sizeof(vr6sensor *));
#if 0
	aux->sensor6_values = (vrEuler *)vrShmemAlloc(devinfo->num_6sensors * sizeof(vrEuler));
#endif
	aux->sensor6_map = (int *)vrShmemAlloc(devinfo->num_6sensors * sizeof(int));

	/* Here we return to the conventional way of creating the inputs */
	vrInputCreateDataContainers(devinfo, _FobInputs);
	vrInputCreateSelfControlContainers(devinfo, _FobInputs, _FobControlList);
	/* TODO: anything to do for NON self-controls?  Implement for Magellan first. */

	vrDbgPrintf("Done creating Flock of Birds inputs for '%s'\n", devinfo->name);
	devinfo->created = 1;

	return;
}


/************************************************************/
static void _MsOpenFunction(vrInputDevice *devinfo)
{
	vrTrace("_MsOpenFunction", devinfo->name);

	_FobPrivateInfo	*aux = (_FobPrivateInfo *)devinfo->aux_data;

	/*******************/
	/* open the device */
#if 0
	aux->fd = vrSerialOpen(aux->port, aux->baud_enum);
#else
	/* new for ms */
	aux->fd = socket(AF_INET, SOCK_STREAM, 0);
#endif
	if (aux->fd < 0) {
		aux->open = 0;
		vrErrPrintf("(%s::_MsFunction::%d) error: "
			RED_TEXT "couldn't open socket port\n" NORM_TEXT,
			__FILE__, __LINE__);
		sprintf(aux->version, "- unconnected Flock of Birds -");
	} else {
		int con;

		aux->open = 1;
		/* new for ms */
		bzero((void *)&aux->server, sizeof(aux->server));
		aux->server.sin_family = AF_INET;
		aux->server.sin_port=htons(aux->bird_port);
#ifdef IAO
		aux->server.sin_addr.s_addr = inet_addr("137.251.54.150"); /* TODO: unhardcode this */
#else
		aux->server.sin_addr.s_addr = inet_addr("129.69.29.125"); /* TODO: unhardcode this */
#endif
printf("just set inet_address \n");
#if defined(__linux) || defined(__APPLE__) || defined(__CYGWIN__)
		if (connect(aux->fd, (struct sockaddr *)&aux->server, sizeof(aux->server))) /* TODO: why is this "sockaddr", when the type is "sockaddr_in" ?? */
#else
		if (con = connect(aux->fd, (struct SOCKADDR *)&aux->server, sizeof(aux->server)))
#endif
		{
printf("aux->server = %#p, sizeof() = %d, con = %d\n", &aux->server, sizeof(aux->server), con);
			vrFprintf(stderr, "can't connect to MotionStar\n");
			vrFprintf(stderr, BOLD_TEXT "Perhaps another program is communicating with it.\n" NORM_TEXT);
			aux->connected = 0;
			close(aux->fd);
			return;
		} else {
			aux->connected = 1;
		}

		/* TODO: NOTE in the FOB code, we call _FobInitializeDevice(aux) here!  Don't we need to do the same for the MotionStar? */

		/* TODO: check to make sure device opened properly, and fix if not */
		if (0 /* TODO: s/b (_FobInitializeDevice(aux) < 0) */) {
			vrErrPrintf("_MsOpenFunction(): "
				RED_TEXT "Warning, left Flock not or improperly connected to '%s' Flock.\n" NORM_TEXT, devinfo->name);
		} else {
			vrDbgPrintf("_MsOpenFunction(): Done opening MotionStar input device '%s'.\n", devinfo->name);
			devinfo->operating = 1;
		}
	}

	return;
}


/************************************************************/
static void _MsCloseFunction(vrInputDevice *devinfo)
{
	_FobPrivateInfo	*aux = (_FobPrivateInfo *)devinfo->aux_data;

	if (aux != NULL) {
		_FobCloseDevice(aux);
#if 0
		vrSerialClose(aux->fd);
#endif
		vrShmemFree(aux);	/* aka devinfo->aux_data */
	}

	return;
}


/************************************************************/
static void _MsResetFunction(vrInputDevice *devinfo)
{
	_FobPrivateInfo	*aux = (_FobPrivateInfo *)devinfo->aux_data;

	if (aux != NULL) {
		_FobResetDevice(aux);
	}

	return;
}


/************************************************************/
static void _MsPollFunction(vrInputDevice *devinfo)
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
void vrMsInitInfo(vrInputDevice *devinfo)
{
	devinfo->version = (char *)vrShmemStrDup("-get from Flock of Birds device-");
	devinfo->Create = vrCallbackCreateNamed("MStarInput:Create-Def", _MsCreateFunction, 1, devinfo);
	devinfo->Open = vrCallbackCreateNamed("MStarInput:Open-Def", _MsOpenFunction, 1, devinfo);
	devinfo->Close = vrCallbackCreateNamed("MStarInput:Close-Def", _MsCloseFunction, 1, devinfo);
	devinfo->Reset = vrCallbackCreateNamed("MStarInput:Reset-Def", _MsResetFunction, 1, devinfo);
	devinfo->PollData = vrCallbackCreateNamed("MStarInput:PollData-Def", _MsPollFunction, 1, devinfo);
	devinfo->PrintAux = vrCallbackCreateNamed("MStarInput:PrintAux-Def", _FobPrintStruct, 0);
}


#endif /* } FREEVR */



	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/



#if defined(TEST_APP) /* { */


/******************************************************************/
/* Ugh, I hate globals, but I don't know a better way to get the  */
/*   aux value into the interrupt signal function exit_testapp(). */
static	int	done = -1;


/*******************************************************************/
void _RenderValuesLine(_FobPrivateInfo *aux)
{
	int		unit;
	_FobUnit	*unitdata;		/* pointer to the current unit */

	for (unit = 0; unit < MAX_FOB_UNITS; unit++) {
		unitdata = &(aux->units[unit]);
		/* nothing to print for non existent sensors */
#if 0 /* non ms { */
		if ((unitdata->config & FOB_CONFIG_SENSOR) == 0)
			continue;
#else /* }{ */
		if (!unitdata->is_receiver)
			continue;
#endif /* } */

		if (unitdata->has_button) {
			vrPrintf("  Bird %d(%d): b=%02x %6.1f %6.1f %6.1f %6.1f %6.1f %6.1f",
				unit+1,	/* NOTE: ms doesn't add one to number -- never mind, yes it does */
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
				unit+1,	/* NOTE: ms doesn't add one to number -- never mind, yes it does */
				unitdata->data_sensor,
				unitdata->data_pos[0],	/* X */
				unitdata->data_pos[1],	/* Y */
				unitdata->data_pos[2],	/* Z */
				unitdata->data_pos[3],	/* AZIM */
				unitdata->data_pos[4],	/* ELEV */
				unitdata->data_pos[5]);	/* ROLL */
		}
	}
	if (aux->button_system) {
		/* if using the feedthrough button system, then print the system-button info */
		vrPrintf("  But ft=%02x", aux->data_button);
	}
	vrPrintf("\n");
}


/*******************************************************************/
/* NOTE: this routine relies on using the ANSI terminal character */
/*   codes.  Since this is just a test application, I decided to  */
/*   forgo the requirement of the curses library, and instead     */
/*   limit the terminal on which this version would work.         */
void _RenderValuesScreen(_FobPrivateInfo *aux)
{
	int		unit;
	_FobUnit	*unitdata;		/* pointer to the current unit */
	int		num_lines = 40;		/* number of lines for showing chart */
	float		max_trans = 96.0 / 12.0 /*ms*/;	/* 8 feet in one direction */
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
		unitdata = &(aux->units[unit]);

		/* nothing to print for non existent sensors */
#if 0 /* non ms { */
		if ((unitdata->config & FOB_CONFIG_SENSOR) == 0)
			continue;
#else /* }{ */
		if (!unitdata->is_receiver)
			continue;
#endif /* } */

	/* NOTE: in ms, don't add one to unit */
		/* move to top of column and print unit # (underlined) */
		vrPrintf("[2;%dH[4mBird_%d:%d[24m", unit*10 + 5, unit+1, unitdata->data_sensor);

		/* print button value (if appropriate) */
		if (unitdata->has_button)
			vrPrintf("[3;%dH (%02x) ", unit*10 + 5, unitdata->data_button);

		if (unit == 1 && aux->button_system) {
			/* if using the feedthrough button system, then print the system-button info */
			vrPrintf("[3;%dH (sys %02x) ", unit*10 + 5, aux->data_button);
		}

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
	int			rendermode = 1;
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
#if 0
	aux->port = getenv("FOB_TTY");
	if (aux->port == NULL)
		aux->port = DEFAULT_PORT;		/* default, if no file given */
	aux->baud_int = DEFAULT_BAUD;			/* default, if no baud given */
	aux->baud_enum = vrSerialBaudIntToEnum(aux->baud_int);
#endif

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

	if (getenv("FOB_LINERENDER") != NULL)
		rendermode = 0;

	/* terminate after acquiring and rendering just one set of inputs */
	if (getenv("FOB_JUSTONCE") != NULL)
		justonce = 1;


	/*****************************************************/
	/* adjust parameters based on command-line-arguments */
	/* TODO: parse CLAs for non-default serial port, baud, etc. */


	/**************************************************/
	/* open the serial port and initialize the device */
	/* TODO: probably should set TIOCNOTTY for the FoB port */
#if 0
	aux->fd = vrSerialOpen(aux->port, aux->baud_enum);
#else
	/* new for ms */
	aux->fd = socket(AF_INET, SOCK_STREAM, 0);
#endif
	if (aux->fd < 0) {
		aux->open = 0;
		vrFprintf(stderr, RED_TEXT "couldn't open socket.\n" NORM_TEXT);
		sprintf(aux->version, "- unconnected Flock of Birds -");

		exit (1);
	} else {
		int con;
		aux->open = 1;

		/* new for ms */
		bzero(aux->server, sizeof(aux->server));
		aux->server.sin_family = AF_INET;
		aux->server.sin_port = htons(6000 /* aux->bird_port */);
#ifdef IAO
		aux->server.sin_addr.s_addr = inet_addr("137.251.54.150"); /* TODO: unhardcode this */
#else
		aux->server.sin_addr.s_addr = inet_addr("129.69.29.125"); /* TODO: unhardcode this */
#endif
#ifdef __linux
		if (connect(aux->fd, (sockaddr *)&aux->server, sizeof(aux->server))) /* TODO: why is this "sockaddr", when the type is "sockaddr_in" ?? */
#else
		if (con = connect(aux->fd, (struct SOCKADDR *)&(aux->server), sizeof(aux->server)))
#endif
		{
printf("aux->server = %#p, sizeof() = %d, con = %d\n", &aux->server, sizeof(aux->server), con);
			vrFprintf(stderr, "can't connect to MotionStar\n");
			vrFprintf(stderr, BOLD_TEXT "Perhaps another program is communicating with it.\n" NORM_TEXT);
			aux->connected = 0;
			close(aux->fd);
			exit(-1);
		} else {
printf("Connected: aux->server = %#p, sizeof() = %d, con = %d\n", &aux->server, sizeof(aux->server), con);
			aux->connected = 1;
		}

		if (_FobInitializeDevice(aux) < 0) {
			vrErrPrintf("main: "
				RED_TEXT "Warning, unable to establish communications with Flock of Birds.\n" NORM_TEXT);
		} else {
			vrPrintf(BOLD_TEXT "MotionStar device has been initialized.\n" NORM_TEXT);
			aux->operating = 1;
		}
	}

	vrPrintf("\n");
	_FobPrintStruct(stdout, aux, verbose);

	if (aux->open == 0) {
		vrPrintf(RED_TEXT "Quitting due to lack of communication with the Flock.\n" NORM_TEXT);
		exit(1);
	}

	/* print the values that we've already received */
	_RenderValuesLine(aux);


	/**********************/
	/* display the output */

	/* for screen-rendering, scroll everything up so it won't be erased */
	if (rendermode == 1 && !justonce) {
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
		case 0:
			_RenderValuesLine(aux);
			break;
		case 1:
			_RenderValuesScreen(aux);
			break;
		}
	}


	/*****************/
	/* close up shop */
	_FobCloseDevice(aux);
	vrPrintf(BOLD_TEXT "\nMotionStar device closed\n" NORM_TEXT);
}

#endif /* } TEST_APP */



	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/



#if defined(CAVE) || defined(TRACKD_SA) /* { */

/*** local variables ***/
static	_FobPrivateInfo	*ms_static = NULL;

/*******************************************************************************/
/*** public functions for accessing the Ascension MotionStar as a            ***/
/***   tracking device:                                                      ***/
/***     void CAVEInitMotionstarTracking(CAVE_ST *cave)                      ***/
/***     void CAVEResetMotionstarTracking(CAVE_ST *cave)                     ***/
/***     int CAVEGetMotionstarTracking(CAVE_ST *cave, CAVE_SENSOR_ST *sensor)***/
/***     -------                                                             ***/
/***     void CAVEReadMotionstarController(CAVE_ST *, CAVE_CONTROLLER_ST *)  ***/
/***     int CAVENumMotionstarButtons(CAVE_ST *cave)                         ***/
/***     int CAVENumMotionstarValuators(CAVE_ST *cave)                       ***/
/*******************************************************************************/

/*****************************************************************/
void CAVEInitMotionstarTracking(CAVE_ST *cave)
{
	_FobPrivateInfo	*aux;

	/******************************/
	/* setup the device structure */
	ms_static = (_FobPrivateInfo *)malloc(sizeof(_FobPrivateInfo));
	aux = ms_static;
	memset(aux, 0, sizeof(_FobPrivateInfo));
	_FobInitializeStruct(aux, "fob");

	aux->fd = socket(AF_INET, SOCK_STREAM, 0);

	if (aux->fd < 0) {
		aux->open = 0;
		vrFprintf(stderr, RED_TEXT "couldn't open socket.\n" NORM_TEXT);
		sprintf(aux->version, "- unconnected Flock of Birds -");

		exit (1);
	} else {
		int con;
		aux->open = 1;

		/* new for ms */
		bzero(aux->server, sizeof(aux->server));
		aux->server.sin_family = AF_INET;
		aux->server.sin_port = htons(6000 /* aux->bird_port */);
#ifdef IAO
		aux->server.sin_addr.s_addr = inet_addr("137.251.54.150"); /* TODO: unhardcode this */
#else
		aux->server.sin_addr.s_addr = inet_addr("129.69.29.125"); /* TODO: unhardcode this */
#endif
#ifdef __linux
		if (connect(aux->fd, (sockaddr *)&aux->server, sizeof(aux->server))) /* TODO: why is this "sockaddr", when the type is "sockaddr_in" ?? */
#else
		if (con = connect(aux->fd, (struct SOCKADDR *)&(aux->server), sizeof(aux->server)))
#endif
		{
printf("aux->server = %#p, sizeof() = %d, con = %d\n", &aux->server, sizeof(aux->server), con);
			fprintf(stderr, "can't connect to MotionStar\n");
		} else {
printf("Connected: aux->server = %#p, sizeof() = %d, con = %d\n", &aux->server, sizeof(aux->server), con);
			aux->connected = 1;
		}

		if (_FobInitializeDevice(aux) < 0) {
			vrErrPrintf("main: "
				RED_TEXT "Warning, unable to establish communications with Flock of Birds.\n" NORM_TEXT);
		} else {
			aux->operating = 1;
		}
	}

	vrPrintf("\n");
	_FobPrintStruct(stdout, aux, verbose);

	if (aux->open == 0) {
		vrPrintf(RED_TEXT "Quitting due to lack of communication with the Flock.\n" NORM_TEXT);
		exit(1);
	}

#if 0 /* I'm not sure if this is needed -- perhaps CAVENumMotionstarSensors() is enough */
	cave->num_sensors = 3;	/* TODO: unhardcode this */
#endif

}


/*******************************************************************/
void CAVEExitMotionstarTracking(CAVE_ST *cave)
{
	_FobPrivateInfo	*aux = ms_static;	/* point to the static data structure */

	_FobCloseDevice(aux);
	vrPrintf(BOLD_TEXT "\nMotionStar device closed\n" NORM_TEXT);
}


/*******************************************************************/
void CAVEResetMotionstarTracking(CAVE_ST *cave)
{
	_FobPrivateInfo	*aux = ms_static;	/* point to the static data structure */

	_FobInitializeStruct(aux, "ms");

	vrSleep(20000);
}


/*******************************************************************/
int CAVEGetMotionstarTracking(CAVE_ST *cave, CAVE_SENSOR_ST *sensor)
{
	_FobPrivateInfo	*aux = ms_static;	/* point to the static data structure */

	/* read the data */
	_FobReadInput(aux);

#if 1 /* TODO: this is a hack filter needed at IAO */
  {
	int unit, count;
	for (unit = 1; unit <= 3; unit++) {
		for (count = 0; count < 6; count ++) {
			aux->units[unit].data_pos[count] += aux->units[unit].old_dp[count];
			aux->units[unit].data_pos[count] += aux->units[unit].old_dp2[count];
			aux->units[unit].data_pos[count] += aux->units[unit].old_dp3[count];
			aux->units[unit].data_pos[count] /= 4.0;
			aux->units[unit].old_dp3[count] = aux->units[unit].old_dp2[count];
			aux->units[unit].old_dp2[count] = aux->units[unit].old_dp[count];
			aux->units[unit].old_dp[count] = aux->units[unit].data_pos[count];
		}
	}
  }
#endif


	/* fill in controller info while we're here */
	if (aux->button_system) {
		/* This is for the feedthough method of buttons (eg. at IAO) */
		aux->controls.button[0] = (aux->data_button & 0x01) != 0;
		aux->controls.button[1] = (aux->data_button & 0x02) != 0;
		aux->controls.button[2] = (aux->data_button & 0x04) != 0;

		aux->controls.valuator[0] = 0.0;
#if 0 /* hack: no joystick at uni-stuttgart */
		aux->controls.valuator[1] = 0.0;
#else
		aux->controls.valuator[1] = 0.0;
		aux->controls.valuator[1] += 0.8 * ((aux->data_button & 0x10) != 0);
		aux->controls.valuator[1] -= 0.5 * ((aux->data_button & 0x20) != 0);
#endif
	} else {
		/* TODO: unhardcode the fact that the button data is in unit 2 */
		/* TODO: need to interpret data_button differently at IAO */
		aux->controls.button[0] = (aux->units[2].data_button == DEVICE_BUTTON_x1x);
		aux->controls.button[1] = (aux->units[2].data_button == DEVICE_BUTTON_x2x);
		aux->controls.button[2] = (aux->units[2].data_button == DEVICE_BUTTON_x3x);

		aux->controls.valuator[0] = 0.0;
#if 0 /* hack: no joystick at uni-stuttgart */
		aux->controls.valuator[1] = 0.0;
#else
		aux->controls.valuator[1] = 0.5 * (aux->units[2].data_button == DEVICE_BUTTON_x2x);
#endif
	}

	/* TODO: unhardcode this stuff */
	aux->head.x =  aux->units[1].data_pos[VR_Y];
	aux->head.y = -aux->units[1].data_pos[VR_Z];
	aux->head.z = -aux->units[1].data_pos[VR_X];
	aux->head.azim = -aux->units[1].data_pos[VR_AZIM+3];
	aux->head.elev =  aux->units[1].data_pos[VR_ELEV+3];
	aux->head.roll = -aux->units[1].data_pos[VR_ROLL+3];
#if 0
	aux->head.timestamp = 0;
#endif
	aux->head.calibrated = 0;
	aux->head.frame = CAVE_TRACKER_FRAME;

	aux->wand1.x =  aux->units[2].data_pos[VR_Y];
	aux->wand1.y = -aux->units[2].data_pos[VR_Z];
	aux->wand1.z = -aux->units[2].data_pos[VR_X];
	aux->wand1.azim = -aux->units[2].data_pos[VR_AZIM+3];
	aux->wand1.elev =  aux->units[2].data_pos[VR_ELEV+3];
	aux->wand1.roll = -aux->units[2].data_pos[VR_ROLL+3];
#if 0
	aux->wand1.timestamp = 0;
#endif
	aux->wand1.calibrated = 0;
	aux->wand1.frame = CAVE_TRACKER_FRAME;

	aux->wand2.x =  aux->units[3].data_pos[VR_Y];
	aux->wand2.y = -aux->units[3].data_pos[VR_Z];
	aux->wand2.z = -aux->units[3].data_pos[VR_X];
	aux->wand2.azim = -aux->units[3].data_pos[VR_AZIM+3];
	aux->wand2.elev =  aux->units[3].data_pos[VR_ELEV+3];
	aux->wand2.roll = -aux->units[3].data_pos[VR_ROLL+3];
#if 0
	aux->wand2.timestamp = 0;
#endif
	aux->wand2.calibrated = 0;
	aux->wand2.frame = CAVE_TRACKER_FRAME;

#ifdef IAO /* hack stuff because CAVE config isn't flexible enough */
	aux->wand1.z -= 1.5;
	aux->wand1.y -= 1.0;
#endif

#if 1
printf("\rhead.x = %f", aux->head.x);
#else
usleep(500000);
#endif
	sensor[0] = aux->head;
	sensor[1] = aux->wand1;
	sensor[2] = aux->wand2;

}


/*******************************************************************/
int CAVENumMotionstarSensors(CAVE_ST *cave)
{
        return 3;
}


/*******************************************************************/
void CAVEInitMotionstarController(CAVE_ST *cave)
{
	_FobPrivateInfo	*aux = ms_static;	/* point to the static data structure */

	if (aux == NULL || aux->open == 0) {
		fprintf(stderr, "CAVEInitMotionstarController(): MotionStar not initialized yet.\n");
		return;
	}

	/* nothing to do because the tracker functions should do all the initialization */
}


/*******************************************************************/
void CAVEReadMotionstarController(CAVE_ST *cave, volatile CAVE_CONTROLLER_ST *control)
{
	_FobPrivateInfo	*aux = ms_static;	/* point to the static data structure */

	if (aux == NULL || aux->open == 0) {
		fprintf(stderr, "CAVE tracker: MotionStar not initialized yet.\n");
		return;
	}

	control->button[0] = aux->controls.button[0];
	control->button[1] = aux->controls.button[1];
	control->button[2] = aux->controls.button[2];

	control->valuator[0] = aux->controls.valuator[0];
	control->valuator[1] = aux->controls.valuator[1];
}


/*******************************************************************/
int CAVENumMotionstarButtons(CAVE_ST *cave)
{
        return 3;
}

/*******************************************************************/
int CAVENumMotionstarValuators(CAVE_ST *cave)
{
        return 2;
}

/******************************/
/*** For CAVE DSO operation ***/
/* initialize the tracking functions */
void DSOInitTrackerFunctions(CAVETrackerClass_t *class)
{
	class->init_fn = CAVEInitMotionstarTracking;
	class->read_fn = CAVEGetMotionstarTracking;
	class->num_sensors_fn = CAVENumMotionstarSensors;
	class->reset_fn = CAVEResetMotionstarTracking;
	class->exit_fn = CAVEExitMotionstarTracking;
}

/* initialize the tracking functions */
void DSOInitControllerFunctions(CAVEControllerClass_t *class)
{
	class->init_fn = CAVEInitMotionstarController;
	class->read_fn = CAVEReadMotionstarController;
	class->exit_fn = NULL;
	class->num_buttons_fn = CAVENumMotionstarButtons;
	class->num_valuators_fn = CAVENumMotionstarValuators;
}



#endif /* } CAVE || TRACKD_SA */



	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/



#if defined(TRACKD_SA) /* { */

/*******************************************************************************/
/*** public functions for accessing the Ascension MotionStar as a            ***/
/***   tracking device:                                                      ***/
/* TODO: fix these comments */
/***     void TRACKDInitMotionstarTracking(struct TRACKD_CONTROLLER *controller, char *portname) ***/
/***     void TRACKDResetMotionstarTracking()                     ***/
/***     int TRACKDGetMotionstarTracking()***/
/***     -------                                                             ***/
/***     void TRACKDReadMotionstarController()  ***/
/***     int TRACKDNumMotionstarButtons()                         ***/
/***     int TRACKDNumMotionstarValuators()                       ***/
/*******************************************************************************/

/*****************************************************************/
/* TODO: NOTE: Almost all of this is exactly from CAVEInitMotionstarTracking(), which really */
/*   should just be used with NULL passed as *cave -- it isn't used anyway */
int TRACKDInitMotionstarTracking(struct TRACKD_TRACKING *tracker, char *portname)
{
#if 1
	CAVEInitMotionstarTracking(NULL);
#else
	_FobPrivateInfo	*aux;

	/******************************/
	/* setup the device structure */
	ms_static = (_FobPrivateInfo *)malloc(sizeof(_FobPrivateInfo));
	aux = ms_static;
	memset(aux, 0, sizeof(_FobPrivateInfo));
	_FobInitializeStruct(aux, "fob");

	aux->fd = socket(AF_INET, SOCK_STREAM, 0);

	if (aux->fd < 0) {
		aux->open = 0;
		vrFprintf(stderr, RED_TEXT "couldn't open socket.\n" NORM_TEXT);
		sprintf(aux->version, "- unconnected Flock of Birds -");

		exit (1);
	} else {
		int con;
		aux->open = 1;

		/* new for ms */
		bzero(aux->server, sizeof(aux->server));
		aux->server.sin_family = AF_INET;
		aux->server.sin_port = htons(6000 /* aux->bird_port */);
		aux->server.sin_addr.s_addr = inet_addr("129.69.29.125"); /* TODO: unhardcode this */
#ifdef __linux
		if (connect(aux->fd, (sockaddr *)&aux->server, sizeof(aux->server))) /* TODO: why is this "sockaddr", when the type is "sockaddr_in" ?? */
#else
		if (con = connect(aux->fd, (struct SOCKADDR *)&(aux->server), sizeof(aux->server)))
#endif
		{
printf("aux->server = %#p, sizeof() = %d, con = %d\n", &aux->server, sizeof(aux->server), con);
			fprintf(stderr, "can't connect to MotionStar\n");
		} else {
printf("Connected: aux->server = %#p, sizeof() = %d, con = %d\n", &aux->server, sizeof(aux->server), con);
			aux->connected = 1;
		}

		if (_FobInitializeDevice(aux) < 0) {
			vrErrPrintf("main: "
				RED_TEXT "Warning, unable to establish communications with Flock of Birds.\n" NORM_TEXT);
		} else {
			aux->operating = 1;
		}
	}

	vrPrintf("\n");
	_FobPrintStruct(stdout, aux, verbose);

	if (aux->open == 0) {
		vrPrintf(RED_TEXT "Quitting due to lack of communication with the Flock.\n" NORM_TEXT);
		exit(1);
	}
#endif

	tracker->header.numSensors = 3;		/* TODO: unhardcode this */

#if 0 /* this is done in tracker_init_data() */
	CAVE_SENSOR_ST	zero_sensor;
	int		count;

	tracker->header.version = CAVELIB_2_6;
	tracker->header.sensorSize = sizeof(CAVE_SENSOR_ST);
	tracker->header.timestamp[0] = tracker->header.timestamp[1] = 0;
	tracker->header.command = 0;

	zero_sensor.x = 0.0;
	zero_sensor.y = 0.0;
	zero_sensor.z = 0.0;
	zero_sensor.elev = 0.0;
	zero_sensor.azim = 0.0;
	zero_sensor.roll = 0.0;
	zero_sensor.calibrated = 0;

	for (count=0; count < TRACKD_MAX_SENSORS; ++count)
		tracker->sensor[count] = zero_sensor;
#endif

	return 1;
}


/*****************************************************************/
int TRACKDCloseMotionstarTracking()
{
	return 1;
}


/*****************************************************************/
int TRACKDGetMotionstarTracking(CAVE_SENSOR_ST *sensor_data)
{
	/* TODO: note that CAVEGetMotionstarTracking() would work here, just need to pass NULL as *cave */
	CAVEGetMotionstarTracking(NULL, sensor_data);
	return 1;
}


/*****************************************************************/
int TRACKDInitMotionstarController(struct TRACKD_CONTROLLER *controller, char *portname)
{
	_FobPrivateInfo	*aux = ms_static;	/* point to the static data structure */
	int		count;

	if (aux == NULL || aux->open == 0) {
		fprintf(stderr, "TRACKDInitMotionstarController(): MotionStar not initialized yet.\n");
		return;
	}

	controller->header.numButtons = 3;		/* TODO: unhardcode this */
	controller->header.numValuators = 2;		/* TODO: unhardcode this */
	controller->controller.num_buttons = 3;		/* TODO: unhardcode this */
	controller->controller.num_valuators = 2;		/* TODO: unhardcode this */

	return 1;
}


/*****************************************************************/
int TRACKDCloseMotionstarController()
{
	return 1;
}


/*****************************************************************/
int TRACKDGetMotionstarController(CAVE_CONTROLLER_ST *controller_data)
{
	CAVEReadMotionstarController(NULL, controller_data);
	return 1;
}


	/* ================================================= */
	/* Here are the standalone specific parts of the code */


/*** local variables ***/
static int			tracker_shmid = -1, controller_shmid = -1;
static struct TRACKD_TRACKING	*tracker = NULL;
static struct TRACKD_CONTROLLER	*controller = NULL;


/*************************************************************************
 void trackd_cleanup()
	When the daemon is interrupted by SIGINT, this routine closes the
 connections, disposes of the memory, and exits.
**************************************************************************/
void trackd_cleanup(void)
{
	fprintf(stderr, "Interrupted.  Quitting...\n");

	if (tracker) {
		TRACKDCloseMotionstarTracking();
		detach_shared_mem(tracker);
	}
	if (tracker_shmid > -1) {
		remove_shared_mem_id(tracker_shmid);
	}
	if (controller) {
		TRACKDCloseMotionstarController();
		detach_shared_mem(controller);
	}
	if (controller_shmid > -1) {
		remove_shared_mem_id(controller_shmid);
	}
	exit(0);
}


/*************************************************************************
 void get_timestamp()
	Get the current time, and stamp it into the memory provided.
**************************************************************************/
void get_timestamp(uint32_t stamp[2])
{
	struct timeval  curtime;
	gettimeofday(&curtime, NULL);
	stamp[0] = curtime.tv_sec;
	stamp[1] = curtime.tv_usec;
}


/*************************************************************************
 void tracker_init_data()
	Fills in the header of the tracker data, and fills all the sensors
  WITHOUT defaults from the config file.
**************************************************************************/
void tracker_init_data(struct TRACKD_TRACKING *track)
{
	int             count;
	CAVE_SENSOR_ST  sensor;

	if (track == NULL) {
		fprintf(stderr, "No tracker memory, unable to initialize data.\n");
		exit(1);
	}
	track->header.version = CAVELIB_2_6;
	track->header.numSensors = 0;
	track->header.sensorOffset = ((char *) &track->sensor[0]) - ((char *) track);
	track->header.sensorSize = sizeof(CAVE_SENSOR_ST);
	get_timestamp(track->header.timestamp);
	track->header.command = 0;

	sensor.x = 0.0;
	sensor.y = 0.0;
	sensor.z = 0.0;
	sensor.elev = 0.0;
	sensor.azim = 0.0;
	sensor.roll = 0.0;
	sensor.timestamp.sec = 0;
	sensor.timestamp.usec = 0;
	sensor.calibrated = 0;
	sensor.frame = CAVE_TRACKER_FRAME;

	for (count = 0; count < TRACKD_MAX_SENSORS; ++count)
		track->sensor[count] = sensor;
}


/*************************************************************************
 void controller_init_data()
	Fills in the controller header and data with initial values.
**************************************************************************/
void controller_init_data(struct TRACKD_CONTROLLER *cont)
{
	int             loop;

	if (cont == NULL) {
		fprintf(stderr, "No controller memory, unable to initialize data.\n");
		exit(1);
	}
	cont->header.version = CAVELIB_2_6;
	cont->header.buttonOffset = ((char *) &cont->controller.button[0]) - ((char *) cont);
	cont->header.valuatorOffset = ((char *) &cont->controller.valuator[0]) - ((char *) cont);
	cont->header.numButtons = 0;
	cont->header.numValuators = 0;
	get_timestamp(cont->header.timestamp);
	cont->header.command = 0;

	for (loop = 0; loop < CAVE_MAX_BUTTONS; ++loop)
		cont->controller.button[loop] = 0;
	for (loop = 0; loop < CAVE_MAX_VALUATORS; ++loop)
		cont->controller.valuator[loop] = 0.0;
	cont->controller.num_buttons = 0;
	cont->controller.num_valuators = 0;
}


	/* ================================================= */
	/* Here is the standalone main function */


/*************************************************************************/
main(int argc, char *argv[])
{
	int             sensornum = 0;
	key_t           tracker_shmkey = 4126, controller_shmkey = 4127;
	CAVE_SENSOR_ST  sensor;

	signal(SIGINT, trackd_cleanup);	/* catch interrupts to close nicely */

	/***   handle command line arguments here.    ***/
	/*** (ie. to change shared memory keys, etc.) ***/

#if 0
	/* d */
	schedctl(NDPRI, 0, NDPHIMIN);	/* set a non-degrading priority */
#endif


	printf("Using shared memory keys %d and %d.\n", (int) tracker_shmkey, (int) controller_shmkey);
	controller_shmid = get_shared_mem(controller_shmkey, sizeof(*controller), (void *) &controller, 1);
	tracker_shmid = get_shared_mem(tracker_shmkey, sizeof(*tracker), (void *) &tracker, 1);

	/* setup the data structures for the tracker & controller */
	tracker_init_data(tracker);
	controller_init_data(controller);

	/* open the ports, etc. for the tracker & controller */
	if (!TRACKDInitMotionstarTracking(tracker, "TODO: port name goes here")) {
		fprintf(stderr, "Problem initializing tracker.\n");
		exit(1);
	}
	if (!TRACKDInitMotionstarController(controller, "TODO: port name goes here")) {
		fprintf(stderr, "Problem initializing controller.\n");
		exit(1);
	}
	while (1) {

		sensornum = TRACKDGetMotionstarTracking(tracker->sensor);
		if (sensornum != -1) {
			get_timestamp(tracker->header.timestamp);
		}
		if (TRACKDGetMotionstarController(&(controller->controller))) {
			get_timestamp(controller->header.timestamp);
		}
	}
}


#endif /* } TRACKD_SA */


#if defined(TEST_APP) || defined(CAVE) || defined(TRACKD_SA) /* { */

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
#endif /* } TEST_APP || CAVE */


