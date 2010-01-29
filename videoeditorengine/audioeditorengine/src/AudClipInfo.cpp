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


/* Copyright (c) 2003, Nokia. All rights reserved */

#include "AudClipInfo.h"
#include "AudProcessor.h"
#include "AudPanic.h"



#include <fbs.h>
#include <e32base.h>

// Debug print macro
#if defined _DEBUG 
#include <e32svr.h>
#define PRINT(x) RDebug::Print x;
#else
#define PRINT(x)
#endif


EXPORT_C CAudClipInfo* CAudClipInfo::NewL(const TDesC& aFileName, 
                                          MAudClipInfoObserver& aObserver)
    {
    CAudClipInfo* self = NewLC(aFileName, aObserver);
    CleanupStack::Pop(self);
    return self;
    }

    
EXPORT_C CAudClipInfo* CAudClipInfo::NewLC(const TDesC& aFileName, 
                                           MAudClipInfoObserver& aObserver)
    {
    CAudClipInfo* self = new (ELeave) CAudClipInfo();
    CleanupStack::PushL(self);
    self->ConstructL(aFileName, aObserver);
    return self;
    }
    
EXPORT_C CAudClipInfo* CAudClipInfo::NewL(RFile* aFileHandle,
                                          MAudClipInfoObserver& aObserver)
    {
    CAudClipInfo* self = NewLC(aFileHandle, aObserver);
    CleanupStack::Pop(self);
    return self;
    }

    
EXPORT_C CAudClipInfo* CAudClipInfo::NewLC(RFile* aFileHandle,
                                           MAudClipInfoObserver& aObserver)
    {
    CAudClipInfo* self = new (ELeave) CAudClipInfo();
    CleanupStack::PushL(self);
    self->ConstructL(aFileHandle, aObserver);
    return self;
    }

EXPORT_C TAudFileProperties CAudClipInfo::Properties() const
    {

    return *iProperties;
    }

EXPORT_C TPtrC CAudClipInfo::FileName() const 
    {
    return *iFileName;
    }
    
EXPORT_C RFile* CAudClipInfo::FileHandle() const 
    {
    return iFileHandle;
    }

EXPORT_C void CAudClipInfo::GetVisualizationL(MAudVisualizationObserver& aObserver,
                                              TInt aSize, TInt aPriority) const 
    {


    iOperation->StartVisualizationL(aObserver, aSize, aPriority);
    }

EXPORT_C void CAudClipInfo::CancelVisualization()
    {


    iOperation->CancelVisualization();
    }

CAudClipInfo::CAudClipInfo() : iInfoReady(EFalse)

    {
    }


void CAudClipInfo::ConstructL(const TDesC& aFileName, MAudClipInfoObserver& aObserver)
    {

    iProperties = new (ELeave) TAudFileProperties();

    iFileName = HBufC::NewL(aFileName.Length());
    *iFileName = aFileName;
    iFileHandle = NULL;
    
    iOperation = CAudClipInfoOperation::NewL(this, aObserver);
    iOperation->StartGetPropertiesL();

    }
    
void CAudClipInfo::ConstructL(RFile* aFileHandle, MAudClipInfoObserver& aObserver)
    {

    iProperties = new (ELeave) TAudFileProperties();
        
    iFileHandle = aFileHandle;
    
    iFileName = HBufC::NewL(1);
    
    iOperation = CAudClipInfoOperation::NewL(this, aObserver);
    iOperation->StartGetPropertiesL();

    }


EXPORT_C CAudClipInfo::~CAudClipInfo() 
    {
    
	PRINT((_L("CAudClipInfo::~CAudClipInfo() in")));
    if (iProperties != 0)
        {
        delete iProperties;
        iProperties = 0;
        }
	PRINT((_L("CAudClipInfo::~CAudClipInfo() deleted iProperties")));
    if (iOperation)
        {
        delete iOperation;
        iOperation = 0;
        }

	PRINT((_L("CAudClipInfo::~CAudClipInfo() deleted iOperation")));
    if (iFileName != 0)
        {
        delete iFileName;
        iFileName = 0;
        }
	PRINT((_L("CAudClipInfo::~CAudClipInfo() out")));
    
    }




CAudClipInfoOperation* CAudClipInfoOperation::NewL(CAudClipInfo* aInfo,
                                                             MAudClipInfoObserver& aObserver)
    {
    CAudClipInfoOperation* self = 
        new (ELeave) CAudClipInfoOperation(aInfo, aObserver);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }


CAudClipInfoOperation::CAudClipInfoOperation(CAudClipInfo* aInfo,
                                             MAudClipInfoObserver& aObserver) : 
                                             iInfo(aInfo), iObserver(&aObserver)
                                    
    {

    }


void CAudClipInfoOperation::ConstructL() 
    {

   
    }


CAudClipInfoOperation::~CAudClipInfoOperation() 
    {

	PRINT((_L("CAudClipInfoOperation::~CAudClipInfoOperation() in")));
    if (iProcessor != 0)
        {
        delete iProcessor;
        iProcessor = 0;

        }
	PRINT((_L("CAudClipInfoOperation::~CAudClipInfoOperation() out")));

    }


void CAudClipInfoOperation::NotifyClipInfoReady(TInt aError) 
    {
    
	PRINT((_L("CAudClipInfoOperation::NotifyClipInfoReady() in")));
    
    iInfo->iInfoReady = ETrue;

    if (iProcessor == 0) 
        {
        TAudPanic::Panic(TAudPanic::EInternal);
        }

    if (iProcessor != 0)
        {
        delete iProcessor;
        iProcessor = 0;
        }

    iObserver->NotifyClipInfoReady(*iInfo, aError);

	PRINT((_L("CAudClipInfoOperation::NotifyClipInfoReady() out")));

    }

void CAudClipInfoOperation::StartGetPropertiesL() 
    {
	PRINT((_L("CAudClipInfoOperation::StartGetPropertiesL() in")));

    if (iProcessor != 0) 
        {
        TAudPanic::Panic(TAudPanic::EInternal);
        }
    iProcessor = CAudProcessor::NewL();        
    iProcessor->GetAudFilePropertiesL(*iInfo->iFileName, iInfo->iFileHandle, 
                                      iInfo->iProperties, *this, CActive::EPriorityStandard);

	PRINT((_L("CAudClipInfoOperation::StartGetPropertiesL() out")));
    }

void CAudClipInfoOperation::StartVisualizationL(MAudVisualizationObserver& aObserver, TInt aSize, 
                                            TInt aPriority) 
    {

    if (iProcessor != 0) 
        {
        TAudPanic::Panic(TAudPanic::EClipInfoProcessAlreadyRunning);
        }
    iVisualizationObserver = &aObserver;

    CAudProcessor* processor = CAudProcessor::NewLC();
    
    processor->StartGetClipVisualizationL(iInfo, aSize, *this, aPriority);

    
    CleanupStack::Pop(processor);
    iProcessor = processor;

    }

void CAudClipInfoOperation::CancelVisualization()
    {
    PRINT((_L("CAudClipInfoOperation::CancelVisualization() in")));

    if (iProcessor == 0) 
        {
        PRINT((_L("CAudClipInfoOperation::CancelVisualization() no visualization going on, ignore")));
        return;
        }
    else 
        {
        PRINT((_L("CAudClipInfoOperation::CancelVisualization() cancel iProcessor")));
        iProcessor->CancelClipVisualization();
        
        delete iProcessor;
        iProcessor = 0;
            
        iVisualizationObserver = 0;
        PRINT((_L("CAudClipInfoOperation::CancelVisualization() out")));
        }
    }

void CAudClipInfoOperation::NotifySongVisualizationCompleted(const CAudSong& /*aSong*/, 
        TInt /*aError*/, 
        TInt8* /*aVisualization*/,
        TInt /*aSize*/) 
    {

    // should not be called
    TAudPanic::Panic(TAudPanic::EInternal);


    }

void CAudClipInfoOperation::NotifySongVisualizationStarted(const CAudSong& /*aSong*/, 
                                                                    TInt /*aError*/) 
    {
    // should not be called
    TAudPanic::Panic(TAudPanic::EInternal);

    }


void CAudClipInfoOperation::NotifySongVisualizationProgressed(const CAudSong& /*aSong*/, 
                                                                       TInt /*aPercentage*/) 
    {

    // should not be called
    TAudPanic::Panic(TAudPanic::EInternal);
    

    }

void CAudClipInfoOperation::NotifyClipInfoVisualizationCompleted(const CAudClipInfo& aClipInfo, 
        TInt aError, 
        TInt8* aVisualization,
        TInt aSize) 
    {
    PRINT((_L("CAudClipInfoOperation::NotifyClipInfoVisualizationCompleted() in")));
    if (aError != KErrNone)
        {
        iVisualizationObserver->NotifyClipInfoVisualizationCompleted(aClipInfo, aError, aVisualization, aSize);
        PRINT((_L("CAudClipInfoOperation::NotifyClipInfoVisualizationCompleted() completed with error %d"),aError));
        }
    else
        {
        if (iProcessor != 0)
            {
            delete iProcessor;
            iProcessor = 0;
            }

        MAudVisualizationObserver* observer = iVisualizationObserver;
        iVisualizationObserver = 0;
        observer->NotifyClipInfoVisualizationProgressed(aClipInfo, 100);
        observer->NotifyClipInfoVisualizationCompleted(aClipInfo, aError, aVisualization, aSize);
        PRINT((_L("CAudClipInfoOperation::NotifyClipInfoVisualizationCompleted() completed")));
        }
    
    
    PRINT((_L("CAudClipInfoOperation::NotifyClipInfoVisualizationCompleted() out")));
    }


void CAudClipInfoOperation::NotifyClipInfoVisualizationStarted(const CAudClipInfo& aClipInfo, 
                                        TInt aError) 
    {

    
    iVisualizationObserver->NotifyClipInfoVisualizationStarted(aClipInfo, aError);

    }

void CAudClipInfoOperation::NotifyClipInfoVisualizationProgressed(const CAudClipInfo& aClipInfo, 
                                                                           TInt aPercentage) 
    {

    
    iVisualizationObserver->NotifyClipInfoVisualizationProgressed(aClipInfo, aPercentage);
    }
