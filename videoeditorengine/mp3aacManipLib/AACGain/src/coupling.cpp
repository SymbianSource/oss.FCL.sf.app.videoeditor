/*
* Copyright (c) 2010 Ixonos Plc.
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - Initial contribution
*
* Contributors:
* Ixonos Plc
*
* Description:
*
*/


/**************************************************************************
  coupling.cpp - Coupling channel implementations (parsing only!).
 
  Author(s): Juha Ojanpera
  Copyright (c) 2000-2004 by Nokia Research Center, Speech and Audio Systems.
  *************************************************************************/

/*-- System Headers. --*/
#include <math.h>

/*-- Project Headers. --*/
#include "tool2.h"
#include "defines.h"

int16
GetCCE(CAACAudDec *aac, TBitStream *bs, CMC_Info *mip, CCInfo **ccInfo)
{
  TCh_Info cip;
  CToolInfo *tool;
  CWindowInfo *ccWin;
  int16 i, j, tag, cidx, ch, nele, nch;
  int16 shared[2 * ((LEN_NCC << 3) + 1)], global_gain;

  ZERO_MEMORY(&cip, sizeof(TCh_Info));

  /*-- Get (dummy) channel index for this coupling channel. --*/
  cidx = mip->dummyCCh;

  tool = ccInfo[cidx]->tool;
  ccWin = ccInfo[cidx]->winInfo;

  tag = (int16) BsGetBits(bs, LEN_TAG);
  mip->cc_ind[cidx] = (int16) BsGetBits(bs, LEN_IND_SW_CCE);

  /*-- Coupled (target) elements. --*/
  ZERO_MEMORY(shared, sizeof(shared));
  nele = (int16) BsGetBits(bs, LEN_NCC);
  for(i = 0, nch = 0; i < nele + 1; i++)
  {
    int16 cpe;
    
    cpe = (int16) BsGetBits(bs, LEN_IS_CPE);
    tag = (int16) BsGetBits(bs, LEN_TAG);
    ch = CCChIndex(mip, cpe, tag);

    if(!cpe)
      shared[nch++] = 0;
    else
    {
      int16 cc_l = (int16) BsGetBits(bs, LEN_CC_LR);
      int16 cc_r = (int16) BsGetBits(bs, LEN_CC_LR);
      j = (int16) ((cc_l << 1) | cc_r);

      switch(j)
      {
        /*-- Shared gain list. --*/
        case 0:
          shared[nch] = 1;
          shared[nch + 1] = 1;
          nch += 2;
          break;
 
        /*-- Left channel gain list. --*/
        case 1:
          shared[nch] = 0;
          nch += 1;
          break;

        /*-- Right channel gain list. --*/
        case 2:
          shared[nch] = 0;
          nch += 1;
          break;

        /*-- Two gain lists. --*/
        case 3:
          shared[nch] = 0;
          shared[nch + 1] = 0;
          nch += 2;
          break;
      
        default:
          shared[nch] = 0;
          shared[nch + 1] = 0;
          break;
      }
    }
  }
  
  int16 cc_dom = (int16) BsGetBits(bs, LEN_CC_DOM);
  int16 cc_gain_ele_sign = (int16) BsGetBits(bs, LEN_CC_SGN);
  int16 scl_idx = (int16) BsGetBits(bs, LEN_CCH_GES);

  /*-- Global gain. --*/
  global_gain = (int16) BsGetBits(bs, LEN_SCL_PCM);

  /*-- Side information. --*/
  if(!GetICSInfo(bs, ccWin, tool->ltp, NULL))
    return (-1);
  
  cip.info = mip->sfbInfo->winmap[ccWin->wnd];
  
  /*-- Channel parameters. --*/
  if (cip.sf_huf == 0) return (-1);
  if(!GetICS(bs, &cip, ccWin->group, ccWin->max_sfb, ccWin->cb_map, 
             tool->quant, global_gain, ccWin->sfac))
    return (-1);
  
  /*-- Coupling for first target channel(s) is already at correct scale. --*/
  ch = shared[0] ? (int16) 2 : (int16) 1;

  /*-- Bitstreams for target channel scale factors. --*/
  for( ; ch < nch; ch++)
  {
    /*-- If needed, get common gain elment present. --*/
    int16 cgep = (mip->cc_ind[cidx]) ? (int16) 1 : (int16) BsGetBits(bs, LEN_CCH_CGP);

    if(cgep) 
      int16 fac = (int16) (GetHcb(aac->sf_huf, bs) - MIDFAC);
    
    /*-- Must be dependently switched CCE. --*/
    else
      if(!huf_sfac(bs, &cip, ccWin->group, ccWin->cb_map, 0, ccWin->sfac, ccWin->max_sfb))
        return (-1);
  }

  return (TRUE);
}
