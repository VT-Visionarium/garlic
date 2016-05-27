/* ======================================================================
 *
 *  CCCCC          vr_sem_tcp.c
 * CC   CC         Author(s): Sukru Tikves
 * CC              Created: August 12, 2002
 * CC   CC         Last Modified: March 20, 2002
 *  CCCCC
 *
 * Code file for using a TCP interface to implement a semaphore system.
 *
 * Copyright 2002, Bill Sherman & Friends, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#if defined(SEM_TCP) || 1/* { this entire file is unneeded when SEM_TCP is not defined */

#ifdef SOCK_W32 /* Use WinSock 2 Library */
#  include <windows.h>
#  include <winsock2.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#ifdef NOT_CURRENTLY_USED
#include <time.h>
#include <sys/time.h>		/* needed for struct timeval on non MW-Windows systems */
#endif
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#ifndef SOCK_W32 /* Use BSD Sockets Library instead of WinSock 2 Library */
#  include <sys/socket.h>
#  include <netinet/in.h>
#  define closesocket close
#  define INVALID_SOCKET  -1
#  define SOCKADDR	struct sockaddr_in
   typedef int	SOCKET;

#endif

#include "vr_sem_tcp.h"
#include "vr_debug.h"

#define MAX_CLIENTS 64
#define MAX_MUTEX   8192

#ifdef printf
#undef printf
#endif

#ifdef fprintf
#undef fprintf
#endif


/****************************************************************************/
/*** File-scope variables ***/
static	int	port_id = 6500;
static	SOCKET	sock;
static	int	server_pid = -1;
static	int	*server_pid_p = &server_pid;

static	int	_TcpConnected = 0;


/****************************************************************************/
static void _TcpLog(const char *fmt, ...)
{
	va_list		args;
static	FILE		*log_file = NULL;

	if (!log_file)
		log_file = fopen("__tcp_mutex_log", "w");

	va_start(args, fmt);

	vfprintf(log_file, fmt, args);
	fflush(log_file);

	va_end(args);
}


/****************************************************************************/
void vrTcpInitSockets(void)
{
#ifdef WIN32
	WSADATA		wsaData;
	WORD		wVersionRequested;

	wVersionRequested = MAKEWORD(2, 0);

	if (WSAStartup(wVersionRequested, &wsaData))
		vrFatalError("TCP: Cannot initialize socket subsystem.\n");
#endif
}


/****************************************************************************/
static void _TcpServerInit(void)
{
	SOCKADDR	addr;

	_TcpLog("TCP: _TcpServerInit()\n");

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
		vrFatalError("TCP: Cannot allocate socket.");

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port_id);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)))	/* TODO: BSD socket API complains about arg 2: "incompatible pointer type"*/
		vrFatalError("TCP: Cannot bind socket.");

	if (listen(sock, 10))
		vrFatalError("TCP: Listen failed.");

	_TcpLog("TCP: _TcpServerInit() = OK\n");
}


/****************************************************************************/
static int _TcpProcessClient(SOCKET clients[], int id)
{
	int	mutex_id;
static	int	mutexes[MAX_MUTEX];
static	int	mutex_cnt = 1;
static	int	mutex_wait[MAX_CLIENTS];

	if (recv(clients[id], (char *)&mutex_id, sizeof(mutex_id), 0) != sizeof(mutex_id)) {
		closesocket(clients[id]);
		clients[id] = INVALID_SOCKET;
		return 0;
	}

	if (!mutex_id) {
		mutex_id = (mutex_cnt < MAX_MUTEX) ? mutex_cnt++ : 0;
		_TcpLog("TCP: new_mutex() = %d\n", mutex_id);
		send(clients[id], (char *)&mutex_id, sizeof(mutex_id), 0);

	} else if (mutex_id < 0) {
		mutex_id = -mutex_id;

		_TcpLog("TCP: release_mutex(%d)\n", mutex_id);

		if (mutex_id >= mutex_cnt || !mutexes[mutex_id]) {
			mutex_id = 0;

		} else if (mutexes[mutex_id]) {
			int	count;

			for (count = 0; count < MAX_CLIENTS; count++)
				if (clients[count] != INVALID_SOCKET && mutex_wait[count] == mutex_id) {
					send(clients[count], (char *)&mutex_id, sizeof(mutex_id), 0);
					mutex_wait[count] = 0;
				}

			mutexes[mutex_id] = 0;
		}

		mutex_id = -mutex_id;
		send(clients[id], (char *)&mutex_id, sizeof(mutex_id), 0);
	} else {
		_TcpLog("TCP: acquire_mutex(%d)\n", mutex_id);

		if (mutex_id >= mutex_cnt) {
			mutex_id = 0;
		} else if (mutexes[mutex_id]) {
			mutex_wait[id] = mutex_id;
			return 1;
		} else {
			mutexes[mutex_id] = 1;
		}

		send(clients[id], (char *)&mutex_id, sizeof(mutex_id), 0);
	}

	return 1;
}


/****************************************************************************/
static void _TcpServerLoop(void)
{
	fd_set		read_fds;
	SOCKET		clients[MAX_CLIENTS];
#ifdef NOT_CURRENTLY_USED
	struct timeval	time;
#endif
	int		count;
	int		num_clients = 0;

	for (count = 0; count < MAX_CLIENTS; count++)
		clients[count] = INVALID_SOCKET;

	while (1) {
		FD_ZERO(&read_fds);
		FD_SET(sock, &read_fds);

		for (count = 0; count < MAX_CLIENTS; count++)
			if (clients[count] != INVALID_SOCKET)
				FD_SET(clients[count], &read_fds);

#ifdef NOT_CURRENTLY_USED
		time.tv_sec = 1;
		time.tv_usec = 0;
#endif

		if (select(MAX_CLIENTS, &read_fds, NULL, NULL, NULL) < 0)
			vrFatalError("TCP: select() failed. MUTEX Server is aborting.\n");

		for (count = 0; count < MAX_CLIENTS; count++)
			if (FD_ISSET(clients[count], &read_fds))
				if (!_TcpProcessClient(clients, count)) {
#if 0
					printf("DISCONNECT\n");
#endif
					if (!--num_clients)
						return;
				}

		if (FD_ISSET(sock, &read_fds)) {
			for (count = 0; count < MAX_CLIENTS; count++)
				if (clients[count] == INVALID_SOCKET) {
					clients[count] = accept(sock, NULL, NULL);
#if 0
					printf("   CONNECT\n");
#endif
					num_clients++;
					break;
				}
		}
	}
}


/****************************************************************************/
static void _TcpServerMain(void)
{
	printf("TCP: Initializing MUTEX server...\n");

	vrTcpInitSockets();
	_TcpServerInit();

	vrDbgPrintfN(ALWAYS_DBGLVL, "TCP: MUTEX server is ready...\n");

	_TcpServerLoop();

	_TcpLog("TCP: Shutdown...\n");

	vrDbgPrintfN(ALWAYS_DBGLVL, "TCP: MUTEX server is shutting down...\n");

	exit(0);
}


/****************************************************************************/
static SOCKET _TcpConnect(void)
{
	SOCKET		sock;
	SOCKADDR	addr;

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		vrDbgPrintf("TCP: socket() failed.\n");
		return sock;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port_id);
	addr.sin_addr.s_addr = htonl(0x7F000001);

	if (connect(sock, (struct sockaddr *)&addr, sizeof(addr))) {	/* TODO: BSD socket API complains about arg 2: "incompatible pointer type" */
		vrDbgPrintf("TCP: connect() failed.\n");
		closesocket(sock);
		return INVALID_SOCKET;
	}

	_TcpConnected = 1;

	return sock;
}


/****************************************************************************/
MUTEX_HANDLE vrTcpNewMutex(void)
{
	SOCKET		sock;
	MUTEX_HANDLE	mutex_id = 0;

	vrTcpInitialize();

	sock = _TcpConnect();

	if (sock == INVALID_SOCKET) {
		vrErr("TCP: Connection failed for MUTEX.\n");
		return 0;
	}

	send(sock, (char *)&mutex_id, sizeof(mutex_id), 0);
	recv(sock, (char *)&mutex_id, sizeof(mutex_id), 0);

	if (mutex_id < 0)
		mutex_id = 0;

	if (!mutex_id)
		vrErr("TCP: Cannot allocate mutex.");

	closesocket(sock);

	return mutex_id;
}


/****************************************************************************/
void vrTcpDeleteMutex(MUTEX_HANDLE mutex_id)
{
	/* TODO: ... */
	vrDbgPrintf("TCP: TODO: Mutex delete operation isn't implemented.\n");
}


/****************************************************************************/
void vrTcpAcquireMutex(MUTEX_HANDLE mutex_id)
{
	SOCKET		sock = _TcpConnect();

	if (sock == INVALID_SOCKET) {
		vrErr("TCP: Connection failed for MUTEX.\n");
		return;
	}

	send(sock, (char *)&mutex_id, sizeof(mutex_id), 0);
	recv(sock, (char *)&mutex_id, sizeof(mutex_id), 0);

	closesocket(sock);
}


/****************************************************************************/
void vrTcpReleaseMutex(MUTEX_HANDLE mutex_id)
{
	SOCKET		sock = _TcpConnect();

	if (sock == INVALID_SOCKET) {
		vrErr("TCP: Connection failed for MUTEX.\n");
		return;
	}

	mutex_id = -mutex_id;

	send(sock, (char *)&mutex_id, sizeof(mutex_id), 0);
	recv(sock, (char *)&mutex_id, sizeof(mutex_id), 0);

	closesocket(sock);
}


/****************************************************************************/
void vrTcpInitialize(void)
{
static	int		initialized = 0;

	if (initialized)
		return;

	initialized = 1;

	printf("SEM_TCP debug: attempt to fork from pid %d (server_pid = %d, server_pid_p = %p\n", getpid(), server_pid, server_pid_p);
	if (!fork()) {
		/* New child process -- the Tcp Server */
		*server_pid_p = getpid();
		printf(BOLD_TEXT "FreeVR: Process \"%s\" forked!  " RED_TEXT "Pid = %d" NORM_TEXT BOLD_TEXT ", time = %f\n" NORM_TEXT,
			"TCP Server", server_pid, (double)vrCurrentSimTime());
		printf("SEM_TCP debug: Child: server_pid = %d, server_pid_p = %p\n", server_pid, server_pid_p);
		_TcpServerMain();
	} else {
		/* Parent processes, now conintues by initializing the sockets */
		/*   and attempting to connect to the server.                  */
		int	count = 0;

		vrTcpInitSockets();

		sleep(1);

		while (count++ < 3 && _TcpConnect() == INVALID_SOCKET) {
			printf("TCP: Waiting for MUTEX server...\n");
			sleep(1);
		}

		if (!_TcpConnected)
			vrFatalError("TCP: Cannot connect to server!");
#if 0
		else	vrDbgPrintfN(ALWAYS_DBGLVL, "TCP: Connected.\n");
#endif
	}
}


/****************************************************************************/
void vrTcpShutdown(void)
{
	printf("SEM_TCP debug: Shutdown: server_pid = %d, server_pid_p = %p\n", server_pid, server_pid_p);
	printf("SEM_TCP debug: Killing pid %d (I am %d)\n", server_pid, getpid());

	if (server_pid > 1)
		kill(server_pid, SIGHUP);
	else	printf("SEM_TCP debug: " BOLD_TEXT "Unable to terminate invalid server pid (%d), you'll have to kill it yourself.\n" NORM_TEXT, server_pid);
}

#endif /* } SEM_TCP */
