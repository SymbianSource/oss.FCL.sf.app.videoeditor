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
  mpif.h - Interchange format interface for MPEG Layer I/II/III.

  Author(s): Juha Ojanpera
  Copyright (c) 1999-2004 by Nokia Research Center, Speech and Audio Systems.
  *************************************************************************/

#ifndef MP_AIF_H_
#define MP_AIF_H_

/*-- Project Headers. --*/
#include "defines.h"
#include "auddef.h"
#include "mstream.h"
#include "MP3API.h"
#include "AACAPI.h"


/*
   Purpose:     Parent structure for error checking.
   Explanation: The number of bits that are protected varies between different 
                versions (MPEG-1, MPEG-2). */
/*
typedef struct CRC_Check
{
  uint8 bufLen;                       // Length of 'crc_payload' 
  int16 crc;                          // CRC error-check word.   

} CRC_Check;
*/



/*
 * Implemented interfaces.
 */
class TMpTransportHandle;

void
mpInitTransport(TMpTransportHandle *tHandle);

SEEK_STATUS
mpSyncTransport(TMpTransportHandle *tHandle, uint8 *syncBuf, 
                uint32 syncBufLen, uint32 *readBits);

int16
GetSideInfoSlots(TMpTransportHandle *tHandle);

int16
GetSideInfoSlots(TMPEG_Header *header);

int16
MP_SeekSync(TMpTransportHandle *tHandle, uint8 *syncBuf, 
            uint32 syncBufLen, int16 *readBytes, 
            int16 *frameBytes, int16 *headerBytes,
            uint8 initMode);

int16
MP_FreeMode(TMpTransportHandle *tHandle, uint8 *syncBuf, 
            uint32 syncBufLen, int16 *readBytes, 
            int16 *frameBytes, int16 *headerBytes);

int16
MP_EstimateBitrate(TMpTransportHandle *tHandle, uint8 isVbr);

uint32
MP_FileLengthInMs(TMpTransportHandle *tHandle, int32 fileSize);

int32
MP_GetSeekOffset(TMpTransportHandle *tHandle, int32 seekPos);

int32
MP_GetFrameTime(TMpTransportHandle *tHandle);



#endif /*-- MP_AIF_H_ --*/
