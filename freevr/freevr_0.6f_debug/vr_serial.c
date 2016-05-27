/* ======================================================================
 *
 *  CCCCC          vr_serial.c
 * CC   CC         Author(s): Bill Sherman
 * CC              Created: November 23, 1999
 * CC   CC         Last Modified: February 14, 2000
 *  CCCCC
 *
 * Code file for FreeVR serial interfacing with I/O devices.
 *
 * Copyright 2013, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#include <stdio.h>
#include <string.h>	/* for memchr() */
#include <stdlib.h>	/* for getenv() */
#include <unistd.h>	/* for open(), read(), write() */
#include <termios.h>
#include <fcntl.h>
#include <sys/time.h>
#include <errno.h>	/* for errno in vrSerialRead() */

#include "vr_serial.h"
#include "vr_debug.h"


/*******************************************************************/
int vrSerialOpen(char *serialPort, int baud_enum)
{
#ifdef UNPICKY_COMPILER
	int		result;
#endif
	int		port_id;
	struct termios  tios;
#if 0
	char		*yo = "this is a test";		/* TODO: figure out why cgtest needs this */
#endif

	if (baud_enum == 0) {
		vrFprintf(stderr, "vrSerialOpen(): invalid baud rate given for port %s.\n", serialPort);
		return -1;
	}

	port_id = open(serialPort, O_RDWR | O_NONBLOCK | O_NOCTTY);
	if (port_id < 0) {
		vrFprintf(stderr, "vrSerialOpen(): couldn't open serial port %s at %d baud.\n", serialPort, vrSerialBaudEnumToInt(baud_enum));
		perror("vrSerialOpen():");
		return (port_id);
	}

#ifdef UNPICKY_COMPILER
	result = tcgetattr(port_id, &tios);
#else
	tcgetattr(port_id, &tios);
#endif

	/* NOTE: original Magellan code set "c_iflag |= IGNBRK | IGNPAR | IXON;" */
	/*   and unset "c_iflag &= ~(BRKINT | PARMRK | INPCK | ISTRIP | INLCR |  */
	/*                           IGNCR | ICRNL | IUCLC | IXANY | IXOFF | IMAXBEL); */
	/* NOTE: this works, but spaceorb/ball was just "tios.c_iflag = IGNBRK | IGNPAR;" */
	/* NOTE: FAROarm just sets "c_iflag = 0;" */
	/* NOTE: PinchGlove requires IXON to be UNset */
#if 0 /* 12/30/99 testing */
	tios.c_iflag |= IGNBRK | IGNPAR | IXON;
	tios.c_iflag &= ~(BRKINT | PARMRK | INPCK | ISTRIP | INLCR | IGNCR | ICRNL | IUCLC | IXANY | IXOFF | IMAXBEL);
#elif 0
	tios.c_iflag |= IGNBRK | IGNPAR;
	tios.c_iflag &= ~(BRKINT | PARMRK | INPCK | ISTRIP | INLCR | IGNCR | ICRNL | IUCLC | IXANY | IXOFF | IXON | IMAXBEL);
#else
	tios.c_iflag = IGNBRK | IGNPAR;
#endif /* 12/30/99 testing */

	/* NOTE: spaceorb, magellan and FAROarm all set "c_oflag = 0;" */
	tios.c_oflag = 0;

#if 1 /* 12/30/99 testing */
	/* NOTE: original Magellan code set "c_cflag = CREAD | CS8 | CLOCAL | HUPCL | CSTOPB;" */
	/* NOTE: this works, but spaceorb/ball was "c_cflag = CREAD | CS8 | CLOCAL | HUPCL;" */
	/* NOTE: FAROarm sets "c_cflag = baudRate | CREAD | CS8 | CLOCAL;" */
	/* NOTE: [1/3/00] To work properly, the Magellan needs CSTOPB to be set, and the other  */
	/*          serial input devices (PinchGlove, SpaceOrb & SpaceBall) work fine with this */
	/* NOTE: Clemson Bird library sets "c_cflag = CREAD | CS8 | CLOCAL;" */
	tios.c_cflag = CREAD | CS8 | CLOCAL | HUPCL | CSTOPB;
#else
	tios.c_cflag = CREAD | CS8 | CLOCAL | HUPCL;
#endif /* 12/30/99 testing */

#if 0 /* 12/30/99 testing */
	tios.c_cc[VEOL] = '\000';
	tios.c_cc[VSTART] = '\000';	/* probably not necessary */
	tios.c_cc[VSTOP] = '\000';	/* probably not necessary */
#else
	tios.c_cc[VEOL] = '\r';
#endif /* 12/30/99 testing */

	/* NOTE: spaceorb/ball, magellan and FAROarm all set "c_lflag = 0;" */
	tios.c_lflag = 0;		/* -ECHO -ISIG -ICANON */

	/* NOTE: this works, but spaceorb/ball (and old Magellan code) also set: "c_cc[VEOL] = '\r';" */
	/* NOTE: FAROarm specifically sets "c_cc[VEOL] = '\000';" */
	tios.c_cc[VERASE] = '\000';
	tios.c_cc[VKILL] = '\000';

#if defined(USE_BLOCKING_MODE) || 0 /* not yet fully implemented */
	tios.c_cc[VMIN] = 1;
	tios.c_cc[VTIME] = 0;

#elif 0 /* for FARO code testing */
	/* NOTE: FAROarm blocks with a 2 sec timeout: "c_cc[VMIN] = 1; c_cc[VTIME] = 20;" */
	tios.c_cc[VMIN] = 1;
	tios.c_cc[VTIME] = 20;
	tios.c_cc[VEOL] = '\000';
#else
	tios.c_cc[VMIN] = 0;
	tios.c_cc[VTIME] = 0;		/* Don't wait (ie. non-blocking) */
#endif

	cfsetospeed(&tios, baud_enum);
	cfsetispeed(&tios, baud_enum);

#ifdef UNPICKY_COMPILER
	result = tcsetattr(port_id, TCSAFLUSH, &tios);
#else
	tcsetattr(port_id, TCSAFLUSH, &tios);
#endif

#if 0 /* This should just be done by the specific device -- this was from Magellan code */
	/* FAROarm also sleeps 1 second after open */
	sleep(1);
#endif

	if (vrDbgDo(SERIAL_DBGLVL) || getenv("VRSERIAL_DEBUG")) {
		vrFprintf(stderr, "vrSerialOpen(): Port now open\n");
	}

	return (port_id);
}


/************************************************************/
int vrSerialFlushI(int fd)
{
	return tcflush(fd, TCIFLUSH);
}


/************************************************************/
int vrSerialFlushO(int fd)
{
	return tcflush(fd, TCOFLUSH);
}


/************************************************************/
int vrSerialFlushIO(int fd)
{
	return tcflush(fd, TCIOFLUSH);
}


/************************************************************/
int vrSerialDrain(int fd)
{
	return tcdrain(fd);
}


/************************************************************/
int vrSerialClose(int fd)
{
	return close(fd);
}


/*******************************************************************/
/* was static int MAG_awaitdata(vrMagellanPrivateInfo *mag) */
/* NOTE: this probably also works on sockets!  TODO: consider renaming this, and putting it in "vr_utils.c" or something */
int vrSerialAwaitData(int fd)
{
	struct timeval	awhile;
	fd_set		fds;

	awhile.tv_sec = 0;
	awhile.tv_usec = 150000;	/* Wait up to .15 second */

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	return select(fd+1, &fds, NULL, NULL, &awhile);
}


/*******************************************************************/
/* was static void MAG_MediateWrite(vrMagellanPrivateInfo *mag, char *buf, int len) */
int vrSerialMediateWrite(int fd, char *buf, int len)
{
#ifdef BEFORE_REALIZING_MAG_IS_HALF_DUPLEX
	int		i;
	struct timeval	moment;
	int		interdelay = 30000;
	int		postdelay = 60000;

	if (getenv("VRSERIAL_USEC"))
		sscanf(getenv("VRSERIAL_USEC"), "%d %d", &interdelay, &postdelay);

	if (vrDbgDo(SERIAL_DBGLVL) || getenv("VRSERIAL_DEBUG")) {
		write(2, ">", 1);
		write(2, buf, len-1);
		write(2, "< ", 2);
	}

	moment.tv_sec = 0;  moment.tv_usec = interdelay;
	select(0, NULL, NULL, NULL, &moment);
	for(i = 0; i < len; i++) {
		write(fd, buf+i, 1);
		moment.tv_sec = 0;  moment.tv_usec = interdelay;
		select(0, NULL, NULL, NULL, &moment);
	}
	moment.tv_sec = 0;  moment.tv_usec = postdelay - interdelay;
	select(0, NULL, NULL, NULL, &moment);

	return i;

#else	/* nowadays, we just take care not to write to the Magellan while */
	/* expecting a reply from it. */
	return write(fd, buf, len);
#endif
}


/************************************************************/
int vrSerialWrite(int fd, char *buf, int len)
{
	return write(fd, buf, len);
}


/************************************************************/
int vrSerialWriteString(int fd, char *buf)
{
	return write(fd, buf, strlen(buf));
}


/************************************************************/
int vrSerialRead(int fd, char *buf, int len)
{
	int	result;

	result = read(fd, buf, len);

	if (result < 0) {
		perror("vrSerialRead:");
		vrPrintf("YO: read(%d, %#p, %d) returned %d, with errno %d\n", fd, buf, len, result, errno);
	}

	return result;
}


/************************************************************/
/* for some devices (eg. FAROarm), we will want a vrSerialReadNbytes() */
/*   which waits for the <len> number of bytes before returning.       */
/************************************************************/
/* The FAROarm also has a vrSerialReadToCRLF(), which can probably     */
/*   make use of the following function (vrSerialReadToCR()).          */


/************************************************************/
/* We may want to add an option to use blocking mode rather */
/*   than the now-hardwired non-blocking mode.              */
/************************************************************/
char *vrSerialReadToChar(int fd, char endbyte, char *leftover_buf, int lo_bufsize, int lo_buflen, char *return_buf, int return_buflen)
{
	char	*last_byte;	/* pointer to the last byte of the incoming message (always a CR) */
	int	bytes_read;	/* number of bytes read from the serial port */
	int	msg_len;	/* length of the incoming message */
	int	bytes_left;	/* number of bytes left in the leftover buffer */
	int	count;

	while ((last_byte = memchr(leftover_buf, endbyte, lo_buflen)) == NULL) {
		/******************************************************/
		/* if we haven't yet read a CR, then keep reading and */
		/*   add new stuff into the buffer.                   */

		bytes_left = lo_bufsize-1 - lo_buflen;
		if (bytes_left <= 0) {
			vrFprintf(stderr, "vrSerialReadToChar(): Receive Buffer Overflow!  Had %d:", lo_buflen);
			for(count = 0; count < lo_buflen; count++) {
				printf((leftover_buf[count] == endbyte) ? "\\r"
					: (leftover_buf[count] >= ' ') ? "%c" : "\\x%02x",
					leftover_buf[count]);
			}
			vrFprintf(stderr, "\n");
			lo_buflen = 0;
			bytes_left = sizeof(leftover_buf);
		}
		bytes_read = read(fd, leftover_buf + lo_buflen, bytes_left);
		if (bytes_read < 0 || (bytes_read == 0 && lo_buflen <= 0))
			return NULL;

		if (bytes_read == 0) {	/* and, implicitly, lo_buflen > 0 */
			/* we have a partial line -- wait a bit for more to arrive */
			if (vrSerialAwaitData(fd) <= 0) {
				return NULL;
			}
			/* got some more -- loop again for next read */
			continue;
		}

		/* if debug envvar is set then print all incoming data */
		if (vrDbgDo(SERIAL_DBGLVL) || getenv("VRSERIAL_DEBUG")) {
			write(2, "[", 2);
			write(2, leftover_buf+lo_buflen, bytes_read - (leftover_buf[lo_buflen+bytes_read-1]=='\r' ? 1 : 0));
			write(2, "] ", 2);
		}

		lo_buflen += bytes_read;
	}

	/* calculate length of this message */
	msg_len = last_byte+1 - leftover_buf;
	if (msg_len > return_buflen) {
		vrFprintf(stderr, "vrSerialReadToChar(): Receive Buffer overflow!  Had %d:", msg_len);
		for(count = 0; count < msg_len; count++) {
			 printf((leftover_buf[count] == endbyte) ? "\\r"
				: (leftover_buf[count] >= ' ') ? "%c" : "\\x%02x",
				leftover_buf[count]);
		}
		vrFprintf(stderr, "\n");
	}

	/* Copy result (up to first CR) into return buffer, */
	/*   and ensure string is NULL-terminated.          */
	if (msg_len <= return_buflen) {
		memcpy(return_buf, leftover_buf, msg_len);
		return_buf[msg_len] = '\0';
	} else {
		memcpy(return_buf, leftover_buf, return_buflen);
		return_buf[return_buflen-1] = '\0';
	}

	/* If we have more buffered Mag data, shift it back to beginning of inbuf. */
	memcpy(leftover_buf, last_byte+1, lo_buflen - msg_len);
	lo_buflen -= msg_len;

	return return_buf;



#ifdef USE_BLOCKING_MODE
	/* OLD code from the blocking version of the magellan reader    */
	/*   use this when we go to implement a choice between blocking */
	/*   and non-blocking reads.                                    */

	char	readbuf[32];	/* buffer to read from the port */
	char	constbuf[256];	/* buffer to construct the mag command   */
				/*   memory problems if constbuf is [64] */
	char	*last_byte;

	do {
		last_byte = strchr(constbuf, endbyte);
		if (last_byte == NULL) {
			memset(readbuf, 0x00, sizeof(readbuf));
			read(port_id, readbuf, sizeof(readbuf));
			strncat(constbuf, readbuf, sizeof(constbuf)-strlen(constbuf));
			continue;
		} else {
			*last_byte = '\000';
			if (strlen(constbuf) > buflen) {
				printf("Receive Buffer overflow!\n\r");
			} else {
				strcpy(buf, constbuf);
				strcat(buf, "\r");
				strcpy(constbuf, ++last_byte);
				return strlen(buf);
			};
		};
	} while (TRUE);
#endif
}


/*******************************************************************/
/* read a string terminated by CRLF (unknown length)  -- or maybe just term'd by a CR */
/*   was: char *FARO_getrecord(int fd) */
/* NOTE: this code assumes the serial port was opened in **BLOCKING** mode */
/* NOTE Also: memory is allocated in here that will later need to be unallocated */
char *vrSerialReadToCR_Block(int fd)
{
static	char	inbuf[512];
	char	*retbuf;
	char	inchar;
	int	bytes_read = 0;
	int	byte_read;

	byte_read = read(fd, &inchar, 1);
	while ((inchar != '\r') && (byte_read == 1)) {
		inbuf[bytes_read] = inchar;
		bytes_read++;
		byte_read = read(fd, &inchar, 1);
	}
	inbuf[bytes_read] = '\0';
	if (byte_read == 0) {
		/* must have timed out -- remember, we're blocking */
		vrFprintf(stderr, "vrSerialReadToCR_Block(): Must have timed out doing a read.\n");
	}

	retbuf = (char *)malloc(bytes_read+1);
	memcpy(retbuf, inbuf, bytes_read+1);

	return (retbuf);
}


/*******************************************************************/
/* was: int MAG_Read(vrMagellanPrivateInfo *mag, char *buf, int buflen) */
char *vrSerialReadToCR(int fd, char *leftover_buf, int lo_bufsize, int lo_buflen, char *return_buf, int return_buflen)
{
	return(vrSerialReadToChar(fd, '\r', leftover_buf, lo_bufsize, lo_buflen, return_buf, return_buflen));
}

/*******************************************************************/
/* read a string of a specific length */
int vrSerialReadNbytes_Block(int fd, char *buf, int len)
{
	int	bytes_read = 0;

#if 0
	vrPrintf("entering vrSerialReadNbytes(%d, %d)\n", fd, len);
#endif

	do {
		vrSerialAwaitData(fd);
		bytes_read += read(fd, &(buf[bytes_read]), len - bytes_read);
#if 0
		if (bytes_read == 0)
			vrPrintf(".");
		else	vrPrintf("read %d bytes so far\n", bytes_read);
#endif
	} while (bytes_read < len);
	if (bytes_read != len) {
		vrFprintf(stderr, "vrSerialReadNbytes(): %d bytes ('%s') read from device doesn't match the %d bytes requested.\n", bytes_read, buf, len);
	}
	buf[bytes_read] = '\0';

#if 0
	printf("leaving vrSerialReadNbytes(%d, %d)\n", fd, len);
#endif
	return (bytes_read);
}


/*******************************************************************/
/* read a string of a specific length */
/*   was: char *FARO_getchars(int fd, int len) */
char *XXvrSerialReadNbytes_Block(int fd, int len)
{
static	char	inbuf[512];
	char	*retbuf;
	int	bytes_read = 0;

	vrPrintf("entering vrSerialReadNbytes(%d, %d)\n", fd, len);

	if (len > 512) {
		vrFprintf(stderr, "vrSerialReadNbytes(): Sorry can't handle more than 512 characters.\n");
		return NULL;
	}
	do {
		bytes_read += read(fd, &(inbuf[bytes_read]), len - bytes_read);
	} while (bytes_read < len);
	if (bytes_read != len) {
		vrFprintf(stderr, "vrSerialReadNbytes(): %d bytes ('%s') read from device doesn't match the %d bytes requested.\n", bytes_read, inbuf, len);
	}
	inbuf[bytes_read] = '\0';


	retbuf = (char *)malloc(len+1);
	memcpy(retbuf, inbuf, len+1);
	printf("leaving vrSerialReadNbytes(%d, %d)\n", fd, len);
	return (retbuf);
}


/* this is identical to vrSerialWriteString() */
int FARO_sendmsg(int fd, char *str)
{
	int	bytes_sent;

	bytes_sent = write(fd, str, strlen(str));
#if 0
	printf("FARO_sendmsg(): sent %d bytes of message '%s' (%d)\n", bytes_sent, str, strlen(str));
#endif

	return bytes_sent;
}

/************************************************************/
int vrSerialBaudIntToEnum(int baud_int)
{
	switch (baud_int) {
	case     50: return (    B50);
	case     75: return (    B75);
	case    110: return (   B110);
	case    134: return (   B134);
	case    150: return (   B150);
	case    200: return (   B200);
	case    300: return (   B300);
	case    600: return (   B600);
	case   1200: return (  B1200);
	case   1800: return (  B1800);
	case   2400: return (  B2400);
	case   4800: return (  B4800);
	case   9600: return (  B9600);
	case  19200: return ( B19200);
	case  38400: return ( B38400);
#ifdef B57600
	case  57600: return ( B57600);
#endif
#ifdef B115200
	case 115200: return (B115200);
#endif
#ifdef B230400
	case 230400: return (B230400);
#endif
#ifdef B460800
	case 460800: return (B460800);
#endif
	}

	vrErrPrintf(RED_TEXT "Unknown Baud value (%d)\n" NORM_TEXT, baud_int);
	return 0;
}

/************************************************************/
int vrSerialBaudEnumToInt(int baud_enum)
{
	switch (baud_enum) {
	case     B50: return (    50);
	case     B75: return (    75);
	case    B110: return (   110);
	case    B134: return (   134);
	case    B150: return (   150);
	case    B200: return (   200);
	case    B300: return (   300);
	case    B600: return (   600);
	case   B1200: return (  1200);
	case   B1800: return (  1800);
	case   B2400: return (  2400);
	case   B4800: return (  4800);
	case   B9600: return (  9600);
	case  B19200: return ( 19200);
	case  B38400: return ( 38400);
#ifdef B57600
	case  B57600: return ( 57600);
#endif
#ifdef B115200
	case B115200: return (115200);
#endif
#ifdef B230400
	case B230400: return (230400);
#endif
#ifdef B460800
	case B460800: return (460800);
#endif
	}

	vrErrPrintf(RED_TEXT "Unknown Baud value (%d)\n" NORM_TEXT, baud_enum);
	return 0;
}

