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
* Definition for CDecoder, an abstract base class for all decoders.
*
*/




#ifndef     __DECODER_H__
#define     __DECODER_H__


//  INCLUDES

#ifndef __DATAPROCESSOR_H__
#include "dataprocessor.h"
#endif


//  CLASS DEFINITIONS

// Decoder base class
class CDecoder : public CDataProcessor
{
public:
    CDecoder(TInt aPriority=EPriorityStandard) : CDataProcessor(aPriority) {};
    // CDataProcessor provides a virtual destructor

    // Start/stop decoding
    virtual void Start() = 0;
    virtual void Stop() = 0;
};


#endif      //  __DECODER_H__
            
// End of File
