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

/*
 * Static functions
 */

static void *allocMem(int blkSize, size_t unitSize);

static frmBuf_s *initRefFrame(frmBuf_s *recoBuf, frmBuf_s *refBuf);



/*
 *
 * allocMem:
 *
 * Parameters:
 *      blkSize               Block size
 *      unitSize              unit size
 *
 * Function:
 *      Allocate blkSize*unitSize bytes of memory
 *
 * Returns:
 *      Pointer to allocated memory or NULL
 *
 */
static void *allocMem(int blkSize, size_t unitSize)
{
  void *mem;

  if ((mem = User::Alloc(blkSize*unitSize)) == NULL) {
    PRINT((_L("Cannot allocate memory for frame\n")));     
  }

  return mem;
}


/*
 *
 * frmOpen:
 *
 * Parameters:
 *      mbData                Macroblock state buffer
 *      width                 Width of the frame
 *      height                Height of the frame
 *
 * Function:
 *      Allocate memory for frame buffer and macroblock state buffer.
 *
 * Returns:
 *      Pointer to allocated frame buffer or NULL
 *
 */
frmBuf_s *frmOpen(mbAttributes_s **mbData, int width, int height)
{
//  int numPels = width*height;
  int numBlksPerLine = width/BLK_SIZE;
  int numBlocks = numBlksPerLine*height/BLK_SIZE;
  int numMacroblocks = width/MBK_SIZE*height/MBK_SIZE;
  frmBuf_s *recoBuf;
  mbAttributes_s *mbDataTmp;

  if ((recoBuf = (frmBuf_s *)User::Alloc(sizeof(frmBuf_s))) == NULL)
    return NULL;

  memset(recoBuf, 0, sizeof(frmBuf_s));

  recoBuf->width = width;
  recoBuf->height = height;

  if ((mbDataTmp = (mbAttributes_s *)User::Alloc(sizeof(mbAttributes_s))) == NULL)
    return NULL;

  memset(mbDataTmp, 0, sizeof(mbAttributes_s));

  if ((mbDataTmp->sliceMap = (int *)allocMem(numMacroblocks, sizeof(int))) == NULL)
    return NULL;
  if ((mbDataTmp->mbTypeTable = (int8 *)allocMem(numMacroblocks, sizeof(int8))) == NULL)
    return NULL;
  if ((mbDataTmp->qpTable = (int8 *)allocMem(numMacroblocks, sizeof(int8))) == NULL)
    return NULL;
  if ((mbDataTmp->refIdxTable = (int8 *)allocMem(numBlocks, sizeof(int8))) == NULL)
    return NULL;
  if ((mbDataTmp->cbpTable = (int *)allocMem(numMacroblocks, sizeof(int))) == NULL)
    return NULL;
  if ((mbDataTmp->motVecTable = (motVec_s *)allocMem(numBlocks, sizeof(motVec_s))) == NULL)
    return NULL;
  if ((mbDataTmp->ipModesUpPred = (int8 *)allocMem(numBlksPerLine, sizeof(int8))) == NULL)
    return NULL;

  if ((mbDataTmp->numCoefUpPred[0] = (int8 *)allocMem(numBlksPerLine, sizeof(int8))) == NULL)
    return NULL;
  if ((mbDataTmp->numCoefUpPred[1] = (int8 *)allocMem(numBlksPerLine/2, sizeof(int8))) == NULL)
    return NULL;
  if ((mbDataTmp->numCoefUpPred[2] = (int8 *)allocMem(numBlksPerLine/2, sizeof(int8))) == NULL)
    return NULL;

  if ((mbDataTmp->filterModeTab = (int8 *)allocMem(numMacroblocks, sizeof(int8))) == NULL)
    return NULL;
  if ((mbDataTmp->alphaOffset = (int8 *)allocMem(numMacroblocks, sizeof(int8))) == NULL)
    return NULL;
  if ((mbDataTmp->betaOffset = (int8 *)allocMem(numMacroblocks, sizeof(int8))) == NULL)
    return NULL;

  *mbData = mbDataTmp;

  return recoBuf;
}


/*
 *
 * frmOpenRef:
 *
 * Parameters:
 *      width                 Width of the frame
 *      height                Height of the frame
 *
 * Function:
 *      Allocate memory for reference frame buffer
 *
 * Returns:
 *      Pointer to Reference frame or NULL
 *
 */
frmBuf_s *frmOpenRef(int width, int height)
{
//  int numPels = width*height;
  frmBuf_s *ref;

  if ((ref = (frmBuf_s *)User::Alloc(sizeof(frmBuf_s))) == NULL)
    return NULL;

  memset(ref, 0, sizeof(frmBuf_s));

  ref->width      = width;
  ref->height     = height;
  ref->imgPadding = 0;

  ref->forOutput = 0;
  ref->refType = FRM_NON_REF_PIC;

  return ref;
}


/*
 *
 * frmClose:
 *
 * Parameters:
 *      frame                 Frame
 *      mbData                MB state buffers
 *
 * Function:
 *      Deallocate frame buffer and state array memory.
 *
 * Returns:
 *      Nothing
 *
 */
void frmClose(frmBuf_s *recoBuf, mbAttributes_s *mbData)
{
  if (!recoBuf)
    return;

  User::Free(mbData->sliceMap);
  User::Free(mbData->mbTypeTable);
  User::Free(mbData->qpTable);
  User::Free(mbData->refIdxTable);
  User::Free(mbData->cbpTable);
  User::Free(mbData->motVecTable);
  User::Free(mbData->ipModesUpPred);
  User::Free(mbData->numCoefUpPred[0]);
  User::Free(mbData->numCoefUpPred[1]);
  User::Free(mbData->numCoefUpPred[2]);
  User::Free(mbData->filterModeTab);
  User::Free(mbData->alphaOffset);
  User::Free(mbData->betaOffset);
  User::Free(recoBuf);
  User::Free(mbData);
}


/*
 *
 * frmCloseRef:
 *
 * Parameters:
 *      ref                   Reference frame
 *
 * Function:
 *      Deallocate reference frame buffer memory.
 *
 * Returns:
 *      Nothing
 *
 */
void frmCloseRef(frmBuf_s *ref)
{
  if (!ref)
    return;
  
  User::Free(ref);
}


/*
 *
 * initRefFrame:
 *
 * Parameters:
 *      recoBuf                Reconstruction frame
 *      frameBuf               Frame buffer to initialize
 *
 * Function:
 *      Initialize reference frame buffer refBuf using reconstructed buffer
 *      recoBuf. If width and height of the reference buffer do not those
 *      of the reconstructed buffer, reference buffer is reallocated.
 *
 * Returns:
 *      Pointer to reference frame
 *
 */
static frmBuf_s *initRefFrame(frmBuf_s *recoBuf, frmBuf_s *refBuf)
{

  /* If pic size is different, reopen with new size */
  if (!refBuf || refBuf->width != recoBuf->width || refBuf->height != recoBuf->height) {
    frmCloseRef(refBuf);
    if ((refBuf = frmOpenRef(recoBuf->width, recoBuf->height)) == NULL)
      return NULL;
  }

  /* Copy variables */
  *refBuf = *recoBuf;

  return refBuf;
}


/*
 *
 * frmMakeRefFrame:
 *
 * Parameters:
 *      recoBuf               Reconstructed frame
 *      refBuf                Reference frame
 *
 * Function:
 *      Generate reference frame refBuf using reconstructed frame recoBuf.
 *      Function does not copy pixel data, but it simply swaps pointers.
 *
 * Returns:
 *      Pointer to reference frame
 *
 */
frmBuf_s *frmMakeRefFrame(frmBuf_s *recoBuf, frmBuf_s *refBuf)
{

  refBuf = initRefFrame(recoBuf, refBuf);
  if (refBuf == NULL)
    return NULL;

  return refBuf;
}
