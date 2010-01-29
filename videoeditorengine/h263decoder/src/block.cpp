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
* Motion compensation functions.
*
*/


#include "h263dConfig.h"

#include "block.h"

#include "errcodes.h"
#include "vdcmvc.h"
#include "debug.h"

#ifndef blcAssert
   #define blcAssert assert
#endif
#include "blcllcpy.cpp"

/* See the description above. */
typedef void (* blcCopyBlock_t) (u_char *, u_char *,
    int, int, int, int, int, int, int, int, int, int, int, int, int);

/* This structure is used to pass a pointer either to blcUVCountRefXY1MV or
   blcUVCountRefXY4MVs. */
typedef void (* blcUVCountRefXY_t) 
   (int, int, int *, int *, int *, int *, int *, int *);


/*
 * Local function prototypes
 */

static int blcCopyNormalYPredictionMBWith1MV(
   blcCopyBlock_t copyBlock,
   int *mvxArray, int *mvyArray, 
   u_char *dstYBlk,
   u_char *srcYFrame,
   int dstYXPos, int dstYYPos, 
   int dstYXSize,
   int srcYXSize, int srcYYSize,
   int fMVsOverPictureBoundaries, 
   int rcontrol);

static int blcCopyNormalYPredictionMBWith4MVs(
   blcCopyBlock_t copyBlock,
   int *mvxArray, int *mvyArray, 
   u_char *dstYBlk,
   u_char *srcYFrame,
   int dstYXPos, int dstYYPos, 
   int dstYXSize,
   int srcYXSize, int srcYYSize,
   int fMVsOverPictureBoundaries, 
   int rcontrol);

static int blcCopyUVPredictionBlocks(
   blcUVCountRefXY_t uvCountSourceXY,
   blcCopyBlock_t copyBlock,
   int *mvxArray, int *mvyArray, 
   u_char *dstUBlk, u_char *dstVBlk,
   u_char *srcUFrame, u_char *srcVFrame, 
   int dstUVXPos, int dstUVYPos, 
   int dstUVXSize,
   int srcUVXSize, int srcUVYSize,
   int fMVsOverPictureBoundaries, 
   int rcontrol);

static void blcHandleRefOverPictBoundariesBC(
   int xBlkSize, int yBlkSize,
   int xSize, int ySize, 
   int subpixelX, int subpixelY, 
   int *sourceX, int *sourceY,
   int *xlt0, int *xNormal, int *xgtmax, 
   int *ylt0, int *yNormal, int *ygtmax);

static int blcIsRefOverPictBoundaries(
   int xBlkSize, int yBlkSize,
   int xSize, int ySize, 
   int subpixelX, int subpixelY, 
   int sourceX, int sourceY);

static void blcUVCountMVOffset4MVs(
   int *mvxArray, int *mvyArray,
   int *offsetX, int *offsetY,
   int *subpixelX, int *subpixelY);

static void blcUVCountRefXY1MV(
   int origoX, int origoY,
   int *mvxArray, int *mvyArray, 
   int *sourceX, int *sourceY,
   int *subpixelX, int *subpixelY);

static void blcUVCountRefXY4MVs(
   int origoX, int origoY,
   int *mvxArray, int *mvyArray, 
   int *sourceX, int *sourceY,
   int *subpixelX, int *subpixelY);

static void blcYCountRefXY(
   int mvxVal, int mvyVal,
   int destX, int destY, 
   int *sourceX, int *sourceY,
   int *subpixelX, int *subpixelY);


/*
 * Module-scope constants
 */

/* Clipping table to sature values to range 0..255 */
static const u_char wholeClippingTable[4*256] = {
        0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   1,   2,   3,   4,
        5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
       15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
       25,  26,  27,  28,  29,  30,  31,  32,  33,  34,
       35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
       45,  46,  47,  48,  49,  50,  51,  52,  53,  54,
       55,  56,  57,  58,  59,  60,  61,  62,  63,  64,
       65,  66,  67,  68,  69,  70,  71,  72,  73,  74,
       75,  76,  77,  78,  79,  80,  81,  82,  83,  84,
       85,  86,  87,  88,  89,  90,  91,  92,  93,  94,
       95,  96,  97,  98,  99, 100, 101, 102, 103, 104,
      105, 106, 107, 108, 109, 110, 111, 112, 113, 114,
      115, 116, 117, 118, 119, 120, 121, 122, 123, 124,
      125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
      135, 136, 137, 138, 139, 140, 141, 142, 143, 144,
      145, 146, 147, 148, 149, 150, 151, 152, 153, 154,
      155, 156, 157, 158, 159, 160, 161, 162, 163, 164,
      165, 166, 167, 168, 169, 170, 171, 172, 173, 174,
      175, 176, 177, 178, 179, 180, 181, 182, 183, 184,
      185, 186, 187, 188, 189, 190, 191, 192, 193, 194,
      195, 196, 197, 198, 199, 200, 201, 202, 203, 204,
      205, 206, 207, 208, 209, 210, 211, 212, 213, 214,
      215, 216, 217, 218, 219, 220, 221, 222, 223, 224,
      225, 226, 227, 228, 229, 230, 231, 232, 233, 234,
      235, 236, 237, 238, 239, 240, 241, 242, 243, 244,
      245, 246, 247, 248, 249, 250, 251, 252, 253, 254,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255};

static const u_char * const clippingTable = &wholeClippingTable[512];


/*
 * Global functions
 */

/* {{-output"blcAddBlock.txt"}} */
/*
 * blcAddBlock
 *    
 *
 * Parameters:
 *    block                block array (of 64 pixels)
 *    dstBlk               pointer to present frame in the place,
 *                         where the block is added
 *    xSize                X size of the frame.
 *    mbPlace              flag that indicates the place for the current
 *                         macroblock inside the macroblock row:
 *                            -1 beginning of row
 *                             0 middle of row
 *                             1 end of row
 *    fourMVs              1 if Advanced Prediction Mode is used, otherwise 0
 *    prevDiffBlock        if fourMVs == 1 and mbPlace <= 0 the difference
 *                         block is stored to prevDiffBlock for later use
 *
 * Function:
 *    This function sums the given block into block being currently decoded in
 *    present frame. Parameters are the table consisting a block, a pointer to
 *    the frame at the correct place and the width of the frame.
 *
 * Returns:
 *    Nothing.
 *
 */

void blcAddBlock(int *block, u_char *dstBlk, int xSize,
   int mbPlace, u_char fourMVs, int *prevDiffBlock)
/* {{-output"blcAddBlock.txt"}} */
{
   int i;

   if (fourMVs && mbPlace <= 0) {
       MEMCPY(prevDiffBlock, block, 64 * sizeof(int));
   }
   else {
      for(i = 8; i; i--) {
         *dstBlk++ = clippingTable[*dstBlk + *block++];
         *dstBlk++ = clippingTable[*dstBlk + *block++];
         *dstBlk++ = clippingTable[*dstBlk + *block++];
         *dstBlk++ = clippingTable[*dstBlk + *block++];
         *dstBlk++ = clippingTable[*dstBlk + *block++];
         *dstBlk++ = clippingTable[*dstBlk + *block++];
         *dstBlk++ = clippingTable[*dstBlk + *block++];
         *dstBlk++ = clippingTable[*dstBlk + *block++];
         dstBlk += xSize - 8;
      }
   }
}


/* {{-output"blcBlockToFrame.txt"}} */
/*
 * blcBlockToFrame
 *    
 *
 * Parameters:
 *    block        block array (of 64 pixels)
 *    dstBlk       pointer to present frame in the place, where the block is
 *                 written
 *    xSize        X size of the frame.
 *
 * Function:
 *    This function writes the given block into block being currently decoded
 *    in present frame. Parameters are the table consisting a block, a pointer
 *    to the frame at the correct place and the width of the frame.
 *
 * Returns:
 *      Nothing.
 *
 */

void blcBlockToFrame(int *block, u_char *dstBlk, int xSize)
/* {{-output"blcBlockToFrame.txt"}} */
{
   int i;

   for( i = 0; i < 8; i++ )
   {
     *dstBlk = clippingTable[ *block ];
     *(dstBlk+1) = clippingTable[ *(block+1) ];
     *(dstBlk+2) = clippingTable[ *(block+2) ];
     *(dstBlk+3) = clippingTable[ *(block+3) ];
     *(dstBlk+4) = clippingTable[ *(block+4) ];
     *(dstBlk+5) = clippingTable[ *(block+5) ];
     *(dstBlk+6) = clippingTable[ *(block+6) ];
     *(dstBlk+7) = clippingTable[ *(block+7) ];
     dstBlk += xSize;
     block += 8;
   }
}


/* {{-output"blcCopyPredictionMB.txt"}} */
/*
 * blcCopyPredictionMB
 *    
 *
 * Parameters:
 *    param                      input and output parameters
 *
 * Function:
 *    This function copies one macroblock from previous frame into present
 *    frame at the location of macroblock being currently decoded. 
 *    The location where the macroblock is read is the location of 
 *    the current block changed with motion vectors.
 *
 * Returns:
 *    >= 0                       the function was successful
 *    < 0                        an error occured
 *
 */

int blcCopyPredictionMB(blcCopyPredictionMBParam_t *param)
/* {{-output"blcCopyPredictionMB.txt"}} */
{
   int
      uvWidth = param->uvWidth,
      uvHeight = param->uvHeight,
      yWidth = uvWidth * 2,
      yHeight = uvHeight * 2,
      uvDstX = param->uvBlkXCoord,
      uvDstY = param->uvBlkYCoord,
      yDstX = uvDstX * 2,
      yDstY = uvDstY * 2,
      *mvx = param->mvx, 
      *mvy = param->mvy,
      status;

   u_char
      *dstYBlk = param->currYMBInFrame,
      *dstUBlk = param->currUBlkInFrame,
      *dstVBlk = param->currVBlkInFrame,
      *srcYFrame = param->refY,
      *srcUFrame = param->refU,
      *srcVFrame = param->refV;

   blcCopyBlock_t copyBlock = (param->fMVsOverPictureBoundaries) ?
      blcCopyBlockBC : blcCopyBlockNBC;

   blcAssert(param != NULL);
   blcAssert(param->rcontrol == 0 || param->rcontrol == 1);

   /* Copy UV prediction blocks */
   status = blcCopyUVPredictionBlocks(
      (param->fourMVs) ? 
         blcUVCountRefXY4MVs : blcUVCountRefXY1MV,
      copyBlock,
      mvx, mvy,
      dstUBlk, dstVBlk,
      srcUFrame, srcVFrame,
      uvDstX, uvDstY,
      uvWidth,
      uvWidth, uvHeight,
      param->fMVsOverPictureBoundaries,
      param->rcontrol);

   if (status < 0) 
      return status;

   /* If Advanced Prediction is in use */
   if (param->fAdvancedPrediction) {

        // not supported
        
   }

   /* Else normal prediction mode is in use */
   else { 

      /* Copy Y prediction MB */
      if(param->fourMVs)
         status = blcCopyNormalYPredictionMBWith4MVs(
            copyBlock,
            mvx, mvy,
            dstYBlk,
            srcYFrame,
            yDstX, yDstY,
            yWidth,
            yWidth, yHeight,
            param->fMVsOverPictureBoundaries,
            param->rcontrol);
      else
         status = blcCopyNormalYPredictionMBWith1MV(
            copyBlock,
            mvx, mvy,
            dstYBlk,
            srcYFrame,
            yDstX, yDstY,
            yWidth,
            yWidth, yHeight,
            param->fMVsOverPictureBoundaries,
            param->rcontrol);
      if (status < 0)
         return status;
   }

   return 0;
}





/*
 *    Local functions
 */





/*
 * blcCopyNormalYPredictionMBWith1MV
 *    
 *
 * Parameters:
 *    copyBlock                  a pointer to either blcCopyBlockBC or
 *                               blcCopyBlockNBC (or their Assembler versions)
 *
 *    mvxArray                   an array of four for x component of MVs
 *    mvyArray                   an array of four for y component of MVs
 *                               (Only the first entry of the array is used.)
 *
 *    dstYBlk                    a pointer to the current Y macroblock
 *                               in the destination Y frame
 *
 *    srcYFrame                  the top-left corner of the source Y frame
 *
 *    dstYXPos                   the x coordinate of dstYBlk (in pixels)
 *    dstYYPos                   the y coordinate of dstYBlk (in pixels)
 *
 *    dstYXSize                  the width of the Y destination frame
 *    srcYXSize                  the width of the Y source frame
 *    srcYYSize                  the height of the Y source frame
 *
 *    fMVsOverPictureBoundaries  non-zero if motion vectors may point outside
 *                               picture boundaries, zero otherwise
 *
 *    rcontrol                   RCONTROL (section 6.1.2 of the H.263 standard)
 *
 * Function:
 *    This function copies a luminance prediction macroblock from the given
 *    source frame to the given position of the destination frame.
 *    The prediction macroblock is associated with one motion vector.
 *
 * Returns:
 *    >= 0 if everything is ok
 *    < 0 if an error occured
 *
 */

static int blcCopyNormalYPredictionMBWith1MV(
   blcCopyBlock_t copyBlock,
   int *mvxArray, int *mvyArray, 
   u_char *dstYBlk,
   u_char *srcYFrame,
   int dstYXPos, int dstYYPos, 
   int dstYXSize,
   int srcYXSize, int srcYYSize,
   int fMVsOverPictureBoundaries, 
   int rcontrol)
{
   int 
      mvxVal = *mvxArray, mvyVal = *mvyArray,
      subpixelX, subpixelY,
      xlt0, xNormal, xgtmax,
      ylt0, yNormal, ygtmax,
      srcXPos, srcYPos;
   u_char *srcBlk;

   blcYCountRefXY(mvxVal, mvyVal, dstYXPos, dstYYPos, &srcXPos, &srcYPos,
      &subpixelX, &subpixelY);

   if (fMVsOverPictureBoundaries)
      blcHandleRefOverPictBoundariesBC(
         16, 16,
         srcYXSize, srcYYSize, 
         subpixelX, subpixelY, 
         &srcXPos, &srcYPos,
         &xlt0, &xNormal, &xgtmax, 
         &ylt0, &yNormal, &ygtmax);

   else {
      if (blcIsRefOverPictBoundaries(
         16, 16,
         srcYXSize, srcYYSize, 
         subpixelX, subpixelY, 
         srcXPos, srcYPos))
         return -1;
      xlt0 = xgtmax = ylt0 = ygtmax = 0;
      xNormal = yNormal = 16;
   }

   srcBlk = srcYFrame + srcYPos * srcYXSize + srcXPos;

   copyBlock(srcBlk, dstYBlk, xlt0, xNormal, xgtmax,
      ylt0, yNormal, ygtmax, subpixelX, subpixelY,
      srcYXSize, dstYXSize, 16, 16, rcontrol);

   return 0;
}


/*
 * blcCopyNormalYPredictionMBWith4MVs
 *    
 *
 * Parameters:
 *    See blcCopyNormalYPredictionMBWith1MV.
 *    All 4 entries of mvxArray and mvyArray are used.
 *
 * Function:
 *    This function copies a luminance prediction macroblock from the given
 *    source frame to the given position of the destination frame.
 *    The prediction macroblock is associated with four motion vectors.
 *
 * Returns:
 *    >= 0 if everything is ok
 *    < 0 if an error occured
 *
 */

static int blcCopyNormalYPredictionMBWith4MVs(
   blcCopyBlock_t copyBlock,
   int *mvxArray, int *mvyArray, 
   u_char *dstYBlk,
   u_char *srcYFrame,
   int dstYXPos, int dstYYPos, 
   int dstYXSize,
   int srcYXSize, int srcYYSize,
   int fMVsOverPictureBoundaries, 
   int rcontrol)
{
   int 
      nh, nv, 
      mvi = 0, 
      mvxVal, mvyVal,
      subpixelX, subpixelY,
      xlt0, xNormal, xgtmax,
      ylt0, yNormal, ygtmax,
      srcXPos, srcYPos;

   u_char 
      *srcBlk, 
      *origDest; 

   origDest = dstYBlk; 
   for (nv = 0; nv <= 1; nv++, dstYYPos += 8,
      origDest += (dstYXSize << 3) - 16, dstYXPos -= 16) {
      for (nh = 0; nh <= 1; nh++, dstYXPos += 8, origDest += 8, mvi++) { 
         dstYBlk = origDest; 
         mvxVal = mvxArray[mvi];
         mvyVal = mvyArray[mvi];

         blcYCountRefXY(mvxVal, mvyVal, dstYXPos, dstYYPos, &srcXPos, &srcYPos,
            &subpixelX, &subpixelY);

         if (fMVsOverPictureBoundaries)
            blcHandleRefOverPictBoundariesBC(
               8, 8,
               srcYXSize, srcYYSize, 
               subpixelX, subpixelY, 
               &srcXPos, &srcYPos,
               &xlt0, &xNormal, &xgtmax, 
               &ylt0, &yNormal, &ygtmax);

         else {
            if (blcIsRefOverPictBoundaries(
               8, 8,
               srcYXSize, srcYYSize, 
               subpixelX, subpixelY, 
               srcXPos, srcYPos))
               return -1;
            xlt0 = xgtmax = ylt0 = ygtmax = 0;
            xNormal = yNormal = 8;
         }

         srcBlk = srcYFrame + srcYPos * srcYXSize + srcXPos; 

         copyBlock(srcBlk, dstYBlk, xlt0, xNormal, xgtmax, 
            ylt0, yNormal, ygtmax, subpixelX, subpixelY, srcYXSize, 
            dstYXSize, 8, 8, rcontrol); 
      } 
   } 

   return 0;
}



/*
 * blcCopyUVPredictionBlocks
 *    
 *
 * Parameters:
 *    uvCountSourceXY            a pointer to either blcUVCountRefXY1MV or 
 *                               blcUVCountRefXY4MVs
 *
 *    copyBlock                  a pointer to either blcCopyBlockBC or
 *                               blcCopyBlockNBC (or their Assembler versions)
 *
 *    mvxArray                   an array of four for x component of MVs
 *    mvyArray                   an array of four for y component of MVs
 *
 *    dstUBlk                    a pointer to the current U block
 *                               in the destination U frame
 *    dstVBlk                    a pointer to the current V block
 *                               in the destination V frame
 *
 *    srcUFrame                  the top-left corner of the source U frame
 *    srcVFrame                  the top-left corner of the source V frame
 *
 *    dstUVXPos                  the x coordinate of dstUBlk
 *    dstUVYPos                  the y coordinate of dstUBlk
 *
 *    dstUVXSize                 the width of the U and V destination frame
 *    srcUVXSize                 the width of the U and V source frame
 *    srcUVYSize                 the height of the U and V source frame
 *
 *    fMVsOverPictureBoundaries  non-zero if motion vectors may point outside
 *                               picture boundaries, zero otherwise
 *
 *    rcontrol                   RCONTROL (section 6.1.2 of the H.263 standard)
 *
 * Function:
 *    This function copies chrominance prediction blocks from the given
 *    source frames to the given positions of the destination frames.
 *
 * Returns:
 *    >= 0 if everything is ok
 *    < 0 if an error occured
 *
 */

static int blcCopyUVPredictionBlocks(
   blcUVCountRefXY_t uvCountSourceXY,
   blcCopyBlock_t copyBlock,
   int *mvxArray, int *mvyArray, 
   u_char *dstUBlk, u_char *dstVBlk,
   u_char *srcUFrame, u_char *srcVFrame, 
   int dstUVXPos, int dstUVYPos, 
   int dstUVXSize,
   int srcUVXSize, int srcUVYSize,
   int fMVsOverPictureBoundaries, 
   int rcontrol)
{
   int
      srcXPos,
      srcYPos,
      subpixelX,
      subpixelY,
      xlt0,
      xNormal,
      xgtmax,
      ylt0,
      yNormal,
      ygtmax;

   u_char *srcBlk;

   uvCountSourceXY(dstUVXPos, dstUVYPos, mvxArray, mvyArray, 
       &srcXPos, &srcYPos, &subpixelX, &subpixelY);

   if (fMVsOverPictureBoundaries)
      blcHandleRefOverPictBoundariesBC(
         8, 8,
         srcUVXSize, srcUVYSize, 
         subpixelX, subpixelY, 
         &srcXPos, &srcYPos,
         &xlt0, &xNormal, &xgtmax, 
         &ylt0, &yNormal, &ygtmax);

   else {
      if (blcIsRefOverPictBoundaries(
         8, 8,
         srcUVXSize, srcUVYSize, 
         subpixelX, subpixelY, 
         srcXPos, srcYPos))
         return -1;
      xlt0 = xgtmax = ylt0 = ygtmax = 0;
      xNormal = yNormal = 8;
   }

   /* U block */ \
   srcBlk = srcUFrame + srcYPos * srcUVXSize + srcXPos;

   /*deb0p("Copy U block\n");
   deb1p("srcBlk %x\n", srcBlk);
   deb1p("dstUBlk %x\n", dstUBlk);
   deb1p("xlt0 %d\n", xlt0);
   deb1p("xNormal %d\n", xNormal);
   deb1p("xgtmax %d\n", xgtmax);
   deb1p("ylt0 %d\n", ylt0);
   deb1p("yNormal %d\n", yNormal);
   deb1p("ygtmax %d\n", ygtmax);
   deb1p("subpixelX %d\n", subpixelX);
   deb1p("subpixelY %d\n", subpixelY);
   deb1p("srcUVXSize %d\n", srcUVXSize);
   deb1p("dstUVXSize %d\n", dstUVXSize);
   deb1p("rcontrol %d\n", rcontrol);*/

   copyBlock(srcBlk, dstUBlk, xlt0, xNormal, xgtmax,
      ylt0, yNormal, ygtmax, subpixelX, subpixelY,
      srcUVXSize, dstUVXSize, 8, 8, rcontrol);

   /* V block */ \
   srcBlk = srcVFrame + srcYPos * srcUVXSize + srcXPos;

   copyBlock(srcBlk, dstVBlk, xlt0, xNormal, xgtmax,
      ylt0, yNormal, ygtmax, subpixelX, subpixelY,
      srcUVXSize, dstUVXSize, 8, 8, rcontrol);

   return 0;
}


/*
 * blcHandleRefOverPictBoundariesBC
 *    
 *
 * Input parameters:
 *    xBlkSize                   the width of the block to copy
 *    yBlkSize                   the height of the block to copy
 *    xSize                      the width of the frame
 *    ySize                      the height of the frame
 *
 *    subpixelX                  1 if half pixel position is used in X direction
 *    subpixelY                  1 if half pixel position is used in Y direction
 *
 * Input/output parameters:
 *    sourceX                    input: the absolute position of the source
 *    sourceY                    block (may be negative, half-pixel values are
 *                               truncated to the next smaller integer)
 *                               output: the coordinates of the first valid
 *                               pixel in the source block
 *
 * Output parameters:
 *    The next parameters describe a row of source pixels.
 *    xlt0                       the number of the source pixels in the left of
 *                               the image area
 *    xNormal                    the number of the source pixels within 
 *                               the image area
 *    xgtmax                     the number of the source pixels in the right of
 *                               the image area
 *
 *    The next parameters describe a column of source pixels.
 *    ylt0                       the number of the source pixels above 
 *                               the image area
 *    yNormal                    the number of the source pixels withing 
 *                               the image area
 *    ygtmax                     the number of the source pixels below 
 *                               the image area
 *
 * Function:
 *    This function counts how many pixels are outside the picture area
 *    and the coordinates of the first valid pixel in the source block.
 *
 * Returns:
 *    See output parameters.
 *
 */

static void blcHandleRefOverPictBoundariesBC(
   int xBlkSize, int yBlkSize,
   int xSize, int ySize, 
   int subpixelX, int subpixelY, 
   int *sourceX, int *sourceY,
   int *xlt0, int *xNormal, int *xgtmax, 
   int *ylt0, int *yNormal, int *ygtmax)
{
   int
      xSizeMinus = xSize - xBlkSize,
      ySizeMinus = ySize - yBlkSize;

   if (*sourceX < 0) {
      *xlt0 = (-(*sourceX) < xBlkSize) ? -(*sourceX) : xBlkSize;
      *sourceX = 0;
   }
   else *xlt0 = 0;

   if (*sourceY < 0) {
      *ylt0 = (-(*sourceY) < yBlkSize) ? -(*sourceY) : yBlkSize;
      *sourceY = 0;
   }
   else *ylt0 = 0;

   if (*sourceX + subpixelX > xSizeMinus) {
      if (*sourceX + subpixelX < xSize)
         *xgtmax = *sourceX + subpixelX - xSizeMinus;
      else {
         *xgtmax = xBlkSize;
         *sourceX = xSize - 1;
      }
   }
   else *xgtmax = 0;

   if (*sourceY + subpixelY > ySizeMinus) {
      if (*sourceY + subpixelY < ySize)
         *ygtmax = *sourceY + subpixelY - ySizeMinus;
      else {
         *ygtmax = yBlkSize;
         *sourceY = ySize - 1;
      }
   }
   else *ygtmax = 0;

   *xNormal = xBlkSize - *xgtmax - *xlt0;
   *yNormal = yBlkSize - *ygtmax - *ylt0;
}


/*
 * blcIsRefOverPictBoundaries
 *    
 *
 * Input parameters:
 *    xBlkSize                   the width of the block to copy
 *    yBlkSize                   the height of the block to copy
 *    xSize                      the width of the frame
 *    ySize                      the height of the frame
 *
 *    subpixelX                  1 if half pixel position is used in X direction
 *    subpixelY                  1 if half pixel position is used in Y direction
 *
 *    sourceX                    the absolute position of the source
 *    sourceY                    block (may be negative, half-pixel values are
 *                               truncated to the next smaller integer)
 *
 * Function:
 *    This function checks if the reference (source, prediction) block
 *    is (partly) outside the picture area.
 *
 * Returns:
 *    1 if the reference block is (partly) outside the picture area
 *    0 if the whole reference block is inside the picture area
 *
 */

static int blcIsRefOverPictBoundaries(
   int xBlkSize, int yBlkSize,
   int xSize, int ySize, 
   int subpixelX, int subpixelY, 
   int sourceX, int sourceY)
{
   if (sourceX < 0)
      return 1;

   if (sourceY < 0)
      return 1;

   if (sourceX + subpixelX > xSize - xBlkSize)
      return 1;

   if (sourceY + subpixelY > ySize - yBlkSize)
      return 1;

   return 0;
}



/*
 * blcUVCountMVOffset4MVs
 *    
 *
 * Input parameters:
 *    mvxArray                   an array of the four horizontal components of
 *                               the motion vectors for a particular macroblock
 *    mvyArray                   an array of the four vertical components of
 *                               the motion vectors for a particular macroblock
 *
 * Output parameters:
 *    offsetX                    the relative position of the source
 *    offsetY                    block (the origo is the destination block, 
 *                               half-pixel values are truncated to the next 
 *                               smaller integer)
 *
 *    subpixelX                  1 if half pixel position is used in X direction
 *    subpixelY                  1 if half pixel position is used in Y direction
 *
 * Function:
 *    This function counts the relative position of the chrominance source block
 *    when it is given the motion vectors for the destination block. The macro
 *    uses interpolation of four motion vectors described in Annex F of H.263
 *    Recommendation.
 *
 * Returns:
 *    See output parameters.
 *
 *    
 *
 */

static void blcUVCountMVOffset4MVs(
   int *mvxArray, int *mvyArray,
   int *offsetX, int *offsetY,
   int *subpixelX, int *subpixelY)
{
   /* These arrays define the table 16 in H.263 recommendation which is used
      for interpolating chrominance motion vectors when four motion vectors
      per macroblock is used. */
   static const int
      halfPixelOrig16[31] =
         {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
          0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
      * const halfPixel16 = &halfPixelOrig16[15],
      fullPixelOrig16[31] =
         {-1,-1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
      * const fullPixel16 = &fullPixelOrig16[15];

   int mvSum, pixelPos;

   mvSum = mvxArray[0] + mvxArray[1] + mvxArray[2] + mvxArray[3];
   pixelPos = (mvSum % 80) / 5;
   *subpixelX = halfPixel16[pixelPos];
   *offsetX = (mvSum / 80) + fullPixel16[pixelPos];
   if (mvSum < 0 && *subpixelX)
      (*offsetX)--;

   mvSum = mvyArray[0] + mvyArray[1] + mvyArray[2] + mvyArray[3];
   pixelPos = (mvSum % 80) / 5;
   *subpixelY = halfPixel16[pixelPos];
   *offsetY = (mvSum / 80) + fullPixel16[pixelPos];
   if (mvSum < 0 && *subpixelY)
      (*offsetY)--;
}


/*
 * blcUVCountRefXY1MV
 * blcUVCountRefXY4MVs
 *    
 *
 * Input parameters:
 *    origoX                     the coordinates of the destination block
 *    origoY
 *
 *    mvxArray                   an array of the four horizontal components of
 *                               the motion vectors for a particular macroblock
 *    mvyArray                   an array of the four vertical components of
 *                               the motion vectors for a particular macroblock
 *                               In blcUVCountRefXY1MV, only the first entry of
 *                               the array is used.
 *
 * Output parameters:
 *    sourceX                    the absolute position of the source
 *    sourceY                    block (may be negative, half-pixel values are
 *                               truncated to the next smaller integer)
 *
 *    subpixelX                  1 if half pixel position is used in X direction
 *    subpixelY                  1 if half pixel position is used in Y direction
 *
 * Function:
 *    These macros count the absolute position of the chrominance source block 
 *    (and sets sourceX, sourceY, subpixelX and subpixelY according to it) 
 *    when it is given the position of the destination block and 
 *    the motion vectors for the destination block. blcUVCountRefXY4MVs
 *    uses interpolation of four motion vectors described in Annex F of H.263
 *    Recommendation. blcUVCountRefXY1MV uses only the first motion vector
 *    of the array as described in chapter 6.1.1 of H.263 Recommendation.
 *
 * Returns:
 *    See output parameters.
 *
 *    
 *
 */

static void blcUVCountRefXY1MV(
   int origoX, int origoY,
   int *mvxArray, int *mvyArray, 
   int *sourceX, int *sourceY,
   int *subpixelX, int *subpixelY)
{
   int mvxVal = *mvxArray, mvyVal = *mvyArray;

   *sourceX = origoX + mvxVal / 20;
   *sourceY = origoY + mvyVal / 20;

   *subpixelX = (mvxVal % 20) != 0;
   *subpixelY = (mvyVal % 20) != 0;

   if (mvxVal < 0 && *subpixelX)
      *sourceX -= 1;
   if (mvyVal < 0 && *subpixelY)
      *sourceY -= 1;
}


static void blcUVCountRefXY4MVs(
   int origoX, int origoY,
   int *mvxArray, int *mvyArray, 
   int *sourceX, int *sourceY,
   int *subpixelX, int *subpixelY)
{
   int offX, offY;

   blcUVCountMVOffset4MVs(mvxArray, mvyArray, &offX, &offY, subpixelX, subpixelY);
   *sourceX = origoX + offX;
   *sourceY = origoY + offY;
}


/*
 * blcYCountRefXY
 *    
 *
 * Input parameters:
 *    mvxVal                     motion vector components for the block
 *    mvyVal
 *
 *    destX                      the coordinates of the destination block
 *    destY
 *
 * Output parameters:
 *    sourceX                    the absolute position of the source
 *    sourceY                    block (may be negative, half-pixel values are
 *                               truncated to the next smaller integer)
 *
 *    subpixelX                  1 if half pixel position is used in X direction
 *    subpixelY                  1 if half pixel position is used in Y direction
 *
 * Function:
 *    This function counts the absolute position of the luminance source block (and
 *    sets sourceX, sourceY, subpixelX and subpixelY according to it) when it is
 *    given the position of the destination block and a motion vector pointing
 *    to the source block.
 *
 * Returns:
 *    See output parameters.
 *
 *    
 *
 */

static void blcYCountRefXY(
   int mvxVal, int mvyVal,
   int destX, int destY, 
   int *sourceX, int *sourceY,
   int *subpixelX, int *subpixelY)
{
   *sourceX = destX + mvxVal / 10;
   *sourceY = destY + mvyVal / 10;

   *subpixelX = mvxVal & 1;
   *subpixelY = mvyVal & 1;

   if (mvxVal < 0 && *subpixelX)
      *sourceX -= 1;
   if (mvyVal < 0 && *subpixelY)
      *sourceY -= 1;
}
// End of File
