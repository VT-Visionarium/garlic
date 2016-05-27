/* ======================================================================
 *
 *  CCCCC          vr_telnet.c
 * CC   CC         Author(s): Bill Sherman
 * CC              Created: October 22, 2001
 * CC   CC         Last Modified: September 1, 2014
 *  CCCCC
 *
 * Code file for FreeVR telnet communications process.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/*************************************************************************

USAGE:
	Making a connection:
		% telnet <host> <port>
	      [ Enter FreeVR password> <#######> ]
		FreeVR> <command> <arguments>
		...
		FreeVR> ^D
		%

	Possible commands:
		"help" -- print a list of possible commands
		"quit" -- quit the telnet process (further socket
			connections will NOT be possible).
		"close" -- close the socket connection (further socket
			connections to the same port WILL be possible).
	  :-}	"term" -- quit the entire FreeVR application (working by kludge).
	  :-(	"reset" -- reset something (not sure what).
		"pause" -- pause the system
		"setenv <variable name> <value>" -- set an environment variable
		"unsetenv <variable name>" -- unset an environment variable [NOTE: doesn't work under IRIX]
		"echo <string>" -- echo a string (includes variable evaluation)
			possible variables are:
			- $machine | $hostname -- name of the machine application is running on
			- $version -- short name of the version of the library
			- $fullversion -- longer name of the library version
			- $arch | $binaryformat
			- $compile -- compile string that includes information about when the library was
				compiled and information about the operating system it was compiled on.
			- $freevrhomedir -- the system location where config and other files can be found
			- $memsize -- size of the shared memory segment (in bytes)
			- $status -- current status of the library (running, ended, etc.)
			- $startup_error -- a bitwise flag of errors that may have occurred on startup (0 is good)
			- $time_immemorial -- the (unix-style) time of when the library was started
			- $sim_time | $time -- the number of seconds that the simulation has been running
				(NOTE: paused time is subtracted out of this)
			- $wall_time -- current ...
			- $debug_level --
			- $debugthistoo --
			- $system -- the freevr system object that is active
			- $def_visrenmode -- the default visual rendering mode
			- $win_style -- type of windows used by this library (GLX vs. MSWindows)
			- $gfx_style -- type of graphics library (currently only "OpenGL")
			- $sem_style -- method of semaphores implementation
			- $sock_style -- method of sockets interface implementation (usually "BSD")
			- $mp_style --
			- $compile_options --
			- $<env var name>
		"verbose" -- set print style to verbose mode.
		"file" -- set print style to config file format mode.
		"brief" -- set print style to brief mode.
		"one_line" -- set print style to one_line mode.
		"machine" -- set print style to machine mode.
		"print" | "p" <query> -- print values from running application.
		"printfile" | "pf" [+]<filename> <query> -- print values from running application to a file
			possible print and printfile requests are:
			- "help" -- the list of what can be printed
			- "context" -- the context structure
			- "shmem" -- information about the shared memory system
			- "config" -- the configuration structure
			- "system" -- the system structure (of the running system)
			- "settings -- print the system settings values
			- "defaults" -- the system default settings structure (of the running system)
			- "near -- the near clipping plane value
			- "far -- the far clipping plane value
			- "procs" -- a summary of the running processes (or threads)
			- "proc[<num>] -- detailed information about a particular process (or thread)
			- "ui" -- a summary of all the inputs' user interface functionality (requires application-programmer specification)
			- "inputs" -- a summary of all the system's inputs
			- "input <name>" -- detailed information about a particular (named) input
			- "inputdevices" -- print info about all inputs devices
			- "inputdevice[<num>]" -- print info about an input device
			- "windows" -- print a summary of all the windows
			- "window[<num>]" -- print info about a particular window
			- "eyelists" -- print info about all eyelists
			- "eyelist[<num>]" -- print info about the eyelist
			- "eyes" -- print info about the eyes of the system
			- "eye[<num>]" -- print info about a particular eye
			- "users" -- print info about all the active users
			- "user[<num>]" -- print info about all the Nth user
			- "object" | "o" <type> list -- list all <type> config objects
			- "object" | "o" <type> <name> -- print info about a config object
		"set" | "s" <request> -- set some particular setting to the given value.
			possible set-requests are:
			- "input" <variable> <value> -- set a particular input to the given value.
				- NOTE: there are three options for setting a 6-sensor:
					- "id" (set to the identity matrix)
					- "loc [<x|*> [<y|*> [<z|*>]]]" ('*' means no-change)
					- "move [<x> [<y> [<z> [<azim> [<elev> [<roll]]]]]]" (ungiven values are 0.0)
			- "pause" {0,1} -- pause the entire system.
			- "debuglevel|dl" <value> -- set the global debug level.
			- "debugthistoo|dtt" <value> -- set the global debug-this-too.
			- "debuglevel|dl[<num>]" <value> -- set process <num>'s debug level.
			- "debugthistoo|dtt[<num>]" <value> -- set process <num>'s debug-this-too.
			- "near <value>" -- set the near clipping plane for all (active) windows
			- "far <value>" -- set the far clipping plane for all (active) windows
			- "window[<num>] fps" {0,1} -- turn off/on a window's fps display.
			- "window[<num>] fps_loc" <v1>,<v2> -- relocate a window's fps display.
			- "window[<num>] fps_color" <v1>,<v2>,<v3> -- set color of a window's fps display.
			- "window[<num>] stats" {0,1} -- turn off/on a window's stats display.
			- "window[<num>] frame" {0,1} -- turn off/on a window's frame display in all windows.
			- "window[<num>] world" {0,1} -- turn off/on a window's virtual world display.
			- "window[<num>] frm" <value> -- set the front rendering mode -- currently s/b {0,0x1B00,0x1B01,0x1B02}
			- "window[<num>] brm" <value> -- set the back rendering mode -- currently s/b {0,0x1B00,0x1B01,0x1B02}
			- "window[<num>] sim" <move cmd> -- move the simulator view of a window
				WARNING: currently there is no check to limit this to simulator windows
				possible move-commands are:
				- "away"
				- "toward"
				- "center"
				- "sethome"
				- "gohome"
				- "posy"
				- "negy"
				- "posx"
				- "negx"
				- "toggle"
			- "user[<num>] iod" <v1> -- set the iod of a user.
			- "user[<num>] color" <v1>,<v2>,<v3> -- set the color of a user.
			- "proc[<num>] end" <v1> -- set the end_proc flag of a process
			- "proc[<num>] done" <v1> -- set the proc_done flag of a process
			- "proc[<num>] usec" <v1> -- set the usec_min value of a process
			- "proc[<num>] printcolor" <v1> -- set the print_color flag of a process
			- "proc[<num>] stats_calc" <v1> -- set the flag of whether to calculate stats\n"
			- "proc[<num>] stats_show" <v1> -- set the flag of whether to show these stats\n"
			- "proc[<num>] stats_mask" <v1> -- set the mask of which statistics to show\n"
			- "proc[<num>] stats_xloc" <v1> -- set the x location of where to put stats\n"
			- "proc[<num>] stats_yloc" <v1> -- set the y location of where to put stats\n"
			- "proc[<num>] stats_width" <v1> -- set the width of the stats display\n"
			- "proc[<num>] stats_top" <v1> -- set the top timeline of the stats display\n"
			- "proc[<num>] stats_interval" <v1> -- set the horizontal time intervals of the stats display\n"
			- "proc[<num>] stats_scale" <v1> -- set the vertical scale of the stats display\n"
			- "proc[<num>] stats_color" <v1>,<v2>,<v3> -- set the background color of the stats display\n"
			- "proc[<num>] stats_opac" <v1> -- set the background opacity of the stats display\n"
		"lock" | "l" <command> -- send a command to a FreeVR lock
			... (see help message)
		"barrier" | "b" <command> -- send a command to a FreeVR barrier
			... (see help message)
		"relbar" -- shortcut to release a barrier



	FreeVR configuration options for telnet-type processes:
		"port" -- TCP port to request
		"portrange" -- number of (consecutive) ports to check.
			If 0 or less, then wait for specified port only.
		"prompt" -- prompt to use (sent to socket clients)
		"password" -- a password that must be typed to access the
			running application.
			NOTE: this is currently stored "in the clear" in the config file

	Application control:
	  :-(	[NYI] a callback function that allows the application first
		dibs at parsing commands that come over the telnet socket.
		Perhaps totally overriding the built-in parsing, or perhaps
		just done first, followed by the built-in work.  If the
		former, then there needs to be a function that can be called
		to do the built-in stuff.


HISTORY:
	22 October 2001 (Bill Sherman) -- obtained initial code ordering
		from vr_visren.c.  (Not all of it is needed since the
		"telnet" type processes don't, yet, have "objects").
		Obtained socket parsing code from my example VR
		application (ex8_socket.c).  Also copied and made minor
		changes to my socket_api.c file -- called vr_socket.c
		in the FreeVR library source.

	23 October 2001 (Bill Sherman) -- added the "print" command, and
		the corresponding vrInfoGet() function to respond to such
		queries.  This included the use of fdopen() to convert
		a standard file descriptor to a stream version that can
		be used with existing functions that expect a stream as
		the argument.

	1 November 2001 (Bill Sherman) -- added the "set" command to set
		the value of a given variable.  Wrote vrInfoSet() function
		to handle the dirty work.

	5-6 November 2001 (Bill Sherman) -- ...

	20 April 2002 (Bill Sherman) -- ... (see diary for now)

	22 April 2002 (Bill Sherman) -- sim move command

	6 January 2003 (Bill Sherman) -- Added the ability to set the "end_proc"
		and "proc_done" fields for a process.  Also fixed a minor bug
		in the invalid user error message that gave feedback using
		the number of windows.

	7 January 2003 (Bill Sherman) -- Implemented the "printobject" command
		which allows the user to see any of the configuration objects
		regardless of whether they are used by the current system or
		not.  A list of all the names of a particular class of object
		can also be obtained by giving the string "list" as the object
		name.

	8 January 2003 (Bill Sherman) -- "echo"

	13 January 2003 (Bill Sherman) -- "setenv"

	5 March 2003 (Bill Sherman) -- added "relbar" command

	7 March 2003 (Bill Sherman) -- Implemented a new "lock" set of
		commands that can be used to control existing locks in
		the FreeVR system, or a new test lock can be created.
		The use of this is primarily indented for the debugging
		of locks.  Did pretty much the same for FreeVR "barriers".

	11 March 2003 (Bill Sherman) -- setting of a process' "print_color"

	23 April 2003 (Bill Sherman) -- added code to print the auxiliary data
		of specific windows.  Also went with a system wide change of
		the "opaque" fields to now be called "aux_data".

	13 May 2003 (Bill Sherman) -- added code to change parameters of
		the rendering statistics display.  Also to change the
		"usec_min" time for a process.  And, because the later
		also allows this process to have a minimum time per
		frame, deleted the "loop_delay" field in _TelnetPrivate.

	29 May - 2 June 2003 (Bill Sherman) -- added commands to set the
		statistics values associated with each process.  Changed
		the nature of fps_color to use floats rather than unsigned
		bytes.  Added "set pause <n>" and "pause" commands.

	May 2006 (Bill Sherman) -- added commands to keep up with new
		settings (including user-interface display settings, and
		rendermode settings).

	June 13-19 2006 (Bill Sherman) -- Lots of changes, all primarily
		with the goal of aiding the interface to a "machine"
		script program to display/alter features of the running
		application.  Changes include making a slight alteration to
		the response to a "windows" & "users" commands when "machine"
		printstyle has been requested.  Also, improved the command
		parsing to:
			- allow for multiple commands in a single read
			- use semicolon or newline as command separator
			- ignore beginning whitespace in command

	May 5-6 2013 (Bill Sherman) -- Added the ability to affect the
		configuration values for window objects that are not in
		the active configuration (with "object window").  (This
		is done to turn on the new "showFrame" feature for windows
		and have them appear in the simulator view.)

	June 12 2013 (Bill Sherman, very early in the  AM) -- Added the
		ability to nudge window placement in any cardinal direction
		with the "nudgex", "nudgey" and "nudgez" window commands.

	2 September 2013 (Bill Sherman) -- adopting a consistent usage
		of str<...>cmp() functions (using logical negation).

		Also, though not coded here in vr_telnet.c, I fixed the
		ability to alter the value of a 6-sensor input by fixing
		the vrAssignGenericInput() function in vr_input.c
		[reported on 09/03/2013 impl_diary]

	15 September 2013 (Bill Sherman)
		Changed the "0x%p" format to the improved "%#p" format.

	24 August 2014 (Bill Sherman)
		Split the vrTelnetMainLoop() function into separate functions
		for vrTelnetInitProc(), vrTelnetOneFrame() and then still the
		MainLoop.  This is to enable merging processes into a single
		thread.  (Also fixed a couple of bugs along the way.)

TODO:
	Allow all objects in configuration to have values set.  (NOTE: I
		made this work for window objects, so just need to duplicate
		that code-skeleton, and should be easy to make it work for
		any of the object types.)

	Ability to set a value for ALL of the windows/procs/etc. (eg. "stats")
		An example might be:
			FreeVR> s proc[*] stats_calc 0

	Consider an alternative to the "printfile" command to simply add
		"> 'filename'" or ">> 'filename'" to the end of a print
		statement.

	Commands to be implemented:
		- "reset" ?? (I'm not sure what we would reset)
		- open windows for operation (or window processes)
		- start inputs (or input processes)
		- remap existing inputs
		- redirect -- redirect telnet output to file rather than
			back through the socket.  Also need an option to
			end the redirection and close the file.
		- DONE: close operating windows (or window processes)
			(simply set the "end" value of a visren process)
		- DONE: pause -- not sure how much to pause, perhaps on a
			process by process selectivity, with the option to
			pause all.  (This may require some additional code
			in vr_procs.c, and perhaps a new flag in vr_procs.h
			-- or just a way to block a barrier.)

	The other queries commented out in the vrInfoGet() help_msg.
		* Especially "inputbyname" !  [09/03/13]

	A command history mechanism (even simple will do).

	Implement an "alias" command -- and make the current implementation
		of "term" a built in alias.  Aliases can be specified in
		the config file, perhaps with an option to read some initial
		telnet commands from another file -- which would often just
		have aliases.

	Parse the variables out of the command string for all commands, not
		just the "echo" command -- which will greatly simplify the
		echo command.  Thus a variable can be set for complex
		expressions, and allow the user to refer to the variable
		instead of the expression every time.  This same function
		could also be used to handle the history and aliasing
		mechanisms.

	Figure out how to encode a password in the config file (and also
		therefore, what command can be used by the administrator
		to create the proper string.  (Allow option to require the
		password of the user running the app.)

	When requesting password from socket, disable echo.

	Queries that access the vrFprint<object>() functions should be able
		to pass an optional argument of the level of detail that
		should be printed (i.e. the vrPrintStyle).
		NOTE: Partially solved by having a default print style that
		is passed.

	PARTIAL: Fix "term" command.  Currently fixed by setting the first
		2-way input switch to 1, which is the standard way for apps
		to quit.

	DONE: A new "machine" print style that can be set.
		[NOTE: the vr_telnet.c part of this has already been
		implemented, but now it requires all the object print
		functions to handle the new style.]
		[06/19/2006: at least as done as this file is concerned.  As
		new needs arise due to the interface with the Tcl/Tk (or
		other) scripts, they will be implemented in the appropriate
		(other) file.

	PARTIAL: Pass vrContext around through arguments rather than as a global.
		[06/19/2006: only place left to do is vrTelnetMainLoop() -- which
		will leave only the vrTelnetSignalHandler() function, which may
		not be doable.]

*************************************************************************/
#include <stdio.h>	/* standard input and output functions! */
#include <stdlib.h>	/* needed for atof() */
#include <string.h>	/* needed for strchr() */
#include <signal.h>	/* needed for signal() and associated #defines */

#include "vr_socket.h"
#include "vr_debug.h"
#include "vr_procs.h"	/* needed for proc_info */
#include "vr_config.h"	/* needed for vrContext and callbacks functions */
#include "vr_utils.h"	/* needed for vrSleep() */
#include "vr_objects.h"	/* needed for vrObjectSearch() */

#define	TAB	"\t"

/* static (file-scope) globals for the (TODO: each?) telnet process */
static vrProcessInfo	*proc_info = NULL;	/* TODO: is this nece since we also have vrThisProc? */


/************************************************************************/
/** A structure of auxiliary information for a particular *PROCESS*.  **/
typedef struct {
		int		listen_sock;	/* socket to receive commands */
		int		cmd_socket;	/* socket from which to get commands */
		FILE		*cmd_fp;	/* stream file-pointer for command socket */
		int		inet_port;	/* port number to which one should connect */
		int		inet_port_range;/* the number of other (consecutive) ports to try */
		vrPrintStyle	style;		/* the default print style for some queries */
		char		host_machine[256]; /* name of the machine we're running on */
		char		*prompt;	/* command prompt for incoming telnets */
		char		*password;	/* a password that must be given to access socket */

		char		inbuf[1024];	/* used to collect incoming message */
		char		*parse;		/* pointer to where in inbuf to parse from */
	} _TelnetPrivate;


/* ===================================================================== */
/* NOTE: the "telnet" type of process (currently, 10/22/01) has no       */
/*   "object", so the object-handling routines that are at the beginning */
/*   of other "process" types such as "visren" and "input" are not       */
/*   present here.                                                       */
/* ===================================================================== */


/******************************************************************/
/* vrTelnetSignalHandler(): on a SIGINT signal, this process will */
/*   close down all things it's responsible for and exit.         */
/******************************************************************/
static void vrTelnetSignalHandler(int which)
{
#if 0 /* not currently used here  -- TODO: should be used to close the open socket */
	_TelnetPrivate	*aux = (_TelnetPrivate *)proc_info->aux_data;
#endif

	switch (which) {
	case SIGUSR2:
		vrMsgPrintf("vrTelnetSignalHandler(): Hey, input process got a USR2 signal\n");
		vrCallbackInvoke(vrContext->callbacks->HandleUSR2);
		break;

	default:
		/* for all other signals, assume death */
		vrMsgPrintf("vrTelnetSignalHandler(): (Telnet proc %d) " RED_TEXT "dying with %s\n" NORM_TEXT, getpid(), vrSigName(which));

		vrMsgPrintf("vrTelnetSignalHandler(): Time of death = %f, number of frames = %d\n",
                        proc_info->frame_wtime, proc_info->frame_count);

		vrMsgPrintf("vrTelnetSignalHandler(): Shared memory usage:%7ld (%ld freed)\n",
			vrShmemUsage(), vrShmemFreed());

		proc_info->end_proc = 1;
		vrFprintProcessInfo(stdout, proc_info, verbose);

#ifdef COREDUMP_IS_GOOD
		vrFprintf(stderr, "vrTelnetSignalHandler(): YO aborting in vr_input.c\n");
		abort();
#endif

		/* PAUSE to allow debugger to attach to the process. */
		vrMsgPrintf("vrTelnetSignalHandler(): Process pausing, use 'dbx -p %d' to debug.\n", getpid());
		pause();
	}
}


/***********************************************************/
/* vrInfoGet(): print a response to the given query to the */
/*   given FILE stream.  See the help_msg string for a     */
/*   list of possible queries.  Passing "help" sends this  */
/*   list.                                                 */
/* NOTE: this function basically takes the input string    */
/*   with the "print" or "p" (or whatever) stripped off.   */
/***********************************************************/
/* NOTE: this function (and vrInfoSet(), which follows) could easily */
/*   be moved to another source file, as they don't have much to do  */
/*   with telnetting, other than this is the only process that makes */
/*   use of it at the moment.                                        */
void vrInfoGet(vrContextInfo *context, FILE *file, char *query, vrPrintStyle style)
{
static	char	*help_msg = "Available queries:\n"
			TAB "help -- print a list of possible queries\n"
			TAB "context -- print the context structure\n"
			TAB "shmem -- print info about the shared memory system\n"
			TAB "config -- print the configuration structure\n"
			TAB "system -- print the system structure\n"
			TAB "settings -- print the system settings values\n"
			TAB "defaults -- print the system default values\n"
			TAB "near -- the near clipping plane value\n"
			TAB "far -- the far clipping plane value\n"
			TAB "procs -- print a summary of all the processes\n"
			TAB "proc[<num>] -- print the structure of the Nth process\n"
			TAB "ui -- prints the inputs' user-interface functionality\n"
			TAB "inputs -- print a list of the available inputs\n"
			TAB "input <name> -- print info about a particular input\n"
		/* :-(	TAB "inputbyname <name> -- print info about a particular input\n"	*/
		/* :-(	TAB "inputmap -- print the active inputmap\n"			*/
			TAB "inputdevices -- print info about all inputs devices\n"
			TAB "inputdevice[<num>] -- print info about an input device\n"
			TAB "windows -- print a summary of all the windows\n"
			TAB "window[<num>] -- print info about a particular window\n"
			TAB "eyelists -- print info about all eyelists\n"
			TAB "eyelist[<num>] -- print info about the eyelist\n"
			TAB "eyes -- print info about the eyes of the system\n"
			TAB "eye[<num>] -- print info about a particular eye\n"
			TAB "users -- print info about all the active users\n"
			TAB "user[<num>] -- print info about all the Nth user\n"
		/* :-(	TAB "props -- print info about all the system's props\n"		*/

			/* These print options specify any objects in the configuration */
			/*   whether used in the existing system or not.                */
			TAB "object|o <type> list -- list all <type> config objects\n"
			TAB "object|o <type> <name> -- print info about a config object\n"
		"";
static	char		*whitespace = " \t\r\b";
static	char		buffer[2048];	/* a place for putting strings -- currently just for "ui" query */
	vrConfigInfo	*config = context->config;
	int		count;				/* for looping through all objects of the requested type */
	int		ival;				/* integer value -- generally for array index */

	vrDbgPrintfN(PARSE_DBGLVL, "FreeVR (telnet): Query request received for '%s' [fp = %#p]\n", query, file);


	/******************************************************/
	/*** parse query here -- very simple-minded parsing ***/
	/******************************************************/

	/********/
	/* help */
	if (!strncmp(query, "help", 4)) {
		vrFprintf(file, help_msg);
	} else

	/*********/
	/* shmem */
	if (!strncmp(query, "shmem", 5)) {
		vrFprintShmemInfo(file, NULL, verbose /*s/b 'style', but only verbose implemented */);
	} else

	/***********/
	/* context */
	if (!strncmp(query, "context", 7)) {
		vrFprintContext(file, context, style);
	} else

	/**********/
	/* config */
	if (!strncmp(query, "config", 6)) {
		vrFprintConfig(file, config, style);
	} else

	/**********/
	/* system */
	if (!strncmp(query, "system", 6)) {
		vrFprintSystemInfo(file, config->system, style);
	} else

	/************/
	/* settings */
	if (!strncmp(query, "settings", 8)) {
		vrFprintSystemSettings(file, &(config->system->settings), style);
	} else

	/************/
	/* defaults */
	if (!strncmp(query, "defaults", 8)) {
		vrFprintSystemSettings(file, &(config->defaults), style);
	} else

	/********/
	/* near */
	if (!strncmp(query, "near", 4)) {
		if (config->system->settings.near_clip < 0.0)
			vrFprintf(file, "%.3f\n", config->defaults.near_clip);
		else	vrFprintf(file, "%.3f\n", config->system->settings.near_clip);
	} else

	/*******/
	/* far */
	if (!strncmp(query, "far", 4)) {
		if (config->system->settings.far_clip < 0.0)
			vrFprintf(file, "%.3f\n", config->defaults.far_clip);
		else	vrFprintf(file, "%.3f\n", config->system->settings.far_clip);
	} else

	/*********/
	/* procs */
	if (!strncmp(query, "procs", 5)) {
		for (count = 0; count < config->num_procs; count++) {
			vrFprintf(file, "proc[%d]: ", count);
			vrFprintProcessInfo(file, config->procs[count], one_line);
		}
	} else

	/*************/
	/* proc[<n>] */
	if (!strncmp(query, "proc[", 5)) {
		ival = vrAtoI(&query[5]);
		if (ival >= config->num_procs) {
			vrFprintf(file, "Invalid proc request (%d), only %d processes (0..%d).\n", ival, config->num_procs, config->num_procs-1);
		} else {
			if (style == machine) {
				vrFprintf(file, "%d:", ival);
			}
			vrFprintProcessInfo(file, config->procs[ival], style);
		}
	} else

	/***********************/
	/* ui (user interface) */
	if (!strncmp(query, "ui", 2)) {
		vrSprintInputUI(buffer, context->input, sizeof(buffer), style);
		vrFprintf(file, "=================================================================\n");
		vrFprintf(file, buffer);
		if (style == machine) {
			/* For machine parsers (which are probably line-based), we */
			/*   need to send a clear signal that the message is over. */
			vrFprintf(file, "==uiend==\n");
		}
	} else

	/**********/
	/* inputs */
	if (!strncmp(query, "inputs", 6)) {
		vrFprintInput(file, context->input, style);
	} else

	/****************/
	/* input <name> */
	if (!strncmp(query, "input ", 6)) {
		vrGenericInput *input;
		input = vrInputGetFromMapname(context, &query[6]);
#if 0
		vrFprintInputValue(file, input, style);
#else
		/* NOTE: printing the object also prints the value */
		if (input != NULL)
			vrFprintInputObject(file, input->my_object, style);
		else	vrFprintf(file, "unknown input: '%s'\n", &query[6]);
#endif
	} else

	/****************/
	/* inputdevices */
	if (!strncmp(query, "inputdevices", 12)) {
		for (count = 0; count < context->input->num_input_devices; count++) {
			vrFprintf(file, "inputdevice[%d]: ", count);
			vrFprintInputDevice(file, context->input->input_devices[count], one_line);
		}
	} else

	/********************/
	/* inputdevice[<n>] */
	if (!strncmp(query, "inputdevice[", 12)) {
		ival = vrAtoI(&query[12]);
		if (ival >= context->input->num_input_devices) {
			vrFprintf(file, "Invalid input-device request (%d), only %d windows (0..%d).\n", ival, context->input->num_input_devices, context->input->num_input_devices-1);
		} else {
			vrFprintInputDevice(file, context->input->input_devices[ival], style);
		}
	} else

	/***********/
	/* windows */
	if (!strncmp(query, "windows", 7)) {
		if (style == machine) {
			/* For the "machine" style, print a one line list of the windows */
			vrFprintf(file, "%d", config->num_windows);
			for (count = 0; count < config->num_windows; count++) {
				vrFprintf(file, ":%s", config->windows[count]->name);
			}
			vrFprintf(file, "\n");
		} else {
			for (count = 0; count < config->num_windows; count++) {
				vrFprintf(file, "window[%d]: ", count);
				vrFprintWindowInfo(file, config->windows[count], one_line);
			}
		}
	} else

	/***************/
	/* window[<n>] */
	if (!strncmp(query, "window[", 7)) {
		ival = vrAtoI(&query[7]);
		if (ival >= config->num_windows) {
			vrFprintf(file, "Invalid window request (%d), only %d windows (0..%d).\n", ival, config->num_windows, config->num_windows-1);
		} else {
			vrFprintWindowInfo(file, config->windows[ival], style);
		}
	} else

	/*********/
	/* users */
	if (!strncmp(query, "users", 5)) {
		if (style == machine) {
			/* For the "machine" style, print a one line list of the users */
			vrFprintf(file, "%d", config->num_users);
			for (count = 0; count < config->num_users; count++) {
				vrFprintf(file, ":%s", config->users[count]->name);
			}
			vrFprintf(file, "\n");
		} else {
			for (count = 0; count < config->num_users; count++) {
				vrFprintf(file, "user[%d]: ", count);
				vrFprintUserInfo(file, config->users[count], one_line);
			}
		}
	} else

	/*************/
	/* user[<n>] */
	if (!strncmp(query, "user[", 5)) {
		ival = vrAtoI(&query[5]);
		if (ival >= config->num_users) {
			vrFprintf(file, "Invalid user request (%d), only %d users (0..%d).\n", ival, config->num_users, config->num_users-1);
		} else {
			if (style == machine) {
				vrFprintf(file, "%d:", ival);
			}
			vrFprintUserInfo(file, config->users[ival], style);
		}
	} else

	/*********/
	/* eyelists */
	if (!strncmp(query, "eyelists", 8)) {
		for (count = 0; count < config->num_eyelists; count++) {
			vrFprintf(file, "eyelist[%d]: ", count);
			vrFprintEyelistInfo(file, config->eyelists[count], one_line);
		}
	} else

	/*************/
	/* eyelist[<n>] */
	if (!strncmp(query, "eyelist[", 8)) {
		ival = vrAtoI(&query[8]);
vrFprintf(file, "YO num_eyelists = %d, ival = %d\n", config->num_eyelists, ival);
		if (ival >= config->num_eyelists) {
			vrFprintf(file, "Invalid eyelist request (%d), only %d eyelists (0..%d).\n", ival, config->num_eyelists, config->num_eyelists-1);
		} else {
			vrFprintEyelistInfo(file, config->eyelists[ival], style);
		}
	} else

	/*********/
	/* eyes */
	if (!strncmp(query, "eyes", 4)) {
		for (count = 0; count < config->num_eyes; count++) {
			vrFprintf(file, "eye[%d]: ", count);
			vrFprintEyeInfo(file, config->eyes[count], one_line);
		}
	} else

	/*************/
	/* eye[<n>] */
	if (!strncmp(query, "eye[", 4)) {
		ival = vrAtoI(&query[4]);
		if (ival >= config->num_eyes) {
			vrFprintf(file, "Invalid eye request (%d), only %d eyes (0..%d).\n", ival, config->num_eyes, config->num_eyes-1);
		} else {
			vrFprintEyeInfo(file, config->eyes[ival], style);
		}
	} else

	/*************************/
	/* configuration objects */
	if ((!strncmp(query, "object ", 7)) || (!strncmp(query, "o ", 2))) {
		char	argv[5][128];		/* for parsing argument list */
		char	*str_begin;
		int	str_len;

		if (query[1] == ' ')
			str_begin = &query[1];
		else	str_begin = &query[6];

		/* first argument */
		str_begin += strspn(str_begin, whitespace);
		str_len = strcspn(str_begin, whitespace);
		strncpy(argv[1], str_begin, str_len);
		argv[1][str_len] = '\0';
		str_begin += str_len;

		/* second argument */
		str_begin += strspn(str_begin, whitespace);
		str_len = strcspn(str_begin, whitespace);
		strncpy(argv[2], str_begin, str_len);
		argv[2][str_len] = '\0';
		str_begin += str_len;

		vrFprintObjectTypeInfo(file, context, argv[1], argv[2], style);
	} else

	/************/
	/* default: */ {
		vrFprintf(file, "Unknown query: '%s'.\n", query);
	}
}


/***********************************************************/
/* vrInfoSet(): parse a request and attempt to modify the  */
/*   system as requested.  Errors or other messages *may*  */
/*   be sent to the given FILE stream.  See the help_msg   */
/*   string for a list of possible requests.  Passing      */
/*   "help" prints this list.                              */
/* NOTE: this function basically takes the input string    */
/*   with the "set" or "s" (or whatever) stripped off.     */
/***********************************************************/
/* NOTE: this function (and vrInfoGet(), which precedes) could easily*/
/*   be moved to another source file, as they don't have much to do  */
/*   with telnetting, other than this is the only process that makes */
/*   use of it at the moment.                                        */
void vrInfoSet(vrContextInfo *context, FILE *file, char *request, vrPrintStyle style)
{
static	char	*whitespace = " \t\r\b\n";
static	char	*help_msg = "Available settables:\n"
			TAB "help -- print this message (a list of possible setting requests).\n"
			TAB "input <obj> <value> -- set the value of an input.\n"
			TAB TAB "input <6-sensor> { id | loc <x> <y> <z> | move <x> <y> <z> <az> <el> <ro> }.\n"
			TAB "pause {0,1} - pause the entire system.\n"
			TAB "debuglevel|dl <value> -- set the global debug level.\n"
			TAB "debugthistoo|dtt <value> -- set the global debug-this-too.\n"
			TAB "debuglevel|dl[<num>] <value> -- set process <num>'s debug level.\n"
			TAB "debugthistoo|dtt[<num>] <value> -- set process <num>'s debug-this-too.\n"
			TAB "near <value> -- set the global value of the near clipping plane.\n"
			TAB "far <value> -- set the global value of the far clipping plane.\n"
			TAB "window[<num>] ui {0,1} -- turn off/on a window's user-interface display.\n"
			TAB "window[<num>] ui_loc <v1>,<v2> -- relocate a window's user-interface display.\n"
			TAB "window[<num>] ui_color <v1>,<v2>,<v3> -- set color of a window's user-interface display.\n"
			TAB "window[<num>] fps {0,1} -- turn off/on a window's fps display.\n"
			TAB "window[<num>] fps_loc <v1>,<v2> -- relocate a window's fps display.\n"
			TAB "window[<num>] fps_color <v1>,<v2>,<v3> -- set color of a window's fps display.\n"
			TAB "window[<num>] frame {0,1} -- turn off/on a window's frame display in all windows.\n"
			TAB "window[<num>] frm <value> -- set the front rendering mode\n"
			TAB "window[<num>] brm <value> -- set the back rendering mode\n"
			TAB "window[<num>] stats {0,1} -- turn off/on a window's stats display.\n"
			TAB "window[<num>] world {0,1} -- turn off/on a window's virtual world display.\n"
			TAB "window[<num>] nudgex <value> -- shift the window along the X-axis\n"
			TAB "window[<num>] nudgey <value> -- shift the window along the Y-axis\n"
			TAB "window[<num>] nudgez <value> -- shift the window along the Z-axis\n"
			TAB "window[<num>] sim <move command> -- move the view of a simulator window.\n"
			TAB "object window[<name>] <any window command> -- affect a command on any window in the configuration.\n"
			TAB "user[<num>] iod <v1> -- set the iod of a user.\n"
			TAB "user[<num>] color <v1>,<v2>,<v3> -- set the color of a user.\n"
			TAB "proc[<num>] end <v1> -- set the end_proc flag of a process.\n"
			TAB "proc[<num>] done <v1> -- set the proc_done flag of a process.\n"
			TAB "proc[<num>] usec <v1> -- set the usec_min value of a process.\n"
			TAB "proc[<num>] printcolor <v1> -- set the print_color flag of a process.\n"
			TAB "proc[<num>] stats_calc -- set the flag of whether to calculate stats\n"
			TAB "proc[<num>] stats_show -- set the flag of whether to show these stats\n"
			TAB "proc[<num>] stats_mask -- set the mask of which statistics to show\n"
			TAB "proc[<num>] stats_xloc -- set the x location of where to put stats\n"
			TAB "proc[<num>] stats_yloc -- set the y location of where to put stats\n"
			TAB "proc[<num>] stats_width -- set the width of the stats display\n"
			TAB "proc[<num>] stats_top -- set the top timeline of the stats display\n"
			TAB "proc[<num>] stats_interval -- set the horizontal time intervals of the stats display\n"
			TAB "proc[<num>] stats_scale -- set the vertical scale of the stats display (aka time_scale)\n"
			TAB "proc[<num>] stats_color -- set the background color of the stats display\n"
			TAB "proc[<num>] stats_opac -- set the background opacity of the stats display\n"
		"";
	vrConfigInfo	*config = context->config;
	char		*parse;
	int		obj_num;			/* number of the object to modify (ie. proc, user, window) */
	int		value1i, value2i, value3i;	/* some integer values for arguments */
	float		value1f, value2f, value3f;	/* some float values for arguments */

	vrDbgPrintfN(PARSE_DBGLVL, "FreeVR (telnet): Set request received for '%s' [fp = %#p]\n", request, file);


	/********************************************************/
	/*** parse setting here -- very simple-minded parsing ***/
	/********************************************************/

	/********/
	/* help */
	if (!strncmp(request, "help", 4)) {
		vrFprintf(file, help_msg);
	} else

	/***********************/
	/* input <obj> <value> */
	if (!strncmp(request, "input ", 6)) {
		vrGenericInput	*input;

		input = vrInputGetFromMapname(context, &request[6]);
		if (input == NULL) {
			vrFprintf(file, "Unknown variable: '%s'\n", &request[6]);
		} else {
			vrAssignGenericInput(input, strchr(&request[6], ' '));
			if (style == verbose)
				vrFprintInputValue(file, input, brief);
		}
	} else

	/***********************/
	/* pause <value> */
	if (!strncmp(request, "pause ", 6)) {
		context->paused = vrAtoI(&request[6]);
		if (style == verbose)
			vrFprintf(file, "Set System Pause value to %d\n", context->paused);
	} else

	/***********************/
	/* debug_level <value> */
	if (!strncmp(request, "debuglevel ", 11)) {
		config->defaults.debug_level = vrAtoI(&request[11]);
		if (style == verbose)
			vrFprintf(file, "Set Global DebugLevel to %d\n", config->defaults.debug_level);
	} else
	if (!strncmp(request, "dl ", 3)) {
		config->defaults.debug_level = vrAtoI(&request[3]);
		if (style == verbose)
			vrFprintf(file, "Set Global DebugLevel to %d\n", config->defaults.debug_level);
	} else
	if (!strncmp(request, "debuglevel[", 11)) {
		obj_num = vrAtoI(&request[11]);
		if (obj_num >= config->num_procs) {
			vrFprintf(file, "Invalid proc request (%d), only %d processes (0..%d).\n", obj_num, config->num_procs, config->num_procs-1);
		} else if (strchr(&request[11], ']') == NULL) {
			vrFprintf(file, "Invalid proc request -- no closing ']'.\n");
		} else {
			config->procs[obj_num]->settings.debug_level = vrAtoI(strchr(&request[11], ']')+1);
			if (style == verbose)
				vrFprintf(file, "Set Process[%d] DebugLevel to %d\n", obj_num, config->procs[obj_num]->settings.debug_level);
		}
	} else
	if (!strncmp(request, "dl[", 3)) {
		obj_num = vrAtoI(&request[3]);
		if (obj_num >= config->num_procs) {
			vrFprintf(file, "Invalid proc request (%d), only %d processes (0..%d).\n", obj_num, config->num_procs, config->num_procs-1);
		} else if (strchr(&request[3], ']') == NULL) {
			vrFprintf(file, "Invalid proc request -- no closing ']'.\n");
		} else {
			config->procs[obj_num]->settings.debug_level = vrAtoI(strchr(&request[3], ']')+1);
			if (style == verbose) {
				vrFprintf(file, "Set Process[%d] DebugLevel to %d\n", obj_num, config->procs[obj_num]->settings.debug_level);
			}
		}
	} else

	/***********************/
	/* debug_exact <value> */
	if (!strncmp(request, "debugthistoo ", 13)) {
		config->defaults.debug_exact = vrAtoI(&request[13]);
		if (style == verbose)
			vrFprintf(file, "Set Global DebugThisToo to %d\n", config->defaults.debug_exact);
	} else
	if (!strncmp(request, "dtt ", 4)) {
		config->defaults.debug_exact = vrAtoI(&request[4]);
		if (style == verbose)
			vrFprintf(file, "Set Global DebugThisToo to %d\n", config->defaults.debug_exact);
	} else
	if (!strncmp(request, "debugthistoo[", 13)) {
		obj_num = vrAtoI(&request[13]);
		if (obj_num >= config->num_procs) {
			vrFprintf(file, "Invalid proc request (%d), only %d processes (0..%d).\n", obj_num, config->num_procs, config->num_procs-1);
		} else if (strchr(&request[13], ']') == NULL) {
			vrFprintf(file, "Invalid proc request -- no closing ']'.\n");
		} else {
			config->procs[obj_num]->settings.debug_exact = vrAtoI(strchr(&request[13], ']')+1);
			if (style == verbose)
				vrFprintf(file, "Set Process[%d] DebugThisToo to %d\n", obj_num, config->procs[obj_num]->settings.debug_exact);
		}
	} else
	if (!strncmp(request, "dtt[", 4)) {
		obj_num = vrAtoI(&request[4]);
		if (obj_num >= config->num_procs) {
			vrFprintf(file, "Invalid proc request (%d), only %d processes (0..%d).\n", obj_num, config->num_procs, config->num_procs-1);
		} else if (strchr(&request[4], ']') == NULL) {
			vrFprintf(file, "Invalid proc request -- no closing ']'.\n");
		} else {
			config->procs[obj_num]->settings.debug_exact = vrAtoI(strchr(&request[4], ']')+1);
			if (style == verbose)
				vrFprintf(file, "Set Process[%d] DebugThisToo to %d\n", obj_num, config->procs[obj_num]->settings.debug_exact);
		}
	} else

	/****************/
	/* near <value> */
	if (!strncmp(request, "near ", 5)) {
		value1f = atof(&request[5]);
		/* NOTE: "-1.0" represents the "default", and will allow inheritance */
		if ((value1f == -1.0) || (value1f > 0.0)) {
			vrSetNear(context, value1f);
			if (style == verbose)
				vrFprintf(file, "Set Global near clipping plane to %f\n", value1f);
		} else {
			vrFprintf(file, "Unable to set near clipping plane to non-positive value: %f\n", value1f);
		}
	} else

	/***************/
	/* far <value> */
	if (!strncmp(request, "far ", 4)) {
		value1f = atof(&request[4]);
		/* NOTE: "-1.0" represents the "default", and will allow inheritance */
		if ((value1f == -1.0) || (value1f > 0.0)) {
			vrSetFar(context, value1f);
			if (style == verbose)
				vrFprintf(file, "Set Global far clipping plane to %f\n", value1f);
		} else {
			vrFprintf(file, "Unable to set far clipping plane to non-positive value: %f\n", value1f);
		}
	} else

	/******************************************************/
	/* window[<n>] {ui,ui_loc,ui_color,fps,fps_loc,fps_color,nudge[xyz],stats,sim} <value(s)> */
	/* or object window[<name>] {ui,ui_loc,ui_color,fps,fps_loc,fps_color,nudge[xyz],stats,sim} <value(s)> */
	/* TODO: I'd like to also have the ability to give an object window number as another option. */
	if ((!strncmp(request, "window[", 7)) || (!strncmp(request, "object window[", 14))) {
		vrWindowInfo	*window = NULL;

		/* determine the window requested (from the active config or from the complete list */
		if (strchr(&request[7], ']') == NULL) {
			vrFprintf(file, "Invalid window request -- no closing ']'.\n");
		} else if (request[0] == 'w') {
			/* the standard request is for an enumerated active window */
			obj_num = vrAtoI(&request[7]);
			if (obj_num >= config->num_windows) {
				vrFprintf(file, "Invalid window request (%d), only %d windows (0..%d).\n", obj_num, config->num_windows, config->num_windows-1);
			} else {
				parse = strchr(&request[7], ']')+1;	/* begin parsing the command after the object string */
				window = config->windows[obj_num];
			}
		} else if (request[0] == 'o') {
			/* one may also request a named window from the entire configuration */
			parse = strchr(&request[14], ']')+1;	/* begin parsing the command after the object string */
			parse[-1] = '\0';			/* end the object portion of the string at the closing square-bracket */
			window = (vrWindowInfo *)vrObjectSearch(context, VROBJECT_WINDOW, &request[14]);
			parse[-1] = ']';			/* Restore the string for future skipping of command */
			/* TODO: if window == NULL here, check whether a number was given and use that */
			if (window == NULL) {
				vrFprintf(file, "Invalid window request: no window named '%s'.\n", &request[14]);
			}
		}

		/* Now that we have a pointer to a window parse the rest of the request */
		if (window != NULL) {
			parse += strspn(parse, whitespace);	/* skip white */

			if (!strncmp(parse, "ui ", 3)) {
				parse += 3;					/* skip "ui " */
				value1i = vrAtoI(parse);
				window->ui_show = value1i;
				if (style == verbose)
					vrFprintf(file, "set window[%d] ui to %d.\n", obj_num, value1i);
			} else if (!strncmp(parse, "ui_loc ", 7)) {
				parse += 7;					/* skip "ui_loc " */
				value1f = atof(parse);
				parse = (strchr(parse, ',') == NULL ?		/* skip to past next comma */
					parse + strcspn(parse, whitespace) :	/*   or next whitespace    */
					strchr(parse, ',')+1);
				parse += strspn(parse, whitespace);		/* skip white */
				value2f = atof(parse);
				window->ui_loc[0] = value1f;
				window->ui_loc[1] = value2f;
				if (style == verbose)
					vrFprintf(file, "set window[%d] ui_loc to %.2f,%.2f.\n", obj_num, value1f, value2f);
			} else if (!strncmp(parse, "ui_color ", 9)) {
				parse += 9;					/* skip "ui_color " */
				value1f = atof(parse);
				parse = (strchr(parse, ',') == NULL ?		/* skip to past next comma */
					parse + strcspn(parse, whitespace) :	/*   or next whitespace    */
					strchr(parse, ',')+1);
				parse += strspn(parse, whitespace);		/* skip white */
				value2f = atof(parse);
				parse = (strchr(parse, ',') == NULL ?		/* skip to past next comma */
					parse + strcspn(parse, whitespace) :	/*   or next whitespace    */
					strchr(parse, ',')+1);
				parse += strspn(parse, whitespace);		/* skip white */
				value3f = atof(parse);
				window->ui_color[0] = value1f;
				window->ui_color[1] = value2f;
				window->ui_color[2] = value3f;
				if (style == verbose)
					vrFprintf(file, "set window[%d] ui_color to %.3f,%.3f,%.3f.\n", obj_num, value1f, value2f, value3f);
			} else if (!strncmp(parse, "fps ", 4)) {
				parse += 4;					/* skip "fps " */
				value1i = vrAtoI(parse);
				window->fps_show = value1i;
				if (style == verbose)
					vrFprintf(file, "set window[%d] fps to %d.\n", obj_num, value1i);
			} else if (!strncmp(parse, "fps_loc ", 8)) {
				parse += 8;
				value1f = atof(parse);
				parse = (strchr(parse, ',') == NULL ?		/* skip to past next comma */
					parse + strcspn(parse, whitespace) :	/*   or next whitespace    */
					strchr(parse, ',')+1);
				parse += strspn(parse, whitespace);		/* skip white */
				value2f = atof(parse);
				window->fps_loc[0] = value1f;
				window->fps_loc[1] = value2f;
				if (style == verbose)
					vrFprintf(file, "set window[%d] fps_loc to %.2f,%.2f.\n", obj_num, value1f, value2f);
			} else if (!strncmp(parse, "fps_color ", 10)) {
				parse += 10;
				value1f = atof(parse);
				parse = (strchr(parse, ',') == NULL ?		/* skip to past next comma */
					parse + strcspn(parse, whitespace) :	/*   or next whitespace    */
					strchr(parse, ',')+1);
				parse += strspn(parse, whitespace);		/* skip white */
				value2f = atof(parse);
				parse = (strchr(parse, ',') == NULL ?		/* skip to past next comma */
					parse + strcspn(parse, whitespace) :	/*   or next whitespace    */
					strchr(parse, ',')+1);
				parse += strspn(parse, whitespace);		/* skip white */
				value3f = atof(parse);
				window->fps_color[0] = value1f;
				window->fps_color[1] = value2f;
				window->fps_color[2] = value3f;
				if (style == verbose)
					vrFprintf(file, "set window[%d] fps_color to %.3f,%.3f,%.3f.\n", obj_num, value1f, value2f, value3f);
			} else if (!strncmp(parse, "frame ", 6)) {
				parse += 6;				/* skip "frame " */
				value1i = vrAtoI(parse);
				window->show_in_simulator = value1i;
				if (style == verbose)
					vrFprintf(file, "set window[%s] frame to %d.\n", window->name, value1i);
			} else if (!strncmp(parse, "stats ", 6)) {
				parse += 6;				/* skip "stats " */
				value1i = vrAtoI(parse);
				window->stats_show = value1i;
				if (style == verbose)
					vrFprintf(file, "set window[%d] stats to %d.\n", obj_num, value1i);
			} else if (!strncmp(parse, "world ", 6)) {
				parse += 6;				/* skip "world " */
				value1i = vrAtoI(parse);
				window->world_show = value1i;
				if (style == verbose)
					vrFprintf(file, "set window[%d] world to %d.\n", obj_num, value1i);
			} else if (!strncmp(parse, "sim ", 4)) {
				parse += 4;				/* skip "sim " */
				parse += strspn(parse, whitespace);	/* skip extra whitespace */
				vrSimulatorMoveString(window, parse);
			} else if (!strncmp(parse, "frm ", 4)) {
				parse += 4;				/* skip "frm " */
				parse += strspn(parse, whitespace);	/* skip extra whitespace */
				value1f = atof(parse);
				window->frontrendermode = value1f;
			} else if (!strncmp(parse, "brm ", 4)) {
				parse += 4;				/* skip "brm " */
				parse += strspn(parse, whitespace);	/* skip extra whitespace */
				value1f = atof(parse);
				window->backrendermode = value1f;
			} else if (!strncmp(parse, "nudgex ", 7)) {
				parse += 7;				/* skip "nudgex " */
				parse += strspn(parse, whitespace);	/* skip extra whitespace */
				value1f = atof(parse);
				vrMatrixPreTranslate3d(window->rw2w_xform, value1f, 0.0, 0.0);
			} else if (!strncmp(parse, "nudgey ", 7)) {
				parse += 7;				/* skip "nudgey " */
				parse += strspn(parse, whitespace);	/* skip extra whitespace */
				value1f = atof(parse);
				vrMatrixPreTranslate3d(window->rw2w_xform, 0.0, value1f, 0.0);
			} else if (!strncmp(parse, "nudgez ", 7)) {
				parse += 7;				/* skip "nudgez " */
				parse += strspn(parse, whitespace);	/* skip extra whitespace */
				value1f = atof(parse);
				vrMatrixPreTranslate3d(window->rw2w_xform, 0.0, 0.0, value1f);
			} else {
				vrFprintf(file, "Unknown window request: '%s'.\n", request);
			}
		}
	} else

	/**************************************************/
	/* user[<n>] {iod,color} <value(s)> */
	if (!strncmp(request, "user[", 5)) {
		obj_num = vrAtoI(&request[5]);
		if (obj_num >= config->num_users) {
			vrFprintf(file, "Invalid user request (%d), only %d users (0..%d).\n", obj_num, config->num_users, config->num_users-1);
		} else if (strchr(&request[5], ']') == NULL) {
			vrFprintf(file, "Invalid user request -- no closing ']'.\n");
		} else {
			parse = strchr(&request[5], ']')+1;
			parse += strspn(parse, whitespace);	/* skip white */

			if (!strncmp(parse, "iod ", 4)) {
				parse += 4;
				value1f = atof(parse);
				config->users[obj_num]->iod = value1f;
				if (style == verbose)
					vrFprintf(file, "set user[%d] iod to %.3f.\n", obj_num, value1f);
			} else if (!strncmp(parse, "color ", 6)) {
				parse += 6;
				value1i = vrAtoI(parse);
				parse = (strchr(parse, ',') == NULL ?		/* skip to past next comma */
					parse + strcspn(parse, whitespace) :	/*   or next whitespace    */
					strchr(parse, ',')+1);
				parse += strspn(parse, whitespace);		/* skip white */
				value2i = vrAtoI(parse);
				parse = (strchr(parse, ',') == NULL ?		/* skip to past next comma */
					parse + strcspn(parse, whitespace) :	/*   or next whitespace    */
					strchr(parse, ',')+1);
				parse += strspn(parse, whitespace);		/* skip white */
				value3i = vrAtoI(parse);
				config->users[obj_num]->color[0] = value1i;
				config->users[obj_num]->color[1] = value2i;
				config->users[obj_num]->color[2] = value3i;
				if (style == verbose)
					vrFprintf(file, "set user[%d] color to %d,%d,%d.\n", obj_num, value1i, value2i, value3i);
			} else {
				vrFprintf(file, "Unknown user request: '%s'.\n", request);
			}
		}
	} else

	/**************************************************/
	/* proc[<n>] {end,done,printcolor,usec,stats_calc,stats_show,stats_mask,stats_xloc,stats_yloc,stats_width,stats_top,stats_interval,stats_scale,stats_color,stats_opac} <value(s)> */
	if (!strncmp(request, "proc[", 5)) {
		obj_num = vrAtoI(&request[5]);
		if (obj_num >= config->num_procs) {
			vrFprintf(file, "Invalid process request (%d), only %d processes (0..%d).\n", obj_num, config->num_procs, config->num_procs-1);
		} else if (strchr(&request[5], ']') == NULL) {
			vrFprintf(file, "Invalid process request -- no closing ']'.\n");
		} else {
			parse = strchr(&request[5], ']')+1;
			parse += strspn(parse, whitespace);	/* skip white */

			if (!strncmp(parse, "end ", 4)) {
				parse += 4;
				value1i = vrAtoI(parse);
				config->procs[obj_num]->end_proc = value1i;
				if (style == verbose)
					vrFprintf(file, "set proc[%d] end_proc to %d.\n", obj_num, value1i);
			} else if (!strncmp(parse, "done ", 5)) {
				parse += 5;
				value1i = vrAtoI(parse);
				config->procs[obj_num]->proc_done = value1i;
				if (style == verbose)
					vrFprintf(file, "set proc[%d] proc_done to %d.\n", obj_num, value1i);
			} else if (!strncmp(parse, "printcolor ", 11)) {
				parse += 11;
				value1i = vrAtoI(parse);
				config->procs[obj_num]->print_color = value1i;
				vrDbgPrintfN(AALWAYS_DBGLVL, "New print color for proc[%d] is %d\n", obj_num, value1i);
				if (style == verbose)
					vrFprintf(file, "set proc[%d] print_color to %d.\n", obj_num, value1i);
			} else if (!strncmp(parse, "usec ", 5)) {
				parse += 5;
				value1i = vrAtoI(parse);
				config->procs[obj_num]->usec_min = value1i;
				vrDbgPrintfN(AALWAYS_DBGLVL, "New minimal usec delay for proc[%d] is %d\n", obj_num, value1i);
				if (style == verbose)
					vrFprintf(file, "set proc[%d] usec_min to %d.\n", obj_num, value1i);
			} else if (!strncmp(parse, "stats_calc ", 11)) {
				parse += 11;
				value1i = vrAtoI(parse);
				if (config->procs[obj_num]->stats != NULL) {
					config->procs[obj_num]->stats->calc_flag = value1i;
					if (style == verbose)
						vrFprintf(file, "set proc[%d] stats->calc_flag to %d.\n", obj_num, value1i);
				}
			} else if (!strncmp(parse, "stats_show ", 11)) {
				parse += 11;
				value1i = vrAtoI(parse);
				if (config->procs[obj_num]->stats != NULL) {
					config->procs[obj_num]->stats->show_flag = value1i;
					if (style == verbose)
						vrFprintf(file, "set proc[%d] stats->show_flag to %d.\n", obj_num, value1i);
				}
			} else if (!strncmp(parse, "stats_mask ", 11)) {
				parse += 11;
				value1i = vrAtoI(parse);
				if (config->procs[obj_num]->stats != NULL) {
					config->procs[obj_num]->stats->show_mask = value1i;
					if (style == verbose)
						vrFprintf(file, "set proc[%d] stats->show_mask to %d.\n", obj_num, value1i);
				}
			} else if (!strncmp(parse, "stats_xloc ", 11)) {
				parse += 11;
				value1f = atof(parse);
				if (config->procs[obj_num]->stats != NULL) {
					config->procs[obj_num]->stats->xloc = value1f;
					if (style == verbose)
						vrFprintf(file, "set proc[%d] stats->xloc to %f.\n", obj_num, value1f);
				}
			} else if (!strncmp(parse, "stats_yloc ", 11)) {
				parse += 11;
				value1f = atof(parse);
				if (config->procs[obj_num]->stats != NULL) {
					config->procs[obj_num]->stats->yloc = value1f;
					if (style == verbose)
						vrFprintf(file, "set proc[%d] stats->yloc to %f.\n", obj_num, value1f);
				}
			} else if (!strncmp(parse, "stats_width ", 12)) {
				parse += 12;
				value1f = atof(parse);
				if (config->procs[obj_num]->stats != NULL) {
					config->procs[obj_num]->stats->width = value1f;
					if (style == verbose)
						vrFprintf(file, "set proc[%d] stats->width to %f.\n", obj_num, value1f);
				}
			} else if (!strncmp(parse, "stats_top ", 10)) {
				parse += 10;
				value1f = atof(parse);
				if (config->procs[obj_num]->stats != NULL) {
					config->procs[obj_num]->stats->top_time = value1f;
					if (style == verbose)
						vrFprintf(file, "set proc[%d] stats->top_time to %f.\n", obj_num, value1f);
				}
			} else if (!strncmp(parse, "stats_interval ", 15)) {
				parse += 15;
				value1f = atof(parse);
				if (config->procs[obj_num]->stats != NULL) {
					config->procs[obj_num]->stats->hline_interval = value1f;
					if (style == verbose)
						vrFprintf(file, "set proc[%d] stats->hline_interval to %f.\n", obj_num, value1f);
				}
			} else if (!strncmp(parse, "stats_scale ", 12)) {
				parse += 12;
				value1f = atof(parse);
				if (config->procs[obj_num]->stats != NULL) {
					config->procs[obj_num]->stats->time_scale = value1f;
					if (style == verbose)
						vrFprintf(file, "set proc[%d] stats->time_scale to %f.\n", obj_num, value1f);
				}
			} else if (!strncmp(parse, "stats_color ", 12)) {
				parse += 12;
				value1f = atof(parse);
				parse = (strchr(parse, ',') == NULL ?		/* skip to past next comma */
					parse + strcspn(parse, whitespace) :	/*   or next whitespace    */
					strchr(parse, ',')+1);
				parse += strspn(parse, whitespace);		/* skip white */
				value2f = atof(parse);
				parse = (strchr(parse, ',') == NULL ?		/* skip to past next comma */
					parse + strcspn(parse, whitespace) :	/*   or next whitespace    */
					strchr(parse, ',')+1);
				parse += strspn(parse, whitespace);		/* skip white */
				value3f = atof(parse);
				if (config->procs[obj_num]->stats != NULL) {
					config->procs[obj_num]->stats->back_color[0] = value1f;
					config->procs[obj_num]->stats->back_color[1] = value2f;
					config->procs[obj_num]->stats->back_color[2] = value3f;
					if (style == verbose)
						vrFprintf(file, "set proc[%d] stats->back_color to %.3f,%.3f,%.3f.\n", obj_num, value1f, value2f, value3f);
				}
			} else if (!strncmp(parse, "stats_opac ", 11)) {
				parse += 11;
				value1f = atof(parse);
				if (config->procs[obj_num]->stats != NULL) {
					config->procs[obj_num]->stats->back_color[3] = value1f;
					if (style == verbose)
						vrFprintf(file, "set proc[%d] stats->back_color[3] to %f.\n", obj_num, value1f);
				}
			} else {
				vrFprintf(file, "Unknown proc request: '%s'.\n", request);
			}
		}
	} else

	/************/
	/* default: */ {
		vrFprintf(file, "Unknown request: '%s'.\n", request);
	}
}


/************************************************************/
/* vrLockCmd(): parse a request to alter the state of a     */
/*   FreeVR lock, and process it.  This is basically meant  */
/*   for debugging locks -- since they are frequently       */
/*   implemented in a different manner on differing systems.*/
/* NOTE: this function basically takes the input string     */
/*   with the "lock" or "l" (or whatever) stripped off.     */
/************************************************************/
/* NOTE: this function (and vrInfoGet(), which precedes) could easily*/
/*   be moved to another source file, as they don't have much to do  */
/*   with telnetting, other than this is the only process that makes */
/*   use of it at the moment.                                        */
void vrLockCmd(vrContextInfo *context, FILE *file, char *command, vrPrintStyle style)
{
static	char	*whitespace = " \t\r\b\n";
static	char	*help_msg = "Available settables:\n"
			TAB "help|? -- print this message (a list of possible setting requests)\n"
			TAB "num -- print the number of locks in the system\n"
			TAB "print|p <lock> -- print the state of the lock\n"
			TAB "printall|pa -- print the state of all the locks\n"
#if 0
			TAB "trace|t <lock> <name> -- put the lock into tracing mode\n"
			TAB "trace|t <lock> stop -- remove the lock from tracing mode\n"
#else
			TAB "trace|t <lock> <0|1> -- set the lock's tracing flag\n"
#endif
			TAB "setwrite|sw <lock> -- wait for and set the write state for the lock\n"
			TAB "relwrite|rw|uw <lock> -- release the write state for the lock\n"
			TAB "setread|sr <lock> -- wait for and set the write state for the lock\n"
			TAB "relread|rr|ur <lock> -- release the write state for the lock\n"
			TAB "create -- create a new lock\n"
		"";
	vrLock		lock;			/* lock upon which we are to operate */
#if 0
	char		*name;			/* name argument for some commands */
#endif
	char		argv[5][128];		/* for parsing argument lists */
	char		*str_begin;		/* for parsing next argument */
	int		str_len;		/* length of current argument */


	vrTrace("vrLockCmd", "top of vrLockCmd");
	vrDbgPrintfN(PARSE_DBGLVL, "FreeVR (telnet): Lock command received for '%s' [fp = %#p]\n", command, file);


	/********************************************************/
	/*** parse command here -- very simple-minded parsing ***/
	/********************************************************/

	/********/
	/* help */
	if ((!strncmp(command, "help", 4)) || (!strncmp(command, "?", 1))) {
		vrFprintf(file, help_msg);
	} else

	/**********/
	/* num */
	if (!strncmp(command, "num", 3)) {
		vrFprintf(file, "%d\n", vrCountLockList(context->head_lock));
	} else

	/**********/
	/* create */
	if (!strncmp(command, "create", 6)) {
		lock = vrLockCreateName(context, "telnet created");
		vrFprintf(file, "Created new lock: %#p\n", lock);
	} else

	/****************/
	/* print <lock> */
	if ((!strncmp(command, "print ", 6)) || (!strncmp(command, "p ", 2))) {
		if (command[1] == ' ')
			str_begin = &command[2];
		else	str_begin = &command[6];

		lock = (vrLock)vrAtoP(str_begin);

		if (vrLockCheck(lock)) {
			vrFprintLock(file, lock, style);
		} else {
			vrFprintf(file, "Invalid Lock: %#p\n", lock);
		}
	} else

	/*******************/
	/* printall <lock> */
	if ((!strncmp(command, "printall", 8)) || (!strncmp(command, "pa", 2))) {
		lock = context->head_lock;

		if (vrLockCheck(lock)) {
			vrFprintLockList(file, lock, style);
		} else {
			vrFprintf(file, "No locks!\n", lock);
		}
	} else

#if 0
	/************************************/
	/* trace <lock> { <name> | "stop" } */
	if ((!strncmp(command, "trace ", 6)) || (!strncmp(command, "t ", 2))) {
		if (command[1] == ' ')
			str_begin = &command[2];
		else	str_begin = &command[6];

		/* first argument */
		str_begin += strspn(str_begin, whitespace);
		str_len = strcspn(str_begin, whitespace);
		strncpy(argv[1], str_begin, str_len);
		argv[1][str_len] = '\0';
		str_begin += str_len;

		lock = (vrLock)vrAtoP(argv[1]);

		if (vrLockCheck(lock)) {
			/* second argument */
			str_begin += strspn(str_begin, whitespace);
			str_len = strcspn(str_begin, whitespace);
			strncpy(argv[2], str_begin, str_len);
			argv[2][str_len] = '\0';
			str_begin += str_len;

			if (!strncmp(argv[2], "stop", 4))
				name = NULL;
			else	name = argv[2];

			vrLockTrace(lock, name);
			vrFprintf(file, "Tracing lock: %#p as '%s'\n", lock, name);
		} else {
			vrFprintf(file, "Invalid Lock: %#p\n", lock);
		}
#else
	/**********************/
	/* trace <lock> <0|1> */
	if ((!strncmp(command, "trace ", 6)) || (!strncmp(command, "t ", 2))) {
		if (command[1] == ' ')
			str_begin = &command[2];
		else	str_begin = &command[6];

		/* first argument */
		str_begin += strspn(str_begin, whitespace);
		str_len = strcspn(str_begin, whitespace);
		strncpy(argv[1], str_begin, str_len);
		argv[1][str_len] = '\0';
		str_begin += str_len;

		lock = (vrLock)vrAtoP(argv[1]);

		if (vrLockCheck(lock)) {
			/* second argument */
			str_begin += strspn(str_begin, whitespace);
			str_len = strcspn(str_begin, whitespace);
			strncpy(argv[2], str_begin, str_len);
			argv[2][str_len] = '\0';
			str_begin += str_len;

			if (vrAtoI(argv[2])) {
				vrLockTrace(lock, 1);
				vrFprintf(file, "Tracing lock: %#p\n", lock);
			} else {
				vrLockTrace(lock, 0);
				vrFprintf(file, "Not Tracing lock: %#p\n", lock);
			}
		} else {
			vrFprintf(file, "Invalid Lock: %#p\n", lock);
		}
#endif
	} else

	/*******************/
	/* setwrite <lock> */
	if ((!strncmp(command, "setwrite ", 9)) || (!strncmp(command, "sw ", 3))) {
		if (command[2] == ' ')
			str_begin = &command[3];
		else	str_begin = &command[9];

		lock = (vrLock)vrAtoP(str_begin);

		if (vrLockCheck(lock)) {
			vrFprintf(file, "Setting lock %#p for writing -- may have to wait.\n", lock);
			fflush(file);				/* force the string to be sent */
			vrLockWriteSet(lock);
			vrFprintf(file, "Lock %#p now set for writing.\n", lock);
		} else {
			vrFprintf(file, "Invalid Lock: %#p\n", lock);
		}
	} else

	/*******************/
	/* relwrite <lock> */
	if ((!strncmp(command, "relwrite ", 9)) || (!strncmp(command, "rw ", 3)) || (!strncmp(command, "uw ", 3))) {
		if (command[2] == ' ')
			str_begin = &command[3];
		else	str_begin = &command[9];

		lock = (vrLock)vrAtoP(str_begin);

		if (vrLockCheck(lock)) {
			vrFprintf(file, "Releasing lock %#p from writing.\n", lock);
			fflush(file);				/* force the string to be sent */
			vrLockWriteRelease(lock);
			vrFprintf(file, "Lock %#p now released from writing.\n", lock);
		} else {
			vrFprintf(file, "Invalid Lock: %#p\n", lock);
		}
	} else

	/******************/
	/* setread <lock> */
	if ((!strncmp(command, "setread ", 8)) || (!strncmp(command, "sr ", 3))) {
		if (command[2] == ' ')
			str_begin = &command[3];
		else	str_begin = &command[8];

		lock = (vrLock)vrAtoP(str_begin);

		if (vrLockCheck(lock)) {
			vrFprintf(file, "Setting lock %#p for reading -- may have to wait.\n", lock);
			fflush(file);				/* force the string to be sent */
			vrLockReadSet(lock);
			vrFprintf(file, "Lock %#p now set for reading.\n", lock);
		} else {
			vrFprintf(file, "Invalid Lock: %#p\n", lock);
		}
	} else

	/******************/
	/* relread <lock> */
	if ((!strncmp(command, "relread ", 8)) || (!strncmp(command, "rr ", 3)) || (!strncmp(command, "ur ", 3))) {
		if (command[2] == ' ')
			str_begin = &command[3];
		else	str_begin = &command[8];

		lock = (vrLock)vrAtoP(str_begin);

		if (vrLockCheck(lock)) {
			vrFprintf(file, "Releasing lock %#p from reading.\n", lock);
			fflush(file);				/* force the string to be sent */
			vrLockReadRelease(lock);
			vrFprintf(file, "Lock %#p now released from reading.\n", lock);
		} else {
			vrFprintf(file, "Invalid Lock: %#p\n", lock);
		}
	} else

	/************/
	/* default: */ {
		vrFprintf(file, "Unknown lock command: '%s'.\n", command);
	}
}


/************************************************************/
/* vrBarrierCmd(): parse a request to alter the state of a  */
/*   FreeVR barrier, and process it.  This is basically for */
/*   debugging barriers -- since they are built on locks,   */
/*   which are frequently implemented in a different manner */
/*   on differing systems.                                  */
/* NOTE: this function basically takes the input string     */
/*   with the "barrier" or "b" (or whatever) stripped off.  */
/************************************************************/
/* NOTE: this function (and vrInfoGet(), which precedes) could easily*/
/*   be moved to another source file, as they don't have much to do  */
/*   with telnetting, other than this is the only process that makes */
/*   use of it at the moment.                                        */
void vrBarrierCmd(vrContextInfo *context, FILE *file, char *command, vrPrintStyle style)
{
static	char	*help_msg = "Available settables:\n"
			TAB "help|? -- print this message (a list of possible setting requests)\n"
			TAB "num -- print the number of locks in the system\n"
			TAB "print|p <barrier> -- print the state of the barrier\n"
			TAB "printall|pa -- print the state of all the barriers\n"
			TAB "sync <barrier> -- wait for all members to be at the barrier\n"
			TAB "relonce <barrier> -- prematurely release the barrier\n"
			TAB "create -- create a new barrier\n"
			TAB "increment <barrier> -- add one member to the barrier\n"
			TAB "decrement <barrier> -- subtract one member from the barrier\n"
		"";
	vrBarrier	*barrier;		/* barrier upon which we are to operate */
	char		*str_begin;		/* for parsing next argument */
#if 0
static	char	*whitespace = " \t\r\b\n";
	char		argv[5][128];		/* for parsing argument lists */
	int		str_len;		/* length of current argument */
#endif


	vrDbgPrintfN(PARSE_DBGLVL, "FreeVR (telnet): Barrier command received for '%s' [fp = %#p]\n", command, file);


	/********************************************************/
	/*** parse command here -- very simple-minded parsing ***/
	/********************************************************/

	/********/
	/* help */
	if ((!strncmp(command, "help", 4)) || (!strncmp(command, "?", 1))) {
		vrFprintf(file, help_msg);
	} else

	/**********/
	/* num */
	if (!strncmp(command, "num", 3)) {
		vrFprintf(file, "%d\n", vrCountBarrierList(context->head_barrier));
	} else

	/**********/
	/* create */
	if (!strncmp(command, "create", 6)) {
		barrier = vrBarrierCreate(context, "telnet created", 1);
		vrFprintf(file, "Created new barrier: %#p\n", barrier);
	} else

	/****************/
	/* print <barrier> */
	if ((!strncmp(command, "print ", 6)) || (!strncmp(command, "p ", 2))) {
		if (command[1] == ' ')
			str_begin = &command[2];
		else	str_begin = &command[6];

		barrier = (vrBarrier *)vrAtoP(str_begin);

		if (vrBarrierCheck(barrier)) {
			vrFprintBarrier(file, barrier, style);
		} else {
			vrFprintf(file, "Invalid Barrier: %#p\n", barrier);
		}
	} else

	/*******************/
	/* printall <barrier> */
	if ((!strncmp(command, "printall", 8)) || (!strncmp(command, "pa", 2))) {
		barrier = context->head_barrier;

		if (vrBarrierCheck(barrier)) {
			vrFprintBarrierList(file, barrier, style);
		} else {
			vrFprintf(file, "No barriers.\n", barrier);
		}
	} else

	/***************/
	/* sync <barrier> */
	if ((!strncmp(command, "sync ", 5)) || (!strncmp(command, "s ", 2))) {
		if (command[1] == ' ')
			str_begin = &command[2];
		else	str_begin = &command[5];

		barrier = (vrBarrier *)vrAtoP(str_begin);

		if (vrBarrierCheck(barrier)) {
			vrFprintf(file, "Barrier %#p status: %d waiting, first? %d, last? %d.\n",
				barrier,
				barrier->num_waiting,
				vrBarrierFirstToSync(barrier),
				vrBarrierLastToSync(barrier));
			vrFprintf(file, "Synching on barrier %#p -- may have to wait.\n", barrier);
			fflush(file);				/* force the string to be sent */
			vrBarrierSync(barrier);
			vrFprintf(file, "Barrier %#p now synced.\n", barrier);
		} else {
			vrFprintf(file, "Invalid Barrier: %#p\n", barrier);
		}
	} else

	/*******************/
	/* relonce <lock> */
	if ((!strncmp(command, "relonce ", 8)) || (!strncmp(command, "ro ", 3)) || (!strncmp(command, "r ", 2))) {
		if (command[1] == ' ')
			str_begin = &command[2];
		else if (command[2] == ' ')
			str_begin = &command[3];
		else	str_begin = &command[8];

		barrier = (vrBarrier *)vrAtoP(str_begin);

		if (vrBarrierCheck(barrier)) {
			vrFprintf(file, "Barrier %#p status: %d waiting, first? %d, last? %d.\n",
				barrier,
				barrier->num_waiting,
				vrBarrierFirstToSync(barrier),
				vrBarrierLastToSync(barrier));
			vrFprintf(file, "Releasing Barrier %#p prematurely.\n", barrier);
			fflush(file);				/* force the string to be sent */
			vrBarrierReleaseOnce(barrier);
			vrFprintf(file, "Barrier %#p now released from writing.\n", barrier);
		} else {
			vrFprintf(file, "Invalid Barrier: %#p\n", barrier);
		}
	} else

	/********************/
	/* increment <lock> */
	if ((!strncmp(command, "increment ", 10)) || (!strncmp(command, "i ", 2))) {
		if (command[1] == ' ')
			str_begin = &command[2];
		else	str_begin = &command[10];

vrPrintf("str_begin = '%s'\n", str_begin);
		barrier = (vrBarrier *)vrAtoP(str_begin);
vrPrintf("barrier = %p\n", barrier);
vrPrintf("vrBarrierCheck(barrier) = %d\n", vrBarrierCheck(barrier));


		if (vrBarrierCheck(barrier)) {
			vrBarrierIncrement(barrier, 1);
			vrFprintf(file, "Incremented members of barrier %#p to %d.\n", barrier, barrier->num_clients);
		} else {
			vrFprintf(file, "Invalid Barrier: %#p\n", barrier);
		}
	} else

	/********************/
	/* decrement <lock> */
	if ((!strncmp(command, "decrement ", 10)) || (!strncmp(command, "d ", 2))) {
		if (command[1] == ' ')
			str_begin = &command[2];
		else	str_begin = &command[10];

		barrier = (vrBarrier *)vrAtoP(str_begin);

		if (vrBarrierCheck(barrier)) {
			vrBarrierDecrement(barrier, 1);
			vrFprintf(file, "Decremented members of barrier %#p to %d.\n", barrier, barrier->num_clients);
		} else {
			vrFprintf(file, "Invalid Barrier: %#p\n", barrier);
		}
	} else

	/************/
	/* default: */ {
		vrFprintf(file, "Unknown barrier command: '%s'.\n", command);
	}
}


/************************************************************/
/* vrEcho(): echo a message back to the given FILE stream.  */
/*   This primary use for this is to evaluate any variables */
/*   found in the string to be echoed.                      */
/* NOTE: this function basically takes the input string    */
/*   with the "echo" or "e" (or whatever) stripped off.    */
/* NOTE: the echoing will end when reaching a semicolon or */
/*   a newline.                                            */
/***********************************************************/
/* NOTE: this function also could easily be moved to another     */
/*   source file, as they don't have much to do with telnetting, */
/*   other than this is the only process that makes use of it at */
/*   the moment.                                                 */
void vrEcho(vrContextInfo *context, FILE *file, char *string)
{
static	char		*whitespace = " \t\r\b";
static	char		*strnumchars = "_:abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.0123456789";
	int		count;
	int		string_length;
	char		*var_begin;		/* where the variable name begins in string */
	int		var_len;		/* length of variable name (less one) */
	char		variable_name[256];	/* copy of the full variable name */
	char		*variable_value;	/* returned value of the variable */

	string_length = strlen(string);

	for (count = 0; count < string_length; count++) {
		if (string[count] == ';' || string[count] == '\n') {
			/* the semicolon and newline indicate the end of this command, so break out of the loop */
			break;
		} else if (string[count] != '$') {
			/* for all other characters (except the '$' which indicates a variable), just echo it */
			vrFprintf(file, "%c", string[count]);
		} else {
			var_begin = &string[count+1];	/* variable name begins right after '$' */
			if (strspn(var_begin, whitespace) > 0) {
				/* if whitespace follows the '$' then just print the '$' and move on */
				vrFprintf(file, "%c", string[count]);
			} else {
				/* otherwise treat the next character and all alphanums following as variable name */
				/* NOTE: this will pass on non-alphanumerics in the first position */
				/*   for special operations such as the $?<var> construct.         */
				var_len = strspn(var_begin+1, strnumchars);
				strncpy(variable_name, var_begin, var_len+1);
				variable_name[var_len+1] = '\0';
				count += var_len+1;

				variable_value = vrEvaluateVariable(context, variable_name);
				if (variable_value != NULL)
					vrFprintf(file, "%s", variable_value);
				else	vrFprintf(file, "<null>");
			}

		}
	}
	vrFprintf(file, "\n");
}


/**********************************************************/
/* vrSetenv(): Set the value of an environment variable.  */
/* NOTE: this function basically takes the input string   */
/*   with the "setenv" (or whatever) stripped off.        */
/*****************************************************************/
/* NOTE: this function also could easily be moved to another     */
/*   source file, as they don't have much to do with telnetting, */
/*   other than this is the only process that makes use of it at */
/*   the moment.                                                 */
void vrSetenv(FILE *file, char *string)
{
static	char		*whitespace = " \t\r\b";
static	char		variable_name[256];	/* copy of the full variable name */
static	char		assignment_string[1024];/* string to give to putenv to make assignment */
	int		var_len;		/* length of variable name (less one) */
	char		*variable_value;	/* returned value of the variable */

	string += strspn(string, whitespace);	/* skip white */
	var_len = strcspn(string, whitespace);	/* measure non-whitespace -- this is the variable name */
	strncpy(variable_name, string, var_len);
	variable_value = string + var_len + strspn(string+var_len, whitespace);

	sprintf(assignment_string, "%s=%s", variable_name, variable_value);
vrPrintf("making assignment '%s'\n", assignment_string);

	if (putenv(assignment_string)) {
		vrErrPrintf(RED_TEXT "FreeVR telnet: Unable to set the variable '%s'\n" NORM_TEXT, variable_name);
		vrFprintf(file, "Unable to make assign value to environment variable\n");
	}
}


/***********************************************************/
/* vrUnsetenv(): Set the value of an environment variable. */
/* NOTE: this function basically takes the input string    */
/*   with the "unsetenv" (or whatever) stripped off.       */
/*****************************************************************/
/* NOTE: this function also could easily be moved to another     */
/*   source file, as they don't have much to do with telnetting, */
/*   other than this is the only process that makes use of it at */
/*   the moment.                                                 */
/* NOTE: this doesn't work under IRIX, and perhaps other Unixii. */
void vrUnsetenv(FILE *file, char *string)
{
static	char		*whitespace = " \t\r\b";
static	char		variable_name[256];	/* copy of the full variable name */
static	char		assignment_string[1024];/* string to give to putenv to make assignment */
	int		var_len;		/* length of variable name (less one) */

	string += strspn(string, whitespace);	/* skip white */
	var_len = strcspn(string, whitespace);	/* measure non-whitespace -- this is the variable name */
	strncpy(variable_name, string, var_len);

	/* NOTE: this doesn't work under IRIX, and perhaps other Unixii */
	sprintf(assignment_string, "%s", variable_name);
vrPrintf("making assignment '%s'\n", assignment_string);

	if (putenv(assignment_string)) {
		vrErrPrintf(RED_TEXT "FreeVR telnet: Unable to unset the variable '%s'\n" NORM_TEXT, variable_name);
		vrFprintf(file, "Unable to unset the environment variable\n");
	}
}


/******************************************************************/
/* vrTelnetInitProc(): Initialize the socket communication with   */
/*   this process.                                                */
/******************************************************************/
void vrTelnetInitProc(vrProcessInfo *myproc_info)
{
	_TelnetPrivate	*aux;			/* pointer to auxiliary data of the telnet process */
	int		result;			/* used to parse socket port */
	int		inet_port_max;		/* highest number of port to attempt opening */

	vrTrace("vrTelnetInitProc", "beginning");
	proc_info = myproc_info;

#if 0 /* TODO: readd this -- not doing this allows for seg faults, so we can debug easier */
	vrSetSignalHandler(vrTelnetSignalHandler);
#endif

	/***************************************************************/
	/* create your own little memory space for proc-specific stuff */
	myproc_info->aux_data = (_TelnetPrivate *)vrShmemAlloc(sizeof(_TelnetPrivate));
	aux = myproc_info->aux_data;


	/*******************************************************/
	/* set some initial values for the auxiliary structure */
	aux->inet_port = 3000;		/* the current default socket port */
	aux->inet_port_range = 2;	/* the number of other (consecutive) ports to try */
	aux->listen_sock = -1;		/* -1 indicates ... */
	aux->cmd_socket = -1;		/* -1 indicates no one is connected */
	aux->cmd_fp = NULL;
	aux->style = verbose;		/* default to verbose query outputs */
	aux->prompt = vrShmemStrDup(BOLD_TEXT "FreeVR>" NORM_TEXT);
	aux->password = vrShmemStrDup("");


	/****************************************************************/
	/* adjust auxiliary structure values based on process arguments */
	if (vrArgParseInteger(myproc_info->args, "port", &result)) {
		aux->inet_port = result;
	}

	if (vrArgParseInteger(myproc_info->args, "portrange", &result)) {
		aux->inet_port_range = result;
	}

	vrArgParseString(myproc_info->args, "prompt", &(aux->prompt));

	vrArgParseString(myproc_info->args, "password", &(aux->password));


	/********************************************/
	/* output debugging info about this process */
	if (vrDbgDo(DEFAULT_DBGLVL+1)) {
		vrMsgPrintf("FreeVR: Telnet proc is: ");
		vrFprintProcessInfo(stdout, myproc_info, verbose);
	}


	/******************************/
	/* initialize the socket port */
	vrTrace("vrTelnetInitProc", "initializing socket port");
	gethostname(aux->host_machine, 128);

	/* if range is 0 or less, wait for specific port */
	if (aux->inet_port_range > 0) {
		/* loop through N possible Internet ports to hopefully find one that works */
		vrDbgPrintf("FreeVR (telnet): Attempting to use ");
		for (inet_port_max = aux->inet_port + aux->inet_port_range; aux->listen_sock < 0 && aux->inet_port <= inet_port_max; aux->inet_port++) {
			vrDbgPrintf("port %d ... ", aux->inet_port);
			vrDbgFlush();
			aux->listen_sock = vrSocketCreateListen(&aux->inet_port, 0);
		}
		aux->inet_port--;
		vrDbgPrintf("\n");
	} else {
		/* loop over time until the requested socket port is available */
		aux->listen_sock = vrSocketCreateListen(&aux->inet_port, 0);
		while (aux->listen_sock < 0) {
			vrMsgPrintf("FreeVR (telnet): " RED_TEXT "Port %d -- unable to open, will continue to try.\n" NORM_TEXT, aux->inet_port);
			vrSleep(5000000);
			aux->listen_sock = vrSocketCreateListen(&aux->inet_port, 0);

			/* break out of the waiting for socket loop if this process has been set for termination */
			if (myproc_info->end_proc)
				break;
		}
	}

	if (aux->listen_sock < 0) {
		vrTrace("vrTelnetInitProc", "unable to find an open socket");
		vrMsgPrintf("FreeVR (telnet): " RED_TEXT "WARNING: Unable to find an open socket port.\n" NORM_TEXT);
	} else {
		vrDbgPrintfN(TRACE_DBGLVL, "(%s::%d) %s -> %s %s:%d\n", __FILE__, __LINE__, "vrTelnetInitProc", "Listening on socket", aux->host_machine, aux->inet_port);
		vrMsgPrintf("\rFreeVR (telnet): Connect to '%s:%d' to send commands to FreeVR\n", aux->host_machine, aux->inet_port);
	}

	/* prepare the parsing buffer */
	aux->parse = &(aux->inbuf[0]);
	aux->parse[0] = '\0';
}


/*************************************************************/
/* vrTelnetTermProc(): Close the socket port associated with */
/*   this process.                                           */
/*************************************************************/
void vrTelnetTermProc(vrProcessInfo *myproc_info)
{
	_TelnetPrivate	*aux;			/* pointer to auxiliary data of the telnet process */

	aux = myproc_info->aux_data;
	if (aux->listen_sock > 0)
		vrSocketClose(aux->listen_sock);
}


/******************************************************************/
/* vrTelnetOneFrame(): Handle one interface with the user.        */
/******************************************************************/
void vrTelnetOneFrame(vrProcessInfo *myproc_info)
{
static	char	*help_msg = "Socket commands:\n"
			TAB "help -- this message\n"
			TAB "close -- close the connection\n"
			TAB "quit -- quit the telnet process\n"
		  	TAB "term -- terminate FreeVR application\n"
		  /*	TAB "reset -- nothing (yet)\n"						*/
			TAB "pause -- toggle the system pause status\n"
			TAB "echo <string> -- echo the string back to the output, with variable substitutions\n"
			TAB "verbose -- set verbose printing mode\n"
			TAB "brief -- set brief printing mode\n"
			TAB "one_line -- set one_line printing mode\n"
			TAB "machine -- set print style to machine mode\n"
			TAB "fileformat -- set the print style to file_format mode\n"
			TAB "print|p <query> -- print some info about the FreeVR app\n"
			TAB "printfile|pf [+]<filename> <query> -- print some info about the FreeVR app to a file\n"
			TAB "set|s <request> -- set the value of part of the system\n"
			TAB "setenv <envvar> <value> -- set the value of an environment variable\n"
			TAB "unsetenv <envvar> -- unset the environment variable\n"
			TAB "lock <command> -- send a command to a FreeVR lock\n"
			TAB "barrier <command> -- send a command to a FreeVR barrier\n"
			TAB "relbar <address> -- release the barrier at that address\n"
		/* :-(	TAB "procdown <num> -- stop the given process\n"			*/
		/* :-(	TAB "procup <name> -- start a process of the given name\n"	*/
		"";
static	char		*whitespace = " \t\r\b";
	vrContextInfo	*context = vrContext;	/* TODO: get rid of this once we pass context as handle */
	_TelnetPrivate	*aux;			/* pointer to auxiliary data of the telnet process */
	int		result;			/* used to parse socket port */

	aux = myproc_info->aux_data;

	/* calculate frame rates and set the process time values */
	myproc_info->frame_count++;
	vrProcessCalcFrameRate(myproc_info);

	/* measure: update time measurement array index */
	vrProcessStatsNextFrame(myproc_info->stats);

	/*******************************/
	/* Handle Commands from socket */

	/* if no connection (and a listen socket is open), see if someone is calling */
	if (aux->cmd_socket < 0 && aux->listen_sock >= 0) {
		/* aux->cmd_socket will be -1 when no one is connected */

		aux->cmd_socket = vrSocketAnswer(aux->listen_sock);
		if (aux->cmd_socket > 0) {
			vrTrace("vrTelnetOneFrame", "socket connection established");
			vrDbgPrintfN(PARSE_DBGLVL, "FreeVR (telnet): New client connected.\n");
			aux->cmd_fp = fdopen(aux->cmd_socket, "r+");

			/* Verify password from socket */
			if (strlen(aux->password) > 0) {
/*** TODO: turn off echoing!! ***/
				vrSocketSendMsg(aux->cmd_socket, "Enter FreeVR password [NOTE: currently visible (TODO: make invisible)]> ");

				/* wait for the password input */
				while (vrSocketReadMsgClean(aux->cmd_socket, aux->inbuf, sizeof(aux->inbuf)) <= 0)
					vrSleep(myproc_info->usec_min);

				/* compare the given password with config password */
				/* TODO: use an encoded password */
				if (!strcmp(aux->password, aux->inbuf)) {
					vrDbgPrintfN(PARSE_DBGLVL, "FreeVR (telnet): Correct password given.\n");
				} else {
					vrDbgPrintfN(PARSE_DBGLVL, "FreeVR (telnet): " RED_TEXT "Incorrect password given.\n" NORM_TEXT);
					vrSocketSendMsg(aux->cmd_socket, "Incorrect password.\n");
					close(aux->cmd_socket);
					aux->cmd_socket = -1;
				}

/*** TODO: turn on echoing!! ***/
			}

			/* if correct password, then send a prompt */
			if ((aux->cmd_socket > 0) && (aux->style != machine)) {
				vrSocketSendMsg(aux->cmd_socket, aux->prompt);
			}
		}
	}

	/* if a connection, see if client sent a command.  If so, act on it */
	if (aux->cmd_socket > 0) {

		/* If no leftover material then refill input buffer */
		if (aux->parse[0] == '\0') {
			result = vrSocketReadMsgClean(aux->cmd_socket, aux->inbuf, sizeof(aux->inbuf));
			aux->parse = aux->inbuf;
		}

		switch (result) {
		case  EOF:
			vrTrace("vrTelnetOneFrame", "socket connection closed");
			vrDbgPrintfN(PARSE_DBGLVL, "FreeVR (telnet): Client stopped communicating.\n");
			close(aux->cmd_socket);	/* NOTE: vrSocketClose() is not used because it is for the listen socket. */
			fclose(aux->cmd_fp);
			aux->cmd_socket = -1;
			break;

		/* NOTE: previously "0" indicated EOF, but now it simply indicates a 0-length message. */
		/*   So now what used to be the error number (-1) is used by EOF, so we use -2 to      */
		/*   indicate an actual error.                                                         */
		case -2:
			vrDbgPrintfN(PARSE_DBGLVL, "FreeVR (telnet): socket read error\n");
			break;
		default:
			vrDbgPrintfN(PARSE_DBGLVL, "FreeVR (telnet): socket cmd = '%s'\n", aux->parse);

	/* TODO: allow a programmer-defined callback to be inserted here to */
	/*   have first dibs at the incoming commands.  OR, call either the */
	/*   callback, or a function containing the following commands,     */
	/*   which itself might be called from the programmer code.  Also,  */
	/*   we might want to have the option of a DSO-based function being */
	/*   used to do the parsing, etc.  With the DSO function defined in */
	/*   the process-config arguments.                                  */

			/* first, skipover whitespace */
			aux->parse += strspn(aux->parse, whitespace);			/* skip white */

			/* parse command here -- very simple-minded parsing */
			vrTrace("vrTelnetOneFrame", BOLD_TEXT "parsing input" NORM_TEXT);

			/********/
			/* quit */
			if (!strncmp(aux->parse, "quit", 4)) {
				/* set the process for termination */
				myproc_info->end_proc = 1;
			} else

			/********/
			/* term */
			/* TODO: this should eventually be handled as an "alias" */
			if (!strncmp(aux->parse, "term", 4)) {
				/* terminate FreeVR */
#if 0
				vrExit();
				exit(0);
#else /* kludge method is to set the first 2-way input to 1 */
				vrInfoSet(context, aux->cmd_fp, "input 2-way[0] 1", aux->style);
				fflush(aux->cmd_fp);	/* socket fp's don't flush often */
#endif
			} else

			/********/
			/* help */
			if (!strncmp(aux->parse, "help", 4) || !strncmp(aux->parse, "?", 1)) {
				/* give usage information */
				vrSocketSendMsg(aux->cmd_socket, help_msg);
			} else

			/*********/
			/* close */
			if (!strncmp(aux->parse, "close", 5)) {
				/* close socket connection */
				vrDbgPrintfN(PARSE_DBGLVL, "FreeVR (telnet): Client ending communication.\n");
				close(aux->cmd_socket);
				aux->cmd_socket = -1;
			} else

			/*********/
			/* reset */
			if (!strncmp(aux->parse, "reset", 5)) {
				/* make the system/world as it was in the beginning */

				/* TODO: do stuff */

			} else

			/*********************************************/
			/* verbose|brief|one_line|machine|fileformat */
			if (!strncmp(aux->parse, "file", 4)) {
				aux->style = file_format;
			} else
			if (!strncmp(aux->parse, "verbose", 7)) {
				aux->style = verbose;
			} else
			if (!strncmp(aux->parse, "brief", 5)) {
				aux->style = brief;
			} else
			if (!strncmp(aux->parse, "one_line", 8)) {
				aux->style = one_line;
			} else
			if (!strncmp(aux->parse, "machine", 7)) {
				aux->style = machine;
				vrSocketSendMsg(aux->cmd_socket, "\n");
				fflush(aux->cmd_fp);
			} else
			if (!strncmp(aux->parse, "fileformat", 10)) {
				aux->style = file_format;
			} else

			/*********/
			/* print */
			if (!strncmp(aux->parse, "print ", 6)) {
				/* print some information about the system */
				vrInfoGet(context, aux->cmd_fp, &aux->parse[6], aux->style);
				fflush(aux->cmd_fp);	/* socket fp's don't flush often */
			} else
			if (!strncmp(aux->parse, "p ", 2)) {
				/* (shortcut) print some information about the system */
				vrInfoGet(context, aux->cmd_fp, &aux->parse[2], aux->style);
				fflush(aux->cmd_fp);	/* socket fp's don't flush often */
			} else

			/*************/
			/* printfile */
			if ((!strncmp(aux->parse, "printfile ", 10)) || (!strncmp(aux->parse, "pf ", 3))) {
				/* print some information about the system to a file */
				FILE	*file;		/* the file to output to */
				char	*parse;		/* the next part of the buffer to parse */
				char	*filename;	/* the name of the file to output to */
				char	*opentype = "w";/* how to write to the file (from beginning[default], or append) */

				/* skip the operation name */
				parse = aux->parse + (aux->parse[1] == 'r' ? 10 : 3);
				if (parse[2] == ' ')
					parse = parse + 3;

				filename = parse;

				parse += strcspn(parse, whitespace);		/* skip non-white */
				parse[0] = '\0';
				parse++;
				parse += strspn(parse, whitespace);		/* skip non-white */

				/* open the file */
				/* NOTE: if filename begins with a '+' do an append */
				if (filename[0] == '+') {
					opentype = "a";
					filename++;
				}

				file = fopen(filename, opentype);

				/* output the information */
				if (file != NULL) {
					vrInfoGet(context, file, parse, aux->style);
					fclose(file);
				} else {
					vrSocketSendMsg(aux->cmd_socket, "Unable to open file for output.\n");
				}
			} else

			/*******/
			/* set */
			if (!strncmp(aux->parse, "set ", 4)) {
				/* change some information about the system */
				vrInfoSet(context, aux->cmd_fp, &aux->parse[4], aux->style);
				fflush(aux->cmd_fp);	/* socket fp's don't flush often */
			} else
			if (!strncmp(aux->parse, "s ", 2)) {
				/* change some information about the system */
				vrInfoSet(context, aux->cmd_fp, &aux->parse[2], aux->style);
				fflush(aux->cmd_fp);	/* socket fp's don't flush often */
			} else

			/********/
			/* lock */
			if (!strncmp(aux->parse, "lock ", 5)) {
				/* change some information about the lock */
				vrLockCmd(context, aux->cmd_fp, &aux->parse[5], aux->style);
				fflush(aux->cmd_fp);	/* socket fp's don't flush often */
			} else
			if (!strncmp(aux->parse, "l ", 2)) {
				/* change some information about the lock */
				vrLockCmd(context, aux->cmd_fp, &aux->parse[2], aux->style);
				fflush(aux->cmd_fp);	/* socket fp's don't flush often */
			} else

			/***********/
			/* barrier */
			if (!strncmp(aux->parse, "barrier ", 8)) {
				/* change some information about the barrier */
				vrBarrierCmd(context, aux->cmd_fp, &aux->parse[8], aux->style);
				fflush(aux->cmd_fp);	/* socket fp's don't flush often */
			} else
			if (!strncmp(aux->parse, "b ", 2)) {
				/* change some information about the barrier */
				vrBarrierCmd(context, aux->cmd_fp, &aux->parse[2], aux->style);
				fflush(aux->cmd_fp);	/* socket fp's don't flush often */
			} else

			/**********/
			/* relbar */
			if (!strncmp(aux->parse, "relbar ", 7)) {
				/* release a barrier just once */
				vrBarrierReleaseOnce((void *)vrAtoP(&aux->parse[7]));
				fflush(aux->cmd_fp);	/* socket fp's don't flush often */
			} else

			/*********/
			/* pause */
			if (!strncmp(aux->parse, "pause", 5)) {
				/* toggle the system pause value */
				context->paused ^= 1;
				fflush(aux->cmd_fp);	/* socket fp's don't flush often */
			} else

			/**********/
			/* setenv */
			if (!strncmp(aux->parse, "setenv ", 7)) {
				/* set a machine environment variable */
				vrSetenv(aux->cmd_fp, &aux->parse[7]);
				fflush(aux->cmd_fp);	/* socket fp's don't flush often */
			} else

			/********/
			/* echo */
			if (!strncmp(aux->parse, "echo ", 5)) {
				/* echo command -- repeat string to output w/ variable substitutions */
				vrEcho(context, aux->cmd_fp, &aux->parse[5]);
				fflush(aux->cmd_fp);	/* socket fp's don't flush often */
			} else

			/*******/
			/* nop */
			if (!strcmp(aux->parse, "")) {
				/* empty command -- do nothing */
				fflush(aux->cmd_fp);	/* socket fp's don't flush often */
			} else

			/************/
			/* default: */ {
				vrSocketSendMsg(aux->cmd_socket, "Unknown command.");
				vrDbgPrintfN(PARSE_DBGLVL, "FreeVR (telnet): Unknown command warning sent.\n", aux->parse);
			}


			/* shift the parse pointer to be after the first semicolon or newline */
			/*   (ie. in case there is an additional command in the buffer)       */
			aux->parse += strcspn(aux->parse, ";\n");		/* skip to ;|NL */
			aux->parse++;						/* skip over ;|NL */


			/***************************************************************************/
			/* prompt for another command (if socket still open & no command in inbuf) */
			if ((aux->cmd_socket > 0) && (aux->style != machine) && (aux->parse[0] == '\0')) {
				vrSocketSendMsg(aux->cmd_socket, aux->prompt);
			}
		}
	}
}


/********************************************************/
/* the main loop for each telnet communications process */
/********************************************************/
void vrTelnetMainLoop(vrProcessInfo *myproc_info)
{
#if 0
static	char	*help_msg = "Socket commands:\n"
			TAB "help -- this message\n"
			TAB "close -- close the connection\n"
			TAB "quit -- quit the telnet process\n"
		  	TAB "term -- terminate FreeVR application\n"
		  /*	TAB "reset -- nothing (yet)\n"						*/
			TAB "pause -- toggle the system pause status\n"
			TAB "echo <string> -- echo the string back to the output, with variable substitutions\n"
			TAB "verbose -- set verbose printing mode\n"
			TAB "brief -- set brief printing mode\n"
			TAB "one_line -- set one_line printing mode\n"
			TAB "machine -- set print style to machine mode\n"
			TAB "fileformat -- set the print style to file_format mode\n"
			TAB "print|p <query> -- print some info about the FreeVR app\n"
			TAB "printfile|pf [+]<filename> <query> -- print some info about the FreeVR app to a file\n"
			TAB "set|s <request> -- set the value of part of the system\n"
			TAB "setenv <envvar> <value> -- set the value of an environment variable\n"
			TAB "unsetenv <envvar> -- unset the environment variable\n"
			TAB "lock <command> -- send a command to a FreeVR lock\n"
			TAB "barrier <command> -- send a command to a FreeVR barrier\n"
			TAB "relbar <address> -- release the barrier at that address\n"
		/* :-(	TAB "procdown <num> -- stop the given process\n"			*/
		/* :-(	TAB "procup <name> -- start a process of the given name\n"	*/
		"";
static	char		*whitespace = " \t\r\b";
	vrContextInfo	*context = vrContext;	/* TODO: get rid of this once we pass context as handle */
	int		result;			/* used to get socket input result */
#endif
	_TelnetPrivate	*aux;			/* pointer to auxiliary data of the telnet process */
	vrTime		loop_wtime;		/* time of the beginning of each loop */

#if 0 /* Moving this to vr_procs.c */
	vrTelnetInitProc(myproc_info);
#endif
	aux = myproc_info->aux_data;

	/*************************/
	/*** Main process loop ***/
	/*************************/

	vrTrace("vrTelnetMainLoop", BOLD_TEXT "beginning process loop" NORM_TEXT);
	loop_wtime = vrCurrentWallTime();
	while (!myproc_info->end_proc) {

		/* NOTE: I'm not sure why the frame delay and barrier stuff are at the top */
		/*   of this loop -- probably due to how things work for the visren loop.  */

		/**************************/
		/* do minimal frame delay */
		vrSleep(myproc_info->usec_min - (long)((vrCurrentWallTime() - loop_wtime) * 1000000.0));

		/*************************************************************/
		/* Sync with other processes in group & calculate frame info */
		vrProcessSync(myproc_info, 0, 0);		/* NOTE: the last two arguments are for stats info that we don't worry about for telnet */

		/* start measuring for minimal loop time */
		loop_wtime = vrCurrentWallTime();

		vrTelnetOneFrame(myproc_info);
	}

	if (aux->listen_sock > 0)
		vrSocketClose(aux->listen_sock);

	vrTrace("vrTelnetMainLoop", "ending");
}

