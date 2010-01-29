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
* A header file for internal usage of the services provided by 
* the Video Decoder Engine.
*
*/


#ifndef _VDE_H_
#define _VDE_H_


/*
 * Includes
 */

#include "vdefrt.h"
#include "vdeims.h"

#include "biblin.h"
#include "vdc263.h"
#include "h263dapi.h"


/*
 * Defines
 */

/* Aliased return values from h263dext.h.
   (These values may also be exported outside the decoder.) */

#define VDE_OK_BUT_NOT_CODED \
                           H263D_OK_BUT_NOT_CODED
#define VDE_OK_BUT_FRAME_USELESS \
                           H263D_OK_BUT_FRAME_USELESS
#define VDE_OK_EOS         H263D_OK_EOS
#define VDE_OK             H263D_OK
#define VDE_ERROR          H263D_ERROR
#define VDE_ERROR_HALTED   H263D_ERROR_HALTED
#define VDE_ERROR_NO_INTRA H263D_ERROR_NO_INTRA

/* Internal return values.
   See h263dext.h that they do not conflict with the exported ones. */
#define VDE_OK_NOT_AVAILABLE 10  /* a requested item is not available,
                                    see vdeFrtGetItem */
#define VDE_OK_SEGMENTATION_CHANGED 11
                                 /* image segmentation in the Reference
                                    Picture Selection mode has changed,
                                    see rsbGetLatest */

/* VDE states */
#define VDE_STATE_BEGIN 0        /* no frame data decoded */
#define VDE_STATE_MIDDLE 1       /* decoding */
#define VDE_STATE_EOS 2          /* End-of-Stream reached */
#define VDE_STATE_HALTED 3       /* fatal error, halted */
#define VDE_STATE_RESYNC 4       /* Resyncing to INTRA frame */

/* Temporal scalability levels, aliased from h263dext.h */
#define VDE_LEVEL_ALL_FRAMES        H263D_LEVEL_ALL_FRAMES
#define VDE_LEVEL_INTRA_FRAMES      H263D_LEVEL_INTRA_FRAMES
#define VDE_LEVEL_ANNEX_N_LEVEL_0   (VDE_LEVEL_INTRA_FRAMES + 1)


/*
 * Macros
 */

/* Memory allocation wrappers */

#define vdeMalloc malloc
#define vdeCalloc calloc
#define vdeRealloc realloc
#define vdeDealloc free

/* Assertion wrapper */
#ifndef vdeAssert
   #define vdeAssert(exp) assert(exp);
#endif


/*
 * Structures and typedefs
 */

/* {{-output"vdeInstance_t_info.txt" -ignore"*" -noCR}}
   vdeInstance_t holds the instance data for a Video Decoder Engine instance.
   This structure is used to keep track of the internal state of the VDE
   instance.
   {{-output"vdeInstance_t_info.txt"}} */

   /* {{-output"vdeInstance_t.txt"}} */
   typedef struct {
      vdcHInstance_t vdcHInstance;        /* Video Decoder Core instance handle */

      bibBuffer_t *inBuffer;              /* Bit Buffer instance data */


      bibBuffer_t *outBuffer;              /* output stream Bit Buffer instance data */
	  bibBufferEdit_t *bufEdit; 
      int iColorEffect; 
      int iColorToneU; 				             /* U color tone value to be used */
      int iColorToneV; 				             /* V color tone value to be used */
      int iRefQp;                          /* first frame QP for color toning */
	  TBool iGetDecodedFrame;


      vdeIms_t imageStore;                /* Image Store instance data */

      vdeFrtStore_t renStore;             /* Frame type specific store for renderer 
                                             handles */

      vdeFrtStore_t startCallbackStore;   /* Frame type specific store for
                                             "start frame decoding" callbacks */

      vdeFrtStore_t endCallbackStore;     /* Frame type specific store for
                                             "ended frame decoding" callbacks */

      int state;                          /* current state of VDE, see 
                                             VDE_STATE_XXX definitions for
                                             possible values */

      u_int32 creationTime;               /* the time returned by gtGetTime
                                             when the instance was created */

      u_char fMPEG4;                      /* this flag is used to signal the stream type 
                                             "0" H.263, "1" MPEG-4. 
                                             Set by calling vdeDetermineStreamType before 
                                             the first frame of the stream */

      int lumWidth;                       /* Expected size of the incoming */
      int lumHeight;                      /* luminance pictures */

      int requestedScalabilityLevel;      /* Requested temporal scalability 
                                             level to decode.
                                             See vdeSetTemporalScalability 
                                             for further details. */

      int fPrevFrameDecoded;              /* 0 if the previous frame was not
                                             decoded because it did not belong
                                             to requested temporal scalability
                                             level,
                                             1 otherwise */
   } vdeInstance_t;
   /* {{-output"vdeInstance_t.txt"}} */

#endif
// End of File
