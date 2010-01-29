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
  sfbdata.cpp - Sfb table definitions for AAC.
 
  Author(s): Juha Ojanpera
  Copyright (c) 2000-2004 by Nokia Research Center, Speech and Audio Systems.
  *************************************************************************/

/*-- Project Headers. --*/
#include "aacdef.h"

/*
   Purpose:     Sfb widths (long and short block) for each sampling rate.
   Explanation: Supports frame lengths 1024 and 960. */

const int16 sfb_96_1024[] = {
  4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 64, 72, 80, 88, 96, 
  108, 120, 132, 144, 156, 172, 188, 212, 240, 276, 320, 384, 448, 512, 576, 
  640, 704, 768, 832, 896, 960, 1024};

const int16 sfb_96_128[] = {4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 128};
const int16 sfb_96_120[] = {4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 120};

const int16 sfb_64_1024[] = {
  4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 64, 72, 80, 88, 100, 
  112, 124, 140, 156, 172, 192, 216, 240, 268, 304, 344, 384, 424, 464, 504, 
  544, 584, 624, 664, 704, 744, 784, 824, 864, 904, 944, 984, 1024};
const int16 sfb_64_960[] = { 
  4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 64, 72, 80, 88, 100, 
  112, 124, 140, 156, 172, 192, 216, 240, 268, 304, 344, 384, 424, 464, 504, 
  544, 584, 624, 664, 704, 744, 784, 824, 864, 904, 944, 960};

const int16 sfb_64_128[] = {4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 128};
const int16 sfb_64_120[] = {4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 120};

const int16 sfb_48_1024[] = {
  4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72, 80, 88, 96, 108, 120, 
  132, 144, 160, 176, 196, 216, 240, 264, 292, 320, 352, 384, 416, 448, 480, 
  512, 544, 576, 608, 640, 672, 704, 736, 768, 800, 832, 864, 896, 928, 1024};
const int16 sfb_48_960[] = { 
  4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72, 80, 88, 96, 108, 120, 
  132, 144, 160, 176, 196, 216, 240, 264, 292, 320, 352, 384, 416, 448, 480, 
  512, 544, 576, 608, 640, 672, 704, 736, 768, 800, 832, 864, 896, 928, 960, 0};

const int16 sfb_48_128[] = 
{4, 8, 12, 16, 20, 28, 36, 44, 56, 68, 80, 96, 112, 128};
const int16 sfb_48_120[] =
{4, 8, 12, 16, 20, 28, 36, 44, 56, 68, 80, 96, 112, 120};

const int16 sfb_32_1024[] = {
  4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72, 80, 88, 96, 108, 120, 
  132, 144, 160, 176, 196, 216, 240, 264, 292, 320, 352, 384, 416, 448, 480, 
  512, 544, 576, 608, 640, 672, 704, 736, 768, 800, 832, 864, 896, 928, 960,
  992, 1024};
const int16 sfb_32_960[] =
{ 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72, 80, 88, 96, 108, 120, 
  132, 144, 160, 176, 196, 216, 240, 264, 292, 320, 352, 384, 416, 448, 480, 
  512, 544, 576, 608, 640, 672, 704, 736, 768, 800, 832, 864, 896, 928, 960, 0};

const int16 sfb_24_1024[] = {
  4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 52, 60, 68, 76, 84, 92, 100, 108, 
  116, 124, 136, 148, 160, 172, 188, 204, 220, 240, 260, 284, 308, 336, 364, 
  396, 432, 468, 508, 552, 600, 652, 704, 768, 832, 896, 960, 1024};

const int16 sfb_24_128[] =
{ 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 64, 76, 92, 108, 128};
const int16 sfb_24_120[] =
{ 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 64, 76, 92, 108, 120};

const int16 sfb_16_1024[] = {
  8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 100, 112, 124, 136, 148, 160, 
  172, 184, 196, 212, 228, 244, 260, 280, 300, 320, 344, 368, 396, 424, 456, 
  492, 532, 572, 616, 664, 716, 772, 832, 896, 960, 1024};

const int16 sfb_16_128[] = 
{ 4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 60, 72, 88, 108, 128};
const int16 sfb_16_120[] =
{ 4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 60, 72, 88, 108, 120};

const int16 sfb_8_1024[] = {
  12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 156, 172, 188, 204, 220, 
  236, 252, 268, 288, 308, 328, 348, 372, 396, 420, 448, 476, 508, 544, 580, 
  620, 664, 712, 764, 820, 880, 944, 1024};
const int16 sfb_8_960[] =
{ 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 156, 172, 188, 204, 
  220, 236, 252, 268, 288, 308, 328, 348, 372, 396, 420, 448, 476, 508, 544, 
  580, 620, 664, 712, 764, 820, 880, 944, 960};

const int16 sfb_8_128[] = 
{ 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 60, 72, 88, 108, 128};
const int16 sfb_8_120[] =
{ 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 60, 72, 88, 108, 120};

/*
   Purpose:     Block parameters for each sampling rate.
   Explanation: - */
const SR_Info AAC_sampleRateInfo[(1 << LEN_SAMP_IDX)] = {

  /* 96000 */
  {96000, 41, sfb_96_1024, 12, sfb_96_128, 40, sfb_96_1024, 12, sfb_96_120},

  /* 88200 */
  {88200, 41, sfb_96_1024, 12, sfb_96_128, 40, sfb_96_1024, 12, sfb_96_120},

  /* 64000 */
  {64000, 47, sfb_64_1024, 12, sfb_64_128, 46, sfb_64_960,  12, sfb_64_120},

  /* 48000 */
  {48000, 49, sfb_48_1024, 14, sfb_48_128, 49, sfb_48_960,  14, sfb_48_120},

  /* 44100 */
  {44100, 49, sfb_48_1024, 14, sfb_48_128, 49, sfb_48_960,  14, sfb_48_120},

  /* 32000 */
  {32000, 51, sfb_32_1024, 14, sfb_48_128, 49, sfb_32_960,  14, sfb_48_120}, 

  /* 24000 */
  {24000, 47, sfb_24_1024, 15, sfb_24_128, 46, sfb_24_1024, 15, sfb_24_120},

  /* 22050 */
  {22050, 47, sfb_24_1024, 15, sfb_24_128, 46, sfb_24_1024, 15, sfb_24_120},

  /* 16000 */
  {16000, 43, sfb_16_1024, 15, sfb_16_128, 42, sfb_16_1024, 15, sfb_16_120},

  /* 12000 */
  {12000, 43, sfb_16_1024, 15, sfb_16_128, 42, sfb_16_1024, 15, sfb_16_120},

  /* 11025 */
  {11025, 43, sfb_16_1024, 15, sfb_16_128, 42, sfb_16_1024, 15, sfb_16_120},

  /* 8000  */
  { 8000, 40,  sfb_8_1024, 15, sfb_8_128,  40, sfb_8_960,   15, sfb_8_120},

  {0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0}};

/*
 * Retrieves AAC sample rate corresponding to the specified sample rate index.
 */
int32
AACSampleRate(uint8 sampleRateIdx)
{
  int32 sampleRate = 0;

  if(sampleRateIdx > 0 && sampleRateIdx < 12)
    sampleRate = AAC_sampleRateInfo[sampleRateIdx].samp_rate;

  return (sampleRate);
}

/*
 * Initializes SFB and sample rate tables for the specified AAC encoder.
 */
void
AACSfbInfoInit(CSfb_Info *sfb, uint8 sIndex, uint8 is960)
{
  CInfo *ip;
  const SR_Info *sip;
  const int16 *sfbands;
  int16 i, j, k, n, ws;

  sip = &AAC_sampleRateInfo[sIndex];

  /*-- Long block info. --*/
  ip = sfb->only_long_info;
  sfb->winmap[ONLY_LONG_WND] = ip;

  ip->nsbk = 1;
  ip->islong = 1;
  ip->bins_per_bk = (int16) ((is960) ? LN2_960 : LN2);
  for(i = 0; i < ip->nsbk; i++)
  {
    if(is960)
    {
      ip->sfb_per_sbk[i] = sip->nsfb960;
      ip->sbk_sfb_top[i] = sip->SFbands960;
    }
    else
    {
      ip->sfb_per_sbk[i] = sip->nsfb1024;
      ip->sbk_sfb_top[i] = sip->SFbands1024;
    }
  }
  ip->sfb_width_128 = NULL;
  ip->num_groups = 1;
  ip->group_len[0] = 1;
  ip->group_offs[0] = 0;

  /*-- Short block info. --*/
  ip = sfb->eight_short_info;
  sfb->winmap[ONLY_SHORT_WND] = ip;

  ip->islong = 0;
  ip->nsbk = NSHORT;
  ip->bins_per_bk = (int16) ((is960) ? LN2_960 : LN2);
  for(i = 0; i < ip->nsbk; i++)
  {
    if(is960)
    {
      ip->sfb_per_sbk[i] = sip->nsfb120;
      ip->sbk_sfb_top[i] = sip->SFbands120;
    }
    else
    {
      ip->sfb_per_sbk[i] = sip->nsfb128;
      ip->sbk_sfb_top[i] = sip->SFbands128;
    }
  }
  
  /*-- Construct sfb width table. --*/
  ip->sfb_width_128 = sfb->sfbwidth128;
  for(i = 0, j = 0, n = ip->sfb_per_sbk[0]; i < n; i++)
  {
    k = ip->sbk_sfb_top[0][i];
    ip->sfb_width_128[i] = (int16) (k - j);
    j = k;
  }

  /*-- Common to long and short. --*/
  for(ws = 0; ws < NUM_WIN_SEQ; ws++)
  {
    if(ws == 1 || ws == 3)
      continue;

    ip = sfb->winmap[ws];
    ip->sfb_per_bk = 0;
    for(i = 0, k= 0, n = 0; i < ip->nsbk; i++)
    {
      /*-- Compute bins_per_sbk. --*/
      ip->bins_per_sbk[i] = (int16) (ip->bins_per_bk / ip->nsbk);

      /*-- Compute sfb_per_bk. --*/
      ip->sfb_per_bk = (int16) (ip->sfb_per_bk + ip->sfb_per_sbk[i]);

      /*-- Construct default (non-interleaved) bk_sfb_top[]. --*/
      sfbands = ip->sbk_sfb_top[i];
      for(j = 0; j < ip->sfb_per_sbk[i]; j++)
        ip->bk_sfb_top[j + k] = (int16) (sfbands[j] + n);

      n = (int16) (n + ip->bins_per_sbk[i]);
      k = (int16) (k + ip->sfb_per_sbk[i]);
    }
  }

  sfb->winmap[1] = sfb->winmap[ONLY_LONG_WND];
  sfb->winmap[3] = sfb->winmap[ONLY_LONG_WND];
}
