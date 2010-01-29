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



#ifndef __CPROCAACFRAMEHANDLER_H__
#define __CPROCAACFRAMEHANDLER_H__

#include <e32base.h>
#include "AudCommon.h"
#include "ProcConstants.h"
#include "ProcFrameHandler.h"

#include "AACConstants.h"
#include "AACAPI.h"

class CProcAACFrameHandler : public CProcFrameHandler 
    {

public:

    // aGain -40(dB) - +20(dB)
    virtual TBool ManipulateGainL(const HBufC8* aFrameIn, HBufC8*& aFrameOut, TInt8 aGain);

    // From base class
    virtual TBool GetGainL(const HBufC8* aFrame, RArray<TInt>& aGains, TInt& aMaxGain) const;
    virtual TBool GetNormalizingMargin(const HBufC8* aFrame, TInt8& aMargin) const;

    virtual ~CProcAACFrameHandler();

    static CProcAACFrameHandler* NewL(TAACFrameHandlerInfo aAACInfo);
    static CProcAACFrameHandler* NewLC(TAACFrameHandlerInfo aAACInfo);
    
    /*
    *
    * If there are several AAC frames in a row, their start positions and
    * lengths can be parsed with this function
    *
    * @param aFrame                input frame to be parsed
    * @param aFrameStarts        starting locations of each frame
    * @param aFrameLengths        frame lengths of each frame
    */
    
    TBool ParseFramesL(HBufC8* aFrame, RArray<TInt>& aFrameStarts, RArray<TInt>& aFrameLengths);

    /*
    * GetEnhancedAACPlusParametersL
    *
    * Fills in aacPlus related fields in aProperties
    *
    *
    * @param    buf            audio data
    * @param    bufLen        lenght of "buf"
    * @param     aProperties    properties to fill in
    * @param    aAACInfo    info for AAC framehandler    
    *
    */

    static void GetEnhancedAACPlusParametersL(TUint8* buf, TInt bufLen, 
                                            TAudFileProperties* aProperties, 
                                            TAACFrameHandlerInfo *aAACInfo);

    /*
    * CalculateNumberOfHeaderBytes returns the number of ADTS header bytes 
    * and the number of data blocks in a given frame
    *
    * @param    aFrame    input frame
    * @param    aNumBlocks (output) number of data blocks in the frame (usually 1)
    *
    * @return    the number of ADTS header bytes
    */

    TInt CalculateNumberOfHeaderBytes(const HBufC8* aFrame, TInt& aNumBlocksInFrame) const;

private:

    // constructL
    void ConstructL(TAACFrameHandlerInfo aAACInfo);
    
    // c++ constructor
    CProcAACFrameHandler();

    // updates the ADTS header if the lenght has 
    // been changed after modifying AACPlus gain
    TBool UpdateHeaderL(HBufC8* aFrame);

private:

    // AAC decoder handle
    CAACAudDec* iDecHandle; 
    
    // stores the starting points of raw data blocks if there are 
    // more than one data block in AAC frame
    RArray<TInt> iFrameStarts;
    
    // lenghts of the data blocks 
    RArray<TInt> iFrameLengths;
    
    // info needed for AAC decoder handle creation
    TAACFrameHandlerInfo iAACInfo;
    
    };

#endif
