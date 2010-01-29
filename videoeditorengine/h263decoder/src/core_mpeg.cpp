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
* MPEG-4 decoder core functions.
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
#include "decvp_mpeg.h"
#include "decpich.h"
#include "h263dapi.h" /* for H263D_BC_MUX_MODE_SEPARATE_CHANNEL and H263D_ERD_ */
#include "stckheap.h"
#include "sync.h"
#include "vdeims.h"
#include "vdeimb.h"
#include "viddemux.h"
#include "biblin.h"
/* MVE */
#include "MPEG4Transcoder.h"


/*
 * Typedefs and structs
 */

/* This structure is used to indicate the expected decoding position. */
typedef enum {
   EDP_START_OF_FRAME,
   EDP_START_OF_VIDEO_PACKET,
   EDP_END_OF_FRAME
} vdcExpectedDecodingPosition_t;


/*
 * Local function prototypes
 */ 

extern int vdcFillImageBuffers(
   vdcInstance_t *instance,
   int numOfCodedMBs,
   vdeImb_t *imbP);


/*
 * Global functions
 */

/* {{-output"vdcDecodeMPEGVolHeader.txt"}} */
/*
 * vdcDecodeMPEGVolHeader
 *    
 *
 * Parameters:
 *    None.
 *
 * Function:
 *    This function reads the VOL Header and updates the instance data
 *    with the read parameters.
 *
 * Returns:
 *    >= 0  if succeeded
 *    < 0   if failed
 *
 */

int vdcDecodeMPEGVolHeader(vdcHInstance_t hInstance, bibBuffer_t *inBuffer, CMPEG4Transcoder *hTranscoder)
/* {{-output"vdcDecodeMPEGVolHeader.txt"}} */
{
    dphInOutParam_t pichInOut;
    int headerSuccess = 0;

    pichInOut.vdcInstance = (vdcInstance_t *) hInstance;
    pichInOut.inBuffer = inBuffer;

    headerSuccess = dphGetMPEGVolHeader(&pichInOut, hTranscoder);
    if (headerSuccess != 0)
        return VDC_ERR;
    else 
        return VDC_OK;
}


/* {{-output"vdcDecodeMPEGVop.txt"}} */
/*
 * vdcDecodeMPEGVop
 *    
 *
 * Parameters:
 *    hInstance                  handle of instance data
 *
 *    inBuffer                   pointer to bit buffer, the current position
 *                               of the buffer must start with a PSC
 *
 * Function:
 *    The vdcDecodeMPEGVop function implements the decoding process of the MPEG-4
 *    Simple Video Object described in ISO/IEC 14496-2. 
 *
 *    The function decodes the next frame in the buffer (inBuffer) meaning that
 *    the decoding continues until the next VOP start code or EOB is found or
 *    until the end of the buffer is not reached.
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
int vdcDecodeMPEGVop(vdcHInstance_t hInstance, bibBuffer_t *inBuffer, bibBuffer_t *outBuffer,
                     bibBufferEdit_t *bufEdit, int aColorEffect, TBool aGetDecodedFrame, 
                     int aStartByteIndex, int aStartBitIndex,
                     CMPEG4Transcoder *hTranscoder)
/* {{-output"vdcDecodeMPEGVop.txt"}} */
{
   int sncCode;               /* the latest synchronization code, see 
                                 sncCheckSync for the possible values */

   int retValue = VDC_OK;     /* return value of this function */

   int16 error = 0;

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

   int currMBNum = 0;       /* Current macro block */

   int decStatus = 0;
   int corruptedVPs = 0;
   int numberOfVPs = 1;
   int headerSuccess = 0;
   u_char fVOPHeaderCorrupted = 0, fVOPHeaderLost = 0;

   vdcExpectedDecodingPosition_t expectedDecodingPosition;
                              /* Tells in which part of the bitstream
                                 the decoding should be */

   vdcInstance_t *instance = (vdcInstance_t *) hInstance;
                              /* instance data */
      
   vdcAssert(instance != NULL);

   /* Initializations */

   instance->currFrame = NULL;
   expectedDecodingPosition = EDP_START_OF_FRAME;

   /* Main loop */
   for (;;) {
      int bitErrorIndication = 0;

      sncCode = sncCheckMpegSync(inBuffer, instance->pictureParam.fcode_forward, &error);

      /* If sncCheckSync failed */
      if (error && error != ERR_BIB_NOT_ENOUGH_DATA) {
         deb1p("vdcDecodeMPEGVop: ERROR - sncCheckSync returned %d.\n", error);
         retValue = VDC_ERR;
         goto exitFunction;
      }

      /* If EOS was got */
      if (sncCode == SNC_EOB)
         instance->fEOS = 1;

      /* If frame ends appropriately */
      if (expectedDecodingPosition == EDP_END_OF_FRAME &&
         (sncCode == SNC_VOP || sncCode == SNC_GOV || sncCode == SNC_EOB || 
          sncCode == SNC_STUFFING || error == ERR_BIB_NOT_ENOUGH_DATA))
         goto exitFunction;

      /* Else if frame (or stream) data ends suddenly */
      else if (error == ERR_BIB_NOT_ENOUGH_DATA) {
         retValue = VDC_OK_BUT_BIT_ERROR;
         goto exitFunction;
      }

      /* Else if EOS was reached */
      else if (sncCode == SNC_EOB) {
         /* The current frame is useless since it ends before it is complete.
            On the other hand, there is no point in concealing it since
            it is the last frame of the sequence. */
         goto exitFunction;
      }

      /* Else if frame starts as expected */
      else if (expectedDecodingPosition == EDP_START_OF_FRAME &&
         ((sncCode == SNC_GOV) || (sncCode == SNC_VOP) || fVOPHeaderLost)) {

         dphInParam_t pichIn;
         dphInOutParam_t pichInOut;
         dphOutParam_t pichOut;

         if (sncCode == SNC_GOV) {
              vdxGovHeader_t govHeader;

              headerSuccess = vdxGetGovHeader(inBuffer, &govHeader, &bitErrorIndication);
              if ( headerSuccess < 0) {
                  retValue = VDC_ERR;
                  goto exitFunction;
              } else if (( headerSuccess > 0 ) || 
                  (govHeader.time_stamp < instance->pictureParam.mod_time_base) || 
                  (govHeader.time_stamp - instance->pictureParam.mod_time_base > 60)) {
                  
                  if(sncCheckMpegVOP(inBuffer, &error) != SNC_PSC) {
                      retValue = VDC_OK_BUT_FRAME_USELESS;
                      goto exitFunction;
                  }

              } else {
                  instance->pictureParam.mod_time_base = govHeader.time_stamp;

                  /* copying the user data */
                  if (govHeader.user_data != NULL) {

                      if (!instance->user_data)
                          instance->user_data = (char *) vdcMalloc(
                              MAX_USER_DATA_LENGTH);

                      govHeader.user_data_length = 
                          ((instance->user_data_length + govHeader.user_data_length) >= MAX_USER_DATA_LENGTH) ?
                          (MAX_USER_DATA_LENGTH - instance->user_data_length) : govHeader.user_data_length;
                      memcpy(
                          instance->user_data + instance->user_data_length, 
                          govHeader.user_data, 
                          govHeader.user_data_length);
                      instance->user_data_length += govHeader.user_data_length;

                      vdcDealloc(govHeader.user_data);
                  }

              }
         }

         /* Start VOP decoding */
         pichIn.fReadBits = (fVOPHeaderLost ? 0 : 1);
         pichIn.fNeedDecodedFrame = aGetDecodedFrame | hTranscoder->NeedDecodedYUVFrame();
         pichInOut.vdcInstance = instance;
         pichInOut.inBuffer = inBuffer;

         /* Get VOP header */
         headerSuccess = dphGetMPEGVopHeader(&pichIn, &pichInOut, &pichOut, &bitErrorIndication);
         
         deb1p("vdcDecodeMPEGVop: frameNum %d.\n", instance->frameNum);
         if ( headerSuccess < 0) {
             retValue = VDC_ERR;
             goto exitFunction;
         } else if ( headerSuccess > 0 ) {

             if (headerSuccess == DPH_OK_BUT_BIT_ERROR) {
                 /* find the next resync marker, to get the number of MBs in the current VP */
                 sncCode = sncRewindAndSeekNewMPEGSync(-1, inBuffer, instance->pictureParam.fcode_forward, &error);
                 if (error && error != ERR_BIB_NOT_ENOUGH_DATA) {
                     retValue = VDC_ERR;
                     goto exitFunction;
                 }

                 if (sncCode == SNC_VIDPACK) {
                     fVOPHeaderCorrupted = 1;
                 } else {
                     retValue = VDC_OK_BUT_FRAME_USELESS;
                     goto exitFunction;
                 }

             } else if (headerSuccess == DPH_OK_BUT_NOT_CODED) {
                             
                /* MVE */
                /* copy VOP header to output buffer in case VOP is not coded, including the GOV */
                bufEdit->copyMode = CopyWhole; /* copyWhole */
                CopyStream(inBuffer,outBuffer,bufEdit, aStartByteIndex, aStartBitIndex);
                             
                /* behaves the same as if the frame was useless, but it's just not coded */
                retValue = VDC_OK_BUT_NOT_CODED;
                goto exitFunction;
             }
         }


         currYFrame = pichOut.currYFrame;
         currUFrame = pichOut.currUFrame;
         currVFrame = pichOut.currVFrame;

         currMBNum = 0;

         numOfCodedMBs = 0;
         fCodedMBs = renDriCodedMBs(instance->currFrame->imb->drawItem);
         memset(fCodedMBs, 0, renDriNumOfMBs(
            instance->currFrame->imb->drawItem) * sizeof(u_char));

         /* Initialize quantization parameter array */
         quantParams = instance->currFrame->imb->yQuantParams;
         memset(quantParams, 0, renDriNumOfMBs(
            instance->currFrame->imb->drawItem) * sizeof(int));

         /* If this is the first frame and callback function has been set, report frame size */ 
         if (instance->nOfDecodedFrames == 0 && instance->reportPictureSizeCallback) {
            h263dReportPictureSizeCallback_t cb = 
               (h263dReportPictureSizeCallback_t)instance->reportPictureSizeCallback;
            cb(instance->hParent, instance->pictureParam.lumWidth, instance->pictureParam.lumHeight);
         }

         /* Decode the 1st VP segment */
         {
            dvpVPInParam_t dvpi;
            dvpVPInOutParam_t dvpio;

            dvpi.pictParam = &instance->pictureParam;
            dvpi.inBuffer = inBuffer;
            dvpi.outBuffer = outBuffer;
            dvpi.bufEdit = bufEdit;
            dvpi.iColorEffect = aColorEffect;
            dvpi.iGetDecodedFrame = aGetDecodedFrame;

            if (fVOPHeaderLost) fVOPHeaderCorrupted = 1;
            dvpi.fVOPHeaderCorrupted = fVOPHeaderCorrupted;
            
            dvpio.currMBNum = 0;
            dvpio.quant = pichOut.pquant;
            dvpio.fCodedMBs = fCodedMBs;
            dvpio.numOfCodedMBs = numOfCodedMBs;
            dvpio.quantParams = quantParams;
            dvpio.mvcData = &instance->mvcData;
            dvpio.aicData = &instance->aicData;
            dvpio.imageStore = instance->imageStore;
            dvpio.frameNum = instance->frameNum;
    
            dvpio.refY = refYFrame;
            dvpio.refU = refUFrame;
            dvpio.refV = refVFrame;
            dvpio.currPY = currYFrame;
            dvpio.currPU = currUFrame;
            dvpio.currPV = currVFrame;
            
            /* MVE */
            hTranscoder->VOPHeaderEnded(aStartByteIndex, aStartBitIndex,
                pichOut.pquant, instance->pictureParam.pictureType,
                instance->frameNum, headerSuccess == DPH_OK_BUT_NOT_CODED);
            /* VOP header parsing success, begin on VP */
            hTranscoder->BeginOneVideoPacket(&dvpi);
            hTranscoder->AfterVideoPacketHeader(&dvpio); // the first VP does not have VPHeader

            decStatus = dvpGetAndDecodeVideoPacketContents(&dvpi,
                instance->pictureParam.pictureType != VDX_PIC_TYPE_I,
                &dvpio, hTranscoder);

            if (decStatus < 0) {
                retValue = VDC_ERR;
                goto exitFunction;
            } else if (decStatus > 0 ) {
                if (decStatus == DGOB_OK_BUT_FRAME_USELESS) {
                    retValue = VDC_OK_BUT_FRAME_USELESS;
                    goto exitFunction;
                }
                corruptedVPs ++;
            }
                        
            /* MVE */
            /* the first VP ends */
            hTranscoder->OneVPEnded();

            currMBNum = dvpio.currMBNum;
            numOfCodedMBs = dvpio.numOfCodedMBs;
            refYFrame = dvpio.refY;
            refUFrame = dvpio.refU;
            refVFrame = dvpio.refV;
            

            if ((decStatus == DGOB_OK && currMBNum == instance->pictureParam.numMBsInGOB) || 
                (decStatus == DGOB_OK_BUT_BIT_ERROR && sncCheckMpegVOP(inBuffer, &error) == SNC_PSC))
                expectedDecodingPosition = EDP_END_OF_FRAME;
            else
                expectedDecodingPosition = EDP_START_OF_VIDEO_PACKET;
         }
      }
   
      /* Else if Video Packet starts as expected */
      else if (expectedDecodingPosition == EDP_START_OF_VIDEO_PACKET &&
         sncCode == SNC_VIDPACK) {

            dvpVPInParam_t dvpi;
            dvpVPInOutParam_t dvpio;

            dvpi.pictParam = &instance->pictureParam;
            dvpi.inBuffer = inBuffer;
            dvpi.outBuffer = outBuffer;
            dvpi.bufEdit = bufEdit;
            dvpi.iColorEffect = aColorEffect;
            dvpi.iGetDecodedFrame = aGetDecodedFrame;
            dvpi.fVOPHeaderCorrupted = fVOPHeaderCorrupted;
            
            dvpio.currMBNum = currMBNum;
            dvpio.fCodedMBs = fCodedMBs;
            dvpio.numOfCodedMBs = numOfCodedMBs;
            dvpio.quantParams = quantParams;
            dvpio.mvcData = &instance->mvcData;
            dvpio.aicData = &instance->aicData;
            dvpio.imageStore = instance->imageStore;

            dvpio.refY = refYFrame;
            dvpio.refU = refUFrame;
            dvpio.refV = refVFrame;
            dvpio.currPY = currYFrame;
            dvpio.currPU = currUFrame;
            dvpio.currPV = currVFrame;
            
            /* MVE */
            /* VOP header parsing success, begin on VP */
            hTranscoder->BeginOneVideoPacket(&dvpi);

            /* if the VOP header data needs to be corrected from the HEC codes set
            inParam->fVOPHeaderCorrupted and pictParam values will be set + read
            inOutParam->frameNum into instance->frameNum */
            decStatus = dvpGetAndDecodeVideoPacketHeader(&dvpi, &dvpio);
                        
            /* MVE */
            /* After parsing the VP header */
            hTranscoder->AfterVideoPacketHeader(&dvpio);
                        
            if (decStatus < 0) {
                retValue = VDC_ERR;
                goto exitFunction;
            } else if (decStatus > 0) {
                
                if (fVOPHeaderCorrupted) {
                    /* this is also true when the whole packet with the VOP header is lost */

                    if (dvpio.frameNum <= instance->frameNum) {
                        /* VOP header could not be recovered from HEC (there was no HEC)
                           or there was an error in the VP header: in this case should we try to recover 
                           VOP header from the next VP instead of the exiting here? */
                        retValue = VDC_OK_BUT_FRAME_USELESS;
                        goto exitFunction;

                    } else {
                        /* VOP header was succesfully recovered from HEC, 
                           the first VP is treated as corrupted or lost */
                        instance->frameNum = dvpio.frameNum;

                    }
                } else if (currMBNum < dvpio.currMBNum) {
                    /* when there was no bit-error in the VP header and the MB counter shows difference 
                       we know, that a whole VP was lost */
                    
                    corruptedVPs++;
                    numberOfVPs++;
                    

                }
            }
            
            numberOfVPs++;

            if (!decStatus) {

           
                decStatus = dvpGetAndDecodeVideoPacketContents(&dvpi,0,&dvpio, hTranscoder);
                if (decStatus < 0) {
                    retValue = VDC_ERR;
                    goto exitFunction;
                } else if (decStatus > 0 ) {
                    if (decStatus == DGOB_OK_BUT_FRAME_USELESS) {
                        retValue = VDC_OK_BUT_FRAME_USELESS;
                        goto exitFunction;
                    }
                    corruptedVPs++;
                }
                
            }
                        
            /* MVE */
            hTranscoder->OneVPEnded();

            currMBNum = dvpio.currMBNum;
            numOfCodedMBs = dvpio.numOfCodedMBs;
                        
            if ((decStatus == DGOB_OK && currMBNum == instance->pictureParam.numMBsInGOB) || 
                            (decStatus == DGOB_OK_BUT_BIT_ERROR && sncCheckMpegVOP(inBuffer, &error) == SNC_PSC))
                expectedDecodingPosition = EDP_END_OF_FRAME;
            else {
                expectedDecodingPosition = EDP_START_OF_VIDEO_PACKET;
                if (fVOPHeaderCorrupted) fVOPHeaderCorrupted=0;
            }
      }
            
      /* Else decoding is out of sync */
      else {
         switch (expectedDecodingPosition) {

            case EDP_START_OF_FRAME:
                if (sncCode == SNC_VIDPACK) {
                    /* VP start code instead of VOP start code -> 
                       packet including VOP header is lost */                    
                    fVOPHeaderLost = 1;
                    continue;                    
                } else {
                    /* No start code */
                    retValue = VDC_OK_BUT_FRAME_USELESS;
                    goto exitFunction;
                }

            case EDP_START_OF_VIDEO_PACKET:
               /* If the decoding gets out of sync, the next sync code is
                  seeked in dvpGetAndDecodeVideoPacketContents. Then, if 
                  the frame ends instead of a new VP header, we are here. */

                /* Mark the missing VP corrupted */
                {
                    numberOfVPs++;
                    corruptedVPs++;
                    
                }

                retValue = VDC_OK_BUT_BIT_ERROR;
                goto exitFunction;

            case EDP_END_OF_FRAME:
               /* Too much data */
               retValue = VDC_OK_BUT_BIT_ERROR;
               goto exitFunction;
         }
      }
   }


     
exitFunction:
     
    /* MVE */
    hTranscoder->VOPEnded();

    if (sncCheckMpegSync(inBuffer, instance->pictureParam.fcode_forward, &error) == SNC_EOB) {
        instance->fEOS = 1;
    }

   /* If frame(s) not useless */
   if (retValue == VDC_OK || retValue == VDC_OK_BUT_BIT_ERROR) {

       /* If bit errors */  
       if (corruptedVPs) {
           retValue = VDC_OK_BUT_FRAME_USELESS;
       }

       if ( retValue != VDC_OK_BUT_FRAME_USELESS ) {

           if ( instance->nOfDecodedFrames < 0xffffffff )
              instance->nOfDecodedFrames++;
           if (vdcFillImageBuffers(instance, numOfCodedMBs, 
               instance->currFrame->imb) < 0)
               retValue = VDC_ERR;

       }
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


/* {{-output"vdcIsMPEGINTRA.txt"}} */
/*
 * vdcIsMPEGINTRA
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

int vdcIsMPEGINTRA(
   vdcHInstance_t hInstance,
   void *frameStart,
   unsigned frameLength)
/* {{-output"vdcIsINTRA.txt"}} */
{
   bibBuffer_t *tmpBitBuffer;
   int fINTRA = 0, bitErrorIndication, syncCode, vdxStatus;
   int16 error = 0;
   vdcInstance_t *instance = (vdcInstance_t *) hInstance;
   vdxGovHeader_t govHeader;

   vdcAssert(instance);

   tmpBitBuffer = bibCreate(frameStart, frameLength, &error);
   if (!tmpBitBuffer || error)
      return 0;

   syncCode = sncCheckMpegSync(tmpBitBuffer, instance->pictureParam.fcode_forward, &error);

   if ((syncCode == SNC_GOV || syncCode == SNC_VOP) && error == 0) {
      vdxGetVopHeaderInputParam_t vopIn;
      vdxVopHeader_t vopOut;

      if (syncCode == SNC_GOV) {
         vdxStatus = vdxGetGovHeader(tmpBitBuffer, &govHeader, 
            &bitErrorIndication);

         if (vdxStatus < 0 || bitErrorIndication != 0) 
            return fINTRA;
      }

      /* MVE */
      int dummy1, dummy2, dummy3, dummy4; /* not used for any processing */
      /* Get VOP header */
      vopIn.time_increment_resolution = instance->pictureParam.time_increment_resolution;
      vdxStatus = vdxGetVopHeader(tmpBitBuffer, &vopIn, &vopOut, 
                &dummy1, &dummy2, &dummy3, &dummy4,
                &bitErrorIndication);


      if (vdxStatus >= 0 && bitErrorIndication == 0)
         fINTRA = (vopOut.coding_type == VDX_VOP_TYPE_I);
   }
   
   bibDelete(tmpBitBuffer, &error);

   return fINTRA;
}
// End of File
