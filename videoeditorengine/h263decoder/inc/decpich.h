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
* Header for picture header decoding functions.
*
*/




#ifndef _DECPICH_H_
#define _DECPICH_H_

/*
 * Includes
 */

#include "core.h"

#include "biblin.h"


/*
 * Defines
 */

// unify error codes
#define DPH_OK            H263D_OK  /* everything ok */

#define DPH_OK_BUT_BIT_ERROR     H263D_OK_BUT_BIT_ERROR  /* bitstream corruption was detected */
#define DPH_OK_BUT_NOT_CODED     H263D_OK_BUT_NOT_CODED  /* the VOP is not_coded: no video data sent, 
                                       only the timestamp -> skip the frame */
#define DPH_ERR         H263D_ERROR /* a general error */
#define DPH_ERR_NO_INTRA H263D_ERROR_NO_INTRA /* no INTRA frame has been received */
#define DPH_ERR_FORMAT_CHANGE H263D_ERROR /* picture size change in bitstream */


/*
 * Structs and typedefs
 */

/* {{-output"dphInParam_t_info.txt" -ignore"*" -noCR}}
   This structure is used to pass input parameters to
   the dphGetPictureHeader function.
   {{-output"dphInParam_t_info.txt"}} */

/* {{-output"dphInParam_t.txt"}} */
typedef struct {
   int numStuffBits;             /* Number of stuffing bits before PSC */
   int fReadBits;                /* 0, if picture header should not be read (use previous 
                                    picture header but decode it using dphGetPictureHeader) */
   int fNeedDecodedFrame;                                    
} dphInParam_t;
/* {{-output"dphInParam_t.txt"}} */


/* {{-output"dphInOutParam_t_info.txt" -ignore"*" -noCR}}
   This structure is used to pass input parameters to
   the dphGetPictureHeader function. The function may modify
   the parameters (or the structures pointed by the parameters).
   {{-output"dphInOutParam_t_info.txt"}} */

/* {{-output"dphInOutParam_t.txt"}} */
typedef struct {
   vdcInstance_t *vdcInstance;   /* Video Decoder Core instance */
   bibBuffer_t *inBuffer;        /* Bit Buffer instance */
   bibBuffer_t *outBuffer;        /* Out Bit Buffer instance data */


} dphInOutParam_t;
/* {{-output"dphInOutParam_t.txt"}} */


/* {{-output"dphOutParam_t_info.txt" -ignore"*" -noCR}}
   The dphGetPictureHeader function uses this structure to return
   output parameters.
   {{-output"dphOutParam_t_info.txt"}} */

/* {{-output"dphOutParam_t.txt"}} */
typedef struct {
   int pquant;                   /* Initial quantizer */
   int trp;                      /* Temporal Reference for Prediction,
                                    -1 if not indicated in the bitstream */
   u_char *currYFrame;           /* Memory buffer for new Y frame */
   u_char *currUFrame;           /* Memory buffer for new U frame */
   u_char *currVFrame;           /* Memory buffer for new V frame */
   u_char *currYBFrame;          /* New Y frame of B part of PB-frame */
   u_char *currUBFrame;          /* New U frame of B part of PB-frame */
   u_char *currVBFrame;          /* New V frame of B part of PB-frame */
} dphOutParam_t;
/* {{-output"dphOutParam_t.txt"}} */


/*
 * Function prototypes
 */

int dphGetPictureHeader(
   const dphInParam_t *inParam,
   dphInOutParam_t *inOutParam,
   dphOutParam_t *outParam,
   int *bitErrorIndication);

int dphGetSEI(
   vdcInstance_t *vdcInstance,   /* Video Decoder Core instance */
   bibBuffer_t *inBuffer,        /* Bit Buffer instance */
   int *bitErrorIndication);

class CMPEG4Transcoder;
int dphGetMPEGVolHeader(dphInOutParam_t *inOutParam, CMPEG4Transcoder *hTranscoder);

int dphGetMPEGVopHeader(
   const dphInParam_t *inParam,
   dphInOutParam_t *inOutParam,
   dphOutParam_t *outParam,
   int *bitErrorIndication);

#endif

// End of file
