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
* Implementation of audio processor class.                
*
*/


//  Include Files
#include "movieprocessorimpl.h"
#include "AudSong.h"
#include "audioprocessor.h"

#ifdef _DEBUG
#include <e32svr.h>
#define PRINT(x) RDebug::Print x;
#else
#define PRINT(x)
#endif

// Macros
#define APASSERT(x) __ASSERT_DEBUG(x, User::Panic(_L("CAudioProcessor"), -30000))

const TUint KNumFramesInOneRun = 20;


// ================= MEMBER FUNCTIONS =======================


// ---------------------------------------------------------
// CAudioProcessor::NewL
// Two-phased constructor.
// ---------------------------------------------------------
//
CAudioProcessor* CAudioProcessor::NewL(CMovieProcessorImpl *aMovieProcessor, CAudSong* aSong)
{
    CAudioProcessor* self = NewLC(aMovieProcessor, aSong);
    CleanupStack::Pop(self);
    return self;
}

CAudioProcessor* CAudioProcessor::NewLC(CMovieProcessorImpl *aMovieProcessor, CAudSong* aSong)
{
    CAudioProcessor* self = new (ELeave) CAudioProcessor(aMovieProcessor, aSong);
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
}


// ---------------------------------------------------------
// CAudioProcessor::NewL
// C++ default constructor
// ---------------------------------------------------------
//
CAudioProcessor::CAudioProcessor(CMovieProcessorImpl *aMovieProcessor, CAudSong* aSong) : CActive(EPriorityNormal)
{
    iMovieProcessor = aMovieProcessor;
    iSong = aSong;	
    iProcessing = EFalse;
}

// -----------------------------------------------------------------------------
// CAudioProcessor::ConstructL
// Symbian 2nd phase constructor 
// -----------------------------------------------------------------------------
//
void CAudioProcessor::ConstructL()
{

    if(!iSong)
    {
        User::Leave(KErrArgument);	
    }     	

    SetPriority( EPriorityNormal );
    // Add to active scheduler
    CActiveScheduler::Add(this);
    
    // Make us active
    SetActive();
    iStatus = KRequestPending;
  
}

// -----------------------------------------------------------------------------
// CAudioProcessor::~CAudioProcessor
// Destructor
// -----------------------------------------------------------------------------
//
CAudioProcessor::~CAudioProcessor()
{
    PRINT((_L("CAudioProcessor::~CAudioProcessor() begin")));

    Cancel();

    PRINT((_L("CAudioProcessor::~CAudioProcessor() end")));
}


// -----------------------------------------------------------------------------
// CAudioProcessor::DoCancel()
// DoCancel for AO framework
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CAudioProcessor::DoCancel()
{
    // Cancel our internal request
    if ( iStatus == KRequestPending )
    {
        PRINT((_L("CAudioProcessor::DoCancel() cancel request")))
        TRequestStatus *status = &iStatus;
        User::RequestComplete(status, KErrCancel);
    }
}

// -----------------------------------------------------------------------------
// CAudioProcessor::RunL()
// Running method of AO framework
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CAudioProcessor::RunL()
{

    if (!iProcessing)
        return;
    
    ProcessFramesL();
        
}


// -----------------------------------------------------------------------------
// CAudioProcessor::ProcessFramesL()
// Process audio frames
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CAudioProcessor::ProcessFramesL()
{

    TInt progress = 0;
    HBufC8* frame = 0;
    TPtr8 ptr(0,0);	
    TTimeIntervalMicroSeconds time(0);
    
    TBool gotFrame = ETrue; // true indicates end of sequence
    
    TInt framesProcessed = 0;
    
    do
    {	    
        gotFrame = iSong->SyncProcessFrameL(frame,progress,time); 

        if(gotFrame)
        {
            break;		
        }

        if (frame == NULL || frame->Size() == 0)
        {
            continue;
        }
		
        CleanupStack::PushL(frame);

        TInt duration = I64INT(time.Int64()); 

        ptr.Set((TUint8*)frame->Ptr(),frame->Length(),frame->Length());

        User::LeaveIfError(iMovieProcessor->WriteAllAudioFrames((TDesC8&)ptr,duration));			

        CleanupStack::Pop();

        if(frame != 0)
        {
            delete frame;		
            frame = 0;
        }

        // Increment the frame number
        framesProcessed++;

        if (framesProcessed >= KNumFramesInOneRun)
        {
            iStatus = KRequestPending;
            SetActive();
            TRequestStatus *tmp = &iStatus;
            User::RequestComplete( tmp, KErrNone );
            return;
        }			
		
	} while(!gotFrame);
	
    if(frame!=0)
    {
        delete frame;
        frame = 0;
    }

    // finished
    iProcessing = EFalse;

    iMovieProcessor->AudioProcessingComplete(KErrNone);	
	
}

// -----------------------------------------------------------------------------
// CAudioProcessor::RunError()
// RunError method of AO framework
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CAudioProcessor::RunError(TInt aError)
{
    iProcessing = EFalse;

    iMovieProcessor->AudioProcessingComplete(aError);

    return KErrNone;
}

// -----------------------------------------------------------------------------
// CAudioProcessor::StartL()
// Stops audio processing
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CAudioProcessor::StartL()
{
    PRINT((_L("CAudioProcessor::StartL() begin")));
    
    if ( iProcessing )
        return;
    
    if (iSong->ClipCount(KAllTrackIndices) == 0)
    {
        iMovieProcessor->AudioProcessingComplete(KErrNone);
        return;    	
    }

    iSong->SyncStartProcessingL();
    
    TRequestStatus *status = &iStatus;
    User::RequestComplete(status, KErrNone);
    
    iProcessing = ETrue;    

    PRINT((_L("CAudioProcessor::StartL() end")));
}

// -----------------------------------------------------------------------------
// CAudioProcessor::StopL()
// Stops audio processing
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CAudioProcessor::StopL()
{
    PRINT((_L("CAudioProcessor::StopL() begin")));
    
    if ( !iProcessing )
        return;

    Cancel();
    
    iSong->SyncCancelProcess();

    iMovieProcessor->AudioProcessingComplete(KErrNone);

    iProcessing = EFalse;
    
    PRINT((_L("CAudioProcessor::StopL() end")));
}

// End of file

