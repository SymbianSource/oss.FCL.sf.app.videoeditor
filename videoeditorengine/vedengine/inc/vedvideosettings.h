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
* Header file for video related settings/constants.
*
*/


#ifndef __VEDVIDEOSETTINGS_H__
#define __VEDVIDEOSETTINGS_H__

// VIDEO ENCODER RELATED SETTINGS / DEFAULTS

// Maximum supported resolution
//WVGA task
const TUint KVedMaxVideoWidth = 864;
//const TUint KVedMaxVideoWidth = 640;
const TUint KVedMaxVideoHeight = 480;

// Max duration for video frame in microseconds. This limits the slow motion effect.
// This now limits the duration to 30 seconds which is already a very extreme case. This has impact to MPEG4's module_time_base variable 
// which should not be longer than 32 bits since there are some variables e.g. in video decoder than can handle only 32-bit fields. 
const TInt KVedMaxFrameDuration(30000000);

// target/maximum bitrates  
const TUint KVedBitRateH263Level10 = 64000;
const TUint KVedBitRateH263Level20 = 512000;
const TUint KVedBitRateH263Level45 = 128000;
const TUint KVedBitRateMPEG4Level0 = 64000;
const TUint KVedBitRateMPEG4Level0B = 128000;
const TUint KVedBitRateMPEG4Level1 = 64000;
const TUint KVedBitRateMPEG4Level2 = 512000;
const TUint KVedBitRateMPEG4Level3 = 1024000;
const TUint KVedBitRateMPEG4Level4A = 4000000;

const TUint KVedBitRateAVCLevel1 = 64000;
const TUint KVedBitRateAVCLevel1b = 128000;
const TUint KVedBitRateAVCLevel1_1 = 192000;
const TUint KVedBitRateAVCLevel1_2 = 384000;
const TUint KVedBitRateAVCLevel1_3 = 768000;
const TUint KVedBitRateAVCLevel2 = 2000000;
//WVGA task
const TUint KVedBitRateAVCLevel2_1 = 4000000;
const TUint KVedBitRateAVCLevel2_2 = 4000000;
const TUint KVedBitRateAVCLevel3 = 10000000;
const TUint KVedBitRateAVCLevel3_1 = 14000000;


const TReal KVedMaxVideoFrameRate = 15.0;

// number of frames for transition effect - NOTE: This must be an even number !!!
const TUint KNumTransitionFrames = 10; 

#define KVedResolutionSubQCIF       (TSize(128,96))
#define KVedResolutionQCIF          (TSize(176,144))
#define KVedResolutionCIF           (TSize(352,288))
#define KVedResolutionQVGA          (TSize(320,240))
#define KVedResolutionVGA16By9      (TSize(640,352))
#define KVedResolutionVGA           (TSize(640,480))
//WVGA task
#define KVedResolutionWVGA           (TSize(864,480))


const TUint KMaxCodedPictureSizeQCIF = 16384; // QCIF and smaller
const TUint KMaxCodedPictureSizeCIF = 65536; // CIF and smaller
const TUint KMaxCodedPictureSizeMPEG4QCIF = 20480; // QCIF and smaller
const TUint KMaxCodedPictureSizeMPEG4L0BQCIF = 40960; // QCIF and smaller
const TUint KMaxCodedPictureSizeMPEG4CIF = 81920; // MPEG-4 CIF
const TUint KMaxCodedPictureSizeVGA = 163840; // For vga support
//WVGA task
const TUint KMaxCodedPictureSizeWVGA = 327680; // For WVGA support

const TUint KMaxCodedPictureSizeAVCLevel1 = 21875;
const TUint KMaxCodedPictureSizeAVCLevel1B = 43750;
const TUint KMaxCodedPictureSizeAVCLevel1_1 = 62500;
const TUint KMaxCodedPictureSizeAVCLevel1_2 = 125000;
const TUint KMaxCodedPictureSizeAVCLevel1_3 = 250000;
const TUint KMaxCodedPictureSizeAVCLevel2 = 250000;
//WVGA task
const TUint KMaxCodedPictureSizeAVCLevel2_1 = 500000;
const TUint KMaxCodedPictureSizeAVCLevel2_2 = 1000000;
const TUint KMaxCodedPictureSizeAVCLevel3 = 2000000;
const TUint KMaxCodedPictureSizeAVCLevel3_1 = 4000000;

_LIT8(KVedMimeTypeH263, "video/H263-2000");
_LIT8(KVedMimeTypeH263BaselineProfile, "video/H263-2000; profile=0");
_LIT8(KVedMimeTypeH263Level10, "video/H263-2000; profile=0; level=10");
_LIT8(KVedMimeTypeH263Level45, "video/H263-2000; profile=0; level=45");

_LIT8(KVedMimeTypeMPEG4Visual, "video/mp4v-es");
_LIT8(KVedMimeTypeMPEG4SimpleVisualProfile, "video/mp4v-es; profile-level-id=8");
_LIT8(KVedMimeTypeMPEG4SimpleVisualProfileLevel2, "video/mp4v-es; profile-level-id=2");
_LIT8(KVedMimeTypeMPEG4SimpleVisualProfileLevel3, "video/mp4v-es; profile-level-id=3");
_LIT8(KVedMimeTypeMPEG4SimpleVisualProfileLevel4A, "video/mp4v-es; profile-level-id=4");

_LIT8(KVedMimeTypeAVC, "video/H264");
_LIT8(KVedMimeTypeAVCBaselineProfileLevel1, "video/H264; profile-level-id=42800A");
_LIT8(KVedMimeTypeAVCBaselineProfileLevel1B, "video/H264; profile-level-id=42900B");
_LIT8(KVedMimeTypeAVCBaselineProfileLevel1_1, "video/H264; profile-level-id=42800B");
_LIT8(KVedMimeTypeAVCBaselineProfileLevel1_2, "video/H264; profile-level-id=42800C");
_LIT8(KVedMimeTypeAVCBaselineProfileLevel1_3, "video/H264; profile-level-id=42800D");
_LIT8(KVedMimeTypeAVCBaselineProfileLevel2, "video/H264; profile-level-id=428014");

//WVGA task
_LIT8(KVedMimeTypeAVCBaselineProfileLevel2_1, "video/H264; profile-level-id=428015");
_LIT8(KVedMimeTypeAVCBaselineProfileLevel2_2, "video/H264; profile-level-id=428016");
_LIT8(KVedMimeTypeAVCBaselineProfileLevel3, "video/H264; profile-level-id=42801E");
_LIT8(KVedMimeTypeAVCBaselineProfileLevel3_1, "video/H264; profile-level-id=42801F");

// 3gpmp4 buffer sizes
const TInt K3gpMp4ComposerWriteBufferSize = 65536;
const TInt K3gpMp4ComposerNrOfWriteBuffers = 10;
const TInt K3gpMp4ParserReadBufferSize = 8192;

#endif
