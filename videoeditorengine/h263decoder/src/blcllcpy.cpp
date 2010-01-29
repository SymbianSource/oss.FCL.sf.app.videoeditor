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
* Block copying functions.
*
*/




#include "h263dconfig.h"


/*
 *
 * blcMemSetIncBuf
 *
 * Parameters:
 *    void HUGE *buf    destination buffer
 *    int ch            input value
 *    size_t count      the number of values to put into the buffer
 *
 * Function:
 *    This macro sets count bytes of buf into the low-order byte of ch
 *    and increases buf by count.
 *
 * Changes:
 *    buf
 *
 * Returns:
 *    Nothing.
 *
 * Error codes:
 *    None.
 *
 */

#define blcMemSetIncBuf(buf, ch, count) \
   MEMSET(buf, ch, count); \
   buf += count


/*
 *
 * blcCopyRowNBC           copy a row without border checking
 *
 * Parameters:
 *    u_char HUGE *srcMem  source row
 *    u_char HUGE *dstMem  destination row
 *
 *    int xlt0             ignored
 *    int xNormal          See blcCopyBlockBC.
 *    int xgtmax           ignored
 *
 * Function:
 *    This macro copies a row of pixels and assumes that
 *       - all pixels are within the image area
 *       - the pixels are in integer positions
 *
 *    dstMem is increased by xNormal
 *    srcMem is increased by xNormal - 1
 *
 * Changes:
 *    srcMem
 *    dstMem
 *
 * Returns:
 *    Nothing.
 *
 * Error codes:
 *    None.
 *
 */

#define blcCopyRowNBC(srcMem, dstMem, xlt0, xNormal, xgtmax, roundIncNotUsed) \
{ \
   MEMCPY(dstMem, srcMem, xNormal); \
   dstMem += xNormal; \
   srcMem += xNormal - 1; \
}


/*
 *
 * blcCopyRowBC            copy a row with border checking
 *
 * Parameters:
 *    u_char HUGE *srcMem  source row
 *    u_char HUGE *dstMem  destination row
 *
 *    int xlt0             See blcCopyBlockBC.
 *    int xNormal          See blcCopyBlockBC.
 *    int xgtmax           See blcCopyBlockBC.
 *
 * Function:
 *    This macro copies a row of pixels and assumes that
 *       - xlt0 pixels are in the left side of the image area and
 *       - xgtmax pixels are in the right side of the image area and
 *       - the pixels are in integer positions
 *
 *    dstMem is increased by xlt0 + xNormal + xgtmax
 *    srcMem is increased by xNormal - 1 if xNormal > 0
 *
 * Changes:
 *    srcMem
 *    dstMem
 *
 * Returns:
 *    Nothing.
 *
 * Error codes:
 *    None.
 *
 */

#define blcCopyRowBC(srcMem, dstMem, xlt0, xNormal, xgtmax, roundIncNotUsed) \
{ \
   if (xlt0) MEMSET(dstMem, *srcMem, xlt0); \
   dstMem += xlt0; \
   if (xNormal) blcCopyRowNBC(srcMem, dstMem, xlt0, xNormal, xgtmax, roundIncNotUsed); \
   if (xgtmax) MEMSET(dstMem, *srcMem, xgtmax); \
   dstMem += xgtmax; \
}


/*
 *
 * blcCopyRowSubXNBC
 *
 * Parameters:
 *    u_char HUGE *srcMem  source row
 *    u_char HUGE *dstMem  destination row
 *
 *    int xlt0             ignored
 *    int xNormal          See blcCopyBlockBC.
 *    int xgtmax           ignored
 *
 *    roundInc             See blcCopyBlockSub*.
 *
 * Function:
 *    This macro copies a row of pixels and assumes that
 *       - all pixels are within the image area
 *       - the pixels are in integer pixel position in vertical direction
 *       - the pixels are in half pixel position in horizontal direction
 *
 *    dstMem is increased by xNormal
 *    srcMem is increased by xNormal
 *
 * Changes:
 *    srcMem
 *    dstMem
 *    loopVar
 *
 * Returns:
 *    Nothing.
 *
 * Error codes:
 *    None.
 *
 */

#define blcCopyRowSubXNBC(srcMem, dstMem, xlt0, xNormal, xgtmax, loopVar, roundInc) \
for (loopVar = xNormal; loopVar; loopVar--, srcMem++) \
   *dstMem++ = (u_char) ((*srcMem + *(srcMem + 1) + roundInc) >> 1)


/*
 *
 * blcCopyRowSubXBC
 *
 * Parameters:
 *    u_char HUGE *srcMem  source row
 *    u_char HUGE *dstMem  destination row
 *
 *    int xlt0             See blcCopyBlockBC.
 *    int xNormal          See blcCopyBlockBC.
 *    int xgtmax           See blcCopyBlockBC.
 *
 *    roundInc             See blcCopyBlockSub*.
 *
 * Function:
 *    This macro copies a row of pixels and assumes that
 *       - xlt0 pixels are in the left side of the image area and
 *       - xgtmax pixels are in the right side of the image area and
 *       - the pixels are in integer pixel position in vertical direction
 *       - the pixels are in half pixel position in horizontal direction
 *
 *    dstMem is increased by xlt0 + xNormal + xgtmax
 *    srcMem is increased by xlt0 + xNormal + xgtmax
 *
 * Changes:
 *    srcMem
 *    dstMem
 *    loopVar
 *
 * Returns:
 *    Nothing.
 *
 * Error codes:
 *    None.
 *
 */

#define blcCopyRowSubXBC(srcMem, dstMem, xlt0, xNormal, xgtmax, loopVar, roundInc) \
{ \
   if (xlt0) \
      blcMemSetIncBuf(dstMem, *srcMem, xlt0); \
   blcCopyRowSubXNBC(srcMem, dstMem, xlt0, xNormal, xgtmax, loopVar, roundInc); \
   if (xgtmax) blcMemSetIncBuf(dstMem, *srcMem, xgtmax); \
}


/*
 *
 * blcCopyRowSubYNBC
 *
 * Parameters:
 *    u_char HUGE *srcMem  source row
 *    u_char HUGE *dstMem  destination row
 *
 *    int xlt0             ignored
 *    int xNormal          See blcCopyBlockBC.
 *    int xgtmax           ignored
 *
 *    roundInc             See blcCopyBlockSub*.
 *
 * Function:
 *    This macro copies a row of pixels and assumes that
 *       - all pixels are within the image area
 *       - the pixels are in half pixel position in vertical direction
 *       - the pixels are in integer pixel position in horizontal direction
 *
 *    dstMem is increased by xNormal
 *    srcMem is increased by xNormal - 1 if xNormal > 0
 *
 * Changes:
 *    srcMem
 *    dstMem
 *    loopVar
 *
 * Returns:
 *    Nothing.
 *
 * Error codes:
 *    None.
 *
 */

#define blcCopyRowSubYNBC(srcMem, dstMem, xlt0, xNormal, xgtmax, loopVar, \
   srcXSize, roundInc) \
{ \
   for (loopVar = xNormal; loopVar; loopVar--, srcMem++) \
      *dstMem++ = (u_char) ((*srcMem + *(srcMem + srcXSize) + roundInc) >> 1); \
   if (xNormal) srcMem--; \
}


/*
 *
 * blcCopyRowSubYBC
 *
 * Parameters:
 *    u_char HUGE *srcMem  source row
 *    u_char HUGE *dstMem  destination row
 *
 *    int xlt0             See blcCopyBlockBC.
 *    int xNormal          See blcCopyBlockBC.
 *    int xgtmax           See blcCopyBlockBC.
 *
 *    roundInc             See blcCopyBlockSub*.
 *
 * Function:
 *    This macro copies a row of pixels and assumes that
 *       - xlt0 pixels are in the left side of the image area and
 *       - xgtmax pixels are in the right side of the image area and
 *       - the pixels are in half pixel position in vertical direction
 *       - the pixels are in integer pixel position in horizontal direction
 *
 *    dstMem is increased by xlt0 + xNormal + xgtmax
 *    srcMem is increased by xNormal - 1 if xNormal > 0
 *
 * Changes:
 *    srcMem
 *    dstMem
 *    loopVar
 *
 * Returns:
 *    Nothing.
 *
 * Error codes:
 *    None.
 *
 */

#define blcCopyRowSubYBC(srcMem, dstMem, xlt0, xNormal, xgtmax, loopVar, \
   srcXSize, roundInc) \
{ \
   if (xlt0) { \
      *dstMem++ = (u_char) ((*srcMem + *(srcMem + srcXSize) + roundInc) >> 1); \
      if (xlt0 > 1) blcMemSetIncBuf(dstMem, *(dstMem - 1), xlt0 - 1); \
   } \
   blcCopyRowSubYNBC(srcMem, dstMem, xlt0, xNormal, xgtmax, loopVar, \
      srcXSize, roundInc); \
   if (xgtmax) { \
      if (xNormal) { \
         blcMemSetIncBuf(dstMem, *(dstMem - 1), xgtmax); \
      } \
      else { \
         *dstMem++ = (u_char) ((*srcMem + *(srcMem + srcXSize) + roundInc) >> 1); \
         if (xgtmax > 1) blcMemSetIncBuf(dstMem, *(dstMem - 1), xgtmax - 1); \
      } \
   } \
}


/*
 *
 * blcCopyRowSubXYNBC
 *
 * Parameters:
 *    u_char HUGE *srcMem  source row
 *    u_char HUGE *dstMem  destination row
 *
 *    int xlt0             ignored
 *    int xNormal          See blcCopyBlockBC.
 *    int xgtmax           ignored
 *
 *    roundInc             See blcCopyBlockSub*.
 *
 * Function:
 *    This macro copies a row of pixels and assumes that
 *       - all pixels are within the image area
 *       - the pixels are in half pixel position in vertical direction
 *       - the pixels are in half pixel position in horizontal direction
 *
 *    dstMem is increased by xNormal
 *    srcMem is increased by xNormal
 *
 * Changes:
 *    srcMem
 *    dstMem
 *    loopVar
 *
 * Returns:
 *    Nothing.
 *
 * Error codes:
 *    None.
 *
 */

#define blcCopyRowSubXYNBC(srcMem, dstMem, xlt0, xNormal, xgtmax, loopVar, \
   srcXSize, roundInc) \
for (loopVar = xNormal; loopVar; loopVar--, srcMem++) \
   *dstMem++ = (u_char) ((*srcMem + *(srcMem + 1) + *(srcMem + srcXSize) + \
      *(srcMem + srcXSize + 1) + roundInc) >> 2)


/*
 *
 * blcCopyOutPicXPixelSubXYRoundInc1
 * blcCopyOutPicXPixelSubXYRoundInc2
 *
 * Parameters:
 *    u_char HUGE *srcMem  source row
 *    u_char HUGE *dstMem  destination row
 *    srcXSize             number of pixels (bytes) in source row
 *
 * Function:
 *    These macros copy one pixel which lies horizontally outside the picture
 *    area assuming that
 *       - the pixels are in half pixel position in vertical direction
 *       - the pixels are in half pixel position in horizontal direction
 *
 *    If roundInc is 2, the macro takes advantage of the fact that
 *       (A + B + C + D + roundInc) / 4 = (A + A + C + C + 2) / 4 = 
 *       (A + C + 1) / 2
 *    when using the notation of section 6.1.2 of the H.263 recommendation.
 *
 *    dstMem is increased by 1
 *
 * Changes:
 *    dstMem
 *
 * Returns:
 *    Nothing.
 *
 * Error codes:
 *    None.
 *
 */

#define blcCopyOutPicXPixelSubXYRoundInc2(srcMem, dstMem, srcXSize) \
   *dstMem++ = (u_char) ((*srcMem + *(srcMem + srcXSize) + 1) >> 1)

#define blcCopyOutPicXPixelSubXYRoundInc1(srcMem, dstMem, srcXSize) \
   *dstMem++ = (u_char) ((((*srcMem) << 1) + ((*(srcMem + srcXSize)) << 1) + 1) >> 2)


/*
 *
 * blcCopyRowSubXYBC
 *
 * Parameters:
 *    u_char HUGE *srcMem  source row
 *    u_char HUGE *dstMem  destination row
 *
 *    int xlt0             ignored
 *    int xNormal          See blcCopyBlockBC.
 *    int xgtmax           ignored
 *
 *    roundInc             See blcCopyBlockSub*.
 *
 * Function:
 *    This macro copies a row of pixels and assumes that
 *       - xlt0 pixels are in the left side of the image area and
 *       - xgtmax pixels are in the right side of the image area and
 *       - the pixels are in half pixel position in vertical direction
 *       - the pixels are in half pixel position in horizontal direction
 *
 *    dstMem is increased by xNormal
 *    srcMem is increased by xNormal
 *
 * Changes:
 *    srcMem
 *    dstMem
 *    loopVar
 *
 * Returns:
 *    Nothing.
 *
 * Error codes:
 *    None.
 *
 */

#define blcCopyRowSubXYBC(srcMem, dstMem, xlt0, xNormal, xgtmax, loopVar, \
   srcXSize, roundInc) \
{ \
   if (xlt0) { \
      blcCopyOutPicXPixelSubXYRoundInc ## roundInc(srcMem, dstMem, srcXSize); \
      if (xlt0 > 1) blcMemSetIncBuf(dstMem, *(dstMem - 1), xlt0 - 1); \
   } \
   blcCopyRowSubXYNBC(srcMem, dstMem, xlt0, xNormal, xgtmax, loopVar, \
      srcXSize, roundInc); \
   if (xgtmax) { \
      blcCopyOutPicXPixelSubXYRoundInc ## roundInc(srcMem, dstMem, srcXSize); \
      if (xgtmax > 1) blcMemSetIncBuf(dstMem, *(dstMem - 1), xgtmax - 1); \
   } \
}


/*
 *
 * blcCopyOutPicRowSubXYRoundInc1
 * blcCopyOutPicRowSubXYRoundInc2
 *
 * Parameters:
 *    u_char HUGE *srcMem  source row
 *    u_char HUGE *dstMem  destination row
 *    srcXSize             number of pixels (bytes) in source row
 *
 * Function:
 *    These macros copy one row which lies vertically outside the picture
 *    area assuming that
 *       - the pixels are in half pixel position in vertical direction
 *       - the pixels are in half pixel position in horizontal direction
 *
 *    If roundInc is 2, the macro takes advantage of the fact that
 *       (A + B + C + D + roundInc) / 4 = (A + A + B + B + 2) / 4 = 
 *       (A + B + 1) / 2
 *    when using the notation of section 6.1.2 of the H.263 recommendation.
 *
 * Changes:
 *    srcMem
 *    dstMem
 *    loopVar
 *
 * Returns:
 *    Nothing.
 *
 * Error codes:
 *    None.
 *
 */

#define blcCopyOutPicRowSubXYRoundInc2(borderCheck, srcMem, dstMem, \
   xlt0, xNormal, xgtmax, loopVar, srcXSize) \
   blcCopyRowSubX ## borderCheck(srcMem, dstMem, xlt0, xNormal, xgtmax, loopVar, 1)

#define blcCopyOutPicRowSubXYRoundInc1NBC(srcMem, dstMem, xlt0, xNormal, xgtmax, loopVar, srcXSize) \
   for (loopVar = xNormal; loopVar; loopVar--, srcMem++) \
      *dstMem++ = (u_char) ((((*srcMem) << 1) + ((*(srcMem + 1)) << 1) + 1) >> 2)

#define blcCopyOutPicRowSubXYRoundInc1BC(srcMem, dstMem, xlt0, xNormal, xgtmax, loopVar, srcXSize) \
{ \
   if (xlt0) {\
      blcMemSetIncBuf(dstMem, *(srcMem), xlt0); \
   } \
   blcCopyOutPicRowSubXYRoundInc1NBC(srcMem, dstMem, xlt0, xNormal, xgtmax, loopVar, srcXSize); \
   if (xgtmax) { \
      blcMemSetIncBuf(dstMem, *(srcMem), xgtmax); \
   } \
}

#define blcCopyOutPicRowSubXYRoundInc1(borderCheck, srcMem, dstMem, \
   xlt0, xNormal, xgtmax, loopVar, srcXSize) \
   blcCopyOutPicRowSubXYRoundInc1 ## borderCheck(srcMem, dstMem, \
      xlt0, xNormal, xgtmax, loopVar, srcXSize)


/*
 *
 * Integer pixel position (x and y picture boundary checks respectively):
 *    blcCopyBlockNoSubNBCBC
 *    blcCopyBlockNoSubBCNBC
 *    blcCopyBlockNoSubNBCNBC
 *    blcCopyBlockNoSubBCBC
 *
 * Half pixel position in horizontal direction:
 *    blcCopyBlockSubXNBC
 *    blcCopyBlockSubXBC
 *
 * Half pixel position in vertical direction:
 *    blcCopyBlockSubYNBC
 *    blcCopyBlockSubYBC
 *
 * Half pixel position in horizontal and vertical direction:
 *    blcCopyBlockSubXYNBC
 *    blcCopyBlockSubXYBC
 *
 * NBC means that all pixels are within the vertical range of the image area.
 * BC means that ylt0 pixels are above the image area and ygtmax pixels are
 * below the image area.
 *
 * Parameters:
 *    Note that some parameters are not included into every macro.
 *
 *    routineName          corresponding row copying macro name
 *
 *    u_char HUGE *srcMem  source block
 *    u_char HUGE *dstMem  destination block
 *
 *    int xlt0             See blcCopyBlockBC.
 *    int xNormal          See blcCopyBlockBC.
 *    int xgtmax           See blcCopyBlockBC.
 *
 *    int ylt0             See blcCopyBlockBC.
 *    int yNormal          See blcCopyBlockBC.
 *    int ygtmax           See blcCopyBlockBC.
 *
 *    loopVar1, loopVar2   loop variables
 *
 *    srcXSize             the horizontal size of the source image
 *    dstXSize             the horizontal size of the destination image
 *    blkXSize             the horizontal size of the block to be copied,
 *                         should be equal to xlt0 + xNormal + xgtmax
 *
 *    srcInc               increment to srcMem (after row copying)
 *                         to get the next row
 *    dstInc               increment to dstMem (after row copying)
 *                         to get the next row
 *
 *    roundInc             for subpixel functions only:
 *                         for interpolation in X or Y  directions:
 *                            roundInc = 1 - RCONTROL, i.e.
 *                            1 if RCONTROL == 0,
 *                            0 if RCONTROL == 1,
 *                         for interpolation in X and Y directions:
 *                            roundInc = 2 - RCONTROL, i.e.
 *                            2 if RCONTROL == 0,
 *                            1 if RCONTROL == 1
 *
 * Function:
 *    These macros copy a block of pixels.
 *
 * Changes:
 *    srcMem
 *    dstMem
 *    loopVar1
 *    loopVar2
 *
 * Returns:
 *    Nothing.
 *
 * Error codes:
 *    None.
 *
 */


#define blcCopyBlockNoSubNBC(routineName, srcMem, dstMem, \
   xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, \
   loopVar1, loopVar2, srcXSize, dstXSize, blkXSize, srcInc, dstInc) \
for (loopVar1 = yNormal; loopVar1; loopVar1--) { \
   routineName(srcMem, dstMem, xlt0, xNormal, xgtmax, 0); \
   srcMem += srcInc; \
   dstMem += dstInc; \
}

#define blcCopyBlockNoSubNBCBC(routineName, srcMem, dstMem, \
   xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, \
   loopVar1, loopVar2, srcXSize, dstXSize, blkXSize, srcInc, dstInc) \
blcCopyBlockNoSubNBC(routineName, srcMem, dstMem, xlt0, xNormal, xgtmax, \
   ylt0, yNormal, ygtmax, loopVar1, loopVar2, srcXSize, dstXSize, blkXSize, \
   srcInc, dstInc)

#define blcCopyBlockNoSubBCNBC(routineName, srcMem, dstMem, \
   xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, \
   loopVar1, loopVar2, srcXSize, dstXSize, blkXSize, srcInc, dstInc) \
{ \
   if (ylt0) { \
      for (loopVar2 = ylt0; loopVar2; loopVar2--, dstMem += dstXSize) \
         MEMCPY(dstMem, srcMem, blkXSize); \
   } \
\
   blcCopyBlockNoSubNBC(routineName, srcMem, dstMem, \
      xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, \
      loopVar1, loopVar2, srcXSize, dstXSize, blkXSize, srcInc, dstInc); \
\
   if (ygtmax) { \
      if (yNormal) srcMem -= srcXSize; \
      for (loopVar2 = ygtmax; loopVar2; loopVar2--, dstMem += dstXSize) \
         MEMCPY(dstMem, srcMem, blkXSize); \
   } \
}

#define blcCopyBlockNoSubNBCNBC(routineName, srcMem, dstMem, \
   xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, \
   loopVar1, loopVar2, srcXSize, dstXSize, blkXSize, srcInc, dstInc) \
for (loopVar1 = yNormal; loopVar1; loopVar1--) { \
   MEMCPY(dstMem, srcMem, xNormal); \
   dstMem += dstXSize; \
   srcMem += srcXSize; \
}
   
#define blcCopyBlockNoSubBCBC(routineName, srcMem, dstMem, \
   xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, \
   loopVar1, loopVar2, srcXSize, dstXSize, blkXSize, srcInc, dstInc) \
{ \
   if (ylt0) { \
      routineName(srcMem, dstMem, xlt0, xNormal, xgtmax, 0); \
      dstMem += dstInc; \
      for (loopVar2 = ylt0 - 1; loopVar2; loopVar2--, dstMem += dstXSize) \
         MEMCPY(dstMem, dstMem - dstXSize, blkXSize); \
      if (xNormal) srcMem -= (xNormal - 1); \
   } \
\
   blcCopyBlockNoSubNBC(routineName, srcMem, dstMem, \
      xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, \
      loopVar1, loopVar2, srcXSize, dstXSize, blkXSize, srcInc, dstInc); \
\
   if (ygtmax) { \
      if (yNormal) srcMem -= srcXSize; \
      routineName(srcMem, dstMem, xlt0, xNormal, xgtmax, 0); \
      dstMem += dstInc; \
      for (loopVar2 = ygtmax - 1; loopVar2; loopVar2--, dstMem += dstXSize) \
         MEMCPY(dstMem, dstMem - dstXSize, blkXSize); \
   } \
}

#define blcCopyBlockSubXNBC(routineName, srcMem, dstMem, \
   xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, \
   loopVar1, loopVar2, dstXSize, blkXSize, srcInc, dstInc, roundInc) \
for (loopVar1 = yNormal; loopVar1; loopVar1--) { \
   routineName(srcMem, dstMem, xlt0, xNormal, xgtmax, loopVar2, roundInc); \
   srcMem += srcInc; \
   dstMem += dstInc; \
}

#define blcCopyBlockSubXBC(routineName, srcMem, dstMem, \
   xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, \
   loopVar1, loopVar2, dstXSize, blkXSize, srcInc, dstInc, roundInc) \
{ \
   if (ylt0) { \
      routineName(srcMem, dstMem, xlt0, xNormal, xgtmax, loopVar2, roundInc); \
      dstMem += dstInc; \
      for (loopVar2 = ylt0 - 1; loopVar2; loopVar2--, dstMem += dstXSize) \
         MEMCPY(dstMem, dstMem - dstXSize, blkXSize); \
      srcMem -= xNormal; \
   } \
\
   blcCopyBlockSubXNBC(routineName, srcMem, dstMem, xlt0, xNormal, xgtmax, \
      ylt0, yNormal, ygtmax, loopVar1, loopVar2, dstXSize, blkXSize, \
      srcInc, dstInc, roundInc); \
\
   if (ygtmax) { \
      if (!yNormal) { \
         routineName(srcMem, dstMem, xlt0, xNormal, xgtmax, loopVar2, roundInc); \
         dstMem += dstInc; \
      } \
      for (loopVar2 = ygtmax - !yNormal; loopVar2; loopVar2--, \
         dstMem += dstXSize) \
         MEMCPY(dstMem, dstMem - dstXSize, blkXSize); \
   } \
}

#define blcCopyBlockSubYNBC(routineName, yoffRoutine, srcMem, dstMem, \
   xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, \
   loopVar1, loopVar2, srcXSize, dstXSize, blkXSize, srcInc, dstInc, roundInc) \
for (loopVar1 = yNormal; loopVar1; loopVar1--) { \
   routineName(srcMem, dstMem, xlt0, xNormal, xgtmax, loopVar2, srcXSize, roundInc); \
   srcMem += srcInc; \
   dstMem += dstInc; \
} \

#define blcCopyBlockSubYBC(normRoutine, yoffRoutine, srcMem, dstMem, \
   xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, \
   loopVar1, loopVar2, srcXSize, dstXSize, blkXSize, srcInc, dstInc, roundInc) \
{ \
   if (ylt0) { \
      yoffRoutine(srcMem, dstMem, xlt0, xNormal, xgtmax, roundInc); \
      dstMem += dstInc; \
      for (loopVar2 = ylt0 - 1; loopVar2; loopVar2--, dstMem += dstXSize) \
         MEMCPY(dstMem, dstMem - dstXSize, blkXSize); \
      if (xNormal) srcMem -= (xNormal - 1); \
   } \
\
   blcCopyBlockSubYNBC(normRoutine, yoffRoutine, srcMem, dstMem, \
      xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, \
      loopVar1, loopVar2, srcXSize, dstXSize, blkXSize, srcInc, dstInc, roundInc); \
\
   if (ygtmax) { \
      yoffRoutine(srcMem, dstMem, xlt0, xNormal, xgtmax, roundInc); \
      dstMem += dstInc; \
      for (loopVar2 = ygtmax - 1; loopVar2; loopVar2--, dstMem += dstXSize) \
         MEMCPY(dstMem, dstMem - dstXSize, blkXSize); \
   } \
}

#define blcCopyBlockSubXYNBC(routineName, notUsed, srcMem, dstMem, \
   xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, \
   loopVar1, loopVar2, srcXSize, dstXSize, blkXSize, srcInc, dstInc, roundInc) \
for (loopVar1 = yNormal; loopVar1; loopVar1--) { \
   routineName(srcMem, dstMem, xlt0, xNormal, xgtmax, loopVar2, \
      srcXSize, roundInc); \
   srcMem += srcInc; \
   dstMem += dstInc; \
} \

#define blcCopyBlockSubXYBC(normRoutine, xoffBorderCheck, srcMem, dstMem, \
   xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, \
   loopVar1, loopVar2, srcXSize, dstXSize, blkXSize, srcInc, dstInc, roundInc) \
{ \
   if (ylt0) { \
      blcCopyOutPicRowSubXYRoundInc ## roundInc(xoffBorderCheck, srcMem, dstMem, \
         xlt0, xNormal, xgtmax, loopVar2, srcXSize); \
      dstMem += dstInc; \
      for (loopVar2 = ylt0 - 1; loopVar2; loopVar2--, dstMem += dstXSize) \
         MEMCPY(dstMem, dstMem - dstXSize, blkXSize); \
      srcMem -= xNormal; \
   } \
\
   blcCopyBlockSubXYNBC(normRoutine, yoffRoutine, srcMem, dstMem, \
      xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, \
      loopVar1, loopVar2, srcXSize, dstXSize, blkXSize, srcInc, dstInc, roundInc); \
\
   if (ygtmax) { \
      blcCopyOutPicRowSubXYRoundInc ## roundInc(xoffBorderCheck, srcMem, dstMem, \
         xlt0, xNormal, xgtmax, loopVar2, srcXSize); \
      dstMem += dstInc; \
      for (loopVar2 = ygtmax - 1; loopVar2; loopVar2--, dstMem += dstXSize) \
         MEMCPY(dstMem, dstMem - dstXSize, blkXSize); \
   } \
}


/*
 *
 * blcCopyBlockBCVar
 *
 * Parameters:
 *    xoffBorderCheck      NBC if all the pixels of the block are within
 *                         the horizontal range of the image area
 *                         BC otherwise
 *    yoffBorderCheck      NBC if all the pixels of the block are within
 *                         the vertical range of the image area
 *                         BC otherwise
 *
 *    u_char HUGE *srcMem  source block
 *    u_char HUGE *dstMem  destination block
 *
 *    int xlt0             See blcCopyBlockBC.
 *    int xNormal          See blcCopyBlockBC.
 *    int xgtmax           See blcCopyBlockBC.
 *    int ylt0             See blcCopyBlockBC.
 *    int yNormal          See blcCopyBlockBC.
 *    int ygtmax           See blcCopyBlockBC.
 *    int subpixelX        See blcCopyBlockBC.
 *    int subpixelY        See blcCopyBlockBC.
 *
 *    loopVar1, loopVar2   loop variables
 *
 *    srcXSize             the horizontal size of the source image
 *    dstXSize             the horizontal size of the destination image
 *
 *    blkXSize             the horizontal size of the block to be copied,
 *                         should be equal to xlt0 + xNormal + xgtmax
 *    blkYSize             the vertical size of the block to be copied,
 *                         should be equal to ylt0 + yNormal + ygtmax
 *
 *    dstInc               increment to dstMem (after row copying)
 *                         to get the next row
 *
 *    rcontrol             See blcCopyBlockBC.
 *
 * Function:
 *    This macro copies a block with appropriate image border crossing
 *    checkings.
 *
 * Changes:
 *    srcMem
 *    dstMem
 *    loopVar1
 *    loopVar2
 *
 * Returns:
 *    Nothing.
 *
 * Error codes:
 *    None.
 *
 */

#define blcCopyBlockBCVar(xoffBorderCheck, yoffBorderCheck, srcMem, dstMem, \
   xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, subpixelX, subpixelY, \
   loopVar1, loopVar2, srcXSize, dstXSize, blkXSize, blkYSize, dstInc, \
   rcontrol) \
{ \
   if (subpixelX) { \
      if (subpixelY) { \
         int srcInc = srcXSize - xNormal; \
         if (rcontrol) { \
            blcCopyBlockSubXY ## yoffBorderCheck( \
               blcCopyRowSubXY ## xoffBorderCheck, \
               xoffBorderCheck, srcMem, \
               dstMem, xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, \
               loopVar1, loopVar2, srcXSize, dstXSize, blkXSize, srcInc, dstInc, 1); \
         } \
         else { \
            blcCopyBlockSubXY ## yoffBorderCheck( \
               blcCopyRowSubXY ## xoffBorderCheck, \
               xoffBorderCheck, srcMem, \
               dstMem, xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, \
               loopVar1, loopVar2, srcXSize, dstXSize, blkXSize, srcInc, dstInc, 2); \
         } \
      } \
      else /* !subPixelY */ { \
         int srcInc = srcXSize - xNormal; \
         if (rcontrol) { \
            blcCopyBlockSubX ## yoffBorderCheck( \
               blcCopyRowSubX ## xoffBorderCheck, srcMem, dstMem, \
               xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, \
               loopVar1, loopVar2, dstXSize, blkXSize, srcInc, dstInc, 0) \
         } \
         else { \
            blcCopyBlockSubX ## yoffBorderCheck( \
               blcCopyRowSubX ## xoffBorderCheck, srcMem, dstMem, \
               xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, \
               loopVar1, loopVar2, dstXSize, blkXSize, srcInc, dstInc, 1) \
         } \
      } \
   } \
\
   else /* !subpixelX */ { \
      if (subpixelY) { \
         int srcInc = srcXSize - xNormal + ((xNormal) ? 1 : 0); \
         if (rcontrol) { \
            blcCopyBlockSubY ## yoffBorderCheck( \
               blcCopyRowSubY ## xoffBorderCheck, \
               blcCopyRow ## yoffBorderCheck, srcMem, dstMem, \
               xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, \
               loopVar1, loopVar2, srcXSize, dstXSize, blkXSize, srcInc, dstInc, 0); \
         } \
         else { \
            blcCopyBlockSubY ## yoffBorderCheck( \
               blcCopyRowSubY ## xoffBorderCheck, \
               blcCopyRow ## yoffBorderCheck, srcMem, dstMem, \
               xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, \
               loopVar1, loopVar2, srcXSize, dstXSize, blkXSize, srcInc, dstInc, 1); \
         } \
      } \
      else /* !subpixelY */ { \
         int srcInc = srcXSize - xNormal + ((xNormal) ? 1 : 0); \
         blcCopyBlockNoSub ## yoffBorderCheck ## xoffBorderCheck ( \
            blcCopyRow ## xoffBorderCheck, srcMem, dstMem, \
            xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, \
            loopVar1, loopVar2, srcXSize, dstXSize, blkXSize, srcInc, dstInc); \
      } \
   } \
}


/*
 *
 * blcCopyBlockBC
 * blcCopyBlockNBC
 *
 * Parameters:
 *    u_char HUGE *srcMem  source block
 *    u_char HUGE *dstMem  destination block
 *
 *    The next parameters describe a row of source pixels.
 *    int xlt0             the number of the source pixels in the left of
 *                         the image area
 *    int xNormal          the number of the source pixels within the image
 *                         area
 *    int xgtmax           the number of the source pixels in the right of
 *                         the image area
 *
 *    The next parameters describe a column of source pixels.
 *    int ylt0             the number of the source pixels above the image area
 *    int yNormal          the number of the source pixels withing the image
 *                         area
 *    int ygtmax           the number of the source pixels below the image area
 *
 *    int subpixelX        1 if source pixels are in half pixel position in
 *                         horizontal direction, 0 otherwise
 *    int subpixelY        1 if source pixels are in half pixel position in
 *                         vertical direction, 0 otherwise
 *
 *    srcXSize             the horizontal size of the source image
 *    dstXSize             the horizontal size of the destination image
 *
 *    blkXSize             the horizontal size of the block to be copied,
 *                         should be equal to xlt0 + xNormal + xgtmax
 *    blkYSize             the vertical size of the block to be copied,
 *                         should be equal to ylt0 + yNormal + ygtmax
 *    rcontrol             0 or 1, RCONTROL as in section 6.1.2. of H.263 
 *                         version 2
 *
 * Function:
 *    blcCopyBlockBC macro copies a block according to unrestricted motion
 *    vector mode, i.e. it checks if the motion vector points outside the
 *    image area, i.e. xlt0, xgtmax, ylt0 or ygtmax != 0.
 *    blcCopyBlockNBC macro copies a block assuming that the whole block is
 *    inside the image area. blcCopyBlockNBC is used in normal prediction
 *    mode (no options are used).
 *
 * Changes:
 *    srcMem
 *    dstMem
 *
 * Returns:
 *    Nothing.
 *
 * Error codes:
 *    None.
 *
 */

/*#define BLC_COPY_BLOCK_MACROS*/
#ifdef BLC_COPY_BLOCK_MACROS
#define blcCopyBlockBC(srcMem, dstMem, \
   xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, subpixelX, subpixelY, \
   srcXSize, dstXSize, blkXSize, blkYSize, rcontrol) \
{ \
   int \
      dstInc = dstXSize - blkXSize, \
      i, j; \
\
   if (xNormal == blkXSize) { \
      if (yNormal == blkYSize) { \
         blcCopyBlockBCVar(NBC, NBC, srcMem, dstMem, \
            xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, subpixelX, subpixelY, \
            j, i, srcXSize, dstXSize, blkXSize, blkYSize, dstInc, rcontrol); \
      } \
      else { \
         blcCopyBlockBCVar(NBC, BC, srcMem, dstMem, \
            xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, subpixelX, subpixelY, \
            j, i, srcXSize, dstXSize, blkXSize, blkYSize, dstInc, rcontrol); \
      } \
   } \
   else { \
      if (yNormal == blkYSize) { \
         blcCopyBlockBCVar(BC, NBC, srcMem, dstMem, \
            xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, subpixelX, subpixelY, \
            j, i, srcXSize, dstXSize, blkXSize, blkYSize, dstInc, rcontrol); \
      } \
      else { \
         blcCopyBlockBCVar(BC, BC, srcMem, dstMem, \
            xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, subpixelX, subpixelY, \
            j, i, srcXSize, dstXSize, blkXSize, blkYSize, dstInc, rcontrol); \
      } \
   } \
}

#define blcCopyBlockNBC(srcMem, dstMem, \
   xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, subpixelX, subpixelY, \
   srcXSize, dstXSize, blkXSize, blkYSize, rcontrol) \
{ \
   int \
      dstInc = dstXSize - blkXSize, \
      i, j; \
\
   blcCopyBlockBCVar(NBC, NBC, srcMem, dstMem, \
      xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, subpixelX, subpixelY, \
      j, i, srcXSize, dstXSize, blkXSize, blkYSize, dstInc, rcontrol); \
}

#else
static void blcCopyBlockBC(u_char HUGE *srcMem, u_char HUGE *dstMem,
   int xlt0, int xNormal, int xgtmax, int ylt0, int yNormal, int ygtmax,
   int subpixelX, int subpixelY,
   int srcXSize, int dstXSize, int blkXSize, int blkYSize, int rcontrol)
{
   int
      dstInc = dstXSize - blkXSize,
      i, j;

   if (xNormal == blkXSize) {
      if (yNormal == blkYSize) {
         blcCopyBlockBCVar(NBC, NBC, srcMem, dstMem,
            xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, subpixelX, subpixelY,
            j, i, srcXSize, dstXSize, blkXSize, blkYSize, dstInc, rcontrol);
      }
      else {
         blcCopyBlockBCVar(NBC, BC, srcMem, dstMem,
            xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, subpixelX, subpixelY,
            j, i, srcXSize, dstXSize, blkXSize, blkYSize, dstInc, rcontrol);
      }
   }
   else {
      if (yNormal == blkYSize) {
         blcCopyBlockBCVar(BC, NBC, srcMem, dstMem,
            xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, subpixelX, subpixelY,
            j, i, srcXSize, dstXSize, blkXSize, blkYSize, dstInc, rcontrol);
      }
      else {
         blcCopyBlockBCVar(BC, BC, srcMem, dstMem,
            xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, subpixelX, subpixelY,
            j, i, srcXSize, dstXSize, blkXSize, blkYSize, dstInc, rcontrol);
      }
   }
}

static void blcCopyBlockNBC(u_char HUGE *srcMem, u_char HUGE *dstMem,
   int xlt0, int xNormal, int xgtmax, int ylt0, int yNormal, int ygtmax,
   int subpixelX, int subpixelY,
   int srcXSize, int dstXSize, int blkXSize, int blkYSize, int rcontrol)
{
   int
      dstInc = dstXSize - blkXSize,
      i, j;

   blcCopyBlockBCVar(NBC, NBC, srcMem, dstMem,
      xlt0, xNormal, xgtmax, ylt0, yNormal, ygtmax, subpixelX, subpixelY,
      j, i, srcXSize, dstXSize, blkXSize, blkYSize, dstInc, rcontrol);
}
#endif


// End of file
