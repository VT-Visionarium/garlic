/* ======================================================================
 *
 *  CCCCC          valtest.c (aka inputs -- new Makefile target)
 * CC   CC         Author(s): Bill Sherman
 * CC              Created: March 14, 2000
 * CC   CC         Last Modified: January 30, 2013
 *  CCCCC
 *
 * Code file for a sample VR application with multiple valuators.
 *  - initially created to test the cyberglove interface.
 *  - started as a copy of travel.c
 *  - now serves as a useful application when running as an input server
 *    (hence a new Makefile target was added called "inputs" that uses this file.)
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>

#include "freevr.h"


/* these functions will be used as arguments */
void	init_gfx();
void	draw_server_world();
void	exit_application();
void	update_world();
char	*list_self_controls();


/*********************************************************************/
int main(int argc, char* argv[])
{
	char	buffer[2048];		/* string buffer for self-control list */

	signal(SIGINT, SIG_IGN);

	vrConfigure(&argc, argv, NULL);
	vrStart();

	signal(SIGINT, exit_application);

	/* TODO: it would be nice to put these above vrStart(), since */
	/*   it will block until all input devices are open, and this */
	/*   way we could be rendering something while waiting.       */
	/* TODO: actually, we almost *need* to have the init_gfx stuff*/
	/*   set before vrStart, so the window opening function will  */
	/*   have something to call (other than vrDoNothing()).       */
	/*   In fact, I have no idea how init_gfx() gets called at    */
	/*   all given the current setup.                             */
	vrFunctionSetCallback(VRFUNC_ALL_DISPLAY_INIT, vrCallbackCreate(init_gfx, 0));
	vrFunctionSetCallback(VRFUNC_ALL_DISPLAY, vrCallbackCreate(draw_server_world, 0));
	vrFunctionSetCallback(VRFUNC_HANDLE_USR2, vrCallbackCreate(vrDbgInfo, 0));

	if (vrContext->input->num_input_devices > 0) {
		vrDbgPrintfN(SELDOM_DBGLVL+1, "input viewer: input device[0] version = '%s'\n",
			vrContext->input->input_devices[0]->version);
	} else {
		vrDbgPrintfN(SELDOM_DBGLVL+1, "input viewer: No input devices defined.\n");
	}

	vrSystemSetName("inputs -- FreeVR inputs display application");
	vrSystemSetAuthors("Bill Sherman");
	list_self_controls(buffer, vrContext->input, sizeof(buffer), verbose);
	vrPrintf(buffer);
	vrSystemSetExtraInfo(buffer);

	vrDbgPrintf("input viewer: looping\n");
	while(vrFrame()) {
		update_world();
	}

	exit_application();

	exit(0);
}


/********************************************************************/
/* exit_application(): clean up anything started by the application */
/*   (forked processes, open files, open sockets, etc.)             */
/********************************************************************/
void exit_application()
{
	vrDbgPrintfN(ALWAYS_DBGLVL, "input viewer: quitting.\n");
	vrExit();
}


/***********************/
/* update_world(): ... */
/***********************/
void update_world()
{
	int		input;

	/* Nothing happens, there are no moving objects in the world. */

	/* set DebugLevel to 200 to print this info to screen every frame */
	if (vrDbgDo(RARE_DBGLVL)) {
		vrMsgPrintf("input viewer: controller --");

		vrMsgPrintf(" buttons:");
		for (input = 0; input < vrContext->input->num_2ways; input++)
			vrMsgPrintf(" %d", vrContext->input->switch2[input]->value);

		vrMsgPrintf(" valuators:");
		for (input = 0; input < vrContext->input->num_valuators; input++)
			vrMsgPrintf(" %6.3f", vrContext->input->valuator[input]->value);

		vrMsgPrintf("\n");
	}

	/* allow other processes to get some work done */
	vrSleep(10000);
}


/****************************************************************************/
/* list_self_controls(): A function that creates a string containing a list */
/*   of all the self-controls within the active FreeVR system.  This string */
/*   can then be printed or added to the help string.                       */
/*                                                                          */
/* The arguments are based on the vrSprintInputUI() function in vr_input.c. */
/*   As with that, the "style" argument is currently ignored.               */
/*                                                                          */
/* TODO: consider adding a robust version of this function as part of the   */
/*   overall vr_input.c code base.                                          */
/****************************************************************************/
char *list_self_controls(char *str, vrInputInfo *inputinfo, int str_size, vrPrintStyle style)
{
	int	bytes_remaining = str_size;
	int	bytes_used = 0;
	int	bytes;			/* return result from snprintf() calls -- usually # of bytes written */

	int	count;
	int	devcount;
	vrInput	*input;

	/* Warning: snprintf() returns number of bytes that *would* have been written, not actual! */

#if 0 /* These are inputs, not self-controls, but I may want this in the future */
	bytes = snprintf(str, bytes_remaining, "Num Buttons: %d\n", inputinfo->num_2ways);
	bytes_used += bytes; bytes_remaining -= bytes;
	if ((bytes < 1) || (bytes_remaining < 1)) { str[str_size] = '\0'; return str; }	/* for safety */

	for (count = 0; count < inputinfo->num_2ways; count++) {
		bytes = snprintf(&str[bytes_used], bytes_remaining, "  Button %d: '%s' -- '%s'\n", count, inputinfo->switch2[count]->my_object->name, inputinfo->switch2[count]->my_object->desc_str);
		bytes_used += bytes; bytes_remaining -= bytes;
		if ((bytes < 1) || (bytes_remaining < 1)) { str[str_size] = '\0'; return str; }	/* for safety */
	}

	bytes = snprintf(&str[bytes_used], bytes_remaining, "Num Valuators: %d\n", inputinfo->num_valuators);
	bytes_used += bytes; bytes_remaining -= bytes;
	if ((bytes < 1) || (bytes_remaining < 1)) { str[str_size] = '\0'; return str; }	/* for safety */
	for (count = 0; count < inputinfo->num_valuators; count++) {
		bytes = snprintf(&str[bytes_used], bytes_remaining, "  Valuator %d: '%s' -- '%s'\n", count, inputinfo->valuator[count]->my_object->name, inputinfo->valuator[count]->my_object->desc_str);
		bytes_used += bytes; bytes_remaining -= bytes;
		if ((bytes < 1) || (bytes_remaining < 1)) { str[str_size] = '\0'; return str; }	/* for safety */
	}

	bytes = snprintf(&str[bytes_used], bytes_remaining, "Num 6-sensors: %d\n", inputinfo->num_6sensors);
	bytes_used += bytes; bytes_remaining -= bytes;
	if ((bytes < 1) || (bytes_remaining < 1)) { str[str_size] = '\0'; return str; }	/* for safety */
	for (count = 0; count < inputinfo->num_6sensors; count++) {
		bytes = snprintf(&str[bytes_used], bytes_remaining, "  6-sensor %d: '%s' -- '%s'\n", count, inputinfo->sensor6[count]->my_object->name, inputinfo->sensor6[count]->my_object->desc_str);
		bytes_used += bytes; bytes_remaining -= bytes;
		if ((bytes < 1) || (bytes_remaining < 1)) { str[str_size] = '\0'; return str; }	/* for safety */
	}

	/* TODO: Nswitches  -- num_Nways/switchN */

	/* TODO: Nsensors -- num_Nsensors/sensorN */

	/* TODO: Controls -- num_controls/control */
#endif

	bytes = snprintf(&str[bytes_used], bytes_remaining, "Self-controls:\n");
	bytes_used += bytes; bytes_remaining -= bytes;
	if ((bytes < 1) || (bytes_remaining < 1)) { str[str_size] = '\0'; return str; }	/* for safety */
	for (devcount = 0; devcount < inputinfo->num_input_devices; devcount++) {
		/* NOTE: we could list the inputs for each input device here, but as */
		/*   the default (and only choice) mapping includes all inputs, such */
		/*   a list would be redundant.                                      */
		input = inputinfo->input_devices[devcount]->inputs; /* not sure how to get the count -- other than a NULL terminal on the list */

		/* List the self-controls for each input device. */
		input = inputinfo->input_devices[devcount]->self_controls;
		for (count = 0; count < inputinfo->input_devices[devcount]->num_scontrols; count++) {
#if 0 /* 0 is somewhat more verbose */
			bytes = snprintf(&str[bytes_used], bytes_remaining, "  Device[%d] (%s): self-control %d: '%s' -- '%s'\n", devcount, inputinfo->input_devices[devcount]->name, count, input->name, input->desc_str);
#else
			bytes = snprintf(&str[bytes_used], bytes_remaining, "  %s self-control: '%s' from '%s'\n", inputinfo->input_devices[devcount]->name, input->name, input->desc_str);
#endif
			bytes_used += bytes; bytes_remaining -= bytes;
			if ((bytes < 1) || (bytes_remaining < 1)) { str[str_size] = '\0'; return str; }	/* for safety */

			input = input->next;
		}
	}
}

