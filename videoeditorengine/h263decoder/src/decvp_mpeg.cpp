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
* Video packet decoding module (MPEG-4).
*
*/


/*
 * Includes
 */
#include "h263dConfig.h"
#include "decvp_mpeg.h"
#include "block.h"
#include "debug.h"
#include "decmbs.h"
#include "stckheap.h"
#include "sync.h"
#include "viddemux.h"
#include "biblin.h"
/* MVE */
#include "MPEG4Transcoder.h"

/*
 * Global functions
 */

/* {{-output"dvpGetAndDecodeVideoPacketHeader.txt"}} */
/*
 * dvpGetAndDecodeVideoPacketHeader
 *    
 *
 * Parameters:
 *    inParam                    input parameters
 *    inOutParam                 input/output parameters, these parameters
 *                               may be modified in the function
 *
 * Function:
 *    This function gets and decodes a Video Packet header at the 
 *    current position of the bit buffer. It checks its correctness and 
 *    if it fits to the sequence of VPs, and does the necessary actions for 
 *    correction.
 *
 * Returns:
 *    >= 0                       the function was successful
 *    < 0                        an error occured
 *
 */

int dvpGetAndDecodeVideoPacketHeader(
   const dvpVPInParam_t *inParam,
   dvpVPInOutParam_t *inOutParam)
/* {{-output"dvpGetAndDecodeVideoPacketHeader.txt"}} */
{
   int16 error = 0;
   vdxGetVideoPacketHeaderInputParam_t vdxParam;
   vdxVideoPacketHeader_t header;
   bibBuffer_t *inBuffer = inParam->inBuffer;
   int bitErrorIndication = 0, retVal, fLostSegment = FALSE;

   /* 
    * Get VP header 
    */

   vdxParam.fcode_forward = inParam->pictParam->fcode_forward;
   vdxParam.time_increment_resolution = inParam->pictParam->time_increment_resolution;
   vdxParam.numOfMBs = inParam->pictParam->numMBsInGOB;

   retVal = vdxGetVideoPacketHeader(inBuffer, &vdxParam, &header, &bitErrorIndication);
   if (retVal < 0) {
     return DGOB_ERR;
   } else if (retVal > 0) {
      deb("dvpGetAndDecodeVideoPacketHeader: ERROR - vdxGetVideoPacketHeader failed.\n");
      goto headerFailure;
   }


   /*
    * Check header validity
    */

   if (header.currMBNum == 0 || header.currMBNum >= inParam->pictParam->numMBsInGOB) {
      deb("dvpGetAndDecodeVideoPacketHeader: ERROR - too big currMBNum.\n");
      goto headerFailure;
   }
   
   /* quant can not be zero */
   if(header.quant == 0) {
      goto headerFailure;
   }

   if (header.fHEC) {

      if (header.time_base_incr < 0 || header.time_base_incr > 60) {
         if (bitErrorIndication) {
            goto headerFailure;
         }
      }

      /* fcode can have only the valid values 1..7 */
      if (header.coding_type == VDX_VOP_TYPE_P) 
         if (header.fcode_forward == 0) {
            goto headerFailure;
         }

      if (!inParam->fVOPHeaderCorrupted) {

         if ((inParam->pictParam->pictureType != header.coding_type) ||
            (inParam->pictParam->time_base_incr != header.time_base_incr) ||
            (inParam->pictParam->time_inc != header.time_inc)) {
            deb("dvpGetAndDecodeVideoPacketHeader: ERROR - Parameter change VOP<->VP header.\n");
            goto headerFailure;
         }
      }
   }
   if (inParam->fVOPHeaderCorrupted) {
      /* Get the coding type parameter since it is needed in the concealment before the other updates */
      if (header.fHEC) {
         inParam->pictParam->pictureType = header.coding_type;
      }
   }
   if (header.currMBNum != inOutParam->currMBNum) {
      if ( header.currMBNum > inOutParam->currMBNum && bitErrorIndication == 0) {

         fLostSegment = TRUE;
         goto headerFailure;

      } else if (header.currMBNum < inOutParam->currMBNum) {
         deb("dvpGetAndDecodeVideoPacketHeader: ERROR - MB counting is out of sync.\n");
         goto headerFailure;
      }
   }

   /*
    * Update parameters
    */

   inOutParam->currMBNum = header.currMBNum;
   inOutParam->quant = header.quant;

   if (inParam->fVOPHeaderCorrupted) {
      if (header.fHEC) {
         inParam->pictParam->mod_time_base += inParam->pictParam->time_base_incr = header.time_base_incr;
         
         inParam->pictParam->time_inc = header.time_inc;

         inOutParam->frameNum = (int) ((inParam->pictParam->mod_time_base + 
            ((double) header.time_inc) / ((double) inParam->pictParam->time_increment_resolution)) *
            30.0 + 0.001);

         inParam->pictParam->tr = inOutParam->frameNum % 256;
         inParam->pictParam->pictureType = header.coding_type;

         inParam->pictParam->intra_dc_vlc_thr = header.intra_dc_vlc_thr;

         if (header.coding_type == VDX_VOP_TYPE_P)
             if (inParam->pictParam->fcode_forward != header.fcode_forward) {
                 /* Initialize once to count parameters for the mvc module */
                 int r_size, scale_factor;
                 
                 inParam->pictParam->fcode_forward = header.fcode_forward;

                 inOutParam->mvcData->f_code = inParam->pictParam->fcode_forward;
                 r_size = inParam->pictParam->fcode_forward - 1;
                 scale_factor = (1 << r_size);
                 inOutParam->mvcData->range = 160 * scale_factor;
             }
      } else {
         /* seek next PSC, VP start code is not good enough */
         sncRewindAndSeekNewMPEGSync(-1, inBuffer, 0, &error);
         return DGOB_OK_BUT_BIT_ERROR;
      }
   }
   
   if (fLostSegment)
       return DGOB_OK_BUT_BIT_ERROR;
   else
       return DGOB_OK;

   headerFailure:
      sncRewindAndSeekNewMPEGSync(-1, inBuffer, inParam->pictParam->fcode_forward, &error);
     
     if (error && error != ERR_BIB_NOT_ENOUGH_DATA)
         return DGOB_ERR;
      return DGOB_OK_BUT_BIT_ERROR;
}


/* {{-output"dvpGetAndDecodeVideoPacketContents.txt"}} */
/*
 * dvpGetAndDecodeVideoPacketContents
 *    
 *
 * Parameters:
 *    inParam                    input parameters
 *    fGetNewReferenceFrame      non-zero if a new reference frame must be
 *                               requested from the image store, otherwise 0
 *    inOutParam                 input/output parameters, these parameters
 *                               may be modified in the function
 *
 * Function:
 *    This function gets and decodes the contents of a Video Packet
 *    after the header of the VP (either VP header or picture
 *    header) is already got and processed. It works MB-by-MB or if VP
 *    is data partitioned calls the corresponding decoding functions.
 *    Error concealment for the missing (not decodable) MBs in a P-frame
 *    is called.
 *
 * Returns:
 *    >= 0                       the function was successful
 *    < 0                        an error occured
 *
 */

int dvpGetAndDecodeVideoPacketContents(
   const dvpVPInParam_t *inParam,
   int fGetNewReferenceFrame,
   dvpVPInOutParam_t *inOutParam, CMPEG4Transcoder *hTranscoder)
/* {{-output"dvpGetAndDecodeVideoPacketContents.txt"}} */
{
   int16 error = 0;
   int bitErrorIndication = 0;
   dmbPFrameMBInParam_t dpmbi;
   dmbPFrameMBInOutParam_t dpmbio;
   dmbIFrameMBInParam_t dimbi;
   dmbIFrameMBInOutParam_t dimbio;
   int yPosInMBs, xPosInMBs = 0;
   bibBuffer_t *inBuffer;
   bibBuffer_t 
       *outBuffer;        /* Output bit buffer instance */
   
   bibBufferEdit_t              
       *bufEdit; 
   
   int colorEffect; 
   TBool getDecodedFrame;


   int fSegmentCorrupted = 0;

   int dmbRetValue;
   int sncCode = SNC_NO_SYNC;
   int lastMBNumInVP = 0;
   int startMB = inOutParam->currMBNum;

   SOH_DEFINE(blcDiffMB_t, pDiffMB); /* Storage for the previous difference blocks */
  
   SOH_ALLOC(blcDiffMB_t, pDiffMB);

   if (pDiffMB == NULL) {
      deb("dvpGetAndDecodeVideoPacketContents: SOH_ALLOC failed.\n");
      goto unexpectedError;
   }

   pDiffMB->cbpy = 0;

   inBuffer = inParam->inBuffer;
   outBuffer = inParam->outBuffer;
   bufEdit = inParam->bufEdit;
   colorEffect = inParam->iColorEffect;
   getDecodedFrame = inParam->iGetDecodedFrame;

   /* If the reference frame changed */
   if (fGetNewReferenceFrame) {
      vdeIms_t *store = inOutParam->imageStore;
      vdeImsItem_t *imsItem;
      vdeImb_t *imb;
      int width, height;

     if (vdeImsGetReference(store, VDEIMS_REF_LATEST, 0, &imsItem) < 0) {
        deb("dvpGetAndDecodeVideoPacketContents: ERROR - vdeImsGetReference "
           "failed.\n");
        goto unexpectedError;
     }

      /* If no reference frame available */
      if (!imsItem) {
         /* Treat the situation like a decoding error.*/
         deb("dvpGetAndDecodeVideoPacketContents: Warning - no reference frame "
            "available.\n");
         goto headerFailure;
      }

      if (vdeImsStoreItemToImageBuffer(imsItem, &imb) < 0) {
         deb("dvpGetAndDecodeVideoPacketContents: ERROR - vdeImsStoreItemToImageBuffer "
            "failed.\n");
         goto unexpectedError;
      }

      if (vdeImbYUV(imb, &inOutParam->refY, &inOutParam->refU, 
         &inOutParam->refV, &width, &height) < 0) {
         deb("dvpGetAndDecodeVideoPacketContents: ERROR - vdeImbYUV "
            "failed.\n");
         goto unexpectedError;
      }
   }

   xPosInMBs = (inOutParam->currMBNum % inParam->pictParam->numMBsInMBLine);
   yPosInMBs = (inOutParam->currMBNum / inParam->pictParam->numMBsInMBLine);

   /* if VOP header corrupted and first VP -> exit */
   if(inParam->fVOPHeaderCorrupted && inOutParam->currMBNum==0) {
      fSegmentCorrupted = 1;
      goto exitWhenVOPHeaderCorrupted;
   }

   /* in case of an I-VOP */
   if (inParam->pictParam->pictureType == VDX_VOP_TYPE_I) {
      dimbi.inBuffer = inBuffer;
      dimbi.outBuffer = outBuffer;
      dimbi.bufEdit = bufEdit;
      dimbi.iColorEffect = colorEffect;
      dimbi.iGetDecodedFrame = getDecodedFrame;

      dimbi.pictParam = inParam->pictParam;

      dimbi.xPosInMBs = xPosInMBs;
      dimbi.yPosInMBs = yPosInMBs;

      dimbio.currMBNum = inOutParam->currMBNum;
      dimbio.currMBNumInVP = 0;

     dimbio.aicData = inOutParam->aicData;

      dimbio.fCodedMBs = inOutParam->fCodedMBs;
      dimbio.numOfCodedMBs = inOutParam->numOfCodedMBs;
      dimbio.quant = inOutParam->quant;
     
      /* YUV pointers */
     {
      int32 yOffset, uvOffset;
   
      yOffset = inParam->pictParam->lumMemWidth * dimbi.yPosInMBs + dimbi.xPosInMBs;
      uvOffset = (inParam->pictParam->lumMemWidth >> 1) * dimbi.yPosInMBs + dimbi.xPosInMBs;

      if ( inOutParam->currPY != NULL )
        {
          dimbio.yMBInFrame = inOutParam->currPY + (yOffset << 4);
          dimbio.uBlockInFrame = inOutParam->currPU + (uvOffset << 3);
          dimbio.vBlockInFrame = inOutParam->currPV + (uvOffset << 3);
        }
      else
        {
          dimbio.yMBInFrame = NULL;
          dimbio.uBlockInFrame = NULL;
          dimbio.vBlockInFrame = NULL;
        }
     }
   }
   /* in case of a P-VOP */
   else {
      dpmbi.inBuffer = inBuffer;
      dpmbi.outBuffer = outBuffer;
      dpmbi.bufEdit = bufEdit;
      dpmbi.iColorEffect = colorEffect;
      dpmbi.iGetDecodedFrame = getDecodedFrame;

      dpmbi.pictParam = inParam->pictParam;

      dpmbi.xPosInMBs = xPosInMBs;
      dpmbi.yPosInMBs = yPosInMBs;

      dpmbi.refY = inOutParam->refY;
      dpmbi.refU = inOutParam->refU;
      dpmbi.refV = inOutParam->refV;
      dpmbi.currPY = inOutParam->currPY;
      dpmbi.currPU = inOutParam->currPU;
      dpmbi.currPV = inOutParam->currPV;

      dpmbio.currMBNum = inOutParam->currMBNum;
      dpmbio.currMBNumInVP = 0;

      dpmbio.fCodedMBs = inOutParam->fCodedMBs;
      dpmbio.numOfCodedMBs = inOutParam->numOfCodedMBs;
      dpmbio.quant = inOutParam->quant;

     dpmbio.aicData = inOutParam->aicData;

     dpmbio.mvcData = inOutParam->mvcData;
      dpmbio.diffMB = pDiffMB;

      /* YUV pointers */
     {
      int32 yOffset, uvOffset;
   
      yOffset = inParam->pictParam->lumMemWidth * dpmbi.yPosInMBs + dpmbi.xPosInMBs;
      uvOffset = (inParam->pictParam->lumMemWidth >> 1) * dpmbi.yPosInMBs + dpmbi.xPosInMBs;

      if ( inOutParam->currPY != NULL )
        {
          dpmbio.yMBInFrame = inOutParam->currPY + (yOffset << 4);
          dpmbio.uBlockInFrame = inOutParam->currPU + (uvOffset << 3);
          dpmbio.vBlockInFrame = inOutParam->currPV + (uvOffset << 3);
        }
      else
        {
          dimbio.yMBInFrame = NULL;
          dimbio.uBlockInFrame = NULL;
          dimbio.vBlockInFrame = NULL;
        }
     }
   }

   /* Decode multiple Macroblocks until a resync_marker is found or error occurs */
   while (inParam->pictParam->numMBsInGOB != inOutParam->currMBNum && 
         sncCode != SNC_VIDPACK && sncCode != SNC_VOP) {

       /* MVE */
       hTranscoder->BeginOneMB((inParam->pictParam->pictureType == VDX_VOP_TYPE_I) ? dimbio.currMBNum : dpmbio.currMBNum );
       
     /* decode an I-frame MB */
      if (inParam->pictParam->pictureType == VDX_VOP_TYPE_I) {
         
         if(inParam->pictParam->data_partitioned) {

            dmbRetValue = dmbsGetAndDecodeIMBsDataPartitioned(&dimbi, &dimbio,
               inOutParam->quantParams, hTranscoder);

            if (dmbRetValue < 0)
               goto unexpectedError;
            
            inOutParam->currMBNum = dimbio.currMBNum;

            /* Video Packet corrupted */
            if ( fSegmentCorrupted )
               break;

         } else {

            dmbRetValue = dmbGetAndDecodeIFrameMB(&dimbi, &dimbio, 1, hTranscoder);
            
            if (dmbRetValue < 0)
               goto unexpectedError;
            else if (dmbRetValue == DMB_BIT_ERR ) {
               /* Video Packet corrupted */
               fSegmentCorrupted = 1;
               break;
            }

            /* Store quantizer */
            inOutParam->quantParams[dimbio.currMBNum] = dimbio.quant;

            /* increment the frame pointers and MB counters */
            dimbio.currMBNum++;
            dimbio.currMBNumInVP++;

            if ( dimbio.yMBInFrame != NULL )
            {
                dimbio.yMBInFrame += 16;
                dimbio.uBlockInFrame += 8;
                dimbio.vBlockInFrame += 8;
            }
            dimbi.xPosInMBs++;

            if (dimbi.xPosInMBs == inParam->pictParam->numMBsInMBLine) {
              if ( dimbio.yMBInFrame != NULL )
                {
                   dimbio.yMBInFrame += 15 * inParam->pictParam->lumMemWidth;
                   dimbio.uBlockInFrame += 7 * (inParam->pictParam->lumMemWidth >> 1);
                   dimbio.vBlockInFrame += 7 * (inParam->pictParam->lumMemWidth >> 1);
                }
               dimbi.xPosInMBs = 0;
               dimbi.yPosInMBs++;
            }

            inOutParam->currMBNum = dimbio.currMBNum;
         }

         /* decode a P-frame MB */
     } else {

         if(inParam->pictParam->data_partitioned) {

            dmbRetValue = dmbsGetAndDecodePMBsDataPartitioned(&dpmbi, &dpmbio,
               inOutParam->quantParams, hTranscoder);

            if (dmbRetValue < 0)
               goto unexpectedError;

            inOutParam->currMBNum = dpmbio.currMBNum;
            lastMBNumInVP = dpmbio.currMBNumInVP;

            /* Video Packet corrupted */
            if ( fSegmentCorrupted )
               break;

         } else {

            dmbRetValue = dmbGetAndDecodePFrameMB(&dpmbi, &dpmbio, 1, hTranscoder);
            
            if (dmbRetValue < 0)
               goto unexpectedError;
            else if (dmbRetValue == DMB_BIT_ERR ) {
               /* Video Packet corrupted */
               fSegmentCorrupted = 1;
               break;
            }
            
            /* Store quantizer */
            inOutParam->quantParams[dpmbio.currMBNum] = dpmbio.quant;

            /* increment the frame pointers and MB counters */
            dpmbio.currMBNum++;
            dpmbio.currMBNumInVP++;

            if ( dpmbio.yMBInFrame != NULL )
              {
                dpmbio.yMBInFrame += 16;
                dpmbio.uBlockInFrame += 8;
                dpmbio.vBlockInFrame += 8;
              }
            dpmbi.xPosInMBs++;

            if (dpmbi.xPosInMBs == inParam->pictParam->numMBsInMBLine) {
                if ( dpmbio.yMBInFrame != NULL )
                  {
                   dpmbio.yMBInFrame += 15 * inParam->pictParam->lumMemWidth;
                   dpmbio.uBlockInFrame += 7 * (inParam->pictParam->lumMemWidth >> 1);
                   dpmbio.vBlockInFrame += 7 * (inParam->pictParam->lumMemWidth >> 1);
                  }
               dpmbi.xPosInMBs = 0;
               dpmbi.yPosInMBs++;
            }

            inOutParam->currMBNum = dpmbio.currMBNum;
         }
     }

     /* check for a resync_marker */
     sncCode = sncCheckMpegSync(inBuffer, inParam->pictParam->fcode_forward, &error);
     
     /* If sncCheckMpegSync failed */
     if (error && error != ERR_BIB_NOT_ENOUGH_DATA) {
         deb1p("dvpGetAndDecodeVideoPacketContents: ERROR - sncCheckSync returned %d.\n", error);
         goto unexpectedError;
         
     } else 
         /* If buffer ends (in one-frame-per-one-buffer case) */
         if (sncCode == SNC_EOB ||
             sncCode == SNC_STUFFING ||
             error == ERR_BIB_NOT_ENOUGH_DATA) {
             break; 
         }
     
   }


// <--

   inOutParam->numOfCodedMBs = (inParam->pictParam->pictureType != VDX_VOP_TYPE_I) ? 
        dpmbio.numOfCodedMBs : dimbio.numOfCodedMBs;
   
   if (fSegmentCorrupted) {
      u_int32 nextVPBitPos;

      /* find the next resync marker, to get the number of MBs in the current VP */
      sncCode = sncRewindAndSeekNewMPEGSync(-1, inBuffer, inParam->pictParam->fcode_forward, &error);
      if (error && error != ERR_BIB_NOT_ENOUGH_DATA) {
         goto unexpectedError;
      } else if (sncCode == SNC_EOB) {
         goto exitFunction;
      }

      nextVPBitPos = bibNumberOfFlushedBits(inParam->inBuffer);

      /* if the lastMBNumInVP was not yet read */
      if (inParam->pictParam->pictureType == VDX_VOP_TYPE_I || 
         !inParam->pictParam->data_partitioned || 
         lastMBNumInVP == 0) { 

         /* if VP header is found: read it, and get last MB number in current vop */
         if (sncCode == SNC_VIDPACK) {
            vdxVideoPacketHeader_t header;
            vdxGetVideoPacketHeaderInputParam_t vdxParam;
            int retValue;
            
            vdxParam.fcode_forward = inParam->pictParam->fcode_forward;
            vdxParam.time_increment_resolution = inParam->pictParam->time_increment_resolution;
            vdxParam.numOfMBs = inParam->pictParam->numMBsInGOB;
            
            retValue = vdxGetVideoPacketHeader(inParam->inBuffer, &vdxParam, &header, &bitErrorIndication);
            if (retValue < 0) {
               goto unexpectedError;
            } else if (retValue == VDX_OK_BUT_BIT_ERROR) {
               /* If bit error occurred */
               goto headerFailure;
            }
            
            /* rewind the bits to the beginning of the VP header */
            bibRewindBits(bibNumberOfFlushedBits(inParam->inBuffer) - nextVPBitPos,
               inParam->inBuffer, &error);
            
            lastMBNumInVP = header.currMBNum;
            
         } else {
            lastMBNumInVP = inParam->pictParam->numMBsInGOB;
         }
      }

      /* if possibly the next VP header is damaged (it gives MB index which is smaller than the start of the current one */
      if (lastMBNumInVP <= startMB || lastMBNumInVP > inParam->pictParam->numMBsInGOB)
         goto exitFunction;

      /* if the MB counting due to error overran the next VP header's value */
      if (inOutParam->currMBNum > lastMBNumInVP)
         inOutParam->currMBNum = VDC_MAX(lastMBNumInVP-3,startMB);
      
      /* if not all the MBs have been predicted due to error */
      else if (inOutParam->currMBNum < lastMBNumInVP)
         inOutParam->currMBNum = VDC_MAX(inOutParam->currMBNum-1,startMB);

      
      inOutParam->currMBNum = lastMBNumInVP;
   }

   exitFunction:


   exitWhenVOPHeaderCorrupted:

   if (!fSegmentCorrupted) {
      SOH_DEALLOC(pDiffMB);
      return DGOB_OK;
   }
   else {
      SOH_DEALLOC(pDiffMB);
      return DGOB_OK_BUT_BIT_ERROR;
   }

   headerFailure:
      SOH_DEALLOC(pDiffMB);
      sncRewindAndSeekNewMPEGSync(-1, inBuffer, 0, &error);
      if (error && error != ERR_BIB_NOT_ENOUGH_DATA)
         return DGOB_ERR;
      return DGOB_OK_BUT_BIT_ERROR;

   unexpectedError:
      SOH_DEALLOC(pDiffMB);
      return DGOB_ERR;
}
// End of File
