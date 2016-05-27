/* ======================================================================
 *
 *  CCCCC          socketspy.c
 * CC   CC         Author(s): Bill Sherman
 * CC              Created: November 14, 2008 -- adapted from my serialspy.c code
 * CC   CC         Last Modified: September 16, 2013
 *  CCCCC
 *
 * Code file for spying on the bytes going through (ie. coming from)
 *   a socket port.  Intended to help make sure socket communications
 *   are functioning properly with various daemons and some devices,
 *   and also to help reverse engineer data protocols used by said
 *   devices and daemons.
 *
 * NOTE: while this program makes some use of the vr_socket.c functions,
 *   it does not use vrSocketSendMsg() and vrSocketReadMsg(), as they
 *   are designed for communication of ASCII strings, which will often
 *   not be the case with the data analyzed with this program.
 *   (Actually, the Read version isn't ASCII specific, but we'll use
 *   the system read() here for consistency within this program.)
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/*************************************************************************

USAGE:
	socketspy [-h <input host>] [-p <input port>] [-o <output port>]
		[-f <frequency>] [-mb <max bytes>] [-1] [-pt] [-flag <byte>]
		[-asc|-noasc] [-bin|-nobin] [-hex|-nohex]

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

	(for the man page, SEE ALSO: "netstat -nlup", "netstat -nltp" and "ss")

HISTORY:
	14 November 2008 -- First version written by Bill Sherman on a monster
		Dell laptop (XPS-M1730).  At the time was called "vruiddtest.c",
		and was adapted from the FreeVR "serialspy.c" code.
		[corresponds to Freevr Version 0.5g]

	20 April 2009 -- A new "first" version written by Bill Sherman by again
		adapting the socketspy.c program -- I basically started again
		from scratch, but did view the earlier prototype program I
		had started called "vruiddtest.c" to experiment with reading
		data from Oliver Kreylos' VRUI Device Daemon.  Coding was
		done in my hotel room while attending the DoE-CGF in Monterey.
		[corresponds to Freevr Version 0.5g]

	15-16 October 2009 (Bill Sherman) -- This is really the first "clean"
		version of the socketspy program, and has been used in the
		development and testing of the VruiDD input device as well as
		for verifying information about the VRPN data from the OptiTrack
		system.

		The code was cleaned up significantly.  Also, new command line
		arguments were added to control the initial output parameters.
		Finally, the program is set to terminate when the incoming socket
		closes.
		[corresponds to Freevr Version 0.6a]

	18 January 2010 (Bill Sherman) -- Cleaned out the old serial code and
		updated the CLAs for specifying the outgoing (listening) and
		incoming sockets.
		[corresponds to Freevr Version 0.6b-dev]

	12 October 2010 (Bill Sherman) -- Fixed the operation of the "-o" (output port)
		option.  Added a new "-mb" (max-bytes) option -- using this option will
		now cause the program to terminate after the buffer is emptied and the
		given number of bytes have been handled (which can mean that a little
		more than the specified number of bytes will be printed, but I think
		that's acceptable).  We now close the host and server sockets.
		[corresponds to Freevr Version 0.6b]

	16 September 2013 (Bill Sherman) -- Add a new "Flag-byte" feature that will
		highlight the value of a particular byte no matter where it appears.
		This is handy when you know (or suspect) a particular marker byte
		that demarcates the boundaries of some packet.

		I also changed "pts" to "server" throughout -- as requested in the
		TODO list.
		[corresponds to Freevr Version 0.6d]

TODO:
	- DONE: Write a man-page.  (Make a note that this requires being configured to
		be in the middle of socket communications, therefore the user
		must be in charge of at least one end of the communication so
		this tool is therefore not suited for spying on communications
		by third parties.

		- also note that the port defaults are presently set for Vrui

		- also note that the value for "-flag" is decimal only (even though it
			reports it back in hex)

	- DONE: Change "pts" to "server" in all cases -- PTS was for the serial version

	- do a clean exit when an interrupt signal (or others) is received

	- figure out how to disable processing of keyboard keys, so hitting
		^M sends a 0x0d rather than a 0x0a.

	- change the default behavior of losing a connection with the incoming
		stream to then wait for a new connection.  The terminate-upon-
		connection-loss method should still be provided as an option.

*************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#define __USE_XOPEN	/* a flag for stdlib.h */
#include <stdlib.h>	/* for exit() system call */

#if 1	/* this seems to be required for Linux (at least post RH 5.2 or 6.0) */
#define FIONREAD	0x541B
#endif

#ifndef TEST_APP	/* TEST_APP should be defined by the compile line, but if not ... */
#  define TEST_APP
#endif
#include "vr_socket.h"
#include "vr_debug.h"
#define	FLAG_TEXT	"[1m" "[32m"

/* We need to do this as a #include because the defines in vr_debug.h  */
/*   will affect how the vr_socket.c code is compiled.                 */
#include "vr_socket.c"


/* declaration of function(s) defined later in this file */
char	*binary(unsigned int val, int len, char *str);
int	print_bytes(char *buffer, int bytes_to_print, int bytes_printed, int print_hex, int print_binary, int print_ascii, int flag_byte);


main(int argc, char *argv[])
{
	/* CLA parsing information */
static	char	*err_usage = "Usage: %s [-h <input host>] [-p <input port>] [-o <output port>] [-f <frequency>] [-1] [-pt] [-mb <max bytes>] [-flag <byte>] [-asc|-noasc] [-bin|-nobin] [-hex|-nohex]\n";
static	char	*err_unk_opt = "%s: unknown option '%s'.\n";
static	char	*err_missing_arg = "%s: missing argument to '%s'.\n";
static	char	*progname;

	/* Host-socket port related variables (ie. connection to the actual device) */
static	char	buffer[2048];
	char	*in_machine = "localhost";	/* default to socket on local machine */
	int	in_port = 8555;			/* (for now) default to VRUI socket port */
	int	fd_socket = -1;			/* the file-descriptor for the client port */
	int	read_result;
	int	bytes_in_buffer = 0;
	int	bytes_to_print;
	int	bytes_printed;			/* number of bytes printed since last read */
	int	total_bytes_printed = 0;	/* the total count of bytes output to the screen */

	int	listen_port = 8556;		/* aka "output port" the socket we listen for sending out data */
	int	fd_listen_sock = -1;

	/* Standard input (stdin) related variables */
struct	termios	stdin_tios;
unsigned char	input;
	int	inbytes_waiting;
	char	intr_char = '\3';
	char	store_vtime;
	char	store_vmin;

	/* Server-socket related variables */
static	char	server_buffer[2048];
struct	termios	server_tios;
	int	server_fd = -1;			/* the file-descriptor for the server/listener port */
	int	server_bytes_in_buffer = 0;
	int	server_bytes_to_print;
	int	server_bytes_sent;			/* number of bytes printed since last read */
	char	*server_name;

	/* Options (both CLA & interactive, and some both) */
	int	one_time = 0;			/* option flag to quit after one read */
	int	passthru_all_chars = 0;		/* flag that indicates all characters should be sent to device rather than possibly used as an interactive command to socketspy */
	int	print_hex = 1;			/* whether or not to display the hexadecimal representation */
	int	print_binary = 0;		/* whether or not to display the binary representation */
	int	print_ascii = 0;		/* whether or not to display the ASCII representation */
	int	frequency = 16;			/* the stride of the data output (how many bytes per lines) */
	int	phase_shift = 0;		/* number of extra (less) bytes to represent on the next output line */
	int	flag_byte = -1;			/* a particular byte value to highlight (< 0 for no flagging) */

	int	max_bytes_to_print = -1;	/* terminate after handling this many bytes (or don't terminate if < 0) */

	/* Misc flags */
	int	need_separator = 0;
	int	done = 0;


	/******************/
	/* parse the CLAs */
	progname = argv[0];
	while ((argc > 1) && (argv[1][0] == '-')) {
		if (!strcmp(argv[1], "-h")) {		/** input machine (host) **/
			argv++; argc--;

			if (argc !=1) {
				in_machine = strdup(argv[1]);
				fprintf(stderr, "Connecting to device on host '%s'.\n", in_machine);
			} else {
				fprintf(stderr, err_missing_arg, progname, argv[0]);
				fprintf(stderr, err_usage, progname);
				exit(1);
			}
		} else if (!strcmp(argv[1], "-p")) {	/** input port **/
			argv++; argc--;

			if (argc !=1) {
				in_port = atoi(argv[1]);
				fprintf(stderr, "Connecting to host on port '%d'.\n", in_port);
			} else {
				fprintf(stderr, err_missing_arg, progname, argv[0]);
				fprintf(stderr, err_usage, progname);
				exit(1);
			}
		} else if (!strcmp(argv[1], "-o")) {	/** output port (listening port) **/
			argv++; argc--;

			if (argc !=1) {
				listen_port = atoi(argv[1]);
				fprintf(stderr, "Listening to connect for output on port '%d'.\n", listen_port);
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

		} else if (!strcmp(argv[1], "-pt")) {	/** passthrough option (ie. no interactive key commands) **/

			passthru_all_chars = 1;
			fprintf(stderr, "Will pass all keyboard input to the host port (no interactive commands).\n");

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
	fd_socket = vrSocketCall(in_machine, in_port);
	if (fd_socket < 0) {
		fprintf(stderr, RED_TEXT "Unable to open connection to socket '%s:%d'\n" NORM_TEXT, in_machine, in_port);
		exit(1);
	}

	if (fcntl(fd_socket, F_SETFL, O_RDWR | O_NONBLOCK | O_NOCTTY) < 0) {
		fprintf(stderr, "%s: An error occurred while trying to set the file controls.\n", progname);
	}


	/*****************************************************/
	/* Open the outgoing socket (master-side) connection */
	fd_listen_sock = vrSocketCreateListen(&listen_port, 0);
	if (fd_listen_sock < 0) {
		fprintf(stderr, RED_TEXT "Unable to open socket listen port '%d'\n" NORM_TEXT, listen_port);
		exit(1);
	}
	printf("Now going to wait for connection on %d\n", listen_port);
	while ((server_fd = vrSocketAnswer(fd_listen_sock)) < 0) {
		sleep(1);
		printf(".\n");
	}
	printf("Connection established -- server_fd = %d\n", server_fd);

	/* set up in non-blocking on the master */
	if (fcntl(server_fd, F_SETFL, O_RDWR | O_NONBLOCK | O_NOCTTY) < 0) {
		fprintf(stderr, "%s: An error occurred while trying to reset the file controls.\n", progname);
	}

	/* adjust the file descriptor parameters */
	tcgetattr(server_fd, &server_tios);

	/* turn off canonical processing and the echoing of characters */
	server_tios.c_lflag &= ~(ICANON | ECHO);

	/* setup for non-blocking mode */
	server_tios.c_cc[VMIN] = 0;
	server_tios.c_cc[VTIME] = 0;

	/* make the new parameters take affect */
	tcsetattr(server_fd, TCSAFLUSH, &server_tios);


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


	/**************************/
	/* loop until interrupted */
	while (!done) {
		/******************************************************/
		/* read the terminal for any inputs, and process them */
		while (ioctl(STDIN_FILENO, FIONREAD, &inbytes_waiting) == 0 && inbytes_waiting > 0 && read(STDIN_FILENO, &input, 1) > 0) {
#if 1
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
						"'q' 'Q' -- quit socketspy\n"
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
					phase_shift = bytes_in_buffer - frequency;		/* print any bytes sitting in buffer */	/* [10/16/09] NOTE: was "-bytes_in_buffer" but that went the wrong way -- is serialspy.c wrong too? */
printf("will shift by %d bytes\n", bytes_in_buffer);
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

				case 0x08: /* backspace */
					passthru_all_chars = 1;
					printf("Will now pass through all keys to host port.\n");
					break;

				case 0x0d:
					printf("HEY, got a 0x0d!\n");
				case 0x0a:	/* for a reason I haven't yet investigated, both ^M & ^J generate a '0a' character rather than a '0d' */
					input = 0x0d;
				default:	/* (feature in testing) pass along other characters to the host port */
					write(fd_socket, (char *)(&input), sizeof(char));
					fsync(fd_socket);			/* didn't this used to be the flush() command? */
					printf(BOLD_TEXT "Keyboard input: " NORM_TEXT "'%c' (0x%02x) being sent to socket.\n", input, input);
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
					write(fd_socket, (char *)(&input), sizeof(char));
					fsync(fd_socket);			/* didn't this used to be the flush() command? */
					printf(BOLD_TEXT "Keyboard input: " NORM_TEXT "(0x%02x) being sent to socket.\n", input);
					break;
				}
			}
		}

		/*******************************/
		/* now read from the host port */
#if 0
		fsync(fd_socket);	/* I'm not sure whether this is necessary -- I was trying to get that last byte, which I wasn't seeing due to a bug a few lines down. */
#endif
		read_result = (ssize_t)read(fd_socket, &buffer[bytes_in_buffer], sizeof(buffer)-bytes_in_buffer);
		if (read_result < 0) {
#if 0
			printf("Got a bad read from the socket.\n");
#endif
		} else if (read_result == 0) {
#if 0 /* first quick attempt to wait for another socket when the first one closes */
			printf("Got a read of 0 length -- socket closed?  Will now wait for another connection.\n");
			printf("yo3\n");
			fd_listen_sock = vrSocketCreateListen(&listen_port, 0);
			if (fd_listen_sock < 0) {
				fprintf(stderr, RED_TEXT "Unable to open socket listen port '%s'\n" NORM_TEXT, listen_port);
				exit(1);
			}
			printf("Now going to wait for connection on %d\n", listen_port);
			while ((server_fd = vrSocketAnswer(fd_listen_sock)) < 0) {
				sleep(1);
				printf(".\n");
			}
			printf("Connection re-established -- server_fd = %d\n", server_fd);
#else
			/* When the incoming socket closes, print the remaining bytes in the buffer and terminate. */
			printf("Got a read of 0 length -- incoming socket closed, will now terminate. (%d bytes_in_buffer)\n", bytes_in_buffer);
			phase_shift = bytes_in_buffer - frequency;	/* print the remaining bytes */
			done = 1;
#endif
		} else {
			/* Now echo the input to the server-side socket */
			if (server_fd > -1) {
				write(server_fd, &buffer[bytes_in_buffer], read_result);
			}

			/* and now increment the bytes_in_buffer value */
			bytes_in_buffer += read_result;
		}
		bytes_printed = 0;
		bytes_to_print = frequency + phase_shift;

#if 0
printf("bytes_in_buffer (%d) >= bytes_printed (%d) + bytes_to_print (%d)\n", bytes_in_buffer, bytes_printed, bytes_to_print);
#endif
		while ((bytes_in_buffer >= bytes_printed + bytes_to_print) && (bytes_to_print > 0)) {
			phase_shift = 0;
			/* if not requested to print anything, then actually print nothing at all -- sort of a pause of the printing */
			if (print_hex + print_binary + print_ascii != 0) {
				printf(BOLD_TEXT "From Server: " NORM_TEXT);
				bytes_printed += print_bytes(buffer, bytes_to_print, bytes_printed, print_hex, print_binary, print_ascii, flag_byte);
#if 0
				printf(BOLD_TEXT "%d: ", bytes_printed);
#endif
			}
		}

		if (bytes_printed > 0) {
			/* shift the non-printed stuff to beginning of buffer */
			bytes_in_buffer -= bytes_printed;
			memmove(buffer, buffer+bytes_printed, bytes_in_buffer);
		}


		/*******************************************************/
		/* now read from the incoming socket (from the client) */
		fsync(server_fd);	/* I'm not sure whether this is necessary -- I was trying to get that last byte, which I wasn't seeing due to a bug a few lines down. */
		if (ioctl(server_fd, FIONREAD, &inbytes_waiting) == 0 && inbytes_waiting > 0) {
			read_result = (ssize_t)read(server_fd, &server_buffer[server_bytes_in_buffer], sizeof(server_buffer)-server_bytes_in_buffer);
			if (read_result < 0) {
				printf("Got a bad read from the client socket.\n");
			} else if (read_result == 0) {
				printf("incoming read returned 0\n");
			} else {
				server_bytes_in_buffer += read_result;
			}
		}
		server_bytes_sent = 0;
		server_bytes_to_print = frequency + phase_shift;	/* NOTE: I'm not sure I'll handle outgoing data the same way */

		if (server_bytes_in_buffer > 0) {
			printf(BOLD_TEXT "From Client: " NORM_TEXT);
			bytes_printed += print_bytes(server_buffer, server_bytes_in_buffer, 0, print_hex, print_binary, print_ascii, flag_byte);
			server_bytes_sent = write(fd_socket, server_buffer, server_bytes_in_buffer);
			fsync(server_fd);			/* didn't this used to be the flush() command? */

			server_bytes_in_buffer -= server_bytes_sent;
			memmove(server_buffer, server_buffer+server_bytes_sent, server_bytes_in_buffer);
		}

		/************************************************************/
		/* set for termination if we've exceeded the max byte count */
		total_bytes_printed += bytes_printed;
		if ((max_bytes_to_print >= 0) && (total_bytes_printed > max_bytes_to_print)) {
			printf("\nReached maximum byte count (%d), terminating.\n", max_bytes_to_print);
			done = 1;
		}

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
	close(fd_socket);
	close(server_fd);
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

