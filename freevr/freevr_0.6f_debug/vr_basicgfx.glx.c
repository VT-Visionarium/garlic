/* ======================================================================
 *
 *  CCCCC          vr_basicgfx.glx.c
 * CC   CC         Author(s): Bill Sherman, John Stone
 * CC              Created: November 30, 1999
 * CC   CC         Last Modified: July 11, 2013
 *  CCCCC
 *
 * Code file for FreeVR basic graphics routines.
 *
 * NOTE: these routines make some assumptions about the current
 *   graphics rendering state.  Namely:
 *	- counterclockwise ordering of vertices designates the front face of a polygon
 *      ??? lighting
 *	??? blending enable/function
 * Also, the outline lines will be rendered with current linewidth
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#include <GL/gl.h>

#include <stdio.h>
#include "vr_debug.h"
#include "vr_basicgfx.glx.h"
#include "vr_input.h"		/* for input sensing functions in vrGLRenderDefaultSimultor() */
#include "vr_objects.h"		/* for use of call to vrObjectFirst() in vrGLRenderDefaultSimultor() */


/****************************************************************************/
/** code from Bill's shapes.c                                              **/
/****************************************************************************/
/*   Cube (and pyramid) vertices.  Labeled 0-7 (cube) & 8 (top of pyramid). */
/*                                                                          */
/*                                                                          */
/*             7_______ 4                                                   */
/*             /|     /|        (+y)                                        */
/*            / | .8 / |         |                                          */
/*          0/__|___/3 |         |                                          */
/*           | 6|___|__|5        o---- (+x)                                 */
/*           | /    | /         /                                           */
/*           |/     |/         /                                            */
/*          1|______|2       (+z)                                           */
/*                                                                          */
/****************************************************************************/



static float cube_vertices[9][3] = {			/* specifies the (x,y,z)   */
		{ -1.0,  1.0,  1.0 },	/* 0 */		/* location of each vertex */
		{ -1.0, -1.0,  1.0 },	/* 1 */
		{  1.0, -1.0,  1.0 },	/* 2 */
		{  1.0,  1.0,  1.0 },	/* 3 */
		{  1.0,  1.0, -1.0 },	/* 4 */
		{  1.0, -1.0, -1.0 },	/* 5 */
		{ -1.0, -1.0, -1.0 },	/* 6 */
		{ -1.0,  1.0, -1.0 },	/* 7 */
		{  0.0,  1.0,  0.0 },	/* 8 */
	};

static int cube_polygons[6][4] = {			/* specifies the vertices */
		{ 0, 1, 2, 3 },	/* front  */		/* of each polygon        */
		{ 3, 2, 5, 4 },	/* right  */
		{ 5, 6, 7, 4 },	/* back   */
		{ 7, 6, 1, 0 },	/* left   */
		{ 6, 5, 2, 1 },	/* bottom */
		{ 0, 3, 4, 7 },	/* top    */
	};

static float cube_normals[6][3] = {			/* specifies the normal */
		{  0.0,  0.0,  1.0 }, /* front  */	/* of each polygon      */
		{  1.0,  0.0,  0.0 }, /* right  */
		{  0.0,  0.0, -1.0 }, /* back   */
		{ -1.0,  0.0,  0.0 }, /* left   */
		{  0.0, -1.0,  0.0 }, /* bottom */
		{  0.0,  1.0,  0.0 }, /* top    */
	};

static int pyramid_polygons[5][4] = {			/* specifies the vertices */
		{ 1, 2, 8 },	/* front  */		/* of each polygon        */
		{ 2, 5, 8 },	/* right  */
		{ 5, 6, 8 },	/* back   */
		{ 6, 1, 8 },	/* left   */
		{ 6, 5, 2, 1 },	/* bottom */
	};

static float pyramid_normals[5][3] = {			/* specifies the normal    */
		{  0.0,  0.0,  1.0 }, /* front  */	/* of each polygon -- only */
		{  1.0,  0.0,  0.0 }, /* right  */	/* the bottom normal is    */
		{  0.0,  0.0, -1.0 }, /* back   */	/* perpendicular to the    */
		{ -1.0,  0.0,  0.0 }, /* left   */	/* surface.                */
		{  0.0, -1.0,  0.0 }, /* bottom */
	};


/*********************************************************************/
void vrShapeGLCube()
{
	int	poly;

	for (poly = 0; poly < 6; poly++) {
		glBegin(GL_POLYGON);
			glNormal3fv(cube_normals[poly]);
			glVertex3fv(cube_vertices[cube_polygons[poly][0]]);
			glVertex3fv(cube_vertices[cube_polygons[poly][1]]);
			glVertex3fv(cube_vertices[cube_polygons[poly][2]]);
			glVertex3fv(cube_vertices[cube_polygons[poly][3]]);
		glEnd();
	}
}


/*********************************************************************/
void vrShapeGLCubeOpen()
{
	int	poly;

	for (poly = 0; poly < 6; poly++) {
		if (poly != 0) {
			glBegin(GL_POLYGON);
				glNormal3fv(cube_normals[poly]);
				glVertex3fv(cube_vertices[cube_polygons[poly][0]]);
				glVertex3fv(cube_vertices[cube_polygons[poly][1]]);
				glVertex3fv(cube_vertices[cube_polygons[poly][2]]);
				glVertex3fv(cube_vertices[cube_polygons[poly][3]]);
			glEnd();
		}
	}
}


/*********************************************************************/
void vrShapeGLCubeOutline()
{
	glPushAttrib(GL_COLOR_BUFFER_BIT);
	glDisable(GL_BLEND);
	glBegin(GL_LINE_STRIP);
		glVertex3fv(cube_vertices[0]);
		glVertex3fv(cube_vertices[1]);
		glVertex3fv(cube_vertices[2]);
		glVertex3fv(cube_vertices[3]);
		glVertex3fv(cube_vertices[0]);
		glVertex3fv(cube_vertices[7]);
		glVertex3fv(cube_vertices[6]);
		glVertex3fv(cube_vertices[1]);
		glVertex3fv(cube_vertices[2]);
		glVertex3fv(cube_vertices[5]);
		glVertex3fv(cube_vertices[6]);
		glVertex3fv(cube_vertices[7]);
		glVertex3fv(cube_vertices[4]);
		glVertex3fv(cube_vertices[3]);
		glVertex3fv(cube_vertices[4]);
		glVertex3fv(cube_vertices[5]);
	glEnd();
	glPopAttrib();
}


/*********************************************************************/
void vrShapeGLCubePlusOutline()
{

	vrShapeGLCube();
	glColor4ub((GLbyte)0, (GLbyte)0, (GLbyte)0, (GLbyte)255);	/* black */
#if 0 /* I don't know if the "glDisable(GL_BLEND);" is undesireable or not */
	glBegin(GL_LINE_STRIP);
		glVertex3fv(cube_vertices[0]);
		glVertex3fv(cube_vertices[1]);
		glVertex3fv(cube_vertices[2]);
		glVertex3fv(cube_vertices[3]);
		glVertex3fv(cube_vertices[0]);
		glVertex3fv(cube_vertices[7]);
		glVertex3fv(cube_vertices[6]);
		glVertex3fv(cube_vertices[1]);
		glVertex3fv(cube_vertices[2]);
		glVertex3fv(cube_vertices[5]);
		glVertex3fv(cube_vertices[6]);
		glVertex3fv(cube_vertices[7]);
		glVertex3fv(cube_vertices[4]);
		glVertex3fv(cube_vertices[3]);
		glVertex3fv(cube_vertices[4]);
		glVertex3fv(cube_vertices[5]);
	glEnd();
#else
	vrShapeGLCubeOutline();
#endif
}


/*********************************************************************/
void vrShapeGLPyramid()
{
	int	poly;

	for (poly = 0; poly < 5; poly++) {
		glBegin(GL_POLYGON);
			glNormal3fv(pyramid_normals[poly]);
			glVertex3fv(cube_vertices[pyramid_polygons[poly][0]]);
			glVertex3fv(cube_vertices[pyramid_polygons[poly][1]]);
			glVertex3fv(cube_vertices[pyramid_polygons[poly][2]]);
			if (poly == 4)
				glVertex3fv(cube_vertices[pyramid_polygons[poly][3]]);
		glEnd();
	}
}


/*********************************************************************/
void vrShapeGLPyramidOutline()
{
	glPushAttrib(GL_COLOR_BUFFER_BIT);
	glDisable(GL_BLEND);
	glBegin(GL_LINE_STRIP);
		glVertex3fv(cube_vertices[1]);
		glVertex3fv(cube_vertices[8]);
		glVertex3fv(cube_vertices[2]);
		glVertex3fv(cube_vertices[1]);
		glVertex3fv(cube_vertices[6]);
		glVertex3fv(cube_vertices[8]);
		glVertex3fv(cube_vertices[5]);
		glVertex3fv(cube_vertices[2]);
		glVertex3fv(cube_vertices[5]);
		glVertex3fv(cube_vertices[6]);
	glEnd();
	glPopAttrib();
}


/*********************************************************************/
void vrShapeGLPyramidPlusOutline()
{

	vrShapeGLPyramid();
	glColor3ub((GLbyte)0, (GLbyte)0, (GLbyte)0);		/* black */
	vrShapeGLPyramidOutline();
}


/*******************************************************************/
/* vrShapeGLFloor(): draws a floor especially designed for a 10x10 */
/*   operating area.                                               */
/*******************************************************************/
void vrShapeGLFloor()
{
	glColor3ub((GLbyte)25, (GLbyte)25, (GLbyte)200);	/* blue */
	glBegin(GL_POLYGON);
		glVertex3f(-5.0, 0.0, -5.0);
		glVertex3f( 5.0, 0.0, -5.0);
		glVertex3f( 5.0, 0.0,  5.0);
		glVertex3f(-5.0, 0.0,  5.0);
	glEnd();
}


/***************************************************************************/
/* vrGLReportAttributes(): Report the current graphics parameters related  */
/*   to the current color and blending and alpha constructs.               */
/* TODO: add values for glColorMaterial(GL_FRONT, GL_DIFFUSE) & glEnable(GL_COLOR_MATERIAL) */
/* TODO: there are several more texture values that can be reported */
void vrGLReportAttributes(char *msg, unsigned long reports)
{
static	int	header_count = 0;
	int	header_repeat = 30;
	int	report_color = reports & 0x01,
		report_blend = reports & 0x02,
		report_logic = reports & 0x04,
		report_alpha = reports & 0x08,
		report_lights = reports & 0x10,
		report_polys = reports & 0x20,
		report_texture = reports & 0x40,
		report_client_state = reports & 0x80;
	int	rgba_mode;
	float	color[4];
	int	blend_en,
		blend_dst,
		blend_src,
		blend_equ;
	float	blend_clr[4];
	int	logic_mode,
		logic_clr,
		logic_ind;
	int	alpha_en,
		alpha_func;
	float	alpha_ref,
		alpha_bias,
		alpha_scale;
	int	light_en,
		light0_en,
		light1_en,
		light2_en,
		light3_en,
		light4_en,
		light5_en,
		light6_en,
		light7_en,
		light_viewer,
		light_2side;
	float	light_ambclr[4];
	int	poly_cull,
		poly_side,
		poly_front,
		poly_modes[2],
		poly_stipple,
		poly_smooth;
	int	tex_en[3];		/* GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D_EXT */
	int	tex_bind[3];
	int	tex_gen[4];		/* GL_TEXTURE_GEN_Q, R, S, T */
	int	tex_map1coords[4];	/* GL_MAP1_TEXTURE_COORD_1, etc */
	int	tex_map2coords[4];	/* GL_MAP2_TEXTURE_COORD_1, etc */
					/* also perhaps GL_POST_TEXTURE_FILTER_BIAS_RANGE_SGIX */
					/* also perhaps GL_POST_TEXTURE_FILTER_SCALE_RANGE_SGIX */
					/* and so on, there are several more texture values */
	int	cstate_colorarray,
		cstate_eflagarray,
		cstate_indexarray,
		cstate_normalarray,
		cstate_tcoordarray,
		cstate_vertexarray;

	/* display nothing if nothing to report */
	if (reports == 0)
		return;

	/*************************/
	/* get the current color */
	if (report_color) {
		glGetIntegerv(GL_RGBA_MODE, &rgba_mode);
		glGetFloatv(GL_CURRENT_COLOR, color);

	/* TODO: add values for glColorMaterial(GL_FRONT, GL_DIFFUSE) & glEnable(GL_COLOR_MATERIAL) */
	}

	/***************************/
	/* get the blending values */
	if (report_blend) {
		glGetIntegerv(GL_BLEND, &blend_en);
		glGetIntegerv(GL_BLEND_DST, &blend_dst);
		glGetIntegerv(GL_BLEND_SRC, &blend_src);
		glGetIntegerv(GL_BLEND_EQUATION_EXT, &blend_equ);
#if defined(GL_BLEND_COLOR_EXT)
                /* TODO: this needs to be improved... JS */
		glGetFloatv(GL_BLEND_COLOR_EXT, blend_clr);
#endif
	}

	/****************************************/
	/* get the pixel logic operation values */
	if (report_logic) {
		glGetIntegerv(GL_LOGIC_OP_MODE, &logic_mode);
		logic_clr = glIsEnabled(GL_COLOR_LOGIC_OP);
		logic_ind = glIsEnabled(GL_INDEX_LOGIC_OP);
	}

	/**********************************/
	/* get the alpha operation values */
	if (report_alpha) {
		alpha_en = glIsEnabled(GL_ALPHA_TEST);
		glGetIntegerv(GL_ALPHA_TEST_FUNC, &alpha_func);
		glGetFloatv(GL_ALPHA_TEST_REF, &alpha_ref);
		glGetFloatv(GL_ALPHA_BIAS, &alpha_bias);
		glGetFloatv(GL_ALPHA_SCALE, &alpha_scale);
	}

	/*************************************/
	/* get the lighting operation values */
	if (report_lights) {
		light_en = glIsEnabled(GL_LIGHTING);
		light0_en = glIsEnabled(GL_LIGHT0);
		light1_en = glIsEnabled(GL_LIGHT1);
		light2_en = glIsEnabled(GL_LIGHT2);
		light3_en = glIsEnabled(GL_LIGHT3);
		light4_en = glIsEnabled(GL_LIGHT4);
		light5_en = glIsEnabled(GL_LIGHT5);
		light6_en = glIsEnabled(GL_LIGHT6);
		light7_en = glIsEnabled(GL_LIGHT7);
		glGetFloatv(GL_LIGHT_MODEL_AMBIENT, light_ambclr);
		glGetIntegerv(GL_LIGHT_MODEL_LOCAL_VIEWER, &light_viewer);
		glGetIntegerv(GL_LIGHT_MODEL_TWO_SIDE, &light_2side);
	}

	/************************************/
	/* get the polygon rendering values */
	if (report_polys) {
		poly_cull = glIsEnabled(GL_CULL_FACE);
		glGetIntegerv(GL_CULL_FACE_MODE, &poly_side);	/* == GL_BACK, GL_FRONT, GL_FRONT_AND_BACK */
		glGetIntegerv(GL_FRONT_FACE, &poly_front);	/* == GL_CCW, GL_CW */
		glGetIntegerv(GL_POLYGON_MODE, poly_modes);	/* == GL_FILL, GL_POINT, GL_LINE*/
		poly_stipple = glIsEnabled(GL_POLYGON_STIPPLE);
		poly_smooth = glIsEnabled(GL_POLYGON_SMOOTH);
	}

	/********************************/
	/* get the current texture info */
	if (report_texture) {
		tex_en[0] = glIsEnabled(GL_TEXTURE_1D);
		tex_en[1] = glIsEnabled(GL_TEXTURE_2D);
#if defined(GL_EXT_texture3D)
		tex_en[2] = glIsEnabled(GL_TEXTURE_3D_EXT);
#else
		tex_en[2] = GL_FALSE;
#endif


#if defined(GL_VERSION_1_1) || defined(GL_VERSION_1_2)
		glGetIntegerv(GL_TEXTURE_BINDING_1D, &(tex_bind[0]));
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &(tex_bind[1]));
#  if 0
                /* TODO: still need to implement a better check or fallback
                 *       for 3D texture stuff - JS
                 */
		glGetIntegerv(GL_TEXTURE_BINDING_3D, &(tex_bind[2]));
#  else
                tex_bind[2] = GL_FALSE;
#  endif

#elif defined(GL_EXT_texture_object)
		glGetIntegerv(GL_TEXTURE_1D_BINDING_EXT, &(tex_bind[0]));
		glGetIntegerv(GL_TEXTURE_2D_BINDING_EXT, &(tex_bind[1]));
#  if defined(GL_TEXTURE_3D_BINDING_EXT)
		glGetIntegerv(GL_TEXTURE_3D_BINDING_EXT, &(tex_bind[2]));
#  else
                tex_bind[2] = GL_FALSE;
#  endif

#else
                tex_bind[0] = GL_FALSE;
                tex_bind[1] = GL_FALSE;
                tex_bind[2] = GL_FALSE;
#endif

		tex_gen[0] = glIsEnabled(GL_TEXTURE_GEN_Q);
		tex_gen[1] = glIsEnabled(GL_TEXTURE_GEN_R);
		tex_gen[2] = glIsEnabled(GL_TEXTURE_GEN_S);
		tex_gen[3] = glIsEnabled(GL_TEXTURE_GEN_T);

		tex_map1coords[0] = glIsEnabled(GL_MAP1_TEXTURE_COORD_1);
		tex_map1coords[1] = glIsEnabled(GL_MAP1_TEXTURE_COORD_2);
		tex_map1coords[2] = glIsEnabled(GL_MAP1_TEXTURE_COORD_3);
		tex_map1coords[3] = glIsEnabled(GL_MAP1_TEXTURE_COORD_4);

		tex_map2coords[0] = glIsEnabled(GL_MAP2_TEXTURE_COORD_1);
		tex_map2coords[1] = glIsEnabled(GL_MAP2_TEXTURE_COORD_2);
		tex_map2coords[2] = glIsEnabled(GL_MAP2_TEXTURE_COORD_3);
		tex_map2coords[3] = glIsEnabled(GL_MAP2_TEXTURE_COORD_4);
	}

	/********************************/
	/* get the current texture info */
	if (report_client_state) {
		cstate_colorarray = glIsEnabled(GL_COLOR_ARRAY);
		cstate_eflagarray = glIsEnabled(GL_EDGE_FLAG_ARRAY);
		cstate_indexarray = glIsEnabled(GL_INDEX_ARRAY);
		cstate_normalarray = glIsEnabled(GL_NORMAL_ARRAY);
		cstate_tcoordarray = glIsEnabled(GL_TEXTURE_COORD_ARRAY);
		cstate_vertexarray = glIsEnabled(GL_VERTEX_ARRAY);
	}


	/********************/
	/* print the header */
	/********************/
	if (header_count == 0) {
		vrPrintf("Message  |");

		if (report_color) {
			vrPrintf("|cmode|color (0-99)|");
		}

		if (report_blend) {
			vrPrintf("|blend|src|dst| equ |color (0-99)|");
		}

		if (report_logic) {
			vrPrintf("|logclr|logind|lmode|");
		}

		if (report_alpha) {
			vrPrintf("|alpha|afunc|a-ref|bias |scale|");
		}

		if (report_lights) {
			vrPrintf("|light| lights |lview|sides|ambclr(0-99)|");
		}

		if (report_polys) {
			vrPrintf("|pcull|faces|dir|rmodes(f,b)|pstip|psmth|");
		}

		if (report_texture) {
			vrPrintf("| tex |binding(1D, 2D, 3D)| gen  | map1c| map2c|");
		}

		if (report_client_state) {
			vrPrintf("| client state ... |");
		}

		vrPrintf("\n");
	}
	header_count++;
	header_count %= header_repeat;

	/********************/
	/* print the values */
	vrPrintf("%9s|", msg);

	if (report_color) {
		vrPrintf("|%s|%3d%3d%3d%3d|",
			(rgba_mode == GL_FALSE ? "index" : "rgba "),
			(int)(color[0] * 99.0), (int)(color[1] * 99.0), (int)(color[2] * 99.0), (int)(color[3] * 99.0));
	}

	if (report_blend) {
		vrPrintf("| %s |%3d|%3d|%5d|%3d%3d%3d%3d|",
			(blend_en == GL_FALSE ? "dis" : "en "),
			blend_src,
			blend_dst,
			blend_equ,
			(int)(blend_clr[0] * 99.0), (int)(blend_clr[1] * 99.0), (int)(blend_clr[2] * 99.0), (int)(blend_clr[3] * 99.0));
	}

	if (report_logic) {
		vrPrintf("| %s  | %s  |%4d |",
			(logic_clr == GL_FALSE ? "dis" : "en "),
			(logic_ind == GL_FALSE ? "dis" : "en "),
			logic_mode);
	}

	if (report_alpha) {
		vrPrintf("| %s |%5d|%5.1f|%5.1f|%5.1f|",
			(alpha_en == GL_FALSE ? "dis" : "en "),
			alpha_func,
			alpha_ref,
			alpha_bias,
			alpha_scale);
	}

	if (report_lights) {
		vrPrintf("| %s |%1d%1d%1d%1d%1d%1d%1d%1d|%4d |%4d |%3d%3d%3d%3d|",
			(light_en == GL_FALSE ? "dis" : "en "),
			(light0_en == GL_FALSE ? 0 : 1),
			(light1_en == GL_FALSE ? 0 : 1),
			(light2_en == GL_FALSE ? 0 : 1),
			(light3_en == GL_FALSE ? 0 : 1),
			(light4_en == GL_FALSE ? 0 : 1),
			(light5_en == GL_FALSE ? 0 : 1),
			(light6_en == GL_FALSE ? 0 : 1),
			(light7_en == GL_FALSE ? 0 : 1),
			light_viewer,
			light_2side,
			(int)(light_ambclr[0] * 99.0), (int)(light_ambclr[1] * 99.0), (int)(light_ambclr[2] * 99.0), (int)(light_ambclr[3] * 99.0));
	}

	if (report_polys) {
		vrPrintf("| %s |%5s|%3s|%5s,%5s| %s | %s |",
			(poly_cull == GL_FALSE ? "dis" : "en "),
			(poly_side == GL_BACK ? "back " :
				(poly_side == GL_FRONT ? "front" :
				(poly_side == GL_FRONT_AND_BACK ? " all " : "none"))),
			(poly_front == GL_CCW ? "ccw" : "cw "),
			(poly_modes[0] == GL_FILL ? "fill " :
				(poly_modes[0] == GL_POINT ? "point" :
				(poly_modes[0] == GL_LINE ? "line " : "???"))),
			(poly_modes[1] == GL_FILL ? "fill " :
				(poly_modes[1] == GL_POINT ? "point" :
				(poly_modes[1] == GL_LINE ? "line " : "???"))),
			(poly_stipple == GL_FALSE ? "dis" : "en "),
			(poly_smooth == GL_FALSE ? "dis" : "en "));
	}

	if (report_texture) {
		vrPrintf("| %c%c%c | %5d %5d %5d | %1d%1d%1d%1d | %1d%1d%1d%1d | %1d%1d%1d%1d |",
			(tex_en[0] == GL_FALSE ? 'd' : 'e'),
			(tex_en[1] == GL_FALSE ? 'd' : 'e'),
			(tex_en[2] == GL_FALSE ? 'd' : 'e'),
			tex_bind[0],
			tex_bind[1],
			tex_bind[2],
			tex_gen[0],
			tex_gen[1],
			tex_gen[2],
			tex_gen[3],
			tex_map1coords[0],
			tex_map1coords[1],
			tex_map1coords[2],
			tex_map1coords[3],
			tex_map2coords[0],
			tex_map2coords[1],
			tex_map2coords[2],
			tex_map2coords[3]);
	}

	if (report_client_state) {
		vrPrintf("| %1d %1d %1d %1d %1d %1d |",
			cstate_colorarray,
			cstate_eflagarray,
			cstate_indexarray,
			cstate_normalarray,
			cstate_tcoordarray,
			cstate_vertexarray
		);
	}

	vrPrintf("\n");
}


/******************************************************************************/
/* _GLRenderWindowOutline(): renders the outline of the provided window (in   */
/*   pink/magenta), and puts the name of the window in the upper-left corner. */
/* TODO:                                                                      */
/*     * make the frustum rendering an argument flag                          */
/*     * add an indicator of which portion of a screen is the lower half      */
/*         (e.g. rendering lower portion of the diagonals)                    */
/*         (though perhaps this is solved by the fact that the name is in the */
/*            upper-left corner).                                             */
/* NOTE: there is also a compile-time option to show the render frustum of    */
/*   a window from the perspective of a particular user.                      */
void _GLRenderWindowOutline(vrRenderInfo *renderinfo, vrWindowInfo *window)
{
	double		coords_ur[3];
	char		msg[128];

	/********************************************/
	/*** Calculate the upper-right coordinate ***/
	/* This is basically adding the vector (UL - LL) to the point LR */
	/*   which is mathematically equivalent to (LR - LL) + UL.       */
	coords_ur[VR_X] = window->coords_ul[VR_X] - window->coords_ll[VR_X] + window->coords_lr[VR_X];	
	coords_ur[VR_Y] = window->coords_ul[VR_Y] - window->coords_ll[VR_Y] + window->coords_lr[VR_Y];	
	coords_ur[VR_Z] = window->coords_ul[VR_Z] - window->coords_ll[VR_Z] + window->coords_lr[VR_Z];	

#  if 0 /* temporary debugging code */
	vrPrintf("Name of window to be rendered is '%s' (%d:%d): ul=(%.2f,%.2f,%.2f) ll=(%.2f,%.2f,%.2f) lr=(%.2f,%.2f,%.2f) ur=(%.2f,%.2f,%.2f).\n",
		window->name, window->id, window->num,
		window->coords_ll[VR_X], window->coords_ll[VR_Y], window->coords_ll[VR_Z],
		window->coords_lr[VR_X], window->coords_lr[VR_Y], window->coords_lr[VR_Z],
		coords_ur[VR_X], coords_ur[VR_Y], coords_ur[VR_Z]);
#  endif

	/******************************/
	/*** draw the frame outline ***/
	glLineWidth(4.5);
	glPolygonMode(GL_FRONT, GL_POINT);

	glColor3ub((GLbyte)254,  (GLbyte)38, (GLbyte)174);		/* a magenta-ish/pink-ish color */

	glBegin(GL_LINE_STRIP);
		glVertex3dv(window->coords_ul);
		glVertex3dv(window->coords_ll);
		glVertex3dv(window->coords_lr);
		glVertex3dv(        coords_ur);
		glVertex3dv(window->coords_ul);
	glEnd();

	/****************************/
	/*** draw the window name ***/
	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	glColor3ub((GLbyte)255, (GLbyte)255, (GLbyte)255);		/* white */
	glRasterPos3dv(window->coords_ul);
#  if 1 /* set to '0' for more detail (i.e. inclusion of the window's arguments) */
	sprintf(msg, "%s (%d)", window->name, window->id);
#  else
	sprintf(msg, "%s (%d) -- %s", window->name, window->id, window->args);
#  endif
	vrRenderText(renderinfo, msg);

	glPopAttrib();

#if 0 /* More development code that will normally be off (0) (tempchange) { */
	/*************************************************************/
	/* draw a frustum of what the user will see through a window */
	vrUserInfo	*userinfo = renderinfo->eye->user;
	vrPoint		*eye_location;
	vrPoint		head_location;

	vrPoint		frustum[8];	/* each frustum is defined by 8 points */
	vrVector	direction[4];	/* eash frustum is defined by 4 directions */
#define NEAR_RATIO 0.5
#define FAR_RATIO 2.0

	/* TODO: choose between options -- presently can get location of the current eye, or the location of the current user's head */
	eye_location = &(renderinfo->eye->loc);
	vrPointGetTransFromMatrix(&head_location, userinfo->visren_headpos);

#  if 0
	vrPrintf("Name of user for simulator window is '%s' -- head is at (%f, %f, %f) -- eye is at (%f, %f, %f)\n", userinfo->name, head_location.v[VR_X], head_location.v[VR_Y], head_location.v[VR_Z], eye_location->v[VR_X], eye_location->v[VR_Y], eye_location->v[VR_Z]);
#  endif

	vrVectorFromTwoPoints(&direction[0], eye_location, (vrPoint *)(window->coords_ul));
	vrVectorFromTwoPoints(&direction[1], eye_location, (vrPoint *)(window->coords_ll));
	vrVectorFromTwoPoints(&direction[2], eye_location, (vrPoint *)(window->coords_lr));
	vrVectorFromTwoPoints(&direction[3], eye_location, (vrPoint *)(        coords_ur));

	vrPointAddScaledVector(&frustum[0], eye_location, &direction[0], NEAR_RATIO);	/* upper-left direction */
	vrPointAddScaledVector(&frustum[1], eye_location, &direction[1], NEAR_RATIO);	/* lower-left direction */
	vrPointAddScaledVector(&frustum[2], eye_location, &direction[2], NEAR_RATIO);	/* lower-right direction */
	vrPointAddScaledVector(&frustum[3], eye_location, &direction[3], NEAR_RATIO);	/* upper-right direction */
	vrPointAddScaledVector(&frustum[4], eye_location, &direction[0],  FAR_RATIO);	/* upper-left direction */
	vrPointAddScaledVector(&frustum[5], eye_location, &direction[1],  FAR_RATIO);	/* lower-left direction */
	vrPointAddScaledVector(&frustum[6], eye_location, &direction[2],  FAR_RATIO);	/* lower-right direction */
	vrPointAddScaledVector(&frustum[7], eye_location, &direction[3],  FAR_RATIO);	/* upper-right direction */

	/* draw the shaded (semi-transparent) frustum */
	glLineWidth(2.5);

	/* first render them all as outlines */
	glColor4ub((GLbyte)219, (GLbyte)177, (GLbyte)177, (GLbyte)255);		/* a pink-rose color */

	/* left side of frustum */
	glBegin(GL_LINE_STRIP);
		glVertex3dv(frustum[4].v);
		glVertex3dv(frustum[0].v);
		glVertex3dv(frustum[1].v);
		glVertex3dv(frustum[5].v);
	glEnd();

	/* top side of frustum */
	glBegin(GL_LINE_STRIP);
		glVertex3dv(frustum[7].v);
		glVertex3dv(frustum[3].v);
		glVertex3dv(frustum[0].v);
		glVertex3dv(frustum[4].v);
	glEnd();

	/* right side of frustum */
	glBegin(GL_LINE_STRIP);
		glVertex3dv(frustum[6].v);
		glVertex3dv(frustum[2].v);
		glVertex3dv(frustum[3].v);
		glVertex3dv(frustum[7].v);
	glEnd();

	/* under side of frustum */
	glBegin(GL_LINE_STRIP);
		glVertex3dv(frustum[5].v);
		glVertex3dv(frustum[1].v);
		glVertex3dv(frustum[2].v);
		glVertex3dv(frustum[6].v);
	glEnd();

	/* now render them as semi-transparent sides */
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	/* also works with GL_ONE as arg2 */
	glColor4ub((GLbyte)219, (GLbyte)177, (GLbyte)177, (GLbyte)96);		/* a pink-rose color */

	/* left side of frustum */
	glBegin(GL_POLYGON);
		glVertex3dv(frustum[4].v);
		glVertex3dv(frustum[0].v);
		glVertex3dv(frustum[1].v);
		glVertex3dv(frustum[5].v);
	glEnd();

	/* top side of frustum */
	glBegin(GL_POLYGON);
		glVertex3dv(frustum[7].v);
		glVertex3dv(frustum[3].v);
		glVertex3dv(frustum[0].v);
		glVertex3dv(frustum[4].v);
	glEnd();

	/* right side of frustum */
	glBegin(GL_POLYGON);
		glVertex3dv(frustum[6].v);
		glVertex3dv(frustum[2].v);
		glVertex3dv(frustum[3].v);
		glVertex3dv(frustum[7].v);
	glEnd();

	/* under side of frustum */
	glBegin(GL_POLYGON);
		glVertex3dv(frustum[5].v);
		glVertex3dv(frustum[1].v);
		glVertex3dv(frustum[2].v);
		glVertex3dv(frustum[6].v);
	glEnd();

#endif /* } */
}


/***************************************************************************/
/* _GLRenderWindowOutlines(): loops through all the windows and render the */
/*   ones with their "render-in-simulator" flag set.                       */
void _GLRenderWindowOutlines(vrRenderInfo *renderinfo)
{
	vrWindowInfo	*windowdefs;
	vrWindowInfo	*window;
	int		count;

	/* get the first window of the global config list of windows */
	windowdefs = (vrWindowInfo *)vrObjectFirst(renderinfo->context, VROBJECT_WINDOW);	/* get the head of the linked list of defined windows */

	/* loop over the entire linked list of windows in the configuration */
	for (window = windowdefs; window->next != NULL; window = window->next) {
		if (window->show_in_simulator) {
			_GLRenderWindowOutline(renderinfo, window);
		}
	}
}


/****************************************************************************/
/* _GLRenderWindowOutlineID(): selects the window in the configuration with */
/*   the matching ID number, and renders the outline of that window.        */
void _GLRenderWindowOutlineID(vrRenderInfo *renderinfo, int screen_id)
{
	vrWindowInfo	*windowdefs;
	vrWindowInfo	*window;
	int		count;

	/* get the nth window from the global config structure */
	windowdefs = (vrWindowInfo *)vrObjectFirst(renderinfo->context, VROBJECT_WINDOW);	/* get the head of the linked list of defined windows */
	window = windowdefs;	/* get the first window on the linked list */

	for (count = 0; (count < screen_id) && (window->next != NULL); count++)
		window = window->next;

	/* call the routine to do the actual rendering */
	_GLRenderWindowOutline(renderinfo, window);
}


/****************************************************************************/
/* TODO: make complete use of the "mask" argument */
void vrGLRenderDefaultSimulator(vrRenderInfo *renderinfo, int mask)
{
	vrEyeInfo	*curr_eye = renderinfo->eye;
	vrUserInfo	*curr_user = curr_eye->user;
	vrMatrix	dstmat;
	int		num_6sensors = renderinfo->context->input->num_6sensors;
	int		sensor_num;


#define	ATTR_REPORT 0x00	/* Set this value to 0x0 to disable attribute reporting to stdout (was 0x3b) */

	/* when the mask is set with all flags off, then no need to render anything */
	if (mask == 0)
		return;

	/* TODO: we should probably either get rid of these, or make them a DBG setting */
	vrGLReportAttributes("Before ", ATTR_REPORT);

	/* Since this routine modifies some of the graphics rendering properties */
	/*   we need to store them here and retrieve them upon exit.             */
	glPushAttrib(0
		| GL_COLOR_BUFFER_BIT		/* covers blending and alpha parameters    */
		| GL_CURRENT_BIT		/* covers current color and rendering mode */
		| GL_POLYGON_BIT		/* covers polygon ordering and style       */
		| GL_LINE_BIT			/* covers line width and style             */
		| GL_LIGHTING_BIT		/* covers all aspects of lighting          */
		| GL_ENABLE_BIT			/* covers all enable bits (inc. texture 2d)*/
	);

#if 1 /* Choose whether to disable lighting, or set to vertex colors as material */
	/* disable lighting and textures */
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_TEXTURE_2D);
	/* TODO: add 3D, after checking for that extension */
#else
	/* When lighting is enabled, these two lines cause the */
	/*   vertex colors to be used (otherwise all is gray). */
	/* TODO: should we specifically enable lighting for this style? */
	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
#endif

	/* setup the proper depth checks */
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	/* the basicgfx functions assume CCW polygon ordering */
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);


	/**********************************************/
	/* draw a brown 0.5' cube to represent the head */
	glPushMatrix(); /* { */
	glMultMatrixd(vrMatrixGet6sensorValuesDirect(&dstmat, curr_user->head)->v);
	glScalef(0.50, 0.50, 0.50);	/* set the size of the head */

	/* the "head" */
	if (!renderinfo->window->sim_follow_head) {	/* don't render the head when the view is constantly inside it */
		glColor3fv(curr_user->color);
		vrShapeGLCube();
	}

	/* use the head's outline to signify the sensor status */
	if (curr_user->head->error > 0)
		glColor3ub((GLbyte)230, (GLbyte)0, (GLbyte)0);		/* red   -- error */
	else if (curr_user->head->dummy)
		glColor3ub((GLbyte)252, (GLbyte)206, (GLbyte)100);	/* yellow  -- dummy input */
	else if (curr_user->head->oob)
		glColor3ub((GLbyte)0, (GLbyte)0, (GLbyte)230);		/* blue  -- out of bounds */
	else if (curr_user->head->active)
		glColor3ub((GLbyte)0, (GLbyte)230, (GLbyte)0);		/* green -- active */
	else
		glColor3ub((GLbyte)0, (GLbyte)0, (GLbyte)0);		/* black -- inactive, in bounds, no error */
	vrShapeGLCubeOutline();

	/* the "nose" */
	glPushMatrix(); /* { */
	glTranslatef(0.0, 0.0, -1.0);
	glScalef(0.1, 0.2, 0.5);
	glColor3f(curr_user->color[0]*0.5, curr_user->color[1]*0.5, curr_user->color[2]*0.5);
	vrShapeGLCubePlusOutline();
	glPopMatrix(); /* } */

	/* TODO: the eye positions should use the actual values. */
	/* the "left eye" */
	glPushMatrix(); /* { */
	glTranslatef(-0.5, 0.5, -1.0);
	glScalef(0.25, 0.25, 0.25);
	glColor3ub((GLbyte)255, (GLbyte)255, (GLbyte)255);		/* white */
	vrShapeGLCubePlusOutline();
	glPopMatrix(); /* } */

	/* the "right eye" */
	glPushMatrix(); /* { */
	glTranslatef(0.5, 0.5, -1.0);
	glScalef(0.25, 0.25, 0.25);
	glColor3ub((GLbyte)255, (GLbyte)255, (GLbyte)255);		/* white */
	vrShapeGLCubePlusOutline();
	glPopMatrix(); /* } */

	glPopMatrix(); /* } */


	/**************************************************************/
	/* draw a simple object to represent sensor-1 (usu. the wand) */
#if 1 /* compile-time option: draw the wand? { */
	if (num_6sensors >= 2) {
		glPushMatrix(); /* { */
		glMultMatrixd(vrMatrixGet6sensorValues(&dstmat, 1)->v);

		/* the "wand" */
#  if 1 /* draw as a pyramid */
		glColor3fv(curr_user->color);
		glScalef(0.20, 0.20, 0.75);
		glRotatef(-90.0, 1.0, 0.0, 0.0);
		vrShapeGLPyramid();

		/* use the sensor representation outline to signify the sensor status */
		if (vrGet6sensorErrorValue(1) > 0)
			glColor3ub((GLbyte)230, (GLbyte)0, (GLbyte)0);		/* red   -- error */
		else if (vrGet6sensorDummyValue(1))
			glColor3ub((GLbyte)252, (GLbyte)206, (GLbyte)100);	/* yellow  -- dummy input */
		else if (vrGet6sensorOobValue(1))
			glColor3ub((GLbyte)0, (GLbyte)0, (GLbyte)230);		/* blue  -- out of bounds */
		else if (vrGet6sensorActiveValue(1))
			glColor3ub((GLbyte)0, (GLbyte)230, (GLbyte)0);		/* green -- active */
		else
			glColor3ub((GLbyte)0, (GLbyte)0, (GLbyte)0);		/* black -- inactive, in bounds, no error */
		vrShapeGLPyramidOutline();

#  else /* line-method */
		{
		vrPoint		proploc;
		vrPoint		drawto;
		vrVector	vec;

		vrPointGetRWFrom6sensor(&proploc, 1);
		vrVectorGetRWFrom6sensorDir(&vec, 1, VRDIR_FORE);
		vrPointAddScaledVector(&drawto, &proploc, &vec, 2.0);
		glColor4ub((GLbyte)255, (GLbyte)255, (GLbyte)0, (GLbyte)255);	/* yellow */
		glDisable(GL_BLEND);
		glBegin(GL_LINE_STRIP);
			glVertex3dv(proploc.v);
			glVertex3dv(drawto.v);
		glEnd();
		}
#  endif
		glPopMatrix(); /* } */
	}

#  if 1 /* compile-time option: draw the other sensors as boxes? */
	/* draw simple objects to represent sensors 2-n */
	for (sensor_num = 2; sensor_num < num_6sensors; sensor_num++) {
		glPushMatrix(); /* { */
		glMultMatrixd(vrMatrixGet6sensorValues(&dstmat, sensor_num)->v);

		glColor3fv(curr_user->color);
		glScalef(0.20, 0.20, 0.20);
		vrShapeGLCube();

		/* use the sensor representation outline to signify the sensor status */
		if (vrGet6sensorErrorValue(sensor_num) > 0)
			glColor3ub((GLbyte)230, (GLbyte)0, (GLbyte)0);		/* red   -- error */
		else if (vrGet6sensorDummyValue(sensor_num))
			glColor3ub((GLbyte)252, (GLbyte)206, (GLbyte)100);	/* yellow  -- dummy input */
		else if (vrGet6sensorOobValue(sensor_num))
			glColor3ub((GLbyte)0, (GLbyte)0, (GLbyte)230);		/* blue  -- out of bounds */
		else if (vrGet6sensorActiveValue(sensor_num))
			glColor3ub((GLbyte)0, (GLbyte)230, (GLbyte)0);		/* green -- active */
		else
			glColor3ub((GLbyte)0, (GLbyte)0, (GLbyte)0);		/* black -- inactive, in bounds, no error */
		vrShapeGLCubeOutline();

		glPopMatrix(); /* } */
	}
#  endif
#endif /* } draw wand */

	/*********************************/
	/* draw selected window outlines */
	_GLRenderWindowOutlines(renderinfo);


	/******************************************************************/
	/* draw the outline and a semi-transparent surface of a 10^3 CAVE */
	glPushMatrix(); /* { */

	glLineWidth(1.5);
	glPolygonMode(GL_FRONT, GL_POINT);		/* TODO: [11/05/09] shouldn't this be: GL_FRONT_AND_BACK, GL_FILL ??? */

	/* move and scale how the cube will be drawn */
	/* TODO: it would be nice to base this off some information stored */
	/*   with the window about the size of the working volume.         */
	glTranslatef(0.0, 5.0, 0.0);
	glScalef(4.999, 4.999, 4.999);

	/* draw the outline */
	glColor3ub((GLbyte)230, (GLbyte)230, (GLbyte)230);		/* gray */
	vrShapeGLCubeOutline();

#if 1 /* compile-time: draw the semi-transparent CAVE screen surface (tempchange) */
	/* NOTE: because this is semi-transparent, it should be the last feature rendered! */
	glDisable(GL_CULL_FACE);	/* since we want to see the screens from inside or out */
	/* TODO: figure out why the "outside" of the CAVE screen box isn't rendered */

	/* draw the surface */
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	/* also works with GL_ONE as arg2 */
	glColor4ub((GLbyte)200, (GLbyte)200, (GLbyte)200, (GLbyte)64);	/* semi-transparent gray */
	vrShapeGLCube();

	/* redraw the outline to cover holes made by screen surface */
	glColor3ub((GLbyte)230, (GLbyte)230, (GLbyte)230);		/* gray */
	vrShapeGLCubeOutline();

	glColor4ub((GLbyte)255, (GLbyte)255, (GLbyte)255, (GLbyte)255);	/* opaque white */
#endif /* draw cave surface vs. just the white outline */

	vrGLReportAttributes("Middle ", ATTR_REPORT);

	glPopMatrix(); /* } */


	/*********************************/
	/* restore the OpenGL attributes */
	glPopAttrib();
	vrGLReportAttributes("After  ", ATTR_REPORT);
}

