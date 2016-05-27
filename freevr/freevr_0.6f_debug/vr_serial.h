/* ======================================================================
 *
 * HH   HH         vr_serial.h
 * HH   HH         Author(s): Bill Sherman
 * HHHHHHH         Created: November 23, 1999
 * HH   HH         Last Modified: February 14, 2000
 * HH   HH
 *
 * Header file for FreeVR serial communication functions.
 *
 * Copyright 2000, Bill Sherman & Friends, All rights reserved
 * With the intent to provide an open-source licence to be named later
 * ====================================================================== */
#ifndef __VRSERIAL_H__
#define __VRSERIAL_H__

/***** Functions *****/

int	vrSerialOpen(char *serialPort, int baud_enum);
int	vrSerialFlushI(int fd);
int	vrSerialFlushO(int fd);
int	vrSerialFlushIO(int fd);
int	vrSerialDrain(int fd);
int	vrSerialClose(int fd);
int	vrSerialAwaitData(int fd);
int	vrSerialMediateWrite(int fd, char *buf, int len);
int	vrSerialWrite(int fd, char *buf, int len);
int	vrSerialWriteString(int fd, char *buf);
int	vrSerialRead(int fd, char *buf, int len);
char	*vrSerialReadToChar(int fd, char endbyte, char *leftover_buf, int lo_bufsize, int lo_buflen, char *return_buf, int return_buflen);
char	*vrSerialReadToCR(int fd, char *leftover_buf, int lo_bufsize, int lo_buflen, char *return_buf, int return_buflen);
char	*vrSerialReadToCR_Alt(int fd);
int	vrSerialReadNbytes_Block(int fd, char *buf, int len);
char	*XXvrSerialReadNbytes(int fd, int len);
int	vrSerialBaudIntToEnum(int baud_int);
int	vrSerialBaudEnumToInt(int baud_enum);

#endif  /* __VRSERIAL_H__ */
