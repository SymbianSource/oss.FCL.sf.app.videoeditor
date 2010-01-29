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




#ifndef __VEDCOMMON_H__
#define __VEDCOMMON_H__

#include <e32std.h>


/**
 * Constants for video clip speed settings. The speed is given by % x10, i.e. 1000 means 100 %.
 */
const TInt KVedNormalSpeed(1000);
const TInt KVedMaxSpeed(1000);
const TInt KVedMinSpeed(1);


/**
 * Enumeration for video clip classes.
 */
enum TVedVideoClipClass
    {
    EVedVideoClipClassFile = 15001,
    EVedVideoClipClassGenerated
    };


/**
 * Enumeration for bitrate modes.
 */
enum TVedBitrateMode
	{
	EVedBitrateModeUnrecognized = 18001,
	EVedBitrateModeConstant,
	EVedBitrateModeVariable,
	EVedBitrateModeLast  // should always be the last one
	};

/**
 * Enumeration for video bitstream modes.
 */
enum TVedVideoBitstreamMode
    {
    EVedVideoBitstreamModeUnknown = 0,          /* unrecognized mode; outside of H.263 Profile 0 Level 10, or MPEG-4 Visual Simple Profile */
    EVedVideoBitstreamModeH263,                 /* H.263 Simple Profile (Profile 0, Level 10) */                                    
    EVedVideoBitstreamModeMPEG4ShortHeader,     /* MPEG-4 Visual Simple Profile - Short Header */
    EVedVideoBitstreamModeMPEG4Regular,         /* MPEG-4 Visual Simple Profile - Regular */
    EVedVideoBitstreamModeMPEG4Resyn,           /* MPEG-4 Visual Simple Profile - Regular with Resynchronization Markers */
    EVedVideoBitstreamModeMPEG4DP,              /* MPEG-4 Visual Simple Profile - Data Partitioned */
    EVedVideoBitstreamModeMPEG4DP_RVLC,         /* MPEG-4 Visual Simple Profile - Data Partitioned with Reversible VLCs */
    EVedVideoBitstreamModeMPEG4Resyn_DP,        /* MPEG-4 Visual Simple Profile - Data Partitioned with Resynchronization Markers */
    EVedVideoBitstreamModeMPEG4Resyn_DP_RVLC,   /* MPEG-4 Visual Simple Profile - Data Partitioned with Reversible VLCs & Resynchronization Markers */
    EVedVideoBitstreamModeAVC
    };



/* Video format of bitstream - used everywhere */
enum TVedVideoType
    {
    EVedVideoTypeUnrecognized = 13001,  /* should always be the first one */
    EVedVideoTypeNoVideo,               /* video is not present */
    EVedVideoTypeH263Profile0Level10,   /* H.263 Simple Profile (Profile 0, Level 10) */
    EVedVideoTypeH263Profile0Level45,   /* H.263 Simple Profile (Profile 0, Level 45) */
    EVedVideoTypeMPEG4SimpleProfile,    /* MPEG-4 Visual Simple Profile (any mode) */
    EVedVideoTypeAVCBaselineProfile,    /* AVC Baseline Profile */
    EVedVideoTypeLast                   /* should always be the last one */
    };
	
/**
 * Enumeration for video formats.
 */
enum TVedVideoFormat
    {
    EVedVideoFormatUnrecognized = 11001,  // should always be the first one
    EVedVideoFormat3GPP,
    EVedVideoFormatMP4,
    EVedVideoFormatLast  // should always be the last one
    };

/**
 * Enumeration for audio formats.
 */
enum TVedAudioFormat
	{
	EVedAudioFormatUnrecognized = 16001,  // should always be the first one
	EVedAudioFormat3GPP,
	EVedAudioFormatMP4,
	EVedAudioFormatAMR,
	EVedAudioFormatAMRWB,
	EVedAudioFormatMP3,
	EVedAudioFormatAAC_ADIF,
	EVedAudioFormatAAC_ADTS,
	EVedAudioFormatWAV,
	EVedAudioFormatLast  // should always be the last one
	};


/**
 * Enumeration for audio types (that is, codecs).
 */
enum TVedAudioType
    {
    EVedAudioTypeUnrecognized = 14001,  // should always be the first one
    EVedAudioTypeNoAudio,
    EVedAudioTypeAMR,
    EVedAudioTypeAMRWB,
    EVedAudioTypeMP3,
    EVedAudioTypeAAC_LC,
    EVedAudioTypeAAC_LTP,
    EVedAudioTypeWAV,
    EVedAudioTypeLast  // should always be the last one
    };

/**
 * Enumeration for audio channel modes.
 */
enum TVedAudioChannelMode
    {
    EVedAudioChannelModeUnrecognized = 17001,
    EVedAudioChannelModeStereo,
    EVedAudioChannelModeDualChannel,
    EVedAudioChannelModeSingleChannel
    };

/**
 * Output parameter class
 */ 

class TVedOutputParameters
    {
    public:
        // default constructor; initializes optional parameters to values which mean that editor can decide the value
        inline TVedOutputParameters() 
        : iVideoType(EVedVideoTypeH263Profile0Level10), iVideoResolution(TSize(0,0)), iVideoBitrate(0), iVideoFrameRate(0), 
        iAudioType(EVedAudioTypeAMR), iAudioBitrate(0), iAudioChannelMode(EVedAudioChannelModeSingleChannel), iAudioSamplingRate(0), 
        iSyncIntervalInPicture(0), iSegmentSizeInBytes(0)
            {}
            
    public:

        // video codec
        TVedVideoType iVideoType;
        // target resolution, 0 = no preference (use from input)
        TSize iVideoResolution;
        // target video bitrate, 0 = no preference
        TInt iVideoBitrate;
        // target video framerate, 0 = no preference
        TReal iVideoFrameRate;

        // audio codec
        TVedAudioType iAudioType;
        // target audio bitrate, 0 = no preference
        TInt iAudioBitrate;
        // audio channel mode
        TVedAudioChannelMode iAudioChannelMode;
        // audio sampling rate, 0 = no preference
        TInt iAudioSamplingRate;
        
        // Segment interval in picture. In H.263 baseline this means number of non-empty GOB headers (1=every GOB has a header), 
        // Default is 0 == no segments inside picture
        // Coding standard & used profile etc. limit the value.
        TInt iSyncIntervalInPicture;
        
        // Target size of each coded video segment. Valid for H.264 and MPEG-4
        // Default is 0 == no segments inside picture
        TInt iSegmentSizeInBytes;
        
    };


/**
 * Enumeration for start transition effects.
 */
enum TVedStartTransitionEffect
    {
    EVedStartTransitionEffectNone = 21001,  // should always be the first one
    EVedStartTransitionEffectFadeFromBlack,
    EVedStartTransitionEffectFadeFromWhite,
    EVedStartTransitionEffectLast  // should always be the last one
    };


/**
 * Enumeration for middle transition effects.
 */
enum TVedMiddleTransitionEffect
    {
    EVedMiddleTransitionEffectNone = 22001,  // should always be the first one
    EVedMiddleTransitionEffectDipToBlack,
    EVedMiddleTransitionEffectDipToWhite,
    EVedMiddleTransitionEffectCrossfade,
    EVedMiddleTransitionEffectWipeLeftToRight,
    EVedMiddleTransitionEffectWipeRightToLeft,
    EVedMiddleTransitionEffectWipeTopToBottom,
    EVedMiddleTransitionEffectWipeBottomToTop,
    EVedMiddleTransitionEffectLast           // should always be the last one
    };

    
/**
 * Enumeration for end transition effects.
 */
enum TVedEndTransitionEffect
    {
    EVedEndTransitionEffectNone = 23001,  // should always be the first one
    EVedEndTransitionEffectFadeToBlack,
    EVedEndTransitionEffectFadeToWhite,
    EVedEndTransitionEffectLast  // should always be the last one
    };

    
/**
 * Enumeration for color effects.
 */
enum TVedColorEffect
    {
    EVedColorEffectNone = 31001,  // should always be the first one
    EVedColorEffectBlackAndWhite,
    EVedColorEffectToning,
    EVedColorEffectLast  // should always be the last one
    };


/**
 * Class for storing dynamic level marks.
 */
class TVedDynamicLevelMark 
    {
public:
    /** Mark time. */
    TTimeIntervalMicroSeconds iTime;

    /** 
     * Dynamic level (-63.5 - +12.7) in dB:s; one step represents +0.1 or -0.5 dB => values are -127...+127
     */
    TInt8 iLevel;

    /**
     * Constructs a new dynamic level mark.
     * 
     * @param aTime   time for the mark
     * @param aLevel  dynamic level (-63.5 ... +12.7) in dB:s one step = +0.1 or -0.5 dB => values can be -127...+127
     */
    inline TVedDynamicLevelMark(TTimeIntervalMicroSeconds aTime, TInt aLevel);

    /**
     * Constructs a new dynamic level mark from existing instance.
     * 
     * @param aMark  dynamic level mark to copy
     */
    inline TVedDynamicLevelMark(const TVedDynamicLevelMark& aMark);
    };	


/**
 * Transcode factor.
 */
struct TVedTranscodeFactor
    {
    TInt iTRes;
    TVedVideoBitstreamMode iStreamType;
    };


/**
 * Enumerates video editor engine panic codes and 
 * provides a static Panic() function.
 *
 */
class TVedPanic
    {
public:
    enum TVedPanicCodes 
        {
        EInternal = 1,  // internal error (that is, a  in the video editor engine)
        EDeprecated,    // deprecated class or method
        EVideoClipInfoNotReady,
        EVideoClipInfoIllegalVideoFrameIndex,
        EVideoClipInfoIllegalVideoFrameTime,
        EVideoClipInfoFrameOperationAlreadyRunning,
        EVideoClipInfoIllegalFrameResolution,
        EVideoClipInfoNoFileAssociated,
        EVideoClipInfoNoGeneratorAssociated,
        EAudioClipInfoNotReady,  //10
        EMovieEmpty,
        EMovieAddOperationAlreadyRunning,
        EMovieIllegalStartTransitionEffect,
        EMovieIllegalMiddleTransitionEffect,
        EMovieIllegalEndTransitionEffect,
        EMovieProcessingOperationAlreadyRunning,
        EMovieObserverAlreadyRegistered,
        EMovieObserverNotRegistered,
        EMovieIllegalQuality,
        EVideoClipIllegalIndex, //20
        EVideoClipIllegalSpeed, 
        EVideoClipIllegalColorEffect,
        EVideoClipIllegalCutInTime,
        EVideoClipIllegalCutOutTime,
        EVideoClipNoFileAssociated,
        EAudioClipIllegalStartTime,
        EAudioClipIllegalCutInTime,
        EAudioClipIllegalCutOutTime,
        EVideoClipGeneratorNotInserted,
        EVideoClipGeneratorNotReady,//30
        EVideoClipGeneratorAlreadyInserted,
        EVideoClipGeneratorIllegalVideoFrameIndex,
        EVideoClipGeneratorIllegalDuration,
        EVideoClipGeneratorIllegalFrameResolution,
        EVideoClipGeneratorIllegalVideoFrameTime,
        EImageClipGeneratorIllegalMaxResolution,
        EImageClipGeneratorNotReady,
        EImageClipGeneratorFrameOperationAlreadyRunning,
        ETitleClipGeneratorIllegalMaxResolution,
        ETitleClipGeneratorIllegalMaxFramerate,//40
        ETitleClipGeneratorFrameOperationAlreadyRunning,
        EIllegalDynamicLevelMark,
        EIllegalDynamicLevelMarkIndex,
        EVideoClipGeneratorNotOveray,
        EAnimationClipGeneratorNotReady
        };

public:
    inline static void Panic(TInt aPanic);
    };


#include "VedCommon.inl"

#endif // __VEDCOMMON_H__

