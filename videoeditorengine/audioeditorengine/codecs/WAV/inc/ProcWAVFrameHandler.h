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



#ifndef __CPROCWAVFRAMEHANDLER_H__
#define __CPROCWAVFRAMEHANDLER_H__

#include <e32base.h>
#include "AudCommon.h"
#include "ProcConstants.h"
#include "ProcFrameHandler.h"

class CProcWAVFrameHandler : public CProcFrameHandler 
    {

public:

    /*
    * Constructors & destructor
    *
    */
    static CProcWAVFrameHandler* NewL(TInt aBitsPerSample);
    static CProcWAVFrameHandler* NewLC(TInt aBitsPerSample);
    virtual ~CProcWAVFrameHandler();


    // From base class --------------->
    virtual TBool ManipulateGainL(const HBufC8* aFrameIn, HBufC8*& aFrameOut, TInt8 aGain);

    
    virtual TBool GetGainL(const HBufC8* aFrame, RArray<TInt>& aGains, TInt& aMaxGain) const;
    virtual TBool GetNormalizingMargin(const HBufC8* aFrame, TInt8& aMargin) const;

    virtual TBool IsMixingAvailable() const;
    virtual TBool MixL(const HBufC8* aFrame1, const HBufC8* aFrame2, HBufC8*& aMixedFrame);
    //<---------------------------------

    
private:

    // ConstructL
    void ConstructL();
    
    // c++ constructor
    CProcWAVFrameHandler(TInt aBitsPerSample);
    
    // gets the highest gain
    TInt GetHighestGain(const HBufC8* aFrame, TInt& aMaxGain) const;

    // bitdepth
    TInt iBitsPerSample;

    
    };

#endif
