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



#include "ProcVisualizationAO.h"
#include "ProcVisProcessor.h"
#include "AudPanic.h"


void CProcVisualizationAO::RunL()
    {
    __ASSERT_DEBUG(iProcVisProcessor != 0, TAudPanic::Panic(TAudPanic::EInternal));
    __ASSERT_DEBUG(iObserver != 0, TAudPanic::Panic(TAudPanic::EInternal));
    
    if(iVisualizationState == EProcVisualizationIdle) 
        {
        TAudPanic::Panic(TAudPanic::EInternal);
        }
    
    else if (iVisualizationState == EProcGettingClipVisualization) 
        {
        
        TBool completed = EFalse;
        TInt progress = 0;
        TRAPD(err, completed = iProcVisProcessor->VisualizeClipPieceL(progress));
        
        if (err != KErrNone)
            {
            delete iProcVisProcessor;
            iProcVisProcessor = 0;
            
            MAudVisualizationObserver* observer = iObserver;
            iObserver = 0;
            iSize = 0;
            iVisualizationState = EProcVisualizationIdle;
            observer->NotifyClipInfoVisualizationCompleted(*iClipInfo, err, NULL, 0);
            }
        else if (completed)
            {
            TInt8* visualization = 0;
            TInt size = 0;
            TRAPD(err2, iProcVisProcessor->GetFinalVisualizationL(visualization, size));
            
            if (err2 != KErrNone)
                {
                MAudVisualizationObserver* observer = iObserver;
                iObserver = 0;
            
                if (visualization != 0) delete[] visualization;
                delete iProcVisProcessor;
                iProcVisProcessor = 0;
                iVisualizationState = EProcVisualizationIdle;
                
                observer->NotifyClipInfoVisualizationCompleted(*iClipInfo, err2, 0, 0);
                return;
                }
            
            delete iProcVisProcessor;
            iProcVisProcessor = 0;
            
            MAudVisualizationObserver* observer = iObserver;
            iObserver = 0;
            iSize = 0;
            iVisualizationState = EProcVisualizationIdle;

            // NOTE: the upper level application is responsible for releasing "visualization"
            observer->NotifyClipInfoVisualizationCompleted(*iClipInfo, KErrNone, visualization, size);
            }
        else
            {

            // progress is sent every 1 percent
            if (iPreviousProgressValue < progress)
                {
                iObserver->NotifyClipInfoVisualizationProgressed(*iClipInfo, progress);
                iPreviousProgressValue = progress;
                }

            SetActive();
            TRequestStatus* status = &iStatus;
            User::RequestComplete(status, KErrNone);
            }
        
        }
    }

void CProcVisualizationAO::DoCancel() 
    {

    // ref to observer should be reset before callback to avoid recursion
    MAudVisualizationObserver* observer = iObserver;
    iObserver = 0;
    // ref to clipInfo is good to reset before callback
    const CAudClipInfo* clipInfo = iClipInfo;
    iClipInfo = 0;
    // processor not needed and should not exist any more; a new processor will be created for next visualization
    delete iProcVisProcessor;
    iProcVisProcessor = 0;
    
    if ((observer != 0) && (iVisualizationState == EProcGettingClipVisualization))
        {
        // reset state
        iVisualizationState = EProcVisualizationIdle;

        // note! this callback at least at the time of writing deletes this object => no operations should be done after this call
        observer->NotifyClipInfoVisualizationCompleted(*clipInfo, KErrCancel, 0, 0);
        }
    }
   
CProcVisualizationAO* CProcVisualizationAO::NewL() 
    {
    CProcVisualizationAO* self = new (ELeave) CProcVisualizationAO();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }

CProcVisualizationAO::~CProcVisualizationAO()
    {
    // cancel the active object; it also deletes or resets member variables
    Cancel();
    }
    

void CProcVisualizationAO::ConstructL() 
    {
    // nothing to do; iProcVisProcessor will be created when started
    }

CProcVisualizationAO::CProcVisualizationAO() : 
        CActive(0),
        iVisualizationState(EProcVisualizationIdle) 
        {

    CActiveScheduler::Add(this);

    }

void CProcVisualizationAO::StartClipVisualizationL(const CAudClipInfo* aClipInfo, TInt aSize,
                                                   MAudVisualizationObserver& aObserver, 
                                                   TInt aPriority) 
    {

    if (iVisualizationState != EProcVisualizationIdle || iClipInfo != 0) 
        {
        TAudPanic::Panic(TAudPanic::EVisualizationProcessAlreadyRunning);
        }

    iSize = aSize;
    iClipInfo = aClipInfo;
    iProcVisProcessor = CProcVisProcessor::NewL();
    
    iPreviousProgressValue = 0;


    TRAPD(err, iProcVisProcessor->VisualizeClipL(aClipInfo, aSize));

    if (err != KErrNone)
        {
        delete iProcVisProcessor;
        iProcVisProcessor = 0;
        aObserver.NotifyClipInfoVisualizationCompleted(*aClipInfo, err, 0, 0);
        User::Leave(err);
        }

    aObserver.NotifyClipInfoVisualizationStarted(*aClipInfo, KErrNone);

    iObserver = &aObserver;

    SetPriority(aPriority);
    SetActive();
    TRequestStatus* status = &iStatus;
    User::RequestComplete(status, KErrNone);

    iVisualizationState = EProcGettingClipVisualization;

    }




void CProcVisualizationAO::CancelVisualization() 
    {
    Cancel();
    }
    
