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
  mpif.cpp - MPEG-1, MPEG-2 LSF and MPEG-2.5 file format implementations.

  Author(s): Juha Ojanpera
  Copyright (c) 1999-2004 by Nokia Research Center, Speech and Audio Systems.
  *************************************************************************/

/*-- project headers --*/
#include "mpif.h"
//#include "auddef.h"
#include "mpheader.h"
#include "nok_bits.h"
#include "mstream.h"


/**************************************************************************
  Internal Objects
  *************************************************************************/

/*
   Purpose:     Sync lost after 16384 bytes.
   Explanation: - */
#define SYNC_THRESHOLD (16384 << 3)

/**************************************************************************
  Title        : main_data_slots

  Purpose      : Computes the number of bytes for the layer III payload. The
                 payload consists of the scalefactors and quantized data of
                 the channel(s).

  Usage        : y = main_data_slots(mp)

  Input        : mp - mp3 file format parameters

  Output       : y  - # of payload bytes for this frame

  Author(s)    : Juha Ojanpera
  *************************************************************************/

INLINE int32
main_data_slots(TMpTransportHandle *tHandle)
{
  int16 nSlots;

  if(bit_rate(&tHandle->header))
  {
    nSlots = tHandle->SlotTable[bit_rate_idx(&tHandle->header)];

    if(padding(&tHandle->header))
      nSlots++;
    if(error_protection(&tHandle->header))
      nSlots -= 2;
  }
  else
  {
    nSlots = tHandle->FreeFormatSlots;

    if(padding(&tHandle->header))
      nSlots++;
  }

  return(nSlots);
}

/*
 * Maximum # of bits needed for the header. Note that only
 * 20 bits are read since 12 bits were read already when 
 * locating the start of frame. 16 bits are needed for
 * the optional CRC codeword.
 */
#define MAX_MP_HEADER_SIZE (20 + 16)

/**************************************************************************
  Title        : decode_header

  Purpose      : Reads header information (excluding syncword) from the bitstream.

  Usage        : decode_header(tHandle, bs)

  Input        : tHandle - file format parser
                 bs      - input bitstream

  Explanation  : Header information is commmon to all layers. Note also that
                 this function doesn't interprete the fields of the header.

  Author(s)    : Juha Ojanpera
  *************************************************************************/

INLINE SEEK_STATUS
decode_header(TMpTransportHandle *tHandle, TBitStream *bs)
{
  uint32 header, bitsLeft;

  bitsLeft = (BsGetBufSize(bs) << 3) - BsGetBitsRead(bs);
  if(bitsLeft < MAX_MP_HEADER_SIZE)
    return (SYNC_BITS_OUT);

  /*-- Read rest of the header bits. --*/
  tHandle->headerOld.header = tHandle->header.header;
  header = (!tHandle->mpeg25) << HEADER_BITS;
  header |= BsGetBits(bs, HEADER_BITS);
  tHandle->header.header = header;

  /*-- Read CRC codeword. --*/
  if(error_protection(&tHandle->header))
    tHandle->crc.crc = (int16) BsGetBits(bs, 16);

  return (SYNC_FOUND);
}

/**************************************************************************
  Title        : GetSideInfoSlots

  Purpose      : Retrieves the amount of side info for layer III stream.

  Usage        : y = GetSideInfoSlots(tHandle)

  Input        : tHandle - file format parser

  Output       : y       - # of side info bytes

  Author(s)    : Juha Ojanpera
  *************************************************************************/

int16
GetSideInfoSlots(TMpTransportHandle *tHandle)
{
  int16 nSlots = 0;

  if(version(&tHandle->header) != MPEG_PHASE2_LSF)
    nSlots = (channels(&tHandle->header) == 1) ? (int16) 17 : (int16) 32;
  else
    nSlots = (channels(&tHandle->header) == 1) ? (int16) 9 : (int16) 17;

  return (nSlots);
}

int16
GetSideInfoSlots(TMPEG_Header *header)
{
  int16 nSlots = 0;

  if(version(header) != MPEG_PHASE2_LSF)
    nSlots = (channels(header) == 1) ? 17 : 32;
  else
    nSlots = (channels(header) == 1) ? 9 : 17;

  return (nSlots);
}

/**************************************************************************
  Title        : GetSlotSize

  Purpose      : Retrieves the amount of frame data for layer III stream.

  Usage        : y = GetSlotSize(tHandle)

  Input        : tHandle - file format parser

  Output       : y       - # of frame data bytes

  Author(s)    : Juha Ojanpera
  *************************************************************************/

static INLINE int16
GetSlotSize(TMpTransportHandle *tHandle)
{
  int16 ave_slots;
  
  if(bit_rate(&tHandle->header))
    ave_slots = (int16) tHandle->aveFrameLen;
  else
  {
    ave_slots = (int16) (tHandle->FreeFormatSlots + GetSideInfoSlots(tHandle));
    if(error_protection(&tHandle->header))
      ave_slots += 2;
    ave_slots += 4;
  }

  return (ave_slots);
}

INLINE int32 *
GetSeekOffset(TMpTransportHandle *tHandle, int32 *seekValues)
{
  int16 main_data;
  int32 frameOffset, byte_offset;
  
  /*
   * Some reinitialization need to be performed after file pointer has been
   * moved to the new postion. For this reason, the total number of frames
   * to be skipped does not correspond exactly to the given time offset.
   * However, after reinitialization the playback position should be the correct.
   */
  byte_offset = GetSlotSize(tHandle);
  main_data = (int16) ((version(&tHandle->header) == MPEG_PHASE2_LSF) ? 256 : 512);
  frameOffset = (byte_offset) ? main_data / byte_offset : 0;

  if(byte_offset)
    if(main_data % byte_offset)
      frameOffset++;

  seekValues[0] = frameOffset;
  seekValues[1] = byte_offset;

  return (seekValues);
}

/*
 * Estimates the bitrate of the mp3 stream from the size of the 
 * payload + side info + header. The accuracy of the computed 
 * bitrate depends very much on the accuracy of the average frame 
 * size.
 */
int16
MP_EstimateBitrate(TMpTransportHandle *tHandle, uint8 isVbr)
{
  int16 bitRate;

  if(!bit_rate(&tHandle->header) || isVbr)
  {
    FLOAT div;

    div = (FLOAT) ((version(&tHandle->header) == MPEG_PHASE2_LSF) ? 72 : 144);
    bitRate = (int16) (((frequency(&tHandle->header) / 1000.0f) * GetSlotSize(tHandle) / div) + 0.5f);
  }
  else
    bitRate = bit_rate(&tHandle->header);

  return (bitRate);
}

/*
 * Returns the length of the track in milliseconds.
 */
uint32
MP_FileLengthInMs(TMpTransportHandle *tHandle, int32 fileSize)
{
  FLOAT frames = fileSize / (FLOAT) GetSlotSize(tHandle);

  return (uint32) (frames * GetFrameTime(&tHandle->header) + 0.5f);
}

/*
 * Returns the byte offset corresponding to the specified seeking position.
 */
int32
MP_GetSeekOffset(TMpTransportHandle *tHandle, int32 seekPos)
{
  FLOAT numFrames;
  int32 seekValues[2], frameOffset, byte_offset;

  GetSeekOffset(tHandle, seekValues);

  frameOffset = seekValues[0];
  byte_offset = seekValues[1];
  
  numFrames = seekPos / (FLOAT) GetFrameTime(&tHandle->header);
  if(numFrames < frameOffset)
    frameOffset = 0;

  /*-- Total offset. --*/
  byte_offset = (int32) ((byte_offset * (numFrames - frameOffset)) + 0.5f);

  return (byte_offset);
}

/*
 * Returns the duration of each frame in milliseconds.
 */
int32
MP_GetFrameTime(TMpTransportHandle *tHandle)
{
  return (GetFrameTime(&tHandle->header));
}

/*
 * Checks if the specified mp3 stream is using free format 
 * i.e., bitrate is not specified in the header part
 * of the frame.
 *
 * Return TRUE if bitrate not specified, FALSE otherwise
 */
BOOL
IsMP3FreeFormat(TMpTransportHandle *tHandle)
{  
  return (bit_rate(&tHandle->header) == 0);
}

/**************************************************************************
  Title        : FillDataSlotTable

  Purpose      : Pre-computes (to avoid division operation during decoding) 
                 the payload size of layer III for all bitrates. This function
                 should be called once the start of 1st frame has been located.

  Usage        : y = FillDataSlotTable(tHandle)

  Input        : tHandle - file format parser

  Author(s)    : Juha Ojanpera
  *************************************************************************/

INLINE void
FillDataSlotTable(TMpTransportHandle *tHandle)
{
  int16 nSlots;
  const int16 *brTbl;
  
  brTbl = GetBitRateTable(&tHandle->header);

  /*
   * index 0 is free format and index 14 illegal bitrate.
   */
  for(int16 i = 1; i < 15; i++)
  {
    nSlots = (int16)((144 * brTbl[i]) / (frequency(&tHandle->header) / 1000.0f));

    if(version(&tHandle->header) == MPEG_PHASE2_LSF)
      nSlots >>= 1;

    nSlots = (int16) (nSlots - GetSideInfoSlots(tHandle) - 4);
    tHandle->SlotTable[i] = nSlots;
  }
}

/**************************************************************************
  Title        : SeekSync

  Purpose      : Seeks for a byte aligned sync word in the bitstream and
                 places the bitstream pointer right after the sync word.

  Usage        : y = SeekSync(mp, bs_mcu, bufMapper, execState)

  Input        : mp             - mp3 stream parameters
                 bs_mcu         - bitstream parameters
                 execState      - exec status of this function

  Output       : y :
                  SYNC_BITS_OUT - bit buffer need to be updated
                  SYNC_LOST     - start of next frame was not found
                  SYNC_FOUND    - OK to decode next frame

  Author(s)    : Juha Ojanpera
  *************************************************************************/

INLINE SEEK_STATUS
SeekSync(TMpTransportHandle *tHandle, TBitStream *bs_mcu, ExecState *execState)
{
#define PRESYNCBITS  (MP_SYNC_WORD_LENGTH + 8)
#define POSTSYNCBITS (MAX_MP_HEADER_SIZE + 1)

  uint32 hdr, bits_left;
  BOOL exitCheck = TRUE;
  int32 sync_cand, bits_read;

  bits_left = (BsSlotsLeft(bs_mcu) - 1) << 3;
  
  if(execState->execMode == GLITCH_FREE && bits_left > PRESYNCBITS)
  {
    bits_left -= (BsByteAlign(bs_mcu) + (bits_read = tHandle->syncInfo.sync_length));
    sync_cand = BsGetBits(bs_mcu, tHandle->syncInfo.sync_length);
  }
  else if(execState->execMode == GLITCH_FREE)
  {
    execState->execMode = GLITCH_FREE;

    return (SYNC_BITS_OUT);
  }
  else
  {
    sync_cand = (int32)execState->a0_u32[0];
    bits_read = (int32)execState->a0_u32[1];
  }
  
  while(exitCheck)
  {
    while(sync_cand != tHandle->syncInfo.sync_word && 
      bits_read < SYNC_THRESHOLD && bits_left)
    {
      bits_read++;
      bits_left--;
      sync_cand = (sync_cand << 1) & tHandle->syncInfo.sync_mask;
      sync_cand |= (int16) BsGetBits(bs_mcu, 1);
    }
    
    if(bits_read > SYNC_THRESHOLD)
      return (SYNC_LOST);
    else if(bits_left < POSTSYNCBITS)
    {
      execState->execMode = GLITCH0;
      execState->a0_u32[0] = sync_cand;
      execState->a0_u32[1] = bits_read;

      return (SYNC_BITS_OUT);
    }
    else
    {
      bits_read++;
      bits_left--;
      sync_cand = (sync_cand << 1) & tHandle->syncInfo.sync_mask;
      sync_cand |= (int16) BsGetBits(bs_mcu, 1);
      
      tHandle->mpeg25 = !(sync_cand & 0x1);
      
      /* Check the next frame header. */
      hdr = (!tHandle->mpeg25) << HEADER_BITS;
      tHandle->header.header = hdr | BsLookAhead(bs_mcu, HEADER_BITS);
      
      /* Detect false frame boundaries. */
      switch(tHandle->syncInfo.sync_status)
      {
        case LAYER3_STREAM:
          if(HEADER_MASK(tHandle->header.header) == HEADER_MASK(tHandle->headerOld.header) &&
            sfreq(&tHandle->header) != 3 && LAYER_MASK(tHandle->header.header) == 0x1)
          {
            if(tHandle->FreeFormatSlots == 0)
            {
              if(bit_rate(&tHandle->header))
                exitCheck = FALSE;
            }
            else exitCheck = FALSE;
          }
          break;
      
        case INIT_LAYER3_STREAM:
          if(!(version(&tHandle->header) && mp25version(&tHandle->header)))
          {
            if(sfreq(&tHandle->header) != 3 && LAYER_MASK(tHandle->header.header) == 0x1 && bit_rate_idx(&tHandle->header) != 15)
            {
              tHandle->headerOld.header = tHandle->header.header;
              tHandle->syncInfo.sync_status = LAYER3_STREAM;
              exitCheck = FALSE;
            }
          }
          break;

        default:
          break;
      }
    }
  }  

  execState->execMode = GLITCH_FREE;

  return (SYNC_FOUND);
}

/**************************************************************************
  Title        : FindFreeFormatSlotCount

  Purpose      : Determines the size of the payload of a free format stream.

  Usage        : y = FindFreeFormatSlotCount(mpDec, bs_mcu, execState)

  Input        : mpDec     - mp3 stream parameters
                 bs_mcu    - bitstream parameters
                 execState - exec status of this function

  Output       : y :
                  SYNC_BITS_OUT - bit buffer need to be updated
                  SYNC_LOST     - stream is undecodable
                  SYNC_FOUND    - OK to start playback

  Author(s)    : Juha Ojanpera
  *************************************************************************/

INLINE SEEK_STATUS
FreeFormat(TMpTransportHandle *tHandle, TBitStream *bs_mcu, ExecState *execState)
{
  int16 exitCheck;
  TMPEG_Header oldHeader;
  uint16 sync_cand;
  uint32 bits_read, bits_left;

  tHandle->FreeFormatSlots = 0;
  
  if(execState->execMode == GLITCH_FREE)
  {
    int16 nSlots, minBytes;

    nSlots = GetSideInfoSlots(tHandle);
    minBytes = (uint16) (nSlots + 7);
    if((BsSlotsLeft(bs_mcu) - 1) < (uint16) minBytes)
    {
      execState->execMode = GLITCH_FREE;
      return (SYNC_BITS_OUT);
    }

    /*-- Read 1st header. --*/
    decode_header(tHandle, bs_mcu);
    oldHeader.header = tHandle->header.header;

    /*-- Skip side info part. --*/
    BsSkipNBits(bs_mcu, nSlots << 3);

    sync_cand = (uint16) BsGetBits(bs_mcu, tHandle->syncInfo.sync_length);
    bits_read = tHandle->syncInfo.sync_length;
  }
  else
  {
    sync_cand = (uint16) execState->a0_u32[0];
    bits_read = (uint32) execState->a0_u32[1];
    oldHeader.header = (uint32)execState->a0_u32[2];
  }

  exitCheck = 1;
  bits_left = (BsSlotsLeft(bs_mcu) - 1) << 3;
  //mask = (uint16) ((1 << (tHandle->syncInfo.sync_length - 1)) - 1);
    
  while(exitCheck)
  {
    while(sync_cand != tHandle->syncInfo.sync_word && bits_left)
    {
      bits_read++;
      bits_left--;
      sync_cand = (uint16) ((sync_cand << 1) & tHandle->syncInfo.sync_mask);
      sync_cand |= (uint16) BsGetBits(bs_mcu, 1);
    }

    if(bits_left < (HEADER_BITS + 1))
    {
      execState->a0_u32[0] = sync_cand;
      execState->a0_u32[1] = bits_read;
      execState->a0_u32[2] = oldHeader.header;
      execState->execMode = GLITCH0;

      return (SYNC_BITS_OUT);
    }
    else
    {
      uint32 hdr;

      bits_read++;
      bits_left--;
      sync_cand = (sync_cand << 1) & tHandle->syncInfo.sync_mask;
      sync_cand |= (int16) BsGetBits(bs_mcu, 1);
      
      tHandle->mpeg25 = !(sync_cand & 0x1);
      
      /* Check the next frame header. */
      hdr = (!tHandle->mpeg25) << HEADER_BITS;
      tHandle->header.header = hdr | BsLookAhead(bs_mcu, HEADER_BITS);
      
      /* 
       * Detect false frame boundraries. We could use header macros here
       * to speed up the comparison but since this function is called only
       * once (before playback/editing starts) the advantages of macros are
       * negligible.
       */
      if(channels(&tHandle->header)         == channels(&oldHeader) &&
         sfreq(&tHandle->header)            == sfreq(&oldHeader) &&
         bit_rate(&tHandle->header)         == bit_rate(&oldHeader) &&
         layer_number(&tHandle->header)     == layer_number(&oldHeader) &&
         version(&tHandle->header)          == version(&oldHeader) &&
         mode(&tHandle->header)             == mode(&oldHeader) &&
         error_protection(&tHandle->header) == error_protection(&oldHeader))
         exitCheck = 0;

      /* The payload size cannot be determined. */
      else if(bits_read > SYNC_THRESHOLD)     
        return (SYNC_LOST);
    }
  }
    
  /*-- Determine the size of the payload data. --*/
  if(bits_read != 0)
  {
    bits_read -= (tHandle->syncInfo.sync_length + 1);
    tHandle->FreeFormatSlots = (int16) (bits_read >> 3);

    if(padding(&oldHeader))
      tHandle->FreeFormatSlots -= 1;
  }
  else
  {
    tHandle->FreeFormatSlots = -1;

    return (SYNC_LOST);
  }
  
  execState->execMode = GLITCH_FREE;
  tHandle->transportType = GET_MPSYNC_STREAM;
  tHandle->syncInfo.sync_status = INIT_LAYER3_STREAM;

  return (SYNC_FOUND);
}

/**************************************************************************
  Title        : mpSyncTransport

  Purpose      : Sync layer interface for MPEG Layer I/II/III file formats.

  Usage        : y = mpSyncTransport(tHandle, syncBuf, syncBufLen, readBits)

  Input        : tHandle        - handle to MPEG Layer I/II/III parser
                 syncBuf        - handle to sync layer buffer
                 syncBufLen     - # of bytes present in sync layer buffer
  
  Output       : y              - status of operation
                  SYNC_BITS_OUT - the sync layer buffer needs to be updated
                  SYNC_LOST     - function failed
                  SYNC_FOUND    - sync layer processing successfull
                 readBits       - # of bits read from sync layer buffer

  Author(s)    : Juha Ojanpera
  *************************************************************************/


SEEK_STATUS
mpSyncTransport(TMpTransportHandle *tHandle, uint8 *syncBuf, 
                uint32 syncBufLen, uint32 *readBits)
{
  BOOL hitExit;
  ExecState *execState;
  TBitStream m_Bitstream;
  SEEK_STATUS frameStatus;

  *readBits = 0;
  hitExit = FALSE;
  frameStatus = SYNC_LOST;
  execState = &tHandle->execState;

  BsInit(&m_Bitstream, syncBuf, syncBufLen);

  if(tHandle->offsetBits)
    BsSkipNBits(&m_Bitstream, tHandle->offsetBits);
  tHandle->offsetBits = 0;

  while(hitExit == FALSE)
  {
    uint32 tmp;
    uint8 syncByte(0);

    switch(tHandle->transportType)
    {
      case INIT_MP_STREAM:
        frameStatus = SYNC_LOST;
        tHandle->transportType = GET_1ST_MPSYNC_STREAM;
        break;

      case GET_1ST_MPSYNC_STREAM:
        /*
         * Locate sync and on succes, switch to read
         * the headers values.
         */
        frameStatus = SeekSync(tHandle, &m_Bitstream, execState);
        if((frameStatus == SYNC_FOUND) && IsMP3FreeFormat(tHandle))
        {
          hitExit = TRUE;
          frameStatus = SYNC_MP3_FREE;
          tHandle->offsetBits = (int16) (BsGetBitsRead(&m_Bitstream) & 0x7);
        }
        else if(frameStatus == SYNC_FOUND)
          tHandle->transportType = GET_MPHEADER_STREAM;
        else
        {
          hitExit = TRUE;
          if(frameStatus != SYNC_LOST)
          {
            /*
             * The sync search locates the syncword which is at the
             * start of the header (1st 12 bits). Also in order to 
             * make the search reliable, the rest of the header bits
             * are also checked via lookahead by the sync routine.
             * Because of this it is possible that the input buffer
             * may run out of bits in the lookahead part. In that case
             * the input buffer needs to be updated. The number of bytes
             * to be updated are calculated based on the number of read 
             * bits. Thus, in the worst case, the update process will 
             * throw away the 1st 8 bits from the syncword. This is 
             * something we don't want to experience since subsequent 
             * processing relies on the fact that the whole header can 
             * be read from the input buffer once the start of the frame 
             * has been found. That's why we reduce the update size of the 
             * input buffer (shown below).
             */
            tmp = BsGetBitsRead(&m_Bitstream);
            syncByte = syncBuf[MAX(0, ((int32) (tmp >> 3) - 1))];
            if((tmp & 0x7) && syncByte == 0xFF)
            {
              /*-- Keep the previous byte in the buffer as it may be part of header. --*/  
              tHandle->offsetBits = (int16) (8 + (tmp & 0x7));

              tmp -= 8;
              BsClearBitsRead(&m_Bitstream);
              BsSetBitsRead(&m_Bitstream, tmp);
            }
            else tHandle->offsetBits = (int16) (tmp & 0x7);
          }
        }
        break;
    
      case GET_MPSYNC_STREAM:
        /*
         * Locate sync and on succes, switch to read
         * the headers values.
         */
        frameStatus = SeekSync(tHandle, &m_Bitstream, execState);
        if(frameStatus == SYNC_FOUND)
          tHandle->transportType = GET_MPHEADER_STREAM;
        else
        {
          hitExit = TRUE;
          if(frameStatus != SYNC_LOST)
          {
            /*-- See explanation above. --*/
            tmp = BsGetBitsRead(&m_Bitstream);
            syncByte = syncBuf[MAX(0, ((int32) (tmp >> 3) - 1))];
            if((tmp & 0x7) && syncByte == 0xFF)
            {
              /*-- Keep the previous byte in the buffer as it may be part of header. --*/
              tHandle->offsetBits = (int16) (8 + (tmp & 0x7));

              tmp -= 8;
              BsClearBitsRead(&m_Bitstream);
              BsSetBitsRead(&m_Bitstream, tmp);
            }
            else tHandle->offsetBits = (int16) (tmp & 0x7);
          }
        }
        break;
    
      case GET_MPHEADER_STREAM:
        /*
         * Read headers values and on success, switch the state back
         * to sync search for next frame.
         */
        hitExit = TRUE;
        frameStatus = decode_header(tHandle, &m_Bitstream);
        if(frameStatus == SYNC_FOUND)
          tHandle->transportType = GET_MPSYNC_STREAM;
        else if(frameStatus != SYNC_LOST)
        {
          /*-- Don't loose the syncword... --*/
          tmp = BsGetBitsRead(&m_Bitstream);
          syncByte = syncBuf[MAX(0, ((int32) (tmp >> 3) - 1))];
          if((tmp & 0x7) && syncByte == 0xFF)
          {
            /*-- Keep the previous byte in the buffer as it may be part of header. --*/
            tHandle->offsetBits = (int16) (8 + (tmp & 0x7));

            tmp -= 8;
            BsClearBitsRead(&m_Bitstream);
            BsSetBitsRead(&m_Bitstream, tmp);
          }
          else tHandle->offsetBits = (int16) (tmp & 0x7);
        }
        break;
    
      default:
        hitExit = TRUE;
        frameStatus = SYNC_LOST;
        break;
    }
  }

  *readBits = BsGetBitsRead(&m_Bitstream);
  
  return (frameStatus);
}

/*
 * Returns the # of bytes reserved for the Layer I/II/III payload part of current frame.
 */
INLINE int16
mpGetTranportFrameLength(TMpTransportHandle *tHandle)
{
  int16 frameBytes;

  tHandle->mainDataSlots = 0;

  switch(tHandle->transportType)
  {
    case GET_MPSYNC_STREAM:
    case GET_MPHEADER_STREAM:
      tHandle->mainDataSlots = (int16) main_data_slots(tHandle);
      frameBytes = (int16) (tHandle->mainDataSlots + GetSideInfoSlots(tHandle));
      break;
      
    default:
      frameBytes = 0;
      break;
  }
  
  return (frameBytes);
}

/*
 * Initializes MPEG Layer I/II/III transport handle.
 */
void
mpInitTransport(TMpTransportHandle *tHandle)
{
  ZERO_MEMORY(tHandle, sizeof(TMpTransportHandle));

  tHandle->syncInfo.sync_word = (int16) SYNC_WORD;
  tHandle->syncInfo.sync_length = (int16) MP_SYNC_WORD_LENGTH;
  tHandle->syncInfo.sync_mask = (uint16) ((1 << tHandle->syncInfo.sync_length) - 1);
  tHandle->syncInfo.sync_status = INIT_LAYER3_STREAM;
  tHandle->transportType = INIT_MP_STREAM;
  tHandle->execState.execMode = GLITCH_FREE;
  tHandle->offsetBits = 0;
}

/**************************************************************************
  Title        : MP_SeekSync

  Purpose      : Interface to layer I/II/III frame search implementation.

  Usage        : y = MP_SeekSync(tHandle, syncBuf, syncBufLen, readBytes, 
                                 frameBytes, headerBytes, initMode)

  Input        : tHandle     - layer I/II/III transport handle
                 syncBuf     - input buffer
                 syncBufLen  - length of 'sync_buf'
                 initMode    - '1' when searching the 1st frame, '0' otherwise

  Output       : y           - status of operation
                 frameBytes  - # of bytes reserved for the payload part of the frame
                 readBytes   - # of bytes read from the buffer
                 headerBytes - # of bytes reserved for the header part of the frame

  Author(s)    : Juha Ojanpera
  *************************************************************************/

int16
MP_SeekSync(TMpTransportHandle *tHandle, uint8 *syncBuf, uint32 syncBufLen, 
            int16 *readBytes, int16 *frameBytes, int16 *headerBytes,
            uint8 initMode)
{
  uint32 readBits;
  SEEK_STATUS syncSeekStatus;
 
  syncSeekStatus = mpSyncTransport(tHandle, syncBuf, syncBufLen, &readBits);
  if(initMode && syncSeekStatus == SYNC_FOUND)
    FillDataSlotTable(tHandle);

  *readBytes = (int16) (readBits >> 3);
  *frameBytes = (int16) mpGetTranportFrameLength(tHandle);
  *headerBytes = (int16) ((error_protection(&tHandle->header) ? 2 : 0) + 4);

  return (int16) (syncSeekStatus);
}

/**************************************************************************
  Title        : MP_FreeMode

  Purpose      : Interface for determining the frame/payload size of free format
                 layer III bitstream.

  Usage        : y = MP_FreeMode(tHandle, syncBuf, syncBufLen, readBytes, 
                                 frameBytes, headerBytes)

  Input        : tHandle     - layer I/II/III transport handle
                 syncBuf     - input buffer
                 syncBufLen  - length of 'sync_buf'

  Output       : y           - status of operation
                 frameBytes  - # of bytes reserved for the payload part of the frames
                 readBytes   - # of bytes read from the buffer
                 headerBytes - # of bytes reserved for the header part of the frame

  Author(s)    : Juha Ojanpera
  *************************************************************************/

int16
MP_FreeMode(TMpTransportHandle *tHandle, uint8 *syncBuf, uint32 syncBufLen, 
            int16 *readBytes, int16 *frameBytes, int16 *headerBytes)
{
  TBitStream m_Bitstream;
  SEEK_STATUS syncSeekStatus;

  BsInit(&m_Bitstream, syncBuf, syncBufLen);

  if(tHandle->offsetBits)
    BsSkipNBits(&m_Bitstream, tHandle->offsetBits);
  tHandle->offsetBits = 0;
 
  syncSeekStatus = FreeFormat(tHandle, &m_Bitstream, &tHandle->execState);
  if(syncSeekStatus == SYNC_BITS_OUT)
    tHandle->offsetBits = (int16) (BsGetBitsRead(&m_Bitstream) & 0x7);

  *readBytes = (int16) (BsGetBitsRead(&m_Bitstream) >> 3);
  *frameBytes = (int16) mpGetTranportFrameLength(tHandle);
  *headerBytes = (int16) ((error_protection(&tHandle->header) ? 2 : 0) + 4);

  return (int16) (syncSeekStatus);
}
