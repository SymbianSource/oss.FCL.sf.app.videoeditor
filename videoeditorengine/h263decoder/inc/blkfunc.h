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
* Header file for block handling functions.
*
*/



#ifndef __BLKFUNC_H
#define __BLKFUNC_H

#include "epoclib.h"

// General-purpose clip tables for signed and unsigned chars.
// Both of these tables range from -2048 to +2047.

extern const u_char unsignedCharClip[];
extern const char signedCharClip[]; // MH

/*
 *
 * blcAddBlock
 *
 * Parameters:
 *    block                block array
 *    frame_p              pointer to present frame in the place,
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
 * Error codes:
 *    None.
 *
 */

void __cdecl blcAddBlockAsm(int *block, u_char *frame_p, int xSize,
   int mbPlace, u_char fourMVs, int *prevDiffBlock);

/*
 *
 * blcMixBlocks
 *
 * Parameters:
 *    dest        Destination pointer
 *    src         Source pointer
 *    yn          Y size of the block
 *    xn          X size of the block (0-16)
 *    ydiff       X line length of the destination block
 *    sydiff      X line length of the source block
 *
 * Function:
 *    This function replaces all the pixels in the destination block with
 *    an average calculated from the corresponding source and destination
 *    block pixels.
 *
 * Returns:
 *    Nothing.
 *
 * Error codes:
 *    None.
 *
 */

void __cdecl blcMixBlocks( u_char *dest, u_char *src, int yn, int xn,
   int ydiff, int sydiff );

/*
 *
 * blcMixOverlapped
 *
 * Parameters:
 *    blkBuf      Source blocks:
 *                64b whole, 32b left, 32b right, 32b up, 32b down
 *    dest        Destination pointer
 *    ddelta      Delta for destination lines
 *
 * Function:
 *    Calculates weighted averages of several macroblock pieces - see
 *    function blcOverlappedMC in block.c for more detailed description.
 *
 * Returns:
 *    Nothing.
 *
 * Error codes:
 *    None.
 *
 */

void __cdecl blcMixOverlapped( u_char *blkBuf, u_char *dest, int ddelta );

/*
 *
 * blcInvQuant
 *
 * Parameters:
 *    block       Block pointer
 *    quant       Quantization value
 *    count       Number of values to process
 *
 * Function:
 *    Does inverse quantization for a block for idct.
 *
 * Returns:
 *    Nothing.
 *
 * Error codes:
 *    None.
 *
 */

void __cdecl blcInvQuant( int *block, int quant, int count );

#endif

// End of File
