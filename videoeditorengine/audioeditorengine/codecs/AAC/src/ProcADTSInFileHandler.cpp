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



#include <e32base.h>
#include <f32file.h>
#include "AudCommon.h"
#include "ProcConstants.h"
#include "ProcADTSInFileHandler.h"
//#include "mp4aud.h"
#include "mp4config.h"
#include "audconstants.h"

#include "ProcTools.h"
#include "ProcAACFrameHandler.h"


 
CProcADTSInFileHandler* CProcADTSInFileHandler::NewL(const TDesC& aFileName, RFile* aFileHandle,
                                                     CAudClip* aClip, TInt aReadBufferSize, 
                                                     TInt aTargetSampleRate, TChannelMode aChannelMode)
    {
  
  CProcADTSInFileHandler* self = NewLC(aFileName, aFileHandle, aClip, aReadBufferSize, aTargetSampleRate, aChannelMode);
  CleanupStack::Pop(self);
  return self;

    }
    
CProcADTSInFileHandler* CProcADTSInFileHandler::NewLC(const TDesC& aFileName, RFile* aFileHandle,
                                                      CAudClip* aClip, TInt aReadBufferSize, TInt aTargetSampleRate,
                                                      TChannelMode aChannelMode)

    {

  CProcADTSInFileHandler* self = new (ELeave) CProcADTSInFileHandler();
  CleanupStack::PushL(self);
  self->ConstructL(aFileName, aFileHandle, aClip, aReadBufferSize, aTargetSampleRate, aChannelMode);
  return self;

    }

void CProcADTSInFileHandler::GetPropertiesL(TAudFileProperties* aProperties)
    {

    if (iProperties != 0)
        {
        *aProperties = *iProperties;
        return;
        }

    aProperties->iDuration = 0;
    aProperties->iSamplingRate = 0;
    aProperties->iBitrate = 0;
    aProperties->iChannelMode = EAudChannelModeNotRecognized;
    aProperties->iFrameLen = 0;
    aProperties->iFrameCount = 0;

    aProperties->iAudioType = EAudTypeUnrecognized;
    aProperties->iAudioTypeExtension = EAudExtensionTypeNoExtension;
    aProperties->iFileFormat = EAudFormatUnrecognized;
    aProperties->iBitrateMode = EAudBitrateModeNotRecognized;
    aProperties->iFrameDuration = 0;
    aProperties->iNumFramesPerSample = 1;
    aProperties->iChannelModeExtension = EAudChannelModeNotRecognized;
    aProperties->iAACObjectType = EAudAACObjectTypeNone;
    
    if (!iSupportedFile)
        {
        User::Leave(KErrNotSupported);
        return;
        }

    ReadHeaderInformation(aProperties);
    ReadOtherInformationL(aProperties);

    aProperties->iBitrateMode = EAudConstant;

    if (iProperties == 0)
        {
        iProperties = new (ELeave) TAudFileProperties;
        *iProperties = *aProperties;
        }

    }


TBool CProcADTSInFileHandler::SeekAudioFrame(TInt32 aTime)
    {
    
    if (!iSupportedFile)
        {
        return EFalse;
        }

    if (aTime == 0) 
        {
        FindFirstAACFrame();
        iCurrentTimeMilliseconds = aTime;
        return ETrue;
        }
    
    else FindFirstAACFrame();

    TBuf8<6> header;

    if(BufferedFileRead(header, 6) != 6)
        {
        return EFalse;
        }

    if (header.Length() != 6)
        {
        return EFalse;
        }

    TUint8 byte2 = header[1];
    TBuf8<8> byte2b;
    ProcTools::Dec2Bin(byte2, byte2b);
    // lets confirm that we have found a legal AAC header
    if (header[0] == 0xFF &&
        byte2b[0] == '1' &&
        byte2b[1] == '1' &&
        byte2b[2] == '1' &&
        byte2b[3] == '1' &&
        //byte2b[4] == '1' &&
        byte2b[5] == '0' &&
        byte2b[6] == '0')
        {
        }    
    else
        {
        return EFalse;
        }

    TUint8 byte4 = header[3];
    TUint8 byte5 = header[4];
    TUint8 byte6 = header[5];

    TUint frameLen = CalculateAACFrameLength(byte4, byte5, byte6);    
    //aFrame = HBufC8::NewL(frameLen);

    TInt dur = 0;
    
    while (BufferedFileSetFilePos(iFilePos-6+frameLen))
        {

        dur = dur + ProcTools::MilliSeconds(iProperties->iFrameDuration);
        if (dur > aTime)
            {
            break;
            }

        if(BufferedFileRead(header, 6) != 6)
            {
            break;
            }

        if (header.Length() != 6)
            {
            break;
            }

        if (header[0] != 0xFF) break;
        byte4 = header[3];
        byte5 = header[4];
        byte6 = header[5];

        frameLen = CalculateAACFrameLength(byte4, byte5, byte6);    
    
        }


    aTime = dur;
    iCurrentTimeMilliseconds = aTime;
    return ETrue;
    }

TBool CProcADTSInFileHandler::SeekCutInFrame()
    {
    iCurrentTimeMilliseconds = iCutInTime;
    return SeekAudioFrame(iCutInTime);
    }




TBool CProcADTSInFileHandler::SetNormalizingGainL(const CProcFrameHandler* aFrameHandler)
    {
    
    
    HBufC8* point = 0;
    TInt siz;
    TInt32 tim = 0;
    TInt maxGain = 0;
    RArray<TInt> gains;
    TInt maxAverage = 0;
    TInt tmpGain = 0;
    TInt gainCounter = 0;
    TInt timeNow = 0;
    while(GetEncAudioFrameL(point, siz, tim)) 
    {
        CleanupStack::PushL(point);
        
        aFrameHandler->GetGainL(point, gains, maxGain);
        timeNow += tim;
        
        for (TInt a = 0 ; a < gains.Count() ; a++)
        {
            tmpGain += gains[a];
            gainCounter++;
        }
        gains.Reset();
        
        if (timeNow > 1000)
        {
            if (tmpGain/gainCounter > maxAverage)
            {
                maxAverage = tmpGain/gainCounter;
            }
            
            timeNow = 0;
            tmpGain = 0;
            gainCounter = 0;
        }
        
        CleanupStack::PopAndDestroy(point);
        
    }
    
    // bigger value makes normalizing more efficient, but makes
    // dynamic compression more likely to occur
    TInt NormalizingFactor = 175;
    if (iProperties->iBitrate > 20000 && iProperties->iBitrate < 40000)
    {
        
        // 32 kBit/s
        NormalizingFactor = 170;
        
    }
    else if (iProperties->iBitrate > 80000 && iProperties->iBitrate < 110000)
    {
        // 96 kBit/s
        NormalizingFactor = 170;
        
    }
    
    
    else if (iProperties->iBitrate > 110000 && iProperties->iBitrate < 140000)
    {
        // 128 kBit/s
        if (iProperties->iChannelMode == EAudSingleChannel)
            NormalizingFactor = 160;
        else
            NormalizingFactor = 170;
        
    }
    else if (iProperties->iBitrate > 150000)
    {
        // 256 kBit/s
        if (iProperties->iChannelMode == EAudSingleChannel)
            NormalizingFactor = 150;
        else
            NormalizingFactor = 165;
        
    }
    else
    {
        
        if (iProperties->iChannelMode == EAudSingleChannel)
            NormalizingFactor = 170;
        
    }
    
    
    TInt gainBoost = (NormalizingFactor-maxAverage)*3;
    
    if (gainBoost < 0) gainBoost = 0;
    
    iNormalizingMargin = static_cast<TInt8>(gainBoost);
    
    return ETrue;

    }


TBool CProcADTSInFileHandler::ReadAudioDecoderSpecificInfoL(HBufC8*& aBytes, TInt aBufferSize)
    {

    if (aBufferSize < iMP4DecSpecInfo->Size()) return EFalse;

    if (iMP4DecSpecInfo != 0)
        {    
        aBytes = HBufC8::NewL(iMP4DecSpecInfo->Size());    
        aBytes->Des().Append(iMP4DecSpecInfo->Des());
        return ETrue;
        }
    else 
        {
        aBytes = 0;
        return EFalse;
        }    

    }


CProcADTSInFileHandler::~CProcADTSInFileHandler()
    {
    ResetAndCloseFile();

    if (iSilentFrame != 0)
        {
        delete iSilentFrame;
        }

    if (iMP4DecSpecInfo != 0)
        {
        delete iMP4DecSpecInfo;

        }


    if (iFrameInfo != 0)
        {
        delete iFrameInfo;

        }
    
    delete iFrameHandler;
        
    delete iDecoder;
    
    }


void CProcADTSInFileHandler::ConstructL(const TDesC& aFileName, 
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
    
    TBool frameFound = FindFirstAACFrame();
    
    if (!frameFound)
        {
        iSupportedFile = EFalse;
        return;
        }
    
    iClip = aClip;

    TAudFileProperties prop;
    iFrameInfo = new (ELeave) TAACFrameHandlerInfo;
    GetPropertiesL(&prop);

    if (prop.iAudioTypeExtension != EAudExtensionTypeNoExtension)
        {
        // AACPlus frame duration is calculated from sampling rate
        // the actual sampling rate of AAC plus is twice as high as
        // the AAC part
        iProperties->iFrameDuration = iProperties->iFrameDuration.Int64()/2;
        
        }

    // fill in aacInfo for retrieving decoder specific information
    //AACStreamInfo aacInfo;
    int16 frameLen = 0;
    int32 sampleRate = 0;
    uint8 profile = 0;
    uint8 nChannels = 0;

    // generate a silent frame ------------------>

    if (iProperties->iChannelMode == EAudSingleChannel)
        {

        iSilentFrame = HBufC8::NewL(KSilentMonoAACFrameLenght);    
        
        iSilentFrame->Des().Append(KSilentMonoAACFrame, KSilentMonoAACFrameLenght);

        nChannels = 1;
        }
    else if (iProperties->iChannelMode == EAudStereo)
        {

        iSilentFrame = HBufC8::NewL(KSilentStereoAACFrameLenght);    
        iSilentFrame->Des().Append(KSilentStereoAACFrame, KSilentStereoAACFrameLenght);
        
        nChannels = 2;
        }
    else
        {
        User::Leave(KErrNotSupported);
        }
    
    // Check that the sample rate is supported    
    if( (iProperties->iSamplingRate != KAedSampleRate8kHz) &&
        (iProperties->iSamplingRate != KAedSampleRate11kHz) &&
        (iProperties->iSamplingRate != KAedSampleRate16kHz) &&
        (iProperties->iSamplingRate != KAedSampleRate22kHz) &&
        (iProperties->iSamplingRate != KAedSampleRate24kHz) &&
        (iProperties->iSamplingRate != KAedSampleRate32kHz) &&
        (iProperties->iSamplingRate != KAedSampleRate44kHz) &&
        (iProperties->iSamplingRate != KAedSampleRate48kHz) )
        {
        User::Leave(KErrNotSupported);
        }
         


    uint8 decSpecInfo[16];
    int16 nConfigBytes;
  
    
    frameLen = 1024;
    sampleRate = iProperties->iSamplingRate;
    if (iFrameInfo->iProfileID == 3)
        {
        profile = LTP_OBJECT;
        }
    else if (iFrameInfo->iProfileID == 1)
        {
        profile = LC_OBJECT;
        }
    else
        {
        User::Leave(KErrNotSupported);
        }

    //nConfigBytes = AACGetMP4ConfigInfo(&aacInfo, decSpecInfo, 16);
    nConfigBytes = AACGetMP4ConfigInfo(sampleRate, profile, 
        nChannels, frameLen, decSpecInfo, 16);


    if (nConfigBytes > 0)
        {

        iMP4DecSpecInfo = HBufC8::NewL(nConfigBytes);
        iMP4DecSpecInfo->Des().Append(decSpecInfo, nConfigBytes);

        }


    mp4_u32 frameDurationMilli = ProcTools::MilliSeconds(iProperties->iFrameDuration);
    
    iSilentFrameDuration = frameDurationMilli;
    

    // <------------------generate a silent frame

    // Generate a decoder ----------------------->

    iDecoder = CProcDecoder::NewL();
    
    iDecodingPossible = iDecoder->InitL(*iProperties, aTargetSampleRate, aChannelMode);
    
    // <----------------------- Generate a decoder 
    
    // Create a frame handler --------------------->
    
    iFrameHandler = CProcAACFrameHandler::NewL(*iFrameInfo);
    
    if (iClip != 0 && iClip->Normalizing())
        {
        SetNormalizingGainL(iFrameHandler);    
        }
    

    }




TBool CProcADTSInFileHandler::GetEncAudioFrameL(HBufC8*& aFrame, TInt& aSize, TInt32& aTime)
    {

    if (!iSupportedFile)
        {
        return EFalse;
        }
        
        
    TBuf8<7> header;

    if(BufferedFileRead(header, 7) != 7)
        {
        return EFalse;
        }

    if (header.Length() != 7)
        {
        return EFalse;
        }

    if (header[0] != 0xFF) return EFalse;
    TUint8 byte4 = header[3];
    TUint8 byte5 = header[4];
    TUint8 byte6 = header[5];
//    TUint8 byte7 = header[6];

    
    TInt frameLen = CalculateAACFrameLength(byte4, byte5, byte6);    
    aFrame = HBufC8::NewL(frameLen);

    BufferedFileSetFilePos(iFilePos-7);
    TPtr8 tmpDes((TPtr8)aFrame->Des());

    if (BufferedFileRead((TPtr8&)tmpDes, frameLen) != frameLen)
        {
        delete aFrame;
        aFrame = 0;
        return EFalse;
        }
        
    aSize = frameLen;
    aTime = ProcTools::MilliSeconds(iProperties->iFrameDuration);
    
    // remove ADTS header -------------------------------------------->
    
    TInt headerLength = 0;  //  because we need to remove header in case of AAC frame from .aac files
    //check Header length to be removed from the frame before writting to 3gp file
    TUint8* dtPtr = (TUint8*)(aFrame->Ptr());
    if((dtPtr[1]&& 0x01) == 0x00)    //based on bytes decide header length to remove
        {
            headerLength =9;
        }
    else if((dtPtr[1]&& 0x01)== 0x01)
        {
            headerLength =7;
        }
        else
        {
            headerLength =0;
        }
    
    aFrame->Des().Delete(0,headerLength);
    aSize -= headerLength;
 
    // <--------------------------------------------- remove ADTS header
    
    iCurrentTimeMilliseconds += ProcTools::MilliSeconds(iProperties->iFrameDuration);
    
    
    // Fix for synchronizing problem ---------------------------------->
    // If the accurate frame length cannot be represented in milliseconds
    // store the remainder and increase the output frame lenght by one ms 
    // when needed. Accuracy depends on sampling rate
    
    TReal accurateFrameLen = TReal(1024000)/iProperties->iSamplingRate;
    
    iAACFrameLenRemainderMilli += accurateFrameLen - aTime;
        
    if (iAACFrameLenRemainderMilli > 1)
        {
        aTime += 1;
        iCurrentTimeMilliseconds += 1;
        iAACFrameLenRemainderMilli -= 1;
        }

    
    // <---------------------------------- Fix for synchronizing problem
    
    
    if (iProperties->iAudioTypeExtension == EAudExtensionTypeNoExtension)
        {
         
          
        // AACPlus is handled after decoding
                
        TRAPD(err, ManipulateGainL(aFrame));
        
        if (err != KErrNone)
            {
            // something went wrong with the gain manipulation
            // continue by returning the original frame
            }    
        }
    
        
    aSize = aFrame->Size();
    
    return ETrue;

    
    }

CProcADTSInFileHandler::CProcADTSInFileHandler()  : CProcInFileHandler(), iFrameInfo(0),
                                                    iSupportedFile(ETrue)
    {
    

    }


TBool CProcADTSInFileHandler::FindFirstAACFrame()
    {    

    TBuf8<8> byte2b;
    TUint8 byte2;
    TUint8 byte1;

    TBool fileEnded = EFalse;
    TUint ind = 0;
    TBool frameFound = EFalse;

  BufferedFileSetFilePos(0);

    while (!fileEnded) 
        {
        if (BufferedFileReadChar(ind, byte1) == 0) 
            {
            fileEnded = ETrue;
            }
        ind++;
        
        if (byte1 != 0xFF) continue;


        if (BufferedFileReadChar(ind, byte2) == 0) 
            {
            fileEnded = ETrue;
            }
        ind++;
        ProcTools::Dec2Bin(byte2, byte2b);
        
    
        if (byte1 == 0xFF &&
            byte2b[0] == '1' &&
            byte2b[1] == '1' &&
            byte2b[2] == '1' &&
            byte2b[3] == '1' &&
            //byte2b[4] == '1' &&
            byte2b[5] == '0' &&
            byte2b[6] == '0') 
            {
                
            // this looks like AAC frame header, let's confirm...    
            TUint8 byte3, byte4, byte5, byte6;
            BufferedFileReadChar(ind, byte3);    
            BufferedFileReadChar(ind+1, byte4);
            BufferedFileReadChar(ind+2, byte5);
            BufferedFileReadChar(ind+3, byte6);

            ind++;

            TUint frameLength = CalculateAACFrameLength(byte4, byte5, byte6);
            
            TUint8 testByte1, testByte2, testByte3;
            BufferedFileReadChar(ind+frameLength-3, testByte1);
            BufferedFileReadChar(ind+frameLength-2, testByte2);
            BufferedFileReadChar(ind+frameLength-1, testByte3);
            
            if (byte1 == testByte1 && byte2 == testByte2
                && byte3 == testByte3) 
                {
                // this must be the header...
                iFirstFrame = ind-3;
                //FillInHeaderValues(byte2, byte3, byte4);
                frameFound = ETrue;
                break;
                }
    
            }

        }
    if (frameFound)
        {
        BufferedFileSetFilePos(iFirstFrame);
        }

    return frameFound;
    }


TUint CProcADTSInFileHandler::CalculateAACFrameLength(TUint8 byte4, TUint8 byte5, TUint8 byte6) 
    {

    TBuf8<13> headerLengthB;
    TBuf8<8> byte4b, byte5b, byte6b;
    ProcTools::Dec2Bin(byte4, byte4b);
    ProcTools::Dec2Bin(byte5, byte5b);
    ProcTools::Dec2Bin(byte6, byte6b);
    
    headerLengthB.Append(byte4b[6]);
    headerLengthB.Append(byte4b[7]);
    headerLengthB.Append(byte5b);
    headerLengthB.Append(byte6b[0]);
    headerLengthB.Append(byte6b[1]);
    headerLengthB.Append(byte6b[2]);


    TUint headerLength = 0;
    TLex8 leks(headerLengthB);

    if (leks.Val(headerLength, EBinary) != KErrNone) 
        {
        return 0;
        }

    return headerLength;

}


TBool CProcADTSInFileHandler::ReadHeaderInformation(TAudFileProperties* aProperties)
    {

    TInt origFilePos = iFilePos;
    
    if (aProperties == 0)
        return EFalse;

    TBuf8<4> header;

    if(BufferedFileRead(header, 4) != 4)
        {
        BufferedFileSetFilePos(origFilePos);
        return EFalse;
        }

    if (header.Length() != 4)
        {
        BufferedFileSetFilePos(origFilePos);
        return EFalse;
        }

    TUint8 byte2 = header[1];
    TBuf8<8> byte2b;
    ProcTools::Dec2Bin(byte2, byte2b);
    // lets confirm that we have found a legal AAC header
    if (header[0] == 0xFF &&
        byte2b[0] == '1' &&
        byte2b[1] == '1' &&
        byte2b[2] == '1' &&
        byte2b[3] == '1' &&
        //byte2b[4] == '1' &&
        byte2b[5] == '0' &&
        byte2b[6] == '0')
        {
        // OK
        aProperties->iFileFormat = EAudFormatAAC_ADTS;
    //    aProperties->iFrameDuration = 23000;
        }    
    else
        {
        return EFalse;
        }
    /*
    if (byte2b[4] == '1')
        {
        aProperties->iAudioType = EAudAAC_MPEG2;
        }
    else
        {
        aProperties->iAudioType = EAudAAC_MPEG4;
        }
    */
    
    // NOTE: call all MPeg audio EAudAAC_MPEG4
    aProperties->iAudioType = EAudAAC_MPEG4;
    
    
    TUint8 byte3 = header[2];
    TUint8 byte4 = header[3];

    TBuf8<8> byte3b;
    TBuf8<8> byte4b;

    ProcTools::Dec2Bin(byte2, byte2b);
    ProcTools::Dec2Bin(byte3, byte3b);
    ProcTools::Dec2Bin(byte4, byte4b);

    TBuf8<2> profileId(byte3b.Left(2));
    TUint proID = 0;
    ProcTools::Bin2Dec(profileId, proID);
    
    iFrameInfo->iProfileID = static_cast<TUint8>(proID);
    aProperties->iAACObjectType = TAudAACObjectType(iFrameInfo->iProfileID);


    TBuf8<4> samplingRateIndexB;
    samplingRateIndexB.Append(byte3b[2]);
    samplingRateIndexB.Append(byte3b[3]);
    samplingRateIndexB.Append(byte3b[4]);
    samplingRateIndexB.Append(byte3b[5]);

    TUint srIndex = 0;
    TLex8 lek(samplingRateIndexB);

    if (lek.Val(srIndex, EBinary) != KErrNone) 
        {
        BufferedFileSetFilePos(origFilePos);
        return EFalse;
        }
    // aac sampling rates
        
    const TInt KAAC_SAMPLING_RATES[16] = {96000,88200,64000,48000,44100,32000,24000
            ,22050,16000,12000,11025,8000,0,0,0,0};

    iFrameInfo->iSampleRateID = static_cast<TUint8>(srIndex);
    aProperties->iSamplingRate = KAAC_SAMPLING_RATES[srIndex];
    samplingRateIndexB.Delete(0, samplingRateIndexB.Length());
        
    // channel configuration


    TUint aacChannels = 0;
    TBuf8<3> channelB;
    channelB.Append(byte3b[7]);
    channelB.Append(byte4b[0]);
    channelB.Append(byte4b[1]);

    TLex8 leks2(channelB);
    if (leks2.Val(aacChannels, EBinary) != KErrNone) 
        {
        BufferedFileSetFilePos(origFilePos);
        return EFalse;
        }
        
    if (aacChannels == 1)
        {
        aProperties->iChannelMode = EAudSingleChannel;
        }
    else if (aacChannels == 2)
        {
        aProperties->iChannelMode = EAudStereo;
        }
    
    iFrameInfo->iNumChannels = static_cast<TUint8>(aacChannels);
    iFrameInfo->iNumCouplingChannels = 0;
    iFrameInfo->iIs960 = 0;
    

    BufferedFileSetFilePos(origFilePos);
    return ETrue;

    }


TBool CProcADTSInFileHandler::ReadOtherInformationL(TAudFileProperties* aProperties)
    {

    TInt origFilePos = iFilePos;

    FindFirstAACFrame();

    TBuf8<7> header;

    if(BufferedFileRead(header, 7) != 7)
        {
        return EFalse;
        }

    if (header.Length() != 7)
        {
        return EFalse;
        }

    TUint8 byte2 = header[1];
    TBuf8<8> byte2b;
    ProcTools::Dec2Bin(byte2, byte2b);
    // lets confirm that we have found a legal AAC header
    if (header[0] == 0xFF &&
        byte2b[0] == '1' &&
        byte2b[1] == '1' &&
        byte2b[2] == '1' &&
        byte2b[3] == '1' &&
    //    byte2b[4] == '1' &&
        byte2b[5] == '0' &&
        byte2b[6] == '0')
        {
        // OK
        //aProperties->iAudioType = EAudAAC;
        //aProperties->iFileFormat = EAudFormatAAC_ADTS;
        }    
    else
        {
        return EFalse;
        }

    TUint8 byte4 = header[3];
    TUint8 byte5 = header[4];
    TUint8 byte6 = header[5];

    aProperties->iFrameDuration = ((1024*1000)/(aProperties->iSamplingRate))*1000;

    TInt frameLen = CalculateAACFrameLength(byte4, byte5, byte6);
    
    
    

     /*-- Allocate resources and read 1st frame. --*/
    HBufC8 *aFrame = HBufC8::NewL(frameLen);
    CleanupStack::PushL(aFrame);

    TPtr8 tmpDes((TPtr8)aFrame->Des());

    BufferedFileRead((TPtr8&)tmpDes, frameLen - 7);

    
    const TInt KShortestAACFrame = 20;

    if (frameLen > KShortestAACFrame) // a silent frame?
        {
        
        

        /*
         * Get frame parameters for eAAC+ codec. It is possible that the bitstream
        * is plain AAC but we don't know it before the 1st frame is parsed!
        */

        TUint8* buf = const_cast<TUint8*>(aFrame->Right(aFrame->Size()).Ptr());
        CProcAACFrameHandler::GetEnhancedAACPlusParametersL(buf, frameLen - 7, aProperties, iFrameInfo);

        }
    else
        {
        iFrameInfo->iIsParametricStereo = EFalse;
        iFrameInfo->isSBR = EFalse;
            
        }

    CleanupStack::Pop(aFrame);
    delete aFrame;
    aFrame = 0;


        


    if(BufferedFileRead(header, 7) != 7)
        {
        return EFalse;
        }

    if (header.Length() != 7)
        {
        return EFalse;
        }

    byte4 = header[3];
    byte5 = header[4];
    byte6 = header[5];

    TInt dur = ProcTools::MilliSeconds(aProperties->iFrameDuration);
    TInt bytes = frameLen;
    TInt frames = 1;

    frameLen = CalculateAACFrameLength(byte4, byte5, byte6);

    while (BufferedFileSetFilePos(iFilePos-7+frameLen))
        {

        dur += ProcTools::MilliSeconds(aProperties->iFrameDuration);
        bytes += frameLen;
        frames++;

        if(BufferedFileRead(header, 7) != 7)
            {
            break;
            }

        if (header.Length() != 7)
            {
            break;
            }

        if (header[0] != 0xFF) break;
        byte4 = header[3];
        byte5 = header[4];
        byte6 = header[5];

        frameLen = CalculateAACFrameLength(byte4, byte5, byte6);    
    
        }
    
    TTimeIntervalMicroSeconds tmp((TInt64)(TInt)dur*1000);
    aProperties->iDuration = tmp;

    aProperties->iFrameCount = frames;
    aProperties->iFrameLen = bytes/frames;
    aProperties->iBitrate = (iFilePos*1000/(dur))*8;


    BufferedFileSetFilePos(origFilePos);

    return ETrue;
    }

TBool CProcADTSInFileHandler::GetInfoForFrameHandler(TAACFrameHandlerInfo& aAACInfo)
    {
    
    aAACInfo = *iFrameInfo;
    return ETrue;
    }

