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



#ifndef __CPROCFRAMEHANDLER_H__
#define __CPROCFRAMEHANDLER_H__

#include <e32base.h>
#include "AudCommon.h"
#include "ProcConstants.h"

class CProcFrameHandler : public CBase 
    {

public:


    /**
    * Manipulates the gain of a certain audio frame
    *
    * @param    aFrameIn        a frame to be modified
    * @param    aFrameOut    a modified frame, the caller is responsible for releasing!
    * @param    aGain        gain change. One step represents 0.5 dB, so that
    *                        0->no change, 10 -> +5dB and -10 -> -5dB
    * @return    ETrue        if successful
    *
    *
    */
    virtual TBool ManipulateGainL(const HBufC8* aFrameIn, HBufC8*& aFrameOut, TInt8 aGain) = 0;

    /**
    * Retrieves the gains of a certain audio frame
    *
    * @param    aFrame    an audio frame
    * @param    aGains    retrieved gains
    * @param    aMaxGain the biggest possible gain value
    * @return    ETrue    if successful
    *
    *
    */
    virtual TBool GetGainL(const HBufC8* aFrame, RArray<TInt>& aGains, TInt& aMaxGain) const = 0;

    /**
    * Calculates how many decibels this frame can be amplified
    *
    * @param aFrame        an audio frame
    * @param aMargin    margin in debicels/2 -> 4 equals 2 decibels    
    */
    virtual TBool GetNormalizingMargin(const HBufC8* aFrame, TInt8& aMargin) const = 0;

    /**
    * Tells whether mixing is available or not
    *
    * @return ETrue        mixing is available
    * @return EFalse    mixing is not available
    */
    virtual TBool IsMixingAvailable() const;

    /**
    * Mixes two frames with each other
    *
    * @param aFrame1        frame 1 to be mixed
    * @param aFrame2        frame 2 to be mixed
    * @param aMixedFrame    resulting mixed frame
    *
    * @return                ETrue if mixing was successful. The caller is responsible for releasing memory
    *                        EFalse if mixing was not successful. aMixedFrame = 0 and the caller
    *                                doesn't need to release memory
    */
    virtual TBool MixL(const HBufC8* aFrame1, const HBufC8* aFrame2, HBufC8*& aMixedFrame);


    virtual ~CProcFrameHandler();

private:

protected:

    
    };

#endif
