/* ======================================================================
 *
 * HH   HH         vr_input.opts.h
 * HH   HH         Author(s): Bill Sherman
 * HHHHHHH         Created: August 17, 1998
 * HH   HH         Last Modified: August 13, 2013
 * HH   HH
 *
 * Header file for available FreeVR input device options
 *
 * Copyright 2013, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/**************************************************************************/
/* Library programmers: add new input device options here.                */
/*                                                                        */
/* To add a new type "T", add the declaration for the initialization      */
/*   function and add a new record to the vrInputOpts array.  The new     */
/*   record can be placed anywhere before the all NULL record.  The first */
/*   field of the record is a string representation of the new type,      */
/*   followed by a single function for putting the specific device info   */
/*   into the input device structure.                                     */
/*                                                                        */
/* The function can be named anything, but must take a specific argument: */
/*   function1(vrInputDevice *)                                           */
/* The function must create all the necessary callbacks for opening/      */
/*   polling/resetting/closing the device, and return "void".             */
/*                                                                        */
/* Also, don't forget to add the new source file in the Makefile list.    */
/**************************************************************************/

#include "vr_input.h"

void vrStaticInitInfo(vrInputDevice *info);
void vrShmemdInitInfo(vrInputDevice *info);
void vrShmemdOutInitInfo(vrInputDevice *info);
void vrFobInitInfo(vrInputDevice *info);
void vrMsInitInfo(vrInputDevice *info);
void vrFastrakInitInfo(vrInputDevice *info);
void vrJoydevInitInfo(vrInputDevice *info);
#ifdef __linux
void vrEvioInitInfo(vrInputDevice *info);
#endif
void vrMagellanInitInfo(vrInputDevice *info);
void vrSpacetecInitInfo(vrInputDevice *info);
void vrPinchgloveInitInfo(vrInputDevice *info);
void vrCybergloveInitInfo(vrInputDevice *info);
void vrDTrackInitInfo(vrInputDevice *info);
void vrVruiDDInitInfo(vrInputDevice *info);
void vrVRPNInitInfo(vrInputDevice *info);
void vrVruiDDOutInitInfo(vrInputDevice *info);

#ifdef WIN_GLX
void vrXwindowsInitInfo(vrInputDevice *info);
#endif

#ifdef WIN_WGL
void vrWin32InitInfo(vrInputDevice *info);
#endif

#ifdef VRINPUT_ONLY /* { */
vrInputOptsType vrInputOpts[] = {
		{ "static",	vrStaticInitInfo },
		{ "shmemd",	vrShmemdInitInfo },
		{ "shmemdout",	vrShmemdOutInitInfo },		/* NOTE: a special tracking "output" device */
		{ "asc_fob",	vrFobInitInfo },
		{ "asc_ms",	vrMsInitInfo },
		{ "polhemus",	vrFastrakInitInfo },
		{ "fastrak",	vrFastrakInitInfo },
		{ "is900",	vrFastrakInitInfo },
		{ "patriot",	vrFastrakInitInfo },
		{ "joydev",	vrJoydevInitInfo },
#ifdef __linux
		{ "evio",	vrEvioInitInfo },
		{ "evdev",	vrEvioInitInfo },		/* Alternate name for EVIO device */
#endif
		{ "magellan",	vrMagellanInitInfo },
		{ "spaceorb",	vrSpacetecInitInfo },
		{ "spaceball",	vrSpacetecInitInfo },
		{ "pinchglove",	vrPinchgloveInitInfo },
		{ "cyberglove",	vrCybergloveInitInfo },
		{ "dtrack",	vrDTrackInitInfo },
		{ "vrpn",	vrVRPNInitInfo },
		{ "vruidd",	vrVruiDDInitInfo },
		{ "vruiddout",	vrVruiDDOutInitInfo },		/* NOTE: a special tracking "output" device */
#ifdef WIN_GLX
		{ "xwindows",	vrXwindowsInitInfo },
#endif
#ifdef WIN_WGL
		{ "win32",      vrWin32InitInfo },
#endif
		{ NULL, NULL }
	};
#endif /* } VRINPUT_ONLY */

