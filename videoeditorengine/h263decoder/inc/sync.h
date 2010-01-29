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
* Sync codes.
*
*/


#ifndef _SYNC_H_
#define _SYNC_H_


#include "epoclib.h"

#include "biblin.h"


/*
 * Defines
 */

/* Return values */
#define SNC_NO_SYNC  0        /* No synchronization code found */
#define SNC_PSC      1        /* Picture Start Code */
#define SNC_GBSC     2        /* GOB Start Code */
#define SNC_EOS      4        /* End of Sequence */

/*** MPEG-4 REVISION ***/
#define SNC_VIDPACK  5        /* Video Packet resynchronization marker */
#define SNC_VOS      6        /* Video Object Sequence (VOS) Start Code */
#define SNC_VOP      7        /* Video Object Plane (VOP) Start Code */
#define SNC_GOV      8        /* Group of VOPs (GOV) Start Code */
#define SNC_USERDATA 9        /* User Data Start Code */
#define SNC_EOB      10       /* Visual Sequence End Code */
#define SNC_VID      11       /* Other Video Object Header Start Codes */
#define SNC_PATTERN  12       /* The search pattern in sncSeekBitPattern */
/*** End MPEG-4 REVISION ***/

#define SNC_STUFFING 13       /* Stuffing in the end of a buffer containing
                                 one frame */

/* See sncRewindAndSeekNewSync for description. */
#define SNC_DEFAULT_REWIND -1


/*
 * Function prototypes
 */

int sncCheckSync(bibBuffer_t *buffer, int *numStuffBits, int16 *error);

int sncRewindAndSeekNewSync(u_int32 numBitsToRewind, bibBuffer_t *buffer, 
   int16 *error);

int sncSeekFrameSync(bibBuffer_t *buffer, int16 *error);

int sncSeekSync(bibBuffer_t *buffer, int16 *error);

   int sncCheckMpegVOP(bibBuffer_t *buffer, int16 *error);
   int sncCheckMpegSync(bibBuffer_t *buffer, int f_code, int16 *error);
   int sncRewindAndSeekNewMPEGSync(int earliestBitPos, bibBuffer_t *buffer,
                                   int f_code, int16 *error);
   int sncSeekMPEGSync(bibBuffer_t *buffer, int f_code, int16 *error);
   int sncSeekMPEGStartCode(bibBuffer_t *buffer, int f_code, int skipVPSync, int checkUD, int16 *error);
   int sncSeekBitPattern(bibBuffer_t *buffer, u_int32 BitPattern, 
      int BitPatternLength, int16 *error);
   int sncRewindStuffing(bibBuffer_t *buffer, int16 *error);

#endif
// End of File
