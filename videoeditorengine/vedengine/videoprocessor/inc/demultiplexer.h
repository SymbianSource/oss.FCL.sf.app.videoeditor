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
* Definition for CDemultiplexer, an abstract base class for all
* demultiplexers.
*
*/


#ifndef     __DEMULTIPLEXER_H__
#define     __DEMULTIPLEXER_H__


//  INCLUDES

#ifndef __DATAPROCESSOR_H__
#include "dataprocessor.h"
#endif


//  CLASS DEFINITIONS

// Demultiplexer base class
class CDemultiplexer : public CDataProcessor
{
public:
    CDemultiplexer(TInt aPriority=EPriorityStandard) : CDataProcessor(aPriority) {};
    // CDataProcessor provides a virtual destructor

    // Start/stop demuxing
    virtual void Start() = 0;
    virtual void Stop() = 0;
};


#endif      //  __DEMULTIPLEXER_H__
            
// End of File
