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
* Picture header decoding functions.
*
*/



#include "h263dConfig.h"

#include "decpich.h"
#include "debug.h"
#include "vdeims.h"
#include "vdeimb.h"
#include "viddemux.h"
#include "sync.h"


/*
 * Global functions
 */

/* {{-output"dphGetPictureHeader.txt"}} */
/*
 * dphGetPictureHeader
 *    
 *
 * Parameters:
 *    inParam                    input parameters
 *    inOutParam                 input/output parameters, these parameters
 *                               may be modified in the function
 *    outParam                   output parameters
 *    bitErrorIndication         bit error indication, see biterr.h for
 *                               the possible values
 *
 * Function:
 *    This function gets and decodes a picture header which should start
 *    with a PSC at the current position of the bit buffer. At first,
 *    the function reads the picture header fields from the bitstream.
 *    Then, it checks whether the values of the fields are supported.
 *    An unsupported value is interpreted as a bit error. Finally,
 *    the function updates VDC instance data and output parameters
 *    (according to the new picture header).
 *
 * Returns:
 *    DPH_OK
 *    DPH_OK_BUT_BIT_ERROR
 *    DPH_ERR
 *    DPH_ERR_NO_INTRA
 *    DPH_ERR_FORMAT_CHANGE
 *
 */

int dphGetPictureHeader(
   const dphInParam_t *inParam,
   dphInOutParam_t *inOutParam,
   dphOutParam_t *outParam,
   int *bitErrorIndication)
/* {{-output"dphGetPictureHeader.txt"}} */
{
   bibBuffer_t *inBuffer = inOutParam->inBuffer;
   u_char fPrevIntraGot;         /* Non-zero if an INTRA frame has been decoded
                           before the current frame */
   vdxGetPictureHeaderInputParam_t picHdrIn;
   vdxPictureHeader_t picHeader;
   vdcInstance_t *instance = inOutParam->vdcInstance;
   vdcPictureParam_t *pictureParam = &instance->pictureParam;
   vdcPictureParam_t prevPictParam;
   int trd;                      /* Difference between present and previous tr */
   int fInterFrame;              /* non-zero = INTER frame, 0 = INTRA frame */
   int fPBFrame;                 /* Flag indicating a PB-frame */
   int fRPS;                     /* 1 = RPS used, 0 = RPS not used */
   int ret = 0;                  /* temporary variable to carry return values */
   int retVal = DPH_OK;          /* return value if everything as expected,
                                    either DPH_OK or DPH_OK_BUT_SKIP */
   int16 error = 0;              /* snc and mvc module error code */


   /* 
    * Get picture header 
    */

   picHdrIn.numStuffBits = inParam->numStuffBits;
   picHdrIn.fCustomPCF = 0;
   picHdrIn.fScalabilityMode = 0;
   picHdrIn.fRPS = instance->fRPS;
   picHdrIn.flushBits = bibFlushBits;
   picHdrIn.getBits = bibGetBits;
   picHdrIn.showBits = bibShowBits;
   
    /* Used to recover the 1st picture segment */
    u_int32 picHeaderStartPos = bibNumberOfFlushedBits(inBuffer);

    if ( inParam->numStuffBits ) {
     bibFlushBits(inParam->numStuffBits, inBuffer, &picHdrIn.numStuffBits, bitErrorIndication, &error);
     picHdrIn.numStuffBits = 0;
    }

    // Read header bits normally from the bitstream
    ret = vdxGetPictureHeader(inBuffer, &picHdrIn, &picHeader, 
            bitErrorIndication);      

    if ( ret < 0 )
        return DPH_ERR;
    else if ( ret == VDX_OK_BUT_BIT_ERROR ) 
    {
        // Bit error exists.
        return DPH_OK_BUT_BIT_ERROR;
    } // end of if ( ret == VDX_OK_BUT_BIT_ERROR ) 


   /* 
    * Check picture header validity 
    */

   /* Local parameters needed in validity check (and possibly afterwards) */
   fPrevIntraGot = instance->fIntraGot;
   fInterFrame = (picHeader.pictureType == VDX_PIC_TYPE_P ||
      picHeader.pictureType == VDX_PIC_TYPE_PB ||
      picHeader.pictureType == VDX_PIC_TYPE_IPB);
   fPBFrame = ((picHeader.pictureType == VDX_PIC_TYPE_PB)||
      (picHeader.pictureType == VDX_PIC_TYPE_IPB));

   /* TR with the same value as in the previous picture can be interpreted
      two ways:
         - if Reference Picture Selection is enabled, it means
           a synchronization frame
         - if Reference Picture Selection is not used, it means the maximum
           difference between two frames.
           However, at least VDOPhone uses TR equal to zero always, which
           in practice should be interpreted that TR field is not used in
           display timing. */

   /* At first, check if RPS is used */
   if (picHeader.fPLUSPTYPE && picHeader.ufep)
      fRPS = picHeader.fRPS;
   else
      fRPS = instance->fRPS;

   /* Calculate temporal reference difference (from the previous frame) */
   trd = picHeader.tr - instance->pictureParam.tr;
   if (trd < 0)
      trd += 256; /* Note: Add ETR support */

   /* If TR is the same as in the previous frame and Reference Picture Selection
      is not used and not the very first frame (in which case previous tr == 0), 
      the temporal reference difference is the maximum one. */
   if (trd == 0 && !fRPS && instance->frameNum > 0)
      trd = 256; /* Note: Add ETR support */

   /* If unsupported picture type */
   if (picHeader.pictureType != VDX_PIC_TYPE_I
      && picHeader.pictureType != VDX_PIC_TYPE_P
      && picHeader.pictureType != VDX_PIC_TYPE_PB
      && picHeader.pictureType != VDX_PIC_TYPE_IPB
      ) {
      return DPH_OK_BUT_BIT_ERROR;
   }

   if (picHeader.pictureType == VDX_PIC_TYPE_PB ||
      picHeader.pictureType == VDX_PIC_TYPE_IPB) {
      return DPH_OK_BUT_BIT_ERROR;
   }
   
   if (picHeader.cpm) {
      /* No CPM support */
      return DPH_OK_BUT_BIT_ERROR;
   }

   if (picHeader.fPLUSPTYPE) {
      /* If unsupported modes (and UFEP is whatever) */
      if (picHeader.fRPR ||
         picHeader.fRRU) {
         return DPH_OK_BUT_BIT_ERROR;
      }

      if (picHeader.ufep == 1) {
         /* If unsupported mode */
         if (picHeader.fISD ||
            picHeader.fSAC ||
            picHeader.fAIV ||
            picHeader.fCustomPCF) {
            return DPH_OK_BUT_BIT_ERROR;
         }

         if (picHeader.fUMV) {
            return DPH_OK_BUT_BIT_ERROR;
         }

         if (picHeader.fAP) {
            return DPH_OK_BUT_BIT_ERROR;
         }
         
         if (picHeader.fAIC) {
            return DPH_OK_BUT_BIT_ERROR;
         }
         
         
         
         if (picHeader.fDF) {
            return DPH_OK_BUT_BIT_ERROR;
         }
         
         
         
         if (picHeader.fSS) {
            return DPH_OK_BUT_BIT_ERROR;
         }
         
         
         
         if (picHeader.fMQ) {
            return DPH_OK_BUT_BIT_ERROR;
         }
         
      }

      /* Note: Add check for "The Indenpenent Segment Decoding mode shall not
         be used with the Reference Picture Resampling mode"
         from section 5.1.4.6 of the H.263 recommendation whenever both of
         the modes in question are supported. 
         Note that this check cannot reside in vdxGetPictureHeader since
         ISD and RPR can be turned on in different pictures. */
   }

   /* Else no PLUSPTYPE */
   else {
      /* If unsupported modes */
      if (picHeader.fSAC) {
         return DPH_OK_BUT_BIT_ERROR;
      }

         if (picHeader.fUMV) {
            return DPH_OK_BUT_BIT_ERROR;
         }

         if (picHeader.fAP) {
            return DPH_OK_BUT_BIT_ERROR;
         }
   }

   /* If no INTRA frame has ever been decoded and the current frame is INTER */
   if (!fPrevIntraGot && fInterFrame)
      return DPH_ERR_NO_INTRA;

   if (fPrevIntraGot) {

      /* If width and height are indicated in the header and
         they are different from the previous */
      if ((picHeader.fPLUSPTYPE == 0 || picHeader.ufep == 1) &&
         (instance->pictureParam.lumWidth != picHeader.lumWidth ||
         instance->pictureParam.lumHeight != picHeader.lumHeight)) {

         if (fInterFrame) {
            /* INTER frame, check that the source format has not changed */
            deb("vdcDecodeFrame: ERROR - Source format has changed "
               "(INTER frame)\n");
            return DPH_OK_BUT_BIT_ERROR;
         }
         else if ( *bitErrorIndication ) {
            deb("vdcDecodeFrame: ERROR - Frame size changing during "
               "the sequence, cause: bitError\n");
            return DPH_OK_BUT_BIT_ERROR;
         }
         else {
            /* At the moment do NOT allow source format changes during
               the sequence. */
            deb("vdcDecodeFrame: ERROR - Frame size changing during "
               "the sequence\n");
            return DPH_ERR_FORMAT_CHANGE;
         }
      }
   }

   /* If the temporal reference of B points after P */
   if (fPBFrame && trd <= picHeader.trb) {
      deb("vdcDecodeFrame: ERROR - TRB illegal.\n");
      return DPH_OK_BUT_BIT_ERROR;
   }


   /* Now, the picture header is considered to be error-free,
      and its contents are used to manipulate instance data */

   /*
    * Update instance data and output parameters
    */

   /* Update frame numbers */
   {
      int32 oldFrameNum = instance->frameNum;

      instance->frameNum += trd;
      if (instance->frameNum < 0)
         /* Wrap from 0x7fffffff to 0 instead of the largest negative number */
         instance->frameNum += 0x80000000;

      if (fPBFrame) {
         instance->frameNumForBFrame = oldFrameNum + picHeader.trb;
         if (instance->frameNumForBFrame < 0)
            /* Wrap from 0x7fffffff to 0 instead of the largest negative number */
            instance->frameNumForBFrame += 0x80000000;
      }
      else
         instance->frameNumForBFrame = -1;
   }

   /* Note: If no INTRA has been got the function has already returned */
   instance->fIntraGot = 1;

   outParam->pquant = picHeader.pquant;

   instance->fRPS = fRPS;

   if (instance->fRPS && picHeader.trpi)
      outParam->trp = picHeader.trp;
   else
      outParam->trp = -1;

   if (instance->fRPS && picHeader.fPLUSPTYPE && picHeader.ufep)
      instance->rpsMode = picHeader.rpsMode;
   
   /* Store previous picture parameters temporarily */
   memcpy(&prevPictParam, pictureParam, sizeof(vdcPictureParam_t));

   /* Update picture parameters */
   pictureParam->tr = picHeader.tr;
   pictureParam->trd = trd;
   pictureParam->fSplitScreenIndicator = picHeader.fSplitScreen;
   pictureParam->fDocumentCameraIndicator = picHeader.fDocumentCamera;
   pictureParam->fFullPictureFreezeRelease = picHeader.fFreezePictureRelease;
   pictureParam->pictureType = picHeader.pictureType;
   pictureParam->fPLUSPTYPE = picHeader.fPLUSPTYPE;
   pictureParam->cpm = picHeader.cpm;
   pictureParam->psbi = picHeader.psbi;

   if (fPBFrame) {
      pictureParam->trb = picHeader.trb;
      pictureParam->dbquant = picHeader.dbquant;
   }

   if (!picHeader.fPLUSPTYPE || picHeader.ufep) {
      pictureParam->lumWidth = picHeader.lumWidth;
      pictureParam->lumHeight = picHeader.lumHeight;
      pictureParam->fUMV = picHeader.fUMV;
      pictureParam->fSAC = picHeader.fSAC;
      pictureParam->fAP = picHeader.fAP;

      if (pictureParam->lumWidth % 16)
         pictureParam->lumMemWidth = 
            (pictureParam->lumWidth / 16 + 1) * 16;
      else
         pictureParam->lumMemWidth = pictureParam->lumWidth;

      if (pictureParam->lumHeight % 16)
         pictureParam->lumMemHeight = (pictureParam->lumHeight / 16 + 1) * 16;
      else
         pictureParam->lumMemHeight = pictureParam->lumHeight;

      if (picHeader.lumHeight <= 400)
         pictureParam->numMBLinesInGOB = 1;
      else if (picHeader.lumHeight <= 800)
         pictureParam->numMBLinesInGOB = 2;
      else
         pictureParam->numMBLinesInGOB = 4;

      pictureParam->numMBsInMBLine = pictureParam->lumMemWidth / 16;

      {
         int 
            numMBLines = pictureParam->lumMemHeight / 16,
            numMBLinesInLastGOB = 
               numMBLines % pictureParam->numMBLinesInGOB;

         if (numMBLinesInLastGOB) {
            pictureParam->numGOBs = 
               numMBLines / pictureParam->numMBLinesInGOB + 1;
            pictureParam->fLastGOBSizeDifferent = 1;
            pictureParam->numMBsInLastGOB = 
               numMBLinesInLastGOB * pictureParam->numMBsInMBLine;
         }
         else {
            pictureParam->numGOBs = 
               numMBLines / pictureParam->numMBLinesInGOB ;
            pictureParam->fLastGOBSizeDifferent = 0;
            pictureParam->numMBsInLastGOB = 
               pictureParam->numMBLinesInGOB * pictureParam->numMBsInMBLine;
         }
      }

      pictureParam->numMBsInGOB = 
         pictureParam->numMBsInMBLine * pictureParam->numMBLinesInGOB;
   }

   else {
      pictureParam->lumWidth = prevPictParam.lumWidth;
      pictureParam->lumHeight = prevPictParam.lumHeight;
      pictureParam->lumMemWidth = prevPictParam.lumMemWidth;
      pictureParam->lumMemHeight = prevPictParam.lumMemHeight;
      pictureParam->fUMV = prevPictParam.fUMV;
      pictureParam->fSAC = prevPictParam.fSAC;
      pictureParam->fAP = prevPictParam.fAP;
   }

   /* Note: Mode inference rules defined in section 5.1.4.5 of
      the H.263 recommendation are (in most cases) not reflected to members
      of pictureParam due to the fact that they are implicitly turned
      off or irrelevant in the current implementation. (This is particularly
      true when assigning inferred state "off".) */

   if (picHeader.fPLUSPTYPE) {
      pictureParam->fRPR = picHeader.fRPR;
      pictureParam->fRRU = picHeader.fRRU;
      pictureParam->rtype = picHeader.rtype;

      /* Note: irrelevant if Annex O is not in use */
      pictureParam->elnum = picHeader.elnum;

      if (picHeader.ufep) {
         pictureParam->fCustomSourceFormat = picHeader.fCustomSourceFormat;
         pictureParam->fAIC = picHeader.fAIC;
         pictureParam->fDF = picHeader.fDF;
         pictureParam->fSS = picHeader.fSS;
         pictureParam->fRPS = picHeader.fRPS;
         pictureParam->fISD = picHeader.fISD;
         pictureParam->fAIV = picHeader.fAIV;
         pictureParam->fMQ = picHeader.fMQ;
      
         pictureParam->fUMVLimited = picHeader.fUMVLimited;

      }

      /* Else no UFEP */
      else {
         pictureParam->fCustomSourceFormat = 
            prevPictParam.fCustomSourceFormat;
         pictureParam->fAIC = prevPictParam.fAIC;
         pictureParam->fDF = prevPictParam.fDF;
         pictureParam->fSS = prevPictParam.fSS;
         pictureParam->fRPS = prevPictParam.fRPS;
         pictureParam->fISD = prevPictParam.fISD;
         pictureParam->fAIV = prevPictParam.fAIV;
         pictureParam->fMQ = prevPictParam.fMQ;
      
         pictureParam->fUMVLimited = prevPictParam.fUMVLimited;

      }
   }

   /* Else no PLUSPTYPE */
   else {
      /* Section 5.1.4.5 of the H.263 recommendation states:
         "If a picture is sent which does not contain (PLUSPTYPE),
         a state "off" shall be assigned to all modes not explicitly set to
         "on" in the PTYPE filed, and all modes shall continue to have 
         an inferred state of "off" until a new picture containing
         the optional part of PLUSPTYPE is sent." */

      /* Fields occuring in mandatory part of PLUSPTYPE are copied from
         the previous picture. */
      pictureParam->fRPR = prevPictParam.fRPR;
      pictureParam->fRRU = prevPictParam.fRRU;

      /* RTYPE is set to zero according to section 6.1.2 of 
         the H.263 recommendation */
      pictureParam->rtype = 0;

      /* Modes occuring in the optional part of PLUSPTYPE are turned off
         (unless occuring in PTYPE field too, like Advanced Prediction). */
      pictureParam->fCustomSourceFormat = 0;
      pictureParam->fCustomPCF = 0;
      pictureParam->fAIC = 0;
      pictureParam->fDF = 0;
      pictureParam->fSS = 0;
      pictureParam->fRPS = 0;
      pictureParam->fISD = 0;
      pictureParam->fAIV = 0;
      pictureParam->fMQ = 0;
   
      /* Other related settings turned off just to be sure that they are
         not misused. */
      pictureParam->fUMVLimited = 0;
   }

   pictureParam->fMVsOverPictureBoundaries =
      (pictureParam->fUMV || pictureParam->fAP || 
      pictureParam->fDF);

   /* Initialize motion vector count module */
    mvcStart(   &instance->mvcData, 
               instance->pictureParam.lumMemWidth / 16 - 1, 
               pictureParam->lumMemWidth, 
               pictureParam->lumMemHeight, &error);
   /* Note: This assumes that the memory for frame->mvcData is filled with 0
      in the first time the function is called.
      (This is ensured by setting the instance data to zero when it is
      initialized in vdcOpen.) */
   instance->mvcData.fSS = pictureParam->fSS;

   if (error) {
      deb("mvcStart failed.\n");
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

   /* Initialize the parameters for the advanced intra coding structure. */
   if (pictureParam->fAIC)
    {
    // not supported
    }

   /* Initialize the parameters for Slice Structured Mode. */
   if (pictureParam->fSS)
    {
    // not supported
    }
    

   /* If picture header successfully decoded or reliably recovered */
   if ( inParam->fReadBits && ret == VDX_OK ) {

      /* In the beginning, tr is set to -1 */
      if ( instance->fPrevPicHeaderReliable
         && instance->prevPicHeader->tr >= 0
         && ( picHeader.pictureType != instance->prevPicHeader->pictureType 
            || picHeader.fUMV != instance->prevPicHeader->fUMV 
            || picHeader.fAP != instance->prevPicHeader->fAP
            || picHeader.fPLUSPTYPE != instance->prevPicHeader->fPLUSPTYPE
            || ( ( picHeader.fPLUSPTYPE == 0 || picHeader.ufep == 1 ) 
               && ( picHeader.lumWidth != instance->prevPicHeader->lumWidth 
                    || picHeader.lumHeight != instance->prevPicHeader->lumHeight )
                  )
            || ( picHeader.fPLUSPTYPE == 1 && picHeader.ufep == 1
               && ( picHeader.fAIC != instance->prevPicHeader->fAIC
                  || picHeader.fDF != instance->prevPicHeader->fDF
                  || picHeader.fMQ != instance->prevPicHeader->fMQ
                  || picHeader.fUMVLimited != instance->prevPicHeader->fUMVLimited
                     )
                  )
               )
            || ( picHeader.fPLUSPTYPE == 1 
               && picHeader.rtype != instance->prevPicHeader->rtype )
            ) {

         /* Some parameter in PTYPE/PLUSPTYPE has changed, GFID should change also */
         /* Note that some parameters, such as SAC, are not checked since they have not been implemented */

         instance->fGFIDShouldChange = 1;
      }
      else if ( !instance->fPrevPicHeaderReliable ) {
         /* Previous picture header was not reliable, don't check GFID */
         instance->fGFIDShouldChange = 1;
         instance->gfid = -1;
      }
      else if ( instance->gfid == -1 ) {
         /* The very first frame, or a frame after a lost frame, GFID can be whatever */
         /* Note: instance->gfid == -1 is used to indicate that previous GFID
            is not available as in the first picture in first GOB header. 
            If there is a picture without GOB headers and having a different
            picture header than the surrounding pictures, instance->gfid is
            reset to -1 in vdcDecodeFrame()! (Otherwise, a wrong GFID may be
            used in GFID correctness check.) */
         instance->fGFIDShouldChange = 1;
      }
      else {
         /* No changes in PTYPE => the same GFID.
          * It is better not to change the value of this flag here, since
          * if there are no GOB or slice headers where GFID can change, the next
          * read GFID in some other frame than the one with the different PTYPE 
          * will be considered illegal 
          */
         /* instance->fGFIDShouldChange = 0; */
      }

      /* Header was reliable and can be used when checking the next header */
      instance->fPrevPicHeaderReliable = 1;

      /* Copy the read header to be used if the next header is corrupted or lost */
      memcpy( instance->prevPicHeader, &picHeader, sizeof( vdxPictureHeader_t ) );
   }

   /* Else picture header unreliable */
   else {
      /* Since we cannot trust that picture header is reliable, 
         we cannot detect errors based on GFID.
         Thus, let's disable GFID checking in GOB level. */
      instance->fPrevPicHeaderReliable = 0;
      instance->fGFIDShouldChange = 1;
      instance->gfid = -1;
   }

   return retVal;

   /* Error condition handling,
      release everything in reverse order */
//   errAfterGetFreeB:

//   vdeImsPutFree(instance->imageStore, instance->bFrame);
//   errGetFreeB:

   errAfterGetFreeP:

   vdeImsPutFree(instance->imageStore, instance->currFrame);
   errGetFreeP:

   instance->currFrame = NULL;
   instance->bFrame = NULL;

   return DPH_ERR;
}


/* {{-output"dphGetSEI.txt"}} */
/*
 * dphGetSEI
 *    
 *
 * Parameters:
 *    vdcInstance                Video Decoder Core instance
 *    inBuffer                   Bit Buffer instance
 *    bitErrorIndication         bit error indication, see biterr.h for
 *                               the possible values
 *
 * Function:
 *    This function gets and decodes Supplemental Enhancement
 *    Information.
 *
 * Returns:
 *    >= 0                       the function was successful
 *    < 0                        an error occured
 *
 */

int dphGetSEI(
   vdcInstance_t *vdcInstance,   /* Video Decoder Core instance */
   bibBuffer_t *inBuffer,        /* Bit Buffer instance */
   int *bitErrorIndication)
{
/* {{-output"dphGetSEI.txt"}} */
   int
      vdxStatus;

   vdxSEI_t
      sei;

   vdxStatus = vdxGetAndParseSEI(
      inBuffer,
      vdcInstance->pictureParam.fPLUSPTYPE,
      vdcInstance->numAnnexNScalabilityLayers,
      &sei,
      bitErrorIndication);

   if (vdxStatus < 0)
      return DPH_ERR;
   else if (vdxStatus == VDX_OK_BUT_BIT_ERROR)
      return DPH_OK_BUT_BIT_ERROR;

      
   return DPH_OK;
}
// End of File
