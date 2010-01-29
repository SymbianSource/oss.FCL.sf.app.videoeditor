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



#ifndef __CPROCPROCESSAO_H__
#define __CPROCPROCESSAO_H__

#include <e32base.h>
#include "AudCommon.h"
#include "AudSong.h"

#include "AudProcessor.h"

#include "AudProcessorImpl.h"

#include "ProcEncoder.h"

class MProcProcessObserver;
class CAudProcessorImpl;
class CAudProcessor;

class CProcProcess : public CBase
    {

public:

    /**
    *
    * Constructor & destructor
    *
    */
    static CProcProcess* NewL();

    ~CProcProcess();

    /**
    * Starts a syncronous song processing operation
    *
    * @param    aSong        song
    * 
    */
    TBool StartSyncProcessingL(const CAudSong* aSong, TBool aGetTimeEstimation = EFalse);

    /**
    * Processes one piece syncronously
    *
    * @param    aFrame        audio frame in output
    * @param    aProgerss    current progress (0-100)
    * @param    aDuration    duration of aFrame
    * 
    */
    TBool ProcessSyncPieceL(HBufC8*& aFrame, TInt& aProgress,
                                       TTimeIntervalMicroSeconds& aDuration);

    /**
    *
    * Gets time estimate once the time estimate has been calculated
    *
    * If time estimation has not been processed, returns 0
    *
    * @return processing time estimate in microseconds
    */

    TInt64 GetFinalTimeEstimate() const;
    
    
protected:

private:
    
    // constructL
    void ConstructL();
    
    // C++ constructor
    CProcProcess();
    
private:
    
    // observer for callbacks
    MProcProcessObserver* iObserver;
    
    // processorImpl owned by this
    CAudProcessorImpl* iProcessorImpl;
    
    // song
    const CAudSong* iSong;
    
    
    // encoder
    CProcEncoder* iEncoder;
    
    // buffer for getting data from encoder
    HBufC8* iDecBuffer;
    
    // sometimes the encoder returns more than one AMR frame at a time
    // still we need to return only one frame to the higher level
    // this buffer is a temporary storage for extra AMR frames
    
    HBufC8* iAMRBuf;
    
    // progress
    TInt iProgress;
    
    HBufC8* iAACBuf;
    
    // buffer for feeding the encoder
    CMMFDataBuffer* iEncFeedBuffer;
    
    TInt64 iTimeEstimate;
    
    };


#endif
