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
#include "vld.h"
#include "parameterset.h"


#define MIN_CHROMA_QP_INDEX           -12
#define MAX_CHROMA_QP_INDEX            12
#define NUM_LEVELS                     16     /* The number of AVC levels */
#define MAX_PIC_SIZE_IN_MBS            36864
#define MAX_PIC_WIDTH_IN_MBS           543    /* Sqrt( MAX_PIC_SIZE_IN_MBS * 8 ) */
#define MAX_PIC_HEIGHT_IN_MBS          543    /* Sqrt( MAX_PIC_SIZE_IN_MBS * 8 ) */


/* These fields are defined in Annex A of the standard */
typedef struct _level_s 
{
  int8 levelNumber;
  int8 constraintSet3flag;
  int32 maxMBPS;
  int32 maxFS;
  int32 maxDPB;
  int32 maxBR;
  int32 maxCPB;
  int16 maxVmvR;
  int8 minCR;
  int8 maxMvsPer2Mb;
} level_s;

/* Parameters for all levels */
static const level_s levelArray[NUM_LEVELS] = {
  {10, 0,   1485,    99,   152064,     64,    175,  64, 2, 32},
  {11, 1,   1485,    99,   152064,    128,    350,  64, 2, 32}, /* level 1b */
  {11, 0,   3000,   396,   345600,    192,    500, 128, 2, 32},
  {12, 0,   6000,   396,   912384,    384,   1000, 128, 2, 32},
  {13, 0,  11880,   396,   912384,    768,   2000, 128, 2, 32},
  {20, 0,  11880,   396,   912384,   2000,   2000, 128, 2, 32},
  {21, 0,  19800,   792,  1824768,   4000,   4000, 256, 2, 32},
  {22, 0,  20250,  1620,  3110400,   4000,   4000, 256, 2, 32},
  {30, 0,  40500,  1620,  3110400,  10000,  10000, 256, 2, 32},
  {31, 0, 108000,  3600,  6912000,  14000,  14000, 512, 4, 16},
  {32, 0, 216000,  5120,  7864320,  20000,  20000, 512, 4, 16},
  {40, 0, 245760,  8192, 12582912,  20000,  25000, 512, 4, 16},
  {41, 0, 245760,  8192, 12582912,  50000,  62500, 512, 2, 16},
  {42, 0, 491520,  8192, 12582912,  50000,  62500, 512, 2, 16},
  {50, 0, 589824, 22080, 42393600, 135000, 135000, 512, 2, 16},
  {51, 0, 983040, 36864, 70778880, 240000, 240000, 512, 2, 16}
};

#ifdef VIDEOEDITORENGINE_AVC_EDITING

struct aspectRatio_s 
{
  int width;
  int height;
};

static const struct aspectRatio_s aspectRatioArr[13] = 
{
  {  1,  1},
  { 12, 11},
  { 10, 11},
  { 16, 11},
  { 40, 33},
  { 24, 11},
  { 20, 11},
  { 32, 11},
  { 80, 33},
  { 18, 11},
  { 15, 11},
  { 64, 33},
  {160, 99}
};
#endif  // VIDEOEDITORENGINE_AVC_EDITING

/*
 * AVC syntax functions as specified in specification
 */

/* Return fixed length code */
static int u_n(bitbuffer_s *bitbuf, int len, unsigned int *val)
{
  *val = vldGetFLC(bitbuf, len);

  if (bibGetStatus(bitbuf) < 0)
    return PS_ERROR;

  return PS_OK;
}

/* Return unsigned UVLC code */
static int ue_v(bitbuffer_s *bitbuf, unsigned int *val, unsigned int maxVal)
{
  *val = vldGetUVLC(bitbuf);

  if (bibGetStatus(bitbuf) < 0)
    return PS_ERROR;

  if (*val > maxVal)
    return PS_ERR_ILLEGAL_VALUE;

  return PS_OK;
}

/* Return long signed UVLC code */
static int se_v_long(bitbuffer_s *bitbuf, int32 *val)
{
  *val = vldGetSignedUVLClong(bitbuf);

  if (bibGetStatus(bitbuf) < 0)
    return PS_ERROR;

  return PS_OK;
}

/* Return long unsigned UVLC code */
static int ue_v_long(bitbuffer_s *bitbuf, u_int32 *val, u_int32 maxVal)
{
  *val = vldGetUVLClong(bitbuf);

  if (bibGetStatus(bitbuf) < 0)
    return PS_ERROR;

  if (*val > maxVal)
    return PS_ERR_ILLEGAL_VALUE;

  return PS_OK;
}

#ifdef VIDEOEDITORENGINE_AVC_EDITING

/* Return signed UVLC code */
static int se_v(bitbuffer_s *bitbuf, int *val, int minVal, int maxVal)
{
  *val = vldGetSignedUVLC(bitbuf);

  if (bibGetStatus(bitbuf) < 0)
    return PS_ERROR;

  if (*val < minVal || *val > maxVal)
    return PS_ERR_ILLEGAL_VALUE;

  return PS_OK;
}

#endif  // VIDEOEDITORENGINE_AVC_EDITING


/*
 * getLevel:
 *
 * Parameters:
 *      levelNumber
 *      constraintSet3flag
 *
 * Function:
 *      Return parameters for level based on level number.
 *
 *  Return:
 *      Pointer to level or 0 if level does not exist
 */
static const level_s *getLevel(int levelNumber, int constraintSet3flag)
{
  int i;

  for (i = 0; i < NUM_LEVELS; i++) {
    if (levelArray[i].levelNumber == levelNumber &&
        levelArray[i].constraintSet3flag == constraintSet3flag)
      return &levelArray[i];
  }

  PRINT((_L("Unknown level: %i.\n"), levelNumber));            
  return 0;
}

/*
 *
 * getHrdParameters:
 *
 * Parameters:
 *      bitbuf                The bitbuffer object
 *      hrd                   the pointer for returning HRD parameters
 *
 * Function:
 *      decode the HRD Parameters
 *
 * Returns:
 *      PS_OK:                Hrd parameters decoded succesfully
 *      <0:                   Fail
 */
static int getHrdParameters(bitbuffer_s *bitbuf, hrd_parameters_s *hrd)
{
  unsigned int i;
  int retCode;

  if ((retCode = ue_v(bitbuf, &hrd->cpb_cnt_minus1, 31)) < 0)
    return retCode;

  if ((retCode = u_n(bitbuf, 4, &hrd->bit_rate_scale)) < 0)
    return retCode;

  if ((retCode = u_n(bitbuf, 4, &hrd->cpb_size_scale)) < 0)
    return retCode;

  for (i = 0; i <= hrd->cpb_cnt_minus1; i++) {
    /* bit_rate_value_minus1 must be in range of 0 to 2^32-2 */
    if ((retCode = ue_v_long(bitbuf, &hrd->bit_rate_value_minus1[i], (u_int32)4294967294U)) < 0)
      return retCode;

    /* cpb_size_value_minus1 must be in range of 0 to 2^32-2 */
    if ((retCode = ue_v_long(bitbuf, &hrd->cpb_size_value_minus1[i], (u_int32)4294967294U)) < 0)
      return retCode;

    if ((retCode = u_n(bitbuf, 1, &hrd->cbr_flag[i])) < 0)
      return retCode;
  }

  if ((retCode = u_n(bitbuf, 5, &hrd->initial_cpb_removal_delay_length_minus1)) < 0)
    return retCode;

  if ((retCode = u_n(bitbuf, 5, &hrd->cpb_removal_delay_length_minus1)) < 0)
    return retCode;

  if ((retCode = u_n(bitbuf, 5, &hrd->dpb_output_delay_length_minus1)) < 0)
    return retCode;

  if ((retCode = u_n(bitbuf, 5, &hrd->time_offset_length)) < 0)
    return retCode;

  return PS_OK;
}



/*
 *
 * getVUI:
 *
 * Parameters:
 *      bitbuf                The bitbuffer object
 *      vui                   the pointer for returning VUI parameters
 *
 * Function:
 *      decode the VUI Parameters
 *
 * Returns:
 *      PS_OK:                VUI parameters decoded succesfully
 *      <0:                   Fail
 */
static int getVUI(bitbuffer_s *bitbuf, vui_parameters_s *vui)
{
  unsigned tempWordHi, tempWordLo;
  int retCode;

  if ((retCode = u_n(bitbuf, 1, &vui->aspect_ratio_info_present_flag)) < 0)
    return retCode;

  if (vui->aspect_ratio_info_present_flag) {
    if ((retCode = u_n(bitbuf, 8, &vui->aspect_ratio_idc)) < 0)
      return retCode;
    if (vui->aspect_ratio_idc == PS_EXTENDED_SAR) {
      if ((retCode = u_n(bitbuf, 16, &vui->sar_width)) < 0)
        return retCode;
      if ((retCode = u_n(bitbuf, 16, &vui->sar_height)) < 0)
        return retCode;
    }
  }

  if ((retCode = u_n(bitbuf, 1, &vui->overscan_info_present_flag)) < 0)
    return retCode;

  if (vui->overscan_info_present_flag) {
    if ((retCode = u_n(bitbuf, 1, &vui->overscan_appropriate_flag)) < 0)
      return retCode;
  }

  if ((retCode = u_n(bitbuf, 1, &vui->video_signal_type_present_flag)) < 0)
    return retCode;

  if (vui->video_signal_type_present_flag) {
    if ((retCode = u_n(bitbuf, 3, &vui->video_format)) < 0)
      return retCode;
    if ((retCode = u_n(bitbuf, 1, &vui->video_full_range_flag)) < 0)
      return retCode;
    if ((retCode = u_n(bitbuf, 1, &vui->colour_description_present_flag)) < 0)
      return retCode;
    if (vui->colour_description_present_flag) {
      if ((retCode = u_n(bitbuf, 8, &vui->colour_primaries)) < 0)
        return retCode;
      if ((retCode = u_n(bitbuf, 8, &vui->transfer_characteristics)) < 0)
        return retCode;
      if ((retCode = u_n(bitbuf, 8, &vui->matrix_coefficients)) < 0)
        return retCode;
    }
  }

  if ((retCode = u_n(bitbuf, 1, &vui->chroma_loc_info_present_flag)) < 0)
    return retCode;

  if (vui->chroma_loc_info_present_flag) {
    if ((retCode = ue_v(bitbuf, &vui->chroma_sample_loc_type_top_field, 5)) < 0)
      return retCode;
    if ((retCode = ue_v(bitbuf, &vui->chroma_sample_loc_type_bottom_field, 5)) < 0)
      return retCode;
  }

  if ((retCode = u_n(bitbuf, 1, &vui->timing_info_present_flag)) < 0)
    return retCode;

  if (vui->timing_info_present_flag) {
    if ((retCode = u_n(bitbuf, 16, &tempWordHi)) < 0)
      return retCode;
    if ((retCode = u_n(bitbuf, 16, &tempWordLo)) < 0)
      return retCode;
    vui->num_units_in_tick = (((u_int32)tempWordHi) << 16) | ((u_int32)tempWordLo);

    if ((retCode = u_n(bitbuf, 16, &tempWordHi)) < 0)
      return retCode;
    if ((retCode = u_n(bitbuf, 16, &tempWordLo)) < 0)
      return retCode;
    vui->time_scale = (((u_int32)tempWordHi) << 16) | ((u_int32)tempWordLo);

    if ((retCode = u_n(bitbuf, 1, &vui->fixed_frame_rate_flag)) < 0)
      return retCode;
  }

  if ((retCode = u_n(bitbuf, 1, &vui->nal_hrd_parameters_present_flag)) < 0)
    return retCode;

  if (vui->nal_hrd_parameters_present_flag) {
    if ((retCode = getHrdParameters(bitbuf, &vui->nal_hrd_parameters)) < 0)
      return retCode;
  }

  if ((retCode = u_n(bitbuf, 1, &vui->vcl_hrd_parameters_present_flag)) < 0)
    return retCode;

  if (vui->vcl_hrd_parameters_present_flag) {
    if ((retCode = getHrdParameters(bitbuf, &vui->vcl_hrd_parameters)) < 0)
      return retCode;
  }

  if (vui->nal_hrd_parameters_present_flag || vui->vcl_hrd_parameters_present_flag) {
    if ((retCode = u_n(bitbuf, 1, &vui->low_delay_hrd_flag)) < 0)
      return retCode;
  }

  if ((retCode = u_n(bitbuf, 1, &vui->pic_struct_present_flag)) < 0)
    return retCode;

  if ((retCode = u_n(bitbuf, 1, &vui->bitstream_restriction_flag)) < 0)
    return retCode;

  if (vui->bitstream_restriction_flag) {
    if ((retCode = u_n(bitbuf, 1, &vui->motion_vectors_over_pic_boundaries_flag)) < 0)
      return retCode;
    if ((retCode = ue_v(bitbuf, &vui->max_bytes_per_pic_denom, 16)) < 0)
      return retCode;
    if ((retCode = ue_v(bitbuf, &vui->max_bits_per_mb_denom, 16)) < 0)
      return retCode;
    if ((retCode = ue_v(bitbuf, &vui->log2_max_mv_length_horizontal, 16)) < 0)
      return retCode;
    if ((retCode = ue_v(bitbuf, &vui->log2_max_mv_length_vertical, 16)) < 0)
      return retCode;
    if ((retCode = ue_v(bitbuf, &vui->num_reorder_frames, 16)) < 0)
      return retCode;
    if ((retCode = ue_v(bitbuf, &vui->max_dec_frame_buffering, 16)) < 0)
      return retCode;
  }

  return PS_OK;
}


/*
 *
 * setVUIdefaults:
 *
 * Parameters:
 *      vui                   Pointer to VUI parameters
 *
 * Function:
 *      Set VUI parameters to their default values when default value is non-zero.
 *
 * Returns:
 *      -
 */
static void setVUIdefaults(seq_parameter_set_s *sps)
{
  vui_parameters_s *vui;
  const level_s *level;
  int MaxDpbSize;

  vui = &sps->vui_parameters;

  vui->video_format                            = 5;
  vui->colour_primaries                        = 2;
  vui->transfer_characteristics                = 2;
  vui->matrix_coefficients                     = 2;
  vui->motion_vectors_over_pic_boundaries_flag = 1;
  vui->max_bytes_per_pic_denom                 = 2;
  vui->max_bits_per_mb_denom                   = 1;
  vui->log2_max_mv_length_horizontal           = 16;
  vui->log2_max_mv_length_vertical             = 16;

  level = getLevel(sps->level_idc, sps->constraint_set3_flag);
  MaxDpbSize = level->maxDPB /
    ((sps->pic_width_in_mbs_minus1+1) * (sps->pic_height_in_map_units_minus1+1) * 384);
  MaxDpbSize = clip(1, 16, MaxDpbSize);

  vui->max_dec_frame_buffering = MaxDpbSize;
  vui->num_reorder_frames      = vui->max_dec_frame_buffering;
}

/*
 *
 * psDecodeSPS:
 *      
 * Parameters:
 *      bitbuf                Bitbuffer object
 *      spsList               The list for SPS's, the newly decoded SPS will be stored into the list
 *
 * Function:
 *      Decode the SPS, and store it into the SPS list
 *
 * Returns:
 *      PS_OK:                SPS decoded succesfully
 *      <0:                   Fail
 */
int psDecodeSPS( bitbuffer_s *bitbuf, seq_parameter_set_s **spsList, 
                 TInt& aWidth, TInt& aHeight )
{
  seq_parameter_set_s *sps;
  unsigned int i;
  int retCode;

  unsigned  profile_idc;                                      // u(8)
  Boolean   constraint_set0_flag;                             // u(1)
  Boolean   constraint_set1_flag;                             // u(1)
  Boolean   constraint_set2_flag;                             // u(1)
  Boolean   constraint_set3_flag;                             // u(1)
  Boolean   reserved_zero_4bits;                              // u(4)
  unsigned  level_idc;                                        // u(8)
  unsigned  seq_parameter_set_id;                             // ue(v)


  /*
   * Parse sequence parameter set syntax until sps id
   */

  if ((retCode = u_n(bitbuf, 8, &profile_idc)) < 0)
    return retCode;

  /* If constraint_set0_flag == 1, stream is Baseline Profile compliant */
  if ((retCode = u_n(bitbuf, 1, &constraint_set0_flag)) < 0)
    return retCode;

  /* If constraint_set1_flag == 1, stream is Main Profile compliant */
  if ((retCode = u_n(bitbuf, 1, &constraint_set1_flag)) < 0)
    return retCode;

  /* If constraint_set2_flag == 1, stream is Extended Profile compliant */
  if ((retCode = u_n(bitbuf, 1, &constraint_set2_flag)) < 0)
    return retCode;

  if ((retCode = u_n(bitbuf, 1, &constraint_set3_flag)) < 0)
    return retCode;

  /* If CABAC is not defined we support only baseline compliant streams */
#ifndef ENABLE_CABAC
  if (profile_idc != PS_BASELINE_PROFILE_IDC && constraint_set0_flag == 0)
    return PS_ERR_UNSUPPORTED_PROFILE;
#else
  if (profile_idc != PS_BASELINE_PROFILE_IDC && constraint_set0_flag == 0 &&
      profile_idc != PS_MAIN_PROFILE_IDC && constraint_set1_flag == 0)
    return PS_ERR_UNSUPPORTED_PROFILE;
#endif

  /* We don't care what is in these bits */
  if ((retCode = u_n(bitbuf, 4, &reserved_zero_4bits)) < 0)
    return retCode;

  /* Fetch level */
  if ((retCode = u_n(bitbuf, 8, &level_idc)) < 0)
    return retCode;

  /* Find level in the list of legal levels */
  for (i = 0; i < NUM_LEVELS; i++) {
    if ((int)level_idc == levelArray[i].levelNumber)
      break;
  }

  /* If level was not found in the list, return with error */
  if (i == NUM_LEVELS)
    return PS_ERR_ILLEGAL_VALUE;

  /* Get sequence parameter set id */
  if ((retCode = ue_v(bitbuf, &seq_parameter_set_id, PS_MAX_NUM_OF_SPS-1)) < 0)
    return retCode;


  /*
   * Allocate memory for SPS
   */

  /* Pointer to sequence parameter set structure */
  sps = spsList[seq_parameter_set_id];

  /* allocate mem for SPS, if it has not been allocated already */
  if (!sps) {
    sps = (seq_parameter_set_s *) User::Alloc(sizeof(seq_parameter_set_s));    
    if (sps == 0) {
      return PS_ERR_MEM_ALLOC;
    }
    memset( sps, 0, sizeof(seq_parameter_set_s));
    spsList[seq_parameter_set_id] = sps;
  }


  /* Copy temporary variables to sequence parameter set structure */
  sps->profile_idc          = profile_idc;
  sps->constraint_set0_flag = constraint_set0_flag;
  sps->constraint_set1_flag = constraint_set1_flag;
  sps->constraint_set2_flag = constraint_set2_flag;
  sps->constraint_set3_flag = constraint_set3_flag;
  sps->reserved_zero_4bits  = reserved_zero_4bits;
  sps->level_idc            = level_idc;
  sps->seq_parameter_set_id = seq_parameter_set_id;


  /*
   * Parse rest of the sequence parameter set syntax
   */

  /* This defines how many bits there are in frame_num syntax element */
  if ((retCode = ue_v(bitbuf, &sps->log2_max_frame_num_minus4, 12)) < 0)
    return retCode;

  /* Fetch POC type */
  if ((retCode = ue_v(bitbuf, &sps->pic_order_cnt_type, 2)) < 0)
    return retCode;

  if (sps->pic_order_cnt_type == 0) {
    if ((retCode = ue_v(bitbuf, &sps->log2_max_pic_order_cnt_lsb_minus4, 12)) < 0)
      return retCode;
  }
  else if (sps->pic_order_cnt_type == 1) {
    if ((retCode = u_n(bitbuf, 1, &sps->delta_pic_order_always_zero_flag)) < 0)
      return retCode;

    if ((retCode = se_v_long(bitbuf, &sps->offset_for_non_ref_pic)) < 0)
      return retCode;

    if ((retCode = se_v_long(bitbuf, &sps->offset_for_top_to_bottom_field)) < 0)
      return retCode;

    if ((retCode = ue_v(bitbuf, &sps->num_ref_frames_in_pic_order_cnt_cycle, 255)) < 0)
      return retCode;

    for (i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++) {
      if ((retCode = se_v_long(bitbuf, &sps->offset_for_ref_frame[i])) < 0)
        return retCode;
    }
  }

  if ((retCode = ue_v(bitbuf, &sps->num_ref_frames, 16)) < 0)
    return retCode;

  if ((retCode = u_n(bitbuf, 1, &sps->gaps_in_frame_num_value_allowed_flag)) < 0)
    return retCode;

  if ((retCode = ue_v(bitbuf, &sps->pic_width_in_mbs_minus1, MAX_PIC_WIDTH_IN_MBS-1)) < 0)
    return retCode;
  
  aWidth = (sps->pic_width_in_mbs_minus1 + 1) * 16;

  if ((retCode = ue_v(bitbuf, &sps->pic_height_in_map_units_minus1, MAX_PIC_WIDTH_IN_MBS-1)) < 0)
    return retCode;
  
  aHeight = (sps->pic_height_in_map_units_minus1 + 1) * 16;

  if ((retCode = u_n(bitbuf, 1, &sps->frame_mbs_only_flag)) < 0)
    return retCode;

  if (!sps->frame_mbs_only_flag) {
    // u_n(bitbuf, 1, &sps->mb_adaptive_frame_field_flag);
    return PS_ERR_UNSUPPORTED_FEATURE;
  }

  if ((retCode = u_n(bitbuf, 1, &sps->direct_8x8_inference_flag)) < 0)
    return retCode;

  if ((retCode = u_n(bitbuf, 1, &sps->frame_cropping_flag)) < 0)
    return retCode;

  /* Fetch cropping window */
  if (sps->frame_cropping_flag) {
    if ((retCode = ue_v(bitbuf, &sps->frame_crop_left_offset, 8*(sps->pic_width_in_mbs_minus1+1)-1)) < 0)
      return retCode;

    if ((retCode = ue_v(bitbuf, &sps->frame_crop_right_offset, 8*(sps->pic_width_in_mbs_minus1+1)-sps->frame_crop_left_offset-1)) < 0)
      return retCode;

    if ((retCode = ue_v(bitbuf, &sps->frame_crop_top_offset, 8*(sps->pic_height_in_map_units_minus1+1)-1)) < 0)
      return retCode;

    if ((retCode = ue_v(bitbuf, &sps->frame_crop_bottom_offset, 8*(sps->pic_height_in_map_units_minus1+1)-sps->frame_crop_top_offset-1)) < 0)
      return retCode;
    
    TInt cropUnitX = 2;
    TInt cropUnitY = 2 * ( 2 - sps->frame_mbs_only_flag );
    
    TInt leftBorder = cropUnitX * sps->frame_crop_left_offset;
    TInt rightBorder = aWidth - ( cropUnitX * sps->frame_crop_right_offset );
    
    aWidth = rightBorder - leftBorder;
    
    TInt topBorder = cropUnitY * sps->frame_crop_top_offset;
    TInt bottomBorder = ( 16 * (sps->pic_height_in_map_units_minus1 + 1) ) -
                        cropUnitY * sps->frame_crop_bottom_offset;
                        
    aHeight = bottomBorder - topBorder;
    
  }

  if ((retCode = u_n(bitbuf, 1, &sps->vui_parameters_present_flag)) < 0)
    return retCode;

  setVUIdefaults(sps);

  if (sps->vui_parameters_present_flag) {
    if ((retCode = getVUI(bitbuf, &sps->vui_parameters)) < 0)
      return retCode;
  }

  if (bibSkipTrailingBits(bitbuf) < 0)
    return PS_ERROR;

  return PS_OK;
}

/*
 *
 * psCloseParametersSets:
 *
 * Parameters:
 *      spsList               The sequence parameter set list
 *      ppsList               The picture parameter set list
 *
 * Fucntion:
 *      Free all parameter sets
 *
 * Returns:
 *      -
 */
void psCloseParametersSets(seq_parameter_set_s **spsList,
                           pic_parameter_set_s **ppsList)
{
  int i;

  for (i = 0; i < PS_MAX_NUM_OF_SPS; i++) {
    psCloseSPS(spsList[i]);
    spsList[i] = 0;
  }

  for (i = 0; i < PS_MAX_NUM_OF_PPS; i++) {
    psClosePPS(ppsList[i]);
    ppsList[i] = 0;
  }
}

/*
 *
 * psClosePPS:
 *
 * Parameters:
 *      pps                   the picture parameter set to be freed
 *
 * Function:
 *      free the picture parameter set
 *
 * Returns:
 *      -
 */
void psClosePPS( pic_parameter_set_s *pps )
{
  if (pps == 0)
    return;
  
  	// [KW]: Added
  	if (pps->codedPPSBuffer)
	  	User::Free(pps->codedPPSBuffer);


  if (pps->slice_group_id)
    User::Free(pps->slice_group_id);
//    nccFree(pps->slice_group_id);

//  nccFree(pps);
	User::Free(pps);
}


/*
 *
 * psCloseSPS:
 *
 * Parameters:
 *      sps                   the sequence parameter set to be freed
 *
 * Fucntion:
 *      free the sequence parameter set
 *
 * Returns:
 *      -
 */
void psCloseSPS( seq_parameter_set_s *sps )
{
  if (sps == 0)
    return;
  	
  	// [KW]: Added
  	if (sps->codedSPSBuffer)
	  	User::Free(sps->codedSPSBuffer);

//  nccFree(sps);
  User::Free(sps);
}


// psParseLevelFromSPS
// Returns the baseline profile level from SPS
TInt psParseLevelFromSPS( bitbuffer_s *bitbuf, TInt& aLevel )
{
  	TInt retCode;
  	TUint profile_idc;                                      // u(8)
  	Boolean constraint_set0_flag;                          // u(1)
  	Boolean constraint_set1_flag;                          // u(1)
  	Boolean constraint_set2_flag;                          // u(1)
  	Boolean constraint_set3_flag;                          // u(1)
  	Boolean reserved_zero_4bits;                           // u(4)
  	TUint level_idc;                                        // u(8)      
  
  	// Parse sequence parameter set syntax until sps id
  	if ((retCode = u_n(bitbuf, 8, &profile_idc)) < 0)
    	return retCode;

  	// If constraint_set0_flag == 1, stream is Baseline Profile compliant 
  	if ((retCode = u_n(bitbuf, 1, &constraint_set0_flag)) < 0)
    	return retCode;

  	// If constraint_set1_flag == 1, stream is Main Profile compliant 
  	if ((retCode = u_n(bitbuf, 1, &constraint_set1_flag)) < 0)
    	return retCode;

  	// If constraint_set2_flag == 1, stream is Extended Profile compliant 
  	if ((retCode = u_n(bitbuf, 1, &constraint_set2_flag)) < 0)
    	return retCode;

  	if ((retCode = u_n(bitbuf, 1, &constraint_set3_flag)) < 0)
    	return retCode;

  	// If CABAC is not defined we support only baseline compliant streams 
	if (profile_idc != PS_BASELINE_PROFILE_IDC && constraint_set0_flag == 0)
   		return PS_ERR_UNSUPPORTED_PROFILE;

  	// We don't care what is in these bits 
  	if ((retCode = u_n(bitbuf, 4, &reserved_zero_4bits)) < 0)
    	return retCode;

  	// Fetch level 
  	if ((retCode = u_n(bitbuf, 8, &level_idc)) < 0)
    	return retCode;
  
  	aLevel = level_idc;
  
  	if ( level_idc == 11 && constraint_set3_flag == 1 )
    	aLevel = 101;  // level 1b
  
  	return KErrNone;
}


// AddBytesToBuffer
// Adds aNumBytes bytes to bit buffer aBitBuffer, reallocates the bit buffer data if necessary.
TInt AddBytesToBuffer(bitbuffer_s *aBitBuffer, TUint aNumBytes)
{
	TInt i;
	
	// Reallocate the bitbuffer data
	aBitBuffer->data = (TUint8 *) User::ReAlloc(aBitBuffer->data, (aBitBuffer->dataLen+aNumBytes));

	if (aBitBuffer->data == 0)
        return KErrNoMemory;

	// Set the new bytes as zeros
	for (i=aBitBuffer->dataLen; i<aBitBuffer->dataLen+aNumBytes; i++)
	{
		aBitBuffer->data[i] = 0;
	}

	return KErrNone;
}


#ifdef VIDEOEDITORENGINE_AVC_EDITING


/*
 *
 * psGetAspectRatio:
 *      
 * Parameters:
 *      sps                   Sequence parameter set
 *      width                 Horizontal size of the sample aspect ratio
 *      height                Vertical size of the sample aspect ratio
 *
 * Function:
 *      Return sample aspect ratio in width and height
 *
 * Returns:
 *      -
 */
void psGetAspectRatio(seq_parameter_set_s *sps, int *width, int *height)
{
  vui_parameters_s *vui;

  vui = &sps->vui_parameters;

  *width  = 0;
  *height = 0;

  if (sps->vui_parameters_present_flag &&
      vui->aspect_ratio_info_present_flag &&
      vui->aspect_ratio_idc != 0 &&
      (vui->aspect_ratio_idc <= 13 || vui->aspect_ratio_idc == 255))
  {
    if (vui->aspect_ratio_idc == 255) {
      /* Extended_SAR */
      if (vui->sar_width != 0 && vui->sar_height != 0) {
        *width  = vui->sar_width;
        *height = vui->sar_height;
      }
    }
    else {
      *width  = aspectRatioArr[vui->aspect_ratio_idc-1].width;
      *height = aspectRatioArr[vui->aspect_ratio_idc-1].height;
    }
  }

}


// CompareSPSSets
// Compares two SPS input sets to see if we can use one for both clips, if exact match is not required
// then some parameters maybe be different in the two sets. 
TInt CompareSPSSets( seq_parameter_set_s *aSPSSet1, seq_parameter_set_s *aSPSSet2, TBool aExactMatch )
{
	TUint i;

	// Different maxFrameNum & maxPOCLsb can be handled by modifying the slice header, thus do not return EFalse
	if ( aExactMatch )
	{
		// If exact match is required, return false for different max frame number value
		if ( aSPSSet1->log2_max_frame_num_minus4 != aSPSSet2->log2_max_frame_num_minus4 )
			return EFalse;	
	}
	
	if ( aSPSSet1->pic_order_cnt_type != aSPSSet2->pic_order_cnt_type )
	{
		return EFalse;
	}
	else
	{
	  	if (aSPSSet1->pic_order_cnt_type == 0) 
  		{
			// Different maxFrameNum & maxPOCLsb can be handled by modifying the slice header, thus do not return EFalse
			if ( aExactMatch )
			{
				// If exact match is required, return false for different max POCLSB number value
				if ( aSPSSet1->log2_max_pic_order_cnt_lsb_minus4 != aSPSSet2->log2_max_pic_order_cnt_lsb_minus4 )
					return EFalse;	
			}
	
  		}
	  	else if (aSPSSet1->pic_order_cnt_type == 1) 
  		{
			if ( aSPSSet1->delta_pic_order_always_zero_flag != aSPSSet2->delta_pic_order_always_zero_flag ||
				 aSPSSet1->offset_for_non_ref_pic != aSPSSet2->offset_for_non_ref_pic ||
				 aSPSSet1->num_ref_frames_in_pic_order_cnt_cycle != aSPSSet2->num_ref_frames_in_pic_order_cnt_cycle )
			{
				return EFalse;
			}

	    	for (i = 0; i < aSPSSet1->num_ref_frames_in_pic_order_cnt_cycle; i++) 
    		{
				if ( aSPSSet1->offset_for_ref_frame[i] != aSPSSet2->offset_for_ref_frame[i] )
				{
					return EFalse;
				}
    		}
  		}
	}

	if ( aSPSSet1->num_ref_frames != aSPSSet2->num_ref_frames )
	{
		return EFalse;
	}
	
	// Direct 8x8 inference flag is not used in baseline, ignore

	return ETrue;
}


// IsSPSSupported
// Checks if the input SPS contains supported values. Returns KErrNotSupported if 
// unsupported parameters are found.
TInt IsSPSSupported( seq_parameter_set_s *aSPS )
{

	// Only Baseline profile supported at the moment
	if ( aSPS->profile_idc != PS_BASELINE_PROFILE_IDC )
	{
		return KErrNotSupported;
	}
	
	// Check if maximum supported level is exceeded
	if ( aSPS->level_idc > PS_MAX_SUPPORTED_LEVEL )
	{
		return KErrNotSupported;
	}
	
	// For now more than one reference frames are not supported
	if ( aSPS->num_ref_frames > 1 )
	{
		return KErrNotSupported;
	}
	
	// Coded fields are not supported
	if ( !aSPS->frame_mbs_only_flag )
	{
		return KErrNotSupported;
	}
	
	if ( aSPS->vui_parameters_present_flag )
	{
		if ( aSPS->vui_parameters.num_reorder_frames != 0 && aSPS->pic_order_cnt_type != 2)
		{
			// Since we can't be sure how many input frames we have to buffer before getting 
			// an output picture, return KErrNotSupported
			return KErrNotSupported;
		}
	}
	else
	{
		if ( aSPS->pic_order_cnt_type != 2)
		{
			// Since we can't be sure how many input frames we have to buffer before getting 
			// an output picture, return KErrNotSupported
			return KErrNotSupported;
		}
	}
	
	return KErrNone;
}


// IsPPSSupported
// Checks if the input PPS contains supported values. Returns KErrNotSupported if 
// unsupported parameters are found.
TInt IsPPSSupported( pic_parameter_set_s *aPPS )
{

	// For baseline, both prediction values shall be zero
	if( aPPS->weighted_pred_flag != 0 || aPPS->weighted_bipred_idc != 0)
	{
		return KErrNotSupported;
	}
	
	// For baseline, entropy coding mode shall be zero
	if ( aPPS->entropy_coding_mode_flag != 0 )
	{
		return KErrNotSupported;
	}
	
	if ( aPPS->num_slice_groups_minus1 > PS_BASELINE_MAX_SLICE_GROUPS )
	{
		return KErrNotSupported;
	}

	return KErrNone;
}


// psParseSPS
// Parses the input SPS set. Modifies the SPS id if a conflicting id is found 
// and stores the modified data to codedSPSBuffer for later use.
TInt psParseSPS( bitbuffer_s *bitbuf, seq_parameter_set_s **spsList, TUint aFrameFromEncoder, TBool *aEncodeUntilIDR, TUint *aNumSPS )
{
  	seq_parameter_set_s *sps;
  	TUint i;
  	TInt retCode;
  	TUint bitPosit = 0;
  	TUint bytePosit = 0;
  	TUint profile_idc;                                      // u(8)
  	Boolean constraint_set0_flag;                          // u(1)
  	Boolean constraint_set1_flag;                          // u(1)
  	Boolean constraint_set2_flag;                          // u(1)
  	Boolean constraint_set3_flag;                          // u(1)
  	Boolean reserved_zero_4bits;                           // u(4)
  	TUint level_idc;                                        // u(8)
  	TUint seq_parameter_set_id;                             // ue(v)
  	TUint newSPSId = 0;
  	TUint possibleIdConflict = 0;
  	TUint useOneSPS = 0;


  	if (!aFrameFromEncoder)
  	{	
	  	// Reset the encode until IDR flag if this SPS is not from the encoder.
  		*aEncodeUntilIDR = EFalse;
  	}

  	// Parse sequence parameter set syntax until sps id
  	if ((retCode = u_n(bitbuf, 8, &profile_idc)) < 0)
    	return retCode;
	
  	// If constraint_set0_flag == 1, stream is Baseline Profile compliant 
  	if ((retCode = u_n(bitbuf, 1, &constraint_set0_flag)) < 0)
    	return retCode;

  	// If constraint_set1_flag == 1, stream is Main Profile compliant 
  	if ((retCode = u_n(bitbuf, 1, &constraint_set1_flag)) < 0)
    	return retCode;

  	// If constraint_set2_flag == 1, stream is Extended Profile compliant 
  	if ((retCode = u_n(bitbuf, 1, &constraint_set2_flag)) < 0)
    	return retCode;

  	if ((retCode = u_n(bitbuf, 1, &constraint_set3_flag)) < 0)
    	return retCode;

  	// If CABAC is not defined we support only baseline compliant streams 
	if (profile_idc != PS_BASELINE_PROFILE_IDC && constraint_set0_flag == 0)
   		return PS_ERR_UNSUPPORTED_PROFILE;

  	// We don't care what is in these bits 
  	if ((retCode = u_n(bitbuf, 4, &reserved_zero_4bits)) < 0)
    	return retCode;

  	// Fetch level 
  	if ((retCode = u_n(bitbuf, 8, &level_idc)) < 0)
    	return retCode;

  	// Find level in the list of legal levels 
  	for (i = 0; i < NUM_LEVELS; i++) 
  	{
    	if ((int)level_idc == levelArray[i].levelNumber)
      		break;
  	}

  	// If level was not found in the list, return with error 
  	if (i == NUM_LEVELS)
    	return PS_ERR_ILLEGAL_VALUE;

  	// Get sequence parameter set id 
  	if ((retCode = ue_v(bitbuf, &seq_parameter_set_id, PS_MAX_NUM_OF_SPS-1)) < 0)
    	return retCode;

  	// Pointer to sequence parameter set structure 
  	sps = spsList[seq_parameter_set_id];

  	// Allocate memory for SPS, if it has not been allocated already 
  	if (!sps) 
  	{
    	sps = (seq_parameter_set_s *) User::Alloc(sizeof(seq_parameter_set_s));
    	
    	if (sps == 0) 
    	{
			PRINT((_L("Error while allocating memory for SPS.\n")));
      			return PS_ERR_MEM_ALLOC;
    	}
    	
    	memset( sps, 0, sizeof(seq_parameter_set_s));
    	spsList[seq_parameter_set_id] = sps;

  		sps->seq_parameter_set_id = seq_parameter_set_id;
  		(*aNumSPS)++;
  	}
  	else
  	{
  		// There might be a conflicting Id with an existing SPS set
  		// Give the new SPS set the next free SPS Id
  		possibleIdConflict = 1;
  		newSPSId = 0;
  		useOneSPS = 1;
  		
  		// Search for the first free SPS id
  		while (spsList[newSPSId])
  		{
  			newSPSId++;
  		}
  	
  		// And allocate memory for the SPS
    	sps = (seq_parameter_set_s *) User::Alloc(sizeof(seq_parameter_set_s));
    	
    	if (sps == 0) 
    	{
      		PRINT((_L("Error while allocating memory for SPS.\n")));
      			return PS_ERR_MEM_ALLOC;
    	}
    
    	memset( sps, 0, sizeof(seq_parameter_set_s));
    
    	sps->seq_parameter_set_id = newSPSId;

  		// Store the position of the bit buffer
  		bitPosit = bitbuf->bitpos;
  		bytePosit = bitbuf->bytePos;
  	}


  	// Copy temporary variables to sequence parameter set structure 
  	sps->profile_idc          = profile_idc;
  	sps->constraint_set0_flag = constraint_set0_flag;
  	sps->constraint_set1_flag = constraint_set1_flag;
  	sps->constraint_set2_flag = constraint_set2_flag;
  	sps->constraint_set3_flag = constraint_set3_flag;
  	sps->reserved_zero_4bits  = reserved_zero_4bits;
  	sps->level_idc            = level_idc;

	// Initialize
	sps->maxFrameNumChanged = 0;
	sps->maxPOCNumChanged = 0;
	
  	// This defines how many bits there are in frame_num syntax element 
  	if ((retCode = ue_v(bitbuf, &sps->log2_max_frame_num_minus4, 12)) < 0)
    	return retCode;

  	// Fetch POC type 
  	if ((retCode = ue_v(bitbuf, &sps->pic_order_cnt_type, 2)) < 0)
    	return retCode;

  	if (sps->pic_order_cnt_type == 0) 
  	{
    	if ((retCode = ue_v(bitbuf, &sps->log2_max_pic_order_cnt_lsb_minus4, 12)) < 0)
      		return retCode;
  	}
  	else if (sps->pic_order_cnt_type == 1) 
  	{
    	if ((retCode = u_n(bitbuf, 1, &sps->delta_pic_order_always_zero_flag)) < 0)
      		return retCode;

    	if ((retCode = se_v_long(bitbuf, &sps->offset_for_non_ref_pic)) < 0)
      		return retCode;

    	if ((retCode = se_v_long(bitbuf, &sps->offset_for_top_to_bottom_field)) < 0)
      		return retCode;

    	if ((retCode = ue_v(bitbuf, &sps->num_ref_frames_in_pic_order_cnt_cycle, 255)) < 0)
      		return retCode;

    	for (i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++) 
    	{
      		if ((retCode = se_v_long(bitbuf, &sps->offset_for_ref_frame[i])) < 0)
        		return retCode;
    	}
  	}

  	if ((retCode = ue_v(bitbuf, &sps->num_ref_frames, 16)) < 0)
    	return retCode;

  	if ((retCode = u_n(bitbuf, 1, &sps->gaps_in_frame_num_value_allowed_flag)) < 0)
    	return retCode;

  	if ((retCode = ue_v(bitbuf, &sps->pic_width_in_mbs_minus1, MAX_PIC_WIDTH_IN_MBS-1)) < 0)
    	return retCode;

  	if ((retCode = ue_v(bitbuf, &sps->pic_height_in_map_units_minus1, MAX_PIC_WIDTH_IN_MBS-1)) < 0)
    	return retCode;

  	if ((retCode = u_n(bitbuf, 1, &sps->frame_mbs_only_flag)) < 0)
    	return retCode;

  	if (!sps->frame_mbs_only_flag) 
  	{
    	return PS_ERR_UNSUPPORTED_FEATURE;
  	}

  	if ((retCode = u_n(bitbuf, 1, &sps->direct_8x8_inference_flag)) < 0)
    	return retCode;

  	if ((retCode = u_n(bitbuf, 1, &sps->frame_cropping_flag)) < 0)
    	return retCode;

  	// Fetch cropping window 
  	if (sps->frame_cropping_flag) 
  	{
    	if ((retCode = ue_v(bitbuf, &sps->frame_crop_left_offset, 8*(sps->pic_width_in_mbs_minus1+1)-1)) < 0)
      		return retCode;

    	if ((retCode = ue_v(bitbuf, &sps->frame_crop_right_offset, 8*(sps->pic_width_in_mbs_minus1+1)-sps->frame_crop_left_offset-1)) < 0)
      		return retCode;

    	if ((retCode = ue_v(bitbuf, &sps->frame_crop_top_offset, 8*(sps->pic_height_in_map_units_minus1+1)-1)) < 0)
      		return retCode;

    	if ((retCode = ue_v(bitbuf, &sps->frame_crop_bottom_offset, 8*(sps->pic_height_in_map_units_minus1+1)-sps->frame_crop_top_offset-1)) < 0)
      		return retCode;
  	}

  	if ((retCode = u_n(bitbuf, 1, &sps->vui_parameters_present_flag)) < 0)
    	return retCode;

  	setVUIdefaults(sps);

  	if (sps->vui_parameters_present_flag) 
  	{
    	if ((retCode = getVUI(bitbuf, &sps->vui_parameters)) < 0)
      		return retCode;
  	}

  	if (bibSkipTrailingBits(bitbuf) < 0)
    	return PS_ERROR;
  
  	// Store the size of the SPS set
  	sps->SPSlength = bitbuf->bytePos;
  
	syncBitBufferBitpos(bitbuf);
	
  	// If we had a possible conflict, compare the SPS sets with the same id to see if only one SPS set can be used.
  	if (possibleIdConflict)
  	{
  		// Check if one SPS can be used instead of two separate ones
	  	useOneSPS = CompareSPSSets(spsList[seq_parameter_set_id],sps,EFalse);
	  	
	  	if (!useOneSPS)
	  	{
  			TUint trailingBits = GetNumTrailingBits(bitbuf);
  			TInt diff = 0;
			TUint oldSPSId = seq_parameter_set_id;
			TUint oldIdLength = ReturnUnsignedExpGolombCodeLength(oldSPSId);
			TUint newIdLength = ReturnUnsignedExpGolombCodeLength(newSPSId);
			TUint storeSPS = 1;
		
  			for (i=0; i<PS_MAX_NUM_OF_SPS; i++)
  			{
			  	// Check if an older SPS matches exactly this one, then use the old
  				if (spsList[i])
	  				useOneSPS = CompareSPSSets(spsList[i],sps,ETrue);
  			
	  			if (useOneSPS)
	  			{
	  				newSPSId = i;
	  				storeSPS  =0;
  					break;		
	  			}
  			}
  		
	  		if ( newSPSId > PS_MAX_NUM_OF_SPS )
	  		{
	  			// We have reached maximum number of SPS, return an error
	  			return PS_ERROR;
	  		}
	  		
	  		(*aNumSPS)++;
	  	
	  		// Set indexChanged to true and give the new index to old SPS, so that (new) PPS can refer to the new SPS
	  		spsList[seq_parameter_set_id]->indexChanged = 1;
	  		spsList[seq_parameter_set_id]->newSPSId = newSPSId;	// The new Id
	  	
	  		if (aFrameFromEncoder)
	  		{
		  		spsList[seq_parameter_set_id]->encSPSId = newSPSId;	// The new Id
		  	
		  		// Store information that there are different SPS in use, we have to encode until an IDR NAL unit.
		  		*aEncodeUntilIDR = ETrue;
	  		}
			else
		  		spsList[seq_parameter_set_id]->origSPSId = newSPSId;	// The new Id
		  	
	  		
	  		// Store the new SPS at the new index and modify SPS id, 
	  		// unless we are using a previously stored SPS
	  		if(storeSPS)
	  		{
		  		spsList[newSPSId] = sps;
		  		
				// Restore the bit buffer position at the SPS Id
				bitbuf->bitpos = bitPosit;
				bitbuf->bytePos = bytePosit;
		
				if(trailingBits > 8)
				{
					trailingBits = 8;
				}
		
				if ( oldIdLength == newIdLength )
				{
					// Just encode the new Id on top of the old Id
					bitbuf->bitpos += oldIdLength;
					if (bitbuf->bitpos > 8)
					{	
						// Go to the right byte and bit position
						bitbuf->bytePos -= bitbuf->bitpos / 8;
						bitbuf->bitpos = bitbuf->bitpos % 8;
					}
				
					EncodeUnsignedExpGolombCode(bitbuf, newSPSId);
				}
				else if ( oldIdLength < newIdLength )
				{
					diff = newIdLength - oldIdLength;
			
					// Adjust the SPS length
					if (diff >= 8)
					{
						// Add as many extra bytes as is required
						sps->SPSlength += (diff / 8);
					}
				
					if ( trailingBits < (diff % 8) )
					{
						// Add one byte since there aren't enough trailing bits for the extra bits
						sps->SPSlength += 1;
					}
			
					ShiftBufferRight(bitbuf, diff, trailingBits, oldIdLength);
	
					// After shifting, encode the new value to the bit buffer
					EncodeUnsignedExpGolombCode(bitbuf, newSPSId);
				}
				else
				{
					// New id's length is smaller than old id's length
					diff = oldIdLength - newIdLength;
			
					if (diff >= 8)
					{
						// Adjust the SPS length
						sps->SPSlength -= (diff / 8);
					}
			
					ShiftBufferLeft(bitbuf, diff, oldIdLength);
			
					// After shifting, encode the new value to the bit buffer
					EncodeUnsignedExpGolombCode(bitbuf, newSPSId);
				}
	  		}
	  	}
	  	else	// Use one SPS for both
	  	{
	  		// Reset indexChanged to false 
	  		spsList[seq_parameter_set_id]->indexChanged = 0;
	  		
	  		// Check if the frame numbering or POC numbering has to be changed
	  		if (spsList[seq_parameter_set_id]->log2_max_frame_num_minus4 != sps->log2_max_frame_num_minus4)
	  		{
	  			spsList[seq_parameter_set_id]->maxFrameNumChanged = 1;
	  		
	  			if (aFrameFromEncoder)
			  		spsList[seq_parameter_set_id]->encMaxFrameNum = sps->log2_max_frame_num_minus4;
				else
		  			spsList[seq_parameter_set_id]->origMaxFrameNum = sps->log2_max_frame_num_minus4;
	  		}
	  		else
	  		{
	  			// Reset the value in case it was changed for another clip earlier
	  			spsList[seq_parameter_set_id]->maxFrameNumChanged = 0;
	  		}

	  		if (spsList[seq_parameter_set_id]->log2_max_pic_order_cnt_lsb_minus4 != sps->log2_max_pic_order_cnt_lsb_minus4)
	  		{
	  			spsList[seq_parameter_set_id]->maxPOCNumChanged = 1;
	  		
	  			if (aFrameFromEncoder)
			  		spsList[seq_parameter_set_id]->encMaxPOCNum = sps->log2_max_pic_order_cnt_lsb_minus4;
				else
		  			spsList[seq_parameter_set_id]->origMaxPOCNum = sps->log2_max_pic_order_cnt_lsb_minus4;
	  		}
	  		else
	  		{
	  			// Reset the value in case it was changed for another clip earlier
	  			spsList[seq_parameter_set_id]->maxPOCNumChanged = 0;
	  		}
	  	}
  	}

  	if ( IsSPSSupported(sps) == KErrNotSupported )
  		return KErrNotSupported;
  	

  	// Store the buffer containing the SPS set in order to later pass it to the 3gpmp4library
  	// If we use the same sps for both, don't allocate, otherwise allocate
  	if ( !useOneSPS )
  	{
  		
		// Store the buffer containing the SPS set in order to later pass it to the 3gpmp4library
  		sps->codedSPSBuffer = (TUint8*) User::Alloc(sps->SPSlength);    

		if (sps->codedSPSBuffer == 0)
			return PS_ERR_MEM_ALLOC;
  
  		for (i=0; i<sps->SPSlength; i++)
  		{
  			sps->codedSPSBuffer[i] = bitbuf->data[i];
  		}
  	}
  	else if (possibleIdConflict)
  	{
  		// Free the SPS since we will use only one which has been already allocated earlier
	  	User::Free(sps);
  	}


  	return PS_OK;
}


// ComparePPSSets
// Compares two input PPS sets to see if a single PPS set could be used for both. 
// Returns ETrue if the sets are similar enough, EFalse otherwise.
TInt ComparePPSSets( pic_parameter_set_s *aPPSSet1, pic_parameter_set_s *aPPSSet2 )
{
	TUint i;

	// This is the most likely parameter to differ, thus check it first
	if ( aPPSSet1->pic_init_qp_minus26 != aPPSSet2->pic_init_qp_minus26 ||
		 aPPSSet1->pic_init_qs_minus26 != aPPSSet2->pic_init_qs_minus26 )
	{
		return EFalse;
	}

	if ( aPPSSet1->entropy_coding_mode_flag != aPPSSet2->entropy_coding_mode_flag )
	{
		return EFalse;
	}

	if ( aPPSSet1->pic_order_present_flag != aPPSSet2->pic_order_present_flag )
	{
		return EFalse;
	}

	if ( aPPSSet1->num_slice_groups_minus1 != aPPSSet2->num_slice_groups_minus1 )
	{
		return EFalse;
	}
	else
	{
	  	if ( aPPSSet1->num_slice_groups_minus1 > 0 ) 
  		{
			if ( aPPSSet1->slice_group_map_type != aPPSSet2->slice_group_map_type ) 
			{
				return EFalse;
			}
			
	    	switch ( aPPSSet1->slice_group_map_type ) 
   	 		{

	      		case PS_SLICE_GROUP_MAP_TYPE_INTERLEAVED:
    	    		for (i = 0; i <= aPPSSet1->num_slice_groups_minus1; i++) 
        			{
						if ( aPPSSet1->run_length_minus1[i] != aPPSSet2->run_length_minus1[i] ) 
						{
							return EFalse;
						}
	        		}
    	    		break;

      			case PS_SLICE_GROUP_MAP_TYPE_DISPERSED:
        			break;

	      		case PS_SLICE_GROUP_MAP_TYPE_FOREGROUND:
    	    		for (i = 0; i < aPPSSet1->num_slice_groups_minus1; i++) 
        			{
						if ( aPPSSet1->top_left[i] != aPPSSet2->top_left[i] ||
						     aPPSSet1->bottom_right[i] != aPPSSet2->bottom_right[i] ) 
						{
							return EFalse;
						}
	        		}
    	    		break;

	      		case PS_SLICE_GROUP_MAP_TYPE_CHANGING_3:
    	  		case PS_SLICE_GROUP_MAP_TYPE_CHANGING_4:
      			case PS_SLICE_GROUP_MAP_TYPE_CHANGING_5:
					if ( aPPSSet1->slice_group_change_direction_flag != aPPSSet2->slice_group_change_direction_flag ||
					     aPPSSet1->slice_group_change_rate_minus1 != aPPSSet2->slice_group_change_rate_minus1 ) 
					{
						return EFalse;
					}
        			break;

	      		case PS_SLICE_GROUP_MAP_TYPE_EXPLICIT:
					if ( aPPSSet1->pic_size_in_map_units_minus1 != aPPSSet2->pic_size_in_map_units_minus1 ) 
					{
						return EFalse;
					}

	        		for( i = 0; i <= aPPSSet1->pic_size_in_map_units_minus1; i++ ) 
    	    		{
						if ( aPPSSet1->slice_group_id[i] != aPPSSet2->slice_group_id[i] ) 
						{
							return EFalse;
						}
        			}

	        		break;

    	  		default:
        			// Cannnot happen 
        			break;
  			}
  		}
	}

	if ( aPPSSet1->num_ref_idx_l0_active_minus1 != aPPSSet2->num_ref_idx_l0_active_minus1 ||
		 aPPSSet1->num_ref_idx_l1_active_minus1 != aPPSSet2->num_ref_idx_l1_active_minus1 )
	{
		return EFalse;
	}

	if ( aPPSSet1->weighted_pred_flag != aPPSSet2->weighted_pred_flag ||
		 aPPSSet1->weighted_bipred_idc != aPPSSet2->weighted_bipred_idc )
	{
		return EFalse;
	}

	if ( aPPSSet1->chroma_qp_index_offset != aPPSSet2->chroma_qp_index_offset )
	{
		return EFalse;
	}

	if ( aPPSSet1->deblocking_filter_parameters_present_flag != aPPSSet2->deblocking_filter_parameters_present_flag )
	{
		return EFalse;
	}

	if ( aPPSSet1->constrained_intra_pred_flag != aPPSSet2->constrained_intra_pred_flag )
	{
		return EFalse;
	}

	if ( aPPSSet1->redundant_pic_cnt_present_flag != aPPSSet2->redundant_pic_cnt_present_flag )
	{
		return EFalse;
	}

	return ETrue;
}


// GetNumTrailingBits
// Returns the number of trailing (zero) bits in the input bit buffer.
TInt GetNumTrailingBits(bitbuffer_s *aBitBuffer)
{
	TInt i;
	TUint bit = 0;

	for (i=0; i<8; i++)
	{
		// Get the i'th bit from the end
		bit = (aBitBuffer->data[aBitBuffer->dataLen - 1] & (1 << i)) >> i;
		if (bit)
		{
			return (i);		// Return the number of trailing bits here
		}
	}
	
	// Return 9 for cases when there are one or more zero byte at the end
	return (9);
}


// ReturnUnsignedExpGolombCodeLength
// Returns the amount of bits required for encoding the input aValue with unsigned Exp-Golomb codes.
TInt ReturnUnsignedExpGolombCodeLength(TUint aValue)
{
	TUint codeNumLength;
	
	codeNumLength = 0;
	
	aValue++;
	
	while ( aValue > 1 )
	{
		aValue >>= 1;
		codeNumLength++;
	}

	// The required code length is codeNumLength*2+1
	return ((codeNumLength << 1) + 1);
}


// EncodeUnsignedExpGolombCode
// Encodes the input aValue to the bit buffer with unsigned Exp-Golomb codes.
void EncodeUnsignedExpGolombCode(bitbuffer_s *aBitBuffer, TUint aValue)
{
	TUint codeLength;
	TUint tempValue = aValue;
	TInt i;
	TUint8 byteValue;
	 
	// First, compute the required code length
	codeLength = ReturnUnsignedExpGolombCodeLength(aValue);

	// The Exp-Golomb coded value is the same as value+1 with the prefix zero bits,
	// thus it can be simply coded by coding value+1 with the number of bits computed 
	// by the above function.
	aValue++;	

	// Then write the bits to the bit buffer one bit at a time
	for (i=codeLength-1; i>=0; i--)
	{
		tempValue = (aValue & (1 << i)) >> i;
		
		// Zero out the bitpos bit
		byteValue = aBitBuffer->data[aBitBuffer->bytePos-1] & ~(1<<(aBitBuffer->bitpos-1));

		// Add the bit from the value to be coded and store the result back to bit buffer
		byteValue |= tempValue << (aBitBuffer->bitpos-1);
		aBitBuffer->data[aBitBuffer->bytePos-1] = byteValue;
		aBitBuffer->bitpos--;
		
		if(aBitBuffer->bitpos == 0)
		{
			aBitBuffer->bytePos++;
			aBitBuffer->bitpos = 8;
		}
	}

 	// Update the currentBits value 
 	aBitBuffer->currentBits = aBitBuffer->data[aBitBuffer->bytePos-1];
}


// ShiftBitBufferBitsRight
// This function shifts bits right by aDiff in the aBitBuffer, note that if 
// the shift is more than 8 bits, full bytes should be shifted before calling this function.
// The gap between unmodified and shofted part of the buffer is filled with zero bits
void ShiftBitBufferBitsRight(bitbuffer_s *aBitBuffer, TInt aDiff)
{
	TUint8 byteValue;
	TUint8 tempValue;
	TUint8 bitMask;
	TInt i;
	
 	// Start from the end, shift bits in each byte until the current byte
 	for (i=aBitBuffer->dataLen-1; i>=aBitBuffer->bytePos; i--)
 	{
		bitMask = (1 << aDiff) - 1;	// The aDiff lowest bits
		
		// Shift the bits in this byte right by aDiff
 		byteValue = aBitBuffer->data[i];
 		byteValue >>= aDiff;
 		
 		// The aDiff lowest bits from the next byte (to the left)
 		tempValue = aBitBuffer->data[i-1] & bitMask;
 		
 		tempValue <<= (8 - aDiff);
 		aBitBuffer->data[i] = byteValue | tempValue;
 	}
 	
 	// Take care of the first byte separately
	bitMask = (1 << aBitBuffer->bitpos) - 1;	// The bitPos lowest bits
	byteValue = aBitBuffer->data[aBitBuffer->bytePos-1] & bitMask;
	byteValue >>= aDiff;		// Shift right by aDiff bits
		
	bitMask = 255 << (aBitBuffer->bitpos);	// Mask the 8-bitPos upper bits
		
	// Write the shifted value back to bit buffer
	aBitBuffer->data[aBitBuffer->bytePos-1] = (bitMask & aBitBuffer->data[aBitBuffer->bytePos-1]) | byteValue;
 	
 	// Update the currentBits value 
 	aBitBuffer->currentBits = aBitBuffer->data[aBitBuffer->bytePos-1];
}


// ShiftBitBufferBitsLeft
// This function shifts bits left by aDiff in the aBitBuffer, note that if 
// the shift is more than 8 bits, full bytes should be shifted before calling this function.
void ShiftBitBufferBitsLeft(bitbuffer_s *aBitBuffer, TInt aDiff)
{
	TUint8 byteValue;
	TUint8 tempValue;
	TUint8 bitMask;
	TInt i;
	
	
 	// Take care of the first byte separately
 	if ( aBitBuffer->bitpos > aDiff )
 	{
		bitMask = (1 << aBitBuffer->bitpos) - 1;	// The aBitBuf->bitpos lowest bits
		byteValue = aBitBuffer->currentBits & bitMask;
		
		// Shift the byteValue left by aDiff
		byteValue <<= aDiff;
		// Take only the bitpos lowest bits from this value
		byteValue &= bitMask;
		
		bitMask = 255 << (aBitBuffer->bitpos);	// Mask the 8-bitPos upper bits, i.e. the bits to the left from the start of the shift
		byteValue = byteValue | (aBitBuffer->currentBits & bitMask);
		aBitBuffer->data[aBitBuffer->bytePos-1] = byteValue;
		
		bitMask = 255 << (8 - aDiff);	// Mask the aDiff upper bits
		byteValue = aBitBuffer->data[aBitBuffer->bytePos] & bitMask;
		byteValue >>= (8 - aDiff);
		
		// "Add" the aDiff bits from the next byte (msb) to this byte (lsb)
 		aBitBuffer->data[aBitBuffer->bytePos-1] |= byteValue;
 	}
 	else
 	{
		bitMask = 255 << (aBitBuffer->bitpos);	// Mask the 8-bitPos upper bits, i.e. the bits to the left from the start of the shift
		aBitBuffer->data[aBitBuffer->bytePos-1] = aBitBuffer->currentBits & bitMask;
		
		bitMask = (1 << (8-aDiff+aBitBuffer->bitpos)) - 1;	// The 8-diff+aBitBuf->bitpos lowest bits
		tempValue = aBitBuffer->data[aBitBuffer->bytePos] & bitMask;
		
		// Shift tempValue right by 8 - diff bits, resulting in bitpos lowest bits 
		tempValue >>= (8 - aDiff);

 		aBitBuffer->data[aBitBuffer->bytePos-1] |= tempValue;
 	}
 	
 	
 	// Start from the current byte, shift bits in each byte until the end
 	for (i=aBitBuffer->bytePos; i<(aBitBuffer->dataLen-1); i++)
 	{
 	
		bitMask = 255 << (8 - aDiff);	// Mask the 8-aDiff upper bits
		byteValue = aBitBuffer->data[i+1] & bitMask;
		byteValue >>= (8 - aDiff);
		
 		tempValue = aBitBuffer->data[i];
 		tempValue <<= aDiff;

 		aBitBuffer->data[i] = byteValue | tempValue;
 	}
 	
 	// Take care of the last byte separately, just shift to the left
	aBitBuffer->data[aBitBuffer->dataLen-1] <<= aDiff;
 	
 	// Update the currentBits value 
 	aBitBuffer->currentBits = aBitBuffer->data[aBitBuffer->bytePos-1];
}


// ShiftBufferLeftByOneByte
// Shifts the bytes left in the bit buffer by one position starting from the current byte position.
void ShiftBufferLeftByOneByte(bitbuffer_s *aBitBuffer)
{
	TInt i;
	TUint8 byteValue;
	TUint8 bitMask;

	
	// For the current byte, take 8-bitpos upper bits from this byte and bitpos lowest bits from the next 
	// byte (this is ok since we are shift at least 8 bits when this function is called)
	bitMask = 255 << (aBitBuffer->bitpos);	// Mask the 8-bitPos upper bits
	aBitBuffer->data[aBitBuffer->bytePos] &= bitMask;

	bitMask = (1 << aBitBuffer->bitpos) - 1;	// The aBitBuf->bitpos lowest bits
	byteValue = aBitBuffer->data[aBitBuffer->bytePos+1] & bitMask;
	aBitBuffer->data[aBitBuffer->bytePos] |= byteValue;
	
	// Start from the next byte position, and go through the whole buffer
 	for (i=aBitBuffer->bytePos+1; i<(aBitBuffer->dataLen-1); i++)
 	{
 		// Copy the next byte to here
		aBitBuffer->data[i] =  	aBitBuffer->data[i+1];
 	}
 	
 	// Adjust the bit buffer length
 	aBitBuffer->dataLen--;
}


// ShiftBufferRightByOneByte
// Shifts the bytes right in the bit buffer by one position starting from the current byte position.
void ShiftBufferRightByOneByte(bitbuffer_s *aBitBuffer)
{
	TInt i;
	
	// Start from the last byte position, and go through the whole buffer until the current byte position
	// Note: also the current byte can be shifted, since the bits that should not be shifted from that byte
	// will be written over by the new value coded later, thus no error will be there.
 	for (i=aBitBuffer->dataLen-1; i>=aBitBuffer->bytePos; i--)
 	{
 		// Copy the next byte to here
		aBitBuffer->data[i] =  	aBitBuffer->data[i-1];
	}
}


// ShiftBufferRight
// Shifts bits right in the input bit buffer by aDiff value, the bit buffer length is modified if required.
void ShiftBufferRight(bitbuffer_s *aBitBuffer, TInt aDiff, TUint aTrailingBits, TUint aOldIdLength)
{
	TInt i;
	
	if ( aDiff >= 8 )
	{
		TUint bytesToShift = aDiff / 8;
				
		// Add byte(s) to the bit buffer
		aBitBuffer->dataLen += bytesToShift;
				
		// Shift full bytes to right
		for (i=0; i<bytesToShift; i++)
		{
			ShiftBufferRightByOneByte(aBitBuffer);
			aDiff -= 8;
		}
				
		aDiff = aDiff % 8;
	}
	
	// If there are less trailing bits than we need to shift then we have to add one byte to the buffer		
	if ( aTrailingBits < aDiff )	
	{
		// Have to add byte to the SPS set
		aBitBuffer->dataLen += 1;
	}

	if (aDiff != 0)
	{
		// Shift the bits in the bit buffer to the right
		ShiftBitBufferBitsRight(aBitBuffer, aDiff);
	}
				
	// Adjust the bitbuffer bitpos value
	aBitBuffer->bitpos += aOldIdLength;
	if ( aBitBuffer->bitpos > 8 )
	{
		aBitBuffer->bitpos -= 8;
		aBitBuffer->bytePos--;
	}
}


// ShiftBufferLeft
// Shifts bits left in the input bit buffer by aDiff value.
void ShiftBufferLeft(bitbuffer_s *aBitBuffer, TInt aDiff, TUint aOldIdLength)
{
	TInt i;
	
	if (aDiff >= 8)
	{
		// Shift full bytes to the left before shifting bits
		TUint bytesToShift = aDiff / 8;

		// First, adjust the byte position to be correct
		aBitBuffer->bytePos -= bytesToShift;
				
		for (i=0; i<bytesToShift; i++)
		{
			ShiftBufferLeftByOneByte(aBitBuffer);
			aDiff -= 8;
		}
	}
			
	// Adjust the bit position of the bit buffer
	aBitBuffer->bitpos += aOldIdLength;
	if ( aBitBuffer->bitpos > 8 )
	{
		aBitBuffer->bitpos -= 8;
		aBitBuffer->bytePos--;
					
		aBitBuffer->currentBits = aBitBuffer->data[aBitBuffer->bytePos-1];
	}
			
	if ( aDiff != 0 )
	{
		// Shift the bits in the bit buffer to the left
		ShiftBitBufferBitsLeft(aBitBuffer, aDiff);
	}
	
}


// psParsePPS
// Parses the input PPS set, the PPS and SPS id's are modified if necessary 
// and the modified data is stored in codedPPSBuffer.
TInt psParsePPS( bitbuffer_s *bitbuf, pic_parameter_set_s **ppsList, seq_parameter_set_s **spsList, 
				 TUint aFrameFromEncoder, TUint *aNumPPS )
{
  	pic_parameter_set_s *pps;
  	TUint i, tmp;
  	TInt len;
  	TUint pic_parameter_set_id;
  	TInt retCode;
  	TUint pic_size_in_map_units_minus1;
  	TUint newPPSId = 0;
  	TUint possibleIdConflict = 0;
  	TUint modifySPSId = 0;
  	TUint bitPosit = 0;
  	TUint bytePosit = 0;
  	TInt bitSPSPosit = 0;
  	TUint byteSPSPosit = 0;
  	TUint useOnePPS = 0;

  	// Parse pps id 
  	if ((retCode = ue_v(bitbuf, &pic_parameter_set_id, PS_MAX_NUM_OF_PPS-1)) < 0)
    	return retCode;

  	// Allocate memory for pps if not already allocated
  	pps = ppsList[pic_parameter_set_id];

  	if (!pps) 
  	{
    	pps = (pic_parameter_set_s *) User::Alloc(sizeof(pic_parameter_set_s));
    	
    	if (pps == 0) 
    	{
      		PRINT((_L("Error while allocating memory for PPS.\n")));
      		return PS_ERR_MEM_ALLOC;
    	}
    
    	memset( pps, 0, sizeof(pic_parameter_set_s));
    	ppsList[pic_parameter_set_id] = pps;
    	
    	(*aNumPPS)++;
  	}
  	else
  	{
  		// There might be a conflicting Id with an existing PPS set
  		// Give the new SPS set the next free PPS Id
  		possibleIdConflict = 1;
  		useOnePPS = 1;
  		newPPSId = 0;
  	
  		while (ppsList[newPPSId])
  		{
  			newPPSId++;
  		}
  	
  		// Allocate memory for the PPS
    	pps = (pic_parameter_set_s *) User::Alloc(sizeof(pic_parameter_set_s));
    	
    	if (pps == 0) 
    	{
      		PRINT((_L("Error while allocating memory for PPS.\n")));
      		return PS_ERR_MEM_ALLOC;
    	}
    
    	memset( pps, 0, sizeof(pic_parameter_set_s));
    	pps->pic_parameter_set_id = newPPSId;
    
  		// Store the position of the bit buffer
  		bitPosit = bitbuf->bitpos;
  		bytePosit = bitbuf->bytePos;
  	}


  	// Parse the rest of the picture parameter set syntax
  	if ((retCode = ue_v( bitbuf, &pps->seq_parameter_set_id, PS_MAX_NUM_OF_SPS-1)) < 0)
    	return retCode;
  
  	// Check if the Id of the SPS that this PPS refers to has changed (and that 
  	// the frame originated form the encoder)
  	if( spsList[pps->seq_parameter_set_id]->indexChanged)
  	{
  		if ( !aFrameFromEncoder )
  		{
  			if (pps->seq_parameter_set_id != spsList[pps->seq_parameter_set_id]->origSPSId)
  			{
		  		// Indicate a changed SPS Id, perform the change at the end (when we know the size of the PPS set)
  				modifySPSId = ETrue;
  	
  				// Store the position of the bit buffer
  				bitSPSPosit = bitbuf->bitpos;
  				byteSPSPosit = bitbuf->bytePos;
  			}
  		}
  		else
  		{
	  		// Indicate a changed SPS Id, perform the change at the end (when we know the size of the PPS set)
  			modifySPSId = ETrue;
	  	
  			// Store the position of the bit buffer
  			bitSPSPosit = bitbuf->bitpos;
  			byteSPSPosit = bitbuf->bytePos;
  		}
  	}

  	// Fetch entropy coding mode. Mode is 0 for CAVLC and 1 for CABAC 
  	if ((retCode = u_n( bitbuf, 1, &pps->entropy_coding_mode_flag)) < 0)
    	return retCode;

  	// If this flag is 1, POC related syntax elements are present in slice header 
  	if ((retCode = u_n( bitbuf, 1, &pps->pic_order_present_flag)) < 0)
    	return retCode;

  	// Fetch the number of slice groups minus 1 
  	if ((retCode = ue_v( bitbuf, &pps->num_slice_groups_minus1, PS_MAX_NUM_SLICE_GROUPS-1)) < 0)
    	return retCode;

  	if(pps->num_slice_groups_minus1 > 0 ) 
  	{

    	if ((retCode = ue_v( bitbuf, &pps->slice_group_map_type, 6)) < 0)
      		return retCode;

    	switch (pps->slice_group_map_type) 
    	{

      		case PS_SLICE_GROUP_MAP_TYPE_INTERLEAVED:
        		for (i = 0; i <= pps->num_slice_groups_minus1; i++) 
        		{
          			if ((retCode = ue_v( bitbuf, &pps->run_length_minus1[i], MAX_PIC_SIZE_IN_MBS-1 )) < 0)
            			return retCode;
        		}
        		break;

      		case PS_SLICE_GROUP_MAP_TYPE_DISPERSED:
        		break;

      		case PS_SLICE_GROUP_MAP_TYPE_FOREGROUND:
        		for (i = 0; i < pps->num_slice_groups_minus1; i++) 
        		{
          			// Fetch MB address of the top-left corner 
          			if ((retCode = ue_v( bitbuf, &pps->top_left[i], MAX_PIC_SIZE_IN_MBS-1)) < 0)
            			return retCode;
          			// Fetch MB address of the bottom-right corner (top-left address must 
          			// be smaller than or equal to bottom-right address)                  
          			if ((retCode = ue_v( bitbuf, &pps->bottom_right[i], MAX_PIC_SIZE_IN_MBS-1)) < 0)
            			return retCode;

          			if (pps->top_left[i] > pps->bottom_right[i])
            			return PS_ERR_ILLEGAL_VALUE;
        		}
        		break;

      		case PS_SLICE_GROUP_MAP_TYPE_CHANGING_3:
      		case PS_SLICE_GROUP_MAP_TYPE_CHANGING_4:
      		case PS_SLICE_GROUP_MAP_TYPE_CHANGING_5:
        		if ((retCode = u_n( bitbuf, 1, &pps->slice_group_change_direction_flag)) < 0)
          			return retCode;
        		if ((retCode = ue_v( bitbuf, &pps->slice_group_change_rate_minus1, MAX_PIC_SIZE_IN_MBS-1)) < 0)
          			return retCode;
        		break;

      		case PS_SLICE_GROUP_MAP_TYPE_EXPLICIT:

        		if ((retCode = ue_v( bitbuf, &pic_size_in_map_units_minus1, MAX_PIC_SIZE_IN_MBS-1 )) < 0)
          			return retCode;

        		// Allocate array for slice group ids if not already allocated 
        		if (pic_size_in_map_units_minus1 != pps->pic_size_in_map_units_minus1) 
        		{
          			User::Free(pps->slice_group_id);

          			pps->slice_group_id = (unsigned int *)User::Alloc( (pic_size_in_map_units_minus1+1) * sizeof(int));

					if (pps->slice_group_id == 0)
                        return PS_ERR_MEM_ALLOC;

          			pps->pic_size_in_map_units_minus1 = pic_size_in_map_units_minus1;
        		}

        		// Calculate len = ceil( Log2( num_slice_groups_minus1 + 1 ) )
        		tmp = pps->num_slice_groups_minus1 + 1;
        		tmp = tmp >> 1;
        		for( len = 0; len < 16 && tmp != 0; len++ )
          			tmp >>= 1;
        		
        		if ( (((unsigned)1)<<len) < (pps->num_slice_groups_minus1 + 1) )
          			len++;

        		for( i = 0; i <= pps->pic_size_in_map_units_minus1; i++ ) 
        		{
          			if ((retCode = u_n( bitbuf, len, &pps->slice_group_id[i])) < 0)
            			return retCode;
        		}

        		break;

      		default:
        		// Cannnot happen 
        		break;
    		}
  	}

  	if ((retCode = ue_v( bitbuf, &pps->num_ref_idx_l0_active_minus1, 31 )) < 0)
    	return retCode;

  	if ((retCode = ue_v( bitbuf, &pps->num_ref_idx_l1_active_minus1, 31 )) < 0)
    	return retCode;

  	if ((retCode = u_n( bitbuf, 1, &pps->weighted_pred_flag)) < 0)
    	return retCode;

  	if ((retCode = u_n( bitbuf, 2, &pps->weighted_bipred_idc)) < 0)
    	return retCode;

  	if (pps->weighted_bipred_idc > 2)
    	return PS_ERR_ILLEGAL_VALUE;

  	if ((retCode = se_v( bitbuf, &pps->pic_init_qp_minus26, -26, 25 )) < 0)
    	return retCode;

  	if ((retCode = se_v( bitbuf, &pps->pic_init_qs_minus26, -26, 25  )) < 0)
    	return retCode;

  	if ((retCode = se_v( bitbuf, &pps->chroma_qp_index_offset, -12, 12 )) < 0)
    	return retCode;

  	pps->chroma_qp_index_offset = clip(MIN_CHROMA_QP_INDEX, MAX_CHROMA_QP_INDEX, pps->chroma_qp_index_offset);

  	if ((retCode = u_n( bitbuf, 1, &pps->deblocking_filter_parameters_present_flag )) < 0)
    	return retCode;

  	if ((retCode = u_n( bitbuf, 1, &pps->constrained_intra_pred_flag )) < 0)
    	return retCode;

  	if ((retCode = u_n( bitbuf, 1, &pps->redundant_pic_cnt_present_flag )) < 0)
    	return retCode;

  	if (bibSkipTrailingBits(bitbuf) < 0)
    	return PS_ERROR;

  	// Store the size of the PPS set
  	pps->PPSlength = bitbuf->bytePos;
  
	syncBitBufferBitpos(bitbuf);
	
  	// If we had a possible conflict, compare the PPS sets with the same id to see if only one PPS set can be used.
  	if (possibleIdConflict)
  	{
		useOnePPS = ComparePPSSets(ppsList[pic_parameter_set_id],pps);

	  	if (!useOnePPS)
	  	{
  			TUint trailingBits = GetNumTrailingBits(bitbuf);
  			TInt diff = 0;
			TUint oldPPSId = pic_parameter_set_id;
			TUint oldIdLength = ReturnUnsignedExpGolombCodeLength(oldPPSId);
			TUint newIdLength = ReturnUnsignedExpGolombCodeLength(newPPSId);

	  		for (i=0; i<PS_MAX_NUM_OF_PPS; i++)
  			{
  				// Compared if a previously stored PPS might be used
  				if (ppsList[i])
					useOnePPS = ComparePPSSets(ppsList[i],pps);
  			
	  			if (useOnePPS)
  				{
  					// Check also that the SPS id's match
				  	if ( modifySPSId )
				  	{
						if (spsList[pps->seq_parameter_set_id]->newSPSId != ppsList[i]->seq_parameter_set_id)
							useOnePPS = 0;
						else
						{
	  						// We can use this previously generated PPSId here also
  							newPPSId = i;
  							break;	
						}
				  	}
				  	else
				  	{
						if (pps->seq_parameter_set_id != ppsList[i]->seq_parameter_set_id)
							useOnePPS = 0;
						else
						{
	  						// We can use this previously generated PPSId here also
  							newPPSId = i;
  							break;	
						}
				  	}
  				}
  			}
		
	  		if ( newPPSId > PS_MAX_NUM_OF_PPS )
	  		{
	  			// We have reached maximum number of PPS, return an error
	  			return PS_ERROR;
	  		}	
	  	
	    	(*aNumPPS)++;

	  		// Set indexChanged to true and give the new index to old PPS, so that (new) slices can refer to the new PPS
	  		ppsList[pic_parameter_set_id]->indexChanged = 1;
	  		ppsList[pic_parameter_set_id]->newPPSId = newPPSId;	// The new Id
	  	
	  		if (aFrameFromEncoder)
		  		ppsList[pic_parameter_set_id]->encPPSId = newPPSId;		// The new Id
			else
		  		ppsList[pic_parameter_set_id]->origPPSId = newPPSId;	// The new Id
		  	
	  		// Store the new PPS at the new index, unless we are using a previously stored PPS
			if (!ppsList[newPPSId])
		  		ppsList[newPPSId] = pps;
  		
			// Restore the bit buffer position at the PPS Id
			bitbuf->bitpos = bitPosit;
			bitbuf->bytePos = bytePosit;
		
			if(trailingBits > 8)
			{	
				trailingBits = 8;
			}
		
			if ( oldIdLength == newIdLength )
			{	
				// Just encode the new Id on top of the old Id
				bitbuf->bitpos += oldIdLength;
				if (bitbuf->bitpos > 8)
				{	
					// Go to the right byte and bit position
					bitbuf->bytePos -= bitbuf->bitpos / 8;
					bitbuf->bitpos = bitbuf->bitpos % 8;
				}
				
				EncodeUnsignedExpGolombCode(bitbuf, newPPSId);
			}
			else if ( oldIdLength < newIdLength )
			{
				diff = newIdLength - oldIdLength;
			
				// Adjust the PPS length
				if (diff >= 8)
				{
					// Add as many extra bytes as is required
					pps->PPSlength += (diff / 8);
				}
			
				if ( trailingBits < (diff % 8) )
				{
					// Add one byte since there aren't enough trailing bits for the extra bits
					pps->PPSlength += 1;
				}
			
				ShiftBufferRight(bitbuf, diff, trailingBits, oldIdLength);

				// After shifting, encode the new value to the bit buffer
				EncodeUnsignedExpGolombCode(bitbuf, newPPSId);
			}
			else
			{
				// New id's length is smaller than old id's length
				diff = oldIdLength - newIdLength;
			
				if (diff >= 8)
				{
					// Adjust the PPS length
					pps->PPSlength -= (diff / 8);
				}
			
				ShiftBufferLeft(bitbuf, diff, oldIdLength);
			
				// After shifting, encode the new value to the bit buffer
				EncodeUnsignedExpGolombCode(bitbuf, newPPSId);
			}

	  		// Store the position of the bit buffer for possible SPS id modification
  			// The right bit position is the current minus the size of the SPS id length
  			bitSPSPosit = bitbuf->bitpos - ReturnUnsignedExpGolombCodeLength(pps->seq_parameter_set_id);
  			byteSPSPosit = bitbuf->bytePos;
  		
  			if(bitSPSPosit < 1)
  			{
	  			byteSPSPosit++;
  				bitSPSPosit += 8;
  			}
		}
		else
		{
	  		// In case the index was changed for some earlier PPS, 
	  		// reset the index changed value back to zero
	  		ppsList[pic_parameter_set_id]->indexChanged = 0;
		}
  	}
  
  	if ( modifySPSId )
  	{
  		TUint trailingBits = GetNumTrailingBits(bitbuf);
  		TInt diff = 0;

		TUint oldSPSId = pps->seq_parameter_set_id;
		TUint newSPSId = spsList[pps->seq_parameter_set_id]->newSPSId;
		
		TUint oldIdLength = ReturnUnsignedExpGolombCodeLength(oldSPSId);
		TUint newIdLength = ReturnUnsignedExpGolombCodeLength(newSPSId);
		
		
		// Restore the bit buffer position at the SPS Id
		bitbuf->bitpos = bitSPSPosit;
		bitbuf->bytePos = byteSPSPosit;
		
		if(trailingBits > 8)
		{
			trailingBits = 8;
		}
		
		if ( oldIdLength == newIdLength )
		{
			// Just encode the new Id on top of the old Id
			bitbuf->bitpos += oldIdLength;
			if(bitbuf->bitpos > 8)
			{	
				// Go to the right byte and bit position
				bitbuf->bytePos -= bitbuf->bitpos / 8;
				bitbuf->bitpos = bitbuf->bitpos % 8;
			}
		
			EncodeUnsignedExpGolombCode(bitbuf, newSPSId);
		}
		else if ( oldIdLength < newIdLength )
		{
			diff = newIdLength - oldIdLength;
			
			// Adjust the PPS length
			if (diff >= 8)
			{
				// Add as many extra bytes as is required
				pps->PPSlength += (diff / 8);
			}
		
			if ( trailingBits < (diff % 8) )
			{
				// Add one byte since there aren't enough trailing bits for the extra bits
				pps->PPSlength += 1;
			}
			
			ShiftBufferRight(bitbuf, diff, trailingBits, oldIdLength);

			// After shifting, encode the new value to the bit buffer
			EncodeUnsignedExpGolombCode(bitbuf, newSPSId);
			
		}
		else
		{
			// New id's length is smaller than old id's length
			diff = oldIdLength - newIdLength;
			
			if (diff >= 8)
			{
				// Adjust the PPS length
				pps->PPSlength -= (diff / 8);
			}
			
			ShiftBufferLeft(bitbuf, diff, oldIdLength);
			
			// After shifting, encode the new value to the bit buffer
			EncodeUnsignedExpGolombCode(bitbuf, newSPSId);
		}
  
  		// Modify the SPS id in the pps
  		pps->seq_parameter_set_id = newSPSId;	
  	}
  
	if ( IsPPSSupported(pps) == KErrNotSupported )
  		return KErrNotSupported;
  
  	
  	// Allocate memory for the encoded PPS data in case we have a new PPS (i.e. a new original PPS or a PPS with a new index)  
  	if ( !useOnePPS )
  	{
		// Store the buffer containing the PPS set in order to later pass it to the 3gpmp4library
  		pps->codedPPSBuffer = (TUint8*) User::Alloc(pps->PPSlength);    

		if (pps->codedPPSBuffer == 0)
		    return PS_ERR_MEM_ALLOC;
  
  		for (i=0; i<pps->PPSlength; i++)
  		{
  			pps->codedPPSBuffer[i] = bitbuf->data[i];
  		}
  	}
  	else if (possibleIdConflict)
  	{
  		// In case of conflicting id and one PPS, free the PPS allocated here
	  	User::Free(pps);
  	}

  	return PS_OK;
}

#endif  // VIDEOEDITORENGINE_AVC_EDITING

