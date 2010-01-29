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



// INCLUDE FILES
#include <devvideoconstants.h>
#include "ctrvideodecoderclient.h"
#include "ctrtranscoder.h"
#include "ctrdevvideoclientobserver.h"
#include "ctrsettings.h"
#include "ctrhwsettings.h"


// MACROS
#define TRASSERT(x) __ASSERT_DEBUG(x, User::Panic(_L("CTRANSCODERVIDEODECODERCLIENT"), -10010))


// CONSTANTS


// ============================ MEMBER FUNCTIONS ===============================

// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::NewL
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CTRVideoDecoderClient* CTRVideoDecoderClient::NewL(MTRDevVideoClientObserver& aObserver)
    {
    PRINT((_L("CTRVideoDecoderClient::NewL(), In")))
    CTRVideoDecoderClient* self = new (ELeave) CTRVideoDecoderClient(aObserver);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();

    PRINT((_L("CTRVideoDecoderClient::NewL(), Out")))
    return self;
    }


// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::CTRVideoDecoderClient
// C++ default constructor can NOT contain any code, that
// might leave.
// -----------------------------------------------------------------------------
//
CTRVideoDecoderClient::CTRVideoDecoderClient(MTRDevVideoClientObserver& aObserver) :
    iObserver(aObserver)
    {
    iDevVideoPlay = NULL;
    iCompresedFormat = NULL;
    iUid = TUid::Null();
    iFallbackUid = TUid::Null();
    iHwDeviceId = THwDeviceId(0);
    iInputBuffer = NULL;
    iCodedBuffer = NULL;
    iDecodedPicture = NULL;
    
    iVideoResourceHandlerCI = NULL;   
    
    iFatalError = KErrNone;
    iDataUnitType = EDuCodedPicture;
    iStop = EFalse;
    iLastTimestamp = -1;
    iAcceleratedCodecSelected = EFalse;
    }


// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::ConstructL
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//
void CTRVideoDecoderClient::ConstructL()
    {
    iDevVideoPlay = CMMFDevVideoPlay::NewL(*this);
    }


// ---------------------------------------------------------
// CTRVideoDecoderClient::~CTRVideoDecoderClient()
// Destructor
// ---------------------------------------------------------
//
CTRVideoDecoderClient::~CTRVideoDecoderClient()
    {
    PRINT((_L("CTRVideoDecoderClient::~CTRVideoDecoderClient(), In")))

    if (iDevVideoPlay)
        {
        delete iDevVideoPlay;
        iDevVideoPlay = NULL;
        }

    iInputBuffer = NULL;
    
    if (iCompresedFormat)
        {
        delete iCompresedFormat;
        }

    PRINT((_L("CTRVideoDecoderClient::~CTRVideoDecoderClient(), Out")))
    }


// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::SupportsCodec
// Checks whether this coded is supported
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TBool CTRVideoDecoderClient::SupportsCodec(const TDesC8& aFormat, const TDesC8& aShortFormat, TInt aUid, TInt aFallbackUid)
    {
    TBool supports = EFalse;
    TBool preferredFound = EFalse;
    TBool fallbackFound = EFalse;

    if (iDevVideoPlay)
        {
        RArray<TUid> decoders;
        
        TRAPD( status, iDevVideoPlay->FindDecodersL(aShortFormat, 0/*aPreProcType*/, decoders, EFalse/*aExactMatch*/) );

        if( status != KErrNone  )
            {
            PRINT((_L("CTRVideoDecoderClient::SupportsCodec(), status[%d]"), status))
            supports = EFalse;
            }
        else if( decoders.Count() <= 0 )
            {
            PRINT((_L("CTRVideoDecoderClient::SupportsCodec(), No decoders found")))
            supports = EFalse;
            }
        else
            {
            
            PRINT((_L("CTRVideoDecoderClient::SupportsCodec(), %d decoders found"), decoders.Count() ))
            
            // Check if any of the found decoders matches with the given Uids
            for( TInt i = 0; i < decoders.Count(); ++i )
                {
                
                PRINT((_L("CTRVideoDecoderClient::SupportsCodec(), found codec 0x%x"), decoders[i].iUid))
                
                if( decoders[i].iUid == aUid )
                    {
                    PRINT((_L("CTRVideoDecoderClient::SupportsCodec(), preferred found")))
                    iUid = decoders[i];
                    preferredFound = ETrue;
                    }
                if( decoders[i].iUid == aFallbackUid )
                    {
                    PRINT((_L("CTRVideoDecoderClient::SupportsCodec(), fallback found")))
                    iFallbackUid = decoders[i];
                    fallbackFound = ETrue;
                    }
                
                if( preferredFound && fallbackFound )
                    {
                    // No need to search anymore
                    break;
                    }
                }
            }

        decoders.Reset();
        decoders.Close();
        }
        
    if( !preferredFound )
        {
        // Preferred decoder was not found => Probably the given decoder Uid is wrong
        PRINT((_L("CTRVideoDecoderClient::SupportsCodec(), No supported decoders found")))
        supports = EFalse;
        }
    else
        {
        PRINT((_L("CTRVideoDecoderClient::SupportsCodec(), Supported decoder found: 0x%x"), iUid.iUid))
        iMimeType = aFormat;
        iShortMimeType = aShortFormat;
        supports = ETrue;
        }

    return supports;
    }


// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::SetCodecParametersL
// Sets codec parameters
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoDecoderClient::SetCodecParametersL(TInt aCodecType, TInt aCodecLevel, const TTRVideoFormat& aInputFormat, 
                                                const TTRVideoFormat& aOutputFormat)
    {
    PRINT((_L("CTRVideoDecoderClient::SetCodecParametersL(), In")))
    TInt status = KErrNone;
    iCodecType = aCodecType;
    iCodecLevel = aCodecLevel;
    iInputFormat = aInputFormat;
    iOutputFormat = aOutputFormat;

    // Input format
    if (!iCompresedFormat)
        {
        iCompresedFormat = CCompressedVideoFormat::NewL( iMimeType );
        }
        
    TRAP( status, iAcceleratedCodecSelected = CheckCodecInfoL(iUid) );
    
    if (status != KErrNone)
        {
        // Try again with the fallback decoder if one exists
        if( (iFallbackUid != TUid::Null()) && (iFallbackUid.iUid != iUid.iUid) )
            {
            TRAP( status, iAcceleratedCodecSelected = CheckCodecInfoL(iFallbackUid) );
            
            if (status != KErrNone)
                {
                PRINT((_L("CTRVideoDecoderClient::SetCodecParametersL(), Failed to get codec info")))
                User::Leave(KErrNotSupported);
                }
                
            PRINT((_L("CTRVideoDecoderClient::SetCodecParametersL(), Reverting to fallback decoder")))
            
            // Fallback ok, take it    
            iUid = iFallbackUid;
            }
        else
            {
            PRINT((_L("CTRVideoDecoderClient::SetCodecParametersL(), No suitable decoders found")))
            User::Leave(KErrNotSupported);
            }
        }
      
    PRINT((_L("CTRVideoDecoderClient::SetCodecParametersL(), Out")))
    }


// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::CheckCodecInfoL
// Checks coded info
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TBool CTRVideoDecoderClient::CheckCodecInfoL(TUid aUid)
    {
    CVideoDecoderInfo* decoderInfo = NULL; // Decoder info for retrieving capabilities
    TInt status = KErrNone;
    TBool accelerated = EFalse;


    // Check decoder
    PRINT((_L("CTRVideoDecoderClient::CheckCodecInfoL(), getting info from [0x%x]"), aUid.iUid ))
    decoderInfo = iDevVideoPlay->VideoDecoderInfoLC( aUid );

    if (!decoderInfo)
        {
        PRINT((_L("CTRVideoDecoderClient::CheckCodecInfoL(), getting info from [0x%x] failed[%d]"), aUid.iUid, status ))
        User::Leave(KErrNotSupported);
        } //  AA skip info check before symbian fix
    else /* if ( !decoderInfo->SupportsFormat(*iCompresedFormat) ) // Check input format
        {
        PRINT((_L("CTRVideoDecoderClient::CheckCodecInfoL(), Input format is not supported")))
        status = KErrNotSupported;
        }
    else */
        {
        // Check max rate for requested image format
        TSize maxSize = decoderInfo->MaxPictureSize();

        if ( (iInputFormat.iSize.iWidth > maxSize.iWidth) || (iInputFormat.iSize.iHeight > maxSize.iHeight) )
            {
            PRINT((_L("CTRVideoDecoderClient::CheckCodecInfoL(), Picture size is not supported")))
            status = KErrNotSupported;
            }
        }
        
    accelerated = decoderInfo->Accelerated();

    // Delete codec info
    CleanupStack::PopAndDestroy(decoderInfo);

    if (status != KErrNone)
        {
        User::Leave(status);
        }

    return accelerated;
    }



// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::InitializeL
// Initializes encoder
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoDecoderClient::InitializeL()
    {
    PRINT((_L("CTRVideoDecoderClient::InitializeL(), In")))
    TUint maxBufferSize = 0;
    TInt status = KErrNone;


    switch(iInputFormat.iDataType)
        {
        case CTRTranscoder::ETRDuCodedPicture:
            {
            iDataUnitType = EDuCodedPicture;
            break;
            }

        case CTRTranscoder::ETRDuVideoSegment:
            {
            iDataUnitType = EDuVideoSegment;
            break;
            }

        default:
            {
            // Should never happend. Decoder does not support uncompressed input format. 
            TRASSERT(0);
            }
        }

    iBufferOptions.iMinNumInputBuffers = KTRDecoderMinNumberOfBuffers;

    // Select decoder first
    this->SelectDecoderL();

    // Set now output format for this device    
    TRAP(status, iDevVideoPlay->SetOutputFormatL(iHwDeviceId, iUncompressedFormat));
    
    // 3. Buffer options
    iBufferOptions.iPreDecodeBufferSize = 0;            // "0" - use default decoder value
    iBufferOptions.iMaxPostDecodeBufferSize = 0;        // No limitations
    iBufferOptions.iPreDecoderBufferPeriod = 0;
    iBufferOptions.iPostDecoderBufferPeriod = 0;
    
    // Check max coded picture size for specified codec level
    switch(iCodecLevel)
        {
        case KTRH263CodecLevel10:
            {
            maxBufferSize = KTRMaxBufferSizeLevel10;
            break;
            }

        case KTRH263CodecLevel20:
            {
            maxBufferSize = KTRMaxBufferSizeLevel20;
            break;
            }

        case KTRH263CodecLevel30:
            {
            maxBufferSize = KTRMaxBufferSizeLevel30;
            break;
            }

        case KTRH263CodecLevel40:
            {
            maxBufferSize = KTRMaxBufferSizeLevel40;
            break;
            }

        case KTRH263CodecLevel50:
            {
            maxBufferSize = KTRMaxBufferSizeLevel50;
            break;
            }

        case KTRH263CodecLevel60:
            {
            maxBufferSize = KTRMaxBufferSizeLevel60;
            break;
            }

        case KTRH263CodecLevel70:
            {
            maxBufferSize = KTRMaxBufferSizeLevel70;
            break;
            }
            
        case KTRH264CodecLevel10:
            {
            maxBufferSize = KTRMaxBufferSizeH264Level10;
            break;
            }
            
        case KTRH264CodecLevel10b:
            {
            maxBufferSize = KTRMaxBufferSizeH264Level10b;
            break;
            }
            
        case KTRH264CodecLevel11:
            {
            maxBufferSize = KTRMaxBufferSizeH264Level11;
            break;
            }
            
        case KTRH264CodecLevel12:
            {
            maxBufferSize = KTRMaxBufferSizeH264Level12;
            break;
            }
            
        case KTRH264CodecLevel13:
            {
            maxBufferSize = KTRMaxBufferSizeH264Level13;
            break;
            }
            
        case KTRH264CodecLevel20:
            {
            maxBufferSize = KTRMaxBufferSizeH264Level20;
            break;
            }
            
        case KTRH264CodecLevel30:
            {
            maxBufferSize = KTRMaxBufferSizeH264Level30;
            break;
            }
            
        case KTRH264CodecLevel31:
            {
            maxBufferSize = KTRMaxBufferSizeH264Level31;
            break;
            }

        case KTRMPEG4CodecLevel0:
            {
            maxBufferSize = KTRMaxBufferSizeLevel0;
            break;
            }
            
        case KTRMPEG4CodecLevel1:
            {
            maxBufferSize = KTRMaxBufferSizeLevel1;
            break;
            }
            
        case KTRMPEG4CodecLevel2:
            {
            maxBufferSize = KTRMaxBufferSizeLevel2;
            break;
            }
            
        case KTRMPEG4CodecLevel3:
            {
            maxBufferSize = KTRMaxBufferSizeLevel3;
            break;
            }

        case KTRMPEG4CodecLevel0b:
            {
            maxBufferSize = KTRMaxBufferSizeLevel0b;
            break;
            }

        case KTRMPEG4CodecLevel4a:
            {
            maxBufferSize = KTRMaxBufferSizeLevel4a;
            break;
            }

        default:
            {
            maxBufferSize = KTRMaxBufferSizeLevel0;
            break;
            }
        }

    iBufferOptions.iMaxInputBufferSize = maxBufferSize;
    PRINT((_L("CTRVideoDecoderClient::InitializeL(), InputBufferSize[%d], NumberOfBuffers[%d]"), 
               iBufferOptions.iMaxInputBufferSize, iBufferOptions.iMinNumInputBuffers ))

    iDevVideoPlay->SetBufferOptionsL(iBufferOptions);
    
    if (iScalingInUse)
        {
        PRINT((_L("CTRVideoDecoderClient::InitializeL(), Enabling scaling")))
        if (iScalingWithDeblocking)
            {
            // Enable scaling with deblocking
            iDevVideoPlay->SetPostProcessTypesL(iHwDeviceId, EPpScale | EPpDeblocking);
            }
        else
            {
            // Deblocking not supported, enable just scaling
            iDevVideoPlay->SetPostProcessTypesL(iHwDeviceId, EPpScale);
            }
        
        iDevVideoPlay->SetScaleOptionsL(iHwDeviceId, iScaledOutputSize, EFalse);
        }

    // Initialize devVideoPlay
    iDevVideoPlay->Initialize();

    PRINT((_L("CTRVideoDecoderClient::InitializeL(), Out")))
    }


// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::SelectDecoderL
// Selects decoder
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoDecoderClient::SelectDecoderL()
    {
    PRINT(( _L("CTRVideoDecoderClient::SelectDecoderL(), In") ))
    TInt status = KErrNone;
    TBool exit = EFalse;
    
    TVideoDataUnitEncapsulation dataUnitEncapsulation = EDuElementaryStream;
    
    // Use generic payload encapsulation for H.264
    if (iCodecType == EH264)
        {
        dataUnitEncapsulation = EDuGenericPayload;
        }

    if (iUid != TUid::Null())
        {
        TRAP( status, iHwDeviceId = iDevVideoPlay->SelectDecoderL(iUid) );
        }
    else
        {
        // Probably the error already exists, if iUid == NULL; 
        status = KErrAlreadyExists;
        }

    while (!exit )
        {
        if (status == KErrNone)
            {
            // To get Output format list devvideoplay requires to define output format first. 
            iDevVideoPlay->SetInputFormatL(iHwDeviceId, *iCompresedFormat, iDataUnitType, dataUnitEncapsulation, ETrue);

            // It's time to check input format support (since the plugin is loaded to the memory)
            iUncompressedFormat.iDataFormat = EYuvRawData;
            
            TUncompressedVideoFormat uncFormat;
            TBool found = EFalse;
            TInt pattern1, pattern2;
            TInt dataLayout;
                    
            switch (iOutputFormat.iDataType)
                {
                case CTRTranscoder::ETRYuvRawData420:
                    {
                    pattern1 = EYuv420Chroma1;
                    pattern2 = EYuv420Chroma2;
                    dataLayout = EYuvDataPlanar;
                    }
                    break;

                case CTRTranscoder::ETRYuvRawData422:
                    {
                    pattern1 = EYuv422Chroma1;
                    pattern2 = EYuv422Chroma2;
                    dataLayout = EYuvDataInterleavedBE;
                    }
                    break;
                
                default:
                    {
                    // set 420 as a default
                    pattern1 = EYuv420Chroma1;
                    pattern2 = EYuv420Chroma2;
                    dataLayout = EYuvDataPlanar;
                    }
                }
                
            RArray<TUncompressedVideoFormat> supportedOutputFormats; 
            TRAP(status, iDevVideoPlay->GetOutputFormatListL( iHwDeviceId, supportedOutputFormats ));
            
            TInt formatCount = 0;
            if (status == KErrNone)
                {
                formatCount = supportedOutputFormats.Count();
                PRINT((_L("CTRVideoDecoderClient::InitializeL(), formatCount[%d]"), formatCount ))
                }
                
            if (formatCount <= 0)
                {
                supportedOutputFormats.Close();
                status = KErrAlreadyExists;
                PRINT((_L("CTRVideoDecoderClient::InitializeL(), There are no supported output formats") ))
                //User::Leave(KErrNotSupported);
                }
            else
                {
                // Check the most important paramers
                for ( TInt i = 0; i < formatCount; i ++ )
                    {
                    uncFormat = supportedOutputFormats[i];
                    PRINT((_L("CTRVideoDecoderClient::InitializeL(), pattern[%d]"), uncFormat.iYuvFormat.iPattern ))
                    
                    if ( (uncFormat.iDataFormat == iUncompressedFormat.iDataFormat) &&
                         (uncFormat.iYuvFormat.iDataLayout == dataLayout) &&
                         ( (uncFormat.iYuvFormat.iPattern == pattern1) || 
                           (uncFormat.iYuvFormat.iPattern == pattern2) ) )
                        {
                        // Assign the rest of parameters
                        iUncompressedFormat = uncFormat;
                        found = ETrue;
                        exit = ETrue;
                        supportedOutputFormats.Close();
                        break;
                        }
                    }

                if (!found)
                    {
                    supportedOutputFormats.Close();
                    status = KErrAlreadyExists;
                    PRINT((_L("CTRVideoDecoderClient::SelectDecoderL(), Supported format was not found") ))
                    //User::Leave(KErrNotSupported);
                    }
                }
            
            }
        else
            {
            if (iScalingInUse)
                {
                // We can't revert to fallback decoder here since scaling has been taken into use
                // and we can't check here if the fallback decoder supports scaling nor
                // disable scaling if it's not supported
                PRINT((_L("CTRVideoDecoderClient::SelectDecoderL(), Failed to select decoder")))
                User::Leave(KErrNotSupported);
                }
                
            // Try again with the fallback decoder if one exists
            if( (iFallbackUid != TUid::Null()) && (iFallbackUid.iUid != iUid.iUid) )
                {
                PRINT((_L("CTRVideoDecoderClient::SelectEncoderL(), Reverting to fallback decoder")))
                iUid = iFallbackUid;
                }
            else
                {
                PRINT((_L("CTRVideoDecoderClient::SelectDecoderL(), Failed to select decoder")))
                User::Leave(KErrNotSupported);
                }
                
            TRAP( status, iAcceleratedCodecSelected = CheckCodecInfoL(iUid) );
            
            if (status != KErrNone)
                {
                PRINT((_L("CTRVideoDecoderClient::SelectDecoderL(), Failed to get codec info")))
                User::Leave(KErrNotSupported);
                }
              
            TRAP(status, iHwDeviceId = iDevVideoPlay->SelectDecoderL(iUid));
            
            if (status != KErrNone)
                {
                PRINT((_L("CTRVideoDecoderClient::SelectDecoderL(), Failed to select decoder")))
                User::Leave(KErrNotSupported);
                }
            }
        }

    PRINT((_L("CTRVideoDecoderClient::SelectDecoderL(), Out")))
    }


// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::MdvpoInitComplete
// Notifies for initialization complete with init status
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoDecoderClient::MdvpoInitComplete(TInt aError)
    {
    if ((aError == KErrHardwareNotAvailable) || (aError == KErrNotSupported))
        {
        PRINT((_L("CTRVideoDecoderClient::MdvpoInitComplete(), Error in initialization")))
        
        // Map both error codes to the same
        aError = KErrNotSupported;
        
        // Try again with the fallback decoder if one exists
        while( (iFallbackUid != TUid::Null()) && (iFallbackUid.iUid != iUid.iUid) )
            {
            PRINT((_L("CTRVideoDecoderClient::MdvpoInitComplete(), Reverting to fallback decoder")))
            
            iUid = iFallbackUid;
            
            // Devvideo must be recreated from scratch
            if (iDevVideoPlay)
                {
                delete iDevVideoPlay;
                iDevVideoPlay = NULL;
                }
            
            TRAPD( status, iDevVideoPlay = CMMFDevVideoPlay::NewL(*this) );
            if (status != KErrNone)
                {
                // Something went wrong, let CTRTranscoderImp handle the error
                PRINT((_L("CTRVideoDecoderClient::MdvpoInitComplete(), Failed to create DevVideoPlay")))
                break;
                }
                
            TRAP( status, iAcceleratedCodecSelected = CheckCodecInfoL(iUid) );
            if (status != KErrNone)
                {
                // Fallback decoder can not be used, let CTRTranscoderImp handle the error
                PRINT((_L("CTRVideoDecoderClient::MdvpoInitComplete(), Failed to get codec info")))
                break;
                }
            
            // We are now ready to reinitialize the decoder, let CTRTranscoderImp do it    
            aError = KErrHardwareNotAvailable;
            break;
            }
        }
    
    iObserver.MtrdvcoDecInitializeComplete(aError);
    }


// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::StartL
// Starts decoding
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoDecoderClient::StartL()
    {
    PRINT((_L("CTRVideoDecoderClient::StartL(), In")))

    // Start decoding
    if (iFatalError == KErrNone)
        {
        iDevVideoPlay->Start();
        }

    if (!iInputBuffer)
        {
        // Get buffer from the decoder to fill
        iInputBuffer = iDevVideoPlay->GetBufferL(iBufferOptions.iMaxInputBufferSize);
        }
    
    // Reset iStop    
    iStop = EFalse;
    iPause = EFalse;
    
    // Reset ts monitor
    iLastTimestamp = -1;

    PRINT((_L("CTRVideoDecoderClient::StartL(), Out")))
    }

// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::Pause
// Pauses decoding
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoDecoderClient::Pause()
    {
    PRINT((_L("CTRVideoDecoderClient::Pause(), In")))

    // Pause decoding
    iDevVideoPlay->Pause();
    
    // Return coded buffer to client since it can not be send to decoder now
    if (iCodedBuffer)
        {
        CCMRMediaBuffer* codedBuffer = iCodedBuffer;
        
        // Reset buffer ptr
        iCodedBuffer = NULL;
        
        iObserver.MtrdvcoReturnCodedBuffer(codedBuffer);
        }
    
    // Get all pictures from devvideoplay and return them to decoder
    TVideoPicture* picture = NULL;    
    TRAPD(status, picture = iDevVideoPlay->NextPictureL());
    
    while ((picture != NULL) && (status == KErrNone))
        {
        PRINT((_L("CTRVideoDecoderClient::Pause(), Sending picture [0x%x] back to decoder"), picture))
        
        iDevVideoPlay->ReturnPicture(picture);
        picture = NULL;
        
        TRAP(status, picture = iDevVideoPlay->NextPictureL());
        }
    
    // Input buffer is not valid anymore   
    iInputBuffer = NULL;    
        
    iPause = ETrue;
    
    PRINT((_L("CTRVideoDecoderClient::Pause(), Out")))
    }

// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::ResumeL
// Resumes decoding
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoDecoderClient::ResumeL()
    {
    PRINT((_L("CTRVideoDecoderClient::ResumeL(), In")))

    // Start decoding
    if (iFatalError == KErrNone)
        {
        iDevVideoPlay->Resume();
        }

    if (!iInputBuffer)
        {
        // Get buffer from the decoder to fill
        iInputBuffer = iDevVideoPlay->GetBufferL(iBufferOptions.iMaxInputBufferSize);
        }
    
    // Reset ts monitor
    iLastTimestamp = -1;
    
    iPause = EFalse;

    PRINT((_L("CTRVideoDecoderClient::ResumeL(), Out")))
    }


// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::MdvpoNewBuffers()
// New buffers are available
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoDecoderClient::MdvpoNewBuffers()
    {
    TInt status = KErrNone;


    if (iStop)
        {
        PRINT((_L("CTRVideoDecoderClient::MdvpoNewBuffers(), Stop was already called, nothing to do")))
        return;
        }

    // One or more new empty input buffers are available
    if (!iInputBuffer)
        {
        // Get buffer from the decoder to fill
        TRAP(status, iInputBuffer = iDevVideoPlay->GetBufferL(iBufferOptions.iMaxInputBufferSize));
        
        if (status != KErrNone)
            {
            PRINT((_L("CTRVideoDecoderClient::MdvpoNewBuffers(), GetBufferL status[%d]"), status))
            iObserver.MtrdvcoFatalError(status);
            return;
            }

        if (!iInputBuffer)
            {
            PRINT((_L("CTRVideoDecoderClient::MdvpoNewBuffers(), There are available buffer, but decoder returned NULL")))
            
            // Report an error or wait for the next MdvpoNewBuffers ?: Wait, GetBufferL is called when client send new coded buffer. 
            //iObserver.MtrdvcoFatalError(KErrAlreadyExists);
            return;
            }
        }

    if (iCodedBuffer)
        {
        CCMRMediaBuffer* codedBuffer = iCodedBuffer;
        
        // Reset buffer ptr
        iCodedBuffer = NULL;
        
        // Send coded buffer, since the client has already done request
        TRAP(status, this->SendBufferL(codedBuffer));
        if (status != KErrNone)
            {
            PRINT((_L("CTRVideoDecoderClient::MdvpoNewBuffers(), Send buffer error[%d]"), status))
            iObserver.MtrdvcoFatalError(status);
            return;
            }
        }
    }


// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::WriteCodedBufferL
// Writes coded data to decoder
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoDecoderClient::WriteCodedBufferL(CCMRMediaBuffer* aBuffer)
    {
    PRINT((_L("CTRVideoDecoderClient::WriteCodedBufferL(), In")))
    CCMRMediaBuffer::TBufferType bufferType;
    
    if (!aBuffer)
        {
        PRINT((_L("CTRVideoDecoderClient::WriteCodedBufferL(), Input buffer is invalid, Leave")))
        User::Leave(KErrArgument);
        }

    if (iFatalError != KErrNone)
        {
        PRINT((_L("CTRVideoDecoderClient::WriteCodedBufferL(), FatalError was reported by decoder")))
        
        // Return coded buffer
        iObserver.MtrdvcoReturnCodedBuffer(aBuffer);
        return;
        }
    
    TTimeIntervalMicroSeconds ts = aBuffer->TimeStamp();
        
    if ( ts <= iLastTimestamp)
        {
        // Prevent propagation of the error now
        PRINT((_L("CTRVideoDecoderClient::WriteCodedBufferL(), Client sends invalid data (ts field), Leave")))
        User::Leave(KErrArgument);
        }
    else
        {
        iLastTimestamp = ts;
        }
    
    if (aBuffer->BufferSize() <= 0)
        {
        PRINT((_L("CTRVideoDecoderClient::WriteCodedBufferL(), Input data buffer is invalid (empty), Leave")))
        User::Leave(KErrArgument);
        }
        
    bufferType = aBuffer->Type();
        
    if ( ( bufferType != CCMRMediaBuffer::EVideoH263 ) && 
         ( bufferType != CCMRMediaBuffer::EVideoMPEG4 ) )   // : Add H264
        {
        PRINT((_L("CTRVideoDecoderClient::WriteCodedBufferL(), [%d] This data type is not supported, Leave"), aBuffer->Type() ))
        User::Leave(KErrNotSupported);
        }

    if (!iInputBuffer)
        {
        // Request new empty buffer
        iInputBuffer = iDevVideoPlay->GetBufferL(iBufferOptions.iMaxInputBufferSize);
        }

    if (iInputBuffer)
        {
        this->SendBufferL(aBuffer);
        }
    else
        {
        iCodedBuffer = aBuffer;
        }

    PRINT((_L("CTRVideoDecoderClient::WriteCodedBufferL(), Out")))
    }


// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::SendBufferL
// Sends buffer to decoder
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoDecoderClient::SendBufferL(CCMRMediaBuffer* aBuffer)
    {
    PRINT((_L("CTRVideoDecoderClient::SendBufferL(), In")))
    
    if (iStop)
        {
        PRINT((_L("CTRVideoDecoderClient::SendBufferL(), Stop was already called, nothing to do, out")))
        iObserver.MtrdvcoReturnCodedBuffer(aBuffer);
        return;
        }

    PRINT((_L("CTRVideoDecoderClient::SendBufferL(), iInputBuffer[%d], aBuffer[%d]"), iInputBuffer->iData.MaxLength(), 
               aBuffer->BufferSize() ))

    if ( iInputBuffer->iData.MaxLength() < aBuffer->BufferSize() )
        {
        PRINT((_L("CTRVideoDecoderClient::SendBufferL(), buffer length exceeds max length")))
        User::Leave(KErrOverflow);
        }

    iInputBuffer->iData.Copy( aBuffer->Data().Ptr(), aBuffer->BufferSize() );
    iInputBuffer->iData.SetLength( aBuffer->BufferSize() );

    // Data unit presentation timestamp. Valid if EPresentationTimestamp is set in the options. 
    // If the input bitstream does not contain timestamp information, this field should be valid, 
    // otherwise pictures cannot be displayed at the correct time. If the input bitstream contains 
    // timestamp information (such as the TR syntax element of H.263 bitstreams) and valid 
    // iPresentationTimestamp is provided, the value of iPresentationTimestamp is used in playback.    
    iInputBuffer->iOptions = TVideoInputBuffer::EPresentationTimestamp;
    iInputBuffer->iPresentationTimestamp = aBuffer->TimeStamp();
    /*Other data: TBC*/
    
    TVideoInputBuffer* inputBuffer = iInputBuffer;
    
    // Reset InputBuffer ptr
    iInputBuffer = NULL;

    // Write data to decoder
    iDevVideoPlay->WriteCodedDataL(inputBuffer);

    //  return buffer only after it's writtent to decoder (client could write next buffer synchronously from observer call)
    // Return buffer to the client immediately after copying
    iObserver.MtrdvcoReturnCodedBuffer(aBuffer);

    PRINT((_L("CTRVideoDecoderClient::SendBufferL(), Out")))
    }


// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::MdvpoNewPictures
// New decoded pictures available from decoder
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoDecoderClient::MdvpoNewPictures()
    {
    PRINT((_L("CTRVideoDecoderClient::MdvpoNewPictures(), In")))
    
    TInt status = KErrNone;

    // 1 or more decoded pictures are available
    if (!iDecodedPicture)
        {
        // Get new picture
        TRAP(status, iDecodedPicture = iDevVideoPlay->NextPictureL());

        if (status != KErrNone)
            {
            PRINT((_L("CTRVideoDecoderClient::MdvpoNewPictures(), NextPicture status[%d]"), status))
            iObserver.MtrdvcoFatalError(status);
            return;
            }

        if (!iDecodedPicture)
            {
            // Error: DevVideo notified of new buffers, but returns NULL
            PRINT((_L("CTRVideoDecoderClient::MdvpoNewPictures(), DevVideo notified of new buffers, but returns NULL")))
            iObserver.MtrdvcoFatalError(KErrAlreadyExists);
            return;
            }

        // Send new picture to the client
        iObserver.MtrdvcoNewPicture(iDecodedPicture);
        }
    else
        {
        // Previous picture still was not returned by the client, nothing to do. 
        //  SetActive();  // ???
        }
        
    PRINT((_L("CTRVideoDecoderClient::MdvpoNewPictures(), Out")))
    }


// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::ReturnPicture
// Returns picture 
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoDecoderClient::ReturnPicture(TVideoPicture* aPicture)
    {
    PRINT((_L("CTRVideoDecoderClient::ReturnPicture(), In")))
    TInt status = KErrNone;


    iDevVideoPlay->ReturnPicture(aPicture);

    // Reset decoded picture
    iDecodedPicture = NULL;
    
    if (iPause)
        {
        // Nothing else to do when paused
        PRINT((_L("CTRVideoDecoderClient::ReturnPicture(), Out")))
        return;
        }

    TRAP(status, iDecodedPicture = iDevVideoPlay->NextPictureL());

    if (status != KErrNone)
        {
        PRINT((_L("CTRVideoDecoderClient::ReturnPicture(), NextPicture status[%d]"), status))
        iObserver.MtrdvcoFatalError(status);
        return;
        }

    if (iDecodedPicture)
        {
        // Send new picture to the client
        iObserver.MtrdvcoNewPicture(iDecodedPicture);
        }

    PRINT((_L("CTRVideoDecoderClient::ReturnPicture(), Out")))
    }


// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::StopL
// Stops decoding synchronously
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoDecoderClient::StopL()
    {
    PRINT((_L("CTRVideoDecoderClient::StopL(), In")))
    
    iStop = ETrue;
    iPause = EFalse;

    if (iFatalError == KErrNone)
        {
        iDevVideoPlay->Stop();
        }
        
    PRINT((_L("CTRVideoDecoderClient::StopL(), Out")))
    }


// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::AsyncStopL
// Stops decoding asynchronously
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoDecoderClient::AsyncStopL()
    {
    PRINT((_L("CTRVideoDecoderClient::StopL(), Async In")))

    if (iFatalError == KErrNone)
        {
        iDevVideoPlay->InputEnd();
        }
        
    iStop = ETrue;

    PRINT((_L("CTRVideoDecoderClient::StopL(), Async Out")))
    }


// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::MdvpoStreamEnd
// Indicates when stream end is reached
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoDecoderClient::MdvpoStreamEnd()
    {
    PRINT((_L("CTRVideoDecoderClient::MdvpoStreamEnd()")))
    iObserver.MtrdvcoDecStreamEnd();
    }

// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::MdvpoReturnPicture
// Returns a used input video picture back to the caller. The picture memory can be re-used or freed (only relevant to postprocessor)
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoDecoderClient::MdvpoReturnPicture(TVideoPicture* /*aPicture*/)
    {
    PRINT((_L("CTRVideoDecoderClient::MdvpoReturnPicture()")))
    }


// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::MdvpoSupplementalInformation
// Sends SupplementalInformation
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoDecoderClient::MdvpoSupplementalInformation(const TDesC8& /*aData*/, 
                                                         const TTimeIntervalMicroSeconds& /*aTimestamp*/, 
                                                         const TPictureId& /*aPictureId*/)
    {
    PRINT((_L("CTRVideoDecoderClient::MdvpoSupplementalInformation()")))
    }


// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::MdvpoPictureLoss
// Back channel information from the decoder, indicating a picture loss without specifying the lost picture
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoDecoderClient::MdvpoPictureLoss()
    {
    PRINT((_L("CTRVideoDecoderClient::MdvpoPictureLoss(), report an error")))
    iObserver.MtrdvcoFatalError(KErrAbort);
    }


// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::MdvpoPictureLoss
// Back channel information from the decoder, indicating the pictures that have been lost
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoDecoderClient::MdvpoPictureLoss(const TArray< TPictureId >& /*aPictures*/)
    {
    PRINT((_L("CTRVideoDecoderClient::MdvpoPictureLoss(), pictureId: report an error")))
    iObserver.MtrdvcoFatalError(KErrAbort);
    }


// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::MdvpoSliceLoss
// Reports that slice is lost
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoDecoderClient::MdvpoSliceLoss(TUint /*aFirstMacroblock*/, TUint /*aNumMacroblocks*/, 
                                           const TPictureId& /*aPicture*/)
    {
    PRINT((_L("CTRVideoDecoderClient::MdvpoSliceLoss()")))
    // This error is not considered a s fatal for decoder or application, nothing to do
    }


// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::MdvpoReferencePictureSelection
// Back channel information from the decoder, indicating a reference picture selection request.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoDecoderClient::MdvpoReferencePictureSelection(const TDesC8& /*aSelectionData*/)
    {
    PRINT((_L("CTRVideoDecoderClient::MdvpoReferencePictureSelection()")))
    }


// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::MdvpoTimedSnapshotComplete
// Called when a timed snapshot request has been completed. 
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoDecoderClient::MdvpoTimedSnapshotComplete(TInt /*aError*/, TPictureData* /*aPictureData*/, 
                                                       const TTimeIntervalMicroSeconds& /*aPresentationTimestamp*/, 
                                                       const TPictureId& /*aPictureId*/)
    {
    PRINT((_L("CTRVideoDecoderClient::MdvpoTimedSnapshotComplete()")))
    }


// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::MdvpoFatalError
// Reports the fatal error to the client
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoDecoderClient::MdvpoFatalError(TInt aError)
    {
    PRINT((_L("CTRVideoDecoderClient::MdvpoFatalError(), error[%d]"), aError))
    iFatalError = aError;
    iObserver.MtrdvcoFatalError(iFatalError);
    }


// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::EstimateDecodeFrameTimeL
// Returns a time estimate on long it takes to decode one frame 
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//    
TReal CTRVideoDecoderClient::EstimateDecodeFrameTimeL(const TTRVideoFormat& aInput, TInt aCodecType)
    {
    if (iUid == TUid::Null())
        {
        PRINT((_L("CTRVideoDecoderClient::EstimateDecodeFrameTimeL(), no decoder selected yet")))
        User::Leave(KErrNotReady);
        }
    
    TReal time = 0.0;    
    
    // Select the predefined constant using the current settings
    if (aCodecType == EH263)
        {
        time = iAcceleratedCodecSelected ? KTRDecodeTimeFactorH263HW : KTRDecodeTimeFactorH263SW;
        }
    else if (aCodecType == EH264)
        {
        time = iAcceleratedCodecSelected ? KTRDecodeTimeFactorH264HW : KTRDecodeTimeFactorH264SW;
        }
    else
        {
        time = iAcceleratedCodecSelected ? KTRDecodeTimeFactorMPEG4HW : KTRDecodeTimeFactorMPEG4SW;
        }
    
    // Multiply the time by the resolution of the input frame    
    time *= static_cast<TReal>(aInput.iSize.iWidth + aInput.iSize.iHeight) * KTRTimeFactorScale;
    
    PRINT((_L("CTRVideoDecoderClient::EstimateDecodeFrameTimeL(), decode frame time: %.2f"), time))
        
    return time;
    }

// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::SetDecoderScaling
// Checks if decoder supports scaling and enables scaling if supported
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//    
TBool CTRVideoDecoderClient::SetDecoderScaling(TSize& aInputSize, TSize& aOutputSize)
    {
    PRINT((_L("CTRVideoDecoderClient::SetDecoderScaling(), In")))
    
    CPostProcessorInfo* ppInfo = NULL;
    TBool scalingSupported = EFalse;
    
    // Check that the given sizes are valid
    if( (aInputSize.iWidth == 0) || (aInputSize.iHeight == 0) ||
        (aOutputSize.iWidth == 0) || (aOutputSize.iHeight == 0) )
        {
        PRINT((_L("CTRVideoDecoderClient::SetDecoderScaling(), invalid input/output size")))
        return EFalse;
        }
        
    iScalingInUse = EFalse;
    iScalingWithDeblocking = EFalse;
    
    if( aInputSize == aOutputSize )
        {
        PRINT((_L("CTRVideoDecoderClient::SetDecoderScaling(), scaling disabled")))
        return EFalse;
        }
       
    // Get post processor info
    TRAPD( status, ppInfo = iDevVideoPlay->PostProcessorInfoLC( iUid ); CleanupStack::Pop( ppInfo ) );

    if( (status != KErrNone) || !ppInfo )
        {
        PRINT((_L("CTRVideoDecoderClient::SetDecoderScaling(), getting info from [0x%x] failed"), iUid.iUid ))
        return EFalse;
        }
        
    if( ppInfo->SupportsArbitraryScaling() )
        {
        PRINT((_L("CTRVideoDecoderClient::SetDecoderScaling(), arbitrary scaling supported")))
        scalingSupported = ETrue;
        }
    else if( (aInputSize.iWidth * aOutputSize.iHeight) != (aInputSize.iHeight * aOutputSize.iWidth) )
        {
        // Aspect ratio needs to be changed but decoder does not support arbitrary scaling => not supported
        scalingSupported = EFalse;
        }
    else
        {        
        RArray<TScaleFactor> scaleFactors = ppInfo->SupportedScaleFactors();
        
        for( TInt i = 0; i < scaleFactors.Count(); ++i )
            {
            if( (aInputSize.iWidth * scaleFactors[i].iScaleNum) == (aOutputSize.iWidth * scaleFactors[i].iScaleDenom) )
                {
                PRINT((_L("CTRVideoDecoderClient::SetDecoderScaling(), supported scale factors found")))
                scalingSupported = ETrue;
                break;
                }
            }
        }
    
    if( scalingSupported )
        {        
        if( ppInfo->SupportsCombination( EPpScale | EPpDeblocking ) )
            {
            // Deblocking should be used with scaling if supported
            PRINT((_L("CTRVideoDecoderClient::SetDecoderScaling(), deblocking supported")))
            iScalingWithDeblocking = ETrue;
            }

        iScalingInUse = ETrue;
        iScaledOutputSize = aOutputSize;
        }

    // Delete codec info
    delete ppInfo;
    
    PRINT((_L("CTRVideoDecoderClient::SetDecoderScaling(), Out")))
    
    return scalingSupported;
    }

// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::EnableResourceObserver
// Enable / Disable resourece observer
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//     
void CTRVideoDecoderClient::EnableResourceObserver(TBool aEnable)
    {
    PRINT((_L("CTRVideoDecoderClient::EnableResourceObserver(), In")))
    
    iVideoResourceHandlerCI = (MMmfVideoResourceHandler*)iDevVideoPlay->CustomInterface( iHwDeviceId, KUidMmfVideoResourceManagement );
    PRINT((_L("CTRVideoDecoderClient::EnableResourceObserver(), iVideoResourceHandlerCI[0x%x]"), iVideoResourceHandlerCI))

    if (iVideoResourceHandlerCI)
        {
        if (aEnable)
            {
            iVideoResourceHandlerCI->MmvrhSetObserver(this);
            PRINT((_L("CTRVideoDecoderClient::EnableResourceObserver(), Enabled")))
            }
        else
            {
            iVideoResourceHandlerCI->MmvrhSetObserver(NULL);
            }
        }

    PRINT((_L("CTRVideoDecoderClient::EnableResourceObserver(), Out")))
    }

// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::MmvroResourcesLost
// Indicates that a media device has lost its resources
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//     
void CTRVideoDecoderClient::MmvroResourcesLost(TUid /*aMediaDevice*/)
    {
    iObserver.MtrdvcoResourcesLost(ETrue);
    }
        

// -----------------------------------------------------------------------------
// CTRVideoDecoderClient::MmvroResourcesRestored
// Indicates that a media device has regained its resources
// (other items were commented in a header).
// -----------------------------------------------------------------------------
// 
void CTRVideoDecoderClient::MmvroResourcesRestored(TUid /*aMediaDevice*/)
    {
    iObserver.MtrdvcoResourcesRestored();
    }


// End of file
