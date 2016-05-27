/* ======================================================================
 *
 *  CCCCC          drawing.c
 * CC   CC         Author(s): Bill Sherman
 * CC              Created: (unknown, sometime before June 20, 1998)
 * CC   CC         Last Modified: February 20, 2014
 *  CCCCC
 *
 * Code file for a sample VR application to render a simple world.
 *   A lot of this code came from my VR programming training examples.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#include <stdio.h>
#include <GL/gl.h>

#if defined(FREEVR)
#  include "freevr.h"
#  include "vr_visren.h"	/* needed for RenderInfo to get eye info */
#  include "vr_visren.glx.h"	/* needed to get the value of ENABLE_STENCIL_STEREO_TEST (remove when that test is complete) */

#  define	HEAD_SENSOR		0
#  define	WAND_SENSOR		1
#  define	WIREDHEAD_SENSOR	2
#  define	WIREDWAND_SENSOR	3
#else
   void vrPrintf() { }		/* needed for vr_basicgfx.glx.h */
#  define	vrDbgDo(n)	1
#  define	vrPrintf	printf
#  define	vrErrPrintf	printf
#  define	vrFprintf	fprintf
#endif

#include "vr_basicgfx.glx.h"

#undef	LIMITED_TRACE
#undef	SECOND_USER_PERSPECTIVE_TEST

void draw_axes();
void world1(vrRenderInfo *rendinfo);
void world2(vrRenderInfo *rendinfo);
void gray_world(vrRenderInfo *rendinfo);
void draw_text(vrRenderInfo *rendinfo);
void draw_inputs_text(vrRenderInfo *rendinfo);
void draw_lines(vrRenderInfo *rendinfo);
void draw_config_lines(vrRenderInfo *rendinfo);
void draw_inputs(vrRenderInfo *rendinfo);


/*********************************************************************/
/* init_gfx(): initialize general graphics properties (eg. lighting) */
/*********************************************************************/
void init_gfx(vrRenderInfo *rendinfo)
{
static	int	num_init = 0;

#ifdef UNPICKY_COMPILER
	GLfloat	light_position[] = { 1.0, 1.0, 9.0, 1.0 };
	GLfloat	light_diffuse[] = { 0.8, 0.8, 0.8, 1.0 };
	GLfloat	light_ambient[] = { 0.3, 0.3, 0.3, 1.0 };
#endif

#if defined(FREEVR) && 0
	vrPrintf("App: " BOLD_TEXT "Entering init_gfx().  When am I?\n" NORM_TEXT);
	vrPrintf("App: init graphics for context %p, persp %p, window %p (%d), eye %p\n",
		rendinfo->context, rendinfo->persp, rendinfo->window, rendinfo->window->num, rendinfo->eye);
#endif
	/* If we init too many times, there is probably a bug, so abort and dump core */
	num_init++;
	if (num_init == 10) abort();


	glShadeModel(GL_SMOOTH);
#if 0 /* this causes overly thick lines on SGI/IRIX */
	glEnable(GL_LINE_SMOOTH);
#endif
	glLineWidth(2.0);

	glDrawBuffer(GL_FRONT_AND_BACK);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDrawBuffer(GL_BACK);

#if 0
	/* TODO: when using a lighting model, objects without a */
	/*   material will appear as shades of gray.            */
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
#endif
}


/*********************************************************************/
/* draw_world(): draw a simple world.                                */
/*********************************************************************/
void draw_world(vrRenderInfo *rendinfo)
{

#if 0 /* include this when testing the stats feature -- especially for multiple bins */
vrSleep(12000);
vrRenderCategory(rendinfo, 2);
vrSleep(8000);
#endif
#if 0
	int		val;
	do {
		val = glGetError();
		vrPrintf(RED_TEXT "glGetError returned %d\n" NORM_TEXT, val);
	} while (val != 0);
#endif

#if defined(FREEVR) && 0
	vrPrintf("App: render graphics for context %p, persp %p, window %p (%d), eye %p (%d)\n",
		rendinfo->context, rendinfo->persp,
		rendinfo->window, rendinfo->window->num,
		rendinfo->eye, rendinfo->eye->num);
#endif
#if 0 /* To print information about which process/thread we are rendering to */
	vrFprintProcessInfo(stdout, rendinfo->window->proc, oneline);
#endif
	vrTrace("draw_world", "beginning");

#if !defined(ENABLE_STENCIL_STEREO_TEST) || 0 /* Don't do this when rendering to checkerboard stencil */
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif

	/* draw colored axes at the origin: red=x, green=y, blue=z */
	draw_axes();

#if defined(FREEVR) /* { */
	/* draw colored axes at the head: red=x, green=y, blue=z */
	if ((rendinfo->window->mount == VRWINDOW_SIMULATOR) && (rendinfo->context->input->num_6sensors > 0)) {
		glPushMatrix();
			vrRenderTransform6sensor(rendinfo, 0);
			draw_axes();
		glPopMatrix();
	}
#endif /* } */

vrTrace("draw_world", "yo1");
	/* render a simple world */
#if 1
	world1(rendinfo);	/* world populated by various objects in the RW, plus a pyramid in the VW */
#elif 1
	world2(rendinfo);	/* a world with a cube in each of the 6 directions, plus a pyramid on the floor -- all in the RW */
#else
	gray_world(rendinfo);	/* just a RW gray cube and VW gray pyramid */
#endif
vrTrace("draw_world", "yo2");

	/* add some other information */
	draw_text(rendinfo);
vrTrace("draw_world", "yo3");
	draw_lines(rendinfo);
vrTrace("draw_world", "yo4");
	draw_inputs(rendinfo);
vrTrace("draw_world", "yo5");

#if 0
	glColor3ub(100,255,100);
	vrShapeGLFloor();
#endif

	vrTrace("draw_world", "ending");
}


/************************************************************************/
/* draw_server_world(): draw a simple world for use as an input server. */
/************************************************************************/
void draw_server_world(vrRenderInfo *rendinfo)
{
	vrTrace("draw_server_world", "beginning");

#if !defined(ENABLE_STENCIL_STEREO_TEST) || 0 /* Don't do this when rendering to checkerboard stencil */
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif

#if 0
	/* draw colored axes at the origin: red=x, green=y, blue=z */
	draw_axes();
#endif

#if defined(FREEVR) /* { */
	/* draw colored axes at the head: red=x, green=y, blue=z */
	if ((rendinfo->window->mount == VRWINDOW_SIMULATOR) && (rendinfo->context->input->num_6sensors > 0)) {
		glPushMatrix();
			vrRenderTransform6sensor(rendinfo, 0);
			draw_axes();
		glPopMatrix();
	}
#endif /* } */

	/* add some useful information for a server display */
#if 0 /* it's a bit too much right now */
	draw_lines(rendinfo);
#endif
	draw_inputs_text(rendinfo);
	draw_inputs(rendinfo);

	vrTrace("draw_server_world", "ending");
}


/*********************************************************************/
void draw_axes()
{
	glBegin(GL_LINES);
		/* positive X-axis in red */
		glColor3ub(128, 0, 0);
		glVertex3f(0.0, 0.0, 0.0);
		glVertex3f(2.0, 0.0, 0.0);

		/* positive Y-axis in green */
		glColor3ub(0, 128, 0);
		glVertex3f(0.0, 0.0, 0.0);
		glVertex3f(0.0, 2.0, 0.0);

		/* positive Z-axis in blue */
		glColor3ub(30, 30, 128);
		glVertex3f(0.0, 0.0, 0.0);
		glVertex3f(0.0, 0.0, 2.0);
	glEnd();
}


/*********************************************************************/
void gray_world(vrRenderInfo *rendinfo)
{

	/* NOTE: start in RW coordinate space */

	/* a 2' 50% gray cube just above the floor of the cave */
	glPushMatrix();
		glColor3ub(128, 128, 128);
		glTranslatef(-2.5, 2.0, -0.5);
		glScalef(1.0, 1.0, 1.0);
		vrShapeGLCubePlusOutline();
		glPopMatrix();

		glPushMatrix();
#if defined(FREEVR) /* { */
		vrRenderTransformUserTravel(rendinfo);	/* now in VW coordinate space */
#endif /* } */

		/* an 80% gray pyramid near eye-height */
		glPushMatrix();
			glColor3ub(204, 204, 204);
			glTranslatef(-2.3, 6.2, -0.5);
			glScalef(1.5, 1.5, 1.5);
			glRotatef(90.0, 1.0, 0.0, 0.0);
#if 0
			vrShapeGLPyramidPlusOutline();
#else
			vrShapeGLPyramid();
#endif
		glPopMatrix();

	glPopMatrix();			/* back to RW coordinate space */
}


/*********************************************************************/
void world1(vrRenderInfo *rendinfo)
{

#ifndef LIMITED_TRACE
	vrTrace("draw_world", "the more complicated test world");
#endif
	/* NOTE: start in RW coordinate space */

	/* a 1' rose cube behind the cave */
	glPushMatrix();
		glColor3ub(240, 138, 133);
		glTranslatef(0.0, 5.0, 7.0);
		glScalef(0.5, 0.5, 0.5);
		vrShapeGLCubePlusOutline();
	glPopMatrix();

	/* a 4' blue cube in front of the cave */
	glPushMatrix();
		glColor3ub(0, 0, 255);
		glTranslatef(0.0, 3.0, -7.0);
		glScalef(2.0, 2.0, 2.0);
		vrShapeGLCubePlusOutline();
	glPopMatrix();

	/* a 4' white cube left of the cave */
	glPushMatrix();
		glColor3ub(255, 255, 255);
		glTranslatef(-7.0, 3.0, 0.0);
		glScalef(2.0, 2.0, 2.0);
		vrShapeGLCubePlusOutline();
	glPopMatrix();

	/* a pink pyramid on the floor of the cave near the front wall */
	glPushMatrix();
		glColor3ub(255, 100, 100);
		glTranslatef(0.0, 1.0, -4.0);
		vrShapeGLPyramidPlusOutline();
	glPopMatrix();

	/* a 2' light purple cube at the front left corner of the cave */
	glPushMatrix();
		glColor3ub(100, 100, 255);
		glTranslatef(-6.0, 7.0, -6.0);
		glScalef(1.0, 1.0, 1.0);
		vrShapeGLCubePlusOutline();
	glPopMatrix();

	/* a 2' green cube below the cave */
	glPushMatrix();
		glColor3ub(0, 255, 0);
		glTranslatef(5.0, 1.0, 0.0);
		glScalef(1.0, 1.0, 1.0);
		vrShapeGLCubePlusOutline();
	glPopMatrix();

	/* a 2' dark purple cube below the center of the cave */
	glPushMatrix();
		glColor3ub(100, 0, 255);
		glTranslatef(0.0, -1.0, 0.0);
		glScalef(1.0, 1.0, 1.0);
		vrShapeGLCubePlusOutline();
	glPopMatrix();

	/* a 10' tall red pillar inside the right front corner of the cave */
	glPushMatrix();
		glColor3ub(240, 50, 50);
		glTranslatef(4.5, 5.0, -4.5);
		glScalef(0.5, 5.0, 0.5);
		vrShapeGLCubePlusOutline();
	glPopMatrix();

	/* a 10' deep cyan pillar inside the right front corner of the cave */
	glPushMatrix();
		glColor3ub(50, 240, 240);
		glTranslatef(4.5, -5.0, -4.5);
		glScalef(0.5, 5.0, 0.5);
		vrShapeGLCubePlusOutline();
	glPopMatrix();

	/* a 4' tall yellow pillar near the right wall of the cave */
	glPushMatrix();
		glColor3ub(240, 240, 50);
		glTranslatef(-2.5, 2.0, 2.0);
		glScalef(0.5, 2.0, 0.5);
		vrShapeGLCubePlusOutline();
	glPopMatrix();

#  if 0 /* temporarily turned off so we can see the front screen */
	/* a 4' long orange pillar out of the right wall of the cave */
	glPushMatrix();
		glColor3ub(240, 130, 30);
		glTranslatef(-3.0, 2.0, -2.5);
		glScalef(2.0, 0.5, 0.5);
		vrShapeGLCubePlusOutline();
	glPopMatrix();
#  endif

	glPushMatrix();
#if defined(FREEVR) /* { */
		vrRenderTransformUserTravel(rendinfo);	/* now in VW coordinate space */
#endif /* } */

		/* an 80% gray pyramid near eye-height */
		glPushMatrix();
			glColor3ub(204, 204, 204);	/* 80% gray */
			glTranslatef(-2.3, 6.2, -0.5);
			glScalef(1.5, 1.5, 1.5);
			glRotatef(90.0, 1.0, 0.0, 0.0);
			vrShapeGLPyramidPlusOutline();
		glPopMatrix();
	glPopMatrix();			/* back to RW coordinate space */

#if defined(FREEVR) && defined(SECOND_USER_PERSPECTIVE_TEST) /* (FreeVR-only feature) { */
	/* This is a new one!  [01/27/2009]  An object that is rendered from  */
	/*   the perspective of a second user (although for the moment really */
	/*   just some other tracking device.                                 */
	glPushMatrix();
#if 0 /* Choose an option */
		/* render from the perspective of sensor-1 (aka the wand) -- skip if push fails */
		/* Hmmm, so this will mean that if the push fails, this bit of the world won't  */
		/*   get rendered -- interesting to consider whether this is the appropriate    */
		/*   behavior.                                                                  */
		if (vrRenderPushPerspFrom6sensor(rendinfo, WAND_SENSOR)) {
			glPushMatrix();
				glColor3ub(240, 130, 30);	/* orange */
				glTranslatef(0.0, 5.0, 0.0);	/* ~center of a CAVE */
				glScalef(0.5, 0.5, 0.5);
				vrShapeGLPyramidPlusOutline();
			glPopMatrix();

			vrRenderPopPersp(rendinfo);
		}
#elif 0
		/* This version puts the orange pyramid at the location of the WAND, */
		/*   but viewed from the 2nd user POV.                               */
		if (vrRenderPushPerspFrom6sensor(rendinfo, WIREDHEAD_SENSOR)) {
			glPushMatrix();
				glColor3ub(240, 130, 30);	/* orange */
				vrRenderTransform6sensor(rendinfo, WAND_SENSOR);	/* located at the WAND */
				glScalef(0.5, 0.5, 0.5);
				vrShapeGLPyramidPlusOutline();
			glPopMatrix();

			vrRenderPopPersp(rendinfo);
		}
#elif 1
		/* This version puts the orange pyramid at the location of the WAND, */
		/*   and renders it from both users' points of view.                 */
		glPushMatrix();
			glColor3ub(240, 130, 30);	/* orange */
			vrRenderTransform6sensor(rendinfo, WAND_SENSOR);	/* located at the WAND */
			glScalef(0.5, 0.5, 0.5);
			vrShapeGLPyramidPlusOutline();
		glPopMatrix();
		if (vrRenderPushPerspFrom6sensor(rendinfo, WIREDHEAD_SENSOR)) {
			glPushMatrix();
				glColor3ub(240, 130, 30);	/* orange */
				vrRenderTransform6sensor(rendinfo, WAND_SENSOR);	/* located at the WAND */
				glScalef(0.5, 0.5, 0.5);
				vrShapeGLPyramidPlusOutline();
			glPopMatrix();

			vrRenderPopPersp(rendinfo);
		}
#endif

	glPopMatrix();
#endif /* }  (FreeVR-only code) */

#ifndef LIMITED_TRACE
	vrTrace("draw_world", "end of the more complicated test world");
#endif
}


/*********************************************************************/
void world2(vrRenderInfo *rendinfo)
{

	/* NOTE: start in RW coordinate space */

	/* a 6' blue cube left of the cave */
	glPushMatrix();
		glColor3ub(0, 0, 255);
		glTranslatef(-10.0, 3.0, 0.0);
		glScalef(3.0, 3.0, 3.0);
		vrShapeGLCubePlusOutline();
	glPopMatrix();

	/* a 6' dark purple cube above the user */
	glPushMatrix();
		glColor3ub(100, 0, 255);
		glTranslatef(0.0, 10.0, 0.0);
		glScalef(3.0, 3.0, 3.0);
		vrShapeGLCubePlusOutline();
	glPopMatrix();

	/* a 6' red cube right of the cave */
	glPushMatrix();
		glColor3ub(255, 0, 0);
		glTranslatef(10.0, 3.0, 0.0);
		glScalef(3.0, 3.0, 3.0);
		vrShapeGLCubePlusOutline();
	glPopMatrix();

	/* a 6' green cube below the cave */
	glPushMatrix();
		glColor3ub(0, 255, 0);
		glTranslatef(0.0, -10.0, 0.0);
		glScalef(3.0, 3.0, 3.0);
		vrShapeGLCubePlusOutline();
	glPopMatrix();

	/* a 6' light purple cube behind the cave */
	glPushMatrix();
		glColor3ub(100, 100, 255);
		glTranslatef(0.0, 3.0, 10.0);
		glScalef(3.0, 3.0, 3.0);
		vrShapeGLCubePlusOutline();
	glPopMatrix();

	/* a 6' white cube in front of the cave */
	glPushMatrix();
		glColor3ub(255, 255, 255);
		glTranslatef(0.0, 3.0, -10.0);
		glScalef(3.0, 3.0, 3.0);
		vrShapeGLCubePlusOutline();
	glPopMatrix();

	/* a pyramid sticking through the floor of the cave near the right wall */
	glPushMatrix();
		glColor3ub(255, 100, 100);
		glTranslatef(4.0, 0.0, 0.0);
		vrShapeGLPyramidPlusOutline();
	glPopMatrix();
}


/*********************************************************************/
void draw_inputs(vrRenderInfo *rendinfo)
{
	int		count;
	vrInputInfo	*inputinfo;	/* Pointer to the overall input data structure */

#ifndef LIMITED_TRACE
	vrTrace("draw_inputs", "doing the input-vis rendering");
#endif
	inputinfo = rendinfo->context->input;

	/*******************************************************/
	/** Print some boxes representing 2-switch input info **/
	/*******************************************************/
	for (count = 0; count < inputinfo->num_2ways; count++) {
		glPushMatrix();
		glTranslatef(-4.5 + (0.35 * count), 0.0, -4.0);
		glScalef(0.1, 1.2 * vrGet2switchValueNoLastUpdate(count) + 0.1, 0.1);
		glColor3ub(84, 134, 72);
		vrShapeGLCubePlusOutline();
		glPopMatrix();
	}

	/*******************************************************/
	/** Print some boxes representing N-switch input info **/
	/*******************************************************/
	for (count = 0; count < inputinfo->num_Nways; count++) {
		glPushMatrix();
		glTranslatef(-4.5 + (0.35 * count), 0.0, -3.0);
		glScalef(0.1, 0.4 * vrGetNswitchValueNoLastUpdate(count) + 0.1, 0.1);
		glColor3ub(84, 134, 72);
		vrShapeGLCubePlusOutline();
		glPopMatrix();
	}

	/**********************************************************/
	/** Print some pyramids representing valuator input info **/
	/**********************************************************/
	for (count = 0; count < inputinfo->num_valuators; count++) {
		glPushMatrix();
		glTranslatef(-4.5 + (0.35 * count), 0.0, -2.0);
		glScalef(0.1, 4.0 * vrGetValuatorValueNoLastUpdate(count), 0.1);
		glColor3ub(84, 134, 72);
		vrShapeGLPyramidPlusOutline();
		glPopMatrix();
	}

	/*******************************************************/
	/** Now display boxes from N-sensor[0] (if available) **/
	/*******************************************************/
	if (inputinfo->num_Nsensors > 0) {
		for (count = 0; count < 22; count++) {
			glPushMatrix();
			glTranslatef(-4.5 + (0.35 * count), -1.0, -3.5);
			glScalef(0.1, 4.0 * vrGetNsensorValueNoLastUpdate(0, count), 0.1);
			glColor3ub(84, 72, 134);
			vrShapeGLCube();
			glPopMatrix();
		}
	}

#ifndef LIMITED_TRACE
	vrTrace("draw_inputs", "ending");
#endif
}

#include "vr_math.h"
#include "vr_procs.h"
/*********************************************************************/
void draw_lines(vrRenderInfo *rendinfo)
{
	vrMatrix	head;
	vrPoint		headloc;
	vrPoint		headlocvw;
	vrPoint		proploc;
	vrVector	vec;
	vrPoint		drawto;
	int		count;
	vrInputInfo	*inputinfo;	/* Pointer to the overall input data structure */

	inputinfo = rendinfo->context->input;

#ifndef LIMITED_TRACE
	vrTrace("draw_lines", "doing the lines rendering");
#endif

	if (vrMatrixGetRWFromUserHead(&head, 0) != NULL) {
		vrPointGetRWLocationFromMatrix(&headloc, &head);
		vrPointGetVWFromUserMatrix(&headlocvw, 0, &head);

		/* first store, then adjust some graphics attributes for vector rendering */
		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_LINE_BIT);
		glLineWidth(5.0);
		glDisable(GL_BLEND);

		/* only draw these lines in simulator views & in systems with at least one 6sensor */
		if (rendinfo->window->mount == VRWINDOW_SIMULATOR && inputinfo->num_6sensors >= 1) {
			/* draw a red line connecting the user to the back-low-center of the CAVE */
			drawto.v[VR_X] = 0.0;
			drawto.v[VR_Y] = 0.0;
			drawto.v[VR_Z] = 5.0;
			glColor4ub(255, 0, 0, 255);
			glBegin(GL_LINE_STRIP);
				glVertex3dv(headloc.v);
				glVertex3dv(drawto.v);
			glEnd();

			/* draw a green line pointing out the front of the user */
			vrVectorGetRWFrom6sensorDir(&vec, 0, VRDIR_FORE);
			vrPointAddScaledVector(&drawto, &headloc, &vec, 2.0);
			glColor4ub(0, 255, 0, 255);
			glBegin(GL_LINE_STRIP);
				glVertex3dv(headloc.v);
				glVertex3dv(drawto.v);
			glEnd();

			/* draw a blue line pointing from the user to center of gray pyramid */
			drawto.v[VR_X] = -2.3;
			drawto.v[VR_Y] =  6.2;
			drawto.v[VR_Z] = -0.5;
			vrPointGetRWFromVWUserPoint(&drawto, 0, &drawto);
			glColor4ub(0, 0, 255, 255);
			glBegin(GL_LINE_STRIP);
				glVertex3dv(headloc.v);
				glVertex3dv(drawto.v);
			glEnd();

			glPushMatrix();
			vrRenderTransformUserTravel(rendinfo);	/* now in VW coordinate space */
			/* draw a magenta line pointing out of the user toward the front of the VW */
			/* NOTE: this should be the same as the green line coming from the user's  */
			/*   nose, but in this case we're doing all the calculations in VW space.  */
			vrVectorGetVWFromUser6sensorDir(&vec, 0, 0, VRDIR_FORE);
			vrPointAddScaledVector(&drawto, &headlocvw, &vec, 1.8);
			glColor4ub(255, 0, 255, 255);
			glLineWidth(16.0);
			glBegin(GL_LINE_STRIP);
				glVertex3dv(headlocvw.v);
				glVertex3dv(drawto.v);
			glEnd();
			glLineWidth(5.0);
			glPopMatrix();
		}

		if (inputinfo->num_6sensors >= 2) {
			/* draw a yellow line pointing out the front of the "wand" */
			vrPointGetRWFrom6sensor(&proploc, WAND_SENSOR);
			vrVectorGetRWFrom6sensorDir(&vec, WAND_SENSOR, VRDIR_FORE);
			vrPointAddScaledVector(&drawto, &proploc, &vec, 2.0);
			glColor4ub(255, 255, 0, 255);		/* yellow */
			glBegin(GL_LINE_STRIP);
				glVertex3dv(proploc.v);
				glVertex3dv(drawto.v);
			glEnd();

			/* draw a cyan line pointing up from the "wand" */
			vrVectorGetRWFrom6sensorDir(&vec, WAND_SENSOR, VRDIR_UP);
			vrPointAddScaledVector(&drawto, &proploc, &vec, 1.5);
			glColor4ub(0, 255, 255, 255);		/* cyan */
			glBegin(GL_LINE_STRIP);
				glVertex3dv(proploc.v);
				glVertex3dv(drawto.v);
			glEnd();

		}

		glPopAttrib();

#if 0
		sprintf(buffer, "drawto: (%6.2f %6.2f %6.2f)", drawto.v[VR_X], drawto.v[VR_Y], drawto.v[VR_Z]);
		glRasterPos3f(-6.5,  13.0, -5.01);
		vrRenderText(rendinfo, buffer);
#endif
	}

#ifndef LIMITED_TRACE
	vrTrace("draw_lines", "ending");
#endif
}

/*********************************************************************/
void draw_config_lines(vrRenderInfo *rendinfo)
{
	vrMatrix	head;
	vrPoint		headloc;
	vrPoint		headlocvw;
	vrPoint		proploc;
	vrVector	vec;
	vrPoint		drawto;
	int		count;
	vrInputInfo	*inputinfo;	/* Pointer to the overall input data structure */

	inputinfo = rendinfo->context->input;

#ifndef LIMITED_TRACE
	vrTrace("draw_config_lines", "doing the lines rendering");
#endif

	if (vrMatrixGetRWFromUserHead(&head, 0) != NULL) {
		vrPointGetRWLocationFromMatrix(&headloc, &head);
		vrPointGetVWFromUserMatrix(&headlocvw, 0, &head);

		/* first store, then adjust some graphics attributes for vector rendering */
		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_LINE_BIT);
		glLineWidth(5.0);
		glDisable(GL_BLEND);

		/* only draw these lines in simulator views & in systems with at least one 6sensor */
		if (rendinfo->window->mount == VRWINDOW_SIMULATOR && inputinfo->num_6sensors >= 1) {
			/* draw a red line connecting the user to the front-screen of the CAVE */
			drawto.v[VR_X] = headloc.v[VR_X];
			drawto.v[VR_Y] = headloc.v[VR_Y];
			drawto.v[VR_Z] = 0.0;	/* changed from -5 to 0.0 to be in the center of the CAVE where the IQ-station is */
			glColor4ub(255, 0, 0, 255);
			glBegin(GL_LINE_STRIP);
				glVertex3dv(headloc.v);
				glVertex3dv(drawto.v);
			glEnd();

#if 0
			/* draw a green line pointing out the front of the user */
			vrVectorGetRWFrom6sensorDir(&vec, 0, VRDIR_FORE);
			vrPointAddScaledVector(&drawto, &headloc, &vec, 2.0);
			glColor4ub(0, 255, 0, 255);
			glBegin(GL_LINE_STRIP);
				glVertex3dv(headloc.v);
				glVertex3dv(drawto.v);
			glEnd();

			/* draw a blue line pointing from the user to center of gray pyramid */
			drawto.v[VR_X] = -2.3;
			drawto.v[VR_Y] =  6.2;
			drawto.v[VR_Z] = -0.5;
			vrPointGetRWFromVWUserPoint(&drawto, 0, &drawto);
			glColor4ub(0, 0, 255, 255);
			glBegin(GL_LINE_STRIP);
				glVertex3dv(headloc.v);
				glVertex3dv(drawto.v);
			glEnd();

			glPushMatrix();
			vrRenderTransformUserTravel(rendinfo);	/* now in VW coordinate space */
			/* draw a magenta line pointing out of the user toward the front of the VW */
			/* NOTE: this should be the same as the green line coming from the user's  */
			/*   nose, but in this case we're doing all the calculations in VW space.  */
			vrVectorGetVWFromUser6sensorDir(&vec, 0, 0, VRDIR_FORE);
			vrPointAddScaledVector(&drawto, &headlocvw, &vec, 1.8);
			glColor4ub(255, 0, 255, 255);
			glLineWidth(16.0);
			glBegin(GL_LINE_STRIP);
				glVertex3dv(headlocvw.v);
				glVertex3dv(drawto.v);
			glEnd();
			glLineWidth(5.0);
			glPopMatrix();
#endif
		}

		if (inputinfo->num_6sensors >= 2) {
			/* draw a yellow line pointing out the front of the "wand" */
			vrPointGetRWFrom6sensor(&proploc, WAND_SENSOR);
			vrVectorGetRWFrom6sensorDir(&vec, WAND_SENSOR, VRDIR_FORE);
			vrPointAddScaledVector(&drawto, &proploc, &vec, 2.0);
			glColor4ub(255, 255, 0, 255);		/* yellow */
			glBegin(GL_LINE_STRIP);
				glVertex3dv(proploc.v);
				glVertex3dv(drawto.v);
			glEnd();

			/* draw a cyan line pointing up from the "wand" */
			vrVectorGetRWFrom6sensorDir(&vec, WAND_SENSOR, VRDIR_UP);
			vrPointAddScaledVector(&drawto, &proploc, &vec, 1.5);
			glColor4ub(0, 255, 255, 255);		/* cyan */
			glBegin(GL_LINE_STRIP);
				glVertex3dv(proploc.v);
				glVertex3dv(drawto.v);
			glEnd();

		}

		/* draw cross-hair guides on some of the CAVE-walls */
		glColor3ub((GLbyte)55, (GLbyte)55, (GLbyte)55);	/* gray */
		glLineWidth(2.0);

		/* front wall */
		glBegin(GL_LINE_STRIP);
			glVertex3d(-5.0, 5.0, -5.0);
			glVertex3d( 5.0, 5.0, -5.0);
			glVertex3d( 5.0, 0.0, -5.0);	/* filler point to allow one strip */
			glVertex3d( 0.0, 0.0, -5.0);
			glVertex3d( 0.0,10.0, -5.0);
		glEnd();

		/* floor surface */
		glBegin(GL_LINE_STRIP);
			glVertex3d(-5.0, 0.0,  0.0);
			glVertex3d( 5.0, 0.0,  0.0);
			glVertex3d( 5.0, 0.0,  5.0);	/* filler point to allow one strip */
			glVertex3d( 0.0, 0.0,  5.0);
			glVertex3d( 0.0, 0.0, -5.0);
		glEnd();

		/* left wall */
		glBegin(GL_LINE_STRIP);
			glVertex3d(-5.0, 5.0, -5.0);
			glVertex3d(-5.0, 5.0,  5.0);
			glVertex3d(-5.0, 0.0,  5.0);	/* filler point to allow one strip */
			glVertex3d(-5.0, 0.0,  0.0);
			glVertex3d(-5.0,10.0,  0.0);
		glEnd();

		/* right wall */
		glBegin(GL_LINE_STRIP);
			glVertex3d( 5.0, 5.0, -5.0);
			glVertex3d( 5.0, 5.0,  5.0);
			glVertex3d( 5.0, 0.0,  5.0);	/* filler point to allow one strip */
			glVertex3d( 5.0, 0.0,  0.0);
			glVertex3d( 5.0,10.0,  0.0);
		glEnd();

		glPopAttrib();
	}

#ifndef LIMITED_TRACE
	vrTrace("draw_config_lines", "ending");
#endif
}


/*********************************************************************/
/**************************/
/** Print some text info **/
/**************************/
void draw_text(vrRenderInfo *rendinfo)
{
	int		count;
	char		buffer[2048];	/* this could be smaller, but is now safer against      */
					/*   memory overflow bugs.  TODO: replace sprintf's     */
					/*   with snprintf when that becomes ubiquitous -- it's */
					/*   not in IRIX 6.2                                    */
	vrTime		beginRenderTime = vrCurrentWallTime();/* Time when we begin the rendering process */
	vrTime		renderTime;	/* amount of time spent rendering */
	vrMatrix	head;
	vrProcessInfo	*input_proc;
	vrInputInfo	*inputinfo;	/* Pointer to the overall input data structure */

	inputinfo = rendinfo->context->input;

#ifndef LIMITED_TRACE
	vrTrace("draw_text", "doing the text rendering");
#endif

	/* first store, then adjust some graphics attributes for text rendering */
	/*      disregard z-buffer -- always display the text on top */
	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	/* set the text color */
	glColor3ub(155, 155, 155);


	/* render on left wall */
	if (inputinfo->num_6sensors >= 1) {
		input_proc = inputinfo->sensor6[0]->my_device->proc;
#if 0
		sprintf(buffer, "input frame: %11d", input_proc->frame_count);
		glRasterPos3f(-5.0,  8.5, 1.5);
		vrRenderText(rendinfo, buffer);

		sprintf(buffer, "input rates: %5.2f(1) %5.2f(10)", input_proc->fps1, input_proc->fps10);
		glRasterPos3f(-5.0,  8.0, 1.5);
		vrRenderText(rendinfo, buffer);

		sprintf(buffer, "render frame: %10d", rendinfo->frame_count);
		glRasterPos3f(-5.0,  7.5, 1.5);
		vrRenderText(rendinfo, buffer);

		sprintf(buffer, "render rates: %5.2f(1) %5.2f(10)", vrThisProc->fps1, vrThisProc->fps10);
		glRasterPos3f(-5.0,  7.0, 1.5);
		vrRenderText(rendinfo, buffer);
#else
		sprintf(buffer, "Shared memory usage:%7ld (%ld freed)", vrShmemUsage(), vrShmemFreed());
		glRasterPos3f(-5.0,  8.0, 1.75);
		vrRenderText(rendinfo, buffer);

		sprintf(buffer, "input info:%9ld %6.2f(1) %6.2f(10)",
			input_proc->frame_count, input_proc->fps1, input_proc->fps10);
		glRasterPos3f(-5.0,  7.5, 1.75);
		vrRenderText(rendinfo, buffer);

		sprintf(buffer, "render info:%8ld %6.2f(1) %6.2f(10)",
			rendinfo->frame_count, vrThisProc->fps1, vrThisProc->fps10);
		glRasterPos3f(-5.0,  7.0, 1.75);
		vrRenderText(rendinfo, buffer);
#endif
	}

	/* render on front wall */
#if 0
	sprintf(buffer, "$YO = '%s' sim time: %8.4f", getenv("YO"), (double)(rendinfo->frame_stime));
#else
	sprintf(buffer, "sim time: %8.4f", (double)(rendinfo->frame_stime));
#endif
	glRasterPos3f(-4.5,  9.0, -5.01);
	vrRenderText(rendinfo, buffer);

	glRasterPos3f(-4.5,  7.0, -5.01);
	sprintf(buffer, "buttons:");
	for (count = 0; count < inputinfo->num_2ways; count++)
		sprintf(buffer, "%s %d", buffer, vrGet2switchValueNoLastUpdate(count));
	vrRenderText(rendinfo, buffer);

	if (inputinfo->num_valuators > 0) {
		sprintf(buffer, "valuators:");
		for (count = 0; count < inputinfo->num_valuators; count++)
			sprintf(buffer, "%s %6.3f", buffer, vrGetValuatorValueNoLastUpdate(count));
		glRasterPos3f(-4.5,  6.0, -5.01);
		vrRenderText(rendinfo, buffer);
	}

	if (inputinfo->num_Nways > 0) {
		glRasterPos3f(-4.5,  6.5, -5.01);
		sprintf(buffer, "nswitches:");
		for (count = 0; count < inputinfo->num_Nways; count++)
			sprintf(buffer, "%s %d", buffer, vrGetNswitchValueNoLastUpdate(count));
		vrRenderText(rendinfo, buffer);
	}

	if (vrMatrixGetRWFromUserHead(&head, 0) != NULL) {
		vrPoint	*leye = &(vrRenderCurrentEye(rendinfo)->loc);

		sprintf(buffer, "head: %5.2f %5.2f %5.2f", VRMAT_ROWCOL(&head, VR_X, VR_W), VRMAT_ROWCOL(&head, VR_Y, VR_W), VRMAT_ROWCOL(&head, VR_Z, VR_W));
		glRasterPos3f(-5.0,  5.0,  4.50);
		vrRenderText(rendinfo, buffer);
		glRasterPos3f(-4.5,  5.0, -5.01);	/* ditto this to left wall */
		vrRenderText(rendinfo, buffer);

		if (rendinfo->eye->type == VREYE_LEFT) {
			sprintf(buffer, "leye: %5.2f %5.2f %5.2f", leye->v[VR_X], leye->v[VR_Y], leye->v[VR_Z]);
			glRasterPos3f(-4.5,  3.0, -5.01);
			vrRenderText(rendinfo, buffer);
		}

		if (rendinfo->eye->type == VREYE_LEFT) {
			/* at/near the front wall */
			glRasterPos3f(-4.5,  4.0, -5.01);
			vrRenderText(rendinfo, "Left Eye");

			/* at/near the left wall */
			glRasterPos3f(-4.49,  4.0, 0.5);
			vrRenderText(rendinfo, "Left Eye");

			/* at/near the right wall */
			glRasterPos3f( 4.49,  4.0, -0.5);
			vrRenderText(rendinfo, "Left Eye");

			/* at/near the floor wall */
			glRasterPos3f( -0.5,  0.01, 0.0);
			vrRenderText(rendinfo, "Left Eye");
		} else {
			/* at/near the front wall */
			glRasterPos3f(-4.0,  4.0, -5.01);
			vrRenderText(rendinfo, "Right Eye");

			/* at/near the left wall */
			glRasterPos3f(-4.49,  4.0, 0.0);
			vrRenderText(rendinfo, "Right Eye");

			/* at/near the right wall */
			glRasterPos3f( 4.49,  4.0, 0.0);
			vrRenderText(rendinfo, "Right Eye");

			/* at/near the floor wall */
			glRasterPos3f( -0.0,  0.01, 0.0);
			vrRenderText(rendinfo, "Right Eye");
		}
	} else {
		sprintf(buffer, "no head configured.");
		glRasterPos3f(-4.5,  2.0, -5.01);
		vrRenderText(rendinfo, buffer);
	}

	if (inputinfo->num_6sensors >= 1) {
		vrMatrix	tmp_matrix;

		vrMatrixGet6sensorValues(&tmp_matrix, 0);
		sprintf(buffer, "%csensor6[0]: %5.2f %5.2f %5.2f",
			(vrGet6sensorOobValue(0) ? ' ' : '>'),
			VRMAT_ROWCOL(&tmp_matrix, VR_X, VR_W), VRMAT_ROWCOL(&tmp_matrix, VR_Y, VR_W), VRMAT_ROWCOL(&tmp_matrix, VR_Z, VR_W));
		glRasterPos3f(-8.5,  1.0, -5.01);
		vrRenderText(rendinfo, buffer);
	}

	if (inputinfo->num_6sensors >= 2) {
#if 0
		sprintf(buffer, "%csensor6[1]: %5.2f %5.2f %5.2f",
			(inputinfo->sensor6[1]->oob ? ' ' : '>'),
			VRMAT_ROWCOL(inputinfo->sensor6[1]->position, VR_X, VR_W),
			VRMAT_ROWCOL(inputinfo->sensor6[1]->position, VR_Y, VR_W),
			VRMAT_ROWCOL(inputinfo->sensor6[1]->position, VR_Z, VR_W));
#else
	/* temporarily change this for a test on how to calculate a point relative to another sensor's coordinate system */
		vrMatrix	head_mat;
		vrMatrix	head_invert;
		vrPoint		wand_point;
		vrPoint		relwand;

		vrMatrixGet6sensorValues(&head_mat, HEAD_SENSOR);
		vrPointGetRWFrom6sensor(&wand_point, WAND_SENSOR);
		vrMatrixInvert(&head_invert, &head_mat);
		vrPointTransformByMatrix(&relwand, &wand_point, &head_invert);

		sprintf(buffer, "wand rel to head: %5.2f %5.2f %5.2f", relwand.v[VR_X], relwand.v[VR_Y], relwand.v[VR_Z]);
#endif


		glRasterPos3f(-8.5,  0.5, -5.01);
		vrRenderText(rendinfo, buffer);
	}

	if (inputinfo->num_6sensors >= 3) {
		sprintf(buffer, "%csensor6[2]: %5.2f %5.2f %5.2f",
			(inputinfo->sensor6[2]->oob ? ' ' : '>'),
			VRMAT_ROWCOL(inputinfo->sensor6[2]->position, VR_X, VR_W),
			VRMAT_ROWCOL(inputinfo->sensor6[2]->position, VR_Y, VR_W),
			VRMAT_ROWCOL(inputinfo->sensor6[2]->position, VR_Z, VR_W));

		glRasterPos3f(-8.5,  0.0, -5.01);
		vrRenderText(rendinfo, buffer);
	}

	glPopAttrib();

	renderTime = vrCurrentWallTime() - beginRenderTime;

	/* set the text color */
	glColor3ub(155, 155, 155);

	/* just print how long it takes to render */
	sprintf(buffer, "render time: %8.8f", (float)(renderTime));
	glRasterPos3f(-4.5,  8.0, -5.01);
	vrRenderText(rendinfo, buffer);

#ifndef LIMITED_TRACE
	vrTrace("draw_text", "ending");
#endif
}

/***************************************************/
/** Print some text info for the "inputs" program **/
/***************************************************/
void draw_inputs_text(vrRenderInfo *rendinfo)
{
	int		count;
	char		buffer[2048];	/* this could be smaller, but is now safer against      */
					/*   memory overflow bugs.  TODO: replace sprintf's     */
					/*   with snprintf when that becomes ubiquitous -- it's */
					/*   not in IRIX 6.2                                    */
	vrTime		beginRenderTime = vrCurrentWallTime();/* Time when we begin the rendering process */
	vrTime		renderTime;	/* amount of time spent rendering */
	vrMatrix	head;
	vrProcessInfo	*input_proc;
	vrInputInfo	*inputinfo;	/* Pointer to the overall input data structure */

	inputinfo = rendinfo->context->input;

#ifndef LIMITED_TRACE
	vrTrace("draw_inputs_text", "doing the text rendering");
#endif

	/* first store, then adjust some graphics attributes for text rendering */
	/*      disregard z-buffer -- always display the text on top */
	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	/* set the text color */
	glColor3ub((GLbyte)155, (GLbyte)155, (GLbyte)155);	/* gray */


	/* render on front wall */
	glRasterPos3f(-4.5,  8.5, -5.01);
	sprintf(buffer, "system: %s", rendinfo->context->config->system_name);
	vrRenderText(rendinfo, buffer);

	glRasterPos3f(-4.5,  7.5, -5.01);
	sprintf(buffer, "buttons:");
	for (count = 0; count < inputinfo->num_2ways; count++)
		sprintf(buffer, "%s %d", buffer, vrGet2switchValueNoLastUpdate(count));
	vrRenderText(rendinfo, buffer);

	if (inputinfo->num_valuators > 0) {
		sprintf(buffer, "valuators:");
		for (count = 0; count < inputinfo->num_valuators; count++)
			sprintf(buffer, "%s %6.3f", buffer, vrGetValuatorValueNoLastUpdate(count));
		glRasterPos3f(-4.5,  6.5, -5.01);
		vrRenderText(rendinfo, buffer);
	}

	if (inputinfo->num_Nways > 0) {
		glRasterPos3f(-4.5,  7.0, -5.01);
		sprintf(buffer, "nswitches:");
		for (count = 0; count < inputinfo->num_Nways; count++)
			sprintf(buffer, "%s %d", buffer, vrGetNswitchValueNoLastUpdate(count));
		vrRenderText(rendinfo, buffer);
	}

#if 0
	if (vrMatrixGetRWFromUserHead(&head, 0) != NULL) {
		vrPoint	*leye = &(vrRenderCurrentEye(rendinfo)->loc);

		sprintf(buffer, "head: %5.2f %5.2f %5.2f", VRMAT_ROWCOL(&head, VR_X, VR_W), VRMAT_ROWCOL(&head, VR_Y, VR_W), VRMAT_ROWCOL(&head, VR_Z, VR_W));
		glRasterPos3f(-5.0,  5.0,  4.50);
		vrRenderText(rendinfo, buffer);
	} else {
		sprintf(buffer, "no head configured.");
		glRasterPos3f(-4.5,  2.0, -5.01);
		vrRenderText(rendinfo, buffer);
	}
#endif

	if (inputinfo->num_6sensors > 0) {
		vrMatrix	tmp_matrix;
		vrEuler		r2e_euler;

		sprintf(buffer, "6-sensors:");
		for (count = 0; count < inputinfo->num_6sensors; count++) {
			vrMatrixGet6sensorValues(&tmp_matrix, count);
			sprintf(buffer, "%csensor6[%d]: %5.2f %5.2f %5.2f",
				(vrGet6sensorOobValue(count) ? '!' : ' '), count,
				VRMAT_ROWCOL(&tmp_matrix, VR_X, VR_W), VRMAT_ROWCOL(&tmp_matrix, VR_Y, VR_W), VRMAT_ROWCOL(&tmp_matrix, VR_Z, VR_W));

#if 1 /* add some additional info for the new "config" program */
			vrEulerSetFromMatrix(&r2e_euler, inputinfo->sensor6[count]->r2e_xform);
			sprintf(buffer, "%s  r2e: %.2f %.2f %.2f  %.2f %.2f %.2f", buffer, r2e_euler.t[VR_X], r2e_euler.t[VR_Y], r2e_euler.t[VR_Z], r2e_euler.r[VR_AZIM], r2e_euler.r[VR_ELEV], r2e_euler.r[VR_ROLL]);
#endif

			/* Set the text color based on the state */
			if (vrGet6sensorActiveValue(count))
				glColor3ub((GLbyte)0, (GLbyte)230, (GLbyte)0);	/* green */
			if (vrGet6sensorOobValue(count))
				glColor3ub((GLbyte)0, (GLbyte)0, (GLbyte)230);	/* blue */
			if (vrGet6sensorErrorValue(count))
				glColor3ub((GLbyte)230, (GLbyte)0, (GLbyte)0);	/* red */
			if (vrGet6sensorDummyValue(count))
				glColor3ub((GLbyte)252, (GLbyte)206, (GLbyte)100);	/* yellow */

			glRasterPos3f(-4.5,  5.5 - (float)(count), -5.01);
			vrRenderText(rendinfo, buffer);
			glColor3ub((GLbyte)155, (GLbyte)155, (GLbyte)155);	/* return to gray */
		}
#if 1 /* more additional info for the new "config" program */
		vrEulerSetFromMatrix(&r2e_euler, inputinfo->sensor6[0]->my_device->t2rw_xform);
		sprintf(buffer, "t2rw: %.2f %.2f %.2f  %.2f %.2f %.2f", r2e_euler.t[VR_X], r2e_euler.t[VR_Y], r2e_euler.t[VR_Z], r2e_euler.r[VR_AZIM], r2e_euler.r[VR_ELEV], r2e_euler.r[VR_ROLL]);
		glRasterPos3f(-4.5,  5.5 - (float)(count), -5.01);
		vrRenderText(rendinfo, buffer);
#endif
	}

	glPopAttrib();

	renderTime = vrCurrentWallTime() - beginRenderTime;

	/* set the text color */
	glColor3ub(155, 155, 155);

#ifndef LIMITED_TRACE
	vrTrace("draw_text", "ending");
#endif
}

