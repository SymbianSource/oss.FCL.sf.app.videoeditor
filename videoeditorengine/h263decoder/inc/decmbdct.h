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
* Header for macroblock content blocks decoding functions.
*
*/




#ifndef _DECMBDCT_H_
#define _DECMBDCT_H_

/*
 * Includes
 */

#include "block.h"
#include "vdcaic.h"

#include "biblin.h"

class CMPEG4Transcoder;

/*
 * Defines
 */

// unify error codes
#define DMD_BIT_ERR H263D_OK_BUT_BIT_ERROR
#define DMD_OK H263D_OK
#define DMD_ERR H263D_ERROR


/*
 * Structs and typedefs
 */

/* {{-output"dmdBOfPBParam_t_info.txt" -ignore"*" -noCR}}
   This structure is used to pass parameters to
   the dmdGetAndDecodeBOfPBMBBlocks function.
   {{-output"dmdBOfPBParam_t_info.txt"}} */

/* {{-output"dmdBOfPBParam_t.txt"}} */
typedef struct {
   bibBuffer_t *inBuffer;        /* Bit Buffer instance data */


   bibBuffer_t *outBuffer;        /* Out Bit Buffer instance data */
   int StartByteIndex;
   int StartBitIndex;

   int cbpb;                     /* Coded block pattern for B-blocks */
   
   int quant;                    /* Current quantizer */

   BLC_COPY_PREDICTION_MB_PARAM  /* See block.h */

   int fMQ;                      /* Modified Quantization Mode flag */

} dmdBOfPBParam_t;
/* {{-output"dmdBOfPBParam_t.txt"}} */


/* {{-output"dmdIParam_t_info.txt" -ignore"*" -noCR}}
   This structure is used to pass parameters to
   the dmdGetAndDecodeIMBBlocks function.
   {{-output"dmdIParam_t_info.txt"}} */

/* {{-output"dmdIParam_t.txt"}} */
typedef struct {
   bibBuffer_t *inBuffer;        /* Bit Buffer instance data */


   bibBuffer_t *outBuffer;        /* Out Bit Buffer instance data */
   bibBufferEdit_t *bufEdit; 
   int iColorEffect; 
   TBool iGetDecodedFrame;
   int StartByteIndex;
   int StartBitIndex;


   int cbpy;                     /* Coded block pattern for luminance */

   int cbpc;                     /* Coded block pattern for chrominance */

   int quant;                    /* Current quantizer */

   int yWidth;                   /* Picture width in luminance pixels */

   u_char *yMBInFrame;           /* Current macroblock in frame */
   u_char *uBlockInFrame; 
   u_char *vBlockInFrame;

   int xPosInMBs;                /* Horizontal position of the macroblock
                                    (in macroblocks), 
                                    0 .. (numMBsInMBLine - 1) */

   int yPosInMBs;                /* Vertical position of the macroblock,
                                    (in macroblocks), 0 .. */

   int numMBLinesInGOB;          /* Number of macroblock lines in GOB */

   int pictureType;              /* Picture type is one of the following:
                                       VDX_PIC_TYPE_I       INTRA
                                       VDX_PIC_TYPE_P       INTER
                                       VDX_PIC_TYPE_PB      PB (Annex G)
                                       VDX_PIC_TYPE_IPB     Improved PB
                                       VDX_PIC_TYPE_B       B (Annex O)
                                       VDX_PIC_TYPE_EI      EI (Annex O)
                                       VDX_PIC_TYPE_EP      EP (Annex O) */

   int numMBsInMBLine;           /* Number of MBs in one MB Line */
   
   int fGOBHeaderPresent;        /* 1 if GOB header is present.*/
   
   int predMode;                 /* prediction mode given by INTRA_MODE field */

   int fMQ;                      /* Modified Quantization Mode flag */

   int sumBEI;                   /* Sum (bit-wise OR) of bit error indications for the whole MB */

   int rightOfBorder;            /* There is a border on the left of the current MB */
   
   int downOfBorder;             /* There is a border on top of the current MB */

} dmdIParam_t;
/* {{-output"dmdIParam_t.txt"}} */

/* {{-output"dmdMPEGIParam_t_info.txt" -ignore"*" -noCR}}
   This structure is used to pass parameters to
   the dmdGetAndDecodeMPEGIMBBlocks function.
   {{-output"dmdMPEGIParam_t_info.txt"}} */

/* {{-output"dmdMPEGIParam_t.txt"}} */
typedef struct {
   bibBuffer_t *inBuffer;        /* Bit Buffer instance data */


   bibBuffer_t *outBuffer;        /* Out Bit Buffer instance data */
   bibBufferEdit_t *bufEdit; 
   int iColorEffect; 
   TBool iGetDecodedFrame;
   int StartByteIndex;
   int StartBitIndex;

   int cbpy;                     /* Coded block pattern for luminance */

   int cbpc;                     /* Coded block pattern for chrominance */

   int quant;                    /* Current quantizer */

   int yWidth;                   /* Picture width in luminance pixels */

   int switched;                 /* if TRUE instead of optimized IntraDC
                                    decoding IntraAC VLC table is used for
                                    DC coefficient coding */

   int currMBNum;                /* current MB Number */

   u_char fTopOfVP;              /* The current Macroblock is in the top row
                                    of the current Video Packet */
   u_char fLeftOfVP;             /* The current Macroblock is the first
                                    (on the left) of the current Video Packet */
   u_char fBBlockOut;            /* The "B" AC/DC prediction block for the 
                                    current Macroblock is outside of the
                                    current Video Packet */

   aicData_t *aicData;           /* storage for the intra prediction (AIC) data */

   u_char data_partitioned;
   int *DC;                      /* if data_partitioned, this is the array of
                                    the DC coefficients decoded in the previous
                                    partition of the VP */

   u_char reversible_vlc;        /* TRUE if reversible VLC is used */
   u_char vlc_dec_direction;     /* 0: forward, 1: backward decoding of RVLC */

   u_char *yMBInFrame;           /* Current macroblock in frame */
   u_char *uBlockInFrame; 
   u_char *vBlockInFrame;

   int xPosInMBs;                /* Horizontal position of the macroblock
                                    (in macroblocks), 
                                    0 .. (numMBsInMBLine - 1) */

   int yPosInMBs;                /* Vertical position of the macroblock,
                                    (in macroblocks), 0 .. */

   int numMBsInMBLine;           /* Number of macroblocks in one line */

   int numMBLinesInGOB;          /* Number of macroblock lines in GOB */

   int pictureType;              /* Picture type is one of the following:
                                       VDX_PIC_TYPE_I       INTRA
                                       VDX_PIC_TYPE_P       INTER
                                       VDX_PIC_TYPE_PB      PB (Annex G)
                                       VDX_PIC_TYPE_IPB     Improved PB
                                       VDX_PIC_TYPE_B       B (Annex O)
                                       VDX_PIC_TYPE_EI      EI (Annex O)
                                       VDX_PIC_TYPE_EP      EP (Annex O) */

   int fMQ;
   int fAIC;

} dmdMPEGIParam_t;
/* {{-output"dmdMPEGIParam_t.txt"}} */

/* {{-output"dmdPParam_t_info.txt" -ignore"*" -noCR}}
   This structure is used to pass parameters to
   the dmdGetAndDecodeIMBBlocks function.
   {{-output"dmdPParam_t_info.txt"}} */

/* {{-output"dmdPParam_t.txt"}} */
typedef struct {
   bibBuffer_t *inBuffer;        /* Bit Buffer instance data */


   bibBuffer_t *outBuffer;        /* Out Bit Buffer instance data */
   bibBufferEdit_t *bufEdit; 
   int iColorEffect;
   TBool iGetDecodedFrame;
   int StartByteIndex;
   int StartBitIndex;

   int cbpy;                     /* Coded block pattern for luminance */

   int cbpc;                     /* Coded block pattern for chrominance */

   int quant;                    /* Current quantizer */

   BLC_COPY_PREDICTION_MB_PARAM  /* See block.h */

/*** MPEG-4 REVISION ***/
   u_char reversible_vlc;        /* TRUE if reversible VLC is used */
   u_char vlc_dec_direction;     /* 0: forward, 1: backward decoding of RVLC */
/*** End MPEG-4 REVISION ***/

   int xPosInMBs;                /* Horizontal position of the macroblock
                                    (in macroblocks), 
                                    0 .. (numMBsInMBLine - 1) */

   int yPosInMBs;                /* Vertical position of the macroblock,
                                    (in macroblocks), 0 .. */
   
   int numMBsInMBLine;           /* Number of MBs in one MB Line */
   
   int fGOBHeaderPresent;        /* 1 if GOB header is present.*/
   
   int mbType;


} dmdPParam_t;
/* {{-output"dmdPParam_t.txt"}} */


/*
 * Function prototypes
 */

int dmdGetAndDecodeBOfPBMBBlocks(
   dmdBOfPBParam_t *param);

int dmdGetAndDecodeIMBBlocks(
   dmdIParam_t *param, CMPEG4Transcoder *hTranscoder);

int dmdGetAndDecodePMBBlocks(
   dmdPParam_t *param, CMPEG4Transcoder *hTranscoder);

int dmdGetAndDecodeAdvIMBBlocks(
   dmdIParam_t *inpParam);

/*** MPEG-4 REVISION ***/
int dmdGetAndDecodeMPEGPMBBlocks(
   dmdPParam_t *param, CMPEG4Transcoder *hTranscoder);

int dmdGetAndDecodeMPEGIMBBlocks(
   dmdMPEGIParam_t *param, CMPEG4Transcoder *hTranscoder);
/*** End MPEG-4 REVISION ***/

#endif
// End of File
