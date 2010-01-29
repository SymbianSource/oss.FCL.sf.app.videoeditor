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
* Implementation for size estimate.
*
*/


// Include Files

#include "SizeEstimate.h"
#include "movieprocessorimpl.h"
#include "AudSong.h"
#include "VedMovieImp.h"
#include "VedVideoClipInfoImp.h"
#include "VedVideoClipGenerator.h"
#include "vedaudiosettings.h"



// An assertion macro wrapper to clean up the code a bit
#define VPASSERT(x) __ASSERT_DEBUG(x, User::Panic(_L("CSizeEstimate"), CMovieProcessorImpl::EInvalidInternalState))

#ifdef _DEBUG
#include <e32svr.h>
#define PRINT(x) RDebug::Print x;
#else
#define PRINT(x)
#endif


// ================= MEMBER FUNCTIONS =======================

// -----------------------------------------------------------------------------
// CSizeEstimate::NewL
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CSizeEstimate* CSizeEstimate::NewL(CMovieProcessorImpl* aProcessor) 
    {
    CSizeEstimate* self = NewLC(aProcessor);
    CleanupStack::Pop(self);
    return self;
    }

CSizeEstimate* CSizeEstimate::NewLC(CMovieProcessorImpl* aProcessor)
    {
	CSizeEstimate* self = new (ELeave) CSizeEstimate(aProcessor);
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
    }

// -----------------------------------------------------------------------------
// CSizeEstimate::CSizeEstimate
// C++ default constructor can NOT contain any code, that
// might leave.
// -----------------------------------------------------------------------------
//
CSizeEstimate::CSizeEstimate(CMovieProcessorImpl* aProcessor)
    {
    iProcessor = aProcessor;
    }

// -----------------------------------------------------------------------------
// CSizeEstimate::~CSizeEstimate
// Destructor.
// -----------------------------------------------------------------------------
//
CSizeEstimate::~CSizeEstimate()
    {
    }

// -----------------------------------------------------------------------------
// CSizeEstimate::ConstructL
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//
void CSizeEstimate::ConstructL()
    {	
    }

// -----------------------------------------------------------------------------
// CSizeEstimate::GetMovieSizeEstimateL
// Calculates file size estimate
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CSizeEstimate::GetMovieSizeEstimateL(const CVedMovie* aMovie, TInt& aFileSize) 
    {
    TInt i = 0;
    TInt j = 0;
    
    TInt videoDataSize = 0;
    TInt audioDataSize = 0;
    TInt video3gpSize = 0;
    TInt audio3gpSize = 0;
    TInt numberOfVideoFrames = 0; 
    
    const CVedMovieImp* movie = reinterpret_cast<const CVedMovieImp *>(aMovie); 
    
    TInt movieBitrate = movie->VideoStandardBitrate();
    if (movieBitrate < KSELowBitrateLimit)
        {
        // Adjust low bit rates to slightly higher
        movieBitrate += KSELowBitrateIncrement;
        }
    
    TInt numberOfVideoClips = movie->VideoClipCount();
    
    // Calculate video data size from all clips
    for (i = 0; i < numberOfVideoClips; i++) 
        {
        CVedVideoClip* vedClip = movie->VideoClip(i);
        TInt videoClipSize = 0;
        
        // Get the number of video frames in the clip
        TInt videoFramesInClip = 0;
        if (vedClip->Info()->Class() == EVedVideoClipClassGenerated)
            {
            videoFramesInClip = vedClip->Info()->Generator()->VideoFrameCount(); 
            }
        else
            {
            videoFramesInClip = vedClip->Info()->VideoFrameCount(); 
            }
            
        if (videoFramesInClip <= 0)
            {
            // No video frames in clip, check next one
            continue;
            }
        
        if (vedClip->Info()->Class() == EVedVideoClipClassGenerated)
            {
            TInt startTransitionFrames = 0;
            TInt endTransitionFrames = 0;
            
            // Get the number of transition frames in the clip
            GetTransitionFrames(movie, i, startTransitionFrames, endTransitionFrames);
            
            // Calculate the size of the transition frames and add to video clip size     
            TInt transitionLength = (TInt) (1000000.0 / (TReal) movie->MaximumFramerate()) * (startTransitionFrames + endTransitionFrames);  
            videoClipSize += (transitionLength * movieBitrate) / 8 / 1000000;
            
            // Calculate the size of the non-transition frames and add to video clip size
            TInt nonTransitionFrames = videoFramesInClip - startTransitionFrames - endTransitionFrames;
            
            if (nonTransitionFrames > 0)
                {
                // The first one is intra and the rest are inter frames
                videoClipSize += GetGeneratedFrameSize(movie, ETrue);
                videoClipSize += GetGeneratedFrameSize(movie, EFalse) * (nonTransitionFrames - 1);
                }
            
            // Add clip size to video data size    
            videoDataSize += videoClipSize;
            numberOfVideoFrames += videoFramesInClip;
            
            // All done for this clip, go to next one
            continue;
            }
        
        // Calculate the frame rate of the source clip       
        TReal videoClipFramerate = 1.0;                  
        if (vedClip->Info()->Duration().Int64() != 0)
            {
            videoClipFramerate = (TReal) videoFramesInClip  * 1000000.0 /
                                 I64REAL(vedClip->Info()->Duration().Int64());
            }
            
        if (vedClip->Info()->Resolution() == movie->Resolution() &&
            vedClip->Info()->VideoType() == movie->VideoType() &&
            videoClipFramerate < (TReal) movie->MaximumFramerate() + 0.2)
            {
            // Clip is not transcoded => calculate the clip size using the frame sizes of the original clip
            TInt64 frameTime = 0;
            TInt64 videoClipCutInTime = vedClip->CutInTime().Int64();
            TInt64 videoClipCutOutTime = vedClip->CutOutTime().Int64();
        
            for (j = 0; j < videoFramesInClip; j++) 
                {
                frameTime = vedClip->Info()->VideoFrameStartTimeL(j).Int64();
                
                if (frameTime < videoClipCutInTime)
                    {
                    continue;   // Frame is before cut in position => check next one
                    }
                else if (frameTime >= videoClipCutOutTime)
                    {
                    break;      // Cut out position reached => no need to check frames anymore
                    }
                
                // Add frame size to video clip size
                videoClipSize += vedClip->Info()->VideoFrameSizeL(j);
                numberOfVideoFrames++;
                }
            
            // Check if color effect is in use    
            if (vedClip->ColorEffect() != EVedColorEffectNone)
                {
                // Color tone effects decreases the size of the frames slightly
                videoClipSize -= (TInt) ((TReal) videoClipSize * KSEBWReductionFactor);
                }
            }
        else    // Clip is transcoded
            {
            TInt estimatedBitrate = movieBitrate;
            
            // If we are transcoding a low quality clip to high quality (e.g. QCIF to VGA)
            if (movieBitrate >= KSEHighBitrateLimit &&
                movie->Resolution().iWidth > vedClip->Info()->Resolution().iWidth * 3  &&
                videoClipFramerate < (TReal) movie->MaximumFramerate() * 0.5 + 0.2)
                {
                // Halve the estimated bit rate if the source frame rate is 
                // roughly less than half of the maximum frame rate
                estimatedBitrate = estimatedBitrate >> 1;
                }
            
            // Clip frame rate can be decreased if necessary but not increased    
            TReal estimatedFramerate = Min(videoClipFramerate, (TReal) movie->MaximumFramerate());
            
            // Calculate the clip length
            TInt64 videoClipLength = vedClip->CutOutTime().Int64() - vedClip->CutInTime().Int64();
            
            // Calculate clip size
            videoClipSize = I64INT((videoClipLength * (TInt64) estimatedBitrate) / 8 / 1000000);
            
            // Calculate the number of frames included between cut in and cut out
            numberOfVideoFrames += (TInt) (I64REAL(videoClipLength * estimatedFramerate) / 1000000.0);
            }
        
        // Add clip size to video data size    
        videoDataSize += videoClipSize;
        }
            
    // Calculate video 3gp size
    video3gpSize = GetVideo3gpSizePerFrame(numberOfVideoFrames) * numberOfVideoFrames;
    
    // Check if there's any audio
    if (movie->AudioType() != EVedAudioTypeNoAudio)
        {
        // Calculate audio data size
        CAudSong* song = ((CVedMovieImp*)aMovie)->Song();
        audioDataSize = song->GetFrameSizeEstimateL(TTimeIntervalMicroSeconds(0), movie->Duration());
        
        if (audioDataSize > 0)
            {
            // Calculate audio 3gp size
            TReal numberOfAudioFrames = I64REAL(aMovie->Duration().Int64() / song->GetFrameDurationMicro());
            TReal numberOfAudioSamples = numberOfAudioFrames / (TReal) iProcessor->GetAudioFramesInSample();
            
            audio3gpSize = (TInt) (GetAudio3gpSizePerSample(numberOfAudioSamples) * numberOfAudioSamples); 
            }
        }
        
    // Calculate final estimated file size
    aFileSize = videoDataSize + audioDataSize + video3gpSize + audio3gpSize + KSEFixedSize; 
    return KErrNone;              
    }


// -----------------------------------------------------------------------------
// CSizeEstimate::GetMovieSizeEstimateForMMSL
// Calculates file size estimate for MMS use
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CSizeEstimate::GetMovieSizeEstimateForMMSL(const CVedMovie* aMovie, TInt aTargetSize, 
                                                  TTimeIntervalMicroSeconds aStartTime, 
                                                  TTimeIntervalMicroSeconds& aEndTime) 
    {
    TInt i = 0;
    TInt j = 0;
    
    TInt videoDataSize = 0;
    TInt audioDataSize = 0;
    TInt video3gpSize = 0;
    TInt audio3gpSize = 0;
    TInt numberOfVideoFrames = 0;
    TReal numberOfAudioSamples = 0.0;
    TInt64 currentTime = aStartTime.Int64(); 
    
    CVedVideoClip* vedClip = 0;
    CAudSong* song = ((CVedMovieImp*)aMovie)->Song();
    const CVedMovieImp* movie = reinterpret_cast<const CVedMovieImp *>(aMovie);
    
    TReal audioDurationToSamplesFactor = 1.0 / (I64REAL(song->GetFrameDurationMicro()) * (TReal) iProcessor->GetAudioFramesInSample());

    TInt movieBitrate = movie->VideoStandardBitrate();
    if (movieBitrate < KSELowBitrateLimit)
        {
        // Adjust low bit rates to slightly higher
        movieBitrate += KSELowBitrateIncrement;
        }
        
    TInt numberOfVideoClips = movie->VideoClipCount();
    
    // Find first clip included in the output 
    TInt startClipNumber = 0;
    for (i = 1; i < numberOfVideoClips; i++, startClipNumber++) 
        { 
        vedClip = movie->VideoClip(i); 
         
        if (aStartTime < vedClip->StartTime())
            {
            break;
            }
        }
    
    // Go through all the clips    
    for (i = startClipNumber; i < numberOfVideoClips; i++) 
        { 
        vedClip = movie->VideoClip(i);
        
        // Get the number of video frames in the clip
        TInt videoFramesInClip = 0;
        if (vedClip->Info()->Class() == EVedVideoClipClassGenerated)
            {
            videoFramesInClip = vedClip->Info()->Generator()->VideoFrameCount(); 
            }
        else
            {
            videoFramesInClip = vedClip->Info()->VideoFrameCount(); 
            }
            
        if (videoFramesInClip <= 0)
            {
            // No video frames in clip, check next clip
            continue;
            }
        
        // Calculate the frame rate of the source clip        
        TReal videoClipFramerate = 1.0;                  
        if (vedClip->Info()->Duration().Int64() != 0)
            {
            videoClipFramerate = (TReal) videoFramesInClip  * 1000000.0 /
                                 I64REAL(vedClip->Info()->Duration().Int64());
            }

        TInt videoFrameSize = 0;    
        TInt64 frameTime = 0;
        TInt64 frameDuration = 0;
        TInt64 videoClipCutInTime = vedClip->CutInTime().Int64();
        TInt64 videoClipCutOutTime = vedClip->CutOutTime().Int64();
        currentTime = vedClip->StartTime().Int64();
        
        TInt startTransitionFrames = 0;
        TInt endTransitionFrames = 0;
        
        // Check if clip needs to be transcoded
        TBool clipIsTranscoded = EFalse;
        if (vedClip->Info()->Class() == EVedVideoClipClassFile)
            {
            if (!(vedClip->Info()->Resolution() == movie->Resolution() &&
                  vedClip->Info()->VideoType() == movie->VideoType() &&
                  videoClipFramerate < (TReal) movie->MaximumFramerate() + 0.2))
                {
                clipIsTranscoded = ETrue;
                }
            }
        else
            {
            // Get the number of transition frames in the clip
            GetTransitionFrames(movie, i, startTransitionFrames, endTransitionFrames);
            }
            
        TInt estimatedBitrate = movieBitrate;
            
        // If we are transcoding a low quality clip to high quality (e.g. QCIF to VGA)
        if (movieBitrate >= KSEHighBitrateLimit &&
            movie->Resolution().iWidth > vedClip->Info()->Resolution().iWidth * 3  &&
            videoClipFramerate < (TReal) movie->MaximumFramerate() * 0.5 + 0.2)
            {
            // Halve the estimated bit rate if the source frame rate is 
            // roughly less than half of the maximum frame rate
            estimatedBitrate = estimatedBitrate >> 1;
            }
        
        if (i == startClipNumber)
            {
            // We need to adjust the cut in time for the first clip since the client
            // requested to get the size estimate starting from this position
            videoClipCutInTime = aStartTime.Int64() - vedClip->StartTime().Int64();
            
            // Initialize the current time to this position 
            currentTime = aStartTime.Int64();
            }
    
        // Estimate the size on every frame
        for (j = 0; j < videoFramesInClip; j++) 
            {
            // Get frame start time and duration
            if (vedClip->Info()->Class() == EVedVideoClipClassFile)
                {
                frameTime = vedClip->Info()->VideoFrameStartTimeL(j).Int64();
                frameDuration = vedClip->Info()->VideoFrameDurationL(j).Int64();
                }
            else
                {
                frameTime = vedClip->Info()->Generator()->VideoFrameStartTime(j).Int64();
                frameDuration = vedClip->Info()->Generator()->VideoFrameDuration(j).Int64();
                }
                
            if (frameTime < videoClipCutInTime)
                {
                continue;   // Frame is before cut in position => check next one
                }
            else if (frameTime >= videoClipCutOutTime)
                {
                break;      // Cut out position reached => no need to check frames anymore
                }
            
            // Check that duration is valid    
            if ((frameDuration <= 0) || (frameDuration > KSEMaxFrameDuration))
                {
                // Calculate frame duration from the frame rate
                frameDuration = (TInt64) (1000000.0 /  videoClipFramerate);
                }
                
            // Check for slow motion
            if (vedClip->Speed() != KVedNormalSpeed)
                {
                frameDuration = (frameDuration * KVedNormalSpeed) / vedClip->Speed();
                }
            
            if (clipIsTranscoded)
                {
                // Clip is transcoded so calculate frame size from estimated bit rate           
                videoFrameSize = I64INT((frameDuration * estimatedBitrate) / 8 / 1000000);
                }
            else
                {
                if (vedClip->Info()->Class() == EVedVideoClipClassGenerated)
                    {
                    // Check if the frame is a transition frame
                    if (j < (startTransitionFrames - 1) ||
                        j > (videoFramesInClip - endTransitionFrames))
                        {
                        // Frame is encoded so calculate frame size from movie bit rate 
                        videoFrameSize = I64INT((frameDuration * movieBitrate) / 8 / 1000000);
                        }
                    else
                        {
                        // Frame is intra if it's the first one in the clip
                        // or if it's right after or before transition
                        TBool isIntra = j == 0 || j == (startTransitionFrames - 1) || j == (videoFramesInClip - endTransitionFrames);
                        videoFrameSize = GetGeneratedFrameSize(movie, isIntra);
                        }  
                    }
                else
                    {
                    // Get frame size from the original clip
                    videoFrameSize = vedClip->Info()->VideoFrameSizeL(j);
                    
                    // Check if color effect is in use    
                    if (vedClip->ColorEffect() != EVedColorEffectNone)
                        {
                        // Color tone effects decreases the size of the frame slightly
                        videoFrameSize -= (TInt) ((TReal) videoFrameSize * KSEBWReductionFactor);
                        }
                    }
                }
            
            // Calculate video data size
            videoDataSize += videoFrameSize;
            
            // Calculate video 3gp size
            numberOfVideoFrames++;
            video3gpSize = GetVideo3gpSizePerFrame(numberOfVideoFrames) * numberOfVideoFrames;
            
            if (movie->AudioType() != EVedAudioTypeNoAudio)
                {
                // Calculate audio data size
                audioDataSize += song->GetFrameSizeEstimateL(currentTime, currentTime + frameDuration);
                
                if (audioDataSize > 0)
                    {
                    // Calculate audio 3gp size
                    numberOfAudioSamples += I64REAL(frameDuration) * audioDurationToSamplesFactor;
                    audio3gpSize = (TInt) (GetAudio3gpSizePerSample(numberOfAudioSamples) * numberOfAudioSamples); 
                    }
                }
                
            // Check if target size is reached        
            if (videoDataSize + audioDataSize + video3gpSize + audio3gpSize + KSEFixedSize >= aTargetSize)
                {
                aEndTime = currentTime;
                
                // Make sure we didn't overflow the end time
                if (aEndTime > movie->Duration())
                    {
                    aEndTime = movie->Duration();
                    }
                
                return KErrNone;
                }
            
            // Increase time to the next frame    
            currentTime += frameDuration;
            }
        }
        
    // Check if audio is longer than video
    TInt64 remainingAudioDuration = 0;

    if (numberOfVideoClips > 0)
        {
        remainingAudioDuration = movie->Duration().Int64() - 
            movie->VideoClip(numberOfVideoClips - 1)->EndTime().Int64();	
        
        // Adjust current time to the end of the last clip 
        currentTime = movie->VideoClip(numberOfVideoClips - 1)->EndTime().Int64();
        }
    else 
        {
        remainingAudioDuration = aMovie->Duration().Int64(); 
        }
        
    if (remainingAudioDuration > 0)
        {
        // Estimate how many black frames are inserted
        TInt frameDuration = KSEBlackFrameDuration;
        TInt blackVideoFrames = (TInt) (remainingAudioDuration / frameDuration) + 1;
        
        for (j = 0; j < blackVideoFrames; j++) 
            {           
            // Calculate video data size
            TBool isIntra = j == 0;
            videoDataSize += GetBlackFrameSize(movie, isIntra);  
            
            // Calculate video 3gp size
            numberOfVideoFrames++; 
            video3gpSize = GetVideo3gpSizePerFrame(numberOfVideoFrames) * numberOfVideoFrames;
            
            // Calculate audio data size
            audioDataSize += song->GetFrameSizeEstimateL(currentTime, currentTime + frameDuration);
            
            // Calculate audio 3gp size
            numberOfAudioSamples += (TReal) frameDuration * audioDurationToSamplesFactor;
            audio3gpSize = (TInt) (GetAudio3gpSizePerSample(numberOfAudioSamples) * numberOfAudioSamples);
            
            // Check if target size is reached        
            if (videoDataSize + audioDataSize + video3gpSize + audio3gpSize + KSEFixedSize >= aTargetSize)
                {              
                aEndTime = currentTime;
                
                // Make sure we didn't overflow the end time
                if (aEndTime > movie->Duration())
                    {
                    aEndTime = movie->Duration();
                    }
                
                return KErrNone;
                }
                
            currentTime += frameDuration;
            }
        }

    // Target size not reached till end of movie
    aEndTime = movie->Duration();

    return KErrNone; 
    } 

// -----------------------------------------------------------------------------
// CSizeEstimate::GetTransitionFrames
// Returns the number of start and end transition frames in given clip
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//    
void CSizeEstimate::GetTransitionFrames(const CVedMovieImp *aMovie, TInt aIndex,
                                        TInt& aStartTransitionFrames, TInt& aEndTransitionFrames)
    {
    aStartTransitionFrames = 0;
    aEndTransitionFrames = 0;
    
    CVedVideoClip* vedClip = aMovie->VideoClip(aIndex);
    
    // Get the number of video frames in the clip    
    TInt videoFramesInClip = 0;
    if (vedClip->Info()->Class() == EVedVideoClipClassGenerated)
        {
        videoFramesInClip = vedClip->Info()->Generator()->VideoFrameCount(); 
        }
    else
        {
        videoFramesInClip = vedClip->Info()->VideoFrameCount(); 
        }
    
    // Calculate the amount of start transition frames in the clip
    if (aIndex == 0)
        {
        // First clip in the movie so check movie start transition
        if (aMovie->StartTransitionEffect() != EVedStartTransitionEffectNone)
            {
            aStartTransitionFrames = KSEFadeTransitionFrames;   // Fade from
            }
        }
    else if (aMovie->MiddleTransitionEffect(aIndex - 1) != EVedMiddleTransitionEffectNone)
        {
        // Check if the previous clip has a middle transition
        if (aMovie->MiddleTransitionEffect(aIndex - 1) == EVedMiddleTransitionEffectDipToBlack ||
            aMovie->MiddleTransitionEffect(aIndex - 1) == EVedMiddleTransitionEffectDipToWhite)
            {
            aStartTransitionFrames = KSEFadeTransitionFrames;   // Dip
            }
        else
            {
            aStartTransitionFrames = KSEWipeTransitionFrames;   // Wipe / crossfade
            }
        }
        
    // Calculate the amount of end transition frames in the clip
    if (aIndex == aMovie->VideoClipCount() - 1)
        {
        // Last clip in the movie so check movie end transition
        if (aMovie->EndTransitionEffect() != EVedEndTransitionEffectNone)
            {
            aEndTransitionFrames = KSEFadeTransitionFrames;     // Fade to
            }
        }
    else if (aMovie->MiddleTransitionEffect(aIndex) != EVedMiddleTransitionEffectNone)
        {
        // Check if this clip has a middle transition
        if (aMovie->MiddleTransitionEffect(aIndex) == EVedMiddleTransitionEffectDipToBlack ||
            aMovie->MiddleTransitionEffect(aIndex) == EVedMiddleTransitionEffectDipToWhite)
            {
            aEndTransitionFrames = KSEFadeTransitionFrames;     // Dip
            }
        else
            {
            aEndTransitionFrames = KSEWipeTransitionFrames;     // Wipe / crossfade
            }
        }
    
    // Check that the number of transition frames does not overflow    
    if (aStartTransitionFrames > videoFramesInClip)
        {
        aStartTransitionFrames = videoFramesInClip;
        }
    if (aEndTransitionFrames > videoFramesInClip - aStartTransitionFrames)
        {
        aEndTransitionFrames = videoFramesInClip - aStartTransitionFrames;
        }
    }

// -----------------------------------------------------------------------------
// CSizeEstimate::GetGeneratedFrameSize
// Estimates the average generated frame size
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//     
TInt CSizeEstimate::GetGeneratedFrameSize(const CVedMovieImp *aMovie, TBool aIntra)
    {    
    TInt frameFactor = 0;
    
    // Estimate the frame factor based on codec type
    if (aMovie->VideoType() == EVedVideoTypeMPEG4SimpleProfile)
        {
        // MPEG-4
        frameFactor = aIntra ? KSEGeneratedIFrameFactorMPEG4 : KSEGeneratedPFrameFactorMPEG4;  
        }
    else if (aMovie->VideoType() == EVedVideoTypeAVCBaselineProfile)
        {
        // H.264
        frameFactor = aIntra ? KSEGeneratedIFrameFactorH264 : KSEGeneratedPFrameFactorH264; 
        }
    else
        {
        // H.263
        frameFactor = aIntra ? KSEGeneratedIFrameFactorH263 : KSEGeneratedPFrameFactorH263;
        }
    
    // Estimate frame size based on movie resolution and frame factor   
    return (aMovie->Resolution().iWidth * frameFactor) >> 3;    // Convert bits to bytes
    }

// -----------------------------------------------------------------------------
// CSizeEstimate::GetBlackFrameSize
// Estimates the average black frame size
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//    
TInt CSizeEstimate::GetBlackFrameSize(const CVedMovieImp *aMovie, TBool aIntra)
    {
    TInt frameFactor = aIntra ? KSEBlackIFrameFactor : KSEBlackPFrameFactor;
        
    return (aMovie->Resolution().iWidth * frameFactor) >> 3;    // Convert bits to bytes
    }

// -----------------------------------------------------------------------------
// CSizeEstimate::GetVideo3gpSizePerFrame
// Returns the 3gp size for video based on number of video frames
// (other items were commented in a header).
// -----------------------------------------------------------------------------
// 
TInt CSizeEstimate::GetVideo3gpSizePerFrame(TInt aNumberOfVideoFrames)
    {
    TInt video3gpSize = KSEVideo3gpSizePerFrame6;
    
    if (aNumberOfVideoFrames < KSEVideo3gpFramesLimit1)      video3gpSize = KSEVideo3gpSizePerFrame1; 
    else if (aNumberOfVideoFrames < KSEVideo3gpFramesLimit2) video3gpSize = KSEVideo3gpSizePerFrame2; 
    else if (aNumberOfVideoFrames < KSEVideo3gpFramesLimit3) video3gpSize = KSEVideo3gpSizePerFrame3; 
    else if (aNumberOfVideoFrames < KSEVideo3gpFramesLimit4) video3gpSize = KSEVideo3gpSizePerFrame4; 
    else if (aNumberOfVideoFrames < KSEVideo3gpFramesLimit5) video3gpSize = KSEVideo3gpSizePerFrame5; 
    
    return video3gpSize;
    }

// -----------------------------------------------------------------------------
// CSizeEstimate::GetAudio3gpSizePerSample
// Returns the 3gp size for audio based on number of audio samples
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//     
TReal CSizeEstimate::GetAudio3gpSizePerSample(TReal aNumberOfAudioSamples)
    {
    TReal audio3gpSize = KSEAudio3gpSizePerSample8;
     
    if (aNumberOfAudioSamples < KSEAudio3gpSamplesLimit1)      audio3gpSize = KSEAudio3gpSizePerSample1; 
    else if (aNumberOfAudioSamples < KSEAudio3gpSamplesLimit2) audio3gpSize = KSEAudio3gpSizePerSample2; 
    else if (aNumberOfAudioSamples < KSEAudio3gpSamplesLimit3) audio3gpSize = KSEAudio3gpSizePerSample3; 
    else if (aNumberOfAudioSamples < KSEAudio3gpSamplesLimit4) audio3gpSize = KSEAudio3gpSizePerSample4; 
    else if (aNumberOfAudioSamples < KSEAudio3gpSamplesLimit5) audio3gpSize = KSEAudio3gpSizePerSample5; 
    else if (aNumberOfAudioSamples < KSEAudio3gpSamplesLimit6) audio3gpSize = KSEAudio3gpSizePerSample6; 
    else if (aNumberOfAudioSamples < KSEAudio3gpSamplesLimit7) audio3gpSize = KSEAudio3gpSizePerSample7; 
    
    return audio3gpSize;
    }


//  End of File

