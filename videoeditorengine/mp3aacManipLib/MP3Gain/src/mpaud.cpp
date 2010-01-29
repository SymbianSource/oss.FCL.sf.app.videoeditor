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
  mpaud.cpp - MPEG-1, MPEG-2 LSF and MPEG-2.5 layer III bitstream decoder.

  Author(s): Juha Ojanpera
  Copyright (c) 1999-2004 by Nokia Research Center, Speech and Audio Systems.
  *************************************************************************/

/*-- project headers --*/
#include "mpaud.h"
#include "mpheader.h"
#include "mp3tool.h"

/**************************************************************************
  Internal Objects
  *************************************************************************/

/**************************************************************************
  Title        : L3BitReservoir

  Purpose      : Layer III bit reservoir subroutine.

  Usage        : y = L3BitReservoir(mp)

  Input        : mp - mp3 stream parameters

  Output       : y - # of bytes discarded from the bit reservoir buffer.
                     Value -1 indicates that an error has occured during
                     decoding (invalid bitstream syntax found)

  Explanation  : Return value -1 should be regarded as a fatal error in the
                 sense that decoding of current frame should not be continued.
                 Instead, the decoder should be re-initialized.

  Author(s)    : Juha Ojanpera
  *************************************************************************/

INLINE int16
L3BitReservoir(CMPAudDec *mp)
{
  int16 bytes_to_discard;
  int16 flush_main;
  int32 bits_read;
  TBitStream *br;

  br = &mp->br;

  /*------------ Start of bit reservoir processing. ------------------*/

  if(mp->WasSeeking == FALSE)
  {
    /*-- Byte alignment. --*/
    bits_read = BsGetBitsRead(br);
    flush_main = (int16) (bits_read & 7);
    if(flush_main)
      BsSkipBits(br, (int16) (8 - flush_main));

    /*
     * Determine how many bits were left from the previous frame.
     */
    if(mp->SkipBr == FALSE)
    {
      BsClearBitsRead(br);
      BsSetBitsRead(br, (mp->PrevSlots << 3) - bits_read);
    }

    /*
     * Determine how many bytes need to be discarded from the previous
     * frame to find the start of next frame.
     */
    bytes_to_discard = (int16) ((BsGetBitsRead(br) >> 3) - mp->side_info->main_data_begin);
    
    /*-- Reset the bit reservoir bit counter. --*/
    BsClearBitsRead(br);
    
    /*-- # of slots available for this frame. --*/
    mp->PrevSlots = (int16) (mp->mpFileFormat->mainDataSlots + mp->side_info->main_data_begin);
    
    if(bytes_to_discard < 0)
    {
      BsClearBitsRead(br);
      BsSetBitsRead(br, mp->mpFileFormat->mainDataSlots << 3);
      mp->SkipBr = TRUE;
      return (-1);
    }

    mp->SkipBr = FALSE;

    if(bytes_to_discard)
    {
      mp->PrevSlots = (int16) (mp->PrevSlots + bytes_to_discard);
      BsSkipNBits(br, bytes_to_discard << 3);
    }
  }
  else
  {
    bytes_to_discard = 0;

    /*-- # of slots available for this frame. --*/
    mp->PrevSlots = (int16) (mp->mpFileFormat->mainDataSlots + mp->side_info->main_data_begin);

    mp->SkipBr = FALSE;
    mp->WasSeeking = FALSE;
    
    if(mp->side_info->main_data_begin)
      BsRewindNBits(br, mp->side_info->main_data_begin << 3);

    /*-- Reset the bit reservoir bit counter. --*/
    BsClearBitsRead(br);
  }
  /*-------------- End of bit reservoir processing. ------------------*/

  return (bytes_to_discard);
}

/**************************************************************************
  Title        : DeleteMPAudDec

  Purpose      : Releases resources allocated to the mp3 decoder core.

  Usage        : y = DeleteMPAudDec(mpAud)

  Input        : mpAud - Layer I/II/III decoder core

  Output       : y     - NULL

  Author(s)    : Juha Ojanpera
  *************************************************************************/


INLINE uint8
GetLayer3Version(TMpTransportHandle *tHandle)
{
  uint8 mp_version;
  
  /*
   * Which version of mp3 ?
   */
  if(version(&tHandle->header))
    mp_version = 1; /*-- MPEG-1   --*/
  else if(!mp25version(&tHandle->header))
    mp_version = 2; /*-- MPEG-2   --*/
  else
    mp_version = 3; /*-- MPEG-2.5 --*/

  return (mp_version);
}
