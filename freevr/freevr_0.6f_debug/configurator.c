/* ======================================================================
 *
 *  CCCCC          config.c
 * CC   CC         Author(s): Bill Sherman
 * CC              Created: October 17, 2013 (from valtest.c)
 * CC   CC         Last Modified: February 20, 2014
 *  CCCCC
 *
 * Code file for a VR application designed to allow the configuration
 *  to be viewed for evaluation, and also to allow some tweaking of
 *  some properties of the configuration.
 *  - initially created to configure an IQ-station w/ SMARTTRACK
 *  - started as a copy of valtest.c (which came from travel.c)
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/*************************************************************************
USAGE:
	Running the command:
		% configurator [-system <name>] [-refobj <x> <y> <z>]
		-system -- Use the system within the configuration file as
			the basis for how to render the virtual world during
			the configuration process

		- refobj -- Place a reference object (blue) in the scene.

	Operating the application:

	...
		- pressing button #3 outputs the current calibration settings
			for the active sensor.  The Euler values are reported
			in the order:
				ELEV, AZIM, ROLL
			this an be translated into FreeVR configuration values
			as:
				r2e_translate = X, Y, Z;
				r2e_rotate *= -1.0, 0.0, 0.0,  ELEV;
				r2e_rotate *=  0.0,-1.0, 0.0,  AZIM;
				r2e_rotate *=  0.0, 0.0,-1.0,  ROLL;



HISTORY:
	17 October 2013 (Bill Sherman) -- wrote initial version starting from
		the code for "inputs" (i.e "valtest.c").  Presently it requires
		setting various #if statements in order to affect which aspect
		of the tracker configuration is being affected (e.g "t2rw" or
		"r2e" for a particular sensor.

		NOTE: focus is entirely on position tracking calibration.

	11 November 2013 (Bill Sherman) -- I added the ability to use a button
		press to loop through the active sensors to select the one
		being configured.  Another button press to toggle between
		translation and rotation, and another button to print values
		that can be applied to the configuration.  (And I fixed a bug
		in the way the values were being reported to the screen, and
		the way I used them to (mis!) configure the objects.

		I also added "shadow" copies of the first two sensors to the
		front wall so I could get a sense of where in space they are
		with a way to more easily make quantitative determinations
		(of the sort: is it in the center of the screen).

		(renamed "configurator")

	16 November 2013 (Bill Sherman, on the SC'13 Expo floor).  I further
		enhanced the sensor loops -- now the "t2rw" transformation is
		included after the last sensor.  I added "shadow" objects to
		all four of the primary screens.  I added cross-hairs down the
		center of each of the four screens for better quantitative
		analysis.

		(I also added these top-comments.)

	26-30 December 2013 (Bill Sherman).  Really this version was just
		updating of the todo list with some debugging code changes.

	20 February 2014 (Bill Sherman).  I now use the newly created
		"NoLastUpdate" versions of the input access programs so as
		not to disrupt the actual use of the inputs in the Update()
		routine.

TODO:
	- DONE: fix the missing button events issue
		- problem is that the "Delta" is being affected by the fact that
			I'm reading the button values in the world rendering
			routine, so that resets the button events.

	- add a means of putting a "reference object" in the world to assist
		with the positioning of the tracker system origin

	- DONE: move "draw_config_world()" from drawing.c to this file

	- add a null-region for the valuator inputs

	- add a scale factor for the valuator inputs

	- instructions should mention t2rw vs r2e

	- pass the WorldDataStruct to draw_config_world(), and using the
		knowledge of which sensor is "active", rendering its "shadow"
		objects as green.

	- output the exact lines to use in the ".freevrrc" file.

	- MOSTLY: add the ability to control which configured window objects will
		be rendered in the scene. (based on system names).  Really
		this is done by setting the "show_in_simulator" flag for
		the desired windows.

**************************************************************************/
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>

#include <GL/gl.h>

#include "freevr.h"
#include "vr_objects.h"		/* for vrObjectFirst() declaration */


/* Define a structure containing all the shared data */
typedef struct {
		int	sensor;
		int	rotVtrans;

		vrLock	lock;		/* to avoid conflicting  multiprocess accesses */
	} WorldDataType;


/* these functions will be used as arguments */
void	init_gfx();
void	draw_config_world();
void	exit_application();
void	update_world(WorldDataType *wd);


/*********************************************************************/
int main(int argc, char* argv[])
{
	WorldDataType	*wd;		/* pointer to the shared data */
	char		buffer[2048];	/* string buffer for self-control list */
	char		*progname;	/* string name this program was run as */

	/*****************************************************/
	/* adjust parameters based on command-line-arguments */
	progname = argv[0];
#if 0 /* not fully implemented yet */
	while ((argc > 1) && (argv[1][0] == '-')) {
		/* Specify a system in the configuration file to configure for */
		if (!strcmp(argv[1], "-system")) {
			argv++; argc--;
			if (argc > 1) {
				config_system = 1;
				system_name = strdup(argv[1]);
				argv++; argc--;
			} else {
				fprintf(stderr, err_missing_arg, progname, argv[0]);
				fprintf(stderr, RED_TEXT "Usage:" NORM_TEXT " %s ...]\n", progname);
				exit(1);
			}
		}

		/* Unknown option */
		else {
			/* There are currently no other "-" options, so this is an error */
			fprintf(stderr, RED_TEXT "Usage:" NORM_TEXT " %s [-system <name>] [-refobj <x> <y> <z>]\n", progname);
			exit(1);
		}
	}
#endif

#if 0
	/* if there are any arguments left, use them */
	if (argc > 1) {
		aux->server_host = strdup(argv[1]);
	}
#endif



	/**************************/
	/* Initiate VR operations */
	signal(SIGINT, SIG_IGN);

	vrConfigure(&argc, argv, NULL);

	/*****************************************************************************************/
	/* Between vrConfigure() and vrStart(), display some select info about the configuration */
	vrSystemInfo	*systemdef;
	vrSystemInfo	*system;
	vrProcessInfo	*this_proc = NULL;
	vrWindowInfo	*this_window = NULL;
	int		count = 0;
	int		proccount, wincount;

	/* first, list all the systems in the configuration */
	systemdef = (vrSystemInfo *)vrObjectFirst(vrContext, VROBJECT_SYSTEM);	/* get the head of the linked list of defined systems */
	vrPrintf("---------------------------------------------------------------------\n");
	vrPrintf("System in use is '%s' ('%s')\n", vrContext->config->system_name, vrContext->config->system->name);
	vrPrintf("Configuration defines %d systems.\n", ((vrObjectLists *)(vrContext->object_lists))->systemsDefined);
#if 0 /* not needed, more of a debugging statement. */
	vrPrintf("The first system is at %p ('%s')\n", systemdef, systemdef->name);
#endif
	while (systemdef != NULL) {
		count++;

#if 0 /* set to "1" to print the windows of all the systems */
		vrPrintf("System '%s' has procs:\n", systemdef->name);
		for (proccount = 0; proccount < systemdef->num_procs; proccount++) {
			vrPrintf(" '%s'", systemdef->proc_names[proccount]);
			this_proc = vrObjectSearch(vrContext, VROBJECT_PROCESS, systemdef->proc_names[proccount]);
			vrPrintf("%d", this_proc->type);
			if (this_proc->type == VRPROC_VISREN) {
				for (wincount = 0; wincount < this_proc->num_thing_names; wincount++) {
					vrPrintf("--%s--", this_proc->thing_names[wincount]);
				}
			}
		}
		vrPrintf("\n");
#else
		if (!strcmp(systemdef->name, "stone-tile")) {
			vrPrintf("Found system '%s' -- turning on window frames.\n", systemdef->name);
			/* if we have a system match, then turn on the "show_in_simulator" flag for each window */
			for (proccount = 0; proccount < systemdef->num_procs; proccount++) {
				this_proc = vrObjectSearch(vrContext, VROBJECT_PROCESS, systemdef->proc_names[proccount]);
				if (this_proc->type == VRPROC_VISREN) {
					for (wincount = 0; wincount < this_proc->num_thing_names; wincount++) {
						this_window = vrObjectSearch(vrContext, VROBJECT_WINDOW, this_proc->thing_names[wincount]);
						/* Only turn on the fixed-mount windows */
						if (this_window->mount == VRWINDOW_FIXED) {
							this_window->show_in_simulator = 1;
							vrPrintf("Turning on 'show_in_simulator' flag for window '%s'\n", this_window->name);
						}
					}
				}
			}
		}
#endif
		systemdef = systemdef->next;
	}
	vrPrintf("---------------------------------------------------------------------\n");
	vrSleep(1000000);
	/*****************************************************************************************/

	/********/
	vrStart();

	signal(SIGINT, exit_application);

	/*****************************************/
	/* Create and initialize the shared data */
	wd = (WorldDataType *)vrShmemAlloc0(sizeof(WorldDataType));
	wd->lock = vrLockCreate();
	wd->sensor = 0;
	wd->rotVtrans = 0;	/* 0 is rot, 1 is trans */

	/* TODO: it would be nice to put these above vrStart(), since */
	/*   it will block until all input devices are open, and this */
	/*   way we could be rendering something while waiting.       */
	/* TODO: actually, we almost *need* to have the init_gfx stuff*/
	/*   set before vrStart, so the window opening function will  */
	/*   have something to call (other than vrDoNothing()).       */
	/*   In fact, I have no idea how init_gfx() gets called at    */
	/*   all given the current setup.                             */
	vrFunctionSetCallback(VRFUNC_ALL_DISPLAY_INIT, vrCallbackCreate(init_gfx, 0));
	vrFunctionSetCallback(VRFUNC_ALL_DISPLAY, vrCallbackCreate(draw_config_world, 0));
	vrFunctionSetCallback(VRFUNC_HANDLE_USR2, vrCallbackCreate(vrDbgInfo, 0));

	if (vrContext->input->num_input_devices > 0) {
		vrDbgPrintfN(SELDOM_DBGLVL+1, "input viewer: input device[0] version = '%s'\n",
			vrContext->input->input_devices[0]->version);
	} else {
		vrDbgPrintfN(SELDOM_DBGLVL+1, "input viewer: No input devices defined.\n");
	}

	vrSystemSetName("configurator -- FreeVR configuration assistant");
	vrSystemSetAuthors("Bill Sherman");
	vrInputSet2switchDescription(0, "Terminate the application");
	vrInputSet2switchDescription(1, "Select the next sensor");
	vrInputSet2switchDescription(2, "Toggle between rotation and translation");
	vrInputSet2switchDescription(3, "Print current values to terminal");
	vrInputSetValuatorDescription(0, "Rotation or Translate");
	vrInputSetValuatorDescription(1, "Rotation or Translate");
	vrInputSetValuatorDescription(2, "Rotation or Translate");

	sprintf(buffer, "Adjusting 6sensor[%d] %s", wd->sensor, (wd->rotVtrans ? "translation" : "rotation"));
	vrSystemSetExtraInfo(buffer);

	while(vrFrame()) {
		update_world(wd);
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
void update_world(WorldDataType *wd)
{
	int		input;
	float		joy_x, joy_y;
	float		joy2_x, joy2_y;
	vrMatrix	*adjust_mat;
	vrMatrix	temp_mat;
static	char		buffer[2048];	/* string buffer adjusting documentation */

	joy_x = vrGetValuatorValue(0);
	joy_y = vrGetValuatorValue(1);
	joy2_x = vrGetValuatorValue(2);
	joy2_y = vrGetValuatorValue(3);

	/* When button "1" is pressed advance to the next sensor */
	if (vrGet2switchDelta(1) == 1) {
vrPrintf("Pressed button 1\n");
		vrLockWriteSet(wd->lock);
		wd->sensor++;
		if (wd->sensor == vrContext->input->num_6sensors) {
			wd->sensor = -1;
		}
		vrLockWriteRelease(wd->lock);

		if (wd->sensor < 0) {
			sprintf(buffer, "Adjusting tracker base %s", (wd->rotVtrans ? "translation" : "rotation"));
		} else {
			sprintf(buffer, "Adjusting 6sensor[%d] %s", wd->sensor, (wd->rotVtrans ? "translation" : "rotation"));
		}
		vrSystemSetExtraInfo(buffer);
	}

	/* When button "2" is pressed toggle between rotation and translation adjustment */
	if (vrGet2switchDelta(2) == 1) {
vrPrintf("Pressed button 2\n");
		vrLockWriteSet(wd->lock);
		wd->rotVtrans ^= 1;
		vrLockWriteRelease(wd->lock);

		if (wd->sensor < 0) {
			sprintf(buffer, "Adjusting tracker base %s", (wd->rotVtrans ? "translation" : "rotation"));
		} else {
			sprintf(buffer, "Adjusting 6sensor[%d] %s", wd->sensor, (wd->rotVtrans ? "translation" : "rotation"));
		}
		vrSystemSetExtraInfo(buffer);
	}

	/* When button "3" is pressed print the current setting */
	if (vrGet2switchDelta(3) == 1) {
		vrEuler		r2e_euler;
		vrQuat		r2e_quat;

vrPrintf("Pressed button 3\n");
		if (wd->sensor < 0) {
			/* Adjust the "t2rw_xform" of the first sensor using joysticks */
			vr6sensor *sensor6 = vrContext->input->sensor6[0];
			adjust_mat = sensor6->my_device->t2rw_xform;
		} else {
			/* Adjust a sensor r2e */
			vr6sensor *sensor6 = vrContext->input->sensor6[wd->sensor];
			adjust_mat = sensor6->r2e_xform;
		}

		vrEulerSetFromMatrix(&r2e_euler, adjust_mat);
		vrQuatSetFromMatrix(&r2e_quat, adjust_mat);
		vrPrintf("Sensor %d:  r2e: %.2f %.2f %.2f  %.2f %.2f %.2f\n", wd->sensor, r2e_euler.t[VR_X], r2e_euler.t[VR_Y], r2e_euler.t[VR_Z], r2e_euler.r[VR_ELEV], r2e_euler.r[VR_AZIM], r2e_euler.r[VR_ROLL]);
		vrPrintf("Sensor %d:  quat: %.3f %.3f %.3f %.3f\n", wd->sensor, r2e_quat.v[VR_X], r2e_quat.v[VR_Y], r2e_quat.v[VR_Z], r2e_quat.v[VR_W]);
	}

	/* Now adjust the active sensor */
	if (wd->sensor < 0) {
		/* Adjust the "t2rw_xform" of the first sensor using joysticks */
		vr6sensor *sensor6 = vrContext->input->sensor6[0];
		adjust_mat = sensor6->my_device->t2rw_xform;
	} else {
		/* Adjust a sensor r2e */
		vr6sensor *sensor6 = vrContext->input->sensor6[wd->sensor];
		adjust_mat = sensor6->r2e_xform;
	}

	if (wd->rotVtrans) {
		vrMatrixPostMult(adjust_mat, vrMatrixSetTranslation3d(&temp_mat, joy_x*0.1, joy_y*0.1, joy2_x*0.1));
	} else {
		vrMatrixPostMult(adjust_mat, vrMatrixSetRotationId(&temp_mat, VR_X, -joy_y));
		vrMatrixPostMult(adjust_mat, vrMatrixSetRotationId(&temp_mat, VR_Y, joy_x));
		vrMatrixPostMult(adjust_mat, vrMatrixSetRotationId(&temp_mat, VR_Z, joy2_x));
	}

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

/**********************************************************************/
/* draw_config_world(): draw a world for use as a configuration tool. */
/**********************************************************************/
/* TODO: in a final application, the WorldDataStructure should be  */
/*   passed to allow the "shadow" objects to be colored green when */
/*   they represent the active sensor.                             */
void draw_config_world(vrRenderInfo *rendinfo)
{
	vr6sensor	*sensor6;				/* temporary pointer to on of the 6-sensor inputs */
	vrInputInfo	*inputinfo = rendinfo->context->input;	/* Pointer to the overall input data structure */

	vrTrace("draw_config_world", "beginning");

#if !defined(ENABLE_STENCIL_STEREO_TEST) || 0 /* Don't do this when rendering to checkerboard stencil */
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif

#if 0
	/***********************************************************/
	/* draw colored axes at the origin: red=x, green=y, blue=z */
	draw_axes();
#endif
	/******************************************************************************************************/
	/* draw a yellow box w/ protruding colored axes at the tracking origin for the tracker of 6-sensor[0] */
	if (inputinfo->num_6sensors > 0) {
		sensor6 = inputinfo->sensor6[0];
		vrMatrix *mat = sensor6->my_device->t2rw_xform;
		glPushMatrix();
			vrRenderTransform(rendinfo, mat);
			draw_axes();
		glPopMatrix();
		glPushMatrix();
			glMultMatrixd(mat->v);
			glScalef(0.3, 0.3, 0.3);
			glColor3ub(184, 134, 72);
			vrShapeGLCubePlusOutline();
		glPopMatrix();
	}

#if defined(FREEVR) /* { */
	/*********************************************************/
	/* draw colored axes at the head: red=x, green=y, blue=z */
	if ((rendinfo->window->mount == VRWINDOW_SIMULATOR) && (rendinfo->context->input->num_6sensors > 0)) {
		glPushMatrix();
			vrRenderTransform6sensor(rendinfo, 0);
			draw_axes();
		glPopMatrix();
	}
#endif /* } */

	/******************************************/
	/* draw a "reference" object in the world */
	glPushMatrix();
		glTranslated(0.0, 3.0, 0.0);		/* Set this to location for reference object */
		glScalef(0.3, 0.3, 0.3);
		glColor3ub(100, 100, 255);
		vrShapeGLCubePlusOutline();
	glPopMatrix();

	/********************************************/
	/* draw "shadow" objects of the sensors on  */
	/*   the surface of the virtual CAVE walls. */
	if (inputinfo->num_6sensors > 0) {
		vrMatrix	sensor;
		vrPoint		sensor_location;
		int		sensor_num;		/* for looping over all sensors */

		for (sensor_num = 0; sensor_num < 2; sensor_num++) {
			if (vrPointGetRWFrom6sensor(&sensor_location, sensor_num) != NULL) {
				glColor3ub(184, 134, sensor_num*60);	/* shades of brown */
				/* NOTE: the color gets reset to "black" when "CubePlusOutline" */
				/*   objects are drawn.                                         */

				/* draw on front wall */
				glPushMatrix();
					glTranslated(sensor_location.v[VR_X], sensor_location.v[VR_Y], -5.0);
					glScalef(0.15, 0.15, 0.01);
					vrShapeGLCube();
				glPopMatrix();

				/* draw on floor surface */
				glPushMatrix();
					glTranslated(sensor_location.v[VR_X], 0.0, sensor_location.v[VR_Z]);
					glScalef(0.15, 0.01, 0.15);
					vrShapeGLCube();
				glPopMatrix();

				/* draw on right wall */
				glPushMatrix();
					glTranslated(5.0, sensor_location.v[VR_Y], sensor_location.v[VR_Z]);
					glScalef(0.01, 0.15, 0.15);
					vrShapeGLCube();
				glPopMatrix();

				/* draw on left wall */
				glPushMatrix();
					glTranslated(-5.0, sensor_location.v[VR_Y], sensor_location.v[VR_Z]);
					glScalef(0.01, 0.15, 0.15);
					vrShapeGLCube();
				glPopMatrix();
			}
		}
	}

	/***********************************************************/
	/* add some useful information for a configuration display */
	draw_config_lines(rendinfo);
	draw_inputs_text(rendinfo);
	draw_inputs(rendinfo);

	vrTrace("draw_config_world", "ending");
}


