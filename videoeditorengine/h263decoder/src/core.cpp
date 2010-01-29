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
* H.263 decoder core functions.
*
*/


/* 
 * Includes 
 */
#include "h263dConfig.h"
#include "vdc263.h"
#include "core.h"
#include "debug.h"
#include "decblock.h" /* for dblFree and dblLoad */
#include "decgob.h"
#include "decpich.h"
#include "h263dapi.h" /* for H263D_BC_MUX_MODE_SEPARATE_CHANNEL and H263D_ERD_ */
#include "stckheap.h"
#include "sync.h"
#include "vdeims.h"
#include "vdeimb.h"
#include "viddemux.h"
#include "biblin.h"
#include "MPEG4Transcoder.h"


/*
 * Typedefs and structs
 */

/* This structure is used to indicate the expected decoding position. */
typedef enum {
   EDP_START_OF_FRAME,
   EDP_START_OF_GOB_SEGMENT,
   EDP_START_OF_SLICE_SEGMENT,
   EDP_END_OF_FRAME
} vdcExpectedDecodingPosition_t;


/*
 * Local function prototypes
 */ 

int vdcFillImageBuffers(
   vdcInstance_t *instance,
   int numOfCodedMBs,
   vdeImb_t *imbP);


/*
 * Global functions
 */

/* {{-output"vdcClose.txt"}} */
/*
 * vdcClose
 *    
 *
 * Parameters:
 *    hInstance                  handle of instance data
 *
 * Function:
 *    This function closes the instance. The function frees all the resources
 *    allocated for the instance. The instance handle is no longer valid
 *    after calling this function.
 *
 * Returns:
 *    >= 0                       if the function was successful
 *    < 0                        if an error occured
 *
 */

int vdcClose(vdcHInstance_t hInstance)
/* {{-output"vdcClose.txt"}} */
{
   vdcInstance_t *instance = (vdcInstance_t *) hInstance;
   int retValue = VDC_OK;

   if (instance == NULL)
      return retValue;

   mvcFree(&instance->mvcData);

   if ( instance->prevPicHeader != NULL )
      free( instance->prevPicHeader );

   aicFree(&instance->aicData);

              
   if ( instance->user_data != NULL )
      free( instance->user_data );


   vdcDealloc(instance);

   return retValue;
}


/* {{-output"vdcDecodeFrame.txt"}} */
/*
 * vdcDecodeFrame
 *    
 *
 * Parameters:
 *    hInstance                  handle of instance data
 *
 *    inBuffer                   pointer to bit buffer, the current position
 *                               of the buffer must start with a PSC
 *
 * Function:
 *    The vdcDecodeFrame function implements the decoding process described
 *    in the H.263 recommendation (version 2). However, it does not support
 *    the following features of the recommendation:
 *       decoding using the H.261 standard (bit 2 in PTYPE),
 *       source format changes during a video sequence.
 *
 *    The function decodes the next frame in
 *    the buffer (inBuffer) meaning that the decoding continues until the next
 *    PSC or EOS is found or until the end of the buffer is not reached.
 *
 * Returns:
 *    VDC_OK                     if the function was succesful
 *    VDC_OK_BUT_BIT_ERROR       if bit errors were detected but
 *                               decoded frames are provided by concealing
 *                               the errors
 *    VDC_OK_BUT_FRAME_USELESS   if severe bit errors were detected
 *                               (no concealment was possible) or
 *                               the bitstream ended unexpectedly
 *    VDC_ERR                    if a processing error occured,
 *                               the instance should be closed
 *
 */


int vdcDecodeFrame(vdcHInstance_t hInstance, bibBuffer_t *inBuffer, bibBuffer_t *outBuffer,
                   bibBufferEdit_t *bufEdit, int aColorEffect, TBool aGetDecodedFrame,
                   CMPEG4Transcoder *hTranscoder)

/* {{-output"vdcDecodeFrame.txt"}} */
{
   int prevGN = -1;           /* GOB number of the latest decoded GOB */
   int prevGNWithHeader = -1; /* GOB number of the latest decoded GOB with
                                 a GOB header */

   int numStuffBits;          /* Number of stuffing bits before the sync code */

   int sncCode;               /* the latest synchronization code, see 
                                 sncCheckSync for the possible values */

   int rtr = -1;              /* reference tr, 0.. */
   int trp = -1;              /* tr for prediction, -1 if not indicated in 
                                 the bitstream */

   int retValue = VDC_OK;     /* return value of this function */


   int16 error = 0;           /* Used to pass error codes from the sync module */

   u_char
      *currYFrame = NULL,     /* current P frame */
      *currUFrame = NULL,
      *currVFrame = NULL,
      *refYFrame = NULL,      /* reference frame */
      *refUFrame = NULL,
      *refVFrame = NULL;

   u_char *fCodedMBs = NULL;  /* Pointer to table, which indicates coded \
                                 macroblocks by non-zero value */
   int numOfCodedMBs = 0;     /* The number of coded macroblocks */

   int *quantParams = NULL;   /* Pointer to table, in which the quantization
                                 parameter for each macroblock is stored */

   int decStatus = 0;         /* Decoding status, returned from decgob.c */
   int corruptedSegments = 0; /* counter for corrupted segments */
   int decodedSegments = 0;   /* counter for decoded segments (used in slice mode) */
   int headerSuccess = 0;     /* success of picture header */
   int numDecodedMBs = 0;     /* Total number of decoded MBs */
   int numMBsInFrame = 0;     /* Number of MBs in frame */


   vdcExpectedDecodingPosition_t expectedDecodingPosition;
                              /* Tells in which part of the bitstream
                                 the decoding should be */

   vdcInstance_t *instance = (vdcInstance_t *) hInstance;
                              /* instance data */
      
   vdcAssert(instance != NULL);

   /* Initializations */

   instance->currFrame = NULL;
   expectedDecodingPosition = EDP_START_OF_FRAME;
   instance->pictureParam.prevTR = instance->pictureParam.tr;

   /* Main loop */
   for (;;) {
      int bitErrorIndication = 0;

      sncCode = sncCheckSync(inBuffer, &numStuffBits, &error);

      /* If sncCheckSync failed */
      if (error && error != ERR_BIB_NOT_ENOUGH_DATA) {
         deb1p("vdcDecodeFrame: ERROR - sncCheckSync returned %d.\n", error);
         retValue = VDC_ERR;
         goto exitFunction;
      }

      /* If EOS was got */
      if (sncCode == SNC_EOS)
         instance->fEOS = 1;

      /* If frame ends appropriately */
      if (expectedDecodingPosition == EDP_END_OF_FRAME &&
         (sncCode == SNC_PSC || sncCode == SNC_EOS || 
          sncCode == SNC_STUFFING || error == ERR_BIB_NOT_ENOUGH_DATA )) {
         goto exitFunction;
      }

      /* Else if frame (or stream) data ends suddenly */
      else if (error == ERR_BIB_NOT_ENOUGH_DATA) {
         retValue = VDC_OK_BUT_BIT_ERROR;
         goto exitFunction;
      }

      /* Else if EOS was reached */
      else if (sncCode == SNC_EOS) {
         /* The current frame is useless since it ends before it is complete.
            On the other hand, there is no point in concealing it since
            it is the last frame of the sequence. */
         retValue = VDC_OK_BUT_FRAME_USELESS;
         goto exitFunction;
      }

      /* Else if frame starts as expected */
      else if (expectedDecodingPosition == EDP_START_OF_FRAME &&
         sncCode == SNC_PSC) {

         dphInParam_t pichIn;
         dphInOutParam_t pichInOut;
         dphOutParam_t pichOut;

         pichIn.numStuffBits = numStuffBits;
         pichIn.fNeedDecodedFrame = aGetDecodedFrame | hTranscoder->NeedDecodedYUVFrame();

         pichIn.fReadBits = 1;
         pichInOut.vdcInstance = instance;
         pichInOut.inBuffer = inBuffer;

         if ( instance->fRPS ) {
            /* Store the previous TR for VRC needs */
            if ( instance->nOfDecodedFrames > 0 )
               trp = instance->pictureParam.tr;
            else
               trp = -1;
         }
         /* Get picture header */
         headerSuccess = dphGetPictureHeader(&pichIn, &pichInOut, &pichOut, &bitErrorIndication);

         if (headerSuccess != DPH_OK) {
            
            deb("vdcDecodeFrame: Header decoding unsuccessful due to errors, picture will be discarded.\n");
            retValue = VDC_OK_BUT_FRAME_USELESS;
            goto exitFunction;
         }

         numMBsInFrame = renDriNumOfMBs(instance->currFrame->imb->drawItem);

         currYFrame = pichOut.currYFrame;
         currUFrame = pichOut.currUFrame;
         currVFrame = pichOut.currVFrame;

         numOfCodedMBs = 0;
         fCodedMBs = renDriCodedMBs(instance->currFrame->imb->drawItem);
         memset(fCodedMBs, 0, numMBsInFrame * sizeof(u_char));

         /* Initialize quantization parameter array */
         quantParams = instance->currFrame->imb->yQuantParams;
         memset(quantParams, 0, numMBsInFrame * sizeof(int));

         /* If this is the first frame and callback function has been set, report frame size */ 
         if (instance->nOfDecodedFrames == 0 && instance->reportPictureSizeCallback) {
            h263dReportPictureSizeCallback_t cb = 
               (h263dReportPictureSizeCallback_t)instance->reportPictureSizeCallback;
            cb(instance->hParent, instance->pictureParam.lumWidth, instance->pictureParam.lumHeight);
         }

         /* If picture header was ok */
         if (headerSuccess == DPH_OK)
         {
            /* Get and decode Supplemental Enhancement Information */
            int seiSuccess = dphGetSEI(instance, inBuffer, &bitErrorIndication);

            /* If fatal error while reading SEI */
            if (seiSuccess < 0) {
               retValue = VDC_ERR;
               deb("vdcDecodeFrame: dphGetSEI failed.\n");
               goto exitFunction;
            }

            /* Else if bit error while reading SEI */
            else if (seiSuccess == DPH_OK_BUT_BIT_ERROR) {
               /* We can't trust that the 1st segment can be decoded.
                  Thus, let's continue with the 2nd segment. */
               if ( instance->pictureParam.fSS )
                  expectedDecodingPosition = EDP_START_OF_SLICE_SEGMENT;
               else
                  expectedDecodingPosition = EDP_START_OF_GOB_SEGMENT;
               sncCode = sncSeekSync(inBuffer, &error);
               if (error && error != ERR_BIB_NOT_ENOUGH_DATA) {
                  retValue = VDC_ERR;
                  deb("vdcDecodeFrame: sncSeekSync failed.\n");
                  goto exitFunction;
               }
               continue;
            }


         }

         if ( hTranscoder->H263PictureHeaderEnded(&pichOut, &pichInOut) != VDC_OK )
            {
              retValue = VDC_ERR;
              goto exitFunction;
            }

         if (instance->pictureParam.fSS)
         /* Decode the 1st Slice segment - not supported */
         {
             retValue = VDC_OK_BUT_FRAME_USELESS;
             goto exitFunction;
         }
         else
         /* Decode the 1st GOB segment */
         {
            dgobGOBSegmentInParam_t dgobi;
            dgobGOBSegmentInOutParam_t dgobio;

            dgobi.numStuffBits = 0;
            dgobi.pictParam = &instance->pictureParam;
            dgobi.inBuffer = inBuffer;

            dgobi.outBuffer = outBuffer;
            dgobi.bufEdit = bufEdit;

            dgobi.iColorEffect = aColorEffect; 
            dgobi.iGetDecodedFrame=aGetDecodedFrame;
            dgobio.StartByteIndex=0;
            dgobio.StartBitIndex=7;

            /* fGFIDShouldChange not relevant here */

            dgobio.prevGNWithHeader = 0;
            dgobio.prevGN = 0;
            /* dgobio.gfid, not relevant here */
            dgobio.fCodedMBs = fCodedMBs;
            dgobio.numOfCodedMBs = numOfCodedMBs;
            dgobio.quantParams = quantParams;
            dgobio.mvcData = &instance->mvcData;
            dgobio.imageStore = instance->imageStore;
            dgobio.trp = pichOut.trp;
            dgobio.rtr = 0; /* not relevant since no reference frame exists yet */
            dgobio.refY = refYFrame;
            dgobio.refU = refUFrame;
            dgobio.refV = refVFrame;
            dgobio.currPY = currYFrame;
            dgobio.currPU = currUFrame;
            dgobio.currPV = currVFrame;

            /* the first GOB doesn't have a header */
            hTranscoder->H263GOBSliceHeaderBegin();
            hTranscoder->H263GOBSliceHeaderEnded(NULL, NULL);

            decStatus = dgobGetAndDecodeGOBSegmentContents(&dgobi, 
               instance->pictureParam.pictureType != VDX_PIC_TYPE_I,
               pichOut.pquant, &dgobio, hTranscoder);

            if (decStatus < 0) {
               retValue = VDC_ERR;
               goto exitFunction;
            }

            hTranscoder->H263OneGOBSliceWithHeaderEnded();

            prevGNWithHeader = dgobio.prevGNWithHeader;
            prevGN = dgobio.prevGN;
            numOfCodedMBs = dgobio.numOfCodedMBs;
            trp = dgobio.trp;
            rtr = dgobio.rtr;
            refYFrame = dgobio.refY;
            refUFrame = dgobio.refU;
            refVFrame = dgobio.refV;
            if (prevGN == instance->pictureParam.numGOBs - 1)
               expectedDecodingPosition = EDP_END_OF_FRAME;
            else
               expectedDecodingPosition = EDP_START_OF_GOB_SEGMENT;
         }
      }
   
      /* Else if GOB segment starts as expected */
      else if (expectedDecodingPosition == EDP_START_OF_GOB_SEGMENT &&
         sncCode == SNC_GBSC) {

         dgobGOBSegmentInParam_t dgobi;
         dgobGOBSegmentInOutParam_t dgobio;

         dgobi.numStuffBits = numStuffBits;
         dgobi.pictParam = &instance->pictureParam;
         dgobi.inBuffer = inBuffer;

         dgobi.outBuffer = outBuffer;
         dgobi.bufEdit = bufEdit;

         dgobi.iColorEffect = aColorEffect; 
         dgobi.iGetDecodedFrame= aGetDecodedFrame;
         if(prevGN==-1)
         {
             dgobio.StartByteIndex=0;
             dgobio.StartBitIndex=7;
         }
         else
         {
             dgobio.StartByteIndex=dgobi.inBuffer->getIndex;
             dgobio.StartBitIndex=dgobi.inBuffer->bitIndex;
         }


         /* fGFIDShouldChange, see below */

         dgobio.prevGNWithHeader = prevGNWithHeader;
         dgobio.prevGN = prevGN;
         /* dgobio.gfid, see below */
         dgobio.fCodedMBs = fCodedMBs;
         dgobio.numOfCodedMBs = numOfCodedMBs;
         dgobio.quantParams = quantParams;
         dgobio.mvcData = &instance->mvcData;
         dgobio.imageStore = instance->imageStore;
         dgobio.trp = trp;
         dgobio.rtr = rtr;
         dgobio.refY = refYFrame;
         dgobio.refU = refUFrame;
         dgobio.refV = refVFrame;
         dgobio.currPY = currYFrame;
         dgobio.currPU = currUFrame;
         dgobio.currPV = currVFrame;

         dgobi.fGFIDShouldChange = instance->fGFIDShouldChange;
         dgobio.gfid = instance->gfid;

         /* Get and decode GOB segment */
         decStatus = dgobGetAndDecodeGOBSegment(&dgobi, &dgobio, hTranscoder);

         if (decStatus == DGOB_ERR) {
            retValue = VDC_ERR;
            goto exitFunction;
         }
         if ( instance->fGFIDShouldChange ) {
            instance->gfid = dgobio.gfid;
            instance->fGFIDShouldChange = 0;
         }

         prevGNWithHeader = dgobio.prevGNWithHeader;
         prevGN = dgobio.prevGN;
         numOfCodedMBs = dgobio.numOfCodedMBs;
         trp = dgobio.trp;


         rtr = dgobio.rtr;

         refYFrame = dgobio.refY;
         refUFrame = dgobio.refU;
         refVFrame = dgobio.refV;

         if (prevGN == instance->pictureParam.numGOBs - 1)
            expectedDecodingPosition = EDP_END_OF_FRAME;
         else
            expectedDecodingPosition = EDP_START_OF_GOB_SEGMENT;
      }

      /* Else if Slice segment starts as expected */
      else if (expectedDecodingPosition == EDP_START_OF_SLICE_SEGMENT &&
         sncCode == SNC_GBSC) {
         /* slides not supported */
         retValue = VDC_OK_BUT_FRAME_USELESS;
         goto exitFunction;

      }

      /* Else decoding is out of sync */
      else {
         switch (expectedDecodingPosition) {

            case EDP_START_OF_FRAME:
               /* No PSC */
               /* Check if GFID could be used to recover the picture header */
               {
                  dphInParam_t pichIn;
                  dphInOutParam_t pichInOut;
                  dphOutParam_t pichOut;

                  pichIn.numStuffBits = numStuffBits;
                  pichIn.fReadBits = 0;

                  pichInOut.vdcInstance = instance;
                  pichInOut.inBuffer = inBuffer;

                  headerSuccess = dphGetPictureHeader(&pichIn, &pichInOut, &pichOut, &bitErrorIndication);
                  if ( headerSuccess == DPH_OK) {
                     /* Header recovery was successful, start decoding from the next GOB available */
                     if ( instance->pictureParam.fSS ) {
                        expectedDecodingPosition = EDP_START_OF_SLICE_SEGMENT;
                        /* decSlice does not increment these */
                        decodedSegments++;
                        corruptedSegments++;
                     }
                     else
                        expectedDecodingPosition = EDP_START_OF_GOB_SEGMENT;
                     deb1p("vdcDecodeFrame: Header successfully recovered after PSC loss. FrameNum %d\n",instance->frameNum);
                     currYFrame = pichOut.currYFrame;
                     currUFrame = pichOut.currUFrame;
                     currVFrame = pichOut.currVFrame;
                     numMBsInFrame = renDriNumOfMBs(instance->currFrame->imb->drawItem);
                     numOfCodedMBs = 0;
                     numDecodedMBs = 0;
                     fCodedMBs = renDriCodedMBs(instance->currFrame->imb->drawItem);
                     memset(fCodedMBs, 0, numMBsInFrame * sizeof(u_char));

                     /* Initialize quantization parameter array */
                     quantParams = instance->currFrame->imb->yQuantParams;
                     memset(quantParams, 0, numMBsInFrame * sizeof(int));

                     /* If this is the first frame and callback function has been set, report frame size */ 
                     if (instance->nOfDecodedFrames == 0 && instance->reportPictureSizeCallback) {
                        h263dReportPictureSizeCallback_t cb = 
                           (h263dReportPictureSizeCallback_t)instance->reportPictureSizeCallback;
                        cb(instance->hParent, instance->pictureParam.lumWidth, instance->pictureParam.lumHeight);
                     }

                     continue;
                  }
                  else {
                     retValue = VDC_OK_BUT_FRAME_USELESS;
                     deb1p("vdcDecodeFrame: Header recovery unsuccessful after PSC loss. FrameNum %d\n",instance->frameNum);
                     goto exitFunction;
                  }
               }
            case EDP_START_OF_GOB_SEGMENT:
               if ( sncCode == SNC_PSC ) {
                  retValue = VDC_OK_BUT_BIT_ERROR;
                  goto exitFunction;
               }
               else {
                  /* 
                   * Picture header recovery used next picture header 
                   * => we are not at the position of GBSC => search the next one 
                   * This is caused by a erdRestorePictureHeader that does not synchronize 
                   * bit buffer to GOB headers that have bitErrorIndication != 0.
                   * This may be considered as a , and it might be good to change it later on.
                   * This seeking should solve the problem, although maybe not in the most elegant way
                   */
                  sncSeekSync( inBuffer, &error );
                  continue;
               }
           default :
            {
            
                retValue = VDC_OK_BUT_FRAME_USELESS;
                 goto exitFunction;
            }

         }
      }
   }

   exitFunction:


   /* If frame(s) not useless */
   if (retValue == VDC_OK || retValue == VDC_OK_BUT_BIT_ERROR) {

      if (!corruptedSegments && (numDecodedMBs > 0 || prevGN > 0 || instance->pictureParam.numGOBs == 1))
         retValue = VDC_OK;
      else {
         retValue = VDC_OK_BUT_FRAME_USELESS;
         deb1p("vdcDecodeFrame: Frame useless, too many corrupted segments. FrameNum %d\n",instance->frameNum);
      }
   }

   /* If decoding ok and frame not useless */
   if ( retValue == VDC_OK || retValue == VDC_OK_BUT_BIT_ERROR ) {


       /* stuff bits here 'END OF FRAME' -->*/
         bibStuffBits(outBuffer); 
       /* <-- */

      if ( instance->nOfDecodedFrames < 0xffffffff )
         instance->nOfDecodedFrames++;

      if (vdcFillImageBuffers(
         instance, 
         numOfCodedMBs, 
         instance->currFrame->imb) < 0)
         retValue = VDC_ERR;



   }
   if ( retValue == VDC_OK_BUT_FRAME_USELESS ) {
      /* GFID of the next frame can be whatever, since this frame was useless */
      instance->gfid = -1; 
   }
   if ( instance->fGFIDShouldChange
        && (    (instance->pictureParam.fSS && decodedSegments == 1) 
             || (!instance->pictureParam.fSS && prevGNWithHeader <= 0) 
           )
      ) 
   {
      /* GFID of the next frame can be whatever, since this frame didn't have any GFID's and GFID was supposed to change */
      instance->gfid = -1; 
   }

   /* If a fatal error occurred */
   if (retValue < 0) {
      /* Return frame buffers for decoded output images,
         as they are useless for the caller and 
         as the caller cannot get a handle to return them */
      if (instance->currFrame)
         vdeImsPutFree(instance->imageStore, instance->currFrame);
   }

   return retValue;
}


/* {{-output"vdcDecodePictureHeader.txt"}} */
/*
 * vdcDecodePictureHeader
 *
 * Parameters:
 *    hInstance                  I: handle of instance data
 *                               (May set instance->errorVar if bit errors.)
 *
 * Function:
 *    
 *
 * Note:
 *    This function does not recover corrupted picture headers (by means of
 *    GFID or redundant picture header copies).
 *
 * Returns:
 *    See above.
 */

int vdcDecodePictureHeader(
   vdcHInstance_t hInstance,
   bibBuffer_t *inBuffer,
   vdxPictureHeader_t *header,
   vdxSEI_t *sei)
{
   int
      retValue = VDC_OK, 
      sncCode,
      numStuffBits,
      bitErrorIndication = 0;

   int16
      error = 0;

   u_int32
      pictureStartPosition;

   vdcInstance_t 
      *instance = (vdcInstance_t *) hInstance;

   /* The function is implemented by calling stateless Video Demultiplexer
      (vdx) functions. Consequently, the function does not have sophisticated
      logic to track illegal/corrupted parameters based on the previous
      picture header. It cannot recover corrupted picture headers either.

      Alternatively, the function could have been implemented by creating
      an identical copy of the Video Decoder Core instance and by using
      "Decode Picture Header" (dph) module functions. However, as there is
      no instance copying functions implemented, the former alternative
      was chosen.

      This function was targeted for getting the picture type and 
      Nokia-proprietary Annex N scalability layer information encapsulated
      in Supplemental Enhancement Information in an error-free situation.
      For this purpose, the former and simpler solution is more than 
      adequate. */

   pictureStartPosition = bibNumberOfFlushedBits(inBuffer);
   sncCode = sncCheckSync(inBuffer, &numStuffBits, &error);

   /* If sncCheckSync failed */
   if (error) {
      deb1p("vdcDecodeFrame: ERROR - sncCheckSync returned %d.\n", error);
      retValue = VDC_ERR;
      goto exitFunction;
   }

   /* Else if EOS was got */
   else if (sncCode == SNC_EOS) {
      retValue = VDC_OK_BUT_FRAME_USELESS;
      goto exitFunction;
   }

   /* Else if frame starts as expected */
   else if (sncCode == SNC_PSC) {
      int
         picHdrStatus,
         seiStatus;

      vdxGetPictureHeaderInputParam_t
         picHdrIn;

      picHdrIn.numStuffBits = numStuffBits;
      picHdrIn.fCustomPCF = 0;
      picHdrIn.fScalabilityMode = 0;
      picHdrIn.fRPS = instance->fRPS;
      picHdrIn.flushBits = bibFlushBits;
      picHdrIn.getBits = bibGetBits;
      picHdrIn.showBits = bibShowBits;

      /* Get picture header */
      picHdrStatus = vdxGetPictureHeader(
         inBuffer, 
         &picHdrIn,
         header,
         &bitErrorIndication);

      /* If the header was not successfully retrieved */
      if (picHdrStatus < 0) {
         retValue = VDC_ERR;
         goto exitFunction;
      }
      else if (picHdrStatus != VDX_OK) {
         retValue = VDC_OK_BUT_BIT_ERROR;
         goto exitFunction;
      }

      /* Get and decode Supplemental Enhancement Information */
      seiStatus = vdxGetAndParseSEI(
         inBuffer,
         instance->pictureParam.fPLUSPTYPE,
         instance->numAnnexNScalabilityLayers,
         sei,
         &bitErrorIndication);

      /* If error while reading SEI */
      if (seiStatus < 0) {
         retValue = VDC_ERR;
         goto exitFunction;
      }
      else if (seiStatus != VDX_OK) {
         retValue = VDC_OK_BUT_BIT_ERROR;
         goto exitFunction;
      }
   }

   /* Else no valid frame start */
   else
      retValue = VDC_OK_BUT_FRAME_USELESS;

   exitFunction:

   /* Reset the bit buffer to its original position */
   bibRewindBits(
      bibNumberOfFlushedBits(inBuffer) - pictureStartPosition,
      inBuffer, &error);

   if (error)
      retValue = VDC_ERR;

   return retValue;
}


/* {{-output"vdcFree.txt"}} */
/*
 * vdcFree
 *    
 *
 * Parameters:
 *    None.
 *
 * Function:
 *    This function deinitializes the Video Decoder Core module.
 *    Any functions of this module must not be called after vdcFree (except 
 *    vdcLoad).
 *
 * Returns:
 *    >= 0  if succeeded
 *    < 0   if failed
 *
 *    
 */

int vdcFree(void)
/* {{-output"vdcFree.txt"}} */
{
   if (dblFree() < 0)
      return VDC_ERR;

   return VDC_OK;
}


/* {{-output"vdcGetImsItem.txt"}} */
/*
 * vdcGetImsItem
 *    
 *
 * Parameters:
 *    hInstance                  handle of instance data
 *    index                      output frame number,
 *                               should be 0 for I and P frames
 *
 * Function:
 *    This function returns a pointer to the requested output frame.
 *
 * Returns:
 *    a pointer to a image store item which corresponds to the passed output
 *    frame index, or
 *    NULL if the function fails
 *
 */

vdeImsItem_t *vdcGetImsItem(vdcHInstance_t hInstance, int index)
/* {{-output"vdcGetImsItem.txt"}} */
{
   vdcInstance_t *instance = (vdcInstance_t *) hInstance;
   int numOutputFrames = vdcGetNumberOfOutputFrames(hInstance);

   vdcAssert(instance);
   vdcAssert(index >= 0);

   if (index >= numOutputFrames)
      return NULL;

   return instance->currFrame;
}

   
/* {{-output"vdcGetNumberOfAnnexNScalabilityLayers.txt"}} */
/*
 * vdcGetNumberOfAnnexNScalabilityLayers
 *
 * Parameters:
 *    hInstance                  I: handle of instance data
 *
 * Function:
 *    Returns the number of Nokia-proprietary Annex N temporal scalability
 *    layers.
 *
 * Returns:
 *    See above.
 */

int vdcGetNumberOfAnnexNScalabilityLayers(
   vdcHInstance_t hInstance)
/* {{-output"vdcGetNumberOfAnnexNScalabilityLayers.txt"}} */
{
   vdcInstance_t *instance = (vdcInstance_t *) hInstance;

   vdcAssert(instance);

   return instance->numAnnexNScalabilityLayers;
}


/* {{-output"vdcGetNumberOfOutputFrames.txt"}} */
/*
 * vdcGetNumberOfOutputFrames
 *    
 *
 * Parameters:
 *    hInstance                  handle of instance data
 *
 * Function:
 *    This function returns the number of output frames which were produced
 *    during the latest vdcDecodeFrame. 
 *
 * Returns:
 *    0 if vdcDecodeFrame failed to produce any output frames
 *    1 for I and P frames
 *
 */

int vdcGetNumberOfOutputFrames(vdcHInstance_t hInstance)
/* {{-output"vdcGetNumberOfOutputFrames.txt"}} */
{
   vdcInstance_t *instance = (vdcInstance_t *) hInstance;

   vdcAssert(instance);

   return ((instance->currFrame) ? 1 : 0);
}


/* {{-output"vdcGetTR.txt"}} */
/*
 * vdcGetTR
 *    
 *
 * Parameters:
 *    inpBuffer                  buffer containing the frame data,
 *                               must start with a PSC
 *    tr                         temporal reference
 *
 * Function:
 *    Gets the temporal reference field from the input buffer.
 *    Notice that the validity of the bitstream is not checked.
 *    This function does not support enhanced temporal reference
 *    (ETR) defined in section 5.1.8 of the H.263 recommendation.
 *
 * Returns:
 *    Nothing
 *
 *    
 */

void vdcGetTR(void *inpBuffer, u_int8 *tr)
/* {{-output"vdcGetTR.txt"}} */
{
   const u_char 
      *tmpBuffer = (u_char *) inpBuffer;

   *tr = (u_int8) (((tmpBuffer[2] & 2) << 6) | ((tmpBuffer[3] & 252) >> 2));
}


/* {{-output"vdcIsEOSReached.txt"}} */
/*
 * vdcIsEOSReached
 *    
 *
 * Parameters:
 *    hInstance                  handle of instance data
 *
 * Function:
 *    This function returns 1 if the EOS code has been reached during
 *    the decoding. Otherwise, it returns 0.
 *
 * Returns:
 *    See above.
 *
 */

int vdcIsEOSReached(vdcHInstance_t hInstance)
/* {{-output"vdcIsEOSReached.txt"}} */
{
   vdcInstance_t *instance = (vdcInstance_t *) hInstance;

   vdcAssert(instance);

   return instance->fEOS;
}


/* {{-output"vdcIsINTRA.txt"}} */
/*
 * vdcIsINTRA
 *    
 *
 * Parameters:
 *    hInstance                  handle of instance data
 *    frameStart                 pointer to memory chunk containing a frame
 *    frameLength                number of bytes in frame
 *
 * Function:
 *    This function returns 1 if the passed frame is an INTRA frame.
 *    Otherwise the function returns 0.
 *
 * Returns:
 *    See above.
 *
 *    
 */

int vdcIsINTRA(
   vdcHInstance_t hInstance,
   void *frameStart,
   unsigned frameLength)
/* {{-output"vdcIsINTRA.txt"}} */
{
   bibBuffer_t *tmpBitBuffer;
   int fINTRA = 0, bitErrorIndication, syncCode, vdxStatus;
   int16 error = 0;
   vdcInstance_t *instance = (vdcInstance_t *) hInstance;
   vdxGetPictureHeaderInputParam_t vdxIn;
   vdxPictureHeader_t vdxOut;

   vdcAssert(instance);

   tmpBitBuffer = bibCreate(frameStart, frameLength, &error);
   if (!tmpBitBuffer || error)
      return 0;

   syncCode = sncCheckSync(tmpBitBuffer, &(vdxIn.numStuffBits), &error);

   if (syncCode == SNC_PSC && error == 0) {

      /* Note: Needs to be changed when support for custom PCF or scalability mode
         is added */
      vdxIn.fCustomPCF = 0; 
      vdxIn.fScalabilityMode = 0;
      vdxIn.fRPS = instance->fRPS;
      vdxIn.flushBits = bibFlushBits;
      vdxIn.getBits = bibGetBits;
      vdxIn.showBits = bibShowBits;

      vdxStatus = vdxGetPictureHeader(tmpBitBuffer, &vdxIn, &vdxOut, 
         &bitErrorIndication);

      if (vdxStatus >= 0 && bitErrorIndication == 0)
         fINTRA = (vdxOut.pictureType == VDX_PIC_TYPE_I);
   }

   bibDelete(tmpBitBuffer, &error);

   return fINTRA;
}


/* {{-output"vdcIsINTRAGot.txt"}} */
/*
 * vdcIsINTRAGot
 *    
 *
 * Parameters:
 *    hInstance                  handle of instance data
 *
 * Function:
 *    This function returns 1 if the an INTRA frame has been decoded
 *    during the lifetime of the instance. Otherwise, it returns 0.
 *
 * Returns:
 *    See above.
 *
 *    
 */

int vdcIsINTRAGot(vdcHInstance_t hInstance)
/* {{-output"vdcIsEOSReached.txt"}} */
{
   vdcInstance_t *instance = (vdcInstance_t *) hInstance;

   vdcAssert(instance);

   return instance->fIntraGot;
}


/* {{-output"vdcLoad.txt"}} */
/*
 * vdcLoad
 *    
 *
 * Parameters:
 *    None.
 *
 * Function:
 *    This function initializes the Video Decoder Core module meaning
 *    all the data common to all Video Decoder Core instances.
 *    vdcLoad has to be called before any other function of this module
 *    is used.
 *
 * Returns:
 *    >= 0  if succeeded
 *    < 0   if failed
 *
 *    
 */

int vdcLoad(void)
/* {{-output"vdcLoad.txt"}} */
{
   if (dblLoad() < 0)
      return VDC_ERR;

   return VDC_OK;
}


/* {{-output"vdcOpen.txt"}} */
/*
 * vdcOpen
 *    
 *
 * Parameters:
 *    imageStore                 pointer to image store instance
 *    numReferenceFrames         1 if the Reference Picture Selection (RPS)
 *                               mode (Annex N) should not be used,
 *                               >1 tells how many reference images
 *                               are stored in the RPS mode
 *    fudInstance                pointer to Fast Update module instance
 *    hParent                    handle of Video Decoder Engine instance
 *                               that created this VDC instance
 *
 * Function:
 *    This function creates and initializes a new Video Decoder Core instance.
 *
 * Returns:
 *    a handle to the created instance,
 *    or NULL if the function fails
 *
 *    
 */

vdcHInstance_t vdcOpen(
   vdeIms_t *imageStore, 
   int numReferenceFrames,
   void *hParent)
{
   vdcInstance_t *instance;

   vdcAssert(numReferenceFrames >= 1);

   instance = (vdcInstance_t *) vdcMalloc(sizeof(vdcInstance_t));
   if (!instance)
      return NULL;
   memset(instance, 0, sizeof(vdcInstance_t));

   instance->prevPicHeader = (vdxPictureHeader_t *)vdcMalloc( sizeof( vdxPictureHeader_t ));
   if ( instance->prevPicHeader == NULL ) {
      deb("vdcOpen, MALLOC for prevPicHeader failed.\n");
      goto errPHOpen;
   }
   instance->fPrevPicHeaderReliable = 1;

   instance->imageStore = imageStore;

   instance->numAnnexNScalabilityLayers = -1; /* Indicates no decoded frames */

   instance->fGFIDShouldChange = 0;
   instance->gfid = -1;

   instance->nOfDecodedFrames = 0;

   instance->hParent = hParent;
   instance->snapshotStatus = -1;

   return (vdcHInstance_t) instance;

   /* Error cases: release everything in reverse order */
   errPHOpen:

   vdcDealloc(instance);
   return NULL;
}
/* {{-output"vdcOpen.txt"}} */


/* {{-output"vdcRestartVideo.txt"}} */
/*
 * vdcRestartVideo
 *    
 *
 * Parameters:
 *    hInstance                  handle of instance data
 *
 * Function:
 *    Resets the instance data but does not deallocate the allocated buffers.
 *    After this function vdcDecodeFrame can be called as if no data for this
 *    instance has been decoded.
 *
 * Note:
 *    This function is obsolete and not used anymore. If it is needed again,
 *    it should be re-implemented.
 *
 * Returns:
 *    Nothing
 *
 *    
 */

void vdcRestartVideo(vdcHInstance_t hInstance)
/* {{-output"vdcRestartVideo.txt"}} */
{
   vdcInstance_t *instance = (vdcInstance_t *) hInstance;

   if (instance)
      memset(instance, 0, sizeof(vdcInstance_t));
}





/*
 * Local functions
 */

/*
 * vdcFillCommonPartsOfImb
 *    
 *
 * Parameters:
 *    instance                   instance data
 *    numOfCodedMBs              number of coded macroblocks
 *    imb                        pointer to image buffer to fill
 *
 * Function:
 *    This function fills the passed image buffer according to the latest
 *    decoding results. Only those parts of the image buffer are filled
 *    which can be composed with the presence of another image buffer,
 *    e.g. the P image buffer corresponding to the B image buffer.
 *
 * Returns:
 *    Nothing
 *
 *    
 */

static void vdcFillCommonPartsOfImb(
   vdcInstance_t *instance, 
   int numOfCodedMBs,
   vdeImb_t *imb)
{
   vdcAssert(imb);
   vdcAssert(instance);

   imb->drawItem->param.dwFlags = 0;
   imb->drawItem->param.lTime = instance->frameNum;

   /* Note: for now, convert whole frame */
   imb->drawItem->extParam.flags = 0;
   /* Else one could convert just coded MBs as follows: */
      /* imb->drawItem->extParam.flags = (fBPart) ? 0 : REN_EXTDRAW_NEW_SOURCE; */

   imb->drawItem->extParam.rate = 30000;
   imb->drawItem->extParam.scale = 1001;
   imb->drawItem->extParam.numOfCodedMBs = numOfCodedMBs;

   /* imb->drawItem->retFrame and imb->drawItem->retFrameParam are set by
      the caller */

   imb->fReferenced = 1;
   imb->tr = instance->pictureParam.tr;
}


/*
 * vdcFillImageBuffers
 *    
 *
 * Parameters:
 *    instance                   instance data
 *    numOfCodedMBs              number of coded macroblocks
 *    imbP                       pointer to image buffer of P or I frame
 *
 * Function:
 *    This function fills the passed image buffers according to the latest
 *    decoding results.
 *
 * Returns:
 *    >= 0                       if the function was successful
 *    < 0                        if an error occured
 *
 *    
 */

/* Codec and renderer dependent */
int vdcFillImageBuffers(
   vdcInstance_t *instance, 
   int numOfCodedMBs,
   vdeImb_t *imbP)
{
   /* Table to convert from lumiance quantization parameter to chrominance
      quantization parameter if Modified Quantization (Annex T) is in use. */
   static const int yToUVQuantizer[32] = 
      {0,1,2,3,4,5,6,6,7,8,9,9,10,10,11,11,12,
       12,12,13,13,13,14,14,14,14,14,15,15,15,
       15,15};

   int
      firstQPIndex,        /* the index of the first non-zero quantizer */

      *yQuantParams = imbP->yQuantParams,
                           /* array of Y quantizers in scan-order */

      *uvQuantParams = imbP->uvQuantParams,
                           /* array of UV quantizers in scan-order */

      numMBsInPicture,     /* number of macroblocks in the image */

      i;                   /* loop variable */

   vdcAssert(imbP);
   vdcAssert(instance);

   /*
    * Fill basic stuff
    */

   vdcFillCommonPartsOfImb(instance, numOfCodedMBs, imbP);

   /*
    * Calculate quantizer arrays for loop/post-filtering
    */

   numMBsInPicture = instance->pictureParam.lumMemWidth * 
      instance->pictureParam.lumMemHeight / 256;

   /* Get the index of the first non-zero quantizer in scan-order */
   for (firstQPIndex = 0; firstQPIndex < numMBsInPicture; firstQPIndex++) {
      if (yQuantParams[firstQPIndex] > 0)
         break;
   }

   /* Assert that at least one macroblock is decoded successfully */
   vdcAssert(firstQPIndex < numMBsInPicture);

   /* Replace the first zero quantizers with the first non-zero quantizer
      (in scan-order) */
   for (i = 0; i < firstQPIndex; i++)
      yQuantParams[i] = yQuantParams[firstQPIndex];

   /* Replace all other zero quantizers with the predecessor (in scan-order) */
   for (i = firstQPIndex; i < numMBsInPicture; i++) {
      if (yQuantParams[i] == 0)
         yQuantParams[i] = yQuantParams[i - 1];
   }

   /* If Modified Quantization is in use */
   if (instance->pictureParam.fMQ) {
      /* Convert Y quantizers to UV quantizers */
      for (i = 0; i < numMBsInPicture; i++)
         uvQuantParams[i] = yToUVQuantizer[yQuantParams[i]];
   }
   else
      /* Copy Y quantizers to UV quantizers */
      memcpy(uvQuantParams, yQuantParams, numMBsInPicture * sizeof(int));


   return VDC_OK;
}

// End of File
