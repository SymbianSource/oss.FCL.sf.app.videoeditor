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
  huffdec1.cpp - Bit parsing routines for decoding LFE, SCE and CPE audio elements.
 
  Author(s): Juha Ojanpera
  Copyright (c) 2000-2004 by Nokia Research Center, Speech and Audio Systems.
  *************************************************************************/

/*-- Project Headers. --*/
#include "tool.h"
#include "tool2.h"

#define SCE_START_BITS (LEN_TAG + LEN_SCL_PCM)
#define CPE_START_BITS (LEN_TAG + LEN_COM_WIN)

/**************************************************************************
  Title:        getmask

  Purpose:      Reads MS mask and MS flag bits from the bitstream.

  Usage:        y = getmask(bs, info, group, max_sfb, mask)

  Input:        bs      - bitstream parameters
                info    - block (long/short) parameters
                group   - grouping info for short blocks
                max_sfb - # of sfb's present in the channel pair

  Output:       y       - value of MS mask in the bitstream
                mask    - flag bit for each sfb (only if y == 1)

  Author(s):    Juha Ojanpera
  *************************************************************************/

INLINE int16
getmask(TBitStream *bs, CInfo *info, uint8 *group, uint8 max_sfb, uint8 *mask)
{
  int16 b, i, mp;
  
  mp = (int16) BsGetBits(bs, LEN_MASK_PRES);
  
  /*--Get mask. --*/
  if(mp == 1)
  {
    for(b = 0; b < info->nsbk; b = *group++)
    {
      int16 sfb;
      uint32 tmp, flag;

#if 1
      /*
       * Speed up the reading of the flags.
       */
      sfb = max_sfb;
      if(sfb > 32) sfb = 32;
      flag = 1 << (sfb - 1);
      tmp = BsGetBits(bs, sfb);
      
      for(i = 0; i < sfb; i++, mask++, flag >>= 1)
        *mask = (tmp & flag) ? (uint8) 1 : (uint8) 0;

      if(sfb == 32)
      {
        sfb = int16(max_sfb - 32);
        flag = 1 << (sfb - 1);
        tmp = BsGetBits(bs, sfb);

        for( ; i < max_sfb; i++, mask++, flag >>= 1)
          *mask = (tmp & flag) ? (uint8) 1 : (uint8) 0;
      }

#else
      for(i = 0; i < max_sfb; i++, mask++)
        *mask = (uint8) BsGetBits(bs, LEN_MASK);
      mask += info->sfb_per_sbk[0] - i;
#endif
    }
  }
  else if(mp == 2)
  {
    for(b = 0; b < info->nsbk; b = *group++)
    {
      SET_MEMORY(mask, max_sfb, 1);
      mask += info->sfb_per_sbk[0];
    }
  }
  else
  {
    for(b = 0; b < info->nsbk; b = *group++)
    {
      ZERO_MEMORY(mask, max_sfb);
      mask += info->sfb_per_sbk[0];
    }
  }
  
  return (mp);
}

/**************************************************************************
  Object Definitions
  *************************************************************************/

/**************************************************************************
  Title:        GetSCE

  Purpose:      Decodes LFE/SCE audio element from the bitstream.

  Usage:        y = GetSCE(aac, bs, mip, gains, gainPos, bufBitOffset)

  Input:        bs           - bitstream parameters
                bufBitOffset - # of bits read read so far

  Output:       y            - 0 on success, -1 otherwise
                mip
                  ch_info    - channel mapping parameters
                aac
                  winInfo    - block/window parameters for each channel
                  toolInfo
                    quant    - quantized spectral coefficients
                gains        - 'global_gain' bitstream element
                gainPos      - location of 'global_gain' within the bitstream

  Author(s):    Juha Ojanpera
  *************************************************************************/

int16
GetSCE(CAACAudDec *aac, TBitStream *bs, CMC_Info *mip, uint8 *gains, 
       uint32 *gainPos, uint32 bufBitOffset)
{
  int16 ch, global_gain;
  CWindowInfo **winInfo;
  CToolInfo **toolInfo;
  CWindowInfo *win;
  CToolInfo *tool;
  TCh_Info *cip;
  uint32 tmp;

  toolInfo = aac->tool;
  winInfo = aac->winInfo;

  tmp = BsGetBits(bs, SCE_START_BITS);
  
  ch = ChIndex(1, (int16) (tmp >> LEN_SCL_PCM), 0, mip);
  cip = &mip->ch_info[ch];
  tool = toolInfo[ch];
  win = winInfo[ch];

  /*-- Global gain. --*/
  global_gain = (int16) (tmp & 0xFF);

  /*-- Save global gain element and its position. --*/
  if(gainPos)
  {
    gains[0] = (uint8) global_gain;
    gainPos[0] = BsGetBitsRead(bs) - bufBitOffset - LEN_SCL_PCM;
  }

  if(!GetICSInfo(bs, win, tool->ltp, NULL)) return (-1);
  
  cip->info = mip->sfbInfo->winmap[win->wnd];

  win->hasmask = 0;
  tool = toolInfo[ch];

  if(!GetICS(bs, cip, win->group, win->max_sfb, win->cb_map,
             tool->quant, global_gain, win->sfac))
    return (-1);

  return (0);
}

/**************************************************************************
  Title:        GetCPE

  Purpose:      Decodes CPE audio element from the bitstream.

  Usage:        y = GetCPE(aac, bs, mip, gains, gainPos, bufBitOffset

  Input:        bs           - bitstream parameters
                bufBitOffset - # of bits read read so far

  Output:       y            - 0 on success, -1 otherwise
                mip
                  ch_info    - channel mapping parameters
                aac
                  winInfo    - block/window parameters for each channel
                  toolInfo
                    quant    - quantized spectral coefficients
                gains        - 'global_gain' bitstream element
                gainPos      - location of 'global_gain' within the bitstream

  Author(s):    Juha Ojanpera
  *************************************************************************/

int16
GetCPE(CAACAudDec *aac, TBitStream *bs, CMC_Info *mip, uint8 *gains, 
       uint32 *gainPos, uint32 bufBitOffset)
{
  int16 i, common_window, ch;
  CWindowInfo **winInfo;
  CToolInfo **toolInfo;
  int16 global_gain;
  CWindowInfo *win;
  CToolInfo **tool;
  TCh_Info *cip;

  toolInfo = aac->tool;
  winInfo = aac->winInfo;

  uint16 tmp = (uint16) BsGetBits(bs, CPE_START_BITS);
  
  common_window = (int16) (tmp & 0x1);
  
  ch = ChIndex(2, (int16) (tmp >> 1), common_window, mip);
  
  if(common_window)
  {
    win = winInfo[ch];
    
    if(!GetICSInfo(bs, win, toolInfo[ch]->ltp, toolInfo[ch + 1]->ltp))
      return (-1);
    
    win->hasmask = (uint8) getmask(bs, mip->sfbInfo->winmap[win->wnd], win->group, win->max_sfb, win->mask);
  }
  else
    winInfo[ch]->hasmask = winInfo[ch + 1]->hasmask = 0;

  tool = toolInfo + ch;
  cip = &mip->ch_info[ch];
  for(i = ch; i < ch + 2; i++, cip++, tool++)
  {
    CWindowInfo *wi = winInfo[i];

    win = winInfo[cip->widx];
    
    /*-- Global gain. --*/
    global_gain = (int16) BsGetBits(bs, LEN_SCL_PCM);

    /*-- Save global gain element and its position. --*/
    if(gainPos)
    {
      gains[i - ch] = (uint8) global_gain;
      gainPos[i - ch] = BsGetBitsRead(bs) - bufBitOffset - LEN_SCL_PCM;
      bufBitOffset = BsGetBitsRead(bs);
    }

    if(!common_window)
      if(!GetICSInfo(bs, win, (*tool)->ltp, NULL))
        return (-1);

    cip->info = mip->sfbInfo->winmap[win->wnd];

    if(!GetICS(bs, cip, win->group, win->max_sfb, wi->cb_map, 
               (*tool)->quant, global_gain, wi->sfac))
      return (-1);
  }
  
  return (0);
}

/**************************************************************************
  Title       : LTP_Decode

  Purpose     :    Decodes the bitstream elements for LTP tool.

  Usage       : LTP_Decode(bs, ltp_info, max_sfb)

  Input       : bs       - input bitstream
                max_sfb  - # scalefactor bands to be used for current frame

  Output      : ltp_info - LTP parameters for this channel

  Author(s)   : Juha Ojanpera
  *************************************************************************/

INLINE void
LTP_Decode(TBitStream *bs, CLTP_Info *ltp_info, int16 max_sfb)
{
  /*
  Purpose:    1 bits is used to indicate the presence of LTP. */
#define    LEN_LTP_DATA_PRESENT 1

/*
  Purpose:    # of bits used for the pitch lag. */
#define    LEN_LTP_LAG 11

/*
  Purpose:    # of bits used for the gain value. */
#define    LEN_LTP_COEF 3

  ltp_info->ltp_present = (uint8) BsGetBits(bs, LEN_LTP_DATA_PRESENT);
  if(ltp_info->ltp_present)
  {
    uint32 bits = BsGetBits(bs, LEN_LTP_LAG + LEN_LTP_COEF);

    /*-- LTP lag. --*/
    ltp_info->delay[0] = (int16) (bits >> LEN_LTP_COEF);
    
    /*-- LTP gain. --*/
    ltp_info->cbIdx = (uint8) (bits & 0x7);

    /*-- Sfb flags. --*/
    ltp_info->max_sfb = max_sfb;
    if(max_sfb < 33)
      ltp_info->sfbflags[0] = BsGetBits(bs, max_sfb);
    else
    {
      ltp_info->sfbflags[0] = BsGetBits(bs, 32);
      ltp_info->sfbflags[1] = BsGetBits(bs, (int16) (max_sfb - 32));
    }
  }
}

/**************************************************************************
  Title:        GetICSInfo

  Purpose:      Reads side information for individual channel element.
                Individual channel elements within channel pair element may 
                share this information.

  Usage:        y = GetICSInfo(bs, winInfo, ltp_left, ltp_right)

  Input:        bs         - bitstream parameters

  Output:       y          - # of sfb's present in this channel
                winInfo :
                 wnd       - window type for this channel
                 wshape    - window shape for this channel
                 group     - grouping of short blocks
                 lpflag    - BWAP prediction flags for each sfb
                 prstflag  - reset flags for BWAP
                ltp_left   - LTP parameters for left channel of the audio element
                ltp_right  - LTP parameters for right channel of the audio element

  Author(s):    Juha Ojanpera
  *************************************************************************/

int16
GetICSInfo(TBitStream *bs, CWindowInfo *winInfo, CLTP_Info *ltp_left, CLTP_Info *ltp_right)
{
#define INFO_BITS (LEN_ICS_RESERV + LEN_WIN_SEQ + LEN_WIN_SH + LEN_MAX_SFBL + LEN_PRED_PRES)
  int16 i, j;
  uint32 tmp;

  tmp =  BsGetBits(bs, INFO_BITS);
  winInfo->wnd = (uint8) ((tmp >> 8) & 0x3);
  winInfo->wshape[0].this_bk = (uint8) ((tmp >> 7) & 0x1);

  /*-- Max scalefactor, scalefactor grouping and prediction flags. --*/
  winInfo->prstflag[0] = 0;
  if(winInfo->wnd == 2)
  {
    tmp <<= 4;
    tmp |= BsGetBits(bs, 4);
    winInfo->max_sfb = (uint8) ((tmp >> 7) & 0xF);
    if(winInfo->max_sfb > MAXLONGSFBBANDS) return (FALSE);

    /*-- Get grouping info for short blocks. --*/
    j = 0;
    if(!(tmp & 64)) winInfo->group[j++] = 1;
    if(!(tmp & 32)) winInfo->group[j++] = 2;
    if(!(tmp & 16)) winInfo->group[j++] = 3;
    if(!(tmp & 8))  winInfo->group[j++] = 4;
    if(!(tmp & 4))  winInfo->group[j++] = 5;
    if(!(tmp & 2))  winInfo->group[j++] = 6;
    if(!(tmp & 1))  winInfo->group[j++] = 7;
    winInfo->group[j] = NSHORT;

    /*-- Prediction (BWAP) is disabled. --*/
    winInfo->lpflag[0] = 0;
  }
  else
  {    
    winInfo->group[0] = 1;
    winInfo->max_sfb = (uint8) ((tmp >> 1) & 0x3F);
    if(winInfo->max_sfb > MAXLONGSFBBANDS) return (FALSE);

    if(winInfo->predType == BWAP_PRED)
    {
      /*-- Read BWAP predictor parameters. --*/
      winInfo->lpflag[0] = (uint8) (tmp & 0x1);
      if(winInfo->lpflag[0])
      {
        /*-- Predictor reset pattern. --*/
        winInfo->prstflag[0] = (uint8) BsGetBits(bs, LEN_PRED_RST);
        if(winInfo->prstflag[0])
          winInfo->prstflag[1] = (uint8) BsGetBits(bs, LEN_PRED_RSTGRP);
    
        /*-- Sfb flags for each predictor band. --*/
        j = (winInfo->max_sfb < winInfo->predBands) ? winInfo->max_sfb : winInfo->predBands;
    
        for(i = 1; i < j + 1; i++)
          winInfo->lpflag[i] = (uint8) BsGetBits(bs, LEN_PRED_ENAB);

        for(; i < winInfo->predBands + 1; i++)
          winInfo->lpflag[i] = 0;
      }
    }

    else if(winInfo->predType == LTP_PRED)
    {
      /*-- Is LTP used in this channel ? --*/
      ltp_left->ltp_present = (uint8) (tmp & 1);
      if(ltp_left->ltp_present)
      {
        int16 nbands = MIN(winInfo->max_sfb, winInfo->predBands);
        LTP_Decode(bs, ltp_left, nbands);
        if(ltp_right)
          LTP_Decode(bs, ltp_right, nbands);
      }
      else if(ltp_right) ltp_right->ltp_present = 0;
    }
    
    /*-- Bitstream syntax error. --*/
    else if(tmp & 0x1) return (FALSE);
  }

  return (TRUE);
}

/*
 * Retrieves 'global_gain' bitstream elements from single channel element.
 */
int16
GetSCEGain(CAACAudDec *aac, TBitStream *bs, uint8 *gains, uint32 *gainPos, uint32 bufBitOffset)
{ 
  return GetSCE(aac, bs, aac->mc_info, gains, gainPos, bufBitOffset);
}

/*
 * Retrieves 'global_gain' bitstream elements from channel pair element.
 */
int16 
GetCPEGain(CAACAudDec *aac, TBitStream *bs, uint8 *gains, uint32 *gainPos, uint32 bufBitOffset)
{
  return GetCPE(aac, bs, aac->mc_info, gains, gainPos, bufBitOffset);
}
