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
* Transcoder Implementation.
*
*/



// INCLUDE FILES
#include <centralrepository.h>
#include "ctrvideopicturesink.h"
#include "ctrtranscoderobserver.h"
#include "ctrtranscoderimp.h"
#include "ctrsettings.h"
#include "ctrhwsettings.h"
#include "ctrvideodecoderclient.h"
#include "ctrvideoencoderclient.h"
#include "ctrscaler.h"
#include "ctrprivatecrkeys.h"


// MACROS
#define TRASSERT(x) __ASSERT_DEBUG(x, User::Panic(_L("CTRANSCODERIMP"), KErrAbort))


// ============================ MEMBER FUNCTIONS ===============================

// -----------------------------------------------------------------------------
// CTRTranscoderImp::NewL
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CTRTranscoderImp* CTRTranscoderImp::NewL(MTRTranscoderObserver& aObserver)
    {
    PRINT(_L("CTRTranscoderImp::NewL(), In"))

    CTRTranscoderImp* self = new (ELeave) CTRTranscoderImp(aObserver);
    CleanupStack::PushL( reinterpret_cast<CTRTranscoder*>(self) );
    self->ConstructL();
    CleanupStack::Pop();

    PRINT(_L("CTRTranscoderImp::NewL(), Out"))
    return self;
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::CTRTranscoderImp
// C++ default constructor can NOT contain any code, that
// might leave.
// -----------------------------------------------------------------------------
//
CTRTranscoderImp::CTRTranscoderImp(MTRTranscoderObserver& aObserver) :
    CActive(EPriorityStandard), 
    iObserver(aObserver)
    {
    CActiveScheduler::Add(this);
    iVideoDecoderClient = NULL;
    iVideoEncoderClient = NULL;
    iRealTime = EFalse;
    iMediaSink = NULL;
    iScaler = NULL;
    iPictureSink = NULL;
    iPictureSinkTemp = NULL;
    iState = ETRNone;
    iEncoderInitStatus = KTRErrNotReady;    /*Internal error status*/
    iDecoderInitStatus = KTRErrNotReady;
    iEncoderStreamEnd = EFalse;
    iDecoderStreamEnd = EFalse;
    iDataArray = NULL;
    iVideoPictureArray = NULL;
    iDecodedPicture = NULL;
    iBitRateSetting = EFalse;
    iFatalError = KErrNone;
    iOptimizedDataTransfer = EFalse;
    iEncoderEnabled = ETrue;
    iPictureSinkEnabled = EFalse;
    iPictureSinkClientSetting = EFalse;
    iSetRandomAccessPoint = EFalse;
    iEncoderEnabledSettingChanged = EFalse;
    iPictureSinkSettingChanged = EFalse;
    iEncoderEnableClientSetting = ETrue;
    iWaitPictureFromClient = NULL;
    iWaitNewDecodedPicture = NULL;
    iNewEvent = NULL;
    iCurrentPictureSinkEnabled = EFalse;
    iCurrentAsyncPictureSinkEnabled = EFalse;
    iCurrentEncoderEnabled = ETrue;
    iCurrentAsyncEncoderEnabled = ETrue;
    iCurrentRandomAccess = EFalse;
    iAsyncStop = EFalse;
    iMaxFramesInProcessing = KTRMaxFramesInProcessingDefault;


    // Set offset for queues
    iTranscoderPictureQueue.SetOffset( static_cast<TInt>( _FOFF( TVideoPicture, iLink )));
    iEncoderPictureQueue.SetOffset( static_cast<TInt>( _FOFF( TVideoPicture, iLink )));
    iCIPictureBuffersQueue.SetOffset( static_cast<TInt>( _FOFF( TVideoPicture, iLink )));
    iTranscoderTRPictureQueue.SetOffset( static_cast<TInt>( _FOFF( TTRVideoPicture, iLink )));
    iContainerWaitQueue.SetOffset( static_cast<TInt>( _FOFF( TVideoPicture, iLink )));
    iTranscoderEventSrc.SetOffset( static_cast<TInt>( _FOFF( CTREventItem, iLink )));
    iTranscoderEventQueue.SetOffset( static_cast<TInt>( _FOFF( CTREventItem, iLink )));
    iTranscoderAsyncEventQueue.SetOffset( static_cast<TInt>( _FOFF( CTREventItem, iLink )));
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::ConstructL
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::ConstructL()
    {
    PRINT((_L("CTRTranscoderImp::ConstructL(), In")))
    
    // Allocate ivent quque
    iEvents = NULL;
    iEvents = new (ELeave) CTREventItem[KNumberOfEvents];
    
    // Fill event's queue
    for (TInt i = 0; i < KNumberOfEvents; i ++)
        {
        iEvents[i].Reset();
        iTranscoderEventSrc.AddLast( iEvents[i] );
        }
        
    LoadCodecUids();
        
    PRINT((_L("CTRTranscoderImp::ConstructL(), Out")))
    }


// ---------------------------------------------------------
// CTRTranscoderImp::~CTRTranscoderImp()
// Destructor
// ---------------------------------------------------------
//
CTRTranscoderImp::~CTRTranscoderImp()
    {
    PRINT((_L("CTRTranscoderImp::~CTRTranscoderImp(), In")))
    TInt i = 0;
    
    
    if ( iState == ETRRunning )
        {
        TRAPD(status, this->StopL());
        PRINT((_L("CTRTranscoderImp::~CTRTranscoderImp(), StopL status[%d]"), status))
        
        if (status != KErrNone)
            {
            // Nothing to do, since destruction of the app
            }
        }
            
    Cancel();

    if (iVideoDecoderClient)
        {
        delete iVideoDecoderClient;
        iVideoDecoderClient = NULL;
        }

    if (iVideoEncoderClient)
        {
        delete iVideoEncoderClient;
        iVideoEncoderClient = NULL;
        }

    if (iScaler)
        {
        delete iScaler;
        iScaler = NULL;
        }
        
    if (iDataArray)
        {
        for (i = 0; i < KTRPictureBuffersNumber; i ++)
            {
            if ( iDataArray[i] )
                {
                delete[] iDataArray[i];
                iDataArray[i] = NULL;
                }
            }

        delete[] iDataArray;
        iDataArray = NULL;
        }
        
    if ( (iMode == EEncoding) && (iInputPictureSize == iOutputPictureSize) )
        {
        // Clean iRawData ptrs, since the actual memory was allocated by the client
        if (iVideoPictureArray)
            {
            for (i = 0; i < KTRPictureBuffersNumber; i ++)
                {
                if (iVideoPictureArray[i].iData.iRawData)
                    {
                    iVideoPictureArray[i].iData.iRawData = NULL;
                    }
                }
            }
        }
    
    if (iVideoPictureArray)
        {
        for (i = 0; i < KTRPictureBuffersNumber; i ++)
            {
            if (iVideoPictureArray[i].iData.iRawData)
                {
                delete iVideoPictureArray[i].iData.iRawData;
                iVideoPictureArray[i].iData.iRawData = NULL;
                }
            }
            
        delete[] iVideoPictureArray;
        iVideoPictureArray = NULL;
        }

    if (iTRVideoPictureArray)
        {
        delete[] iTRVideoPictureArray;
        iTRVideoPictureArray = NULL;
        }
        
    if (iEvents)
        {
        delete[] iEvents;
        }

    PRINT((_L("CTRTranscoderImp::~CTRTranscoderImp(), Out")))
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::SupportsInputVideoFormat
// Checks whether given input format is supported
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TBool CTRTranscoderImp::SupportsInputVideoFormat(const TDesC8& aMimeType)
    {
    PRINT((_L("CTRTranscoderImp::SupportsInputVideoFormat(), In")))
    TBool supports = EFalse;


    if ( aMimeType == KNullDesC8 )
        {
        PRINT((_L("CTRTranscoderImp::SupportsInputVideoFormat(), Mime type is undefined")))
        return EFalse;
        }
        
    // Check video decoder
    if (!iVideoDecoderClient)
        {
         // Parse MIME
        TRAPD(status, this->ParseMimeTypeL(aMimeType, ETrue));
        
        if (status != KErrNone)
            {
            PRINT((_L("CTRTranscoderImp::SupportsInputVideoFormat(), ParseMimeTypeL[%d]"), status))
            return EFalse;
            }
                
        // Create the decoder client first
        TRAP( status, iVideoDecoderClient = CTRVideoDecoderClient::NewL(*this) );
                
        if (status != KErrNone)
            {
            PRINT((_L("CTRTranscoderImp::SupportsInputVideoFormat(), VideoDecClient status[%d]"), status))
            iVideoDecoderClient = NULL;
            return EFalse;
            }
        }
        
    // Choose the correct codec Uids to use    
    TInt preferredUid = 0;
    TInt fallbackUid = 0;
    TInt resolutionUid = 0;
    
    if (iInputCodec == EH263)
        {
        preferredUid = iH263DecoderUid;
        fallbackUid = KTRFallbackDecoderUidH263;
        
        PRINT((_L("CTRTranscoderImp::SupportsInputVideoFormat() H.263, preferred = 0x%x"), preferredUid))
        PRINT((_L("CTRTranscoderImp::SupportsInputVideoFormat() H.263, fallback = 0x%x"), fallbackUid))
        
        if (iInputPictureSize.iWidth <= iH263DecoderLowResThreshold)
            {
            resolutionUid = iH263DecoderLowResUid;
            
            PRINT((_L("CTRTranscoderImp::SupportsInputVideoFormat() H.263, below, resolutions = 0x%x"), resolutionUid))
            }
        }
    else if (iInputCodec == EH264)
        {
        preferredUid = iH264DecoderUid;
        fallbackUid = KTRFallbackDecoderUidH264;
        
        PRINT((_L("CTRTranscoderImp::SupportsInputVideoFormat() AVC, preferred = 0x%x"), preferredUid))
        PRINT((_L("CTRTranscoderImp::SupportsInputVideoFormat() AVC, fallback = 0x%x"), fallbackUid))
        
        if (iInputPictureSize.iWidth <= iH264DecoderLowResThreshold)
            {
            resolutionUid = iH264DecoderLowResUid;
            PRINT((_L("CTRTranscoderImp::SupportsInputVideoFormat() AVC, below, resolutions = 0x%x"), resolutionUid))
            }
        }
    else
        {
        preferredUid = iMPEG4DecoderUid;
        fallbackUid = KTRFallbackDecoderUidMPEG4;
        
        PRINT((_L("CTRTranscoderImp::SupportsInputVideoFormat() MPEG-4, preferred = 0x%x"), preferredUid))
        PRINT((_L("CTRTranscoderImp::SupportsInputVideoFormat() MPEG-4, fallback = 0x%x"), fallbackUid))
        
        if (iInputPictureSize.iWidth <= iMPEG4DecoderLowResThreshold)
            {
            resolutionUid = iMPEG4DecoderLowResUid;
            PRINT((_L("CTRTranscoderImp::SupportsInputVideoFormat() MPEG-4, below, resolutions = 0x%x"), resolutionUid))
            }
        }
        
    if (resolutionUid != 0)
        {
        preferredUid = resolutionUid;
        }
        
    PRINT((_L("CTRTranscoderImp::SupportsInputVideoFormat() preferred = 0x%x"), preferredUid))    
            
    supports = iVideoDecoderClient->SupportsCodec(iInputMimeType, iInputShortMimeType, preferredUid, fallbackUid);

    PRINT((_L("CTRTranscoderImp::SupportsInputVideoFormat(), supports[%d] Out"), supports))
    return supports;
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::SupportsOutputVideoFormat
// Checks whether given output format is supported
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TBool CTRTranscoderImp::SupportsOutputVideoFormat(const TDesC8& aMimeType)
    {
    PRINT((_L("CTRTranscoderImp::SupportsOutputVideoFormat(), In")))
    TBool supports = EFalse;


    if ( aMimeType == KNullDesC8 )
        {
        PRINT((_L("CTRTranscoderImp::SupportsOutputVideoFormat(), Mime type is undefined")))
        return EFalse;
        }

    // Check video encoder
    if (!iVideoEncoderClient)
        {
        // Parse MIME
        TRAPD(status, this->ParseMimeTypeL(aMimeType, EFalse));
            
        if (status != KErrNone)
            {
            PRINT((_L("CTRTranscoderImp::SupportsInputVideoFormat(), ParseMimeTypeL[%d]"), status))
            return EFalse;
            }

        // Create the encoder client first
        TRAP(status, iVideoEncoderClient = CTRVideoEncoderClient::NewL(*this) );
                
        if (status != KErrNone)
            {
            PRINT((_L("CTRTranscoderImp::SupportsOutputVideoFormat(), VideoEncClient status[%d]"), status))
            iVideoEncoderClient = NULL;
            return EFalse;
            }
        }
        
    // Choose the correct codec Uids to use    
    TInt preferredUid = 0;
    TInt fallbackUid = 0;
    TInt resolutionUid = 0;
    
    if (iOutputCodec == EH263)
        {
        preferredUid = iH263EncoderUid;
        fallbackUid = KTRFallbackEncoderUidH263;
        
        PRINT((_L("CTRTranscoderImp::SupportsOutputVideoFormat() H.263, preferred = 0x%x"), preferredUid))
        PRINT((_L("CTRTranscoderImp::SupportsOutputVideoFormat() H.263, fallback = 0x%x"), fallbackUid))
        
        if (iOutputPictureSize.iWidth <= iH263EncoderLowResThreshold)
            {
            resolutionUid = iH263EncoderLowResUid;
            PRINT((_L("CTRTranscoderImp::SupportsOutputVideoFormat() H.263, below, resolutions = 0x%x"), resolutionUid))
            }
        }
    else if (iOutputCodec == EH264)
        {
        preferredUid = iH264EncoderUid;
        fallbackUid = KTRFallbackEncoderUidH264;
        
        PRINT((_L("CTRTranscoderImp::SupportsOutputVideoFormat() AVC, preferred = 0x%x"), preferredUid))
        PRINT((_L("CTRTranscoderImp::SupportsOutputVideoFormat() AVC, fallback = 0x%x"), fallbackUid))
        
        if (iOutputPictureSize.iWidth <= iH264EncoderLowResThreshold)
            {
            resolutionUid = iH264EncoderLowResUid;
            PRINT((_L("CTRTranscoderImp::SupportsOutputVideoFormat() AVC, below, resolutions = 0x%x"), resolutionUid))
            }
        }
    else
        {
        preferredUid = iMPEG4EncoderUid;
        fallbackUid = KTRFallbackEncoderUidMPEG4;
        
        PRINT((_L("CTRTranscoderImp::SupportsOutputVideoFormat() MPEG-4, preferred = 0x%x"), preferredUid))
        PRINT((_L("CTRTranscoderImp::SupportsOutputVideoFormat() MPEG-4, fallback = 0x%x"), fallbackUid))
        
        if (iOutputPictureSize.iWidth <= iMPEG4EncoderLowResThreshold)
            {
            resolutionUid = iMPEG4EncoderLowResUid;
            PRINT((_L("CTRTranscoderImp::SupportsOutputVideoFormat() MPEG-4, below, resolutions = 0x%x"), resolutionUid))
            }
        }
        
    if (resolutionUid != 0)
        {
        preferredUid = resolutionUid;
        }
        
    PRINT((_L("CTRTranscoderImp::SupportsOutputVideoFormat() preferred = 0x%x"), preferredUid))        
                
    supports = iVideoEncoderClient->SupportsCodec(iOutputMimeType, iOutputShortMimeType, preferredUid, fallbackUid);

    PRINT((_L("CTRTranscoderImp::SupportsOutputVideoFormat(), supports[%d] Out"), supports))
    return supports;
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::LoadCodecUids
// Loads codec Uids from central repository
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//    
void CTRTranscoderImp::LoadCodecUids()
    {
    PRINT((_L("CTRTranscoderImp::LoadCodecUids(), In")))
    
    CRepository* repository = NULL;
    
    TRAPD(error, repository = CRepository::NewL(KCRUidTranscoder));
    
    if (error != KErrNone)
        {
        PRINT((_L("CTRTranscoderImp::LoadCodecUids(), Error: %d, Out"), error))
        return;
        }
    
    if (repository->Get(KTRH263DecoderUid, iH263DecoderUid) != KErrNone)
        {
        iH263DecoderUid = 0;
        }
    if (repository->Get(KTRH264DecoderUid, iH264DecoderUid) != KErrNone)
        {
        iH264DecoderUid = 0;
        }
    if (repository->Get(KTRMPEG4DecoderUid, iMPEG4DecoderUid) != KErrNone)
        {
        iMPEG4DecoderUid = 0;
        }
        
    if (repository->Get(KTRH263EncoderUid, iH263EncoderUid) != KErrNone)
        {
        iH263EncoderUid = 0;
        }
    if (repository->Get(KTRH264EncoderUid, iH264EncoderUid) != KErrNone)
        {
        iH264EncoderUid = 0;
        }
    if (repository->Get(KTRMPEG4EncoderUid, iMPEG4EncoderUid) != KErrNone)
        {
        iMPEG4EncoderUid = 0;
        }
        
    if (repository->Get(KTRH263DecoderLowResUid, iH263DecoderLowResUid) != KErrNone)
        {
        iH263DecoderLowResUid = 0;
        }
    if (repository->Get(KTRH264DecoderLowResUid, iH264DecoderLowResUid) != KErrNone)
        {
        iH264DecoderLowResUid = 0;
        }
    if (repository->Get(KTRMPEG4DecoderLowResUid, iMPEG4DecoderLowResUid) != KErrNone)
        {
        iMPEG4DecoderLowResUid = 0;
        }
        
    if (repository->Get(KTRH263EncoderLowResUid, iH263EncoderLowResUid) != KErrNone)
        {
        iH263EncoderLowResUid = 0;
        }
    if (repository->Get(KTRH264EncoderLowResUid, iH264EncoderLowResUid) != KErrNone)
        {
        iH264EncoderLowResUid = 0;
        }
    if (repository->Get(KTRMPEG4EncoderLowResUid, iMPEG4EncoderLowResUid) != KErrNone)
        {
        iMPEG4EncoderLowResUid = 0;
        }
        
    if (repository->Get(KTRH263DecoderLowResThreshold, iH263DecoderLowResThreshold) != KErrNone)
        {
        iH263DecoderLowResThreshold = 0;
        }
    if (repository->Get(KTRH264DecoderLowResThreshold, iH264DecoderLowResThreshold) != KErrNone)
        {
        iH264DecoderLowResThreshold = 0;
        }
    if (repository->Get(KTRMPEG4DecoderLowResThreshold, iMPEG4DecoderLowResThreshold) != KErrNone)
        {
        iMPEG4DecoderLowResThreshold = 0;
        }
        
    if (repository->Get(KTRH263EncoderLowResThreshold, iH263EncoderLowResThreshold) != KErrNone)
        {
        iH263EncoderLowResThreshold = 0;
        }
    if (repository->Get(KTRH264EncoderLowResThreshold, iH264EncoderLowResThreshold) != KErrNone)
        {
        iH264EncoderLowResThreshold = 0;
        }
    if (repository->Get(KTRMPEG4EncoderLowResThreshold, iMPEG4EncoderLowResThreshold) != KErrNone)
        {
        iMPEG4EncoderLowResThreshold = 0;
        } 
    
    delete repository;
    
    PRINT((_L("CTRTranscoderImp::LoadCodecUids(), Out")))
    }
    
    

// -----------------------------------------------------------------------------
// CTRTranscoderImp::OpenL
// Opens the transcoder
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::OpenL( MCMRMediaSink* aMediaSink, 
                              CTRTranscoder::TTROperationalMode aMode, 
                              const TDesC8& aInputMimeType, 
                              const TDesC8& aOutputMimeType, 
                              const TTRVideoFormat& aVideoInputFormat, 
                              const TTRVideoFormat& aVideoOutputFormat, 
                              TBool aRealTime )
    {
    PRINT((_L("CTRTranscoderImp::OpenL(), In, OperationalMode[%d]"), aMode))
    TBool supports = EFalse;

    if (iState != ETRNone)
        {
        PRINT((_L("CTRTranscoderImp::OpenL(), Transcoder is in wrong state")))
        User::Leave(KErrNotReady);
        }

    // Set picture size
    
    // If decoding then accept all resolutions that are divisible by 16
    if ( (aMode == EDecoding) &&
        ((aVideoInputFormat.iSize.iWidth & 0xf) == 0) &&
        ((aVideoInputFormat.iSize.iHeight & 0xf) == 0) && 
        ((aVideoOutputFormat.iSize.iWidth & 0xf) == 0) &&
        ((aVideoOutputFormat.iSize.iHeight & 0xf) == 0) )
        {
        iOutputPictureSize = aVideoOutputFormat.iSize;
        iInputPictureSize = aVideoInputFormat.iSize;
        }
    else if ( ( this->IsValid( const_cast<TSize&>(aVideoOutputFormat.iSize) ) ) && 
         ( this->IsValid( const_cast<TSize&>(aVideoInputFormat.iSize) ) ) )
        {
        iOutputPictureSize = aVideoOutputFormat.iSize;
        iInputPictureSize = aVideoInputFormat.iSize;
        }
    else
        {
        PRINT((_L("CTRTranscoderImp::OpenL(), picture size is not valid")))
        User::Leave(KErrNotSupported);
        }
        
    // By default the decoded picture is the same size as the input picture
    iDecodedPictureSize = iInputPictureSize;

    // Create Scaler
    if (!iScaler)
        {
        PRINT((_L("CTRTranscoderImp::OpenL(), create scaler")))
        iScaler = CTRScaler::NewL();
        }


    switch(aMode)
        {
        case EFullTranscoding:
            {
            if ( (aInputMimeType != KNullDesC8) && (aOutputMimeType != KNullDesC8) && (aMediaSink != NULL) )
                {
                if ( (aVideoInputFormat.iDataType != CTRTranscoder::ETRDuCodedPicture) && 
                     (aVideoInputFormat.iDataType != CTRTranscoder::ETRDuVideoSegment) ||
                     (aVideoOutputFormat.iDataType != CTRTranscoder::ETRDuCodedPicture) &&
                     (aVideoOutputFormat.iDataType != CTRTranscoder::ETRDuVideoSegment) )
                    {
                    PRINT((_L("CTRTranscoderImp::OpenL(), data format is not supported in selected operational mode")))
                    User::Leave(KErrNotSupported);
                    }

                // Parse mime type and check / define max parameters for requested codec profile-level. 
                this->ParseMimeTypeL(aInputMimeType, ETrue);
                this->ParseMimeTypeL(aOutputMimeType, EFalse);

                if (!iVideoDecoderClient)
                    {
                    iVideoDecoderClient = CTRVideoDecoderClient::NewL(*this);
                    }

                // Check input format
                supports = this->SupportsInputVideoFormat( iInputShortMimeType.Des() );

                if (!supports)
                    {
                    PRINT((_L("CTRTranscoderImp::OpenL(), Input format is not supported by video decoder")))
                    User::Leave(KErrNotSupported);
                    }

                if (!iVideoEncoderClient)
                    {
                    iVideoEncoderClient = CTRVideoEncoderClient::NewL(*this);
                    }

                // Check output format
                supports = this->SupportsOutputVideoFormat( iOutputShortMimeType.Des() );

                if (!supports)
                    {
                    PRINT((_L("CTRTranscoderImp::OpenL(), Output format is not supported by video encoder")))
                    User::Leave(KErrNotSupported);
                    }
                }
            else
                {
                // Inform user about wrong argument
                User::Leave(KErrArgument);
                }

            // Set codec parameters
            iVideoDecoderClient->SetCodecParametersL(iInputCodec, iInputCodecLevel, aVideoInputFormat, aVideoOutputFormat);
            iVideoEncoderClient->SetCodecParametersL(iOutputCodec, iOutputCodecLevel, aVideoInputFormat, aVideoOutputFormat);

            TBool srcWide = iScaler->IsWideAspectRatio(iInputPictureSize);
            TBool dstWide = iScaler->IsWideAspectRatio(iOutputPictureSize);
            
            if (srcWide != dstWide)
                {
                // get intermediate size from scaler
                TSize resolution;
                TSize bb = TSize(0,0);                                
                
                TBool scale = iScaler->GetIntermediateResolution(iInputPictureSize, iOutputPictureSize, 
                                                                 resolution, bb);
            
                if (scale && iVideoDecoderClient->SetDecoderScaling(iInputPictureSize, resolution))
                    {
                    
                    PRINT((_L("CTRTranscoderImp::OpenL(), decoder scaling supported")))
                    
                    //iDecodedPictureSize = iOutputPictureSize;
                    iDecodedPictureSize = resolution;
                    
                    // NOTE: What if decoder init. fails, this would have to be reseted!
                    // Increase the max number of frames in processing since scaling is used
                    iMaxFramesInProcessing = KTRMaxFramesInProcessingScaling;
                    }                 
                }
            
            else if (iVideoDecoderClient->SetDecoderScaling(iInputPictureSize, iOutputPictureSize))
                {
                // Scaling is supported
                iDecodedPictureSize = iOutputPictureSize;
                
                // Increase the max number of frames in processing since scaling is used
                iMaxFramesInProcessing = KTRMaxFramesInProcessingScaling;
                }

            iVideoEncoderClient->SetRealTime(aRealTime);

            break;
            }

        case EDecoding:
            {
            if ( aInputMimeType != KNullDesC8 )
                {
                if ( (aVideoInputFormat.iDataType != CTRTranscoder::ETRDuCodedPicture) && 
                     (aVideoInputFormat.iDataType != CTRTranscoder::ETRDuVideoSegment) ||
                     (aVideoOutputFormat.iDataType != CTRTranscoder::ETRYuvRawData420) &&
                     (aVideoOutputFormat.iDataType != CTRTranscoder::ETRYuvRawData422) )
                    {
                    PRINT((_L("CTRTranscoderImp::OpenL(), data format is not supported in selected operational mode")))
                    User::Leave(KErrNotSupported);
                    }

                // Check mime
                this->ParseMimeTypeL(aInputMimeType, ETrue);

                if (!iVideoDecoderClient)
                    {
                    iVideoDecoderClient = CTRVideoDecoderClient::NewL(*this);
                    }

                // Check input format
                supports = this->SupportsInputVideoFormat( iInputShortMimeType.Des() );

                if (!supports)
                    {
                    PRINT((_L("CTRTranscoderImp::OpenL(), Input format is not supported by video decoder")))
                    User::Leave(KErrNotSupported);
                    }
                }
            else
                {
                // Inform user about wrong argument
                User::Leave(KErrArgument);
                }

            // Set codec information
            iVideoDecoderClient->SetCodecParametersL(iInputCodec, iInputCodecLevel, aVideoInputFormat, aVideoOutputFormat);
            
            // Check if decoder supports scaling
            if (iVideoDecoderClient->SetDecoderScaling(iInputPictureSize, iOutputPictureSize))
                {
                // Scaling is supported
                iDecodedPictureSize = iOutputPictureSize;
                }

            break;
            }

        case EEncoding:
            {
            if ( aMediaSink && (aOutputMimeType != KNullDesC8) )
                {
                if ( (aVideoInputFormat.iDataType != CTRTranscoder::ETRYuvRawData420) && 
                     (aVideoInputFormat.iDataType != CTRTranscoder::ETRYuvRawData422) ||
                     (aVideoOutputFormat.iDataType != CTRTranscoder::ETRDuCodedPicture) &&
                     (aVideoOutputFormat.iDataType != CTRTranscoder::ETRDuVideoSegment) )
                    {
                    PRINT((_L("CTRTranscoderImp::OpenL(), data format is not supported in selected operational mode")))
                    User::Leave(KErrNotSupported);
                    }

                // Check mime
                this->ParseMimeTypeL(aOutputMimeType, EFalse);

                if (!iVideoEncoderClient)
                    {
                    iVideoEncoderClient = CTRVideoEncoderClient::NewL(*this);
                    }

                // Check output format
                supports = this->SupportsOutputVideoFormat( iOutputShortMimeType.Des() );

                if (!supports)
                    {
                    PRINT((_L("CTRTranscoderImp::OpenL(), Output format is not supported by video encoder")))
                    User::Leave(KErrNotSupported);
                    }
                }
            else
                {
                // Inform user about wrong argument
                User::Leave(KErrArgument);
                }

            // Set codec parameters
            iVideoEncoderClient->SetCodecParametersL(iOutputCodec, iOutputCodecLevel, aVideoInputFormat, aVideoOutputFormat);

            break;
            }

        case EResampling:
            {
            if ( (aVideoInputFormat.iDataType != CTRTranscoder::ETRYuvRawData420) ||
                 (aVideoOutputFormat.iDataType != CTRTranscoder::ETRYuvRawData420) )
                {
                PRINT((_L("CTRTranscoderImp::OpenL(), data format is not supported in selected operational mode")))
                User::Leave(KErrNotSupported);
                }

            break;
            }
            
        default:
            {
            // Given option is not supported
            User::Leave(KErrNotSupported);
            }
        }
    
    iMode = aMode;
    iRealTime = aRealTime;
    iMediaSink = aMediaSink;
    iState = ETROpened;
    
    if ( (iMode == EFullTranscoding) || (iMode == EEncoding) )
        {
        // Get frame rate for initial input coded stream
        TReal frameRate = 0.0; 
        iObserver.MtroSetInputFrameRate(frameRate);

        if (frameRate > 0.0)
            {
            iVideoEncoderClient->SetInputFrameRate(frameRate);
            }
        }

    PRINT((_L("CTRTranscoderImp::OpenL(), Out")))
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::SetVideoBitRateL
// Sets video bitrate
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::SetVideoBitRateL(TUint aBitRate)
    {
    PRINT((_L("CTRTranscoderImp::SetVideoBitRateL(), In")))

    if ( (iState != ETROpened) && (iState != ETRInitialized) && (iState != ETRRunning) )
        {
        PRINT((_L("CTRTranscoderImp::SetVideoBitRateL(), Transcoder is in wrong state")))
        User::Leave(KErrNotReady);
        }
        
    if (iVideoEncoderClient)
        {
        iVideoEncoderClient->SetBitRate(aBitRate);
        }

    iBitRateSetting = ETrue;

    PRINT((_L("CTRTranscoderImp::SetVideoBitRateL(), Out")))
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::SetFrameRateL
// Sets frame rate
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::SetFrameRateL(TReal aFrameRate)
    {
    PRINT((_L("CTRTranscoderImp::SetFrameRateL(), In")))

    if ( (iState != ETROpened) && (iState != ETRInitialized) && (iState != ETRRunning) )
        {
        PRINT((_L("CTRTranscoderImp::SetFrameRateL(), Transcoder is in wrong state")))
        User::Leave(KErrNotReady);
        }
        
    if (iVideoEncoderClient)
        {
        iVideoEncoderClient->SetFrameRate(aFrameRate);
        }

    PRINT((_L("CTRTranscoderImp::SetFrameRateL(), Out")))
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::SetChannelBitErrorRateL
// Sets channel bit error rate
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::SetChannelBitErrorRateL(TReal aErrorRate)
    {
    PRINT((_L("CTRTranscoderImp::SetChannelBitErrorRateL(), In")))

    if ( (iState != ETROpened) && (iState != ETRInitialized) && (iState != ETRRunning) )
        {
        PRINT((_L("CTRTranscoderImp::SetChannelBitErrorRateL(), Transcoder is in wrong state")))
        User::Leave(KErrNotReady);
        }
        
    if (iVideoEncoderClient)
        {
        iVideoEncoderClient->SetChannelBitErrorRate(aErrorRate);
        }
        
    PRINT((_L("CTRTranscoderImp::SetChannelBitErrorRateL(), Out")))
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::SetVideoCodingOptionsL
// Sets video coding options
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::SetVideoCodingOptionsL(TTRVideoCodingOptions& aOptions)
    {
    if (iState != ETROpened)
        {
        PRINT((_L("CTRTranscoderImp::SetVideoCodingOptionsL(), Transcoder is in wrong state")))
        User::Leave(KErrNotReady);
        }

    if (iVideoEncoderClient)
        {
        iVideoEncoderClient->SetVideoCodingOptionsL(aOptions);
        }
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::SetVideoPictureSinkOptionsL
// Sets picture sing and options for intermediate format
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::SetVideoPictureSinkOptionsL(TSize& aSize, MTRVideoPictureSink* aSink)
    {
    if (iState == ETRNone)
        {
        PRINT((_L("CTRTranscoderImp::SetVideoPictureSinkOptionsL(), Transcoder is in wrong state")))
        User::Leave(KErrNotReady);
        }

    if (aSink)
        {
        if ((iMode != EDecoding) && (!this->IsValid(aSize)))
            {
            PRINT((_L("CTRTranscoderImp::SetVideoPictureSinkOptionsL(), invalid size")))
            User::Leave(KErrNotSupported);
            }

        switch(iMode)
            {
            case EFullTranscoding:
            case EDecoding:
                {
                if ( aSize == iOutputPictureSize )
                    {
                    iIntermediatePictureSize = aSize;
                    }
                else
                    {
                    // In full transcoding mode (intermediate picture size = outputTarget size) is supported only
                    PRINT((_L("CTRTranscoderImp::SetVideoPictureSinkOptionsL(), Intermediate picture size is not supported")))
                    User::Leave(KErrNotSupported);
                    }
                }
                break;

            case EEncoding:
                {
                if ( (aSize == iInputPictureSize) || (aSize == iOutputPictureSize) )
                    {
                    iIntermediatePictureSize = aSize;
                    }
                else
                    {
                    PRINT((_L("CTRTranscoderImp::SetVideoPictureSinkOptionsL(), invalid intermediate size")))
                    User::Leave(KErrNotSupported);
                    }
                }
                break;

            case EResampling:
                {
                // No need to check size, since it's iOutputPictureSize; 
                iIntermediatePictureSize = iOutputPictureSize;
                }
                break;

            case EOperationNone:
                {
                // Operation is not set yet
                User::Leave(KErrNotReady);
                }
                break;

            default:
                {
                // Operation is not set yet
                User::Leave(KErrNotReady);
                break;
                }
            }

        // Set picture sink
        iPictureSink = aSink;
        iPictureSinkTemp = aSink;
        iPictureSinkEnabled = ETrue;
        iPictureSinkClientSetting = ETrue;
        iCurrentPictureSinkEnabled = ETrue;
        iCurrentAsyncPictureSinkEnabled = ETrue;
        }
    else
        {
        User::Leave(KErrArgument);
        }
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::UseDirectScreenAccessL
// Requests using of DSA
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::UseDirectScreenAccessL(TBool /*aUseDSA*/, CFbsScreenDevice& /*aScreenDevice*/, 
                                              TTRDisplayOptions& /*aOptions*/)
    {
    // Not supported yet
    User::Leave(KErrNotSupported);
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::ParseMimeTypeL
// Parses given MIME type
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::ParseMimeTypeL(const TDesC8& aMimeType, TBool aInOutMime)
    {
    TUint maxBitRate = 0;
    TInt codecType = 0;
    TBuf8<256> shortMimeType;
    TBuf8<256> newMimeType;
    TInt width = 0;
    TUint codecLevel = 0;


    if (aMimeType == KNullDesC8)
        {
        User::Leave(KErrArgument);
        }

    if ( aMimeType.MatchF( _L8("*video/H263-2000*") ) != KErrNotFound )
        {
        // H.263
        codecType = EH263;
        shortMimeType = _L8("video/H263-2000");
        newMimeType = shortMimeType;

        if ( aMimeType.MatchF( _L8("*profile*") ) != KErrNotFound )
            {
            // Profile info is given, check it
            if ( aMimeType.MatchF( _L8("*profile=0*") ) != KErrNotFound )
                {
                // Check level info
                newMimeType += _L8("; profile=0");
                }
            else if ( aMimeType.MatchF( _L8("*profile=3*") ) != KErrNotFound )
                {
                // Check level info
                newMimeType += _L8("; profile=3");
                }
            else
                {
                // We don't support any other profiles yet
                PRINT((_L("CTRTranscoderImp::ParseMimeTypeL(), profile is not supported")))
                User::Leave(KErrNotSupported);
                }
            }
        else
            {
            // Set defaults for profile=0;
            newMimeType += _L8("; profile=0");
            }


        // Check level info
        if ( aMimeType.MatchF( _L8("*level=*") ) != KErrNotFound )
            {
            if ( aMimeType.MatchF( _L8("*level=10*") ) != KErrNotFound )
                {
                // Set level=10;
                maxBitRate = KTRMaxBitRateH263Level10;
                codecLevel = KTRH263CodecLevel10;
                newMimeType += _L8("; level=10");
                }
            else if ( aMimeType.MatchF( _L8("*level=20*") ) != KErrNotFound )
                {
                // Set level=20;
                maxBitRate = KTRMaxBitRateH263Level20;
                codecLevel = KTRH263CodecLevel20;
                newMimeType += _L8("; level=20");
                }
            else if ( aMimeType.MatchF( _L8("*level=30*") ) != KErrNotFound )
                {
                // Set level=30;
                maxBitRate = KTRMaxBitRateH263Level30;
                codecLevel = KTRH263CodecLevel30;
                newMimeType += _L8("; level=30");
                }
            else if ( aMimeType.MatchF( _L8("*level=40*") ) != KErrNotFound )
                {
                // Set level=40;
                maxBitRate = KTRMaxBitRateH263Level40;
                codecLevel = KTRH263CodecLevel40;
                newMimeType += _L8("; level=40");
                }
            else if ( aMimeType.MatchF( _L8("*level=45*") ) != KErrNotFound )
                {
                // Set level=45;
                maxBitRate = KTRMaxBitRateH263Level45;
                codecLevel = KTRH263CodecLevel45;
                newMimeType += _L8("; level=45");
                }
            else if ( aMimeType.MatchF( _L8("*level=50*") ) != KErrNotFound )
                {
                // Set level=50;
                maxBitRate = KTRMaxBitRateH263Level50;
                codecLevel = KTRH263CodecLevel50;
                newMimeType += _L8("; level=50");
                }
            else if ( aMimeType.MatchF( _L8("*level=60*") ) != KErrNotFound )
                {
                // Set level=60;
                maxBitRate = KTRMaxBitRateH263Level60;
                codecLevel = KTRH263CodecLevel60;
                newMimeType += _L8("; level=60");
                }
            else if ( aMimeType.MatchF( _L8("*level=70*") ) != KErrNotFound )
                {
                // Set level=70;
                maxBitRate = KTRMaxBitRateH263Level70;
                codecLevel = KTRH263CodecLevel70;
                newMimeType += _L8("; level=70");
                }
            else
                {
                // We don't support any other levels
                PRINT((_L("CTRTranscoderImp::ParseMimeTypeL(), level is not supported")))
                User::Leave(KErrNotSupported);
                }
            }
        else
            {
            // Codec level is not specified, check requested picture size and set mime
            if (aInOutMime)
                {
                width = iInputPictureSize.iWidth;
                }
            else
                {
                width = iOutputPictureSize.iWidth;
                }

            switch( width )
                {
                case KTRSubQCIFWidth:
                case KTRQCIFWidth:
                    {
                    // Set defaults for level=10;
                    maxBitRate = KTRMaxBitRateH263Level10;
                    codecLevel = KTRH263CodecLevel10;
                    newMimeType += _L8("; level=10");
                    break;
                    }

                case KTRCIFWidth:
                    {
                    // Set defaults for level=30;
                    maxBitRate = KTRMaxBitRateH263Level30;
                    codecLevel = KTRH263CodecLevel30;
                    newMimeType += _L8("; level=30");
                    break;
                    }

                case KTRPALWidth:
                    {
                    // Set defaults for level=60;
                    maxBitRate = KTRMaxBitRateH263Level60;
                    codecLevel = KTRH263CodecLevel60;
                    newMimeType += _L8("; level=60");
                    break;
                    }

                default:
                    {
                    // Set defaults for level=10;
                    maxBitRate = KTRMaxBitRateH263Level10;
                    codecLevel = KTRH263CodecLevel10;
                    newMimeType += _L8("; level=10");
                    break;
                    }
                }
            }
        }
    else if ( (aMimeType.MatchF( _L8("*video/H264*") ) != KErrNotFound) )
        {
        // H.264 (AVC)
        codecType = EH264;
        shortMimeType = _L8("video/H264");
        newMimeType = shortMimeType;
        
        // Check profile-level
        if ( aMimeType.MatchF( _L8("*profile-level-id=*") ) != KErrNotFound )
            {
            if ( aMimeType.MatchF( _L8("*profile-level-id=42800A*") ) != KErrNotFound )
                {
                maxBitRate = KTRMaxBitRateH264Level10;
                codecLevel = KTRH264CodecLevel10;
                newMimeType += _L8("; profile-level-id=42800A");    // Level 1
                }
            else if ( aMimeType.MatchF( _L8("*profile-level-id=42900B*") ) != KErrNotFound )
                {
                maxBitRate = KTRMaxBitRateH264Level10b;
                codecLevel = KTRH264CodecLevel10b;
                newMimeType += _L8("; profile-level-id=42900B");    // Level 1b
                }
            else if ( aMimeType.MatchF( _L8("*profile-level-id=42800B*") ) != KErrNotFound )
                {
                maxBitRate = KTRMaxBitRateH264Level11;
                codecLevel = KTRH264CodecLevel11;
                newMimeType += _L8("; profile-level-id=42800B");    // Level 1.1
                }
            else if ( aMimeType.MatchF( _L8("*profile-level-id=42800C*") ) != KErrNotFound )
                {
                maxBitRate = KTRMaxBitRateH264Level12;
                codecLevel = KTRH264CodecLevel12;
                newMimeType += _L8("; profile-level-id=42800C");    // Level 1.2
                }
            else if ( aMimeType.MatchF( _L8("*profile-level-id=42800D*") ) != KErrNotFound )
                {
                maxBitRate = KTRMaxBitRateH264Level13;
                codecLevel = KTRH264CodecLevel13;
                newMimeType += _L8("; profile-level-id=42800D");    // Level 1.3
                }
            else if ( aMimeType.MatchF( _L8("*profile-level-id=428014*") ) != KErrNotFound )
                {
                maxBitRate = KTRMaxBitRateH264Level20;
                codecLevel = KTRH264CodecLevel20;
                newMimeType += _L8("; profile-level-id=428014");    // Level 2
                }
            //WVGA task
            else if ( aMimeType.MatchF( _L8("*profile-level-id=428015*") ) != KErrNotFound )
                {
                maxBitRate = KTRMaxBitRateH264Level21;
                codecLevel = KTRH264CodecLevel21;
                newMimeType += _L8("; profile-level-id=428015");    // Level 2.1
                }
            else if ( aMimeType.MatchF( _L8("*profile-level-id=428016*") ) != KErrNotFound )
                {
                maxBitRate = KTRMaxBitRateH264Level22;
                codecLevel = KTRH264CodecLevel22;
                newMimeType += _L8("; profile-level-id=428016");    // Level 2.2
                }
            else if ( aMimeType.MatchF( _L8("*profile-level-id=42801E*") ) != KErrNotFound )
                {
                maxBitRate = KTRMaxBitRateH264Level30;
                codecLevel = KTRH264CodecLevel30;
                newMimeType += _L8("; profile-level-id=42801E");    // Level 3
                }
            else if ( aMimeType.MatchF( _L8("*profile-level-id=42801F*") ) != KErrNotFound )
                {
                maxBitRate = KTRMaxBitRateH264Level31;
                codecLevel = KTRH264CodecLevel31;
                newMimeType += _L8("; profile-level-id=42801F");    // Level 3.1
                }
            else
                {
                // We don't support any other levels
                PRINT((_L("CTRTranscoderImp::ParseMimeTypeL(), profile-level-id is not supported")))
                User::Leave(KErrNotSupported);
                }
            }
        else
            {
            // profile-level-id is not specified, check requested picture size
            if (aInOutMime)
                {
                width = iInputPictureSize.iWidth;
                }
            else
                {
                width = iOutputPictureSize.iWidth;
                }
                
            switch( width )
                {
                case KTRSubQCIFWidth:
                case KTRQCIFWidth:
                    {
                    // Set level 1
                    maxBitRate = KTRMaxBitRateH264Level10;
                    codecLevel = KTRH264CodecLevel10;
                    newMimeType += _L8("; profile-level-id=42800A");
                    break;
                    }

                case KTRQVGAWidth:
                case KTRCIFWidth:
                    {
                    // Set level 1.2
                    maxBitRate = KTRMaxBitRateH264Level12;
                    codecLevel = KTRH264CodecLevel12;
                    newMimeType += _L8("; profile-level-id=42800C");
                    break;
                    }
                case KTRWVGAWidth:
                    {
                    // Set level 3.1
                    maxBitRate = KTRMaxBitRateH264Level31;
                    codecLevel = KTRH264CodecLevel31;
                    newMimeType += _L8("; profile-level-id=42801F");
                    break;
                    }

                default:
                    {
                    // Set level 1
                    maxBitRate = KTRMaxBitRateH264Level10;
                    codecLevel = KTRH264CodecLevel10;
                    newMimeType += _L8("; profile-level-id=42800A");
                    break;
                    }
                }
            } 
        }
    else if ( (aMimeType.MatchF( _L8("*video/mp4v-es*") ) != KErrNotFound) || 
              (aMimeType.MatchF( _L8("*video/MP4V-ES*") ) != KErrNotFound) )
        {
        // MPEG-4 Visual
        codecType = EMpeg4;
        shortMimeType = _L8("video/mp4v-es");   // Set short mime
        newMimeType = shortMimeType;

        // Check profile-level
        if ( aMimeType.MatchF( _L8("*profile-level-id=*") ) != KErrNotFound )
            {
            if ( aMimeType.MatchF( _L8("*profile-level-id=8*") ) != KErrNotFound )
                {
                // Set defaults for profile-level-id=8
                newMimeType += _L8("; profile-level-id=8");
                }
            else if( aMimeType.MatchF( _L8("*profile-level-id=1*") ) != KErrNotFound )
                {
                // Set profile-level-id=1
                maxBitRate = KTRMaxBitRateMPEG4Level1;
                codecLevel = KTRMPEG4CodecLevel1;
                newMimeType += _L8("; profile-level-id=1");
                }
            else if ( aMimeType.MatchF( _L8("*profile-level-id=2*") ) != KErrNotFound )
                {
                // Set profile-level-id=2
                maxBitRate = KTRMaxBitRateMPEG4Level2;
                codecLevel = KTRMPEG4CodecLevel2;
                newMimeType += _L8("; profile-level-id=2");
                }
            else if ( aMimeType.MatchF( _L8("*profile-level-id=3*") ) != KErrNotFound )
                {
                // Set profile-level-id=3
                maxBitRate = KTRMaxBitRateMPEG4Level3;
                codecLevel = KTRMPEG4CodecLevel3;
                newMimeType += _L8("; profile-level-id=3");
                }
            else if ( aMimeType.MatchF( _L8("*profile-level-id=9*") ) != KErrNotFound )
                {
                // Set profile-level-id=9
                maxBitRate = KTRMaxBitRateMPEG4Level0b;
                codecLevel = KTRMPEG4CodecLevel0b;
                newMimeType += _L8("; profile-level-id=9");
                }
            else if ( aMimeType.MatchF( _L8("*profile-level-id=4*") ) != KErrNotFound )
                {
                // Set profile-level-id=4
                maxBitRate = KTRMaxBitRateMPEG4Level4a;
                codecLevel = KTRMPEG4CodecLevel4a;
                newMimeType += _L8("; profile-level-id=4");
                }
            else
                {
                // We don't support any other levels
                PRINT((_L("CTRTranscoderImp::ParseMimeTypeL(), profile-level-id is not supported")))
                User::Leave(KErrNotSupported);
                }
            }
        else
            {
            // profile-level-id is not specified, check requested picture size
            // Set defaults for profile-level-id=8
            if (aInOutMime)
                {
                width = iInputPictureSize.iWidth;
                }
            else
                {
                width = iOutputPictureSize.iWidth;
                }

            switch( width )
                {
                case KTRSubQCIFWidth:
                case KTRQCIFWidth:
                    {
                    // Set profile-level-id=0
                    codecLevel = KTRMPEG4CodecLevel0;
                    maxBitRate = KTRMaxBitRateMPEG4Level0;
                    newMimeType += _L8("; profile-level-id=8");
                    break;
                    }

                case KTRQVGAWidth:
                case KTRCIFWidth:
                    {
                    // Set profile-level-id=3
                    maxBitRate = KTRMaxBitRateMPEG4Level3;
                    codecLevel = KTRMPEG4CodecLevel3;
                    newMimeType += _L8("; profile-level-id=3");
                    break;
                    }
                    
                case KTRVGAWidth:
                    {
                    // Set profile-level-id=4 (4a)
                    maxBitRate = KTRMaxBitRateMPEG4Level4a;
                    codecLevel = KTRMPEG4CodecLevel4a;
                    newMimeType += _L8("; profile-level-id=4");
                    break;
                    }

                default:
                    {
                    // Set profile-level-id=0
                    maxBitRate = KTRMaxBitRateMPEG4Level0;
                    codecLevel = KTRMPEG4CodecLevel0;
                    newMimeType += _L8("; profile-level-id=8");
                    break;
                    }
                }
            }
        }
    else
        {
        PRINT((_L("CTRTranscoderImp::ParseMimeL(), there is curently no support for this type")))
        User::Leave(KErrNotSupported);
        }

    if (aInOutMime)
        {
        // Mime type was set for Input format
        iInputCodecLevel = codecLevel;
        iInputCodec = codecType;
        iInputMaxBitRate = maxBitRate;
        
        iInputMimeType = newMimeType;
        iInputShortMimeType = shortMimeType;
        }
    else
        {
        // Mime type was set for Output format
        iOutputCodecLevel = codecLevel;
        iOutputCodec = codecType;
        iOutputMaxBitRate = maxBitRate;
        
        iOutputMimeType = newMimeType;
        iOutputShortMimeType = shortMimeType;
        }
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::IsValid
// Checks whether the size is valid
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TBool CTRTranscoderImp::IsValid(TSize& aSize)
    {
    TBool valid = EFalse;

    switch(aSize.iWidth)
        {
        case KTRSubQCIFWidth:
            {
            if (aSize.iHeight == KTRSubQCIFHeight)
                {
                valid = ETrue;
                }
            break;
            }
            
        case KTRQCIFWidth:
            {
            if (aSize.iHeight == KTRQCIFHeight)
                {
                valid = ETrue;
                }
            break;
            }
            
        case KTRCIFWidth:
            {
            if (aSize.iHeight == KTRCIFHeight)
                {
                valid = ETrue;
                }
            break;
            }
            
        case KTRQVGAWidth:
            {
            if (aSize.iHeight == KTRQVGAHeight)
                {
                valid = ETrue;
                }
            break;
            }
            
        case KTRVGAWidth:
            {
            if (aSize.iHeight == KTRVGAHeight || aSize.iHeight == KTRVGA16By9Height)
                {
                valid = ETrue;
                }
            break;
            }
        case KTRWVGAWidth:
            {
            if (aSize.iHeight == KTRWVGAHeight)
                {
                valid = ETrue;
                }
            break;
            }
            
        case KTR4CIFWidth:
            {
            if (aSize.iHeight == KTR4CIFHeight)
                {
                valid = ETrue;
                }
            break;
            }
            
        case KTRPALWidth:
            {
            if ( (aSize.iHeight == KTRPAL2Height) || (aSize.iHeight == KTRPAL1Height) )
                {
                valid = ETrue;
                }
            break;
            }
        
        default:
            {
            valid = EFalse;
            }
        }

    return valid;
    }



// -----------------------------------------------------------------------------
// CTRTranscoderImp::GetVideoBitRateL
// Gets video bitrate 
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TUint CTRTranscoderImp::GetVideoBitRateL()
    {
    if (iVideoEncoderClient)
        {
        return iVideoEncoderClient->GetVideoBitRateL();
        }
    
    // Otherwise return 0, in case if it's called before initializing or in different operating mode
    // e.g. (without encoding)
    return 0;
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::GetFrameRateL
// Gets frame rate
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TReal CTRTranscoderImp::GetFrameRateL()
    {
    if (iVideoEncoderClient)
        {
        return iVideoEncoderClient->GetFrameRateL();
        }

    // Otherwise return 0, in case if it's called before initializing or in different operating mode
    // e.g. (without encoding)
    return 0;
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::GetVideoCodecL
// Gets current video codec in use
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::GetVideoCodecL(TDes8& aVideoMimeType)
    {
    if (iState == ETRNone)
        {
        aVideoMimeType = KNullDesC8;
        }
    else
        {
        aVideoMimeType = iOutputShortMimeType;
        }
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::GetTranscodedPictureSizeL
// Gets output transcoded picture size
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::GetTranscodedPictureSizeL(TSize& aSize)
    {
    aSize = iOutputPictureSize;
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::InitializeL
// Initializes transcider
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::InitializeL()
    {
    PRINT((_L("CTRTranscoderImp::InitializeL(), In")))
    TInt status = KErrNone;


    if (iState != ETROpened)
        {
        PRINT((_L("CTRTranscoderImp::InitializeL(), Transcoder is in wrong state")))
        User::Leave(KErrNotReady);
        }

    switch(iMode)
        {
        case EFullTranscoding:
            {
            iVideoDecoderClient->InitializeL();
            
            // Check encoder client if it supports optimized data transfer
            TRAP( status, iVideoEncoderClient->UseDataTransferOptimizationL() );
            if (status == KErrNone)
                {
                iOptimizedDataTransfer = ETrue;
                }

            TRAP(status, iVideoEncoderClient->InitializeL());

            if (status != KErrNone)
                {
                iEncoderInitStatus = status;
                InitComplete();
                }

            break;
            }

        case EDecoding:
            {
            iEncoderInitStatus = KErrNone;
            iVideoDecoderClient->InitializeL();
            break;
            }

        case EEncoding:
            {
            iDecoderInitStatus = KErrNone;
            iVideoEncoderClient->InitializeL();
            break;
            }

        case EResampling:
            {
            // Resampler is already created
            iState = ETRInitialized;
            iObserver.MtroInitializeComplete(KErrNone);
            return;
            }

        default:
            {
            // Transcoder is not ready to be initialized
            User::Leave(KErrNotReady);
            break;
            }
        }
    
    PRINT((_L("CTRTranscoderImp::InitializeL(), Out")))
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::MtrdvcoEncInitializeComplete
// Encoder initializing is completed with aError
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::MtrdvcoEncInitializeComplete(TInt aError)
    {
    PRINT((_L("CTRTranscoderImp::MtrdvcoEncInitializeComplete(), status[%d]"), aError))
    
    if (aError == KErrHardwareNotAvailable)
        {
        // Encoder needs to be reinitialized
        
        // Check if encoder client supports optimized data transfer
        iOptimizedDataTransfer = EFalse;
        TRAPD( status, iVideoEncoderClient->UseDataTransferOptimizationL() );
        if (status == KErrNone)
            {
            iOptimizedDataTransfer = ETrue;
            }

        PRINT((_L("CTRTranscoderImp::MtrdvcoEncInitializeComplete(), Trying to reinitialize encoder")))
        TRAP(status, iVideoEncoderClient->InitializeL());

        if (status == KErrNone)
            {
            // Reinitialization done so wait for the next initialize complete call
            return;
            }
        }
    
    iEncoderInitStatus = aError;
    this->InitComplete();
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::MtrdvcoDecInitializeComplete
// Decoder initializing is completed with aError
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::MtrdvcoDecInitializeComplete(TInt aError)
    {
    PRINT((_L("CTRTranscoderImp::MtrdvcoDecInitializeComplete(), status[%d]"), aError))
    
    if (aError == KErrHardwareNotAvailable)
        {
        // Decoder needs to be reinitialized
        
        // Check if decoder supports scaling
        iDecodedPictureSize = iInputPictureSize;
        
        // Set to default
        iMaxFramesInProcessing = KTRMaxFramesInProcessingDefault;
                
        TBool srcWide = iScaler->IsWideAspectRatio(iInputPictureSize);
        TBool dstWide = iScaler->IsWideAspectRatio(iOutputPictureSize);
            
        if (srcWide != dstWide)
            {
            
            PRINT((_L("CTRTranscoderImp::MtrdvcoDecInitializeComplete(), black boxing needed")))
            
            // get intermediate resolution from scaler
            TSize resolution;
            TSize bb = TSize(0,0);                                
            
            TBool scale = iScaler->GetIntermediateResolution(iInputPictureSize, iOutputPictureSize, 
                                                             resolution, bb);
                                                             
            PRINT((_L("CTRTranscoderImp::MtrdvcoDecInitializeComplete(), int. resolution (%d, %d), scale = %d"),
                resolution.iWidth, resolution.iHeight, TInt(scale)))
        
            if (scale && iVideoDecoderClient->SetDecoderScaling(iInputPictureSize, resolution))
                {
                
                PRINT((_L("CTRTranscoderImp::MtrdvcoDecInitializeComplete(), decoder scaling supported")))
                
                iDecodedPictureSize = resolution;
                //iDecodedPictureSize = iOutputPictureSize;
                
                iMaxFramesInProcessing = KTRMaxFramesInProcessingScaling;
                }
            }
        
        else if (iVideoDecoderClient->SetDecoderScaling(iInputPictureSize, iOutputPictureSize))
            {
            // Scaling is supported
            iDecodedPictureSize = iOutputPictureSize;
            
            iMaxFramesInProcessing = KTRMaxFramesInProcessingScaling;
            }

        PRINT((_L("CTRTranscoderImp::MtrdvcoDecInitializeComplete(), Trying to reinitialize decoder")))
        TRAPD(status, iVideoDecoderClient->InitializeL());

        if (status == KErrNone)
            {
            // Reinitialization done so wait for the next initialize complete call
            return;
            }
        }
    
    iDecoderInitStatus = aError;
    this->InitComplete();
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::InitComplete
// Checks and reports init status to the client
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::InitComplete()
    {
    TInt status = KErrNone;
    
    if ( (iEncoderInitStatus != KTRErrNotReady) && (iDecoderInitStatus != KTRErrNotReady) )
        {
        // Both were initialized, check statuses
        if ( (iEncoderInitStatus == KErrNone) && (iDecoderInitStatus == KErrNone) )
            {            
            // Both are ok, 
            // Allocate buffers for intrnal use
            TRAP( status, this->AllocateBuffersL() );

            if (status == KErrNone)
                {
                iState = ETRInitialized;
                }
                
            // Inform the client
            iObserver.MtroInitializeComplete(status);
            }
        else if (iDecoderInitStatus == KErrNone)
            {
            // Report encoder init error
            iObserver.MtroInitializeComplete(iEncoderInitStatus);
            }
        else
            {
            // Report decoder init error
            iObserver.MtroInitializeComplete(iDecoderInitStatus);
            }
        }
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::StartL
// Starts data processing
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::StartL()
    {
    if ( (iState == ETRInitialized) || (iState == ETRStopped) )
        {
        switch(iMode)
            {
            case EFullTranscoding:
                {
                // Reset flag here
                iAsyncStop = EFalse;

                iVideoDecoderClient->StartL();

                if (!iBitRateSetting)
                    {
                    this->SetVideoBitRateL(iOutputMaxBitRate);
                    }

                iVideoEncoderClient->StartL();
                }
                break;
                
            case EDecoding:
                {
                iVideoDecoderClient->StartL();
                }
                break;
                
            case EEncoding:
                {
                if (!iBitRateSetting)
                    {
                    this->SetVideoBitRateL(iOutputMaxBitRate);
                    }

                iVideoEncoderClient->StartL();
                }
                break;
                
            case EResampling:
                {
                // Nothing to do
                }
                break;
            
            default:
                {
                // Should never be reached
                TRASSERT(0);
                }
            }
            
        iState = ETRRunning;
        }
    else
        {
        PRINT((_L("CTRTranscoderImp::StartL(), called in wrong state")))
        User::Leave(KErrNotReady);
        }
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::AllocateBuffersL
// Allocates internal buffers
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::AllocateBuffersL()
    {
    PRINT((_L("CTRTranscoderImp::AllocateBuffersL(), In")))
    TInt i = 0;
    TUint dataBufferSize = 0;
    
    
    // Assume we are using 420 as a middle temporal format
    dataBufferSize = iOutputPictureSize.iWidth * iOutputPictureSize.iHeight * 3 / 2;
    PRINT((_L("CTRTranscoderImp::AllocateBuffersL(), data buffer size[%d]"), dataBufferSize))
    
    // Allocate TVideoPicture containers
    iVideoPictureArray = new (ELeave) TVideoPicture[ KTRPictureBuffersNumber ];
    
    for (i = 0; i < KTRPictureBuffersNumber; i ++)
        {
        // Reset iRawData ptr
        iVideoPictureArray[i].iData.iRawData = NULL;
        }

    switch (iMode)
        {
        case EFullTranscoding:
            {
            // Allocate TTRVideoPicture containers (since the client requested to provide 
            // intermediate trancsoded content )
            iTRVideoPictureArray = new (ELeave)TTRVideoPicture[KTRPictureContainersNumber];
                
            // Add TR picture containers to the queue
            for (i = 0; i < KTRPictureContainersNumber; i ++)
                {
                iTranscoderTRPictureQueue.AddLast( iTRVideoPictureArray[i] );
                }
                
            if (iOptimizedDataTransfer)
                {
                // If optimized data transfer is used, no other allocations required. 
                return;
                }
            }
            break;
            
        case EDecoding:
            {
            if ( iDecodedPictureSize == iOutputPictureSize )
                {
                // Resampling and data allocation is not required
                return;
                }
            }
            break;
            
        case EEncoding:
            {
            if ( iInputPictureSize == iOutputPictureSize )
                {
                // Allocation data buffers for iVideoPictureArray is not needed, TVideoPicture 
                // containers are enough in this case. Initialize the queue
                for (i = 0; i < KTRPictureBuffersNumber; i ++)
                    {
                    iTranscoderPictureQueue.AddLast( iVideoPictureArray[i] );
                    }

                return;
                }
            }           
            break;

        case EResampling:
            {
            // Nothing to do
            return;
            }
            
        default:
            {
            PRINT((_L("CTRTranscoderImp::AllocateBuffersL(), Not ready, mode[%d]"), iMode))
            User::Leave(KErrNotReady);
            }
            break;
        }

        // Common part
        // Allocate data buffers
        iDataArray = new (ELeave) TUint8*[ KTRPictureBuffersNumber ];
            
        for ( i = 0; i < KTRPictureBuffersNumber; i ++ )
            {
            iDataArray[i] = NULL;
            }
            
        for ( i = 0; i < KTRPictureBuffersNumber; i ++ )
            {
            iDataArray[i] = new (ELeave) TUint8[dataBufferSize];
            PRINT((_L("CTRTranscoderImp::AllocateBuffersL(), data buffer[%d], [0x%x], allocated"), i, iDataArray[i]))
            }
                
        // Initialize the queue         
        for ( i = 0; i < KTRPictureBuffersNumber; i ++ )
            {
            iVideoPictureArray[i].iData.iRawData = new (ELeave) TPtr8(0, 0, 0);
            iVideoPictureArray[i].iData.iRawData->Set(iDataArray[i], dataBufferSize, dataBufferSize);
            iVideoPictureArray[i].iData.iDataSize.SetSize(iOutputPictureSize.iWidth, 
                                                          iOutputPictureSize.iHeight);
            iTranscoderPictureQueue.AddLast( iVideoPictureArray[i] );
            PRINT((_L("CTRTranscoderImp::AllocateBuffersL(), picture[%d], added to the queue"), i))
            PRINT((_L("CTRTranscoderImp::AllocateBuffersL(), picture data[0x%x]"), 
            iVideoPictureArray[i].iData.iRawData->Ptr() ))
            }
    
    PRINT((_L("CTRTranscoderImp::AllocateBuffersL(), Out")))
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::WriteCodedBufferL
// Writes coded buffer to transcoder
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::WriteCodedBufferL(CCMRMediaBuffer* aBuffer)
    {
    PRINT((_L("CTRTranscoderImp::WriteCodedBufferL(), In")))
    CTREventItem* newEvent = NULL;
    
    if (!aBuffer)
        {
        PRINT((_L("CTRTranscoderImp::WriteCodedBufferL(), Client sends invalid data")))
        User::Leave(KErrArgument);
        }
        
    if (iState == ETRRunning)
        {
        if (iSetRandomAccessPoint)
            {
            // Get new item from the eventsrc
            if (!iTranscoderEventSrc.IsEmpty())
                {
                newEvent = iTranscoderEventSrc.First();
                newEvent->iLink.Deque();
                }
            else
                {
                PRINT((_L("CTRTranscoderImp::WriteCodedBufferL(), iTranscoderEventSrc queue is empty, abort")))
                iObserver.MtroFatalError(KErrAbort);
                return;
                }
                
            if (newEvent)
                {
                newEvent->iTimestamp = aBuffer->TimeStamp();
                newEvent->iRandomAccessStatus = ETrue;
                PRINT((_L("CTRTranscoderImp::WriteCodedBufferL(), picture[%f] RandomAccess"), I64REAL(aBuffer->TimeStamp().Int64()) ))
                }
            
            iSetRandomAccessPoint = EFalse;
            }
        
        if (iPictureSinkSettingChanged)
            {
            if (!newEvent)
                {
                // Get new item from the event src queue
                if (!iTranscoderEventSrc.IsEmpty())
                    {
                    newEvent = iTranscoderEventSrc.First();
                    newEvent->iLink.Deque();
                    }
                else
                    {
                    PRINT((_L("CTRTranscoderImp::WriteCodedBufferL(), iTranscoderEventSrc queue is empty, abort")))
                    iObserver.MtroFatalError(KErrAbort);
                    return;
                    }
                }
                
            if (newEvent)
                {
                newEvent->iTimestamp = aBuffer->TimeStamp();
                newEvent->iEnablePictureSinkStatus = ETrue;
                newEvent->iEnablePictureSinkClientSetting = iPictureSinkClientSetting;
                PRINT((_L("CTRTranscoderImp::WriteCodedBufferL(), picture[%f] PS[%d]"), I64REAL(aBuffer->TimeStamp().Int64()), iPictureSinkClientSetting ))
                }

            // Reset change flag
            iPictureSinkSettingChanged = EFalse;
            }
        
        // encoder enabled / disabled
        if (iEncoderEnabledSettingChanged)
            {
            if (!newEvent)
                {
                // Get new item from the eventsrc
                if (!iTranscoderEventSrc.IsEmpty())
                    {
                    newEvent = iTranscoderEventSrc.First();
                    newEvent->iLink.Deque();
                    }
                else
                    {
                    PRINT((_L("CTRTranscoderImp::WriteCodedBufferL(), iTranscoderEventSrc queue is empty, abort")))
                    iObserver.MtroFatalError(KErrAbort);
                    return;
                    }
                }
                
            if (newEvent)
                {
                newEvent->iTimestamp = aBuffer->TimeStamp();
                newEvent->iEnableEncoderStatus = ETrue;
                newEvent->iEnableEncoderClientSetting = iEncoderEnableClientSetting;
                PRINT((_L("CTRTranscoderImp::WriteCodedBufferL(), picture[%f] Enc[%d]"), I64REAL(aBuffer->TimeStamp().Int64()), iEncoderEnableClientSetting ))
                }

            // Reset change flag
            iEncoderEnabledSettingChanged = EFalse;
            }
            
        if (newEvent)
            {
            // Put it to transcoder event queue
            iTranscoderEventQueue.AddLast(*newEvent);
            }
            
        // Write buffer to DecoderClient
        iVideoDecoderClient->WriteCodedBufferL(aBuffer);
        }
    else
        {
        PRINT((_L("CTRTranscoderImp::WriteCodedBufferL(), wrong state")))
        User::Leave(KErrNotReady);
        }

    PRINT((_L("CTRTranscoderImp::WriteCodedBufferL(), Out")))
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::MtroReturnCodedBuffer
// Returns coded buffer
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::MtrdvcoReturnCodedBuffer(CCMRMediaBuffer* aBuffer)
    {
    iObserver.MtroReturnCodedBuffer(aBuffer);
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::MtrdvcoNewPicture
// New decoded picture is available
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::MtrdvcoNewPicture(TVideoPicture* aDecodedPicture)
    {
    TInt status = KErrNone;
    TVideoPicture* picture = NULL;
    CTREventItem* nextEvent = NULL;
    MTRVideoPictureSink* pictureSink = iPictureSinkTemp; // 
    TBool pictureSinkEnabled = iCurrentPictureSinkEnabled;
    TBool encoderEnabled = iCurrentEncoderEnabled;
    
    if (iState == ETRFatalError)
        {
        // Nothing to do
        return;
        }
        
    if (!aDecodedPicture)
        {
        PRINT((_L("CTRTranscoderImp::MtrdvcoNewPicture(), decoded picture is not valid")))
        iObserver.MtroFatalError(KErrAlreadyExists);
        return;
        }
    else if (!aDecodedPicture->iData.iRawData)
        {
        PRINT((_L("CTRTranscoderImp::MtrdvcoNewPicture(), picture raw data is not valid")))
        iVideoDecoderClient->ReturnPicture(aDecodedPicture);
        iObserver.MtroFatalError(KErrAlreadyExists);
        return;
        }
        
    PRINT((_L("CTRTranscoderImp::MtrdvcoNewPicture(), decoded picture timestamp [%f]"), I64REAL(aDecodedPicture->iTimestamp.Int64()) ))

    switch(iMode)
        {
        case EFullTranscoding:
            {
            if ( !iNewEvent && !iTranscoderEventQueue.IsEmpty() )
                {
                // Get new event
                iNewEvent = iTranscoderEventQueue.First();
                iNewEvent->iLink.Deque();
                
                // Check the next event if there are any
                if (!iTranscoderEventQueue.IsEmpty())
                    {
                    nextEvent = iTranscoderEventQueue.First();
                    
                    if (aDecodedPicture->iTimestamp >= nextEvent->iTimestamp)
                        {
                        // Should not happen normally, indicate an error
                        PRINT((_L("CTRTranscoderImp::MtrdvcoNewPicture(), Previous event was not handled properly, abort data processing")))
                        iVideoDecoderClient->ReturnPicture(aDecodedPicture);
                        iObserver.MtroFatalError(KErrAlreadyExists);
                        return;
                        }
                    }
                }
                
            if (iNewEvent)
                {
                // Check timestamp for this event
                if (aDecodedPicture->iTimestamp >= iNewEvent->iTimestamp)
                    {
                    // Perform requested client's operation
                    // 1. PictureSinkStatus
                    if (iNewEvent->iEnablePictureSinkStatus)
                        {
                        pictureSinkEnabled = iNewEvent->iEnablePictureSinkClientSetting;
                        }

                    // 2. EnableEncoderStatus
                    if (iNewEvent->iEnableEncoderStatus)
                        {
                        encoderEnabled = iNewEvent->iEnableEncoderClientSetting;
                        }
                    
                    // 3. Random access
                    if (iNewEvent->iRandomAccessStatus)
                        {
                        iCurrentRandomAccess = iNewEvent->iRandomAccessStatus;
                        }

                    // This event is already handled, it's not new anymore
                    if (!pictureSinkEnabled && !encoderEnabled)
                        {
                        // Picture is returned to decoder, we don't need any actions for that
                        iNewEvent->Reset();
                        iTranscoderEventSrc.AddLast(*iNewEvent);
                        }
                    else if (pictureSinkEnabled)
                        {
                        if (iIntermediatePictureSize == iDecodedPictureSize)
                            {
                            // Picture is sent to the client first, decoded picture is not returned 
                            // to decoder until it's hold by the client. 
                            // No new events are handled here, act according global settings. 
                            iNewEvent->Reset();
                            iTranscoderEventSrc.AddLast(*iNewEvent);
                            }
                        else
                            {
                            // Picture is processed acynchronously first
                            iTranscoderAsyncEventQueue.AddLast(*iNewEvent);
                            
                            // Keep previous current global setting
                            iCurrentAsyncPictureSinkEnabled = iCurrentPictureSinkEnabled;
                            iCurrentAsyncEncoderEnabled = iCurrentEncoderEnabled;
                            }
                        }
                    else // encoderEnabled otherwise
                        {
                        // Picture is processed acynchronously first
                        iTranscoderAsyncEventQueue.AddLast(*iNewEvent);

                        // Keep previous current global setting
                        iCurrentAsyncPictureSinkEnabled = iCurrentPictureSinkEnabled;
                        iCurrentAsyncEncoderEnabled = iCurrentEncoderEnabled;
                        }
                    
                    // Reset new event
                    iNewEvent = NULL;

                    // Update current settings
                    iCurrentPictureSinkEnabled = pictureSinkEnabled;
                    iCurrentEncoderEnabled = encoderEnabled;
                    }
                }

            // Settings now defined, act accordingly
            if (!pictureSinkEnabled && !encoderEnabled)
                {
                // Nothing to do with this picture, return it to decoder
                PRINT((_L("CTRTranscoderImp::MtrdvcoNewPicture(), return picture[%f] to decoder"), I64REAL(aDecodedPicture->iTimestamp.Int64()) ))
                iVideoDecoderClient->ReturnPicture(aDecodedPicture);
                return;
                }
                
            if (pictureSinkEnabled)
                {
                pictureSink = iPictureSinkTemp;
                
                if (!pictureSink)
                    {
                    PRINT((_L("CTRTranscoderImp::MtrdvcoNewPicture(), picture sink was not set, report to client!")))
                    iVideoDecoderClient->ReturnPicture(aDecodedPicture);
                    iObserver.MtroFatalError(KErrNotReady);
                    return; 
                    }
                }
            else
                {
                pictureSink = NULL;
                }
                
            PRINT((_L("CTRTranscoderImp::MtrdvcoNewPicture(), PS[%d], Enc[%d], RandAcc[%d] for [%f]"), pictureSinkEnabled, encoderEnabled, iCurrentRandomAccess, I64REAL(aDecodedPicture->iTimestamp.Int64()) ))

            if ( pictureSink && (iIntermediatePictureSize == iDecodedPictureSize) )
                {
                // Since the client requested to provide initial decoded picture, send
                // it first, and only then resample and encode. (See SendPictureToTranscoder)
                if (!iTranscoderTRPictureQueue.IsEmpty())
                    {
                    TTRVideoPicture* pictureToClient = iTranscoderTRPictureQueue.First();
                    pictureToClient->iLink.Deque();
                    pictureToClient->iRawData = aDecodedPicture->iData.iRawData;
                    pictureToClient->iDataSize = aDecodedPicture->iData.iDataSize;
                    pictureToClient->iTimestamp = aDecodedPicture->iTimestamp;
                    
                    // Store picture until it's returned from the client
                    iDecodedPicture = aDecodedPicture;
                                
                    // Deliver picture to the client
                    PRINT((_L("CTRTranscoderImp::MtrdvcoNewPicture(), send picture to the client via sink")))
                    pictureSink->MtroPictureFromTranscoder(pictureToClient);
                    }
                else
                    {
                    PRINT((_L("CTRTranscoderImp::MtrdvcoNewPicture(), iTranscoderTRPictureQueue is empty, abort operation")))
                    iVideoDecoderClient->ReturnPicture(aDecodedPicture);
                    iObserver.MtroFatalError(KErrAbort);
                    }
                    
                return;
                }

            if (iOptimizedDataTransfer)
                {
                // We need a new picture buffer every time !!!
                if ( !iCIPictureBuffersQueue.IsEmpty() )
                    {
                    picture = iCIPictureBuffersQueue.First();
                    picture->iLink.Deque();
                    }
                else
                    {
                    TRAP( status, picture = iVideoEncoderClient->GetTargetVideoPictureL() );
                    }

                if (status != KErrNone)
                    {
                    PRINT((_L("CTRTranscoderImp::MtrdvcoNewPicture(), GetTargetVideoPictureL failed [%d]"), status))
                    iVideoDecoderClient->ReturnPicture(aDecodedPicture);
                    iObserver.MtroFatalError(status);
                    return;
                    }
                else if (picture)
                    {
                    // Set options for picture
                    picture->iData.iDataSize = iOutputPictureSize;
                    
                    // we don't support resampling for 422, set default for 420 length
                    // Add check of output data format (422 or 420) in the future and set dataLength properly
                    TInt length = iOutputPictureSize.iWidth * iOutputPictureSize.iHeight * 3 / 2;

                    if ( length > picture->iData.iRawData->MaxLength() )
                        {
                        PRINT((_L("CTRTranscoderImp::MtrdvcoNewPicture(), length exceeds CI buffer maxlength[%d, %d]"), length, picture->iData.iRawData->MaxLength() ))
                        iVideoDecoderClient->ReturnPicture(aDecodedPicture);
                        iObserver.MtroFatalError(KErrGeneral);
                        return;
                        }

                    // set length
                    picture->iData.iRawData->SetLength(length);
                    }
                else
                    {
                    if (iRealTime) // (picture is not available, act according realtime mode)
                        {
                        // Picture buffer is not available from encoder hwdevice
                        // return decoded picture back, otherwise suspend decoding.. 
                        PRINT((_L("CTRTranscoderImp::MtrdvcoNewPicture(), Picture buffer is not available through CI, Drop") ))
                        iVideoDecoderClient->ReturnPicture(aDecodedPicture);
                        return;
                        }
                    else
                        {
                        // Keep picture and does not return it to DecoderClient before processing
                        PRINT((_L("CTRTranscoderImp::MtrdvcoNewPicture(), Picture buffer is not available through CI, store") ))
                        iWaitNewDecodedPicture = aDecodedPicture;
                        return;
                        }
                    }
                }
            else
                {
                if (!iTranscoderPictureQueue.IsEmpty())
                    {
                    picture = iTranscoderPictureQueue.First();
                    }
                else
                    {
                    PRINT((_L("CTRTranscoderImp::MtrdvcoNewPicture(), iTranscoderPictureQueue is empty")))

                    if (iRealTime) // (picture is not available, act according realtime mode)
                        {
                        // Picture buffer is not available from encoder hwdevice
                        // return decoded picture back, otherwise suspend decoding.. 
                        PRINT((_L("CTRTranscoderImp::MtrdvcoNewPicture(), Picture buffer is not available fromEncoder, Drop") ))
                        iVideoDecoderClient->ReturnPicture(aDecodedPicture);
                        return;
                        }
                    else
                        {
                        // Keep picture and does not return it to DecoderClient before processing
                        PRINT((_L("CTRTranscoderImp::MtrdvcoNewPicture(), Picture buffer is not available fromEncoder, store") ))
                        iWaitNewDecodedPicture = aDecodedPicture;
                        return;
                        }
                    }
                }
                
            if (picture)
                {
                PRINT((_L("CTRTranscoderImp::MtrdvcoNewPicture(), picture[0x%x], data[0x%x]"), 
                picture, picture->iData.iRawData->Ptr() ))

                PRINT((_L("CTRTranscoderImp::MtrdvcoNewPicture(), decPicture[0x%x], data[0x%x]"), 
                aDecodedPicture, aDecodedPicture->iData.iRawData->Ptr() ))

                // Resample this picture
                TRAP(status, iScaler->SetScalerOptionsL( *(aDecodedPicture->iData.iRawData), *(picture->iData.iRawData), 
                                                         aDecodedPicture->iData.iDataSize, picture->iData.iDataSize ) );
                if (status != KErrNone)
                    {
                    PRINT((_L("CTRTranscoderImp::MtrdvcoNewPicture(), Set scaler options failed[%d]"), status))
                    iVideoDecoderClient->ReturnPicture(aDecodedPicture);
                    iObserver.MtroFatalError(status);
                    return;
                    }

                if (!iOptimizedDataTransfer)
                    {
                    // Remove picture from transcoder queue
                    picture->iLink.Deque();
                    }

                // Scale
                iScaler->Scale();
                picture->iData.iDataFormat = EYuvRawData;
                picture->iTimestamp = aDecodedPicture->iTimestamp;

                // Put resampled picture to encoder queue
                iEncoderPictureQueue.AddLast(*picture);

                // Make request to process this picture
                this->DoRequest();

                // Return used picture to decoder
                iVideoDecoderClient->ReturnPicture(aDecodedPicture);
                }
            else
                {
                PRINT((_L("CTRTranscoderImp::MtrdvcoNewPicture(), Transcoder picture queue is empty, skip this buffer")))
                iVideoDecoderClient->ReturnPicture(aDecodedPicture);
                }
            }
            break;
            
        case EDecoding:
            {
            if (!iPictureSink)
                {
                PRINT((_L("CTRTranscoderImp::MtrdvcoNewPicture(), iPictureSink was not set")))
                iObserver.MtroFatalError(KErrNotReady);
                return;
                }
                
            if (aDecodedPicture->iData.iDataSize.iWidth != iDecodedPictureSize.iWidth ||
                aDecodedPicture->iData.iDataSize.iHeight != iDecodedPictureSize.iHeight )
                {
                PRINT((_L("CTRTranscoderImp::MtrdvcoNewPicture(), picture size is not valid")))
                iVideoDecoderClient->ReturnPicture(aDecodedPicture);
                iObserver.MtroFatalError(KErrAlreadyExists);
                return;
                }

            // Store picture until it's returned from the client
            iDecodedPicture = aDecodedPicture;

            if (iDecodedPictureSize == iOutputPictureSize)
                {
                // Send picture directly to the client
                iPicureToClient.iRawData = aDecodedPicture->iData.iRawData;
                iPicureToClient.iDataSize = aDecodedPicture->iData.iDataSize;
                iPicureToClient.iTimestamp = aDecodedPicture->iTimestamp;
                iPictureSink->MtroPictureFromTranscoder(&iPicureToClient);
                }
            else
                {
                // Resample this picture to desired size and send to the client
                if ( !iTranscoderPictureQueue.IsEmpty() )
                    {
                    iVideoPictureTemp = iTranscoderPictureQueue.First();
                    iVideoPictureTemp->iLink.Deque();

                    // Resample this picture
                    TRAP(status, iScaler->SetScalerOptionsL( *(aDecodedPicture->iData.iRawData), *(iVideoPictureTemp->iData.iRawData), 
                                                             aDecodedPicture->iData.iDataSize, iVideoPictureTemp->iData.iDataSize ) );
                    if (status != KErrNone)
                        {
                        PRINT((_L("CTRTranscoderImp::MtrdvcoNewPicture(), Set scaler options failed[%d]"), status))
                        iVideoDecoderClient->ReturnPicture(aDecodedPicture);
                        iObserver.MtroFatalError(status);
                        return;
                        }
                    
                    iScaler->Scale();
                    iPicureToClient.iRawData = iVideoPictureTemp->iData.iRawData;
                    iPicureToClient.iDataSize = iVideoPictureTemp->iData.iDataSize;
                    iPicureToClient.iTimestamp = aDecodedPicture->iTimestamp;

                    // Return decoded picture to decoder
                    iVideoDecoderClient->ReturnPicture(aDecodedPicture);

                    // Sent picture to the client                   
                    iPictureSink->MtroPictureFromTranscoder(&iPicureToClient);
                    }
                }
            }
            break;

        default:
            {
            PRINT((_L("CTRTranscoderImp::MtrdvcoNewPicture(), observer should not be called in this mode")))
            iObserver.MtroFatalError(KErrAlreadyExists);
            return;
            }
        }
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::DoRequest
// Makes a new request
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::DoRequest()
    {
    if ( !this->IsActive() )
        {
        this->SetActive();
        iStat = &iStatus;
        User::RequestComplete( iStat, KErrNone );
        }
    else
        {
        PRINT((_L("CTRTranscoderImp::DoRequest(), AO is already active")))
        }
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::RunL
// CActive's RunL implementation
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::RunL()
    {
    PRINT((_L("CTRTranscoderImp::RunL(), In")))
    TVideoPicture* picture = NULL;
    MTRVideoPictureSink* pictureSink = iPictureSinkTemp;
    TBool pictureSinkEnabled = iCurrentAsyncPictureSinkEnabled;
    TBool encoderEnabled = iCurrentAsyncEncoderEnabled;
    TBool randomAccess = EFalse;
    
    
    if ( !iEncoderPictureQueue.IsEmpty() )
        {
        switch(iMode)
            {
            case EFullTranscoding:
                {
                // Encode all pictures in the queue
                TInt count = 0; 


                while ( !iEncoderPictureQueue.IsEmpty() )
                    {
                    PRINT((_L("CTRTranscoderImp::RunL(), count[%d]"), count))
                    count ++; 
                    picture = iEncoderPictureQueue.First();

                    // Check event queue
                    if (!iAsyncEvent && !iTranscoderAsyncEventQueue.IsEmpty())
                        {
                        // Trasnscoder configuration is unknown, handle new event
                        iAsyncEvent = iTranscoderAsyncEventQueue.First();
                        iAsyncEvent->iLink.Deque();
                        }
                        
                    if (iAsyncEvent)
                        {
                        // Check timestamp for this event
                        if (picture->iTimestamp >= iAsyncEvent->iTimestamp)
                            {
                            // Perform requested client's operation
                            // 1. PictureSinkStatus
                            if (iAsyncEvent->iEnablePictureSinkStatus)
                                {
                                pictureSinkEnabled = iAsyncEvent->iEnablePictureSinkClientSetting;
                                }

                            // 2. EnableEncoderStatus
                            if (iAsyncEvent->iEnableEncoderStatus)
                                {
                                encoderEnabled = iAsyncEvent->iEnableEncoderClientSetting;
                                }
                                
                            // 3. RandomAccess
                            if (iAsyncEvent->iRandomAccessStatus)
                                {
                                randomAccess = iAsyncEvent->iRandomAccessStatus;
                                }
                                
                            // This event is already handled, we don't need it anymore
                            iAsyncEvent->Reset();
                            iTranscoderEventSrc.AddLast(*iAsyncEvent);
                            iAsyncEvent = NULL;
                            
                            // Update current settings
                            iCurrentAsyncPictureSinkEnabled = pictureSinkEnabled;
                            iCurrentAsyncEncoderEnabled = encoderEnabled;
                            iCurrentAsyncRandomAccess = randomAccess;
                            }
                        }
                    // Else: Event queue is empty, act according async global options
                    
                    // Settings now defined, act accordingly
                    PRINT((_L("CTRTranscoderImp::RunL(), PS[%d], Enc[%d], RandAcc[%d] for [%f]"), 
                    pictureSinkEnabled, encoderEnabled, randomAccess, I64REAL(picture->iTimestamp.Int64()) ))
                    
                    if (pictureSinkEnabled)
                        {
                        if (!pictureSink)
                            {
                            PRINT((_L("CTRTranscoderImp::RunL(), Picture sink was not set, time to panic")))
                            TRASSERT(0);
                            }
                            
                        // Send decoded picture to the client first, and encode it after returning
                        if (!iTranscoderTRPictureQueue.IsEmpty())
                            {
                            picture->iLink.Deque();

                            TTRVideoPicture* pictureToClient = iTranscoderTRPictureQueue.First();
                            pictureToClient->iLink.Deque();
                            pictureToClient->iRawData = picture->iData.iRawData;
                            pictureToClient->iDataSize = picture->iData.iDataSize;
                            pictureToClient->iTimestamp = picture->iTimestamp;

                            // Keep TVideoPicture container to the iContainerWaitQueue until the picture is returned back. 
                            iContainerWaitQueue.AddLast(*picture);
                        
                            // Deliver picture to the client
                            pictureSink->MtroPictureFromTranscoder(pictureToClient);
                            // 
                            // return;
                            }
                        else
                            {
                            PRINT((_L("CTRTranscoderImp::RunL(), iTranscoderTRPictureQueue is empty, abort")))
                            iObserver.MtroFatalError(KErrAbort);
                            return;
                            }
                        }
                    else if (!encoderEnabled)
                        {
                        // All picture in this queue must be encoded
                        PRINT((_L("CTRTranscoderImp::RunL(), All pictures from iEncoderPictureQueue must be encoded, error!")))
                        TRASSERT(0);
                        }
                    else
                        {
                        // Send picture to encoder    
                        picture = iEncoderPictureQueue.First();
                        picture->iLink.Deque();
                    
                        if (randomAccess)
                            {
                            iVideoEncoderClient->SetRandomAccessPoint();
                            randomAccess = EFalse;
                            }

                        iVideoEncoderClient->EncodePictureL(picture);
                        }
                    } // END while loop
                }
                break;

            case EDecoding:
                {
                // Send decoded picture to the client. 
                if (!iTranscoderTRPictureQueue.IsEmpty())
                    {
                    picture = iEncoderPictureQueue.First();
                    picture->iLink.Deque();

                    // Take Transcoder picture container
                    TTRVideoPicture* pictureToClient = iTranscoderTRPictureQueue.First();
                    pictureToClient->iLink.Deque();

                    pictureToClient->iRawData = picture->iData.iRawData;
                    pictureToClient->iDataSize = picture->iData.iDataSize;
                    pictureToClient->iTimestamp = picture->iTimestamp;

                    if (iPictureSink)
                        {
                        iPictureSink->MtroPictureFromTranscoder(pictureToClient);
                        }
                    else
                        {
                        // Should not happen
                        PRINT((_L("CTRTranscoderImp::RunL(), Decoding, PictureSink is not valid")))
                        TRASSERT(0);
                        }

                    // Keep TVideoPicture container to the iContainerWaitQueue until the picture is returned back. 
                    iContainerWaitQueue.AddLast(*picture);
                    }
                }
                break;

            case EEncoding:
                {
                // Send picture to video encoder
                if ( !iEncoderPictureQueue.IsEmpty() )
                    {
                    picture = iEncoderPictureQueue.First();
                    picture->iLink.Deque();
                    iVideoEncoderClient->EncodePictureL(picture);
                    }
                }
                break;

            case EResampling:
                {
                PRINT((_L("CTRTranscoderImp::RunL(), should not be called in Resampling mode")))
                TRASSERT(0);
                }
                break;

            case EOperationNone:
                {
                User::Leave(KErrNotReady);
                }
                break;

            default:
                {
                User::Leave(KErrNotReady);
                }
                break;
            }
        }

    PRINT((_L("CTRTranscoderImp::RunL(), Out")))
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::DoCancel
// Cancel
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::DoCancel()
    {
    // Nothing to do
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::RunError
// Handles AO leave
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CTRTranscoderImp::RunError(TInt aError)
    {
    PRINT((_L("CTRTranscoderImp::RunError(), seems RunL leaved, error[%d]."), aError))
    
    iObserver.MtroFatalError(aError);

    return KErrNone;
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::MtrdvcoEncoderReturnPicture
// Returns picture from encoder
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::MtrdvcoEncoderReturnPicture(TVideoPicture* aPicture)
    {
    PRINT((_L("CTRTranscoderImp::MtrdvcoEncoderReturnPicture()")))
    TTRVideoPicture* pictureToClient = NULL;
    TInt status = KErrNone;


    if (!aPicture)
        {
        PRINT((_L("CTRTranscoderImp::MtrdvcoEncoderReturnPicture(), picture is not valid")))
        iObserver.MtroFatalError(KErrAlreadyExists);
        return;
        }
    else if (!aPicture->iData.iRawData)
        {
        PRINT((_L("CTRTranscoderImp::MtrdvcoEncoderReturnPicture(), picture raw data is not valid")))
        iObserver.MtroFatalError(KErrAlreadyExists);
        return;
        }

    switch(iMode)
        {
        case EFullTranscoding:
            {
            // if (BuffManCI) { nothing to do, possibly send next picture for encoder, if available }
            // Put returned picture to the iTranscoderPictureQueue
            if (!iOptimizedDataTransfer)
                {
                iTranscoderPictureQueue.AddLast(*aPicture);
                
                // There could be some decoded pictures waiting for processing, check it
                this->MtrdvcoNewBuffers();
                }
            else
                {
                PRINT((_L("CTRTranscoderImp::MtrdvcoEncoderReturnPicture(), Nothing to do here, since optimized data transfer in use")))
                return;
                }

            if ( !iEncoderPictureQueue.IsEmpty() )
                {
                // If there are pictures available in EncoderPicture for processing, initiate data transfer from here
                PRINT((_L("CTRTranscoderImp::MtrdvcoEncoderReturnPicture(), some pictures were not processed in iEncoderPictureQueue")))
                TRAP(status, this->RunL());
                
                if (status != KErrNone)
                    {
                    // Indicate eror status to the client
                    PRINT((_L("CTRTranscoderImp::MtrdvcoEncoderReturnPicture(), error[%d]"), status))
                    iObserver.MtroFatalError(status);
                    return;
                    }
                }
            }
            break;

        case EDecoding:
                {
                // Should not happen
                PRINT((_L("CTRTranscoderImp::MtrdvcoEncoderReturnPicture(), Encoder returns picture in Decoding mode")))
                TRASSERT(0);
                }

            break;

        case EEncoding:
            {
            // Put picture to transcoder queue
            iTranscoderPictureQueue.AddLast(*aPicture);
            
            if (iInputPictureSize == iOutputPictureSize)
                {
                // Find TRPicture container corresponding to picture data & return it to the client; 
                if ( !iTranscoderTRPictureQueue.IsEmpty() )
                    {
                    pictureToClient = iTranscoderTRPictureQueue.First();
                    pictureToClient->iLink.Deque();
                    
                    if (pictureToClient->iRawData == aPicture->iData.iRawData)
                        {
                        // Return picture
                        iPictureSink->MtroPictureFromTranscoder(pictureToClient);
                        }
                    else
                        {
                        TPtr8 tmp(*pictureToClient->iRawData);
                        iTranscoderTRPictureQueue.AddLast(*pictureToClient);
                        
                        // Encoder returns pictures in different order than they were sent
                        // Check next picture
                        pictureToClient = iTranscoderTRPictureQueue.First();
                        pictureToClient->iLink.Deque();
                        
                        while (*pictureToClient->iRawData != tmp)
                            {
                            if ( pictureToClient->iRawData == aPicture->iData.iRawData )
                                {
                                // Return picture
                                iPictureSink->MtroPictureFromTranscoder(pictureToClient);
                                return;
                                }
                            else
                                {
                                // Put back to the queue this picture and check the next one
                                iTranscoderTRPictureQueue.AddLast(*pictureToClient);
                                pictureToClient = iTranscoderTRPictureQueue.First();
                                pictureToClient->iLink.Deque();
                                }
                            }
                        
                        // The picture has already been removed from the TR picture queue.
                        // Probably the client thought a frame was skipped and sent a new picture for encoding,
                        // therefore the client is not expecting us to send anything back => do nothing
                            
                        PRINT((_L("CTRTranscoderImp::MtrdvcoEncoderReturnPicture(), Picture already returned, nothing to do")))
                        }
                    }
                }
            }
            break;

        case EResampling:
            {
            // Should not happen
            PRINT((_L("CTRTranscoderImp::MtrdvcoEncoderReturnPicture(), Encoder returns picture in Resampling mode.")))
            TRASSERT(0);
            }
            break;

        default:
            {
            PRINT((_L("CTRTranscoderImp::MtrdvcoEncoderReturnPicture(), default case")))
            TRASSERT(0);
            }
        }
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::SendPictureToTranscoderL
// Receives picture from the client
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::SendPictureToTranscoderL(TTRVideoPicture* aPicture)
    {
    PRINT((_L("CTRTranscoderImp::SendPictureToTranscoderL(), In")))
    TVideoPicture* picture = NULL;
    TInt status = KErrNone;
    TBool encoderEnabled = EFalse;
    TBool randomAccess = EFalse;   
    
    if (iState == ETRFatalError)
        {
        // Nothing to do
        return;
        }

    if ( (iState != ETRRunning) && (iState != ETRPaused) &&
         (iState != ETRStopped) && (iMode != EDecoding) )
        {
        PRINT((_L("CTRTranscoderImp::SendPictureToTranscoderL(), wrong state")))
        User::Leave(KErrNotReady);
        }

    if (!aPicture)
        {
        PRINT((_L("CTRTranscoderImp::SendPictureToTranscoderL(), Client sends invalid data")))
        User::Leave(KErrArgument);
        }
    else if (!aPicture->iRawData)
        {
        PRINT((_L("CTRTranscoderImp::SendPictureToTranscoderL(), Client sends invalid raw data")))
        User::Leave(KErrArgument);
        }

    PRINT((_L("CTRTranscoderImp::SendPictureToTranscoderL(), picture timestamp [%f]"), I64REAL(aPicture->iTimestamp.Int64()) ))

    switch(iMode)
        {
        case EFullTranscoding:
            {
            if (!iPictureSinkTemp)
                {
                PRINT((_L("CTRTranscoderImp::SendPictureToTranscoderL(), Client did not set picture sink! leave")))
                User::Leave(KErrNotReady);
                }
                
            if (iIntermediatePictureSize == iDecodedPictureSize)
                {
                // Picture path is Decoder->Client->Transcoder
                // Decoder does not generate new picture until this one have not been returned
                // Act according global settings defined in NewPicture
                encoderEnabled = iCurrentEncoderEnabled;
                randomAccess = iCurrentRandomAccess;
                
                // Reset it here
                iCurrentRandomAccess = EFalse;
                }
            else
                {
                // Picture was processed asynchronously
                // Set global async options
                encoderEnabled = iCurrentAsyncEncoderEnabled;
                randomAccess = iCurrentAsyncRandomAccess;
                
                // Reset it here
                iCurrentAsyncRandomAccess = EFalse;
                }
                
            if (!encoderEnabled)
                {
                // Put TRPicture container to the queue in any case
                iTranscoderTRPictureQueue.AddLast(*aPicture);

                // Nothing to do for this picture, return it to decoder
                if (iIntermediatePictureSize == iDecodedPictureSize)
                    {
                    if (iDecodedPicture)
                        {
                        // Return & reset
                        PRINT((_L("CTRTranscoderImp::SendPictureToTranscoderL(), return picture [%f] to decoder"), I64REAL(iDecodedPicture->iTimestamp.Int64()) ))
                        TVideoPicture* decodedPicture = iDecodedPicture;
                        iDecodedPicture = NULL;
                        iVideoDecoderClient->ReturnPicture(decodedPicture);
                        }
                    }
                else
                    {
                    // If picture was sent to the client asynchronously, nothing to return to decoder
                    // Picture path is Decoder->Transcoder->client->Transcoder
                    // Picture was taken from iTranscoderPictureQueue, put it back, since it's not encoded
                    // Take container for TVideoPicture
                    if ( !iContainerWaitQueue.IsEmpty() )
                        {
                        // Take container
                        picture = iContainerWaitQueue.First();
                        picture->iLink.Deque();
                        picture->iData.iRawData = aPicture->iRawData;
                        picture->iData.iDataSize = aPicture->iDataSize;
                        picture->iTimestamp = aPicture->iTimestamp;
                        
                        if (!iOptimizedDataTransfer)
                            {
                            iTranscoderPictureQueue.AddLast(*picture);
                            }
                        else
                            {
                            iCIPictureBuffersQueue.AddLast(*picture);
                            
                            // Some pictures were not processed yet and wait for processing
                            this->MtrdvcoNewBuffers();
                            }
                        }
                    }

                return;
                }


            // Client returns processed picture. (Resample, if needed) and send it to encoder;
            if ( /*iPictureSink &&*/ (iIntermediatePictureSize == iDecodedPictureSize) )
                {
                // Check input picture size
                if ( aPicture->iDataSize != iDecodedPictureSize )
                    {
                    PRINT((_L("CTRTranscoderImp::SendPictureToTranscoderL(), The client sets wrong picture size")))
                    User::Leave(KErrArgument);
                    }

                //  AA Tuning..(Codecs AOs can be higher priority, so if there are any pictures in iEncoderPictureQueue waiting for processing, send those first)
                if ( (iDecodedPictureSize == iOutputPictureSize) && (!iEncoderPictureQueue.IsEmpty()) )
                    {
                    PRINT((_L("CTRTranscoderImp::SendPictureToTranscoderL(), some pictures were not processed in iEncoderPictureQueue")))
                    this->RunL();
                    }

                if (iOptimizedDataTransfer)
                    {
                    if ( !iCIPictureBuffersQueue.IsEmpty() )
                        {
                        // Target video picture buffer is already available, use it
                        picture = iCIPictureBuffersQueue.First();
                        picture->iLink.Deque();
                        PRINT((_L("CTRTranscoderImp::SendPictureToTranscoderL(), picture is already available")))
                        }
                    else
                        {
                        PRINT((_L("CTRTranscoderImp::SendPictureToTranscoderL(), picture not available, request new;")))
                        // Picture was already returned with WritePictureL
                        TRAP( status, picture = iVideoEncoderClient->GetTargetVideoPictureL() );
                        }

                    if (status != KErrNone)
                        {
                        // return decoded picture to decoder
                        PRINT((_L("CTRTranscoderImp::SendPictureToTranscoderL(), GetTargetVideoPictureL leved with [%d]"), status))
                        
                        if (iDecodedPicture)
                            {
                            // Return & reset
                            TVideoPicture* decodedPicture = iDecodedPicture;
                            iDecodedPicture = NULL;
                            iVideoDecoderClient->ReturnPicture(decodedPicture);
                            }
                        
                        // Report the error to the client
                        iObserver.MtroFatalError(status);
                        return;
                        }
                    else if (picture)
                        {
                        // Set options for picture
                        picture->iData.iDataSize = iOutputPictureSize;
                        
                        // we don't support resampling for 422, set default for 420 length
                        // Add check of output data format (422 or 420) in the future and set dataLength properly
                        TInt length = iOutputPictureSize.iWidth * iOutputPictureSize.iHeight * 3 / 2;

                        if ( length > picture->iData.iRawData->MaxLength() )
                            {
                            PRINT((_L("CTRTranscoderImp::SendPictureToTranscoderL(), length exceeds CI buffer maxlength[%d, %d]"), length, picture->iData.iRawData->MaxLength() ))
                            
                            if (iDecodedPicture)
                                {
                                // Return & reset
                                TVideoPicture* decodedPicture = iDecodedPicture;
                                iDecodedPicture = NULL;
                                iVideoDecoderClient->ReturnPicture(decodedPicture);
                                }
                            
                            iObserver.MtroFatalError(KErrGeneral);
                            return;
                            }

                        // set length
                        picture->iData.iRawData->SetLength(length);
                        }
                    else
                        {
                        if (iRealTime) // (picture is not available, act according realtime mode)
                            {
                            // Picture buffer is not available from encoder hwdevice
                            // return decoded picture back, otherwise suspend decoding.. 
                            PRINT((_L("CTRTranscoderImp::SendPictureToTranscoderL(), Picture buffer is not available through CI, Drop") ))
                            
                            if (iDecodedPicture)
                                {
                                iVideoDecoderClient->ReturnPicture(iDecodedPicture);
                                }

                            return;
                            }
                        else
                            {
                            // Keep picture and does not return it to DecoderClient before processing
                            PRINT((_L("CTRTranscoderImp::SendPictureToTranscoderL(), Picture buffer is not available through CI, store") ))
                            iWaitPictureFromClient = aPicture;
                            return;
                            }
                        }
                    }
                else
                    {
                    if ( !iTranscoderPictureQueue.IsEmpty() )
                        {
                        picture = iTranscoderPictureQueue.First();
                        }
                    else
                        {
                        //  AA (don't report FatalError & wait for newBuffers callback from the encoder)
                        // return decoded picture to decoder
                        PRINT((_L("CTRTranscoderImp::SendPictureToTranscoderL(), iTranscoderPictureQueue is empty")))

                        if (iRealTime) // (picture is not available, act according realtime mode)
                            {
                            // Picture buffer is not available from encoder hwdevice
                            // return decoded picture back, otherwise suspend decoding.. 
                            PRINT((_L("CTRTranscoderImp::SendPictureToTranscoderL(), Picture buffer is not available FromEncoder, Drop") ))
                            
                            if (iDecodedPicture)
                                {
                                iVideoDecoderClient->ReturnPicture(iDecodedPicture);
                                }

                            return;
                            }
                        else
                            {
                            // Keep picture and does not return it to DecoderClient before processing
                            PRINT((_L("CTRTranscoderImp::SendPictureToTranscoderL(), Picture buffer is not available FromEncoder, store") ))
                            iWaitPictureFromClient = aPicture;
                            return;
                            }
                        }
                    }

                if (picture)
                    {
                    // Returned picture was not resampled. Resample it first.
                    TRAP(status, iScaler->SetScalerOptionsL( *(aPicture->iRawData), 
                                                             *(picture->iData.iRawData), 
                                                             aPicture->iDataSize, 
                                                             picture->iData.iDataSize ) );
                    if (status != KErrNone)
                        {
                        PRINT((_L("CTRTranscoderImp::SendPictureToTranscoderL(), Set scaler options failed[%d]"), status))

                        if (iDecodedPicture)
                            {
                            // Return & reset
                            TVideoPicture* decodedPicture = iDecodedPicture;
                            iDecodedPicture = NULL;
                            iVideoDecoderClient->ReturnPicture(decodedPicture);
                            }

                        iTranscoderTRPictureQueue.AddLast(*aPicture);
                        
                        // Inform the client with an error
                        User::Leave(status);
                        }
                    
                    if ( !iOptimizedDataTransfer )
                        {
                        // Remove picture from transcoder queue
                        picture->iLink.Deque();
                        }
                    
                    // Scale picture
                    iScaler->Scale();

                    // Set params
                    picture->iTimestamp = aPicture->iTimestamp;
                    }
                }
            else
                {
                // Send picture to encoder without resampling
                // Take container for TVideoPicture
                if ( !iContainerWaitQueue.IsEmpty() )
                    {
                    // Take container
                    picture = iContainerWaitQueue.First();
                    picture->iLink.Deque();
                    picture->iData.iRawData = aPicture->iRawData;
                    picture->iData.iDataSize = aPicture->iDataSize;
                    picture->iTimestamp = aPicture->iTimestamp;
                    }
                }

            // Put client's container to TRPicture queue
            iTranscoderTRPictureQueue.AddLast(*aPicture);

            if (picture)
                {
                // Encode picture
                if (randomAccess)
                    {
                    iVideoEncoderClient->SetRandomAccessPoint();
                    }

                iVideoEncoderClient->EncodePictureL(picture);
                }
                
            //  AA
            // return decoded picture here, if there's some available
            if (iDecodedPicture)
                {
                PRINT((_L("CTRTranscoderImp::SendPictureToTranscoderL(), Return decoded picture now")))

                // Return & reset
                TVideoPicture* decodedPicture = iDecodedPicture;
                iDecodedPicture = NULL;
                iVideoDecoderClient->ReturnPicture(decodedPicture);
                }
            }
            break;

        case EDecoding:
            {
            // Client returns decoded picture back. 
            if (iDecodedPictureSize == iOutputPictureSize)
                {
                if (iDecodedPicture)
                    {
                    iVideoDecoderClient->ReturnPicture(iDecodedPicture);
                    }
                }
            else
                {
                // Put picture back to the queue
                iTranscoderPictureQueue.AddLast(*iVideoPictureTemp);
                }
            }
            break;

        case EEncoding:
            {
            // Check input picture size
            if ( aPicture->iDataSize != iInputPictureSize )
                {
                PRINT((_L("CTRTranscoderImp::SendPictureToTranscoderL(), Encoding: The client sets wrong picture size")))
                User::Leave(KErrArgument);
                }

            // Client writes new picture to encode. Encode it and return back;
            if (!iPictureSink)
                {
                PRINT((_L("CTRTranscoderImp::SendPictureToTranscoderL(), picture sink is not set by the client")))
                User::Leave(KErrNotReady);
                }

            if ( !iTranscoderPictureQueue.IsEmpty() )
                {
                picture = iTranscoderPictureQueue.First();

                if (iInputPictureSize != iOutputPictureSize)
                    {
                    // Resample this picture first, since picture sizes are different
                    iScaler->SetScalerOptionsL( *(aPicture->iRawData), *(picture->iData.iRawData), aPicture->iDataSize, 
                                                picture->iData.iDataSize );

                    // Scale
                    iScaler->Scale();
                    }
                else
                    {
                    picture->iData.iDataSize = aPicture->iDataSize;
                    picture->iData.iRawData = aPicture->iRawData;
                    }

                picture->iData.iDataFormat = EYuvRawData;
                picture->iTimestamp = aPicture->iTimestamp;

                // Remove picture from transcoder queue
                picture->iLink.Deque();

                // Put video picture to encoder queue
                iEncoderPictureQueue.AddLast(*picture);
                
                // Make request to process this picture
                this->DoRequest();

                if (iInputPictureSize != iOutputPictureSize)
                    {
                    // Return picture to the client
                    iPictureSink->MtroPictureFromTranscoder(aPicture);
                    }
                // Add picture to TR queue unless it's already there
                else if ( iTranscoderTRPictureQueue.IsEmpty() ||
                        ((iTranscoderTRPictureQueue.First() != aPicture) &&
                        (iTranscoderTRPictureQueue.Last() != aPicture)) )
                    {
                    // Keep aPicture until picture is returned by the encoder
                    iTranscoderTRPictureQueue.AddLast(*aPicture);
                    }
                }
            }
            break;

        case EResampling:
            {
            User::Leave(KErrInUse);
            }
            break;

        case EOperationNone:
            {
            User::Leave(KErrNotReady);
            }
            break;

        default:
            {
            User::Leave(KErrNotReady);
            }
            break;
        }

    PRINT((_L("CTRTranscoderImp::SendPictureToTranscoderL(), Out")))
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::MtrdvcoNewBuffer
// New buffer from the Video encoder is available
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::MtrdvcoNewBuffer(CCMRMediaBuffer* aBuffer)
    {
    if (!aBuffer)
        {
        // Should never happend
        PRINT((_L("CTRTranscoderImp::MtrdvcoNewBuffer(), EncoderClient sends invalid data, panic")))
        TRASSERT(0);
        }

    if (!iMediaSink)
        {
        // Should never happend at this point
        PRINT((_L("CTRTranscoderImp::MtrdvcoNewBuffer(), invalid media sink, panic")))
        TRASSERT(0);
        }

    TRAPD(status, iMediaSink->WriteBufferL(aBuffer));
    if (status != KErrNone)
        {
        PRINT((_L("CTRTranscoderImp::MtrdvcoNewBuffer(), WriteBufferL status[%d]"), status))
        iObserver.MtroFatalError(status);
        }
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::ResampleL
// Resamples video picture to new resolution
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::ResampleL(TPtr8& aSrc, TPtr8& aTrg)
    {
    PRINT((_L("CTRTranscoderImp::ResampleL(), In")))

    if ( (iState != ETRInitialized) && (iState != ETRRunning) )
        {
        PRINT((_L("CTRTranscoderImp::ResampleL(), called in wrong state")))
        User::Leave(KErrNotReady);
        }

    if (iMode != EResampling)
        {
        PRINT((_L("CTRTranscoderImp::ResampleL(), supported only in Resampling mode")))
        User::Leave(KErrInUse);
        }

    if (iScaler)
        {
        // SetScaler options
        iScaler->SetScalerOptionsL(aSrc, aTrg, iInputPictureSize, iOutputPictureSize);
        iScaler->Scale();
        }
    else
        {
        PRINT((_L("CTRTranscoderImp::ResampleL(), scaler is not valid")))
        TRASSERT(0);
        }

    PRINT((_L("CTRTranscoderImp::ResampleL(), Out")))
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::StopL
// Stops data processing synchronously
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::StopL()
    {
    PRINT((_L("CTRTranscoderImp::StopL(), In")))

    
    if ( (iState != ETRRunning) && (iState != ETRPaused) && (iState != ETRFatalError) )
        {
        PRINT((_L("CTRTranscoderImp::StopL(), wrong state, leave")))
        User::Leave(KErrNotReady);
        }
        
    switch(iMode)
        {
        case EFullTranscoding:
            {
            // Discard all frames waiting to be sent to encoder
            iEncoderPictureQueue.Reset();
            
            iVideoDecoderClient->StopL();
            iVideoEncoderClient->StopL();
            iState = ETRStopped;
            }
            break;

        case EDecoding:
            {
            iVideoDecoderClient->StopL();
            iState = ETRStopped;
            }
            break;

        case EEncoding:
            {
            iVideoEncoderClient->StopL();
            iState = ETRStopped;
            }
            break;

        case EResampling:
            {
            iState = ETRStopped;
            }
            break;

        case EOperationNone:
            {
            User::Leave(KErrNotReady);
            }
            break;

        default:
            {
            }
            break;
        }
        
    PRINT((_L("CTRTranscoderImp::StopL(), Out")))
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::AsyncStopL
// Stops data processing asynchronously
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::AsyncStopL()
    {
    PRINT((_L("CTRTranscoderImp::AsyncStopL(), Async In")))


    if (iState != ETRRunning)
        {
        PRINT((_L("CTRTranscoderImp::AsyncStopL() Async, wrong state")))
        User::Leave(KErrNotReady);
        }

    switch(iMode)
        {
        case EFullTranscoding:
            {
            iAsyncStop = ETrue;
            iVideoDecoderClient->AsyncStopL();
            }
            break;

        case EDecoding:
            {
            iEncoderStreamEnd = ETrue;
            iVideoDecoderClient->AsyncStopL();
            iState = ETRStopped;
            }
            break;

        case EEncoding:
            {
            iDecoderStreamEnd = ETrue;
            iVideoEncoderClient->AsyncStopL();
            iState = ETRStopped;
            }
            break;

        case EResampling:
            {
            // Just complete asyncStop right here
            iObserver.MtroAsyncStopComplete();
            iState = ETRStopped;
            }
            break;

        case EOperationNone:
            {
            User::Leave(KErrNotReady);
            }
            break;

        default:
            {
            User::Leave(KErrNotReady);
            }
        }
    

    PRINT((_L("CTRTranscoderImp::AsyncStopL(), Async Out")))
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::MtrdvcoEncStreamEnd
// Informs that last data was processed by encoder
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::MtrdvcoEncStreamEnd()
    {
    PRINT((_L("CTRTranscoderImp::MtrdvcoEncStreamEnd()")))
    iEncoderStreamEnd = ETrue;

    if (iDecoderStreamEnd)
        {
        PRINT((_L("CTRTranscoderImp::MtrdvcoEncStreamEnd(), MtroAsyncStopComplete")))
        iObserver.MtroAsyncStopComplete();
        }
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::DecoderStreamEnd
// Informs that last data was processed by decoder
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::MtrdvcoDecStreamEnd()
    {
    PRINT((_L("CTRTranscoderImp::MtrdvcoDecStreamEnd()")))
    TInt status = KErrNone;
    iDecoderStreamEnd = ETrue;

    if (iEncoderStreamEnd)
        {
        PRINT((_L("CTRTranscoderImp::MtrdvcoDecStreamEnd(), MtroAsyncStopComplete")))
        iObserver.MtroAsyncStopComplete();
        return;
        }
        
    if (iAsyncStop)
        {
        // Call now asyncStop for encoder
        TRAP(status, iVideoEncoderClient->AsyncStopL());
        
        if (status != KErrNone)
            {
            // Report error to the client during stop
            PRINT((_L("CTRTranscoderImp::MtrdvcoDecStreamEnd(), EncAsyncStop status[%d]"), status))
            iObserver.MtroFatalError(status);
            }
        
        iState = ETRStopped;
        }
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::MtrdvcoRendererReturnPicture
// Receives picture from renderer
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::MtrdvcoRendererReturnPicture(TVideoPicture* /*aPicture*/)
    {
    }

// -----------------------------------------------------------------------------
// CTRTranscoderImp::MtrdvcoFatalError
// Reports the fatal error to the client
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::MtrdvcoFatalError(TInt aError)
    {
    PRINT((_L("CTRTranscoderImp::MtrdvcoFatalError(), error[%d]"), aError))
    iFatalError = aError;
    iState = ETRFatalError;
    iObserver.MtroFatalError(aError);
    }


//  AA (PV requires to use that, otherwise depends on the decoder hwdevice & decoder itself)
// -----------------------------------------------------------------------------
// CTRTranscoderImp::SetDecoderInitDataL
// Sends init data for decoder
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::SetDecoderInitDataL(TPtrC8& /*aInitData*/)
    {
    // Remove this API in the future; 
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::GetCodingStandardSpecificInitOutputLC
// Retrieve coding options from encoder client
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
HBufC8* CTRTranscoderImp::GetCodingStandardSpecificInitOutputLC()
    {
    if ( iState != ETRInitialized )
        {
        PRINT((_L("CTRTranscoderImp::GetCodingStandardSpecificInitOutputLC(), wrong state, leave")))
        User::Leave(KErrNotReady);
        }
        
    if (iVideoEncoderClient)
        {
        return iVideoEncoderClient->GetCodingStandardSpecificInitOutputLC();
        }
    else
        {
        return NULL;
        }
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::EnablePictureSink
// Enable / Disable picture sink
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::EnablePictureSink(TBool aEnable)
    {
    if (iMode != EFullTranscoding)
        {
        // nothing to do in other mode
        PRINT((_L("CTRTranscoderImp::EnablePictureSink(), nothing to do, wrong mode")))
        return;
        }
        
    if (iPictureSinkClientSetting != aEnable)
        {
        // Setting was changed
        iPictureSinkSettingChanged = ETrue;
        PRINT((_L("CTRTranscoderImp::EnablePictureSink(), EnablePictureSink setting changed[%d]"), aEnable))
        }
        
    iPictureSinkClientSetting = aEnable;
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::EnableEncoder
// Enable / Disable encoder
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::EnableEncoder(TBool aEnable)
    {
    if (iMode != EFullTranscoding)
        {
        // nothing to do in other mode
        PRINT((_L("CTRTranscoderImp::EnableEncoder(), nothing to do, wrong mode")))
        return;
        }

    if (iEncoderEnableClientSetting != aEnable)
        {
        // Setting was changed
        iEncoderEnabledSettingChanged = ETrue;
        PRINT((_L("CTRTranscoderImp::EnableEncoder(), EnableEncoder setting changed[%d]"), aEnable))
        }

    iEncoderEnableClientSetting = aEnable;
    }


// -----------------------------------------------------------------------------
// CTRTranscoderImp::SetRandomAccessPoint
// Sets random access
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::SetRandomAccessPoint()
    {
    if (iMode != EFullTranscoding)
        {
        // nothing to do in other mode
        PRINT((_L("CTRTranscoderImp::EnableEncoder(), nothing to do, wrong mode")))
        return;
        }

    iSetRandomAccessPoint = ETrue;
    }
    
    
// -----------------------------------------------------------------------------
// CTRTranscoderImp::MtrdvcoNewBuffers
// New buffers from encoder
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRTranscoderImp::MtrdvcoNewBuffers()
    {
    PRINT((_L("CTRTranscoderImp::MtrdvcoNewBuffers()")))
    TInt status = KErrNone;
    TTRVideoPicture* pictureFromClient = NULL;
    TVideoPicture* newDecodedPicture = NULL;
    
    
    if (iRealTime)
        {
        // Should never be called in normal data flow or realTime mode, ignore this call!
        PRINT((_L("CTRTranscoderImp::MtrdvcoNewBuffers(), return immediately")))
        return;
        }
        
    // Use this call to initiate data transfer if there are available decoded picture waiting for processing
    if (iWaitPictureFromClient)
        {
        PRINT((_L("CTRTranscoderImp::MtrdvcoNewBuffers(), iWaitPictureFromClient, [0x%x], call SendPictureToTranscoderL()"), iWaitPictureFromClient))
        
        // Simulate call from the client since picture was not processed
        pictureFromClient = iWaitPictureFromClient;
        
        // Reset picture, it can be reinitialized in the following call
        iWaitPictureFromClient = NULL;
        
        TRAP( status, this->SendPictureToTranscoderL(pictureFromClient) );
        
        if (status != KErrNone)
            {
            // Report error to the client
            PRINT((_L("CTRTranscoderImp::MtrdvcoNewBuffers(), status[%d]"), status))
            iObserver.MtroFatalError(status);
            }
            
        return;
        }
        
    if (iWaitNewDecodedPicture)
        {
        PRINT((_L("CTRTranscoderImp::MtrdvcoNewBuffers(), iWaitNewDecodedPicture[0x%x]"), iWaitNewDecodedPicture))
        newDecodedPicture = iWaitNewDecodedPicture;
        
        // Reset newdecoded picture
        iWaitNewDecodedPicture = NULL;
        
        // Simulate new decoded picture call
        this->MtrdvcoNewPicture(newDecodedPicture);
        }
    }
    
// -----------------------------------------------------------------------------
// CTRTranscoderImp::EstimateTranscodeTimeFactorL
// Returns a time estimate of how long it takes to process one second of the input video
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//    
TReal CTRTranscoderImp::EstimateTranscodeTimeFactorL(const TTRVideoFormat& aInput, const TTRVideoFormat& aOutput)
    {
    PRINT((_L("CTRTranscoderImp::EstimateTranscodeTimeFactorL(), In")))
    
    if ( iState == ETRNone )
        {
        PRINT((_L("CTRTranscoderImp::EstimateTranscodeTimeFactorL(), Transcoder is in wrong state")))
        User::Leave(KErrNotReady);
        }
    
    // Get frame rate for output / input video    
    TReal time = 0.0;
    TReal encodeFrameRate = GetFrameRateL();
    TReal decodeFrameRate = 0.0;
    iObserver.MtroSetInputFrameRate(decodeFrameRate);
    
    // Use default frames rates if not set
    if (encodeFrameRate == 0.0) encodeFrameRate = KTRDefaultSrcRate;
    if (decodeFrameRate == 0.0) decodeFrameRate = KTRDefaultSrcRate;
    
    PRINT((_L("CTRTranscoderImp::EstimateTranscodeTimeFactorL(), encode frame rate: %.2f"), encodeFrameRate))
    PRINT((_L("CTRTranscoderImp::EstimateTranscodeTimeFactorL(), decode frame rate: %.2f"), decodeFrameRate))
        
    switch(iMode)
        {
        case EFullTranscoding:
            {
            // Estimate the time for decoding, encoding and possible resampling
            time += iVideoEncoderClient->EstimateEncodeFrameTimeL(aOutput, iOutputCodec) * encodeFrameRate;
            time += iVideoDecoderClient->EstimateDecodeFrameTimeL(aInput, iInputCodec) * decodeFrameRate;
            time += iScaler->EstimateResampleFrameTime(aInput, aOutput) * decodeFrameRate;
            
            break;
            }

        case EDecoding:
            {
            // Estimate the time for decoding and possible resampling
            time += iVideoDecoderClient->EstimateDecodeFrameTimeL(aInput, iInputCodec) * decodeFrameRate;
            time += iScaler->EstimateResampleFrameTime(aInput, aOutput) * decodeFrameRate;
            
            break;
            }

        case EEncoding:
            {
            // Estimate the time for encoding and possible resampling
            time += iVideoEncoderClient->EstimateEncodeFrameTimeL(aOutput, iOutputCodec) * encodeFrameRate;
            time += iScaler->EstimateResampleFrameTime(aInput, aOutput) * encodeFrameRate;
            
            break;
            }

        case EResampling:
            {
            // Estimate the time for resampling
            time += iScaler->EstimateResampleFrameTime(aInput, aOutput) * decodeFrameRate;
            
            break;
            }

        default:
            {
            // Transcoder is not ready
            User::Leave(KErrNotReady);
            break;
            }
        }
        
    PRINT((_L("CTRTranscoderImp::EstimateTranscodeTimeFactorL(), time factor: %.2f"), time))
    
    PRINT((_L("CTRTranscoderImp::EstimateTranscodeTimeFactorL(), Out")))
    
    return time;
    }

// -----------------------------------------------------------------------------
// CTRTranscoderImp::GetMaxFramesInProcessing
// Get max number of frames in processing
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//     
TInt CTRTranscoderImp::GetMaxFramesInProcessing()
    {
    return iMaxFramesInProcessing;
    }
    
// -----------------------------------------------------------------------------
// CTRTranscoderImp::EnablePausing
// Enable / Disable pausing of transcoding if resources are lost
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//  
void CTRTranscoderImp::EnablePausing(TBool aEnable)
    {
    if (iVideoDecoderClient)
        {
        iVideoDecoderClient->EnableResourceObserver(aEnable);
        }
    
    if (iVideoEncoderClient)
        {
        iVideoEncoderClient->EnableResourceObserver(aEnable);
        }
    
    }
    
// -----------------------------------------------------------------------------
// CTRTranscoderImp::::MmvroResourcesLost
// Indicates that a media device has lost its resources
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//     
void CTRTranscoderImp::MtrdvcoResourcesLost(TBool aFromDecoder)
    {
    PRINT((_L("CTRTranscoderImp::MtrdvcoResourcesLost(), In")))
    
    if ( (iState == ETRRunning) || (iState == ETRInitialized) || (iState == ETRStopped) )
        {
        iVideoDecoderClient->Pause();
        iState = ETRPaused;
        
        iDecoderResourceLost = aFromDecoder;
        
        if (!aFromDecoder)
            iVideoEncoderClient->Pause();
        
        // Return decoded picture    
        if (iWaitNewDecodedPicture)
            {
            iVideoDecoderClient->ReturnPicture(iWaitNewDecodedPicture);
            iWaitNewDecodedPicture = NULL;
            }
            
        iObserver.MtroSuspend();   
        }
    
    PRINT((_L("CTRTranscoderImp::MtrdvcoResourcesLost(), Out")))
    }
        

// -----------------------------------------------------------------------------
// CTRTranscoderImp::::MmvroResourcesRestored
// Indicates that a media device has regained its resources
// (other items were commented in a header).
// -----------------------------------------------------------------------------
// 
void CTRTranscoderImp::MtrdvcoResourcesRestored()
    {
    PRINT((_L("CTRTranscoderImp::MtrdvcoResourcesRestored(), In")))
    
    if ( (iState == ETRPaused) || (iState == ETRInitialized) || (iState == ETRStopped) )
        {
        // Clear all events
        if( iNewEvent )
            {
            iNewEvent->Reset();
            iTranscoderEventSrc.AddLast(*iNewEvent);
            iNewEvent = NULL;
            }
            
        while( !iTranscoderEventQueue.IsEmpty() )
            {
            // Get new event
            iNewEvent = iTranscoderEventQueue.First();
            iNewEvent->iLink.Deque();
            
            iNewEvent->Reset();
            iTranscoderEventSrc.AddLast(*iNewEvent);
            iNewEvent = NULL;
            }
            
        if( iAsyncEvent )
            {
            iAsyncEvent->Reset();
            iTranscoderEventSrc.AddLast(*iAsyncEvent);
            iAsyncEvent = NULL;
            }
            
        while( !iTranscoderAsyncEventQueue.IsEmpty() )
            {
            // Get new event
            iAsyncEvent = iTranscoderAsyncEventQueue.First();
            iAsyncEvent->iLink.Deque();
            
            iAsyncEvent->Reset();
            iTranscoderEventSrc.AddLast(*iAsyncEvent);
            iAsyncEvent = NULL;
            }
            
        // Force update of PS / EE settings
        iPictureSinkSettingChanged = ETrue;
        iEncoderEnabledSettingChanged = ETrue;
        
        TRAPD( error, iVideoDecoderClient->ResumeL() );
        if( error != KErrNone ) { }
        
        if (!iDecoderResourceLost)
            {
            iVideoEncoderClient->Resume();            
            }

        iState = ETRRunning;
        
        iObserver.MtroResume();   
        }
    
    PRINT((_L("CTRTranscoderImp::MtrdvcoResourcesRestored(), Out")))
    }

// End of file
