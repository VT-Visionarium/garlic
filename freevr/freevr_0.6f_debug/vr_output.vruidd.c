/* ======================================================================
 *
 *  CCCCC          vr_output.vruidd.c
 * CC   CC         Author(s): Bill Sherman
 * CC              Created: August 3, 2010
 * CC   CC         Last Modified: June 9, 2014
 *  CCCCC
 *
 * Code file for FreeVR outputs to the Vrui VRDeviceDaemon.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/*************************************************************************

USAGE:
	There currently are no options for flexibly specifying the
	configuration options for the VruiDD output server.  At
	present it simply takes all the inputs available (except for
	the first button, which is the 'ESC' key), and puts them, in
	order, into the Vrui VRDeviceDaemon protocol.

	Controls are specified in the freevrrc file:
		control "<control option>" = "2switch(...)";

	Here are the available control options for FreeVR:
	  :-(	"print_help" -- print info on how to use the output device
	  :-(	"system_pause_toggle" -- toggle the system pause flag
	  :-(	"print_context" -- print the overall FreeVR context data structure (for debugging)
	  :-(	"print_config" -- print the overall FreeVR config data structure (for debugging)
	  :-(	"print_input" -- print the overall FreeVR input data structure (for debugging)
	  :-(	"print_struct" -- print the internal Shared Memory data structure (for debugging)
	  :-(	"print_values" -- print just the input values of the memory


	Here are the FreeVR configuration options for the generic Shared Memory Device:
		"port" - socket port listening for applications
			requesting data.  (8555 is the default)
		"protocol" -- version of the Vrui DD communications protocol to
			output data (default is 0 for Vrui 1.x)
	  :-?	"valscale" - scaling factor used to tune the valuator input range
	  :-?	"transscale"/"scale" - set the scaling factor for the location
			of the 6-sensor inputs.

NOTES:
	- The Vrui coordinate system uses a Z-up protocol, and defaults to
		units in inches.  FreeVR on the other hand uses a Y-up 
		coordinate system, and defaults to units as feet.
		BTW: Vrui has +X to the left and +Y into the screen.

	- The VRUI VRDeviceDaemon protocol for position trackers is for 13
		floating point (4-byte) values.  The first 3 are location,
		the next four are orientation (as a quaternion), and the
		remaining 6 are for velocities or something, but in practice
		are unused.  Therefore, this driver will send "0.0" values
		for the trailing 6 values.

	- There does not seem to be a mechanism in the VRUI VRDeviceDaemon
		protocol for times when the server is shutdown, and might
		want to warn the clients.

HISTORY:
	3 August, 2010 (Bill Sherman) -- began writing the initial version.
		Code was started as a copy of the "vr_output.shmemd.c"
		routines.  For now, I'll just output all my inputs through
		the Vrui DD protocol.  Configurability will come later.

	12 August, 2010 (Bill Sherman) -- completed the basics of the Vrui
		VRDeviceDaemon protocol, so now it cleanly disconnects and
		begins waiting for a new connection rather than exiting
		dramatically.  I also did a bit of cleanup in the code.

	12 December, 2012 (Bill Sherman) -- added ability to work with new
		Vrui DD protocols -- will compare with the client and adapt
		to the protocol version used by the client.  Also can now
		use the frame-of-reference configurations to adjust the
		output values (useful for Immersive ParaView).

	10 July, 2013 (Bill Sherman) -- Added the "protocol" option to specify
		which protocol should be sent.

	15 September 2013 (Bill Sherman)
		Changed the "0x%p" format to the improved "%#p" format.

	25 September 2013 (Bill Sherman)
		Fixed the communications problem when more than one message
		from the Vrui Client was received together.  (When that was
		happening, the second one would be ignored -- and to make it
		hard to debug, the packages arrived separately when going
		through "socketspy".)

		Now handle (minimally) Vrui protocol-2(as well as protocol-1).

	9 June 2014 (Bill Sherman)
		Added the "brief" output style to _VruiDDPrintStruct(), and as
		well as expanded the output of the "default"/"verbose" style.

	1 July 2014 (Bill Sherman)
		I added some notations as to the shift in coordinate systems
		that is done to go from FreeVR to Vrui -- and where it happens
		in the code.

TODO:
	- DONE: Need to watch for the STOPSTREAM_REQUEST, DEACTIVATE_REQUEST,
		and DISCONNECT_REQUEST messages (and deal with them).

	- add the ability to configure VruiDD virtual devices as part of
		the protocol-2 format.  Presently we just report that there
		are no virtual devices, which is a valid response of
		protocol-2, and from then on it will behave the same as
		protocol-1, but it might be nice to enable this feature
		of Vrui 3.x.

	- consider whether it is important to fix the bug where shifting to
		a new protocol will make that the new default protocol.

	- Do a better job (than crashing) when more inputs are requested
		(e.g. when 6 buttons are requested but config only has 3).

	- DONE: Figure out why the quaternions have to be adjusted.
		[07/01/14: they have to be adjusted to transition from the
		FreeVR Y-up coordinate system to the Vrui Z-up coordinate system.]

	- Handle the endianness of the data.

	- Need to make it configurable.

BUGS: (or potential bugs)
	- one the protocol shifts as a result of a mismatch with the client,
		then that becomes the new default protocol!

	- if two other messages arrived following a CONNECT_REQUEST, then
		the result could be the interpretation of those 4 byes as
		a protocol number.

*************************************************************************/
#include <stdio.h>		/* needed for sprintf() */
#include <string.h>		/* needed for strchr() */
#include <stdint.h>		/* needed for uint16_t & uint32_t types */
#include <sys/time.h>		/* needed for struct timeval */
#include <netinet/tcp.h>	/* needed for TCP_NODELAY and TCP_CORK */
#include <netinet/in.h>		/* needed for IPPROTO_TCP */

#include "vr_input.h"
#include "vr_input.opts.h"
#include "vr_socket.h"
#include "vr_debug.h"

	/***************************************************/
	/*** definitions for interfacing with the device ***/
	/***                                             ***/

#define	BUFSIZE		1024
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


/****************************************************************/
/*** auxiliary structure of the current data from the device. ***/
typedef struct {
		/* this is for printing extra info in _VruiDDPrintValues() */
		vrInputDevice	*devinfo;

		/* these are for interfacing with the client (usually the application) */
		int		fd_listen_socket;	/* socket to listen for connections requests (file_descriptor) */
		int		fd_receive_socket;	/* socket to communicate with client */
		int		dd_port;		/* port on server listening for application connections */
		int		open;			/* flag that VruiDD successfully open */
		int		vrui_link;		/* flag to indicate when a VRUI client is connected */
		int		streaming;		/* flag indicating whether we're to be streaming data now */
		int		protocol;		/* An integer indicating the Vrui VRDeviceDaemon protocol */

		/* these are for internal data parsing/constructing */
		int		packet_size;		/* the number of bytes in each packet reported by the Daemon */
		unsigned char	buf[BUFSIZE];		/* buffer for sending data to the Vrui Client */
		int		eobuf_pos;		/* position of the last active byte in the buffer */

		/* TODO: most of these fields were part of vr_input.shmemd.c, I suspect most can be deleted */

		/* these are pointers to the FreeVR inputs */
		int		num_buttons;
#if 1 /* saving these for a time when I might do output-mapping */
		vr2switch	**button_inputs;
		int		*button_values;		/* array of current values */
		int		*button_offsets;	/* array of offsets into the Vrui memory structure */
		int		*button_dummy;		/* array of flags whether button data coming from dummy values */
#endif

		int		num_valuators;
#if 1 /* saving these for a time when I might do output-mapping */
		vrValuator	**valuator_inputs;
		float		*valuator_values;	/* array of current values */
		int		*valuator_offsets;	/* array of offsets into the Vrui memory structure */
		int		*valuator_dummy;	/* array of flags whether valuator data coming from dummy values */
#endif

		int		num_6sensors;
#if 1 /* saving these for a time when I might do output-mapping */
		vr6sensor	**sensor6_inputs;
		vrMatrix	*sensor6_values;	/* array of current values */
		int		*sensor6_offsets;	/* array of offsets into the Vrui memory structure */
		int		*sensor6_dummy;		/* array of flags whether 6sensor data coming from dummy values */
#endif

#if 0 /* this part certainly is not required for an "output" device */
		/* these are for storing dummy values when too many inputs are requested */
		int		dummy_button_value;
		int		dummy_switch_value;
		float		dummy_valuator_value;
		vrMatrix	dummy_6sensor_value;
#endif
	} _VruiDDPrivateInfo;



	/*********************************************/
	/*** General NON public interface routines ***/
	/*********************************************/

/**************************************************************************/
static void _VruiDDInitializeStruct(_VruiDDPrivateInfo *aux)
{
	aux->fd_listen_socket = -1;		/* i.e. indicate that it's not yet initialized */
	aux->fd_receive_socket = -1;		/* i.e. indicate that it's not yet initialized */
	aux->dd_port = DEFAULT_PORT;		/* Set the default listening port */
	aux->vrui_link = 0;			/* No client is connected yet */
	aux->streaming = 0;			/* Not presently streaming data */
	aux->protocol = 0;			/* Vrui protocol 0 is from Vrui 1.x, protocol 1 is Vrui 2.x */
}


/**************************************************************************/
static void _VruiDDPrintStruct(FILE *file, _VruiDDPrivateInfo *aux, vrPrintStyle style)
{
	int	count;

	switch (style) {
		case brief:

		vrFprintf(file, "VRUI-output: connection=(%d:%d:%d), open=%d, link=%d, streaming=%d, protocol=%d\n",
			aux->fd_listen_socket, aux->fd_receive_socket, aux->dd_port,
			aux->open, aux->vrui_link, aux->streaming, aux->protocol);
#if 0
	char msg[1024];
	sprintf(msg, "VRUI-output: connection=(%d:%d:%d), open=%d, link=%d, streaming=%d, protocol=%d\n",
		aux->fd_listen_socket, aux->fd_receive_socket, aux->dd_port,
		aux->open, aux->vrui_link, aux->streaming, aux->protocol);
	vrSystemSetStatusDescription(msg);
#endif
		break;

		case verbose:
		default:
		vrFprintf(file, "VRUI output device internal structure (%#p):\n", aux);
		vrFprintf(file, "\r\tvruidd open = %d\n", aux->open);
		vrFprintf(file, "\r\tvruidd listening socket file descriptor = %d\n", aux->fd_listen_socket);

		vrFprintf(file, "\r\t%d buttons:\n", aux->num_buttons);
		vrFprintf(file, "\r\tbutton_inputs = %#p, button_values = %#p, button_offsets = %#p\n",
			aux->button_inputs, aux->button_values, aux->button_offsets);
		for (count = 0; count < aux->num_buttons; count++) {
			vrFprintf(file, "\r\t\tbutton %d: value = %d, offset = %d, input = %#p (type = %d)\n",
				count,
				aux->button_values[count],
				aux->button_offsets[count],
				aux->button_inputs[count],
				aux->button_inputs[count]->input_type);
		}

		vrFprintf(file, "\r\t%d valuators:\n", aux->num_valuators);
		vrFprintf(file, "\r\tvaluator_inputs = %#p, valuator_values = %#p, valuator_offsets = %#p\n",
			aux->valuator_inputs, aux->valuator_values, aux->valuator_offsets);
		for (count = 0; count < aux->num_valuators; count++) {
			vrFprintf(file, "\r\t\tvaluator %d: value = %.2f, offset = %d, input = %#p (type = %d)\n",
				count,
				aux->valuator_values[count],
				aux->valuator_offsets[count],
				aux->valuator_inputs[count],
				aux->valuator_inputs[count]->input_type);
		}

		vrFprintf(file, "\r\t%d 6-sensors:\n", aux->num_6sensors);
		vrFprintf(file, "\r\t6sensor_inputs = %#p, 6sensor_values = %#p, 6sensor_offsets = %#p\n",
			aux->sensor6_inputs, aux->sensor6_values, aux->sensor6_offsets);
		for (count = 0; count < aux->num_6sensors; count++) {
			vrFprintf(file, "\r\t\t6-sensor %d: value = [... (TODO!)], offset = %d, input = %#p (type = %d)\n",
				count,
			/* TODO: put matrix values here  -- perhaps convert to quat/trans */
				aux->sensor6_offsets[count],
				aux->sensor6_inputs[count],
				aux->sensor6_inputs[count]->input_type);
		}
	}
}


/**************************************************************************/
static void _VruiDDPrintHelp(FILE *file, _VruiDDPrivateInfo *aux)
{
	vrFprintf(file, BOLD_TEXT "Sorry, VruiDD -- print_help control not yet implemented.\n" NORM_TEXT);
}


/**************************************************************************/
static void _VruiDDPrintValues(FILE *file, _VruiDDPrivateInfo *aux)
{
	int	count;

	vrFprintf(file, "Vrui DD output device values:\n");

	vrFprintf(file, "\r\t%d buttons\n", aux->num_buttons);
	for (count = 0; count < aux->num_buttons; count++) {
		if (aux->button_inputs[count]->input_type == VRINPUT_BINARY) {
			vrFprintf(file, "\r\t\tbutton %d ('%s') [offset = %d]: = %d\n",
				count,
				aux->devinfo->switch2[count].my_object->name,
				aux->button_offsets[count],
				aux->button_values[count]);
		} else {
			vrFprintf(file, "\r\t\tbutton %d ('%s') [offset = %d]: not reported as VRINPUT_BINARY (%d instead)\n",
				count,
				aux->devinfo->switch2[count].my_object->name,
				aux->button_offsets[count],
				aux->button_inputs[count]->input_type);
		}
	}

	vrFprintf(file, "\r\t%d valuators\n", aux->num_valuators);
	for (count = 0; count < aux->num_valuators; count++) {
		if (aux->valuator_inputs[count]->input_type == VRINPUT_VALUATOR) {
			vrFprintf(file, "\r\t\tvaluator %d ('%s') [offset = %d]: = %.2f\n",
				count,
				aux->devinfo->valuator[count].my_object->name,
				aux->valuator_offsets[count],
				aux->valuator_values[count]);
		} else {
			vrFprintf(file, "\r\t\tvaluator %d ('%s') [offset = %d]: not reported as VRINPUT_VALUATOR (%d instead)\n",
				count,
				aux->devinfo->switch2[count].my_object->name,
				aux->valuator_offsets[count],
				aux->valuator_inputs[count]->input_type);
		}
	}

	vrFprintf(file, "\r\t%d 6-sensors\n", aux->num_6sensors);
	for (count = 0; count < aux->num_6sensors; count++) {
		if (aux->sensor6_inputs[count]->input_type == VRINPUT_6SENSOR) {
			/* TODO: print the matrix */
			vrFprintf(file, "\r\t\t6sensor %d ('%s') [offset = %d]: = ...\n",
				count,
				aux->devinfo->sensor6[count].my_object->name,
				aux->sensor6_offsets[count]);
		} else {
			vrFprintf(file, "\r\t\t6sensor %d ('%s') [offset = %d]: not reported as VRINPUT_6SENSOR (%d instead)\n",
				count,
				aux->devinfo->switch2[count].my_object->name,
				aux->sensor6_offsets[count],
				aux->sensor6_inputs[count]->input_type);
		}
	}
}


	/************************************/
	/***  FreeVR NON public routines  ***/
	/************************************/


/***************************************************************/
static void _VruiDDParseArgs(_VruiDDPrivateInfo *aux, char *args)
{
	/* In the rare case of no arguments, just return */
	if (args == NULL)
		return;

	/****************************************/
	/** Argument format: "port" "=" number **/
	/****************************************/
	vrArgParseInteger(args, "port", &(aux->dd_port));

	/********************************************/
	/** Argument format: "protocol" "=" number **/
	/********************************************/
	vrArgParseInteger(args, "protocol", &(aux->protocol));


	/* it would be reasonable to add some arguments for setting the dummy values here */
}


/***************************************************************/
static void _VruiDDPutData(vrInputDevice *devinfo)
{
	_VruiDDPrivateInfo	*aux = (_VruiDDPrivateInfo *)devinfo->aux_data;
	int			count;
	int			offset;
	int			return_val;

	/* Set the outgoing message type -- PACKET_REPLY */
	aux->buf[0] = 0x06;
	aux->buf[1] = 0x00;

	/*************/
	/** buttons **/

	/* skip the tracker space */
	offset = 2 + aux->num_6sensors * 13 * 4;	/* each 6sensor has 13 4-byte values (also skip the 2 byte packet message) */

	/* NOTE: we skip button 0, which is the escape key -- Note further that "aux->num_buttons" already is reduced by 1 to account for the lack of ESC key */
	for (count = 0; count < aux->num_buttons; count++) {
		aux->buf[offset+count] = vrGet2switchValue(count+1);	/* TODO: probably should use a "Direct()" form of this function so as not to mess with any "Delta()" forms -- though this would be somewhat pedantic, as we really shouldn't be doing output stuff when running a real application -- but I like to be pedantic, it can save from future bugs */
	}

	/***************/
	/** valuators **/

	for (count = 0; count < aux->num_valuators; count++) {
		offset = 2 + (aux->num_6sensors * 13 * 4) + (aux->num_buttons) + (count * 4);

		*((float *)&(aux->buf[offset])) = vrGetValuatorValue(count);	/* TODO: this assume a 4-byte float, and we're not taking endianness into account */
	}

	/***************/
	/** 6-sensors **/

	for (count = 0; count < aux->num_6sensors; count++) {
		vrMatrix	sensor_mat;			/* a transitional place to get the sensor data */
		vrQuat		sensor_quat;			/* a place to put the quaternion version of the rotation */

		offset = 2 + (count * 13 * 4);

		/* extract the actual matrix for this sensor */
		vrMatrixGet6sensorValues(&sensor_mat, count);

		/* modify the matrix based on configuration parameters     */
		/* NOTE: typically this would be the identity operation,   */
		/*   but for some special cases (namely ParaView), it's    */
		/*   beneficial to be able to move the coordinate system.  */
		/* NOTE: this operation uses the "t2rw_xform" field,       */
		/*   but when specified in a configuration file, the       */
		/*   "frame-of-reference" synonym will generally be used   */
		/*   instead (e.g. "for_translate").                       */
#if 0 /* set to "1" to hardcode */
		/* TODO: for the moment (while developing Immersive ParaView),*/
		/*    I'm going to just hard-code the transformation.         */
		VRMAT_ROWCOL(&sensor_mat, VR_Y, VR_W) -= 5.0;
#else
		vrMatrixPreMult(&sensor_mat, devinfo->t2rw_xform);
#endif

		/* extract the rotation from the new matrix as a quaternion */
		vrQuatSetFromMatrix(&sensor_quat, &sensor_mat);

		/* TODO: these assume a 4-byte float, and we're not taking endianness into account */
		*((float *)&(aux->buf[offset +  0])) =  VRMAT_ROWCOL(&sensor_mat, VR_X, VR_W) * 12.0;	/* Vrui uses inches by default */
		*((float *)&(aux->buf[offset +  4])) = -VRMAT_ROWCOL(&sensor_mat, VR_Z, VR_W) * 12.0;
		*((float *)&(aux->buf[offset +  8])) =  VRMAT_ROWCOL(&sensor_mat, VR_Y, VR_W) * 12.0;
		*((float *)&(aux->buf[offset + 12])) = -sensor_quat.v[VR_X];			/* NOTE: swapping VR_Z and VR_Y, along with the sign  */
		*((float *)&(aux->buf[offset + 16])) =  sensor_quat.v[VR_Z];			/*   changes effectively shifts the coordinate system */
		*((float *)&(aux->buf[offset + 20])) = -sensor_quat.v[VR_Y];			/*   from Y-up (FreeVR) to Z-up (Vrui).               */
		*((float *)&(aux->buf[offset + 24])) =  sensor_quat.v[VR_W];
		/* NOTE: the other 6 values are set to 0.0 */
		*((float *)&(aux->buf[offset + 28])) = 0.0;
		*((float *)&(aux->buf[offset + 32])) = 0.0;
		*((float *)&(aux->buf[offset + 36])) = 0.0;
		*((float *)&(aux->buf[offset + 40])) = 0.0;
		*((float *)&(aux->buf[offset + 44])) = 0.0;
		*((float *)&(aux->buf[offset + 48])) = 0.0;
	}

	/*******************/
	/** Send the data **/
	vrTrace("_VruiDDPutData():", BOLD_TEXT "About to send the data to client." NORM_TEXT);
	return_val = write(aux->fd_receive_socket, aux->buf, aux->packet_size);	/* TODO: warning, no effort yet being made to deal with endianness of the order */
	if (return_val < 0) {
		perror("YO");
	} else {
		return_val = fsync(aux->fd_receive_socket); //-- this should work, but maybe it doesn't
//printf("fsync() returned %d\n", return_val);
//perror("fsync");
#if !defined(__APPLE__) && !defined(__CYGWIN__)	/* Darwin & Cygwin don't have the TCP_CORK option */
		int flag=0;
		setsockopt(aux->fd_receive_socket,IPPROTO_TCP,TCP_CORK,&flag,sizeof(int));
		flag=1;
		setsockopt(aux->fd_receive_socket,IPPROTO_TCP,TCP_CORK,&flag,sizeof(int));
#endif
	}
	vrTrace("_VruiDDPutData():", BOLD_TEXT "Sent the data to client." NORM_TEXT);
}


#if 0 /* I don't think I'm going to want this stuff for the "output" function, but I'll leave the code in place for now [04/11/2007] { */
	/* [09/25/13: actually, it might now come in handy for specifying the virtual devices */
	/*   of protocol-2.  Not sure yet, we'll see when the time comes to implement them.]  */

	/*****************************************************************/
	/*   Function(s) for parsing Shared Memory "input" declarations. */
	/*                                                               */
	/*  These _VruiDD<type>Input() functions are called during the   */
	/*  CREATE phase of the input interface.                         */

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

/**************************************************************************/
/* type might be "char", "int", or "long" (and in the future "float" or "double") */
/*    parsing:                                                                    */
/*       input "<name>" = "2switch(<type>[<offset>])";                            */
/*    thus:                                                                       */
/*       the device string is unused.                                             */
/*       the type string is <type> (eg. "char", "int", etc)                       */
/*       the instance string is <offset> (eg. 0, 4, 6)                            */
/**************************************************************************/
static vrInputMatch _VruiDDButtonInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
        _VruiDDPrivateInfo	*aux = (_VruiDDPrivateInfo *)devinfo->aux_data;
	int			button_num;
	int			offset;
	_MemoryType		memtype;

	/* determine current button in our private arrays of button data */
	button_num = aux->num_buttons;
	aux->num_buttons++;

	memtype = _MemoryValue(dti->type);
	offset = vrAtoI(dti->instance);

	vrDbgPrintfN(INPUT_DBGLVL, "_VruiDDButtonInput(): Assigned button event of button %d, to type '%s'(%d) with offset %d\n",
		button_num, _MemoryTypeName(memtype), (int)memtype, offset);

	if (memtype == MEM_UNKNOWN) {
		vrErrPrintf("_VruiDDValuatorInput: " RED_TEXT "Warning, type['%s'] did not match any known type\n" NORM_TEXT, dti->type);
		return VRINPUT_NOMATCH;	/* unacceptable input declaration match, perhaps keep searching */
	}

#if 0 /* NOTE: this check isn't possible here because we haven't connected with the memory yet to know the size */
	if (offset 'out of bounds') {
		...
		return VRINPUT_NOMATCH;	/* unacceptable input declaration match, perhaps keep searching */
	}
#endif

	aux->button_inputs[button_num] = (vr2switch *)input;
	aux->button_values[button_num] = 0;
	aux->button_dummy[button_num] = 0;
	aux->button_offsets[button_num] = offset;

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}


/**************************************************************************/
static vrInputMatch _VruiDDValuatorInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
        _VruiDDPrivateInfo	*aux = (_VruiDDPrivateInfo *)devinfo->aux_data;
	int			valuator_num;
	int			offset;
	_MemoryType		memtype;

	/* determine current valuator in our private arrays of valuator data */
	valuator_num = aux->num_valuators;
	aux->num_valuators++;

	memtype = _MemoryValue(dti->type);
	offset = vrAtoI(dti->instance);

	vrDbgPrintfN(INPUT_DBGLVL, "_VruiDDValuatorInput(): Assigned button event of button %d, to type '%s'(%d) with offset %d\n",
		valuator_num, _MemoryTypeName(memtype), (int)memtype, offset);

	if (memtype == MEM_UNKNOWN) {
		vrErrPrintf("_VruiDDValuatorInput: " RED_TEXT "Warning, type['%s'] did not match any known type\n" NORM_TEXT, dti->type);
		return VRINPUT_NOMATCH;	/* unacceptable input declaration match, perhaps keep searching */
	}

#if 0 /* NOTE: this check isn't possible here because we haven't connected with the memory yet to know the size */
	if (offset 'out of bounds') {
		...
		return VRINPUT_NOMATCH;	/* unacceptable input declaration match, perhaps keep searching */
	}
#endif

	aux->valuator_inputs[valuator_num] = (vrValuator *)input;
	aux->valuator_values[valuator_num] = 0;
	aux->valuator_dummy[valuator_num] = 0.0;
	aux->valuator_offsets[valuator_num] = offset;

	return VRINPUT_MATCH_ABLE;		/* input declaration match */
}


/**************************************************************************/
static vrInputMatch _VruiDD6sensorInput(vrInputDevice *devinfo, vrGenericInput *input, vrInputDTI *dti)
{
        _VruiDDPrivateInfo	*aux = (_VruiDDPrivateInfo *)devinfo->aux_data;
	int			sensor6_num;
	int			offset;
	_MemoryType		memtype;

	/* determine current 6-sensor in our private arrays of 6-sensor data */
	sensor6_num = aux->num_6sensors;
	aux->num_6sensors++;

	memtype = _MemoryValue(dti->type);
	offset = vrAtoI(dti->instance);

	vrDbgPrintfN(INPUT_DBGLVL, "_VruiDD6sensorInput(): Assigned 6-sensor event of 6-sensor %d, to type '%s'(%d) with offset %d\n",
		sensor6_num, _MemoryTypeName(memtype), (int)memtype, offset);

	if (memtype == MEM_UNKNOWN) {
		vrErrPrintf("_VruiDDValuatorInput: " RED_TEXT "Warning, type['%s'] did not match any known type\n" NORM_TEXT, dti->type);
		return VRINPUT_NOMATCH;	/* unacceptable input declaration match, perhaps keep searching */
	}

#if 0 /* NOTE: this check isn't possible here because we haven't connected with the memory yet to know the size */
	if (offset 'out of bounds') {
		...
		return VRINPUT_NOMATCH;	/* unacceptable input declaration match, perhaps keep searching */
	}
#endif

	aux->sensor6_inputs[sensor6_num] = (vr6sensor *)input;
	vrMatrixSetIdentity(&aux->sensor6_values[sensor6_num]);
	aux->sensor6_dummy[sensor6_num] = 0;
	aux->sensor6_offsets[sensor6_num] = offset;

	vrAssign6sensorR2Exform(aux->sensor6_inputs[sensor6_num], strchr(dti->instance, ','));

	return VRINPUT_MATCH_ABLE;	/* input declaration match */
}

#endif /* } */


	/************************************************************/
	/***************** FreeVR Callback routines *****************/
	/************************************************************/

	/************************************************************/
	/*    Callbacks for controlling the device's features.      */
	/*                                                          */

/************************************************************/
static void _VruiDDSystemPauseToggleCallback(vrInputDevice *devinfo, int value)
{
        if (value == 0)
                return;

	devinfo->context->paused ^= 1;
	vrDbgPrintfN(INPUT_DBGLVL, "VruiDD Control: system_pause = %d.\n",
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
	_VruiDDPrivateInfo  *aux = (_VruiDDPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	_VruiDDPrintStruct(stdout, aux, verbose);
}

/************************************************************/
static void _VruiDDPrintHelpCallback(vrInputDevice *devinfo, int value)
{
        _VruiDDPrivateInfo  *aux = (_VruiDDPrivateInfo *)devinfo->aux_data;

        if (value == 0)
                return;

        _VruiDDPrintHelp(stdout, aux);
}

/************************************************************/
static void _VruiDDPrintValuesCallback(vrInputDevice *devinfo, int value)
{
	_VruiDDPrivateInfo  *aux = (_VruiDDPrivateInfo *)devinfo->aux_data;

	if (value == 0)
		return;

	_VruiDDPrintValues(stdout, aux);
}




	/*************************************************/
	/*   Callbacks for interfacing with the device.  */
	/*                                               */

/**********************************************************/
static void _VruiDDCreateFunction(vrInputDevice *devinfo)
{
#if 0 /* There's a chance I'll want to control the outputs similar to the inputs, so I'll leave this here for now { */
	/*** List of possible inputs ***/
static	vrInputFunction	_VruiDDInputs[] = {
			/* TODO: ideally, we should be able to catch all 2-switches with: { NULL, VRINPUT_BINARY, _VruiDDButtonInput }, */
				{ "char", VRINPUT_BINARY, _VruiDDButtonInput },
				{ "int",  VRINPUT_BINARY, _VruiDDButtonInput },
				{ "long", VRINPUT_BINARY, _VruiDDButtonInput },
			/* TODO: a "float/double" binary is also conceivable, perhaps with an epsilon */
				{ "float",  VRINPUT_VALUATOR, _VruiDDValuatorInput },
				{ "double", VRINPUT_VALUATOR, _VruiDDValuatorInput },
			/* TODO: valuators based on integer values are also conceivable */
				{ "vreuler",       VRINPUT_6SENSOR, _VruiDD6sensorInput },
				{ "vrmatrix",      VRINPUT_6SENSOR, _VruiDD6sensorInput },
				{ "vreulerfloat",  VRINPUT_6SENSOR, _VruiDD6sensorInput },
				{ "vrmatrixdouble",VRINPUT_6SENSOR, _VruiDD6sensorInput },
				{ "vreulerdouble", VRINPUT_6SENSOR, _VruiDD6sensorInput },
				{ "vrmatrixfloat", VRINPUT_6SENSOR, _VruiDD6sensorInput },
			/* TODO: I'd rather do without these, but needed to get visbox working ASAP */
				{ "visboxeuler",   VRINPUT_6SENSOR, _VruiDD6sensorInput },
				{ "visboxtrans",   VRINPUT_6SENSOR, _VruiDD6sensorInput },
				{ NULL, VRINPUT_UNKNOWN, NULL } };
#endif /* } */
	/*** List of control functions ***/
static	vrControlFunc	_VruiDDControlList[] = {
				/* overall system controls */
				{ "system_pause_toggle", _VruiDDSystemPauseToggleCallback },

				/* informational output controls */
				{ "print_context", _VruiDDPrintContextStructCallback },
				{ "print_config",  _VruiDDPrintConfigStructCallback },
				{ "print_input",   _VruiDDPrintInputStructCallback },
				{ "print_struct",  _VruiDDPrintStructCallback },
				{ "print_help",    _VruiDDPrintHelpCallback },
				{ "print_values",  _VruiDDPrintValuesCallback },

		/** TODO: other callback control functions go here **/
				/* simulated 6-sensor selection controls */
				/* simulated 6-sensor manipulation controls */
				/* other controls */

				/* end of the list */
				{ NULL, NULL } };

	_VruiDDPrivateInfo	*aux = NULL;

	/******************************************/
	/* allocate and initialize auxiliary data */
	devinfo->aux_data = (void *)vrShmemAlloc0(sizeof(_VruiDDPrivateInfo));
	aux = (_VruiDDPrivateInfo *)devinfo->aux_data;
	_VruiDDInitializeStruct(aux);
	aux->devinfo = devinfo;		/* [1/31/00] -- we don't normally do this, but are experimenting */

	/******************/
	/* handle options */
	_VruiDDParseArgs(aux, devinfo->args);


	/* ?? Here is some hard-coding of what would typically be done in most setups. */
	/*   I.e. output all the buttons (except button 0), all the */
	/*   valuators, and all the 6-sensors.                      */
	/* TODO: these values don't seem to get filled in at this point -- I was */
	/*   thinking that since this "input" device would be last in the list,  */
	/*   that the values would be known by the time it is initialized, but I */
	/*   guess that's not the case, so I'm going to set the value at the top */
	/*   of the processing routine _VruiDDPutData().                         */
	aux->num_buttons = devinfo->context->input->num_2ways;
	aux->num_valuators = devinfo->context->input->num_valuators;
	aux->num_6sensors = devinfo->context->input->num_6sensors;
	vrDbgPrintfN(VRUIDD_DBGLVL, "_VruiDDCreateFunction(): presently set values for: num_buttons = %d, num_valuators = %d, num_6sensors = %d.\n", aux->num_buttons, aux->num_valuators, aux->num_6sensors);

	vrDbgPrintf("_VruiDDCreateFunction(): Done creating VruiDD outputs for '%s'\n", devinfo->name);
	devinfo->created = 1;

vrPrintf("****** bottom of _VruiDDCreateFunction() ******\n");

	return;
}


/**********************************************************/
static void _VruiDDOpenFunction(vrInputDevice *devinfo)
{
	char			host_machine[1024];
	int			host_port;
	_VruiDDPrivateInfo	*aux = (_VruiDDPrivateInfo *)devinfo->aux_data;

	/****************************************/
	/* open the device (a listening socket) */
	gethostname(host_machine, 1024);
	host_port = aux->dd_port;
	vrTrace("_VruiDDOpenFunction():", BOLD_TEXT "Creating the listen socket." NORM_TEXT);
	aux->fd_listen_socket = vrSocketCreateListen(&host_port, 0);

_VruiDDPrintStruct(stdout, aux, brief);
	if (aux->fd_listen_socket < 0) {
		vrDbgPrintfN(AALWAYS_DBGLVL, "_VruiDDOpenFunction(): Unable to open socket %d.\n", aux->dd_port);
		sprintf(devinfo->version, "- unconnected vruidd -");
		aux->open = 0;
		vrTrace("_VruiDDOpenFunction():", BOLD_TEXT "Unable to create listen socket." NORM_TEXT);
	} else {
		vrMsgPrintf("_VruiDDOpenFunction: Connect to VruiDD port %s:%d\n", host_machine, host_port);
		aux->open = 1;
		vrTrace("_VruiDDOpenFunction():", BOLD_TEXT "Listen socket created." NORM_TEXT);
	}

	vrDbgPrintf("_VruiDDOpenFunction(): Done opening VruiDD for output device '%s'\n", devinfo->name);
_VruiDDPrintStruct(stdout, aux, brief);
	devinfo->operating = 1;


	/*******************************************/
	/* Print output at appropriate debug level */
	if (vrDbgDo(SHMEMD_DBGLVL)) {
		_VruiDDPrintValues(stdout, aux);
	}

_VruiDDPrintStruct(stdout, aux, brief);
	return;
}


/**********************************************************/
static void _VruiDDCloseFunction(vrInputDevice *devinfo)
{
	_VruiDDPrivateInfo	*aux = (_VruiDDPrivateInfo *)devinfo->aux_data;

	vrDbgPrintf("Closing VruiDD sockets %d:%d\n", aux->fd_listen_socket, aux->fd_receive_socket);
	if (aux->fd_listen_socket > 0)
		vrSocketClose(aux->fd_listen_socket);
	if (aux->fd_receive_socket > 0)
		vrSocketClose(aux->fd_receive_socket);
	return;
}


/**********************************************************/
static void _VruiDDResetFunction(vrInputDevice *devinfo)
{
	/* TODO: a vruidd reset should verify that the vruidd is sending the */
	/*   same amount of data, and if not, update the vrInputDevice.  */

	return;
}


/**********************************************************/
static void _VruiDDPollFunction(vrInputDevice *devinfo)
{
	_VruiDDPrivateInfo	*aux = (_VruiDDPrivateInfo *)devinfo->aux_data;
	char			inbuf[512];	/* buffer for data received from the Vrui client */
	uint16_t		message;	/* VRUI VRDeviceDaemon messages are 2 byte sequences */
	uint32_t		value;		/* VRUI 4-byte value */
	int			bytes_read = 0;	/* number of bytes read from socket thus far */
	int			read_result;	/* return value of the call to "read()" */
	int			return_value;	/* return value for other calls -- eg. fsync() */
	struct timeval		no_block = { 0, 0 };	/* set the time values for the no-blocking option */
	fd_set			fds;

	vrTrace("_VruiDDPollFunction():", BOLD_TEXT "at top." NORM_TEXT);
	/* TOOD: test for devinfo->operating */
#if 0
vrDbgPrintfN(VRUIDD_DBGLVL, "polling: operating = %d, vrui_link = %d\n", devinfo->operating, aux->vrui_link);
#endif

#if 0
_VruiDDPrintStruct(stdout, aux, brief);
#endif
	/* First check for an existing connection, and if not, check for a new connection */
	if (!aux->vrui_link) {
		/* not connected means listen for a connection */
		if (aux->fd_receive_socket < 0 && aux->fd_listen_socket >= 0) {
			/* fd_receive_socket will be -1 when no one is connected */
			aux->fd_receive_socket = vrSocketAnswer(aux->fd_listen_socket);
			if (aux->fd_receive_socket > 0) {
				vrDbgPrintfN(AALWAYS_DBGLVL, "FreeVR: VruiDD connection has been established with client.\n");
int flag = 1;
setsockopt(aux->fd_listen_socket,IPPROTO_TCP,TCP_NODELAY,&flag,sizeof(int));
			}
		}
	}

	if (aux->fd_receive_socket > 0) {
		/* if we have a connection, see whether there is a message (or disconnect) from the client */
		FD_ZERO(&fds);
		FD_SET(aux->fd_receive_socket, &fds);
		if (select(aux->fd_receive_socket + 1, &fds, NULL, NULL, &no_block)) {

			/***************************************/
			/* Read data from the Vrui VRDD client */

			bytes_read = 0;
			while ((bytes_read < 2) && (aux->fd_receive_socket > 0)) {
				read_result = (ssize_t)read(aux->fd_receive_socket, inbuf, sizeof(inbuf));
				if (read_result == 0) {
					/* this occurs when the other side has disconnected */
					aux->vrui_link = 0;		/* indicate we're not communicating with a Vrui client */
					aux->streaming = 0;		/* set streaming to default off-state */
					aux->fd_receive_socket = -1;	/* indicate that there's no one on the other side of the socket */
					vrDbgPrintfN(AALWAYS_DBGLVL, "FreeVR: VruiDD client has disconnected.\n", bytes_read);
					vrTrace("_VruiDDPollFunction():", BOLD_TEXT "client disconnected." NORM_TEXT);
				} else if (read_result > 0) {
					bytes_read += read_result;
				} 
#ifdef COMM_DEBUG
				vrDbgPrintf("_VruiDDPollFunction(): %d bytes read thus far.\n", bytes_read);
#endif
			}

			/**********************************************************/
			/* handle the VRUI VRDeviceDaemon acknowledgment protocol */

			while (bytes_read >= 2) {

				/* verify expected response received (assuming LITTLE_ENDIAN for now) */
				message = (inbuf[0] + (inbuf[1] << 8));
				memmove(inbuf, &inbuf[2], bytes_read-2);	bytes_read -= 2;	/* shift the data by one message packet */
				switch (message) {
				default:
					vrDbgPrintf("_VruiDDPollFunction(): Got an unexpected message %d.\n", message);
					break;

				case CONNECT_REQUEST:	/* Request to connect to server (0x00 0x00) */
					vrDbgPrintf("_VruiDDPollFunction(): Got a CONNECT_REQUEST packet, will now respond with a CONNECT_REPLY.\n");
#if 0
vrPrintf("CONNECT_REQUEST: default protocol = %d\n", aux->protocol);
					/**********************************************************************/
					/* attempt to determine whether the client is using Vrui protocol > 0 */
					if (aux->protocol == 0) {
						/* For protocol 0, the Vrui client should have only sent 2 bytes. */
						/*   If we got 6 bytes (4 extra), then it probably is a newer     */
						/*   protocol, which sent a protocol number.                      */
						if (bytes_read == 4) {
							uint32_t	client_protocol = 0;
							((unsigned char *)(&client_protocol))[0] = inbuf[0];
							((unsigned char *)(&client_protocol))[1] = inbuf[1];
							((unsigned char *)(&client_protocol))[2] = inbuf[2];
							((unsigned char *)(&client_protocol))[3] = inbuf[3];
							//memmove(inbuf, &inbuf[4], bytes_read-4);	bytes_read -= 4;	/* shift the data by one integer packet */
							vrDbgPrintfN(AALWAYS_DBGLVL, "_VruiDDPollFunction(): Vrui application seems to be using protocol %d, but expecting protocol %d -- shifting to new protocol.\n", client_protocol, aux->protocol);
							aux->protocol = client_protocol;
						}
					}
#endif

					/****************************/
					/* Handle protocol exchange */
 					/* (only exchange for protocol==0 when the client sends a protocol value) */
					if (aux->protocol > 0 || (aux->protocol == 0 && bytes_read >= 4)) {
						uint32_t	client_protocol = 0;

						/* TODO: properly handle endianness */
						((unsigned char *)(&client_protocol))[0] = inbuf[0];
						((unsigned char *)(&client_protocol))[1] = inbuf[1];
						((unsigned char *)(&client_protocol))[2] = inbuf[2];
						((unsigned char *)(&client_protocol))[3] = inbuf[3];
						memmove(inbuf, &inbuf[4], bytes_read-4);	bytes_read -= 4;	/* shift the data by one integer packet */

						if (aux->protocol != client_protocol) {
							vrDbgPrintfN(AALWAYS_DBGLVL, "_VruiDDPollFunction(): Vrui protocol mismatch with the application: serving %d, but client reporting %d -- shifting to client protocol.\n", aux->protocol, client_protocol);
							aux->protocol = client_protocol;
						}
					}

					/************************************************/
					/* send the opening reply message to the client */

					/* TODO: I'd like these numbers to be determined in the _VruiDDCreateFunction() */
					/*   function, but that doesn't seem to be working like I expected.  It's not a */
					/*   big time sink if it's done here though.                                    */
					aux->num_buttons = devinfo->context->input->num_2ways - 1;	/* NOTE: 1 is subtracted because we won't send the Esc key */
					aux->num_valuators = devinfo->context->input->num_valuators;
					aux->num_6sensors = devinfo->context->input->num_6sensors;

					aux->packet_size = sizeof(uint16_t) +				/* message code */
							(sizeof(float)*13 * aux->num_6sensors) +	/* A Vrui TrackerState struct is thirteen floats */
							(sizeof(uint8_t) * aux->num_buttons) +		/* A Vrui button is one uint8_t (aka "bool" in C++) */
							(sizeof(float) * aux->num_valuators);		/* A Vrui valuator is one float */
					vrDbgPrintfN(VRUIDD_DBGLVL, "_VruiDDPollFunction(): Daemon packet size is %d bytes (num_buttons = %d, valuators = %d, position trackers = %d).\n", aux->packet_size, aux->num_buttons, aux->num_valuators, aux->num_6sensors);

					switch (aux->protocol) {
					case 0:
					default:

						message = CONNECT_REPLY;		/* Positive connect reply with server layout (0x01 0x00) */
						aux->buf[ 0] = 0x01;
						aux->buf[ 1] = 0x00;
						aux->buf[ 2] = aux->num_6sensors;	/* TODO: fill the correct values for all bytes (but this will work when there's less than 256 of any type. */
						aux->buf[ 3] = 0x00;
						aux->buf[ 4] = 0x00;
						aux->buf[ 5] = 0x00;
						aux->buf[ 6] = aux->num_buttons;
						aux->buf[ 7] = 0x00;
						aux->buf[ 8] = 0x00;
						aux->buf[ 9] = 0x00;
						aux->buf[10] = aux->num_valuators;
						aux->buf[11] = 0x00;
						aux->buf[12] = 0x00;
						aux->buf[13] = 0x00;
						vrDbgPrintfN(1/*VRUIDD_DBGLVL*/, "_VruiDDPollFunction(): Sending out the CONNECT_REPLY message for protocol %d (0)\n", aux->protocol);
						write(aux->fd_receive_socket, aux->buf, 14);	/* TODO: warning, no effort yet being made to deal with endianness of the order */
						break;

					case 1:
						message = CONNECT_REPLY;		/* Positive connect reply with server layout (0x01 0x00) */
						aux->buf[ 0] = 0x01;
						aux->buf[ 1] = 0x00;
						aux->buf[ 2] = aux->protocol;		/* TODO: fill the correct values for all bytes (but this will work when the protocol number is less than 256. */
						aux->buf[ 3] = 0x00;
						aux->buf[ 4] = 0x00;
						aux->buf[ 5] = 0x00;
						aux->buf[ 6] = aux->num_6sensors;
						aux->buf[ 7] = 0x00;
						aux->buf[ 8] = 0x00;
						aux->buf[ 9] = 0x00;
						aux->buf[10] = aux->num_buttons;
						aux->buf[11] = 0x00;
						aux->buf[12] = 0x00;
						aux->buf[13] = 0x00;
						aux->buf[14] = aux->num_valuators;	/* TODO: fill the correct values for all bytes (but this will work when there's less than 256 of any type. */
						aux->buf[15] = 0x00;
						aux->buf[16] = 0x00;
						aux->buf[17] = 0x00;
						vrDbgPrintfN(1/*VRUIDD_DBGLVL*/, "_VruiDDPollFunction(): Sending out the CONNECT_REPLY message for protocol %d\n", aux->protocol);
						write(aux->fd_receive_socket, aux->buf, 18);	/* TODO: warning, no effort yet being made to deal with endianness of the order */
						aux->buf[18] = 0x00;
						aux->buf[19] = 0x00;
						//write(aux->fd_receive_socket, aux->buf, 20);	/* TODO: warning, no effort yet being made to deal with endianness of the order */
						break;

					case 2:
						message = CONNECT_REPLY;		/* Positive connect reply with server layout (0x01 0x00) */
						aux->buf[ 0] = 0x01;
						aux->buf[ 1] = 0x00;
						aux->buf[ 2] = aux->protocol;		/* TODO: fill the correct values for all bytes (but this will work when the protocol number is less than 256. */
						aux->buf[ 3] = 0x00;
						aux->buf[ 4] = 0x00;
						aux->buf[ 5] = 0x00;
						aux->buf[ 6] = aux->num_6sensors;
						aux->buf[ 7] = 0x00;
						aux->buf[ 8] = 0x00;
						aux->buf[ 9] = 0x00;
						aux->buf[10] = aux->num_buttons;
						aux->buf[11] = 0x00;
						aux->buf[12] = 0x00;
						aux->buf[13] = 0x00;
						aux->buf[14] = aux->num_valuators;	/* TODO: fill the correct values for all bytes (but this will work when there's less than 256 of any type. */
						aux->buf[15] = 0x00;
						aux->buf[16] = 0x00;
						aux->buf[17] = 0x00;
						vrDbgPrintfN(1/*VRUIDD_DBGLVL*/, "_VruiDDPollFunction(): Sending out the CONNECT_REPLY message for protocol %d\n", aux->protocol);
						aux->buf[18] = 0x00;			/* TODO: this is for the virtual devices which we are currently hard-coding to 0 */
						aux->buf[19] = 0x00;
						aux->buf[20] = 0x00;
						aux->buf[21] = 0x00;
						write(aux->fd_receive_socket, aux->buf, 22);	/* TODO: warning, no effort yet being made to deal with endianness of the order */
						break;
					}

					return_value = fsync(aux->fd_receive_socket); //-- this should work, but maybe it doesn't
//printf("fsync() returned %d\n", return_value);
//perror("fsync");
#if !defined(__APPLE__) && !defined(__CYGWIN__)	/* Darwin & Cygwin don't have the TCP_CORK option */
					int flag=0;
					setsockopt(aux->fd_receive_socket,IPPROTO_TCP,TCP_CORK,&flag,sizeof(int));
					flag=1;
					setsockopt(aux->fd_receive_socket,IPPROTO_TCP,TCP_CORK,&flag,sizeof(int));
#endif
					vrDbgPrintfN(VRUIDD_DBGLVL, "_VruiDDPollFunction(): Sent out the CONNECT_REPLY message\n");

					aux->vrui_link = 1;		/* set the flag to indicate we're now linked with a client */
					aux->streaming = 0;		/* set streaming to default off-state */
					break;

				case DISCONNECT_REQUEST:	/* Polite request to disconnect from server (0x02 0x00) */
					vrDbgPrintfN(VRUIDD_DBGLVL, "_VruiDDPollFunction(): Got a DISCONNECT_REQUEST message\n");
					close(aux->fd_receive_socket);
					aux->vrui_link = 0;		/* indicate we're not communicating with a Vrui client */
					aux->streaming = 0;		/* set streaming to default off-state */
					aux->fd_receive_socket = -1;	/* indicate that there's no one on the other side of the socket */
					break;

				case ACTIVATE_REQUEST:		/* Request to activate server (prepare for sending packets) (0x03 0x00) */
					vrDbgPrintfN(VRUIDD_DBGLVL, "_VruiDDPollFunction(): Got an ACTIVATE_REQUEST message -- treating as a no-op.\n");
#if 0 /* an attempt to 'flush' the socket -- seems like it's not needed */
					flag=0;
					setsockopt(aux->fd_receive_socket,IPPROTO_TCP,TCP_CORK,&flag,sizeof(int));
					flag=1;
					setsockopt(aux->fd_receive_socket,IPPROTO_TCP,TCP_CORK,&flag,sizeof(int));
					/* TODO: Presently treating as a no-op  -- should I at least call _VruiDDPutData(devinfo) here? */
					_VruiDDPutData(devinfo);	/* includes the PACKET_REPLY (0x06 0x00) */
#endif
					break;

				case DEACTIVATE_REQUEST:	/* Request to deactivate server (no more packet requests) (0x04 0x00) */
					vrDbgPrintfN(VRUIDD_DBGLVL, "_VruiDDPollFunction(): Got a DEACTIVATE_REQUEST message -- treating as a no-op.\n");
					/* TODO: Presently treating as a no-op */
					break;

				case PACKET_REQUEST:		/* Requests a single packet with current device state (0x05 0x00) */
					vrDbgPrintfN(VRUIDD_DBGLVL, "_VruiDDPollFunction(): Got a PACKET_REQUEST message -- sending.\n");
					_VruiDDPutData(devinfo);	/* includes the PACKET_REPLY (0x06 0x00) */
					break;

				case STARTSTREAM_REQUEST:	/* Requests entering stream mode (server sends packets automatically) (0x07 0x00) */
					vrDbgPrintfN(VRUIDD_DBGLVL, "_VruiDDPollFunction(): Got a STARTSTREAM_REQUEST message -- streaming.\n");
					aux->streaming = 1;
					break;

				case STOPSTREAM_REQUEST:	/* Requests leaving stream mode (0x08 0x00) */
					vrDbgPrintfN(VRUIDD_DBGLVL, "_VruiDDPollFunction(): Got a STOPSTREAM_REQUEST message -- cease streaming.\n");
					/* Set the outgoing message type -- STOPSTREAM_REPLY -- Server's reply after last stream packet has been sent (0x09 0x00) */
					aux->buf[0] = 0x09;
					aux->buf[1] = 0x00;
					write(aux->fd_receive_socket, aux->buf, 2);	/* TODO: warning, no effort yet being made to deal with endianness of the order */
					fsync(aux->fd_receive_socket);

					aux->streaming = 0;
					break;

				/* TODO: case STOPSTREAM_REPLY        * Server's reply after last stream packet has been sent (0x09 0x00) */

				}
			}
		}
	}

	/* Now if there is a connection, and we're to be streaming the data -- send it out */
	if (aux->vrui_link && aux->streaming) {
		vrTrace("_VruiDDPollFunction():", BOLD_TEXT "about to send data." NORM_TEXT);
		_VruiDDPutData(devinfo);
		vrTrace("_VruiDDPollFunction():", BOLD_TEXT "sent data." NORM_TEXT);
	}

	return;
}



	/******************************/
	/*** FreeVR public routines ***/
	/******************************/


/**********************************************************/
void vrVruiDDOutInitInfo(vrInputDevice *devinfo)
{
	devinfo->version = (char *)vrShmemStrDup("VruiDD out-dev 0.1");
	devinfo->Create = vrCallbackCreateNamed("VruiDD:Create-Def", _VruiDDCreateFunction, 1, devinfo);
	devinfo->Open = vrCallbackCreateNamed("VruiDD:Open-Def", _VruiDDOpenFunction, 1, devinfo);
	devinfo->Close = vrCallbackCreateNamed("VruiDD:Close-Def", _VruiDDCloseFunction, 1, devinfo);
	devinfo->Reset = vrCallbackCreateNamed("VruiDD:Reset-Def", _VruiDDResetFunction, 1, devinfo);
	devinfo->PollData = vrCallbackCreateNamed("VruiDD:PollData-Def", _VruiDDPollFunction, 1, devinfo);
	devinfo->PrintAux = vrCallbackCreateNamed("VruiDD:PrintAux-Def", _VruiDDPrintStruct, 0);
}

