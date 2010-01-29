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



#ifndef __CPROCAMRFRAMEHANDLER_H__
#define __CPROCAMRFRAMEHANDLER_H__

#include <e32base.h>
#include <f32file.h>

#include "AudCommon.h"
#include "ProcConstants.h"
#include "ProcFrameHandler.h"

class CProcAMRFrameHandler : public CProcFrameHandler 
    {

public:

    /*
    * Symbian constructors and a destructor
    */

    static CProcAMRFrameHandler* NewL();
    static CProcAMRFrameHandler* NewLC();

    virtual ~CProcAMRFrameHandler();

    // From base class CProcFrameHandler
    virtual TBool ManipulateGainL(const HBufC8* aFrameIn, HBufC8*& aFrameOut, TInt8 aGain);
    virtual TBool GetGainL(const HBufC8* aFrame, RArray<TInt>& aGains, TInt& aMaxGain) const;
    virtual TBool GetNormalizingMargin(const HBufC8* aFrame, TInt8& aMargin) const;

    
private:
    
    // constructL
    void ConstructL();
    
    // c++ constructor
    CProcAMRFrameHandler();

    // internal function to read AMR gains
    TBool GetAMRGains(const HBufC8* aFrame, RArray<TInt>& aGains, TInt& aMaxGain) const;

    // gets the length of the next frame based on mode myte
    TInt GetNextFrameLength(const HBufC8* aFrame, TInt aPosNow);
    
    TBool GetGPGains(const HBufC8* aFrame, RArray<TInt>& aGains) const;
    
    // member variables for energy calculation
    mutable TReal iPreviousEnergy;
	mutable TReal iPreviousRn[4];

    
    };

#endif
