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
  auddef.h - Constants and general declarations for MPEG type of audio formats.

  Author(s): Juha Ojanpera
  Copyright (c) 1999-2004 by Nokia Research Center, Speech and Audio Systems.
  *************************************************************************/

#ifndef MPAUD_DEF_H_
#define MPAUD_DEF_H_

/*- Project Headers. --*/
#include "defines.h"


/**************************************************************************
  External Objects Provided
  *************************************************************************/

/*-- General MPx Definitions. --*/
/*
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

#define HEADER_BITS          (20)
#define MAX_LONG_SFB_BANDS   (22)
#define MAX_SHORT_SFB_BANDS  (13)
#define MAX_BITRESER_SIZE    (512)
#define CRC_MAX_PAYLOAD      (34)
*/
#define MP_SYNC_WORD_LENGTH  (11)
/*-- MPEG Header Definitions - Mode Values --*/
#define MPG_MD_STEREO        (0)
#define MPG_MD_JOINT_STEREO  (1)
#define MPG_MD_DUAL_CHANNEL  (2)
#define MPG_MD_MONO          (3)

/*-- Channel definitions. --*/
#define MONO_CHAN            (0)
#define MAX_CHANNELS         (2)
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
   Purpose:     Frame detection status.
   Explanation: - */
typedef enum SEEK_STATUS
{
  SYNC_FOUND = 0,
  SYNC_LOST,
  SYNC_BITS_OUT,
  SYNC_MP3_FREE

} SEEK_STATUS;

/*
   Purpose:     Sync layer codes.
   Explanation: - */
typedef enum SYNC_STATUS
{
  LAYER1_STREAM,
  LAYER2_STREAM,
  LAYER3_STREAM,

  INIT_LAYER1_STREAM,
  INIT_LAYER2_STREAM,
  INIT_LAYER3_STREAM,

  INIT_MP_STREAM,
  GET_1ST_MPSYNC_STREAM,
  GET_MPSYNC_STREAM,
  GET_MPHEADER_STREAM,

  INIT_AAC_STREAM,
  ADTS_STREAM,
  GET_ADTSSYNC_STREAM,
  GET_ADTSHEADER_STREAM

} SYNC_STATUS;

/*
   Purpose:     Bitrate modes.
   Explanation: - */
/*
typedef enum BrType
{
  UNKNOWN,
  CBR,
  VBR,
  FREE

} BrType;
*/
/*
   Purpose:     
   Explanation: - */
typedef enum GLITCH
{
  GLITCH_FREE,
  GLITCH0,
  GLITCH1

} GLITCH;

/*
   Purpose:     Message definitions.
   Explanation: - */
typedef enum MsgType
{
  NO_MESSAGES,
  UPDATE_BUFFER,
  SEEK_BUFFER,
  GET_POSITION,
  GET_SIZE
  
} MsgType;


/*
   Purpose:     Definition of generic message parameter.
   Explanation: - */
typedef uint32 MsgParam;

/*
   Purpose:     Message structure of the decoder and/or player.
   Explanation: - */
typedef struct MsgStr
{
  MsgType msgType;
  MsgParam msgInParam;
  MsgParam msgOutParam;
  
} Msg;

/*
   Purpose:     Execution state for user specified functions.
   Explanation: - */
typedef struct ExecStateStr
{
  GLITCH execMode;
  int16 a0_s16[3];
  uint32 a0_u32[3];
  Msg *msg;

} ExecState;

/*
   Purpose:     Parent structure for sync layer processing.
   Explanation: - */
typedef struct SyncInfoStr
{
  int16 sync_length;       /* Length of sync word.                    */
  int16 sync_word;         /* Synchronization word.                   */
  int16 sync_mask;         /* Bitmask for sync word detection.        */
  SYNC_STATUS sync_status; /* Which layer we supposed to be decoding. */

} SyncInfo;

enum
{
  VBR_MODE = 1,
  FILE_SIZE_KNOWN = 2
};

/*
   Purpose:     Frame detection status.
   Explanation: - */
/*

CAN BE FOUND IN auddef.h
typedef enum SEEK_STATUS
{
  SYNC_FOUND,
  SYNC_LOST,
  SYNC_BITS_OUT

} SEEK_STATUS;
*/
/*
   Purpose:     State of the decoder.
   Explanation: - */
typedef enum DecState
{
  STATE_UNDEFINED,
  DEC_INIT,
  DEC_INIT_COMPLETE,
  FIND_FIRST_FRAME,
  FIND_FRAME,
  FIND_PAYLOAD,
  FIND_AVERAGE_BR,
  DECODE_FRAME,
  CORRUPTED_FRAME,
  GET_PAYLOAD,
  RESTART_AFTER_VBR,
  DECODE_FRAME_CORRUPTED

} DecState;

/*
   Purpose:     Supported UI features.
   Explanation: - */
typedef enum UIMode
{
  UI_UNDEFINED,
  UI_PLAY,
  UI_STOP,
  UI_WIND_FORWARD,
  UI_WIND_BACKWARD,
  UI_REPEAT_LOOP

} UIMode;

/*
   Purpose:     A-B repeat.
   Explanation: - */
typedef enum MARKER_ID
{
  RESET_MARKERS = -1,
  A_MARKER,
  B_MARKER

} MARKER_ID;

/*
   Purpose:     Bitrate modes.
   Explanation: - */

typedef enum BrType
{
  UNKNOWN,
  CBR,
  VBR,
  FREE

} BrType;


/*
   Purpose:     Playback quality mappings.
   Explanation: - */
typedef enum QUALITY
{
  FULL_QUALITY,
  HALF_QUALITY,
  QUARTER_QUALITY

} QUALITY;

/*
   Purpose:     Error codes of the player.
   Explanation: - */
typedef enum MP3_ERROR
{
  AUDIO_OK = 0,
  AUDIO_ERROR_CRC,
  AUDIO_INVALID_LAYER,
  AUDIO_INVALID_SYNTAX,
  AUDIO_FREE_FORMAT_ERROR,
  AUDIO_FREE_FORMAT_BR_ERROR,
  AUDIO_BUFFER_TOO_SMALL

} MP3_Error;

/*
   Purpose:     General info about the file/stream.
   Explanation: - */
typedef struct TrackInfoStr
{
  int32 frequency;
  int16 bitRate;
  int16 numChannels;
  int32 lengthInms;
  uint32 numFrames;
  BrType brType;

} TrackInfo;

/*
   Purpose:     Playback quality parameters and general init info.
   Explanation: - */
typedef struct InitParamStr
{
  uint8 out_channels;
  uint8 decim_factor;
  uint8 window_pruning;
  uint8 alias_subbands;
  uint8 imdct_subbands;
  uint32 specFreqBinLimit;

  int16 bitrate;
  int32 VBRframesLimit;

} InitParam;




#endif /*-- MPAUD_DEF_H_ --*/
