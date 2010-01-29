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
* MB decoding in data partitioned mode (MPEG-4).
*
*/



/*
 * Includes 
 */
#include "h263dConfig.h"
#include "decmbs.h"
#include "decmbdct.h"
#include "viddemux.h"
#include "errcodes.h"
#include "sync.h"
#include "mpegcons.h"
#include "debug.h"
/* MVE */
#include "MPEG4Transcoder.h"

/*
 * Local functions
 */


/*
 * Global functions
 */

/* {{-output"dmbsGetAndDecodeIMBsDataPartitioned.txt"}} */
/*
 * dmbsGetAndDecodeIMBsDataPartitioned
 *    
 *
 * Parameters:
 *    inParam                    input parameters
 *    inOutParam                 input/output parameters, these parameters
 *                               may be modified in the function
 *
 * Function:
 *    This function gets and decodes the MBs of a data partitioned 
 *    Video Packet in an Intra Frame.
 *
 * Returns:
 *    >= 0                       the function was successful
 *    < 0                        an error occured
 *
 */

int dmbsGetAndDecodeIMBsDataPartitioned(
   dmbIFrameMBInParam_t *inParam,
   dmbIFrameMBInOutParam_t *inOutParam,
   int *quantParams, CMPEG4Transcoder *hTranscoder)
/* {{-output"dmbsGetAndDecodeIMBsDataPartitioned.txt"}} */
{
   int retValue = DMBS_OK;

   int currMBNumInVP, lastMBNum, numMBsInVP, numCorrectMBs;
   int yWidth = inParam->pictParam->lumMemWidth;
   int uvWidth = yWidth / 2;
   int bitErrorIndication = 0, ret = 0, sncCode, bitsGot, bitErrorsInPart1 = 0;
   u_int32 startVPBitPos = 0, 
         errorBitPos = 0,
         backwards_errorBitPos = 0,
         nextVPBitPos = 0,
         startBlockDataBitPos = 0,
         DCMarkerBitPos = 0;
   u_char fPart1Error=0, fPart2Error=0, fBlockError=0;
   int16 error = 0;
#ifdef DEBUG_OUTPUT
   FILE *rvlc_stat;
#endif

   dlst_t MBList;
   vdxIMBListItem_t *MBinstance;

   vdxGetDataPartitionedIMBLayerInputParam_t vdxDPIn;
   dmdMPEGIParam_t dmdIn;
   
   if (dlstOpen(&MBList) < 0)
      return DMBS_ERR;

   /* mark the bit position at the beginning of the VP */
   startVPBitPos = bibNumberOfFlushedBits(inParam->inBuffer);

   /* 
    * read the first partition: DC coefficients, etc. 
    */

   vdxDPIn.intra_dc_vlc_thr = inParam->pictParam->intra_dc_vlc_thr;
   vdxDPIn.quant = inOutParam->quant;

   /* MVE */
   ret = vdxGetDataPartitionedIMBLayer_Part1(inParam->inBuffer, inParam->outBuffer, 
         inParam->bufEdit, inParam->iColorEffect, &(inOutParam->StartByteIndex), 
         &(inOutParam->StartBitIndex), hTranscoder, &vdxDPIn, &MBList, 
         &bitErrorIndication);

   bitErrorsInPart1 = bitErrorIndication;
   if (ret < 0) {
       retValue = DMBS_ERR;
       goto exitFunction;
   }
   else if (ret == VDX_OK_BUT_BIT_ERROR) {

      fPart1Error = 1;
      bitErrorIndication = 0;
      deb("dmbsGetAndDecodeIMBsDataPartitioned:ERROR - DC partition error.\n");
   } else {
      DCMarkerBitPos = bibNumberOfFlushedBits(inParam->inBuffer);
   }

   /* read the next VP header to determine the number of MBs in this VP */
   if ( fPart1Error ) {
       /* Stop decoding this segment */
      goto exitFunction;
   }
   else {
      sncCode = sncRewindAndSeekNewMPEGSync( 1,
          inParam->inBuffer, inParam->pictParam->fcode_forward, &error);
   }
   if (error) {
      if (error == ERR_BIB_NOT_ENOUGH_DATA) error = 0;
      else { 
          retValue = DMBS_ERR;
          goto exitFunction;          
      }
   }
   
   nextVPBitPos = bibNumberOfFlushedBits(inParam->inBuffer);
   
   if (sncCode == SNC_VIDPACK) {
      
      vdxVideoPacketHeader_t header;
      vdxGetVideoPacketHeaderInputParam_t vdxParam;
      int retValue;
      
      vdxParam.fcode_forward = inParam->pictParam->fcode_forward;
      vdxParam.time_increment_resolution = inParam->pictParam->time_increment_resolution;
      vdxParam.numOfMBs = inParam->pictParam->numMBsInGOB;
      
      retValue = vdxGetVideoPacketHeader(inParam->inBuffer, &vdxParam, &header, &bitErrorIndication);
      if (retValue < 0) {
          retValue = DMBS_ERR;
          goto exitFunction;      
      } else if (retValue == VDX_OK_BUT_BIT_ERROR) {
         /* If bit error occurred */
         deb("dmbsGetAndDecodeIMBsDataPartitioned:ERROR - VP Header error.\n");
         bitErrorIndication = 0;
         lastMBNum = 0;
      } else {    
         lastMBNum = header.currMBNum;
      }
   } else {
      lastMBNum = inParam->pictParam->numMBsInGOB;
   }

   /* rewind the bits to the beginning of the current VP data */
   bibRewindBits(bibNumberOfFlushedBits(inParam->inBuffer) - (fPart1Error ? errorBitPos : DCMarkerBitPos),
      inParam->inBuffer, &error);
      
   if (fPart1Error) {
      /* Seek the DC_MARKER */
      if ((sncSeekBitPattern(inParam->inBuffer, MP4_DC_MARKER, MP4_DC_MARKER_LENGTH, &error) != SNC_PATTERN) ||
         (bibNumberOfFlushedBits(inParam->inBuffer) >= nextVPBitPos)) {
         
         /* rewind the bits to the beginning of the current VP data */
         bibRewindBits( VDC_MIN(bibNumberOfRewBits(inParam->inBuffer), (bibNumberOfFlushedBits(inParam->inBuffer) - startVPBitPos)),
            inParam->inBuffer, &error);

         deb("dmbsGetAndDecodeIMBsDataPartitioned:ERROR - Part1Error && No DC marker found.\n");
         goto exitFunction;
      }
   } 

   if ((lastMBNum <= inOutParam->currMBNum) ||
      (lastMBNum > inParam->pictParam->numMBsInGOB) ||
      (sncCode == SNC_EOB)) {
      numMBsInVP = MBList.numItems;
      if (fPart1Error) fPart2Error = 1;
   } else 
      numMBsInVP = lastMBNum - inOutParam->currMBNum;

   if (numMBsInVP != MBList.numItems || fPart1Error) {
      deb("dmbsGetAndDecodePMBsDataPartitioned:ERROR - MB list length != num MBs.\n");
      if ( fPart1Error || bitErrorsInPart1 ) {
         /* Discard few MBs from the end of the list, 
            since there are errors somewhere in the bitstream. 
            If the list is short due to the missing packet, 
            all the read MBs are likely to be OK and there is no
            need to discard them */
         dlstTail(&MBList, (void **) &MBinstance);
         dlstRemove(&MBList, (void **) &MBinstance);
         free( MBinstance );
         if (numMBsInVP > MBList.numItems ) {
            /* Take still one away */
            dlstTail(&MBList, (void **) &MBinstance);
            dlstRemove(&MBList, (void **) &MBinstance);
            free( MBinstance );
         }
      }
      if (numMBsInVP < MBList.numItems ) {
         /* Discard all the extra MBs from the end of the list + 2 just to be sure, 
            since there are errors somewhere in the bitstream */
         while ( MBList.numItems > VDC_MAX(numMBsInVP-2,0) ) {
            dlstTail(&MBList, (void **) &MBinstance);
            dlstRemove(&MBList, (void **) &MBinstance);
            free( MBinstance );
         }
      }
      fPart1Error = 1;
      errorBitPos = bibNumberOfFlushedBits(inParam->inBuffer);
   }

   /* Flush the DC_Marker */
   bibFlushBits(MP4_DC_MARKER_LENGTH, inParam->inBuffer, &bitsGot, &bitErrorIndication, &error);
   if (error)
   {
       retValue = DMBS_ERR;
       goto exitFunction;        
   }

   /*
    * Read the second partition header: cpby, ac_pred_flag 
    */

   ret = vdxGetDataPartitionedIMBLayer_Part2(inParam->inBuffer, inParam->outBuffer, 
         inParam->bufEdit, inParam->iColorEffect, &(inOutParam->StartByteIndex), &(inOutParam->StartBitIndex),
         &MBList, numMBsInVP, &bitErrorIndication);
   if (ret < 0) {
      retValue = DMBS_ERR;
      goto exitFunction;
   }
   else if (ret == VDX_OK_BUT_BIT_ERROR) {
        fPart2Error = 1;
        deb("dmbsGetAndDecodeIMBsDataPartitioned:ERROR - Part2Error.\n");
        /* Stop decoding this segment */
        goto exitFunction;
   }

   /* common input parameters for all blocks */
   dmdIn.aicData = inOutParam->aicData;

   /* MVE */
   dmdIn.inBuffer = inParam->inBuffer;
   dmdIn.outBuffer = inParam->outBuffer;
   dmdIn.bufEdit = inParam->bufEdit;
   dmdIn.iColorEffect = inParam->iColorEffect;
   dmdIn.iGetDecodedFrame = inParam->iGetDecodedFrame;

   dmdIn.yWidth = inParam->pictParam->lumMemWidth;
   dmdIn.numMBsInMBLine = inParam->pictParam->numMBsInMBLine;
   dmdIn.pictureType = inParam->pictParam->pictureType;

   /*
    * Read block data partition in forward direction 
    */

   startBlockDataBitPos = bibNumberOfFlushedBits(inParam->inBuffer);

   /* set block pointers to the beginning of the VP */   
   dmdIn.yMBInFrame = inOutParam->yMBInFrame;
   dmdIn.uBlockInFrame = inOutParam->uBlockInFrame;
   dmdIn.vBlockInFrame = inOutParam->vBlockInFrame;

   dmdIn.xPosInMBs = inParam->xPosInMBs;
   dmdIn.yPosInMBs = inParam->yPosInMBs;

   dmdIn.currMBNum = inOutParam->currMBNum;

   /* get the first MB of the list */
   dlstHead(&MBList, (void **) &MBinstance);

   for (currMBNumInVP = 0; currMBNumInVP < numMBsInVP; currMBNumInVP++) {
         
         /* if MBList is shorter then the number of MBs in the VP */
         if (MBinstance == NULL) {
             deb("dmbsGetAndDecodeIMBsDataPartitioned:ERROR - MB list < num MBs.\n");
             goto exitFunction;
         }
         
         /* header params. If partition 2 or AC partition contains errors, no bits are read any more 
         from the bitstream, but the blocks are reconstructed based only on DC values */
         dmdIn.cbpy = (fPart2Error || fBlockError) ? 0 : MBinstance->cbpy;
         dmdIn.cbpc = (fPart2Error || fBlockError) ? 0 : MBinstance->cbpc;
         dmdIn.quant = MBinstance->quant;
         
         /* AC/DC prediction params */
         dmdIn.switched = MBinstance->switched;
         
         dmdIn.fTopOfVP = (u_char) (currMBNumInVP < inParam->pictParam->numMBsInMBLine);
         dmdIn.fLeftOfVP = (u_char) (currMBNumInVP == 0);
         dmdIn.fBBlockOut = (u_char) (currMBNumInVP <= inParam->pictParam->numMBsInMBLine);
         
         inOutParam->aicData->ACpred_flag = (u_char) ((fPart2Error || fBlockError) ? 0 : MBinstance->ac_pred_flag);
         
         /* error resilience params */
         dmdIn.data_partitioned = 1;
         dmdIn.DC = MBinstance->DC;
         
         dmdIn.reversible_vlc = inParam->pictParam->reversible_vlc;
         dmdIn.vlc_dec_direction = 0;
         
         /* MVE */
         hTranscoder->OneIMBDataStartedDataPartitioned(MBinstance, &MBList, currMBNumInVP, dmdIn.currMBNum);
         
         /* get the next macroblock data */
         ret = dmdGetAndDecodeMPEGIMBBlocks(&dmdIn, hTranscoder);
         
         if ( ret < 0 ) {
             retValue = DMBS_ERR;
             goto exitFunction;         
         }
      else if ( ret == DMD_BIT_ERR ) {
         if (fPart1Error) {
            deb("dmbsGetAndDecodeIMBsDataPartitioned:ERROR - IMB Blocks decoding error && Part1Error.\n");
            goto exitFunction;
         } else {
            deb("dmbsGetAndDecodeIMBsDataPartitioned:ERROR - IMB Blocks decoding error. Using DCs only.\n");
            fBlockError = 1;
         }
      }

      /* Store quantizer */
      quantParams[dmdIn.currMBNum] = MBinstance->quant;

      /* increment the block pointers and counters */
      dmdIn.currMBNum++;
      if ( dmdIn.yMBInFrame != NULL )
        {
            dmdIn.yMBInFrame += 16;
            dmdIn.uBlockInFrame += 8;
            dmdIn.vBlockInFrame += 8;
        }
      dmdIn.xPosInMBs++;

      if (dmdIn.xPosInMBs == inParam->pictParam->numMBsInMBLine) {
         if ( dmdIn.yMBInFrame )
            {
             dmdIn.yMBInFrame += 15 * yWidth;
             dmdIn.uBlockInFrame += 7 * uvWidth;
             dmdIn.vBlockInFrame += 7 * uvWidth;
            }
         dmdIn.xPosInMBs = 0;
         dmdIn.yPosInMBs++;
         if (dmdIn.yPosInMBs >= inParam->pictParam->numMBLinesInGOB)
            break;
      }
      
      dlstNext(&MBList, (void **) &MBinstance);
   }

   if (!fPart1Error && !fPart2Error && !fBlockError) {
         if (sncCode == SNC_EOB) {
             inOutParam->currMBNum += numMBsInVP;
             goto exitFunction;
         } else {
             
             sncCode = sncCheckMpegSync(inParam->inBuffer, inParam->pictParam->fcode_forward, &error);
             if (sncCode == SNC_NO_SYNC) {
                 deb("dmbsGetAndDecodeIMBsDataPartitioned:ERROR - After block data no start code found.\n");
                 if (lastMBNum != 0) 
                     fBlockError = 1;
             }
         }
   }

   /*
    * In case of error, read block data partition in backward direction 
    */

   /* WARNING: backwards decoding of INTRA frame RVLC is disabled by setting the VP size
      higher then 10. The useful VP sizes used in low bitrates allow max. 8-10 MBs per
     VP, but in average 3-5 MBs. With this small amount the backwards decoding always
     causes overlap of the MB counter, and the overlapped MBs must be discarded. So the
     RVLC doesn't have any use. */

   if (!fPart2Error && fBlockError && inParam->pictParam->reversible_vlc && (numMBsInVP >= 10)) {
      numCorrectMBs = currMBNumInVP;

#ifdef DEBUG_OUTPUT
      {
         int bitPos[6], xpos, ypos, mbnum;
         
         rvlc_stat = fopen("rvlc.log", "a+t");
         
         fprintf(rvlc_stat, "I-VOP: (frame)(MB_first):%3d  (MB_last):%3d\n", 
            inOutParam->currMBNum, (inOutParam->currMBNum + numMBsInVP-1));
         
         for (xpos = inParam->xPosInMBs, ypos = inParam->yPosInMBs, mbnum = 0; mbnum < numCorrectMBs; mbnum++, xpos++) {
            
            if (xpos / inParam->pictParam->numMBsInMBLine)
               xpos = 0, ypos++;

            fprintf(rvlc_stat, "fw: MB#%3d\tY0: %8d | Y1: %8d | Y2: %8d | Y3: %8d | U: %8d | V: %8d\n", 
               inOutParam->currMBNum+mbnum, bitPos[0], bitPos[1], bitPos[2], bitPos[3], bitPos[4], bitPos[5]); 
         }
      }
#endif

         /* find next VP header (end of MB block data of this VP) */
      sncCode = sncRewindAndSeekNewMPEGSync(errorBitPos-startBlockDataBitPos, inParam->inBuffer,
         inParam->pictParam->fcode_forward, &error);      
      if (error) {
          if (error == ERR_BIB_NOT_ENOUGH_DATA) error = 0;
          else { 
              retValue = DMBS_ERR;
              goto exitFunction;              
          }
      }

      if (sncCode == SNC_EOB) {
         inOutParam->currMBNum += numMBsInVP;
         goto exitFunction;
      }

      nextVPBitPos = bibNumberOfFlushedBits(inParam->inBuffer);

      backwards_errorBitPos = startBlockDataBitPos;

      /* rewind the stuffing bits */
      if (sncCode != SNC_NO_SYNC || !(nextVPBitPos % 8)) {
         if(sncRewindStuffing(inParam->inBuffer, &error) != SNC_PATTERN) {
            deb("dmbsGetAndDecodeIMBsDataPartitioned:ERROR - Backwards decoding, stuffing not found.\n");
            inOutParam->currMBNum = dmdIn.currMBNum;
            goto exitFunction;
         }
      }

      /* set the block pointers and counters to the end of the VP */
      if ( dmdIn.yMBInFrame != NULL )
        {
          dmdIn.yMBInFrame = inOutParam->yMBInFrame + 16 * (numMBsInVP-1);
          dmdIn.uBlockInFrame = inOutParam->uBlockInFrame + 8 * (numMBsInVP-1);
          dmdIn.vBlockInFrame = inOutParam->vBlockInFrame + 8 * (numMBsInVP-1);
        }
      else
        {
            dmdIn.yMBInFrame = dmdIn.uBlockInFrame = dmdIn.vBlockInFrame = NULL;
        }
      dmdIn.xPosInMBs = inParam->xPosInMBs + (numMBsInVP-1);
      dmdIn.yPosInMBs = inParam->yPosInMBs;

      if (dmdIn.xPosInMBs / inParam->pictParam->numMBsInMBLine) {

         int numFullLines = dmdIn.xPosInMBs / inParam->pictParam->numMBsInMBLine;

         if ( dmdIn.yMBInFrame != NULL )
            {
                dmdIn.yMBInFrame += 15 * yWidth * numFullLines;
                dmdIn.uBlockInFrame += 7 * uvWidth * numFullLines;
                dmdIn.vBlockInFrame += 7 * uvWidth * numFullLines;
            }
         dmdIn.xPosInMBs = dmdIn.xPosInMBs % inParam->pictParam->numMBsInMBLine;
         dmdIn.yPosInMBs+=numFullLines;
      }

      dmdIn.currMBNum = inOutParam->currMBNum + (numMBsInVP-1);

      /* get the last MB of the list */
      dlstTail(&MBList, (void **) &MBinstance);

      for (currMBNumInVP = numMBsInVP-1; currMBNumInVP >= 0; currMBNumInVP--) {

         /* header params */
         dmdIn.cbpy = MBinstance->cbpy;
         dmdIn.cbpc = MBinstance->cbpc;
         dmdIn.quant = MBinstance->quant;

         /* AC/DC prediction params */
         dmdIn.switched = MBinstance->switched;

         dmdIn.fTopOfVP = (u_char) (currMBNumInVP < inParam->pictParam->numMBsInMBLine);
         dmdIn.fLeftOfVP = (u_char) (currMBNumInVP == 0);
         dmdIn.fBBlockOut = (u_char) (currMBNumInVP <= inParam->pictParam->numMBsInMBLine);

         inOutParam->aicData->ACpred_flag = MBinstance->ac_pred_flag;

         /* error resilience params */
         dmdIn.data_partitioned = 1;
         dmdIn.DC = MBinstance->DC;

         dmdIn.reversible_vlc = 1;
         dmdIn.vlc_dec_direction = 1;
                 
         /* get the next macroblock data */
         ret = dmdGetAndDecodeMPEGIMBBlocks(&dmdIn, hTranscoder);
                 
         if ( ret < 0 ) {
             retValue = DMBS_ERR;
             goto exitFunction;                      
         }
         else if ( ret == DMD_BIT_ERR ) {
            backwards_errorBitPos = bibNumberOfFlushedBits(inParam->inBuffer);
            deb("dmbsGetAndDecodeIMBsDataPartitioned:ERROR - Backwards decoding, IMB Blocks error.\n");
            break;
         }


         if (bibNumberOfFlushedBits(inParam->inBuffer) <= startBlockDataBitPos) {
            deb("dmbsGetAndDecodeIMBsDataPartitioned:ERROR - Backwards decoding, block data start position reached.\n");
            break;
         }

         /* deincrement the block pointers and counters */
         dmdIn.xPosInMBs--;
         if (dmdIn.xPosInMBs < 0) {
            if (dmdIn.yPosInMBs > 0) {
               if ( dmdIn.yMBInFrame != NULL )
                {
                   dmdIn.yMBInFrame -= 15 * yWidth;
                   dmdIn.uBlockInFrame -= 7 * uvWidth;
                   dmdIn.vBlockInFrame -= 7 * uvWidth;
                }
               dmdIn.xPosInMBs = inParam->pictParam->numMBsInMBLine -1;
               dmdIn.yPosInMBs--;
            } else {
               dmdIn.xPosInMBs = 0;
               backwards_errorBitPos = bibNumberOfFlushedBits(inParam->inBuffer);
               break;
            }
         }
         if ( dmdIn.yMBInFrame != NULL )
         {
             dmdIn.yMBInFrame -= 16;
             dmdIn.uBlockInFrame -= 8;
             dmdIn.vBlockInFrame -= 8;
         }

         dmdIn.currMBNum--;
         dlstPrev(&MBList, (void **) &MBinstance);
      }

      if (currMBNumInVP < 0) {
         currMBNumInVP = 0;
         backwards_errorBitPos = bibNumberOfFlushedBits(inParam->inBuffer);
         deb("dmbsGetAndDecodeIMBsDataPartitioned:ERROR - Backwards decoding, all MBs decoded without detected error.\n");
      }

#ifdef DEBUG_OUTPUT
      {
         int bitPos[6], xpos, ypos, mbnum;
         
         for (xpos = dmdIn.xPosInMBs, ypos = dmdIn.yPosInMBs, mbnum = currMBNumInVP; mbnum < numMBsInVP; mbnum++, xpos++) {
            
            if (xpos / inParam->pictParam->numMBsInMBLine)
               xpos = 0, ypos++;
            
            fprintf(rvlc_stat, "bw: MB#%3d\tY0: %8d | Y1: %8d | Y2: %8d | Y3: %8d | U: %8d | V: %8d\n", 
               inOutParam->currMBNum+mbnum, bitPos[0], bitPos[1], bitPos[2], bitPos[3], bitPos[4], bitPos[5]); 
         }

         fprintf(rvlc_stat, "(blk_st):%8u (fw_det):%8u (bw_det):%8u (nxt_vp):%8u\n", 
            startBlockDataBitPos, errorBitPos, backwards_errorBitPos, nextVPBitPos);
      }
#endif

      /* strategy 1 */
      if ((backwards_errorBitPos > errorBitPos) && 
         (currMBNumInVP + 1 > numCorrectMBs)) {
#ifdef DEBUG_OUTPUT
         fprintf(rvlc_stat, "I-VOP: strategy 1!\n\n");
#endif
      
      }
      /* strategy 2 */
      else if ((backwards_errorBitPos > errorBitPos) && 
            (currMBNumInVP + 1 <= numCorrectMBs)) {
         numCorrectMBs = VDC_MAX(currMBNumInVP-1,0);
      }
      /* strategy 3 */
      else if ((backwards_errorBitPos <= errorBitPos) && 
            (currMBNumInVP + 1 > numCorrectMBs)) {
#ifdef DEBUG_OUTPUT
         fprintf(rvlc_stat, "I-VOP: strategy 3!\n\n");
#endif
         
      }
      /* strategy 4 */
      else if ((backwards_errorBitPos <= errorBitPos) && 
            (currMBNumInVP + 1 <= numCorrectMBs)) {
         numCorrectMBs = VDC_MAX(currMBNumInVP,0);
      }
         
#ifdef DEBUG_OUTPUT
      fclose (rvlc_stat);
#endif

      /* if backward decoding, set the currentMB to the first MB of the next VP */
      inOutParam->currMBNum += numMBsInVP;

    } else {   
      /* if no error or no backward decoding, set the currentMB */
      inOutParam->currMBNum = dmdIn.currMBNum;
   }

#ifdef DEBUG_OUTPUT
   if (errorBitPos)
   deb_core("%08lu: MB#%3d VP Data Starts\n%08lu: DC Marker\n%08lu: DCT data starts\n%08lu: MB#%3d Next VP/VOP Header\n%08lu: Fw Error Detected\n%08lu: Bw Error Detected\n", 
      startVPBitPos, (inParam->yPosInMBs*inParam->pictParam->numMBsInMBLine + inParam->xPosInMBs),
      DCMarkerBitPos, startBlockDataBitPos, nextVPBitPos, lastMBNum, 
      errorBitPos,backwards_errorBitPos);
#endif

exitFunction:

   deb1p("dmbsGetAndDecodeIMBsDataPartitioned:Finished.\n",inOutParam->currMBNum);
   /* Free the MB list */
   if (MBList.numItems != 0)
   {     
      dlstHead(&MBList, (void **) &MBinstance);
      dlstRemove(&MBList, (void **) &MBinstance);
      while (MBinstance != NULL) {
         free(MBinstance);
         dlstRemove(&MBList, (void **) &MBinstance);
      }
      dlstClose(&MBList);
   }

   return retValue;   
}


/* {{-output"dmbsGetAndDecodePMBsDataPartitioned.txt"}} */
/*
 * dmbsGetAndDecodePMBsDataPartitioned
 *    
 *
 * Parameters:
 *    inParam                    input parameters
 *    inOutParam                 input/output parameters, these parameters
 *                               may be modified in the function
 *
 * Function:
 *    This function gets and decodes the MBs of a data partitioned 
 *    Video Packet in an Inter Frame.
 *
 * Returns:
 *    >= 0                       the function was successful
 *    < 0                        an error occured
 *
 */

int dmbsGetAndDecodePMBsDataPartitioned(
   const dmbPFrameMBInParam_t *inParam,
   dmbPFrameMBInOutParam_t *inOutParam,
   int *quantParams, CMPEG4Transcoder *hTranscoder)
/* {{-output"dmbsGetAndDecodePMBsDataPartitioned.txt"}} */
{
   int currMBNumInVP, currMBNum, numMBsInVP, lastMBNum, numCorrectMBs, numCorrectBackwardsMBs = 0;
   int xPosInMBs, yPosInMBs;
   int yWidth = inParam->pictParam->lumMemWidth;
   int uvWidth = yWidth / 2;
   int bitErrorIndication = 0, 
      ret = 0, sncCode, bitsGot,
      bitErrorsInPart1 = 0;
   u_int32 errorBitPos = 0,
         backwards_errorBitPos = 0,
         nextVPBitPos = 0,
         startBlockDataBitPos = 0,
         motionMarkerBitPos = 0;
   u_char fPart1Error=0, fPart2Error=0, fBlockError=0;
   u_char *currYMBInFrame, *currUBlkInFrame, *currVBlkInFrame;
   u_char fourMVs = 0;
#ifdef DEBUG_OUTPUT
   FILE *rvlc_stat;
#endif

   int16 error = 0;

   dlst_t MBList;
   vdxPMBListItem_t *MBinstance;

   vdxGetDataPartitionedPMBLayerInputParam_t vdxDPIn;
   
   if (dlstOpen(&MBList) < 0)
      return DMBS_ERR;

   currMBNum = inParam->yPosInMBs*inParam->pictParam->numMBsInMBLine + inParam->xPosInMBs;

   /* Store initial quantizer */
   quantParams[currMBNum] = inOutParam->quant;

   /* 
    * read the first partition: DC coefficients, etc. 
    */

   vdxDPIn.intra_dc_vlc_thr = inParam->pictParam->intra_dc_vlc_thr;
   vdxDPIn.quant = inOutParam->quant;
   vdxDPIn.f_code = inParam->pictParam->fcode_forward;

   ret = vdxGetDataPartitionedPMBLayer_Part1(inParam->inBuffer, inParam->outBuffer, 
         inParam->bufEdit, inParam->iColorEffect, &(inOutParam->StartByteIndex), &(inOutParam->StartBitIndex),
         &vdxDPIn, &MBList, &bitErrorIndication);
   bitErrorsInPart1 = bitErrorIndication;
   if (ret < 0)
      return DMBS_ERR;
   else if (ret == VDX_OK_BUT_BIT_ERROR) {

      /* Must break down decoding, because even if we know the number of MBs 
         in the 2nd partition, the content (quant, ac_pred_flag) is dependent
        on "not_coded" and the type of MB (I-vop or P-vop), which information
        is derived from the 1st !currupted! partition. */
      
      deb("dmbsGetAndDecodePMBsDataPartitioned:ERROR - Part 1 Error.\n");
      fPart1Error = 1;
      bitErrorIndication = 0;
      errorBitPos = bibNumberOfFlushedBits(inParam->inBuffer);
   } else {
      motionMarkerBitPos = bibNumberOfFlushedBits(inParam->inBuffer);
   }

   /* read the next VP header to determine the number of MBs in this VP */
   if ( fPart1Error ) {
       /* Stop decoding this segment */
      goto exitFunction;
   }
   else {
      sncCode = sncRewindAndSeekNewMPEGSync( 1,
          inParam->inBuffer, inParam->pictParam->fcode_forward, &error);
   }
   if (error) {
      if (error == ERR_BIB_NOT_ENOUGH_DATA) error = 0;
      else 
          return DMBS_ERR;
   }
   
   nextVPBitPos = bibNumberOfFlushedBits(inParam->inBuffer);
   
   if (sncCode == SNC_VIDPACK) {
      
      vdxVideoPacketHeader_t header;
      vdxGetVideoPacketHeaderInputParam_t vdxParam;
      int retValue = 0;
      
      vdxParam.fcode_forward = inParam->pictParam->fcode_forward;
      vdxParam.time_increment_resolution = inParam->pictParam->time_increment_resolution;
      vdxParam.numOfMBs = inParam->pictParam->numMBsInGOB;
      
      retValue = vdxGetVideoPacketHeader(inParam->inBuffer, &vdxParam, &header, &bitErrorIndication);
      if (retValue < 0) {
         return DMBS_ERR;
      } else if (retValue == VDX_OK_BUT_BIT_ERROR) {
         /* If bit error occurred */
         deb("dmbsGetAndDecodePMBsDataPartitioned:ERROR - Reading Next VP Header error.\n");
         lastMBNum = 0;
      } else {
         lastMBNum = header.currMBNum;
      }
   } else {
      lastMBNum = inParam->pictParam->numMBsInGOB;
   }

   /* signaling to the caller function, that the Next VP header has been read */
   inOutParam->currMBNumInVP = lastMBNum;
      
   /* rewind the bits before the next resync_marker */
   bibRewindBits(bibNumberOfFlushedBits(inParam->inBuffer) - (fPart1Error ? nextVPBitPos: motionMarkerBitPos),
      inParam->inBuffer, &error);
   
   if ((bitErrorIndication && !fPart1Error) || 
      (lastMBNum <= currMBNum || lastMBNum > inParam->pictParam->numMBsInGOB) ||
      (sncCode == SNC_EOB)) {
      bitErrorIndication = 0;
      numMBsInVP = MBList.numItems;
   } else
      numMBsInVP = lastMBNum - currMBNum;

   if (numMBsInVP != MBList.numItems || fPart1Error) {
      deb("dmbsGetAndDecodePMBsDataPartitioned:ERROR - MB list length != num MBs.\n");
      if ( fPart1Error || bitErrorsInPart1 ) {
         /* Discard few MBs from the end of the list, 
            since there are errors somewhere in the bitstream. 
            If the list is short due to the missing packet, 
            all the read MBs are likely to be OK and there is no
            need to discard them */
         dlstTail(&MBList, (void **) &MBinstance);
         dlstRemove(&MBList, (void **) &MBinstance);
         free( MBinstance );
         if (numMBsInVP > MBList.numItems ) {
            /* Take still one away */
            dlstTail(&MBList, (void **) &MBinstance);
            dlstRemove(&MBList, (void **) &MBinstance);
            free( MBinstance );
         }
      }
      if (numMBsInVP < MBList.numItems ) {
         /* Discard all the extra MBs from the end of the list + 2 just to be sure, 
            since there are errors somewhere in the bitstream */
         while ( MBList.numItems > VDC_MAX(numMBsInVP-2,0) ) {
            dlstTail(&MBList, (void **) &MBinstance);
            dlstRemove(&MBList, (void **) &MBinstance);
            free( MBinstance );
         }
      }
      fPart1Error = 1;
      errorBitPos = bibNumberOfFlushedBits(inParam->inBuffer);
   }

   if (currMBNum + numMBsInVP > inParam->pictParam->numMBsInGOB) {
      deb("dmbsGetAndDecodePMBsDataPartitioned:ERROR - Determined numMBsInVP overrun numMBsInFrame.\n");
      fPart1Error = 1;
      errorBitPos = bibNumberOfFlushedBits(inParam->inBuffer);
      numMBsInVP = inParam->pictParam->numMBsInGOB - currMBNum;
   }

   if (!fPart1Error) {
      
      /* Flush the Motion_Marker */
      bibFlushBits(MP4_MOTION_MARKER_COMB_LENGTH, inParam->inBuffer, &bitsGot, &bitErrorIndication, &error);
      if (error)
         return DMBS_ERR;
      
      /*
       * Read the second partition header: cpby, ac_pred_flag 
       */

      ret = vdxGetDataPartitionedPMBLayer_Part2(inParam->inBuffer, inParam->outBuffer, 
                inParam->bufEdit, inParam->iColorEffect, &(inOutParam->StartByteIndex), 
                &(inOutParam->StartBitIndex), hTranscoder, 
                &vdxDPIn, &MBList, &bitErrorIndication);
      if (ret < 0)
         return DMBS_ERR;
      else if (ret == VDX_OK_BUT_BIT_ERROR) {
         deb("dmbsGetAndDecodePMBsDataPartitioned:ERROR - Part 2 Error.\n");
         fPart2Error = 1;
         /* Stop decoding this segment */
         goto exitFunction;
      }
   }

   /*
    * Count the motion vectors and copy the prediction blocks 
    */
   
   xPosInMBs = inParam->xPosInMBs;
   yPosInMBs = inParam->yPosInMBs;
   currYMBInFrame = inOutParam->yMBInFrame;
   currUBlkInFrame = inOutParam->uBlockInFrame;
   currVBlkInFrame = inOutParam->vBlockInFrame;

   /* get the first MB of the list */
   dlstHead(&MBList, (void **) &MBinstance);

   for (currMBNumInVP = 0; currMBNumInVP < numMBsInVP; currMBNumInVP++) {

      /* Motion vectors for P-macroblock */
      int mvx[4];
      int mvy[4];
       int mbPos;       /* the position of the current macroblock, 
                           -1 = the leftmost MB of the image, 
                           0 = MB is not in the border of the image, 
                           1 = rightmost MB of the image */

      blcCopyPredictionMBParam_t blcParam;
      
      /* if MBList is shorter then the number of MBs in the VP */
      if (MBinstance == NULL) {
         deb("dmbsGetAndDecodePMBsDataPartitioned:ERROR - MB list < num MBs.\n");
         inOutParam->currMBNum = currMBNum;
         goto exitFunction;
      }

      if (!MBinstance->fCodedMB) {
         
         inOutParam->fCodedMBs[currMBNum] = 0;
         /* Motion vectors to 0 */ 
         mvx[0] = mvx[1] = mvx[2] = mvx[3] = 0;
         mvy[0] = mvy[1] = mvy[2] = mvy[3] = 0;
         mvcMarkMBNotCoded(
            inOutParam->mvcData, 
            xPosInMBs,
            yPosInMBs,
            inParam->pictParam->tr);
         MBinstance->cbpy = 0;
         fourMVs = 0;

      } else {

         inOutParam->fCodedMBs[currMBNum] = 1;
         inOutParam->numOfCodedMBs++;
         
         if(MBinstance->mbClass == VDX_MB_INTER) {
            int currMVNum;

            fourMVs = (u_char) (MBinstance->numMVs == 4);
         
            for (currMVNum = 0; currMVNum < MBinstance->numMVs; currMVNum++) {

               mvcCalcMPEGMV(
                  inOutParam->mvcData,
                  MBinstance->mvx[currMVNum], MBinstance->mvy[currMVNum],
                  &mvx[currMVNum], &mvy[currMVNum],
                  (u_char) currMVNum, fourMVs,
                  (u_char) (currMBNumInVP < inParam->pictParam->numMBsInMBLine),
                  (u_char) (currMBNumInVP == 0), 
                  (u_char) (currMBNumInVP < (inParam->pictParam->numMBsInMBLine-1)),
                  xPosInMBs,
                  yPosInMBs,
                  inParam->pictParam->tr,
                  (MBinstance->mbClass == VDX_MB_INTRA) ? MVC_MB_INTRA : MVC_MB_INTER,
                  &error);   

                if (error == ERR_MVC_MVPTR)
                     return DMB_BIT_ERR;
                else if (error)
                     return DMB_ERR;
            }
            
            if (MBinstance->numMVs == 1) {
               mvx[1] = mvx[2] = mvx[3] = mvx[0];
               mvy[1] = mvy[2] = mvy[3] = mvy[0];
            }

         } else { /* VDX_MB_INTRA */
            mvcMarkMBIntra(inOutParam->mvcData, xPosInMBs, yPosInMBs, 
               inParam->pictParam->tr);

         }
      }

      /* mbPos, needed in blcCopyPredictionMB */
      if (xPosInMBs == 0)
         mbPos = -1;
      else if (xPosInMBs == inParam->pictParam->numMBsInMBLine - 1)
         mbPos = 1;
      else
         mbPos = 0;

      if (!MBinstance->fCodedMB || MBinstance->mbClass == VDX_MB_INTER) {
                blcParam.refY = inParam->refY;
                blcParam.refU = inParam->refU;
                blcParam.refV = inParam->refV;
                blcParam.currYMBInFrame = currYMBInFrame;
                blcParam.currUBlkInFrame = currUBlkInFrame;
                blcParam.currVBlkInFrame = currVBlkInFrame;
                blcParam.uvBlkXCoord = xPosInMBs * 8;
                blcParam.uvBlkYCoord = yPosInMBs * 8;
                blcParam.uvWidth = uvWidth;
                blcParam.uvHeight = inParam->pictParam->lumMemHeight / 2;
                blcParam.mvcData = inOutParam->mvcData;
                blcParam.mvx = mvx;
                blcParam.mvy = mvy;
                blcParam.mbPlace = mbPos;
                blcParam.fAdvancedPrediction = inParam->pictParam->fAP;
                blcParam.fMVsOverPictureBoundaries =
                    inParam->pictParam->fMVsOverPictureBoundaries;
                blcParam.diffMB = inOutParam->diffMB;
                blcParam.rcontrol = inParam->pictParam->rtype;
                blcParam.fourMVs = fourMVs;
                
                /* MVE */
                if (inParam->iGetDecodedFrame || hTranscoder->NeedDecodedYUVFrame())
                {
                    
                    /* Do motion compensation */
                    if (blcCopyPredictionMB(&blcParam) < 0) {
                        deb("dmbsGetAndDecodePMBsDataPartitioned:ERROR - Block copying failed, illegal MV.\n");
                        inOutParam->currMBNum = currMBNum;
                        goto exitFunction;
                        /* MV was illegal => caused by bitError */
                    }
                }
      }
            
      currMBNum++;

      /* increment the block pointers and counters */
      if ( currYMBInFrame != NULL )
        {
          currYMBInFrame += 16;
          currUBlkInFrame += 8;
          currVBlkInFrame += 8;
        }
      xPosInMBs++;

      if (xPosInMBs == inParam->pictParam->numMBsInMBLine) {
          if ( currYMBInFrame != NULL )
            {
                currYMBInFrame += 15 * yWidth;
                currUBlkInFrame += 7 * uvWidth;
                currVBlkInFrame += 7 * uvWidth;
            }
            xPosInMBs = 0;
            yPosInMBs++;
            if (yPosInMBs >= inParam->pictParam->numMBLinesInGOB)
                break;
      }
            
      /* MVE */
      MBinstance->mv_x[0] = mvx[0]; MBinstance->mv_x[1] = mvx[1]; MBinstance->mv_x[2] = mvx[2]; MBinstance->mv_x[3] = mvx[3];
      MBinstance->mv_y[0] = mvy[0]; MBinstance->mv_y[1] = mvy[1]; MBinstance->mv_y[2] = mvy[2]; MBinstance->mv_y[3] = mvy[3];
            
      dlstNext(&MBList, (void **) &MBinstance);
   }
     
   /* if error occured in the first 2 MV&header partitions, then stop decoding */
   if (fPart1Error || fPart2Error) {

#ifdef DEBUG_OUTPUT
      deb_core("%08lu: MB#%3d VP Data Starts\n%08lu: Motion Marker\n%08lu: DCT data starts\n%08lu: MB#%3d Next VP/VOP Header\n%08lu: Fw Error Detected\n", 
         startVPBitPos, (inParam->yPosInMBs*inParam->pictParam->numMBsInMBLine + inParam->xPosInMBs),
         motionMarkerBitPos, startBlockDataBitPos, nextVPBitPos, lastMBNum, 
         errorBitPos);
#endif
      
      inOutParam->currMBNum = currMBNum;
      goto exitFunction;
   }

   /*
    * Read block data partition in forward direction 
    */

   xPosInMBs = inParam->xPosInMBs;
   yPosInMBs = inParam->yPosInMBs;
   currMBNumInVP = 0;
   currMBNum = inParam->yPosInMBs*inParam->pictParam->numMBsInMBLine + inParam->xPosInMBs;
   currYMBInFrame = inOutParam->yMBInFrame;
   currUBlkInFrame = inOutParam->uBlockInFrame;
   currVBlkInFrame = inOutParam->vBlockInFrame;

   startBlockDataBitPos = bibNumberOfFlushedBits(inParam->inBuffer);

   for (dlstHead(&MBList, (void **) &MBinstance); 
     MBinstance != NULL; 
     dlstNext(&MBList, (void **) &MBinstance))
   {
        /* MVE */
        hTranscoder->OnePMBDataStartedDataPartitioned(MBinstance, &MBList, currMBNumInVP, currMBNum);
         
        if (MBinstance->fCodedMB) {
             
            /* If INTER MB */
            if (MBinstance->mbClass == VDX_MB_INTER) {
                dmdPParam_t dmdIn;
                     
                dmdIn.inBuffer = inParam->inBuffer;
                dmdIn.outBuffer = inParam->outBuffer;
                dmdIn.bufEdit = inParam->bufEdit;
                dmdIn.iColorEffect = inParam->iColorEffect;
                dmdIn.iGetDecodedFrame = inParam->iGetDecodedFrame;
                dmdIn.cbpy = MBinstance->cbpy;
                dmdIn.cbpc = MBinstance->cbpc;
                dmdIn.quant = MBinstance->quant;
                     
                /* MVE */
                dmdIn.refY = inParam->refY;
                dmdIn.refU = inParam->refU;
                dmdIn.refV = inParam->refV;
                dmdIn.mvx  = MBinstance->mv_x;
                dmdIn.mvy  = MBinstance->mv_y;

                dmdIn.currYMBInFrame = currYMBInFrame;
                dmdIn.currUBlkInFrame = currUBlkInFrame;
                dmdIn.currVBlkInFrame = currVBlkInFrame;
                dmdIn.uvWidth = uvWidth;
                dmdIn.reversible_vlc = inParam->pictParam->reversible_vlc;     
                dmdIn.vlc_dec_direction = 0;

                dmdIn.xPosInMBs = xPosInMBs;
                dmdIn.yPosInMBs = yPosInMBs;
                dmdIn.numMBsInMBLine = inParam->pictParam->numMBsInMBLine;
                     
                /* Store quantizer */
                quantParams[currMBNum] = MBinstance->quant;
                     
                /* Update the AIC module data, marking the MB as Inter (quant=0) */
                aicBlockUpdate (inOutParam->aicData, currMBNum, 0, NULL, 0, 0);
                     
                /* Decode prediction error blocks */
                ret = dmdGetAndDecodeMPEGPMBBlocks(&dmdIn, hTranscoder);
                     
                if ( ret < 0)
                    return DMBS_ERR;
                else if ( ret == DMD_BIT_ERR ) {
                    deb("dmbsGetAndDecodePMBsDataPartitioned:ERROR - PMB Blocks decoding failed.\n");
                    fBlockError = 1;
                    errorBitPos = bibNumberOfFlushedBits(inParam->inBuffer);
                    break;
                }

            }
            /* Else block layer decoding of INTRA macroblock */
            else {
                dmdMPEGIParam_t dmdIn;
                     
                /* Get block layer parameters and decode them */
                dmdIn.inBuffer = inParam->inBuffer;
                dmdIn.outBuffer = inParam->outBuffer;
                dmdIn.bufEdit = inParam->bufEdit;
                dmdIn.iColorEffect = inParam->iColorEffect;
                dmdIn.iGetDecodedFrame = inParam->iGetDecodedFrame;
                     
                dmdIn.cbpy = MBinstance->cbpy;
                dmdIn.cbpc = MBinstance->cbpc;
                dmdIn.quant = MBinstance->quant;
                dmdIn.yWidth = yWidth;
                dmdIn.yMBInFrame = currYMBInFrame;
                dmdIn.uBlockInFrame = currUBlkInFrame;
                dmdIn.vBlockInFrame = currVBlkInFrame;
                     
                dmdIn.xPosInMBs = xPosInMBs;
                dmdIn.yPosInMBs = yPosInMBs;
                dmdIn.numMBsInMBLine = inParam->pictParam->numMBsInMBLine;
                dmdIn.numMBLinesInGOB = inParam->pictParam->numMBLinesInGOB;
                dmdIn.pictureType = inParam->pictParam->pictureType;
                     
                inOutParam->aicData->ACpred_flag = MBinstance->ac_pred_flag;
                dmdIn.aicData = inOutParam->aicData;
                     
                dmdIn.data_partitioned = 1;
                dmdIn.switched = MBinstance->switched;
                dmdIn.DC = MBinstance->DC;
                     
                dmdIn.reversible_vlc = inParam->pictParam->reversible_vlc;
                dmdIn.vlc_dec_direction = 0;
                     
                dmdIn.currMBNum = currMBNum;
                     
                dmdIn.fTopOfVP = (u_char) 
                        (currMBNumInVP < inParam->pictParam->numMBsInMBLine ||
                         !aicIsBlockValid(inOutParam->aicData, currMBNum-inParam->pictParam->numMBsInMBLine));
                dmdIn.fLeftOfVP = (u_char)
                        (currMBNumInVP == 0 || 
                         xPosInMBs == 0 ||
                         !aicIsBlockValid(inOutParam->aicData, currMBNum-1));
                dmdIn.fBBlockOut = (u_char) 
                        (currMBNumInVP <= inParam->pictParam->numMBsInMBLine ||
                         xPosInMBs == 0 ||
                         !aicIsBlockValid(inOutParam->aicData, currMBNum-inParam->pictParam->numMBsInMBLine-1));
                     
                ret = dmdGetAndDecodeMPEGIMBBlocks(&dmdIn, hTranscoder);
                     
                if ( ret < 0 )
                    return DMBS_ERR;
                else if ( ret == DMD_BIT_ERR ) {
                    deb("dmbsGetAndDecodePMBsDataPartitioned:ERROR - IMB Blocks decoding failed.\n");
                    fBlockError = 1;
                    errorBitPos = bibNumberOfFlushedBits(inParam->inBuffer);
                    break;
                }
            }
                 
        } else {  /* end of if coded MB */
                     
            /* Update the AIC module data, marking the MB as Not Coded (quant=0) */
            aicBlockUpdate (inOutParam->aicData, currMBNum, 0, NULL, 0, 0);
        }
                 
        currMBNumInVP++;
        currMBNum++;

        /* increment the block pointers and counters */
        if ( currYMBInFrame != NULL )
         {
            currYMBInFrame += 16;
            currUBlkInFrame += 8;
            currVBlkInFrame += 8;
         }
        xPosInMBs++;

        if (xPosInMBs == inParam->pictParam->numMBsInMBLine) {
          if ( currYMBInFrame != NULL )
            {
                currYMBInFrame += 15 * yWidth;
                currUBlkInFrame += 7 * uvWidth;
                currVBlkInFrame += 7 * uvWidth;
            }
            xPosInMBs = 0;
            yPosInMBs++;
            if (yPosInMBs >= inParam->pictParam->numMBLinesInGOB)
                 break;
        }
   } // end of for-loop
   
   if (!fPart1Error && !fPart2Error && !fBlockError) {
      if (sncCode == SNC_EOB) {
         inOutParam->currMBNum += numMBsInVP;
         goto exitFunction;
      } else {
         sncCode = sncCheckMpegSync(inParam->inBuffer, inParam->pictParam->fcode_forward, &error);
         if (sncCode == SNC_NO_SYNC) {
            deb("dmbsGetAndDecodePMBsDataPartitioned:ERROR - After block data no start code found.\n");
            if (lastMBNum != 0) 
               fBlockError = 1;
            errorBitPos = bibNumberOfFlushedBits(inParam->inBuffer);
         }
      }
   }

   /*
    * In case of error, read block data partition in backward direction 
   */

   if (fBlockError && inParam->pictParam->reversible_vlc) {
      numCorrectMBs = currMBNumInVP;

#ifdef DEBUG_OUTPUT
      {
         int bitPos[6], xpos, ypos, mbnum;
         
         rvlc_stat = fopen("rvlc.log", "a+t");
         
         fprintf(rvlc_stat, "P-VOP: (MB_first):%3d  (MB_last):%3d\n", 
            inOutParam->currMBNum, (inOutParam->currMBNum + numMBsInVP-1));
         
         for (xpos = inParam->xPosInMBs, ypos = inParam->yPosInMBs, mbnum = 0; mbnum < numCorrectMBs; mbnum++, xpos++) {
            
            if (xpos / inParam->pictParam->numMBsInMBLine)
               xpos = 0, ypos++;

            fprintf(rvlc_stat, "fw: MB#%3d\tY0: %8d | Y1: %8d | Y2: %8d | Y3: %8d | U: %8d | V: %8d\n", 
               inOutParam->currMBNum+mbnum, bitPos[0], bitPos[1], bitPos[2], bitPos[3], bitPos[4], bitPos[5]); 
         }
      }
#endif

         /* find next VP header (end of MB block data of this VP) */
      sncCode = sncRewindAndSeekNewMPEGSync(errorBitPos-startBlockDataBitPos, inParam->inBuffer,
         inParam->pictParam->fcode_forward, &error);      
      if (error) {
          if (error == ERR_BIB_NOT_ENOUGH_DATA) error = 0;
          else 
              return DMBS_ERR;
      }

      if (sncCode == SNC_EOB) {
         inOutParam->currMBNum += numMBsInVP;
         goto exitFunction;
      }

      nextVPBitPos = bibNumberOfFlushedBits(inParam->inBuffer);
   
      backwards_errorBitPos = startBlockDataBitPos;

      /* rewind the stuffing bits */
      if (sncCode != SNC_NO_SYNC || !(nextVPBitPos % 8)) {
         if(sncRewindStuffing(inParam->inBuffer, &error) != SNC_PATTERN) {
            deb("dmbsGetAndDecodePMBsDataPartitioned:ERROR - Backwards decoding, stuffing not found.\n");
            goto exitFunction;
         }
      }

      /* set the block pointers and counters to the end of the VP */
      if ( currYMBInFrame != NULL )
        {
          currYMBInFrame = inOutParam->yMBInFrame + 16 * (numMBsInVP-1);
          currUBlkInFrame = inOutParam->uBlockInFrame + 8 * (numMBsInVP-1);
          currVBlkInFrame = inOutParam->vBlockInFrame + 8 * (numMBsInVP-1);
        }

      xPosInMBs = inParam->xPosInMBs + (numMBsInVP-1);
      yPosInMBs = inParam->yPosInMBs;

      if (xPosInMBs / inParam->pictParam->numMBsInMBLine) {

         int numFullLines = xPosInMBs / inParam->pictParam->numMBsInMBLine;

         if ( currYMBInFrame != NULL )
            {
             currYMBInFrame += 15 * yWidth * numFullLines;
             currUBlkInFrame += 7 * uvWidth * numFullLines;
             currVBlkInFrame += 7 * uvWidth * numFullLines;
            }
         xPosInMBs = xPosInMBs % inParam->pictParam->numMBsInMBLine;
         yPosInMBs+=numFullLines;
      }

      currMBNum = inParam->yPosInMBs*inParam->pictParam->numMBsInMBLine +
               inParam->xPosInMBs + (numMBsInVP-1);
      currMBNumInVP = numMBsInVP-1;

      for (dlstTail(&MBList, (void **) &MBinstance); 
         MBinstance != NULL; 
         dlstPrev(&MBList, (void **) &MBinstance))
      {
         if (MBinstance->fCodedMB) 
         {

             /* If INTER MB */
             if (MBinstance->mbClass == VDX_MB_INTER) 
             {
                dmdPParam_t dmdIn;

                dmdIn.inBuffer = inParam->inBuffer;
                dmdIn.outBuffer = inParam->outBuffer;
                dmdIn.bufEdit = inParam->bufEdit;
                dmdIn.iColorEffect = inParam->iColorEffect;
                dmdIn.iGetDecodedFrame = inParam->iGetDecodedFrame;
                dmdIn.cbpy = MBinstance->cbpy;
                dmdIn.cbpc = MBinstance->cbpc;
                dmdIn.quant = MBinstance->quant;
                            
                /* MVE */
                dmdIn.refY = inParam->refY;
                dmdIn.refU = inParam->refU;
                dmdIn.refV = inParam->refV;
                dmdIn.mvx  = MBinstance->mv_x;
                dmdIn.mvy  = MBinstance->mv_y;
                            
                dmdIn.currYMBInFrame = currYMBInFrame;
                dmdIn.currUBlkInFrame = currUBlkInFrame;
                dmdIn.currVBlkInFrame = currVBlkInFrame;
                dmdIn.uvWidth = uvWidth;
                
                dmdIn.reversible_vlc = inParam->pictParam->reversible_vlc;     
                dmdIn.vlc_dec_direction = 1;
                
                dmdIn.xPosInMBs = xPosInMBs;
                dmdIn.yPosInMBs = yPosInMBs;
                dmdIn.numMBsInMBLine = inParam->pictParam->numMBsInMBLine;

                /* Update the AIC module data, marking the MB as Inter (quant=0) */
                aicBlockUpdate (inOutParam->aicData, currMBNum, 0, NULL, 0, 0);
                
                /* Decode prediction error blocks */
                ret = dmdGetAndDecodeMPEGPMBBlocks(&dmdIn, hTranscoder);

                if ( ret < 0)
                   return DMBS_ERR;
                else if ( ret == DMD_BIT_ERR ) {
                   deb("dmbsGetAndDecodePMBsDataPartitioned:ERROR - Backwards decoding, PMB Blocks decoding failed.\n");
                   backwards_errorBitPos = bibNumberOfFlushedBits(inParam->inBuffer);
                   break;
                }

             }
             /* Else block layer decoding of INTRA macroblock */
             else 
             {
                dmdMPEGIParam_t dmdIn;

                /* Get block layer parameters and decode them */
                dmdIn.inBuffer = inParam->inBuffer;
                dmdIn.outBuffer = inParam->outBuffer;
                dmdIn.bufEdit = inParam->bufEdit;
                dmdIn.iColorEffect = inParam->iColorEffect;
                dmdIn.iGetDecodedFrame = inParam->iGetDecodedFrame;

                dmdIn.cbpy = MBinstance->cbpy;
                dmdIn.cbpc = MBinstance->cbpc;
                dmdIn.quant = MBinstance->quant;
                dmdIn.yWidth = yWidth;
                dmdIn.yMBInFrame = currYMBInFrame;
                dmdIn.uBlockInFrame = currUBlkInFrame;
                dmdIn.vBlockInFrame = currVBlkInFrame;

                dmdIn.xPosInMBs = xPosInMBs;
                dmdIn.yPosInMBs = yPosInMBs;
                dmdIn.numMBsInMBLine = inParam->pictParam->numMBsInMBLine;
                dmdIn.numMBLinesInGOB = inParam->pictParam->numMBLinesInGOB;
                dmdIn.pictureType = inParam->pictParam->pictureType;

                inOutParam->aicData->ACpred_flag = MBinstance->ac_pred_flag;
                dmdIn.aicData = inOutParam->aicData;

                dmdIn.data_partitioned = inParam->pictParam->data_partitioned;
                dmdIn.switched = MBinstance->switched;
                dmdIn.DC = MBinstance->DC;

                dmdIn.reversible_vlc = inParam->pictParam->reversible_vlc;
                dmdIn.vlc_dec_direction = 1;

                dmdIn.currMBNum = currMBNum;

                dmdIn.fTopOfVP = (u_char) 
                   (currMBNumInVP < inParam->pictParam->numMBsInMBLine ||
                    !aicIsBlockValid(inOutParam->aicData, currMBNum-inParam->pictParam->numMBsInMBLine));
                dmdIn.fLeftOfVP = (u_char)
                   (currMBNumInVP == 0 || 
                    xPosInMBs == 0 ||
                    !aicIsBlockValid(inOutParam->aicData, currMBNum-1));
                dmdIn.fBBlockOut = (u_char) 
                   (currMBNumInVP <= inParam->pictParam->numMBsInMBLine ||
                    xPosInMBs == 0 ||
                    !aicIsBlockValid(inOutParam->aicData, currMBNum-inParam->pictParam->numMBsInMBLine-1));
                
                ret = dmdGetAndDecodeMPEGIMBBlocks(&dmdIn, hTranscoder);

                if ( ret < 0 )
                   return DMBS_ERR;
                else if ( ret == DMD_BIT_ERR ) {
                   deb("dmbsGetAndDecodePMBsDataPartitioned:ERROR - Backwards decoding, IMB Blocks decoding failed.\n");
                   backwards_errorBitPos = bibNumberOfFlushedBits(inParam->inBuffer);

                   break;
                }
             } // end of else

         } /* end of if coded MB */
         else 
         {  
       
            /* Update the AIC module data, marking the MB as Not Coded (quant=0) */
            aicBlockUpdate (inOutParam->aicData, currMBNum, 0, NULL, 0, 0);
         }

         if (bibNumberOfFlushedBits(inParam->inBuffer) <= startBlockDataBitPos) {
            deb("dmbsGetAndDecodePMBsDataPartitioned:ERROR - Backwards decoding, block data start position reached.\n");
            break;
         }

         /* deincrement the block pointers and counters */
         xPosInMBs--;
         if (xPosInMBs < 0) {
            if (yPosInMBs > 0) {
              if ( currYMBInFrame != NULL )
                {
                   currYMBInFrame -= 15 * yWidth;
                   currUBlkInFrame -= 7 * uvWidth;
                   currVBlkInFrame -= 7 * uvWidth;
                }
               xPosInMBs = inParam->pictParam->numMBsInMBLine -1;
               yPosInMBs--;
            } else {
               xPosInMBs = 0;
               backwards_errorBitPos = bibNumberOfFlushedBits(inParam->inBuffer);
               break;
            }
         }
          if ( currYMBInFrame != NULL )
            {
             currYMBInFrame -= 16;
             currUBlkInFrame -= 8;
             currVBlkInFrame -= 8;
            }

         currMBNumInVP--;
         currMBNum--;
      }

      if (currMBNumInVP < 0) {
         currMBNumInVP = 0;
         backwards_errorBitPos = bibNumberOfFlushedBits(inParam->inBuffer);
         deb("dmbsGetAndDecodePMBsDataPartitioned:ERROR - Backwards decoding, all MBs decoded without detected error.\n");
      }

#ifdef DEBUG_OUTPUT
      {
         int bitPos[6], xpos, ypos, mbnum;

         xPosInMBs++;
         if (xPosInMBs >= inParam->pictParam->numMBsInMBLine) {
            xPosInMBs = 0;
            if (yPosInMBs < (inParam->pictParam->numMBLinesInGOB-1))
               yPosInMBs++;
         }

         for (xpos = xPosInMBs, ypos = yPosInMBs, mbnum = (currMBNumInVP+1); mbnum < numMBsInVP; mbnum++, xpos++) {
            
            if (xpos / inParam->pictParam->numMBsInMBLine)
               xpos = 0, ypos++;
            
            fprintf(rvlc_stat, "bw: MB#%3d\tY0: %8d | Y1: %8d | Y2: %8d | Y3: %8d | U: %8d | V: %8d\n", 
               inOutParam->currMBNum+mbnum, bitPos[0], bitPos[1], bitPos[2], bitPos[3], bitPos[4], bitPos[5]); 
         }
         
         fprintf(rvlc_stat, "(blk_st):%8u (fw_det):%8u (bw_det):%8u (nxt_vp):%8u\n", 
            startBlockDataBitPos, errorBitPos, backwards_errorBitPos, nextVPBitPos);
      }
#endif
      
      /* 
       * Based on the decoder MB counters forwards and backwards, 
       * recopy the "to be discarded" blocks 
       */

      /* strategy 1 */
      if ((backwards_errorBitPos > errorBitPos) && 
         (currMBNumInVP + 1 > numCorrectMBs)) {
         numCorrectBackwardsMBs = VDC_MIN(currMBNumInVP + 2,numMBsInVP);       
         numCorrectMBs = VDC_MAX(numCorrectMBs-2,0);
#ifdef DEBUG_OUTPUT
         fprintf(rvlc_stat, "P-VOP strategy 1: MBs %3d-%3d concealed\n\n", inOutParam->currMBNum+numCorrectMBs, inOutParam->currMBNum+numCorrectBackwardsMBs-1);
#endif
      }
      /* strategy 2 */
      else if ((backwards_errorBitPos > errorBitPos) && 
             (currMBNumInVP + 1 <= numCorrectMBs)) {
         numCorrectBackwardsMBs = VDC_MIN(numCorrectMBs + 1,numMBsInVP);
         numCorrectMBs = VDC_MAX(currMBNumInVP-1,0);
#ifdef DEBUG_OUTPUT
         fprintf(rvlc_stat, "P-VOP strategy 2: MBs %3d-%3d concealed\n\n", inOutParam->currMBNum+numCorrectMBs, inOutParam->currMBNum+numCorrectBackwardsMBs-1);
#endif
      }
      /* strategy 3 */
      else if ((backwards_errorBitPos <= errorBitPos) && 
             (currMBNumInVP + 1 > numCorrectMBs)) {
         numCorrectBackwardsMBs = VDC_MIN(currMBNumInVP + 2,numMBsInVP);       
         numCorrectMBs = VDC_MAX(numCorrectMBs-2,0);
#ifdef DEBUG_OUTPUT
         fprintf(rvlc_stat, "P-VOP strategy 3: MBs %3d-%3d concealed\n\n", inOutParam->currMBNum+numCorrectMBs, inOutParam->currMBNum+numCorrectBackwardsMBs-1);
#endif
      }
      /* strategy 4 */
      else if ((backwards_errorBitPos <= errorBitPos) && 
             (currMBNumInVP + 1 <= numCorrectMBs)) {
         numCorrectBackwardsMBs = VDC_MIN(numCorrectMBs + 1,numMBsInVP);       
         numCorrectMBs = VDC_MAX(currMBNumInVP-1,0);
#ifdef DEBUG_OUTPUT
         fprintf(rvlc_stat, "P-VOP strategy 4: MBs %3d-%3d concealed\n\n", inOutParam->currMBNum+numCorrectMBs, inOutParam->currMBNum+numCorrectBackwardsMBs-1);
#endif
      }

#ifdef DEBUG_OUTPUT
      fclose (rvlc_stat);
#endif


      /* set the block pointers and counters to the end of the VP */
      if ( currYMBInFrame != NULL )
        {
          currYMBInFrame = inOutParam->yMBInFrame + 16 * numCorrectMBs;
          currUBlkInFrame = inOutParam->uBlockInFrame + 8 * numCorrectMBs;
          currVBlkInFrame = inOutParam->vBlockInFrame + 8 * numCorrectMBs;
        }
      
      xPosInMBs = inParam->xPosInMBs + numCorrectMBs;
      yPosInMBs = inParam->yPosInMBs;
      
      if (xPosInMBs / inParam->pictParam->numMBsInMBLine) {
         
         int numFullLines = xPosInMBs / inParam->pictParam->numMBsInMBLine;
         
         if ( currYMBInFrame != NULL )
            {
             currYMBInFrame += 15 * yWidth * numFullLines;
             currUBlkInFrame += 7 * uvWidth * numFullLines;
             currVBlkInFrame += 7 * uvWidth * numFullLines;
            }
         xPosInMBs = xPosInMBs % inParam->pictParam->numMBsInMBLine;
         yPosInMBs+=numFullLines;
      }
      
      /* get the first MB of the list */
      for (currMBNumInVP = 0, dlstHead(&MBList, (void **) &MBinstance); 
         currMBNumInVP < numCorrectMBs; currMBNumInVP++) 
         dlstNext(&MBList, (void **) &MBinstance);
      
      for (currMBNumInVP = numCorrectMBs; currMBNumInVP < numCorrectBackwardsMBs; currMBNumInVP++) {
         /* The blocks whose DCT are missing, will be reconstructed (possibly again => clear effects of corrupted DCT) here,
            using the MVs decoded from 1st partition */         
         /* Motion vectors for P-macroblock */
         int mvx[4];
         int mvy[4];
         int mbPos;       /* the position of the current macroblock, 
                      -1 = the leftmost MB of the image, 
                      0 = MB is not in the border of the image,
                      1 = rightmost MB of the image */
         
         blcCopyPredictionMBParam_t blcParam;
         
         if (MBinstance == NULL) {
            break;
         }
         
         if (!MBinstance->fCodedMB) {           
            /* Motion vectors to 0 */ 
            mvx[0] = mvx[1] = mvx[2] = mvx[3] = 0;
            mvy[0] = mvy[1] = mvy[2] = mvy[3] = 0;
            mvcMarkMBNotCoded(
               inOutParam->mvcData, 
               xPosInMBs,
               yPosInMBs,
               inParam->pictParam->tr);
            MBinstance->cbpy = 0;
            fourMVs = 0;
            
         } else {
            
            if(MBinstance->mbClass == VDX_MB_INTER) {
               int currMVNum;
               
               fourMVs = (u_char) (MBinstance->numMVs == 4);
               
               for (currMVNum = 0; currMVNum < MBinstance->numMVs; currMVNum++) {
                  
                  mvcCalcMPEGMV(
                     inOutParam->mvcData,
                     MBinstance->mvx[currMVNum], MBinstance->mvy[currMVNum],
                     &mvx[currMVNum], &mvy[currMVNum],
                     (u_char) currMVNum, fourMVs,
                     (u_char) (currMBNumInVP < inParam->pictParam->numMBsInMBLine),
                     (u_char) (currMBNumInVP == 0), 
                     (u_char) (currMBNumInVP < (inParam->pictParam->numMBsInMBLine-1)),
                     xPosInMBs,
                     yPosInMBs,
                     inParam->pictParam->tr,
                     (MBinstance->mbClass == VDX_MB_INTRA) ? MVC_MB_INTRA : MVC_MB_INTER,
                     &error);   
                  
                if (error == ERR_MVC_MVPTR)
                     return DMB_BIT_ERR;
                else if (error)
                     return DMB_ERR;
                        
                  /* if MVs over VOP boundaries are not allowed */
                  if ((xPosInMBs == 0 && mvx[currMVNum] < 0) ||
                      (xPosInMBs == inParam->pictParam->numMBsInMBLine-1 && mvx[currMVNum] > 0) ||
                      (yPosInMBs == 0 && mvy[currMVNum] < 0) ||
                      (yPosInMBs == inParam->pictParam->numMBLinesInGOB-1 && mvy[currMVNum] > 0)) {
                      mvx[currMVNum] = 0;
                      mvy[currMVNum] = 0;
                  }
               }
               
               if (MBinstance->numMVs == 1) {
                  mvx[1] = mvx[2] = mvx[3] = mvx[0];
                  mvy[1] = mvy[2] = mvy[3] = mvy[0];
               }
                                 
            } else { /* VDX_MB_INTRA */

               /*int predBlocks[8] = {0,0,0,0,0,0,0,0};*/

               mvcMarkMBIntra(inOutParam->mvcData, xPosInMBs, yPosInMBs, 
                  inParam->pictParam->tr);
                    

                    /* Conceal the MB in intra mode (use top and left predictor only)
                    if (xPosInMBs > 0) predBlocks[5] = 1;
                    if (yPosInMBs > 0) predBlocks[4] = 1;
                    epixConcealMB( inParam->currPY, yPosInMBs<<1, xPosInMBs<<1, predBlocks, yWidth, 2 );
                    epixConcealMB( inParam->currPU, yPosInMBs, xPosInMBs, predBlocks, uvWidth, 1 );
                    epixConcealMB( inParam->currPV, yPosInMBs, xPosInMBs, predBlocks, uvWidth, 1 ); */

               mvcCalcMPEGMV(
                  inOutParam->mvcData,
                  0, 0,
                  &mvx[0], &mvy[0],
                  (u_char) 0, 0,
                  (u_char) (currMBNumInVP < inParam->pictParam->numMBsInMBLine),
                  (u_char) (currMBNumInVP == 0), 
                  (u_char) (currMBNumInVP < (inParam->pictParam->numMBsInMBLine-1)),
                  xPosInMBs,
                  yPosInMBs,
                  inParam->pictParam->tr,
                  MVC_MB_INTRA,
                  &error);   
               
                if (error == ERR_MVC_MVPTR)
                     return DMB_BIT_ERR;
                else if (error)
                     return DMB_ERR;
               
               /* if MVs over VOP boundaries are not allowed */
               if ((xPosInMBs == 0 && mvx[0] < 0) ||
                  (xPosInMBs == inParam->pictParam->numMBsInMBLine-1 && mvx[0] > 0) ||
                  (yPosInMBs == 0 && mvy[0] < 0) ||
                  (yPosInMBs == inParam->pictParam->numMBLinesInGOB-1 && mvy[0] > 0)) {
                  mvx[0] = 0;
                  mvy[0] = 0;
               }

               mvx[1] = mvx[2] = mvx[3] = mvx[0];
               mvy[1] = mvy[2] = mvy[3] = mvy[0];        
            }
         }
         
         /* mbPos, needed in blcCopyPredictionMB */
         if (xPosInMBs == 0)
            mbPos = -1;
         else if (xPosInMBs == inParam->pictParam->numMBsInMBLine - 1)
            mbPos = 1;
         else
            mbPos = 0;
         
/*         if (!MBinstance->fCodedMB || MBinstance->mbClass == VDX_MB_INTER) {*/
            blcParam.refY = inParam->refY;
            blcParam.refU = inParam->refU;
            blcParam.refV = inParam->refV;
            blcParam.currYMBInFrame = currYMBInFrame;
            blcParam.currUBlkInFrame = currUBlkInFrame;
            blcParam.currVBlkInFrame = currVBlkInFrame;
            blcParam.uvBlkXCoord = xPosInMBs * 8;
            blcParam.uvBlkYCoord = yPosInMBs * 8;
            blcParam.uvWidth = uvWidth;
            blcParam.uvHeight = inParam->pictParam->lumMemHeight / 2;
            blcParam.mvcData = inOutParam->mvcData;
            blcParam.mvx = mvx;
            blcParam.mvy = mvy;
            blcParam.mbPlace = mbPos;
            blcParam.fAdvancedPrediction = inParam->pictParam->fAP;
            blcParam.fMVsOverPictureBoundaries =
               inParam->pictParam->fMVsOverPictureBoundaries;
            blcParam.diffMB = inOutParam->diffMB;
            blcParam.rcontrol = inParam->pictParam->rtype;
            blcParam.fourMVs = fourMVs;
            
            /* Do motion compensation */
            if (blcCopyPredictionMB(&blcParam) < 0) {
               goto exitFunction;
            }
/*         }*/
         
         /* increment the block pointers and counters */
         if ( currYMBInFrame != NULL )
            {
             currYMBInFrame += 16;
             currUBlkInFrame += 8;
             currVBlkInFrame += 8;
            }
         xPosInMBs++;
         
         if (xPosInMBs == inParam->pictParam->numMBsInMBLine) {
          if ( currYMBInFrame != NULL )
            {
                currYMBInFrame += 15 * yWidth;
                currUBlkInFrame += 7 * uvWidth;
                currVBlkInFrame += 7 * uvWidth;
            }
            xPosInMBs = 0;
            yPosInMBs++;
            if (yPosInMBs >= inParam->pictParam->numMBLinesInGOB)
               break;
         }
         
         dlstNext(&MBList, (void **) &MBinstance);
      }
    } 

#ifdef DEBUG_OUTPUT
   if (errorBitPos)
   deb_core("%08lu: MB#%3d VP Data Starts\n%08lu: Motion Marker\n%08lu: DCT data starts\n%08lu: MB#%3d Next VP/VOP Header\n%08lu: Fw Error Detected\n%08lu: Bw Error Detected\n", 
      startVPBitPos, (inParam->yPosInMBs*inParam->pictParam->numMBsInMBLine + inParam->xPosInMBs),
      motionMarkerBitPos, startBlockDataBitPos, nextVPBitPos, lastMBNum, 
      errorBitPos,backwards_errorBitPos);
#endif

   /* if no error in Part1 and Part2, set the currentMB to the first MB of the next VP */
   inOutParam->currMBNum += numMBsInVP;

exitFunction:

   deb1p("dmbsGetAndDecodePMBsDataPartitioned:Finished.\n",inOutParam->currMBNum);
   /* Free the MB list */
   if (MBList.numItems != 0)
   {     
      dlstHead(&MBList, (void **) &MBinstance);
      dlstRemove(&MBList, (void **) &MBinstance);
      while (MBinstance != NULL) {
         free(MBinstance);
         dlstRemove(&MBList, (void **) &MBinstance);
      }
      dlstClose(&MBList);
   }

   return DMBS_OK;
}
// End of File
