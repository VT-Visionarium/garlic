/* ======================================================================
 *
 *  CCCCC          simple.c
 * CC   CC         Author(s): Sukru Tikves
 * CC              Created: July 31, 2002
 * CC   CC         Last Modified: September 9, 2002
 *  CCCCC
 *
 * Code file for the simplest possible sample VR application.
 *
 * Copyright 2002, Bill Sherman & Friends, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#include <GL/gl.h>
#include <GL/glu.h>
#include "freevr.h"

#include <stdio.h>
#include <math.h>

int		*data;
GLUquadricObj	*quadObj;


/********************************************************************/
void initProc(void)
{
	printf("SIMPLE.C: initProc() called.\n");

	quadObj = gluNewQuadric();
	gluQuadricDrawStyle(quadObj, GLU_LINE);

	glEnable(GL_DEPTH_TEST);
}


/********************************************************************/
void renderProc(void)
{
static	double	angle = 0.0;
static	int	called = 0;
	double	x, y;

	if(!called) {
		called = 1;
		printf("SIMPLE.C: renderProc() called.\n");
	}

	angle += 0.1;

	x = 0.25 * cos(angle);
	y = 0.25 * sin(angle);

	glClearColor(x, x + y, y, 1.0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	glColor3f(1.0f, 1.0f, 1.0f);
	gluSphere(quadObj, 0.5, 16, 16);
}


/********************************************************************/
int main(int argc, char *argv[])
{
	vrLock lock;

	vrConfigure(&argc, argv, NULL);

	data = vrShmemAlloc(sizeof(int));
	*data = 0;

	printf("SIMPLE.C: main.data = %d\n", *data);

	vrSetFunc(VRFUNC_ALL_DISPLAY_INIT, vrCreateCallback(initProc, 0));
	vrSetFunc(VRFUNC_ALL_DISPLAY, vrCreateCallback(renderProc, 0));

	vrStart();

	printf("SIMPLE.C: Initialization complete\n");
	while (*data < 5) {
		(*data)++;
		printf("Value = %d\n", vrGet2switchValue(0));
		sleep(1);
	}

	vrExit();

	return 0;
}

