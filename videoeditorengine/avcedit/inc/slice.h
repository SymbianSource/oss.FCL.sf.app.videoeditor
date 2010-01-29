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


#ifndef _SLICE_H_
#define _SLICE_H_


#include "globals.h"
#include "nrctyp32.h"
#include "bitbuffer.h"
#include "framebuffer.h"
#include "motcomp.h"
#include "parameterset.h"
#include "dpb.h"
#include <e32std.h>

/* Error codes */
#define SLICE_ERR_NON_EXISTING_PPS     -5
#define SLICE_ERR_NON_EXISTING_SPS     -4
#define SLICE_ERR_UNSUPPORTED_FEATURE  -3
#define SLICE_ERR_ILLEGAL_VALUE        -2
#define SLICE_ERROR                    -1
#define SLICE_OK                        0
#define SLICE_STOP_PARSING				1
#define SLICE_STOP_PARSING				1

#define MAX_SLICE_GROUP_NUM      8
#define MAX_NUM_OF_REORDER_CMDS  17
#define MAX_NUM_OF_MMCO_OPS      35


typedef struct _sliceMMCO_s {
  unsigned int memory_management_control_operation;
  unsigned int difference_of_pic_nums_minus1;
  unsigned int long_term_pic_num;
  unsigned int long_term_frame_idx;
  unsigned int max_long_term_frame_idx_plus1;
} sliceMMCO_s;


typedef struct _sliceRefPicListReorderCmd_s {
  unsigned int reordering_of_pic_nums_idc;
  unsigned int abs_diff_pic_num_minus1;
  unsigned int long_term_pic_num;
} sliceRefPicListReorderCmd_s;



typedef struct _slice_s {

  /* Copied from NAL deader */
  int nalType;
  int nalRefIdc;

  u_int32       maxFrameNum;

  unsigned int  isIDR;
  unsigned int  qp;
  unsigned int  picHasMMCO5;

  /*
   * These are slice header syntax elements
   */

  unsigned int  first_mb_in_slice;
  unsigned int  slice_type;
  unsigned int  pic_parameter_set_id;
  unsigned int  frame_num;

    unsigned int  idr_pic_id;

    unsigned int  pic_order_cnt_lsb;
      int32         delta_pic_order_cnt_bottom;

    int32         delta_pic_order_cnt_0;
      int32         delta_pic_order_cnt_1;

    unsigned int  redundant_pic_cnt;

  unsigned int  num_ref_idx_active_override_flag;
    unsigned int  num_ref_idx_l0_active_minus1;

  unsigned int  ref_pic_list_reordering_flag0;
    sliceRefPicListReorderCmd_s reorderCmdList[MAX_NUM_OF_REORDER_CMDS];

/* pred_weight_table() */

  /* if( nal_unit_type  = =  5 ) */
    unsigned int  no_output_of_prior_pics_flag;
    unsigned int  long_term_reference_flag;
  /* else */
    unsigned int  adaptive_ref_pic_marking_mode_flag;
      sliceMMCO_s mmcoCmdList[MAX_NUM_OF_MMCO_OPS];

  int           slice_qp_delta;

    unsigned int  disable_deblocking_filter_idc;
      int           slice_alpha_c0_offset_div2;
      int           slice_beta_offset_div2;

    unsigned int  slice_group_change_cycle;
    
    TInt	bitOffset;
    TUint sliceDataModified;
} slice_s;


slice_s *sliceOpen();

void sliceClose(slice_s *slice);

TInt ParseSliceHeader(slice_s *slice, seq_parameter_set_s *spsList[],
                     pic_parameter_set_s *ppsList[], bitbuffer_s *bitbuf, 
                     TUint* frameNumber, TUint aFrameFromEncoder);

TInt sliceParseMacroblocks(slice_s *slice, frmBuf_s *recoBuf, dpb_s *dpb,
                           pic_parameter_set_s *pps,
                           mbAttributes_s *mbData, TInt sliceID,
                           bitbuffer_s *bitbuf,
                           TBool aBitShiftInSlice);

int sliceInitRefPicList(dpb_s *dpb, frmBuf_s *refPicList[]);

int sliceFixRefPicList(dpb_s *dpb, frmBuf_s *refPicList[],
                       int numRefPicActive, int numExistingRefFrames,
                       int width, int height);
                       
void EncodeUnsignedNBits(bitbuffer_s *aBitBuffer, TUint aValue, TUint aLength);


#endif
