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



#ifndef MP3MIXAPI__H
#define MP3MIXAPI__H

#include "ProcInFileHandler.h"
#include "ProcOutFileHandler.h"
#include "imdct2.h"
#include "editdef.h"


class CImdct2;

class CMp3Mix : public CBase
    {

public:

    IMPORT_C static CMp3Mix* NewL();
    IMPORT_C ~CMp3Mix();
    
    IMPORT_C TBool StartMixingL(CProcInFileHandler* aMP3InFileHandler1, 
                                TInt aStartPosMilli1,
                                CProcInFileHandler* aMP3InFileHandler2,
                                TInt aStartPosMilli2,
                                CProcOutFileHandler* aMP3OutFileHandler,
                                TInt aBitrateShort,
                                TInt aBitrateLong,
                                   TInt& aMixingDuration);

    IMPORT_C TBool MixFrameL(HBufC8* aMP3InBuffer1, HBufC8* aMP3InBuffer2, TInt& aFramesProcessed);

    IMPORT_C TBool StopMixing();


    static TBool WriteL(TDesC& aMessage);
    TBool WriteFloatsL(FLOAT* aArray, TInt aLen) const;
    TBool WriteIntsL(TInt16* aArray, TInt aLen);



private:

    void ConstructL();
    CMp3Mix();

    CProcInFileHandler* iIn1;
    CProcInFileHandler* iIn2;
    CProcOutFileHandler* iOut1;
    TAudioMixerInputInfo* iInfo;
    CEditorAPIHandle* iEditorAPIHandle1;
    CEditorAPIHandle* iEditorAPIHandle2;
    TUint iFrameCount1;
    TUint iFrameCount2;

    CEditorChunk* iChunk1;
    CEditorChunk* iChunk2;


    TUint iFramesMixed;

    CL3MixerHelper* iL3Mix;
    CImdct2* imdct2; 

    TInt iMixingDuration;

    RFile* iFile;
    RFs* iFs;
    TBool iFileOpened;
    HBufC8* iWriteBuffer; 


    };

#endif
