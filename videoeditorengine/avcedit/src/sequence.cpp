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


#include "globals.h"
#include "bitbuffer.h"
#include "vld.h"
#include "framebuffer.h"
#include "avcdapi.h"
#include "parameterset.h"
#include "dpb.h"
#include "sequence.h"


#define SEQ_OK                      0
#define SEQ_ERROR                  -1
#define SEQ_ERR_MEM                -2
#define SEQ_ERR_GAPS_IN_FRAME_NUM  -3
#define SEQ_ERR_DPB_CORRUPTED      -4

// Definitions of SEI types
#define SEI_TYPE_SCENE_INFO       9


#ifdef CHECK_MV_RANGE
int maxVerticalMvRange;
#endif


void GenerateEmptyFrame(sequence_s *seq, bitbuffer_s *bitbuf, TUint aFrameNumber);



/*
 * setLower4Bits:
 *
 * Parameters:
 *      value               the destination to store the 4 bits
 *      bits                the 4 bits to be copied
 *
 * Function:
 *      Assign the value to the lowest 4 bits
 *
 *  Return:
 *      -
 */
#define setLower4Bits(value, bits) (value) = ((value) & ~0xF) | ((bits))


/*
 *
 * avcdOpen:
 *
 * Parameters:
 *      -
 *
 * Function:
 *      Open AVC decoder.
 *      
 * Returns:
 *      Pointer to initialized avcdDecoder_t object
 */
avcdDecoder_t *avcdOpen()
{
  sequence_s *seq;


  // Allocate sequence object
  if ((seq = (sequence_s *)User::Alloc(sizeof(sequence_s))) == NULL)
    return NULL;

  memset(seq, 0, sizeof(sequence_s));
  
  // Open bitbuffer
  if ((seq->bitbuf = bibOpen()) == NULL)
    return NULL;

  seq->isFirstSliceOfSeq       = 1;
  seq->unusedShortTermFrameNum = -1;

#ifdef VIDEOEDITORENGINE_AVC_EDITING
  // Open slices
  if ((seq->currSlice = sliceOpen()) == NULL)
    return NULL;

  if ((seq->nextSlice = sliceOpen()) == NULL)
    return NULL;

  // Open dpb
  if ((seq->dpb = dpbOpen()) == NULL)
    return NULL;

	seq->iFrameNumber = 0;
	seq->iFromEncoder = 0;
	seq->iEncodeUntilIDR = 0;
	seq->iBitShiftInSlice = 0;
	seq->iPreviousPPSId = 0;
	seq->iNumSPS = 0;
	seq->iNumPPS = 0;

	seq->iTotalFrameNumber = 0;

#endif  // VIDEOEDITORENGINE_AVC_EDITING
	
  return seq;
}


/*
 *
 * avcdClose:
 *
 * Parameters:
 *      seq                   Sequence object
 *
 * Function:
 *      Close sequence.
 *      
 * Returns:
 *      -
 */
void avcdClose(avcdDecoder_t *dec)
{
  sequence_s *seq = (sequence_s *)dec;
  
  /* Close bitbuffer */
  bibClose(seq->bitbuf);
  
  /* Close parameter sets */
  psCloseParametersSets(seq->spsList, seq->ppsList);

#ifdef VIDEOEDITORENGINE_AVC_EDITING
  /* Close current frame */
  frmClose(seq->recoBuf, seq->mbData);

  /* Close decoded picture buffer */
  dpbClose(seq->dpb);

  /* Close slices */
  sliceClose(seq->currSlice);
  sliceClose(seq->nextSlice);

#endif  // VIDEOEDITORENGINE_AVC_EDITING

  User::Free(seq);
}


/*
 *
 * initSPSParsing
 *
 * Parameters:
 *      dec                   Sequence object
 *      nauUnitBits           Buffer containing SPS NAL unit
 *      nalUnitLen            Length of buffer
 *
 * Function:
 *      Initialises bit buffer for parsing, checks
 *      nal_unit_type
 *      
 * Returns:
 *      KErrNone for no error, negative value for error
 */
TInt initSPSParsing(avcdDecoder_t *dec, void *nalUnitBits, TUint* nalUnitLen)
{
    sequence_s *seq = (sequence_s *)dec;

    TInt nalHeaderByte;
    TInt nalType;
    TUint nalUnitLength = *nalUnitLen;
    
    // tempData allocation
    TUint8* tempData = (TUint8*) User::Alloc(nalUnitLength);
  	
  	if (!tempData)
  	    return KErrNoMemory;
  	
  	Mem::Copy(tempData, nalUnitBits, nalUnitLength);  	  	

    if (bibInit(seq->bitbuf, tempData, nalUnitLength) < 0)
    {
        User::Free(seq->bitbuf->data);
        return AVCD_ERROR;
    }
    
    if (bibGetByte(seq->bitbuf, &nalHeaderByte))
    {
        User::Free(seq->bitbuf->data);
        return AVCD_ERROR;
    }
    
    // Decode NAL unit type
    nalType   = nalHeaderByte & 0x1F;    
    
    if (nalType != NAL_TYPE_SPS)
    {
        User::Free(seq->bitbuf->data);
        return AVCD_ERROR;
    }
    
    return KErrNone;
}

/*
 *
 * avcdParseLevel
 *
 * Parameters:
 *      dec                   Sequence object
 *      nauUnitBits           Buffer containing SPS NAL unit
 *      nalUnitLen            Length of buffer
 *      aLevel                [output]Parsed level
 *
 * Function:
 *      Parses AVC level from SPS
 *      
 * Returns:
 *      KErrNone for no error, negative value for error
 */
TInt avcdParseLevel(avcdDecoder_t *dec, void *nalUnitBits, TUint* nalUnitLen, TInt& aLevel)                    
{

    TInt error = initSPSParsing(dec, nalUnitBits, nalUnitLen);
    
    if (error < 0)
    	return AVCD_ERROR;

    sequence_s *seq = (sequence_s *)dec;

    error = psParseLevelFromSPS(seq->bitbuf, aLevel);

    User::Free(seq->bitbuf->data);
    
    if (error < 0)
    	return AVCD_ERROR;
    
    return KErrNone;    	    

}

/*
 *
 * avcdParseResolution
 *
 * Parameters:
 *      dec                   Sequence object
 *      nauUnitBits           Buffer containing SPS NAL unit
 *      nalUnitLen            Length of buffer
 *      aResolution           [output]Parsed resolution
 *
 * Function:
 *      Parses resolution from SPS
 *      
 * Returns:
 *      KErrNone for no error, negative value for error
 */
TInt avcdParseResolution(avcdDecoder_t *dec, void *nalUnitBits, TUint* nalUnitLen, 
                         TInt& aWidth, TInt& aHeight)
{

    TInt error = initSPSParsing(dec, nalUnitBits, nalUnitLen);
    
    if (error < 0)
    	return AVCD_ERROR;

    sequence_s *seq = (sequence_s *)dec;

    error = psDecodeSPS(seq->bitbuf, seq->spsList, aWidth, aHeight);

    User::Free(seq->bitbuf->data);
    
    if (error < 0)
    	return AVCD_ERROR;
    
    return KErrNone;    	    
    
}



#ifdef VIDEOEDITORENGINE_AVC_EDITING

/*
 *
 * slidingWindowDecRefPicMarking:
 *
 * Parameters:
 *      seq                   Sequence object
 *
 * Function:
 *      Sliding window decoded reference picture marking. Reference pictures
 *      in dpb are marked based on first in first out principle.
 *      
 * Returns:
 *      SEQ_OK for no error, negative value for error
 */
static int slidingWindowDecRefPicMarking(sequence_s *seq)
{
  dpb_s *dpb;
  int numRefPics;

  dpb = seq->dpb;

  numRefPics = dpb->numShortTermPics + dpb->numLongTermPics;

  /* If dpb contains maximum number of reference pitures allowed, short */
  /*  term reference picture with lowest picture number is removed.     */
  if (numRefPics == dpb->maxNumRefFrames) {
    if (dpb->numShortTermPics == 0) {
      PRINT((_L("numShortTerm must be greater than zero\n")));
      return SEQ_ERR_DPB_CORRUPTED;
    }

    dpbMarkLowestShortTermPicAsNonRef(dpb);
  }

  return SEQ_OK;
}


/*
 *
 * adaptiveDecRefPicMarking:
 *
 * Parameters:
 *      seq                   Sequence object
 *
 * Function:
 *      Adaptive decoded reference picture marking. Reference pictures in dpb
 *      are marked based on memory management command operations that were
 *      decoded in slice header earlier.
 *      
 * Returns:
 *      SEQ_OK for no error, SEQ_ERR_DPB_CORRUPTED for error in DPB
 */
static int adaptiveDecRefPicMarking(sequence_s *seq)
{
  dpb_s *dpb;
  sliceMMCO_s *mmcoCmdList;
  int32 currPicNum, picNumX;
  int i;

  dpb = seq->dpb;
  currPicNum = seq->currSlice->frame_num;
  mmcoCmdList = seq->currSlice->mmcoCmdList;

  i = 0;
  do {
    switch (mmcoCmdList[i].memory_management_control_operation) {
      case 1:
        picNumX = currPicNum - (mmcoCmdList[i].difference_of_pic_nums_minus1 + 1);
        if (dpbMarkShortTermPicAsNonRef(dpb, picNumX) < 0)
          return SEQ_ERR_DPB_CORRUPTED;
        break;
      case 2:
        if (dpbMarkLongTermPicAsNonRef(dpb, mmcoCmdList[i].long_term_pic_num) < 0)
          return SEQ_ERR_DPB_CORRUPTED;
        break;
      case 3:
        picNumX = currPicNum - (mmcoCmdList[i].difference_of_pic_nums_minus1 + 1);
        if (dpbMarkShortTermPicAsLongTerm(dpb, picNumX, mmcoCmdList[i].long_term_frame_idx) < 0)
          return SEQ_ERR_DPB_CORRUPTED;
        break;
      case 4:
        dpbSetMaxLongTermFrameIdx(dpb, mmcoCmdList[i].max_long_term_frame_idx_plus1);
        break;
      case 5:
        dpbMarkAllPicsAsNonRef(dpb);
        dpb->maxLongTermFrameIdx = -1;
        break;
      case 6:
        /* To avoid duplicate of longTermFrmIdx */
        dpbVerifyLongTermFrmIdx(dpb, mmcoCmdList[i].long_term_frame_idx);

        seq->recoBuf->refType        = FRM_LONG_TERM_PIC;
        seq->recoBuf->longTermFrmIdx = mmcoCmdList[i].long_term_frame_idx;
        break;
    }
    i++;
  } while (mmcoCmdList[i].memory_management_control_operation != 0 && i < MAX_NUM_OF_MMCO_OPS);

  return SEQ_OK;
}


/*
 *
 * decRefPicMarking:
 *
 * Parameters:
 *      seq                   Sequence object
 *
 * Function:
 *      Decoded reference picture marking. Reference pictures in dpb are marked
 *      differently depending on whether current picture is IDR picture or not
 *      and whether it is reference picture or non-reference picture.
 *      If current picture is non-IDR reference picture, reference pictures are
 *      marked with either sliding window marking process or adaptive marking
 *      process depending on the adaptiveRefPicMarkingModeFlag flag.
 *      
 * Returns:
 *      -
 */
static int decRefPicMarking(sequence_s *seq)
{
  slice_s *slice;
  frmBuf_s *recoBuf;

  slice = seq->currSlice;
  recoBuf = seq->recoBuf;

  recoBuf->refType  = FRM_SHORT_TERM_PIC;
  recoBuf->frameNum = slice->frame_num;
  recoBuf->hasMMCO5 = slice->picHasMMCO5;
  recoBuf->isIDR    = slice->isIDR;

  if (slice->isIDR) {
    recoBuf->idrPicID = slice->idr_pic_id;

    /* All reference frames are marked as non-reference frames */
    dpbMarkAllPicsAsNonRef(seq->dpb);

    /* Set reference type for current picture */
    if (!slice->long_term_reference_flag) {
      seq->dpb->maxLongTermFrameIdx = -1;
    }
    else {
      recoBuf->refType         = FRM_LONG_TERM_PIC;
      recoBuf->longTermFrmIdx  = 0;
      seq->dpb->maxLongTermFrameIdx = 0;
    }
  }
  else if (slice->nalRefIdc != 0) {
    if (!slice->adaptive_ref_pic_marking_mode_flag)
      return slidingWindowDecRefPicMarking(seq);
    else
      return adaptiveDecRefPicMarking(seq);
  }
  else
    recoBuf->refType  = FRM_NON_REF_PIC;

  return SEQ_OK;
} 


/*
 *
 * buildSliceGroups:
 *
 * Parameters:
 *      seq                   Sequence object
 *      slice                 Slice object
 *      sps                   Sequence parameter set
 *      pps                   Picture parameter set
 *
 * Function:
 *      Build slice group map. Syntax elements for slice groups are
 *      in active picture parameter set.
 *      
 * Returns:
 *      -
 *
 */
static void buildSliceGroups(sequence_s* seq, slice_s *slice,
                             seq_parameter_set_s *sps, pic_parameter_set_s *pps)
{
  int xTopLeft, yTopLeft;
  int xBottomRight, yBottomRight;
  int x, y;
  int leftBound, topBound;
  int rightBound, bottomBound;
  int xDir, yDir;
  int mapUnitsInSliceGroup0;
  int mapUnitVacant;
  int sizeOfUpperLeftGroup;
  int iGroup, picSizeInMapUnits;
  int picWidthInMbs, picHeightInMapUnits;
  int i, j, k;
  int *sliceMap;

  sliceMap = seq->mbData->sliceMap;

  picWidthInMbs = sps->pic_width_in_mbs_minus1+1;
  picHeightInMapUnits = sps->pic_height_in_map_units_minus1+1;

  picSizeInMapUnits = picWidthInMbs * picHeightInMapUnits;

  if (pps->num_slice_groups_minus1 == 0) {
    /* Only one slice group */
    for (i = 0; i < picSizeInMapUnits; i++)
      sliceMap[i] = 0;
  }
  else {
    /* There are more than one slice groups in this picture */

    switch (pps->slice_group_map_type) {

    case PS_SLICE_GROUP_MAP_TYPE_INTERLEAVED:
      i = 0;
      do {
        for (iGroup = 0; iGroup <= (int)pps->num_slice_groups_minus1 && i < picSizeInMapUnits;
          i += pps->run_length_minus1[iGroup++] + 1)
        {
          for (j = 0; j <= (int)pps->run_length_minus1[iGroup] && i+j < picSizeInMapUnits; j++)
            sliceMap[i+j] = iGroup;   /* Only the group number */
        }
      } while (i < picSizeInMapUnits);
      break;

    case PS_SLICE_GROUP_MAP_TYPE_DISPERSED:
      for ( i = 0; i < picSizeInMapUnits; i++ )
        sliceMap[i] = ( ( i % picWidthInMbs ) + 
        ( ( ( i / picWidthInMbs ) * ( pps->num_slice_groups_minus1 + 1 ) ) / 2 ) )
        % ( pps->num_slice_groups_minus1 + 1 );
      break;

    case PS_SLICE_GROUP_MAP_TYPE_FOREGROUND:
      for (i = 0; i < picSizeInMapUnits; i++)
        setLower4Bits(sliceMap[i], pps->num_slice_groups_minus1);
      for (iGroup = pps->num_slice_groups_minus1 - 1; iGroup >= 0; iGroup--) {
        yTopLeft = pps->top_left[iGroup] / picWidthInMbs;
        xTopLeft = pps->top_left[iGroup] % picWidthInMbs;
        yBottomRight = pps->bottom_right[iGroup] / picWidthInMbs;
        xBottomRight = pps->bottom_right[iGroup] % picWidthInMbs;
        for (y = yTopLeft; y <= yBottomRight; y++)
          for (x = xTopLeft; x <= xBottomRight; x++)
            sliceMap[y * picWidthInMbs + x] = iGroup;
      }
      break;

    case PS_SLICE_GROUP_MAP_TYPE_CHANGING_3:
      /* mapUnitsInSliceGroup0 */
      mapUnitsInSliceGroup0 =	min((int)(slice->slice_group_change_cycle * (pps->slice_group_change_rate_minus1+1)), picSizeInMapUnits);

      for (i = 0; i < picSizeInMapUnits; i++)
        sliceMap[i] = 1; // mapUnitToSliceGroupMap[ i ] = 1;

      x = (picWidthInMbs - pps->slice_group_change_direction_flag) / 2;
      y = (picHeightInMapUnits - pps->slice_group_change_direction_flag ) / 2;
      // ( leftBound, topBound ) = ( x, y )
      leftBound = x; 
      topBound = y;
      // ( rightBound, bottomBound ) = ( x, y )
      rightBound = x; 
      bottomBound = y;
      // ( xDir, yDir ) = ( slice_group_change_direction_flag - 1, slice_group_change_direction_flag )
      xDir = pps->slice_group_change_direction_flag - 1;
      yDir = pps->slice_group_change_direction_flag;
      for (i = 0; i < mapUnitsInSliceGroup0; i += mapUnitVacant) {
        mapUnitVacant = ( (sliceMap[y * picWidthInMbs + x] & 0xF) == 1);
        if (mapUnitVacant)
          setLower4Bits(sliceMap[y * picWidthInMbs + x], 0);
        if (xDir == -1 && x == leftBound) {
          leftBound = max(leftBound - 1, 0);
          x = leftBound;
          //( xDir, yDir ) = ( 0, 2 * slice_group_change_direction_flag - 1 )
          xDir = 0;
          yDir = 2 * pps->slice_group_change_direction_flag - 1;
        } 
        else if (xDir == 1 && x == rightBound) {
          rightBound = min(rightBound + 1, picWidthInMbs - 1);
          x = rightBound;
          //( xDir, yDir ) = ( 0, 1 - 2 * slice_group_change_direction_flag )
          xDir = 0;
          yDir = 1 - 2 * pps->slice_group_change_direction_flag;
        } 
        else if (yDir == -1 && y == topBound) {
          topBound = max(topBound - 1, 0);
          y = topBound;
          //( xDir, yDir ) = ( 1 - 2 * slice_group_change_direction_flag, 0 )
          xDir = 1 - 2 * pps->slice_group_change_direction_flag;
          yDir = 0;
        } 
        else if (yDir == 1 && y == bottomBound) {
          bottomBound = min(bottomBound + 1, picHeightInMapUnits - 1);
          y = bottomBound;
          //( xDir, yDir ) = ( 2 * slice_group_change_direction_flag - 1, 0 )
          xDir = 2 * pps->slice_group_change_direction_flag - 1;
          yDir = 0;
        } 
        else {
          //( x, y ) = ( x + xDir, y + yDir )
          x = x + xDir;
          y = y + yDir;
        }
      }
      break;

    case PS_SLICE_GROUP_MAP_TYPE_CHANGING_4:
      /* mapUnitsInSliceGroup0 */
      mapUnitsInSliceGroup0 =	min((int)(slice->slice_group_change_cycle * (pps->slice_group_change_rate_minus1+1)), picSizeInMapUnits);

      sizeOfUpperLeftGroup = ( pps->slice_group_change_direction_flag ? 
			                        ( picSizeInMapUnits - mapUnitsInSliceGroup0 ) : mapUnitsInSliceGroup0 );

      for( i = 0; i < picSizeInMapUnits; i++ )
        if( i < sizeOfUpperLeftGroup )
          sliceMap[ i ] = pps->slice_group_change_direction_flag;
        else
          sliceMap[ i ] = 1 - pps->slice_group_change_direction_flag;
      break;

    case PS_SLICE_GROUP_MAP_TYPE_CHANGING_5:
      /* mapUnitsInSliceGroup0 */
      mapUnitsInSliceGroup0 =	min((int)(slice->slice_group_change_cycle * (pps->slice_group_change_rate_minus1+1)), picSizeInMapUnits);

      sizeOfUpperLeftGroup = ( pps->slice_group_change_direction_flag ? 
			                        ( picSizeInMapUnits - mapUnitsInSliceGroup0 ) : mapUnitsInSliceGroup0 );

      k = 0;
      for( j = 0; j < picWidthInMbs; j++ )
        for( i = 0; i < picHeightInMapUnits; i++ )
          if( k++ < sizeOfUpperLeftGroup )
            sliceMap[ i * picWidthInMbs + j ] = pps->slice_group_change_direction_flag;
          else
            sliceMap[ i * picWidthInMbs + j ] = 1 - pps->slice_group_change_direction_flag;
      break;

    case PS_SLICE_GROUP_MAP_TYPE_EXPLICIT:
      for (i = 0; i < picSizeInMapUnits; i++)
        sliceMap[i] = pps->slice_group_id[i];
      break;

    default:
      break;
    }
  }

}


/*
 *
 * isPicBoundary:
 *
 * Parameters:
 *      seq                   Sequence object
 *
 * Function:
 *      Check if current slice and next slice belong to different pictures.
 *      
 * Returns:
 *      1: slices belong to different pictures (picture boundary detected)
 *      0: slices belong to the same picture
 *
 */
static int isPicBoundary(sequence_s *seq)
{
  slice_s *currSlice, *nextSlice;
  seq_parameter_set_s *prevSps, *currSps;

  currSlice = seq->currSlice;
  nextSlice = seq->nextSlice;

  /* frame_num differs in value. */
  if (currSlice->frame_num != nextSlice->frame_num)
    return 1;

  /* nal_ref_idc differs in value with one of the nal_ref_idc values being equal to 0. */
  if ((currSlice->nalRefIdc != nextSlice->nalRefIdc) &&
      (currSlice->nalRefIdc == 0 || nextSlice->nalRefIdc == 0))
    return 1;

  /* nal_unit_type is equal to 5 for one coded slice NAL unit and */
  /* is not equal to 5 in the other coded slice NAL unit */
  if ((currSlice->nalType == NAL_TYPE_CODED_SLICE_IDR || nextSlice->nalType == NAL_TYPE_CODED_SLICE_IDR) &&
      (currSlice->nalType != nextSlice->nalType))
    return 1;

  /* nal_unit_type is equal to 5 for both and idr_pic_id differs in value. */
  if (currSlice->nalType == NAL_TYPE_CODED_SLICE_IDR &&
      nextSlice->nalType == NAL_TYPE_CODED_SLICE_IDR &&
      (currSlice->idr_pic_id != nextSlice->idr_pic_id))
    return 1;

  prevSps = seq->spsList[seq->ppsList[currSlice->pic_parameter_set_id]->seq_parameter_set_id];
  currSps = seq->spsList[seq->ppsList[nextSlice->pic_parameter_set_id]->seq_parameter_set_id];

  /* pic_order_cnt_type is equal to 0 for both and */
  /* either pic_order_cnt_lsb differs in value, or delta_pic_order_cnt_bottom differs in value. */
  if ((prevSps->pic_order_cnt_type == 0 && currSps->pic_order_cnt_type == 0) &&
    ((currSlice->pic_order_cnt_lsb != nextSlice->pic_order_cnt_lsb) ||
    (currSlice->delta_pic_order_cnt_bottom != nextSlice->delta_pic_order_cnt_bottom)))
    return 1;

  /* pic_order_cnt_type is equal to 1 for both and */
  /* either delta_pic_order_cnt[ 0 ] differs in value, or delta_pic_order_cnt[ 1 ] differs in value. */
  if ((prevSps->pic_order_cnt_type == 1 && currSps->pic_order_cnt_type == 1) &&
      ((currSlice->delta_pic_order_cnt_0 != nextSlice->delta_pic_order_cnt_0) ||
       (currSlice->delta_pic_order_cnt_1 != nextSlice->delta_pic_order_cnt_1)))
    return 1;

  return 0;
}


/*
 *
 * decodePictureOrderCount:
 *
 * Parameters:
 *      seq                   Sequence object
 *      slice                 Slice object
 *      sps                   Sequence parameter set
 *
 * Function:
 *      Decode picture order count using syntax elements in slice object.
 *      
 * Returns:
 *      poc
 *
 */
static int decodePictureOrderCount(sequence_s* seq, slice_s *slice,
                                   seq_parameter_set_s *sps)
{
  int i;
  int32 maxPocLsb;
  int32 expectedPicOrderCnt, picOrderCntCycleCnt = 0;
  int32 expectedDeltaPerPicOrderCntCycle, frameNumInPicOrderCntCycle = 0, absFrameNum;
  int32 tempPicOrderCnt;
  int32 poc = 0;

  /* POC */
  if (sps->pic_order_cnt_type == 0) {
    /* Reset prevPocMsb, prevPocLsb if needed */
    if (slice->isIDR || seq->prevPicHasMMCO5) {
      seq->prevPocMsb = seq->prevPocLsb = 0;
    }
    /* PicOrderCntMsb is derived: */
    maxPocLsb = (u_int32)1 << (sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
    if ( (int32)slice->pic_order_cnt_lsb < seq->prevPocLsb &&  (seq->prevPocLsb - (int32)slice->pic_order_cnt_lsb ) >= (maxPocLsb / 2) )
      seq->pocMsb = seq->prevPocMsb + maxPocLsb;
    else if ( (int32)slice->pic_order_cnt_lsb > seq->prevPocLsb && ((int32)slice->pic_order_cnt_lsb - seq->prevPocLsb) > (maxPocLsb / 2) )
      seq->pocMsb = seq->prevPocMsb - maxPocLsb;
    else
      seq->pocMsb = seq->prevPocMsb;
    /* poc */
    poc = seq->pocMsb + slice->pic_order_cnt_lsb;
  }

  else if (sps->pic_order_cnt_type == 1) {
    /* Reset prevFrameNumOffset if needed */
    if (!slice->isIDR && seq->prevPicHasMMCO5)  /* : prevPicHasMMCO5 has not been tested. */
      seq->prevFrameNumOffset = 0;

    /* frameNumOffset is derived as follows: */
    if (slice->isIDR)
      seq->frameNumOffset = 0;
    else if (seq->prevFrameNum > (int32)slice->frame_num)
      seq->frameNumOffset = seq->prevFrameNumOffset + slice->maxFrameNum;
    else
      seq->frameNumOffset = seq->prevFrameNumOffset;

    /* absFrameNum is derived as follows: */
    if (sps->num_ref_frames_in_pic_order_cnt_cycle != 0)
      absFrameNum = seq->frameNumOffset + slice->frame_num;
    else
      absFrameNum = 0;
    if (slice->nalRefIdc == 0 && absFrameNum > 0)
      absFrameNum = absFrameNum - 1;

    /* When absFrameNum > 0, picOrderCntCycleCnt and frameNumInPicOrderCntCycle are derived as follows */
    if (absFrameNum > 0) {
      picOrderCntCycleCnt = (absFrameNum - 1) / sps->num_ref_frames_in_pic_order_cnt_cycle;
      frameNumInPicOrderCntCycle = (absFrameNum - 1) % sps->num_ref_frames_in_pic_order_cnt_cycle;
    }

    /* expectedDeltaPerPicOrderCntCycle */
    expectedDeltaPerPicOrderCntCycle = 0;
    for (i = 0; i < (int)sps->num_ref_frames_in_pic_order_cnt_cycle; i++)
      expectedDeltaPerPicOrderCntCycle += sps->offset_for_ref_frame[i];

    /* expectedPicOrderCnt */
    if (absFrameNum > 0) {
      expectedPicOrderCnt = picOrderCntCycleCnt * expectedDeltaPerPicOrderCntCycle;
      for (i = 0; i <= frameNumInPicOrderCntCycle; i++)
        expectedPicOrderCnt = expectedPicOrderCnt + sps->offset_for_ref_frame[i];
    }
    else
      expectedPicOrderCnt = 0;
    if (slice->nalRefIdc == 0)
      expectedPicOrderCnt = expectedPicOrderCnt + sps->offset_for_non_ref_pic;

    /* poc */
    poc = expectedPicOrderCnt + slice->delta_pic_order_cnt_0;
  }

  else if (sps->pic_order_cnt_type == 2) {
    /* prevFrameNumOffset is derived as follows */
    if (!slice->isIDR && seq->prevPicHasMMCO5)
      seq->prevFrameNumOffset = 0;

    /* FrameNumOffset is derived as follows. */
    if (slice->isIDR)
      seq->frameNumOffset = 0;
    else if (seq->prevFrameNum > (int32)slice->frame_num)
      seq->frameNumOffset = seq->prevFrameNumOffset + slice->maxFrameNum;
    else
      seq->frameNumOffset = seq->prevFrameNumOffset;

    /* tempPicOrderCnt is derived as follows */
    if (slice->isIDR)
      tempPicOrderCnt = 0;
    else if (slice->nalRefIdc == 0)
      tempPicOrderCnt = 2 * (seq->frameNumOffset + slice->frame_num) - 1;
    else
      tempPicOrderCnt = 2 * (seq->frameNumOffset + slice->frame_num);

    /* poc */
    poc = tempPicOrderCnt;
  }

  return poc;
}


/*
 *
 * getOutputPic:
 *
 * Parameters:
 *      seq                   Sequence object
 *
 * Function:
 *      Get one output picture. Pictures are output from output queue and
 *      if queue is empty pictures are ouput from dpb. Ouput from dpb can only
 *      happen if sequence is finished (i.e there are not more bits to decode).
 *
 * Returns:
 *      1: output picture is available
 *      0: output picture is not available
 */
static int getOutputPic(sequence_s *seq)
{
  frmBuf_s *srcBuf;
  seq_parameter_set_s *sps;

  /* If no slices have been decoded, there are no pictures available */
  if (seq->isFirstSliceOfSeq)
    return 0;

  /* Check if there are pictures in output queue */
  if (seq->numQueuedOutputPics == 0) 
  {

    /* Get active sequence parameter set */
    sps = seq->spsList[seq->ppsList[seq->currSlice->pic_parameter_set_id]->seq_parameter_set_id];

    /*
     * There are no queued pictures, but we can still output a picture if
     * at least one of the following conditions is true:
     * - we have decoded all NAL units
     * - num_reorder_frames in VUI parameters is zero
     * - POC type is 2
     */
    if (seq->isSeqFinished ||
        sps->vui_parameters.num_reorder_frames == 0 ||
        sps->pic_order_cnt_type == 2)
    {
      int dummy;

      /* Check if there are pictures in dpb */
      srcBuf = dpbGetNextOutputPic(seq->dpb, &dummy);
      if (!srcBuf)
        return 0;   /* There were no pictures to output */
    }
    else
      return 0;   /* None of the conditions were true */
  }
  else 
  {
    /* Take next picture from queue. */
    srcBuf = seq->outputQueue[seq->outputQueuePos];

    seq->numQueuedOutputPics--;
    if (seq->numQueuedOutputPics == 0)
      seq->outputQueuePos = 0;
    else
      seq->outputQueuePos++;
  }

  srcBuf->forOutput          = 0;

  return 1;
}


/*
 *
 * finishCurrentPic:
 *
 * Parameters:
 *      seq                   Sequence object
 *
 * Function:
 *      Finish decoding of current picture. Call loopfilter for the picture
 *      and try to store picture in dpb. Function also updates variables
 *      for previous decoded frame and previous decoded reference frame.
 *
 * Returns:
 *       0 : no frames were output
 *      >0 : the number of frames output
 *      <0 : error
 */
static int finishCurrentPic(sequence_s *seq)
{
  slice_s *slice;
  frmBuf_s *currPic;
  int numOutput;
  int retVal;

  slice   = seq->currSlice;
  currPic = seq->recoBuf;

  if ((retVal = decRefPicMarking(seq)) < 0)
    return retVal;

  /* After the decoding of the current picture and the processing of the     */
  /* memory management control operations a picture including                */
  /* a memory_management_control_operation equal to 5 shall be inferred      */
  /* to have had frame_num equal to 0 for all subsequent use in the decoding */
  /* process.                                                                */
  if (slice->picHasMMCO5)
    currPic->frameNum = slice->frame_num = 0;

  /* Try to store current picture to dpb */
  numOutput = dpbStorePicture(seq->dpb, currPic, seq->outputQueue);
  /* If numOutput != 0, picture was not stored */

  if (numOutput != 0) 
  {

    /* numOutput != 0 implies that pictures were output from dpb */
    seq->outputQueuePos      = 0;
    seq->numQueuedOutputPics = numOutput;

    /* Picture was not stored so we have to store it later */
    seq->isDpbStorePending   = 1;
  }
  else
    seq->isDpbStorePending = 0;

  seq->prevFrameNum       = slice->frame_num;
  seq->prevFrameNumOffset = seq->frameNumOffset;

  seq->prevPicHasMMCO5 = slice->picHasMMCO5;

  /* prevRefFrameNum, prevPocLsb and prevPocMsb for latest reference picture */
  if (slice->nalRefIdc != 0) 
  {
    seq->prevRefFrameNum = slice->frame_num;
    seq->prevPocLsb      = slice->pic_order_cnt_lsb;
    seq->prevPocMsb      = seq->pocMsb;
  }

  seq->isCurrPicFinished = 1;

  return numOutput;
}


/*
 *
 * generateNonExistingFrames:
 *
 * Parameters:
 *      seq                   Sequence object
 *
 * Function:
 *      Generate non-existing frame for each unused frame number between
 *      two closest existing frames in decoding order. Generated frames
 *      are stored to dpb in finishCurrentPic function.
 *
 * Returns:
 *       0 : no frames were output
 *      >0 : the number of frames output
 *      <0 : error
 */
static int generateNonExistingFrames(sequence_s *seq)
{
  slice_s *slice;
  frmBuf_s *currPic;
  int32 nextFrameNum;
  int numOutput;

  slice   = seq->currSlice;
  currPic = seq->recoBuf;

  slice->picHasMMCO5                        = 0;
  slice->isIDR                              = 0;
  slice->adaptive_ref_pic_marking_mode_flag = 0;
  slice->nalType                            = NAL_TYPE_CODED_SLICE;
  slice->nalRefIdc                          = 1;

  currPic->forOutput   = 0;
  currPic->nonExisting = 1;

  do {
    slice->frame_num = seq->unusedShortTermFrameNum;

    dpbUpdatePicNums(seq->dpb, slice->frame_num, slice->maxFrameNum);

    numOutput = finishCurrentPic(seq);

    nextFrameNum = (seq->unusedShortTermFrameNum + 1) % seq->nextSlice->maxFrameNum;

    if (nextFrameNum == (int)seq->nextSlice->frame_num)
      seq->unusedShortTermFrameNum = -1;
    else
      seq->unusedShortTermFrameNum = nextFrameNum;

  } while (numOutput == 0 && seq->unusedShortTermFrameNum >= 0);

  return numOutput;
}


/*
 *
 * initializeCurrentPicture:
 *
 * Parameters:
 *      seq                   Sequence object
 *      sps                   Active sequence parameter set
 *      pps                   Active picture parameter set
 *      width                 Picture width
 *      height                Picture height
 *
 * Function:
 *      Current frame and dpb are initialized according to active
 *      parameter sets.
 *
 * Returns:
 *      SEQ_OK for no error, negative value for error
 */
static int initializeCurrentPicture(sequence_s *seq, seq_parameter_set_s *sps,
                                    pic_parameter_set_s *pps,
                                    int width, int height)

{
  frmBuf_s *currPic;
  slice_s *slice;
  int i;

#ifdef CHECK_MV_RANGE
  if (sps->level_idc <= 10)
    maxVerticalMvRange = 64;
  else if (sps->level_idc <= 20)
    maxVerticalMvRange = 128;
  else if (sps->level_idc <= 30)
    maxVerticalMvRange = 256;
  else
    maxVerticalMvRange = 512;
#endif

  currPic = seq->recoBuf;
  slice   = seq->currSlice;

  /*
    * (Re)initialize frame buffer for current picture if picture size has changed
    */

  if (!currPic || width != currPic->width || height != currPic->height) {
    frmClose(currPic, seq->mbData);
    if ((currPic = frmOpen(&seq->mbData, width, height)) == NULL)
      return SEQ_ERR_MEM;
    seq->recoBuf = currPic;
  }

  for (i = 0; i < MAX_SLICE_GROUP_NUM; i++)
    seq->sliceNums[i] = 0;

  /* Build slice group map */
  buildSliceGroups(seq, slice, sps, pps);

  /* Parameter from SPS */
  currPic->constraintSet0flag = sps->constraint_set0_flag;
  currPic->constraintSet1flag = sps->constraint_set1_flag;
  currPic->constraintSet2flag = sps->constraint_set2_flag;
  currPic->profile            = sps->profile_idc;
  currPic->level              = sps->level_idc;
  currPic->maxFrameNum        = slice->maxFrameNum;

  /* Parameter from PPS */
  currPic->qp = pps->pic_init_qp_minus26 + 26;

  /* By default picture will be output */
  currPic->forOutput   = 1;
  currPic->nonExisting = 0;
  currPic->picType     = slice->slice_type;
  currPic->isIDR       = slice->isIDR;

  psGetAspectRatio(sps, &currPic->aspectRatioNum, &currPic->aspectRatioDenom);
  currPic->overscanInfo        = sps->vui_parameters.overscan_appropriate_flag;
  currPic->videoFormat         = sps->vui_parameters.video_format;
  currPic->videoFullRangeFlag  = sps->vui_parameters.video_full_range_flag;
  currPic->matrixCoefficients  = sps->vui_parameters.matrix_coefficients;
  currPic->chromaSampleLocType = sps->vui_parameters.chroma_sample_loc_type_top_field;
  currPic->numReorderFrames    = sps->vui_parameters.num_reorder_frames;

  if (sps->frame_cropping_flag) {
    currPic->cropLeftOff   = sps->frame_crop_left_offset;
    currPic->cropRightOff  = sps->frame_crop_right_offset;
    currPic->cropTopOff    = sps->frame_crop_top_offset;
    currPic->cropBottomOff = sps->frame_crop_bottom_offset;
  }
  else {
    currPic->cropLeftOff   = 0;
    currPic->cropRightOff  = 0;
    currPic->cropTopOff    = 0;
    currPic->cropBottomOff = 0;
  }

  if (sps->vui_parameters_present_flag &&
      sps->vui_parameters.timing_info_present_flag &&
      sps->vui_parameters.num_units_in_tick != 0)
    currPic->frameRate = (float)(0.5 * (float)sps->vui_parameters.time_scale/(float)sps->vui_parameters.num_units_in_tick);
  else
    currPic->frameRate = 0.0;

  /* Get poc for current picture */
  currPic->poc = decodePictureOrderCount(seq, slice, sps);

  /* Set chroma qp index offset */
  currPic->chromaQpIndexOffset = pps->chroma_qp_index_offset;

  currPic->pictureStructure = 0;

  currPic->lossy = 0;
  seq->isCurrPicFinished = 0;
  seq->redundantPicCnt = slice->redundant_pic_cnt;

  return SEQ_OK;
}

// parseSliceData
// Reads and parses slice data
static TInt parseSliceData(sequence_s *seq)
{
  	slice_s *slice;
  	pic_parameter_set_s *pps;
  	seq_parameter_set_s *sps;
  	TInt width, height;
  	TInt sliceGroupNum, sliceID;
  	TInt retCode;

  	// New slice becomes current slice 
  	slice = seq->nextSlice;
  	seq->nextSlice = seq->currSlice;
  	seq->currSlice = slice;

  	// Get current parameter sets 
  	pps = seq->ppsList[slice->pic_parameter_set_id];
  	sps = seq->spsList[pps->seq_parameter_set_id];

  	// Get picture size 
  	width  = (sps->pic_width_in_mbs_minus1+1)*16;
  	height = (sps->pic_height_in_map_units_minus1+1)*16;

  	// If this is the first slice of a picture, initialize picture 
  	if (seq->isFirstSliceOfSeq || seq->isPicBoundary) 
  	{

    	if (slice->isIDR || seq->isFirstSliceOfSeq) 
    	{

      		// Set dpb according to level
      		dpbSetSize(seq->dpb, sps->vui_parameters.max_dec_frame_buffering);

      		seq->dpb->maxNumRefFrames = sps->num_ref_frames;
    	}

    	retCode = initializeCurrentPicture(seq, sps, pps, width, height);

    	if (retCode < 0)
      		return retCode;
  	}
  	else 
  	{
    	if (IS_SLICE_P(slice->slice_type))
      	// If there is a P-slice in the picture, mark picture as P-picture 
      		seq->recoBuf->picType = slice->slice_type;
  	}

  	// Compute picture numbers for all reference frames 
  	if (!slice->isIDR)
    	dpbUpdatePicNums(seq->dpb, slice->frame_num, slice->maxFrameNum);

  	// Get slice group number if there are more than 1 slice groups 
  	if (pps->num_slice_groups_minus1 == 0)
    	sliceGroupNum = 0;
  	else
    	sliceGroupNum = seq->mbData->sliceMap[slice->first_mb_in_slice] & 0xF;

  	// Increment slice number for current slice group (slice numbers start from 1) 
  	seq->sliceNums[sliceGroupNum]++;

  	// sliceID for current slice 
  	sliceID = seq->sliceNums[sliceGroupNum]*16 | sliceGroupNum;


  	// Parse the macroblocks in the slice
  	retCode = sliceParseMacroblocks(slice, seq->recoBuf, seq->dpb,
    	                            pps, seq->mbData, sliceID, seq->bitbuf, 
                                    seq->iBitShiftInSlice);

  	// Update sequence variables 
  	seq->isFirstSliceOfSeq        = 0;
  	seq->isPicBoundary            = 0;

  	if (retCode < 0)
    	return SEQ_ERROR;
  	else
    	return SEQ_OK;
}


// parseSlice
// Parses the slice header and calls parseSliceData to parse the slice data if necessary
static TInt parseSlice(sequence_s *seq, int nalType, int nalRefIdc)
{
  	slice_s *slice;
  	TInt nextFrameNum;
  	TInt numOutput;
  	TInt retCode;
  

  	slice = seq->nextSlice;

  	slice->nalType   = nalType;
  	slice->nalRefIdc = nalRefIdc;
  	slice->isIDR     = (nalType == NAL_TYPE_CODED_SLICE_IDR);
  
  	slice->sliceDataModified = 0;
  
  	// Reset the bit shift flag
  	seq->iBitShiftInSlice = EFalse;

	// Parse the slice header
  	retCode = ParseSliceHeader(slice, seq->spsList, seq->ppsList, seq->bitbuf, &seq->iFrameNumber, seq->iFromEncoder);
  
  	if (slice->sliceDataModified)
  		seq->sliceDataModified = 1;
  
  	if ( retCode != SLICE_STOP_PARSING )
  		seq->iBitShiftInSlice = ETrue;

  	if (retCode < 0)
    	return SEQ_ERROR;

  	// Check if next slice belongs to next picture 
  	if (seq->isFirstSliceOfSeq)
    	seq->isPicBoundary = 0;
  	else
    	seq->isPicBoundary = isPicBoundary(seq);

  	if (!seq->isPicBoundary) 
  	{	
    	// There is no picture boundary. Decode new slice if redundant 
    	// picture count is same as in previous slice                  
    	if (seq->isFirstSliceOfSeq || slice->redundant_pic_cnt == seq->redundantPicCnt)
      		return parseSliceData(seq);
    	else
      		return SEQ_OK;
  	}
  	else 
  	{
    	// Picture boundary reached or all MBs of current picture were decoded. 
    	if (!seq->isCurrPicFinished) 
    	{
      		// Finish decoding of current picture 
      		numOutput = finishCurrentPic(seq);

      		// numOutput is the number of pictures in output queue 
      		// If numOutput < 0, error occured 
      		if (numOutput < 0)
        		return numOutput;
    	}
    	else
      		numOutput = 0;

    	// Compute expected next frame number 
    	nextFrameNum = (seq->prevRefFrameNum + 1) % slice->maxFrameNum;

    	// Check if there is a gap in frame numbers 
    	if (!slice->isIDR && (TInt)slice->frame_num != seq->prevRefFrameNum &&
        	(TInt)slice->frame_num != nextFrameNum)
    	{
       		// Start filling in gaps in frame numbers 
       		seq->unusedShortTermFrameNum = nextFrameNum;

	        // If dpb was not full (i.e. we did not have to output any pictures), 
       		// we can generate non-existing frames.                               
       		if (numOutput == 0) 
       			numOutput = generateNonExistingFrames(seq);
    	}

    	if (numOutput == 0)
    	{
      		// If there are no pictures in output queue we can decode next slice 
      		return parseSliceData(seq);
    	}
    	else 
    	{
      		// Don't decode slice since it belongs to next picture 
      		return SEQ_OK;
    	}
  	}
}


// avcdParseParameterSet
// Parses SPS / PPS parameter sets from the input NAL unit
TInt avcdParseParameterSet(avcdDecoder_t *dec, void *nalUnitBits, TUint* nalUnitLen)
{
    sequence_s *seq = (sequence_s *)dec;
    TInt nalHeaderByte;
    TInt nalType;
//    TInt nalRefIdc;
    TInt retCode;
    TUint nalUnitLength = *nalUnitLen;
    
		
	PRINT((_L("Sequence::avcdParseParameterSet() in, frame # %d, total # %d"), seq->iFrameNumber, seq->iTotalFrameNumber));

  	// Check for end of stream 
  	if (nalUnitBits == 0 || nalUnitLen == 0) 
  	{
  		return AVCD_OK;
  	}

  	// Allocate memory for the bitbuffer data, add 10 to nal length in case SPS/PPS sets are modified
  	seq->bitbuf->data = (TUint8*) User::Alloc(nalUnitLength+10);
  	
  	if (seq->bitbuf->data == 0)
  	    return KErrNoMemory;
  	
	Mem::FillZ(seq->bitbuf->data, (nalUnitLength+10)*sizeof(TUint8) );
	
  	TUint8* tpD = (TUint8*)nalUnitBits;
  	 
	Mem::Copy(seq->bitbuf->data, tpD, nalUnitLength*sizeof(TUint8));

  	// Initialize bitbuffer and get first byte containing NAL type and NAL ref idc 
  	if (bibInit(seq->bitbuf, seq->bitbuf->data, nalUnitLength) < 0)
  	{
		User::Free(seq->bitbuf->data);
    
    	return AVCD_ERROR;
  	}

  	if (bibGetByte(seq->bitbuf, &nalHeaderByte))
  	{
		User::Free(seq->bitbuf->data);
    
    	return AVCD_ERROR;
  	}
  
  	// Decode NAL unit type and reference indicator 
  	nalType   = nalHeaderByte & 0x1F;
//  	nalRefIdc = (nalHeaderByte & 0x60) >> 5;

  	// Decode NAL unit data 
  	switch (nalType)
  	{
  		case NAL_TYPE_SPS:              // 7
			retCode = psParseSPS(seq->bitbuf, seq->spsList, seq->iFromEncoder, &seq->iEncodeUntilIDR, &seq->iNumSPS);
    		if ( retCode == KErrNotSupported)
    		{
				User::Free(seq->bitbuf->data);
      		
      			return KErrNotSupported;
    		}
    		else if (retCode < 0)
		  	{
				User::Free(seq->bitbuf->data);
    		
    			return AVCD_ERROR;
  			}
    		break;
  		case NAL_TYPE_PPS:              // 8
			retCode = psParsePPS(seq->bitbuf, seq->ppsList, seq->spsList, seq->iFromEncoder, &seq->iNumPPS);
    		if (retCode == KErrNotSupported)
    		{
				User::Free(seq->bitbuf->data);
      		
      			return KErrNotSupported;
    		}
    		else if (retCode < 0)
	  		{
				User::Free(seq->bitbuf->data);
    		
    			return AVCD_ERROR;
  			}
    		break;
  		default:
   			PRINT((_L("Not a parameter set NAL type: (%i)\n"), nalType));    
    		break;
  		}

	// Take care of emulation prevention bytes
	int error = bibEnd(seq->bitbuf);	

	// Free the bitbuffer data
	User::Free(seq->bitbuf->data);

	if (error != 0)
        return error;

	return AVCD_OK;  
}


// avcdParseOneNal
// Parses one input NAL unit
TInt avcdParseOneNal(avcdDecoder_t *dec, void *nalUnitBits, TUint* nalUnitLen)
{
    sequence_s *seq = (sequence_s *)dec;
    TInt nalHeaderByte;
    TInt nalType;
    TInt nalRefIdc;
    TInt retCode;
    TUint nalUnitLength = *nalUnitLen;
		
	PRINT((_L("Sequence::avcdParseOneNal() in, frame # %d, total # %d"), seq->iFrameNumber, seq->iTotalFrameNumber));
	
  /*
   * The following conditions are tested to see what is the current decoder state
   * and to act upon that state:
   *
   * - Check if picture can be output from output queue without further decoding.
   * - Check if dpb store is pending (i.e current picture was not be
   *   stored to dpb during previous call because dpb was full).
   * - Check any non-existing frames should be generated (i.e there were gaps in
   *   frame number). If non-existing frame(s) were generated, check output
   *   queue again.
   * - Check for end of stream. If end of stream was reached then current picture
   *   is finished if not yet finished. Check again whether picture can be output
   *   from either output queue or dpb (check is internal to getOutputPic(...)).
   * - Check if slice decode is pending (i.e only header of the latest slice was
   *   decoded during previous call and we now need to decode slice data) and if
   *   so, decode slice data.
   * - Check any lost frames being recovered (i.e there were ref frames lost)
   *   If lost frames were rescued, check output queue again.
   */

  	// We can return immediately if there are queued output pics 
  	if (seq->numQueuedOutputPics > 0) 
  	{
  		// "Flush" all output pictures 
  		while (seq->numQueuedOutputPics > 0)
  		{
	    	getOutputPic(seq);
  		}
  	}

  	// Is current picture waiting to be moved to DPB? 
  	if (seq->isDpbStorePending) 
  	{
    	if (dpbStorePicture(seq->dpb, seq->recoBuf, seq->outputQueue) != 0) 
    	{
      		PRINT((_L("Error: dpb store failed\n")));
      		return AVCD_ERROR;
    	}
    	
    	seq->isDpbStorePending = 0;
  	}

  	// Check for end of stream 
  	if (nalUnitBits == 0 || nalUnitLen == 0) 
  	{
    	if (!seq->isSeqFinished && !seq->isCurrPicFinished && seq->recoBuf != NULL) 
    	{
      		if (finishCurrentPic(seq) < 0)
        		return AVCD_ERROR;
    	}
    	
    	seq->isSeqFinished = 1;
      	
		getOutputPic(seq);
      	return AVCD_OK;
  	}

  	// Reset the sliceDataModified flag
  	seq->sliceDataModified = 0;

	// Initialize bitbuffer and get first byte containing NAL type and NAL ref idc 
  	if (bibInit(seq->bitbuf, (TUint8 *)nalUnitBits, nalUnitLength) < 0)
    	return AVCD_ERROR;

  	if (bibGetByte(seq->bitbuf, &nalHeaderByte))
    	return AVCD_ERROR;

  	// Decode NAL unit type and reference indicator 
  	nalType   = nalHeaderByte & 0x1F;
  	nalRefIdc = (nalHeaderByte & 0x60) >> 5;

  	// Decode NAL unit data 
  	switch (nalType)
  	{
  		case NAL_TYPE_CODED_SLICE:      // 1
    		parseSlice(seq, nalType, nalRefIdc);
			seq->iTotalFrameNumber++;
    		break;
  		case NAL_TYPE_CODED_SLICE_P_A:  // 2
  		case NAL_TYPE_CODED_SLICE_P_B:  // 3
  		case NAL_TYPE_CODED_SLICE_P_C:  // 4
    		PRINT((_L("Slice data partition NAL type (%i) not supported.\n"), nalType));    
    		break;
  		case NAL_TYPE_CODED_SLICE_IDR:  // 5
    		parseSlice(seq, nalType, nalRefIdc);
			seq->iTotalFrameNumber++;
    		break;
  		case NAL_TYPE_SEI:              // 6
    		PRINT((_L("SEI NAL unit (6) skipped.\n")));
    		break;
  		case NAL_TYPE_SPS:              // 7
			retCode = psParseSPS(seq->bitbuf, seq->spsList, seq->iFromEncoder, &seq->iEncodeUntilIDR, &seq->iNumSPS);
    		if ( retCode == KErrNotSupported)
      			return KErrNotSupported;
    		else if (retCode < 0)
    			return AVCD_ERROR;
    		break;
  		case NAL_TYPE_PPS:              // 8
			retCode = psParsePPS(seq->bitbuf, seq->ppsList, seq->spsList, seq->iFromEncoder, &seq->iNumPPS);
    		if (retCode == KErrNotSupported)
      			return KErrNotSupported;
    		else if (retCode < 0)
      			return AVCD_ERROR;
    		break;
  		case NAL_TYPE_PIC_DELIMITER:    // 9
    		PRINT((_L("Picture Delimiter NAL unit (9) skipped.\n")));
    		break;
  		case NAL_TYPE_END_SEQ:          // 10
    		PRINT((_L("End of Sequence NAL unit (10) skipped.\n")));
    		break;
  		case NAL_TYPE_END_STREAM:       // 11
    		PRINT((_L("End of Stream NAL unit (11) skipped.\n")));
    		break;
  		case NAL_TYPE_FILLER_DATA:      // 12
    		PRINT((_L("Filler Data NAL unit (12) skipped.\n")));
    		break;
  		default:
    		// Unspecied NAL types 0 and 24-31 
    		if (nalType == 0 || (nalType >= 24 && nalType <= 31))
    		{
      			PRINT((_L("Unspecified NAL type: (%i)\n"), nalType));    
    		}
    		// Reserved NAL types 13-23 
    		else
    		{
      			PRINT((_L("Reserved NAL type (%i)\n"), nalType));    
    		}
    		break;
  	}


  	// Check the output queue once again
  	if (getOutputPic(seq)) 
  	{
  		// If current slice has not been parsed yet, parse it not
    	if (seq->isPicBoundary)
    	{
			parseSliceData(seq);
    		getOutputPic(seq);
    	}
  	}

	// Take care of emulation prevention bytes
	bibEndSlice(seq->bitbuf);

	// If slice data was modified, copy the modified data back to the nal data buffer
	if (seq->sliceDataModified)
	{
		// If buffer length has been modified, change nalUnitLen
		if (seq->bitbuf->dataLen != nalUnitLength)
		{	
			(*nalUnitLen) = seq->bitbuf->dataLen;
		}
	}

	return AVCD_OK;  
}


// FrameIsFromEncoder
// Stores information about the origin of next frame (if it is generated by the encoder)
void FrameIsFromEncoder(avcdDecoder_t *dec, TUint aFromEncoder)
{
    sequence_s *seq = (sequence_s *)dec;
	
	seq->iFromEncoder = aFromEncoder;
}


// ReturnPPSSet
// This function returns the aIndex'th PPS set stored
TUint8* ReturnPPSSet(avcdDecoder_t *dec, TUint aIndex, TUint* aPPSLength)
{
	TUint i=0;
	TUint j;
    sequence_s *seq = (sequence_s *)dec;
	
	for (j=0; j<PS_MAX_NUM_OF_PPS; j++)
	{
		if (seq->ppsList[j])
		{
			if ( i == aIndex )
			{
				*aPPSLength = seq->ppsList[j]->PPSlength;			
				return seq->ppsList[j]->codedPPSBuffer;
			}
			
			i++;
		}
	}
	
	// No PPS set found with that index
	return NULL;
}


// ReturnNumPPS
// Returns the number of PPS units stored 
TUint ReturnNumPPS(avcdDecoder_t *dec)
{
	TUint i=0;
	TUint j;
    sequence_s *seq = (sequence_s *)dec;
	
	for (j=0; j<PS_MAX_NUM_OF_PPS; j++)
	{
		if (seq->ppsList[j])
		{
			i++;
		}
	}
	
	return i;
}


// ReturnSPSSet
// This function returns the aIndex'th PPS set stored
TUint8* ReturnSPSSet(avcdDecoder_t *dec, TUint aIndex, TUint* aSPSLength)
{
	TUint i=0;
	TUint j;
    sequence_s *seq = (sequence_s *)dec;
	
	for (j=0; j<PS_MAX_NUM_OF_SPS; j++)
	{
		if (seq->spsList[j])
		{
			if ( i == aIndex )
			{
				*aSPSLength = seq->spsList[j]->SPSlength;			
				return seq->spsList[j]->codedSPSBuffer;
			}
			
			i++;
		}
		
	}
	
	// No SPS set found with that index
	return NULL;
}


// ReturnNumSPS
// Returns the number of SPS units stored 
TUint ReturnNumSPS(avcdDecoder_t *dec)
{
	TUint i=0;
	TUint j;
    sequence_s *seq = (sequence_s *)dec;
	
	for (j=0; j<PS_MAX_NUM_OF_SPS; j++)
	{
		if (seq->spsList[j])
		{
			i++;
		}
	}
	
	return i;
}


// ReturnEncodeUntilIDR
// Returns information whether frames should be encoded until the next IDR
TBool ReturnEncodeUntilIDR(avcdDecoder_t *dec)
{
    sequence_s *seq = (sequence_s *)dec;

	return (seq->iEncodeUntilIDR);
}


// EncodeZeroValueWithVariableLength
// Encodes zero (which is encoded with a single one bit) to the bitbuffer
void EncodeZeroValueWithVariableLength(bitbuffer_s *aBitBuffer)
{
	// If bit position is zero, move to next byte
	if(aBitBuffer->bitpos == 0)
	{
		// Get the next byte
		aBitBuffer->currentBits = aBitBuffer->data[aBitBuffer->bytePos];
		aBitBuffer->bytePos++;
		aBitBuffer->bitpos = 8;
	}
		
	// Change the bitpos bit's value to one
	aBitBuffer->data[aBitBuffer->bytePos-1] |= 1 << (aBitBuffer->bitpos-1);
	aBitBuffer->bitpos--;
		
	if(aBitBuffer->bitpos == 0)
	{
		// Get the next byte
		aBitBuffer->currentBits = aBitBuffer->data[aBitBuffer->bytePos];
		aBitBuffer->bytePos++;
		aBitBuffer->bitpos = 8;
	}
	
	// Make sure the bit buffer currentBits is up-to-date
	aBitBuffer->currentBits = aBitBuffer->data[aBitBuffer->bytePos-1];
}


// GenerateEmptyFrame
// Generates an empty frame (a not coded frame) by generating the slice header and slice body
// Slice body contains only a value to skip all the macroblocks in the slice.
void GenerateEmptyFrame(sequence_s *seq, bitbuffer_s *bitbuf, TUint aFrameNumber)
{
  	seq_parameter_set_s *sps;
  	pic_parameter_set_s *pps;
	TUint8 bitMask;
	TUint picSizeInMapUnits;
	TUint ppsId;
  	
  	
  	pps = seq->ppsList[seq->iPreviousPPSId];

  	if (pps->indexChanged)
  	{
	  	// Since we generate empty frames for original clips only, 
	  	// find out what is the original PPS id
  		ppsId = pps->origPPSId;
  		
  		pps = seq->ppsList[ppsId];
  	}
  		
  	sps = seq->spsList[pps->seq_parameter_set_id];
  	
  	// Compute the number of macroblocks
	picSizeInMapUnits = (sps->pic_width_in_mbs_minus1+1) * (sps->pic_height_in_map_units_minus1+1);

	// Generate the not coded frame
	// Set the first byte as zero
	bitbuf->bytePos = 0;
	bitbuf->data[bitbuf->bytePos] = 0;
	bitbuf->bitpos = 8;
	
	// Generate the NAL header, with nal type as 1 and nal_ref_idc as 1
	bitbuf->data[0] = 0x21;
	bitbuf->bytePos = 1;

	// Set the first_mb_in_slice & slice_type to be zero (coded as 1), advance bitpos by two
	bitbuf->data[bitbuf->bytePos] = 0xc0;	// First two bits == 1
	bitbuf->bitpos -= 2;
	bitbuf->bytePos = 2;
	
	// Encode the PPS index
	EncodeUnsignedExpGolombCode(bitbuf, 0);
	
 	// Encode the new frame number here
 	// If the max frame num was changed use original value since empty clips are generated for original clips
  	if (sps->maxFrameNumChanged)
	  	EncodeUnsignedNBits(bitbuf, aFrameNumber, sps->origMaxFrameNum+4);
  	else
	  	EncodeUnsignedNBits(bitbuf, aFrameNumber, sps->log2_max_frame_num_minus4+4);
  
  	// POC parameters 
	if (sps->pic_order_cnt_type == 0) 
  	{
	  	// For now encode the POC as the frame number
 		// If the max frame num was changed use original value since empty clips are generated 
 		// for original clips
	  	if (sps->maxPOCNumChanged)
	  		EncodeUnsignedNBits(bitbuf, aFrameNumber, sps->origMaxPOCNum+4);
	  	else
	  		EncodeUnsignedNBits(bitbuf, aFrameNumber, sps->log2_max_pic_order_cnt_lsb_minus4+4);
  	}
  	else if (sps->pic_order_cnt_type == 1) 
  	{
    	if (!sps->delta_pic_order_always_zero_flag) 
    	{
      		EncodeZeroValueWithVariableLength(bitbuf);
      		if (pps->pic_order_present_flag) 
      		{
	      		EncodeZeroValueWithVariableLength(bitbuf);
      		}
    	}
  	}
  	
  	// Redundant picture count
  	if (pps->redundant_pic_cnt_present_flag) 
  	{
  		EncodeZeroValueWithVariableLength(bitbuf);
  	}
  	
  	// Encode num_ref_idx_active:override_flag with zero value
	EncodeUnsignedNBits(bitbuf, 0, 1);
	
  	// Encode reference picture list reordering with single zero value
	EncodeUnsignedNBits(bitbuf, 0, 1);
	
	// Since nal_ref_idc == 1, encode zero value for decoded reference picture marking
	EncodeUnsignedNBits(bitbuf, 0, 1);
  	
  	// Encode slice_qp_delta with zero value
	EncodeZeroValueWithVariableLength(bitbuf);
	
  	if (pps->deblocking_filter_parameters_present_flag == 1) 
  	{
  		// Encode value of 1 which is 010, i.e. two with three bits
  		EncodeUnsignedNBits(bitbuf, 2, 3);
  	}

  	if (pps->num_slice_groups_minus1 > 0 && pps->slice_group_map_type >= 3 &&
        pps->slice_group_map_type <= 5)
  	{
  		TUint temp, temp2, len1;
  		
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
    	
		// Encode zero value with len1 bits
		EncodeUnsignedNBits(bitbuf, 0, len1);
  	}
  	
  	// Now encode the slice data
  	// Encode the mb_skip_run to indicate all macroblocks to be skipped
  	// For example in CIF we have 22x18 = 396 macroblocks
  	// 396 = 00000000 1 10001101
	EncodeUnsignedExpGolombCode(bitbuf, picSizeInMapUnits);
  	
  	// Take care of the trailing bits, i.e. encode the rest of the bits in this byte to zero
  	bitMask = 1 << (bitbuf->bitpos - 1);
	bitbuf->data[bitbuf->bytePos-1] = bitbuf->data[bitbuf->bytePos-1] | bitMask;
	
	bitbuf->bitpos--;
	if(bitbuf->bitpos != 0)
	{
		bitMask = 255 << (bitbuf->bitpos);	// Mask the 8-bitPos upper bits
		bitbuf->data[bitbuf->bytePos-1] = bitbuf->data[bitbuf->bytePos-1] & bitMask;
	}
  	
  	bitbuf->dataLen = bitbuf->bytePos;
}


// avcdGenerateNotCodedFrame
// Generates an empty frame
TInt avcdGenerateNotCodedFrame(avcdDecoder_t *dec, void *aNalUnitBits, TUint aNalUnitLen, TUint aFrameNumber)
{
    sequence_s *seq = (sequence_s *)dec;
	
	// Initialize bitbuffer
  	if (bibInit(seq->bitbuf, (TUint8 *)aNalUnitBits, aNalUnitLen) < 0)
    	return AVCD_ERROR;
  	
  	GenerateEmptyFrame(seq, seq->bitbuf, aFrameNumber);

  	return (seq->bitbuf->dataLen);
}


// avcdStoreCurrentPPSId
// Stores the value of PPS Id from the input NAL unit (if that unit is a coded slice). 
// The Id value is used in the generation of an empty frame. 
TInt avcdStoreCurrentPPSId(avcdDecoder_t *dec, TUint8 *nalUnitBits, TUint aNalUnitLen)
{
    sequence_s *seq = (sequence_s *)dec;
    TInt nalHeaderByte;
    TInt nalType;
//    TInt nalRefIdc;
//    TUint temp;
		
	// Initialize bitbuffer and get first byte containing NAL type and NAL ref idc 
  	if (bibInit(seq->bitbuf, nalUnitBits, aNalUnitLen) < 0)
    	return AVCD_ERROR;

  	if (bibGetByte(seq->bitbuf, &nalHeaderByte))
    	return AVCD_ERROR;

  	// Decode NAL unit type and reference indicator 
  	nalType   = nalHeaderByte & 0x1F;
//  	nalRefIdc = (nalHeaderByte & 0x60) >> 5;

  	// Decode NAL unit data 
  	if (nalType == NAL_TYPE_CODED_SLICE || nalType == NAL_TYPE_CODED_SLICE_IDR)
  	{
  		// Parse the slice haeder until the PPS id
  		// First macroblock in slice 
//  		temp = vldGetUVLC(seq->bitbuf);
	  		
	  	// Slice type 
//  		temp = vldGetUVLC(seq->bitbuf);

		// PPS id 
  		seq->iPreviousPPSId = vldGetUVLC(seq->bitbuf);
  	}

	return AVCD_OK;  
}


// ue_v
// Returns unsigned UVLC code from the bitbuffer
static int ue_v(bitbuffer_s *bitbuf, unsigned int *val, unsigned int maxVal)
{
  *val = vldGetUVLC(bitbuf);

  if (bibGetStatus(bitbuf) < 0)
    return SLICE_ERROR;

  if (*val > maxVal)
    return SLICE_ERR_ILLEGAL_VALUE;

  return SLICE_OK;
}


// ModifyFrameNumber
// Modifies the frame numbering from the input bit buffer. The bit buffer must be positioned 
// at the start of the slice header when this function is called.
void ModifyFrameNumber(sequence_s *seq, bitbuffer_s *bitbuf, TUint aFrameNumber, TInt aNalType)
{
  	seq_parameter_set_s *sps;
  	pic_parameter_set_s *pps;
	TUint tempValue;
	TUint firstMbInSlice;
	TUint ppsId;
//	TInt spsId;
  	
  	
  	// First macroblock in slice 
  	ue_v(bitbuf, &firstMbInSlice, 65535);

  	// Slice type 
  	ue_v(bitbuf, &tempValue, SLICE_MAX);

  	// PPS id 
  	ue_v(bitbuf, &ppsId, PS_MAX_NUM_OF_PPS-1);
  
  	pps = seq->ppsList[ppsId];

  	if (pps->indexChanged)
  	{
	  	// Since we generate empty frames for original clips only, 
	  	// find out what is the original PPS id
  		ppsId = pps->origPPSId;
  		
  		pps = seq->ppsList[ppsId];
  	}
  		
/*
  	if (pps == NULL) 
  	{
		// Use zero for SPS id
		spsId = 0;
  	}
  	else
  	{
	  	spsId = pps->seq_parameter_set_id;
  	}
*/

	syncBitBufferBitpos(bitbuf);
	
  	sps = seq->spsList[pps->seq_parameter_set_id];
  	
  	if (sps == NULL) 
  	{
    	PRINT((_L("Error: referring to non-existing SPS.\n")));     
		return;
  	}

  	if (sps->maxFrameNumChanged)
  	{
	  	// Encode the new frame number here
  		EncodeUnsignedNBits(bitbuf, aFrameNumber, sps->origMaxFrameNum+4);
  	}
  	else
  	{
	  	// Encode the new frame number here
  		EncodeUnsignedNBits(bitbuf, aFrameNumber, sps->log2_max_frame_num_minus4+4);
  	}
  	
  	// IDR picture 
  	if (aNalType == NAL_TYPE_CODED_SLICE_IDR) 
  	{
    	ue_v(bitbuf, &tempValue, 65535);
  	}

	syncBitBufferBitpos(bitbuf);
	
  	// POC parameters 
	if (sps->pic_order_cnt_type == 0) 
  	{
	  	if (sps->maxPOCNumChanged)
  		{
		  	// For now encode the POC as the frame number
	  		EncodeUnsignedNBits(bitbuf, aFrameNumber, sps->origMaxPOCNum+4);
  		}
  		else
  		{
		  	// For now encode the POC as the frame number
	  		EncodeUnsignedNBits(bitbuf, aFrameNumber, sps->log2_max_pic_order_cnt_lsb_minus4+4);
  		}
  	}
}


// avcdModifyFrameNumber
// Modifies the input NAL unit's frame numbering
void avcdModifyFrameNumber(avcdDecoder_t *dec, void *aNalUnitBits, TUint aNalUnitLen, TUint aFrameNumber)
{
    sequence_s *seq = (sequence_s *)dec;
    TInt nalHeaderByte;
    TInt nalType;
	
	// Initialize bitbuffer
  	bibInit(seq->bitbuf, (TUint8 *)aNalUnitBits, aNalUnitLen);
  	
  	// Read the nalHeaderByte
  	bibGetByte(seq->bitbuf, &nalHeaderByte);
  	
  	nalType   = nalHeaderByte & 0x1F;

  	if (nalType == NAL_TYPE_CODED_SLICE || nalType == NAL_TYPE_CODED_SLICE_IDR)
	  	ModifyFrameNumber(seq, seq->bitbuf, aFrameNumber, nalType);
}


#endif  // VIDEOEDITORENGINE_AVC_EDITING
