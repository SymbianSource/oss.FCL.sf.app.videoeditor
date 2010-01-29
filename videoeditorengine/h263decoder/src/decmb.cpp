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
* Macroblock decoding functions.
*
*/


/* 
 * Includes 
 */
#include "h263dConfig.h"
#include "decmb.h"
#include "viddemux.h"
#include "decmbdct.h"
#include "errcodes.h"
/* MVE */
#include "MPEG4Transcoder.h"

/*
 * Global functions
 */

/* {{-output"dmbGetAndDecodeIFrameMB.txt"}} */
/*
 * dmbGetAndDecodeIFrameMB
 *    
 *
 * Parameters:
 *    inParam                    input parameters
 *    inOutParam                 input/output parameters, these parameters
 *                               may be modified in the function
 *    fMPEG4                     flag indicating if H.263 ("0") or MPEG-4 ("1")
 *                               specific block decoding should be used
 *
 * Function:
 *    This function gets the coding parameters of a macroblock belonging
 *    to an INTRA frame (from the bitstream) and decodes the macroblock.
 *
 * Returns:
 *    >= 0                       the function was successful
 *    < 0                        an error occured when accessing bit buffer
 *
 */
    
int dmbGetAndDecodeIFrameMB(
   const dmbIFrameMBInParam_t *inParam,
   dmbIFrameMBInOutParam_t *inOutParam,
   u_char fMPEG4, CMPEG4Transcoder *hTranscoder)
/* {{-output"dmbGetAndDecodeIFrameMB.txt"}} */
{
   int
      bitErrorIndication = 0, 
                        /* Carries bit error indication information returned
                           by the video demultiplexer module */
      ret = 0;          /* Used to check return values of function calls */

   vdxGetIMBLayerInputParam_t 
      vdxIn;            /* Input parameters for vdxGetIMBLayer */

   vdxIMBLayer_t 
      mbLayer;          /* Macroblock layer data */

   int
      rightOfBorder,    /* There is a border on the left of the current MB */
      downOfBorder;     /* There is a border on top of the current MB */

   int StartByteIndex = inOutParam->StartByteIndex;
   int StartBitIndex  = inOutParam->StartBitIndex;

   inOutParam->fCodedMBs[inParam->yPosInMBs * 
      inParam->pictParam->numMBsInMBLine + inParam->xPosInMBs] = 1;
   inOutParam->numOfCodedMBs++;

   /* Get MB layer parameters */
   
   vdxIn.fMQ = inParam->pictParam->fMQ;
   vdxIn.quant = inOutParam->quant;
   vdxIn.fAIC = inParam->pictParam->fAIC;
   vdxIn.fMPEG4 = fMPEG4;

   ret = vdxGetIMBLayer(inParam->inBuffer, inParam->outBuffer, inParam->bufEdit, inParam->iColorEffect,&StartByteIndex, &StartBitIndex, 
            inParam->iGetDecodedFrame, &vdxIn, &mbLayer, 
            &bitErrorIndication, hTranscoder);

   if ( ret <0 )
      goto error;
   else if ( ret == VDX_OK_BUT_BIT_ERROR )
      goto bitError;
   
   /* Store output parameters */
   inOutParam->quant = mbLayer.quant;
   
   /* Get block layer parameters and decode them */


   if(fMPEG4) {
       dmdMPEGIParam_t dmdIn;

   
       dmdIn.inBuffer = inParam->inBuffer;
       dmdIn.outBuffer = inParam->outBuffer;
       dmdIn.bufEdit = inParam->bufEdit;
       dmdIn.iColorEffect = inParam->iColorEffect;
       dmdIn.iGetDecodedFrame = inParam->iGetDecodedFrame;

       dmdIn.cbpy = mbLayer.cbpy;
       dmdIn.cbpc = mbLayer.cbpc;
       dmdIn.quant = mbLayer.quant;
       dmdIn.yWidth = inParam->pictParam->lumMemWidth;
       dmdIn.yMBInFrame = inOutParam->yMBInFrame;
       dmdIn.uBlockInFrame = inOutParam->uBlockInFrame;
       dmdIn.vBlockInFrame = inOutParam->vBlockInFrame;

       dmdIn.xPosInMBs = inParam->xPosInMBs;
       dmdIn.yPosInMBs = inParam->yPosInMBs;
       dmdIn.numMBsInMBLine = inParam->pictParam->numMBsInMBLine;
       dmdIn.numMBLinesInGOB = inParam->pictParam->numMBLinesInGOB;
       dmdIn.pictureType = inParam->pictParam->pictureType;

       inOutParam->aicData->ACpred_flag = mbLayer.ac_pred_flag;
       dmdIn.aicData = inOutParam->aicData;
  
       dmdIn.switched = 
           aicIntraDCSwitch(inParam->pictParam->intra_dc_vlc_thr,mbLayer.quant);
  
       dmdIn.data_partitioned = 0;
       dmdIn.reversible_vlc = 0;
  
       dmdIn.currMBNum = inOutParam->currMBNum;
  
       dmdIn.fTopOfVP = (u_char) (inOutParam->currMBNumInVP < inParam->pictParam->numMBsInMBLine);
       dmdIn.fLeftOfVP = (u_char) (inOutParam->currMBNumInVP == 0);
       dmdIn.fBBlockOut = (u_char) (inOutParam->currMBNumInVP <= inParam->pictParam->numMBsInMBLine);
  
       ret = dmdGetAndDecodeMPEGIMBBlocks(&dmdIn, hTranscoder);

       if ( ret < 0 )
         goto error;
       else if ( ret == DMD_BIT_ERR )
         goto bitError;
  
   } else 

   {
      dmdIParam_t dmdIn;
              
      /* Store the coding type of the MB*/
      if ( inParam->pictParam->fAIC )  {
         mvcSetBorders(
            NULL, 
            inParam->xPosInMBs,
            inParam->yPosInMBs,
            (inParam->pictParam->fSS)?inParam->sliceStartMB:-1,  /* If Annex K is not in use, set to -1 */
            inParam->pictParam->numMBsInMBLine, 
            &rightOfBorder, 
            &downOfBorder);
      }
         
  
      dmdIn.inBuffer = inParam->inBuffer;


      dmdIn.outBuffer = inParam->outBuffer;
      dmdIn.bufEdit = inParam->bufEdit;
      dmdIn.iColorEffect = inParam->iColorEffect;
      dmdIn.iGetDecodedFrame = inParam->iGetDecodedFrame;
      dmdIn.StartByteIndex = inOutParam->StartByteIndex;
      dmdIn.StartBitIndex  = inOutParam->StartBitIndex;


      dmdIn.cbpy = mbLayer.cbpy;
      dmdIn.cbpc = mbLayer.cbpc;
      dmdIn.quant = mbLayer.quant;
      dmdIn.yWidth = inParam->pictParam->lumMemWidth;
      dmdIn.yMBInFrame = inOutParam->yMBInFrame;
      dmdIn.uBlockInFrame = inOutParam->uBlockInFrame;
      dmdIn.vBlockInFrame = inOutParam->vBlockInFrame;

      dmdIn.xPosInMBs = inParam->xPosInMBs;
      dmdIn.yPosInMBs = inParam->yPosInMBs;
      dmdIn.numMBsInMBLine = inParam->pictParam->numMBsInMBLine;
      dmdIn.numMBLinesInGOB = inParam->pictParam->numMBLinesInGOB;
      dmdIn.pictureType = inParam->pictParam->pictureType;

      dmdIn.predMode = mbLayer.predMode;
      dmdIn.numMBsInMBLine = inParam->pictParam->numMBsInMBLine;
      dmdIn.fGOBHeaderPresent = inParam->fGOBHeaderPresent;
      dmdIn.rightOfBorder = rightOfBorder;
      dmdIn.downOfBorder = downOfBorder;
      dmdIn.sumBEI = 0;

      if (!inParam->pictParam->fAIC) 
         ret = dmdGetAndDecodeIMBBlocks(&dmdIn, hTranscoder);
      else
        {
        // not supported
        goto error;
        }

      inOutParam->StartByteIndex = dmdIn.StartByteIndex;
      inOutParam->StartBitIndex = dmdIn.StartBitIndex;


      if ( ret < 0 )
         goto error;
      else if ( ret == DMD_BIT_ERR )
         goto bitError;
   }

   return DMB_OK;

bitError:

   inOutParam->fCodedMBs[inParam->yPosInMBs * 
      inParam->pictParam->numMBsInMBLine + inParam->xPosInMBs] = 0;
   inOutParam->numOfCodedMBs--;
   return DMB_BIT_ERR;

error:
   return DMB_ERR;
}
    
    
/* {{-output"dmbGetAndDecodePFrameMB.txt"}} */
/*
 * dmbGetAndDecodePFrameMB
 *    
 *
 * Parameters:
 *    inParam                    input parameters
 *    inOutParam                 input/output parameters, these parameters
 *                               may be modified in the function
 *    fMPEG4                     flag indicating if H.263 ("0") or MPEG-4 ("1")
 *                               specific block decoding should be used
 *
 * Function:
 *    This function gets the coding parameters of a macroblock belonging
 *    to an INTER frame (from the bitstream) and decodes the macroblock.
 *
 * Returns:
 *    >= 0                       the function was successful
 *    < 0                        an error occured when accessing bit buffer
 *
 */

int dmbGetAndDecodePFrameMB(
   const dmbPFrameMBInParam_t *inParam,
   dmbPFrameMBInOutParam_t *inOutParam,
   u_char fMPEG4, CMPEG4Transcoder *hTranscoder)
/* {{-output"dmbGetAndDecodePFrameMB.txt"}} */
{
   int
      bitErrorIndication = 0, 
                        /* Carries bit error indication information returned
                           by the video demultiplexer module */
      sumBEI = 0,       /* Sum (bit-wise OR) of bit error indications for the whole MB */
      ret,              /* Used to check return values of function calls */
      mbPos,            /* the position of the current macroblock, 
                           -1 = the leftmost MB of the image, 
                           0 = MB is not in the border of the image, 
                           1 = rightmost MB of the image */
      cbpy,             /* Coced block pattern for luminance */
      xPosInMBs,        /* Current macroblock position in x-direction 
                           in units of macroblocks starting from zero */
      yPosInMBs,        /* Current macroblock position in y-direction 
                           in units of macroblocks starting from zero */
      numMBsInMBLine,   /* The number of macroblocks in one line */
      yHeight,          /* Luminance image height in pixels */
      uvHeight,         /* Chrominance image height in pixels */
      yWidth,           /* Luminance image width in pixels */
      uvWidth,          /* Chrominance image width in pixels */
      mbNum,            /* Macroblock number within a picture starting
                           from zero in the top-left corner and
                           increasing in scan-order */
      quant;            /* Current quantization parameter */

   /* Motion vectors for P-macroblock */
   int mvx[4];
   int mvy[4];


   /* MVE */
   int StartByteIndex = inOutParam->StartByteIndex;
   int StartBitIndex  = inOutParam->StartBitIndex;

   int16 
      error = 0;        /* Used for return value of vdcmvc module */

   u_char 
      fourMVs,          /* Flag which tells if four motion vectors is
                           present in the current macroblock */
      mbNotCoded;       /* == 1 if current macro block is not coded */

   vdxGetPPBMBLayerInputParam_t 
      vdxIn;            /* Input parameters for vdxGetPPBMBLayer */

   vdxPPBMBLayer_t 
      mbLayer;          /* Macroblock layer data */
   int
      rightOfBorder,    /* There is a border on the left of the current MB */
      downOfBorder;     /* There is a border on top of the current MB */

   /* Add assertions here */

   xPosInMBs = inParam->xPosInMBs;
   yPosInMBs = inParam->yPosInMBs;
   numMBsInMBLine = inParam->pictParam->numMBsInMBLine;
   mbNum = yPosInMBs * numMBsInMBLine + xPosInMBs;
   yHeight = inParam->pictParam->lumMemHeight;
   uvHeight = (yHeight >>1 /*/ 2*/);
   yWidth = inParam->pictParam->lumMemWidth;
   uvWidth = (yWidth >>1 /*/ 2*/);

   /* mbPos, needed in blcCopyPredictionMB */
   if (inParam->pictParam->fSS) {
      if (xPosInMBs == numMBsInMBLine - 1)
         if (mbNum == inParam->sliceStartMB)
            mbPos = 2;
         else
            mbPos = 1;
      else if (mbNum == inParam->sliceStartMB) 
         /* if this is the first MB of the slice but not the last MB of the MB line */
         mbPos = -1;
      else if (xPosInMBs == 0)
         mbPos = -1;
      else
         mbPos = 0;  
   }
   else  {
      if (xPosInMBs == 0)
         mbPos = -1;
      else if (xPosInMBs == numMBsInMBLine - 1)
         mbPos = 1;
      else
         mbPos = 0;
   }

   /* Get MB layer parameters */
   vdxIn.pictureType = inParam->pictParam->pictureType;
   vdxIn.fPLUSPTYPE = inParam->pictParam->fPLUSPTYPE;
   vdxIn.fUMV = inParam->pictParam->fUMV;
   vdxIn.fDF = inParam->pictParam->fDF;
   vdxIn.fMQ = inParam->pictParam->fMQ;
   vdxIn.fCustomSourceFormat = inParam->pictParam->fCustomSourceFormat;
   vdxIn.fAIC = inParam->pictParam->fAIC;
   vdxIn.quant = inOutParam->quant;
   vdxIn.fFirstMBOfPicture = (yPosInMBs == 0 && xPosInMBs == 0);

   vdxIn.fMPEG4 = fMPEG4;

   if (fMPEG4) {
       vdxIn.fAP = 1;
       vdxIn.f_code = inParam->pictParam->fcode_forward;
   } else 

   {
       vdxIn.fAP = inParam->pictParam->fAP;
   }


   int mbType=3;    // default
   ret = vdxGetPPBMBLayer(inParam->inBuffer, inParam->outBuffer, inParam->bufEdit, inParam->iColorEffect,&StartByteIndex, &StartBitIndex,
         inParam->iGetDecodedFrame, &mbType, &vdxIn, &mbLayer, &bitErrorIndication,
         hTranscoder);

   if ( ret < 0 )
      goto error;
   else if ( ret == VDX_OK_BUT_BIT_ERROR ) {
      goto bitError;
   }
   /* PB macroblock */
   if  ((inParam->pictParam->pictureType == VDX_PIC_TYPE_PB) ||
         (inParam->pictParam->pictureType == VDX_PIC_TYPE_IPB)) {

        // PB not supported
        goto error;
   }  /* if (PB macroblock) */

   sumBEI |= bitErrorIndication;

   inOutParam->quant = quant = mbLayer.quant;

   cbpy = mbLayer.cbpy;
   fourMVs = (u_char) (mbLayer.numMVs == 4);

   if(!fMPEG4) {
      mvcSetBorders(
         inOutParam->mvcData, 
         xPosInMBs,
         yPosInMBs,
         (inParam->pictParam->fSS)?inParam->sliceStartMB:-1,  /* If Annex K is not in use, set to -1 */
         numMBsInMBLine, 
         &rightOfBorder, 
         &downOfBorder);
   }

   if (mbLayer.fCodedMB) {
      int currMVNum;

      /* Decode motion vectors */
      mbNotCoded = 0;
      inOutParam->fCodedMBs[mbNum] = 1;
      inOutParam->numOfCodedMBs++;

      for (currMVNum = 0; currMVNum < mbLayer.numMVs; currMVNum++) {

          if(fMPEG4)
              mvcCalcMPEGMV(
                  inOutParam->mvcData,
                  mbLayer.mvdx[currMVNum], mbLayer.mvdy[currMVNum],
                  &mvx[currMVNum], &mvy[currMVNum],
                  (u_char) currMVNum, fourMVs,
                  (u_char) (inOutParam->currMBNumInVP < inParam->pictParam->numMBsInMBLine),
                  (u_char) (inOutParam->currMBNumInVP == 0), 
                  (u_char) (inOutParam->currMBNumInVP < (inParam->pictParam->numMBsInMBLine-1)),
                  xPosInMBs,
                  yPosInMBs,
                  inParam->pictParam->tr,
                  (mbLayer.mbClass == VDX_MB_INTRA) ? MVC_MB_INTRA : MVC_MB_INTER,
                  &error);    
          else {
             mvcCalcMV(
                  inOutParam->mvcData, 
                  mbLayer.mvdx[currMVNum], mbLayer.mvdy[currMVNum],
                  &mvx[currMVNum], &mvy[currMVNum],
                  (u_char) currMVNum, 
                  (u_char) (mbLayer.numMVs == 4),
                  (u_char) inParam->pictParam->fUMV,
                  (u_char) ((inParam->pictParam->fSS)?1:inParam->fGOBHeaderPresent),
                  xPosInMBs,
                  yPosInMBs,
                  inParam->pictParam->tr,
                  (mbLayer.mbClass == VDX_MB_INTRA) ? 
                        MVC_MB_INTRA : MVC_MB_INTER,
                  &error,
                  inParam->pictParam->fPLUSPTYPE,
                  inParam->pictParam->fUMVLimited);
          }

          /* If motion vector points illegally outside the picture,
             there may be two reasons for it:
             1) bit error has occured and corrupted MVD, or
             2) encoder (e.g. /UBC) does not follow the standard.
             Since we assume that encoders may violate this feature relatively
             frequently, the decoder considers these cases as bit errors
             only if the demultiplexer indicates a similar condition.
             Note that there may be a very improbable situation where
             the demultiplexer error indication has failed (it reports
             no errors even though there are errors), and these bit errors
             would cause an illegal motion vector. Now, we won't detect
             these cases. */
          if (error == ERR_MVC_MVPTR && bitErrorIndication)
               goto bitError;
          else if (error && error != ERR_MVC_MVPTR)
               goto error;
      }

      if (mbLayer.numMVs == 1) {
         mvx[1] = mvx[2] = mvx[3] = mvx[0];
         mvy[1] = mvy[2] = mvy[3] = mvy[0];
      }
   }

   else {
      mbNotCoded = 1;
      /* Motion vectors to 0 */
      mvx[0] = mvx[1] = mvx[2] = mvx[3] =
         mvy[0] = mvy[1] = mvy[2] = mvy[3] = 0;
      mvcMarkMBNotCoded(
         inOutParam->mvcData, 
         xPosInMBs,
         yPosInMBs,
         inParam->pictParam->tr);
      inOutParam->fCodedMBs[mbNum] = 0;
      cbpy = 0;
      fourMVs = (u_char) (fMPEG4 ? fourMVs : inParam->pictParam->fAP);
   }
  
   
  
   /* If INTER MB */
   if (mbNotCoded || mbLayer.mbClass == VDX_MB_INTER) {
       dmdPParam_t dmdIn;
       blcCopyPredictionMBParam_t blcCopyParam;
       
       dmdIn.inBuffer = inParam->inBuffer;


       dmdIn.outBuffer = inParam->outBuffer;
       dmdIn.bufEdit = inParam->bufEdit;
       dmdIn.iColorEffect = inParam->iColorEffect;
       dmdIn.iGetDecodedFrame = inParam->iGetDecodedFrame;
       dmdIn.StartByteIndex = inOutParam->StartByteIndex;
       dmdIn.StartBitIndex  = inOutParam->StartBitIndex;
       dmdIn.mbType = mbType; 

       dmdIn.cbpy = cbpy;
       dmdIn.cbpc = mbLayer.cbpc;
       dmdIn.quant = quant;
       dmdIn.refY = inParam->refY;
       dmdIn.refU = inParam->refU;
       dmdIn.refV = inParam->refV;
       dmdIn.currYMBInFrame = inOutParam->yMBInFrame;
       dmdIn.currUBlkInFrame = inOutParam->uBlockInFrame;
       dmdIn.currVBlkInFrame = inOutParam->vBlockInFrame;
       dmdIn.uvBlkXCoord = xPosInMBs * 8;
       dmdIn.uvBlkYCoord = yPosInMBs * 8;
       dmdIn.uvWidth = uvWidth;
       dmdIn.uvHeight = uvHeight;
       dmdIn.mvcData = inOutParam->mvcData;
       dmdIn.mvx = mvx;
       dmdIn.mvy = mvy;
       dmdIn.mbPlace = mbPos;
       dmdIn.fAdvancedPrediction = inParam->pictParam->fAP;
       dmdIn.fMVsOverPictureBoundaries =
           inParam->pictParam->fMVsOverPictureBoundaries;
       dmdIn.diffMB = inOutParam->diffMB;
       dmdIn.rcontrol = inParam->pictParam->rtype;

      dmdIn.fourMVs = fourMVs;
      dmdIn.reversible_vlc = 0;

      dmdIn.xPosInMBs = xPosInMBs;
      dmdIn.yPosInMBs = yPosInMBs;
      dmdIn.numMBsInMBLine = inParam->pictParam->numMBsInMBLine;

      /* Copy blcCopyPredictionMB parameters from input parameters */
      memcpy(&blcCopyParam, &(dmdIn.refY), sizeof(blcCopyPredictionMBParam_t));
      /* Note: In order to operate properly, this memcpy requires that
            the structure members are in the same order and allocate the same
            amount of space. This is not guaranteed in C! */
            
      if (inParam->iGetDecodedFrame || hTranscoder->NeedDecodedYUVFrame())
      {
           /* Do motion compensation */
           if (blcCopyPredictionMB(&blcCopyParam) < 0) {
               /* MV was illegal => caused by bitError */
               goto bitError;
           }
      }


      if (fMPEG4) {
        /* Update the AIC module data, marking the MB as Inter (quant=0) */
        aicBlockUpdate (inOutParam->aicData, inOutParam->currMBNum, 0, NULL, 0, 0);
      }


      /* Store new CBPY */
      inOutParam->diffMB->cbpy = cbpy;

      /* If some prediction error blocks are coded */
      if (mbLayer.fCodedMB) {
          /* Decode prediction error blocks */

          if (fMPEG4) {
              ret = dmdGetAndDecodeMPEGPMBBlocks(&dmdIn, hTranscoder);
          } else 

          {
              ret = dmdGetAndDecodePMBBlocks(&dmdIn, hTranscoder);
          }

          
          inOutParam->StartByteIndex = dmdIn.StartByteIndex;
          inOutParam->StartBitIndex = dmdIn.StartBitIndex;


          if ( ret < 0)
               goto error;
          else if ( ret == DMD_BIT_ERR ) {
               goto bitError;
          }
      }

      else  // for the case when the MB is not coded 
      {
        /* nothing here */
      }


   }  /* if (INTER block ) */
   
   /* Else block layer decoding of INTRA macroblock */
   else {
        
       if (inParam->pictParam->pictureType != VDX_PIC_TYPE_PB) 
           mvcMarkMBIntra(inOutParam->mvcData, xPosInMBs, yPosInMBs, 
           inParam->pictParam->tr);
       
       inOutParam->diffMB->cbpy = 0;
       
       /* Get block layer parameters and decode them */
       
       if(fMPEG4) {
             dmdMPEGIParam_t dmdIn;
             
             dmdIn.inBuffer = inParam->inBuffer;

             /* MVE */
             dmdIn.outBuffer = inParam->outBuffer;
             dmdIn.bufEdit = inParam->bufEdit;
             dmdIn.iColorEffect = inParam->iColorEffect;
             dmdIn.iGetDecodedFrame = inParam->iGetDecodedFrame;

             dmdIn.cbpy = cbpy;
             dmdIn.cbpc = mbLayer.cbpc;
             dmdIn.quant = quant;
             dmdIn.yWidth = yWidth;
             dmdIn.yMBInFrame = inOutParam->yMBInFrame;
             dmdIn.uBlockInFrame = inOutParam->uBlockInFrame;
             dmdIn.vBlockInFrame = inOutParam->vBlockInFrame;
             
             dmdIn.xPosInMBs = inParam->xPosInMBs;
             dmdIn.yPosInMBs = inParam->yPosInMBs;
             dmdIn.numMBsInMBLine = inParam->pictParam->numMBsInMBLine;
             dmdIn.numMBLinesInGOB = inParam->pictParam->numMBLinesInGOB;
             dmdIn.pictureType = inParam->pictParam->pictureType;
             
             
             inOutParam->aicData->ACpred_flag = mbLayer.ac_pred_flag;
             dmdIn.aicData = inOutParam->aicData;
             
             dmdIn.switched = 
                 aicIntraDCSwitch(inParam->pictParam->intra_dc_vlc_thr,mbLayer.quant);
             
             dmdIn.data_partitioned = 0;
             dmdIn.reversible_vlc = 0;
             
             dmdIn.currMBNum = inOutParam->currMBNum;
             
             dmdIn.fTopOfVP = (u_char) 
                 (inOutParam->currMBNumInVP < inParam->pictParam->numMBsInMBLine ||
                 !aicIsBlockValid(inOutParam->aicData, inOutParam->currMBNum-inParam->pictParam->numMBsInMBLine));
             dmdIn.fLeftOfVP = (u_char)
                 (inOutParam->currMBNumInVP == 0 || 
                 inParam->xPosInMBs == 0 ||
                 !aicIsBlockValid(inOutParam->aicData, inOutParam->currMBNum-1));
             dmdIn.fBBlockOut = (u_char) 
                 (inOutParam->currMBNumInVP <= inParam->pictParam->numMBsInMBLine ||
                 inParam->xPosInMBs == 0 ||
                 !aicIsBlockValid(inOutParam->aicData, inOutParam->currMBNum-inParam->pictParam->numMBsInMBLine-1));
         
             ret = dmdGetAndDecodeMPEGIMBBlocks(&dmdIn, hTranscoder);
             
             if ( ret < 0 )
                 goto error;
             else if ( ret == DMD_BIT_ERR )
                 goto bitError;
                 
       } else 

       {
             dmdIParam_t dmdIn;
             
             dmdIn.inBuffer = inParam->inBuffer;           
             
             dmdIn.outBuffer = inParam->outBuffer;
             dmdIn.bufEdit = inParam->bufEdit;
             dmdIn.iColorEffect = inParam->iColorEffect; 
             dmdIn.iGetDecodedFrame = inParam->iGetDecodedFrame;
             dmdIn.StartByteIndex = inOutParam->StartByteIndex;
             dmdIn.StartBitIndex  = inOutParam->StartBitIndex;
             
             dmdIn.cbpy = cbpy;
             dmdIn.cbpc = mbLayer.cbpc;
             dmdIn.quant = quant;
             dmdIn.yWidth = yWidth;
             dmdIn.yMBInFrame = inOutParam->yMBInFrame;
             dmdIn.uBlockInFrame = inOutParam->uBlockInFrame;
             dmdIn.vBlockInFrame = inOutParam->vBlockInFrame;
             
             dmdIn.xPosInMBs = inParam->xPosInMBs;
             dmdIn.yPosInMBs = inParam->yPosInMBs;
             dmdIn.numMBsInMBLine = inParam->pictParam->numMBsInMBLine;
             dmdIn.numMBLinesInGOB = inParam->pictParam->numMBLinesInGOB;
             dmdIn.pictureType = inParam->pictParam->pictureType;
             
             dmdIn.predMode = mbLayer.predMode;
             dmdIn.numMBsInMBLine = inParam->pictParam->numMBsInMBLine;
             dmdIn.fGOBHeaderPresent = (inParam->pictParam->fSS)?1:inParam->fGOBHeaderPresent;
             dmdIn.rightOfBorder = rightOfBorder;
             dmdIn.downOfBorder = downOfBorder;
             
             if (!inParam->pictParam->fAIC)
                 ret = dmdGetAndDecodeIMBBlocks(&dmdIn, hTranscoder);
             else
                {
                // not supported
                goto error;
                }
             
             inOutParam->StartByteIndex = dmdIn.StartByteIndex;
             inOutParam->StartBitIndex = dmdIn.StartBitIndex;
             
             if ( ret < 0 )
                 goto error;
             else if ( ret == DMD_BIT_ERR )
                 goto bitError;
       }
   }
   
   
   
   return DMB_OK;

bitError:
   if ( inOutParam->fCodedMBs[mbNum] ) {
      inOutParam->fCodedMBs[mbNum] = 0;
      inOutParam->numOfCodedMBs--;
   }
   return DMB_BIT_ERR;

error:
   return DMB_ERR;
}

// End of File
