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



#ifndef __CPROCAWBFRAMEHANDLER_H__
#define __CPROCAWBFRAMEHANDLER_H__

#include <e32base.h>
#include "AudCommon.h"
#include "ProcConstants.h"
#include "ProcFrameHandler.h"

class CProcAWBFrameHandler : public CProcFrameHandler 
    {

public:

    // aGain -40(dB) - +20(dB)
    virtual TBool ManipulateGainL(const HBufC8* aFrameIn, HBufC8*& aFrameOut, TInt8 aGain);

    // From base class
    virtual TBool GetGainL(const HBufC8* aFrame, RArray<TInt>& aGains, TInt& aMaxGain) const;
    virtual TBool GetNormalizingMargin(const HBufC8* aFrame, TInt8& aMargin) const;

    virtual ~CProcAWBFrameHandler();

    static CProcAWBFrameHandler* NewL();
    static CProcAWBFrameHandler* NewLC();

private:

    void ConstructL();
    CProcAWBFrameHandler();

    TBool GetAWBGainsL(const HBufC8* aFrame, RArray<TInt>& aGains, TInt& aMaxGain) const;

//    RArray<TInt> iGains;
//    TInt iGainsRead;

    
    };

#endif
