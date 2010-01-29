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



#include "ProcWAVInFileHandler.h"
#include "ProcWAVFrameHandler.h"
#include "AudPanic.h"
#include "ProcTools.h"
#include "audconstants.h"



CProcWAVInFileHandler* CProcWAVInFileHandler::NewL(const TDesC& aFileName, 
                                                   RFile* aFileHandle,
                                                   CAudClip* aClip, 
                                                   TInt aReadBufferSize,
                                                   TInt aTargetSampleRate, 
                                                   TChannelMode aChannelMode) 
    {

    CProcWAVInFileHandler* self = NewLC(aFileName, aFileHandle, aClip, 
                    aReadBufferSize, aTargetSampleRate, aChannelMode);
    CleanupStack::Pop(self);
    return self;
    }

CProcWAVInFileHandler* CProcWAVInFileHandler::NewLC(const TDesC& aFileName, 
                                                    RFile* aFileHandle,
                                                    CAudClip* aClip, 
                                                    TInt aReadBufferSize,
                                                    TInt aTargetSampleRate, 
                                                    TChannelMode aChannelMode) 
    {
    
    CProcWAVInFileHandler* self = new (ELeave) CProcWAVInFileHandler();
    CleanupStack::PushL(self);
    self->ConstructL(aFileName, aFileHandle, aClip, aReadBufferSize, aTargetSampleRate, aChannelMode);
    return self;
    }

CProcWAVInFileHandler::CProcWAVInFileHandler() : CProcInFileHandler(), iNumberofSamplesInFrame(1)
    {
    
    
    }

void CProcWAVInFileHandler::GetPropertiesL(TAudFileProperties* aProperties) 
    {

    if (iProperties != 0)
        {
        *aProperties = *iProperties;
        return;
        }
    
    
    const TInt KFrameDurationMilli = 20;
    const TInt KFramesPerSecond = 50;
    
    if (iFileOpen) 
        {
        aProperties->iFileFormat = EAudFormatUnrecognized;
        aProperties->iAudioType = EAudTypeUnrecognized;
        aProperties->iAudioTypeExtension = EAudExtensionTypeNoExtension;
        aProperties->iBitrate = 0;
        aProperties->iBitrateMode = EAudBitrateModeNotRecognized;
        aProperties->iChannelMode = EAudChannelModeNotRecognized;
        aProperties->iDuration = 0;
        aProperties->iSamplingRate = 0;
        aProperties->iFrameLen = 0;
        aProperties->iFrameCount = 0;

        TInt oldPos = iFilePos;

        // check the validity of WAVE header

        TBuf8<4> chunckID;
        TBuf8<4> format;
        TBuf8<4> fmt;
        TBuf8<4> data;
        
        TBuf8<1> audioFormat;

        BufferedFileRead(0, chunckID);
        BufferedFileRead(8, format);
        BufferedFileRead(12, fmt);
        BufferedFileRead(20, audioFormat);
        BufferedFileRead(36, data);


        if (chunckID.Compare(_L8("RIFF")) == 0 &&
            format.Compare(_L8("WAVE")) == 0 &&
            fmt.Compare(_L8("fmt ")) == 0 &&
            data.Compare(_L8("data")) == 0 &&
            audioFormat[0] == 0x01)
            {
            aProperties->iFileFormat = EAudFormatWAV;
            aProperties->iAudioType = EAudWAV;
            }
        else
            {
            aProperties->iFileFormat = EAudFormatUnrecognized;
            User::Leave(KErrNotSupported);
            return;

            }
        
        TBuf8<1> numChannels;
        BufferedFileRead(22, numChannels);
        if (numChannels[0] == 0x01)
            {
            aProperties->iChannelMode = EAudSingleChannel;
            }
        else if (numChannels[0] == 0x02)
            {
            aProperties->iChannelMode = EAudStereo;
            }
        else
            {
            aProperties->iFileFormat = EAudFormatUnrecognized;
            User::Leave(KErrNotSupported);
            return;
            }

        TBuf8<4> sr;
        BufferedFileRead(24, sr);

        TBuf8<8> tmpBin;
        TBuf8<8*4> srBin;
    
        // little endian:
        for (TInt a = 3 ; a >= 0; a--)
            {
            ProcTools::Dec2Bin(sr[a], tmpBin);
            srBin.Append(tmpBin);
            }
        
        TUint srDec = 0;
        ProcTools::Bin2Dec(srBin, srDec);

        aProperties->iSamplingRate = srDec;
        
        // Check that the sample rate is supported    
        if( (aProperties->iSamplingRate != KAedSampleRate8kHz) &&
            (aProperties->iSamplingRate != KAedSampleRate11kHz) &&
            (aProperties->iSamplingRate != KAedSampleRate16kHz) &&
            (aProperties->iSamplingRate != KAedSampleRate22kHz) &&
            (aProperties->iSamplingRate != KAedSampleRate24kHz) &&
            (aProperties->iSamplingRate != KAedSampleRate32kHz) &&
            (aProperties->iSamplingRate != KAedSampleRate44kHz) &&
            (aProperties->iSamplingRate != KAedSampleRate48kHz) )
            {
            User::Leave(KErrNotSupported);
            }

        iNumberofSamplesInFrame = srDec/(KFramesPerSecond/numChannels[0]);
        if (iNumberofSamplesInFrame%2 != 0) iNumberofSamplesInFrame++;
        
        tmpBin.Delete(0, tmpBin.Length());

        TBuf8<4> byteRate;
        TBuf8<8*4> byteRateBin;
        TUint byteRateDec = 0;

        BufferedFileRead(28, byteRate);
        for (TInt b = 3 ; b >= 0; b--)
            {
            ProcTools::Dec2Bin(byteRate[b], tmpBin);
            byteRateBin.Append(tmpBin);
            }
        ProcTools::Bin2Dec(byteRateBin, byteRateDec);

        aProperties->iBitrate = byteRateDec*8;

        TBuf8<1> numberOfBitsPerSample;
        BufferedFileRead(34, numberOfBitsPerSample);
        aProperties->iNumberOfBitsPerSample = numberOfBitsPerSample[0];

        // other bit depths can be added later if (ever) needed
        if (aProperties->iNumberOfBitsPerSample != 16 &&
            aProperties->iNumberOfBitsPerSample != 8)
            {
            aProperties->iFileFormat = EAudFormatUnrecognized;
            User::Leave(KErrNotSupported);
            return;
            }
            
        if (aProperties->iNumberOfBitsPerSample == 16)
            {
            iNumberofSamplesInFrame *= 2;
            }

        TBuf8<4> dataSize;
        BufferedFileRead(40, dataSize);

        TBuf8<8*4> dataSizeBin;
        TUint dataSizeDec = 0;
        tmpBin.Delete(0, tmpBin.Length());

        for (TInt c = 3 ; c >= 0; c--)
            {
            ProcTools::Dec2Bin(dataSize[c], tmpBin);
            dataSizeBin.Append(tmpBin);
            }
        ProcTools::Bin2Dec(dataSizeBin, dataSizeDec);
        
        TTimeIntervalMicroSeconds durationMicro((TInt64)(TInt)((TReal(dataSizeDec)/byteRateDec)*1000000));

        aProperties->iDuration = durationMicro;
        aProperties->iBitrateMode = EAudConstant;
        aProperties->iFrameLen = iNumberofSamplesInFrame;
        
        aProperties->iFrameDuration = KFrameDurationMilli*1000;
        
        aProperties->iFrameCount = (TInt)dataSizeDec/iNumberofSamplesInFrame;
        aProperties->iNumFramesPerSample = 1;



        BufferedFileSetFilePos(oldPos);
        }
    else 
        {
        TAudPanic::Panic(TAudPanic::EInternal);
        }

    
    if (iProperties == 0)
        {
        iProperties = new (ELeave) TAudFileProperties;
        *iProperties = *aProperties;
        }
    
    
    }

TBool CProcWAVInFileHandler::SeekAudioFrame(TInt32 aTime) 
    {
    
    TInt frameLengthMilliSeconds = ProcTools::MilliSeconds(iProperties->iFrameDuration);
    
    TInt framesFromStart = aTime/frameLengthMilliSeconds;

    TInt bytesFromStart = framesFromStart*iNumberofSamplesInFrame;

    const TInt KWAVHeaderLength = 44;

    BufferedFileSetFilePos(bytesFromStart+KWAVHeaderLength);

    if (!iFileOpen) 
        {
        TAudPanic::Panic(TAudPanic::EInternal);
        }

    iCurrentTimeMilliseconds = aTime;

    return ETrue;
    }    

TBool CProcWAVInFileHandler::SeekCutInFrame() 
    {

    iCurrentTimeMilliseconds = iCutInTime;
    return SeekAudioFrame(iCutInTime);
    }
    


void CProcWAVInFileHandler::ConstructL(const TDesC& aFileName, 
                                       RFile* aFileHandle,
                                       CAudClip* aClip, 
                                       TInt aReadBufferSize,
                                       TInt aTargetSampleRate, 
                                       TChannelMode aChannelMode) 
    {
    
    
    
    

    iTargetSampleRate = aTargetSampleRate;
    iChannelMode = aChannelMode;

    InitAndOpenFileL(aFileName, aFileHandle, aReadBufferSize);

    
    if (aClip != 0)
        {
        iCutInTime = ProcTools::MilliSeconds(aClip->CutInTime());
        
        }


    TAudFileProperties prop;
    GetPropertiesL(&prop);

    if (iProperties != 0)
        {
        TInt samplesIn20ms = ((iProperties->iSamplingRate) * 
            (iProperties->iNumberOfBitsPerSample)/8)/50;
        
        if (iProperties->iChannelMode == EAudStereo)
            {
            samplesIn20ms *= 2;

            }
        if (samplesIn20ms % 2 != 0) samplesIn20ms--;
        
        iSilentFrame = HBufC8::NewL(samplesIn20ms);
        
        if (iProperties->iNumberOfBitsPerSample == 16)
        {
            iSilentFrame->Des().SetLength(samplesIn20ms);
            iSilentFrame->Des().Fill(0);
            
        }
        else if(iProperties->iNumberOfBitsPerSample == 8)
        {
            iSilentFrame->Des().SetLength(samplesIn20ms);
            iSilentFrame->Des().Fill(128);
            
        }
        iSilentFrameDuration = 20;
        }
    else
        {
        User::Leave(KErrNotSupported);
        }
        
    iClip = aClip;
    
    iFrameHandler = CProcWAVFrameHandler::NewL(iProperties->iNumberOfBitsPerSample);
    
    // Generate a decoder ----------------------->

    iDecoder = CProcDecoder::NewL();
    
    iDecodingPossible = iDecoder->InitL(*iProperties, aTargetSampleRate, aChannelMode);
    

    // <----------------------- Generate a decoder 

    if (iClip != 0 && iClip->Normalizing())
        {
        SetNormalizingGainL(iFrameHandler);    
        }

    }
    
    
TBool CProcWAVInFileHandler::GetEncAudioFrameL(HBufC8*& aFrame, TInt& aSize, TInt32& aTime)
    {
    
    if (!iFileOpen) 
        {
        TAudPanic::Panic(TAudPanic::EInternal);
        }
    
    TInt numberOfBytesInFrame = iNumberofSamplesInFrame;
    TInt bufferSize = numberOfBytesInFrame;
    if (iProperties->iNumberOfBitsPerSample == 8)
        {
        // need to expand to 16 bits per sample since encoders and MMF sample rate converter assumes 16-bit signed input
        bufferSize *= 2;
        }
    

    aFrame = HBufC8::NewL(bufferSize);
    
    TPtr8 tmpDes((TPtr8)aFrame->Des());

    BufferedFileRead((TPtr8&)tmpDes , numberOfBytesInFrame);

    if (aFrame->Des().Length() < numberOfBytesInFrame)
        {
        delete aFrame;
        aFrame = 0;
        return EFalse;
        }
    aTime = ProcTools::MilliSeconds(iProperties->iFrameDuration);
    iCurrentTimeMilliseconds += aTime;
    
    TRAPD(err, ManipulateGainL(aFrame));
    
    if (err != KErrNone)
        {
        // something went wrong with the gain manipulation
        // continue by returning the original frame
        }
    
    if (iProperties->iNumberOfBitsPerSample == 8)
        {
        // need to expand to 16 bits per sample since encoders and MMF sample rate converter assumes 16-bit signed input
        TUint8* buffer = const_cast<TUint8*>(aFrame->Ptr());
        // start from the end to avoid overlapping
        for ( TInt i = numberOfBytesInFrame-1; i >= 0 ; i-- )
            {
            buffer[i*2+1] = TInt8(buffer[i]) - 128; // 8-bit Wav's are unsigned, 16-bit Wav's are signed
            buffer[i*2] = 0;
            }
        tmpDes.SetLength(bufferSize);

        }
    aSize = aFrame->Length();
    
    
    return ETrue;
    
    }
    
    


CProcWAVInFileHandler::~CProcWAVInFileHandler() 
    {

    ResetAndCloseFile();

    if (iSilentFrame != 0) delete iSilentFrame;

    delete iFrameHandler;
    
    delete iDecoder;

    }


TBool CProcWAVInFileHandler::SetNormalizingGainL(const CProcFrameHandler* aFrameHandler)
    {

    HBufC8* point = 0;
    TInt siz;
    TInt32 tim = 0;
    TInt8 margin = 0;
    TInt minMargin = KMaxTInt;
    
    while(GetEncAudioFrameL(point, siz, tim)) 
                    
        {
        aFrameHandler->GetNormalizingMargin(point, margin);
                    
        delete point;
        point = 0;
        
        if (minMargin > margin)
            {
            minMargin = margin;
            }
    
        }

    iNormalizingMargin = static_cast<TInt8>(minMargin);

    return ETrue;

    }
    
