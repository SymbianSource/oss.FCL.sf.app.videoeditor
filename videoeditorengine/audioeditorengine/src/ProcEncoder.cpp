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
*
*/



#include "ProcEncoder.h"
#include "audconstants.h"
#include    <MmfDatabuffer.h>
#include    <mmfcontrollerpluginresolver.h>
#include    <mmfutilities.h>
#include    <mmf/plugin/mmfCodecImplementationUIDs.hrh>
#include    <MMFCodec.h>



// MACROS

// Debug print macro
#if defined _DEBUG 
#include <e32svr.h>
#define PRINT(x) RDebug::Print x; 
#else
#define PRINT(x)
#endif

// -----------------------------------------------------------------------------
// TCMRAMREncParams
// Encoding parameters structure for SW AMR codec
// -----------------------------------------------------------------------------
//
class TVedACAMREncParams
    {
public:
    // encoding mode (for AMR-NB: 0=MR475,1=MR515,...,7=MR122, default 7)
    TInt iMode;
    // DTX flag (TRUE or default FALSE)
    TInt iDTX;
    };

CProcEncoder* CProcEncoder::NewL()
                            
    {
    CProcEncoder* self = NewLC();
    CleanupStack::Pop(self);
    return self;
    
    }


CProcEncoder* CProcEncoder::NewLC()
                                
    {

    CProcEncoder* self = new (ELeave) CProcEncoder();
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
    
    }

CProcEncoder::~CProcEncoder()
    {
    

    delete iSourceInputBuffer;
    
    delete  iDestInputBuffer;
        
    delete iDestCodec;
    
    
    }
    
    
    
    
TBool CProcEncoder::InitL(TAudType aAudioType, TInt aTargetSamplingRate, TChannelMode aChannelMode,
                          TInt aBitrate)
    {
    PRINT((_L("CProcEncoder::InitL() in")));
    
    if (aAudioType == EAudAMR)
        {
        iOutputFrameDurationMilli = 20;
        }
    else if (aAudioType == EAudAAC_MPEG4)
        {
        iOutputFrameDurationMilli = (1024*1000)/(aTargetSamplingRate);
        }
    else
        {
        // only AMR & AAC encoding supported
        PRINT((_L("CProcEncoder::InitL() out, unsupported audio type")));
        User::Leave(KErrNotSupported);
        }
    
    iToBitRate = aBitrate;
    
    iAudioType = aAudioType;
    
    iNumberOfFramesInOutputBuffer = 0;

    iToSampleRate = aTargetSamplingRate;
       
    iToChannels = 1; 
    
    if (aChannelMode == EAudStereo)
        {
        iToChannels = 2;    
        }

    
    TInt destInputBufferSize = 0;
    TInt sourceInputBufferSize = 0;
   
    // input buffer size:
    // we never get longer than 64 milliseconds input
    // as it is limited by the input formats
    // for 16kHz AAC the frame duration is 64 ms
    //
    const TInt KMaxInputDurationMilli = 64;
   
    // multiplied by 2 as we have a bitdepth of 16
    sourceInputBufferSize = (2*aTargetSamplingRate*KMaxInputDurationMilli)/1000;

    if (aChannelMode == EAudStereo)
        {
        sourceInputBufferSize *= 2;
        }

    if ( aAudioType == EAudAMR )
        {
        
        // from 64 ms input we can have maximum 4 AMR frames
        destInputBufferSize = KAedMaxAMRFrameLength*4;
        
        
        }
    else if (aAudioType == EAudAAC_MPEG4)
        {
 
        if ( aChannelMode == EAudSingleChannel )
            {
            destInputBufferSize = KAedMaxAACFrameLengthPerChannel;
            }
        else
            {
            destInputBufferSize = 2 * KAedMaxAACFrameLengthPerChannel;
        
            }        
        }
    
    if ( iSourceInputBuffer )
        {
        delete iSourceInputBuffer;
        iSourceInputBuffer = NULL;
        }
    
    iSourceInputBuffer = CMMFDataBuffer::NewL(sourceInputBufferSize*5);
    
    
    if ( iDestInputBuffer )
        {
        delete iDestInputBuffer;
        iDestInputBuffer = NULL;
        }

    TInt errC = KErrNone;

    if (aAudioType == EAudAMR)
        {
        
        // shouldn't get more than 6 AMR frames at a time -> 120 ms
        iDestInputBuffer = CMMFDataBuffer::NewL( destInputBufferSize);
        
        }
    else
        {
        
        iDestInputBuffer = CMMFDataBuffer::NewL( destInputBufferSize);
        
        }
    
    TRAP (errC,SetDestCodecL());
    
    if (errC != KErrNone)
        {
        // initialization failed for some reason
        User::Leave(KErrNotSupported);
        
        }

    TInt err = KErrNone;

    if ( iAudioType == EAudAAC_MPEG4 )
        {
        TRAP( err, ConfigureAACEncoderL());
        
        }
    else if (iAudioType == EAudAMR)
        {
        TRAP( err, ConfigureAMREncoderL());
        }
    
    
    if (err != KErrNone || errC != KErrNone)
        {

        // initialization failed for some reason
        User::Leave(KErrNotSupported);
        }

    iReady = ETrue;
    PRINT((_L("CProcEncoder::InitL() out")));

    return ETrue;
    }

TBool CProcEncoder::FillEncBufferL(const TDesC8& aRawFrame, HBufC8* aEncBuffer, TInt& aOutputDurationMilli)
    {
    PRINT((_L("CProcEncoder::FillEncBufferL() in")));
    
    aOutputDurationMilli = 0;
    

    if (!iReady)
        {
        User::Leave(KErrNotReady);
        }
    
    iEncBuffer = aEncBuffer;
    
    if (iEncBuffer->Length() == 0)
        {
        iNumberOfFramesInOutputBuffer = 0;
        }
       
    if ( !aRawFrame.Length() )
        {
        return EFalse;
        }
    
    if ( (TInt)(aRawFrame.Length() + iSourceInputBuffer->Position() ) > iSourceInputBuffer->Data().MaxLength() )
        {
        ReAllocBufferL( iSourceInputBuffer, aRawFrame.Length() + iSourceInputBuffer->Position() );
        
        }
    
    
    // copy the input data to MMF buffer
    iSourceInputBuffer->Data().SetLength( 0 );
    iSourceInputBuffer->SetPosition( 0 );

    iSourceInputBuffer->Data().Append( aRawFrame );
    iSourceInputBuffer->Data().SetLength( iSourceInputBuffer->Data().Length() );
    iSourceInputBuffer->SetStatus(EFull);
    
    iDestInputBuffer->Data().SetLength(0);
    iDestInputBuffer->SetPosition(0); 
  
    FeedCodecL( iDestCodec, iSourceInputBuffer, iDestInputBuffer);

    
    
    iDestInputBuffer->Data().SetLength(0);
    iDestInputBuffer->SetPosition(0); 
    
    if (iEncBuffer->Size() > 0)
       {
       
       aOutputDurationMilli = iOutputFrameDurationMilli * iNumberOfFramesInOutputBuffer;
       PRINT((_L("CProcEncoder::FillEncBufferL() out with ETrue (complete)")));
       return ETrue;
       }
       
    PRINT((_L("CProcEncoder::FillEncBufferL() out with EFalse (incomplete)")));
    return EFalse;
    }

TAudType CProcEncoder::DestAudType()
    {
    return iAudioType;
    }

void CProcEncoder::ConstructL()
    {
    
    }

CProcEncoder::CProcEncoder()
    {
    
    }
    
    
void CProcEncoder::ConfigureAMREncoderL()
    {
    PRINT((_L("CProcEncoder::ConfigureAMREncoderL() in")));

    if ( (iToBitRate < KAedMinAMRBitRate) || (iToBitRate > KAedMaxAMRBitRate) )
        {
        User::Leave( KErrArgument );
        }

    TVedACAMREncParams* configData = new (ELeave) TVedACAMREncParams;
    CleanupStack::PushL( configData );

    // the bitrates in the switch & if below are not magic numbers but AMR bitrates in bits per seconds and mode indices from TAmrEncParams

    switch ( iToBitRate )
        {
        case 4750:
            configData->iMode = 0;
            configData->iDTX = EFalse;
            break;
        case 5150:
            configData->iMode = 1;
            configData->iDTX = EFalse;
            break;
        case 5900:
            configData->iMode = 2;
            configData->iDTX = EFalse;
            break;
        case 6700:
            configData->iMode = 3;
            configData->iDTX = EFalse;
            break;
        case 7400:
            configData->iMode = 4;
            configData->iDTX = EFalse;
            break;
        case 7950:
            configData->iMode = 5;
            configData->iDTX = EFalse;
            break;
        case 10200:
            configData->iMode = 6;
            configData->iDTX = EFalse;
            break;
        case 12200:
            configData->iMode = 7;
            configData->iDTX = EFalse;
            break;
        default :
            // Interprets now the bitrate proprietarily: bitrates that are not exactly AMR bitrates 
            // mean that voice activity detection is used and the AMR bitrates is the given bitrate rounded upwards to the next AMR bitrate
            if ( iToBitRate < 4750 )
                {
                configData->iMode = 0;
                configData->iDTX = ETrue;
                }
            else if ( iToBitRate < 5150 )
                {
                configData->iMode = 1;
                configData->iDTX = ETrue;
                }
            else if ( iToBitRate < 5900 )
                {
                configData->iMode = 2;
                configData->iDTX = ETrue;
                }
            else if ( iToBitRate < 6700 )
                {
                configData->iMode = 3;
                configData->iDTX = ETrue;
                }
            else if ( iToBitRate < 7400 )
                {
                configData->iMode = 4;
                configData->iDTX = ETrue;
                }
            else if ( iToBitRate < 7950 )
                {
                configData->iMode = 5;
                configData->iDTX = ETrue;
                }
            else if ( iToBitRate < 10200 )
                {
                configData->iMode = 6;
                configData->iDTX = ETrue;
                }
            else // must be: ( iToBitRate < 12200 ) since checked earlier
                {
                configData->iMode = 7;
                configData->iDTX = ETrue;
                }
        }

    TUid uid ={KUidMmfCodecAudioSettings}; // Use Uid reserved for codec configurations
    iDestCodec->ConfigureL( uid, reinterpret_cast<TDesC8&>(*configData));
    CleanupStack::PopAndDestroy( configData );
    PRINT((_L("CProcEncoder::ConfigureAMREncoderL() out")));
    }

// -----------------------------------------------------------------------------
// CProcEncoder::ConfigureAACEncoderL
// 
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CProcEncoder::ConfigureAACEncoderL()
    {
    PRINT((_L("CProcEncoder::ConfigureAACEncoderL() in")));
    
    TInt i = 0;
    TBool iSet = EFalse;
    for ( i = 0; i < KAedNumSupportedAACSampleRates; i++ )
        {
        if ( iToSampleRate == KAedSupportedAACSampleRates[i] )
            {
            iSet = ETrue;
            }
        }
    if ( !iSet )
        {
        // given samplerate is not supported
        User::Leave( KErrNotSupported );
        }

    // AAC codec interprets the input as array of TInts, not as a class
    RArray<TInt> config;
    config.Append (iToBitRate); //BitRate
    config.Append (iToSampleRate);  //SamplingRate
    config.Append (0);  //iToolFlags
    config.Append (iToChannels);    //iNumChan
    
    // NOTE Ali: for 48kHz stereo we might need to use ADTS as output format
    // as we can get more than one frame in synchronous call!
    config.Append (0);  //iuseFormat 0=RAW; 1=ADTS; 2=ADIF
    config.Append (0);  // 0 = 1 Frame only; 1 = Full Buffer

    TUid uid ={KUidMmfCodecAudioSettings}; // Use Uid reserved for codec configurations
    iDestCodec->ConfigureL( uid,  reinterpret_cast<TDesC8&>(config));
    config.Close();
    PRINT((_L("CProcEncoder::ConfigureAACEncoderL() out")));
    }

    
void CProcEncoder::ReAllocBufferL( CMMFDataBuffer* aBuffer, TInt aNewMaxSize )
    {
    if ( aBuffer->Data().Length() )
        {
        TInt position = aBuffer->Position();
        TInt length = aBuffer->Data().Length();
        HBufC8* oldData = aBuffer->Data().AllocL();
        CleanupStack::PushL( oldData );
        ((CMMFDescriptorBuffer*)aBuffer)->ReAllocBufferL( aNewMaxSize );
        aBuffer->Data().Copy( *oldData );
        CleanupStack::PopAndDestroy( oldData );
        aBuffer->Data().SetLength( length );
        aBuffer->SetPosition( position );
        }
    else
        {
        ((CMMFDescriptorBuffer*)aBuffer)->ReAllocBufferL( aNewMaxSize );
        }
    }
    
void CProcEncoder::FeedCodecL( CMMFCodec* aCodec, CMMFDataBuffer* aSourceBuffer, CMMFDataBuffer* aDestBuffer )
    {
    PRINT((_L("CProcEncoder::FeedCodecL() in")));
    TBool completed = EFalse;
    TCodecProcessResult result;
    TInt aSrcUsed = 0;

    while ( !completed )
        {

        // encode and check the result
        result = EncodeL(aCodec, aSourceBuffer, aDestBuffer);
             
        switch ( result.iStatus )
            {
            case TCodecProcessResult::EProcessIncomplete:
                // Not all data from input was consumed (EncodeL updated buffer members), but output was generated
                
                iEncBuffer->Des().Append(aDestBuffer->Data());
                iNumberOfFramesInOutputBuffer++;
                         
                aDestBuffer->Data().SetLength( 0 );
                aDestBuffer->SetPosition( 0 );
                
                break;

            case TCodecProcessResult::EProcessComplete:
                // all data from input was used and output was generated
      
                iEncBuffer->Des().Append(aDestBuffer->Data());
                
                iNumberOfFramesInOutputBuffer++;
                    
                aDestBuffer->Data().SetLength( 0 );
                aDestBuffer->SetPosition( 0 );
         
                //completed = ETrue;
                
                break;

            case TCodecProcessResult::EDstNotFilled:
                // need more input data, can't fill the output yet; put it back to the empty queue
                //completed = ETrue;
          
                break;

            default:
                // EEndOfData, EProcessError, EProcessIncompleteRepositionRequest, EProcessCompleteRepositionRequest
                User::Leave( KErrUnknown );
            }
        
        aSrcUsed += result.iSrcBytesProcessed;
        if (aSrcUsed >= (STATIC_CAST(CMMFDataBuffer*, aSourceBuffer)->Data().Length()))
			{
            PRINT((_L("CProcEncoder::FeedCodecL() ProcessL is completed aSrcUsed[%d]"), aSrcUsed));
			completed = ETrue;
			}
            

        }
    
    PRINT((_L("CProcEncoder::FeedCodecL() out")));
    }
    

TBool CProcEncoder::GetIsSupportedDestCodec()
    {
    TFourCC fourCC; 
    TUid euid;
 
    if (iAudioType == EAudAMR)
        {
        fourCC = TFourCC(KMMFFourCCCodeAMR);
        euid = KAedAMRNBEncSWCodecUid;
        }
    else if (iAudioType == EAudAAC_MPEG4 )
        {
        fourCC = TFourCC(KMMFFourCCCodeAAC);
        euid = KAedAACEncSWCodecUid;
        }
    
    _LIT8(emptyFourCCString, "    ,    ");
    TBufC8<9> fourCCString(emptyFourCCString);
    TPtr8 fourCCPtr = fourCCString.Des();
    TPtr8 fourCCPtr1(&fourCCPtr[0], 4);
    TPtr8 fourCCPtr2(&fourCCPtr[5], 4 );

    TFourCC srcFourCC(' ','P','1','6');
    srcFourCC.FourCC(&fourCCPtr1);
    fourCC.FourCC(&fourCCPtr2);

    TBool found = EFalse;
    TRAPD( err, found = CheckIfCodecAvailableL( fourCCPtr, euid ));
    
    if (err == KErrNone)
        {
        return found;    
        }
    else
        {
        return EFalse;
        }
    
    }

    
void CProcEncoder::SetDestCodecL()
    {
    PRINT((_L("CProcEncoder::SetDestCodecL() in")));

    if ( !GetIsSupportedDestCodec() )
        {
        PRINT((_L("CProcEncoder::SetDestCodecL() error, unsupported codec")));
        User::Leave( KErrNotSupported );
        }
    
    if ( iDestCodec )
        {
        delete iDestCodec;
        iDestCodec = NULL;
        }

    TUid euid = TUid::Null();
 
    if (iAudioType == EAudAMR)
        {
        euid = KAedAMRNBEncSWCodecUid;
        }
    else if (iAudioType == EAudAAC_MPEG4 )
        {
        euid = KAedAACEncSWCodecUid;
        }
    
    
    iDestCodec = CMMFCodec::NewL (euid);
    iReady = EFalse;

    PRINT((_L("CProcEncoder::SetDestCodecL() out")));
    }   
   
    
TBool CProcEncoder::CheckIfCodecAvailableL(
    const TDesC8& aCodecFourCCString, const TUid& aCodecUId )
    {
    PRINT((_L("CProcEncoder::CheckIfCodecAvailableL() in")));
    TBool found = EFalse;

    // Create a TEcomResolverParams structure.
    TEComResolverParams resolverParams ;
    resolverParams.SetDataType( aCodecFourCCString ) ;
    resolverParams.SetWildcardMatch( EFalse ) ;

    RImplInfoPtrArray plugInArray ; // Array to return matching decoders in (place on cleanupstack _after_ ListImplementationsL() )

    TUid UidMmfPluginInterfaceCodec = {KMmfUidPluginInterfaceCodec};

    // ListImplementationsL leaves if it cannot find anything so trap the error and ignore it.
    TRAPD( err, REComSession::ListImplementationsL(UidMmfPluginInterfaceCodec, resolverParams, plugInArray ) ) ;
    CleanupResetAndDestroyPushL(plugInArray);

    if (err == KErrNone)
        {
        found = EFalse;
        for ( TInt i = 0; i < plugInArray.Count(); i++)
            {
            // there is a match, but 1st we need to ensure it is the one we have tested with, and that have compatible implementation of ConfigureL
            PRINT((_L("CProcEncoder::CheckIfCodecAvailable() plugin found with Uid 0x%x"), plugInArray[i]->ImplementationUid().iUid ));
            if ( plugInArray[i]->ImplementationUid() == aCodecUId )
                {
			    //match accepted
                PRINT((_L("CProcEncoder::CheckIfCodecAvailable() plugin accepted")));
			    found = ETrue;
                }
            }
        }
    else
        {
        PRINT((_L("CProcEncoder::CheckIfCodecAvailable() Error in ListImp.: %d"), err));
        //no match
        found = EFalse;
        }


    CleanupStack::PopAndDestroy();  //plugInArray
    PRINT((_L("CProcEncoder::CheckIfCodecAvailableL() out")));
    return found;
    }

TCodecProcessResult CProcEncoder::EncodeL( CMMFCodec* aCodec, CMMFDataBuffer* aInBuffer, CMMFDataBuffer* aOutBuffer)
    {
    PRINT((_L("CProcEncoder::EncodeL() in, input pos: %d, length: %d"), aInBuffer->Position(), aInBuffer->Data().Length() ));
    TCodecProcessResult result;

    result = aCodec->ProcessL (*aInBuffer, *aOutBuffer);

    switch (result.iStatus)
        {
        case TCodecProcessResult::EProcessComplete:
            // finished processing source data, all data in sink buffer
            PRINT((_L("CProcEncoder::FeedCodecL() EProcessComplete")));
            aInBuffer->SetPosition( 0 );
            aInBuffer->Data().SetLength(0);
            break;

        case TCodecProcessResult::EDstNotFilled:
            // the destination is not full, we need more data. Handled in caller
            PRINT((_L("CProcEncoder::FeedCodecL() EDstNotFilled")));
            aInBuffer->SetPosition( 0 );
            aInBuffer->Data().SetLength(0);
            break;

        case TCodecProcessResult::EProcessIncomplete:
            // the sink was filled before all the source was processed
            PRINT((_L("CProcEncoder::FeedCodecL() EProcessIncomplete")));
            aOutBuffer->SetPosition( 0 );
            aInBuffer->SetPosition( aInBuffer->Position() + result.iSrcBytesProcessed );
            break;

        default:
            break;
        }

 

    PRINT((_L("CProcEncoder::EncodeL() out, %d -> %d"),result.iSrcBytesProcessed, result.iDstBytesAdded));
    PRINT((_L("CProcEncoder::EncodeL() out, input pos: %d, length: %d"), aInBuffer->Position(), aInBuffer->Data().Length() ));
    return result;
    }

