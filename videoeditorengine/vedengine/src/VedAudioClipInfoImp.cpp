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



#include "VedAudioClipInfoImp.h"
#include "AudClip.h"
#include "AudClipInfo.h"

#include "movieprocessor.h"

// Print macro
#ifdef _DEBUG
#include <e32svr.h>
#define PRINT(x) RDebug::Print x
#else
#define PRINT(x)
#endif


// -----------------------------------------------------------------------------
// CVedAudioClipInfo::NewL
// Constructs a new CVedAudioClipInfo object
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
EXPORT_C CVedAudioClipInfo* CVedAudioClipInfo::NewL(const TDesC& aFileName,
                                                    MVedAudioClipInfoObserver& aObserver)
    {
    CVedAudioClipInfoImp* self = (CVedAudioClipInfoImp*)NewLC(aFileName, aObserver);
    CleanupStack::Pop(self);
    return self;
    }
    
EXPORT_C CVedAudioClipInfo* CVedAudioClipInfo::NewL(RFile* aFileHandle,
                                                    MVedAudioClipInfoObserver& aObserver)
    {
    CVedAudioClipInfoImp* self = (CVedAudioClipInfoImp*)NewLC(aFileHandle, aObserver);
    CleanupStack::Pop(self);
    return self;
    }

// -----------------------------------------------------------------------------
// CVedAudioClipInfo::NewLC
// Constructs a new CVedAudioClipInfo object, leaves it to cleanupstack
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
EXPORT_C CVedAudioClipInfo* CVedAudioClipInfo::NewLC(const TDesC& aFileName,
                                                     MVedAudioClipInfoObserver& aObserver)
    {
    CVedAudioClipInfoImp* self = new (ELeave) CVedAudioClipInfoImp();
    CleanupStack::PushL(self);
    self->ConstructL(aFileName, aObserver);
    return self;
    }

EXPORT_C CVedAudioClipInfo* CVedAudioClipInfo::NewLC(RFile* aFileHandle,
                                                     MVedAudioClipInfoObserver& aObserver)
    {
    CVedAudioClipInfoImp* self = new (ELeave) CVedAudioClipInfoImp();
    CleanupStack::PushL(self);
    self->ConstructL(aFileHandle, aObserver);
    return self;
    }


CVedAudioClipInfoImp* CVedAudioClipInfoImp::NewL(CAudClip* aAudClip,
                                                 MVedAudioClipInfoObserver& aObserver)
    {
    CVedAudioClipInfoImp* self = (CVedAudioClipInfoImp*)NewLC(aAudClip, aObserver);
    CleanupStack::Pop(self);
    return self;
    }


CVedAudioClipInfoImp* CVedAudioClipInfoImp::NewLC(CAudClip* aAudClip,
                                            MVedAudioClipInfoObserver& aObserver)
    {
    CVedAudioClipInfoImp* self = new (ELeave) CVedAudioClipInfoImp();
    CleanupStack::PushL(self);
    self->ConstructL(aAudClip, aObserver);
    return self;
    }


// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// CVedAudioClipInfo::CVedAudioClipInfoImp
// Constuctor 
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
CVedAudioClipInfoImp::CVedAudioClipInfoImp()
        : iReady(EFalse)
    {
    }

// -----------------------------------------------------------------------------
// CVedAudioClipInfoImp::ConstructL
// Symbian two phased constructor.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
    
void CVedAudioClipInfoImp::ConstructL(CAudClip* aAudClip, 
                                      MVedAudioClipInfoObserver& aObserver)
    {
    PRINT(_L("CVedAudioClipInfoImp::ConstructL in"));
    iAudClip = aAudClip;
    iAudClipInfo = iAudClip->Info();
    iAudioProperties = iAudClipInfo->Properties();
    iReady = ETrue;
    iObserver = &aObserver;
    iOperation = CVedAudioClipInfoOperation::NewL(this, aObserver);
    PRINT(_L("CVedAudioClipInfoImp::ConstructL out"));
    }
    
void CVedAudioClipInfoImp::ConstructL(const TDesC& aFileName, 
                                     MVedAudioClipInfoObserver& aObserver)
    {
    
    PRINT(_L("CVedAudioClipInfoImp::ConstructL in"));
    iReady = EFalse;
    iAudClipInfo = CAudClipInfo::NewL(aFileName, *this);
    
    iObserver = &aObserver;
    PRINT(_L("CVedAudioClipInfoImp::ConstructL out"));
    }
    
void CVedAudioClipInfoImp::ConstructL(RFile* aFileHandle, 
                                     MVedAudioClipInfoObserver& aObserver)
    {
    
    PRINT(_L("CVedAudioClipInfoImp::ConstructL in"));
    iReady = EFalse;
    iAudClipInfo = CAudClipInfo::NewL(aFileHandle, *this);
    
    iObserver = &aObserver;
    PRINT(_L("CVedAudioClipInfoImp::ConstructL out"));
    }


// -----------------------------------------------------------------------------
// CVedAudioClipInfoImp::~CVedAudioClipInfoImp
// Destroys the object and releases all resources.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
CVedAudioClipInfoImp::~CVedAudioClipInfoImp()
    {
    
    if (iAudClipInfo != 0 && !iAudClip) 
    {
    delete iAudClipInfo;
    }
    
    delete iOperation;    
    REComSession::FinalClose();
    }


void CVedAudioClipInfoImp::NotifyClipInfoReady(CAudClipInfo& aInfo, TInt aError)
    {
    if (aError == KErrNone)
        {
        iAudioProperties = aInfo.Properties();    
        iReady = ETrue;
        }

    iObserver->NotifyAudioClipInfoReady(*this, aError);
    }
    

// -----------------------------------------------------------------------------
// CVedAudioClipInfoImp::ChannelMode
// Returns the channel mode of the audio if applicable.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TVedAudioChannelMode CVedAudioClipInfoImp::ChannelMode() const
    {
    
    __ASSERT_ALWAYS(iReady, TVedPanic::Panic(TVedPanic::EAudioClipInfoNotReady));
    
    TVedAudioChannelMode vedChannelMode = EVedAudioChannelModeUnrecognized;
    if (iAudioProperties.iChannelMode == EAudStereo)
        {
        vedChannelMode = EVedAudioChannelModeStereo;
        }
    else if (iAudioProperties.iChannelMode == EAudSingleChannel)
        {
        vedChannelMode = EVedAudioChannelModeSingleChannel;
        }
    else if (iAudioProperties.iChannelMode == EAudDualChannel)
        {
        vedChannelMode = EVedAudioChannelModeDualChannel;
        }

    return vedChannelMode;   
    }

// -----------------------------------------------------------------------------
// CVedAudioClipInfoImp::SamplingRate
// Returns the sampling rate in hertz.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CVedAudioClipInfoImp::SamplingRate() const
    {
    
    __ASSERT_ALWAYS(iReady, TVedPanic::Panic(TVedPanic::EAudioClipInfoNotReady));
    
    return iAudioProperties.iSamplingRate;

    }
  
// -----------------------------------------------------------------------------
// CVedAudioClipInfoImp::Format
// Returns the audio format
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//  
TVedAudioFormat CVedAudioClipInfoImp::Format() const 
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EAudioClipInfoNotReady));

    TVedAudioFormat vedAudioFormat = EVedAudioFormatUnrecognized;

    switch(iAudioProperties.iFileFormat) 
        {
    case EAudFormat3GPP:
        vedAudioFormat = EVedAudioFormat3GPP;
        break;
    case EAudFormatMP4:
        vedAudioFormat = EVedAudioFormatMP4;
        break;
    case EAudFormatAMR:
        vedAudioFormat = EVedAudioFormatAMR;
        break;
    case EAudFormatAMRWB:
        vedAudioFormat = EVedAudioFormatAMRWB;
        break;
    case EAudFormatMP3:
        vedAudioFormat = EVedAudioFormatMP3;
        break;
    case EAudFormatAAC_ADIF:
        vedAudioFormat = EVedAudioFormatAAC_ADIF;
        break;
    case EAudFormatAAC_ADTS:
        vedAudioFormat = EVedAudioFormatAAC_ADTS;
        break;
    case EAudFormatWAV:
        vedAudioFormat = EVedAudioFormatWAV;
        break;
    default:
        TVedPanic::Panic(TVedPanic::EInternal);
        }
    
    return vedAudioFormat;
    }

// -----------------------------------------------------------------------------
// CVedAudioClipInfoImp::BitrateMode
// Returns the bitrate mode
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//  
TVedBitrateMode CVedAudioClipInfoImp::BitrateMode() const
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EAudioClipInfoNotReady));
    
    TVedBitrateMode vedBitrateMode = EVedBitrateModeUnrecognized;
    switch(iAudioProperties.iBitrateMode) 
        {
    case EAudConstant:
        vedBitrateMode = EVedBitrateModeConstant;
        break;
    case EAudVariable:
        vedBitrateMode = EVedBitrateModeVariable;
        break;
    default:
        TVedPanic::Panic(TVedPanic::EInternal);
        }
    return vedBitrateMode;
    }

// -----------------------------------------------------------------------------
// CVedAudioClipInfoImp::Bitrate
// Returns the bitrate
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//  
TInt CVedAudioClipInfoImp::Bitrate() const
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EAudioClipInfoNotReady));
    return iAudioProperties.iBitrate;
    }

// -----------------------------------------------------------------------------
// CVedAudioClipInfoImp::GetVisualizationL
// Generates a visualization of the audio clip
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//  
void CVedAudioClipInfoImp::GetVisualizationL(MVedAudioClipVisualizationObserver& aObserver, TInt aResolution, TInt aPriority)
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EAudioClipInfoNotReady));

    iVisualizationObserver = &aObserver;
    iAudClipInfo->GetVisualizationL(*this, aResolution, aPriority);
    }

// -----------------------------------------------------------------------------
// CVedAudioClipInfoImp::CancelVisualizationL
// Cancels visualization generation
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//  
void CVedAudioClipInfoImp::CancelVisualizationL()
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EAudioClipInfoNotReady));
    
    iAudClipInfo->CancelVisualization();
    }
    
// -----------------------------------------------------------------------------
// CVedAudioClipInfoImp::NotifyClipInfoVisualizationCompleted
// Callback for visualization complete
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//  
void CVedAudioClipInfoImp::NotifyClipInfoVisualizationCompleted(const CAudClipInfo& /*aClipInfo*/, TInt aError, TInt8* aVisualization, TInt aSize)
    {
    iVisualizationObserver->NotifyAudioClipVisualizationCompleted(*this, aError, aVisualization, aSize);
    iVisualizationObserver = 0;
    }
    
// -----------------------------------------------------------------------------
// CVedAudioClipInfoImp::NotifyClipInfoVisualizationStarted
// Callback for visualization started 
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//  
void CVedAudioClipInfoImp::NotifyClipInfoVisualizationStarted(const CAudClipInfo& /*aClipInfo*/, TInt aError)
    {
    if (aError != KErrNone) 
        {
        iVisualizationObserver->NotifyAudioClipVisualizationCompleted(*this, aError, NULL, 0);
        iVisualizationObserver = 0;
        }
    else
        {
        iVisualizationObserver->NotifyAudioClipVisualizationStarted(*this);
        }
    }

// -----------------------------------------------------------------------------
// CVedAudioClipInfoImp::NotifyClipInfoVisualizationProgressed
// Callback for visualization progress
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//  
void CVedAudioClipInfoImp::NotifyClipInfoVisualizationProgressed(const CAudClipInfo& /*aClipInfo*/, TInt aPercentage)
    {
    iVisualizationObserver->NotifyAudioClipVisualizationProgressed(*this, aPercentage);
    }

// -----------------------------------------------------------------------------
// CVedAudioClipInfoImp::
// Returns the audio clip filename
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//  
TPtrC CVedAudioClipInfoImp::FileName() const
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EAudioClipInfoNotReady));

    return iAudClipInfo->FileName();

    }
    
RFile* CVedAudioClipInfoImp::FileHandle() const
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EAudioClipInfoNotReady));

    return iAudClipInfo->FileHandle();

    }

// -----------------------------------------------------------------------------
// CVedAudioClipInfoImp::Type
// Returns the audio type
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//  
TVedAudioType CVedAudioClipInfoImp::Type() const
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EAudioClipInfoNotReady));

    TVedAudioType vedAudioType = EVedAudioTypeUnrecognized;

    if (iAudioProperties.iAudioType == EAudAMR)
        {
        vedAudioType = EVedAudioTypeAMR;
        }
    else if (iAudioProperties.iAudioType == EAudAMRWB)
        {
        vedAudioType = EVedAudioTypeAMRWB;
        }
    else if (iAudioProperties.iAudioType == EAudMP3)
        {
        vedAudioType = EVedAudioTypeMP3;
        }
    else if (iAudioProperties.iAudioType == EAudAAC_MPEG2 || iAudioProperties.iAudioType == EAudAAC_MPEG4 )
        {
        vedAudioType = EVedAudioTypeAAC_LC;
        }
    else if (iAudioProperties.iAudioType == EAudWAV)
        {
        vedAudioType = EVedAudioTypeWAV;
        }

    return vedAudioType;
    }

// -----------------------------------------------------------------------------
// CVedAudioClipInfoImp::Duration
// Returns the audio clip duration
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//  
TTimeIntervalMicroSeconds CVedAudioClipInfoImp::Duration() const
    {
    __ASSERT_ALWAYS(iReady, 
                    TVedPanic::Panic(TVedPanic::EAudioClipInfoNotReady));

    return iAudioProperties.iDuration;
    }


// -----------------------------------------------------------------------------
// CVedAudioClipInfoImp::Compare
// Compares two audio clip info classes
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//  
TInt CVedAudioClipInfoImp::Compare(const CVedAudioClipInfoImp& c1, 
                                   const CVedAudioClipInfoImp& c2) 
    {


    if (c1.iAudClip == 0 || c2.iAudClip == 0)
        {
        return 0;
        }
    if (c1.iAudClip->StartTime() > c2.iAudClip->StartTime()) 
        {
        return 1;
        }
    else if (c1.iAudClip->StartTime() < c2.iAudClip->StartTime()) 
        {
        return -1;
        }
    else 
        {
        return 0;
        }
    }

//////////////////////////////////////////////////////////////////////////


// CVedAudioClipInfoOperation
// 

// -----------------------------------------------------------------------------
// CVedAudioClipInfoOperation::NewL
// Constructs a new CVedAudioClipInfoOperation object.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
CVedAudioClipInfoOperation* CVedAudioClipInfoOperation::NewL(CVedAudioClipInfoImp* aInfo,
                                                             MVedAudioClipInfoObserver& aObserver)
    {
    CVedAudioClipInfoOperation* self = 
        new (ELeave) CVedAudioClipInfoOperation(aInfo, aObserver);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }

// -----------------------------------------------------------------------------
// CVedAudioClipInfoOperation::CVedAudioClipInfoOperation
// Constructor.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
CVedAudioClipInfoOperation::CVedAudioClipInfoOperation(CVedAudioClipInfoImp* aInfo,
                                                       MVedAudioClipInfoObserver& aObserver)
        : CActive(EPriorityStandard), iObserver(aObserver), iInfo(aInfo)
          
    {
    CActiveScheduler::Add(this);
    }

// -----------------------------------------------------------------------------
// CVedAudioClipInfoOperation::ConstructL
// Symbian two phased constructor
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CVedAudioClipInfoOperation::ConstructL()
    {
    PRINT(_L("CVedAudioClipInfoOperation::ConstructL in"));
    
    SetActive();
    TRequestStatus* status = &iStatus;
    User::RequestComplete(status, KErrNone);

    PRINT(_L("CVedAudioClipInfoOperation::ConstructL out"));
    }

// -----------------------------------------------------------------------------
// CVedAudioClipInfoOperation::~CVedAudioClipInfoOperation
// Destructor
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
CVedAudioClipInfoOperation::~CVedAudioClipInfoOperation()
    {
    Cancel();
    }

// -----------------------------------------------------------------------------
// CVedAudioClipInfoOperation::RunError
// Handle errors from RunL()
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CVedAudioClipInfoOperation::RunError( TInt aError )
    {
    iObserver.NotifyAudioClipInfoReady(*iInfo, aError);
    return KErrNone;
    }

// -----------------------------------------------------------------------------
// CVedAudioClipInfoOperation::RunL
// Active objects RunL function ran when request completes.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CVedAudioClipInfoOperation::RunL()
    {
    PRINT(_L("CVedAudioClipInfoOperation::RunL in"));
    
    iObserver.NotifyAudioClipInfoReady(*iInfo, KErrNone);

    PRINT(_L("CVedAudioClipInfoOperation::RunL out"));
    }

// -----------------------------------------------------------------------------
// CVedAudioClipInfoOperation::DoCancel
// Cancel ongoing request.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CVedAudioClipInfoOperation::DoCancel()
    {
    iObserver.NotifyAudioClipInfoReady(*iInfo, KErrCancel);
    }

