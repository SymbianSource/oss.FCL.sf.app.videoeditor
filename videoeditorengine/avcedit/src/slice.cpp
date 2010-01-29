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


#include <string.h>
#include "globals.h"
#include "bitbuffer.h"
#include "vld.h"
#include "macroblock.h"
#include "parameterset.h"
#include "framebuffer.h"
#include "dpb.h"
#include "slice.h"
#include "sequence.h"


#define MIN_ALPHA_BETA_OFFSET   -6
#define MAX_ALPHA_BETA_OFFSET   6

/*
 * AVC syntax functions as specified in specification
 */


/*
 * Static functions
 */

static int getRefPicListReorderingCmds(slice_s *slice, unsigned int numRefFrames,
                                       bitbuffer_s *bitbuf);

static int getDecRefPicMarkingCmds(slice_s *slice, unsigned int numRefFrames,
                                   bitbuffer_s *bitbuf);

static int refPicListReordering(slice_s *slice, dpb_s *dpb,
                                 frmBuf_s *refPicList[], int numRefPicActive,
                                 sliceRefPicListReorderCmd_s reorderCmdList[]);



/*
 * AVC syntax functions as specified in specification
 */

/* Return fixed length code */
static int u_n(bitbuffer_s *bitbuf, int len, unsigned int *val)
{
  *val = vldGetFLC(bitbuf, len);

  if (bibGetStatus(bitbuf) < 0)
    return SLICE_ERROR;

  return SLICE_OK;
}

/* Return unsigned UVLC code */
static int ue_v(bitbuffer_s *bitbuf, unsigned int *val, unsigned int maxVal)
{
  *val = vldGetUVLC(bitbuf);

  if (bibGetStatus(bitbuf) < 0)
    return SLICE_ERROR;

  if (*val > maxVal)
    return SLICE_ERR_ILLEGAL_VALUE;

  return SLICE_OK;
}

/* Return signed UVLC code */
static int se_v(bitbuffer_s *bitbuf, int *val, int minVal, int maxVal)
{
  *val = vldGetSignedUVLC(bitbuf);

  if (bibGetStatus(bitbuf) < 0)
    return SLICE_ERROR;

  if (*val < minVal || *val > maxVal)
    return SLICE_ERR_ILLEGAL_VALUE;

  return SLICE_OK;
}

/* Return long signed UVLC code */
static int se_v_long(bitbuffer_s *bitbuf, int32 *val)
{
  *val = vldGetSignedUVLClong(bitbuf);

  if (bibGetStatus(bitbuf) < 0)
    return SLICE_ERROR;

  return SLICE_OK;
}


/*
 *
 * sliceOpen:
 *
 * Parameters:
 *
 * Function:
 *      Allocate and initialize a slice.
 *
 * Returns:
 *      Pointer to slice
 *
 */
slice_s *sliceOpen()
{
  slice_s *slice;

  slice = (slice_s *)User::Alloc(sizeof(slice_s));

  if (slice != NULL)
    memset(slice, 0, sizeof(slice_s));

  return slice;
}


/*
 *
 * sliceClose:
 *
 * Parameters:
 *      slice                 Slice object
 *
 * Function:
 *      Deallocate slice
 *
 * Returns:
 *      Nothing
 *
 */
void sliceClose(slice_s *slice)
{
  User::Free(slice);
}


/*
 * getRefPicListReorderingCmds:
 *
 * Parameters:
 *     slice               Slice object
 *     bitbuf              Bitbuffer object
 *     numRefFrames        Number of reference frames in used
 *
 * Function:
 *     Parse and store the ref pic reordering commands
 *
 * Return:
 *     The number of bits being parsed
 */
static int getRefPicListReorderingCmds(slice_s *slice, unsigned int numRefFrames,
                                       bitbuffer_s *bitbuf)
{
  int i;
  unsigned int reordering_of_pic_nums_idc;
  int retCode;

  if (!IS_SLICE_I(slice->slice_type)) {

    if ((retCode = u_n(bitbuf, 1, &slice->ref_pic_list_reordering_flag0)) < 0)
      return retCode;

    if (slice->ref_pic_list_reordering_flag0) {

      i = 0;
      do {
        /* Get command */
        if ((retCode = ue_v(bitbuf, &reordering_of_pic_nums_idc, 3)) < 0)
          return retCode;

        slice->reorderCmdList[i].reordering_of_pic_nums_idc = reordering_of_pic_nums_idc;

        /* Get command parameters */
        if (reordering_of_pic_nums_idc == 0 || reordering_of_pic_nums_idc == 1) {
          unsigned int maxDiff = slice->maxFrameNum/2-1;
          if (reordering_of_pic_nums_idc == 1)
            maxDiff = maxDiff - 1;
          if ((retCode = ue_v(bitbuf, &slice->reorderCmdList[i].abs_diff_pic_num_minus1, maxDiff)) < 0)
            return retCode;
        }
        else if (reordering_of_pic_nums_idc == 2) {
          /* longTermPicNum be in the range of 0 to num_ref_frames, inclusive. */
          if ((retCode = ue_v(bitbuf, &slice->reorderCmdList[i].long_term_pic_num, numRefFrames)) < 0)
            return retCode;
        }

        i++;
      } while (reordering_of_pic_nums_idc != 3 && i < MAX_NUM_OF_REORDER_CMDS);
    }
  }

  return SLICE_OK;
}


/*
 * getDecRefPicMarkingCmds:
 *
 * Parameters:
 *     slice               Slice object
 *     bitbuf              Bitbuffer object
 *     numRefFrames        Number of reference frames in used
 *
 * Function:
 *     Parse and store the MMCO commands
 *
 * Return:
 *     The number of bits being parsed
 */
static int getDecRefPicMarkingCmds(slice_s *slice, unsigned int numRefFrames,
                                   bitbuffer_s *bitbuf)
{
  int i;
  unsigned int mmco;
  int retCode;

  /* MMCO commands can exist only in slice header of a reference picture */
  if (slice->nalRefIdc != 0) {
    if (slice->isIDR) {
      if ((retCode = u_n(bitbuf, 1, &slice->no_output_of_prior_pics_flag)) < 0)
        return retCode;
      if ((retCode = u_n(bitbuf, 1, &slice->long_term_reference_flag)) < 0)
        return retCode;
    }
    else {
      if ((retCode = u_n(bitbuf, 1, &slice->adaptive_ref_pic_marking_mode_flag)) < 0)
        return retCode;

      if (slice->adaptive_ref_pic_marking_mode_flag) {

        i = 0;
        do {
          /* Get MMCO command */
          if ((retCode = ue_v(bitbuf, &mmco, 6)) < 0)
            return retCode;

          slice->mmcoCmdList[i].memory_management_control_operation = mmco;

          /* Get command parameter (if any) */
          if (mmco == 1 || mmco == 3) {
            if ((retCode = ue_v(bitbuf, &slice->mmcoCmdList[i].difference_of_pic_nums_minus1, 65535)) < 0)
              return retCode;
          }
          if (mmco == 2) {
            if ((retCode = ue_v(bitbuf, &slice->mmcoCmdList[i].long_term_pic_num, numRefFrames)) < 0)
              return retCode;
          }
          if (mmco == 3 || mmco == 6) {
            if ((retCode = ue_v(bitbuf, &slice->mmcoCmdList[i].long_term_frame_idx, numRefFrames)) < 0)
              return retCode;
          }
          if (mmco == 4) {
            if ((retCode = ue_v(bitbuf, &slice->mmcoCmdList[i].max_long_term_frame_idx_plus1, numRefFrames)) < 0)
              return retCode;
          }
          if (mmco == 5) {
            slice->picHasMMCO5 = 1;
          }

          i++;
        } while (mmco != 0 && i < MAX_NUM_OF_MMCO_OPS);
      }
    }
  }
  else
    slice->adaptive_ref_pic_marking_mode_flag = 0;

  return 1;
}


/*
 * sliceInitRefPicList:
 *
 * Parameters:
 *     dpb                  DPB buffer
 *     refPicList           Reference picture list (output)
 *
 * Fucntion:
 *     Initialize reference picture list.
 *
 * Return:
 *     Number of reference frames in the list
 */
int sliceInitRefPicList(dpb_s *dpb, frmBuf_s *refPicList[])
{
  int numRef, numShort;
  frmBuf_s *refTmp;
  int i, j;

  /*
   * Select the reference pictures from the DPB
   */
  j = 0;
  /* Put short term pictures first in the list */
  for (i = 0; i < dpb->fullness; i++) {
    if (dpb->buffers[i]->refType == FRM_SHORT_TERM_PIC) 
      refPicList[j++] = dpb->buffers[i];
  }
  numShort = j;
  /* Put long term pictures after the short term pictures */
  for (i = 0; i < dpb->fullness; i++) {
    if (dpb->buffers[i]->refType == FRM_LONG_TERM_PIC) 
      refPicList[j++] = dpb->buffers[i];
  }
  numRef = j;
  /* numLong = numRef - numShort; */

  /*
   * Initialisation process for reference picture lists
   */
  /* Sort short term pictures in the order of descending picNum */
  for (i = 0; i < numShort; i++) {
    for (j = i+1; j < numShort; j++) {
      if (refPicList[i]->picNum < refPicList[j]->picNum) {
        /* exchange refPicList[i] and refPicList[j] */
        refTmp = refPicList[i];
        refPicList[i] = refPicList[j];
        refPicList[j] = refTmp;
      }
    }
  }
  /* Sort long term pictures in the order of ascending longTermPicNum */
  for (i = numShort; i < numRef; i++) {
    for (j = i+1; j < numRef; j++) {
      if (refPicList[i]->longTermPicNum > refPicList[j]->longTermPicNum) {
        /* exchange refPicList[i] and refPicList[j] */
        refTmp = refPicList[i];
        refPicList[i] = refPicList[j];
        refPicList[j] = refTmp;
      }
    }
  }

  return numRef;
}


/*
 * sliceFixRefPicList:
 *
 * Parameters:
 *     dpb                  DPB buffer
 *     refPicList           Reference picture list (output)
 *     numRefPicActive      Number of active reference frames
 *     numExistingRefFrames Number of reference frames in refPicList
 *
 * Fucntion:
 *     If numExistingRefFrames < numRefPicActive, try to fill up the
 *     reference frame list
 *
 * Return:
 *     0 for no pictures exist in reference frame list
 *     1 for pictures exist in reference frame list
 */
int sliceFixRefPicList(dpb_s *dpb, frmBuf_s *refPicList[],
                       int numRefPicActive, int numExistingRefFrames,
                       int width, int height)
{
  int i;

  if (numExistingRefFrames == 0) {
    /* Try to find any picture in DPB, even non-reference frame */
    for (i = 0; i < dpb->size; i++) {
      if (dpb->buffers[i] != NULL && dpb->buffers[i]->width == width && dpb->buffers[i]->height == height)
        break;
    }

    if (i < dpb->size) {
      refPicList[0] = dpb->buffers[i];
      numExistingRefFrames = 1;
    }
    else
      return 0;
  }

  /* Duplicate last extry of the reference frame list so that list becomes full */
  for (i = numExistingRefFrames; i < numRefPicActive; i++)
    refPicList[i] = refPicList[numExistingRefFrames-1];

  return 1;
}

/*
 * refPicListReordering:
 *
 * Parameters:
 *     slice                Current slice object
 *     dpb                  DPB buffer
 *     refPicList           Reference picture list (modified by this function)
 *     numRefPicActive      Number of active reference frames
 *     reorderCmdList       Reordering command list
 *
 * Fucntion:
 *     Reorder the reference picture list for current slice
 *
 * Return:
 *     SLICE_OK for no error and negative value for error
 */
static int refPicListReordering(slice_s *slice, dpb_s *dpb,
                                frmBuf_s *refPicList[], int numRefPicActive,
                                sliceRefPicListReorderCmd_s reorderCmdList[])
{
  int i;
  int reordering_of_pic_nums_idc, longTermPicNum;
  int32 absDiffPicNum;
  int refIdx;
  int32 currPicNum, picNumPred, picNumNoWrap;
  int32 maxPicNum, picNum;
  int cmdNum;
  int cIdx, nIdx;

  /*
   * 3. Reordering process for reference picture list
   */

  maxPicNum = slice->maxFrameNum;   /* for frame coding only */
  currPicNum = slice->frame_num;
  picNumPred = currPicNum;
  refIdx = 0;
  cmdNum = 0;

  do {
    reordering_of_pic_nums_idc = reorderCmdList[cmdNum].reordering_of_pic_nums_idc;

    if (reordering_of_pic_nums_idc == 0 || reordering_of_pic_nums_idc == 1) {

      /*
       * reorder short-term ref pic  -subclause 8.2.4.3.1
       */

      absDiffPicNum = reorderCmdList[cmdNum].abs_diff_pic_num_minus1 + 1;

      /* Derive picNumNoWrap */
      if (reordering_of_pic_nums_idc == 0) {
        if (picNumPred - absDiffPicNum < 0)
          picNumNoWrap = picNumPred - absDiffPicNum + maxPicNum;
        else
          picNumNoWrap = picNumPred - absDiffPicNum;
      }
      else { /* reordering_of_pic_nums_idc == 1 */
        if (picNumPred + absDiffPicNum >= maxPicNum) 
          picNumNoWrap = picNumPred + absDiffPicNum - maxPicNum;
        else
          picNumNoWrap = picNumPred + absDiffPicNum;
      }

      /* Derive picNum */
      if (picNumNoWrap > currPicNum)
        picNum = picNumNoWrap - maxPicNum;
      else
        picNum = picNumNoWrap;

      /* Find short term picture with picture number picNum */
      for (i = 0; i < dpb->fullness; i++) {
        if (!dpb->buffers[i]->nonExisting &&
            dpb->buffers[i]->refType == FRM_SHORT_TERM_PIC &&
            dpb->buffers[i]->picNum == picNum)
          break;
      }

      /* If picNum was not found */
      if (i == dpb->fullness) {
        PRINT((_L("The short term ref pic(%d) is not found!\n"), picNum));
        return SLICE_ERR_ILLEGAL_VALUE;
      }

      /* Shift remaining pictures later in the list */
      for (cIdx = numRefPicActive; cIdx > refIdx; cIdx--)
        refPicList[cIdx] = refPicList[cIdx - 1];

      /* Place picture with number picNum into the index position refIdx */
      refPicList[refIdx++] = dpb->buffers[i];

      /* Remove duplicate of the inserted picture */
      nIdx = refIdx;
      for (cIdx = refIdx; cIdx <= numRefPicActive; cIdx++)
        if (refPicList[cIdx]->refType == FRM_LONG_TERM_PIC || refPicList[cIdx]->picNum != picNum)
          refPicList[nIdx++] = refPicList[cIdx];

      picNumPred = picNumNoWrap;
    }

    else if (reordering_of_pic_nums_idc == 2) {

      /*
       * reorder long-term ref pic  -subclause 8.2.4.3.2
       */

      /* Get long-term picture number */
      longTermPicNum = reorderCmdList[cmdNum].long_term_pic_num;

      /* Find long-term picture with picture number longTermPicNum */
      for (i = 0; i < dpb->fullness; i++)
        if (dpb->buffers[i]->refType == FRM_LONG_TERM_PIC &&
            dpb->buffers[i]->longTermPicNum == longTermPicNum)
          break;

      if (i == dpb->fullness) {
        // something wrong !
        PRINT((_L("The long term ref pic(%d) is not found!\n"), longTermPicNum));
        return SLICE_ERR_ILLEGAL_VALUE;
      }

      /* Shift remaining pictures later in the list */
      for (cIdx = numRefPicActive; cIdx > refIdx; cIdx--)
        refPicList[cIdx] = refPicList[cIdx - 1];

      /* Place picture with number longTermPicNum into the index position refIdx */
      refPicList[refIdx++] = dpb->buffers[i];

      /* Remove duplicate of the inserted picture */
      nIdx = refIdx;
      for (cIdx = refIdx; cIdx <= numRefPicActive; cIdx++)
        if (refPicList[cIdx]->refType == FRM_SHORT_TERM_PIC ||
            refPicList[cIdx]->longTermPicNum != longTermPicNum)
        {
          refPicList[nIdx++] = refPicList[cIdx];
        }
    }

    cmdNum++;

  } while (reordering_of_pic_nums_idc != 3 && cmdNum < MAX_NUM_OF_REORDER_CMDS);


  refPicList[numRefPicActive] = 0;

  return SLICE_OK;
}


// sliceParseMacroblocks
// Parses the macroblocks one by one in the input slice. 
TInt sliceParseMacroblocks(slice_s *slice, frmBuf_s *recoBuf, dpb_s *dpb,
                           pic_parameter_set_s *pps,
                           mbAttributes_s *mbData, TInt sliceID,
                           bitbuffer_s *bitbuf,
                           TBool aBitShiftInSlice)
{
  	frmBuf_s *refPicList0[DPB_MAX_SIZE+1];
  	macroblock_s mb;
  	TInt numRefFrames;
  	TInt numExistingRefFrames;
//  	TInt refFramesExist;
  	TInt mbIdxY;
  	TInt mbIdxX;
  	TInt mbksPerLine;
  	TInt mbksPerCol;
  	TInt picSizeInMbs;
  	TInt currMbAddr;
  	TInt sliceGroupNum;
  	void *stream;
  	TInt retCode;

  	// Choose number of reference frames and build reference picture list 
  	numRefFrames   = 0;
//  	refFramesExist = 0;
  	if (!IS_SLICE_I(slice->slice_type)) 
  	{	
    	if (slice->num_ref_idx_active_override_flag)
      		numRefFrames = slice->num_ref_idx_l0_active_minus1 + 1;
    	else
      		numRefFrames = pps->num_ref_idx_l0_active_minus1 + 1;

    	numExistingRefFrames = sliceInitRefPicList(dpb, refPicList0);

    	if (numExistingRefFrames < numRefFrames) 
    	{
//      		refFramesExist = sliceFixRefPicList(dpb, refPicList0, numRefFrames, numExistingRefFrames,
//            	                                recoBuf->width, recoBuf->height);
    	}
    	else
//      		refFramesExist = 1;

    	if (slice->ref_pic_list_reordering_flag0 && numExistingRefFrames > 0) 
    	{
      		retCode = refPicListReordering(slice, dpb, refPicList0, numRefFrames, slice->reorderCmdList);
      		
      		if (retCode < 0)
        		return retCode;
    	}
  	}

  	mbksPerLine = recoBuf->width/MBK_SIZE;
  	mbksPerCol  = recoBuf->height/MBK_SIZE;
  	picSizeInMbs = mbksPerLine*mbksPerCol;

  	currMbAddr = slice->first_mb_in_slice;
  	sliceGroupNum = sliceID & 0xF;

  	mbIdxY = currMbAddr / mbksPerLine;
  	mbIdxX = currMbAddr - mbIdxY*mbksPerLine;

  	mbkSetInitialQP(&mb, slice->qp, pps->chroma_qp_index_offset);

  	stream = bitbuf;

  	// Loop until all macroblocks in current slice have been decoded 
  	// If there has been a bitshift in the slice, we must go through 
  	// the macroblocks to see if any PCM coded MB are used.
  	if(aBitShiftInSlice)
  	{
  	
  		do 
  		{

    		// Store slice ID for current macroblock 
    		mbData->sliceMap[currMbAddr] = sliceID;

    		// Store loopfilter mode 
    		mbData->filterModeTab[currMbAddr] = (int8) slice->disable_deblocking_filter_idc;
    		mbData->alphaOffset[currMbAddr]   = (int8) (slice->slice_alpha_c0_offset_div2*2);
    		mbData->betaOffset[currMbAddr]    = (int8) (slice->slice_beta_offset_div2*2);

    		retCode = mbkParse(&mb, numRefFrames,
            		           mbData, recoBuf->width, 
                    		   slice->slice_type, pps->constrained_intra_pred_flag,
                       		   pps->chroma_qp_index_offset,
		                       mbIdxX, mbIdxY, stream, slice->bitOffset);

			if (retCode == MBK_PCM_FOUND)
			{
				// We can stop parsing this slice
				// Check later if this is the case also if we have slices in more than one NAL (is it even possible?)!!!
				return SLICE_OK;
			}
	
    		if (retCode < 0)
      			return SLICE_ERROR;

      		// If end of slice data has been reached and there are no 
      		// skipped macroblocks left, stop decoding slice.         
      		if (!bibMoreRbspData(bitbuf) && mb.numSkipped <= 0)
        		break;

    		// Find next mb address in the current slice group 
    		do 
    		{
      			// Next mb address 
      			currMbAddr++;

      			// If end of frame was reached, stop search 
      			if (currMbAddr == picSizeInMbs)
        			break;

      			// Update mb location 
      			mbIdxX++;
      			if (mbIdxX == mbksPerLine) 
      			{
        			mbIdxX = 0;
        			mbIdxY++;
      			}

    		} while ((mbData->sliceMap[currMbAddr] & 0xF) != sliceGroupNum);

    	// If end of frame was reached, stop decoding slice 
  		} while (currMbAddr < picSizeInMbs);
  	}

  	return SLICE_OK;
}


// EncodeUnsignedNBits
// Encodes the input aValue to the bit buffer with aLength bits.
void EncodeUnsignedNBits(bitbuffer_s *aBitBuffer, TUint aValue, TUint aLength)
{
	TUint tempValue;
	TInt i;
	TUint8 byteValue;
	 
	if(aBitBuffer->bitpos == 0)
	{
		// Get the next byte
		aBitBuffer->currentBits = aBitBuffer->data[aBitBuffer->bytePos];
		aBitBuffer->bytePos++;
		aBitBuffer->bitpos = 8;
	}
		
	// Write aValue bit by bit to the bit buffer
	for (i=aLength-1; i>=0; i--)
	{
		// Get the ith bit from aValue
		tempValue = (aValue & (1 << i)) >> i;
		
		// Zero out the bitpos bit
		byteValue = aBitBuffer->data[aBitBuffer->bytePos-1] & ~(1<<(aBitBuffer->bitpos-1));
		byteValue |= tempValue << (aBitBuffer->bitpos-1);
		
		aBitBuffer->data[aBitBuffer->bytePos-1] = byteValue;
		aBitBuffer->bitpos--;
		
		if(aBitBuffer->bitpos == 0)
		{
			// Get the next byte
			aBitBuffer->currentBits = aBitBuffer->data[aBitBuffer->bytePos];

			aBitBuffer->bytePos++;
			aBitBuffer->bitpos = 8;
		}
	}
	
	// Make sure the bit buffer currentBits is up-to-date
	aBitBuffer->currentBits = aBitBuffer->data[aBitBuffer->bytePos-1];
}


// ParseSliceHeader
// Parses the input slice header. PPS Id, frame numbering and POC LSB are modified if required.
TInt ParseSliceHeader(slice_s *slice, seq_parameter_set_s *spsList[],
                      pic_parameter_set_s *ppsList[], bitbuffer_s *bitbuf, 
                      TUint* aFrameNumber, TUint aFrameFromEncoder)
{
  	seq_parameter_set_s *sps;
  	pic_parameter_set_s *pps;
  	TUint picSizeInMapUnits;
  	TUint temp, temp2;
  	TInt sliceQp, len1;
  	TInt retCode;
  	TInt shiftedBits = 0;


  	slice->picHasMMCO5 = 0;
  
  	// Initialize the bit offset to zero
  	slice->bitOffset = 0;

  	// First macroblock in slice 
  	if ((retCode = ue_v(bitbuf, &slice->first_mb_in_slice, 65535)) < 0)
    	return retCode;

  	// Slice type 
  	if ((retCode = ue_v(bitbuf, &slice->slice_type, SLICE_MAX)) < 0)
    	return retCode;

  	// PPS id 
  	if ((retCode = ue_v(bitbuf, &slice->pic_parameter_set_id, PS_MAX_NUM_OF_PPS-1)) < 0)
    	return retCode;
  
  	pps = ppsList[slice->pic_parameter_set_id];

  	if (pps == NULL) 
  	{
    	PRINT((_L("Error: referring to non-existing PPS.\n")));     
    		return SLICE_ERR_NON_EXISTING_PPS;
  	}

	syncBitBufferBitpos(bitbuf);
	
  	// If the index has been changed the new index must be coded instead of 
  	// the old one to the slice header
  	if (pps->indexChanged)
  	{
  		// We have to encode the new PPS Id to the bitbuffer
  		TUint oldPPSId = slice->pic_parameter_set_id;
  		TUint newPPSId;
  	
  		if ( aFrameFromEncoder )
  			newPPSId = pps->encPPSId;
  		else
  			newPPSId = pps->origPPSId;
  	
		TUint trailingBits = GetNumTrailingBits(bitbuf);
		TInt diff = 0;
	
		TUint oldIdLength = ReturnUnsignedExpGolombCodeLength(oldPPSId);
		TUint newIdLength = ReturnUnsignedExpGolombCodeLength(newPPSId);
	
		// Signal that the slice data has been modified
		slice->sliceDataModified = 1;    
    
		// Get the new pps
		pps = ppsList[newPPSId];
		
		if(trailingBits > 8)
		{
			trailingBits = 8;
		}
		
		if ( oldIdLength == newIdLength )	
		{
			// Just encode the new Id on top of the old Id
			bitbuf->bitpos += oldIdLength;
			
			if(bitbuf->bitpos > 8)
			{	
				// Go to the right byte and bit position
				bitbuf->bytePos -= bitbuf->bitpos / 8;
				bitbuf->bitpos = bitbuf->bitpos % 8;
			}
			
			EncodeUnsignedExpGolombCode(bitbuf, newPPSId);
		}
		else if ( oldIdLength < newIdLength )
		{
			diff = newIdLength - oldIdLength;
		
			// Positive bit offset indicates bit-wise shift to right
			slice->bitOffset = (diff % 8);
		
			ShiftBufferRight(bitbuf, diff, trailingBits, oldIdLength);
		
			// After shifting, encode the new value to the bit buffer
			EncodeUnsignedExpGolombCode(bitbuf, newPPSId);
		}
		else
		{
			// New id's length is smaller than old id's length
			diff = oldIdLength - newIdLength;
	
			// Negative bit offset indicates bit-wise shift to left
			slice->bitOffset = -(diff % 8);
		
			ShiftBufferLeft(bitbuf, diff, oldIdLength);
		
			// After shifting, encode the new value to the bit buffer
			EncodeUnsignedExpGolombCode(bitbuf, newPPSId);
		}
	
		shiftedBits = diff;
  	}

  	sps = spsList[pps->seq_parameter_set_id];
  	
  	if (sps == NULL) 
  	{
    	PRINT((_L("Error: referring to non-existing SPS.\n")));     
    	return SLICE_ERR_NON_EXISTING_SPS;
  	}

  	picSizeInMapUnits = (sps->pic_width_in_mbs_minus1+1) * (sps->pic_height_in_map_units_minus1+1);

  	if (slice->first_mb_in_slice >= picSizeInMapUnits)
    	return SLICE_ERR_ILLEGAL_VALUE;

  	// Maximum frame number 
  	slice->maxFrameNum = (TUint)1 << (sps->log2_max_frame_num_minus4+4);
  
  	// IDR type NAL unit shall have frame number as zero
	if ( slice->nalType == NAL_TYPE_CODED_SLICE_IDR )
	{
		// Reset frame number for an IDR slice
	 	*aFrameNumber = 0;	
	}
	else if ( *aFrameNumber == (slice->maxFrameNum - 1 ) )
	{
		// Reset frame number if maximum frame number has been reached
	 	*aFrameNumber = 0;	
	}
	else if( !slice->first_mb_in_slice )
	{
	    (*aFrameNumber)++;	// Increment frame number only if this is the first mb in slice
	}
	

  	if (sps->maxFrameNumChanged)
  	{
  		// Frame number field size
  		TUint oldFrameNum;
  		TUint newFrameNum = sps->log2_max_frame_num_minus4+4;
  	
  		if ( aFrameFromEncoder )
  			oldFrameNum = sps->encMaxFrameNum+4;
  		else
  			oldFrameNum = sps->origMaxFrameNum+4;
  	
		TUint trailingBits = GetNumTrailingBits(bitbuf);
		TInt diff = 0;
	
		// Signal that the slice data has been modified
		slice->sliceDataModified = 1;    
    
		if(trailingBits > 8)
		{
			trailingBits = 8;
		}
		
		// If the size of the frame number field has changed then shift bits in the buffer
		if ( oldFrameNum < newFrameNum )
		{
			diff = newFrameNum - oldFrameNum;
		
			// Positive bit offset indicates bit-wise shift to right
			slice->bitOffset += (diff % 8);
		
			ShiftBufferRight(bitbuf, diff, trailingBits, 0);
		}
		else if ( oldFrameNum > newFrameNum )
		{
			// New id's length is smaller than old id's length
			diff = oldFrameNum - newFrameNum;
	
			// Negative bit offset indicates bit-wise shift to left
			slice->bitOffset -= (diff % 8);
		
			ShiftBufferLeft(bitbuf, diff, 0);
		}
	
		shiftedBits += diff;
  	}
  
  	// Encode the new frame number here
  	EncodeUnsignedNBits(bitbuf, *aFrameNumber, sps->log2_max_frame_num_minus4+4);
  
  	slice->frame_num = *aFrameNumber;

  	// IDR picture 
  	if (slice->isIDR) 
  	{
    	if ((retCode = ue_v(bitbuf, &slice->idr_pic_id, 65535)) < 0)
      		return retCode;
  	}

	syncBitBufferBitpos(bitbuf);
	
  	// POC parameters 
	if (sps->pic_order_cnt_type == 0) 
  	{
	  	if (sps->maxPOCNumChanged)
  		{
  			// POC lsb number 
  			TUint oldPOCNum;
  			TUint newPOCNum = sps->log2_max_pic_order_cnt_lsb_minus4+4;
  	
  			if ( aFrameFromEncoder )
  				oldPOCNum = sps->encMaxPOCNum+4;
  			else
  				oldPOCNum = sps->origMaxPOCNum+4;
  	
			TUint trailingBits = GetNumTrailingBits(bitbuf);
			TInt diff = 0;
	
			// Signal that the slice data has been modified
			slice->sliceDataModified = 1;    
    
			if (trailingBits > 8)
			{
				trailingBits = 8;
			}
		
			// If the size of the POC lsb field has changed then shift bits in the buffer
			if ( oldPOCNum < newPOCNum )
			{	
				diff = newPOCNum - oldPOCNum;
		
				// Positive bit offset indicates bit-wise shift to right
				slice->bitOffset += (diff % 8);
		
				ShiftBufferRight(bitbuf, diff, trailingBits, 0);

			}
			else if ( oldPOCNum > newPOCNum )
			{
				// New id's length is smaller than old id's length
				diff = oldPOCNum - newPOCNum;
	
				// Negative bit offset indicates bit-wise shift to left
				slice->bitOffset -= (diff % 8);
		
				ShiftBufferLeft(bitbuf, diff, 0);
			
			}
	
			shiftedBits += diff;
  		}
  
	  	// For now encode the POC as the frame number
  		EncodeUnsignedNBits(bitbuf, *aFrameNumber, sps->log2_max_pic_order_cnt_lsb_minus4+4);
  
   		slice->delta_pic_order_cnt_bottom = 0;
    	
    	if (pps->pic_order_present_flag) 
    	{ //  && !field_pic_flag 
      		if ((retCode = se_v_long(bitbuf, &slice->delta_pic_order_cnt_bottom)) < 0)
        		return retCode;
    	}
  	}
  	else if (sps->pic_order_cnt_type == 1) 
  	{
    	slice->delta_pic_order_cnt_0 = 0;
    	slice->delta_pic_order_cnt_1 = 0;
    	// Read delta_pic_order_cnt[ 0 ] and delta_pic_order_cnt[ 1 ]
    	if (!sps->delta_pic_order_always_zero_flag) 
    	{
      		// delta_pic_order_cnt[ 0 ] 
      		if ((retCode = se_v_long(bitbuf, &slice->delta_pic_order_cnt_0)) < 0)
        		return retCode;
      		if (pps->pic_order_present_flag) 
      		{ //  delta_pic_order_cnt[ 1 ] 
        		if ((retCode = se_v_long(bitbuf, &slice->delta_pic_order_cnt_1)) < 0)
          			return retCode;
      		}
    	}
  	}

	// If we don't have to do any bit shifting (left or right) with the slice header, 
	// we can just stop parsing the header and return.
	if (shiftedBits == 0)
	{
		return SLICE_STOP_PARSING;
	}
	
  	// Redundant picture count
  	if (pps->redundant_pic_cnt_present_flag) 
  	{
    	if ((retCode = ue_v(bitbuf, &slice->redundant_pic_cnt, 127)) < 0)
      		return retCode;
  	}
  	else
    	slice->redundant_pic_cnt = 0;

  	// Reference picture management 
  	if (IS_SLICE_P(slice->slice_type)) 
  	{
    	if ((retCode = u_n(bitbuf, 1, &slice->num_ref_idx_active_override_flag)) < 0)
      		return retCode;
    	if (slice->num_ref_idx_active_override_flag) 
    	{
      		if ((retCode = ue_v(bitbuf, &slice->num_ref_idx_l0_active_minus1, DPB_MAX_SIZE-1)) < 0)
        		return retCode;
    	}
  	}

  	// Reordering the reference picture list 
  	retCode = getRefPicListReorderingCmds(slice, sps->num_ref_frames, bitbuf);

	if (retCode < 0)
    	return retCode;

  	// Read the MMCO commands, but not do the operations until all the slices in current picture are decoded 
  	if (slice->nalRefIdc) 
  	{
    	retCode = getDecRefPicMarkingCmds(slice, sps->num_ref_frames, bitbuf);
    	if (retCode < 0)
      		return retCode;
  	}

  	// Slice quant 
  	if ((retCode = se_v(bitbuf, &slice->slice_qp_delta, -MAX_QP, MAX_QP)) < 0)
    	return retCode;

  	sliceQp = pps->pic_init_qp_minus26 + 26 + slice->slice_qp_delta;
  	if (sliceQp < MIN_QP || sliceQp > MAX_QP) 
  	{
    	PRINT((_L("Error: illegal slice quant.\n")));     
    	return SLICE_ERR_ILLEGAL_VALUE;
  	}
  	
  	slice->qp = sliceQp;

  	// Deblocking filter 
  	slice->disable_deblocking_filter_idc = 0;
  	slice->slice_alpha_c0_offset_div2    = 0;
  	slice->slice_beta_offset_div2        = 0;

  	if (pps->deblocking_filter_parameters_present_flag == 1) 
  	{

    	if ((retCode = ue_v(bitbuf, &slice->disable_deblocking_filter_idc, 2)) < 0)
      		return retCode;

    	if (slice->disable_deblocking_filter_idc != 1) 
    	{
      		if ((retCode = se_v(bitbuf, &slice->slice_alpha_c0_offset_div2, MIN_ALPHA_BETA_OFFSET, MAX_ALPHA_BETA_OFFSET)) < 0)
        		return retCode;
      		if ((retCode = se_v(bitbuf, &slice->slice_beta_offset_div2, MIN_ALPHA_BETA_OFFSET, MAX_ALPHA_BETA_OFFSET)) < 0)
        		return retCode;
    	}
  	}

  	// Read slice_group_change_cycle 
  	if (pps->num_slice_groups_minus1 > 0 && pps->slice_group_map_type >= 3 &&
        pps->slice_group_map_type <= 5)
  	{
    	// len = Ceil( Log2( PicSizeInMapUnits / SliceGroupChangeRate + 1 ) ) 
	    // PicSizeInMapUnits / SliceGroupChangeRate 
    	temp = picSizeInMapUnits / (pps->slice_group_change_rate_minus1+1);

    	// Calculate Log2 
    	temp2 = (temp + 1) >> 1;
    	for (len1 = 0; len1 < 16 && temp2 != 0; len1++)
      		temp2 >>= 1;

    	// Calculate Ceil 
    	if ( (((unsigned)1) << len1) < (temp + 1) )
      		len1++;

    	if ((retCode = u_n(bitbuf, len1, &slice->slice_group_change_cycle)) < 0)
      		return retCode;

    	// Ceil( PicSizeInMapUnits/SliceGroupChangeRate ) 
    	if (temp * (pps->slice_group_change_rate_minus1+1) != picSizeInMapUnits)
      		temp++;

    	// The value of slice_group_change_cycle shall be in the range of  
    	// 0 to Ceil( PicSizeInMapUnits/SliceGroupChangeRate ), inclusive. 
    	if (slice->slice_group_change_cycle > temp)
      		return SLICE_ERR_ILLEGAL_VALUE;
  	}

  	return SLICE_OK;
}

