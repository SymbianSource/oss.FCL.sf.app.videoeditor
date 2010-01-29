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
  External Objects Needed
  *************************************************************************/

/*-- Project Headers --*/
#include "mstream.h"
#include "mpheader.h" 
#include "mp3Tool.h"



CIII_Channel_Info* CIII_Channel_Info::NewL() 
    {


    CIII_Channel_Info* self = new (ELeave) CIII_Channel_Info();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }

void CIII_Channel_Info::ConstructL()
    {
    
    }

CIII_Channel_Info::CIII_Channel_Info()
    {


    }

CIII_Channel_Info::~CIII_Channel_Info()
    {


    }


CIII_SfbData* CIII_SfbData::NewL() 
    {

    CIII_SfbData* self = new (ELeave) CIII_SfbData();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }

void CIII_SfbData::ConstructL()
    {
    sfbOffsetLong = new (ELeave) int16[MAX_LONG_SFB_BANDS + 1];
    sfbOffsetShort = new (ELeave) int16[MAX_SHORT_SFB_BANDS + 1];
    sfbWidthShort = new (ELeave) int16[MAX_SHORT_SFB_BANDS + 1];
    }

CIII_SfbData::CIII_SfbData()
    {


    }

CIII_SfbData::~CIII_SfbData()
    {
    if (sfbOffsetLong != 0) delete[] sfbOffsetLong;
    if (sfbOffsetShort != 0) delete[] sfbOffsetShort;
    if (sfbWidthShort != 0) delete[] sfbWidthShort;

    }

CIII_Side_Info* CIII_Side_Info::NewL() 
    {

    CIII_Side_Info* self = new (ELeave) CIII_Side_Info();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }

void CIII_Side_Info::ConstructL()
    {
    sfbData = GET_SYMBIAN_CHUNK(CIII_SfbData);

    }

CIII_Side_Info::CIII_Side_Info()
    {

    }

CIII_Side_Info::~CIII_Side_Info()
    {
    if (sfbData != 0) SAFE_SYMBIAN_DELETE(sfbData);

    }

CMCUBuf* CMCUBuf::NewL(TInt aBufLen) 
    {

    CMCUBuf* self = new (ELeave) CMCUBuf();
    CleanupStack::PushL(self);
    self->ConstructL(aBufLen);
    CleanupStack::Pop(self);
    return self;
    }

void CMCUBuf::ConstructL(TInt aBufLen)
    {
    
    bs = new (ELeave) TBitStream();

    mcuBufbits = new (ELeave) uint8[aBufLen];  


    }

CMCUBuf::CMCUBuf()
    {

    }

CMCUBuf::~CMCUBuf()
    {

    if (bs != 0) delete bs;
    if (mcuBufbits != 0) 
        {
        delete[] mcuBufbits;
        mcuBufbits = 0;
        }

    }

/**************************************************************************
  Title        : decode_header

  Purpose      : Reads header information (excluding syncword) from the bitstream.

  Usage        : decode_header(mp)

  Input        : mp - mp3 bitstream parameters

  Explanation  : Header information is commmon to all layers. Note also that
                 this function doesn't interprete the fields of the header.

  Author(s)    : Juha Ojanpera
  *************************************************************************/

void
decode_header(CMP_Stream *mp, TBitStream *bs)
{
  uint32 header;

  mp->headerOld.header = mp->header->header;
  header = (!mp->side_info->mpeg25) << HEADER_BITS;
  header |= BsGetBits(bs, HEADER_BITS);
  mp->header->header = header;

  /*-- Store the header bits 16-31 for CRC error checking. --*/
  mp->mp3_crc.crc_payload[0] = (uint8)((header >> 8) & 255);
  mp->mp3_crc.crc_payload[1] = (uint8) (header & 255);

  if(error_protection(mp->header))
    mp->mp3_crc.crc = (int16)(BsGetBits(bs, 16));
}

/**************************************************************************
  Title        : FillDataSlotTable

  Purpose      : Pre-computes (to avoid division operation during decoding) 
                 the payload size of layer III for all bitrates.

  Usage        : y = FillDataSlotTable(mp)

  Input        : mp - mp3 stream parameters

  Author(s)    : Juha Ojanpera
  *************************************************************************/

void
FillDataSlotTable(CMP_Stream *mp)
{
  const int16 *brTbl;
  int16 nSlots;
  
  brTbl = GetBitRateTable(mp->header);
  /*
   * index 0 is free format and index 14 illegal bitrate.
   */
  for(int16 i = 1; i < 15; i++)
  {
    nSlots = (int16)((144 * brTbl[i]) / (frequency(mp->header) / 1000.0f));

    if(version(mp->header) == MPEG_PHASE2_LSF)
      nSlots >>= 1;

    mp->FrameTable[i] = nSlots;

    nSlots = (int16)(nSlots - (GetSideInfoSlots(mp->header) + 4));
    mp->SlotTable[i] = nSlots;
  }
}

/**************************************************************************
  Title        : main_data_slots

  Purpose      : Computes the number of bytes for the layer III payload. The
                 payload consists of the scalefactors and quantized data of
                 the channel(s).

  Usage        : y = main_data_slots(mp)

  Input        : mp - mp3 stream parameters

  Output       : y - # of payload bytes for this frame

  Author(s)    : Juha Ojanpera
  *************************************************************************/

int32
main_data_slots(CMP_Stream *mp)
{
  int16 nSlots;

  if(bit_rate(mp->header))
  {
    nSlots = mp->SlotTable[bit_rate_idx(mp->header)];

    if(padding(mp->header))
      nSlots++;
    if(error_protection(mp->header))
      nSlots -= 2;
  }
  else
  {
    nSlots = mp->FreeFormatSlots;

    if(padding(mp->header))
      nSlots++;
  }

  return(nSlots);
}

/**************************************************************************
  Title        : ReleaseMP3Decoder

  Purpose      : Releases resources allocated to the mp3 decoder core.

  Usage        : ReleaseMP3Decoder(mp)

  Input        : mp - mp3 decoder core

  Author(s)    : Juha Ojanpera
  *************************************************************************/

void
ReleaseMP3Decoder(CMP_Stream *mp)
    {
    int16 i, j;
  
    if(mp)
        {
        /* Scalefactors. */
        SAFE_DELETE(mp->frame->scale_factors);
        
        /* Quantized samples. */
        SAFE_DELETE(mp->frame->quant);

        /* Synthesis buffer. */
        for(i = 0; i < MAX_CHANNELS; i++)
            {
            SAFE_DELETE(mp->buffer->synthesis_buffer[i]);    
            }
          
        
        /* Dequantized samples. */
        SAFE_DELETE(mp->buffer->reconstructed);

        /* Huffman codebooks. */
        SAFE_DELETE(mp->huffman);
        
        if(mp->side_info)
            {
              for(i = 0; i < MAX_CHANNELS; i++)
                  {
                SAFE_DELETE(mp->side_info->ch_info[i]->scale_fac);
      
                for(j = 0; j < 2; j++)
                    {
                    SAFE_DELETE(mp->side_info->ch_info[i]->gr_info[j]);    
                    }
      
      
                SAFE_SYMBIAN_DELETE(mp->side_info->ch_info[i]);
                  }
    
            SAFE_DELETE(mp->side_info->s_mode_long);
    
            for(i = 0; i < 3; i++)
                {
                SAFE_DELETE(mp->side_info->s_mode_short[i]);    
                }
                
    
            SAFE_SYMBIAN_DELETE(mp->side_info);
            }
  
        SAFE_DELETE(mp->header);
        SAFE_DELETE(mp->frame);
        SAFE_DELETE(mp->buffer);
        SAFE_DELETE(mp->br);
//    SAFE_DELETE(mp->bs);
          }
    }

/**************************************************************************
  Title        : GetMP3Handle

  Purpose      : Returns mp3 decoder core handle to the callee.

  Usage        : GetMP3Handle()

  Output       : mp - handle of mp3 decoder core

  Author(s)    : Juha Ojanpera
  *************************************************************************/

CMP_Stream *
GetMP3HandleL(void)
{
  int16 i, j, groups, idx[] = {0, 23, 36, 49, 62, 85, 98, 111};
  CIII_Scale_Factors *scale_fac;
  CMP_Stream *mp;

  //mp = (CMP_Stream *) GET_CHUNK(sizeof(CMP_Stream));
  
  mp = new (ELeave) CMP_Stream();
  IS_ERROR(mp);



  //mp->bs = (TBitStream *) GET_CHUNK(sizeof(TBitStream));
  //IS_ERROR(mp->bs);

  mp->header = (TMPEG_Header *) GET_CHUNK(sizeof(TMPEG_Header));
  IS_ERROR(mp->header);

  mp->frame = (TMPEG_Frame *) GET_CHUNK(sizeof(TMPEG_Frame));
  IS_ERROR(mp->frame);

  mp->buffer = (TMPEG_Buffer *) GET_CHUNK(sizeof(TMPEG_Buffer)); 
  IS_ERROR(mp->buffer);

  mp->side_info = (CIII_Side_Info *) GET_SYMBIAN_CHUNK(CIII_Side_Info);
  IS_ERROR(mp->side_info);

  mp->frame->scale_factors = (uint8 *) GET_CHUNK(MAX_CHANNELS * SBLIMIT * 3 * sizeof(uint8));
  Mem::FillZ    (mp->frame->scale_factors, MAX_CHANNELS * SBLIMIT * 3 * sizeof(uint8));

  IS_ERROR(mp->frame->scale_factors);

  mp->huffman = (CHuffman *) GET_CHUNK(33 * sizeof(CHuffman)); 
  IS_ERROR(mp->huffman);

  mp->br = (TBitStream *) GET_CHUNK(sizeof(TBitStream)); 
  IS_ERROR(mp->br);

  Mem::Fill(mp->PrevStreamInfo, sizeof(uint32) * 2, 0);

  for(i = 0; i < MAX_CHANNELS; i++)
  {
    //mp->side_info->ch_info[i] = (CIII_Channel_Info *) GET_CHUNK(sizeof(CIII_Channel_Info));
      mp->side_info->ch_info[i] = GET_SYMBIAN_CHUNK(CIII_Channel_Info);
    IS_ERROR(mp->side_info->ch_info[i]);
    for(j = 0; j < 2; j++)
    {
      mp->side_info->ch_info[i]->gr_info[j] = (TGranule_Info *) GET_CHUNK(sizeof(TGranule_Info));
      IS_ERROR(mp->side_info->ch_info[i]->gr_info[j]);
    }
  }
  
  mp->side_info->s_mode_long = (StereoMode *) GET_CHUNK(22 * sizeof(StereoMode));
  IS_ERROR(mp->side_info->s_mode_long);
  for(i = 0; i < 3; i++)
  {
    mp->side_info->s_mode_short[i] =  (StereoMode *) GET_CHUNK(13 * sizeof(StereoMode));
    IS_ERROR(mp->side_info->s_mode_short[i]);
  }
  
  for(i = j = 0; i < MAX_CHANNELS; i++)
  {
    mp->side_info->ch_info[i]->scale_fac = (CIII_Scale_Factors *) GET_CHUNK(sizeof(CIII_Scale_Factors));
    IS_ERROR(mp->side_info->ch_info[i]->scale_fac);
    scale_fac = mp->side_info->ch_info[i]->scale_fac;
    
    scale_fac->scalefac_long = mp->frame->scale_factors + idx[j++];
    scale_fac->scalefac_short[0] = mp->frame->scale_factors + idx[j++];
    scale_fac->scalefac_short[1] = mp->frame->scale_factors + idx[j++];
    scale_fac->scalefac_short[2] = mp->frame->scale_factors + idx[j++];
  }

  groups = MAX_MONO_SAMPLES * MAX_CHANNELS;

  TInt a = 0;
  mp->frame->quant = (int16 *) GET_CHUNK((groups + 10) * sizeof(int16));
    for (a = 0 ; a < groups ; a++) mp->frame->quant[a] = 0;

  IS_ERROR(mp->frame->quant);

  mp->buffer->reconstructed = (FLOAT *) GET_CHUNK(groups * sizeof(FLOAT));
  IS_ERROR(mp->buffer->reconstructed);
  for (a = 0 ; a < groups ; a++) mp->buffer->reconstructed[a] = 0;

  for(i = 0; i < MAX_CHANNELS; i++)
  {
    mp->frame->ch_quant[i] = mp->frame->quant + i * MAX_MONO_SAMPLES;
    mp->buffer->ch_reconstructed[i] = mp->buffer->reconstructed + i * MAX_MONO_SAMPLES;
    for(j = 0; j < SBLIMIT; j++)
      mp->spectrum[i][j] = &mp->buffer->ch_reconstructed[i][j * SSLIMIT];
  }

  for(i = 0; i < MAX_CHANNELS; i++)
  {
    mp->buffer->buf_idx[i] = mp->buffer->dct_idx[i] = 0;
    mp->buffer->synthesis_buffer[i] = (FLOAT *) GET_CHUNK((HAN_SIZE << 1) * sizeof(FLOAT));
    IS_ERROR(mp->buffer->synthesis_buffer[i]);
  }

  //-- Get the Huffman codebooks. --
  //init_huffman(mp->huffman);
  InitL3Huffman(mp->huffman);
  return (mp);

// error_exit:
  
  //ReleaseMP3Decoder(mp);
  
  //return (NULL);
}

/**************************************************************************
  Title        : MP3DecPrepareInit

  Purpose      : Prepares the core engine parameters for the search of 
                 first mp3 frame.

  Usage        : MP3DecPrepareInit(mp, out_param, complex, br_buffer, br_size)

  Input        : mp        - handle of mp3 decoder core
                 out_param - output parameters of current track
         complex   - decoding complexity parameters
         br_buffer - address of bit reservoir buffer
         br_size   - size of bit reservoir buffer

  Author(s)    : Juha Ojanpera
  *************************************************************************/

void
MP3DecPrepareInit(CMP_Stream *mp, Out_Param *out_param, Out_Complexity *complex,
          DSP_BYTE *br_buffer, uint32 br_size)
{
  mp->complex = complex;
  mp->out_param = out_param;

  BsInit2(mp->br, br_buffer, br_size);
  
  mp->mp3_crc.crc = 0;
  
  mp->header->header = 0;
  
  mp->syncInfo.sync_word = (int16)SYNC_WORD;
  mp->syncInfo.sync_length = (int16)SYNC_WORD_LENGTH;
  mp->syncInfo.sync_mask = (int16)((1 << mp->syncInfo.sync_length) - 1);
  mp->syncInfo.sync_status = FIRST_FRAME_WITH_LAYER3;

  mp->FreeFormatSlots = 0;
  mp->idx_increment = 0;
  mp->PrevSlots = 0;
  mp->FrameStart = 0;
  mp->SkipBr = FALSE;
  mp->WasSeeking = FALSE;
  mp->OverlapBufPtr[0] = mp->OverlapBufPtr[1] = 0;
}

/**************************************************************************
  Title        : MP3DecCompleteInit

  Purpose      : Completes the initialization of the core engine parameters.

  Usage        : MP3DecPrepareInit(mp, frameBytes)

  Input        : mp - handle of mp3 decoder core

  Output       : frameBytes - # of bytes for the first frame

  Author(s)    : Juha Ojanpera
  *************************************************************************/

void
MP3DecCompleteInit(CMP_Stream *mp, int16 *frameBytes)
{
  //-- Fixed size (unit is bytes !!). --
  mp->mp3_crc.bufLen = (uint16)(2 + GetSideInfoSlots(mp->header));

  //-- MPEG-1 --/
  if(version(mp->header) == MPEG_AUDIO_ID)
  {
    mp->side_info->lsf = FALSE;
    mp->side_info->max_gr = 2;
  }
  //-- MPEG-2 LSF or MPEG-2.5 --
  else
  {
    mp->side_info->lsf = TRUE;
    mp->side_info->max_gr = 1;
  }

  //-- Determine the size of the payload only when necessary. --/
  if(bit_rate(mp->header))
  {
    mp->FreeFormatSlots = 0;

    if((int32)(frequency(mp->header) != mp->PrevStreamInfo[0]) ||
       (int32)(channels(mp->header) != mp->PrevStreamInfo[1]))
      FillDataSlotTable(mp);
  }
  else FillDataSlotTable(mp);
  
  mp->PrevStreamInfo[0] = frequency(mp->header);
  mp->PrevStreamInfo[1] = channels(mp->header);

  //-- Get the scalefactor band related parameters. --/
  III_SfbDataInit(mp->side_info->sfbData, mp->header);
  
  //-- Init re-ordering table. --//
  init_III_reorder(mp->reorder_idx, mp->side_info->sfbData->sfbShort, 
           mp->side_info->sfbData->sfbWidth);

  //-- Number of bytes for next frame. --/
  *frameBytes = (int16)(main_data_slots(mp) + GetSideInfoSlots(mp->header) + 3);
  if(error_protection(mp->header))
    *frameBytes += 2;
}
