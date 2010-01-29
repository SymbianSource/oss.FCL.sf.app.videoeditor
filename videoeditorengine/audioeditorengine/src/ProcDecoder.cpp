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



#include "ProcDecoder.h"
#include "audconstants.h"
#include    <MmfDatabuffer.h>
#include    <mmfcontrollerpluginresolver.h>
#include    <mmf/plugin/mmfCodecImplementationUIDs.hrh>
#include    <MMFCodec.h>


// CONSTANTS





// MACROS

// Debug print macro
#if defined _DEBUG 
#include <e32svr.h>
#define PRINT(x) RDebug::Print x;
#else
#define PRINT(x)
#endif


CProcDecoder* CProcDecoder::NewL()
                            
    {
    CProcDecoder* self = NewLC();
    CleanupStack::Pop(self);
    return self;
    
    }


CProcDecoder* CProcDecoder::NewLC()
                                
    {

    CProcDecoder* self = new (ELeave) CProcDecoder();
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
    
    }

CProcDecoder::~CProcDecoder()
    {
    
    delete iSourceInputBuffer;
    
    // Input buffer for encoder (alternative output buffer for decoder).
    delete  iDestInputBuffer;
        
    // Codec used in decoding input audio to PCM16
    delete iSourceCodec;
    
    delete iSampleRateChannelBuffer;
    
    delete iRateConverter;
        
    }
 
TBool CProcDecoder::InitL(TAudFileProperties aProperties, TInt aTargetSamplingRate, TChannelMode aChannelMode)
    {
    PRINT((_L("CProcDecoder::InitL() in")));
    
    if (aTargetSamplingRate == 0)
        {
        
        // we are just adding a clip now, check if the source codec is available
        iProperties = aProperties;    
        SetSourceCodecL();
        
        return ETrue;
        }
    
    
    iDecimFactor = 0; // by default
    
    iDoDecoding = ETrue;
    
    if (aProperties.iAudioType == EAudWAV)
        {
        // if input is PCM, no decoding is required, but possibly sample rate conversion
        iDoDecoding = EFalse;
        }
    
    
    
    // set output params
    iToSampleRate = aTargetSamplingRate;
    iToChannels = 1;
    
    if (aChannelMode == EAudStereo)
        {
        iToChannels = 2;    
        }
    
    
    //set input params
    iProperties = aProperties;
    
    iFromSampleRate = iProperties.iSamplingRate;
    
    iFromChannels = 1;
    
    if (aProperties.iChannelMode == EAudStereo)
        {
        iFromChannels = 2;    
        }
    
    
    iDoSampleRateChannelConversion = EFalse;
    if (iFromSampleRate != iToSampleRate || iProperties.iChannelMode != aChannelMode)
        {
        // sample rate or channel conversion is needed
        iDoSampleRateChannelConversion = ETrue;
        }
    
 
    PRINT((_L("CProcDecoder::PrepareConverterL() in")));
    
    TInt destInputBufferSize = 0;
    TInt sourceInputBufferSize = 0;
    
    // buffer sizes for input and output
    if ( iProperties.iAudioType == EAudAMR )
        {
        destInputBufferSize = KAedSizeAMRBuffer;
        sourceInputBufferSize = KAedMaxAMRFrameLength;
       
        }
    else if ( iProperties.iAudioType == EAudAMRWB )
        {
        destInputBufferSize = KAedSizeAWBBuffer;
        sourceInputBufferSize = KAedMaxAWBFrameLength;
       
        }
    else if (iProperties.iAudioType == EAudAAC_MPEG4 &&
             iProperties.iAudioTypeExtension == EAudExtensionTypeNoExtension)
        {
        if ( aProperties.iChannelMode == EAudSingleChannel )
            {
            destInputBufferSize = KAedSizeAACBuffer;
            sourceInputBufferSize = KAedMaxAACFrameLengthPerChannel;
            }
        else
            {
            destInputBufferSize = KAedSizeAACStereoBuffer;
            sourceInputBufferSize = 2* KAedMaxAACFrameLengthPerChannel;
            }
                 
        }
    else if (iProperties.iAudioType == EAudAAC_MPEG4 &&
             iProperties.iAudioTypeExtension != EAudExtensionTypeNoExtension)
        {
        if ( iProperties.iAudioTypeExtension == EAudExtensionTypeEnhancedAACPlusParametricStereo )
            {
            // output is parametric stereo (mono channel, but decoded to stereo)
            destInputBufferSize = KAedSizeAACStereoBuffer*2; // 2 for eAAC+
            sourceInputBufferSize = KAedMaxAACFrameLengthPerChannel;
            }
        else
            {
            // output is normal stereo
            destInputBufferSize = KAedSizeAACBuffer*2; // 2 for eAAC+
            sourceInputBufferSize = 2* KAedMaxAACFrameLengthPerChannel;
            }
            
                 
        }
    
    else if ( iProperties.iAudioType == EAudMP3 )
        {
        // mp3 can be directly decoded to mono if necessary
        if (iToChannels == 1)
            {
            iFromChannels = 1;
            }
        
        // check if decimation would be useful    
        if (iFromSampleRate == iToSampleRate * 4)
            {
            iFromSampleRate /= 4;
            iDecimFactor = 4;
            }
            
        if (iFromSampleRate == iToSampleRate * 2)
            {
            iFromSampleRate /= 2;
            iDecimFactor = 2;
            }
            
        if ((iToSampleRate == 8000) && ((iFromSampleRate == 48000) || (iFromSampleRate == 44100)))
            {
            iFromSampleRate /= 2;
            iDecimFactor = 2;
            }        
        
        const TReal KMP3FrameDurationSec = 0.026;
        
        destInputBufferSize =  (iFromSampleRate * KMP3FrameDurationSec + 1);
        destInputBufferSize *= 2; //16 bit samples
        if (iFromChannels == 2)
            {
            destInputBufferSize *= 2; // stereo
            }
        
        sourceInputBufferSize = KAedMaxMP3FrameLength;
        }
        
    else if (iProperties.iAudioType == EAudWAV)
        {
        // frame duration in WAV case is always 20 ms, that is 50 frames per sec
        const TInt KNumberOfFramesInSecond = 50;
            
        destInputBufferSize = iFromSampleRate / KNumberOfFramesInSecond;    // Number of samples
        destInputBufferSize *= iFromChannels;
        if ((destInputBufferSize % 2) != 0) destInputBufferSize++;          // Must be even
        destInputBufferSize *= 2;                                           // 16-bit samples
        
        sourceInputBufferSize = iProperties.iFrameLen;
        
        TInt sourceBufferSizeAfterExpansion = sourceInputBufferSize;
        if (iProperties.iNumberOfBitsPerSample == 8)
            {
            // 8-bit samples are expanded to 16-bit so twice as big buffer is needed
            sourceBufferSizeAfterExpansion *= 2;
            }
        
        if (sourceBufferSizeAfterExpansion > destInputBufferSize)
            {
            // Make sure there's no overflow
            destInputBufferSize = sourceBufferSizeAfterExpansion;
            }
        }
        
    PRINT((_L("CProcDecoder::InitL() source size: %d, dest size %d"), sourceInputBufferSize, destInputBufferSize));
    

    if ( iSourceInputBuffer )
        {
        delete iSourceInputBuffer;
        iSourceInputBuffer = NULL;
        }
    // create buffer for input data
    iSourceInputBuffer = CMMFDataBuffer::NewL(sourceInputBufferSize);
    
    
    if ( iDestInputBuffer )
        {
        delete iDestInputBuffer;
        iDestInputBuffer = NULL;
        }

    
    TInt errC = KErrNone;
    if (iDoDecoding)
        {
        SetSourceCodecL();
        
        // create buffer for output data if necessary
        iDestInputBuffer = CMMFDataBuffer::NewL( destInputBufferSize);
        }
    


    TInt err = KErrNone;

    if (iProperties.iAudioType == EAudAAC_MPEG4)
        {
        // configure AAC plus decoder, used also for AAC
        TRAP( err, ConfigureAACPlusDecoderL());
        
        }
    
    else if ( iProperties.iAudioType == EAudMP3 )
        {
        // configure mp3 decoder
        TRAP( err, ConfigureMP3DecoderL());
        
        }
    if (err != KErrNone || errC != KErrNone)
        {
        User::Leave(KErrNotSupported);
        }

    if ( iDoSampleRateChannelConversion )
        {
        if (iRateConverter)
            {
            delete iRateConverter;
            iRateConverter = NULL;
            }

        iRateConverter = CRateConverter::NewL(iFromSampleRate, iToSampleRate, iFromChannels, iToChannels);
        
        if( !iRateConverter->InitL(destInputBufferSize / (2 * iFromChannels)) ) // 16-bit samples
            {
            User::Leave(KErrNotSupported);
            }
            
        TInt sampleRateBufferSize = iRateConverter->GetOutputBufferSize() * 2 * iToChannels;    // 16-bit samples
        
        PRINT((_L("CProcDecoder::InitL() sample rate buffer size %d"), sampleRateBufferSize));
               
        if ( iSampleRateChannelBuffer )
            {
            delete iSampleRateChannelBuffer;
            iSampleRateChannelBuffer = NULL;
            }

        // Buffer for sample rate conversion output
        iSampleRateChannelBuffer = CMMFDataBuffer::NewL(sampleRateBufferSize);
        }


    iReady = ETrue;
    PRINT((_L("CProcDecoder::InitL() out")));

    
    return ETrue;
    }

TBool CProcDecoder::FillDecBufferL(const HBufC8* aEncFrame, HBufC8*& aDecBuffer)
    {
    PRINT((_L("CProcDecoder::FillDecBufferL() in")));
    
    iDecBuffer = 0;
    if (iProperties.iAudioType == EAudWAV && !iDoSampleRateChannelConversion)
        {
        
        // if we don't have to do anything for input data ->
        aDecBuffer = HBufC8::NewL(aEncFrame->Size());
        aDecBuffer->Des().Append(*aEncFrame);
        PRINT((_L("CProcDecoder::FillDecBufferL() out from Wav branch with ETrue")));
        return ETrue;

        }
        
        
    if (!iReady)
        {
        User::Leave(KErrNotReady);
        }
      
    if ( aEncFrame == 0 || !aEncFrame->Length() )
        {
        // no data in input buffer
        PRINT((_L("CProcDecoder::FillDecBufferL() no input data, out with EFalse")));
        return EFalse;
        }
    
    if ( (TInt)(aEncFrame->Length() + iSourceInputBuffer->Position() ) > iSourceInputBuffer->Data().MaxLength() )
        {
        
        ReAllocBufferL( iSourceInputBuffer, aEncFrame->Length() + iSourceInputBuffer->Position() );
        }
    
    iSourceInputBuffer->Data().SetLength( 0 );
    iSourceInputBuffer->SetPosition( 0 );
    
    iSourceInputBuffer->Data().Append( *aEncFrame );
    iSourceInputBuffer->Data().SetLength( aEncFrame->Length() );
    
    PRINT((_L("CProcDecoder::FillDecBufferL(), iSourceInputBuffer length = %d"),aEncFrame->Length()));
    if (iDoDecoding)
        {
        iDestInputBuffer->Data().SetLength(0);
        iDestInputBuffer->SetPosition(0); 
        
        }
            
    if ( iDoSampleRateChannelConversion )
        {
        iSampleRateChannelBuffer->Data().SetLength(0);
        iSampleRateChannelBuffer->SetPosition(0);
        }
        
    
    FeedCodecL( iSourceCodec, iSourceInputBuffer, iDestInputBuffer);
 
    if ( aDecBuffer )
        {
        // in case of EDstNotFilled from decoder, you may end up looping and to avoid memory leaks, you need to 
        // delete the previous allocated buffer. Alternative might be to not allocate a new one in this case.
        delete aDecBuffer;
        aDecBuffer = NULL;
        }
    aDecBuffer = iDecBuffer;    
            
    if (iDoDecoding)
        {
        iDestInputBuffer->Data().SetLength(0);
        iDestInputBuffer->SetPosition(0); 
        
        }
    
    if (iDecBuffer != 0 && iDecBuffer->Size() > 0)
        {
        PRINT((_L("CProcDecoder::FillDecBufferL() out with ETrue")));
        return ETrue;
          
        }
               
    PRINT((_L("CProcDecoder::FillDecBufferL() out with EFalse")));
    return EFalse;
        
    }


void CProcDecoder::ConstructL()
    {
    
    
    
    }

CProcDecoder::CProcDecoder()
    {
    
    }
    
    
    
void CProcDecoder::ConfigureAACPlusDecoderL()
    {

    
    RArray<TInt> config;
    
    if (iProperties.iAudioTypeExtension == EAudExtensionTypeEnhancedAACPlusParametricStereo)
        {
        
        // for sample rate and channel converter, the output from parametric stereo
        // is stereo, only the AAC part is incoded as mono
        iFromChannels = 2;
        }
    
    
    config.Append( iFromSampleRate);
    config.Append( iFromChannels );
   
    
    if (iProperties.iAACObjectType ==  EAudAACObjectTypeLC)
        {
        config.Append( 1 );     // {1 - LC, 3 - LTP}    
        }
    else if (iProperties.iAACObjectType ==  EAudAACObjectTypeLTP)
        {
        config.Append( 3 );     // {1 - LC, 3 - LTP}    
        }
    else
        {
        User::Leave(KErrNotSupported);
        }
    
    config.Append( 8192 );  //Size of PCM Samples generated by decoder
    
    config.Append( 1024 );  //Number of PCM Samples generated by decoder per frame
    
    config.Append( iFromSampleRate);        //Sampling freq of AAC Code decoder
    
    config.Append( 0 );     // not used ??
    
  
    config.Append( 0 );     // down sampled mode
    
    config.Append( 16 );    // Sample resolution: 16-bit resolution
    
    
        
    // NOTE!: for some reason, the sample rate of the output from AACPlus decoder used
    // to be a half of the proper output sampling rate. 
    // eAAC+ data consists of AAC band + enhancement band. The enhancement band is not included
    // in the rate indicated in the header (iFromSampleRate)
    // There was a workaround for the 
    // earlier problem not to increase the sampling rate.
    // Now, with 2006 releases it seems to work, and the sampling rate need to be doubled
    // for the sampling rate converter.
    
    if (iProperties.iAudioTypeExtension == EAudExtensionTypeEnhancedAACPlus)
        {
        iFromSampleRate *= 2; // for sample rate converter ->
        config.Append(iFromSampleRate);// Output sampling frequency
        config.Append( 5 ); //Type of extended object (5=SBR/HE AAC profile; 6=PS is present)
        
        }
    else if (iProperties.iAudioTypeExtension == EAudExtensionTypeEnhancedAACPlusParametricStereo)
        {
        iFromSampleRate *= 2; // for sample rate converter ->
        config.Append(iFromSampleRate);// Output sampling frequency
        config.Append( 6 ); //Type of extended object (5=SBR/HE AAC profile; 6=PS is present)
        }
    else
        {
        // AAC, in&out samplerates are equal, and extended object type is 0
        config.Append(iFromSampleRate);// Output sampling frequency
        config.Append( 0 );
        
        }
        

    TUid uid ={KUidMmfCodecAudioSettings}; // Use Uid reserved for codec configurations
    TRAPD( err, iSourceCodec->ConfigureL( uid, reinterpret_cast<TDesC8&>(config)));
            
    if ( err != KErrNone )
        {
        PRINT((_L("CProcDecoder::PrepareConverterL() error, Source codec config failed")));
        config.Close();
        User::Leave( err );
        }
    config.Close();
    
    }
    
    
void CProcDecoder::ConfigureMP3DecoderL()
    {

    
    RArray<TInt> config;
    
    TInt stereoToMono = 0;
    if (iToChannels == 1 || iFromChannels == 1)
        {
        stereoToMono = 1;
        }
    
    config.Append( stereoToMono); // stereo to mono
    config.Append( 0 ); //iLeftRight??
    config.Append( iDecimFactor ); //iDecimFactor
    config.Append( 0 ); //iConcealment
    config.Append( 0 ); //iSampleLength??
    config.Append( 0 ); //iSamplingFrequency
    
    TUid uid ={KUidMmfCodecAudioSettings}; // Use Uid reserved for codec configurations
    TRAPD( err, iSourceCodec->ConfigureL( uid, reinterpret_cast<TDesC8&>(config)));
            
    if ( err != KErrNone )
        {
        PRINT((_L("CProcDecoder::PrepareConverterL() error, Source codec config failed")));
        config.Close();
        User::Leave( err );
        }
    config.Close();
    }
    
    
void CProcDecoder::ReAllocBufferL( CMMFDataBuffer* aBuffer, TInt aNewMaxSize )
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
    
void CProcDecoder::FeedCodecL( CMMFCodec* aCodec, CMMFDataBuffer* aSourceBuffer, CMMFDataBuffer* aDestBuffer )
    {
    PRINT((_L("CProcDecoder::FeedCodecL() in")));
    TBool completed = EFalse;
    TCodecProcessResult result;

    while ( !completed )
        {

        if (iDoDecoding)
            {
            // decode and check the result
            result = DecodeL(aCodec, aSourceBuffer, aDestBuffer);
            }
        else
            {
            
            // no need for decoding, just perform sample rate and channel conversion
            result.iStatus = TCodecProcessResult::EProcessComplete;
            }
       
        
        switch ( result.iStatus )
            {
            case TCodecProcessResult::EProcessIncomplete:
                // Not all data from input was consumed (DecodeL updated buffer members), but output was generated
                    
                if ( iDoSampleRateChannelConversion )
                    {
                    if ( !iRateConverter )
                        {
                        PRINT((_L("CProcDecoder::FeedCodecL() error, no rate converter")));
                        User::Leave( KErrNotFound );
                        }
                    
                    // Convert buffer size in bytes    
                    TUint convertBufferSize = aDestBuffer->Data().Length();
                    
                    // Number of samples in the buffer
                    TUint inputSamples = convertBufferSize / (2 * iFromChannels);    // 16-bit samples
                    
                    PRINT((_L("CProcDecoder::FeedCodecL() converting %d samples"), inputSamples));
                    
                    if ( convertBufferSize > ( iDestInputBuffer->Data().MaxLength() - iDestInputBuffer->Position() ) )
                        {
                        ReAllocBufferL( iDestInputBuffer, convertBufferSize + iDestInputBuffer->Position() );
                        }

                    TInt outputSamples = iRateConverter->ConvertBufferL( (TInt16*) aDestBuffer->Data().Ptr(),
                        (TInt16*) iSampleRateChannelBuffer->Data().Ptr(), inputSamples );
                        
                    iSampleRateChannelBuffer->Data().SetLength( outputSamples * 2 * iToChannels );

                    UpdateOutputBufferL(iSampleRateChannelBuffer);
                    
                    }  
                else
                    {
                    if (iDoDecoding)
                        {

                        UpdateOutputBufferL(aDestBuffer);
                        }
                    }
                    
                break;

            case TCodecProcessResult::EProcessComplete:
                // all data from input was used and output was generated
                if ( iDoSampleRateChannelConversion )
                    {
                    if ( !iRateConverter )
                        {
                        PRINT((_L("CProcDecoder::FeedCodecL() error, no rate converter")));
                        User::Leave( KErrNotFound );
                        }
                        
                        
                    CMMFDataBuffer* src = 0;
                        
                    if (!iDoDecoding)
                        {
                        
                        // if decoding was not needed, 
                        // the input data for SR converter is in aSourceBuffer
                        src = aSourceBuffer;
                        }
                    else
                        {
                        // if decoding was needed, 
                        // the input data for SR converter is in iDestBuffer
                      
                        src = iDestInputBuffer;
                        }
                        
                    // Convert buffer size in bytes 
                    TUint convertBufferSize = src->Data().Length();
                    
                    // Number of samples in the buffer
                    TUint inputSamples = convertBufferSize / (2 * iFromChannels);    // 16-bit samples
                    
                    PRINT((_L("CProcDecoder::FeedCodecL() converting %d samples"), inputSamples));
   
                    if ( convertBufferSize > ( src->Data().MaxLength() - src->Position() ) )
                        {
                        ReAllocBufferL( src, convertBufferSize + src->Position() );
                        }
                        
                    TInt outputSamples = iRateConverter->ConvertBufferL( (TInt16*) src->Data().Ptr(),
                        (TInt16*) iSampleRateChannelBuffer->Data().Ptr(), inputSamples );
                        
                    iSampleRateChannelBuffer->Data().SetLength( outputSamples * 2 * iToChannels );

                    UpdateOutputBufferL(iSampleRateChannelBuffer);
 
                    }
                else
                    {
                    if (iDoDecoding)
                        {
                        UpdateOutputBufferL(aDestBuffer);
                        }
                    
                    }
        
                completed = ETrue;
                break;

            case TCodecProcessResult::EDstNotFilled:
                // need more input data, can't fill the output yet; put it back to the empty queue
                completed = ETrue;
             
                break;

            default:
                // EEndOfData, EProcessError, EProcessIncompleteRepositionRequest, EProcessCompleteRepositionRequest
                User::Leave( KErrUnknown );
            }

        }
    

    PRINT((_L("CProcDecoder::FeedCodecL() out")));
    }
    
TCodecProcessResult CProcDecoder::DecodeL( CMMFCodec* aCodec, CMMFDataBuffer* aInBuffer, CMMFDataBuffer* aOutBuffer)
    {
    PRINT((_L("CProcDecoder::DecodeL() in, input pos: %d, length: %d"), aInBuffer->Position(), aInBuffer->Data().Length() ));
    TCodecProcessResult result;

    result = aCodec->ProcessL (*aInBuffer, *aOutBuffer);

    switch (result.iStatus)
        {
        case TCodecProcessResult::EProcessComplete:
            // finished processing source data, all data in sink buffer
            PRINT((_L("CProcDecoder::FeedCodecL() EProcessComplete")));
            aInBuffer->SetPosition( 0 );
            aInBuffer->Data().SetLength(0);
            break;

        case TCodecProcessResult::EDstNotFilled:
            // the destination is not full, we need more data. Handled in caller
            PRINT((_L("CProcDecoder::FeedCodecL() EDstNotFilled")));
            aInBuffer->SetPosition( 0 );
            aInBuffer->Data().SetLength(0);
            break;

        case TCodecProcessResult::EProcessIncomplete:
            // the sink was filled before all the source was processed
            PRINT((_L("CProcDecoder::FeedCodecL() EProcessIncomplete")));
            aOutBuffer->SetPosition( 0 );
            aInBuffer->SetPosition( aInBuffer->Position() + result.iSrcBytesProcessed );
            break;

        default:
            break;
        }

    PRINT((_L("CProcDecoder::DecodeL() out, %d -> %d"),result.iSrcBytesProcessed, result.iDstBytesAdded));
    PRINT((_L("CProcDecoder::DecodeL() out, input pos: %d, length: %d"), aInBuffer->Position(), aInBuffer->Data().Length() ));
    return result;
    }
    
TBool CProcDecoder::GetIsSupportedSourceCodec()
    {
 
     TFourCC fourCC;
     TUid uID;
 
    if (iProperties.iAudioType == EAudAMR)
        {
        fourCC = TFourCC(KMMFFourCCCodeAMR);
        uID = TUid(KMmfAMRNBDecSWCodecUid);
        }
    else if (iProperties.iAudioType == EAudAAC_MPEG4)
        {
        // use eAAC+ also for AAC
        fourCC = TFourCC(KMMFFourCCCodeAACPlus);
        uID = TUid(KMmfUidCodecEnhAACPlusToPCM16);
        }
    else if (iProperties.iAudioType == EAudMP3)
        {
        fourCC = TFourCC(KMMFFourCCCodeMP3);
        uID = TUid(KMmfAdvancedUidCodecMP3ToPCM16);
        }
    else if (iProperties.iAudioType == EAudAMRWB)
        {
        fourCC = TFourCC(KMMFFourCCCodeAWB);
        uID = TUid(KMmfAMRWBDecSWCodecUid);
        }
    else
        {
        //Wav, no codec
        return ETrue;
        }
    
    
    
    _LIT8(emptyFourCCString, "    ,    ");
    TBufC8<9> fourCCString(emptyFourCCString);
    TPtr8 fourCCPtr = fourCCString.Des();
    TPtr8 fourCCPtr1(&fourCCPtr[0], 4);
    TPtr8 fourCCPtr2(&fourCCPtr[5], 4 );

    TFourCC srcFourCC(' ','P','1','6');
    fourCC.FourCC(&fourCCPtr1);
    srcFourCC.FourCC(&fourCCPtr2);

    TBool found = EFalse;
    TRAPD( err, found = CheckIfCodecAvailableL( fourCCPtr , uID));
    
    if (err == KErrNone)
        {
        return found;    
        }
    else
        {
        return EFalse;
        }
    }


void CProcDecoder::SetSourceCodecL()
    {
    PRINT((_L("CProcDecoder::SetSourceCodecL() in")));

    if ( !GetIsSupportedSourceCodec() )
        {
        PRINT((_L("CProcDecoder::SetSourceCodecL() error, unsupported codec")));
        User::Leave( KErrNotSupported );
        }

    if ( iSourceCodec )
        {
        delete iSourceCodec;
        iSourceCodec = NULL;
        }
    
    TFourCC destFourCC = KMMFFourCCCodePCM16;
    
    if (iProperties.iAudioType == EAudAMR)
        {
        iSourceCodec = CMMFCodec::NewL( KMmfAMRNBDecSWCodecUid );
    
        }
    else if (iProperties.iAudioType == EAudAAC_MPEG4)
        {
        // use eAAC+ also for AAC
        iSourceCodec = CMMFCodec::NewL( KMmfUidCodecEnhAACPlusToPCM16 );
    
        }
    else if (iProperties.iAudioType == EAudMP3)
        {
        iSourceCodec = CMMFCodec::NewL( KMmfAdvancedUidCodecMP3ToPCM16 );
    
        }
    else if (iProperties.iAudioType == EAudAMRWB)
        {
        iSourceCodec = CMMFCodec::NewL( KMmfAMRWBDecSWCodecUid );
    
        }
    else
        {
        // Wav, but no codec needed then
        }
    
    iReady = EFalse;

    PRINT((_L("CProcDecoder::SetSourceCodecL() out")));
    }
    
    
    
TBool CProcDecoder::CheckIfCodecAvailableL(const TDesC8& aCodecFourCCString, 
                                            const TUid& aCodecUId )
    {
    PRINT((_L("CProcDecoder::CheckIfCodecAvailableL() in")));
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
            PRINT((_L("CProcDecoder::CheckIfCodecAvailable() plugin found with Uid 0x%x"), plugInArray[i]->ImplementationUid().iUid ));
            if ( plugInArray[i]->ImplementationUid() == aCodecUId )
                {
			    //match accepted
                PRINT((_L("CProcDecoder::CheckIfCodecAvailable() plugin accepted")));
			    found = ETrue;
                }
            }
        }
    else
        {
        PRINT((_L("CProcDecoder::CheckIfCodecAvailable() Error in ListImp.: %d"), err));
        //no match
        found = EFalse;
        }

    CleanupStack::PopAndDestroy();  //plugInArray
    PRINT((_L("CProcDecoder::CheckIfCodecAvailableL() out")));
    return found;
    }
    
TBool CProcDecoder::UpdateOutputBufferL(CMMFDataBuffer* aInBuffer)
    {
    if (iDecBuffer == 0)
        {
        iDecBuffer = HBufC8::NewL(aInBuffer->BufferSize());
        iDecBuffer->Des().Append(aInBuffer->Data());
        }
    else
        {
        iDecBuffer = iDecBuffer->ReAlloc(iDecBuffer->Size()+aInBuffer->BufferSize());
        iDecBuffer->Des().Append(aInBuffer->Data());
        }
    
    aInBuffer->Data().SetLength( 0 );
    aInBuffer->SetPosition( 0 );    
    
    return ETrue;
    }
    
