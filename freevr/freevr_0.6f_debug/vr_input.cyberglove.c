/* ======================================================================
 *
 *  CCCCC          vr_input.cyberglove.c
 * CC   CC         Author(s): Bill Sherman
 * CC              Created: March 8, 2000
 * CC   CC         Last Modified: June 7, 2003
 *  CCCCC
 *
 * Code file for FreeVR inputs from the VTI Cyberglove input device.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/*************************************************************************

USAGE:
	...

	Inputs are specified with the "input" option:
		input "<name>" = "2switch(button[<number>])";
	  :-(	input "<name>" = "2switch({glove|sensor}[<sensor number>, <threshold>])";
		input "<name>" = "Nsensor(glove[<number>])";

	  :-(	input "<name>" = "Nswitch(joint[<number>])";	 (each joint 0-255)
		input "<name>" = "valuator({glove|sensor}[<{sensor }number>])";	(maybe for each joint)
	  :-(	input "<name>" = "6sensor(receiver[<number>])";	(some sort of simulator?)

	Controls are specified with the "control" option:
		control "<control option>" = "2switch(button[wrist])";
	  ...

	Here is the available control options for FreeVR:
		"print_help" -- print info on how to use the input device
		"system_pause_toggle" -- toggle the system pause flag
		"print_context" -- print the overall FreeVR context data structure (for debugging)
		"print_config" -- print the overall FreeVR config data structure (for debugging)
		"print_input" -- print the overall FreeVR input data structure (for debugging)
		"print_struct" -- print the internal Cyberglove data structure (for debugging)

	  :-(	"led" -- turn on/off led on wrist switch
	  :-(	"vibrate_thumb" -- activate the thumb vibration (non-binary)
	  :-(	"vibrate_index" -- activate the index finger vibration (non-binary)
	  :-(	"vibrate_middle" -- activate the middle finger vibration (non-binary)
	  :-(	"vibrate_ring" -- activate the ring finger vibration (non-binary)
	  :-(	"vibrate_pinky" -- activate the pinky finger vibration (non-binary)
	  ...

	Here are the FreeVR configuration argument options for the Cyberglove:
		"port" - serial port Cyberglove is connected to
			("/dev/input/cyberglove" is the default)
		"baud" - baud rate of serial port connection
			(38400 is the default)
	  :-(	"filter" - boolean value of whether the sensor data should be internally filtered
			(yes is the default)
	  ...

HISTORY:
	10-13 March 2000 (Bill Sherman) -- used vr_input.skeleton.c to help create an
		initial version.  Got TEST_APP version working reasonably well in
		polling mode.

	14-15 March 2000 (Bill Sherman) -- implemented the FreeVR interface.

	10 August 2000 (Bill Sherman)
		Changed all occurrences of usleep() and sginap() to use new vrSleep()
		  function now available in vr_utils.c.

	3 May 2001 (Bill Sherman)
		I made a few minor changes to catch up to the general
		  vr_input.skeleton.c format.

	20 May 2002 (Bill Sherman)
		Updated to new variable-type names ("2switch" instead of "Switch2", etc),
		  and the prepension of "VR_" to #defined values.

	11 September 2002 (Bill Sherman)
		Moved the control callback array into the _CybergloveFunction() callback.

	21-23 April 2003 (Bill Sherman)
		Updated to new input matching function code style.  Changed "opaque"
		  field to "aux_data".  Split _CybergloveFunction() into 5 functions.
		  Added new vrPrintStyle argument to _CyberglovePrintStruct() for the
		  sake of the new "PrintAux" callback.

	3 June 2003 (Bill Sherman)
		Now include "vr_enums.h" for the TEST_APP code.
		Added the address of the auxiliary data to the printout.
		Added the "system_pause_toggle" control callback.
		Added comments classifying the controls.
		Now use the "trans_scale" and "trans_rot" fields of vr6SensorConv
			instead of local copies (though, in fact that's part
			of an as-yet unused block of code).

	16 October 2009 (Bill Sherman)
		A quick fix to the _CybergloveParseArgs() routine to handle
		  the no-arguments case.

	14 September 2013 (Bill Sherman)
		I am making use of the newly created SELFCTRL_DBGLVL for all
		the self-control outputs.  I then added some reasonable output
		to the "print_help" callback to print out what all the device's
		inputs are doing.

	15 September 2013 (Bill Sherman)
		Renaming "active_sensor" to "active_sim6sensor".
		Changed the "0x%p" format to the improved "%#p" format.

TODO:
	- Determine if (and how) stream-mode should be used

	- Implement the Binary Input data parsing

	- Implement the rest of the query and command parsing

	- Implement the 2switch-button, 2switch-threshold, Nswitch, Nsensor, and
		6sensor inputs.

	- Implement some/all of the control options.


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

#include "vr_serial.h"
#include "vr_debug.h"
#include "vr_utils.h"
#define	CYBERG_DBGLVL	155	/* TODO: this goes in vr_debug.h */


#if defined (FREEVR)
#  include "vr_input.h"
#  include "vr_input.opts.h"
#  include "vr_parse.h"
#  include "vr_shmem.h"
#endif


/* TODO: see if we actually need these defines */
#if defined(TEST_APP) || defined(CAVE)
#  define	vrAtoI	atoi
#  define	VR_X	0
#  define	VR_Y	1
#  define	VR_Z	2
#  define	VR_AZIM	0
#  define	VR_ELEV	1
#  define	VR_ROLL	2
#endif

#if defined(TEST_APP)
#  include "vr_serial.c"
#  include "vr_enums.h"
#endif


/*** local defines ***/

#if 0
#define	DEFAULT_PORT	"/dev/input/cyberglove"
#define	DEFAULT_BAUD	38400
#define	DEFAULT_PORT	"/dev/ttyS0"
#endif
#define	DEFAULT_PORT	"/dev/ttyd2"
#define	DEFAULT_BAUD	19200

#define	BUFSIZE		1024

#define POLLMODE	1

#if defined(TEST_APP) && POLLMODE
#  include "vr_utils.c"		/* POLLMODE makes use of the vrSleep() function */
#endif

	/**************************************************************/
	/*** definitions for interfacing with the Cyberglove device ***/
	/***                                                        ***/

#define	CYBERG_ENDBYTE	'\0'
/* Cyberglove command sequences */
#define	CYBERGLOVEx1xMsg		"Xx1x\r"		/* Command 1 */
#define	CYBERGLOVEx2xMsg		"Xx2x\r"		/* Command 2 */

/* Flock of Birds BirdMouse bit-masks & indices */
#define CYBERGLOVE_BUTTON_x1x	0x01
#define CYBERGLOVE_BUTTON_x2x	0x02
#define CYBERGLOVE_BUTTON_x3x	0x04

#define CYBERGLOVE_BUTTONINDEX_x1x	0x00
#define CYBERGLOVE_BUTTONINDEX_x2x	0x01
#define CYBERGLOVE_BUTTONINDEX_x3x	0x02



/********************************************************************/
/*** auxiliary structure of the current data from the Cyberglove. ***/
typedef struct {
		/* these are for interfacing with the hardware */
		int		fd;		/* was commhandle */
		char		*port;		/* name of serial port */
		int		baud_enum;	/* communication rate as an enumerated value */
		int		baud_int;	/* communication rate as the real value */
		int		open;		/* flag with Cyberglove successfully open */

		/* these are for internal data parsing */
		unsigned char	buf[BUFSIZE];	/* buffer of incoming data to be parsed TODO: determine usage (seems not to be used) */
		int		eobuf_pos;	/* end-of-buffer position (generally the number of bytes in the buffer) TODO: determine usage (seems not to be used) */
		char		lo_buf[1024];	/* buffer of left-over bytes           */
		int		lo_buflen;	/* length of lo_buf                    */
		char		version[512];	/* self-reported version of the device */
		char		op_params[256];	/* operating parameters of the device (according to it) */

#define	MAX_GLOVESENSORS	24
#define MAX_GLOVEBUTTONS	 1

#ifdef CAVE
		/* CAVE specific stuff here */

#elif defined(FREEVR)
	/* FREEVR stuff */

#  define MAX_6SENSORS   1
#  define MAX_NSENSORS   1
		vr2switch	*button_inputs[MAX_GLOVEBUTTONS];
		vr6sensor	*sensor6_inputs[MAX_6SENSORS];
		vrNsensor	*sensorN_inputs[MAX_NSENSORS];

		vrNswitch	*switchN_inputs[MAX_GLOVESENSORS];
		vrValuator	*valuator_inputs[MAX_GLOVESENSORS];

#endif /* end library-specific fields */

		/* information about the current values */
		int		glove_data[MAX_GLOVESENSORS+5];	/* incoming data */
		int		buttons[MAX_GLOVEBUTTONS];	/* incoming button info */
		int		timer;		/* incoming timer info  TODO: or is it a checksum?*/
		int		data_change;	/* boolean indicator if new data has arrived */

		int		right_hand;
		int		reported_baud;
		int		glove_report;
		unsigned long	sensor_mask;	/* bitmask of which sensors will be reported */
		int		include_glove_status;	/* boolean showing extra byte per data report */
		int		include_time_stamp;	/* boolean showing extra time data reported */
		int		sensors_reported;	/* number of sensors that will be reported*/


		/* information about how to filter the data for use with the VR library (FreeVR or CAVE) */

	} _CyberglovePrivateInfo;



	/******************************************************/
	/*** General NON public Cyberglove interface routines ***/
	/******************************************************/

/******************************************************/
/* typename is used to specify a particular device among many that */
/*   share (more or less) the same protocol.  The typename is then */
/*   used to determine what specific features are available with   */
/*   this particular type of device.  This can be used in the      */
/*   Cyberglove interface to distinguish between 18 & 22 sensor    */
/*   gloves and/or whether they are equipped with vibrators or     */
/*   other haptic output.                                          */
static void _CybergloveInitializeStruct(_CyberglovePrivateInfo *aux)
{
	int	count;

	aux->version[0] = '\0';
	aux->op_params[0] = '\0';

	aux->buf[0] = '\0';
	aux->eobuf_pos = 0;
	aux->lo_buf[0] = '\0';
	aux->lo_buflen = 0;

	for (count = 0; count < MAX_GLOVEBUTTONS; count++)
		aux->buttons[count] = 0;
	for (count = 0; count < (MAX_GLOVESENSORS+5); count++)
		aux->glove_data[count] = 0;
	aux->right_hand = -1;		/* not yet set */
	aux->reported_baud = -1;	/* not yet set */
	aux->glove_report = 0x10;	/* not yet set (only 2 ls-bits used) */
	aux->sensor_mask = 0x0;		/* not yet set */
	aux->include_glove_status = 0;	/* off by default (and not yet set) */
	aux->include_time_stamp = 0;	/* off by default (and not yet set) */
	aux->sensors_reported = 0;	/* not yet set */

	aux->timer = 0;
	aux->data_change = 0;

	/* everything else is zero'd by default */
}


/******************************************************/
static void _CyberglovePrintStruct(FILE *file, _CyberglovePrivateInfo *aux, vrPrintStyle style)
{
	int	count;

	vrFprintf(file, "Cyberglove device internal structure (%#p):\n", aux);
	vrFprintf(file, "\r\tversion -- '%s'\n", aux->version);
	vrFprintf(file, "\r\toperating parameters -- '%s'\n", aux->op_params);
	vrFprintf(file, "\r\tfd = %d\n\tport = '%s'\n\tbaud = %d (%d)\n\topen = %d\n",
		aux->fd,
		aux->port,
		aux->baud_int, aux->baud_enum,
		aux->open);
	vrFprintf(file, "\r\tright glove? -- %d\n", aux->right_hand);
	vrFprintf(file, "\r\treported_baud -- %d\n", aux->reported_baud);
	vrFprintf(file, "\r\tglove report -- %d (%s:%s)\n", aux->glove_report,
		(aux->glove_report & 0x02 ? "plugged in" : "not plugged in"),
		(aux->glove_report & 0x01 ? "initialized" : "not initialized"));
	vrFprintf(file, "\r\tsensors_reported -- %d\n", aux->sensors_reported);
	vrFprintf(file, "\r\tsensor_mask -- 0x%06x\n", aux->sensor_mask);
	vrFprintf(file, "\r\tinclude_glove_status -- %d\n", aux->include_glove_status);
	vrFprintf(file, "\r\tinclude_time_stamp -- %d\n", aux->include_time_stamp);

	vrFprintf(file, "\r\tbutton_value -- %d\n", aux->buttons[0]);
	vrFprintf(file, "\r\tglove_data -- ");
	for (count = 0; count < (MAX_GLOVESENSORS+5); count++)
		vrFprintf(file, "%d ", aux->glove_data[count]);
	vrFprintf(file, "\n");

#ifdef FREEVR /* { */
	vrFprintf(file, "\r\tvaluator inputs:\n");
	for (count = 0; count < MAX_GLOVESENSORS; count++)
		if (aux->valuator_inputs[count] != NULL) {
			vrFprintf(file, "\r\t\tvaluator_input[%d] = %#p\n", count, aux->valuator_inputs[count]);
		}

#ifdef NOT_YET_IMPLEMENTED
	vrFprintf(file, "\r\tbutton inputs:\n");
	for (count = 0; count < MAX_GLOVEBUTTONS; count++)
		vrFprintf(file, "\r\t\tbutton_input[%d] = %#p\n", count, aux->button_inputs[count]);

	vrFprintf(file, "\r\t6sensor inputs (active = %d):\n", aux->active_sim6sensor);
	for (count = 0; count < MAX_6SENSORS; count++)
		vrFprintf(file, "\r\t\t6sensor_inputs[%d] = %#p\n", count, aux->sensor6_inputs[count]);

	vrFprintf(file, "\r\tNsensor inputs (active = %d):\n", aux->active_sim6sensor);
	for (count = 0; count < MAX_NSENSORS; count++)
		vrFprintf(file, "\r\t\tNsensor_inputs[%d] = %#p\n", count, aux->sensorN_inputs[count]);

#endif /* NOT_YET_IMPLEMENTED */
#endif /* } FREEVR */

	/* TODO: print some info about the current values */

}


/**************************************************************************/
static void _CyberglovePrintHelp(FILE *file, _CyberglovePrivateInfo *aux)
{
	int	count;		/* looping variable */

#if 0 || defined(TEST_APP)
	vrFprintf(file, BOLD_TEXT "Sorry, Cyberglove - print_help control not yet implemented.\n" NORM_TEXT);
#else
	vrFprintf(file, BOLD_TEXT "Cyberglove - inputs:" NORM_TEXT "\n");
	for (count = 0; count < MAX_GLOVEBUTTONS; count++) {
		if (aux->button_inputs[count] != NULL) {
			vrFprintf(file, "\t%s -- %s%s\n",
				aux->button_inputs[count]->my_object->desc_str,
				(aux->button_inputs[count]->input_type == VRINPUT_CONTROL ? "control:" : ""),
				aux->button_inputs[count]->my_object->name);
		}
	}
	for (count = 0; count < MAX_GLOVESENSORS; count++) {
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


/******************************************************/
/* this version is based on the pinchglove input methods -- works well for   */
/*   devices that use a constant byte for signalling the end of transmission */
/*   This function returns a character based on the most recently received   */
/*   message type (i.e. the first character of the message).                 */
static char _CybergloveReadInput(_CyberglovePrivateInfo *aux)
{
	char		buffer[1024];
	char		return_code = '\0';
	unsigned int	temp_uint;
	int		count;

	if (vrSerialReadToChar(aux->fd, CYBERG_ENDBYTE, aux->lo_buf, sizeof(aux->lo_buf), aux->lo_buflen, buffer, sizeof(buffer)) == NULL) {
		return return_code;
	}
#if 0
printf("read data '%s'\n", buffer);
#endif

#ifndef TEST_APP
	if (aux->lo_buflen > 0)
		vrDbgPrintfN(CYBERG_DBGLVL, "_CybergloveReadInput(): leftover buffer has %d bytes\n", aux->lo_buflen);
#endif

	/* all responses are at least two bytes, if less than two received, then return error */
	if (strlen(buffer) <= 1) {
		/* response isn't long enough, return an error msg_type */
		return '\1';	/* need to make sure this code is okay */
	}

	/* except for query's the return code is the first byte */
	return_code = buffer[0];

	switch (buffer[0]) {

	/*******************/
	/*** Query codes ***/
	case '?':
		/* query codes return the second byte as the return code */
		return_code = buffer[1];

		switch (buffer[1]) {
		case 'i':	/* ASCII: Information query (?? bytes) */
		/* TODO: version should be constructed from components of the info message */
			strncpy(aux->version, &buffer[1], sizeof(aux->version));
			break;

		case 'r':	/* ASCII: right handed glove? (1 integer) */
			aux->right_hand = vrAtoI(&buffer[2]);
			break;
		case 'R':	/* BINARY: right handed glove? (1 byte value) */
			aux->right_hand = (int)(buffer[2]);
			break;

		case 'b':	/* ASCII: Baud rate info (1 integer) */
			temp_uint = vrAtoI(&buffer[2]);
		case 'B':	/* BINARY: Baud rate info (1 byte value) */
			if (buffer[1] == 'B')
				temp_uint = (int)(buffer[2]);

			aux->reported_baud = 115200;
			switch (temp_uint & 0x18) {
			case 0x00:
				aux->reported_baud /= 1;
				break;
			case 0x08:
				aux->reported_baud /= 3;
				break;
			case 0x10:
				aux->reported_baud /= 4;
				break;
			case 0x18:
				aux->reported_baud /= 13;
				break;
			}

			switch (temp_uint & 0x07) {
			case 0x00:
				aux->reported_baud /= 1;
				break;
			case 0x01:
				aux->reported_baud /= 2;
				break;
			case 0x02:
				aux->reported_baud /= 4;
				break;
			case 0x03:
				aux->reported_baud /= 8;
				break;
			case 0x04:
				aux->reported_baud /= 16;
				break;
			case 0x05:
				aux->reported_baud /= 32;
				break;
			case 0x06:
				aux->reported_baud /= 64;
				break;
			case 0x07:
				aux->reported_baud /= 128;
				break;
			}

			break;

		case 'g':	/* ASCII: glove info (1 integer) */
			aux->glove_report = vrAtoI(&buffer[2]);
printf("setting glove_report to %d\n", aux->glove_report);
			break;
		case 'G':	/* BINARY: glove info (1 byte value) */
			aux->glove_report = (int)(buffer[2]);
			break;

		case 'u':	/* ASCII: include-glove-statUs (1 integer) */
			aux->include_glove_status = vrAtoI(&buffer[2]);
printf("set include_glove_status to %d\n", aux->include_glove_status);
			break;
		case 'U':	/* BINARY: include-glove-statUs (1 byte value) */
			aux->include_glove_status = (int)(buffer[2]);
			break;

		case 'n':	/* ASCII: Number of sensors to sample (1 integer) */
			aux->sensors_reported = vrAtoI(&buffer[2]);
			break;
		case 'N':	/* BINARY: Number of sensors to sample (1 byte value) */
			aux->sensors_reported = (int)(buffer[2]);
			break;

		case 'm':	/* ASCII: software sensor Mask (3 integers) */
			aux->sensor_mask = 0x0;
			sscanf(&buffer[2], "%d%[^\r]", &temp_uint, buffer);
			aux->sensor_mask |=  temp_uint;
			sscanf(buffer, "%d%[^\r]", &temp_uint, buffer);
			aux->sensor_mask |=  temp_uint <<  8;
			sscanf(buffer, "%d", &temp_uint);
			aux->sensor_mask |=  temp_uint << 16;
			break;
		case 'M':	/* BINARY: software sensor Mask (3 bytes) */
			aux->sensor_mask = 0x0;
			aux->sensor_mask |=  (unsigned int)(buffer[2]);
			aux->sensor_mask |= ((unsigned int)(buffer[3]) <<  8);
			aux->sensor_mask |= ((unsigned int)(buffer[4]) << 16);
			break;

		case 'p':	/* ASCII: Parameter flags (3 integers) */
		case 'c':	/* ASCII: Calibration info (3 lines of 22 integers) */
		case 'C':	/* BINARY: Calibration info (? * 1 byte values) */
		case 'P':	/* BINARY: Parameter flags (3 * 1 byte values) */
		case 't':	/* ASCII: sample period (Time) (2 integers) */
		case 'T':	/* BINARY: sample period (Time) (2 * 2 byte values) */
		case 'k':	/* ASCII: hardware sensor masK (3 integers) */
		case 'K':	/* BINARY: hardware sensor masK (3 * 1 byte values) */
		case 's':	/* ASCII: number Sensors in glove (1 integer) */
		case 'S':	/* BINARY: number Sensors in glove (1 byte value) */
		case 'v':	/* ASCII: Version numbers (cgiu & glove) (2 integers) */
		case 'V':	/* BINARY: Version numbers (cgiu & glove) (2 * 2 byte values) */
		case 'd':	/* ASCII: incluDe-time-stamp status (1 integer) */
		case 'D':	/* BINARY: incluDe-time-stamp status (1 byte value) */
		case 'f':	/* ASCII: Filter status (1 integer) */
		case 'F':	/* BINARY: Filter status (1 byte value) */
		case 'j':	/* ASCII: switch-controls-light status (1 integer) */
		case 'J':	/* BINARY: switch-controls-light status (1 byte value) */
		case 'l':	/* ASCII: Light status (1 integer) */
		case 'L':	/* BINARY: Light status (1 byte value) */
		case 'q':	/* ASCII: send-Quant-vals status (1 integer) */
		case 'Q':	/* BINARY: send-Quant-vals status (1 byte value) */
		case 'w':	/* ASCII: sWitch status (1 integer) */
		case 'W':	/* BINARY: sWitch status (1 byte value) */
		case 'y':	/* ASCII: enable/disable external sYnc (1 integer) */
		case 'Y':	/* BINARY: enable/disable external sYnc (1 byte value) */
		case '?':	/* BINARY: display command/error Summary (1 integer) */
			vrPrintf("_CybergloveReadInput: " RED_TEXT
				"unimplemented parse code '?%c'\n" NORM_TEXT,
				return_code);
			break;

		default:
			return_code = '\1';
			break;
		}

		break;

	/*******************/
	/*** Error codes ***/
	case 'e':
		switch (buffer[1]) {
		case '?':		/* ERROR: "Unknown command" */
			return '\1';	/* TODO: determine a good error return code */
			break;
		case 'g':		/* ERROR: "Glove not plugged in" */
			return '\1';	/* TODO: determine a good error return code */
			break;
		case 'n':		/* ERROR: "error with entered Number" */
			return '\1';	/* TODO: determine a good error return code */
			break;
		case 's':		/* ERROR: "Sampling rate too fast" */
			return '\1';	/* TODO: determine a good error return code */
			break;
		case 'y':		/* ERROR: "sYnc input rate too fast" */
			return '\1';	/* TODO: determine a good error return code */
			break;
		default:		/* ERROR: unknown error response */
			return '\1';	/* TODO: determine a good error return code */
			break;
		}

		break;

	/*********************/
	/*** Command codes ***/
	case 'b':	/* set Baud rate (1 word) */
	case 'c':	/* Calibrate hrdw offset & gain */
	case 'm':	/* set sftw sensor Mask (3 bytes) */
	case 'n':	/* set Number of sensors to sample (1 byte) */
	case 'p':	/* set Parameter flags (3 bytes) */
	case 't':	/* set sample period (2 words) */
	case ('i'-'@'):	/* re-Initialize glove information */
	case ('r'-'@'):	/* Re-start cgiu firmware pgm */
	case 'd':	/* include-time-stamp on/off */
	case 'f':	/* set Filter on/off */
	case 'j':	/* set switch-controls-light on/off */
	case 'l':	/* turn Light on/off */
	case 'q':	/* set send-Quantized-values of/off */
	case 'u':	/* include-glove-statUs on/off */
	case 'w':	/* set sWitch status on/off */
	case 'y':	/* set external sYnc on/off */

		vrPrintf("_CybergloveReadInput: " RED_TEXT
			"unimplemented command code '%c'\n" NORM_TEXT,
			return_code);
		break;

	/******************/
	/*** Data codes ***/
	case 'g':	/* ASCII data -- polled */
	case 's':	/* ASCII data -- stream */
		buffer[0] = ' ';
		count = 0;
		do {
			temp_uint = -1;
			sscanf(buffer, "%d%[^\r]", &temp_uint, buffer);
			aux->glove_data[count] = temp_uint;
			count++;
		} while (temp_uint != -1 && count < (MAX_GLOVESENSORS+5));
		break;

	case 'G':	/* BINARY data -- polled */
	case 'S':	/* BINARY data -- stream */
		vrPrintf("_CybergloveReadInput: " RED_TEXT
			"unimplemented data code '%c'\n" NORM_TEXT,
			return_code);
		break;

	default:
		vrPrintf("_CybergloveReadInput: " RED_TEXT
			"unknown code '%c'\n" NORM_TEXT,
			return_code);
		return '\1';	/* TODO: determine a good error return code */
	}

#if 0
printf("_CybergloveReadInput: returning '%c'\n", return_code);
#endif
	return (return_code);
}


/******************************************************/
/* _CybergloveInitializeDevice() is called in the OPEN phase  */
/*   of input interface -- after the types of inputs have     */
/*   been determined (during the CREATE phase).               */
static int _CybergloveInitializeDevice(_CyberglovePrivateInfo *aux)
{
	char	tmpbuf[1024];

	if (aux == NULL) {
		vrErrPrintf("_CybergloveInitializeDevice(): "
			RED_TEXT "Warning, no auxiliary data for Cyberglove.\n" NORM_TEXT);
		return -1;
	}


	/********************/
	/* clear the palate */
	_CybergloveReadInput(aux);

	/**********************************/
	/* request version and param info */
	do {
		vrSerialWriteString(aux->fd, "?i");	/* request info msg */
		vrSerialAwaitData(aux->fd);
	} while (_CybergloveReadInput(aux) != 'i');

	do {
		vrSerialWriteString(aux->fd, "?r");	/* request handedness */
		vrSerialAwaitData(aux->fd);
	} while (_CybergloveReadInput(aux) != 'r');

	do {
		vrSerialWriteString(aux->fd, "?b");	/* request baud rate */
		vrSerialAwaitData(aux->fd);
	} while (_CybergloveReadInput(aux) != 'b');

	do {
		vrSerialWriteString(aux->fd, "?g");	/* request glove info */
		vrSerialAwaitData(aux->fd);
	} while (_CybergloveReadInput(aux) != 'g');

	do {
		vrSerialWriteString(aux->fd, "?m");	/* request sensor mask */
		vrSerialAwaitData(aux->fd);
	} while (_CybergloveReadInput(aux) != 'm');

	do {
		vrSerialWriteString(aux->fd, "?n");	/* request number of sensors reported */
		vrSerialAwaitData(aux->fd);
	} while (_CybergloveReadInput(aux) != 'n');


	/********************************/
	/* send some device setup codes */

	do {
		vrSerialWriteString(aux->fd, "u 1\r");	/* include glove status with bend data */
		vrSerialAwaitData(aux->fd);
	} while (_CybergloveReadInput(aux) != 'u');

	do {
		vrSerialWriteString(aux->fd, "?u");	/* verify glove status inclusion */
		vrSerialAwaitData(aux->fd);
	} while (_CybergloveReadInput(aux) != 'u');


	/************************/
	/* read some input data */
#if 1
	do {
		vrSerialWriteString(aux->fd, "g");	/* poll for glove data */
		vrSerialAwaitData(aux->fd);
	} while (_CybergloveReadInput(aux) != 'g');
#endif

#if POLLMODE
	vrSerialWriteString(aux->fd, "g");	/* poll for glove data */
#else
	vrSerialWriteString(aux->fd, "s");	/* request streaming glove data */
#endif

	return 0;
}



/******************************************************/
static unsigned int _CybergloveButtonValue(char *buttonname)
{
	switch (buttonname[0]) {
	case 'l':
	case 'L':
	case '1':
		return CYBERGLOVE_BUTTONINDEX_x1x;
	case 'm':
	case 'M':
	case '2':
		return CYBERGLOVE_BUTTONINDEX_x2x;
	case 'r':
	case 'R':
	case '3':
		return CYBERGLOVE_BUTTONINDEX_x3x;
	}

	return -1;
}


	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/


#if defined(FREEVR) /* { */
/*****************************************************************/
/*** Functions for FreeVR access of Cyberglove devices for input ***/
/*****************************************************************/


	/*************************************/
	/***  FreeVR NON public routines   ***/
	/*************************************/


/*********************************************************/
static void _CybergloveParseArgs(_CyberglovePrivateInfo *aux, char *args)
{
static	char	*XX_choices[] = { "up", "away", NULL };
static	int	XX_values[] = { 1, 0 };
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

	/** TODO: other arguments to parse go here **/
}


/************************************************************/
static void _CybergloveGetData(vrInputDevice *devinfo)
{
	_CyberglovePrivateInfo	*aux = (_CyberglovePrivateInfo *)devinfo->aux_data;
	char			msg_type;		/* change from skel */
	int			buttons[3];
static	int			buttons_last = 0;
	int			count;
	float			value;

	aux->data_change = 0;

	/*******************/
	/* gather the data */
	msg_type = _CybergloveReadInput(aux);

	switch (msg_type) {
	case 'g':	/* ASCII data -- polled */
	case 's':	/* ASCII data -- stream */
	case 'G':	/* BINARY data -- polled */
	case 'S':	/* BINARY data -- stream */
		aux->data_change = 1;

		break;
	}

	/***************/
	/** valuators **/
	if (aux->data_change) {
		for (count = 0; count < MAX_GLOVESENSORS; count++) {
			if (aux->valuator_inputs[count] != NULL) {
				switch (aux->valuator_inputs[count]->input_type) {
				case VRINPUT_VALUATOR:
					value = aux->glove_data[count] * (1.0/255.0);
#if 0
vrPrintf("about to assign value %f to input %#p\n", value, aux->valuator_inputs[count]);
#endif
					vrAssignValuatorValue((vrValuator *)(aux->valuator_inputs[count]), value);
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
#ifdef NOT_YET_IMPLEMENTED
		if (aux->sensor6_options.ignore_all || aux->sensor6_options.ignore_trans) {
			for (count = 0; count < (aux->sensor6_options.ignore_all ? 6 : 3); count++) {
				if (aux->valuator_inputs[count] != NULL) {
					switch (aux->valuator_inputs[count]->input_type) {
					case VRINPUT_VALUATOR:
						vrAssignValuatorValue((vrValuator *)(aux->valuator_inputs[count]), aux->data[count] * aux->scale_valuator * aux->valuator_sign[count]);
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
#ifdef OLD_6SENSOR_CONV
			values[VR_X] = aux->data[VR_X] * scale_trans;
			values[VR_Y] = aux->data[VR_Y] * scale_trans;
			values[VR_Z] = aux->data[VR_Z] * scale_trans;
			values[VR_AZIM+3] = aux->data[VR_AZIM+3] * scale_rot;
			values[VR_ELEV+3] = aux->data[VR_ELEV+3] * scale_rot;
			values[VR_ROLL+3] = aux->data[VR_ROLL+3] * scale_rot;
#else
			values[VR_X] = aux->data[VR_X];
			values[VR_Y] = aux->data[VR_Y];
			values[VR_Z] = aux->data[VR_Z];
			values[VR_AZIM+3] = aux->data[VR_AZIM+3];
			values[VR_ELEV+3] = aux->data[VR_ELEV+3];
			values[VR_ROLL+3] = aux->data[VR_ROLL+3];
#endif
			vrAssign6sensorValueFromValuators(&(devinfo->sensor6[aux->active_sim6sensor]), values, &(aux->sensor6_options));
		}
	}

	/*************/
	/** buttons **/
	if (aux->button_change) {
		for (count = 0; count < MAX_GLOVEBUTTONS; count++) {
			if ((aux->buttons & (0x1 << count)) != (buttons_last & (0x1 << count))) {
				if (aux->button_inputs[count] != NULL) {
					switch (aux->button_inputs[count]->input_type) {
					case VRINPUT_BINARY:
						vrAssign2switchValue((vr2switch *)(aux->button_inputs[count]), ((aux->buttons & (0x1 << count)) != 0));
						break;
					case VRINPUT_CONTROL:
						vrCallbackInvokeDynamic(((vrControl *)(aux->button_inputs[count]))->callback, 1, ((aux->buttons & (0x1 << count)) != 0));
						break;
					default:
						vrErrPrintf(RED_TEXT "_CybergloveGetData: Unable to handle button inputs that aren't Binary or Control inputs\n" NORM_TEXT);
						break;
					}
				}
			}
		}

		buttons_last = aux->buttons;
#endif
	}
}


	/*******************************************************************/
	/*    Function(s) for parsing Cyberglove "input" declarations.     */
	/*                                                                 */
	/*  These _Cyberglove<type>Input() functions are called during the */
	/*  CREATE phase of the input interface.                           */

/**************************************************************************/
static vrInputMatch _CybergloveButtonInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
        _CyberglovePrivateInfo	*aux = (_CyberglovePrivateInfo *)devinfo->aux_data;
	int			button_num;

#ifdef NOT_YET_IMPLEMENTED
	button_num = _CybergloveButtonValue(dti->instance);
	if (button_num == -1)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_CybergloveButtonValue: Warning, button['%s'] did not match any known button\n", dti->instance);
	else if (aux->button_inputs[button_num] != NULL)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_CybergloveButtonValue: Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);
	aux->button_inputs[button_num] = (vr2switch *)input;
	vrDbgPrintfN(INPUT_DBGLVL, "assigned button event of value 0x%02x to input pointer = %#p)\n",
		button_num, aux->button_inputs[button_num]);
#endif /* NOT_YET_IMPLEMENTED */

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/**************************************************************************/
static vrInputMatch _CybergloveValInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
        _CyberglovePrivateInfo	*aux = (_CyberglovePrivateInfo *)devinfo->aux_data;
	int			valuator_num;

#if 0 /* TODO: valuator num should be decoded from an alpha string that indicates joint */
	valuator_num = _CybergloveValuatorValue(dti->instance);
#else
	valuator_num = vrAtoI(dti->instance);
#endif
	if (valuator_num == -1)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_CybergloveValInput: Warning, valuator['%s'] did not match any known valuator\n", dti->instance);
	else if (aux->valuator_inputs[valuator_num] != NULL)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_CybergloveValInput: Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);
	aux->valuator_inputs[valuator_num] = (vrValuator *)input;
	vrDbgPrintfN(INPUT_DBGLVL, "assigned valuator event of value 0x%02x to input pointer = %#p)\n",
		valuator_num, aux->valuator_inputs[valuator_num]);

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/**************************************************************************/
static vrInputMatch _CybergloveReceiverInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
        _CyberglovePrivateInfo	*aux = (_CyberglovePrivateInfo *)devinfo->aux_data;
	int			sensor_num;

#ifdef NOT_YET_IMPLEMENTED
	sensor_num = atoi(dti->instance);
	if (sensor_num < 0)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_CybergloveReceiverInput: Warning, sensor number must be between %d and %d\n", 0, MAX_6SENSORS);
	else if (aux->sensor6_inputs[sensor_num] != NULL)
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, RED_TEXT "_CybergloveReceiverInput: Warning, new input from %s[%s] overwriting old value.\n" NORM_TEXT,
			dti->type, dti->instance);
	aux->sensor6_inputs[sensor_num] = (vr6sensor *)input;
	vrAssign6sensorR2Exform(aux->sensor6_inputs[sensor_num], strchr(dti->instance, ','));
	vrDbgPrintfN(INPUT_DBGLVL, "assigned 6sensor event of value 0x%02x to input pointer = %#p)\n",
		sensor_num, aux->sensor6_inputs[sensor_num]);
#endif /* NOT_YET_IMPLEMENTED */

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


	/************************************************************/
	/***************** FreeVR Callback routines *****************/
	/************************************************************/

	/************************************************************/
	/*    Callbacks for controlling the Cyberglove features.      */
	/*                                                          */

/************************************************************/
static void _CybergloveSystemPauseToggleCallback(vrInputDevice *devinfo, int value)
{
	_CyberglovePrivateInfo	*aux = (_CyberglovePrivateInfo *)devinfo->aux_data;

        if (value == 0)
                return;

	devinfo->context->paused ^= 1;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Cyberglove Control: system_pause = %d.\n",
		devinfo->context->paused);
}

/************************************************************/
static void _CyberglovePrintContextStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintContext(stdout, devinfo->context, verbose);
}

/************************************************************/
static void _CyberglovePrintConfigStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintConfig(stdout, devinfo->context->config, verbose);
}

/************************************************************/
static void _CyberglovePrintInputStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintInput(stdout, devinfo->context->input, verbose);
}

/************************************************************/
static void _CyberglovePrintStructCallback(vrInputDevice *devinfo, int value)
{
        _CyberglovePrivateInfo  *aux = (_CyberglovePrivateInfo *)devinfo->aux_data;

        if (value == 0)
                return;

        _CyberglovePrintStruct(stdout, aux, verbose);
}

/************************************************************/
static void _CyberglovePrintHelpCallback(vrInputDevice *devinfo, int value)
{
        _CyberglovePrivateInfo  *aux = (_CyberglovePrivateInfo *)devinfo->aux_data;

        if (value == 0)
                return;

        _CyberglovePrintHelp(stdout, aux);
}



	/************************************************************/
	/*   Callbacks for interfacing with the Cyberglove device.  */
	/*                                                          */


/************************************************************/
static void _CybergloveCreateFunction(vrInputDevice *devinfo)
{
	/*** List of possible inputs ***/
static	vrInputFunction	_CybergloveInputs[] = {
				{ "button", VRINPUT_2WAY, _CybergloveButtonInput },
				{ "glove", VRINPUT_VALUATOR, _CybergloveValInput },
				{ "receiver", VRINPUT_6SENSOR, _CybergloveReceiverInput },
				{ NULL, VRINPUT_UNKNOWN, NULL } };
	/*** List of control functions ***/
static	vrControlFunc	_CybergloveControlList[] = {
				/* overall system controls */
				{ "system_pause_toggle", _CybergloveSystemPauseToggleCallback },

				/* informational output controls */
				{ "print_context", _CyberglovePrintContextStructCallback },
				{ "print_config", _CyberglovePrintConfigStructCallback },
				{ "print_input", _CyberglovePrintInputStructCallback },
				{ "print_struct", _CyberglovePrintStructCallback },
				{ "print_help", _CyberglovePrintHelpCallback },

		/** TODO: other callback control functions go here **/
				/* simulated 6-sensor selection controls */
				/* simulated 6-sensor manipulation controls */
				/* other controls */

				/* end of the list */
				{ NULL, NULL } };

	_CyberglovePrivateInfo	*aux = NULL;

	/******************************************/
	/* allocate and initialize auxiliary data */
	devinfo->aux_data = (void *)vrShmemAlloc0(sizeof(_CyberglovePrivateInfo));
	aux = (_CyberglovePrivateInfo *)devinfo->aux_data;
	_CybergloveInitializeStruct(aux);

	/******************/
	/* handle options */
	aux->port = vrShmemStrDup(DEFAULT_PORT);	/* default, if no port given */
	aux->baud_int = DEFAULT_BAUD;			/* default, if no baud given */
	aux->baud_enum = vrSerialBaudIntToEnum(aux->baud_int);
	_CybergloveParseArgs(aux, devinfo->args);

	/***************************************/
	/* create the inputs and self-controls */
	vrInputCreateDataContainers(devinfo, _CybergloveInputs);
	vrInputCreateSelfControlContainers(devinfo, _CybergloveInputs, _CybergloveControlList);
	/* TODO: anything to do for NON self-controls?  Implement for Magellan first. */

	vrDbgPrintf("Done creating Cyberglove inputs for '%s'\n", devinfo->name);
	devinfo->created = 1;

	return;
}


/************************************************************/
static void _CybergloveOpenFunction(vrInputDevice *devinfo)
{
	vrTrace("_CybergloveOpenFunction", devinfo->name);

	_CyberglovePrivateInfo	*aux = (_CyberglovePrivateInfo *)devinfo->aux_data;

	/*******************/
	/* open the device */
	aux->fd = vrSerialOpen(aux->port, aux->baud_enum);
	if (aux->fd < 0) {
		aux->open = 0;
		vrErrPrintf("(%s::_CybergloveFunction::%d) error: "
			RED_TEXT "couldn't open serial port %s for %s\n" NORM_TEXT,
			__FILE__, __LINE__, aux->port, devinfo->name);
		sprintf(aux->version, "- unconnected Cyberglove -");
	} else {
		aux->open = 1;
		if (_CybergloveInitializeDevice(aux) < 0) {
			vrErrPrintf("_CybergloveOpenFunction(): "
				RED_TEXT "Warning, unable to initialize '%s' Cyberglove.\n" NORM_TEXT,
				devinfo->name);
		} else {
			vrDbgPrintf("_CybergloveOpenFunction(): Done opening Cyberglove input device '%s'.\n", devinfo->name);
			devinfo->operating = 1;
		}
	}
#if 1
_CyberglovePrintStruct(stdout, aux, verbose);
#endif

	return;
}


/************************************************************/
static void _CybergloveCloseFunction(vrInputDevice *devinfo)
{
	_CyberglovePrivateInfo	*aux = (_CyberglovePrivateInfo *)devinfo->aux_data;

	if (aux != NULL) {
		vrSerialClose(aux->fd);
		vrShmemFree(aux);	/* aka devinfo->aux_data */
	}
	return;
}


/************************************************************/
static void _CybergloveResetFunction(vrInputDevice *devinfo)
{
	/* TODO: reset code */
	return;
}


/************************************************************/
static void _CyberglovePollFunction(vrInputDevice *devinfo)
{
	_CyberglovePrivateInfo	*aux = (_CyberglovePrivateInfo *)devinfo->aux_data;

	if (devinfo->operating) {
#if POLLMODE
		vrSerialWriteString(aux->fd, "g");	/* poll for glove data */
		vrSleep(250000);
#endif
		_CybergloveGetData(devinfo);
	} else {
		/* TODO: try to open the device again */
	}

	return;
}



	/******************************/
	/*** FreeVR public routines ***/
	/******************************/


/******************************************************/
void vrCybergloveInitInfo(vrInputDevice *devinfo)
{
	devinfo->version = (char *)vrShmemStrDup("-get from Cyberglove device-");
	devinfo->Create = vrCallbackCreateNamed("Cyberglove:Create-Def", _CybergloveCreateFunction, 1, devinfo);
	devinfo->Open = vrCallbackCreateNamed("Cyberglove:Open-Def", _CybergloveOpenFunction, 1, devinfo);
	devinfo->Close = vrCallbackCreateNamed("Cyberglove:Close-Def", _CybergloveCloseFunction, 1, devinfo);
	devinfo->Reset = vrCallbackCreateNamed("Cyberglove:Reset-Def", _CybergloveResetFunction, 1, devinfo);
	devinfo->PollData = vrCallbackCreateNamed("Cyberglove:PollData-Def", _CyberglovePollFunction, 1, devinfo);
	devinfo->PrintAux = vrCallbackCreateNamed("Cyberglove:PrintAux-Def", _CyberglovePrintStruct, 0);

	vrDbgPrintfN(INPUT_DBGLVL, "vrCybergloveInitInfo: callbacks created.\n");
}


#endif /* } FREEVR */



	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/



#if defined(TEST_APP) /* { */

/* A test program to communicate with a Cyberglove device and print the results. */
main(int argc, char *argv[])
{
	_CyberglovePrivateInfo	*aux;
	int			count;


	/******************************/
	/* setup the device structure */
	aux = (_CyberglovePrivateInfo *)malloc(sizeof(_CyberglovePrivateInfo));
	memset(aux, 0, sizeof(_CyberglovePrivateInfo));
	_CybergloveInitializeStruct(aux);


	/*********************************************************/
	/* set default parameters based on environment variables */
	/* TODO: use of environment variables */
	aux->port = DEFAULT_PORT;			/* default, if no file given */
	aux->baud_int = DEFAULT_BAUD;			/* default, if no baud given */
	aux->baud_enum = vrSerialBaudIntToEnum(aux->baud_int);

	/*****************************************************/
	/* adjust parameters based on command-line-arguments */
	/* TODO: parse CLAs for non-default serial port, baud, etc. */


	/**************************************************/
	/* open the serial port and initialize the device */
	aux->fd = vrSerialOpen(aux->port, aux->baud_enum);
	if (aux->fd < 0) {
		aux->open = 0;
		fprintf(stderr, RED_TEXT "couldn't open serial port %s\n" NORM_TEXT, aux->port);
		sprintf(aux->version, "- unconnected Cyberglove -");
	} else {
		aux->open = 1;
		if (_CybergloveInitializeDevice(aux) < 0) {
			vrErrPrintf("main: "
				RED_TEXT "Warning, unable to initialize Cyberglove.\n" NORM_TEXT);
		}
	}

	_CyberglovePrintStruct(stdout, aux, verbose);
	fprintf(stdout, "\n");


	/**********************/
	/* display the output */
	while (1 /* or something */) {
		_CybergloveReadInput(aux);
		fprintf(stdout, "glove_data -- ");
		for (count = 0; (count < (MAX_GLOVESENSORS+5)) && (aux->glove_data[count] != -1); count++)
			fprintf(stdout, "%3d ", aux->glove_data[count]);
		fprintf(stdout, "    \r");
		fflush(stdout);
#if POLLMODE
		vrSerialWriteString(aux->fd, "g");	/* poll for glove data */
		vrSleep(250000);
#endif
	}


	/*****************/
	/* close up shop */
	if (aux != NULL) {
		vrSerialClose(aux->fd);
		free(aux);			/* aka devinfo->aux_data */
	}

	printf("\nCyberglove device closed\n");
}

#endif /* } TEST_APP */



	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/



#if defined(CAVE) /* { */


	/* ... CAVE stuff here if to also work with CAVElib */


#endif /* } CAVE */

