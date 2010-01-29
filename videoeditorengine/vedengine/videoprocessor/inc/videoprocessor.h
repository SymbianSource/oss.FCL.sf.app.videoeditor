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
* Header file for video processor.
*
*/


#ifndef     __VIDEOPROCESSOR_H__
#define     __VIDEOPROCESSOR_H__

//  INCLUDES
#ifndef _H263DAPI_H_
#include "h263dapi.h"
#endif

#include "videodecoder.h"
#include "vedVideoclip.h"


//  FORWARD DECLARATIONS
class CActiveQueue;
class CStatusMonitor;
class CMovieProcessorImpl;
class CVideoEncoder;
class CVedH263Dec;
class CMPEG4Timer;
class CVedAVCEdit;

#include "CTRTranscoderObserver.h"
#include "CTRTranscoder.h"
#include "CTRCommon.h"
#include <CCMRMediaSink.h>
#include "CTRVideoPictureSink.h"

enum TTransitionColor
{
	EColorNone = 0,
	EColorBlack,
	EColorWhite,
	EColorTransition
};

enum TTransitionPosition
{
    EPositionNone = 0,
    EPositionStartOfClip,
    EPositionEndOfClip
};


// Threshold for using postfilter. Assumption is that CIF is encoded with high bitrate => no need for postfilter
const TInt KThrWidthForPostFilter = 176;

//  CLASS DEFINITIONS


// Observer class for thumbnail generation
class MThumbnailObserver
{
public:
    void NotifyThumbnailReady(TInt aError);
};

class MTimerObserver
{
public:
    virtual void MtoTimerElapsed(TInt aError) = 0;
    
};


// Timer class to use when waiting for encoding to complete
class CCallbackTimer : public CActive
{
 
public:

    /**
    * Two-phased constructor.
    */
    static CCallbackTimer* NewL(MTimerObserver& aObserver);
    
    /**
    * Destructor.
    */
    ~CCallbackTimer();
    
public:
    
    /**
    * Set timer
    *
    * @param aDuration Duration
    */
    void SetTimer(TTimeIntervalMicroSeconds32 aDuration);
    
     
    /**
    * Query whether timer is active
    *
    * @return TRUE if timer is pending
    */
    inline TBool IsPending() { return iTimerRequestPending; }
    
    /**
    * Cancel timer    
    */
    void CancelTimer();
    
private:

    /**
    * C++ default constructor.
    */
    CCallbackTimer(MTimerObserver& aObserver);

    /**
    * Symbian 2nd phase constructor 
    */
    void ConstructL();
    
protected:

    /**
    * From CActive. 
    * @param 
    * @return void
    */
    void RunL();

    /**
    * From CActive. 
    * @param 
    * @return void
    */
    void DoCancel();

    /**
    * From CActive. 
    * @param 
    * @return void
    */
    TInt RunError(TInt aError);

 
private:   // Data
 
    MTimerObserver& iObserver;
    
    RTimer iTimer;
    
    TBool iTimerCreated;    
    TBool iTimerRequestPending;
};



class CVideoProcessor : public CVideoDecoder, public MVideoRenderer, 
                        public MTRTranscoderObserver, public MTRVideoPictureSink,
                        public MCMRMediaSink, public MTimerObserver

{
public: // constants
    enum TErrorCode
    {
        EInternalAssertionFailure = -1400,
        EDecoderFailure = -1401
    };

    enum TTimingSource
    {
        ETemporalReference = 0,
        ETimeStamp
    };

    enum TDecoderFrameOperation
    {
        EDecodeAndWrite = 1,
        EDecodeNoWrite,
        EWriteNoDecode,
        ENoDecodeNoWrite
    }; 
    
    enum TTranscoderMode
    {
        EDecodeOnly = 1,
        EFull,
        EFullWithIM,
        EUndefined
    };
	
public: // Data structures
    struct TStreamParameters
    {
        TUint iWidth;
        TUint iHeight;
        TInt64 iPicturePeriodNsec;
        TUint iIntraFrequency;
        TUint iReferencePicturesNeeded; // 0 = disable RPS
        TUint iNumScalabilityLayers;
        TUint iLayerFrameRates[8]; // frames per 256 sec
        TTimingSource iTiming;
    };
    
   
private: // Data structures

    // structure to store information about frames
    // in decoding/transcoding progress 
    struct TFrameInformation
    {
        TTranscoderMode iTranscoderMode;
                
        TInt iFrameNumber;
        TInt64 iTimeStamp;  // timestamp in ticks        
        TBool iEncodeFrame; // ETrue if frame will be encoded        
        TBool iKeyFrame;  
        
        // transition frame info
        TBool iTransitionFrame;  // is this a transition frame?
        TBool iFirstTransitionFrame;  // is this the first transition frame in this instance?
        TTransitionPosition iTransitionPosition;
        TTransitionColor iTransitionColor;        
        TInt iTransitionFrameNumber;
        
        TBool iModificationApplied; // has intermediate modification been done ?
        TBool iRepeatFrame; // ETrue for second instance of a color transition frame
    
    };
    
public: // New functions
    
    /**
    * Two-phased constructor.
    *
    * @param anInputQueue Input active queue
    * @param aStreamParameters Stream parameters
    * @param aProcessor Video processor object
    * @param aStatusMonitor Status monitor object
    * @param aEncoder Video encoder object
    * @param aPriority Active object priority
    */

    static CVideoProcessor* NewL(CActiveQueue *anInputQueue,
                                 CVideoProcessor::TStreamParameters *aStreamParameters,
                                 CMovieProcessorImpl* aProcessor,
                                 CStatusMonitor *aStatusMonitor,   
                                 CVedAVCEdit *aAvcEdit,
                                 TBool aThumbnailMode,
                                 TInt aPriority=EPriorityStandard);


    /**
    * Destructor.
    */
    ~CVideoProcessor();
    
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
    * From CActive Active object error handling method
    */
    TInt RunError(TInt aError);
    
    /**
    * From CDecoder Start video decoding
    */    
    void Start();

    /**
    * From CDecoder Stop video decoding
    */    
    void Stop();    

    /**
    * From CDataProcessor Notifies that input stream has ended
    */
    void StreamEndReached(TAny *aUserPointer);

    /**
    * From CDataProcessor Notifies that new input queue blocks are available
    */
    void InputDataAvailable(TAny *aUserPointer);

	/**
    * From MVideoRenderer Renders a decoded frame
    */
	TInt RenderFrame(TAny* aFrame);
	
    /**
    * From MTRTranscoderObserver
    */
    void MtroInitializeComplete(TInt aError);
    
    /**
    * From MTRTranscoderObserver
    */
    void MtroFatalError(TInt aError);
    
    /**
    * From MTRTranscoderObserver
    */
    void MtroReturnCodedBuffer(CCMRMediaBuffer* aBuffer);
        
    /**
    * From MTRTranscoderObserver
    */
    void MtroSetInputFrameRate(TReal& aRate);
        
    /**
    * From MTRTranscoderObserver
    */
    void MtroAsyncStopComplete();
    
    /**
    * From MTRTranscoderObserver
    */
    void MtroSuspend();
    
    /**
    * From MTRTranscoderObserver
    */
    void MtroResume();
    
    /**	
    * From MTRVideoPictureSink
    */  
    void MtroPictureFromTranscoder(TTRVideoPicture* aPicture);
    
     /**	
     * From MCMRMediaSink
     */        
     void WriteBufferL(CCMRMediaBuffer* aBuffer);
        
     /**	
     * From MCMRMediaSink
     */  
     TInt SetVideoFrameSize(TSize aSize);
        
     /**	
     * From MCMRMediaSink
     */  
     TInt SetAverageVideoBitRate(TInt aBitRate);
        
     /**	
     * From MCMRMediaSink
     */  
     TInt SetMaxVideoBitRate(TInt aBitRate);
        
     /**	
     * From MCMRMediaSink
     */  
     TInt SetAverageAudioBitRate(TInt aBitRate);
     
     /**
     * From MTimerObserver
     */
     void MtoTimerElapsed(TInt aError);
    
public:  // New functions
    
    /**
    * Generates a thumbnail image
    *
    * @param aYUVDataPtr For returning the thumbnail
    * @param aFrameIndex Frame index of the thumbnail image
    *                   -1 means best thumbnail is generated
    * @param aStartFrameIndex Frame index of the start frame (last intra before thumbnail)    
    *
    * @return Error code
    */
    
    /**
    * Starts generating a thumbnail image
    *
    * @param aThumbObserver Observer class
    * @param aFrameIndex Frame index of the thumbnail image
    *                   -1 means best thumbnail is generated
    * @param aStartFrameIndex Frame index of the start frame (last intra before thumbnail)    
    * @param aFactor Pointer to transcode factor structure
    *
    * @return Error code
    */
    TInt ProcessThumb(MThumbnailObserver* aThumbObserver, TInt aFrameIndex, 
                      TInt aStartFrameIndex, TVedTranscodeFactor* aFactor);       
		          
    TInt FetchThumb(TUint8** aYUVDataPtr);
	
	/**
	*	Gets the frame number of the current frame 
	*	
	*/
	TInt GetFrameNumber() { return iFrameNumber; };

	/**
    * Keeps track whether the clip was resolution transcoded 
	*
	*/	
	inline TInt GetChangeResolution() const { return iFullTranscoding; };

	/*	
	* Checks whether the resynch bit is set if set then resets to zero
	* 
	* @return TBool 
	*/	
	TBool CheckVosHeaderL(TPtrC8& aBuf);
	
	/**
	 * Gets the transcode factor from the current clip
	 * @param aFactor For returning the transcode factor
	 * @return error code
	 */
	TInt GetTranscodeFactorL(TVedTranscodeFactor& aFactor);		    
	
    /*	
	* Writes a delayed frame, i.e. a frame which has been buffered
	* since transcoder has unprocesessed frames that must be written
	* before this frame		
	*/	
	void WriteDelayedFrameL();
	
	/*	
	* Checks if there are any frames waiting to be encoded inside transcoder
	* @return TBool 
	*/	
	TBool IsEncodeQueueEmpty();
	
	/* 
	* Calculates encoding delay based on what kind of frame is 
	* next in the encoding queue
	*
	* @return Delay in microseconds
	*/
	TInt GetEncodingDelay();	
	
	/* 
	* Checks if the next frame in processing queue
	* is being encoded
	*
	* @return TBool result
	*/
	TBool IsNextFrameBeingEncoded();
	
    /* 
	* Gets MPEG-4 VOS header size in bytes (from encoder)	
	*
	* @return TInt VOS size
	*/
	TInt GetVosHeaderSize();

private: // internal methods

    /**
    * C++ default constructor.
    *
    * @param anInputQueue Input active queue
    * @param aStreamParameters Stream parameters
    * @param aProcessor Video processor object
    * @param aStatusMonitor Status monitor object
    * @param aEncoder Video encoder object
    * @param aPriority Active object priority
    */
    CVideoProcessor(CActiveQueue *anInputQueue,
                    TStreamParameters *aStreamParameters,
                    CMovieProcessorImpl* aProcessor,
                    CStatusMonitor *aStatusMonitor,
                    CVedAVCEdit *aAvcEdit,
                    TBool aThumbnailMode,
                    TInt aPriority);
    

    /** 
    * Symbian 2nd phase constructor 
    */
    void ConstructL();

    /**
    * Processes one input video frame
    *
    * @return ETrue If the current clip has been 
    *         processed entirely, EFalse if not
    */
    TBool ProcessFrameL();
    
    /** 
    * Processes the second frame of a color transition double frame
    */
    TBool Process2ndColorTransitionFrameL();

    /**
    * Read one frame from input queue to internal buffer
    *
    * @return ETrue if a complete frame was read, EFalse otherwise.
    */
    TInt ReadFrame();

    /**
    * Reads a H.263 frame from input queue to internal buffer
    *
    * @return ETrue if a complete frame was read, EFalse otherwise.
    */    
    TBool ReadH263Frame();
    
    /**
    * Reads a MPEG-4 frame from input queue to internal buffer
    *
    * @return ETrue if a complete frame was read, EFalse otherwise.
    */
    TBool ReadMPEG4Frame();
    
    /**
    * Reads an AVC frame from input queue to internal buffer
    *
    * @return ETrue if a complete frame was read, EFalse otherwise.
    */
    TBool ReadAVCFrame();

    /**
    * Checks if a frame has "good" or "legible" quality
    *
    * @param aYUVDataPtr Pointer to the frame to be checked
    *
    * @return 1 if frame quality is OK, 0 otherwise
    */
    TInt CheckFrameQuality(TUint8* aYUVDataPtr);

    /**
    * Apply color effect on a frame
    *
    * @param aColorEffect Effect to be used
    * @param aYUVDataPtr Pointer to the frame
    * @param aColorToneYUV for extracting the UV values
    * @return void
    */
    void ApplySpecialEffect(TInt aColorEffect, TUint8* aYUVDataPtr,
      TInt aColorToneU, TInt aColorToneV);    

    /**
    * Convert frame operation enumeration to TInt
    *
    * @param aFrameOperation Frame operation     
    *
    * @return Frame operation as TInt
    */
    TInt TFrameOperation2TInt(TDecoderFrameOperation aFrameOperation); 

    /**
    * Convert color effect enumeration to TInt
    *
    * @param aColorEffect Color effect
    *
    * @return Color effect as TInt
    */
    TInt TColorEffect2TInt(TVedColorEffect aColorEffect);

    /**
    * Determines transition effect parameters
    *
    * @param aTransitionEffect Output: ETrue if transition effect is to be applied
    * @param aStartOfClipTransition ETrue if starting transition is to be applied
    * @param aEndOfClipTransition ETrue if ending transition is to be applied
    * @param aStartTransitionColor Color for starting transition
    * @param aEndTransitionColor Color for ending transition
    *
    * @return Error code
    */
    TInt DetermineClipTransitionParameters(TInt& aTransitionEffect,TInt& aStartOfClipTransition,
			TInt& aEndOfClipTransition,TTransitionColor& aStartTransitionColor,TTransitionColor& aEndTransitionColor);

    
    /**
    * Applies fading transition effect to YUV frame
    *
    * @param aYUVPtr Pointer to the frame
    * @param aTransitionPosition 1 = start transition, 2 = end transition
    * @param aTransitionColor Transition color to be used (EColorWhite/EColorBlack/EColorTransition)
    * @param aTransitionFrameNumber ordinal number of transition frame (0 - (number of transition frames - 1))
    *
    * @return void
    */
    void ApplyFadingTransitionEffect(TUint8* aYUVPtr, TTransitionPosition aTransitionPosition,
                                     TTransitionColor aTransitionColor, TInt aTransitionFrameNumber);

	/**
    * Applies blending transition effect between YUV frames
    *
    * @param aYUVPtr1 Pointer to the frame 1
    * @param aYUVPtr2 Pointer to the frame 2
    * @param aRepeatFrame True for the second instance of two frames to be blended
    * @param aTransitionFrameNumber ordinal number of transition frame (0 - (number of transition frames - 1))
    *
    * @return void
    */
    void ApplyBlendingTransitionEffect(TUint8* aYUVPtr1,TUint8* aYUVPtr2, TInt aRepeatFrame, 
                                       TInt aTransitionFrameNumber);
    
		
    /**
    * Applies sliding transition effect between YUV frames
    *
    * @param aYUVPtr1 Pointer to the frame 1
    * @param aYUVPtr2 Pointer to the frame 2
    * @param aRepeatFrame True for the second instance of two frames to be blended
    * @param aTransitionFrameNumber ordinal number of transition frame (0 - (number of transition frames - 1))
    *
    * @return void
    */
    void ApplySlidingTransitionEffect(TUint8* aYUVPtr1,TUint8* aYUVPtr2, TVedMiddleTransitionEffect aVedMiddleTransitionEffect, 
                                      TInt aRepeatFrame, TInt aTransitionFrameNumber);
		
    /**
    * Get the start transition info of the next clip
    *
    * @return void
    */
    void GetNextClipTransitionInfo();

	/*
	* Resolution Transcoder
	* Determine if the resolution transcoding will apply to the current clip
	* 
	*/
	TBool DetermineResolutionChange();

    /*
	* 
	* Determine if frame rate needs to be changed for the current clip
	* 
	*/
    TBool DetermineFrameRateChange();

    /*
	* 
	* Determine if bitrate needs to be changed for the current clip
	* 
	*/
    TBool DetermineBitRateChange();
    
    /*	
	* Calculate the duration of current frame
	*	
	* @param aFrameNumber frame number 
	*
	* @return Frame duration in ticks
	*/    
    TInt GetFrameDuration(TInt aFrameNumber);
    
    /*	
	* Decode frame using vedh263decoder
	* 
	* @param aOperation Operation 
	* @param aModeChanged ETrue if compressed domain transcoding is neede
	* @param aFrameSizeInBytes Return value, coded output frame size in bytes
	* @param aVosHeaderSize Return value, size of VOS header in bytes
	*/  
    void DecodeFrameL(TInt aOperation, TBool aModeChanged, TInt& aFrameSizeInBytes);        
    
    /*
    * Create and initialize transcoder
    *
    * @param aInputType Input video type 
    * @param aMode Transcoder operational mode to be used
    *    
    */
    void CreateAndInitializeTranscoderL(TVedVideoType aInputType, CTRTranscoder::TTROperationalMode aMode);
    
    /*
    * Determines if compressed domain transcoding is needed
    *
    */
    TBool GetModeChangeL();
    
    /*
    * Processes a thumbnail frame
    *
    * @param aFirstFrame ETrue if the first thumbnail frame is being processed
    */
    void ProcessThumb(TBool aFirstFrame);
    
    /*
    * Calculate the number of transition frames
    *
    * @param aStartCutTime Start cut time
    * @param aEndCutTime End cut time
    */
    void GetNumberOfTransitionFrames(TTimeIntervalMicroSeconds aStartCutTime, 
                                     TTimeIntervalMicroSeconds aEndCutTime);
      
    /*
    * Set parameters needed to process a transition frame
    *
    * @param aIncludedFrameNumber Ordinal counting from the first included frame number
    * @param aDecodeFrame ETrue if this frame must be decoded in order to apply end transition
    */                               
    void SetTransitionFrameParams(TInt aIncludedFrameNumber, TBool& aDecodeFrame);
    
    /*
    * Sets output codec parameters
    * 
    * aMimeType Output mime type
    */
    void SetOutputVideoCodecL(const TPtrC8& aMimeType);
    
    /*
    * Generates bitstream for a not coded frame
    * to be used in color transitions
    */
    void GenerateNotCodedFrameL();
    
    /*
    * Reads a frame from input queue and writes it to transcoder
    *
    */
	void ReadAndWriteThumbFrame();	
	
	/*
    * Does thumbnail finalization, stops processing and informs observer
    *
    * aError Error code for observer
    */
	void FinalizeThumb(TInt aError);
	
	/*
    * Writes a frame to output file
    *  
    * @param aBuf Buffer containing the frame
    * @param aDurationInTicks Frame duration in ticks
    * @param aFrameNumber Frame number
    * @return ETrue if clip end was written
    */
	TBool WriteFrameToFileL(TPtr8& aBuf, TInt aDurationInTicks, TInt aFrameNumber);
	
	/*
    * Writes a frame to transcoder
    *      
    * @param aFrameNumber Frame number
    * @param aKeyFrame ETrue for a keyframe
    * @param aVolHeaderInBuffer ETrue if frame buffer contains MPEG-4 VOL header
    */
	void WriteFrameToTranscoderL(TInt aFrameNumber, TBool aKeyFrame, TBool aVolHeaderInBuffer);
	
    /*
    * Handles a decoded thumbnail frame received from transcoder
    *      
    * @param aPicture Pointer to received picture
    */
	void HandleThumbnailFromTranscoder(TTRVideoPicture* aPicture);
	
	/*
    * Handles a decoded "decode-only" frame received from transcoder
    *      
    * @param aPicture Pointer to received picture
    * @param aIndex Index of decoded frame to iFrameInfoArray
    */
	void HandleDecodeOnlyFrameFromTranscoder(TTRVideoPicture* aPicture, TInt aIndex);
	
	/*
    * Handles a decoded transition frame (intermediate frame) received from transcoder
    *      
    * @param aPicture Pointer to received picture
    * @param aIndex Index of decoded frame to iFrameInfoArray
    */
	void HandleTransitionFrameFromTranscoder(TTRVideoPicture* aPicture, TInt aIndex);
	
    /*
    * Inserts AVC/MPEG-4 decoder specific info data in front of internal coded frame buffer
    *
    */
	void InsertDecoderSpecificInfoL();

private: // internal constants
    enum TDataFormat
    {
        EDataUnknown = 0,
        EDataH263,
        EDataMPEG4,
        EDataAVC
    };


private: // data
    CActiveQueue *iQueue; // input data queue
    TBool iReaderSet; // have we been set as reader?
    CStatusMonitor *iMonitor; // status monitor object
    CMovieProcessorImpl *iProcessor;

    CVedH263Dec *iDecoder;  // H.263/MPEG-4 decoder    

    TUint iVideoWidth, iVideoHeight; // video picture dimensions
    TInt64 iPicturePeriodNsec; // one PCF tick period in nanoseconds
    TDataFormat iDataFormat; // the stream data format
    TUint iIntraFrequency; // intra picture frequency (intras per 256 sec)
    TUint iReferencePicturesNeeded; // number of reference pictures needed
    
    TBool iFirstFrameFlag; // flag for decoder: is this the first frame?

    TDecoderFrameOperation iFrameOperation; 

    TInt iNumberOfIncludedFrames; 

    TBool iDecoding; // are we decoding?
    TBool iStreamEnd; // has stream end been reached?    
    TBool iEncoderResetPending; // is the encoder being reseted

    TPtr8 *iBlock; // queue block
    TUint iBlockPos; // current block position
    TUint8 *iDataBuffer; // data buffer for the current compressed frame
    TUint iBufferLength; // buffer total length
    TUint iDataLength; // amount of data in buffer
    TUint iCurrentFrameLength; // the length of the current frame in the buffer

    TUint8 *iFrameBuffer;  // Concatenated YUV data for decoded frame

    TUint8 *iOutVideoFrameBuffer; // data buffer for the output compressed frame    
    TInt iOutVideoFrameBufferLength; // buffer total length
    TInt iOutDataLength; // amount of data in buffer
    TPtr8 iWriteDes;  // writing descriptor for encoding
    TInt iFrameNumber;  // current frame number
    TInt iNumberOfFrames;  // no. of frames in current clip
    TInt iPreviousFrameIncluded;  // ETrue if previous frame was included in output movie
    TInt  iTrPrevious;

    // transition effects
    TInt iTransitionEffect;	    // is transition effect to be applied?
    TInt iStartOfClipTransition;  // is starting transition effect to be applied ?
    TInt iEndOfClipTransition; // is ending transition effect to be applied ?

	TInt iStartNumberOfTransitionFrames;  // number of frames in transition
	TInt iEndNumberOfTransitionFrames;  // number of frames in transition
	TInt iTransitionFrameNumber;
	TInt iNextTransitionNumber;    
    
    TInt iFirstFrameInRange;  // is the current frame the first to be included in output movie
    TInt iFirstIncludedFrameNumber;  // number of first included frame
    TTransitionColor iStartTransitionColor;	// color for starting transition
    TTransitionColor iEndTransitionColor;// color for ending transition
    
    // number of preceding I frame (for end-of-clip trans.)this number is relative of start-of-clip
    TInt iLastIntraFrameBeforeTransition; //The last intra frame before transition effect takes place.       

    TTimingSource iTiming;  

	// for transition effect - blending and sliding
	TInt    iFrameDuration;
	TBool   iRepeatFrame;
	TInt64  iTimeStamp;
	TUint8 *iOrigPreviousYUVBuffer;
	TUint8 *iColorTransitionBuffer;

	CMPEG4Timer *iMPEG4Timer;		// Stores MPEG-4 timing information
	
	CTRTranscoder *iTransCoder;	
	CCMRMediaBuffer* iMediaBuffer;	
	
	TBool iTranscoderInitPending;  // is transcoder init in progress ?
	TBool iDecodePending;  // is decode in progress ?
	TBool iTranscoderStarted; // has transcoder been started ?
	TBool iFrameToEncode;  // should current frame be encoded ? (: could be changed to local for ProcessFrame)	
	
	TBool iTransitionFrame;
	TBool iFirstTransitionFrame;
	TTransitionPosition iTransitionPosition;
    TTransitionColor iTransitionColor;	
    
    // for color toning
    TInt   iFirstFrameQp;       // QP for first MPEG4 frame, used for color toning 
	
	TBool iFullTranscoding; // Flag to indicate whether current clip needs to be fully
	                        // transcoded (decode & encode)   
    
    TBool iThumbnailMode;   // True if we are generating a thumbnail frame
    
    TInt iThumbFrameIndex;  // index of thumbnail to be generated, >= 0 means a specific frame,
                            // < 0 means the first "good" or "legible" frame
    TInt iThumbFrameNumber;
    TInt iFramesToSkip;     // for searching a good thumbnail
    
    MThumbnailObserver* iThumbObserver;  // observer for thumbnail operation
    
    HBufC8* iDecoderSpecificInfo;  // decoder specific info header read from input stream, to be sent to transcoder
    TBool iDecoderSpecificInfoSent;  // True if decoder specific info has been sent to transcoder
    
    HBufC8* iOutputVolHeader;  // VOL header read from encoder, to be inserted in the first
                               // encoded frame buffer
                               
    TBool iOutputVolHeaderWritten;  // True if VOL/AVC header has been written to output bitstream
    
    RArray<TFrameInformation> iFrameInfoArray;  // array for storing info about frames in progress
    
    TTranscoderMode iTranscoderMode;  // current transcoder operation mode

    // used & max frame size, and info if size can be arbitrary    
    TSize iMaxOutputResolution;
    TBool iArbitrarySizeAllowed;

    // max allowed frame rate
    TReal32 iMaxOutputFrameRate;           

	// target and max allowed bit rate
	TInt iOutputBitRate;
	TInt iMaxOutputBitRate;        
	
	// Mime type for encoded data
    TBuf8<256> iOutputMimeType;
    
    TBool iDelayedWrite;  // True if a frame has to be stored for later writing since
                          // there are frames inside the transcoder that have to be written
                          // before this frame
    HBufC8* iDelayedBuffer;  // buffer for storing the frame  
    TInt64 iDelayedTimeStamp;  // timestamp
    TBool iDelayedKeyframe;  // is it a keyframe            

    TInt iTimeStampIndex;  // index for reading stored timestamps in case of color transition
    TBool iFirstColorTransitionFrame;  // True if we are processing the first frame of a
                                       // 'doubled' frame in color transitions
    
    TInt iTimeStampOffset;  // offset to be added to timestamps, got from processor
    
    CCallbackTimer* iTimer;  // timer for waiting encoding to complete
    
    TBool iStreamEndRead;
    
    TBool iProcessingComplete;
    
    TInt iInputMPEG4ProfileLevelId;  // profile-level id for MPEG-4 input
    
    // Timer timeout, depends on codec and resolution
    TUint iMaxEncodingDelay;
    
    TInt iNumThumbFrameSkips;  // number of frame skip cycles done to get a 
                               // a good thumbnail
                               
    TTimeIntervalMicroSeconds iPreviousTimeStamp; // timestamp of previous frame        

    TInt iSkippedFrameNumber;  // frame number of skipped frame
    
    TInt64 iCutInTimeStamp;  // timestamp of cut-in point, needed in color transitions
    
    TBool iDecodingSuspended;  // flag for suspended decoding
    
    HBufC8* iNotCodedFrame;  // buffer for not coded frame
    
    TInt iInputTimeIncrementResolution;  // MPEG-4 time increment resolution from input clip
    
    TBool iFirstFrameAfterTransition;  // True for first frame after start transition
    
    TBool iFirstRead;  // True for first read frame        

    TBool iThumbDecoded;  // flag for indicating when a thumb has been decoded,
                          // needed in AVC thumbnail generation
    
    TInt iInputAVCLevel;  // level of input AVC clip
    
    TInt iFrameLengthBytes;  // number of bytes used for NAL length in input
    TInt iThumbFramesToWrite;  // number of thumb frames to write to transcoder
    
    CVedAVCEdit* iAvcEdit;  // AVC editing instance
    
    TBool iEncodeUntilIDR;  // Flag for encoding until IDR  

    TBool iIsThumbFrameBeingCopied;  // True if thumbnail frame has been sent for
                                     // decoding, but MtroReturnCodedBuffer hasn't been called
                                     
    TInt iModifiedFrameNumber;  // frame number modification for AVC decoder, 
                                // used with color transitions    
                                
    TInt iMaxItemsInProcessingQueue;  // Maximum number of frames kept in transcoder
                                      // processing queue
                                      
    TInt iLastWrittenFrameNumber;  // number of last written frame    
    
    TInt iDelayedFrameNumber;  // number of delayed frame
    
    TBool iInitializing;  // True when initialisation is ongoing
    TVedVideoBitstreamMode iInputStreamMode;    //MPEG-4 bitstream mode in input clip
};

#endif      //  __VIDEOPROCESSOR_H__
            
// End of File
