/* ======================================================================
 *
 *  CCCCC          serialspy.c
 * CC   CC         Author(s): Bill Sherman
 * CC              Created: January 5, 2000
 * CC   CC         Last Modified: September 18, 2010
 *  CCCCC
 *
 * Code file for spying on the bytes going through (ie. coming from)
 *   a serial cable.  Intended to help make sure serial devices are
 *   functioning properly, and also to help reverse engineer data
 *   protocols used by said devices.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/*************************************************************************

USAGE:
	serialspy [-l <line>] [-b <baud>] [-f <frequency>] [-1] [-pseudo] [-pt]
		[-mb <max bytes>] [-flag <byte>] [-asc|-noasc] [-bin|-nobin] [-hex|-nohex]

	While running, the following keys can be used to adjust some parameters:
	  [NOTE: this feature is disabled when in "passthru" mode]
		'q' - quit
		'?' - print help information for the interactive keys
		'+' - increase the frequency
		'-' - decrease the frequency
		'>' - shift the phase to the right
		'<' - shift the phase to the left
		'0' - print remaining bytes in buffer (essentially a big phase shift left)
		'a' - toggle ASCII output display
		'b' - toggle binary output display
		'h' - toggle hex output display
		(backspace) - go into passthru mode (from which there
			currently is no return).

HISTORY:
	5 January 2000 -- First version written by Bill Sherman, with help
		from Stuart Levy on the non-blocked reading of characters
		from the stdin terminal.  Tested on SGI IRIX 6.2 (works!),
		and Linux Redhat 6.0 (also works!).
		[corresponds to Freevr Version 0.3b]

	1 March 2000 or 16 February 2003 (Bill Sherman) -- turned on the definition
		of FIONREAD, which seems to be required by many versions of
		Linux (and included a comment about at least two of them).
		[the latter date corresponds to Freevr Version 0.4e]

	21 October 2005 (Bill Sherman) -- Added the "one_time" feature that
		reads whatever is in the buffer, displays it, and then quits.
		A handy feature for clearing the buffer.
		[corresponds to Freevr Version 0.5e]

	25 September 2006 (Bill Sherman) -- Added the ability to send characters
		out through the serial port (including a feature to disable
		the interactive keys, so they too can be sent to the port).
		This has encouraged me to think more about implementing the
		desired feature of connecting two (logical) serial ports
		through serialspy and watching traffic in both directions.

		I also fixed a small bug that I just discovered, where the
		last incoming character would not be printed at the time of
		reading, but rather would be included as the first character
		of the next batch.  (Basically a ">" vs. ">=" error.)
		[corresponds to Freevr Version 0.5g]

	27 September 2006 (Bill Sherman) -- Added the ability to act as
		man-in-the-middle, acting as a pass through between a pseudo
		tty and the actual serial port, and printing out the values
		going both ways.

		I separated out the printing routines into a separate function.
		By doing this, I can use the same format for the data going to
		the serial device as I can for the data coming from it.

		Added new interactive command to print all the remaining bytes
		in the buffer -- basically implemented as a large phase shift
		to the left.
		[corresponds to Freevr Version 0.5g]

	16 October 2009 (Bill Sherman) -- Fixed the zero-the-buffer command ('0').
		Also, added the CLA's for dis/enabling the initial printing of
		hex/ASCII/binary output.
		[corresponds to Freevr Version 0.6a]

	12 October 2010 (Bill Sherman) -- Brought from socketspy the new "-mb"
		option.  Also did some minor cleanups -- one of which is to
		update the usage information, including removing of the "-o"
		option which is only useful for socketspy.c.
		[corresponds to Freevr Version 0.6b]

	17 September 2013 (Bill Sherman) -- swapped "fd_pts" to "pts_fd" to
		make consistent with other variables related to "pts".
		[corresponds to Freevr Version 0.6d]

	18 September 2013 (Bill Sherman) -- Added the "-flag" option that I just
		added to socketspy two days ago.
		[corresponds to Freevr Version 0.6d]

TODO:
	- DONE: Write a man-page.

	- DONE: Add the "flag_byte" feature added to socketspy on 09/16/13

	- figure out how to disable processing of keyboard keys, so hitting
		^M sends a 0x0d rather than a 0x0a.

	- add an option to use an actual serial port for the outgoing data
		(ie. a separate computer on one end (outgoing), with the physical
		device on the "incoming" end).  The current method only allows
		operations for which the software is running on the same computer
		as serialspy -- which might not be the case when trying to reverse
		engineer stuff from a Windows box.  (This probably won't be
		difficult at all, but I'll need a separate box to test with).
		[10/15/2009: Is this not already now possible with the man-in-the-middle
		feature added 09/27/06?] [10/12/2010: I think the difference is
		that the request is to allow a separate computer to be the man-in-the-middle,
		without it running any of the communicating software -- ie. to go
		from physical serial port to physical serial port.]

*************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#define __USE_XOPEN	/* a flag for stdlib.h */
#include <stdlib.h>	/* for the pseudo terminal stuff (see "man 4 pts") */

#if 1	/* this seems to be required for Linux (at least post RH 5.2 or 6.0) */
#define FIONREAD	0x541B
#endif

#ifndef TEST_APP	/* TEST_APP should be defined by the compile line, but if not ... */
#  define TEST_APP
#endif
#include "vr_serial.h"
#include "vr_debug.h"
#define	FLAG_TEXT	"[1m" "[32m"


/* We need to do this as a #include because the defines in vr_debug.h */
/*   will affect how the vr_serial.c code is compiled.                */
#include "vr_serial.c"

/* for some reason, this must come after vr_serial.c, or STDIN_FILENO gets undef'd */
#include <unistd.h>	/* for STDIN_FILENO */

/* declaration of function(s) defined later in this file */
char	*binary(unsigned int val, int len, char *str);
int	print_bytes(char *buffer, int bytes_to_print, int bytes_printed, int print_hex, int print_binary, int print_ascii, int flag_byte);


main(int argc, char *argv[])
{
	/* CLA parsing information */
static	char	*err_usage = "Usage: %s [-l <line>] [-b <baud>] [-f <frequency>] [-1] [-pseudo] [-pt] [-mb <max bytes>] [-flag <byte>] [-asc|-noasc] [-bin|-nobin] [-hex|-nohex]\n";
static	char	*err_unk_opt = "%s: unknown option '%s'.\n";
static	char	*err_missing_arg = "%s: missing argument to '%s'.\n";
static	char	*progname;

	/* Serial port related variables (ie. connection to the actual device) */
static	char	buffer[2048];
#if defined(__linux)
#  if 0
	char	*inport = "/dev/ttyUSB0";	/* default serial port on SGI Prism Linux w/ USB-RS232 adaptor */
#  elif 0
	char	*inport = "/dev/ttyIOC4/1";	/* default serial port on SGI Prism Linux */
#  else
	char	*inport = "/dev/ttyS0";		/* default serial port on Linux */
#  endif
#else
	char	*inport = "/dev/ttyd2";		/* default serial port on SGI & other */
#endif
	int	fd_serial = -1;
	int	baud_int = 115200;
	int	baud_enum = vrSerialBaudIntToEnum(baud_int);
	int	read_result;
	int	bytes_in_buffer = 0;
	int	bytes_to_print;
	int	bytes_printed;			/* number of bytes printed since last read */
	int	total_bytes_printed = 0;	/* the total count of bytes output to the screen */

	/* Standard input (stdin) related variables */
struct	termios	stdin_tios;
unsigned char	input;
	int	inbytes_waiting;
	char	intr_char = '\003';
	char	store_vtime;
	char	store_vmin;

	/* Pseudo terminal related variables */
static	char	pts_buffer[2048];
struct	termios	pts_tios;
	int	pts_fd = -1;
	int	pts_bytes_in_buffer = 0;
	int	pts_bytes_to_print;
	int	pts_bytes_sent;			/* number of bytes printed since last read */
	char	*pts_name;

	/* Options (both CLA & interactive, and some both) */
	int	one_time = 0;			/* option flag to quit after one read */
	int	passthru_all_chars = 0;		/* flag that indicates all characters should be sent to device rather than possibly used as an interactive command to serialspy */
	int	use_pseudo = 0;			/* option flag to indicate whether a man-in-the-middle interface should be established */
	int	print_hex = 1;			/* whether or not to display the hexadecimal representation */
	int	print_binary = 0;		/* whether or not to display the binary representation */
	int	print_ascii = 1;		/* whether or not to display the ASCII representation */
	int	frequency = 16;			/* the stride of the data output (how many bytes per lines) */
	int	phase_shift = 0;		/* number of extra (less) bytes to represent on the next output line */
	int	flag_byte = -1;			/* a particular byte value to highlight (< 0 for no flagging) */

	int	max_bytes_to_print = 2048;	/* terminate after handling this many bytes */

	/* Misc flags */
	int	need_separator = 0;
	int	done = 0;


	/******************/
	/* parse the CLAs */
	progname = argv[0];
	while ((argc > 1) && (argv[1][0] == '-')) {
		if (!strcmp(argv[1], "-l")) {		/** input port (line) **/
			argv++; argc--;

			if (argc !=1) {
				inport = strdup(argv[1]);
				fprintf(stderr, "Connecting to device on port '%s'.\n", inport);
			} else {
				fprintf(stderr, err_missing_arg, progname, argv[0]);
				fprintf(stderr, err_usage, progname);
				exit(1);
			}

		} else if (!strcmp(argv[1], "-b")) {	/** baud rate **/
			argv++; argc--;

			if (argc !=1) {
				baud_int = atoi(argv[1]);
				baud_enum = vrSerialBaudIntToEnum(baud_int);
				fprintf(stderr, "Connecting to device at %d baud.\n", baud_int);
			} else {
				fprintf(stderr, err_missing_arg, progname, argv[0]);
				fprintf(stderr, err_usage, progname);
				exit(1);
			}

		} else if (!strcmp(argv[1], "-f")) {	/** print frequency **/
			argv++; argc--;

			if (argc !=1) {
				frequency = atoi(argv[1]);
				fprintf(stderr, "Will begin printing with %d bytes per line.\n", frequency);
			} else {
				fprintf(stderr, err_missing_arg, progname, argv[0]);
				fprintf(stderr, err_usage, progname);
				exit(1);
			}

		} else if (!strcmp(argv[1], "-mb")) {	/** max bytes **/
			argv++; argc--;

			if (argc !=1) {
				max_bytes_to_print = atoi(argv[1]);
				fprintf(stderr, "Will terminate after handling approximately %d bytes.\n", max_bytes_to_print);
			} else {
				fprintf(stderr, err_missing_arg, progname, argv[0]);
				fprintf(stderr, err_usage, progname);
				exit(1);
			}

		} else if (!strcmp(argv[1], "-1")) {	/** one-time option **/

			one_time = 1;
			fprintf(stderr, "Will stop after one read operation.\n");

		} else if (!strcmp(argv[1], "-pseudo")) {/** use pseudo-tty for man-in-the-middle display **/

			use_pseudo = 1;
			fprintf(stderr, "Will create a pseudo tty for direct communication with device.\n");

		} else if (!strcmp(argv[1], "-pt")) {	/** passthrough option (ie. no interactive key commands) **/

			passthru_all_chars = 1;
			fprintf(stderr, "Will pass all keyboard input to the serial port (no interactive commands).\n");

		} else if (!strcmp(argv[1], "-ascii") || !strcmp(argv[1], "-asc")) {		/** startup with ASCII display mode enabled **/

			print_ascii = 1;

		} else if (!strcmp(argv[1], "-noascii") || !strcmp(argv[1], "-noasc")) {	/** startup with ASCII display mode disabled **/

			print_ascii = 0;

		} else if (!strcmp(argv[1], "-binary") || !strcmp(argv[1], "-bin")) {		/** startup with binary display mode enabled **/

			print_binary = 1;

		} else if (!strcmp(argv[1], "-nobinary") || !strcmp(argv[1], "-nobin")) {	/** startup with binary display mode disabled **/

			print_binary = 0;

		} else if (!strcmp(argv[1], "-hexadecimal") || !strcmp(argv[1], "-hex")) {	/** startup with hexadecimal display mode enabled **/

			print_hex = 1;

		} else if (!strcmp(argv[1], "-nohexadecimal") || !strcmp(argv[1], "-nohex")) {	/** startup with hexadecimal display mode disabled **/

			print_hex = 0;

		} else if (!strcmp(argv[1], "-flag")) {						/** highlight a particular byte value **/
			argv++; argc--;

			if (argc !=1) {
				flag_byte = atoi(argv[1]);
				if (flag_byte < 0 || flag_byte > 255) {
					fprintf(stderr, "Flag byte must be in [0..255].\n", flag_byte);
					exit(1);
				}
				fprintf(stderr, "Will highlight byte 0x%x.\n", flag_byte);
			} else {
				fprintf(stderr, err_missing_arg, progname, argv[0]);
				fprintf(stderr, err_usage, progname);
				exit(1);
			}

		} else if (!strcmp(argv[1], "-help") || !strcmp(argv[1], "-usage")) {

			/* show usage and exit */
			fprintf(stderr, err_usage, progname);
			exit(2);

		} else {

			/* state unknown option */
			fprintf(stderr, err_unk_opt, progname, argv[1]);
			fprintf(stderr, err_usage, progname);
		}

		argv++; argc--;
	}

	if (argc < 1) {
		fprintf(stderr, err_usage, progname);
		exit(1);
	}


	/***************************************************************/
	/* open the "incoming" port (ie. port connected to the device) */
	fd_serial = vrSerialOpen(inport, baud_enum);
	if (fd_serial < 0) {
		fprintf(stderr, RED_TEXT "Unable to open serial port '%s'\n" NORM_TEXT, inport);
		exit(1);
	}

	/*********************************************************************************/
	/* setup standard in to be non-canonical, and thus we can get chars w/o blocking */

	/* get the current parameters */
	tcgetattr(STDIN_FILENO, &stdin_tios);

	/* turn off canonical processing and the echoing of characters */
	stdin_tios.c_lflag &= ~(ICANON | ECHO);

	/* setup for non-blocking mode (storing old values for later restoration) */
	store_vmin = stdin_tios.c_cc[VMIN];
	store_vtime = stdin_tios.c_cc[VTIME];
	stdin_tios.c_cc[VMIN] = 0;
	stdin_tios.c_cc[VTIME] = 0;

	/* make the new parameters take affect */
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &stdin_tios);

	intr_char = stdin_tios.c_cc[VINTR];


	/*****************************************************/
	/* Open the Pseudo terminal (master-side) connection */
	if (use_pseudo) {
		pts_fd = getpt();
		if (pts_fd < 0) {
			fprintf(stderr, "%s: Unable to open pseudo terminal.\n", progname);
			exit(1);
		}
		if (grantpt(pts_fd) < 0) {
			fprintf(stderr, "%s: Unable to change pseudo terminal permissions.\n", progname);
		}
		if (unlockpt(pts_fd) < 0) {
			fprintf(stderr, "%s: Unable to unlock pseudo terminal.\n", progname);
		}

#if 0 /* This was to set us up in non-blocking on the pseudo master, but it only causes errors in the read */
		/* set the file control flags for the pseudo master (to the flags used by open() in vrSerialOpen() */
		if (fcntl(pts_fd, F_SETFL, O_RDWR | O_NONBLOCK | O_NOCTTY) < 0) {
			fprintf(stderr, "%s: An error occurred while trying to reset the file controls.\n", progname);
		}
#endif

		/* set the baud rate for the pts to match our real serial connection */
		tcgetattr(pts_fd, &pts_tios);
		printf("initial baud of pts is %d/%d\n", cfgetispeed(&pts_tios), cfgetospeed(&pts_tios));
		cfsetispeed(&pts_tios, baud_enum);
		cfsetospeed(&pts_tios, baud_enum);
		printf("new baud of pts is %d/%d\n", cfgetispeed(&pts_tios), cfgetospeed(&pts_tios));

		/* turn off canonical processing and the echoing of characters */
		pts_tios.c_lflag &= ~(ICANON | ECHO);

		/* setup for non-blocking mode */
		pts_tios.c_cc[VMIN] = 0;
		pts_tios.c_cc[VTIME] = 0;

		/* make the new parameters take affect */
		tcsetattr(pts_fd, TCSAFLUSH, &pts_tios);

		pts_name = ptsname(pts_fd);
		if (pts_name != NULL) {
			printf("Connect to the serial device via tty: '%s'\n", ptsname(pts_fd));
		} else {
			printf("Unable to get name of pts device for file descriptor %d.\n", pts_fd);
		}
	}

	/**************************/
	/* loop until interrupted */
	while (!done) {
		/******************************************************/
		/* read the terminal for any inputs, and process them */
		while (ioctl(STDIN_FILENO, FIONREAD, &inbytes_waiting) == 0 && inbytes_waiting > 0 && read(STDIN_FILENO, &input, 1) > 0) {
#if 0
			printf("input char = %02x\n", input);
#endif
			/* this doesn't seem to work, but no harm */
			if (input == intr_char)
				done = 1;

			if (!passthru_all_chars) {
				switch (input) {
				case '\3':
				case 'q':
				case 'Q':
					done = 1;
					break;
				case '?':
				case '/':
					printf(NORM_TEXT "\n"
						"---------------------------------\n"
						"'?' '/' -- display help string\n"
						"'q' 'Q' -- quit serialspy\n"
						"'+' '=' -- increment frequency\n"
						"'-' '_' -- decrement frequency\n"
						"'>' '.' -- phase shift down\n"
						"'<' ',' -- phase shift up\n"
						"'0' ')' -- print remaining bytes in buffer\n"
						"'a' 'A' -- toggle ASCII display\n"
						"'b' 'B' -- toggle binary display\n"
						"'h' 'H' -- toggle hex display\n"
						"(backspace) -- switch to pass through all keys mode (cannot return)\n"
						"---------------------------------\n"
						"");
					break;
				case '+':
				case '=':
					frequency++;
					break;
				case '-':
				case '_':
					frequency--;
					if (frequency <= 0)
						frequency = 1;
					break;
				case '>':
				case '.':
					phase_shift--;
					break;
				case '<':
				/*case ',': */
					phase_shift++;
					break;
				case '0':
				case ')':
#if 0
					phase_shift = -bytes_in_buffer;		/* print any bytes sitting in buffer */
#else
					phase_shift = bytes_in_buffer - frequency;		/* print any bytes sitting in buffer [NOTE: this version from socketspy.c -- I suspect the old version was a bug, but can't test right now [10/16/09] */
#endif
					break;

				case 'a':
				case 'A':
					print_ascii ^= 1;
					break;
				case 'b':
				case 'B':
					print_binary ^= 1;
					break;
				case 'h':
				case 'H':
					print_hex ^= 1;
					break;

				case 0x08:
					passthru_all_chars = 1;
					printf("Will now pass through all keys to serial port.\n");
					break;

				case 0x0d:
					printf("HEY, got a 0x0d!\n");
				case 0x0a:	/* for a reason I haven't yet investigated, both ^M & ^J generate a '0a' character rather than a '0d' */
					input = 0x0d;
				default:	/* (feature in testing) pass along other characters to the serial port */
					vrSerialWrite(fd_serial, (char *)(&input), sizeof(char));
					fsync(fd_serial);			/* didn't this used to be the flush() command? */
					printf(BOLD_TEXT "Keyboard input: " NORM_TEXT "'%c' (0x%02x) being sent to serial port\n", input, input);
					break;
				}
			} else {
				/* This is the pass through all characters to the device command */
				/* However, the 0x0a character is sent as a 0x0d.                */
				/* TODO: there is probably a way to disable whatever processing  */
				/*   is causing the ^M to be converted to a 0x0a in the first    */
				/*   place.  This would be a better solution.                    */
				switch (input) {
				case 0x0d:
					printf("HEY, got a 0x0d!\n");
				case 0x0a:	/* for a reason I haven't yet investigated, both ^M & ^J generate a '0a' character rather than a '0d' */
					input = 0x0d;
				default:
					vrSerialWrite(fd_serial, (char *)(&input), sizeof(char));
					fsync(fd_serial);			/* didn't this used to be the flush() command? */
					printf(BOLD_TEXT "Keyboard input: " NORM_TEXT "(0x%02x) being sent to serial port\n", input);
					break;
				}
			}
		}

		/******************************************/
		/* now read from the incoming serial port */
		fsync(fd_serial);	/* I'm not sure whether this is necessary -- I was trying to get that last byte, which I wasn't seeing due to a bug a few lines down. */
		read_result = (ssize_t)vrSerialRead(fd_serial, &buffer[bytes_in_buffer], sizeof(buffer)-bytes_in_buffer);
		if (read_result < 0) {
			printf("Got a bad read from the serial port.\n");
		} else {
			/* Now echo the input to the pseudo tty device */
			if (pts_fd > -1) {
				vrSerialWrite(pts_fd, &buffer[bytes_in_buffer], read_result);
			}

			/* and now increment the bytes_in_buffer value */
			bytes_in_buffer += read_result;
		}
		bytes_printed = 0;
		bytes_to_print = frequency + phase_shift;

		while ((bytes_in_buffer >= bytes_printed + bytes_to_print) && (bytes_to_print > 0)) {
			phase_shift = 0;
			printf(BOLD_TEXT "From Device: " NORM_TEXT);
			bytes_printed += print_bytes(buffer, bytes_to_print, bytes_printed, print_hex, print_binary, print_ascii, flag_byte);
#if 0
			printf(BOLD_TEXT "%d: ", bytes_printed);
#endif
		}

		if (bytes_printed > 0) {
			/* shift the non-printed stuff to beginning of buffer */
			bytes_in_buffer -= bytes_printed;
			memmove(buffer, buffer+bytes_printed, bytes_in_buffer);
		}


		/*****************************************/
		/* now read from the incoming pseudo-tty */
		fsync(pts_fd);	/* I'm not sure whether this is necessary -- I was trying to get that last byte, which I wasn't seeing due to a bug a few lines down. */
		if (ioctl(pts_fd, FIONREAD, &inbytes_waiting) == 0 && inbytes_waiting > 0) {
			read_result = (ssize_t)vrSerialRead(pts_fd, &pts_buffer[pts_bytes_in_buffer], sizeof(pts_buffer)-pts_bytes_in_buffer);
			if (read_result < 0) {
				printf("Got a bad read from the pseudo tty.\n");
			} else {
				pts_bytes_in_buffer += read_result;
			}
		}
		pts_bytes_sent = 0;
		pts_bytes_to_print = frequency + phase_shift;	/* NOTE: I'm not sure I'll handle outgoing data the same way */

		if (pts_bytes_in_buffer > 0) {
#if 0
			printf("PTY input '%s'(0x%02x...) (len=%d) being sent to serial port\n", pts_buffer, pts_buffer[0], pts_bytes_in_buffer);
#endif
			printf(BOLD_TEXT "From PTY: " NORM_TEXT);
			bytes_printed += print_bytes(pts_buffer, pts_bytes_in_buffer, 0, print_hex, print_binary, print_ascii, flag_byte);
			pts_bytes_sent = vrSerialWrite(fd_serial, pts_buffer, pts_bytes_in_buffer);
			fsync(pts_fd);				/* didn't this used to be the flush() command? */

			pts_bytes_in_buffer -= pts_bytes_sent;
			memmove(pts_buffer, pts_buffer+pts_bytes_sent, pts_bytes_in_buffer);
		}

		/************************************************************/
		/* set for termination if we've exceeded the max byte count */
		total_bytes_printed += bytes_printed;
		if ((max_bytes_to_print >= 0) && (total_bytes_printed > max_bytes_to_print))
			done = 1;

		/***************************************************/
		/* set for termination if the one-time flag is set */
		if (one_time)
			done = 1;
	} /* while loop */

	/********************************/
	/* restore stdin as best we can */
	stdin_tios.c_lflag |= ICANON | ECHO;
	stdin_tios.c_cc[VMIN] = store_vmin;
	stdin_tios.c_cc[VTIME] = store_vtime;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &stdin_tios);

	/*********************/
	/* close the sockets */
	close(fd_serial);
	if (pts_fd != -1)
		close(pts_fd);
}


/*******************************************************************/
/* binary(): takes an integer, a number of bits, and a pointer to  */
/*   memory in which to store the result.  The resultant string    */
/*   is placed in the memory, and returned.  The string is made up */
/*   of the least significant N bits, based on the value of len.   */
/*******************************************************************/
char *binary(unsigned int val, int len, char *str)
{
	int     count;

	str[len] = '\0';
	for (count = 0; count < len; count++) {
		str[len-count-1] = (((1 << count) & val) ? '1' : '0');
	}
	return(str);
}


/********************************************************************/
int print_bytes(char *buffer, int bytes_to_print, int bytes_printed, int print_hex, int print_binary, int print_ascii, int flag_byte)
{
static	char	bin_buf[32];
	int	need_separator = 0;
	int	count = bytes_to_print;
	char	byte;

	printf(BOLD_TEXT "%d: " NORM_TEXT, bytes_to_print);

	/************************/
	/* print the HEX values */
	if (print_hex) {
		for (count = 0; count < bytes_to_print; count++) {
			byte = buffer[count+bytes_printed] & 0x7f;
			if (byte != buffer[count+bytes_printed] & 0x7f)
				printf(RED_TEXT);
			else	printf(NORM_TEXT);
			if (flag_byte >= 0 && byte == flag_byte)
				printf(FLAG_TEXT);
			printf("%02x ", (unsigned int)(buffer[count+bytes_printed] & 0xff));
		}

		need_separator = 1;
	}

	/***************************/
	/* print the BINARY values */
	if (print_binary) {
		if (need_separator)
			printf(BOLD_TEXT " |  " NORM_TEXT);
		for (count = 0; count < bytes_to_print; count++) {
			byte = buffer[count+bytes_printed] & 0x7f;
			if (byte != buffer[count+bytes_printed] & 0x7f)
				printf(RED_TEXT);
			else	printf(NORM_TEXT);
			if (flag_byte >= 0 && byte == flag_byte)
				printf(FLAG_TEXT);
			printf("%s ", binary((unsigned int)(buffer[count+bytes_printed] & 0xff), 8, bin_buf));
		}
		need_separator = 1;
	}

	/**************************/
	/* print the ASCII values */
	if (print_ascii) {
		if (need_separator)
			printf(BOLD_TEXT " |  " NORM_TEXT);
		for (count = 0; count < bytes_to_print; count++) {
			byte = buffer[count+bytes_printed] & 0x7f;
			if (byte != buffer[count+bytes_printed] & 0x7f)
				printf(RED_TEXT);
			else	printf(NORM_TEXT);
			if (flag_byte >= 0 && byte == flag_byte)
				printf(FLAG_TEXT);
			printf("%c ", (isprint(byte) ? byte : '.'));
		}
		need_separator = 1;
	}

	printf(NORM_TEXT "\n");

	return count;
}

