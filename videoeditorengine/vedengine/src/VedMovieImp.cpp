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




#include "VedMovieImp.h"
#include "VedVideoClipGenerator.h"
#include "movieprocessor.h"
#include "VedVideosettings.h"
#include "VedAudiosettings.h"
#include <ecom/ecom.h>
#include "VedAudioClipInfoImp.h"
#include "AudSong.h"
#include "AudClip.h"
#include "AudClipInfo.h"
#include "Vedqualitysettingsapi.h"
#include "ctrtranscoder.h"
#include "ctrtranscoderobserver.h"
#include "vedproctimeestimate.h"
#include "vedcodecchecker.h"

#include <vedcommon.h>

const TInt KVedAudioTrackIndex = 1;

// Print macro
#ifdef _DEBUG
#include <e32svr.h>
#define PRINT(x) RDebug::Print x
#else
#define PRINT(x)
#endif



// Near-dummy observer class for temporary transcoder instance. In practice is only used to provide input framerate
// to the transcoder
class CTrObs : public CBase, public MTRTranscoderObserver
    {
public:
    /* Constructor & destructor */
    
    inline CTrObs(TReal aFrameRate) : iInputFrameRate(aFrameRate) 
        {
        };
    inline ~CTrObs()
        {
        };

    // Dummy methods from MTRTranscoderObserver, just used to complete the observer class
    inline void MtroInitializeComplete(TInt /*aError*/)
        {
        };
    inline void MtroFatalError(TInt /*aError*/)
        {
        };
    inline void MtroReturnCodedBuffer(CCMRMediaBuffer* /*aBuffer*/) 
        {
        };
    // method to provide clip input framerate to transcoder
    inline void MtroSetInputFrameRate(TReal& aRate)
        {
        aRate = iInputFrameRate;
        };
    inline void MtroAsyncStopComplete()
        {
        };
        
    inline void MtroSuspend()
        {
        };
        
    inline void MtroResume()
        {
        };
        
private:// data

        // clip input framerate (fps)
        TReal iInputFrameRate;
    
    };
   
    
// -------- Local functions ---------

// Map video format mimetype to editor's internal enumeration 
static TVedVideoFormat MapVideoFormatTypes(const TText8* aVideoFormatType)
    {
    TPtrC8 mimeType(aVideoFormatType);
    TBuf8<256> string;
    string = _L8("video/3gpp");
    
    if ( mimeType.MatchF( (const TDesC8& )string ) != KErrNotFound ) 
        {
        return EVedVideoFormat3GPP;
        }
    else
        {
        string = _L8("video/mp4");
        if ( mimeType.MatchF( (const TDesC8& )string ) != KErrNotFound ) 
            {
            return EVedVideoFormatMP4;
            }
        }

    return EVedVideoFormatUnrecognized;
    }

// Map video codec mimetype to editor's internal enumeration 
static TVedVideoType MapVideoCodecTypes(const TText8* aVideoCodecType)
    {
    TPtrC8 mimeType(aVideoCodecType);
    TBuf8<256> string;
    string = _L8("video/H263*");
    
    if ( mimeType.MatchF( (const TDesC8& )string ) != KErrNotFound ) 
        {
        // H.263
        string = _L8("*level*");
        if ( mimeType.MatchF( (const TDesC8& )string ) != KErrNotFound ) 
            {
            string = _L8("*level=10");
            if ( mimeType.MatchF( (const TDesC8& )string ) != KErrNotFound ) 
                {
                return EVedVideoTypeH263Profile0Level10;
                }
            string = _L8("*level=45");
            if ( mimeType.MatchF( (const TDesC8& )string ) != KErrNotFound ) 
                {
                return EVedVideoTypeH263Profile0Level45;
                }
            }
        // no level specified => 10        
        return EVedVideoTypeH263Profile0Level10;
        }
    else
        {
        string = _L8("video/mp4v-es*");
        if ( mimeType.MatchF( (const TDesC8& )string ) != KErrNotFound ) 
            {
            return EVedVideoTypeMPEG4SimpleProfile;
            }
            
        else 
            {
            string = _L8("video/h264*");
            if ( mimeType.MatchF( (const TDesC8& )string ) != KErrNotFound ) 
                {
                return EVedVideoTypeAVCBaselineProfile;
                }
            }
        }
    return EVedVideoTypeUnrecognized;
    }

// Map editor's internal enumeration to video codec mimetype
static void MapVideoCodecTypeToMime(TVedVideoType aType, TBufC8<255>& aMimeType)
    {
    switch ( aType )
        {
        case EVedVideoTypeH263Profile0Level10:
            {
            aMimeType =  KVedMimeTypeH263BaselineProfile;
            }
            break;
        case EVedVideoTypeH263Profile0Level45:
            {
            aMimeType = KVedMimeTypeH263Level45;
            }
            break;
        default:
            {
            //EVedVideoTypeMPEG4SimpleProfile
            aMimeType = KVedMimeTypeMPEG4Visual;
            }
        }
    }


// Map audio codec fourcc to editor's internal enumeration 
static TAudType MapAudioCodecTypes(const TText8* aAudioCodecType)
    {
    TPtrC8 fourCCType(aAudioCodecType);
    TBuf8<256> string;
    string = _L8(" AMR");
    
    if ( fourCCType.MatchF( (const TDesC8& )string ) != KErrNotFound ) 
        {
        return EAudAMR;
        }
    else  
        {
        string = _L8(" AAC");
        if ( fourCCType.MatchF( (const TDesC8& )string ) != KErrNotFound ) 
            {
            return EAudAAC_MPEG4;
            }
        
        }
    return EAudTypeUnrecognized;
    }

// -------- Member functions ---------
EXPORT_C CVedMovie* CVedMovie::NewL(RFs *aFs)
    {
    PRINT(_L("CVedMovie::NewL"));

    CVedMovieImp* self = (CVedMovieImp*)NewLC(aFs);
    CleanupStack::Pop(self);
    return self;
    }

    
EXPORT_C CVedMovie* CVedMovie::NewLC(RFs *aFs)
    {
    PRINT(_L("CVedMovie::NewLC"));

    CVedMovieImp* self = new (ELeave) CVedMovieImp(aFs);
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
    }


CVedMovieImp::CVedMovieImp(RFs *aFs)
        : iFs(aFs), 
          iVideoClipArray(8), // Initial size of video clip array is 8
          iAudioClipInfoArray(8), // Initial size of audio clip array is 8
          iObserverArray(4), // Initial size of observer array is 4
          iStartTransitionEffect(EVedStartTransitionEffectNone),
          iEndTransitionEffect(EVedEndTransitionEffectNone),
          iQuality(EQualityAutomatic),
          iNotifyObserver(ETrue),
          iMovieProcessingObserver(0)
    {
    }


void CVedMovieImp::ConstructL()
    {
    iAudSong = CAudSong::NewL(iFs);
    iAudSong->RegisterSongObserverL(this);
    iAddOperation = CVedMovieAddClipOperation::NewL(this);
    
    iCodecChecker = CVedCodecChecker::NewL();
    iProcessor = CMovieProcessor::NewL();
    
    PRINT((_L("CVedMovie::ConstructL() CVideoQualitySelector in use")));
    iQualitySelector = CVideoQualitySelector::NewL();
    
    CalculatePropertiesL();
    }


CVedMovieImp::~CVedMovieImp()
    {
    DoReset();
    iObserverArray.Reset();

    delete iAddOperation;
    delete iCodecChecker;    
    delete iProcessor;

    delete iAudSong;
    delete iAddedVideoClipFilename;
    
    delete iQualitySelector;
    
    REComSession::FinalClose();
    }

CVedMovie::TVedMovieQuality CVedMovieImp::Quality() const
    {
    return iQuality;
    }

void CVedMovieImp::SetQuality(TVedMovieQuality aQuality)
    {
    __ASSERT_ALWAYS(aQuality >= EQualityAutomatic && aQuality < EQualityLast,
        TVedPanic::Panic(TVedPanic::EMovieIllegalQuality));
    if (aQuality != iQuality) 
        {
        iOutputParamsSet = EFalse;
        TVedMovieQuality prevQuality = iQuality;
        iQuality = aQuality;
        TRAPD(err,CalculatePropertiesL());//should not leave with current implementation, but try to handle it anyway
        if ( err == KErrNone )
            {
            // successful
            FireMovieQualityChanged(this);
            }
        else
            {
            // setting was not successful, use the previous value
            iQuality = prevQuality;
            TRAP(err,CalculatePropertiesL());// previous should always succee
            }
            
        }
    }

TVedVideoFormat CVedMovieImp::Format() const
    {
    return iFormat;
    }

TVedVideoType CVedMovieImp::VideoType() const
    {
    return iVideoType;
    }

TSize CVedMovieImp::Resolution() const
    {
    return iResolution;
    }

TInt CVedMovieImp::MaximumFramerate() const
    {
    return iMaximumFramerate;
    }

TVedAudioType CVedMovieImp::AudioType() const
    {
    TAudType audioType = iAudSong->OutputFileProperties().iAudioType;
    TVedAudioType vedAudioType = EVedAudioTypeUnrecognized;
    
    if (iAudSong->ClipCount(KAllTrackIndices) == 0)
        {
        vedAudioType = EVedAudioTypeNoAudio;
        }
    else if (audioType == EAudAMR)
        {
        vedAudioType = EVedAudioTypeAMR;
        }
    else if (audioType == EAudAAC_MPEG4 )
        {
        vedAudioType = EVedAudioTypeAAC_LC;
        }
	else if (audioType == EAudNoAudio)
		{
		vedAudioType = EVedAudioTypeNoAudio;
		}

    return vedAudioType;
    }

TInt CVedMovieImp::AudioSamplingRate() const
    {
    return iAudSong->OutputFileProperties().iSamplingRate;
    }

TVedAudioChannelMode CVedMovieImp::AudioChannelMode() const
    {
    TVedAudioChannelMode vedChannelMode = EVedAudioChannelModeUnrecognized;
    if (iAudSong->OutputFileProperties().iChannelMode == EAudStereo)
        {
        vedChannelMode = EVedAudioChannelModeStereo;
        }
    else if (iAudSong->OutputFileProperties().iChannelMode == EAudSingleChannel)
        {
        vedChannelMode = EVedAudioChannelModeSingleChannel;
        }
    else if (iAudSong->OutputFileProperties().iChannelMode == EAudDualChannel)
        {
        vedChannelMode = EVedAudioChannelModeDualChannel;
        }
    return vedChannelMode;    
    }


TVedBitrateMode CVedMovieImp::AudioBitrateMode() const
    {
    TVedBitrateMode vedBitrateMode = EVedBitrateModeUnrecognized;
    switch(iAudSong->OutputFileProperties().iBitrateMode) 
        {
    case EAudConstant:
        vedBitrateMode = EVedBitrateModeConstant;
        break;
    case EAudVariable:
        vedBitrateMode = EVedBitrateModeVariable;
        break;
    default:
        TVedPanic::Panic(TVedPanic::EInternal);
        }
    return vedBitrateMode;
    }
TInt CVedMovieImp::AudioBitrate() const
    {
    return iAudSong->OutputFileProperties().iBitrate;
    }

TInt CVedMovieImp::VideoBitrate() const
    {
    // restricted bitrate: forces transcoding of video content to this bitrate
    return iVideoRestrictedBitrate;
    }
    
TInt CVedMovieImp::VideoStandardBitrate() const
    {
    // the default bitrate to be used when encoding new content. This can be actually lower than the standard limit
    // but it is not restricted bitrate that would trigger transcoding to happen
    return iVideoStandardBitrate;
    }

TReal CVedMovieImp::VideoFrameRate() const
    {
    return iVideoFrameRate;
    }

// This must not be called for 3gp clips that should always have AMR audio
// returns ETrue if aAudioProperties contain settings found in input (can be modified by this method)
// and EFalse if client should decide what to do with them
TBool CVedMovieImp::MatchAudioPropertiesWithInput( TAudFileProperties& aAudioProperties )
    {
    // If there are no audio clips
    if (iAudSong->ClipCount(KAllTrackIndices) == 0)
        {
        return ETrue;
        }
        
    // Go through the audio clips and select the best one
    TAudFileProperties prop;
    TAudFileProperties highestProp;
    TBool highestPropFound = EFalse;
    
    for (TInt a = 0; a < iAudSong->ClipCount(KAllTrackIndices); a++)
        {
        CAudClip* clip = iAudSong->Clip(a, KAllTrackIndices);        
        prop = clip->Info()->Properties();

        if ( ( prop.iChannelMode == aAudioProperties.iChannelMode ) &&
             ( prop.iSamplingRate == aAudioProperties.iSamplingRate ) )
            {
            // there is a match => keep the properties
            PRINT((_L("CVedMovie::MatchAudioPropertiesWithInput() found preferred set from input, sampling rate & channels %d & %d"), prop.iSamplingRate, prop.iChannelMode));
            return ETrue;
            }
        else
            {
            // need to stay within limits given by aAudioProperties (it has the highest preferred mode)
            if ( ( prop.iAudioType == aAudioProperties.iAudioType ) &&
                 ( prop.iSamplingRate <= aAudioProperties.iSamplingRate ) )
                {
                // take the highest channelmode & sampling rate from input. 
                if ( !highestPropFound || 
                    ( (prop.iSamplingRate > highestProp.iSamplingRate) ||
                      ( (prop.iSamplingRate == highestProp.iSamplingRate) && (prop.iChannelMode > highestProp.iChannelMode) ) ) )
                    {
                    PRINT((_L("CVedMovie::MatchAudioPropertiesWithInput() found new highest prop from input, sampling rate & channels %d & %d"), prop.iSamplingRate, prop.iChannelMode));
                    highestProp.iAudioType = prop.iAudioType;
                    highestProp.iChannelMode = prop.iChannelMode;
                    highestProp.iSamplingRate = prop.iSamplingRate;
                    highestPropFound = ETrue;
                    }
                }
            }
        }

    // if we come here, there was no exact match found. Use the best one, if found
    if ( highestPropFound )
        {
        // take the sampling rate and channel mode from inProp. Currently we support only 16k and 48k output, but this should be changed
        // once we know what we need to support; but requires synchronization with audio editor engine

        if ( iAudSong->AreOutputPropertiesSupported(highestProp))
            {
            PRINT((_L("CVedMovie::MatchAudioPropertiesWithInput() selected audio parameters, sampling rate & channels %d & %d"), highestProp.iSamplingRate, highestProp.iChannelMode));
            aAudioProperties.iChannelMode = highestProp.iChannelMode;
            aAudioProperties.iSamplingRate = highestProp.iSamplingRate;
            aAudioProperties.iBitrate = KAudBitRateDefault; //use default since we don't know the bitrate of the input.
            return ETrue;
            }
        else
            {
            // We have some AAC in the input but it is not any of our supported sampling rates. 
            // The aAudioProperties may have 48k here but since we don't have such high input, better to use 16k in output
            PRINT((_L("CVedMovie::MatchAudioPropertiesWithInput() no good match with input")));
            return EFalse;
            }
        }
    else
        {
        // Not even a close match
        PRINT((_L("CVedMovie::MatchAudioPropertiesWithInput() not even a close match with input")));
        return EFalse;
        }
    
    }


// Set video codec mimetype member variable, and also max values that level defines (e.g. max framerate; actual values are taken from quality set)
void CVedMovieImp::SetVideoCodecMimeType(const TText8* aVideoCodecType)
    {
    TPtrC8 mimeType(aVideoCodecType);
    
    if ( mimeType.MatchF( KVedMimeTypeH263 ) != KErrNotFound ) 
        {
        // H.263 baseline
        iVideoCodecMimeType.Set(KVedMimeTypeH263BaselineProfile);
        iMaximumFramerate = 15;
        }
    else if ( mimeType.MatchF( KVedMimeTypeH263BaselineProfile ) != KErrNotFound ) 
        {
        // H.263 baseline
        iVideoCodecMimeType.Set(KVedMimeTypeH263BaselineProfile);
        iMaximumFramerate = 15;
        }
    else if (mimeType.MatchF( KVedMimeTypeH263Level45 ) != KErrNotFound )
        {
        // H.263 level 45
        iVideoCodecMimeType.Set(KVedMimeTypeH263Level45);
        iMaximumFramerate = 15;
        }
    else if (mimeType.MatchF( KVedMimeTypeMPEG4SimpleVisualProfileLevel2 ) != KErrNotFound )
        {
        // MPEG-4 SP level 2
        iVideoCodecMimeType.Set(KVedMimeTypeMPEG4SimpleVisualProfileLevel2);
        iMaximumFramerate = 15;
        }
    else if (mimeType.MatchF( KVedMimeTypeMPEG4SimpleVisualProfileLevel3 ) != KErrNotFound )
        {
        // MPEG-4 SP level 3
        iVideoCodecMimeType.Set(KVedMimeTypeMPEG4SimpleVisualProfileLevel3);
        iMaximumFramerate = 30;
        }
    else if (mimeType.MatchF( KVedMimeTypeMPEG4SimpleVisualProfileLevel4A ) != KErrNotFound )
        {
        // MPEG-4 SP level 4a
        iVideoCodecMimeType.Set(KVedMimeTypeMPEG4SimpleVisualProfileLevel4A);
        iMaximumFramerate = 30;
        }
    else if (mimeType.MatchF( KVedMimeTypeMPEG4Visual ) != KErrNotFound )                 
        {
        // MPEG-4 SP level 0
        iVideoCodecMimeType.Set(KVedMimeTypeMPEG4SimpleVisualProfile);
        iMaximumFramerate = 15;
        }        
    else if (mimeType.MatchF( KVedMimeTypeAVCBaselineProfileLevel1B ) != KErrNotFound )
        {
        // AVC level 1b
        iVideoCodecMimeType.Set(KVedMimeTypeAVCBaselineProfileLevel1B);
        iMaximumFramerate = 15;
        }
    else if (mimeType.MatchF( KVedMimeTypeAVCBaselineProfileLevel1_1 ) != KErrNotFound )
        {
        // AVC level 1.1
        iVideoCodecMimeType.Set(KVedMimeTypeAVCBaselineProfileLevel1_1);
        iMaximumFramerate = 7.5;
        }
    else if (mimeType.MatchF( KVedMimeTypeAVCBaselineProfileLevel1_2 ) != KErrNotFound )
        {
        // AVC level 1.2
        iVideoCodecMimeType.Set(KVedMimeTypeAVCBaselineProfileLevel1_2);
        iMaximumFramerate = 15;
        } 
    //WVGA task
    else if (mimeType.MatchF( KVedMimeTypeAVCBaselineProfileLevel1_3 ) != KErrNotFound )
        {
        // AVC level 1.3
        iVideoCodecMimeType.Set(KVedMimeTypeAVCBaselineProfileLevel1_3);
        iMaximumFramerate = 15;
        }
    else if (mimeType.MatchF( KVedMimeTypeAVCBaselineProfileLevel2 ) != KErrNotFound )
        {
        // AVC level 2
        iVideoCodecMimeType.Set(KVedMimeTypeAVCBaselineProfileLevel2);
        iMaximumFramerate = 15;
        }
    else if (mimeType.MatchF( KVedMimeTypeAVCBaselineProfileLevel2_1 ) != KErrNotFound )
        {
        // AVC level 2.1
        iVideoCodecMimeType.Set(KVedMimeTypeAVCBaselineProfileLevel2_1);
        iMaximumFramerate = 15;
        }
    else if (mimeType.MatchF( KVedMimeTypeAVCBaselineProfileLevel2_2 ) != KErrNotFound )
        {
        // AVC level 2.2
        iVideoCodecMimeType.Set(KVedMimeTypeAVCBaselineProfileLevel2_2);
        iMaximumFramerate = 15;
        }
    else if (mimeType.MatchF( KVedMimeTypeAVCBaselineProfileLevel3 ) != KErrNotFound )
        {
        // AVC level 3
        iVideoCodecMimeType.Set(KVedMimeTypeAVCBaselineProfileLevel3);
        iMaximumFramerate = 15;
        }
    else if (mimeType.MatchF( KVedMimeTypeAVCBaselineProfileLevel3_1 ) != KErrNotFound )
        {
        // AVC level 3.1
        iVideoCodecMimeType.Set(KVedMimeTypeAVCBaselineProfileLevel3_1);
        iMaximumFramerate = 15;
        }
    else
        {
        // AVC level 1
        iVideoCodecMimeType.Set(KVedMimeTypeAVCBaselineProfileLevel1);    
        iMaximumFramerate = 15;
        }
        
    }
    
   
// -----------------------------------------------------------------------------
// CVedMovieImp::GetQCIFPropertiesL
// Get settings for QCIF or subQCIF resolution; H.263, H.264 or MPEG-4
// This is a special case since it has also H.263 codec support.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CVedMovieImp::GetQCIFPropertiesL(SVideoQualitySet& aLocalQualitySet)
    {
    TInt foundQualitySetForLevelError = KErrNone;
    
    // QCIF and subQCIF are handled together, since they are covered by the same levels of the H.263, MPEG-4 and H.264.
    // We have the preferred video codec in iVideoType (based on input clips)
    
    if ( iVideoType == EVedVideoTypeAVCBaselineProfile )
        {
        // first check if we support H.264 output
        // level 1B
        PRINT((_L("CVedMovie::GetQCIFPropertiesL() check H.264 level 1B")));
        TRAP(foundQualitySetForLevelError,iQualitySelector->GetVideoQualitySetL( aLocalQualitySet, TPtrC8(KVedMimeTypeAVCBaselineProfileLevel1B)));        

        if ( foundQualitySetForLevelError == KErrNotSupported )
            {
            // check H.264 level 1 instead
            PRINT((_L("CVedMovie::GetQCIFPropertiesL() check H.264 level 1")));
            TRAP(foundQualitySetForLevelError,iQualitySelector->GetVideoQualitySetL( aLocalQualitySet, TPtrC8(KVedMimeTypeAVCBaselineProfileLevel1)));
            if ( foundQualitySetForLevelError == KErrNotSupported )
                {
                // H.264 @ QCIF is not supported. Fall back to H.263 (=> input is transcoded to H.263)
                PRINT((_L("CVedMovie::getQCIFPropertiesL() no set for MPEG-4 level 0, switch to H.263")));
                iVideoType = EVedVideoTypeH263Profile0Level45;  // use level 45 since it is better than 10, and we don't have bitrate restrictions here
                foundQualitySetForLevelError = GetQCIFPropertiesL(aLocalQualitySet);
                // keep H.263; H.264 may not be supported at all
                }
            }                    
            
        if ( foundQualitySetForLevelError == KErrNone )
            {
            SetVideoCodecMimeType(aLocalQualitySet.iVideoCodecMimeType);
            }
        }
    else if ( iVideoType == EVedVideoTypeMPEG4SimpleProfile )
        {
        // MPEG-4 @ QCIF is an exceptional case; should not happen for locally recorded clips, but support is kept here for compatibility
        PRINT((_L("CVedMovie::GetQCIFPropertiesL() check MPEG-4 level 0")));
        TRAP(foundQualitySetForLevelError,iQualitySelector->GetVideoQualitySetL( aLocalQualitySet, TPtrC8(KVedMimeTypeMPEG4SimpleVisualProfile)));
        if ( foundQualitySetForLevelError == KErrNotSupported )
            {
            // MPEG-4 QCIF is not listed in the quality set. Use it with H.263 settings, except the codec type
            PRINT((_L("CVedMovie::GetQCIFPropertiesL() no set for MPEG-4 level 0, take settings from H.263")));
            iVideoType = EVedVideoTypeH263Profile0Level10;// level 10 is comparable to level 0 of MPEG-4; level 0b is not used
            foundQualitySetForLevelError = GetQCIFPropertiesL(aLocalQualitySet);
            // change back to MPEG-4
            iVideoType = EVedVideoTypeMPEG4SimpleProfile;
            }
        if ( foundQualitySetForLevelError == KErrNone )
            {
            // set MPEG-4 MIME-type and other related settings;
            // also if the quality set showed H.263; this way we can support mpeg-4 even if it is not any of the preferred ones, since we have supported it earlier too...
            iVideoCodecMimeType.Set(KVedMimeTypeMPEG4SimpleVisualProfile);
            iMaximumFramerate = 15;
            }
        }
    else
        {
        // H.263
        if ( iVideoType == EVedVideoTypeH263Profile0Level45 )
            {
            PRINT((_L("CVedMovie::GetQCIFPropertiesL() check H.263 level 45")));
            TRAP(foundQualitySetForLevelError,iQualitySelector->GetVideoQualitySetL( aLocalQualitySet, TPtrC8(KVedMimeTypeH263Level45)));
            }
        if ( (iVideoType == EVedVideoTypeH263Profile0Level10) || (foundQualitySetForLevelError == KErrNotSupported) )
            {
            PRINT((_L("CVedMovie::GetQCIFPropertiesL() check H.263 level 10")));
            TRAP(foundQualitySetForLevelError,iQualitySelector->GetVideoQualitySetL( aLocalQualitySet, TPtrC8(KVedMimeTypeH263)));
            }
            
        if ( foundQualitySetForLevelError == KErrNone )
            {
            SetVideoCodecMimeType(aLocalQualitySet.iVideoCodecMimeType);
            }
        }
    // assume subQCIF can use other QCIF settings except resolution
    
   
    return foundQualitySetForLevelError;
    }
    
    
// -----------------------------------------------------------------------------
// CVedMovieImp::GetCIFQVGAPropertiesL
// Get settings for CIF or QVGA resolution; H.264 or MPEG-4
// Some encoders may not support both, hence we may need to change the resolution here
// and that's why this is treated as a special case.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CVedMovieImp::GetCIFQVGAPropertiesL(TSize aSize, TReal aFrameRate, SVideoQualitySet& aLocalQualitySet)
    {
    TInt foundQualitySetForLevelError = KErrNone;
    
    // CIF and QVGA are handled together, since they are covered by the same levels of the MPEG-4 and H.264;
    // framerate brings a difference though.
    // We have the preferred video codec in iVideoType (based on input clips), and framerate in aFrameRate
    // so let's check the level first, and then additionally check if the resolution is supported.
    
    if ( iVideoType == EVedVideoTypeAVCBaselineProfile )
        {
        // first check if we support H.264 output
        // try level 1.2; higher levels not supported yet, and hence aFrameRate has no impact; 
        // level 1.1 is not seen relevant since it supports only 7.5 fps
        PRINT((_L("CVedMovie::GetCIFQVGAPropertiesL() check H.264 level 1.2")));
        TRAP(foundQualitySetForLevelError,iQualitySelector->GetVideoQualitySetL( aLocalQualitySet, TPtrC8(KVedMimeTypeAVCBaselineProfileLevel1_2)));
        if ( foundQualitySetForLevelError == KErrNotSupported )
            {
            // H.264 level 1.2 is not supported; switch to MPEG-4
            iVideoType = EVedVideoTypeMPEG4SimpleProfile;
            }
        }

    if ( iVideoType == EVedVideoTypeMPEG4SimpleProfile )         
        {
        // MPEG-4 was dominant in the input, OR H.264 is not supported in quality set. 
        // The order is like this since MPEG-4 is generally more supported than H.264. 
        // Hence this CANNOT handle the case where MPEG-4 CIF/QVGA is not in the quality set but H.264 is, but in that case it falls back to H.263 QCIF in the host method
        if ( aFrameRate > 15.0 )
            {
            // check level 3
            PRINT((_L("CVedMovie::GetCIFQVGAPropertiesL() check MPEG-4 level 3")));
            TRAP(foundQualitySetForLevelError,iQualitySelector->GetVideoQualitySetL( aLocalQualitySet, TPtrC8(KVedMimeTypeMPEG4SimpleVisualProfileLevel3)));
            }
        if ( (aFrameRate <= 15.0) || (foundQualitySetForLevelError == KErrNotSupported) )
            {
            // try level 2
            PRINT((_L("CVedMovie::GetCIFQVGAPropertiesL() check MPEG-4 level 2")));
            TRAP(foundQualitySetForLevelError,iQualitySelector->GetVideoQualitySetL( aLocalQualitySet, TPtrC8(KVedMimeTypeMPEG4SimpleVisualProfileLevel2)));
            }
        }

    if ( foundQualitySetForLevelError == KErrNotSupported )
        {
        // these resolutions are not supported! The default one will be taken into use in CalculatePropertiesL
        PRINT((_L("CVedMovie::GetCIFQVGAPropertiesL() no support for CIF/QVGA found in quality set")));
        return foundQualitySetForLevelError;
        }
        
        

    SVideoQualitySet tmpSet;
    TRAPD(resolutionSupported, iQualitySelector->GetVideoQualitySetL( tmpSet, aSize));
    if ( resolutionSupported == KErrNotSupported )
        {
        // the preferred resolution is not supported => must use the other one
        // the level was supported already, so no need to check support for the other one
        if ( aSize == KVedResolutionCIF )
            {
            PRINT((_L("CVedMovie::GetCIFQVGAPropertiesL() CIF support not available, must use QVGA instead")));
            iResolution = KVedResolutionQVGA;
            }
        else
            {
            PRINT((_L("CVedMovie::GetCIFQVGAPropertiesL() QVGA support not available, must use CIF instead")));
            iResolution = KVedResolutionCIF;
            }
        }
    else
        {
        PRINT((_L("CVedMovie::GetCIFQVGAPropertiesL() resolution support ok")));
        iResolution = aSize;
        }

    // at this point we must have found a suitable set
    SetVideoCodecMimeType(aLocalQualitySet.iVideoCodecMimeType);
    return foundQualitySetForLevelError;
    }
    
    
// -----------------------------------------------------------------------------
// CVedMovieImp::GetVGAPropertiesL
// Get settings for VGA resolution. If not supported, get High settings instead
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CVedMovieImp::GetVGAPropertiesL(SVideoQualitySet& aLocalQualitySet)
    {
    TInt foundQualitySetForResolutionError = KErrNone;
    
    if ( iVideoType == EVedVideoTypeAVCBaselineProfile )
        {
        PRINT((_L("CVedMovie::GetWVGAPropertiesL() check H.264 level 3.0")));
        TRAP(foundQualitySetForResolutionError,iQualitySelector->GetVideoQualitySetL( aLocalQualitySet, TPtrC8(KVedMimeTypeAVCBaselineProfileLevel3)));
        if (foundQualitySetForResolutionError == KErrNotSupported )
            {
            PRINT((_L("CVedMovie::GetVGAPropertiesL() H.264 level 3.0 not supported")));
            // H.264 level 3.0 is not supported; switch to MPEG-4
            iVideoType = EVedVideoTypeMPEG4SimpleProfile;
            }
        else
            {
            PRINT((_L("CVedMovie::GetVGAPropertiesL() H.264 level 3.0 supported")));
            iResolution = KVedResolutionVGA;
            SetVideoCodecMimeType(aLocalQualitySet.iVideoCodecMimeType);
            }
        }
    
    if ( iVideoType == EVedVideoTypeMPEG4SimpleProfile )         
        {
        // VGA; check MPEG-4 level 4a
        // Settings may not exist => trap the leave
        TRAP(foundQualitySetForResolutionError,iQualitySelector->GetVideoQualitySetL( aLocalQualitySet, TPtrC8(KVedMimeTypeMPEG4SimpleVisualProfileLevel4A)));
        if ( foundQualitySetForResolutionError == KErrNotSupported )
            {
            PRINT((_L("CVedMovie::GetVGAPropertiesL() VGA not supported as output, try highest supported")));
            // No VGA, try the highest instead
            foundQualitySetForResolutionError = GetHighPropertiesL(aLocalQualitySet);
            }
        else
            {
            PRINT((_L("CVedMovie::GetVGAPropertiesL() VGA with MPEG-4 level 4a selected as output")));
            iResolution = KVedResolutionVGA;
            SetVideoCodecMimeType(aLocalQualitySet.iVideoCodecMimeType);
            }
        }
    
    return foundQualitySetForResolutionError;
    }
    
// -----------------------------------------------------------------------------
// CVedMovieImp::GetVGA16By9PropertiesL
// Get settings for VGA 16:9 resolution. If not supported, get High settings instead
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CVedMovieImp::GetVGA16By9PropertiesL(SVideoQualitySet& aLocalQualitySet)
    {
    TInt foundQualitySetForResolutionError = KErrNone;
    
    if ( iVideoType != EVedVideoTypeMPEG4SimpleProfile )
        {
        // NOTE: max supported H.264/AVC level currently (1.2) allows only CIF 15 fps, no VGA. 
        PRINT((_L("CVedMovie::GetVGA16By9PropertiesL() VGA & H.264 == NOT SUPPORTED")));
        TVedPanic::Panic(TVedPanic::EInternal);
        }
         
    // VGA; check MPEG-4 level 4a
    // Settings may not exist => trap the leave    
    TRAP(foundQualitySetForResolutionError,iQualitySelector->GetVideoQualitySetL( aLocalQualitySet, TPtrC8(KVedMimeTypeMPEG4SimpleVisualProfileLevel4A)));
    if ( foundQualitySetForResolutionError == KErrNotSupported )
        {
        PRINT((_L("CVedMovie::GetVGAPropertiesL() VGA 16:9 not supported as output, try highest supported")));
        // No VGA, try the highest instead
        foundQualitySetForResolutionError = GetHighPropertiesL(aLocalQualitySet);
        }
    else
        {
        PRINT((_L("CVedMovie::GetVGAPropertiesL() VGA 16:9 with MPEG-4 level 4a selected as output")));
        iResolution = KVedResolutionVGA16By9;
        SetVideoCodecMimeType(aLocalQualitySet.iVideoCodecMimeType);
        }
    return foundQualitySetForResolutionError;
    }

//WVGA task   
// -----------------------------------------------------------------------------
// CVedMovieImp::GetWVGAPropertiesL
// -----------------------------------------------------------------------------
//
TInt CVedMovieImp::GetWVGAPropertiesL(SVideoQualitySet& aLocalQualitySet)
    {
    TInt foundQualitySetForResolutionError = KErrNone;;
    
    if ( iVideoType == EVedVideoTypeAVCBaselineProfile )
        {
        PRINT((_L("CVedMovie::GetWVGAPropertiesL() check H.264 level 3.1")));
        TRAP(foundQualitySetForResolutionError,iQualitySelector->GetVideoQualitySetL( aLocalQualitySet, TPtrC8(KVedMimeTypeAVCBaselineProfileLevel3)));
        if (foundQualitySetForResolutionError == KErrNotSupported )
            {
            PRINT((_L("CVedMovie::GetWVGAPropertiesL() H.264 level 3.0 not supported")));
            // H.264 level 3.0 is not supported; switch to MPEG-4
            iVideoType = EVedVideoTypeMPEG4SimpleProfile;
            }
        else
            {
            PRINT((_L("CVedMovie::GetWVGAPropertiesL() H.264 level 3.0  supported")));
            iResolution = KVedResolutionWVGA;
            SetVideoCodecMimeType(aLocalQualitySet.iVideoCodecMimeType);
            }
        }

    if ( iVideoType == EVedVideoTypeMPEG4SimpleProfile )         
        {
        // Settings may not exist => trap the leave
        TRAP(foundQualitySetForResolutionError,iQualitySelector->GetVideoQualitySetL( aLocalQualitySet, TPtrC8(KVedMimeTypeMPEG4SimpleVisualProfileLevel4A)));
        if ( foundQualitySetForResolutionError == KErrNotSupported )
            {
            PRINT((_L("CVedMovie::GetWVGAPropertiesL() WVGA not supported as output, try highest supported")));
            // No WVGA, try the highest instead
            foundQualitySetForResolutionError = GetHighPropertiesL(aLocalQualitySet);
            }
        else
            {
            PRINT((_L("CVedMovie::GetWVGAPropertiesL() WVGA with MPEG-4 level 4a selected as output")));
            iResolution = KVedResolutionWVGA;
            SetVideoCodecMimeType(aLocalQualitySet.iVideoCodecMimeType);
            }
            
        }
    return foundQualitySetForResolutionError;

    }

// -----------------------------------------------------------------------------
// CVedMovieImp::GetHighPropertiesL
// Get settings for High quality and apply relevant parts to member variables;
// the rest are applied in common part
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CVedMovieImp::GetHighPropertiesL(SVideoQualitySet& aLocalQualitySet)
    {
    TInt foundQualitySetForResolutionError = KErrNone;
    
    PRINT((_L("CVedMovie::SetHighPropertiesL() use the highest supported")));
    TRAP(foundQualitySetForResolutionError, iQualitySelector->GetVideoQualitySetL( aLocalQualitySet, CVideoQualitySelector::EVideoQualityHigh));
    if ( foundQualitySetForResolutionError == KErrNone ) 
        {
        iResolution.iWidth = aLocalQualitySet.iVideoWidth;
        iResolution.iHeight = aLocalQualitySet.iVideoHeight;
        iVideoType = MapVideoCodecTypes(aLocalQualitySet.iVideoCodecMimeType);
        SetVideoCodecMimeType(aLocalQualitySet.iVideoCodecMimeType);
        }
    else
        {
        // handled in the end of CalculatePropertiesL, but should not be possible to reach this since High should always exist
        }
    
    return foundQualitySetForResolutionError;
    }
    
    
// -----------------------------------------------------------------------------
// CVedMovieImp::ApplyAutomaticPropertiesL
// Apply the settings based on properties of input clips
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CVedMovieImp::ApplyAutomaticPropertiesL(TAudFileProperties &aAudioProperties)
    {
    // Automatic quality selection
    
    PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() automatic quality selected")));
    TTimeIntervalMicroSeconds durationOfMP4MPEG4(0);
    TTimeIntervalMicroSeconds durationOfMP4AVC(0);
    
    TTimeIntervalMicroSeconds durationOf3gppH263(0);
    TTimeIntervalMicroSeconds durationOf3gppMPEG4(0);
    TTimeIntervalMicroSeconds durationOf3gppAVC(0);
    
    TTimeIntervalMicroSeconds durationOfVGA(0);
    TTimeIntervalMicroSeconds durationOfVGA16By9(0);
    TTimeIntervalMicroSeconds durationOfQVGA(0);
    TTimeIntervalMicroSeconds durationOfCIF(0);
    TTimeIntervalMicroSeconds durationOfQCIF(0);
    TTimeIntervalMicroSeconds durationOfSubQCIF(0);
    TReal frameRateOfQVGA(0);
    TReal frameRateOfCIF(0);
    TReal framerate(0);
    TBool audioOnlyMP4 = EFalse;
    
    //WVGA task
    TTimeIntervalMicroSeconds durationOfWVGA(0);
    TReal frameRateOfWVGA(0);

    iVideoType = EVedVideoTypeH263Profile0Level10;
    
    for (TInt i = 0; i < VideoClipCount(); i++)
        {
        CVedVideoClipInfo* currentInfo = VideoClip(i)->Info();

        if (currentInfo->Class() == EVedVideoClipClassFile)
            {
            // Add up the durations of different resolution video
            if (currentInfo->Resolution() == KVedResolutionSubQCIF)
                {
                PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() subQCIF input")));
                durationOfSubQCIF= durationOfSubQCIF.Int64() + VideoClip(i)->EditedDuration().Int64(); 
                }
            else if (currentInfo->Resolution() == KVedResolutionQCIF)
                {
                PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() QCIF input")));
                durationOfQCIF= durationOfQCIF.Int64() + VideoClip(i)->EditedDuration().Int64();
                }
            else if (currentInfo->Resolution() == KVedResolutionCIF)
                {
                PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() CIF input")));
                durationOfCIF= durationOfCIF.Int64() + VideoClip(i)->EditedDuration().Int64();
                framerate = (1000*currentInfo->VideoFrameCount())/(currentInfo->Duration().Int64()/1000);
                if ( framerate > frameRateOfCIF )
                    {
                    frameRateOfCIF = framerate;
                    }
                }
            else if (currentInfo->Resolution() == KVedResolutionQVGA)
                {
                PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() QVGA input")));
                durationOfQVGA= durationOfQVGA.Int64() + VideoClip(i)->EditedDuration().Int64();
                framerate = (1000*currentInfo->VideoFrameCount())/(currentInfo->Duration().Int64()/1000);
                if ( framerate > frameRateOfQVGA )
                    {
                    frameRateOfQVGA = framerate;
                    }
                }                 
            else if (currentInfo->Resolution() == KVedResolutionVGA16By9)
                {
                PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() VGA 16:9 input")));
                durationOfVGA16By9 = durationOfVGA16By9.Int64() + VideoClip(i)->EditedDuration().Int64();
                }
            
            else if (currentInfo->Resolution() == KVedResolutionVGA)
                {
                PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() VGA input")));
                durationOfVGA= durationOfVGA.Int64() + VideoClip(i)->EditedDuration().Int64();
                }
                //WVGA task
            else if (currentInfo->Resolution() == KVedResolutionWVGA)
                {
                PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() WVGA input")));
                durationOfWVGA= durationOfWVGA.Int64() + VideoClip(i)->EditedDuration().Int64();
                }

            
            if (currentInfo->Format() == EVedVideoFormat3GPP) 
                {
                if ( currentInfo->VideoType() == EVedVideoTypeMPEG4SimpleProfile )
                    {
                    // 3gpp file with MPEG-4 video
                    PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() 3gpp with MPEG-4 input")));
                    durationOf3gppMPEG4 = durationOf3gppMPEG4.Int64() + VideoClip(i)->EditedDuration().Int64(); 
                    }
                else if ( currentInfo->VideoType() == EVedVideoTypeAVCBaselineProfile )
                    {
                    // 3gpp file with AVC video
                    PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() 3gpp with AVC input")));
                    durationOf3gppAVC = durationOf3gppAVC.Int64() + VideoClip(i)->EditedDuration().Int64(); 
                    // NOTE: Level 1B is assumed always for QCIF AVC in automatic input case. 
                    }	                	                
                else
                    {
                    // 3gpp file with H.263 video
                    PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() 3gpp with H.263 input")));
                    durationOf3gppH263 = durationOf3gppH263.Int64() + VideoClip(i)->EditedDuration().Int64();
                    if (currentInfo->VideoType() == EVedVideoTypeH263Profile0Level45)
                        {
                        PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() 3gpp with H.263 L45 input")));
                        iVideoType = EVedVideoTypeH263Profile0Level45;
                        }
                    }
                }
                
            else if (currentInfo->Format() == EVedVideoFormatMP4) 
                {
                
                if ( currentInfo->VideoType() == EVedVideoTypeMPEG4SimpleProfile )
                    {
                    // MP4 file with MPEG-4 video
                    PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() MP4 with MPEG-4 input")));
                    durationOfMP4MPEG4 = durationOfMP4MPEG4.Int64() + VideoClip(i)->EditedDuration().Int64(); 
                    }
                    
                else if ( currentInfo->VideoType() == EVedVideoTypeAVCBaselineProfile )
                    {
                    // MP4 file with AVC video
                    PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() MP4 with AVC input")));
                    durationOfMP4AVC = durationOfMP4AVC.Int64() + VideoClip(i)->EditedDuration().Int64(); 
                    // NOTE: What about levels ?	                    
                    } 
                    
                else
                    {
                    PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() MP4 with other input, not supported")));
                    TVedPanic::Panic(TVedPanic::EInternal);
                    }
                
                }
            else
                {
                // Unknown format
                PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() not 3gpp nor MP4 , not supported")));
                TVedPanic::Panic(TVedPanic::EInternal);
                }
            }
        } // end of for loop, all input has been checked now



    //
    // Decide the output format
    //
     
    // First check if we had any video
    if ( (durationOfMP4MPEG4.Int64() == 0) && (durationOfMP4AVC.Int64() == 0) && 
         (durationOf3gppH263.Int64() == 0) && (durationOf3gppMPEG4.Int64() == 0) && (durationOf3gppAVC.Int64() == 0) 
       )              
        {
        // no video, check audio
        for ( TInt i = 0; i < iAudSong->ClipCount(KVedAudioTrackIndex); i++)
            {
            if ( iAudSong->Clip(i, KVedAudioTrackIndex)->Info()->Properties().iAudioType == EAudAMR )
                {
                // 3gpp
                durationOf3gppH263 = durationOf3gppH263.Int64() + iAudSong->Clip(i, KVedAudioTrackIndex)->EditedDuration().Int64();
                }
            else
                {
                // mp4; mark audio-only as Mpeg4 video at this point
                durationOfMP4MPEG4 = durationOfMP4MPEG4.Int64() + iAudSong->Clip(i, KVedAudioTrackIndex)->EditedDuration().Int64();
                audioOnlyMP4 = ETrue;
                }
            }
        }
    
    // Then decide the file format    
    if ( ( durationOfMP4MPEG4.Int64() + durationOfMP4AVC.Int64() > 0 ) &&       
         ( ( durationOfMP4MPEG4.Int64() + durationOfMP4AVC.Int64() ) >=
           ( durationOf3gppH263.Int64() + durationOf3gppMPEG4.Int64() + durationOf3gppAVC.Int64() ) ) )
        {
        // majority of input is MP4
        iFormat = EVedVideoFormatMP4;
        if ( audioOnlyMP4 )
            {
            // no video, just audio, use highest quality; this branch is only used to skip the else-branches
            PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() audio only with MP4")));    
            // iVideoType is set in GetHighPropertiesL based on high quality level
            }
        else if ( durationOfMP4MPEG4 > durationOfMP4AVC )
            {
            // MP4, video is MPEG-4
            iVideoType = EVedVideoTypeMPEG4SimpleProfile;
            PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() majority of input is MP4 with MPEG-4 video")));                
            } 
        else
            {
            // MP4, video is AVC
            iVideoType = EVedVideoTypeAVCBaselineProfile;
            PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() majority of input is MP4 with AVC video")));
            }
        }
    else        
        {
        // majority of input is 3gpp, or no video. Keep 3gpp as in previous versions
        PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() majority of input is 3gpp or no video")));
        iFormat = EVedVideoFormat3GPP;
        
        if ((durationOf3gppMPEG4 > 0) &&
            (durationOf3gppMPEG4 >= durationOf3gppAVC) &&
            (durationOf3gppMPEG4 >= durationOf3gppH263))
            {
            iVideoType = EVedVideoTypeMPEG4SimpleProfile;
            PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() majority of input is 3gpp with MPEG-4 video")));
            } 
        
        else if ((durationOf3gppAVC > 0) &&
                 (durationOf3gppAVC >= durationOf3gppH263))
            {
            iVideoType = EVedVideoTypeAVCBaselineProfile;
            PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() majority of input is 3gpp with AVC video")));                
            } 
        
        else
            {
            // H.263
            PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() majority of input is 3gpp with H.263 video")));
            if ( iVideoType != EVedVideoTypeH263Profile0Level45 )
                {
                //use level 10 as used in previous versions
                iVideoType = EVedVideoTypeH263Profile0Level10;
                PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() majority of input is 3gpp with H.263 L10 video")));
                }
            
            }                        
        }
   
   
   
   
    // Then decide the resolution
   
   
   
    // cannot use the iQualitySet since it overrides the video settings. Use this mainly for checking audio settings
    SVideoQualitySet localQualitySet;
    TInt foundQualitySetForResolutionError = KErrNone;
    
    if ( audioOnlyMP4 )
        {
        // get video parameters for audio-only MP4 clip; audio-only 3gpp is handled as QCIF
        foundQualitySetForResolutionError = GetHighPropertiesL(localQualitySet);
        }
    //WVGA task
    else if ((durationOfWVGA > 0) &&
    		(durationOfWVGA >= durationOfVGA) &&
            (durationOfWVGA >= durationOfVGA16By9) &&
            (durationOfWVGA >= durationOfQVGA) && 
            (durationOfWVGA >= durationOfCIF) && 
            (durationOfWVGA >= durationOfQCIF) && 
            (durationOfWVGA >= durationOfSubQCIF))
            {
            // VGA
            PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() majority of input is WVGA")));
            iResolution = KVedResolutionWVGA;
            foundQualitySetForResolutionError = GetWVGAPropertiesL(localQualitySet);
            } 
    else if ((durationOfVGA > 0) &&
        (durationOfVGA >= durationOfVGA16By9) &&
        (durationOfVGA >= durationOfQVGA) && 
        (durationOfVGA >= durationOfCIF) && 
        (durationOfVGA >= durationOfQCIF) && 
        (durationOfVGA >= durationOfSubQCIF))
        {
        // VGA
        PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() majority of input is VGA")));
        iResolution = KVedResolutionVGA;
        foundQualitySetForResolutionError = GetVGAPropertiesL(localQualitySet);
        } 
        
    else if ((durationOfVGA16By9 > 0) &&
             (durationOfVGA16By9 >= durationOfCIF) &&
             (durationOfVGA16By9 >= durationOfQVGA) &&
             (durationOfVGA16By9 >= durationOfQCIF) &&
             (durationOfVGA16By9 >= durationOfSubQCIF))
        {
        // VGA 16:9
        PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() majority of input is VGA 16:9")));
        iResolution = KVedResolutionVGA16By9;
        foundQualitySetForResolutionError = GetVGA16By9PropertiesL(localQualitySet);
        }
        
    else if ((durationOfCIF > 0) &&
             (durationOfCIF >= durationOfQVGA) && 
             (durationOfCIF >= durationOfQCIF) && 
             (durationOfCIF >= durationOfSubQCIF))
        {
        // CIF, but may be translated to QVGA if CIF not supported as output
        PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() majority of input is CIF")));
        iResolution = KVedResolutionCIF;
        foundQualitySetForResolutionError = GetCIFQVGAPropertiesL(KVedResolutionCIF, frameRateOfCIF, localQualitySet);
        // audio parameters set in the end
        }
    else if ((durationOfQVGA > 0) &&
             (durationOfQVGA >= durationOfQCIF) && 
             (durationOfQVGA >= durationOfSubQCIF))
        {
        // QVGA, but may be translated to CIF if QVGA not supported as output
        PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() majority of input is QVGA")));
        iResolution = KVedResolutionQVGA;
        foundQualitySetForResolutionError = GetCIFQVGAPropertiesL(KVedResolutionQVGA, frameRateOfQVGA, localQualitySet);
        // audio parameters set in the end
        }
    else if ((durationOfSubQCIF > 0) &&
             (durationOfSubQCIF > durationOfQCIF))    // keep the > and not >= since QCIF is better than subQCIF
        {
        // subQCIF is not that important but is kept here anyway
        PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() majority of input is subQCIF")));
        iResolution = KVedResolutionSubQCIF;
        foundQualitySetForResolutionError = GetQCIFPropertiesL(localQualitySet);
        // audio parameters set in the end
        }
    else 
        {
        // QCIF
        PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() majority of input is QCIF")));
        iResolution = KVedResolutionQCIF;
        foundQualitySetForResolutionError = GetQCIFPropertiesL(localQualitySet);
        // audio parameters set in the end
        }
        
    if ( foundQualitySetForResolutionError == KErrNone )
        {
        // iVideoType and MIME-type were set earlier. 
        iVideoStandardBitrate = localQualitySet.iVideoBitRate;
        iVideoFrameRate = localQualitySet.iVideoFrameRate;
        iRandomAccessRate = localQualitySet.iRandomAccessRate;
        // audio parameters set in the end
        }
        
        
    // Then handle cases where we didn't found suitable output format based on input resolution 
    // (i.e. we can't generate output that matches with majority of input)            
    if ( foundQualitySetForResolutionError == KErrNotSupported )
        {
        
        PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() no suitable quality set found yet...")));
        // did not get preferred set from qualityselector; use highest one; may be overruled in MatchAudioPropertiesWithInput
        if (iFormat == EVedVideoFormatMP4 )
            {
            // MP4 => AAC
            // High quality should be always present
            
            PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() try taking the high quality")));
            TRAPD(err,iQualitySelector->GetVideoQualitySetL( localQualitySet, CVideoQualitySelector::EVideoQualityHigh));
            if ( err )
                {
                // should not happen if the variation is done properly
                PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() problem in getting high quality set, ERROR")));
                User::Leave( err );
                }
            
            aAudioProperties.iAudioType = MapAudioCodecTypes(localQualitySet.iAudioFourCCType);
            aAudioProperties.iSamplingRate = localQualitySet.iAudioSamplingRate;    
            if ( localQualitySet.iAudioChannels == 1 )
                {
                aAudioProperties.iChannelMode = EAudSingleChannel;
                }
            else
                {
                aAudioProperties.iChannelMode = EAudStereo;
                }
            aAudioProperties.iBitrate = localQualitySet.iAudioBitRate;
            // take also video settings from the new set
            iResolution.iWidth = localQualitySet.iVideoWidth;
            iResolution.iHeight = localQualitySet.iVideoHeight;
            // MIME-type and bitrate
            SetVideoCodecMimeType(localQualitySet.iVideoCodecMimeType);
            iVideoStandardBitrate = localQualitySet.iVideoBitRate;
            }
        else
            {
            PRINT((_L("CVedMovie::ApplyAutomaticPropertiesL() use 3gpp")));
            // 3gp => use AMR
            aAudioProperties.iAudioType = EAudAMR;
            aAudioProperties.iSamplingRate = KVedAudioSamplingRate8k;
            aAudioProperties.iChannelMode = EAudSingleChannel;
            aAudioProperties.iBitrate = KAudBitRateDefault;  //use default. 
            // set also video settings for 3gpp
            iResolution = KVedResolutionQCIF;
            // MIME-type and bitrate
            iVideoCodecMimeType.Set(KVedMimeTypeH263);
            iVideoStandardBitrate = KVedBitRateH263Level10;
            }
            
        }
    else
        {
        // preferred settings were got from quality set API without any error; take the audio settings too
        
        aAudioProperties.iAudioType = MapAudioCodecTypes(localQualitySet.iAudioFourCCType);
        aAudioProperties.iSamplingRate = localQualitySet.iAudioSamplingRate;    
        if ( localQualitySet.iAudioChannels == 1 )
            {
            aAudioProperties.iChannelMode = EAudSingleChannel;
            }
        else
            {
            aAudioProperties.iChannelMode = EAudStereo;
            }
        aAudioProperties.iBitrate = localQualitySet.iAudioBitRate;
        }
        
        
        
    if ( aAudioProperties.iAudioType != EAudAMR )
        {
        // still check that we don't do unnecessary audio transcoding. AMR output is not changed to anything else
        if ( !MatchAudioPropertiesWithInput( aAudioProperties ) )
            {
            // there was no input clip that matched the properties
            
            if ( foundQualitySetForResolutionError == KErrNotSupported )
                {
                // the properties were taken from highest quality set after not finding a suitable set; better to ensure we have 16k
                aAudioProperties.iSamplingRate = KVedAudioSamplingRate16k;
                aAudioProperties.iChannelMode = EAudSingleChannel;
                aAudioProperties.iBitrate = KAudBitRateDefault;  //use default. 
                }
            // else the properties were taken from a suitable set; convert audio to match them
            
            }
        }
    
    }
    
// -----------------------------------------------------------------------------
// CVedMovieImp::ApplyRequestedPropertiesL
// Apply the settings the client requested
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CVedMovieImp::ApplyRequestedPropertiesL(TAudFileProperties &aAudioProperties)
    {
    TBool legacyMode = EFalse;
    
    if (iQuality == EQualityMMSInteroperability)
        {
        // MMS interoperability settings
        PRINT((_L("CVedMovie::ApplyRequestedPropertiesL() MMS quality selected")));
        // MMS interoperability should be always there => no leave
        iQualitySelector->GetVideoQualitySetL( iQualitySet, CVideoQualitySelector::EVideoQualityMMS);
        // map the set to member variables later
        
        }

    else if (iQuality == EQualityResolutionQCIF)
        {
        // this is a legacy branch, used only with older editor UI
        PRINT((_L("CVedMovie::ApplyRequestedPropertiesL() QCIF quality selected; check level 45 vs level 10")));
        legacyMode = ETrue;

        iResolution = KVedResolutionQCIF;
        
        // Start with MMS and upgrade based on input if needed
        iQualitySelector->GetVideoQualitySetL( iQualitySet, CVideoQualitySelector::EVideoQualityMMS);
        iFormat = MapVideoFormatTypes(iQualitySet.iVideoFileMimeType);
        iVideoType = MapVideoCodecTypes(iQualitySet.iVideoCodecMimeType);
        
        aAudioProperties.iAudioType = MapAudioCodecTypes(iQualitySet.iAudioFourCCType);
        aAudioProperties.iSamplingRate = iQualitySet.iAudioSamplingRate;    
        if ( iQualitySet.iAudioChannels == 1 )
            {
            aAudioProperties.iChannelMode = EAudSingleChannel;
            }
        else
            {
            aAudioProperties.iChannelMode = EAudStereo;
            }
        aAudioProperties.iBitrate = iQualitySet.iAudioBitRate;
        
        
        for (TInt i = 0; i < VideoClipCount(); i++)
            {
            CVedVideoClipInfo* currentInfo = VideoClip(i)->Info();

            // Go through all files and check if any are H.263 Profile0 Level45
            if (currentInfo->Class() == EVedVideoClipClassFile)
                {
                if (currentInfo->VideoType() == EVedVideoTypeH263Profile0Level45)
                    {
                    iVideoType = EVedVideoTypeH263Profile0Level45;
                    }
                }
            }        
            
        
        if ( iVideoType == EVedVideoTypeH263Profile0Level45 )
            {
            iVideoCodecMimeType.Set(KVedMimeTypeH263Level45);
            iVideoStandardBitrate = KVedBitRateH263Level45;
            }
        else
            {
            iVideoCodecMimeType.Set(KVedMimeTypeH263);
            iVideoStandardBitrate = KVedBitRateH263Level10;
            }
        }

    else if (iQuality == EQualityResolutionCIF)
        {
        // this is a legacy branch, used only with older editor UI
        PRINT((_L("CVedMovie::ApplyRequestedPropertiesL() CIF quality selected")));
        legacyMode = ETrue;
        iFormat = EVedVideoFormatMP4;
        iVideoType = EVedVideoTypeMPEG4SimpleProfile;
        iResolution = KVedResolutionCIF;
        iVideoCodecMimeType.Set(KVedMimeTypeMPEG4SimpleVisualProfileLevel2);
        iVideoStandardBitrate = KVedBitRateMPEG4Level2;
        // use 16 kHz mono AAC, as used to be the selection with CIF in older editor versions
        aAudioProperties.iAudioType = EAudAAC_MPEG4;
        aAudioProperties.iSamplingRate = KVedAudioSamplingRate16k;
        aAudioProperties.iChannelMode = EAudSingleChannel;
        aAudioProperties.iBitrate = KAudBitRateDefault;  //use default.
        }
    else if (iQuality == EQualityResolutionMedium)
        {
        PRINT((_L("CVedMovie::ApplyRequestedPropertiesL() medium quality selected")));
        // Medium quality should be always there => no leave
        iQualitySelector->GetVideoQualitySetL( iQualitySet, CVideoQualitySelector::EVideoQualityNormal);
        // map the set to member variables later
        
        }
    else if (iQuality == EQualityResolutionHigh)
        {
        PRINT((_L("CVedMovie::ApplyRequestedPropertiesL() high quality selected")));
        // High quality should be always there => no leave
        iQualitySelector->GetVideoQualitySetL( iQualitySet, CVideoQualitySelector::EVideoQualityHigh);
        // map the set to member variables later
        
        }

    if ( !legacyMode )  // legacyMode is forced CIF or QCIF; their parameters were set already, this is for others
        {
        // set common video settings from the obtained iQualitySet. In automatic case they were taken from input
        iFormat = MapVideoFormatTypes(iQualitySet.iVideoFileMimeType);
        iVideoType = MapVideoCodecTypes(iQualitySet.iVideoCodecMimeType);
        iResolution.iWidth = iQualitySet.iVideoWidth;
        iResolution.iHeight = iQualitySet.iVideoHeight;
        iVideoFrameRate = iQualitySet.iVideoFrameRate;
        iVideoStandardBitrate = iQualitySet.iVideoBitRate;    // still not restricted => not necessary to transcode
        SetVideoCodecMimeType(iQualitySet.iVideoCodecMimeType);
        iRandomAccessRate = iQualitySet.iRandomAccessRate;
        
        aAudioProperties.iAudioType = MapAudioCodecTypes(iQualitySet.iAudioFourCCType);
        aAudioProperties.iSamplingRate = iQualitySet.iAudioSamplingRate;    
        if ( iQualitySet.iAudioChannels == 1 )
            {
            aAudioProperties.iChannelMode = EAudSingleChannel;
            }
        else
            {
            aAudioProperties.iChannelMode = EAudStereo;
            }
        aAudioProperties.iBitrate = iQualitySet.iAudioBitRate;
        }
    }
    
    
// -----------------------------------------------------------------------------
// CVedMovieImp::CalculatePropertiesL
// Determine & apply the output properties for movie
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CVedMovieImp::CalculatePropertiesL()
    {
    PRINT((_L("CVedMovie::CalculatePropertiesL() in")));

    if (iOutputParamsSet)
       return;  
    
    if ( (VideoClipCount() == 0) && (AudioClipCount() == 0) )
        {
        PRINT((_L("CVedMovie::CalculatePropertiesL() no clips in movie, out")));
        return;
        }
        
    
    // set some default values. These will be updated if needed, in the two functions called below
    // default video framerate is 15
    iVideoFrameRate = iMaximumFramerate = 15.0;
    // default random access rate is 1/5 fps
    iRandomAccessRate = 0.2;

    TAudFileProperties audioProperties;

    if ( iQuality == EQualityAutomatic )
        {
        // Automatic quality selection based on input
        ApplyAutomaticPropertiesL(audioProperties);   	    
        }
    else    
        { 
        // forced quality
        ApplyRequestedPropertiesL(audioProperties);
        } 
        

        
    // Common finalization        
        
    PRINT((_L("CVedMovie::CalculatePropertiesL() fileformat: %d"), iFormat));
    PRINT((_L("CVedMovie::CalculatePropertiesL() video codec, resolution, bitrate, framerate, random access rate: %d, (%d,%d), %d, %f, %f"), iVideoType, iResolution.iWidth, iResolution.iHeight, iVideoStandardBitrate, iVideoFrameRate, iRandomAccessRate));
    PRINT((_L("CVedMovie::CalculatePropertiesL() audio codec, samplingrate, channels: %d, %d, %d"), audioProperties.iAudioType, audioProperties.iSamplingRate, audioProperties.iChannelMode));
    
    // Set the audio output properties here to the values specified above
    if ( !iAudSong->SetOutputFileFormat(audioProperties.iAudioType, audioProperties.iSamplingRate,audioProperties.iChannelMode, audioProperties.iBitrate ) )
        {
        PRINT((_L("CVedMovie::CalculatePropertiesL() iAudSong->SetOutputFileFormat failed, try again with default settings")));
        // something was wrong with the settings. Use default ones that should always work:
        if ( audioProperties.iAudioType == EAudAMR )
            {
            audioProperties.iSamplingRate = KVedAudioSamplingRate8k;
            audioProperties.iChannelMode = EAudSingleChannel;
            }
        else
            {
            audioProperties.iAudioType = EAudAAC_MPEG4;
            audioProperties.iSamplingRate = KVedAudioSamplingRate16k;
            audioProperties.iChannelMode = EAudSingleChannel;
            }
        // try again; bitrate is default 
        iAudSong->SetOutputFileFormat(audioProperties.iAudioType, audioProperties.iSamplingRate, audioProperties.iChannelMode);
        }
                                                              
    }


void CVedMovieImp::SetAudioFadingL()
    {
    // Apply fading only if there are no specific dynamic level marks inserted. 
    // I.e. this is used only if audio volume is set by SetxxVolumeGainL or not set at all
    if ( !iApplyDynamicLevelMark )
        {
        
        iNotifyObserver = EFalse;
        for (TInt index = 0; index < VideoClipCount(); index++)
        {
            TBool fadeIn = EFalse;
            TBool fadeOut = EFalse;
            if ( index == 0 )
                {
                // first clip, allow fade in always
                fadeIn = ETrue;
                }
            else if (iVideoClipArray[index-1]->iMiddleTransitionEffect != EVedMiddleTransitionEffectNone) 
                {
                // there is a transition effect before this clip, allow fade in
                fadeIn = ETrue;
                }
            else
                {
                // no transition, no audio fading either
                fadeIn = EFalse;
                }
            
            if ( index == VideoClipCount()-1 )
                {
                // last clip, allow fade out always
                fadeOut = ETrue;
                }
            else if (iVideoClipArray[index]->iMiddleTransitionEffect != EVedMiddleTransitionEffectNone) 
                {
                // there is a transition effect after this clip, allow fade out
                fadeOut = ETrue;
                }
            else
                {
                // no transition, no audio fading either
                fadeOut = EFalse;
                }
                
            CVedVideoClip* clip = iVideoClipArray[index];
            
            if (clip->EditedHasAudio() && !clip->IsMuted() && (fadeIn || fadeOut))
            {                                    

                TTimeIntervalMicroSeconds clipLength = 
                    TTimeIntervalMicroSeconds( clip->CutOutTime().Int64() - clip->CutInTime().Int64() );
                    
                // Do fading only if clip length is over 1 second
                // Otherwise the original markers remain untouched
                if ( clipLength > TTimeIntervalMicroSeconds(1000000) )
                {
                    // get normal level
                    TInt normalLevel = 0;

                    // get normal level 
                
                    // NOTE: This assumes that the client has set a constant 
                    // gain value for the whole clip, i.e. two markers,
                    // one at the beginning and other at the end        
                    // iApplyDynamicLevelMark now controls that removing others is safe
                    
                    normalLevel = clip->GetVolumeGain();
                    
                    // remove all existing markers               
                    while (clip->DynamicLevelMarkCount())
                    {
                        clip->RemoveDynamicLevelMark(0);
                    }                        
                     
                    // add movie-level volume gain
                    normalLevel += iVolumeGainForAllVideoClips;
                    
                    // use normal - 50dB as lower level; step in values is 0.5db
                    TInt lowLevel = normalLevel - 100;
                    if ( lowLevel < -127 )
                        {
                        lowLevel = -127;
                        }

                    // 0s: low
                    TVedDynamicLevelMark mark1(clip->CutInTime(), lowLevel);
                    
                    // 0.5s: normal
                    TTimeIntervalMicroSeconds time = 
                        TTimeIntervalMicroSeconds(clip->CutInTime().Int64() + 
                                                  TInt64(500000) );
                    
                    TVedDynamicLevelMark mark2(time, normalLevel);
                    
                    // clip length - 0.5s: normal
                    time = TTimeIntervalMicroSeconds(clip->CutOutTime().Int64() -
                                                     TInt64(500000) );

                    TVedDynamicLevelMark mark3(time, normalLevel);                                    
                    
                    // clip length: low
                    TVedDynamicLevelMark mark4(clip->CutOutTime(), lowLevel);
                    
                    if ( fadeIn )
                        {
                        clip->InsertDynamicLevelMarkL(mark1);
                        clip->InsertDynamicLevelMarkL(mark2);
                        }
                    if ( fadeOut )
                        {
                        clip->InsertDynamicLevelMarkL(mark3);
                        clip->InsertDynamicLevelMarkL(mark4);
                        }
                }
            }
        }
        
        for (TInt index = 0; index < AudioClipCount(); index++)
        {
            CAudClip* clip = iAudSong->Clip(index, KVedAudioTrackIndex);
            
            if (!clip->Muting())
            {

                TTimeIntervalMicroSeconds clipLength = 
                    TTimeIntervalMicroSeconds( clip->CutOutTime().Int64() - clip->CutInTime().Int64() );
                    
                // Do fading only if clip length is over 1 second
                // Otherwise the original markers remain untouched
                if ( clipLength > TTimeIntervalMicroSeconds(1000000) )                
                {
                    // get normal level
                    TInt normalLevel = 0;
                    
                    // get normal level 
                
                    // NOTE: This assumes that the client has set a constant 
                    // gain value for the whole clip, i.e. two markers,
                    // one at the beginning and other at the end        
                    // iApplyDynamicLevelMark now controls that removing others is safe
                
                    normalLevel = clip->GetVolumeGain();
                    
                    // remove all existing markers               
                    while (clip->DynamicLevelMarkCount())
                    {                    
                        clip->RemoveDynamicLevelMark(0);                 
                    }
                    // add movie-level volume gain
                    normalLevel += iVolumeGainForAllAudioClips;
                    
                    // use normal - 50dB as lower level; step in values is 0.5db
                    TInt lowLevel = normalLevel - 100;
                    if ( lowLevel < -127 )
                        {
                        lowLevel = -127;
                        }
                
                    // 0s: low
                    TAudDynamicLevelMark mark1(clip->CutInTime(), lowLevel);
                    
                    // 0.5s: normal
                    TTimeIntervalMicroSeconds time = 
                        TTimeIntervalMicroSeconds(clip->CutInTime().Int64() + 
                                                  TInt64(500000) );
                                
                    TAudDynamicLevelMark mark2(time, normalLevel);
                    
                    // clip length - 0.5s: normal
                    time = TTimeIntervalMicroSeconds(clip->CutOutTime().Int64() -
                                                     TInt64(500000) );
                                                                                  
                    TAudDynamicLevelMark mark3(time, normalLevel);                                    
                    
                    // clip length: low
                    TAudDynamicLevelMark mark4(clip->CutOutTime(), lowLevel);
                                              
                    clip->InsertDynamicLevelMarkL(mark1);
                    clip->InsertDynamicLevelMarkL(mark2);
                    clip->InsertDynamicLevelMarkL(mark3);
                    clip->InsertDynamicLevelMarkL(mark4);
                }
            }
        }
        
        iNotifyObserver = ETrue;
        }
    }

void CVedMovieImp::SetOutputParametersL(TVedOutputParameters& aOutputParams)
{
    PRINT((_L("CVedMovie::SetOutputParametersL(), aOutputParams: video %d, (%dx%d), %d bps, %ffps"), aOutputParams.iVideoType, aOutputParams.iVideoResolution.iWidth, aOutputParams.iVideoResolution.iHeight, aOutputParams.iVideoBitrate, aOutputParams.iVideoFrameRate));
    PRINT((_L("CVedMovie::SetOutputParametersL(), aOutputParams: audio %d, %d bps, s/m: %d, %d Hz"), aOutputParams.iAudioType, aOutputParams.iAudioBitrate, aOutputParams.iAudioChannelMode, aOutputParams.iAudioSamplingRate));
    PRINT((_L("CVedMovie::SetOutputParametersL(), aOutputParams: video segmentation: %d GOBs, %d bytes/segment"), aOutputParams.iSyncIntervalInPicture, aOutputParams.iSegmentSizeInBytes));
    
    if (aOutputParams.iVideoType != EVedVideoTypeUnrecognized && 
        aOutputParams.iVideoType != EVedVideoTypeNoVideo)
    {
        iVideoType = aOutputParams.iVideoType;
    }
    
    if ( (aOutputParams.iVideoResolution != TSize(0, 0)) &&
         (aOutputParams.iVideoResolution != KVedResolutionSubQCIF) && 
		 (aOutputParams.iVideoResolution != KVedResolutionQCIF) &&
		 (aOutputParams.iVideoResolution != KVedResolutionCIF) && 
		 (aOutputParams.iVideoResolution != KVedResolutionQVGA) &&
		 (aOutputParams.iVideoResolution != KVedResolutionVGA16By9) &&
		 (aOutputParams.iVideoResolution != KVedResolutionVGA) &&
		 (aOutputParams.iVideoResolution != KVedResolutionWVGA) )
	{
        User::Leave(KErrNotSupported);
    }
    
#ifndef VIDEOEDITORENGINE_AVC_EDITING   
    if ( iVideoType == EVedVideoTypeAVCBaselineProfile )
        User::Leave(KErrNotSupported);
#endif

    // Allow AVC & MPEG-4 QVGA/CIF
    if ( ( aOutputParams.iVideoResolution == KVedResolutionCIF ) ||
         ( aOutputParams.iVideoResolution == KVedResolutionQVGA ) )
    {
        if ( iVideoType == EVedVideoTypeH263Profile0Level10 ||
             iVideoType == EVedVideoTypeH263Profile0Level45 )
            User::Leave(KErrNotSupported);
        
    }
    // Allow MPEG VGA
    if ( aOutputParams.iVideoResolution == KVedResolutionVGA )
    {
        if ( iVideoType != EVedVideoTypeMPEG4SimpleProfile )
            User::Leave(KErrNotSupported);        
    }            

    if (aOutputParams.iVideoResolution != TSize(0,0))
        iResolution = aOutputParams.iVideoResolution;

    switch (iVideoType)
        {
        case EVedVideoTypeH263Profile0Level10:

            // check bitrate
            if ( aOutputParams.iVideoBitrate > KVedBitRateH263Level10 )
                User::Leave(KErrNotSupported);
            iVideoCodecMimeType.Set(KVedMimeTypeH263);
            break;

        case EVedVideoTypeH263Profile0Level45:

            // check bitrate
            if ( aOutputParams.iVideoBitrate > KVedBitRateH263Level45 )
                User::Leave(KErrNotSupported);                        
            iVideoCodecMimeType.Set(KVedMimeTypeH263Level45);
            break;

        case EVedVideoTypeMPEG4SimpleProfile:

            // check bitrate
            if ( iResolution == KVedResolutionCIF || iResolution == KVedResolutionQVGA )
                {
                if ( aOutputParams.iVideoBitrate > KVedBitRateMPEG4Level2 )
                    User::Leave(KErrNotSupported);
                iVideoCodecMimeType.Set(KVedMimeTypeMPEG4SimpleVisualProfileLevel2);
                }
                
           else if ( iResolution == KVedResolutionVGA || iResolution == KVedResolutionVGA16By9 )
               {
               if ( aOutputParams.iVideoBitrate > KVedBitRateMPEG4Level4A )
                   User::Leave(KErrNotSupported);           	
               iVideoCodecMimeType.Set(KVedMimeTypeMPEG4SimpleVisualProfileLevel4A);
               }

            else
                {
                if ( aOutputParams.iVideoBitrate > KVedBitRateMPEG4Level0 )
                    User::Leave(KErrNotSupported);
                iVideoCodecMimeType.Set(KVedMimeTypeMPEG4SimpleVisualProfile);
                }
            break;
            
        case EVedVideoTypeAVCBaselineProfile:

            // check bitrate
        	//WVGA task
        	if ( iResolution == KVedResolutionWVGA )
        	{
        		iVideoCodecMimeType.Set(KVedMimeTypeAVCBaselineProfileLevel3_1);
        	}
        		
        	else if ( iResolution == KVedResolutionCIF || iResolution == KVedResolutionQVGA )
            {
                if ( aOutputParams.iVideoBitrate > KVedBitRateAVCLevel1_2 )
                    User::Leave(KErrNotSupported);
                if ( aOutputParams.iVideoBitrate <= KVedBitRateAVCLevel1_1 )
                    iVideoCodecMimeType.Set(KVedMimeTypeAVCBaselineProfileLevel1_1);
                else
                    iVideoCodecMimeType.Set(KVedMimeTypeAVCBaselineProfileLevel1_2);
            }
            else
            {
                if ( aOutputParams.iVideoBitrate > KVedBitRateAVCLevel1b )
                    User::Leave(KErrNotSupported);
                if ( aOutputParams.iVideoBitrate <= KVedBitRateAVCLevel1 )                
                    iVideoCodecMimeType.Set(KVedMimeTypeAVCBaselineProfileLevel1);
                else
                    iVideoCodecMimeType.Set(KVedMimeTypeAVCBaselineProfileLevel1B);
            }
        break;

        default:
            User::Leave(KErrArgument);

        }
        
    // check if the selected codec/resolution is a supported format
    TBool supported = iCodecChecker->IsSupportedOutputFormatL(iVideoCodecMimeType, iResolution);    
    if ( !supported )
    {
        PRINT(_L("CVedMovie::SetOutputParametersL(), format not supported"));        
        User::Leave(KErrNotSupported);
    }   
    
    // can be zero => not used 
    iVideoStandardBitrate = iVideoRestrictedBitrate = aOutputParams.iVideoBitrate;

    // check frame rate
    if (aOutputParams.iVideoFrameRate > KVedMaxVideoFrameRate)
        User::Leave(KErrNotSupported);

    // default random access rate is 1/5 fps
    iRandomAccessRate = 0.2;
    
    // can be zero => not used 
    iVideoFrameRate = aOutputParams.iVideoFrameRate;
    
    if (aOutputParams.iAudioType == EVedAudioTypeNoAudio)
        {
        // use default audio for this video
        switch (iVideoType)
            {
            case EVedVideoTypeH263Profile0Level10:
            case EVedVideoTypeH263Profile0Level45:
                aOutputParams.iAudioType = EVedAudioTypeAMR;
            break;

            case EVedVideoTypeMPEG4SimpleProfile:
            case EVedVideoTypeAVCBaselineProfile:
            default:
                aOutputParams.iAudioType = EVedAudioTypeAAC_LC;
            break;
            }
            
        }

    if (aOutputParams.iAudioType != EVedAudioTypeAMR &&
        aOutputParams.iAudioType != EVedAudioTypeAAC_LC )
        User::Leave(KErrNotSupported);        

    if ( aOutputParams.iAudioChannelMode != EVedAudioChannelModeStereo && 
         aOutputParams.iAudioChannelMode != EVedAudioChannelModeDualChannel && 
         aOutputParams.iAudioChannelMode != EVedAudioChannelModeSingleChannel )
         User::Leave(KErrArgument);    

    if ( aOutputParams.iAudioType == EVedAudioTypeAMR )
        {
        if ( aOutputParams.iAudioBitrate != 0 &&
             aOutputParams.iAudioBitrate != KVedAMRBitrate4_75k &&
             aOutputParams.iAudioBitrate != KVedAMRBitrate5_15k &&
             aOutputParams.iAudioBitrate != KVedAMRBitrate5_90k &&
             aOutputParams.iAudioBitrate != KVedAMRBitrate6_70k &&
             aOutputParams.iAudioBitrate != KVedAMRBitrate7_40k &&
             aOutputParams.iAudioBitrate != KVedAMRBitrate7_95k &&
             aOutputParams.iAudioBitrate != KVedAMRBitrate10_2k &&
             aOutputParams.iAudioBitrate != KVedAMRBitrate12_2k ) 
            {
            User::Leave(KErrNotSupported);
            }
        
        if ( aOutputParams.iAudioSamplingRate != 0 && 
             aOutputParams.iAudioSamplingRate != KVedAudioSamplingRate8k )
            User::Leave(KErrNotSupported);

        if ( aOutputParams.iAudioChannelMode != EVedAudioChannelModeSingleChannel )
            User::Leave(KErrNotSupported);

        }

    else if (aOutputParams.iAudioType == EVedAudioTypeAAC_LC)
        {        
        if ( aOutputParams.iAudioSamplingRate != 0 &&
             aOutputParams.iAudioSamplingRate != KVedAudioSamplingRate8k &&
             aOutputParams.iAudioSamplingRate != KVedAudioSamplingRate11_025k &&
             aOutputParams.iAudioSamplingRate != KVedAudioSamplingRate12k &&
             aOutputParams.iAudioSamplingRate != KVedAudioSamplingRate16k &&
             aOutputParams.iAudioSamplingRate != KVedAudioSamplingRate22_050k &&
             aOutputParams.iAudioSamplingRate != KVedAudioSamplingRate24k &&
             aOutputParams.iAudioSamplingRate != KVedAudioSamplingRate32k &&
             aOutputParams.iAudioSamplingRate != KVedAudioSamplingRate44_1k &&
             aOutputParams.iAudioSamplingRate != KVedAudioSamplingRate48k &&
             aOutputParams.iAudioSamplingRate != KVedAudioSamplingRate64k &&
             aOutputParams.iAudioSamplingRate != KVedAudioSamplingRate88_2k &&
             aOutputParams.iAudioSamplingRate != KVedAudioSamplingRate96k )
            {
            User::Leave(KErrNotSupported);
            }

        TInt numChannels = 1;
        if ( aOutputParams.iAudioChannelMode == EVedAudioChannelModeStereo || 
             aOutputParams.iAudioChannelMode == EVedAudioChannelModeDualChannel )
             numChannels = 2;              

        TInt samplingRate = (aOutputParams.iAudioSamplingRate != 0) ? aOutputParams.iAudioSamplingRate : KVedAudioSamplingRate16k;

        TInt minBitrate = 1 * numChannels * samplingRate;
        TInt maxBitrate = 6 * numChannels * samplingRate;        

        if ( aOutputParams.iAudioBitrate > 0 && 
            (aOutputParams.iAudioBitrate < minBitrate || aOutputParams.iAudioBitrate > maxBitrate) )
            User::Leave(KErrNotSupported);

        }
    
    if ( iVideoType == EVedVideoTypeH263Profile0Level10 ||
        iVideoType == EVedVideoTypeH263Profile0Level45 )
        {
        // For H.263
        iSyncIntervalInPicture = aOutputParams.iSyncIntervalInPicture;
        
        // Segment size not supported for H.263
        if ( aOutputParams.iSegmentSizeInBytes != 0 )
            User::Leave(KErrNotSupported);
        }
    else
        {
        // For H.264 and MPEG-4
        iSyncIntervalInPicture = aOutputParams.iSegmentSizeInBytes;
        
        // Sync interval not supported for H.264 and MPEG-4
        if ( aOutputParams.iSyncIntervalInPicture != 0 )
            User::Leave(KErrNotSupported);
        }

    TAudFileProperties audioProperties;
    
    if (aOutputParams.iAudioType == EVedAudioTypeAMR)
        audioProperties.iAudioType = EAudAMR;
    
    else if (aOutputParams.iAudioType == EVedAudioTypeAAC_LC)
        audioProperties.iAudioType = EAudAAC_MPEG4;
    
    if (aOutputParams.iAudioBitrate > 0)
        audioProperties.iBitrate = aOutputParams.iAudioBitrate;
    
    if (aOutputParams.iAudioSamplingRate > 0)
        audioProperties.iSamplingRate = aOutputParams.iAudioSamplingRate;
    
    audioProperties.iChannelMode = EAudSingleChannel;
           
    if (aOutputParams.iAudioChannelMode == EVedAudioChannelModeStereo)
        audioProperties.iChannelMode = EAudStereo;    
    
    else if (aOutputParams.iAudioChannelMode == EAudDualChannel)
        audioProperties.iChannelMode = EAudDualChannel;
    
    // Set the audio output properties here to the values specified above
    if ( iAudSong->SetOutputFileFormat(audioProperties.iAudioType, audioProperties.iSamplingRate,audioProperties.iChannelMode, audioProperties.iBitrate) )
        {
        iOutputParamsSet = ETrue;
        FireMovieOutputParametersChanged(this);
        }
    else
        {
        // parameters didn't work
        User::Leave( KErrArgument );
        }
}


TInt CVedMovieImp::CheckVideoClipInsertable(CVedVideoClip *aClip) const
    {
    /* Check format and video and audio types of a file-based clip. */
    if ( !aClip )
        {
        return KErrGeneral;
        }

    if (aClip->iInfo->Class() == EVedVideoClipClassFile)
        {
        if ( (aClip->iInfo->Format() != EVedVideoFormat3GPP) && 
             (aClip->iInfo->Format() != EVedVideoFormatMP4)
            )       
            {
            return KErrNotSupported;
            }

        if ( (aClip->iInfo->VideoType() != EVedVideoTypeH263Profile0Level10) && 
             (aClip->iInfo->VideoType() != EVedVideoTypeH263Profile0Level45) && 
             (aClip->iInfo->VideoType() != EVedVideoTypeMPEG4SimpleProfile) &&
             (aClip->iInfo->VideoType() != EVedVideoTypeAVCBaselineProfile)             
            )
            {
            return KErrNotSupported;
            }

        if ( (aClip->iInfo->AudioType() != EVedAudioTypeNoAudio) && 
             (aClip->iInfo->AudioType() != EVedAudioTypeAMR) && 
             (aClip->iInfo->AudioType() != EVedAudioTypeAMRWB) &&
             (aClip->iInfo->AudioType() != EVedAudioTypeAAC_LC) )
            {
            return KErrNotSupported;
            }

        if ( (aClip->iInfo->AudioType() != EVedAudioTypeNoAudio) && aClip->iInfo->AudioChannelMode() == EVedAudioChannelModeUnrecognized )
            {
            return KErrNotSupported;
            }
        }

    if (aClip->Info()->Class() == EVedVideoClipClassFile)
    {
        TInt error = KErrNone;
        TBool supported = EFalse;
        
        // check if clip format / resolution is supported
        TRAP( error, supported = iCodecChecker->IsSupportedInputClipL(aClip) );
        
        if ( error != KErrNone )
        {
            return error;
        }
        
        if ( !supported )
        {
            PRINT(_L("CVedMovie::CheckVideoClipInsertable(), clip not supported"));
            return KErrNotSupported;            
        }
    }            

    return KErrNone;
    }


TInt CVedMovieImp::GetSizeEstimateL() const
    {
    PRINT(_L("CVedMovieImp::GetSizeEstimateL"));

    if ((VideoClipCount() == 0) && (AudioClipCount() == 0)) 
        {
        return 0;
        }

    if ( iProcessor )
        {
        return iProcessor->GetMovieSizeEstimateL(this);
        }
    else
        {
        return 0;
        }
    }

void CVedMovieImp::GetDurationEstimateL(TInt aTargetSize, 
                                        TTimeIntervalMicroSeconds aStartTime, 
                                        TTimeIntervalMicroSeconds& aEndTime)
    {
    if ((VideoClipCount() == 0) && (AudioClipCount() == 0)) 
        {
        User::Leave( KErrNotReady );
        }

    if ( iProcessor )
        {
        User::LeaveIfError( iProcessor->GetMovieSizeEstimateForMMSL(this, 
                                                                    aTargetSize,
                                                                    aStartTime,
                                                                    aEndTime) );
        }
    else
        {
        User::Leave( KErrNotReady );
        }
    }


TBool CVedMovieImp::IsMovieMMSCompatible() const
    {
    return ( ( iFormat == EVedVideoFormat3GPP ) &&
             ( iVideoType == EVedVideoTypeH263Profile0Level10 ) &&
             ( AudioType() == EVedAudioTypeAMR ) );
    }


void CVedMovieImp::InsertVideoClipL(const TDesC& aFileName, TInt aIndex)
    {
    PRINT(_L("CVedMovieImp::InsertVideoClipL"));

    __ASSERT_ALWAYS(iAddOperation->iVideoClip == 0, 
                    TVedPanic::Panic(TVedPanic::EMovieAddOperationAlreadyRunning));
    iAddedVideoClipIndex = aIndex;
    
    if (iAddedVideoClipFilename) 
        {
        delete iAddedVideoClipFilename;
        iAddedVideoClipFilename = 0;
        }

    iAddedVideoClipFilename = HBufC::NewL(aFileName.Size());
    *iAddedVideoClipFilename = aFileName;
    iAddedVideoClipFileHandle = NULL;

    // add audio clip
    TTimeIntervalMicroSeconds startTime(0);

    if (aIndex > 0) 
        {
        startTime = VideoClipEndTime(aIndex - 1);
        }
    iAudSong->AddClipL(aFileName, startTime);
    }

void CVedMovieImp::InsertVideoClipL(RFile* aFileHandle, TInt aIndex)
    {
    PRINT(_L("CVedMovieImp::InsertVideoClipL"));

    __ASSERT_ALWAYS(iAddOperation->iVideoClip == 0, 
                    TVedPanic::Panic(TVedPanic::EMovieAddOperationAlreadyRunning));
    iAddedVideoClipIndex = aIndex;
    
    if (iAddedVideoClipFilename) 
        {
        delete iAddedVideoClipFilename;
        iAddedVideoClipFilename = 0;
        }

    iAddedVideoClipFileHandle = aFileHandle;

    // add audio clip
    TTimeIntervalMicroSeconds startTime(0);

    if (aIndex > 0) 
        {
        startTime = VideoClipEndTime(aIndex - 1);
        }
    iAudSong->AddClipL(aFileHandle, startTime);
    }



void CVedMovieImp::InsertVideoClipL(CVedVideoClipGenerator& aGenerator, 
                                    TBool aIsOwnedByVideoClip, TInt aIndex)
    {
    PRINT(_L("CVedMovieImp::InsertVideoClipL"));

    __ASSERT_ALWAYS(iAddOperation->iVideoClip == 0, 
                    TVedPanic::Panic(TVedPanic::EMovieAddOperationAlreadyRunning));

    iAddOperation->iVideoClip = CVedVideoClip::NewL(this, aGenerator, 
                                                    aIndex, *iAddOperation, aIsOwnedByVideoClip);
    iAddOperation->iIsVideoGeneratorOwnedByVideoClip = aIsOwnedByVideoClip;
    }

    
void CVedMovieImp::RemoveVideoClip(TInt aIndex)
    {
    PRINT(_L("CVedMovieImp::RemoveVideoClip"));

    if ( (aIndex > VideoClipCount()) || (aIndex < 0) )
    {
        return;    
    }

    CVedVideoClip* clip = iVideoClipArray[aIndex];
    TBool hasAudio = clip->EditedHasAudio();
    if (hasAudio)
    	{
    	iAudSong->RemoveClip(clip->iAudClip->IndexOnTrack(), 0);	
    	}
    
    iVideoClipArray.Remove(aIndex);
    delete clip;
    
    if (aIndex < VideoClipCount())
        {
        clip = iVideoClipArray[aIndex];
        clip->iIndex = aIndex;
        RecalculateVideoClipTimings(clip);
        }
    else if (VideoClipCount() > 0)
        {
        clip = iVideoClipArray[aIndex - 1];
        RecalculateVideoClipTimings(clip);
        }    

    TRAPD(err,CalculatePropertiesL());// ignore error?
    if (err != KErrNone) { }

    FireVideoClipRemoved(this, aIndex);
    }

    
void CVedMovieImp::RecalculateVideoClipTimings(CVedVideoClip* aVideoClip)
    {
    TInt index = aVideoClip->iIndex;

    TInt64 startTime;
    if (index == 0)
        {
        startTime = 0;
        }
    else
        {
        startTime = iVideoClipArray[index - 1]->EndTime().Int64();
        }
    
    for (; index < VideoClipCount(); index++)
        {
        CVedVideoClip* clip = iVideoClipArray[index];
        clip->iIndex = index;
        clip->iStartTime = startTime;
        clip->UpdateAudioClip();
        startTime = clip->EndTime().Int64();
        }
    iAudSong->SetDuration(Duration());
    }


void CVedMovieImp::SetStartTransitionEffect(TVedStartTransitionEffect aEffect)
    {
    __ASSERT_ALWAYS(VideoClipCount() > 0, TVedPanic::Panic(TVedPanic::EMovieEmpty));
    __ASSERT_ALWAYS((aEffect >= EVedStartTransitionEffectNone) 
                    && (aEffect < EVedStartTransitionEffectLast), 
                    TVedPanic::Panic(TVedPanic::EMovieIllegalStartTransitionEffect));

    if (aEffect != iStartTransitionEffect)
        {
        iStartTransitionEffect = aEffect;
        FireStartTransitionEffectChanged(this);
        }
    }


void CVedMovieImp::SetMiddleTransitionEffect(TVedMiddleTransitionEffect aEffect,
                                                   TInt aIndex)
    {
    __ASSERT_ALWAYS((aEffect >= EVedMiddleTransitionEffectNone) 
                    && (aEffect < EVedMiddleTransitionEffectLast), 
                    TVedPanic::Panic(TVedPanic::EMovieIllegalMiddleTransitionEffect));

    if (aIndex == (iVideoClipArray.Count() - 1))
        {
        aIndex++;   // make aIndex out of range to cause panic
        }

    if (aEffect != iVideoClipArray[aIndex]->iMiddleTransitionEffect)
        {
        iVideoClipArray[aIndex]->iMiddleTransitionEffect = aEffect;
        FireMiddleTransitionEffectChanged(this, aIndex);
        }
    }


void CVedMovieImp::SetEndTransitionEffect(TVedEndTransitionEffect aEffect)
    {
    __ASSERT_ALWAYS(VideoClipCount() > 0, TVedPanic::Panic(TVedPanic::EMovieEmpty));
    __ASSERT_ALWAYS((aEffect >= EVedEndTransitionEffectNone) 
                    && (aEffect < EVedEndTransitionEffectLast), 
                    TVedPanic::Panic(TVedPanic::EMovieIllegalEndTransitionEffect));

    if (aEffect != iEndTransitionEffect)
        {
        iEndTransitionEffect = aEffect;
        FireEndTransitionEffectChanged(this);
        }
    }


void CVedMovieImp::AddAudioClipL(const TDesC& aFileName,
                                 TTimeIntervalMicroSeconds aStartTime,
                                 TTimeIntervalMicroSeconds aCutInTime,
                                 TTimeIntervalMicroSeconds aCutOutTime)
    {
    __ASSERT_ALWAYS(iAddOperation->iVideoClip == 0, 
                    TVedPanic::Panic(TVedPanic::EMovieAddOperationAlreadyRunning));

    iAudSong->AddClipL(aFileName, aStartTime, KVedAudioTrackIndex, aCutInTime, aCutOutTime);
    }
    
void CVedMovieImp::AddAudioClipL(RFile* aFileHandle,
                                 TTimeIntervalMicroSeconds aStartTime,
                                 TTimeIntervalMicroSeconds aCutInTime,
                                 TTimeIntervalMicroSeconds aCutOutTime)
    {
    __ASSERT_ALWAYS(iAddOperation->iVideoClip == 0, 
                    TVedPanic::Panic(TVedPanic::EMovieAddOperationAlreadyRunning));

    iAudSong->AddClipL(aFileHandle, aStartTime, KVedAudioTrackIndex, aCutInTime, aCutOutTime);
    }


void CVedMovieImp::RemoveAudioClip(TInt aIndex)
    {
    iAudSong->RemoveClip(aIndex, KVedAudioTrackIndex);
    }

void CVedMovieImp::Reset()
    {
    DoReset();

    FireMovieReseted(this);
    }


void CVedMovieImp::DoReset()
    {
    iVideoClipArray.ResetAndDestroy();
    iAudioClipInfoArray.ResetAndDestroy();

    iStartTransitionEffect = EVedStartTransitionEffectNone;
    iEndTransitionEffect = EVedEndTransitionEffectNone;
    
    if (iAudSong != 0)
        {
        iAudSong->Reset(EFalse);
        }
    }


void CVedMovieImp::RegisterMovieObserverL(MVedMovieObserver* aObserver)
    {
    PRINT(_L("CVedMovieImp::RegisterMovieObserverL"));

    for (TInt i = 0; i < iObserverArray.Count(); i++)
        {
        if (iObserverArray[i] == aObserver)
            {
            TVedPanic::Panic(TVedPanic::EMovieObserverAlreadyRegistered);
            }
        }

    User::LeaveIfError(iObserverArray.Append(aObserver));
    }


void CVedMovieImp::UnregisterMovieObserver(MVedMovieObserver* aObserver)
    {
    PRINT(_L("CVedMovieImp::UnregisterMovieObserver"));

    for (TInt i = 0; i < iObserverArray.Count(); i++)
        {
        if (iObserverArray[i] == aObserver)
            {
            iObserverArray.Remove(i);
            return;
            }
        }
    }
    
TBool CVedMovieImp::MovieObserverIsRegistered(MVedMovieObserver* aObserver)
    {
    PRINT(_L("CVedMovieImp::MovieObserverIsRegistered"));

    for (TInt i = 0; i < iObserverArray.Count(); i++)
        {
        if (iObserverArray[i] == aObserver)
            {
            return ETrue;
            }
        }
        
    return EFalse;
    }

CVedVideoClipInfo* CVedMovieImp::VideoClipInfo(TInt aIndex) const
    {
    return iVideoClipArray[aIndex]->Info();
    }

TBool CVedMovieImp::VideoClipEditedHasAudio(TInt aIndex) const
    {
    return iVideoClipArray[aIndex]->EditedHasAudio();
    }

void CVedMovieImp::VideoClipSetIndex(TInt aOldIndex, TInt aNewIndex)
    {
    iVideoClipArray[aOldIndex]->SetIndex(aNewIndex);
    }

TInt CVedMovieImp::VideoClipSpeed(TInt aIndex) const
    {
    return iVideoClipArray[aIndex]->Speed();   
    }

void CVedMovieImp::VideoClipSetSpeed(TInt aIndex, TInt aSpeed)
    {
    iVideoClipArray[aIndex]->SetSpeed(aSpeed);
    }

TVedColorEffect CVedMovieImp::VideoClipColorEffect(TInt aIndex) const
    {
    return iVideoClipArray[aIndex]->ColorEffect();     
    }

void CVedMovieImp::VideoClipSetColorEffect(TInt aIndex, TVedColorEffect aColorEffect)
    {
    iVideoClipArray[aIndex]->SetColorEffect(aColorEffect);
    }

TBool CVedMovieImp::VideoClipIsMuteable(TInt aIndex) const
    {
    return iVideoClipArray[aIndex]->IsMuteable();
    }

TBool CVedMovieImp::VideoClipIsMuted(TInt aIndex) const
    {
    return iVideoClipArray[aIndex]->IsMuted();
    }

void CVedMovieImp::VideoClipSetMuted(TInt aIndex, TBool aMuted)
    {
    iVideoClipArray[aIndex]->SetMuted(aMuted);
    }
    
TBool CVedMovieImp::VideoClipNormalizing(TInt aIndex) const
    {
    return iVideoClipArray[aIndex]->Normalizing();
    }

void CVedMovieImp::VideoClipSetNormalizing(TInt aIndex, TBool aNormalizing)
    {
    iVideoClipArray[aIndex]->SetNormalizing(aNormalizing);
    }
           
void CVedMovieImp::VideoClipInsertDynamicLevelMarkL(TInt aIndex, TVedDynamicLevelMark aMark)
    {
    __ASSERT_ALWAYS(aIndex >= 0, 
                    TVedPanic::Panic(TVedPanic::EVideoClipIllegalIndex));

    iApplyDynamicLevelMark = ETrue;
    iVideoClipArray[aIndex]->InsertDynamicLevelMarkL(aMark);
    }

void CVedMovieImp::VideoClipRemoveDynamicLevelMark(TInt aClipIndex, TInt aMarkIndex)
    {
    __ASSERT_ALWAYS(aClipIndex >= 0, 
                    TVedPanic::Panic(TVedPanic::EVideoClipIllegalIndex));
    
    iVideoClipArray[aClipIndex]->RemoveDynamicLevelMark(aMarkIndex);
    }

TInt CVedMovieImp::VideoClipDynamicLevelMarkCount(TInt aIndex) const
    {
    __ASSERT_ALWAYS(aIndex >= 0, 
                    TVedPanic::Panic(TVedPanic::EVideoClipIllegalIndex));

    return iVideoClipArray[aIndex]->DynamicLevelMarkCount();
    }

TVedDynamicLevelMark CVedMovieImp::VideoClipDynamicLevelMark(TInt aClipIndex, TInt aMarkIndex)
    {
    __ASSERT_ALWAYS(aClipIndex >= 0, 
                    TVedPanic::Panic(TVedPanic::EVideoClipIllegalIndex));

    return iVideoClipArray[aClipIndex]->DynamicLevelMark(aMarkIndex);
    }

TTimeIntervalMicroSeconds CVedMovieImp::VideoClipCutInTime(TInt aIndex) const
    {
    return iVideoClipArray[aIndex]->CutInTime();
    }

void CVedMovieImp::VideoClipSetCutInTime(TInt aIndex, TTimeIntervalMicroSeconds aCutInTime)
    {
    iVideoClipArray[aIndex]->SetCutInTime(aCutInTime);
    // basically we should call CalculatePropertiesL after this one, but since it is called in StartMovie, we can probably skip it
    }

TTimeIntervalMicroSeconds CVedMovieImp::VideoClipCutOutTime(TInt aIndex) const
    {
    return iVideoClipArray[aIndex]->CutOutTime();
    }

void CVedMovieImp::VideoClipSetCutOutTime(TInt aIndex, TTimeIntervalMicroSeconds aCutOutTime)
    {
    iVideoClipArray[aIndex]->SetCutOutTime(aCutOutTime);
    // basically we should call CalculatePropertiesL after this one, but since it is called in StartMovie, we can probably skip it
    }

TTimeIntervalMicroSeconds CVedMovieImp::VideoClipStartTime(TInt aIndex) const
    {
    return iVideoClipArray[aIndex]->StartTime();
    }

TTimeIntervalMicroSeconds CVedMovieImp::VideoClipEndTime(TInt aIndex) const
    {
    return iVideoClipArray[aIndex]->EndTime();
    }

TTimeIntervalMicroSeconds CVedMovieImp::VideoClipEditedDuration(TInt aIndex) const
    {
    return iVideoClipArray[aIndex]->EditedDuration();
    }

CVedAudioClipInfo* CVedMovieImp::AudioClipInfo(TInt aIndex) const
    {
    return iAudioClipInfoArray[aIndex];
    }

TTimeIntervalMicroSeconds CVedMovieImp::AudioClipStartTime(TInt aIndex) const
    {
    return iAudSong->Clip(aIndex, KVedAudioTrackIndex)->StartTime();
    }

void CVedMovieImp::AudioClipSetStartTime(TInt aIndex, TTimeIntervalMicroSeconds aStartTime)
    {
    iAudSong->Clip(aIndex, KVedAudioTrackIndex)->SetStartTime(aStartTime);
    }

TTimeIntervalMicroSeconds CVedMovieImp::AudioClipEndTime(TInt aIndex) const
    {
    return iAudSong->Clip(aIndex, KVedAudioTrackIndex)->EndTime();
    }

TTimeIntervalMicroSeconds CVedMovieImp::AudioClipEditedDuration(TInt aIndex) const
    {
    return iAudSong->Clip(aIndex, KVedAudioTrackIndex)->EditedDuration();
    }

void CVedMovieImp::AudioClipSetCutInTime(TInt aIndex, TTimeIntervalMicroSeconds aCutInTime)
    {
    iAudSong->Clip(aIndex, KVedAudioTrackIndex)->SetCutInTime(aCutInTime);
    }

void CVedMovieImp::AudioClipSetCutOutTime(TInt aIndex, TTimeIntervalMicroSeconds aCutOutTime)
    {
    iAudSong->Clip(aIndex, KVedAudioTrackIndex)->SetCutOutTime(aCutOutTime);
    }


TBool CVedMovieImp::AudioClipNormalizing(TInt aIndex) const
    {
    return iAudSong->Clip(aIndex, KVedAudioTrackIndex)->Normalizing();
    }

void CVedMovieImp::AudioClipSetNormalizing(TInt aIndex, TBool aNormalizing)
    {
    iAudSong->Clip(aIndex, KVedAudioTrackIndex)->SetNormalizing(aNormalizing);
    }


void CVedMovieImp::AudioClipInsertDynamicLevelMarkL(TInt aIndex, TVedDynamicLevelMark aMark)
    {
    __ASSERT_ALWAYS(aMark.iTime.Int64() >= 0, 
                    TVedPanic::Panic(TVedPanic::EIllegalDynamicLevelMark));
    __ASSERT_ALWAYS(aMark.iTime.Int64() <= AudioClipCutOutTime(aIndex).Int64(),
                    TVedPanic::Panic(TVedPanic::EIllegalDynamicLevelMark));
    __ASSERT_ALWAYS(aIndex >= 0, 
                    TVedPanic::Panic(TVedPanic::EVideoClipIllegalIndex));
    
    iApplyDynamicLevelMark = ETrue;
    iAudSong->Clip(aIndex, KVedAudioTrackIndex)->InsertDynamicLevelMarkL(TAudDynamicLevelMark(aMark.iTime, aMark.iLevel));
    }

void CVedMovieImp::AudioClipRemoveDynamicLevelMark(TInt aClipIndex, TInt aMarkIndex)
    {
    __ASSERT_ALWAYS(aMarkIndex >= 0, 
                    TVedPanic::Panic(TVedPanic::EIllegalDynamicLevelMarkIndex));
    __ASSERT_ALWAYS(aMarkIndex < AudioClipDynamicLevelMarkCount(aClipIndex),
                    TVedPanic::Panic(TVedPanic::EIllegalDynamicLevelMarkIndex));
    __ASSERT_ALWAYS(aClipIndex >= 0, 
                    TVedPanic::Panic(TVedPanic::EVideoClipIllegalIndex));

    iAudSong->Clip(aClipIndex, KVedAudioTrackIndex)->RemoveDynamicLevelMark(aMarkIndex);
    }

TInt CVedMovieImp::AudioClipDynamicLevelMarkCount(TInt aIndex) const
    {
    __ASSERT_ALWAYS(aIndex >= 0, 
                    TVedPanic::Panic(TVedPanic::EVideoClipIllegalIndex));
    return iAudSong->Clip(aIndex, KVedAudioTrackIndex)->DynamicLevelMarkCount();
    }

TVedDynamicLevelMark CVedMovieImp::AudioClipDynamicLevelMark(TInt aClipIndex, TInt aMarkIndex)
    {
    __ASSERT_ALWAYS(aMarkIndex >= 0, 
                    TVedPanic::Panic(TVedPanic::EIllegalDynamicLevelMarkIndex));
    __ASSERT_ALWAYS(aMarkIndex < AudioClipDynamicLevelMarkCount(aClipIndex),
                    TVedPanic::Panic(TVedPanic::EIllegalDynamicLevelMarkIndex));
    __ASSERT_ALWAYS(aClipIndex >= 0, 
                    TVedPanic::Panic(TVedPanic::EVideoClipIllegalIndex));
                   
    CAudClip* clip = iAudSong->Clip(aClipIndex, KVedAudioTrackIndex);
    TAudDynamicLevelMark mark(clip->DynamicLevelMark(aMarkIndex));
    return TVedDynamicLevelMark(mark.iTime, mark.iLevel);
    }


void CVedMovieImp::SetVideoClipVolumeGainL(TInt aClipIndex, TInt aVolumeGain)
    {
    iApplyDynamicLevelMark = EFalse;
    
    TInt marks = 0;
    if (aClipIndex == KVedClipIndexAll)
        {
        // whole movie, sum with clip values in SetAudioFadingL
        iVolumeGainForAllVideoClips = aVolumeGain;
        marks = 1;
        }
    else
        {
        __ASSERT_ALWAYS(aClipIndex >= 0, 
                        TVedPanic::Panic(TVedPanic::EVideoClipIllegalIndex));
        __ASSERT_ALWAYS(aClipIndex <= VideoClipCount(), 
                        TVedPanic::Panic(TVedPanic::EVideoClipIllegalIndex));
        // set volume gain to the clip
        CVedVideoClip* clip = iVideoClipArray[aClipIndex];
        clip->SetVolumeGain(aVolumeGain);
        
        // should also add/remove a dynamic level mark to have the observer calls more meaningful 
        // and avoid problems if client assumes a mark was really added
        // the actual mark is removed when starting to process
        if ( aVolumeGain != 0 )
            {
            // add
            clip->InsertDynamicLevelMarkL(TVedDynamicLevelMark(clip->CutInTime(),aVolumeGain));
            clip->InsertDynamicLevelMarkL(TVedDynamicLevelMark(clip->CutOutTime(),aVolumeGain));
            }
        else
            {
            // gain == 0 => remove marks
            while (clip->DynamicLevelMarkCount() > 0)
                {
                marks++;
                clip->RemoveDynamicLevelMark(0);
                }
            }
        }
        
    // inform observers
    if ( aVolumeGain == 0 )
        {
        // volume gain was set to 0 => no gain ~= remove level mark
        for (TInt i = 0; i < iObserverArray.Count(); i++)
            {
            for ( TInt j = 0; j < marks; j++ )
                {
                iObserverArray[i]->NotifyVideoClipDynamicLevelMarkRemoved(*this, aClipIndex,0);
                }
            }
        }
    else
        {
        // volume gain was set to nonzero => gain ~= insert level mark
        for (TInt i = 0; i < iObserverArray.Count(); i++)
            {
            iObserverArray[i]->NotifyVideoClipDynamicLevelMarkInserted(*this, aClipIndex, 0);
            iObserverArray[i]->NotifyVideoClipDynamicLevelMarkInserted(*this, aClipIndex, 1);
            }
        }
    }

TInt CVedMovieImp::GetVideoClipVolumeGainL(TInt aClipIndex)
    {
    if (aClipIndex == KVedClipIndexAll)
        {
        return iVolumeGainForAllVideoClips;
        }
    else
        {
        // get volume gain from the clip
        __ASSERT_ALWAYS(aClipIndex >= 0, 
                        TVedPanic::Panic(TVedPanic::EVideoClipIllegalIndex));
        __ASSERT_ALWAYS(aClipIndex <= VideoClipCount(), 
                        TVedPanic::Panic(TVedPanic::EVideoClipIllegalIndex));
        return iVideoClipArray[aClipIndex]->GetVolumeGain();
        }
    }

void CVedMovieImp::SetAudioClipVolumeGainL(TInt aClipIndex, TInt aVolumeGain)
    {
    iApplyDynamicLevelMark = EFalse;
    TInt marks = 0;
    if (aClipIndex == KVedClipIndexAll)
        {
        // whole movie, sum with clip values in SetAudioFading
        iVolumeGainForAllAudioClips = aVolumeGain;
        marks = 1;
        }
    else
        {
        // set volume gain to the clip; the clip asserts the index
        CAudClip* clip = iAudSong->Clip(aClipIndex, KVedAudioTrackIndex);
        clip->SetVolumeGain(aVolumeGain);
        
        // should also add/remove a dynamic level mark to have the observer calls more meaningful 
        // and avoid problems if client assumes a mark was really added
        // the actual mark is removed when starting to process
        if ( aVolumeGain != 0 )
            {
            // add
            clip->InsertDynamicLevelMarkL(TAudDynamicLevelMark(clip->CutInTime(),aVolumeGain));
            clip->InsertDynamicLevelMarkL(TAudDynamicLevelMark(clip->CutOutTime(),aVolumeGain));
            }
        else
            {
            // gain == 0 => remove
            while (clip->DynamicLevelMarkCount() > 0)
                {
                marks++;
                clip->RemoveDynamicLevelMark(0);
                }
            }
        }
        
    // inform observers
    if ( aVolumeGain == 0 )
        {
        // volume gain was set to 0 => no gain ~= remove level mark
        for (TInt i = 0; i < iObserverArray.Count(); i++)
            {
            for ( TInt j = 0; j < marks; j++ )
                {
                iObserverArray[i]->NotifyAudioClipDynamicLevelMarkRemoved(*this, aClipIndex,0);
                }
            }
        }
    else
        {
        // volume gain was set to nonzero => gain ~= insert level mark
        for (TInt i = 0; i < iObserverArray.Count(); i++)
            {
            iObserverArray[i]->NotifyAudioClipDynamicLevelMarkInserted(*this, aClipIndex,0);
            }
        }
    }

TInt CVedMovieImp::GetAudioClipVolumeGainL(TInt aClipIndex)
    {
    if (aClipIndex == KVedClipIndexAll)
        {
        return iVolumeGainForAllAudioClips;
        }
    else
        {
        // get volume gain from the clip; the clip asserts the index
        CAudClip* clip = iAudSong->Clip(aClipIndex, KVedAudioTrackIndex);
        return clip->GetVolumeGain();
        }
    }


TRgb CVedMovieImp::VideoClipColorTone(TInt aVideoClipIndex) const
    {    
    return iVideoClipArray[aVideoClipIndex]->ColorTone();            
    }

void CVedMovieImp::VideoClipSetColorTone(TInt aVideoClipIndex, TRgb aColorTone)   
    {
    iVideoClipArray[aVideoClipIndex]->SetColorTone(aColorTone);    
    }
    
    
TTimeIntervalMicroSeconds CVedMovieImp::Duration() const
    {
    TTimeIntervalMicroSeconds duration = TTimeIntervalMicroSeconds(0);

    if (VideoClipCount() > 0)
        {
        duration = iVideoClipArray[VideoClipCount() - 1]->EndTime();
        }

    for (TInt i = 0; i < AudioClipCount(); i++)
        {
        TTimeIntervalMicroSeconds endTime = iAudSong->Clip(i, KVedAudioTrackIndex)->EndTime();
        if (endTime > duration)
            {
            duration = endTime;
            }
        }

    return duration;
    }

    
TInt CVedMovieImp::VideoClipCount() const
    {
    return iVideoClipArray.Count();
    }


CVedVideoClip* CVedMovieImp::VideoClip(TInt aClipIndex) const
    {
    return iVideoClipArray[aClipIndex];
    }


TVedStartTransitionEffect CVedMovieImp::StartTransitionEffect() const
    {
    __ASSERT_ALWAYS(VideoClipCount() > 0, TVedPanic::Panic(TVedPanic::EMovieEmpty));
    return iStartTransitionEffect;
    }


TInt CVedMovieImp::MiddleTransitionEffectCount() const
    {
    return Max(0, iVideoClipArray.Count() - 1);
    }


TVedMiddleTransitionEffect CVedMovieImp::MiddleTransitionEffect(TInt aIndex) const
    {
    if (aIndex == (iVideoClipArray.Count() - 1))
        {
        aIndex++;    // make aIndex out of range to cause panic
        }
        
    return iVideoClipArray[aIndex]->iMiddleTransitionEffect;
    }


TVedEndTransitionEffect CVedMovieImp::EndTransitionEffect() const
    {
    __ASSERT_ALWAYS(VideoClipCount() > 0, TVedPanic::Panic(TVedPanic::EMovieEmpty));
    return iEndTransitionEffect;
    }


TInt CVedMovieImp::AudioClipCount() const
    {
    return iAudSong->ClipCount(KVedAudioTrackIndex);
    }

TTimeIntervalMicroSeconds CVedMovieImp::AudioClipCutInTime(TInt aIndex) const
    {
    return iAudSong->Clip(aIndex, KVedAudioTrackIndex)->CutInTime();
    }

TTimeIntervalMicroSeconds CVedMovieImp::AudioClipCutOutTime(TInt aIndex) const
    {
    return iAudSong->Clip(aIndex, KVedAudioTrackIndex)->CutOutTime();
    }



void CVedMovieImp::FireVideoClipAdded(CVedMovie* aMovie, CVedVideoClip* aClip)
    {
    for (TInt i = 0; i < iObserverArray.Count(); i++)
        {
        iObserverArray[i]->NotifyVideoClipAdded(*aMovie, aClip->Index());
        }
    // reset the estimated processing time
    iEstimatedProcessingTime = TTimeIntervalMicroSeconds(0);
    }


void CVedMovieImp::FireVideoClipAddingFailed(CVedMovie* aMovie, TInt aError)
    {
    for (TInt i = 0; i < iObserverArray.Count(); i++)
        {
        iObserverArray[i]->NotifyVideoClipAddingFailed(*aMovie, aError);
        }
    }


void CVedMovieImp::FireVideoClipRemoved(CVedMovie* aMovie, TInt aIndex)
    {
    for (TInt i = 0; i < iObserverArray.Count(); i++)
        {
        iObserverArray[i]->NotifyVideoClipRemoved(*aMovie, aIndex);
        }
    // reset the estimated processing time
    iEstimatedProcessingTime = TTimeIntervalMicroSeconds(0);
    }


void CVedMovieImp::FireVideoClipIndicesChanged(CVedMovie* aMovie, TInt aOldIndex, 
                                            TInt aNewIndex)
    {
    for (TInt i = 0; i < iObserverArray.Count(); i++)
        {
        iObserverArray[i]->NotifyVideoClipIndicesChanged(*aMovie, aOldIndex, aNewIndex);
        }
    }


void CVedMovieImp::FireVideoClipTimingsChanged(CVedMovie* aMovie, CVedVideoClip* aClip)
    {
    for (TInt i = 0; i < iObserverArray.Count(); i++)
        {
        iObserverArray[i]->NotifyVideoClipTimingsChanged(*aMovie, aClip->Index());
        }
    // reset the estimated processing time
    iEstimatedProcessingTime = TTimeIntervalMicroSeconds(0);
    }


void CVedMovieImp::FireVideoClipColorEffectChanged(CVedMovie* aMovie, CVedVideoClip* aClip)
    {
    for (TInt i = 0; i < iObserverArray.Count(); i++)
        {
        iObserverArray[i]->NotifyVideoClipColorEffectChanged(*aMovie, aClip->Index());
        }
    }


void CVedMovieImp::FireVideoClipAudioSettingsChanged(CVedMovie* aMovie, CVedVideoClip* aClip)
    {
    for (TInt i = 0; i < iObserverArray.Count(); i++)
        {
        iObserverArray[i]->NotifyVideoClipAudioSettingsChanged(*aMovie, aClip->Index());
        }
    // reset the estimated processing time
    iEstimatedProcessingTime = TTimeIntervalMicroSeconds(0);
    }


void CVedMovieImp::FireVideoClipGeneratorSettingsChanged(CVedMovie* aMovie, CVedVideoClip* aClip)
    {
    for (TInt i = 0; i < iObserverArray.Count(); i++)
        {
        iObserverArray[i]->NotifyVideoClipGeneratorSettingsChanged(*aMovie, aClip->Index());
        }
    // reset the estimated processing time
    iEstimatedProcessingTime = TTimeIntervalMicroSeconds(0);
    }

void CVedMovieImp::FireVideoClipDescriptiveNameChanged(CVedMovie* aMovie, CVedVideoClip* aClip)
    {
    for (TInt i = 0; i < iObserverArray.Count(); i++)
        {
        iObserverArray[i]->NotifyVideoClipDescriptiveNameChanged(*aMovie, aClip->Index());
        }
    }

void CVedMovieImp::FireStartTransitionEffectChanged(CVedMovie* aMovie)
    {
    for (TInt i = 0; i < iObserverArray.Count(); i++)
        {
        iObserverArray[i]->NotifyStartTransitionEffectChanged(*aMovie);
        }
    // reset the estimated processing time
    iEstimatedProcessingTime = TTimeIntervalMicroSeconds(0);
    }


void CVedMovieImp::FireMiddleTransitionEffectChanged(CVedMovie* aMovie, TInt aIndex)
    {
    for (TInt i = 0; i < iObserverArray.Count(); i++)
        {
        iObserverArray[i]->NotifyMiddleTransitionEffectChanged(*aMovie, aIndex);
        }
    // reset the estimated processing time
    iEstimatedProcessingTime = TTimeIntervalMicroSeconds(0);
    }


void CVedMovieImp::FireEndTransitionEffectChanged(CVedMovie* aMovie)
    {
    for (TInt i = 0; i < iObserverArray.Count(); i++)
        {
        iObserverArray[i]->NotifyEndTransitionEffectChanged(*aMovie);
        }
    // reset the estimated processing time
    iEstimatedProcessingTime = TTimeIntervalMicroSeconds(0);
    }


void CVedMovieImp::FireAudioClipAdded(CVedMovie* aMovie, TInt aIndex)
    {
    for (TInt i = 0; i < iObserverArray.Count(); i++)
        {
        iObserverArray[i]->NotifyAudioClipAdded(*aMovie, aIndex);
        }
        
    TRAPD(err,CalculatePropertiesL());// ignore error?
    if (err != KErrNone) { }

    // reset the estimated processing time
    iEstimatedProcessingTime = TTimeIntervalMicroSeconds(0);
    }


void CVedMovieImp::FireAudioClipAddingFailed(CVedMovie* aMovie, TInt aError)
    {
    for (TInt i = 0; i < iObserverArray.Count(); i++)
        {
        iObserverArray[i]->NotifyAudioClipAddingFailed(*aMovie, aError);
        }
    }


void CVedMovieImp::FireAudioClipRemoved(CVedMovie* aMovie, TInt aIndex)
    {
    for (TInt i = 0; i < iObserverArray.Count(); i++)
        {
        iObserverArray[i]->NotifyAudioClipRemoved(*aMovie, aIndex);
        }

    TRAPD(err,CalculatePropertiesL());// ignore error?
    if (err != KErrNone) { }

    // reset the estimated processing time
    iEstimatedProcessingTime = TTimeIntervalMicroSeconds(0);
    }


void CVedMovieImp::FireAudioClipIndicesChanged(CVedMovie* aMovie, TInt aOldIndex, TInt aNewIndex)
    {
    for (TInt i = 0; i < iObserverArray.Count(); i++)
        {
        iObserverArray[i]->NotifyAudioClipIndicesChanged(*aMovie, 
                                                         aOldIndex, aNewIndex);
        }
    }


void CVedMovieImp::FireAudioClipTimingsChanged(CVedMovie* aMovie, CAudClip* aClip)
    {
    for (TInt i = 0; i < iObserverArray.Count(); i++)
        {
        iObserverArray[i]->NotifyAudioClipTimingsChanged(*aMovie, aClip->IndexOnTrack());
        }
    // reset the estimated processing time
    iEstimatedProcessingTime = TTimeIntervalMicroSeconds(0);
    }


void CVedMovieImp::FireDynamicLevelMarkRemoved(CAudClip& aClip, TInt aMarkIndex)
    {
    TInt trackIndex = aClip.TrackIndex();
    TInt clipIndex = aClip.IndexOnTrack();
    
    if (!iNotifyObserver)
        return;
    
    if (trackIndex == 0) 
        {
        for (TInt i = 0; i < iObserverArray.Count(); i++)
            {
            iObserverArray[i]->NotifyVideoClipDynamicLevelMarkRemoved(*this, clipIndex, aMarkIndex);
            }
        }
    else if (trackIndex == 1)
        {
        for (TInt i = 0; i < iObserverArray.Count(); i++)
            {
            iObserverArray[i]->NotifyAudioClipDynamicLevelMarkRemoved(*this, clipIndex, aMarkIndex);
            }
        }
    else
        {
        TVedPanic::Panic(TVedPanic::EInternal);
        }
    // reset the estimated processing time
    iEstimatedProcessingTime = TTimeIntervalMicroSeconds(0);
    }

void CVedMovieImp::FireDynamicLevelMarkInserted(CAudClip& aClip, TAudDynamicLevelMark& /*aMark*/, TInt aMarkIndex)
    {
    TInt trackIndex = aClip.TrackIndex();
    TInt clipIndex = aClip.IndexOnTrack();
    
    if (!iNotifyObserver)
        return;
    
    if (trackIndex == 0) 
        {
        for (TInt i = 0; i < iObserverArray.Count(); i++)
            {
            iObserverArray[i]->NotifyVideoClipDynamicLevelMarkInserted(*this, clipIndex, aMarkIndex);
            }
        }
    else if (trackIndex == 1)
        {
        for (TInt i = 0; i < iObserverArray.Count(); i++)
            {
            iObserverArray[i]->NotifyAudioClipDynamicLevelMarkInserted(*this, clipIndex, aMarkIndex);
            }
        }
    else
        {
        TVedPanic::Panic(TVedPanic::EInternal);
        }
    // reset the estimated processing time
    iEstimatedProcessingTime = TTimeIntervalMicroSeconds(0);
    }


void CVedMovieImp::FireMovieQualityChanged(CVedMovie* aMovie)
    {
    for (TInt i = 0; i < iObserverArray.Count(); i++)
        {
        iObserverArray[i]->NotifyMovieQualityChanged(*aMovie);
        }
    // reset the estimated processing time
    iEstimatedProcessingTime = TTimeIntervalMicroSeconds(0);
    }
    
    
void CVedMovieImp::FireMovieOutputParametersChanged(CVedMovie* aMovie)
    {

    for (TInt i = 0; i < iObserverArray.Count(); i++)
        {
        iObserverArray[i]->NotifyMovieOutputParametersChanged(*aMovie);
        }    
    // reset the estimated processing time
    iEstimatedProcessingTime = TTimeIntervalMicroSeconds(0);
    }

void CVedMovieImp::FireMovieReseted(CVedMovie* aMovie)
    {
    for (TInt i = 0; i < iObserverArray.Count(); i++)
        {
        iObserverArray[i]->NotifyMovieReseted(*aMovie);
        }
    }


void CVedMovieImp::ProcessL(const TDesC& aFileName,
                                  MVedMovieProcessingObserver& aObserver)
    {
    __ASSERT_ALWAYS((VideoClipCount() > 0) || (AudioClipCount() > 0), 
                    TVedPanic::Panic(TVedPanic::EMovieEmpty));    

    CalculatePropertiesL();
    iAudSong->SetDuration(Duration());
    SetAudioFadingL();
    
    iMovieProcessingObserver = &aObserver;
    
    iProcessor->StartMovieL(this, aFileName, NULL, &aObserver);
    }
    
void CVedMovieImp::ProcessL(RFile* aFileHandle,
                            MVedMovieProcessingObserver& aObserver)
    {
    __ASSERT_ALWAYS((VideoClipCount() > 0) || (AudioClipCount() > 0), 
                    TVedPanic::Panic(TVedPanic::EMovieEmpty));    

    CalculatePropertiesL();
    iAudSong->SetDuration(Duration());
    SetAudioFadingL();
    
    iMovieProcessingObserver = &aObserver;
    TFileName dummy;
    
    iProcessor->StartMovieL(this, dummy, aFileHandle, &aObserver);
    }


void CVedMovieImp::CancelProcessing()
    {
    TRAPD(error, iProcessor->CancelProcessingL());
    
    if (error != KErrNone)
        iMovieProcessingObserver->NotifyMovieProcessingCompleted(*this, error);
    
    }
    
void CVedMovieImp::SetMovieSizeLimit(TInt aLimit)
    {
    iProcessor->SetMovieSizeLimit(aLimit);
    }

TInt CVedMovieImp::SyncIntervalInPicture()
    {
    return iSyncIntervalInPicture;
    }
    
TReal CVedMovieImp::RandomAccessRate()
    {
    return iRandomAccessRate;
    }

// Create a temporary transcoder instance and ask for transcoding time factor for the given settings
TReal CVedMovieImp::AskComplexityFactorFromTranscoderL(CVedVideoClipInfo* aInfo, CTRTranscoder::TTROperationalMode aMode, TReal aInputFrameRate)
    {
    PRINT((_L("CVedMovieImp::AskComplexityFactorFromTranscoderL() in")));
    TReal complexityFactor;
    CTRTranscoder *tmpTranscoder;
    TTRVideoFormat formIn;
    TTRVideoFormat formOut;
    formIn.iSize = aInfo->Resolution();
    formIn.iDataType = CTRTranscoder::ETRDuCodedPicture;
    formOut.iSize = iResolution;
    formOut.iDataType = CTRTranscoder::ETRDuCodedPicture;
    TBufC8<255> inputMime;
    MapVideoCodecTypeToMime(aInfo->VideoType(), inputMime);
    
    // create temporary transcoder observer object, with input framerate as parameter, since it is asked by the transcoder
    CTrObs* tmpObs = new (ELeave) CTrObs(aInputFrameRate);
    CleanupStack::PushL(tmpObs);
    
    // create temporary transcoder instance
    tmpTranscoder = CTRTranscoder::NewL(*tmpObs);
    CleanupStack::PushL(tmpTranscoder);
    tmpTranscoder->OpenL(reinterpret_cast<MCMRMediaSink*>(1),//the sink will not be used, hence this is acceptable 
                aMode,
                inputMime,
                iVideoCodecMimeType,
                formIn, 
                formOut, 
                EFalse );

    // ask complexity/time factor from the transcoder                
    complexityFactor = tmpTranscoder->EstimateTranscodeTimeFactorL(formIn, formOut);

    CleanupStack::PopAndDestroy(tmpTranscoder);
    CleanupStack::PopAndDestroy(tmpObs);

    PRINT((_L("CVedMovieImp::AskComplexityFactorFromTranscoderL() out, complexity factor %f"), complexityFactor));
    return complexityFactor;
    }


TTimeIntervalMicroSeconds CVedMovieImp::GetProcessingTimeEstimateL()
    {    
    PRINT((_L("CVedMovieImp::GetProcessingTimeEstimateL() in")));
    TInt64 estimatedTime = 0;
    TReal complexityFactor;
    TBool transcodingImpactIncluded = EFalse;

    // Don't estimate the time if it has been asked already. 
    // This also means the result is not the remaining time but the total processing time
    if ( iEstimatedProcessingTime > 0 )
        {
        return iEstimatedProcessingTime;
        }
        
        
    // Loop through the clips in the movie and estimate the processing time for each of them
    for (TInt i = 0; i < VideoClipCount(); i++)
        {
        CVedVideoClipInfo* currentInfo = VideoClip(i)->Info();
        
        complexityFactor = 0;
        transcodingImpactIncluded = EFalse;
        
        if ( currentInfo->Class() == EVedVideoClipClassGenerated )
            {
            // only encoding for generated content. Framerate is low.
            transcodingImpactIncluded = ETrue;
            
            // open transcoder in encoding mode & ask the factor. 
            // This is in practice for generated content only. Tbd if it is worth opening the transcoder?
            complexityFactor = AskComplexityFactorFromTranscoderL(currentInfo, CTRTranscoder::EEncoding, 1.0);
            // the temporary clip is then processed like a normal input clip, add the impact
            complexityFactor += (KVedBitstreamProcessingFactor*iVideoStandardBitrate) / KVedBitstreamProcessingFactorBitrate;
            }
        else
            {
            // check if transcoding is needed
            
            // If input and output resolutions don't match, transcoder knows it need to do resolution transcoding
            // However, we need to check here in which mode we'd open the transcoder
            if ((currentInfo->Resolution() != iResolution) || (iVideoRestrictedBitrate != 0) 
                || ((currentInfo->VideoType() == EVedVideoTypeH263Profile0Level45) && (iVideoType == EVedVideoTypeH263Profile0Level10)))
                {
                // need to do resolution transcoding
                transcodingImpactIncluded = ETrue;
                
                // open transcoder in full transcoding mode & ask for the factor.
                TReal framerate = TReal(1000*currentInfo->VideoFrameCount())/(currentInfo->Duration().Int64()/1000);
                complexityFactor = AskComplexityFactorFromTranscoderL(currentInfo, CTRTranscoder::EFullTranscoding, framerate);
                }
            else 
                {
                // no full transcoding
                
                if ( currentInfo->VideoType() != iVideoType )
                    {
                    // compressed domain transcoding between H.263 and MPEG-4. Assuming they are equally complex
                    
                    complexityFactor = (KVedCompressedDomainTranscodingFactor * iVideoStandardBitrate) / KVedBitstreamProcessingFactorBitrate;
                    }
                else 
                    {
                    // Only bitstream processing + possibly cut or some other simple operation. Transition effect is estimated later
                    // It is assumed that pure decoding is needed only for such a short period of time that 
                    // the impact can be estimated here without opening processor/transcoder.
                    
                    // Bitrate has impact here. MPEG-4 is more complex to process than H.263 but bitrate is dominant. E,g, H.263 128 kbps vs MPEG4 512 kbps: 1:5 complexity; 1:4 bitrate
                    complexityFactor = (KVedBitstreamProcessingFactor*iVideoStandardBitrate) / KVedBitstreamProcessingFactorBitrate;
                    }
                    
                }
            }
            
        // Now we have the video complexity factor for the current clip. Add the contribution to the total time
        estimatedTime = estimatedTime + TInt64(complexityFactor * I64INT(VideoClip(i)->EditedDuration().Int64()));
        
        if (!transcodingImpactIncluded && (VideoClip(i)->iMiddleTransitionEffect != EVedMiddleTransitionEffectNone))
            {
            // add impact of transition effect; need decoding + encoding for a short period
            
            // open transcoder in full transcoding mode & ask for the factor.
            TReal framerate = TReal(1000*currentInfo->VideoFrameCount())/(currentInfo->Duration().Int64()/1000);
            complexityFactor = this->AskComplexityFactorFromTranscoderL(currentInfo, CTRTranscoder::EFullTranscoding, framerate);
            estimatedTime = estimatedTime + TInt64(complexityFactor * 1000000);//transition duration; 1 sec is accurate enough here
            }
        
        }

    PRINT((_L("CVedMovieImp::GetProcessingTimeEstimateL(), video part estimated to %d sec"),I64INT(estimatedTime)/1000000 ));
        
    // add audio processing time
    estimatedTime = estimatedTime + iAudSong->GetTimeEstimateL().Int64();
    
    PRINT((_L("CVedMovieImp::GetProcessingTimeEstimateL(), audio included, before rounding the estimate is %d sec"),I64INT(estimatedTime)/1000000 ));
    // estimate is not too accurate anyway; round it
    if ( estimatedTime > 600000000 )
        {
        // more than 10 minutes => round to nearest 2 min
        estimatedTime = ((estimatedTime+60000000) / 120000000) * 120000000;
        }
    else if (estimatedTime > 120000000 )
        {
        // more than 2 minutes => round to nearest 30 sec
        estimatedTime = ((estimatedTime+30000000) / 30000000) * 30000000;
        }
    else if (estimatedTime > 30000000 )
        {
        // more than 30 secs => round to nearest 10 sec
        estimatedTime = ((estimatedTime+5000000) / 10000000) * 10000000;
        }
    else
        {
        // less than 30 secs => round to nearest 2 sec
        estimatedTime = ((estimatedTime+1000000) / 2000000) * 2000000;
        }
        
    iEstimatedProcessingTime = estimatedTime;
    PRINT((_L("CVedMovieImp::GetProcessingTimeEstimateL() out, estimate is %d sec"),I64INT(estimatedTime)/1000000 ));
    return estimatedTime;
    
    }


void CVedMovieImp::NotifyClipAdded(CAudSong& aSong, CAudClip& aClip, TInt aIndex, TInt aTrackIndex)
    {
    PRINT((_L("CVedMovieImp::NotifyClipAdded() in") ));
    // Track index 0 means it's the video track - we need to create the video clip 
    if (aTrackIndex == 0) 
        {
        PRINT((_L("CVedMovieImp::NotifyClipAdded() added audio track of video clip to song successfully") ));

        TInt err;        
        if (iAddedVideoClipFileHandle)
            {            
            TRAP(err, iAddOperation->iVideoClip = CVedVideoClip::NewL(this, aClip.Info()->FileHandle(), iAddedVideoClipIndex, &aClip, *iAddOperation));
            }
        else
            {
            TRAP(err, iAddOperation->iVideoClip = CVedVideoClip::NewL(this, aClip.Info()->FileName(), iAddedVideoClipIndex, &aClip, *iAddOperation));
            }
        
        if (err != KErrNone) 
            {
            PRINT((_L("CVedMovieImp::NotifyClipAdded() creating video clip failed, removing also audio") ));
            // delete the audio clip from song
            aSong.RemoveClip(aIndex);
            
            FireVideoClipAddingFailed(this, err);
            }
        delete iAddedVideoClipFilename;
        iAddedVideoClipFilename = 0;
        iAddedVideoClipFileHandle = 0;
        }
    else if (aTrackIndex == KVedAudioTrackIndex) 
        {
        // We're on the audio track so we need to create an audio clip info
        PRINT((_L("CVedMovieImp::NotifyClipAdded() added audio track of audio clip to song successfully") ));
        CVedAudioClipInfoImp* audioClipInfo = 0;
        TRAPD(err, audioClipInfo = CVedAudioClipInfoImp::NewL(&aClip, *this));
        if (err != KErrNone) 
            {
            delete audioClipInfo;
            FireAudioClipAddingFailed(this, err);
            return;
            }
        err = iAudioClipInfoArray.Insert(audioClipInfo, aIndex);
        if (err != KErrNone) 
            {
            delete audioClipInfo;
            FireAudioClipAddingFailed(this, err);
            }
        }
    PRINT((_L("CVedMovieImp::NotifyClipAdded() out") ));
    }

void CVedMovieImp::NotifyClipAddingFailed(CAudSong& /*aSong*/, TInt aError, TInt aTrackIndex)
    {
    if (aTrackIndex == 0) 
        {
        if (aError == KErrNoAudio) 
            {
            // We can have video clips that have no audio track
            
            TInt err;
            if (iAddedVideoClipFileHandle)
                {
                TRAP(err, iAddOperation->iVideoClip = CVedVideoClip::NewL(this, iAddedVideoClipFileHandle, iAddedVideoClipIndex, NULL, *iAddOperation));
                }
            else
                {                
                TRAP(err, iAddOperation->iVideoClip = CVedVideoClip::NewL(this, *iAddedVideoClipFilename, iAddedVideoClipIndex, NULL, *iAddOperation));
                }
            if (err != KErrNone) 
                {
                FireAudioClipAddingFailed(this, aError);
                }
            }
        else
            {
            FireVideoClipAddingFailed(this, aError);
            }

        delete iAddedVideoClipFilename;
        iAddedVideoClipFilename = 0;
        }
    else if (aTrackIndex == KVedAudioTrackIndex) 
        {
        FireAudioClipAddingFailed(this, aError);
        }
    }

void CVedMovieImp::NotifyClipRemoved(CAudSong& /*aSong*/, TInt aIndex, TInt aTrackIndex)
    {
    if (aTrackIndex == KVedAudioTrackIndex) 
        {
        CVedAudioClipInfoImp* info = iAudioClipInfoArray[aIndex];
        delete info;
        iAudioClipInfoArray.Remove(aIndex);
        FireAudioClipRemoved(this, aIndex);
        }
    }

void CVedMovieImp::NotifyClipTimingsChanged(CAudSong& /*aSong*/, CAudClip& aClip)
    {
    if (aClip.TrackIndex() == KVedAudioTrackIndex) 
        {
        FireAudioClipTimingsChanged(this, &aClip);
        }
    }

void CVedMovieImp::NotifyClipIndicesChanged(CAudSong& /*aSong*/, TInt aOldIndex, TInt aNewIndex, TInt aTrackIndex)
    {
    if (aTrackIndex == KVedAudioTrackIndex) 
        {
        TLinearOrder<CVedAudioClipInfoImp> order(CVedAudioClipInfoImp::Compare);
        iAudioClipInfoArray.Sort(order);
        FireAudioClipIndicesChanged(this, aOldIndex, aNewIndex);
        }
    }

void CVedMovieImp::NotifySongReseted(CAudSong& /*aSong*/)
    {
    // nothing to do here
    }

void CVedMovieImp::NotifyClipReseted(CAudClip& /*aClip*/)
    {
    // nothing to do here
    }

void CVedMovieImp::NotifyDynamicLevelMarkInserted(CAudClip& aClip, TAudDynamicLevelMark& aMark, TInt aIndex)
    {
    FireDynamicLevelMarkInserted(aClip, aMark, aIndex);
    }

void CVedMovieImp::NotifyDynamicLevelMarkRemoved(CAudClip& aClip, TInt aIndex)
    {
    FireDynamicLevelMarkRemoved(aClip, aIndex);
    }


void CVedMovieImp::NotifyAudioClipInfoReady(CVedAudioClipInfo& aInfo, TInt aError)
    {
    if (aError != KErrNone) 
        {
        iAudioClipInfoArray.Remove(iAudioClipInfoArray.Count() - 1);
        FireAudioClipAddingFailed(this, aError);
        }
    else
        {
        CVedAudioClipInfoImp* infoImp = static_cast<CVedAudioClipInfoImp*> (&aInfo);
        FireAudioClipAdded(this, infoImp->iAudClip->IndexOnTrack());
        }
    }

CAudSong* CVedMovieImp::Song()
	{
	return iAudSong;
	}

TPtrC8& CVedMovieImp::VideoCodecMimeType()
    {
    return iVideoCodecMimeType;
    }


CVedMovieAddClipOperation* CVedMovieAddClipOperation::NewL(CVedMovie* aMovie)
    {
    PRINT(_L("CVedMovieAddClipOperation::NewL"));

    CVedMovieAddClipOperation* self = 
        new (ELeave) CVedMovieAddClipOperation(aMovie);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }


CVedMovieAddClipOperation::CVedMovieAddClipOperation(CVedMovie* aMovie)
        : CActive(EPriorityStandard), iMovie((CVedMovieImp*)aMovie)
    {

    CActiveScheduler::Add(this);
    }


void CVedMovieAddClipOperation::ConstructL()
    {
    }


CVedMovieAddClipOperation::~CVedMovieAddClipOperation()
    {
    Cancel();
    
    if (iVideoClip)
        {
        delete iVideoClip;
        iVideoClip = NULL;
        }
    }


void CVedMovieAddClipOperation::NotifyVideoClipInfoReady(CVedVideoClipInfo& /*aInfo*/, 
                                                         TInt aError)
    {

    // Cannot delete iVideoClip here, since we are in its callback function,
    // so schedule the active object to complete the add operation and 
    // delete iVideoClip if needed.

    iError = aError;

    SetActive();
    TRequestStatus* status = &iStatus;
    User::RequestComplete(status, KErrNone);
    }
void CVedMovieAddClipOperation::RunL()
    {
    __ASSERT_DEBUG((iVideoClip != 0),
                   TVedPanic::Panic(TVedPanic::EInternal));

    if (iVideoClip != 0)
        {
        CompleteAddVideoClipOperation();
        }
    
    __ASSERT_DEBUG(iVideoClip == 0,
                   TVedPanic::Panic(TVedPanic::EInternal));
    }


void CVedMovieAddClipOperation::DoCancel()
    {
    if (iVideoClip)
        {
        if (iVideoClip->EditedHasAudio())
            {
            TInt audioClipIndex = iVideoClip->iAudClip->IndexOnTrack();
            iMovie->iAudSong->RemoveClip(audioClipIndex, 0);           
            }
        delete iVideoClip;
        iVideoClip = NULL;
        iMovie->FireVideoClipAddingFailed(iMovie, KErrCancel);
        }
    }


void CVedMovieAddClipOperation::CompleteAddVideoClipOperation()
    {
    if (iError != KErrNone)
        {
        if (iVideoClip->EditedHasAudio())
            {
            TInt audioClipIndex = iVideoClip->iAudClip->IndexOnTrack();
            iMovie->iAudSong->RemoveClip(audioClipIndex, 0);
            }        
        delete iVideoClip;
        iVideoClip = 0;
        iMovie->FireVideoClipAddingFailed(iMovie, iError);        
        }
    else 
        {
        TInt insertableError = iMovie->CheckVideoClipInsertable(iVideoClip);

        if (insertableError != KErrNone)
            {
            if (iVideoClip->EditedHasAudio())
                {
                TInt audioClipIndex = iVideoClip->iAudClip->IndexOnTrack();
                iMovie->iAudSong->RemoveClip(audioClipIndex, 0);
                }
            delete iVideoClip;
            iVideoClip = 0;
            iMovie->FireVideoClipAddingFailed(iMovie, insertableError);
            }
        else
            {
            iVideoClip->iMiddleTransitionEffect = EVedMiddleTransitionEffectNone;

            if (iVideoClip->iInfo->Class() == EVedVideoClipClassFile)
                {
                iVideoClip->iCutOutTime = iVideoClip->iInfo->Duration();
                }
            else
                {
                iVideoClip->iCutOutTime = TTimeIntervalMicroSeconds(0);
                }

            TInt err = iMovie->iVideoClipArray.Insert(iVideoClip, iVideoClip->iIndex);
            if (err != KErrNone)
                {
                if (iVideoClip->EditedHasAudio())
                    {
                    // delete corresponding audio clip from song
                    TInt audioClipIndex = iVideoClip->iAudClip->IndexOnTrack();
                    iMovie->iAudSong->RemoveClip(audioClipIndex, 0);
                    }
                delete iVideoClip;
                iVideoClip = 0;
                iMovie->FireVideoClipAddingFailed(iMovie, err);            
                }
            else
                {
                if (iVideoClip->iInfo->Class() == EVedVideoClipClassGenerated)
                    {
                    iVideoClip->iInfo->Generator()->SetVideoClip(*iVideoClip, 
                        iIsVideoGeneratorOwnedByVideoClip);
                    }
                
                iMovie->RecalculateVideoClipTimings(iVideoClip);
                TRAPD(err,iMovie->CalculatePropertiesL());//ignore error, should not leave with current implementation
                if (err != KErrNone) { }

                CVedVideoClip* clip = iVideoClip;
                iVideoClip = 0;
                iMovie->FireVideoClipAdded(iMovie, clip);
                }
            }
        }
    }


