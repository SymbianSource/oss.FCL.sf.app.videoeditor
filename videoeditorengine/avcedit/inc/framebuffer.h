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


#ifndef _FRAMEBUFFER_H_
#define _FRAMEBUFFER_H_


#include "nrctyp32.h"
#include "globals.h"
#include "avcdapi.h"


#define FRM_NON_REF_PIC     0
#define FRM_SHORT_TERM_PIC  1
#define FRM_LONG_TERM_PIC   2


typedef struct _frmBuf_s {
  int constraintSet0flag;
  int constraintSet1flag;
  int constraintSet2flag;
  int profile;
  int level;
  int width;
  int height;
  unsigned cropLeftOff;
  unsigned cropRightOff;
  unsigned cropTopOff;
  unsigned cropBottomOff;

  int aspectRatioNum;
  int aspectRatioDenom;
  int overscanInfo;
  int videoFormat;
  int videoFullRangeFlag;
  int matrixCoefficients;
  int chromaSampleLocType;
  int numReorderFrames;
  float frameRate;

  int imgPadding;

  int qp;
  int chromaQpIndexOffset;

  int32 frameNum;
  int32 maxFrameNum;
  int32 picNum;
  int longTermFrmIdx;
  int longTermPicNum;
  int refType;        /* non-ref, short term or long term */
  int forOutput;      /* If this frame is waiting for output */
  int nonExisting;    
  int32 poc;
  int isIDR;
  int idrPicID;
  int hasMMCO5;
  int picType;

  int pictureStructure;

  int lossy;
/*#if defined(ERROR_CONCEALMENT) && defined(BACKCHANNEL_INFO)
  unsigned char *mbLossMap;
#endif*/

  int sceneCut;

} frmBuf_s;


typedef struct _mbAttributes_s {
  int *sliceMap;
  int8 *mbTypeTable;
  int8 *qpTable;
  int8 *refIdxTable;
  int *cbpTable;
  int8 *ipModesUpPred;
  motVec_s *motVecTable;
  int8 *numCoefUpPred[3];
  int8 *filterModeTab;
  int8 *alphaOffset;
  int8 *betaOffset;
} mbAttributes_s;


frmBuf_s *frmOpen(mbAttributes_s **mbData, int width, int height);

frmBuf_s *frmOpenRef(int width, int height);

void frmClose(frmBuf_s *recoBuf, mbAttributes_s *mbData);

void frmCloseRef(frmBuf_s *ref);

frmBuf_s *frmMakeRefFrame(frmBuf_s *recoBuf, frmBuf_s *refBuf);


#endif
