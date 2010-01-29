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
* Internal header for the core module of the Video Decoder Core.
*
*/


#ifndef _CORE_H_
#define _CORE_H_

#include "vdcmvc.h"
#include "vdcaic.h"
#include "vdeims.h"
#include "viddemux.h"

#include "biblin.h"


/*
 * Defines
 */

#define vdcMalloc malloc
#define vdcCalloc calloc
#define vdcRealloc realloc
#define vdcDealloc free

#ifndef vdcAssert
#define vdcAssert(exp) assert(exp);
#endif


/*
 * Structs and typedefs
 */

/* {{-output"vdcPictureParam_t_info.txt" -ignore"*" -noCR}}
   vdcPictureParam_t is used to store the attributes for one picture.
   {{-output"vdcPictureParam_t_info.txt"}} */

/* {{-output"vdcPictureParam_t.txt"}} */
typedef struct {
   int tr;                    /* the TR field or a combination of ETR and TR
                                 if ETR is used */

   int trd;                   /* Difference between present and previous tr */

   int prevTR;                /* TR of the previous frame */

   /* The following three fields correspond to the flags in the PTYPE field */
   int fSplitScreenIndicator;
   int fDocumentCameraIndicator;
   int fFullPictureFreezeRelease;

   int pictureType;           /* Picture type is one of the following:
                                    VDX_PIC_TYPE_I       INTRA
                                    VDX_PIC_TYPE_P       INTER
                                    VDX_PIC_TYPE_PB      PB (Annex G)
                                    VDX_PIC_TYPE_IPB     Improved PB
                                    VDX_PIC_TYPE_B       B (Annex O)
                                    VDX_PIC_TYPE_EI      EI (Annex O)
                                    VDX_PIC_TYPE_EP      EP (Annex O) */

   int fPLUSPTYPE;            /* 0 = no PLUSPTYPE, 1 = PLUSPTYPE exists */

   int cpm;                   /* The CPM field */
   int psbi;                  /* The PSBI field, valid only if cpm == 1 */

   /* The following two fields are relevant if picture type indicates
      a PB frame */
   int trb;                   /* The TRB field */
   int dbquant;               /* The DBQUANT field */

   int lumWidth;              /* the width and height of the luminance */
   int lumHeight;             /* image to display (divisible with 4) */

   int lumMemWidth;           /* the width and height of the luminance */
   int lumMemHeight;          /* image in memory (divisible with 16) */

   /* The following six fields are relevant only if fSS is off */
   int numGOBs;               /* Number of GOBs in picture */
   int fLastGOBSizeDifferent; /* Flag indicating different size of last GOB */
   int numMBsInGOB;           /* Number of macroblocks in other GOBs than 
                                 the last one */
   int numMBsInLastGOB;       /* Number of macroblocks in last GOB */
   int numMBLinesInGOB;       /* Number of macroblock lines in one GOB */
   int numMBsInMBLine;        /* Number of macroblocks in macroblock line */

   /* The following six fields are relevant only if fSS is on */
   int mbaFieldWidth;         /* MBA Field width */
   int mbaMaxValue;           /* MBA maximum value. See Table K.2/H.263 */
   int swiFieldWidth;         /* SWI Field width */
   int swiMaxValue;           /* SWI maximum value. See Table K.3/H.263 */

   int fUMV;                  /* Unrestricted Motion Vector Mode */
   int fSAC;                  /* Syntax-based Arithmetic Coding Mode */
   int fAP;                   /* Advanced Prediction Mode */

   int fRPR;                  /* Reference Picture Resampling Mode */
   int fRRU;                  /* Reduced-Resolution Update Mode */
   int rtype;                 /* Rounding type (RTYPE) */

   int elnum;                 /* Enhancement layer number */

   int fCustomSourceFormat;   /* Flag indicating if custom source format 
                                 is used */

   int fAIC;                  /* Advanced INTRA Coding Mode */
   int fDF;                   /* Deblocking Filter Mode */
   int fSS;                   /* Slice Structured Mode */
   int fRPS;                  /* Reference Picture Selection Mode,
                                 Note: always valid and therefore should be
                                 the same as instance->fRPS after decoding
                                 the first picture header. */
   int fISD;                  /* Independent Segment Decoding Mode */
   int fAIV;                  /* Alternate INTER VLC Mode */
   int fMQ;                   /* Modified Quantization Mode */

                              /* Picture Clock Frequence (PCF) fields */
   int fCustomPCF;            /* 0 = CIF PCF, 1 = custom PCF */
   int pcfRate;               /* PCF = pcfRate / pcfScale Hz */
   int pcfScale;

   /* The following field is relevant only if fUMV is on with fPLUSPTYPE */
   int fUMVLimited;           /* 0 = motion vector range is not limited,
                                 1 = motion vector range is limited
                                    according to Annex D */

   int fMVsOverPictureBoundaries;
                              /* 0 = prediction over picture boundaries is 
                                     disallowed,
                                 1 = prediction over picture boundaries is
                                     allowed */

   /* The MPEG-4 Video Object Layer (VOL) parameters */
/*** MPEG-4 REVISION ***/

   int vo_id;                 /* VO Id */
   int vol_id;                /* VO Id */

   u_char error_res_disable;  /* VOL disable error resilence mode */
   u_char reversible_vlc;     /* VOL reversible VLCs */
   u_char data_partitioned;   /* VOL data partitioning */

   int time_increment_resolution; /* resolution of the time increment
                                     in the VOP header */

   /* The in H.263 not existing MPEG-4 Video Object Plane (VOP) parameters */
   
   int mod_time_base;         /* VOP modulo time base (absolute) */
   int time_base_incr;        /* time base increment of the current VOP
                                 (used in HEC of Video Packet Header,
                                 when time_base_incr of the VOP is
                                 retransmitted */
   int time_inc;              /* VOP time increment 
                                 (relative to last mod_time_base) */ 
   int intra_dc_vlc_thr;
   int fcode_forward;
   u_char fixed_vop_rate;       /* fixed vop rate indication, added for transcoding */
/*** End MPEG-4 REVISION ***/

} vdcPictureParam_t;
/* {{-output"vdcPictureParam_t.txt"}} */


/* {{-output"vdcInstance_t_info.txt" -ignore"*" -noCR}}
   vdcInstance_t holds the instance data for a Video Decoder Core instance.
   This structure is used to keep track of the internal state of
   a VDC instance.
   {{-output"vdcInstance_t_info.txt"}} */

/* {{-output"vdcInstance_t.txt"}} */
typedef struct {
   vdeImsItem_t *currFrame;   /* Current P/I frame (image store item) */
   vdeImsItem_t *bFrame;      /* Current B frame (of PB, image store item) */
                              /* NULL pointer indicates that the frames are
                                 not valid. */

   vdcPictureParam_t pictureParam;
                              /* Picture parameters for the current picture */

   int32 frameNum;            /* Frame number */
   int32 frameNumForBFrame;   /* Frame number for B frame */

   int gfid;                  /* GOB Frame ID */
   vdxPictureHeader_t *prevPicHeader;  /* Header of the previous picture */
   int fPrevPicHeaderReliable;	/* if header is not 100% reliable, it is better not to compare it with the next header */

   mvcData_t mvcData;         /* Storage for motion vector data */

/*** MPEG-4 REVISION ***/

   aicData_t aicData;         /* Storage AC/DC reconstruction*/

   char *user_data;           /* User Data */
   int user_data_length; 

/*** End MPEG-4 REVISION ***/

   u_int32 nOfDecodedFrames;  /* Counter for (partially) successfully decoded frames */
   u_char fIntraGot;          /* non-zero = INTRA frame has been decoded */
   u_char fEOS;               /* 1 if EOS has been reached, 0 otherwise */

   int fRPS;                  /* Reference Picture Selection in use? 
                                 At first, set to zero in vdcOpen.
                                 Then, modified according to the bitstream. */

   int rpsMode;               /* both/either/neither ACK and/or/nor NACK, 
                                 VDX_RPS_MODE_XXX */

   int fIgnoreRPSBufferUpdate;/* 0 = segment buffering in FIFO mode as normally
                                 1 = decoded pictures are not put into
                                     reference segment buffers */

   int numAnnexNScalabilityLayers;
                              /* -1  = no frames decoded yet,
                                 0   = Nokia-proprietary Annex N scalability 
                                       layers not in use,
                                 2.. = number of scalability layers */

   int fGFIDShouldChange;     /* 1, if GFID should change, 0 otherwise */

   vdeIms_t *imageStore;      /* Pointer to image store */

   void *hParent;             /* typeless handle to vdeInstance_t */

   u_int32 snapshotStartCallback;  /* function pointer to a function informing   w
                                      the beginning of a snapshot */

   u_int32 snapshotEndCallback;  /* function pointer to a function informing
                                      the end of a snapshot */

   int snapshotStatus;           /* snapshot transmission status */

   u_int32 reportPictureSizeCallback; /* callback function for informing
                                         frame size */

} vdcInstance_t;
/* {{-output"vdcInstance_t.txt"}} */


/*
 * Function prototypes
 */

VDC_INLINE int VDC_MIN(int a, int b) {return a < b ? a : b;}
VDC_INLINE int VDC_MAX(int a, int b) {return a > b ? a : b;}


#endif

// End of file
