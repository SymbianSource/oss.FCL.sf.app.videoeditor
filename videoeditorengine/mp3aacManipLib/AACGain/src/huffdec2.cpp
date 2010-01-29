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
  huffdec2.cpp - Bit parsing routines for decoding individual channel elements.
 
  Author(s): Juha Ojanpera
  Copyright (c) 2000-2004 by Nokia Research Center, Speech and Audio Systems.
  *************************************************************************/

/*-- Project Headers. --*/
#include "tool2.h"

/**************************************************************************
  Title:        calc_gsfb_table

  Purpose:      Calculates reordering parameters for short blocks.

  Usage:        calc_gsfb_table(info, group)

  Input:        info  - block (long/short) parameters
                group - grouping info for short blocks

  Author(s):    Juha Ojanpera
  *************************************************************************/

INLINE void
calc_gsfb_table(CInfo *info, uint8 *group)
{
  int16 group_offset, group_idx, offset;
  int16 sfb, len, *group_offset_p;

  /*-- First calc the group length. --*/
  group_offset = 0;
  group_idx = 0;
  do
  {
    info->group_len[group_idx] = (int16) (group[group_idx] - group_offset);
    group_offset = group[group_idx];
    group_idx++;
  }
  while(group_offset < NSHORT);
  
  info->num_groups = group_idx;
  group_offset_p = info->bk_sfb_top;
  offset = 0;
  for(group_idx = 0; group_idx < info->num_groups; group_idx++)
  {
    len = info->group_len[group_idx];
    for(sfb = 0; sfb < info->sfb_per_sbk[group_idx]; sfb++)
    {
      offset += info->sfb_width_128[sfb] * len;
      *group_offset_p++ = offset;
    }
  }
}

/**************************************************************************
  Title:        get_pulse_nc

  Purpose:      Reads pulse data from the bitstream.

  Usage:        get_pulse_nc(bs, pulse_info)

  Input:        bs - bitstream parameters

  Output:       pulse_info - pulse data parameters for this channel

  Author(s):    Juha Ojanpera
  *************************************************************************/

INLINE void
get_pulse_nc(TBitStream *bs, PulseInfo *pulse_info)
{
  int16 i, mask;

  mask = (1 << LEN_PULSE_PAMP) - 1;
  pulse_info->number_pulse = (int16) BsGetBits(bs, LEN_PULSE_NPULSE);
  pulse_info->pulse_start_sfb = (int16) BsGetBits(bs, LEN_PULSE_ST_SFB);

  for(i = 0; i < pulse_info->number_pulse + 1; i++)
  {
    uint32 codeword;

    codeword = BsGetBits(bs, LEN_PULSE_POFF + LEN_PULSE_PAMP);

    pulse_info->pulse_offset[i] = (int16) (codeword >> LEN_PULSE_PAMP);
    pulse_info->pulse_amp[i] = (int16) (codeword & mask);
  }
}

/**************************************************************************
  Title:        huf_cb

  Purpose:      Reads the Huffman codebook number and boundaries for each section.

  Usage:        y = huf_cb(bs, sect, info, tot_sfb, max_sfb, cb_map)

  Input:        bs      - bitstream parameters
                info    - block (long/short) parameters
                tot_sfb - total # of sfb's present
                max_sfb - max # of sfb's per block present

  Output:       y      - # of sections
                sect   - sectioning info (cb, cb_length; cb, cb_length; ...)
                cb_map - Huffman codebook number for each sfb

  Author(s):    Juha Ojanpera
  *************************************************************************/

INLINE int16
huf_cb(TBitStream *bs, uint8 *sect, CInfo *info, int16 tot_sfb, uint8 max_sfb, uint8 *cb_map)
{
  int16 nsect, n, base, bits, len, cb, diff, cb_len, bot;

  bits = (info->islong) ? (int16) LONG_SECT_BITS : (int16) SHORT_SECT_BITS;

  nsect = 0;
  len = (int16) ((1 << bits) - 1);
  diff = info->sfb_per_sbk[0] - max_sfb;

  for(bot = base = 0; base < tot_sfb && nsect < tot_sfb; )
  {
    *sect++ = uint8(cb = BsGetBits(bs, LEN_CB));

    n = BsGetBits(bs, bits);
    while(n == len && base < tot_sfb)
    {
      base += len;
      n = BsGetBits(bs, bits);
    }
    base += n; 
    *sect++ = uint8(base); 
    nsect++;

    cb_len = base - bot; SET_MEMORY(cb_map, cb_len, cb); cb_map += cb_len;
    
    if(base == max_sfb)
    {
      base += diff;
      if(info->islong) break;

      if(diff)
      {
        *sect++ = 0; *sect++ = uint8(base);
        ZERO_MEMORY(cb_map, diff); cb_map += diff;
        nsect++;
      }
      max_sfb += info->sfb_per_sbk[0];
    }
    bot = base;
  }
  
  if(base != tot_sfb || nsect > tot_sfb)
    nsect = -1;
  
  return (nsect);
}

/**************************************************************************
  Title:        TNS_Decode

  Purpose:      Reads TNS bitstream elements.

  Usage:        TNS_Decode(bs, info)

  Input:        bs   - bitstream parameters
                info - block (long/short) parameters

  Author(s):    Juha Ojanpera
  *************************************************************************/

INLINE void
TNS_Decode(TBitStream *bs, CInfo *info)
{
  int16 f, s, res, res2, nBlockBits, nOrderBits, nOrderBitsMask;

  nBlockBits = (!info->islong) ? (int16) 4 : (int16) 6;
  nOrderBits = (!info->islong) ? (int16) 3 : (int16) 5;
  nOrderBitsMask = (int16) ((1 << nOrderBits) - 1);

  for(s = 0; s < info->nsbk; s++)
  {
    int16 nTnsFilt;
    
    nTnsFilt = (int16) BsGetBits(bs, (!info->islong) ? (int16) 1 : (int16) 2);
    if(!nTnsFilt)
      continue;
    
    res = (int16) (BsGetBits(bs, 1) + 3);
    for(f = nTnsFilt; f > 0; f--)
    {
      int16 t, order;

      t = (int16) BsGetBits(bs, (int16) (nBlockBits + nOrderBits));
      order = (int16) (t & nOrderBitsMask);
      
      if(order)
      {
        t = (int16) BsGetBits(bs, 2);
        res2 = (int16) (res - (t & 0x1));
        
        BsSkipNBits(bs, res2 * order);
      }
    }
  }
}

/**************************************************************************
  Title:        GetICS

  Purpose:      Decodes individual channel elements.

  Usage:        y = GetICS(bs, cip, group, max_sfb, cb_map, coef, gain, factors)

  Input:        bs           - bitstream parameters
                cip :
                 info        - block (long/short) parameters
                group        - grouping info for short blocks
                max_sfb      - max # of sfb's present
                gain         - start value for scalefactor decoding

  Output:       y            - TRUE on success, FALSE otherwise
                cb_map       - Huffman codebook number for each sfb
                factors      - scalefactors for this channel
                cip :
                 is_present  - TRUE if IS is used, FALSE otherwise
                 tns_present - TRUE if TNS is used, FALSE otherwise

  Author(s):    Juha Ojanpera
*************************************************************************/

int16
GetICS(TBitStream *bs, TCh_Info *cip, uint8 *group, uint8 max_sfb, uint8 *cb_map, 
       int16 *quant, int16 gain, int16 *factors)
{
  CInfo *info;
  int16 nsect;
  PulseInfo pulse_info;
  uint8 sect[(MAXBANDS + 1) << 1];

  info = cip->info;
  
  /*-- Calculate total number of sfb for this grouping. --*/
  if(max_sfb == 0)
  {
    nsect = 0;
    ZERO_MEMORY(cb_map, MAXBANDS);
    ZERO_MEMORY(factors, MAXBANDS * sizeof(int16));
  }
  else
  {    
    int16 tot_sfb = info->sfb_per_sbk[0];
    if(!info->islong)
    {
      int16 i = 0;
      while(group[i++] < info->nsbk)
        tot_sfb += info->sfb_per_sbk[0];

      /*-- Calculate band offsets. --*/
      calc_gsfb_table(info, group);
    }

    /*-- Section data. --*/
    nsect = huf_cb(bs, sect, info, tot_sfb, max_sfb, cb_map);
    if(nsect < 0) return (FALSE);

    /*-- Scalefactor data. --*/
    if(!huf_sfac(bs, cip, group, cb_map, gain, factors, max_sfb))
      return (FALSE);
  }

  cip->tns_present = FALSE;
  pulse_info.pulse_data_present = FALSE;

  /*-- PULSE noiseless coding. --*/
  pulse_info.pulse_data_present = (BOOL) BsGetBits(bs, 1);
  if(pulse_info.pulse_data_present)
  {
    if(info->islong)
      get_pulse_nc(bs, &pulse_info);
    else
      return (FALSE);
  }

  /*-- TNS data. --*/
  if(BsGetBits(bs, 1))
  {
    cip->tns_present = TRUE;
    TNS_Decode(bs, info);
  }

  /*-- Gain control. --*/
  if(BsGetBits(bs, 1))
    return (FALSE);
    
  /*-- Quantized spectral data. --*/
  if(!(!nsect && max_sfb))
    cip->num_bins = huf_spec(bs, info, nsect, sect, quant, cip->huf, 1);
  else 
    return (FALSE);
  
  return (TRUE);
}
