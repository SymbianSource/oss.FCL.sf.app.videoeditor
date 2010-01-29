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
* GOB decoding functions.
*
*/


/* 
 * Includes 
 */
#include "h263dConfig.h"
#include "decgob.h"
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

/* {{-output"dgobGetAndDecodeGOBSegment.txt"}} */
/*
 * dgobGetAndDecodeGOBSegment
 *    
 *
 * Parameters:
 *    inParam                    input parameters
 *    inOutParam                 input/output parameters, these parameters
 *                               may be modified in the function
 *
 * Function:
 *    This function gets and decodes a GOB segment which should start
 *    with a GBSC at the current position of the bit buffer.
 *
 * Returns:
 *    >= 0                       the function was successful
 *    < 0                        an error occured
 *
 */

int dgobGetAndDecodeGOBSegment(
   const dgobGOBSegmentInParam_t *inParam,
   dgobGOBSegmentInOutParam_t *inOutParam,
   CMPEG4Transcoder *hTranscoder)
/* {{-output"dgobGetAndDecodeGOBSegment.txt"}} */
{
   bibBuffer_t 
      *inBuffer;        /* Input bit buffer instance */

   int 
      retValue,         /* Value returned from this function */
      
      fGetNewReferenceFrame,
                        /* 1 if the reference picture has changed from
                           the previous one */
      
      bitErrorIndication = 0, 
                        /* Carries bit error indication information returned
                           by the video demultiplexer module */
      
      intraGobsMissing = 0;
                        /* Flag to indicate if INTRA coded GOBs are missing */
      
   u_int32
      segmStart = bibNumberOfFlushedBits( inParam->inBuffer );
                        /* Start bit buffer position of the GOB segment */

   vdxGetGOBHeaderInputParam_t 
      vdxParam;         /* Input parameters for vdxGetGOBHeader */

   vdxGOBHeader_t 
      header;           /* GOB header data */

   inBuffer = inParam->inBuffer;

   /* 
    * Get GOB header 
    */

   vdxParam.numStuffBits = inParam->numStuffBits;
   vdxParam.fCustomPCF = inParam->pictParam->fCustomPCF;
   vdxParam.fCPM = inParam->pictParam->cpm;
   vdxParam.fRPS = inParam->pictParam->fRPS;

   if (vdxGetGOBHeader(inBuffer, &vdxParam, &header, &bitErrorIndication, 
       inParam->iColorEffect, &inOutParam->StartByteIndex, &inOutParam->StartBitIndex, hTranscoder) < 0) {

      deb("dgobGetAndDecodeGOBSegment: ERROR - vdxGetGOBHeader failed.\n");
      goto unexpectedError;
   }

   /*
    * Check header validity
    */

   if (header.gn >= inParam->pictParam->numGOBs) {
      deb("dgobGetAndDecodeGOBSegment: ERROR - too big GN.\n");
      goto unexpectedError;
   }

   /* If TRP present and TRP is not 8-bit if only 8-bit TRs have existed.
      Note: The following condition assumes that only 8-bit TRs are allowed. */
   if (header.trpi && header.trp > 255) {
      deb("dgobGetAndDecodeGOBSegment: ERROR - too big TRP.\n");
      goto unexpectedError;
   }

   /* If GFID is not as expected */
   if (inOutParam->gfid >= 0 && bitErrorIndication != 0 && 
      ((inParam->fGFIDShouldChange && inOutParam->gfid == header.gfid) ||
      (!inParam->fGFIDShouldChange && inOutParam->gfid != header.gfid))) {
      deb("dgobGetAndDecodeGOBSegment: ERROR - illegal GFID.\n");
      goto unexpectedError;
   }

   inOutParam->prevGN = inOutParam->prevGNWithHeader = header.gn;
   /* GFID was valid, and in the next GOB the gfidShouldChange flag should be 0 and GFID should be
      the same as the current one */
   inOutParam->gfid = header.gfid;

   fGetNewReferenceFrame = 0;
   if (inParam->pictParam->fRPS) {
      /* If TRP has changed */
      if ((header.trpi && header.trp != inOutParam->trp) ||
         (!header.trpi && inOutParam->trp >= 0))
         fGetNewReferenceFrame = 1;

      if (header.trpi)
         inOutParam->trp = header.trp;
      else
         inOutParam->trp = -1;
   }

  /* MVE */
     hTranscoder->H263GOBSliceHeaderEnded(&header, NULL);
   

   /*
    * Decode GOB contents
    */

   retValue = dgobGetAndDecodeGOBSegmentContents(inParam, fGetNewReferenceFrame, 
      header.gquant, inOutParam, hTranscoder);

  /* MVE */
    hTranscoder->H263OneGOBSliceWithHeaderEnded();


   if ( intraGobsMissing && retValue == 0 )
      return DGOB_OK_BUT_BIT_ERROR;
   else
      return retValue;

   unexpectedError:
      return DGOB_ERR;
}


/* {{-output"dgobGetAndDecodeGOBSegmentContents.txt"}} */
/*
 * dgobGetAndDecodeGOBSegmentContents
 *    
 *
 * Parameters:
 *    inParam                    input parameters
 *    fGetNewReferenceFrame      non-zero if a new reference frame must be
 *                               requested from the image store, otherwise 0
 *    quant                      initial quantization parameter
 *    inOutParam                 input/output parameters, these parameters
 *                               may be modified in the function
 *
 * Function:
 *    This function gets and decodes the contents of a GOB segment
 *    meaning that the header of the GOB (either GOB header or picture
 *    header) is already got and processed and the macroblocks belonging
 *    to the GOB segment are decoded.
 *
 * Returns:
 *    >= 0                       the function was successful
 *    < 0                        an error occured
 *
 */

int dgobGetAndDecodeGOBSegmentContents(
   const dgobGOBSegmentInParam_t *inParam,
   int fGetNewReferenceFrame,
   int quant,
   dgobGOBSegmentInOutParam_t *inOutParam,
   CMPEG4Transcoder *hTranscoder)
/* {{-output"dgobGetAndDecodeGOBSegmentContents.txt"}} */
{
   bibBuffer_t 
      *inBuffer;        /* Input bit buffer instance */


   bibBuffer_t 
       *outBuffer;        /* Output bit buffer instance */
   
   bibBufferEdit_t              
       *bufEdit; 
   
   int colorEffect; 
   TBool getDecodedFrame;


   /* decmbs input and output parameters */
   dmbPFrameMBInParam_t dpmbi;
   dmbPFrameMBInOutParam_t dpmbio;
   dmbIFrameMBInParam_t dimbi;
   dmbIFrameMBInOutParam_t dimbio;

   int 
      *pYPosInMBs,      /* Pointer to variable containing the y-position
                           of the current macroblock in macroblocks
                           (starting from zero in the top row) */
      fSegmentCorrupted = 0;
                        /* Flag to indicate if the current GOB segment
                           is corrupted */

   int16 
      error = 0;        /* Used to pass error codes from snc and erd modules */

   /* Pointers to pointers pointing to the top-left corner of the current 
      GOB (inside the current frame) */
   u_char **pYGOB, **pUGOB, **pVGOB;

   SOH_DEFINE(blcDiffMB_t, pDiffMB);
                        /* Storage for the previous difference blocks */

   inBuffer = inParam->inBuffer;

   outBuffer = inParam->outBuffer;
   bufEdit = inParam->bufEdit;
   colorEffect = inParam->iColorEffect;
   getDecodedFrame = inParam->iGetDecodedFrame;


   SOH_ALLOC(blcDiffMB_t, pDiffMB);

   if (pDiffMB == NULL) {
      deb("dgobGetAndDecodeGOBSegmentContents: SOH_ALLOC failed.\n");
      goto unexpectedError;
   }

   pDiffMB->cbpy = 0;

   /* If the reference frame changed */
   if (fGetNewReferenceFrame) {
      vdeIms_t *store = inOutParam->imageStore;
      vdeImsItem_t *imsItem;
      vdeImb_t *imb;
      int width, height;

      /* Get the reference frame */
      if (inOutParam->trp >= 0) {
         if (vdeImsGetReference(store, VDEIMS_REF_TR, inOutParam->trp, &imsItem) < 0) {
            deb("dgobGetAndDecodeGOBSegment: ERROR - vdeImsGetReference "
               "failed.\n");
            goto unexpectedError;
         }
      }

      else {
         if (vdeImsGetReference(store, VDEIMS_REF_LATEST, 0, &imsItem) < 0) {
            deb("dgobGetAndDecodeGOBSegment: ERROR - vdeImsGetReference "
               "failed.\n");
            goto unexpectedError;
         }
      }

      /* If no reference frame available */
      if (!imsItem) {

         /* Treat the situation like a decoding error.
            This should cause error concealment and
            a NACK message if Annex N is used. */
         deb("dgobGetAndDecodeGOBSegment: Warning - no reference frame "
            "available.\n");

         goto unexpectedError;
      }

      if (vdeImsStoreItemToImageBuffer(imsItem, &imb) < 0) {
         deb("dgobGetAndDecodeGOBSegment: ERROR - vdeImsStoreItemToImageBuffer "
            "failed.\n");
         goto unexpectedError;
      }

      inOutParam->rtr = imb->tr;

      if (vdeImbYUV(imb, &inOutParam->refY, &inOutParam->refU, 
         &inOutParam->refV, &width, &height) < 0) {
         deb("dgobGetAndDecodeGOBSegment: ERROR - vdeImbYUV "
            "failed.\n");
         goto unexpectedError;
      }
   }

   /* Preset structures for multiple macroblock decoding */
   if (inParam->pictParam->pictureType == VDX_PIC_TYPE_I) {
      dimbi.inBuffer = inBuffer;


      dimbi.outBuffer = outBuffer;
      dimbi.bufEdit = bufEdit;
      dimbi.iColorEffect = colorEffect; 
      dimbi.iGetDecodedFrame = getDecodedFrame; 
      dimbio.StartByteIndex = inOutParam->StartByteIndex;
      dimbio.StartBitIndex = inOutParam->StartBitIndex;


      dimbi.xPosInMBs = 0;
      /* yPosInMBs set inside the loop (below) */
      dimbi.pictParam = inParam->pictParam;
      dimbi.fGOBHeaderPresent = 1;

      dimbio.fCodedMBs = inOutParam->fCodedMBs;
      dimbio.numOfCodedMBs = inOutParam->numOfCodedMBs;
      dimbio.quant = quant;

      /* YUV pointers set iside the loop (below) */

      pYPosInMBs = &dimbi.yPosInMBs;
      pYGOB = &dimbio.yMBInFrame;
      pUGOB = &dimbio.uBlockInFrame;
      pVGOB = &dimbio.vBlockInFrame;
   }

   else {
      dpmbi.inBuffer = inBuffer;

      
      dpmbi.outBuffer = outBuffer;
      dpmbi.bufEdit = bufEdit;
      dpmbi.iColorEffect = colorEffect; 
      dpmbi.iGetDecodedFrame = getDecodedFrame;
      dpmbio.StartByteIndex = inOutParam->StartByteIndex;
      dpmbio.StartBitIndex = inOutParam->StartBitIndex;


      dpmbi.xPosInMBs = 0;
      /* yPosInMBs set inside the loop (below) */
      dpmbi.pictParam = inParam->pictParam;
      dpmbi.fGOBHeaderPresent = 1;
      dpmbi.refY = inOutParam->refY;
      dpmbi.refU = inOutParam->refU;
      dpmbi.refV = inOutParam->refV;
      dpmbi.currPY = inOutParam->currPY;
      dpmbi.currPU = inOutParam->currPU;
      dpmbi.currPV = inOutParam->currPV;

      dpmbio.fCodedMBs = inOutParam->fCodedMBs;
      dpmbio.numOfCodedMBs = inOutParam->numOfCodedMBs;
      dpmbio.quant = quant;

      /* YUV pointers set iside the loop (below) */
      dpmbio.mvcData = inOutParam->mvcData;
      dpmbio.diffMB = pDiffMB;

      pYPosInMBs = &dpmbi.yPosInMBs;
      pYGOB = &dpmbio.yMBInFrame;
      pUGOB = &dpmbio.uBlockInFrame;
      pVGOB = &dpmbio.vBlockInFrame;
   }

   /* Loop forever (until the GOB segment ends) */
   for (;;) {
      int dmbsRetValue;
      int numMBsInCurrGOB;
      int sncCode;
      int numStuffBits;
      int mbNumberInScanOrder;

      if ((inOutParam->prevGN == inParam->pictParam->numGOBs - 1) && 
         (inParam->pictParam->fLastGOBSizeDifferent))
         numMBsInCurrGOB = inParam->pictParam->numMBsInLastGOB;
      else
         numMBsInCurrGOB = inParam->pictParam->numMBsInGOB;

      *pYPosInMBs = inOutParam->prevGN * inParam->pictParam->numMBLinesInGOB;

      mbNumberInScanOrder = *pYPosInMBs * inParam->pictParam->numMBsInMBLine;
      
      /* Set pointers for the GOB (inside frame(s)) */
      if ( inOutParam->currPY != NULL )
      {
         int32 yOffset, uvOffset;

         yOffset = *pYPosInMBs * 16 * inParam->pictParam->lumMemWidth;
            
         *pYGOB = inOutParam->currPY + yOffset;

         uvOffset = yOffset / 4;
         *pUGOB = inOutParam->currPU + uvOffset;
         *pVGOB = inOutParam->currPV + uvOffset;

      }
      else
        {
        *pYGOB = NULL;
        *pUGOB = NULL;
        *pVGOB = NULL;
        }

      /* Decode macroblocks of the current GOB */
      if (inParam->pictParam->pictureType == VDX_PIC_TYPE_I)   {
         dimbi.numMBsInSegment = numMBsInCurrGOB;
         dmbsRetValue = dmbsGetAndDecodeIMBsInScanOrder(&dimbi, 
            &dimbio, &inOutParam->quantParams[mbNumberInScanOrder], hTranscoder);
         
         inOutParam->StartByteIndex = dimbio.StartByteIndex;
         inOutParam->StartBitIndex  = dimbio.StartBitIndex;


      }
      else  {
         dpmbi.numMBsInSegment = numMBsInCurrGOB;
         dmbsRetValue = dmbsGetAndDecodePMBsInScanOrder(&dpmbi,
            &dpmbio, &inOutParam->quantParams[mbNumberInScanOrder], hTranscoder);
         
         inOutParam->StartByteIndex = dpmbio.StartByteIndex;
         inOutParam->StartBitIndex  = dpmbio.StartBitIndex;
      }

      if (dmbsRetValue < 0)
         goto unexpectedError;

      /* Some bit errors were found inside the segment 
         (dpmbi/dimbi fSegmentCorrupted have the same address as fSegmentCorrupted)
         Note that if no suspicious/corrupted blocks was found, but there was crc-error, 
         this flag is not set. However, that case will be checked if no sync code is found */
      if ( fSegmentCorrupted )
         break;

      /* Check if there is a synchronization code in the current bitstream
         position */
      sncCode = sncCheckSync(inBuffer, &numStuffBits, &error);

      /* If buffer ends (in one-frame-per-one-buffer case) */
      if (error == ERR_BIB_NOT_ENOUGH_DATA)
         break;

      if (error)
         goto unexpectedError;

      /* If there is a synchronization code */
      if (sncCode != SNC_NO_SYNC )
         break;


      if (inOutParam->prevGN + 1 < inParam->pictParam->numGOBs) 
         inOutParam->prevGN++;

      else {
         deb("dgobGetAndDecodeGOBSegment: ERROR - too much frame data.\n");
         fSegmentCorrupted = 1;
         break;
      }

      dpmbi.fGOBHeaderPresent = 0;
      dimbi.fGOBHeaderPresent = 0;
   }

   /* Update coded macroblock counter */
   if (inParam->pictParam->pictureType == VDX_PIC_TYPE_I)
      inOutParam->numOfCodedMBs = dimbio.numOfCodedMBs;
   else
      inOutParam->numOfCodedMBs = dpmbio.numOfCodedMBs;

   if (!fSegmentCorrupted) {
      SOH_DEALLOC(pDiffMB);
      if (inOutParam->trp >= 0) {
      }

      return DGOB_OK;
   }
   else {

      return DGOB_OK_BUT_BIT_ERROR;
   }

   unexpectedError:
      SOH_DEALLOC(pDiffMB);
      return DGOB_ERR;
}


// End of File
