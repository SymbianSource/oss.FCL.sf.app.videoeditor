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
* Motion vector calculations and storage.
*
*/




#include "h263dConfig.h"

#include "vdcmvc.h"

#include "core.h"
#include "errcodes.h"
#include "debug.h"


/* 
 * Defines 
 */

#define NEIGHBOUR_SIZE 2      /* The size for neighbour array */
#define MVD_NOT_CODED -32767  /* Macroblock is not coded in mvcTable_t */
#define MVD_INTRA -32765      /* Marks intra macroblock in mvcTable_t */

#define MVD_UMIN_MPEG4 -10240

#define MVD_MIN -160          /* The minimum for mvd. */
#define MVD_MAX 155           /* The maximum for mvd. */
#define MVD_UMIN_V1 -315      /* The minimum for MV in unrestricted mode (v1) */
#define MVD_UMAX_V1 315       /* The maximum for MV in unrestricted mode (v1) */
#define MVD_UMIN_V2 -32000    /* The minimum for MV in unrestricted mode (v2)
                                 Note: this value is used to check the validity
                                 of MVs. If there is no MV for a MB 
                                 MVD_NOT_CODED or MVD_INTRA are used, and
                                 therefore any value bigger than those
                                 suits for this definition. In H.263 v2 
                                 unlimited submode, MVs can have any length
                                 within a picture (plus 15-pixel border),
                                 and therefore this definition is set to small
                                 enough value. */
#define MVD_155 155           /* MV for 15.5 */
#define MVD_160 160           /* MV for 16 */
#define MVD_320 320           /* MV for 32 */
/* It is assumed that motion vector values are signed and in order
   (for example MV for 15.5 < MV for 16).
   No other assumptions are made. */


/* 
 * Macros 
 */

/* mvValid checks if the specified item is a valid motion vector */
/* Note: MVD_NOT_CODED and MVD_INTRA are very small negative numbers.
         Thus, we can use the minimum MV for H.263 Annex D unlimited
         submode to check the validity of a passed item. Actually,
         we could use any value larger than MVD_NOT_CODED and MVD_INTRA
         and smaller than or equal to the minimum MV. */
#define mvValid(d1item, d1y, d1time) \
   (d1item.mvx >= MVD_UMIN_V2 \
      && d1item.y == (d1y) && d1item.time == (d1time))


#define mvValidMPEG(d1item, d1y, d1time) \
   (d1item.mvx >= MVD_UMIN_MPEG4 \
      && d1item.y == (d1y) && d1item.time == (d1time))

/* mvLegal cheks if the specified item has been updated to either contain
   a motion vector or not coded / intra information */
#define mvLegal(d5item, d5y, d5time) \
   (d5item.y == (d5y) && d5item.time == (d5time))

/* mvStore puts given arguments into the motion vector array
   Used from: mvcCalcMV */
#define mvStore(d2row, d2x, d2y, d2mvx, d2mvy, d2time, d2type) \
   d2row[d2x].mvx = d2mvx; d2row[d2x].mvy = d2mvy; \
   d2row[d2x].y = d2y; d2row[d2x].time = d2time; d2row[d2x].fourMVs = 1; \
   d2row[d2x].type = d2type

/* mvStoreMB puts given arguments into the motion vector array for the
   whole macroblock
   Used from: mvcCalcMV, mvcMarkMBIntra, mvcMarkMBNotCoded */
#define mvStoreMB(d4row1, d4row2, d4x, d4y, d4mvx, d4mvy, d4time, d4type) \
   d4row1[d4x].mvx = d4row1[d4x + 1].mvx = \
      d4row2[d4x].mvx = d4row2[d4x + 1].mvx = d4mvx; \
   d4row1[d4x].mvy = d4row1[d4x + 1].mvy = \
      d4row2[d4x].mvy = d4row2[d4x + 1].mvy = d4mvy; \
   d4row1[d4x].y = d4row1[d4x + 1].y = d4y; \
   d4row2[d4x].y = d4row2[d4x + 1].y = d4y + 1; \
   d4row1[d4x].time = d4row1[d4x + 1].time = \
      d4row2[d4x].time = d4row2[d4x + 1].time = time; \
   d4row1[d4x].fourMVs = d4row1[d4x + 1].fourMVs = \
      d4row2[d4x].fourMVs = d4row2[d4x + 1].fourMVs = 0; \
   d4row1[d4x].type = d4row1[d4x + 1].type = \
      d4row2[d4x].type = d4row2[d4x + 1].type = (d4type)

/* updateRowPointers checks if currX, currY or currTime have changed
   and updates them. It also updates mvRow0, mvRow1 and mvRow2.
   Used from: mvcCalcMV, mvcMarkMBIntra, mvcMarkMBNotCoded */
#define updateRowPointers(d3x, d3y, d3time) \
   if (d3x != mvcData->currX) \
      mvcData->currX = d3x; \
   if (d3y != mvcData->currY) { \
      int \
         mvRowIndex, \
         currMaxX = mvcData->currMaxX; \
      mvRowItem_t *mvRow = mvcData->mvRow; \
      mvcData->currY = d3y; \
      mvcData->mvRowIndex = mvRowIndex = (mvcData->mvRowIndex + 2) % 3; \
      mvcData->mvRow0 = &mvRow[(currMaxX + 1) * 2 * mvRowIndex]; \
      mvcData->mvRow1 = &mvRow[(currMaxX + 1) * 2 * ((mvRowIndex + 1) % 3)]; \
      mvcData->mvRow2 = &mvRow[(currMaxX + 1) * 2 * ((mvRowIndex + 2) % 3)]; \
   } \
   if (d3time != mvcData->currTime) \
      mvcData->currTime = d3time 

/* mvcInvMVD takes the other one of the possible MVD values defined in
   H.263 Recommendation */
#define mvcInvMVD(invMv) \
   (invMv < 0) ? \
      MVD_320 + invMv : \
      -MVD_320 + invMv

/* sign123(x) returns
      1 if x < 0,
      2 if x > 0,
      3 if x == 0 
   In other words,
      bit 0 = 1 if x <= 0, 
      bit 1 = 1 if x >= 0 */
#define sign123(x) ((x == 0) ? 3 : (x < 0) ? 1 : 2)


/* 
 * Local prototypes
 */

static void mvcGetNeighbourMVs(mvcData_t *mvcData, int x, int y, int time,
   int *nmvx, int *nmvy, int prevFlag, int16 *error);

__inline static void mvcCheckAndSet( 
      mvRowItem_t *mvRowPtr, int xind, int yind, int time, int nind, 
      int *nmvx, int *nmvy, mvRowItem_t *cmvRowPtr, int cxind, int cyind,
      int16 *error);

__inline static void mvcSetToCurrent( 
      mvRowItem_t *mvRowPtr, int xind, int yind, int time, int nind,
      int *nmvx, int *nmvy, int16 *error);


/*
 * Global functions
 */


/* {{-output"mvcSetBorders.txt"}} */
/*
 *
 * mvcSetBorders
 *
 * Parameters:
 *    mvcData        mvcData_t structure
 *    x              the x coordinate of the MB (0 .. maxX)
 *    y              the y coordinate of the MB
 *                   (0 .. macroblock rows in frame - 1)
 *    mba            Macroblock address of starting MB of the slice. This 
 *                   value SHOULD be set to -1 if Annex K is not in use.
 *    numMBsInMBLine    Number of MBs in MB line    
 *    rightOfBorder     There is a border on the left of the current MB
 *    downOfBorder      There is a border on top of the current MB
 *
 * Function:
 *    This function calculates the borders of GOBs and Slices..This function 
 *    SHOULD be called before mvcCalcMV.....
 *
 * Returns:
 *    Nothing
 *
 * Error codes:
 *    None
 *
 */

void mvcSetBorders(
      mvcData_t *mvcData, int x, int y, int mba, int numMBsInMBLine, 
      int *rightOfBorder, int *downOfBorder 
)
{
   int
      upRightMBIsStart;

   if (mvcData)   {
      mvcData->rightOfBorderPrev = mvcData->rightOfBorder;
      mvcData->downOfBorderPrev = mvcData->downOfBorder;
   }

   if (mba == -1) {
      if (x == 0) {
         *rightOfBorder = 1;
      }
      else  {
         *rightOfBorder = 0;
      }
      
      /* Always downOfBorder is set to 1, since in QCIF and CIF every GOB
         has only 1 MB line. If larger images are supported, numMBLinesInGOB
         should be passed here. Also number of starting MB of the GOB may be
         needed...*/
      *downOfBorder = 1;
      upRightMBIsStart = 0;
   }
   else  {
      int
         xs,   /* x coordinate of start MB */
         ys;   /* y coordinate of start MB */
      xs = mba % numMBsInMBLine;
      ys = mba / numMBsInMBLine;

      /* slice structured mode*/
      /* Check if there is slice border on top of the  current MB:
         For that, check either current MB should be on the same MB line 
         with the starting MB of slice or current MB should not be on the
         left of the starting MB if the MB is on the second line of the 
         slice. */
      
      if ((y == ys) || ((x < xs) && (ys+1 == y)))  {
         *downOfBorder = 1;
      }
      else  {
         *downOfBorder = 0;
      }   
      /* Check if there is a slice border on the left of the current MB.*/
      if (((x == xs) && (y == ys)) || (x == 0))  {
         *rightOfBorder = 1;
      }
      else  {
         *rightOfBorder = 0;
      }

      /* Test the situation of MV2 is in another slice but MV3 in in the
         same slice... i.e:
         MB1 MB1 MB1 MB2 MB2 .........MB2
         MB2 MB2 MB2 MB2 MB2..........MB2
                  *                        
         The MB marked with * is an exmaple. MB on top of it is in another slice
         but the MB which MV3 will come from is in the same slice...*/
      if ((x+1 == xs) && (y-1 == ys))
         upRightMBIsStart = 1;
      else
         upRightMBIsStart = 0;
   }   

   if (mvcData)   {
      mvcData->upRightMBIsStart = upRightMBIsStart;
      mvcData->downOfBorder = *downOfBorder;
      mvcData->rightOfBorder = *rightOfBorder;
      mvcData->leftOfBorder = 0;
   }
}

/* {{-output"mvcCalcMV.txt"}} */
/*
 *
 * mvcCalcMV
 *
 * Parameters:
 *       mvcData        mvcData_t structure
 *
 *       mvdx           motion vector data returned by vlcGetMVD
 *       mvdy
 *
 *       *mvx           the resulting motion vector is placed to *mvx and *mvy
 *       *mvy
 *
 *       predictorMode  0, 1, 2 or 3. See H.263 recommendation, Annex F, Figure
 *                      15, where modes are in the following order:
 *                         0  1
 *                         2  3
 *
 *                      Advanced Prediction mode: It is assumed that this
 *                      function is called by setting predictorMode 0, 1, 2
 *                      and 3 in this order for a particular macroblock.
 *
 *                      Normal: This function should be called by setting
 *                      predictorMode to 0.
 *
 *       fourMVs        if != 0, four motion vectors for current macroblock
 *                      is assumed
 *
 *       unrestrictedMV       if != 0, unrestricted motion vector mode is used
 *
 *       topOfGOB             if != 0, the current MB is assumed to be in the
 *                            first line of the current GOB
 *
 *       nonEmptyGOBHeader    if != 0, the GOB header of the current GOB is
 *                            is assumed to be non-empty, and MV2 and MV3 are
 *                            set to MV1 if they are outside the GOB. If Annex K
 *                            is in use set this value to 1......
 *
 *       x              the x coordinate of the MB (0 .. maxX)
 *       y              the y coordinate of the MB
 *                      (0 .. macroblock rows in frame - 1)
 *
 *       time           a value which is related to the time when the current
 *                      frame must be shown. This value should be unique
 *                      among a relatively small group of consecutive frames.
 *
 *       mbType         must be MVC_MB_INTER or MVC_MB_INTRA. MVC_MB_INTRA 
 *                      should be used only in PB frames mode.
 *
 *       error          error code
 *
 *       fPLUSPTYPE     PLUSTYPE flag
 *
 *       fUMVLimited    Annex D v2 flag
 *                      1  => Limited MVs    (UUI = 1)
 *                      0  => Unlimited MVs  (UUI = 01)
 *
 * Function:
 *       This function counts the motion vector value for given macroblock.
 *       See H.263 Recommendation 6.1.1 Differential Motion Vectors and
 *       Annex F. Advanced Prediction Mode.
 *
 * Returns:
 *       Changes *mvx, *mvy and possibly *error.
 *
 * Error codes:
 *       ERR_MVC_MVDX_ILLEGAL    mvdx illegal
 *       ERR_MVC_MVDY_ILLEGAL    mvdy illegal
 *       ERR_MVC_MVX_ILLEGAL     mvx is NULL
 *       ERR_MVC_MVY_ILLEGAL     mvy is NULL
 *       ERR_MVC_MODE_ILLEGAL    predictorMode illegal
 *       ERR_MVC_X_ILLEGAL       x < 0 or x > maxX
 *       ERR_MVC_Y_ILLEGAL       y < 0
 *       ERR_MVC_TIME_ILLEGAL    time < 0
 *       ERR_MVC_MVPTR           result is not legal
 *
 */

void mvcCalcMV(mvcData_t *mvcData, int mvdx, int mvdy,
   int *mvx, int *mvy, u_char predictorMode, u_char fourMVs,
   u_char unrestrictedMV,  u_char nonEmptyGOBHeader, int x, 
   int y, int time, int mbType, int16 *error, int fPLUSPTYPE, 
   int fUMVLimited)
/* {{-output"mvcCalcMV.txt"}} */
{

   int 
      i, j,    /* loop variables */
      mvcx[3], /* Candidates: mv[0] = MV1, mv[1] = MV2, mv[2] = MV3 */
      mvcy[3],
      *mv,     /* mvcx or mvcy */
      *mvptr,  /* == mvx or mvy */
      mvd,     /* mvdx or mvdy */
      xx2,     /* x * 2 */
      yx2,     /* y * 2 */
      xmv1,    /* index for MV1 */
      xmv2,    /* index for MV2 */
      xmv3,    /* index for MV3 */
      ymv1,
      ymv2,
      ymv3,
      mvRangeLow,    /* Lower boundary for MV Range  
                        Look H.263+ Rec. Table D.1, D.2 */
      mvRangeHigh,   /* Higher boundary for MV Range  
                        Look H.263+ Rec. Table D.1, D.2 */
      picDim,        /* Temporary variable for luminance width or height */
      mbCorner,      /* Temporary variable for MB's topleft corner's 
                        coordinate (x or y)*/
      mvRes;         /* MB's rightmost or bottommost pixel plus MV
                        for checking restriction D.1.1 */
   mvRowItem_t
      *currMvRow,
      *prevMvRow,
      *mvRow0,
      *mvRow1,
      *mvRow2;

   /* Check parameters */
    if ((!fPLUSPTYPE) || (!unrestrictedMV))
   {
      if (mvdx < MVD_MIN || mvdx > MVD_MAX) {
         *error = ERR_MVC_MVDX_ILLEGAL;
         return;
      }

      if (mvdy < MVD_MIN || mvdy > MVD_MAX) {
         *error = ERR_MVC_MVDY_ILLEGAL;
         return;
      }
   }

   if (!mvx) {
      *error = ERR_MVC_MVX_ILLEGAL;
      return;
   }

   if (!mvy) {
      *error = ERR_MVC_MVY_ILLEGAL;
      return;
   }

   if ((fourMVs && predictorMode > 3) ||
      (!fourMVs && predictorMode > 0)) {
      *error = ERR_MVC_MODE_ILLEGAL;
      return;
   }

   if (x < 0 || x > mvcData->currMaxX) {
      *error = ERR_MVC_X_ILLEGAL;
      return;
   }

   if (y < 0) {
      *error = ERR_MVC_Y_ILLEGAL;
      return;
   }

   if (time < 0) {
      *error = ERR_MVC_TIME_ILLEGAL;
      return;
   }

   updateRowPointers(x, y, time);
   mvRow0 = mvcData->mvRow0;
   mvRow1 = mvcData->mvRow1;
   mvRow2 = mvcData->mvRow2;

   /* Seek candidate predictors */

   xx2 = x << 1; /* xx2 = x * 2 */
   yx2 = y << 1;

   xmv1 = xx2 - 1 + (predictorMode & 1);
   xmv2 = xx2 + (predictorMode == 1);
   xmv3 = xx2 + 2 - (predictorMode > 1);
   ymv1 = yx2 + (predictorMode > 1);
   ymv2 = ymv3 = yx2 - 1 + (predictorMode > 1);

   if (predictorMode <= 1) {
      currMvRow = mvRow1;
      prevMvRow = mvRow0;
   }
   else {
      currMvRow = mvRow2;
      prevMvRow = mvRow1;
   }

   if ( (predictorMode & 1) || /* right column of the MB */
      ( !mvcData->rightOfBorder && mvValid(currMvRow[xmv1], ymv1, time) ) ) {
      mvcx[0] = currMvRow[xmv1].mvx;
      mvcy[0] = currMvRow[xmv1].mvy;
   }
   else
      /* if (x == 0 || motion vector for previous MB does not exist ||
             the previous MB belongs to a different slice) */
      mvcx[0] = mvcy[0] = 0;

   if ((y == 0 || (mvcData->downOfBorder && nonEmptyGOBHeader)) && predictorMode <= 1) {
      /* mv2 and mv3 are outside the picture or GOB
         (if non-empty GOB header) or outside the slice*/
      mvcx[1] = mvcx[0];
      mvcy[1] = mvcy[0];
      /* Check if the MB ont the previous line and on the next column
         in in the same slice (means start MBof the slice).. */
      if (!mvcData->upRightMBIsStart)   {
         mvcx[2] = mvcx[0];
         mvcy[2] = mvcy[0];
      }
      if (x == mvcData->currMaxX)
         mvcx[2] = mvcy[2] = 0;
   }
   else {
      if (mvValid(prevMvRow[xmv2], ymv2, time)) {
         mvcx[1] = prevMvRow[xmv2].mvx;
         mvcy[1] = prevMvRow[xmv2].mvy;
      }
      else 
         mvcx[1] = mvcy[1] = 0;

      if (mvValid(prevMvRow[xmv3], ymv3, time)) {
         mvcx[2] = prevMvRow[xmv3].mvx;
         mvcy[2] = prevMvRow[xmv3].mvy;
      }
      else
         mvcx[2] = mvcy[2] = 0;
   }

    for (j = 0, mv = mvcx, mvd = mvdx, mvptr = mvx,
         picDim = mvcData->currLumWidth, mbCorner = x,
         mvRangeLow = mvcData->mvRangeLowX,
         mvRangeHigh = mvcData->mvRangeHighX;
         j < 2;
         j++, mv = mvcy, mvd = mvdy, mvptr = mvy, 
         picDim = mvcData->currLumHeight, mbCorner = y,
         mvRangeLow = mvcData->mvRangeLowY,
         mvRangeHigh = mvcData->mvRangeHighY) {

      int choice1, min, predictor;

      /* Find the median of the candidates */

      min = mv[0];
      predictor = 32767;

      for (i = 1; i < 3; i++) {
         if (mv[i] <= min) {
            predictor = min;
            min = mv[i];
            continue;
         }
         if (mv[i] < predictor)
            predictor = mv[i];
      }

      /* Count the new motion vector value */

      if (!unrestrictedMV) {
         /* Default prediction mode.
            Count the legal [-16..15.5] motion vector */

         choice1 = predictor + mvd;

         *mvptr = (MVD_MIN <= choice1 && choice1 <= MVD_MAX) ?
            choice1 :
            /*(mvd == 0) ?
               predictor :*/ /* index 32, should never happen?! */
               (mvd < 0) ?
                  MVD_320 + choice1 : /* index 0..31: 32 + mvd + predictor */
                  -MVD_320 + choice1; /* index 33..63: -32 + mvd + predictor */

         /* Check that the result is [-16..15.5] */
         if (*mvptr < MVD_MIN || *mvptr > MVD_MAX) {
            *error = ERR_MVC_MVPTR;
            return;
         }
      }

      else if ((-MVD_155 <= predictor && predictor <= MVD_160) && (!fPLUSPTYPE))
         /* Unrestricted motion vector mode && -15.5 <= predictor <= 16.
            mvd is always valid. */
         *mvptr = predictor + mvd;

       else if (!fPLUSPTYPE)
       {
         /* Unrestricted motion vector mode && predictor not in [-15.5, 16]
            Result in [-31.5, 31.5] and has the same sign as predictor */
         choice1 = predictor + mvd;
         *mvptr = (MVD_UMIN_V1 <= choice1 && choice1 <= MVD_UMAX_V1 &&
            (sign123(choice1) & sign123(predictor))) ?
            choice1 :
            /*(mvd == 0) ?
               predictor :*/ /* index 32, should never happen!? */
               (mvd < 0) ?
                  MVD_320 + choice1 : /* index 0..31: 32 + mvd + predictor */
                  -MVD_320 + choice1; /* index 33..63: -32 + mvd + predictor */

         /* Check that the result is in the appropriate range. */
         if ((predictor < -MVD_155 && *mvptr > 0) ||
            (predictor > MVD_160 && *mvptr < 0)) {
            *error = ERR_MVC_MVPTR;
            return;
         }

      }
       else if (fUMVLimited)  
      /* fPlusType==1 && UUI == 1*/
      {
         *mvptr = predictor + mvd;

         if ( *mvptr > 0 )
            /* mvRes is the leftmost/topmost pixels coordinate of next
               macroblock plus 2 times MV */
              mvRes = (mbCorner+1)*32 + (*mvptr)/5 ;
           else
             /* mvRes is the leftmost/topmost pixels coordinate of current
               macroblock plus 2 times MV,
            half a pixel corresponds to 1 in integer scale */
               mvRes = mbCorner*32 + (*mvptr)/5 ;

         if ((mvRes < -30) || (mvRes > picDim * 2 + 30))
         /* Restriction D.1.1 */
         {
            *error = ERR_MVC_MVPTR;
            deb("Restriction D.1.1\n");
            return;
         }
      
         if ((*mvptr < mvRangeLow) || (*mvptr > mvRangeHigh))
         /* Restriction D.2 */
         {
            *error = ERR_MVC_MVPTR;
            deb("Restriction D.2 \n");
            return;
         }
      }
      else  /* fPlusType==1 && UUI == 01 */
      {
         *mvptr = predictor + mvd;
         if ( *mvptr > 0 )
               mvRes = (mbCorner+1)*32 + (*mvptr)/5;
           else
               mvRes = mbCorner*32 + (*mvptr)/5;
         if ((mvRes < -30) || (mvRes > picDim * 2 + 30))
         /* Restriction D.1.1 */
         {
            *error = ERR_MVC_MVPTR;
            deb("Restriction D.1.1\n");
            return;
         }
      }
   }

   /* Update motion vector buffer arrays */
   switch (predictorMode) {
      case 0:
         if (fourMVs) {
            mvStore(mvRow1, xx2, yx2, *mvx, *mvy, time, mbType);
         }
         else {
            mvStoreMB(mvRow1, mvRow2, xx2, yx2, *mvx, *mvy, time, mbType);
         }
         break;
      case 1:
         mvStore(mvRow1, xx2 + 1, yx2, *mvx, *mvy, time, mbType);
         break;
      case 2:
         mvStore(mvRow2, xx2, yx2 + 1, *mvx, *mvy, time, mbType);
         break;
      case 3:
         mvStore(mvRow2, xx2 + 1, yx2 + 1, *mvx, *mvy, time, mbType);
         break;
   }
}


/* {{-output"mvcCalcMPEGMV.txt"}} */
/*
 *
 * mvcCalcMPEGMV
 *
 * Parameters:
 *       mvcData        mvcData_t structure
 *
 *       mvdx           motion vector data returned by vlcGetMVD
 *       mvdy
 *
 *       *mvx           the resulting motion vector is placed to *mvx and *mvy
 *       *mvy
 *
 *       predictorMode  0, 1, 2 or 3. See H.263 recommendation, Annex F, Figure
 *                      15, where modes are in the following order:
 *                         0  1
 *                         2  3
 *
 *                      Advanced Prediction mode: It is assumed that this
 *                      function is called by setting predictorMode 0, 1, 2
 *                      and 3 in this order for a particular macroblock.
 *
 *                      Normal: This function should be called by setting
 *                      predictorMode to 0.
 *
 *       fourMVs        if != 0, four motion vectors for current macroblock
 *                      is assumed
 *
 *       topOfVP        if != 0, the current MB is assumed to be in the
 *                      first line of the current VideoPacket
 *
 *       leftOfVP       if != 0, the current MB is assumed to be the
 *                      first in the VideoPacket
 *
 *       fmv3_out       if != 0, the 3rd MB (top, right) used as MV predictor
 *                      is outside of the current VideoPacket
 *
 *       x              the x coordinate of the MB (0 .. maxX)
 *       y              the y coordinate of the MB
 *                      (0 .. macroblock rows in frame - 1)
 *
 *       time           a value which is related to the time when the current
 *                      frame must be shown. This value should be unique
 *                      among a relatively small group of consecutive frames.
 *
 *       mbType         must be MVC_MB_INTER or MVC_MB_INTRA. MVC_MB_INTRA 
 *                      should be used only in PB frames mode.
 *
 *       error          error code
 *
 * Function:
 *       This function counts the motion vector value for given macroblock.
 *       See H.263 Recommendation 6.1.1 Differential Motion Vectors and
 *       Annex F. Advanced Prediction Mode.
 *
 * Returns:
 *       Changes *mvx, *mvy and possibly *error.
 *
 * Error codes:
 *       The following codes are assertion-like checks:
 *       ERR_MVC_MVDX_ILLEGAL    mvdx illegal
 *       ERR_MVC_MVDY_ILLEGAL    mvdy illegal
 *       ERR_MVC_MVX_ILLEGAL     mvx is NULL
 *       ERR_MVC_MVY_ILLEGAL     mvy is NULL
 *       ERR_MVC_MODE_ILLEGAL    predictorMode illegal
 *       ERR_MVC_X_ILLEGAL       x < 0 or x > maxX
 *       ERR_MVC_Y_ILLEGAL       y < 0
 *       ERR_MVC_TIME_ILLEGAL    time < 0
 *
 *       The following code may also be caused by a bit error:
 *       ERR_MVC_MVPTR           result is not legal
 *
 */

void mvcCalcMPEGMV(mvcData_t *mvcData,
   int mvdx, int mvdy, int *mvx, int *mvy,
   u_char predictorMode, u_char fourMVs,
   u_char topOfVP, u_char leftOfVP, u_char fmv3_out,
   int x, int y, int time, int mbType, int16 *error)
/* {{-output"mvccalc.txt"}} */
{
   int 
      i, j,    /* loop variables */
      mvcx[3], /* Candidates: mv[0] = MV1, mv[1] = MV2, mv[2] = MV3 */
      mvcy[3],
      mvc1_out = 0, mvc2_out = 0, mvc3_out = 0,
      *mv,     /* mvcx or mvcy */
      *mvptr,  /* == mvx or mvy */
      mvd,     /* mvdx or mvdy */
      xx2,     /* x * 2 */
      yx2,     /* y * 2 */
      xmv1,    /* index for MV1 */
      xmv2,    /* index for MV2 */
      xmv3,    /* index for MV3 */
      ymv1,
      ymv2,
      ymv3;
   mvRowItem_t
      *currMvRow,
      *prevMvRow,
      *mvRow0,
      *mvRow1,
      *mvRow2;


   if (!mvx) {
      *error = ERR_MVC_MVX_ILLEGAL;
      return;
   }

   if (!mvy) {
      *error = ERR_MVC_MVY_ILLEGAL;
      return;
   }

   if ((fourMVs && predictorMode > 3) ||
      (!fourMVs && predictorMode > 0)) {
      *error = ERR_MVC_MODE_ILLEGAL;
      return;
   }

   if (x < 0 || x > mvcData->currMaxX) {
      *error = ERR_MVC_X_ILLEGAL;
      return;
   }

   if (y < 0) {
      *error = ERR_MVC_Y_ILLEGAL;
      return;
   }

   if (time < 0) {
      *error = ERR_MVC_TIME_ILLEGAL;
      return;
   }

   updateRowPointers(x, y, time);
   mvRow0 = mvcData->mvRow0;
   mvRow1 = mvcData->mvRow1;
   mvRow2 = mvcData->mvRow2;

   /* Seek candidate predictors */

   xx2 = x << 1; /* xx2 = x * 2 */
   yx2 = y << 1;

   xmv1 = xx2 - 1 + (predictorMode & 1);
   xmv2 = xx2 + (predictorMode == 1);
   xmv3 = xx2 + 2 - (predictorMode > 1);
   ymv1 = yx2 + (predictorMode > 1);
   ymv2 = ymv3 = yx2 - 1 + (predictorMode > 1);

   if (predictorMode <= 1) {
      currMvRow = mvRow1;
      prevMvRow = mvRow0;
   }
   else {
      currMvRow = mvRow2;
      prevMvRow = mvRow1;
   }

   if ((x == 0 || leftOfVP) && !(predictorMode & 1)) {
      /* mv1 is outside the VP or VOP */
      mvcx[0] = mvcy[0] = 0;
      mvc1_out = 1;
   }
   else {
      if (mvValidMPEG(currMvRow[xmv1], ymv1, time)) {
          mvcx[0] = currMvRow[xmv1].mvx;
          mvcy[0] = currMvRow[xmv1].mvy;
      }
      else mvcx[0] = mvcy[0] = 0;
   }

   if ((y == 0 || topOfVP) && predictorMode <= 1) {
      /* mv2 is outside the VP or VOP */
      mvcx[1] = mvcy[1] = 0;
      mvc2_out = 1;
   }
   else {
      if (mvValidMPEG(prevMvRow[xmv2], ymv2, time)) {
         mvcx[1] = prevMvRow[xmv2].mvx;
         mvcy[1] = prevMvRow[xmv2].mvy;
      }
      else mvcx[1] = mvcy[1] = 0;
   }

   if ((y == 0 || fmv3_out || x == mvcData->currMaxX) && predictorMode <= 1) {
      /* mv3 is outside the VP or VOP */
      mvcx[2] = mvcy[2] = 0;
      mvc3_out = 1;
   }
   else {
      if (mvValidMPEG(prevMvRow[xmv3], ymv3, time)) {
         mvcx[2] = prevMvRow[xmv3].mvx;
         mvcy[2] = prevMvRow[xmv3].mvy;
      }
      else
         mvcx[2] = mvcy[2] = 0;
   }

   for (j = 0, mv = mvcx, mvd = mvdx, mvptr = mvx;
      j < 2;
      j++, mv = mvcy, mvd = mvdy, mvptr = mvy) {

      int min, predictor;

      switch (mvc1_out + mvc2_out + mvc3_out)
      {
         case 3:
             predictor = 0;
             break;
         case 2:
             predictor = mv[0] + mv[1] + mv[2];
             break;
         case 1:
         case 0:
             /* Find the median of the candidates */
             min = mv[0];
             predictor = 32767;

             for (i = 1; i < 3; i++) {
                 if (mv[i] <= min) {
                     predictor = min;
                     min = mv[i];
                     continue;
                 }
                 if (mv[i] < predictor)
                     predictor = mv[i];
             }
             break;
         default:
            /* Should never happen */
            vdcAssert(0);
            predictor = 0;
            break;
      }

      /* Count the new motion vector value */
      *mvptr = predictor + mvd;

      if (*mvptr < -(mvcData->range))
        *mvptr += 2*(mvcData->range);
      else if (*mvptr > (mvcData->range)-5)
        *mvptr -= 2*(mvcData->range);
   }

   /* Update motion vector buffer arrays */
   switch (predictorMode) {
      case 0:
         if (fourMVs) {
            mvStore(mvRow1, xx2, yx2, *mvx, *mvy, time, mbType);
         }
         else {
            mvStoreMB(mvRow1, mvRow2, xx2, yx2, *mvx, *mvy, time, mbType);
         }
         break;
      case 1:
         mvStore(mvRow1, xx2 + 1, yx2, *mvx, *mvy, time, mbType);
         break;
      case 2:
         mvStore(mvRow2, xx2, yx2 + 1, *mvx, *mvy, time, mbType);
         break;
      case 3:
         mvStore(mvRow2, xx2 + 1, yx2 + 1, *mvx, *mvy, time, mbType);
         break;
   }
}

/* {{-output"mvcFree.txt"}} */
/*
 *
 * mvcFree
 *
 * Parameters:
 *    mvcData        mvcData_t structure
 *
 * Function:
 *    This function frees the dynamic memory allocated by mvcStart.
 *    mvcFree should be called at least when exiting the main program.
 *    Alternatively it can be called whenever the playing a video has
 *    ended.
 *
 * Returns:
 *    Nothing
 *
 * Error codes:
 *    None
 *
 *    
 *    
 *    
 */

void mvcFree(mvcData_t *mvcData)
/* {{-output"mvcFree.txt"}} */
{
   if (mvcData) {
      if (mvcData->mvRow) free(mvcData->mvRow);
      mvcData->currMaxX = -1;
   }
}


/* {{-output"mvcGetCurrentMVs.txt"}} */
/*
 *
 * mvcGetCurrentMVs
 *
 * Parameters:
 *       mvcData  mvcData_t structure
 *       mvx
 *       mvy
 *       error    error code
 *
 * Function:
 *       These functions return the motion vectors of the current 
 *       blocks.
 *
 * Returns:
 *       Changes mvx, mvy and possibly error.
 *
 * Error codes:
 *
 *    ERR_MVC_CURR_NOT_VALID        if the motion vectors for the current
 *                                  macroblock do not exist
 *
 *    ERR_MVC_CURR_NOT_CODED        if the current macroblock was not coded
 *
 *    ERR_MVC_CURR_INTRA            if the current macroblock was coded in
 *                                  INTRA mode
 *
 */
void mvcGetCurrentMVs(mvcData_t *mvcData, int *mvx, int *mvy,
   int16 *error)
{
   int
      xx2 = mvcData->currX << 1,
      yx2 = mvcData->currY << 1;
   mvRowItem_t
      *mvRow1 = mvcData->mvRow1,
      *mvRow2 = mvcData->mvRow2;
   if (mvValid(mvRow1[xx2], yx2, mvcData->currTime) &&
         mvRow1[xx2].type == MVC_MB_INTER) {
      mvx[0] = mvRow1[xx2].mvx;
      mvx[1] = mvRow1[xx2 + 1].mvx;
      mvx[2] = mvRow2[xx2].mvx;
      mvx[3] = mvRow2[xx2 + 1].mvx;
      mvy[0] = mvRow1[xx2].mvy;
      mvy[1] = mvRow1[xx2 + 1].mvy;
      mvy[2] = mvRow2[xx2].mvy;
      mvy[3] = mvRow2[xx2 + 1].mvy;
   }
   else  {
      mvx[0] = mvx[1] = mvx[2] = mvx[3] =
      mvy[0] = mvy[1] = mvy[2] = mvy[3] = 0;
      if mvLegal(mvRow1[xx2], yx2, mvcData->currTime) {
         if (mvRow1[xx2].type == MVC_MB_NOT_CODED) {
            /*deb("mvcGetCurrentMVs: ERROR - macroblock not coded.\n");*/
            *error = ERR_MVC_CURR_NOT_CODED;
         }
         else if (mvRow1[xx2].type == MVC_MB_INTRA) {
            /*deb("mvcGetCurrentMVs: ERROR - INTRA macroblock.\n");*/
            *error = ERR_MVC_CURR_INTRA;
         }
         else {
            /*deb("mvcGetCurrentMVs: ERROR - macroblock not valid.\n");*/
            *error = ERR_MVC_CURR_NOT_VALID;
         }
      }
      else {
         /*deb("mvcGetCurrentMVs: ERROR - macroblock not valid.\n");*/
         *error = ERR_MVC_CURR_NOT_VALID;
      }
   }
}

/* {{-output"mvcGetNeighbourMVs.txt"}} */
/*
 *
 * mvcGetCurrNeighbourMVs
 * mvcGetPrevNeighbourMVs
 *
 * Parameters:
 *       mvcData  mvcData_t structure
 *
 *       nmvx     pointer to motion vector x component array
 *       nmvy     pointer to motion vector y component array
 *                The size of the array must be 6. The indexing of the
 *                neighboring motion vectors goes like this:
 *                (X = the motion vector which is investigated, i.e.
 *                the last one decoded (mvcGetCurrNeighbourMVs) or
 *                the one before the last one (mvcGetPrevNeighbourMVs))
 *                     2 3
 *                   0 X X 4
 *                   1 X X 5
 *                It is suggested that the following defines are used
 *                instead of values:
 *                   MVC_XM1_UP (0)
 *                   MVC_XM1_DOWN (1)
 *                   MVC_YM1_LEFT (2)
 *                   MVC_YM1_RIGHT (3)
 *                   MVC_XP1_UP (4)
 *                   MVC_XP1_DOWN (5)
 *                (M stand for minus and P for plus)
 *
 *       pmvx and pmvy are for mvcGetPrevNeighbourMVs only:
 *       pmvx     pointer to the motion vector x component array of the
 *                previous macroblock
 *       pmvy     pointer to the motion vector y component array of the
 *                previous macroblock
 *                The size of the array must be 4. The indexing is similar
 *                to normal block indexing inside a macroblock:
 *                     0 1
 *                     2 3
 *
 *       error    error code
 *
 * Function:
 *       These functions return the motion vectors of the neighboring
 *       blocks (on the left side, above or on the right side) of the
 *       macroblock which is investigated, i.e.
 *          - the last one decoded (mvcGetCurrNeighbourMVs) or
 *          - the one before the last one (mvcGetPrevNeighbourMVs))
 *
 *       The H.263 standard for Overlapped motion compensation for luminance
 *       (Annex F.3) is followed, that is:
 *       If one of the surrounding macroblocks was not coded, the
 *       corresponding remote motion vector is set to zero. If one of the
 *       surrounding (macro)blocks was coded in INTRA mode, the corresponding
 *       remote motion vector is replaced by the motion vector for the
 *       current block expect when in PB-frames mode. In this case (INTRA
 *       block in PB-frame mode), the INTRA block's motion vector is used.
 *       If the current block is at the border of the picture and therefore
 *       a surrounding block is not present, the corresponding remote motion
 *       vector is replaced by the current motion vector.
 *
 *       mvcGetPrevNeighbourMVs also returns the motion vector for the
 *       previous macroblock.
 *
 * Returns:
 *       Changes nmvx, nmvy, pmvx, pmvy and possibly error.
 *
 * Error codes:
 *    ERR_MVC_NO_PREV_MB            if the current MB has x coordinate 0 and
 *                                  mvcGetPrevNeighbourMVs is called
 *
 *    ERR_MVC_NEIGHBOUR_NOT_VALID   if one of the neighboring MVs is not
 *                                  valid, i.e. it has not been updated
 *                                  for this frame. The MV in nmvx and nmvy
 *                                  is set to zero.
 *
 *    for mvcGetPrevNeighbourMVs only:
 *    ERR_MVC_PREV_NOT_VALID        if the motion vectors for the previous
 *                                  macroblock do not exist
 *
 *    ERR_MVC_PREV_NOT_CODED        if the previous macroblock was not coded
 *
 *    ERR_MVC_PREV_INTRA            if the previous macroblock was coded in
 *                                  INTRA mode
 *
 *    
 *    
 *    
 *    
 *    
 *    
 */

void mvcGetCurrNeighbourMVs(mvcData_t *mvcData, int *nmvx, int *nmvy,
   int16 *error)
/* {{-output"mvcGetNeighbourMVs.txt"}} */
{
   mvRowItem_t
      *mvRow1 = mvcData->mvRow1;
   int xx2 = mvcData->currX << 1;

   mvcGetNeighbourMVs(mvcData, mvcData->currX, mvcData->currY,
      mvcData->currTime, nmvx, nmvy, 0, error);
   if (*error)   {
      if mvLegal(mvRow1[xx2], mvcData->currY << 1, mvcData->currTime) {
         if (mvRow1[xx2].type == MVC_MB_NOT_CODED) {
            /*deb("mvcGetPrevNeighbourMVs: ERROR - macroblock not coded.\n");*/
            *error = ERR_MVC_CURR_NOT_CODED;
         }
         else if (mvRow1[xx2].type == MVC_MB_INTRA) {
            /*deb("mvcGetPrevNeighbourMVs: ERROR - INTRA macroblock.\n");*/
            *error = ERR_MVC_CURR_INTRA;
         }
         else {
            /*deb("mvcGetPrevNeighbourMVs: ERROR - macroblock not valid.\n");*/
            *error = ERR_MVC_CURR_NOT_VALID;
         }
      }
      else {
         /*deb("mvcGetPrevNeighbourMVs: ERROR - macroblock not valid.\n");*/
         *error = ERR_MVC_CURR_NOT_VALID;
      }
   }
}

/* {{-output"mvcGetPrevNeighbourMVs.txt"}} */
void mvcGetPrevNeighbourMVs(mvcData_t *mvcData, int *nmvx, int *nmvy,
   int *pmvx, int *pmvy, u_char *fourMVs, int16 *error)
/* {{-output"mvcGetPrevNeighbourMVs.txt"}} */
{
   int
      currX = mvcData->currX,
      currY = mvcData->currY;
   mvRowItem_t
      *mvRow1 = mvcData->mvRow1,
      *mvRow2 = mvcData->mvRow2;

   if (currX > 0) {
      int xx2 = (currX - 1) << 1;
      mvcGetNeighbourMVs(mvcData, currX - 1, currY,
         mvcData->currTime, nmvx, nmvy, 1, error);
      if (mvValid(mvRow1[xx2], currY << 1, mvcData->currTime) &&
         mvRow1[xx2].type == MVC_MB_INTER) {
         pmvx[0] = mvRow1[xx2].mvx;
         pmvx[1] = mvRow1[xx2 + 1].mvx;
         pmvx[2] = mvRow2[xx2].mvx;
         pmvx[3] = mvRow2[xx2 + 1].mvx;
         pmvy[0] = mvRow1[xx2].mvy;
         pmvy[1] = mvRow1[xx2 + 1].mvy;
         pmvy[2] = mvRow2[xx2].mvy;
         pmvy[3] = mvRow2[xx2 + 1].mvy;
         *fourMVs = mvRow1[xx2].fourMVs;
      }
      else {
         pmvx[0] = pmvx[1] = pmvx[2] = pmvx[3] =
         pmvy[0] = pmvy[1] = pmvy[2] = pmvy[3] = 0;
         *fourMVs = 0;
         if mvLegal(mvRow1[xx2], currY << 1, mvcData->currTime) {
            if (mvRow1[xx2].type == MVC_MB_NOT_CODED) {
               /*deb("mvcGetPrevNeighbourMVs: ERROR - macroblock not coded.\n");*/
               *error = ERR_MVC_PREV_NOT_CODED;
            }
            else if (mvRow1[xx2].type == MVC_MB_INTRA) {
               /*deb("mvcGetPrevNeighbourMVs: ERROR - INTRA macroblock.\n");*/
               *error = ERR_MVC_PREV_INTRA;
            }
            else {
               /*deb("mvcGetPrevNeighbourMVs: ERROR - macroblock not valid.\n");*/
               *error = ERR_MVC_PREV_NOT_VALID;
            }
         }
         else {
            /*deb("mvcGetPrevNeighbourMVs: ERROR - macroblock not valid.\n");*/
            *error = ERR_MVC_PREV_NOT_VALID;
         }
      }
   }
   else {
      /*deb("mvcGetPrevNeighbourMVs: ERROR - no previous macroblock.\n");*/
      *error = ERR_MVC_NO_PREV_MB;
   }
}


/* {{-output"mvcGetPrevMVFsAndMVBs.txt"}} */
/*
 *
 * mvcGetPrevMVFsAndMVBs
 *
 * Parameters:
 *    mvcData           mvcData_t structure
 *
 *    mvfx              the resulting forward motion vector
 *    mvfy
 *
 *    mvbx              the resulting backward motion vector
 *    mvby
 *
 *    fourMVs           1 if there is four motion vectors per macroblock
 *                      0 otherwise
 *
 * Function:
 *    This function gets the forward and backward motion vectors for the
 *    previous B macroblock.
 *
 * Returns:
 *    Changes *mvfx, *mvfy, *mvbx, *mvby and *fourMVs.
 *
 * Error codes:
 *    ERR_MVC_NO_PREV_MB            if the current MB has x coordinate 0 and
 *                                  mvcGetPrevNeighbourMVs is called
 *
 *    ERR_MVC_PREV_NOT_VALID        if the motion vectors for the previous
 *                                  macroblock do not exist
 *
 *    
 */

void mvcGetPrevMVFsAndMVBs(mvcData_t *mvcData, int *mvfx, int *mvfy,
   int *mvbx, int *mvby, u_char *fourMVs, int16 *error)
/* {{-output"mvcGetPrevMVFsAndMVBs.txt"}} */
{
   mvBFBufItem_t
      *mvFBuf = mvcData->mvFBufArray[mvcData->mvBFBufIndex ^ 1],
      *mvBBuf = mvcData->mvBBufArray[mvcData->mvBFBufIndex ^ 1];
   int i;

   if (mvcData->currX > 0) {
      if (mvBBuf[0].x == mvcData->currX - 1 &&
         mvValid(mvBBuf[0], mvcData->currY, mvcData->currTime)) {
         *fourMVs = mvBBuf[0].fourMVs;
         for (i = 0; i < 4; i++) {
            mvfx[i] = mvFBuf[i].mvx;
            mvfy[i] = mvFBuf[i].mvy;
            mvbx[i] = mvBBuf[i].mvx;
            mvby[i] = mvBBuf[i].mvy;
         }
      }
      else {
         /*deb("mvcGetPrevMVFsAndMVBs: ERROR - PREV_NOT_VALID.\n");*/
         *error = ERR_MVC_PREV_NOT_VALID;
      }
   }
   else {
      /*deb("mvcGetPrevMVFsAndMVBs: ERROR - NO_PREV_MB.\n");*/
      *error = ERR_MVC_NO_PREV_MB;
   }
}


/* {{-output"mvcMarkMB.txt"}} */
/*
 *
 * mvcMarkMBIntra
 * mvcMarkMBNotCoded
 *
 * Parameters:
 *    mvcData        mvcData_t structure
 *    x              the x coordinate of the MB (0 .. maxX)
 *    y              the y coordinate of the MB
 *                   (0 .. macroblock rows in frame - 1)
 *
 *    time           a value which is related to the time when the current
 *                   frame must be shown. This value should be unique
 *                   among a relatively small group of consecutive frames.
 *
 * Function:
 *    These functions are used to mark that the macroblock is either
 *    intra coded or not coded at all. The information is used when
 *    deciding the neighboring motion vectors of a macroblock in
 *    mvcGetPrevNeighbourMVs and mvcGetCurrNeighbourMVs.
 *    Note that mvcMarkMBIntra should not be called in case of
 *    INTRA block in PB-frame mode. Instead mvcCalcMV should be used.
 *
 * Returns:
 *    Nothing
 *
 * Error codes:
 *    None
 *
 *    
 *    
 */

void mvcMarkMBIntra(mvcData_t *mvcData, int x, int y, int time)
/* {{-output"mvcMarkMB.txt"}} */
{
   int
      xx2 = x << 1,
      yx2 = y << 1;
   mvRowItem_t
      *mvRow1,
      *mvRow2;

   updateRowPointers(x, y, time);
   mvRow1 = mvcData->mvRow1;
   mvRow2 = mvcData->mvRow2;
   mvStoreMB(mvRow1, mvRow2, xx2, yx2, MVD_INTRA, MVD_INTRA, time, MVC_MB_INTRA);
}

/* {{-output"mvcMarkMBNotCoded.txt"}} */
void mvcMarkMBNotCoded(mvcData_t *mvcData, int x, int y, int time)
/* {{-output"mvcMarkMBNotCoded.txt"}} */
{
   int
      xx2 = x << 1,
      yx2 = y << 1;
   mvRowItem_t
      *mvRow1,
      *mvRow2;

   updateRowPointers(x, y, time);
   mvRow1 = mvcData->mvRow1;
   mvRow2 = mvcData->mvRow2;
   mvStoreMB(mvRow1, mvRow2, xx2, yx2, MVD_NOT_CODED,
      MVD_NOT_CODED, time, MVC_MB_NOT_CODED);
}


/* {{-output"mvcStart.txt"}} */
/*
 *
 * mvcStart
 *
 * Parameters:
 *       mvcData        mvcData_t structure
 *
 *       maxX           the largest x coordinate possible for a macroblock
 *                      If maxX if different from maxX when the function was
 *                      last called, a new dynamic memory allocation is made.
 *
 *       lumWidth       Luminance width
 *
 *       lumHeight      Luminance height
 *
 *       error          error code
 *
 * Function:
 *       This function initialises motion vector buffers. It also allocates
 *       buffer memory if needed.
 *       One should call mvcStart in the beginning of each video sequence.
 *
 * Returns:
 *       Nothing
 *
 * Error codes:
 *       ERR_MVC_ALLOC1          dynamic allocation error
 *       ERR_MVC_MAX_X_ILLEGAL   maxX <= 0
 *
 *      
 *      
 *  
 */

void mvcStart(mvcData_t *mvcData, int maxX, int lumWidth, int lumHeight, int16 *error)
/* {{-output"mvcStart.txt"}} */
{
   int
      i, j, /* loop variables */
      rowSize; /* the number of items in prevMV?Row */

   if (maxX < 0) {
      *error = ERR_MVC_MAX_X_ILLEGAL;
      return;
   }

   rowSize = (maxX + 1) << 1;

   if (mvcData->currMaxX != maxX || mvcData->mvRow == NULL) {
      /* frame size changed */

      if (mvcData->mvRow) free(mvcData->mvRow);

      mvcData->mvRow = (mvRowItem_t *) vdcMalloc(
         rowSize * 3 * sizeof(mvRowItem_t));
      if (mvcData->mvRow == NULL) {
         deb("mvcStart: ERROR - memory allocation failed.\n");
         *error = ERR_MVC_ALLOC1;
         return;
      }

      mvcData->currMaxX = maxX;
   }
   
   /* Horizontal motion vector range when PLUSTYPE present and UUI = 1
      See Table D.1/H.263 */
   if (mvcData->currLumHeight != lumHeight)
   {
      mvcData->currLumHeight = lumHeight;
      if ((lumHeight>=4)&&(lumHeight<=288))
      {
         mvcData->mvRangeLowY = -320;
         mvcData->mvRangeHighY = 315;
      }
      else if ((lumHeight>=292)&&(lumHeight<=576))
      {
         mvcData->mvRangeLowY = -640;
         mvcData->mvRangeHighY = 635;
      }
      else if ((lumHeight>=580)&&(lumHeight<=1152))
      {
         mvcData->mvRangeLowY = -1280;
         mvcData->mvRangeHighY = 1275;
      }
   }

   /* Vertical motion vector range when PLUSTYPE present and UUI = 1
      See Table D.2/H.263 */
   if (mvcData->currLumWidth != lumWidth)
   {
      mvcData->currLumWidth = lumWidth;
      if ((lumWidth>=4)&&(lumWidth<=352))
      {
         mvcData->mvRangeLowX = -320;
         mvcData->mvRangeHighX = 315;
      }
      else if ((lumWidth>=356)&&(lumWidth<=704))
      {
         mvcData->mvRangeLowX = -640;
         mvcData->mvRangeHighX = 635;
      }
      else if ((lumWidth>=708)&&(lumWidth<=1408))
      {
         mvcData->mvRangeLowX = -1280;
         mvcData->mvRangeHighX = 1275;
      }
      else if ((lumWidth>=1412)&&(lumWidth<=2048))
      {
         mvcData->mvRangeLowX = -2560;
         mvcData->mvRangeHighX = 2555;
      }
   }
   /* Set time to be impossible */
   for (i = 0; i < 3 * rowSize; i++) {
      mvcData->mvRow[i].time = -2;
   }

   for (j = 0; j < 2; j++) {
      for (i = 0; i < 4; i++) {
         mvcData->mvFBufArray[j][i].time = -2;
         mvcData->mvBBufArray[j][i].time = -2;
      }
   }

   mvcData->currX = -1;
   mvcData->currY = -1;
   mvcData->currTime = -2;
   mvcData->mvRowIndex = 0;
   mvcData->mvBFBufIndex = 0;
   mvcData->prevPredMode = 3;
}


/* Local functions */



/*
 *
 * mvcGetNeighbourMVs
 *
 * Parameters:
 *       mvcData  mvcData_t structure
 *       x
 *       y        macroblock coordinates
 *       time     time reference
 *       nmvx     pointer to motion vector x component array
 *       nmvy     pointer to motion vector y component array
 *                See mvcGetCurrNeighbourMVs/mvcGetPrevNeighbourMVs for
 *                description.
 *       prevFlag If this flag is set, the previous border values should
 *                be used.
 *       error    error code
 *
 * Function:
 *       This function return the motion vectors of the neighboring
 *       blocks (on the left side, above or on the right side) of the
 *       macroblock which is investigated. mvcGetNeighbourMVs is used
 *       by mvcGetCurrNeighbourMVs and mvcGetPrevNeighbourMVs.
 *       See also the functional description for these functions.
 *
 * Returns:
 *       Changes nmvx and nmvy and possibly error.
 *
 * Error codes:
 *    ERR_MVC_NEIGHBOUR_NOT_VALID   if one of the neighboring MVs is not
 *                                  valid, i.e. it has not been updated
 *                                  for this frame. The MV in nmvx and nmvy
 *                                  is set to zero.
 *
 *    
 */

static void mvcGetNeighbourMVs(mvcData_t *mvcData, int x, int y, int time,
   int *nmvx, int *nmvy, int prevFlag, int16 *error)
{
   int
      xx2 = x << 1,
      yx2 = y << 1,
      xtmp,
      ytmp,
      rightOfBorder,
      downOfBorder,
      leftOfBorder;
   mvRowItem_t
      *mvRow0 = mvcData->mvRow0,
      *mvRow1 = mvcData->mvRow1,
      *mvRow2 = mvcData->mvRow2;

   if (prevFlag)  {
      rightOfBorder = mvcData->rightOfBorderPrev;
      downOfBorder = (mvcData->fSS)?mvcData->downOfBorderPrev:(y==0);
      leftOfBorder = 0;
   }
   else  {
      rightOfBorder = mvcData->rightOfBorder;
      downOfBorder = (mvcData->fSS)?mvcData->downOfBorder:(y==0);
      leftOfBorder = (mvcData->fSS)?(x == mvcData->currMaxX)||(mvcData->leftOfBorder):(x == mvcData->currMaxX);
   }
   //if (x > 0) {
   if (!rightOfBorder)  {
      xtmp = xx2 - 1;
      mvcCheckAndSet(mvRow1, xtmp, yx2, time, MVC_XM1_UP, nmvx, nmvy, 
         mvRow1, xx2, yx2, error);
      mvcCheckAndSet(mvRow2, xtmp, yx2 + 1, time, MVC_XM1_DOWN, nmvx, nmvy, 
         mvRow2, xx2, yx2 + 1, error);
   }
   else {
      mvcSetToCurrent(mvRow1, xx2, yx2, time, MVC_XM1_UP, nmvx, nmvy, 
         error);
      mvcSetToCurrent(mvRow2, xx2, yx2 + 1, time, MVC_XM1_DOWN, nmvx, nmvy, 
         error);
   }

   //if (y > 0) {
   if (!downOfBorder)   {
      ytmp = yx2 - 1;
      mvcCheckAndSet(mvRow0, xx2, ytmp, time, MVC_YM1_LEFT, nmvx, nmvy, 
         mvRow1, xx2, yx2, error);
      mvcCheckAndSet(mvRow0, xx2 + 1, ytmp, time, MVC_YM1_RIGHT, nmvx, nmvy,
         mvRow1, xx2 + 1, yx2, error);
   }
   else {
      mvcSetToCurrent(mvRow1, xx2, yx2, time, MVC_YM1_LEFT, nmvx, nmvy, 
         error);
      mvcSetToCurrent(mvRow1, xx2 + 1, yx2, time, MVC_YM1_RIGHT, nmvx, nmvy, 
         error);
   }

   //if (x < mvcData->currMaxX) {
   if (!leftOfBorder) {
      xtmp = xx2 + 2;
      mvcCheckAndSet(mvRow1, xtmp, yx2, time, MVC_XP1_UP, nmvx, nmvy, 
         mvRow1, xtmp - 1, yx2, error);
      mvcCheckAndSet(mvRow2, xtmp, yx2 + 1, time, MVC_XP1_DOWN, nmvx, nmvy,
         mvRow2, xtmp -1, yx2 + 1, error);
   }
   else {
      xtmp = xx2 + 1;
      mvcSetToCurrent(mvRow1, xtmp, yx2, time, MVC_XP1_UP, nmvx, nmvy, 
         error);
      mvcSetToCurrent(mvRow2, xtmp, yx2 + 1, time, MVC_XP1_DOWN, nmvx, nmvy, 
         error);
   }
}



/*
 * mvcCheckAndSet
 *
 * Parameters:
 *    mvRowPtr    Source row item pointer
 *    xind        x index of the source block
 *    yind        y index of the source block
 *    timeref     time reference
 *    nind        index to motion vector arrays ( x and y)
 *    nmvx        pointer to motion vector x component array
 *    nmvy        pointer to motion vector y component array
 *    cmvRowPtr   Target row item pointer
 *    cxind       x index of the target block
 *    cyind       y index of the target block
 *    error       error code
 *
 * Function:
 *       Sets the correct value for neighboring motion vector
 *       used from: mvcGetNeighbourMVs 
 *
 * Returns:
 *       Nothing
 *
 * Error codes:
 *
 */

__inline static void mvcCheckAndSet( 
      mvRowItem_t *mvRowPtr, int xind, int yind, int timeref, int nind, 
      int *nmvx, int *nmvy, mvRowItem_t *cmvRowPtr, int cxind, int cyind,
      int16 *error)
{

   if (mvLegal(mvRowPtr[xind], yind, timeref))  {
      if (mvRowPtr[xind].mvx >= MVD_UMIN_V2) {
         nmvx[nind] = mvRowPtr[xind].mvx; 
         nmvy[nind] = mvRowPtr[xind].mvy; 
      } 
      else if (mvRowPtr[xind].mvx == MVD_INTRA) { 
         if (mvLegal(cmvRowPtr[cxind], cyind, timeref)) { 
            if (mvValid(cmvRowPtr[cxind], cyind, timeref)) { 
               nmvx[nind] = cmvRowPtr[cxind].mvx; 
               nmvy[nind] = cmvRowPtr[cxind].mvy; 
            } 
            else /* Not coded macroblock */ 
               nmvx[nind] = nmvy[nind] = 0; 
         } 
         else { 
            deb("mvcCheckAndSet: ERROR - neighbour not valid.\n"); 
            *error = ERR_MVC_NEIGHBOUR_NOT_VALID; 
            nmvx[nind] = nmvy[nind] = 0; 
         } 
      } 
      else { /* MVD_NOT_CODED */ 
         nmvx[nind] = nmvy[nind] = 0; 
      } 
   } 
   else { 
      deb("mvcCheckAndSet: ERROR - neighbour not valid.\n"); 
      *error = ERR_MVC_NEIGHBOUR_NOT_VALID; 
      nmvx[nind] = nmvy[nind] = 0; 
   }
}

/*
 * mvcSetToCurrent
 *
 * Parameters:
 *    mvRowPtr    Current row item pointer
 *    xind        x index of the block
 *    yind        y index of the block
 *    timeref     time reference
 *    nind        index to motion vector arrays ( x and y)
 *    nmvx        pointer to motion vector x component array
 *    nmvy        pointer to motion vector y component array
 *    error       error code
 *
 * Function:
 *       Sets the correct value for neighboring motion vector using current
 *       motion vector, used in cases of border used from: mvcGetNeighbourMVs
 *
 * Returns:
 *       Nothing
 *
 * Error codes:
 *
 * History
 */

__inline static void mvcSetToCurrent( 
      mvRowItem_t *mvRowPtr, int xind, int yind, int timeref, int nind,
      int *nmvx, int *nmvy, int16 *error)
{
   if (mvValid(mvRowPtr[xind], yind, timeref)) { 
      nmvx[nind] = mvRowPtr[xind].mvx; 
      nmvy[nind] = mvRowPtr[xind].mvy; 
   } 
   else { 
      deb("mvcSetToCurrent: ERROR - neighbour not valid.\n"); 
      *error = ERR_MVC_NEIGHBOUR_NOT_VALID; 
      nmvx[nind] = nmvy[nind] = 0; 
   }
}

// End of File
