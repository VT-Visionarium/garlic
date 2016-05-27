/* ======================================================================
 *
 * HH   HH         vr_visren.pf.h
 * HH   HH         Author(s): Bill Sherman
 * HHHHHHH         Created: January 4, 2002
 * HH   HH         Last Modified: February 12, 2004
 * HH   HH
 *
 * Header file for FreeVR visual rendering into PF windows.
 *
 * Copyright 2002, Bill Sherman & Friends, All rights reserved.
 * With the intent to provide an open-source license to be named later.
 * ====================================================================== */
#ifndef __VRVISREN_PF_H__
#define __VRVISREN_PF_H__

#include <Performer/pf/pfChannel.h>

#ifdef __cplusplus
extern "C" {
#endif


/* some Performer functions */
pfMatrix	*vrPfMatrixFromVrMatrix(pfMatrix *pfmat, vrMatrix *vrmat);
vrMatrix	*vrMatrixFromPfMatrix(vrMatrix *vrmat, pfMatrix *pfmat);
void		 vrPfPreFrame(void);
void		 vrPfPostFrame(void);
pfChannel	*vrPfMasterChannel(void);
void		 vrPfDCSTransform6sensor(pfDCS *dcs, int sensor6_num);
void		 vrPfDCSTransformUserTravel(pfDCS *dcs, vrUserInfo *user);
int		 vrPfDCSTransformUserTravelCB(pfTraverser *trav, void *data);
void		 vrPfDCSTransformUserTravel_zup(pfDCS *dcs, vrUserInfo *user);
int		 vrPfDCSTransformUserTravelCB_zup(pfTraverser *trav, void *data);

#ifdef __cplusplus
}
#endif

#endif
