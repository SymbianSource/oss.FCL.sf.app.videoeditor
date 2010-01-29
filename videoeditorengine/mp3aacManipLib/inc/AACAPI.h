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



#ifndef _MP3AACMANIPLIB_H_
#define _MP3AACMANIPLIB_H_

#include "tool.h"
#include "sbr_codec.h"

/*
  Purpose:     AAC decoder core.
  Explanation: - 

    Implemented in aacaud.cpp
*/

class CAACAudDec : public CBase
{
    
public:
    
    static CAACAudDec* NewL(int16 aNumCh, int16 aNumCCh);
    ~CAACAudDec();
    
    void GetFIL(TBitStream *bs, uint32 prevEleID);
    int32 extension_payload(TBitStream *bs, int32 cnt, uint32 prevEleID);
    
    int16 numCh;
    int16 numCCh;
    int16 numOutCh;
    int16 samplesPerFrame;
    
    CToolInfo **tool;
    CMC_Info* mc_info;
    CCInfo **ccInfo;
    CWindowInfo **winInfo;
    
    /*-- Huffman tables. --*/
    Huffman_DecInfo **huf;
    Huffman_DecSfInfo *sf_huf;
    
    SbrBitStream *sbrStream;
    SBR_Decoder *sbrDecInfo;
    
private:
    void ConstructL(int16 aNumCh, int16 aNumCCh);
    CAACAudDec();
    
    TInt windowAmount;
    
};

/*
*    Methods for AAC gain manipulation -------------->
*/

/*-- Closes AAC decoder handle. --*/
IMPORT_C CAACAudDec *
DeleteAACAudDec(CAACAudDec *aac);

/*-- Creates AAC decoder handle. --*/
IMPORT_C void
CreateAACAudDecL(CAACAudDec*& aDecHandle, int16 numCh, int16 numCCh);


/*-- Creates (e)AAC+ decoder handle. --*/
IMPORT_C uint8
CreateAACPlusAudDecL(CAACAudDec *aDecHandle, int16 sampleRateIdx, uint8 isStereo, uint8 isDualMono);


/*-- Initializes AAC decoder handle. --*/
IMPORT_C void
InitAACAudDec(CAACAudDec *aac, int16 profile, int16 sampleRateIdx, uint8 is960);

/*-- Resets AAC decoder handle. --*/
IMPORT_C void
ResetAACAudDec(CAACAudDec *aac);

/*-- Counts AAC frame length (and parses the frame at same cost). --*/
IMPORT_C int16
CountAACChunkLength(TBitStream *bs, CAACAudDec *aac, int16 *bytesInFrame);

/*-- Stores global gain values. --*/
IMPORT_C void
SetAACGlobalGains(TBitStream* bs,
                  uint8 numGains, uint8 *globalGain, uint32 *globalGainPos);


IMPORT_C void
SetAACPlusGlobalGains(TBitStream* bs, TBitStream* bsOut, CAACAudDec *aac, int16 gainChangeValue,
                  uint8 numGains, uint8 *globalGain, uint32 *globalGainPos);


/*-- Extracts global gain values. --*/
IMPORT_C uint8
GetAACGlobalGains(TBitStream* bs, CAACAudDec *aac, uint8 nBufs, uint8 *globalGain, uint32 *globalGainPos);

/**
 *
 * Retrieves decoder specific information for the
 * specified AAC encoder configuration. This information should be
 * placed in the MP4/3GP header to describe the encoder
 * parameters for the AAC decoder at the receiving end.
 *
 * @param aacInfo     Handle to AAC configuration information
 * @param pBuf        Pointer to decoder specific information buffer
 * @param nBytesInBuf Size of 'pBuf'
 *
 * @return          Number of bytes in 'pBuf'
 */
IMPORT_C int16
AACGetMP4ConfigInfo(int32 sampleRate, uint8 profile, uint8 nChannels, int16 frameLen,
                    uint8 *pBuf, uint8 nBytesInBuf);


/*-- Checks whether parametric stereo enabled in the AAC bitstream. --*/
IMPORT_C uint8
IsAACParametricStereoEnabled(CAACAudDec *aac);



/*-- Checks whether SBR enabled in the AAC bitstream. --*/
IMPORT_C uint8
IsAACSBREnabled(CAACAudDec *aac);


/*
* <-------    Methods for AAC gain manipulation
*/

#endif
