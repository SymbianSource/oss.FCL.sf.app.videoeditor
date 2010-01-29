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



#ifndef __CPROCCLIPINFOAO_H__
#define __CPROCCLIPINFOAO_H__

#include <e32base.h>
#include "AudCommon.h"
#include "AudClipInfo.h"

#include "AudProcessorImpl.h"

class MProcClipInfoObserver;
class CAudProcessorImpl;

class CProcClipInfoAO : public CActive 
    {

public:

    static CProcClipInfoAO* NewL();

    virtual ~CProcClipInfoAO();

    /**
    *
    * Starts retrieving audio clip info
    *
    * @param    aFilename        filename of the input file
    * @param    aFileHandle      file handle of the input file
    * @param    aObserver        observer to be notified of progress
    * @param    aProperties        properties of the input file.
    *                            Needs to be allocated by the caller,                            
    *                            and will filled in as a result of calling
    *                            this function
    * @param    aPriority        priority of the operation
    *
    */
    void StartL(const TDesC& aFilename, 
                RFile* aFileHandle,
        MProcClipInfoObserver &aObserver,         
        TAudFileProperties* aProperties,
        TInt aPriority);
        
      
protected:
    virtual void RunL();
    virtual void DoCancel();

private:
    


    void ConstructL();


private:
    
    CProcClipInfoAO();
    MProcClipInfoObserver* iObserver;
    TAudFileProperties* iProperties;
    HBufC* iFileName;
    CAudProcessorImpl* iProcessorImpl;
    RFile* iFileHandle;
    
    };


#endif
