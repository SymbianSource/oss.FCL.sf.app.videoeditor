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
* Implementation for MPEG4(H263) video transcoder. 
* Block matching algorithms for MPEG4(H263) video transcoder.
*
*/



#include "biblin.h"
#include "common.h"
#include "h263dconfig.h"
#include "decmbdct.h"
#include "vdxint.h"

/* 
 * Constants/definitions 
 */

#define DIAMOND_SEARCH_NUMBER 2


const tVLCTable sCBPCIType[9] = 
{
    {0x01,0x01}, {0x01,0x03}, {0x02,0x03}, {0x03,0x03}, 
        {0x01,0x04}, {0x01,0x06}, {0x02,0x06}, {0x03,0x06}, 
        {0x01,0x09}
};

const tVLCTable sCBPCPType[21] = 
{
    {0x01,0x01}, {0x03,0x04}, {0x02,0x04}, {0x05,0x06},
    {0x03,0x03}, {0x07,0x07}, {0x06,0x07}, {0x05,0x09}, 
    {0x02,0x03}, {0x05,0x07}, {0x04,0x07}, {0x05,0x08},
    {0x03,0x05}, {0x04,0x08}, {0x03,0x08}, {0x03,0x07},
    {0x04,0x06}, {0x04,0x09}, {0x03,0x09}, {0x02,0x09}, 
    {0x01,0x09}
};

const tVLCTable sCBPY[16] = 
{
    {0x03,0x04}, {0x05,0x05}, {0x04,0x05}, {0x09,0x04},
    {0x03,0x05}, {0x07,0x04}, {0x02,0x06}, {0x0b,0x04},
    {0x02,0x05}, {0x03,0x06}, {0x05,0x04}, {0x0a,0x04},
    {0x04,0x04}, {0x08,0x04}, {0x06,0x04}, {0x03,0x02}
};

const unsigned int sDquant[5] = 
{
    0x01, 0x00, (unsigned int)NOT_VALID, 0x02, 0x03
};

const tVLCTable sMVTab[33] =
{
        {0x01,0x01}, {0x02,0x03}, {0x02,0x04}, {0x02,0x05}, 
        {0x06,0x07}, {0x0a,0x08}, {0x08,0x08}, {0x06,0x08},
        {0x16,0x0a}, {0x14,0x0a}, {0x12,0x0a}, {0x22,0x0b},
        {0x20,0x0b}, {0x1e,0x0b}, {0x1c,0x0b}, {0x1a,0x0b},
        {0x18,0x0b}, {0x16,0x0b}, {0x14,0x0b}, {0x12,0x0b},
        {0x10,0x0b}, {0x0e,0x0b}, {0x0c,0x0b}, {0x0a,0x0b},
        {0x08,0x0b}, {0x0e,0x0c}, {0x0c,0x0c}, {0x0a,0x0c}, 
        {0x08,0x0c}, {0x06,0x0c}, {0x04,0x0c}, {0x06,0x0d},
        {0x04,0x0d}
}; 

const int32  sFixedQuantScale1[32]=
{
      0x0000, 0x7fff, 0x3fff, 0x2aaa, 
        0x1fff, 0x1999, 0x1555, 0x1249, 
        0x0fff, 0x0e38, 0x0ccc, 0x0ba2,  
        0x0aaa, 0x09d8, 0x0924, 0x0888,  
        0x07ff, 0x0787, 0x071c, 0x06bc,  
        0x0666, 0x0618, 0x05d1, 0x1590,  
        0x0555, 0x051e, 0x04ec, 0x04bd,  
        0x0492, 0x0469, 0x0444, 0x0421  
};  

const u_int16 sPrePostMult[64] = 
{ 
      0x8000, 0xb18a, 0xa73d, 0x9683,  
        0x8000, 0x9683, 0xa73d, 0xb18a, 
      0xb18a, 0xf641, 0xe7f7, 0xd0c3,  
        0xb18a, 0xd0c3, 0xe7f7, 0xf641, 
      0xa73d, 0xe7f7, 0xda82, 0xc4a7,  
        0xa73d, 0xc4a7, 0xda82, 0xe7f7, 
      0x9683, 0xd0c3, 0xc4a7, 0xb0fb,  
        0x9683, 0xb0fb, 0xc4a7, 0xd0c3, 
      0x8000, 0xb18a, 0xa73d, 0x9683,  
        0x8000, 0x9683, 0xa73d, 0xb18a, 
      0x9683, 0xd0c3, 0xc4a7, 0xb0fb,  
        0x9683, 0xb0fb, 0xc4a7, 0xd0c3, 
      0xa73d, 0xe7f7, 0xda82, 0xc4a7,  
        0xa73d, 0xc4a7, 0xda82, 0xe7f7, 
      0xb18a, 0xf641, 0xe7f7, 0xd0c3,  
        0xb18a, 0xd0c3, 0xe7f7, 0xf641
};



/* 
 * Function Declarations 
 */
int32 vlbCodeACCoeffsSVHWithZigZag(int32 coeffStart, int16* block, bibBuffer_t * outBuf,
                                 int32 svh, int32 lastPos);
void vlbPutBits(bibBuffer_t *base, int32 numBits, u_int32 value);



/* 
 * Function Definitions 
 */

/* {{-output"vbmGetH263IMCBPC.txt"}} */
/*
* vbmGetH263IMCBPC
*
* Parameters:
*     vopCodingType            coding type (INTER/INTRA) for the VOP
*           dQuant                   quantization parameter
*           colorEffect              indicates color effect to be aplpied (e.g., black & white)
*           cbpy                       cbpy value for the macro block
*           mcbpcVal                   computed mcbpc value for the macro block
*           length                   length of the computed mcbpc value codeword
*
* Function:
*     This function evaluates the mcpbc codeword for INTRA macro block
*
* Returns:
*     Nothing.
*
* Error codes:
*     None.
*
*/
void vbmGetH263IMCBPC(int dQuant, int vopCodingType, int /*colorEffect*/, 
                                     int cbpy, int& mcbpcVal, int& length)
{
    int index;
    int len, mcbpc;

    if(vopCodingType == 1 /* VDX_VOP_TYPE_P */)
    {
        index = (dQuant == 0 ? 12 : 16) + mcbpcVal;
        len = sCBPCPType[index].length + sCBPY[cbpy].length + 1;
        mcbpc = (sCBPCPType[index].code << sCBPY[cbpy].length) | sCBPY[cbpy].code;
        mcbpcVal = mcbpc;
        length = len;
    }
    else
    {
        index = (dQuant == 0 ? 0 : 4) + mcbpcVal;
        len = sCBPCIType[index].length + sCBPY[cbpy].length ;
        mcbpc = (sCBPCIType[index].code << sCBPY[cbpy].length) | sCBPY[cbpy].code;
        mcbpcVal = mcbpc;
        length = len;
    }
}



/* {{-output"vbmGetH263PMCBPC.txt"}} */
/*
* vbmGetH263PMCBPC
*
* Parameters:
*           dQuant                   quantization parameter
*           colorEffect              indicates color effect to be aplpied (e.g., black & white)
*           cbpy                       cbpy value for the macro block
*           mcbpcVal                   computed mcbpc value for the macro block
*           length                   length of the computed mcbpc value codeword
*
* Function:
*     This function evaluates the mcpbc codeword for INTER macro block
*
* Returns:
*     Nothing.
*
* Error codes:
*     None.
*
*/
void vbmGetH263PMCBPC(int dQuant, int /*colorEffect*/, int cbpy, int& mcbpcVal, int& length)
{
    int index;
    int len, mcbpc;
    cbpy = (~cbpy) & 0xf;
    
    /* only ONE MV in baseline H263 */
    index = (dQuant == 0 ? 0 : 4) + mcbpcVal;
    len = sCBPCPType[index].length + sCBPY[cbpy].length;
    mcbpc = (sCBPCPType[index].code << sCBPY[cbpy].length) | sCBPY[cbpy].code;
    mcbpcVal = mcbpc;
    length = len;
}



/* {{-output"vbmEncodeMVDifferential.txt"}} */
/*
* vbmEncodeMVDifferential
*
* Parameters:
*     outBuf                     output buffer
*           mvdx                     motion vector difference value in horizontal direction, in half-pixel unit
*           mvdy                     motion vector difference value in vertical direction, in half-pixel unit
*           fCode                      Fcode value
*
* Function:
*     This function encodes the MV difference after prediction into the bitstream
*
* Returns:
*     Nothing.
*
* Error codes:
*     None.
*
*/
void vbmEncodeMVDifferential(int32 mvdx, int32 mvdy, int32 fCode, 
                           bibBuffer_t * outBuf)
{
    int32   mvd[2];
    u_int32 i;
    int32   temp;
    int32   motioncode;
    int32   motionresidual;
    int32   rsize = fCode - 1;
    u_int32 f     = 1 << rsize;
    int32   high  = 32 * f - 1;
    int32   low   = -32 * (int32) f;
    int32   range = 64 *f;
    
    mvd[0] = mvdx;
    mvd[1] = mvdy;
    
    for(i = 0; i < 2; i++)    
    {
        if (mvd[i] < low)
        {
            mvd[i] += range;
        }
        else
        {
            if (mvd[i] > high)
            {
                mvd[i] -= range;
            }
        }
        temp = ABS(mvd[i]) -1 + f;
        motioncode = temp >> rsize;    
        if(mvd[i] >= 0)
        {
            vlbPutBits(outBuf, sMVTab[motioncode].length, sMVTab[motioncode].code);
        }
        else
        {
            vlbPutBits(outBuf, sMVTab[motioncode].length, (sMVTab[motioncode].code) ^ 1);
        }        
        if (rsize != 0 && motioncode != 0)
        {
            motionresidual = temp & (f - 1);
            vlbPutBits(outBuf, rsize, motionresidual);
        }   
    }
    return;
}




/* {{-output"vbmMedian3.txt"}} */
/*
* vbmMedian3
*
* Parameters:
*     s1                         pointer to the first vector
*     s2                         pointer to the second vector
*     s3                         pointer to the third vector
*     med                        pointer to store the median vector
*
* Function:
*     This function finds the median of three vectors
*
* Returns:
*     Nothing.
*
* Error codes:
*     None.
*
*/
void vbmMedian3(int16 *s1, int16 *s2, int16 *s3, tMotionVector *med)
{
    int32 temp1;
    int32 temp2;
    int32 temp3;
    
    temp1 = s1[0];
    temp2 = s2[0];
    temp3 = s3[0];
    
    if (temp1 > temp2)
    {
        temp1 += temp2;
        temp2 = temp1 - temp2;
        temp1 -= temp2;
    }
    if (temp2 > temp3)
    {
        temp2 += temp3;
        temp3 = temp2 - temp3;
        temp2 -= temp3;
    }
    if (temp1 > temp2)
    {
        temp1 += temp2;
        temp2 = temp1 - temp2;
        temp1 -= temp2;
    }
    
    med->mvx = (int16) temp2;
    temp1 = s1[1];
    temp2 = s2[1];
    temp3 = s3[1];
    
    if (temp1 > temp2)
    {
        temp1 += temp2;
        temp2 = temp1 - temp2;
        temp1 -= temp2;
    }
    if (temp2 > temp3)
    {
        temp2 += temp3;
        temp3 = temp2 - temp3;
        temp2 -= temp3;
    }
    if (temp1 > temp2)
    {
        temp1 += temp2;
        temp2 = temp1 - temp2;
        temp1 -= temp2;
    }
    med->mvy = (int16) temp2;
    
    return;
}



/* {{-output"vbmMvPrediction.txt"}} */
/*
* vbmMvPrediction
*
* Parameters:
*     mbi                  macro block info needed for processing
*     predMV               pointer to motion vector predictors
*     mbinWidth            number of MBs per row
*     mBCnt                    mecro block number
*
* Function:
*     This function performs prediction of MV based on adjacent blocks
*
* Returns:
*     Nothing.
*
* Error codes:
*     None.
*
*/
void vbmMvPrediction(tMBInfo *mbi, int32 mBCnt, tMotionVector *predMV, int32 mbinWidth)
{
    int16    p[3][2];
    int32    zeroFound = 0;
    int32    zeroPredictors = 0;
    
    if (mBCnt % mbinWidth == 0)
    {
        p[0][0] = 0;
        p[0][1] = 0;
        zeroPredictors++;
    }
    else
    {
        p[0][0] = (mbi - 1)->MV[0][0];
        p[0][1] = (mbi - 1)->MV[0][1];
    }
    
    if (mBCnt / mbinWidth == 0) 
    {
        p[1][0] = 0;
        p[1][1] = 0;
        zeroPredictors++;
        zeroFound += 1;
    }
    else
    {
        p[1][0] = (mbi - mbinWidth)->MV[0][0];
        p[1][1] = (mbi - mbinWidth)->MV[0][1];
    }
    
    if ((mBCnt / mbinWidth == 0) ||
        ((mBCnt % mbinWidth) == (mbinWidth - 1)))
    {
        p[2][0] = 0;
        p[2][1] = 0;
        zeroPredictors++;
        zeroFound += 2;
    }
    else
    {
        p[2][0] = (mbi - mbinWidth + 1)->MV[0][0];
        p[2][1] = (mbi - mbinWidth + 1)->MV[0][1];
    }
    
    if (zeroPredictors == 3)
    {
        predMV->mvx = 0;
        predMV->mvy = 0;
    }
    else if (zeroPredictors == 2)
    {
        predMV->mvx = p[zeroFound^3][0];
        predMV->mvy = p[zeroFound^3][1];
    }
    else
    {
        vbmMedian3(p[0], p[1], p[2], predMV);
    }
    return;
}



/* {{-output"vbmMBSAD.txt"}} */
/*
* vbmMBSAD
*
* Parameters:
*     refFrame             pointer to reference frame
*     currMB               pointer to current MB
*     mv                   pointer to store the SAD and having motion vector
*     mbPos                pointer to macroblock position structure
*     vopWidth             width of frame/VOP
*     yWidth               width of luminance frame 
*
* Function:
*     This function computes the SAD (Sum of Absolute Difference) 
*     for a macroblock for integer motion vector
*
* Returns:
*     Nothing.
*
* Error codes:
*     None.
*
*/
void vbmMBSAD(tPixel* refFrame, u_int32 vopWidth, tPixel* currMB, u_int32 yWidth,
                            tMotionVector* mv, const tMBPosition* mbPos)
{
    u_int32     sad = 0;
    tPixel*     refPos;
    tPixel*     currPos;
    int32       row;
    int32       col;
    
    refPos = refFrame + (mbPos->y + mv->mvy) * vopWidth + (mbPos->x + mv->mvx); 
    currPos = currMB; 
    
    for(row = 0; row < MB_SIZE; row++)
    {
        col = row & 0x1;
        for(; col < MB_SIZE; col+= 2)
        {
            sad += ABS(*(refPos + col) - *(currPos + col));
        }
        refPos += vopWidth;
        currPos += yWidth;
    }
    mv->SAD = (sad << 1);
    return;
}



/* {{-output"vbmMVOutsideBound.txt"}} */
/*
* vbmMVOutsideBound
*
* Parameters:
*     mbPos                pointer to macroblock position structure
*     bestMV               pointer to store the best match motion vector
*     halfPixel            flag to indicate whether half pel search is needed
*
* Function:
*     This function checks whether the MV is within valid range
*
* Returns:
*     Nothing.
*
* Error codes:
*     None.
*
*/
tBool vbmMVOutsideBound(tMBPosition *mbPos, tMotionVector* bestMV, tBool halfPixel)
{
    int32   xLeft;
    int32   xRight;
    int32   yTop;
    int32   yBottom;
    
    xLeft = (mbPos->x << halfPixel) + bestMV->mvx;
    xRight = (mbPos->x << halfPixel) + (MB_SIZE << halfPixel) + bestMV->mvx;
    yTop = (mbPos->y << halfPixel) + bestMV->mvy;
    yBottom = (mbPos->y << halfPixel) + (MB_SIZE << halfPixel) + bestMV->mvy;
    
    if (halfPixel)
    {
        return ((xLeft < mbPos->LeftBound) ||
            (xRight > mbPos->RightBound) ||
            (yTop < mbPos->TopBound) ||
            (yBottom > mbPos->BottomBound) ||
            ((bestMV->mvx) < -32  || (bestMV->mvx) > 31) ||
            ((bestMV->mvy) < -32  || (bestMV->mvy) > 31)
            );
    }
    else
    {
        return ((xLeft < mbPos->LeftBound) ||
            (xRight > mbPos->RightBound) ||
            (yTop < mbPos->TopBound) ||
            (yBottom > mbPos->BottomBound)||
            ((bestMV->mvx) < -16  || (bestMV->mvx) > 15) ||
            ((bestMV->mvy) < -16  || (bestMV->mvy) > 15)
            );
    }
}



/* {{-output"vbmEstimateBestPredictor.txt"}} */
/*
* vbmEstimateBestPredictor
*
* Parameters:
*     refFrame             pointer to reference frame
*     currMB               pointer to current MB
*     initPred             pointer to predictor motion vectors, pixel unit
*     mbPos                pointer to macroblock position structure
*     vopWidth             width of frame/VOP
*     yWidth               width of luminance frame 
*     bestMV               pointer to store the best match motion vector
*     noofPredictors       number of predictors
*
* Function:
*     This function estimates the best predictor among the set
*
* Returns:
*     Nothing.
*
* Error codes:
*     None.
*
*/
void vbmEstimateBestPredictor(tPixel* refFrame, u_int32 vopWidth, tPixel* currMB, u_int32 yWidth, 
                              tMotionVector* initPred, tMBPosition* mbPos,
                              int32 noOfPredictors, tMotionVector* bestMV)
{
    int32 i;
    bestMV->SAD = 65535;
    for(i = 0; i < noOfPredictors; i++)
    {
        if (vbmMVOutsideBound(mbPos, (initPred + i), 0))
        {
            initPred[i].SAD = 65535;
        }
        else
        {
            vbmMBSAD(refFrame, vopWidth, currMB, yWidth, (initPred + i), mbPos);
        }
        if(initPred[i].SAD < bestMV->SAD)
        {
            bestMV->SAD = initPred[i].SAD;
            bestMV->mvx = initPred[i].mvx;
            bestMV->mvy = initPred[i].mvy;
        }
    }
    
    return;
}       



/* {{-output"vbmEstimateBound.txt"}} */
/*
* vbmEstimateBound
*
* Parameters:
*     mbPos                pointer to macroblock position structure
*     vopWidth             width of frame/VOP
*     vopHeight            height of frame/VOP
*     searchRange          search range
*
* Function:
*     This function evaluates the bounds for a macroblock BM search
*
* Returns:
*     Nothing.
*
* Error codes:
*     None.
*
*/
void vbmEstimateBound(tMBPosition *mbPos,u_int32 vopWidth, u_int32 vopHeight, int32 searchRange)
{
    mbPos->LeftBound = mbPos->x - searchRange;
    mbPos->RightBound = mbPos->x + 16 + searchRange;
    mbPos->TopBound = mbPos->y - searchRange;
    mbPos->BottomBound = mbPos->y + 16 + searchRange;
    
    if(mbPos->LeftBound < 0)
    {
        mbPos->LeftBound = 0;
    }
    if(mbPos->RightBound > (int32)vopWidth)
    {
        mbPos->RightBound = vopWidth;
    }
    if(mbPos->TopBound < 0 )
    {
        mbPos->TopBound = 0;
    }
    if(mbPos->BottomBound > (int32)vopHeight)
    {
        mbPos->BottomBound = vopHeight;
    }
    
    return;
}



/* {{-output"vbmEstimateBoundHalfPel.txt"}} */
/*
* vbmEstimateBoundHalfPel
*
* Parameters:
*     mbPos                pointer to macroblock position structure
*     vopWidth             width of frame/VOP
*     vopHeight            height of frame/VOP
*
* Function:
*     This function evaluates the bounds for a macroblock BM search in half pel accuracy
*
* Returns:
*     Nothing.
*
* Error codes:
*     None.
*
*/
void vbmEstimateBoundHalfPel(tMBPosition *mbPos, u_int32 vopWidth, u_int32 vopHeight)
{
    mbPos->LeftBound = 0;
    mbPos->RightBound = (vopWidth << 1);
    mbPos->TopBound = 0;
    mbPos->BottomBound = (vopHeight << 1);
    return;
}



/* {{-output"vbmSmallDiamondSearch.txt"}} */
/*
* vbmSmallDiamondSearch
*
* Parameters:
*     refFrame             pointer to reference frame
*     currMB               pointer to current MB
*     initPred             pointer to predictor motion vectors, pixel unit
*     mbPos                pointer to macroblock position structure
*     vopWidth             width of frame/VOP
*     yWidth               width of luminance frame 
*     bestMV               pointer to store the best match motion vector
* Function:
*     This function performs motion estimation for a macroblock using small diamond search
*
* Returns:
*     Nothing.
*
* Error codes:
*     None.
*
*/
void vbmSmallDiamondSearch(tPixel* refFrame, tPixel* currMB, tMotionVector *initPred, 
                                                     tMotionVector* bestMV, u_int32 vopWidth, tMBPosition* mbPos, 
                                                     u_int32 yWidth)
{
    u_int8 locateSDS[5][5] =
    {
        {0,0,0,0,0},
        {0,1,1,0,1},
        {0,1,1,1,0},
        {0,0,1,1,1},
        {0,1,0,1,1}
    };
    u_int32  i;
    int32   stepCount;
    int32   flag=1;
    int32   position=0;
    
    //form the diamond shap search pattern
    stepCount = 1;
    initPred[0].SAD = bestMV->SAD;
    initPred[1].mvx = (int16)(bestMV->mvx + 1);
    initPred[1].mvy = bestMV->mvy;
    initPred[2].mvx = bestMV->mvx;
    initPred[2].mvy = (int16)(bestMV->mvy - 1);
    initPred[3].mvx = (int16)(bestMV->mvx - 1);
    initPred[3].mvy = bestMV->mvy;
    initPred[4].mvx = bestMV->mvx;
    initPred[4].mvy = (int16)(bestMV->mvy + 1);
    
    for(i = 1; i < 5; i++)
    {
        if(vbmMVOutsideBound(mbPos, &initPred[i], 0) )
        {
            initPred[i].SAD = 65535;
        }
        else
        {
            vbmMBSAD(refFrame, vopWidth, currMB, yWidth, (initPred + i), mbPos);
        }
        if(initPred[i].SAD < bestMV->SAD)
        {
            bestMV->SAD = initPred[i].SAD;
            bestMV->mvx = initPred[i].mvx;
            bestMV->mvy = initPred[i].mvy;
            position = i;
        }
    }
    
    /* the minimum SAD falls in the center */
    if(bestMV->SAD == initPred[0].SAD)
    {
        return;
    }
    
    while(flag)
    {
        stepCount++;
        initPred[0].SAD = bestMV->SAD;
        initPred[1].mvx = (int16)(bestMV->mvx + 1);
        initPred[1].mvy = bestMV->mvy;
        initPred[2].mvx = bestMV->mvx;
        initPred[2].mvy = (int16)(bestMV->mvy - 1);
        initPred[3].mvx = (int16)(bestMV->mvx - 1);
        initPred[3].mvy = bestMV->mvy;
        initPred[4].mvx = bestMV->mvx;
        initPred[4].mvy = (int16)(bestMV->mvy + 1);
        
        for(i = 1; i < 5; i++)
        {
            if(locateSDS[position][i] != 0)
            {
                if(vbmMVOutsideBound(mbPos, &initPred[i],0))
                {
                    initPred[i].SAD = 65535;
                }
                else
                {
                    vbmMBSAD(refFrame, vopWidth, currMB, yWidth, (initPred + i), mbPos);
                    if(initPred[i].SAD < bestMV->SAD)
                    {
                        bestMV->mvx = initPred[i].mvx;
                        bestMV->mvy = initPred[i].mvy;
                        bestMV->SAD = initPred[i].SAD;
                        position = i;
                    }
                }
            }
        }
        if(bestMV->SAD == initPred[0].SAD)
        {
            break;
        }
        if(stepCount > DIAMOND_SEARCH_NUMBER) // we only do 2 diamond search here
        {
            break;
        }
    }
    
    return;
}


                                                     
/* {{-output"vbmSADHalfPel.txt"}} */
/*
* vbmSADHalfPel
*
* Parameters:
*     refFrame             pointer to reference frame
*     currMB               pointer to current MB
*     mbPos                pointer to macroblock position structure
*     vopWidth             width of frame/VOP
*     yWidth               width of luminance frame 
*     mv                   pointer to motion vector
*     roundingControl      rounding control value
*     blockSize            block size
* Function:
*     This function evaluates SAD for a macroblock/block for half pel motion vector
*
* Returns:
*     Nothing.
*
* Error codes:
*     None.
*
*/
void vbmSADHalfPel(tPixel* refFrame, u_int32 vopWidth, tPixel* currMB, u_int32 yWidth,
                                     tMotionVector* mv, const tMBPosition* mbPos, tBool roundingControl, 
                   int32 blockSize)
{
    u_int32      sad = 0;
    tPixel*     refPos;
    tPixel*     currPos;
    int32       row;
    int32       col;
    int32       extendedVOPWidth;
    int32       temp;
    
    extendedVOPWidth = vopWidth ;
    refPos = refFrame + (mbPos->y + (mv->mvy >> 1)) * extendedVOPWidth + 
        (mbPos->x + (mv->mvx >> 1)); 
    currPos = currMB; 
    
    if(mv->mvy & 1)
    {
        if(mv->mvx & 1)
        {
            /* Both horizontal and vertical components are having half pel */
            for(row = 0; row < blockSize; row++)
            {
                col = row & 0x1;
                for(; col < blockSize; col += 2)
                {
                    temp = (refPos[col] + refPos[col + 1] + 
                        refPos[col + extendedVOPWidth] +
                        refPos[col + extendedVOPWidth + 1] +
                        2 -roundingControl) >> 2;
                    sad += ABS(temp - currPos[col]);
                }
                refPos += extendedVOPWidth;
                currPos += yWidth;
            }
            mv->SAD = (sad << 1);
        }
        else
        {
            /* Vertical component is having half pel */
            for(row = 0; row < blockSize; row++)
            {
                col = row & 0x1;
                for(; col < blockSize; col += 2)
                {
                    temp = (refPos[col] + refPos[col + extendedVOPWidth]+
                        1 -roundingControl) >> 1;
                    sad += ABS(temp - currPos[col]);
                }
                refPos += extendedVOPWidth;
                currPos += yWidth;
            }
            mv->SAD = (sad << 1);
        }
    }
    else
    {
        if(mv->mvx & 1)
        {
            /* Horizontal component is having half pel */
            for(row = 0; row < blockSize; row++)
            {
                col = row & 0x1;
                for(; col < blockSize; col += 2)
                {
                    temp = (refPos[col] + refPos[col + 1] + 
                        1 -roundingControl) >> 1;
                    sad += ABS(temp - currPos[col]);
                }
                refPos += extendedVOPWidth;
                currPos += yWidth;
            }
            mv->SAD = (sad << 1);
        }
        else
        {
            /* Both horizontal and vertical components are integer pel */
            for(row = 0; row < blockSize; row++)
            {
                col = row & 0x1;
                for(; col < blockSize; col += 2)
                {
                    sad += ABS(refPos[col] - currPos[col]);
                }
                refPos += extendedVOPWidth;
                currPos += yWidth;
            }
            mv->SAD = (sad << 1);
        }
    }
    
    return;
}




/* {{-output"vbmHalfPelSearchMB.txt"}} */
/*
* vbmHalfPelSearchMB
*
* Parameters:
*     refFrame             pointer to reference frame
*     currMB               pointer to current MB
*     mbPos                pointer to macroblock position structure
*     vopWidth             width of frame/VOP
*     yWidth               width of luminance frame 
*     initPred             pointer to predictor motion vectors, pixel unit
*     bestMV               pointer to store the best match motion vector
* Function:
*     This function evaluates the half pel motion vector for a 16x16 block using 
*     the integer pel motion vector
*
* Returns:
*     Nothing.
*
* Error codes:
*     None.
*
*/
void vbmHalfPelSearchMB(tPixel* refFrame, u_int32 vopWidth, tPixel* currMB, 
                                                u_int32 yWidth, tMotionVector* initPred,
                                                tMotionVector* bestMV, tMBPosition* mbPos)
{
    int32 i;
    int32 position=0;
    
    initPred[0].SAD = bestMV->SAD;
    initPred[1].mvx = (int16)(bestMV->mvx + 1);
    initPred[1].mvy = bestMV->mvy;
    initPred[2].mvx = bestMV->mvx;
    initPred[2].mvy = (int16)(bestMV->mvy - 1);
    initPred[3].mvx = (int16)(bestMV->mvx - 1);
    initPred[3].mvy = bestMV->mvy;
    initPred[4].mvx = bestMV->mvx;
    initPred[4].mvy = (int16)(bestMV->mvy + 1);
    
    for(i = 1; i < 5; i++)
    {
        if(vbmMVOutsideBound(mbPos, &initPred[i], 1))
        {
            initPred[i].SAD = 65535;
        }
        else
        {
            vbmSADHalfPel(refFrame, vopWidth, currMB, yWidth, 
                (initPred + i), mbPos,(tBool)(0), 16);
        }
        
        if(initPred[i].SAD < bestMV->SAD)
        {
            bestMV->mvx = initPred[i].mvx;
            bestMV->mvy = initPred[i].mvy;
            bestMV->SAD = initPred[i].SAD;
            position = i;
        }
    }
    
    if(1)
    {
        if(bestMV->SAD == initPred[0].SAD)
        {
            return;
        }
        else
        {
            switch(position)
            {
            case 1: case 3:
                initPred[5].mvx = bestMV->mvx;
                initPred[5].mvy = (int16)(bestMV->mvy - 1);
                initPred[6].mvx = bestMV->mvx;
                initPred[6].mvy = (int16)(bestMV->mvy + 1);
                break;
                
            case 2: case 4:
                initPred[5].mvx = (int16)(bestMV->mvx - 1);
                initPred[5].mvy = bestMV->mvy;
                initPred[6].mvx = (int16)(bestMV->mvx + 1);
                initPred[6].mvy = bestMV->mvy;
                break;
                
            default:
                break;
            }
            
            for(i = 5; i < 7; i++)
            {
                
                if(vbmMVOutsideBound(mbPos, &initPred[i],1))
                {
                    initPred[i].SAD = 65535;
                }
                else
                {
                    vbmSADHalfPel(refFrame, vopWidth, currMB, yWidth,
                        (initPred + i), mbPos, (tBool)0, 16);
                }
                if(initPred[i].SAD < bestMV->SAD)
                {
                    bestMV->mvx = initPred[i].mvx;
                    bestMV->mvy = initPred[i].mvy;
                    bestMV->SAD = initPred[i].SAD;
                }
            }
        }
    }
    
    return;
}



/* {{-output"vbmMEMBSpatioTemporalSearch.txt"}} */
/*
* vbmMEMBSpatioTemporalSearch
*
* Parameters:
*     refFrame             pointer to reference frame
*     currMB               pointer to current MB
*     mbPos                pointer to macroblock position structure
*     vopWidth             width of frame/VOP
*     vopHeight            height of frame/VOP
*     yWidth               width of luminance frame 
*     initPred             pointer to predictor motion vectors, pixel unit
*     bestMV               pointer to store the best match motion vector
*     noOfPredictors       number of MV predictors
*     searchRange          search range
*     minSAD               minimum SAD
* Function:
*     This function performs motion estimation for a macroblock using 
*     spatio-temporal correlation based search
*
* Returns:
*     Nothing.
*
* Error codes:
*     None.
*
*/
int32 vbmMEMBSpatioTemporalSearch(tPixel* refFrame, u_int32 vopWidth, u_int32 vopHeight, 
                                                                    tPixel* currMB, u_int32 yWidth, tMBPosition* mbPos,
                                                                    tMotionVector   *initPred, int32 noOfPredictors,
                                                                    tMotionVector* bestMV, int32 searchRange, u_int32 minSAD)
{
    /* estimate the bound of MV for current MB */
    vbmEstimateBound(mbPos, vopWidth, vopHeight, searchRange);
    
    /* get the best MV predictor from the candidates set */
    vbmEstimateBestPredictor(refFrame, vopWidth, currMB, yWidth, initPred, mbPos, noOfPredictors, bestMV);
    
    if(bestMV->SAD >= minSAD)
    {
        /* from the MV predictor, starts the small diamond search    */
        vbmSmallDiamondSearch(refFrame, currMB, initPred, bestMV, vopWidth, mbPos, yWidth);
    }
    
    /* adjustment for half-pixel search */
    bestMV->mvx <<= 1;
    bestMV->mvy <<= 1;
    
    /* MV bound in half-pixel  */
    vbmEstimateBoundHalfPel(mbPos,vopWidth, vopHeight);
    
    /* starts the half-pixel search around the integer-pixel MV */
    vbmHalfPelSearchMB(refFrame, vopWidth, currMB, yWidth, 
        initPred, bestMV, mbPos);
    
    return bestMV->SAD;
}



/* {{-output"vbmRowDCT.txt"}} */
/*
* vbmRowDCT
*
* Parameters:
*     block                array of 64 block coefficients 
* Function:
*     This function performs row DCT of 8x8 block of data elements
*
* Returns:
*     Nothing.
*
* Error codes:
*     None.
*
*/
void vbmRowDCT(int16 *block)
{
    int32  coeff0, coeff1, coeff2, coeff3;
    int32  coeff4, coeff5, coeff6, coeff7;
    int32  temp;
    u_int16 count;
    int32  rowStartIndex;
    
    for(count = 0; count < BLOCK_SIZE; count++)
    {
        rowStartIndex = count << LOG_BLOCK_WIDTH;
        
        /* Stage 1 and up scaling of the input data to improve precision */
        coeff0 = (block[rowStartIndex] + block[rowStartIndex + 7]) <<
            DCT_KEPT_PRECISION;
        coeff7 = (block[rowStartIndex] - block[rowStartIndex + 7]) <<
            DCT_KEPT_PRECISION;
        
        coeff1 = (block[rowStartIndex + 1] + block[rowStartIndex + 6]) <<
            DCT_KEPT_PRECISION;
        coeff6 = (block[rowStartIndex + 1] - block[rowStartIndex + 6]) <<
            DCT_KEPT_PRECISION;
        
        coeff2 = (block[rowStartIndex + 2] + block[rowStartIndex + 5]) <<
            DCT_KEPT_PRECISION;
        coeff5 = (block[rowStartIndex + 2] - block[rowStartIndex + 5]) <<
            DCT_KEPT_PRECISION;
        
        coeff3 = (block[rowStartIndex + 3] + block[rowStartIndex + 4]) <<
            DCT_KEPT_PRECISION;
        coeff4 = (block[rowStartIndex + 3] - block[rowStartIndex + 4]) <<
            DCT_KEPT_PRECISION;
        
        /* Stage 2 */
        temp =   coeff0 + coeff3;
        coeff3 = coeff0 - coeff3;
        coeff0 = temp;
        
        temp =   coeff1 + coeff2;
        coeff2 = coeff1 - coeff2;
        coeff1 = temp;
        
        temp =   ((coeff6 - coeff5) * COS_PI_BY_4 + DCT_ROUND) >> DCT_PRECISION;
        coeff6 = ((coeff6 + coeff5) * COS_PI_BY_4 + DCT_ROUND) >> DCT_PRECISION;
        coeff5 = temp;
        
        /* Stage 3 */
        temp =   coeff0 + coeff1;
        coeff1 = coeff0 - coeff1;
        coeff0 = temp;
        
        temp =   ((coeff2 * TAN_PI_BY_8 + DCT_ROUND) >> DCT_PRECISION) + coeff3;
        coeff3 = ((coeff3 * TAN_PI_BY_8 + DCT_ROUND) >> DCT_PRECISION) - coeff2;
        coeff2 = temp;
        
        temp =   coeff4 + coeff5;
        coeff5 = coeff4 - coeff5;
        coeff4 = temp;
        
        temp =   coeff7 - coeff6;
        coeff7 = coeff7 + coeff6;
        coeff6 = temp;
        
        /* Stage 4 */
        temp =   ((coeff4 * TAN_PI_BY_16 + DCT_ROUND) >> DCT_PRECISION) + coeff7;
        coeff7 = ((coeff7 * TAN_PI_BY_16 + DCT_ROUND) >> DCT_PRECISION) - coeff4;
        coeff4 = temp;
        
        temp =   coeff5 + ((coeff6 * TAN_3PI_BY_16 + DCT_ROUND) >> DCT_PRECISION);
        coeff6 = coeff6 - ((coeff5 * TAN_3PI_BY_16 + DCT_ROUND) >> DCT_PRECISION);
        coeff5 = temp;
        
        block[rowStartIndex] =     (int16)coeff0;
        block[rowStartIndex + 4] = (int16)coeff1;
        block[rowStartIndex + 2] = (int16)coeff2;
        block[rowStartIndex + 6] = (int16)coeff3;
        block[rowStartIndex + 1] = (int16)coeff4;
        block[rowStartIndex + 5] = (int16)coeff5;
        block[rowStartIndex + 3] = (int16)coeff6;
        block[rowStartIndex + 7] = (int16)coeff7;
    }
    
    return;
}



/* {{-output"vbmDCTQuantInterSVH.txt"}} */
/*
* vbmDCTQuantInterSVH
*
* Parameters:
*     block                array of 64 block coefficients 
*     mbi                  contains info about MB quantization scale and coding type
*     lastPosition         indicates last non zero coefficient
* Function:
*     This function performs DCT of a 8x8 block of data elements and 
*     quantizes the DCT coefficients with short video header flag set
*
* Returns:
*     Nothing.
*
* Error codes:
*     None.
*
*/
void vbmDCTQuantInterSVH(int16 *block, tMBInfo *mbi, int32* lastPosition)
{
    int32  coeff0, coeff1, coeff2, coeff3;
    int32  coeff4, coeff5, coeff6, coeff7;
    int32  temp;
    u_int16 count;
    int32   sign;
    
    vbmRowDCT(block);
    
    for(count = 0; count < BLOCK_SIZE; count++)
    {
        /* Stage 1 */
        coeff0 = block[count] + block[count + 56];
        coeff7 = block[count] - block[count + 56];
        
        coeff1 = block[count + 8] + block[count + 48];
        coeff6 = block[count + 8] - block[count + 48];
        
        coeff2 = block[count + 16] + block[count + 40];
        coeff5 = block[count + 16] - block[count + 40];
        
        coeff3 = block[count + 24] + block[count + 32];
        coeff4 = block[count + 24] - block[count + 32];
        
        /* Stage 2 */
        temp =   coeff0 + coeff3;
        coeff3 = coeff0 - coeff3;
        coeff0 = temp;
        
        temp =   coeff1 + coeff2;
        coeff2 = coeff1 - coeff2;
        coeff1 = temp;
        
        temp =   ((coeff6 - coeff5) * COS_PI_BY_4 + DCT_ROUND) >> DCT_PRECISION;
        coeff6 = ((coeff6 + coeff5) * COS_PI_BY_4 + DCT_ROUND) >> DCT_PRECISION;
        coeff5 = temp;
        
        /* Stage 3 */
        temp =   coeff0 + coeff1;
        coeff1 = coeff0 - coeff1;
        coeff0 = temp;
        
        temp =   ((coeff2 * TAN_PI_BY_8 + DCT_ROUND) >> DCT_PRECISION) + coeff3;
        coeff3 = ((coeff3 * TAN_PI_BY_8 + DCT_ROUND) >> DCT_PRECISION) - coeff2;
        coeff2 = temp;
        
        temp =   coeff4 + coeff5;
        coeff5 = coeff4 - coeff5;
        coeff4 = temp;
        
        temp =   coeff7 - coeff6;
        coeff7 = coeff7 + coeff6;
        coeff6 = temp;
        
        /* Stage 4 */
        temp =   ((coeff4 * TAN_PI_BY_16 + DCT_ROUND) >> DCT_PRECISION) + coeff7;
        coeff7 = ((coeff7 * TAN_PI_BY_16 + DCT_ROUND) >> DCT_PRECISION) - coeff4;
        coeff4 = temp;
        
        temp =   coeff5 + ((coeff6 * TAN_3PI_BY_16 + DCT_ROUND) >> DCT_PRECISION);
        coeff6 = coeff6 - ((coeff5 * TAN_3PI_BY_16 + DCT_ROUND) >> DCT_PRECISION);
        coeff5 = temp;
        
        block[count] =     (int16) ( (coeff0* sPrePostMult[count] + 
            DCT_ROUND_PLUS_KEPT) >> DCT_PRECISION_PLUS_KEPT);
        block[count + 32] = (int16) ((coeff1* sPrePostMult[count+32] + 
            DCT_ROUND_PLUS_KEPT) >> DCT_PRECISION_PLUS_KEPT);
        block[count + 16] = (int16) ((coeff2* sPrePostMult[count+16] + 
            DCT_ROUND_PLUS_KEPT) >> DCT_PRECISION_PLUS_KEPT);
        block[count + 48] = (int16) ((coeff3* sPrePostMult[count+48] + 
            DCT_ROUND_PLUS_KEPT) >> DCT_PRECISION_PLUS_KEPT);
        block[count + 8] =  (int16) ((coeff4* sPrePostMult[count+8] + 
            DCT_ROUND_PLUS_KEPT) >> DCT_PRECISION_PLUS_KEPT);
        block[count + 40] = (int16) ((coeff5* sPrePostMult[count+40] + 
            DCT_ROUND_PLUS_KEPT) >> DCT_PRECISION_PLUS_KEPT);
        block[count + 24] = (int16) ((coeff6* sPrePostMult[count+24] + 
            DCT_ROUND_PLUS_KEPT) >> DCT_PRECISION_PLUS_KEPT);
        block[count + 56] = (int16) ((coeff7* sPrePostMult[count+56] + 
            DCT_ROUND_PLUS_KEPT) >> DCT_PRECISION_PLUS_KEPT);    
        
        coeff1 = mbi->QuantScale;
        temp= count;
        while(temp<64)
        {
            coeff0 = (int32) block[temp];   
            if (coeff0 >= 0) {
                sign = 1;
            }
            else{
                sign = -1;
            }
            coeff0 = sign * coeff0;
            
            if (coeff0 < ((coeff1 * 5 ) >> 1))
            {
                block[temp] = 0;
            } 
            else
            {
                coeff0 -= (coeff1 >> 1);
                coeff0 += 1;
                coeff0 *= sFixedQuantScale1[coeff1];
                coeff0 >>= FIXED_PT_BITS;
                coeff0 *= sign;
                
                if (coeff0 > MAX_SAT_VAL_SVH)
                {
                    coeff0 = MAX_SAT_VAL_SVH;
                }
                else if (coeff0 < MIN_SAT_VAL_SVH)
                {
                    coeff0 = MIN_SAT_VAL_SVH;
                }
                
                block[temp] = (int16) coeff0;       
                if( temp > (*lastPosition)) *lastPosition = temp;
            }
            temp = temp+8;
        }
    }
        
    return;
}



/* {{-output"vbmCBPYInter.txt"}} */
/*
* vbmCBPYInter
*
* Parameters:
*     mbi                  contains info about MB quantization scale and coding type
*     lastPosition         indicates last non zero coefficient
* Function:
*     This function evaluates the coded bit pattern of the Inter MB
*
* Returns:
*     Nothing.
*
* Error codes:
*     None.
*
*/
void vbmCBPYInter(tMBInfo *mbi, int32* lastPosition)
{
    int32 blkCnt;
    
    mbi->CodedBlockPattern = 0;
    for (blkCnt = 0; blkCnt < 6; blkCnt++)
    {   
        mbi->CodedBlockPattern <<= 1;
        
        if(lastPosition[blkCnt] != -1)
        {
            mbi->CodedBlockPattern |= 1;
        }
    }
    
    return;
}   



/* {{-output"vbmCBPYInter.txt"}} */
/*
* vbmCBPYInter
*
* Parameters:
*     refFrame             reference frame
*     currBlockPos         current block
*     block                block for storing the difference between predicted and
*                          actual pixel values.
*     mv                   motion vector for the block in half pixel unit
*     x                    x-coordinate for the first pixel of block
*     y                    y-coordinate for the first pixel of block
*     refFrameWidth        width of the reference frame
*     curFrameWidth        width of current Frame
*     extendVopSize        extension of the frame width
* Function:
*     This function finds the predicted block from the reference frame and 
*     copies the predicted frame into the current frame
*
* Returns:
*     Nothing.
*
* Error codes:
*     None.
*
*/
int32 vbmPredictBlock(tPixel *refFrame, int32 refFrameWidth, u_int32 extendVopSize,
                                            tPixel *currBlockPos, int32 curFrameWidth,
                                            int16 *block, tMotionVector *mv, int32 x, int32 y)
{
    int32 count1, count2;
    u_int8 *tempFrame, *tempFrame1, *tempFrame2;
    int32 tempVal, sad;
    int32 extframeWidth;
    extframeWidth = refFrameWidth + 2 * extendVopSize; /* Because of padding */
    
                                                                                                         /* 
                                                                                                         * Since the frame is appended by 8 rows and 8 colums on all sides
                                                                                                         * 0,0 of referenceframe will now be frameStartPoint
     */
        
    tempFrame = refFrame + extframeWidth * extendVopSize + extendVopSize
        + ((y + (mv->mvy >> 1)) * extframeWidth) + x + (mv->mvx >> 1);
    tempFrame2 = currBlockPos;
    
    if (mv->mvy & 1) /* Motion vector of y has half pel accuracy */
    {
        if (mv->mvx & 1) /* MV has half pel value in both coordinates */
        {
            tempFrame1 = tempFrame + extframeWidth;
            sad=0;
            for (count1 = 0; count1 < BLOCK_SIZE; count1++)
            {
                count2 = 0;
                tempVal = (tempFrame[count2]
                    + tempFrame[count2 + 1]
                    + tempFrame1[count2]
                    + tempFrame1[count2 + 1]+ 2) >> 2;
                
                block[count2] = (int16)(tempFrame2[count2] - tempVal);
                sad+=ABS(block[count2]);
                tempVal = (tempFrame[count2 + 1]
                    + tempFrame[count2 + 2]
                    + tempFrame1[count2 + 1]
                    + tempFrame1[count2 + 2]+ 2) >> 2;
                
                block[count2 + 1] = (int16)(tempFrame2[count2 + 1] - tempVal);
                sad+=ABS(block[count2+1]);
                tempVal = (tempFrame[count2 + 2]
                    + tempFrame[count2 + 3]
                    + tempFrame1[count2 + 2]
                    + tempFrame1[count2 + 3]+ 2) >> 2;
                
                block[count2 + 2] = (int16)(tempFrame2[count2 + 2] - tempVal);
                sad+=ABS(block[count2+2]);
                tempVal = (tempFrame[count2 + 3]
                    + tempFrame[count2 + 4]
                    + tempFrame1[count2 + 3]
                    + tempFrame1[count2 + 4]+ 2) >> 2;
                
                block[count2 + 3] = (int16)(tempFrame2[count2 + 3] - tempVal);
                sad+=ABS(block[count2+3]);
                tempVal = (tempFrame[count2 + 4]
                    + tempFrame[count2 + 5]
                    + tempFrame1[count2 + 4]
                    + tempFrame1[count2 + 5]+ 2) >> 2;
                
                block[count2 + 4] = (int16)(tempFrame2[count2 + 4] - tempVal);
                sad+=ABS(block[count2+4]);
                tempVal = (tempFrame[count2 + 5]
                    + tempFrame[count2 + 6]
                    + tempFrame1[count2 + 5]
                    + tempFrame1[count2 + 6]+ 2) >> 2;
                
                block[count2 + 5] = (int16)(tempFrame2[count2 + 5] - tempVal);
                sad+=ABS(block[count2+5]);
                tempVal = (tempFrame[count2+ 6]
                    + tempFrame[count2 + 7]
                    + tempFrame1[count2 + 6]
                    + tempFrame1[count2 + 7]+ 2) >> 2;
                
                block[count2 + 6] = (int16)(tempFrame2[count2 + 6] - tempVal);
                sad+=ABS(block[count2+6]);
                tempVal = (tempFrame[count2 + 7]
                    + tempFrame[count2 + 8]
                    + tempFrame1[count2 + 7]
                    + tempFrame1[count2 + 8]+ 2) >> 2;
                
                block[count2 + 7] = (int16)(tempFrame2[count2 + 7] - tempVal);
                sad+=ABS(block[count2+7]);
                block += BLOCK_SIZE;
                tempFrame += extframeWidth;
                tempFrame1 += extframeWidth;
                tempFrame2 += curFrameWidth;
            }
        }
        else  /* MV has half pel only in y direction */
        {
            tempFrame1 = tempFrame + extframeWidth;
            sad=0;
            for (count1 = 0; count1 < BLOCK_SIZE; count1++)
            {
                count2 = 0;
                tempVal = (tempFrame[count2]
                    + tempFrame1[count2]+ 1) >> 1;
                
                block[count2] = (int16)(tempFrame2[count2] - tempVal);
                sad+=ABS(block[count2]);
                tempVal = (tempFrame[count2 + 1]
                    + tempFrame1[count2 + 1]+ 1) >> 1;
                
                block[count2 + 1] = (int16)(tempFrame2[count2 + 1] - tempVal);
                sad+=ABS(block[count2+1]);
                tempVal = (tempFrame[count2 + 2]
                    + tempFrame1[count2 + 2]+ 1) >> 1;
                
                block[count2 + 2] = (int16)(tempFrame2[count2 + 2] - tempVal);
                sad+=ABS(block[count2+2]);
                tempVal = (tempFrame[count2 + 3]+ tempFrame1[count2 + 3]+1) >> 1;
                
                block[count2 + 3] = (int16)(tempFrame2[count2 + 3] - tempVal);
                sad+=ABS(block[count2+3]);
                tempVal = (tempFrame[count2 + 4]
                    + tempFrame1[count2 + 4]+ 1) >> 1;
                
                block[count2 + 4] = (int16)(tempFrame2[count2 + 4] - tempVal);
                sad+=ABS(block[count2+4]);
                tempVal = (tempFrame[count2 + 5]
                    + tempFrame1[count2 + 5]+ 1) >> 1;
                
                block[count2 + 5] = (int16)(tempFrame2[count2 + 5] - tempVal);
                sad+=ABS(block[count2+5]);
                tempVal = (tempFrame[count2+ 6]
                    + tempFrame1[count2 + 6]+ 1) >> 1;
                
                block[count2 + 6] = (int16)(tempFrame2[count2 + 6] - tempVal);
                sad+=ABS(block[count2+6]);
                tempVal = (tempFrame[count2 + 7]
                    + tempFrame1[count2 + 7]+ 1) >> 1;
                
                block[count2 + 7] = (int16)(tempFrame2[count2 + 7] - tempVal);
                sad+=ABS(block[count2+7]);
                block += BLOCK_SIZE;
                tempFrame += extframeWidth;
                tempFrame1 += extframeWidth;
                tempFrame2 += curFrameWidth;
            }
        }
    }
    else
    {
        if (mv->mvx & 1) /* MV has half pel only in x direction */
        {
            sad=0;
            for (count1 = 0; count1 < BLOCK_SIZE; count1++)
            {
                count2 = 0;
                tempVal = (tempFrame[count2]
                    + tempFrame[count2 + 1]+1) >> 1;
                
                block[count2] = (int16)(tempFrame2[count2] - tempVal);
                sad+=ABS(block[count2]);
                tempVal = (tempFrame[count2 + 1]
                    + tempFrame[count2 + 2]+ 1) >> 1;
                
                block[count2 + 1] = (int16)(tempFrame2[count2 + 1] - tempVal);
                sad+=ABS(block[count2+1]);
                tempVal = (tempFrame[count2 + 2]
                    + tempFrame[count2 + 3]+ 1) >> 1;
                
                block[count2 + 2] = (int16)(tempFrame2[count2 + 2] - tempVal);
                sad+=ABS(block[count2+2]);
                tempVal = (tempFrame[count2 + 3]
                    + tempFrame[count2 + 4]+1) >> 1;
                
                block[count2 + 3] = (int16)(tempFrame2[count2 + 3] - tempVal);
                sad+=ABS(block[count2+3]);
                tempVal = (tempFrame[count2 + 4]
                    + tempFrame[count2 + 5]+ 1) >> 1;
                
                block[count2 + 4] = (int16)(tempFrame2[count2 + 4] - tempVal);
                sad+=ABS(block[count2+4]);
                tempVal = (tempFrame[count2 + 5]
                    + tempFrame[count2 + 6]+ 1) >> 1;
                
                block[count2 + 5] = (int16)(tempFrame2[count2 + 5] - tempVal);
                sad+=ABS(block[count2+5]);
                tempVal = (tempFrame[count2+ 6]
                    + tempFrame[count2 + 7]
                    + 1) >> 1;
                
                block[count2 + 6] = (int16)(tempFrame2[count2 + 6] - tempVal);
                sad+=ABS(block[count2+6]);
                tempVal = (tempFrame[count2 + 7]
                    + tempFrame[count2 + 8]+ 1) >> 1;
                
                block[count2 + 7] = (int16)(tempFrame2[count2 + 7] - tempVal);
                sad+=ABS(block[count2+7]);
                block += BLOCK_SIZE;
                tempFrame += extframeWidth;
                tempFrame2 += curFrameWidth;
            }
        }
        else    /* MV has full pel accuracy in both coordinates */
        {
            sad=0;
            for (count1 = 0; count1 < BLOCK_SIZE; count1++)
            {
                for (count2 = 0; count2 < BLOCK_SIZE; count2++)
                {
                    block[count2] = (int16)(tempFrame2[count2]- tempFrame[count2]);
                    sad+=ABS(block[count2]);
                }
                block += BLOCK_SIZE;
                tempFrame += extframeWidth;
                tempFrame2 += curFrameWidth;
            }
        }
    }
    return sad;
}




/* {{-output"vbmPutInterMBSVH.txt"}} */
/*
* vbmPutInterMBSVH
*
* Parameters:
*     outBuf               pointer to the output bit-stream buffer structure
*     mbData               pointer to the macro block data structure
*     mbi                  pointer to macro block information structure.
*     numTextureBits       pointer to store the number of bits needed to encode macro block
*     predMV               pointer to the predicted motion vector data
*     lastPos              pointer to last non-zero position of each block in the macro block
*     colorEffect          color effect applied
* Function:
*     This function encodes INTER macroblock in SVH mode
*
* Returns:
*     Nothing.
*
* Error codes:
*     None.
*
*/
void vbmPutInterMBSVH(tMacroblockData* mbData, tMBInfo* mbi, bibBuffer_t *outBuf, 
                                            int32 *numTextureBits, tMotionVector* predMV, int32* lastPos, 
                                            int16 colorEffect)
{
  int32  dQuant;
    int32    mcbpcVal;
    int32    index;
    int32    cbpy;
    int32    textureBits;
    int16* coeff;
    int32    len;
    
    vlbPutBits(outBuf, 1, mbi->SkippedMB);
    if(mbi->SkippedMB == ON)
    {
        *numTextureBits = 0;
        return;
    }
    
    dQuant = mbi->dQuant;
    mcbpcVal = colorEffect? 0 : (mbi->CodedBlockPattern & 3);
    cbpy = (~((mbi->CodedBlockPattern >> 2) & 0xf)) & 0xf;
    
    /* only ONE MV in baseline H263 */
    index = (dQuant == 0 ? 0 : 4) + mcbpcVal;
    len = sCBPCPType[index].length + sCBPY[cbpy].length;
    mcbpcVal = (sCBPCPType[index].code << sCBPY[cbpy].length) | sCBPY[cbpy].code;
    
    vlbPutBits(outBuf, len, mcbpcVal);
    
    if(dQuant != 0)
    {
        vlbPutBits(outBuf, 2, sDquant[dQuant + 2]);
    }
    
    /* encode motion vectors */
    {
        vbmEncodeMVDifferential(mbi->MV[0][0] - predMV[0].mvx,
            mbi->MV[0][1] - predMV[0].mvy,
            1, outBuf);
    }
    
    /* encode texture coefficents */
    textureBits = 0;
    cbpy = 32;
    for (index = 0; index < (colorEffect ? 4 : 6); index++)
    {
        if(cbpy & mbi->CodedBlockPattern)
        {
            coeff = mbData->Data + index * 64;
            textureBits += vlbCodeACCoeffsSVHWithZigZag(0, coeff, outBuf, ON, lastPos[index]);
        }
        cbpy >>= 1;
    }
    *numTextureBits = textureBits;
    
    return;
}

/*
*******************************************************************************
Name            : vbmPutInterMB
Description     : INTER macroblock is encoded here.
Parameter       : 
    mbData: Pointer to the macro block data structure
    mbi:        Pointer to the macro block information structure.
    vopData:    Pointer to the VOP data structure.
    currFrame:  Pointer to the current frame data
    mbNo:       The number macro block being encoded.
    numTextureBits:Pointer to store the number of bits taken to encode the macro block.
    prevQuant:  Pointer to the quantization sacle.
    blockSAD:   Pointer to store the block SAD values.                  
    outBuf:     Pointer to the output bit-stream buffer structure
    mvi:        Pointer to the MV array
Return Value    : void
*******************************************************************************
*/
/* {{-output"vbmPutInterMB.txt"}} */
/*
* vbmPutInterMB
*
* Parameters:
*     outBuf               pointer to the output bit-stream buffer structure
*     mbPos                pointer to macroblock position structure
*     paramMB              pointer to macroblock parameters structure
*     initPred             pointer to predictor motion vectors, pixel unit
*     noOfPredictors       number of MV predictors
*     numTextureBits       pointer to store the number of bits needed to encode macro block
*     colorEffect          color effect applied
*     vopWidth             width of frame/VOP
*     vopHeight            height of frame/VOP
*     mbsinfo              pointer to macro block information structure
*     searchRange          search range
* Function:
*     This function encodes INTER macroblock
*
* Returns:
*     Nothing.
*
* Error codes:
*     None.
*
*/
void vbmPutInterMB(tMBPosition* mbPos, bibBuffer_t *outBuf, dmdPParam_t *paramMB,
                                     tMotionVector *initPred, int32 noOfPredictors, u_int32 vopWidth, 
                                     u_int32 vopHeight,int32 searchRange, int32 mbNo, 
                                     int32* numTextureBits, int16 colorEffect, tMBInfo *mbsinfo)
{
    int32           blkCnt;
    tMotionVector   predMV[4];
    int32           lastPosition[6] = {-1,-1,-1,-1,-1,-1};
    tPixel *currBlockPos = NULL; 
    tMotionVector bestMV; 
    tMacroblockData mbData;
    tMBInfo *mbi = mbsinfo + mbNo;
    mbi->SkippedMB = OFF;
    u_int32 yWidth = paramMB->uvWidth * 2;
    
    bestMV.mvx = 0;
    bestMV.mvy = 0;
    bestMV.SAD = MB_SIZE * MB_SIZE / 2; // quich search stop threshold
    
    vbmMEMBSpatioTemporalSearch(paramMB->refY, vopWidth,vopHeight,paramMB->currYMBInFrame, yWidth, 
        mbPos, initPred, noOfPredictors, &bestMV,
        searchRange, bestMV.SAD);
    
    /* update MV buffer */
    mbi->MV[0][0] = bestMV.mvx;
    mbi->MV[0][1] = bestMV.mvy;
    mbi->SAD      = bestMV.SAD;
    currBlockPos = paramMB->currYMBInFrame;
    
    for (blkCnt= 0; blkCnt < 4; blkCnt++)
    {
        int blkPosX, blkPosY;
        blkPosX = (blkCnt & 1) ? mbPos->x + 8 : mbPos->x;
        blkPosY = (blkCnt & 2) ? mbPos->y + 8 : mbPos->y;
        
        vbmPredictBlock(paramMB->refY, vopWidth, 0, currBlockPos, yWidth,
            &mbData.Data[blkCnt * BLOCK_COEFF_SIZE], 
            &bestMV, blkPosX, blkPosY);
        vbmDCTQuantInterSVH(&mbData.Data[blkCnt * BLOCK_COEFF_SIZE], 
            mbi, &(lastPosition[blkCnt]));
        currBlockPos += 8;
        if (blkCnt & 1)
            currBlockPos += 8 * yWidth - 16;
    }
    
    if (!colorEffect)
    {
        /* Find the Chrominance Block Motion vectors */
        tMotionVector lChrMv;
        
        lChrMv.mvx = (int16)(bestMV.mvx % 4 == 0 ? bestMV.mvx >> 1 : (bestMV.mvx >> 1) | 1);
        lChrMv.mvy = (int16)(bestMV.mvy % 4 == 0 ? bestMV.mvy >> 1 : (bestMV.mvy >> 1) | 1);
        blkCnt = 4; /* U */
        vbmPredictBlock(paramMB->refU, paramMB->uvWidth, 0, paramMB->currUBlkInFrame, paramMB->uvWidth,
            &mbData.Data[blkCnt * BLOCK_COEFF_SIZE], 
            &lChrMv, mbPos->x >> 1, mbPos->y >> 1);
        vbmDCTQuantInterSVH(&mbData.Data[blkCnt * BLOCK_COEFF_SIZE], 
            mbi, &(lastPosition[blkCnt]));
        blkCnt = 5; /* V */
        vbmPredictBlock(paramMB->refV, paramMB->uvWidth, 0, paramMB->currVBlkInFrame, paramMB->uvWidth,
            &mbData.Data[blkCnt * BLOCK_COEFF_SIZE], 
            &lChrMv, mbPos->x >> 1, mbPos->y >> 1);
        vbmDCTQuantInterSVH(&mbData.Data[blkCnt * BLOCK_COEFF_SIZE], 
            mbi, &(lastPosition[blkCnt]));
    }
    
    vbmCBPYInter(mbi, lastPosition);
    if((mbi->CodedBlockPattern == 0) &&
        (mbi->MV[0][0] == 0) && (mbi->MV[0][1] == 0))
    {
        mbi->SkippedMB = ON;
    }
    else //(mbi->SkippedMB == OFF)
    {
        vbmMvPrediction(mbi, mbNo, predMV, vopWidth / MB_SIZE);
    }
    vbmPutInterMBSVH(&mbData, mbi, outBuf,
        numTextureBits, predMV, lastPosition, colorEffect);
    
    return;
}


/* End of bma.cpp */

