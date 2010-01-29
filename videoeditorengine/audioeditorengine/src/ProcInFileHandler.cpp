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



#include <f32file.h>
#include "AudPanic.h"
#include "ProcInFileHandler.h"
#include "AudClipInfo.h"
#include "ProcTools.h"
#include "audconstants.h"

// Debug print macro
#if defined _DEBUG 
#include <e32svr.h>
#define PRINT(x) RDebug::Print x;
#else
#define PRINT(x)
#endif

CProcInFileHandler::~CProcInFileHandler() 
    {

    if (iFileOpen && iOwnsFile)
        {
        iFile.Close();
        iFs.Close();
        }


    if (iProperties != 0)
        {
        delete iProperties;
        iProperties = 0;
        }


    delete iRawSilentFrame;
    
    delete iInputBuffer;
    
    delete iWavFrameHandler;

    }


TBool CProcInFileHandler::DecodingRequired()
    {
    return iDecodingRequired;
    }
    

TInt CProcInFileHandler::GetDecodedFrameSize()
    {
    if (iProperties == 0)
        {
        return 0;
        }
    else
        {
        
        // divided by 1000000/2 cause time is in 
        // microseconds and 16 bits are used for one sample
#ifdef EKA2
        TInt frameSize = (iProperties->iFrameDuration.Int64())*(iTargetSampleRate)/(1000000/2);
#else
        TInt frameSize = (iProperties->iFrameDuration.Int64().GetTInt())*(iTargetSampleRate)/(1000000/2);
#endif        
        
        
        if (iChannelMode == EAudStereo)
            {
            frameSize *= 2;
            }
        
        return frameSize;
        }
    
    
    }


void CProcInFileHandler::SetDecodingRequired(TBool aDecodingRequired)
    {
    iDecodingRequired = aDecodingRequired;
    
    }

TBool CProcInFileHandler::GetAudioFrameL(HBufC8*& aFrame, TInt& aSize, TInt32& aTime, TBool& aRawFrame) 
    {

    if (iDecodingRequired)
        {
        aRawFrame = ETrue;
        return GetRawAudioFrameL(aFrame, aSize, aTime);
        }
    else
        {
        aRawFrame = EFalse;
        return GetEncAudioFrameL(aFrame, aSize, aTime);
        
        }
    }
    
    
TBool CProcInFileHandler::GetRawAudioFrameL(HBufC8*& aFrame, TInt& aSize, TInt32& aTime)
    {
    
    
    while (iInputBuffer == 0 || iInputBuffer->Length() < iRawFrameLength)
        {
        TBool ret = GetOneRawAudioFrameL(aFrame, aSize, aTime);
        if (!ret)
            {
            return ret;
            }
        
        WriteDataToInputBufferL(*aFrame);
        delete aFrame;
        aFrame = 0;
    
        }
        
    if (iInputBuffer->Length() >= iRawFrameLength)
        {
        aFrame = HBufC8::NewL(iRawFrameLength);
        aFrame->Des().Append(iInputBuffer->Left(iRawFrameLength));
        
        
        TInt bytesInSecond = iTargetSampleRate*2;
            
        if (iChannelMode == EAudStereo)
            {
            bytesInSecond *= 2;
            }
        
        
        // Fix for synchronizing problem ---------------------------------->
        // If the accurate frame length cannot be represented in milliseconds
        // store the remainder and increase the output frame lenght by one ms 
        // when needed. Accuracy depends on sampling rate
        
        TReal accurateFrameLen = TReal(aFrame->Length()*1000)/bytesInSecond;
        aTime = TUint((aFrame->Length()*1000)/bytesInSecond);
        
        iFrameLenRemainderMilli += accurateFrameLen - aTime;
        
        if (iFrameLenRemainderMilli > 1)
            {
            aTime += 1;
            iFrameLenRemainderMilli -= 1;
            }
        // <---------------------------------- Fix for synchronizing problem
            
        iInputBuffer->Des().Delete(0, iRawFrameLength);

        
        }
    
    return ETrue;
    
    
    }

    
TBool CProcInFileHandler::GetOneRawAudioFrameL(HBufC8*& aFrame, TInt& aSize, TInt32& aTime)
    {
    

    TInt size = 0;
        
    TInt32 time = 0;
        
    HBufC8* frame = 0;
        
    TBool encFrameRead = GetEncAudioFrameL(frame, size, time);    
    
    if (encFrameRead) 
        {
        CleanupStack::PushL(frame);
        }
    
    aTime = time;
    
    if (!encFrameRead)
        {
        return EFalse;
        }
    
    TRAPD(err, iDecoder->FillDecBufferL(frame, aFrame));
    if (err == KErrNotFound)
        {
        // S60 audio decoders seem to leave with KErrNotFound if they find problems in input clips. 
        // However, sometimes the problem is only in one frame, e.g. the 1st frame may contain some metadata that can't be decoded,
        // but decoding can continue after that. But if there are many errors, the clip is most likely unusable.
	    PRINT((_L("CProcInFileHandler::GetOneRawAudioFrameL() iDecoder->FillDecBufferL leaved with %d"),err));
        if ( iDecoderErrors > 0 )
            {
            // several errors, leave, but change the error code to more readable
    	    PRINT((_L("CProcInFileHandler::GetOneRawAudioFrameL() leave with %d"),KErrCorrupt));
            User::Leave(KErrCorrupt);
            }
        iDecoderErrors++;
        }
    else if (err == KErrNone)
        {
        // keep filling the decoder buffer; the decoder seem to leave with KErrNone if it can't get enough data, ignore the leave
        }
    else
        {
        // some other error
	    PRINT((_L("CProcInFileHandler::GetOneRawAudioFrameL() iDecoder->FillDecBufferL leaved with %d, leaving"),err));
        User::Leave(err);
        }
    
    if (encFrameRead) 
        {
        CleanupStack::PopAndDestroy(frame);
        frame = 0;
        
        }
    
    while (aFrame == 0 || aFrame->Size() == 0)
        {
        
        encFrameRead = GetEncAudioFrameL(frame, size, time);
    
        if (encFrameRead) 
            {
            CleanupStack::PushL(frame);
            }
        else
            {
            return EFalse;
            }
    
        
        aTime += time;
        
        
        
        iDecoder->FillDecBufferL(frame, aFrame);
        if (encFrameRead) 
            {
            CleanupStack::PopAndDestroy(frame);
            frame = 0;
            }        
        
        }
    
    
    aSize = aFrame->Length();
    
    if (iProperties->iAudioTypeExtension != EAudExtensionTypeNoExtension) 
        {
        
        // AACPlus is always decoded, therefore the gain manipulation can be done
        // in time domain
        
        ManipulateGainL(aFrame);
        aSize = aFrame->Length();
        
        
        }
    
    
    return ETrue;
        
    }


TBool CProcInFileHandler::SetRawAudioFrameSize(TInt aSize)
    {
    
    const TInt KMaxRawFrameSize = 4096;
    if (aSize > 0 && aSize <= KMaxRawFrameSize)
        {
        iRawFrameLength = aSize;
        return ETrue;
        }
        
    return EFalse;
    }

TBool CProcInFileHandler::GetSilentAudioFrameL(HBufC8*& aFrame, TInt& aSize, TInt32& aTime, TBool& aRawFrame)
    {
    
    if (iDecodingRequired)
        {
        aRawFrame = ETrue;
        return GetRawSilentAudioFrameL(aFrame, aSize, aTime);
        
        }
        
    else
        {
        aRawFrame = EFalse;
        return GetEncSilentAudioFrameL(aFrame, aSize, aTime);
        }
    }
    
    


TBool CProcInFileHandler::GetRawSilentAudioFrameL(HBufC8*& aFrame, TInt& aSize, TInt32& aDuration)
    {
    
    
    aFrame = HBufC8::NewL(iRawFrameLength);    
    aFrame->Des().Fill(0, aFrame->Des().MaxLength());

    TInt bytesInSecond = iTargetSampleRate*2;
            
    if (iChannelMode == EAudStereo)
        {
        bytesInSecond *= 2;
        }
    
    aDuration = TUint((aFrame->Length()*1000)/bytesInSecond);
    aSize = aFrame->Length();
  
    
    // input buffer is created only if needed
    if (iInputBuffer)
        {
        iInputBuffer->Des().Delete(0, iInputBuffer->Size());
        
        }
    return ETrue;
        
    }

TBool CProcInFileHandler::GetEncSilentAudioFrameL(HBufC8*& aFrame, TInt& aSize, TInt32& aDuration)
    {
    
    if (iSilentFrame == 0)
        {
        return EFalse;
        }
    
    aFrame = HBufC8::NewL(iSilentFrame->Size());    
    aFrame->Des().Append(iSilentFrame->Des());

    aDuration = iSilentFrameDuration;
    aSize = iSilentFrame->Size();
    return ETrue;

    }


CProcInFileHandler::CProcInFileHandler() : iFileOpen(EFalse)
    {
    
    

    }
    
TInt32 CProcInFileHandler::GetCurrentTimeMilliseconds() 
    {
    return iCurrentTimeMilliseconds;

    }

// default implementation for files that do not have buffered bytes
TBool CProcInFileHandler::ReadAudioDecoderSpecificInfoL(HBufC8*& /*aBytes*/, TInt /*aBufferSize*/)
    {
    return EFalse;
    }

TBool CProcInFileHandler::OpenFileForReadingL() 
    {   

    if (iFileOpen)
        {
        User::Leave(KErrGeneral);
        }
 
    TInt err = iFs.Connect();
    if (err != KErrNone) 
        {
        iFileOpen = EFalse;
        User::Leave(err);
        }

    err=iFile.Open(iFs, iFileName->Des(), EFileShareReadersOnly);
    if (err != KErrNone) 
        {
        err=iFile.Open(iFs, iFileName->Des(), EFileShareAny);
        }
    if (err != KErrNone) 
        {
        iFileOpen = EFalse;
        iFs.Close();
        User::Leave(err);
        }
    else 
        {
        iFileOpen = ETrue;
        return ETrue;
        }

    return EFalse;

    }

TBool CProcInFileHandler::
SetPropertiesL(TAudFileProperties aProperties) 
{
  if(iProperties == 0)
    iProperties = new (ELeave) TAudFileProperties();

  iProperties->iAudioType = aProperties.iAudioType;
  iProperties->iBitrate = aProperties.iBitrate;
  iProperties->iBitrateMode = aProperties.iBitrateMode;
  iProperties->iChannelMode = aProperties.iChannelMode;
  iProperties->iDuration = 0;
  iProperties->iFileFormat = aProperties.iFileFormat;
  iProperties->iSamplingRate = aProperties.iSamplingRate;
  iProperties->iFrameLen = aProperties.iFrameLen;
  iProperties->iFrameDuration = aProperties.iFrameDuration;
  iProperties->iFrameCount = aProperties.iFrameCount;
  
  return (ETrue);
}

TBool CProcInFileHandler::SetPriority(TInt aPriority)
    {
    if (aPriority < 0) return EFalse;

    iPriority = aPriority;

    return ETrue;

    }
TInt CProcInFileHandler::Priority() const
    {
    return iPriority;

    }

TBool CProcInFileHandler::CloseFile() 
    {

    if (iOwnsFile)
    {        
        iFile.Close();
        iFs.Close();
        iFileOpen = EFalse;
    }
    
    return ETrue;

    }



TBool CProcInFileHandler::InitAndOpenFileL(const TDesC& aFileName, 
                                           RFile* aFileHandle,
                                           TInt aReadBufferSize) 
    {

    
    iBufferStartOffset = 0;
    iBufferEndOffset = 0;
    iCutInTime = 0;

    if (!aFileHandle)
    {        
        iFileName = HBufC::NewL(aFileName.Length());
        *iFileName = aFileName;
    }
        
    iReadBufferSize = aReadBufferSize;
    iReadBuffer = HBufC8::NewL(iReadBufferSize);
    
    const TInt KVedACSizeAMRBuffer = 320;
    
    // [JK]: Moved from OpenFileForReadingL
    // AMR buffer is the smallest possible
    // buffersize fill be increased if needed
    iRawFrameLength = KVedACSizeAMRBuffer;
    iInputBuffer = HBufC8::NewL(iRawFrameLength);

    if (aFileHandle)
    {
        iFile = *aFileHandle;
        iOwnsFile = EFalse;
        iFileOpen = ETrue;
        return 0;
    }
    
    TBool err = OpenFileForReadingL();
    if (!err) iFileOpen = ETrue;
    
    iOwnsFile = ETrue;
    
    return err;

    }

void CProcInFileHandler::ResetAndCloseFile()
    {

    if (iFileOpen) 
        {
        CloseFile();
        }
    if (iFileName != 0)
        delete iFileName;
    iFileName = 0;
    if (iReadBuffer != 0)
        delete iReadBuffer;
    iReadBuffer = 0;
    
    delete iRawSilentFrame;
    iRawSilentFrame = 0;
    


    }

TInt CProcInFileHandler::BufferedFileRead(TInt aPos,TDes8& aDes) 
    {
    TInt bufSize;
    
    if (!iFileOpen) 
        {
        TAudPanic::Panic(TAudPanic::EInternal);
        }
    
    TBool readingNeeded = EFalse;
    
    if (aPos < iBufferStartOffset || aPos + aDes.MaxSize() > iBufferEndOffset) 
        {
        readingNeeded = ETrue;
        }
    
    if(readingNeeded) 
        {
        TInt fSize;

        iFile.Size(fSize);
        if(aPos >= fSize) 
            {
            aDes.SetLength(0);
            return 0;
            }

        TPtr8 tmpDes((TPtr8)iReadBuffer->Des());

        iFile.Read(aPos, (TPtr8&)tmpDes, iReadBufferSize);

        iBufferStartOffset = aPos;
        iBufferEndOffset = aPos+iReadBuffer->Des().Size()-1;        
        }

    if (iReadBuffer->Size() == 0) 
        {
        return 0;
        }
    else 
        {
        bufSize = Min(iReadBuffer->Des().Length(), aDes.MaxSize());
        aDes.Copy(iReadBuffer->Des().Mid(aPos-iBufferStartOffset, bufSize));
        iFilePos = aPos+aDes.Size();

        return aDes.Size();
        }
    }

TInt CProcInFileHandler::BufferedFileRead(TDes8& aDes) 
    {

    if (!iFileOpen) 
        {
        TAudPanic::Panic(TAudPanic::EInternal);
        }

    return BufferedFileRead(iFilePos, aDes);
    

    }

TInt CProcInFileHandler::BufferedFileRead(TDes8& aDes,TInt aLength) 
    {
    TInt bufSize;

    if (!iFileOpen) 
        {
        TAudPanic::Panic(TAudPanic::EInternal);
        }
    
    aDes.Zero();

    TBool readingNeeded = EFalse;
    
    if (iFilePos < iBufferStartOffset || iFilePos + aLength-1 > iBufferEndOffset) 
        {
        readingNeeded = ETrue;
        }
    
    if(readingNeeded) 
        {
        TPtr8 tmpDes((TPtr8)iReadBuffer->Des());

        iFile.Read(iFilePos, (TPtr8&)tmpDes, iReadBufferSize);
        
        if (iReadBuffer->Size() == 0) return 0; 
        iFile.Seek(ESeekStart, iFilePos);
        iBufferStartOffset = iFilePos;
        iBufferEndOffset = iFilePos+iReadBuffer->Des().Size()-1;        
        }
    bufSize = Min(iReadBuffer->Des().Length(), aLength);
    aDes.Copy(iReadBuffer->Des().Mid(iFilePos-iBufferStartOffset, bufSize));
    iFilePos = iFilePos+aDes.Size();
    return aDes.Size();

    }

TInt CProcInFileHandler::BufferedFileReadChar(TInt aPos, TUint8& aChar)
    {

    TBuf8<1> cha;
    BufferedFileRead(aPos, cha);

    if (cha.Size() == 1)
        {
        aChar = cha[0];
        return 1;
        }
    else
        {
        return 0;
        }

    }

TBool CProcInFileHandler::BufferedFileSetFilePos(TInt aPos) 
    {

    
    TInt fileSize = 0;
    iFile.Seek(ESeekEnd, fileSize);

    iFile.Seek(ESeekStart, aPos);

    if (aPos == 0)
        {
        iBufferStartOffset = 0;
        iBufferEndOffset = 0;

        }
    
    if (aPos < 0) 
        {
        return EFalse;
        }
    else if (aPos > fileSize) 
        {
        return EFalse;
        }
    else 
        {
        iFilePos = aPos;
        }
    return ETrue;
    }

TInt CProcInFileHandler::BufferedFileGetFilePos() 
    {

    return iFilePos;

    }

TInt CProcInFileHandler::BufferedFileGetSize()
    {

    TInt fileSize = 0;
    iFile.Size(fileSize);

    return fileSize;


    }

    
TInt8 CProcInFileHandler::NormalizingMargin() const 
    {
    return iNormalizingMargin;
    }
    
TBool CProcInFileHandler::ManipulateGainL(HBufC8*& aFrameIn) 
    {
    
    if (iClip == 0)
        {
        return ETrue;
        }
                
    if (iProperties->iAudioTypeExtension != EAudExtensionTypeNoExtension)
        {
        
        // gain manipulation with AACPlus is done in time domain
        
        if (iWavFrameHandler == 0)
            {
            
            const TInt KBitDepth = 16;
            iWavFrameHandler = CProcWAVFrameHandler::NewL(KBitDepth);
            }
            
        
        }
                
    
    // check if the clip is normalized
    TInt8 normalizingGain1 = iNormalizingMargin;
    
    // check what is the current gain according to dynamic level marks
    TInt8 newGain1 = GetGainNow();
    
    
    // the combination of dynamic level marks + normalizing
    TInt8 normalizedGain1 = 0;
    if ( ((TInt)newGain1+normalizingGain1) > (TInt)KMaxTInt8)
        {
        normalizedGain1 = KMaxTInt8;
        }
    else
        {
        normalizedGain1 = static_cast<TInt8>(newGain1+normalizingGain1); 
        }
        
    
    if (normalizedGain1 != 0)
        {
        HBufC8* frameOut = 0;    
        // if we need to adjust gain...
        TBool manip = ETrue;
        if (iProperties->iAudioTypeExtension != EAudExtensionTypeNoExtension)
            {
            
            manip = iWavFrameHandler->ManipulateGainL(aFrameIn, frameOut, normalizedGain1);
            }
        else
            {
            manip = iFrameHandler->ManipulateGainL(aFrameIn, frameOut, normalizedGain1);    
            }
        
        
        if (manip)
            {
        
            CleanupStack::PushL(aFrameIn);
            CleanupStack::PushL(frameOut);
            
            if (frameOut->Size() > aFrameIn->Size())
                {
                // if manipulated frame was longer than the original
                aFrameIn = aFrameIn->ReAllocL(frameOut->Size());
                
                }
            aFrameIn->Des().Delete(0, aFrameIn->Size());
            aFrameIn->Des().Copy(frameOut->Des());
            
            CleanupStack::PopAndDestroy(frameOut);
            CleanupStack::Pop(); // aFrameIn
            }
        
    
        }
    else
        {
        return EFalse;
        }
        
    return ETrue;
    }

TInt8 CProcInFileHandler::GetGainNow() 
    {
    TInt markAmount = iClip->DynamicLevelMarkCount();
    TTimeIntervalMicroSeconds ti(0);
    TAudDynamicLevelMark previous(ti,0);
    //TInt32 durationMilliseconds =     (aClip->Info()->Properties().iDuration.Int64()/1000).GetTInt();
    TAudDynamicLevelMark next(iClip->Info()->Properties().iDuration,0);

    for (TInt a = 0 ; a < markAmount ; a++) 
        {
        TAudDynamicLevelMark markNow = iClip->DynamicLevelMark(a);
        
        if (ProcTools::MilliSeconds(markNow.iTime) > iCurrentTimeMilliseconds) 
            {
            next = markNow;
            break;
            }

        previous = markNow;

        }

    TInt32 previousMilli = ProcTools::MilliSeconds(previous.iTime);
    
    TInt32 nextMilli = ProcTools::MilliSeconds(next.iTime);
    
    TInt32 timeDifference = nextMilli - previousMilli;
    
    TInt8 previousLevel = previous.iLevel;
    TInt8 nextLevel = next.iLevel;
    
    // If the levels are positive then the amount of gain needs to be reduced
    if (previousLevel > 0)
        {
        previousLevel /= KAedPositiveGainDivider;
        }
    if (nextLevel > 0)
        {
        nextLevel /= KAedPositiveGainDivider;
        }

    if (timeDifference == 0) 
        {
        return previousLevel;
        }
        
    TInt32 fraction = ((iCurrentTimeMilliseconds-previousMilli)*100)/timeDifference;
    TInt8 newGain = 0;
    
    TInt8 levelDifference = static_cast<TInt8>(Abs(nextLevel - previousLevel));
    TInt8 inc = 0;
    if (fraction > 0) 
        {
        inc = static_cast<TInt8>((levelDifference*fraction)/100);
        }

    if (nextLevel - previousLevel >= 0) 
        {
        newGain = static_cast<TInt8>(previousLevel + inc);
        }
    else 
        {
        newGain = static_cast<TInt8>(previousLevel - inc);
        }
    
    return newGain;

    }
    
TBool CProcInFileHandler::WriteDataToInputBufferL(const TDesC8& aData)
    {
    
    if (iInputBuffer == 0)
        {
        iInputBuffer = HBufC8::NewL(aData.Size());
        }
    else if (iInputBuffer->Des().MaxSize() < iInputBuffer->Length() + aData.Length())
        {
        iInputBuffer = iInputBuffer->ReAllocL(iInputBuffer->Length() + aData.Length());
        }
     
    iInputBuffer->Des().Append(aData);
    
    return ETrue;
    
    }
