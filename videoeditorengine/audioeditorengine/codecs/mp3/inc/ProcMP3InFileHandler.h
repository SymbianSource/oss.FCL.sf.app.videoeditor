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


#ifndef __CPROCMP3INFILEHANDLER_H__
#define __CPROCMP3INFILEHANDLER_H__

/*-- System Headers. --*/
#include <e32base.h>
#include <f32file.h>

/*-- Project Headers. --*/
#include "AudCommon.h"
#include "ProcFrameHandler.h"
#include "ProcInFileHandler.h"
#include "Logfile.h"

#include "defines.h"

#include "Mp3API.h"

#include "ProcDecoder.h"
//#include "mpif.h"



/**
 * File handler class for accessing MP3 content.
 *
 * @author  Juha Ojanperä
 */
class CProcMP3InFileHandler: public CProcInFileHandler 
    {
public:

    /*
    * Constructors & destructor
    */

    static CProcMP3InFileHandler* NewL(const TDesC& aFileName, RFile* aFileHandle,
                                       CAudClip* aClip, TInt aReadBufferSize,
                                           TInt aTargetSampleRate = 0, 
                                           TChannelMode aChannelMode = EAudSingleChannel);

    static CProcMP3InFileHandler* NewLC(const TDesC& aFileName, RFile* aFileHandle,
                                        CAudClip* aClip, TInt aReadBufferSize,
                                            TInt aTargetSampleRate = 0, 
                                            TChannelMode aChannelMode = EAudSingleChannel);
                                        
    virtual ~CProcMP3InFileHandler();


    // From CProcInFileHandler ----------------->
    void GetPropertiesL(TAudFileProperties* aProperties);
    
    TBool SeekAudioFrame(TInt32 aTime);

    TBool SeekCutInFrame();
    
    virtual TBool GetDurationL(TInt32& aTime, TInt& aFrameAmount);

    virtual TBool SetNormalizingGainL(const CProcFrameHandler* aFrameHandler);

    //<-----------------------------------------


private:
    
    // constructL
    void ConstructL(const TDesC& aFileName, RFile* aFileHandle, CAudClip* aClip, 
                    TInt aReadBufferSize, TInt aTargetSampleRate = 0, 
                    TChannelMode aChannelMode = EAudSingleChannel);
    
    // C++ constructor
    CProcMP3InFileHandler();
    
    TBool GetEncAudioFrameL(HBufC8*& aFrame, TInt& aSize, TInt32& aTime);

    // gets one Mp3 frame
    int16 GetMP3Frame(uint8 *dataBytes, int16 bufSize, TMpFrameState *frState, uint8 syncStatus);

    // gets bitrate
    int16 GetMP3Bitrate(void);
    
    // check that file is valid mp3
    TBool Validate(TDes8& aDes);
    
    // seek for frame sync codes
    TInt SeekSync(TDes8& aDes, TInt aBufPos);
    
    // get info for a single frame
    TInt FrameInfo(const TUint8* aBuf,TInt aBufLen,TInt& aBitRate);
    
    // get ID3 header length
    TInt ID3HeaderLength(TDes8& aDes, TInt aPosition);

private:

    /*-- MP3 file format handle. --*/
    TMpTransportHandle *mp3FileFormat;

    /*-- Legal MP3 file? --*/
    uint8 isValidMP3;

    /*-- Input buffer; start of frame is seached from here. --*/
    uint8 *mp3HeaderBytes;
    
    CLogFile *mp3Log;
    
    /*-- Free format flag. --*/
    uint8 isFreeFormatMP3;
    
    
    // a handle to mp3 editing class in MP3AACManipLib    
    CMp3Edit* iMp3Edit;

    /*-- VBR bitrate flag; ETrue indicates VBR file, EFalse constant. --*/
    uint8 isVbrMP3;
    
    };

#endif /*-- __CPROCMP3INFILEHANDLER_H__ --*/
