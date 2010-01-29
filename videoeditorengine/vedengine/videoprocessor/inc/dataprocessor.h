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
* Definition for CDataProcessor, the data processing object abstract
* base class.
*
*/



#ifndef     __DATAPROCESSOR_H__
#define     __DATAPROCESSOR_H__


//  INCLUDES

#ifndef __E32BASE_H__
#include <e32base.h>
#endif


//  FORWARD DECLARATIONS

//class MVideoPlayerObserver;


//  CLASS DEFINITIONS

class CDataProcessor : public CActive
{
public: // constants
    enum TErrorCode
    {
        EInternalAssertionFailure = -1100
    };

public: // new functions
    // Constructors
    CDataProcessor(TInt aPriority=EPriorityStandard);
    void ConstructL();

    // Destructor
    virtual ~CDataProcessor();

    // Called by CActiveQueue when input data is available
    virtual void InputDataAvailable(TAny *aUserPointer);

    // Called by CActiveQueue when output space is available
    virtual void OutputSpaceAvailable(TAny *aUserPointer);

    // Called by CActiveQeueue after all data has been read from a queue where
    // stream end has been signaled
    virtual void StreamEndReached(TAny *aUserPointer);

    // Standard active object RunL() method, overridden by derived class
    virtual void RunL() = 0;

    // Cancels the internal request
    virtual void DoCancel();
};



#endif      //  __DATAPROCESSOR_H__
            
// End of File
