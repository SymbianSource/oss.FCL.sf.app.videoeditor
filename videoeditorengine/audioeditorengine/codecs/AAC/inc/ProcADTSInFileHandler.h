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


#ifndef __CPROCADTSINFILEHANDLER_H__
#define __CPROCADTSINFILEHANDLER_H__

#include <e32base.h>
#include <f32file.h>
#include "AudCommon.h"
#include "ProcConstants.h"
#include "ProcInFileHandler.h"
#include "AACConstants.h"
#include "ProcDecoder.h"



class CProcADTSInFileHandler: public CProcInFileHandler 
    {

public:

    static CProcADTSInFileHandler* NewL(const TDesC& aFileName, 
                                        RFile* aFileHandle,
                                        CAudClip* aClip, TInt aReadBufferSize, 
                                        TInt aTargetSampleRate = 0,
                                        TChannelMode aChannelMode = EAudSingleChannel);
                                        
    static CProcADTSInFileHandler* NewLC(const TDesC& aFileName, 
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

    /**
    *
    * Sets the normalizing gain
    *
    * @param    aFrameHandler    AAC frame handler
    *
    * @return    ETrue if successful
    */
    TBool SetNormalizingGainL(const CProcFrameHandler* aFrameHandler);

    /*
    * Fills in aac info
    *
    * @param     aAACInfo    aac info
    *
    * @return    ETrue if successful
    */
    TBool GetInfoForFrameHandler(TAACFrameHandlerInfo& aAACInfo);


    /**
    * Generates decoder specific information for MP4/3GP files
    * 
    * The caller is responsible for releasing aDecSpecInfo 
    *
    * @param    aBytes    decoder specific info 
    * @param    aBufferSize        max size of decSpecInfo
    *
    * @return    ETrue  if info was generated
    *            EFalse if info was not generated (no need to release aFrame)
    */
    TBool ReadAudioDecoderSpecificInfoL(HBufC8*& aBytes, TInt aBufferSize);


    ~CProcADTSInFileHandler();


    

private:

    // constructL    
    void ConstructL(const TDesC& aFileName, RFile* aFileHandle, CAudClip* aClip, TInt aReadBufferSize,
                    TInt aTargetSampleRate = 0, TChannelMode aChannelMode = EAudSingleChannel);
                    
    virtual TBool GetEncAudioFrameL(HBufC8*& aFrame, TInt& aSize, TInt32& aTime);
    
    // C++ constructor
    CProcADTSInFileHandler();
    
    // Find the first AAC frame
    // There might be some stuff in the file before audio data (e.g. ID3 header)
    TBool FindFirstAACFrame();
    
    // calculates the frame length based of 4th, 5th and 6th byte of ADTS header
    TUint CalculateAACFrameLength(TUint8 byte4, TUint8 byte5, TUint8 byte6);

    // fills in fields of aProperties based on ADTS header
    TBool ReadHeaderInformation(TAudFileProperties* aProperties);

    // fills in the rest of the fields
    TBool ReadOtherInformationL(TAudFileProperties* aProperties);

private:

    // info for frame handler
    TAACFrameHandlerInfo* iFrameInfo;

    // MP4 decoder specific info
    HBufC8* iMP4DecSpecInfo;

    // offset of the first ADTS frame in a file
    TUint32 iFirstFrame;
    
    // flag to indicate whether the file can be parsed
    TBool iSupportedFile;

    // remainder if audio duration can't be handled accurately in TInt milliseconds, depends on sampling rate
    TReal iAACFrameLenRemainderMilli;
    
    };

#endif
