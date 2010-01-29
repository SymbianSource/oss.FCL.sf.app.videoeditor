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


#ifndef _MACROBLOCK_H_
#define _MACROBLOCK_H_


#include "globals.h"
#include "framebuffer.h"
#include "bitbuffer.h"


#define MBK_ERROR  -1
#define MBK_OK      0
#define MBK_PCM_FOUND  2


typedef struct _macroblock_s
{
  int type;
  int numSkipped;

  int intraType;
  int intraMode;
  int intraModeChroma;

  int interMode;
  int inter8x8modes[4];
  int refNum[4];
  int numMotVecs;

  int qp, qpC;
  int idxX, idxY;
  int blkX, blkY;
  int pixX, pixY;

  int cbpY, cbpC, cbpChromaDC;

  u_int8 predY[MBK_SIZE][MBK_SIZE];
  u_int8 predC[MBK_SIZE/2][MBK_SIZE];

  int dcCoefY[BLK_PER_MB][BLK_PER_MB];
  int dcCoefC[2][BLK_PER_MB/2][BLK_PER_MB/2];

  int coefY[BLK_PER_MB][BLK_PER_MB][BLK_SIZE][BLK_SIZE];
  int coefC[2][BLK_PER_MB/2][BLK_PER_MB/2][BLK_SIZE][BLK_SIZE];

  int mbAvailBits;

  int8 numCoefLeftPred[BLK_PER_MB];
  int8 numCoefLeftPredC[2][BLK_PER_MB/2];

  int8 ipModesLeftPred[BLK_PER_MB];

} macroblock_s;


void mbkSetInitialQP(macroblock_s *mb, int qp, int chromaQpIdx);

TInt mbkParse(macroblock_s *mb, 
              TInt numRefFrames, mbAttributes_s *mbData, TInt picWidth, 
              TInt picType, TInt constIpred, TInt chromaQpIdx,
              TInt mbIdxX, TInt mbIdxY, void *streamBuf, TInt aBitOffset);

#endif
