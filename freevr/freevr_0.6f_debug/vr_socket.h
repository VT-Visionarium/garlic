/* ======================================================================
 *
 * HH   HH         vr_socket.h
 * HH   HH         Author(s): Bill Sherman
 * HHHHHHH         Created: October 22, 2001
 * HH   HH         Last Modified: October 22, 2001
 * HH   HH
 *
 * Header file for FreeVR socket communication functions.
 *
 * Copyright 2010, Bill Sherman & Friends, All rights reserved
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#ifndef __VRSOCKET_H__
#define __VRSOCKET_H__

/***** Functions *****/

int	vrSocketCreateListen(int *port, int block);
int	vrSocketCreateListenUDP(int *port, int block);
void	vrSocketClose(int sock_fd);
int	vrSocketAnswer(int sock_fd);
int	vrSocketCall(char *hostname, int port);
int	vrSocketReadMsg(int sock_fd, char *msg, int msglen);
int	vrSocketReadMsgClean(int sock_fd, char *msg, int msglen);
int	vrSocketSendMsg(int sock_fd, char *msg);

#endif  /* __VRSOCKET_H__ */
