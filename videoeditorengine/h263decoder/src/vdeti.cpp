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
* Frame decoding function.
*
*/



/*
 * Includes
 */

#include "h263dconfig.h"
#include "vdeti.h"
#include "debug.h"
#include "sync.h"
#include "vde.h"
#include "vdemain.h"
#include "core.h"
/* MVE */
#include "MPEG4Transcoder.h"

/*
 * Local function prototype
 */

static int vdeSeekNextValidDecodingStartPosition(
   vdeInstance_t *instance, CMPEG4Transcoder *hTranscoder);


/*
 * Functions visible outside this module
 */

/* {{-output"vdeDecodeFrame.txt"}} */
/*
 * vdeDecodeFrame
 *
 * Parameters:
 *    hInstance                  instance data
 *
 * Function:
 *    This function decodes the bitstream by using
 *    vdcDecodeFrame until it gets at least one decoded frame. It also shows 
 *    the resulting frames (by calling renDraw). 
 *    In addition, the function handles the parameter updating synchronization
 *    by calling the VDE Queue module.
 *
 *    This function is intended to be called from the thread main function.
 *    It won't return until it gets a new decoded frame. If the bitstream
 *    is totally corrupted, this feature might cause a considerable delay
 *    in the execution of the thread main function.
 *
 * Returns:
 *    VDE_OK                     if the function was successful
 *    VDE_OK_EOS                 if the end of stream has been reached
 *    VDE_OK_BUT_FRAME_USELESS   if the function behaved normally, but no 
 *                               decoding output was produced due to too
 *                               corrupted input frame
 *    VDE_ERROR                  if a fatal error, from which the decoder
 *                               cannot be restored, has occured
 *    VDE_ERROR_HALTED           the instance is halted, it should be closed
 *
 *    
 */


int vdeDecodeFrame(vdeHInstance_t hInstance, int /*aStartByteIndex*/, int /*aStartBitIndex*/, CMPEG4Transcoder *hTranscoder)
/* {{-output"vdeDecodeFrame.txt"}} */
{
   int
      sncCode,                   /* sync code from which to start decoding */
      numOutputFrames,           /* number of output frames, 2 for PB-frames,
                                    1 otherwise */
      fOutputUseless,            /* 1 if frame(s) too corrupted to display,
                                    0 otherwise */
      i,                         /* loop variable */
      vdcStatus,                 /* return value of vdcDecodeFrame or 
                                    vdcDecodeMPEGVop */
      fFullPictureFreeze = 0;    /* set to 1 if full-picture freeze is
                                    going on as described in section L.4
                                    of the H.263 recommendation */

   vdeInstance_t 
      *instance = (vdeInstance_t *) hInstance;
                                 /* pointer to instance structure */

    /* MVE */
     int StartByteIndex ;
     int StartBitIndex ;

   instance->fPrevFrameDecoded = 0;

   /* If the instance is in the EOS state */
   if (instance->state == VDE_STATE_EOS) {

      /* If MPEG-4 stream, visual_object_sequence_end_code terminates
         a visual session, and therefore we return.
         If H.263 stream, EOS (End of Sequence) has no semantics, and
         therefore we return only if MoVi buffers wrapper indicates so. */
      if (instance->fMPEG4)
         /* Return an indication that the stream has ended */
         return VDE_OK_EOS;
   }

   /* If the instance is in the "Halted" state */
   if (instance->state == VDE_STATE_HALTED) {

      /* If the current frame is INTRA */
      if (vdcIsINTRA(instance->vdcHInstance, instance->inBuffer->baseAddr, 20)) {
         /* Reset the state and try to decode */
         instance->state = VDE_STATE_MIDDLE;
         deb("vdeDecodeFrame: Trying to recover from halted state.\n");
      }

      else {
         deb("vdeDecodeFrame: ERROR - already halted.\n");
         /* Return an indication that the stream cannot be decoded further */
         return VDE_ERROR_HALTED;
      }
   }

   /* If the instance state indicates that we should resynchronize to INTRA
      frame */
   if (instance->state == VDE_STATE_RESYNC) {

      /* If the current frame is INTRA */
      if (vdcIsINTRA(instance->vdcHInstance, instance->inBuffer->baseAddr, 20)) {
         /* Reset the state and continue decoding */
         instance->state = VDE_STATE_MIDDLE;
      }

      else
         return VDE_OK;
   }

   /* Read a synchronization code from the current position of the 
   bitstream or seek the next valid sync code to start decoding */
   sncCode = vdeSeekNextValidDecodingStartPosition(instance, hTranscoder);

   /* If error in seeking sync code */
   if (sncCode < 0)
      /* Return error */
      return sncCode;

   /* Else if sync code not found 
      Note: Can only happen in "one frame per one input bit buffer" case, 
      i.e. not in videophone */
   else if (sncCode == SNC_NO_SYNC)
      /* Note: One could send a INTRA picture request if such a service is
         available. However, it is assumed that the service is not available
         in this non-videophone application, and therefore no update is
         requested. */

      /* Return indication of useless (too corrupted) frame */
      return VDE_OK_BUT_FRAME_USELESS;

   /* Set the instance to the "Middle" state */
   instance->state = VDE_STATE_MIDDLE;

   /* If the synchronization code == EOS */
   if (sncCode == SNC_EOS) {

      /* Set the instance to the EOS state */
      instance->state = VDE_STATE_EOS;

      /* Return an indication that the stream ended */
      return VDE_OK_EOS;
   }
      
   /* MVE */
     StartByteIndex = instance->inBuffer->getIndex;
     StartBitIndex  = instance->inBuffer->bitIndex;




   /* Decode a frame */

   if (instance->fMPEG4) {

      vdcStatus = vdcDecodeMPEGVop(instance->vdcHInstance, instance->inBuffer,
          instance->outBuffer, instance->bufEdit, instance->iColorEffect, instance->iGetDecodedFrame,
          StartByteIndex, StartBitIndex, hTranscoder);
   } 
   else 
   {


       vdcStatus = vdcDecodeFrame(instance->vdcHInstance, instance->inBuffer, 
           instance->outBuffer, instance->bufEdit, instance->iColorEffect,instance->iGetDecodedFrame, hTranscoder);

   }


   if (vdcStatus < 0) {
      deb("vdeDecodeFrame: ERROR - vdcDecodeFrame failed.\n");

      instance->state = VDE_STATE_HALTED;

      if (vdcStatus == VDC_ERR_NO_INTRA)
          return VDE_ERROR_NO_INTRA;
      
      return VDE_ERROR_HALTED;
   }

   /* If EOS occured */
   if (vdcIsEOSReached(instance->vdcHInstance))
      instance->state = VDE_STATE_EOS;

   /* If the decoded frame is totally corrupted */
   if ((vdcStatus == VDC_OK_BUT_FRAME_USELESS) || (vdcStatus == VDC_OK_BUT_NOT_CODED)) {

      /* If continuous input buffer */

      fOutputUseless = 1;
   }

   else {
      fOutputUseless = 0;
      instance->fPrevFrameDecoded = 1;
   }


   /* If H.263 bitstream and decoding (partially) successful */
   if (!instance->fMPEG4 && !fOutputUseless) {
   }

   /*
    * Return decoded images to the image store and 
    * send them to Video Renderer if needed
    */

   numOutputFrames = vdcGetNumberOfOutputFrames(instance->vdcHInstance);

   /* Loop to handle each output frame separately */
   for (i = 0; i < numOutputFrames; i++) {
      vdeImsItem_t *imsItem;

      imsItem = vdcGetImsItem(instance->vdcHInstance, i);

      vdeAssert(imsItem);

      /* If the frame is useless */
      if (fOutputUseless) {

         /* Return the image buffer to the "Free" store */
         if (vdeImsPutFree(&instance->imageStore, imsItem) < 0) {
           deb("vdeDecodeFrame: ERROR - vdeImsPutFree failed.\n");
           instance->state = VDE_STATE_HALTED;
           return VDE_ERROR_HALTED;
         }
      }

      /* Else the frame is ok */
      else {

         vdeImb_t *imb;

         vdeImsStoreItemToImageBuffer(imsItem, &imb);


         /* If full-picture freeze requested */
         if (fFullPictureFreeze) {

            /* Return the image buffer to the image buffer store
               but don't send it to the renderer */
            if (vdeImsPut(&instance->imageStore, imsItem) < 0) {
               deb("vdeDecodeFrame: ERROR - vdeImsPut failed.\n");
               instance->state = VDE_STATE_HALTED;
               return VDE_ERROR_HALTED;
            }
         }

         /* Else normal displaying */
         else {
            h263dFrameType_t frameType;
            int renStatus;
            u_int32 renHandle;

            /* Get renderer for the output frame */
            frameType.scope = H263D_FTYPE_NDEF;
            frameType.width = renDriBitmapWidth(imb->drawItem);
            frameType.height = renDriBitmapHeight(imb->drawItem);
       
            renStatus = vdeFrtGetItem(&instance->renStore, &frameType, &renHandle);

            if (renStatus == VDE_OK_NOT_AVAILABLE)
               renHandle = 0;


            if (renStatus >= 0) {
               /* Return the image buffer to the image buffer store
                  and send it to the renderer (if possible) */
               if (vdeImsPut(&instance->imageStore, imsItem) < 0) {
                  deb("vdeDecodeFrame: ERROR - vdeImsPut failed.\n");
                  instance->state = VDE_STATE_HALTED;
                  return VDE_ERROR_HALTED;
               }
            }

            else {
               deb("vdeDecodeFrame: ERROR - vdeFrtGetItem failed.\n");
               instance->state = VDE_STATE_HALTED;
               return VDE_ERROR_HALTED;
            }
         }

      }
   }
   
   // fatal errors were returned already earlier
   switch ( vdcStatus )
    {
    case VDC_OK_BUT_FRAME_USELESS:
        {
        return VDE_OK_BUT_FRAME_USELESS;
        }
//        break;
    case VDC_OK_BUT_NOT_CODED:
        {
        return VDE_OK_BUT_NOT_CODED;
        }
//        break;
    default:
      {
        break;
//      return VDE_OK;
      }
    }
    
return VDE_OK;
}


/*
 * Local functions
 */

/*
 * vdeSeekNextValidDecodingStartPosition
 *
 * Parameters:
 *    instance                   instance data
 *
 * Function:
 *    This function seeks the next synchronization position from the bitstream
 *    (starting from the current position) from which the decoder can start
 *    decoding.
 *
 * Returns:
 *    VDE_ERROR                  if a fatal error, from which the decoder
 *                               cannot be restored, has occured
 *
 *    SNC_NO_SYNC                if no suitable sync code was found
 *                               (can happen only in non-videophone case
 *                               where only one frame is in one bit input
 *                               buffer)
 *
 *    SNC_PSC                    H.263 Picture Start Code found
 *
 *    SNC_EOS                    H.263 End of Sequence found or
 *                               MPEG-4 EOB found
 *
 *    SNC_GBSC                   H.263 GBSC found
 *
 *    SNC_VOP                    MPEG-4 VOP
 *    SNC_GOV                    MPEG-4 GOV
 *
 *    
 */

static int vdeSeekNextValidDecodingStartPosition(
   vdeInstance_t *instance, CMPEG4Transcoder *hTranscoder)
{
   int
      sncCode = SNC_NO_SYNC;  /* sync code in current position */
   int16 
      error = 0;              /* error code returned from sync module */

   /* Note: For the first INTRA of the stream,
   do the H.263 vs. non-H.263 stream decision in a higher level,
   now assume that there has simply been a bit error in the header,
   and one should try from the next PSC */

   if (instance->fMPEG4) {
      vdcInstance_t* vdcinst = (vdcInstance_t *) instance->vdcHInstance;
      int fcode = vdcinst->pictureParam.fcode_forward;

      while (sncCode != SNC_VOP && 
             sncCode != SNC_GOV && 
             sncCode != SNC_EOB &&
             sncCode != SNC_VIDPACK) {

         sncCode = sncSeekMPEGStartCode(instance->inBuffer, (fcode ? fcode : 1), 1 /* VPs not relevant at this stage */, 0, &error);
         if (error == ERR_BIB_NOT_ENOUGH_DATA)
            return SNC_NO_SYNC;
         else if (error)
            return VDE_ERROR;
         else if (sncCode == SNC_VOS)
         {            
            /* Visual Object Sequence start code found, VOS + VO + VOL headers have to be parsed now.. */
            /* (The bitstream may contain multiple instances of VOS start codes before Visual
                Object Sequence end code) */
            if (vdcDecodeMPEGVolHeader(vdcinst, instance->inBuffer, hTranscoder) != 0)
               return VDE_ERROR;
         }
      }

      if (sncCode == SNC_EOB)
         sncCode = SNC_EOS;
   }
   else 
   {
      while (
         sncCode != SNC_PSC && 
         sncCode != SNC_EOS &&
         sncCode != SNC_GBSC ) {

         sncCode = sncSeekSync(instance->inBuffer, &error);
         if (error == ERR_BIB_NOT_ENOUGH_DATA)
            return SNC_NO_SYNC;
         else if (error)
            return VDE_ERROR;
      }
   }

   return sncCode;
}


// End of file
