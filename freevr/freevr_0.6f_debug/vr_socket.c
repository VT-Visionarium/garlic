/* ======================================================================
 *
 *  CCCCC          vr_socket.c
 * CC   CC         Author(s): Bill Sherman, John Stone
 * CC              Created: October 22, 2001  (from code originally written in 1987)
 * CC   CC         Last Modified: September 1, 2014
 *  CCCCC
 *
 * Code file for FreeVR socket interfacing.
 *
 * Copyright 2014, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
/****************************************************************************/
/* Socket API by Bill Sherman                                               */
/*                                                                          */
/* This version is for TCP style sockets only.                              */
/*                                                                          */
/* ROUTINES:                                                                */
/*   int vrSocketCreateListen(port, block)                                  */
/*   int vrSocketCreateListenUDP(port, block)                               */
/*   void vrSocketClose(sock_fd)                                            */
/*   int vrSocketAnswer(sock_fd)                                            */
/*   int vrSocketCall(hostname, port)                                       */
/*   int vrSocketReadMsg(sock_fd, msg, msglen)                              */
/*   int vrSocketReadMsgClean(sock_fd, msg, msglen)                         */
/*   int vrSocketSendMsg(sock_fd, msg)                                      */
/*                                                                          */
/* HISTORY:                                                                 */
/*  1987/8: original version(s) isockets.c and usockets.c written by Bill   */
/*	Sherman as alternative (IP vs. Unix) socket routines for a series   */
/*	of programs for card playing over the Internet (way back in 1987).  */
/*	The Cards programs were written specifically to learn how to        */
/*	write sockets and curses programs.                                  */
/* ...                                                                      */
/*                                                                          */
/*  Feb 1998: renamed isockets.c to socket_api.c and refined the interface. */
/*      This was for my CAVE library tutorial, which eventually became my   */
/*      FreeVR library tutorial.                                            */
/*                                                                          */
/*  10/22/2001: incorporated into FreeVR library as vr_socket.c, with       */
/*	functions renamed to fit the FreeVR naming scheme.                  */
/*                                                                          */
/*  11/09/2010: (Bill Sherman)                                              */
/*      Added a new "vrSocketCreateListenUDP()" function that creates a     */
/*      socket specifically to listen on a UDP port -- needed for the       */
/*      dtrack interface.  It's mostly identical from the non-UDP version,  */
/*      And I started my experiments with the old (1987) usockets.c code.   */
/*                                                                          */
/*  11/09/2010: (Bill Sherman)                                              */
/*	Enhanced some of the error reporting to be more informative.        */
/*                                                                          */
/*  06/18/2014: (Bill Sherman)                                              */
/*	Altered the response codes for vrSocketReadMsgClean() to work better*/
/*      with "pvtest".                                                      */
/*                                                                          */
/*  09/01/2014: (Bill Sherman)                                              */
/*	Adjusted vrSocketReadMsgClean() to avoid a bug introduced by the    */
/*      change on 06/18/2014.  Now returns specific values for EOF and      */
/*      error results.                                                      */
/*                                                                          */
/* TODO:                                                                    */
/*   - Describe each function and its arguments in comments.                */
/*                                                                          */
/*   - Consider adding a version of SendMsg for non character strings.      */
/*                                                                          */
/*   - Consider whether I should use the recv() and send() system calls     */
/*     instead of read() and write.                                         */
/*                                                                          */
/*   - Re-investigate which include headers are actually required.          */
/*                                                                          */
/****************************************************************************/
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>		/* added by Matt Schechtman for Apple? */
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>		/* needed for fnctl() */
#include <string.h>		/* needed for strlen */
#include <strings.h>		/* needed for bzero */
#include <unistd.h>		/* needed for read() & write() */
#include <stdio.h>		/* needed for printf() & fprintf() */

#include "vr_socket.h"		/* really this is just to make sure function args match */

/* From debug.h -- but no need to include that entire file */
#define	NORM_TEXT	"[0m"
#define	BOLD_TEXT	"[1m"
#define	RED_TEXT	"[31m"
#define	SET_TEXT	"[%dm"

/*************************************************************************/
/* vrSocketCreateListen(): ...    */
/* port is used to assign a port.  If port==0, then an unassigned socket */
/*   is chosen, and the result is passed back via the port variable.     */
/*************************************************************************/
int vrSocketCreateListen(port, block)
	int	*port;		/* port is returned via this CBR variable */
	int	block;		/* flag to indicate whether to block (1) or not (0) */
{
struct	sockaddr_in	server_addr;		/* TODO: should this be "sockaddr" type? */
	int		sock_fd,
			fd_flags, fd_flags_block, fd_flags_noblock;
	socklen_t	length;			/* BS: 02/04/10 -- was unsigned int, changed for Onyx4 gcc */

	/* Create socket */
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd < 0) {
		perror("vrSocketCreateListen(): opening stream socket");
		return -1;
	}

	/* set socket port to blocking or non-blocking as indicated */
	fd_flags = fcntl(sock_fd, F_GETFL, 0);
	fd_flags_block = fd_flags & ~FNDELAY;  /* Turn blocking on */
	fd_flags_noblock = fd_flags | FNDELAY; /* Turn blocking off */
	fcntl(sock_fd, F_SETFL, block ? fd_flags_block : fd_flags_noblock);

	/* Name socket using inet port scheme */
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(*port);
	if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr))) { /* TODO: why is this "sockaddr", when the type is "sockaddr_in" ?? */
		vrErrPrintf("vrSocketCreateListen(): Trouble creating a listening socket: %d\n", *port);
		perror("vrSocketCreateListen():" RED_TEXT " ** binding stream socket **" NORM_TEXT);
		close(sock_fd);
		return -1;
	}

	length = sizeof(server_addr);
	if (getsockname(sock_fd, (struct sockaddr *)&server_addr, &length)) { /* TODO: why is this "sockaddr", when the type is "sockaddr_in" ?? */
		vrErrPrintf("vrSocketCreateListen(): Trouble creating a listening socket: %d\n", *port);
		perror("vrSocketCreateListen():" RED_TEXT " ** getting socket name **" NORM_TEXT);
		return -1;
	}
	*port = ntohs(server_addr.sin_port);
	listen(sock_fd, 5);
	return sock_fd;
}


/*************************************************************************/
/* vrSocketCreateListenUDP(): ...    */
/* port is used to assign a port.  If port==0, then an unassigned socket */
/*   is chosen, and the result is passed back via the port variable.     */
/*************************************************************************/
int vrSocketCreateListenUDP(port, block)
	int	*port;		/* port is returned via this CBR variable */
	int	block;		/* flag to indicate whether to block (1) or not (0) */
{
struct	sockaddr_in	server_addr;		/* TODO: should this be "sockaddr" type? */
	int		sock_fd,
			fd_flags, fd_flags_block, fd_flags_noblock;
	socklen_t	length;		/* BS: 02/04/10 -- was unsigned int, changed for Onyx4 gcc */

	/* Create socket */
	sock_fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock_fd < 0) {
		perror("vrSocketCreateListenUDP(): opening stream socket");
		return -1;
	}

	/* set socket port to blocking or non-blocking as indicated */
	fd_flags = fcntl(sock_fd, F_GETFL, 0);
	fd_flags_block = fd_flags & ~FNDELAY;  /* Turn blocking on */
	fd_flags_noblock = fd_flags | FNDELAY; /* Turn blocking off */
	fcntl(sock_fd, F_SETFL, block ? fd_flags_block : fd_flags_noblock);

	/* Name socket using inet port scheme */
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(*port);
#if 1 /* BS: 11/27/12 enabled this section -- seems to do better error reporting -- why wasn't this section being used? */
	if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr))) { /* TODO: why is this "sockaddr", when the type is "sockaddr_in" ?? */
		vrErrPrintf("vrSocketCreateListenUDP(): Trouble creating a listening UDP socket: %d\n", *port);
		perror("vrSocketCreateListenUDP():" RED_TEXT " ** binding stream socket **" NORM_TEXT);
		close(sock_fd);
		return -1;
	}
#else
int return_val;
	return_val = bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
		vrErrPrintf("vrSocketCreateListenUDP(): bind returns %d (UDP port: %d\n", return_val, *port);
		perror("vrSocketCreateListenUDP():" RED_TEXT " ** binding stream socket **" NORM_TEXT);
#endif

	length = sizeof(server_addr);
	if (getsockname(sock_fd, (struct sockaddr *)&server_addr, &length)) { /* TODO: why is this "sockaddr", when the type is "sockaddr_in" ?? */
		perror("vrSocketCreateListenUDP():" RED_TEXT " ** getting socket name **" NORM_TEXT);
		return -1;
	}
	*port = ntohs(server_addr.sin_port);
	listen(sock_fd, 5);
	return sock_fd;
}


/******************************************************/
/* vrSocketClose(): ... */
/******************************************************/
void vrSocketClose(sock_fd)
	int	sock_fd;
{
	close(sock_fd);
	shutdown(sock_fd, 2);
}


/******************************************************/
/* vrSocketAnswer(): ... */
/******************************************************/
/* if vrSocketAnswer() returns -1, then no connection is made -- which  */
/*    is fine for non-blocking sockets.                                */
int vrSocketAnswer(sock_fd)
	int	sock_fd;
{
	int	asock;

	asock = accept(sock_fd, 0, 0);

	return asock;
}


/******************************************************/
/* vrSocketCall(): ... */
/******************************************************/
/* this routine is used by the client end of a connection */
int vrSocketCall(hostname, port /* , block */)
	char	*hostname;
	int	port;
#if 0
	int	block; /* not sure if this will be useful */
#endif
{
struct	sockaddr_in	client_addr;		/* TODO: should this be "sockaddr" type? */
struct	hostent		*hp,
			*gethostbyname();
	int		sock_fd;

	/* Create socket */
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd < 0) {
		perror("vrSocketCall(): opening stream socket");
		return -1;
	}

	/* Name socket using file system name */
	if (hostname == NULL) {
		vrErrPrintf("vrSocketCall(): No hostname specified, using 'localhost'.\n");
		hp = gethostbyname("localhost");
	} else {
		hp = gethostbyname(hostname);
	}
	if (hp == NULL) {
		vrErrPrintf("vrSocketCall(): %s: unknown host\n", hostname);
		return -1;
	}
	bcopy(hp->h_addr, &client_addr.sin_addr, hp->h_length);

	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(port);

	if (connect(sock_fd, (struct sockaddr *)&client_addr, sizeof(struct sockaddr_in)) < 0) { /* TODO: why is this "sockaddr", when the type is "sockaddr_in" ?? */
		close(sock_fd);
		vrErrPrintf("vrSocketCall(): Trouble connecting to '%s':%d\n", hostname, port);
		perror("vrSocketCall(): connecting stream socket");
		return -1;
	}
	return sock_fd;
}


/******************************************************/
/* vrSocketReadMsg(): ... */
/******************************************************/
int vrSocketReadMsg(sock_fd, msg, msglen)
	int	sock_fd;
	char	*msg;
	int	msglen;
{
	int	rval;

	bzero(msg, msglen);

	rval = read(sock_fd, msg, msglen);

	return rval;
}


/***************************************************************/
/* vrSocketReadMsgClean(): ...                                 */
/* same as vrSocketReadMsg, but strips tail '\n'               */
/*   06/18/2014 -- fixed two related bugs:                     */
/*     1) subtracting two from rval after erase two characters */
/*     2) not erasing characters if no message received.       */
/* CAUTION: the return values aren't consistent with vrSocketReadMsg() */
/*   Here:                                                             */
/*     * EOF (-1) indidates an EOF -- closed socket                    */
/*     * -2 indidates an error in the read                             */
/*     * 0 (which normally means EOF) indicates a 0-length message     */
/***************************************************************/
int vrSocketReadMsgClean(sock_fd, msg, msglen)
	int	sock_fd;
	char	*msg;
	int	msglen;
{
	int	rval;

	bzero(msg, msglen);
	rval = read(sock_fd, msg, msglen);

	/* return immediately if EOF received */
	if (rval == 0)
		return EOF;

	/* return immediately if a valid message wasn't received */
	if (rval < 0)
		return -2;	/* NOTE: normally -1 is the error signal, but that's the actual value of EOF, so we have to shift. */
	
	/* clean off trailing '\n' */
	if (msg[rval-1] == '\012') {
		rval--;
		msg[rval] = '\0';
	}
	if (msg[rval-1] == '\r') {
		rval--;
		msg[rval] = '\0';
	}

	return rval;
}


/******************************************************/
/* vrSocketSendMsg(): ... */
/******************************************************/
int vrSocketSendMsg(sock_fd, msg)
	int	sock_fd;
	char	*msg;
{
	int	rval;

#if 0 /* this is basically a nop (or should be) but for some reason causes a seg fault on Linux */
	/* TODO: figure out why this seg faults on Linux */
	msg[strlen(msg)] = '\0';
#endif

#if 0
	rval = write(sock_fd, msg, strlen(msg)+1);		/* 06/15/2006: Why the "+1" -- why send the trailing '\0'?  Today it's causing me problems with the Tcl/TK interface. */
#else
	rval = write(sock_fd, msg, strlen(msg));
#endif
	if (rval < 0)
		perror("vrSocketSendMsg(): unable to send message");

	return rval;
}

