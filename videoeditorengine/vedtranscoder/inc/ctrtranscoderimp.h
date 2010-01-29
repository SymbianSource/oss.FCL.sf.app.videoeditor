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
* Transcoder.
*
*/


#ifndef CTRTRANSCODERIMP_H
#define CTRTRANSCODERIMP_H


// INCLUDES
#include "ctrtranscoder.h"
#include "ctrdevvideoclientobserver.h"


// FORWARD DECLARATIONS
class CTRVideoDecoderClient;
class CTRVideoEncoderClient;
class CTRScaler;


// CONSTANTS
const TInt KTRErrNotReady = -1001;


// CLASS DECLARATION

/**
*  CTranscoder Implementation class
*  @lib TRANSCODER.LIB
*  @since 3.1
*/
NONSHARABLE_CLASS(CTRTranscoderImp) : public CTRTranscoder, public MTRDevVideoClientObserver, public CActive
    {
    public: // Constructors and destructor

        /**
        * Two-phased constructor.
        */
        static CTRTranscoderImp* NewL(MTRTranscoderObserver& aObserver);

        /**
        * Destructor.
        */
        ~CTRTranscoderImp();

    public: // Transcoder methods
        /**
        * From CTranscoder. Open the transcoder. Instantiates other submodules based on the selected operational mode. 
        * @param aMediaSink MCMRMediaSink for encoded data output
        * @param aMode Mode for the transcoder, operation to perform
        * @param aMimeTypeInputFormat  Mime type for input format
        * @param aMimeTypeOutputFormat Mime type for otput format
        * @param aRealTimeProcessing Specifies how to process data. ETrue - Real-time processing; EFalse - non-real time. 
        * If the processing cannot be performed in real-time, the transcoder process data as fast as it can. 
        * @return void. Can leave (KErrNotSupported - selected operation or codecs are not supported, 
        * @ or any other Symbian error code )
        */
        void OpenL( MCMRMediaSink* aMediaSink, 
                    CTRTranscoder::TTROperationalMode aMode, 
                    const TDesC8& aInputMimeType, 
                    const TDesC8& aOutputMimeType, 
                    const TTRVideoFormat& aVideoInputFormat, 
                    const TTRVideoFormat& aVideoOutputFormat, 
                    TBool aRealTime );
        
        // Information methods
        /**
        * Check support for input format
        * @param aMimeType MIME type
        * @return TBool: ETrue - supports; EFalse - does not support
        */
        TBool SupportsInputVideoFormat(const TDesC8& aMimeType);

        /**
        * Check support for output format
        * @param aMimeType MIME type
        * @return TBool: ETrue - supports; EFalse - does not support
        */
        TBool SupportsOutputVideoFormat(const TDesC8& aMimeType);

        // Control
        /**
        * From CTranscoder. Initializes the transcoder. Initializing completes asynchronously with MtroInitializeComplete observer callback.
        * @param none
        * @return void
        */
        void InitializeL();

        /**
        * From CTranscoder. Starts transcoding. 
        * @param none
        * @return void. Can leave with Symbian error code in case of an error. 
        */
        void StartL();

        /**
        * From CTranscoder. Stops transcoding synchronously. Use this method to stop data processing synchronously. In this case 
        * all active requests are cancelled and data that was not processed is returned to data owner immediately. 
        * @param none
        * @return void. Can leave with Symbian error code in case of an error. 
        */
        void StopL();

        /**
        * From CTranscoder. Stops transcoding asynchronously. The transcoder use this signal to ensure that the remaining data gets processed, 
        * without waiting for new data. This method is mainly useful for file-to-file conversions and other non-realtime 
        * processing. 
        * @param none
        * @return void. Can leave with Symbian error code in case of an error. 
        */
        void AsyncStopL();

        /**
        * From CTranscoder. Pauses transcoding.
        * @param 
        * @return void. Can leave with Symbian error code in case of an error. 
        */
        inline void PauseL(){};

        /**
        * From CTranscoder. Resumes transcoding. 
        * @param 
        * @return void. 
        */
        inline void Resume(){};

        // Settings
        /**
        * From CTranscoder. Sets bit rate parameter for transcoded bitstream. Can be run-time setting
        * @param aBitRate Bitrate
        * @return void Can leave with KErrNotSupported
        */
        void SetVideoBitRateL(TUint aBitRate);

        /**
        * From CTranscoder. Sets frame rate for decoded bitstream. Can be run-time setting. 
        * @param aFrameRate Frame rate
        * @return void Can leave with KErrNotSupported
        */
        void SetFrameRateL(TReal aFrameRate);

        /**
        * From CTranscoder. Sets channel bit error rate. Can be run-time setting
        * @param aErrorRate Channel expected error rate.
        *        aErrorRate = 0.0     - low bit error rate or error is not expected
        *        aErrorRate = 0.00001 - medium bit error rate
        *        aErrorRate = 0.0001  - high bit error rate
        * @return void
        */
        void SetChannelBitErrorRateL(TReal aErrorRate);

        /**
        * From CTranscoder. Sets misc video coding options
        * @param aOptions Coding options
        * @return void
        */
        void SetVideoCodingOptionsL(TTRVideoCodingOptions& aOptions);

        /**
        * From CTranscoder. Makes a request to retrieve the intermediate content from the transcoder
        * @param aRetrieve: ETrue - retrieve; EFalse - don't retrieve.
        * @param aSize Requested picture size for intermediate decoded picture
        * @return void Can leave with KErrNotSupported, if the intermediate picture size is not supported
        */
        void SetVideoPictureSinkOptionsL(TSize& aSize, MTRVideoPictureSink* aSink);

        /**
        * From CTranscoder. Makes a request to use direct screen access
        * @param aUseDSA Use Direct Screen Access (DSA). ETrue - use DSA; EFalse - don't use DSA. 
        * @param aOptions Display options, valid only if aUseDSA is set to ETrue
        * @return void Can leave with KErrNotSupported, the DSA is not supported. 
        */
        void UseDirectScreenAccessL(TBool aUseDSA, CFbsScreenDevice& aScreenDevice, TTRDisplayOptions& aOptions);

        /**
        * From CTranscoder. Gets the current output bit rate for transcoded bit stream
        * @param none
        * @return TUint Bit rate
        */
        TUint GetVideoBitRateL();

        /**
        * From CTranscoder. Gets the current frame rate for transcoded stream
        * @param none
        * @return TReal Framerate
        */
        TReal GetFrameRateL();

        /**
        * From CTranscoder. Retrieve currently used codec for transcoding stream
        * @param aVideoMimeType video mime type
        * @return void
        */
        void GetVideoCodecL(TDes8& aVideoMimeType);

        /**
        * From CTranscoder. Get transcoded picture size
        * @param aSize Picture size
        * @return void
        */
        void GetTranscodedPictureSizeL(TSize& aSize);

        // Data transfer methods
        /**
        * Sends filled buffer with new portion of the bitstream to transcoder. Note, this is asynchronous operation, 
        * and this bufer is returned back with Transcoder Observer call MtroReturnCodedBuffer(aBuffer); 
        * @param aBuffer Media buffer. It is not allowed to use this buffer on the client side until it is returned back 
        * from the transcoder with MtroReturnCodedBuffer(aBuffer).
        * @return void 
        */
        void WriteCodedBufferL(CCMRMediaBuffer* aBuffer);

        /**
        * From CTranscoder. Sends video picture to transcoder / releases decoded picture (Decode mode)
        * @param aPicture Video picture
        * @return void
        */
        void SendPictureToTranscoderL(TTRVideoPicture* aPicture);

        /**
        * Resample picture in Resampling only mode. The client should specify souce picture and buffer for target 
        * resampled picture
        * @param aSrc Source picture
        * @param aTrg Target picture
        * @return void
        */
        void ResampleL(TPtr8& aSrc, TPtr8& aTrg);

        // MTRDevVideoClientObserver implementation
        /**
        * From MTRDevVideoClientObserver. Reports an error from the devVideo client
        * @param aError Error reason
        * @return void
        */
        void MtrdvcoFatalError(TInt aError);

        /**
        * From MTRDevVideoClientObserver. Reports that data encoding process has been finished
        * @param none
        * @return void
        */
        void MtrdvcoEncStreamEnd();

        /**
        * From MTRDevVideoClientObserver. Reports that data decoding process has been finished
        * @param none
        * @return void
        */
        void MtrdvcoDecStreamEnd();

        /**
        * From MTRDevVideoClientObserver. Returns video picture from the video encoder client
        * @param aPicture video picture
        * @return void
        */
        void MtrdvcoEncoderReturnPicture(TVideoPicture* aPicture);

        /**
        * From MTRDevVideoClientObserver. Returns videopicture from the renderer
        * @param aPicture video picture
        * @return void
        */
        void MtrdvcoRendererReturnPicture(TVideoPicture* aPicture);

        /**
        * From MTRDevVideoClientObserver. Informs about initializing video encoder client
        * @param aError Initializing error status 
        * @return void
        */
        void MtrdvcoEncInitializeComplete(TInt aError);

        /**
        * From MTRDevVideoClientObserver. Informs about initializing video decoder client
        * @param aError Initializing error status 
        * @return void
        */
        void MtrdvcoDecInitializeComplete(TInt aError);

        /**
        * From MTRDevVideoClientObserver. Supplies new encoded bitstream buffer
        * @param aPicture Decoded picture
        * @return void
        */
        void MtrdvcoNewPicture(TVideoPicture* aPicture);

        /**
        * Supplies new encoded bitstream buffer
        * @param 
        * @return void
        */
        void MtrdvcoNewBuffer(CCMRMediaBuffer* aBuffer);
        /**
        * Returns media bitstream buffer to the client
        * @param aBuffer Bitstream media buffer
        * @return void
        */
        void MtrdvcoReturnCodedBuffer(CCMRMediaBuffer* aBuffer);
        
        
        //  AA
        void SetDecoderInitDataL(TPtrC8& aInitData);
        
        /**
        * Gets coding-standard specific initialization output. The buffer is pushed to the cleanup stack, 
        * and the caller is responsible for deallocating it. 
        * @param none
        * @return HBufC8 Coding options (VOL header)
        */
        HBufC8* GetCodingStandardSpecificInitOutputLC();

        /**
        * Enable / Disable use of picture sink; Note: This method is relevant only for full transcoding use cases;
        * 
        * @param aEnable ETrue: Picture sink is enabled; EFalse: Picture sink is disabled
        * @return none
        */
        void EnablePictureSink(TBool aEnable);
    
        /**
        * Enable / Disable use of Encoder; Note: This method is relevant only for full transcoding use cases;
        * By default encoding is always enabled in FullTranscoding mode
        * 
        * @param aEnable ETrue: Encoder is enabled; EFalse: Encoder is disabled
        * @return none
        */
        void EnableEncoder(TBool aEnable);
        
        /**
        * Requests random access point to the following transcoded picture
        * This change is applied for the next written coded buffer with WriteCodedBufferL
        * @param none
        * @return none
        */
        void SetRandomAccessPoint();
        
        /**
        * From MTRDevVideoClientObserver. Notifies the transcoder about available picture buffers throug CI or MDF
        * @param none
        * @return none
        */
        void MtrdvcoNewBuffers();
        
        /**
        * Returns a time estimate of how long it takes to process one second of the input video with the
        * current transcoder settings. OpenL must be called before calling this function. Estimate
        * will be more accurate if InitializeL is called before calling this function.
        * @param aInput Input video format
        * @param aOutput Output video format
        * @return TReal Time estimate in seconds
        */
        TReal EstimateTranscodeTimeFactorL(const TTRVideoFormat& aInput, const TTRVideoFormat& aOutput);
        
        /**
        * From CTranscoder. Get max number of frames in processing.
        * This is just a recommendation, not a hard limit.
        * @return TInt Number of frames
        */
        TInt GetMaxFramesInProcessing();
        
        /**
        * Enable / Disable pausing of transcoding if resources are lost. If enabled transcoder
        * will notify the observer about resource losses by calling MtroSuspend / MtroResume
        * @param aEnable ETrue: Pausing is enabled
        *                EFalse: Pausing is disabled and resource losses will cause a fatal error
        * @return none
        */
        void EnablePausing(TBool aEnable);
        
        /**
        * Indicates that a media device has lost its resources
        * @param aFromDecoder Flag to indicate source of resource loss 
        * @return none
        */
        void MtrdvcoResourcesLost(TBool aFromDecoder);
        
        /**
        * Indicates that a media device has regained its resources
        * @return none
        */
        void MtrdvcoResourcesRestored();

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
        * @param aError error
        * @return void
        */
        TInt RunError(TInt aError);

    private:
        /**
        * C++ default constructor.
        */
        CTRTranscoderImp(MTRTranscoderObserver& aObserver);

        /**
        * Symbian 2nd phase constructor, can leave
        */
        void ConstructL();

        /**
        * Parses mime type
        * @param aInputMimeType Input mime tipe
        * @param aVideoInputFormat Output mime type
        * @param aInOutMime ETrue - input mime, EFalse
        * @return void
        */
        void ParseMimeTypeL(const TDesC8& aInputMimeType, TBool aInOutMime);

        /**
        * Check picture size if valid
        * @param 
        * @return void
        */
        TBool IsValid(TSize& aSize);

        /**
        * Handles initialize complete call
        * @param 
        * @return void
        */
        void InitComplete();

        /**
        * Allocate transcoder buffers and other data structures
        * @param 
        * @return void
        */
        void AllocateBuffersL();

        /**
        * Makes a new request
        * @param 
        * @return void
        */
        void DoRequest();
        
        /**
        * Loads codec Uids from central repository
        * @return void
        */
        void LoadCodecUids();

    private:
        // Transcoder Observer
        MTRTranscoderObserver& iObserver;

        // Video decoder client
        CTRVideoDecoderClient* iVideoDecoderClient;

        // Video Encoder client
        CTRVideoEncoderClient* iVideoEncoderClient;

        // Scaler
        CTRScaler* iScaler;

        // Processing mode (RT / Non-RT)
        TBool iRealTime;

        // MediaSink for output transcoded stream
        MCMRMediaSink* iMediaSink;

        // MTRVideoPictureSink
        MTRVideoPictureSink* iPictureSink;

        // MTRVideoPictureSink temp sink
        MTRVideoPictureSink* iPictureSinkTemp;

        // Enable / disable encoder flag (relevant only for full transcoding use cases)
        TBool iEncoderEnabled;

        // Input picture size
        TSize iInputPictureSize;
        
        // Picture size after decoding
        TSize iDecodedPictureSize;

        // Output picture size
        TSize iOutputPictureSize;

        // Intermediate picture size
        TSize iIntermediatePictureSize;

        // Operational mode
        TInt iMode;

        // Transcoder state
        TInt iState;

        // Encoder init status
        TInt iEncoderInitStatus;

        // Decoder init status
        TInt iDecoderInitStatus;

        // Codec level
        TInt iInputCodecLevel;
        
        // Codec level
        TInt iOutputCodecLevel;

        // Request status
        TRequestStatus* iStat;

        // Encoder stream end status
        TBool iEncoderStreamEnd;

        // Decoder stream end status
        TBool iDecoderStreamEnd;

        // Input mimetype
        TBufC8<256> iInputMimeType;
        
        // Short mimetype
        TBufC8<256> iInputShortMimeType;

        // Output mimetype
        TBufC8<256> iOutputMimeType;
        
        // Short mimetype
        TBufC8<256> iOutputShortMimeType;

        // Max bit rate
        TUint iOutputMaxBitRate;

        // Input codec
        TInt iInputCodec;

        // Output codec
        TInt iOutputCodec;

        // Input Max Bit Rate
        TUint iInputMaxBitRate;

        // Input max picture size
        TSize iInputMaxPictureSize;

        // Output max picture size
        TSize iOutputMaxPictureSize;

        // Transcoder picture queue
        TDblQue<TVideoPicture> iTranscoderPictureQueue;

        // Transcoder picture queue
        TDblQue<TVideoPicture> iEncoderPictureQueue;

        // CI picture buffers queue
        TDblQue<TVideoPicture> iCIPictureBuffersQueue;
        
        // Transcoder events queue
        TDblQue<CTREventItem> iTranscoderEventQueue;

        // Transcoder events queue
        TDblQue<CTREventItem> iTranscoderEventSrc;

        // Transcoder async events queue
        TDblQue<CTREventItem> iTranscoderAsyncEventQueue;

        // Transcoder TRVideoPicture queue
        TDblQue<TTRVideoPicture> iTranscoderTRPictureQueue;

        // TVideopicture container wait queue
        TDblQue<TVideoPicture> iContainerWaitQueue;
        
        // Data array
        TUint8** iDataArray;
        
        // Events
        CTREventItem* iEvents;
        
        // TVideo picture array
        TVideoPicture* iVideoPictureArray;
        
        // TTRVideo picture array
        TTRVideoPicture* iTRVideoPictureArray;
        
        // Decoded picture
        TVideoPicture* iDecodedPicture;
        
        // Picture container to client
        TTRVideoPicture iPicureToClient;
        
        // Temp video picture
        TVideoPicture* iVideoPictureTemp;
        
        // Set bitrate flag
        TBool iBitRateSetting;
        
        // Fatal error
        TInt iFatalError;

        // Data transfer optimization (applicable only to full transcoding mode)
        TBool iOptimizedDataTransfer;
        
        // Set random access point flag
        TBool iSetRandomAccessPoint;
        
        // EnableEncoder setting status flag
        TBool iEncoderEnabledSettingChanged;
        
        // EnableEncoder setting from the client
        TBool iEncoderEnableClientSetting;
        
        // EnablePicture sink transcoder setting
        TBool iPictureSinkEnabled;
        
        // EnablePictureSink setting status flag
        TBool iPictureSinkSettingChanged;
        
        // PictuereSink enabled client's setting
        TBool iPictureSinkClientSetting;
        
        // Wait picture from client
        TTRVideoPicture* iWaitPictureFromClient;
        
        // Wait new decoded picture
        TVideoPicture* iWaitNewDecodedPicture;
        
        // New event
        CTREventItem* iNewEvent;
        
        // Acync event
        CTREventItem* iAsyncEvent;
        
        // Current setting
        TBool iCurrentPictureSinkEnabled;
        
        // Current setting 
        TBool iCurrentEncoderEnabled;
        
        // Current setting
        TBool iCurrentRandomAccess;
        
        // Current async PS setting
        TBool iCurrentAsyncPictureSinkEnabled;
        
        // Current async enc setting
        TBool iCurrentAsyncEncoderEnabled;

        // Current async setting
        TBool iCurrentAsyncRandomAccess;
        
        // Asynchronous stop is used
        TBool iAsyncStop;
        
        // Max number of frames sent to transcoder at a time
        // This is just a recommendation, not a hard limit
        TInt iMaxFramesInProcessing;
        
        // Default decoder uids for different codecs
        TInt iH263DecoderUid;
        TInt iH264DecoderUid;
        TInt iMPEG4DecoderUid;
        
        // Default encoder uids for different codecs
        TInt iH263EncoderUid;
        TInt iH264EncoderUid;
        TInt iMPEG4EncoderUid;
        
        // Decoder uids for low resolutions
        TInt iH263DecoderLowResUid;
        TInt iH264DecoderLowResUid;
        TInt iMPEG4DecoderLowResUid;
        
        // Max width of low resolutions
        TInt iH263DecoderLowResThreshold;
        TInt iH264DecoderLowResThreshold;
        TInt iMPEG4DecoderLowResThreshold;
        
        // Decoder uids for low resolutions
        TInt iH263EncoderLowResUid;
        TInt iH264EncoderLowResUid;
        TInt iMPEG4EncoderLowResUid;
        
        // Max width of low resolutions
        TInt iH263EncoderLowResThreshold;
        TInt iH264EncoderLowResThreshold;
        TInt iMPEG4EncoderLowResThreshold;
        
        // Flag to store resource loss source (decoder / encoder)
        TBool iDecoderResourceLost;
        
    };


#endif    // CTRTRANSCODERIMP_H
