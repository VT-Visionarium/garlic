/* ======================================================================
 *
 * HH   HH         vr_utils.h
 * HH   HH         Author(s): John Stone, Bill Sherman
 * HHHHHHH         Created: August 10, 2000
 * HH   HH         Last Modified: May 24, 2013
 * HH   HH
 *
 * Header file for miscellaneous machine and operating system dependent
 * utility routines.
 *
 * Copyright 2013, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#ifndef __VRUTILS_H__
#define __VRUTILS_H__

#ifdef __cplusplus
extern "C" {
#endif


void	vrSleep(long useconds);
void	vrShellCmd(char *cmdstr, char *name, int pid, int error_code);
char	*vrProcessString(char *string);
char	*vrPrintableString(char *string);
int	vrStringCharWidth(char *str);
int	vrStringCharHeight(char *str);
int	vrSearchStringList(char *item, int list_length, char *list[]);
int	vrSearch256StringList(char *item, int list_length, char list[][256]);
void	vrEndianSwap4(void *bytes);
void	vrEndianSwap8(void *bytes);


#ifdef __cplusplus
}
#endif

#endif /* __VRUTILS_H__ */
