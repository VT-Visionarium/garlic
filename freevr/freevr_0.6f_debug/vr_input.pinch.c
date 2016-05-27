/* ======================================================================
 *
 *  CCCCC          vr_input.pinch.c
 * CC   CC         Author(s): Bill Sherman
 * CC              Created: December 23, 1999
 * CC   CC         Last Modified: June 7, 2003
 *  CCCCC
 *
 * Code file for FreeVR tracking from the FakeSpace Pinch Glove device.
 *   (used vr_input.static.c as the code base)
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/*************************************************************************

USAGE:
	Currently, the pinch glove device operates only as a means of
	generating button (2switch) inputs.
	(Other possibilities include a valuator based on a hand, or an
	N-switch (N=5) based on which finger the thumb is touching (0=none).)

	Inputs are specified with the "input" option:
		input "<name>" = "2switch(contact[<lefthand byte>:<righthand byte>])";

		(NOTE: we might also allow contacts to be defined using a string of
		fingers from the list: LT, LI, LM, LR, LP, RT, RI, RM, RR, RP -- and
		something for the palm that doesn't conflict with pinky.)

	Controls are specified with the "control" option:
		control "<control option>" = "2switch(contact[<lefthand byte>.<righthand byte>])";

	Here is the available control options for FreeVR:
		"print_help" -- print info on how to use the input device
		"system_pause_toggle" -- toggle the system pause flag
		"print_context" -- print the overall FreeVR context data structure (for debugging)
		"print_config" -- print the overall FreeVR config data structure (for debugging)
		"print_input" -- print the overall FreeVR input data structure (for debugging)
		"print_struct" -- print the internal PinchGlove data structure (for debugging)

	Here are the FreeVR configuration argument options for the PinchGlove:
		"port" - serial port PinchGlove is connected to
			("/dev/input/pinchglove" is the default)
		"baud" - baud rate of serial port connection (generally 9600)
			(9600 is the default)

HISTORY:
	23-27 December 1999 (Bill Sherman) -- wrote initial version (on a
		ThinkPad running Linux) using vr_input.static.c as the base.

	29 December 1999 (Bill Sherman) -- wrote the test application, and wrote
		new CREATE section of _PinchgloveFunction() to comply with new
		input device open sequence.

	3 January 2000 (Bill Sherman) -- Implemented the vrInputFunction method
		of parsing "input" configuration lines.  And added the
		"print_struct" control callback to help to some debugging.

	24 January 2000  (Bill Sherman) -- Implemented (and then generalized) the
		algorithm for creating self-control inputs.

	3 May 2001 (Bill Sherman)
		I made a few minor changes to catch up to the general
		  vr_input.skeleton.c format.

	11 January 2002 (Bill Sherman) -- Added code to quit if process is set
		to end while still trying to initialize device (which is
		especially important if device isn't even connected).

	11 September 2002 (Bill Sherman)
		Moved the control callback array into the _PinchgloveFunction()
		callback.

	21-23 April 2003 (Bill Sherman)
		Updated to new input matching function code style.  Changed
		  "opaque" field to "aux_data".  Split _PinchgloveFunction()
		  into 5 functions.  Added new vrPrintStyle argument to
		  _PinchglovePrintStruct() for the sake of the new "PrintAux"
		  callback.

	3 June 2003 (Bill Sherman)
		Now include "vr_enums.h" for the TEST_APP code.
		Added the address of the auxiliary data to the printout.
		Added the "system_pause_toggle" control callback.
		Added comments classifying the controls.

	16 October 2009 (Bill Sherman)
		A quick fix to the _PinchgloveParseArgs() routine to handle
		  the no-arguments case.

	2 September 2013 (Bill Sherman) -- adopting a consistent usage
		of str<...>cmp() functions (using logical negation).

	14 September 2013 (Bill Sherman)
		I am making use of the newly created SELFCTRL_DBGLVL for all
		the self-control outputs.  I then added some reasonable output
		to the "print_help" callback to print out what all the device's
		inputs are doing.

	15 September 2013 (Bill Sherman)
		Changed the "0x%p" format to the improved "%#p" format. (once)

TODO:
	- Give up trying to open the device after some amount of time.

	- Determine and Return an error value when unable to initialize
		in _PinchgloveInitializeDevice().

	- Implement routine for attempting to open a device that wasn't
		successfully opened the first time.

	- Implement an N-way switch input type allowing the config file to
		specify one (or both) hand as an N-way.  This would result in
		an input that gives one of upto 5 values.  So, an input config
		might look like: input "<name>" = "switchN(lefthand[<max value>])";

	- Figure out a good way to inform the VR system when an error occurs
		(such as receiving a bad packet because a 0x11 byte went missing). 

	- Find out if there is a way to poll the device for current contacts,
		which can be used when a bad packet is received (of course, if
		the 0x11 bytes are still missing, that won't help much).

	- Find out what the protocol is for the gloves with palm connections.

*************************************************************************/
#if !defined(FREEVR) && !defined(TEST_APP) && !defined(CAVE)
/* Choose how this code should be compiled (ie. for FreeVR, testing, etc). */
#  define	FREEVR
#  undef	TEST_APP
#  undef	CAVE
#endif

#include <stdio.h>
#include <stdlib.h>	/* for atof() */
#include <string.h>
#include <signal.h>

#include "vr_serial.h"
#include "vr_debug.h"

#if defined(FREEVR)
#  include "vr_input.h"
#  include "vr_input.opts.h"
#  include "vr_shmem.h"
#  include "vr_math.h"	/* for vrAtoI() */
#  include "vr_parse.h"	/* for vrParseInputDTI() */
#endif

#if defined(TEST_APP)
#  include "vr_serial.c"
#  include "vr_enums.h"
#endif



	/*******************************************************/
	/*** definitions for interfacing with the PinchGlove ***/
	/***                                                 ***/

#define	DEFAULT_PORT	"/dev/input/pinchglove"
#define	DEFAULT_BAUD	9600

/*** PinchGlove protocol bytes ***/
#define PINCH_HANDDATA		((char)0x80)
#define PINCH_HANDTIMEDATA	((char)0x81)
#define PINCH_TEXTDATA		((char)0x82)
#define PINCH_ENDBYTE		((char)0x8F)

/*** PinchGlove command sequences ***/
#define CopyrightMsg	"CP"	/* request processor info (incl. copyright) */
#define LeftGloveMsg	"CL"	/* request left glove info */
#define RightGloveMsg	"CR"	/* request right glove info */
#define TimerInfoMsg	"CT"	/* request timing info */
#define TimerOnMsg	"T1"	/* turn timer info reporting on */
#define TimerOffMsg	"T0"	/* turn timer info reporting off */
#define ProtocolVersMsg	"V0"	/* request version of command protocol */
#define Protocol1Msg	"V1"	/* request to use command protocol version 1 */


/********************************************************************/
/*** auxiliary structure of the current data from the PinchGlove. ***/
typedef struct {
		/* these are for PinchGlove interfacing */
#ifndef TEST_APP /* { */
		vrInputDevice	*device;	/* pointer back to device to which this struct belongs */
#endif /* } */
		int		fd;		/* file descriptor */
		char		*port;		/* name of the serial port */
		int		baud_enum;	/* communication rate as an enumerated value */
		int		baud_int;	/* communication rate as the real value */
		int		open;		/* flag when pinchglove port successfully open */

		/* these are for PinchGlove data parsing */
		int		bad_datapackets;/* count the number of bad contact data packets */
		char		lo_buf[512];	/* buffer of left-over bytes */
		int		lo_buflen;	/* number of left-over bytes */
		char		last_message[1025];/* string with the last text message from Pinch*/
		char		version[1025];	/* string containing PinchGlove version & (c) info*/
		char		lg_info[1025];	/* string containing left glove info */
		int		lg_connected;	/* boolean of whether the left glove is properly connected */
		char		rg_info[1025];	/* string containing right glove info */
		int		rg_connected;	/* boolean of whether the right glove is properly connected */
		char		time_info[256];	/* string containing timing inputs */
		double		time_conv;	/* value for converting time ticks to seconds */
		char		timer_value[16];/* string containing on/off status of timer */
		char		protocol_version[16];/* string containing command protocol in use */

		/* information about the most recent data packet */
		double		time_delta;	/* time since last contact -- max of ... */
		int		num_contacts;	/* number of current sets of contacts */
		unsigned char	left_contacts[5];
		unsigned char	right_contacts[5];

#if defined(FREEVR)
		/* FREEVR input information */
#  define MAX_BUTTONS	20
#  define MAX_VALUATORS	 2
#  define MAX_NSENSORS	 2

		int		num_postures;
		unsigned char	posturecode_left[MAX_BUTTONS];
		unsigned char	posturecode_right[MAX_BUTTONS];
		int		button_values[MAX_BUTTONS];
		vr2switch	*button_inputs[MAX_BUTTONS];
		vrValuator	*valuator_inputs[MAX_VALUATORS];
		vrNsensor	*sensorN_inputs[MAX_NSENSORS];
#endif

	} _PinchglovePrivateInfo;



	/***********************************/
	/*** General NON public routines ***/
	/***********************************/

/**************************************************************************/
/* typename is used to specify a particular device among many that */
/*   share (more or less) the same protocol.  The typename is then */
/*   used to determine what specific features are available with   */
/*   this particular type of device.  Currently there is only one  */
/*   type of device handled by this file, so there is no typename  */
/*   argument.                                                     */
/*                                                                 */
static void _PinchgloveInitializeStruct(_PinchglovePrivateInfo *pinch)
{
	pinch->lo_buf[0] = '\0';
	pinch->lo_buflen = 0;

	pinch->version[0] = '\0';
	pinch->lg_info[0] = '\0';
	pinch->lg_connected = 1;
	pinch->rg_info[0] = '\0';
	pinch->rg_connected = 1;
	pinch->time_info[0] = '\0';
	pinch->time_conv = 1.0;
	pinch->time_delta = -1.0;

	pinch->num_contacts = 0;
	pinch->bad_datapackets = 0;

#ifdef FREEVR
	pinch->num_postures = 0;
#endif
}


/**************************************************************************/
static void _PinchglovePrintStruct(FILE *file, _PinchglovePrivateInfo *pinch, vrPrintStyle style)
{
	int	count;

	vrFprintf(file, "PinchGlove device internal structure (%#p):\n", pinch);
	vrFprintf(file, "\r\tversion -- '%s'\n", pinch->version);
	vrFprintf(file, "\r\tleft glove info -- '%s' (%d)\n", pinch->lg_info, pinch->lg_connected);
	vrFprintf(file, "\r\tright glove info -- '%s' (%d)\n", pinch->rg_info, pinch->rg_connected);
	vrFprintf(file, "\r\ttimer conversion string -- '%s'\n", pinch->time_info);
	vrFprintf(file, "\r\ttimer conversion value -- %lf\n", pinch->time_conv);
	vrFprintf(file, "\r\ttimer value -- '%s'\n", pinch->timer_value);
	vrFprintf(file, "\r\tcommand protocol -- '%s'\n", pinch->protocol_version);
	vrFprintf(file, "\r\tbad data packets -- %d\n", pinch->bad_datapackets);
	vrFprintf(file, "\r\ttime since last contact -- %lf\n", pinch->time_delta);
	vrFprintf(file, "\r\tnumber of current contacts: %d\n", pinch->num_contacts);
	for (count = 0; count < pinch->num_contacts; count++)
		vrFprintf(file, "\r\t\tcontact: 0x%02x.0x%02x\n", pinch->left_contacts[count], pinch->right_contacts[count]);
}


/**************************************************************************/
static void _PinchglovePrintHelp(FILE *file, _PinchglovePrivateInfo *aux)
{
	int	count;		/* looping variable */

#if 0 || defined(TEST_APP)
	vrFprintf(file, BOLD_TEXT "Sorry, PinchGlove - print_help control not yet implemented.\n" NORM_TEXT);
#else
	vrFprintf(file, BOLD_TEXT "PinchGlove - inputs:" NORM_TEXT "\n");
	for (count = 0; count < MAX_BUTTONS; count++) {
		if (aux->button_inputs[count] != NULL) {
			vrFprintf(file, "\t%s -- %s%s\n",
				aux->button_inputs[count]->my_object->desc_str,
				(aux->button_inputs[count]->input_type == VRINPUT_CONTROL ? "control:" : ""),
				aux->button_inputs[count]->my_object->name);
		}
	}

	/* NOTE: PinchGlove has no valuator or 6sensor inputs */

#ifdef NOT_YET_IMPLEMENTED
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
#endif /* NOT_YET_IMPLEMENTED */
#endif
}



/**************************************************************************/
/* note: this combines the functionality of MagellanGetInputs() and MagellanDecodeBuffer() */
/*   This function returns a character based on the most recently received   */
/*   message type (i.e. the first character of the message).                 */
static char _PinchgloveReadInput(_PinchglovePrivateInfo *pinch)
{
	char	buffer[512];
	char	return_code = '\0';

	if (vrSerialReadToChar(pinch->fd, PINCH_ENDBYTE, pinch->lo_buf, (int)sizeof(pinch->lo_buf), pinch->lo_buflen, buffer, sizeof(buffer)) == NULL) {
		return 0;
	}

	if (pinch->lo_buflen > 0)
		vrDbgPrintfN(PINCHG_DBGLVL, "_PinchgloveReadInput(): leftover buffer has %d bytes\n", pinch->lo_buflen);

	switch(buffer[0]) {

	case PINCH_TEXTDATA: /* text information */
		vrDbgPrintfN(PINCHG_DBGLVL, "_PinchgloveReadInput(): text message: '%s'\n", &buffer[1]);

		/* replace the pinchglove endbyte with a '\0' */
		*(strchr(buffer, PINCH_ENDBYTE)) = '\0';

		/* copy the text to the incoming text buffer */
		strncpy(pinch->last_message, &buffer[1], sizeof(pinch->last_message));

		/* for certain messages, copy to the appropriate location */
		/* (ie. the ones that can be determined from the string) */
		switch(buffer[1]) {
		case 'V':
			strncpy(pinch->version, &buffer[1], sizeof(pinch->version));
			return_code = 'V';
			break;
		case 'L':
			strncpy(pinch->lg_info, &buffer[1], sizeof(pinch->lg_info));
			return_code = 'L';
			break;
		case 'R':
			strncpy(pinch->rg_info, &buffer[1], sizeof(pinch->rg_info));
			return_code = 'R';
			break;
		case '?':
			return_code = '?';
			break;
		default:
			return_code = 'T';
			break;
		}
		break;

	case PINCH_HANDDATA:
	case PINCH_HANDTIMEDATA:
	 {
		int	count;
		int	msg_len;
		int	time_delta;
		char	*endbyte;

		vrDbgPrintfN(PINCHG_DBGLVL, "_PinchgloveReadInput(): "
			"bytes: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			buffer[0], buffer[1], buffer[2], buffer[3], buffer[4],
			buffer[5], buffer[6], buffer[7], buffer[8], buffer[9]);

		return_code = 'C';
		endbyte = memchr(buffer, PINCH_ENDBYTE, sizeof(buffer));
		msg_len = (int)(endbyte - buffer);
		vrDbgPrintfN(PINCHG_DBGLVL, "_PinchgloveReadInput(): "
			"got a contact message of length %d\n", msg_len);

		/* if msg_len is not odd, then there is something wrong    */
		/*   with the packet -- s/b start-byte + 2 * num_contacts. */
		if (msg_len == (msg_len/2)*2) {
			/* msg_len is even */
			pinch->bad_datapackets++;
			vrErrPrintf("_PinchgloveReadInput(): "
				RED_TEXT "wrong number of bytes in contact data packet (%d)\n"
				NORM_TEXT, msg_len);
		} else {
			/* msg_len is odd */

			if (buffer[0] == PINCH_HANDTIMEDATA) {
				/* if this is a contact data packet with time info, */
				/*   then calculate the time delta.                 */
				time_delta = (int)(endbyte[-2]) + 128 * (int)(endbyte[-1]);
				pinch->time_delta = time_delta * pinch->time_conv;
				vrDbgPrintfN(PINCHG_DBGLVL, "_PinchgloveReadInput(): "
					"time delta = %lf (%d)\n", pinch->time_delta, time_delta);

				/* subtract off the two bytes used for timer info */
				msg_len -= 2;
			}

			pinch->num_contacts = (msg_len - 1) / 2;
			vrDbgPrintfN(PINCHG_DBGLVL, "_PinchgloveReadInput(): "
				"num_contact = %d\n", pinch->num_contacts);

			for (count = 0; count < pinch->num_contacts; count++) {
				pinch->left_contacts[count] = buffer[count*2 + 1];
				pinch->right_contacts[count] = buffer[count*2 + 2];
			}
		}

		if (vrDbgDo(PINCHG_DBGLVL) && pinch->time_delta > 3.0)
			_PinchglovePrintStruct(stdout, pinch, verbose);
		break;
	 }
	}

	return (return_code);
}


/**************************************************************************/
/* _PinchgloveInitializeDevice() is called in the OPEN phase  */
/*   of input interface -- after the types of inputs have     */
/*   been determined (during the CREATE phase).               */
static void _PinchgloveInitializeDevice(_PinchglovePrivateInfo *pinch)
{
	/* get the information about the connected pinchglove */
#ifdef TEST_APP /* { */
	printf("requesting copyright message\n");
#endif /* } */
	do {
		vrSerialWriteString(pinch->fd, CopyrightMsg);
		vrSerialAwaitData(pinch->fd);
	} while (_PinchgloveReadInput(pinch) != 'V' 
#ifndef TEST_APP /* { */
			&& pinch->device->proc->end_proc == 0
#endif /* } */
		);
#ifdef TEST_APP /* { */
	printf("got copyright message\n");
#endif /* } */
#ifndef TEST_APP /* { */
	if (pinch->device->proc->end_proc == 1) {
		vrDbgPrintfN(AALWAYS_DBGLVL, RED_TEXT "Pinch Glove initialization terminated due to process exit request.\n" NORM_TEXT);
		return;
	}
#endif /* } */

	do {
		vrSerialWriteString(pinch->fd, LeftGloveMsg);
		vrSerialAwaitData(pinch->fd);
	} while (_PinchgloveReadInput(pinch) != 'L');
	if (!strcmp(pinch->lg_info, "L NO GLOVE")) {
		pinch->lg_connected = 0;
	}

	do {
		vrSerialWriteString(pinch->fd, RightGloveMsg);
		vrSerialAwaitData(pinch->fd);
	} while (_PinchgloveReadInput(pinch) != 'R');
	if (!strcmp(pinch->rg_info, "R NO GLOVE")) {
		pinch->rg_connected = 0;
	}

	do {
		vrSerialWriteString(pinch->fd, TimerInfoMsg);
		vrSerialAwaitData(pinch->fd);
	} while (_PinchgloveReadInput(pinch) != 'T');
	strcpy(pinch->time_info, pinch->last_message);
	pinch->time_conv = atof(pinch->time_info) / 1000000.0; /* TODO: really should check units of time_info */

	/* set the pinchglove options */
	do {
		vrSerialWriteString(pinch->fd, TimerOnMsg);
		vrSerialAwaitData(pinch->fd);
	} while (_PinchgloveReadInput(pinch) != 'T');
	strcpy(pinch->timer_value, pinch->last_message);

	do {
		vrSerialWriteString(pinch->fd, Protocol1Msg);
		vrSerialAwaitData(pinch->fd);
	} while (_PinchgloveReadInput(pinch) != 'T');
	strcpy(pinch->protocol_version, pinch->last_message);

	if (vrDbgDo(PINCHG_DBGLVL))
		_PinchglovePrintStruct(stdout, pinch, verbose);
}


	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/


#if defined(FREEVR) /* { */

	/**********************************/
	/*** FreeVR NON public routines ***/
	/**********************************/


/**************************************************************************/
static void _PinchgloveParseArgs(_PinchglovePrivateInfo *aux, char *args)
{
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
}


/**************************************************************************/
static void _PinchgloveGetData(vrInputDevice *devinfo)
{
	_PinchglovePrivateInfo	*aux = (_PinchglovePrivateInfo *)devinfo->aux_data;
	int			count_post;		/* for counting through the postures */
	int			count_cont;		/* for counting through the contacts */
	vr2switch		*current_button;	/* the button associated with a posture */
	int			*current_value;		/* current value of the current button */
	int			new_value;		/* new value of the current button */
	char			msg_type;

	/*******************/
	/* gather the data */
	msg_type = _PinchgloveReadInput(aux);

	/*************/
	/** buttons **/
	if (msg_type == 'C') {

		/* loop through all the postures we're interested in checking */
		for (count_post = 0; count_post < aux->num_postures; count_post++) {
			current_button = aux->button_inputs[count_post];
			current_value = &(aux->button_values[count_post]);

			/* check if the current posture is active */
			new_value = 0;
			for (count_cont = 0; count_cont < aux->num_contacts; count_cont++) {
				if (aux->left_contacts[count_cont] == aux->posturecode_left[count_post] && aux->right_contacts[count_cont] == aux->posturecode_right[count_post]) {
					new_value = 1;
				}
			}
			vrDbgPrintfN(ALMOSTNEVER_DBGLVL, RED_TEXT"posture %d (%02x.%02x): value = %d\n"NORM_TEXT, count_post, aux->posturecode_left[count_post], aux->posturecode_right[count_post], new_value);

			/* when the value actually changes, then assign the new value */
			if (*current_value != new_value) {
				*current_value = new_value;

				switch (current_button->input_type) {
				case VRINPUT_BINARY:
					vrDbgPrintfN(PINCHG_DBGLVL, "_PinchgloveGetData: "
						"Assigning a value of %d to pinch button %d\n", new_value, count_post);
					vrAssign2switchValue(current_button, new_value);
					break;

				case VRINPUT_CONTROL:
					vrDbgPrintfN(PINCHG_DBGLVL, "_PinchgloveGetData: "
						"Activating a callback with %d to pinch button %d\n", new_value, count_post);
					vrCallbackInvokeDynamic(((vrControl *)current_button)->callback, 1, new_value);
					break;

				default:
					vrErrPrintf(RED_TEXT "_PinchgloveGetData: Unable to handle button inputs that aren't Binary or Control inputs\n" NORM_TEXT);
					break;
				}
			}
		}
	}
}


	/*******************************************************************/
	/*    Function(s) for parsing PinchGlove "input" declarations.     */
	/*                                                                 */
	/*  These _Pinchglove<type>Input() functions are called during the */
	/*  CREATE phase of the input interface.                           */

/**************************************************************************/
static vrInputMatch _PinchgloveContactInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
	_PinchglovePrivateInfo	*aux = (_PinchglovePrivateInfo *)devinfo->aux_data;
	char			*left_contact_str;
	char			*right_contact_str;
	int			left_contact_val;
	int			right_contact_val;
	int			posture_num;
	int			posture_count;

	left_contact_str = dti->instance;
	right_contact_str = strchr(dti->instance, '.');
	if (left_contact_str == NULL || right_contact_str == NULL) {
		/* invalid contact description */
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_PinchgloveContactInput: "
			"Warning, contact['%s'] is invalid.\n",
			dti->instance);

		return VRINPUT_MATCH_UNABLE;	/* input declaration match, but unable to create */

	} else if (aux->num_postures == MAX_BUTTONS) {
		/* no room at the input */
		vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_PinchgloveContactInput: "
			"Warning, too many ""contact"" inputs, ignoring contact['%s'].\n",
			dti->instance);

		return VRINPUT_MATCH_UNABLE;	/* input declaration match, but unable to create */

	} else {
		/* add contact posture to list */
		left_contact_val = vrAtoI(left_contact_str);
		right_contact_val = vrAtoI(right_contact_str+1);

		/* check if the given posture is already in the list */
		for (posture_count = 0; posture_count < aux->num_postures; posture_count++) {
			if (left_contact_val == aux->posturecode_left[posture_count] && right_contact_val == aux->posturecode_right[posture_count]) {
				/* this posture is already in the list */
				vrDbgPrintfN(CONFIG_WARN_DBGLVL, "_PinchgloveContactInput: Warning, contact['%s'] is already in the list -- duplicating.\n", dti->instance);
			}
		}

		/* okay, now do the adding */
		posture_num = aux->num_postures;
		aux->num_postures++;
		aux->posturecode_left[posture_num] = left_contact_val;
		aux->posturecode_right[posture_num] = right_contact_val;
		aux->button_values[posture_num] = 0;
		aux->button_inputs[posture_num] = (vr2switch *)input;

		vrDbgPrintfN(PINCHG_DBGLVL, "_PinchgloveContactInput: "
			"assigned contact '%s' (%02x.%02x) to button %d (type %d)\n",
			dti->instance, left_contact_val, right_contact_val, posture_num, input->input_type);
	}

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


	/************************************************************/
	/***************** FreeVR Callback routines *****************/
	/************************************************************/

	/************************************************************/
	/*    Callbacks for controlling the PinchGlove features.    */
	/*                                                          */

/************************************************************/
static void _PinchgloveSystemPauseToggleCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

	devinfo->context->paused ^= 1;
	vrDbgPrintfN(SELFCTRL_DBGLVL, "Pinchglove Control: system_pause = %d.\n",
		devinfo->context->paused);
}

/************************************************************/
static void _PinchglovePrintContextStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintContext(stdout, devinfo->context, verbose);
}

/************************************************************/
static void _PinchglovePrintConfigStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintConfig(stdout, devinfo->context->config, verbose);
}

/************************************************************/
static void _PinchglovePrintInputStructCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

        vrFprintInput(stdout, devinfo->context->input, verbose);
}

/************************************************************/
static void _PinchglovePrintStructCallback(vrInputDevice *devinfo, int value)
{
	_PinchglovePrivateInfo	*aux = (_PinchglovePrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	_PinchglovePrintStruct(stdout, aux, verbose);
}

/************************************************************/
static void _PinchglovePrintHelpCallback(vrInputDevice *devinfo, int value)
{
        _PinchglovePrivateInfo  *aux = (_PinchglovePrivateInfo *)devinfo->aux_data;

        if (value == 0)
                return;

        _PinchglovePrintHelp(stdout, aux);
}




	/*************************************************************/
	/*   Callbacks for interfacing with the PinchGlove device.   */
	/*                                                           */

/**************************************************************************/
static void _PinchgloveCreateFunction(vrInputDevice *devinfo)
{
	/*** List of possible inputs ***/
static	vrInputFunction	_PinchgloveInputs[] = {
				{ "contact", VRINPUT_2WAY, _PinchgloveContactInput },
				{ NULL, VRINPUT_UNKNOWN, NULL } };
	/*** List of control functions ***/
static vrControlFunc _PinchgloveControlList[] = {
				/* overall system controls */
				{ "system_pause_toggle", _PinchgloveSystemPauseToggleCallback },

				/* informational output controls */
				{ "print_context", _PinchglovePrintContextStructCallback },
				{ "print_config", _PinchglovePrintConfigStructCallback },
				{ "print_input", _PinchglovePrintInputStructCallback },
				{ "print_struct", _PinchglovePrintStructCallback },
				{ "print_help", _PinchglovePrintHelpCallback },

		/** TODO: other callback control functions go here **/
				/* simulated 6-sensor selection controls */
				/* simulated 6-sensor manipulation controls */
				/* other controls */

				/* end of the list */
				{ NULL, NULL } };

	_PinchglovePrivateInfo	*aux = NULL;

	/******************************************/
	/* allocate and initialize auxiliary data */
	devinfo->aux_data = (void *)vrShmemAlloc0(sizeof(_PinchglovePrivateInfo));
	aux = (_PinchglovePrivateInfo *)devinfo->aux_data;
	aux->device = devinfo;
	_PinchgloveInitializeStruct(aux);

	/******************/
	/* handle options */

	/* set defaults */
	aux->port = vrShmemStrDup(DEFAULT_PORT);
	aux->baud_int = DEFAULT_BAUD;
	aux->baud_enum = vrSerialBaudIntToEnum(aux->baud_int);

	_PinchgloveParseArgs(aux, devinfo->args);

	/***************************************/
	/* create the inputs and self-controls */
	vrInputCreateDataContainers(devinfo, _PinchgloveInputs);
	vrInputCreateSelfControlContainers(devinfo, _PinchgloveInputs, _PinchgloveControlList);
	/* TODO: anything to do for NON self-controls?  Implement for Magellan first. */

	vrDbgPrintf("Done creating Pinchglove inputs for '%s'\n", devinfo->name);
	devinfo->created = 1;

	return;
}


/**************************************************************************/
static void _PinchgloveOpenFunction(vrInputDevice *devinfo)
{
	_PinchglovePrivateInfo	*aux = (_PinchglovePrivateInfo *)devinfo->aux_data;

	vrTrace("_PinchgloveOpenFunction", devinfo->name);

	/*******************/
	/* open the device */
	aux->fd = vrSerialOpen(aux->port, aux->baud_enum);
	if (aux->fd < 0) {
		aux->open = 0;
		vrErrPrintf("(%s::_PinchgloveOpenFunction::%d) error: "
			RED_TEXT "couldn't open serial port %s for %s\n" NORM_TEXT,
			__FILE__, __LINE__, aux->port, devinfo->name);
		sprintf(aux->version, "- unconnected pinchglove -");
	} else {
		aux->open = 1;
		_PinchgloveInitializeDevice(aux);
		if (aux->device->proc->end_proc == 1) {
			/* We've already been told to quit -- before we even got started */
			return;
		}
		if (aux->lg_connected == 0 && aux->rg_connected == 0) {
			vrErrPrintf("_PinchgloveOpenFunction: "
				RED_TEXT "Warning, no or improper gloves connected to '%s' PinchGlove (check if reversed).\n" NORM_TEXT, devinfo->name);
		} else if (aux->lg_connected == 0) {
			vrErrPrintf("_PinchgloveOpenFunction: "
				RED_TEXT "Warning, left pinchglove not or improperly connected to '%s' PinchGlove.\n" NORM_TEXT, devinfo->name);
		} else if (aux->rg_connected == 0) {
			vrErrPrintf("_PinchgloveOpenFunction: "
				RED_TEXT "Warning, right pinchglove not or improperly connected to '%s' PinchGlove.\n" NORM_TEXT, devinfo->name);
		}

		/* NOTE: for the Pinch-Glove, it is assumed that if we successfully open */
		/*   a connection that we will successfully initialize the device.       */
	}
	vrDbgPrintf("_PinchgloveOpenFunction(): Done opening PinchGlove input device '%s'.\n", devinfo->name);
	devinfo->operating = 1;

	return;
}


/**************************************************************************/
static void _PinchgloveCloseFunction(vrInputDevice *devinfo)
{
	_PinchglovePrivateInfo	*aux = (_PinchglovePrivateInfo *)devinfo->aux_data;

	if (aux != NULL) {
		vrSerialClose(aux->fd);
		vrShmemFree(aux);	/* aka devinfo->aux_data */
	}

	return;
}


/**************************************************************************/
static void _PinchgloveResetFunction(vrInputDevice *devinfo)
{
	/* TODO: reset code */
	return;
}


/**************************************************************************/
static void _PinchglovePollFunction(vrInputDevice *devinfo)
{
	if (devinfo->operating) {
		_PinchgloveGetData(devinfo);
	} else {
		/* TODO: try to open the device again */
	}

	return;
}


	/**********************************************************/
	/***************** FreeVR public routines *****************/
	/**********************************************************/


/**********************************************************/
void vrPinchgloveInitInfo(vrInputDevice *devinfo)
{
	devinfo->version = (char *)vrShmemStrDup("- PinchGlove version info not yet retrieved -");
	devinfo->Create = vrCallbackCreateNamed("Pinchglove:Create-Def", _PinchgloveCreateFunction, 1, devinfo);
	devinfo->Open = vrCallbackCreateNamed("Pinchglove:Open-Def", _PinchgloveOpenFunction, 1, devinfo);
	devinfo->Close = vrCallbackCreateNamed("Pinchglove:Close-Def", _PinchgloveCloseFunction, 1, devinfo);
	devinfo->Reset = vrCallbackCreateNamed("Pinchglove:Reset-Def", _PinchgloveResetFunction, 1, devinfo);
	devinfo->PollData = vrCallbackCreateNamed("Pinchglove:PollData-Def", _PinchglovePollFunction, 1, devinfo);
	devinfo->PrintAux = vrCallbackCreateNamed("Pinchglove:PrintAux-Def", _PinchglovePrintStruct, 0);

	vrDbgPrintfN(PINCHG_DBGLVL, "vrPinchgloveInitInfo: callbacks created.\n");
}

#endif /* } defined(FREEVR) */


	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/
	/*===========================================================*/


#ifdef TEST_APP /* { */

/*********************************************************************************/
/* A test program to communicate with a Fakespace PinchGlove and print some info */
/*********************************************************************************/
main(int argc, char *argv[])
{
	_PinchglovePrivateInfo	*pinch;
	int			count;


	/******************************/
	/* setup the device structure */
	pinch = (_PinchglovePrivateInfo *)malloc(sizeof(_PinchglovePrivateInfo));
	memset(pinch, 0, sizeof(_PinchglovePrivateInfo));
	_PinchgloveInitializeStruct(pinch);


	/*********************************************************/
	/* set default parameters based on environment variables */
	/* TODO: use of environment variables */
	pinch->port = getenv("PINCH_TTY");
	if (pinch->port == NULL)
		pinch->port = DEFAULT_PORT;	/* default, if no file given */
	pinch->baud_int = DEFAULT_BAUD;		/* default, if no baud given */
	pinch->baud_enum = vrSerialBaudIntToEnum(pinch->baud_int);


	/*****************************************************/
	/* adjust parameters based on command-line-arguments */
	/* TODO: parse CLAs for non-default serial port and baud */


	/**************************************************/
	/* open the serial port and initialize the device */
printf("About to open serial port '%s' at baud code %d\n", pinch->port, pinch->baud_enum);
	pinch->fd = vrSerialOpen(pinch->port, pinch->baud_enum);
printf("Result of serial port open is %d\n", pinch->fd);
	if (pinch->fd < 0) {
		fprintf(stderr, RED_TEXT "couldn't open serial port %s\n" NORM_TEXT, pinch->port);
		pinch->open = 0;
		sprintf(pinch->version, "- unconnected pinchglove -");
	} else {
printf("About to initialize device\n");
		_PinchgloveInitializeDevice(pinch);
printf("initialize device completed\n");
		if (pinch->lg_connected == 0 && pinch->rg_connected == 0) {
			fprintf(stderr, RED_TEXT "Warning, no or improper gloves connected (check if reversed).\n" NORM_TEXT);
		} else if (pinch->lg_connected == 0) {
			fprintf(stderr, RED_TEXT "Warning, left pinchglove not or improperly connected.\n" NORM_TEXT);
		} else if (pinch->rg_connected == 0) {
			fprintf(stderr, RED_TEXT "Warning, right pinchglove not or improperly connected.\n" NORM_TEXT);
		}
		pinch->open = 1;
	}

	_PinchglovePrintStruct(stdout, pinch, verbose);


	/**********************/
	/* display the output */
	/* (loop until the two thumbs are touched together) */
	while (!(pinch->left_contacts[count] == 0x10 && pinch->right_contacts[count] == 0x10)) {
		_PinchgloveReadInput(pinch);
		printf("%d contacts: ", pinch->num_contacts);
		for (count = 0; count < pinch->num_contacts; count++) {
			printf("%02x.%02x ", pinch->left_contacts[count], pinch->right_contacts[count]);
		}
		printf("                            ");
		printf("%c%s", (getenv("PG_DEBUG") ? '\n' : '\r'), "");
		fflush(stdout);
	}


	/*****************/
	/* close up shop */
	if (pinch != NULL) {
		vrSerialClose(pinch->fd);
		free(pinch);			/* aka devinfo->aux_data */
	}

	vrPrintf(BOLD_TEXT "\nPinchglove device closed\n" NORM_TEXT);
}

#endif /* } TEST_APP */

