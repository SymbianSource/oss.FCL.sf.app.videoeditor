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
  \brief SBR codec interface $Revision: 1.1.1.1.4.1 $
*/

/**************************************************************************
  sbr_codec.h - SBR codec interface.
 
  Author(s): Juha Ojanpera
  Copyright (c) 2004 by Nokia Research Center, Multimedia Technologies.
  *************************************************************************/

#ifndef SBR_CODEC_H_
#define SBR_CODEC_H_

/*-- Project Headers. --*/
#include "nok_bits.h"
#include "defines.h"

#pragma warning( disable : 4244)

#define SBR_EXTENSION     (13)  /*-- 1101 --*/
#define SBR_EXTENSION_CRC (14)  /*-- 1110 --*/
#define MAX_NR_ELEMENTS   (2)
#define MAX_SBR_BYTES     (128)

/**
 * Error codes for SBR processing.
 */
typedef enum
{
  SBRDEC_OK = 0,
  SBRDEC_CONCEAL,
  SBRDEC_NOSYNCH,
  SBRDEC_ILLEGAL_PROGRAM,
  SBRDEC_ILLEGAL_TAG,
  SBRDEC_ILLEGAL_CHN_CONFIG,
  SBRDEC_ILLEGAL_SECTION,
  SBRDEC_ILLEGAL_SCFACTORS,
  SBRDEC_ILLEGAL_PULSE_DATA,
  SBRDEC_MAIN_PROFILE_NOT_IMPLEMENTED,
  SBRDEC_GC_NOT_IMPLEMENTED,
  SBRDEC_ILLEGAL_PLUS_ELE_ID,
  SBRDEC_CREATE_ERROR,
  SBRDEC_NOT_INITIALIZED

} SBR_ERROR;

/**
 * SBR element tags, these are the same as used in AAC also.
 */
typedef enum
{
  SBR_ID_SCE = 0,
  SBR_ID_CPE,
  SBR_ID_CCE,
  SBR_ID_LFE,
  SBR_ID_DSE,
  SBR_ID_PCE,
  SBR_ID_FIL,
  SBR_ID_END

} SBR_ELEMENT_ID;

/**
 * Bitstream element for SBR payload data. 
 */
typedef struct
{
  /**
   * Length of payload data.
   */
  int16 Payload;

  /**
   * Channel element ID for the associated SBR data.
   */
  int16 ElementID;

  /**
   * Offset to the start of the channel element, in bits.
   */
  int32 elementOffset;

  /**
   * Length of the channel element, in bits.
   */
  int32 chElementLen;

  /**
   * Decoding status of SBR header.
   */
  uint8 headerStatus;

  /**
   * Type of SBR element (with or without CRC codeword).
   */
  int16 ExtensionType;

  /**
   * SBR bitstream data.
   */
  uint8 *Data;

} SbrElementStream;

typedef struct
{
  int16 NrElements;
  SbrElementStream sbrElement[MAX_NR_ELEMENTS];

} SbrBitStream;

/**
 * SBR handle.
 */
typedef struct SBR_Decoder_Instance SBR_Decoder;

/**
    * Creates SBR bitstream handle.
    * 
    * Possible leave codes:
    *    - <code>KErrNoMemory</code> if memory allocation fails
    *
  * @return  Handle to SBR bitstream handle
    *
    */
SbrBitStream *
OpenSBRBitStreamL(void);

/**
    * Closes SBR bitstream handle.
  *
    * @param Bitstr  Handle to SBR bitstream to be deleted
  * @return        NULL
    *
    */
SbrBitStream *
CloseSBRBitStream(SbrBitStream *Bitstr);

/**
    * Creates SBR decoding/parsing handle.
    * 
    * Possible leave codes:
    *    - <code>KErrNoMemory</code> if memory allocation fails
    *
  * @param sampleRate      Sample rate of the AAC bitstream
  * @param samplesPerFrame Frame length of the AAC stream (1024 or 960)
  * @param isStereo        1 if stereo AAC stream, 0 otherwise (=mono)
  * @param isDualMono      1 if two single channel elements present in the AAC bistream, 0 otherwise
  * @return                Handle to SBR decoding/parsing handle
    * 
    */
SBR_Decoder *
OpenSBRDecoderL(int32 sampleRate, 
                int16 samplesPerFrame, 
                uint8 isStereo, 
                uint8 isDualMono);

/**
    * Closes SBR decoding handle.
  *
    * @param Bitstr  Handle to SBR decoding/parsing to be deleted
  * @return        NULL
    *
    */
SBR_Decoder *
CloseSBR(SBR_Decoder *self);

/**
    * Parses the SBR payload data and writes the modified bitstream element(s)
  * to output AAC bitstream.
    *
  * @param bsIn    Handle to AAC input bitstream
  * @param bsOut   Handle to AAC output bitstream
  * @param self    Handle to SBR parser
  * @param Bitstr  Handle to SBR bitstream element(s)
  * @param decVal  Volume level adjustment factor  
  * @return        Number of bytes written to output bitstream
    * 
    */
int16
ParseSBR(TBitStream *bsIn, 
         TBitStream *bsOut, 
         SBR_Decoder *self, 
         SbrBitStream *Bitstr,
         int16 decVal);

/**
    * Chacks whether parametric stereo tool is enabled in the SBR elements.
    *
  * @param self    Handle to SBR parser
  * @param Bitstr  Handle to SBR bitstream element(s)
  * @return        1 if parametric stereo found, 0 otherwise
    * 
    */
uint8
IsSBRParametricStereoEnabled(SBR_Decoder *self, SbrBitStream *Bitstr);

/**
    * Chacks whether SBR elements present in the bistream.
    *
  * @param Bitstr  Handle to SBR bitstream element(s)
  * @return        1 if SBR elements found, 0 otherwise
    * 
    */
uint8
IsSBREnabled(SbrBitStream *Bitstr);

/**
    * Reads SBR payload data from AAC bitstream into SBR bitstream handle.
    *
  * @param bs             Handle to AAC bitstream
  * @param streamSBR      Handle to SBR bitstream handle
  * @param extension_type SBR payload type
  * @param prev_element   Channel element type prior the fill element
  * @param dataCount      Length of SBR payload data, in bytes
  * @return               1 if SBR read, 0 otherwise (in which case the callee should read the data)
    * 
    */
int16
ReadSBRExtensionData(TBitStream *bs, 
                     SbrBitStream *streamSBR, 
                     int16 extension_type, 
                     int16 prev_element, 
                     int16 dataCount);

/**
    * Initializes SBR handle for silence data generation.
    *
  * @param sbrDec             Handle to SBR decoder
  * @param isStereo           1 if stereo AAC stream, 0 otherwise (=mono)
  * @param isParametricStereo 1 if parametric stereo should be included in the SBR data
    * 
    */
void
InitSBRSilenceData(SBR_Decoder *sbrDec, uint8 isStereo, uint8 isParametricStereo);

/**
    * Writes silence to SBR channel element. Please note that this method is supposed
  * to be called inside AAC bitstream multiplexer and is therefore part of encoding. 
  * If you want to upgrade plain AAC bitstream to eAAC+, use 'GenerateSBRSilenceDataL()' 
  * first to generate the silence bits for SBR, then parse the AAC bitstream (so that the 
  * channel element positions and their length are known) and finally call 
  * 'WriteSBRSilence()' to write the output eAAC+ bitstream. 
    *
  * @param sbrDecoder  Handle to SBR codec
  * @param bsOut       Handle to output (AAC) bitstream
  * @param isStereo    1 if stereo AAC stream, 0 otherwise (=mono)
  * @return            Length of SBR silence data, in bits
    * 
    */
int16
WriteSBRSilenceElement(SBR_Decoder *sbrDecoder, TBitStream *bsOut, uint8 isStereo);

/**
    * Generates silence bits for SBR bitstream.
    * 
    * Possible leave codes:
    *    - <code>KErrNoMemory</code> if memory allocation fails
    *
  * @param OutBuffer          Output buffer receiving silence data bits
  * @param OutBufferSize      Size of output buffer
  * @param sampleRate         Sample rate of the AAC bitstream
  * @param isStereo           1 if stereo AAC stream, 0 otherwise (=mono)
  * @param isParametricStereo 1 if parametric stereo should be included in the SBR data
  * @return                   Length of SBR silence data, in bits
    * 
    */
int16
GenerateSBRSilenceDataL(uint8 *OutBuffer, 
                        int16 OutBufferSize, 
                        int32 sampleRate, 
                        uint8 isStereo, 
                        uint8 isParametricStereo);

/**
    * Writes silence to the SBR part of the AAC bitstream.
    *
  * @param bsIn      AAC input bitstream
  * @param bsOut     Output bitstream (AAC + SBR)
  * @param streamSBR Handle to SBR bitstream element
  * @param SbrBuffer Payload data to generate SBR silence
  * @param SbrBits   Length of SBR silence, in bits
  * @return          Length of AAC + SBR bitstream, in bytes
    * 
    */
int16
WriteSBRSilence(TBitStream *bsIn, 
                TBitStream *bsOut,
                SbrBitStream *streamSBR,
                uint8 *SbrBuffer, 
                int16 SbrBits);

#endif /*-- SBR_CODEC_H_ --*/
