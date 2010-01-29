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
* Picture header decoding functions (MPEG-4).
*
*/



#include "h263dConfig.h"
#include "mpegcons.h"
#include "sync.h"
#include "decpich.h"
#include "debug.h"
#include "vdeims.h"
#include "vdeimb.h"
#include "viddemux.h"


/*
 * Global functions
 */

/*
 * dphGetMPEGVolHeader
 *    
 *
 * Parameters:
 *    hInstance                  instance data
 *
 * Function:
 *   This function decodes the Video Object Header and the VOL Header
 *   and initializes the fields of vdcInstance which are set in these
 *   headers. Check and reports (generating error messages) if the header
 *   shows a configuration on feature which is not supported by the
 *   Simple Video Object Profile of MPEG4
 *
 * Returns:
 *    VDE_OK                     if the function was successful
 *    VDE_ERROR                  if a fatal error, from which the decoder
 *                               cannot be restored, has occured
 *
 */

int dphGetMPEGVolHeader(dphInOutParam_t *inOutParam, CMPEG4Transcoder *hTranscoder)
{
   vdcInstance_t *instance = inOutParam->vdcInstance;
   bibBuffer_t *inBuffer = inOutParam->inBuffer;
   int bitErrorIndication = 0;
   vdxVolHeader_t volHeader;   

   /* read the Stream headers from the bitstream */
   if ((vdxGetVolHeader(inBuffer, &volHeader, &bitErrorIndication, 0, 0, 0, hTranscoder) != VDX_OK) ||
       (bitErrorIndication != 0)) {
      return DPH_ERR;
   }   

   /* copy the useful VOL parameters to pictureParam or the core data structure */
   instance->pictureParam.vo_id = volHeader.vo_id;
   instance->pictureParam.vol_id = volHeader.vol_id;
   instance->pictureParam.time_increment_resolution = volHeader.time_increment_resolution;

     /* MVE */
   instance->pictureParam.fixed_vop_rate = volHeader.fixed_vop_rate;

   instance->pictureParam.lumWidth = volHeader.lumWidth;
   instance->pictureParam.lumHeight = volHeader.lumHeight;

   if (volHeader.lumWidth%16)
       instance->pictureParam.lumMemWidth = (((volHeader.lumWidth >>4/*/16*/) + 1) <<4/** 16*/);
   else
       instance->pictureParam.lumMemWidth = volHeader.lumWidth;
   
   if (volHeader.lumHeight%16)
       instance->pictureParam.lumMemHeight = (((volHeader.lumHeight >>4/*/16*/) + 1) <<4/** 16*/);
   else
       instance->pictureParam.lumMemHeight = volHeader.lumHeight;
   
   instance->pictureParam.error_res_disable = volHeader.error_res_disable;
   instance->pictureParam.data_partitioned = volHeader.data_partitioned;
   instance->pictureParam.reversible_vlc = volHeader.reversible_vlc;

   /* copy the got user data to the core data structure */
   if (volHeader.user_data != NULL) {
       instance->user_data = (char *) malloc(MAX_USER_DATA_LENGTH);
   
       volHeader.user_data_length = 
           ((instance->user_data_length + volHeader.user_data_length) >= MAX_USER_DATA_LENGTH) ?
           (MAX_USER_DATA_LENGTH - instance->user_data_length) : volHeader.user_data_length;
       
       memcpy(
           instance->user_data + instance->user_data_length, 
           volHeader.user_data, 
           volHeader.user_data_length);

       instance->user_data_length += volHeader.user_data_length;

       free(volHeader.user_data);
   }

   return DPH_OK;
}

/* {{-output"dphGetMPEGVopHeader.txt"}} */
/*
 * dphGetMPEGVopHeader
 *    
 *
 * Parameters:
 *    inOutParam                 input/output parameters, these parameters
 *                               may be modified in the function
 *    outParam                   output parameters
 *    bitErrorIndication         bit error indication, see biterr.h for
 *                               the possible values
 *
 * Function:
 *    This function gets and decodes an MPEG Vop header which should start
 *    with a VOP start_code at the current position of the bit buffer.
 *    At first, the function reads the picture header fields from the bitstream.
 *    Then, it checks whether the values of the fields are supported.
 *    An unsupported value is interpreted as a bit error. Finally,
 *    the function updates VDC instance data and output parameters
 *    (according to the new picture header).
 *
 * Returns:
 *    >= 0                       the function was successful
 *    < 0                        an error occured
 *
 */

int dphGetMPEGVopHeader(
   const dphInParam_t *inParam,
   dphInOutParam_t *inOutParam,
   dphOutParam_t *outParam,
   int *bitErrorIndication)
/* {{-output"dphGetMPEGVopHeader.txt"}} */
{
   bibBuffer_t *inBuffer = inOutParam->inBuffer;
   u_char fPrevIntraGot;         /* Non-zero if an INTRA frame has been decoded
                                    before the current frame */
   vdxGetVopHeaderInputParam_t picHdrIn;
   vdxVopHeader_t picHeader;
   vdcInstance_t *instance = inOutParam->vdcInstance;
   vdcPictureParam_t *pictureParam = &instance->pictureParam;
   int fInterFrame;              /* non-zero = INTER frame, 0 = INTRA frame */
   int ret = DPH_OK, curr_mod_time_base=0;
   int16 error = 0;
   int32 currFrameNum=0;

   /* In normal case read the picture header from the bitstream. 
      This function can be also called just to initialize the default 
      picture header parameters and allocate image buffers, but not read
      the picture header from bitstream. This is used when the picture
      header has been lost (e.g. packet loss) but decoding is continued */
      
   if (inParam->fReadBits) {

   /* 
    * Get picture header 
    */

   picHdrIn.time_increment_resolution = pictureParam->time_increment_resolution;

     /* MVE */
   int dummy1, dummy2, dummy3, dummy4; /* not used for processing */
   ret = vdxGetVopHeader(inBuffer, &picHdrIn, &picHeader, 
         &dummy1, &dummy2, &dummy3, &dummy4,
         bitErrorIndication);

   if ( ret < 0 )
      return DPH_ERR;
   else if ( ret > 0 ) {
     ret = DPH_OK_BUT_BIT_ERROR;
     goto updateInstanceData;
   } else {
     ret = DPH_OK;
   }

   /* 
    * Check picture header validity 
    */

   /* Local parameters needed in validity check (and possibly afterwards) */
   fPrevIntraGot = instance->fIntraGot;
   fInterFrame = (picHeader.coding_type == VDX_VOP_TYPE_P);


   if (picHeader.time_base_incr < 0 || picHeader.time_base_incr > 60) {
      if ( *bitErrorIndication ) {
         ret = DPH_OK_BUT_BIT_ERROR;
         goto updateInstanceData;
      }
   }

   curr_mod_time_base = pictureParam->mod_time_base + picHeader.time_base_incr;

   currFrameNum = (int) ((curr_mod_time_base + 
      ((double) picHeader.time_inc) / ((double) pictureParam->time_increment_resolution)) *
      30.0 + 0.001);

   if ((currFrameNum <= instance->frameNum) && 
      (instance->frameNum > 0)) {
      if (*bitErrorIndication) {
         ret = DPH_OK_BUT_BIT_ERROR;
         goto updateInstanceData;
      } else {
         /* this can happen if a previous picture header is lost and it had
            mod_time_base increase information. */
         int i;
         int32 tryFrameNum=0;
         for (i=1;i<5;i++) {
            tryFrameNum = (int) (((curr_mod_time_base+i) + 
               ((double) picHeader.time_inc) / ((double) pictureParam->time_increment_resolution)) *
               30.0 + 0.001);
            if (tryFrameNum > instance->frameNum) {
               currFrameNum = tryFrameNum;
               curr_mod_time_base += i;
               ret = DPH_OK;
               break;
            }
         }
         if (i==5) {
            ret = DPH_OK_BUT_BIT_ERROR;
            goto updateInstanceData;
         }
      }
   }

   /* If no INTRA frame has ever been decoded and the current frame is INTER */
   if (!fPrevIntraGot && fInterFrame) {
      deb("ERROR. No I-vop before a P-vop\n");
      return DPH_ERR_NO_INTRA;

   }

   /* fcode can have only the valid values 1..7 */
   if(picHeader.fcode_forward == 0) {
      ret = DPH_OK_BUT_BIT_ERROR;
      goto updateInstanceData;
   }

   /* quant can not be zero */
   if(picHeader.quant == 0) {
      ret = DPH_OK_BUT_BIT_ERROR;
      goto updateInstanceData;
   }

   } /* end fReadBits */

   /* Now, the picture header is considered to be error-free,
      and its contents are used to manipulate instance data */

updateInstanceData:

   /*
    * Update instance data and output parameters
    */

   /* some parameters need to be set if error has been detected, for initializing */
   if (!inParam->fReadBits || (ret == DPH_OK_BUT_BIT_ERROR)) {
      picHeader.rounding_type = 0;
      picHeader.fcode_forward = 1;
      picHeader.coding_type = 1;
   } else {    
      /* Update frame numbers */
      pictureParam->time_base_incr = picHeader.time_base_incr;
      pictureParam->mod_time_base = curr_mod_time_base;
      pictureParam->time_inc = picHeader.time_inc;
      instance->frameNum = currFrameNum;
   }

   /* Update picture parameters */
   pictureParam->trd = (instance->frameNum % 256) - pictureParam->tr;
   pictureParam->tr = instance->frameNum % 256;

   pictureParam->pictureType = picHeader.coding_type;

   if (!picHeader.vop_coded) {
     return DPH_OK_BUT_NOT_CODED;
   }

   /* NO reference picture selection */
   outParam->trp = -1;

   /* No PB Frames allowed */
   instance->frameNumForBFrame = -1;

   /* Note: If no INTRA has been got the function has already returned */
   instance->fIntraGot = 1;

   pictureParam->intra_dc_vlc_thr = picHeader.intra_dc_vlc_thr;
   pictureParam->fcode_forward = picHeader.fcode_forward;
   
   outParam->pquant = picHeader.quant;

   /* Initialize for only 1 GOB */
   pictureParam->numMBLinesInGOB = pictureParam->lumMemHeight / 16;
   pictureParam->numMBsInMBLine = pictureParam->lumMemWidth / 16;

   pictureParam->numGOBs = 1;
   pictureParam->fLastGOBSizeDifferent = 0;
   pictureParam->numMBsInGOB = 
      pictureParam->numMBsInMBLine * pictureParam->numMBLinesInGOB;
   pictureParam->numMBsInLastGOB = pictureParam->numMBsInGOB;

   /* Rounding type */
   pictureParam->rtype = picHeader.rounding_type;

   /* NO channel Multiplexing */
   pictureParam->fSplitScreenIndicator = 0;
   pictureParam->fDocumentCameraIndicator = 0;
   pictureParam->fFullPictureFreezeRelease = 0;
   pictureParam->fPLUSPTYPE = 0;
   pictureParam->cpm = 0;

   /* All the Annexes are switched off */
   pictureParam->fSAC = 0;
   pictureParam->fRPR = 0;
   pictureParam->fRRU = 0;
   pictureParam->fCustomSourceFormat = 0;
   pictureParam->fAIC = 0;
   pictureParam->fDF = 0;
   pictureParam->fSS = 0;
   pictureParam->fRPS = 0;
   pictureParam->fISD = 0;
   pictureParam->fAIV = 0;
   pictureParam->fMQ = 0;
   pictureParam->fUMVLimited = 0;

   /* Unrestricted MV allowed
      OBMC NOT allowed */
   pictureParam->fUMV = 1;
   pictureParam->fAP = 0;
   pictureParam->fMVsOverPictureBoundaries = 1;

   /* Initialize motion vector count module */
   mvcStart(   &instance->mvcData, 
               ((instance->pictureParam.lumMemWidth >>4 /*/ 16*/) - 1), 
               pictureParam->lumWidth, 
               pictureParam->lumHeight, &error);
   /* Note: This assumes that the memory for frame->mvcData is filled with 0
      in the first time the function is called.
      (This is ensured by setting the instance data to zero when it is
      initialized in vdcOpen.) */
   if (error) {
      deb("mvcStart failed.\n");
      return DPH_ERR;
   }

   /* Initialize once to count parameters for the mvc module */
   {
   int r_size, scale_factor;
 
   instance->mvcData.f_code = pictureParam->fcode_forward;
   r_size = pictureParam->fcode_forward - 1;
   scale_factor = (1 << r_size);
   instance->mvcData.range = 160 * scale_factor;
   }

   /* Start, initialize the Advanced Intra Coding module */
   aicStart(&instance->aicData, pictureParam->numMBsInMBLine, &error);
   if (error) {
      deb("aicStart failed.\n");
      return DPH_ERR;
   }

   /* Get image buffers */
   {
      vdeIms_t *store = instance->imageStore;
      vdeImsItem_t *imsItem;
      vdeImb_t *imb;
      int width, height;

      vdeImsSetYUVNeed(store, inParam->fNeedDecodedFrame);

      if (vdeImsGetFree(store, instance->pictureParam.lumMemWidth, 
         instance->pictureParam.lumMemHeight, 
         &instance->currFrame) < 0)
         goto errGetFreeP;
      imsItem = instance->currFrame;

      if (vdeImsStoreItemToImageBuffer(imsItem, &imb) < 0)
         goto errAfterGetFreeP;

      if (vdeImbYUV(imb, &outParam->currYFrame, 
         &outParam->currUFrame, &outParam->currVFrame,
         &width, &height) < 0)
         goto errAfterGetFreeP;
   }

   return ret;

   /* Error condition handling,
      release everything in reverse order */
   errAfterGetFreeP:

   vdeImsPutFree(instance->imageStore, instance->currFrame);
   errGetFreeP:

   instance->currFrame = NULL;
   instance->bFrame = NULL;

   return DPH_ERR;
}
// End of File
