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
* Header file for movie processor implementation.
*
*/


#ifndef     __MOVIEPROCESSORIMPL_H__
#define     __MOVIEPROCESSORIMPL_H__

#ifndef __E32BASE_H__
#include <e32base.h>
#endif

#include "mp4demux.h"
#include "parser.h"
#include "videoprocessor.h"
#include "VedCommon.h"
#include "VedMovieImp.h"
#include "VedVideoClipInfoImp.h"
#include "VedVideoClipGenerator.h"
#include "VedRgb2YuvConverter.h"

//  FORWARD DECLARATIONS
class CStatusMonitor;
class CActiveQueue;
class CVideoProcessor;
class CDemultiplexer; 
class CComposer; 
class CVideoEncoder;
class CVedMovie;
class MVedMovieProcessingObserver;
class CDisplayChain;
class CTranscoder;
class CAudioProcessor;
class CSizeEstimate;
class CVedAVCEdit;

// audio operation
enum TAudioOperation
{
	EAudioRetain = 0,
	EAudioMute,
	EAudioReplace
};

enum TVideoOperation  // used in image insertion
{
    EVideoEncodeFrame = 0,
    EVideoWriteFrameToFile
};
		
// video frame parameters
struct TFrameParameters
{	
	TInt64 iTimeStamp;
	TUint8 iType;	
};

// H.263 TR parameters
struct TTrParameters
{
	TInt iTrPrevNew; 
	TInt iTrPrevOrig; 
};

// video clip parameters
struct TVideoClipParameters
{
	TInt64 iStartTime; 
	TInt64 iEndTime; 
}; 
   

//  CLASS DEFINITIONS
class CMovieProcessorImpl : public CActive, public MVedVideoClipGeneratorFrameObserver,
                            public MThumbnailObserver
{
public:

    // Error codes
    enum TErrorCode
    {
        EInvalidProcessorState = -1000,
        EUnsupportedFormat = -1001,
        EVideoTooLarge = -1002,                                
        EStreamCorrupted = -1003,        
        EInvalidStreamData = -1004,
        EInvalidInternalState = -1050
        // Also error codes <EInvalidInternalState may be returned, they can be
        // considered fatal internal errors.
    };

    // Input data format
    enum TDataFormat
    {    
        EData3GP,  // 3GP
        EDataAutoDetect // autodetect type
    };    


public:  // Functions from base classes

    /**
    * From CActive Active object running method
    */
    void RunL();

    /**
    * From CActive Active object cancelling method
    */    
    void DoCancel();

    /**
    * From CActive Active Error handling method
    */    
    TInt RunError(TInt aError);

    /** Frame insertion support 
	 * From CVedVideoClipGenerator, this function is callback indicating image is ready
	 * @param aError      <code>KErrNone</code>, if frame was completed successfully; 
	 *                    one of the system wide error codes, if frame generation failed
     * @param aFrame      pointer to frame, if it was completed successfully;
	 *                    <code>NULL</code>, if frame generation failed
	 */
	void NotifyVideoClipGeneratorFrameCompleted(CVedVideoClipGenerator& aGenerator, 
        TInt aError, CFbsBitmap* aFrame);    
    
    /**
    * From MThumbnailObserver
	* 
	* Called when thumbnail generation is ready
	*	
    * @param aError error code
    */ 
    void NotifyThumbnailReady(TInt aError);

public:  // New functions
	
    /** 
     * Constructors for instantiating new video processors.
     * Should reserve as little resources as possible at this point.
	 */
	static CMovieProcessorImpl* NewL();
	static CMovieProcessorImpl* NewLC();
	
	/** 
	 * Destructor can be called at any time (i.e., also in the middle of a processing operation)
	 * Should release all allocated resources, including releasing all allocated memory and 
	 * *deleting* all output files that are currently being processed but not yet completed.
	 */
	virtual ~CMovieProcessorImpl();
		
	/**
	 * Do all initializations necessary to start processing a movie, e.g. open files, allocate memory.
	 * If this method leaves, destructor should be called to free allocated resources. 
     * Starts processing after initializations are done.
     *
	 * @param aMovie  movie to process
	 * @param aFilename  output file name
	 * @param aFileHandle output file handle
     * @param aObserver observer object    
     *
	 */
	void StartMovieL(CVedMovieImp* aMovie, const TDesC& aFileName, RFile* aFileHandle,
	                 MVedMovieProcessingObserver *aObserver);			

    /** Create 3gp generated clips 
     * Create all the necessary 3gp files before inserting them into the actual movie
     * If this method leaves, destructor should be called to free allocated resources. 
     * Starts processing after initializations are done.
     *
     * @param aCreateMode mode of operation of the create image files function
     */
	TInt CreateImage3GPFilesL(TVideoOperation aCreateMode);

	/** Prepare for 3gp generated clips
	 * Create all the data structures necessary for 3gp files creation before calling create.
	 * If this method leaves, destructor should be called to free allocated resources. 
     * Starts processing after initializations are done.
     *
	 * @param aCreateMode        mode of operation of the create image files function
	 */
	TInt ProcessImageSetsL(TVideoOperation aCreateMode);

	/** Encode raw frames for 3gp generated clips
	 * This method is used to encode a raw frame for 3gp generated clips
	 * If this method leaves, destructor should be called to free allocated resources. 
     * VideEncoder should be created and initialized before using this function
     * 
	 */
	TInt EncodeImageFrameL();

	/** Process raw frames for 3gp generated clips
	 * Decide what to do in this function - whether to call encoder or compose output
	 * If this method leaves, destructor should be called to free allocated resources. 
     * Starts processing after initializations are done.
     *
	 */
	void DoImageSetProcessL();

	
	/**
	 * Do all initializations necessary to start generating a thumbnail, e.g. open files, 
	 * allocate memory. The video clip file should be opened with EFileShareReadersOnly 
	 * share mode. The thumbnail should be scaled to the specified resolution and 
	 * converted to the specified display mode. If this method leaves, destructor should be called to free 
	 * allocated resources.
	 * 
	 * Possible leave codes:
	 *	- <code>KErrNoMemory</code> if memory allocation fails
	 *	- <code>KErrNotFound</code> if there is no file with the specified name
	 *    in the specified directory (but the directory exists)
	 *	- <code>KErrPathNotFound</code> if the specified directory
	 *    does not exist
	 *	- <code>KErrUnknown</code> if the specified file is of unknown format
	 *  - <code>KErrNotSupported</code> if the specified combination of parameters
	 *                                  is not supported
	 *
	 * @param aFileName  name of the file to generate thumbnail from
	 * @param aFileHandle  handle of the file to generate thumbnail from
     * @param aIndex       Frame index for selecting the thumbnail frame
     *                     -1 means the best thumbnail is retrieved
 	 * @param aResolution  resolution of the desired thumbnail bitmap, or
	 *                     <code>NULL</code> if the thumbnail should be
	 *                     in the original resolution
	 * @param aDisplayMode desired display mode; or <code>ENone</code> if 
	 *                     any display mode is acceptable
	 * @param aEnhance	   apply image enhancement algorithms to improve
	 *                     thumbnail quality; note that this may considerably
	 *                     increase the processing time needed to prepare
	 *                     the thumbnail
	 */
	void StartThumbL(const TDesC& aFileName, RFile* aFileHandle, TInt aIndex, TSize aResolution, 
		TDisplayMode aDisplayMode, TBool aEnhance);		
	
	/**
	 * Starts thumbnail generation. Thumbnail generation is an asynchronous operation,
	 * and its completion is informed to the caller via Active object request completion;
	 * the iStatus member of the caller is passed as a parameter to this method.
	 *
	 * This method may leave if an error occurs in initiating the thumbnail generation.	
	 * If this method leaves, destructor should be called to free allocated resources.	
	 * 
	 * @param aStatus     Reference to caller's iStatus member variable
	 * @param aFactor     Pointer to a TVedTranscodeFactor structure, which is updated by this method
	 *
	 * @return  
	 *          
	 */
	void ProcessThumbL(TRequestStatus &aStatus, TVedTranscodeFactor* aFactor);
		
	 /** 
	 * Method for retrieving the completed thumbnail bitmap.
	 * 
	 * Video processor should not free the CFbsBitmap instance after it has passed it on 
	 * as a return value of this function 
	 *	 
	 */
	void FetchThumb(CFbsBitmap*& aThumb);
	
	/**
	 * Read clip header from the specified file and return its properties.	 
	 * This method should leave if clip is invalid, cannot be opened, etc.
	 *
	 * @param aFileName  name of the file to read
	 * @param aFileHandle handle of file to read
	 * @param aVideoFormat  for returning the video format
     * @param aVideoType Type of video data
	 * @param aResolution  for returning the resolution
	 * @param aAudioType Type of audio data
     * @param aDuration  for returning the duration
     * @param aVideoFrameCount Number of video frames in the clip	 
	 * @param aSamplingRate Audio sampling rate
	 * @param aChannelMode Audio channel mode
	 */
	void GetClipPropertiesL(const TDesC& aFileName, RFile* aFileHandle, TVedVideoFormat& aFormat,
		TVedVideoType& aVideoType, TSize& aResolution, TVedAudioType& aAudioType, 
		TTimeIntervalMicroSeconds& aDuration, TInt& aVideoFrameCount,TInt& aSamplingRate, 
		TVedAudioChannelMode& aChannelMode);	

    /**
	 * Read video frame information from the specified video clip file and fills array of info for 
     * all frames in video.The file should be opened with EFileShareReadersOnly share mode. Video processor 
	 * should not free the video frame info array after it has passed it on as a return value 
	 * of this function. Returned array should be allocated with User::AllocL() and should be 
	 * freed by the caller of this method with User::Free(). 
	 *
	 * Possible leave codes:
	 *	- <code>KErrNoMemory</code> if memory allocation fails
	 *	- <code>KErrNotFound</code> if there is no file with the specified name
	 *    in the specified directory (but the directory exists)
	 *	- <code>KErrPathNotFound</code> if the specified directory
	 *    does not exist
	 *	- <code>KErrUnknown</code> if the specified file is of unknown format
	 *
	 * @param aFileName             name of the file to read
	 * @param aFileHandle           handle of the file to read
	 * @param aVideoFrameInfoArray  Array for video frame parameters
	 */
    void GenerateVideoFrameInfoArrayL(const TDesC& aFileName, RFile* aFileHandle, TVedVideoFrameInfo*& aVideoFrameInfoArray);

    /**
     * Get movie size estimate
     *
     * @param aMovie Movie for which the estimate is done
     * @return Movie size in bytes
     */

    TInt GetMovieSizeEstimateL(const CVedMovie* aMovie);    

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

    /**
	 *  Cancels the processing of a movie	 
	 */
    void CancelProcessingL();

    /**
     * Enhances the visual quality of the frame
     *     
     */				
	void EnhanceThumbnailL(const CFbsBitmap* aInBitmap, CFbsBitmap* aTargetBitmap); 

    /**
     * Get number of frames in output movie
     *     
     * @return Number of output frames
     */				
	inline TInt GetOutputNumberOfFrames() const { return iOutputNumberOfFrames; }

	/**
	* Get the file name of the current clip
	*     
	* @return File name of the current clip
	*/				
	inline TFileName GetCurrentClipFileName() const { return iClipFileName; }

    /**
     * Get number of frames in current clip
     *     
     * @return Number of frames in clip
     */
    inline TInt GetClipNumberOfFrames() const { return iParser->GetNumberOfFramesInClip(); }	
	
	/**
     * Get the timescale of the current video clip
     *     
     * @return Timescale
     */
	inline TInt GetVideoClipTimeScale() const { return iParser->iStreamParameters.iVideoTimeScale; }

    /**
     * Get color effect for the current clip
     *     
     * @return Color effect
     */
	inline TVedColorEffect GetColorEffect() const { return iColorEffect; } 
	
	inline void GetColorTone(TInt& aU, TInt& aV) { aU=iColorToneU; aV=iColorToneV; }

    /**
     * Get number of audio frames in one 3gp sample
     *     
     * @return Number of frames in sample
     */
    inline TInt GetAudioFramesInSample() const { return iAudioFramesInSample; }

    /**
     * Finalize processing a clip
     *     
     */
    void FinalizeVideoClip();

    /**
     * Set frame type 
     *
     * @param aFrameIndex Index to the frame
     * @param aType P frame = 0, I frame = 1
     */    
    void SetFrameType(TInt aFrameIndex, TUint8 aType);
	
    /**
     * Get video timestamp in ms from timestamp in ticks
     *
     * @param aTimeStampInTicks Time in ticks
     * @param aCommonTimeScale ETrue for using the output time scale
     *                         EFalse for using the scale of current clip
     * @return Timestamp in milliseconds
     */        
    TInt64 GetVideoTimeInMsFromTicks(TInt64 aTimeStampInTicks, TBool aCommonTimeScale) const;    
    
    /**
     * Get video timestamp in ticks from timestamp in ms
     *
     * @param aTimeStampInMs Time in ms
     * @param aCommonTimeScale ETrue for using the output time scale
     *                         EFalse for using the scale of current clip
     * @return Timestamp in ticks
     */        
    TInt64 GetVideoTimeInTicksFromMs(TInt64 aTimeStampInMs, TBool aCommonTimeScale) const;
    
    /**
     * Get audio timestamp in ticks from timestamp in ms
	 *
	 * @param aTimeStampInMs Time in ms 		
	 * @return Timestamp in ticks
	 */ 	 
	TUint GetAudioTimeInTicksFromMs(TUint aTimeStampInMs) const;        
	
	/**
     * Get audio timestamp in ms from timestamp in ticks
	 *
	 * @param aTimeStampInTicks Time in ticks 		
	 * @return Timestamp in milliseconds
	 */ 	 
	TUint GetAudioTimeInMsFromTicks(TUint aTimeStampInTicks) const;

    /**
     * Get video frame index based on timestamp
     *
     * @param aTime Frame timestamp in microseconds
     * @return Frame index
     */    
    TInt GetVideoFrameIndex(TTimeIntervalMicroSeconds aTime) const;

    /**
     * Get frame index for the first frame in current clip
     *
     * @param aTime Timestamp in microseconds
     * @return Frame index
     */
    inline TInt GetStartFrameIndex() const { return iStartFrameIndex; }

    /**
     * Get frame type at specified index
     *
     * @param aIndex Frame index
     * @return Frame type
     */
    inline TInt8 GetVideoFrameType(TInt aIndex) const { return iParser->GetVideoFrameType(aIndex); };

    inline TInt8 VideoFrameType(TInt aIndex) const
        {
        if (iThumbnailInProgress || (iFrameParameters == 0))
            {
            return iParser->GetVideoFrameType(iParser->GetStartFrameIndex() + aIndex);
            }
            
        return (TInt8) iFrameParameters[aIndex].iType;
        }

     /**
     * Get frame timestamp at specified index, in ticks
     *
     * @param aIndex Frame index
     * @return timestamp in ticks
     */
    inline TInt64 VideoFrameTimeStamp(TInt aIndex)
        {
        if (iThumbnailInProgress || (iFrameParameters == 0))
            {
            TInt startTimeInTicks = 0;
            iParser->GetVideoFrameStartTime(iParser->GetStartFrameIndex() + aIndex, &startTimeInTicks);
            return startTimeInTicks;
            }
        
        return (TInt64) iFrameParameters[aIndex].iTimeStamp;
        }

    inline void SetTrPrevNew(TInt aTr) { iTr.iTrPrevNew = aTr; }

    inline void SetTrPrevOrig(TInt aTr) { iTr.iTrPrevOrig = aTr; }

    inline TInt GetTrPrevNew() const { return iTr.iTrPrevNew; }

    inline TInt GetTrPrevOrig() const { return iTr.iTrPrevOrig; }

    /**
     * Get pointer to demultiplexer object
     *     
     * @return demux pointer
     */
    inline CDemultiplexer* GetDemux() const { return iDemux; }
	
    /**
     * Get the start time for the current clip
     *     
     * @return Time in microseconds
     */
	inline TTimeIntervalMicroSeconds GetStartCutTime() const { return iStartCutTime; }

     /**
     * Get thumbnail processing status
     * 
     * @return ETrue if thumbnail is being processed, EFalse otherwise
     */
    inline TBool IsThumbnailInProgress() const { return iThumbnailInProgress; }

    /**
     * Get the end time for the current clip
     *     
     * @return Time in microseconds
     */
	inline TTimeIntervalMicroSeconds GetEndCutTime() const { return iEndCutTime; }	

	/**
     * Get current metadata size
     *     
     * @return Metadata size in bytes
     */
	TUint CurrentMetadataSize();
       
    /**
     * Write video frame to output 3gp file
     * @param aBuf Descriptor containing the frame
     * @param aTimeStampInTicks Time stamp of the frame in ticks
     * @param aDurationInTicks Frame duration in ticks
     * @param aKeyFrame ETrue if the frame is a keyframe     
     * @param aCommonTimeScale if ETrue, output movie timescale will
     *         be used, otherwise the current clip's timescale is used
     * @param aColorTransitionFlag ETrue if writing frames from latter
     *        clip in a color transition (wipe/crossfade)
     * @return Error code
     */
    TInt WriteVideoFrameToFile(TDesC8& aBuf, TInt64 aTimeStampInTicks, 
                               TInt aDurationInTicks, TBool aKeyFrame,
                               TBool aCommonTimeScale, TBool aColorTransitionFlag,
                               TBool aFromEncoder);

	/**
     * Save a video YUV frame to the tmp file
     * @param aBuf Descriptor containing the YUV frame
     * @param aDuration The frame duration
     * @param aTimeStamp The frame timestamp
     * @return Error code
     */
    TInt SaveVideoFrameToFile(TDesC8& aBuf, TInt aDuration, TInt64 aTimeStamp);

	/**
     * Retrieve a video YUV frame from the tmp file
     * @param aBuf Descriptor containing the YUV frame
     * @param aLength Length of the descriptor
     * @param aDuration The frame duration
     * @param aTimeStamp The frame timestamp
     * @return Error code
     */
    TInt GetVideoFrameFromFile(TDes8& aBuf, TInt aLength, TInt& aDuration, TInt64& aTimeStamp);

	/**
     * Get the next frame duration and timestamp
     * @param aDuration The frame duration
     * @param aTimeStamp The frame timestamp
     * @param aIndex Index of timestamp to get
     * @param aTimeStampOffset Offset for timestamp
     * @return Error code
     */
    void GetNextFrameDuration(TInt& aDuration, TInt64& aTimeStamp, TInt aIndex, TInt& aTimeStampOffset);

    /**
     * Append the next frame duration and timestamp
     * @param aDuration The frame duration
     * @param aTimeStamp The frame timestamp
     * @return Error code
     */
    void AppendNextFrameDuration(TInt aDuration, TInt64 aTimeStamp);

    /**
     * Get the number of transition duration in the list
     * @return Number of transition
     */
    TInt NumberOfTransition() { return iCurClipDurationList.Count(); }

    /**
     * Get the number of transition at the start of next clip
     * @return Number of transition number
     */
    TInt NextClipStartTransitionNumber();

    /**
     * Get the transition duration of the current clip
     * @return Transition duration
     */
    TInt TransitionDuration();

    /**
     * Release all internal data hold for transition effect
     */
    void CloseTransitionInfoL();

	/**
     * Update progress based on time processed so far
     *          
     */
	void IncProgressBar();
    
     /**
     * Get the number of the audio clip being processed
     * 
     * @return Clip number
     */
    inline TInt GetAudioClipNumber() const  { return iAudioClipNumber; }	
	
	/**
	* Get the output type of audio in the movie
	* 
	* @return Audio type
	*/
	TVedAudioType GetOutputAudioType();		
     
    /**
     * Get the number of the video clip being processed
     *          
     * @return Clip number
     */
	inline TInt GetVideoClipNumber() const { return iVideoClipNumber; }    

    /**
     * Get the total number of the video clips in movie
     *          
     * @return Number of clips
     */
    inline TInt GetNumberOfVideoClips() const { return iNumberOfVideoClips; }
        
    /**
     * Get the starting transition effect for the current clip
     *          
     * @return Transition effect
     */
	inline TVedStartTransitionEffect GetStartTransitionEffect() const { return iStartTransitionEffect; }

    /**
     * Get the middle transition effect for the current clip
     *          
     * @return Transition effect
     */
	inline TVedMiddleTransitionEffect GetMiddleTransitionEffect() const { return iMiddleTransitionEffect; }

    /**
     * Get the middle transition effect for the previous clip
     *          
     * @return Transition effect
     */
	inline TVedMiddleTransitionEffect GetPreviousMiddleTransitionEffect() const { return iPreviousMiddleTransitionEffect; }

    /**
     * Get the ending transition effect for the current clip
     *          
     * @return Transition effect
     */
	inline TVedEndTransitionEffect GetEndTransitionEffect() const { return iEndTransitionEffect; }

    /**
     * Get the slow motion speed of the current clip
     *          
     * @return Speed
     */
    inline TInt GetSlowMotionSpeed() const { return iVideoClip->Speed(); }

    /**
     * Get the duration of the current video clip
     *          
     * @return duration
     */
    inline TInt64 GetVideoClipDuration() { return iVideoClipDuration; }

	/**
     * Get the resolution of the current video clip
     *          
     * @return resolution
     */
	TSize GetVideoClipResolution();
	
	/**
     * Get the resolution of the output movie
     *          
     * @return resolution
     */
	TSize GetMovieResolution();

    /**
     * Get sync interval in picture setting from movie
     *
     * @return sync interval (H.263 GOB header frequency)
     */
    TInt GetSyncIntervalInPicture();
    
    /**
     * Get random access rate setting from movie
     *
     * @return random access rate in pictures per second
     */
    TReal GetRandomAccessRate();

	/**
     * Get the format of the output video
     *          
     * @return format
     */
	TVedVideoType GetOutputVideoType();

    /**
     * Get the format of the current video clip
     *          
     * @return format
     */
	TVedVideoType GetCurrentClipVideoType();

	/**	
     * Get the transcoding information of the video clip being processed
     *          
	 * @param aNum         video clip number in the movie
	 *
     * @return pointer to TTrascodeFactor object
     */
	TVedTranscodeFactor GetVideoClipTranscodeFactor(TInt aNum);		

	/** 
     * Get the flag that indicates if mode translation is required for the MPEG-4 video clip
     *          
     * @return duration
     */
	inline TBool GetModeTranslationMpeg4() { return iModeTranslationRequired; }

	/** 
     * Set the flag to indicate that the clip was transcoded
     *   
     * @param aModeChanged         ETrue if mode is changed
     */
	inline void SetClipModeChanged(TBool aModeChanged) { iModeChanged = aModeChanged; }

    /**
     * Stops processing
     *
     * @return Error code
     */
	TInt Stop();    

    /**
     * Stops processing and closes all submodules except status monitor & video encoder
     *
     * @return Error code
     */
    TInt Close();

    /**
     * Gets the average frame rate of current video clip
     *
     * @return Frame rate
     */
    TReal GetVideoClipFrameRate();
    
    /**
     * Gets the target movie frame rate
     *
     * @return Frame rate
     */
    TReal GetMovieFrameRate();
    
    /**
     * Get the target video bitrate
     *
     * @return bitrate
     */
    TInt GetMovieVideoBitrate();
    
    TInt GetMovieStandardVideoBitrate();
    
    /**
     * Sets the maximum size for the movie
     * 
     * @param aLimit Maximum size in bytes
     */
    void SetMovieSizeLimit(TInt aLimit);
    
     /**
      * Get the ColorTone value in Rgb for the clip and convert to YUV
	  *
	  * @param aColorEffect 			 TVedColorEffect type (B&W or ColorTone)			
	  * @param aColorToneRgb			 Rgb value for the colortone
	  */													 
	void ConvertColorToneRGBToYUV(TVedColorEffect aColorEffect,TRgb aColorToneRgb);
	
     /**
      * Adjust the U,V color tone value for H.263 video format
	  *
	  * @param aValue The color to adjustTVedColorEffect type (B&W or ColorTone)			
	  */													 
    void AdjustH263UV(TInt& aValue);
        
    /**
     * Process audio frames
	 *
	 */													 
    void ProcessAudioL();
    
    /**    	
	* Called when audio clip has been processed
	*
    * @param aError error code
    */         
    void AudioProcessingComplete(TInt aError);
    
    /**
     * Returns the MIME-type for the video in the movie
     * 
     * @return Video codec MIME-type
     */
    TPtrC8& GetOutputVideoMimeType();
        
    /**
     * Returns the number of saved frames from previous clip
     * 
     * @return number of frames 
     */
    inline TInt GetNumberOfSavedFrames() const { return iCurClipTimeStampList.Count(); }            
    
     /**
     * Returns the length (in bytes) of decoder specific info
     * 
     * @return Decoder specific info size
     */
    inline TInt GetDecoderSpecificInfoSize() const { return iParser->GetDecoderSpecificInfoSize(); }
    
     /**
     * Suspends demux. New output can arrive from transcoder, but no new
     * input is fed there.
     *
     * @return TInt error code
     */
    TInt SuspendProcessing();
    
    /**
     * Resumes processing, restoring the state. Input to transcoder
     * goes to the position before the suspend or the last I-frame before that.
     *
     * @param aStartIndex Index of the first frame to be decoded
     * @param aSuspendTime Frame number of last written frame
     *
     * @return TInt error code
     */
    TInt ResumeProcessing(TInt& aStartFrameIndex, TInt aFrameNumber);    
    
    /**
     * Check if video transcoder is needed any longer. If not, suspend is not necessary.
     *
     * @return TBool ETrue if needed, EFalse if not
     */
    TBool NeedTranscoderAnyMore();
								
private: // Constants
	
	// state	
    enum TProcessorState
        {
            EStateIdle = 0,		
            EStateOpened,   // clip is open at the decoder (?)
            EStatePreparing,
            EStateReadyToProcess,
            EStateProcessing        
        };
	
	// Audio types
	enum TAudioType
	{
		EAudioNone = 0,
		EAudioAMR,
		EAudioAAC
	};
	
	// Video types
	enum TVideoType
	{
		EVideoNone = 0,
		EVideoH263Profile0Level10,
		EVideoH263Profile0Level45,
		EVideoMPEG4,
		EVideoAVCProfileBaseline
	};
	
	// Multiplex types
	enum TMuxType
	{
        EMuxNone = 0,
		EMux3GP		
	};     
	
private: // Private methods

    /**
    * By default Symbian OS constructor is private.
	*/
    void ConstructL();

    /**
    * c++ default constructor
	*/
    CMovieProcessorImpl();    

    /**
     * Set the number of output frames
     *          
     * @param aOutputNumberOfFrames No. of output frames     
     */
    inline void SetOutputNumberOfFrames(TInt aOutputNumberOfFrames) {
        iOutputNumberOfFrames = aOutputNumberOfFrames; }
    
    /**
     * Opens a 3gp clip for processing
     *
     * @param aFileName Clip filename
     * @param aFileHandle Clip file handle
     * @param aDataFormat Clip file format
     *
     * @return Error code
     */
	TInt OpenStream(TFileName aFileName, RFile* aFileHandle, TDataFormat aDataFormat);

    /**
     * Closes the processed clip from parser
     *
     * @return Error code
     */
	TInt CloseStream();

    /**
     * Prepares the processor for processing a movie
     *
     * @return Error code
     */
	TInt Prepare();
    
    /**
     * Deletes objects used in processing
     *     
     */
    void DoCloseVideoL();

    /**
     * Finalizes movie processing, creates the output 3gp file
     * 
     */
	void FinalizeVideoSequenceL();

    /**
     * Starts processing the movie
     *          
     */
    void DoStartProcessing();

    /**
     * Sets suitable default values for parameters
     *          
     */
    void SetHeaderDefaults();

	/**
     * Sets output audio & video formats
     *          
     */
	void SetOutputMediaTypesL();

	/**
     * Gets bitstream modes of input clips
     *          
     */
	void GetTranscodeFactorsL();

	/**
     * Sets video transcoding parameters
     *          
     */
	void SetupTranscodingL();

    /**
     * Resets modules & variables used in processing
     *          
     */
    void ResetL();

    /**
     * Processes the movie when no video clips are present
     *          
     */
    void ProcessAudioOnly();

    /**
     * Parses the clip header
     *
     * @param aStreamParams Destination structure for stream parameters
     * @param Filename of the clip
     * @param FileHandle of the clip
     *          
     */
	void ParseHeaderOnlyL(CParser::TStreamParameters& aStreamParams, TFileName& aFileName, RFile* aFileHandle); 

    /**
     * Copies stream parameters from source to destination structure
     *
     * @param aDestParameters Destination structure
     * @param aSrcParameters Source structure     
     *          
     */
	void UpdateStreamParameters(CParser::TStreamParameters& aDestParameters, 
		CParser::TStreamParameters& aSrcParameters);    
	
    /**
     * Parse the clip header & update internal variables accordingly
     *     
     */
	void ParseHeaderL();	
    
    /**
     * Initializes the video queue and decoder
     *
     * @param aQueueBlocks Number of video queue blocks
     * @param aQueueBlockSize Size of one video queue block
     */
	void InitVideoL(TUint aQueueBlocks, TUint aQueueBlockSize);

	/**
     * Initializes the demultiplexer
     *     
     */
	void InitDemuxL();

    /**
     * Initializes the processor for the next clip to be processed,
     * called for each clip    
     */
	void InitializeClipL();

    /**
     * Initializes structures for movie processing, called once per movie
     *
     * @param iAudioDataArray Array containing parameters for audio clips     
     *
     */
    void InitializeClipStructuresL();

    /**
     * Initializes the processor for the generated clip to be processed.
     * Called for each Image clip    
     */
	void InitializeGeneratedClipL();

	/**
     * Initializes the processor for creating the temporary 3gp file (clip).
     * Called for each Image clip 
     */
	void TemporaryInitializeGeneratedClipL();

    /**
     * Frees memory allocated for movie processing
     *
     */   
    void DeleteClipStructures();

    /**
     * Fills an array containing video frame parameters
     *
     */   
    void FillVideoFrameInfoArrayL(TInt& aVideoFrameCount, TVedVideoFrameInfo*& aVideoFrameInfoArray);

    /**
     * Fills internal frame parameter structure
     *
     */
    void FillFrameParametersL(TInt aCurrentFrameIndex);    

    /**
     * Writes the remaining audio frames to file
     *          
     * @return Error code
     */
    TInt FinalizeAudioWrite();	

	/**
	* Decide the type of Audio in terms of the common types
	* 
	* @return Audio type
	*/
	TVedAudioType DecideAudioType(TAudioType aAudioType);	
	
	/**
    * Write an audio frame to internal buffer (AMR)
    * or to output file (AAC)
	*
	* @param aBuf Source buffer
	* @param aDuration duration of the current frame
	*
	 @return Error code
	*/		
	TInt WriteAllAudioFrames(TDesC8& aBuf, TInt aDuration);

	/**
	* Add a number of AMR frames to internal buffer
	*
	* @param aBuf Source buffer
	* @param aNumFrames Number of frames in buffer
	*
	* @return Error code
	*/
	TInt BufferAMRFrames(const TDesC8& aBuf, TInt aNumFrames, TInt aDuration);	

	/**
	* Purge AMR frames from internal buffer to output file
	*
	*
	* @return Error code
	*/
	TInt WriteAMRSamplesToFile();    

    /**
     * Get the target audio bitrate
     *
     * @return bitrate
     */
    TInt GetMovieAudioBitrate();
    
    /**
     * Writes a buffered frame to output file
     *
     * @param aDuration Duration of the frame being written from buffer,
     *        expressed in timescale of the output movie
     * @param aColorTransitionFlag ETrue if writing frames from latter
     *        clip in a color transition (wipe/crossfade)     
     * @return Error code
     */
    TInt WriteVideoFrameFromBuffer(TReal aDuration, TBool aColorTransitionFlag);

    /**
     * Writes the last buffered frame to file
     *
     * @return Error code
     */
    TInt FinalizeVideoWrite();
    
    /**
     * Reports an error in thumbnail generator
     * to the caller
     *
     * @return ETrue if error was reported, EFalse otherwise
     */
    TBool HandleThumbnailError(TInt aError);
    
    /**
     * Get output AVC level
     *
     * @return Level
     */
    TInt GetOutputAVCLevel();

private:  // Data

	// True if processing has been cancelled
	TBool iProcessingCancelled;

	// True if video encoder initialisation is ongoing
	TBool iEncoderInitPending;

	// timestamp for encoding ending black frames
	TTimeIntervalMicroSeconds iTimeStamp;

	// Duration by which audio is longer than video
	TInt64 iLeftOverDuration;

    // Video frame parameters
	TFrameParameters* iFrameParameters;
	
	// Size of the video frame parameters array
	TInt iFrameParametersSize;

    // number of frames in output movie
	TInt iOutputNumberOfFrames;	

    // TR parameters
	TTrParameters iTr;

    // Thumbnail resolution
	TSize iOutputThumbResolution;    

	// Thumbnail index
	TInt iThumbIndex; 

	// Thumbnail display mode
	TDisplayMode iThumbDisplayMode; 

	// Thumbnail enhance
	TBool iThumbEnhance; 

    // stream duration: max. of audio,video
	TInt64 iVideoClipDuration;

	// output audio buffer	
    // stores output raw amr data for processed clip
	HBufC8 *iOutAudioBuffer;
    
    // output video buffer for one frame
    HBufC8 *iOutVideoBuffer;

    // Is there a frame in buffer ?
    TBool iFrameBuffered;

    // Timestamp of buffered frame
    TInt64 iBufferedTimeStamp;

    // key frame info of buffered frame
    TBool iBufferedKeyFrame;
    
    TBool iBufferedFromEncoder;

    TBool iFirstFrameBuffered;

    // number of audio frames in the buffer currently
    TInt iAudioFramesInBuffer;
	
    // Composer object 
	CComposer *iComposer;

    // Filename of the current clip
    TFileName iClipFileName;
    
    // File handle of the current clip
    RFile* iClipFileHandle;
    
    // Output movie filename
	TFileName iOutputMovieFileName; 
	
	// Output movie filehandle
	RFile* iOutputFileHandle;

    // status monitor object
    CStatusMonitor* iMonitor;
    // Movie processing observer used
    MVedMovieProcessingObserver *iObserver; 
    // Are we processing a thumbnail ?
	TBool iThumbnailInProgress; 
		
    // number of frames processes so far
    TUint iFramesProcessed;
    // number of current video clip
	TInt iVideoClipNumber; 
    // total number of video frames
	TInt iNumberOfVideoClips; 
    // index of the first frame in clip
    TInt iStartFrameIndex;	
    // video timescale for output movie
	TUint iOutputVideoTimeScale;
    
    // progress percentage
    TInt iProgress;
    // is the current disk full ?
    TBool iDiskFull;

    // starting cut time for the current video clip
	TTimeIntervalMicroSeconds iStartCutTime; 
    // ending cut time for the current video clip
	TTimeIntervalMicroSeconds iEndCutTime;     
	
    // Current video clip
	CVedVideoClip* iVideoClip; 
    // Movie
	CVedMovieImp* iMovie; 

    // buffer pointers for thumbnail generation
    TUint8* iYuvBuf;
    TUint8* iRgbBuf;

    // Transition effect parameters
	TVedStartTransitionEffect iStartTransitionEffect; 
	TVedMiddleTransitionEffect iMiddleTransitionEffect; 
	TVedMiddleTransitionEffect iPreviousMiddleTransitionEffect; 
	TVedEndTransitionEffect iEndTransitionEffect; 

	// Indicates if an image is being encoded
	TInt iImageEncodedFlag;  

    // finished creating the image 3GP file
	TInt iImageEncodeProcFinished;

	// for storing the converted YUV image in generation case
	TUint8 *iYuvImageBuf; 

	// image size for generated frames
	TInt iImageSize;

	// Descriptor holding the image to be encoded
	TPtr8 iReadImageDes;

	// Image composer
	CComposer* iImageComposer;

	// images processed in generation case
	TInt iTotalImagesProcessed;

	// Time scale for generated frames
	TInt iImageVideoTimeScale;

    // Generated clip has been created
    TInt iImageClipCreated;

	// Are we starting to process generated images
	TInt iFirstTimeProcessing;

	// The name of the current clip is constant for image clips and 0 if buffer supported
	TFileName iCurrentMovieName;

	// number of images from the generator
	TInt iNumOfImages;

	// is getFrame() from generator in progress
	TInt iGetFrameInProgress;
	
	// Are output timescales set ?
	TInt iOutputVideoTimeSet;
	TInt iOutputAudioTimeSet;

	// Are all clips generated ?
	TInt iAllGeneratedClips;

	// Is generated clip image being encoded ?
	TInt iEncodeInProgress;	

	// in ms, audio and video durations added up for progress bar indication
	TInt64 iTotalMovieDuration;  

	// Is the first clip generated
    TBool iFirstClipIsGen;

    TAudioType iOutputAudioType;

	TVideoType iOutputVideoType;

	TInt iMP4SpecificSize; 

	// Is it the first frame of a clip
    TBool iFirstFrameOfClip;

    // for setting the first frame flag
    // when doing wipe/crossfade transitions
    TBool iFirstFrameFlagSet;

	/*Indicates whether mode translation in between MPEG4 modes is required(NOTE in between MPEG4 modes only)*/
	TBool iModeTranslationRequired;

	/*This is set by the decoder indicating he changed either the format or mode so Mp4Specific size may not be right  */
	TBool iModeChanged;	//This for the composer to know that mode was changed for Mp4Specific size

    // Speed for the current clip, 1000 = normal speed
	TUint iSpeed;
    // Color effect for the current clip
	TVedColorEffect iColorEffect;    

    // total number of audio clips
	TInt iNumberOfAudioClips; 

    // Current audio clip number
    TInt iAudioClipNumber; 

    // Current replacement clip number
    TInt iAudioReplaceClipNumber;

    TInt iAudioFrameNumber;
    TInt iVideoFrameNumber;
    TInt iVideoIntraFrameNumber;

    // Current audio time
	TInt64 iCurrentAudioTimeInMs;

	// total time in ms of audio written, 
	// for the whole movie
	TInt64 iTotalAudioTimeWrittenMs;

    // Current video time
	TReal iCurrentVideoTimeInTicks; 

    // duration processed for generated clips
    TInt64 iGeneratedProcessed;

	// Duration of current audio clip written, 
	// reseted after a clip has been processed
	TInt64 iAudioClipWritten;

    // Duration of current video clip written, 
	// reseted after a clip has been processed
    TInt64 iVideoClipWritten;

    // video clip parameters
	TVideoClipParameters *iVideoClipParameters; 
    TInt iNumberOfAudioClipsCreated;
    // number of audio frames per sample
    TInt iAudioFramesInSample; 
    // output audio timescale
    TInt iOutputAudioTimeScale;

    // time stamp for current video clip
    TInt64 iInitialClipStartTimeStamp;
    // ETrue if movie processing is starting
    TBool iStartingProcessing;    

	// Used by AAC to handle writing decoder specific info, 
	// if it does not exist, to default value */	
	TBool iFirstClipHasNoDecInfo;
    
    // the demultiplexer    
    CDemultiplexer *iDemux; 
     // the video decoder
    CVideoProcessor *iVideoProcessor;
    // the video decoder input queue
    CActiveQueue *iVideoQueue; 
     // file format parser
    CParser *iParser;   
    // video encoder
    CVideoEncoder *iVideoEncoder;  	
    
    // audio processor
    CAudioProcessor *iAudioProcessor;
    
    // audio processing done ?
    TBool iAudioProcessingCompleted;

    // frame enhancement
    CDisplayChain *iEnhancer;  

    // Video encoder input buffer
    TUint8 *iEncoderBuffer;
    // ETrue if video encoding is pending
    TBool iEncodePending;
    
    // does the stream have video?
    TBool iHaveVideo; 
    // video stream type
	TVideoType iVideoType; 
    // does the stream have audio?
	TBool iHaveAudio; 
	
	// : is this needed ??
    // audio stream type
	TAudioType iAudioType; 	    
	
    // is the stream multiplexed?
	TBool iIsMuxed; 
    // multiplex type					
	TMuxType iMuxType; 
		
    // current processor state
	TProcessorState iState; 
     // current stream data format	
	TDataFormat iDataFormat;

	// stream start reading buffer
	TUint8 *iReadBuf; 
    // buffer length
	TUint iBufLength; 
    // amount of data in buffer
	TUint iBufData; 
    // reading descriptor	
	TPtr8 iReadDes; 
			
    // number of demux channels used
	TUint iNumDemuxChannels; 
    // demux parameters read
	CMP4Demux::TStreamParameters iMP4Parameters; 
	CMP4Demux::TOutputChannel iMP4Channels[2];
		
    // H.263 stream parameters
	CVideoProcessor::TStreamParameters iVideoParameters; 

    // stream length in milliseconds
	TUint iStreamLength; 
    // stream size in bytes
	TUint iStreamSize; 
    // stream average bitrate
	TUint iStreamBitrate; 
    // is it possible to seek in the file	
	TBool iCanSeek; 

    // ETrue when all video of the movie has been processed
    TBool iAllVideoProcessed;
    
	// tmp file used for Transition Effect
	TFileName iCurClipFileName;
    TFileName iNextClipFileName;

	RFile iCurClip;
    RFile iNextClip;
    RFs   iFs;
    TBool iFsConnected;	

    RArray<TInt>   iCurClipDurationList;
    RArray<TInt>   iNextClipDurationList;
    RArray<TInt64> iCurClipTimeStampList;
    RArray<TInt64> iNextClipTimeStampList;

    TInt           iCurClipIndex;
    TUint          iPreviousTimeScale;
    TInt64         iOffsetTimeStamp;

	// This flag is set to true if first clip uses encoder such as cut 
	// as Resynch bit needs to be reset
	TBool iFirstClipUsesEncoder;	// For changing Vos in case of Mpeg4 transcoding
	TBool iMpeg4ModeTranscoded;

	// Is true if first clip is cut
	TBool iFirstClipIsCut;
	
	TInt iCurrentVideoSize;
	TInt iCurrentAudioSize;
	TInt iMovieSizeLimit;
	TBool iMovieSizeLimitExceeded;

    CActiveSchedulerWait *iWaitScheduler;
    TBool iWaitSchedulerStarted;

    TBool iAudioProcessingCancelled;
    
    TRequestStatus *iThumbnailRequestStatus;
    CFbsBitmap* iOutBitmap;
    
    // Duration of all samples in buffered AMR
    TInt iTotalDurationInSample;
    
    // ColorTone value as RGB
    TRgb iColorToneRgb;
    
	// ColorTone values as U,V
	TInt iColorToneU;
	TInt iColorToneV;
	
	// for scaling the timestamp list
	TBool iTimeStampListScaled;
	
	// true if we are encoding black ending frames
	TBool iEncodingBlackFrames;
	
	// For calculating movie size estimates
	CSizeEstimate *iSizeEstimate;
		    
    // Slow motion is not applied for ending black frames or the last frame of a clip
    TBool iApplySlowMotion;  

    // True if writing the first color transition frame    
    TBool iWriting1stColorTransitionFrame;
    
    // Timestamp of the first color transition frame    
    TInt64 i1stColorTransitionFrameTS;

    // AVC editing instance for movie    
    CVedAVCEdit* iAvcEdit;
    
    // AVC editing instance for a generated clip
    CVedAVCEdit* iImageAvcEdit;
    
    // Bitmap-to-YUV converter
    CVSFbsBitmapYUV420Converter* iImageYuvConverter;  

    friend class CMP4Parser;
    friend class CAudioProcessor;        
		
};


#endif      //  __MOVIEPROCESSORIMPL_H__

// End of File
