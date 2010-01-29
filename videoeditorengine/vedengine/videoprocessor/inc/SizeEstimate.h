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
* Header file for size estimate.
*
*/


#ifndef     __SIZEESTIMATE_H__
#define     __SIZEESTIMATE_H__

#include <e32base.h>


//  FORWARD DECLARATIONS
class CMovieProcessorImpl;
class CVedMovie;
class CVedMovieImp;
class CVedVideoClip;



// CONSTANTS

const TInt KSEFixedSize = 148;
const TReal KSEBWReductionFactor = 0.04;
const TInt KSELowBitrateLimit = 128000;
const TInt KSELowBitrateIncrement = 4000;
const TInt KSEHighBitrateLimit = 1024000;
const TInt KSEBlackFrameDuration = 1000000;
const TInt KSEMaxFrameDuration = 1001000;
const TInt KSEFadeTransitionFrames = 10;
const TInt KSEWipeTransitionFrames = 5;
const TInt KSEGeneratedIFrameFactorMPEG4 = 180;
const TInt KSEGeneratedPFrameFactorMPEG4 = 6;
const TInt KSEGeneratedIFrameFactorH264 = 400;
const TInt KSEGeneratedPFrameFactorH264 = 11;
const TInt KSEGeneratedIFrameFactorH263 = 212;
const TInt KSEGeneratedPFrameFactorH263 = 35;
const TInt KSEBlackIFrameFactor = 25;
const TInt KSEBlackPFrameFactor = 3;
const TInt KSEVideo3gpSizePerFrame1 = 40;
const TInt KSEVideo3gpSizePerFrame2 = 25;
const TInt KSEVideo3gpSizePerFrame3 = 15;
const TInt KSEVideo3gpSizePerFrame4 = 13;
const TInt KSEVideo3gpSizePerFrame5 = 12;
const TInt KSEVideo3gpSizePerFrame6 = 11;
const TInt KSEVideo3gpFramesLimit1 = 50;
const TInt KSEVideo3gpFramesLimit2 = 75;
const TInt KSEVideo3gpFramesLimit3 = 100;
const TInt KSEVideo3gpFramesLimit4 = 200;
const TInt KSEVideo3gpFramesLimit5 = 300;
const TReal KSEAudio3gpSizePerSample1 = 15.0;
const TReal KSEAudio3gpSizePerSample2 = 11.2;
const TReal KSEAudio3gpSizePerSample3 = 9.8;
const TReal KSEAudio3gpSizePerSample4 = 9.2;
const TReal KSEAudio3gpSizePerSample5 = 9.0;
const TReal KSEAudio3gpSizePerSample6 = 8.8;
const TReal KSEAudio3gpSizePerSample7 = 8.6;
const TReal KSEAudio3gpSizePerSample8 = 8.4;
const TReal KSEAudio3gpSamplesLimit1 = 200.0;
const TReal KSEAudio3gpSamplesLimit2 = 300.0;
const TReal KSEAudio3gpSamplesLimit3 = 400.0;
const TReal KSEAudio3gpSamplesLimit4 = 500.0;
const TReal KSEAudio3gpSamplesLimit5 = 800.0;
const TReal KSEAudio3gpSamplesLimit6 = 1600.0;
const TReal KSEAudio3gpSamplesLimit7 = 2000.0;



//  CLASS DEFINITIONS
class CSizeEstimate : public CBase
{
public:  // New functions
	
    /* Constructors. */
	static CSizeEstimate* NewL(CMovieProcessorImpl* aProcessor);
	static CSizeEstimate* NewLC(CMovieProcessorImpl* aProcessor);
	
	/* Destructor. */
	virtual ~CSizeEstimate();

    /**
     * Calculate movie size estimate
     *          
     * @param aMovie            Movie object 
     * @param aFileSize         Size estimate in bytes
     * @return Error code
     */
    TInt GetMovieSizeEstimateL(const CVedMovie* aMovie, TInt& aFileSize);  

    /**
     * Calculate movie size estimate for MMS
     *          
     * @param aMovie			Movie object 
     * @param aTargetSize		Maximum size allowed
     * @param aStartTime		Time of the first frame included in the MMS output
     * @param aEndTime			Time of the last frame included in the MMS output
     * @return Error code
     */
    TInt GetMovieSizeEstimateForMMSL(const CVedMovie* aMovie, TInt aTargetSize, 
		TTimeIntervalMicroSeconds aStartTime, TTimeIntervalMicroSeconds& aEndTime);

	
private: // Private methods

    /**
    * By default Symbian OS constructor is private.
	*/
    void ConstructL();

    /**
    * c++ default constructor
	*/
    CSizeEstimate(CMovieProcessorImpl* aProcessor);
    
    /**
    * Returns the number of start and end transition frames in given clip
    *
    * @param aMovie             Movie object
    * @param aIndex             Index of the clip in the movie
    * @param aStartTransitionFrames (out)   Number of start transition frames
    * @param aEndTransitionFrames (out      Number of end transition frames
    */
    void GetTransitionFrames(const CVedMovieImp *aMovie, TInt aIndex,
        TInt& aStartTransitionFrames, TInt& aEndTransitionFrames);
    
    /**
    * Estimates the average generated frame size
    *
    * @param aMovie             Movie object
    * @param aIntra             ETrue if the frame is intra
    * @return                   Generated frame size in bytes
    */  
    TInt GetGeneratedFrameSize(const CVedMovieImp *aMovie, TBool aIntra);
    
    /**
    * Estimates the average black frame size
    *
    * @param aMovie             Movie object
    * @param aIntra             ETrue if the frame is intra
    * @return                   Black frame size in bytes
    */
    TInt GetBlackFrameSize(const CVedMovieImp *aMovie, TBool aIntra);
    
    /**
     * Returns the 3gp size for video based on number of video frames
     *
     * @param aNumberOfVideoFrames  The amount of frames in the video
     * @return                      3gp size per frame for video
     */
    TInt GetVideo3gpSizePerFrame(TInt aNumberOfVideoFrames);
    
    /**
     * Returns the 3gp size for audio based on number of audio samples
     *
     * @param aNumberOfAudioSamples The amount of samples in the audio
     * @return                      3gp size per sample for audio
     */                    
    TReal GetAudio3gpSizePerSample(TReal aNumberOfAudioSamples);

private:  // Data
    
    // handle to the processor instance
    CMovieProcessorImpl* iProcessor;
		
};


#endif      //  __SIZEESTIMATE_H__

// End of File
