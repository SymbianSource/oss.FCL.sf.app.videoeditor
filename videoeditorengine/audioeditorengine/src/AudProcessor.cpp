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
#include "AudProcessor.h"
#include "AudPanic.h"

// Debug print macro
#if defined _DEBUG 
#include <e32svr.h>
#define PRINT(x) RDebug::Print x;
#else
#define PRINT(x)
#endif


CAudProcessor* CAudProcessor::NewL() 
    {
    CAudProcessor* self = NewLC();
    CleanupStack::Pop(self);
    return self;
    }


CAudProcessor* CAudProcessor::NewLC()
    {
    CAudProcessor* self = new (ELeave) CAudProcessor;
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
    }


CAudProcessor::CAudProcessor() : iProcessing(EFalse)
    {
    
    }


void CAudProcessor::ConstructL()
    {
        iClipInfoAO = CProcClipInfoAO::NewL();
        iProcessingObject = CProcProcess::NewL();
        iVisualizationAO = CProcVisualizationAO::NewL();
        iTimeEstimateAO = CProcTimeEstimateAO::NewL();
        
       
        
        
    }


CAudProcessor::~CAudProcessor()
    {
    PRINT((_L("CAudProcessor::~CAudProcessor() in") ));


        
        if (iVisualizationAO != 0)
            {
            delete iVisualizationAO;
            iVisualizationAO = 0;
            }
    PRINT((_L("CAudProcessor::~CAudProcessor() deleted iVisualizationAO") ));

        if (iProcessingObject != 0)
            {
            delete iProcessingObject;
            iProcessingObject = 0;
            }

    PRINT((_L("CAudProcessor::~CAudProcessor() deleted iProcessingObject") ));
        if (iClipInfoAO != 0)
            {
            delete iClipInfoAO;
            iClipInfoAO = 0;
            }
            
    PRINT((_L("CAudProcessor::~CAudProcessor() deleted iClipInfoAO") ));
        if (iTimeEstimateAO != 0)
            {
            delete iTimeEstimateAO;
            iTimeEstimateAO = 0;
            }


    PRINT((_L("CAudProcessor::~CAudProcessor() out") ));

    }



TBool CAudProcessor::StartSyncProcessingL(const CAudSong* aSong)
    {

    if (iProcessing) 
        {
        TAudPanic::Panic(TAudPanic::ESongProcessingOperationAlreadyRunning);
        }


    return iProcessingObject->StartSyncProcessingL(aSong);

    }

TBool CAudProcessor::ProcessSyncPieceL(HBufC8*& aFrame, TInt& aProgress,
                                       TTimeIntervalMicroSeconds& aDuration)
    {
    TBool ret = iProcessingObject->ProcessSyncPieceL(aFrame, aProgress, aDuration);
    
    if (!ret) return EFalse;
    else
        {
        delete iProcessingObject;
        iProcessingObject = 0;
        return ETrue;
        }

    }

    
/**
 * Cancel song processing.
 */
void CAudProcessor::CancelProcessing(MProcProcessObserver& aObserver) 
    {

    iProcessing = EFalse;
    aObserver.NotifyAudioProcessingCompleted(KErrCancel);
    }


void CAudProcessor::GetAudFilePropertiesL(const TDesC& aFileName, 
                                          RFile* aFileHandle,
                                          TAudFileProperties* aProperties, 
                                          MProcClipInfoObserver& aObserver,
                                          TInt aPriority) 
    {

    iClipInfoAO->StartL(aFileName, aFileHandle, aObserver, aProperties, aPriority);
    }



void CAudProcessor::StartGetClipVisualizationL(const CAudClipInfo* aClipInfo, TInt aSize,
        MAudVisualizationObserver& aObserver, 
        TInt aPriority) 
    {

    
    iVisualizationAO->StartClipVisualizationL(aClipInfo, aSize, aObserver, aPriority);


    }


    
void CAudProcessor::CancelClipVisualization() 
    {

    if (iVisualizationAO)
        {
        iVisualizationAO->CancelVisualization();
        }

    }
    
    
TBool CAudProcessor::StartTimeEstimateL(const CAudSong* aSong, MAudTimeEstimateObserver& aTEObserver) 
    {

    if (iTimeEstimateAO)
        {
        return iTimeEstimateAO->StartL(aSong, aTEObserver);
        }
    else
        {
        return EFalse;
        }

    }

void CAudProcessor::CancelTimeEstimate() 
    {

    if (iTimeEstimateAO)
        {
        iTimeEstimateAO->CancelTimeEstimate();
        }

    }



