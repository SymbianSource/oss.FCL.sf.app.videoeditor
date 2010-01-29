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
#include "AWBConstants.h"
#include "ProcMP4InFileHandler.h"
#include "mp4aud.h"
#include "ProcAACFrameHandler.h"
#include "ProcAMRFrameHandler.h"
#include "ProcAWBFrameHandler.h"
#include "audconstants.h"



#include "ProcTools.h"
//#include "aacenc_interface.h"

// Debug print macro
#if defined _DEBUG 
#include <e32svr.h>
#define PRINT(x) RDebug::Print x;
#else
#define PRINT(x)
#endif

// Max coded AAC frame size (768 bytes per channel)
const TUint KMaxAACFrameSize(1536);


CProcMP4InFileHandler* CProcMP4InFileHandler::NewL(const TDesC& aFileName, 
                                                   RFile* aFileHandle,
                                                   CAudClip* aClip, TInt aReadBufferSize,
                                                   TInt aTargetSampleRate, 
                                                   TChannelMode aChannelMode)
    {

    CProcMP4InFileHandler* self = NewLC(aFileName, aFileHandle, aClip, aReadBufferSize, 
                                        aTargetSampleRate, aChannelMode);
    CleanupStack::Pop(self);
    return self;

    }
    
CProcMP4InFileHandler* CProcMP4InFileHandler::NewLC(const TDesC& aFileName, 
                                                    RFile* aFileHandle,
                                                    CAudClip* aClip, TInt aReadBufferSize,
                                                    TInt aTargetSampleRate, 
                                                    TChannelMode aChannelMode)

    {

    CProcMP4InFileHandler* self = new (ELeave) CProcMP4InFileHandler();
    CleanupStack::PushL(self);
    self->ConstructL(aFileName, aFileHandle, aClip, aReadBufferSize,
                        aTargetSampleRate, aChannelMode);
    return self;

    }

void CProcMP4InFileHandler::GetPropertiesL(TAudFileProperties* aProperties)
    {
    PRINT((_L("CProcMP4InFileHandler::GetPropertiesL in") ));
    if (iProperties != 0)
    {
        *aProperties = *iProperties;
        PRINT((_L("CProcMP4InFileHandler::GetPropertiesL use cached properties, out") ));
        return;
    }
    
    if (iProperties == 0)
    {
        iProperties = new (ELeave) TAudFileProperties;
    }
    
    mp4_u32 audiolength = 0; 
    mp4_u32 audiotype = 0; 
    mp4_u8 framespersample = 0; 
    mp4_u32 timescale = 0; 
    mp4_u32 averagebitrate = 0;
    
    aProperties->iFileFormat = EAudFormatUnrecognized;
    aProperties->iAudioType = EAudTypeUnrecognized;
    aProperties->iDuration = 0;
    aProperties->iSamplingRate = 0;
    aProperties->iBitrate = 0;
    aProperties->iChannelMode = EAudChannelModeNotRecognized;
    aProperties->iBitrateMode = EAudBitrateModeNotRecognized;
    aProperties->iAudioTypeExtension = EAudExtensionTypeNoExtension;
    aProperties->iFrameLen = 0;
    aProperties->iFrameDuration = 0;
    aProperties->iFrameCount = 0;
    aProperties->iNumFramesPerSample = 1;
    aProperties->iAACObjectType = EAudAACObjectTypeNone;
    
    MP4Err err = MP4ParseRequestAudioDescription(iParser, 
        &audiolength, 
        &audiotype, 
        &framespersample, 
        &timescale, 
        &averagebitrate);
    
    
    if (err == MP4_OK)
        {
        if (audiotype == MP4_TYPE_MPEG4_AUDIO)
            {

            aProperties->iAudioType = EAudAAC_MPEG4;
            aProperties->iBitrateMode = EAudVariable;
            
            }
        else if (audiotype == MP4_TYPE_AMR_NB)
            {
            aProperties->iAudioType = EAudAMR;
            aProperties->iBitrateMode = EAudVariable;
            
            }
        else if (audiotype == MP4_TYPE_AMR_WB)
            {
            aProperties->iAudioType = EAudAMRWB;
            aProperties->iBitrateMode = EAudVariable;

            }
        aProperties->iBitrate = averagebitrate;
        
        // casting for PC-Lint
        TInt64 ti = (TInt64)(TInt)(audiolength*1000);
        TTimeIntervalMicroSeconds TTaudiolength(ti);
        
        aProperties->iDuration = TTaudiolength;
        aProperties->iFileFormat = EAudFormatMP4;
        //aProperties->iFrameLen = framespersample;
        
        
        }
    else if (err == MP4_NO_AUDIO)
        {
        aProperties->iAudioType = EAudNoAudio;
        *iProperties = *aProperties;
        return; 

        //User::Leave(KErrGeneral);
        }
    else
        {
        // a special case: there may be audio track but it is empty; if type was recognized, mark as no audio
        if ( (audiotype == MP4_TYPE_MPEG4_AUDIO) || (audiotype == MP4_TYPE_AMR_NB) || (audiotype == MP4_TYPE_AMR_WB))
            {
            PRINT((_L("CProcMP4InFileHandler::GetPropertiesL problems with getting audio description, mark no audio since audio type was recognized")));
            aProperties->iAudioType = EAudNoAudio;
            *iProperties = *aProperties;
            }
        PRINT((_L("CProcMP4InFileHandler::GetPropertiesL out with MP4Err %d"), err ));
        return;

        }
    
    const TInt KMaxBufferSize = 128;
    
    mp4_u8 *buffer = new (ELeave) mp4_u8[KMaxBufferSize];
    CleanupStack::PushL(buffer);
    
    mp4_u32 decspecinfosize = 0;
    
    err = MP4ParseReadAudioDecoderSpecificInfo(
        iParser, 
        buffer, 
        KMaxBufferSize, 
        &decspecinfosize);
    
    
    mp4AACTransportHandle mp4AAC_ff;
    
    if (err == MP4_OK && aProperties->iAudioType == EAudAAC_MPEG4)
        {
        
        PRINT((_L("CProcMP4InFileHandler::GetPropertiesL AAC found") ));
        
        TInt ret = ReadMP4AudioConfig(buffer, 
            decspecinfosize, 
            &mp4AAC_ff);
        if (ret != TRUE)
            {
            aProperties->iFileFormat = EAudFormatUnrecognized;
            aProperties->iAudioType = EAudTypeUnrecognized;
            User::Leave(KErrNotSupported);
            }
        
        //        ProgConfig progCfg = mp4AAC_ff.progCfg;
        AudioSpecificInfo audioInfo = mp4AAC_ff.audioInfo;
        aProperties->iSamplingRate = audioInfo.samplingFrequency;
        //        TInt tmp = mp4AAC_ff.progCfg.coupling.num_ele;
        
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
        
        if (audioInfo.channelConfiguration == 2)
            {
            aProperties->iChannelMode = EAudStereo;
            }
        else if (audioInfo.channelConfiguration == 1)
            {
            
            aProperties->iChannelMode = EAudSingleChannel;
            }
        else
            {
            aProperties->iChannelMode = EAudChannelModeNotRecognized; 
            }
        
        if (mp4AAC_ff.audioInfo.samplingFreqIdx !=
            mp4AAC_ff.progCfg.sample_rate_idx)
            {
            aProperties->iFileFormat = EAudFormatUnrecognized;
            aProperties->iAudioType = EAudTypeUnrecognized;
            aProperties->iDuration = 0;
            aProperties->iSamplingRate = 0;
            aProperties->iBitrate = 0;
            aProperties->iChannelMode = EAudChannelModeNotRecognized;
            aProperties->iBitrateMode = EAudBitrateModeNotRecognized;
            aProperties->iFrameLen = 0;
            aProperties->iFrameDuration = 0;
            aProperties->iFrameCount = 0;
            aProperties->iNumFramesPerSample = 1;
            CleanupStack::PopAndDestroy(buffer);
            PRINT((_L("CProcMP4InFileHandler::GetPropertiesL audio not recognized, out") ));
            User::Leave(KErrNotSupported);
            return;
            }
        

        
        iFrameInfo->iSampleRateID = static_cast<TUint8>(mp4AAC_ff.progCfg.sample_rate_idx);
        iFrameInfo->iSampleRateID = static_cast<TUint8>(mp4AAC_ff.audioInfo.samplingFreqIdx);
        

        iFrameInfo->iProfileID = static_cast<TUint8>(mp4AAC_ff.progCfg.profile);
        
        aProperties->iAACObjectType = TAudAACObjectType(iFrameInfo->iProfileID);    
        
        if (aProperties->iChannelMode == EAudStereo)
            {
            iFrameInfo->iNumChannels = 2;
            }
        else if (aProperties->iChannelMode == EAudSingleChannel)
            {
            iFrameInfo->iNumChannels = 1;
            }
        else
            {
            iFrameInfo->iNumChannels = 0;
            }
        
        
        iFrameInfo->iIs960 = mp4AAC_ff.audioInfo.gaInfo.FrameLengthFlag;
        
        iFrameInfo->iNumCouplingChannels = 0;
        
        iFrameInfo->isSBR = 0;
        iFrameInfo->iIsParametricStereo = 0;
        
        /*
        * Get frame parameters for eAAC+ codec. It is possible that the bitstream
        * is plain AAC but we don't know it before the 1st frame is parsed!
        */

        HBufC8* frame = 0;
        TInt size = 0;
        
        TInt32 time = 0;
        
        // Set the iProperties as GetEncAudioFrameL is using it!! 
        *iProperties = *aProperties;
        if(GetEncAudioFrameL(frame, size, time))
            {
            PRINT((_L("CProcMP4InFileHandler::GetPropertiesL check AAC+ parameters") ));

            TUint8* buf = const_cast<TUint8*>(frame->Right(frame->Size()).Ptr());
            CProcAACFrameHandler::GetEnhancedAACPlusParametersL(buf, frame->Size(), aProperties, iFrameInfo);
            delete frame;
            frame = 0;
          }
        
    }
    else if (err == MP4_OK && aProperties->iAudioType == EAudAMR)
        {
        aProperties->iChannelMode = EAudSingleChannel;
        aProperties->iSamplingRate = 8000;
        
        }
    else if (err == MP4_OK && aProperties->iAudioType == EAudAMRWB)
        {
        aProperties->iChannelMode = EAudSingleChannel;
        aProperties->iSamplingRate = 16000;
        
        }

    TInt frameAmount = 0;
    TInt frameSize = 0;
    TInt frameDuration = 0;
    
    
    TInt rv = ETrue;
    
    *iProperties = *aProperties;
    rv = GetAudioFrameInfoL(frameAmount, frameDuration, frameSize, aProperties);
    
    if (rv)
        {
        aProperties->iFrameCount = frameAmount;
        aProperties->iFrameLen = frameSize;
        
        // casting for PC-Lint
        TInt64 durMicro = (TInt64) (TInt) frameDuration*1000;
        aProperties->iFrameDuration = durMicro;
        
        }    
    
    
    
    
    
    *iProperties = *aProperties;
    
    
    CleanupStack::PopAndDestroy(buffer);
    
    PRINT((_L("CProcMP4InFileHandler::GetPropertiesL out") ));
    }


TBool CProcMP4InFileHandler::SeekAudioFrame(TInt32 aTime)
    {

    mp4_u32 position = static_cast<TUint32>(aTime);
    mp4_u32 audioPosition = 0;

    mp4_u32 videoPosition = 0;
    mp4_bool keyframe = EFalse;

    if (iMP4ReadBuffer != 0)
        {
        delete[] iMP4ReadBuffer;
        iMP4ReadBuffer = 0;
        iMP4ReadBufferPos = 0;
        iMP4ReadBufferSize = 0;    
        }

    MP4Err err = MP4ParseSeek(iParser,
                        position,
                        &audioPosition,
                        &videoPosition,
                        keyframe);

    if (err == MP4_OK)
        {
        
        iCurrentTimeMilliseconds = audioPosition;
        iLastTimeStamp = audioPosition - I64INT(iProperties->iFrameDuration.Int64()/1000);
        aTime = audioPosition;
        return ETrue;
        }
    else
        {
        return EFalse;
        }
    }

TBool CProcMP4InFileHandler::SeekCutInFrame()
    {

    iCurrentTimeMilliseconds = iCutInTime;
    return SeekAudioFrame(iCutInTime);
    }



TBool CProcMP4InFileHandler::SetNormalizingGainL(const CProcFrameHandler* aFrameHandler)
{
    
    
    if (iProperties->iAudioType == EAudAAC_MPEG4)
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
            
            delete point;
            
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
        
        
    }
    
    else if (iProperties->iAudioType == EAudAMR)
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
        }
    return ETrue;
    
}

TBool CProcMP4InFileHandler::ReadAudioDecoderSpecificInfoL(HBufC8*& aBytes, TInt aBufferSize)
    {

    
    aBytes = HBufC8::NewL(aBufferSize);

    CleanupStack::PushL(aBytes);

    mp4_u8 *buffer = new (ELeave) mp4_u8[aBufferSize];
    mp4_u32 decspecinfosize = 0;

    MP4Err err = MP4ParseReadAudioDecoderSpecificInfo(
        iParser,
        buffer,
        aBufferSize,
        &decspecinfosize);

    if (err == MP4_OK)
        {

        for (TUint a = 0 ; a < decspecinfosize ; a++)
            {
            aBytes->Des().Append(buffer[a]);
            }
        }
    else
        {
        delete[] buffer;
        buffer = 0;
        CleanupStack::PopAndDestroy(aBytes);
        aBytes = 0;
        return EFalse;;
        }
    
    delete[] buffer;
    buffer = 0;
        
    CleanupStack::Pop(aBytes);
    return ETrue;


    }


CProcMP4InFileHandler::~CProcMP4InFileHandler()
    {
    if (iFileOpen)
        {
        MP4ParseClose(iParser);
        }
    if (iSilentFrame != 0)
        {
        delete iSilentFrame;
        }

    if (iFrameInfo != 0)
        {
        delete iFrameInfo;
        iFrameInfo = 0;
        }
        
    if (iMP4ReadBuffer != 0)
        {
        delete[] iMP4ReadBuffer;
        }
        
    delete iDecoder;
    
    delete iFrameHandler;
    
    }


void CProcMP4InFileHandler::ConstructL(const TDesC& aFileName, 
                                       RFile* aFileHandle,
                                       CAudClip* aClip, 
                                       TInt /*aReadBufferSize*/,
                                       TInt aTargetSampleRate, 
                                       TChannelMode aChannelMode)
    {
    //InitAndOpenFileL(aFileName, aCutInTime, aReadBufferSize);

    iFrameInfo = new (ELeave) TAACFrameHandlerInfo;
    
    
    // init a raw silent frame
    // in other in file handlers it is created in CProcInFileHandler
        
    const TInt KBitsPerSample = 16;
    
    TInt samplesIn20ms = ((iTargetSampleRate) * 
        (KBitsPerSample)/8)/50;
    
    if (iChannelMode == EAudStereo)
        {
        samplesIn20ms *= 2;

        }
    if (samplesIn20ms % 2 != 0) samplesIn20ms--;
    
    iRawSilentFrame = HBufC8::NewL(samplesIn20ms);
    
    iRawSilentFrame->Des().SetLength(samplesIn20ms);
    iRawSilentFrame->Des().Fill(0);
        
    iRawSilentFrameDuration = 20;    
        
    
    iClip = aClip;
    
    iTargetSampleRate = aTargetSampleRate;
    iChannelMode = aChannelMode;

    if (iParser == 0)
        {   
         
        MP4Err err;
        if (aFileHandle)
        {
            err = MP4ParseOpenFileHandle(&iParser, aFileHandle); 
        }         
        else
        {                    
            TBuf<258> temp(aFileName);
            temp.ZeroTerminate();
            MP4FileName name = (MP4FileName)(temp.Ptr());        
            err = MP4ParseOpen(&iParser , name);
        }

        if (err == MP4_FILE_ERROR)
            {
            User::Leave(KErrNotFound);
            }
        else if (err != MP4_OK)
            {
            User::Leave(KErrGeneral);
            }
        iFileOpen = ETrue;

        }

    TAudFileProperties prop;
    GetPropertiesL(&prop);
    
    if (prop.iAudioType == EAudAAC_MPEG4)
        {
        // generate a silent frame ------------------>
        
        if (iProperties->iChannelMode == EAudSingleChannel)
            {

            iSilentFrame = HBufC8::NewL(KSilentMonoAACFrameLenght);    
            iSilentFrame->Des().Append(KSilentMonoAACFrame, KSilentMonoAACFrameLenght);

            }
        else if (iProperties->iChannelMode == EAudStereo)
            {

            iSilentFrame = HBufC8::NewL(KSilentStereoAACFrameLenght);    
            iSilentFrame->Des().Append(KSilentStereoAACFrame, KSilentStereoAACFrameLenght);


            }
        else
            {
            User::Leave(KErrNotSupported);
            }
        

        mp4_u32 frameDurationMilli = ProcTools::MilliSeconds(iProperties->iFrameDuration);
        
        
        iSilentFrameDuration = frameDurationMilli;
        

        
        // <------------------generate a silent frame
        
        iFrameHandler = CProcAACFrameHandler::NewL(*iFrameInfo);
                
        }
    else if (prop.iAudioType == EAudAMR)
        {
        const TInt KSilentFrameSize = 13;

        TUint8 silentFrame[KSilentFrameSize]=
            {
            0x04,0x63,0x3C,0xC7,0xF0,0x03,0x04,0x39,0xFF,0xE0,
            0x00,0x00,0x00
            };
        

        iSilentFrame = HBufC8::NewL(KSilentFrameSize);
        iSilentFrame->Des().Append(silentFrame, KSilentFrameSize); 
        iSilentFrameDuration = 20;

        
        iFrameHandler = CProcAMRFrameHandler::NewL();
        
        }
    else if (prop.iAudioType == EAudAMRWB)
        {
        TUint8 sf[18] = {0x04,0x10,0x20,0x00,0x21,
            0x1C,0x14,0xD0,0x11,0x40,0x4C,0xC1,0xA0,
            0x50,0x00,0x00,0x44,0x30};
        iSilentFrame = HBufC8::NewL(18);
        iSilentFrame->Des().Append(sf,18);
        iSilentFrameDuration = 20;
        
        iFrameHandler = CProcAWBFrameHandler::NewL();

        }
    else if (prop.iAudioType == EAudNoAudio )
        {
        iDecodingPossible = EFalse;
        return;
        }
    
    if (aClip != 0)
        {
        iCutInTime = ProcTools::MilliSeconds(aClip->CutInTime());
        
        }
    
    iDecoder = CProcDecoder::NewL();
    
    iDecodingPossible = iDecoder->InitL(*iProperties, aTargetSampleRate, aChannelMode);
    
    if (iClip != 0 && iClip->Normalizing())
        {
        SetNormalizingGainL(iFrameHandler);    
        }


    }

CProcMP4InFileHandler::CProcMP4InFileHandler()  : CProcInFileHandler(), iParser(0), 
                                                  iLastTimeStamp(0), iFrameInfo(0)
    {
    
    }

TBool CProcMP4InFileHandler::GetAudioFrameInfoL(TInt& aFrameAmount, 
                                                TInt& aAverageFrameDuration, 
                                                TInt& aAverageFrameSize,
                        TAudFileProperties* aProperties)
    {
        
    PRINT((_L("CProcMP4InFileHandler::GetAudioFrameInfoL in")));
    mp4_u32 audiolength = 0;
    mp4_u32 audiotype = 0;

    mp4_u8 framespersample = 0;
    mp4_u32 timescale = 0;
    mp4_u32 averagebitrate = 0;
    
    if (aProperties->iSamplingRate == 0) return KErrNotSupported;

    MP4Err err = MP4ParseRequestAudioDescription(iParser, &audiolength, &audiotype, 
                                        &framespersample, &timescale, &averagebitrate);


    if (aProperties->iAudioType == EAudAMR)
        {
        aAverageFrameDuration = 20;

        }
    else if(aProperties->iAudioType == EAudAMRWB)
        {
        aAverageFrameDuration = 20;

        }
    else if(aProperties->iAudioType == EAudAAC_MPEG4)
        {

        aAverageFrameDuration = (1024*1000)/(aProperties->iSamplingRate);

        }
    else
        {
        User::Leave(KErrNotSupported);
        }

    aFrameAmount = TInt((TInt)audiolength/aAverageFrameDuration);

    mp4_u32 lastPosition = 0;

    err = MP4ParseGetLastPosition(iParser, &lastPosition);

    SeekAudioFrame(0);


    if (err != MP4_OK)
        {
        return EFalse;
        
        }

    // ignore the first 2 frames
    HBufC8* frame;
    TInt size = 0;
    TInt32 time = 0;


      if(GetEncAudioFrameL(frame, size, time))
          {

        if (aProperties->iAudioType == EAudAAC_MPEG4)
            {
            
            /*
            * Read the eAAC+ parameters. Please note that the decoder specific
            * configuration information does, in case of explicit signalling,
            * signal the presence of SBR bitstream elements but not completely
            * the configuration of the SBR. For example, parametric stereo is
            * signalled only at the bitstream level (due to backwards comptibility
            * reasons).
            */

            TUint8* buf = const_cast<TUint8*>(frame->Right(frame->Size()).Ptr());
            CProcAACFrameHandler::GetEnhancedAACPlusParametersL(buf, size, aProperties, iFrameInfo);
            
            }
        delete frame;
        frame = 0;
          }

    if (GetEncAudioFrameL(frame, size, time))
        {
        delete frame;
        frame = 0;
        }

    // calculate the average of the next 100 frames

    TInt32 timeSum = 0;
    TInt32 sizeSum = 0;
    TInt divider = 0;
    TInt maxFrameNr = 100;
    if ( aFrameAmount < 100 )
        {
        maxFrameNr = aFrameAmount;
        }


    for (TInt a = 0 ; a < maxFrameNr ; a++)
        {
        if (GetEncAudioFrameL(frame, size, time))
            {
            timeSum += time;
            sizeSum += size;
            divider++; 
            delete frame;
            frame = 0;
            }
        else 
            {

            if ( a > 0 )
                {
                PRINT((_L("CProcMP4InFileHandler::GetAudioFrameInfoL breaking the loop since less than 100 frames in input (%d)"),a));
                break;
                }
            else
                {
                PRINT((_L("CProcMP4InFileHandler::GetAudioFrameInfoL can't get any frames from input")));
                SeekAudioFrame(lastPosition);
                return EFalse;
                }
            }

        }
    
    if (divider > 0)
        {
        aAverageFrameDuration = static_cast<TInt>(timeSum/divider); 
        aAverageFrameSize = static_cast<TInt>(sizeSum/divider);
        }

    SeekAudioFrame(lastPosition);
            
    PRINT((_L("CProcMP4InFileHandler::GetAudioFrameInfoL out")));
    return ETrue;
    }


TBool CProcMP4InFileHandler::GetEncAudioFrameL(HBufC8*& aFrame, TInt& aSize, TInt32& aTime)
    {
    PRINT((_L("CProcMP4InFileHandler::GetEncAudioFrameL in")));
    if (iParser == 0)
        {
        aFrame = 0;
        return EFalse;
        }

    mp4_u32 type = 0;
    if (iProperties->iAudioType == EAudAAC_MPEG4)
        type = MP4_TYPE_MPEG4_AUDIO;
    else if (iProperties->iAudioType == EAudAMR)
        {
        type = MP4_TYPE_AMR_NB;
        if (iMP4ReadBuffer != 0 && iMP4ReadBufferSize > 0 && iMP4ReadBufferPos < iMP4ReadBufferSize)
            {
            // we can just read a frame from in-buffer
            aTime = 20;
            TBool ret = ReadOneAMRFrameL(aFrame);
            
            if (iClip != 0 && iFrameHandler != 0)
                {
                TRAPD(err0, ManipulateGainL(aFrame));    
                if (err0 != KErrNone)
                    {
                    // something went wrong with the gain manipulation
                    // continue by returning the original frame
                    }
                
                }
            PRINT((_L("CProcMP4InFileHandler::GetEncAudioFrameL out from AMRNB branch")));
            return ret;
            }
        else
            {
            // if nothing more to read in the inBuffer
            delete[] iMP4ReadBuffer;
            iMP4ReadBuffer = 0;
            iMP4ReadBufferPos = 0;
            iMP4ReadBufferSize = 0;
            }
            
        
        // see if stuff left in the read buffer
        }
        
    else if (iProperties->iAudioType == EAudAMRWB)
        {
        type = MP4_TYPE_AMR_WB;
        
        if (iMP4ReadBuffer != 0 && iMP4ReadBufferSize > 0 && iMP4ReadBufferPos < iMP4ReadBufferSize)
            {
            // we can just read a frame from in-buffer
            aTime = 20;
            TBool ret = ReadOneAWBFrameL(aFrame);
            
            if (iClip != 0 && iFrameHandler != 0)
                {
                TRAPD(err0, ManipulateGainL(aFrame));    
                if (err0 != KErrNone)
                    {
                    // something went wrong with the gain manipulation
                    // continue by returning the original frame
                    }
                
                }
            PRINT((_L("CProcMP4InFileHandler::GetEncAudioFrameL out from AMRWB branch")));
            return ret;
            }
            
        }
        


    mp4_u32 framesize = 0; 
    mp4_u32 audiosize = 0;
    mp4_u32 timestamp = 0;
    mp4_u32 returnedframes = 0;
    mp4_u32 timestamp2 = 0;

    MP4Err err =MP4ParseNextFrameSize(iParser, type, &framesize);
    if (err == MP4_OK)
        {
        
        // some error handling
        if (type == MP4_TYPE_MPEG4_AUDIO && (framesize > KMaxAACFrameSize))
            {
            // we got too many bytes for some reason...
            delete[] iMP4ReadBuffer;
            iMP4ReadBuffer = 0;
            iMP4ReadBufferSize = 0;
            iMP4ReadBufferPos = 0;
            PRINT((_L("CProcClipInfoAO::CProcMP4InFileHandler::GetEncAudioFrameL out, too many bytes for AAC %d"), framesize ));
            return EFalse;
            
            }
        
        
        iMP4ReadBufferSize = framesize;
        delete[] iMP4ReadBuffer;
        iMP4ReadBuffer = 0;
        iMP4ReadBuffer = new (ELeave) mp4_u8[framesize];
        iMP4ReadBufferPos = 0;
        
        err = MP4ParseReadAudioFrames(iParser, 
            iMP4ReadBuffer, 
            framesize, 
            &audiosize, 
            &timestamp, 
            &returnedframes, 
            &timestamp2);

        if (err == MP4_OK)
            {
            aSize = framesize;
            aTime = timestamp - iLastTimeStamp;
            iCurrentTimeMilliseconds = timestamp;
            iLastTimeStamp = timestamp;

            if (type == MP4_TYPE_MPEG4_AUDIO)
                {
                aFrame = HBufC8::NewL(audiosize);    
                aFrame->Des().Append(iMP4ReadBuffer, audiosize);
                
                }
            else if (type == MP4_TYPE_AMR_NB)
                {
                aTime = 20;
                ReadOneAMRFrameL(aFrame);
                }
            else if (type == MP4_TYPE_AMR_WB)
                {
                aTime = 20;
                ReadOneAWBFrameL(aFrame);
                }
            }
        else
            {
            delete[] iMP4ReadBuffer;
            iMP4ReadBuffer = 0;
            iMP4ReadBufferSize = 0;
            iMP4ReadBufferPos = 0;
            PRINT((_L("CProcMP4InFileHandler::GetEncAudioFrameL out since MP4ParseReadAudioFrames failed")));
            return EFalse;

            }

        // delete if MP4AUDIO.
        // AMR frames will be read from buffer as MP4 library 
        // might have returned more than one AMR-frames
        if (type == MP4_TYPE_MPEG4_AUDIO)
            {
            delete[] iMP4ReadBuffer;
            iMP4ReadBuffer = 0;
            iMP4ReadBufferSize = 0;
            iMP4ReadBufferPos = 0;
            }
        
        }
    else
        {
        PRINT((_L("CProcMP4InFileHandler::GetEncAudioFrameL out since MP4ParseNextFrameSize failed, error %d"),err));
        return EFalse;

        }

    TInt err2 = KErrNone;
    
    if (iProperties->iAudioTypeExtension == EAudExtensionTypeNoExtension)
        {
        
        // AAC Plus is manipulated in time domain 
    
        if (iClip != 0 && iFrameHandler != 0)
            {
            TRAP(err2, ManipulateGainL(aFrame));    
            }
        
        }
    
    if (err2 != KErrNone)
        {
        // something went wrong with the gain manipulation
        // continue by returning the original frame
        }
    aSize = aFrame->Size();

    PRINT((_L("CProcMP4InFileHandler::GetEncAudioFrameL out successfully")));
    return ETrue;
    }


TBool CProcMP4InFileHandler::ReadTimeScale(mp4_u32& aTimescale)
    {

    mp4_u32 audiolength = 0; 
    mp4_u32 audiotype = 0; 
    mp4_u8 framespersample = 0; 
    mp4_u32 timescale = 0; 
    mp4_u32 averagebitrate = 0;

    MP4Err err = MP4ParseRequestAudioDescription(iParser, 
        &audiolength, 
        &audiotype, 
        &framespersample, 
        &timescale, 
        &averagebitrate);



    if (err == MP4_OK)
        {
        aTimescale = timescale;
        return ETrue;
        }
    else
        {
        return EFalse;
        }

    }


MP4Err CProcMP4InFileHandler::ParseRequestAudioDescription(
                                           mp4_u32 *aAudiolength,
                                           mp4_u32 *aAudiotype,
                                           mp4_u8 *aFramespersample,
                                           mp4_u32 *aTimescale,
                                           mp4_u32 *aAveragebitrate)

    {

    return MP4ParseRequestAudioDescription(iParser,
                                            aAudiolength,
                                           aAudiotype,
                                           aFramespersample,
                                           aTimescale,
                                           aAveragebitrate);

    }


TBool CProcMP4InFileHandler::GetInfoForFrameHandler(TAACFrameHandlerInfo& aAACInfo)
    {
    if (iParser == 0)
        {
        return EFalse;
        }

    aAACInfo.iIs960 = iFrameInfo->iIs960;
    aAACInfo.iIsParametricStereo = iFrameInfo->iIsParametricStereo;
    aAACInfo.iNumChannels = iFrameInfo->iNumChannels;
    aAACInfo.iNumCouplingChannels = iFrameInfo->iNumCouplingChannels;
    aAACInfo.iProfileID = iFrameInfo->iProfileID;
    aAACInfo.iSampleRateID = iFrameInfo->iSampleRateID;
    aAACInfo.isSBR = iFrameInfo->isSBR;


    mp4_u8 *buffer = 0;
    
    const TInt bufSize = 64; 
    TRAPD(nerr, buffer = new (ELeave) mp4_u8[bufSize])
    
    if (nerr != KErrNone)
        {
        delete[] buffer;
        return EFalse;
        }
    
    
    mp4_u32 decspecinfosize = 0;

    MP4Err err = MP4ParseReadAudioDecoderSpecificInfo(
        iParser, 
        buffer, 
        64, 
        &decspecinfosize);


    if (err == MP4_OK)
        {

        mp4AACTransportHandle mp4AAC_ff;
        int16 err2 = ReadMP4AudioConfig(buffer, 
                                      decspecinfosize, 
                                      &mp4AAC_ff);
        if (err2 != TRUE)
            {
            
            delete[] buffer;
            return EFalse;

            }

        aAACInfo.iSampleRateID = static_cast<TUint8>(mp4AAC_ff.progCfg.sample_rate_idx);
        aAACInfo.iSampleRateID = static_cast<TUint8>(mp4AAC_ff.audioInfo.samplingFreqIdx);

        aAACInfo.iProfileID = static_cast<TUint8>(mp4AAC_ff.progCfg.profile);
        
        if (iProperties->iChannelMode == EAudStereo)
            {
            aAACInfo.iNumChannels = 2;
            }
        else if (iProperties->iChannelMode == EAudSingleChannel)
            {
            aAACInfo.iNumChannels = 1;
            }
        else
            {
            aAACInfo.iNumChannels = 0;
            }

        
        aAACInfo.iIs960 = mp4AAC_ff.audioInfo.gaInfo.FrameLengthFlag;

        aAACInfo.iNumCouplingChannels = 0;

        aAACInfo.iIsParametricStereo = 0;
        aAACInfo.isSBR = 0;

        }

    else
        {

        delete[] buffer;
        return EFalse;
        
        }

    delete[] buffer;
    return ETrue;
        
        

    }
    

    
    
TBool CProcMP4InFileHandler::ReadOneAMRFrameL(HBufC8*& aOneAMRFrame)
    {
        if (iMP4ReadBuffer == 0 || iMP4ReadBufferSize < 1 
                                || iMP4ReadBufferPos >= iMP4ReadBufferSize) 
            {
            aOneAMRFrame = 0;
            return EFalse;    
            }
        
        
        TInt readSize = 0;
        TUint8 dec_mode = (enum Mode)((iMP4ReadBuffer[iMP4ReadBufferPos] & 0x0078) >> 3);
        
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
        
        aOneAMRFrame = HBufC8::NewL(readSize+1);
        
//        TPtr8 tmpDes((TPtr8)aOneAMRFrame->Des());

        TInt lastByte = iMP4ReadBufferPos + readSize;
        
        for (; iMP4ReadBufferPos <= lastByte ; iMP4ReadBufferPos++)
            {
            
            aOneAMRFrame->Des().Append(iMP4ReadBuffer[iMP4ReadBufferPos]);
            }
//        iMP4ReadBufferPos++;

        return ETrue;
        
    }
    
TBool CProcMP4InFileHandler::ReadOneAWBFrameL(HBufC8*& aOneAWBFrame)
    {
        if (iMP4ReadBuffer == 0 || iMP4ReadBufferSize < 1 
                                || iMP4ReadBufferPos >= iMP4ReadBufferSize) 
            {
            aOneAWBFrame = 0;
            return EFalse;    
            }
        TUint8 toc = iMP4ReadBuffer[iMP4ReadBufferPos];
        TUint8 mode = static_cast<TUint8>((toc >> 3) & 0x0F);

        TInt readSize = KAWBPacked_size[mode];
            
        aOneAWBFrame = HBufC8::NewL(readSize+1);
        
        TInt lastByte = iMP4ReadBufferPos + readSize;
        
        for (; iMP4ReadBufferPos <= lastByte ; iMP4ReadBufferPos++)
            {
            
            aOneAWBFrame->Des().Append(iMP4ReadBuffer[iMP4ReadBufferPos]);
            }

        return ETrue;
        
    }
    
