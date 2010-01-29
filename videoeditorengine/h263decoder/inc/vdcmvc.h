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
* Function prototypes for motion vector counting for H.263 decoder.
*
*/


#ifndef _VDCMVC_H_
#define _VDCMVC_H_


#include "epoclib.h"

/*
 * Defines
 */

#define MVC_XM1_UP 0
#define MVC_XM1_DOWN 1
#define MVC_YM1_LEFT 2
#define MVC_YM1_RIGHT 3
#define MVC_XP1_UP 4
#define MVC_XP1_DOWN 5

#define MVC_MB_INTER 1
#define MVC_MB_INTRA 2
#define MVC_MB_NOT_CODED 3


/*
 * Structs and typedefs
 */

/* {{-output"mvRowItem_t_info.txt" -ignore"*" -noCR}}
   mvRowItem_t is capable of storing one P frame motion vector.
   It is used internally within the MVC module.
   {{-output"mvRowItem_t_info.txt"}} */

/* {{-output"mvRowItem_t.txt"}} */
typedef struct {
   int mvx;          /* motion vector value or MVD_INTRA or MVD_NOT_CODED */
   int mvy;
   int y;            /* the y coordinate for motion vector in block units */
   int time;         /* a relative time when motion vector was counted */
   u_char fourMVs;   /* flag indicating four MBs per macroblock */
   int type;         /* MVC_MB_INTER, MVC_MB_INTRA or MVC_MB_NOT_CODED */
} mvRowItem_t;
/* {{-output"mvRowItem_t.txt"}} */


/* {{-output"mvBFBufItem_t_info.txt" -ignore"*" -noCR}}
   mvBFBufItem_t is capable of storing one B frame motion vector.
   It is used internally within the MVC module.
   {{-output"mvBFBufItem_t_info.txt"}} */

/* {{-output"mvBFBufItem_t.txt"}} */
typedef struct {
   int mvx;          /* motion vector value */
   int mvy;
   int x;            /* the x coordinate for mv in block units */
   int y;            /* the y coordinate for mv in block units */
   int time;         /* a relative time when mv was counted */
   u_char fourMVs;   /* flag indicating four MBs per macroblock */
} mvBFBufItem_t;
/* {{-output"mvBFBufItem_t.txt"}} */


/* {{-output"mvcData_t_info.txt" -ignore"*" -noCR}}
   mvcData_t is used to store the instance data for one MVC module and
   it is capable of keeping state of the motion vector decoding process
   for one encoded H.263 bitstream.
   {{-output"mvcData_t_info.txt"}} */

/* {{-output"mvcData_t.txt"}} */
typedef struct {
   /* motion vector buffers
    *
    * mvRow contains three motion vector lines: mvRow0, mvRow1 and mvRow2
    * mvRow0 is the lower block line in the previous macroblock line
    * mvRow1 is the higher block line in the current macroblock line
    * mvRow2 is the lower block line in the current macroblock line
    */
   mvRowItem_t *mvRow;
   mvRowItem_t *mvRow0;
   mvRowItem_t *mvRow1;
   mvRowItem_t *mvRow2;

   int currMaxX;     /* maxX, when mvcStart was last called */
   int currX;        /* x for current macroblock being handled */
   int currY;        /* y for current macroblock */
   int currTime;     /* time for current macroblock */
   int mvRowIndex;   /* the index of mvRow0 in mvRow array (0..2) */

   mvBFBufItem_t mvFBufArray[2][4]; /* forward motion vectors for B frame 
                                       for current and previous MB */
   mvBFBufItem_t mvBBufArray[2][4]; /* backward motion vectors for B frame
                                       for current and previous MB */
   int mvBFBufIndex; /* the index of the current MB in mvFBufArray and 
                        mvBBufArray. The index of the previous MB is generated
                        by XORing 1 to this variable. */
   int currLumWidth;
   int currLumHeight;
   int mvRangeLowX;  /* Lower boundary for horz. MV range */
   int mvRangeHighX; /* Higher boundary for horz. MV range */
   int mvRangeLowY;  /* Lower boundary for vert. MV range */
   int mvRangeHighY; /* Higher boundary for vert. MV range */
   int prevPredMode; /* Flag for prediction of forward prediction vector in IPB
                        1  => MB to the left has a FWD MV
                        0  => Otherwise   */
   int rightOfBorder;      /* There is a border on the left of the current MB */
   int downOfBorder;       /* There is a border on top of the current MB */
   int leftOfBorder;   /* There is a border on the right of the previous MB */
   int rightOfBorderPrev;  /* There is a border on the left of the previous MB */
   int downOfBorderPrev;   /* There is a border on top of the previous MB */
   int upRightMBIsStart;   /* The MB, on the previous line and next column is
                              the starting MB of the slice. If Annex K is not 
                              in use returns zero.*/
   int fSS;                /* Annex K use flag */


   int range;  /* allowed range of the MVs: low= -range; high= range-1 */
   int f_code; /* f_code_forward for P-frames */

} mvcData_t;
/* {{-output"mvcData_t.txt"}} */


/*
 * Function prototypes
 */

void mvcCalcMV(mvcData_t *mvcData, int mvdx, int mvdy,
   int *mvx, int *mvy, u_char predictorMode, u_char fourMVs,
   u_char unrestrictedMV,  u_char nonEmptyGOBHeader, int x, 
   int y, int time, int mbType, int16 *error, int fPLUSPTYPE, 
   int fUMVLimited);

void mvcSetBorders(
      mvcData_t *mvcData, int x, int y, int mba, int numMBsInMBLine, 
      int *rightOfBorder, int *downOfBorder);

void mvcCalcMPEGMV(mvcData_t *mvcData,
   int mvdx, int mvdy, int *mvx, int *mvy,
   u_char predictorMode, u_char fourMVs,
   u_char topOfVP, u_char leftOfVP, u_char fmv3_out,
   int x, int y, int time, int mbType, int16 *error);

void mvcCalcMVsForBFrame(mvcData_t *mvcData, int *mvfx, int *mvfy, 
   int *mvbx, int *mvby, int *mvx, int *mvy, int mvdx, int mvdy, 
   int trb, int trd, int numMVs, u_char unrestrictedMV, 
   int x, int y, int time);

void mvcCalcFwdMVsForImpBFrame(mvcData_t *mvcData, int *mvfx, int *mvfy,
   int *mvbx, int *mvby, int mvdx, int mvdy, int x, int y, int fUMV);

void mvcFree(mvcData_t *mvcData);

void mvcGetCurrNeighbourMVs(mvcData_t *mvcData, int *nmvx, int *nmvy,
   int16 *error);

void mvcGetPrevNeighbourMVs(mvcData_t *mvcData, int *nmvx, int *nmvy,
   int *pmvx, int *pmvy, u_char *fourMVs, int16 *error);

void mvcGetPrevMVFsAndMVBs(mvcData_t *mvcData, int *mvfx, int *mvfy, 
   int *mvbx, int *mvby, u_char *fourMVs, int16 *error);

void mvcMarkMBIntra(mvcData_t *mvcData, int x, int y, int time);

void mvcMarkMBNotCoded(mvcData_t *mvcData, int x, int y, int time);

void mvcStart(mvcData_t *mvcData, int maxX, int lumWidth, 
   int lumHeight, int16 *error);

void mvcGetCurrentMVs(mvcData_t *mvcData, int *mvx, int *mvy,
   int16 *error);

#endif
// End of File
