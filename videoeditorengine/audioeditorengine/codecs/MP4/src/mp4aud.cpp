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
  mp4aud.cpp - MPEG-4 Audio interface to MPEG-4 Systems layer.

  Author(s): Juha Ojanpera
  Copyright (c) 2001-2004 by Nokia Research Center, Speech and Audio Systems.
  *************************************************************************/

/*-- Project Headers. --*/
#include "nok_bits.h"
#include "mp4aud.h"
#include "AACAPI.h"
const int32 sample_rates[] = {
  96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 
  11025, 8000, 0, 0, 0, 0
};

/*
 * Reads program configuration element from the specified bitstream.
 */
static INLINE int16
GetPCE(TBitStream *bs, ProgConfig *p, TInt *numChans)
{
  int16 i;

  p->tag = (int16) BsGetBits(bs, LEN_TAG);
  p->profile = (int16) BsGetBits(bs, LEN_PROFILE);
  p->sample_rate_idx = (int16) BsGetBits(bs, LEN_SAMP_IDX);
  p->front.num_ele = (int16) BsGetBits(bs, LEN_NUM_ELE);
  p->side.num_ele = (int16) BsGetBits(bs, LEN_NUM_ELE);
  p->back.num_ele = (int16) BsGetBits(bs, LEN_NUM_ELE);
  p->lfe.num_ele = (int16) BsGetBits(bs, LEN_NUM_LFE);
  p->data.num_ele = (int16) BsGetBits(bs, LEN_NUM_DAT);
  p->coupling.num_ele = (int16) BsGetBits(bs, LEN_NUM_CCE);

  p->mono_mix.present = (int16) BsGetBits(bs, 1);
  if(p->mono_mix.present == 1)
    p->mono_mix.ele_tag = (int16) BsGetBits(bs, LEN_TAG);

  p->stereo_mix.present = (int16) BsGetBits(bs, 1);
  if(p->stereo_mix.present == 1)
    p->stereo_mix.ele_tag = (int16) BsGetBits(bs, LEN_TAG);

  p->matrix_mix.present = (int16) BsGetBits(bs, 1);
  if(p->matrix_mix.present == 1)
  {
    p->matrix_mix.ele_tag = (int16) BsGetBits(bs, LEN_MMIX_IDX);
    p->matrix_mix.pseudo_enab = (int16) BsGetBits(bs, LEN_PSUR_ENAB);
  }

  for(i = 0; i < p->front.num_ele; i++)
  {
    p->front.ele_is_cpe[i] = (int16) BsGetBits(bs, LEN_ELE_IS_CPE);
    p->front.ele_tag[i] = (int16) BsGetBits(bs, LEN_TAG);

    *numChans += (p->front.ele_is_cpe[i] == 1) ? 2 : 1;
  }

  for(i = 0; i < p->side.num_ele; i++)
  {
    p->side.ele_is_cpe[i] = (int16) BsGetBits(bs, LEN_ELE_IS_CPE);
    p->side.ele_tag[i] = (int16) BsGetBits(bs, LEN_TAG);

    *numChans += (p->front.ele_is_cpe[i] == 1) ? 2 : 1;
  }

  for(i = 0; i < p->back.num_ele; i++)
  {
    p->back.ele_is_cpe[i] = (int16) BsGetBits(bs, LEN_ELE_IS_CPE);
    p->back.ele_tag[i] = (int16) BsGetBits(bs, LEN_TAG);

    *numChans += (p->front.ele_is_cpe[i] == 1) ? 2 : 1;
  }

  for(i = 0; i < p->lfe.num_ele; i++)
  {
    *numChans += 1;

    p->lfe.ele_is_cpe[i] = 0;
    p->lfe.ele_tag[i] = (int16) BsGetBits(bs, LEN_TAG);
  }

  for(i = 0; i < p->data.num_ele; i++)
  {
    p->data.ele_is_cpe[i] = 0;
    p->data.ele_tag[i] = (int16) BsGetBits(bs, LEN_TAG);
  }

  for(i = 0; i < p->coupling.num_ele; i++)
  {
    p->coupling.ele_is_cpe[i] = (int16) BsGetBits(bs, LEN_ELE_IS_CPE);
    p->coupling.ele_tag[i] = (int16) BsGetBits(bs, LEN_TAG);
  }

  BsByteAlign(bs);

  p->num_comment_bytes = (int16) BsGetBits(bs, LEN_COMMENT_BYTES);
  for(i = 0; i < p->num_comment_bytes; i++)
    p->comments[i] = (uint8) BsGetBits(bs, LEN_BYTE);
  p->comments[i] = 0;

  return (p->tag);
}

/*
 * Determines sampling rate index for the MPEG-4 decoder. The input sampling
 * rate as specified in the audio config part is not one of the rates listed
 * in the standard.
 */
static INLINE int16
DetermineSamplingFreqIdx(int32 samplingFreq)
{
  int16 idx;

  if(samplingFreq >= 92017L)                               idx = 0;
  else if(samplingFreq < 92017L && samplingFreq >= 75132L) idx = 1;
  else if(samplingFreq < 75132L && samplingFreq >= 55426)  idx = 2;
  else if(samplingFreq < 55426L && samplingFreq >= 46009L) idx = 3;
  else if(samplingFreq < 46009L && samplingFreq >= 37566L) idx = 4;
  else if(samplingFreq < 37566L && samplingFreq >= 27713L) idx = 5;
  else if(samplingFreq < 27713L && samplingFreq >= 23004L) idx = 6;
  else if(samplingFreq < 23004L && samplingFreq >= 18783L) idx = 7;
  else if(samplingFreq < 18783L && samplingFreq >= 13856L) idx = 8;
  else if(samplingFreq < 13856L && samplingFreq >= 11502L) idx = 9;
  else if(samplingFreq < 11502L && samplingFreq >= 9391L)  idx = 10;
  else                                                     idx = 11;

  return (idx);
}

/*
 * Reads GA decoder configuration information from the specified bitstream.
 */
static INLINE BOOL
ReadGASpecificInfo(TBitStream *bs, AudioSpecificInfo *audInfo, ProgConfig *progCfg)
{
  int16 extensionFlag;
  GaSpecificInfo *gaInfo = &audInfo->gaInfo;

  gaInfo->FrameLengthFlag = (BOOL) BsGetBits(bs, 1);
  gaInfo->DependsOnCoreCoder = (BOOL) BsGetBits(bs, 1);
  gaInfo->CoreCoderDelay = (gaInfo->DependsOnCoreCoder) ? (int16) BsGetBits(bs, 14) : (int16)0;
  extensionFlag = (int16) BsGetBits(bs, 1);

  if(audInfo->channelConfiguration == 0)
  {
    TInt numChans = 0;

    GetPCE(bs, progCfg, &numChans);
    audInfo->channelConfiguration = (uint8) numChans;
    if (audInfo->samplingFreqIdx != (uint8) progCfg->sample_rate_idx)
        {
        return FALSE;
        }
  }

  /*
   * Determine the sampling rate index so that sampling rate dependent
   * tables can be initialized.
   */
  if(audInfo->samplingFreqIdx == 0xf && audInfo->channelConfiguration != 0)
  {
    audInfo->samplingFreqIdx = (uint8) DetermineSamplingFreqIdx(audInfo->samplingFrequency);
    progCfg->sample_rate_idx = audInfo->samplingFreqIdx;
  }
  else
    {

    audInfo->samplingFreqIdx = (uint8) progCfg->sample_rate_idx;
  
    }

  if(audInfo->audioObject == AAC_SCALABLE || audInfo->audioObject == ER_AAC_SCALABLE)
    gaInfo->layerNr = (uint8) BsGetBits(bs, 3);
  
  if(extensionFlag)
  {
    if(audInfo->audioObject == ER_BSAC)
    {
      gaInfo->numOfSubframe = (uint8) BsGetBits(bs, 5);
      gaInfo->layer_length = (uint8) BsGetBits(bs, 11);
    }
    
    switch(audInfo->audioObject)
    {
      case ER_AAC_LC:
      case ER_AAC_LTP:
      case ER_AAC_SCALABLE:
      case ER_TWINVQ:
      case ER_AAC_LD:
        gaInfo->aacSectionDataResilienceFlag = (BOOL) BsGetBits(bs, 1);
        gaInfo->aacScalefactorDataResilienceFlag = (BOOL) BsGetBits(bs, 1);
        gaInfo->aacSpectralDataResilienceFlag = (BOOL) BsGetBits(bs, 1);
        break;

      default:
        break;
    }

    extensionFlag = (int16) BsGetBits(bs, 1);
    if(extensionFlag)
    {
      ;
    }
  }
  return TRUE;
}

/*
 * Reads MPEG-4 audio decoder specific configuration information.
 *
 * Retuns TRUE on success, FALSE otherwise. FALSE indicates that 
 * unsupported decoder configuration (e.g., object type) was detected.
 */
static INLINE BOOL
ReadAudioSpecificConfig(TBitStream *bs, AudioSpecificInfo *audInfo, 
            ProgConfig *progCfg, int32 bufLen)
{
  int16 bitsLeft;
  AudioObjectType audObj;

  /*-- Read common configuration information. --*/
  audObj = audInfo->audioObject = (AudioObjectType)(BsGetBits(bs, 5) - 1);
  audInfo->samplingFreqIdx = (uint8) BsGetBits(bs, 4);
  audInfo->samplingFrequency = sample_rates[audInfo->samplingFreqIdx];
  if(audInfo->samplingFreqIdx == 0xf)
    audInfo->samplingFrequency = (int32) BsGetBits(bs, 24);
  audInfo->channelConfiguration = (uint8) BsGetBits(bs, 4);

  audInfo->extAudioObject = (AudioObjectType) 0;
  audInfo->sbrPresentFlag = 0;

  if(audInfo->audioObject == AAC_SBR)
  {
    audInfo->extAudioObject = audInfo->audioObject;
    audInfo->sbrPresentFlag = 1;
    audInfo->extSamplingFreqIdx = (uint8) BsGetBits(bs, 4);
    if(audInfo->extSamplingFreqIdx == 0xf)
      audInfo->extSamplingFrequency = (int32) BsGetBits(bs, 24);
    audObj = audInfo->audioObject = (AudioObjectType)(BsGetBits(bs, 5) - 1);
  }

  /*-- Read object type specific configuration information. --*/
  switch(audInfo->audioObject)
  {
    case AAC_MAIN:
    case AAC_LC:
    case AAC_LTP:
      progCfg->profile = (int16) audObj;
      progCfg->sample_rate_idx = audInfo->samplingFreqIdx;
      if (ReadGASpecificInfo(bs, audInfo, progCfg) == FALSE)
        {
        return FALSE;
        }
      if(audInfo->samplingFreqIdx != 0xf)
        audInfo->samplingFrequency = sample_rates[audInfo->samplingFreqIdx];
      break;

    default:
      audObj = NULL_OBJECT;
      break;
  }

  bitsLeft = (int16) ((bufLen << 3) - BsGetBitsRead(bs));
  if(audInfo->extAudioObject != AAC_SBR && bitsLeft >= 16)
  {
    audInfo->syncExtensionType = (int16) BsGetBits(bs, 11);
    if(audInfo->syncExtensionType == 0x2b7)
    {
      audInfo->extAudioObject = (AudioObjectType)(BsGetBits(bs, 5) - 1);
      if(audInfo->extAudioObject == AAC_SBR)
      {
        audInfo->sbrPresentFlag = (uint8) BsGetBits(bs, 1);
        if(audInfo->sbrPresentFlag)
        {
          audInfo->extSamplingFreqIdx = (uint8) BsGetBits(bs, 4);
          if(audInfo->extSamplingFreqIdx == 0xf)
            audInfo->extSamplingFrequency = (int32) BsGetBits(bs, 24);
        }
      }
    }
  }

  return ((audObj == NULL_OBJECT) ? (BOOL) FALSE : (BOOL) TRUE);
}

BOOL
ReadMP4AudioConfig(uint8 *buffer, uint32 bufLen, mp4AACTransportHandle *mp4AAC_ff)
{
  TBitStream bs;
  BOOL retValue;

  BsInit(&bs, buffer, bufLen);

  retValue = ReadAudioSpecificConfig(&bs, &mp4AAC_ff->audioInfo, &mp4AAC_ff->progCfg, bufLen);

  return (retValue);
}

void
WriteMP4AudioConfig(uint8 *buffer, uint32 bufLen, mp4AACTransportHandle /* *mp4AAC_ff*/)
{

  TBitStream bs;

  BsInit(&bs, buffer, bufLen);

  /*-- Change object type to LC. --*/
  BsPutBits(&bs, 5, 2);

}

