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


#ifndef _SEQUENCE_H_
#define _SEQUENCE_H_


#include "globals.h"
#include "framebuffer.h"
#include "slice.h"
#include "bitbuffer.h"


/*
 * Definitions of NAL types
 */
#define NAL_TYPE_UNSPECIFIED      0
#define NAL_TYPE_CODED_SLICE      1
#define NAL_TYPE_CODED_SLICE_P_A  2
#define NAL_TYPE_CODED_SLICE_P_B  3
#define NAL_TYPE_CODED_SLICE_P_C  4
#define NAL_TYPE_CODED_SLICE_IDR  5
#define NAL_TYPE_SEI              6
#define NAL_TYPE_SPS              7
#define NAL_TYPE_PPS              8
#define NAL_TYPE_PIC_DELIMITER    9
#define NAL_TYPE_END_SEQ          10
#define NAL_TYPE_END_STREAM       11
#define NAL_TYPE_FILLER_DATA      12


typedef struct _sequence_s 
{

  dpb_s *dpb;

  frmBuf_s *outputQueue[DPB_MAX_SIZE];
  int outputQueuePos;
  int numQueuedOutputPics;

  bitbuffer_s *bitbuf;

  int sliceNums[PS_MAX_NUM_SLICE_GROUPS];

  seq_parameter_set_s *spsList[PS_MAX_NUM_OF_SPS];
  pic_parameter_set_s *ppsList[PS_MAX_NUM_OF_PPS];

  slice_s *currSlice;
  slice_s *nextSlice;

  int isFirstSliceOfSeq;
  int isPicBoundary;
  int isCurrPicFinished;
  int isDpbStorePending;
  int isSeqFinished;

  unsigned int redundantPicCnt;

  int32 unusedShortTermFrameNum;
  int32 prevFrameNum;
  int32 prevRefFrameNum;

  /* for POC type 0 : */
  int32 pocMsb;
  int32 prevPocMsb;
  int32 prevPocLsb;
  /* for POC type 1 : */
  int32 frameNumOffset;
  int32 prevFrameNumOffset;

  /* The previous decoded picture in decoding order includes */
  /* a memory_management_control_operation equal to 5        */
  int prevPicHasMMCO5; 
                       
  mbAttributes_s *mbData;
  frmBuf_s *recoBuf;

  TUint iFrameNumber;
  TUint iFromEncoder;
  TUint sliceDataModified;
  	    
  TBool iEncodeUntilIDR;	//	Encoded beginning of a clip has different SPS than the original
  TBool iBitShiftInSlice;
  
  TUint iNumSPS;
  TUint iNumPPS;
  
  TUint iPreviousPPSId;

  // [KW]: For testing, remove later!!!
  TUint iTotalFrameNumber;

} sequence_s;


#endif
