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
  sbr_codec.cpp - SBR codec implementation.
 
  Author(s): Juha Ojanpera
  Copyright (c) 2004 by Nokia Research Center, Multimedia Technologies.
  *************************************************************************/

/*-- Project Headers. --*/
#include "sbr_codec.h"
#include "env_extr.h"
#include "sbr_rom.h"
#include "sbr_bitmux.h"

struct SBR_Decoder_Instance
{
  SbrFrameData *frameData[2];
  SbrHeaderData *sbrHeader[2];
  FreqBandData *freqBandData[2];

  SbrExtensionData *sbrExtData;

};


FLOAT 
FloatFR_logDualis(int16 a) 
{ 
  return (logDualisTable[a]); 
}

FLOAT 
FloatFR_getNumOctaves(int16 a, int16 b) 
{
  return (FloatFR_logDualis(b) - FloatFR_logDualis(a));
}

int16
ReadSBRExtensionData(TBitStream *bs, SbrBitStream *streamSBR, 
                     int16 extension_type, int16 prev_element, 
                     int16 dataCount)
{
  int16 i, sbrPresent;

  sbrPresent = 0;
  if(!(prev_element == SBR_ID_SCE || prev_element == SBR_ID_CPE))
    return (sbrPresent);

  if(!(extension_type == SBR_EXTENSION || extension_type == SBR_EXTENSION_CRC))
    return (sbrPresent);

  if(dataCount < MAX_SBR_BYTES && streamSBR->NrElements < MAX_NR_ELEMENTS)
  {
    streamSBR->sbrElement[streamSBR->NrElements].Data[0] = (uint8) BsGetBits(bs, 4);

    for(i = 1; i < dataCount; i++)
      streamSBR->sbrElement[streamSBR->NrElements].Data[i] = (uint8) BsGetBits(bs, 8);

    streamSBR->sbrElement[streamSBR->NrElements].ExtensionType = extension_type;
    streamSBR->sbrElement[streamSBR->NrElements].Payload = dataCount;
    streamSBR->NrElements += 1;
    sbrPresent = 1;
  }

  return (sbrPresent);
}

SbrBitStream *
CloseSBRBitStream(SbrBitStream *Bitstr)
{
  if(Bitstr)
  {
    int16 i;

    for(i = 0; i < MAX_NR_ELEMENTS; i++)
    {
      if(Bitstr->sbrElement[i].Data != 0)
        delete[] Bitstr->sbrElement[i].Data;
      Bitstr->sbrElement[i].Data = NULL;
    }

    
    delete Bitstr;
    Bitstr = NULL;
  }

  return (NULL);
}

SbrBitStream *
OpenSBRBitStreamL(void)
{
  int16 i;
  SbrBitStream *Bitstr;

  /*-- Create SBR bitstream handle. --*/
  Bitstr = (SbrBitStream *) new (ELeave) SbrBitStream[1];
  CleanupStack::PushL(Bitstr);

  ZERO_MEMORY(Bitstr, sizeof(SbrBitStream));

  /*-- Create payload handle for each supported element. --*/
  for(i = 0; i < MAX_NR_ELEMENTS; i++)
  {
    Bitstr->sbrElement[i].Data = (uint8 *) new (ELeave) uint8[MAX_SBR_BYTES];
    CleanupStack::PushL(Bitstr->sbrElement[i].Data);

    ZERO_MEMORY(Bitstr->sbrElement[i].Data, MAX_SBR_BYTES);
  }

  CleanupStack::Pop(MAX_NR_ELEMENTS + 1); /*-- 'Bitstr->sbrElement[i].Data' + 'Bitstr' --*/

  return (Bitstr);
}

/*!
  \brief     Set up SBR decoder

  \return    Handle
*/
SBR_Decoder *
OpenSBRDecoderL(int32 sampleRate, int16 samplesPerFrame, uint8 isStereo, uint8 isDualMono)
{
  uint8 nPops;
  SBR_Decoder *sbrDecoder;

  /*-- Create main handle. --*/
  sbrDecoder = (SBR_Decoder *) new (ELeave) SBR_Decoder[1];
  CleanupStack::PushL(sbrDecoder); nPops = 1;

  ZERO_MEMORY(sbrDecoder, sizeof(SBR_Decoder));

  /*-- Create frame data for mono or left channel. --*/
  sbrDecoder->frameData[0] = (SbrFrameData *) new (ELeave) SbrFrameData[1];
  CleanupStack::PushL(sbrDecoder->frameData[0]); nPops++;

  ZERO_MEMORY(sbrDecoder->frameData[0], sizeof(SbrFrameData));

  /*-- Create frame data for right channel. --*/
  if(isStereo)
  {
    sbrDecoder->frameData[1] = (SbrFrameData *) new (ELeave) SbrFrameData[1];
    CleanupStack::PushL(sbrDecoder->frameData[1]); nPops++;

    ZERO_MEMORY(sbrDecoder->frameData[1], sizeof(SbrFrameData));
  }

  /*-- Create header data. --*/
  sbrDecoder->sbrHeader[0] = (SbrHeaderData *) new (ELeave) SbrHeaderData[1];
  CleanupStack::PushL(sbrDecoder->sbrHeader[0]); nPops++;

  ZERO_MEMORY(sbrDecoder->sbrHeader[0], sizeof(SbrHeaderData));

  /*
   * Create header data for dual channel if so needed. Remember that in
   * dual channel mode individual channels are not jointly coded,
   * each channel element is coded separately. Thus, also header data 
   * can change between frames of the individual channels.
   */
  if(isDualMono)
  {
    sbrDecoder->sbrHeader[1] = (SbrHeaderData *) new (ELeave) SbrHeaderData[1];
    CleanupStack::PushL(sbrDecoder->sbrHeader[1]); nPops++;

    ZERO_MEMORY(sbrDecoder->sbrHeader[1], sizeof(SbrHeaderData));
  }

  /*-- Create frequency band tables for mono or left channel. --*/
  sbrDecoder->freqBandData[0] = (FreqBandData *) new (ELeave) FreqBandData[1];
  CleanupStack::PushL(sbrDecoder->freqBandData[0]); nPops++;

  ZERO_MEMORY(sbrDecoder->freqBandData[0], sizeof(FreqBandData));

  /*-- Create frequency band data for dual channel. --*/
  if(isDualMono)
  {
    sbrDecoder->freqBandData[1] = (FreqBandData *) new (ELeave) FreqBandData[1];
    CleanupStack::PushL(sbrDecoder->freqBandData[1]); nPops++;

    ZERO_MEMORY(sbrDecoder->sbrHeader[1], sizeof(FreqBandData));
  }

  /*-- Create extension data handle. --*/
  sbrDecoder->sbrExtData = (SbrExtensionData *) new (ELeave) SbrExtensionData[1];
  CleanupStack::PushL(sbrDecoder->sbrExtData); nPops++;

  ZERO_MEMORY(sbrDecoder->sbrExtData, sizeof(SbrExtensionData));

  /*
   * Create data buffer for extension data. Data for parametric stereo 
   * will be stored here. Can exist only in mono mode.
   */
  if(!isStereo && !isDualMono)
  {
    sbrDecoder->sbrExtData->extDataBufLen = 128;
    sbrDecoder->sbrExtData->extensioData = (uint8 *) new (ELeave) uint8[sbrDecoder->sbrExtData->extDataBufLen];
    CleanupStack::PushL(sbrDecoder->sbrExtData->extensioData); nPops++;

    ZERO_MEMORY(sbrDecoder->sbrExtData->extensioData, sbrDecoder->sbrExtData->extDataBufLen);
  }

  /*-- Initialize header(s) with default values. --*/
  initHeaderData(sbrDecoder->sbrHeader[0], sbrDecoder->freqBandData[0], sampleRate, samplesPerFrame);
  if(isDualMono)
    initHeaderData(sbrDecoder->sbrHeader[1], sbrDecoder->freqBandData[1], sampleRate, samplesPerFrame);

  CleanupStack::Pop(nPops);

  return (sbrDecoder);
}

/*!
  \brief     Close SBR decoder resources

  \return    NULL
*/

SBR_Decoder *
CloseSBR(SBR_Decoder *sbrDecoder)
{
  if(sbrDecoder)
  {
    if(sbrDecoder->frameData[0] != 0)
      delete sbrDecoder->frameData[0];
    sbrDecoder->frameData[0] = NULL;

    if(sbrDecoder->frameData[1] != 0)
      delete sbrDecoder->frameData[1];
    sbrDecoder->frameData[1] = NULL;

    if(sbrDecoder->sbrHeader[0] != 0)
      delete sbrDecoder->sbrHeader[0];
    sbrDecoder->sbrHeader[0] = NULL;

    if(sbrDecoder->sbrHeader[1] != 0)
      delete sbrDecoder->sbrHeader[1];
    sbrDecoder->sbrHeader[1] = NULL;

    if(sbrDecoder->freqBandData[0] != 0)
      delete sbrDecoder->freqBandData[0];
    sbrDecoder->freqBandData[0] = NULL;

    if(sbrDecoder->freqBandData[1] != 0)
      delete sbrDecoder->freqBandData[1];
    sbrDecoder->freqBandData[1] = NULL;

    if(sbrDecoder->sbrExtData != 0)
    {      
      if(sbrDecoder->sbrExtData->extensioData != 0)
        delete[] sbrDecoder->sbrExtData->extensioData;
      sbrDecoder->sbrExtData->extensioData = NULL;
     
      delete sbrDecoder->sbrExtData;
    }
    sbrDecoder->sbrExtData = NULL;

    delete sbrDecoder;
    sbrDecoder = NULL;
  }

  return (NULL);
}

int32
SBR_WritePayload(TBitStream *bs, SbrFrameData *frameData[2], 
                 SbrHeaderData *hHeaderData, SbrExtensionData *sbrExtData,
                 uint8 headerStatus, uint8 isStereo, uint8 writeFlag)
{
  int32 bitsWritten;

  /*-- Write header flag. --*/
  bitsWritten = 1;
  if(writeFlag) BsPutBits(bs, 1, headerStatus);

  /*-- Write header data. --*/
  if(headerStatus)
    bitsWritten += SBR_WriteHeaderData(hHeaderData, bs, writeFlag);

  /*-- Write payload data. --*/
  if(hHeaderData->syncState == SBR_ACTIVE)
  {
    if(isStereo)
      bitsWritten += SBR_WriteCPE(hHeaderData, frameData[0], frameData[1], sbrExtData, bs, writeFlag);
    else
      bitsWritten += SBR_WriteSCE(hHeaderData, frameData[0], sbrExtData, bs, 1, writeFlag);
  }

  return (bitsWritten);
}

INLINE int32
WriteSBR(TBitStream *bs, SbrFrameData *frameData[2], SbrHeaderData *hHeaderData, 
         SbrExtensionData *sbrExtData, uint8 isStereo, uint8 headerPresent, 
         uint8 writeFlag)
{
  int32 bitsWritten;

  bitsWritten = 0;

  /*-- Write extension tag. --*/
  bitsWritten += 4;
  if(writeFlag) BsPutBits(bs, 4, SBR_EXTENSION);

  /*-- Write actual SBR payload. --*/
  bitsWritten += SBR_WritePayload(bs, frameData, hHeaderData, sbrExtData, headerPresent, isStereo, writeFlag);

  /*-- Byte align. --*/
  if(bitsWritten & 0x7)
  {
    uint8 bitsLeft = (uint8) (8 - (bitsWritten & 0x7));

    bitsWritten += bitsLeft;      
    if(writeFlag) BsPutBits(bs, bitsLeft, 0);
  }

  return (bitsWritten);
}

/*
 * Writes dummy payload data.
 */
INLINE int32 
WriteDummyPayload(TBitStream *bs, int32 nDummyBytes)
{
  int32 i, nBitsWritten;

  nBitsWritten = 4;
  BsPutBits(bs, 4, 0);

  nBitsWritten += 4;
  BsPutBits(bs, 4, 0);
  for(i = 0; i < (nDummyBytes - 1); i++)
  {
    nBitsWritten += 8;
    BsPutBits(bs, 8, 0xA5);
  }

  return (nBitsWritten);
}

/*
 * Writes the length of AAC Fill Element (FIL) as specified in the standard.
 */
INLINE int32 
WriteAACFilLength(TBitStream *bs, int32 nFilBytes)
{
  int32 cnt, nBitsWritten;

  nBitsWritten = 0;
  cnt = (nFilBytes >= 15) ? 15 : nFilBytes;

  nBitsWritten += 4;
  BsPutBits(bs, 4, cnt);
  if(cnt == 15)
  {
    int32 diff;

    diff = nFilBytes - 15 + 1;

    nBitsWritten += 8;
    BsPutBits(bs, 8, diff);
  }

  return (nBitsWritten);
}

int32
WriteSBRExtensionData2(TBitStream *bsOut, SbrFrameData *frameData[2], 
                       SbrHeaderData *hHeaderData, SbrExtensionData *sbrExtData, 
                       uint8 isStereo, uint8 headerPresent)
{
  int32 bitsWritten, nFilBytes;

  /*-- Write fill element code. --*/
  BsPutBits(bsOut, 3, 0x6);

  /*-- Count SBR part. --*/
  bitsWritten = WriteSBR(NULL, frameData, hHeaderData, sbrExtData, isStereo, headerPresent, 0);
  nFilBytes = bitsWritten >> 3;

  /*-- Write length of FIL element. --*/
  WriteAACFilLength(bsOut, nFilBytes);

  /*-- Write SBR data. --*/
  if(nFilBytes > 0)
  {
    bitsWritten = WriteSBR(bsOut, frameData, hHeaderData, sbrExtData, isStereo, headerPresent, 1);
    nFilBytes -= bitsWritten >> 3;
  }

  /*-- Write dummy data if needed. --*/
  if(nFilBytes > 0)
  {
    /*-- Write fill element code. --*/
    BsPutBits(bsOut, 3, 0x6);

    /*-- Write length of FIL element. --*/
    bitsWritten += WriteAACFilLength(bsOut, nFilBytes);

    WriteDummyPayload(bsOut, nFilBytes);
  }

  return (BsGetBitsRead(bsOut));
}

int32
WriteSBRExtensionData(TBitStream *bsIn, TBitStream *bsOut, int16 bitOffset,
                      SBR_Decoder *sbrDecoder, 
                      SbrHeaderData *hHeaderData,
                      uint8 isStereo, 
                      uint8 headerPresent)
{
  int32 bitsWritten; 

  /*-- Locate start position for SBR data within the buffer. --*/
  BsCopyBits(bsIn, bsOut, bitOffset);

  bitsWritten = BsGetBitsRead(bsOut);

  WriteSBRExtensionData2(bsOut, sbrDecoder->frameData, hHeaderData, sbrDecoder->sbrExtData, isStereo, headerPresent);

  bitsWritten = BsGetBitsRead(bsOut) - bitsWritten;

  return (bitsWritten);
}

int16
WriteSBRSilenceElement(SBR_Decoder *sbrDecoder, TBitStream *bsOut, uint8 isStereo)
{
  int32 bitsWritten;

  bitsWritten = BsGetBitsRead(bsOut);

  WriteSBRExtensionData2(bsOut, sbrDecoder->frameData, sbrDecoder->sbrHeader[0], sbrDecoder->sbrExtData, isStereo, 1);

  bitsWritten = BsGetBitsRead(bsOut) - bitsWritten;

  return (bitsWritten);
}

void
InitSBRSilenceData(SBR_Decoder *sbrDecoder, uint8 isStereo, uint8 isParametricStereo)
{
  sbrDecoder->sbrHeader[0]->startFreq = 15;
  sbrDecoder->sbrHeader[0]->stopFreq = 14;

  sbrDecoder->frameData[0]->frameInfo.nEnvelopes = 1;
  sbrDecoder->frameData[0]->frameInfo.nNoiseEnvelopes = 1;
  if(isStereo)
  {
    sbrDecoder->frameData[1]->frameInfo.nEnvelopes = 1;
    sbrDecoder->frameData[1]->frameInfo.nNoiseEnvelopes = 1;
  }

  sbrDecoder->sbrExtData->writePsData = (uint8) ((isParametricStereo) ? 1 : 0);

  resetFreqBandTables(sbrDecoder->sbrHeader[0]);
  sbrDecoder->sbrHeader[0]->syncState = SBR_ACTIVE;
}

int16
GenerateSBRSilenceDataL(uint8 *OutBuffer, int16 OutBufferSize, int32 sampleRate, 
                        uint8 isStereo, uint8 isParametricStereo)
{
  int16 sbrBits;
  TBitStream bsOut;
  SBR_Decoder *sbrDecoder;

  sbrDecoder = OpenSBRDecoderL(sampleRate, 1024, isStereo, 0);

  InitSBRSilenceData(sbrDecoder, isStereo, isParametricStereo);

  BsInit(&bsOut, OutBuffer, OutBufferSize);

  sbrBits = (int16) WriteSBRExtensionData2(&bsOut, sbrDecoder->frameData, sbrDecoder->sbrHeader[0], sbrDecoder->sbrExtData, isStereo, 1);

  return (sbrBits);
}

int16
WriteSBRExtensionSilenceData(TBitStream *bsOut, uint8 *SbrBuffer, 
                             int16 SbrBits, uint8 writeTerminationCode)
{
  TBitStream bsSbr;
  int16 bitsWritten;

  /*-- Initialize bitstream parser. --*/
  BsInit(&bsSbr, SbrBuffer, (SbrBits + 8) >> 3);

  /*-- Write the silence data. --*/
  BsCopyBits(&bsSbr, bsOut, SbrBits);

  /*-- Write termination code. --*/
  if(writeTerminationCode)
    BsPutBits(bsOut, 3, 0x7);

  bitsWritten = BsGetBitsRead(bsOut);

  if(writeTerminationCode)
  {
    /*-- Byte align. --*/
    if(bitsWritten & 0x7)
    {
      int16 bitsLeft = 8 - (bitsWritten & 0x7);

      bitsWritten += bitsLeft;
      BsPutBits(bsOut, bitsLeft, 0);
    }
  }

  return (bitsWritten);
}

uint8
ParseSBRPayload(SBR_Decoder *self, SbrHeaderData *hHeaderData, 
                SbrElementStream *sbrElement, int16 decVal,
                uint8 isStereo)
{
  TBitStream bs;

  /*-- Initialize bitstream. --*/
  BsInit(&bs, sbrElement->Data, sbrElement->Payload);

  /*-- Remove invalid data from bit buffer. --*/
  BsGetBits(&bs, 4);

  /*-- CRC codeword present? --*/
  if(sbrElement->ExtensionType == SBR_EXTENSION_CRC)
    BsGetBits(&bs, 10);

  /*-- Header present? --*/
  sbrElement->headerStatus = (uint8) BsGetBits(&bs, 1);

  /*-- Read header data. --*/
  if(sbrElement->headerStatus) 
  {
    SBR_HEADER_STATUS headerStatus;
    
    headerStatus = sbrGetHeaderData(hHeaderData, &bs);
    if(headerStatus == HEADER_RESET) 
    {
      int16 err;

      /*-- Reset values. --*/
      err = resetFreqBandTables(hHeaderData);
      if(err == 0) hHeaderData->syncState = SBR_ACTIVE;
    }
  }

  /*-- Read payload data. --*/
  if(hHeaderData->syncState == SBR_ACTIVE) 
  {
    /*-- Read channel pair element related data. --*/
    if(isStereo)
      sbrGetCPE(hHeaderData, self->frameData[0], self->frameData[1], self->sbrExtData, &bs, decVal);

    /*-- Read mono data. --*/
    else
      sbrGetSCE(hHeaderData, self->frameData[0], self->sbrExtData, &bs, decVal, 1);
  }

  return (1);
}

/*!
  \brief     SBR bitstream parsing

  \return    Number of bytes written to output bitstream
*/
int16
ParseSBR(TBitStream *bsIn, TBitStream *bsOut, SBR_Decoder *self, 
         SbrBitStream *Bitstr, int16 decVal)
{
  int32 i, bitsWritten = BsGetBitsRead(bsOut);

  /*
   * Write frame bits from the start of frame till 
   * the start of 1st channel element. 
   */
  BsCopyBits(bsIn, bsOut, Bitstr->sbrElement[0].elementOffset);

  if(Bitstr->NrElements) 
  {
    int16 SbrFrameOK, dualMono;

    SbrFrameOK = 1;
    dualMono = (Bitstr->NrElements == 2) ? 1 : 0;

    for(i = 0; i < Bitstr->NrElements; i++) 
    {
      uint8 stereo;
      SbrElementStream *sbrElement;

      sbrElement = &Bitstr->sbrElement[i];
      
      if(sbrElement->Payload < 1)
        continue;

      stereo = 0;

      switch(sbrElement->ElementID) 
      {
        case SBR_ID_SCE:
          stereo = 0;
          break;

        case SBR_ID_CPE:
          stereo = 1;
          break;

        default:
          SbrFrameOK = 0;
          break;
      }

      if(SbrFrameOK) 
      {
        ParseSBRPayload(self, self->sbrHeader[dualMono ? i : 0], sbrElement, decVal, stereo);

        WriteSBRExtensionData(bsIn, bsOut, sbrElement->chElementLen, self,
                              self->sbrHeader[dualMono ? i : 0],
                              stereo, sbrElement->headerStatus);

        if(i < (Bitstr->NrElements - 1))
        {
          int32 endElement, startNextElement;

          endElement = sbrElement->elementOffset + sbrElement->chElementLen;
          startNextElement = Bitstr->sbrElement[i + 1].elementOffset;

          BsSkipNBits(bsIn, startNextElement - endElement);
        }
      }
    }

    /*-- Write termination code. --*/
    BsPutBits(bsOut, 3, 0x7);

    bitsWritten = BsGetBitsRead(bsOut) - bitsWritten;

    /*-- Byte align. --*/
    if(bitsWritten & 0x7)
    {
      uint8 bitsLeft = (uint8) (8 - (bitsWritten & 0x7));
      
      bitsWritten += bitsLeft;
      BsPutBits(bsOut, bitsLeft, 0);
    }
  }

  return (int16) (bitsWritten >> 3);
}

uint8
IsSBRParametricStereoEnabled(SBR_Decoder *self, SbrBitStream *Bitstr)
{
  uint8 isParamStereoPresent;

  isParamStereoPresent = 0;

  if(self && Bitstr->NrElements == 1) 
  {
    SbrElementStream *sbrElement;

    sbrElement = &Bitstr->sbrElement[0];

    if(sbrElement->Payload > 0 && sbrElement->ElementID == SBR_ID_SCE)
    {
      ParseSBRPayload(self, self->sbrHeader[0], sbrElement, 0, 0);

      if(self->sbrExtData->extensionDataPresent)
        if(self->sbrExtData->extension_id == SBR_PARAMETRIC_STEREO_ID)
          isParamStereoPresent = 1;
    }
  }

  return (isParamStereoPresent);
}

uint8
IsSBREnabled(SbrBitStream *Bitstr)
{
  uint8 isSBR;

  isSBR = (Bitstr && Bitstr->NrElements) ? 1 : 0;

  return (isSBR);
}

int16
WriteSBRSilence(TBitStream *bsIn, TBitStream *bsOut, SbrBitStream *streamSBR,
                uint8 *SbrBuffer, int16 SbrBits)
{
  int32 i;

  /*
   * Write frame bits from the start of frame till 
   * the start of 1st channel element. 
   */
  BsCopyBits(bsIn, bsOut, streamSBR->sbrElement[0].elementOffset);

  /*-- Write channel element. --*/
  BsCopyBits(bsIn, bsOut, streamSBR->sbrElement[0].chElementLen);

  for(i = 0; i < streamSBR->NrElements; i++)
  {
    uint8 writeEndCode = (i == (streamSBR->NrElements - 1)) ? 1 : 0;

    WriteSBRExtensionSilenceData(bsOut, SbrBuffer, SbrBits, writeEndCode);

    if(i < (streamSBR->NrElements - 1))
    {
      int32 endElement, startNextElement;
      
      endElement = streamSBR->sbrElement[i].elementOffset + streamSBR->sbrElement[i].chElementLen;
      startNextElement = streamSBR->sbrElement[i + 1].elementOffset;

      BsSkipNBits(bsIn, startNextElement - endElement);

      BsCopyBits(bsIn, bsOut, streamSBR->sbrElement[i + 1].chElementLen);
    }
  }

  return (int16) (BsGetBitsRead(bsOut) >> 3);
}
