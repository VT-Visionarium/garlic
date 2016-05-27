/* ======================================================================
 *
 *  CCCCC          fvconfig.c
 * CC   CC         Author(s): Bill Sherman
 * CC              Created: November 10, 1999
 * CC   CC         Last Modified: May 12, 2006
 *  CCCCC
 *
 * Non-application that simply does the configuration, prints some info
 *   about it, and exits.
 *
 * Copyright 2006, Bill Sherman & Friends, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#include <stdio.h>
#include "freevr.h"

#include "vr_objects.h"		/* for vrObjectInfo type */

int main (int argc, char* argv[])
{
	vrObjectInfo	*object = NULL;
	int		count;

	vrConfigure(&argc, argv, NULL);

#if 0
	vrFprintContext(stdout, vrContext, verbose);
	vrFprintConfig(stdout, vrContext->config, brief);
#else
	/* I'm developing a new option that simply prints all the systems available */
	/*    in the configuration.                                                 */
#  if 0
	vrFprintObjectTypeInfo(stdout, vrContext, "system", "list", one_line);
#  endif
	object = vrObjectFirst(vrContext, VROBJECT_SYSTEM);
	while (object != NULL) {
		vrFprintf(stdout, "%2d: ", count);
		vrFprintObjectInfo(stdout, object, one_line);
		object = object->next;
		count++;
	}
#endif

#if 0
	/* the input structure isn't initialized until vrStart() is called */
	vrFprintInput(stdout, vrContext->input, brief);
#endif
}

