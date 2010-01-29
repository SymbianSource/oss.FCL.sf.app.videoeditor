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



#include "ProcAWBInFileHandler.h"
#include "ProcAWBFrameHandler.h"

#include "AudPanic.h"
#include "AWBConstants.h"
#include "ProcTools.h"


CProcAWBInFileHandler* CProcAWBInFileHandler::NewL(const TDesC& aFileName,  
                                                   RFile* aFileHandle,
                                                   CAudClip* aClip, 
                                                   TInt aReadBufferSize,
                                                   TInt aTargetSampleRate, 
                                                   TChannelMode aChannelMode) 
    {

    CProcAWBInFileHandler* self = NewLC(aFileName, aFileHandle, aClip, 
                    aReadBufferSize, aTargetSampleRate, aChannelMode);
    CleanupStack::Pop(self);
    return self;
    }

CProcAWBInFileHandler* CProcAWBInFileHandler::NewLC(const TDesC& aFileName, 
                                                    RFile* aFileHandle,
                                                    CAudClip* aClip, 
                                                    TInt aReadBufferSize,
                                                    TInt aTargetSampleRate, 
                                                    TChannelMode aChannelMode) 
    {
    
    CProcAWBInFileHandler* self = new (ELeave) CProcAWBInFileHandler();
    CleanupStack::PushL(self);
    self->ConstructL(aFileName, aFileHandle, aClip, aReadBufferSize, 
                    aTargetSampleRate, aChannelMode);
    return self;
    }

CProcAWBInFileHandler::CProcAWBInFileHandler() : CProcInFileHandler() 
    {
    
    
    }

void CProcAWBInFileHandler::GetPropertiesL(TAudFileProperties* aProperties) 
    {

    if (iProperties != 0)
        {
        *aProperties = *iProperties;
        return;
        }
    
    aProperties->iAudioTypeExtension = EAudExtensionTypeNoExtension;
    
    if (iFileOpen) 
        {
        aProperties->iFileFormat = EAudFormatAMRWB;
        
        TInt oldPos = iFilePos;

        TInt fileSize = 0;
        iFile.Size(fileSize);
        
        TBuf8<9> header;

        BufferedFileRead(0, header);
        if (header.Compare(_L8("#!AMR-WB\n")) == 0) 
            {
            aProperties->iAudioType = EAudAMRWB;
            aProperties->iSamplingRate = 16000;
            aProperties->iChannelMode = EAudSingleChannel;
            aProperties->iBitrateMode = EAudVariable;
            
            TBuf8<1> modeByte;
            BufferedFileRead(9, modeByte);

            TUint8 toc = modeByte[0];
            
            TUint8 mode = static_cast<TUint8>((toc >> 3) & 0x0F);
            
            aProperties->iBitrate = KAWBBitRates[mode];
            TInt durationMilli = 0;
            TInt frameAmount = 0;
            TInt frameDuration = 0;
            TInt frameSize = 0;

            GetFrameInfo(durationMilli, frameAmount, frameDuration, frameSize);

            TInt64 tmp = (TInt64)(TInt)durationMilli*1000;
            // milliseconds->microseconds
            TTimeIntervalMicroSeconds durationMicro(tmp);
            aProperties->iDuration = durationMicro;
            aProperties->iFrameCount = frameAmount;
            
            tmp = (TInt64)(TInt)frameDuration*1000;
            TTimeIntervalMicroSeconds frameDurationMicro(tmp);
            
            aProperties->iFrameDuration = frameDurationMicro;
            aProperties->iFrameLen = frameSize;
            
            
            }
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

TBool CProcAWBInFileHandler::SeekAudioFrame(TInt32 aTime) 
    {
    
    if (!iFileOpen) 
        {
        TAudPanic::Panic(TAudPanic::EInternal);
        }

    TInt fileSize = 0;
    iFile.Size(fileSize);


    TInt firstModeByte = 9;
    TInt readSize = 0;
    TBuf8<1> modeByte;
    iCurrentTimeMilliseconds = 0;

    BufferedFileSetFilePos(firstModeByte);
    
    while(iCurrentTimeMilliseconds < aTime) 
        {
        
        //iFile.Read(modeByte, 1);
        BufferedFileRead(modeByte);
        if (modeByte.Size() == 0) 
            {
            return EFalse;
            }

        TUint8 toc = modeByte[0];
        TUint8 mode = static_cast<TUint8>((toc >> 3) & 0x0F);

        readSize = KAWBPacked_size[mode];

        if (!(BufferedFileSetFilePos(BufferedFileGetFilePos()+readSize))) 
            {
            
            // EOF
            return EFalse;
            }
        iCurrentTimeMilliseconds += 20;

        }
    return ETrue;

    }    

TBool CProcAWBInFileHandler::SeekCutInFrame() 
    {

    iCurrentTimeMilliseconds = iCutInTime;
    return SeekAudioFrame(iCutInTime);
    }
    


void CProcAWBInFileHandler::ConstructL(const TDesC& aFileName, 
                                       RFile* aFileHandle,
                                       CAudClip* aClip, 
                                       TInt aReadBufferSize,
                                       TInt aTargetSampleRate, 
                                       TChannelMode aChannelMode) 
    {
    

    iTargetSampleRate = aTargetSampleRate;
    iChannelMode = aChannelMode;


    InitAndOpenFileL(aFileName, aFileHandle, aReadBufferSize);
    

    iClip = aClip;

    if (aClip != 0)
        {
        iCutInTime = ProcTools::MilliSeconds(aClip->CutInTime());
        
        }


    TUint8 sf[18] = {0x04,0x10,0x20,0x00,0x21,
    0x1C,0x14,0xD0,0x11,0x40,0x4C,0xC1,0xA0,
    0x50,0x00,0x00,0x44,0x30};
    iSilentFrame = HBufC8::NewL(18);
    iSilentFrame->Des().Append(sf,18);
    iSilentFrameDuration = 20;
    
    
    if (iClip != 0 && iClip->Normalizing())
        {
        SetNormalizingGainL(iFrameHandler);    
        }
    
    TAudFileProperties prop;
    GetPropertiesL(&prop);
    
    iDecoder = CProcDecoder::NewL();
    
    iDecodingPossible = iDecoder->InitL(prop, aTargetSampleRate, aChannelMode);
    
    
    iFrameHandler = CProcAWBFrameHandler::NewL();
    
    if (iClip != 0 && iClip->Normalizing())
        {
        SetNormalizingGainL(iFrameHandler);    
        }
    
    }




TBool CProcAWBInFileHandler::GetEncAudioFrameL(HBufC8*& aFrame, TInt& aSize, TInt32& aTime)
    {
    if (!iFileOpen) 
        {
        TAudPanic::Panic(TAudPanic::EInternal);
        }

    aFrame = NULL;
    TInt readSize = 0;
    TBuf8<1> modeByte;

    BufferedFileRead(modeByte);

    if (modeByte.Size() == 0) 
        {
        return EFalse;
        }

    TUint8 toc = modeByte[0];
    TUint8 mode = static_cast<TUint8>((toc >> 3) & 0x0F);

    readSize = KAWBPacked_size[mode];


    
    HBufC8* audioFrame = HBufC8::NewL(readSize+1);

    TPtr8 tmpDes((TPtr8)audioFrame->Des());
    
    BufferedFileRead((TPtr8&) tmpDes, readSize);
    audioFrame->Des().Insert(0, modeByte);

    aFrame = audioFrame;

    TRAPD(err, ManipulateGainL(aFrame));
    
    if (err != KErrNone)
        {
        // something went wrong with the gain manipulation
        // continue by returning the original frame
        }
    

    aSize = aFrame->Size();
    aTime = 20;
    iCurrentTimeMilliseconds += 20;
    

    return ETrue;
    }



CProcAWBInFileHandler::~CProcAWBInFileHandler() 
    {

    if (iSilentFrame != 0) delete iSilentFrame;
    ResetAndCloseFile();


    delete iDecoder;
    delete iFrameHandler;
    
    }


TBool CProcAWBInFileHandler::GetFrameInfo(TInt& aSongDuration,
                                           TInt& aFrameAmount, 
                                           TInt& aAverageFrameDuration, 
                                           TInt& aAverageFrameSize) 
    {


        

    if (!iFileOpen) 
        {
        TAudPanic::Panic(TAudPanic::EInternal);
        }

    TInt fileSize = 0;
    iFile.Size(fileSize);


    TInt firstModeByte = 9;
    TInt frameIndex = 1;
    TInt readSize = 0;
    TBuf8<1> modeByte;

    TInt32 frameSizeSum = 0;
    
    TInt filePos = iFilePos;

    BufferedFileSetFilePos(firstModeByte);
    
    while(frameIndex < 180000) // 1 hour 
        {
        

        BufferedFileRead(modeByte);
        if (modeByte.Size() == 0) 
            {
            break;
            }

        TUint8 toc = modeByte[0];
        TUint8 mode = static_cast<TUint8>((toc >> 3) & 0x0F);

        readSize = KAWBPacked_size[mode];

        frameSizeSum += readSize;
        aSongDuration += 20;

        if (!(BufferedFileSetFilePos(BufferedFileGetFilePos()+readSize))) 
            {
            
            break; // EOF
            }
        
        frameIndex++;
        }

    aAverageFrameSize = frameSizeSum/frameIndex;
    aFrameAmount = frameIndex;
    aAverageFrameDuration = aSongDuration/aFrameAmount;
    BufferedFileSetFilePos(filePos);
    return ETrue;


    }


TBool CProcAWBInFileHandler::SetNormalizingGainL(const CProcFrameHandler* aFrameHandler)
    {

    HBufC8* point = 0;
    TInt siz;
    TInt32 tim = 0;
    TInt8 margin = 0;
    TInt minMargin = KMaxTInt;
    TInt tmpGain = 0;
    TInt frameCounter = 0;
    while(GetEncAudioFrameL(point, siz, tim)) 
                    
        {
        aFrameHandler->GetNormalizingMargin(point, margin);
        tmpGain += margin;
                    
        delete point;
        point = 0;
        frameCounter++;
        if (frameCounter > 1) 
            {
            tmpGain = tmpGain/3;
                        
            if (tmpGain < minMargin) 
                {
                minMargin = tmpGain;
                }
                
            frameCounter = 0;
            tmpGain = 0;
            }
        }

    iNormalizingMargin = static_cast<TInt8>(minMargin);

    return ETrue;

    }
    
    
