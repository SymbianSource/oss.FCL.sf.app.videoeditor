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
* Header file for Video packet decoding module.
*
*/



#ifndef _DECVP_MPEG_H_
#define _DECVP_MPEG_H_

#include "core.h"
#include "vdcmvc.h"
#include "vdeims.h"


/*
 * Defines
 */

// unify error codes
#define DGOB_OK_BUT_FRAME_USELESS H263D_OK_BUT_FRAME_USELESS
#define DGOB_OK_BUT_BIT_ERROR H263D_OK_BUT_BIT_ERROR
#define DGOB_OK     H263D_OK
#define DGOB_ERR    H263D_ERROR


/*
 * Structs and typedefs
 */

/* {{-output"dvpMarkVPCompleted_t_info.txt" -ignore"*" -noCR}}
   This structure is used to pass parameters to
   the dvpMarkVPCompleted function.
   {{-output"dvpMarkVPCompleted_t_info.txt"}} */

/* {{-output"dvpMarkVPCompleted_t.txt"}} */
typedef struct {
   int numMBsInVP;               /* Number of macroblocks within the VP */

   int xPosInMBs;                /* x position of the first MB in the VP */

   int yPosInMBs;                /* y position of the first MB in the VP */

   int widthInMBs;               /* Image width in macroblocks */

   int fSegmentCorrupted;        /* Non-zero if the reported GOBs are 
                                    corrupted. Otherwise zero. */


} dvpMarkVPCompleted_t;
/* {{-output"dvpMarkVPCompleted_t.txt"}} */


/* {{-output"dvpVPInParam_t_info.txt" -ignore"*" -noCR}}
   This structure is used to pass input parameters to
   the dvpGetAndDecodeVideoPacketHeader and 
   dvpGetAndDecodeVideoPacketContents functions.
   {{-output"dvpVPInParam_t_info.txt"}} */

/* {{-output"dvpVPInParam_t.txt"}} */
typedef struct {

   vdcPictureParam_t *pictParam; /* Picture attributes */

   bibBuffer_t *inBuffer;        /* Bit Buffer instance */

   bibBuffer_t *outBuffer;        /* Out Bit Buffer instance data */

   bibBufferEdit_t *bufEdit; 
   int iColorEffect; 
   TBool iGetDecodedFrame;

   u_char fVOPHeaderCorrupted;   /* If non-zero, the VOP header was detected
                                    to be erronous and the HEC information is
                                    used for updating */

} dvpVPInParam_t;
/* {{-output"dvpVPInParam_t.txt"}} */


/* {{-output"dvpVPInOutParam_t_info.txt" -ignore"*" -noCR}}
   This structure is used to pass input parameters to
   the dvpGetAndDecodeVideoPacketHeader and dvpGetAndDecodeVideoPacketContents
   functions. The functions may modify the parameters (or the structures
   pointed by the parameters).
   {{-output"dvpVPInOutParam_t_info.txt"}} */

/* {{-output"dvpVPInOutParam_t.txt"}} */
typedef struct {
    
   int currMBNum;                /* the number of the next expected MB to decode */

   int frameNum;                 /* the calculated frame number */

   int quant;                    /* QP for the next MB */

   u_char *fCodedMBs;            /* array for coded macroblock flags in
                                    scan-order, 1 = coded, 0 = not coded */

   int numOfCodedMBs;            /* number of coded macroblocks */

   int *quantParams;             /* array of quantization parameters for
                                    macroblocks in scan-order */

   mvcData_t *mvcData;           /* Motion Vector Count module instance data */

   aicData_t *aicData;           /* Advanced Intra Coding module instance data */

   vdeIms_t *imageStore;         /* VDE Image Store instance data */

   u_char *refY;                 /* Reference frame */
   u_char *refU;
   u_char *refV;

   u_char *currPY;               /* Current P (or I) frame */
   u_char *currPU;
   u_char *currPV;

	 /* MVE */
   int StartByteIndex;
	 int StartBitIndex;

} dvpVPInOutParam_t;
/* {{-output"dvpVPInOutParam_t.txt"}} */

/*
 * Functions prototypes
 */

int dvpGetAndDecodeVideoPacketHeader(
   const dvpVPInParam_t *inParam,
   dvpVPInOutParam_t *inOutParam);

int dvpGetAndDecodeVideoPacketContents(
   const dvpVPInParam_t *inParam,
   int fGetNewReferenceFrame,
   dvpVPInOutParam_t *inOutParam, CMPEG4Transcoder *hTranscoder);

int dvpMarkVPCompleted(dvpMarkVPCompleted_t *inParam);

#endif
// End of File
