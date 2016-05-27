/* ======================================================================
 *
 *  CCCCC          vr_utils.c
 * CC   CC         Author(s): John Stone, Bill Sherman
 * CC              Created: August 10, 2000
 * CC   CC         Last Modified: May 24, 2013
 *  CCCCC
 *
 * Code file for miscellaneous machine and operating system dependent
 * utility routines.
 *
 * Copyright 2013, Bill Sherman, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */

#include <stdio.h> 		/* for sprintf() */
#include <string.h>		/* for myriad string functions */
//#include <errno.h> /* TODO: probably delete this line */
//#include <unistd.h> /* TODO: probably delete this line */
#include "vr_shmem.h"		/* for one call to vrShmemStrDup() */
#include "vr_utils.h"
#include "vr_debug.h"

#if defined(TEST_APP)
#  define	vrShmemStrDup	strdup
#endif

/************************************************************/
void vrSleep(long useconds) {

	if (useconds <= 0) {
#if defined(SGINAP)
		/* only use sginap() on IRIX 5.x where usleep is not available */
		sginap(0);
#else
		usleep(0);	/* often just calling usleep() is important to release the CPU */
#endif
		return;
	}

#if defined(SGINAP)
	/* only use sginap() on IRIX 5.x where usleep is not available */
	sginap(useconds / 10000);
#else
	/* handles delays longer than one second */
	while (useconds >= 1000000) {
		usleep(999999);
		useconds -= 999999;
	}
#  if !defined(__linux) /* on systems that produce higher-res waits with calls less than 10000ms, this can improve resolution */
	while (useconds >= 5000) {
		usleep(5000);
		useconds -= 5000;
	}
#  endif
	usleep(useconds);
#endif
}


/**********************************************************************/
void vrShellCmd(char *cmdstr, char *name, int pid, int error_code)
{
	if (cmdstr == NULL)
		return;

	if (strlen(cmdstr) > 1) {
		int	no_subst;		/* true when no substitution done thus far in a loop */
		char	syscmd[1024];
		char	syscmd2[1024];
		char	*subst = NULL;
		char	*percent_loc = NULL;

		strcpy(syscmd, cmdstr);

		/* loop until no substitutions were done */
		/*   NOTE: we do this to make sure multiple        */
		/*   instances of the same substitution will work. */
		do {
			no_subst = 1;

			/* replace "#P" with the process pid */
			subst = strstr(syscmd, "#P");
			if (subst != NULL) {
				/* replace all occurrences of '%' w/ "%%" first */
				while ((percent_loc = strchr(syscmd, '%')) != NULL) {
					strncpy(syscmd2, syscmd, (size_t)(percent_loc - syscmd));
					strcat(syscmd2, percent_loc);
				}

				/* now do the replacement */
				strncpy(syscmd2, syscmd, (subst - syscmd));
				syscmd2[subst - syscmd] = '\0';
				strcat(syscmd2, "%d");
				strcat(syscmd2, &(subst[2]));
				sprintf(syscmd, syscmd2, pid); /* TODO: replace with snprintf, which doesn't exist in IRIX 6.2 */

				no_subst = 0;
			}

			/* replace "#N" with the process name */
			subst = strstr(syscmd, "#N");
			if (subst != NULL) {
				/* replace all occurrences of '%' w/ "%%" first */
				while ((percent_loc = strchr(syscmd, '%')) != NULL) {
					strncpy(syscmd2, syscmd, (size_t)(percent_loc - syscmd));
					strcat(syscmd2, percent_loc);
				}

				/* now do the replacement */
				strncpy(syscmd2, syscmd, (subst - syscmd));
				syscmd2[subst - syscmd] = '\0';
				strcat(syscmd2, "%s");
				strcat(syscmd2, &(subst[2]));
				sprintf(syscmd, syscmd2, name); /* TODO: replace with snprintf, which doesn't exist in IRIX 6.2 */

				no_subst = 0;
			}

			/* replace "#E" with the error code */
			subst = strstr(syscmd, "#E");
			if (subst != NULL) {
				/* replace all occurrences of '%' w/ "%%" first */
				while ((percent_loc = strchr(syscmd, '%')) != NULL) {
					strncpy(syscmd2, syscmd, (size_t)(percent_loc - syscmd));
					strcat(syscmd2, percent_loc);
				}

				/* now do the replacement */
				strncpy(syscmd2, syscmd, (subst - syscmd));
				syscmd2[subst - syscmd] = '\0';
				strcat(syscmd2, "%d");
				strcat(syscmd2, &(subst[2]));
				sprintf(syscmd, syscmd2, error_code); /* TODO: replace with snprintf, which doesn't exist in IRIX 6.2 */

				no_subst = 0;
			}

		} while (no_subst == 0);

		system(syscmd);
	}
}


/**********************************************************************/
/* Convert special character sequences into new forms.  E.g. a "\n"   */
/*   sequence into a newline character.                               */
/* TODO: probably shouldn't have the must-be-shorter-than-4096-byte   */
/*   restriction.                                                     */
char *vrProcessString(char *string)
{
	if (string == NULL)
		return NULL;

	if (strlen(string) > 4096) {
		vrDbgPrintfN(1, "vrProcessString(): can't handle strings longer than 4096 bytes.\n");
		return (string);
	}

	if (strlen(string) > 1) {
		int	no_subst;		/* true when no substitution done thus far in a loop */
		char	str1[4096];
		char	str2[4096];
		char	*subst = NULL;
		char	*percent_loc = NULL;

		strcpy(str1, string);

		/* loop until no substitutions were done */
		/*   NOTE: we do this to make sure multiple        */
		/*   instances of the same substitution will work. */
		do {
			no_subst = 1;

			/* replace "\n" with the newline character */
			subst = strstr(str1, "\\n");
			if (subst != NULL) {
				/* now do the replacement */
				strncpy(str2, str1, (subst - str1));
				str2[subst - str1] = '\0';
				strcat(str2, "\n");
				strcat(str2, &(subst[2]));
				strcpy(str1, str2);

				no_subst = 0;
			}

		} while (no_subst == 0);

		return (vrShmemStrDup(str1));		/* TODO: consider whether this really needs to be in shared memory */
	}
}


/**********************************************************************/
/* Convert non-printable strings to print a "." instead.              */
/* TODO: probably shouldn't have the must-be-shorter-than-4096-byte   */
/*   restriction.                                                     */
char *vrPrintableString(char *string)
{
	int	count;
	char	str1[4096];

	if (string == NULL)
		return NULL;

	if (strlen(string) > 4096) {
		vrDbgPrintfN(1, "vrPrintableString(): can't handle strings longer than 4096 bytes.\n");
		return (string);
	}

	for (count = 0; count < strlen(string); count++) {
		if (isprint(string[count]))
			str1[count] = string[count];
		else	str1[count] = '_';
	}
	str1[count] = string[count];	/* now copy the terminating '\0' */
	
	return (vrShmemStrDup(str1));		/* TODO: consider whether this really needs to be in shared memory */
}


/***************************************************************/
/* vrStringCharWidth(): loops through all the lines of text in */
/*   a string (ie. each newline), and reports the widest line  */
/*   (which can be interpreted as the character-width of the   */
/*   string).                                                  */
int vrStringCharWidth(char *str)
{
	int	count;
	int	max_width = 0;
	int	curr_width = 0;
	int	str_len = strlen(str);

	for (count = 0; count < str_len; count++) {
		if (str[count] == '\n') {
			if (curr_width > max_width)
				max_width = curr_width;
			curr_width = 0;
		} else {
			curr_width++;
		}
	}

	return max_width;
}


/**************************************************************/
/* vrStringCharHeight(): loops through a string, counting the */
/*   number of newline characters (which can be interpreted   */
/*   as the character-height of the string).                  */
int vrStringCharHeight(char *str)
{
	int	count;
	int	str_len = strlen(str);
	int	result = 1;			/* there is at least one line */

	for (count = 0; count < str_len; count++) {
		if (str[count] == '\n')
			result++;
	}

	return result;
}


/******************************************************************/
/* Okay, this was intended to search a list of arbitrary length   */
/*   strings, but of course that was a ridiculous expectation of  */
/*   C.  So for now, don't use this function.  There must be a    */
/*   reasonable solution, but for the moment we'll have a version */
/*   that requires 256 length strings.                            */
int vrSearchStringList(char *item, int list_length, char *list[])
{
	int	count;

	vrDbgPrintfN(1, "vrSearchStringList(%s, %d, %#p)\n", item, list_length, list);
	for (count = 0; count < list_length; count++) {
		vrDbgPrintfN(1, "...vrSearchStringList() %d: comparing item with '%s'(%#p)\n", count, list[count], list[count]);
		if (!strcmp(item, list[count])) {
			vrDbgPrintfN(1, "...vrSearchStringList() returning %d\n", count);
			return count;
		}
	}

	vrDbgPrintfN(1, "...vrSearchStringList() returning -1\n");
	return -1;
}


/************************************************************/
/* a version of the above that assumes (ie. requires) that the */
/*   strings all have a length of 256 bytes.                   */
int vrSearch256StringList(char *item, int list_length, char list[][256])
{
	int	count;

	vrDbgPrintfN(1, "vrSearch256StringList(%s, %d, %#p)\n", item, list_length, list);
	for (count = 0; count < list_length; count++) {
		vrDbgPrintfN(1, "...vrSearchStringList() %d: comparing item with '%s'(%#p)\n", count, list[count], list[count]);
		if (!strcmp(item, list[count])) {
			vrDbgPrintfN(1, "...vrSearch256StringList() returning %d\n", count);
			return count;
		}
	}

	vrDbgPrintfN(1, "...vrSearch256StringList() returning -1\n");
	return -1;
}

/****************************************************************************/
/* vrEndianSwap4(): does an in-place Endian swap of a 4-byte integer number */
void vrEndianSwap4(void *bytes)
{
	char	temp0, temp1;
	char	*cbytes = (char *)bytes;

	temp0 = cbytes[0];
	temp1 = cbytes[1];
	cbytes[0] = cbytes[3];
	cbytes[1] = cbytes[2];
	cbytes[2] = temp1;
	cbytes[3] = temp0;
}

/*****************************************************************************/
/* vrEndianSwap8(): does an in-place Endian swap of an 8-byte integer number */
void vrEndianSwap8(void *bytes)
{
	char	temp;
	char	*cbytes = (char *)bytes;

	temp = cbytes[0];
	cbytes[0] = cbytes[7];
	cbytes[7] = temp;

	temp = cbytes[1];
	cbytes[1] = cbytes[6];
	cbytes[6] = temp;

	temp = cbytes[2];
	cbytes[2] = cbytes[5];
	cbytes[5] = temp;

	temp = cbytes[3];
	cbytes[3] = cbytes[4];
	cbytes[4] = temp;
}

