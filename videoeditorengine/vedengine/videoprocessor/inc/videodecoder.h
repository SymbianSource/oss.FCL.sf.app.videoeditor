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
* Definition for CVideoDecoder, an abstract base class for all video
* decoders.
*
*/



#ifndef     __VIDEODECODER_H__
#define     __VIDEODECODER_H__


//  INCLUDES

#ifndef __DECODER_H__
#include "decoder.h"
#endif


//  CLASS DEFINITIONS

// Decoder base class
class CVideoDecoder : public CDecoder
{
public:
    CVideoDecoder(TInt aPriority=EPriorityStandard) : CDecoder(aPriority) {};
    // CDataProcessor provides a virtual destructor
};


#endif      //  __VIDEODECODER_H__
            
// End of File
