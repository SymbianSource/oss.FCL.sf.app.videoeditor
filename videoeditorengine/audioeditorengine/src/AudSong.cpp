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




#include "AudSong.h"
#include "AudClip.h"
#include "AudPanic.h"
#include "ProcInFileHandler.h"
#include "ProcADTSInFileHandler.h"
#include "ProcAMRInFileHandler.h"
#include "ProcMP4InFileHandler.h"
#include "ProcAWBInFileHandler.h"
#include "AACConstants.h"
#include "audconstants.h"

#include "AudProcessor.h"
#include "AACApi.h"
#include "aedproctimeestimate.h"

#include <e32base.h>

// Debug print macro
#if defined _DEBUG 
#include <e32svr.h>
#define PRINT(x) RDebug::Print x;
#else
#define PRINT(x)
#endif

EXPORT_C CAudSong* CAudSong::NewL(RFs *aFs)
    {
    CAudSong* self = NewLC(aFs);
    CleanupStack::Pop(self);
    return self;
    }

    
EXPORT_C CAudSong* CAudSong::NewLC(RFs *aFs)
    {
    CAudSong* self = new (ELeave) CAudSong(aFs);
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
    }
 
CAudSong::CAudSong(RFs *aFs) : iFs(aFs), iClipArray(32),  iSongDuration(0), 
                                iSongDurationManuallySet(EFalse), iObserverArray(16), iNormalize(EFalse)
    {
    }


void CAudSong::ConstructL()
    {
    iProperties = new (ELeave) TAudFileProperties();
    
    iProperties->iAudioType = EAudNoAudio;
    iProperties->iFileFormat = EAudFormatUnrecognized;
    iProperties->iDuration = 0;
    iProperties->iFrameCount = 0;
    iProperties->iFrameLen = 0;
    iProperties->iFrameDuration = 0;


    iProcessOperation = CAudSongProcessOperation::NewL(this);
    iAddOperation = CAudSongAddClipOperation::NewL(this);


#ifdef _WRITE_OUTPUT_TO_FILE_


    TBuf<32> fileName;
    _LIT(KFn, "C:\\audioEngineOut.");
    _LIT(KTextFileName, "C:\\audioEngineOut.txt");
    
    fileName.Append(KFn);
    
    if (iProperties->iAudioType == EAudAMR)
        {
        fileName.Append(_L("amr"));
        }
    else
        {
        fileName.Append(_L("aac"));
        }
    
    
    User::LeaveIfError(iDebFs.Connect());
    iDebFs.Delete(fileName);
    iDebFs.Delete(KTextFileName);

    iFileOpen = EFalse;
    TInt err1 = iAudioFile.Create(iDebFs, fileName, EFileWrite);
    
    TInt err2 = iTextFile.Create(iDebFs, KTextFileName, EFileWrite);
    
    if (err1 == KErrNone && err2 == KErrNone)
        {
        iFileOpen = ETrue;
        }
        
    if (iFileOpen && iProperties->iAudioType == EAudAMR)
        {
        iAudioFile.Write(_L8("#!AMR"));
        }
    
#endif


    }


EXPORT_C CAudSong::~CAudSong() 
    {

    Reset(EFalse);
    delete iProperties;
    iProperties = 0;
    delete iProcessOperation;
    iProcessOperation = 0;
    delete iAddOperation;
    iAddOperation = 0;


    iObserverArray.Reset();


#ifdef _WRITE_OUTPUT_TO_FILE_
    iAudioFile.Close();
    iTextFile.Close();
    iDebFs.Close();
    
#endif

    }

EXPORT_C TInt CAudSong::GetSizeEstimateL() const
    {
    
    
    // if there are no clips added yet
    if (iClipArray.Count() == 0)
        {
        return 0;
        }
        
    TInt durationMilli = ProcTools::MilliSeconds(iClipArray[iClipArray.Count()-1]->EndTime());
    TInt silenceDuration = ProcTools::MilliSeconds(iSongDuration) - durationMilli;
    
    if (silenceDuration < 0)
        {
        silenceDuration = 0;
        }
       
    TInt sizeInBytes = 0;
        
    const TInt KBitrate = iProperties->iBitrate;
    const TInt KBitsInByte = 8;
    TInt KByterate = KBitrate/KBitsInByte;
        
    // make sure we round up
    sizeInBytes = (durationMilli/1000+1)*KByterate;
    
    TInt frameDurationMilli = 20; // AMR
    TInt silentFrameLen = 13; // AMR
    
    if (iProperties->iAudioType == EAudAAC_MPEG4)
        {
        
        frameDurationMilli = ((1024*1000)/(iProperties->iSamplingRate));
        
        if (iProperties->iChannelMode == EAudSingleChannel)
            {
            silentFrameLen = KSilentMonoAACFrameLenght;
            }
        else
            {
            silentFrameLen = KSilentStereoAACFrameLenght;
            }
        
        }
        
    if (frameDurationMilli > 0)
        {
        TInt silentFrames = silenceDuration/frameDurationMilli;
        TInt silenceSize = silentFrames*silentFrameLen;
        sizeInBytes += silenceSize;
        
        }

    return sizeInBytes;
    
    }
    
EXPORT_C TInt CAudSong::GetFrameSizeEstimateL(TTimeIntervalMicroSeconds aStartTime, 
                                              TTimeIntervalMicroSeconds aEndTime) const
	{
	// if there are no clips added yet
    if (iClipArray.Count() == 0)
        {
        return 0;
        }
	
	TInt frameDurationMicro = 20000; // AMR
    
    if (iProperties->iAudioType == EAudAAC_MPEG4)
        {
        
        frameDurationMicro = ((1024*1000)/(iProperties->iSamplingRate))*1000;
        }
    
    // just in case to prevent infinite loop    
    if (frameDurationMicro <= 0)
        {
        return 0;
        }
	
    TInt size = 0;
    
    // Calculate the time of the first frame included in the time interval
    TInt64 firstFrameIndex = aStartTime.Int64() / frameDurationMicro;
    TInt64 currentTime = firstFrameIndex * frameDurationMicro;
    
    while (currentTime + frameDurationMicro <= aEndTime.Int64())        // Make sure the whole frame fits inside the time interval
        {
     
        TBool silence = ETrue;
    	TInt clipIndex = 0;
    	TInt overLappingClips = 0;
    	
    	for (TInt a = 0 ; a < iClipArray.Count() ; a++)
    	    {
    	    if (!iClipArray[a]->Muting())
    	        {
    	        
        	    if ((currentTime >= iClipArray[a]->iStartTime.Int64() + iClipArray[a]->iCutInTime.Int64()) &&
        	       (currentTime < iClipArray[a]->iStartTime.Int64() + iClipArray[a]->iCutOutTime.Int64()))
        	        
        	        {
        	        silence = EFalse;
        	        clipIndex = a;
                    overLappingClips++;
        	        
        	        }
    	        }
    	    }
    	    
    	TInt frameSize = 0;
    	if (silence)
    	    {
    	    
    	    // if there is no audio around "aTime", just return a silent frame length
    	    
    	    if (iProperties->iAudioType == EAudAAC_MPEG4)
                {
                
                if (iProperties->iChannelMode == EAudSingleChannel)
                    {
                    frameSize = KSilentMonoAACFrameLenght;
                    }
                else
                    {
                    frameSize = KSilentStereoAACFrameLenght;
                    }
                }
    	    else if (iProperties->iAudioType == EAudAMR)
    	        {
    	        
    	        TInt KSilentFrameLen = 13; // AMR
    	        frameSize = KSilentFrameLen;
    	        
    	        }
    	        
    	    size +=frameSize;
    	    currentTime += frameDurationMicro;
    	    continue;
    	    
    	    }
    	    
    	    
    	    
    	// if not silent, estimate according to bitrate if transcoding
    	// if no transcoding is necessary, return the frame length of the original
    	// input clip
    	
    	// input clip is not transcoded if the original clip has the same
    	// audio properties as the output clip and no mixing is needed
    	    
    	TAudFileProperties clipProp = iClipArray[clipIndex]->Info()->Properties();
    	   
    	TBool clipTranscoded = EFalse;
    	    
    	if (clipProp.iAudioType != 
            iProperties->iAudioType ||
            clipProp.iSamplingRate != 
            iProperties->iSamplingRate ||
            clipProp.iChannelMode != 
            iProperties->iChannelMode)
                {
                clipTranscoded = ETrue;
                }
    	
    	if (overLappingClips > 1)
    	    {
    	    clipTranscoded = ETrue;
    	    }
    	
    	
    	if (clipTranscoded)
    	    {
    	    const TInt KBitrate = iProperties->iBitrate;
            const TInt KBitsInByte = 8;
            
            TInt KByterate = KBitrate/KBitsInByte;    
        	
        	TInt frameDurationMilli = 20; // AMR
            
            if (iProperties->iAudioType == EAudAAC_MPEG4)
                {
                
                frameDurationMilli = ((1024*1000)/(iProperties->iSamplingRate));
        	
        		}
        	
        	frameSize = (KByterate/(1000/frameDurationMilli)+1);
        	    
    	    }
    	else
    	    {
    	    
    	    frameSize = clipProp.iFrameLen;
    	    
    	    }    
    	
    	size +=frameSize;
    	currentTime += frameDurationMicro;
    	continue;
    	
        }
        
    return size;

	}

EXPORT_C TAudFileProperties CAudSong::OutputFileProperties() const 
    {

    return *iProperties;
    }
    

EXPORT_C TBool CAudSong::GetMP4DecoderSpecificInfoLC(HBufC8*& aDecSpecInfo, TInt aMaxSize) const
    {
    
    if (iClipArray.Count() == 0)
        {
        return EFalse;
        }
    
    if (iProperties->iAudioType == EAudAAC_MPEG4)
        
        {
        
        int16 frameLen = 1024;
        int32 sampleRate = iProperties->iSamplingRate;
        uint8 profile = LC_OBJECT;
        uint8 nChannels = 1;
        uint8 decSpecInfo[16];
        int16 nConfigBytes;
        
        if (iProperties->iChannelMode == EAudStereo)
            {
            nChannels = 2;
            }
        
        
        //nConfigBytes = AACGetMP4ConfigInfo(&aacInfo, decSpecInfo, 16);
        nConfigBytes = AACGetMP4ConfigInfo(sampleRate, profile, 
            nChannels, frameLen, decSpecInfo, aMaxSize);


        if (nConfigBytes > 0)
            {

            aDecSpecInfo = HBufC8::NewLC(nConfigBytes);
            aDecSpecInfo->Des().Append(decSpecInfo, nConfigBytes);

            }
            
        return ETrue;
        
        }
    else if (iProperties->iAudioType == EAudAMR)
        {
        aDecSpecInfo = HBufC8::NewLC(aMaxSize);
        
        const TUint8 frameDecSpecInfo[] = {0x14,0x08};    //the decoder specific 
        const TInt frameSize = 2;  //constant as maximum size of decoderspecific info is 2

        for (TInt a = 0 ; a < frameSize ; a++)
            {
            aDecSpecInfo->Des().Append(frameDecSpecInfo[a]);
            }
        
        
        return ETrue;
        }
      
    else
        {
        return EFalse;
        }
    

    }

EXPORT_C TBool CAudSong::GetTimeEstimateL(MAudTimeEstimateObserver& aObserver,
                                            TAudType aAudType,
                                            TInt aSamplingRate,
                                            TChannelMode aChannelMode,
                                            TInt aBitRate)
    {
    
    
    if (iClipArray.Count() == 0)
        {
        return EFalse;
        }
    
    if (SetOutputFileFormat(aAudType, aSamplingRate, aChannelMode, aBitRate))
        {
        return iProcessOperation->GetTimeEstimateL(aObserver);
        
        }
    else
        {
        return EFalse;
        }
    
    
    }
    
EXPORT_C TTimeIntervalMicroSeconds CAudSong::GetTimeEstimateL()
    {
    TAudFileProperties prop;
    TInt64 estimatedTime = TInt64(0);
    TReal complexityFactor = 0.0;
    TInt a;
    
	for (a = 0; a < iClipArray.Count() ; a++) 
		{		
		complexityFactor = 0.0;
		
		prop = iClipArray[a]->Info()->Properties();
		if ( iClipArray[a]->Muting() )
		    {
		    // the clip is muted
		    complexityFactor = KAEDMutingComplFactor;
		    }
		else if ( (prop.iAudioType != iProperties->iAudioType )
		    || (prop.iChannelMode != iProperties->iChannelMode )
		    || (prop.iSamplingRate != iProperties->iSamplingRate ) )
		    {
		    // need transcoding
		    
		    // decoding
		    switch (prop.iAudioType)
		        {
	            case EAudAMR :
	                {
    		        // AMR decoding
    		        complexityFactor = KAEDAMRDecComplFactor;
	                }
	                break;
	            case EAudAAC_MPEG4 :
	                {
    		        // AAC decoding
    		        complexityFactor = KAEDAACDecComplFactor;
	                }
	                break;
	            case EAudAMRWB :
	                {
    		        // AMR-WB decoding
    		        complexityFactor = KAEDAMRWBDecComplFactor;
	                }
	                break;
	            case EAudMP3 :
	                {
    		        // MP3 decoding
    		        complexityFactor = KAEDMP3DecComplFactor;
	                }
	                break;
                default:
                    {
	                //EAudWAV
    		        complexityFactor = KAEDWavDecComplFactor;
                    }
		        }
	        if ( prop.iChannelMode == EAudStereo )
	            {
	            complexityFactor += KAEDAACStereoDecAddComplFactor;
	            }
	        if ( prop.iSamplingRate > 8000 )
	            {
	            complexityFactor *= prop.iSamplingRate/16000;
	            }
		        
		        
		    // encoding
		    if (iProperties->iAudioType == EAudAMR)
                {
		        // AMR encoding
		        complexityFactor += KAEDAMREncComplFactor;
                }
            else
                {
		        // AAC encoding
		        complexityFactor += KAEDAACEncComplFactor;
		        if ( iProperties->iChannelMode == EAudStereo )
		            {
		            complexityFactor += KAEDAACStereoEncAddComplFactor;
		            }
		        complexityFactor *= iProperties->iSamplingRate/16000;
                }
                
                
		    }
		else if (iClipArray[a]->DynamicLevelMarkCount() > 0)
		    {
		    // need bitstream processing (level control etc)
		    complexityFactor = KAEDBitstreamProcComplFactor;
		    }
		else
		    {
		    // just passing through
		    complexityFactor = KAEDPassThroughComplFactor;
		    }
		    
		    
		
        estimatedTime = estimatedTime + TInt64(complexityFactor * I64INT(iClipArray[a]->EditedDuration().Int64()));
		}
		
    return estimatedTime;
    }
    
EXPORT_C TInt CAudSong::GetFrameDurationMicro()
    {
    TInt frameDurationMicro = 20000; // AMR
    
    if (iProperties->iAudioType == EAudAAC_MPEG4)
        {
        frameDurationMicro = ((1024 * 1000) / iProperties->iSamplingRate) * 1000;
        }
        
    return frameDurationMicro;
    }
    

EXPORT_C TInt CAudSong::ClipCount(TInt aTrackIndex) const 
    {


    if (aTrackIndex == KAllTrackIndices)
        {
        return iClipArray.Count();

        }

    TInt amount = 0;
    for (TInt a = 0; a < iClipArray.Count() ; a++) 
        {    
        if (iClipArray[a]->TrackIndex() == aTrackIndex) amount++;

        }

    return amount;

    }


EXPORT_C CAudClip* CAudSong::Clip(TInt aIndex, TInt aTrackIndex) const 
    {
    
    if (aTrackIndex == KAllTrackIndices)
        {
        return iClipArray[aIndex];
        }

	
	TInt index = 0;
	TInt a = 0;
	TBool found = EFalse;
	
	if (aTrackIndex == KAllTrackIndices)
	    {
	    return iClipArray[aIndex];
	    }

	for (a = 0; a < iClipArray.Count() ; a++) 
		{		
		
		if (iClipArray[a]->TrackIndex() == aTrackIndex) index++;
		
		if (index == aIndex+1) 
			{
			found = ETrue;
			break;
			}
		

		}

	if (found) 
		{
		return iClipArray[a];
		}
	else 
		{
		TAudPanic::Panic(TAudPanic::EAudioClipIllegalIndex);
		}
	return NULL;
	}



EXPORT_C void CAudSong::AddClipL(const TDesC& aFileName,
        TTimeIntervalMicroSeconds aStartTime, TInt aTrackIndex,
        TTimeIntervalMicroSeconds aCutInTime,
        TTimeIntervalMicroSeconds aCutOutTime) 
    {

    PRINT((_L("CAudSong::AddClipL in")));
    if (iAddOperation->iClip != 0) 
        {
        TAudPanic::Panic(TAudPanic::ESongAddOperationAlreadyRunning);
        }
    if (iProcessOperation->iProcessor != 0 ) 
        {
        TAudPanic::Panic(TAudPanic::ESongProcessingOperationAlreadyRunning);
        }

    
    iAddOperation->iClip = CAudClip::NewL(this, aFileName, aStartTime, *iAddOperation, aTrackIndex);
    iAddOperation->iClip->iCutInTime = aCutInTime;
    iAddOperation->iClip->iCutOutTime = aCutOutTime;

    PRINT((_L("CAudSong::AddClipL out")));

    }
    

EXPORT_C void CAudSong::RemoveClip(TInt aIndex, TInt aTrackIndex) 
    {
    PRINT((_L("CAudSong::RemoveClip in")));

    TInt index = -1;
    TInt a = 0;
    TBool found = EFalse;

    for (a = 0; a < iClipArray.Count() ; a++) 
        {        
    
        
        if (iClipArray[a]->TrackIndex() == aTrackIndex) index++;
        
        if (index == aIndex) 
            {
            found = ETrue;
            break;
            }
        
        }
    
    
    if (found) 
        {
        
        CAudClip* clip = iClipArray[a];
        iClipArray.Remove(a);
        delete clip;        
        UpdateClipIndexes();
        FireClipRemoved(this, aIndex, aTrackIndex);
        }
    else 
        {
        TAudPanic::Panic(TAudPanic::EAudioClipIllegalIndex);
        }
    
    PRINT((_L("CAudSong::RemoveClip out")));

    }


EXPORT_C TBool CAudSong::SetOutputFileFormat(TAudType aAudType,
                                            TInt aSamplingRate,
                                            TChannelMode aChannelMode,
                                            TInt aBitRate)
    {
    PRINT((_L("CAudSong::SetOutputFileFormat in")));
    
    // allow both EAudAAC_MPEG2 and EAudAAC_MPEG4
    // as inpyt type, but consider all AAC_ MPEG as mpeg4
    
    if (aAudType == EAudAAC_MPEG2) 
        {
        aAudType = EAudAAC_MPEG4;
        }
    
    // make sure the given parameters are correct
   
    if (aBitRate == KAudBitRateDefault)
        {
        // the defaut bitrates:
        PRINT((_L("CAudSong::SetOutputFileFormat use default bitrate")));
        if (aAudType == EAudAMR)
            {
            aBitRate = KAedBitRateAMR;
            }
        else if (aAudType == EAudAAC_MPEG4)
            {
            if (aSamplingRate == KAedSampleRate16kHz) 
                {
                aBitRate = KAedBitRateAAC16kHz;
                }
            else 
                {
                aBitRate = KAedBitRateAAC48kHz;
                }
            }
        }
    
    if (aAudType == EAudAAC_MPEG4)
        {
        
        iProperties->iAudioType = EAudAAC_MPEG4; 
        iProperties->iAACObjectType = EAudAACObjectTypeLC;
        
        TInt channels = (aChannelMode == EAudSingleChannel) ? 1 : 2;
        
        // legal sampling rates are 16000 and 48000 Hz      
        if (aSamplingRate == KAedSampleRate16kHz)
            {
            if (aBitRate < KAedAACMinBitRateMultiplier * KAedSampleRate16kHz * channels ||
                aBitRate > KAedAACMaxBitRateMultiplier * KAedSampleRate16kHz * channels)
                {
                // illegal bitrate
                PRINT((_L("CAudSong::SetOutputFileFormat out, unsupported bitrate given")));
                return EFalse;
                }
            else 
                {
                iProperties->iSamplingRate = aSamplingRate;
                iProperties->iBitrate = aBitRate;
                iProperties->iChannelMode = aChannelMode;
                }
            }
        else if (aSamplingRate == KAedSampleRate48kHz)
            {
            if (aBitRate < KAedAACMinBitRateMultiplier * KAedSampleRate48kHz * channels ||
                aBitRate > KAedAACMaxBitRateMultiplier * KAedSampleRate48kHz * channels)
                {
                // illegal bitrate
                PRINT((_L("CAudSong::SetOutputFileFormat out, unsupported bitrate given")));
                return EFalse;
                }
            else 
                {
                iProperties->iSamplingRate = aSamplingRate;
                iProperties->iBitrate = aBitRate;
                iProperties->iChannelMode = aChannelMode;
                }
            }
        else
            {
            PRINT((_L("CAudSong::SetOutputFileFormat out, unsupported sampling rate given")));
            return EFalse;
            }
            
        }
        
    
    else if (aAudType == EAudAMR)
        {
        
        iProperties->iAudioType = EAudAMR;
        // for AMR the bitrate is always set to 12200 and sampling rate to 8000
        iProperties->iSamplingRate = KAedSampleRate8kHz;
        iProperties->iBitrate = KAedBitRateAMR;
        iProperties->iChannelMode = EAudSingleChannel;
        
        }
        
    else 
        {
        PRINT((_L("CAudSong::SetOutputFileFormat out, unsupported output format given")));
        return EFalse;
        }
    

    PRINT((_L("CAudSong::SetOutputFileFormat out")));
    return ETrue;
    }

EXPORT_C TBool CAudSong::AreOutputPropertiesSupported(const TAudFileProperties& aProperties )
    {
    if (   ( aProperties.iAudioType == EAudAAC_MPEG4 )
        && ((aProperties.iSamplingRate == KAedSampleRate16kHz) 
        ||  (aProperties.iSamplingRate == KAedSampleRate48kHz)))
        {
        return ETrue;
        }
    else if ( (aProperties.iAudioType == EAudAMR) 
        &&    (aProperties.iSamplingRate == KAedSampleRate8kHz))
        {
        return ETrue;
        }
    else
        {
        return EFalse;
        }
    }
    

EXPORT_C TBool CAudSong::SyncStartProcessingL()
    {
    PRINT((_L("CAudSong::SyncStartProcessingL")));
    return iProcessOperation->StartSyncProcL();    
    }

EXPORT_C TBool CAudSong::SyncProcessFrameL(HBufC8*& aFrame, TInt& aProgress,
                                       TTimeIntervalMicroSeconds& aDuration)
    {
    PRINT((_L("CAudSong::SyncProcessFrameL in")));
    
    aFrame = 0;
    TBool ret = iProcessOperation->ProcessSyncPieceL(aFrame, aProgress, aDuration);
    
#ifdef _WRITE_OUTPUT_TO_FILE_
    
    
    if (!ret)
        {
        
        if (iFileOpen)
            {
        
        
        
            TBuf8<32> mes;
            mes.Append(_L8("aProgress: "));
            mes.AppendNum(aProgress);
            mes.Append(_L8("aDuration: "));
            mes.AppendNum(I64INT(aDuration.Int64()/1000));
            mes.Append(_L8("\n"));
            
            
            iTextFile.Write(mes);
            
            
            if (iProperties->iAudioType == EAudAMR)
                {
                iAudioFile.Write(aFrame->Des());
                
                }
            else
                {
                TBuf8<7> adtsHeader;
                
                ProcTools::GenerateADTSHeaderL(adtsHeader, aFrame->Size(), *iProperties);
                iAudioFile.Write(adtsHeader);
                iAudioFile.Write(aFrame->Des());
                
                }
            
            }
        
        }
    
#endif
    
    PRINT((_L("CAudSong::SyncProcessFrameL out")));
    return ret;

    }

EXPORT_C void CAudSong::SyncCancelProcess() 
    {
    
    iProcessOperation->Cancel();

    }


EXPORT_C void CAudSong::Reset(TBool aNotify) 
    {
    iSongDurationManuallySet = EFalse;

    iDynamicLevelMarkArray.ResetAndDestroy();
    iClipArray.ResetAndDestroy();

    if (aNotify) 
        {
        FireSongReseted(*this);
        }
    }

EXPORT_C TBool CAudSong::SetDuration(TTimeIntervalMicroSeconds aDuration)
    {

    if (aDuration.Int64() > 0) 
        {
        iSongDuration = aDuration;
        iSongDurationManuallySet = ETrue;
        return ETrue;
        }

    return EFalse;
    }


EXPORT_C void CAudSong::RegisterSongObserverL(MAudSongObserver* aObserver) 
    {
    for (TInt i = 0; i < iObserverArray.Count(); i++)
        {
        if (iObserverArray[i] == aObserver)
            {
            TAudPanic::Panic(TAudPanic::ESongObserverAlreadyRegistered);
            }
        }

    User::LeaveIfError(iObserverArray.Append(aObserver));
    }


EXPORT_C void CAudSong::UnregisterSongObserver(MAudSongObserver* aObserver) 
    {
    for (TInt i = 0; i < iObserverArray.Count(); i++) 
        {
        if (iObserverArray[i] == aObserver) 
            {
            iObserverArray.Remove(i);
            return;
            }
        }

    TAudPanic::Panic(TAudPanic::ESongObserverNotRegistered);
    }


void CAudSong::UpdateClipIndexes() 
    {

    for (TInt i = 0; i < iClipArray.Count() ; i++)
        {
        
        iClipArray[i]->iIndex = Index2IndexOnTrack(i);
        
        }
    
    }

void CAudSong::UpdateClipArray() 
    {

    TLinearOrder<CAudClip> order(CAudClip::Compare);
    iClipArray.Sort(order);


    }

TInt CAudSong::Index2IndexOnTrack(TInt aIndex) 
    {

    if (aIndex > iClipArray.Count()) 
        {
        TAudPanic::Panic(TAudPanic::EInternal);
        }
    TInt indexOnTrack = 0;
    TInt trackIndex = iClipArray[aIndex]->TrackIndex();

    for (TInt a = 0; a < aIndex ; a++) 
        {
            
        if (iClipArray[a]->TrackIndex() == trackIndex) 
            {
            indexOnTrack++;
            }

        }
    return indexOnTrack;
    }

TInt CAudSong::FindClipIndexOnSong(const CAudClip* aClip) const 
    {
    
    for (TInt index = 0 ; index < iClipArray.Count() ; index++) 
        {
        if (iClipArray[index] == aClip) 
            {
            return index;
            }
        }

    // if the clip is not in the array...
    TAudPanic::Panic(TAudPanic::EInternal);
    return 0;

    }

void CAudSong::FireClipAdded(CAudSong* aSong, CAudClip* aClip, TInt aIndex, TInt aTrackIndex) 
    {

    for (TInt i = 0; i < iObserverArray.Count(); i++) 
        {
        iObserverArray[i]->NotifyClipAdded(*aSong, *aClip, aIndex, aTrackIndex);
        }
    }
    
void CAudSong::FireClipAddingFailed(CAudSong* aSong, TInt aError, TInt aTrackIndex) 
    {

    for (TInt i = 0; i < iObserverArray.Count(); i++) 
        {
        iObserverArray[i]->NotifyClipAddingFailed(*aSong, aError, aTrackIndex);
        }
    }

void CAudSong::FireClipRemoved(CAudSong* aSong, TInt aIndex, TInt aTrackIndex) 
    {
    
    for (TInt i = 0; i < iObserverArray.Count(); i++) 
        {
        iObserverArray[i]->NotifyClipRemoved(*aSong, aIndex, aTrackIndex);
        }

    }

void CAudSong::FireClipIndicesChanged(CAudSong* aSong, TInt aOldIndex, 
                                      TInt aNewIndex, TInt aTrackIndex) 
    {
    
    for (TInt i = 0; i < iObserverArray.Count(); i++) 
        {
        iObserverArray[i]->NotifyClipIndicesChanged(*aSong, aOldIndex, aNewIndex, aTrackIndex);
        }
    }

void CAudSong::FireClipTimingsChanged(CAudSong* aSong, CAudClip* aClip)    
    {

    
    for (TInt i = 0; i < iObserverArray.Count(); i++) 
        {
        iObserverArray[i]->NotifyClipTimingsChanged(*aSong, *aClip);
        }

    }

    
void CAudSong::FireDynamicLevelMarkInserted(CAudClip& aClip, 
        TAudDynamicLevelMark& aMark, 
        TInt aIndex) 
    {
    
    for (TInt i = 0; i < iObserverArray.Count(); i++) 
        {
        iObserverArray[i]->NotifyDynamicLevelMarkInserted(aClip, aMark, aIndex);
        }


    }

void CAudSong::FireDynamicLevelMarkRemoved(CAudClip& aClip, TInt aIndex) 
    {

    for (TInt i = 0; i < iObserverArray.Count(); i++) 
        {
        iObserverArray[i]->NotifyDynamicLevelMarkRemoved(aClip, aIndex);
        }

    }

void CAudSong::FireSongReseted(CAudSong& aSong) 
    {

    for (TInt i = 0; i < iObserverArray.Count(); i++) 
        {
        iObserverArray[i]->NotifySongReseted(aSong);
        }
    }

void CAudSong::FireClipReseted(CAudClip& aClip) 
    {

    for (TInt i = 0; i < iObserverArray.Count(); i++) 
        {
        iObserverArray[i]->NotifyClipReseted(aClip);
        }
    }



CAudSongProcessOperation* CAudSongProcessOperation::NewL(CAudSong* aSong)
    {
    CAudSongProcessOperation* self = 
        new (ELeave) CAudSongProcessOperation(aSong);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }


CAudSongProcessOperation::CAudSongProcessOperation(CAudSong* aSong)
: iSong(aSong), iObserver(0), iProcessor(0)
    {

    }


void CAudSongProcessOperation::ConstructL()
    {
    }

CAudSongProcessOperation::~CAudSongProcessOperation()
    {

    if (iProcessor != 0)
        {
        delete iProcessor;
        iProcessor = 0;

        }

    }



void CAudSongProcessOperation::NotifyAudioProcessingStartedL() 
    {
    if (iObserver != 0)
        iObserver->NotifyAudioProcessingStartedL(*iSong);

    }
void CAudSongProcessOperation::NotifyAudioProcessingProgressed(TInt aPercentage) 
    {
    if (iObserver != 0)
        iObserver->NotifyAudioProcessingProgressed(*iSong, aPercentage);

    }
void CAudSongProcessOperation::NotifyAudioProcessingCompleted(TInt aError) 
    {

    delete iProcessor;
    iProcessor = 0;

    MAudSongProcessingObserver* observer = iObserver;
    iObserver = 0;
    if (observer != 0)
        {
        observer->NotifyAudioProcessingProgressed(*iSong, 100);
        observer->NotifyAudioProcessingCompleted(*iSong, aError);
        }
    }

void CAudSongProcessOperation::NotifyTimeEstimateReady(TInt64 aTimeEstimate) 
    {

    delete iProcessor;
    iProcessor = 0;

    MAudTimeEstimateObserver* observer = iTEObserver;
    iTEObserver = 0;
    
    if (observer != 0)
        {
        observer->NotifyTimeEstimateReady(aTimeEstimate);
        }
    }


TBool CAudSongProcessOperation::StartSyncProcL()
    {

    if (iProcessor != 0) 
        {
        User::Leave(KErrNotReady);
        }

    CAudProcessor* processor = CAudProcessor::NewLC();
    TBool ret = processor->StartSyncProcessingL(iSong);
    CleanupStack::Pop(processor);
    iProcessor = processor;

    return ret;

    }

TBool CAudSongProcessOperation::ProcessSyncPieceL(HBufC8*& aFrame, TInt& aProgress,
                                       TTimeIntervalMicroSeconds& aDuration)
    {
    TBool ret = iProcessor->ProcessSyncPieceL(aFrame, aProgress, aDuration);
    if (!ret) return EFalse;
    else
        {
        delete iProcessor;
        iProcessor = 0;
        return ETrue;

        }
    
    }


void CAudSongProcessOperation::Cancel() 
    {

    if (iProcessor == 0) 
        {
        TAudPanic::Panic(TAudPanic::ESongProcessingOperationNotRunning);
        }
    else 
        {
        iProcessor->CancelProcessing(*this);
        }
    }

TBool CAudSongProcessOperation::GetTimeEstimateL(MAudTimeEstimateObserver& aTEObserver)
    {
    
    
    
    if (iProcessor != 0)
        {
        User::Leave(KErrNotReady);
        }
    iTEObserver = &aTEObserver;
    
    CAudProcessor* processor = CAudProcessor::NewLC();
    
    
    TBool ret = processor->StartTimeEstimateL(iSong, *this);
    CleanupStack::Pop(processor);
    iProcessor = processor;

    return ret;
    
    }


CAudSongAddClipOperation* CAudSongAddClipOperation::NewL(CAudSong* aSong)
    {
    CAudSongAddClipOperation* self = 
        new (ELeave) CAudSongAddClipOperation(aSong);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }


CAudSongAddClipOperation::CAudSongAddClipOperation(CAudSong* aSong)
        : iSong(aSong), iClip(0)
    {
    }


void CAudSongAddClipOperation::ConstructL()
    {
    }


CAudSongAddClipOperation::~CAudSongAddClipOperation()
    {
    if (iClip)
        {
        delete iClip;
        iClip = 0;
        }

    }


void CAudSongAddClipOperation::NotifyClipInfoReady(CAudClipInfo& /*aInfo*/, 
                                                         TInt aError)
    {


    iError = aError;
    CompleteAddClipOperation();

    }


void CAudSongAddClipOperation::CompleteAddClipOperation()
    {
    PRINT((_L("CAudSongAddClipOperation::CompleteAddClipOperation in")));


    if (iError != KErrNone)
        {
        TInt trackIndex = iClip->TrackIndex();
        delete iClip;
        iClip = 0;
        iSong->FireClipAddingFailed(iSong, iError, trackIndex);
        PRINT((_L("CAudSong::CompleteAddClipOperation failed, out")));
        return;
        }
    else
        {
    
        TAudFileProperties info = iClip->iInfo->Properties();

        if (iSong->iClipArray.Count() > 0)
            {
            if (!(info.isCompatible(iSong->iClipArray[0]->Info()->Properties()))) 
                {
                TInt trackIndex = iClip->TrackIndex();
        
                delete iClip;
                iClip = 0;
                iSong->FireClipAddingFailed(iSong, KErrNotSupported, trackIndex);
                PRINT((_L("CAudSong::CompleteAddClipOperation failed, out")));
                return;
                }
            }
        
        if (iClip->CutOutTime() == TTimeIntervalMicroSeconds(KClipEndTime))
            {
    
            iClip->iCutOutTime = info.iDuration;
            }

            
        TInt err = KErrNone;

        TBool added = EFalse;

        // insert clips so that they are always sorted by start time
        TInt index = 0;
        for (index = 0 ; index < iSong->iClipArray.Count() ; index++) 
            {
            if (iSong->iClipArray[index]->StartTime() > iClip->StartTime()) 
                {
                err = iSong->iClipArray.Insert(iClip, index);
                added = ETrue;
                break;
                }
            }
        if (!added) 
            {
            index = iSong->iClipArray.Count();
            err = iSong->iClipArray.Insert(iClip, index);
            if (err != KErrNone)
                {
                TInt trackIndex = iClip->TrackIndex();
                delete iClip;
                iClip = 0;
                iSong->FireClipAddingFailed(iSong, KErrGeneral, trackIndex);
                PRINT((_L("CAudSong::CompleteAddClipOperation failed, out")));
                return;
                }
            
            }
        iClip->iIndex = iSong->Index2IndexOnTrack(index);
        
        if (err != KErrNone) 
            {
            TInt trackIndex = iClip->TrackIndex();
        
            delete iClip;
            iClip = 0;
            iSong->FireClipAddingFailed(iSong, err, trackIndex);            
            }
        else
            {
            iSong->UpdateClipIndexes();
            CAudClip* clip = iClip;
            iClip = 0;
            

            if (clip->EndTime() > iSong->iSongDuration)
                {
                iSong->iSongDuration = clip->EndTime();
                }
            

            iSong->FireClipAdded(iSong, clip, clip->iIndex, clip->iTrackIndex);

            
            }
        }
    PRINT((_L("CAudSongAddClipOperation::CompleteAddClipOperation out")));
    }    
    
EXPORT_C void CAudSong::AddClipL(RFile* aFileHandle,
        TTimeIntervalMicroSeconds aStartTime, TInt aTrackIndex,
        TTimeIntervalMicroSeconds aCutInTime,
        TTimeIntervalMicroSeconds aCutOutTime) 
    {

    PRINT((_L("CAudSong::AddClipL in")));
    if (iAddOperation->iClip != 0) 
        {
        TAudPanic::Panic(TAudPanic::ESongAddOperationAlreadyRunning);
        }
    if (iProcessOperation->iProcessor != 0 ) 
        {
        TAudPanic::Panic(TAudPanic::ESongProcessingOperationAlreadyRunning);
        }

    iAddOperation->iClip = CAudClip::NewL(this, aFileHandle, aStartTime, *iAddOperation, aTrackIndex);
    iAddOperation->iClip->iCutInTime = aCutInTime;
    iAddOperation->iClip->iCutOutTime = aCutOutTime;

    PRINT((_L("CAudSong::AddClipL out")));

    }



