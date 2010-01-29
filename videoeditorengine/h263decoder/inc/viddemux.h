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
* Header for the video demultiplexer module.
*
*/


#ifndef _VIDDEMUX_H_
#define _VIDDEMUX_H_

#include "epoclib.h"

#include "biblin.h"

#include "dlist.h"
#include "vdc263.h" /* for vdxPictureHeader_t and vdxSEI_t */
class CMPEG4Transcoder;


/*
 * Defines
 */

/* General return values */
// unify error codes
#define VDX_OK_BUT_BIT_ERROR  H263D_OK_BUT_BIT_ERROR     /* Handled portion of the bistream
                                       contains a bit error */
#define VDX_OK         H263D_OK     /* Everything ok */
#define VDX_ERR     H263D_ERROR     /* Everything ok */
#define VDX_ERR_NOT_SUPPORTED H263D_ERROR    /* Some bit combination (in header)
                                       occurs which is not supported by the 
                                       implementetion */
#define VDX_ERR_BIB  H263D_ERROR    /* Bit Buffer module returned an error */

/* Picture types */
#define VDX_PIC_TYPE_I     0
#define VDX_PIC_TYPE_P     1
#define VDX_PIC_TYPE_PB    8
#define VDX_PIC_TYPE_IPB   2
#define VDX_PIC_TYPE_B     3
#define VDX_PIC_TYPE_EI    4
#define VDX_PIC_TYPE_EP    5
#define VDX_VOP_TYPE_I     0 /* MPEG-4 Intra Picture */
#define VDX_VOP_TYPE_P     1 /* MPEG-4 Inter Picture */
#define VDX_VOP_NOT_CODED -1 /* MPEG-4 Not Coded Picture 
                                (only the time_increment is present) */

/* Reference Picture Selection modes */
#define VDX_RPS_MODE_NO_MSGS     0
#define VDX_RPS_MODE_ACK         1
#define VDX_RPS_MODE_NACK        2
#define VDX_RPS_MODE_BOTH_MSGS   3

/* Macroblock classes */
#define VDX_MB_INTER    1
#define VDX_MB_INTRA    2

#define MAX_USER_DATA_LENGTH 512    /* The maximum allocated memory for 
                                       user data (UD). Only this much of the UD
                                       from the bitstream is stored */

/*
 * Typedefs
 */

/*
 * vdxVolHeader_t
 */

/* {{-output"vdxVolHeader_t_info.txt" -ignore"*" -noCR}}
   The vdxGetVolHeader function uses this structure to return VOL header data.
   /MPEG-4/
   {{-output"vdxVolHeader_t_info.txt"}} */

/* {{-output"vdxVolHeader_t.txt"}} */
typedef struct {

   int profile_level;       /* Indicates the Level (1,2 or 3) of the Simple
                               Profile the Video Object conforms to */
   int vo_priority;         /* Priority assigned to the Video Object 
                               a value between 1(lowest)-7(highest).
                               If not in the bitstream, set to zero */
   
   int vo_id;               /* id of the Video Object */
   int vol_id;              /* id of the Video Object Layer */
   int random_accessible_vol; 
                            /* set to 1 if all the VOPs in the stream are
                                 I-VOP. */
   int pixel_aspect_ratio;  /* see. MPEG-4 visual spec. pp. 71 */

   int time_increment_resolution;
                            /* Resolution to interpret the time_increment 
                               fields in the VOP headers */

   int lumWidth;            /* Frame width of the Y component in pixels */
   int lumHeight;           /* Frame height of the Y component in pixels */

   u_char error_res_disable;/* Flag ON if no resynchronization markers are
                               used inside frames. 
                               When OFF it doesn't mean they ARE used. */
   u_char data_partitioned; /* Flag indicating if data partitioning inside 
                               a VP is used or not. */
   u_char reversible_vlc;   /* flag indicating the usage of reversible 
                               VLC codes */

   /* the following parameters concern the input video signal,
      see MPEG-4 visual spec. pp. 66-70, and "Note" in viddemux_mpeg.c 
      Not used in the current implementatnion. */
   int video_format;
   int video_range;
   int colour_primaries;    
   int transfer_characteristics;
   int matrix_coefficients;

   /* the following parameters are used in the Video Buffering Verifier
      (Annex D) to monitor the input bit buffer, complexity, memory buffer
      used in the decoding process. The conformance of a stream can be checked
      using these parameters. 
      Not used in the current implementatnion. */
   u_int32 bit_rate;
   u_int32 vbv_buffer_size;
   u_int32 vbv_occupancy;

   char *user_data;         /* User Data if available */
   int user_data_length;    /* Length of the recieved user data */
   u_char fixed_vop_rate;       /* fixed vop rate indication, added for transcoding */
} vdxVolHeader_t;
/* {{-output"vdxVolHeader_t.txt"}} */

/*
 * vdxGovHeader_t
 */

/* {{-output"vdxGovHeader_t_info.txt" -ignore"*" -noCR}}
   The vdxGetGovHeader function uses this structure to return GOV header data.
   /MPEG-4/
   {{-output"vdxGovHeader_t_info.txt"}} */

/* {{-output"vdxGovHeader_t.txt"}} */
typedef struct {
   int time_stamp;              /* The time stamp value in the GOV Header */

   u_char closed_gov;           /* only important if B-VOPs can be in the stream */
   u_char broken_link;          /* only important if B-VOPs can be in the stream */

   char *user_data;             /* User Data if available */
   int user_data_length;        /* Length of the recieved user data */
} vdxGovHeader_t;
/* {{-output"vdxGovHeader_t.txt"}} */


/*
 * vdxGetVopHeaderInputParam_t
 */

/* {{-output"vdxGetVopHeaderInputParam_t_info.txt" -ignore"*" -noCR}}
   This structure is used to pass input parameters to
   the vdxGetVopHeader function. /MPEG-4/
   {{-output"vdxGetVopHeaderInputParam_t_info.txt"}} */

/* {{-output"vdxGetVopHeaderInputParam_t.txt"}} */
typedef struct {

    int time_increment_resolution; /* resolution of the time increment field */

} vdxGetVopHeaderInputParam_t;
/* {{-output"vdxGetVopHeaderInputParam_t.txt"}} */


/*
 * vdxVopHeader_t
 */

/* {{-output"vdxVopHeader_t_info.txt" -ignore"*" -noCR}}
   The vdxGetVopHeader function uses this structure to return the picture
   header parameters. /MPEG-4/
   {{-output"vdxVopHeader_t_info.txt"}} */

/* {{-output"vdxVopHeader_t.txt"}} */
typedef struct {
    int time_base_incr;         /* Increment of the modulo time base from the
                                   previous VOP or GOV header in seconds */
    int time_inc;             /* Time increment value of the current VOP,
                                   plus the modulo time base = absolute time */

    u_char vop_coded;           /* Flag: 1 - VOP coded;
                                         0 - VOP not coded/skipped */
    int coding_type;            /* VOP coding type is one of the following:
                                       VDX_VOP_TYPE_I       MPEG-4 INTRA
                                       VDX_VOP_TYPE_P       MPEG-4 INTER */

    int rounding_type;          /* Rounding of the sub-pixel predictor
                                   calculations: 0 or 1. */

    int intra_dc_vlc_thr;       /* QP dependent switch control of coding the
                                   Intra DC coefficient as separate/optimized or
                                   as the other Intra AC. (0..7) */
    int quant;                  /* Initial Quantizer value */

    int fcode_forward;          /* For P-VOPs Motion Vector range control
                                   1: [-16,16] .. 7:[-1024,1024] */
} vdxVopHeader_t;
/* {{-output"vdxVopHeader_t.txt"}} */


/*
 * vdxGetVideoPacketHeaderInputParam_t
 */

/* {{-output"vdxGetVideoPacketHeaderInputParam_t_info.txt" -ignore"*" -noCR}}
   This structure is used to pass input parameters to
   the vdxGetVideoPacketHeader function. /MPEG-4/
   {{-output"vdxGetVideoPacketHeaderInputParam_t_info.txt"}} */

/* {{-output"vdxGetVideoPacketHeaderInputParam_t.txt"}} */
typedef struct {

    int fcode_forward;          /* used for determining the resync_marker
                                   length */
    int time_increment_resolution; /* resolution of the time increment field */

    int numOfMBs;               /* Number of MBs in a VOP */

} vdxGetVideoPacketHeaderInputParam_t;
/* {{-output"vdxGetVideoPacketHeaderInputParam_t.txt"}} */


/*
 * vdxVideoPacketHeader_t
 */

/* {{-output"vdxVideoPacketHeader_t_info.txt" -ignore"*" -noCR}}
   The vdxGetVideoPacketHeader function uses this structure to return the
   Number of the immediately following MB in the VOP and optionally confirm
   header parameters. /MPEG-4/
   {{-output"vdxVideoPacketHeader_t_info.txt"}} */

/* {{-output"vdxVideoPacketHeader_t.txt"}} */
typedef struct {

    int currMBNum;              /* Number of the immediately following MB */
    int quant;                  /* Quantizer used for the following MB */

    u_char fHEC;                /* Flag for header extension code */

    int time_base_incr;         /* Increment of the modulo time base from the
                                   previous VOP or GOV header in seconds */
    int time_inc;             /* Time increment value of the current VOP,
                                   plus the modulo time base = absolute time */

    int coding_type;            /* VOP coding type is one of the following:
                                       VDX_VOP_TYPE_I       MPEG-4 INTRA
                                       VDX_VOP_TYPE_P       MPEG-4 INTER */

    int intra_dc_vlc_thr;       /* QP dependent switch control of coding the
                                   Intra DC coefficient as separate/optimized or
                                   as the other Intra AC. (0..7) */

    int fcode_forward;          /* For P-VOPs Motion Vector range control
                                   1: [-16,16] .. 7:[-1024,1024] */
} vdxVideoPacketHeader_t;
/* {{-output"vdxVideoPacketHeader_t.txt"}} */


/*
 * vdxGetPictureHeaderInputParam_t
 */

/* {{-output"vdxGetPictureHeaderInputParam_t_info.txt" -ignore"*" -noCR}}
   This structure is used to pass input parameters to
   the vdxGetPictureHeader function.
   {{-output"vdxGetPictureHeaderInputParam_t_info.txt"}} */

/* {{-output"vdxGetPictureHeaderInputParam_t.txt"}} */
typedef struct {
   int numStuffBits;             /* Number of stuffing bits before PSC */

   int fCustomPCF;               /* Non-zero if custom picture clock frequency
                                    is allowed. Otherwise zero. */

   int fScalabilityMode;         /* Non-zero if the Temporal, SNR, and Spatial
                                    Scalability mode (Annex O) is in use.
                                    Otherwise zero. */

   int fRPS;                     /* Non-zero if the Refence Picture Selection
                                    mode (Annex N) is in use. Otherwise zero. */

   bibFlushBits_t flushBits;     /* Pointers for bit buffer functions. */
   bibGetBits_t getBits;         /* Needed to provide a possibly different */
   bibShowBits_t showBits;       /* input mechanism for normal reading from
                                    incoming bit-stream and for parsing
                                    a picture header copy */
} vdxGetPictureHeaderInputParam_t;
/* {{-output"vdxGetPictureHeaderInputParam_t.txt"}} */


/*
 * vdxPictureHeader_t
 * Defined in vdc263.h because Video Decoder Core exports the data structure.
 */


/*
 * vdxSEI_t
 * Defined in vdc263.h because Video Decoder Core exports the data structure.
 */


/*
 * vdxGetGOBHeaderInputParam_t
 */

/* {{-output"vdxGetGOBHeaderInputParam_t_info.txt" -ignore"*" -noCR}}
   This structure is used to pass input parameters to
   the vdxGetGOBHeader function.
   {{-output"vdxGetGOBHeaderInputParam_t_info.txt"}} */

/* {{-output"vdxGetGOBHeaderInputParam_t.txt"}} */
typedef struct {
   int numStuffBits;             /* Number of stuffing bits before PSC */

   int fCustomPCF;               /* Non-zero if custom picture clock frequency
                                    is allowed. Otherwise zero. */

   int fCPM;                     /* Non-zero if the Continuous Presence
                                    Multipoint feature (Annex C) is in use.
                                    Otherwise zero. */

   int fRPS;                     /* Non-zero if the Refence Picture Selection
                                    mode (Annex N) is in use. Otherwise zero. */
} vdxGetGOBHeaderInputParam_t;
/* {{-output"vdxGetGOBHeaderInputParam_t.txt"}} */


/*
 * vdxGOBHeader_t
 */

/* {{-output"vdxGOBHeader_t_info.txt" -ignore"*" -noCR}}
   The vdxGetGOBHeader function uses this structure to return the GOB
   header parameters.
   {{-output"vdxGOBHeader_t_info.txt"}} */

/* {{-output"vdxGOBHeader_t.txt"}} */
typedef struct {
   int gn;                       /* Group Number (GN) field */

   int gfid;                     /* GOB Frame ID (GFID) field */

   int gquant;                   /* Quantizer Information (GQUANT) field */

   /* If CPM (Annex C) is in use */
      int gsbi;                  /* GOB Sub-Bitstream Indicator (GSBI) field */

   /* If the Reference Picture Selection mode is in use */
      int tri;                   /* Temporal Reference Indicator (TRI) field */
      int trpi;                  /* Temporal Reference for Prediction
                                    Indicator (TRPI) field */

      /* If tri == 1 */
         int tr;                 /* Temporal Reference (TR) field */

      /* If trpi == 1 */
         int trp;                /* Temporal Reference for Prediction (TRP) 
                                    field */
} vdxGOBHeader_t;
/* {{-output"vdxGOBHeader_t.txt"}} */


/*
 * vdxGetSliceHeaderInputParam_t
 */

/* {{-output"vdxGetSliceHeaderInputParam_t_info.txt" -ignore"*" -noCR}}
   This structure is used to pass input parameters to
   the vdxGetSliceHeader function.
   {{-output"vdxGetSliceHeaderInputParam_t_info.txt"}} */

/* {{-output"vdxGetSliceHeaderInputParam_t.txt"}} */
typedef struct {
   int sliceHeaderAfterPSC;      /* If the slice follows a PSC, the value 
                                    should be 1 */ 

   int numStuffBits;             /* Number of stuffing bits before SSC */

   int fCPM;                     /* Non-zero if the Continuous Presence
                                    Multipoint feature (Annex C) is in use.
                                    Otherwise zero. */

   int fRPS;                     /* Non-zero if the Refence Picture Selection
                                    mode (Annex N) is in use. Otherwise zero. */

   int mbaFieldWidth;            /* MBA Field width */
   int mbaMaxValue;              /* MBA maximum value. See Table K.2/H.263 */

   int fRectangularSlices;       /* 0 = free-running slices, 
                                    1 = rectangular slices */
   
   int swiFieldWidth;            /* SWI Field width */
   int swiMaxValue;              /* SWI maximum value. See Table K.3/H.263 */
} vdxGetSliceHeaderInputParam_t;
/* {{-output"vdxGetSliceHeaderInputParam_t.txt"}} */

/*
 * vdxSliceHeader_t
 */

/* {{-output"vdxSliceHeader_t_info.txt" -ignore"*" -noCR}}
   The vdxGetSliceHeader function uses this structure to return the Slice
   header parameters.
   {{-output"vdxSliceHeader_t_info.txt"}} */

/* {{-output"vdxSliceHeader_t.txt"}} */
typedef struct {
   /* If CPM (Annex C) is in use */
      int ssbi;                   /* Slice Sub-Bitstream Indicator (SSBI) */

      int gn;                    /* Group Number (GN) field */

      int sbn;                   /* Sub-Bitstream number */

   int mba;                      /* Macroblock Address (MBA) */

   /* If not the slice just after the PSC */
      int squant;                /* Quantizer Inforation (SQUANT) */

   /* If fRectangularSlices */
      int swi;                   /* Slice Width Indication in Macroblocks (SWI)*/
      
   int gfid;                     /* GOB Frame ID (GFID) field */
   /* If the Reference Picture Selection mode is in use */
      int tri;                   /* Temporal Reference Indicator (TRI) field */
      int trpi;                  /* Temporal Reference for Prediction
                                    Indicator (TRPI) field */

      /* If tri == 1 */
         int tr;                 /* Temporal Reference (TR) field */

      /* If trpi == 1 */
         int trp;                /* Temporal Reference for Prediction (TRP) 
                                    field */

} vdxSliceHeader_t;
/* {{-output"vdxSliceHeader_t.txt"}} */

/*
 * vdxGetIMBLayerInputParam_t
 */

/* {{-output"vdxGetIMBLayerInputParam_t_info.txt" -ignore"*" -noCR}}
   This structure is used to pass input parameters to
   the vdxGetIMBLayer function.
   {{-output"vdxGetIMBLayerInputParam_t_info.txt"}} */

/* {{-output"vdxGetIMBLayerInputParam_t.txt"}} */
typedef struct {
   int fMQ;                      /* Modified Quantization */
   int quant;                    /* Current QUANT */
   int fAIC;                     /* Advanced Intra Coding Flag */
   int fSS;                      /* Slice Structured Mode Flag */
   u_char fMPEG4;                /* MPEG4 or H.263 */
} vdxGetIMBLayerInputParam_t;
/* {{-output"vdxGetIMBLayerInputParam_t.txt"}} */


/*
 * vdxGetDataPartitionedIMBLayerInputParam_t
 */

/* {{-output"vdxGetDataPartitionedIMBLayerInputParam_t_info.txt" -ignore"*" -noCR}}
   This structure is used to pass input parameters to
   the vdxGetDataPartitionedIMBLayer function. /MPEG-4/
   {{-output"vdxGetDataPartitionedIMBLayerInputParam_t_info.txt"}} */

/* {{-output"vdxGetDataPartitionedIMBLayerInputParam_t.txt"}} */
typedef struct {
   int intra_dc_vlc_thr;
   int quant;                    /* Current QUANT */
} vdxGetDataPartitionedIMBLayerInputParam_t;
/* {{-output"vdxGetDataPartitionedIMBLayerInputParam_t.txt"}} */


/*
 * vdxIMBLayer_t
 */

/* {{-output"vdxIMBLayer_t_info.txt" -ignore"*" -noCR}}
   The vdxGetIMBLayer function uses this structure to return
   the macroblock header parameters.
   {{-output"vdxIMBLayer_t_info.txt"}} */

/* {{-output"vdxIMBLayer_t.txt"}} */
typedef struct {
	int mcbpc;                     /* Macroblock type and the coded block 
																    pattern for chrominance */

   int cbpc;                     /* Coded Block Pattern for Chrominance,
                                    bit 1 is for U and bit 0 is for V,
                                    use vdxIsUCoded and vdxIsVCoded macros 
                                    to access this value */

   int cbpy;                     /* Coded Block Pattern for Luminance,
                                    bit 3 is for block 1, ..., 
                                    bit 0 is for block 4,
                                    use vdxIsYCoded macro to access this 
                                    value */

/*** MPEG-4 REVISION ***/
   u_char ac_pred_flag;          /* "1" AC prediction used, "0" not */
/*** End MPEG-4 REVISION ***/

   int quant;                    /* New (possibly updated) QUANT */

   int predMode;                 /* Prediction Mode used in Annex I
                                    0 (DC Only)
                                    1 (Vertical DC&AC)
                                    2 (Horizontal DC&AC) */
   
   int mbType;                   /* 0..5, see table 9/H.263 */

} vdxIMBLayer_t;
/* {{-output"vdxIMBLayer_t.txt"}} */


/*
 * vdxGetPPBMBLayerInputParam_t
 */

/* {{-output"vdxGetPPBMBLayerInputParam_t_info.txt" -ignore"*" -noCR}}
   This structure is used to pass input parameters to
   the vdxGetPPBMBLayer function.
   {{-output"vdxGetPPBMBLayerInputParam_t_info.txt"}} */

/* {{-output"vdxGetPPBMBLayerInputParam_t.txt"}} */
typedef struct {  
   int pictureType;              /* Picture type is one of the following:
                                       VDX_PIC_TYPE_P       INTER
                                       VDX_PIC_TYPE_PB      PB (Annex G)
                                       VDX_PIC_TYPE_IPB     Improved PB */

   /* See the description of vdxPictureHeader_t for the following flags. */
   int fPLUSPTYPE;
   int fUMV;
   int fAP;
   int fDF;
   int fMQ;

   int fCustomSourceFormat;      /* Flag indicating if custom source format 
                                    is used */


/*** MPEG-4 REVISION ***/
   u_char fMPEG4;                /* MPEG4 or H.263 */
   int f_code;
/*** End MPEG-4 REVISION ***/

   int quant;                    /* Current QUANT */

   int fFirstMBOfPicture;        /* Non-zero if the macroblock is the first
                                    (top-left) one of the picture */

   int fAIC;                     /* Advanced Intra Coding Flag */
} vdxGetPPBMBLayerInputParam_t;
/* {{-output"vdxGetPPBMBLayerInputParam_t.txt"}} */


/*
 * vdxGetDataPartitionedPMBLayerInputParam_t
 */

/* {{-output"vdxGetDataPartitionedPMBLayerInputParam_t_info.txt" -ignore"*" -noCR}}
   This structure is used to pass input parameters to
   the vdxGetDataPartitionedPMBLayer function. /MPEG-4/
   {{-output"vdxGetDataPartitionedPMBLayerInputParam_t_info.txt"}} */

/* {{-output"vdxGetDataPartitionedPMBLayerInputParam_t.txt"}} */
typedef struct {
   int intra_dc_vlc_thr;
   int quant;                    /* Current QUANT */
   int f_code;
} vdxGetDataPartitionedPMBLayerInputParam_t;
/* {{-output"vdxGetDataPartitionedPMBLayerInputParam_t.txt"}} */


/*
 * vdxPPBMBLayer_t
 */

/* {{-output"vdxPPBMBLayer_t_info.txt" -ignore"*" -noCR}}
   The vdxGetPPBMBLayer function uses this structure to return
   the macroblock header parameters.
   {{-output"vdxPPBMBLayer_t_info.txt"}} */

/* {{-output"vdxPPBMBLayer_t.txt"}} */
typedef struct {
	 int mcbpc;                    /* Macroblock type and the coded block 
																    pattern for chrominance */

	 int fCodedMB;                 /* 1 if the macroblock is coded. 
                                    Otherwise 0. */

   int mbType;                   /* 0..5, see table 9/H.263 */

   int mbClass;                  /* VDX_MB_INTRA or VDX_MB_INTER */

   int cbpc;                     /* Coded Block Pattern for Chrominance,
                                    bit 1 is for U and bit 0 is for V,
                                    use vdxIsUCoded and vdxIsVCoded macros 
                                    to access this value */

   int cbpy;                     /* Coded Block Pattern for Luminance,
                                    bit 3 is for block 1, ..., 
                                    bit 0 is for block 4,
                                    use vdxIsYCoded macro to access this 
                                    value */

   int cbpb;                     /* Coded Block Pattern for B-blocks,
                                    bit 5 is for block 1, ..., 
                                    bit 0 is for block 6 */

/*** MPEG-4 REVISION ***/
   u_char ac_pred_flag;          /* "1" AC prediction used, "0" not */
/*** End MPEG-4 REVISION ***/

   int quant;                    /* New (possibly updated) QUANT */
                           
   int numMVs;                   /* Number of motion vectors for the MB,
                                    0 if MVD is not in the stream,
                                    1 if MVD but not MVD2-4 is in the stream,
                                    4 if MVD and MVD2-4 is in the stream */

   /* Only the number of MVs indicated in the numMVs member is valid
      in the following arrays. */
   int mvdx[4];                  /* Horizontal components of MVD1-4,
                                    index to table 14/H.263 */
   int mvdy[4];                  /* Vertical components of MVD1-4,
                                    index to table 14/H.263 */

   int fMVDB;                    /* 1 = MVDB exists in the bitstream,
                                    i.e. mvdbx and mvdby are valid */

   int mvdbx;                    /* Horizontal components of MVDB,
                                    index to table 14/H.263 */
   int mvdby;                    /* Vertical components of MVDB,
                                    index to table 14/H.263 */
   int predMode;                 /* Prediction Mode used in Annex I
                                    0 (DC Only)
                                    1 (Vertical DC&AC)
                                    2 (Horizontal DC&AC) */
   int IPBPredMode;              /* Prediction Modes used in annex M
                                    0  Bidirectional prediction
                                    1  Forward prediction
                                    2  Backward prediction  */
} vdxPPBMBLayer_t;
/* {{-output"vdxPPBMBLayer_t.txt"}} */


/*
 * vdxIMBListItem_t
 */

/* {{-output"vdxIMBListItem_t_info.txt" -ignore"*" -noCR}}
   List Item which is used to build a queue of Intra MBs during the data
   partitioned decoding of an I-VOP. /MPEG-4/
   The vdxGetDataPartitionedIMBLayer function uses this structure.
   {{-output"vdxIMBListItem_t_info.txt"}} */

/* {{-output"vdxIMBListItem_t.txt"}} */


/* Data item recording for data partitioned transcoding */
#ifndef VDT_OPERATION
#define VDT_OPERATION
#define VDT_SET_START_POSITION(mb,item,byte,bit) \
{\
	(mb)->DataItemStartByteIndex[(item)] = (byte);\
     (mb)->DataItemStartBitIndex[(item)] = (bit);\
}

#define VDT_SET_END_POSITION(mb,item,byte,bit) \
{\
	(mb)->DataItemEndByteIndex[(item)] = (byte);\
     (mb)->DataItemEndBitIndex[(item)] = (bit);\
}
#endif



typedef struct {
   DLST_ITEM_SKELETON

/* for data partitioned transcoding
   array ordering: 0: mcbpc, 1: dquant, 2: cbpy, 3: ac_pred_flag, 4~9: intraDCs, 10: mv, 11: mb stuffing
 */
   int mcbpc;
   int DataItemStartByteIndex[12];
   int DataItemStartBitIndex[12];
   int DataItemEndByteIndex[12];
   int DataItemEndBitIndex[12];
   int dquant;

   int DC[6];                 /* DC component */
   int quant;                 /* QUANT */
   int cbpc;                  /* Coded Block Pattern for Chrominance */
   int cbpy;                  /* Coded Block Pattern for Luminance */
   u_char ac_pred_flag;       /* "1" AC prediction used, "0" not */
   u_char switched;           /* intra_dc_vlc_thr indicated switch 
                                 to Intra AC VLC coding of the DC coeff
                                 intstead of the usual DC VLC coding 
                                 at this QP */
} vdxIMBListItem_t;
/* {{-output"vdxIMBListItem_t.txt"}} */


/*
 * vdxPMBListItem_t
 */

/* {{-output"vdxPMBListItem_t_info.txt" -ignore"*" -noCR}}
   List Item which is used to build a queue of Inter MBs during the data
   partitioned decoding of a P-VOP. /MPEG-4/
   The vdxGetDataPartitionedPMBLayer function uses this structure.
   {{-output"vdxPMBListItem_t_info.txt"}} */

/* {{-output"vdxPMBListItem_t.txt"}} */
typedef struct {
   DLST_ITEM_SKELETON
/* for data partitioned transcoding
   array ordering: 0: mcbpc, 1: dquant, 2: cbpy, 3: ac_pred_flag, 4~9: intraDCs, 10: mv, 11: mb stuffing
 */
   int mcbpc;
   int DataItemStartByteIndex[12];
   int DataItemStartBitIndex[12];
   int DataItemEndByteIndex[12];
   int DataItemEndBitIndex[12];
   int dquant;
   int mv_x[4];                /* horizontal componenet of 1-4 MVs */ 
   int mv_y[4];                /* vertical componenet of 1-4 MVs */

   u_char fCodedMB;           /* 1 if the macroblock is coded. 
                                    Otherwise 0. */
   int mbType;                /* 0..5, see table 9/H.263 */
   int mbClass;               /* VDX_MB_INTRA or VDX_MB_INTER */
   int DC[6];                 /* DC component */
   int quant;                 /* QUANT */
   int cbpc;                  /* Coded Block Pattern for Chrominance */
   int cbpy;                  /* Coded Block Pattern for Luminance */
   u_char ac_pred_flag;       /* "1" AC prediction used, "0" not */
   u_char switched;           /* intra_dc_vlc_thr indicated switch 
                                 to Intra AC VLC coding of the DC coeff
                                 intstead of the usual DC VLC coding 
                                 at this QP */
   int numMVs;                /* Number of motion vectors for the MB */
   int mvx[4];                /* horizontal componenet of 1-4 MVs */ // actually it is dMV
   int mvy[4];                /* vertical componenet of 1-4 MVs */
} vdxPMBListItem_t;
/* {{-output"vdxPMBListItem_t.txt"}} */


/*
 * Macros
 */

/* These arrays are intialized in viddemux.c and needed in the vdxIsYXCoded 
   macros. */
extern const int vdxBlockIndexToCBPYMask[5];
extern const int vdxYBlockIndexToCBPBMask[5];

/* {{-output"vdxIsXCoded.txt"}} */
/*
 * vdxIsYCoded
 * vdxIsUCoded
 * vdxIsVCoded
 * vdxIsYBCoded
 * vdxIsUBCoded
 * vdxIsVBCoded
 *
 * Parameters:
 *    for vdxIsYCoded:
 *       cbpy                    coded block for luminance
 *       blockIndex              index for luminance block 1..4
 *
 *    for vdxIsUCoded and vdxIsVCoded:
 *       cbpc                    coded block pattern for chrominance
 *
 *    for vdxIsXBCoded:
 *       cbpb                    coded block pattern for B blocks
 *       blockIndex              index for B luminance block 1..4
 *
 * Function:
 *    This macros access the coded block bit patterns.
 *
 * Returns:
 *    0 if the requested block is not coded
 *    non-zero if the requested block is coded
 *
 */

#define vdxIsYCoded(cbpy, blockIndex) \
   ((cbpy) & vdxBlockIndexToCBPYMask[(blockIndex)])

#define vdxIsUCoded(cbpc) ((cbpc) & 2)
#define vdxIsVCoded(cbpc) ((cbpc) & 1)

#define vdxIsYBCoded(cbpb, blockIndex) \
   ((cbpb) & vdxYBlockIndexToCBPBMask[(blockIndex)])

#define vdxIsUBCoded(cbpb) ((cbpb) & 2)
#define vdxIsVBCoded(cbpb) ((cbpb) & 1)
/* {{-output"vdxIsXCoded.txt"}} */


/*
 * Function prototypes
 */

/* Picture Layer Global Functions */
class CMPEG4Transcoder;

int vdxGetVolHeader(
   bibBuffer_t *inBuffer,
   vdxVolHeader_t *header,
   int *bitErrorIndication,
   int getInfo, int *aByteIndex, int *aBitIndex,
   CMPEG4Transcoder *hTranscoder);

int vdxGetGovHeader(
   bibBuffer_t *inBuffer,
   vdxGovHeader_t *header,
   int *bitErrorIndication);

int vdxGetVopHeader(
   bibBuffer_t *inBuffer,
   const vdxGetVopHeaderInputParam_t *inpParam,
   vdxVopHeader_t *header,
   int * ModuloByteIndex,
   int * ModuloBitIndex,
   int * TimeIncByteIndex,
   int * TimeIncBitIndex,
   int *bitErrorIndication);

int vdxGetPictureHeader(
   bibBuffer_t *inBuffer, 
   const vdxGetPictureHeaderInputParam_t *inpParam,
   vdxPictureHeader_t *header,
   int *bitErrorIndication);

int vdxFlushSEI(
   bibBuffer_t *inBuffer,
   int *bitErrorIndication);

int vdxGetSEI(
   bibBuffer_t *inBuffer,
   int *ftype,
   int *dsize,
   u_char *parameterData,
   int *fLast,
   int *bitErrorIndication);

int vdxGetAndParseSEI(
   bibBuffer_t *inBuffer,
   int fPLUSPTYPE,
   int numScalabilityLayers,
   vdxSEI_t *sei,
   int *bitErrorIndication);

int vdxGetUserData(bibBuffer_t *inBuffer,
   char *user_data, int *user_data_length,
   int *bitErrorIndication);

/* GOB/Video Packet Layer Global Functions */

int vdxGetGOBHeader(
   bibBuffer_t *inBuffer, 
   const vdxGetGOBHeaderInputParam_t *inpParam,
   vdxGOBHeader_t *header,
   int *bitErrorIndication,
   int aColorEffect,
   int* aStartByteIndex,
   int* aStartBitIndex,
   CMPEG4Transcoder *hTranscoder);

int vdxGetVideoPacketHeader(
   bibBuffer_t *inBuffer,
   const vdxGetVideoPacketHeaderInputParam_t *inpParam,
   vdxVideoPacketHeader_t *header,
   int *bitErrorIndication);

/* Slice Layer Global Functions*/

int vdxGetSliceHeader(
   bibBuffer_t *inBuffer, 
   const vdxGetSliceHeaderInputParam_t *inpParam,
   vdxSliceHeader_t *header,
   int *bitErrorIndication);

void vdxGetMBAandSWIValues(
   int width,
   int height,
   int fRRU,
   int *mbaFieldWidth,
   int *mbaMaxValue,
   int *swiFieldWidth,
   int *swiMaxValue);

/* Macroblock Layer Global Functions */


int vdxGetIMBLayer(
   bibBuffer_t *inBuffer, 
	 bibBuffer_t *outBuffer, 
   bibBufferEdit_t *bufEdit,
   int aColorEffect, 
	 int *StartByteIndex, 
	 int *StartBitIndex, 
   TBool aGetDecodedFrame, 
   const vdxGetIMBLayerInputParam_t *inpParam,
   vdxIMBLayer_t *outParam,
   int *bitErrorIndication,
   CMPEG4Transcoder *hTranscoder);


int vdxGetPPBMBLayer(
   bibBuffer_t *inBuffer, 
   bibBuffer_t *outBuffer, 
   bibBufferEdit_t *bufEdit,
   int aColorEffect,
	 int *aStartByteIndex, 
	 int *aStartBitIndex, 
   TBool aGetDecodedFrame, 
   int *bwMBType, 
   const vdxGetPPBMBLayerInputParam_t *inpParam,
   vdxPPBMBLayer_t *outParam,
   int *bitErrorIndication,
   CMPEG4Transcoder *hTranscoder);


int vdxGetDataPartitionedIMBLayer_Part1(
   bibBuffer_t *inBuffer, bibBuffer_t *outBuffer, bibBufferEdit_t *bufEdit, 
	 int aColorEffect, int *aStartByteIndex, int *aStartBitIndex, 
	 CMPEG4Transcoder *hTranscoder, 
   const vdxGetDataPartitionedIMBLayerInputParam_t *inpParam,
   dlst_t *MBList,
   int *bitErrorIndication);

int vdxGetDataPartitionedIMBLayer_Part2(
   bibBuffer_t *inBuffer, bibBuffer_t *outBuffer, bibBufferEdit_t *bufEdit, 
	 int aColorEffect, int *aStartByteIndex, int *aStartBitIndex, 
   dlst_t *MBList,
   int numMBsInVP,
   int *bitErrorIndication);

int vdxGetDataPartitionedPMBLayer_Part1(
   bibBuffer_t *inBuffer, bibBuffer_t *outBuffer, bibBufferEdit_t *bufEdit, 
	 int aColorEffect, int *aStartByteIndex, int *aStartBitIndex, 
   const vdxGetDataPartitionedPMBLayerInputParam_t *inpParam,
   dlst_t *MBList,
   int *bitErrorIndication);

int vdxGetDataPartitionedPMBLayer_Part2(
   bibBuffer_t *inBuffer,
   bibBuffer_t *outBuffer, 
   bibBufferEdit_t *bufEdit,
   int aColorEffect, int *aStartByteIndex, int *aStartBitIndex, 
   CMPEG4Transcoder *hTranscoder, 
   const vdxGetDataPartitionedPMBLayerInputParam_t *inpParam,
   dlst_t *MBList,
   int *bitErrorIndication);

/* Block Layer Global Functions */

int vdxGetIntraDCTBlock(
   bibBuffer_t *inBuffer, 
   int fCodedBlock,
   int *block,
   int *bitErrorIndication,
   int fMQ,
   int qp);

int vdxGetIntraDC(
   bibBuffer_t *inBuffer,
	 bibBuffer_t *outBuffer, 
   bibBufferEdit_t *bufEdit,
   int aColorEffect, 
	 int *aStartByteIndex, 
	 int *aStartBitIndex,    
	 int compnum,
	 int *IntraDCSize, 
   int *IntraDCDelta, 
	 int *bitErrorIndication);

int vdxGetMPEGIntraDCTBlock(
   bibBuffer_t *inBuffer, 
   int startIndex,
   int *block,
   int *bitErrorIndication);

int vdxGetDCTBlock(
   bibBuffer_t *inBuffer,
   int startIndex,
   u_char fMPEG4EscapeCodes,
   int *block,
   int *bitErrorIndication,
   int fMQ,
   int qp,
   int *fEscapeCodeUsed);

int vdxGetAdvIntraDCTBlock(
   bibBuffer_t *inBuffer, 
   int fCodedBlock,
   int *block,
   int *bitErrorIndication,
   int predMode,
   int fMQ,
   int qp);

int vdxGetAdvDCTBlock(
   bibBuffer_t *inBuffer, 
   int startIndex,
   int *block,
   int *bitErrorIndication,
   int predMode,
   int fMQ,
   int qp);

int vdxGetRVLCDCTBlock(
   bibBuffer_t *inBuffer, 
   int startIndex,
   int fIntraBlock,
   int *block,
   int *bitErrorIndication);

int vdxGetRVLCDCTBlockBackwards(
   bibBuffer_t *inBuffer, 
   int startIndex,
   int fIntraBlock,
   int *block,
   int *bitErrorIndication);



int vdxChangeBlackAndWhiteHeaderIntraIMB(bibBufferEdit_t *bufEdit, 
																				 int mcbpcIndex,
																				 int StartByteIndex, 
																				 int StartBitIndex);

int vdxChangeBlackAndWhiteHeaderInterPMB(bibBufferEdit_t *bufEdit, 
																				 int mcbpcIndex,
																				 int StartByteIndex, 
																				 int StartBitIndex);



#endif
// End of File
