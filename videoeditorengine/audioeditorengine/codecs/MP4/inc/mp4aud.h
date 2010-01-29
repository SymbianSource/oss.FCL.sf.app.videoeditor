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
  mp4aud.h - MPEG-4 Audio structures and interface methods.

  Author(s): Juha Ojanpera
  Copyright (c) 2001-2004 by Nokia Research Center, Speech and Audio Systems.
  *************************************************************************/

#ifndef MP4AUD_H_
#define MP4AUD_H_

/*-- Project Headers. --*/
#include "nok_bits.h"
#include "chandefs.h"

/*-- Raw bitstream constants. --*/
//#define LEN_PC_COMM       (8)

/*-- Program configuration element --*/
/*
#define LEN_BYTE          (8)
#define LEN_TAG           (4)
#define LEN_PROFILE       (2) 
#define LEN_SAMP_IDX      (4) 
#define LEN_NUM_ELE       (4) 
#define LEN_NUM_LFE       (2) 
#define LEN_NUM_DAT       (3) 
#define LEN_NUM_CCE       (4) 
#define LEN_MMIX_IDX      (2) 
#define LEN_PSUR_ENAB     (1) 
#define LEN_ELE_IS_CPE    (1)
#define LEN_IND_SW_CCE    (1)  
#define LEN_COMMENT_BYTES (8)
*/
/*
   Purpose:     MPEG-4 Audio object types.
   Explanation: - */
typedef enum AudioObjectType
{
  NULL_OBJECT = -1,
  AAC_MAIN,
  AAC_LC,
  AAC_SSR,
  AAC_LTP,
  AAC_SBR,
  AAC_SCALABLE,
  TWINVQ,
  CELP,
  HVXC,
  RESERVED10,
  RESERVED11,
  TTSI = 11,
  MAIN_SYNTHETIC,
  WAVETABLE_SYNTHESIS,
  GENERAL_MIDI,
  AUDIO_FX,
  ER_AAC_LC, 
  RESERVED18, 
  ER_AAC_LTP, 
  ER_AAC_SCALABLE, 
  ER_TWINVQ,
  ER_BSAC, 
  ER_AAC_LD, 
  ER_CELP, 
  ER_HVXC, 
  ER_HILN,
  ER_PARAMETRIC, 
  RESERVED28, 
  RESERVED29, 
  RESERVED30, 
  RESERVED31

} AudioObjectType;

/*
   Purpose:     Configuration information for MPEG-4 GA coder.
   Explanation: - */
typedef struct GaSpecificInfoStr
{
  /*-- Common to all object types. --*/
  BOOL FrameLengthFlag;
  BOOL DependsOnCoreCoder;
  int16 CoreCoderDelay;

  /*-- AAC Scalable parameters. --*/
  uint8 layerNr;

  /*-- ER BSAC parameters. --*/
  uint8 numOfSubframe;
  int16 layer_length;

  /*-- ER object specific data. --*/
  BOOL aacSectionDataResilienceFlag;
  BOOL aacScalefactorDataResilienceFlag;
  BOOL aacSpectralDataResilienceFlag;
  
  /*-- Future extension flag. --*/
  BOOL extensionFlag3;

} GaSpecificInfo;

/*
   Purpose:     Configuration information for MPEG-4 audio coder.
   Explanation: - */
typedef struct AudioSpecificInfoStr
{
  AudioObjectType audioObject; /* Object type.                            */
  uint8 samplingFreqIdx;       /* Index to sampling frequency table.      */
  int32 samplingFrequency;     /* Sampling frequency if not in the table. */
  uint8 channelConfiguration;  /* Index to channel configuration table.   */
  GaSpecificInfo gaInfo;       /* Config information for GA coder.        */

  AudioObjectType extAudioObject;
  int16 syncExtensionType;
  int32 extSamplingFrequency;
  uint8 extSamplingFreqIdx;
  uint8 sbrPresentFlag;

} AudioSpecificInfo;

/*
   Purpose:     Information about the audio channel.
   Explanation: - */
typedef struct 
{
  int16 num_ele;
  int16 ele_is_cpe[1 << LEN_TAG];
  int16 ele_tag[1 << LEN_TAG];

} EleList;

/*
   Purpose:     Mixing information for downmixing multichannel input
                into two-channel output.
   Explanation: - */
typedef struct 
{
  int16 present;
  int16 ele_tag;
  int16 pseudo_enab;

} MIXdown;

/*
   Purpose:     Program configuration element.
   Explanation: - */
typedef struct 
{
  int16 tag;
  int16 profile;
  int16 sample_rate_idx;

  BOOL pce_present;

  EleList front;
  EleList side;
  EleList back;
  EleList lfe;
  EleList data;
  EleList coupling;

  MIXdown mono_mix;
  MIXdown stereo_mix;
  MIXdown matrix_mix;

  int16 num_comment_bytes;
  char comments[(1 << LEN_PC_COMM) + 1];

} ProgConfig;

/*
   Purpose:     MPEG-4 AAC transport handle.
   Explanation: - */
typedef struct mp4AACTransportHandleStr
{
  ProgConfig progCfg;
  AudioSpecificInfo audioInfo;

} mp4AACTransportHandle;


/*-- Access methods. --*/

BOOL
ReadMP4AudioConfig(uint8 *buffer, uint32 bufLen, mp4AACTransportHandle *mp4AAC_ff);

void
WriteMP4AudioConfig(uint8 *buffer, uint32 bufLen, mp4AACTransportHandle *mp4AAC_ff);

#endif /*-- MP4AUD_H_ --*/
