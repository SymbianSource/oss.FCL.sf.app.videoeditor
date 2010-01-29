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


/**************************************************************************
  chandefs.h - Constants for AAC coder.
 
  Author(s): Juha Ojanpera
  Copyright (c) 1999-2004 by Nokia Research Center, Audio-Visual Systems.
  *************************************************************************/

#ifndef AAC_CHANDEFS_H_
#define AAC_CHANDEFS_H_

/*-- Project Headers. --*/
#include "defines.h"

#define GAIN_START                  (40)
#define MAGIC_NUMBER                (0.4054)
#define MAGIC_NUMBER2               (1.0f - MAGIC_NUMBER)
#define MAGIC_NUMBER3               (0.5f * MAGIC_NUMBER)
#define QSTEP_SIZE                  (7)
#define GAIN_FACTOR                 (1.0f / -0.25f)
#define IMAX_QUANT                  (1.0f / MAX_AAC_QUANT)
#define POWRAISE43                  (4.0f / 3.0f)
#define ILOG2                       (1.0f / log10(2.0f))
#define LOG34                       (log10(0.75f) * ILOG2)
#define SFCGAIN                     (16.0f / 3.0f)
#define QROUND_ERROR LOG2           (5.5f)
#define MAXCOEF_LOG2_16_3_MINUS_TWO (67.33239402f)
#define ILOG2_16_3                  (17.71694984f)
#define LOG2(x)                     (log10(x) * ILOG2)

enum
{
  /*
   * Channels for 5.1 main profile configuration 
   * (modify for any desired decoder configuration)
   */

  /*-- Front channels: left, center, right --*/
  FChansD                = 2,

  /*-- 1 if decoder has front center channel. --*/
  FCenterD               = 0,

  /*-- Side channels: --*/
  SChansD                = 0,

  /*-- Back channels: left surround, right surround. --*/
  BChansD                = 0,

  /*-- 1 if decoder has back center channel. --*/
  BCenterD               = 0,

  /*-- LFE channels. --*/
  LChansD                = 0,

  /*-- Scratch space for parsing unused channels. --*/
  XChansD                = 2,
  
  /*-- Total number of supported channels. --*/
  ChansD                 = FChansD + SChansD + BChansD + LChansD + XChansD,

  /*-- Independently switched coupling channels. --*/
  ICChansD               = 0,

  /*-- Dependently switched coupling channels. --*/
  DCChansD               = 0,

  /*-- Scratch space index for parsing unused coupling channels. --*/
  XCChansD               = 1,

  /*-- Total number of supported CC channels. --*/
  CChansD                = (ICChansD + DCChansD + XCChansD),


  /*-- Block switching. --*/
  LN                     = 2048,
  SN                     = 256,
  LN2_960                = 960,
  SN2_120                = 120,
  LN2                    = LN / 2,
  SN2                    = SN / 2,
  NSHORT                 = LN / SN,
  MAX_SBK                = NSHORT,
  NUM_WIN_SEQ            = 4,

  /*-- Max number of scale factor bands. --*/
  MAXBANDS               = 16 * NSHORT,
  MAXLONGSFBBANDS        = 51,
  MAXSHORTSFBBANDS       = 15,

  /*-- Maximum scale factor. --*/
  SFACBOOK_SIZE          = 121,
  MIDFAC                 = (SFACBOOK_SIZE - 1) / 2,
  
  /*-- Global gain must be positive. --*/
  SF_OFFSET              = 100,
  /*-- Quantization constants. --*/
  MAX_QUANT_VALUE        = 8191,
  
  /*-- Huffman parameters. --*/
  ZERO_HCB               = 0,
  ESCBOOK                = 11,
  NSPECBOOKS             = ESCBOOK + 1,
  BOOKSCL                = NSPECBOOKS,
  NBOOKS                 = NSPECBOOKS + 1,
  INTENSITY_HCB2         = 14,
  INTENSITY_HCB          = 15,
  NOISE_HCB              = 13,
  NOISE_PCM_BITS         = 9,
  NOISE_PCM_OFFSET       = (1 << (NOISE_PCM_BITS - 1)),
  NOISE_OFFSET           = 90,
  RESERVED_HCB           = 13,
  SF_INDEX_OFFSET        = 60, /*-- Offset for Huffman table indices of scalefactors. --*/
  
  LONG_SECT_BITS         = 5, 
  SHORT_SECT_BITS        = 3, 
  
  /*-- Program Configuration. --*/
  Main_Object            = 0, 
  LC_Object              = 1, 
  SSR_Object             = 2,
  LTP_Object             = 3,
  Scalable_Object        = 4,
  
  Fs_48                  = 3,
  Fs_44                  = 4, 
  Fs_32                  = 5, 
 
  /*-- Raw bitstream constants. --*/
  LEN_SE_ID              = 3,
  LEN_TAG                = 4,
  LEN_COM_WIN            = 1,
  LEN_ICS_RESERV         = 1, 
  LEN_WIN_SEQ            = 2,
  LEN_WIN_SH             = 1,
  LEN_MAX_SFBL           = 6, 
  LEN_MAX_SFBS           = 4, 
  LEN_CB                 = 4,
  LEN_SCL_PCM            = 8,
  LEN_SCL_PCM_MASK       = (1 << LEN_SCL_PCM) - 1,
  LEN_PRED_PRES          = 1,
  LEN_PRED_RST           = 1,
  LEN_PRED_RSTGRP        = 5,
  LEN_PRED_ENAB          = 1,
  LEN_MASK_PRES          = 2,
  LEN_MASK               = 1,
  
  LEN_PULSE_NPULSE       = 2, 
  LEN_PULSE_ST_SFB       = 6, 
  LEN_PULSE_POFF         = 5, 
  LEN_PULSE_PAMP         = 4, 
  NUM_PULSE_LINES        = 4, 
  PULSE_OFFSET_AMP       = 4, 
  
  LEN_IND_CCE_FLG        = 1,  
  LEN_NCC                = 3,
  LEN_IS_CPE             = 1, 
  LEN_CC_LR              = 1,
  LEN_CC_DOM             = 1,
  LEN_CC_SGN             = 1,
  LEN_CCH_GES            = 2,
  LEN_CCH_CGP            = 1,
  
  LEN_D_ALIGN            = 1,
  LEN_D_CNT              = 8,
  LEN_D_ESC              = 8,
  LEN_F_CNT              = 4,
  LEN_F_ESC              = 8,
  LEN_NIBBLE             = 4,
  LEN_BYTE               = 8,
  LEN_PAD_DATA           = 8,
  
  LEN_PC_COMM            = 8, 
  
  /*-- FILL --*/
  LEN_EX_TYPE            = 4,
  EX_FILL                = 0, 
  EX_FILL_DATA           = 1, 
  EX_DRC                 = 11, 
  
  /*-- DRC --*/
  LEN_DRC_PL             = 7, 
  LEN_DRC_PL_RESV        = 1, 
  LEN_DRC_PCE_RESV       = (8 - LEN_TAG),
  LEN_DRC_BAND_INCR      = 4, 
  LEN_DRC_BAND_RESV      = 4, 
  LEN_DRC_BAND_TOP       = 8, 
  LEN_DRC_SGN            = 1, 
  LEN_DRC_MAG            = 7, 
  DRC_BAND_MULT          = 4,

  /*-- Channel elements. --*/
  ID_SCE                 = 0,
  ID_CPE,
  ID_CCE,
  ID_LFE,
  ID_DSE,
  ID_PCE,
  ID_FIL,
  ID_END,

  FILL_VALUE             = 0x55,

  /*-- Bit reservoir. --*/
  BIT_RESERVOIR_SIZE     = 6144,
  
  /*-- Program configuration element --*/
  LEN_PROFILE            = 2, 
  LEN_SAMP_IDX           = 4, 
  LEN_NUM_ELE            = 4, 
  LEN_NUM_LFE            = 2, 
  LEN_NUM_DAT            = 3, 
  LEN_NUM_CCE            = 4, 
  LEN_MMIX_IDX           = 2, 
  LEN_PSUR_ENAB          = 1, 
  LEN_ELE_IS_CPE         = 1,
  LEN_IND_SW_CCE         = 1,  
  LEN_COMMENT_BYTES      = 8,

  /*-- LTP constants. --*/
  LTP_MAX_PRED_BANDS     = 40,
  LTP_COEF_BITS          = 3,
  LTP_LAG_BITS           = 11,

  /*-- TNS constants. --*/
  TNS_MAX_FILT           = 3,
  TNS_MAX_COEFF_RES      = 2,
  TNS_MAX_COEFF          = 32,

  TNS_MAX_ORDER          = 12,
  TNS_COEFF_RES_OFFSET   = 3,

  /*-- BWP constants. --*/
  MAX_PGRAD              = 2,
  MINVAR                 = 1,
  Q_ZERO                 = 0x0000,
  Q_ONE                  = 0x3F80,

  /*-- PNS parameters. --*/
  MAX_DCT_LEN            = 64,
  NOISE_FREE_BANDS       = 3,
  NOISE_FREE_MASK        = (1 << NOISE_FREE_BANDS) - 1,
  /*-- Max length of DSE, bytes. --*/
  MAX_DBYTES             = ((1 << LEN_D_CNT) + (1 << LEN_D_ESC)),
  
  /*-- size of exp cache table. --*/
  TEXP                   = 256,
  TEXP_MASK              = (TEXP - 1),

  /*-- Size of inv quant table. --*/
  MAX_AAC_QUANT          = 255,
  MAX_IQ_TBL             = MAX_AAC_QUANT + 1,
  IQ_MASK                = (MAX_IQ_TBL - 1)
};

/*
   Purpose:     Mixing modes for CCE (coupling channel element).
   Explanation: - */
typedef enum CCmixMode
{
  CC_BEFORE_TNS, /* Dependently switched.   */
  CC_AFTER_TNS,  /* Dependently switched.   */
  CC_TIME_MIX    /* Independently switched. */

} CCmixMode;

/*
   Purpose:     Window shapes.
   Explanation: - */
typedef enum WindowShape
{
  WS_SIN = 0, /* Sine window.                  */
  WS_KBD      /* Kaiser-Bessel Derived window. */
  
} WindowShape;

/*
   Purpose:     AAC predictor type.
   Explanation: - */
typedef enum PredType
{
  NO_PRED = 0,
  BWAP_PRED,
  LTP_PRED

} PredType;

/*
   Purpose:     Block types for transform coders using block switching. 
   Explanation: The first four describe the actual block type for each subband,
                the rest of the declarations describe the block type for the
                whole frame. */
typedef enum AAC_WINDOW_TYPE
{
  ONLY_LONG_WND,
  LONG_SHORT_WND,
  ONLY_SHORT_WND,
  SHORT_LONG_WND

} AAC_WINDOW_TYPE, AacWindowType;

/*
   Purpose:     Block sequence (long and short) parameters.
   Explanation: - */
class CInfo : public CBase
    {

public:

    static CInfo* NewL();
    ~CInfo();

    int16 islong;
    int16 nsbk;
    int16 bins_per_bk;
    int16 sfb_per_bk;
    int16* bins_per_sbk;
    int16* sfb_per_sbk;
    const int16 *sbk_sfb_top[MAX_SBK];
    int16 *sfb_width_128;
    int16* bk_sfb_top;
    int16 num_groups;
    int16* group_len;
    int16* group_offs;
  
private:
    void ConstructL();
    CInfo();

};

/*
   Purpose:     Sampling rate dependent parameters.
   Explanation: - */
typedef struct Sr_InfoStr
{
  int32 samp_rate;

  int16 nsfb1024;
  const int16 *SFbands1024;

  int16 nsfb128;
  const int16 *SFbands128;

  int16 nsfb960;
  const int16 *SFbands960;

  int16 nsfb120;
  const int16 *SFbands120;
  
} SR_Info;

/*
   Purpose:     Sfb related information.
   Explanation: - */
class CSfb_Info : public CBase
{  

public:

    static CSfb_Info* NewL(uint8 isEncoder = FALSE);
    ~CSfb_Info();

    CInfo *only_long_info;
    CInfo *eight_short_info;
    
    CInfo *winmap[NUM_WIN_SEQ];
    int16 sfbwidth128[1 << LEN_MAX_SFBS];
    
    /*-- Scalefactor offsets. --*/
    int16 *sect_sfb_offsetL;
    int16 *sect_sfb_offsetS;
    int16 *sfbOffsetTablePtr[2];
    int16 *sect_sfb_offsetS2[NSHORT];

private:
    void ConstructL(uint8 isEncoder);
    CSfb_Info();

};

/*
   Purpose:     Window shapes for this and previous frame.
   Explanation: - */
typedef struct Wnd_ShapeStr
{
  uint8 this_bk;
  uint8 prev_bk;
  
} Wnd_Shape;

#endif /*-- AAC_CHANDEFS_H_ --*/
