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
* Header file for video encoder class.
*
*/


#ifndef VIDEOENCODER_H
#define VIDEOENCODER_H

// INCLUDES

#include <CCMRMediaSink.h>
#include "CTRTranscoderObserver.h"
#include "CTRTranscoder.h"
#include "CTRCommon.h"
#include "CTRVideoPictureSink.h"
#include "Vedcommon.h"

// FORWARD DECLARATIONS

class CStatusMonitor;
class CVedVolReader;
class CVedAVCEdit;

class CVideoEncoder : public CActive, public MTRTranscoderObserver, 
                      public MCMRMediaSink, public MTRVideoPictureSink
    {

    public:  // Constructors and destructor
        
        /**
        * Two-phased constructor.
        */
		static CVideoEncoder* NewL(CStatusMonitor *aMonitor, CVedAVCEdit* aAvcEdit, 
		                           const TPtrC8& aVideoMimeType);
        
        /**
        * Destructor.
        */
        
        ~CVideoEncoder();               

    public:   // Constants        
        
        enum TErrorCode
            {
            EInternalAssertionFailure = -10030,
            };
            
    public: // Functions from base classes

        /**
        * From CActive Active object running method
        */
        void RunL();
        
        /**
        * From CActive Active object error method
        */
        TInt RunError(TInt aError);
        
        /**
        * From CActive Active object cancelling method
        */
        void DoCancel();  

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
        inline void MtroSuspend() { };
        
        /**
        * From MTRTranscoderObserver
        */
        inline void MtroResume() { };
        
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
        * From MTRVideoPictureSink
        */  
        void MtroPictureFromTranscoder(TTRVideoPicture* aPicture);

    public:  // New functions

        /**
        * Sets the used frame size
        * 
        * @param aSize Frame size 
        * @return Error codew
        */
        void SetFrameSizeL(const TSize& aSize);

        /**
        * Gets the current frame size
        * 
        * @param aSize Frame size 
        * @return void
        */
        void FrameSize(TSize& aSize) const;

        /**
        * Sets the target frame rate
        * 
        * @param aFrameRate frame rate
        * @return Error code
        */
        TInt SetFrameRate(const TReal aFrameRate);
        
        /**
        * Sets the frame rate of input sequence
        * 
        * @param aFrameRate frame rate
        * @return Error code
        */
        TInt SetInputFrameRate(const TReal aFrameRate);
        
        /**
        * Sets the target random access rate
        * 
        * @param aRate random access rate
        * @return Error code
        */
        TInt SetRandomAccessRate(const TReal aRate);

        /**
        * Sets the target bitrate
        * 
        * @param aBitrate Bitrate 
        * @return Error code
        */
        TInt SetBitrate(const TInt aBitrate);
        
        /**
        * Initializes the encoder
        * 
        * @param aStatus Status object for active object
        * @return void
        */        
        void InitializeL(TRequestStatus &aStatus);
               
        /**
        * Encodes a frame
        * 
        * @param aYUVFrame Frame to be encoded
        * @param aStatus Status object for active object
        * @param Timestamp of the frame
        * @return void
        */        
        void EncodeFrameL(TPtr8 &aYUVFrame, TRequestStatus &aStatus, TTimeIntervalMicroSeconds aTimeStamp);
        
        /**
        * Gets the encoded bitstream
        *         
        * @param aKeyFrame Keyframe flag, True if the frame is Intra
        * @return Descriptor containing the output bitstream
        */   
        TPtrC8& GetBufferL(TBool& aKeyFrame);

        /**
        * Return used bitstream buffer
        *         
        * @return void
        */         
        void ReturnBuffer();        

        /**
        * Starts the encoder
        *         
        * @return void
        */ 
        void Start();

        /**
        * Stops the encoder
        *         
        * @return void
        */ 
        void Stop();

        /**
        * Resets the encoder
        *         
        * @param aStatus Status object for active object
        * @return void
        */ 
        void Reset(TRequestStatus &aStatus);
        
        /**
        * Resets the encoder synchronously
        *        
        * @return void
        */ 
        void Reset();

		/**
        * Gets status of whether encoding has been started
        *                 
        * @return ETrue if encoding has been started
        */ 
		inline TBool BeenStarted() const { return iStarted; }

		/**
        * Gets status of whether encoding request is pending
        *                 
        * @return ETrue if encode is pending
        */ 
		inline TBool IsEncodePending() const { return iEncodePending; }

        /**
        * Gets time increment resolution used in MPEG-4 
        *                 
        * @return Time increment resolution
        */ 
        TInt GetTimeIncrementResolution() const;
        
        /**
        * Forces the next encoded frame to be Intra
        *
        */ 
        void SetRandomAccessPoint();
            
    private:

        /**
        * C++ default constructor.
        */
        CVideoEncoder();

        /**
        * By default EPOC constructor is private.
        */
		void ConstructL(CStatusMonitor *aMonitor, CVedAVCEdit* aAvcEdit, 
		                const TPtrC8& aVideoMimeType);

        /**
        * Interpret and store video codec MIME-type, used by ConstructL
        * @return void
        */
        void SetVideoCodecL(const TPtrC8& aMimeType);
        
        /**
        * Set encoding options
        * @return void
        */
        void SetupEncoderL();
        
        /**
        * Parses and writes codec specific information, i.e
        * MPEG-4 VOL header and AVC decoder configuration record
        * @return void
        */
        void HandleCodingStandardSpecificInfoL();

    private:    // Data
    
        // Transcoder
        CTRTranscoder* iTranscoder;

		// is encode pending ?
		TBool iEncodePending;

		// has encoding been started ?
		TBool iStarted;

        // used & max frame size, and info if size can be arbitrary
        TSize iFrameSize;
        TSize iMaxResolution;
        TBool iArbitrarySizeAllowed;

        // target and max allowed frame rate
        TReal32 iFrameRate;        
        TReal32 iMaxFrameRate;
        
        // input frame rate
        TReal32 iInputFrameRate;
        
        // target random access rate
        TReal iRandomAccessRate;

		// target and max allowed bit rate
		TInt iBitRate;
		TInt iMaxBitRate;

        // Mime type for encoded data
        TBuf8<256> iMimeType;        

        // input picture         
        TTRVideoPicture* iInputPicture;     
        
        // bitstream buffer
        HBufC8* iDataBuffer;
        
        // keyframe flag for bitstream buffer
        TBool iKeyFrame;
        
        TPtrC8 iReturnDes;

        // External status info
        TRequestStatus *iResetRequestStatus;

		TRequestStatus *iEncodeRequestStatus;

        TTimeIntervalMicroSeconds iPreviousTimeStamp;

        // MPEG-4 time increment resolution
        TInt iTimeIncrementResolution;        

        // input YUV data
        TPtr8 iInputBuffer;

        // status monitor object
        CStatusMonitor *iMonitor;
        
        // video object layer reader
        CVedVolReader *iVolReader;

        RTimer iTimer; // timer object
        TBool iTimerCreated;
        TBool iTimerRequestPending;
        
        // Timer timeout, depends on codec and resolution
        TUint iMaxEncodingDelay;

        // flag to check if devvideorec methods can be called or not; they can't be called after a fatal error 
        TBool iFatalError;

#ifdef _DEBUG
        TTime iEncodeStartTime;
#endif
        
        // AVC editing module
        CVedAVCEdit* iAvcEdit;
        
    };



#endif
