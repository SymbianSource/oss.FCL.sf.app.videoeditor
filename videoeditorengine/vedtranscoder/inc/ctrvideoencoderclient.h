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
* Video decoder client.
*
*/



#ifndef CTRVIDEOENCODERCLIENT_H
#define CTRVIDEOENCODERCLIENT_H


// INCLUDES
#include <e32base.h>
#include <devvideobase.h>   // Common data types
#include <CCMRMediaSink.h>  // CmmfBuffer
#include <e32cmn.h>         // TRequestStatus
#include <devvideorecord.h>

// When the flag SPP_BUFFER_MGMT_CI_HEADER_SYMBIAN is NOT defined,
// Buffer management interface is specified in Symbian header
#ifndef SPP_BUFFER_MGMT_CI_HEADER_SYMBIAN
#include <devvideostandardcustominterfaces.h>
#else
#include <devvideostandardcustominterfaces.h>
#include "buffermanagementci.h"
#endif

#include "ctrcommon.h"
#include "ctrsettings.h"

#ifndef SPP_BUFFER_MGMT_CI_HEADER_SYMBIAN
/** Buffer Management Custom Interface UID */
const TUid KMmfVideoBuffermanagementUid = { 0x10204bea };
#endif

// FORWARD DECLARATIONS
class MTRDevVideoClientObserver;


// CONSTANTS


/**
*  Video encoder client
*  @lib TRANSCODER.LIB
*  @since 3.1
*/
class TUidAndRate
    {
    public:
        // Encoder Uid
        TUid iUid;

        // Max supported framerate
        TReal iMaxRate;
    };

/**
*  Video encoder client
*  @lib TRANSCODER.LIB
*  @since 3.1
*/
NONSHARABLE_CLASS(CTRVideoEncoderClient) : public CBase, public MMMFDevVideoRecordObserver, public MMmfVideoBufferManagementObserver,
                                           public MMmfVideoResourceObserver
    {
    public: // Constuctor / destructor

        /**
        * Two-phased constructor.
        */
        static CTRVideoEncoderClient* NewL(MTRDevVideoClientObserver& aObserver);

        /**
        * Destructor.
        */
        ~CTRVideoEncoderClient();

        // Information methods
        /**
        * Check codec's support by MIME type
        * @param aFormat Codec MIME type
        * @param aShortFormat Codec MIME type (short version)
        * @param aUid Uid of the codec to check
        * @param aFallbackUid Fallback Uid incase the first codec is not found
        * @return TBool: ETrue - supports, EFalse - Does not support
        */
        TBool SupportsCodec(const TDesC8& aFormat, const TDesC8& aShortFormat, TInt aUid, TInt aFallbackUid);

        /**
        * Sets codec parameters
        * @param aCodecType Codec type
        * @param aCodecLevel Codec level
        * @param aInputFormat Input video format
        * @param aOutputFormat Output video format
        * @return none
        */
        void SetCodecParametersL(TInt aCodecType, TInt aCodecLevel, const TTRVideoFormat& aInputFormat, 
                                 const TTRVideoFormat& aOutputFormat);

        /**
        * Sets real-time operation
        * @param aRealTime Real-time operatiopn
        * @return none
        */
        void SetRealTime(TBool aRealTime);

        /**
        * From MMMFDevVideoRecordObserver. Returns a used input video picture back to the caller. 
        * The picture memory can be re-used or freed
        * @param aPicture Video Picture
        * @return none
        */
        void MdvroReturnPicture(TVideoPicture* aPicture);

        /**
        * From MMMFDevVideoRecordObserver. Signals that the supplemental info send request has completed
        * @param none
        * @return none
        */
        void MdvroSupplementalInfoSent();

        /**
        * From MMMFDevVideoRecordObserver. Notifies the client that one or more new output buffers are available
        * @param none
        * @return none
        */
        void MdvroNewBuffers();

        /**
        * From MMMFDevVideoRecordObserver. Reports a fatal encoding or capturing error 
        * @param aError Run-time error
        * @return none
        */
        void MdvroFatalError(TInt aError);

        /**
        * From MMMFDevVideoRecordObserver. Reports that DevVideoRecord initialization has completed
        * @param aError Init error
        * @return none
        */
        void MdvroInitializeComplete(TInt aError);

        /**
        * From MMMFDevVideoRecordObserver. Reports that the input video data end has been reached and all pictures 
        * have been processed
        * @param none
        * @return none
        */
        void MdvroStreamEnd();

        /**
        * Encode picture
        * @param aPicture Video picture to encode
        * @return none
        */
        void EncodePictureL(TVideoPicture* aPicture);

        /**
        * Starts encoding
        * @param none
        * @return none
        */
        void StartL();

        /**
        * Stops encoding synchronously
        * @param none
        * @return none
        */
        void StopL();

        /**
        * Stops encoding asynchronously
        * @param none
        * @return none
        */
        void AsyncStopL();

        /**
        * Sets target bitrate
        * @param aBitRate bitrate
        * @return none
        */
        void SetBitRate(TUint aBitRate);

        /**
        * Sets target frame rate 
        * @param aFrameRate Target frame rate 
        * @return none
        */
        void SetFrameRate(TReal& aFrameRate);

        /**
        * Sets Channel bit-error rate
        * @param aErrorRate error rate
        * @return none
        */
        void SetChannelBitErrorRate(TReal aErrorRate);

        /**
        * Sets Video coding options
        * @param aOptions Coding options
        * @return none
        */
        void SetVideoCodingOptionsL(TTRVideoCodingOptions& aOptions);

        /**
        * Gets current video bitrate
        * @param none
        * @return Video bitrate
        */
        TUint GetVideoBitRateL();

        /**
        * Gets Current frame rate
        * @param none
        * @return Frame rate 
        */
        TReal GetFrameRateL();

        /**
        * Sets Input / source frame rate 
        * @param aFrameRate Input / Source frame rate 
        * @return none
        */
        void SetInputFrameRate(TReal aFrameRate);

        /**
        * Initialize encoder
        * @param none 
        * @return none
        */
        void InitializeL();

        /**
        * Select encoder
        * @param none 
        * @return none 
        */
        void SelectEncoderL();
        
        /**
        * Get Coding standard / specific output (VOL / VOS / VO Header)
        * @param none 
        * @return Coding standard / specific output (VOL / VOS / VO Header) 
        */
        HBufC8* GetCodingStandardSpecificInitOutputLC();

        /**
        * Select encoder
        * @param none 
        * @return none 
        */
        void UseDataTransferOptimizationL();

        /**
        * Informs about new buffers available in DevVideoPlay queue
        * @param none 
        * @return none 
        */
        void MmvbmoNewBuffers();

        /**
        * Release buffers request
        * @param none 
        * @return none 
        */
        void MmvbmoReleaseBuffers();
        
        /**
        * Gets target video picture buffer
        * @param none 
        * @return Video picture
        */
        TVideoPicture* GetTargetVideoPictureL();
        
        /**
        * Sets Random access point
        * @param none 
        * @return none 
        */
        void SetRandomAccessPoint();
        
        /**
        * Returns a time estimate on long it takes to encode one frame with the current settings
        * @param aOutput Output video format
        * @param aCodecType EH263 or EMpeg4
        * @return TReal time estimate in seconds
        */
        TReal EstimateEncodeFrameTimeL(const TTRVideoFormat& aOutput, TInt aCodecType);
        
        /**
        * Pauses encoding
        * @param none
        * @return none
        */
        void Pause();

        /**
        * Resumes encoding
        * @param none
        * @return none
        */
        void Resume();
        
        /**
        * Enable / Disable resource observer
        * @param aEnable ETrue: Observer is enabled, EFalse observer is disabled
        * @return none
        */
        void EnableResourceObserver(TBool aEnable);
        
        /**
        * From MMmfVideoResourceObserver. Indicates that a media device has lost its resources
        * @param aMediaDevice UID for the media device that lost resources
        * @return none
        */
        void MmvroResourcesLost(TUid aMediaDevice);
        
        /**
        * From MMmfVideoResourceObserver. Indicates that a media device has regained its resources
        * @param aMediaDevice UID for the media device that regained resources
        * @return none
        */
        void MmvroResourcesRestored(TUid aMediaDevice);

    private:
        /**
        * C++ default constructor.
        */
        CTRVideoEncoderClient(MTRDevVideoClientObserver& aObserver);

        /**
        * Symbian 2nd phase constructor, can leave
        */
        void ConstructL();

        /**
        * Checks codec info
        * @param aUid Decoder Uid
        * @return TBool ETrue - accelerated codec; EFalse - non-accelerated codec
        */
        TBool CheckCodecInfoL(TUid aUid);
        
    private:
        // Observer
        MTRDevVideoClientObserver& iObserver;

        // DevVideoRecord
        CMMFDevVideoRecord* iDevVideoRecord;

        // Encoder mime type
        TBuf8<256> iMimeType;
        
        // Short version mime type
        TBuf8<256> iShortMimeType;

        // Uid of the selected codec
        TUid iUid;
        
        // Fallback codec to use if the first codec doesn't work
        TUid iFallbackUid;

        // Max frame rate 
        TReal iMaxFrameRate;

        // Uncompressed format
        TUncompressedVideoFormat iUncompressedFormat;

        // Compressed format
        CCompressedVideoFormat* iCompresedFormat;

        // Picture size
        TSize iPictureSize;

        // Rate control options
        TRateControlOptions iRateControlOptions;

        // Coding options
        TTRVideoCodingOptions iCodingOptions;

        // HwDevice Uid
        THwDeviceId iHwDeviceId;

        // MPEG4 VOL header
        TBufC8<256> iVolHeader;

        // Output buffer type
        TInt iBufferType;

        // Output media buffer
        CCMRMediaBuffer* iOutputMediaBuffer;

        // Request status
        TRequestStatus* iStat;

        // Real-time processing
        TBool iRealTime;

        // State
        TInt iState;

        // Src frame rate
        TReal iSrcRate;

        // Output data type
        TInt iOutputDataType;

        // Codec level
        TInt iCodecLevel;

        // Codec type
        TInt iCodecType;

        // Error rate 
        TReal iErrorRate;

        // Fatal error code
        TInt iFatalError;
        
        // Vol data sending status
        TBool iVolHeaderSent;
        
        // Vol header status 
        TBool iRemoveHeader;
        
        // Vol header length
        TUint iVolLength;
        
        // Rate control options
        TRateControlOptions iRateOptions;
        
        // Rate setting
        TBool iBitRateSetting;
        
        // Buffer Management CI
        MMmfVideoBufferManagement* iVideoBufferManagementCI;
        
        // Last ts
        TTimeIntervalMicroSeconds iLastTimestamp;
        
        // If selected encoder is accelerated or not
        TBool iAcceleratedCodecSelected;
        
        // ETrue if random access point was requested
        TBool iSetRandomAccessPoint;
        
        // Number of H.264 SPS/PPS NAL units from encoder
        TInt iNumH264SPSPPS;
        
        // Video resource handler custom interface
        MMmfVideoResourceHandler* iVideoResourceHandlerCI;
    };



#endif  // CTRVIDEOENCODERCLIENT_H
