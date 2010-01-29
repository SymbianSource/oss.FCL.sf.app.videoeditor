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
* An application programming interface for core functions of
* video decoder.
*
*/


#ifndef _VDC263_H_
#define _VDC263_H_


/*
 * Includes
 */ 


#include "biblin.h"
#include "vdeims.h"


/*
 * Defines
 */

/* Return values */
// unify error codes
#define VDC_OK_BUT_NOT_CODED H263D_OK_BUT_NOT_CODED 
                                    /* not coded frame, 
                                       copied to output as is, 
                                       no decoded output generated */  
#define VDC_OK_BUT_FRAME_USELESS H263D_OK_BUT_FRAME_USELESS  
                                    /* Decoder behaved normally the decoded
                                       frame(s) should not be displayed since
                                       it is too heavily corrupted. */

#define VDC_OK_BUT_BIT_ERROR H263D_OK_BUT_BIT_ERROR  
                                    /* The decoded frame(s) are degraded due
                                       to transmission errors but can be
                                       displayed due to successful error
                                       concealment. */

#define VDC_OK            H263D_OK  /* Everything ok. */

#define VDC_ERR         H263D_ERROR /* An unexpected processing error 
                                       occured. */
#define VDC_ERR_NO_INTRA H263D_ERROR_NO_INTRA /* No INTRA frame has been decoded
                                       and the current frame is INTER  */

/* vdcPictureHeader_t and vdxSEI_t related definitions */
#define VDX_MAX_BYTES_IN_PIC_HEADER 44
                                    /* Maximum number of bytes allowed in
                                       SEI picture header repetition.

                                       Picture headers not including
                                       BCM (Annex N video-mux back-channel),
                                       RPRP (Annex P warping parameters), and
                                       Annex U 
                                       allocate 152 bits = 19 bytes in 
                                       the worst case.
                                       Our decoder does not support the features
                                       mentioned above. However, to ensure
                                       that introduction of any of the mentioned
                                       tools would not break this mechanism,
                                       a relatively large buffer is allocated
                                       for picture header copies. Note that
                                       Annex U does not have an upper limit
                                       for picture header lengths. */


/*
 * Structs and typedefs
 */

/* {{-output"vdcHInstance_t_info.txt" -ignore"*" -noCR}}
   vdcHInstance_t is used as a unique identifier of a VDC instance.
   The type can be casted to u_int32 or void pointer and then back to
   vdcHInstance_t without any problems.
   {{-output"vdcHInstance_t_info.txt"}} */

/* {{-output"vdcHInstance_t.txt"}} */
typedef void * vdcHInstance_t;
/* {{-output"vdcHInstance_t.txt"}} */


/* The following vdx data types are defined here, because VDC exports
   these structures (see vdcDecodePictureHeader). */

/*
 * vdxPictureHeader_t
 */

/* {{-output"vdxPictureHeader_t_info.txt" -ignore"*" -noCR}}
   The vdxGetPictureHeader function uses this structure to return the picture
   header parameters.
   {{-output"vdxPictureHeader_t_info.txt"}} */

/* {{-output"vdxPictureHeader_t.txt"}} */
typedef struct {
   /* The following fields are always set */
      int numBits;               /* number of bits in the picture header,
                                    including PSC, excluding PSTUFF, PSUPP and
                                    PEI */

      int tr;                    /* the TR field or a combination of ETR and TR
                                    if ETR is used */

      int fSplitScreen;
      int fDocumentCamera;
      int fFreezePictureRelease;

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

      int pquant;                /* The PQUANT field */
      int pquantPosition;        /* Number of bits preceding the PQUANT field,
                                    including PSC, excluding PSTUFF */

   /* The following fields are set 
      if pictureType == VDX_PIC_TYPE_PB || pictureType == VDX_PIC_TYPE_IPB */
      int trb;                   /* The TRB field */
      int dbquant;               /* The DBQUANT field */

   /* The following fields are set if fPLUSPTYPE == 0 || ufep == 1 */
      int lumWidth;              /* the width and height of the luminance */
      int lumHeight;             /* image */

      int fUMV;                  /* Unrestricted Motion Vector Mode */
      int fSAC;                  /* Syntax-based Arithmetic Coding Mode */
      int fAP;                   /* Advanced Prediction Mode */

   /* The following fields are set if fPLUSPTYPE == 1 */
      int ufep;                  /* The UFEP field */

      int fRPR;                  /* Reference Picture Resampling Mode */
      int fRRU;                  /* Reduced-Resolution Update Mode */
      int rtype;                 /* Rounding type (RTYPE) */


      /* If Annex O is in use */
         int elnum;              /* Enhancement layer number */

      /* If the Reference Picture Selection mode is in use */
         int trpi;               /* 1 = trp is valid */
         int trp;                /* Temporal reference for prediction */

      /* The following fields are set if ufep == 1 */
         int fCustomSourceFormat;
   
         int fAIC;               /* Advanced INTRA Coding Mode */
         int fDF;                /* Deblocking Filter Mode */
         int fSS;                /* Slice Structured Mode */
         int fRPS;               /* Reference Picture Selection Mode */
         int fISD;               /* Independent Segment Decoding Mode */
         int fAIV;               /* Alternate INTER VLC Mode */
         int fMQ;                /* Modified Quantization Mode */

         int parWidth;           /* Pixel aspect ratio = parWidth : parHeight */
         int parHeight;

                                 /* Picture Clock Frequence (PCF) fields */
         int fCustomPCF;         /* 0 = CIF PCF, 1 = custom PCF */
         int pcfRate;            /* PCF = pcfRate / pcfScale Hz */
         int pcfScale;

         /* If fUMV == 1 */
            int fUMVLimited;     /* 0 = motion vector range is not limited,
                                    1 = motion vector range is limited
                                        according to Annex D */

         /* If fSS == 1 */
            int fRectangularSlices;  
                                 /* 0 = free-running slices, 
                                    1 = rectangular slices */

            int fArbitrarySliceOrdering;
                                 /* 0 = sequential order */
                                 /* 1 = arbitrary order */

         /* If Annex O is in use */                                 
            int rlnum;           /* Reference layer number */


         /* If fRPS == 1 */
            int rpsMode;         /* Reference Picture Selection Mode:
                                       VDX_RPS_MODE_NO_MSGS
                                       VDX_RPS_MODE_ACK
                                       VDX_RPS_MODE_NACK
                                       VDX_RPS_MODE_BOTH_MSGS */
} vdxPictureHeader_t;
/* {{-output"vdxPictureHeader_t.txt"}} */


/*
 * vdxSEI_t
 */

/* {{-output"vdxSEI_t_info.txt" -ignore"*" -noCR}}
   This structure is used to return parameters parsed from Supplemental
   Enhancement Information.
   {{-output"vdxSEI_t_info.txt"}} */

/* {{-output"vdxSEI_t.txt"}} */
typedef struct {
   int fFullPictureFreezeRequest;   /* 1 = Full-picture freeze requested as in
                                       section L.4 of H.263, 0 otherwise */

   int fFullPictureSnapshot;        /* 1 = Full-picture snapshot tag as in
                                        section L.8 of H.263, 0 otherwise */

   u_int32 snapshotId;              /* Snapshot identification number */
   
   u_int8 snapshotStatus;           /* Snapshot transmission status */

   int scalabilityLayer;            /* Annex N scalability layer for 
                                       the picture, -1 if not defined */

   int numScalabilityLayers;        /* Number of Annex N scalability layers,
                                       value from 2 to 15, 
                                       0 if no scalability layers in use. */

   u_char prevPicHeader[VDX_MAX_BYTES_IN_PIC_HEADER];
                                    /* Previous picture header repetition
                                       in bit-stream syntax including
                                       the first two bytes of PSC */

   int numBytesInPrevPicHeader;     /* Number of bytes valid in prevPicHeader,
                                       including the first two bytes of PSC
                                       and the last byte of picture header
                                       where the LSB bits are possibly not
                                       belonging to the picture header copy */

   int numBitsInLastByteOfPrevPicHeader;
                                    /* Number of valid bits in the last byte
                                       of prevPicHeader (1..8) */

   int fPrevPicHeaderTooLarge;      /* 1 if the previous picture header copy 
                                       did not fit into prevPicHeader,
                                       0 otherwise */
} vdxSEI_t;
/* {{-output"vdxSEI_t.txt"}} */


/*
 * Function prototypes, core.c
 */

int vdcClose(vdcHInstance_t hInstance);

class CMPEG4Transcoder;
int vdcDecodeFrame(vdcHInstance_t hInstance, bibBuffer_t *inBuffer, bibBuffer_t *outBuffer,
									 bibBufferEdit_t *bufEdit, int aColorEffect, TBool aGetDecodedFrame,
									 CMPEG4Transcoder *hTranscoder);


int vdcDecodePictureHeader(
   vdcHInstance_t hInstance,
   bibBuffer_t *inBuffer,
   vdxPictureHeader_t *header,
   vdxSEI_t *sei);

int vdcFree(void);

vdeImsItem_t *vdcGetImsItem(vdcHInstance_t hInstance, int index);

int vdcGetNumberOfAnnexNScalabilityLayers(
   vdcHInstance_t hInstance);

int vdcGetNumberOfOutputFrames(vdcHInstance_t hInstance);

void vdcGetTR(void *inpBuffer, u_int8 *tr);

int vdcIsEOSReached(vdcHInstance_t hInstance);

int vdcIsINTRA(
   vdcHInstance_t hInstance,
   void *frameStart,
   unsigned frameLength);

int vdcIsINTRAGot(vdcHInstance_t hInstance);

int vdcIsPB(
   vdcHInstance_t hInstance,
   void *frameStart,
   unsigned frameLength);

int vdcLoad(void);

vdcHInstance_t vdcOpen(
   vdeIms_t *imageStore, 
   int numReferenceFrames,
   void *hParent);

void vdcRestartVideo(vdcHInstance_t hInstance);

int vdcSetBackChannelUsage(vdcHInstance_t hInstance, int fSendMessages);

int vdcSetErrorResilience(vdcHInstance_t hInstance, int feature, int value);

int vdcSetStartOrEndSnapshotCallback(
   vdcHInstance_t hInstance, 
   u_int32 mode,
   u_int32 callback);

int vdcGetSnapshotStatus(vdcHInstance_t hInstance);

int vdcSetReportPictureSizeCallback(
   vdcHInstance_t hInstance, 
   u_int32 callback);

/*
 * Function prototypes, core_mpeg.c
 */

   int vdcDecodeMPEGVolHeader(vdcHInstance_t hInstance, bibBuffer_t *inBuffer, CMPEG4Transcoder *hTranscoder);

   int vdcDecodeMPEGVop(vdcHInstance_t hInstance, bibBuffer_t *inBuffer, bibBuffer_t *outBuffer,
                        bibBufferEdit_t *bufEdit, int aColorEffect, TBool aGetDecodedFrame, 
						int aStartByteIndex, int aStartBitIndex, CMPEG4Transcoder *hTranscoder);

   int vdcIsMPEGINTRA(
      vdcHInstance_t hInstance,
      void *frameStart,
      unsigned frameLength);

#endif
// End of File
