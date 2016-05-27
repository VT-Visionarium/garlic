/* ======================================================================
 *
 *  CCCCC          static.c
 * CC   CC         Author(s): Bill Sherman
 * CC              Created: long ago
 * CC   CC         Last Modified: September 9, 2002
 *  CCCCC
 *
 * Code file for a simple sample VR application
 *
 * Copyright 2010, Bill Sherman & Friends, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include "freevr.h"


/* these functions will be used as arguments */
void	init_gfx();
void	draw_world();
void	exit_application();
void	update_world();

#define PERSONAL_SIMULATOR
#ifdef PERSONAL_SIMULATOR
void	my_sim(vrRenderInfo *renderinfo, int mask);
#endif


/*********************************************************************/
int main(int argc, char* argv[])
{

	signal(SIGINT, SIG_IGN);

	vrConfigure(&argc, argv, NULL);
	vrCallbackSet(VRFUNC_ALL_DISPLAY_INIT, "init_gfx", init_gfx, 0);
	vrCallbackSet(VRFUNC_ALL_DISPLAY, "draw_world", draw_world, 0);
#ifdef PERSONAL_SIMULATOR
	vrCallbackSet(VRFUNC_ALL_DISPLAY_SIM, "my_sim", my_sim, 0);
#endif

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
	vrCallbackSet(VRFUNC_HANDLE_USR2, "travel:vrDbgInfo", vrDbgInfo, 0);

	if (vrContext->input->num_input_devices > 0) {
		vrDbgPrintfN(SELDOM_DBGLVL+1, "STATIC: input device[0] version = '%s'\n",
			vrContext->input->input_devices[0]->version);
	} else {
		vrDbgPrintfN(SELDOM_DBGLVL+1, "STATIC: No input devices defined.\n");
	}

	if (vrDbgDo(AALWAYS_DBGLVL+1)) {
		vrFprintContext(stdout, vrContext, verbose);
		vrFprintConfig(stdout, vrContext->config, verbose);
		vrFprintInput(stdout, vrContext->input, verbose);
	}

	vrDbgPrintf("STATIC: input device[0] version = '%s'\n",
		vrContext->input->input_devices[0]->version);

	vrDbgPrintf("STATIC: looping\n");

	/* TODO: this really should use a queued event, because we often miss the key event */
	while(!vrGet2switchValue(0)) {
		update_world();
	}

	vrPrintf(BOLD_TEXT "STATIC: I guess we're all done now.\n" NORM_TEXT);
	exit_application();

	exit(0);
}


/********************************************************************/
/* exit_application(): clean up anything started by the application */
/*   (forked processes, open files, open sockets, etc.)             */
/********************************************************************/
void exit_application()
{
	vrDbgPrintfN(ALWAYS_DBGLVL, "STATIC: quitting.\n");
	vrExit();
}


#ifdef PERSONAL_SIMULATOR
/********************************************************************/
void my_sim(vrRenderInfo *renderinfo, int mask)
{
#if 0
	vrPrintf("Yo, doing my own simulator!\n");
#endif
	vrRenderDefaultSimulator(renderinfo, mask);
}
#endif


/***********************/
/* update_world(): ... */
/***********************/
void update_world()
{
	/* Nothing happens -- this is a static world. */

	int		input;

	/* set DebugLevel to 200 to print this info to screen every frame */
	if (vrDbgDo(RARE_DBGLVL)) {
		vrMsgPrintf("STATIC: controller --");

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

