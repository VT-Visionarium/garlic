/* ======================================================================
 *
 *  CCCCC          vr_input.fastrak.c
 * CC   CC         Author(s): Bill Sherman
 * CC              Created: April 22, 2001 (from vr_input.skeleton.c)
 * CC   CC         Last Modified: April 4, 2011
 *  CCCCC
 *
 * Code file for FreeVR inputs from the Fastrak input device.
 * And other devices that use the same (or an extended version of)
 * protocol as the Polhemus Fastrak, such as the InterSense line
 * of trackers.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/*************************************************************************

USAGE:
	...

	Inputs are specified with the "input" option:
		input "<name>" = "2switch(button[<number>]|<number>)";
		input "<name>" = "valuator(joystick[<number>]|<number>)";
	  :-?	input "<name>" = "valuator(valuator['q'|'m'|'r']|<code>)";   (for reporting tracking qualities as valuators)
		input "<name>" = "6sensor(receiver[<number>]|<number> {, 'id'|'r2e'|'xform ...'})";

	Controls are specified with the "control" option:
		control "<control option>" = "2switch(button[<number>])";
		control "<control option>" = "valuator(...)"; -- NOTE: no valuator oriented controls yet available for this device
	  ...

	Here is the available control options for FreeVR:
		"print_help" -- print info on how to use the input device
		"system_pause_toggle" -- toggle the system pause flag
		"print_context" -- print the overall FreeVR context data structure (for debugging)
		"print_config" -- print the overall FreeVR config data structure (for debugging)
		"print_input" -- print the overall FreeVR input data structure (for debugging)
		"print_struct" -- print the internal Fastrak data structure (for debugging)
	  ...

	Here are the FreeVR configuration argument options for the Fastrak:
		"port" - serial port Fastrak is connected to
			("/dev/input/fastrak" is the default)
			NOTE: the test app can override with FASTRAK_TTY environment variable
		"baud" - baud rate of serial port connection
			(115200 is the default)
		"command" - provide a string of Fastrak commands to send to the device
		"hemisphere" - select the hemisphere tracking will take place in
		"transScale" (or "scale") - scale the units reported by the tracking system
			into FreeVR coordinates


NOTES on the Fastrak protocol and the InterSense IS-900:

	The initial development was done on an IS-900 reporting firmware 3.11
	Beginning with April 15, 2004, development was on an IS-900 reporting
	firmware 4.11 (the same IS-900, but probably in February of 2003, when
	it was setup on our passive stereo display wall, it got an upgrade).
	Beginning in October 2004, I was using an "IS900VWT" with firmware
	version 4.2005.  As of September 24, 2009, the DRI unit reports
	"IS900VET" w/ firmware version 4.2301 -- not sure when the shift
	from reporting VWT to VET occurred (or if it's now connected to the
	other CAVE).

	Here are some of the basic Fastrak protocol commands for manual
	interaction w/ the device:

		S -- Request System Status and ID
		P -- Poll for one record of tracking info
		F -- Put in ASCII output mode
		f -- Put in Binary output mode
		C -- Place in Continuous output mode
		c -- Place in polled output mode
		U -- set output data units to inches
		u -- set output data units to centimeters
		R -- Reset reference frame to default

		l<n>\n -- station state request (on or off) (if <n> = '*' all are reported)
			[NOTE: the IS-900 IT manual makes a mistake on page 111.  It states
			that the command behaves differently when request of stations 1-4 is
			made, but in fact it does not.  It always returns the state of the
			requested station in the 4th byte, followed by 3 bytes with stations
			2, 3 & 4 (more if '*' is the station request).]
		l<n>,{0|1}\n -- disable/enable station <n>
		O<n>, ...\n -- Set the parameters from a particular station
		O<n>\n -- Get the parameters sent from a particular station

		^K - Write current configuration into non-volatile memory
		W - Restore the system to factory defaults
		^Y - Restore the system to poweron settings
		^S - Suspend data transmission
		^Q - Resume data transmission

	InterSense additions to the protocol (NOTE: they all start with 'M'):
		ML0\n -- Turn lights off
		ML1\n -- Turn lights on
		MLB<n>\n -- Turn on light for beacon <n> (lasts about 7 seconds)
		MS\n -- InterSense Status Info (manufacturer specific)
		Ms<n>\n -- Station Status request [<n> = 1-c]
		Mp<n>[,<interval>] -- set/retrieve prediction interval
		MP\n -- Request Tracking status for all stations
		MCF\n -- display the configuration list of Fixed PSEs (ie. beacons)
		MCF<n>\n -- return the configuration data for a beacon
		MCF<n>,x,y,z,xn,yn,zn,IDcode\n -- set the configuration data for a beacon
		MCf<n>,IDcode\n -- remove Fixed PSE info from configuration
		MCC\n -- clear the configuration
		MCe\n -- apply the new configuration data
		MCx\n -- do not apply (cancel) the new configuration data
		ME\n -- reports all the errors in the error log
		MEC\n -- clear all the errors in the error log
		ME{0|1}\n -- disable/enable error reporting
		MLogOpen\n -- begin logging commands sent to the IS-900
		MLogClose\n -- stop logging commands sent to the IS-900
		MLogClear\n -- clear the current buffer of commands sent to the IS-900
		MLogState\n -- the current logging state
		MLogSend\n -- send the logged commands back
		MInfoSystem\n -- get info about the system (undocumented, so not all values known)
		MSystemTest1\n -- do some tests and print some information about the hardware
		MSystemTest2\n -- reports communication and tracking quality and noise

	I also discovered that if the IS-900 somehow gets stuck, then the one
	(and only) command that I found that will unstick it is "MSystemTest1\n".


	TODO: Discuss station numbers (<n>) here ...

	The configuration for the tracking device is beyond the functionality
	of this code.  The configuration is typically done once, and often
	directly by InterSense (or their partners) when the product is delivered
	and installed.

	However, there are some systems that are more fixed, and do not require
	a theodolite, so could be configured by the local VR support staff.
	Perhaps we'll add a tool for them to do the configuration without
	using the Windows-only software provided by InterSense (ISDEMO32.exe).

	In fact, the commands to do the configuration are not that complicated.
	For example, the 3-strip setup called the "SoniFrame" can be configured
	with the following commands:
		-------
		MCC
		MCF1, 0.9144,-0.6096, 0.0000, 0.00, 0.00, 1.00,5001
		MCF2, 0.0000,-0.6096, 0.0000, 0.00, 0.00, 1.00,5002
		MCF3,-0.9144,-0.6096, 0.0000, 0.00, 0.00, 1.00,5003
		MCF4,-0.6096, 0.0000, 0.0000, 0.00, 0.00, 1.00,5004
		MCF5, 0.0000, 0.0000, 0.0000, 0.00, 0.00, 1.00,5005
		MCF6, 0.6096, 0.0000, 0.0000, 0.00, 0.00, 1.00,5006
		MCF7, 0.9144, 0.6096, 0.0000, 0.00, 0.00, 1.00,5007
		MCF8, 0.0000, 0.6096, 0.0000, 0.00, 0.00, 1.00,5008
		MCF9,-0.9144, 0.6096, 0.0000, 0.00, 0.00, 1.00,5009
		MCe
		MCF
		-------
	And then a ^K to write the data to non-volatile memory.


	Polhemus Patriot protocol:

		The Patriot device is somewhat similar to the original
		Fastrak, but there are some significant differences.

		First, when the Patriot is powered up, it sends the string
		"Patriot Ready!\n\n" [and it's possible it actually sends
		that on both the TX and RX lines, since the information seems
		to get out when a null-modem is in the serial cable, but
		then information doesn't seem to go back.

		Second, the Patriot does not follow the traditional scheme
		of using the leading character of the report to indicate
		the type of report that is coming.  Basically all reports
		(except errors) begin with a '0', whereas for the non-patriot
		fastrak devices, a '0' specifically meant we were getting a
		data record (i.e. from a particular unit), and '2' was the
		indicator of a status report (with '3' being used by InterSense
		for their specific types of reports).  But the Patriot uses
		'0' for both data reports and status reports.  Plus errors
		are just printed as an ASCII message with no leading byte
		to indicate the type.

		Third, the case of the commands seems to be ignored, so
		commands like "F" and "f" now take an argument to specify
		the format, rather than using the case of the command.
		Fortunately, the output formats (and "O" command) seem to
		work more or less the same (at least for ASCII, binary was
		not checked).

		Also, it seems that all commands (except "P") require a
		carriage return in order to execute them.

		So here are the key commands for the Patriot:

			P -- Poll for one record of tracking info (and if
				in continuous mode, switch to polled mode)
			F\n -- report output mode
			F0\n -- Put in ASCII output mode
			F1\n -- Put in Binary output mode
			C\n -- Place in Continuous output mode
			U\n -- report output data units
			U0\n -- set output data units to inches
			U1\n -- set output data units to centimeters
			^V{0,1,2,3}\n -- report version/serial information
				[0 is transmitter ID, 1 & 2 are station IDs,
				blank or 3 reports system version information]
				[At least for the Wireless, the "0" option doesn't
				really work -- it just repeats whatever the last
				valid ^V command was -- or garbage if no valid one
				has been done yet.]
			l{1,2}\n -- "launch" markers for receptor number 1 or 2
			^G{lr,ur}\n -- autolaunch with lr=launch range, and ur=unlaunch
			^U0\n -- report active marker map
			@A{0,1}\n -- autolaunch
			@S\n -- report signal strength of the receptors
			@R<receptor>[,mode]\n -- set/report close-range mode


		Here are some other possibly useful commands:
			^P{1,2,3,4} -- change the phase of the given marker [Wireless only]
			^T[0]\n -- report/clear startup errors [09/20/10 -- does not work on Wireless]
			^E{0,1}\n -- report/set echo mode (0 - off, 1 - on)
			^U...\n -- report/set station state(s)
				[similar to the Fastrak "l" command]
			^X...\n -- report/set internally stored configurations
			^K{...} -- set an internal configuration [wireless only??]
			^W{...} -- write the current internal configuration to be the startup [wireless only??]
			R -- Reset reference frame to default (or perhaps this
				is ^R -- there seems to be a discrepancy between
				the manual and the response from the unit)
			X...\n -- set location filter parameters
			Y...\n -- set attitude filter parameters
			@B...\n -- USB buffering mode -- didn't work as
				documented for me (perhaps because USB cable
				not connected?)
			^Y -- warm-reset: go through power-up initialization
				(takes a couple seconds)
			^Z{0,1,2,3} -- report configuration settings
				[blank is current, 0 is factory default
				settings, 1-3 are user defined config settings.]
			M\n -- show serial numbers of "installed markers"
			Nmarker[,string]\n -- report ID of a launched marker

		And here are some significant missing commands:
			S -- Request System Status and ID

		And as far as the output format (ie. "O") values go, there are
		some the same, and some different.  The ones that are the same
		are:
			0 - ASCII space
			1 - ASCII CR/LF
			2 - X,Y,Z Cartesian coordinate
			4 - Az,El,Rl Euler orientation angles
		which are the ones FreeVR typically uses.  The other value to
		note is:
			10 (patriot) -- stylus value
				[and currently (09/22/2006), I cannot find a way
				to determine which button on the Fledermaus
				controller is pressed -- all report "1"]
				[and after some inquiries, the current sentiment
				is that all the buttons are mapped to the same
				physical input, so there really is only one
				button to build the interface around.]
				[also, this does not exist for the wireless,
				though a value of 0 does get reported -- making
				it an undocumented feature I guess, but of
				course there are no buttons on the markers.]
		Also, there seems to be a consistency in that the maximum
		parameter value is 66 -- even though many of them are undefined.

	Polhemus Patriot Wireless protocol:

		As far as I know, there is no real difference in protocol
		between the Patriot Wireless and the wired Patriot, but I no
		longer have access to the wired model (09/17/2010), so if I find
		that the Wireless differs from the above, I'll report it here.

		Also, I'm not sure how RS-232 communications parameters need to
		set for the wired model, but here they are for the wireless:
			- baud: 115200
			- databits: 8
			- parity: none
			- stopbits: 1
			- flow control: none

		NOTE: that in kermit, flow control must be explicitly set to "none"
		(ie. it's not the default).  So to connect via kermit do:
			% kermit -b 115200 -l /dev/input/patriot  (or whatever)
			C-Kermit>set flow-control none
			C-Kermit>SET CARRIER-WATCH OFF
			C-Kermit>connect
		or more quickly:
			% kermit -l /dev/ttyS0 -b 115200 -C "set flow-control none,SET CARRIER-WATCH OFF,connect"

		So at least one difference is that when the Patriot Wireless
		starts up, it reports:
			Patriot Wireless Ready!

		rather than just "Patriot Ready!".

	Hardware/firmware issues with the Polhemus Patriot Wireless:
		- autolaunching only works with two receptors on the system
			(not mentioned in current manual)
		- without autolaunching and hemisphere commands, it's not clear
			how to consistently have markers operate with the proper
			orientation, etc. (ie. without significant user intervention)
		- The "@S\n" command is mis-documented in the manual
		- moving any one marker to be too close to the (a?) receptor
			causes all the markers to report 0.000 signal strength
			to (that?) receptor, thus no tracking can be performed.
		- after a period of time (that isn't particularly long), the
			firmware on the unit gets to a point where it requires
			an extra character in the buffer before it will
			interpret a command -- and then after more time, it
			seems to require a 2nd extra character.  The warm-reset
			seems to fix this, but then also requires doing a
			relaunch!


HISTORY:
	22 April 2001 (Bill Sherman)
		I copied vr_input.skeleton.c and made the initial changes to
		  convert code to that of a specific device.

	3 May 2001 (Bill Sherman)
		I don't have this device working yet, but I'm catching up all
		  the input devices to a few minor changes made to the general
		  vr_input.skeleton.c format, and this one is no exception.

	6 May 2001 (Bill Sherman) (mid-mod)
		I brought some of the concepts from the FOB code into this
		  file.  Primarily the concept of a data structure for each
		  unit of the overall device.
		I also began writing a general decoder routine for all incoming
		  messages.

	14 June 2001 (Bill Sherman) (mid-mod)
		Got basics (very) working.  I.e. it reads the system id, and
		  knows that it got an output list record or a data record.

	20 May 2002 (Bill Sherman)
		A few modifications to keep up with changes to the library such
		  as the renaming of input types (sensor6 to 6sensor, etc), and
		  some #define'd values (X to VR_X, Y to VR_Y, etc).

	11 September 2002 (Bill Sherman)
		Moved the control callback array into the _FastrakFunction()
		  callback.

	21-23 April 2003 (Bill Sherman)
		Updated to new input matching function code style.  Changed
		  "opaque" field to "aux_data".  Split _FastrakFunction() into
		  5 functions.  Added new vrPrintStyle argument to
		  _FastrakPrintStruct() for the sake of the new "PrintAux"
		  callback.

	3 June 2003 (Bill Sherman)
		Now include "vr_enums.h" for the TEST_APP code.
		Added the address of the auxiliary data to the printout.
		Added the "system_pause_toggle" control callback.
		Added comments classifying the controls.

	15 April 2004 (Bill Sherman, still at NCSA)
		After no real work since June of 2001 (apart from some basic
		  general library changes), I'm back to working on the IS-900
		  tracker input device.

	25 October 2004 (Bill Sherman, now at DRI)
		Again after a bout of no real work, I finally got around to
		  actually reading data in a useful fashion.  However, a lot
		  of stuff was hardcoded for a particular setup so I could
		  have some FreeVR apps running at the DRI demo days.  I
		  managed to get the test application to function minimally
		  (copying over the ASCII display routine from vr_input.asc_fob.c),
		  and began to work on the FreeVR version, but not enough of
		  the skeleton contained the necessary code, so more stuff
		  was copied from the Flock of Birds driver code.

	27 October 2004 (Bill Sherman, in Las Vegas with the FakeSpace ROVR)
		I worked in Las Vegas with the Fakespace ROVR on loan.  I
		  figured out how to parse the "O<n>" responses from the
		  IS-900, and got pretty close with the results, but not enough
		  time to getting really working before the demos.

	18-29 August 2005 (Bill Sherman) (mid-mod)
		Focused on getting the "fastraktest" program functioning
		  properly (and the line-rendering version at that).  In the
		  process added several debugging print statements, but have
		  been #if'd out now that things have begun working.
		Added the line-rendering output routine.

	2-8 September 2005 (Bill Sherman) (mid-mod)
		Discovered a decoding bug, and finally discovered and corrected
		  the fatal flaw.  (Search for the word "robust" for how to
		  recreate errors, so that the rest of the code can be made
		  more robust.)

	12-14 September 2005 (Bill Sherman) (mid-mod)
		I'm fairly happy with the fastraktest implementation, and now I
		  need to get the FreeVR interface completed.

	18-19 September 2005 (Bill Sherman)
		Implemented the FreeVR interface.

	21 October 2005 (Bill Sherman)
		Added a new time-plot of tracker quality compile-time option to
		  the fastraktest application.

	6 March 2006 (Bill Sherman)
		Fixed some type casts to make the code cleaner (and compile
		  without warnings on pickier architectures).

	22-28 September 2006 (Bill Sherman)
		Implemented the Polhemus Patriot protocol (which is somewhat
		  similar to the standard Fastrak protocol, but not really --
		  vaguely familiar is probably a better way to describe it.
		  In any event, however, in the end I decided to keep it
		  merged within this source file.

	25 February 2008 (Bill Sherman)
		Added the ability to specify tracker quality, measurements
		  and/or reject numbers as valuator inputs.  The intent is
		  to use this with the newly implemented feature that allows
		  input histories to be rendered on-screen.

	8 August 2008 (Bill Sherman)
		Added a new "__RenderValuesCSV()" function for the test program
		  that now allows me to run the test program and output the
		  values to a comma-separated-value (CSV) file that can be
		  plotted with gnuplot or something.

	24 September 2009 (Bill Sherman)
		Did some debugging and code changes to make the "fastraktest"
		  program more robust.

	16 October 2009 (Bill Sherman)
		A quick fix to the _FastrakParseArgs() routine to handle the
		  no-arguments case.

	17-20 September 2010 (Bill Sherman)
		Made adjustments required to allow the Polhemus Patriot Wireless
		  units to work with the driver.  I also fixed some other things
		  here and there that I discovered issues with.

	4 April 2011 (Bill Sherman, working w/ Dave Reagan)
		Got the code to the point where it at least will run with the
		  Patriot Wireless in a dual-receptor configuration.  There
		  are still some major issues that need to be resolved such
		  as that it only works on the second and later runs after a
		  cold-restart, and even then it's confused as to whether it
		  is communicating with a Wireless system.
		[corresponds to FreeVR Version 0.6b]

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
	- Fix major bugs in getting the Patriot Wireless working:
		- currently hard-coded to dual-receptor system (which really is
			the only reasonable system to use -- and it's still
			quite flakey).
		- first run after a cold-reset of the base unit hangs after
			auto-launch, waiting for "aux->got_state" to be set
			to 1 -- which should be happening, not sure what the
			problem is [04/04/11]
		- subsequent runs after a cold-reset of the base unit operate,
			but don't realize they are communicating with the
			Wireless model.  (fortunately, since the first run did
			an auto-launch, the markers are all reporting).
		- Determine what coordinate system it comes up in -- and how
			that might vary.

	- look into removing the call(s) to _FastrakDecodeRecord() from inside
		_FastrakGetInput() see [09/24/2009]

	- determine how the Polhemus Liberty protocol fits in with the rest of
		the Polhemus systems.

	- enhance the Patriot and Patriot Wireless protocol options and observances:
		- add an argument to set/unset close-range mode (wireless)
		- add a live-call function to report the station values (wireless)
		- add a live-call function to alter the Phase state (wireless)
		- DONE: poll the receptor status and include results as a tracking quality
		- when not all markers are reporting, need to show the correct
			ones in fastraktest
		- when not all markers are reporting, need to do the correct
			mapping in FreeVR library
		- make sure tracking quality valuator inputs work!

	- The rest of the implementation:
		- are there commands to get the device's Global status, Error
			status, and Unit Status?
		- go into a stream mode if specified
			NOTE: with current protocol, we can't know the tracking
			"letter" when in stream mode -- since that has to be
			polled separately.
		- go into a binary reporting mode if specified
		- separate out some of the IS-900 stuff
		- do some error checking
		- should parse for the error code (e.g. "2 E*ERROR*O:,40,2,4,1*ERROR* EC -1*PS1  *FL1 *ST0") -- in this case caused by the ':' character sent as a station number.
			[8/30/05: which would have been caused by a unit-10, and
			not checking to make sure the correct ASCII value was
			given to address the 10th (or 11th or 12th) receiver
			units.]

	- test whether "\n\r" is the proper terminator for Fastrak & IS-900 (the Patriot
		seems to prefer only "\r" to terminate the commands).  Thus,
		many of the commands particular to Polhemus devices (such as
		Hemisphere) have only been tested on the Wired-Patriot.

	- PARTIAL: test the delay values -- current values were determined for
		the FOB [I've reduced the values, and it still works]

	- search for the word "robust", and deal with related issues.

	- should be able to treat loss of tracking on any/all sensor(s) as a
		button press to the application (or used as a control to other
		parts of FreeVR to make a sound or whatever).  The IS-900
		front panel reports three tracker conditions: 'X', 'L' and 'T'.
		'T' means good quality tracking, and I think 'L' means low
		quality tracking, and I'm guessing that 'X' means no tracker
		(perhaps because there is no communication with a wireless
		device).  Also need to see if there is any other flags that
		indicate lack of wireless communication.
		[01/31/2008: InterSense has a new firmware that now has the
		option to receive the wireless communication value.]

		Another (perhaps duplicate) thing to do is have an n-sensor with
		the three values.  [02/25/2008: for now, this will be implemented
		as a valuator input instead -- because that requires less effort
		to implement, and I need to see these values to analyze the
		quality of the IS-900 on the DRI 4-sided CAVE.]

	- add to test program the ability to specify which units have buttons
		(ie. are wands).

	- PARTIAL: add to test program, an option to display the beacon
		information off to the side of the rendering (perhaps even a
		"graphical" display).

	- write a companion program for configuring an IS-900 device (ie.
		entering the beacon locations and sending to device.)

	- for "fastraktest" add envvars and command line arguments for which
		receiver has buttons/joystick, and other configuration options
		(eg. stream mode).

	- write an IS900 Ethernet protocol communications option (vs. the RS-232)

	- write a Patriot-Wireless USB protocol communications option (vs. RS-232)

	- get this code tested on an actual Polhemus Fastrak system

	- determine whether there is a way to query whether a particular
		receiver has buttons.


**************************************************************************/
#if !defined(FREEVR) && !defined(TEST_APP) && !defined(CAVE)
/* Choose how this code should be compiled (ie. for CAVE, standalone, etc). */
#  define	FREEVR
#  undef	TEST_APP
#  undef	CAVE
#endif

#undef COMM_DEBUG	/* define this to add a lot of output for debugging communications w/ the device */


#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <math.h>	/* for sqrt() */

#include "vr_serial.h"
#include "vr_debug.h"
#include "vr_utils.h"


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
#  include <stdlib.h>		/* needed for getenv() & atoi() */
#  include "vr_serial.c"
#  include "vr_utils.c"
#  include "vr_enums.h"
#  if defined(COMM_DEBUG) && defined(__linux)	/* This construct may not work on non-Linux compilers or at least non-gnu compilers */
#    define	vrDbgPrintfN(a,...)	printf
#  endif
#endif


/*** define other library stuff here if desired ***/


/*** local defines ***/

#define STATION_NUM2CHAR(s)	('0' + ((char)(s)) + (((char)(s) > (char)9) ? (char)7 : (char)0))
#define STATION_CHAR2NUM(s)	(int)((((char)(s)) - '0') - (((char)(s) > '9') ? (char)7 : (char)0))

#if 0
#define	DEFAULT_PORT	"/dev/input/fastrak"
#elif 1
#define	DEFAULT_PORT	"/dev/ttyS0"		/* standard for Linux motherboard ports */
#elif 0
#define	DEFAULT_PORT	"/dev/ttyIOC4/1"	/* for the SGI Altix */
#else
#define	DEFAULT_PORT	"/dev/ttyUSB0"		/* typical for Linux USB-RS-232 devices */
#endif
#define	DEFAULT_BAUD	115200

#define	BUFSIZE		1024


	/******************************************************************/
	/*** definitions for interfacing with a Fastrak protocol device ***/
	/***                                                            ***/

/* some of the timing delay values */
#if 1
#define FASTRAK_EXAM_DELAY		 10000		/* Warning: only tested on an IS-900 VETracker */
#define FASTRAK_SET_DELAY		  5000		/* Warning: only tested on an IS-900 VETracker */
#define PATRIOT_EXAM_DELAY		 100000
#define PATRIOT_SET_DELAY		  50000
#else
/* these are for testing Patriot via serialspy */
#define FASTRAK_EXAM_DELAY		 1000000
#define FASTRAK_SET_DELAY		  500000
#define PATRIOT_EXAM_DELAY		 1000000
#define PATRIOT_SET_DELAY		  500000
#endif

/* Fastrak button bit-masks & indices */
#define Fastrak_BUTTON_x1x	0x01
#define Fastrak_BUTTON_x2x	0x02
#define Fastrak_BUTTON_x3x	0x04
#define Fastrak_BUTTON_x4x	0x08
#define Fastrak_BUTTON_x5x	0x10
#define Fastrak_BUTTON_x6x	0x20
#define Fastrak_BUTTON_x7x	0x40

#define Fastrak_BUTTONINDEX_x1x	0x00
#define Fastrak_BUTTONINDEX_x2x	0x01
#define Fastrak_BUTTONINDEX_x3x	0x02
#define Fastrak_BUTTONINDEX_x4x	0x03
#define Fastrak_BUTTONINDEX_x5x	0x04
#define Fastrak_BUTTONINDEX_x6x	0x05
#define Fastrak_BUTTONINDEX_x7x	0x06

#define Fastrak_VALUATORINDEX_X	 0
#define Fastrak_VALUATORINDEX_Y	 1
#define Fastrak_VALUATORINDEX_TRACKINGQUALITY	7	/* obtained by a "40" entry in the output list */
#define Fastrak_VALUATORINDEX_TRACKINGMEASURE	8	/* obtained by an "MP" command */
#define Fastrak_VALUATORINDEX_TRACKINGREJECT 	9	/* obtained by an "MP" command */

/* some possible manufacturer values */
#define	MANU_POLHEMUS	1
#define	MANU_INTERSENSE	2

/* some possible models */
#define MODEL_FASTRAK	1
#define MODEL_PATRIOT	2
#define MODEL_PATRIOTWL	3
#define MODEL_LIBERTY	4
#define MODEL_IS900	5


/*******************************/
/* Device communications codes */
typedef	enum {
		FASTRAK_SYSSTAT,

		FASTRAK_ASCIIMODE,
		FASTRAK_BINARYMODE,

		FASTRAK_STREAMMODE,
		FASTRAK_POLLMODE,
		FASTRAK_POLL,

		FASTRAK_SET_INCHES,
		FASTRAK_SET_CENTIMETERS,

		FASTRAK_GET_ALLRECEIVERS,

		/* Some commands specific to the Polhemus Patriot */
		PATRIOT_WARM_RESET,
		PATRIOT_POLLMODE,
		PATRIOT_ASCIIMODE,
		PATRIOT_SET_INCHES,
		PATRIOT_SET_ECHO_MODE,
		PATRIOT_SYSSTAT,
		PATRIOT_GET_ALLRECEIVERS,
		PATRIOTWL_LAUNCH_RECEPTOR,
		PATRIOTWL_AUTOLAUNCH_RECEPTOR,
		PATRIOTWL_GET_SIGNAL_STATUS,

		/* More generic commands */
		FASTRAK_SET_HEMISPHERE_FORE,
		FASTRAK_SET_HEMISPHERE_AFT,
		FASTRAK_SET_HEMISPHERE_RIGHT,
		FASTRAK_SET_HEMISPHERE_LEFT,
		FASTRAK_SET_HEMISPHERE_LOWER,
		FASTRAK_SET_HEMISPHERE_UPPER,

		FASTRAK_SET_OUTPUT_ANGLES,
		FASTRAK_SET_OUTPUT_MATRIX,
		FASTRAK_SET_OUTPUT_POSITION,
		FASTRAK_SET_OUTPUT_POSITION_ANGLES,
		FASTRAK_SET_OUTPUT_POSITION_ANGLES_BJ,
		FASTRAK_SET_OUTPUT_POSITION_MATRIX,
		FASTRAK_SET_OUTPUT_POSITION_MATRIX_BJ,
		FASTRAK_SET_OUTPUT_POSITION_QUATERNION,
		FASTRAK_SET_OUTPUT_POSITION_QUATERNION_BJ,
		FASTRAK_SET_OUTPUT_QUATERNION,
		PATRIOT_SET_OUTPUT_POSITION_ANGLES,
		PATRIOT_SET_OUTPUT_POSITION_ANGLES_B,

		FASTRAK_GET_STATION_PARAMS,

		/* Some commands specific to the InterSense units */
		IS900_GET_TUNITS,
		IS900_GET_TRACKING_STATUS,
		IS900_SET_LIGHTS,

		FASTRAK_LASTCOMMAND		/* used to check bounds on array */
	} _FastrakCommand;


/**********************************************/
/* _FastrakComandInfo structure associates the command names with   */
/*    the Fastrak protocol command byte sequence, (incl. the number */
/*    of bytes, a post-command delay, and a string with the name.   */
typedef struct {
		char	name[128];
		int	len;
		int	us_delay;
		char	msg[32];
	} _FastrakCommandInfo;

	/* NOTE: these are not proper C-strings, in that '\0' is a valid character,   */
	/*   So some of the character arrays require the individual characters to be  */
	/*   specified as an array of chars, rather than a C-string -- in particular, */
	/*   strings that contain the NUL value.                                      */

	/* NOTE: (also) that this list is not comprehensive.  It contains all the     */
	/*   commands that are deemed (or were once deemed) necessary for normal      */
	/*   operation of the device.                                                 */
static	_FastrakCommandInfo	_FastrakMsgList[] = {
		{ "FASTRAK_SYSSTAT",		1,	FASTRAK_EXAM_DELAY,	"S" },	/* pg 93 */

		{ "FASTRAK_ASCIIMODE",		1,	FASTRAK_SET_DELAY,	"F" },	/* pg 94 */
		{ "FASTRAK_BINARYMODE",		1,	FASTRAK_SET_DELAY,	"f" },	/* pg 94 */
		{ "FASTRAK_STREAMMODE",		1,	FASTRAK_SET_DELAY,	"C" },	/* pg 90 */
		{ "FASTRAK_POLLMODE",		1,	FASTRAK_SET_DELAY,	"c" },	/* pg 90 */
		{ "FASTRAK_POLL",		1,	FASTRAK_SET_DELAY,	"P" },	/* pg 90 */

		{ "FASTRAK_SET_INCHES",		1,	FASTRAK_SET_DELAY,	"U" },	/* pg 93 */
		{ "FASTRAK_SET_CENTIMETERS",	1,	FASTRAK_SET_DELAY,	"u" },	/* pg 93 */

		{ "FASTRAK_GET_ALLRECEIVERS",	3,	FASTRAK_EXAM_DELAY,	"l*\n"}, /* pg ? */

		{ "PATRIOT_WARM_RESET",		1,	PATRIOT_SET_DELAY,	"\031\r" },	/* ^Y */
		{ "PATRIOT_POLLMODE",		1,	PATRIOT_SET_DELAY,	"p" },	/* either 'P' or 'p' works, I do this for debugging while monitoring the stream */
		{ "PATRIOT_ASCIIMODE",		3,	PATRIOT_SET_DELAY,	"F0\r" },
		{ "PATRIOT_SET_INCHES",		3,	PATRIOT_SET_DELAY,	"U0\r" },
		{ "PATRIOT_SET_ECHO_MODE",	3,	PATRIOT_SET_DELAY,	"\0050\r" },	/* ^E0 -- set to ^E1 to turn echo on */
		{ "PATRIOT_SYSSTAT",		2,	PATRIOT_EXAM_DELAY,	"\026\r" },	/* ^V */
		{ "PATRIOT_GET_ALLRECEIVERS",	3,	PATRIOT_EXAM_DELAY,	"\0251\r" },	/* ^U1 */
		{ "PATRIOTWL_LAUNCH_RECEPTOR",	3,	PATRIOT_SET_DELAY,	"l1\r" },
		{ "PATRIOTWL_AUTOLAUNCH_RECEPTOR",4,	PATRIOT_SET_DELAY,	"@A1\r" },
		{ "PATRIOTWL_GET_SIGNAL_STATUS",3,	PATRIOT_EXAM_DELAY,	"@S\r" },

	/* TODO: the hemisphere parameters have not been tested, also station number needs to be set in the string. */
		/* NOTE: hemisphere commands only tested on Patriot -- uses just "\r" as terminator rather than "\n\r" */
		{ "FASTRAK_SET_HEMISPHERE_FORE",   9,	FASTRAK_SET_DELAY,	"H1,1,0,0\r" },
		{ "FASTRAK_SET_HEMISPHERE_AFT",   10,	FASTRAK_SET_DELAY,	"H1,-1,0,0\r" },
		{ "FASTRAK_SET_HEMISPHERE_RIGHT" , 9,	FASTRAK_SET_DELAY,	"H1,0,1,0\r" }, /* pg 100 */
		{ "FASTRAK_SET_HEMISPHERE_LEFT",  10,	FASTRAK_SET_DELAY,	"H1,0,-1,0\r" },
		{ "FASTRAK_SET_HEMISPHERE_LOWER",  9,	FASTRAK_SET_DELAY,	"H1,0,0,1\r" },
		{ "FASTRAK_SET_HEMISPHERE_UPPER", 10,	FASTRAK_SET_DELAY,	"H1,0,0,-1\r" },

		/* NOTE: the "40" parameter in the following output option commands are actually IS900 specific */

		{ "FASTRAK_SET_OUTPUT_ANGLES",	11,	FASTRAK_SET_DELAY,	"O1,40,4,1\n\r" },	/* pg 95-99 */
		{ "FASTRAK_SET_OUTPUT_MATRIX",	15,	FASTRAK_SET_DELAY,	"O1,40,5,6,7,1\n\r" },
		{ "FASTRAK_SET_OUTPUT_POSITION",11,	FASTRAK_SET_DELAY,	"O1,40,2,1\n\r" },
		{ "FASTRAK_SET_OUTPUT_POSITION_ANGLES",13,FASTRAK_SET_DELAY,	"O1,40,2,4,1\n\r" },
		{ "FASTRAK_SET_OUTPUT_POSITION_ANGLES_BJ",19,FASTRAK_SET_DELAY,	"O1,40,2,4,22,23,1\n\r" },
		{ "FASTRAK_SET_OUTPUT_POSITION_MATRIX",17,FASTRAK_SET_DELAY,	"O1,40,2,5,6,7,1\n\r" },
		{ "FASTRAK_SET_OUTPUT_POSITION_MATRIX_BJ",23,FASTRAK_SET_DELAY,	"O1,40,2,5,6,7,22,23,1\n\r" },
		{ "FASTRAK_SET_OUTPUT_POSITION_QUATERNION",14,FASTRAK_SET_DELAY,"O1,40,2,11,1\n\r" },
		{ "FASTRAK_SET_OUTPUT_POSITION_QUATERNION_BJ",20,FASTRAK_SET_DELAY,"O1,40,2,11,22,23,1\n\r" },
		{ "FASTRAK_SET_OUTPUT_QUATERNION",12,	FASTRAK_SET_DELAY,	"O1,40,11,1\n\r" },
		{ "PATRIOT_SET_OUTPUT_POSITION_ANGLES",9,PATRIOT_SET_DELAY,	"O1,2,4,1\r" },
		{ "PATRIOT_SET_OUTPUT_POSITION_ANGLES_B",12,PATRIOT_SET_DELAY,	"O1,2,4,10,1\r" },

		{ "FASTRAK_GET_STATION_PARAMS", 3,	FASTRAK_SET_DELAY,	"O1\r" },

		/********************************/
		/* InterSense specific commands */
		{ "IS900_GET_TUNITS",		4,	FASTRAK_EXAM_DELAY,	"MCF\n" },
		{ "IS900_GET_TRACKING_STATUS",	3,	FASTRAK_EXAM_DELAY,	"MP\n" },
		{ "IS900_SET_LIGHTS",		4,	FASTRAK_SET_DELAY,	"ML0\n" }, /* pg 103 */

		{ "FASTRAK_LASTCOMMAND",	0,	0,			"" }
	};



/******************************************************************/
/*** auxillary structures of the current data from the Fastrak. ***/

	/* _FastrakUnit: contains data for a single unit of the Fastrak system */
typedef struct {
		unsigned char	state;		/* state byte of the sensor '0'=off, '1' = on */
		unsigned char	status[3];	/* from InterSense status ("Ms/n") */
		char		toreport[128];	/* what the system reports it will report ("O<n>/n") */
		int		toreport_len;	/* number of bytes expected for each report */
		int		toreport_type[16];/* an array of what values to expect */
		int		toreport_num;	/* the number of items in the toreport_type array */
		int		read_count;	/* the number of times this device has reported values */
		float		time_stamp;	/* incoming time data from sensor [TODO: use this] */
		float		data_pos[32];	/* incoming position data of the sensor */
		int		buttons;	/* incoming button data of the sensor */
		int		joystick[2];	/* incoming joystick data of the sensor */
		int		has_button;	/* whether button/joystick is attached to this unit */
		unsigned char	tracking_quality;/* incoming tracking quality of the sensor */
		char		tracking_status;/* incoming tracking status of the sensor (from IS900 "MP" command) */
		int		tracking_meas;	/* number of range measurements received this cycle ("MP") */
		int		tracking_reject;/* number of range measurements rejected this cycle ("MP") */

		/* other possibilities: */
		/*	- alignment reference frame: 9 floats */
		/*	- boresight reference angles: 3 floats */
		/*	- hemisphere params: 3 floats */
		/*	- tip offset values: 3 floats */
		/*	- location operational envelop: 6 floats */
		/*	- prediction interval: 1 integer */
		/*	- sensitivity level: 1 integer (1-5) */

	} _FastrakUnit;

	/* _FastrakTransUnit: contains data for a single transmitter unit */
	/*   (e.g. on the IS-900 system each beacon is a separate unit)   */
typedef struct {
		int		assigned;	/* boolean flag to indicate the existence of this unit */
		float		position[6];	/* location and directional normal */
		int		pse_id;		/* position-sensing-element id number (usually 1..n) */
		int		hardware_id;	/* the hardware-encoded ID for the element (soniDisc) */
	} _FastrakTransUnit;

typedef struct {
		/* these are for interfacing with the hardware */
		int		fd;		/* was commhandle */
		char		*port;		/* name of serial port */
		int		baud_enum;	/* communication rate as an enumerated value */
		int		baud_int;	/* communication rate as the real value */
		int		open;		/* flag with Fastrak successfully open */

		/* these are for internal data parsing */
		unsigned char	buf[BUFSIZE];	/* buffer of incoming data to be parsed */
		int		eobuf_pos;	/* end-of-buffer position (generally the number of bytes in the buffer) */
		char		version[256];	/* self-reported version of the device (constructed from other params) */
		char		op_params[256];	/* operating parameters of the device (according to it) */
		int		manufacturer;	/* code for manufacturer specific operations */
		int		model;		/* code for specific model */

		/* general information obtained from the system */
		int		got_state;	/* flag to indicate that we've received the receiver states */
		unsigned char	status[3];	/* InterSense status ("MS/n") */
		char		*init_command;	/* string to send to tracker during initialization */
		int		stream_mode;	/* whether stream mode should be used to get data */
		int		hemisphere;	/* code of which hemisphere to use */

		char		firmware_version[16];	/* version of system software */
		char		system_id[33];	/* system id code (eg. "IS900") */
		int		reported_format;/* data will be sent in ASCII or Binary (as reported by system) */
		int		reported_units;	/* location data will be sent in Inches or Centimeters (as reported by system) */
		int		reported_mode;	/* data will be streamed or polled (as reported by system) */
		int		update_rate;	/* Hz of reported "Update rate per station" [IS900 pg 118] */
		char		genlock_id;	/* state of Genlock operation ('G', 'X' or ' ') */

		/* other possibilities: */
		/*	- genlock synch: 3 integers (?) */
		/*	- ultrasonic timeout: 1 integer */
		/*	- ultrasonic sensitivity: 1 integer */
		/*	- fixed PSE: ... */

		/* information about the current values */
#define MAX_UNITS 12
		int		num_units;	/* number of units that return status > 0 */
		_FastrakUnit	units[MAX_UNITS];
#define MAX_TUNITS 100				/* For Polhemus usually 1, or IS-900 several more */
		int		num_tunits;	/* number of transmitter units (soniDisc beacons for InterSense trackers) */
		_FastrakTransUnit tunits[MAX_TUNITS];	/* transmitter units */

		int		timer;		/* incoming timer info  TODO: or is it a checksum?*/
		int		button_change;	/* boolean indicator if button values have changed*/
		int		valuator_change;/* boolean indicator of change in valuator values */
		int		receiver_change;/* boolean indicator of change in receiver values [09/22/10: not currently used -- just set] */

		/* information about how to filter the data for use with the VR library (FreeVR or CAVE) */
		float		scale_trans;	/* multiplier to scale from the Fastrak units (inches) */

		/* information about the inputs and mappings */
#if defined(FREEVR)
		int		num_buttons;	/* the number of buttons in the configuration */
		vr2switch	**button_inputs;/* An array of pointers to the input structures */
		int		*button_map_unit;/* An array of unit numbers from which the data should come */
		int		*button_map_button;/* An array of numbers indicating the particular button */
		int		*button_values;	/* An array containing the current value of each (used for debouncing) */

		int		num_valuators;	/* the number of valuators in the configuration */
		vrValuator	**valuator_inputs;/* An array of pointers to the input structures */
		int		*valuator_map_unit;/* An array of unit numbers from which the data should come */
		int		*valuator_map_valuator;/* An array of numbers indicating the particular valuator */
		float		*valuator_sign;/* An array of numbers indicating the particular valuator */
		int		*valuator_values;	/* An array containing the current value of each (used for debouncing) */

		int		num_6sensors;	/* the number of 6-dof input sensors in config */
		vr6sensor	**sensor6_inputs;/* An array of pointers to the input structures */
		int		*sensor6_map;	/* An array of unit numbers from which the data should come */
#endif

	} _FastrakPrivateInfo;



	/******************************************************/
	/*** General NON public Fastrak interface routines ***/
	/******************************************************/

/******************************************************/
/* Since this routine is generally called before we know what type of */
/*   device we're connected to, we'll just do a generic initialization.*/
/*   (At one point I considered having the initialization be affected */
/*   by the device type, but I think that's backwards  -- although,   */
/*   now that I think about it further, it is possible to get the type*/
/*   from the configuration information.  I guess we'll leave it this */
/*   way for now.                                                     */
static void _FastrakInitializeStruct(_FastrakPrivateInfo *aux)
{
	int	count;

	aux->got_state = 0;

	aux->version[0] = '\0';
	aux->op_params[0] = '\0';
	aux->manufacturer = 0;

	aux->buf[0] = '\0';
#ifndef COMM_DEBUG
	memset(aux->buf, '\0', BUFSIZE);
#else /* debugging stuff */
	memset(aux->buf, '*', BUFSIZE);
	aux->buf[BUFSIZE-1] = '\0';
#endif
	aux->eobuf_pos = 0;

	aux->timer = 0;
	aux->button_change = 0;
	aux->valuator_change = 0;
	aux->receiver_change = 0;

	/* TODO: set default stream_mode & filter mode here */
	aux->hemisphere = FASTRAK_SET_HEMISPHERE_LOWER;	/* use lower hemisphere by default */
	aux->scale_trans = 1.0/12.0;			/* convert from inches to feet by default */

	/* initialize individual units */
	for (count = 0; count < MAX_UNITS; count++) {
		aux->units[count].tracking_status = '-';
	}

	aux->num_units = 0;
	aux->num_tunits = 0;

	/* everything else is zero'd by default */

}


/******************************************************/
static void _FastrakPrintStruct(FILE *file, _FastrakPrivateInfo *aux, vrPrintStyle style)
{
	int	count;

	vrFprintf(file, "Fastrak protocol device internal structure (%#p):\n", aux);
	vrFprintf(file, "\r\tversion -- '%s'\n", aux->version);
	vrFprintf(file, "\r\toperating parameters -- '%s'\n", aux->op_params);
	vrFprintf(file, "\r\tfd = %d\n\tport = '%s'\n\tbaud = %d (%d)\n\topen = %d\n",
		aux->fd,
		aux->port,
		aux->baud_int, aux->baud_enum,
		aux->open);
	vrFprintf(file, "\r\tfirmware -- '%s'\n", aux->firmware_version);
	vrFprintf(file, "\r\tsystem id -- '%s'\n", aux->system_id);
	vrFprintf(file, "\r\tmanufacturer -- '%d'\n", aux->manufacturer);
	vrFprintf(file, "\r\treport format -- '%s' (%d)\n",
		(aux->reported_format == 0 ? "ASCII" : "Binary"),
		aux->reported_format);
	vrFprintf(file, "\r\treport units -- '%s' (%d)\n",
		(aux->reported_units == 0 ? "Inches" : "Centimeters"),
		aux->reported_units);
	vrFprintf(file, "\r\treport mode -- '%s' (%d)\n",
		(aux->reported_mode == 0 ? "Polled" : "Continuous"),
		aux->reported_mode);
	vrFprintf(file, "\r\tupdate rate -- %dHz\n", aux->update_rate);

	vrFprintf(file, "\r\tgot receiver states -- %d\n", aux->got_state);
	vrFprintf(file, "\r\tactive units -- %d\n", aux->num_units);
	vrFprintf(file, "\r\tactive transmitter units -- %d\n", aux->num_tunits);


#ifdef FREEVR /* { */
	vrFprintf(file, "\r\t%d buttons:\n", aux->num_buttons);
	vrFprintf(file, "\r\tbutton_values = %#p, button_map_unit = %#p, button_map_button = %#p, button_inputs = %#p\n",
		aux->button_values,
		aux->button_map_unit,
		aux->button_map_button,
		aux->button_inputs);
	for (count = 0; count < aux->num_buttons; count++) {
		vrFprintf(file, "\r\t\tbutton %d: value = %d, map_unit = %d, map_button = %d, input = %#p (type = %d)\n",
			count,
			aux->button_values[count],
			aux->button_map_unit[count],
			aux->button_map_button[count],
			aux->button_inputs[count],
			aux->button_inputs[count]->input_type);
	}

	vrFprintf(file, "\r\t%d valuators:\n", aux->num_valuators);
	vrFprintf(file, "\r\tvaluator_values = %#p, valuator_map_unit = %#p, valuator_map_valuator = %#p, valuator_inputs = %#p\n",
		aux->valuator_values,
		aux->valuator_map_unit,
		aux->valuator_map_valuator,
		aux->valuator_inputs);
	for (count = 0; count < aux->num_valuators; count++) {
		vrFprintf(file, "\r\t\tvaluator %d: value = %d, map_unit = %d, map_valuator = %d, input = %#p (type = %d)\n",
			count,
			aux->valuator_values[count],
			aux->valuator_map_unit[count],
			aux->valuator_map_valuator[count],
			aux->valuator_inputs[count],
			aux->valuator_inputs[count]->input_type);
	}

	vrFprintf(file, "\r\t%d 6-sensors:\n", aux->num_6sensors);
	vrFprintf(file, "\r\t6sensor_values = %#p, 6sensor_map = %#p, sensor6_inputs = %#p\n",
		aux->sensor6_inputs,
		aux->sensor6_map,
		aux->sensor6_inputs);
	for (count = 0; count < aux->num_6sensors; count++) {
		vrFprintf(file, "\r\t\t6-sensor %d: value = [%.2f %.2f %.2f  %.2f %.2f %.2f], map_unit = %d, input = %#p (type = %d)\n",
			count,
			aux->sensor6_inputs[count]->position->v[VR_X],
			aux->sensor6_inputs[count]->position->v[VR_Y],
			aux->sensor6_inputs[count]->position->v[VR_Z],
			aux->sensor6_inputs[count]->position->v[VR_AZIM+3],
			aux->sensor6_inputs[count]->position->v[VR_ELEV+3],
			aux->sensor6_inputs[count]->position->v[VR_ROLL+3],
			aux->sensor6_map[count],
			aux->sensor6_inputs[count],
			aux->sensor6_inputs[count]->input_type);
	}

#  if 0 /* TODO: not sure if we want/need this */
	vrFprintf(file, "\r\tcontrol inputs:\n");
	for (count = 0; count < MAX_CONTROLS; count++)
		vrFprintf(file, "\r\t\tcontrol_inputs[%d] = %#p\n", count, aux->control_inputs[count]);
#  endif
#endif /* } FREEVR */

	vrFprintf(file, "\r\tscale_trans = %f\n", aux->scale_trans);
	vrFprintf(file, "\r\themisphere = %d\n", aux->hemisphere);
	/* TODO: print some info about other current values */

	/* print some info about each receiver unit */
	vrFprintf(file, "\r\t%d position receivers:\n", aux->num_units);
	for (count = 0; count < aux->num_units; count++) {
		vrFprintf(file, "\t\treceiver unit %d: state = %d, toreport (%d bytes) = '%s'\n", count+1, aux->units[count].state, aux->units[count].toreport_len, aux->units[count].toreport);
	}

	/* print some info about each transmitter unit */
	vrFprintf(file, "\r\t%d position transmitters:\n", aux->num_tunits);
	for (count = 0; count < aux->num_tunits; count++) {
		vrFprintf(file, "\t\ttransmitter unit %d: pse_id = %d, loc = %7.4f,%7.4f,%7.4f, norm = %.2f,%.2f,%.2f, hw_id = %d\n", \
			count+1,
			aux->tunits[count].pse_id,
			aux->tunits[count].position[VR_X],
			aux->tunits[count].position[VR_Y],
			aux->tunits[count].position[VR_Z],
			aux->tunits[count].position[VR_X+3],
			aux->tunits[count].position[VR_Y+3],
			aux->tunits[count].position[VR_Z+3],
			aux->tunits[count].hardware_id);
	}
}


/**************************************************************************/
static void _FastrakPrintHelp(FILE *file, _FastrakPrivateInfo *aux)
{
	int	count;		/* looping variable */

#if 0 || defined(TEST_APP)
	vrFprintf(file, BOLD_TEXT "Sorry, Fastrak - print_help control not yet implemented.\n" NORM_TEXT);
#else
	vrFprintf(file, BOLD_TEXT "Fastrak - inputs:" NORM_TEXT "\n");
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
static char _FastrakSendCommand(_FastrakPrivateInfo *aux, _FastrakCommand command)
{
	_FastrakCommandInfo	*command_info;
	char			return_code = '\0';

	/* if command doesn't exist, then return immediately */
	if (command >= FASTRAK_LASTCOMMAND) {
		vrPrintf(RED_TEXT "_FastrakSendCommand(): No command (%d), nothing sent to Fastrak protocol device\n" NORM_TEXT, (int)command);
		return (return_code);
	}

	command_info = &(_FastrakMsgList[command]);

#ifdef COMM_DEBUG
	vrPrintf(
#else /* ) */
	vrDbgPrintfN(FASTRK_DBGLVL + RARE_DBGLVL,
#endif
		"_FastrakSendCommand(): Sending command '%s'\n", command_info->name);

	vrSerialWrite(aux->fd, command_info->msg, command_info->len);
#if 1
	vrSerialDrain(aux->fd);
fsync(aux->fd);
#endif
	vrSleep(command_info->us_delay);

	/* TODO: should we check for error status after every command? */
	/*   Well, we certainly shouldn't do it after a stream command. */
	/*   Probably best to require specific code to do error checking. */

#if 0
vrPrintf("_FastrakSendCommand() complete: command %d ('%c') -- \"%s\"\n", (int) command, command_info->msg[0], command_info->msg);
#endif
	return (return_code);
}


/******************************************************/
static char _FastrakSendCommandAddr(_FastrakPrivateInfo *aux, _FastrakCommand command, int addr)
{
static	char			newMsg[32];
	_FastrakCommandInfo	*command_info;
	char			return_code = '\0';

	/* if command doesn't exist, then return immediately */
	if (command >= FASTRAK_LASTCOMMAND) {
		vrPrintf(RED_TEXT "_FastrakSendCommandAddr(): No command (%d), nothing sent to Fastrak protocol device\n" NORM_TEXT, (int)command);
		return (return_code);
	}

	command_info = &(_FastrakMsgList[command]);

	if (addr < 1 || addr > 12) {
		vrPrintf(RED_TEXT "_FastrakSendCommandAddr(): Address out of range for command '%s' (%d), nothing sent to Fastrak protocol device\n" NORM_TEXT, command_info->msg, (int)command);
		return (return_code);
	}

#ifdef COMM_DEBUG
	vrPrintf(
#else /* ) */
	vrDbgPrintfN(FASTRK_DBGLVL + RARE_DBGLVL,
#endif
		"_FastrakSendCommandAddr(): Sending command '%s' to unit #%d\n", command_info->name, addr);

	memcpy(newMsg, command_info->msg, command_info->len);

	/* place the address into the command string */
	newMsg[1] = STATION_NUM2CHAR((unsigned char)addr);

	/* put a terminating character at the end of the string */
	/* NOTE: this is only necessary for making printouts nice for debugging */
	newMsg[command_info->len] = '\0';

	vrSerialWrite(aux->fd, newMsg, command_info->len);
#if 1
	vrSerialDrain(aux->fd);
fsync(aux->fd);
#endif
	vrSleep(command_info->us_delay);

	/* TODO: should we check for error status after every command? */
	/*   Well, we certainly shouldn't do it after a stream command. */
	/*   Probably best to require specific code to do error checking. */

#if 0
vrPrintf("_FastrakSendCommandAddr() complete: command %d to addr %d ('%c','%c') -- \"%s\"\n", (int) command, addr, newMsg[0], newMsg[1], newMsg);
#endif
	return (return_code);
}


/******************************************************/
static char _FastrakSendCommandString(_FastrakPrivateInfo *aux, char *command)
{
	char			return_code = '\0';

	vrDbgPrintfN(
#ifdef COMM_DEBUG
		1,
#else
		FASTRK_DBGLVL + RARE_DBGLVL,
#endif
		"FASTRAK: Sending command '%s'\n", command);

	vrSerialWrite(aux->fd, command, strlen(command));
#if 1
	vrSerialDrain(aux->fd);
fsync(aux->fd);
#endif

	/* TODO: should we check for error status after every command? */
	/*   Well, we certainly shouldn't do it after a stream command. */
	/*   Probably best to require specific code to do error checking. */

	return (return_code);
}


/***********************************************************************/
/* NOTE: num_chars is only used for ASCII decoding */
float _FastrakReadFloat(unsigned char *buf, int num_chars, int binary)
{
static	char	local[10] = "\0\0\0\0\0\0\0\0\0\0";
	float	return_value;

	if (binary) {
		/* TODO: not implemented yet */
		printf("Warning, binary parsing not implemented yet\n");
		return 0.0;
	} else {
		if (num_chars > 10) {
			vrPrintf("_FastrakReadFloat(): Warning, too many characters: %d\n", num_chars);
			return 0.0;
		}

		memcpy(local, buf, num_chars);
		return_value = atof(local);
	}

#if 0
	vrPrintf("_FastrakReadFloat(): returning atof(%s)\n", local);
#endif
	return return_value;
}


/***********************************************************************/
/* NOTE: num_chars is only used for ASCII decoding */
int _FastrakReadIntN(unsigned char *buf, int num_chars, int binary)
{
static	char	local[10] = "\0\0\0\0\0\0\0\0\0\0";

	if (binary) {
		/* TODO: not implemented yet */
		printf("_FastrakReadIntN(): Warning, binary parsing not implemented yet\n");
		return 0;
	} else {
		if (num_chars > 10) {
			vrPrintf("_FastrakReadIntN(): Warning, too many characters: %d\n", num_chars);
			return 0;
		}

		memcpy(local, buf, num_chars);
		local[num_chars] = '\0';

		return vrAtoI(local);
	}
}


/******************************************************/
/* _FastrakDecodeRecord() ...                         */
/*                                                    */
/* Overview of records (based on IS-900 manual):                 */
/* 09/24/2009 -- page numbers are out of sync with my new manual, not sure what was used for these values. */
/*    (2nd byte always represents station number, and            */
/*       when reporting a system-wide value is '1').             */
/*   .	0.  -- incoming sensor data [pg. 109]                    */
/*   .	21S -- System Status and Version info [pg. 110]          */
/*   .	2.O -- Output List Record [pg. 111]                      */
/*   .	2.l -- Station State Record (on/off) [pg. 111]           */
/*	2.A -- Alignment Reference Record (NIY) [pg. 112]        */
/*	2.G -- Boresight Reference Angles Record (NIY) [pg. 112] */
/*	2.H -- Hemisphere Record (NIY) [pg. 113]                 */
/*	2.N -- Tip Offset Record (NIY) [pg. 113]                 */
/*	2.V -- Location Operational Envelope (NIY) [pg. 114]     */
/*      2 E -- An error message (NOTE: 2nd value is a space)     */
/*   .	31S -- Manufacturer System Status Record [pg. 114]       */
/*   .	3.s -- Manufacturer Station Record [pg. 115]             */
/*	3.p -- Prediction Interval Record (NIY) [pg. 116]        */
/*	3.Q -- Sensitivity Level Record (NIY) [pg. 116]          */
/*	31G -- Genlock Synchronization Record (NIY) [pg. 116]    */
/*	31U -- Ultrasonic Timeout Record (NIY) [pg. 117]         */
/*	31g -- Ultrasonic Sensitivity Record (NIY) [pg. 117]     */
/*	31F -- Fixed PSE Record (NIY) [pg. 117]                  */
/*   .	31P -- Tracking Status Record [pg. 118]                  */
/*      51T -- System Test Data record (NIY)                     */
/*                                                               */
/* Here are some records that might come from a Patriot device:  */
/*   [NOTE: I've mostly limited these to ones that are possible  */
/*   given the commands that have been implemented.]             */
/*    	0.u -- Units that are enabled (only up to 2 on a Patriot)*/
/*      0.v -- Version information (00v can be 1 of 2 things)    */
/*      0.O -- Output List Record (like Fastrak 2.O)             */
/*      0.F -- Output Format (ASCII or Binary)                   */
/*      0.U -- Output Units (Inches or Centimeters)              */
/*                                                               */
/* Here are some records that might come from a Patriot Wireless:*/
/*   [NOTE: The Patriot Wireless in Echo-On mode echoes all the  */
/*   commands back to the computer, so that's really what most   */
/*   of these parsing segments are dealing with.  I eventually   */
/*   decided to turn off Echo Mode right at the beginning -- of  */
/*   course it still echoes the fact that Echo Mode has been     */
/*   disabled.]                                                  */
/*   [NOTE: as with the new general Patriot commands, I'm only   */
/*   handling the responses that are possible given the commands */
/*   that are sent to the unit.                                  */
/*    	F. -- Format command echo                                */
/*    	U. -- Units command echo                                 */
/*    	@A. -- Auto-launch command echo                          */
/*	Echo Off -- Disabling echo command echo                  */
/*	Invalid ...! ***** -- Error messages from the Patriot    */
/*****************************************************************/
static int _FastrakDecodeRecord(_FastrakPrivateInfo *aux)
{
	int		station_num;		/* number of the station of incoming report */
	int		shift_bytes = 0;	/* number of bytes to shift after the decoding */

#ifdef COMM_DEBUG
vrPrintf("_FastrakDecodeRecord(): Top of function\n");
#endif
/* NOTE: the buffer is stored in aux and the data in the buffer   */
/*   is shifted after decoded, so the next decode cycle can work  */
/*   from what is now in at the beginning of the buffer.  This    */
/*   function returns a zero when there isn't a complete record   */
/*   to parse, and one when something was parsed, and there *may* */
/*   be more to parse.                                            */

	/* if there aren't enough bytes for proper decoding, then return, to come back later */
	/* NOTE: some command-responses use the second byte to determine the type of information */
	/*   that is being returned, and some use the third.  For now we will require that there */
	/*   be at least three bytes in the buffer before we do any parsing, but if there are    */
	/*   commands that will only return two bytes and no more, then there is a small chance  */
	/*   that this could cause a problem.  But we'll go with it for now.                     */
	if (aux->eobuf_pos < 3) {	/* for robustness testing, change to "< 1" vs. "< 3" */
#ifdef COMM_DEBUG
static	int	warning_count = 0;
		vrPrintf("_FastrakDecodeRecord(): Skipping decoding of buffer with %d bytes (waiting for at least 3 bytes)\n", aux->eobuf_pos);
		warning_count++;
		if (warning_count > 6) {
			vrPrintf("_FastrakDecodeRecord(): aborting after %d warnings regarding insufficient data in the buffer\n", 6);
			abort();
		}
#endif
		return 0;
	}

#if defined(COMM_DEBUG) || 0
vrPrintf("_FastrakDecodeRecord(): Decoding buffer with %d bytes: '%s'\n", aux->eobuf_pos, aux->buf);
#endif

	switch(aux->buf[0]) {

	/***************/
	/* Data Record */
	case '0': {
		int		count;
		int		report_pos;	/* report position */
		int		report_len;	/* length of data report */
		_FastrakUnit	in_unit;	/* place into which to decode incoming data */
		_FastrakUnit	*unit;		/* pointer to actual unit data memory */

		station_num = STATION_CHAR2NUM(aux->buf[1]);
		if (((station_num < 1) || (station_num > 12)) && !((aux->model == MODEL_PATRIOT) || aux->model == MODEL_PATRIOTWL))  {
			vrPrintf(RED_TEXT "_FastrakDecodeRecord(): Warning: bad station number: %d\n" NORM_TEXT, station_num);
			vrPrintf("_FastrakDecodeRecord(): Data (eobuf_pos = %d): '%s'\n", aux->eobuf_pos, aux->buf);

			/* mark the next record as bad, and therefore skip to newline */
			aux->buf[0] = '_';
			return 0;
		}

		/*******************************************************************/
		/* NOTE: the Polhemus Patriot also uses the "0" report for items   */
		/*   other than the data record.  So need to check for those here. */
		if ((aux->model == MODEL_PATRIOT || aux->model == MODEL_PATRIOTWL)) {
#if defined(COMM_DEBUG) || 0
			vrPrintf(RED_TEXT "_FastrakDecodeRecord()-PATRIOT: First checking whether '%c' is a valid patriot response.\n" NORM_TEXT, aux->buf[2]);
#endif
			switch (aux->buf[2]) {

			/* A space indicates we've got a normal data record, so fall through */
			case ' ':
				break;

			/* reporting the Format: ASCII = 0, Binary = 1 (actually, if it's in */
			/*   Binary mode, we get Binary feedback -- which makes it difficult */
			/*   to easily parse).                                               */
			case 'F':
vrPrintf("Patriot: got an 'F' command\n");
				/* TODO: implement this */
				shift_bytes = 8;
				break;

			/* reporting the Units: Inches = 0, Centimeters = 1 */
			case 'U':
vrPrintf("Patriot: got a 'U' command\n");
				/* TODO: implement this */
				shift_bytes = 8;
				break;

			/* reporting the version information */
			/* WARNING: the return protocol for the Patriot has redundant codes   */
			/*   that report different information depending on the command given */
			/*   requesting the report.                                           */
			case 'v':
				/* TODO: implement this */

				/* this is temporary for at least properly skipping the correct number of bytes */

				if (!strncmp((char *)(&(aux->buf[24])), "Wireless", 8)) {
					vrDbgPrintfN(AALWAYS_DBGLVL, "_FastrakDecodeRecord(): This is a Wireless Patriot, so switching to wireless protocol\n");
					aux->model = MODEL_PATRIOTWL;
				}
				shift_bytes = (aux->model == MODEL_PATRIOT ? 128 : 137);
				vrPrintf(RED_TEXT "_FastrakDecodeRecord()-PATRIOT: Skipping Patriot version information '%s' (skipping %d bytes)\n" NORM_TEXT, &(aux->buf[7]), shift_bytes);
				break;

			case 'u': {
				int		count;
				int		max_units;	/* the "l*" command only returns 32 units */
#if 0 /* BS: 09/20/2010 -- a test to see if this is needed: I don't think that it is */
				_FastrakUnit	*unit;

				station_num = STATION_CHAR2NUM(aux->buf[1]);
				unit = &(aux->units[station_num-1]);
#endif

#if defined(COMM_DEBUG) || 1 /* TODO: set this to 0 when done debugging */
				vrPrintf("_FastrakDecodeRecord()-PATRIOT: Got a Station State Record for all stations '%s'\n", aux->buf);
#endif
				/* clamp number of units to 2 or 4 (4 is for the wireless Patriot) */
				max_units = MAX_UNITS;
				if (max_units > (aux->model == MODEL_PATRIOT ? 2 : 4))
					max_units = (aux->model == MODEL_PATRIOT ? 2 : 4);

				/* BS: 9/20/2010 -- was "count-5", but that doesn't match the bitwise numbering, so */
				/*   I changed it to 8-count -- the response will be something like: "01u  0001".   */
				for (count = 0; count < max_units; count++) {
					aux->units[count].state = (int)(aux->buf[(aux->model == MODEL_PATRIOT ? 6 : 8)-count] == '1');
#if defined(COMM_DEBUG) || 1 /* TODO: set this to 0 when done debugging */
vrPrintf("Patriot: unit %d state is %d ('%c')\n", count, aux->units[count].state, aux->buf[(aux->model == MODEL_PATRIOT ? 6 : 8)-count]);
#endif
				}
				shift_bytes = (aux->model == MODEL_PATRIOT ? 9 : 11);	/* 9 bytes for wired, 11 bytes for wireless */

				aux->got_state = 1;	/* flag that we've got the receiver states */
vrPrintf("Patriot: done checking unit states\n");

				} break;

			case 'O': {
				int		num_types = 0;
				int		record_pos;		/* position within the record being decoded */
				int		record_length = 4;	/* 3 bytes for the station number */
				_FastrakUnit	*unit;

				station_num = STATION_CHAR2NUM(aux->buf[1]);
				if ((station_num < 1) || (station_num > 4)) {	/* was "2" for the wired version, but the wireless can do up to "4" */
					vrPrintf(RED_TEXT "_FastrakDecodeRecord()-PATRIOT: Warning: bad station number: %d\n" NORM_TEXT, station_num);
					vrPrintf("_FastrakDecodeRecord()-PATRIOT: Data (eobuf_pos = %d): '%s'\n", aux->eobuf_pos, aux->buf);

					/* mark the next record as bad, and therefore skip to newline */
					aux->buf[0] = '_';
					return 0;
				}

				unit = &(aux->units[station_num-1]);

#if defined(COMM_DEBUG) || 0 /* TODO: set this to 0 when done debugging */
				vrPrintf("_FastrakDecodeRecord()-PATRIOT: Got Output List Record for Patriot station %d: '%s'\n", station_num, aux->buf);
#endif

				for (record_pos = 4; (aux->buf[record_pos+1] != '\r') && (record_pos < aux->eobuf_pos); record_pos += 3) {
					int	type;

#if defined(COMM_DEBUG) || 0 /* TODO: set this to 0 when done debugging */
vrPrintf("Evaluating for '%c' '%c' '%c'\n", aux->buf[record_pos], aux->buf[record_pos+1], aux->buf[record_pos+2]);
#endif
					type = _FastrakReadIntN(&aux->buf[record_pos], 3, 0);

					switch (type) {
					case  0: record_length += 1;	break;	/* ASCII space character */
					case  1: record_length += 2;	break;	/* CR/LF */
					/* TODO: binary values for below -- these work for ASCII only */
					case  2: record_length += 9*3;	break;	/* x,y,z location: 3 floats */
					case  3: record_length += 15*3;	break;	/* x,y,z location: 3 high-res floats */
					case  4: record_length += 9*3;	break;	/* yaw,pitch,roll angles: 3 floats */
					case  5: record_length += 15*3;	break;	/* yaw,pitch,roll angles: 3 high-res floats */
					case  6: record_length += 87;	break;	/* Direction Cosine 3x3 matrix */
					case  7: record_length += 9*4;	break;	/* Quaternion Orientation */
					case  8: record_length += 10;	break;	/* Timestamp */
					case  9: record_length += 10;	break;	/* Framecount */
					case 10: record_length += 3;	break;	/* stylus switch status (always 0) */
					default:
						vrPrintf("_FastrakDecodeRecord()-PATRIOT: Warning, unimplemented output record type: %d for station %d\n", type, station_num);
					}
vrPrintf("_FastrakDecodeRecord()-PATRIOT: output list for station %d: got type %d, record_length is now %d\n", station_num, type, record_length);
					unit->toreport_type[num_types] = type;
					num_types++;
				}
				unit->toreport_num = num_types;
				unit->toreport_len = record_length;
#if defined(COMM_DEBUG)
				vrPrintf("_FastrakDecodeRecord()-PATRIOT: Patriot unit %d: report_len set to %d\n", station_num, record_length);
#endif

				memcpy(unit->toreport, &aux->buf[3], record_pos-3);
				unit->toreport[record_pos-3] = '\0';	/* terminate the string */

				record_pos += 3;			/* skip the space/CR/LF */
				shift_bytes = record_pos;

				/* If the record doesn't end in an LF, then it was bad, so skip to the actual next NL */
				if (aux->buf[shift_bytes-1] != '\n') {
					aux->buf[0] = '_';		/* the underscore is a signal to skip-to-NL in the next decode cycle */
					shift_bytes = 0;
				}
				} break;	/* end of the 'O' case */

			case '@': {
				/* This is a safety test to make sure we have a 4th byte before decoding further, */
				/*   but it should be noted that we could do a lot more safety checks, both here  */
				/*   and elsewhere -- since we don't check for the full report.                   */
				if (aux->eobuf_pos < 4) {
					return 0;		/* return to get more data */
				}
				switch (aux->buf[3]) {

				/* Reporting the signal strength of the Patriot Wireless unit */
				/* Format is '00@S   %8f  %8f  %8f  %8f \n      %8f  %8f  %8f  %8f \n      %8f  %8f  %8f  %8f \n     \n' */
				/* for a total of 46 + 2 + 45 + 2 + 45 + 2 + 5 + 2 = 149 bytes                      */
				/* Some observations on signal strength:                                            */
				/*   - below 0.0001 basically means no signal at all                                */
				/*   - below 0.0050 seems to indicate a unit that is far away, or weakening battery */
				/*   - my guess is that there is some sort of log relationship                      */
				case 'S': {
					float	signals[4];
					if (aux->eobuf_pos < 149) {
						return 0;		/* return to get more data */
vrPrintf("only %d bytes so far\n", aux->eobuf_pos);
					}
#if 0 /* I'm saving this in case I need to see a lot of the buffer in hex again */
vrPrintf("_FastrakDecodeRecord(): '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', ...)\n", (unsigned int)aux->buf[0], (unsigned int)aux->buf[1], (unsigned int)aux->buf[2], (unsigned int)aux->buf[3], (unsigned int)aux->buf[4], (unsigned int)aux->buf[5], (unsigned int)aux->buf[6], (unsigned int)aux->buf[7], (unsigned int)aux->buf[8], (unsigned int)aux->buf[9]);
vrPrintf(": '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', ...)\n", (unsigned int)aux->buf[10], (unsigned int)aux->buf[11], (unsigned int)aux->buf[12], (unsigned int)aux->buf[13], (unsigned int)aux->buf[14], (unsigned int)aux->buf[15], (unsigned int)aux->buf[16], (unsigned int)aux->buf[17], (unsigned int)aux->buf[18], (unsigned int)aux->buf[19]);
vrPrintf(": '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', ...)\n", (unsigned int)aux->buf[20], (unsigned int)aux->buf[21], (unsigned int)aux->buf[22], (unsigned int)aux->buf[23], (unsigned int)aux->buf[24], (unsigned int)aux->buf[25], (unsigned int)aux->buf[26], (unsigned int)aux->buf[27], (unsigned int)aux->buf[28], (unsigned int)aux->buf[29]);
vrPrintf(": '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', ...)\n", (unsigned int)aux->buf[30], (unsigned int)aux->buf[31], (unsigned int)aux->buf[32], (unsigned int)aux->buf[33], (unsigned int)aux->buf[34], (unsigned int)aux->buf[35], (unsigned int)aux->buf[36], (unsigned int)aux->buf[37], (unsigned int)aux->buf[38], (unsigned int)aux->buf[39]);
vrPrintf(": '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', ...)\n", (unsigned int)aux->buf[40], (unsigned int)aux->buf[41], (unsigned int)aux->buf[42], (unsigned int)aux->buf[43], (unsigned int)aux->buf[44], (unsigned int)aux->buf[45], (unsigned int)aux->buf[46], (unsigned int)aux->buf[47], (unsigned int)aux->buf[48], (unsigned int)aux->buf[49]);
vrPrintf(": '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', ...)\n", (unsigned int)aux->buf[50], (unsigned int)aux->buf[51], (unsigned int)aux->buf[52], (unsigned int)aux->buf[53], (unsigned int)aux->buf[54], (unsigned int)aux->buf[55], (unsigned int)aux->buf[56], (unsigned int)aux->buf[57], (unsigned int)aux->buf[58], (unsigned int)aux->buf[59]);
vrPrintf(": '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', ...)\n", (unsigned int)aux->buf[60], (unsigned int)aux->buf[61], (unsigned int)aux->buf[62], (unsigned int)aux->buf[63], (unsigned int)aux->buf[64], (unsigned int)aux->buf[65], (unsigned int)aux->buf[66], (unsigned int)aux->buf[67], (unsigned int)aux->buf[68], (unsigned int)aux->buf[69]);
vrPrintf(": '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', ...)\n", (unsigned int)aux->buf[70], (unsigned int)aux->buf[71], (unsigned int)aux->buf[72], (unsigned int)aux->buf[73], (unsigned int)aux->buf[74], (unsigned int)aux->buf[75], (unsigned int)aux->buf[76], (unsigned int)aux->buf[77], (unsigned int)aux->buf[78], (unsigned int)aux->buf[79]);
vrPrintf(": '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', ...)\n", (unsigned int)aux->buf[80], (unsigned int)aux->buf[81], (unsigned int)aux->buf[82], (unsigned int)aux->buf[83], (unsigned int)aux->buf[84], (unsigned int)aux->buf[85], (unsigned int)aux->buf[86], (unsigned int)aux->buf[87], (unsigned int)aux->buf[88], (unsigned int)aux->buf[89]);
vrPrintf(": '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', ...)\n", (unsigned int)aux->buf[90], (unsigned int)aux->buf[91], (unsigned int)aux->buf[92], (unsigned int)aux->buf[93], (unsigned int)aux->buf[94], (unsigned int)aux->buf[95], (unsigned int)aux->buf[96], (unsigned int)aux->buf[97], (unsigned int)aux->buf[98], (unsigned int)aux->buf[99]);
vrPrintf(": '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', ...)\n", (unsigned int)aux->buf[100], (unsigned int)aux->buf[101], (unsigned int)aux->buf[102], (unsigned int)aux->buf[103], (unsigned int)aux->buf[104], (unsigned int)aux->buf[105], (unsigned int)aux->buf[106], (unsigned int)aux->buf[107], (unsigned int)aux->buf[108], (unsigned int)aux->buf[109]);
vrPrintf(": '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', ...)\n", (unsigned int)aux->buf[110], (unsigned int)aux->buf[111], (unsigned int)aux->buf[112], (unsigned int)aux->buf[113], (unsigned int)aux->buf[114], (unsigned int)aux->buf[115], (unsigned int)aux->buf[116], (unsigned int)aux->buf[117], (unsigned int)aux->buf[118], (unsigned int)aux->buf[119]);
vrPrintf(": '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', ...)\n", (unsigned int)aux->buf[120], (unsigned int)aux->buf[121], (unsigned int)aux->buf[122], (unsigned int)aux->buf[123], (unsigned int)aux->buf[124], (unsigned int)aux->buf[125], (unsigned int)aux->buf[126], (unsigned int)aux->buf[127], (unsigned int)aux->buf[128], (unsigned int)aux->buf[129]);
vrPrintf(": '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', ...)\n", (unsigned int)aux->buf[130], (unsigned int)aux->buf[131], (unsigned int)aux->buf[132], (unsigned int)aux->buf[133], (unsigned int)aux->buf[134], (unsigned int)aux->buf[135], (unsigned int)aux->buf[136], (unsigned int)aux->buf[137], (unsigned int)aux->buf[138], (unsigned int)aux->buf[139]);
vrPrintf(": '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', ...)\n", (unsigned int)aux->buf[140], (unsigned int)aux->buf[141], (unsigned int)aux->buf[142], (unsigned int)aux->buf[143], (unsigned int)aux->buf[144], (unsigned int)aux->buf[145], (unsigned int)aux->buf[146], (unsigned int)aux->buf[147], (unsigned int)aux->buf[148], (unsigned int)aux->buf[149]);
vrPrintf(": '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', ...)\n", (unsigned int)aux->buf[150], (unsigned int)aux->buf[151], (unsigned int)aux->buf[152], (unsigned int)aux->buf[153], (unsigned int)aux->buf[154], (unsigned int)aux->buf[155], (unsigned int)aux->buf[156], (unsigned int)aux->buf[157], (unsigned int)aux->buf[158], (unsigned int)aux->buf[159]);
#endif
					sscanf(&(aux->buf[8]), "%f %f %f %f", &(signals[0]), &(signals[1]), &(signals[2]), &(signals[3]));
					for (count = 0; count < 4; count++) {
						aux->units[count].tracking_quality = (unsigned char)(sqrtf(signals[count]) * 256);		/* NOTE: 256 is arbitrary -- I just chose a power of 2 that was in the ballpark, but doesn't even have to be a power of 2. */
						if (signals[count] < 0.0001)
							aux->units[count].tracking_status = 'X';
						else if (signals[count] < 0.0050)
							aux->units[count].tracking_status = 'L';
						else	aux->units[count].tracking_status = 'T';
					}
#if 0
					sscanf(&(aux->buf[55]), "%f %f %f %f", &(signals[0]), &(signals[1]), &(signals[2]), &(signals[3]));
					sscanf(&(aux->buf[102]), "%f %f %f %f", &(signals[0]), &(signals[1]), &(signals[2]), &(signals[3]));
#endif
					shift_bytes = 149;
					}
					break;	/* end of the '@S' case */

				}	/* end of the "aux->buf[3]" switch for '@' responses */
				} break;/* end of the '@' case */

			} /* End of "aux->buf[2]" switch for Patriot */

			/* for all the non-fall-through codes, skip out of the current switch statement */
			/* Ugly, I know. */
			if (aux->buf[2] != ' ')
				break;
		}
		/* End of special Patriot section.                                 */
		/*******************************************************************/

		unit = &(aux->units[station_num-1]);	/* TODO: read data into a temporary unit location and copy at the end -- but first copy current unit values into temporary space */
		in_unit = *unit;

		report_len = in_unit.toreport_len;
#if defined(COMM_DEBUG) || 0
		vrPrintf("_FastrakDecodeRecord(): unit %d: report_len is %d\n", station_num, report_len);
#endif
		/* this shouldn't happen, but if report length is bigger */
		/*   than the buffer, we have some serious problems.     */
		if ((report_len > BUFSIZE || report_len < 1)) {
static	int	warning_count = 0;
			vrPrintf(RED_TEXT "_FastrakDecodeRecord(): Warning: improbable report length for unit %d: %d\n" NORM_TEXT, station_num, report_len);
#if defined(TEST_APP)
			warning_count++;
			if (warning_count > 36) {
				vrPrintf(RED_TEXT "_FastrakDecodeRecord(): Warning: repeating improbable report length message, terminating.\n" NORM_TEXT);
				abort();
			}
#endif

			aux->buf[0] = '_';		/* the underscore is a signal to skip-to-NL in the next decode cycle */
			return 0;
		}

		/* if there aren't enough bytes for this report, then return from the function */
		if (aux->eobuf_pos < report_len) {
#ifdef COMM_DEBUG
			vrPrintf("_FastrakDecodeRecord(): Not enough data (%d) for station %d (%d bytes needed)\n", aux->eobuf_pos, station_num, report_len);
#endif
			return 0;
		}

#ifdef COMM_DEBUG
		vrDbgPrintfN(FASTRK_DBGLVL, "_FastrakDecodeRecord(): Decoding Data Record (station %d): '%s' (eobuf_pos = %d)\n", station_num, aux->buf, aux->eobuf_pos);
#endif

		if (aux->model != MODEL_PATRIOT && aux->model != MODEL_PATRIOTWL) {
			for (count = 0, report_pos = 3; count < in_unit.toreport_num; count++) {
				switch (in_unit.toreport_type[count]) {
#if 1
				case 0: report_pos++; break;/* just a space */
				case 1: report_pos+=2;break;/* just a CR/LF */
#else
				case 0:	break;		/* just a space */
				case 1: break;		/* just a CR/LF */
#endif
				case 2:			/* x,y,z locations in 3 floats */
					in_unit.data_pos[VR_X] = _FastrakReadFloat(&aux->buf[report_pos], 7, aux->reported_format);
					report_pos += 7;
					in_unit.data_pos[VR_Y] = _FastrakReadFloat(&aux->buf[report_pos], 7, aux->reported_format);
					report_pos += 7;
					in_unit.data_pos[VR_Z] = _FastrakReadFloat(&aux->buf[report_pos], 7, aux->reported_format);
					report_pos += 7;
					aux->receiver_change = 1;	/* TODO: ideally this would be a per-unit setting */
					break;
				case 4:			/* yaw, pitch & roll angles */
					in_unit.data_pos[VR_AZIM+3] = _FastrakReadFloat(&aux->buf[report_pos], 7, aux->reported_format);
					report_pos += 7;
					in_unit.data_pos[VR_ELEV+3] = _FastrakReadFloat(&aux->buf[report_pos], 7, aux->reported_format);
					report_pos += 7;	/* TODO: check this number */
					in_unit.data_pos[VR_ROLL+3] = _FastrakReadFloat(&aux->buf[report_pos], 7, aux->reported_format);
					report_pos += 7;	/* TODO: check this number */
					aux->receiver_change = 1;	/* TODO: ideally this would be a per-unit setting */
					break;
				case 22:		/* button information */
					/* TODO: we should be able to handle buttons for more than one wand -- ie. ideally s/b in_unit.buttons = ... 8/29/05 -- starting to address this */
					in_unit.buttons = _FastrakReadIntN(&aux->buf[report_pos], 4, aux->reported_format);
					report_pos += 5;	/* NOTE: the number is 4 ASCII bytes, but for some reason it comes back with a trailing space */
					aux->button_change = 1;
					break;
				case 23:		/* joystick information */
					/* TODO: we should be able to handle joysticks for more than one wand -- ie. ideally s/b in_unit.joystick[n] = ... 8/29/05 -- starting to address this */
					in_unit.joystick[0] = _FastrakReadIntN(&aux->buf[report_pos], 4, aux->reported_format);
					report_pos += 4;
					in_unit.joystick[1] = _FastrakReadIntN(&aux->buf[report_pos], 4, aux->reported_format);
					report_pos += 4;
					aux->valuator_change = 1;
					break;
				case 40:		/* tracking quality information */
					in_unit.tracking_quality = _FastrakReadIntN(&aux->buf[report_pos], 4, aux->reported_format);
					report_pos += 4;
					break;

				case 5:			/* X-axis direction cosines */
				case 6:			/* Y-axis direction cosines */
				case 7:			/* Z-axis direction cosines */
				case 11:		/* Quaternion Orientation */
				case 18:		/* special 16 bit binary x,y,z format */
				case 19:		/* special 16 bit binary a,e,r format */
				case 20:		/* special 16 bit binary quaternion format */
				case 21:		/* time stamp data */

				case 68:		/* "auxiliary input" */
				case 69:		/* "auxiliary input" */
				case 70:		/* "auxiliary input" */
				case 71:		/* "auxiliary input" */
				default:
					vrPrintf("_FastrakDecodeRecord(): Warning, unimplemented data type from tracker %d -- buf = '%c'\n", in_unit.toreport_type[count], aux->buf[0]);
					break;
				}
			}
		} else {
			/* This section is for decoding of the Patriot data */
			for (count = 0, report_pos = 4; count < in_unit.toreport_num; count++) {
				switch (in_unit.toreport_type[count]) {
#if 1
				case 0: report_pos++; break;/* just a space */
				case 1: report_pos+=2;break;/* just a CR/LF */
#else
				case 0:	break;		/* just a space */
				case 1: break;		/* just a CR/LF */
#endif
				case 2:			/* x,y,z locations in 3 floats */
					in_unit.data_pos[VR_X] = _FastrakReadFloat(&aux->buf[report_pos], 9, aux->reported_format);
					report_pos += 9;
					in_unit.data_pos[VR_Y] = _FastrakReadFloat(&aux->buf[report_pos], 9, aux->reported_format);
					report_pos += 9;
					in_unit.data_pos[VR_Z] = _FastrakReadFloat(&aux->buf[report_pos], 9, aux->reported_format);
					report_pos += 9;
					aux->receiver_change = 1;	/* TODO: ideally this would be a per-unit setting */
					break;
				case 4:			/* yaw, pitch & roll angles */
					in_unit.data_pos[VR_AZIM+3] = _FastrakReadFloat(&aux->buf[report_pos], 9, aux->reported_format);
					report_pos += 9;
					in_unit.data_pos[VR_ELEV+3] = _FastrakReadFloat(&aux->buf[report_pos], 9, aux->reported_format);
					report_pos += 9;	/* TODO: check this number */
					in_unit.data_pos[VR_ROLL+3] = _FastrakReadFloat(&aux->buf[report_pos], 9, aux->reported_format);
					report_pos += 9;	/* TODO: check this number */
					aux->receiver_change = 1;	/* TODO: ideally this would be a per-unit setting */
					break;
				case 10:		/* button information */
					in_unit.buttons = _FastrakReadIntN(&aux->buf[report_pos], 3, aux->reported_format);
					report_pos += 3;	/* NOTE: the number is 4 ASCII bytes, but for some reason it comes back with a trailing space */
					aux->button_change = 1;
					break;

				case 3:			/* High-res Cartesian location */
				case 5:			/* High-res Euler angles */
				case 6:			/* Direction Cosines matrix */
				case 7:			/* Quaternion Orientation */
				case 8:			/* time stamp data */
				case 9:			/* frame count data */

				default:
					vrPrintf("_FastrakDecodeRecord(): Warning, unimplemented data type from tracker %d -- buf = '%c'\n", in_unit.toreport_type[count], aux->buf[0]);
					break;
				}
			}
			/* End of special Patriot decoding section */
		}

		/* Make sure last character was a CR/LF -- perhaps this can be done in the general section */
		if (aux->buf[report_len-1] != '\n') {
			vrPrintf(RED_TEXT "_FastrakDecodeRecord(): Warning, station %d expecting a newline at byte %d, but got (0x%02x) instead: '%s'\n" NORM_TEXT, station_num, report_len, aux->buf[report_len-1], aux->buf);
			aux->buf[0] = '_';		/* the underscore is a signal to skip-to-NL in the next decode cycle */
			shift_bytes = 0;
		} else {
#if defined(COMM_DEBUG) || 0
			vrPrintf(BOLD_TEXT "_FastrakDecodeRecord(): Made a good decoding for station %d\n" NORM_TEXT, station_num);
#endif
			shift_bytes = report_len;

			/* now put the decoded data into the actual unit data structure */
			*unit = in_unit;
			unit->read_count++;
		}
		} break;

	/*****************/
	/* Status Record */
	case '2': /* { */

		switch (aux->buf[2]) {

		/***************************************************/
		/* System Status Record (w/ version info), pg. 110 */
		case 'S':
			/* byte 1: s/b '1'                                    */
			/* byte 3: Config Hex Char 0                          */
			/* byte 4: Config Hex Char 1                          */
			/* byte 5: Config Hex Char 2                          */
			/*	bit 0: output format (0=ASCII, 1=Binary)      */
			/*	bit 1: output units (0=inches, 1=centimeters) */
			/*	bit 3: transmit mode (0=Polled, 1=Continuous) */
			/* bytes 16-21: Firmware version ID                   */
			/* bytes 22-53: System Identification                 */

			/* verify that eobuf_pos >= 55 and that aux->buf[1] = '1' */
			if (aux->eobuf_pos < 55) {
				/* haven't read enough bytes yet */
				return 0;
			}

			if (aux->buf[1] != '1') {
				/* incorrect byte in the record */
				vrDbgPrintfN(AALWAYS_DBGLVL, "_FastrakDecodeRecord(): invalid record\n");
			}

			vrDbgPrintfN(FASTRK_DBGLVL, "_FastrakDecodeRecord(): Got Status Record: '%s'\n", aux->buf);
			aux->reported_format = aux->buf[5] & 0x01;	/* bit 0 of byte 5 */
			aux->reported_units  = (aux->buf[5] & 0x02) != 0;	/* bit 1 of byte 5 */ /* TODO: make 0 or 1 */
			aux->reported_mode   = (aux->buf[5] & 0x08) != 0;	/* bit 3 of byte 5 */ /* TODO: make 0 or 1 */
			sscanf((char *)&aux->buf[15], "%s %s", aux->firmware_version, aux->system_id);

			/* use the system id string to set the manufacturer */
			if (!strncasecmp("is900", (char *)&aux->buf[22], 5)) {
				aux->manufacturer = MANU_INTERSENSE;
				aux->model = MODEL_IS900;
			} else {
				aux->manufacturer = MANU_POLHEMUS;
				aux->model = MODEL_FASTRAK;	/* NOTE: Polhemus Patriot (and perhaps Liberty) do not have the "S" command */
			}

#if defined(COMM_DEBUG) || 0
			vrPrintf(BOLD_TEXT "_FastrakDecodeRecord(): Made a good decoding for system status.\n" NORM_TEXT);
#endif
			shift_bytes = 55;
			if (aux->buf[shift_bytes-1] != '\n') {
				aux->buf[0] = '_';		/* the underscore is a signal to skip-to-NL in the next decode cycle */
				shift_bytes = 0;
			}

			break;

		/*******************************/
		/* Output List Record, pg. 111 */
		case 'O': {
			int		num_types = 0;
			int		record_pos;		/* position in the record we are currently decoding */
			int		record_length = 3;	/* 3 bytes for the station number */
			_FastrakUnit	*unit;

			station_num = STATION_CHAR2NUM(aux->buf[1]);
			unit = &(aux->units[station_num-1]);

#if defined(COMM_DEBUG) || 0
			vrPrintf("_FastrakDecodeRecord(): Got Output List Record for station %d: '%s'\n", station_num, aux->buf);
#endif

			for (record_pos = 3; aux->buf[record_pos] != '\r'; record_pos += 2) {
				int	type;

				type = _FastrakReadIntN(&aux->buf[record_pos], 2, 0);

				switch (type) {
				case  0: record_length += 1;	break;	/* ASCII space character */
				case  1: record_length += 2;	break;	/* CR/LF */
				/* TODO: binary values for below -- these work for ASCII only */
				case  2: record_length += 7*3;	break;	/* x,y,z location: 3 floats */
				case  4: record_length += 7*3;	break;	/* yaw,pitch,roll angles: 3 floats */
				case  5: record_length += 7*3;	break;	/* X-axis cosines: 3 floats */
				case  6: record_length += 7*3;	break;	/* Y-axis cosines: 3 floats */
				case  7: record_length += 7*3;	break;	/* Z-axis cosines: 3 floats */
				case 11: record_length += 7*4;	break;	/* Quaternion Orientation */
				case 16: record_length += 1;	break;	/* stylus switch status (always 0) */
				case 22: record_length += 5;	break;	/* button presses */
				case 23: record_length += 8;	break;	/* X & Y joystick values */
				case 40: record_length += 4;	break;	/* Tracking status */

				/* TODO: implement the rest of the types */
				case 18:	/* special 16 bit binary x,y,z format */
				case 19:	/* special 16 bit binary a,e,r format */
				case 20:	/* special 16 bit binary quaternion format */
				case 21:	/* time stamp data */

				case 68:	/* "auxiliary input" */
				case 69:	/* "auxiliary input" */
				case 70:	/* "auxiliary input" */
				case 71:	/* "auxiliary input" */
				default:
					vrPrintf("_FastrakDecodeRecord(): Warning, unimplemented output record type: %d for station %d\n", type, station_num);
				}
				unit->toreport_type[num_types] = type;
				num_types++;
			}
			unit->toreport_num = num_types;
			unit->toreport_len = record_length;
#if defined(COMM_DEBUG)
			vrPrintf("_FastrakDecodeRecord(): Fastrak unit %d: report_len set to %d\n", station_num, record_length);
#endif

			memcpy(unit->toreport, &aux->buf[3], record_pos-3);
			unit->toreport[record_pos-3] = '\0';	/* terminate the string */

#if defined(COMM_DEBUG) || 0
			vrPrintf(BOLD_TEXT "_FastrakDecodeRecord(): Made a good decoding for station %d output format.\n" NORM_TEXT, station_num);
#endif
			record_pos += 2;	/* skip the CR/LF */
			shift_bytes = record_pos;
			if (aux->buf[shift_bytes-1] != '\n') {
				vrDbgPrintfN(FASTRK_DBGLVL, "_FastrakDecodeRecord(): Warning, doing a force-skip in unsure circumstances.\n", aux->buf);	/* TODO: determine whether/why we ever need to do this -- it should be cleaner */
				aux->buf[0] = '_';		/* the underscore is a signal to skip-to-NL in the next decode cycle */
				shift_bytes = 0;
			}
			} break;

		/*********************************/
		/* Station State Record, pg. 111 */
		case 'l': { /* The response to an "l<n>\n" or "l*\n" command */
			int		count;
			int		max_units;	/* the "l*" command only returns 32 units */
			_FastrakUnit	*unit;

			/* NOTE: there are basically two possible response types:             */
			/*   1) the state of 4 units  (<n>234)                                */
			/*   2) the state of 32 units (123456789abcdefghijklmnopqrstuvw)      */
			/* If we get the first response, we will only deal with station <n>.  */
			/* If we get the second type of response, we will do all 32 stations. */
			station_num = STATION_CHAR2NUM(aux->buf[1]);
			unit = &(aux->units[station_num-1]);

			if (aux->buf[7] == '\r') {
#ifdef COMM_DEBUG
				vrPrintf("_FastrakDecodeRecord(): Got a Single Station State Record for station %d: '%s'\n", station_num, aux->buf);
#endif
				unit->state = (int)(aux->buf[3] == '1');
				shift_bytes = 9;
			} else {
#ifdef COMM_DEBUG
				vrPrintf("_FastrakDecodeRecord(): Got a Station State Record for all stations '%s'\n", aux->buf);
#endif
				/* clamp number of units to 32 */
				max_units = MAX_UNITS;
				if (max_units > 32)
					max_units = 32;

				for (count = 0; count < max_units; count++) {
					aux->units[count].state = (int)(aux->buf[count+3] == '1');
				}
#if defined(COMM_DEBUG) || 0
			vrPrintf(BOLD_TEXT "_FastrakDecodeRecord(): Made a good decoding for all station states.\n" NORM_TEXT);
#endif
				shift_bytes = 37;
			}

			aux->got_state = 1;	/* flag that we've got the receiver states */

			} break;

		/*****************************/
		/* Unimplemented 2.x Records */
		case 'A':
		case 'G':
		case 'H':
		case 'N':
		case 'V':
			/* skip buffer to next CR,LF */
			aux->buf[0] = '_';		/* the underscore is a signal to skip-to-NL in the next decode cycle */
			shift_bytes = 0;

			vrPrintf("_FastrakDecodeRecord(): Got Unimplemented Status Record of type '%c': '%s'\n", aux->buf[2], aux->buf);
			break;

		/************************/
		/* Unknown Record Types */
		default:
			aux->buf[0] = '_';		/* the underscore is a signal to skip-to-NL in the next decode cycle */
			shift_bytes = 0;
			vrPrintf("_FastrakDecodeRecord(): Got Unknown Status Record of type '%c': '%s'\n", aux->buf[2], aux->buf);
			break;
		}
		break;
	/* } end of case '2' */

	/****************************/
	/* InterSense Status Record */
	case '3': /* { */

		switch (aux->buf[2]) {

		/**********************************************/
		/* Manufacturer System Status Record, pg. 114 */
		/*   [NOTE: for now we assume "Manufacturer" is InterSense.] */
		case 'S': /* The response to an "MS\n" command */
			/* TODO: verify that aux->buf[1] = '1' */

			vrPrintf("_FastrakDecodeRecord(): Got InterSense Status Record: '%s' (but parsing not yet implemented)\n", aux->buf);

			/* TODO: something interesting with this data -- see pg 73 of IS900_SIM_VE_Manual_v1.pdf */
			aux->buf[0] = '_'; shift_bytes = 0;	/* Temporary, until parsing code added */
			break;

		/****************************************/
		/* Manufacturer Station Record, pg. 115 */
		case 's': /* The response to an "Ms<n>\n" command */
			station_num = STATION_CHAR2NUM(aux->buf[1]);

			vrPrintf("_FastrakDecodeRecord(): Got InterSense Station %d Record: '%s' (but parsing not yet implemented)\n", station_num, aux->buf);

			/* TODO: something interesting with this data -- see pp 73-74 of IS900_SIM_VE_Manual_v1.pdf */
			aux->buf[0] = '_'; shift_bytes = 0;	/* Temporary, until parsing code added */
			break;

		/***********************************/
		/* Tracking Status Record, pg. 118 */
		case 'P': { /* The response to an "MP\n" command */
			int		count;
			int		max_units;
			_FastrakUnit	*unit;

			/* verify that aux->buf[1] = '1' */
			if (aux->buf[1] != '1') {
				vrPrintf(RED_TEXT "_FastrakDecodeRecord(): Warning 2nd byte of '31P' response is not '1' -- skipping record\n");
				aux->buf[0] = '_'; shift_bytes = 0;
				return 0;
			}

			if (aux->eobuf_pos < 48) {
				/* haven't read enough bytes yet */
				return 0;
			}

#if defined(COMM_DEBUG) || 0
			vrPrintf("_FastrakDecodeRecord(): Parsing InterSense Tracking Status Record: '%s'\n", aux->buf);
#endif

			/* Parse the tracking status data -- see pg 76,79 of IS900_SIM_VE_Manual_v1.pdf */

			/* clamp max_units to 12 as per the max reported by the MP command */
			max_units = MAX_UNITS;
			if (max_units > 12)
				max_units = 12;

			for (count = 0; count < MAX_UNITS; count++) {
				unit = &(aux->units[count]);

				unit->tracking_status = aux->buf[count*3 + 3];

				/* tracking_meas & tracking_reject are single digit hex values */
				unit->tracking_meas   = (int)((aux->buf[count*3 + 4]) - '0');
				if (unit->tracking_meas > 9)
					unit->tracking_meas -= ('a' - '0' - 10);
				unit->tracking_reject = (int)((aux->buf[count*3 + 5]) - '0');
				if (unit->tracking_reject > 9)
					unit->tracking_reject -= ('a' - '0' - 10);
			}

			aux->update_rate = _FastrakReadIntN(&aux->buf[39], 4, 0);
			aux->genlock_id = aux->buf[45];

#if defined(COMM_DEBUG) || 0
			vrPrintf(BOLD_TEXT "_FastrakDecodeRecord(): Made a good decoding for system tracking status.\n" NORM_TEXT);
#endif
			shift_bytes = 48;

			} break;

		/***********************************/
		/* PSE data, pg. ? */
		case 'F': { /* The response to an "MCF\n" command */
			int			count;
			int			bonus_pse = 0;	/* the number of bytes which the 2nd field causes an extension to the record length -- to account for an IS900 firmware bug */
			_FastrakTransUnit	new_tunit;

			/* verify that aux->buf[1] = '1' */
			if (aux->buf[1] != '1') {
				vrPrintf(RED_TEXT "_FastrakDecodeRecord(): Warning 2nd byte of '31F' response is not '1' -- skipping record\n");
				aux->buf[0] = '_'; shift_bytes = 0;
				return 0;
			}

			if (aux->eobuf_pos < 70) {	/* Of course, due to the IS900 firmware bug, the message might be a couple bytes longer, so this is no guarantee */
				/* haven't read enough bytes yet */
				return 0;
			}

			new_tunit.assigned = 0;

			/* Some adjustments need to be made based on bugs in the IS900 firmware -- the 2nd field often goes long and extends the entire record */
			for (bonus_pse = 0; aux->buf[10+bonus_pse] != ' '; bonus_pse++);
			vrDbgPrintfN(FASTRK_DBGLVL, "_FastrakDecodeRecord(): Need to extend the MCF-31F record by %d bytes\n", bonus_pse);
#if defined(COMM_DEBUG) || 0
			vrPrintf("_FastrakDecodeRecord(): Need to extend the MCF-31F record by %d bytes\n", bonus_pse);
#endif
			new_tunit.pse_id = _FastrakReadIntN(&aux->buf[3], 7+bonus_pse, 0);
			new_tunit.position[VR_X] = _FastrakReadFloat(&aux->buf[10+bonus_pse], 10, 0);
			new_tunit.position[VR_Y] = _FastrakReadFloat(&aux->buf[20+bonus_pse], 10, 0);
			new_tunit.position[VR_Z] = _FastrakReadFloat(&aux->buf[30+bonus_pse], 10, 0);
			new_tunit.position[VR_X+3] = _FastrakReadFloat(&aux->buf[40+bonus_pse], 7, 0);
			new_tunit.position[VR_Y+3] = _FastrakReadFloat(&aux->buf[47+bonus_pse], 7, 0);
			new_tunit.position[VR_Z+3] = _FastrakReadFloat(&aux->buf[54+bonus_pse], 7, 0);
			new_tunit.hardware_id = _FastrakReadIntN(&aux->buf[61+bonus_pse], 7, 0);

			/* Now search to see if this hardware_id is already in the list */
			for (count = 0; count < aux->num_tunits; count++) {
				if ((aux->tunits[count].assigned == 1) && (aux->tunits[count].hardware_id == new_tunit.hardware_id)) {
					new_tunit.assigned = 1;
					aux->tunits[count] = new_tunit;
				}
			}
			/* If not, add it to the list */
			if (!new_tunit.assigned) {
				new_tunit.assigned = 1;
				aux->tunits[aux->num_tunits] = new_tunit;
				aux->num_tunits++;
				vrDbgPrintfN(FASTRK_DBGLVL, "_FastrakDecodeRecord(): new hardware id %d\n", new_tunit.hardware_id);
			} else {
				vrDbgPrintfN(FASTRK_DBGLVL, "_FastrakDecodeRecord(): existing hardware id %d\n", new_tunit.hardware_id);
			}

#if defined(COMM_DEBUG) || 0
			vrPrintf(BOLD_TEXT "_FastrakDecodeRecord(): Made a good decoding for transmitter unit %d:%d.\n" NORM_TEXT, new_tunit.pse_id, new_tunit.hardware_id);
#endif
			shift_bytes = 70+bonus_pse;	/* Need to adjust to accommodate for the IS900 firmware bug */

			} break;

		/*************************/
		/* Unimplemented Records */
		case 'p':
		case 'Q':
		case 'G':
		case 'U':
		case 'g':
			/* skip buffer to next CR,LF */
			aux->buf[0] = '_';		/* the underscore is a signal to skip-to-NL in the next decode cycle */
			shift_bytes = 0;
			break;

		/************************/
		/* Unknown Record Types */
		default:
			/* skip buffer to next CR,LF */
			aux->buf[0] = '_';		/* the underscore is a signal to skip-to-NL in the next decode cycle */
			shift_bytes = 0;
			break;
		}
		break;
	/* } end of case '3' for InterSense */

	/*******************/
	/* Whitespace type */
	case ' ':
	case '\n':
vrPrintf("YO whitespace -- %02x\n", aux->buf[0]);
		/* TODO: skip entire whitespace string */
		shift_bytes = 1;
#if 0	/* setting this to '1' should cause a skip to the new newline */
		if (aux->buf[shift_bytes-1] != '\n') {
			aux->buf[0] = '_';		/* the underscore is a signal to skip-to-NL in the next decode cycle */
			shift_bytes = 0;
		}
#endif
		break;

	/****************************/
	/* Other Misc possibilities */
	case 'P':
		/* most probable input is the Patriot startup string */
		if (!strncmp((char *)(&(aux->buf[0])), "Patriot Ready!", 14)) {
			vrPrintf("_FastrakDecodeRecord(): Got the Patriot startup message - '%s'\n", &aux->buf[0]);
			shift_bytes = 16;
			aux->model = MODEL_PATRIOT;
		} else
		if (!strncmp((char *)(&(aux->buf[0])), "Patriot Wireless Ready!", 23)) {
			vrPrintf("_FastrakDecodeRecord(): Got the Patriot Wireless startup message - '%s'\n", &aux->buf[0]);
			shift_bytes = 25;
			aux->model = MODEL_PATRIOTWL;
		} else {
			vrDbgPrintfN(FASTRK_DBGLVL, "_FastrakDecodeRecord(): " RED_TEXT "Warning:" NORM_TEXT " got an unknown message that starts with 'P' -- '%s'\n", &aux->buf[0]);
		}
		break;

	case 'I':
		/* most probable response is an error message from the Patriot */
		if (!strncmp((char *)(&(aux->buf[0])), "Invalid Command!  *****", 23)) {
			vrPrintf("_FastrakDecodeRecord(): Got a Patriot error message - Invalid Command! '%02x %02x %02x %02x %02x'\n", aux->buf[25], aux->buf[26], aux->buf[27], aux->buf[28], aux->buf[29]);
			shift_bytes = 27;
		} else
		if (!strncmp((char *)(&(aux->buf[0])), "Invalid Parameter!  *****", 25)) {
			vrPrintf("_FastrakDecodeRecord(): Got a Patriot error message - Invalid Parameter! '%02x %02x %02x %02x %02x'\n", aux->buf[27], aux->buf[28], aux->buf[29], aux->buf[30], aux->buf[31]);
			shift_bytes = 29;
		} else
			vrPrintf("_FastrakDecodeRecord(): Got an unknown 'I' message from the Patriot (probably an error): '%50s'\n", &(aux->buf[0]));

		break;

	/* TODO: do a better job of handling these Patriot-only (perhaps Patriot Wireless only) responses */
	case 'F':
		/* most probable response is response from the Patriot for the format command (ASCII/binary) */
		if (!strncmp((char *)(&(aux->buf[0])), "F0", 2)) {
			vrPrintf("_FastrakDecodeRecord(): Got a Patriot message - F0\n");
			shift_bytes = 4;
		} else
			vrPrintf("_FastrakDecodeRecord(): Got an unknown 'F' message from the Patriot (probably an error): '%50s'\n", &(aux->buf[0]));
		break;

	case 'U':
		/* most probable response is response from the Patriot for the scale command (inches/centimeters) */
		if (!strncmp((char *)(&(aux->buf[0])), "U0", 2)) {
			vrPrintf("_FastrakDecodeRecord(): Got a Patriot message - U0\n");
			shift_bytes = 4;
		} else
			vrPrintf("_FastrakDecodeRecord(): Got an unknown 'U' message from the Patriot (probably an error): '%50s'\n", &(aux->buf[0]));
		break;

	case '@':
		/* most probable response is response from the Patriot for the autolaunch command */
		if (!strncmp((char *)(&(aux->buf[0])), "@A1", 3)) {
			vrPrintf("_FastrakDecodeRecord(): Got a Patriot message - @A1\n");
			shift_bytes = 5;
		} else
			vrPrintf("_FastrakDecodeRecord(): Got an unknown '@' message from the Patriot (probably an error): '%50s'\n", &(aux->buf[0]));
		break;

	case 'E':
		/* most probable response is response from the Patriot for the Echo Off command */
		if (!strncmp((char *)(&(aux->buf[0])), "Echo Off", 8)) {
			vrPrintf("_FastrakDecodeRecord(): Got a Patriot message - Echo Off\n");
			shift_bytes = 10;
		} else
			vrPrintf("_FastrakDecodeRecord(): Got an unknown 'E' message from the Patriot (probably an error): '%50s'\n", &(aux->buf[0]));
		break;


	/***********************************/
	/* Skip to newline (NL, aka CR/LF) */
	case '_': {
		/* NOTE: this is not part of the official protocol, but rather is used by this  */
		/*   driver to indicate a past decoding failure, and therefore the need to skip */
		/*   over all data until (and including) the next newline.                      */
		int	find_nl;	/* counter to loop through data looking for newline */

		for (find_nl = 0; find_nl < aux->eobuf_pos && aux->buf[find_nl] != '\n'; find_nl++);

		if (aux->buf[find_nl] == '\n') {
			shift_bytes = find_nl+1;
#if defined(COMM_DEBUG) || 0 /* TODO: set this to 0 when done debugging */
			vrPrintf("_FastrakDecodeRecord(): skipping to newline (%d bytes: '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', '0x%02x', ...)\n", shift_bytes, (unsigned int)aux->buf[0], (unsigned int)aux->buf[1], (unsigned int)aux->buf[2], (unsigned int)aux->buf[3], (unsigned int)aux->buf[4], (unsigned int)aux->buf[5], (unsigned int)aux->buf[6], (unsigned int)aux->buf[7]);
#endif
		} else {
			/* need to wait until more data is read and there is a newline */
			shift_bytes = 0;
#if defined(COMM_DEBUG) || 0 /* TODO: set this to 0 when done debugging */
			vrPrintf("_FastrakDecodeRecord(): need more data to skip to newline\n");
#endif
			return 0;	/* return 0 causes driver to read more data before doing more decoding (otherwise we're stuck in an infinite loop) */
		}
		} break;

	/***********************/
	/* Unknown Record type */
	default:
		if (aux->model != MODEL_PATRIOT && aux->model != MODEL_PATRIOTWL) {
			vrPrintf("_FastrakDecodeRecord(): Got an unknown record type ('0x%02x') will skip to newline\n", aux->buf[0]);
		} else {
			vrPrintf("_FastrakDecodeRecord(): Got an unknown message from the Patriot (probably an error): '%50s'\n", &(aux->buf[0]));
		}
		aux->buf[0] = '_';		/* the underscore is a signal to skip-to-NL in the next decode cycle */
		shift_bytes = 0;
#if 1 /* 04/04/11: Test for skipping garbage after doing a warm reset on the Patriot Wireless */
		shift_bytes = 1;
#endif
		break;
	} /* End of aux->buf[0] (first byte of response) switch */

	/* Now do the buffer shifting */
	if (shift_bytes > 0) {
		/* shift the buffer past parsed data */
#ifdef COMM_DEBUG
		printf("_FastrakDecodeRecord(): shifting %d bytes (prior to shift eobuf_pos is %d)\n", shift_bytes, aux->eobuf_pos);
#endif
#if 0 /* set to "1" for robustness testing -- ie. adding a bug to see if the rest of the program can recover */
		memmove(aux->buf, &aux->buf[shift_bytes], shift_bytes);	/* for robustness testing, use this line for Major bug */
#else
		memmove(aux->buf, &aux->buf[shift_bytes], (aux->eobuf_pos-shift_bytes));	/* Major bug removed from this line 9/7/2005 */
#endif
		aux->eobuf_pos -= shift_bytes;
#ifndef COMM_DEBUG
		aux->buf[aux->eobuf_pos] = '\0';
#else
		aux->buf[aux->eobuf_pos] = '%';	/* put a percent-sign in the buffer to help debug */
		vrPrintf("_FastrakDecodeRecord(): decoded & shifted %d bytes, now have %d bytes in buffer: '%s'\n", shift_bytes, aux->eobuf_pos, aux->buf);
#endif
	}

	/* if the buffer has been exhausted, then nothing more to parse */
	if (aux->eobuf_pos == 0)
		return 0;

	/* otherwise report that there may be more to parse */
	return 1;
}


/****************************************************************************/
/* _FastrakReadInput(): function reads data from the serial port, and then  */
/*   looks for and parses packets from the device.  The data is then placed */
/*   into the generic portion of the "_FasktrakPrivateInfo" structure.      */
/* Returns a boolean based on whether any input was read this call.         */
/* NOTE: _FastrakReadInput() is placed above the _FastrakInitializeDevice() */
/*   function to allow the latter to call the former.                       */
static int _FastrakReadInput(_FastrakPrivateInfo *aux)
{
	int	bytes_read;

	/* Read as much data as is available, and decode whatever is then in the buffer */
#ifdef COMM_DEBUG
		vrPrintf("_FastrakReadInput(): reading up to %d bytes (%d - %d - 1)\n", (sizeof(aux->buf) - aux->eobuf_pos - 1), sizeof(aux->buf), aux->eobuf_pos);
#endif
	bytes_read = vrSerialRead(aux->fd, ((char *)aux->buf + aux->eobuf_pos), sizeof(aux->buf) - aux->eobuf_pos - 1);
	if (bytes_read == 0) {
		/* there wasn't any input ready */
		return 0;
	}
	if (bytes_read < 0) {
		vrPrintf(RED_TEXT "_FastrakReadInput(): error -- %d reported from vrSerialRead()\n" NORM_TEXT, bytes_read);
	} else {
		aux->eobuf_pos += bytes_read;
#ifdef COMM_DEBUG
		vrPrintf("_FastrakReadInput(): just read %d bytes (eobuf_pos is now %d)\n", bytes_read, aux->eobuf_pos);
		aux->buf[aux->eobuf_pos] = '$';		/* When in debug mode, show the end of the read with a '$' -- Warning, since the buffer may not end in '\0', the printout may show other data that isn't part of the buffer! */
#else
		aux->buf[aux->eobuf_pos] = '\0';
#endif
	}

	/* decode data in buffer until no complete records are available */
	//while (printf("hey\n"),_FastrakDecodeRecord(aux)) {
	while (_FastrakDecodeRecord(aux)) {
#ifdef COMM_DEBUG
		vrPrintf("_FastrakReadInput(): one pass through the _FastrakDecodeRecord() while loop\n");
#endif
	}

#ifdef COMM_DEBUG
	vrPrintf("_FastrakReadInput(): Bottom of function\n");
#endif
	return 1;	/* some input has been read (and perhaps decoded too -- though perhaps that part should be elsewhere) */
}


/***********************************************************/
/* _FastrakInitializeDevice() is called in the OPEN phase  */
/*   of input interface -- after the types of inputs have  */
/*   been determined (during the CREATE phase).            */
static int _FastrakInitializeDevice(_FastrakPrivateInfo *aux, char *name)
{
	int		bytes_read;
	int		unit_count;

	if (aux == NULL) {
		vrErrPrintf("_FastrakInitializeDevice(): "
			RED_TEXT "Warning, no auxiliary data for Fastrak.\n" NORM_TEXT);
		return -1;
	}

	/******************************************************************************/
	/* First, verify that (some of) the underlying data structures are consistent */
	if (strcmp(_FastrakMsgList[FASTRAK_LASTCOMMAND].name, "FASTRAK_LASTCOMMAND") != 0) {
		/* NOTE: this generally happens when a new command is added to     */
		/*   the _FastrakMsgList[] array, but the corresponding enumerated */
		/*   value is not added to the _FastrakCommand type definition.    */
		vrErrPrintf(RED_TEXT "_FastrakInitializeDevice(): The Fastrak device source code is not consistent -- '_FastrakMsgList[]'\n" NORM_TEXT);
		exit(1);
	}

	aux->model = MODEL_FASTRAK;	/* default setting */

	/****************************************/
	/* Make initial contact with the Device */

	/* Go into "polled mode" for communicating with the device */
	vrPrintf("_FastrakInitializeDevice(): Fastrak Device '%s': Going into 'polled mode'\n", name);
	_FastrakSendCommand(aux, FASTRAK_POLLMODE);

	/* go into ASCII mode */
	_FastrakSendCommand(aux, FASTRAK_ASCIIMODE);

	/* go into Inches mode */
	_FastrakSendCommand(aux, FASTRAK_SET_INCHES);

	/* get system information */
	_FastrakSendCommand(aux, FASTRAK_SYSSTAT);	/* see IS900 pg 110 to decode */

	/* send any initialization commands from the FreeVR configuration */
	if (aux->init_command != NULL) {
		_FastrakSendCommandString(aux, aux->init_command);
	}

	/****************************************************************/
	/* request data from device necessary for proper initialization */
	_FastrakSendCommand(aux, FASTRAK_GET_ALLRECEIVERS);
	vrSleep(40000);					/* give time for the responses */

	/**********************************************************/
	/* parse all the incoming responses to the above requests */

	/* TODO: Since we know the "get receivers" command was the last sent, */
	/*   we continue to decode until that response has been parsed. */

	while (!aux->got_state) {
#if 1 /* TODO: determine whether to delete this line (see also the "_PatriotInitializeDevice()" routine) */
		while (_FastrakDecodeRecord(aux)) { vrPrintf("_FastrakInitializeDevice(): loop of unknown value -- do we get here?\n"); }
#endif
		_FastrakReadInput(aux); /* [NOTE: we're not making use of the return value.] */
	}	/* get all the incoming data */

	/* now do manufacturer specific calls -- NOTE: must wait until after FASTRAK_SYSSTAT command is processed */
	if (aux->manufacturer == MANU_INTERSENSE) {
		vrPrintf("_FastrakInitializeDevice(): Fastrak Device '%s': " RED_TEXT "Sending InterSense IS900 specific commands\n" NORM_TEXT, name);
#if 1 /* [09/24/2009 BS: removing this for now because it may cause issues (due to incorrect responses from the IS900), and probably isn't necessary.] [11/05/2009 BS: obviously I put this back in order to do robustness testing, and because I installed a work-around for the IS900 firmware bug.] */
		_FastrakSendCommand(aux, IS900_GET_TUNITS);		/* TODO: BS: [09/24/2009] Is this necessary?  Do we even store this data? */
#endif
		_FastrakSendCommand(aux, IS900_GET_TRACKING_STATUS);
		vrSleep(40000);				/* give extra time for IS-900 responses */

		_FastrakDecodeRecord(aux);

		if (aux->num_tunits < 0) {
			vrErrPrintf(RED_TEXT "_FastrakInitializeDevice(): Warning: no transmitter data obtained for InterSense device '%s'.\n" NORM_TEXT, name);
		}
	}

	/****************************************/
	/* Set the parameters for all the units */

	/* count the number of active units */
	aux->num_units = 0;
	for (unit_count = 0; unit_count < MAX_UNITS; unit_count++) {
		if (aux->units[unit_count].state) {
			aux->num_units++;
		}
	}

	for (unit_count = 0; unit_count < MAX_UNITS; unit_count++) {
		/* TODO: perhaps only send to units known to exist -- though shouldn't hurt to send to all */
		if (aux->units[unit_count].has_button)
			_FastrakSendCommandAddr(aux, FASTRAK_SET_OUTPUT_POSITION_ANGLES_BJ, unit_count+1);
		else	_FastrakSendCommandAddr(aux, FASTRAK_SET_OUTPUT_POSITION_ANGLES, unit_count+1);
	}

	/* make sure to request enough information from the system to properly decode input */
	/* NOTE: since we're setting the params, we could just know this in advance, but */
	/*   it's cleaner to cover even the times when we just make use of the existing  */
	/*    parameters.                                                                */
	for (unit_count = 0; unit_count < MAX_UNITS; unit_count++) {
		if (aux->units[unit_count].state) {
			_FastrakSendCommandAddr(aux, FASTRAK_GET_STATION_PARAMS, unit_count+1);
#if defined(COMM_DEBUG)
			vrPrintf("_FastrakInitializeDevice(): Sent request for station parameters for station %d\n", unit_count+1);
#endif
		}
	}

#if 0 /* not sure whether I want to implement this */
	/******************************************************************/
	/* Reget the current status of the system to verify it is running */
		/* TODO: this */
#endif

	/****************************************/
	/* Create the version and param strings */
	sprintf(aux->version, "model: %s, revision: %s", aux->system_id, aux->firmware_version);
		/* TODO: the param strings (if any) */


	/*****************************/
	/* Set the system parameters */
		/* TODO: set for binary mode for example */



	/***************************************/
	/* Go into stream mode -- if specified */
	if (aux->stream_mode) {
		/* Set the report rate */
		/* TODO: is there a report-rate command? */

		/* Go into stream mode */
		/* TODO: this */
	}


	/*****************************/
	/* read the first input data */

	/* send request for data when in polled mode */
	if (!aux->stream_mode) {
		_FastrakSendCommand(aux, FASTRAK_POLL);
#define STATUS_POLL
#ifdef STATUS_POLL
		if (aux->manufacturer == MANU_INTERSENSE) {
			_FastrakSendCommand(aux, IS900_GET_TRACKING_STATUS);
		}
		if (aux->model == MODEL_PATRIOTWL) {
			_FastrakSendCommand(aux, PATRIOTWL_GET_SIGNAL_STATUS);
		}
#endif
	}
	_FastrakReadInput(aux);		/* NOTE: also decodes as much as possible.  NOTE: we're not making use of the return value. */

	return 0;
}


/***********************************************************/
/* _PatriotInitializeDevice() is called in the OPEN phase  */
/*   of input interface -- after the types of inputs have  */
/*   been determined (during the CREATE phase).            */
static int _PatriotInitializeDevice(_FastrakPrivateInfo *aux, char *name)
{
	int		bytes_read;
	int		unit_count;

	if (aux == NULL) {
		vrErrPrintf("_PatriotInitializeDevice(): "
			RED_TEXT "Warning, no auxiliary data for Patriot.\n" NORM_TEXT);
		return -1;
	}

	/******************************************************************************/
	/* First, verify that (some of) the underlying data structures are consistent */
	if (strcmp(_FastrakMsgList[FASTRAK_LASTCOMMAND].name, "FASTRAK_LASTCOMMAND") != 0) {
		/* NOTE: this generally happens when a new command is added to */
		/*   the _FastrakMsgList[] array, but the corresponding enumerated */
		/*   value is not added to the _FastrakCommand type definition.    */
		vrErrPrintf(RED_TEXT "_PatriotInitializeDevice(): The Fastrak device source code is not consistent -- '_FastrakMsgList[]'\n" NORM_TEXT);
		return(-1);
	}

	aux->model = MODEL_PATRIOT;	/* default setting */

	/****************************************/
	/* Make initial contact with the Device */

#if 0 /* TODO: this seems to cause the Patriot to get confused on future inputs -- invalid parameter gets reported */
	vrPrintf("_PatriotInitializeDevice(): Sending a warm reset signal to the Patriot.\n", name);
	_FastrakSendCommand(aux, PATRIOT_WARM_RESET);
	vrSleep(4000000);				/* give time for the response */ /* NOTE: was 400000 */
#endif

	/* Go into "polled mode" for communicating with the device */
	vrPrintf("_PatriotInitializeDevice(): Patriot Device '%s': Disabling Echo\n", name);
	_FastrakSendCommand(aux, PATRIOT_SET_ECHO_MODE);

	/* Go into "polled mode" for communicating with the device */
	vrPrintf("_PatriotInitializeDevice(): Patriot Device '%s': Going into 'polled mode'\n", name);
	_FastrakSendCommand(aux, PATRIOT_POLLMODE);

	/* go into ASCII mode */
	_FastrakSendCommand(aux, PATRIOT_ASCIIMODE);

	/* go into Inches mode */
	_FastrakSendCommand(aux, PATRIOT_SET_INCHES);

	/* get system information */
	_FastrakSendCommand(aux, PATRIOT_SYSSTAT);	/* NOTE: the parsing of this command will cause the Patriot Wireless to be distinguished from the Wired model (ie. aux->model will be set to MODEL_PATRIOTWL) */

	_FastrakSendCommand(aux, FASTRAK_POLL); /* NOTE: I'm sending a poll here because sometimes the Patriot gets into a messed up mode where it needs one extra character */

	/* send any initialization commands from the FreeVR configuration */
	if (aux->init_command != NULL) {
		_FastrakSendCommandString(aux, aux->init_command);
	}

	/* We need to parse the response to the above commands in order to know */
	/* what type of Patriot we're communicating with -- Wired vs. Wireless. */
	while (_FastrakDecodeRecord(aux)) {  } /* get all the incoming data */
	_FastrakReadInput(aux); /* [NOTE: we're not making use of the return value.] */

vrPrintf("Fastrak model is %d (?? patriotwl is %d)\n", aux->model, MODEL_PATRIOTWL);
	/* Wireless should launch the markers related to the receptor [TODO: don't hard code to single/dual receptor] */
	if (aux->model == MODEL_PATRIOTWL) {
#if 0 /* 1 = single receptor */
		vrPrintf("_PatriotInitializeDevice(): Patriot Device '%s': Launching receptor'\n", name);
		_FastrakSendCommand(aux, PATRIOTWL_LAUNCH_RECEPTOR);
		_FastrakSendCommand(aux, FASTRAK_POLL); /* NOTE: I'm sending a poll here because sometimes the Patriot gets into a messed up mode where it needs one extra character */
		vrSleep(40000);				/* give time for the responses */
#else /* the dual receptor mode */
		vrPrintf("_PatriotInitializeDevice(): Patriot Device '%s': Launching multi-receptor setup'\n", name);
		_FastrakSendCommand(aux, PATRIOTWL_AUTOLAUNCH_RECEPTOR);
		_FastrakSendCommand(aux, FASTRAK_POLL); /* NOTE: I'm sending a poll here because sometimes the Patriot gets into a messed up mode where it needs one extra character */
		vrSleep(400000);				/* give time for the responses */
#endif
	}


	/****************************************************************/
	/* request data from device necessary for proper initialization */
	_FastrakSendCommand(aux, PATRIOT_GET_ALLRECEIVERS);
	_FastrakSendCommand(aux, FASTRAK_POLL); /* NOTE: I'm sending a poll here because sometimes the Patriot gets into a messed up mode where it needs one extra character */

	vrSleep(40000);				/* give time for the responses */

	/**********************************************************/
	/* parse all the incoming responses to the above requests */

	/* TODO: Since we know the "get receivers" command was the last sent, */
	/*   we continue to decode until that response has been parsed. */

	while (!aux->got_state) {
		while (_FastrakDecodeRecord(aux)) {  }
		_FastrakReadInput(aux); /* [NOTE: we're not making use of the return value.] */
	}	/* get all the incoming data */

	/****************************************/
	/* Set the parameters for all the units */

	/* count the number of active units */
	aux->num_units = 0;
	for (unit_count = 0; unit_count < MAX_UNITS; unit_count++) {
		if (aux->units[unit_count].state) {
			aux->num_units++;
		}
#if defined(COMM_DEBUG) || 1 /* TODO: set this to 0 when done debugging */
		vrPrintf("_PatriotInitializeDevice(): Patriot unit %d state is %d, %d units so far.\n", unit_count+1, aux->units[unit_count].state, aux->num_units);
#endif
	}

	for (unit_count = 0; unit_count < aux->num_units; unit_count++) {
		/* TODO: perhaps only send to units known to exist -- though shouldn't hurt to send to all */
vrPrintf("Initializing Unit %d\n", unit_count+1);


		/* set type of data to report */
		if (aux->units[unit_count].has_button)
			_FastrakSendCommandAddr(aux, PATRIOT_SET_OUTPUT_POSITION_ANGLES_B, unit_count+1);
		else	_FastrakSendCommandAddr(aux, PATRIOT_SET_OUTPUT_POSITION_ANGLES, unit_count+1);
		_FastrakSendCommand(aux, FASTRAK_POLL); /* NOTE: I'm sending a poll here because sometimes the Patriot gets into a messed up mode where it needs one extra character */

		/* set hemisphere to use */
		if (aux->model != MODEL_PATRIOTWL) {
			/* there is no hemisphere command for the Patriot Wireless */
			_FastrakSendCommandAddr(aux, aux->hemisphere, unit_count+1);
		}
	}

	/* make sure to request enough information from the system to properly decode input */
	/* NOTE: since we're setting the params, we could just know this in advance, but */
	/*   it's cleaner to cover even the times when we just make use of the existing  */
	/*    parameters.                                                                */
	for (unit_count = 0; unit_count < aux->num_units; unit_count++) {
vrPrintf("Getting Station params for unit %d (state = %d)\n", unit_count+1, aux->units[unit_count].state);
		if (aux->units[unit_count].state) {
			_FastrakSendCommandAddr(aux, FASTRAK_GET_STATION_PARAMS, unit_count+1);
			_FastrakSendCommand(aux, FASTRAK_POLL); /* NOTE: I'm sending a poll here because sometimes the Patriot gets into a messed up mode where it needs one extra character */
#if defined(COMM_DEBUG) || 1 /* TODO: set this to 0 when done debugging */
			vrPrintf("_PatriotInitializeDevice(): Sent request for station parameters for station %d\n", unit_count+1);
#endif
		}
	}

	/****************************************/
	/* Create the version and param strings */
		/* TODO: this */


	/*****************************/
	/* Set the system parameters */
		/* TODO: set for binary mode for example */



	/***************************************/
	/* Go into stream mode -- if specified */
	if (aux->stream_mode) {
		/* Set the report rate */
		/* TODO: is there a report-rate command? */

		/* Go into stream mode */
		/* TODO: this */
	}


	/************************/
	/* read the first input data */

	/* send request for data when in polled mode */
	if (!aux->stream_mode) {
		_FastrakSendCommand(aux, FASTRAK_POLL);
	}
	_FastrakReadInput(aux);		/* NOTE: also decodes as much as possible */

	return 0;
}


/******************************************************/
static int _FastrakResetDevice(_FastrakPrivateInfo *aux)
{
	if (aux == NULL)
		return -1;

	/* TODO: figure out the proper reset command sequence */

}


/******************************************************/
static int _FastrakCloseDevice(_FastrakPrivateInfo *aux)
{
	if (aux == NULL)
		return -1;

	/* TODO: figure out the proper pre-close command sequence */

	/* close the serial port */
	vrSerialClose(aux->fd);
}



/******************************************************/
/* NOTE: these names are probably specific to the IS-900, and */
/*   so we need to check the name of the device before using  */
/*   particular names. */
static unsigned int _FastrakButtonValue(char *buttonname)
{
	switch (buttonname[0]) {
	case 'y':	/* yellow button */
	case 'Y':
	case '1':
		return Fastrak_BUTTONINDEX_x1x;
	case 'r':	/* red button */
	case 'R':
	case '2':
		return Fastrak_BUTTONINDEX_x2x;
	case 'g':	/* green button */
	case 'G':
	case '3':
		return Fastrak_BUTTONINDEX_x3x;
	case 'b':	/* blue button */
	case 'B':
	case '4':
		return Fastrak_BUTTONINDEX_x4x;
	case 'j':	/* joystick button */
	case 'J':
	case '5':
		return Fastrak_BUTTONINDEX_x5x;
	case 't':	/* trigger button */
	case 'T':
	case '6':
		return Fastrak_BUTTONINDEX_x6x;
	case 'u':	/* a pseudo "point-straight-up" button */
	case 'U':
	case '7':
		return Fastrak_BUTTONINDEX_x7x;
	}

	return -1;
}


/******************************************************/
/* Translate a string name of a valuator (the "instance" config) into a numeric value */
static unsigned int _FastrakValuatorValue(char *valuatorname)
{
	/* skip an opening sign character */
	if (valuatorname[0] == '-' || valuatorname[0] == '+')
		valuatorname++;

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
		break;
	/* Joystick, X and Y */
	case 'x':
	case 'X':
		return Fastrak_VALUATORINDEX_X;
	case 'y':
	case 'Y':
		return Fastrak_VALUATORINDEX_Y;

	/* Tracking qualities */
	case 'q':
	case 'Q':
		return Fastrak_VALUATORINDEX_TRACKINGQUALITY;
	case 'm':
	case 'M':
		return Fastrak_VALUATORINDEX_TRACKINGMEASURE;
	case 'r':
	case 'R':
		return Fastrak_VALUATORINDEX_TRACKINGREJECT;
	default:
		return -1;
	}

	return -1;
}


	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/


#if defined(FREEVR) /* { */
/*****************************************************************/
/*** Functions for FreeVR access of Fastrak devices for input ***/
/*****************************************************************/


	/*************************************/
	/***  FreeVR NON public routines   ***/
	/*************************************/


/*********************************************************/
static void _FastrakParseArgs(_FastrakPrivateInfo *aux, char *args)
{
static	char	*hemi_choices[] = { "fore", "aft", "right", "left", "lower", "upper" };
static	int	hemi_values[] = {
			FASTRAK_SET_HEMISPHERE_FORE,
			FASTRAK_SET_HEMISPHERE_AFT,
			FASTRAK_SET_HEMISPHERE_RIGHT,
			FASTRAK_SET_HEMISPHERE_LEFT,
			FASTRAK_SET_HEMISPHERE_LOWER,
			FASTRAK_SET_HEMISPHERE_UPPER  };
	int	null_value = -1;
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

	/*****************************************************/
	/** Argument format: "command" "=" <command string> **/
	/*****************************************************/
	vrArgParseString(args, "command", &(aux->init_command));
#if 0
	vrPrintf(RED_TEXT "_FastrakParseArgs(): init_command is '%s'\n", aux->init_command);
#endif
	aux->init_command = vrProcessString(aux->init_command);
#if 0
	vrPrintf(RED_TEXT "_FastrakParseArgs(): init_command is now: '%s'\n", aux->init_command);
#endif

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


	/** TODO: other arguments to parse go here **/
}


/**************************************************************************/
/* The _FastrakGetData(): function calls the _FastrakReadInput() function */
/*   to get the latest data, and then puts that data into the FreeVR data */
/*   structures.  I.e. this is the FreeVR-specific portion of the data    */
/*   parsing.                                                             */
static void _FastrakGetData(vrInputDevice *devinfo)
{
	_FastrakPrivateInfo	*aux = (_FastrakPrivateInfo *)devinfo->aux_data;
	int			count;
	float			scale_trans = aux->scale_trans;
	int			buttons[3];
static	int			buttons_last = 0;
	float			values[6];

	/*******************/
	/* gather the data */

	/* send request for data when in polled mode */
	if (!aux->stream_mode) {
		_FastrakSendCommand(aux, FASTRAK_POLL);
#ifdef STATUS_POLL
		if (aux->manufacturer == MANU_INTERSENSE) {
			_FastrakSendCommand(aux, IS900_GET_TRACKING_STATUS);
		}
		if (aux->model == MODEL_PATRIOTWL) {
			_FastrakSendCommand(aux, PATRIOTWL_GET_SIGNAL_STATUS);
		}
#endif
	}
	_FastrakReadInput(aux);			/* Make sure we have the latest data stored in the "_FastrakPrivateInfo" structure [NOTE: we're not making use of the return value.] */

	/*************/
	/** buttons **/
	if (aux->button_change) {
		for (count = 0; count < aux->num_buttons; count++) {
			vr2switch	*current_2switch;
			_FastrakUnit	*current_ftunit;
			int		mapped_6sensor;
			int		new_value;

			current_2switch = aux->button_inputs[count];
			current_ftunit = &(aux->units[aux->button_map_unit[count]]);
			mapped_6sensor = aux->button_map_unit[count];

			if (current_ftunit == NULL) {
				vrErrPrintf("_FastrakGetData(): Attempt to read button from a non existent FastrakUnit (%d)\n", aux->button_map_unit[count]);
			} else {
				unsigned short	button_value = 1 << aux->button_map_button[count];

				new_value = ((current_ftunit->buttons & button_value) != 0);
				vrDbgPrintfN(FASTRK_DBGLVL, "_FastrakGetData(): Hey, current_ftunit->buttons = 0x%02x (masked: 0x%02x), button_map_button = %d, value = 0x%02x, new_value = %d, old = %d\n",
					current_ftunit->buttons,
					(current_ftunit->buttons & button_value),
					aux->button_map_button[count],
					button_value,
					new_value,
					aux->button_values[count]);

				/* compare with value on file, and if changed then assign the new value to the input */
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
							vrErrPrintf(RED_TEXT "_FastrakGetData(): Unable to handle button inputs that aren't Binary or Control inputs\n" NORM_TEXT);
							break;
						}
					}
				}
			}
		}

		aux->button_change = 0;
	}

	/***************/
	/** valuators **/
	if (aux->valuator_change /* TODO: 02/25/2008: adding the following to force checking for the purpose of tracking quality numbers */ || 1) {
		for (count = 0; count < aux->num_valuators; count++) {
			vrValuator	*current_valuator;
			_FastrakUnit	*current_ftunit;
			int		mapped_6sensor;
			int		new_int_value;
			int		valuator_map;
			float		new_value;

			current_valuator = aux->valuator_inputs[count];
			current_ftunit = &(aux->units[aux->valuator_map_unit[count]]);
			mapped_6sensor = aux->valuator_map_unit[count];
			valuator_map = aux->valuator_map_valuator[count];

			if (current_ftunit == NULL) {
				vrErrPrintf("_FastrakGetData(): Attempt to read valuator from a non existent FastrakUnit (%d)\n", aux->valuator_map_unit[count]);
			} else {
				switch (valuator_map) {
				case Fastrak_VALUATORINDEX_TRACKINGQUALITY:
					new_int_value = current_ftunit->tracking_quality/2 + 127;
#if 0
printf("tq = %d\n", new_int_value);
#endif
					break;
				case Fastrak_VALUATORINDEX_TRACKINGMEASURE:
					new_int_value = current_ftunit->tracking_meas*4 + 127;
#if 0
printf("tm = %d\n", new_int_value);
#endif
					break;
				case Fastrak_VALUATORINDEX_TRACKINGREJECT:
					new_int_value = current_ftunit->tracking_reject*4 + 127;
#if 0
printf("tr = %d\n", new_int_value);
#endif
					break;
				default:
					new_int_value = current_ftunit->joystick[valuator_map];
					break;
				}
				if (new_int_value != aux->valuator_values[count] /* TODO: 02/25/2008 -- temporary */ || 1) {
					/* we have a new value */
					aux->valuator_values[count] = new_int_value;

					new_value = ((float)(new_int_value - 127) * aux->valuator_sign[count]) / 127.0;
					if (new_value > 1.0)
						new_value = 1.0;
					if (new_value < -1.0)
						new_value = -1.0;

					vrDbgPrintfN(FASTRK_DBGLVL, "_FastrakGetData(): current_ftunit->j[x,y] = %d,%d, valuator_map:unit = %d, valuator = %d, sign = %.1f, values (old) = %d, new_value = %.3f\n",
						current_ftunit->joystick[0], current_ftunit->joystick[1],
						aux->valuator_map_unit[count],
						aux->valuator_map_valuator[count],
						aux->valuator_sign[count],
						aux->valuator_values[count],
						new_value);

					switch (aux->valuator_inputs[count]->input_type) {
					case VRINPUT_VALUATOR:
						vrAssignValuatorValue(current_valuator, new_value);
						break;
					case VRINPUT_CONTROL:
						vrCallbackInvokeDynamic(((vrControl *)(current_valuator))->callback, 1, &new_value);
						break;
					default:
						vrErrPrintf(RED_TEXT "_FastrakGetData(): Unable to handle valuator inputs that aren't Floating or Control inputs\n" NORM_TEXT);
						break;
					}
				}
			}
		}

		aux->valuator_change = 0;
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

		/* Make the assignment only when the error flag is off */
		/* NOTE: we're using the tracking_status field to decide if there's an error */
		if (aux->units[mapped_6sensor].tracking_status == 'X') {
			vrAssign6sensorErrorValue(current_6sensor, 1);
		} else {
			vrAssign6sensorValue(current_6sensor, vrMatrixSetEulerAzimaxis(&tmpmat, &new_value, VR_Z), 0 /*, TODO: timestamp */);
			vrAssign6sensorErrorValue(current_6sensor, 0);
		}
		vrAssign6sensorActiveValue(current_6sensor, aux->units[mapped_6sensor].state);
		aux->receiver_change = 0;		/* NOTE: we're not making use of this at the moment */
	}
}


	/****************************************************************/
	/*    Function(s) for parsing Fastrak "input" declarations.    */
	/*                                                              */
	/*  These _DEVICE<type>Input() functions are called during the  */
	/*  CREATE phase of the input interface.                        */

/**************************************************************************/
static vrInputMatch _FastrakButtonInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
static	char		*whitespace = " \t\r\b\n";
	_FastrakPrivateInfo	*aux = (_FastrakPrivateInfo *)devinfo->aux_data;
	char		*button_name_instance;
	int		button_num;
	int		unit_number;
	int		button_choice;

	vrDbgPrintfN(FASTRK_DBGLVL, "_FastrakButtonInput(): instance = '%s'\n" NORM_TEXT, dti->instance);

	button_num = aux->num_buttons;
	aux->num_buttons++;
	unit_number = vrAtoI(dti->instance);
	button_name_instance = strchr(dti->instance, ',') + 1;			/* jump past comma */

	if (button_name_instance != NULL) {
		button_name_instance += strspn(button_name_instance, whitespace);/* skip white */
		button_choice = _FastrakButtonValue(button_name_instance);
	} else {
		button_choice = -1;
	}

	/*************************************************/
	/* verify that the requested 'instance' is valid */
	if (button_choice == -1)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_FastrakButtonInput(): Warning, button['%s'] did not match any known button\n", dti->instance);
	else if (unit_number < 1 || unit_number > MAX_UNITS)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_FastrakButtonInput(): Warning, sensor number must be between %d and %d\n", 1, MAX_UNITS); /* TODO: determine better (more specific) last argument */
	else {
		aux->units[unit_number-1].has_button = 1;
		if (aux->button_inputs[button_num] != NULL)
			vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_FastrakButtonInput(): Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);
	}

	vrDbgPrintfN(INPUT_DBGLVL, "_FastrakButtonInput(): assigned button #%d -- type %d of sensor %d\n",
		button_num, button_choice, unit_number);

	/***********************/
	/* now setup the input */
	aux->button_inputs[button_num] = (vr2switch *)input;
	aux->button_map_unit[button_num] = unit_number - 1;	/* change to 1-based unit code */
	aux->button_map_button[button_num] = button_choice;
	aux->button_values[button_num] = 0;	/* TODO: should vrAssign2switchValue() be used here?  probably */

	vrDbgPrintfN(INPUT_DBGLVL, "_FastrakButtonInput(): assigned button event of value 0x%02x to input pointer = %#p)\n",
		button_num, aux->button_inputs[button_num]);

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/**************************************************************************/
static vrInputMatch _FastrakValuatorInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
static	char		*whitespace = " \t\r\b\n";
        _FastrakPrivateInfo	*aux = (_FastrakPrivateInfo *)devinfo->aux_data;
	char			*valuator_name_instance;
	int			valuator_num;
	int			unit_number;
	int			valuator_choice;

	vrDbgPrintfN(FASTRK_DBGLVL, "_FastrakValuatorInput(): instance = '%s'\n" NORM_TEXT, dti->instance);

	valuator_num = aux->num_valuators;
	aux->num_valuators++;
	unit_number = vrAtoI(dti->instance);
	valuator_name_instance = strchr(dti->instance, ',') + 1;			/* jump past comma*/

	if (valuator_name_instance != NULL) {
		valuator_name_instance += strspn(valuator_name_instance, whitespace);/* skip white */
		valuator_choice = _FastrakValuatorValue(valuator_name_instance);
	} else {
		valuator_choice = -1;
	}

	/*************************************************/
	/* verify that the requested 'instance' is valid */
	if (valuator_choice == -1)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_FastrakValuatorInput(): Warning, valuator['%s'] did not match any known valuator\n", dti->instance);
	else if (unit_number < 1 || unit_number > MAX_UNITS)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_FastrakValuatorInput(): Warning, sensor number must be between %d and %d\n", 1, MAX_UNITS); /* TODO: determine better (more specific) last argument */
	else {
		aux->units[unit_number-1].has_button = 1;
		if (aux->valuator_inputs[valuator_num] != NULL)
			vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_FastrakValuatorInput(): Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);
	}

	/***********************/
	/* now setup the input */
	aux->valuator_inputs[valuator_num] = (vrValuator *)input;
	aux->valuator_map_unit[valuator_num] = unit_number - 1;	/* change to 1-based unit code */
	aux->valuator_map_valuator[valuator_num] = valuator_choice;
	aux->valuator_values[valuator_num] = 0;	/* TODO: should vrAssignValuatorValue() be used here?  probably */
	if (dti->instance[0] == '-')
		aux->valuator_sign[valuator_num] = -1.0;
	else	aux->valuator_sign[valuator_num] =  1.0;

	vrDbgPrintfN(INPUT_DBGLVL, "_FastrakValuatorInput(): assigned valuator event of value 0x%02x to input pointer = %#p)\n",
		valuator_num, aux->valuator_inputs[valuator_num]);

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/**************************************************************************/
static vrInputMatch _FastrakReceiverInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
        _FastrakPrivateInfo	*aux = (_FastrakPrivateInfo *)devinfo->aux_data;
	int			sensor_num;
	int			unit_number;

	sensor_num = aux->num_6sensors;
	aux->num_6sensors++;
	unit_number = vrAtoI(dti->instance);

	/*************************************************/
	/* verify that the requested 'instance' is valid */
	if (unit_number < 1)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_FastrakReceiverInput(): Warning, sensor number must be between %d and %d\n", 1, MAX_UNITS);
	else if (sensor_num > MAX_UNITS)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_FastrakReceiverInput(): Warning, sensor number must be between %d and %d\n", 1, MAX_UNITS);
	else if (aux->sensor6_inputs[sensor_num] != NULL) {
		printf("sensor_num is now %d\n", sensor_num);
		printf("aux->6sensor_inputs = %p\n", aux->sensor6_inputs);
		if (aux->sensor6_inputs[sensor_num] != NULL) {
			printf("hey\n");
			printf("type is '%s'\n", dti->type);
			printf("instance is '%s'\n", dti->instance);
			vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_FastrakReceiverInput(): Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
				dti->type, dti->instance);
printf("old address = %p\n", aux->sensor6_inputs[sensor_num]);
		}
	}

	/***********************/
	/* now setup the input */
	aux->sensor6_inputs[sensor_num] = (vr6sensor *)input;
	aux->sensor6_map[sensor_num] = unit_number - 1; /* change from one based (unit_number) to zero based (sensor_num) */

	/* the vrAssign6sensorR2Exform() function does the parsing of the next token */
	vrAssign6sensorR2Exform(aux->sensor6_inputs[sensor_num], strchr(dti->instance, ','));

	vrDbgPrintfN(INPUT_DBGLVL, "_FastrakReceiverInput(): assigned 6sensor event of value 0x%02x to input pointer = %#p)\n",
		unit_number, aux->sensor6_inputs[sensor_num]);

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


	/************************************************************/
	/***************** FreeVR Callback routines *****************/
	/************************************************************/

	/************************************************************/
	/*    Callbacks for controlling the Fastrak features.      */
	/*                                                          */

/************************************************************/
static void _FastrakSystemPauseToggleCallback(vrInputDevice *devinfo, int value)
{
	_FastrakPrivateInfo	*aux = (_FastrakPrivateInfo *)devinfo->aux_data;

        if (value == 0)
                return;

	devinfo->context->paused ^= 1;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Fastrak Control: system_pause = %d.\n",
		devinfo->context->paused);
}

/************************************************************/
static void _FastrakPrintContextStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintContext(stdout, devinfo->context, verbose);
}

/************************************************************/
static void _FastrakPrintConfigStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintConfig(stdout, devinfo->context->config, verbose);
}

/************************************************************/
static void _FastrakPrintInputStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintInput(stdout, devinfo->context->input, verbose);
}

/************************************************************/
static void _FastrakPrintStructCallback(vrInputDevice *devinfo, int value)
{
	_FastrakPrivateInfo	*aux = (_FastrakPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	_FastrakPrintStruct(stdout, aux, verbose);
}

/************************************************************/
static void _FastrakPrintHelpCallback(vrInputDevice *devinfo, int value)
{
	_FastrakPrivateInfo	*aux = (_FastrakPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	_FastrakPrintHelp(stdout, aux);
}



	/************************************************************/
	/*    Callbacks for interfacing with the Fastrak device.    */
	/*                                                          */


/************************************************************/
static void _FastrakCreateFunction(vrInputDevice *devinfo)
{
	/*** List of possible inputs ***/
static	vrInputFunction	_FastrakInputs[] = {
				{ "",		VRINPUT_2WAY, _FastrakButtonInput },
				{ "button",	VRINPUT_2WAY, _FastrakButtonInput },

				{ "",		VRINPUT_VALUATOR, _FastrakValuatorInput },
				{ "joy",	VRINPUT_VALUATOR, _FastrakValuatorInput },
				{ "joystick",	VRINPUT_VALUATOR, _FastrakValuatorInput },
				{ "valuator",	VRINPUT_VALUATOR, _FastrakValuatorInput },

				{ "",		VRINPUT_6SENSOR, _FastrakReceiverInput },
				{ "receiver",	VRINPUT_6SENSOR, _FastrakReceiverInput },

				{ NULL,		VRINPUT_UNKNOWN, NULL } };
	/*** List of control functions ***/
static	vrControlFunc	_FastrakControlList[] = {
				/* overall system controls */
				{ "system_pause_toggle", _FastrakSystemPauseToggleCallback },

				/* informational output controls */
				{ "print_context", _FastrakPrintContextStructCallback },
				{ "print_config", _FastrakPrintConfigStructCallback },
				{ "print_input", _FastrakPrintInputStructCallback },
				{ "print_struct", _FastrakPrintStructCallback },
				{ "print_help", _FastrakPrintHelpCallback },

		/** TODO: other callback control functions go here **/
				/* simulated 6-sensor selection controls */
				/* simulated 6-sensor manipulation controls */
				/* other controls */

				/* end of the list */
				{ NULL, NULL } };

	_FastrakPrivateInfo	*aux = NULL;

	/******************************************/
	/* allocate and initialize auxiliary data */
	devinfo->aux_data = (void *)vrShmemAlloc0(sizeof(_FastrakPrivateInfo));
	aux = (_FastrakPrivateInfo *)devinfo->aux_data;
	_FastrakInitializeStruct(aux);

	/******************/
	/* handle options */
	aux->port = vrShmemStrDup(DEFAULT_PORT);	/* default, if no port given */
	aux->baud_int = DEFAULT_BAUD;			/* default, if no baud given */
	aux->baud_enum = vrSerialBaudIntToEnum(aux->baud_int);
	_FastrakParseArgs(aux, devinfo->args);

	/***************************************/
	/* create the inputs and self-controls */
	/* Because a Fastrak type device can have a large unknown number of inputs,  we allocate  */
	/*   memory for handling them here.  First the vrInputCreateDataContainers() function     */
	/*   is called to determine how many of each type of input needs to be created.  Normally */
	/*   vrInputCreateDataContainers() is called by vrInputCreateDataContainers() (see below),*/
	/*   but for this circumstance we split the two operations.                               */
	vrInputCountDataContainers(devinfo);

	aux->num_buttons = 0;		/* NOTE: this gets incremented in _FastrakButtonInput() */
	aux->button_inputs = (vr2switch **)vrShmemAlloc0((devinfo->num_2ways + devinfo->num_scontrols) * sizeof(vr2switch *));
	aux->button_map_unit = (int *)vrShmemAlloc0((devinfo->num_2ways + devinfo->num_scontrols) * sizeof(int));
	aux->button_map_button = (int *)vrShmemAlloc0((devinfo->num_2ways + devinfo->num_scontrols) * sizeof(int));
	aux->button_values = (int *)vrShmemAlloc0((devinfo->num_2ways + devinfo->num_scontrols) * sizeof(int));

	aux->num_valuators = 0;		/* NOTE: this gets incremented in _FastrakValuatorInput() */
	aux->valuator_inputs = (vrValuator **)vrShmemAlloc0((devinfo->num_valuators + devinfo->num_scontrols) * sizeof(vrValuator *));
	aux->valuator_map_unit = (int *)vrShmemAlloc0((devinfo->num_valuators + devinfo->num_scontrols) * sizeof(int));
	aux->valuator_map_valuator = (int *)vrShmemAlloc0((devinfo->num_valuators + devinfo->num_scontrols) * sizeof(int));
	aux->valuator_sign = (float *)vrShmemAlloc0((devinfo->num_valuators + devinfo->num_scontrols) * sizeof(float));
	aux->valuator_values = (int *)vrShmemAlloc0((devinfo->num_valuators + devinfo->num_scontrols) * sizeof(int));

	aux->num_6sensors = 0;		/* NOTE: this gets incremented in _FastrakReceiverInput() */
	aux->sensor6_inputs = (vr6sensor **)vrShmemAlloc0(devinfo->num_6sensors * sizeof(vr6sensor *));
#if 0
	aux->sensor6_values = (vrEuler *)vrShmemAlloc0(devinfo->num_6sensors * sizeof(vrEuler));
#endif
	aux->sensor6_map = (int *)vrShmemAlloc0(devinfo->num_6sensors * sizeof(int));

	/* Here we return to the conventional way of creating the inputs */
	vrInputCreateDataContainers(devinfo, _FastrakInputs);
	vrInputCreateSelfControlContainers(devinfo, _FastrakInputs, _FastrakControlList);
	/* TODO: anything to do for NON self-controls?  Implement for Magellan first. */

	vrDbgPrintf(BOLD_TEXT "_FastrakCreateFunction(): Done creating Fastrak inputs for '%s'\n" NORM_TEXT, devinfo->name);
	devinfo->created = 1;

	return;
}


/************************************************************/
static void _FastrakOpenFunction(vrInputDevice *devinfo)
{
	vrTrace("_FastrakOpenFunction", devinfo->name);

	_FastrakPrivateInfo	*aux = (_FastrakPrivateInfo *)devinfo->aux_data;

	/*******************/
	/* open the device */
	aux->fd = vrSerialOpen(aux->port, aux->baud_enum);
	if (aux->fd < 0) {
		aux->open = 0;
		vrErrPrintf("(%s::_FastrakOpenFunction::%d) error: "
			RED_TEXT "couldn't open serial port %s for %s\n" NORM_TEXT,
			__FILE__, __LINE__, aux->port, devinfo->name);
		sprintf(aux->version, "- unconnected Fastrak -");
	} else {
		aux->open = 1;
		if (!strcasecmp(devinfo->type, "patriot")) {
			if (_PatriotInitializeDevice(aux, devinfo->name) < 0) {
				vrErrPrintf("_PatriotOpenFunction(): "
					RED_TEXT "Warning, unable to initialize '%s' Patriot.\n" NORM_TEXT,
					devinfo->name);
			}
		} else if (_FastrakInitializeDevice(aux, devinfo->name) < 0) {
			vrErrPrintf("_FastrakOpenFunction(): "
				RED_TEXT "Warning, unable to initialize '%s' Fastrak.\n" NORM_TEXT,
				devinfo->name);
		} else {
			devinfo->operating = 1;
			vrDbgPrintf("_FastrakOpenFunction(): Done opening Fastrak input device '%s'.\n", devinfo->name);
		}
	}

	return;
}


/************************************************************/
static void _FastrakCloseFunction(vrInputDevice *devinfo)
{
	_FastrakPrivateInfo	*aux = (_FastrakPrivateInfo *)devinfo->aux_data;

	if (aux != NULL) {
		_FastrakCloseDevice(aux);
		vrSerialClose(aux->fd);
		vrShmemFree(aux);	/* aka devinfo->aux_data */
	}

	return;
}


/************************************************************/
static void _FastrakResetFunction(vrInputDevice *devinfo)
{
	_FastrakPrivateInfo	*aux = (_FastrakPrivateInfo *)devinfo->aux_data;

	if (aux != NULL) {
		_FastrakResetDevice(aux);
	}

	return;
}


/************************************************************/
static void _FastrakPollFunction(vrInputDevice *devinfo)
{
#if 1
	_FastrakPrivateInfo	*aux = (_FastrakPrivateInfo *)devinfo->aux_data;

	if (vrDbgDo(FASTRK_DBGLVL)) {
static		int	first_time = 1;
		if (first_time) {
			_FastrakPrintStruct(stdout, aux, verbose);
			first_time = 0;
		}
	}
#endif
	if (devinfo->operating) {
		_FastrakGetData(devinfo);
	} else {
		/* TODO: try to open the device again */
	}

	return;
}



	/******************************/
	/*** FreeVR public routines ***/
	/******************************/


/******************************************************/
void vrFastrakInitInfo(vrInputDevice *devinfo)
{
	devinfo->version = (char *)vrShmemStrDup("-get from Fastrak device-");
	devinfo->Create = vrCallbackCreateNamed("Fastrak:Create-Def", _FastrakCreateFunction, 1, devinfo);
	devinfo->Open = vrCallbackCreateNamed("Fastrak:Open-Def", _FastrakOpenFunction, 1, devinfo);
	devinfo->Close = vrCallbackCreateNamed("Fastrak:Close-Def", _FastrakCloseFunction, 1, devinfo);
	devinfo->Reset = vrCallbackCreateNamed("Fastrak:Reset-Def", _FastrakResetFunction, 1, devinfo);
	devinfo->PollData = vrCallbackCreateNamed("Fastrak:PollData-Def", _FastrakPollFunction, 1, devinfo);
	devinfo->PrintAux = vrCallbackCreateNamed("Fastrak:PrintAux-Def", _FastrakPrintStruct, 0);

	vrDbgPrintfN(INPUT_DBGLVL, "vrFastrakInitInfo: callbacks created.\n");
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
static	int	done = 0;

#define	RENDERLINES	0
#define	RENDERCSV	1
#define	RENDERSCREEN	2


/*******************************************************************/
void _RenderValuesLine(_FastrakPrivateInfo *aux)
{
static	int		last_rc = -1;		/* keep track of last read_count total, and don't print if same */
	int		rc_sum;			/* sum of all the receiver read counts */
	int		unit;
	_FastrakUnit	*unitdata;		/* pointer to the current unit */

	rc_sum = 0;
	for (unit = 0; unit < aux->num_units; unit++) {
		rc_sum += aux->units[unit].read_count;
	}
	if (rc_sum == last_rc) {
		vrPrintf(".");
		fflush(stdout);
		return;
	}
	last_rc = rc_sum;

	for (unit = 0; unit < aux->num_units; unit++) {
		unitdata = &(aux->units[unit]);

		if (unitdata->has_button) {
			vrPrintf("   Rec %d [%3d(%c%X%X)]: bj=%02x,%3d,%3d %6.1f %6.1f %6.1f %6.1f %6.1f %6.1f",
				unit+1,
				unitdata->tracking_quality,
				unitdata->tracking_status,
				unitdata->tracking_meas,
				unitdata->tracking_reject,
				unitdata->buttons,
				unitdata->joystick[0],
				unitdata->joystick[1],
				unitdata->data_pos[0],	/* X */
				unitdata->data_pos[1],	/* Y */
				unitdata->data_pos[2],	/* Z */
				unitdata->data_pos[3],	/* AZIM */
				unitdata->data_pos[4],	/* ELEV */
				unitdata->data_pos[5]);	/* ROLL */
		} else {
			vrPrintf("   Rec %d [%3d(%c%X%X)]: %6.1f %6.1f %6.1f %6.1f %6.1f %6.1f",
				unit+1,
				unitdata->tracking_quality,
				unitdata->tracking_status,
				unitdata->tracking_meas,
				unitdata->tracking_reject,
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


/**********************************************************************/
/* Render the Tracking values in a comma-separated-value (CSV) format */
void _RenderValuesCSV(_FastrakPrivateInfo *aux)
{
static	int		last_rc = -1;		/* keep track of last read_count total, and don't print if same */
	int		rc_sum;			/* sum of all the receiver read counts */
	int		unit;
	_FastrakUnit	*unitdata;		/* pointer to the current unit */

	rc_sum = 0;
	for (unit = 0; unit < aux->num_units; unit++) {
		rc_sum += aux->units[unit].read_count;
	}
	if (rc_sum == last_rc) {
		vrPrintf(".");
		fflush(stdout);
		return;
	}
	last_rc = rc_sum;

	for (unit = 0; unit < aux->num_units; unit++) {
		unitdata = &(aux->units[unit]);

		if (unitdata->has_button) {
			/* Tracker has buttons and joystick */
			vrPrintf("%d,%3d,%d,%c,%d,%d,%02x,%3d,%3d,%6.1f,%6.1f,%6.1f,%6.1f,%6.1f,%6.1f,  ",
				unit+1,
				unitdata->read_count,
				unitdata->tracking_quality,
				unitdata->tracking_status,
				unitdata->tracking_meas,
				unitdata->tracking_reject,
				unitdata->buttons,
				unitdata->joystick[0],
				unitdata->joystick[1],
				unitdata->data_pos[0],	/* X */
				unitdata->data_pos[1],	/* Y */
				unitdata->data_pos[2],	/* Z */
				unitdata->data_pos[3],	/* AZIM */
				unitdata->data_pos[4],	/* ELEV */
				unitdata->data_pos[5]);	/* ROLL */
		} else {
			/* The no-button on tracker case */
			vrPrintf("%d,%3d,%d,%c,%d,%d,ff,-999,-999,%6.1f,%6.1f,%6.1f,%6.1f,%6.1f,%6.1f,  ",
				unit+1,
				unitdata->read_count,
				unitdata->tracking_quality,
				unitdata->tracking_status,
				unitdata->tracking_meas,
				unitdata->tracking_reject,
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
/* TODO: make "max_trans" settable via environment variable.      */
void _RenderValuesScreen(_FastrakPrivateInfo *aux)
{
	int		unit;
	_FastrakUnit	*unitdata;		/* pointer to the current unit */

	int		count;

	int		col_width = 15;		/* the number of characters per column */
	int		col1_offset = 5;	/* the offset before the first column */
	int		num_lines = 40;		/* number of lines for showing chart */
	int		num_columns = 100;

#if 1
	float		max_trans = 96.0;	/* 8 feet in one direction (for "extended-range" devices) */
#else
	float		max_trans = 36.0;	/* 3 feet in one direction (good for low-range devices) */
#endif
	float		max_rot = 180.0;	/* 180 degrees max in each direction */
	float		max_qual = 255.0;	/* 0-255 is the tracking quality range */

	int		center = (num_lines-4) / 2.0 + 4;	/* shift for header */
	float		trans_scale = (num_lines-4) * -0.5 / max_trans;
	float		rot_scale = (num_lines-4) * -0.5 / max_rot;
	float		qual_scale = (num_lines-6) * -1.0 / max_qual;


	/* save current cursor position */
	vrPrintf("7");

	/* move to upper left corner of screen */
	vrPrintf("[0;0H");

	/* clear to end of screen */
	vrPrintf("[J");

	for (unit = 0; unit < aux->num_units; unit++) {
		unitdata = &(aux->units[unit]);

		vrPrintf("[4m");	/* Change color to green */

		/* move to top of column and print unit # (underlined) */
		vrPrintf("[2;%dHRec_%d:[%3d(%c%X%X)]", unit*col_width + col1_offset, unit+1,
			unitdata->tracking_quality,	/* or "read_count" since TQ is also graphically displayed */
			unitdata->tracking_status,
			unitdata->tracking_meas,
			unitdata->tracking_reject);


#if 1
		/* print button value (if appropriate) */
		if (unitdata->has_button)
			vrPrintf("[3;%dH (%02x) ", unit*col_width + col1_offset, unitdata->buttons);
#endif

		/* now print info headers */
		vrPrintf("[4;%dHQXYZAER", unit*col_width + col1_offset);
		if (unitdata->has_button) {
			vrPrintf("xy");
		}

		/* print a *zero* line */
		vrPrintf("[%d;%dH-------", center, unit*col_width + col1_offset);
		if (unitdata->has_button) {
			vrPrintf("--");
		}

		vrPrintf("[24m");	/* Change color to default */

		/* print the Q value (tracker quality) */
		vrPrintf("[%d;%dHm", (int)(unitdata->tracking_meas*qual_scale*16)+num_lines-1, unit*col_width + col1_offset);
		vrPrintf("[%d;%dHr", (int)(unitdata->tracking_reject*qual_scale*16)+num_lines-1, unit*col_width + col1_offset);
		vrPrintf("[%d;%dHT", (int)(unitdata->tracking_quality*qual_scale)+num_lines-1, unit*col_width + col1_offset);

		/* print the X value */
		vrPrintf("[%d;%dHX", (int)(unitdata->data_pos[VR_X]*trans_scale)+center, unit*col_width + col1_offset+1);

		/* print the Y value */
		vrPrintf("[%d;%dHY", (int)(unitdata->data_pos[VR_Y]*trans_scale)+center, unit*col_width + col1_offset+2);

		/* print the Z value */
		vrPrintf("[%d;%dHZ", (int)(unitdata->data_pos[VR_Z]*trans_scale)+center, unit*col_width + col1_offset+3);

		/* print the A value */
		vrPrintf("[%d;%dHA", (int)(unitdata->data_pos[VR_AZIM+3]*rot_scale)+center, unit*col_width + col1_offset+4);

		/* print the E value */
		vrPrintf("[%d;%dHE", (int)(unitdata->data_pos[VR_ELEV+3]*rot_scale)+center, unit*col_width + col1_offset+5);

		/* print the R value */
		vrPrintf("[%d;%dHR", (int)(unitdata->data_pos[VR_ROLL+3]*rot_scale)+center, unit*col_width + col1_offset+6);

		/* print the joystick values */
		if (unitdata->has_button) {
			vrPrintf("[%d;%dHx", (int)((unitdata->joystick[VR_X]+1)*qual_scale)+num_lines-1, unit*col_width + col1_offset+7);
			vrPrintf("[%d;%dHy", (int)((unitdata->joystick[VR_Y]+1)*qual_scale)+num_lines-1, unit*col_width + col1_offset+8);
		}
#if 0
if (unit == 0) {
vrPrintf("[%d;%dH Tracking quality for unit 0 is %d, vert pos is %d = (%d * %f) + num_lines", 30, 50, unitdata->tracking_quality, (int)(unitdata->tracking_quality*qual_scale)+num_lines-1, unitdata->tracking_quality, qual_scale);
vrPrintf("[%d;%dH X value for unit 0 is %f, vert pos is %d = (%f * %f) + %d", 32, 50, unitdata->data_pos[VR_X], (int)(unitdata->data_pos[VR_X]*trans_scale)+center, unitdata->data_pos[VR_X], trans_scale, center);
vrPrintf("[%d;%dH X,Y joystick values for unit 0 are %d,%d, vert pos are %d,%d", 34, 50, unitdata->joystick[0], unitdata->joystick[1], (int)(unitdata->joystick[VR_X]*qual_scale)+num_lines-1, (int)(unitdata->joystick[VR_Y]*qual_scale)+num_lines);
}
#endif
	}

	if (0) {	/* choose to display all the fixed PSE's (or not) */
		for (count = 0; count < aux->num_tunits; count++) {
			vrPrintf("[%d;%dHbeacon %d: pse = %d, loc = %7.4f,%7.4f,%7.4f, norm = %.2f,%.2f,%.2f, id = %d", \
				8+count, 35,
				count+1,
				aux->tunits[count].pse_id,
				aux->tunits[count].position[VR_X],
				aux->tunits[count].position[VR_Y],
				aux->tunits[count].position[VR_Z],
				aux->tunits[count].position[VR_X+3],
				aux->tunits[count].position[VR_Y+3],
				aux->tunits[count].position[VR_Z+3],
				aux->tunits[count].hardware_id);
		}
	}

	if (0) {	/* choose to display a running graph of the tracking quality (or not) */
		static	int	ring1[300];		/* TODO: should have a double array */
		static	int	ring2[300];
		static	int	index = 0;
		int		col_offset = 35;	/* TODO: base this off number of receivers */
		int		num_columns = 240;
		int		col;
		int		col_div = 3;		/* number of data columns per text column */

		/* TODO: use a double array based on number of receivers */
		ring1[index] = aux->units[0].tracking_quality;
		ring2[index] = aux->units[1].tracking_quality;

		for (col = 0; col < num_columns; col++) {
			/* print the quality for receiver-1 */
			vrPrintf("[%d;%dH*", (int)(ring1[col]*qual_scale)+num_lines-1, (col/col_div)+col_offset);
			vrPrintf("[%d;%dHo", (int)(ring2[col]*qual_scale)+num_lines-1, (col/col_div)+col_offset);
		}

		/* redo the current value with numbers so we can see movement */
		vrPrintf("[%d;%dH1", (int)(ring1[index]*qual_scale)+num_lines-1, (index/col_div)+col_offset);
		vrPrintf("[%d;%dH2", (int)(ring2[index]*qual_scale)+num_lines-1, (index/col_div)+col_offset);


		index++;
		index %= num_columns;
	}

	/* return to the original cursor position */
	vrPrintf("8");
	fflush(stdout);
}


/*******************************************************************/
void exit_testapp()
{
	done = 1;
}


/*******************************************************************/
/* A test program to communicate with a Fastrak protocol device and print the results. */
main(int argc, char *argv[])
{
	_FastrakPrivateInfo	*aux;
	int			count;
	int			numevents;
	int			rendermode = RENDERSCREEN;
	int			justonce = 0;		/* quit after just one output ? */
	int			device_fastrak = 1;

	done = 0;
	signal(SIGINT, exit_testapp);


	/******************************/
	/* setup the device structure */
	aux = (_FastrakPrivateInfo *)malloc(sizeof(_FastrakPrivateInfo));
	memset(aux, 0, sizeof(_FastrakPrivateInfo));
	_FastrakInitializeStruct(aux);


	/****************************************************/
	/* adjust parameters based on environment variables */
	aux->port = getenv("FASTRAK_TTY");
	if (aux->port == NULL)
		aux->port = DEFAULT_PORT;		/* default, if no file given */

	if (getenv("FT_BAUD") != NULL)
		aux->baud_int = atoi(getenv("FT_BAUD"));
	if (aux->baud_int == 0)
		aux->baud_int = DEFAULT_BAUD;			/* default, if no baud given */
	aux->baud_enum = vrSerialBaudIntToEnum(aux->baud_int);

	if (getenv("FT_LINERENDER") != NULL) {
		rendermode = RENDERLINES;
	}

	if (getenv("FT_CSVRENDER") != NULL) {
		rendermode = RENDERCSV;
	}

	if (getenv("FT_SCREENRENDER") != NULL) {
		rendermode = RENDERSCREEN;
	}

	/* terminate after acquiring and rendering just one set of inputs */
	if (getenv("FT_JUSTONCE") != NULL)
		justonce = 1;

	/* allow for testing of the patriot device */
	if (getenv("FT_PATRIOT") != NULL)
		device_fastrak = 0;


	/*****************************************************/
	/* adjust parameters based on command-line-arguments */
	/* TODO: parse CLAs for non-default serial port, baud, etc. */


	/**************************************************/
	/* open the serial port and initialize the device */
	/* NOTE: the _FastrakOpenFunction() has too many FreeVR library references to be called from here */
	aux->fd = vrSerialOpen(aux->port, aux->baud_enum);
	if (aux->fd < 0) {
		fprintf(stderr, RED_TEXT "%s: couldn't open serial port %s\n" NORM_TEXT, argv[0], aux->port);
		aux->open = 0;
		sprintf(aux->version, "- unconnected Fastrak -");
	} else {
		if (device_fastrak == 1) {
			/* TODO: avoid this hardcoding w/ environment variables and/or a CLI */
			/* NOTE: this must be set prior to calling _FastrakInitializeDevice(), but we */
 			/*   don't yet know how many units are there, yet.                            */
			aux->units[1].has_button = 1;		/* For now we hardcode that the 2nd unit (unit #1) is a wand with buttons */

			if (_FastrakInitializeDevice(aux, "Fastrak-protocol") < 0) {
				vrErrPrintf("main: " RED_TEXT "Warning, unable to initialize Fastrak.\n" NORM_TEXT);
			}
		} else {
			/* TODO: avoid this hardcoding w/ environment variables and/or a CLI */
			aux->units[0].has_button = 1;

			if (_PatriotInitializeDevice(aux, "Patriot") < 0) {
				vrErrPrintf("main: " RED_TEXT "Warning, unable to initialize Patriot.\n" NORM_TEXT);
			}
		}
		aux->open = 1;
	}

	_FastrakPrintStruct(stdout, aux, verbose);

	/* printout any values that we might already have -- in line mode */
	vrPrintf("Printing out values from initialization phase\n");
	_RenderValuesLine(aux);

	/**********************/
	/* display the output */

	/* for screen-rendering, scroll everything up so it won't be erased */
	if (rendermode == RENDERSCREEN) {
		for (count = 0; count < 60; count++)
			vrPrintf("\n");
vrPrintf("[%d;%dH Test message", 28, 55);
	}

	/* just loop and print values */
	while(aux->open && !done) {
		/* send request for data when in polled mode */
		if (!aux->stream_mode) {
			_FastrakSendCommand(aux, FASTRAK_POLL);
#ifdef STATUS_POLL
			if (aux->manufacturer == MANU_INTERSENSE) {
				_FastrakSendCommand(aux, IS900_GET_TRACKING_STATUS);
			}
			if (aux->model == MODEL_PATRIOTWL) {
				_FastrakSendCommand(aux, PATRIOTWL_GET_SIGNAL_STATUS);
			}
#endif
		}

		_FastrakReadInput(aux); /* [NOTE: we're not making use of the return value.] */

		switch(rendermode) {
		default:
		case RENDERLINES:
			_RenderValuesLine(aux);
			break;
		case RENDERCSV:
			_RenderValuesCSV(aux);
			break;
		case RENDERSCREEN:
			_RenderValuesScreen(aux);
			break;
		}

		/* we're done already if just one printout */
		if (justonce)
			done = 1;
	}

	if (aux != NULL) {
		vrSerialClose(aux->fd);
		free(aux);			/* aka devinfo->aux_data */
	}


	/*****************/
	/* close up shop */
	_FastrakCloseDevice(aux);
	vrPrintf(BOLD_TEXT "\nFastrak device closed\n" NORM_TEXT);
}

#endif /* } TEST_APP */

