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


#ifndef MP3_DEF_H_
#define MP3_DEF_H_

/*- Project Headers. --*/
#include "defines.h"

/**************************************************************************
  External Objects Provided
  *************************************************************************/

/*-- General MPx Definitions. --*/
#ifndef PI
#define PI                   (3.14159265358979)
#endif
#define MPEG_AUDIO_ID        (1)
#define MPEG_PHASE2_LSF      (0)
#define SBLIMIT              (32)
#define SSLIMIT              (18)
#define MAX_MONO_SAMPLES     (SBLIMIT * SSLIMIT)
#define HAN_SIZE             (512)
#define NUM_SUBWIN           (16)
#define SCALE                (32768L)
#define SYNC_WORD            ((long) 0x7ff)
#define SYNC_WORD_LENGTH     (11)
#define HEADER_BITS          (20)
#define MAX_LONG_SFB_BANDS   (22)
#define MAX_SHORT_SFB_BANDS  (13)
#define MIN_MP3FRAMELEN      (1440)
#define MAX_BITRESER_SIZE    (512)
#define CRC_MAX_PAYLOAD      (34)

/*-- MPEG Header Definitions - Mode Values --*/
#define MPG_MD_STEREO        (0)
#define MPG_MD_JOINT_STEREO  (1)
#define MPG_MD_DUAL_CHANNEL  (2)
#define MPG_MD_MONO          (3)

/*-- Some Useful Macros. --*/
#ifdef MIN
#undef MIN
#endif
#define MIN(A, B)            ((A) < (B) ? (A) : (B))

#ifdef MAX
#undef MAX
#endif
#define MAX(A, B)            ((A) > (B) ? (A) : (B))

/*-- Channel definitions. --*/
#define MONO_CHAN            (0)
//#define MAX_CHANNELS         (2)
#define LEFT_CHANNEL         (MONO_CHAN)
#define RIGHT_CHANNEL        (MONO_CHAN + 1)

/*
   Purpose:     Masks those bit fields from the header to zero that
                do not remain fixed from frame to frame.
   Explanation: Following fields are assumed to be fixed :
                 * 12th bit from the sync word
                 * version
                 * layer description
                 * sampling rate
         * channel mode (layer 3 only)
         * copyright bit
                 * original bit
                 * de-emphasis

                Following fields can vary from frame to frame :
         * protection bit
                 * bit rate
                 * padding bit
                 * private bit
                 * channel mode extension
                */
#define HEADER_MASK(header) ((uint32)header & 0x001E0CCF)

/*
   Purpose:     Macro to extract layer description.
   Explanation: This is the bit value, use MP_Header::layer_number method
                to interpret this value. */
#define LAYER_MASK(header) (((uint32)header >> 17) & 3)

/*
   Purpose:     Layer III flags.
   Explanation: - */
typedef enum LayerIIIFlags
{
  WINDOW_SWITCHING_FLAG = 4,
  MIXED_BLOCK_FLAG = 8,
  PRE_FLAG = 16,
  SCALEFAC_SCALE = 32,
  COUNT_1_TABLE_SELECT = 64

} Layer_III_Flags;

/*
   Purpose:     Stereo modes for layer III.
   Explanation: - */
typedef enum StereoMode
{
  ONLY_MONO,
  ONLY_STEREO,
  MS_STEREO,
  IS_STEREO,
  LSF_IS_STEREO

} StereoMode;

/*
   Purpose:     Block types for layer III.
   Explanation: The first four describe the actual block type for each subband,
                the rest of the declarations describe the block type for the
                whole frame. */

typedef enum MP3_WINDOW_TYPE
{
  ONLY_LONG_WINDOW,
  LONG_SHORT_WINDOW,
  ONLY_SHORT_WINDOW,
  SHORT_LONG_WINDOW,
  MIXED_BLOCK_MODE,
  SHORT_BLOCK_MODE,
  LONG_BLOCK_MODE

} MP3_WINDOW_TYPE, MP3WindowType;

/*
   Purpose:     Structure to hold scalefactor band parameters.
   Explanation: - */
typedef struct SFBAND_DATA_STR
{
  int16 l[23]; /* long block.  */
  int16 s[14]; /* short block. */

} SFBAND_DATA;

/*
   Purpose:     Number of bits reserved for decoding each group
                of scalefactors.
   Explanation: - */
typedef struct SFBITS_DATA_STR
{
  int16 l[5];
  int16 s[3];
  
} SFBITS_DATA;

/*
   Purpose:     Sync seek code.
   Explanation: - */
typedef enum MIX_SYNC_STATUS
{
  LAYER1_STREAM_MIX,
  LAYER2_STREAM_MIX,
  LAYER3_STREAM_MIX,
  FIRST_FRAME_WITH_LAYER1,
  FIRST_FRAME_WITH_LAYER2,
  FIRST_FRAME_WITH_LAYER3

} MIX_SYNC_STATUS;

#endif /* MP3_DEF_H_ */
