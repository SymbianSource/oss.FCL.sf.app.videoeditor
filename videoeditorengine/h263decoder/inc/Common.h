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
* Header file for definitions and common structures for 
* compressed domain transcoding.
*
*/


#ifndef INCLUDE_COMMON
#define INCLUDE_COMMON

/*
 * Includes
 */
# include "epoclib.h"

/*
 * Defines
 */

/* General */
#define E_SUCCESS            0
#define E_FAILURE         (-1)
#define NULL                 0

/* Data */
#define MB_SIZE 16
#define BLOCK_SIZE 8
#define LOG_BLOCK_WIDTH 3
#define BLOCK_COEFF_SIZE 64
 
/* Bit stream formating */
#define MOTION_MARKER 0x1F001
#define DC_MARKER     0x6B001
#define DC_MARKER_LENGTH 19
#define MOTION_MARKER_LENGTH 17 

/* Codes */
#define VISUAL_OBJECT_SEQUENCE_START_CODE 0x1B0
#define VISUAL_OBJECT_SEQUENCE_END_CODE   0x1B1
#define VIDEO_OBJECT_START_CODE           0x0100
#define VIDEO_OBJECT_LAYER_START_CODE     0x120
#define USER_DATA_START_CODE              0x1B2
#define GROUP_OF_VOP_START_CODE           0x1B3
#define VISUAL_OBJECT_START_CODE          0x1B5
#define VOP_START_CODE                    0x1B6
#define PROFILE_LEVEL                     0x3
#define VISUAL_OBJECT                     0x1
#define SIMPLE_OBJECT                     0x1
#define ASPECT_RATIO_INFO                 0x1
#define CHROMA_FORMAT                     0x1
#define RECTANGULAR                       0x0
#define MARKER_BIT                          1
#define SHORT_VIDEO_START_MARKER          0x20
#define SHORT_VIDEO_END_MARKER            0x3F
#define GOB_RESYNC_MARKER                 0x01

/* Quantization */
#define	MAX_SAT_VAL_SVH		127
#define	MIN_SAT_VAL_SVH		-127
#define	FIXED_PT_BITS		16

/* Variable length encoding */
#define NOT_VALID 65535
#define ESCAPE_CODE_VLC  0x03
#define ESCAPE_CODE_LENGTH_VLC 7

/* Constant multipliers */
#define TAN_PI_BY_8     27145
#define TAN_PI_BY_16    13036
#define TAN_3PI_BY_16   43789
#define COS_PI_BY_4     46340 
#define COS_PI_BY_8     60546 
#define COS_PI_BY_16    64276 
#define COS_3PI_BY_16   54490

/* Shift amount and corresponding rounding constants for DCT */
#define DCT_PRECISION            16
#define DCT_ROUND                0      /*32768*/   /* 2^(DCT_PRECISION - 1) */ 
#define DCT_KEPT_PRECISION       1
#define DCT_PRECISION_PLUS_KEPT  19      /* DCT_PRECISION + 2 + DCT_KEPT_PRECISION */
#define DCT_ROUND_PLUS_KEPT       0 /*262144*/  /* 2^(DCT_PRECISION_PLUS_KEPT - 1) */ 


/* Macros */
#define ABS(x)   ((x) >= 0   ? (x) :-(x))

/* 
 * Enumerations 
 */
enum {
	INTRA,
	INTER
};

enum {
	ONEMV, 
	FOURMV
};

enum {
	I_VOP,
	P_VOP
};

enum {
	H263,
	MPEG4
};

enum {
	OFF, 
	ON
};

enum {
	CODE_FOUND, 
	CODE_NOT_FOUND
};


/*
 * Structs and typedefs
 */
typedef unsigned char        tBool;
/* Typedef for 8 bit pixel */
typedef  u_int8   tPixel;

/* Macroblock position in yuv frame data */
typedef struct{
	tPixel *yFrame;
	u_int32 yFrameWidth;
	tPixel *uFrame;
	u_int32 uFrameWidth;
	tPixel *vFrame;
	u_int32 vFrameWidth;
} tMBPosInYUVFrame;

/* Motion vector information */
typedef struct{
    int16   mvx;
    int16   mvy;
    u_int32  SAD;
} tMotionVector;

/* Macroblock information */
typedef struct
{
    int16   MV[4][2];
    u_int32  SAD;
    int16   QuantScale;
    int16   CodedBlockPattern;
	int16   dQuant;
    int16   SkippedMB;
} tMBInfo;

/* Macroblock position */
typedef struct{
    u_int32  x;
    u_int32  y;
    int32   LeftBound;
    int32   RightBound;
    int32   TopBound;
    int32   BottomBound;
} tMBPosition;

/* Macroblock data (16x16 Y, 8x8 UV) */
typedef struct{
    int16   Data[384];
} tMacroblockData;

#endif  /* INCLUDE_COMMON */
