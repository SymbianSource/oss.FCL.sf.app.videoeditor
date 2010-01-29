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


#ifndef _DPB_H_
#define _DPB_H_


#include "globals.h"
#include "nrctyp32.h"
#include "framebuffer.h"


#define DPB_ERR_MEM_ALLOC          -3
#define DPB_ERR_PICTURE_NOT_FOUND  -2
#define DPB_ERROR                  -1
#define DPB_OK                      0


#define DPB_MAX_SIZE  16


typedef struct _dpb_s {
  int size;
  frmBuf_s *buffers[DPB_MAX_SIZE];

  int fullness;
  int maxNumRefFrames;
  int numShortTermPics;
  int numLongTermPics;

  int maxLongTermFrameIdx;

} dpb_s;



dpb_s *dpbOpen();

void dpbClose(dpb_s *dpb);

void dpbSetSize(dpb_s *dpb, int dpbSize);

frmBuf_s *dpbGetNextOutputPic(dpb_s *dpb, int *dpbHasIDRorMMCO5);

int dpbStorePicture(dpb_s *dpb, frmBuf_s *currPic, frmBuf_s *outputQueue[]);

void dpbUpdatePicNums(dpb_s *dpb, int32 frameNum, int32 maxFrameNum);

void dpbMarkAllPicsAsNonRef(dpb_s *dpb);

void dpbMarkLowestShortTermPicAsNonRef(dpb_s *dpb);

int dpbMarkShortTermPicAsNonRef(dpb_s *dpb, int32 picNum);

int dpbMarkLongTermPicAsNonRef(dpb_s *dpb, int longTermPicNum);

void dpbVerifyLongTermFrmIdx(dpb_s *dpb, int longTermFrmIdx);

int dpbMarkShortTermPicAsLongTerm(dpb_s *dpb, int32 picNum, int longTermFrmIdx);

void dpbSetMaxLongTermFrameIdx(dpb_s *dpb, int maxLongTermFrmIdxPlus1);


#endif
