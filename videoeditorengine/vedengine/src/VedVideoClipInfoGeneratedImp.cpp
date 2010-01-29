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




#include "VedVideoClipInfoGeneratedImp.h"
#include "VedMovieImp.h"
#include "VedVideoClip.h"
#include "VedVideoClipGenerator.h"

#include <fbs.h>

// Print macro
#ifdef _DEBUG
#include <e32svr.h>
#define PRINT(x) RDebug::Print x
#else
#define PRINT(x)
#endif


CVedVideoClipInfoGeneratedImp::CVedVideoClipInfoGeneratedImp(CVedVideoClipGenerator& aGenerator, TBool aOwnsGenerator)
        : iGenerator(&aGenerator), iOwnsGenerator(aOwnsGenerator)
    {
    }


void CVedVideoClipInfoGeneratedImp::ConstructL(MVedVideoClipInfoObserver& aObserver)
    {
    PRINT(_L("CVedVideoClipInfoGeneratedImp::ConstructL in"));

    iInfoOperation = CVedVideoClipInfoGeneratedOperation::NewL(this, aObserver);
    iAdapter = new (ELeave) CVedVideoClipGeneratedFrameToFrameAdapter(*this);

    PRINT(_L("CVedVideoClipInfoGeneratedImp::ConstructL out"));
    }


CVedVideoClipInfoGeneratedImp::~CVedVideoClipInfoGeneratedImp()
    {
    PRINT(_L("CVedVideoClipInfoGeneratedImp::~CVedVideoClipInfoGeneratedImp in"));

    if ( iGenerator )
        {
        if ( (TInt)iOwnsGenerator == (TInt)ETrue )
            {
            delete iGenerator;
            }
            
        iGenerator = 0;
        }

    delete iInfoOperation;
    delete iAdapter;

    PRINT(_L("CVedVideoClipInfoGeneratedImp::~CVedVideoClipInfoGeneratedImp out"));
    }

TPtrC CVedVideoClipInfoGeneratedImp::DescriptiveName() const 
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));
    return iGenerator->DescriptiveName();
    }

CVedVideoClipGenerator* CVedVideoClipInfoGeneratedImp::Generator() const
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));
    return iGenerator;
    }


TVedVideoClipClass CVedVideoClipInfoGeneratedImp::Class() const 
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));
    return EVedVideoClipClassGenerated;
    }


TInt CVedVideoClipInfoGeneratedImp::GetVideoFrameIndexL(TTimeIntervalMicroSeconds aTime)
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));
    return iGenerator->GetVideoFrameIndex(aTime);
    }


void CVedVideoClipInfoGeneratedImp::GetFrameL(MVedVideoClipFrameObserver& aObserver, 
                                     TInt aIndex,
                                     TSize* const aResolution,
                                     TDisplayMode aDisplayMode,
                                     TBool aEnhance,
                                     TInt aPriority)
    {
    PRINT(_L("CVedVideoClipInfoGeneratedImp::GetFrameL in"));
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));
    iAdapter->iFrameObserver = &aObserver;
    iGenerator->GetFrameL(*iAdapter, aIndex, aResolution, aDisplayMode, aEnhance, aPriority);
    PRINT(_L("CVedVideoClipInfoGeneratedImp::GetFrameL out"));
    }


void CVedVideoClipInfoGeneratedImp::CancelFrame()
    {
    PRINT(_L("CVedVideoClipInfoGeneratedImp::CancelFrame in"));

    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));
    iGenerator->CancelFrame();
    PRINT(_L("CVedVideoClipInfoGeneratedImp::CancelFrame out"));
    }

TPtrC CVedVideoClipInfoGeneratedImp::FileName() const
    {
    // Always panic
    TVedPanic::Panic(TVedPanic::EVideoClipInfoNoFileAssociated);
    
    // This will never be reached.
    return NULL;
    }
    
    
RFile* CVedVideoClipInfoGeneratedImp::FileHandle() const
    {
    // Always panic
    TVedPanic::Panic(TVedPanic::EVideoClipInfoNoFileAssociated);
    
    // This will never be reached.
    return NULL;
    }


TVedVideoFormat CVedVideoClipInfoGeneratedImp::Format() const
    {
    // Always panic
    TVedPanic::Panic(TVedPanic::EVideoClipInfoNoFileAssociated);
    
    // This will never be reached.
    return EVedVideoFormatUnrecognized;
    }


TVedVideoType CVedVideoClipInfoGeneratedImp::VideoType() const
    {
    // Always panic
    TVedPanic::Panic(TVedPanic::EVideoClipInfoNoFileAssociated);
    
    // This will never be reached.
    return EVedVideoTypeUnrecognized;
    }


TSize CVedVideoClipInfoGeneratedImp::Resolution() const
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));

    return iGenerator->Movie()->Resolution();
    }


TBool CVedVideoClipInfoGeneratedImp::HasAudio() const
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));

    return EFalse;  
    }


TVedAudioType CVedVideoClipInfoGeneratedImp::AudioType() const
    {
    // Always panic
    TVedPanic::Panic(TVedPanic::EVideoClipInfoNoFileAssociated);
    
    // This will never be reached.
    return EVedAudioTypeUnrecognized;
    }

TVedAudioChannelMode CVedVideoClipInfoGeneratedImp::AudioChannelMode() const
    {
    // Always panic
    TVedPanic::Panic(TVedPanic::EVideoClipInfoNoFileAssociated);
    
    // This will never be reached.
    return EVedAudioChannelModeUnrecognized;
    }

TInt CVedVideoClipInfoGeneratedImp::AudioSamplingRate() const
    {
    // Always panic
    TVedPanic::Panic(TVedPanic::EVideoClipInfoNoFileAssociated);
    
    // This will never be reached.
    return -1;
    }



TTimeIntervalMicroSeconds CVedVideoClipInfoGeneratedImp::Duration() const
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));

    return iGenerator->Duration();
    }


TInt CVedVideoClipInfoGeneratedImp::VideoFrameCount() const
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));

    return iGenerator->VideoFrameCount();
    }


TTimeIntervalMicroSeconds CVedVideoClipInfoGeneratedImp::VideoFrameStartTimeL(TInt aIndex)
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));

    return iGenerator->VideoFrameStartTime(aIndex);
    }


TTimeIntervalMicroSeconds CVedVideoClipInfoGeneratedImp::VideoFrameEndTimeL(TInt aIndex)
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));
    return iGenerator->VideoFrameEndTime(aIndex);
    }


TTimeIntervalMicroSeconds CVedVideoClipInfoGeneratedImp::VideoFrameDurationL(TInt aIndex)
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));
    return iGenerator->VideoFrameDuration(aIndex);
    }


TInt CVedVideoClipInfoGeneratedImp::VideoFrameSizeL(TInt /*aIndex*/) 
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));

    // always panic
    TVedPanic::Panic(TVedPanic::EVideoClipInfoNoFileAssociated);

    // will never be reached
    return 0;
    }


TBool CVedVideoClipInfoGeneratedImp::VideoFrameIsIntraL(TInt aIndex) 
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EVideoClipInfoNotReady));

    if (aIndex == 0) 
        {
        return ETrue;
        }
    return EFalse;
    }


void CVedVideoClipInfoGeneratedImp::SetTranscodeFactor(TVedTranscodeFactor aFactor)
    {
    iTFactor.iStreamType =aFactor.iStreamType;
    iTFactor.iTRes = aFactor.iTRes;
    }

TVedTranscodeFactor CVedVideoClipInfoGeneratedImp::TranscodeFactor()
    {
    return iTFactor;
    }


TBool CVedVideoClipInfoGeneratedImp::IsMMSCompatible()
    {
    // Always panic
    TVedPanic::Panic(TVedPanic::EVideoClipInfoNoFileAssociated);
    
    // This will never be reached.
    return ETrue;;
    }


CVedVideoClipInfoGeneratedOperation* CVedVideoClipInfoGeneratedOperation::NewL(CVedVideoClipInfoGeneratedImp* aInfo,
                                                                               MVedVideoClipInfoObserver& aObserver)
    {
    PRINT(_L("CVedVideoClipInfoGeneratedOperation::NewL in"));

    CVedVideoClipInfoGeneratedOperation* self = 
        new (ELeave) CVedVideoClipInfoGeneratedOperation(aInfo, aObserver);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);

    PRINT(_L("CVedVideoClipInfoGeneratedOperation::NewL out"));
    return self;
    }


CVedVideoClipInfoGeneratedOperation::CVedVideoClipInfoGeneratedOperation(CVedVideoClipInfoGeneratedImp* aInfo,
                                                                         MVedVideoClipInfoObserver& aObserver)
        : CActive(EPriorityStandard), iInfo(aInfo)
    {
    PRINT(_L("CVedVideoClipInfoGeneratedOperation::CVedVideoClipInfoGeneratedOperation in"));

    iObserver = &aObserver;
    CActiveScheduler::Add(this);

    PRINT(_L("CVedVideoClipInfoGeneratedOperation::CVedVideoClipInfoGeneratedOperation out"));
    }


void CVedVideoClipInfoGeneratedOperation::ConstructL()
    {
    PRINT(_L("CVedVideoClipInfoGeneratedOperation::ConstructL in"));

    SetActive();
    TRequestStatus* status = &iStatus;
    User::RequestComplete(status, KErrNone);

    PRINT(_L("CVedVideoClipInfoGeneratedOperation::ConstructL out"));
    }


CVedVideoClipInfoGeneratedOperation::~CVedVideoClipInfoGeneratedOperation()
    {
    Cancel();
    }


void CVedVideoClipInfoGeneratedOperation::RunL()
    {
    PRINT(_L("CVedVideoClipInfoGeneratedOperation::RunL in"));

    iInfo->iReady = ETrue;

    iObserver->NotifyVideoClipInfoReady(*iInfo, KErrNone);

    PRINT(_L("CVedVideoClipInfoGeneratedOperation::RunL out"));
    }


void CVedVideoClipInfoGeneratedOperation::DoCancel()
    {
    }



CVedVideoClipGeneratedFrameToFrameAdapter::CVedVideoClipGeneratedFrameToFrameAdapter(CVedVideoClipInfo& aInfo)
: iInfo(aInfo)
    {
    }

void CVedVideoClipGeneratedFrameToFrameAdapter::NotifyVideoClipGeneratorFrameCompleted(
                        CVedVideoClipGenerator& /*aInfo*/, TInt aError, CFbsBitmap* aFrame)
    {
    iFrameObserver->NotifyVideoClipFrameCompleted(iInfo, aError, aFrame);
    }
