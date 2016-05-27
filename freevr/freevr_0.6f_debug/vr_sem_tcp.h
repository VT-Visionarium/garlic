/* ======================================================================
 *
 * HH   HH         vr_sem_tcp.h
 * HH   HH         Author(s): Sukru Tikves
 * HHHHHHH         Created: August 12, 2002
 * HH   HH         Last Modified:  September 11, 2002
 * HH   HH
 *
 * Header file for using a TCP interface to implement a semaphore system.
 *
 * Copyright 2002, Bill Sherman & Friends, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#ifndef __VR_SEM_TCP__
#define __VR_SEM_TCP__

#ifdef __cplusplus
extern "C" {
#endif

typedef int MUTEX_HANDLE;

MUTEX_HANDLE	vrTcpNewMutex(void);
void		vrTcpDeleteMutex(MUTEX_HANDLE);
void		vrTcpAcquireMutex(MUTEX_HANDLE);
void		vrTcpReleaseMutex(MUTEX_HANDLE);

void		vrTcpInitSockets(void);
void		vrTcpInitialize(void);
void		vrTcpShutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* __VR_SEM_TCP__ */
