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


/*
  \file
  \brief  Frequency scale calculation $Revision: 1.2.4.1 $
*/

/**************************************************************************
  sbr_freq_sca.cpp - SBR frequency scale calculations.
 
  Author(s): Juha Ojanpera
  Copyright (c) 2004 by Nokia Research Center, Multimedia Technologies.
  *************************************************************************/

/*-- System Headers. --*/
#include <e32math.h>

/*-- Project Headers. --*/
#include "sbr_rom.h"

const int16 MAX_OCTAVE = 29;
const int16 MAX_SECOND_REGION = 50;
const FLOAT WARP_FACTOR = 25200.0f / 32768.0f;

/*!
  \brief   Sorting routine
*/
void 
shellsort(uint8 *in, uint8 n)
{
  int16 i, j, v, w, inc = 1;

  do
  {
    inc = 3 * inc + 1;

  } while (inc <= n);

  do 
  {
    inc = inc / 3;

    for(i = inc; i < n; i++) 
    {
      v = in[i];
      j = i;

      while((w = in[j - inc]) > v) 
      {
        in[j] = w;
        j -= inc;
        if(j < inc)
          break;
      }
      in[j] = v;
    }

  } while(inc > 1);
}

/*!
  \brief     Calculate number of SBR bands between start and stop band

  \return    number of bands
*/
static int16
numberOfBands(FLOAT bpo, int16 start, int16 stop, int16 warpFlag)
{
  int16 num_bands;
  FLOAT num_bands_div2;

  num_bands_div2 = 0.5f * FloatFR_getNumOctaves(start, stop) * bpo;
  if(warpFlag) num_bands_div2 *= WARP_FACTOR;

  num_bands_div2 += 0.5f;
  num_bands = (int16) num_bands_div2;
  num_bands <<= 1;

  return (num_bands);
}

/*!
  \brief     Calculate width of SBR bands

*/
static void
CalcBands(uint8 *diff, uint8 start, uint8 stop, uint8 num_bands)
{
  FLOAT exact, bandfactor;
  int16 i, previous, current;
 
  previous = start;
  exact = (FLOAT) start;

  Math::Pow(bandfactor, stop * sbr_invIntTable[start], sbr_invIntTable[num_bands]);

  for(i = 1; i <= num_bands; i++)  
  {
    exact *= bandfactor;
    current = (int16) (exact + 0.5f);
    diff[i - 1] = current - previous;
    previous = current;
  }
}

/*!
  \brief     Calculate cumulated sum vector from delta vector
*/
static void
cumSum(uint8 start_value, uint8* diff, uint8 length, uint8 *start_adress)
{
  int16 i;

  start_adress[0] = start_value;
  for(i = 1; i <= length; i++)
    start_adress[i] = start_adress[i - 1] + diff[i - 1];
}

/*!
  \brief     Adapt width of frequency bands in the second region
*/
static int16
modifyBands(uint8 max_band_previous, uint8 * diff, uint8 length)
{
  int16 change = max_band_previous - diff[0];

  if(change > (diff[length - 1] - diff[0]) / 2)
    change = (diff[length - 1] - diff[0]) / 2;

  diff[0] += change;
  diff[length - 1] -= change;
  shellsort(diff, length);

  return (0);
}

/*!
  \brief     Retrieve QMF-band where the SBR range starts

  \return  Number of start band
*/
static int16
getStartBand(int32 fs, uint8 startFreq)
{
  int16 band;

  switch(fs) 
  {
    case 96000:
    case 88200:
      band = sbr_start_freq_88[startFreq];
      break;

    case 64000:
      band = sbr_start_freq_64[startFreq];
      break;

    case 48000:
      band = sbr_start_freq_48[startFreq];
      break;

    case 44100:
      band = sbr_start_freq_44[startFreq];
      break;

    case 32000:
      band = sbr_start_freq_32[startFreq];
      break;

    case 24000:
      band = sbr_start_freq_24[startFreq];
      break;

    case 22050:
      band = sbr_start_freq_22[startFreq];
      break;

    case 16000:
      band = sbr_start_freq_16[startFreq];
      break;

    default:
      band = -1;
      break;
   }

  return (band);
}

/*!
  \brief   Generates master frequency tables

  \return  errorCode, 0 if successful
*/
int16
sbrdecUpdateFreqScale(uint8 * v_k_master, uint8 *numMaster, SbrHeaderData *hHeaderData)
{
  int32 fs;
  FLOAT bpo;
  int16 err, dk, k2_achived, k2_diff, incr;
  uint8 k0, k2, k1, i, num_bands0, num_bands1;
  uint8 diff_tot[MAX_OCTAVE + MAX_SECOND_REGION], *diff0, *diff1;

  diff0 = diff_tot;
  diff1 = diff_tot + MAX_OCTAVE;

  incr = k1 = dk = err = 0;
  fs = hHeaderData->outSampleRate;

  k0 = getStartBand(fs, hHeaderData->startFreq);

  if(hHeaderData->stopFreq < 14) 
  {
    switch(fs) 
    {
      case 48000:
        k1 = 21;
        break;
      
      case 44100:
        k1 = 23;
        break;
      
      case 32000:
      case 24000:
        k1 = 32;
        break;
      
      case 22050:
        k1 = 35;
        break;
      
      case 16000:
        k1 = 48;
        break;
      
      default:
        return (1);
    }

    CalcBands(diff0, k1, 64, 13);
    shellsort(diff0, 13);
    cumSum(k1, diff0, 13, diff1);
    k2 = diff1[hHeaderData->stopFreq];
  }
  else 
  {
    if(hHeaderData->stopFreq == 14) 
      k2 = 2 * k0;
    else
      k2 = 3 * k0;
  }

  if(k2 > NO_SYNTHESIS_CHANNELS)
    k2 = NO_SYNTHESIS_CHANNELS;

  if(((k2 - k0) > MAX_FREQ_COEFFS) || (k2 <= k0) ) 
    return (2);

  if(fs == 44100 && ((k2 - k0) > MAX_FREQ_COEFFS_FS44100)) 
    return (3);

  if(fs >= 48000 && ((k2 - k0) > MAX_FREQ_COEFFS_FS48000)) 
    return (4);

  if(hHeaderData->freqScale>0) 
  {
    if(hHeaderData->freqScale == 1)
      bpo = 12.0f;
    else 
    {
      if(hHeaderData->freqScale == 2)
        bpo = 10.0f;
      else
        bpo =  8.0f;
    }

    if(1000 * k2 > 2245 * k0) 
    {
      k1 = 2 * k0;
      num_bands0 = numberOfBands(bpo, k0, k1, 0);
      num_bands1 = numberOfBands(bpo, k1, k2, hHeaderData->alterScale);

      if(num_bands0 < 1)
        return (5);

      if(num_bands1 < 1)
        return (6);

      CalcBands(diff0, k0, k1, num_bands0);
      shellsort(diff0, num_bands0);

      if(diff0[0] == 0) 
        return (7);

      cumSum(k0, diff0, num_bands0, v_k_master);
      CalcBands(diff1, k1, k2, num_bands1);
      shellsort(diff1, num_bands1);

      if(diff0[num_bands0-1] > diff1[0]) 
      {
        err = modifyBands(diff0[num_bands0 - 1], diff1, num_bands1);
        if(err) return (8);
      }

      cumSum(k1, diff1, num_bands1, &v_k_master[num_bands0]);
      *numMaster = num_bands0 + num_bands1;
    }
    else 
    {
      k1 = k2;

      num_bands0 = numberOfBands(bpo, k0, k1, 0);
      if(num_bands0 < 1) 
        return (9);

      CalcBands(diff0, k0, k1, num_bands0);
      shellsort(diff0, num_bands0);
      if(diff0[0] == 0)
        return (10);

      cumSum(k0, diff0, num_bands0, v_k_master);
      *numMaster = num_bands0;
    }
  }
  else 
  {
    if(hHeaderData->alterScale == 0) 
    {
      dk = 1;
      num_bands0 = (k2 - k0) & 254;
    } 
    else 
    {
      dk = 2;
      num_bands0 = (((k2 - k0) >> 1) + 1) & 254;
    }
    
    if(num_bands0 < 1) 
      return (11);

    k2_achived = k0 + num_bands0 * dk;
    k2_diff = k2 - k2_achived;
    
    for(i = 0;i < num_bands0; i++)
      diff_tot[i] = dk;
    
    if(k2_diff < 0) 
    {
      incr = 1;
      i = 0;
    }

    if(k2_diff > 0) 
    {
      incr = -1;
      i = num_bands0 - 1;
    }
    
    while(k2_diff != 0) 
    {
      diff_tot[i] = diff_tot[i] - incr;
      i = i + incr;
      k2_diff = k2_diff + incr;
    }
    
    cumSum(k0, diff_tot, num_bands0, v_k_master);

    *numMaster = num_bands0;
  }

  if(*numMaster < 1) 
    return (12);

  return (0);
}

/*!
  \brief   Reset frequency band tables

  \return  error code, 0 on success
*/
int16
resetFreqBandTables(SbrHeaderData *hHeaderData)
{
  uint8 nBandsHi;
  int16 err, k2, kx, intTemp;
  FreqBandData *hFreq = hHeaderData->hFreqBandData;

  err = sbrdecUpdateFreqScale(hFreq->v_k_master, &hFreq->numMaster, hHeaderData);
  if(err) return (err);

  if(hHeaderData->xover_band > hFreq->numMaster)
    return (13);

  nBandsHi = hFreq->numMaster - hHeaderData->xover_band;

  hFreq->nSfb[0] = ((nBandsHi & 1) == 0) ? nBandsHi >> 1 : (nBandsHi + 1) >> 1;
  hFreq->nSfb[1] = nBandsHi;

  k2 = hFreq->v_k_master[nBandsHi - hHeaderData->xover_band];
  kx = hFreq->v_k_master[hHeaderData->xover_band];

  if(hHeaderData->noise_bands == 0)
    hFreq->nNfb = 1;
  else
  {
    FLOAT temp;

    temp = FloatFR_getNumOctaves(kx,k2);
    temp = temp * (FLOAT) hHeaderData->noise_bands;
    intTemp = (int16) (temp + 0.5f);

    if(intTemp == 0)
      intTemp = 1;

    hFreq->nNfb = intTemp;
  }

  hFreq->nInvfBands = hFreq->nNfb;

  if(hFreq->nNfb > MAX_NOISE_COEFFS)
    return (14);

  return (0);
}
