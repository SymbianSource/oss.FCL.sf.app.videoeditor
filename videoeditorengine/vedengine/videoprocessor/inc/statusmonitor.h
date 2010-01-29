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

#ifndef     __STATUSMONITOR_H__
#define     __STATUSMONITOR_H__


//  INCLUDES

#ifndef __E32BASE_H__
#include <e32base.h>
#endif


//  FORWARD DECLARATIONS

class MVedMovieProcessingObserver;
class CMovieProcessorImpl;
class CVedMovie;


//  CLASS DEFINITIONS

class CStatusMonitor : public CActive
{
public: // constants
    enum TErrorCode
    {        
        EInternalAssertionFailure = -2000,
        EInvalidStateTransition = -2001
    };

public: // new functions
    // Constructors
    CStatusMonitor(MVedMovieProcessingObserver *anObserver, 
                   CMovieProcessorImpl *aProcessor, CVedMovie *aMovie,
                   TInt aPriority=EPriorityHigh);

    void ConstructL();

    // An error has occurred
    void Error(TInt anErrorCode);

    //  Initialisation for processing has started    
    void StartPreparing();

    // The processor has been initialized and is ready for processing
    void PrepareComplete();

    // The processor has been closed
    void Closed();

    // Processing has been started
    void ProcessingStarted(TBool aNotifyObserver);

    // Processing progress indication
    void Progress(TInt aPercentage);

    // Processing has been stopped
    void ProcessingStopped();

    // The clip end has been reached
    void ClipProcessed();

    // The movie end has been reached
    void ProcessingComplete();

    // processing has been cancelled
    void ProcessingCancelled();

    // A fatal non-recovereable error has occurred
    void FatalError(TInt anError);

     // dummy
    void StreamEndReached();
    

public: // CActive methods
    ~CStatusMonitor();
    void RunL();
    void DoCancel();

    
private: // Data
    MVedMovieProcessingObserver *iObserver;
    CMovieProcessorImpl *iProcessor;

    CVedMovie *iMovie;

    TInt iError; // the error that has been encountered
    TInt iOutError; // for returning the error to observer

    TBool iProcessingStarted;  // has processing been started ?
    TBool iProcessing; // are we currently processing ?
    TBool iCancelled;  // has processing been cancelled ?
    
    TBool iClipProcessed; // a video clip has been processed
    TBool iComplete;  // processing complete => inform observer

    TBool iPrepared; // is the processor open & ready to process ?
    TBool iPreparing; // is the processor being initialized  ?

     
};



#endif      //  __STATUSMONITOR_H__
            
// End of File
