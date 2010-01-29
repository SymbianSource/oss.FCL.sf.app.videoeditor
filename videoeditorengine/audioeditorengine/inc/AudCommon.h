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
* Common definitions for audio.
*
*/



#ifndef __AUDCOMMON_H__
#define __AUDCOMMON_H__

#include <e32std.h>


const TInt KErrNoAudio = -400;
/**
 * Enumeration for audio file formats.
 */
enum TAudFormat
    {
    EAudFormatUnrecognized = 100001,  // should always be the first one
    EAudFormat3GPP,
    EAudFormatMP4,
    EAudFormatAMR,
    EAudFormatAMRWB,
    EAudFormatMP3,
    EAudFormatAAC_ADIF,
    EAudFormatAAC_ADTS,
    EAudFormatWAV,
    EAudFormatLast  // should always be the last one
    };


/**
 * Enumeration for audio types (that is, codecs).
 */
enum TAudType
    {
    EAudTypeUnrecognized = 101001,  // should always be the first one
    EAudNoAudio,
    EAudAMR,
    EAudAMRWB,
    EAudMP3,
    EAudAAC_MPEG2, // EAudAAC_MPEG2 not used, all EAudAAC_MPEG is now 4
    EAudAAC_MPEG4,
    EAudWAV,
    EAudTypeLast  // should always be the last one
    };
    
/*
   Purpose:     MPEG-4 Audio object types.
   Explanation: - */
enum TAudAACObjectType
{
  EAudAACObjectTypeNone = -1,
  EAudAACObjectTypeMain,
  EAudAACObjectTypeLC,
  EAudAACObjectTypeSSR,
  EAudAACObjectTypeLTP,
  EAudAACObjectTypeLast
};
  
/**
 * Enumeration for audio extension types (AAC+)
 */
enum TAudExtensionType
    {
    EAudExtensionTypeNoExtension = 20000,
    EAudExtensionTypeEnhancedAACPlus,
    EAudExtensionTypeEnhancedAACPlusParametricStereo
    
    };    

/**
 * Constants for audio clip priorities.
 */
const TInt KAudClipPriorityHighest = 400;
const TInt KAudClipPriorityHigh = 300;
const TInt KAudClipPriorityNormal = 200;
const TInt KAudClipPriorityLow = 100;
const TInt KAudClipPriorityLowest = 0;


  


/**
 * Enumeration for channel modes
 */
enum TChannelMode 
    {
    EAudChannelModeNotRecognized = 103001,   
    EAudStereo,
    EAudDualChannel,
    EAudSingleChannel,
    EAudParametricStereoChannel
    };

/**
 * Enumeration for bitrate modes
 */
enum TBitrateMode 
    {
    EAudBitrateModeNotRecognized = 103001,
    EAudConstant,
    EAudVariable
    };
    
/**
 * Class for storing dynamic level marks.
 */
class TAudDynamicLevelMark 
    {

public:

    /** Mark time. */
    TTimeIntervalMicroSeconds iTime;

    /** Dynamic level (-63.5 ... +63.5) in dB:s
    * one step represents 0.5 dB, hence values can be -127 ... +127
    */
    TInt8 iLevel;

    TAudDynamicLevelMark(TTimeIntervalMicroSeconds aTime, TInt aLevel) 
        {
        iTime = aTime;
        if ( aLevel < -127 )
            {
            iLevel = -127;
            }
        else if (aLevel > 127 )
            {
            iLevel = 127;
            }
        else
            {
            iLevel = TInt8(aLevel);
            }
        }

    TAudDynamicLevelMark(const TAudDynamicLevelMark& aMark) 
        {
        iTime = aMark.iTime;
        iLevel = aMark.iLevel;
        }
    };    

/**
 * Class for storing general properties about an audio file.
 */
class TAudFileProperties 
    {

public:

    // File format
    TAudFormat iFileFormat;
    // Codec
    TAudType iAudioType;
    // Extension type
    TAudExtensionType iAudioTypeExtension;
    // Duration of the clip in microseconds
    TTimeIntervalMicroSeconds iDuration;
    // Sampling rate in Hertzes
    TInt iSamplingRate;
    // Bitrate in bits/s
    TInt iBitrate;
    // Channel mode
    TChannelMode iChannelMode;
    // Channel mode for extension
    TChannelMode iChannelModeExtension;
    // Bitrate mode (constant or variable)
    TBitrateMode iBitrateMode;
    // Number of bytes in a compressed frame
    TInt32 iFrameLen;
    // Is iFrameLen constant?
    TBool iConstantFrameLength;
    // Duration of decoded frame in microseconds
    TTimeIntervalMicroSeconds iFrameDuration;
    // Number of frames in the clip
    TInt32 iFrameCount;
//    TCodecMode iCodecMode;
    // Number of frames in a sample (e.g. amr in mp4 has several)
    TInt iNumFramesPerSample;
    // Bitdepth in PCM data (usually 8, 16 or 24, only for PCM-data)
    TInt iNumberOfBitsPerSample; 
    // Original field in MP3 fixed header
    TBool iOriginal;
    
    // AAC object type
    TAudAACObjectType iAACObjectType;

    TBool isCompatible(const TAudFileProperties /*&c2*/) const 
        {
        
        // Now that the clips can be decoded and encoded
        // all the clips are "compatible" with each other        
        
        return ETrue;


        }

    };    



#endif // __AUDCOMMON_H__

