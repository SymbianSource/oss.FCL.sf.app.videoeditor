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


#ifndef __CPROCMP3FRAMEHANDLER_H__
#define __CPROCMP3FRAMEHANDLER_H__

/*-- System Headers. --*/
#include <e32base.h>

/*-- Project Headers. --*/
#include "AudCommon.h"
#include "ProcFrameHandler.h"
#include "ProcInFileHandler.h"

#include "MP3API.h"

/**
 * Frame handler class for accessing MP3 data elements.
 *
 * @author  Juha Ojanperä
 */
class CProcMP3FrameHandler : public CProcFrameHandler 
    {
public:

    
    /*
    * Constructors & destructor
    */
    static CProcMP3FrameHandler* NewL();
    static CProcMP3FrameHandler* NewLC();

    ~CProcMP3FrameHandler();


    // From base class CProcFrameHandler ---------->
    TBool ManipulateGainL(const HBufC8* aFrameIn, HBufC8*& aFrameOut, TInt8 aGain);
    TBool GetGainL(const HBufC8* aFrame, RArray<TInt>& aGains, TInt& aMaxGain) const;
    TBool GetNormalizingMargin(const HBufC8* aFrame, TInt8& aMargin) const;
    TBool IsMixingAvailable() const;
    // <--------------------------------------------
    
    

private:

    // ConstructL
    void ConstructL();
    
    // c++ constructor
    CProcMP3FrameHandler();
    
    // internal function for reading MP3 gains
    TBool GetMP3Gains(const HBufC8* aFrame, RArray<TInt>& aGains, TInt& aMaxGain, TBitStream& aBs) const;

private:

    // a handle to MP3 editing class in MP3AACManipLib
    CMp3Edit* iMp3Edit;
    
    // a flag to indicate whether MP3 decoder is initialized
    mutable TBool decInitialized;
    
    // a handle to CMPAudDec
    CMPAudDec *iMpAud;
    TMpTransportHandle *iTHandle;
    

    friend class CProcMP3InFileHandler;

    };

#endif /*-- __CPROCMP3FRAMEHANDLER_H__ --*/
