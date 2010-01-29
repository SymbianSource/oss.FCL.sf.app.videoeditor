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


#ifndef __CPROCAWBINFILEHANDLER_H__
#define __CPROCAWBINFILEHANDLER_H__

#include <e32base.h>
#include <f32file.h>
#include "AudCommon.h"
#include "ProcConstants.h"
#include "ProcInFileHandler.h"


class CProcAWBInFileHandler: public CProcInFileHandler 
    {

public:

    static CProcAWBInFileHandler* NewL(const TDesC& aFileName, 
                                       RFile* aFileHandle,
                                       CAudClip* aClip, TInt aReadBufferSize,
                                       TInt aTargetSampleRate = 0, 
                                       TChannelMode aChannelMode = EAudSingleChannel);
                                       
    static CProcAWBInFileHandler* NewLC(const TDesC& aFileName, 
                                        RFile* aFileHandle,
                                        CAudClip* aClip, TInt aReadBufferSize,
                                        TInt aTargetSampleRate = 0, 
                                        TChannelMode aChannelMode = EAudSingleChannel);


    // From base class

    void GetPropertiesL(TAudFileProperties* aProperties);
    
    /**
    * Seeks a certain audio frame for reading
    *
    * Possible leave codes:  
    *    
    * @param aFrameIndex    frame index
    * @param aTime            time from the beginning of file in milliseconds
    *
    * @return    ETrue if successful
    *            EFalse if beyond the file
    */
    TBool SeekAudioFrame(TInt32 aTime);

    
    /**
    * Seeks a cut in audio frame for reading
    *
    * Possible leave codes:  
    *    
    *
    * @return    ETrue if successful
    *                
    */
    TBool SeekCutInFrame();
    
    virtual TBool SetNormalizingGainL(const CProcFrameHandler* aFrameHandler);

    virtual ~CProcAWBInFileHandler();


    

private:
    
    void ConstructL(const TDesC& aFileName, RFile* aFileHandle, CAudClip* aClip, 
                    TInt aReadBufferSize, TInt aTargetSampleRate = 0, 
                    TChannelMode aChannelMode = EAudSingleChannel);
                    
    TBool GetEncAudioFrameL(HBufC8*& aFrame, TInt& aSize, TInt32& aTime);
    
    // durations in milliseconds
    virtual TBool GetFrameInfo(TInt& aSongDuration,
                                TInt& aFrameAmount, 
                                TInt& aAverageFrameDuration, 
                                TInt& aAverageFrameSize);



protected:
    
    CProcAWBInFileHandler();
    
    };

#endif
