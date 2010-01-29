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
* A header file for the draw item abstract data type.
*
*/


#ifndef _RENDRI_H_
#define _RENDRI_H_


/*
 * Includes
 */

#include "renapi.h"


/*
 * Macros
 */

/* {{-output"renDriMacros.txt"}} */
/*
 * The following macros return a specific feature of the draw item
 * passed as a parameter.
 *
 *
 * renDriNewSourceFlag
 *    returns 1, if the draw item is a new source for macroblock copying,
 *    otherwise, it returns 0,
 *    see REN_EXTDRAW_NEW_SOURCE in renExtDrawParam_t
 *
 * renDriCopyNotCodedFlag
 *    returns 1, if the draw item contains not-coded macroblock info,
 *    otherwise, it returns 0,
 *    see REN_EXTDRAW_COPY_NOT_CODED in renExtDrawParam_t
 *
 * renDriSnapshotFlag
 *    returns 1, if the draw item is marked as a snapshot,
 *    otherwise, it returns 0,
 *    see REN_EXTDRAW_SNAPSHOT in renExtDrawParam_t
 *
 * renDriFreezeReleaseFlag
 *    returns 1, if the draw item ends picture freezing,
 *    otherwise, it returns 0,
 *    see REN_EXTDRAW_SNAPSHOT in renExtDrawParam_t
 *
 * renDriNoStatusCallbackFlag
 *    returns 1, if no status callback is called after this draw item is,
 *    processed, 
 *    otherwise, it returns 0,
 *    see REN_EXTDRAW_NO_STATUS_CALLBACK in renExtDrawParam_t
 *
 * renDriRate
 *    returns the rate parameter of renExtDrawParam_t
 *
 * renDriScale
 *    returns the scale parameter of renExtDrawParam_t
 *
 * renDriCodedMBs
 *    returns a pointer to an unsigned char array of flags of coded MBs,
 *    the array is indexed by scan-ordered macroblock index,
 *    1 means that the corresponding macroblock is coded,
 *    0 means that the corresponding macroblock is not coded
 *
 * renDriNumOfMBs
 *    returns the number of macroblocks in the draw item
 *
 * renDriNumOfCodedMBs
 *    returns the number of coded macroblocks in the draw item
 *
 * renDriBitmapWidth
 *    returns the width of the bitmap to be displayed (in pixels)
 *
 * renDriBitmapHeight
 *    returns the height of the bitmap to be displayed (in pixels)
 *
 * renDriBitmapImageSize
 *    returns the size of the bitmap to be displayed (in bytes)
 *
 * renDriBitmapCompression
 *    returns the compression ID number of the draw item
 *
 * renDriFrameNumber
 *    returns the frame number of the draw item 
 *    (the lTime Parameter of renDrawParam_t)
 *
 * renDriRetFrame
 *    returns the pointer to the function where to return the processed draw
 *    item (the retFrame param of renDrawItem_t)
 *
 * renDriRetFrameParam
 *    returns the parameter to pass to the function where to return
 *    the processed draw item (the retFrameParam of renDrawItem_t)
 *
 */

#define renDriNewSourceFlag(drawItem) \
   (((drawItem)->extParam.flags & REN_EXTDRAW_NEW_SOURCE) > 0)

#define renDriCopyNotCodedFlag(drawItem) \
   (((drawItem)->extParam.flags & REN_EXTDRAW_COPY_NOT_CODED) > 0)

#define renDriRate(drawItem) \
   ((drawItem)->extParam.rate)

#define renDriCodedMBs(drawItem) \
   ((drawItem)->extParam.fCodedMBs)

#define renDriNumOfMBs(drawItem) \
   ((drawItem)->extParam.numOfMBs)

#define renDriNumOfCodedMBs(drawItem) \
   ((drawItem)->extParam.numOfCodedMBs)

#define renDriBitmapWidth(drawItem) \
   (((renBitmapInfoHeader_t *) ((drawItem)->param.lpFormat))->biWidth)

#define renDriBitmapHeight(drawItem) \
   (labs(((renBitmapInfoHeader_t *) ((drawItem)->param.lpFormat))->biHeight))

#define renDriBitmapImageSize(drawItem) \
    (((renBitmapInfoHeader_t *) ((drawItem)->param.lpFormat))->biSizeImage)

#define renDriFrameNumber(drawItem) \
   ((drawItem)->param.lTime)


/*
 * Function prototypes
 */

renDrawItem_t * renDriAlloc(int width, int height, int fYuvNeeded);

void renDriCopyParameters(
   renDrawItem_t *dstDrawItem, 
   const renDrawItem_t *srcDrawItem);

void renDriCopyFrameData(
   renDrawItem_t *dstDrawItem, 
   const renDrawItem_t *srcDrawItem);

void renDriFree(renDrawItem_t *drawItem);

int renDriYUV(renDrawItem_t *drawItem,
   u_char **y, u_char **u, u_char **v, int *width, int *height);

#endif
// End of File
