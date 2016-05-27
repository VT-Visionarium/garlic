/* ======================================================================
 *
 *  CCCCC          vr_input.vruidd.c
 * CC   CC         Author(s): Bill Sherman
 * CC              Created: October 16, 2009
 * CC   CC         Last Modified: September 16, 2013
 *  CCCCC
 *
 * Code file for FreeVR inputs from the VruiDD input device.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/*************************************************************************
USAGE:
	...

	Inputs are specified with the "input" option:
		input "<name>" = "2switch(button[<number>])";
		input "<name>" = "valuator(valuator[<number>])";
		input "<name>" = "6sensor({tracker|6sensor}[<number>])";
	  XX	input "<name>" = "Nswitch(switch[<number>])"; -- The VruiDD has no concept of N-switch
	  XX	input "<name>" = "Nsensor(glove[<number>])"; -- The VruiDD has no concept of N-sensor

	Controls are specified with the "control" option:
		control "<control option>" = "2switch(...)";
		control "<control option>" = "valuator(...)"; -- NOTE: no valuator oriented controls yet available for this device

	Here are the available control options for FreeVR:
		"print_help" -- print info on how to use the input device
		"system_pause_toggle" -- toggle the system pause flag
		"print_context" -- print the overall FreeVR context data structure (for debugging)
		"print_config" -- print the overall FreeVR config data structure (for debugging)
		"print_input" -- print the overall FreeVR input data structure (for debugging)
		"print_struct" -- print the internal VruiDD data structure (for debugging)

	Here are the FreeVR configuration argument options for the VruiDD:
		"host" - machine running the Vrui VRDeviceDaemon.
			("localhost" is the default)
		"port" - socket port of server serving the tracking data.
			(8555 is the default)
		"protocol" - the version of the Vrui Protocol in use
		"valscale" - scaling factor used to tune the valuator input range
		"transscale"/"scale" - set the scaling factor for the location
			of the 6-sensor inputs.

NOTES:
	The protocol is "documented in code" in the Vrui source, which can
	make it difficult to discern what's going on.  Also, Oliver Kreylos
	has begun changing the protocol between Vrui 1.x and Vrui 2.x versions
	of the Vrui library.

	Protocol 0 -- Vrui-1.x:
		APP: connect to socket
		APP: send 0x00 0x00 (CONNECT_REQUEST)
		DD: send 0x01 0x00 (CONNECT_REPLY) + 12 bytes w/ num trackers, buttons & valuators
		APP: send 0x03 0x00 (ACTIVATE_REQUEST)
		APP: send 0x07 0x00 (STARTSTREAM_REQUEST)
		DD: send continuous packets of 0x06 0x00 (PACKET_REPLY) + N bytes w/ input data
		APP: send 0x08 0x00 (STOPSTREAM_REQUEST)
		DD: send 0x06 0x00 (PACKET_REPLY) + N bytes w/ input data
		APP: send 0x04 0x00 (DEACTIVATE_REQUEST)
		APP: send 0x02 0x00 (DISCONNECT_REQUEST)
		DD: send 0x09 0x00 (STOPSTREAM_REPLY)
		APP: disconnect from socket

		(where N = 1 * <num buttons> + 4 * <num valuators> + 4*13 * <num trackers>)

		The tracker values are somehow defined in "Vrui/VRDeviceState.h",
		but I can't quite figure out how 7 values are contained in the
		"PositionOrientation" field.

	Protocol 1 -- Vrui-2.x:
		APP: connect to socket
		APP: send 0x00 0x00 (CONNECT_REQUEST)
		APP: send 0x01 0x00 0x00 0x00 (ie. the numeric code of this protocol -- "1")
		DD: send 0x01 0x00 (CONNECT_REPLY) + 4 bytes protocol value + 12 bytes w/ num trackers, buttons & valuators
		APP: send 0x03 0x00 (ACTIVATE_REQUEST)
		APP: send 0x07 0x00 (STARTSTREAM_REQUEST)
		DD: send continuous packets of 0x06 0x00 (PACKET_REPLY) + N bytes w/ input data
		APP: send 0x08 0x00 (STOPSTREAM_REQUEST)
		DD: send 0x06 0x00 (PACKET_REPLY) + N bytes w/ input data
		APP: send 0x04 0x00 (DEACTIVATE_REQUEST)
		APP: send 0x02 0x00 (DISCONNECT_REQUEST)
		DD: send 0x09 0x00 (STOPSTREAM_REPLY)
		APP: disconnect from socket

	So in the end it turns out the only difference between Protocol-0 and
	Protocol-1 is that there is a two-way exchange of 4 bytes to give a
	protocol number!


	Protocol 2 -- Vrui-3.x:

	Vrui version 3.0-001 introduced a new protocol for its VRDeviceDaemon:
	protocol-2!  It is mostly the same as protocol-1, except that it has
	a segment in the "header" that enumerates some virtual devices and
	specifies a mapping of the physical inputs to the virtual devices
	(which can be thought of as "props").  Once the mapping is read, the
	rest of protocol-2 is identical to protocol-1.

	So the only difference is the amount of data that is returned as
	part of the CONNECT_REPLY (0x01 0x00) packet:
		DD: send 0x01 0x00 (CONNECT_REPLY) + 4 bytes protocol value + 12 bytes w/ num trackers, buttons & valuators
			+ 4 bytes w/ number of virtual devices
	this is followed by a sequence of bytes with a length that can only
	be determined by parsing the data:
			- stringSequence -- name of virtual device
			- 4-byte integer -- trackType (0=None, 1=3D, 3=Ray, 7=6D)
			- 3 * 4-byte floats -- rayDirection X,Y,Z
			- 4-byte number -- it's always been zero, so I have no idea what it represents
			- 4-byte integer -- trackerIndex (-1 when trackType==None)
			- 4-byte integer -- numButtons
			- numButtons * stringSequence -- name of buttons
			- numButtons * 4-byte integer -- index of buttons
			- 4-byte integer -- numValuators
			- numValuators * stringSequence -- name of valuators
			- numValuators * 4-byte integer -- index of valuators

		where "stringSequence" is a 4-byte integer followed by ASCII bytes w/o a trailing '\0'

HISTORY:
	16-20 October 2009 (Bill Sherman) -- wrote initial version using the
		example code from vr_input.skeleton.c.

	28-29 October 2009 (Bill Sherman) -- corrected a couple of issues with
		the 6-sensor configuration options (using the "r2e" value,
		plus getting the quaternion orientation correct).  Added the
		"transscale" configuration option.

	18 August 2010 (Bill Sherman) -- changed the allocation for the
		inputs to use the '0' version of vrShmemAlloc0() so each will
		start with a known (0) state.  Also a slight improvement to
		_VruiDDPrintStruct().  Plus fixed a bug whereby the last
		button wasn't being updated.

	19 July 2011 (Bill Sherman) -- fixed a simple, but massive bug where
		sizeof() was called on a math expression rather than a variable.

	30 March - 2 April 2012 (Bill Sherman) -- Added the ability to handle
		the "new" Vrui VRDeviceDaemon protocol -- which in the end was
		just an exchange of protocol numbers.

	14 September 2013 (Bill Sherman)
		I am making use of the newly created SELFCTRL_DBGLVL for all
		the self-control outputs.  I then added some reasonable output
		to the "print_help" callback to print out what all the device's
		inputs are doing.

	15 September 2013 (Bill Sherman)
		Changed the "0x%p" format to the improved "%#p" format.

	16-17 September 2013 (Bill Sherman)
		I reverse engineered the new Vrui VRDeviceDaemon protocol-2.
		As part of this I just read the new data in the header, print
		it, and then ignore it -- which will still work both for the
		standalone "vruiddtest" program and for FreeVR input parsing.
		What it doesn't yet do is allow me to configure the FreeVR
		input to take advantage of the virtual devices.

		NOTE: it's possible that the new protocol mimics in some way
		what Oliver was doing with the record and playback input
		scheme.

		I added command line argument parsing for "vruiddtest" (-p,
		-prot, -nodata -- and host requires no switch).

TODO:
	- integrate the protocol-2 virtual devices into the FreeVR input
		system.

		- as part of this, I'll probably want to add some socket
			stream data parsing routines that 1) checks that there
			is enough data in the buffer, read the data out
			of the buffer and do the conversion to int/float/string,
			and then shift and shrink the buffer.

		- also as part of this, have the virtual devices reported as
			part of _VruiDDPrintStruct(), and therefore don't
			just print the values as they are parsed.

	- The "vruiddtest" program always starts with these messages:
		_VruiDDInitializeDevice(): Got a bad read result.
		_VruiDDInitializeDevice(): Resource temporarily unavailable

		fix this!

	- should cleanly address the issue of data endianness over the socket.
		(Currently, if daemon and client run on same architecture, it
		should work fine.  [04/03/12: Actually, I'm not sure that's true.
		The parsing of numbers in _VruiDDInitializeDevice() assumes the
		bytes are in little-endian order.  Reading the rest of the data
		should be fine on same-architectured machines, but this part
		not.]

	- If unable to connect to the daemon right away, would be nice to
		keep checking and connect when possible.

	- DONE: Add command-line-arguments for "vruiddtest":
		* host (no switch needed)
		* port
		* protocol
		* nodata -- just read the header (print info) and quit

	- DONE: write the man page for "vruiddtest"

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
#include <stdint.h>		/* needed for uint16_t & uint32_t types */
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
#  include <stdlib.h>		/* needed for malloc(), getenv() & atoi() */
#  include "vr_socket.c"
#  include "vr_enums.h"
#  include "vr_utils.c"		/* needed for vrSleep() */
#  define vrShmemAlloc0 malloc
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

#define	DEFAULT_HOST	"localhost"
#define	DEFAULT_PORT	8555


/** VruiDD command sequences **/
enum MessageId {
		CONNECT_REQUEST,        /* Request to connect to server (0x00 0x00) */
		CONNECT_REPLY,          /* Positive connect reply with server layout (0x01 0x00) */
		DISCONNECT_REQUEST,     /* Polite request to disconnect from server (0x02 0x00) */
		ACTIVATE_REQUEST,       /* Request to activate server (prepare for sending packets) (0x03 0x00) */
		DEACTIVATE_REQUEST,     /* Request to deactivate server (no more packet requests) (0x04 0x00) */
		PACKET_REQUEST,         /* Requests a single packet with current device state (0x05 0x00) */
		PACKET_REPLY,           /* Sends a device state packet (0x06 0x00) */
		STARTSTREAM_REQUEST,    /* Requests entering stream mode (server sends packets automatically) (0x07 0x00) */
		STOPSTREAM_REQUEST,     /* Requests leaving stream mode (0x08 0x00) */
		STOPSTREAM_REPLY        /* Server's reply after last stream packet has been sent (0x09 0x00) */
	};

/** VruiDD bit-masks & indices **/
/* these are often used to decode specific bits that map to individual buttons */
#define VruiDD_BUTTON_x1x	0x01
#define VruiDD_BUTTON_x2x	0x02
#define VruiDD_BUTTON_x3x	0x04

#define VruiDD_BUTTONINDEX_x1x	0x00
#define VruiDD_BUTTONINDEX_x2x	0x01
#define VruiDD_BUTTONINDEX_x3x	0x02

/* VruiDD sensitivity values */
#define TRANS_SENSITIVITY	0.001
#define ROT_SENSITIVITY		0.02
#define VALUATOR_SENSITIVITY	1.0



/****************************************************************/
/*** auxiliary structure of the current data from the device. ***/
typedef struct {
		/* these are for interfacing with the daemon */
		int		fd_socket;	/* communication file descriptor */
		char		*dd_host;	/* name of daemon's host */
		int		dd_port;	/* port on host listening for application connections */
		int		open;		/* flag with VruiDD successfully open */

		/* these are for internal data parsing */
		unsigned char	buf[BUFSIZE];	/* the incoming data stream */
		int		eobuf_pos;	/* the last read byte in the buffer */
#if 0
		unsigned char	packtype;
		int		packlen;
#endif
		char		version[256];	/* self-reported version of the device */
		char		op_params[256];	/* operating parameters of the device (according to it) */
		int		protocol;	/* An integer indicating the Vrui VRDeviceDaemon protocol */

		/* information about the current values */
		int		packet_size;		/* the number of bytes in each packet reported by the Daemon */
		int		num_dd_buttons;		/* number of buttons reported by the Vrui Device Daemon */
		int		*incoming_buttons;	/* button data coming from the device daemon -- allocated in device initialization */
		int		*incoming_buttons_prev;	/* previous values for button data from the device daemon -- allocated in device initialization */
		int		num_dd_valuators;	/* number of valuator reported by the Vrui Device Daemon */
		double		*incoming_valuators;	/* valuator data coming from the device daemon -- allocated in device initialization */
		double		*incoming_valuators_prev;/* previous values for valuator data from the device daemon -- allocated in device initialization */
		int		num_dd_trackers;	/* number of position trackers reported by the Vrui Device Daemon */
		float		*incoming_trackers[13];	/* each incoming position-tracker has 13 floats */

		int		num_dd_virtdevs;	/* number of virtual devices sent by the Vrui Device Daemon */

#if 0 /* possible other items to handle (from vr_input.skeleton.c) */
		int		timer;			/* incoming timer info  TODO: or is it a checksum?*/
		int		button_change;		/* boolean indicator if button values have changed (may be an array if each change is independent) */
		int		valuator_change;	/* boolean indicator of change in valuator values (ditto) */
		int		receiver_change;	/* boolean indicator of change in 6sensor values (ditto) */
#endif

		/* information about how to filter the data for use with the VR library (FreeVR or CAVE) */
		float		scale_valuator;		/* scaling factor for valuators */
		float		scale_trans;		/* multiplier to scale from the VruiDD location units (inches) */

		/* information about the inputs and mappings */
#ifdef CAVE
		/* CAVE specific fields here */

#elif defined(FREEVR)
		/* FREEVR specific fields here */
		int		num_buttons;		/* it is often wise to store the number of button inputs in addition to knowing the maximum possible */
		vr2switch	**button_inputs;	/* the maximum number of buttons is devinfo->num_2ways + devinfo->num_scontrols (all the self-controls are not necessarily buttons) */
		int		*button_map_button;	/* An array of numbers indicating the particular button */
		int		num_switches;
		vrNswitch	**switch_inputs;
		int		num_valuators;
		vrValuator	**valuator_inputs;
		int		*valuator_map_valuator;	/* An array of numbers indicating the particular valuator */
		float		*valuator_sign;		/* An array of numbers indicating the sign of each valuator input */
		int		num_6sensors;
		vr6sensor	**sensor6_inputs;
		int		*tracker_map_tracker;	/* An array of numbers indicating the particular tracker */
		int		num_Nsensors;
		vrNsensor	**sensorN_inputs;

#endif /* end library-specific fields */

	} _VruiDDPrivateInfo;



	/*********************************************/
	/*** General NON public interface routines ***/
	/*********************************************/

/******************************************************/
/* typename is used to specify a particular device among many that */
/*   share (more or less) the same protocol.  The typename is then */
/*   used to determine what specific features are available with   */
/*   this particular type of device.                               */
static void _VruiDDInitializeStruct(_VruiDDPrivateInfo *aux, char *typename)
{
	aux->version[0] = '\0';
	aux->op_params[0] = '\0';

	aux->buf[0] = '\0';
	aux->eobuf_pos = 0;

	aux->protocol = 0;

#if 0 /* possible other items to handle (from vr_input.skeleton.c) */
	aux->timer = 0;
	aux->button_change = 0;
	aux->valuator_change = 0;
	aux->receiver_change = 0;
#endif

	aux->scale_valuator = VALUATOR_SENSITIVITY;	/* set the default valuator scaling factor */
	aux->scale_trans = 1.0/12.0;			/* convert from inches to feet by default */

	/* everything else is zero'd by default */
}


/******************************************************/
static void _VruiDDPrintStruct(FILE *file, _VruiDDPrivateInfo *aux, vrPrintStyle style)
{
	int	count;
	int	ptv_count;

	vrFprintf(file, "VruiDD device internal structure (%#p):\n", aux);
	vrFprintf(file, "\r\tversion -- '%s'\n", aux->version);
	vrFprintf(file, "\r\toperating parameters -- '%s'\n", aux->op_params);
	vrFprintf(file, "\r\tfd_socket = %d\n\thost = '%s'\n\tport = %d\n\topen = %d\n",
		aux->fd_socket,
		aux->dd_host,
		aux->dd_port,
		aux->open);

	/* print the filter values */
	vrFprintf(file, "\r\tscale_valuator = %f\n", aux->scale_valuator);
	vrFprintf(file, "\r\tscale_trans = %f\n", aux->scale_trans);

	/* print the raw values */
	vrFprintf(file, "Buttons (%d): ", aux->num_dd_buttons);
	for (count = 0; count < aux->num_dd_buttons; count++) {
		vrFprintf(file, "%c", (aux->incoming_buttons[count] == 0 ? '.' : 'X'));
	}
	vrFprintf(file, "\n");

	vrFprintf(file, "Valuators (%d): ", aux->num_dd_valuators);
	for (count = 0; count < aux->num_dd_valuators; count++) {
		vrFprintf(file, "%6.3f ", aux->incoming_valuators[count]);
	}
	vrFprintf(file, "\n");

	vrFprintf(file, "Position-Trackers (%d): ", aux->num_dd_trackers);
	for (count = 0; count < aux->num_dd_trackers; count++) {
		vrFprintf(file, "\n\tPT %d: ", count);
		for (ptv_count = 0; ptv_count < (count == 0 ? 13:3); ptv_count++) {	/* TODO: s/b 13, but doing 3 for now */
			vrFprintf(file, "%6.3f ", aux->incoming_trackers[ptv_count][count]);
		}
	}
	vrFprintf(file, "\n");

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
static void _VruiDDPrintHelp(FILE *file, _VruiDDPrivateInfo *aux)
{
	int	count;		/* looping variable */

#if 0 || defined(TEST_APP)
	vrFprintf(file, BOLD_TEXT "Sorry, VruiDD - print_help control not yet implemented.\n" NORM_TEXT);
#else
	vrFprintf(file, BOLD_TEXT "VruiDD - inputs:" NORM_TEXT "\n");
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
/* _VruiDDReadInput(): function reads data from the socket, and then looks */
/*   for and parses packets from the daemon.  The data is then placed into */
/*   the generic portion of the "_VruiDDPrivateInfo" structure.            */
/* NOTE: _VruiDDReadInput() is placed above the __VruiDDInitializeDevice() */
/*   function to allow the latter to call the former.                      */
static int _VruiDDReadInput(_VruiDDPrivateInfo *aux)
/* Returns number of packets read this time (-1 indicates reception of the STOPSTREAM_REPLY message) */
{
	uint16_t	message;		/* VRUI VRDeviceDaemon messages are 2 byte sequences */
	int		read_result;		/* return value of the call to "read()" */
	int		packets_decoded = 0;	/* return the number of packets that are decoded */
	int		decode_pos = 0;		/* position in the buffer currently being decoded */
	uint8_t		bool_value;		/* place holder for any button data being processed */
	float		float_value;		/* place holder for any valuator or sub-tracker data being processed */
	int		count;			/* loop counter */
	int		ptv_count;		/* loop counter for position-tracker values */

	/*** read available data into available buffer space ***/
	do {
		read_result = (ssize_t)read(aux->fd_socket, &aux->buf[aux->eobuf_pos], sizeof(aux->buf) - aux->eobuf_pos);
		if (read_result > 0) {
			aux->eobuf_pos += read_result;
#ifdef COMM_DEBUG
			vrDbgPrintf("_VruiDDReadInput(): %d bytes currently in buffer.\n", aux->eobuf_pos);
#endif
		}
	} while (read_result > 0);
#ifdef COMM_DEBUG
if (aux->eobuf_pos > 0)
vrPrintf("yoyo about to decode: eobuf_pos = %d, packet_size = %d\n", aux->eobuf_pos, aux->packet_size);
#endif

	/*** decode data contained in the buffer ***/
	while (aux->eobuf_pos >= 2) {
		message = (aux->buf[0] + (aux->buf[1] << 8));

		if (message == PACKET_REPLY) {
			/* decode only if we have enough data for at least one packet */
			if (aux->eobuf_pos >= aux->packet_size) {
				decode_pos = 2;		/* start decoding right after the Daemon message packet */

				/* loop over all the position-trackers */
				for (count = 0; count < aux->num_dd_trackers; count++) {
					for (ptv_count = 0; ptv_count < 13; ptv_count++) {
						((char *)(&float_value))[0] = aux->buf[decode_pos+0];		/* NOTE: dangerous assumption of 4-byte floats here */
						((char *)(&float_value))[1] = aux->buf[decode_pos+1];
						((char *)(&float_value))[2] = aux->buf[decode_pos+2];
						((char *)(&float_value))[3] = aux->buf[decode_pos+3];

						aux->incoming_trackers[ptv_count][count] = float_value;
						decode_pos += sizeof(float);
					}
				}

				/* loop over all the buttons */
				for (count = 0; count < aux->num_dd_buttons; count++) {
					bool_value = aux->buf[decode_pos+0];

					aux->incoming_buttons[count] = bool_value;
					decode_pos += sizeof(uint8_t);
				}

				/* loop over all the valuators */
				for (count = 0; count < aux->num_dd_valuators; count++) {
					((char *)(&float_value))[0] = aux->buf[decode_pos+0];			/* NOTE: dangerous assumption of 4-byte floats here */
					((char *)(&float_value))[1] = aux->buf[decode_pos+1];
					((char *)(&float_value))[2] = aux->buf[decode_pos+2];
					((char *)(&float_value))[3] = aux->buf[decode_pos+3];

					aux->incoming_valuators[count] = float_value;
					decode_pos += sizeof(float);
				}

				memmove(aux->buf, &aux->buf[aux->packet_size], (aux->eobuf_pos-aux->packet_size));	/* shift the data by one packet */
				aux->eobuf_pos -= aux->packet_size;
				packets_decoded++;
			} else {
				break;;		/* skip out of the while loop (or we get stuck) */
			}
		} else if (message == STOPSTREAM_REPLY) {
			memmove(aux->buf, &aux->buf[2], (aux->eobuf_pos-2));					/* shift the data by two bytes */
			aux->eobuf_pos -= 2;
			vrDbgPrintf("_VruiDDReadInput(): received a STOPSTREAM_REPLY message from the daemon -- stopping.\n");
			return (-1);
		} else {
			vrErrPrintf("_VruiDDReadInput(): " RED_TEXT "Expected a 'PACKET_REPLY' message, got %02x, skipping two bytes.\n" NORM_TEXT, message);
			memmove(aux->buf, &aux->buf[2], (aux->eobuf_pos-2));	/* shift the data by two bytes */
			aux->eobuf_pos -= 2;
#if defined(COMM_DEBUG) || 1
			vrDbgPrintf("_VruiDDReadInput(): %d bytes currently in buffer.\n", aux->eobuf_pos);
{ int count;
for (count = 0; count < aux->eobuf_pos; count++)
	vrPrintf("%02x ", aux->buf[count]);
vrPrintf("\n");
}
#endif
		}
	}

#ifdef COMM_DEBUG
if (packets_decoded > 0)
vrPrintf("_VruiDDReadInput(): %d packets decoded this call.\n", packets_decoded);
#endif
	return (packets_decoded);
}


/**********************************************************/
/* _VruiDDInitializeDevice() is called in the OPEN phase  */
/*   of input interface -- after the types of inputs have */
/*   been determined (during the CREATE phase).           */
static int _VruiDDInitializeDevice(_VruiDDPrivateInfo *aux)
{
	uint16_t	message;	/* VRUI VRDeviceDaemon messages are 2 byte sequences */
	uint32_t	value;		/* VRUI 4-byte value */
	int		read_result;	/* return value of the call to "read()" */
	int		bytes_read = 0;	/* number of bytes read from socket (the Vrui Daemon) thus far */
	int		opening_bytes = 14;	/* the number of bytes to expect in the CONNECT_REPLY response */
	int		response_protocol;	/* the value the VRDeviceDaemon reports as the used protocol */
	int		count;		/* loop counter */
static	int		bad_reads = 0;	/* bad read counter -- used to try a jump-start after 5 bad reads */

	if (aux == NULL) {
		vrErrPrintf("_VruiDDInitializeDevice(): " RED_TEXT "Warning, no auxiliary data for VruiDD.\n" NORM_TEXT);
		return -1;
	}

	if (fcntl(aux->fd_socket, F_SETFL, O_RDWR | O_NONBLOCK | O_NOCTTY) < 0) {
                vrErrPrintf("_VruiDDInitializeDevice(): An error occurred while trying to set the file controls.\n");
        }


	/***********************************************/
	/* send the opening hail message to the daemon */
	message = CONNECT_REQUEST;
	write(aux->fd_socket, &message, sizeof(uint16_t));	/* TODO: warning, no effort yet being made to deal with endianness of the order */
	if (aux->protocol > 0) {
		value = aux->protocol;
		write(aux->fd_socket, &value, sizeof(uint32_t));        /* TODO: warning -- endianness */ /* this is protocol version info */
	}
	fsync(aux->fd_socket);

	/*****************************************************************/
	/* now await the data with information about the incoming data   */
	/* The first packet of information should be 14 bytes in length: */
	/*   - 2 bytes with 0x01 0x00 (CONNECT_REPLY)                    */
	/*   - 4 bytes with the protocol value response (prot > 0)       */
	/*   - 4 bytes with number of position trackers (aka 6-sensors)  */
	/*   - 4 bytes with number of buttons                            */
	/*   - 4 bytes with number of valuators                          */
	switch (aux->protocol) {
	case 0:
		/* The first packet of information should be 14 bytes in length: */
		/*   - 2 bytes with 0x01 0x00 (CONNECT_REPLY)                    */
		/*   - 4 bytes with number of position trackers (aka 6-sensors)  */
		/*   - 4 bytes with number of buttons                            */
		/*   - 4 bytes with number of valuators                          */
		opening_bytes = 14;
		break;
	case 1:
		/* The first packet of information should be 18 bytes in length: */
		/*   - 2 bytes with 0x01 0x00 (CONNECT_REPLY)                    */
		/*   - 4 bytes with the protocol value response                  */
		/*   - 4 bytes with number of position trackers (aka 6-sensors)  */
		/*   - 4 bytes with number of buttons                            */
		/*   - 4 bytes with number of valuators                          */
		opening_bytes = 18;
		break;
	case 2:
		/* The first packet of information should be 18 bytes in length: */
		/*   - 2 bytes with 0x01 0x00 (CONNECT_REPLY)                    */
		/*   - 4 bytes with the protocol value response                  */
		/*   - 4 bytes with number of position trackers (aka 6-sensors)  */
		/*   - 4 bytes with number of buttons                            */
		/*   - 4 bytes with number of valuators                          */
		/*   - 4 bytes with number of virtual devices                    */
		opening_bytes = 22;
		break;
	}

	/* wait until the entire opening bytes sequence is received */
	while (bytes_read < opening_bytes) {
		read_result = (ssize_t)read(aux->fd_socket, &aux->buf[aux->eobuf_pos], sizeof(aux->buf) - aux->eobuf_pos);
		if (read_result > 0) {
			bytes_read += read_result;
			aux->eobuf_pos += read_result;
			bad_reads = 0;
		} else if (read_result < 0) {
			/* I don't think this should happen with a working VruiDD */
			bad_reads++;
			vrDbgPrintf("_VruiDDInitializeDevice(): Got a bad read result.\n", bytes_read);
			perror("_VruiDDInitializeDevice()");
			vrSleep(100000);

			if (bad_reads > 5) {
				vrDbgPrintf("_VruiDDInitializeDevice(): 5 bad reads -- attempting to send protocol-1 message.\n");
				bad_reads = 0;
				aux->protocol = 1;
				opening_bytes = 18;
				value = aux->protocol;
				write(aux->fd_socket, &value, sizeof(uint32_t));        /* TODO: warning -- endianness */ /* this is protocol version info */
				fsync(aux->fd_socket);
			}
		}

#ifdef COMM_DEBUG
		vrDbgPrintf("_VruiDDInitializeDevice(): %d bytes read thus far.\n", bytes_read);
#endif
	}

	/* verify expected opening response received (assuming LITTLE_ENDIAN for now) */
	message = (aux->buf[0] + (aux->buf[1] << 8));
	if (message == CONNECT_REPLY) {
		vrDbgPrintf("_VruiDDInitializeDevice(): Got a CONNECT_REPLY packet, will now decode.\n");

		switch (aux->protocol) {
		default:
		case 0:
			/* protocol == 0 */
			response_protocol = 0;

			/* number of 6-sensors */
			value = (aux->buf[2] + (aux->buf[3] << 8) + (aux->buf[4] << 16) + (aux->buf[5] << 24));
			aux->num_dd_trackers = value;

			/* number of buttons */
			value = (aux->buf[6] + (aux->buf[7] << 8) + (aux->buf[8] << 16) + (aux->buf[9] << 24));
			aux->num_dd_buttons = value;

			/* number of valuators */
			value = (aux->buf[10] + (aux->buf[11] << 8) + (aux->buf[12] << 16) + (aux->buf[13] << 24));
			aux->num_dd_valuators = value;

			break;

		case 1:
			/* protocol response */
			value = (aux->buf[2] + (aux->buf[3] << 8) + (aux->buf[4] << 16) + (aux->buf[5] << 24));
			response_protocol = value;
			vrPrintf("_VruiDDInitializeDevice(): Got a response protocol value of %d (requested %d).\n", response_protocol, aux->protocol);

			/* number of 6-sensors */
			value = (aux->buf[6] + (aux->buf[7] << 8) + (aux->buf[8] << 16) + (aux->buf[9] << 24));
			aux->num_dd_trackers = value;

			/* number of buttons */
			value = (aux->buf[10] + (aux->buf[11] << 8) + (aux->buf[12] << 16) + (aux->buf[13] << 24));
			aux->num_dd_buttons = value;

			/* number of valuators */
			value = (aux->buf[14] + (aux->buf[15] << 8) + (aux->buf[16] << 16) + (aux->buf[17] << 24));
			aux->num_dd_valuators = value;

			break;
		case 2:
			/* protocol response */
			value = (aux->buf[2] + (aux->buf[3] << 8) + (aux->buf[4] << 16) + (aux->buf[5] << 24));
			response_protocol = value;
			vrPrintf("_VruiDDInitializeDevice(): Got a response protocol value of %d (requested %d).\n", response_protocol, aux->protocol);

			/* number of 6-sensors */
			value = (aux->buf[6] + (aux->buf[7] << 8) + (aux->buf[8] << 16) + (aux->buf[9] << 24));
			aux->num_dd_trackers = value;

			/* number of buttons */
			value = (aux->buf[10] + (aux->buf[11] << 8) + (aux->buf[12] << 16) + (aux->buf[13] << 24));
			aux->num_dd_buttons = value;

			/* number of valuators */
			value = (aux->buf[14] + (aux->buf[15] << 8) + (aux->buf[16] << 16) + (aux->buf[17] << 24));
			aux->num_dd_valuators = value;

			/* number of virtual devices */
			value = (aux->buf[18] + (aux->buf[19] << 8) + (aux->buf[20] << 16) + (aux->buf[21] << 24));
			aux->num_dd_virtdevs = value;

			break;
		}

		vrDbgPrintf("_VruiDDInitializeDevice(): Daemon reporting %d trackers, %d buttons and %d valuators.\n", aux->num_dd_trackers, aux->num_dd_buttons, aux->num_dd_valuators);
		if (aux->protocol >= 2)
			vrDbgPrintf("_VruiDDInitializeDevice(): Daemon reporting %d virtual devices.\n", aux->num_dd_virtdevs);

		/* now shift the data in the buffer */
		memmove(aux->buf, &aux->buf[opening_bytes], (aux->eobuf_pos-opening_bytes));	/* shift the data by the opening_bytes */
		aux->eobuf_pos -= opening_bytes;

		/* For protocol=2, read the virtual devices information */
		if (aux->protocol == 2) {
			for (count = 0; count < aux->num_dd_virtdevs; count++) {
				char	virtdev_name[256];
				int	num_buttons;
				int	num_valuators;
				int	strcount;
				int	count2;
				float	float_value;	/* NOTE: dangerous assumption of 4-byte float */
				/* first 4-byte integer has a string length for the name of the virtual device */
#if 0 /* in theory, the data ought to already be in the buffer */
				do {
					read_result = (ssize_t)read(aux->fd_socket, &aux->buf[aux->eobuf_pos], sizeof(aux->buf) - aux->eobuf_pos);
					if (read_result > 0) {
						aux->eobuf_pos += read_result;
					}
				} while (read_result > 0);
#endif

				/* get the name of this virtual device */
				value = (aux->buf[0] + (aux->buf[1] << 8) + (aux->buf[2] << 16) + (aux->buf[3] << 24));
				for (strcount = 0; strcount < value; strcount++)
					virtdev_name[strcount] = aux->buf[4+strcount];
				virtdev_name[strcount] = '\0';
				vrPrintf("virtual device %d is %s'%s'%s (%d)\n", count, BOLD_TEXT, virtdev_name, NORM_TEXT, value);
				memmove(aux->buf, &aux->buf[4+value], (aux->eobuf_pos-(4+value)));	/* shift the data by the string data */
				aux->eobuf_pos -= 4+value;

				/* first 4-byte integer is the trackerType code */
				value = (aux->buf[0] + (aux->buf[1] << 8) + (aux->buf[2] << 16) + (aux->buf[3] << 24));
				vrPrintf("\tGot TrackerType code of %d (7=6D)\n", value);
				memmove(aux->buf, &aux->buf[4], (aux->eobuf_pos-4));	/* shift the data by the 4 bytes */
				aux->eobuf_pos -= 4;

				/* next 3 numbers are 4-byte floats with the rayDirection */
				((char *)(&float_value))[0] = aux->buf[0];		/* NOTE: dangerous assumption of 4-byte floats here */
				((char *)(&float_value))[1] = aux->buf[1];
				((char *)(&float_value))[2] = aux->buf[2];
				((char *)(&float_value))[3] = aux->buf[3];
				vrPrintf("\trayDirection = (%f", float_value);
				memmove(aux->buf, &aux->buf[4], (aux->eobuf_pos-4));	/* shift the data by the 4 bytes */
				aux->eobuf_pos -= 4;

				((char *)(&float_value))[0] = aux->buf[0];		/* NOTE: dangerous assumption of 4-byte floats here */
				((char *)(&float_value))[1] = aux->buf[1];
				((char *)(&float_value))[2] = aux->buf[2];
				((char *)(&float_value))[3] = aux->buf[3];
				vrPrintf(" %f", float_value);
				memmove(aux->buf, &aux->buf[4], (aux->eobuf_pos-4));	/* shift the data by the 4 bytes */
				aux->eobuf_pos -= 4;

				((char *)(&float_value))[0] = aux->buf[0];		/* NOTE: dangerous assumption of 4-byte floats here */
				((char *)(&float_value))[1] = aux->buf[1];
				((char *)(&float_value))[2] = aux->buf[2];
				((char *)(&float_value))[3] = aux->buf[3];
				vrPrintf(" %f)\n", float_value);
				memmove(aux->buf, &aux->buf[4], (aux->eobuf_pos-4));	/* shift the data by the 4 bytes */
				aux->eobuf_pos -= 4;

				/* next 4-byte sequence is unknown (always been zero thus far) */
				value = (aux->buf[0] + (aux->buf[1] << 8) + (aux->buf[2] << 16) + (aux->buf[3] << 24));
				((char *)(&float_value))[0] = aux->buf[0];		/* NOTE: dangerous assumption of 4-byte floats here */
				((char *)(&float_value))[1] = aux->buf[1];
				((char *)(&float_value))[2] = aux->buf[2];
				((char *)(&float_value))[3] = aux->buf[3];
				vrPrintf("\tUnknown Value is %d or %f (always been zero thus far)\n", value, float_value);
				memmove(aux->buf, &aux->buf[4], (aux->eobuf_pos-4));	/* shift the data by the 4 bytes */
				aux->eobuf_pos -= 4;

				/* next 4-byte integer is the trackerIndex (will be -1 if there isn't one) */
				value = (aux->buf[0] + (aux->buf[1] << 8) + (aux->buf[2] << 16) + (aux->buf[3] << 24));
				vrPrintf("\tGot trackerIndex of %d.\n", value);
				memmove(aux->buf, &aux->buf[4], (aux->eobuf_pos-4));	/* shift the data by the 4 bytes */
				aux->eobuf_pos -= 4;

				/* next 4-byte integer is the numButtons */
				num_buttons = (aux->buf[0] + (aux->buf[1] << 8) + (aux->buf[2] << 16) + (aux->buf[3] << 24));
				vrPrintf("\tGot numButtons value of %d\n", num_buttons);
				memmove(aux->buf, &aux->buf[4], (aux->eobuf_pos-4));	/* shift the data by the 4 bytes */
				aux->eobuf_pos -= 4;

				/* Now loop over the button names */
				for (count2 = 0; count2 < num_buttons; count2++) {
					value = (aux->buf[0] + (aux->buf[1] << 8) + (aux->buf[2] << 16) + (aux->buf[3] << 24));
					for (strcount = 0; strcount < value; strcount++)
						virtdev_name[strcount] = aux->buf[4+strcount];
					virtdev_name[strcount] = '\0';
					vrPrintf("\t\tbutton %d is '%s' (%d)\n", count2, virtdev_name, value);
					memmove(aux->buf, &aux->buf[4+value], (aux->eobuf_pos-(4+value)));	/* shift the data by the string data */
					aux->eobuf_pos -= 4+value;
				}

				/* Now loop over the button indices */
				if (num_buttons > 0)
					vrPrintf("\tButton indices:\n");
				for (count2 = 0; count2 < num_buttons; count2++) {
					value = (aux->buf[0] + (aux->buf[1] << 8) + (aux->buf[2] << 16) + (aux->buf[3] << 24));
					vrPrintf("\t\tButton %d is indexed to %d\n", count2, value);
					memmove(aux->buf, &aux->buf[4], (aux->eobuf_pos-4));	/* shift the data by the 4 bytes */
					aux->eobuf_pos -= 4;
				}

				/* next 4-byte integer is the numValuators */
				num_valuators = (aux->buf[0] + (aux->buf[1] << 8) + (aux->buf[2] << 16) + (aux->buf[3] << 24));
				vrPrintf("\tGot numValuators value of %d\n", num_valuators);
				memmove(aux->buf, &aux->buf[4], (aux->eobuf_pos-4));	/* shift the data by the 4 bytes */
				aux->eobuf_pos -= 4;

				/* Now loop over the valuator names */
				for (count2 = 0; count2 < num_valuators; count2++) {
					value = (aux->buf[0] + (aux->buf[1] << 8) + (aux->buf[2] << 16) + (aux->buf[3] << 24));
					for (strcount = 0; strcount < value; strcount++)
						virtdev_name[strcount] = aux->buf[4+strcount];
					virtdev_name[strcount] = '\0';
					vrPrintf("\t\tvaluator %d is '%s' (%d)\n", count2, virtdev_name, value);
					memmove(aux->buf, &aux->buf[4+value], (aux->eobuf_pos-(4+value)));	/* shift the data by the string data */
					aux->eobuf_pos -= 4+value;
				}

				/* Now loop over the valuator indices */
				if (num_valuators > 0)
					vrPrintf("\tValuator indices:\n");
				for (count2 = 0; count2 < num_valuators; count2++) {
					value = (aux->buf[0] + (aux->buf[1] << 8) + (aux->buf[2] << 16) + (aux->buf[3] << 24));
					vrPrintf("\t\tValuator %d is indexed to %d\n", count2, value);
					memmove(aux->buf, &aux->buf[4], (aux->eobuf_pos-4));	/* shift the data by the 4 bytes */
					aux->eobuf_pos -= 4;
				}
			}
		}

		/* set the packet size */
		aux->packet_size = sizeof(uint16_t) +				/* message code */
				(sizeof(float)*13 * aux->num_dd_trackers) +	/* A Vrui TrackerState struct is thirteen floats */
				(sizeof(uint8_t) * aux->num_dd_buttons) +	/* A Vrui button is one uint8_t (aka "bool" in C++) */
				(sizeof(float) * aux->num_dd_valuators);	/* A Vrui valuator is one float */
		vrDbgPrintf("_VruiDDInitializeDevice(): Daemon packet size is %d bytes.\n", aux->packet_size);

		/* now allocated the memory in the generic portion of the structure */
		aux->incoming_buttons = (void *)vrShmemAlloc0(sizeof(int) * aux->num_dd_buttons);
		aux->incoming_buttons_prev = (void *)vrShmemAlloc0(sizeof(int) * aux->num_dd_buttons);
		aux->incoming_valuators = (void *)vrShmemAlloc0(sizeof(double) * aux->num_dd_valuators);
		aux->incoming_valuators_prev = (void *)vrShmemAlloc0(sizeof(double) * aux->num_dd_valuators);
		for (count = 0; count < 13; count++) {
			aux->incoming_trackers[count] = (void *)vrShmemAlloc0(sizeof(float) * aux->num_dd_trackers);	/* allocate for each of the 13 values */
#if defined(TEST_APP) /* In the test-app vrShmemAlloc0() is mapped to just malloc(), so the values need to be zeroed. */
			memset(aux->incoming_trackers[count], 0, sizeof(float) * aux->num_dd_trackers);
#endif
		}

		/* assign the initial "previous" values (set to force a difference in first acquisition) */
		for (count = 0; count < aux->num_dd_buttons; count++) {
			aux->incoming_buttons_prev[count] = -1;
#if defined(TEST_APP) /* In the test-app vrShmemAlloc0() is mapped to just malloc(), so the values need to be zeroed. */
			aux->incoming_buttons[count] = 0;
#endif
		}
		for (count = 0; count < aux->num_dd_valuators; count++) {
			aux->incoming_valuators_prev[count] = -9999.0;
#if defined(TEST_APP) /* In the test-app vrShmemAlloc0() is mapped to just malloc(), so the values need to be zeroed. */
			aux->incoming_valuators[count] = 0.0;
#endif
		}
	} else {
		/* unexpected response */
		vrErrPrintf("_VruiDDInitializeDevice(): " RED_TEXT "Warning, got a packet of type %02x, expecting a %02x\n" NORM_TEXT, message, CONNECT_REPLY);
		return -1;
	}

	snprintf(aux->version, sizeof(aux->version), "VruiDD protocol:%d", response_protocol);

	/* ACTIVATE_REQUEST -- Request to activate server (prepare for sending packets) (0x03 0x00) */
	message = ACTIVATE_REQUEST;
	write(aux->fd_socket, &message, sizeof(uint16_t));	/* TODO: warning, no effort yet being made to deal with endianness of the order */
	fsync(aux->fd_socket);

	/* STARTSTREAM_REQUEST -- Requests entering stream mode (server sends packets automatically) (0x07 0x00) */
	message = STARTSTREAM_REQUEST;
	write(aux->fd_socket, &message, sizeof(uint16_t));	/* TODO: warning, no effort yet being made to deal with endianness of the order */
	fsync(aux->fd_socket);

	return 0;
}


/**********************************************************/
/* _VruiDDCloseDevice() is called in the CLOSE phase      */
/*   of input interface -- just before the ports are      */
/*   closed, etc.                                         */
static int _VruiDDCloseDevice(_VruiDDPrivateInfo *aux)
{
	uint16_t		message;	/* VRUI VRDeviceDaemon messages are 2 byte sequences */

	/* Don't close a device that's not open */
	if (aux->open == 0)
		return 0;

	message = STOPSTREAM_REQUEST;
	write(aux->fd_socket, &message, sizeof(uint16_t));	/* TODO: warning, no effort yet being made to deal with endianness of the order */
	fsync(aux->fd_socket);

	message = DEACTIVATE_REQUEST;
	write(aux->fd_socket, &message, sizeof(uint16_t));	/* TODO: warning, no effort yet being made to deal with endianness of the order */
	fsync(aux->fd_socket);

	message = DISCONNECT_REQUEST;
	write(aux->fd_socket, &message, sizeof(uint16_t));	/* TODO: warning, no effort yet being made to deal with endianness of the order */
	fsync(aux->fd_socket);


	/* TODO: send a Vrui DISCONNECT_REQUEST message (perhaps also the STOPSTREAM_REQUEST & DEACTIVATE_REQUEST messages too) */
}



	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/


#if defined(FREEVR) /* { */
/*****************************************************************/
/***  Functions for FreeVR access of VruiDD devices for input  ***/
/*****************************************************************/


	/************************************/
	/***  FreeVR NON public routines  ***/
	/************************************/


/*********************************************************/
static void _VruiDDParseArgs(_VruiDDPrivateInfo *aux, char *args)
{
#if 0 /* possible other items to handle (from vr_input.skeleton.c) */
static	char	*XX_choices[] = { "up", "away", NULL };
static	int	XX_values[] = { 1, 0 };		/* integer values that map to the text strings in XX_choices */
	int 	null_value = -1;		/* for reading one of the null region values */
#endif
	float	scale_value = -1.0;		/* for reading one of the scaling factor values */

	/* In the rare case of no arguments, just return */
	if (args == NULL)
		return;

	/******************************************/
	/** Argument format: "host" "=" hostname **/
	/******************************************/
	vrArgParseString(args, "host", &(aux->dd_host));

	/****************************************/
	/** Argument format: "port" "=" number **/
	/****************************************/
	vrArgParseInteger(args, "port", &(aux->dd_port));

	/****************************************/
	/** Argument format: "protocol" "=" number **/
	/****************************************/
	vrArgParseInteger(args, "protocol", &(aux->protocol));

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
/* The _VruiDDGetData(): function calls the _VruiDDReadInput() function   */
/*   to get the latest data, and then puts that data into the FreeVR data */
/*   structures.  I.e. this is the FreeVR-specific portion of the data    */
/*   parsing.                                                             */
static void _VruiDDGetData(vrInputDevice *devinfo)
{
	_VruiDDPrivateInfo	*aux = (_VruiDDPrivateInfo *)devinfo->aux_data;
	int			num_inputs;		/* the number of incoming input values read (used for debugging only) */
	int			vruidd_map;		/* local storage of the mapping code from FreeVR input number to VruiDD input number */
	int			count;			/* counter for looping over inputs of a specific type */
	float			valuator_value;		/* holder for compositing a value together from components (e.g. sign & value) */
	float			scale_trans = aux->scale_trans;

	/*******************/
	/* gather the data */
	num_inputs = _VruiDDReadInput(aux);			/* Make sure we have the latest data stored in the "_FastrakPrivateInfo" structure */

	/*************/
	/** buttons **/
	for (count = 0; count < aux->num_buttons; count++) {
		vruidd_map = aux->button_map_button[count];
		if (vruidd_map >= aux->num_dd_buttons) {	/* NOTE: '>=' because vruidd_map is a zero-based number */
			vrErrPrintf(RED_TEXT "_VruiDDGetData(): request for button (%d) not provided by the Vrui Device Daemon (max %d).\n", vruidd_map, aux->num_dd_buttons-1 /* correct for zero-base */);
			/* TODO: I should look into creating a dummy input, and using that after the first warning is printed */
		} else if ((aux->incoming_buttons[vruidd_map]) != (aux->incoming_buttons_prev[vruidd_map])) {
			/* handle button inputs as buttons or self-controls */
			if (aux->button_inputs[count] != NULL) {
				switch (aux->button_inputs[count]->input_type) {
				case VRINPUT_BINARY:
					vrAssign2switchValue((vr2switch *)(aux->button_inputs[count]), (aux->incoming_buttons[vruidd_map] != 0));
#if 0
vrPrintf("Assignment DD button %d maps to FreeVR button %d, max = %d, val = %d, prev = %d\n", vruidd_map, count, aux->num_dd_buttons, aux->incoming_buttons[vruidd_map], aux->incoming_buttons_prev[vruidd_map]);
#endif
					break;
				case VRINPUT_CONTROL:
					vrCallbackInvokeDynamic(((vrControl *)(aux->button_inputs[count]))->callback, 1, (aux->incoming_buttons[vruidd_map] != 0));
					break;
				default:
					vrErrPrintf(RED_TEXT "_VruiDDGetData(): Unable to handle button inputs that aren't Binary or Control inputs\n" NORM_TEXT);
					break;
				}
			}
		}
	}

	/* now save all the current values as the "previous" values */
	for (count = 0; count < aux->num_dd_buttons; count++) {
		aux->incoming_buttons_prev[count] = aux->incoming_buttons[count];
	}

	/***************/
	/** valuators **/
	for (count = 0; count < aux->num_valuators; count++) {
		vruidd_map = aux->valuator_map_valuator[count];
		if (vruidd_map >= aux->num_dd_valuators) {	/* NOTE: '>=' because vruidd_map is a zero-based number */
			vrErrPrintf(RED_TEXT "_VruiDDGetData(): request for valuator (%d) not provided by the Vrui Device Daemon (max %d).\n", vruidd_map, aux->num_dd_valuators-1 /* correct for zero-base */);
			/* TODO: I should look into creating a dummy input, and using that after the first warning is printed */
		} else if ((aux->incoming_valuators[vruidd_map]) != (aux->incoming_valuators_prev[vruidd_map])) {
			/* handle valuator inputs as valuators or self-controls */
			if (aux->valuator_inputs[count] != NULL) {
				valuator_value = aux->incoming_valuators[count] * aux->scale_valuator * aux->valuator_sign[count];
				switch (aux->valuator_inputs[count]->input_type) {
				case VRINPUT_VALUATOR:
					vrAssignValuatorValue((vrValuator *)(aux->valuator_inputs[count]), valuator_value);
#if 0
vrPrintf("Assignment DD valuator %d maps to FreeVR valuator %d, max = %d, val = %f, prev = %f\n", vruidd_map, count, aux->num_dd_valuators, aux->incoming_valuators[vruidd_map], aux->incoming_valuators_prev[vruidd_map]);
#endif
					break;
				case VRINPUT_CONTROL:
					vrCallbackInvokeDynamic(((vrControl *)(aux->valuator_inputs[count]))->callback, 1, &valuator_value);
					break;
				default:
					vrErrPrintf(RED_TEXT "_VruiDDGetData(): Unable to handle valuator inputs that aren't Floating or Control inputs\n" NORM_TEXT);
					break;
				}
			}
		}
	}

	/* now save all the current values as the "previous" values */
	for (count = 0; count < aux->num_dd_valuators; count++) {
		aux->incoming_valuators_prev[count] = aux->incoming_valuators[count];
	}

	/***************/
	/** 6-sensors **/
	for (count = 0; count < aux->num_6sensors; count++) {
		vr6sensor	*current_6sensor;
		vrMatrix	new_position;
		vrVector	new_location;
		vrQuat		new_orientation;

		current_6sensor = aux->sensor6_inputs[count];
		vruidd_map = aux->tracker_map_tracker[count];

		new_location.v[VR_X] = aux->incoming_trackers[VR_X][vruidd_map] * scale_trans;
		new_location.v[VR_Y] = aux->incoming_trackers[VR_Y][vruidd_map] * scale_trans;
		new_location.v[VR_Z] = aux->incoming_trackers[VR_Z][vruidd_map] * scale_trans;

		new_orientation.v[VR_X] = aux->incoming_trackers[VR_QX][vruidd_map];
		new_orientation.v[VR_Y] = aux->incoming_trackers[VR_QY][vruidd_map];
		new_orientation.v[VR_Z] = aux->incoming_trackers[VR_QZ][vruidd_map];
		new_orientation.v[VR_W] = aux->incoming_trackers[VR_QW][vruidd_map];
#if 0
vrPrintf("assigning tracker %d to location %f %f %f\n", vruidd_map, new_location.v[VR_X], new_location.v[VR_Y], new_location.v[VR_Z]);
#endif

		vrMatrixSetIdentity(&new_position);
		vrMatrixSetTransFromVector(&new_position, &new_location);
#if 1	/* [03/18/2015: I've gone back to the original name, but in fact the functionality is what the QuatCR version did, because it seems that that was the correct one all along.] */
		vrMatrixSetRotationFromQuat(&new_position, &new_orientation);
#else
		vrMatrixSetRotationFromQuatCR(&new_position, &new_orientation);		/* [07/01/2014] Okay, so I suspect the reason I'm using the "CR" version of this routine is because Vrui uses a Z-up coordinate system, whereas FreeVR uses Y-up.  And thus things get twisted around! -- So if I do things correctly, I can get rid of vrMatrixSetRotationFromQuatCR(). */
#endif

		/* since 6-sensors generally change rapidly, there is little point */
		/*   and often moderate expense in comparing for changes from the  */
		/*   current value to the new value.                               */
		/* although perhaps watching the timestamp might be a useful way   */
		/*   to do this.                                                   */
		/* TODO: perhaps verify that this is indeed a VRINPUT_6SENSOR */
		vrAssign6sensorValue(current_6sensor, &new_position, 0 /*, TODO: timestamp from daemon */);
	}
}


	/****************************************************************/
	/*    Function(s) for parsing VruiDD "input" declarations.      */
	/*                                                              */
	/*  These _VruiDD<type>Input() functions are called during the  */
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
static vrInputMatch _VruiDDButtonInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
        _VruiDDPrivateInfo	*aux = (_VruiDDPrivateInfo *)devinfo->aux_data;
	int			button_num;		/* the current count of FreeVR button inputs */
	int			button_choice;		/* the chosen number for this button */

	/* The VruiDD method of handling input arrays may be a combination of the way */
	/*   suggested by vr_input.skeleton.c as the FoB, MS & Fastrak input drivers. */

	/* select a button */
	button_num = aux->num_buttons;
	aux->num_buttons++;
	button_choice = vrAtoI(dti->instance);

	/* check the selected button */
	if (button_choice == -1) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_VruiDDButtonInput(): Warning, button['%s'] did not match any known button\n", dti->instance);
		return VRINPUT_MATCH_UNABLE;	/* device match, but still unable to handle request */
	} else if (aux->button_inputs[button_num] != NULL) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_VruiDDButtonInput(): Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);
	}

	/* make the mapping */
	aux->button_inputs[button_num] = (vr2switch *)input;
	aux->button_map_button[button_num] = button_choice;
	vrDbgPrintfN(INPUT_DBGLVL, "_VruiDDButtonInput(): assigned button event of value %d (VruiDD button %d) to input pointer = %#p)\n",
		button_num, button_choice, aux->button_inputs[button_num]);

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/**************************************************************************/
static vrInputMatch _VruiDDValuatorInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
        _VruiDDPrivateInfo	*aux = (_VruiDDPrivateInfo *)devinfo->aux_data;
	int			valuator_num;		/* the current count of FreeVR valuator inputs */
	int			valuator_choice;	/* the chosen number for this valuator */

	/* The VruiDD method of handling input arrays may be a combination of the way */
	/*   suggested by vr_input.skeleton.c as the FoB, MS & Fastrak input drivers. */

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
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_VruiDDValuatorInput(): Warning, valuator['%s'] did not match any known valuator\n", dti->instance);
		return VRINPUT_MATCH_UNABLE;	/* device match, but still unable to handle request */
	} else if (aux->valuator_inputs[valuator_num] != NULL) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_VruiDDValuatorInput(): Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);
	}

	/* make the mapping */
	aux->valuator_inputs[valuator_num] = (vrValuator *)input;
	aux->valuator_map_valuator[valuator_num] = valuator_choice;
vrPrintf("mapping FreeVR valuator %d to VruiDD valuator %d\n", valuator_num, valuator_choice);
	vrDbgPrintfN(INPUT_DBGLVL, "_VruiDDValuatorInput(): assigned valuator event of value %d (VruiDD valuator %d) to input pointer = %#p)\n",
		valuator_num, valuator_choice, aux->valuator_inputs[valuator_num]);

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/**************************************************************************/
static vrInputMatch _VruiDD6sensorInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
        _VruiDDPrivateInfo	*aux = (_VruiDDPrivateInfo *)devinfo->aux_data;
	int			tracker_num;		/* the current count of FreeVR tracker inputs */
	int			tracker_choice;		/* the chosen number for this tracker */

	/* The VruiDD method of handling input arrays may be a combination of the way */
	/*   suggested by vr_input.skeleton.c as the FoB, MS & Fastrak input drivers. */

	/* select a tracker */
	tracker_num = aux->num_6sensors;
	aux->num_6sensors++;
	tracker_choice = vrAtoI(dti->instance);

	/* check the selected tracker */
	if (tracker_choice == -1) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_VruiDD6sensorInput(): Warning, tracker['%s'] did not match any known tracker\n", dti->instance);
		return VRINPUT_MATCH_UNABLE;	/* device match, but still unable to handle request */
	} else if (aux->sensor6_inputs[tracker_num] != NULL) {
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_VruiDD6sensorInput(): Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);
	}

	/* make the mapping */
	aux->sensor6_inputs[tracker_num] = (vr6sensor *)input;
	aux->tracker_map_tracker[tracker_num] = tracker_choice;
vrPrintf("mapping FreeVR tracker %d to VruiDD tracker %d\n", tracker_num, tracker_choice);
	vrDbgPrintfN(INPUT_DBGLVL, "_VruiDD6sensorInput(): assigned tracker event of value %d (VruiDD tracker %d) to input pointer = %#p)\n",
		tracker_num, tracker_choice, aux->sensor6_inputs[tracker_num]);

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
static void _VruiDDSystemPauseToggleCallback(vrInputDevice *devinfo, int value)
{
	_VruiDDPrivateInfo	*aux = (_VruiDDPrivateInfo *)devinfo->aux_data;

        if (value == 0)
                return;

	devinfo->context->paused ^= 1;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "VruiDD Control: system_pause = %d.\n",
		devinfo->context->paused);
}

/************************************************************/
static void _VruiDDPrintContextStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintContext(stdout, devinfo->context, verbose);
}

/************************************************************/
static void _VruiDDPrintConfigStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintConfig(stdout, devinfo->context->config, verbose);
}

/************************************************************/
static void _VruiDDPrintInputStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintInput(stdout, devinfo->context->input, verbose);
}

/************************************************************/
static void _VruiDDPrintStructCallback(vrInputDevice *devinfo, int value)
{
        _VruiDDPrivateInfo	*aux = (_VruiDDPrivateInfo *)devinfo->aux_data;

        if (value == 0)
                return;

        _VruiDDPrintStruct(stdout, aux, verbose);
}

/************************************************************/
static void _VruiDDPrintHelpCallback(vrInputDevice *devinfo, int value)
{
        _VruiDDPrivateInfo	*aux = (_VruiDDPrivateInfo *)devinfo->aux_data;

        if (value == 0)
                return;

        _VruiDDPrintHelp(stdout, aux);
}


	/*************************************************/
	/*   Callbacks for interfacing with the device.  */
	/*                                               */


/************************************************************/
static void _VruiDDCreateFunction(vrInputDevice *devinfo)
{
	/*** List of possible inputs ***/
static	vrInputFunction	_VruiDDInputs[] = {
				{ "button", VRINPUT_2WAY, _VruiDDButtonInput },
				{ "valuator", VRINPUT_VALUATOR, _VruiDDValuatorInput },
				{ "6sensor", VRINPUT_6SENSOR, _VruiDD6sensorInput },
				{ "tracker", VRINPUT_6SENSOR, _VruiDD6sensorInput },	/* a synonym for "6sensor" */
				{ NULL, VRINPUT_UNKNOWN, NULL } };
	/*** List of control functions ***/
static	vrControlFunc	_VruiDDControlList[] = {
				/* overall system controls */
				{ "system_pause_toggle", _VruiDDSystemPauseToggleCallback },

				/* informational output controls */
				{ "print_context", _VruiDDPrintContextStructCallback },
				{ "print_config", _VruiDDPrintConfigStructCallback },
				{ "print_input", _VruiDDPrintInputStructCallback },
				{ "print_struct", _VruiDDPrintStructCallback },
				{ "print_help", _VruiDDPrintHelpCallback },
		/** TODO: other callback control functions go here **/
				/* other controls */

				/* end of the list */
				{ NULL, NULL } };

	_VruiDDPrivateInfo	*aux = NULL;

	/******************************************/
	/* allocate and initialize auxiliary data */
	devinfo->aux_data = (void *)vrShmemAlloc0(sizeof(_VruiDDPrivateInfo));
	aux = (_VruiDDPrivateInfo *)devinfo->aux_data;
	_VruiDDInitializeStruct(aux, devinfo->type);

	/******************/
	/* handle options */
	aux->dd_host = vrShmemStrDup(DEFAULT_HOST);	/* initialize to the default host value */
	aux->dd_port = DEFAULT_PORT;			/* initialize to the default port value */
	_VruiDDParseArgs(aux, devinfo->args);

	/***************************************/
	/* create the inputs and self-controls */
	vrInputCountDataContainers(devinfo);

	/* Allocate memory for specific input types */
	aux->button_inputs = (vr2switch **)vrShmemAlloc((devinfo->num_2ways + devinfo->num_scontrols) * sizeof(vr2switch *));
	aux->button_map_button = (int *)vrShmemAlloc0((devinfo->num_2ways + devinfo->num_scontrols) * sizeof(int));

	aux->valuator_inputs = (vrValuator **)vrShmemAlloc0((devinfo->num_valuators + devinfo->num_scontrols) * sizeof(vrValuator *));
	aux->valuator_map_valuator = (int *)vrShmemAlloc0((devinfo->num_valuators + devinfo->num_scontrols) * sizeof(int));
	aux->valuator_sign = (float *)vrShmemAlloc0((devinfo->num_valuators + devinfo->num_scontrols) * sizeof(float));

	aux->sensor6_inputs = (vr6sensor **)vrShmemAlloc((devinfo->num_6sensors) * sizeof(vr6sensor *));
	aux->tracker_map_tracker = (int *)vrShmemAlloc0((devinfo->num_6sensors) * sizeof(int));

	/* Create the inputs within the FreeVR system */
	vrInputCreateDataContainers(devinfo, _VruiDDInputs);
	vrInputCreateSelfControlContainers(devinfo, _VruiDDInputs, _VruiDDControlList);
	/* TODO: anything to do for NON self-controls?  Implement for Magellan first. */

	vrDbgPrintf("_VruiDDCreateFunction(): Done creating VruiDD inputs for '%s'\n", devinfo->name);
	devinfo->created = 1;

	return;
}


/************************************************************/
static void _VruiDDOpenFunction(vrInputDevice *devinfo)
{
	_VruiDDPrivateInfo	*aux = (_VruiDDPrivateInfo *)devinfo->aux_data;

	vrTrace("_VruiDDOpenFunction", devinfo->name);

	/*******************/
	/* open the device */
	aux->fd_socket = vrSocketCall(aux->dd_host, aux->dd_port);
	if (aux->fd_socket < 0) {
		aux->open = 0;
		devinfo->operating = 0;
		vrErrPrintf("(%s::_VruiDDOpenFunction()::%d) error: "
			RED_TEXT "couldn't open socket %s:%d for %s\n" NORM_TEXT,
			__FILE__, __LINE__, aux->dd_host, aux->dd_port, devinfo->name);
		sprintf(aux->version, "- unconnected VruiDD -");
	} else {
		aux->open = 1;
		if (_VruiDDInitializeDevice(aux) < 0) {
			devinfo->operating = 0;
			vrErrPrintf("_VruiDDOpenFunction(): "
				RED_TEXT "Warning, unable to initialize '%s' VruiDD.\n" NORM_TEXT,
				devinfo->name);
		} else {
			devinfo->operating = 1;
			vrDbgPrintf("_VruiDDOpenFunction(): Done opening VruiDD input device '%s' (operating = %d).\n", devinfo->name, devinfo->operating);
		}
	}

	return;
}


/************************************************************/
static void _VruiDDCloseFunction(vrInputDevice *devinfo)
{
	_VruiDDPrivateInfo	*aux = (_VruiDDPrivateInfo *)devinfo->aux_data;
	int			count;			/* loop counter */

	vrDbgPrintf("_VruiDDCloseFunction(): About to close VruiDD input device '%s'.\n", devinfo->name);

	_VruiDDCloseDevice(aux);

	if (aux != NULL) {
		if (aux->open)
			vrSocketClose(aux->fd_socket);
		if (aux->incoming_buttons != NULL) vrShmemFree(aux->incoming_buttons);
		if (aux->incoming_buttons_prev != NULL) vrShmemFree(aux->incoming_buttons_prev);
		if (aux->incoming_valuators != NULL) vrShmemFree(aux->incoming_valuators);
		if (aux->incoming_valuators_prev != NULL) vrShmemFree(aux->incoming_valuators_prev);
		for (count = 0; count < 13; count++)
			if (aux->incoming_trackers[count] != NULL) vrShmemFree(aux->incoming_trackers[count]);

#ifdef FREEVR
		/* free the FreeVR specific fields of "aux" */
		/* TODO: ... */
#endif

		vrShmemFree(aux);			/* aka devinfo->aux_data */
	}

	return;
}


/************************************************************/
static void _VruiDDResetFunction(vrInputDevice *devinfo)
{
	_VruiDDPrivateInfo	*aux = (_VruiDDPrivateInfo *)devinfo->aux_data;

	return;
}


/************************************************************/
static void _VruiDDPollFunction(vrInputDevice *devinfo)
{
	_VruiDDPrivateInfo	*aux = (_VruiDDPrivateInfo *)devinfo->aux_data;

	if (devinfo->operating) {
		_VruiDDGetData(devinfo);
	} else {
		/* TODO: try to open the device again */
	}

	return;
}



	/******************************/
	/*** FreeVR public routines ***/
	/******************************/


/******************************************************/
void vrVruiDDInitInfo(vrInputDevice *devinfo)
{
	devinfo->version = (char *)vrShmemStrDup("-get from VruiDD device-");
	devinfo->Create = vrCallbackCreateNamed("VruiDD:Create-Def", _VruiDDCreateFunction, 1, devinfo);
	devinfo->Open = vrCallbackCreateNamed("VruiDD:Open-Def", _VruiDDOpenFunction, 1, devinfo);
	devinfo->Close = vrCallbackCreateNamed("VruiDD:Close-Def", _VruiDDCloseFunction, 1, devinfo);
	devinfo->Reset = vrCallbackCreateNamed("VruiDD:Reset-Def", _VruiDDResetFunction, 1, devinfo);
	devinfo->PollData = vrCallbackCreateNamed("VruiDD:PollData-Def", _VruiDDPollFunction, 1, devinfo);
	devinfo->PrintAux = vrCallbackCreateNamed("VruiDD:PrintAux-Def", _VruiDDPrintStruct, 0);

	vrDbgPrintfN(INPUT_DBGLVL, "vrVruiDDInitInfo(): callbacks created.\n");
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
/* A test program to communicate with a VruiDD device and print the results. */
main(int argc, char *argv[])
{
static	char			*err_missing_arg = "%s: missing argument to '%s'.\n";
	_VruiDDPrivateInfo	*aux;
	char			*progname;			/* name of the program executable */
	int			count;				/* for looping over things */

	done = 0;
	signal(SIGINT, exit_testapp);


	/******************************/
	/* setup the device structure */
	aux = (_VruiDDPrivateInfo *)malloc(sizeof(_VruiDDPrivateInfo));
	memset(aux, 0, sizeof(_VruiDDPrivateInfo));
	_VruiDDInitializeStruct(aux, "VruiDDsubtype");


	/*********************************************************/
	/* set default parameters based on environment variables */
	aux->dd_host = getenv("VRUIDD_HOST");
	if (aux->dd_host == NULL)
		aux->dd_host = DEFAULT_HOST;		/* default, if no host given */
	aux->dd_port = ((getenv("VRUIDD_PORT")==NULL) ? -1 : atoi(getenv("VRUIDD_PORT")));
	if (aux->dd_port == -1)
		aux->dd_port = DEFAULT_PORT;		/* default, if no port given */

	aux->protocol = ((getenv("VRUIDD_PROTOCOL")==NULL) ? 0 : atoi(getenv("VRUIDD_PROTOCOL")));


	/*****************************************************/
	/* adjust parameters based on command-line-arguments */
	progname = argv[0];
	while ((argc > 1) && (argv[1][0] == '-')) {
		/* Report information about the device and then quit */
		if (!strcmp(argv[1], "-nodata")) {
			/* NOTE: for other input-test applications we set a separate "nodata_flag" */
			/*   but that isn't necessary here, and the premature exit() was avoiding  */
			/*   the clean-disconnect with the Vrui VRDeviceDaemon.                    */
			done = 1;
			argv++; argc--;
		}

		/* Set the port */
		if (!strcmp(argv[1], "-p") || !strcmp(argv[1], "-port")) {
			argv++; argc--;
			if (argc > 1) {
				aux->dd_port = atoi(argv[1]);
				argv++; argc--;
			} else {
				fprintf(stderr, err_missing_arg, progname, argv[0]);
				fprintf(stderr, RED_TEXT "Usage:" NORM_TEXT " %s [-nodata] [-p <port>(%d)] [-protocol <prot>(%d)] [<Vrui host> (default = '%s')]\n",
					progname, aux->dd_port, aux->protocol, aux->dd_host);
				exit(1);
			}
		}

		/* Set the protocol */
		if (!strcmp(argv[1], "-prot") || !strcmp(argv[1], "-protocol")) {
			argv++; argc--;
			if (argc > 1) {
				aux->protocol = atoi(argv[1]);
				argv++; argc--;
			} else {
				fprintf(stderr, err_missing_arg, progname, argv[0]);
				fprintf(stderr, RED_TEXT "Usage:" NORM_TEXT " %s [-nodata] [-p <port>(%d)] [-protocol <prot>(%d)] [<Vrui host> (default = '%s')]\n",
					progname, aux->dd_port, aux->protocol, aux->dd_host);
				exit(1);
			}
		}

		/* Unknown option */
		else {
			/* There are currently no other "-" options, so this is an error */
			/* NOTE: reported defaults might be effected by environment variables */
			fprintf(stderr, RED_TEXT "Usage:" NORM_TEXT " %s [-nodata] [-p <port>(%d)] [-protocol <prot>(%d)] [<Vrui host> (default = '%s')]\n",
				progname, aux->dd_port, aux->protocol, aux->dd_host);
			exit(1);
		}
	}

	/* if there are any arguments left, use the first as the hostname */
	if (argc > 1) {
		aux->dd_host = strdup(argv[1]);
	}


	/**************************************************/
	/* open the socket port and initialize the device */
	aux->fd_socket = vrSocketCall(aux->dd_host, aux->dd_port);
	if (aux->fd_socket < 0) {
		aux->open = 0;
		fprintf(stderr, RED_TEXT "couldn't open socket '%s:%d'\n" NORM_TEXT, aux->dd_host, aux->dd_port);
		sprintf(aux->version, "- unconnected VruiDD -");
	} else {
		aux->open = 1;
		if (_VruiDDInitializeDevice(aux) < 0) {
			vrErrPrintf("main: " RED_TEXT "Warning, unable to initialize VruiDD.\n" NORM_TEXT);
		}
	}

	_VruiDDPrintStruct(stdout, aux, verbose);


	/****************************************/
	/* read the data and display the output */
	while(aux->open && !done) {
		if (_VruiDDReadInput(aux) > 0) {
			if (aux->num_dd_trackers > 0) {
				int	ptv_count;		/* for looping over the position-tracker values */
				printf("pt: ");
				for (count = 0; count < aux->num_dd_trackers; count++) {
					for (ptv_count = 0; ptv_count < (count == 0 ? 13:3); ptv_count++) {	/* TODO: s/b 13, but doing 3 for now (except for the first) */
						printf("%6.3f ", aux->incoming_trackers[ptv_count][count]);
					}
					printf("-- ");
				}
			}

			if (aux->num_dd_buttons > 0) {
				printf("buts: ");
				for (count = 0; count < aux->num_dd_buttons; count++) {
					printf("%c", (aux->incoming_buttons[count] == 0 ? '.' : 'X'));
				}
			}

			if (aux->num_dd_valuators > 0) {
				printf(" vals: ");
				for (count = 0; count < aux->num_dd_valuators; count++) {
					printf("%6.3f ", aux->incoming_valuators[count]);
				}
			}


			/* check to see if we should quit */
			if (aux->num_dd_buttons > 2) {
				if (aux->incoming_buttons[0] && aux->incoming_buttons[1])
					done = 1;
			}
			printf("    \r");
			fflush(stdout);
		}
	}
	printf("\n");


	/*****************/
	/* close up shop */
	if (aux->open) {
		_VruiDDCloseDevice(aux);

		while (_VruiDDReadInput(aux) >= 0);	/* wait for the -1 response indicating the reception of the STOPSTREAM_REPLY from the daemon */
	}

	if (aux != NULL) {
		if (aux->open)
			vrSocketClose(aux->fd_socket);

		free(aux);
	}

	vrDbgPrintf(BOLD_TEXT "\nVruiDD device closed\n" NORM_TEXT);
}

#endif /* } TEST_APP */



	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/



#if defined(CAVE) /* { */


	/* ... CAVE stuff here if to also work with CAVElib */


#endif /* } CAVE */

