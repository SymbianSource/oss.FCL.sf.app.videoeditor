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





#include "ProcTimeEstimateAO.h"


void CProcTimeEstimateAO::RunL() 
    {
    
    HBufC8* frame = 0;
    TInt progress = 0;
    TTimeIntervalMicroSeconds duration;
    
    
    TBool ret = EFalse; 
    TRAPD(err, ret = iProcessingObject->ProcessSyncPieceL(frame, progress, duration));
    
    if (err != KErrNone)
        {
        // something went wrong
        
        delete frame;
        
        delete iProcessingObject;
        iProcessingObject = 0;
        
        // notify
        iTEObserver->NotifyTimeEstimateReady(0);
        
        }
    
    if (!ret) 
        {
        
        if (frame)
            {
            
            // frame is not needed
            delete frame;
            frame = 0;
            
            }
        
        SetActive();
        TRequestStatus* status = &iStatus;
        User::RequestComplete(status, KErrNone);
        
        return;
        }
    else
        {
        
        TInt64 timeEstimate = iProcessingObject->GetFinalTimeEstimate();
        
        
        delete iProcessingObject;
        iProcessingObject = 0;
        
        // notify
        iTEObserver->NotifyTimeEstimateReady(timeEstimate);
        
        return;
        
        }


    }

void CProcTimeEstimateAO::DoCancel() 
    {

    }
   
CProcTimeEstimateAO* CProcTimeEstimateAO::NewL() 
    {


    CProcTimeEstimateAO* self = new (ELeave) CProcTimeEstimateAO();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }

CProcTimeEstimateAO::~CProcTimeEstimateAO() 
    {
    Cancel();   
    delete iProcessingObject;    

    }
    

TBool CProcTimeEstimateAO::StartL(const CAudSong* aSong, MAudTimeEstimateObserver& aTEObserver) 
    {
    
    iTEObserver = &aTEObserver;
    
    iSong = aSong;

    delete iProcessingObject;
    iProcessingObject = NULL; /* Must set to null before reallocating with NewL */
    iProcessingObject = CProcProcess::NewL();
    
    iProcessingObject->StartSyncProcessingL(iSong, ETrue);
     
    SetActive();
    TRequestStatus* status = &iStatus;
    User::RequestComplete(status, KErrNone);
    
    return ETrue;

    }
    
void CProcTimeEstimateAO::CancelTimeEstimate()
    {

    Cancel();   
    delete iProcessingObject;
    iProcessingObject = 0;
    // notify
    return;
    
    
    }

void CProcTimeEstimateAO::ConstructL() 
    {

    }

CProcTimeEstimateAO::CProcTimeEstimateAO() :  CActive(0)
    {

    CActiveScheduler::Add(this);

    }

