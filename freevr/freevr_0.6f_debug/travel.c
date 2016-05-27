/* ======================================================================
 *
 *  CCCCC          travel.c
 * CC   CC         Author(s): Bill Sherman
 * CC              Created: November 5, 1999
 * CC   CC         Last Modified: July 6, 2006
 *  CCCCC
 *
 * Code file for a sample VR application with travel controls
 *  (mostly a copy of static.c)
 *
 * Copyright 2010, Bill Sherman & Friends, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>

#include "freevr.h"


/* these functions will be used as arguments */
void	init_gfx();
void	draw_world();
void	gfx_frame();
void	exit_application();
void	update_world();

#define PERSONAL_SIMULATOR
#ifdef PERSONAL_SIMULATOR
void	my_sim(vrRenderInfo *renderinfo, int mask);
#endif


/*********************************************************************/
int main(int argc, char* argv[])
{

	/* TODO: do I want to ignore interrupts for all of this?  or */
	/*   just during vrStart()?                                  */
	signal(SIGINT, SIG_IGN);

	vrShmemInit(30 * 1024 * 1024);
	vrConfigure(&argc, argv, NULL);

	/* TODO: it would be nice to put these above vrStart(), since */
	/*   it will block until all input devices are open, and this */
	/*   way we could be rendering something while waiting.       */
	/* TODO: actually, we almost *need* to have the init_gfx stuff*/
	/*   set before vrStart, so the window opening function will  */
	/*   have something to call (other than vrDoNothing()).       */
	/*   In fact, I have no idea how init_gfx() gets called at    */
	/*   all given the current setup.                             */
	/* [5/30/00: I have now moved the function setting routines to*/
	/*   come before vrStart().  The library needs to be changed  */
	/*   such that the incoming callbacks are stored in a holding */
	/*   bin, until the render/input/whatever loop is ready to    */
	/*   update them.                                             */
#if 1
	/* TODO: unfortunately this shorthand doesn't work when there are extra */
	/*   arguments but these work okay.                                     */
	vrCallbackSet(VRFUNC_DISPLAY_INIT, "init_gfx", init_gfx, 0);
	vrCallbackSet(VRFUNC_DISPLAY_FRAME, "gfx_frame", gfx_frame, 0);
	vrCallbackSet(VRFUNC_DISPLAY, "draw_world", draw_world, 0);
	vrCallbackSet(VRFUNC_DISPLAY_SIM, "my_sim", my_sim, 0);
	vrCallbackSet(VRFUNC_HANDLE_USR2, "travel:vrDbgInfo", vrDbgInfo, 0);
#else
#  if 1 /* 1/4/2003: test of possible startup race condition */
	vrSetFunc(VRFUNC_DISPLAY_INIT, vrCreateNamedCallback("init_gfx", init_gfx, 0));
#  endif
	vrSetFunc(VRFUNC_DISPLAY, vrCreateNamedCallback("draw_world", draw_world, 0));
#  ifdef PERSONAL_SIMULATOR
	vrSetFunc(VRFUNC_DISPLAY_SIM, vrCreateNamedCallback("my_sim", my_sim, 0));
#  endif
	vrSetFunc(VRFUNC_HANDLE_USR2, vrCreateNamedCallback("travel:vrDbgInfo", vrDbgInfo, 0));
#endif

	vrStart();

	signal(SIGINT, exit_application);

	if (vrContext->input->num_input_devices > 0) {
		vrDbgPrintfN(SELDOM_DBGLVL+1, "TRAVEL: input device[0] version = '%s'\n",
			vrContext->input->input_devices[0]->version);
	} else {
		vrDbgPrintfN(SELDOM_DBGLVL+1, "TRAVEL: No input devices defined.\n");
	}

#if 0
	if (vrDbgDo(AALWAYS_DBGLVL+1)) {
		vrFprintContext(stdout, vrContext, verbose);
		vrFprintConfig(stdout, vrContext->config, verbose);
		vrFprintInput(stdout, vrContext->input, verbose);
	}
#endif

        if(vrContext->input->num_input_devices > 0)
	    vrDbgPrintf("TRAVEL: input device[0] version = '%s'\n",
		vrContext->input->input_devices[0]->version);

#if 0 /* NOTE: this isn't really necessary, so don't use for non-test applications */
	update_world();
	vrInputWaitForAllInputDevicesToBeOpen();
#endif
	vrSystemSetName("travel -- FreeVR test program");
	vrSystemSetAuthors("Bill Sherman");
	vrSystemSetExtraInfo("A really good program for testing the FreeVR library");
	vrSystemSetStatusDescription("Application running fine");
	vrInputSet2switchDescription(0, "Terminate the application");
	vrInputSet2switchDescription(1, "Reset User Travel");
	vrInputSet2switchDescription(2, "Move User 0.1 to the right");
	vrInputSet2switchDescription(3, "Move User 0.1 to the left");
	vrInputSetValuatorDescription(0, "Rotate User");
	vrInputSetValuatorDescription(1, "Move User in direction of Wand");
#if 0 /* for testing */
	vrInputSetNswitchDescription(0, "Do something with a switch");
	vrInputSetNsensorDescription(0, "Do something with an N-sensor");
	vrInputSet6sensorDescription(0, "Move the Head");
	vrInputSet6sensorDescription(1, "Move the Wand");
#endif

	vrDbgPrintf("TRAVEL: looping\n");
#if 1
	while(vrFrame()) {
		update_world();
	}
#else /* run for two frames and then quit */
	vrFrame();
	update_world();
	vrFrame();
	update_world();
#endif

	vrPrintf(BOLD_TEXT "TRAVEL: I guess we're all done now.\n" NORM_TEXT);
	exit_application();

	exit(0);
}


/********************************************************************/
/* exit_application(): clean up anything started by the application */
/*   (forked processes, open files, open sockets, etc.)             */
/********************************************************************/
void exit_application()
{
	vrDbgPrintfN(ALWAYS_DBGLVL, "TRAVEL: quitting.\n");
	vrExit();

	exit(0);
}

#ifdef PERSONAL_SIMULATOR
/********************************************************************/
void my_sim(vrRenderInfo *renderinfo, int mask)
{
	vrUserInfo	*me;

#if 0 /* print some info about the main user */
	me = vrRenderCurrentUser(renderinfo);
	vrPrintf("My head pointer is %p (vrp is %p & vrhp is %p\n", me->head, me->head->visren_position, me->visren_headpos);
	vrPrintMatrix("  pointer position:", me->head->position);
	vrPrintMatrix("  pointer vr position:", me->head->visren_position);
	vrPrintMatrix("  head:", me->visren_headpos);
	vrPrintMatrix("  rw2vw:", me->rw2vw_xform);
#endif
	
	vrRenderDefaultSimulator(renderinfo, mask);
}
#endif


/********************************************************************/
void gfx_frame(vrRenderInfo *renderinfo)
{
#if 0
	vrSleep(30000);
#endif
}


/********************************************************************/
/* update_world(): ... */
/***********************/
void update_world()
{
static	vrTime		lastTime = -1;
	vrTime		deltaTime;
	float		joy_x, joy_y;
	vrVector	vec;
	int		input;

	if (lastTime < 0)
		lastTime = vrCurrentSimTime();

	/* do some traveling. */
	deltaTime = vrCurrentSimTime() - lastTime;
	lastTime += deltaTime;

	joy_x = vrGetValuatorValue(0);
	joy_y = vrGetValuatorValue(1);

	vrUserTravelLockSet(VR_ALLUSERS);
	if (fabs(joy_x) > 0.125) {
		vrUserTravelRotateId(VR_ALLUSERS, VR_Y, deltaTime * joy_x * -25.0);
	}

	if (fabs(joy_y) > 0.125) {
		vrVectorGetRWFrom6sensorDir(&vec, 1, VRDIR_FORE);
		vrVectorScale(&vec, deltaTime * joy_y * 2.5);
		vrUserTravelTranslateAd(VR_ALLUSERS, vec.v);
	}

	/* NOTE: this if relies on lazy evaluation -- the second condition */
	/*   shouldn't be checked if the first isn't true.                 */
	if (vrContext->input->num_2ways > 1 && vrGet2switchValue(1)) {
		vrUserTravelReset(VR_ALLUSERS);
	}

	/* NOTE: this if statement relies on lazy evaluation */
	if (vrContext->input->num_2ways > 2 && vrGet2switchValue(2)) {
		vrUserTravelTranslate3d(VR_ALLUSERS, 0.1, 0.0, 0.0);
	}
	/* NOTE: this if statement relies on lazy evaluation */
	if (vrContext->input->num_2ways > 3 && vrGet2switchValue(3)) {
		vrUserTravelTranslate3d(VR_ALLUSERS, -0.1, 0.0, 0.0);
	}
	vrUserTravelLockRelease(VR_ALLUSERS);

	/* set DebugLevel to 200 to print this info to screen every frame */
	if (vrDbgDo(RARE_DBGLVL)) {
		vrMsgPrintf("TRAVEL: controller --");

		vrMsgPrintf(" buttons:");
		for (input = 0; input < vrContext->input->num_2ways; input++)
			vrMsgPrintf(" %d", vrContext->input->switch2[input]->value);

		vrMsgPrintf(" valuators:");
		for (input = 0; input < vrContext->input->num_valuators; input++)
			vrMsgPrintf(" %6.3f", vrContext->input->valuator[input]->value);

		vrMsgPrintf("\n");
	}

#if 1
	/* Demonstration of how to segment different portions of the simulation in the statistics display */
	vrSystemSimCategory(1);		/* up until this point, count time spent in simulation as category-1 */
	vrSleep(10000);
	vrSystemSimCategory(2);		/* between last Stats call and this one, count time spent as category-2 */
	vrSleep(5000);
	/* NOTE: time spent from last stats call (or beginning) is counted as category-1 */
#else
	/* allow other processes to get some work done */
	vrSleep(1);
#endif
}

