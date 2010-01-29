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
  \brief  Envelope extraction prototypes $Revision: 1.2.4.1 $
*/

/**************************************************************************
  env_extr.h - SBR bitstream demultiplexer interface + constants.
 
  Author(s): Juha Ojanpera
  Copyright (c) 2004 by Nokia Research Center, Multimedia Technologies.
  *************************************************************************/

#ifndef SBR_BITDEMUX_H_
#define SBR_BITDEMUX_H_

/*-- Project Headers. --*/
#include "sbr_codec.h"

#define SBR_FREQ_SCALE_DEF       (2)
#define SBR_ALTER_SCALE_DEF      (1)
#define SBR_NOISE_BANDS_DEF      (2)

#define SBR_LIMITER_BANDS_DEF    (2)
#define SBR_LIMITER_GAINS_DEF    (2)
#define SBR_INTERPOL_FREQ_DEF    (1)
#define SBR_SMOOTHING_LENGTH_DEF (1)

#define SBR_AMP_RES_1_5          (0)
#define SBR_AMP_RES_3_0          (1)

#define FIXFIX                   (0)
#define FIXVAR                   (1)
#define VARFIX                   (2)
#define VARVAR                   (3)

#define SBR_UPSAMPLE_FAC         (2)
#define NO_SYNTHESIS_CHANNELS    (64)
#define NO_ANALYSIS_CHANNELS     (NO_SYNTHESIS_CHANNELS / SBR_UPSAMPLE_FAC)
#define MAX_NOISE_ENVELOPES      (2)
#define MAX_NOISE_COEFFS         (8)
#define MAX_NUM_NOISE_VALUES     (MAX_NOISE_ENVELOPES * MAX_NOISE_COEFFS)
#define MAX_NUM_LIMITERS         (12)
#define MAX_ENVELOPES            (8)
#define MAX_FREQ_COEFFS          (48)
#define MAX_FREQ_COEFFS_FS44100  (35)
#define MAX_FREQ_COEFFS_FS48000  (32)
#define MAX_NUM_ENVELOPE_VALUES  (MAX_ENVELOPES * MAX_FREQ_COEFFS)
#define MAX_INVF_BANDS           (MAX_NOISE_COEFFS)
#define SBR_PARAMETRIC_STEREO_ID (2)

/**
 * SBR header status.
 */
typedef enum
{
  HEADER_OK,
  HEADER_RESET,
  CONCEALMENT,
  HEADER_NOT_INITIALIZED

} SBR_HEADER_STATUS;

/**
 * SBR codec status.
 */
typedef enum
{
  SBR_NOT_INITIALIZED,
  UPSAMPLING,
  SBR_ACTIVE

} SBR_SYNC_STATE;

/**
 * SBR coupling modes.
 */
typedef enum
{
  COUPLING_OFF,
  COUPLING_LEVEL,
  COUPLING_BAL

} COUPLING_MODE;

/**
 * Frequency scale tables for SBR.
 */
typedef struct FreqBandDataStr
{
  uint8 nSfb[2];
  uint8 nNfb;
  uint8 numMaster;
  uint8 noLimiterBands;
  uint8 nInvfBands;
  uint8 v_k_master[MAX_FREQ_COEFFS + 1];

} FreqBandData;

/**
 * SBR header element.
 */
typedef struct SbrHeaderDataStr
{
  SBR_SYNC_STATE syncState;
  uint8 numberTimeSlots;
  uint8 timeStep;
  uint16 codecFrameSize;
  int32 outSampleRate;

  uint8 ampResolution;

  uint8 startFreq;
  uint8 stopFreq;
  uint8 xover_band;
  uint8 freqScale;
  uint8 alterScale;
  uint8 noise_bands;

  uint8 limiterBands;
  uint8 limiterGains;
  uint8 interpolFreq;
  uint8 smoothingLength;

  FreqBandData *hFreqBandData;

} SbrHeaderData;

/**
 * SBR frame info element.
 */
typedef struct FRAME_INFOStr
{
  uint8 frameClass;
  uint8 nEnvelopes;
  uint8 freqRes;
  uint8 nNoiseEnvelopes;

} FRAME_INFO;

/**
 * SBR grid info element.
 */
typedef struct SbrGridInfoStr
{
  uint8 bs_num_env;

  uint8 bs_pointer;
  uint8 bs_num_rel[2];
  uint8 bs_var_board[2];
  uint8 bs_rel_board_0[4];
  uint8 bs_rel_board_1[4];

} SbrGridInfo;

/**
 * SBR frame data element.
 */
typedef struct SbrFrameDataStr
{
  uint8 dataPresent;

  FRAME_INFO frameInfo;
  SbrGridInfo sbrGridInfo;

  uint8 domain_vec;
  uint8 domain_vec_noise;

  uint16 sbr_invf_mode;
  COUPLING_MODE coupling;

  uint8 isSinesPresent;
  uint32 addHarmonics[2];

  int8 iEnvelope[MAX_NUM_ENVELOPE_VALUES];
  int8 sbrNoiseFloorLevel[MAX_NUM_NOISE_VALUES];

} SbrFrameData;

/**
 * SBR extension data element.
 */
typedef struct SbrExtensionDataStr
{
  uint8 writePsData;
  uint8 extensionDataPresent;
  int16 byteCount;
  uint8 extension_id;
  int16 extDataBufLen;
  uint8 *extensioData;

} SbrExtensionData;

/**
    * Reads SBR single channel element.
    *
  * @param hHeaderData  Handle to SBR header data
  * @param hFrameData   Handle to SBR frame data
  * @param sbrExtData   Handle to SBR extension data
  * @param bs           Input bitstream
  * @param decVal       Volume level adjustment factor
  * @param isMono       1 if mono SBR bitstream, 0 otherwise
  * @return             Error code, 0 on success
    * 
    */
int16
sbrGetSCE(SbrHeaderData *hHeaderData,
          SbrFrameData  *hFrameData,
          SbrExtensionData *sbrExtData,
          TBitStream *bs,
          int16 decVal,
          uint8 isMono);

/**
    * Reads SBR channel pair element.
    *
  * @param hHeaderData     Handle to SBR header data
  * @param hFrameDataLeft  Handle to left channel SBR frame data
  * @param hFrameDataRight Handle to right channel SBR frame data
  * @param sbrExtData      Handle to SBR extension data
  * @param bs              Input bitstream
  * @param decVal          Volume level adjustment factor
  * @return                Error code, 0 on success
    * 
    */
int16 
sbrGetCPE(SbrHeaderData *hHeaderData,
          SbrFrameData *hFrameDataLeft,
          SbrFrameData *hFrameDataRight,
          SbrExtensionData *sbrExtData,
          TBitStream *bs,
          int16 decVal);

/**
    * Reads SBR header element.
    *
  * @param hHeaderData  Handle to SBR header data
  * @param bs           Input bitstream
  * @return             Status of header processing, see status codes
    * 
    */
SBR_HEADER_STATUS
sbrGetHeaderData(SbrHeaderData *h_sbr_header,
                 TBitStream *bs);

/**
    * Initalizes SBR header element.
    *
  * @param hHeaderData     Handle to SBR header data
  * @param FreqBandData    Handle to SBR frequency scale data
  * @param sampleRate      Sampling rate of AAC bitstream
  * @param samplesPerFrame Number of samples in a frame (1024 or 960)
    * 
    */
void
initHeaderData(SbrHeaderData *headerData, 
               FreqBandData *FreqBandData,
               int32 sampleRate, 
               int16 samplesPerFrame);

/**
    * Initalizes SBR frequency scale tables.
    *
  * @param hHeaderData     Handle to SBR header data
  * @return                Error code, 0 on success
    * 
    */
int16
resetFreqBandTables(SbrHeaderData *hHeaderData);

FLOAT 
FloatFR_logDualis(int16 a);

FLOAT 
FloatFR_getNumOctaves(int16 a, int16 b);

#endif /*-- SBR_BITDEMUX_H_ --*/
