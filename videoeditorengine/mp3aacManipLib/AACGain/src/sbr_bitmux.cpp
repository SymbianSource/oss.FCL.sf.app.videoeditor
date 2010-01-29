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
  \brief SBR bitstream multiplexer implementation $Revision: 1.1.1.1 $
*/

/**************************************************************************
  sbr_bitmux.cpp - Bitstream implementations for SBR encoder.
 
  Author(s): Juha Ojanpera
  Copyright (c) 2004 by Nokia Research Center, Multimedia Technologies.
  *************************************************************************/

/*-- Project Headers. --*/
#include "sbr_rom.h"
#include "sbr_bitmux.h"

#define CODE_BOOK_SCF_LAV10         (60)
#define CODE_BOOK_SCF_LAV11         (31)
#define CODE_BOOK_SCF_LAV_BALANCE11 (12)
#define CODE_BOOK_SCF_LAV_BALANCE10 (24)

/**
 * Writes SBR header data.
 */
int16
SBR_WriteHeaderData(SbrHeaderData *h_sbr_header, TBitStream *bs, uint8 writeFlag)
{
  int16 bitsWritten;
  uint8 headerExtra1, headerExtra2;

  bitsWritten = 1 + 4 + 4 + 3 + 2 + 2;

  if(writeFlag)
  {
    BsPutBits(bs, 1, h_sbr_header->ampResolution);
    BsPutBits(bs, 4, h_sbr_header->startFreq);
    BsPutBits(bs, 4, h_sbr_header->stopFreq); 
    BsPutBits(bs, 3, h_sbr_header->xover_band);
    BsPutBits(bs, 2, 0);
  }

  headerExtra1 = 0;
  if(h_sbr_header->freqScale   != SBR_FREQ_SCALE_DEF ||
     h_sbr_header->alterScale  != SBR_ALTER_SCALE_DEF ||
     h_sbr_header->noise_bands != SBR_NOISE_BANDS_DEF)
   headerExtra1 = 1;

  headerExtra2 = 0;
  if(h_sbr_header->limiterBands    != SBR_LIMITER_BANDS_DEF ||
     h_sbr_header->limiterGains    != SBR_LIMITER_GAINS_DEF ||
     h_sbr_header->interpolFreq    != SBR_INTERPOL_FREQ_DEF ||
     h_sbr_header->smoothingLength != SBR_SMOOTHING_LENGTH_DEF)
   headerExtra2 = 1;

  if(writeFlag)
  {
    BsPutBits(bs, 1, headerExtra1);
    BsPutBits(bs, 1, headerExtra2);
  }

  if(headerExtra1)
  {
    bitsWritten += 5;

    if(writeFlag)
    {
      BsPutBits(bs, 2, h_sbr_header->freqScale);
      BsPutBits(bs, 1, h_sbr_header->alterScale);
      BsPutBits(bs, 2, h_sbr_header->noise_bands);
    }
  }

  if(headerExtra2)
  {
    bitsWritten += 6;

    if(writeFlag)
    {
      BsPutBits(bs, 2, h_sbr_header->limiterBands);
      BsPutBits(bs, 2, h_sbr_header->limiterGains);
      BsPutBits(bs, 1, h_sbr_header->interpolFreq);
      BsPutBits(bs, 1, h_sbr_header->smoothingLength);
    }
  }

  return (bitsWritten);
}

/**
 * Writes additional sinusoidal data for SBR.
 */
int16
SBR_WriteSinusoidalData(SbrFrameData *hFrameData, TBitStream *bs, int16 nSfb, uint8 writeFlag)
{
  int16 bitsWritten;

  bitsWritten = 1;
  if(writeFlag)
  {
    if(hFrameData->isSinesPresent)
      BsPutBits(bs, 1, 1);
    else
      BsPutBits(bs, 1, 0);
  }

  if(hFrameData->isSinesPresent)
  {
    bitsWritten += nSfb;

    if(writeFlag)
    {
      int16 wSfb;

      wSfb = (nSfb > 32) ? 32 : nSfb;

      BsPutBits(bs, wSfb, hFrameData->addHarmonics[0]);
      if(nSfb > 32)
      {
        wSfb = nSfb - 32;
        BsPutBits(bs, wSfb, hFrameData->addHarmonics[1]);
      }
    }
  }


  return (bitsWritten);
}

/**
 * Writes envelope direction coding data.
 */
int16
SBR_WriteDTDFData(SbrFrameData *frameData, TBitStream *bs, uint8 writeFlag)
{
  int16 bitsWritten;

  bitsWritten = frameData->frameInfo.nEnvelopes + frameData->frameInfo.nNoiseEnvelopes;

  if(writeFlag)
  {
    BsPutBits(bs, frameData->frameInfo.nEnvelopes, frameData->domain_vec);
    BsPutBits(bs, frameData->frameInfo.nNoiseEnvelopes, frameData->domain_vec_noise);
  }

  return (bitsWritten);
}

/**
 * Writes envelope data for SBR.
 */
int16
SBR_WriteEnvelopeData(SbrHeaderData *hHeaderData, SbrFrameData *frameData,
                      TBitStream *bs, uint8 writeFlag)
{
  const uint8 *hCbBits_f, *hCbBits_t;
  const int32 *hCb_f = 0, *hCb_t = 0;
  const uint16 *hCb_f0 = 0, *hCb_t0 = 0;
  uint8 no_band[MAX_ENVELOPES], hCbIndexOffset;
  int16 i, j, k, offset, start_bits, bitsWritten, start_bits_balance, ampRes;

  ampRes = hHeaderData->ampResolution;
  if(frameData->frameInfo.frameClass == FIXFIX && frameData->frameInfo.nEnvelopes == 1)
    ampRes = SBR_AMP_RES_1_5;

  if(ampRes == SBR_AMP_RES_3_0)
  {
    start_bits = 6;
    start_bits_balance = 5;
  }
  else
  {
    start_bits = 7;
    start_bits_balance = 6;
  }

  k = SBR_BIT_ARRAY_SIZE - frameData->frameInfo.nEnvelopes;
  for(i = 0; i < frameData->frameInfo.nEnvelopes; i++, k++)
  {
    uint8 resValue = (frameData->frameInfo.freqRes & bitArray[k]) ? 1 : 0;

    no_band[i] = hHeaderData->hFreqBandData->nSfb[resValue];
  }

  /*
   * Indexing of the codebooks needs some effort in order to same ROM memory...
   */
  if(frameData->coupling == COUPLING_BAL) 
  {
    if(ampRes == SBR_AMP_RES_1_5) 
    {
      hCb_f = bookSbrEnvBalanceC10F;
      hCbBits_f = bookSbrEnvBalanceL10F;

      hCb_t = bookSbrEnvBalanceC10T;
      hCbBits_t = bookSbrEnvBalanceL10T;

      hCbIndexOffset = CODE_BOOK_SCF_LAV_BALANCE10; 
    }
    else 
    {
      hCb_f0 = bookSbrEnvBalanceC11F;
      hCbBits_f = bookSbrEnvBalanceL11F;

      hCb_t0 = bookSbrEnvBalanceC11T;
      hCbBits_t = bookSbrEnvBalanceL11T;

      hCbIndexOffset = CODE_BOOK_SCF_LAV_BALANCE11;
    }
  }
  else 
  {
    if(ampRes == SBR_AMP_RES_1_5) 
    {
      hCb_f = v_Huff_envelopeLevelC10F;
      hCbBits_f = v_Huff_envelopeLevelL10F;

      hCb_t = v_Huff_envelopeLevelC10T;
      hCbBits_t = v_Huff_envelopeLevelL10T;

      hCbIndexOffset = CODE_BOOK_SCF_LAV10;
    }
    else 
    {
      hCb_f = v_Huff_envelopeLevelC11F;
      hCbBits_f = v_Huff_envelopeLevelL11F;

      hCb_t = v_Huff_envelopeLevelC11T;
      hCbBits_t = v_Huff_envelopeLevelL11T;

      hCbIndexOffset = CODE_BOOK_SCF_LAV11;
    }
  }

  k = SBR_BIT_ARRAY_SIZE - frameData->frameInfo.nEnvelopes;
  for(j = 0, offset = 0, bitsWritten = 0; j < frameData->frameInfo.nEnvelopes; j++, k++) 
  {
    int16 index;
    uint8 codValue;

    codValue = frameData->domain_vec & bitArray[k];

    if(!codValue) 
    {
      index = frameData->iEnvelope[offset];
      index = MAX(0, index);

      if(frameData->coupling == COUPLING_BAL) 
      {
        index = MIN(index, (1 << start_bits_balance) - 1);

        bitsWritten += start_bits_balance;
        if(writeFlag) BsPutBits(bs, start_bits_balance, index);
      }
      else 
      {
        index = MIN(index, (1 << start_bits) - 1);

        bitsWritten += start_bits;
        if(writeFlag) BsPutBits(bs, start_bits, index);
      }
    }

    for(i = (1 - ((codValue) ? 1 : 0)); i < no_band[j]; i++)
    {
      uint32 codeWord;

      index  = frameData->iEnvelope[offset + i];
      index  = (index < 0) ? MAX(index, -hCbIndexOffset) : MIN(index, hCbIndexOffset);
      index += hCbIndexOffset;

      if(!codValue)
      {
        bitsWritten += hCbBits_f[index];
        if(writeFlag) 
        {
          if(frameData->coupling == COUPLING_BAL && ampRes != SBR_AMP_RES_1_5)
            codeWord = hCb_f0[index];
          else
            codeWord = hCb_f[index];

          BsPutBits(bs, hCbBits_f[index], codeWord);
        }
      }
      else
      {
        bitsWritten += hCbBits_t[index];
        if(writeFlag) 
        {
          if(frameData->coupling == COUPLING_BAL && ampRes != SBR_AMP_RES_1_5)
            codeWord = hCb_t0[index];
          else
            codeWord = hCb_t[index];

          BsPutBits(bs, hCbBits_t[index], codeWord);
        }
      }
    }

    offset += no_band[j];
  }

  return (bitsWritten);
}

/**
 * Writes noise data for SBR.
 */
int16
SBR_WriteNoiseData(SbrHeaderData *hHeaderData, SbrFrameData *frameData,
                   TBitStream *bs, uint8 writeFlag)
{
  uint8 hCbIndexOffset;
  const int32 *hCb_f = 0;
  const uint16 *hCb_f0 = 0, *hCb_t = 0;
  const uint8 *hCbBits_f, *hCbBits_t;
  int16 i, k, noNoiseBands, bitsWritten;

  noNoiseBands = hHeaderData->hFreqBandData->nNfb;

  if(frameData->coupling == COUPLING_BAL) 
  {
    hCb_f0 = bookSbrEnvBalanceC11F;
    hCbBits_f = bookSbrEnvBalanceL11F;

    hCb_t = bookSbrNoiseBalanceC11T;
    hCbBits_t = bookSbrNoiseBalanceL11T;

    hCbIndexOffset = CODE_BOOK_SCF_LAV_BALANCE11;
  }
  else 
  {
    hCb_f = v_Huff_envelopeLevelC11F;
    hCbBits_f = v_Huff_envelopeLevelL11F;

    hCb_t = v_Huff_NoiseLevelC11T;
    hCbBits_t = v_Huff_NoiseLevelL11T;

    hCbIndexOffset = CODE_BOOK_SCF_LAV11;
  }

  k = SBR_BIT_ARRAY_SIZE - frameData->frameInfo.nNoiseEnvelopes;
  for(i = 0, bitsWritten = 0; i < frameData->frameInfo.nNoiseEnvelopes; i++, k++) 
  {
    int16 j, index;
    uint8 codValue;

    codValue = frameData->domain_vec_noise & bitArray[k];

    if(!codValue) 
    {      
      bitsWritten += 5;
      if(writeFlag) 
        BsPutBits(bs, 5, frameData->sbrNoiseFloorLevel[i * noNoiseBands]);

      for(j = 1; j < noNoiseBands; j++) 
      {
        index  = frameData->sbrNoiseFloorLevel[i * noNoiseBands + j];
        index  = (index < 0) ? MAX(index, -hCbIndexOffset) : MIN(index, hCbIndexOffset);
        index += hCbIndexOffset;

        bitsWritten += hCbBits_f[index];
        if(writeFlag) 
        {
          uint32 codeWord = (frameData->coupling == COUPLING_BAL) ? hCb_f0[index] : hCb_f[index];
          BsPutBits(bs, hCbBits_f[index], codeWord);
        }
      }
    }
    else 
    {
      for(j = 0; j < noNoiseBands; j++) 
      {
        index  = frameData->sbrNoiseFloorLevel[i * noNoiseBands + j];
        index  = (index < 0) ? MAX(index, -hCbIndexOffset) : MIN(index, hCbIndexOffset);
        index += hCbIndexOffset;

        bitsWritten += hCbBits_t[index];
        if(writeFlag) BsPutBits(bs, hCbBits_t[index], hCb_t[index]);
      }
    }
  }

  return (bitsWritten);
}

/**
 * Writes dummy (=silence) data for parametric stereo part of SBR.
 */
int16
SBR_WriteParametricStereoData(TBitStream *bs, uint8 writeFlag)
{
  int16 bitsWritten = 0;

  /*-- Write header flag. --*/
  bitsWritten += 1;
  if(writeFlag) BsPutBits(bs, 1, 1);

  /*-- Write 'enable_iid' element. --*/
  bitsWritten += 1;
  if(writeFlag) BsPutBits(bs, 1, 0);

  /*-- Write 'enable_icc' element. --*/
  bitsWritten += 1;
  if(writeFlag) BsPutBits(bs, 1, 0);

  /*-- Write 'enable_ext' element. --*/
  bitsWritten += 1;
  if(writeFlag) BsPutBits(bs, 1, 0);

  /*-- Write frame class. --*/
  bitsWritten += 1;
  if(writeFlag) BsPutBits(bs, 1, 0);

  /*-- Write envelope index. --*/
  bitsWritten += 2;
  if(writeFlag) BsPutBits(bs, 2, 0);

  return (bitsWritten);
}

/**
 * Writes extension data for SBR.
 */
int16 
SBR_WritetExtendedData(TBitStream *bs, SbrExtensionData *extData, uint8 writeFlag)
{
  uint8 writeExt;
  int16 bitsWritten = 0;

  writeExt = (extData->writePsData || extData->extensionDataPresent) ? 1 : 0;

  bitsWritten += 1;
  if(writeFlag) BsPutBits(bs, 1, writeExt);

  if(writeExt) 
  {
    int16 i, nBitsLeft, cnt, byteCount;

    byteCount = (extData->writePsData) ? 2 : extData->byteCount;
    cnt = (byteCount >= 15) ? 15 : byteCount;

    bitsWritten += 4;
    if(writeFlag) BsPutBits(bs, 4, cnt);

    if(cnt == 15)
    {
      int16 diff;

      diff = byteCount - 15;

      bitsWritten += 8;
      if(writeFlag) BsPutBits(bs, 8, diff);
    }

    nBitsLeft = byteCount << 3;

    while(nBitsLeft > 7) 
    {
      nBitsLeft -= 2;
      bitsWritten += 2;
      if(writeFlag) BsPutBits(bs, 2, (extData->writePsData) ? 2 : extData->extension_id);

      cnt = nBitsLeft >> 3;
      bitsWritten += cnt << 3;
      if(writeFlag)
      {
        if(extData->writePsData)
        {
          SBR_WriteParametricStereoData(bs, writeFlag);
          BsPutBits(bs, 1, 0); /*-- This will byte align the data. --*/
        }
        else
          for(i = 0; i < cnt; i++)
            BsPutBits(bs, 8, extData->extensioData[i]);
      }

      nBitsLeft -= cnt << 3;
    }

    bitsWritten += nBitsLeft;
    if(writeFlag) BsPutBits(bs, nBitsLeft, 0);
  }

  return (bitsWritten);
}

/**
 * Writes envelope coding grid for SBR.
 */
int16
SBR_WriteGridInfo(TBitStream *bs, SbrFrameData *frameData, uint8 writeFlag)
{
  FRAME_INFO *frameInfo;
  SbrGridInfo *sbrGridInfo;
  int16 pointer_bits, nEnv, bitsWritten, k;

  nEnv = 0;
  bitsWritten = 0;
  frameInfo = &frameData->frameInfo;
  sbrGridInfo = &frameData->sbrGridInfo;

  bitsWritten += 2;
  if(writeFlag) BsPutBits(bs, 2, frameInfo->frameClass);

  switch(frameInfo->frameClass) 
  {
    case FIXFIX:
      bitsWritten += 2;
      if(writeFlag) BsPutBits(bs, 2, sbrGridInfo->bs_num_env);

      bitsWritten += 1;
      if(writeFlag) 
        BsPutBits(bs, 1, frameInfo->freqRes & 0x1);
      break;
      
    case FIXVAR:
    case VARFIX:
      bitsWritten += 2;
      if(writeFlag) BsPutBits(bs, 2, sbrGridInfo->bs_var_board[0]);

      bitsWritten += 2;
      if(writeFlag) BsPutBits(bs, 2, sbrGridInfo->bs_num_env);

      bitsWritten += sbrGridInfo->bs_num_env << 1;
      if(writeFlag)
        for(k = 0; k < sbrGridInfo->bs_num_env; k++) 
          BsPutBits(bs, 2, sbrGridInfo->bs_rel_board_0[k]);

      pointer_bits = (int16) (FloatFR_logDualis(sbrGridInfo->bs_num_env + 2) + 0.992188f);

      bitsWritten += pointer_bits;
      if(writeFlag) BsPutBits(bs, pointer_bits, sbrGridInfo->bs_pointer);

      bitsWritten += sbrGridInfo->bs_num_env + 1;
      break;
  }

  switch(frameInfo->frameClass) 
  {
    case FIXVAR:
      if(writeFlag)
      {
        uint8 tmp = 0, tmp2 = frameInfo->freqRes;
        for(k = sbrGridInfo->bs_num_env; k >= 0; k--)
        {
          tmp <<= 1;
          tmp  |= tmp2 & 0x1;
          tmp2 >>= 1;
        }
        BsPutBits(bs, sbrGridInfo->bs_num_env + 1, tmp);
      }
      break;

    case VARFIX:
      if(writeFlag)
        BsPutBits(bs, sbrGridInfo->bs_num_env + 1, frameInfo->freqRes);
      break;

    case VARVAR:
      bitsWritten += 8;
      if(writeFlag)
      {
        BsPutBits(bs, 2, sbrGridInfo->bs_var_board[0]);
        BsPutBits(bs, 2, sbrGridInfo->bs_var_board[1]);
        BsPutBits(bs, 2, sbrGridInfo->bs_num_rel[0]);
        BsPutBits(bs, 2, sbrGridInfo->bs_num_rel[1]);
      }
      
      nEnv = sbrGridInfo->bs_num_rel[0] + sbrGridInfo->bs_num_rel[1] + 1;

      bitsWritten += sbrGridInfo->bs_num_rel[0] << 1;
      if(writeFlag)
        for(k = 0; k < sbrGridInfo->bs_num_rel[0]; k++)
          BsPutBits(bs, 2, sbrGridInfo->bs_rel_board_0[k]);

      bitsWritten += sbrGridInfo->bs_num_rel[1] << 1;
      if(writeFlag)
        for(k = 0; k < sbrGridInfo->bs_num_rel[1]; k++) 
          BsPutBits(bs, 2, sbrGridInfo->bs_rel_board_1[k]);

      pointer_bits = (int16) (FloatFR_logDualis(nEnv + 1) + 0.992188f);

      bitsWritten += pointer_bits;
      if(writeFlag) BsPutBits(bs, pointer_bits, sbrGridInfo->bs_pointer);

      bitsWritten += nEnv;
      if(writeFlag)
        BsPutBits(bs, nEnv, frameInfo->freqRes);
      break;
  }

  return (bitsWritten);
}

int16
SBR_WriteSCE(SbrHeaderData *hHeaderData, SbrFrameData  *hFrameData,
             SbrExtensionData *sbrExtData, TBitStream *bs,
             uint8 isMono, uint8 writeFlag)
{
  int16 bitsWritten;

  /*-- No reserved bits. --*/
  bitsWritten = 1;
  if(writeFlag) BsPutBits(bs, 1, hFrameData->dataPresent);

  bitsWritten += (hFrameData->dataPresent) ? 4 : 0;
  if(hFrameData->dataPresent)
    if(writeFlag) BsPutBits(bs, 4, 0);

  /*-- Write grid info. --*/
  bitsWritten += SBR_WriteGridInfo(bs, hFrameData, writeFlag);

  /*-- Write direction info for the envelope coding. --*/
  bitsWritten += SBR_WriteDTDFData(hFrameData, bs, writeFlag);

  /*-- Write inverse filtering modes. --*/
  bitsWritten += hHeaderData->hFreqBandData->nInvfBands << 1;
  if(writeFlag)
    BsPutBits(bs, hHeaderData->hFreqBandData->nInvfBands << 1, hFrameData->sbr_invf_mode);

  /*-- Write envelope values. --*/
  bitsWritten += SBR_WriteEnvelopeData(hHeaderData, hFrameData, bs, writeFlag);

  /*-- Write noise floor values. --*/
  bitsWritten += SBR_WriteNoiseData(hHeaderData, hFrameData, bs, writeFlag);

  /*-- Write additional sinusoidals, if any. --*/
  bitsWritten += SBR_WriteSinusoidalData(hFrameData, bs, hHeaderData->hFreqBandData->nSfb[1], writeFlag);

  /*
   * Write extended data info, this is only in case of mono since parameters
   * for parametric stereo are decoded via the data extension. In case of stereo
   * do nothing.
   */
  if(!isMono)
  {
    bitsWritten += 1;
    if(writeFlag) BsPutBits(bs, 1, 0);
  }
  else
    bitsWritten += SBR_WritetExtendedData(bs, sbrExtData, writeFlag);

  return (bitsWritten);
}

int16
SBR_WriteCPE(SbrHeaderData *hHeaderData, SbrFrameData *hFrameDataLeft,
             SbrFrameData *hFrameDataRight, SbrExtensionData *sbrExtData,
             TBitStream *bs, uint8 writeFlag)
{
  int16 bitsWritten;

  /*-- No reserved bits. --*/
  bitsWritten = 1;
  if(writeFlag) BsPutBits(bs, 1, 0);
  bitsWritten += (hFrameDataLeft->dataPresent) ? 8 : 0;
  if(hFrameDataLeft->dataPresent)
    if(writeFlag) BsPutBits(bs, 8, 0);

  bitsWritten += 1;
  if(writeFlag)
  {
    if(hFrameDataLeft->coupling) 
      BsPutBits(bs, 1, 1);
    else
      BsPutBits(bs, 1, 0);
  }

  /*-- Write grid info (left channel). --*/
  bitsWritten += SBR_WriteGridInfo(bs, hFrameDataLeft, writeFlag);

  /*-- Write grid info (right channel). --*/
  if(!hFrameDataLeft->coupling)
    bitsWritten += SBR_WriteGridInfo(bs, hFrameDataRight, writeFlag);

  /*-- Write direction info for the envelope coding (left channel). --*/
  bitsWritten += SBR_WriteDTDFData(hFrameDataLeft, bs, writeFlag);

  /*-- Write direction info for the envelope coding (right channel). --*/
  bitsWritten += SBR_WriteDTDFData(hFrameDataRight, bs, writeFlag);

  /*-- Write inverse filtering modes. --*/
  bitsWritten += hHeaderData->hFreqBandData->nInvfBands << 1;
  if(writeFlag)
    BsPutBits(bs, hHeaderData->hFreqBandData->nInvfBands << 1, hFrameDataLeft->sbr_invf_mode);

  if(hFrameDataLeft->coupling) 
  {
    /*-- Write envelope values. --*/
    bitsWritten += SBR_WriteEnvelopeData(hHeaderData, hFrameDataLeft, bs, writeFlag);

    /*-- Write noise floor values. --*/
    bitsWritten += SBR_WriteNoiseData(hHeaderData, hFrameDataLeft, bs, writeFlag);

    /*-- Write envelope values. --*/
    bitsWritten += SBR_WriteEnvelopeData(hHeaderData, hFrameDataRight, bs, writeFlag);
  }
  else 
  {
    bitsWritten += hHeaderData->hFreqBandData->nInvfBands << 1;
    if(writeFlag)
      BsPutBits(bs, hHeaderData->hFreqBandData->nInvfBands << 1, hFrameDataRight->sbr_invf_mode);

    /*-- Write envelope values. --*/
    bitsWritten += SBR_WriteEnvelopeData(hHeaderData, hFrameDataLeft, bs, writeFlag);

    /*-- Write envelope values. --*/
    bitsWritten += SBR_WriteEnvelopeData(hHeaderData, hFrameDataRight, bs, writeFlag);

    /*-- Write noise floor values. --*/
    bitsWritten += SBR_WriteNoiseData(hHeaderData, hFrameDataLeft, bs, writeFlag);
  }

  /*-- Write noise floor values. --*/
  bitsWritten += SBR_WriteNoiseData(hHeaderData, hFrameDataRight, bs, writeFlag);

  /*-- Write additional sinusoidals, if any. --*/
  bitsWritten += SBR_WriteSinusoidalData(hFrameDataLeft, bs, hHeaderData->hFreqBandData->nSfb[1], writeFlag);
  bitsWritten += SBR_WriteSinusoidalData(hFrameDataRight, bs, hHeaderData->hFreqBandData->nSfb[1], writeFlag);

  /*-- No extended data. --*/
#if 0
  bitsWritten += 1;
  if(writeFlag) BsPutBits(bs, 1, 0);
#else
  bitsWritten += SBR_WritetExtendedData(bs, sbrExtData, writeFlag);
#endif /*-- 0 --*/

  return (bitsWritten);
}
