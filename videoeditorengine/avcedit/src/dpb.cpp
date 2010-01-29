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
#include "framebuffer.h"
#include "dpb.h"



/*
 *
 * dpbOpen:
 *
 * Parameters:
 *
 * Function:
 *      Allocate DPB.
 *
 * Returns:
 *      Pointer to dpb or NULL
 *
 */
dpb_s *dpbOpen()
{
  dpb_s *dpb;

  dpb = (dpb_s *)User::Alloc(sizeof(dpb_s));

  if (dpb != NULL)
    memset(dpb, 0, sizeof(dpb_s));

  return dpb;
}


/*
 *
 * dpbClose:
 *
 * Parameters:
 *      dpb                   DPB object
 *
 * Function:
 *      Deinitialize DPB.
 *
 * Returns:
 *      -
 *
 */
void dpbClose(dpb_s *dpb)
{
  int i;

  for (i = 0; i < DPB_MAX_SIZE; i++)
    frmCloseRef(dpb->buffers[i]);

  User::Free(dpb);
}


/*
 *
 * dpbSetSize:
 *
 * Parameters:
 *      dpb                   DPB object
 *      dpbSize               New DPB size
 *
 * Function:
 *      Set DPB size in frames.
 *
 * Returns:
 *      -
 *
 */
void dpbSetSize(dpb_s *dpb, int dpbSize)
{
  int i;

  /* If new DPB size is smaller than old, close any unneeded frame buffers */
  for (i = dpbSize; i < dpb->size; i++) {
    frmCloseRef(dpb->buffers[i]);
    dpb->buffers[i] = NULL;
  }

  dpb->size = min(dpbSize, DPB_MAX_SIZE);
}


/*
 *
 * dpbGetNextOutputPic:
 *
 * Parameters:
 *      dpb                   DPB object
 *      dpbHasIDRorMMCO5      Set upon return if IDR picture or picture with
 *                            MMCO5 command is found in DPB
 *
 * Function:
 *      Find next frame to output (frame with the lowest POC).
 *      Search is started from the last active frame in dpb and is
 *      stopped if all frames have been checked or IDR picture or
 *      picture with MMCO5 is found.
 *
 * Returns:
 *      Framebuffer with the lowest POC or 0 if DPB is empty
 *
 */
frmBuf_s *dpbGetNextOutputPic(dpb_s *dpb, int *dpbHasIDRorMMCO5)
{
  frmBuf_s * tmpFrm;
  int i;

  tmpFrm = 0;

  /* Find first output pic in decoding order */
  for (i = dpb->fullness-1; i >= 0; i--) {
    if (dpb->buffers[i]->forOutput) {
      tmpFrm = dpb->buffers[i];
      break;
    }
  }

  *dpbHasIDRorMMCO5 = 0;

  /* Find picture with lowest poc. Stop search if IDR or MMCO5 is found */
  for (; i >= 0; i--) {
    if (dpb->buffers[i]->isIDR || dpb->buffers[i]->hasMMCO5) {
      *dpbHasIDRorMMCO5 = 1;
      break;
    }
    if (dpb->buffers[i]->forOutput && dpb->buffers[i]->poc < tmpFrm->poc)
      tmpFrm = dpb->buffers[i];
  }

  return tmpFrm;
}


/*
 *
 * dpbStorePicture:
 *
 * Parameters:
 *      dpb                   DPB object
 *      currPic               Current picture
 *      outputQueue           Output queue
 *
 * Function:
 *      - Remove unused frames (non-reference, non-output frames) from the DPB.
 *      - If there is room in the DPB, put picture to DPB.
 *      - If there is no room in DPB, put pictures to output queue
 *        until frame is available.
 *
 * Returns:
 *      The number of pictures in output queue or negative value for error.
 *
 */
int dpbStorePicture(dpb_s *dpb, frmBuf_s *currPic, frmBuf_s *outputQueue[])
{
  frmBuf_s *tmpRemList[DPB_MAX_SIZE];
  frmBuf_s *tmpFrm;
  int numFrm, numRemFrm;
  int i;
  int freeBufferFound;
  int numOutput;
  int dpbHasIDRorMMCO5;

  /*
   * If the current picture is a reference picture and DPB is already full of
   * reference pictures, we cannot insert current picture to DPB. Therefore,
   * we force one reference picture out of DPB to make space for current
   * picture. This situation can only happen with corrupted bitstreams.
   */
  if (currPic->refType != FRM_NON_REF_PIC &&
      (dpb->numShortTermPics+dpb->numLongTermPics) == dpb->size)
  {
    if (dpb->numLongTermPics == dpb->size)
      dpb->buffers[dpb->fullness-1]->refType = FRM_NON_REF_PIC;
    else
      dpbMarkLowestShortTermPicAsNonRef(dpb);
  }

  /*
   * Remove unused frames from dpb
   */

  numFrm = 0;
  numRemFrm = 0;
  for (i = 0; i < dpb->fullness; i++) {
    if (dpb->buffers[i]->refType != FRM_NON_REF_PIC || dpb->buffers[i]->forOutput) {
      dpb->buffers[numFrm] = dpb->buffers[i];
      numFrm++;
    }
    else {
      /* Put unsused pics to temporary list */
      tmpRemList[numRemFrm] = dpb->buffers[i];
      numRemFrm++;
    }
  }

  /* Copy unused pics after active pics */
  for (i = 0; i < numRemFrm; i++)
    dpb->buffers[numFrm+i] = tmpRemList[i];

  dpb->fullness = numFrm;


  /*
   * Try to store current pic to dpb. If there is no room in dpb, we have to
   * output some pictures to make room.
   */

  /* Case 1: if current pic is unused, it won't be stored in dpb */
  if (currPic->refType == FRM_NON_REF_PIC && !currPic->forOutput) {
    return 0;
  }

  /* Case 2: if there is space left in dpb, store current pic */
  if (dpb->fullness < dpb->size) {

    tmpFrm = dpb->buffers[dpb->fullness];   /* Unused frame */

    tmpFrm = frmMakeRefFrame(currPic, tmpFrm);
    if (tmpFrm == NULL)
      return DPB_ERR_MEM_ALLOC;

    /* Move frames one position toward end of the list */
    for (i = dpb->fullness; i > 0; i--)
      dpb->buffers[i] = dpb->buffers[i-1];

    /* Always insert new frame to the beginning of the list */
    dpb->buffers[0] = tmpFrm;
    dpb->fullness++;

    if (currPic->refType == FRM_SHORT_TERM_PIC)
      dpb->numShortTermPics++;
    else if (currPic->refType == FRM_LONG_TERM_PIC)
      dpb->numLongTermPics++;

    /* Current picture is marked unused */
    currPic->forOutput = 0;
    currPic->refType   = FRM_NON_REF_PIC;

    /* No pictures in ouput queue */
    return 0;
  }

  /* Case 3: output pictures with bumping process until there is space in dpb or
  *  current pic is non-reference and has lowest poc */
  freeBufferFound = 0;
  numOutput       = 0;
  while (!freeBufferFound) {

    /* Next picture to output is a picture having smallest POC in DPB */
    tmpFrm = dpbGetNextOutputPic(dpb, &dpbHasIDRorMMCO5);

    /* Current picture is output if it is non-reference picture having */
    /* smaller POC than any of the pictures in DPB                     */
    if (currPic->refType == FRM_NON_REF_PIC &&
        (tmpFrm == 0 || (!dpbHasIDRorMMCO5 && currPic->poc < tmpFrm->poc)))
    {
      /* Output current picture  */
      currPic->forOutput = 0;
      outputQueue[numOutput] = currPic;
      numOutput++;
      freeBufferFound = 1;
    }
    else {
      /* Output picture that we got from DPB */
      tmpFrm->forOutput = 0;
      outputQueue[numOutput] = tmpFrm;
      numOutput++;
      if (tmpFrm->refType == FRM_NON_REF_PIC)
        freeBufferFound = 1;
    }
  }

  return numOutput;
}


/*
 *
 * dpbUpdatePicNums:
 *
 * Parameters:
 *      dpb                   DPB object
 *      frameNum              Current picture frame number
 *      maxFrameNum           Maximum frame number
 *
 * Function:
 *      Compute picture numbers for all pictures in DPB.
 *
 * Returns:
 *      -
 *
 */
void dpbUpdatePicNums(dpb_s *dpb, int32 frameNum, int32 maxFrameNum)
{
  int i;
  frmBuf_s **refPicList;

  refPicList = dpb->buffers;

  for (i = 0; i < dpb->fullness; i++) {
    if (refPicList[i]->refType == FRM_SHORT_TERM_PIC) {
      /* Short term pictures */
      if (refPicList[i]->frameNum > frameNum)
        refPicList[i]->picNum = refPicList[i]->frameNum - maxFrameNum;
      else
        refPicList[i]->picNum = refPicList[i]->frameNum;
    }
    else if (refPicList[i]->refType == FRM_LONG_TERM_PIC)
      /* Long term pictures */
      refPicList[i]->longTermPicNum = refPicList[i]->longTermFrmIdx;
  }
}


/*
 *
 * dpbMarkAllPicsAsNonRef:
 *
 * Parameters:
 *      dpb                   DPB object
 *
 * Function:
 *      Mark all picrures as non-reference pictures.
 *
 * Returns:
 *      -
 *
 */
void dpbMarkAllPicsAsNonRef(dpb_s *dpb)
{
  int i;

  /* Mark all pictures as not used for reference */
  for (i = 0; i < dpb->fullness; i++)
    dpb->buffers[i]->refType = FRM_NON_REF_PIC;

  dpb->numShortTermPics = 0;
  dpb->numLongTermPics  = 0;
}


/*
 *
 * dpbMarkLowestShortTermPicAsNonRef:
 *
 * Parameters:
 *      dpb                   DPB object
 *
 * Function:
 *      Mark short-term picture having lowest picture number as
 *      non-reference pictures.
 *
 * Returns:
 *      -
 *
 */
void dpbMarkLowestShortTermPicAsNonRef(dpb_s *dpb)
{
  int picIdx;
  int i;

  /* Find short term pic with lowest picNum */
  picIdx = -1;
  for (i = dpb->fullness-1; i >= 0; i--) {
    if (dpb->buffers[i]->refType == FRM_SHORT_TERM_PIC &&
        (picIdx < 0 || dpb->buffers[i]->picNum < dpb->buffers[picIdx]->picNum))
      picIdx = i;
  }

  /* Mark short term pic with lowest picNum as not reference picture */
  if (picIdx >= 0) {
    dpb->buffers[picIdx]->refType = FRM_NON_REF_PIC;
    dpb->numShortTermPics--;
  }
}


/*
 *
 * dpbMarkShortTermPicAsNonRef:
 *
 * Parameters:
 *      dpb                   DPB object
 *      picNum                Picture number
 *
 * Function:
 *      Mark short-term picture having picture number picNum as
 *      non-reference picture.
 *
 * Returns:
 *      DPB_OK or DPB_ERR_PICTURE_NOT_FOUND
 *
 */
int dpbMarkShortTermPicAsNonRef(dpb_s *dpb, int32 picNum)
{
  int i;

  for (i = 0; i < dpb->fullness; i++) {
    if (dpb->buffers[i]->refType == FRM_SHORT_TERM_PIC &&
        dpb->buffers[i]->picNum == picNum)
    {
      dpb->buffers[i]->refType = FRM_NON_REF_PIC;
      dpb->numShortTermPics--;
      return DPB_OK;
    }
  }

  return DPB_ERR_PICTURE_NOT_FOUND;
}


/*
 *
 * dpbMarkLongTermPicAsNonRef:
 *
 * Parameters:
 *      dpb                   DPB object
 *      longTermPicNum        Long-term picture number
 *
 * Function:
 *      Mark long-term picture having long-term picture number longTermPicNum
 *      as non-reference picture.
 *
 * Returns:
 *      DPB_OK or DPB_ERR_PICTURE_NOT_FOUND
 *
 */
int dpbMarkLongTermPicAsNonRef(dpb_s *dpb, int longTermPicNum)
{
  int i;

  for (i = 0; i < dpb->fullness; i++) {
    if (dpb->buffers[i]->refType == FRM_LONG_TERM_PIC &&
        dpb->buffers[i]->longTermPicNum == longTermPicNum)
    {
      dpb->buffers[i]->refType = FRM_NON_REF_PIC;
      dpb->numLongTermPics--;
      return DPB_OK;
    }
  }

  return DPB_ERR_PICTURE_NOT_FOUND;
}


/*
 *
 * dpbVerifyLongTermFrmIdx:
 *
 * Parameters:
 *      dpb                   DPB object
 *      longTermFrmIdx        Long-term frame index
 *
 * Function:
 *      If there is a long-term picture having long term frame index
 *      longTermFrmIdx, mark that picture as non-reference picture.
 *
 * Returns:
 *      -
 *
 */
void dpbVerifyLongTermFrmIdx(dpb_s *dpb, int longTermFrmIdx)
{
  int i;

  /* Check if longTermFrmIdx is already in use */
  for (i = 0; i < dpb->fullness; i++) {
    if (dpb->buffers[i]->refType == FRM_LONG_TERM_PIC &&
        dpb->buffers[i]->longTermFrmIdx == longTermFrmIdx)
    {
      dpb->buffers[i]->refType = FRM_NON_REF_PIC;
      dpb->numLongTermPics--;
      break;
    }
  }
}


/*
 *
 * dpbMarkShortTermPicAsLongTerm:
 *
 * Parameters:
 *      dpb                   DPB object
 *      picNum                Picture number
 *      longTermFrmIdx        Long-term frame index
 *
 * Function:
 *      Mark short-term picture having picture number picNum as long-term
 *      picture having long-term frame index longTermFrmIdx.
 *
 * Returns:
 *      DPB_OK or DPB_ERR_PICTURE_NOT_FOUND
 *
 */
int dpbMarkShortTermPicAsLongTerm(dpb_s *dpb, int32 picNum, int longTermFrmIdx)
{
  int i;

  /* To avoid duplicate of longTermFrmIdx */
  dpbVerifyLongTermFrmIdx(dpb, longTermFrmIdx);

  /* Mark pic with picNum as long term and assign longTermFrmIdx to it */
  for (i = 0; i < dpb->fullness; i++) {
    if (dpb->buffers[i]->refType == FRM_SHORT_TERM_PIC &&
        dpb->buffers[i]->picNum == picNum)
    {
      dpb->buffers[i]->refType = FRM_LONG_TERM_PIC;
      dpb->buffers[i]->longTermFrmIdx = longTermFrmIdx;
      dpb->numShortTermPics--;
      dpb->numLongTermPics++;
      return DPB_OK;
    }
  }

  return DPB_ERR_PICTURE_NOT_FOUND;
}


/*
 *
 * dpbSetMaxLongTermFrameIdx:
 *
 * Parameters:
 *      dpb                     DPB object
 *      maxLongTermFrmIdxPlus1  Maximum long-term frame index plus 1
 *
 * Function:
 *      Set maximum long-term frame index. All long-term pictures having
 *      bigger long-term frame index than maxLongTermFrmIdxPlus1-1 are
 *      marked as non-reference pictures.
 *
 * Returns:
 *      -
 *
 */
void dpbSetMaxLongTermFrameIdx(dpb_s *dpb, int maxLongTermFrmIdxPlus1)
{
  int i;

  for (i = 0; i < dpb->fullness; i++) {
    if (dpb->buffers[i]->refType == FRM_LONG_TERM_PIC &&
        dpb->buffers[i]->longTermFrmIdx > maxLongTermFrmIdxPlus1-1)
    {
      dpb->buffers[i]->refType = FRM_NON_REF_PIC;
      dpb->numLongTermPics--;
    }
  }

  dpb->maxLongTermFrameIdx = maxLongTermFrmIdxPlus1 - 1;
}
