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





#include "ProcClipInfoAO.h"

// Debug print macro
#if defined _DEBUG 
#include <e32svr.h>
#define PRINT(x) RDebug::Print x;
#else
#define PRINT(x)
#endif

void CProcClipInfoAO::RunL() 
    {
    PRINT((_L("CProcClipInfoAO::RunL in") ));
    

    TRAPD(error, iProcessorImpl->GetAudFilePropertiesL(*iFileName, iFileHandle, iProperties));

    if (error != KErrNone) 
        {
        
        delete iFileName;
        iFileName = 0;
        delete iProcessorImpl;
        iProcessorImpl = 0;
        iObserver->NotifyClipInfoReady(error);
        iProperties = 0;
        iObserver = 0;
        PRINT((_L("CProcClipInfoAO::RunL out with error %d"), error ));
        return;
        }
    else 
        {
        
        
        delete iFileName;
        iFileName = 0;
        delete iProcessorImpl;
        iProcessorImpl = 0;
        
        if (iProperties->iAudioType == EAudNoAudio)
            {
            MProcClipInfoObserver* observer = iObserver;
            
            iObserver = 0;
            observer->NotifyClipInfoReady(KErrNoAudio); 
            PRINT((_L("CProcClipInfoAO::RunL no audio in the clip") ));

            }
        else if (iProperties->iAudioType == EAudTypeUnrecognized ||
            iProperties->iBitrate == 0 ||
            iProperties->iBitrateMode == EAudBitrateModeNotRecognized || 
            iProperties->iFileFormat == EAudFormatUnrecognized ||
            iProperties->iChannelMode == EAudChannelModeNotRecognized ||
            iProperties->iSamplingRate == 0) 
            {
            MProcClipInfoObserver* observer = iObserver;
            
            iObserver = 0;
            observer->NotifyClipInfoReady(KErrNotSupported); 
            PRINT((_L("CProcClipInfoAO::RunL audio in the clip not supported") ));
            }
        else 
            {
            MProcClipInfoObserver* observer = iObserver;
            
            iObserver = 0;
            observer->NotifyClipInfoReady(KErrNone); 
            }
        
        }
    
    PRINT((_L("CProcClipInfoAO::RunL out") ));


    }

void CProcClipInfoAO::DoCancel() 
    {

    }
   
CProcClipInfoAO* CProcClipInfoAO::NewL() 
    {


    CProcClipInfoAO* self = new (ELeave) CProcClipInfoAO();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }

CProcClipInfoAO::~CProcClipInfoAO() 
    {
    PRINT((_L("CProcClipInfoAO::~CProcClipInfoAO() in") ));
    
    Cancel();
    
    if (iFileName)
        {
        delete iFileName;
        iFileName = 0;
        }
    
    if (iProcessorImpl)
        {
        delete iProcessorImpl;
        iProcessorImpl = 0;
        }

    PRINT((_L("CProcClipInfoAO::~CProcClipInfoAO() out") ));
    }
    

void CProcClipInfoAO::StartL(const TDesC& aFilename, 
                             RFile* aFileHandle,
                             MProcClipInfoObserver &aObserver, 
                             TAudFileProperties* aProperties,
                             TInt aPriority) 
    {

    iObserver = &aObserver;
    iProperties = aProperties;

    if (!aFileHandle)
    {        
        iFileName = HBufC::NewL(aFilename.Length());
        *iFileName = aFilename;
        iFileHandle = NULL;
    } 
    else
    {        
        iFileHandle = aFileHandle;
        iFileName = HBufC::NewL(1);
    }
    
    CAudProcessorImpl* processorImpl = CAudProcessorImpl::NewLC();
    
    CleanupStack::Pop(processorImpl);
    iProcessorImpl = processorImpl;
    
    SetPriority(aPriority);
    SetActive();
    TRequestStatus* status = &iStatus;
    User::RequestComplete(status, KErrNone);

    }
    


void CProcClipInfoAO::ConstructL() 
    {

    }

CProcClipInfoAO::CProcClipInfoAO() :  CActive(0), iProperties(0), iFileName(0)
                                       
    {

    CActiveScheduler::Add(this);

    }

