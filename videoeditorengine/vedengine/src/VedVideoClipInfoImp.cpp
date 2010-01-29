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




#include "VedVideoClipInfoImp.h"
#include "movieprocessor.h"
#include <fbs.h>
#include <ecom/ecom.h>
#include "audclipinfo.h"
#include "audcommon.h"
#include "VedVideoClip.h"

// Print macro
#ifdef _DEBUG
#include <e32svr.h>
#define PRINT(x) RDebug::Print x
#else
#define PRINT(x)
#endif


EXPORT_C CVedVideoClipInfo* CVedVideoClipInfo::NewL(const TDesC& aFileName,
                                                    MVedVideoClipInfoObserver& aObserver)
    {
    PRINT(_L("CVedVideoClipInfo::NewL"));

    CVedVideoClipInfoImp* self = (CVedVideoClipInfoImp*)NewLC(aFileName, aObserver);
    self->iVideoClipIsStandalone = ETrue;
    CleanupStack::Pop(self);
    PRINT(_L("CVedVideoClipInfo::NewL out"));
    return self;
    }
    
EXPORT_C CVedVideoClipInfo* CVedVideoClipInfo::NewLC(const TDesC& aFileName,
                                                     MVedVideoClipInfoObserver& aObserver)
    {
    PRINT(_L("CVedVideoClipInfo::NewLC in"));

    CVedVideoClipInfoImp* self = new (ELeave) CVedVideoClipInfoImp(NULL);
    CleanupStack::PushL(self);
    self->ConstructL(aFileName, aObserver);

    PRINT(_L("CVedVideoClipInfo::NewLC out"));
    return self;
    }

EXPORT_C CVedVideoClipInfo* CVedVideoClipInfo::NewL(RFile* aFileHandle,
                                                    MVedVideoClipInfoObserver& aObserver)
    {
    PRINT(_L("CVedVideoClipInfo::NewL"));

    CVedVideoClipInfoImp* self = (CVedVideoClipInfoImp*)NewLC(aFileHandle, aObserver);
    self->iVideoClipIsStandalone = ETrue;
    CleanupStack::Pop(self);
    PRINT(_L("CVedVideoClipInfo::NewL out"));
    return self;
    }
    
EXPORT_C CVedVideoClipInfo* CVedVideoClipInfo::NewLC(RFile* aFileHandle,
                                                     MVedVideoClipInfoObserver& aObserver)
    {
    PRINT(_L("CVedVideoClipInfo::NewLC in"));

    CVedVideoClipInfoImp* self = new (ELeave) CVedVideoClipInfoImp(NULL);
    CleanupStack::PushL(self);
    self->ConstructL(aFileHandle, aObserver);

    PRINT(_L("CVedVideoClipInfo::NewLC out"));
    return self;
    }


CVedVideoClipInfo* CVedVideoClipInfoImp::NewL(CAudClipInfo* aAudClipInfo, 
                                              const TDesC& aFileName, MVedVideoClipInfoObserver& aObserver)
    {
    CVedVideoClipInfoImp* self = new (ELeave) CVedVideoClipInfoImp(aAudClipInfo);
    CleanupStack::PushL(self);
    if (aAudClipInfo) 
        {
        self->ConstructL(aAudClipInfo->FileName(), aObserver);
        }
    else
        {
        self->ConstructL(aFileName, aObserver);
        }
    self->iVideoClipIsStandalone = EFalse;
    CleanupStack::Pop(self);
    return self;
    }
    
CVedVideoClipInfo* CVedVideoClipInfoImp::NewL(CAudClipInfo* aAudClipInfo, 
                                              RFile* aFileHandle, MVedVideoClipInfoObserver& aObserver)
    {
    CVedVideoClipInfoImp* self = new (ELeave) CVedVideoClipInfoImp(aAudClipInfo);
    CleanupStack::PushL(self);
    if (aAudClipInfo) 
        {
        self->ConstructL(aAudClipInfo->FileHandle(), aObserver);
        }
    else
        {
        self->ConstructL(aFileHandle, aObserver);
        }
    self->iVideoClipIsStandalone = EFalse;
    CleanupStack::Pop(self);
    return self;
    }
    

CVedVideoClipInfoImp::CVedVideoClipInfoImp(CAudClipInfo* aAudClipInfo)
        : iReady(EFalse), iAudClipInfoOwnedByVideoClipInfo(EFalse), iAudClipInfo(aAudClipInfo)
    {
    }


void CVedVideoClipInfoImp::ConstructL(const TDesC& aFileName,
                                   MVedVideoClipInfoObserver& aObserver)
    {
    PRINT(_L("CVedVideoClipInfo::ConstructL in"));

    iFileName = HBufC::NewL(aFileName.Length());
    *iFileName = aFileName;

    iFrameOperation = CVedVideoClipFrameOperation::NewL(this);

    iInfoOperation = CVedVideoClipInfoOperation::NewL(this, aObserver);
    
    PRINT(_L("CVedVideoClipInfo::ConstructL out"));
    }
    
void CVedVideoClipInfoImp::ConstructL(RFile* aFileHandle,
                                   MVedVideoClipInfoObserver& aObserver)
    {
    PRINT(_L("CVedVideoClipInfo::ConstructL in"));

    iFileName = HBufC::NewL(1);
    iFileHandle = aFileHandle;

    iFrameOperation = CVedVideoClipFrameOperation::NewL(this);

    iInfoOperation = CVedVideoClipInfoOperation::NewL(this, aObserver);
    
    PRINT(_L("CVedVideoClipInfo::ConstructL out"));
    }
    

CVedVideoClipInfoImp::~CVedVideoClipInfoImp()
    {
    PRINT(_L("CVedVideoClipInfo::~CVedVideoClipInfoImp in"));

    delete iInfoOperation;
    delete iFrameOperation;

    User::Free(iVideoFrameInfoArray);
    delete iFileName;

    REComSession::FinalClose();

    if (iAudClipInfoOwnedByVideoClipInfo) 
        {
        delete iAudClipInfo;
        }

    PRINT(_L("CVedVideoClipInfo::~CVedVideoClipInfoImp out"));
    }


TPtrC CVedVideoClipInfoImp::DescriptiveName() const 
    {
    return *iFileName;
    }

CVedVideoClipGenerator* CVedVideoClipInfoImp::Generator() const
    {
    // Always panic
    TVedPanic::Panic(TVedPanic::EVideoClipInfoNoGeneratorAssociated);
    
    // This will never be reached.
    return NULL;
    }


TVedVideoClipClass CVedVideoClipInfoImp::Class() const 
    {
    return EVedVideoClipClassFile;
    }


TInt CVedVideoClipInfoImp::GenerateVideoFrameInfoArrayL() 
    {
    PRINT(_L("CVedVideoClipInfoImp::GenerateVideoFrameInfoArrayL in"));

    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));

    TInt error = KErrNone;

    if ( !iVideoFrameInfoArrayReady )
        {
        CMovieProcessor* processor = CMovieProcessor::NewL();
        CleanupStack::PushL(processor);
        TRAP( error, processor->GenerateVideoFrameInfoArrayL(*iFileName, iFileHandle, iVideoFrameInfoArray));
        CleanupStack::PopAndDestroy(processor);

        if ( (error == KErrNone) && iVideoFrameInfoArray )
            {
            iVideoFrameInfoArrayReady = ETrue;            
            }
        else
            {
            PRINT((_L("CVedVideoClipInfoImp::GenerateVideoFrameInfoArrayL Error=%d"), error));
            }
        }

    PRINT(_L("CVedVideoClipInfoImp::GenerateVideoFrameInfoArrayL out"));
    return error;
    }


TInt CVedVideoClipInfoImp::GetVideoFrameIndexL(TTimeIntervalMicroSeconds aTime)
    {
    PRINT(_L("CVedVideoClipInfoImp::GetVideoFrameIndex in"));

    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));
    __ASSERT_ALWAYS((aTime >= TTimeIntervalMicroSeconds(0)) && (aTime <= Duration()), 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoIllegalVideoFrameTime));

    /* Use binary search to find the right frame. */

    TInt start = 0;
    TInt end = iVideoFrameCount - 1;
    TInt index = -1;
    TBool always = ETrue;

    while( always )
        {
        index = start + ((end - start) / 2);

        TTimeIntervalMicroSeconds startTime = VideoFrameStartTimeL(index);
        TTimeIntervalMicroSeconds endTime = VideoFrameEndTimeL(index);
        if (index < (VideoFrameCount() - 1))
            {
            endTime = TTimeIntervalMicroSeconds(endTime.Int64() - 1);
            }

        if (aTime < startTime)
            {
            end = index - 1;
            }
        else if (aTime > endTime)
            {
            start = index + 1;
            }
        else
            {
            always = EFalse;
            }
        }

    PRINT(_L("CVedVideoClipInfoImp::GetVideoFrameIndex out"));
    return index;
    }


void CVedVideoClipInfoImp::GetFrameL(MVedVideoClipFrameObserver& aObserver, 
                                     TInt aIndex,
                                     TSize* const aResolution,
                                     TDisplayMode aDisplayMode,
                                     TBool aEnhance,
                                     TInt aPriority)
    {
    PRINT(_L("CVedVideoClipInfoImp::GetFrameL in"));

    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));

    iFrameOperation->StartL(aObserver, aIndex, aResolution, aDisplayMode, aEnhance, aPriority);
    
    PRINT(_L("CVedVideoClipInfoImp::GetFrameL out"));
    }


void CVedVideoClipInfoImp::CancelFrame()
    {
    PRINT(_L("CVedVideoClipInfoImp::CancelFrame in"));

    if ( !iReady )
        {
        PRINT(_L("CVedVideoClipInfoImp::CancelFrame not even info ready yet, cancel it and get out"));
        iInfoOperation->Cancel();
        return;
        }

    iFrameOperation->Cancel();

    PRINT(_L("CVedVideoClipInfoImp::CancelFrame out"));
    }




TPtrC CVedVideoClipInfoImp::FileName() const
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));

    return *iFileName;
    }
    
RFile* CVedVideoClipInfoImp::FileHandle() const
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));

    return iFileHandle;
    }


TVedVideoFormat CVedVideoClipInfoImp::Format() const
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));

    return iFormat;
    }


TVedVideoType CVedVideoClipInfoImp::VideoType() const
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));

    return iVideoType;
    }


TSize CVedVideoClipInfoImp::Resolution() const
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));

    return iResolution;
    }


TBool CVedVideoClipInfoImp::HasAudio() const
    {
    
    if (!iReady)  
        {
        if (iVideoClipIsStandalone)
            return EFalse;
        }

    if (iAudClipInfo == 0) 
        {
        return EFalse;
        }
    return ((iAudClipInfo->Properties().iAudioType != EAudTypeUnrecognized)
        && (iAudClipInfo->Properties().iAudioType != EAudNoAudio));
    }


TVedAudioType CVedVideoClipInfoImp::AudioType() const
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));

    if (iAudClipInfo == 0) 
        {
        return EVedAudioTypeNoAudio;
        }

    TVedAudioType vedAudioType = EVedAudioTypeUnrecognized;
    if (iAudClipInfo->Properties().iAudioType == EAudAMR)
        {
        vedAudioType = EVedAudioTypeAMR;
        }
    else if (iAudClipInfo->Properties().iAudioType == EAudAMRWB)
        {
        vedAudioType = EVedAudioTypeAMRWB;
        }
    else if (iAudClipInfo->Properties().iAudioType == EAudMP3)
        {
        vedAudioType = EVedAudioTypeMP3;
        }
    else if (iAudClipInfo->Properties().iAudioType == EAudAAC_MPEG4 )
        {
        vedAudioType = EVedAudioTypeAAC_LC;
        }
    else if (iAudClipInfo->Properties().iAudioType == EAudWAV)
        {
        vedAudioType = EVedAudioTypeWAV;
        }
    else if (iAudClipInfo->Properties().iAudioType == EAudNoAudio)
    	{
    	vedAudioType = EVedAudioTypeNoAudio;
    	}
        
    return vedAudioType;
    }

TVedAudioChannelMode CVedVideoClipInfoImp::AudioChannelMode() const
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));

    if (iAudClipInfo == 0) 
        {
        return EVedAudioChannelModeUnrecognized;
        }

    TVedAudioChannelMode vedChannelMode = EVedAudioChannelModeUnrecognized;
    if (iAudClipInfo->Properties().iChannelMode == EAudStereo)
        {
        vedChannelMode = EVedAudioChannelModeStereo;
        }
    else if (iAudClipInfo->Properties().iChannelMode == EAudSingleChannel)
        {
        vedChannelMode = EVedAudioChannelModeSingleChannel;
        }
    else if (iAudClipInfo->Properties().iChannelMode == EAudDualChannel)
        {
        vedChannelMode = EVedAudioChannelModeDualChannel;
        }

    return vedChannelMode;
    }

TInt CVedVideoClipInfoImp::AudioSamplingRate() const
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));

    if (iAudClipInfo == 0) 
        {
        return 0;
        }

    return iAudClipInfo->Properties().iSamplingRate;
    }


TTimeIntervalMicroSeconds CVedVideoClipInfoImp::Duration() const
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));

    return iDuration;
    }


TInt CVedVideoClipInfoImp::VideoFrameCount() const
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));

    return iVideoFrameCount;
    }


TTimeIntervalMicroSeconds CVedVideoClipInfoImp::VideoFrameStartTimeL(TInt aIndex)
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));
    __ASSERT_ALWAYS(((aIndex >= 0) && (aIndex < iVideoFrameCount)),
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoIllegalVideoFrameIndex));

    if ( !iVideoFrameInfoArrayReady )
        {
        User::LeaveIfError(GenerateVideoFrameInfoArrayL());
        }

    TInt64 startTime = TInt64(iVideoFrameInfoArray[aIndex].iStartTime) * TInt64(1000);
    return TTimeIntervalMicroSeconds(startTime);
    }


TTimeIntervalMicroSeconds CVedVideoClipInfoImp::VideoFrameEndTimeL(TInt aIndex)
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));
    __ASSERT_ALWAYS(((aIndex >= 0) && (aIndex < iVideoFrameCount)),
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoIllegalVideoFrameIndex));

    if ( !iVideoFrameInfoArrayReady )
        {
        User::LeaveIfError(GenerateVideoFrameInfoArrayL());
        }

    if (aIndex < (iVideoFrameCount - 1))
        {
        return (TInt64(iVideoFrameInfoArray[aIndex + 1].iStartTime) * TInt64(1000));
        }
    else
        {
        return iDuration;
        }
    }


TTimeIntervalMicroSeconds CVedVideoClipInfoImp::VideoFrameDurationL(TInt aIndex)
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));
    __ASSERT_ALWAYS(((aIndex >= 0) && (aIndex < iVideoFrameCount)),
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoIllegalVideoFrameIndex));

    if ( !iVideoFrameInfoArrayReady )
        {
        User::LeaveIfError(GenerateVideoFrameInfoArrayL());
        }


    TInt64 duration = - (TInt64(iVideoFrameInfoArray[aIndex].iStartTime) * TInt64(1000));
    if (aIndex < (iVideoFrameCount - 1))
        {
        duration += TInt64(iVideoFrameInfoArray[aIndex + 1].iStartTime) * TInt64(1000);
        }
    else
        {
        duration += iDuration.Int64();
        }

    return TTimeIntervalMicroSeconds(duration);
    }


TInt CVedVideoClipInfoImp::VideoFrameSizeL(TInt aIndex)
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));
    __ASSERT_ALWAYS(((aIndex >= 0) && (aIndex < iVideoFrameCount)),
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoIllegalVideoFrameIndex));

    if ( !iVideoFrameInfoArrayReady )
        {
        User::LeaveIfError(GenerateVideoFrameInfoArrayL());
        }


    return iVideoFrameInfoArray[aIndex].iSize;
    }


TBool CVedVideoClipInfoImp::VideoFrameIsIntraL(TInt aIndex)
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));
    __ASSERT_ALWAYS(((aIndex >= 0) && (aIndex < iVideoFrameCount)),
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoIllegalVideoFrameIndex));

    if ( !iVideoFrameInfoArrayReady )
        {
        User::LeaveIfError(GenerateVideoFrameInfoArrayL());
        }

    if (iVideoFrameInfoArray[aIndex].iFlags & KVedVideoFrameInfoFlagIntra)
        {
        return ETrue;
        }
    else
        {
        return EFalse;
        }
    }

void CVedVideoClipInfoImp::SetTranscodeFactor(TVedTranscodeFactor aFactor)
    {
    iTimeFactor.iStreamType = aFactor.iStreamType;
    iTimeFactor.iTRes = aFactor.iTRes;
    }

TVedTranscodeFactor CVedVideoClipInfoImp::TranscodeFactor()
    {
    return iTimeFactor;
    }

TBool CVedVideoClipInfoImp::IsMMSCompatible()
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));

    TAudType audioType;
    if (iAudClipInfo == 0) 
        {
        audioType = EAudNoAudio;
        }
    else
        {
        audioType = iAudClipInfo->Properties().iAudioType;
        }

    return ( ( iFormat == EVedVideoFormat3GPP ) &&
             ( iVideoType == EVedVideoTypeH263Profile0Level10 ) &&
             ( audioType == EAudAMR ) );
    }


CVedVideoClipInfoOperation* CVedVideoClipInfoOperation::NewL(CVedVideoClipInfoImp* aInfo,
                                                             MVedVideoClipInfoObserver& aObserver)
    {
    PRINT(_L("CVedVideoClipInfoOperation::NewL in"));

    CVedVideoClipInfoOperation* self = 
        new (ELeave) CVedVideoClipInfoOperation(aInfo, aObserver);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);

    PRINT(_L("CVedVideoClipInfoOperation::NewL out"));
    return self;
    }


CVedVideoClipInfoOperation::CVedVideoClipInfoOperation(CVedVideoClipInfoImp* aInfo,
                                                       MVedVideoClipInfoObserver& aObserver)
        : CActive(EPriorityStandard), iInfo(aInfo), iMovieProcessorError(KErrNone)
    {
    PRINT(_L("CVedVideoClipInfoOperation::CVedVideoClipInfoOperation in"));

    iObserver = &aObserver;
    CActiveScheduler::Add(this);

    PRINT(_L("CVedVideoClipInfoOperation::CVedVideoClipInfoOperation out"));
    }


void CVedVideoClipInfoOperation::ConstructL()
    {
    PRINT(_L("CVedVideoClipInfoOperation::ConstructL in"));

    iGettingAudio = EFalse;

    SetActive();
    TRequestStatus* status = &iStatus;
    User::RequestComplete(status, KErrNone);

    PRINT(_L("CVedVideoClipInfoOperation::ConstructL out"));
    }


CVedVideoClipInfoOperation::~CVedVideoClipInfoOperation()
    {
    Cancel();
    }


void CVedVideoClipInfoOperation::RunL()
    {
    PRINT(_L("CVedVideoClipInfoOperation::RunL in"));    
 
    if (iGettingAudio) 
        {
        if (iMovieProcessorError == KErrNone || iMovieProcessorError == KErrNoAudio) 
            {
            iInfo->iReady = ETrue;
            iMovieProcessorError = KErrNone;
            }
        iObserver->NotifyVideoClipInfoReady(*iInfo, iMovieProcessorError);
        }
    else
        {
        CMovieProcessor* processor = CMovieProcessor::NewL();
        TVedAudioType audioType = EVedAudioTypeUnrecognized;
        TVedAudioChannelMode audioChannelMode = EVedAudioChannelModeUnrecognized;
        TInt audioSamplingRate;
        TRAP(iMovieProcessorError,
             processor->GetVideoClipPropertiesL(*iInfo->iFileName, 
                                                iInfo->iFileHandle,
                                                iInfo->iFormat,
                                                iInfo->iVideoType,
                                                iInfo->iResolution,
                                                audioType,
                                                iInfo->iDuration,
                                                iInfo->iVideoFrameCount,
                                                audioSamplingRate,
                                                audioChannelMode));    
        delete processor;
        processor = 0;                
        
        if (iMovieProcessorError != KErrNone) 
            {
            iObserver->NotifyVideoClipInfoReady(*iInfo, iMovieProcessorError);
            }
        else if (iInfo->iAudClipInfo || !iInfo->iVideoClipIsStandalone)
            {
            if (iMovieProcessorError == KErrNone) 
                {
                iInfo->iReady = ETrue;
                }
            iObserver->NotifyVideoClipInfoReady(*iInfo, iMovieProcessorError);
            }
        else
            {
            
            if (iInfo->iFileHandle)
                iInfo->iAudClipInfo = CAudClipInfo::NewL(iInfo->iFileHandle, *this);
            else
                iInfo->iAudClipInfo = CAudClipInfo::NewL(*iInfo->iFileName, *this);
            
            iInfo->iAudClipInfoOwnedByVideoClipInfo = ETrue;
            iGettingAudio = ETrue;
            }
        }
    PRINT(_L("CVedVideoClipInfoOperation::RunL out"));
    }

void CVedVideoClipInfoOperation::NotifyClipInfoReady(CAudClipInfo& /*aInfo*/, TInt aError)
    {
    SetActive();
    TRequestStatus* status = &iStatus;
    User::RequestComplete(status, KErrNone);
    
    iMovieProcessorError = aError;
    }

void CVedVideoClipInfoOperation::DoCancel()
    {
    }


CVedVideoClipFrameOperation* CVedVideoClipFrameOperation::NewL(CVedVideoClipInfoImp* aInfo)
    {
    PRINT(_L("CVedVideoClipFrameOperation::NewL in"));

    CVedVideoClipFrameOperation* self = 
        new (ELeave) CVedVideoClipFrameOperation(aInfo);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);

    PRINT(_L("CVedVideoClipFrameOperation::NewL out"));
    return self;
    }


CVedVideoClipFrameOperation::CVedVideoClipFrameOperation(CVedVideoClipInfoImp* aInfo)
        : CActive(EPriorityIdle), iInfo(aInfo)
    {
    PRINT(_L("CVedVideoClipFrameOperation::CVedVideoClipFrameOperation in"));
    CActiveScheduler::Add(this);
    PRINT(_L("CVedVideoClipFrameOperation::CVedVideoClipFrameOperation out"));
    }


void CVedVideoClipFrameOperation::ConstructL()
    {
    }


CVedVideoClipFrameOperation::~CVedVideoClipFrameOperation()
    {
    Cancel();
    }


void CVedVideoClipFrameOperation::StartL(MVedVideoClipFrameObserver& aObserver,
                                         TInt aIndex,
                                         TSize* const aResolution, 
                                         TDisplayMode aDisplayMode,
                                         TBool aEnhance,
                                         TInt aPriority)
    {
    PRINT(_L("CVedVideoClipFrameOperation::StartL in"));

    __ASSERT_ALWAYS(!IsActive(), 
        TVedPanic::Panic(TVedPanic::EVideoClipInfoFrameOperationAlreadyRunning));
    __ASSERT_DEBUG(iProcessor == 0, TVedPanic::Panic(TVedPanic::EInternal));

    TSize resolution;
    if (aResolution != 0)
        {
        __ASSERT_ALWAYS(aResolution->iWidth > 0, 
            TVedPanic::Panic(TVedPanic::EVideoClipInfoIllegalFrameResolution));
        __ASSERT_ALWAYS(aResolution->iHeight > 0, 
            TVedPanic::Panic(TVedPanic::EVideoClipInfoIllegalFrameResolution));

        resolution = *aResolution;
        }
    else
        {
        resolution = iInfo->Resolution();
        }

    __ASSERT_ALWAYS(((aIndex >= 0) && (iInfo->VideoFrameCount() == 0) || 
                                      (aIndex < iInfo->VideoFrameCount()) || 
                                      (aIndex == KFrameIndexBestThumb) ), 
            TVedPanic::Panic(TVedPanic::EVideoClipInfoIllegalVideoFrameIndex));

    iIndex = aIndex;
    
    iFactor.iStreamType = EVedVideoBitstreamModeUnknown;
    iFactor.iTRes = 0;

    CMovieProcessor* processor = CMovieProcessor::NewLC();  

    processor->StartThumbL(iInfo->FileName(), iInfo->FileHandle(), aIndex, resolution, aDisplayMode,
                           aEnhance);

    /* Initialization OK. This method cannot leave any more. Start processing. */

    CleanupStack::Pop(processor);

    iProcessor = processor;
    iObserver = &aObserver;

    SetPriority(aPriority);
    SetActive();
    TRequestStatus* status = &iStatus;
    User::RequestComplete(status, KErrNone);

    PRINT(_L("CVedVideoClipFrameOperation::StartL out"));
    }


void CVedVideoClipFrameOperation::RunL()
    {
    PRINT(_L("CVedVideoClipFrameOperation::RunL in"));

    __ASSERT_DEBUG(iProcessor != 0, TVedPanic::Panic(TVedPanic::EInternal));
    __ASSERT_DEBUG(iObserver != 0, TVedPanic::Panic(TVedPanic::EInternal));
    
    TInt err = KErrNone;
    
    if (!iThumbRequestPending)
        TRAP(err, iProcessor->ProcessThumbL(iStatus, &iFactor));
    
    if (err != KErrNone)
        {
        delete iProcessor;
        iProcessor = 0;
        
        MVedVideoClipFrameObserver* observer = iObserver;
        iObserver = 0;
        observer->NotifyVideoClipFrameCompleted(*iInfo, err, NULL);
        return;
        }
        
    if (!iThumbRequestPending)
    {        
        iInfo->SetTranscodeFactor(iFactor);
        iThumbRequestPending = ETrue;
        iStatus = KRequestPending;
        SetActive();        
        return;
    }
    else
    {
        MVedVideoClipFrameObserver* observer = iObserver;
        iObserver = 0;
                
        if (iStatus == KErrNone)
        {
            CFbsBitmap* frame = 0;
            iProcessor->FetchThumb(frame);
            observer->NotifyVideoClipFrameCompleted(*iInfo, KErrNone, frame);            
        }
        else
        {
            observer->NotifyVideoClipFrameCompleted(*iInfo, iStatus.Int(), NULL);            
        }
        iThumbRequestPending = EFalse;
        delete iProcessor;
        iProcessor = 0;
    }      
        
    PRINT(_L("CVedVideoClipFrameOperation::RunL out"));
    }


void CVedVideoClipFrameOperation::DoCancel()
    {
    if (iProcessor != 0)
        {
        delete iProcessor;
        iProcessor = 0;
        
        // Cancel our internal request
        if ( iStatus == KRequestPending )
        {
            PRINT((_L("CVedVideoClipFrameOperation::DoCancel() cancel request")));
            TRequestStatus *status = &iStatus;
            User::RequestComplete(status, KErrCancel);
        }
        iThumbRequestPending = EFalse;

        MVedVideoClipFrameObserver* observer = iObserver;
        iObserver = 0;
        observer->NotifyVideoClipFrameCompleted(*iInfo, KErrCancel, NULL);
        }
    }

