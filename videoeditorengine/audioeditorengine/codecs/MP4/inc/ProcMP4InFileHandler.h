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


#ifndef __CPROCMP4INFILEHANDLER_H__
#define __CPROCMP4INFILEHANDLER_H__

#include <e32base.h>
#include <f32file.h>
#include "mp4lib.h"

#include "AudCommon.h"
#include "ProcConstants.h"
#include "ProcInFileHandler.h"
//#include "mp4aud.h"
#include "AACConstants.h"

#include "ProcDecoder.h"


class CMP4Parser;

class CProcMP4InFileHandler: public CProcInFileHandler 
    {

public:

    /*
    *
    * Constructors & destructor
    * 
    */

    static CProcMP4InFileHandler* NewL(const TDesC& aFileName, RFile* aFileHandle,
                                        CAudClip* aClip, TInt aReadBufferSize,
                                            TInt aTargetSampleRate = 0, 
                                            TChannelMode aChannelMode = EAudSingleChannel);
                                            
    static CProcMP4InFileHandler* NewLC(const TDesC& aFileName,  RFile* aFileHandle,
                                        CAudClip* aClip, TInt aReadBufferSize,
                                            TInt aTargetSampleRate = 0, 
                                            TChannelMode aChannelMode = EAudSingleChannel);
    virtual ~CProcMP4InFileHandler();

    // From base class --------->

    void GetPropertiesL(TAudFileProperties* aProperties);
    
    /**
    * Seeks a certain audio frame for reading
    *
    * Possible leave codes:  
    *    
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

    // <-------------------------------------- from base class

    /*
    *
    * Used to retrieve information for frame handler
    *
    * @param    aBytes (output) decoder specific info
    * @param     aBufferSize        maximum allowed aBytes
    *
    * @return    ETrue     if successful (memory reserved)
    *            EFalse    if not successful (no memory reserved)
    *
    * NOTE: The caller is responsible for releasing the memory!
    */
    virtual TBool ReadAudioDecoderSpecificInfoL(HBufC8*& aBytes, TInt aBufferSize);
    
    /*
    * Reads the time scale of the MP4 file
    *
    * @param    aTimescale    timescale
    *
    */
    virtual TBool ReadTimeScale(mp4_u32& aTimescale);

    /*
    * Reads the audio description of the MP4 file
    *
    * @param    aAudiolength            length of the clip
    * @param    aAudiotype                type of the clip
    * @param    aFramespersample        number of frames in sample
    * @param    aTimescale                timescale
    * @param    aAveragebitrate            average bitrate
    * 
    */
    MP4Err ParseRequestAudioDescription(mp4_u32 *aAudiolength,
                                           mp4_u32 *aAudiotype,
                                           mp4_u8 *aFramespersample,
                                           mp4_u32 *aTimescale,
                                           mp4_u32 *aAveragebitrate);

    /*
    *
    * Returns AAC info
    * @param    aAACInfo    AAC info
    *
    */
    TBool GetInfoForFrameHandler(TAACFrameHandlerInfo& aAACInfo);

    


    

private:

    // constructL
    void ConstructL(const TDesC& aFileName,  RFile* aFileHandle, CAudClip* aClip, 
                    TInt aReadBufferSize, TInt aTargetSampleRate = 0, 
                    TChannelMode aChannelMode = EAudSingleChannel);
    
    // c++ construtor
    CProcMP4InFileHandler();

    // gets audio frame info
    TBool GetAudioFrameInfoL(TInt& aFrameAmount, TInt& aAverageFrameDuration, TInt& aAverageFrameSize,
                            TAudFileProperties* aProperties);

    TBool GetEncAudioFrameL(HBufC8*& aFrame, TInt& aSize, TInt32& aTime);
    
    /*
    * Reads one amr frame from iReadBuffer and updates iReadBufferPos
    *
    * @param aOneAMRFrame        amr frame in return
    *
    * @return                     ETrue if successful, the caller must release the memory
    *                            EFalse if frame can not be read, no need to release memory
    */
    TBool ReadOneAMRFrameL(HBufC8*& aOneAMRFrame);
    
    TBool ReadOneAWBFrameL(HBufC8*& aOneAWBFrame);
    
private:

    // Handle to MP4Lib
    MP4Handle iParser;
    
    // timestamp of a previous frame
    TInt32 iLastTimeStamp;

    // information for frame handler
    TAACFrameHandlerInfo* iFrameInfo;

    // read buffer
    mp4_u8 *iMP4ReadBuffer;
    // read buffer size
    TInt iMP4ReadBufferSize;
    // position of the read buffer
    TInt iMP4ReadBufferPos;
    



protected:
    
    
    
    };

#endif
