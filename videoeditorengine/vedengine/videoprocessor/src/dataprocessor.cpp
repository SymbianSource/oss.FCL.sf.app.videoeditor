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
* Base class for data processor objects.
*
*/



//  EXTERNAL RESOURCES  


//  Include Files  

#include "dataprocessor.h"




//  MEMBER FUNCTIONS


//=============================================================================


/*
-----------------------------------------------------------------------------

    CDataProcessor

    CDataProcessor()

    Standard C++ constructor

-----------------------------------------------------------------------------
*/

CDataProcessor::CDataProcessor(TInt aPriority)
    : CActive(aPriority)
{
}



/*
-----------------------------------------------------------------------------

    CDataProcessor

    ~CDataProcessor()

    Standard C++ destructor

-----------------------------------------------------------------------------
*/

CDataProcessor::~CDataProcessor()
{
    Cancel();
}



/*
-----------------------------------------------------------------------------

    CDataProcessor

    ConstructL()

    Standard Symbian OS second-phase constructor

-----------------------------------------------------------------------------
*/

void CDataProcessor::ConstructL()
{
    // Add to active scheduler and activate
    CActiveScheduler::Add(this);
    SetActive();
    iStatus = KRequestPending;
}



/*
-----------------------------------------------------------------------------

    CDataProcessor

    InputDataAvailable()

    Called by CActiveQueue when input data is available, simply signals the
    object

-----------------------------------------------------------------------------
*/

void CDataProcessor::InputDataAvailable(TAny* /*aUserPointer*/)
{
    if ( iStatus == KRequestPending )
    {
        TRequestStatus *status = &iStatus;
        User::RequestComplete(status, KErrNone);
    }
}



/*
-----------------------------------------------------------------------------

    CDataProcessor

    OutputSpaceAvailable()

    Called by CActiveQueue when output space is available, simply signals the
    object

-----------------------------------------------------------------------------
*/

void CDataProcessor::OutputSpaceAvailable(TAny* /*aUserPointer*/)
{

    if ( iStatus == KRequestPending )
    {
        TRequestStatus *status = &iStatus;
        User::RequestComplete(status, KErrNone);
    }


}



/*
-----------------------------------------------------------------------------

    CDataProcessor

    StreamEndReached()

    Called by CActiveQeueue after all data has been read from a queue where
    stream end has been signaled. Does nothing in this base class.

-----------------------------------------------------------------------------
*/

void CDataProcessor::StreamEndReached(TAny* /*aUserPointer*/)
{
}



/*
-----------------------------------------------------------------------------

    CDataProcessor

    DoCancel()

    Cancels the internal request.

-----------------------------------------------------------------------------
*/

void CDataProcessor::DoCancel()
{
    if ( iStatus == KRequestPending )
    {
        TRequestStatus *status = &iStatus;
        User::RequestComplete(status, KErrCancel);
    }
}


// End of File
