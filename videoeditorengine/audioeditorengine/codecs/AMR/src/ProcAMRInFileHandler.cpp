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



#include "ProcAMRInFileHandler.h"
#include "ProcAMRFrameHandler.h"
#include "ProcTools.h"
#include "AudPanic.h"


CProcAMRInFileHandler* CProcAMRInFileHandler::NewL(const TDesC& aFileName, 
                                                   RFile* aFileHandle,
                                                   CAudClip* aClip, 
                                                   TInt aReadBufferSize,
                                                   TInt aTargetSampleRate, 
                                                   TChannelMode aChannelMode) 
    {

    CProcAMRInFileHandler* self = NewLC(aFileName, aFileHandle, aClip, aReadBufferSize, 
                                        aTargetSampleRate, aChannelMode);
    CleanupStack::Pop(self);
    return self;
    }

CProcAMRInFileHandler* CProcAMRInFileHandler::NewLC(const TDesC& aFileName, 
                                                    RFile* aFileHandle,
                                                    CAudClip* aClip, 
                                                    TInt aReadBufferSize,
                                                    TInt aTargetSampleRate, 
                                                    TChannelMode aChannelMode) 
    {
    
    CProcAMRInFileHandler* self = new (ELeave) CProcAMRInFileHandler();
    CleanupStack::PushL(self);
    self->ConstructL(aFileName, aFileHandle, aClip, aReadBufferSize,
                    aTargetSampleRate, aChannelMode);
    return self;
    }

CProcAMRInFileHandler::CProcAMRInFileHandler() : CProcInFileHandler() 
    {
    
    
    }

void CProcAMRInFileHandler::GetPropertiesL(TAudFileProperties* aProperties) 
    {

    if (iProperties != 0)
        {
        *aProperties = *iProperties;
        return;
        }
    aProperties->iAudioTypeExtension = EAudExtensionTypeNoExtension;
    if (iFileOpen) 
        {
        aProperties->iFileFormat = EAudFormatAMR;
        
        TInt oldPos = iFilePos;

        TInt fileSize = 0;
        iFile.Size(fileSize);
        
        TBuf8<6> header;
        //iFile.Read(0, header);
        BufferedFileRead(0, header);
        if (header.Compare(_L8("#!AMR\n")) == 0) 
            {
            aProperties->iAudioType = EAudAMR;
            aProperties->iSamplingRate = 8000;
            aProperties->iChannelMode = EAudSingleChannel;
            aProperties->iBitrateMode = EAudVariable;
            
            TBuf8<1> buf;
            //    iFile.Read(6, buf);
            BufferedFileRead(6, buf);    
            TUint dec_mode = (enum Mode)((buf[0] & 0x0078) >> 3);
            
            aProperties->iBitrate = KAmrBitRates[dec_mode];
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

TBool CProcAMRInFileHandler::SeekAudioFrame(TInt32 aTime) 
    {
    

    if (!iFileOpen) 
        {
        TAudPanic::Panic(TAudPanic::EInternal);
        }

    TInt fileSize = 0;
    iFile.Size(fileSize);


    TInt firstModeByte = 6;
    TInt readSize = 0;
    TBuf8<1> modeByte;
    iCurrentTimeMilliseconds = 0;

    TUint dec_mode; 
    //iFile.Seek(ESeekStart, firstModeByte);
    BufferedFileSetFilePos(firstModeByte);
    
    while(iCurrentTimeMilliseconds < aTime) 
        {
        
        //iFile.Read(modeByte, 1);
        BufferedFileRead(modeByte);
        if (modeByte.Size() == 0) 
            {
            return EFalse;
            }

        dec_mode = (enum Mode)((modeByte[0] & 0x0078) >> 3);
        
        switch (dec_mode)
        {
        case 0:
            readSize = 12;
            break;
        case 1:
            readSize = 13;
            break;
        case 2:
            readSize = 15;
            break;
        case 3:
            readSize = 17;
            break;
        case 4:
            readSize = 19;
            break;
        case 5:
            readSize = 20;
            break;
        case 6:
            readSize = 26;
            break;
        case 7:
            readSize = 31;
            break;
        case 8:
            readSize = 5;
            break;
        case 15:
            readSize = 0;
            break;
        default:
            readSize = 0;
            break;
        };
        // read the next audio frame
        //iFile.Read(audioFrame, read_size);
        //if (iFile.Seek(ESeekCurrent, readSize) != KErrNone) {
        //    return EFalse;
        //}
        if (!(BufferedFileSetFilePos(BufferedFileGetFilePos()+readSize))) 
            {
            
            // EOF
            return EFalse;
            }
        iCurrentTimeMilliseconds += 20;
        }
    return ETrue;
    }    

TBool CProcAMRInFileHandler::SeekCutInFrame() 
    {

    iCurrentTimeMilliseconds = iCutInTime;
    return SeekAudioFrame(iCutInTime);
    }
    



void CProcAMRInFileHandler::ConstructL(const TDesC& aFileName, 
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

    const TInt KSilentFrameSize = 13;
    
    TUint8 silentFrame[KSilentFrameSize]=
        {
        0x04,0x63,0x3C,0xC7,0xF0,0x03,0x04,0x39,0xFF,0xE0,
        0x00,0x00,0x00
        
        };
    

    iSilentFrame = HBufC8::NewL(KSilentFrameSize);
    iSilentFrame->Des().Append(silentFrame, KSilentFrameSize); 
    iSilentFrameDuration = 20;

    
    iClip = aClip;
    
    TAudFileProperties prop;
    GetPropertiesL(&prop);
    
    iDecoder = CProcDecoder::NewL();
    
    iDecodingPossible = iDecoder->InitL(prop, aTargetSampleRate, aChannelMode);
    
    
    // Generate a frame handler --------------------->
    
    iFrameHandler = CProcAMRFrameHandler::NewL();
    
    if (iClip != 0 && iClip->Normalizing())
        {
        SetNormalizingGainL(iFrameHandler);    
        }
    
    
    }
    
TBool CProcAMRInFileHandler::GetEncAudioFrameL(HBufC8*& aFrame, TInt& aSize, TInt32& aTime)
    {
    if (!iFileOpen) 
        {
        TAudPanic::Panic(TAudPanic::EInternal);
        }

    aFrame = NULL;
    TInt readSize = 0;
    TBuf8<1> modeByte;
//    TBuf8<32> audioFrame;
    TUint dec_mode; 


    //iFile.Read(modeByte);
    BufferedFileRead(modeByte);
    if (modeByte.Size() == 0) 
        {
        return EFalse;
        }

    dec_mode = (enum Mode)((modeByte[0] & 0x0078) >> 3);
        
        switch (dec_mode)
        {
        case 0:
            readSize = 12;
            break;
        case 1:
            readSize = 13;
            break;
        case 2:
            readSize = 15;
            break;
        case 3:
            readSize = 17;
            break;
        case 4:
            readSize = 19;
            break;
        case 5:
            readSize = 20;
            break;
        case 6:
            readSize = 26;
            break;
        case 7:
            readSize = 31;
            break;
        case 8:
            readSize = 5;
            break;
        case 15:
            readSize = 0;
            break;
        default:
            readSize = 0;
            break;
        };

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



CProcAMRInFileHandler::~CProcAMRInFileHandler() 
    {

    if (iSilentFrame != 0) delete iSilentFrame;

    ResetAndCloseFile();
    
    delete iDecoder;

    delete iFrameHandler;

    }


TBool CProcAMRInFileHandler::GetFrameInfo(TInt& aSongDuration,
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


    TInt firstModeByte = 6;
    TInt frameIndex = 1;
    TInt readSize = 0;
    TBuf8<1> modeByte;

    TInt32 frameSizeSum = 0;
    
    TInt filePos = iFilePos;

    TUint dec_mode; 

    BufferedFileSetFilePos(firstModeByte);
    
    while(frameIndex < 180000) // 1 hour 
        {

        BufferedFileRead(modeByte);
        if (modeByte.Size() == 0) 
            {
            break;
            }

        dec_mode = (enum Mode)((modeByte[0] & 0x0078) >> 3);
        
        switch (dec_mode)
        {
        case 0:
            readSize = 12;
            break;
        case 1:
            readSize = 13;
            break;
        case 2:
            readSize = 15;
            break;
        case 3:
            readSize = 17;
            break;
        case 4:
            readSize = 19;
            break;
        case 5:
            readSize = 20;
            break;
        case 6:
            readSize = 26;
            break;
        case 7:
            readSize = 31;
            break;
        case 8:
            readSize = 5;
            break;
        case 15:
            readSize = 0;
            break;
        default:
            readSize = 0;
            break;
        };
        if (!(BufferedFileSetFilePos(BufferedFileGetFilePos()+readSize))) 
            {
            
            break; // EOF
            }
        frameSizeSum += readSize;
        aSongDuration += 20;
        frameIndex++;
        }

    aAverageFrameSize = frameSizeSum/frameIndex;
    aFrameAmount = frameIndex;
    aAverageFrameDuration = aSongDuration/aFrameAmount;
    BufferedFileSetFilePos(filePos);
    return ETrue;


    }


TBool CProcAMRInFileHandler::SetNormalizingGainL(const CProcFrameHandler* aFrameHandler)
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

