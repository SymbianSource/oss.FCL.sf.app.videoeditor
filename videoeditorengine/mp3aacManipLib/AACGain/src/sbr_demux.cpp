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
  \brief SBR codec implementation $Revision: 1.1.1.1.4.1 $
*/

/**************************************************************************
  sbr_demux.cpp - SBR bitstream demultiplexer implementations.
 
  Author(s): Juha Ojanpera
  Copyright (c) 2004 by Nokia Research Center, Multimedia Technologies.
  *************************************************************************/

/*-- Project Headers. --*/
#include "sbr_rom.h"
#include "env_extr.h"

typedef const int8 (*Huffman)[2];

/*
  \brief   Decodes Huffman codeword from TBitStream

  \return  Decoded Huffman value.
*/
int16
DecodeHuffmanCW(Huffman h, TBitStream *bs)
{
  int16 value, bit, index = 0;

  while(index >= 0) 
  {
    bit = (int16) BsGetBits(bs, 1);
    index = h[index][bit];
  }

  value = index + 64;

  return (value);
}

/*
  \brief   Reads direction control data from TBitStream
*/
void
sbrGetDTDFData(SbrFrameData *frameData, TBitStream *bs)
{
  frameData->domain_vec = (uint8) BsGetBits(bs, frameData->frameInfo.nEnvelopes);
  frameData->domain_vec_noise = (uint8) BsGetBits(bs, frameData->frameInfo.nNoiseEnvelopes);
}

/*
  \brief   Reads noise-floor-level data from TBitStream
*/
void
sbrGetNoiseFloorData(SbrHeaderData *headerData, SbrFrameData *frameData, TBitStream *bs)
{
  int16 i, j, k, noNoiseBands;
  Huffman hcb_noiseF, hcb_noise;

  noNoiseBands = headerData->hFreqBandData->nNfb;

  if(frameData->coupling == COUPLING_BAL) 
  {
    hcb_noise = (Huffman) &sbr_huffBook_NoiseBalance11T;
    hcb_noiseF = (Huffman) &sbr_huffBook_EnvBalance11F;
  }
  else 
  {
    hcb_noise = (Huffman) &sbr_huffBook_NoiseLevel11T;
    hcb_noiseF = (Huffman) &sbr_huffBook_EnvLevel11F;
  }

  k = SBR_BIT_ARRAY_SIZE - frameData->frameInfo.nNoiseEnvelopes;
  for(i = 0; i < frameData->frameInfo.nNoiseEnvelopes; i++, k++) 
  {
    uint8 codValue;
    int16 index = i * noNoiseBands;

    codValue = frameData->domain_vec_noise & bitArray[k];

    if(!codValue) 
    {
      frameData->sbrNoiseFloorLevel[index++] = (int16) BsGetBits(bs, 5);

      for(j = 1; j < noNoiseBands; j++, index++) 
        frameData->sbrNoiseFloorLevel[index] = DecodeHuffmanCW(hcb_noiseF, bs);
    }
    else 
    {
      for(j = 0; j < noNoiseBands; j++, index++)
        frameData->sbrNoiseFloorLevel[index] = DecodeHuffmanCW(hcb_noise, bs);
    }
  }
}

/*
  \brief   Reads envelope data from TBitStream

  \return  One on success.
*/
int16
sbrGetEnvelope(SbrHeaderData *headerData, SbrFrameData *frameData, TBitStream *bs, int16 decVal)
{
  Huffman hcb_t, hcb_f;
  uint8 no_band[MAX_ENVELOPES];
  int16 i, j, k, delta, offset, ampRes;
  int16 start_bits, start_bits_balance;

  ampRes = headerData->ampResolution;
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

    no_band[i] = headerData->hFreqBandData->nSfb[resValue];
  }

  if(frameData->coupling == COUPLING_BAL) 
  {
    if(ampRes == SBR_AMP_RES_1_5) 
    {
      hcb_t = (Huffman) &sbr_huffBook_EnvBalance10T;
      hcb_f = (Huffman) &sbr_huffBook_EnvBalance10F;
    }
    else 
    {
      hcb_t = (Huffman) &sbr_huffBook_EnvBalance11T;
      hcb_f = (Huffman) &sbr_huffBook_EnvBalance11F;
    }
  }
  else 
  {
    if(ampRes == SBR_AMP_RES_1_5) 
    {
      hcb_t = (Huffman) &sbr_huffBook_EnvLevel10T;
      hcb_f = (Huffman) &sbr_huffBook_EnvLevel10F;
    }
    else 
    {
      hcb_t = (Huffman) &sbr_huffBook_EnvLevel11T;
      hcb_f = (Huffman) &sbr_huffBook_EnvLevel11F;
    }
  }

  decVal = (ampRes) ? decVal : decVal << 1;

  k = SBR_BIT_ARRAY_SIZE - frameData->frameInfo.nEnvelopes;
  for(j = 0, offset = 0; j < frameData->frameInfo.nEnvelopes; j++, k++) 
  {
    uint8 codValue = frameData->domain_vec & bitArray[k];

    if(!codValue) 
    {
      if(frameData->coupling == COUPLING_BAL)
        frameData->iEnvelope[offset] = (int16) BsGetBits(bs, start_bits_balance);
      else 
      {
        frameData->iEnvelope[offset] = (int16) BsGetBits(bs, start_bits);
        
        frameData->iEnvelope[offset] -= decVal;
        if(frameData->iEnvelope[offset] < 0)
          frameData->iEnvelope[offset] = 0;          
      }
    }

    for(i = (1 - ((codValue) ? 1 : 0)); i < no_band[j]; i++) 
    {
      if(!codValue) 
        delta = DecodeHuffmanCW(hcb_f, bs);
      else 
      {
        delta = DecodeHuffmanCW(hcb_t, bs);

        if(i == 0)
          delta -= decVal;
      }

      frameData->iEnvelope[offset + i] = delta;
    }

    offset += no_band[j];
  }

  return (1);
}

/*
  \brief   Extracts the frame information from the TBitStream

  \return  One on success
*/
int16
sbrReadGridInfo(TBitStream *bs, SbrHeaderData *headerData, SbrFrameData *frameData)
{
  uint8 tmp;
  FRAME_INFO *frameInfo;
  SbrGridInfo *sbrGridInfo;
  int16 pointer_bits, nEnv, k, staticFreqRes;

  nEnv = 0;
  frameInfo = &frameData->frameInfo;
  sbrGridInfo = &frameData->sbrGridInfo;

  frameInfo->frameClass = (uint8) BsGetBits(bs, 2);

  switch(frameInfo->frameClass) 
  {
    case FIXFIX:
      sbrGridInfo->bs_num_env = (uint8) BsGetBits(bs, 2);
      staticFreqRes = (int16) BsGetBits(bs, 1);
      nEnv = (int16) (1 << sbrGridInfo->bs_num_env);

      if(sbrGridInfo->bs_num_env < 3 && headerData->numberTimeSlots == 16)
        COPY_MEMORY(frameInfo, &(sbr_staticFrameInfo[sbrGridInfo->bs_num_env]), sizeof(FRAME_INFO));

      if(!staticFreqRes)
        frameInfo->freqRes = 0;
      break;
      
    case FIXVAR:
    case VARFIX:
      sbrGridInfo->bs_var_board[0] = (uint8) BsGetBits(bs, 2);
      sbrGridInfo->bs_num_env = (uint8) BsGetBits(bs, 2);
      nEnv = sbrGridInfo->bs_num_env + 1;

      for(k = 0; k < sbrGridInfo->bs_num_env; k++) 
        sbrGridInfo->bs_rel_board_0[k] = (uint8) BsGetBits(bs, 2);

      pointer_bits = (int16) (FloatFR_logDualis(sbrGridInfo->bs_num_env + 2) + 0.992188f);
      sbrGridInfo->bs_pointer = (uint8) BsGetBits(bs, pointer_bits);
      break;
  }

  switch(frameInfo->frameClass) 
  {
    case FIXVAR:
      frameInfo->freqRes = 0;
      tmp = (uint8) BsGetBits(bs, sbrGridInfo->bs_num_env + 1);
      for(k = sbrGridInfo->bs_num_env; k >= 0; k--)
      {
        frameInfo->freqRes <<= 1;
        frameInfo->freqRes  |= tmp & 0x1;
        tmp >>= 1;
      }
      break;

    case VARFIX:
      frameInfo->freqRes = (uint8) BsGetBits(bs, sbrGridInfo->bs_num_env + 1);
      break;

    case VARVAR:
      sbrGridInfo->bs_var_board[0] = (uint8) BsGetBits(bs, 2);
      sbrGridInfo->bs_var_board[1] = (uint8) BsGetBits(bs, 2);
      sbrGridInfo->bs_num_rel[0] = (uint8) BsGetBits(bs, 2);
      sbrGridInfo->bs_num_rel[1] = (uint8) BsGetBits(bs, 2);
      
      nEnv = sbrGridInfo->bs_num_rel[0] + sbrGridInfo->bs_num_rel[1] + 1;

      for(k = 0; k < sbrGridInfo->bs_num_rel[0]; k++)
        sbrGridInfo->bs_rel_board_0[k] = (uint8) BsGetBits(bs, 2);

      for(k = 0; k < sbrGridInfo->bs_num_rel[1]; k++) 
        sbrGridInfo->bs_rel_board_1[k] = (uint8) BsGetBits(bs, 2);

      pointer_bits = (int16) (FloatFR_logDualis(nEnv + 1) + 0.992188f);
      sbrGridInfo->bs_pointer = (int16) BsGetBits(bs, pointer_bits);

      frameInfo->freqRes = (uint8) BsGetBits(bs, nEnv);
      break;
  }

  frameInfo->nEnvelopes = (uint8) nEnv;
  frameInfo->nNoiseEnvelopes = (nEnv == 1) ? 1 : 2;

  return (1);
}

/*
  \brief     Initializes SBR header data
*/
void
initHeaderData(SbrHeaderData *headerData, FreqBandData *freqBandData,
               int32 sampleRate, int16 samplesPerFrame)
{
  FreqBandData *hFreq = freqBandData;

  COPY_MEMORY(headerData, &sbr_defaultHeader, sizeof(SbrHeaderData));

  headerData->hFreqBandData = hFreq;
  headerData->codecFrameSize = samplesPerFrame;
  headerData->outSampleRate = SBR_UPSAMPLE_FAC * sampleRate;
  headerData->numberTimeSlots = samplesPerFrame >> (4 + headerData->timeStep);
}

/*
  \brief   Reads header data from TBitStream

  \return  Processing status - HEADER_RESET or HEADER_OK
*/
SBR_HEADER_STATUS
sbrGetHeaderData(SbrHeaderData *h_sbr_header, TBitStream *bs)
{
  SbrHeaderData lastHeader;
  uint8 headerExtra1, headerExtra2;

  COPY_MEMORY(&lastHeader, h_sbr_header, sizeof(SbrHeaderData));

  h_sbr_header->ampResolution = (uint8) BsGetBits(bs, 1);
  h_sbr_header->startFreq = (uint8) BsGetBits(bs, 4);
  h_sbr_header->stopFreq = (uint8) BsGetBits(bs, 4);
  h_sbr_header->xover_band = (uint8) BsGetBits(bs, 3);

  BsGetBits(bs, 2);

  headerExtra1 = (uint8) BsGetBits(bs, 1);
  headerExtra2 = (uint8) BsGetBits(bs, 1);

  if(headerExtra1) 
  {
    h_sbr_header->freqScale = (uint8) BsGetBits(bs, 2);
    h_sbr_header->alterScale = (uint8) BsGetBits(bs, 1);
    h_sbr_header->noise_bands = (uint8) BsGetBits(bs, 2);
  }
  else 
  {
    h_sbr_header->freqScale   = SBR_FREQ_SCALE_DEF;
    h_sbr_header->alterScale  = SBR_ALTER_SCALE_DEF;
    h_sbr_header->noise_bands = SBR_NOISE_BANDS_DEF;
  }

  if(headerExtra2) 
  {
    h_sbr_header->limiterBands = (uint8) BsGetBits(bs, 2);
    h_sbr_header->limiterGains = (uint8) BsGetBits(bs, 2);
    h_sbr_header->interpolFreq = (uint8) BsGetBits(bs, 1);
    h_sbr_header->smoothingLength = (uint8) BsGetBits(bs, 1);
  }
  else 
  {
    h_sbr_header->limiterBands    = SBR_LIMITER_BANDS_DEF;
    h_sbr_header->limiterGains    = SBR_LIMITER_GAINS_DEF;
    h_sbr_header->interpolFreq    = SBR_INTERPOL_FREQ_DEF;
    h_sbr_header->smoothingLength = SBR_SMOOTHING_LENGTH_DEF;
  }

  if(h_sbr_header->syncState != SBR_ACTIVE                ||
     lastHeader.startFreq    != h_sbr_header->startFreq   ||
     lastHeader.stopFreq     != h_sbr_header->stopFreq    ||
     lastHeader.xover_band   != h_sbr_header->xover_band  ||
     lastHeader.freqScale    != h_sbr_header->freqScale   ||
     lastHeader.alterScale   != h_sbr_header->alterScale  ||
     lastHeader.noise_bands  != h_sbr_header->noise_bands) 
    return (HEADER_RESET); /*-- New settings --*/

  return (HEADER_OK);
}

/*
  \brief   Reads additional harmonics parameters

  \return  Number of bits read
*/
static int16
sbrGetSineData(SbrHeaderData *headerData, SbrFrameData *frameData, TBitStream *bs)
{
  int16 bitsRead;

  bitsRead = 1;
  frameData->isSinesPresent = (uint8) BsGetBits(bs, 1);

  if(frameData->isSinesPresent)
  {
    int16 rSfb;

    bitsRead += headerData->hFreqBandData->nSfb[1];

    rSfb = (headerData->hFreqBandData->nSfb[1] > 32) ? 32 : headerData->hFreqBandData->nSfb[1];

    frameData->addHarmonics[0] = (uint8) BsGetBits(bs, rSfb);

    if(headerData->hFreqBandData->nSfb[1] > 32)
    {
      rSfb = headerData->hFreqBandData->nSfb[1] - 32;
      frameData->addHarmonics[1] = (uint8) BsGetBits(bs, rSfb);
    }
  }

  return (bitsRead);
}

/*
  \brief      Reads extension data from the TBitStream
*/
static void 
sbrReadExtensionData(TBitStream *bs, SbrExtensionData *sbrExtData, uint8 isMono)
{
  sbrExtData->writePsData = 0;
  sbrExtData->extensionDataPresent = (uint8) BsGetBits(bs, 1);

  if(sbrExtData->extensionDataPresent) 
  {
    int16 i, nBitsLeft, cnt;

    cnt = (int16) BsGetBits(bs, 4);
    if(cnt == 15)
      cnt += (int16) BsGetBits(bs, 8);

    sbrExtData->byteCount = MIN(cnt, sbrExtData->extDataBufLen);

    nBitsLeft = cnt << 3;

    while(nBitsLeft > 7) 
    {
      sbrExtData->extension_id = (uint8) BsGetBits(bs, 2);

      if(!(sbrExtData->extension_id == SBR_PARAMETRIC_STEREO_ID && isMono))
        sbrExtData->extension_id = 0;
      else
        sbrExtData->writePsData = 1;

      nBitsLeft -= 2;

      cnt = (int16) ((uint16) nBitsLeft >> 3);

      if(sbrExtData->extDataBufLen)
      {
        for(i = 0; i < MIN(cnt, sbrExtData->extDataBufLen); i++)
          sbrExtData->extensioData[i] = (uint8) BsGetBits(bs, 8);

        for( ; i < cnt; i++)
          BsGetBits(bs, 8);
      }
      else
        for(i = 0; i < cnt; i++)
          BsGetBits(bs, 8);

      nBitsLeft -= cnt << 3;
    }

    BsGetBits(bs, nBitsLeft);
  }
}

/*
  \brief   Reads TBitStream elements of one channel

  \return  One on success
*/
int16
sbrGetSCE(SbrHeaderData *headerData, SbrFrameData *frameData,
          SbrExtensionData *sbrExtData, TBitStream *bs, 
          int16 decVal, uint8 isMono)
{
  frameData->coupling = COUPLING_OFF;

  /*-- Data present. --*/
  frameData->dataPresent = (int16) BsGetBits(bs, 1);
  if(frameData->dataPresent) BsGetBits(bs, 4);

  /*-- Read grid info. --*/
  sbrReadGridInfo(bs, headerData, frameData);

  /*-- Read direction info for envelope decoding. --*/
  sbrGetDTDFData(frameData, bs);

  /*-- Read inverse filtering modes. --*/
  frameData->sbr_invf_mode = (uint8) BsGetBits(bs, headerData->hFreqBandData->nInvfBands << 1);

  /*-- Read Huffman coded envelope values. --*/
  sbrGetEnvelope(headerData, frameData, bs, decVal);

  /*-- Read noise data. --*/
  sbrGetNoiseFloorData(headerData, frameData, bs);

  /*-- Read sine data, if any. --*/
  sbrGetSineData(headerData, frameData, bs);

  /*-- Read extension data, if any. --*/
  sbrReadExtensionData(bs, sbrExtData, isMono);

  return (1);
}

/*
  \brief      Reads TBitStream elements of a channel pair

  \return     One on success
*/
int16
sbrGetCPE(SbrHeaderData *headerData, SbrFrameData *frameDataLeft,
          SbrFrameData *frameDataRight, SbrExtensionData *sbrExtData,
          TBitStream *bs, int16 decVal)
{
  /*-- Data present. -*/
  frameDataLeft->dataPresent = (uint8) BsGetBits(bs, 1);
  if(frameDataLeft->dataPresent)
    BsGetBits(bs, 8);

  /*-- Coupling mode. --*/
  if(BsGetBits(bs, 1)) 
  {
    frameDataLeft->coupling = COUPLING_LEVEL;
    frameDataRight->coupling = COUPLING_BAL;
  }
  else 
  {
    frameDataLeft->coupling = COUPLING_OFF;
    frameDataRight->coupling = COUPLING_OFF;
  }

  /*-- Read grid info (left channel). --*/
  sbrReadGridInfo(bs, headerData, frameDataLeft);

  /*-- Read grid info (right channel). --*/
  if(frameDataLeft->coupling) 
    COPY_MEMORY(&frameDataRight->frameInfo, &frameDataLeft->frameInfo, sizeof(FRAME_INFO));
  else 
    sbrReadGridInfo(bs, headerData, frameDataRight);

  /*-- Read direction info for envelope decoding. --*/
  sbrGetDTDFData(frameDataLeft, bs);
  sbrGetDTDFData(frameDataRight, bs);

  /*-- Read inverse filtering modes for left channel. --*/
  frameDataLeft->sbr_invf_mode = (uint8) BsGetBits(bs, headerData->hFreqBandData->nInvfBands << 1);

  if(frameDataLeft->coupling) 
  {
    frameDataRight->sbr_invf_mode = frameDataLeft->sbr_invf_mode;

    /*-- Read Huffman coded envelope + noise values for left channel. --*/
    sbrGetEnvelope(headerData, frameDataLeft, bs, decVal);
    sbrGetNoiseFloorData(headerData, frameDataLeft, bs);

    /*-- Read Huffman coded envelope for right channel. --*/
    sbrGetEnvelope(headerData, frameDataRight, bs, decVal);
  }
  else 
  {
    /*-- Read inverse filtering modes for right channel. --*/
    frameDataRight->sbr_invf_mode = (uint8) BsGetBits(bs, headerData->hFreqBandData->nInvfBands << 1);
 
    /*-- Read Huffman coded envelope values. --*/
    sbrGetEnvelope(headerData, frameDataLeft, bs, decVal);
    sbrGetEnvelope(headerData, frameDataRight, bs, decVal);

    /*-- Read noise data for left channel. --*/
    sbrGetNoiseFloorData(headerData, frameDataLeft, bs);
  }

  /*-- Read noise data for right channel. --*/
  sbrGetNoiseFloorData(headerData, frameDataRight, bs);

  /*-- Read additional sines, if any. --*/
  sbrGetSineData(headerData, frameDataLeft, bs);
  sbrGetSineData(headerData, frameDataRight, bs);

  /*-- Read extension data, if any. --*/
  sbrReadExtensionData(bs, sbrExtData, 0);

  return (1);
}
