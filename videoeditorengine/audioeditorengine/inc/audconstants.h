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
* Common internal constants for audio.
*
*/



#ifndef __AUDCONSTANTS_H__
#define __AUDCONSTANTS_H__



// CONSTANTS

const TInt KAedSampleRate8kHz = 8000;
const TInt KAedSampleRate11kHz = 11025;
const TInt KAedSampleRate16kHz = 16000;
const TInt KAedSampleRate22kHz = 22050;
const TInt KAedSampleRate24kHz = 24000;
const TInt KAedSampleRate32kHz = 32000;
const TInt KAedSampleRate44kHz = 44100;
const TInt KAedSampleRate48kHz = 48000;

const TInt KAedBitRateAMR = 12200;
const TInt KAedBitRateAAC16kHz = 48000;
const TInt KAedBitRateAAC48kHz = 192000;

const TInt KAedAACMinBitRateMultiplier = 1; // min bitrate is 1 bit per sample
const TInt KAedAACMaxBitRateMultiplier = 6; // max bitrate is 6 bits per sample

const TInt KAedAMRFrameDuration = 20000;

const TInt KAedSizeAACBuffer = 2048;
const TInt KAedSizeAACStereoBuffer = 4096;
const TInt KAedMaxFeedBufferSize = 4096;
const TInt KAedMaxAACFrameLengthPerChannel = 768;
const TInt KAedMaxAMRFrameLength = 32;
const TInt KAedMaxAWBFrameLength = 61;
const TInt KAedMaxMP3FrameLength = 1440;
const TInt KAedSizeAMRBuffer = 320;
const TInt KAedSizeAWBBuffer = 640;

const TInt KAedMinAMRBitRate = 400;      // voice activity detection creates at least 1 byte per frame => 50 bytes/s = 400 bits/s
const TInt KAedMaxAMRBitRate = 12200;
const TInt KAedNumSupportedAACSampleRates = 2;
const TInt KAedSupportedAACSampleRates[KAedNumSupportedAACSampleRates] = {KAedSampleRate16kHz, KAedSampleRate48kHz};

// Max resolution for visualization; this should cover > 30 minute clip if 5 samples per sec are requested 
const TInt KAedMaxVisualizationResolution = 10000;

// UId of the AAC CMMFCodec encoder
const TUid KAedAACEncSWCodecUid = {0x1020382F};    //KAdvancedUidCodecPCM16ToAAC
// UId of the AMR-NB CMMFCodec encoder
const TUid KAedAMRNBEncSWCodecUid = {0x101FAF68};    //KAdvancedUidCodecPCM16ToAMR

// UId of the AAC CMMFCodec decoder
const TUid KMmfAACDecSWCodecUid = {0x101FAF81};    //KMmfUidCodecAACToPCM16
// UId of the AMR-NB CMMFCodec decoder
const TUid KMmfAMRNBDecSWCodecUid = {0x101FAF67};    //KAdvancedUidCodecAMRToPCM16
// UId of the eAAC+ CMMFCodec decoder
const TUid KMmfUidCodecEnhAACPlusToPCM16 = {0x10207AA9};
// UId of the AMR-WB CMMFCodec decoder
const TUid KMmfAMRWBDecSWCodecUid = {0x101FAF5E};
// UId of the MP3 CMMFCodec decoder
const TUid KMmfAdvancedUidCodecMP3ToPCM16 = {0x101FAF85};

// FourCC for the eAAC+
const TUint32 KMMFFourCCCodeAACPlus = 0x43414520;		//(' ','E','A','C')

// Used in CProcInFileHandler::GetGainNow() to reduce positive gains
const TInt KAedPositiveGainDivider = 5;


#endif // __AUDCONSTANTS_H__

