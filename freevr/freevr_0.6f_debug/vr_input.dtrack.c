/* ======================================================================
 *
 *  CCCCC          vr_input.dtrack.c
 * CC   CC         Author(s): Bill Sherman
 * CC              Created: November 4, 2010
 * CC   CC         Last Modified: September 15, 2013
 *  CCCCC
 *
 * Code file for FreeVR inputs from the DTrack input device.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/*************************************************************************
USAGE:
	...

	Inputs are specified with the "input" option:
		input "<name>" = "2switch(fs1[<number>]|fs2[<number>])";
		input "<name>" = "valuator(fs2[<number>])"; -- NOTE: the old flysticks had no valuators
		input "<name>" = "6sensor(6body[<number>] {, 'id'|'r2e'|'xform ...'})";
	 :-(	input "<name>" = "6sensor(fs1[<number>] {, 'id'|'r2e'|'xform ...'})";
		input "<name>" = "6sensor(fs2[<number>] {, 'id'|'r2e'|'xform ...'})";
	 :-(	input "<name>" = "6sensor(3body[<number>] {, 'id'|'r2e'|'xform ...'})";
	 :-(	input "<name>" = "6sensor(mt[<number>] {, 'id'|'r2e'|'xform ...'})";
	 :-(	input "<name>" = "6sensor(gl[<number>] {, 'id'|'r2e'|'xform ...'})";
	 :-(	input "<name>" = "Nsensor(glove[<number>])"; -- There are DTrack glove devices, but I do not have access to one
	  XX	input "<name>" = "Nswitch(switch[<number>])"; -- The DTrack has no concept of N-switch

	Controls are specified with the "control" option:
		control "<control option>" = "2switch(...)";
		control "<control option>" = "valuator(...)"; -- NOTE: no valuator oriented controls yet available for this device

	Here are the available control options for FreeVR:
		"print_help" -- print info on how to use the input device
		"system_pause_toggle" -- toggle the system pause flag
		"print_context" -- print the overall FreeVR context data structure (for debugging)
		"print_config" -- print the overall FreeVR config data structure (for debugging)
		"print_input" -- print the overall FreeVR input data structure (for debugging)
		"print_struct" -- print the internal DTrack data structure (for debugging)

	Here are the FreeVR configuration argument options for the DTrack:
		"port" - UDP socket port listening for data servers
			sending data.  (5000 is the default)
		"cmdhost" - machine running the ARTracking DTrack server.
			("localhost" is the default)
			NOTE: this is only necessary for the control features
			which currently aren't used.
		"cmdport" -- port to send control commands (5001 is the default)
		"valscale" - scaling factor used to tune the valuator input range
		"transscale"/"scale" - set the scaling factor for the location
			of the 6-sensor inputs.

	NOTE: presently the command to put the "DTrack" server into streaming
		mode via UDP is not implemented.  Thus the tracking stream must
		be manually started via the DTrack GUI interface.


	Using the "dtracktest" application (optional arguments are set by
	environment variables):
		% setenv DTRACK_HOST <value>
		% setenv DTRACK_PORT <value>
		% ./dtracktest
	NOTE: the screen-rendering format commonly available for input tracking
	systems is not yet available for DTrack inputs.


NOTES on the ARTracking "DTrack" protocol:

	The primary form of communication for the "DTrack" protocol is for
	the "DTrack" server to send tracking and other input information via
	UDP to a select set of hosts at port 5000.  Optionally, the server
	can be sent control commands via UDP to it's port 5001 (I suppose the
	server might send the data to itself, which would preclude the use
	of port 5000 for the commands).  Note that the ability to accept
	commands is a menu option in the DTrack server software, so be sure
	to enable this if you hope to have the end software (ie. FreeVR)
	make use of this feature.  The data can be sent either via an ASCII
	protocol or in binary form.  Thus far, only the ASCII format will be
	described and handled here.

	Command protocol:
		There are only a handful of commands that are recognized by
		the "DTrack" server:
			- "dtrack 10 0" -- deactivate (turn off) the cameras
			- "dtrack 10 1" -- activate (turn on) the cameras
			- "dtrack 10 3" -- activate the cameras and do reconstruction/calculation
			- "dtrack 31" -- start continuous update mode
			- "dtrack 32" -- stop continuous update mode
			- "dtrack 33 %d" -- send <N> data packets (i.e. poll mode)

	ASCII Data protocol:
		Several data items (parcels?) are generally sent as a single
		packet, with a CR/NL pair separating items.
			- "fr %d\n" -- frame counter
			- "ts %f\n" -- timestamp
			- "6dcal %d\n" -- number of calibrated bodies -- see
				questions for ART below for additional info
			- "6d %d ..." -- standard bodies (ie. ones that only
				have extra info)
				num-reports
				[id quality] [sx sy sz nu theta phi] [r0..r8 (3x3 rotation matrix)]
			- "6df %d ..." -- old flystick format
				num-reports
				[id qual button-mask][sx sy sz nu theta phi][r0..r8 (3x3 mat)]
			- "6df2 %d %d ..." -- new flystick format
				num-exist num-reports
				[id qual num-buttons num-controllers][sx sy sz][r0..r8 (3x3 mat)][bt0..btN ct0..ctN]
				NOTE: there is one button integer for every 32
				buttons -- so normally just one.
			- "6dmt %d ..." -- measurement tool
				num-reports
				[id qual unused][sx sy sz][r0..r8 (3x3 mat)]
			- "gl %d ..." -- glove/finger format
				num-reports
				[id qual hand num-fingers][sx sy sz][r0..b8] {(for each fingertip) [sx sy sz][b0..b8][radius, phalange length, phalanx, phalanx angle, 2nd phalanx angle]}
			- "glcal %d\n" -- number of defined hands
			- "3d %d ..." -- other 3d position markers
				num-reports
				[id qual][sx sy sz]

		Questions for AR-Tracking: 
			- are the unit identifiers unique across all types?
				A (from Andreas Werner): No (just as I expected)
			- why would there be a "6d 0\n" parcel in the sample data?
				A (from Andreas): not sure
			- what is the purpose of the "6dcal %d\n" parcel?
				(what does, does not get counted, and why
				 would this be used?)
				A (from Andreas): "6dcal gives the number of
				calibrated 6D objects (not Flysticks etc.) -
				some drivers wanted that information to
				allocate fields statically."
				FQ: why not flysticks?
			- how do I know how many button integers will be
				reported? (the manual explicitly says that
				"num_buttons" is number of buttons, ie. it
				doesn't suggest that it's the number of bytes)
				A (from Andreas): "There is one integer per
				32 buttons - and since none of our devices has
				that many buttons it will only be one. The
				format is just flexible so if we ever build a
				button with 33 buttons, there will be two."


		Where the rotation matrixii are defined as:
			    / b0  b3  b6 \
			R = | b1  b4  b7 |
			    \ b2  b5  b8 /


		Other notes:
			* it seems that with the flystick-1 units, when more
				than one button is pressed (including the
				china-hat), the button values (including the
				valuators) are all reported as 0.  So it's not
				even possible to push forward on the china-hat
				and push the trigger at the same time (and have
				them recognized) -- it will look as though no
				buttons are being pushed.  This is very bad.
				The good news is that the flystick-2 models do
				not have this limitation.
				NOTE: in my test setup, the data for both
				 models of flystick are reported as "6df2".
				A (from Andreas): Yes - the Fly-1 only supports
				one button at a time.

			* it seems as though the quality value is binary (1.0 or -1.0).
				A (from Andreas): No - that was intended for
				future use, currently it only tells you whether
				the Flystick is seen or not

			* it seems as though the 6d units (or at least the head
				tracker at the AVL cave) never report a quality
				other than 1.0, and when that unit is out of
				range, the position numbers freeze.  Whereas w/
				both of the flysticks (model 1 and model 2),
				... [what?]
				A (from Andreas): Yes: a 6d unit is not reported
				when it is not observed, but a Flystick might
				send button data when it is not observed.


HISTORY:
	04-08 November 2010 (Bill Sherman) -- wrote initial version starting
		from vr_input.vruidd.c.  I presently have the basic test
		program working -- it reads the UDP data, stores it in the
		_DTrackPrivateInfo structure, and then prints it.  Presently
		it only parses the two types of position data I expect to get
		from most setups ("6d" and "6df2"), but adding other types
		will be simple once the basic FreeVR interface is completed.
		I have also defined the FreeVR interface, but not yet
		implemented it.

	09 November 2010 (Bill Sherman) -- wrote the initial FreeVR interface.
		Currently I'm only parsing the "6d" and "6df2" position packets
		since I don't have a lot of time right now, and I think those
		will probably cover 98% of the use cases at the moment.  And
		then I did some cleanup (got rid of the VruiDD stuff, etc.)

	11 November 2010 (Bill Sherman) -- did first live tests of full FreeVR
		version on a CAVE with AR-tracking.  From this made several bug
		fixes, and now the interface works with "6d" and "6df2" input
		types.

	5 December 2011 (Bill Sherman) -- just some small adjustments to debug
		output styles.

	2 September 2013 (Bill Sherman) -- adopting a consistent usage
		of str<...>cmp() functions (using logical negation).

	14 September 2013 (Bill Sherman)
		I am making use of the newly created SELFCTRL_DBGLVL for all
		the self-control outputs.  I then added some reasonable output
		to the "print_help" callback to print out what all the device's
		inputs are doing.

	15 September 2013 (Bill Sherman)
		Changed the "0x%p" format to the improved "%#p" format.

		I added command line options:
			- "-list" 

TODO:
	- DONE: Write a man page for "dtracktest"
		- NOTE: include information on how to find out what other
			program is bound to a UDP port:
			% netstat -nlup | grep 5000 (or whatever port)

	- Add a timeout when trying to connect to a DTrack server that
		isn't there or isn't responding.

	- Add curses rendering to "dtracktest" (the test program)

	- Add the ability to set the data-port in "dtracktest"

	- Complete/enhance the FreeVR driver:
		- See whether there's a clean way to only update inputs
			(especially buttons and valuators) when actual
			changes have occurred -- mostly that's important
			for buttons I would think.
	 -->	- Implement the other position types: (fs1, mt, gl, 3body)
		- See about putting the time-stamp into the FreeVR data.
		- finish _DTrackPrintStruct()
		- fill in "version" and "op_params" fields
		- handle the "fs1" button case in _DTrackButtonInput()
		- add "quality" as a valuator value
		- add "quality" as a button value
		- add a feature to freeze flysticks (or any 6body)
			when the quality value is -1.0 -- right now the
			position data jumps to all zeros.

	- Add the ability to send control commands.  Currently, we assume
		that the user manually initiates the data stream from the
		"DTrack" server software GUI interface.

**************************************************************************/
#if !defined(FREEVR) && !defined(TEST_APP) && !defined(CAVE)
/* Choose how this code should be compiled (ie. for CAVE, standalone, etc). */
#  define	FREEVR
#  undef	TEST_APP
#  undef	CAVE
#endif

#undef COMM_DEBUG	/* define this to add a lot of output for debugging communications w/ the device */
#undef DEBUG_NOW	/* define this to add a little extra output for testing unimplemented parcels */


#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>		/* needed for uint16_t type */
#include <fcntl.h>		/* for socket parameter settings */

#include "vr_socket.h"
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
#  define	VR_W	3
#  define	VR_AZIM	0
#  define	VR_ELEV	1
#  define	VR_ROLL	2
#endif

#if defined(TEST_APP)
#  include <stdlib.h>		/* needed for getenv() & atoi() */
#  include "vr_socket.c"
#  include "vr_enums.h"
#  include "vr_utils.c"		/* needed for vrSleep() (temporary) */
#  define vrShmemAlloc malloc
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

#define	BUFSIZE		1024

#define	DEFAULT_DATAPORT	5000
#define DEFAULT_CMDPORT		5001
#define	DEFAULT_CMDHOST		"127.0.0.1"


/* DTrack position tracker types */
#define DTRACK_TYPE_UNK		0
#define DTRACK_TYPE_6BODY	1
#define DTRACK_TYPE_3BODY	2
#define DTRACK_TYPE_FS1		3
#define DTRACK_TYPE_FS2		4
#define DTRACK_TYPE_MT		5
#define DTRACK_TYPE_GLOVE	6

/* DTrack sensitivity values */
#define TRANS_SENSITIVITY	0.001
#define ROT_SENSITIVITY		0.02
#define VALUATOR_SENSITIVITY	1.0



/****************************************************************/
/*** auxiliary structures of the current data from the device. ***/

	/* _DTrackUnit: contains data for a single unit of the ARTracking DTrack */
typedef struct {
		int		id;		/* the numeric identifier for this unit */
		int		type;		/* the type of incoming position data (e.g. standard body, flystick-2) */
		int		new;		/* a flag to indicate whether new data has arrived since this unit was last processed (NOTE: not currently setting the flag to 0 since that would prevent multiple values from the same tracker from registering) */
		int		frame;		/* the frame in which this data was received */
		float		time_stamp;	/* the time stamp for when this data was received */
		float		quality;	/* the quality of this data */
		int		num_buttons;	/* the number of buttons for this unit */
		int		num_valuators;	/* the number of valuators for this unit */
		float		location[3];	/* the location of this unit */
		float		angles[3];	/* the orientation of this unit as reported by angles */
		float		rotation[9];	/* the orientation of this unit (elements of a 3x3 matrix) */
		uint32_t	buttons;	/* information of the buttons */
		float		valuators[32];	/* information of the valuators */
		int		active;		/* a flag to indicate whether position data is being received for this unit */
#ifdef CAVE
		/* CAVE specific fields here */

#elif defined(FREEVR)
		/* FREEVR specific fields here */
		vr6sensor	sensor6_input;	/* the assembled 6-dof data */

#endif /* end library-specific fields */
	} _DTrackUnit;

	/* _DTrackPrivateInfo: Overall data for the system */
typedef struct {
		/* these are for interfacing with the hardware */
		int		fd_socket;	/* communication file descriptor */
		char		*server_host;	/* name of daemon's host */
		int		server_port;	/* port on host listening for application commands */
		int		data_port;	/* port on this machine to which data will be sent from the server */
		int		open;		/* flag with DTrack successfully open */

		/* these are for internal data parsing */
		unsigned char	buf[BUFSIZE];
		int		eobuf_pos;
		char		version[256];	/* self-reported version of the device */
		char		op_params[256];	/* operating parameters of the device (according to it) */

		/* information about the current values */
		int		frame;			/* the number of the last frame of data received */
		float		time_stamp;		/* the last time stamp received */
#define UNITS_PT 5	/* the maximum number of units per position input type */
		_DTrackUnit	units_6body[UNITS_PT];	/* an array of all the 6body units */
		_DTrackUnit	units_fs2[UNITS_PT];	/* an array of all the flystick-2 units */

		/* information about how to filter the data for use with the VR library (FreeVR or CAVE) */
		float		scale_valuator;		/* scaling factor for valuators */
		float		scale_trans;		/* multiplier to scale from the DTrack location units (inches) */

		/* information about the inputs and mappings */
#ifdef CAVE
		/* CAVE specific fields here */

#elif defined(FREEVR)
		/* FREEVR specific fields here */
		int		num_buttons;		/* it is often wise to store the number of button inputs in addition to knowing the maximum possible */
		vr2switch	**button_inputs;	/* the maximum number of buttons is devinfo->num_2ways + devinfo->num_scontrols (all the self-controls are not necessarily buttons) */
		int		*button_map_bit;	/* An array of indices mapping from the server button id to the FreeVR logical id */
		int		*button_map_tracker;	/* An array of indices mapping from the FreeVR logical id to the tracker containing the data */
		int		*button_map_type;	/* An array of DTrack position tracker types for which the buttons are mapped from */

		int		num_switches;
		vrNswitch	**switch_inputs;

		int		num_valuators;
		vrValuator	**valuator_inputs;
		int		*valuator_map_valuator;	/* An array of indices mapping from the server valuator id to the FreeVR logical id */
		int		*valuator_map_tracker;	/* An array of indices mapping from the FreeVR logical id to the tracker containing the data */
		int		*valuator_map_type;	/* An array of DTrack position tracker types for which the valuators are mapped from */
		float		*valuator_map_sign;	/* An array indicating the sign of each valuator input (a multiplier so either "1.0" or "-1.0") */

		int		num_6sensors;
		vr6sensor	**sensor6_inputs;
		int		*tracker_map_tracker;	/* An array of indices mapping from the server receiver id to the FreeVR logical id */
		int		*tracker_map_type;	/* An array of DTrack position tracker types for which the trackers are mapped from */

		int		num_Nsensors;
		vrNsensor	**sensorN_inputs;

#endif /* end library-specific fields */

	} _DTrackPrivateInfo;



	/*********************************************/
	/*** General NON public interface routines ***/
	/*********************************************/

/************************************************************/
char *vrTrackerTypeName(int type)
{
	switch (type) {
	case DTRACK_TYPE_UNK:	return "unknown";
	case DTRACK_TYPE_6BODY:	return "6body";
	case DTRACK_TYPE_3BODY:	return "3body";
	case DTRACK_TYPE_FS1:	return "fs1";
	case DTRACK_TYPE_FS2:	return "fs2";
	case DTRACK_TYPE_MT:	return "mt";
	case DTRACK_TYPE_GLOVE:	return "gl";
	}

	return "unknown";
}


/************************************************************/
int vrTrackerTypeValue(char *name)
{
	if      (!strcasecmp(name, "unknown"))	return DTRACK_TYPE_UNK;
	else if (!strcasecmp(name, "6body"))	return DTRACK_TYPE_6BODY;
	else if (!strcasecmp(name, "6d"))	return DTRACK_TYPE_6BODY;
	else if (!strcasecmp(name, "3body"))	return DTRACK_TYPE_3BODY;
	else if (!strcasecmp(name, "3d"))	return DTRACK_TYPE_3BODY;
	else if (!strcasecmp(name, "fs1"))	return DTRACK_TYPE_FS1;
	else if (!strcasecmp(name, "6df"))	return DTRACK_TYPE_FS1;
	else if (!strcasecmp(name, "fs2"))	return DTRACK_TYPE_FS2;
	else if (!strcasecmp(name, "6df2"))	return DTRACK_TYPE_FS2;
	else if (!strcasecmp(name, "mt"))	return DTRACK_TYPE_MT;
	else if (!strcasecmp(name, "gl"))	return DTRACK_TYPE_GLOVE;
	else if (!strcasecmp(name, "glove"))	return DTRACK_TYPE_GLOVE;
	else {
		vrErrPrintf("Unknown position receiver type '%s'\n", name);
		return DTRACK_TYPE_6BODY;
	}

	/* Can't get to this statement */
	return DTRACK_TYPE_6BODY;
}


/******************************************************/
/* typename is used to specify a particular device among many that */
/*   share (more or less) the same protocol.  The typename is then */
/*   used to determine what specific features are available with   */
/*   this particular type of device.                               */
static void _DTrackInitializeStruct(_DTrackPrivateInfo *aux, char *typename)
{
	aux->version[0] = '\0';
	aux->op_params[0] = '\0';

	aux->buf[0] = '\0';
	aux->eobuf_pos = 0;

	aux->data_port = DEFAULT_DATAPORT;
	aux->server_port = DEFAULT_CMDPORT;
	aux->server_host = vrShmemStrDup(DEFAULT_CMDHOST);

	aux->scale_valuator = VALUATOR_SENSITIVITY;	/* set the default valuator scaling factor */
	aux->scale_trans = 1.0/304.8;			/* convert from mm to feet by default */

	/* everything else is zero'd by default */
}


/******************************************************/
static void _DTrackPrintStruct(FILE *file, _DTrackPrivateInfo *aux, vrPrintStyle style)
{
	int	count;
	int	ptv_count;

	vrFprintf(file, "DTrack device internal structure (%#p):\n", aux);
	vrFprintf(file, "\r\tversion -- '%s'\n", aux->version);
	vrFprintf(file, "\r\toperating parameters -- '%s'\n", aux->op_params);
	vrFprintf(file, "\r\tfd_socket = %d\n\tdata port = %d\n\tserver host = '%s'\n\tserver port = %d\n\topen = %d\n",
		aux->fd_socket,
		aux->data_port,
		aux->server_host,
		aux->server_port,
		aux->open);

	/* print the frame info */
	vrFprintf(file, "\r\tframe num = %d\n", aux->frame);
	vrFprintf(file, "\r\ttime stamp= %f\n", aux->time_stamp);

	/* print the filter values */
	vrFprintf(file, "\r\tscale_valuator = %f\n", aux->scale_valuator);
	vrFprintf(file, "\r\tscale_trans = %f\n", aux->scale_trans);

	/* print the raw values */
	/* TODO: print the raw values */

#ifdef FREEVR /* { */
	/* print the FreeVR input objects */
	vrFprintf(file, "\r\tbutton inputs:\n");
	for (count = 0; count < aux->num_buttons; count++)
		vrFprintf(file, "\r\t\tbutton_input[%d] = %#p%s\n", count, aux->button_inputs[count], ((aux->button_inputs[count]->input_type == VRINPUT_CONTROL) ? " (*control*)" : ""));
	/* TODO: print position-tracker values */

	vrFprintf(file, "\r\tvaluator inputs:\n");
	for (count = 0; count < aux->num_valuators; count++)
		vrFprintf(file, "\r\t\tvaluator_input[%d] = %#p%s\n", count, aux->valuator_inputs[count], ((aux->valuator_inputs[count]->input_type == VRINPUT_CONTROL) ? " (*control*)" : ""));

#if 0 /* NOT YET IMPLEMENTED */
	vrFprintf(file, "\r\t6sensor inputs (active = %d):\n", aux->active_sim6sensor);
	for (count = 0; count < aux->num_6sensors; count++)
		vrFprintf(file, "\r\t\t6sensor_inputs[%d] = %#p\n", count, aux->sensor6_inputs[count]);
#endif

#  if 0 /* TODO: not sure if we want/need this */
	vrFprintf(file, "\r\tcontrol inputs:\n");
	for (count = 0; count < MAX_CONTROLS; count++)
		vrFprintf(file, "\r\t\tcontrol_inputs[%d] = %#p\n", count, aux->control_inputs[count]);
#  endif
#endif /* } FREEVR */

	/* TODO: print some info about the current values */
}


/**************************************************************************/
static void _DTrackPrintHelp(FILE *file, _DTrackPrivateInfo *aux)
{
	int	count;		/* looping variable */

#if 0 || defined(TEST_APP)
	vrFprintf(file, BOLD_TEXT "Sorry, DTrack - print_help control not yet implemented.\n" NORM_TEXT);
#else
	vrFprintf(file, BOLD_TEXT "DTrack - inputs:" NORM_TEXT "\n");
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


/***************************************************************************/
/* _DTrackReadInput(): function reads data from the socket, and then looks */
/*   for and parses packets from the daemon.  The data is then placed into */
/*   the generic portion of the "_DTrackPrivateInfo" structure.            */
/* NOTE: _DTrackReadInput() is placed above the __DTrackInitializeDevice() */
/*   function to allow the latter to call the former.                      */
static int _DTrackReadInput(_DTrackPrivateInfo *aux)
/* Returns number of packets read this time (-1 indicates reception of the STOPSTREAM_REPLY message) */
{
	uint16_t	message;		/* VRUI VRDeviceDaemon messages are 2 byte sequences */
	int		read_result;		/* return value of the call to "read()" */
	int		packets_decoded = 0;	/* return the number of packets that are decoded */
	int		num_bodies;		/* the number of position data received for a given parcel */
	int		count;			/* loop counter */
	int		body_count;		/* loop counter over number of bodies */
	int		shift;			/* number of bytes to shift after parsing */
	int		bytes;			/* number of bytes parsed in a particular operation */
	int		parsed_bytes;		/* number of bytes parsed thus far */
	int		return_count;		/* the number of items that were parsed via sscanf() */
	_DTrackUnit	tempunit;		/* a holding place for incoming unit data */

	do {
		read_result = (ssize_t)read(aux->fd_socket, &aux->buf[aux->eobuf_pos], sizeof(aux->buf) - aux->eobuf_pos);
		if (read_result > 0) {
			aux->eobuf_pos += read_result;
			aux->buf[aux->eobuf_pos] = '\0';
#if defined(COMM_DEBUG) || defined(DEBUG_NOW) || 0
			vrDbgPrintf("_DTrackReadInput(): %d bytes currently in buffer.\n", aux->eobuf_pos);
			vrPrintf("%3d bytes (%d): %s\n", (int)read_result, aux->eobuf_pos, aux->buf);
#endif
		}
	} while (read_result > 0);
#if defined(COMM_DEBUG) || 0
if (aux->eobuf_pos > 0)
	vrPrintf("yoyo about to decode: eofbuf[pos-1] = '%02x', eobuf_pos = %d\n", aux->buf[aux->eobuf_pos-1], aux->eobuf_pos);
#endif

	/* The type of packet is determined by the first 2 to 5 characters, but even the 2-character */
	/*   messages have followon data, so there's nothing to parse until we get at least 5 chars. */
#if defined(DEBUG_NOW) && 0
vrPrintf("HEYHEYHEY buf-pos = %d, size = %d\n", aux->eobuf_pos, sizeof(aux->buf) - aux->eobuf_pos);
vrPrintf("memchr = %p\n", memchr(aux->buf, '\n', aux->eobuf_pos));
vrPrintf("Done with memchr\n");
#endif
#if 0
	while (aux->eobuf_pos >= 5 && aux->buf[aux->eobuf_pos-1] == '\n') { /* old method  -- the problem with this method occurs at the extreme -- when the buffer is nearly full, there are times when the newline character isn't in the expected place.  I only vaguely understand why this would be, so more analysis is required to really know what's going on and what a completely correct conditional check should be.  In the meantime, just looking for a newline seems to be satisfactory and sufficient.  TODO: get a better understanding of what's going on here. */
#else
	while ((aux->eobuf_pos >= 5) && (memchr(aux->buf, '\n', aux->eobuf_pos) != NULL)) { /* TODO: make sure this isn't an off-by-one bug */
#endif
#if defined(DEBUG_NOW) && 0
	vrPrintf("YO: memchr returning %p, aux->eobuf_pos = %d, size = %d\n", memchr(aux->buf, '\n', aux->eobuf_pos), aux->eobuf_pos, sizeof(aux->buf));
#endif
		/**********************************/
		/*** parcel type: frame -- "fr" ***/
		if (!strncmp(aux->buf, "fr ", 3)) {
#ifdef DEBUG_NOW
printf("got fr parcel -- %s\n", vrPrintableString(aux->buf));
#endif
			/* the frame number */
			aux->frame = atoi(&aux->buf[3]);

			/* adjust the buffer */
			shift = (void *)index(aux->buf, '\n') - (void *)aux->buf + 1;
			memmove(aux->buf, &aux->buf[shift], aux->eobuf_pos - shift + 1);				/* shift the data by one packet */
			aux->eobuf_pos -= shift;
			packets_decoded++;

		/***************************************/
		/*** parcel type: time stamp -- "ts" ***/
		} else if (!strncmp(aux->buf, "ts ", 3)) {
#ifdef DEBUG_NOW
printf("got ts parcel -- %s\n", vrPrintableString(aux->buf));
#endif
			/* a time stamp */
			aux->time_stamp = atof(&aux->buf[3]);

			/* adjust the buffer */
			shift = (void *)index(aux->buf, '\n') - (void *)aux->buf + 1;
			memmove(aux->buf, &aux->buf[shift], aux->eobuf_pos - shift + 1);				/* shift the data by one packet */
			aux->eobuf_pos -= shift;
			packets_decoded++;

		/*********************************************/
		/*** parcel type: standard bodies --  "6d" ***/
		} else if (!strncmp(aux->buf, "6d ", 3)) {
#ifdef DEBUG_NOW
printf("got 6d parcel -- %s\n", vrPrintableString(aux->buf));
#endif
			/* a standard body */
			num_bodies = atof(&aux->buf[3]);
			parsed_bytes = 3; /* the first 3 bytes have been parsed */

			/* First assign all standard bodies to have inactive status -- and then we'll set the ones that provide data with active status */
			/* NOTE: the ways that active/inactive is handled for standard bodies is different that for flysticks -- just the nature of how AR-tracking defined the protocol. */
			for (body_count = 0; body_count < UNITS_PT; body_count++) {
				aux->units_6body[body_count].active = 0;
			}

			/* Next get the number of bodies */
			return_count = sscanf(&(aux->buf[parsed_bytes]), "%d%n", &num_bodies, &bytes);
			parsed_bytes += bytes;

			for (body_count = 0; body_count < num_bodies; body_count++) {
				memset(&tempunit, 0, sizeof(tempunit));
				tempunit.type = DTRACK_TYPE_6BODY;
				tempunit.frame = aux->frame;
				tempunit.time_stamp = aux->time_stamp;
				tempunit.new = 1;
				tempunit.active = 1;

				/* parse the first section (info) */
				return_count = sscanf(&(aux->buf[parsed_bytes]), " [%d %f]%n", &tempunit.id, &tempunit.quality, &bytes);
				tempunit.num_buttons = 0;
				tempunit.num_valuators = 0;
				parsed_bytes += bytes;

				/* parse the second section (location & angles) */
				return_count += sscanf(&(aux->buf[parsed_bytes]), "[%f %f %f %f %f %f]%n", &tempunit.location[0], &tempunit.location[1], &tempunit.location[2], &tempunit.angles[0], &tempunit.angles[1], &tempunit.angles[2], &bytes);
				parsed_bytes += bytes;

				/* parse the third section (rotation) */
				return_count += sscanf(&(aux->buf[parsed_bytes]), "[%f %f %f %f %f %f %f %f %f]%n", &tempunit.rotation[0], &tempunit.rotation[1], &tempunit.rotation[2], &tempunit.rotation[3], &tempunit.rotation[4], &tempunit.rotation[5], &tempunit.rotation[6], &tempunit.rotation[7], &tempunit.rotation[8], &bytes);
				parsed_bytes += bytes;

				/* save the data into the generic space */
				if (aux->units_6body[tempunit.id].frame == tempunit.frame) {
					vrDbgPrintf("_DTrackReadInput(): Warning: 6d overwriting duplicate frame (%d) for id = %d.\n", tempunit.frame, tempunit.id);
				}
				aux->units_6body[tempunit.id] = tempunit;
			}

			/* adjust the buffer */
			shift = (void *)index(aux->buf, '\n') - (void *)aux->buf + 1;
			if (shift < parsed_bytes) {
				vrDbgPrintf("_DTrackReadInput(): Warning: 6d parsed more bytes (%d) than the shift (%d).\n", parsed_bytes, shift);
			}
			memmove(aux->buf, &aux->buf[shift], aux->eobuf_pos - shift + 1);				/* shift the data by one packet */
			aux->eobuf_pos -= shift;
			packets_decoded++;

		/******************************************/
		/*** parcel type: flystick-2 --  "6df2" ***/
		} else if (!strncmp(aux->buf, "6df2 ", 5)) {
			/* a flystick (new format) */
			/* num-??? num-bodies {[id qual num-buttons num-controllers][sx sy sz][r0..r8 (3x3 mat)][bt0..btN ct0..ctN]}* */
			parsed_bytes = 5; /* the first five bytes have been parsed */
#ifdef DEBUG_NOW
printf("got 6df2 parcel -- %s\n", vrPrintableString(aux->buf));
#endif

			/* First assign all flystick-2's to have inactive status -- and then we'll set the ones that provide data with active status */
			for (body_count = 0; body_count < UNITS_PT; body_count++) {
				aux->units_fs2[body_count].active = 0;
			}

			/* Next get the number of bodies */
			/* TODO: I'm not sure what the first number means! (so it's being ignored for now) */
			return_count = sscanf(&(aux->buf[parsed_bytes]), "%*d %d%n", &num_bodies, &bytes);
			parsed_bytes += bytes;

			for (body_count = 0; body_count < num_bodies; body_count++) {
				memset(&tempunit, 0, sizeof(tempunit));
				tempunit.type = DTRACK_TYPE_FS2;
				tempunit.frame = aux->frame;
				tempunit.time_stamp = aux->time_stamp;
				tempunit.new = 1;

				/* parse the first section (info) */
				return_count += sscanf(&(aux->buf[parsed_bytes]), " [%d %f %d %d]%n", &tempunit.id, &tempunit.quality, &tempunit.num_buttons, &tempunit.num_valuators, &bytes);
				parsed_bytes += bytes;

				/* Now set the active status based on the quality -- NOTE: this is different than for standard bodies */
				if (tempunit.quality > 0.0)
					tempunit.active = 1;
				else	tempunit.active = 0;

				/* parse the second section (positions) */
				return_count += sscanf(&(aux->buf[parsed_bytes]), "[%f %f %f][%f %f %f %f %f %f %f %f %f]%n", &tempunit.location[0], &tempunit.location[1], &tempunit.location[2], &tempunit.rotation[0], &tempunit.rotation[1], &tempunit.rotation[2], &tempunit.rotation[3], &tempunit.rotation[4], &tempunit.rotation[5], &tempunit.rotation[6], &tempunit.rotation[7], &tempunit.rotation[8], &bytes);
				parsed_bytes += bytes;

				/* parse the third section (buttons & valuators) */
				/* NOTE: it's important to scan that last closing square-bracket since we're in a loop and there may be more data to parse */
				return_count += sscanf(&(aux->buf[parsed_bytes]), "[%d %n", &tempunit.buttons, &bytes);
				parsed_bytes += bytes;
				for (count = 0; count < tempunit.num_valuators; count++) {
					return_count += sscanf(&(aux->buf[parsed_bytes]), "%f %n", &tempunit.valuators[count], &bytes);
					parsed_bytes += bytes;
				}
				return_count += sscanf(&(aux->buf[parsed_bytes]), "]%n", &bytes);
				parsed_bytes += bytes;

				/* save the data into the generic space */
				if (aux->units_fs2[tempunit.id].frame == tempunit.frame) {
					vrDbgPrintf("_DTrackReadInput(): Warning: 6df2 overwriting duplicate frame (%d) for id = %d.\n", tempunit.frame, tempunit.id);
				}
				aux->units_fs2[tempunit.id] = tempunit;
			}

#if 1 /* TODO: working on the bug at VisBox -- full buffers may cause a lack of CR character */
			/* adjust the buffer */
			shift = (void *)index(aux->buf, '\n') - (void *)aux->buf + 1;
			if (shift < parsed_bytes) {
				vrDbgPrintf("_DTrackReadInput(): Warning: 6df2 parsed more bytes (%d) than the shift (%d).\n", parsed_bytes, shift);
			}
#else
			shift = parsed_bytes;	/* this was my test code at Visbox -- with the "improved" while statement, this hack is not needed. */
#endif
			memmove(aux->buf, &aux->buf[shift], aux->eobuf_pos - shift + 1);				/* shift the data by one packet */
			aux->eobuf_pos -= shift;
			packets_decoded++;

		} else {
			/* skip unknown/un-parsed data */
			shift = (void *)index(aux->buf, '\n') - (void *)aux->buf + 1;
#ifdef DEBUG_NOW
printf("Unknown parcel, shifting '%s'\n", vrPrintableString(aux->buf));
#endif
			vrDbgPrintf("\n_DTrackReadInput(): Warning: shifting for unknown parcel ('%c%c%c%c%c') (0x%02x %02x %02x %02x %02x %02x %02x %02x %02x) by %d bytes\n", aux->buf[0], aux->buf[1], aux->buf[2], aux->buf[3], aux->buf[4], aux->buf[0], aux->buf[1], aux->buf[2], aux->buf[3], aux->buf[4], aux->buf[5], aux->buf[6], aux->buf[7], aux->buf[8], shift);
			memmove(aux->buf, &aux->buf[shift], aux->eobuf_pos - shift + 1);				/* shift the data by one packet */
			aux->eobuf_pos -= shift;
			packets_decoded++;
		}
	}
#if defined(DEBUG_NOW) && 0
	vrPrintf("memchr returning %p, aux->eobuf_pos = %d, size = %d\n", memchr(aux->buf, '\n', aux->eobuf_pos), aux->eobuf_pos, sizeof(aux->buf));
#endif

#if defined(COMM_DEBUG) || defined(DEBUG_NOW) || 0
if (packets_decoded > 0)
	vrPrintf("_DTrackReadInput(): %d packets decoded this call.\n", packets_decoded);
#endif
	return (packets_decoded);
}


/**********************************************************/
/* _DTrackInitializeDevice() is called in the OPEN phase  */
/*   of input interface -- after the types of inputs have */
/*   been determined (during the CREATE phase).           */
static int _DTrackInitializeDevice(_DTrackPrivateInfo *aux)
{
	uint16_t	message;	/* VRUI VRDeviceDaemon messages are 2 byte sequences */
	uint32_t	value;		/* VRUI 4-byte value */
	int		read_result;	/* return value of the call to "read()" */
	int		bytes_read = 0;	/* number of bytes read from socket (the Vrui Daemon) thus far */
	int		count;		/* loop counter */

	if (aux == NULL) {
		vrErrPrintf("_DTrackInitializeDevice(): " RED_TEXT "Warning, no auxiliary data for DTrack.\n" NORM_TEXT);
		return -1;
	}

	if (fcntl(aux->fd_socket, F_SETFL, O_RDWR | O_NONBLOCK | O_NOCTTY) < 0) {
                vrErrPrintf("_DTrackInitializeDevice(): An error occurred while trying to set the file controls.\n");
        }

	/***********************************************/
	/* TODO: When using server commands, this may be where they go */

	return 0;
}


/**********************************************************/
/* _DTrackCloseDevice() is called in the CLOSE phase      */
/*   of input interface -- just before the ports are      */
/*   closed, etc.                                         */
static int _DTrackCloseDevice(_DTrackPrivateInfo *aux)
{
	uint16_t		message;	/* VRUI VRDeviceDaemon messages are 2 byte sequences */

	/* Don't close a device that's not open */
	if (aux->open == 0)
		return 0;

	/* TODO: determine what all needs to be done here */

}



	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/


#if defined(FREEVR) /* { */
/*****************************************************************/
/***  Functions for FreeVR access of DTrack devices for input  ***/
/*****************************************************************/


	/************************************/
	/***  FreeVR NON public routines  ***/
	/************************************/


/*********************************************************/
static void _DTrackParseArgs(_DTrackPrivateInfo *aux, char *args)
{
	float	scale_value = -1.0;		/* for reading one of the scaling factor values */

	/* In the rare case of no arguments, just return */
	if (args == NULL)
		return;

	/*****************************************/
	/** Argument format: "cmdhost" "=" file **/
	/*****************************************/
	vrArgParseString(args, "cmdhost", &(aux->server_host));

	/*******************************************/
	/** Argument format: "cmdport" "=" number **/
	/*******************************************/
	vrArgParseInteger(args, "cmdport", &(aux->server_port));

	/*******************************************/
	/** Argument format: "port" "=" number **/
	/*******************************************/
	vrArgParseInteger(args, "port", &(aux->data_port));

#if 1 /* not sure whether we'll want this */
	/********************************************/
	/** Argument format: "valScale" "=" number **/
	/********************************************/
	if (vrArgParseFloat(args, "valscale", &(aux->scale_valuator))) {
		aux->scale_valuator = VALUATOR_SENSITIVITY;
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

	/** TODO: other arguments to parse go here **/
}


/**************************************************************************/
/* The _DTrackGetData(): function calls the _DTrackReadInput() function   */
/*   to get the latest data, and then puts that data into the FreeVR data */
/*   structures.  I.e. this is the FreeVR-specific portion of the data    */
/*   parsing.                                                             */
static void _DTrackGetData(vrInputDevice *devinfo)
{
	_DTrackPrivateInfo	*aux = (_DTrackPrivateInfo *)devinfo->aux_data;
	vrGenericInput		*input;			/* pointer to the input we're processing */
	int			num_inputs;		/* the number of input parcels parsed by _DTrackReadInput() */
	int			dtrack_num;		/* the particular button/valuator data to be extracted for the input */
	int			dtrack_tracker;		/* local storage of the mapping code from FreeVR input number to DTrack input number */
	int			dtrack_type;		/* the type of position input that the buttons/valuators are associated with */
	_DTrackUnit		*type_array;		/* pointer to a particular array of units */
	int			button_value;		/* holder for extracting the button flag from an integer */
	float			valuator_value;		/* holder for compositing a value together from components (e.g. sign & value) */
	float			scale_trans = aux->scale_trans;	/* a valuator scaling factor */
	int			count;			/* a loop counter */

	/*******************/
	/* gather the data */
	num_inputs = _DTrackReadInput(aux);			/* Make sure we have the latest data stored in the "_FastrakPrivateInfo" structure */

	/* There's no point in processing the inputs if we don't have anything new */
#ifdef DEBUG_NOW
printf("num_inputs = %d, operating = %d, open = %d, aux->eobuf_pos = %d\n", num_inputs, devinfo->operating, aux->open, aux->eobuf_pos);
#endif
      if (num_inputs > 0) {
#if 0
printf("num_buttons = %d\n", aux->num_buttons);
printf("button map: %d %d %d %d %d %d -- %d %d %d %d %d %d -- %d %d %d %d %d %d\n",
	aux->button_map_bit[0],
	aux->button_map_bit[1],
	aux->button_map_bit[2],
	aux->button_map_bit[3],
	aux->button_map_bit[4],
	aux->button_map_bit[5],
	aux->button_map_tracker[0],
	aux->button_map_tracker[1],
	aux->button_map_tracker[2],
	aux->button_map_tracker[3],
	aux->button_map_tracker[4],
	aux->button_map_tracker[5],
	aux->button_map_type[0],
	aux->button_map_type[1],
	aux->button_map_type[2],
	aux->button_map_type[3],
	aux->button_map_type[4],
	aux->button_map_type[5]);
#endif

	/*************/
	/** buttons **/
	for (count = 0; count < aux->num_buttons; count++) {
		dtrack_num = aux->button_map_bit[count];
		dtrack_tracker = aux->button_map_tracker[count];
		dtrack_type = aux->button_map_type[count];
		switch(dtrack_type) {
		case DTRACK_TYPE_6BODY:	type_array = aux->units_6body;	break;
		case DTRACK_TYPE_3BODY:	type_array = NULL;		break;
		case DTRACK_TYPE_FS1:	type_array = NULL;		break;
		case DTRACK_TYPE_FS2:	type_array = aux->units_fs2;	break;
		case DTRACK_TYPE_MT:	type_array = NULL;		break;
		case DTRACK_TYPE_GLOVE:	type_array = NULL;		break;
		default:		type_array = NULL;		break;
		}
		if (dtrack_tracker >= UNITS_PT) {	/* NOTE: '>=' because dtrack_map is a zero-based number */
			vrErrPrintf(RED_TEXT "_DTrackGetData(): request for button (%d) not available with the DTrack interface (max %d).\n", dtrack_tracker, UNITS_PT-1 /* correct for zero-base */);
			/* TODO: I should look into creating a dummy input, and using that after the first warning is printed */
		} else if (type_array == NULL) {
			vrErrPrintf(RED_TEXT "_DTrackGetData(): unknown (or un-handled) tracker type (%d) when processing button (%d).\n", dtrack_type, dtrack_tracker);
		} else if (type_array[dtrack_tracker].new == 1) {
			/* handle button inputs as buttons or self-controls */
			input = (vrGenericInput *)(aux->button_inputs[count]);
			button_value = ((type_array[dtrack_tracker].buttons & (0x01 << dtrack_num)) != 0);
			if (input != NULL) {
				switch (input->input_type) {
				case VRINPUT_BINARY:
					vrAssign2switchValue((vr2switch *)(input), button_value);
					break;
				case VRINPUT_CONTROL:
					vrCallbackInvokeDynamic(((vrControl *)(input))->callback, 1, button_value);
					break;
				default:
					vrErrPrintf(RED_TEXT "_DTrackGetData(): Unable to handle button inputs that aren't Binary or Control inputs\n" NORM_TEXT);
					break;
				}
			}
		}
	} /* button loop */

#if 0
printf("num_valuators = %d\n", aux->num_valuators);
printf("valuator map: %f %f -- %d %d -- %d %d -- %d %d\n",
	aux->valuator_map_sign[0],
	aux->valuator_map_sign[1],
	aux->valuator_map_valuator[0],
	aux->valuator_map_valuator[1],
	aux->valuator_map_tracker[0],
	aux->valuator_map_tracker[1],
	aux->valuator_map_type[0],
	aux->valuator_map_type[1]);
#endif

	/***************/
	/** valuators **/
	for (count = 0; count < aux->num_valuators; count++) {
		dtrack_num = aux->valuator_map_valuator[count];
		dtrack_tracker = aux->valuator_map_tracker[count];
		dtrack_type = aux->valuator_map_type[count];
		switch(dtrack_type) {
		case DTRACK_TYPE_6BODY:	type_array = aux->units_6body;	break;
		case DTRACK_TYPE_3BODY:	type_array = NULL;		break;
		case DTRACK_TYPE_FS1:	type_array = NULL;		break;
		case DTRACK_TYPE_FS2:	type_array = aux->units_fs2;	break;
		case DTRACK_TYPE_MT:	type_array = NULL;		break;
		case DTRACK_TYPE_GLOVE:	type_array = NULL;		break;
		default:		type_array = NULL;		break;
		}
		if (dtrack_tracker >= UNITS_PT) {	/* NOTE: '>=' because dtrack_tracker is a zero-based number */
			vrErrPrintf(RED_TEXT "_DTrackGetData(): request for valuator (%d) not available with the DTrack interface (max %d).\n", dtrack_tracker, UNITS_PT-1 /* correct for zero-base */);
			/* TODO: I should look into creating a dummy input, and using that after the first warning is printed */
		} else if (type_array == NULL) {
			vrErrPrintf(RED_TEXT "_DTrackGetData(): unknown (or un-handled) tracker type (%d) when processing valuator (%d).\n", dtrack_type, dtrack_tracker);
		} else if (type_array[dtrack_tracker].new == 1) {
			/* handle valuator inputs as valuators or self-controls */
			input = (vrGenericInput *)(aux->valuator_inputs[count]);
			if (input != NULL) {
				valuator_value = type_array[dtrack_tracker].valuators[dtrack_num] * aux->scale_valuator * aux->valuator_map_sign[count];
				switch (input->input_type) {
				case VRINPUT_VALUATOR:
					vrAssignValuatorValue((vrValuator *)(input), valuator_value);
					break;
				case VRINPUT_CONTROL:
					vrCallbackInvokeDynamic(((vrControl *)(input))->callback, 1, &valuator_value);
					break;
				default:
					vrErrPrintf(RED_TEXT "_DTrackGetData(): Unable to handle valuator inputs that aren't Floating or Control inputs\n" NORM_TEXT);
					break;
				}
			}
		}
	} /* valuator loop */

	/***************/
	/** 6-sensors **/
	for (count = 0; count < aux->num_6sensors; count++) {
		vr6sensor	*current_6sensor;
		vrMatrix	new_position;

		dtrack_tracker = aux->tracker_map_tracker[count];
		dtrack_type = aux->tracker_map_type[count];
		switch(dtrack_type) {
		case DTRACK_TYPE_6BODY:	type_array = aux->units_6body;	break;
		case DTRACK_TYPE_3BODY:	type_array = NULL;		break;
		case DTRACK_TYPE_FS1:	type_array = NULL;		break;
		case DTRACK_TYPE_FS2:	type_array = aux->units_fs2;	break;
		case DTRACK_TYPE_MT:	type_array = NULL;		break;
		case DTRACK_TYPE_GLOVE:	type_array = NULL;		break;
		default:		type_array = NULL;		break;
		}
		if (dtrack_tracker >= UNITS_PT) {	/* NOTE: '>=' because dtrack_tracker is a zero-based number */
			vrErrPrintf(RED_TEXT "_DTrackGetData(): request for valuator (%d) not available with the DTrack interface (max %d).\n", dtrack_tracker, UNITS_PT-1 /* correct for zero-base */);
			/* TODO: I should look into creating a dummy input, and using that after the first warning is printed */
		} else if (type_array == NULL) {
			vrErrPrintf(RED_TEXT "_DTrackGetData(): unknown (or un-handled) tracker type (%d) when processing valuator (%d).\n", dtrack_type, dtrack_tracker);
		} else if (type_array[dtrack_tracker].new == 1) {
			/* handle 6-sensor inputs */
			current_6sensor = aux->sensor6_inputs[count];

#if 0
printf("about to assign (or not) tracker data for tracker %d -- active = %d, type = %d, num = %d, x = %f\n", type_array[dtrack_tracker].active, count, dtrack_type, dtrack_tracker, type_array[dtrack_tracker].location[VR_X]);
#endif
			if (type_array[dtrack_tracker].active) {
				/* Assemble the new_position matrix */
				vrMatrixSetIdentity(&new_position);
				VRMAT_ROWCOL(&new_position, 0, 0) = type_array[dtrack_tracker].rotation[0];
				VRMAT_ROWCOL(&new_position, 1, 0) = type_array[dtrack_tracker].rotation[1];
				VRMAT_ROWCOL(&new_position, 2, 0) = type_array[dtrack_tracker].rotation[2];
				VRMAT_ROWCOL(&new_position, 0, 1) = type_array[dtrack_tracker].rotation[3];
				VRMAT_ROWCOL(&new_position, 1, 1) = type_array[dtrack_tracker].rotation[4];
				VRMAT_ROWCOL(&new_position, 2, 1) = type_array[dtrack_tracker].rotation[5];
				VRMAT_ROWCOL(&new_position, 0, 2) = type_array[dtrack_tracker].rotation[6];
				VRMAT_ROWCOL(&new_position, 1, 2) = type_array[dtrack_tracker].rotation[7];
				VRMAT_ROWCOL(&new_position, 2, 2) = type_array[dtrack_tracker].rotation[8];
				VRMAT_ROWCOL(&new_position, 0, 3) = type_array[dtrack_tracker].location[VR_X] * scale_trans;
				VRMAT_ROWCOL(&new_position, 1, 3) = type_array[dtrack_tracker].location[VR_Y] * scale_trans;
				VRMAT_ROWCOL(&new_position, 2, 3) = type_array[dtrack_tracker].location[VR_Z] * scale_trans;

				//VRMAT_ROWCOL(&new_position, 0, 3) = type_array[dtrack_tracker].frame;
				vrAssign6sensorValue(current_6sensor, &new_position, 0 /*, TODO: timestamp from daemon */);
			}
			vrAssign6sensorActiveValue(current_6sensor, type_array[dtrack_tracker].active);
		}
	} /* 6-sensor loop */

      } /* (num_inputs > 0) */
}


	/****************************************************************/
	/*    Function(s) for parsing DTrack "input" declarations.      */
	/*                                                              */
	/*  These _DTrack<type>Input() functions are called during the  */
	/*  CREATE phase of the input interface.                        */

/* ----------------------------------------------------------------------- */
/* The arguments for each of these "input" declaration parsers follow the  */
/*   same form, no matter what type of input or device:                    */
/*                                                                         */
/* devinfo -- a pointer to the overall structure that defines this device. */
/*            It is used to get access to the auxiliary data of the device.*/
/*                                                                         */
/* input -- a pointer to the vrGenericInput structure that is a specific   */
/*          FreeVR input, such as a single vr2switch, that is part of the  */
/*          specified input device.                                        */
/*          NOTE: the "input_type" can be determined by accessing the      */
/*          "input->input_type" field.  This can be used to have a single  */
/*          mapping function handle requests for multiple types of inputs. */
/*          See _XwindowsKeyboardInput() for an example of this.           */
/*                                                                         */
/* The remaining argument contains three strings that are from the portion */
/*   of the input config statement contained within the parentheses.  This */
/*   is pre-parsed as the "DTI" -- {<sub-Device>:}<Type>[<Instance>].  Any */
/*   of these subfields can be an empty string.  Usually:                  */
/*                                                                         */
/* device -- a string that may be used to indicate a sub-device of the     */
/*           provided hardware.  For example, and X11 input device may     */
/*           have "keyboard", "mouse" and "dial+buttons" subdevices.       */
/*           In most cases the "device" value is the empty string, and     */
/*           probably could always be included as part of the "type" value.*/
/*                                                                         */
/* type -- a string that indicates the class of input interface on the     */
/*         particular device/subdevice.  For example, an X11 input device  */
/*         may have "key", "mousebutton" and "pointerlocation" types.      */
/*         Or, a Magellan may have "button" and "6dof" types.              */
/*                                                                         */
/*         NOTE that the "type" value is also used in determining which of */
/*         these functions should be called (by matching the first element */
/*         of the vrInputFunctions list below), so it has already played a */
/*         roll in how this input is handled.  Therefore it's value is     */
/*         often ignored in the parsing function itself.  However, it is   */
/*         possible for more than one <type> string to point to the same   */
/*         device parsing function.  Thus in those cases it may be         */
/*         important to make use of this information within the parsing    */
/*         function.                                                       */
/*                                                                         */
/*         NOTE also that this string is different than the "input_type"   */
/*         data mentioned above as part of the "input" in that this is a   */
/*         string name that might describe the physical form of the input, */
/*         versus the "input_type" which describes a logical FreeVR input  */
/*         type such as a BINARY, VALUATOR or 6SENSOR input.               */
/*                                                                         */
/* instance -- a string usually used to indicate a specific instance of    */
/*             the type of data.  For example button "2" on a Magellan,    */
/*             unit "1" of a Flock of Birds, or key "Esc" of an X11        */
/*             keyboard.  Other data may also be included for altering how */
/*             this input is handled (See the VRPN device for an example). */
/*                                                                         */
/*    NOTE: since the "device" subfield is often optional, and the "type"  */
/*    subfield is used to determine which function to call, many of these  */
/*    functions will only make use of the "instance" subfield.             */
/*                                                                         */
/* The return values specify whether the initial match is confirmed, or    */
/*   rejected.  For some devices (such as the Xwindows-keyboard), there    */
/*   may be an overall match for the keyboard, but then the specific key   */
/*   instance may not match a known key for the keyboard.  In such cases   */
/*   a VRINPUT_MATCH_UNABLE is returned, indicating that the search failed,*/
/*   but that the search for a matching device should cease.  If the match */
/*   is successful, then a VRINPUT_MATCH_ABLE is returned.  If the match   */
/*   altogether fails, then a VRINPUT_NOMATCH is returned, and the search  */
/*   continues.                                                            */
/*                                                                         */
/* NOTE: these functions are called before making the physical connection  */
/*   to the device, so it is not possible to query features of the device  */
/*   within these functions.                                               */
/* ----------------------------------------------------------------------- */

/**************************************************************************/
/* Here is a sample definition for a DTrack button:                       */
/*     input "button-2" = "2switch(fs2[0, 1])";                           */
/* And since the numbers are 0-based, this means the 2nd button of the    */
/* 1st flystick-2.                                                        */
static vrInputMatch _DTrackButtonInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
        _DTrackPrivateInfo	*aux = (_DTrackPrivateInfo *)devinfo->aux_data;
	int			button_num;		/* the current count of FreeVR button inputs */
	int			button_choice;		/* the chosen number for this button */
	char			*button_name_instance;	/* pointer to the 2nd half of the instance string -- after the comma */
	int			unit_number;		/* the number of the given type of tracker (eg. the 2nd flystick-2) */
	int			tracker_type;		/* the type of DTrack tracker (e.g. a flystick-2) */

	/* The DTrack method of handling input arrays may be a combination of the way */ /* TODO: verify/reject this comment */
	/*   suggested by vr_input.skeleton.c as the FoB, MS & Fastrak input drivers. */

	/* TODO: determine whether type "fs1" or "fs2" */

	/* select a button */
	button_num = aux->num_buttons;
	aux->num_buttons++;

	tracker_type = vrTrackerTypeValue(dti->type);
	unit_number = vrAtoI(dti->instance);
	button_name_instance = strchr(dti->instance, ',') + 1;			/* jump past the comma */

	if (button_name_instance != NULL) {
		button_choice = vrAtoI(button_name_instance);
	} else {
		button_choice = -1;
	}

	/* check the selected button */
	if (button_choice < 0) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_DTrackButtonInput(): Warning, button['%s'] did not match any known button\n", dti->instance);
		return VRINPUT_MATCH_UNABLE;	/* device match, but still unable to handle request */
	} else if (unit_number >= UNITS_PT) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_DTrackButtonInput(): Warning, button['%s'] cannot be mapped due to insufficient position tracker units (%d)\n", dti->instance, UNITS_PT);
		return VRINPUT_MATCH_UNABLE;	/* device match, but still unable to handle request */
	} else if (aux->button_inputs[button_num] != NULL) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_DTrackButtonInput(): " RED_TEXT "Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);
	}

	/* make the mapping */
	aux->button_inputs[button_num] = (vr2switch *)input;
	aux->button_map_bit[button_num] = button_choice;
	aux->button_map_tracker[button_num] = unit_number;
	aux->button_map_type[button_num] = tracker_type;
	vrDbgPrintfN(INPUT_DBGLVL, "_DTrackButtonInput(): assigned button event of value %d (DTrack button %d) to input pointer = %#p)\n",
		button_num, button_choice, aux->button_inputs[button_num]);

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/**************************************************************************/
/* Here is a sample definition for a DTrack valuator:                     */
/*     input "joy-left-Y" = "valuator(fs2[0, -1])";                       */
/* And since the numbers are 0-based, this means the 2nd valuator of the  */
/* 1st flystick-2.  And do a sign-change on that valuator.                */
static vrInputMatch _DTrackValuatorInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
static	char			*whitespace = " \t\r\b\n";
        _DTrackPrivateInfo	*aux = (_DTrackPrivateInfo *)devinfo->aux_data;
	int			valuator_num;		/* the current count of FreeVR valuator inputs */
	int			valuator_choice;	/* the chosen number for this valuator */
	char			*valuator_name_instance;/* pointer to the 2nd half of the instance string -- after the comma */
	int			unit_number;		/* the number of the given type of tracker (eg. the 2nd flystick-2) */
	int			tracker_type;		/* the type of DTrack tracker (e.g. a flystick-2) */

	/* The DTrack method of handling input arrays may be a combination of the way */
	/*   suggested by vr_input.skeleton.c as the FoB, MS & Fastrak input drivers. */

	/* select a valuator (and sign) */
	valuator_num = aux->num_valuators;
	aux->num_valuators++;

	tracker_type = vrTrackerTypeValue(dti->type);
	unit_number = vrAtoI(dti->instance);
	valuator_name_instance = strchr(dti->instance, ',') + 1;		/* jump past the comma */

	if (valuator_name_instance != NULL) {
		valuator_name_instance += strspn(valuator_name_instance, whitespace);/* skip white */
		if (valuator_name_instance[0] == '-') {
			aux->valuator_map_sign[valuator_num] = -1.0;
			valuator_choice = vrAtoI(&(valuator_name_instance[1]));	/* skip the negation sign */
		} else {
			aux->valuator_map_sign[valuator_num] =  1.0;
			valuator_choice = vrAtoI(valuator_name_instance);
		}
	} else {
		valuator_choice = -1;
	}

	/* check the selected valuator */
	if (valuator_choice == -1) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_DTrackValuatorInput(): Warning, valuator['%s'] did not match any known valuator\n", dti->instance);
		return VRINPUT_MATCH_UNABLE;	/* device match, but still unable to handle request */
	} else if (unit_number >= UNITS_PT) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_DTrackValuatorInput(): Warning, valuator['%s'] cannot be mapped due to insufficient position tracker units (%d)\n", dti->instance, UNITS_PT);
		return VRINPUT_MATCH_UNABLE;	/* device match, but still unable to handle request */
	} else if (aux->valuator_inputs[valuator_num] != NULL) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_DTrackValuatorInput(): " RED_TEXT "Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);
	}

	/* make the mapping */
	aux->valuator_inputs[valuator_num] = (vrValuator *)input;
	aux->valuator_map_valuator[valuator_num] = valuator_choice;
	aux->valuator_map_tracker[valuator_num] = unit_number;
	aux->valuator_map_type[valuator_num] = tracker_type;
	vrDbgPrintfN(INPUT_DBGLVL, "_DTrackValuatorInput(): assigned valuator event of value %d (DTrack valuator %d of %s(%d)) to input pointer = %#p)\n",
		valuator_num, valuator_choice, dti->type, tracker_type, aux->valuator_inputs[valuator_num]);

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/**************************************************************************/
/* Here is a sample definition for a DTrack position sensor:              */
/*     input "tracker[1]" = "6sensor(fs2[0, r2e])";                       */
/* And since the numbers are 0-based, this means the 1st flystick-2 will  */
/* be mapped to the name "tracker[1]", and it will have the current       */
/* receiver-to-entity matrix applied to it.                               */
static vrInputMatch _DTrack6sensorInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
        _DTrackPrivateInfo	*aux = (_DTrackPrivateInfo *)devinfo->aux_data;
	int			tracker_num;		/* the current count of FreeVR tracker inputs */
	int			tracker_choice;		/* the chosen number for this tracker */
	int			tracker_type;		/* the type of dtrack tracker (e.g. a flystick-2) */

	/* The DTrack method of handling input arrays may be a combination of the way */
	/*   suggested by vr_input.skeleton.c as the FoB, MS & Fastrak input drivers. */

	/* select a tracker */
	tracker_num = aux->num_6sensors;
	aux->num_6sensors++;

	tracker_type = vrTrackerTypeValue(dti->type);
	tracker_choice = vrAtoI(dti->instance);

	/* check the selected tracker */
	if (tracker_choice == -1) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_DTrack6sensorInput(): Warning, tracker['%s'] did not match any known tracker\n", dti->instance);
		return VRINPUT_MATCH_UNABLE;	/* device match, but still unable to handle request */
	} else if (tracker_choice >= UNITS_PT) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_DTrack6sensorInput(): Warning, tracker['%s'] cannot be mapped due to insufficient position tracker units (%d)\n", dti->instance, UNITS_PT);
		return VRINPUT_MATCH_UNABLE;	/* device match, but still unable to handle request */
	} else if (aux->sensor6_inputs[tracker_num] != NULL) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_DTrack6sensorInput(): " RED_TEXT "Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);
	}

	/* make the mapping */
	aux->sensor6_inputs[tracker_num] = (vr6sensor *)input;
	aux->tracker_map_tracker[tracker_num] = tracker_choice;
	aux->tracker_map_type[tracker_num] = tracker_type;
	vrDbgPrintfN(INPUT_DBGLVL, "_DTrack6sensorInput(): assigned tracker event of value %d (DTrack tracker %d of type %s(%d)) to input pointer = %#p)\n",
		tracker_num, tracker_choice, dti->type, tracker_type, aux->sensor6_inputs[tracker_num]);

	/* the vrAssign6sensorR2Exform() function does the parsing of the next token */
	vrAssign6sensorR2Exform(aux->sensor6_inputs[tracker_num], strchr(dti->instance, ','));

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
static void _DTrackSystemPauseToggleCallback(vrInputDevice *devinfo, int value)
{
	_DTrackPrivateInfo	*aux = (_DTrackPrivateInfo *)devinfo->aux_data;

        if (value == 0)
                return;

	devinfo->context->paused ^= 1;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "DTrack Control: system_pause = %d.\n",
		devinfo->context->paused);
}

/************************************************************/
static void _DTrackPrintContextStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintContext(stdout, devinfo->context, verbose);
}

/************************************************************/
static void _DTrackPrintConfigStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintConfig(stdout, devinfo->context->config, verbose);
}

/************************************************************/
static void _DTrackPrintInputStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintInput(stdout, devinfo->context->input, verbose);
}

/************************************************************/
static void _DTrackPrintStructCallback(vrInputDevice *devinfo, int value)
{
        _DTrackPrivateInfo	*aux = (_DTrackPrivateInfo *)devinfo->aux_data;

        if (value == 0)
                return;

        _DTrackPrintStruct(stdout, aux, verbose);
}

/************************************************************/
static void _DTrackPrintHelpCallback(vrInputDevice *devinfo, int value)
{
        _DTrackPrivateInfo	*aux = (_DTrackPrivateInfo *)devinfo->aux_data;

        if (value == 0)
                return;

        _DTrackPrintHelp(stdout, aux);
}


	/*************************************************/
	/*   Callbacks for interfacing with the device.  */
	/*                                               */


/************************************************************/
static void _DTrackCreateFunction(vrInputDevice *devinfo)
{
	/*** List of possible inputs ***/
static	vrInputFunction	_DTrackInputs[] = {
				{ "fs1", VRINPUT_2WAY, _DTrackButtonInput },
				{ "fs2", VRINPUT_2WAY, _DTrackButtonInput },
				{ "fs2", VRINPUT_VALUATOR, _DTrackValuatorInput },	/* NOTE: the older flysticks (fs1) had no valuators */
				{ "6body", VRINPUT_6SENSOR, _DTrack6sensorInput },
				{ "fs2", VRINPUT_6SENSOR, _DTrack6sensorInput },
				{ NULL, VRINPUT_UNKNOWN, NULL } };
	/*** List of control functions ***/
static	vrControlFunc	_DTrackControlList[] = {
				/* overall system controls */
				{ "system_pause_toggle", _DTrackSystemPauseToggleCallback },

				/* informational output controls */
				{ "print_context", _DTrackPrintContextStructCallback },
				{ "print_config", _DTrackPrintConfigStructCallback },
				{ "print_input", _DTrackPrintInputStructCallback },
				{ "print_struct", _DTrackPrintStructCallback },
				{ "print_help", _DTrackPrintHelpCallback },
		/** TODO: other callback control functions go here **/
				/* other controls */

				/* end of the list */
				{ NULL, NULL } };

	_DTrackPrivateInfo	*aux = NULL;

	/******************************************/
	/* allocate and initialize auxiliary data */
	devinfo->aux_data = (void *)vrShmemAlloc0(sizeof(_DTrackPrivateInfo));
	aux = (_DTrackPrivateInfo *)devinfo->aux_data;
	_DTrackInitializeStruct(aux, devinfo->type);

	/******************/
	/* handle options */
	_DTrackParseArgs(aux, devinfo->args);

	/***************************************/
	/* create the inputs and self-controls */
	vrInputCountDataContainers(devinfo);

	aux->button_inputs = (vr2switch **)vrShmemAlloc((devinfo->num_2ways + devinfo->num_scontrols) * sizeof(vr2switch *));
	aux->button_map_bit = (int *)vrShmemAlloc0((devinfo->num_2ways + devinfo->num_scontrols) * sizeof(int));
	aux->button_map_tracker = (int *)vrShmemAlloc0((devinfo->num_2ways + devinfo->num_scontrols) * sizeof(int));
	aux->button_map_type = (int *)vrShmemAlloc0((devinfo->num_2ways + devinfo->num_scontrols) * sizeof(int));

	aux->valuator_inputs = (vrValuator **)vrShmemAlloc0((devinfo->num_valuators + devinfo->num_scontrols) * sizeof(vrValuator *));
	aux->valuator_map_valuator = (int *)vrShmemAlloc0((devinfo->num_valuators + devinfo->num_scontrols) * sizeof(int));
	aux->valuator_map_tracker = (int *)vrShmemAlloc0((devinfo->num_valuators + devinfo->num_scontrols) * sizeof(int));
	aux->valuator_map_type = (int *)vrShmemAlloc0((devinfo->num_valuators + devinfo->num_scontrols) * sizeof(int));
	aux->valuator_map_sign = (float *)vrShmemAlloc0((devinfo->num_valuators + devinfo->num_scontrols) * sizeof(float));

	aux->sensor6_inputs = (vr6sensor **)vrShmemAlloc((devinfo->num_6sensors) * sizeof(vr6sensor *));
	aux->tracker_map_tracker = (int *)vrShmemAlloc0((devinfo->num_6sensors) * sizeof(int));
	aux->tracker_map_type = (int *)vrShmemAlloc0((devinfo->num_6sensors) * sizeof(int));

	vrInputCreateDataContainers(devinfo, _DTrackInputs);
	vrInputCreateSelfControlContainers(devinfo, _DTrackInputs, _DTrackControlList);
	/* TODO: anything to do for NON self-controls?  Implement for Magellan first. */

	vrDbgPrintf("_DTrackCreateFunction(): Done creating DTrack inputs for '%s'\n", devinfo->name);
	devinfo->created = 1;

	return;
}


/************************************************************/
static void _DTrackOpenFunction(vrInputDevice *devinfo)
{
	vrTrace("_DTrackOpenFunction", devinfo->name);

	_DTrackPrivateInfo	*aux = (_DTrackPrivateInfo *)devinfo->aux_data;

	/*******************/
	/* open the device */
	aux->fd_socket = vrSocketCreateListenUDP(&(aux->data_port), 0);
	if (aux->fd_socket < 0) {
		aux->open = 0;
		devinfo->operating = 0;
		vrErrPrintf("(%s::_DTrackOpenFunction()::%d) error: "
			RED_TEXT "couldn't open UDP socket %d for %s\n" NORM_TEXT,
			__FILE__, __LINE__, aux->data_port, devinfo->name);
		sprintf(aux->version, "- unconnected DTrack -");
	} else {
		aux->open = 1;
		if (_DTrackInitializeDevice(aux) < 0) {
			devinfo->operating = 0;
			vrErrPrintf("_DTrackOpenFunction(): "
				RED_TEXT "Warning, unable to initialize '%s' DTrack.\n" NORM_TEXT,
				devinfo->name);
		} else {
			devinfo->operating = 1;
			vrDbgPrintf("_DTrackOpenFunction(): Done opening DTrack input device '%s' (operating = %d).\n", devinfo->name, devinfo->operating);
		}
	}

	return;
}


/************************************************************/
static void _DTrackCloseFunction(vrInputDevice *devinfo)
{
	_DTrackPrivateInfo	*aux = (_DTrackPrivateInfo *)devinfo->aux_data;
	int			count;							/* loop counter */

	vrDbgPrintf("_DTrackCloseFunction(): About to close DTrack input device '%s'.\n", devinfo->name);

	_DTrackCloseDevice(aux);

	if (aux != NULL) {
		if (aux->open)
			vrSocketClose(aux->fd_socket);
#ifdef FREEVR
		/* free the FreeVR specific fields of "aux" */
		/* TODO: ... */
#endif

		vrShmemFree(aux);			/* aka devinfo->aux_data */
	}

	return;
}


/************************************************************/
static void _DTrackResetFunction(vrInputDevice *devinfo)
{
	_DTrackPrivateInfo	*aux = (_DTrackPrivateInfo *)devinfo->aux_data;

	return;
}


/************************************************************/
static void _DTrackPollFunction(vrInputDevice *devinfo)
{
	_DTrackPrivateInfo	*aux = (_DTrackPrivateInfo *)devinfo->aux_data;

	if (devinfo->operating) {
		_DTrackGetData(devinfo);
	} else {
		/* TODO: try to open the device again */
	}

	return;
}



	/******************************/
	/*** FreeVR public routines ***/
	/******************************/


/******************************************************/
void vrDTrackInitInfo(vrInputDevice *devinfo)
{
	devinfo->version = (char *)vrShmemStrDup("-get from DTrack device-");
	devinfo->Create = vrCallbackCreateNamed("DTrack:Create-Def", _DTrackCreateFunction, 1, devinfo);
	devinfo->Open = vrCallbackCreateNamed("DTrack:Open-Def", _DTrackOpenFunction, 1, devinfo);
	devinfo->Close = vrCallbackCreateNamed("DTrack:Close-Def", _DTrackCloseFunction, 1, devinfo);
	devinfo->Reset = vrCallbackCreateNamed("DTrack:Reset-Def", _DTrackResetFunction, 1, devinfo);
	devinfo->PollData = vrCallbackCreateNamed("DTrack:PollData-Def", _DTrackPollFunction, 1, devinfo);
	devinfo->PrintAux = vrCallbackCreateNamed("DTrack:PrintAux-Def", _DTrackPrintStruct, 0);

	vrDbgPrintfN(INPUT_DBGLVL, "vrDTrackInitInfo(): callbacks created.\n");
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


/*******************************************************************/
void exit_testapp()
{
	done = 1;
}


/*******************************************************************/
/* A test program to communicate with a DTrack device and print the results. */
main(int argc, char *argv[])
{
	_DTrackPrivateInfo	*aux;
	char			*progname;			/* name of the program executable */
	int			count;				/* for looping over things */
	int			nodata_flag = 0;		/* whether or not to actual show input data */

	done = 0;
	signal(SIGINT, exit_testapp);


	/******************************/
	/* setup the device structure */
	aux = (_DTrackPrivateInfo *)malloc(sizeof(_DTrackPrivateInfo));
	memset(aux, 0, sizeof(_DTrackPrivateInfo));
	_DTrackInitializeStruct(aux, "DTracksubtype");


	/*********************************************************/
	/* set default parameters based on environment variables */
	aux->server_host = getenv("DTRACK_HOST");
	if (aux->server_host == NULL)
		aux->server_host = DEFAULT_CMDHOST;		/* default, if no host given */
	aux->server_port = ((getenv("DTRACK_PORT")==NULL) ? DEFAULT_CMDPORT : atoi(getenv("DTRACK_PORT")));


	/*****************************************************/
	/* adjust parameters based on command-line-arguments */
	progname = argv[0];
	while ((argc > 1) && (argv[1][0] == '-')) {
		/* Report information about the device and then quit */
		/* NOTE: the "-nodata" option already basically does a "list" of what's coming from the DTrack server */
		if (!strcmp(argv[1], "-nodata") || !strcmp(argv[1], "-list")) {
			nodata_flag = 1;
			argv++; argc--;
		}

		/* Unknown option */
		else {
			/* There are currently no other "-" options, so this is an error */
			fprintf(stderr, RED_TEXT "Usage:" NORM_TEXT " %s [-list | -nodata] [<server host> (default = '%s')]\n", progname, aux->server_host);	/* NOTE: I'm reporting the default based on what might be changed by the environment variable */
			exit(1);
		}
	}

	/* if there are any arguments left, use the first as the device file path */
	if (argc > 1) {
		aux->server_host = strdup(argv[1]);
	}


	/******************************************************/
	/* open the UDP socket port and initialize the device */
	aux->fd_socket = vrSocketCreateListenUDP(&(aux->data_port), 1);
	if (aux->fd_socket < 0) {
		aux->open = 0;
		fprintf(stderr, RED_TEXT "couldn't open UDP socket '%d'\n" NORM_TEXT, aux->data_port);
		sprintf(aux->version, "- unconnected DTrack -");
	} else {
		aux->open = 1;
		if (_DTrackInitializeDevice(aux) < 0) {
			vrErrPrintf("main: " RED_TEXT "Warning, unable to initialize DTrack.\n" NORM_TEXT);
		}
	}

	_DTrackPrintStruct(stdout, aux, verbose);

	/* quit if flagged to print the device info but not data */
	if (nodata_flag)
		exit(0);

	/****************************************/
	/* read the data and display the output */
	while(aux->open && !done) {
		if (_DTrackReadInput(aux) > 0) {

			/* TODO: offer a screen-rendering (curses) option */

			/* loop over the standard 6-dof bodies */
			for (count = 0; count < UNITS_PT; count++) {
				if (aux->units_6body[count].frame > 0) {
					printf("6d (%d): frame = %d, qual = %.2f -- %f %f %f\n",
						count,
						aux->units_6body[count].frame, 
						aux->units_6body[count].quality, 
						aux->units_6body[count].location[0], 
						aux->units_6body[count].location[1], 
						aux->units_6body[count].location[2]);
				}
			}

			/* loop over the flystick-2 bodies */
			for (count = 0; count < UNITS_PT; count++) {
				if (aux->units_fs2[count].frame > 0) {
					printf("6df2 (%d): frame = %d, qual = %.2f -- %f %f %f  but: %2d  val-1 %4f  val-2 %4f\n",
						count,
						aux->units_fs2[count].frame, 
						aux->units_fs2[count].quality, 
						aux->units_fs2[count].location[0], 
						aux->units_fs2[count].location[1], 
						aux->units_fs2[count].location[2],
						aux->units_fs2[count].buttons,
						aux->units_fs2[count].valuators[0],
						aux->units_fs2[count].valuators[1]);
				}
			}

			/* TODO: loop over the other types of position inputs */

		}
	}
	printf("\n");


	/*****************/
	/* close up shop */
	if (aux->open) {
		_DTrackCloseDevice(aux);
	}

	if (aux != NULL) {
		if (aux->open)
			vrSocketClose(aux->fd_socket);

		free(aux);
	}

	vrDbgPrintf(BOLD_TEXT "\nDTrack device closed\n" NORM_TEXT);
}

#endif /* } TEST_APP */



	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/



#if defined(CAVE) /* { */


	/* ... CAVE stuff here if to also work with CAVElib */


#endif /* } CAVE */

