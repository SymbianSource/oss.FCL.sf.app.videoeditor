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


#ifndef _VLC_H_
#define _VLC_H_


#include "bitbuffer.h"


#define VLD_OK                       0
#define VLD_ERROR                   -1
#define VLD_ERR_MAX_CW_LEN_EXCEEDED -2


typedef struct _vldMBtype_s {
  int type;
  int intraType;
  int intraMode;
  int interMode;
  int inter8x8modes[4];
  int cbpY;
  int cbpC;
  int cbpChromaDC;
} vldMBtype_s;


unsigned int vldGetFLC(bitbuffer_s *bitbuf, int len);

unsigned int vldGetUVLC(bitbuffer_s *bitbuf);

int32 vldGetSignedUVLClong(bitbuffer_s *bitbuf);

u_int32 vldGetUVLClong(bitbuffer_s *bitbuf);

#ifdef VIDEOEDITORENGINE_AVC_EDITING

void vldInvZigZagScan4x4(int *src, int dest[BLK_SIZE][BLK_SIZE]);

int vldGetSignedUVLC(bitbuffer_s *bitbuf);

int getLumaBlkCbp(int cbpY);

void setChromaCbp(int nc, int *cbpDC, int *cbp);

unsigned int vldGetRunIndicator(bitbuffer_s *bitbuf);

int vldGetMBtype(bitbuffer_s *bitbuf, vldMBtype_s *hdr, int picType);

int vldGetIntraPred(bitbuffer_s *bitbuf, int8 *ipTab);

int vldGetChromaIntraPred(bitbuffer_s *bitbuf);

int vldGetMotVecs(bitbuffer_s *bitbuf, int interMode, int numRefFrames,
                  int *refNum, int predVecs[][2], int numVecs);

int vldGetCBP(bitbuffer_s *bitbuf, int type,
              int *cbpY, int *cbpChromaDC, int *cbpC);

int vldGetDeltaqp(bitbuffer_s *bitbuf, int *delta_qp);

int vldGetLumaDCcoeffs(bitbuffer_s *bitbuf, int coef[4][4],
                       int8 *numCoefUpPred, int8 *numCoefLeftPred,
                       int mbAvailBits);

int vldGetLumaCoeffs(bitbuffer_s *bitbuf, int mbType, int intraType,
                     int *cbpY, int coef[4][4][4][4], int8 *numCoefUpPred,
                     int8 *numCoefLeftPred, int mbAvailBits);

void vldGetZeroLumaCoeffs(int8 *numCoefUpPred, int8 *numCoefLeftPred);

int vldGetChromaDCcoeffs(bitbuffer_s *bitbuf, int coef[2][2][2], int *cbpDC);

int vldGetChromaCoeffs(bitbuffer_s *bitbuf, int coef[2][2][2][4][4], int *cbp,
                       int8 *numCoefUpPred, int8 *numCoefUpPredV,
                       int8 *numCoefLeftPred, int8 *numCoefLeftPredV,
                       int mbAvailBits);

void vldGetZeroChromaCoeffs(int8 *numCoefUpPredU, int8 *numCoefUpPredV,
                            int8 numCoefLeftPred[2][2]);

void vldGetAllCoeffs(int8 *numCoefUpPredY, int8 *numCoefUpPredU,
                     int8 *numCoefUpPredV, int8 *numCoefLeftPredY,
                     int8 numCoefLeftPredC[2][2]);

int vldSetUVLC(int codeNumber, int* codeword, int* codewordLength);

#endif  // VIDEOEDITORENGINE_AVC_EDITING

#endif
