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



#ifndef CTRVIDEODECODERCLIENT_H
#define CTRVIDEODECODERCLIENT_H

// INCLUDES
#include <e32base.h>
#include <devvideoplay.h>
#include <devvideobase.h>
#include <devvideostandardcustominterfaces.h>

#include "ctrcommon.h"


// FORWARD DECLARATIONS
class MTRDevVideoClientObserver;
class CCMRMediaBuffer;


/**
*  Video decoder client
*  @lib TRANSCODER.LIB
*/
NONSHARABLE_CLASS(CTRVideoDecoderClient) : public CBase, public MMMFDevVideoPlayObserver, public MMmfVideoResourceObserver

    {
    public: // Constuctor / destructor

        /**
        * Two-phased constructor.
        */
        static CTRVideoDecoderClient* NewL(MTRDevVideoClientObserver& aObserver);

        /**
        * Destructor.
        */
        ~CTRVideoDecoderClient();

        // Information methods
        /**
        * Checks codec support by MIME type
        * @param aFormat MIME type
        * @param aShortFormat MIME type (short version)
        * @param aUid Uid of the codec to check
        * @param aFallbackUid Fallback Uid incase the first one is not found
        * @return TBool value: ETrue - givemn MIME type is supported; EFalse - no support; 
        */
        TBool SupportsCodec(const TDesC8& aFormat, const TDesC8& aShortFormat, TInt aUid, TInt aFallbackUid);

        /**
        * Gets codec info
        * @param none
        * @return none
        */
        void GetCodecInfoL();

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
        * From MMMFDevVideoPlayObserver. Notifies the client that one or more new empty input buffers are available
        * @param none
        * @return none
        */
        void MdvpoNewBuffers();

        /**
        * From MMMFDevVideoPlayObserver. Returns a used input video picture back to the caller
        * @param aPicture Video picture
        * @return none
        */
        void MdvpoReturnPicture(TVideoPicture *aPicture);

        /**
        * From MMMFDevVideoPlayObserver. Delivers supplemental information from a coded data unit
        * @param aData Data, aTimestamp TimeStamp info, aPictureId Picture Id
        * @return none
        */
        void MdvpoSupplementalInformation(const TDesC8 &aData, const TTimeIntervalMicroSeconds &aTimestamp, 
                                          const TPictureId &aPictureId);

        /**
        * From MMMFDevVideoPlayObserver. Back channel information from the decoder, indicating a picture loss 
        * without specifying the lost picture
        * @param none
        * @return none
        */
        void MdvpoPictureLoss();

        /**
        * From MMMFDevVideoPlayObserver. Back channel information from the decoder, indicating the pictures that have been lost
        * @param aPictures Array with picture Ids
        * @return none
        */
        void MdvpoPictureLoss(const TArray< TPictureId > &aPictures);

        /**
        * From MMMFDevVideoPlayObserver. Back channel information from the decoder, indicating the loss of consecutive 
        * macroblocks in raster scan order
        * @param aFirstMacroblock First macroblock, aNumMacroblocks, Number of macroblocks, aPicture PictureId
        * @return none
        */
        void MdvpoSliceLoss(TUint aFirstMacroblock, TUint aNumMacroblocks, const TPictureId &aPicture);

        /**
        * From MMMFDevVideoPlayObserver. Back channel information from the decoder, indicating a reference picture 
        * selection request
        * @param aSelectionData Selection data
        * @return none
        */
        void MdvpoReferencePictureSelection(const TDesC8 &aSelectionData);

        /**
        * From MMMFDevVideoPlayObserver. Called when a timed snapshot request has been completed
        * @param aError Error, aPictureData Picture data, aPresentationTimestamp Presentation timestamp, aPictureId picture Id
        * @return
        */
        void MdvpoTimedSnapshotComplete(TInt aError, TPictureData *aPictureData, 
                                        const TTimeIntervalMicroSeconds &aPresentationTimestamp, const TPictureId &aPictureId);

        /**
        * From MMMFDevVideoPlayObserver. Notifies the client that one or more new output pictures are available
        * @param none
        * @return none
        */
        void MdvpoNewPictures();

        /**
        * From MMMFDevVideoPlayObserver. Reports a fatal decoding or playback error
        * @param aError Rin-time error
        * @return none
        */
        void MdvpoFatalError(TInt aError);

        /**
        * From MMMFDevVideoPlayObserver. Reports that DevVideoPlay initialization has completed
        * @param aError Init error status
        * @return none
        */
        void MdvpoInitComplete(TInt aError);

        /**
        * From MMMFDevVideoPlayObserver. Reports that the input video stream end has been reached and all pictures 
        * have been processed
        * @param none
        * @return none
        */
        void MdvpoStreamEnd();

        /**
        * Returns used videopicture
        * @param aPicture Video picture
        * @return none
        */
        void ReturnPicture(TVideoPicture* aPicture);

        /**
        * Starts decoding
        * @param none
        * @return none
        */
        void StartL();

        /**
        * Stops decoding synchronously
        * @param none
        * @return none
        */
        void StopL();

        /**
        * Stops decoding asynchronously
        * @param none
        * @return none
        */
        void AsyncStopL();

        /**
        * Pauses decoding
        * @param none
        * @return none
        */
        void Pause();
        
        /**
        * Resumes decoding
        * @param none
        * @return none
        */
        void ResumeL();
        
        /**
        * Checks codec info
        * @param aUid Decoder Uid
        * @return TBool ETrue - accelerated codec; EFalse - non-accelerated codec
        */
        TBool CheckCodecInfoL(TUid aUid);

        /**
        * Initialize decoder client
        * @param none
        * @return none
        */
        void InitializeL();

        /**
        * Select decoder
        * @param none
        * @return none
        */
        void SelectDecoderL();

        /**
        * Send buffer
        * @param aBuffer Media buffer
        * @return none
        */
        void SendBufferL(CCMRMediaBuffer* aBuffer);

        /**
        * Write coded buffer
        * @param aBuffer Media buffer
        * @return none
        */
        void WriteCodedBufferL(CCMRMediaBuffer* aBuffer);

        /**
        * Returns a time estimate on long it takes to decode one frame with the current settings
        * @param aInput Input video format
        * @param aCodecType EH263 or EMpeg4
        * @return TReal time estimate in seconds
        */
        TReal EstimateDecodeFrameTimeL(const TTRVideoFormat& aInput, TInt aCodecType);
        
        /**
        * Checks if decoder supports scaling and enables scaling if supported.
        * Disables scaling if aInputSize is equal to aOutputSize or scaling is not supported.
        * @param aInputSize Source picture size
        * @param aOutputSize Decoded picture size
        * @return TBool ETrue if scaling is supported, EFalse otherwise
        */
        TBool SetDecoderScaling(TSize& aInputSize, TSize& aOutputSize);
        
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
        CTRVideoDecoderClient(MTRDevVideoClientObserver& aObserver);

        /**
        * Symbian 2nd phase constructor, can leave
        */
        void ConstructL();

    private:
        // Observer
        MTRDevVideoClientObserver& iObserver;

        // DevVideoPlay
        CMMFDevVideoPlay* iDevVideoPlay;

        // Decoder mime type
        TBuf8<256> iMimeType;
        
        // Short version mime type
        TBuf8<256> iShortMimeType;

        // Uncompressed format
        TUncompressedVideoFormat iUncompressedFormat;

        // Compressed format
        CCompressedVideoFormat* iCompresedFormat;

        // HwDevice Uid
        THwDeviceId iHwDeviceId;

        // Video coded input buffer
        TVideoInputBuffer* iInputBuffer;

        // Decoded picture
        TVideoPicture* iDecodedPicture;

        // Uid of the selected codec
        TUid iUid;
        
        // Fallback codec to use if the first one doesn't work
        TUid iFallbackUid;

        // Coded buffer
        CCMRMediaBuffer* iCodedBuffer;
        
        // Codec type
        TInt iCodecType;

        // Codec level
        TInt iCodecLevel;

        // Buffer options
        CMMFDevVideoPlay::TBufferOptions iBufferOptions;

        // Fatal error code
        TInt iFatalError;
        
        // Input format
        TTRVideoFormat iInputFormat;

        // Output format
        TTRVideoFormat iOutputFormat;
        
        // Data unit type
        TVideoDataUnitType iDataUnitType;
        
        // Stop
        TBool iStop;
        
        // Pause
        TBool iPause;
        
        // Last ts
        TTimeIntervalMicroSeconds iLastTimestamp;
        
        // If selected decoder is accelerated or not
        TBool iAcceleratedCodecSelected;
        
        // If scaling is used or not
        TBool iScalingInUse;
        
        // Decoded picture size if scaling is used
        TSize iScaledOutputSize;
        
        // If deblocking should be used when scaling
        TBool iScalingWithDeblocking;

        // Video resource handler custom interface
        MMmfVideoResourceHandler* iVideoResourceHandlerCI;
    };



#endif  // CTRVIDEODECODERCLIENT_H
