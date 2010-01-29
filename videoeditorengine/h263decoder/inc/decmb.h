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
* Header for macroblock decoding functions.
*
*/



#ifndef _DECMB_H_
#define _DECMB_H_

/*
 * Includes
 */

#include "epoclib.h"

#include "biblin.h"

#include "block.h"
#include "core.h"
#include "vdcmvc.h"


/*
 * Defines
 */

// unify error codes
#define DMB_BIT_ERR  H263D_OK_BUT_BIT_ERROR
#define DMB_OK H263D_OK
#define DMB_ERR H263D_ERROR


/*
 * Structs and typedefs
 */

/* {{-output"dmbIFrameMBInParam_t_info.txt" -ignore"*" -noCR}}
   This structure is used to pass input parameters to
   the dmbGetAndDecodeIFrameMB function.
   {{-output"dmbIFrameMBInParam_t_info.txt"}} */

/* {{-output"dmbIFrameMBInParam_t.txt"}} */
typedef struct {
   bibBuffer_t *inBuffer;        /* Bit Buffer instance data */


   bibBuffer_t *outBuffer;        /* Out Bit Buffer instance data */
   bibBufferEdit_t *bufEdit;
   int iColorEffect; 
   TBool iGetDecodedFrame; //Do we need to get decoded data.
   int StartByteIndex;
   int StartBitIndex;


   int xPosInMBs;                /* Horizontal position of the macroblock
                                    (in macroblocks), 
                                    0 .. (numMBsInMBLine - 1) */

   int yPosInMBs;                /* Vertical position of the macroblock,
                                    (in macroblocks), 0 .. */

   vdcPictureParam_t *pictParam; /* Picture attributes */

   int fGOBHeaderPresent;        /* 1 if GOB header present */

   int numMBsInSegment;          /* Number of MBs in either GOB or Slice Segment */
   int sliceStartMB;             /* MB address of starting MB of the slice */


} dmbIFrameMBInParam_t;
/* {{-output"dmbIFrameMBInParam_t.txt"}} */


/* {{-output"dmbIFrameMBInOutParam_t_info.txt" -ignore"*" -noCR}}
   This structure is used to pass input parameters to
   the dmbGetAndDecodeIFrameMB function. The function may also modify
   these parameters (or the structures pointed by the parameters).
   {{-output"dmbIFrameMBInOutParam_t_info.txt"}} */

/* {{-output"dmbIFrameMBInOutParam_t.txt"}} */
typedef struct {
   u_char *fCodedMBs;            /* Array for coded macroblock flags in
                                    scan-order, 1 = coded, 0 = not coded */

   int numOfCodedMBs;            /* Number of coded macroblocks */

/*** MPEG-4 REVISION ***/
   int currMBNum;                /* current MB number */
   int currMBNumInVP;            /* current MB in VP */
   
   aicData_t *aicData;           /* MPEG-4 Advanced Intra Coding module 
                                    instance data */
/*** End MPEG-4 REVISION ***/

   int quant;                    /* Current quantizer */

   u_char *yMBInFrame;           /* Pointer to the top-left corner of
                                    the current macroblock in a frame */
   u_char *uBlockInFrame;
   u_char *vBlockInFrame;

   int StartByteIndex;
   int StartBitIndex;

} dmbIFrameMBInOutParam_t;
/* {{-output"dmbIFrameMBInOutParam_t.txt"}} */


/* {{-output"dmbPFrameMBInParam_t_info.txt" -ignore"*" -noCR}}
   This structure is used to pass input parameters to
   the dmbGetAndDecodePFrameMB function.
   {{-output"dmbPFrameMBInParam_t_info.txt"}} */

/* {{-output"dmbPFrameMBInParam_t.txt"}} */
typedef struct {
   bibBuffer_t *inBuffer;        /* Bit Buffer instance data */

   bibBuffer_t *outBuffer;        /* Out Bit Buffer instance data */
   bibBufferEdit_t *bufEdit;
   int iColorEffect; 
   TBool iGetDecodedFrame;
   int StartByteIndex;
   int StartBitIndex;


   int xPosInMBs;                /* Horizontal position of the macroblock
                                    (in macroblocks), 
                                    0 .. (numMBsInMBLine - 1) */

   int yPosInMBs;                /* Vertical position of the macroblock,
                                    (in macroblocks), 0 .. */

   vdcPictureParam_t *pictParam; /* Picture attributes */

   int fGOBHeaderPresent;        /* Non-zero indicates that the GOB header
                                    is present in the GOB to which the current
                                    macroblock belongs */

   u_char *refY;                 /* Reference frame */
   u_char *refU;
   u_char *refV;

   u_char *currPY;               /* Current P (or I) frame */
   u_char *currPU;
   u_char *currPV;


   int numMBsInSegment;          /* Number of MBs in either GOB or Slice Segment */
   int sliceStartMB;             /* MB address of starting MB of the slice */

   int mbOnRightOfBorder;
   int mbOnOfBottomBorder;
   int mbOnLeftOfBorder;

} dmbPFrameMBInParam_t;
/* {{-output"dmbPFrameMBInParam_t.txt"}} */


/* {{-output"dmbPFrameMBInOutParam_t_info.txt" -ignore"*" -noCR}}
   This structure is used to pass input parameters to
   the dmbGetAndDecodePFrameMB function. The function may also modify
   these parameters (or the structures pointed by the parameters).
   {{-output"dmbPFrameMBInOutParam_t_info.txt"}} */

/* {{-output"dmbPFrameMBInOutParam_t.txt"}} */
typedef struct {


   bibBuffer_t *outBuffer;        /* Out Bit Buffer instance data */
   int StartByteIndex;
   int StartBitIndex;


   u_char *fCodedMBs;            /* Array for coded macroblock flags in
                                    scan-order, 1 = coded, 0 = not coded */

   int numOfCodedMBs;            /* Number of coded macroblocks */

/*** MPEG-4 REVISION ***/
   int currMBNum;                /* current MB number */
   int currMBNumInVP;            /* current MB in VP */

   aicData_t *aicData;           /* Advanced Intra Coding module instance data */
/*** End MPEG-4 REVISION ***/

   int quant;                    /* Current quantizer */

   u_char *yMBInFrame;           /* Pointer to the top-left corner of
                                    the current macroblock in a frame */
   u_char *uBlockInFrame;
   u_char *vBlockInFrame;

   mvcData_t *mvcData;           /* Motion Vector Count module instance data */

   blcDiffMB_t *diffMB;          /* Storage for the previous prediction error
                                    blocks */

} dmbPFrameMBInOutParam_t;
/* {{-output"dmbPFrameMBInOutParam_t.txt"}} */


/*
 * Function prototypes
 */

int dmbGetAndDecodeIFrameMB(
   const dmbIFrameMBInParam_t *inParam,
   dmbIFrameMBInOutParam_t *inOutParam,
   u_char fMPEG4, CMPEG4Transcoder *hTranscoder);

int dmbGetAndDecodePFrameMB(
   const dmbPFrameMBInParam_t *inParam,
   dmbPFrameMBInOutParam_t *inOutParam,
   u_char fMPEG4, CMPEG4Transcoder *hTranscoder);

#endif
// End of File
