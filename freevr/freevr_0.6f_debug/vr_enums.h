/* ======================================================================
 *
 * HH   HH         vr_enums.h
 * HH   HH         Author(s): Bill Sherman
 * HHHHHHH         Created: February 28, 2003
 * HH   HH         Last Modified: May 6, 2003
 * HH   HH
 *
 * Header file for FreeVR general enumerations.  Most of these are
 * used by several different data types, so it is convenient to have
 * them in a header file with no other ties.
 *
 * Copyright 2010, Bill Sherman & Friends, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#ifndef __VRENUMS_H__
#define __VRENUMS_H__

#ifdef __cplusplus
extern "C" {
#endif


/****************************************************/
/* special user number to set a value for all users */
#define	VR_ALLUSERS	-1


/********************************************************/
/* flags for the "startup_error" field of vrContextInfo */
#define	VRSTARTUP_OKAY		0x0000
#define	VRSTARTUP_BADSHMEM	0x0001
#define	VRSTARTUP_NOPROCS	0x0002
#define	VRSTARTUP_BADPROC	0x0004
#define	VRSTARTUP_BADWINDOW	0x0008
#define	VRSTARTUP_BADINDEV	0x0010
#define	VRSTARTUP_BADINMAP	0x0020
#define VRSTARTUP_NOLOCKSEM	0x0040
#define VRSTARTUP_NOBARRSEM	0x0080


/**********************************************************************/
/* enumerated list of possible "status" field of vrContextInfo values */
typedef enum {
		VRSTATUS_UNITIALIZED = -2,
		VRSTATUS_INITIALIZING,
		VRSTATUS_RUNNING = 0,
		VRSTATUS_TERMINATING,
		VRSTATUS_TERMINATED
	} vrSystemStatus;


/**************************************************/
/* enumerated list of possible user-interface styles.  */
/*   These can be useful for applications that wish to */
/*   alter how the user can interact with them based   */
/*   on the hardware interface style.                  */
typedef enum {
		VRUI_UNKNOWN = -2,
		VRUI_DEFAULT = -1,
		VRUI_SIMULATOR,
		VRUI_SIM = VRUI_SIMULATOR,
		VRUI_STATIONARY,
		VRUI_CAVE = VRUI_STATIONARY,
		VRUI_HBD,
		VRUI_HMD = VRUI_HBD,
		VRUI_HAND,
		VRUI_WALL,
		VRUI_IMMERSADESK,
		VRUI_WORKBENCH,
		VRUI_PIT,
		VRUI_DESKTOP
	} vrUIStyle;


/**************************************************/
typedef enum {
		def = -1,	/* this is when inheritance is possible */
		none = 0,
		brief = 1,
		one_line,
		verbose,
		machine,	/* This is for easy parsing by GUIs connected to telnet */
		file_format,	/* This is for writing something that the config parsing can read */
		everything,

		normal = brief
	} vrPrintStyle;


/**************************************************/
typedef enum vrObject_en {
		VROBJECT_NONE,
		VROBJECT_CALLBACK,	/* NOTE: this type of object is not part of the configuration, but is used throughout the system */
		VROBJECT_SYSTEM,
		VROBJECT_PROCESS,
		VROBJECT_WINDOW,
		VROBJECT_EYELIST,
		VROBJECT_USER,
		VROBJECT_PROP,
		VROBJECT_INDEV,
		VROBJECT_INPUT,
		VROBJECT_EOL_OBJECTS,	/* End of the list of official config OBJECTs */

		/* The following object types are not official configuration  */
		/*   objects, and thus don't necessarily have all the generic */
		/*   object fields, but they do all begin with "object_type". */
		VROBJECT_ADDRESS,	/* NOTE: this non-object is an indicator that an entity has been specified by a memory address */
		VROBJECT_CONTEXT,
		VROBJECT_CONFIGINFO,
		VROBJECT_INPUTINFO,	/* the overarching input structure */
		VROBJECT_INPUTDATA,	/* NOTE: this is not a full-fledged object, but useful for indicating what an address contains */
		VROBJECT_EYEINFO,
		VROBJECT_LOCK,
		VROBJECT_BARRIER,
		VROBJECT_ENDOFLIST
	} vrObject;


/**************************************************/
typedef enum vrVisrenMode_en {
		VRVISREN_NONE_SELECTED = -1,
		VRVISREN_DEFAULT = VRVISREN_NONE_SELECTED,
		VRVISREN_MONO,
		VRVISREN_LEFT,		/* this is mono, but adjusted for left eye */
		VRVISREN_RIGHT,		/* this is mono, but adjusted for right eye */
		VRVISREN_DUALFB,
		VRVISREN_DUALVP,
		VRVISREN_ANAGLYPHIC,
		VRVISREN_CHECKERBOARD,
		VRVISREN_VIBRATE
	} vrVisrenModeType;


/**************************************************/
typedef enum vrFrameBufferType_en {
		VRFB_DEFAULT = 0,
		VRFB_FULL = 0,		/* monoscopic rendering to the full buffer mode */
		VRFB_LEFT,		/* left-eye stereo buffer mode */
		VRFB_RIGHT,		/* right-eye stereo buffer mode */
		VRFB_FULL_LEFTEYE,	/* left eye in it's own window mode */
		VRFB_FULL_RIGHTEYE,	/* right eye in it's own window mode */
		VRFB_SPLIT_LEFTEYE,	/* left eye in half the window mode */
		VRFB_SPLIT_RIGHTEYE	/* right eye in half the window mode */
	} vrFrameBufferType;


/**************************************************/
typedef enum vrAnaglyphType_en {
		VRANAGLYPH_DEFAULT = 0,
		VRANAGLYPH_ALL = 0,
		VRANAGLYPH_RED,
		VRANAGLYPH_GREEN,
		VRANAGLYPH_BLUE
	} vrAnaglyphType;


/**************************************************/
typedef enum vrEyeType_en {
		VREYE_DEFAULT = -1,
		VREYE_LEFT = 0,
		VREYE_RIGHT = 1,
		VREYE_CYCLOPS,
		VREYE_MONO = VREYE_CYCLOPS,
		VREYE_VIBRATE
	} vrEyeType;



#ifdef __cplusplus
}
#endif

#endif  /* __VRENUMS_H__ */
