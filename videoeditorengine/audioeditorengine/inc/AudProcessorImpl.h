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



#ifndef __CAUDPROCESSORIMPL_H__
#define __CAUDPROCESSORIMPL_H__

#include <e32base.h>
#include <e32math.h>
#include "AudClipInfo.h"
#include "AudCommon.h"
#include "AudSong.h"
#include "ProcInFileHandler.h"

#include "ProcFrameHandler.h"
#include "ProcWAVFrameHandler.h"




#include "ProcTools.h"


class MProcClipInfoObserver;

class CAudProcessorImpl : public CBase
    {
    
public:

    /*
    *
    * Symbian constructors and a destructor
    *
    */

    static CAudProcessorImpl* NewL();
    static CAudProcessorImpl* NewLC();

    virtual ~CAudProcessorImpl();


    /**
    * Performs all initializations needed for a song processing
    * such as output file opening
    * 
    * Can leave with one of the system wide error codes
    * related to memory allocation and file opening
    *
    * Possible panic code
    * <code>ESongProcessingOperationAlreadyRunning</code>
    *
    *
    *
    * @param aSong              song to be processed
    * @param aRawFrameSize      length of raw audio frames
    * @param aGetTimeEstimation ETrue if getting a time estimation
    *
    * @return
    *
    */
    void ProcessSongL(const CAudSong* aSong, TInt aRawFrameSize, 
                      TBool aGetTimeEstimation = EFalse);
    
    /**
    * Processes one audio frame and writes it to an output file
    *
    * Possible panic code
    * <code>ESongProcessingOperationNotRunning</code>
    *
    * @param aProgress    output parameter to indicate progress in percents
    *
    *
    * @return            ETrue if processing completed, EFalse otherwise
    *
    */
    TBool ProcessSyncPieceL(HBufC8*& aFrame, TInt& aProgress,
                                       TTimeIntervalMicroSeconds& aDuration, TBool& aRaw);



    /**
    * Gets properties of an audio file
    * 
    * Can leave with one of the error codes related to file opening
    * 
    * @param aFileName        name of the file
    * @param aFileHandle      file handle of the file to read
    * @param aProperties    pointer to audio file properties (needs to be
    *                        allocated before calling this function)
    *
    */
    void GetAudFilePropertiesL(const TDesC& aFileName, RFile* aFileHandle,
                               TAudFileProperties* aProperties);

    /**
    *
    * Gets time estimate once the time estimate has been calculated
    *
    * If time estimation has not been processed, returns 0
    *
    * @return processing time estimate in microseconds
    */
    
    TInt64 GetFinalTimeEstimate() const;
    

private:
    
    /*
    * Symbian constructor
    */
    void ConstructL();

    /*
    * Generates processing events for the output clip
    */
    void GetProcessingEventsL();

    /*
    * Calculates a gain value on a given time
    */
    TInt8 GetGainNow(const CAudClip* aClip, TInt32 aTimeMilliSeconds);

    /*
    * Stops processing
    */
    TBool StopProcessing();

    /*
    * Writes one frame of silence
    */
    TBool WriteSilenceL(HBufC8*& aFrame, TInt& aProgress,
                                       TTimeIntervalMicroSeconds& aDuration, TBool& aRaw);
    /*
    * Returns the index of the clip with highest priority 
    */
    TBool HighestInFilePriority(TInt& aFirst, TInt& aSecond);
    
    /*
    * Compares the two properties and returns true if decoding is required
    */
    TBool IsDecodingRequired(const TAudFileProperties& prop1, const TAudFileProperties& prop2);

    /*
    * C++ constructor
    */
    CAudProcessorImpl();
    
private:
    
    // Array for all the input clips
    RPointerArray<CProcInFileHandler> iInFiles;
    
    // Song
    const CAudSong* iSong;

    // frame handler for PCM operations such as mixing
    CProcWAVFrameHandler* iWAVFrameHandler;

    // time left before the end of last clip
    // (additional silence may be written after that)
    TInt32 iTimeLeft;
    
    // index of the current processing event
    TInt iCurEvent;

    // array for processing events
    RPointerArray<CProcessingEvent> iProcessingEvents;
    
    // array for storing clip indexes that are currenty written
    RArray<TInt> iClipsWritten;
    
    
    // song duration in milliseconds
    TInt iSongDurationMilliSeconds;

    // how much has already been processed
    TInt iSongProcessedMilliSeconds;

    // index of the clip that was previously processed
    TInt iLastInClip1;
    
    // a flag to indicate if we are getting a processing time estimate
    TBool iGetTimeEstimation;
    
    // timer for calculating time estimation
    TTime iTimer;
    
    // a variable for time estimate
    TInt64 iTimeEstimate;
    
    // a variable for time estimate calculation
    TReal iTimeEstimateCoefficient;
    
    TBool iSilenceStarted;
    
    };


#endif
