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
* Block handling.
*
*/


#ifndef _BLOCK_H_
#define _BLOCK_H_

#include "epoclib.h"
#include "vdcmvc.h"

/*
 * Definitions
 */



/*
 * Structs and typedefs
 */

/* {{-output"blcDiffMB_t_info.txt" -ignore"*" -noCR}}
   blcDiffMB_t is used as a temporary storage for the previous difference 
   (INTER) macroblock. This storage is needed for the Advanced Prediction
   mode because the previous prediction macroblock cannot be constructed
   until the motion vectors of the current macroblock have been fetched
   from the bitstream. (The previous difference macroblock cannot be stored
   directly to the current frame either, because the difference block data
   is signed and the frame data is unsigned.)
   {{-output"blcDiffMB_t_info.txt"}} */

/* {{-output"blcDiffMB_t.txt"}} */
typedef struct {
   int block[4][64];    /* luminance blocks for P frame */
   int cbpy;            /* coded block pattern for luminance */
} blcDiffMB_t;
/* {{-output"blcDiffMB_t.txt"}} */


/* {{-output"BLC_COPY_PREDICTION_MB_PARAM_info.txt" -ignore"*" -noCR}}
   This C preprocessor definition is used to store the parameters needed
   for the blcCopyPredictionMB function (and therefore also for
   the blcCopyPredictionMBParam_t data type). These parameters may also be used
   in other modules than the Block Tools and in other data structures than
   blcCopyPredictionMBParam_t. When these parameters are introduced with this
   preprocessor definition in the other data structures, one does not need
   the "extra" reference step which would be needed if the parameters
   would have been introduced directly and only in
   the blcCopyPredictionMBParam_t structure. For example, if otherStructA
   has been typedefd as {BLC_COPY_PREDICTION_MB_PARAM} and otherStructB as
   {blcCopyPredictionMBParam_t predPar;}, to access e.g. refY one has to use
   either otherStructA.refY or otherStructB.predPar.refY.
   {{-output"BLC_COPY_PREDICTION_MB_PARAM_info.txt"}} */

/* {{-output"BLC_COPY_PREDICTION_MB_PARAM.txt"}} */
#define BLC_COPY_PREDICTION_MB_PARAM \
   u_char *refY;              /* Reference frame */ \
   u_char *refU; \
   u_char *refV; \
\
   u_char *currYMBInFrame;    /* Pointer to current macroblock in frame */ \
   u_char *currUBlkInFrame; \
   u_char *currVBlkInFrame; \
\
   int uvBlkXCoord;           /* X coord of MB in chrominance pixels */ \
   int uvBlkYCoord;           /* Y coord of MB (top-left corner) in */ \
                              /* chrominance pixels */ \
\
   int uvWidth;               /* Width of the picture in chro pixels */ \
   int uvHeight;              /* Height of the picture in chro pixels */ \
\
   mvcData_t *mvcData;        /* Motion Vector Count module instance data */ \
\
   int *mvx;                  /* Array of 4 x-components of motion vectors */ \
   int *mvy;                  /* Array of 4 y-components of motion vectors */ \
                              /* If not in 4-MVs-mode, only the first entry */ \
                              /* of the both arrays is used. */ \
\
   int mbPlace;               /* Indicates the place of the current */ \
                              /* macroblock inside the macroblock row: */ \
                              /*    -1 beginning of row */ \
                              /*    0 middle of row */ \
                              /*    1 end of row */ \
                              /* If Annex K is in use */ \
                              /*    -1 beginning of row or slice */ \
                              /*    0 middle of slice */ \
                              /*    1 end of row(if not the beginning of */ \
                              /*      the slice)  or slice */ \
                              /*    2  end of row and the beginning of slice */ \
\
   int fAdvancedPrediction;   /* Non-zero if Advanced Prediction is used */ \
\
   int fMVsOverPictureBoundaries; \
                              /* Non-zero if MVs are allowed to point */ \
                              /* outside picture boundaries */ \
\
   blcDiffMB_t *diffMB;       /* Prediction error for the previous MB */ \
\
   int rcontrol;              /* RCONTROL (section 6.1.2 of H.263) */ \
\
   int fourMVs;               /* Flag to indicate if there is four motion
                                 vectors per macroblock */
/* {{-output"BLC_COPY_PREDICTION_MB_PARAM.txt"}} */


/* {{-output"blcCopyPredictionMBParam_t_info.txt" -ignore"*" -noCR}}
   This structure is used to pass parameters into the blcCopyPredictionMB
   function.
   {{-output"blcCopyPredictionMBParam_t_info.txt"}} */

/* {{-output"blcCopyPredictionMBParam_t.txt"}} */
typedef struct {
   BLC_COPY_PREDICTION_MB_PARAM
} blcCopyPredictionMBParam_t;
/* {{-output"blcCopyPredictionMBParam_t.txt"}} */




/*
 * Function prototypes
 */

void blcAddBlock(int *block, 
								 u_char HUGE *frame_p, 
								 int xSize, 
   int mbPlace, u_char fourMVs, int *prevDiffBlock);

void blcBlockToFrame(int *block, 
										 u_char HUGE *frame_p, 
										 int xSize);

int blcCopyPredictionMB(blcCopyPredictionMBParam_t *param);



#endif

// End of file
