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


#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include <s32file.h>
#include "nrctyp32.h"
//#include "rdtsc.h"


/*
 * General defines
 */

#ifdef __TMS320C55X__
/* If this is defined, int is 16 bits */
#define INT_IS_16_BITS
#endif

// Define the debug printing
#ifdef _DEBUG
#include <e32svr.h>
#define PRINT(x) RDebug::Print x;
#else
#define PRINT(x)
#endif

/* If this is defined as 1, input bitstream is encapsulated in NAL packets */
/* and contians start code emulation prevention bytes                      */
#define ENCAPSULATED_NAL_PAYLOAD 1

/* Minimum and maximum QP value */
#define MIN_QP 0
#define MAX_QP 51

/* If this is defined, pixel clipping will use loop-up table */
#ifndef __TMS320C55X__
#define USE_CLIPBUF
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

/* This macro clips value val to the range of [min, max] */
#define  clip(min, max, val) (((val)<(min))? (min):(((val)>(max))? (max):(val)))


/*
 * Defines for assembly functions
 */

#ifdef AVC_ARM_ASSEMBLY
#define AVC_RECO_BLOCK_ASM
#define AVC_LOOP_FILTER_ASM
#define AVC_MOTION_COMP_ASM
#endif

#ifdef __TMS320C55X__
#define AVC_RECO_BLOCK_ASM
#endif


/*
 * Defines for error concealment
 */

/*#ifndef ERROR_CONCEALMENT
#define ERROR_CONCEALMENT
#endif*/

#ifndef BACKCHANNEL_INFO
#define BACKCHANNEL_INFO
#endif

/*
 * Defines for slice
 */

/* All possible slice types */
#define SLICE_MIN 0
#define SLICE_P   0 // P (P slice)
#define SLICE_B   1 // B (B slice)
#define SLICE_I   2 // I (I slice)
#define SLICE_SP  3 // SP (SP slice)
#define SLICE_SI  4 // SI (SI slice)
#define SLICE_P1  5	// P (P slice)
#define SLICE_B1  6	// B (B slice)
#define SLICE_I1  7	// I (I slice)
#define SLICE_SP1 8	// SP (SP slice)
#define SLICE_SI1 9	// SI (SI slice)
#define SLICE_MAX 9

/* Macros for testing whether slice is I slice, P slice or B slice */
#define IS_SLICE_I(x) ((x) == SLICE_I || (x) == SLICE_I1 || (x) == SLICE_SI || (x) == SLICE_SI1)
#define IS_SLICE_P(x) ((x) == SLICE_P || (x) == SLICE_P1 || (x) == SLICE_SP || (x) == SLICE_SP1)
#define IS_SLICE_B(x) ((x) == SLICE_B || (x) == SLICE_B1)


/*
 * Defines for macroblock
 */

#define MBK_SIZE        16
#define BLK_SIZE        4
#define BLK_PER_MB      (MBK_SIZE/BLK_SIZE)
#define MBK_SIZE_LOG2   4
#define BLK_SIZE_LOG2   2

/* Macroblock type */
#define MBK_INTRA  0
#define MBK_INTER  1

/* Intra macroblock sub-type */
#define MBK_INTRA_TYPE1     0
#define MBK_INTRA_TYPE2     1
#define MBK_INTRA_TYPE_PCM  2


/*
 * Defines for entropy coder
 */

/* These 2 macros are needed even if ENABLE_CABAC is not defined */
#define ENTROPY_CAVLC       0
#define ENTROPY_CABAC       1



/*
 * Global structures
 */

typedef struct _motVec_s {
  int16 x;
  int16 y;
} motVec_s;


/* Chrominance QP mapping table. Has to be static on Symbian. */
/* Chroma QP = qpChroma[Luma QP]                              */
#ifndef __SYMBIAN32__
extern const u_int8 qpChroma[52];
#else
static const u_int8 qpChroma[52] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,
   12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,
   28,29,29,30,31,32,32,33,34,34,35,35,36,36,37,37,
   37,38,38,38,39,39,39,39
}; 
#endif


#endif
