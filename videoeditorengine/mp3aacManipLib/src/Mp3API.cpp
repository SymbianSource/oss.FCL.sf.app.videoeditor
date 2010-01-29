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



#include "Mp3API.h"
#include "mpif.h"
#include "mpaud.h"
#include "mpheader.h"
#include "mp3tool.h"




// Exception handler for FLOATS
void handler(TExcType /*aType*/)
{
#ifndef __WINS__
//    CMp3Mix::WriteL(_L("Exception cought!!!"));
#endif

}

//const int16 Kmp3BufSize = 8;

EXPORT_C CMp3Edit* CMp3Edit::NewL()
    {

    CMp3Edit* self = new (ELeave) CMp3Edit();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;

    }

void CMp3Edit::ConstructL()
    {
    
      }


EXPORT_C CMp3Edit::~CMp3Edit()
    {

    }

CMp3Edit::CMp3Edit()
    {

    }


EXPORT_C uint32
CMp3Edit::FileLengthInMs(TMpTransportHandle *tHandle, int32 fileSize) const
    {
    return MP_FileLengthInMs(tHandle, fileSize);
    }

EXPORT_C int32
CMp3Edit::GetSeekOffset(TMpTransportHandle *tHandle, int32 seekPos) const
    {
    return MP_GetSeekOffset(tHandle, seekPos);
    }

EXPORT_C int32
CMp3Edit::GetFrameTime(TMpTransportHandle *tHandle) const
    {
    return MP_GetFrameTime(tHandle);
    }

EXPORT_C void 
CMp3Edit::InitTransport(TMpTransportHandle *tHandle) const
    {

    mpInitTransport(tHandle);
    }

EXPORT_C int16
CMp3Edit::SeekSync(TMpTransportHandle *tHandle, uint8 *syncBuf, 
            uint32 syncBufLen, int16 *readBytes, 
            int16 *frameBytes, int16 *headerBytes,
            uint8 initMode) const

    {

    return MP_SeekSync(tHandle, syncBuf, 
            syncBufLen, readBytes, 
            frameBytes, headerBytes,
            initMode);
    }

EXPORT_C int16
CMp3Edit::FreeMode(TMpTransportHandle *tHandle, uint8 *syncBuf, 
            uint32 syncBufLen, int16 *readBytes, 
            int16 *frameBytes, int16 *headerBytes) const
    {
    return MP_FreeMode(tHandle, syncBuf, 
            syncBufLen, readBytes, 
            frameBytes, headerBytes);
    }

EXPORT_C int16
CMp3Edit::EstimateBitrate(TMpTransportHandle *tHandle, uint8 isVbr) const
    {
    return MP_EstimateBitrate(tHandle, isVbr) ;

    }



/*
 * Saves modified 'global_gain' bitstream elements to specified layer I/II/III data buffer.
 */
EXPORT_C void
CMp3Edit::SetMPGlobalGains(TBitStream *bs, uint8 numGains, uint8 *globalGain, uint32 *gainPos)
{
  int16 i;

  /*-- Store the gain element back to bitstream. --*/
  for(i = 0; i < numGains; i++)
  {
    BsSkipNBits(bs, gainPos[i]); 
    BsPutBits(bs, 8, globalGain[i]);
  }
}



EXPORT_C uint8
CMp3Edit::GetMPGlobalGains(TBitStream *bs, TMpTransportHandle *tHandle, uint8 *globalGain, uint32 *gainPos)
{
  uint32 bufBitOffset;
  uint8 numGains, nVersion, nChannels;

  numGains = 0;
  nVersion = (uint8) version(&tHandle->header);
  nChannels =(uint8)  channels(&tHandle->header);

  bufBitOffset = BsGetBitsRead(bs);

  /*-- Get the gain for each channel and granule. --*/
  switch(nVersion)
  {
    /*-- MPEG-1. --*/
    case 1:
      switch(nChannels)
      {
        /*
         * Mono frame consists of 2 granules.
         */
        case 1:
          numGains = 2;

          /*-- Left channel, granules 0 and granule 1. --*/
          BsSkipNBits(bs, 0x27);
          gainPos[0] = BsGetBitsRead(bs) - bufBitOffset;
          globalGain[0] = (uint8) BsGetBits(bs, 8);
          BsSkipNBits(bs, 0x33);
          gainPos[1] = 0x33;
          globalGain[1] = (uint8) BsGetBits(bs, 8);
          break;

        /*
         * Two channels, 2 granules.
         */
        case 2:
          numGains = 4;

          /*-- Left and right channel, granule 0. --*/
          BsSkipNBits(bs, 0x29);
          gainPos[0] = BsGetBitsRead(bs) - bufBitOffset;
          globalGain[0] = (uint8) BsGetBits(bs, 8);
          BsSkipNBits(bs, 0x33); 
          gainPos[1] = 0x33;
          globalGain[1] = (uint8) BsGetBits(bs, 8);

          /*-- Left and right channel, granule 1. --*/
          BsSkipNBits(bs, 0x33); 
          gainPos[2] = 0x33;
          globalGain[2] = (uint8) BsGetBits(bs, 8);
          BsSkipNBits(bs, 0x33); 
          gainPos[3] = 0x33;
          globalGain[3] = (uint8) BsGetBits(bs, 8);
          break;
      }
      break;

    /*-- MPEG-2 LSF and MPEG-2.5. --*/
    default:
      switch(nChannels)
      {
        /*
         * Mono channel, 1 granule.
         */
        case 1:
          numGains = 1;
          BsSkipNBits(bs, 0x1E); 
          gainPos[0] = BsGetBitsRead(bs) - bufBitOffset;
          globalGain[0] = (uint8) BsGetBits(bs, 8);
          break;

        /*
         * Two channels, 1 granule.
         */
        case 2:
          numGains = 2;
          BsSkipNBits(bs, 0x1F); 
          gainPos[0] = BsGetBitsRead(bs) - bufBitOffset;
          globalGain[0] = (uint8) BsGetBits(bs, 8);
          BsSkipNBits(bs, 0x37); 
          gainPos[1] = 0x37;
          globalGain[1] = (uint8) BsGetBits(bs, 8);
          break;
      }
      break;
  }


  return (numGains);
}

EXPORT_C CMPAudDec* CMPAudDec::NewL()
    {
    CMPAudDec* self = new (ELeave) CMPAudDec();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;


    }
    
CMPAudDec::CMPAudDec()
    {


    }

EXPORT_C CMPAudDec::~CMPAudDec()
    {

  
    int16 i;

    /* --Scalefactors. --*/
    if (frame->scale_factors != 0) delete[] frame->scale_factors;
    
    /*-- Quantized samples. --*/
    if (frame->quant != 0) delete[] frame->quant;

    /*-- Huffman codebooks. --*/
    if (huffman != 0) delete[] huffman;

    /*-- Layer III side info. --*/
    if(side_info)
        {
        for(i = 0; i < MAX_CHANNELS; i++)
            {
            int16 j;

            if(side_info->ch_info[i])
                {
                if (side_info->ch_info[i]->scale_fac != 0) delete side_info->ch_info[i]->scale_fac;
          
                for(j = 0; j < 2; j++)
                    if (side_info->ch_info[i]->gr_info[j] != 0) delete side_info->ch_info[i]->gr_info[j];
                }      
            if (side_info->ch_info[i] != 0) delete side_info->ch_info[i];
            }
    
        if (side_info->s_mode_long != 0) delete[] side_info->s_mode_long;
    
        for(i = 0; i < 3; i++)
            if (side_info->s_mode_short[i] != 0) delete[] side_info->s_mode_short[i];
    
        if (side_info != 0) delete side_info;
        }
  
    if (frame != 0) delete frame;
    if (bitReserv != 0) delete[] bitReserv;

    

    }

void CMPAudDec::ConstructL()
    {
     
    int16 i, j, groups;

    /*-- Create handle. --*/
    
    /*-- Create bit reservoir buffer. --*/
    bitReserv = new (ELeave) uint8[4 * MAX_BITRESER_SIZE];
    ZERO_MEMORY(bitReserv, 4 * MAX_BITRESER_SIZE * sizeof(uint8));

    BsInit(&br, bitReserv, 4 * MAX_BITRESER_SIZE);
  
    /*-- Create frame parameters. --*/
    frame = new (ELeave) TMPEG_Frame;
  
    /*-- Create side info parameters. --*/
    side_info = CIII_Side_Info::NewL();
  
    /*-- Create scalefactors. --*/
    frame->scale_factors = new (ELeave) uint8[MAX_CHANNELS * SBLIMIT * 3];
    ZERO_MEMORY(frame->scale_factors, MAX_CHANNELS * SBLIMIT * 3 * sizeof(uint8)); 


    /*-- Create L3 side info. --*/
    for(i = 0; i < MAX_CHANNELS; i++)
        {
    
        side_info->ch_info[i] = CIII_Channel_Info::NewL();
    
        for(j = 0; j < 2; j++)
            {
                side_info->ch_info[i]->gr_info[j] = new (ELeave) TGranule_Info;
      
            }
        }
  
    side_info->s_mode_long = new (ELeave) StereoMode[22];
    
    ZERO_MEMORY(side_info->s_mode_long, 22 * sizeof(StereoMode));

    for(i = 0; i < 3; i++)
        {
        side_info->s_mode_short[i] =  new (ELeave) StereoMode[13]; //GET_CHUNK(13 * sizeof(StereoMode));
        ZERO_MEMORY(side_info->s_mode_short[i], 13 * sizeof(StereoMode));
        }
  
    /*-- Initialize scalefactors. --*/
    for(i = j = 0; i < MAX_CHANNELS; i++)
        {
        uint8 idx[] = {0, 23, 36, 49, 62, 85, 98, 111};

        CIII_Scale_Factors *scale_fac;

        side_info->ch_info[i]->scale_fac = new (ELeave) CIII_Scale_Factors();
      
        scale_fac = side_info->ch_info[i]->scale_fac;

        scale_fac->scalefac_long     = frame->scale_factors + idx[j++];
        scale_fac->scalefac_short[0] = frame->scale_factors + idx[j++];
        scale_fac->scalefac_short[1] = frame->scale_factors + idx[j++];
        scale_fac->scalefac_short[2] = frame->scale_factors + idx[j++];
    }

    groups = MAX_MONO_SAMPLES * MAX_CHANNELS;

    /*-- Create buffer for quantized samples. --*/
    frame->quant = new (ELeave) int16[groups + 10];//(int16 *) GET_CHUNK((groups + 10) * sizeof(int16));

    ZERO_MEMORY(frame->quant, (groups + 10) * sizeof(int16));

    for(i = 0; i < MAX_CHANNELS; i++)
        frame->ch_quant[i] = frame->quant + i * MAX_MONO_SAMPLES;

    /*-- Create Huffman handle. --*/
    huffman = new (ELeave) CHuffman[33];// *) GET_CHUNK(33 * sizeof(CHuffman)); 
    
    /*-- Get the Huffman codebooks. --*/
    InitL3Huffman(huffman);

    }

    /*-- Initializes MPEG Layer I/II/III decoder handle. --*/
EXPORT_C void
CMPAudDec::Init(TMpTransportHandle *aMpFileFormat)
{
  mpFileFormat = aMpFileFormat;

  /*-- MPEG-1 --*/
  if(version(&mpFileFormat->header) == MPEG_AUDIO_ID)
    {
    side_info->lsf = FALSE;
    side_info->max_gr = 2;
    }

    /*-- MPEG-2 LSF or MPEG-2.5 --*/
    else
    {
        side_info->lsf = TRUE;
        side_info->max_gr = 1;
    }

    /*-- Get the scalefactor band related parameters. --*/
    III_SfbDataInit(side_info->sfbData, &mpFileFormat->header);
}



