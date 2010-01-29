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


#ifndef _PARAMETERSET_H_
#define _PARAMETERSET_H_

#include "nrctyp32.h"
#include "bitbuffer.h"
#include <e32def.h>
#include <e32base.h>


/* Parameter set error codes */
#define PS_ERR_MEM_ALLOC            -5
#define PS_ERR_UNSUPPORTED_FEATURE  -4
#define PS_ERR_UNSUPPORTED_PROFILE  -3
#define PS_ERR_ILLEGAL_VALUE        -2
#define PS_ERROR                    -1
#define PS_OK                        0


/* Boolean Type */
typedef unsigned int Boolean;

/* According to the Awaji meeting decision: */
#define PS_MAX_NUM_OF_SPS         32
#define PS_MAX_NUM_OF_PPS         256

/* Profile IDC's */
#define PS_BASELINE_PROFILE_IDC   66
#define PS_MAIN_PROFILE_IDC       77
#define PS_EXTENDED_PROFILE_IDC   88

#define PS_MAX_CPB_CNT            32

#define PS_EXTENDED_SAR           255

/* Slice groups supported by baseline profile and extended profile */
#define PS_MAX_NUM_SLICE_GROUPS              8
#define PS_SLICE_GROUP_MAP_TYPE_INTERLEAVED  0
#define PS_SLICE_GROUP_MAP_TYPE_DISPERSED    1
#define PS_SLICE_GROUP_MAP_TYPE_FOREGROUND   2
#define PS_SLICE_GROUP_MAP_TYPE_CHANGING_3   3
#define PS_SLICE_GROUP_MAP_TYPE_CHANGING_4   4
#define PS_SLICE_GROUP_MAP_TYPE_CHANGING_5   5
#define PS_SLICE_GROUP_MAP_TYPE_EXPLICIT     6
#define PS_BASELINE_MAX_SLICE_GROUPS 		7


#define PS_MAX_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_CYCLE  256

// NOTE: Levels up to 2 are eanbled for testing purposes
//       1.2 is the max level we should support
//#define PS_MAX_SUPPORTED_LEVEL 20		// Maximum level that we support

//WVGA task
#define PS_MAX_SUPPORTED_LEVEL 31


/*
 * Hypothetical reference decoder parameters
 */

typedef struct _hrd_parameters_s
{
  unsigned    cpb_cnt_minus1;                                   // ue(v)
  unsigned    bit_rate_scale;                                   // u(4)
  unsigned    cpb_size_scale;                                   // u(4)
    u_int32   bit_rate_value_minus1[PS_MAX_CPB_CNT];            // ue(v)
    u_int32   cpb_size_value_minus1[PS_MAX_CPB_CNT];            // ue(v)
    unsigned  cbr_flag[PS_MAX_CPB_CNT];                         // u(1)
  unsigned    initial_cpb_removal_delay_length_minus1;          // u(5)
  unsigned    cpb_removal_delay_length_minus1;                  // u(5)
  unsigned    dpb_output_delay_length_minus1;                   // u(5)
  unsigned    time_offset_length;                               // u(5)
} hrd_parameters_s;


/*
 * Video usability information
 */

typedef struct _vui_parameters_s
{
  Boolean             aspect_ratio_info_present_flag;                   // u(1)
    unsigned          aspect_ratio_idc;                                 // u(8)
      unsigned        sar_width;                                        // u(16)
      unsigned        sar_height;                                       // u(16)
  Boolean             overscan_info_present_flag;                       // u(1)
    Boolean           overscan_appropriate_flag;                        // u(1)
  Boolean             video_signal_type_present_flag;                   // u(1)
    unsigned          video_format;                                     // u(3)
    Boolean           video_full_range_flag;                            // u(1)
    Boolean           colour_description_present_flag;                  // u(1)
      unsigned        colour_primaries;                                 // u(8)
      unsigned        transfer_characteristics;                         // u(8)
      unsigned        matrix_coefficients;                              // u(8)
  Boolean             chroma_loc_info_present_flag;                     // u(1)
    unsigned          chroma_sample_loc_type_top_field;                 // ue(v)
    unsigned          chroma_sample_loc_type_bottom_field;              // ue(v)
  Boolean             timing_info_present_flag;                         // u(1)
    u_int32           num_units_in_tick;                                // u(32)
    u_int32           time_scale;                                       // u(32)
    Boolean           fixed_frame_rate_flag;                            // u(1)
  Boolean             nal_hrd_parameters_present_flag;                  // u(1)
    hrd_parameters_s  nal_hrd_parameters;                               // hrd_paramters_s
  Boolean             vcl_hrd_parameters_present_flag;                  // u(1)
    hrd_parameters_s  vcl_hrd_parameters;                               // hrd_paramters_s
  // if ((nal_hrd_parameters_present_flag || (vcl_hrd_parameters_present_flag))
    Boolean           low_delay_hrd_flag;                               // u(1)
  Boolean             pic_struct_present_flag;                          // u(1)
  Boolean             bitstream_restriction_flag;                       // u(1)
    Boolean           motion_vectors_over_pic_boundaries_flag;          // u(1)
    unsigned          max_bytes_per_pic_denom;                          // ue(v)
    unsigned          max_bits_per_mb_denom;                            // ue(v)
    unsigned          log2_max_mv_length_horizontal;                    // ue(v)
    unsigned          log2_max_mv_length_vertical;                      // ue(v)
    unsigned          num_reorder_frames;                               // ue(v)
    unsigned          max_dec_frame_buffering;                          // ue(v)
} vui_parameters_s;


/*
 * Picture parameter set
 */

typedef struct _pic_parameter_set_s
{
  unsigned      pic_parameter_set_id;                          // ue(v)
  unsigned      seq_parameter_set_id;                          // ue(v)
  Boolean       entropy_coding_mode_flag;                      // u(1)
  Boolean       pic_order_present_flag;                        // u(1)
  unsigned      num_slice_groups_minus1;                       // ue(v)
    unsigned    slice_group_map_type;                          // ue(v)
      // if( slice_group_map_type = = 0 )
      unsigned  run_length_minus1[PS_MAX_NUM_SLICE_GROUPS];    // ue(v)
      // else if( slice_group_map_type = = 2 )
      unsigned  top_left[PS_MAX_NUM_SLICE_GROUPS];             // ue(v)
      unsigned  bottom_right[PS_MAX_NUM_SLICE_GROUPS];         // ue(v)
      // else if( slice_group_map_type = = 3 || 4 || 5
      Boolean   slice_group_change_direction_flag;             // u(1)
      unsigned  slice_group_change_rate_minus1;                // ue(v)
      // else if( slice_group_map_type = = 6 )
      unsigned  pic_size_in_map_units_minus1;                  // ue(v)
      unsigned  *slice_group_id;                               // complete MBAmap u(v)
  unsigned      num_ref_idx_l0_active_minus1;                  // ue(v)
  unsigned      num_ref_idx_l1_active_minus1;                  // ue(v)
  Boolean       weighted_pred_flag;                            // u(1)
  Boolean       weighted_bipred_idc;                           // u(2)
  int           pic_init_qp_minus26;                           // se(v)
  int           pic_init_qs_minus26;                           // se(v)
  int           chroma_qp_index_offset;                        // se(v)
  Boolean       deblocking_filter_parameters_present_flag;     // u(1)
  Boolean       constrained_intra_pred_flag;                   // u(1)
  Boolean       redundant_pic_cnt_present_flag;                // u(1)
  
	TUint8 indexChanged;	// Flag for changed PPS Id (because of Id conflict in clip (can happen in e.g. merge, cut operations)
  	TUint newPPSId;
  	TUint PPSlength;
  	TUint8* codedPPSBuffer;
    TUint encPPSId;
    TUint origPPSId;
} pic_parameter_set_s;


/*
 * Sequence parameter set
 */

typedef struct _seq_parameter_set_s
{
  unsigned  profile_idc;                                      // u(8)
  Boolean   constraint_set0_flag;                             // u(1)
  Boolean   constraint_set1_flag;                             // u(1)
  Boolean   constraint_set2_flag;                             // u(1)
  Boolean   constraint_set3_flag;                             // u(1)
  Boolean   reserved_zero_4bits;                              // u(4)
  unsigned  level_idc;                                        // u(8)
  unsigned  seq_parameter_set_id;                             // ue(v)
  unsigned  log2_max_frame_num_minus4;                        // ue(v)
  unsigned  pic_order_cnt_type;
    // if( pic_order_cnt_type == 0 ) 
    unsigned log2_max_pic_order_cnt_lsb_minus4;               // ue(v)
    // else if( pic_order_cnt_type == 1 )
    Boolean delta_pic_order_always_zero_flag;                 // u(1)
    int32   offset_for_non_ref_pic;                           // se(v)
    int32   offset_for_top_to_bottom_field;                   // se(v)
    unsigned  num_ref_frames_in_pic_order_cnt_cycle;          // ue(v)
      // for( i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++ )
      int32 offset_for_ref_frame[PS_MAX_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_CYCLE];   // se(v)
  unsigned  num_ref_frames;                                   // ue(v)
  Boolean   gaps_in_frame_num_value_allowed_flag;             // u(1)
  unsigned  pic_width_in_mbs_minus1;                          // ue(v)
  unsigned  pic_height_in_map_units_minus1;                   // ue(v)
  Boolean   frame_mbs_only_flag;                              // u(1)
    // if( !frame_mbs_only_flag ) 
    Boolean mb_adaptive_frame_field_flag;                     // u(1)
  Boolean   direct_8x8_inference_flag;                        // u(1)
  Boolean       frame_cropping_flag;                           // u(1)
    unsigned    frame_crop_left_offset;                        // ue(v)
    unsigned    frame_crop_right_offset;                       // ue(v)
    unsigned    frame_crop_top_offset;                         // ue(v)
    unsigned    frame_crop_bottom_offset;                      // ue(v)
  Boolean   vui_parameters_present_flag;                      // u(1)
    vui_parameters_s vui_parameters;                          // vui_seq_parameters_s
    
  	TUint8 indexChanged;	// Flag for changed PPS Id (because of Id conflict in clip (can happen in e.g. merge, cut operations)
  	TUint newSPSId;
  	TUint SPSlength;
    TUint8* codedSPSBuffer;
    TUint encSPSId;
    TUint origSPSId;
    TUint8	maxFrameNumChanged;
    TUint origMaxFrameNum;
    TUint encMaxFrameNum;
    TUint8	maxPOCNumChanged;
    TUint origMaxPOCNum;
    TUint encMaxPOCNum;
} seq_parameter_set_s;



/*
 * Function prototypes
 */



TInt psParseLevelFromSPS( bitbuffer_s *bitbuf, TInt& aLevel );

TInt AddBytesToBuffer(bitbuffer_s *aBitBuffer, TUint aNumBytes);

int psDecodeSPS( bitbuffer_s *bitbuf, seq_parameter_set_s **spsList, 
                 TInt& aWidth, TInt& aHeight );

void psCloseParametersSets(seq_parameter_set_s **spsList,
                           pic_parameter_set_s **ppsList);
                           
void psClosePPS( pic_parameter_set_s *pps );

void psCloseSPS( seq_parameter_set_s *sps );


#ifdef VIDEOEDITORENGINE_AVC_EDITING

TInt psParseSPS( bitbuffer_s *bitbuf, seq_parameter_set_s **spsList, TUint aFrameFromEncoder, TBool *aEncodeUntilIDR, TUint *aNumSPS );

TInt psParsePPS( bitbuffer_s *bitbuf, pic_parameter_set_s **ppsList, seq_parameter_set_s **spsList, 
				 TUint aFrameFromEncoder, TUint *aNumPPS );

void psGetAspectRatio(seq_parameter_set_s *sps, int *width, int *height);

TInt GetNumTrailingBits(bitbuffer_s *aBitBuffer);

TInt ReturnUnsignedExpGolombCodeLength(TUint aValue);

void EncodeUnsignedExpGolombCode(bitbuffer_s *aBitBuffer, TUint aValue);

void ShiftBitBufferBitsRight(bitbuffer_s *aBitBuffer, TInt aDiff);

void ShiftBitBufferBitsLeft(bitbuffer_s *aBitBuffer, TInt aDiff);

void ShiftBufferLeftByOneByte(bitbuffer_s *aBitBuffer);

void ShiftBufferRightByOneByte(bitbuffer_s *aBitBuffer);

void ShiftBufferRight(bitbuffer_s *aBitBuffer, TInt aDiff, TUint aTrailingBits, TUint aOldIdLength);

void ShiftBufferLeft(bitbuffer_s *aBitBuffer, TInt aDiff, TUint aOldIdLength);


#endif  // VIDEOEDITORENGINE_AVC_EDITING

#endif
