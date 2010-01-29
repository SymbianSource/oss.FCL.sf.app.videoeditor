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



#ifndef __CPROCINFILEHANDLER_H__
#define __CPROCINFILEHANDLER_H__

#include <e32base.h>
#include <f32file.h>
#include "ProcFrameHandler.h"
#include "ProcWAVFrameHandler.h"

#include "AudCommon.h"
#include "ProcConstants.h"
#include "AudClip.h"
#include "ProcDecoder.h"

class CProcInFileHandler : public CBase
    {

public:

    /*
    * Destructor
    */
    virtual ~CProcInFileHandler();


    /**
    * Gets properties of this input clip
    *
    * @param aProperties    audio clip properties. Needs to be allocated by the caller
    *                        and is filled in by this function                        
    *
    */
    virtual void GetPropertiesL(TAudFileProperties* aProperties) = 0;

    /*
    *
    * Is decoding required?
    */

    TBool DecodingRequired();

    /*
    *
    * Returns the size of a decoded frame
    */   
    TInt GetDecodedFrameSize();

    /*
    *
    * Sets whether decoding is required?
    */

    void SetDecodingRequired(TBool aDecodingRequired);

    /**
    * Reads the next audio frame
    * This function allocates memory and
    * the caller is responsible for releasing it
    *
    * Possible leave codes:
    *
    *
    * @param    aFrame        audio frame
    * @param    aSize        size of the retrieved audio frame
    * @param    aTime    duration of the returned frame in milliseconds
    * @return    ETrue  if a frame was read
    *            EFalse if frame was not read (EOF)
    */
    TBool GetAudioFrameL(HBufC8*& aFrame, TInt& aSize, TInt32& aTime, TBool& aRawFrame);
    
    TBool GetRawAudioFrameL(HBufC8*& aFrame, TInt& aSize, TInt32& aTime);


    /**
    * Sets properties of this in file handler
    *
    * @param aProperties    audio clip properties                        
    *
    */
    
    TBool SetPropertiesL(TAudFileProperties aProperties);
    
    /*
    * Set the size of raw audio frames
    *
    * @param    aSize audio size
    *
    * @return   ETrue if successful, EFalse otherwise
    */
    
    TBool SetRawAudioFrameSize(TInt aSize);
    
    
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
    virtual TBool SeekAudioFrame(TInt32 aTime) = 0;


    /**
    * Seeks a cut in audio frame for reading
    *
    * Possible leave codes:  
    *    
    *
    * @return    ETrue if successful
    *                
    */
    virtual TBool SeekCutInFrame() = 0;
    

    //virtual TBool GetAudioFrameL(HBufC8*& aFrame, TInt& aSize, TInt32& aTime, TBool& aRawFrame) = 0;
    
    
    /**
    * Generates and returns a silent audio frame
    * This function allocates memory and
    * the caller is responsible for releasing it.
    * The silent frame is generated according to audio
    * properties of this input clip 
    * (e.g. sampling rate, channel configuration, bitrate etc)
    *
    * Possible leave codes: , at least <code>KErrNotMemory</code>
    *
    *
    * @param    aFrame        audio frame
    * @param    aSize        size of the silent audio frame
    * @param    aDuration    duration of the returned silent frame in milliseconds
    * @return    ETrue  if a frame was generated
    *            EFalse if frame was not generated (EOF, no need to release aFrame)
    */
    virtual TBool GetSilentAudioFrameL(HBufC8*& aFrame, TInt& aSize, TInt32& aDuration,  TBool& aRawFrame);
    


    /**
    * Gets the priority of this clip
    *
    * @return    priority of this clip
    *
    */
    TInt Priority() const;

    /**
    * Gets the current time of the frame reading
    *
    * @return    current time in milliseconds
    *
    */
    TInt32 GetCurrentTimeMilliseconds();

    /**
    * Gets the normalizing margin of this clip
    *
    * @return    normalizing margin
    *
    */
    TInt8 NormalizingMargin() const;

    /**
    *
    * Set priority of this clip
    * 
    * @param aPriority priority >= 0
    *
    * @return ETrue if priority >= 0 
    *          EFalse otherwise, priority not set
    */
    TBool SetPriority(TInt aPriority);
    
    /*
    * Sets the normalizing gain of this clip
    *
    * @param    aFrameHandler    frame handler
    *
    * @return ETrue if successful
    */
    virtual TBool SetNormalizingGainL(const CProcFrameHandler* aFrameHandler) = 0;

    /**
    * Gets ReadAudioDecoderSpecificInfo from file (if any)
    *
    * @param    aBytes            buffer
    * @param    aBufferSize        maximum size of buffer
    *
    * @return    ETrue if bytes were read (The caller must release aBytes!!)
    *
    *            EFalse if no bytes were read (no memory releasing needed)         
    *
    */

    virtual TBool ReadAudioDecoderSpecificInfoL(HBufC8*& aBytes, TInt aBufferSize);


protected:
    
    // constructL
    void ConstructL(const TDesC& aFileName);

    // c++ constructor
    CProcInFileHandler();
    
    
    /**
    * Reads the next raw frame
    * This function allocates memory and
    * the caller is responsible for releasing it
    *
    * Possible leave codes:
    *
    *
    * @param    aFrame        audio frame
    * @param    aSize        size of the retrieved audio frame
    * @param    aTime    duration of the returned frame in milliseconds
    * @return    ETrue  if a frame was read
    *            EFalse if frame was not read (EOF)
    */
    
    virtual TBool GetRawSilentAudioFrameL(HBufC8*& aFrame, TInt& aSize, TInt32& aDuration);

    virtual TBool GetEncAudioFrameL(HBufC8*& aFrame, TInt& aSize, TInt32& aTime) = 0;
    
    virtual TBool GetEncSilentAudioFrameL(HBufC8*& aFrame, TInt& aSize, TInt32& aDuration);
    
    TBool GetOneRawAudioFrameL(HBufC8*& aFrame, TInt& aSize, TInt32& aDuration);
    
    
    /**
    * Performs all the necessary initializations and memory allocations needed.
    * Should be always called by classes inherited from <code>CProcInFileHander</code>
    *
    * @param    aFileName            name of the file
    * @param    aCutInTime            cut in time in milliseconds
    * @param    aReadBufferSize        read buffer size
    *
    * @return    ETrue if successful
    *
    */
    TBool InitAndOpenFileL(const TDesC& aFileName, RFile* aFileHandle, TInt aReadBufferSize);

    
    /**
    * Performs all the necessary resource releasing and file closing
    *
    * should be called if <code>InitAndOpenFileL</code> has been called
    *
    * @return    ETrue if successful
    *
    */
    void ResetAndCloseFile();
    
    // opens a file for readind
    TBool OpenFileForReadingL();
    
    // closes the file if open
    TBool CloseFile();
    
    
    // File reading methods------------------>
    TInt BufferedFileRead(TDes8& aDes,TInt aLength);
    TInt BufferedFileSetFilePos(TInt aPos);
    TInt BufferedFileGetFilePos();
    TInt BufferedFileGetSize();
    TInt BufferedFileRead(TInt aPos,TDes8& aDes);
    TInt BufferedFileRead(TDes8& aDes);
    TInt BufferedFileReadChar(TInt aPos, TUint8& aChar);
    // <------------------ File reading methods
    
    
    TBool ManipulateGainL(HBufC8*& aFrameIn); 
    
    // function for gain manipulation
    TInt8 GetGainNow();
    
    TBool WriteDataToInputBufferL(const TDesC8& aData);
    
protected:

    // is inputfile open?
    TBool iFileOpen;
    
    // file name
    HBufC* iFileName;

    // RFile
    RFile iFile;
    
    // file server session
    RFs iFs;

    // read buffer
    HBufC8* iReadBuffer;

    // size of the read buffer
    TInt iReadBufferSize;
    
    // start offset of the read buffer
    TInt iBufferStartOffset;
    
    // end offset of the read buffer
    TInt iBufferEndOffset;
    
    // current file position
    TInt iFilePos;
    
    // cut in time in milliseconds
    TInt32 iCutInTime;
    
    // current read time in milliseconds
    TInt32 iCurrentTimeMilliseconds;

    // priority of the clip
    TInt iPriority;
    
    // normalizing margin in dB/2
    TInt8 iNormalizingMargin;

    // audio file properties
    TAudFileProperties* iProperties;
    
    // silent frame    
    HBufC8* iSilentFrame;
    
    // duration of the silent frame
    TInt32 iSilentFrameDuration;
    
    // raw silent frame    
    HBufC8* iRawSilentFrame;
    
    // duration of the raw silent frame
    TInt iRawSilentFrameDuration;
   
    // if true, this object opens and closes the input file    
    TBool iOwnsFile;
        
    TInt iTargetSampleRate; 
    
    TChannelMode iChannelMode;
    
    CProcFrameHandler* iFrameHandler;
    
    CAudClip* iClip;
    
    TBool iDecodingRequired;
    
    CProcDecoder* iDecoder;
    
    TBool iDecodingPossible;

    // We need a temporary storage for extra bytes 
    // when retrieving raw frames af equal length
    HBufC8* iInputBuffer;

    TInt iRawFrameLength;

    // wav frame handler for time domain gain manipulation
    CProcWAVFrameHandler* iWavFrameHandler;

    // remainder if audio duration can't be handled accurately in TInt milliseconds, depends on sampling rate
    TReal iFrameLenRemainderMilli;
    
    // Counter for decoder errors. Try to continue after one error, but if there are more, stop decoding.
    TInt iDecoderErrors;
    };

#endif
