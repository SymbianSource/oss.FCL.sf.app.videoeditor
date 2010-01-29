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



#ifndef __CPROCOUTFILEHANDLER_H__
#define __CPROCOUTFILEHANDLER_H__

#include <e32base.h>
#include <f32file.h>
#include "AudCommon.h"
#include "ProcConstants.h"

class CProcOutFileHandler : public CBase
    {

public:

/*
    static CProcOutFileHandler* NewL(const TDesC& aFileName, 
                              TInt aWriteBufferSize,
                              TAudFileProperties aProperties);

    static CProcOutFileHandler* NewLC(const TDesC& aFileName, 
                              TInt aWriteBufferSize,
                              TAudFileProperties aProperties);
*/
    
    /**
    * Sets the output file properties
    *
    * @param    aProperties    properties of the output file
    */
    TBool SetPropertiesL(TAudFileProperties aProperties);

    void GetPropertiesL(const TAudFileProperties*& aProperties);

    /**
    * Initializes an output file
    * (writes file headers etc.)
    *
    * @return ETrue if successful
    *
    */
    virtual TBool InitializeFileL();
    
    /**
    * Finalizes an output file
    * Writes some file specific information if needed and
    * closes the file
    *
    */
    virtual TBool FinalizeFile();

    /**
    * Writes a new audio frame to an output file
    *
    * @param    aFrame    an audio frame to be written
    * @param    aSize    the number of bytes written
    *
    */
    virtual TBool WriteAudioFrameL(const HBufC8*& aFrame, TInt& aSize);

    /**
    * Writes a new audio frame to an output file
    *
    * @param    aFrame    an audio frame to be written
    * @param    aSize    the number of bytes written
    * @param    aDurationMilliSeconds frame duration in milliseconds
    *
    */
    virtual TBool WriteAudioFrameL(const HBufC8*& aFrame, TInt& aSize, TInt aDurationMilliSeconds);
    
    /**
    * Writes silent frames to current file position
    * 
    * @param    aTime    time (in milliseconds) of how much silence is to be written
    */
    virtual TBool WriteSilenceL(TInt32 aTime) = 0;
    virtual ~CProcOutFileHandler();

    
private:
    
protected:

    void ConstructL(const TDesC& aFileName, TInt aWriteBufferSize, TAudFileProperties aProperties);
    void InitL(const TDesC& aFileName, TInt aWriteBufferSize);
    TBool OpenFileForWritingL();
    TBool CloseFile();
    CProcOutFileHandler();

    TInt BufferedFileWriteL(const TDesC8& aDes,TInt aLength);
    TInt BufferedFileGetPos();
    TInt BufferedFileSetPos(TInt aPos);
    
    HBufC* iFileName;

    RFile iFile;
    RFs iFs;
    TBool iFileOpen;

    HBufC8* iWriteBuffer;

    TInt iWriteBufferSize;

    TAudFileProperties* iProperties;

    HBufC8* iSilentFrame;

    };

#endif
