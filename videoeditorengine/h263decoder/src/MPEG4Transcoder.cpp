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
* Implementation for MPEG4(H263) video transcoder. Operations may include:
* 1. Bitstream Copying 
* 2. BlackAndWhite Effect
* 3. MPEG4 to VDT_RESYN (Simple profile)
* 4. H.263 -> MPEG4 (Open Loop structure, no MV refinement)
* 5. MPEG4 -> H263, partial implemented 
*
* We call functions 2~5 as transcoding functions since they involve MB level data processing
*    
* Note:
* When RVLC is used and errors occur during  forward decoding, 
* We don't do backward transcoding, the rest of the data is discarded, 
*
*/



/* 
* Includes
*/
#include "MPEG4Transcoder.h"
#include "debug.h"

/* Print macro */
#ifdef _DEBUG
#include <e32svr.h>
#define PRINT(x) RDebug::Print x
#else
#define PRINT(x)
#endif

/* 
* Defines and Typedefs
*/
typedef unsigned int         uint32;

#define EInternalAssertionFailure 1000
#define VDT_NO_DATA(a, b, c, d) ((a) == (c) && (b) == (d))

const int KDataNotValid = -1; 
const int KMpeg4VopTimeIncrementResolutionLength = 16; 
const int KOutputMpeg4TimeIncResolution = 30000;
const int KShortHeaderMpeg4VosSize = 14; 
const int KH263ToMpeg4VosSize = 28; 



/*Bit stream formating*/
#define WRITE32(op, x) sPutBits((op), 16, ((uint32)(x)) >> 16);       \
sPutBits((op), 16, (x) & 0x0000ffff)

/* 
* Constants 
*/
#ifdef _DEBUG
const TUint KInitialBufferSize = 200000; /* MPEG4 Simple Visual Profile (Levels 0,1,2,3) initial frame data buffer size vga support */
#endif
                                                                                /* 
                                                                                * Function Declarations 
*/

//Static Functions 

static void sStuffBitsH263(bibBuffer_t *outBuffer);
static TVedVideoBitstreamMode sGetMPEG4Mode(int error_resilience_disable, int dp, int rvlc);
static int sFindCBP(int *mbdata, int fUseIntraDCVLC);



void vdtPutInterMBCMT(bibBuffer_t *outBuffer, int coeffStart, int *coeff, int *numTextureBits, int svh);
void vdtPutIntraMBCMT(bibBuffer_t *outBuffer, int *coeff, int *numTextureBits, int index,  int skipDC, int skipAC);
void vbmEncodeMVDifferential(int32 mvdx, int32 mvdy, int32 fCode, bibBuffer_t *outBuffer);
void vbmGetH263IMCBPC(int lDQuant, int vopCodingType, int colorEffect, int cbpy, int& mcbpcVal, int& len);
void vbmGetH263PMCBPC(int lDQuant, int colorEffect, int cbpy, int& mcbpcVal, int& len);
tBool vbmMVOutsideBound(tMBPosition *mbPos, tMotionVector* bestMV, tBool halfPixel);
void vbmMvPrediction(tMBInfo *mbi, int32 mBCnt, tMotionVector *predMV,  int32 mbinWidth);
void vbmPutInterMB(tMBPosition* mbPos, bibBuffer_t *outBuf, dmdPParam_t *paramMB, tMotionVector *initPred, 
                                     int32 noOfPredictors, u_int32 vopWidth, u_int32 vopHeight, int32 searchRange, 
                                     int32 mbNo, int32* numTextureBits, int16 colorEffect, tMBInfo *mbsinfo);

void sPutBits (bibBuffer_t *buf, int numBits, unsigned int value);  //forward decl

/* 
* Function Definitions 
*/

/*
* sStuffBitsH263
*
* Parameters: 
*    outBuffer    output buffer
*
* Function:
*    This function stuffs bits for H.263 format
* Returns:
*    None
* Error codes:
*    None.
*
*/
static void sStuffBitsH263(bibBuffer_t *outBuffer)
{
    const int stuffingBits[8][2] = { {1,0}, {2,0}, {3,0}, {4,0}, {5,0}, {6,0}, {7,0}, {0,0} };
    
    VDTASSERT(outBuffer->baseAddr);
    
    /* find the number of stuffing bits to insert in the output buffer */
    int bi = outBuffer->bitIndex;
    int newNumBits = stuffingBits[bi][0]; 
    int newValue = stuffingBits[bi][1]; 
    sPutBits(outBuffer, newNumBits, newValue);
        
    return;
}



/*
* sGetMPEG4Mode
*
* Parameters: 
*    
*
* Function:
*    This function gets the mode of the MPEG-4 bitstream
* Returns:
*    Nothing
* Error codes:
*    None.
*
*/
static TVedVideoBitstreamMode sGetMPEG4Mode(int error_resilience_disable, int dp, int rvlc)
{
    TVedVideoBitstreamMode mode = EVedVideoBitstreamModeUnknown;
    int combination = ((!error_resilience_disable) << 2) | (dp << 1) | rvlc;
    switch (combination)
    {
        case 0:
            mode = EVedVideoBitstreamModeMPEG4Regular;
            break;
        case 2:
            mode = EVedVideoBitstreamModeMPEG4DP;
            break;
        case 3:
            mode = EVedVideoBitstreamModeMPEG4DP_RVLC;
            break;
        case 4:
            mode = EVedVideoBitstreamModeMPEG4Resyn;
            break;
        case 6:
            mode = EVedVideoBitstreamModeMPEG4Resyn_DP;
            break;
        case 7:
            mode = EVedVideoBitstreamModeMPEG4Resyn_DP_RVLC;
            break;
        default:
            mode = EVedVideoBitstreamModeUnknown;
    }
    
    return mode;
}


/* {{-output"vdtGetVideoBitstreamInfo.txt"}} */
/*
* sFindCBP
*
* Parameters: 
*    mbdata          Contains the actual MB data to be quantized
*    fUseIntraDCVLC  ON for INTRA, OFF for INTER
*
* Function:
*    This function finds the coded bit pattern of the MB 
* Returns:
*    Coded Block Pattern.
* Error codes:
*    None.
*
*/
static int sFindCBP(int *mbdata, int fUseIntraDCVLC)
{
    int coeffCnt;
    int *block;
    int blkCnt;
    int cbpFlag = 0;
    int codedBlockPattern = 0;

    for (blkCnt = 0; blkCnt < 6; blkCnt++)
    {   
        cbpFlag = 0;
        codedBlockPattern <<= 1;
        block = &mbdata[blkCnt * BLOCK_COEFF_SIZE];
        for (coeffCnt = fUseIntraDCVLC; coeffCnt < BLOCK_COEFF_SIZE;
             coeffCnt++)
             {
                 if (block[coeffCnt])
                 {
                     cbpFlag = 1;
                     codedBlockPattern |= cbpFlag;
                     break;
                 }
             }
    }
    
    return codedBlockPattern;
}   






/* {{-output"sPutBits.txt"}} */
/*
* sPutBits
*
* Parameters:
*          outBuf           output buffer
*           numBits         number of bits to output
*           value           new value
*
* Function:
*    This function puts some bits to the output buffer
* Returns:
*    Nothing.
* Error codes:
*    None.
*
*/
void sPutBits (bibBuffer_t *buf, int numBits, unsigned int value)
{
    
    bibEditParams_t edParam;
    
    edParam.curNumBits = edParam.newNumBits = numBits;
    edParam.StartByteIndex = edParam.StartBitIndex = 0; /* used for source buffer only  */
    edParam.newValue = value;                           /* use value 128, encoded as codeword "1111 1111" = 255 */
    
    CopyBufferEdit((bibBuffer_t*)NULL, buf, &edParam, 0); 
}



/*
* GetTimeIncPosition
*    
*
* Parameters:
*    hInstance                  instance handle
*
* Function:
*    Calculates the time increment position for the current VOP.
*
* Returns:
*    VDX error codes
*    
*/

int GetTimeIncPosition(
   bibBuffer_t *inBuffer,
   const vdxGetVopHeaderInputParam_t *inpParam,
   vdxVopHeader_t *header,
   int * ModuloByteIndex,
   int * ModuloBitIndex,
   int * ByteIndex,
   int * BitIndex,
   int *bitErrorIndication)
/* {{-output"vdxGetVopHeader.txt"}} */
{
  /* Get VOP header */
    int ret = vdxGetVopHeader(inBuffer, inpParam, header, 
        ModuloByteIndex, ModuloBitIndex, ByteIndex, BitIndex,
        bitErrorIndication);
    
    return ret;
    
}



/*
* CopyEditVop
*    
*
* Parameters:
*    hInstance                  instance handle
*
* Function:
*    This function copies the VOP header with edited time stamps of the VOP.
*
* Returns:
*    TX error codes
*/

int CopyEditVop(vdeHInstance_t hInstance, int aNrOfBytesToSkip, bibBuffer_t * inBuffer, 
                 vdeDecodeParamters_t *aDecoderInfo)
{
    PRINT((_L("CopyEditVop() begin")));
    int StartByteIndex = 0;
    int StartBitIndex = 7;
    int16 error;
    int sncCode;
    int timeIncByteIndex, timeIncBitIndex;
    int moduloBaseByteIndex, moduloBaseBitIndex;
    int vopheaderBitLeft;
    int numBitChange = 0;
    int increaseBytes = 0;
    int stuffingLength = 0;

    tMPEG4TimeParameter * timeStamp = aDecoderInfo->aMPEG4TimeStamp;
    MPEG4TimeParameter CurNewTimeCode;

    int outTirDecreased = 0;
    int outputTimeResolution = *aDecoderInfo->aMPEG4TargetTimeResolution;
    int numOutputTrBits;
    for (numOutputTrBits = 1; ((outputTimeResolution-1) >> numOutputTrBits) != 0; numOutputTrBits++)
        {
        }

    int num_bits;
    vdxGetVopHeaderInputParam_t inpParam;
    vdxVopHeader_t vopheader;
    int bitErrorIndication;
    vdeInstance_t * vdeTemp = (vdeInstance_t *)hInstance;
    bibBuffer_t *outBuffer = vdeTemp->outBuffer;
    bibBufferEdit_t * bufEdit = vdeTemp->bufEdit;
    
    int FrameSizeInByte = outBuffer->numBytesRead;
    bibRewindBits(bibNumberOfFlushedBits(inBuffer),inBuffer,&error);
    bibRewindBits(bibNumberOfFlushedBits(outBuffer),outBuffer,&error);
    if ( aNrOfBytesToSkip > 0 )
        {
        // VOS header is already in the beginning of the output buffer
        bibForwardBits(aNrOfBytesToSkip<<3, outBuffer);
        // need to also skip the VOS header from input
        }
    bufEdit->copyMode = CopyWhole; /* CopyWhole - default */

    /* record position */
    StartByteIndex = inBuffer->numBytesRead;
    StartBitIndex = inBuffer->bitIndex;

    /* get time increment resolution */
    vdcInstance_t * vdcTemp = (vdcInstance_t *)(vdeTemp->vdcHInstance);
    int currentTimeIncResolution = vdcTemp->pictureParam.time_increment_resolution > 0? vdcTemp->pictureParam.time_increment_resolution : outputTimeResolution;
    inpParam.time_increment_resolution = currentTimeIncResolution;

    /* find the next vop start code */
    do 
    {
        sncCode = sncSeekMPEGStartCode(inBuffer, vdcTemp->pictureParam.fcode_forward, vdcTemp->pictureParam.error_res_disable, 0, &error);
        bibForwardBits(32,inBuffer); // one start code is found, move on  
        if(inBuffer->bitsLeft <= 0) 
        {
            return TX_ERR;  // did not find any sync code --- vop is corrupted
        }
    }
    while (sncCode != SNC_VOP);
    bibRewindBits(32,inBuffer, &error);  // go back 

    // if we have VOS header in the input, we are now just after it. If aNrOfBytesToSkip > 0, we should not copy it
    // however, aNrOfBytesToSkip refers to output buffer, so we should not use it when skipping the input
    if ( aNrOfBytesToSkip > 0 )
        {
        StartByteIndex = inBuffer->numBytesRead;
        StartBitIndex = inBuffer->bitIndex;
        }

    /* read vop header */
    GetTimeIncPosition(inBuffer, &inpParam, &vopheader, 
        &moduloBaseByteIndex, &moduloBaseBitIndex, &timeIncByteIndex, 
        &timeIncBitIndex, &bitErrorIndication);

    /* record the header end; */
    vopheaderBitLeft = inBuffer->bitsLeft;

    /* copy-edit the part from the begin to the end of modulo base */
    CurNewTimeCode = *timeStamp;

    if (CurNewTimeCode.modulo_time_base != vopheader.time_base_incr)
    {
        if (!bufEdit->editParams)
        {
            bufEdit->editParams = (bibEditParams_t *) malloc(sizeof(bibEditParams_t));
            if(!bufEdit->editParams)
            {
                //Memory not available 
                return TX_ERR; //indicating error;
            }
            else
            {
                bufEdit->numChanges = 1;
            }
        }
        bufEdit->copyMode = CopyWithEdit; /* CopyWithEdit */
        bufEdit->editParams->curNumBits = vopheader.time_base_incr + 1;
        bufEdit->editParams->newNumBits = CurNewTimeCode.modulo_time_base + 1;
        bufEdit->editParams->newValue = ((1 << CurNewTimeCode.modulo_time_base) - 1) << 1;
        bufEdit->editParams->StartByteIndex = moduloBaseByteIndex;
        bufEdit->editParams->StartBitIndex = moduloBaseBitIndex;
        numBitChange += bufEdit->editParams->newNumBits - bufEdit->editParams->curNumBits;
    }
    else
    {
        bufEdit->copyMode = CopyWhole; /* CopyWithEdit */
    }
    /* set the end of copy point */
    bibRewindBits((inBuffer->numBytesRead<<3)+7-inBuffer->bitIndex,inBuffer,&error);
    bibForwardBits((moduloBaseByteIndex<<3)+7-moduloBaseBitIndex+vopheader.time_base_incr+2, inBuffer);
      
    /* copy data */
    CopyStream(inBuffer,outBuffer,bufEdit,StartByteIndex,StartBitIndex);
    StartByteIndex=inBuffer->getIndex;
    StartBitIndex=inBuffer->bitIndex;
    bufEdit->copyMode = CopyWhole; /* CopyWhole */

    /* copy and edit until the end of Vop header */
    if (currentTimeIncResolution != outputTimeResolution /* && volheader.time_increment_resolution != 30 */
        || vopheader.time_inc != CurNewTimeCode.time_inc)
    {
        for (num_bits = 1; ((currentTimeIncResolution-1) >> num_bits) != 0; num_bits++) 
            {
            }
        /* prepare editing position */
        if (!bufEdit->editParams)
        {
            bufEdit->editParams = (bibEditParams_t *) malloc(sizeof(bibEditParams_t));
            if(!bufEdit->editParams)
            {
                //Memory not available 
                return TX_ERR; //indicating error no memory;
            }
            else
            {
                bufEdit->numChanges = 1;
            }
            
        }
        bufEdit->copyMode = CopyWithEdit; /* CopyWithEdit */
        bufEdit->editParams->StartByteIndex = timeIncByteIndex;
        bufEdit->editParams->StartBitIndex = timeIncBitIndex;
        bufEdit->editParams->curNumBits = num_bits;
        bufEdit->editParams->newNumBits = numOutputTrBits;
        bufEdit->editParams->newValue = vopheader.time_inc;

        /* update time increment */
        if (vopheader.time_inc != CurNewTimeCode.time_inc)
        {
            /*bufEdit->editParams->newValue = (int)(vopheader.time_inc * (float)outputTimeResolution /
            (float)currentTimeIncResolution + 0.5); //CurNewTimeCode.time_inc;
            */
            bufEdit->editParams->newValue = CurNewTimeCode.time_inc;
        }
        numBitChange += bufEdit->editParams->newNumBits - bufEdit->editParams->curNumBits;
    }

    /* set the copy end point to the end of vop header */
    bibForwardBits(inBuffer->bitsLeft - vopheaderBitLeft, inBuffer);

    /* copy data */
    CopyStream(inBuffer,outBuffer,bufEdit,StartByteIndex,StartBitIndex);
    StartByteIndex=inBuffer->getIndex;
    StartBitIndex=inBuffer->bitIndex;
    
    if ( vdcTemp->pictureParam.error_res_disable && (inBuffer->bitsLeft > 40) )   // 40 as below to avoid negative values below
        {
        // there are no VP headers => no need to search for VP start codes => can jump to the end.
        // 40 = 5 bytes; in the end there is a start code that has 4 bytes
        bibForwardBits( inBuffer->bitsLeft-40 ,inBuffer);
        }

    PRINT((_L("CopyEditVop() seek sync")));
    /* find the next resync marker */
    sncCode = sncSeekMPEGStartCode(inBuffer, vopheader.fcode_forward, vdcTemp->pictureParam.error_res_disable, 0, &error);
    if ( sncCode == SNC_NO_SYNC )
        {
        PRINT((_L("CopyEditVop() sync NOT found, interrupt the copying and return")));
        return TX_ERR;
        }
    PRINT((_L("CopyEditVop() sync found")));

    /* record next resync position */
    int resyncBitsLeft = inBuffer->bitsLeft;
        
    /* rewind stuffing bits */
    sncRewindStuffing(inBuffer, &error);        
    
    if (!bufEdit->editParams)
    {
        bufEdit->editParams = (bibEditParams_t *) malloc(sizeof(bibEditParams_t));
        if(!bufEdit->editParams)
        {
            //Memory not available 
            return TX_ERR; //indicating error no memory;
        }
        else
        {
            bufEdit->numChanges = 1;
        }
    }

    /* record position */
    bufEdit->editParams->StartByteIndex = inBuffer->getIndex;
    bufEdit->editParams->StartBitIndex = inBuffer->bitIndex;

    /* calculate new stuffing bits */
    bufEdit->editParams->curNumBits = inBuffer->bitsLeft - resyncBitsLeft;
    /* calculate number of bits/bytes changed */
    if (numBitChange<0)
    {
        outTirDecreased = 1;
        numBitChange = -numBitChange;
    }
    int numByteChange = numBitChange >> 3;

    if (outTirDecreased)
    {
        increaseBytes -= numByteChange;
        stuffingLength = bufEdit->editParams->curNumBits + (numBitChange - (numByteChange << 3));
        if (stuffingLength > 8)
        {
            stuffingLength -= 8;
            increaseBytes --;
        }
    }
    else
    {
        increaseBytes += numByteChange;
        stuffingLength = bufEdit->editParams->curNumBits - (numBitChange - (numByteChange << 3));
        if (stuffingLength <= 0)
        {
            stuffingLength += 8;
            increaseBytes ++;
        }
    }
        
    /* adjust the output buffer size */
    if (increaseBytes != 0)
    {
        outBuffer->size += increaseBytes;
        if (increaseBytes>=0)
        {
            outBuffer->bitsLeft += (increaseBytes << 3);
        }
        else
        {
            outBuffer->bitsLeft -= ((-increaseBytes) << 3);
        }
    }

    /* update edit statistics */
    bufEdit->editParams->newNumBits = stuffingLength;
    bufEdit->editParams->newValue = stuffingLength>0?(1<<(stuffingLength-1))-1:0;
    if (stuffingLength != bufEdit->editParams->curNumBits)
    {
        bufEdit->copyMode = CopyWithEdit;
    }

    bibForwardBits(bufEdit->editParams->curNumBits, inBuffer);
    /* copy data */
    CopyStream(inBuffer,outBuffer,bufEdit,StartByteIndex,StartBitIndex);
    /* record position */
    StartByteIndex = inBuffer->getIndex;
    StartBitIndex = inBuffer->bitIndex;
        
    do
    {
        /* if it is the start of a video packet, copy it with edit */
        if (sncCode == SNC_VIDPACK )
        {
            /* copy video packet with edit */
            int retVal = CopyEditVideoPacket(inBuffer, outBuffer, bufEdit, vdcTemp, aDecoderInfo, &vopheader, 
                &sncCode, &StartByteIndex, &StartBitIndex);
            if(retVal<0)
            {
                //error inside function return error
                return TX_ERR;
            }
        }
        else if (sncCode == SNC_EOB || sncCode == SNC_EOS || sncCode == SNC_GOV)
        {
            // since it is EOB, so end of sequence has occurred, so no more data, so exit       
            break;
        }
    } 
    while(sncCode != SNC_VOP);

    /* copy the rest of bits */
    if ((int)inBuffer->numBytesRead < FrameSizeInByte)
    {
        bufEdit->copyMode = CopyWhole;
        bibForwardBits((FrameSizeInByte-StartByteIndex)<<3, inBuffer);
        CopyStream(inBuffer,outBuffer,bufEdit,StartByteIndex,StartBitIndex);
    }

    PRINT((_L("CopyEditVop() end")));
    return TX_OK;
    }


/*
* CopyEditVideoPacket
*    
*
* Parameters:
*
* Function:
*    This function copies the video packet with edited time stamps, if HEC is enabled
*
* Returns:
*    TX error codes
*/

int CopyEditVideoPacket(bibBuffer_t* aInBuffer, 
                        bibBuffer_t* aOutBuffer, 
                        bibBufferEdit_t* aBufEdit, 
                        vdcInstance_t * aVdcTemp,
                        vdeDecodeParamters_t* aDecoderInfo, 
                        vdxVopHeader_t* aVopheader, 
                        int* aSncCode, 
                        int* aStartByteIndex, 
                        int* aStartBitIndex)
{
    int16 error = 0;
    int value = 0;
    int bitsGot = 0;
    int num_bits = 0;
    int *bitErrorIndication = 0;
    int numOutputTrBits = 0;
    int resyncMarkerLength = 0;
    int numMBs = 0;
    int mbNumberLength = 0;
    int startByteIndex = *aStartByteIndex;
    int startBitIndex  = *aStartBitIndex;
    int currentTimeIncResolution = 0;
    int outputTimeResolution = 0;
    MPEG4TimeParameter curNewTimeCode;
    MPEG4TimeParameter* timeStamp = aDecoderInfo->aMPEG4TimeStamp;

    int numBitChange = 0;
    int increaseBytes = 0;
    int stuffingLength = 0;
    int outTirDecreased = 0;

    currentTimeIncResolution = aVdcTemp->pictureParam.time_increment_resolution > 0? aVdcTemp->pictureParam.time_increment_resolution : 30000;
    outputTimeResolution = *aDecoderInfo->aMPEG4TargetTimeResolution;
    for (numOutputTrBits=1; ((outputTimeResolution-1)>>numOutputTrBits)!=0; numOutputTrBits++)
        {
        }
    /* evaluate resync marker length */
    resyncMarkerLength = (aVopheader->coding_type == 0 ? 17 : 16+aVopheader->fcode_forward);
    /* evaluate MB number length */
    numMBs = ((aVdcTemp->pictureParam.lumWidth+15)>>4) * ((aVdcTemp->pictureParam.lumHeight+15)>>4);
    for (mbNumberLength = 1; ((numMBs-1) >> mbNumberLength) != 0; mbNumberLength++)
        {
        }

    value = bibGetBits(resyncMarkerLength, aInBuffer, &bitsGot, bitErrorIndication, &error);    // resync marker
    value = bibGetBits(mbNumberLength, aInBuffer, &bitsGot, bitErrorIndication, &error);    // mb number
    value = bibGetBits(5, aInBuffer, &bitsGot, bitErrorIndication, &error); // quant scale
    value = bibGetBits(1, aInBuffer, &bitsGot, bitErrorIndication, &error); // header extension code
    
    /* if HEC enabled, copy edit time increment fields in the video packet */
    if (value == 1) 
    {
        /* copy-edit the part from the begin to the end of modulo base */
        curNewTimeCode = *timeStamp;
        
        if (curNewTimeCode.modulo_time_base != aVopheader->time_base_incr)
        {
            if (!aBufEdit->editParams)
            {
                aBufEdit->editParams = (bibEditParams_t *) malloc(sizeof(bibEditParams_t));
                if(!aBufEdit->editParams)
                {
                    //Memory not available 
                    return TX_ERR; //indicating error no memory;
                }
                else
                {
                    aBufEdit->numChanges = 1;
                }
            }
            aBufEdit->copyMode = CopyWithEdit; /* CopyWithEdit */
            aBufEdit->editParams->curNumBits = aVopheader->time_base_incr + 1;
            aBufEdit->editParams->newNumBits = curNewTimeCode.modulo_time_base + 1;
            aBufEdit->editParams->newValue = ((1 << curNewTimeCode.modulo_time_base) - 1) << 1;
            aBufEdit->editParams->StartByteIndex = aInBuffer->getIndex;
            aBufEdit->editParams->StartBitIndex = aInBuffer->bitIndex;
            numBitChange += aBufEdit->editParams->newNumBits - aBufEdit->editParams->curNumBits;
        } 
        else
        {
            aBufEdit->copyMode = CopyWhole; /* CopyWithEdit */
        }
        /* set the end of copy point */
        bibForwardBits(aVopheader->time_base_incr+2, aInBuffer); // includes one bit for Marker
        
        /* copy data */
        CopyStream(aInBuffer,aOutBuffer,aBufEdit,startByteIndex,startBitIndex);
        startByteIndex = aInBuffer->getIndex;
        startBitIndex  = aInBuffer->bitIndex;
        aBufEdit->copyMode = CopyWhole; /* CopyWhole */
        
        /* copy and edit 'time increment' field */
        if (currentTimeIncResolution != outputTimeResolution /* && volheader.time_increment_resolution != 30 */
            || aVopheader->time_inc != curNewTimeCode.time_inc)
        {
            for (num_bits = 1; ((currentTimeIncResolution-1) >> num_bits) != 0; num_bits++)
                {
                }
            
            /* prepare editing position */
            if (!aBufEdit->editParams)
            {
                aBufEdit->editParams = (bibEditParams_t *) malloc(sizeof(bibEditParams_t));
                if(!aBufEdit->editParams)
                {
                    //Memory not available 
                    return TX_ERR; //indicating error no memory;
                }
                else
                {
                    aBufEdit->numChanges = 1;
                } 
            } 
            aBufEdit->copyMode = CopyWithEdit; /* CopyWithEdit */
            aBufEdit->editParams->StartByteIndex = aInBuffer->getIndex;
            aBufEdit->editParams->StartBitIndex = aInBuffer->bitIndex;
            aBufEdit->editParams->curNumBits = num_bits;
            aBufEdit->editParams->newNumBits = numOutputTrBits;
            aBufEdit->editParams->newValue = aDecoderInfo->aMPEG4TimeStamp->time_inc;
    
            /* move to end position of 'time increment' field */
            bibForwardBits(num_bits, aInBuffer);    // move to end of 'time increment' field
            /* update time increment */
            if (aVopheader->time_inc != curNewTimeCode.time_inc)
            {
                aBufEdit->editParams->newValue = curNewTimeCode.time_inc;
            }
            numBitChange += aBufEdit->editParams->newNumBits - aBufEdit->editParams->curNumBits;
        } 
       
        /* copy time increment field with edit */
        CopyStream(aInBuffer,aOutBuffer,aBufEdit,startByteIndex,startBitIndex);
        startByteIndex=aInBuffer->getIndex;
        startBitIndex=aInBuffer->bitIndex;
        
    }

    /* copy rest of video packet */
    
    /* find the next resync marker */
    *aSncCode = sncSeekMPEGStartCode(aInBuffer, aVopheader->fcode_forward, 0 /* VPs used*/, 0, &error);

    /* record next resync position */
    int resyncBitsLeft = aInBuffer->bitsLeft;
        
    /* rewind stuffing bits */
    sncRewindStuffing(aInBuffer, &error);       
        
    if (!aBufEdit->editParams)
    {
        aBufEdit->editParams = (bibEditParams_t *) malloc(sizeof(bibEditParams_t));
        if(!aBufEdit->editParams)
        { 
            //Memory not available 
            return TX_ERR; //indicating error no memory;
        }
        else
        {
            aBufEdit->numChanges = 1;
        }
    }

    /* record position */
    aBufEdit->editParams->StartByteIndex = aInBuffer->getIndex;
    aBufEdit->editParams->StartBitIndex = aInBuffer->bitIndex;
        
    /* calculate new stuffing bits */
    aBufEdit->editParams->curNumBits = aInBuffer->bitsLeft - resyncBitsLeft;

    /* calculate number of bits/bytes changed */
    if (numBitChange<0)
    {
        outTirDecreased = 1;
        numBitChange = -numBitChange;
    }
    int numByteChange = numBitChange >> 3;
        
    /* evaluate change in buffer size */
    if (outTirDecreased)
    {
        increaseBytes -= numByteChange;
        stuffingLength = aBufEdit->editParams->curNumBits + (numBitChange - (numByteChange << 3));
        if (stuffingLength > 8)
        {
            stuffingLength -= 8;
            increaseBytes --;
        }
    }
    else
    {
        increaseBytes += numByteChange;
        stuffingLength = aBufEdit->editParams->curNumBits - (numBitChange - (numByteChange << 3));
        if (stuffingLength <= 0)
        {
            stuffingLength += 8;
            increaseBytes ++;
        }
    }
        
    /* adjust the output buffer size */
    if (increaseBytes != 0)
    {
        aOutBuffer->size += increaseBytes;
        if (increaseBytes>=0)
        {
            aOutBuffer->bitsLeft += (increaseBytes << 3);
        }
        else
        {
            aOutBuffer->bitsLeft -= ((-increaseBytes) << 3);
        }
    }
    /* update edit statistics */
    aBufEdit->editParams->newNumBits = stuffingLength;
    aBufEdit->editParams->newValue = stuffingLength>0?(1<<(stuffingLength-1))-1:0;
    if (stuffingLength != aBufEdit->editParams->curNumBits)
    {
        aBufEdit->copyMode = CopyWithEdit; // copy with edit
    }
    else 
    {
        aBufEdit->copyMode = CopyWhole; // copy whole 
    }
        
    bibForwardBits(aBufEdit->editParams->curNumBits, aInBuffer);
    /* copy video packet with stuffing bits edited */
    CopyStream(aInBuffer, aOutBuffer, aBufEdit, startByteIndex, startBitIndex);
    /* record position */
    startByteIndex = aInBuffer->getIndex;
    startBitIndex  = aInBuffer->bitIndex;
        
    /* update position for return */
    *aStartByteIndex = startByteIndex;
    *aStartBitIndex  = startBitIndex;

    return TX_OK;
}


/*
* sPutBits
*
* Parameters:
*      outBuf           output buffer
*           numBits         number of bits to output
*           value           new value
*
* Function:
*    Wrapper to sPutBits
* Returns:
*    None.
* Error codes:
*    None.
*
*/
void vdtPutBits (void *buf,  int numBits, unsigned int value)
{
    /* must be in this type! "void" is used here only because of the interface with vlb.cpp */
    sPutBits ((bibBuffer_t *)(buf),  numBits, value);
}



/* {{-output"vdtCopyBuffer.txt"}} */
/*
* vdtCopyBuffer
*
* Parameters:
*
* Function:
*    This function copies some data from source buffer to destination buffer
* Returns:
*    None.
* Error codes:
*    None.
*
*/
void vdtCopyBuffer(bibBuffer_t *SrcBuffer,bibBuffer_t *DestBuffer,
                                     int ByteStart,int BitStart, int ByteEnd, int BitEnd)
{
    int startByteIndex = SrcBuffer->getIndex;
    int startBitIndex  = SrcBuffer->bitIndex;
    int bitsLeft = SrcBuffer->bitsLeft;
    int numBytesRead = SrcBuffer->numBytesRead;
    
    CopyBuffer(SrcBuffer,DestBuffer, ByteStart, BitStart, ByteEnd, BitEnd);
    
    /* recover the postion */
    SrcBuffer->getIndex = startByteIndex ;
    SrcBuffer->bitIndex = startBitIndex;
    SrcBuffer->bitsLeft = bitsLeft;
    SrcBuffer->numBytesRead = numBytesRead;
}




/* {{-output"vdtStuffBitsMPEG4.txt"}} */
/*
* vdtStuffBitsMPEG4
*
* Parameters:
*          outBuf           output buffer
*
* Function:
*    This function puts some stuffing bits to the output buffer
*   bits need to be stuffed in the output buffer at the end of vp in all cases (bw or not)
* Returns:
*    None.
* Error codes:
*    None.
*
*/
void vdtStuffBitsMPEG4(bibBuffer_t *outBuffer)
{
    const int stuffingBits[8][2] = { {1,0}, {2,1}, {3,3}, {4,7}, {5,15}, {6,31}, {7,63}, {8,127} };
  VDTASSERT(outBuffer->baseAddr);
    
    /* find the number of stuffing bits to insert in the output buffer */
    int bi = outBuffer->bitIndex;
    int newNumBits = stuffingBits[bi][0]; 
    int newValue = stuffingBits[bi][1]; 
    sPutBits(outBuffer, newNumBits, newValue);
        
    return;
}


/*************************************************************/


/* {{-output"sResetH263IntraDcUV.txt"}} */
/*
* sResetH263IntraDcUV
*
* Parameters: output buffer
*             uValue
*             vValue
*
* Function:
*    This function reset the chrominace INTRADC when black and white color effect is applied 
* Returns:
*    none
* Error codes:
*    None.
*
*/
inline void sResetH263IntraDcUV(bibBuffer_t *DestBuffer, TInt uValue, TInt vValue)
{
    /* For the Color Effects Fill the U and V buffers with the 
    corresponding color values */
   
    sPutBits(DestBuffer, 8, uValue);
    sPutBits(DestBuffer, 8, vValue);
}


/* {{-output"vdtGetPMBBlackAndWhiteMCBPC.txt"}} */
/*
* vdtGetPMBBlackAndWhiteMCBPC
*
* Parameters: 
*
* Function:
*    This function compute the new mcbpc for black and white effect *
* Returns:
*    the length of the input mcbpc.
* Error codes:
*    None.
*
*/
int vdtGetPMBBlackAndWhiteMCBPC(int& new_len, int& new_val, int mcbpc)
{
    int cur_index, new_index, cur_len;
        
    const tVLCTable sCBPCPType[21] = 
    {
    {1, 1}, {3, 4}, {2, 4}, {5, 6},
    {3, 3}, {7, 7}, {6, 7}, {5, 9}, 
    {2, 3}, {5, 7}, {4, 7}, {5, 8},
    {3, 5}, {4, 8}, {3, 8}, {3, 7},
    {4, 6}, {4, 9}, {3, 9}, {2, 9}, 
    {1, 9}
    };
    
     /* evaluate MCBPC parameters */
    int cur_cbpc = mcbpc & 3;       
    int new_cbpc = 0;       // cpbc=0 indicates chroma is 0
    int mbType; 
    
    mbType = mcbpc / 4;
    /* evaluate indices in table */
    cur_index = mbType * 4 + cur_cbpc;  
    new_index = mbType * 4 + new_cbpc;  
    
    /* retrieve values */
    cur_len = sCBPCPType[cur_index].length;
    new_len = sCBPCPType[new_index].length;
    new_val = sCBPCPType[new_index].code;
    
    return cur_len;
}


/* {{-output"vdtGetIMBBlackAndWhiteMCBPC.txt"}} */
/*
* vdtGetIMBBlackAndWhiteMCBPC
*
* Parameters: None
*
* Function:
*    This function compute the new mcbpc for black and white effect
* Returns:
*    the length of the input mcbpc.
* Error codes:
*    None.
*
*/
int vdtGetIMBBlackAndWhiteMCBPC(int& new_len, int& new_val, int mcbpc)
{
    int cur_index, new_index, cur_len;
    
    const tVLCTable sCBPCIType[9] = 
    {
    {1, 1}, {1, 3}, {2, 3}, {3, 3}, {1, 4},
    {1, 6}, {2, 6}, {3, 6}, {1, 9}
    };
    
    /* evaluate MCBPC parameters */
    int cur_cbpc = mcbpc & 3;       
    int new_cbpc = 0;       // cpbc=0 indicates chroma is 0
    int mbType = (mcbpc <4)?3:4;
    
    /* evaluate indices in table */
    cur_index = (mbType == 3 ? 0 : 4) + cur_cbpc;   
    new_index = (mbType == 3 ? 0 : 4) + new_cbpc;   
    
    /* retrieve values */
    cur_len = sCBPCIType[cur_index].length;
    new_len = sCBPCIType[new_index].length;
    new_val = sCBPCIType[new_index].code;
    
    return cur_len;
}





/* {{-output"vdtGetVideoBitstreamInfo.txt"}} */
/*
* vdtGetVideoBitstreamInfo
*
* Parameters: 
*
* Function:
*    This function provides the bitstream info to the processor *
* Returns:
*    VDE error codes
* Error codes:
*    None.
*
*/
int vdtGetVideoBitstreamInfo(bibBuffer_t *inBuffer, vdeDecodeParamters_t *aInfoOut, int *aByteIndex, int *aBitIndex)
{
    int numBitsGot,
    bitErrorIndication = 0;
    int16 error = 0;
    u_int32 bits;
    int timeResolution = 0;
    TVedVideoBitstreamMode streamMode = EVedVideoBitstreamModeUnknown;
    vdxVolHeader_t volHeader;  
    volHeader.user_data = NULL;
    
    bits = bibShowBits(32, inBuffer, &numBitsGot, &bitErrorIndication, &error);
    if (error)
    {
        streamMode = EVedVideoBitstreamModeUnknown;
        goto exitFunction;
    }
    /* If PSC */
    if ((bits >> 10) == 32) {
        streamMode = EVedVideoBitstreamModeH263;
    } 

    /* Else check for Visual Sequence, Visual Object or Video Object start code */
    else if ((bits == MP4_VOS_START_CODE) || 
        (bits == MP4_VO_START_CODE) ||
        ((bits >> MP4_VID_ID_CODE_LENGTH) == MP4_VID_START_CODE) ||
        ((bits >> MP4_VOL_ID_CODE_LENGTH) == MP4_VOL_START_CODE)) 
    {
        
        /* read the Stream headers from the bitstream */
        if ((vdxGetVolHeader(inBuffer, &volHeader, &bitErrorIndication, 1, aByteIndex, aBitIndex, NULL) != 0) ||
            (bitErrorIndication != 0)) 
        {
            goto exitFunction;
        }   

        timeResolution = volHeader.time_increment_resolution;
        streamMode = sGetMPEG4Mode(volHeader.error_res_disable, volHeader.data_partitioned, volHeader.reversible_vlc);
        bits = bibShowBits(22, inBuffer, &numBitsGot, &bitErrorIndication, &error);
        if (error)
            goto exitFunction;
        
      /* Check if H.263 PSC follows the VOL header, in which case this is 
        MPEG-4 with short header and is decoded as H.263 */
        if ( bits == 32 ) 
        {
            streamMode = EVedVideoBitstreamModeMPEG4ShortHeader;
        }
    }

    /* Else no H.263 and no MPEG-4 start code detected  */
    else {
        streamMode = EVedVideoBitstreamModeUnknown;
    }
    
exitFunction:
    /* copy the got user data to the core data structure */
    if (volHeader.user_data != NULL) 
    {
        free(volHeader.user_data);
    }
    
    bibRewindBits( bibNumberOfFlushedBits( inBuffer ), inBuffer, &error );
    aInfoOut->streamMode = streamMode;
    aInfoOut->iTimeIncrementResolution = timeResolution;
    return TX_OK;
}





/* {{-output"vdtGetVideoBitstreamInfo.txt"}} */
/*
* sQuantizeMB
*
* Parameters: 
*    mbdata          Contains the actual MB data to be quantized
*    oldQuant: 
*    newQuant:
*    intra
*
* Function:
*    This function requantizes the AC/DCT data for one MB, used in MPEG4 -> H263 transcoding 
* Returns:
*    Coded Block Pattern.
* Error codes:
*    None.
*
*/
static void sQuantizeMB(int *mbdata, int oldQuant, int newQuant, int intra, int colorEffect)
{
#define  SIGN(x)          (((x)<0) ? -1:1 )
    int   coeffCnt;
    int   *block;
    int   blkCnt;
    
    for (blkCnt = 0; blkCnt < (colorEffect ? 4 : 6); blkCnt++)
    {   
        block = &mbdata[blkCnt * BLOCK_COEFF_SIZE];
        for (coeffCnt = (intra == VDX_MB_INTRA); coeffCnt < BLOCK_COEFF_SIZE;
             coeffCnt++)
             {
                 if (block[coeffCnt])
                 {
                     int level = abs(block[coeffCnt]);
                     int sign = SIGN(block[coeffCnt]);
                     int rcoeff;
                     // dequantize 
                     if ((oldQuant % 2) == 1)
                         rcoeff = oldQuant * ((level << 1) + 1);
                     else
                         rcoeff = oldQuant * ((level << 1) + 1) - 1;
                     
                     rcoeff = min (2047, max (-2048, sign * rcoeff));
                     
                     // requantize
                     if (intra == VDX_MB_INTRA)
                     {
                         level = (abs (rcoeff)) / (newQuant << 1);
                     }
                     else
                     {
                         level = (abs (rcoeff) - (newQuant >> 1)) / (newQuant << 1);
                         /* clipping to [-127,+127] */
                     }
                     
                     /* clipping to [-127,+127] */
                     block[coeffCnt] = min (127, max (-127, sign * level));
                 }
                 else
                 {
                     /* Nothing */
                 }
             }
    }
    return ;
}   




/* {{-output"vdtGetVideoBitstreamInfo.txt"}} */
/*
* vdtChangeVosHeaderRegResyncL
*
* Parameters: 
*
* Function:
*    This function finds the error resillence bit and change it if necessary
*      to make it Regular Resynchronization mode 
* Returns:
*    ETrue if buffer changed.
* Error codes:
*    None
*
*/
TBool vdtChangeVosHeaderRegResyncL(TPtrC8& aInputBuffer, TUint aBufferSize)
{
    int16   errorCode = 0;             /* return code for bib functions */
    bibBuffer_t *buffer;                   /* input buffer */
    
    /* Create bit buffer */
    buffer = bibCreate((TAny*)aInputBuffer.Ptr(), aBufferSize, &errorCode);
    if (!buffer || errorCode)
        return EFalse;
     
    int startByte = 0, startBit = 7;
    vdeDecodeParamters_t decInfo; 
    vdtGetVideoBitstreamInfo(buffer, &decInfo, &startByte, &startBit);
     
    if (decInfo.streamMode == EVedVideoBitstreamModeMPEG4Regular)
    {
        char *temp = (char *) (TAny*)aInputBuffer.Ptr();
        unsigned char patern[8] = {0xFE,0xFD,0xFB,0xF7,0xEF,0xDF,0xBF,0x7F};
        temp[startByte] &= patern[startBit]; // change the error resillence bit to 0;
        
        /* Delete bit buffer */
        bibDelete(buffer, &errorCode);
        return ETrue;
    }
    else
    {
      /* Delete bit buffer */
        bibDelete(buffer, &errorCode);
        return EFalse;
    }
}

/*
* CMPEG4Transcoder
*
* Parameters: 
*
* Function:
*    Constructor
*
*/
CMPEG4Transcoder::CMPEG4Transcoder(const vdeInstance_t *aVDEInstance, bibBuffer_t *aInBuffer, bibBuffer_t *aOutBuffer)
{
    VDTASSERT(aVDEInstance); 
    VDTASSERT(aInBuffer);
    VDTASSERT(aOutBuffer);
    
    /* passing the arguments */
    iVDEInstance = aVDEInstance;
    iInBuffer  = aInBuffer;
    iOutBuffer = aOutBuffer;
    
    /* Color Toning */
    iMBType      = NULL;
    iCurQuant    = 0;
    iColorEffect = aVDEInstance->iColorEffect;
    iColorToneU  = aVDEInstance->iColorToneU;
    iColorToneV  = aVDEInstance->iColorToneV;
    iRefQuant    = aVDEInstance->iRefQp;
    iDcScaler    = GetMpeg4DcScalerUV(iRefQuant);
        
    iDoModeTranscoding = EFalse;
    iDoTranscoding = (iColorEffect || iDoModeTranscoding);
    iTargetFormat = EVedVideoTypeMPEG4SimpleProfile;
    iStuffingBitsUsed = 0;
    
    /* default values */
    iLastMBNum = -1;
    iCurMBNum = 0;

    /* initialize here but will be changed later to proper value */
    iOutputMpeg4TimeIncResolution = KOutputMpeg4TimeIncResolution;  

    iNumMBsInOneVOP = (iVDEInstance->lumHeight * iVDEInstance->lumWidth) >> 8;  // /256
    iMBsinWidth = iVDEInstance->lumWidth >> 4; 
    iNumMBsInGOB = iVDEInstance->lumWidth >> 4; 
    

    iMBList = NULL;
    iH263DCData = NULL;
    h263mbi = NULL;

    
    iH263MBVPNum = NULL;
    iErrorResilienceStartByteIndex = KDataNotValid;
    iErrorResilienceStartBitIndex  = KDataNotValid;
    
    iVideoPacketNumInMPEG4 = 0; // the video packet number this MB belongs to, NOTE: the first GOB doesn't have a GOB header
    iCurMBNumInVP = -1;
    fFirstFrameInH263 = EFalse;
    
    return;
}

/*
* ~CMPEG4Transcoder
*
* Parameters: 
*
* Function:
*    Destructor
*
*/
CMPEG4Transcoder::~CMPEG4Transcoder()
{
    if (iCurIMBinstance)
    {
        free(iCurIMBinstance);
    }
    if (iCurPMBinstance)
    {
        free(iCurPMBinstance);
    }
    if (bufEdit.editParams)
    {
        free(bufEdit.editParams);
    }
    
    // for H263
    if (iH263MBVPNum)
    {
        free(iH263MBVPNum);
    }
    
    // for color toning
    if (iMBType)
    {
        free(iMBType);
    }


    
    if (iH263DCData)
    {
        for (int i = 0; i < iNumMBsInOneVOP; i++)
        {
            free(iH263DCData[i]);
        }
        free(iH263DCData);
    }

    if ( ( (iTargetFormat == EVedVideoTypeH263Profile0Level10) || 
           (iTargetFormat == EVedVideoTypeH263Profile0Level45) ) &&
           iBitStreamMode != EVedVideoBitstreamModeMPEG4ShortHeader &&
           iBitStreamMode != EVedVideoBitstreamModeH263 &&
           h263mbi)
    {
        free(h263mbi);
    }
    
    if ( iOutBuffer )
        {
        
        PRINT((_L("CMPEG4Transcoder: finish one frame successfully, buffer size %d"), 
           iOutBuffer->getIndex));
        }
        
#ifdef _DEBUG
    if (iOutBuffer->getIndex > KInitialBufferSize)
    {
        PRINT((_L("CMPEG4Transcoder: Output buffer size from engine is not big enough! check KInitialBufferSize")));
        
    }
#endif
    if ( iOutBuffer )
        {
        VDTASSERT(iOutBuffer->getIndex < KInitialBufferSize);
        }
}

/*
* H263EscapeCoding
*
* Parameters: 
*
* Function:
*    Indicates whether escape vlc coding is used in one block
* Returns:
*    None
* Error codes:
*    None
*
*/
void CMPEG4Transcoder::H263EscapeCoding(int aIndex, int fEscapeCodeUsed)
{
    iEscapeCodeUsed[aIndex] = fEscapeCodeUsed;
}

/*
* SetTranscoding
*
* Parameters: 
*
* Function:
*    Indicates whether we need to do MPEG4 bitstream transcoding
* Returns:
*    TX error codes
* Error codes:
*    None
*
*/
int CMPEG4Transcoder::SetTranscoding(vdeDecodeParamters_t *aDecoderInfo)
{
    iColorEffect = aDecoderInfo->aColorEffect;
    iColorToneU = aDecoderInfo->aColorToneU; 
    iColorToneV = aDecoderInfo->aColorToneV;
    iBitStreamMode = (TVedVideoBitstreamMode)aDecoderInfo->streamMode;
    
    iTargetFormat = aDecoderInfo->aOutputVideoFormat; 
    iDoModeTranscoding = aDecoderInfo->fModeChanged ? ETrue: EFalse;
    iDoTranscoding = (iColorEffect || iDoModeTranscoding);

    /* set to proper value */
    iOutputMpeg4TimeIncResolution = *(aDecoderInfo->aMPEG4TargetTimeResolution); 

    /* allocate buffer for MPEG4 - > H263 transcoding */
    /* the following is used if spatial domain processing is not done */
        
    if ( ( (iTargetFormat == EVedVideoTypeH263Profile0Level10) ||
           (iTargetFormat == EVedVideoTypeH263Profile0Level45) ) &&
           iBitStreamMode != EVedVideoBitstreamModeMPEG4ShortHeader &&
           iBitStreamMode != EVedVideoBitstreamModeH263)
    {
        h263mbi = (tMBInfo*) malloc (sizeof(tMBInfo) * iNumMBsInOneVOP);
        if (!h263mbi) 
        {
            PRINT((_L("CMPEG4Transcoder::SetTranscoding - h263mbi creation failed")));
            deb("CMPEG4Transcoder::SetTranscoding - h263mbi creation failed\n");

            return TX_ERR;
        }
        memset(h263mbi, 0, sizeof(tMBInfo) * iNumMBsInOneVOP);
    }
    
    return TX_OK;
}




/*
* BeginOneVideoPacket
*
* Parameters: 
*
* Function:
*    Records the position before one Video packet is processed
* Returns:
*    None
* Error codes:
*    None
*
*/
void CMPEG4Transcoder::BeginOneVideoPacket(dvpVPInParam_t *aVPin)
{
    VDTASSERT(iInBuffer); 
    VDTASSERT(aVPin);
    
    iCurVPIn = aVPin;
    iVPStartByteIndex = iInBuffer->getIndex;
    iVPStartBitIndex  = iInBuffer->bitIndex;
    
    iCurMBNumInVP = -1;

    iStuffingBitsUsed = 0;
    
    iVopCodingType = aVPin->pictParam->pictureType;
    iBitStreamMode = sGetMPEG4Mode(aVPin->pictParam->error_res_disable,
        aVPin->pictParam->data_partitioned, aVPin->pictParam->reversible_vlc);
}


/*
* sConstructH263PicHeader
*
* Parameters: 
*        lBitOut                output buffer
*        quant                quant value to be put in header
*           picType             VDX_VOP_TYPE_P or VDX_VOP_TYPE_I
*           tr                      time increament
* Function:
*    Writes the pic header into bit-stream for one MPEG4 frame
* Returns:
*    None
* Error codes:
*    None
*
*/
void sConstructH263PicHeader(bibBuffer_t *lBitOut, int picType, int quant, int tr, int aVOPNotCoded, int aFormat)

{
    if (aVOPNotCoded)
    {
        return;
    }
    /* ShortVideoStartMarker */
    sPutBits(lBitOut, 22, SHORT_VIDEO_START_MARKER);
    /* TemporalReference */
    sPutBits(lBitOut, 8, (tr & 0x000000ff));
    /* Marker Bit */
    sPutBits(lBitOut, 1, MARKER_BIT);
    /* Zero Bit */
    sPutBits(lBitOut, 1, 0);
    /* SplitScreenIndicator = 0 */
    sPutBits(lBitOut, 1, 0);
    /* DocumentCameraIndicator = 0 */
    sPutBits(lBitOut, 1, 0);
    /* FullPictureFreezeRelease = 0 */
    sPutBits(lBitOut, 1, 0);
    /* Source Fromat */
    sPutBits(lBitOut, 3, aFormat);
    /* PictureCodingType 0 for intra, 1 for inter */
    sPutBits(lBitOut, 1, picType == VDX_VOP_TYPE_P);
    /* UMV= 0 */
    sPutBits(lBitOut, 1, 0);
    /* SAC = 0 */
    sPutBits(lBitOut, 1, 0);
    /* Advanced Prediction = 0 */
    sPutBits(lBitOut, 1, 0);
    /* PB frame = 0 */
    sPutBits(lBitOut, 1, 0);
    /* VOPQuant */
    sPutBits(lBitOut, 5, quant);
    /* ZeroBIt */
    sPutBits(lBitOut, 1, 0);
    /* Pei = 0 */
    sPutBits(lBitOut, 1, 0);        
}


/*
* VOPHeaderEnded
*
* Parameters: 
*
* Function:
*    Copy the VOP Header to output buffer
* Returns:
*    None
* Error codes:
*    None
*
*/
void CMPEG4Transcoder::VOPHeaderEnded(int aStartByteIndex, int aStartBitIndex, 
    int aQuant, int aPicType, int aFrameNum, int aVOPNotCoded)
{
    const int KNumMbInSqcif = 48; 
    const int KNumMbInQcif = 99; 
    const int KNumMbInCif = 396; 

    if (iTargetFormat == EVedVideoTypeH263Profile0Level10 || iTargetFormat == EVedVideoTypeH263Profile0Level45)
    {
        int sourceFormat = 2; // qcif
        /* note: other formate not supported */
        if (iNumMBsInOneVOP == KNumMbInSqcif)  /* sqcif */
        {
            sourceFormat = 1;  // sqcif
        }
        else if (iNumMBsInOneVOP == KNumMbInQcif) /* qcif */
        {
            sourceFormat = 2;  // qcif
        }
        else if (iNumMBsInOneVOP == KNumMbInCif) /* cif */
        {
            sourceFormat = 3;  // cif
        }
        sConstructH263PicHeader(iOutBuffer, aPicType, aQuant, 0, aVOPNotCoded, sourceFormat);
        PRINT((_L("CMPEG4Transcoder: MPEG4 -> H263: picture header generated")));
    }
  /* Copy the VOP header */
  else 
    {
        bufEdit.copyMode = CopyWhole; // whole
        CopyStream(iInBuffer,iOutBuffer,&bufEdit,aStartByteIndex,aStartBitIndex);
    }
  iPreQuant = aQuant;
  iStuffingBitsUsed = 0;

  iCurQuant = aQuant;
  /* Color Toning */
  if (!aFrameNum || !iRefQuant)
  {
     iRefQuant = iCurQuant;
     iDcScaler = GetMpeg4DcScalerUV(iRefQuant); 
  }

}


/*
* VOPEnded
*
* Parameters: 
*
* Function:
*    This function is called when one VOP has ended
* Returns:
*    None
* Error codes:
*    None
*
*/
void CMPEG4Transcoder::VOPEnded()
{
    /* check if MBs are lost */
    int dataPartitioned = (iBitStreamMode == EVedVideoBitstreamModeMPEG4DP_RVLC || iBitStreamMode == EVedVideoBitstreamModeMPEG4Resyn_DP_RVLC 
        || iBitStreamMode == EVedVideoBitstreamModeMPEG4DP ||  iBitStreamMode == EVedVideoBitstreamModeMPEG4Resyn_DP);
    
    if (iLastMBNum != iNumMBsInOneVOP - 1 && !(dataPartitioned && !iDoModeTranscoding))
    {
        for (int i = iLastMBNum+1; i < iNumMBsInOneVOP; i++)
        {
            /* output  1 bit COD */
            sPutBits (iOutBuffer, 1, 1);
        }
    }
    
    iLastMBNum = iNumMBsInOneVOP - 1; 
    /* bits need to be stuffed in the output buffer at the end VOP */
    if (!dataPartitioned && iTargetFormat == EVedVideoTypeMPEG4SimpleProfile)
    {
        if (!iStuffingBitsUsed)
        {
            vdtStuffBitsMPEG4(iOutBuffer);
        }
        iStuffingBitsUsed = 1;
    }
    else if (iTargetFormat == EVedVideoTypeH263Profile0Level10 || iTargetFormat == EVedVideoTypeH263Profile0Level45)
    {
        sStuffBitsH263(iOutBuffer);
    }
    
  PRINT((_L("CMPEG4Transcoder: VOPEnded. color effect: %d, format convert %d, do transcoding: %d streammode: %d, outputformat: %d "), 
       iColorEffect, iDoModeTranscoding, iDoTranscoding, iBitStreamMode, iTargetFormat));
}

/*
* AfterOneVideoPacketHeader
*
* Parameters: 
*
* Function:
*    This function is called after retreiving the VP
*    Records the position before the content of the video packet is processed
* Returns:
*    None
* Error codes:
*    None
*
*/
void CMPEG4Transcoder::AfterVideoPacketHeader(dvpVPInOutParam_t *aVPInfo)
{
    VDTASSERT(aVPInfo); 
    iCurVPInOut = aVPInfo;
    
    iVPHeaderEndByteIndex = iInBuffer->getIndex;
    iVPHeaderEndBitIndex  = iInBuffer->bitIndex;
    
    /* Color Toning */
    iCurQuant = aVPInfo->quant; 
    
    /* Copy the VP header to output stream. Note; the first VP does not have a VP header */
    if (iTargetFormat == EVedVideoTypeMPEG4SimpleProfile)
    {
       bufEdit.copyMode = CopyWhole; /* whole */
         CopyStream(iInBuffer,iOutBuffer,&bufEdit,iVPStartByteIndex,iVPStartBitIndex);
    }
}


/*
* OneVPEnded
*
* Parameters: 
*
* Function:
*    This function is called after one VP  contents are retrieved
* Returns:
*    None
* Error codes:
*    None
*
*/
void CMPEG4Transcoder::OneVPEnded()
{
    if (!iDoTranscoding)
    {
        /* no transcoding, copy whole content VP */
        bufEdit.copyMode = CopyWhole; 
        CopyStream(iInBuffer,iOutBuffer,&bufEdit,iVPHeaderEndByteIndex,iVPHeaderEndBitIndex);
        iStuffingBitsUsed = 1;// also stuffing is copied
    }
    else
    {
        int dataPartitioned = (iBitStreamMode == EVedVideoBitstreamModeMPEG4DP_RVLC || iBitStreamMode == EVedVideoBitstreamModeMPEG4Resyn_DP_RVLC 
            || iBitStreamMode == EVedVideoBitstreamModeMPEG4DP ||  iBitStreamMode == EVedVideoBitstreamModeMPEG4Resyn_DP);
        int nextExpectedMBNum = iCurVPInOut->currMBNum;
        
        if (dataPartitioned && !iDoModeTranscoding)
        {
        /* if data is partitioned and we are not doing format transcoding,
        not coded MBs info is already in data partition 1
            */
        }
        else
        {
            /* MBs are lost or not coded,  */
            for (int i = iLastMBNum+1; i < nextExpectedMBNum; i++)
            {
                /* output  1 bit COD */
                sPutBits (iOutBuffer, 1, 1);
            }
        }
        
        iLastMBNum = nextExpectedMBNum - 1; 
        /* bits need to be stuffed in the output buffer at the end of vp in all cases (bw or not) */
        if (iTargetFormat == EVedVideoTypeMPEG4SimpleProfile) 
        {
            if (!iStuffingBitsUsed)
            {
                vdtStuffBitsMPEG4(iOutBuffer);
            }
            iStuffingBitsUsed = 1;
        }
    }
}



/*
* BeginOneMB
*
* Parameters: 
*
* Function:
*    Records the position before one MB is processed
* Returns:
*    None
* Error codes:
*    None
*
*/
void CMPEG4Transcoder::BeginOneMB(int aMBNum)
{
    VDTASSERT(iInBuffer); 
    
    iMBStartByteIndex = iInBuffer->getIndex;
    iMBStartBitIndex  = iInBuffer->bitIndex;
    iCurMBNum = aMBNum;
    
    // Color Toning
    iCurMBNumInVP++;
    
    if ((iBitStreamMode == EVedVideoBitstreamModeMPEG4ShortHeader || iBitStreamMode == EVedVideoBitstreamModeH263) && iDoModeTranscoding)
    {
       /* H263 -> MPEG4  */
       VDTASSERT(iH263MBVPNum);
         iH263MBVPNum[iCurMBNum]= iVideoPacketNumInMPEG4;
    }
}

/*
* BeginOneBlock
*
* Parameters: 
*
* Function:
*    Records the position before one block in MB is processed
* Returns:
*    None
* Error codes:
*    None
*
*/
void CMPEG4Transcoder::BeginOneBlock(int aBlockIndex)
{
    VDTASSERT(aBlockIndex >= 0 && aBlockIndex < 6); 
    
    iBlockStartByteIndex[aBlockIndex] = iInBuffer->getIndex;
    iBlockStartBitIndex[aBlockIndex]  = iInBuffer->bitIndex;
}


/*
* OneIMBDataStarted
*
* Parameters: 
*
* Function:
*    This function is called after the MB header is read; start one IMB, 
* Returns:
*    None
* Error codes:
*    None
*
*/
void CMPEG4Transcoder::OneIMBDataStarted(vdxIMBListItem_t *aMBInstance)
{
    VDTASSERT(aMBInstance); 
    VDTASSERT(iCurIMBinstance);
    iMBCodingType = VDX_MB_INTRA;
    iVopCodingType = VDX_VOP_TYPE_I;
    iMBType[iCurMBNum] = iMBCodingType;

    
    memcpy(iCurIMBinstance, aMBInstance, sizeof(vdxIMBListItem_t)); 
}

/*
* OnePMBDataStarted
*
* Parameters: 
*
* Function:
*    This function is called after the MB header is read; start one PMB, 
* Returns:
*    None
* Error codes:
*    None
*
*/
void CMPEG4Transcoder::OnePMBDataStarted(vdxPMBListItem_t *aMBInstance)
{
    VDTASSERT(aMBInstance); 
    VDTASSERT(iCurPMBinstance);
    
    iMBCodingType = aMBInstance->mbClass;
    iVopCodingType = VDX_VOP_TYPE_P;
    iMBType[iCurMBNum] = iMBCodingType;

    memcpy(iCurPMBinstance, aMBInstance, sizeof(vdxPMBListItem_t)); 
}



/*
* OneIMBDataStartedDataPartitioned
*
* Parameters: 
*
* Function:
*    Add one IMB instance, It is done after parsing Part1 and Part2 before the block data for current MB
* Returns:
*    None
* Error codes:
*    None
*
*/
void CMPEG4Transcoder::OneIMBDataStartedDataPartitioned(vdxIMBListItem_t *aMBInstance, dlst_t *aMBList, int aCurrMBNumInVP, int aMBNum)
{
    VDTASSERT(aMBInstance); // at this point, the instance should not be null
    VDTASSERT(aMBList);
    
    memcpy(iCurIMBinstance, aMBInstance, sizeof(vdxIMBListItem_t)); 
    iCurMBNumInVP = aCurrMBNumInVP;
    iCurMBNum = aMBNum;
    iMBList = aMBList;
    iMBCodingType = VDX_MB_INTRA;
    iVopCodingType = VDX_VOP_TYPE_I;
    iMBType[iCurMBNum] = iMBCodingType;
    
    if (!iCurMBNumInVP && iDoTranscoding) 
    {
        if (!iDoModeTranscoding)
        {
          /* We are not doing bitstream transcoding,  
            we have color effect here and need to reconstruct data partitions.
            */
            ReconstructIMBPartitions();
        }
    }
}

/*
* OnePMBDataStartedDataPartitioned
*
* Parameters: 
*
* Function:
*    Add one IMB instance, It is done after parsing Part1 and Part2 before the block data for current MB
*    Note: PMB may be INTRA coded.
* Returns:
*    None
* Error codes:
*    None
*
*/
void CMPEG4Transcoder::OnePMBDataStartedDataPartitioned(vdxPMBListItem_t *aMBInstance, dlst_t *aMBList, int aCurrMBNumInVP, int aMBNum)
{
    VDTASSERT(aMBInstance); /* at this point, the instance should not be null */
    VDTASSERT(aMBList);
    
    memcpy(iCurPMBinstance, aMBInstance, sizeof(vdxPMBListItem_t)); 
    iCurMBNumInVP = aCurrMBNumInVP;
    iCurMBNum = aMBNum;
    iMBList = aMBList;
    iMBCodingType = aMBInstance->mbClass;
    iVopCodingType = VDX_VOP_TYPE_P;
    iMBType[iCurMBNum] = iMBCodingType;
    
    if (!aMBInstance->fCodedMB && iDoModeTranscoding)
    {
        /* this MB is not coded and need to be converted to target mode,
        out put the possible MB stuffing bits and COD
        */
        /* MB stuffing bits if they exsit */
        vdtCopyBuffer(iInBuffer, iOutBuffer,
            aMBInstance->DataItemStartByteIndex[11], aMBInstance->DataItemStartBitIndex[11],
            aMBInstance->DataItemEndByteIndex[11], aMBInstance->DataItemEndBitIndex[11]);
        /* It is a not-coded MB, output  1 bit COD (it always exists in P frame) */
        sPutBits (iOutBuffer, 1, 1);
        iLastMBNum = iCurMBNum;
    }
    else if (!iCurMBNumInVP && iDoTranscoding) 
    {
        if (!iDoModeTranscoding)
        {
        /* We are not doing bitstream transcoding, 
        we have color effect here and need to reconstruct data partitions.
            */
            ReconstructPMBPartitions();
        }
    }
}



/*
* TranscodingOneMB
*
* Parameters: 
*
* Function:
*    Transcoding one MB, which may include dequantization, requantization , and re-encoding
* Returns:
*    DMD error codes since called from decmbdct
*
*/
int CMPEG4Transcoder::TranscodingOneMB(dmdPParam_t *aParam = NULL)
{
/* Because it has block data, this MB must be a coded MB,
which means the position indicated by iMBStartByteIndex and iMBStartBitIndex starts with a MCBPC 
    */
    VDTASSERT(!(!iCurPMBinstance->fCodedMB && iMBCodingType == VDX_MB_INTER));
    
    if (!iDoTranscoding)
        {
        iLastMBNum = iCurMBNum; // need to update the variable to avoid marking this as noncoded MB
        return TX_OK;
        }
    
    int dataPartitioned = (iBitStreamMode == EVedVideoBitstreamModeMPEG4DP_RVLC || iBitStreamMode == EVedVideoBitstreamModeMPEG4Resyn_DP_RVLC 
        || iBitStreamMode == EVedVideoBitstreamModeMPEG4DP ||  iBitStreamMode == EVedVideoBitstreamModeMPEG4Resyn_DP);
    
    if (dataPartitioned)
    {
        /* since we need to handle MB stuffing bits, which are handle outside the transcoder in other modes,
        not coded MBs and MB stuffing bits are handled in OnePMBDataStartedDataPartitioned, 
             ReconstuctI/PMBPartitios or ConstructRegularMPEG4MBData
        */
    }
    else if(iCurMBNum != iLastMBNum + 1 )
    {
        /* if previous MBs are lost or not coded, we copy one MB with COD = 1; */
        for (int i = 0 ; i < iCurMBNum - iLastMBNum - 1; i++)
        {
            /* output  1 bit COD */
            sPutBits (iOutBuffer, 1, 1);
        }
    }
    
    int newMCBPCLen = 0;
    int newMCBPC = 0;
    int oldMCBPCLen = 0;
    
    /* determine whether to change mcbpc */
    if (iColorEffect)
    {
        if (iVopCodingType == VDX_VOP_TYPE_P) 
        {
            oldMCBPCLen = vdtGetPMBBlackAndWhiteMCBPC(newMCBPCLen, newMCBPC, iCurPMBinstance->mcbpc);
        }
        else
        {
            oldMCBPCLen = vdtGetIMBBlackAndWhiteMCBPC(newMCBPCLen, newMCBPC, iCurIMBinstance->mcbpc);
        }
    }
    

    
    if (iDoModeTranscoding && ( (iTargetFormat == EVedVideoTypeH263Profile0Level10) ||
                                (iTargetFormat == EVedVideoTypeH263Profile0Level45) ) )
    {
        if ( ConstructH263MBData(aParam, newMCBPCLen, newMCBPC) != VDC_OK )
            {
            return TX_ERR;
            }
    }
    else /* possible H263->MPEG4 transcoding */
        
    {
        /* MPEG4 with VDT_RESYN and VDT_REGULAR, we only do bitstream copying */
        if ( iBitStreamMode == EVedVideoBitstreamModeMPEG4Resyn || iBitStreamMode == EVedVideoBitstreamModeMPEG4Regular ||
            ((iBitStreamMode ==EVedVideoBitstreamModeMPEG4ShortHeader || iBitStreamMode == EVedVideoBitstreamModeH263) && !iDoModeTranscoding) )
        {
        /* the MB stuffing is taken card of outside in file viddemux.cpp
        we only need to output COD, MCBPC
            */
            int mcbpcStartByteIndex, mcbpcStartBitIndex;
            
            if (iVopCodingType == VDX_VOP_TYPE_I)
            {
                mcbpcStartByteIndex = iCurIMBinstance->DataItemStartByteIndex[0];
                mcbpcStartBitIndex = iCurIMBinstance->DataItemStartBitIndex[0];
            }
            else
            {
                mcbpcStartByteIndex = iCurPMBinstance->DataItemStartByteIndex[0];
                mcbpcStartBitIndex = iCurPMBinstance->DataItemStartBitIndex[0];
                /* It is a coded MB, output  1 bit COD (it always exists in P frame) */
                sPutBits (iOutBuffer, 1, 0);
            }
            
            if (!iColorEffect)
            {
                bufEdit.copyMode = CopyWhole; /* whole */
                CopyStream(iInBuffer,iOutBuffer,&bufEdit,mcbpcStartByteIndex,mcbpcStartBitIndex);
            }
            else
            {
                /* modify mcbpc and copy only the Y data */
                bufEdit.copyMode = CopyWithEdit; // copy with edit
                bufEdit.editParams[0].StartByteIndex = mcbpcStartByteIndex; 
                bufEdit.editParams[0].StartBitIndex  = mcbpcStartBitIndex; 
                bufEdit.editParams[0].curNumBits = oldMCBPCLen; 
                bufEdit.editParams[0].newNumBits = newMCBPCLen; 
                bufEdit.editParams[0].newValue = newMCBPC; 
                CopyStream(iInBuffer,iOutBuffer,&bufEdit,mcbpcStartByteIndex,mcbpcStartBitIndex);
                
                /* disgard the UV data */
                int16 errorCode = 0;
                int bitsToRewind = ((iInBuffer->getIndex << 3) + 7  - iInBuffer->bitIndex) 
                                     - ((iBlockStartByteIndex[4] << 3) + 7 - iBlockStartBitIndex[4]);
                bibRewindBits(bitsToRewind, iOutBuffer, &errorCode );
                
                if (iMBCodingType == VDX_MB_INTRA)
                {
                    if (iBitStreamMode == EVedVideoBitstreamModeMPEG4ShortHeader || iBitStreamMode == EVedVideoBitstreamModeH263)
                    {
                        sResetH263IntraDcUV(iOutBuffer, iColorToneU, iColorToneV);                        
                    }
                    else
                    {
                        ResetMPEG4IntraDcUV();
                    }
                }
            }
        }

        else if ((iBitStreamMode == EVedVideoBitstreamModeMPEG4ShortHeader || iBitStreamMode == EVedVideoBitstreamModeH263) && iDoModeTranscoding)
        {
            H263ToMPEG4MBData(newMCBPCLen, newMCBPC);
        }
        
        else /* data partitioned */
        {
            if (iDoModeTranscoding)
            {
                /* for data partitioned bitstream, we do the bitstream rearrangement  */
                ConstructRegularMPEG4MBData(newMCBPCLen, newMCBPC);
            }
            else
            {
                /* copy the ACs or DCTs */
                bufEdit.copyMode = CopyWhole; // whole
                CopyStream(iInBuffer,iOutBuffer,&bufEdit,iBlockStartByteIndex[0],iBlockStartBitIndex[0]);
                if (iColorEffect)
                {
                    /* discard U V data */
                    int16 errorCode = 0;
                    int bitsToRewind = ((iInBuffer->getIndex << 3) + 7  - iInBuffer->bitIndex) 
                                             - ((iBlockStartByteIndex[4] << 3) + 7 - iBlockStartBitIndex[4]);
                    bibRewindBits(bitsToRewind, iOutBuffer, &errorCode );
                }
            }
        }

    }
    iLastMBNum = iCurMBNum;
    return TX_OK;
}




/*
* ReconstructIMBPartitions
*
* Parameters: 
*
* Function:
*    Recontruct the partitions for color effect when we are not doing format transcoding
* Returns:
*    None
* Error codes:
*    None
*
*/
void CMPEG4Transcoder::ReconstructIMBPartitions()
{
    vdxIMBListItem_t *MBinstance;
     int *dataItemStartByteIndex;
     int *dataItemStartBitIndex;
     int *dataItemEndByteIndex;
     int *dataItemEndBitIndex;
     int newMCBPCLen = 0;
     int newMCBPC = 0;
     
     VDTASSERT(iMBList);
     
     for (dlstHead(iMBList, (void **) &MBinstance); 
     MBinstance != NULL; 
     dlstNext(iMBList, (void **) &MBinstance))
      {
         dataItemStartByteIndex = MBinstance->DataItemStartByteIndex;
         dataItemStartBitIndex  = MBinstance->DataItemStartBitIndex;
         dataItemEndByteIndex   = MBinstance->DataItemEndByteIndex;
         dataItemEndBitIndex    = MBinstance->DataItemEndBitIndex;
         // MB stuffing bits if they exsit
         vdtCopyBuffer(iInBuffer, iOutBuffer, dataItemStartByteIndex[11], dataItemStartBitIndex[11],
             dataItemEndByteIndex[11], dataItemEndBitIndex[11]);
         
         /* MCBPC    */
         if (iColorEffect)
         {
             /* MCBPC Changed. Ignore the old value (not used) */
             vdtGetIMBBlackAndWhiteMCBPC(newMCBPCLen, newMCBPC, MBinstance->mcbpc);
             sPutBits(iOutBuffer, newMCBPCLen, newMCBPC);
         }
         else
         {
             vdtCopyBuffer(iInBuffer, iOutBuffer,
                 dataItemStartByteIndex[0], dataItemStartBitIndex[0],
                 dataItemEndByteIndex[0], dataItemEndBitIndex[0]);
         }
         
         /* DQUANT, if it exsits */
         vdtCopyBuffer(iInBuffer, iOutBuffer,
                   dataItemStartByteIndex[1], dataItemStartBitIndex[1],
                     dataItemEndByteIndex[1], dataItemEndBitIndex[1]);
         
         /* INTRA DCs */
         if (!iColorEffect)
         {
             vdtCopyBuffer(iInBuffer, iOutBuffer,
                 dataItemStartByteIndex[4], dataItemStartBitIndex[4],
                 dataItemEndByteIndex[4], dataItemEndBitIndex[4]);
         }
         else
         {
             if (!(VDT_NO_DATA(dataItemStartByteIndex[4], dataItemStartBitIndex[4],
                 dataItemEndByteIndex[4], dataItemEndBitIndex[4])))   
             {
                 vdtCopyBuffer(iInBuffer, iOutBuffer,
                     dataItemStartByteIndex[4], dataItemStartBitIndex[4],
                     dataItemStartByteIndex[8], dataItemStartBitIndex[8]);
                 ResetMPEG4IntraDcUV();
             }
         }
     }
     
     /* DC marker */
     sPutBits(iOutBuffer, DC_MARKER_LENGTH, DC_MARKER);
     
   for (dlstHead(iMBList, (void **) &MBinstance); 
     MBinstance != NULL; 
     dlstNext(iMBList, (void **) &MBinstance))
      {
         dataItemStartByteIndex = MBinstance->DataItemStartByteIndex;
         dataItemStartBitIndex  = MBinstance->DataItemStartBitIndex;
         dataItemEndByteIndex   = MBinstance->DataItemEndByteIndex;
         dataItemEndBitIndex    = MBinstance->DataItemEndBitIndex;
         
         /* ac_pred_flag */
         vdtCopyBuffer(iInBuffer, iOutBuffer,
                   dataItemStartByteIndex[3], dataItemStartBitIndex[3],
                     dataItemEndByteIndex[3], dataItemEndBitIndex[3]);
         
         /* CBPY */
         vdtCopyBuffer(iInBuffer, iOutBuffer,
                   dataItemStartByteIndex[2], dataItemStartBitIndex[2],
                     dataItemEndByteIndex[2], dataItemEndBitIndex[2]);
      }
     
      /* make sure the head of the list is reset */
      dlstHead(iMBList, (void **) &MBinstance);
}

/*
* ReconstructPMBPartitions
*
* Parameters: 
*
* Function:
*    Recontruct the partitions for color effect when we are not doing format transcoding
* Returns:
*    None
* Error codes:
*    None
*
*/
void CMPEG4Transcoder::ReconstructPMBPartitions()
{
    vdxPMBListItem_t *MBinstance;
    int *dataItemStartByteIndex;
    int *dataItemStartBitIndex;
    int *dataItemEndByteIndex;
    int *dataItemEndBitIndex;
    int newMCBPCLen = 0;
    int newMCBPC = 0;
     
    VDTASSERT(iMBList);
     
  for (dlstHead(iMBList, (void **) &MBinstance); 
    MBinstance != NULL; 
    dlstNext(iMBList, (void **) &MBinstance))
    {
        dataItemStartByteIndex = MBinstance->DataItemStartByteIndex;
        dataItemStartBitIndex  = MBinstance->DataItemStartBitIndex;
        dataItemEndByteIndex   = MBinstance->DataItemEndByteIndex;
        dataItemEndBitIndex    = MBinstance->DataItemEndBitIndex;
        
        /* MB stuffing bits if they exsit */
        vdtCopyBuffer(iInBuffer, iOutBuffer, dataItemStartByteIndex[11], dataItemStartBitIndex[11],
            dataItemEndByteIndex[11], dataItemEndBitIndex[11]);
        
        if (MBinstance->fCodedMB)
        {
            /* output  1 bit COD, coded */
            sPutBits (iOutBuffer, 1, 0);
            
            /* MCBPC     */
            if (iColorEffect)
            {
                /* MCBPC Changed, ignore the return value */
                vdtGetPMBBlackAndWhiteMCBPC(newMCBPCLen, newMCBPC, MBinstance->mcbpc);
                sPutBits(iOutBuffer, newMCBPCLen, newMCBPC);
            }
            else
            {
                vdtCopyBuffer(iInBuffer, iOutBuffer,
                    dataItemStartByteIndex[0], dataItemStartBitIndex[0],
                    dataItemEndByteIndex[0], dataItemEndBitIndex[0]);
            }
            
            /* MVs, if they exist */
            vdtCopyBuffer(iInBuffer, iOutBuffer,
                dataItemStartByteIndex[10], dataItemStartBitIndex[10],
                dataItemEndByteIndex[10], dataItemEndBitIndex[10]);
        }
        else
        {
            /* output  1 bit COD, not coded */
            sPutBits (iOutBuffer, 1, 1);
        }
    }
     
    /* MM marker */
    sPutBits(iOutBuffer, MOTION_MARKER_LENGTH, MOTION_MARKER);
     
    for (dlstHead(iMBList, (void **) &MBinstance); 
    MBinstance != NULL; 
    dlstNext(iMBList, (void **) &MBinstance))
    {
        if (MBinstance->fCodedMB)
        {
            dataItemStartByteIndex = MBinstance->DataItemStartByteIndex;
            dataItemStartBitIndex  = MBinstance->DataItemStartBitIndex;
            dataItemEndByteIndex   = MBinstance->DataItemEndByteIndex;
            dataItemEndBitIndex    = MBinstance->DataItemEndBitIndex;
            
            /* ac_pred_flag, if it exsits */
            vdtCopyBuffer(iInBuffer, iOutBuffer,
                dataItemStartByteIndex[3], dataItemStartBitIndex[3],
                dataItemEndByteIndex[3], dataItemEndBitIndex[3]);
            
            /* CBPY,  */
            vdtCopyBuffer(iInBuffer, iOutBuffer,
                dataItemStartByteIndex[2], dataItemStartBitIndex[2],
                dataItemEndByteIndex[2], dataItemEndBitIndex[2]);
            /* DQUANT, if it exsits */
            vdtCopyBuffer(iInBuffer, iOutBuffer,
                dataItemStartByteIndex[1], dataItemStartBitIndex[1],
                dataItemEndByteIndex[1], dataItemEndBitIndex[1]);
            
            /* INTRA DCs, if they exsit */
            if (!iColorEffect)
            {
                vdtCopyBuffer(iInBuffer, iOutBuffer,
                    dataItemStartByteIndex[4], dataItemStartBitIndex[4],
                    dataItemEndByteIndex[4], dataItemEndBitIndex[4]);
            }
            else
            {
                if (!(VDT_NO_DATA(dataItemStartByteIndex[4], dataItemStartBitIndex[4],
                    dataItemEndByteIndex[4], dataItemEndBitIndex[4])))   
                {
                    vdtCopyBuffer(iInBuffer, iOutBuffer,
                        dataItemStartByteIndex[4], dataItemStartBitIndex[4],
                        dataItemStartByteIndex[8], dataItemStartBitIndex[8]);
                    ResetMPEG4IntraDcUV();
                }
            }
        }
    }
     
    /* make sure the head of the list is reset */
    dlstHead(iMBList, (void **) &MBinstance);
}

/*
* ResetMPEG4IntraDcUV
*
* Parameters: 
*
* Function:
*    This function resets the DCc for U V block in INTRA MB.
*    Inputs are valid only with Color Effect
* Returns:
*    None
* Error codes:
*    None
*
*/
void CMPEG4Transcoder::ResetMPEG4IntraDcUV()
{
    /* set INTRADC for u,v in the output buffer */
    TInt sizeU, sizeCodeU, sizeCodeLengthU, valueU, valueCodeU, valueCodeLengthU;
    TInt sizeV, sizeCodeV, sizeCodeLengthV, valueV, valueCodeV, valueCodeLengthV;
    TInt curDcScaler, delta; 
    TReal realDelta, realVal;
    TInt mbh, mbv, mbd;  // previous MB in the horizontal, vertical and diagonal directions
    TInt codeMB; 
    
    // initialize for codewords
    sizeU = sizeCodeU = sizeCodeLengthU = valueCodeU = valueCodeLengthU = 0;
    sizeV = sizeCodeV = sizeCodeLengthV = valueCodeV = valueCodeLengthV = 0;

    // initialize for prediction
    mbh = mbv = mbd = VDX_MB_INTER;
    codeMB = 0;
    
    // NOTE: VDX_MB_INTER=1, VDX_MB_INTRA=2, NOT-CODED MB has a value of 0
    if (iVopCodingType == VDX_VOP_TYPE_I)
    {
        /* encode intra DC coefficients for INTRA MBs of I-VOP if 
           either of the following is true:
            -
            -
            else, do not encode intra DC
           (because rest of MBs have differential intra DC, which is zero)
        */
        if (!iCurMBNumInVP || 
             (!(iCurMBNum%iMBsinWidth) && (iCurMBNumInVP<iMBsinWidth)))
        {
            codeMB = 1;
        }
    }
    else if (iVopCodingType == VDX_VOP_TYPE_P)
    {
        if (iCurMBNumInVP>iMBsinWidth)
        { 
            if (iCurMBNum%iMBsinWidth)
            {
                mbh = iMBType[iCurMBNum-1];
                mbd = iMBType[iCurMBNum-iMBsinWidth-1];
            }
            mbv = iMBType[iCurMBNum-iMBsinWidth];
        }        
        else if (iCurMBNumInVP==iMBsinWidth)
        {
            if (iCurMBNum%iMBsinWidth)
            {
                mbh = iMBType[iCurMBNum-1];
            }
            mbv = iMBType[iCurMBNum-iMBsinWidth];
        }
        else if (iCurMBNumInVP>0)
        {
            if (iCurMBNum%iMBsinWidth)
            {
                mbh = iMBType[iCurMBNum-1];
            }
        }
      
        // 
        if ((mbh<VDX_MB_INTRA && mbv<VDX_MB_INTRA) ||
            (mbd==VDX_MB_INTRA && ((mbh==VDX_MB_INTRA && mbv<VDX_MB_INTRA) || 
            (mbh<VDX_MB_INTRA && mbv==VDX_MB_INTRA))))
        {
            codeMB = 1;
        }
    }

    if (codeMB)  // if IntraDC need to be coded
    {
        // color-toned U,V values
        valueU = iColorToneU;
        valueV = iColorToneV;
      
        // compensate for different QP than original
        if (iCurQuant != iRefQuant)  
        {
            // calculate change in dc value
            curDcScaler = GetMpeg4DcScalerUV(iCurQuant);
            realDelta = TReal(iDcScaler-curDcScaler)/TReal(curDcScaler);
            if (realDelta != 0.0)
            {
                // U
                realVal = realDelta*TReal(valueU);
                delta = TInt(realVal + ((realVal<0) ? (-0.5) : (0.5)));
                valueU += delta;
                // V
                realVal = realDelta*TReal(valueV);
                delta = TInt(realVal + ((realVal<0) ? (-0.5) : (0.5)));
                valueV += delta;
            }
        }
         
        // get codewords
        GetMPEG4IntraDcCoeffUV(valueU, sizeU, sizeCodeU, sizeCodeLengthU, 
        valueCodeU, valueCodeLengthU);
        GetMPEG4IntraDcCoeffUV(valueV, sizeV, sizeCodeV, sizeCodeLengthV, 
        valueCodeV, valueCodeLengthV);
      
        // code codewords
        // U
        sPutBits(iOutBuffer, sizeCodeLengthU, sizeCodeU); // dct_dc_coeff size
        if (sizeCodeU != 3) // size=0
        {
            sPutBits(iOutBuffer, valueCodeLengthU, valueCodeU); // dct_dc_coeff differential
            if (valueCodeLengthU>8)
                sPutBits(iOutBuffer, 1, 1); // marker bit
        }
      
        // V
        sPutBits(iOutBuffer, sizeCodeLengthV, sizeCodeV); // dct_dc_coeff size
        if (sizeCodeV != 3) // size=0
        {
            sPutBits(iOutBuffer, valueCodeLengthV, valueCodeV); // dct_dc_coeff differential
            if (valueCodeLengthV>8)
                sPutBits(iOutBuffer, 1, 1); // marker bit
        }
    }    
    else
    {        
        sPutBits (iOutBuffer, 2, 3); /* U */
        sPutBits (iOutBuffer, 2, 3); /* V */
    }
}


/*
* GetMPEG4IntraDcCoeffUV
*
* Parameters: 
*     aValue   coefficient value
*     aDCAC    pointer the reconstructed coefficients
* Function:
*    This function fills the reconstructed DCAC values for INTRA block.
*    Inputs are valid only with Color Effect
* Returns:
*    None
* Error codes:
*    None
*
*/
void CMPEG4Transcoder::GetMPEG4IntraDcCoeffUV(TInt aValue, TInt& aSize, 
  TInt& aSizeCode, TInt& aSizeCodeLength, TInt& aValueCode, TInt& aValueCodeLength)
{
    int absVal = (aValue>=0 ? aValue : -aValue);
    // size of aValueCode
    for (aSize=0; absVal|0; absVal>>=1, aSize++) ;
    if (aSize)
    {
        // codeword for aSize
        if (aSize==1)
        {
            aSizeCode = 2;
            aSizeCodeLength = 2;
        }
        else
        {
            aSizeCode = 1;
            aSizeCodeLength = aSize;
        }
    
        // codeword for aValue
        aValueCode = aValue;
        if (aValue<0)
            aValueCode += ((1<<aSize)-1);
        aValueCodeLength = aSize;
    }
    else 
    {
        // codeword for aSize
        aSizeCode = 3;        // codeword for size=0
        aSizeCodeLength = 2;
        // no codeword for aValue
        aValueCode = aValueCodeLength = 0;
    }
}


/*
* AddOneBlockDCACrecon
*
* Parameters: 
*     aIndex   block index
*     aDCAC    pointer the reconstructed coefficients
* Function:
*    This function fills the reconstructed DCAC values for INTRA block.
*    Inputs are valid only with Color Effect
* Returns:
*    None
* Error codes:
*    None
*
*/
void CMPEG4Transcoder::AddOneBlockDCACrecon(int aIndex, int *aDCAC)
{
    VDTASSERT(aIndex >= 0 && aIndex < 6); 
    
    if (aDCAC && iDoModeTranscoding && (iTargetFormat == EVedVideoTypeH263Profile0Level10 ||
                                        iTargetFormat == EVedVideoTypeH263Profile0Level45))
    {
      /* we only need the reconstructed DCACs for MPEG4 -> H263 */
      /* It is a coded block    */
      memcpy(iDCTBlockData + (aIndex << 6), aDCAC, sizeof(int) * BLOCK_COEFF_SIZE);
    }
}

/*
* ConstructH263MBData
*
* Parameters: 
*     aNewMCBPCLen     new length of mcbpc
*     aNewMCBPC        new mcbpc
* Function:
*    This function creates a new H263 MB
*    Inputs are valid only with Color Effect
* Returns:
*    VDC error codes
*
*/
int CMPEG4Transcoder::ConstructH263MBData(dmdPParam_t *aParam, int /*aNewMCBPCLen*/, int /*aNewMCBPC*/)
{
    /* MB data part1: output MCBPC, CBPY, DQuant, MV, intra DC etc */
    int *dataItemStartByteIndex;
    int *dataItemStartBitIndex;
    int *dataItemEndByteIndex;
    int *dataItemEndBitIndex;
    int quant, dquant;
    int codedBlockPattern = 0;
    
    const unsigned int sDquant[5] = 
    {
        1, 0, (unsigned int)65536, 2, 3
    };
    
  if (iVopCodingType == VDX_VOP_TYPE_P) 
    {
        dataItemStartByteIndex = iCurPMBinstance->DataItemStartByteIndex;
        dataItemStartBitIndex  = iCurPMBinstance->DataItemStartBitIndex;
        dataItemEndByteIndex   = iCurPMBinstance->DataItemEndByteIndex;
        dataItemEndBitIndex    = iCurPMBinstance->DataItemEndBitIndex;
        quant = iCurPMBinstance->quant;
        dquant = iCurPMBinstance->dquant;
    }
    else
    {
        dataItemStartByteIndex = iCurIMBinstance->DataItemStartByteIndex;
        dataItemStartBitIndex  = iCurIMBinstance->DataItemStartBitIndex;
        dataItemEndByteIndex   = iCurIMBinstance->DataItemEndByteIndex;
        dataItemEndBitIndex    = iCurIMBinstance->DataItemEndBitIndex;
        quant = iCurIMBinstance->quant;
        dquant = iCurIMBinstance->dquant;
    }
    
    if (iPreQuant != quant - dquant)
    {
        /* last quant in MPEG4 and H263 is different, dquant and VLCs may not be reused */
        if (abs(quant - iPreQuant) > 2)
        {
            /* VLCs cannot be reused, obtain the new ones */
            sQuantizeMB(iDCTBlockData, quant, iPreQuant, 
                iMBCodingType, iColorEffect);
            quant = iPreQuant;
            dquant = 0;
        }
        else
        {
            /* VLCs can be reused, but need to change dquant and MCBPC */
            dquant = quant - iPreQuant;
        }
    }
    
    /* MB stuffing bits if they exsit */
    vdtCopyBuffer(iInBuffer, iOutBuffer, dataItemStartByteIndex[11], dataItemStartBitIndex[11],
        dataItemEndByteIndex[11], dataItemEndBitIndex[11]);
    
    if (iMBCodingType == VDX_MB_INTER) 
    {
        VDTASSERT(aParam);
        tMBPosition mbPos;
        tMotionVector mvTestOutside;
        mbPos.x = aParam->xPosInMBs * 16; 
        mbPos.y = aParam->yPosInMBs * 16;
        mbPos.LeftBound = 0;
        mbPos.RightBound = iVDEInstance->lumWidth << 1;
        mbPos.TopBound = 0;
        mbPos.BottomBound = iVDEInstance->lumHeight << 1;
        mvTestOutside.mvx = (int16) ((aParam->mvx[0] << 1) / 10);
        mvTestOutside.mvy = (int16) ((aParam->mvy[0] << 1) / 10);
        
        /* Three cases for MVs. 1): 4MVs -> need mapping 2). 1 outside frame MV 3) rounding type = 1 */
        vdcInstance_t * vdcTemp = (vdcInstance_t *)(iVDEInstance->vdcHInstance);
        
        /* Two cases for MVs. 1): 4MVs -> need mapping 2). 1 outside frame MV */
        if (iCurPMBinstance->numMVs == 4 || (iCurPMBinstance->numMVs == 1 && 
            vbmMVOutsideBound(&mbPos, &mvTestOutside, 1)) || vdcTemp->pictureParam.rtype)
        {
            int32 numTextureBits;
            int32 searchRange = 16;
            tMotionVector   *initPred; 
            (h263mbi+iCurMBNum)->QuantScale = (int16) quant;
            (h263mbi+iCurMBNum)->dQuant = (int16) dquant;
            
            /* note: this buffer is also used in the diamond search and half-pixel search !!!!! 
               which needs at least 8 points
            */
            initPred = (tMotionVector*) malloc(8 * sizeof (tMotionVector));
            if(!initPred)
            {
                //Memory not available 
                return TX_ERR;
            }

                        
            for (int i = 0; i < iCurPMBinstance->numMVs ; i++)
            {
                (initPred + i)->mvx = (int16) (aParam->mvx[i] / 10); /* the recorded mv is multipied by 10, */
                (initPred + i)->mvy = (int16) (aParam->mvy[i] / 10); /* the recorded mv is multipied by 10, */
            }
            int32 noOfPredictors = iCurPMBinstance->numMVs;
            
            /* perform the 4MVs -> 1MV mapping, and output this MB */
            vbmPutInterMB(&mbPos,
                iOutBuffer,aParam,
                initPred, noOfPredictors,
                (u_int32) iVDEInstance->lumWidth, (u_int32) iVDEInstance->lumHeight,
                searchRange, iCurMBNum, &numTextureBits, 
                (int16)iColorEffect, h263mbi);
            /*  the MVs buffer is updated inside vbmPutInterMB */
            
            if (initPred)
                free(initPred);
        }
        else
        {
            /* Here, for Inter MB with One inside Frame MV, we simply reuse the DCTs */
            VDTASSERT(iCurPMBinstance->numMVs == 1);
            
            /* It is a coded MB, output 1 bit COD (it always exists in P frame) */
            sPutBits (iOutBuffer, 1, 0);
            int         cbpy;
            int         mcbpcVal;
            int         len;
            
            codedBlockPattern = sFindCBP(iDCTBlockData, OFF);
            mcbpcVal = iColorEffect? 0 : (codedBlockPattern & 3);
            cbpy = ((codedBlockPattern >> 2) & 0xf);
            vbmGetH263PMCBPC(dquant, iColorEffect, cbpy, mcbpcVal, len);
            sPutBits(iOutBuffer, len, mcbpcVal); //MCBPC, CBPY
            
            /* DQUANT, if it exsits */
            if (dquant)
            {
                sPutBits(iOutBuffer, 2, sDquant[dquant + 2]);
            }
            /* recode MVs, one more indice exists in the MV VLC table in MPEG4
               moreover, vop_fcode can be larger than 1 (although it may rarely happens), 
                 so it is better to redo the VLC coding
      */
            /* the recorded mv is multipied by 10, in pixel unit */
            int16 mvx = (int16) ((aParam->mvx[0] << 1) / 10); 
            int16 mvy = (int16) ((aParam->mvy[0] << 1) / 10);
            
            tMotionVector   lPredMV[4];
            tMBInfo *mbi = h263mbi + iCurMBNum;
            
            /* get the new predicted MV */
            vbmMvPrediction(mbi, iCurMBNum, lPredMV, (u_int32)iMBsinWidth);
            
            vbmEncodeMVDifferential(mvx - lPredMV[0].mvx, mvy - lPredMV[0].mvy, 1, iOutBuffer);
            cbpy = 32;
            /* following is the block level data */
            for (int i = 0; i < (iColorEffect? 4 : 6); i++)
            {
                int fBlockCoded = cbpy & codedBlockPattern;
                if (fBlockCoded)
                {
                    /* here we do the VLC coding again */ 
                    int numTextureBits = 0;
                    vdtPutInterMBCMT(iOutBuffer,0, iDCTBlockData + i * BLOCK_COEFF_SIZE, &numTextureBits, ON);
                }
                cbpy >>= 1;
            } 
            
            /* update the MVs buffer */
            (h263mbi + iCurMBNum)->MV[0][0] = (int16) ((aParam->mvx[0] << 1) / 10); /* the recorded mv is multipied by 10 */
            (h263mbi + iCurMBNum)->MV[0][1] = (int16) ((aParam->mvy[0] << 1) / 10); /* the recorded mv is multipied by 10 */
            
        }  /* end of if 4MVs */
    }
    else /* INTRA MB */
    {
        /* update the MVs buffer */
        (h263mbi + iCurMBNum)->MV[0][0] = 0;
        (h263mbi + iCurMBNum)->MV[0][1] = 0;
        /* MPEG4 and H263 use different methods for INTRA MB, redo the VLC coding */
        int         cbpy;
        int         mcbpcVal;
        int         len;
        
        codedBlockPattern = sFindCBP(iDCTBlockData, ON);
        mcbpcVal = iColorEffect? 0 : (codedBlockPattern & 3);
        cbpy = ((codedBlockPattern >> 2) & 0xf);
        vbmGetH263IMCBPC(dquant, (iVopCodingType == VDX_VOP_TYPE_P), iColorEffect, cbpy, mcbpcVal, len);
        sPutBits(iOutBuffer, len, mcbpcVal); //COD, MCBPC, CBPY
        
        /* DQUANT, if it exsits */
        if (dquant)
        {
            sPutBits(iOutBuffer, 2, sDquant[dquant + 2]);
        }
        cbpy = 32;
        
        /* following is the block level data */
        for (int i = 0; i < (iColorEffect? 4 : 6); i++)
        {
            /* requantize INTRA DC */
            /* DC Quantization */
            int coeff = (iDCTBlockData + i * BLOCK_COEFF_SIZE)[0] >> 3;
            
            if(coeff < 1) coeff = 1;
            if(coeff > 254) coeff = 254;
            if(coeff == 128)
            {
                sPutBits(iOutBuffer, 8, 255);
            }
            else
            {
                sPutBits(iOutBuffer, 8, coeff);
            }
            
            (iDCTBlockData + i * BLOCK_COEFF_SIZE)[0] = coeff;
            if(cbpy & codedBlockPattern)
            {
                int numTextureBits = 0;
                vdtPutInterMBCMT(iOutBuffer,1, iDCTBlockData + i * BLOCK_COEFF_SIZE, &numTextureBits, ON);
            }
            cbpy >>= 1;
        }
    }
    
  if (iColorEffect && (iMBCodingType == VDX_MB_INTRA))
    {
        ResetH263IntraDcUV(iOutBuffer, iColorToneU, iColorToneV);        
    }
  iPreQuant = quant;
  
  return TX_OK;
}

/*
* ConstructRegularMPEG4MBData
*
* Parameters: 
*     aNewMCBPCLen     new length of mcbpc
*     aNewMCBPC        new mcbpc
* Function:
*    This function rearranges the data for bitstream with data partitioning
*    Only valid in Data Partitioned mode
*    Inputs are valid only with Color Effect
* Returns:
*    None
* Error codes:
*    None
*
*/
void CMPEG4Transcoder::ConstructRegularMPEG4MBData(int aNewMCBPCLen, int aNewMCBPC)
{
  /* MB data part1: output MCBPC, CBPY, DQuant, MV, intra DC etc */
    int *dataItemStartByteIndex;
    int *dataItemStartBitIndex;
    int *dataItemEndByteIndex;
    int *dataItemEndBitIndex;
    
    if (iVopCodingType == VDX_VOP_TYPE_P) 
    {
        dataItemStartByteIndex = iCurPMBinstance->DataItemStartByteIndex;
        dataItemStartBitIndex  = iCurPMBinstance->DataItemStartBitIndex;
        dataItemEndByteIndex   = iCurPMBinstance->DataItemEndByteIndex;
        dataItemEndBitIndex    = iCurPMBinstance->DataItemEndBitIndex;
    }
    else
    {
        dataItemStartByteIndex = iCurIMBinstance->DataItemStartByteIndex;
        dataItemStartBitIndex  = iCurIMBinstance->DataItemStartBitIndex;
        dataItemEndByteIndex   = iCurIMBinstance->DataItemEndByteIndex;
        dataItemEndBitIndex    = iCurIMBinstance->DataItemEndBitIndex;
    }
    
    /* MB stuffing bits if they exsit */
    vdtCopyBuffer(iInBuffer, iOutBuffer, dataItemStartByteIndex[11], dataItemStartBitIndex[11],
        dataItemEndByteIndex[11], dataItemEndBitIndex[11]);
    
    if (iVopCodingType == VDX_VOP_TYPE_P)
    {
        /* It is a coded MB, output  1 bit COD (it always exists in P frame) */
        sPutBits (iOutBuffer, 1, 0);
    }
    
    /* MCBPC. NOTE: the positions do not include MCBPC stuffing bits !! */
    if (iColorEffect)
    {
        /* MCBPC Changed */
        sPutBits(iOutBuffer, aNewMCBPCLen, aNewMCBPC);
    }
    else
    {
        vdtCopyBuffer(iInBuffer, iOutBuffer, dataItemStartByteIndex[0], dataItemStartBitIndex[0],
            dataItemEndByteIndex[0], dataItemEndBitIndex[0]);
    }
    /* ac_pred_flag, if it exsits */
    vdtCopyBuffer(iInBuffer, iOutBuffer, dataItemStartByteIndex[3], dataItemStartBitIndex[3],
        dataItemEndByteIndex[3], dataItemEndBitIndex[3]);
    
    /* CBPY */
    vdtCopyBuffer(iInBuffer, iOutBuffer, dataItemStartByteIndex[2], dataItemStartBitIndex[2],
        dataItemEndByteIndex[2], dataItemEndBitIndex[2]);
    
    /* DQUANT, if it exsits */
    vdtCopyBuffer(iInBuffer, iOutBuffer, dataItemStartByteIndex[1], dataItemStartBitIndex[1],
        dataItemEndByteIndex[1], dataItemEndBitIndex[1]);
    
    if (iMBCodingType == VDX_MB_INTER) 
    {
        /* MVs, if they exsit */
        vdtCopyBuffer(iInBuffer, iOutBuffer, dataItemStartByteIndex[10], dataItemStartBitIndex[10],
            dataItemEndByteIndex[10], dataItemEndBitIndex[10]);
    }
    
    /* following is the block level data */
    for (int i = 0; i < (iColorEffect? 4 : 6); i++)
    {
        /* INTRA DC, if it exsits */
        vdtCopyBuffer(iInBuffer, iOutBuffer, dataItemStartByteIndex[i + 4], dataItemStartBitIndex[i + 4],
            dataItemEndByteIndex[i + 4], dataItemEndBitIndex[i + 4]);
        
        /* block data part2, AC or DCT coefficients */
        if (iBitStreamMode == EVedVideoBitstreamModeMPEG4DP_RVLC || iBitStreamMode == EVedVideoBitstreamModeMPEG4Resyn_DP_RVLC)
        {
        /* remember for data partitioning, the positions only indicate the  
        AC or DCTs coefficients
            */
            if (VDT_NO_DATA(iBlockStartByteIndex[i], iBlockStartBitIndex[i],
                iBlockEndByteIndex[i], iBlockEndBitIndex[i]))  
            {
                /* no coefficients,skip this block */
                continue;
            }
            else
            {
                /* for RVLC coding, we transform the block data back to VLC */
                int numTextureBits = 0;
                /* redo the entropy coding, RVLC -> VLC 
                only for AC (IMB) or DCT (PMB) coefficients
                */
                if (iMBCodingType == VDX_MB_INTRA)
                {
                    vdtPutIntraMBCMT(iOutBuffer,iDCTBlockData + i * BLOCK_COEFF_SIZE, &numTextureBits, i, 1, 0);
                }
                else
                {
                    vdtPutInterMBCMT(iOutBuffer,0, iDCTBlockData + i * BLOCK_COEFF_SIZE, &numTextureBits, OFF);
                }
            }
        }
        else /* ouput the AC or DCT coefficients */
        {
            bufEdit.copyMode = CopyWhole; /* CopyWhole */
            vdtCopyBuffer(iInBuffer,iOutBuffer,
                iBlockStartByteIndex[i],iBlockStartBitIndex[i],
                iBlockEndByteIndex[i],iBlockEndBitIndex[i]);
        }
    }
  if (iColorEffect && (iMBCodingType == VDX_MB_INTRA))
    {
        ResetMPEG4IntraDcUV();
    }
}



/*
* AddOneBlockDataToMB
*
* Parameters: 
*     blockData        block data before VLC coding, in ZigZag order
* Function:
*    This function input one block data to current MB
*    only here the whole MB data is retrieved (COD=0)
* Returns:
*    None
* Error codes:
*    None
*
*/
void CMPEG4Transcoder::AddOneBlockDataToMB(int aBlockIndex, int *aBlockData)
{
    VDTASSERT(aBlockIndex >= 0 && aBlockIndex < 6); 
    
    
    if (aBlockData && iDoModeTranscoding)
    {
      /* iDCTBlockData is only used when we need to convert the bitstream to MPEG4_RESYN
      It is a coded block   
        */
      if (iMBCodingType == VDX_MB_INTRA && (iTargetFormat == EVedVideoTypeH263Profile0Level10 ||
                                            iTargetFormat == EVedVideoTypeH263Profile0Level45))
        {
          /* we only need the reconstructed DCACs, skipped */
        }
        else
        {
          memcpy(iDCTBlockData + (aBlockIndex << 6), aBlockData, sizeof(int) * BLOCK_COEFF_SIZE);
        }
    }
    else 
    {
      memset(iDCTBlockData + (aBlockIndex << 6), 0, sizeof(int) * BLOCK_COEFF_SIZE);
    }
    

    
    iBlockEndByteIndex[aBlockIndex] = iInBuffer->getIndex;
    iBlockEndBitIndex[aBlockIndex] = iInBuffer->bitIndex;
}

/*
* ErrorResilienceInfo
*
* Parameters: 
*     header        VOL Header data
* Function:
*    This function records the position of resnc_marker_disable bit
* Returns:
*    None
* Error codes:
*    None
*
*/
void CMPEG4Transcoder::ErrorResilienceInfo(vdxVolHeader_t *header, int aByte, int aBit)
{
    if (header)
    {
                
        memcpy(&iVOLHeader, header, sizeof(vdxVolHeader_t));  // save the header info

    }
    else
    {
        iErrorResilienceStartByteIndex = aByte;  /* save the bits position */
        iErrorResilienceStartBitIndex  = aBit;
    }
}



/*
* MPEG4TimerResolution
*
* Parameters: 
*     
* Function:
*    This function records the position of vop_time_increment_resolution bit
* Returns:
*    None
* Error codes:
*    None
*
*/
void CMPEG4Transcoder::MPEG4TimerResolution(int aStartByteIndex, int aStartBitIndex)
{
    iTimeResolutionByteIndex = aStartByteIndex;
    iTimeResolutionBitIndex  = aStartBitIndex;
}



void CMPEG4Transcoder::ConstructVOSHeader(int aMPEG4, vdeDecodeParamters_t *aDecoderInfo)
{
  if (!aMPEG4 )
  {
    /* for H263 and MPEG4 shortheader, no vos generated */
    if (iErrorResilienceStartByteIndex == KDataNotValid && 
      iErrorResilienceStartBitIndex == KDataNotValid)
    {
      iBitStreamMode = EVedVideoBitstreamModeH263; // pure H.263
      aDecoderInfo->vosHeaderSize = aDecoderInfo->fModeChanged? KH263ToMpeg4VosSize : 0; 
    }
    else
    {
      /* we went into the VOL layer. It is a MPEG4 bitstream with short header */
      iBitStreamMode = EVedVideoBitstreamModeMPEG4ShortHeader; /* MPEG4 shortheader */
      iShortHeaderEndByteIndex = iInBuffer->getIndex;
      iShortHeaderEndBitIndex  = iInBuffer->bitIndex;
      aDecoderInfo->vosHeaderSize = aDecoderInfo->fModeChanged? KH263ToMpeg4VosSize : KShortHeaderMpeg4VosSize; 
    }
    fFirstFrameInH263 = ETrue;
  }
  else 
  {
    iNumMBsInOneVOP = (iVDEInstance->lumHeight * iVDEInstance->lumWidth) / 256;
    
    /* even iBitStreamMode is given outside, we renew it here anyway */
    iBitStreamMode = sGetMPEG4Mode(iVOLHeader.error_res_disable,
      iVOLHeader.data_partitioned, iVOLHeader.reversible_vlc);
    
    if (iTargetFormat != EVedVideoTypeH263Profile0Level10 &&
      iTargetFormat != EVedVideoTypeH263Profile0Level45) /* EVedVideoTypeMPEG4SimpleProfile or None */
    {
      /* copy from the begining of the input buffer */
      vdtCopyBuffer(iInBuffer,iOutBuffer,0,7, iTimeResolutionByteIndex, iTimeResolutionBitIndex); 
      /* it is 16 bits */
      sPutBits (iOutBuffer, KMpeg4VopTimeIncrementResolutionLength, *aDecoderInfo->aMPEG4TargetTimeResolution); 
      
      int startByteIndex, startBitIndex;
      startByteIndex = iTimeResolutionByteIndex + 2;
      startBitIndex  = iTimeResolutionBitIndex;
      
      sPutBits(iOutBuffer, 1, MARKER_BIT);
      /* close fixed_vop_rate */
      sPutBits (iOutBuffer, 1, 0); /* it is 1 bit */
      
      int num_bits = 0;
      vdcInstance_t * vdcTemp = (vdcInstance_t *)(iVDEInstance->vdcHInstance);
      if (vdcTemp->pictureParam.fixed_vop_rate)
      {
        for (num_bits = 1; ((vdcTemp->pictureParam.time_increment_resolution-1) >> num_bits) != 0; num_bits++)
            {
            }
      }
      num_bits += 2;
      
      /* following is to skip the fixed_vop_rate */
      int bitsRemain, bitShift = 0;
      int bitsToMove = 0 ;
      /* complete the byte */
      if (startBitIndex != 7)
      {
        bitShift = startBitIndex + 1;
        bitsToMove = (num_bits < bitShift) ? num_bits : bitShift; 
        /* update statistics to take care of bit addition or byte completion  */
        if (num_bits < bitShift)
        {
          /* bits skipped but byte not completed */
          startBitIndex -= bitsToMove;
        }
        else
        {
          /* byte completed */
          startByteIndex ++;
          startBitIndex = 7;
        }
      }
      /* full bytes to skip */
      startByteIndex += ((num_bits - bitsToMove) >> 3);
      bitsRemain = (num_bits - bitsToMove) % 8;
      
      /* the remaining bits */
      startBitIndex = ( bitsRemain != 0) ? 7 - bitsRemain : startBitIndex;

      
      /* check if we have user data in the end of VOL; it cannot be copied as such but needs to be byte aligned */
      /* first need to rewind the input buffer to be able to seek in it */
      int16 error = 0;
      int curByteIndex = iInBuffer->getIndex;
      int curBitIndex = iInBuffer->bitIndex;
      int bits = ((curByteIndex - iTimeResolutionByteIndex)<<3) + (7-curBitIndex);
      bibRewindBits(bits, iInBuffer, &error);
      int sncCode = sncSeekMPEGStartCode(iInBuffer, 
          vdcTemp->pictureParam.fcode_forward, 1 /* don't check VOPs */, 1 /* check for user data*/, &error);
        
      /* record next resync position */
      int resyncByteIndex = iInBuffer->getIndex;
        
      int stuffBits = 0;
      int userDataExists = 0;
      if ( sncCode == SNC_USERDATA )
      {
        /* copy only until this sync code, and copy the rest separately in the end. */
        userDataExists = 1;
      }
      else
      {
        /* No UD */
        /* restore the original pointers in iInBuffer */
        iInBuffer->getIndex = curByteIndex;
        iInBuffer->bitIndex = curBitIndex;
      }
      
      if (iDoModeTranscoding || aDecoderInfo->fHaveDifferentModes)
      {
        /* close the error resilience tools, change the bitstream to regular MPEG4 with resyn marker */
        int numBits = iVOLHeader.data_partitioned ? 3 : 2;
        
        bufEdit.copyMode = CopyWithEdit; 
        bufEdit.editParams[0].StartByteIndex = iErrorResilienceStartByteIndex; 
        bufEdit.editParams[0].StartBitIndex = iErrorResilienceStartBitIndex; 
        bufEdit.editParams[0].curNumBits = numBits; 
        bufEdit.editParams[0].newNumBits = 2;  
        bufEdit.editParams[0].newValue = 0;  /* new codeword: resyn, no dp, no rvlc */
        CopyStream(iInBuffer,iOutBuffer,&bufEdit, startByteIndex,startBitIndex); /* copy from vop_time_increment_resolution */

        /* rewind stuffing bits */
        int16 error;
        sncRewindStuffing(iOutBuffer, &error);        

        // stuff bits in outbuffer for next start code   
        vdtStuffBitsMPEG4(iOutBuffer);
      }
      else
      {
      
        /* rewind stuffing bits */
        sncRewindStuffing(iInBuffer, &error);        
        
        /* record the number of bits rewinded - can be from 0 to 8 */
        /* note that iInBuffer must be byte aligned before rewinding */
        if (iInBuffer->bitIndex == 7) // full byte rewind
        { 
          stuffBits = ( (int)(iInBuffer->getIndex) < resyncByteIndex) ? 8 : 0;
        }
        else 
        {
          stuffBits = iInBuffer->bitIndex + 1;
        }
          
        /* copy the rest of VOS until the first VOP (or UD) */
        bufEdit.copyMode = CopyWhole; 
        vdtCopyBuffer(iInBuffer,iOutBuffer, startByteIndex,startBitIndex,
          resyncByteIndex, iInBuffer->bitIndex); 
        
        // stuff bits in outbuffer for next start code   
        vdtStuffBitsMPEG4(iOutBuffer);
        
        // move inbuffer pointer back to original value
        bibForwardBits(stuffBits, iInBuffer);
      }
      
      
      if ( userDataExists )
      {
        /* seek for VOP start code */
        int sncCode = sncSeekMPEGStartCode(iInBuffer, 
          vdcTemp->pictureParam.fcode_forward, vdcTemp->pictureParam.error_res_disable, 0, &error);
        
        /* rewind stuffing bits */
        sncRewindStuffing(iInBuffer, &error);        
        
        // record the number of bits rewinded - can be from 0 to 8
        // note that iInBuffer must be byte aligned before rewinding
        if (iInBuffer->bitIndex == 7) // full byte rewind
        { 
          stuffBits = ( (int)(iInBuffer->getIndex) < resyncByteIndex) ? 8 : 0;
        }
        else 
        {
          stuffBits = iInBuffer->bitIndex + 1;
        }
        
        bufEdit.copyMode = CopyWhole; 
        vdtCopyBuffer(iInBuffer,iOutBuffer, resyncByteIndex, 7,
          iInBuffer->getIndex, iInBuffer->bitIndex); 
        
        // stuff bits in outbuffer for next start code   
        vdtStuffBitsMPEG4(iOutBuffer);
        
        // move inbuffer pointer back to original value
        bibForwardBits(stuffBits, iInBuffer);
        
        
        
      }
      
      aDecoderInfo->vosHeaderSize = iOutBuffer->getIndex;
    }
        
    PRINT((_L("CMPEG4Transcoder: ConstructVOSHeader. resyn: %d, data partitioned: %d, rvlc: %d, resolution: %d"),
      iVOLHeader.error_res_disable, iVOLHeader.data_partitioned, iVOLHeader.reversible_vlc,
      aDecoderInfo->iTimeIncrementResolution));
  }
  PRINT((_L("CMPEG4Transcoder:  streammode: %d, outputformat: %d, vos size : %d"), 
    iBitStreamMode, iTargetFormat, aDecoderInfo->vosHeaderSize));
}




/****************************************************************
*                                                               *
*    Functions for H.263, or H.263 -> MPEG4 (only baseline H.263) *
*                                                               *
*****************************************************************/


/* Luminance block dc-scaler value corresponding to QP values of 0-31 */
const u_int8 sLumDCScalerTbl[32] = 
{   
       0,  8,  8,  8,  8, 10, 12, 14, 
    16, 17, 18, 19, 20, 21, 22, 23, 
    24, 25, 26, 27, 28, 29, 30, 31, 
    32, 34, 36, 38, 40, 42, 44, 46 
};

/* Chrominance block dc-scaler value corresponding to QP values of 0-31 */
const u_int8 sChrDCScalerTbl[32] = 
{   
       0,  8,  8,  8,  8,  9,  9, 10, 
    10, 11, 11, 12, 12, 13, 13, 14, 
    14, 15, 15, 16, 16, 17, 17, 18, 
    18, 19, 20, 21, 22, 23, 24, 25 
};

/*
* sGetMPEG4INTRADCValue
*
* Parameters: 
*     intraDC      reconstructed intra DC from H263
*     QP           quantion factor
*     blockNum     block number (0 to 5)
*     currMBNum    current MB number
*     mbinWidth    number of MBs in picure width
*     dcData       matrix to store the INTRA DC values
*     mbVPNumber   matrix recording the video packet number for each MB
*     
* Function:
*    This function gets the new intra DC for MPEG4
* Returns:
*     INTRA DC to put into MPEG4 bitstream
* Error codes:
*    None
*
*/
int sGetMPEG4INTRADCValue(int intraDC, int blockNum, int currMBNum,
                                                    int32 QP, int32 mbinWidth, int **dcData, int *mbVPNumber)
{
    int tempDCScaler;
    int blockA = 0, blockB = 0, blockC = 0;
    int gradHor, gradVer, predDC;
    
    VDTASSERT(currMBNum >= 0);
    VDTASSERT(QP <= 31);
    
  /* Prediction blocks A (left), B (above-left), and C (above) */
    switch (blockNum)
    {
        case 0:
        case 4:
        case 5:
      /* Y0, U, and V blocks */
            if (((currMBNum % mbinWidth) == 0) || /* Left edge */
                (mbVPNumber[currMBNum - 1] != mbVPNumber[currMBNum]))
            {
                blockA = 1024; /* fixed value for H263 */
            }
            else
            {
                blockA = dcData[currMBNum - 1][blockNum > 3? blockNum : 1];
            }
            
            if (((currMBNum / mbinWidth) == 0) || /* Top Edge */
                ((currMBNum % mbinWidth) == 0) || /* Left Edge */
                (mbVPNumber[currMBNum - mbinWidth - 1] != mbVPNumber[currMBNum]))
            {
                blockB = 1024;
            }
            else
            {
                blockB = dcData[currMBNum - mbinWidth - 1][blockNum > 3? blockNum : 3];
            }
            
            if (((currMBNum / mbinWidth) == 0) || /* Top Edge */
                (mbVPNumber[currMBNum - mbinWidth] != mbVPNumber[currMBNum]))
            {
                blockC = 1024;
            }
            else
            {
                blockC = dcData[currMBNum - mbinWidth][blockNum > 3? blockNum : 2];
            }
            break;
            
        case 1:
            /* Y1 block */
            blockA = dcData[currMBNum][0];
            
            if (((currMBNum / mbinWidth) == 0) || /* Top Edge */
                (mbVPNumber[currMBNum - mbinWidth] != mbVPNumber[currMBNum]))
            {
                blockB = 1024;
                blockC = 1024;
            }
            else
            {
                blockB = dcData[currMBNum - mbinWidth][2];
                blockC = dcData[currMBNum - mbinWidth][3];
            }
            break;
            
        case 2:
            /* Y2 block */
            
            if (((currMBNum % mbinWidth) == 0) || /* Left Edge */
                (mbVPNumber[currMBNum - 1] != mbVPNumber[currMBNum]))
            {
                blockA = 1024;
                blockB = 1024;
            }
            else
            {
                blockA = dcData[currMBNum  - 1][3];
                blockB = dcData[currMBNum  - 1][1];
            }
            
            blockC = dcData[currMBNum][0];
            break;
            
        case 3:
            /* Y3 block */
            
            blockA = dcData[currMBNum][2];
            blockB = dcData[currMBNum][0];
            blockC = dcData[currMBNum][1];
            break;

        default:
            break;
    }
    
    gradHor = blockB - blockC;
    gradVer = blockA - blockB;
    
    if ((abs(gradVer)) < (abs(gradHor))) 
    {
        /* Vertical prediction (from C) */
        predDC = blockC;
    }
    else 
    {
        /* Horizontal prediction (from A) */
        predDC = blockA;
    }
    
  /* DC quantization */
  if (blockNum < 4) /* Luminance Block */
  {
        intraDC += (sLumDCScalerTbl[QP] >> 1);
        intraDC /= sLumDCScalerTbl[QP];
        
        /* update the DC data matrix
        note: for INTER MB, the entry is already preset to 1024!!
        */
        dcData[currMBNum][blockNum] = intraDC * sLumDCScalerTbl[QP];
  }
  else            /* Chrominance block */
  {
        intraDC += (sChrDCScalerTbl[QP] >> 1);
        intraDC /= sChrDCScalerTbl[QP];
        /* update the DC data matrix
        note: for INTER MB, the entry is already preset to 1024!!
        */
        dcData[currMBNum][blockNum] = intraDC * sChrDCScalerTbl[QP];
  }
    /* DC prediction */
  tempDCScaler = (blockNum<4)? sLumDCScalerTbl[QP] : sChrDCScalerTbl[QP];
    
  return (intraDC - ((predDC + tempDCScaler/2) / tempDCScaler));
}

/*
* sPutVOLHeader
*
* Parameters: 
*     bitOut       pointer to the output buffer
*     aWidth       picture width
*     aHeight      picture height
*     aTimerResolution timer resolution for MPEG4
*     
* Function:
*    This function writes the VOL header into bit-stream
* Returns:
*     None
* Error codes:
*    None
*
*/
inline void sPutVOLHeader(bibBuffer_t* bitOut, int aWidth, int aHeight, int aTimerResolution)
{
    bibBuffer_t     *lBitOut;
    uint32          vop_time_increment_resolution;
    
    lBitOut = bitOut;
    WRITE32(lBitOut, (uint32)VIDEO_OBJECT_LAYER_START_CODE);
    /* RandomAccessibleVol == 1 means all VOP's can be decoded independently */
    sPutBits(lBitOut, 1, 0);
    /* VideoObjectTypeIndication = SIMPLE OBJECT */
    sPutBits(lBitOut, 8, SIMPLE_OBJECT);
    /* IsObjectLayerIdentifier */
    sPutBits(lBitOut, 1, 0);
    /* AspectRatioInfo */
    sPutBits(lBitOut, 4, ASPECT_RATIO_INFO);
    /* vol_control_parameters = 1 */
    sPutBits(lBitOut, 1, 1);
    /* Chroma Format */
    sPutBits(lBitOut, 2, CHROMA_FORMAT);
    /* LowDelay = 1; */
    sPutBits(lBitOut, 1, 1);
    /* vbvParameters = 0; */
    sPutBits(lBitOut, 1, 0);
    /* VideoObjectLayerShape == RECTANGULAR */
    sPutBits(lBitOut, 2, RECTANGULAR);
    /* Marker Bit */
    sPutBits(lBitOut, 1, MARKER_BIT);
    /* VopTimeIncrementResolution */
    vop_time_increment_resolution = aTimerResolution;
    sPutBits(lBitOut, 16, vop_time_increment_resolution);
    /* Marker Bit */
    sPutBits(lBitOut, 1, MARKER_BIT);    
    /* FixedVOPRate = 0, not fixed */
    sPutBits(lBitOut, 1, 0);    
    /* Marker Bit */
    sPutBits(lBitOut, 1, MARKER_BIT);
    /* VideoObjectLayerWidth */
    sPutBits(lBitOut, 13, aWidth);
    /* Marker Bit */
    sPutBits(lBitOut, 1, MARKER_BIT);
    /* VideoObjectLayerHeight */
    sPutBits(lBitOut, 13, aHeight);
    /* Marker Bit */
    sPutBits(lBitOut, 1, MARKER_BIT);
    /* Interlaced = 0 */
    sPutBits(lBitOut, 1, 0);
    /* ObmcDisable= 1 */
    sPutBits(lBitOut, 1, 1);
    /* SpriteEnable = 0 */
    sPutBits(lBitOut, 1, 0);
    /* Not8Bit = 0 */
    sPutBits(lBitOut, 1, 0);
    /* QuantType = H263 (0) */
    sPutBits(lBitOut, 1, H263);
    /* Complexity Estimation Disable = 1 */
    sPutBits(lBitOut, 1, 1);
    /* ResyncMarkerDisable */
    sPutBits(lBitOut, 1, 0); /* H263 is converted to resyn_marker MPEG4 in video editor */
    /* DataPartioned */
    sPutBits(lBitOut, 1, 0); /* always 0 for H263 */
    /* Reversible VLC  closed */
    /* Scalability = 0 */
    sPutBits(lBitOut, 1, 0);
    vdtStuffBitsMPEG4(lBitOut);
    
    return;
}


/*
* sPutGOVHeader
*
* Parameters: 
*     bitOut       pointer to the output buffer
*     aModuloTimeBase  time base for MPEG4
*     
* Function:
*    This function writes the group of VOP(GOV) header into bit-stream
* Returns:
*     None
* Error codes:
*    None
*
*/
inline void sPutGOVHeader(bibBuffer_t *bitOut, int aModuloTimeBase)
{
    int32 time_code_hours;
    int32 time_code_minutes;
    int32 time_code_seconds;
    
    WRITE32(bitOut, (uint32)GROUP_OF_VOP_START_CODE);
    time_code_seconds = aModuloTimeBase;
    time_code_minutes = time_code_seconds / 60;
    time_code_hours   = time_code_minutes / 60;    
    time_code_minutes = time_code_minutes - (time_code_hours * 60);
    time_code_seconds = time_code_seconds - (time_code_minutes * 60)
        - (time_code_hours * 3600);
    
    sPutBits(bitOut, 5, time_code_hours);
    sPutBits(bitOut, 6, time_code_minutes);
    sPutBits(bitOut, 1, MARKER_BIT);
    sPutBits(bitOut, 6, time_code_seconds);
    
    /* ClosedGov */
    sPutBits(bitOut, 1, 0);
    /* Broken Link */
    sPutBits(bitOut, 1, 0);
    
    /* Stuff bits */
    vdtStuffBitsMPEG4(bitOut);
    
    return;
}

/*
* sConstructMPEG4VOSHeaderForH263
*
* Parameters: 
*     bitOut       pointer to the output buffer
*     aWidth       picture width
*     aHeight      picture height
*     
* Function:
*    This function writes the MPEG4 VOS header into bit-stream
* Returns:
*     None
* Error codes:
*    None
*
*/
void sConstructMPEG4VOSHeaderForH263(bibBuffer_t *bitOut, int aWidth, int aHeight, int aOutputMpeg4TimeRes)
{
    /* visual object sequence header */
    WRITE32(bitOut, (uint32)VISUAL_OBJECT_SEQUENCE_START_CODE);
    
    /* This is for testing for level 0,3 */
    uint32 level = 8;   /* level 0 for QCIF or less */
    if(aWidth > 176 || aHeight > 144)   /* level 2 for greater than QCIF */
        level = 2;
    sPutBits(bitOut, 8, level); /* simple profile, level 0 for H263 */
    
    /* visual object header begins */
    WRITE32(bitOut, (uint32)VISUAL_OBJECT_START_CODE);
    /* IsVisualObjectIdentifier = 0 */
    sPutBits(bitOut, 1, 0);
    /* VisualObjectType = VIDEO_OBJECT */
    sPutBits(bitOut, 4, VISUAL_OBJECT);
    /* VideoSignalType = 0 */
    sPutBits(bitOut, 1, 0);
    
    vdtStuffBitsMPEG4(bitOut);
    WRITE32(bitOut, (uint32)VIDEO_OBJECT_START_CODE);
    
    /* Writes the VOL header into bit-stream , for baseline H263 TR is marked with 29.97fps. ->? */
    sPutVOLHeader(bitOut, aWidth, aHeight, aOutputMpeg4TimeRes);
    
    /* Writes GOV , time base is 0 */
    sPutGOVHeader(bitOut, 0);
    
    return;
}


/*
* sPutVideoPacketHeader
*
* Parameters: 
*     lBitOut      output buffer
*     mbNo         Macroblock number
*     quantiserScale quant value to be put in header
*     aMBsInVOP    # of MBs in one VOP
*     
* Function:
*    This function writes the video packet header information into bit-stream
* Returns:
*     None
* Error codes:
*    None
*
*/
inline void sPutVideoPacketHeader(bibBuffer_t *lBitOut, int32 mbNo, 
                                                                    int16 quantiserScale,
                                                                    int aMBsInVOP)
{
    int32   lResyncMarkerLength = 17;
    int32   lMBNoResolution;
    
    /* ResyncMarker */
    int fCode = 1; /*always 1 for H263 */
    lResyncMarkerLength = 16 + fCode; 
    sPutBits(lBitOut, lResyncMarkerLength, 1);
    
    --aMBsInVOP;
    lMBNoResolution = 1;
    while( (aMBsInVOP =
        aMBsInVOP >> 1) > 0 )
    {
        lMBNoResolution++;
    }
    sPutBits(lBitOut, lMBNoResolution, mbNo);
    
    /* QuantScale */ 
    sPutBits(lBitOut, 5, quantiserScale);
    /* HEC */
    sPutBits(lBitOut, 1, 0); /* always 0 for H263 */
    
    return;
}

/*
* sConstructVOPHeaderForH263
*
* Parameters: 
*     lBitOut      output buffer
*     prevQuant    quant value to be put in header
*     aVOPType     picture coding type
*     aVOPTimeIncrement  time increament
*     
* Function:
*    This function writes the VOP header into bit-stream for one H263 frame
* Returns:
*     None
* Error codes:
*    None
*
*/

void sConstructVOPHeaderForH263(bibBuffer_t *lBitOut, int prevQuant, int aVOPType, int aVOPTimeIncrement,int aOutputMpeg4TimeRes)
{
    WRITE32(lBitOut, (uint32)VOP_START_CODE);
    /* VOPType, 0 for Intra and 1 for inter */
    sPutBits(lBitOut, 2, aVOPType);
    
    /* H263 TR is marked with 29.97fps */
    /* Modulo Time Base */
    int outputTimerResolution = aOutputMpeg4TimeRes;
    while ((unsigned)aVOPTimeIncrement >= (unsigned) outputTimerResolution)
    {
        sPutBits(lBitOut, 1, 1);
        aVOPTimeIncrement -= outputTimerResolution; 
    }
    sPutBits(lBitOut, 1, 0);
    /* Marker Bit */
    sPutBits(lBitOut, 1, MARKER_BIT);   
    /* VopTimeIncrement */
    int numOutputTrBits;
    for (numOutputTrBits = 1; ((outputTimerResolution-1) >> numOutputTrBits) != 0; numOutputTrBits++)
        {
        }
    int VOPIncrementResolutionLength = numOutputTrBits;
    sPutBits(lBitOut, VOPIncrementResolutionLength, aVOPTimeIncrement);
    /* Marker Bit */
    sPutBits(lBitOut, 1, MARKER_BIT);
    /* VOPCoded */
    sPutBits(lBitOut, 1, 1); /* always 1 for H263 */
    /* VOPRoundingType */
    if(aVOPType == P_VOP)
    {
        sPutBits(lBitOut, 1, 0); /* always 0 for H263 */
    }
    /* IntraDCVLCThreshold */
    sPutBits(lBitOut, 3, 0); /* always 0 for H263 */
    /* VOPQuant */
    sPutBits(lBitOut, 5, prevQuant);
    /* VOPFcodeForwad */
    if(aVOPType == P_VOP)
    {
        sPutBits(lBitOut, 3, 1); /* always 1 for H263 */
    }
    
    return;
}



/*
* H263PictureHeaderEndedL
*
* Parameters: 
*     
* Function:
*    This function is called after one H263 picture header is parsed
*    Note: the input header is only valid inside this function
* Returns:
*    VDC error codes
* Error codes:
*    None
*
*/
int CMPEG4Transcoder::H263PictureHeaderEnded(dphOutParam_t *aH263PicHeader, dphInOutParam_t *aInfo)
{
    VDTASSERT(aH263PicHeader);  
    
    // asad
    if (!aInfo->vdcInstance->frameNum || !iRefQuant)
    {
        iRefQuant = aH263PicHeader->pquant;
        iDcScaler = GetMpeg4DcScalerUV(iRefQuant); 
    }
    
    if (!iDoModeTranscoding )
    {
        
       /* Copy the header to output stream */
       bufEdit.copyMode = CopyWhole; /* CopyWhole */
         
         if ( (iTargetFormat == EVedVideoTypeH263Profile0Level10 || iTargetFormat == EVedVideoTypeH263Profile0Level45) && 
               fFirstFrameInH263 && iBitStreamMode == EVedVideoBitstreamModeMPEG4ShortHeader)
         {
             /* we don't need the VOS header, remove it */
             PRINT((_L("CMPEG4Transcoder: MPEG4 shortheader VOS removed")));
             
             CopyStream(iInBuffer,iOutBuffer,&bufEdit,iShortHeaderEndByteIndex,iShortHeaderEndBitIndex);
             fFirstFrameInH263 = EFalse;
         }
         else
         {
           /* copy from the begining of the input buffer, 
              including the VOS header for MPEG4 shortheader if its mode is not being changed
             */
             CopyStream(iInBuffer,iOutBuffer,&bufEdit,0,7);
         }
    }
    

    
    else
    {
        /* H263 Picture Header -> MPEG4 VOP Header mapping
           information about this frame
        */
        iMBsinWidth = aInfo->vdcInstance->pictureParam.lumWidth >> 4;
        
        iNumMBsInOneVOP = (aInfo->vdcInstance->pictureParam.lumWidth >> 4) *
            (aInfo->vdcInstance->pictureParam.lumHeight >> 4);
        
        iNumMBsInGOB = aInfo->vdcInstance->pictureParam.numMBsInGOB;
        
        /* create the intraDC matrix for DC prediction */
        VDTASSERT(!iH263DCData);
        VDTASSERT(!iH263MBVPNum);
        
        iH263DCData = (int **)malloc(iNumMBsInOneVOP * sizeof(int));
        if (!iH263DCData) 
        {
            deb("CMPEG4Transcoder::ERROR - iH263DCData creation failed\n");
            return TX_ERR;
        }
        
        for (int i = 0; i < iNumMBsInOneVOP; i++)
        {
            iH263DCData[i] = (int *) malloc(6 * sizeof(int)); /* six blocks in one MB */
            VDTASSERT(iH263DCData[i]);
            if (!iH263DCData[i]) 
            {
                deb("CMPEG4Transcoder::ERROR - iH263DCData[i] creation failed\n");

                for(int k=0; k<i; k++)
                {
                    free (iH263DCData[k]); 
                }
                if (iH263DCData) 
                {
                    free(iH263DCData);
                }

                return TX_ERR;
            }

            
            /* initialize each entry to 1024 for DC prediction */
            for (int j = 0; j < 6; j++)
            {
                iH263DCData[i][j] = 1024;
            }
        }
        
        iH263MBVPNum = (int *) malloc(iNumMBsInOneVOP * sizeof(int));
        if (!iH263MBVPNum) 
        {
            deb("CMPEG4Transcoder::ERROR - iH263MBVPNum creation failed\n");
            return TX_ERR;
        }
        
        memset(iH263MBVPNum, 0, iNumMBsInOneVOP * sizeof(int));
        int pictureType = aInfo->vdcInstance->pictureParam.pictureType == VDX_PIC_TYPE_I? I_VOP : P_VOP;
        
        if (fFirstFrameInH263)
        {
            /* first frame, construct the VOS header */
            sConstructMPEG4VOSHeaderForH263(iOutBuffer,
                aInfo->vdcInstance->pictureParam.lumWidth,
                aInfo->vdcInstance->pictureParam.lumHeight, 
                iOutputMpeg4TimeIncResolution);
            fFirstFrameInH263 = EFalse;
            
            PRINT((_L("CMPEG4Transcoder: H263 -> MPEG VOS generated")));
        }
        sConstructVOPHeaderForH263(iOutBuffer, aH263PicHeader->pquant, pictureType, aH263PicHeader->trp == -1 ? 0 : aH263PicHeader->trp,iOutputMpeg4TimeIncResolution);

        
    PRINT((_L("CMPEG4Transcoder: H263 -> MPEG4: transcoding one picture")));
    }
    
    
    PRINT((_L("CMPEG4Transcoder: H263PictureHeaderEndedL. color effect: %d, format convert %d, do transcoding: %d streammode: %d, outputformat: %d "), 
       iColorEffect, iDoModeTranscoding, iDoTranscoding,
         iBitStreamMode, iTargetFormat));
    
    return TX_OK;
}

/*
* H263GOBSliceHeaderEnded
*
* Parameters: 
*     
* Function:
*    This function is called after one H263 GOB or Slice header is parsed
* Returns:
*     None
* Error codes:
*    None
*
*/
void CMPEG4Transcoder::H263GOBSliceHeaderEnded(vdxGOBHeader_t *aH263GOBHeader, vdxSliceHeader_t */*aH263SliceHeader*/)
{
    iGOBSliceHeaderEndByteIndex = iInBuffer->getIndex;
    iGOBSliceHeaderEndBitIndex  = iInBuffer->bitIndex;  
    
    if (aH263GOBHeader)
    {
        /* not the first GOB (no header), we have stuffing bits ahead
        since we only store the position of GBSC not including 
        the stuffing bits, we may need to insert the stuffing bits here
        if we have color effect, we have done the stuffing at the end of last GOB
        see functin: H263OneGOBSliceWithHeaderEnded
        */
        sStuffBitsH263(iOutBuffer); 
    }
    
    if (!iDoModeTranscoding)
    {
       /* Copy the header to output stream */
       bufEdit.copyMode = CopyWhole; // whole
         CopyStream(iInBuffer,iOutBuffer,&bufEdit,iGOBSliceStartByteIndex,iGOBSliceStartBitIndex);
         
    }

    else
    {
        /* H263 GOB (Slice) Header -> MPEG4 VP Header mapping */
        if (!VDT_NO_DATA((int)iInBuffer->getIndex, iInBuffer->bitIndex, iGOBSliceStartByteIndex, iGOBSliceStartBitIndex) &&
            aH263GOBHeader)
        {
            /* sPutVideoPacketHeader */
            sPutVideoPacketHeader(iOutBuffer, aH263GOBHeader->gn * iNumMBsInGOB, (short)aH263GOBHeader->gquant, iNumMBsInOneVOP);
        }
    }
        
    // one slice ended, new video packet starting
    iCurMBNumInVP = -1;
    
}


/*
* H263GOBSliceHeaderBegin
*
* Parameters: 
*     
* Function:
*    This function is called before one H263 GOB or Slice header is parsed
* Returns:
*     None
* Error codes:
*    None
*
*/
void CMPEG4Transcoder::H263GOBSliceHeaderBegin()
{
    /* position not include the stuffing bits ahead of the GBSC ! */
    iGOBSliceStartByteIndex = iInBuffer->getIndex;
    iGOBSliceStartBitIndex  = iInBuffer->bitIndex;
    iStuffingBitsUsed = 0;
}


/*
* H263GOBSliceHeaderBegin
*
* Parameters: 
*     
* Function:
*    This function is called after one H263 GOB or Slice ends
* Returns:
*     None
* Error codes:
*    None
*
*/
void CMPEG4Transcoder::H263OneGOBSliceEnded(int nextExpectedMBNum)
{
    if (iDoTranscoding)
    {
        /* MBs are lost or not coded, for H263 and H263->MPEG4 both */
        for (int i = iLastMBNum+1; i < nextExpectedMBNum; i++)
        {
            /* output  1 bit COD */
            sPutBits (iOutBuffer, 1, 1);
        }
        iLastMBNum = nextExpectedMBNum - 1; 
    }
}


/*
* H263OneGOBSliceWithHeaderEnded
*
* Parameters: 
*     
* Function:
*    This function is called after GOB (Slice) that has GOB header (except the first GOB) ends
* Returns:
*     None
* Error codes:
*    None
*
*/
void CMPEG4Transcoder::H263OneGOBSliceWithHeaderEnded()
{
    if (!iDoTranscoding)
    {
       bufEdit.copyMode = CopyWhole; /* CopyWhole */
         CopyStream(iInBuffer,iOutBuffer,&bufEdit,
             iGOBSliceHeaderEndByteIndex,iGOBSliceHeaderEndBitIndex);
    }
    else
    {

        if (iDoModeTranscoding)
        {
            /* for H263 to MPEG4, it implies that one video packet is finished */
            vdtStuffBitsMPEG4(iOutBuffer);
        }
        else

        {
            /* stuffing occurs outside at the end of one GOB/slice withe header or end of frame, check in file core.cpp */
            sStuffBitsH263(iOutBuffer); 
        }
        iVideoPacketNumInMPEG4 ++;
    }

    // one slice ended, new video packet starting
    iCurMBNumInVP = -1; // 0

}



/*
* H263ToMPEG4MBData
*
* Parameters: 
*     
* Function:
*    This function transcodes one H263 MB to one MPEG4 MB
* Returns:
*     None
* Error codes:
*    None
*
*/
void CMPEG4Transcoder::H263ToMPEG4MBData(int aNewMCBPCLen, int aNewMCBPC)
{
    /* MB data part1: output MCBPC, CBPY, DQuant, MV, intra DC etc */
    int *dataItemStartByteIndex;
    int *dataItemStartBitIndex;
    int *dataItemEndByteIndex;
    int *dataItemEndBitIndex;
    int quant, cbpy, cbpc;
    
    if (iVopCodingType == VDX_VOP_TYPE_P) 
    {
        dataItemStartByteIndex = iCurPMBinstance->DataItemStartByteIndex;
        dataItemStartBitIndex  = iCurPMBinstance->DataItemStartBitIndex;
        dataItemEndByteIndex   = iCurPMBinstance->DataItemEndByteIndex;
        dataItemEndBitIndex    = iCurPMBinstance->DataItemEndBitIndex;
        quant = iCurPMBinstance->quant;
        cbpy  = iCurPMBinstance->cbpy;
        cbpc  = iCurPMBinstance->cbpc;
    }
    else
    {
        dataItemStartByteIndex = iCurIMBinstance->DataItemStartByteIndex;
        dataItemStartBitIndex  = iCurIMBinstance->DataItemStartBitIndex;
        dataItemEndByteIndex   = iCurIMBinstance->DataItemEndByteIndex;
        dataItemEndBitIndex    = iCurIMBinstance->DataItemEndBitIndex;
        quant = iCurIMBinstance->quant;
        cbpy  = iCurIMBinstance->cbpy;
        cbpc  = iCurIMBinstance->cbpc;
    }
    
    /* COD and MCBPC, (for I frame, it is MCBPC, for P frame it is COD and MCBPC) */
    if (iVopCodingType == VDX_VOP_TYPE_P)
    {
        /* It is a coded MB, output  1 bit COD (it always exists in P frame) */
        sPutBits (iOutBuffer, 1, 0);
    }
    
    if (iColorEffect)
    {
        /* MCBPC Changed */
        sPutBits(iOutBuffer, aNewMCBPCLen, aNewMCBPC);
    }
    else
    {
        /* remember the positions do not include the possible stuffing MCBPC bits */
        vdtCopyBuffer(iInBuffer, iOutBuffer,
            dataItemStartByteIndex[0], dataItemStartBitIndex[0],
            dataItemEndByteIndex[0], dataItemEndBitIndex[0]);
    }
    
    /* ac_pred_flag  */
    if (iMBCodingType == VDX_MB_INTRA) 
    {
        sPutBits(iOutBuffer, 1, 0);// it is closed for H263 INTRA MB
    }
    
    /* CBPY */
    vdtCopyBuffer(iInBuffer, iOutBuffer,
        dataItemStartByteIndex[2], dataItemStartBitIndex[2],
        dataItemEndByteIndex[2], dataItemEndBitIndex[2]);
    
    /* DQUANT, if it exsits */
    vdtCopyBuffer(iInBuffer, iOutBuffer,
        dataItemStartByteIndex[1], dataItemStartBitIndex[1],
        dataItemEndByteIndex[1], dataItemEndBitIndex[1]);
    
    if (iMBCodingType == VDX_MB_INTER) 
    {
        /* MVs, if they exsit */
        vdtCopyBuffer(iInBuffer, iOutBuffer,
            dataItemStartByteIndex[10], dataItemStartBitIndex[10],
            dataItemEndByteIndex[10], dataItemEndBitIndex[10]);
    }
    
    /* following is the block level data */
    for (int i = 0; i < (iColorEffect? 4 : 6); i++)
    {
        if (iMBCodingType == VDX_MB_INTRA) 
        {
            int fBlockCoded = 0;
            if (i < 4)
            {
                fBlockCoded = vdxIsYCoded(cbpy, i + 1);
            }
            else if (i == 4)
            {
                fBlockCoded = vdxIsUCoded(cbpc);
            }
            else 
            {
                fBlockCoded = vdxIsVCoded(cbpc);
            }
            
            /* Difference between H263 and MPEG4 :
               0. DC quantization is different, DC need to be dequantized and requantized
                  Note:  for ACs, here we only support H263 quantization in MPEG4, we can copy them
                        1. DC prediction is used in MPEG4
                        2. They use different VLC table for INTRA AC coefficient
            */
            /* reconstuct INTRA DC */
            int intraDC = (iDCTBlockData + i * BLOCK_COEFF_SIZE)[0]; /* it is already dequantized in funcion vdxGetIntraDCTBlock */
            
            /* INTRA quantization and prediction */
            (iDCTBlockData + i * BLOCK_COEFF_SIZE)[0] = sGetMPEG4INTRADCValue(intraDC,
                i, iCurMBNum, quant, iMBsinWidth,iH263DCData, iH263MBVPNum);
            
            /* recoding the INTRA block */
            int numTextureBits = 0;
            
            vdtPutIntraMBCMT(iOutBuffer,iDCTBlockData + i * BLOCK_COEFF_SIZE, &numTextureBits, i, 0, 1); /* DC coding */
            
            if (fBlockCoded)
            {
                vdtPutIntraMBCMT(iOutBuffer,iDCTBlockData + i * BLOCK_COEFF_SIZE, &numTextureBits, i, 1, 0); /*encode ACs */
            }
            
        }
        
        else if (!(VDT_NO_DATA(iBlockStartByteIndex[i], iBlockStartBitIndex[i],
            iBlockEndByteIndex[i], iBlockEndBitIndex[i])))
        {
            
        /* 4. same VLC table for DCTs in H263 and MPEG4, but with different Escape Coding type. 
        see 7.4.1.3. in MPEG4 draft
            */
            if (iEscapeCodeUsed[i])
            {
                int numTextureBits = 0;
                vdtPutInterMBCMT(iOutBuffer,0, iDCTBlockData + i * BLOCK_COEFF_SIZE, &numTextureBits, OFF);
            }
            else
            {
                bufEdit.copyMode = CopyWhole; /* CopyWhole */
                vdtCopyBuffer(iInBuffer,iOutBuffer,
                    iBlockStartByteIndex[i],iBlockStartBitIndex[i],
                    iBlockEndByteIndex[i],iBlockEndBitIndex[i]);
            }
        }
    }
    
  if (iColorEffect && (iMBCodingType == VDX_MB_INTRA))
    {
        ResetMPEG4IntraDcUV();
    }
}



/*
* NeedDecodedYUVFrame
*
* Parameters: 
*     
* Function:
*    This function indicates if whether we need the decoded frame 
* Returns:
*     ETrue if we need the decoded frame
* Error codes:
*    None
*
*/
int CMPEG4Transcoder::NeedDecodedYUVFrame()
{
    if ( (iTargetFormat == EVedVideoTypeH263Profile0Level10 || iTargetFormat == EVedVideoTypeH263Profile0Level45) &&
        iBitStreamMode !=EVedVideoBitstreamModeMPEG4ShortHeader && iBitStreamMode != EVedVideoBitstreamModeH263)
        return ETrue;
    else
        return EFalse;
}

/*
* NewL
*
* Parameters: 
*     
* Function:
*    Symbian two-phased constructor 
* Returns:
*     pointer to constructed object, or NULL
* Error codes:
*    None
*
*/
CMPEG4Transcoder* CMPEG4Transcoder::NewL(const vdeInstance_t *aVDEInstance, bibBuffer_t *aInBuffer, bibBuffer_t *aOutBuffer)
{
    CMPEG4Transcoder *self = new (ELeave) CMPEG4Transcoder(aVDEInstance, aInBuffer, aOutBuffer);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    
    return self;
}


/*
* ConstructL
*
* Parameters: 
*     
* Function:
*    Symbian 2nd phase constructor (can leave)
* Returns:
*     None
* Error codes:
*    None
*
*/
void CMPEG4Transcoder::ConstructL()
{
    
    /* Create one IMB Instance  */
    iCurIMBinstance = (vdxIMBListItem_t *) malloc(sizeof(vdxIMBListItem_t));
    if (!iCurIMBinstance) 
    {
        deb("CMPEG4Transcoder::ERROR - iCurIMBinstance creation failed\n");
        User::Leave(KErrNoMemory);
       
    }
    memset(iCurIMBinstance, 0, sizeof(vdxIMBListItem_t));
    
    /* Create one PMBInstance */
    iCurPMBinstance = (vdxPMBListItem_t *) malloc(sizeof(vdxPMBListItem_t));
    if (!iCurPMBinstance) 
    {
        deb("CMPEG4Transcoder::ERROR - iCurPMBinstance creation failed\n");
        User::Leave(KErrNoMemory);
       
    }
    memset(iCurPMBinstance, 0, sizeof(vdxPMBListItem_t));
    
    /* initialize the edit buffer */
    bufEdit.editParams = (bibEditParams_t *) malloc(sizeof(bibEditParams_t));
    VDTASSERT(bufEdit.editParams);
    if (!bufEdit.editParams) 
    {
        deb("CMPEG4Transcoder::ERROR - bufEdit.editParams creation failed\n");
        User::Leave(KErrNoMemory);
       
    }else{
        bufEdit.numChanges = 1;
    }
    
    if (!iMBType)
    {
        iMBType = (u_char *) malloc(iNumMBsInOneVOP * sizeof(u_char));
        if (!iMBType) 
        {
            deb("CMPEG4Transcoder::ERROR - iMBType creation failed\n");
            User::Leave(KErrNoMemory);
        }
        memset(iMBType, VDX_MB_INTER, iNumMBsInOneVOP * sizeof(u_char));
    }    
    
}


/*
* GetMpeg4DcScaler
*
* Parameters: 
*     
*  aQP            Quantization parameter
*
* Function:
*    Evaluates Chrominance DC Scaler from QP for MPEG4
* Returns:
*    int DC scaler value
* Error codes:
*    None
*
*/
int CMPEG4Transcoder::GetMpeg4DcScalerUV(int aQP)
{
    if (aQP>=1 && aQP<=4)
        return (8);
    else if (aQP>=5 && aQP<=24)
        return ((aQP+13)/2);
    else if (aQP>=25 && aQP<=31)
        return (aQP-6);
    else 
        return (0);  // error 
}


void CMPEG4Transcoder::AfterMBLayer(int aUpdatedQp)
{
  iCurQuant = aUpdatedQp;
}




/* End of File */
