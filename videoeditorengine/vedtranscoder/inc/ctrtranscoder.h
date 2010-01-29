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


#ifndef CTRTRANSCODER_H
#define CTRTRANSCODER_H


// INCLUDES
#include <e32base.h>
#include <e32cmn.h>
#include <BITDEV.H>                 // for CFbsScreenDevice
#include <CCMRMediaSink.h>
#include "ctrcommon.h"


// FORWARD DECLARATIONS
class MTRVideoPictureSink;
class MTRTranscoderObserver;
class MCMRMediaSink;


// CLASS DECLARATION
/**
*  CTranscoder interface class
*  Using CTRTranscoder: 
*  - Create the transcoder instance 
*  - Open the transcoder with specified options
*  - Give settings
*  - Initialize the transcoder with given options
*  - Start transcoding after initializing is completed with MtroInitializeComplete observer callback.
*  - Stop transcoding
*  - Delete the transcoder instance
*  @lib TRANSCODER.LIB
*  @since 3.1
*/
class CTRTranscoder : public CBase
    {
    public:
        
        // Video data format
        enum TTRVideoDataType
            {
            ETRDuCodedPicture = 0,  // Each data unit contains one coded video picture
            ETRDuVideoSegment,  // Each data unit contains one coded video segment (according resync value (MPEG4) and GOB header in H.263)
            ETRYuvRawData420,   // Each data unit represents uncompressed video picture in YUV 4:2:0 format
            ETRYuvRawData422    // Each data unit represents uncompressed video picture in YUV 4:2:2 format
            };

        // Transcoder operational mode
        enum TTROperationalMode
            {
            EFullTranscoding = 0,   // Full transcoding operation
            EDecoding,          // Decoding only (picture resampling is possible)
            EEncoding,          // Encoding only (picture resampling is possible)
            EResampling,        // Picture resampling only (for YUV In / Out data)
            EOperationNone      // None
            };

    public: // Constructors and destructor
        /**
        * Two-phased constructor. Create an instance the transcoder instance
        * @param aObserver Transcoder observer
        * @return CTranscoder instance. Can leave with Symbian error code
        */
        IMPORT_C static CTRTranscoder* NewL(MTRTranscoderObserver& aObserver);

        /**
        * Destructor.
        */
        virtual ~CTRTranscoder();


    public:
        /**
        * Open the transcoder. Instantiates other submodules based on the selected operational mode. 
        * @param aMediaSink MCMRMediaSink for encoded data output
        * @param aMode Mode for the transcoder, operation to perform
        * @param aMimeTypeInputFormat  Mime type for input format
        * @param aMimeTypeOutputFormat Mime type for otput format
        * @param aRealTimeProcessing Specifies how to process data. ETrue - Real-time processing; EFalse - non-real time. 
        * If the processing cannot be performed in real-time, the transcoder process data as fast as it can. 
        * @return void. Can leave (KErrNotSupported - selected operation or codecs are not supported, 
        * @ or any other Symbian error code )
        */
        virtual void OpenL( MCMRMediaSink* aMediaSink, 
                            CTRTranscoder::TTROperationalMode aMode, 
                            const TDesC8& aInputMimeType, 
                            const TDesC8& aOutputMimeType, 
                            const TTRVideoFormat& aVideoInputFormat, 
                            const TTRVideoFormat& aVideoOutputFormat, 
                            TBool aRealTime ) = 0;
        
        // Information methods
        /**
        * 
        * @param
        * @return
        */
        virtual TBool SupportsInputVideoFormat(const TDesC8& aMimeType) = 0;

        /**
        * 
        * @param
        * @return
        */
        virtual TBool SupportsOutputVideoFormat(const TDesC8& aMimeType) = 0;

        /**
        * Initializes the transcoder. Initializing completes asynchronously with MtroInitializeComplete observer callback.
        * @param none
        * @return void
        */
        virtual void InitializeL() = 0;

        /**
        * Starts transcoding. 
        * @param none
        * @return void. Can leave with Symbian error code in case of an error. 
        */
        virtual void StartL() = 0;

        /**
        * Stops transcoding synchronously. Use this method to stop data processing synchronously. In this case 
        * all active requests are cancelled and data that was not processed is returned to data owner immediately. 
        * @param none
        * @return void. Can leave with Symbian error code in case of an error. 
        */
        virtual void StopL() = 0;

        /**
        * Stops transcoding asynchronously. The transcoder use this signal to ensure that the remaining data gets processed, 
        * without waiting for new data. This method is mainly useful for file-to-file conversions and other non-realtime 
        * processing. 
        * @param none
        * @return void. Can leave with Symbian error code in case of an error. 
        */
        virtual void AsyncStopL() = 0;

        /**
        * Pauses transcoding.
        * @param 
        * @return void. Can leave with Symbian error code in case of an error. 
        */
        virtual void PauseL() = 0;

        /**
        * Resumes transcoding. 
        * @param 
        * @return void. Can leave with Symbian error code in case of an error. 
        */
        virtual void Resume() = 0;


        //  AA
        virtual void SetDecoderInitDataL(TPtrC8& aInitData) = 0;

        /**
        * Sets bit rate parameter for transcoded bitstream. Can be run-time setting
        * @param aBitRate Bitrate
        * @return void Can leave with KErrNotSupported
        */
        virtual void SetVideoBitRateL(TUint aBitRate) = 0;

        /**
        * Sets frame rate for decoded bitstream. Can be run-time setting. 
        * @param aFrameRate Frame rate
        * @return void Can leave with KErrNotSupported
        */
        virtual void SetFrameRateL(TReal aFrameRate) = 0;

        /**
        * Sets channel bit error rate. Can be run-time setting
        * @param aErrorRate Channel expected error rate.
        *        aErrorRate = 0.0     - low bit error rate or error is not expected
        *        aErrorRate = 0.00001 - medium bit error rate
        *        aErrorRate = 0.0001  - high bit error rate
        * @return void
        */
        virtual void SetChannelBitErrorRateL(TReal aErrorRate) = 0;

        /**
        * Sets misc video coding options
        * @param aOptions Coding options
        * @return void
        */
        virtual void SetVideoCodingOptionsL(TTRVideoCodingOptions& aOptions) = 0;

        /**
        * Makes a request to retrieve the intermediate content from the transcoder
        * @param aRetrieve: ETrue - retrieve; EFalse - don't retrieve.
        * @param aSize Requested picture size for intermediate decoded picture
        * @param aSink 
        * @return void Can leave with KErrNotSupported, if the intermediate picture size is not supported
        */
        //virtual void RetrieveIntermediateContentL(TSize& aSize, MTRVideoPictureSink* aSink) = 0;
        virtual void SetVideoPictureSinkOptionsL(TSize& aSize, MTRVideoPictureSink* aSink) = 0;

        /**
        * Makes a request to use direct screen access
        * @param aUseDSA Use Direct Screen Access (DSA). ETrue - use DSA; EFalse - don't use DSA. 
        * @param aOptions Display options, valid only if aUseDSA is set to ETrue
        * @return void Can leave with KErrNotSupported, the DSA is not supported. 
        */
        virtual void UseDirectScreenAccessL(TBool aUseDSA, CFbsScreenDevice& aScreenDevice, TTRDisplayOptions& aOptions) = 0;

        /**
        * Gets the current output bit rate for transcoded bit stream
        * @param none
        * @return TUint Bit rate
        */
        virtual TUint GetVideoBitRateL() = 0;

        /**
        * Gets the current frame rate for transcoded stream
        * @param none
        * @return TReal Framerate
        */
        virtual TReal GetFrameRateL() = 0;

        /**
        * Retrieve currently used codec for transcoding stream
        * @param aVideoMimeType video mime type
        * @return void
        */
        virtual void GetVideoCodecL(TDes8& aVideoMimeType) = 0;

        /**
        * Get transcoded picture size
        * @param aSize Picture size
        * @return void
        */
        virtual void GetTranscodedPictureSizeL(TSize& aSize) = 0;

        // Data transfer methods
        /**
        * Sends filled buffer with new portion of the bitstream to transcoder. Note, this is asynchronous operation, 
        * and this bufer is returned back with Transcoder Observer call MtroReturnCodedBuffer(aBuffer); 
        * @param aBuffer Media buffer. It is not allowed to use this buffer on the client side until it is returned back 
        * from the transcoder with MtroReturnCodedBuffer(aBuffer).
        * @return void 
        */
        virtual void WriteCodedBufferL(CCMRMediaBuffer* aBuffer) = 0;

        /**
        * Sends video picture to transcoder / releases decoded picture (Decode mode)
        * @param aPicture Video picture
        * @return void
        */
        virtual void SendPictureToTranscoderL(TTRVideoPicture* aPicture) = 0;

        /**
        * Resample picture in Resampling only mode. The client should specify souce picture and buffer for target 
        * resampled picture
        * @param aSrc Source picture
        * @param aTrg Target picture
        * @return void
        */
        virtual void ResampleL(TPtr8& aSrc, TPtr8& aTrg) = 0;
        
        
        // Video Editor specific API:
        /**
        * Gets coding-standard specific initialization output. The buffer is pushed to the cleanup stack, 
        * and the caller is responsible for deallocating it. 
        * @param none
        * @return HBufC8 Coding options (VOL header)
        */
        virtual HBufC8* GetCodingStandardSpecificInitOutputLC() = 0;
        
        /**
        * Enables / Disables use of picture sink; Note: This method is relevant only for full transcoding use cases;
        * This cnage is applied for the next written coded buffer with WriteCodedBufferL
        * @param aEnable ETrue: Picture sink is enabled; EFalse: Picture sink is disabled
        * @return none
        */
        virtual void EnablePictureSink(TBool aEnable) = 0;
    
        /**
        * Enable / Disable use of Encoder; Note: This method is relevant only for full transcoding use cases;
        * By default encoding is always enabled in FullTranscoding mode
        * This cnage is applied for the next written coded buffer with WriteCodedBufferL
        * @param aEnable ETue: Encoder is enabled; EFalse: Encoder is disabled
        * @return none
        */
        virtual void EnableEncoder(TBool aEnable) = 0;

        /**
        * Requests random access point to the following transcoded picture
        * This change is applied for the next written coded buffer with WriteCodedBufferL
        * @param none
        * @return none
        */
        virtual void SetRandomAccessPoint() = 0;
        
        /**
        * Returns a time estimate of how long it takes to process one second of the input video with the
        * current transcoder settings. OpenL must be called before calling this function. Estimate
        * will be more accurate if InitializeL is called before calling this function.
        * @param aInput Input video format
        * @param aOutput Output video format
        * @return TReal Time estimate in seconds
        */
        virtual TReal EstimateTranscodeTimeFactorL(const TTRVideoFormat& aInput, const TTRVideoFormat& aOutput) = 0;
        
        /**
        * Get max number of frames in processing.
        * This is just a recommendation, not a hard limit.
        * @return TInt Number of frames
        */
        virtual TInt GetMaxFramesInProcessing() = 0;
        
        /**
        * Enable / Disable pausing of transcoding if resources are lost. If enabled transcoder
        * will notify the observer about resource losses by calling MtroSuspend / MtroResume
        * @param aEnable ETrue: Pausing is enabled
        *                EFalse: Pausing is disabled and resource losses will cause a fatal error
        * @return none
        */
        virtual void EnablePausing(TBool aEnable) = 0;
    };




#endif
