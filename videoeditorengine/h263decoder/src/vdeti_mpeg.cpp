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
* Input stream type determination.
*
*/




/*
 * Includes
 */

#include "h263dConfig.h"

#include "debug.h"
#include "sync.h"
#include "vde.h"
#include "vdemain.h" 
#include "mpegcons.h"

#include "core.h"


/*
 * Functions visible outside this module
 */

/*
 * vdeDetermineStreamType
 *
 * Parameters:
 *    hInstance                  instance data
 *
 * Function:
 *   This function detects the stream type, looking at the start code
 *   (h.263 vs. MPEG-4) and sets the flag "fMPEG4" which is
 *   used for the configuration of the decoder. 
 *   In case of an MPEG-4 stream calls the Video Object Header and the VOL Header
 *   decoding function to set initial parameters.
 *
 * Returns:
 *    VDE_OK                     if the stream type could be detected and 
 *                               it is supported
 *    VDE_ERROR_HALTED           if the stream type couldn't be detected or
 *                               the configuration is not supported
 *    VDE_ERROR                  if unrecoverable bitbuffer error occered
 *
 */
int vdeDetermineStreamType(vdeHInstance_t hInstance, CMPEG4Transcoder *hTranscoder)
{
   vdeInstance_t *vdeinstance = (vdeInstance_t *) hInstance;
   bibBuffer_t *inBuffer = vdeinstance->inBuffer;

   int numBitsGot,
      bitErrorIndication = 0;
   int16 error = 0;
   u_int32 bits;

   bits = bibShowBits(32, inBuffer, &numBitsGot, &bitErrorIndication, &error);
   if (error)
      return VDE_ERROR;

   /* If PSC */
   if ((bits >> 10) == 32) {
      vdeinstance->fMPEG4 = 0;
   } 

   /* Else check for Visual Sequence, Visual Object or Video Object start code */
   else if ((bits == MP4_VOS_START_CODE) || 
            (bits == MP4_VO_START_CODE) ||
            ((bits >> MP4_VID_ID_CODE_LENGTH) == MP4_VID_START_CODE) ||
            ((bits >> MP4_VOL_ID_CODE_LENGTH) == MP4_VOL_START_CODE)) {

      vdeinstance->fMPEG4 = 1;

      if (vdcDecodeMPEGVolHeader(vdeinstance->vdcHInstance, inBuffer, hTranscoder) != 0)
         return VDE_ERROR;

      bits = bibShowBits(22, inBuffer, &numBitsGot, &bitErrorIndication, &error);
      if (error)
         return VDE_ERROR;

      /* Check if H.263 PSC follows the VOL header, in which case this is 
         MPEG-4 with short header and is decoded as H.263 */
      if ( bits == 32 ) {
         vdeinstance->fMPEG4 = 0;
      }

   }

   /* Else no H.263 and no MPEG-4 start code detected -> let's try H.263,
      since MPEG-4 cannot anyway be decoded without starting header */
   else {
      vdeinstance->fMPEG4 = 0;
   }

   return VDE_OK;
}

// End of File
