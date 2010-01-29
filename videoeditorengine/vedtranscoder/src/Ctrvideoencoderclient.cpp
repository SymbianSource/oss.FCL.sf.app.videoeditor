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
* Video encoder client.
*
*/



// INCLUDE FILES
#include <devvideorecord.h>
#include <devvideobase.h>
#include <devvideoconstants.h>
#include <H263.h>
#include <AVC.h>
#include <Mpeg4Visual.h>
#include "ctrtranscoder.h"
#include "ctrvideoencoderclient.h"
#include "ctrdevvideoclientobserver.h"
#include "ctrsettings.h"
#include "ctrhwsettings.h"


// MACROS
#define TRASSERT(x) __ASSERT_DEBUG(x, User::Panic(_L("CTRANSCODERVIDEOENCODERCLIENT"), -10000))


// CONSTANTS


// ============================ MEMBER FUNCTIONS ===============================

// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::NewL
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CTRVideoEncoderClient* CTRVideoEncoderClient::NewL(MTRDevVideoClientObserver& aObserver)
    {
    PRINT((_L("CTRVideoEncoderClient::NewL(), In")))
    CTRVideoEncoderClient* self = new (ELeave) CTRVideoEncoderClient(aObserver);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();

    PRINT((_L("CTRVideoEncoderClient::NewL(), Out")))
    return self;
    }


// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::CTRVideoEncoderClient
// C++ default constructor can NOT contain any code, that
// might leave.
// -----------------------------------------------------------------------------
//
CTRVideoEncoderClient::CTRVideoEncoderClient(MTRDevVideoClientObserver& aObserver) :
    iObserver(aObserver)
    {
    iDevVideoRecord = NULL;
    iOutputMediaBuffer = NULL;
    iCompresedFormat = NULL;
    iUid = TUid::Null();
    iFallbackUid = TUid::Null();
    iRealTime = EFalse;
    iState = ETRNone;
    iCodingOptions.iSyncIntervalInPicture = 0;
    iCodingOptions.iHeaderExtension = 0;
    iCodingOptions.iDataPartitioning = EFalse;
    iCodingOptions.iReversibleVLC = EFalse;
    iSrcRate = 0.0;
    iVolHeaderSent = EFalse;
    iRemoveHeader = EFalse;
    iVolLength = 0;
    iRateOptions.iControl = EBrControlStream;
    iRateOptions.iPictureRate = KTRTargetFrameRateDefault;
    iRateOptions.iBitrate = KTRMaxBitRateH263Level10;
    iBitRateSetting = EFalse;
    iFatalError = KErrNone;
    iErrorRate = 0.0;
    iVolHeader = KNullDesC8;
    iVideoBufferManagementCI = NULL;
    iLastTimestamp = -1;
    iAcceleratedCodecSelected = EFalse;
    iSetRandomAccessPoint = EFalse;
    iNumH264SPSPPS = 0;
    }


// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::ConstructL
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//
void CTRVideoEncoderClient::ConstructL()
    {
    iDevVideoRecord = CMMFDevVideoRecord::NewL(*this);
    iOutputMediaBuffer = new(ELeave)CCMRMediaBuffer;
    }


// ---------------------------------------------------------
// CTRVideoEncoderClient::~CTRVideoEncoderClient()
// Destructor
// ---------------------------------------------------------
//
CTRVideoEncoderClient::~CTRVideoEncoderClient()
    {
    PRINT((_L("CTRVideoEncoderClient::~CTRVideoEncoderClient(), In")))

    if (iDevVideoRecord)
        {
        delete iDevVideoRecord;
        iDevVideoRecord = NULL;
        }

    if (iCompresedFormat)
        {
        delete iCompresedFormat;
        }

    if (iOutputMediaBuffer)
        {
        delete iOutputMediaBuffer;
        }
        
    PRINT((_L("CTRVideoEncoderClient::~CTRVideoEncoderClient(), Out")))
    }


// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::SupportsInputFormat
// Checks whether given input format is supported
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TBool CTRVideoEncoderClient::SupportsCodec(const TDesC8& aFormat, const TDesC8& aShortFormat, TInt aUid, TInt aFallbackUid)
    {
    TBool supports = EFalse;
    TBool preferredFound = EFalse;
    TBool fallbackFound = EFalse;
    
    if (iDevVideoRecord)
        {
        RArray<TUid> encoders;
        
        TRAPD( status, iDevVideoRecord->FindEncodersL(aShortFormat, 0/*aPreProcType*/, encoders, EFalse/*aExactMatch*/) );
        
        if( status != KErrNone  )
            {
            PRINT((_L("CTRVideoEncoderClient::SupportsCodec(), status[%d]"), status))
            supports = EFalse;
            }
        else if( encoders.Count() <= 0 )
            {
            PRINT((_L("CTRVideoEncoderClient::SupportsCodec(), No encoders found")))
            supports = EFalse;
            }
        else
            {
            
            PRINT((_L("CTRVideoEncoderClient::SupportsCodec(), %d encoders found"), encoders.Count() ))
            
            // Check if any of the found encoders matches with the given Uids
            for( TInt i = 0; i < encoders.Count(); ++i )
                {
                
                PRINT((_L("CTRVideoEncoderClient::SupportsCodec(), found codec 0x%x"), encoders[i].iUid))
                
                if( encoders[i].iUid == aUid )
                    {
                    PRINT((_L("CTRVideoEncoderClient::SupportsCodec(), preferred found")))
                    iUid = encoders[i];
                    preferredFound = ETrue;
                    }
                if( encoders[i].iUid == aFallbackUid )
                    {
                    PRINT((_L("CTRVideoEncoderClient::SupportsCodec(), fallback found")))
                    iFallbackUid = encoders[i];
                    fallbackFound = ETrue;
                    }
                
                if( preferredFound && fallbackFound )
                    {
                    // No need to search anymore
                    break;
                    }
                }
            }

        encoders.Reset();
        encoders.Close();
        }
        
#if !( defined (__WINS__) || defined (__WINSCW__) )
    if( !preferredFound )
#else
    if( !preferredFound && !fallbackFound )
#endif
        {
        // Preferred encoder was not found => Probably the given encoder Uid is wrong
        PRINT((_L("CTRVideoEncoderClient::SupportsCodec(), No supported encoders found")))
        supports = EFalse;
        }
    else
        {
        PRINT((_L("CTRVideoEncoderClient::SupportsCodec(), Supported encoder found: 0x%x"), iUid.iUid))
        iMimeType = aFormat;
        iShortMimeType = aShortFormat;
        supports = ETrue;
        }

    return supports;
    }


// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::SetCodecParametersL
// Sets codec parameters
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoEncoderClient::SetCodecParametersL(TInt aCodecType, TInt aCodecLevel, const TTRVideoFormat& aInputFormat, 
                                                const TTRVideoFormat& aOutputFormat)
    {
    PRINT((_L("CTRVideoEncoderClient::SetCodecParametersL(), In")))
    TInt status = KErrNone;
    iCodecType = aCodecType;
    iCodecLevel = aCodecLevel;
    iPictureSize = aOutputFormat.iSize;
    iOutputDataType = aOutputFormat.iDataType;
    
    // RAW YUV
    iUncompressedFormat.iDataFormat = EYuvRawData;

    // U & V samples are taken from the middle of four luminance samples
    // (as specified in H.263 spec)
    switch ( aInputFormat.iDataType )
        {
        case CTRTranscoder::ETRYuvRawData420:
            {
            iUncompressedFormat.iYuvFormat.iPattern = EYuv420Chroma2;
            break;
            }

        case CTRTranscoder::ETRYuvRawData422:
            {
            iUncompressedFormat.iYuvFormat.iPattern = EYuv422Chroma2;
            break;
            }

        default:
            {
            iUncompressedFormat.iYuvFormat.iPattern = EYuv420Chroma2;
            break;
            }
        }

    // Output format
    if (!iCompresedFormat)
        {
        iCompresedFormat = CCompressedVideoFormat::NewL( iMimeType );
        }
        
    TRAP( status, iAcceleratedCodecSelected = CheckCodecInfoL(iUid) );
    
    if (status != KErrNone)
        {
        // Try again with the fallback encoder if one exists
        if( (iFallbackUid != TUid::Null()) && (iFallbackUid.iUid != iUid.iUid) )
            {
            TRAP( status, iAcceleratedCodecSelected = CheckCodecInfoL(iFallbackUid) );
            
            if (status != KErrNone)
                {
                PRINT((_L("CTRVideoEncoderClient::SetCodecParametersL(), Failed to get codec info")))
                User::Leave(KErrNotSupported);
                }
            
            PRINT((_L("CTRVideoEncoderClient::SetCodecParametersL(), Reverting to fallback encoder")))
            
            // Fallback ok, take it   
            iUid = iFallbackUid;
            }
        else
            {
            PRINT((_L("CTRVideoEncoderClient::SetCodecParametersL(), No suitable encoders found")))
            User::Leave(KErrNotSupported);
            }
        }          
        
    //  AA Set codec here (final hwdevice Uid should be known before checking CI buffermanag support)
    // Select encoder first
    this->SelectEncoderL();

    PRINT((_L("CTRVideoEncoderClient::SetCodecParametersL(), Out")))
    }


// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::CheckCodecInfoL
// Checks codec info
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TBool CTRVideoEncoderClient::CheckCodecInfoL(TUid aUid)
    {
    CVideoEncoderInfo* encoderInfo = NULL; // Encoder info for retrieving capabilities
    iMaxFrameRate = 0;
    TInt status = KErrNone;
    TUint32 dataUnitType = 0;
    TBool accelerated = EFalse;

    switch(iOutputDataType)
        {
        case CTRTranscoder::ETRDuCodedPicture:
            {
            dataUnitType = EDuCodedPicture;
            break;
            }

        case CTRTranscoder::ETRDuVideoSegment:
            {
            dataUnitType = EDuVideoSegment;
            break;
            }

        default:
            {
            PRINT((_L("CTRVideoEncoderClient::CheckCodecInfoL(), should leave at the earlier stage, panic")))
            TRASSERT(0);
            }
        }

    // Check encoder
    PRINT((_L("CTRVideoEncoderClient::CheckCodecInfoL(), getting info from [0x%x]"), aUid.iUid ))
    encoderInfo = iDevVideoRecord->VideoEncoderInfoLC( aUid );
    
    PRINT((_L("CTRVideoEncoderClient::CheckCodecInfoL(), info [0x%x]"), encoderInfo ))

    if (!encoderInfo)
        {
        PRINT((_L("CTRVideoEncoderClient::CheckCodecInfoL(), getting info from [0x%x] failed[%d]"), aUid.iUid, status ))
        User::Leave(KErrNotSupported);
        }
    else
        {
        // Check input format
        // 1. retrieve supported formats from encoder hwdevice (since exact options are unknown)
        RArray<TUncompressedVideoFormat> supportedInputFormats = encoderInfo->SupportedInputFormats();
        
        TInt formatCount = supportedInputFormats.Count();
        
        if (formatCount <= 0)
            {
            supportedInputFormats.Close();
            PRINT((_L("CTRVideoEncoderClient::CheckCodecInfoL(), There are no supported input formats") ))
            User::Leave(KErrNotSupported);
            }
        else
            {
            TUncompressedVideoFormat uncFormat;
            TBool found = EFalse;
            TInt pattern1, pattern2;
            TInt dataLayout;
            
            if (iUncompressedFormat.iYuvFormat.iPattern == EYuv420Chroma2)
                {
                pattern1 = EYuv420Chroma1;
                pattern2 = EYuv420Chroma2;
                dataLayout = EYuvDataPlanar;
                }
            else
                {
                pattern1 = EYuv422Chroma1;
                pattern2 = EYuv422Chroma2;
                dataLayout = EYuvDataInterleavedBE;
                }
            
            // Check the most important paramers
            for ( TInt i = 0; i < formatCount; i ++ )
                {
                uncFormat = supportedInputFormats[i];
                
                if ( (uncFormat.iDataFormat == iUncompressedFormat.iDataFormat) && 
                     (uncFormat.iYuvFormat.iDataLayout == dataLayout) &&
                     ( (uncFormat.iYuvFormat.iPattern == pattern1) || 
                       (uncFormat.iYuvFormat.iPattern == pattern2) ) )
                    {
                    // Assign the rest of parameters
                    iUncompressedFormat = uncFormat;
                    
                    if ( iCodecType != EH263 )
                        {
                        // Set aspect ratio to 1:1 (square)
                        iUncompressedFormat.iYuvFormat.iAspectRatioNum = 1;
                        iUncompressedFormat.iYuvFormat.iAspectRatioDenom = 1;
                        }
                        
                    found = ETrue;
                    break;
                    }
                }
                
            if (!found)
                {
                supportedInputFormats.Close();
                PRINT((_L("CTRVideoEncoderClient::CheckCodecInfoL(), Supported format is not found") ))
                User::Leave(KErrNotSupported);
                }
            }
        
        /*if ( !encoderInfo->SupportsOutputFormat(*iCompresedFormat) ) // comment until symbian error is fixed
            {
            PRINT((_L("CTRVideoEncoderClient::CheckCodecInfoL(), Output format is not supported")))
            status = KErrNotSupported;
            }
        else */
        if ( (encoderInfo->SupportedDataUnitTypes() & dataUnitType) != dataUnitType ) 
            {
            PRINT((_L("CTRVideoEncoderClient::CheckCodecInfoL(), There's no support for this DUType")))
            status = KErrNotSupported;
            }
        else
            {
            // Get max rate for requested image format
            RArray<TPictureRateAndSize> rateAndSize = encoderInfo->MaxPictureRates();
            TUint rates = rateAndSize.Count();
            
            TSize picSize = iPictureSize;
            if ( picSize == TSize(640, 352) )  // Use VGA max frame rate for VGA 16:9
                picSize = TSize(640, 480);

            for ( TUint i = 0; i < rates; i++ )
                {
                if ( rateAndSize[i].iPictureSize == picSize )
                    {
                    status = KErrNone;
                    iMaxFrameRate = rateAndSize[i].iPictureRate;
                    PRINT((_L("CTRVideoEncoderClient::CheckCodecInfoL(), Needed picture size found!")))
                    break;
                    }

                status = KErrNotSupported;
                }
            }
        }

    accelerated = encoderInfo->Accelerated();
    
    // Delete codec info
    CleanupStack::PopAndDestroy(encoderInfo);

    if (status != KErrNone)
        {
        User::Leave(status);
        }

    return accelerated;
    }



// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::SetBitRate
// Sets video bitrate
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoEncoderClient::SetBitRate(TUint aBitRate)
    {
    if ( (iState == ETRInitialized) || (iState == ETRRunning) || (iState == ETRPaused) )
        {
        iRateOptions.iBitrate = aBitRate;
        iRateOptions.iPictureQuality = KTRPictureQuality;
        iRateOptions.iQualityTemporalTradeoff = KTRQualityTemporalTradeoff;

        if (iRealTime)
            {
            iRateOptions.iLatencyQualityTradeoff = KTRLatencyQualityTradeoffRT;
            }
        else
            {
            iRateOptions.iLatencyQualityTradeoff = KTRLatencyQualityTradeoffNonRT;
            }
        
        // Set bit rate to Encoder
        iDevVideoRecord->SetRateControlOptions(0/*Layer 0 is supported*/, iRateOptions);
        iBitRateSetting = ETrue;
        }
    else
        {
        // Keep bit rate and set it later
        iRateOptions.iBitrate = aBitRate;
        }
    }


// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::SetRealTime
// Sets encoder mode to operate real-time
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoEncoderClient::SetRealTime(TBool aRealTime)
    {
    iRealTime = aRealTime;
    }


// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::SetFrameRate
// Sets frame rate
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoEncoderClient::SetFrameRate(TReal& aFrameRate)
    {
    if ( (iState == ETRInitialized) || (iState == ETRRunning) || (iState == ETRPaused) )
        {
        iRateOptions.iPictureRate = aFrameRate;
        
        // The target frame rate must not be greater than the source frame rate
        if ( (iSrcRate > 0.0) && (iRateOptions.iPictureRate > iSrcRate) )
            {
            iRateOptions.iPictureRate = iSrcRate;
            }

        if (iRealTime)
            {
            iRateOptions.iLatencyQualityTradeoff = KTRLatencyQualityTradeoffRT;
            }
        else
            {
            iRateOptions.iLatencyQualityTradeoff = KTRLatencyQualityTradeoffNonRT;
            }
        
        // Set framerate rate to Encoder
        iDevVideoRecord->SetRateControlOptions(0/*Layer 0 is supported*/, iRateOptions);
        }
    else
        {
        // Keep frame rate and set it later
        iRateOptions.iPictureRate = aFrameRate;
        
        // The target frame rate must not be greater than the source frame rate
        if ( (iSrcRate > 0.0) && (iRateOptions.iPictureRate > iSrcRate) )
            {
            iRateOptions.iPictureRate = iSrcRate;
            }
        }
    }


// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::SetChannelBitErrorRate
// Sets channel bit error rate
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoEncoderClient::SetChannelBitErrorRate(TReal aErrorRate)
    {
    if ( (iState == ETRInitialized) || (iState == ETRRunning) || (iState == ETRPaused) )
        {
        // Run-time setting
        iDevVideoRecord->SetChannelBitErrorRate(0/*Error protection level number*/, aErrorRate, 0.0/*Std deviation*/);
        }
    else
        {
        iErrorRate = aErrorRate;
        }
    }


// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::SetVideoCodingOptionsL
// Sets video coding options
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoEncoderClient::SetVideoCodingOptionsL(TTRVideoCodingOptions& aOptions)
    {
    iCodingOptions = aOptions;
    }


// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::GetVideoBitRateL
// Gets video bitrate
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TUint CTRVideoEncoderClient::GetVideoBitRateL()
    {
    return iRateOptions.iBitrate;
    }


// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::GetFrameRateL
// Gets frame rate
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TReal CTRVideoEncoderClient::GetFrameRateL()
    {
    return iRateOptions.iPictureRate;
    }


// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::SetInputFrameRate
// Sets source framerate
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoEncoderClient::SetInputFrameRate(TReal aFrameRate)
    {
    iSrcRate = aFrameRate;
    
    // The target frame rate must not be greater than the source frame rate
    if ( (iSrcRate > 0.0) && (iRateOptions.iPictureRate > iSrcRate) )
        {
        iRateOptions.iPictureRate = iSrcRate;
        }
    }


// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::InitializeL
// Initializes encoder
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoEncoderClient::InitializeL()
    {
    PRINT((_L("CTRVideoEncoderClient::InitializeL(), In")))
    TVideoDataUnitType dataUnitType = EDuCodedPicture;
    TEncoderBufferOptions bufferOptions;
    TUint maxBufferSize = 0;


    // Give Init settings
    // 1. Set output format. Output format was already set in SetCodecParametersL
    switch(iOutputDataType)
        {
        case CTRTranscoder::ETRDuCodedPicture:
            {
            dataUnitType = EDuCodedPicture;
            bufferOptions.iMinNumOutputBuffers = KTRMinNumberOfBuffersCodedPicture;
            break;
            }

        case CTRTranscoder::ETRDuVideoSegment:
            {
            dataUnitType = EDuVideoSegment;
            bufferOptions.iMinNumOutputBuffers = KTRMinNumberOfBuffersVideoSegment;
            break;
            }

        default:
            {
            // Should never happend. Encoder does not support uncompressed output format.
            TRASSERT(0);
            }
        }
        
    TVideoDataUnitEncapsulation dataUnitEncapsulation = EDuElementaryStream;
    
    // Use generic payload encapsulation for H.264
    if (iCodecType == EH264)
        {
        dataUnitEncapsulation = EDuGenericPayload;
        }

    iDevVideoRecord->SetOutputFormatL(iHwDeviceId, *iCompresedFormat, dataUnitType, dataUnitEncapsulation, EFalse);
    
    // 2. Set input format
    iDevVideoRecord->SetInputFormatL(iHwDeviceId, iUncompressedFormat, iPictureSize);

    // 3. Buffer options
    bufferOptions.iMaxPreEncoderBufferPictures = bufferOptions.iMinNumOutputBuffers;
    bufferOptions.iHrdVbvSpec = EHrdVbvNone;
    bufferOptions.iHrdVbvParams.Set(NULL, 0);

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

        case KTRH263CodecLevel45:
            {
            maxBufferSize = KTRMaxBufferSizeLevel45;
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

        case KTRMPEG4CodecLevel1:
        case KTRMPEG4CodecLevel0:
            {
            maxBufferSize = KTRMaxBufferSizeLevel0;
            break;
            }
            
        case KTRMPEG4CodecLevel0b:
            {
            maxBufferSize = KTRMaxBufferSizeLevel0b;
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
            
        case KTRMPEG4CodecLevel4a:
            {
            maxBufferSize = KTRMaxBufferSizeLevel4a;
            break;
            }
            
        default:
            {
            maxBufferSize = KTRMaxBufferSizeLevel0;
            }
        }

    bufferOptions.iMaxOutputBufferSize = maxBufferSize;
    bufferOptions.iMaxCodedPictureSize = bufferOptions.iMaxOutputBufferSize;

    if ((iCodecType != EH263) && (iCodingOptions.iSyncIntervalInPicture > 0))
        {
        // Set segment target size
        if ( iCodingOptions.iSyncIntervalInPicture < KTRMinSegmentSize )
            {
            bufferOptions.iMaxCodedSegmentSize = KTRMinSegmentSize;
            }
        else if (iCodingOptions.iSyncIntervalInPicture > bufferOptions.iMaxCodedPictureSize)
            {
            bufferOptions.iMaxCodedSegmentSize = bufferOptions.iMaxCodedPictureSize;
            }
        else
            {
            bufferOptions.iMaxCodedSegmentSize = iCodingOptions.iSyncIntervalInPicture;
            }
        }
    else
        {
        // Set segment size to max video coded picture size
        bufferOptions.iMaxCodedSegmentSize = bufferOptions.iMaxCodedPictureSize;
        }

    PRINT((_L("CTRVideoEncoderClient::InitializeL(), iMaxOutputBufferSize[%d], iMinNumOutputBuffers[%d]"), 
               bufferOptions.iMaxOutputBufferSize, bufferOptions.iMinNumOutputBuffers ))

    iDevVideoRecord->SetBufferOptionsL(bufferOptions);

    // 4. Random access point
    TReal accessRate = KTRDefaultAccessRate;    /* ~0.2 fps */

    if (iCodingOptions.iMinRandomAccessPeriodInSeconds > 0)
        {
        accessRate = 1.0 / TReal(iCodingOptions.iMinRandomAccessPeriodInSeconds);
        }

    PRINT((_L("CTRVideoEncoderClient::InitializeL(), RandomAcessRate[%f]"), accessRate));
    iDevVideoRecord->SetMinRandomAccessRate(accessRate);

    // 5. Other coding options
    if (iCodecType == EH263)
        {
        if (iCodingOptions.iSyncIntervalInPicture > 0)
            {
            TH263VideoMode h263CodingOptions;
            
            h263CodingOptions.iAllowedPictureTypes = EH263PictureTypeI | EH263PictureTypeP;
            h263CodingOptions.iForceRoundingTypeToZero = ETrue;
            h263CodingOptions.iPictureHeaderRepetition = 0;
            h263CodingOptions.iGOBHeaderInterval = iCodingOptions.iSyncIntervalInPicture;
            
            TPckgC<TH263VideoMode> h263OptionsPckg(h263CodingOptions);
            iDevVideoRecord->SetCodingStandardSpecificOptionsL(h263OptionsPckg);
            }
        }
    else if (iCodecType == EH264)
        {
        // H.264 options
        TAvcVideoMode avcCodingOptions;
        
        avcCodingOptions.iAllowedPictureTypes = EAvcPictureTypeI | EAvcPictureTypeP;
        avcCodingOptions.iFlexibleMacroblockOrder = EFalse;
        avcCodingOptions.iRedundantPictures = EFalse;
        avcCodingOptions.iDataPartitioning = iCodingOptions.iDataPartitioning;
        avcCodingOptions.iFrameMBsOnly = ETrue;
        avcCodingOptions.iMBAFFCoding = EFalse;
        avcCodingOptions.iEntropyCodingCABAC = EFalse;
        avcCodingOptions.iWeightedPPrediction = EFalse;
        avcCodingOptions.iWeightedBipredicitonMode = 0;
        avcCodingOptions.iDirect8x8Inference = EFalse;

        TPckgC<TAvcVideoMode> avc4OptionsPckg(avcCodingOptions);
        iDevVideoRecord->SetCodingStandardSpecificOptionsL(avc4OptionsPckg);
        }
    else
        {
        // Mpeg4 options
        TMPEG4VisualMode mpeg4CodingOptions;
        mpeg4CodingOptions.iShortHeaderMode = EFalse;
        mpeg4CodingOptions.iMPEG4VisualNormalMPEG4Mode.iHeaderExtension = iCodingOptions.iHeaderExtension;
        mpeg4CodingOptions.iMPEG4VisualNormalMPEG4Mode.iDataPartitioning = iCodingOptions.iDataPartitioning;
        mpeg4CodingOptions.iMPEG4VisualNormalMPEG4Mode.iReversibleVLC = iCodingOptions.iReversibleVLC;
        
        mpeg4CodingOptions.iMPEG4VisualNormalMPEG4Mode.iAllowedVOPTypes = EMPEG4VisualVOPTypeI;

        TPckgC<TMPEG4VisualMode> mpeg4OptionsPckg(mpeg4CodingOptions);
        iDevVideoRecord->SetCodingStandardSpecificOptionsL(mpeg4OptionsPckg);
        }

    // Set source
    if (iSrcRate <= 0.0)
        {
        iSrcRate = KTRDefaultSrcRate;   /* ~15.0 fps */
        }

    iDevVideoRecord->SetSourceMemoryL(iSrcRate, ETrue, iRealTime);

    // Initialize devVideoRecord
    iDevVideoRecord->Initialize();

    // Set media buffer type
    if (iCodecType == EH263)
        {
        // Set H.263
        iBufferType = CCMRMediaBuffer::EVideoH263;
        }
    else if (iCodecType == EH264)
        {
        // Set H.264
        iBufferType = CCMRMediaBuffer::EVideoMPEG4; // : What to set here?
        }
    else
        {
        // Set MPEG4
        iBufferType = CCMRMediaBuffer::EVideoMPEG4;
        }

    PRINT((_L("CTRVideoEncoderClient::InitializeL(), Out")))
    }


// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::SelectEncoderL
// Selects encoder
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoEncoderClient::SelectEncoderL()
    {
    PRINT((_L("CTRVideoEncoderClient::SelectEncoderL(), In")))
    TInt status = KErrNone;

    if (iUid != TUid::Null())
        {
        TRAP(status, iHwDeviceId = iDevVideoRecord->SelectEncoderL(iUid));
        }
    else
        {
        // Probably the error already exists, if iUid == NULL; 
        status = KErrAlreadyExists;
        }
              
    if (status != KErrNone)
        {    
        // Try again with the fallback encoder if one exists
        if( (iFallbackUid != TUid::Null()) && (iFallbackUid.iUid != iUid.iUid) )
            {
            PRINT((_L("CTRVideoEncoderClient::SelectEncoderL(), Reverting to fallback encoder")))
            iUid = iFallbackUid;
            }
        else
            {
            PRINT((_L("CTRVideoEncoderClient::SelectDecoderL(), Failed to select encoder")))
            User::Leave(KErrNotSupported);
            }
            
        TRAP( status, iAcceleratedCodecSelected = CheckCodecInfoL(iUid) );
            
        if (status != KErrNone)
            {
            PRINT((_L("CTRVideoEncoderClient::SelectDecoderL(), Failed to get codec info")))
            User::Leave(KErrNotSupported);
            }
            
        TRAP(status, iHwDeviceId = iDevVideoRecord->SelectEncoderL(iUid));
        
        if (status != KErrNone)
            {
            PRINT((_L("CTRVideoEncoderClient::SelectDecoderL(), Failed to select encoder")))
            User::Leave(KErrNotSupported);
            }
        }

    PRINT((_L("CTRVideoEncoderClient::SelectEncoderL(), Out")))
    }



// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::MdvroInitializeComplete
// Informs init status with received error code
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoEncoderClient::MdvroInitializeComplete(TInt aError)
    {
    if (aError == KErrNone)
        {
        iState = ETRInitialized;
        }
    else if ((aError == KErrHardwareNotAvailable) || (aError == KErrNotSupported))
        {
        PRINT((_L("CTRVideoEncoderClient::MdvroInitializeComplete(), Error in initialization")))
        
        // Map both error codes to the same
        aError = KErrNotSupported;
        
        // Try again with the fallback encoder if one exists
        while( (iFallbackUid != TUid::Null()) && (iFallbackUid.iUid != iUid.iUid) )
            {
            PRINT((_L("CTRVideoEncoderClient::MdvroInitializeComplete(), Reverting to fallback encoder")))
            
            iUid = iFallbackUid;
            
            // Devvideo must be recreated from scratch
            if (iDevVideoRecord)
                {
                delete iDevVideoRecord;
                iDevVideoRecord = NULL;
                }
            
            TRAPD( status, iDevVideoRecord = CMMFDevVideoRecord::NewL(*this) );
            if (status != KErrNone)
                {
                // Something went wrong, let CTRTranscoderImp handle the error
                PRINT((_L("CTRVideoEncoderClient::MdvroInitializeComplete(), Failed to create DevVideoRecord")))
                break;
                }
                
            TRAP( status, iAcceleratedCodecSelected = CheckCodecInfoL(iUid) );
            if (status != KErrNone)
                {
                // Fallback encoder can not be used, let CTRTranscoderImp handle the error
                PRINT((_L("CTRVideoEncoderClient::MdvroInitializeComplete(), Failed to get codec info")))
                break;
                }
                
            TRAP( status, SelectEncoderL() );        
            if (status != KErrNone)
                {
                // Fallback encoder can not be used, let CTRTranscoderImp handle the error
                PRINT((_L("CTRVideoEncoderClient::MdvroInitializeComplete(), Failed to select encoder")))
                break;
                }
            
            // We are now ready to reinitialize the encoder, let CTRTranscoderImp do it    
            aError = KErrHardwareNotAvailable;
            break;
            }
        }
          
    iObserver.MtrdvcoEncInitializeComplete(aError);
    }


// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::StartL
// Starts encoding
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoEncoderClient::StartL()
    {
    PRINT((_L("CTRVideoEncoderClient::StartL(), In")))

    if ( (iCodecType == EMpeg4) && (iVolHeader == KNullDesC8) )
        {
        // Before starting the encoder, request config data
        HBufC8* condigDecInfo = iDevVideoRecord->CodingStandardSpecificInitOutputLC();

        if (condigDecInfo)
            {
            PRINT((_L("CTRVideoEncoderClient::StartL(), Vol header length[%d]"), condigDecInfo->Length()))

            if ( (condigDecInfo->Length() > 0) && (condigDecInfo->Length() <= KMaxDesC8Length /*256*/) )
                {
                iVolHeader.Des().Copy( condigDecInfo->Des() );
                
                // Keep vol length for further use
                iVolLength = iVolHeader.Des().Length();
                PRINT((_L("CTRVideoEncoderClient::StartL(), VolLength[%d]"), iVolLength))
                }

            CleanupStack::PopAndDestroy(condigDecInfo);
            }
        else
            {
            PRINT((_L("CTRVideoEncoderClient::StartL(), Invalid codingDecInfo, leave")))
            User::Leave(KErrAlreadyExists);
            }
            
        PRINT((_L("CTRVideoEncoderClient::StartL(), Out")))
        }

    
    // Set Rate control options
    if (!iBitRateSetting)
        {
        iRateOptions.iPictureQuality = KTRPictureQuality;
        iRateOptions.iQualityTemporalTradeoff = KTRQualityTemporalTradeoff;

        if (iRealTime)
            {
            iRateOptions.iLatencyQualityTradeoff = KTRLatencyQualityTradeoffRT;
            }
        else
            {
            iRateOptions.iLatencyQualityTradeoff = KTRLatencyQualityTradeoffNonRT;
            }

        iDevVideoRecord->SetRateControlOptions(0/*Layer 0 is supported*/, iRateOptions);
        iBitRateSetting = ETrue;
        }
        
    if ( (iCodecType != EH263) && (iCodingOptions.iSyncIntervalInPicture > 0) )
        {
        // Set resync value (segment target size)
        iDevVideoRecord->SetSegmentTargetSize(0/*aLayer*/, 
                                              iCodingOptions.iSyncIntervalInPicture/*aSizeBytes*/, 
                                              0/*aSizeMacroblocks*/);
        }
    
    // Start encoding
    if (iFatalError == KErrNone)
        {
        iDevVideoRecord->Start();
        iState = ETRRunning;
        }
        
    // Reset ts monitor
    iLastTimestamp = -1;
    
    iSetRandomAccessPoint = EFalse;

    PRINT((_L("CTRVideoEncoderClient::StartL(), Out")))
    }


// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::EncodePictureL
// Encodes video picture
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoEncoderClient::EncodePictureL(TVideoPicture* aPicture)
    {
    PRINT((_L("CTRVideoEncoderClient::EncodePictureL(), In")))

    if (aPicture)
        {
        if (iFatalError == KErrNone)
            {
            // Set rest of picture options
            aPicture->iOptions = TVideoPicture::ETimestamp;
            PRINT((_L("CTRVideoEncoderClient::EncodePictureL(), picture timestamp [%d]"), TInt(aPicture->iTimestamp.Int64()) ))

            TTimeIntervalMicroSeconds ts = aPicture->iTimestamp;
                
            if ( ts <= iLastTimestamp)
                {
                // Prevent propagation of the error now
                PRINT((_L("CTRVideoEncoderClient::EncodePictureL(), Picture timestamp less than previously encoded")))
                PRINT((_L("CTRVideoEncoderClient::EncodePictureL(), Dropping picture!!!")))
                iObserver.MtrdvcoEncoderReturnPicture(aPicture);
                
                PRINT((_L("CTRVideoEncoderClient::EncodePictureL(), Out")))
                return;
                }
            else
                {
                iLastTimestamp = ts;
                }
            
            // If random access point was requested    
            if( iSetRandomAccessPoint )
                {
                PRINT((_L("CTRVideoEncoderClient::EncodePictureL(), Instant Refresh requested")));
                aPicture->iOptions |= TVideoPicture::EReqInstantRefresh;
                iSetRandomAccessPoint = EFalse;
                }
            else
                {
                aPicture->iOptions &= ~(TVideoPicture::EReqInstantRefresh);
                }
            
            aPicture->iData.iDataFormat = EYuvRawData;  // Force data format to YUV
            
            iDevVideoRecord->WritePictureL(aPicture);
            }
        else
            {
            PRINT((_L("CTRVideoEncoderClient::EncodePictureL(), Operation is not possible, since FatalError was reported by low-level[%d]"), iFatalError))
            return;
            }
        }
    else
        {
        PRINT((_L("CTRVideoEncoderClient::EncodePictureL(), Picture is not valid, leave")))
        User::Leave(KErrArgument);
        }
        
    PRINT((_L("CTRVideoEncoderClient::EncodePictureL(), Out")))
    }


// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::MdvroReturnPicture
// Returns picture
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoEncoderClient::MdvroReturnPicture(TVideoPicture *aPicture)
    {
    iObserver.MtrdvcoEncoderReturnPicture(aPicture);
    }


// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::MdvroNewBuffers
// New buffers are available in the system
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoEncoderClient::MdvroNewBuffers()
    {
    TVideoOutputBuffer* newBuffer = NULL;
    TInt status = KErrNone;
    
    
    if ( (iCodecType == EMpeg4) && (!iVolHeaderSent) && (iVolHeader != KNullDesC8) )
        {
        // Send vol header first to the client
        iOutputMediaBuffer->Set( iVolHeader.Des(), 
                                 CCMRMediaBuffer::EVideoMPEG4DecSpecInfo, 
                                 iVolHeader.Des().Length(), 
                                 0, 
                                 0 );
        
        iObserver.MtrdvcoNewBuffer(iOutputMediaBuffer);
        iVolHeaderSent = ETrue;

        // VOS+VO+VOL header is now going to be stored to metadata, it must not be included in mediadata so remove it from the next buffer
        iRemoveHeader = ETrue;
        }

    // Get new buffer and forward it to the client
    while( iDevVideoRecord->NumDataBuffers() > 0 )
        {
        TRAP(status, newBuffer = iDevVideoRecord->NextBufferL());
        if (status != KErrNone)
            {
            PRINT((_L("CTRVideoEncoderClient::MdvroNewBuffers(), NextBufferL failed[%d]"), status))
            iObserver.MtrdvcoFatalError(status);
            return;
            }

        if (newBuffer)
            {
            PRINT((_L("CTRVideoEncoderClient::MdvroNewBuffers(), dataBuffer length[%d]"), newBuffer->iData.Length()))
            
            // Check data length if valid
            if (newBuffer->iData.Length() <= 0)
                {
                // Data length is invalid
                PRINT((_L("CTRVideoEncoderClient::MdvroNewBuffers(), encoder generates invalid bitstream, abort data processing")))
                iObserver.MtrdvcoFatalError(KErrAbort);
                return;
                }
                
            TBool outputMediaBufferSet = EFalse;
            
            // Remove vol header if needed from the bitstream
            if ( iRemoveHeader )
                {  
                iRemoveHeader = EFalse;
                    
                if ( iCodecType == EMpeg4 )
                    {
                    // check if we need to remove VOS+VO+VOL header from the bitstream, since it is 
                    // stored in metadata, and having it in 2 places may cause interoperability problems
                    
                    // Since the length of the vol is already known, remove that data from the beginning of 
                    // the first buffer
                    if ( (iVolLength > 0) && (iVolLength < newBuffer->iData.Length()) )
                        {
                        TPtr8* buffer = reinterpret_cast<TPtr8*>(&newBuffer->iData);
                        buffer->Delete(0, iVolLength);
                        }
                    }
                    
                else if ( iCodecType == EH264 )
                    {
                    
                    TPtr8* ptr = reinterpret_cast<TPtr8*>(&newBuffer->iData);
                    TPtr8& buffer(*ptr);
                    
                    // Offset to the end
                    TInt readOffset = buffer.Length();
                    readOffset -= 4;
                    
                    TInt numNALUnits = 0;
                    TInt writeOffset = 0;
                    TInt firstFrameStart = 0;
                    TInt alignmentBytes = 0;

                    numNALUnits =  TInt(buffer[readOffset]) + 
                                  (TInt(buffer[readOffset + 1]) << 8 ) + 
                                  (TInt(buffer[readOffset + 2]) << 16) + 
                                  (TInt(buffer[readOffset + 3]) << 24);
                                       
                    // figure out the payload length of all NAL's to 
                    // determine the number of alignment bytes
                    
                    // point to first length field                        
                    readOffset -= (numNALUnits - 1) * 8;
                    readOffset -= 4;
                    
                    TInt allNALsLength = 0;
                                            
                    for (TInt i = 0; i < numNALUnits; i++)
                        {
                        TInt len = TInt(buffer[readOffset]) +
                                  (TInt(buffer[readOffset + 1]) << 8) +
                                  (TInt(buffer[readOffset + 2]) << 16) +
                                  (TInt(buffer[readOffset + 3]) << 24);
                                  
                        allNALsLength += len;
                        readOffset += 8;
                        }
                        
                    // calculate alignment bytes
                    alignmentBytes = (allNALsLength % 4 != 0) * ( 4 - (allNALsLength % 4) );
                    
                    // get pointer to end of last frame
                    writeOffset = buffer.Length() - 4 - numNALUnits * 8;
                    
                    // get number of frame NAL's
                    numNALUnits -= iNumH264SPSPPS;
                    
                    // point to offset field of first frame
                    readOffset = buffer.Length();
                    readOffset -= 4;
                    readOffset -= numNALUnits * 8;
                    
                    firstFrameStart = TInt(buffer[readOffset]) +
                                     (TInt(buffer[readOffset + 1]) << 8) +
                                     (TInt(buffer[readOffset + 2]) << 16) +
                                     (TInt(buffer[readOffset + 3]) << 24);

                    // The buffer should have enough space for the new NAL header
                    // if ( frameStart + frameSize + 12 < buffer.Length() )

                    // point to length field of first frame                                    
                    readOffset += 4;
                    
                    // first frame begins from offset 0
                    TInt nalOffset = 0;
                    TInt totalLength = 0;
                    
                    for (TInt i = 0; i < numNALUnits; i++)
                    {
                       
                        // read NAL length
                        TInt length = TInt(buffer[readOffset]) +
                                     (TInt(buffer[readOffset + 1]) << 8) +
                                     (TInt(buffer[readOffset + 2]) << 16) +
                                     (TInt(buffer[readOffset + 3]) << 24);
                                     
                        totalLength += length;
                       
                        // write offset                   
                        buffer[writeOffset + 0] = TUint8(nalOffset & 0xff);
                        buffer[writeOffset + 1] = TUint8((nalOffset >> 8) & 0xff);
                        buffer[writeOffset + 2] = TUint8((nalOffset >> 16) & 0xff);
                        buffer[writeOffset + 3] = TUint8((nalOffset >> 24) & 0xff);
                                            
                        // Write NAL length
                        writeOffset +=4;
                        buffer[writeOffset + 0] = TUint8(length & 0xff);
                        buffer[writeOffset + 1] = TUint8((length >> 8) & 0xff);
                        buffer[writeOffset + 2] = TUint8((length >> 16) & 0xff);
                        buffer[writeOffset + 3] = TUint8((length >> 24) & 0xff);
                    
                        writeOffset += 4;                    
                        readOffset += 8;
                        
                        nalOffset += length;
                    }
                    
                    // Write the number of NAL units
                    buffer[writeOffset + 0] = TUint8(numNALUnits & 0xff);
                    buffer[writeOffset + 1] = TUint8((numNALUnits >> 8) & 0xff);
                    buffer[writeOffset + 2] = TUint8((numNALUnits >> 16) & 0xff);
                    buffer[writeOffset + 3] = TUint8((numNALUnits >> 24) & 0xff);

                    // Get a pointer to the position where the frame starts          
                    TPtrC8 tmp(buffer.Ptr() + firstFrameStart, totalLength + alignmentBytes + numNALUnits * 8 + 4);

                    // Set output media buffer
                    iOutputMediaBuffer->Set( tmp, static_cast<CCMRMediaBuffer::TBufferType>(iBufferType), 
                                             tmp.Length(), newBuffer->iRandomAccessPoint, 
                                             newBuffer->iCaptureTimestamp );
                                             
                    outputMediaBufferSet = ETrue;
                    
                    }
                }
                
            if (!outputMediaBufferSet)
                {
                iOutputMediaBuffer->Set( newBuffer->iData, static_cast<CCMRMediaBuffer::TBufferType>(iBufferType), 
                                         newBuffer->iData.Length(), newBuffer->iRandomAccessPoint, 
                                         newBuffer->iCaptureTimestamp );
                }
                                     
            iObserver.MtrdvcoNewBuffer(iOutputMediaBuffer);
            
            // Return buffer to devvideo here, since observer call is synchronous
            iDevVideoRecord->ReturnBuffer(newBuffer);
            }
        else
            {
            PRINT((_L("CTRVideoEncoderClient::MdvroNewBuffers(), newBuffer is not available, nothing to do")))
            break;
            }
        }
    }


// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::StopL
// Stops encoding synchronously
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoEncoderClient::StopL()
    {
    PRINT((_L("CTRVideoEncoderClient::StopL(), In")))

    if (iFatalError == KErrNone)
        {
        iDevVideoRecord->Stop();
        }

    // Reset flags here
    iVolHeaderSent = EFalse;
    iState = ETRStopped;
 
    PRINT((_L("CTRVideoEncoderClient::StopL(), Out")))
    }


// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::AsyncStopL
// Stops encoding asynchronoulsy
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoEncoderClient::AsyncStopL()
    {
    PRINT((_L("CTRVideoEncoderClient::StopL(), Async In")))
    
    if (iFatalError == KErrNone)
        {
        iDevVideoRecord->InputEnd();
        }

    // Reset flags here
    iVolHeaderSent = EFalse;
    iState = ETRStopping;
    
    PRINT((_L("CTRVideoEncoderClient::StopL(), Async Out")))
    }


// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::MdvroStreamEnd
// End of stream is reached
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoEncoderClient::MdvroStreamEnd()
    {
    PRINT((_L("CTRVideoEncoderClient::MdvpoStreamEnd()")))
    iState = ETRStopped;
    iObserver.MtrdvcoEncStreamEnd();
    }


// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::MdvroSupplementalInfoSent
// Signals that the supplemental info send request has completed. The buffer used for supplemental information can be re-used or freed.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoEncoderClient::MdvroSupplementalInfoSent()
    {
    }


// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::MdvroFatalError
// Reports the fatal error
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoEncoderClient::MdvroFatalError(TInt aError)
    {
    PRINT((_L("CTRVideoEncoderClient::MdvroFatalError(), error[%d]"), aError))
    iFatalError = aError;
    iObserver.MtrdvcoFatalError(iFatalError);
    }


// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::GetCodingStandardSpecificInitOutputLC
// Requests video encoder codinc options
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
HBufC8* CTRVideoEncoderClient::GetCodingStandardSpecificInitOutputLC()
    {
    if ( iDevVideoRecord )
        {
        HBufC8* condigDecInfo = iDevVideoRecord->CodingStandardSpecificInitOutputLC();

        if (condigDecInfo)
            {
            PRINT((_L("CTRVideoEncoderClient::GetCodingStandardSpecificInitOutputLC(), Vol header length[%d]"), condigDecInfo->Length()))

            if ( (condigDecInfo->Length() > 0) && (condigDecInfo->Length() <= KMaxDesC8Length /*256*/) )
                {
                iVolHeader.Des().Copy( condigDecInfo->Des() );
                
                // Keep vol length for further use
                iVolLength = iVolHeader.Des().Length();
                PRINT((_L("CTRVideoEncoderClient::GetCodingStandardSpecificInitOutputLC(), VolLength[%d]"), iVolLength))

                // VOS+VO+VOL header is now going to be stored to metadata, it must not be included in mediadata so remove it from the next buffer
                iRemoveHeader = ETrue;
                iVolHeaderSent = ETrue;
                
                if ( iCodecType == EH264 )
                {
                    TPtr8 temp = condigDecInfo->Des();                                        
                    
                    // get number of SPS/PPS units to be able to 
                    // remove them later from a frame buffer
                    TInt index = condigDecInfo->Length() - 4;
                    
                    iNumH264SPSPPS = TInt(temp[index]) + 
                                    (TInt(temp[index + 1]) << 8 ) + 
                                    (TInt(temp[index + 2]) << 16) + 
                                    (TInt(temp[index + 3]) << 24);
                    
                }
                
                // Return codingInfo to the client
                return condigDecInfo;
                }
            }
        }
        
    // If condigDecInfo is not valid or something is going wrong, return NULL
    return NULL;
    }


// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::UseDataTransferOptimization
// Client's request to use optimized data transfer via CI
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoEncoderClient::UseDataTransferOptimizationL()
    {
    PRINT((_L("CTRVideoEncoderClient::UseDataTransferOptimizationL(), In")))

    // Check buffer management custom interface support before initializing
    iVideoBufferManagementCI = (MMmfVideoBufferManagement*)iDevVideoRecord->CustomInterface( iHwDeviceId, KMmfVideoBuffermanagementUid );
    PRINT((_L("CTRVideoEncoderClient::UseDataTransferOptimizationL(), iVideoBufferManagementCI[0x%x]"), iVideoBufferManagementCI))

    if (iVideoBufferManagementCI)
        {
        iVideoBufferManagementCI->MmvbmSetObserver(this);
        iVideoBufferManagementCI->MmvbmEnable(ETrue);

        // Set buffer options for this mode
        MMmfVideoBufferManagement::TBufferOptions bufferOptionsCI;
        bufferOptionsCI.iNumInputBuffers = KTRPictureBuffersNumberBMCI;
        bufferOptionsCI.iBufferSize = iPictureSize;
        iVideoBufferManagementCI->MmvbmSetBufferOptionsL(bufferOptionsCI);
        }
    else
        {
        PRINT((_L("CTRVideoEncoderClient::UseDataTransferOptimizationL(), Optimized data transfer is not supported for this hwdevice")))
        User::Leave(KErrNotSupported);
        }

    PRINT((_L("CTRVideoEncoderClient::UseDataTransferOptimizationL(), Out")))
    }
    
    
// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::MmvbmoNewBuffers
// One or several buffers are available throug CI
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoEncoderClient::MmvbmoNewBuffers()
    {
    PRINT((_L("CTRVideoEncoderClient::MmvbmoNewBuffers()")))
    
    if (iVideoBufferManagementCI)
        {
        // Call CI observer to the client only in case, if CI was successfully initialized
        iObserver.MtrdvcoNewBuffers();
        }
    }


// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::MmvbmoReleaseBuffers
// Release buffers
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoEncoderClient::MmvbmoReleaseBuffers()
    {
    PRINT((_L("CTRVideoEncoderClient::MmvbmoReleaseBuffers()")))
    }


// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::GetTargetVideoPictureL
// Gets target video picture buffer
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TVideoPicture* CTRVideoEncoderClient::GetTargetVideoPictureL()
    {
    if (iVideoBufferManagementCI)
        {
        return iVideoBufferManagementCI->MmvbmGetBufferL(iPictureSize);
        }
    else
        {
        PRINT((_L("CTRVideoEncoderClient::GetTargetVideoPictureL(), optimized data transfer is not supported")))
        User::Leave(KErrNotSupported);
        
        // Make compiler happy
        return NULL;
        }
    }
    
    
// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::SetRandomAccessPoint
// Requests to set random access point to bitstream
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRVideoEncoderClient::SetRandomAccessPoint()
    {
    PRINT((_L("CTRVideoEncoderClient::SetRandomAccessPoint(), In")))
    
    // There are several ways to requestI-Frame: 
    // 1. Pause() and immediately resume() should force to encoder to generate I-Frame.
    // 2. Set TVideoPictureOption: some of encoders don't support this
    // 3. PictureLoss(): some of encoders don't support this

    // Use TVideoPictureOption to request the I-frame later 
    iSetRandomAccessPoint = ETrue;
    
    PRINT((_L("CTRVideoEncoderClient::SetRandomAccessPoint(), Out")))
    }
    
// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::EstimateEncodeFrameTimeL
// Returns a time estimate on long it takes to encode one frame
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//    
TReal CTRVideoEncoderClient::EstimateEncodeFrameTimeL(const TTRVideoFormat& aOutput, TInt aCodecType)
    {
    if (iUid == TUid::Null())
        {
        PRINT((_L("CTRVideoEncoderClient::EstimateEncodeFrameTimeL(), no encoder selected yet")))
        User::Leave(KErrNotReady);
        }
    
    TReal time = 0.0;    
    
    // Select the predefined constant using the current settings
    if (aCodecType == EH263)
        {
        time = iAcceleratedCodecSelected ? KTREncodeTimeFactorH263HW : KTREncodeTimeFactorH263SW;
        }
    else if (aCodecType == EH264)
        {
        time = iAcceleratedCodecSelected ? KTREncodeTimeFactorH264HW : KTREncodeTimeFactorH264SW;
        }
    else
        {
        time = iAcceleratedCodecSelected ? KTREncodeTimeFactorMPEG4HW : KTREncodeTimeFactorMPEG4SW;
        }
    
    // Multiply the time by the resolution of the output frame       
    time *= static_cast<TReal>(aOutput.iSize.iWidth + aOutput.iSize.iHeight) * KTRTimeFactorScale;
    
    PRINT((_L("CTRVideoEncoderClient::EstimateEncodeFrameTimeL(), encode frame time: %.2f"), time))
        
    return time;
    }

// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::Pause
// Pauses encoding
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//     
void CTRVideoEncoderClient::Pause()
    {
    if ((iFatalError == KErrNone) && (iState == ETRRunning))
        {
        iDevVideoRecord->Pause();
        iState = ETRPaused;
        }
    }

// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::Resume
// Resumes encoding
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//      
void CTRVideoEncoderClient::Resume()
    {
    if ((iFatalError == KErrNone) && (iState == ETRPaused))
        {
        iDevVideoRecord->Resume();
        iState = ETRRunning;
        }
    }

// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::EnableResourceObserver
// Enable / Disable resourece observer
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//     
void CTRVideoEncoderClient::EnableResourceObserver(TBool aEnable)
    {
    PRINT((_L("CTRVideoEncoderClient::EnableResourceObserver(), In")))
    
    iVideoResourceHandlerCI = (MMmfVideoResourceHandler*)iDevVideoRecord->CustomInterface( iHwDeviceId, KUidMmfVideoResourceManagement );
    PRINT((_L("CTRVideoEncoderClient::EnableResourceObserver(), iVideoResourceHandlerCI[0x%x]"), iVideoResourceHandlerCI))

    if (iVideoResourceHandlerCI)
        {
        if (aEnable)
            {
            iVideoResourceHandlerCI->MmvrhSetObserver(this);
            PRINT((_L("CTRVideoEncoderClient::EnableResourceObserver(), Enabled")))
            }
        else
            {
            iVideoResourceHandlerCI->MmvrhSetObserver(NULL);
            }
        }

    PRINT((_L("CTRVideoEncoderClient::EnableResourceObserver(), Out")))
    }

// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::MmvroResourcesLost
// Indicates that a media device has lost its resources
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//     
void CTRVideoEncoderClient::MmvroResourcesLost(TUid /*aMediaDevice*/)
    {
    iObserver.MtrdvcoResourcesLost(EFalse);
    }
        

// -----------------------------------------------------------------------------
// CTRVideoEncoderClient::MmvroResourcesRestored
// Indicates that a media device has regained its resources
// (other items were commented in a header). 
// -----------------------------------------------------------------------------
// 
void CTRVideoEncoderClient::MmvroResourcesRestored(TUid /*aMediaDevice*/)
    {
    iObserver.MtrdvcoResourcesRestored();
    }

// End of file

