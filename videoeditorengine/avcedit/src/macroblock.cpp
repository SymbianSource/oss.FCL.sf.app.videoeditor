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
#include "macroblock.h"
#include "motcomp.h"
#include "framebuffer.h"
#include "vld.h"
#include "parameterset.h"

#ifdef USE_CLIPBUF
#include "clipbuf.h"
#endif

/*
 * Static functions
 */

static int getMacroblock(macroblock_s *mb, int numRefFrames,
                         int8 *ipTab, int8 **numCoefUpPred, int diffVecs[][2],
                         int picType, int chromaQpIdx, bitbuffer_s *bitbuf);

static int getMbAvailability(macroblock_s *mb, mbAttributes_s *mbData,
                             int picWidth, int constrainedIntra);


#ifdef USE_CLIPBUF
const u_int8 *mcpGetClip8Buf()
{
  return clip8Buf;
}
#endif

/*
 *
 * getMacroblock:
 *
 * Parameters:
 *      mb                    Macroblock parameters
 *      multRef               1 -> multiple reference frames used
 *      ipTab                 Macroblock intra pred. modes
 *      numCoefUpPred         Block coefficient counts of upper MBs
 *      diffVecs              Macroblock delta motion vectors
 *      picType               Picture type (intra/inter)
 *      chromaQpIdx           Chroma QP index relative to luma QP
 *      bitbuf                Bitbuffer handle
 *
 * Function:
 *      Get macroblock parameters from bitbuffer
 *      
 * Returns:
 *      MBK_OK for no error, MBK_ERROR for error
 *
 */
static int getMacroblock(macroblock_s *mb, int numRefFrames,
                         int8 *ipTab, int8 **numCoefUpPred, int diffVecs[][2],
                         int picType, int chromaQpIdx, bitbuffer_s *bitbuf)
{
  vldMBtype_s hdr;
  int numVecs;
  int delta_qp;
  int i;
  int8 *numCoefPtrY, *numCoefPtrU, *numCoefPtrV;
  int retCode;


  numCoefPtrY = &numCoefUpPred[0][mb->blkX];
  numCoefPtrU = &numCoefUpPred[1][mb->blkX>>1];
  numCoefPtrV = &numCoefUpPred[2][mb->blkX>>1];

  /*
   * Get Macroblock type
   */

  /* Check if we have to fetch run indicator */
  if (IS_SLICE_P(picType) && mb->numSkipped < 0) {

    mb->numSkipped = vldGetRunIndicator(bitbuf);

    if (bibGetStatus(bitbuf) < 0)
      return MBK_ERROR;
  }

  if (IS_SLICE_P(picType) && mb->numSkipped > 0) {

    /* If skipped MBs, set MB to COPY */
    mb->type = MBK_INTER;
    mb->interMode = MOT_COPY;
    mb->refNum[0] = 0;
    mb->cbpY = mb->cbpChromaDC = mb->cbpC = 0;
    mb->numSkipped -= 1;

    vldGetZeroLumaCoeffs(numCoefPtrY, mb->numCoefLeftPred);
    vldGetZeroChromaCoeffs(numCoefPtrU, numCoefPtrV, mb->numCoefLeftPredC);

    return MBK_OK;
  }
  else {

    if (vldGetMBtype(bitbuf, &hdr, picType) < 0) {
      PRINT((_L("Error: illegal MB type\n")));     
      return MBK_ERROR;
    }

    mb->type        = hdr.type;
    mb->intraType   = hdr.intraType;
    mb->intraMode   = hdr.intraMode;
    mb->interMode   = hdr.interMode;

    for (i = 0; i < 4; i++)
      mb->inter8x8modes[i] = hdr.inter8x8modes[i];

    mb->cbpY        = hdr.cbpY;
    mb->cbpChromaDC = hdr.cbpChromaDC;
    mb->cbpC        = hdr.cbpC;

    mb->numSkipped -= 1;
  }

  if (mb->type == MBK_INTRA && mb->intraType == MBK_INTRA_TYPE_PCM) {
    vldGetAllCoeffs(numCoefPtrY, numCoefPtrU, numCoefPtrV,
                    mb->numCoefLeftPred, mb->numCoefLeftPredC);
    return MBK_OK;
  }

  /*
   * 4x4 intra prediction modes 
   */
  if (mb->type == MBK_INTRA && mb->intraType == MBK_INTRA_TYPE1) {

    if (vldGetIntraPred(bitbuf, ipTab) < 0) {
      PRINT((_L("Error: illegal intra pred\n")));     
      return MBK_ERROR;
    }
  }

  /*
   * 8x8 chroma intra prediction mode
   */
  if (mb->type == MBK_INTRA) {

    mb->intraModeChroma = vldGetChromaIntraPred(bitbuf);

    if (mb->intraModeChroma < 0) {
      PRINT((_L("Error: illegal chroma intra pred\n")));     
      return MBK_ERROR;
    }
  }

  /*
   * Reference frame number and motion vectors
   */
  if (mb->type == MBK_INTER) {

    numVecs = mcpGetNumMotVecs(mb->interMode, mb->inter8x8modes);
    mb->numMotVecs = numVecs;

    retCode = vldGetMotVecs(bitbuf, mb->interMode, numRefFrames,
                            mb->refNum, diffVecs, numVecs);

    if (retCode < 0) {
      PRINT((_L("Error: illegal motion vectors\n")));     
      return MBK_ERROR;
    }
  }

  /*
   * Coded block pattern
   */
  if (!(mb->type == MBK_INTRA && mb->intraType == MBK_INTRA_TYPE2)) {

    retCode = vldGetCBP(bitbuf, mb->type, &mb->cbpY, &mb->cbpChromaDC, &mb->cbpC);

    if (retCode < 0) {
      PRINT((_L("Error: illegal CBP\n")));     
      return MBK_ERROR;
    }
  }


  /* Delta QP */
  if ((mb->type == MBK_INTRA && mb->intraType == MBK_INTRA_TYPE2) || 
      (mb->cbpY | mb->cbpChromaDC | mb->cbpC) != 0)
  {
    retCode = vldGetDeltaqp(bitbuf, &delta_qp);

    if (retCode < 0 || delta_qp < -(MAX_QP-MIN_QP+1)/2 || delta_qp >= (MAX_QP-MIN_QP+1)/2) {
      PRINT((_L("Error: illegal delta qp\n")));     
      return MBK_ERROR;
    }

    if (delta_qp != 0) {
      int qp = mb->qp + delta_qp;
      if (qp < MIN_QP)
        qp += (MAX_QP-MIN_QP+1);
      if (qp > MAX_QP)
        qp -= (MAX_QP-MIN_QP+1);
      mb->qp = qp;
      mb->qpC = qpChroma[clip(MIN_QP, MAX_QP, mb->qp + chromaQpIdx)];
    }
  }


  /*
   * Get transform coefficients
   */

  /*
   * Luma DC coefficients (if 16x16 intra)
   */
  if (mb->type == MBK_INTRA && mb->intraType == MBK_INTRA_TYPE2) {

    retCode = vldGetLumaDCcoeffs(bitbuf, mb->dcCoefY, numCoefPtrY,
                             mb->numCoefLeftPred, mb->mbAvailBits);
    if (retCode < 0) {
      PRINT((_L("Error: illegal luma DC coefficient\n")));     
      return MBK_ERROR;
    }
  }

  /*
   * Luma AC coefficients
   */
  if (mb->cbpY != 0) {

    retCode = vldGetLumaCoeffs(bitbuf, mb->type, mb->intraType, &mb->cbpY,
                               mb->coefY, numCoefPtrY, mb->numCoefLeftPred,
                               mb->mbAvailBits);
    if (retCode < 0) {
      PRINT((_L("Error: illegal luma AC coefficient\n")));     
      return MBK_ERROR;
    } 
  }
  else
    vldGetZeroLumaCoeffs(numCoefPtrY, mb->numCoefLeftPred);

  /*
   * Chroma DC coefficients
   */
  if (mb->cbpChromaDC != 0) {

    retCode = vldGetChromaDCcoeffs(bitbuf, mb->dcCoefC, &mb->cbpChromaDC);

    if (retCode < 0) {
      PRINT((_L("Error: illegal chroma DC coefficient\n")));     
      return MBK_ERROR;
    }
  }

  /*
   * Chroma AC coefficients
   */
  if (mb->cbpC != 0) {

    retCode = vldGetChromaCoeffs(bitbuf, mb->coefC, &mb->cbpC, numCoefPtrU, numCoefPtrV,
                             mb->numCoefLeftPredC[0], mb->numCoefLeftPredC[1], mb->mbAvailBits);
    if (retCode < 0) {
      PRINT((_L("Error: illegal chroma AC coefficient\n")));     
      return MBK_ERROR;
    }
  }
  else {
    vldGetZeroChromaCoeffs(numCoefPtrU, numCoefPtrV, mb->numCoefLeftPredC);
  }

  return MBK_OK;
}


/*
 *
 * mbkSetInitialQP:
 *
 * Parameters:
 *      mb                    Macroblock object
 *      qp                    Quantization parameter
 *      chromaQpIdx           Chroma QP index relative to luma QP
 *
 * Function:
 *      Set macroblock qp.
 *      
 * Returns:
 *      -
 *
 */
void mbkSetInitialQP(macroblock_s *mb, int qp, int chromaQpIdx)
{
  mb->qp  = qp;
  mb->qpC = qpChroma[clip(MIN_QP, MAX_QP, qp + chromaQpIdx)];

  mb->numSkipped = -1;
}


/*
 *
 * getMbAvailability:
 *
 * Parameters:
 *      mb                    Macroblock object
 *      mbData                Buffers for for macroblock attributes
 *      picWidth              Picture width
 *      constrainedIntra      Constrained intra prediction flag
 *
 * Function:
 *      Get neighboring macroblock availability info
 *      
 * Returns:
 *      Macroblock availability bits:
 *        bit 0 : left macroblock
 *        bit 1 : upper macroblock
 *        bit 2 : upper-right macroblock
 *        bit 3 : upper-left macroblock
 *        bit 4 : left macroblock (intra)
 *        bit 5 : upper macroblock (intra)
 *        bit 6 : upper-right macroblock (intra)
 *        bit 7 : upper-left macroblock (intra)
 */
static int getMbAvailability(macroblock_s *mb, mbAttributes_s *mbData,
                             int picWidth, int constrainedIntra)
{
  int mbsPerLine;
  int mbAddr;
  int currSliceIdx;
  int *sliceMap;
  int8 *mbTypeTable;
  int mbAvailBits;

  mbsPerLine = picWidth/MBK_SIZE;
  mbAddr = mb->idxY*mbsPerLine+mb->idxX;

  sliceMap = & mbData->sliceMap[mbAddr];
  currSliceIdx = sliceMap[0];

  mbAvailBits = 0;

  /* Check availability of left macroblock */
  if (mb->idxX > 0 && sliceMap[-1] == currSliceIdx)
    mbAvailBits |= 0x11;

  /* Check availability of upper, upper-left and upper-right macroblocks */

  if (mb->idxY > 0) {

    sliceMap -= mbsPerLine;

    /* Check availability of upper macroblock */
    if (sliceMap[0] == currSliceIdx)
      mbAvailBits |= 0x22;

    /* Check availability of upper-right macroblock */
    if (mb->idxX+1 < mbsPerLine && sliceMap[1] == currSliceIdx)
      mbAvailBits |= 0x44;

    /* Check availability of upper-left macroblock */
    if (mb->idxX > 0 && sliceMap[-1] == currSliceIdx)
      mbAvailBits |= 0x88;
  }


  /*
   * Check availability of intra MB if constrained intra is enabled
   */

  if (constrainedIntra) {

     mbTypeTable = & mbData->mbTypeTable[mbAddr];

    /* Check availability of left intra macroblock */
    if ((mbAvailBits & 0x10) && mbTypeTable[-1] != MBK_INTRA)
      mbAvailBits &= ~0x10;

    /* Check availability of upper, upper-left and upper-right intra macroblocks */

    if (mbAvailBits & (0x20|0x40|0x80)) {

      mbTypeTable -= mbsPerLine;

      /* Check availability of upper intra macroblock */
      if ((mbAvailBits & 0x20) && mbTypeTable[0] != MBK_INTRA)
        mbAvailBits &= ~0x20;

      /* Check availability of upper-right intra macroblock */
      if ((mbAvailBits & 0x40) && mbTypeTable[1] != MBK_INTRA)
        mbAvailBits &= ~0x40;

      /* Check availability of upper-left intra macroblock */
      if ((mbAvailBits & 0x80) && mbTypeTable[-1] != MBK_INTRA)
        mbAvailBits &= ~0x80;
    }
  }

  return mbAvailBits;
}


// mbkParse
// Parses the input macroblock. If PCM coding is used then re-aligns the byte 
// alignment if previous (slice header) modifications have broken the alignment.
TInt mbkParse(macroblock_s *mb, TInt numRefFrames, mbAttributes_s *mbData, 
			  TInt picWidth, TInt picType, TInt constIpred, TInt chromaQpIdx,
              TInt mbIdxX, TInt mbIdxY, void *streamBuf, TInt aBitOffset)
{
  	TInt8 ipTab[BLK_PER_MB*BLK_PER_MB];
  	TInt diffVecs[BLK_PER_MB*BLK_PER_MB][2];
//  	TInt hasDc;
//  	TInt pixOffset;
  	TInt constrainedIntra;
  	TInt copyMbFlag;
  	TInt mbAddr;
  	TInt pcmMbFlag;
  	TInt retCode;

  	mb->idxX = mbIdxX;
  	mb->idxY = mbIdxY;

  	mb->blkX = mbIdxX*BLK_PER_MB;
  	mb->blkY = mbIdxY*BLK_PER_MB;

  	mb->pixX = mbIdxX*MBK_SIZE;
  	mb->pixY = mbIdxY*MBK_SIZE;

  	mbAddr = mb->idxY*(picWidth/MBK_SIZE)+mb->idxX;

  	copyMbFlag = pcmMbFlag = 0;

  	constrainedIntra = constIpred && !(IS_SLICE_I(picType));

  	mb->mbAvailBits = getMbAvailability(mb, mbData, picWidth, constrainedIntra);

  	// Read macroblock bits
    retCode = getMacroblock(mb, numRefFrames, ipTab, mbData->numCoefUpPred, diffVecs,
                            picType, chromaQpIdx, (bitbuffer_s *)streamBuf);

  	if (retCode < 0)
	    return retCode;

  	// Set PCM flag 
  	if (mb->type == MBK_INTRA && mb->intraType == MBK_INTRA_TYPE_PCM)
	    pcmMbFlag = 1;


  	// Get intra/inter prediction
  	if (mb->type == MBK_INTRA) 
  	{
    	mbData->mbTypeTable[mbAddr] = MBK_INTRA;

    	if (pcmMbFlag) 
    	{
    		bitbuffer_s* tempBitBuffer = (bitbuffer_s *)streamBuf;
    		
    		// Synchronize bitbuffer bit position to get it between 1 and 8
			syncBitBufferBitpos(tempBitBuffer);
	
   			// To find out how much we have to shift to reach the byte alignment again,
   			// we have to first find out the old alignment
   			// oldAlignment is the place of the bitpos before the aBitOffset, i.e. it is
   			// the amount of zero alignment bits plus one
   			TInt oldAlignmentBits = tempBitBuffer->bitpos + aBitOffset - 1;
   			TInt shiftAmount;
	   			
   		
   			// If Bit-wise shift, i.e. aBitOffset is zero, do nothing
    		// Fix the possible bit buffer byte alignment
    		if (aBitOffset > 0)
    		{
    		
    			// aBitOffset > 0 indicates a bitshift to the right
    			
    			// To counter the shift to right we have to shift left by the same amount 
    			// unless shift is larger than the number of original alignment bits in 
    			// which case we have to shift more to the right 
     			
	   			// If the computed old alignment bits value is larger than eight, 
	   			//the correct value is (computed value) % 8
	   			oldAlignmentBits = oldAlignmentBits % 8;

    			if ( oldAlignmentBits < aBitOffset )
    			{
    				// When the amount of shift is larger than the number of original alignment bits,
    				// shift right to fill up the rest of the current byte with zeros
    				shiftAmount = 8 - aBitOffset;

   					// Here we can't shift back left since there were not enough alignment bits originally, 
   					// thus we have to shift right by new bit position - tempBitBuffer->bitpos

   					// E.g. original alignment bits 2, right shift by 4 bits:
   					/////////////////////////////////////////////////////////////////
					// original             after bit shift      byte alignment reset
					// 1. byte: 2. byte:    1. byte: 2. byte:    1. byte: 2. byte: 3. byte:
					// xxxxxx00 yyyyyyyy -> xxxxxxxx xx00yyyy -> xxxxxxxx xx000000 yyyyyyyy
   					/////////////////////////////////////////////////////////////////
   					ShiftBitBufferBitsRight(tempBitBuffer, shiftAmount);
    			}
    			else
    			{
   					// In this case, the old alignment bits are more than enough 
   					// to shift back left by the aBitOffset amount
   					
   					// E.g. original alignment bits 4, right shift by 2 bits:
   					/////////////////////////////////////////////////////////////////
					// original             after bit shift      byte alignment reset
   					// 1. byte: 2. byte:    1. byte: 2. byte:    1. byte: 2. byte:
   					// xxxx0000 yyyyyyyy -> xxxxxx00 00yyyyyy -> xxxxxx00 yyyyyyyy
   					/////////////////////////////////////////////////////////////////
   					ShiftBitBufferBitsLeft(tempBitBuffer, aBitOffset);
    			}
    		}
    		else if(aBitOffset < 0)
    		{
    			// There was a bit shift to left
    			// Change the aBitOffset sign to positive
    			aBitOffset = -aBitOffset;
    		
				// If the computed alignment bits is negative the correct value is -(computed value)
				if ( oldAlignmentBits < 0 )
				{
					oldAlignmentBits = -oldAlignmentBits;
				}
    			
    			if ( oldAlignmentBits + aBitOffset >= 8 )
    			{
					// When old alignment bits plus the shift are at least 8, then 
					// we have to shift left by the 8 - shift to reach byte alignment. 
	    			shiftAmount = 8 - aBitOffset;

   					// E.g. original alignment bits 6, left shift by 4 bits:
   					/////////////////////////////////////////////////////////////////
					// original             after bit shift      byte alignment reset
					// 1. byte: 2. byte:    1. byte: 2. byte:    1. byte: 2. byte: 
					// xx000000 yyyyyyyy -> xxxxxx00 0000yyyy -> xxxxxx00 yyyyyyyy 
   					/////////////////////////////////////////////////////////////////
   					ShiftBitBufferBitsLeft(tempBitBuffer, shiftAmount);
    			}
    			else
    			{

   					// Here we can just shift right by the amount of bits shifted left to reach 
   					// the byte alignment

   					// E.g. original alignment bits 2, left shift by 4 bits:
   					/////////////////////////////////////////////////////////////////
					// original             after bit shift      byte alignment reset
					// 1. byte: 2. byte:    1. byte: 2. byte:    1. byte: 2. byte: 
					// xxxxxx00 yyyyyyyy -> xx00yyyy yyyyyyyy -> xx000000 yyyyyyyy 
   					/////////////////////////////////////////////////////////////////
   					ShiftBitBufferBitsRight(tempBitBuffer, aBitOffset);
    			}
    		}
    	
    		return MBK_PCM_FOUND;
    	
    	}
  	}
  	else 
  	{

    	mbData->mbTypeTable[mbAddr] = (TInt8)(mb->interMode+1);

    	// If COPY MB, put skip motion vectors 
    	if (mb->interMode == MOT_COPY) 
    	{
      		mb->interMode = MOT_16x16;
    	}

  	}


  	// Decode prediction error & reconstruct MB
  	if (!copyMbFlag && !pcmMbFlag) 
  	{

    	// If 4x4 intra mode, luma prediction error is already transformed 
    	if (!(mb->type == MBK_INTRA && mb->intraType == MBK_INTRA_TYPE1)) 
    	{

//      		hasDc = (mb->type == MBK_INTRA && mb->intraType == MBK_INTRA_TYPE2) ? 1 : 0;

    	}

//    	pixOffset = ((mb->pixY*picWidth)>>2)+(mb->pixX>>1);
  	}


  	// Store qp and coded block pattern for current macroblock
	if (pcmMbFlag)
	    mbData->qpTable[mbAddr] = 0;
  	else
    	mbData->qpTable[mbAddr] = (TInt8)mb->qp;

  	mbData->cbpTable[mbAddr] = mb->cbpY;

  	return MBK_OK;
}

