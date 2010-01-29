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



#ifndef __CPROCTIMEESTIMATEAO_H__
#define __CPROCTIMEESTIMATEAO_H__

#include <e32base.h>
#include "AudSong.h"
#include "AudCommon.h"
#include "ProcProcessAO.h"

class CProcProcess;


class CProcTimeEstimateAO : public CActive 
    {

public:

    static CProcTimeEstimateAO* NewL();

    virtual ~CProcTimeEstimateAO();

    /**
    *
    * Starts retrieving audio clip info
    *
    * @param    aFilename        filename of the input file
    * @param    aObserver        observer to be notified of progress
    * @param    aProperties        properties of the input file.
    *                            Needs to be allocated by the caller,                            
    *                            and will filled in as a result of calling
    *                            this function
    * @param    aPriority        priority of the operation
    *
    */
    TBool StartL(const CAudSong* aSong, MAudTimeEstimateObserver& aTEObserver);
    void CancelTimeEstimate();
    
        
      
protected:
    virtual void RunL();
    virtual void DoCancel();

private:

    void ConstructL();

private:
    
    
    // C++ constructor
    CProcTimeEstimateAO();
    
    
    // processing AC for getting frames
    CProcProcess* iProcessingObject;
    
    // observer to notify
    MAudTimeEstimateObserver* iTEObserver;
    
    // song
    const CAudSong* iSong;
    
    };


#endif
