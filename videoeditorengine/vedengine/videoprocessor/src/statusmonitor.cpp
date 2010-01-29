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
* Video player status monitor definitions, class CStatusMonitor.
*
*/


//  EXTERNAL RESOURCES  


//  Include Files

#include "movieprocessorimpl.h"
#include "statusmonitor.h"
#include "vedmovie.h"


// LOCAL CONSTANTS AND MACROS
// Debug print macro
#if defined _DEBUG 
#include <e32svr.h>
#define PRINT(x) RDebug::Print x;
#else
#define PRINT(x)
#endif


//  MEMBER FUNCTIONS


//=============================================================================



/*
-----------------------------------------------------------------------------

    CStatusMonitor

    CStatusMonitor()

    Standard C++ constructor

-----------------------------------------------------------------------------
*/

CStatusMonitor::CStatusMonitor(MVedMovieProcessingObserver *anObserver,                               
                               CMovieProcessorImpl *aMovieProcessor,
                               CVedMovie *aMovie, TInt aPriority)
    : CActive(aPriority)
{
    // Remember the objects
    iObserver = anObserver;
    iProcessor = aMovieProcessor;
    iMovie = aMovie;
    // Initialize status:

    iPrepared = EFalse;
    iPreparing = EFalse;

    iProcessingStarted = EFalse;
    iProcessing = EFalse;
    iClipProcessed = EFalse;
    iComplete = EFalse;
    iCancelled = EFalse;
    iError = KErrNone;
    iOutError = KErrNone;

}



/*
-----------------------------------------------------------------------------

    CStatusMonitor

    ~CStatusMonitor()

    Standard C++ destructor

-----------------------------------------------------------------------------
*/

CStatusMonitor::~CStatusMonitor()
{
    Cancel();
}



/*
-----------------------------------------------------------------------------

    CStatusMonitor

    ConstructL()

    Symbian OS second-phase constructor

-----------------------------------------------------------------------------
*/

void CStatusMonitor::ConstructL()
{
    // Add to active scheduler:
    CActiveScheduler::Add(this);
    
    // Make us active:
    if (!IsActive())
    {
        SetActive();
        iStatus = KRequestPending;
    }
}



/*
-----------------------------------------------------------------------------

    CStatusMonitor

    Error()

    An error has occurred

-----------------------------------------------------------------------------
*/

void CStatusMonitor::Error(TInt anErrorCode)
{
    // Remember the error
    iError = anErrorCode;

#ifndef _DEBUG
    if (iError < KErrHardwareNotAvailable)
        iError = KErrGeneral;
#endif

    PRINT((_L("CStatusMonitor::Error()  Error = %d  "), iError )); 

    // Activate the object:
    if ( iStatus == KRequestPending )
    {
        TRequestStatus *status = &iStatus;
        User::RequestComplete(status, KErrNone);
    }
}



/*
-----------------------------------------------------------------------------

    CStatusMonitor

    StartOpening()

    The stream opening has been started

-----------------------------------------------------------------------------
*/

void CStatusMonitor::StartPreparing()
{
    __ASSERT_DEBUG((!iPrepared) && (!iPreparing) && (!iProcessing),
                   User::Panic(_L("CVideoProcessor"), EInvalidStateTransition));
    
    // Note the state change:
    iPreparing = ETrue;
}



/*
-----------------------------------------------------------------------------

    CStatusMonitor

    Opened()

    The stream has been opened and it is ready for playback

-----------------------------------------------------------------------------
*/

void CStatusMonitor::PrepareComplete()
{
    __ASSERT_DEBUG((!iPrepared) && iPreparing && (!iProcessing),
                   User::Panic(_L("CVideoProcessor"), EInvalidStateTransition));
    
    // Note the state change
    iPrepared = ETrue;
    iPreparing = EFalse;
    
}

void CStatusMonitor::StreamEndReached()
{


}

void CStatusMonitor::Progress(TInt aPercentage)
{
    PRINT((_L("CStatusMonitor::Progress()  Progress = %d  "), aPercentage )); 

    iObserver->NotifyMovieProcessingProgressed(*iMovie, aPercentage);
}

/*
-----------------------------------------------------------------------------

    CStatusMonitor

    Closed()

    The stream has been closed

-----------------------------------------------------------------------------
*/

void CStatusMonitor::Closed()
{
    __ASSERT_DEBUG((iPrepared || iPreparing) && (!iProcessing),
                   User::Panic(_L("CVideoProcessor"), EInvalidStateTransition));

    // Note the state change:    
    iPreparing = EFalse;    

    // Do not report a stream open if one had been done
    iPrepared = EFalse;    
}



/*
-----------------------------------------------------------------------------

    CStatusMonitor

    ProcessingStarted()

    Processing has been started

-----------------------------------------------------------------------------
*/

void CStatusMonitor::ProcessingStarted(TBool aNotifyObserver)
{
    __ASSERT_DEBUG( (!iProcessing) && (!iProcessingStarted) && (iPrepared),
                   User::Panic(_L("CVideoProcessor"), EInvalidStateTransition));

    // Note the state change:    
    iProcessing = ETrue;

    if (aNotifyObserver)
    {
        iProcessingStarted = ETrue;
        
        // Activate the object:
        if ( iStatus == KRequestPending )
        {
            TRequestStatus *status = &iStatus;
            User::RequestComplete(status, KErrNone);
        }
    }   
}



/*
-----------------------------------------------------------------------------

    CStatusMonitor

   ProcessingStopped()

    Processing has been stopped

-----------------------------------------------------------------------------
*/

void CStatusMonitor::ProcessingStopped()
{
    __ASSERT_DEBUG(iProcessing,
                   User::Panic(_L("CVideoProcessor"), EInvalidStateTransition));

    // Note the state change:
    iProcessing = EFalse;
}



/*
-----------------------------------------------------------------------------

    CStatusMonitor

    ClipProcessed()

    The video clip end has been reached

-----------------------------------------------------------------------------
*/

void CStatusMonitor::ClipProcessed()
{
    __ASSERT_DEBUG( (iProcessing),
                   User::Panic(_L("CVideoProcessor"), EInvalidStateTransition));

    // Note the stream end:
    iClipProcessed = ETrue;

    // Activate the object:
    if ( iStatus == KRequestPending )
    {
        TRequestStatus *status = &iStatus;
        User::RequestComplete(status, KErrNone);
    }
}

void CStatusMonitor::ProcessingComplete()
{
    __ASSERT_DEBUG((!iProcessing),
                   User::Panic(_L("CVideoProcessor"), EInvalidStateTransition));

    // Note the stream end:
    iComplete = ETrue;

    // Activate the object:
    if ( iStatus == KRequestPending )
    {
        TRequestStatus *status = &iStatus;
        User::RequestComplete(status, KErrNone);
    }
}

void CStatusMonitor::ProcessingCancelled()
{
    __ASSERT_DEBUG((!iProcessing),
                   User::Panic(_L("CVideoProcessor"), EInvalidStateTransition));
    
    iCancelled = ETrue;

    // Activate the object:
    if ( iStatus == KRequestPending )
    {
        TRequestStatus *status = &iStatus;
        User::RequestComplete(status, KErrNone);
    }
}



/*
-----------------------------------------------------------------------------

    CStatusMonitor

    FatalError()

    A fatal non-recovereable error has occurred

-----------------------------------------------------------------------------
*/

void CStatusMonitor::FatalError(TInt anError)
{
    PRINT((_L("CStatusMonitor::FatalError  Error = %d  "), anError )); 

    // Pass the error to the observer
    iObserver->NotifyMovieProcessingCompleted(*iMovie, anError);

    // The observer returned -- panic the program
    User::Panic(_L("CVideoProcessor"), anError);    
    
}



/*
-----------------------------------------------------------------------------

    CStatusMonitor

    RunL()

    The active object running method, called by the active scheduler when the
    object has been activated.

-----------------------------------------------------------------------------
*/

void CStatusMonitor::RunL()
{
    TInt error;
    
    // Re-activate us:
    if (!IsActive())
    {
        SetActive();
        iStatus = KRequestPending;
    }
    
    // Check if an error occurred:
    if ( iError != KErrNone )
    {
        // Yes, stop processing if we are processing, close stream if it was open:
        if ( iProcessing )
        {
            if ( iProcessor )
            {                
				iProcessor->CancelProcessingL();
            }
            iProcessing = EFalse;
        }
        else
        {
            if ( iPrepared || iPreparing )
            {
                if ( iProcessor )
                {                    
                    iProcessor->CancelProcessingL();                    
                }
                iPreparing = EFalse;
                iPrepared = EFalse;                
            }
        }

        // Report the error to the observer:
        //iObserver->NotifyMovieProcessingCompleted(*iMovie, iError);        
        iOutError = iError;
        iError = KErrNone;

        
    }
    
    else
    {
        // Nope, no errors

        // If processing has been started, report that:
        if ( iProcessingStarted )
        {
            iProcessingStarted = EFalse;
            TRAP(error, iObserver->NotifyMovieProcessingStartedL(*iMovie));
            if ( error != KErrNone )
                FatalError(error);
        }
		else if ( iCancelled )
        {
            // processing has been cancelled
            iCancelled = EFalse;
            iComplete = EFalse;
			iClipProcessed = EFalse;

            if ( iOutError == KErrNone )
                iObserver->NotifyMovieProcessingCompleted(*iMovie, KErrCancel);
            else
                iObserver->NotifyMovieProcessingCompleted(*iMovie, iOutError);

            iOutError = KErrNone;

        }

        else if ( iClipProcessed )
        {
            // a clip has been processed
            iClipProcessed = EFalse;
            iProcessor->FinalizeVideoClip();
        }        
        
        else 
        {
            if ( iComplete ) 
            {
                // If the movie has been completed, report that:            
                iComplete = EFalse;
                iObserver->NotifyMovieProcessingCompleted(*iMovie, KErrNone);
                    
            }
        }
    }
}



/*
-----------------------------------------------------------------------------

    CStatusMonitor

    DoCancel()

    Cancels the internal request

-----------------------------------------------------------------------------
*/

void CStatusMonitor::DoCancel()
{
    // Cancel it:
    TRequestStatus *status = &iStatus;
    User::RequestComplete(status, KErrCancel);
}




//  OTHER EXPORTED FUNCTIONS


//=============================================================================


//  End of File  
