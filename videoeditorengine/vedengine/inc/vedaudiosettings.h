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
* Header file for audio related settings/constants.
*
*/


#ifndef __VEDAUDIOSETTINGS_H__
#define __VEDAUDIOSETTINGS_H__

const TUint KVedAudioFramesInSample = 5;       // number of frames in one AMR audio sample
const TUint KVedMinAMRFrameSize = 13;          // smallest AMR frame size
const TUint KVedMaxAMRFrameSize = 32;          // largest AMR frame size
const TUint KVedAMRConversionBitrate = 12200;  // default target bitrate when converting to AMR
const TUint KVedAACConversionBitrate = 48000;  // default target bitrate when converting to AAC
const TUint KVedSilentAACFrameSize = 6;        // silent AAC frame size
const TUint KVedAverageAACFrameSize = 384;     // average AAC frame size

// AMR bitrates
#define KVedAMRBitrate4_75k  (4750)
#define KVedAMRBitrate5_15k  (5150)
#define KVedAMRBitrate5_90k  (5900)
#define KVedAMRBitrate6_70k  (6700)
#define KVedAMRBitrate7_40k  (7400)
#define KVedAMRBitrate7_95k  (7950)
#define KVedAMRBitrate10_2k (10200)
#define KVedAMRBitrate12_2k (12200)

// AAC samplerates
#define KVedAudioSamplingRate8k       (8000)
#define KVedAudioSamplingRate11_025k (11025)
#define KVedAudioSamplingRate12k     (12000)
#define KVedAudioSamplingRate16k     (16000)
#define KVedAudioSamplingRate22_050k (22050)
#define KVedAudioSamplingRate24k     (24000)
#define KVedAudioSamplingRate32k     (32000)
#define KVedAudioSamplingRate44_1k   (44100)
#define KVedAudioSamplingRate48k     (48000)
#define KVedAudioSamplingRate64k     (64000)
#define KVedAudioSamplingRate88_2k   (88200)
#define KVedAudioSamplingRate96k     (96000)


#endif
