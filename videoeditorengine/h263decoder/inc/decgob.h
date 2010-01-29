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
* Header for GOB segment decoding module.
*
*/


#ifndef _DECGOB_H_
#define _DECGOB_H_

#include "core.h"
#include "vdcmvc.h"
#include "vdeims.h"
#include "MPEG4Transcoder.h"
class CMPEG4Transcoder;
/*
 * Defines
 */
 
// unify error codes
#define DGOB_OK_BUT_BIT_ERROR H263D_OK_BUT_BIT_ERROR           /* Bit errors detected */
#define DGOB_OK H263D_OK
#define DGOB_ERR H263D_ERROR

/*
 * Structs and typedefs
 */

/* {{-output"dgobGOBSegmentInParam_t_info.txt" -ignore"*" -noCR}}
   This structure is used to pass input parameters to
   the dgobGetAndDecodeGOBSegment and dgobGetAndDecodeGOBSegmentContents
   functions.
   {{-output"dgobGOBSegmentInParam_t_info.txt"}} */

/* {{-output"dgobGOBSegmentInParam_t.txt"}} */
typedef struct {
   int numStuffBits;             /* Number of stuffing bits before GBSC */

   vdcPictureParam_t *pictParam; /* Picture attributes */

   bibBuffer_t *inBuffer;        /* Bit Buffer instance */


   bibBuffer_t *outBuffer;        /* output Bit Buffer instance */
   bibBufferEdit_t *bufEdit; 
   int iColorEffect; 
   TBool iGetDecodedFrame;


   int fGFIDShouldChange;        /* If non-zero, the gfid parameter passed in 
                                    the dgobGOBSegmentInOutParam_t structure
                                    should be different from the value got
                                    from the bitstream. Otherwise, the values
                                    should be equal. */

} dgobGOBSegmentInParam_t;
/* {{-output"dgobGOBSegmentInParam_t.txt"}} */



/* {{-output"dgobGOBSegmentInOutParam_t_info.txt" -ignore"*" -noCR}}
   This structure is used to pass input parameters to
   the dgobGetAndDecodeGOBSegment and dgobGetAndDecodeGOBSegmentContents
   functions. The functions may modify the parameters (or the structures
   pointed by the parameters).
   {{-output"dgobGOBSegmentInOutParam_t_info.txt"}} */

/* {{-output"dgobGOBSegmentInOutParam_t.txt"}} */
typedef struct {
   int prevGNWithHeader;         /* GN of the latest decoded GOB with header */

   int prevGN;                   /* GN of the latest decoded GOB */

   int gfid;                     /* GFID of the latest GOB header,
                                    specify -1 if this is the 1st GOB of
                                    the sequence */

   u_char *fCodedMBs;            /* array for coded macroblock flags in
                                    scan-order, 1 = coded, 0 = not coded */

   int numOfCodedMBs;            /* number of coded macroblocks */

   int *quantParams;             /* array of quantization parameters for
                                    macroblocks in scan-order */

   mvcData_t *mvcData;           /* Motion Vector Count module instance data */

   vdeIms_t *imageStore;         /* VDE Image Store instance data */

   int trp;                      /* Temporal Reference for Prediction,
                                    -1 if not indicated in the bitstream */

   int rtr;                      /* Real TR of the referenced image */

   u_char *refY;                 /* Reference frame */
   u_char *refU;
   u_char *refV;

   u_char *currPY;               /* Current P (or I) frame */
   u_char *currPU;
   u_char *currPV;

   int StartByteIndex;
   int StartBitIndex;


} dgobGOBSegmentInOutParam_t;
/* {{-output"dgobGOBSegmentInOutParam_t.txt"}} */


/* {{-output"dgobGOBCheckParam_t_info.txt" -ignore"*" -noCR}}
   This structure is used to pass input parameters to
   the dgobCheckNextGob function.
   {{-output"dgobGOBCheckParam_t_info.txt"}} */

/* {{-output"dgobGOBCheckParam_t.txt"}} */
typedef struct {
   bibBuffer_t *inBuffer;        /* Bit Buffer instance */

   bibBuffer_t *outBuffer;        /* Out Bit Buffer instance data */
	 int StartByteIndex;
	 int StartBitIndex;

   vdcPictureParam_t *pictParam; /* Picture attributes */
   int numStuffBits;             /* Number of stuffing bits before GBSC */
   int prevGN;                   /* GN of the latest GOB */
   int gfid;                     /* GFID of the frame */
} dgobCheckParam_t;
/* {{-output"dgobGOBCheckParam_t.txt"}} */


/*
 * Functions prototypes
 */

int dgobGetAndDecodeGOBSegment(
   const dgobGOBSegmentInParam_t *inParam,
   dgobGOBSegmentInOutParam_t *inOutParam, CMPEG4Transcoder *hTranscoder);

int dgobGetAndDecodeGOBSegmentContents(
   const dgobGOBSegmentInParam_t *inParam,
   int fGetNewReferenceFrame,
   int quant,
   dgobGOBSegmentInOutParam_t *inOutParam, CMPEG4Transcoder *hTranscoder);

int dgobCheckNextGob( 
   dgobCheckParam_t *param , CMPEG4Transcoder *hTranscoder);

#endif
// End of File
