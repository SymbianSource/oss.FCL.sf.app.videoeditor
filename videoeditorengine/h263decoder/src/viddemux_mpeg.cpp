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
* MPEG-4 bitstream parsing.
*
*/


/*
 * Includes
 */

#include "h263dConfig.h"
#include "viddemux.h"
#include "vdxint.h"
#include "mpegcons.h"
#include "sync.h"
#include "vdcaic.h"
#include "zigzag.h"
#include "debug.h"
#include "biblin.h"
/* MVE */
#include "MPEG4Transcoder.h"
// <--

/*
 * Local function prototypes
 */

/* Macroblock Layer */

static int vdxGetIntraDCSize(bibBuffer_t *inBuffer, int compnum, int *IntraDCSize, 
   int *bitErrorIndication);

static int vdxGetRVLCIndex(u_int32 bits, u_int32 *index, int *length, int intra_luma,
   int *bitErrorIndication);


/*
 * Picture Layer Global Functions
 */


/*
 *
 * vdxGetVolHeader
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *    header                     output parameters: VOL header
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 *
 * Function:
 *    This function reads the VO and VOL header from inBuffer.
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *    VDX_ERR_NO_START_CODE      if start code is not found
 *    VDX_ERR_NOT_SUPPORTED      if VOL header 
 *
 *    
 */

int vdxGetVolHeader(
   bibBuffer_t *inBuffer,
   vdxVolHeader_t *header,
   int *bitErrorIndication,
   int getInfo, int *aByteIndex, int *aBitIndex, CMPEG4Transcoder *hTranscoder)
{
   int bitsGot, sncCode, fUseDefaultVBVParams = 0, num_bits;
   int16 bibError = 0;
   u_int32 bits;

   memset(header, 0, sizeof(vdxVolHeader_t));
   *bitErrorIndication = 0;

   /* if Visual Object Sequence Start Code is present */
   bits = bibShowBits(MP4_VOS_START_CODE_LENGTH, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if (bits == MP4_VOS_START_CODE) {

      /* vos_start_code */
      bibFlushBits(MP4_VOS_START_CODE_LENGTH, inBuffer, &bitsGot, bitErrorIndication, &bibError);

      /* profile_and_level_indication (8 bits) */
      bits = bibGetBits(8, inBuffer, &bitsGot, bitErrorIndication, &bibError);
      /* If fatal bit error occurred (lost segment/packet etc.) */
      /*   
           1: Simple Profile Level 1   (from ISO/IEC 14496-2:1999/FPDAM4 [N4350] July 2001)
           2: Simple Profile Level 2   (from ISO/IEC 14496-2:1999/FPDAM4 [N4350] July 2001) 
           3: Simple Profile Level 3   (from ISO/IEC 14496-2:1999/FPDAM4 [N4350] July 2001) 
           8: Simple Profile Level 0   (from ISO/IEC 14496-2:1999/FPDAM4 [N5743] July 2003)
           9: Simple Profile Level 0b  (from ISO/IEC 14496-2:1999/FPDAM4 [N5743] July 2003)
      */
#if 0
      // Disabled since some clips have this set incorrectly, and this is not used for anything important.
      // Only simple profile clips seem to be available so this should not cause any harm.
      // Further, it is still checked in vedVolReader which is always used before editing. It is not checked only when creating thumbnails
      
      
      if (bits != 1 && bits != 2 && bits != 3 && bits != 8 && bits != 9) {
         /* this is not a supported simple profile stream */
         deb("vdxGetMPEGVolHeader: ERROR - This is not a supported simple profile stream\n");
         goto notSupported;
      }
#endif
      if (bits != 8 && bits != 9) 
        header->profile_level = (int) bits;
      else
        header->profile_level = 0;

      /* User data if available */
      bits = bibShowBits(32, inBuffer, &bitsGot, bitErrorIndication, &bibError);
      if (bits == MP4_USER_DATA_START_CODE)
      {
           if (!header->user_data) {
               header->user_data = (char *) malloc(MAX_USER_DATA_LENGTH);
               header->user_data_length = 0;
           }

           if (vdxGetUserData(inBuffer, header->user_data, &(header->user_data_length), bitErrorIndication) != VDX_OK)
               /* also bibError will be handled in exitAfterBitError */
               goto exitAfterBitError;               
       }
   }

   /* if Visual Object Start Code is present */
   bits = bibShowBits(MP4_VO_START_CODE_LENGTH, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if (bits == MP4_VO_START_CODE) {

      /* visual_object_start_code */
      bibFlushBits(MP4_VO_START_CODE_LENGTH, inBuffer, &bitsGot, bitErrorIndication, &bibError);

      /* is_visual_object_identifier (1 bit) */
      bits = bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
      if (bits) {

         /* visual_object_ver_id (4 bits) */
         bits = bibGetBits(4, inBuffer, &bitsGot, bitErrorIndication, &bibError);
         if (bits != 1) {
            /* this is not an MPEG-4 version 1 stream */
            deb("vdxGetMPEGVolHeader: ERROR - This is not an MPEG-4 version 1 stream\n");
            goto notSupported;
         }

         /* visual_object_priority (3 bits) */
         bits = bibGetBits(3, inBuffer, &bitsGot, bitErrorIndication, &bibError);
         header->vo_priority = (int) bits;

      } else {
         header->vo_priority = 0;
      }

      /* visual_object_type (4 bits) */
      bits = bibGetBits(4, inBuffer, &bitsGot, bitErrorIndication, &bibError);
      if (bits != 1) {
         /* this is not a video object */
         deb("vdxGetMPEGVolHeader: ERROR - This is not a video object\n");
         goto notSupported;
      }

      /* is_video_signal_type (1 bit) */
      bits = bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
      if (bits) {

           /* Note: The following fields in the bitstream give information about the 
              video signal type before encoding. These parameters don't influence the 
              decoding algorithm, but the composition at the output of the video decoder.
              There is no normative requirement however to utilize this information 
              during composition, therefore until a way to utilize them is found in
              MoViDe, these fields are just dummyly read, but not interpreted.
              For interpretation see the MPEG-4 Visual standard */

         /* video_format (3 bits) */
         bits = bibGetBits(3, inBuffer, &bitsGot, bitErrorIndication, &bibError);
         header->video_format = (int) bits;

         /* video_range (1 bit) */
         bits = bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
         header->video_range = (int) bits;

         /* colour_description (1 bit) */
         bits = bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
         if (bits) {
             
            /* colour_primaries (8 bits) */
            bits = bibGetBits(8, inBuffer, &bitsGot, bitErrorIndication, &bibError);
            header->colour_primaries = (int) bits;

            /* transfer_characteristics (8 bits) */
            bits = bibGetBits(8, inBuffer, &bitsGot, bitErrorIndication, &bibError);
            header->transfer_characteristics = (int) bits;

            /* matrix_coefficients (8 bits) */
            bits = bibGetBits(8, inBuffer, &bitsGot, bitErrorIndication, &bibError);
            header->matrix_coefficients = (int) bits;
         } else {

            header->colour_primaries = 1; /* default: ITU-R BT.709 */
            header->transfer_characteristics = 1; /* default: ITU-R BT.709 */
            header->matrix_coefficients = 1; /* default: ITU-R BT.709 */
         }

      } else {

         /* default values */
         header->video_format = 5; /* Unspecified video format */
         header->video_range = 0; /* Y range 16-235 pixel values */
         header->colour_primaries = 1; /* ITU-R BT.709 */
         header->transfer_characteristics = 1; /* ITU-R BT.709 */
         header->matrix_coefficients = 1; /* ITU-R BT.709 */
      }

      /* check the next start code */
      sncCode = sncCheckMpegSync(inBuffer, 0, &bibError);
       
      /* If User data is available */
      if (sncCode == SNC_USERDATA)
      {
         if (!header->user_data) {
            header->user_data = (char *) malloc(MAX_USER_DATA_LENGTH);
            header->user_data_length = 0;
         }

         if (vdxGetUserData(inBuffer, header->user_data, &(header->user_data_length), bitErrorIndication) != VDX_OK)
            /* also bibError will be handled in exitAfterBitError */
            goto exitAfterBitError;      
           
      } else if (sncCode != SNC_VID) {

         deb("vdxGetMPEGVolHeader: ERROR. No Start code after VO header\n");
         goto exitAfterBitError;
      }
   }
   
   /* if Video Object Start Code is present */
   bits = bibShowBits(MP4_VID_START_CODE_LENGTH, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if (bits == MP4_VID_START_CODE) {
       /* video_object_start_code */
      bibFlushBits(MP4_VID_START_CODE_LENGTH, inBuffer, &bitsGot, bitErrorIndication, &bibError);

      /* video_object_id */
      bits = bibGetBits(MP4_VID_ID_CODE_LENGTH, inBuffer, &bitsGot, bitErrorIndication, &bibError);
      header->vo_id = (int) bits;
   }

   /* MVE */
   /* dummy indication of the position before VOL start code, only for recoginition of shortheader */
   if (!getInfo)
   {
       hTranscoder->ErrorResilienceInfo(NULL, inBuffer->getIndex, inBuffer->bitIndex);
   }

   /* vol_start_code */
   bits = bibShowBits(MP4_VOL_START_CODE_LENGTH, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if(bits != MP4_VOL_START_CODE)
   {
      /* If H.263 PSC, this is a short header stream, i.e. H.263 baseline */
      if ( (bits >> 6) == 32 ) {
         return VDX_OK;
      } else {      
         deb("vdxGetMPEGVolHeader: ERROR - Bitstream does not start with MP4_VOL_START_CODE\n");
         goto exitAfterBitError;
      }  
   }

   bibFlushBits(MP4_VOL_START_CODE_LENGTH, inBuffer, &bitsGot, bitErrorIndication, &bibError);

   /* vol_id */
   bits = bibGetBits(MP4_VOL_ID_CODE_LENGTH, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   header->vol_id = (int) bits;

   /* random_accessible_vol (1 bit) */
   bits = bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   header->random_accessible_vol = (u_char) bits;

   /* video_object_type_indication (8 bits) */
   bits = bibGetBits(8, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if (bits != 1) {
       /* this is not a simple video object stream */
       deb("vdxGetMPEGVolHeader: ERROR - This is not a simple video object stream\n");
       goto notSupported;
   }

   /* is_object_layer_identifier (1 bit) */
   bits = bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if (bits) {
      /* video_object_layer_verid (4 bits) */
      bits = bibGetBits(4, inBuffer, &bitsGot, bitErrorIndication, &bibError);
      if (bits != 1) {
         /* this is not an MPEG-4 version 1 stream */
         deb("vdxGetMPEGVolHeader: ERROR - This is not an MPEG-4 version 1 stream\n");
         goto notSupported;
      }

      /* video_object_layer_priority (3 bits) */
      bits = bibGetBits(3, inBuffer, &bitsGot, bitErrorIndication, &bibError);
      header->vo_priority = (int) bits;
   } 

   /* aspect_ratio_info: `0010`- 12:11 (625-type for 4:3 picture) */
   bits = bibGetBits(4, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   header->pixel_aspect_ratio = (int) bits;

   /* extended par */
   if (bits == 15) { 
      /* par_width */
      bits = bibGetBits(8, inBuffer, &bitsGot, bitErrorIndication, &bibError);
      /* par_height */
      bits = bibGetBits(8, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   }
   
   /* vol_control_parameters flag */
   bits = bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if (bits) {

      /* chroma_format (2 bits) */
      bits = bibGetBits(2, inBuffer, &bitsGot, bitErrorIndication, &bibError);
      if (bits != 1) {
         goto exitAfterBitError;
      }

      /* low_delay (1 bits) */
      bits = bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);

      /* vbv_parameters (1 bits) */
      bits = bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
      if (bits) {

         /* first_half_bitrate (15 bits) */
         bits = bibGetBits(15, inBuffer, &bitsGot, bitErrorIndication, &bibError);
         header->bit_rate = (bits << 15);
          
         /* marker_bit */
         if (!bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError))
            goto exitAfterBitError;
           
         /* latter_half_bitrate (15 bits) */
         bits = bibGetBits(15, inBuffer, &bitsGot, bitErrorIndication, &bibError);
         header->bit_rate += bits;
         header->bit_rate *= 400;
           
         /* marker_bit */
         if (!bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError))
            goto exitAfterBitError;

         /* first_half_vbv_buffer_size (15 bits) */
         bits = bibGetBits(15, inBuffer, &bitsGot, bitErrorIndication, &bibError);
         header->vbv_buffer_size = (bits << 3);
           
         /* marker_bit */
         if (!bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError))
            goto exitAfterBitError;
           
         /* latter_half_vbv_buffer_size (3 bits) */
         bits = bibGetBits(3, inBuffer, &bitsGot, bitErrorIndication, &bibError);
         header->vbv_buffer_size += bits;
         header->vbv_buffer_size <<= 14 /**= 16384*/;
           
         /* first_half_vbv_occupancy (11 bits) */
         bits = bibGetBits(11, inBuffer, &bitsGot, bitErrorIndication, &bibError);
         header->vbv_occupancy = (bits << 15);
           
         /* marker_bit */
         if (!bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError))
            goto exitAfterBitError;
           
         /* latter_half_vbv_occupancy (15 bits) */
         bits = bibGetBits(15, inBuffer, &bitsGot, bitErrorIndication, &bibError);
         header->vbv_occupancy += bits;
         header->vbv_occupancy <<= 6 /**= 64*/;
           
         /* marker_bit */
         if (!bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError))
            goto exitAfterBitError;

      } else {
         fUseDefaultVBVParams = 1;
      }
   } else {
      fUseDefaultVBVParams = 1;
   }

   if (fUseDefaultVBVParams) {

      /* default values */
      header->vbv_buffer_size = 
        ((header->profile_level == 0 || header->profile_level == 1) ? 5 : 20);
      header->vbv_occupancy = header->vbv_buffer_size*170;
      header->bit_rate = 
        ((header->profile_level == 0 || header->profile_level == 1) ? 64 : 
        ((header->profile_level == 2) ? 128 : 384));
       
      header->vbv_occupancy <<= 6 /**= 64*/;
      header->vbv_buffer_size <<= 14 /**= 16384*/;
      header->bit_rate <<= 10 /**= 1024*/;
   }

   /* vol_shape (2 bits) */
   bits = bibGetBits(2, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   /* rectangular_shape = '00' */
   if (bits != 0) {
      deb("vdxGetMPEGVolHeader: ERROR - Not rectangular shape is not supported\n");
      goto notSupported;
   }

   /* marker_bit */
   if (!bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError))
      goto exitAfterBitError;
    
   /* MVE */
   if (!getInfo)
   {
       hTranscoder->MPEG4TimerResolution(inBuffer->getIndex, inBuffer->bitIndex);
   }
   
   /* time_increment_resolution */
   bits = bibGetBits(16, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   header->time_increment_resolution = (int) bits;
 
   /* marker_bit */
   if (!bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError))
      goto exitAfterBitError;
    
   /* fixed_vop_rate */
   bits = bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);

   /* MVE */
   header->fixed_vop_rate = (u_char)bits;

   /* fixed_vop_time_increment (1-15 bits) */
   if (bits) {
      for (num_bits = 1; ((header->time_increment_resolution-1) >> num_bits) != 0; num_bits++)
        {
        }
 
      bits = bibGetBits(num_bits, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   }

   /* if rectangular_shape !always! */

   /* marker_bit */
   if (!bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError))
      goto exitAfterBitError;
    
   /* vol_width (13 bits) */
   bits = bibGetBits(13, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   header->lumWidth = (int) bits;

   /* marker_bit */
   if (!bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError))
      goto exitAfterBitError;
    
   /* vol_height (13 bits) */
   bits = bibGetBits(13, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   header->lumHeight = (int) bits;

   /* endif rectangular_shape */

   /* marker_bit */
   if (!bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError))
      goto exitAfterBitError;
    
   /* interlaced (1 bit) */
   bits = bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if (bits)
   {
      deb("vdxGetMPEGVolHeader: ERROR - Interlaced VOP not supported\n");
      goto notSupported;
   }

   /* OBMC_disable */
   bits = bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if (!bits) {
     deb("vdxGetMPEGVolHeader: ERROR - Overlapped motion compensation not supported\n");
     goto notSupported;
   }

   /* sprite_enable (1 bit) */
   bits = bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if (bits) {
     deb("vdxGetMPEGVolHeader: ERROR - Sprites not supported\n");
     goto notSupported;
   }

   /* not_8_bit (1 bit) */
   bits = bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if (bits) {
     deb("vdxGetMPEGVolHeader: ERROR - Not 8 bits/pixel not supported\n");
     goto notSupported;
   }

   /* quant_type (1 bit) */
   bits = bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if (bits) {
     deb("vdxGetMPEGVolHeader: ERROR - H.263/MPEG-2 Quant Table switch not supported\n");
     goto notSupported;
   }

   /* complexity_estimation_disable (1 bit) */
   bits = bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if (!bits) {
     deb("vdxGetMPEGVolHeader: ERROR - Complexity estimation header not supported\n");
     goto notSupported;
   }
  
   /* MVE */
   if (!getInfo)
   {
       hTranscoder->ErrorResilienceInfo(NULL, inBuffer->getIndex, inBuffer->bitIndex);
   }
   else
   {
        *aByteIndex = inBuffer->getIndex;
        *aBitIndex = inBuffer->bitIndex;
   }

   /* resync_marker_disable (1 bit) */
   bits = bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   header->error_res_disable = (u_char) bits;

   /* data_partitioned (1 bit) */
   bits = bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   header->data_partitioned = (u_char) bits;

   if (header->data_partitioned) {
      /* reversible_vlc (1 bit) */
      bits = bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
      header->reversible_vlc = (u_char) bits;
   }

   /* MVE */
   if (!getInfo)
   {
       hTranscoder->ErrorResilienceInfo(header, 0, 0);
   }

   /* scalability (1 bit) */
   bits = bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if (bits) {
     deb("vdxGetMPEGVolHeader: ERROR - Scalability not supported\n");
     goto notSupported;
   }

   /* check the next start code */
   sncCode = sncCheckMpegSync(inBuffer, 0, &bibError);

   /* Removed since the VOL header may be followed by another header (H.245 & MPEG-4 signaling)
      and sncCheckMpegSync does not recognize VOS start code
   if (sncCode == SNC_NO_SYNC || bibError)
   {
      deb("vdxGetMPEGVolHeader: ERROR. No Start code after VOL header\n");
      goto exitAfterBitError;
   } 
   */
   /* If User data is available */
   if (sncCode == SNC_USERDATA && !bibError)
   {
      if (!header->user_data) {
         header->user_data = (char *) malloc(MAX_USER_DATA_LENGTH);
         header->user_data_length = 0;
      }
       
      if (vdxGetUserData(inBuffer, header->user_data, &(header->user_data_length), bitErrorIndication) != VDX_OK)
         /* also bibError will be handled in exitAfterBitError */
         goto exitAfterBitError;       
   }

   /* If no error in bit buffer functions */
   if (!bibError)
      return VDX_OK;

   /* Else if ran out of data (i.e. decoding out of sync) */
   else if (bibError == ERR_BIB_NOT_ENOUGH_DATA) {
      return VDX_OK_BUT_BIT_ERROR;
   }

   /* Else other error in bit buffer functions */
   else
      return VDX_ERR_BIB;

   exitAfterBitError:
   if (bibError && bibError != ERR_BIB_NOT_ENOUGH_DATA)
       return VDX_ERR_BIB;

   return VDX_OK_BUT_BIT_ERROR;

   notSupported:
   return VDX_ERR_NOT_SUPPORTED;
}

/* {{-output"vdxGetGovHeader.txt"}} */
/*
 *
 * vdxGetGovHeader
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *    header                     output parameters: picture header
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 *
 * Function:
 *    This function reads the GOV header from inBuffer.
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *    VDX_ERR_NO_START_CODE      if start code is not found
 *    VDX_ERR_NOT_SUPPORTED      if broken_link and closed_gov conflict
 *
 */

int vdxGetGovHeader(
   bibBuffer_t *inBuffer,
   vdxGovHeader_t *header,
   int *bitErrorIndication)
/* {{-output"vdxGetGovHeader.txt"}} */
{
   int tmpvar;
   u_int32 bits;
   int time_s;
   int bitsGot, sncCode;
   int16 bibError = 0;

   vdxAssert(inBuffer != NULL);
   vdxAssert(header != NULL);
   vdxAssert(bitErrorIndication != NULL);

   memset(header, 0, sizeof(vdxGovHeader_t));
   *bitErrorIndication = 0;

   /* group_start_code (32 bits) */
   bits = bibGetBits(32, inBuffer, &bitsGot, bitErrorIndication, &bibError);

   if ( bibError )
       goto exitAfterBitError;
   if( bits != MP4_GROUP_START_CODE )
   {
      deb0p("ERROR. Bitstream does not start with MP4_GROUP_START_CODE\n");
      goto exitAfterBitError;
   }

   /* time_code_hours (5 bits) */
   tmpvar = (int) bibGetBits(5, inBuffer, &bitsGot, bitErrorIndication, &bibError);

   if ( bibError )
       goto exitAfterBitError;

   time_s= tmpvar*3600;
 
   /* time_code_minutes (6 bits) */
   tmpvar = (int) bibGetBits(6, inBuffer, &bitsGot, bitErrorIndication, &bibError);

   if ( bibError )
       goto exitAfterBitError;

   time_s += tmpvar*60;

   /* marker_bit (1 bit) */
   tmpvar = bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if ( bibError )
       goto exitAfterBitError;
   if ( !tmpvar )
      goto exitAfterBitError;

   /* time_code_seconds (6 bits) */
   tmpvar = (int) bibGetBits(6, inBuffer, &bitsGot, bitErrorIndication, &bibError);
    
   time_s += tmpvar;

   /* closed_gov (1 bit) */
   bits = bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if ( bibError )
       goto exitAfterBitError;

   header->closed_gov= (u_char) bits;

   /* broken_link (1 bit) */
   bits = bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if ( bibError )
       goto exitAfterBitError;
   
   header->broken_link = (u_char) bits;   

   if ( (header->closed_gov == 0)&&(header->broken_link == 1) )
   {
      deb0p("ERROR. GOVHeader: closed_gov = 0\tbroken_link = 1\n");
      goto exitAfterBitError;
   }

   /* Stuff the bits before the next start code */
   sncCode = sncCheckMpegSync(inBuffer, 1, &bibError);
   if ((sncCode == SNC_NO_SYNC) || bibError)
   {
      deb0p("ERROR. No VOP Start code after GOV header\n");
      /* also bibError will be handled in exitAfterBitError */
      goto exitAfterBitError;
   }

    /* If User data is available */
   if (sncCode == SNC_USERDATA)
   {
      if (!header->user_data) {
         header->user_data = (char *) malloc(MAX_USER_DATA_LENGTH);
         header->user_data_length = 0;
      }
       
      if (vdxGetUserData(inBuffer, header->user_data, &(header->user_data_length), bitErrorIndication) != VDX_OK)
          /* also bibError will be handled in exitAfterBitError */
          goto exitAfterBitError;       
   }

   header->time_stamp = time_s;

   /* If no error in bit buffer functions */
   if (!bibError)
      return VDX_OK;

   /* Else if ran out of data (i.e. decoding out of sync) */
   else if (bibError == ERR_BIB_NOT_ENOUGH_DATA) {
      return VDX_OK_BUT_BIT_ERROR;
   }

   /* Else other error in bit buffer functions */
   else
      return VDX_ERR_BIB;

   exitAfterBitError:
      if (bibError && bibError != ERR_BIB_NOT_ENOUGH_DATA)
         return VDX_ERR_BIB;

   return VDX_OK_BUT_BIT_ERROR;

} 

/* {{-output"vdxGetVopHeader.txt"}} */
/*
 *
 * vdxGetVopHeader
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *    inParam                    input parameters
 *    header                     output parameters: picture header
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 *
 * Function:
 *    This function reads the VOP header from inBuffer.
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *    VDX_ERR_NO_START_CODE      if start code is not found
 *    VDX_ERR_NOT_SUPPORTED      if broken_link and closed_gov conflict
 *
 *    
 */

int vdxGetVopHeader(
   bibBuffer_t *inBuffer,
   const vdxGetVopHeaderInputParam_t *inpParam,
   vdxVopHeader_t *header,
   int * ModuloByteIndex,
   int * ModuloBitIndex,
   int * ByteIndex,
   int * BitIndex,
   int *bitErrorIndication)
/* {{-output"vdxGetVopHeader.txt"}} */
{

   int tmpvar;
   u_int32 bits;
   int bitsGot;
   int time_base_incr = 0,
      num_bits;
   int16
      bibError = 0;

   vdxAssert(inBuffer != NULL);
   vdxAssert(header != NULL);
   vdxAssert(bitErrorIndication != NULL);

   memset(header, 0, sizeof(vdxVopHeader_t));
   *bitErrorIndication = 0;

   /* vop_start_code (32 bits) */
   bits = bibGetBits(MP4_VOP_START_CODE_LENGTH, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if ( bibError )
      goto exitFunction;
 
   if( bits != MP4_VOP_START_CODE  )
   {
      deb0p("ERROR. Bitstream does not start with MP4_VOP_START_CODE\n");
      goto exitAfterBitError;
   }

   *bitErrorIndication = 0;   

   /* vop_prediction_type (2 bits) */
   bits = bibGetBits(2, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if ( bibError )
       goto exitAfterBitError;

   if ( (bits != VDX_VOP_TYPE_P && bits != VDX_VOP_TYPE_I) )
   {
      deb("ERROR. Not supported VOP prediction type\n");
      goto exitAfterBitError;
   }
  
   header->coding_type = (u_char) bits;

   /* MVE */
   *ModuloByteIndex = inBuffer->numBytesRead;
   *ModuloBitIndex = inBuffer->bitIndex;

   /* modulo_time_base (? bits) */
   tmpvar = (int) bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if ( bibError )
       goto exitAfterBitError;

   while( tmpvar == 1 && !bibError )
   {
      tmpvar = (int) bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
      time_base_incr++;
   }
   if ( bibError )
       goto exitAfterBitError;

   header->time_base_incr = time_base_incr;

   /* marker_bit (1 bit) */
   tmpvar = bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if ( bibError )
       goto exitAfterBitError;
   if ( !tmpvar )
      goto exitAfterBitError;

   /* MVE */
   *ByteIndex = inBuffer->numBytesRead;
   *BitIndex = inBuffer->bitIndex;

   /* vop_time_increment (1-16 bits) */
   for (num_bits = 1; ((inpParam->time_increment_resolution-1) >> num_bits) != 0; num_bits++)
    {
    }
 
   tmpvar = (int) bibGetBits(num_bits, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if ( bibError )
       goto exitAfterBitError;

   header->time_inc = tmpvar;

   /* marker_bit (1 bit) */
   tmpvar = bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if ( bibError )
       goto exitAfterBitError;
   if ( !tmpvar )
      goto exitAfterBitError;

   /* vop_coded (1 bit) */
   header->vop_coded = (u_char) bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if ( bibError )
       goto exitAfterBitError;

   if ( !header->vop_coded ) {
      goto exitAfterBitError;
   }
   
   /* vop_rounding_type (1 bit) */
   if (header->coding_type == VDX_VOP_TYPE_P)
   {
      bits = bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
      if ( bibError )
          goto exitAfterBitError;
      header->rounding_type = (int) bits;
   } else
      header->rounding_type = 0;

   /* intra_dc_vlc_thr (3 bits) */
   bits = bibGetBits(3, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if ( bibError )
       goto exitAfterBitError;

   header->intra_dc_vlc_thr = (int) bits;

   /* vop_quant (5 bits) */
   bits = bibGetBits(5, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if ( bibError )
       goto exitAfterBitError;

   header->quant = (int) bits;

   /* vop_fcode_forward (3 bits) */
   if (header->coding_type == VDX_VOP_TYPE_P)
   {
      bits = bibGetBits(3, inBuffer, &bitsGot, bitErrorIndication, &bibError);
      if ( bibError )
          goto exitAfterBitError;
      header->fcode_forward = (int) bits;
   } else {
      /* In case of an Intra Frame to calculate the length of the 
         VP resynchronization marker later on fcode_forward should be 
         assumed to have the value 1. */
      header->fcode_forward = 1;
   }

   exitFunction:

   /* If no error in bit buffer functions */
   if (!bibError)
      return VDX_OK;

   /* Else if ran out of data (i.e. decoding out of sync) */
   else if (bibError == ERR_BIB_NOT_ENOUGH_DATA) {
      return VDX_OK_BUT_BIT_ERROR;
   }

   /* Else other error in bit buffer functions */
   else
      return VDX_ERR_BIB;

   exitAfterBitError:
   
   if (bibError && bibError != ERR_BIB_NOT_ENOUGH_DATA)
      return VDX_ERR_BIB;

   return VDX_OK_BUT_BIT_ERROR;

}

/*
 * vdxGetUserData
 *    
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *    user_data                  string of user data read byte-by-byte from
 *                               the stream
 *    user_data_length           number of bytes of user data
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 *
 * Function:
 *    This function reads a string of bytes as user data from the bitsream
 *    It is called from te VOL header or GOV header
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 *    
 */
int vdxGetUserData(bibBuffer_t *inBuffer,
   char *user_data, int *user_data_length,
   int *bitErrorIndication)
{
   int i;
   u_int32 bits;
   int bitsGot;
   int16 ownError=0;

   /* user_data_start_code (32 bits) */
   bits = bibGetBits(32, inBuffer, &bitsGot, bitErrorIndication, &ownError);
   if ( ownError )
      goto exitFunction;
 
   if (bits != MP4_USER_DATA_START_CODE)
   {
      deb0p("ERROR. Bitstream does not start with MP4_USER_DATA_START_CODE\n");
      goto exitFunction;
   }

   /* read until start_code 0x000001 */
   for ( i=(*user_data_length);
         (((bits = bibShowBits(24, inBuffer, &bitsGot, bitErrorIndication, &ownError)) != 0x000001)&&(ownError==0));
         i++ )
   {
      bits = bibGetBits(8, inBuffer, &bitsGot, bitErrorIndication, &ownError);
      if (ownError)
         goto exitFunction;

      if (i<MAX_USER_DATA_LENGTH) 
      {
          user_data[i] = (u_char) bits;
      }
      else
      {
          break;
      }
   }
   

   *user_data_length = (i>=MAX_USER_DATA_LENGTH) ? MAX_USER_DATA_LENGTH : i++;

   exitFunction:

   /* If no error in bit buffer functions */
   if (!ownError)
      return VDX_OK;

   /* Else if ran out of data (i.e. decoding out of sync) */
   else if (ownError == ERR_BIB_NOT_ENOUGH_DATA) {
      return VDX_OK_BUT_BIT_ERROR;
   }

   /* Else other error in bit buffer functions */
   else
      return VDX_ERR_BIB;

}

/*
 * Video Packet Layer Global Functions
 */

/* {{-output"vdxGetVideoPacketHeader.txt"}} */
/*
 *
 * vdxGetVideoPacketHeader
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *    header                     output parameters: picture header
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 *
 * Function:
 *    This function reads the Video Packet header from inBuffer.
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *    VDX_ERR_NO_START_CODE      if start code is not found
 *    VDX_ERR_NOT_SUPPORTED      if broken_link and closed_gov conflict
 *
 *      
 */

int vdxGetVideoPacketHeader(
   bibBuffer_t *inBuffer,
   const vdxGetVideoPacketHeaderInputParam_t *inpParam,
   vdxVideoPacketHeader_t *header,
   int *bitErrorIndication)
/* {{-output"vdxGetVideoPacketHeader.txt"}} */
{
   int tmpvar, num_bits, bitsGot;
   u_int32 bits;
   int MBNumLength,
      time_base_incr = 0;
   int16
      bibError = 0;

   vdxAssert(inBuffer != NULL);
   vdxAssert(header != NULL);
   vdxAssert(bitErrorIndication != NULL);

   memset(header, 0, sizeof(vdxVideoPacketHeader_t));
   *bitErrorIndication = 0;

   /* resync marker */
   tmpvar = bibGetBits(16 + inpParam->fcode_forward, inBuffer, &bitsGot,
                   bitErrorIndication, &bibError);
   if ( bibError )
       goto exitAfterBitError;
   if ( (int) tmpvar != MP4_RESYNC_MARKER )
   {
      deb("ERROR. No Resync Marker found\n");
      goto exitAfterBitError;
   }

   /* Macroblock Number */
   for (MBNumLength = 1; ((inpParam->numOfMBs-1) >> MBNumLength) != 0; MBNumLength++)
    {
    }

   header->currMBNum = (int) bibGetBits(MBNumLength, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if ( bibError )
       goto exitAfterBitError;

   /* quant_scale (5 bits) */
   bits = bibGetBits(5, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if ( bibError )
       goto exitAfterBitError;

   header->quant = (int) bits;

   /* header_extension_code (1 bit) */
   header->fHEC = (u_char) bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
   if ( bibError )
       goto exitAfterBitError;

   if(header->fHEC) {
      /* modulo_time_base (? bits) */
      tmpvar = (int) bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
      if ( bibError )
          goto exitAfterBitError;
      
      while (tmpvar == 1 && !bibError)
      {
         tmpvar = (int) bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
         time_base_incr++;
      }
      if ( bibError )
          goto exitAfterBitError;

      header->time_base_incr = time_base_incr;

      /* marker_bit (1 bit) */
      tmpvar = bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
      if ( bibError )
          goto exitAfterBitError;
      if ( !tmpvar )
         goto exitAfterBitError;

      /* vop_time_increment (1-15 bits) */
      for (num_bits = 1; ((inpParam->time_increment_resolution-1) >> num_bits) != 0; num_bits++)
        {
        }
      
      tmpvar = (int) bibGetBits(num_bits, inBuffer, &bitsGot, bitErrorIndication, &bibError);
      if ( bibError )
          goto exitAfterBitError;

      header->time_inc = tmpvar;
      
      /* marker_bit (1 bit) */
      tmpvar = bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);
      if ( bibError )
          goto exitAfterBitError;
      if (!tmpvar)
         goto exitAfterBitError;

      /* vop_prediction_type (2 bits) */
      bits = bibGetBits(2, inBuffer, &bitsGot, bitErrorIndication, &bibError);
      if ( bibError )
          goto exitAfterBitError;

      if ((bits != VDX_VOP_TYPE_P && bits != VDX_VOP_TYPE_I))
      {
         deb("ERROR. Not supported VOP prediction type\n");
         goto exitAfterBitError;
      }
      
      header->coding_type = (u_char) bits;

      /* intra_dc_vlc_thr (3 bits) */
      bits = bibGetBits(3, inBuffer, &bitsGot, bitErrorIndication, &bibError);
      if ( bibError )
          goto exitAfterBitError;

      header->intra_dc_vlc_thr = (int) bits;

      /* vop_fcode_forward (3 bits) */
      if (header->coding_type == VDX_VOP_TYPE_P)
      {
         bits = bibGetBits(3, inBuffer, &bitsGot, bitErrorIndication, &bibError);
         if ( bibError )
             goto exitAfterBitError;
         header->fcode_forward = (int) bits;
      } else
         header->fcode_forward = 1;
   }

   /* Check success and return */

   /* If no error in bit buffer functions */
   if (!bibError)
      return VDX_OK;

   /* Else if ran out of data (i.e. decoding out of sync) */
   else if (bibError == ERR_BIB_NOT_ENOUGH_DATA) {
      return VDX_OK_BUT_BIT_ERROR;
   }

   /* Else other error in bit buffer functions */
   else
      return VDX_ERR_BIB;

   exitAfterBitError:
      if (bibError && bibError != ERR_BIB_NOT_ENOUGH_DATA)
         return VDX_ERR_BIB;

      return VDX_OK_BUT_BIT_ERROR;
}

/*
 * Macroblock Layer Global Functions
 */

/*
 *
 * vdxGetDataPartitionedIMBLayer_Part1
 *
 * Parameters:
 *    inBuffer     input buffer
 *    inpParam     input parameters
 *    MBList       a double-linked list for soring 
 *                 MB parameters + DC values in the VP
 *    bitErrorIndication
 *                 non-zero if a bit error has been detected
 *                 within the bits accessed in this function,
 *                 see biterr.h for possible values
 *
 * Function:
 *    This function gets the first (DC) partition of a data
 *    partitioned encoded Video Packet in an Intra-VOP. 
 *    The parameters MCBPC, DQUANT and DC values of all the MBs
 *    in the VP are read and stored in the linked list.
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 * Error codes:
 *    error codes returned by bibFlushBits/bibGetBits/bibShowBits
 *
 *
 */

int vdxGetDataPartitionedIMBLayer_Part1(
   bibBuffer_t *inBuffer, bibBuffer_t *outBuffer, bibBufferEdit_t *bufEdit,
     int aColorEffect, int *aStartByteIndex, int *aStartBitIndex, 
     CMPEG4Transcoder *hTranscoder, 
   const vdxGetDataPartitionedIMBLayerInputParam_t *inpParam,
   dlst_t *MBList,
   int *bitErrorIndication)
{

    int mcbpcIndex,
        retValue = VDX_OK,
        fDQUANT,
        new_quant, previous_quant,
        bitsGot,
        i, 
        IntraDC_size, 
        IntraDC_delta;
    
    /* MVE */
   int StartByteIndex;
   int StartBitIndex;
     
   int16 error=0;
   vdxIMBListItem_t *MBinstance = NULL;
     
     
   vdxAssert(inpParam != NULL);
   vdxAssert(inBuffer != NULL);
   vdxAssert(bitErrorIndication != NULL);
     
   previous_quant = inpParam->quant;

   /* MVE */
   int stuffingStartByteIndex, stuffingStartBitIndex, stuffingEndByteIndex, stuffingEndBitIndex;
   stuffingStartByteIndex = stuffingEndByteIndex = inBuffer->getIndex;
   stuffingStartBitIndex  = stuffingEndBitIndex = inBuffer->bitIndex;

   while (bibShowBits(MP4_DC_MARKER_LENGTH, inBuffer, &bitsGot, bitErrorIndication, &error) != MP4_DC_MARKER && !error)
   {
         /* MVE */
         StartByteIndex = inBuffer->getIndex; 
         StartBitIndex = inBuffer->bitIndex;
         
         retValue = vdxGetMCBPCIntra(inBuffer, &mcbpcIndex, bitErrorIndication);
         if (retValue != VDX_OK)
             goto exitFunction;
         
     if (mcbpcIndex == 8)
         {
             /* MVE */
             stuffingEndByteIndex = inBuffer->getIndex;
             stuffingEndBitIndex = inBuffer->bitIndex;
             
             continue; /* skip stuffing */
         }
         
         /* Create new MBInstance for the next MB */
         MBinstance = (vdxIMBListItem_t *) malloc(sizeof(vdxIMBListItem_t));
         if (!MBinstance)
         {
             deb("ERROR - MBinstance creation failed\n");
             retValue = H263D_ERROR;
             goto exitFunction;
         }
         memset(MBinstance, 0, sizeof(vdxIMBListItem_t));
         
         /* CBPC (2 LSBs of MCBPC) */
         MBinstance->cbpc = mcbpcIndex & 3;
         
         /* MVE */
         MBinstance->mcbpc = mcbpcIndex;
         VDT_SET_START_POSITION(MBinstance,11,stuffingStartByteIndex,stuffingStartBitIndex); // 11: MB stuffing bits
         VDT_SET_END_POSITION(MBinstance,11,stuffingEndByteIndex,stuffingEndBitIndex); // 11: MB stuffing bits
         
         VDT_SET_START_POSITION(MBinstance,0,StartByteIndex,StartBitIndex); // 0: mcbpc
         VDT_SET_END_POSITION(MBinstance,0,inBuffer->getIndex,inBuffer->bitIndex); // 0: mcbpc
         VDT_SET_START_POSITION(MBinstance,1,inBuffer->getIndex,inBuffer->bitIndex); // 1: dquant
         
         /* DQUANT is given for MCBPC indexes 4..7 */
         fDQUANT = mcbpcIndex & 4;
         
         if (fDQUANT) {
             retValue = vdxUpdateQuant(inBuffer, 0, previous_quant,
                 &new_quant, bitErrorIndication);
             if (retValue != VDX_OK)
                 goto exitFunction;
         }
         /* Else no DQUANT */
         else
             new_quant = previous_quant;
         
         MBinstance->dquant       = fDQUANT ?  new_quant - previous_quant : 0;
         MBinstance->quant = previous_quant = new_quant;
         
         /* MVE */
         VDT_SET_END_POSITION(MBinstance,1,inBuffer->getIndex,inBuffer->bitIndex); // 1: dquant
     VDT_SET_START_POSITION(MBinstance,4,inBuffer->getIndex,inBuffer->bitIndex); // 4: intraDC
     VDT_SET_START_POSITION(MBinstance,5,inBuffer->getIndex,inBuffer->bitIndex); // 5: intraDC
     VDT_SET_START_POSITION(MBinstance,6,inBuffer->getIndex,inBuffer->bitIndex); // 6: intraDC
     VDT_SET_START_POSITION(MBinstance,7,inBuffer->getIndex,inBuffer->bitIndex); // 7: intraDC
     VDT_SET_START_POSITION(MBinstance,8,inBuffer->getIndex,inBuffer->bitIndex); // 8: intraDC
     VDT_SET_START_POSITION(MBinstance,9,inBuffer->getIndex,inBuffer->bitIndex); // 9: intraDC
         
     VDT_SET_END_POSITION(MBinstance,4,inBuffer->getIndex,inBuffer->bitIndex); // 4: intraDC
     VDT_SET_END_POSITION(MBinstance,5,inBuffer->getIndex,inBuffer->bitIndex); // 5: intraDC
     VDT_SET_END_POSITION(MBinstance,6,inBuffer->getIndex,inBuffer->bitIndex); // 6: intraDC
     VDT_SET_END_POSITION(MBinstance,7,inBuffer->getIndex,inBuffer->bitIndex); // 7: intraDC
     VDT_SET_END_POSITION(MBinstance,8,inBuffer->getIndex,inBuffer->bitIndex); // 8: intraDC
     VDT_SET_END_POSITION(MBinstance,9,inBuffer->getIndex,inBuffer->bitIndex); // 9: intraDC

         /* Color Toning */
     hTranscoder->AfterMBLayer(new_quant);
 
         
         MBinstance->switched = aicIntraDCSwitch(inpParam->intra_dc_vlc_thr,new_quant);
         
         /* Intra_DC_Coeffs */
         if(!MBinstance->switched) {
             for (i=0; i<6; i++) {
                 
                 /* MVE */
                 VDT_SET_START_POSITION(MBinstance,i+4,inBuffer->getIndex,inBuffer->bitIndex); // 6: intraDC,chrominance
                 
                 retValue = vdxGetIntraDC(inBuffer, outBuffer, bufEdit, aColorEffect, aStartByteIndex, aStartBitIndex, 
                     i, &IntraDC_size, &IntraDC_delta, bitErrorIndication);
                 
                 /* MVE */
                 VDT_SET_END_POSITION(MBinstance,i+4,inBuffer->getIndex,inBuffer->bitIndex); // 6: intraDC,chrominance
                 
                 if (retValue != VDX_OK)
                     goto exitFunction;
                 
                 MBinstance->DC[i] = IntraDC_delta;
             } 
         } 
         
         
         /* Put MBinstance into the queue */
         dlstAddAfterCurr(MBList, MBinstance);
         MBinstance = NULL;
         
     /* MVE */
         // begin another MB, record the position of MB stuffing
         stuffingStartByteIndex = stuffingEndByteIndex = inBuffer->getIndex;
         stuffingStartBitIndex  = stuffingEndBitIndex = inBuffer->bitIndex;
         
   }
     
exitFunction:
     
   if (MBinstance)
      free(MBinstance);


   /* If no error in bit buffer functions */
   if (!error)
      return retValue;

   /* Else if ran out of data (i.e. decoding out of sync) */
   else if (error == ERR_BIB_NOT_ENOUGH_DATA) {
      return VDX_OK_BUT_BIT_ERROR;
   }

   /* Else other error in bit buffer functions */
   else
      return VDX_ERR_BIB;
}

/*
 *
 * vdxGetDataPartitionedIMBLayer_Part2
 *
 * Parameters:
 *    inBuffer     input buffer
 *    MBList       a double-linked list for soring 
 *                 MB parameters + DC values in the VP
 *    numMBsInVP   number of MBs in this VP
 *    bitErrorIndication
 *                 non-zero if a bit error has been detected
 *                 within the bits accessed in this function,
 *                 see biterr.h for possible values
 *
 * Function:
 *    This function gets the second (CBPY) partition of a data
 *    partitioned encoded Video Packet in an Intra-VOP. 
 *    The parameters CBPY and ac_pred_flag of all the MBs
 *    in the VP are read and stored in the linked list.
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 * Error codes:
 *      error codes returned by bibFlushBits/bibGetBits/bibShowBits
 *
 *      
 *
 */

int vdxGetDataPartitionedIMBLayer_Part2(
   bibBuffer_t *inBuffer, bibBuffer_t */*outBuffer*/, bibBufferEdit_t */*bufEdit*/, 
   int /*aColorEffect*/, int */*aStartByteIndex*/, int */*aStartBitIndex*/, 
   dlst_t *MBList,
   int numMBsInVP,
   int *bitErrorIndication)
{

   int cbpyIndex,
      i,
      retValue = VDX_OK,
      bitsGot;

   int16 error=0;
   u_char code;
   vdxIMBListItem_t *MBinstance;

   vdxAssert(inBuffer != NULL);
   vdxAssert(bitErrorIndication != NULL);

   /* get the first MB of the list */
   dlstHead(MBList, (void **) &MBinstance);

   /* Get ac_pred_flag and cbpy of all MBs in VP */
   for (i = 0; i < numMBsInVP; i++)
   {    
         if (!MBinstance)
         {
             retValue = H263D_ERROR;
             goto exitFunction;
         }

         /* MVE */
         VDT_SET_START_POSITION(MBinstance,3,inBuffer->getIndex,inBuffer->bitIndex); // 3: ac_pred_flag
         
         /* ac_pred_flag (1 bit) */
         code = (u_char) bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &error);
         if (error) 
             goto exitFunction;
         
         /* MVE */
         VDT_SET_END_POSITION(MBinstance,3,inBuffer->getIndex,inBuffer->bitIndex); // 3: ac_pred_flag
         VDT_SET_START_POSITION(MBinstance,2,inBuffer->getIndex,inBuffer->bitIndex); //  2: cbpy
         
         /* CBPY */
         retValue = vdxGetCBPY(inBuffer, &cbpyIndex, bitErrorIndication);
         if (retValue != VDX_OK)
             goto exitFunction;
         
         /* MVE */
         VDT_SET_END_POSITION(MBinstance,2,inBuffer->getIndex,inBuffer->bitIndex); // 2: cbpy
         
                                                                                                                                                             /* add the information to the MB data item.
                                                                                                                                                             If the Part1 of the VP was decoded without errors (all MBs were
         attached to the list), MBinstance should always have a valid value. */ 
         if (MBinstance != NULL) {
             MBinstance->ac_pred_flag = code;
             MBinstance->cbpy = cbpyIndex;
             
             dlstNext(MBList, (void **) &MBinstance);
         }
   }
     
exitFunction:
     
     
   /* If no error in bit buffer functions */
   if (!error)
      return retValue;

   /* Else if ran out of data (i.e. decoding out of sync) */
   else if (error == ERR_BIB_NOT_ENOUGH_DATA) {
      return VDX_OK_BUT_BIT_ERROR;
   }

   /* Else other error in bit buffer functions */
   else
      return VDX_ERR_BIB;
}

/*
 *
 * vdxGetDataPartitionedPMBLayer_Part1
 *
 * Parameters:
 *    inBuffer     input buffer
 *    inpParam     input parameters
 *    MBList       a double-linked list for soring 
 *                 MB parameters + DC values in the VP
 *    bitErrorIndication
 *                 non-zero if a bit error has been detected
 *                 within the bits accessed in this function,
 *                 see biterr.h for possible values
 *
 * Function:
 *    This function gets the first (Motion) partition of a data
 *    partitioned encoded Video Packet in an Inter-VOP. 
 *    The parameters COD, MCBPC and motion vectors of all the MBs
 *    in the VP are read and stored in the linked list.
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 * Error codes:
 *      error codes returned by bibFlushBits/bibGetBits/bibShowBits
 *
 *      
 *
 */

 int vdxGetDataPartitionedPMBLayer_Part1(
   bibBuffer_t *inBuffer,
   bibBuffer_t */*outBuffer*/, bibBufferEdit_t */*bufEdit*/, 
   int /*aColorEffect*/, int */*aStartByteIndex*/, int */*aStartBitIndex*/, 
   const vdxGetDataPartitionedPMBLayerInputParam_t *inpParam,
   dlst_t *MBList,
   int *bitErrorIndication)
 {
     
   static const int mbTypeToMBClass[6] = 
     {VDX_MB_INTER, VDX_MB_INTER, VDX_MB_INTER, 
     VDX_MB_INTRA, VDX_MB_INTRA, VDX_MB_INTER};
     
   int mvdx, mvdy,
         numMVs,
         mcbpcIndex,
         retValue = VDX_OK,
         bitsGot;
     
   int16 error=0;
   u_char code;
   vdxPMBListItem_t *MBinstance = NULL;
     
     /* MVE */
   int StartByteIndex = 0;
   int StartBitIndex = 0;
   int stuffingStartByteIndex, stuffingStartBitIndex, stuffingEndByteIndex, stuffingEndBitIndex;
   
   stuffingStartByteIndex = stuffingEndByteIndex = inBuffer->getIndex;
   stuffingStartBitIndex  = stuffingEndBitIndex = inBuffer->bitIndex;
     
   vdxAssert(inpParam != NULL);
   vdxAssert(inBuffer != NULL);
   vdxAssert(bitErrorIndication != NULL);
     
   while (bibShowBits(MP4_MOTION_MARKER_COMB_LENGTH, inBuffer, &bitsGot, bitErrorIndication, &error) 
         != MP4_MOTION_MARKER_COMB && !error)
   {
         /* COD */
         code = (u_char) bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &error);
         if (error) 
             goto exitFunction;
         
         if (code == 0)
         {
             
             /* MVE */
             StartByteIndex = inBuffer->getIndex; 
             StartBitIndex = inBuffer->bitIndex;
             
             /* MCBPC */
             retValue = vdxGetMCBPCInter(
                 inBuffer, 
                 0, /* No PLUSPTYPE present in MPEG-4 */
                 1, /* Four motion vectors always possible in MPEG-4 */
            0, /* Flag to indicate if this is the first MB of a picture.
                             Since this value is used only for checking if
                             indices 21 - 24 are allowed in H.263, we don't have
                             to set this correctly. (Indices 21 - 24 are not allowed
                             in MPEG-4 at all.) */
                             &mcbpcIndex, 
                             bitErrorIndication);
             
             if (retValue != VDX_OK)
                 goto exitFunction;
             
             if (mcbpcIndex == 20)
             {
                 /* MVE */
                 stuffingEndByteIndex = inBuffer->getIndex;
                 stuffingEndBitIndex = inBuffer->bitIndex;
                 
                 continue;  /* skip stuffing */
             }
             
             /* Indices > 20 not allowed */
             if (mcbpcIndex > 20) {
                 deb0p("vdxGetDataPartitionedPMBLayer_Part1: ERROR - Illegal code.\n");
                 retValue = VDX_OK_BUT_BIT_ERROR;              
                 goto exitFunction;
             }
             
             
         }
         
         /* Create new MBInstance for the MB */
         MBinstance = (vdxPMBListItem_t *) malloc(sizeof(vdxPMBListItem_t));
         if (!MBinstance)
         {
             deb("ERROR - MBinstance creation failed\n");
             goto exitFunction;
         }
         memset(MBinstance, 0, sizeof(vdxPMBListItem_t));
         
         MBinstance->fCodedMB = (u_char) (code ^ 1);      
         
         if (MBinstance->fCodedMB) 
         {                   
             /* MCBPC */
             MBinstance->cbpc = mcbpcIndex;
             
             /* MCBPC --> MB type & included data elements */
             MBinstance->mbType = mcbpcIndex / 4;
             MBinstance->mbClass = mbTypeToMBClass[MBinstance->mbType];
             
             /* MVE */
             MBinstance->mcbpc = mcbpcIndex;
             VDT_SET_START_POSITION(MBinstance,11,stuffingStartByteIndex,stuffingStartBitIndex); // 11: MB stuffing bits
             VDT_SET_END_POSITION(MBinstance,11,stuffingEndByteIndex,stuffingEndBitIndex); // 11: MB stuffing bits
             VDT_SET_START_POSITION(MBinstance,0,StartByteIndex,StartBitIndex);  // MCBPC
             VDT_SET_END_POSITION(MBinstance,0,inBuffer->getIndex,inBuffer->bitIndex);
             
             VDT_SET_START_POSITION(MBinstance,10,inBuffer->getIndex,inBuffer->bitIndex); // MVs
             
             /* MVD is included always for PB-frames and always if MB type is INTER */
             numMVs = MBinstance->numMVs = 
                 (MBinstance->mbClass == VDX_MB_INTER) ?
                 ((MBinstance->mbType == 2 || MBinstance->mbType == 5) ? 4 : 1) : 0;
             
             if (numMVs) {
                 int i;
                 for (i = 0; i < numMVs; i++) {
                     retValue = vdxGetScaledMVD(inBuffer,inpParam->f_code,&mvdx,bitErrorIndication);
                     if (retValue != VDX_OK)
                         goto exitFunction;
                     retValue = vdxGetScaledMVD(inBuffer,inpParam->f_code,&mvdy,bitErrorIndication);
                     if (retValue != VDX_OK)
                         goto exitFunction;
                     
                     MBinstance->mvx[i] = mvdx;
                     MBinstance->mvy[i] = mvdy;
                 }
             }
             
             /* MVE */
             VDT_SET_END_POSITION(MBinstance,10,inBuffer->getIndex,inBuffer->bitIndex); // MVs
             
         }
         
       /* MVE */
         // begin another MB, record the position of MB stuffing
         stuffingStartByteIndex = stuffingEndByteIndex = inBuffer->getIndex;
         stuffingStartBitIndex  = stuffingEndBitIndex = inBuffer->bitIndex;
         
         /* Put MBinstance into the queue */
         dlstAddAfterCurr(MBList, MBinstance);
         MBinstance = NULL;
   }
     
exitFunction:
     
   if (MBinstance)
         free(MBinstance);
   
     
   /* If no error in bit buffer functions */
   if (!error)
         return retValue;
     
   /* Else if ran out of data (i.e. decoding out of sync) */
   else if (error == ERR_BIB_NOT_ENOUGH_DATA) {
         return VDX_OK_BUT_BIT_ERROR;
   }
     
   /* Else other error in bit buffer functions */
   else
         return VDX_ERR_BIB;
     
}


/*
 *
 * vdxGetDataPartitionedPMBLayer_Part2
 *
 * Parameters:
 *    inBuffer     input buffer
 *    inpParam     input parameters
 *    MBList       a double-linked list for soring 
 *                 MB parameters + DC values in the VP
 *    bitErrorIndication
 *                 non-zero if a bit error has been detected
 *                 within the bits accessed in this function,
 *                 see biterr.h for possible values
 *
 * Function:
 *    This function gets the second (CBPY) partition of a data
 *    partitioned encoded Video Packet in an Inter-VOP. 
 *    The parameters CBPY, DQUANT and if Intra MB its DC coeffs
 *    of all the MBs in the VP are read and stored in the linked list.
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 * Error codes:
 *      error codes returned by bibFlushBits/bibGetBits/bibShowBits
 *
 *      
 *
 */

 int vdxGetDataPartitionedPMBLayer_Part2(
   bibBuffer_t *inBuffer,
   bibBuffer_t *outBuffer, 
   bibBufferEdit_t *bufEdit,
   int aColorEffect, int *aStartByteIndex, int *aStartBitIndex, 
   CMPEG4Transcoder *hTranscoder, 
   const vdxGetDataPartitionedPMBLayerInputParam_t *inpParam,
   dlst_t *MBList,
   int *bitErrorIndication)
 {
     
   static const int mbTypeToDQUANTI[6] =
     {0, 1, 0, 0, 1, 1};
     
   int fDQUANT,
         cbpyIndex,
         retValue = VDX_OK,
         new_quant, previous_quant,
         bitsGot,
         i, 
         IntraDC_size, 
         IntraDC_delta;
     
   int16 error=0;
   u_char code;
   vdxPMBListItem_t *MBinstance;
     
   vdxAssert(inpParam != NULL);
   vdxAssert(inBuffer != NULL);
   vdxAssert(bitErrorIndication != NULL);
     
   previous_quant = inpParam->quant;
     
   /* Get ac_pred_flag and cbpy of all MBs in VP */
   for (dlstHead(MBList, (void **) &MBinstance); 
     MBinstance != NULL; 
     dlstNext(MBList, (void **) &MBinstance))
   {
         
         if (!MBinstance->fCodedMB) continue;
         
         /* MVE */
         VDT_SET_START_POSITION(MBinstance,3,inBuffer->getIndex,inBuffer->bitIndex); // 3 ac_pred_flag
         
         if (MBinstance->mbClass == VDX_MB_INTRA) {
             
             /* ac_pred_flag (1 bit) */
             code = (u_char) bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &error);
             if (error) 
                 goto exitFunction;
             
             MBinstance->ac_pred_flag = code;
         }
         
         /* MVE */
         VDT_SET_END_POSITION(MBinstance,3,inBuffer->getIndex,inBuffer->bitIndex); // 3 ac_pred_flag
         VDT_SET_START_POSITION(MBinstance,2,inBuffer->getIndex,inBuffer->bitIndex); // 2: cbpy
         
         /* CBPY */
         retValue = vdxGetCBPY(inBuffer, &cbpyIndex, bitErrorIndication);
         if (retValue != VDX_OK)
             goto exitFunction;
         
         if (MBinstance->mbClass == VDX_MB_INTER)
             /* Convert index to INTER CBPY */
             cbpyIndex = 15 - cbpyIndex;
         
         MBinstance->cbpy = cbpyIndex;
         
         /* MVE */
         VDT_SET_END_POSITION(MBinstance,2,inBuffer->getIndex,inBuffer->bitIndex); // 
         VDT_SET_START_POSITION(MBinstance,1,inBuffer->getIndex,inBuffer->bitIndex); // 1: dquant
         
         /* DQUANT is given for MCBPC indexes 4..7 */
         fDQUANT = mbTypeToDQUANTI[MBinstance->mbType];
         
         if (fDQUANT) {
             retValue = vdxUpdateQuant(inBuffer, 0, previous_quant,
                 &new_quant, bitErrorIndication);
             if (retValue != VDX_OK)
                 goto exitFunction;
         }
         /* Else no DQUANT */
         else
             new_quant = previous_quant;
         
         MBinstance->dquant       = fDQUANT ?  new_quant - previous_quant : 0;
         MBinstance->quant = previous_quant = new_quant;
         
         /* MVE */
         VDT_SET_END_POSITION(MBinstance,1,inBuffer->getIndex,inBuffer->bitIndex); // 1: dquant
         VDT_SET_START_POSITION(MBinstance,4,inBuffer->getIndex,inBuffer->bitIndex); // 4: intraDC
         VDT_SET_START_POSITION(MBinstance,5,inBuffer->getIndex,inBuffer->bitIndex); // 5: intraDC
         VDT_SET_START_POSITION(MBinstance,6,inBuffer->getIndex,inBuffer->bitIndex); // 6: intraDC
         VDT_SET_START_POSITION(MBinstance,7,inBuffer->getIndex,inBuffer->bitIndex); // 7: intraDC
         VDT_SET_START_POSITION(MBinstance,8,inBuffer->getIndex,inBuffer->bitIndex); // 8: intraDC
         VDT_SET_START_POSITION(MBinstance,9,inBuffer->getIndex,inBuffer->bitIndex); // 9: intraDC
         
         VDT_SET_END_POSITION(MBinstance,4,inBuffer->getIndex,inBuffer->bitIndex); // 4: intraDC
         VDT_SET_END_POSITION(MBinstance,5,inBuffer->getIndex,inBuffer->bitIndex); // 5: intraDC
         VDT_SET_END_POSITION(MBinstance,6,inBuffer->getIndex,inBuffer->bitIndex); // 6: intraDC
         VDT_SET_END_POSITION(MBinstance,7,inBuffer->getIndex,inBuffer->bitIndex); // 7: intraDC
         VDT_SET_END_POSITION(MBinstance,8,inBuffer->getIndex,inBuffer->bitIndex); // 8: intraDC
         VDT_SET_END_POSITION(MBinstance,9,inBuffer->getIndex,inBuffer->bitIndex); // 9: intraDC
         
     /* Color Toning */
     hTranscoder->AfterMBLayer(new_quant);
     
         if (MBinstance->mbClass == VDX_MB_INTRA) {
             MBinstance->switched = aicIntraDCSwitch(inpParam->intra_dc_vlc_thr,new_quant);
             
             /* Intra_DC_Coeffs */
             if(!MBinstance->switched) {
                 for (i=0; i<6; i++) {
                     
                     /* MVE */
                     VDT_SET_START_POSITION(MBinstance,i+4,inBuffer->getIndex,inBuffer->bitIndex); // 6: intraDC,chrominance
                     
                     retValue = vdxGetIntraDC(inBuffer, outBuffer, bufEdit, aColorEffect, aStartByteIndex, aStartBitIndex, 
                         i, &IntraDC_size, &IntraDC_delta, bitErrorIndication);
                     
                     /* MVE */
                     VDT_SET_END_POSITION(MBinstance,i+4,inBuffer->getIndex,inBuffer->bitIndex); // 6: intraDC,chrominance
                     
                     if (retValue != VDX_OK)
                         goto exitFunction;
                     MBinstance->DC[i] = IntraDC_delta;
                 } 
             }
         }
   }
     
exitFunction:
   
     
   /* If no error in bit buffer functions */
   if (!error)
         return retValue;
     
   /* Else if ran out of data (i.e. decoding out of sync) */
   else if (error == ERR_BIB_NOT_ENOUGH_DATA) {
         return VDX_OK_BUT_BIT_ERROR;
   }
     
   /* Else other error in bit buffer functions */
   else
         return VDX_ERR_BIB;
     
}

/*
 * Block Layer Global Functions
 */

/* {{-output"vdxGetMPEGIntraDCTBlock.txt"}} */
/*
 * vdxGetMPEGIntraDCTBlock
 *    
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *    startIndex                 the first index in block where to put data
 *    block                      DCT coefficients of the block 
 *                               in zigzag order 
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 *
 * Function:
 *    This function gets the DCT coefficients for one INTRA block.
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 *    
 *    
 */

int vdxGetMPEGIntraDCTBlock(
   bibBuffer_t *inBuffer, 
   int startIndex,
   int *block,
   int *bitErrorIndication)
/* {{-output"vdxGetMPEGIntraDCTBlock.txt"}} */
{
   int
      numBitsGot,
      retValue = VDX_OK,
      tmpvar;
   int16
      bibError = 0;

   static const vdxVLCTable_t Intra_tcoefTab0[] = {
    {0x10401, 7}, {0x10301, 7}, {0x00601, 7}, {0x10501, 7},
    {0x00701, 7}, {0x00202, 7}, {0x00103, 7}, {0x00009, 7},
    {0x10002, 6}, {0x10002, 6}, {0x00501, 6}, {0x00501, 6}, 
    {0x10201, 6}, {0x10201, 6}, {0x10101, 6}, {0x10101, 6},
    {0x00401, 6}, {0x00401, 6}, {0x00301, 6}, {0x00301, 6},
    {0x00008, 6}, {0x00008, 6}, {0x00007, 6}, {0x00007, 6}, 
    {0x00102, 6}, {0x00102, 6}, {0x00006, 6}, {0x00006, 6},
    {0x00201, 5}, {0x00201, 5}, {0x00201, 5}, {0x00201, 5},
    {0x00005, 5}, {0x00005, 5}, {0x00005, 5}, {0x00005, 5}, 
    {0x00004, 5}, {0x00004, 5}, {0x00004, 5}, {0x00004, 5}, 
    {0x10001, 4}, {0x10001, 4}, {0x10001, 4}, {0x10001, 4},
    {0x10001, 4}, {0x10001, 4}, {0x10001, 4}, {0x10001, 4},
    {0x00001, 2}, {0x00001, 2}, {0x00001, 2}, {0x00001, 2},
    {0x00001, 2}, {0x00001, 2}, {0x00001, 2}, {0x00001, 2},
    {0x00001, 2}, {0x00001, 2}, {0x00001, 2}, {0x00001, 2}, 
    {0x00001, 2}, {0x00001, 2}, {0x00001, 2}, {0x00001, 2},
    {0x00001, 2}, {0x00001, 2}, {0x00001, 2}, {0x00001, 2},
    {0x00001, 2}, {0x00001, 2}, {0x00001, 2}, {0x00001, 2}, 
    {0x00001, 2}, {0x00001, 2}, {0x00001, 2}, {0x00001, 2},
    {0x00001, 2}, {0x00001, 2}, {0x00001, 2}, {0x00001, 2},
    {0x00002, 3}, {0x00002, 3}, {0x00002, 3}, {0x00002, 3}, 
    {0x00002, 3}, {0x00002, 3}, {0x00002, 3}, {0x00002, 3},
    {0x00002, 3}, {0x00002, 3}, {0x00002, 3}, {0x00002, 3},
    {0x00002, 3}, {0x00002, 3}, {0x00002, 3}, {0x00002, 3}, 
    {0x00101, 4}, {0x00101, 4}, {0x00101, 4}, {0x00101, 4},
    {0x00101, 4}, {0x00101, 4}, {0x00101, 4}, {0x00101, 4},
    {0x00003, 4}, {0x00003, 4}, {0x00003, 4}, {0x00003, 4},
    {0x00003, 4}, {0x00003, 4}, {0x00003, 4}, {0x00003, 4},
   };


   static const vdxVLCTable_t Intra_tcoefTab1[] = {
    {0x00012,10}, {0x00011,10}, {0x10e01, 9}, {0x10e01, 9},
    {0x10d01, 9}, {0x10d01, 9}, {0x10c01, 9}, {0x10c01, 9},
    {0x10b01, 9}, {0x10b01, 9}, {0x10a01, 9}, {0x10a01, 9}, 
    {0x10102, 9}, {0x10102, 9}, {0x10004, 9}, {0x10004, 9},
    {0x00c01, 9}, {0x00c01, 9}, {0x00b01, 9}, {0x00b01, 9},
    {0x00702, 9}, {0x00702, 9}, {0x00602, 9}, {0x00602, 9}, 
    {0x00502, 9}, {0x00502, 9}, {0x00303, 9}, {0x00303, 9},
    {0x00203, 9}, {0x00203, 9}, {0x00106, 9}, {0x00106, 9},
    {0x00105, 9}, {0x00105, 9}, {0x00010, 9}, {0x00010, 9}, 
    {0x00402, 9}, {0x00402, 9}, {0x0000f, 9}, {0x0000f, 9},
    {0x0000e, 9}, {0x0000e, 9}, {0x0000d, 9}, {0x0000d, 9},
    {0x10801, 8}, {0x10801, 8}, {0x10801, 8}, {0x10801, 8}, 
    {0x10701, 8}, {0x10701, 8}, {0x10701, 8}, {0x10701, 8},
    {0x10601, 8}, {0x10601, 8}, {0x10601, 8}, {0x10601, 8},
    {0x10003, 8}, {0x10003, 8}, {0x10003, 8}, {0x10003, 8},  
    {0x00a01, 8}, {0x00a01, 8}, {0x00a01, 8}, {0x00a01, 8},
    {0x00901, 8}, {0x00901, 8}, {0x00901, 8}, {0x00901, 8},
    {0x00801, 8}, {0x00801, 8}, {0x00801, 8}, {0x00801, 8},  
    {0x10901, 8}, {0x10901, 8}, {0x10901, 8}, {0x10901, 8},
    {0x00302, 8}, {0x00302, 8}, {0x00302, 8}, {0x00302, 8},
    {0x00104, 8}, {0x00104, 8}, {0x00104, 8}, {0x00104, 8},  
    {0x0000c, 8}, {0x0000c, 8}, {0x0000c, 8}, {0x0000c, 8},
    {0x0000b, 8}, {0x0000b, 8}, {0x0000b, 8}, {0x0000b, 8},
    {0x0000a, 8}, {0x0000a, 8}, {0x0000a, 8}, {0x0000a, 8}, 
   };

   static const vdxVLCTable_t Intra_tcoefTab2[] = {
    {0x10007,11}, {0x10007,11}, {0x10006,11}, {0x10006,11},
    {0x00016,11}, {0x00016,11}, {0x00015,11}, {0x00015,11},
    {0x10202,10}, {0x10202,10}, {0x10202,10}, {0x10202,10},  
    {0x10103,10}, {0x10103,10}, {0x10103,10}, {0x10103,10},
    {0x10005,10}, {0x10005,10}, {0x10005,10}, {0x10005,10},
    {0x00d01,10}, {0x00d01,10}, {0x00d01,10}, {0x00d01,10},  
    {0x00503,10}, {0x00503,10}, {0x00503,10}, {0x00503,10},
    {0x00802,10}, {0x00802,10}, {0x00802,10}, {0x00802,10},
    {0x00403,10}, {0x00403,10}, {0x00403,10}, {0x00403,10},  
    {0x00304,10}, {0x00304,10}, {0x00304,10}, {0x00304,10},
    {0x00204,10}, {0x00204,10}, {0x00204,10}, {0x00204,10},
    {0x00107,10}, {0x00107,10}, {0x00107,10}, {0x00107,10}, 
    {0x00014,10}, {0x00014,10}, {0x00014,10}, {0x00014,10},
    {0x00013,10}, {0x00013,10}, {0x00013,10}, {0x00013,10},
    {0x00017,11}, {0x00017,11}, {0x00018,11}, {0x00018,11}, 
    {0x00108,11}, {0x00108,11}, {0x00902,11}, {0x00902,11},
    {0x10302,11}, {0x10302,11}, {0x10402,11}, {0x10402,11},
    {0x10f01,11}, {0x10f01,11}, {0x11001,11}, {0x11001,11}, 
    {0x00019,12}, {0x0001a,12}, {0x0001b,12}, {0x00109,12},
    {0x00603,12}, {0x0010a,12}, {0x00205,12}, {0x00703,12},
    {0x00e01,12}, {0x10008,12}, {0x10502,12}, {0x10602,12}, 
    {0x11101,12}, {0x11201,12}, {0x11301,12}, {0x11401,12},
    {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7},
    {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7},
    {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7},
    {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7},
    {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7},
    {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7},
    {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7},
    {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7},
   };

   static const int intra_max_level[2][64] = {
                                     {27, 10,  5,  4,  3,  3,  3,  3,
                                       2,  2,  1,  1,  1,  1,  1,  0,
                                       0,  0,  0,  0,  0,  0,  0,  0,
                                       0,  0,  0,  0,  0,  0,  0,  0,
                                       0,  0,  0,  0,  0,  0,  0,  0,
                                       0,  0,  0,  0,  0,  0,  0,  0,
                                       0,  0,  0,  0,  0,  0,  0,  0,
                                       0,  0,  0,  0,  0,  0,  0,  0,
                                     },
 
                                      {8,  3,  2,  2,  2,  2,  2,  1,
                                       1,  1,  1,  1,  1,  1,  1,  1,
                                       1,  1,  1,  1,  1,  0,  0,  0,
                                       0,  0,  0,  0,  0,  0,  0,  0,
                                       0,  0,  0,  0,  0,  0,  0,  0,
                                       0,  0,  0,  0,  0,  0,  0,  0,
                                       0,  0,  0,  0,  0,  0,  0,  0,
                                       0,  0,  0,  0,  0,  0,  0,  0
                                    }
                                   };

   static const int intra_max_run0[28] = { 999, 14,  9,  7,  3,  2,  1,
                                    1,  1,  1,  1,  0,  0,  0,
                                    0,  0,  0,  0,  0,  0,  0,
                                    0,  0,  0,  0,  0,  0,  0
                                };
 
   static const int intra_max_run1[9] = { 999, 20,  6,
                                   1,  0,  0,
                                   0,  0,  0
                               };

   int
      code,       /* bits got from bit buffer */
      index,      /* index to zigzag table running from 1 to 63 */
      run,        /* RUN code */
      level;      /* LEVEL code */

   u_int32
     last,       /* LAST code (see standard) */
      sign;       /* sign for level */

   vdxVLCTable_t const *tab; /* pointer to lookup table */

   vdxAssert(inBuffer != NULL);
   vdxAssert(startIndex == 0 || startIndex == 1);
   vdxAssert(block != NULL);
   vdxAssert(bitErrorIndication != NULL);

   index = startIndex;

   do {
      code = (int) bibShowBits(12, inBuffer, &numBitsGot, bitErrorIndication,
         &bibError);
      if (bibError == ERR_BIB_NOT_ENOUGH_DATA) {
         deb("vdxGetDCTBlock: bibShowBits returned not enough data --> "
            "try to use the data available.\n");
         code <<= 12 - numBitsGot;
         bibError = 0;
      }
      else if (bibError ) {
         goto exitFunction;
      }

      /* Select the right table and index for the codeword */
      if (code >= 512)
         tab = &Intra_tcoefTab0[(code >> 5) - 16];
      else if (code >= 128)
         tab = &Intra_tcoefTab1[(code >> 2) - 32];
      else if (code >= 8)
         tab = &Intra_tcoefTab2[code - 8];
      else {
         deb("ERROR - illegal TCOEF\n");
         retValue = VDX_OK_BUT_BIT_ERROR;
         goto exitFunction;
      }

      /* Flush the codeword from the buffer */
      bibFlushBits(tab->len, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
      if (bibError)
        goto exitFunction;
    
      /* the following is modified for 3-mode escape */
      if (tab->val == 7167)     /* ESCAPE */
      {
      
         int run_offset=0,
         level_offset=0;

         code = (int) bibShowBits(2, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         if (bibError) {
            goto exitFunction;
         }

         if (code<=2) {

            /* escape modes: level or run is offset */
            if (code==2) run_offset=1;
            else level_offset=1;

            /* Flush the escape code from the buffer */
            if (run_offset)
               bibFlushBits(2, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
            else
               bibFlushBits(1, inBuffer, &numBitsGot, bitErrorIndication, &bibError);

            if (bibError)
               goto exitFunction;
            /* Read next codeword */
            code = (int) bibShowBits(12, inBuffer, &numBitsGot, bitErrorIndication,
                     &bibError);
            if (bibError == ERR_BIB_NOT_ENOUGH_DATA) {
               deb("vdxGetDCTBlock: bibShowBits returned not enough data --> "
                  "try to use the data available.\n");
               code <<= 12 - numBitsGot;
               bibError = 0;
            }
            else if (bibError) {
               goto exitFunction;
            }

            /* Select the right table and index for the codeword */
            if (code >= 512)
               tab = &Intra_tcoefTab0[(code >> 5) - 16];
            else if (code >= 128)
               tab = &Intra_tcoefTab1[(code >> 2) - 32];
            else if (code >= 8)
               tab = &Intra_tcoefTab2[code - 8];
            else {
               deb("ERROR - illegal TCOEF\n");
               retValue = VDX_OK_BUT_BIT_ERROR;
               goto exitFunction;
            }

            bibFlushBits(tab->len, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
            if (bibError)
               goto exitFunction;

            run = (tab->val >> 8) & 255;
            level = tab->val & 255;
            last = (tab->val >> 16) & 1;

            /* need to add back the max level */
            if (level_offset)
               level = level + intra_max_level[last][run];
            else if (last)
               run = run + intra_max_run1[level]+1;
            else
               run = run + intra_max_run0[level]+1;

            sign = bibGetBits(1, inBuffer, &numBitsGot, bitErrorIndication,
                     &bibError);

            if ( bibError )
               goto exitFunction;
        
            if (sign)
               level = -level;

         } else {
       
            /* Flush the codeword from the buffer */
            bibFlushBits(2, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
            if (bibError)
               goto exitFunction;

            /* LAST */
            last = bibGetBits(1, inBuffer, &numBitsGot, bitErrorIndication, 
                     &bibError);
            if ( bibError )
               goto exitFunction;
            /* RUN */
            run = (int) bibGetBits(6, inBuffer, &numBitsGot, bitErrorIndication, 
                     &bibError);
            if ( bibError )
               goto exitFunction;
            /* MARKER BIT */
            tmpvar = bibGetBits(1, inBuffer, &numBitsGot, bitErrorIndication,&bibError);
            if ( bibError )
               goto exitFunction;
            if (!tmpvar) {

               retValue = VDX_OK_BUT_BIT_ERROR;
               goto exitFunction;
            }
            /* LEVEL */
            level = (int) bibGetBits(12, inBuffer, &numBitsGot, bitErrorIndication, 
                        &bibError);
            if ( bibError )
               goto exitFunction;
            /* MARKER BIT */
            tmpvar = bibGetBits(1, inBuffer, &numBitsGot, bitErrorIndication,&bibError);
            if ( bibError )
               goto exitFunction;
            if(!tmpvar) {

               retValue = VDX_OK_BUT_BIT_ERROR;
               goto exitFunction;
            }

            /* 0000 0000 0000 and 1000 0000 0000 is forbidden unless in MQ mode */
            if (level == 0 || level == 2048) {
               deb("ERROR - illegal level.\n");
               retValue = VDX_OK_BUT_BIT_ERROR;
               goto exitFunction;
            }

            /* Codes 1000 0000 0001 .. 1111 1111 1111 */
            if (level > 2048)
               level -= 4096;
      
         } /* flc */
      }
      else {
   
         run = (tab->val >> 8) & 255;
         level = tab->val & 255;
         last = (tab->val >> 16) & 1;

         sign = bibGetBits(1, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         if ( bibError )
            goto exitFunction;

         if (sign)
            level = -level;
      }

      /* If too many coefficients */
      if (index + run > 63) {
         deb("ERROR - too many TCOEFs.\n");
         retValue = VDX_OK_BUT_BIT_ERROR;
         goto exitFunction;
      }

      /* Do run-length decoding */
         while (run--)
            block[index++] = 0;

         block[index++] = level;

   } while (!last);

   exitFunction:

   /* Set the rest of the coefficients to zero */
   while (index <= 63) {
      block[index++] = 0;
   }

   if (!bibError)
      return retValue;

   else if (bibError == ERR_BIB_NOT_ENOUGH_DATA) {
      return VDX_OK_BUT_BIT_ERROR;
   }

   else
      return VDX_ERR_BIB;
}

/*
 * Macroblock Layer Local Functions
 */

/*
 *
 * vdxGetScaledMVD
 *    
 *
 * Parameters:
 *    inBuffer     input buffer
 *    f_code       f_code for current Vop
 *    error        error code
 *    *mvd10       returned MV value (10x)
 *    *bitErrorIndication
 *
 * Function:
 *      Calculates a component of a block (or MB) vector by decoding 
 *      the magnitude & residual of the diff. vector, making the prediction, 
 *      and combining the decoded diff. and the predicted values
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 *    
 */


int vdxGetScaledMVD(bibBuffer_t *inBuffer, int f_code,
            int *mvd10, int *bitErrorIndication)
{

   int   residual=0, vlc_code_mag=0; 
   int   diff_vector,
         retValue = VDX_OK,
         numBitsGot;
   int16 bibError = 0;

   /* decode component */
   retValue = vdxGetMVD(inBuffer,&vlc_code_mag,bitErrorIndication);
   if (retValue < 0)
      goto exitFunction;

   if ((f_code > 1) && (vlc_code_mag != 0))
   {
      vlc_code_mag /= 5;

      residual = (int) 
         bibGetBits(f_code-1, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
      if (bibError)
         goto exitFunction;

      diff_vector = ((abs(vlc_code_mag)-1)<<(f_code-1)) + residual + 1;
      if (vlc_code_mag < 0)
         diff_vector = -diff_vector;

      *mvd10 = diff_vector * 5;
   } else 
      *mvd10 = vlc_code_mag;

   exitFunction:
   
   if (bibError) {
      if (bibError == ERR_BIB_NOT_ENOUGH_DATA) {
         return VDX_OK_BUT_BIT_ERROR;
      }
      return VDX_ERR_BIB;
   }

   return retValue;
}

/*
 * vdxGetIntraDC
 *    
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *    compnum                    number of the block component in the
 *                       4:2:2 YUV scheme 
 *                       (0..3) luma, (4..5) chroma
 *    IntraDCDelta               the read Intra DC value (quantized)
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 *
 * Function:
 *    This function gets the Intra DC value
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 *    
 *
 */

int vdxGetIntraDC(bibBuffer_t *inBuffer, bibBuffer_t */*outBuffer*/, bibBufferEdit_t */*bufEdit*/, int /*aColorEffect*/, 
                                    int */*aStartByteIndex*/, int */*aStartBitIndex*/, int compnum, int *IntraDC_size, int *IntraDCDelta, 
                                    int *bitErrorIndication)
{
   u_int32 code;
   int16 bibError=0;
   int first_bit,
      numBitsGot,
      retValue,
      IntraDCSize=0,
      tmpvar;
  
   vdxAssert(inBuffer != NULL);
   vdxAssert(IntraDCDelta != NULL);
   vdxAssert(bitErrorIndication != NULL);

   /* read DC size 2 - 8 bits */
   retValue =  vdxGetIntraDCSize(inBuffer, compnum, &IntraDCSize, bitErrorIndication);
   if (retValue != VDX_OK)
      return retValue;
  
   *IntraDC_size = IntraDCSize;
   if (IntraDCSize == 0) {

      *IntraDCDelta = 0;

   } else {

      /* read delta DC 0 - 8 bits */
      code = bibGetBits(IntraDCSize,inBuffer,&numBitsGot,bitErrorIndication, &bibError);

      if (bibError) {
         if (bibError == ERR_BIB_NOT_ENOUGH_DATA) {
            return VDX_OK_BUT_BIT_ERROR;
         }
         return VDX_ERR_BIB;
      } 

      first_bit = code >> (IntraDCSize-1);

      if (first_bit == 0 )
      { /* negative delta INTRA DC */
         *IntraDCDelta = -1 * (int) (code ^ ((1 << IntraDCSize)-1));
      }
      else
      { /* positive delta INTRA DC */
         *IntraDCDelta = (int) code;
      }

      if (IntraDCSize > 8) {
         /* Marker Bit */
         tmpvar = bibGetBits(1,inBuffer,&numBitsGot,bitErrorIndication, &bibError);
         if( !tmpvar ) {
            return VDX_OK_BUT_BIT_ERROR;
         }
      }
   }
                        
   return VDX_OK;
}

/*
 * vdxGetIntraDCSize
 *    
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *    compnum                    number of the block component in the
 *                       4:2:2 YUV scheme 
 *                       (0..3) luma, (4..5) chroma
 *    IntraDCSize                Size of the following Intra DC FLC
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 *
 * Function:
 *    This function gets the IntraDCSize VLC for luma or chroma
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 *    
 *
 */

static int vdxGetIntraDCSize(bibBuffer_t *inBuffer, int compnum, int *IntraDCSize, 
   int *bitErrorIndication)
{
   u_int32  code;
   int numBitsGot,
      retValue = VDX_OK;
   int16
      bibError = 0;

   if( compnum >=0 && compnum < 4 ) /* luminance block */
   {
      code = bibShowBits(11, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
      if (bibError && (bibError != ERR_BIB_NOT_ENOUGH_DATA)) {
         return VDX_ERR_BIB;
      }

      if ((bibError == ERR_BIB_NOT_ENOUGH_DATA) && (numBitsGot != 0)) {
         code <<= (11-numBitsGot);
         bibError = 0;
      }

      if ( code == 1)
      {
         *IntraDCSize = 12;
         bibFlushBits(11, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         goto exitFunction;
      }
      
      code >>= 1;
      if ( code == 1)
      {
         *IntraDCSize = 11;
         bibFlushBits(10, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         goto exitFunction;
      }

      code >>= 1;
      if ( code == 1)
      {
         *IntraDCSize = 10;
         bibFlushBits(9, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         goto exitFunction;
      }

      code >>= 1;
      if ( code == 1)
      {
         *IntraDCSize = 9;
         bibFlushBits(8, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         goto exitFunction;
      }

      code >>= 1;
      if ( code == 1)
      {
         *IntraDCSize = 8;
         bibFlushBits(7, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         goto exitFunction;
      }
      code >>= 1;
      if ( code == 1)
      {
         *IntraDCSize = 7;
         bibFlushBits(6, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         goto exitFunction;
      }
      code >>= 1;
      if ( code == 1)
      {
         *IntraDCSize = 6;
         bibFlushBits(5, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         goto exitFunction;
      }
      code >>= 1;
      if ( code == 1)
      {
         *IntraDCSize = 5;
         bibFlushBits(4, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         goto exitFunction;
      }
      code >>= 1;
      if ( code == 1)
      {
         *IntraDCSize = 4;
         bibFlushBits(3, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         goto exitFunction;
      } else if (code == 2) {
         *IntraDCSize = 3;
         bibFlushBits(3, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         goto exitFunction;
      } else if (code ==3) {
         *IntraDCSize = 0;
         bibFlushBits(3, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         goto exitFunction;
      }
      code >>= 1;
      if ( code == 2)
      {
         *IntraDCSize = 2;
         bibFlushBits(2, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         goto exitFunction;
      } else if (code == 3) {
         *IntraDCSize = 1;
         bibFlushBits(2, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         goto exitFunction;
      }     

   }
   else /* chrominance block */
   {
      code = bibShowBits(12, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
      if (bibError && (bibError != ERR_BIB_NOT_ENOUGH_DATA)) {
         return VDX_ERR_BIB;
      }

      if ((bibError == ERR_BIB_NOT_ENOUGH_DATA) && (numBitsGot != 0)) {
         code <<= (12-numBitsGot);
         bibError = 0;
      }

      if ( code == 1)
      {
         *IntraDCSize = 12;
         bibFlushBits(12, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         goto exitFunction;
      }
      
      code >>= 1;
      if ( code == 1)
      {
         *IntraDCSize = 11;
         bibFlushBits(11, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         goto exitFunction;
      }

      code >>= 1;
      if ( code == 1)
      {
         *IntraDCSize = 10;
         bibFlushBits(10, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         goto exitFunction;
      }

      code >>= 1;
      if ( code == 1)
      {
         *IntraDCSize = 9;
         bibFlushBits(9, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         goto exitFunction;
      }

      code >>= 1;
      if ( code == 1)
      {
         *IntraDCSize = 8;
         bibFlushBits(8, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         goto exitFunction;
      }
      code >>= 1;
      if ( code == 1)
      {
         *IntraDCSize = 7;
         bibFlushBits(7, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         goto exitFunction;
      }
      code >>= 1;
      if ( code == 1)
      {
         *IntraDCSize = 6;
         bibFlushBits(6, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         goto exitFunction;
      }
      code >>= 1;
      if ( code == 1)
      {
         *IntraDCSize = 5;
         bibFlushBits(5, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         goto exitFunction;
      }
      code >>= 1;
      if ( code == 1)
      {
         *IntraDCSize = 4;
         bibFlushBits(4, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         goto exitFunction;
      } 
      code >>= 1;
      if ( code == 1)
      {
         *IntraDCSize = 3;
         bibFlushBits(3, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         goto exitFunction;
      } 
      code >>= 1;
      {
         *IntraDCSize = 3-code;
         bibFlushBits(2, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         goto exitFunction;
      } 
   }

exitFunction:

   /* If no error in bit buffer functions */
   if (!bibError)
      return retValue;

   /* Else if ran out of data (i.e. decoding out of sync) */
   else if (bibError == ERR_BIB_NOT_ENOUGH_DATA) {
      return VDX_OK_BUT_BIT_ERROR;
   }

   /* Else other error in bit buffer functions */
   else
      return VDX_ERR_BIB;
}

/*
 * vdxGetRVLCIndex
 *    
 *
 * Parameters:
 *   bits                   input: the bits read from the stream
 *   index                  output: the RVLC table index corresponding
 *                       to "bits"
 *   length              output: length of the codeword
 *   intra_luma             indicates an intra "1" or inter "0" Block
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 *
 * Function:
 *    This function finds the RVLC table index (LAST,RUN.LEVEL) and length
 *   of the code belonging to the input codeword.
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       no such codeword exists in the table
 *
 *
 */

int vdxGetRVLCIndex(
   u_int32 bits,
   u_int32 *index,
   int *length,
   int intra_luma,
   int *bitErrorIndication)
{

   /* The indexes in the RVLC tables are written in equal RUN groups with
      LEVEL increasing */
   
   /* Intra RVLC table */
   static const vdxVLCTable_t RvlcTcoefTab0[] = {
  /* [0] --> e.g.: RUN = 0; LEVEL = 1..27 */
      {1,3},   {2,3},   {3,4},   {4,5},   {5,6},   {6,6},   {7,7},   {8,8},
      {9,8},  {10,9},  {11,9}, {12,10}, {13,10}, {14,10}, {15,11}, {16,11},
    {17,11}, {18,12}, {19,12}, {20,13}, {21,13}, {22,12}, {23,13}, {24,14},
    {25,14}, {26,14}, {27,15},
  /* [27] */
    {257,4}, {258,5}, {259,7}, {260,8}, {261,8}, {262,9},{263,10},{264,11},
   {265,11},{266,12},{267,13},{268,14},{269,14},
  /* [40] */
    {513,5}, {514,7}, {515,9},{516,10},{517,11},{518,11},{519,13},{520,13},
   {521,13},{522,14},{523,14},
  /* [51] */   
    {769,5}, {770,8}, {771,9},{772,11},{773,12},{774,13},{775,14},{776,14},
   {777,15},
  /* [60] */   
   {1025,6},{1026,8},{1027,10},{1028,12},{1029,12},{1030,14},
  /* [66] */   
   {1281,6},{1282,9},{1283,11},{1284,12},{1285,14},{1286,14},
  /* [72] */     
   {1537,7},{1538,10},{1539,11},{1540,12},{1541,15},
  /* [77] */   
   {1793,7},{1794,10},{1795,11},{1796,13},{1797,15},
  /* [82] */
   {2049,8},{2050,10},{2051,13},{2052,14},
  /* [86] */   
   {2305,8},{2306,11},{2307,13},{2308,15},
  /* [90] */
   {2561,9},{2562,12},
  /* [92] */      
   {2817,10},{2818,13},
  /* [94] */      
   {3073,10},{3074,15},
  /* [96] */
   {3329,11},
   {3585,13},
   {3841,13},
   {4097,14},
   {4353,14},  
   {4609,14},
   {4865,15},

  /* [103] --> LAST = 1 */    
   {65537,4},{65538,8},{65539,11},{65540,13},{65541,14},
  /* [108] */     
   {65793,5},{65794,9},{65795,12},{65796,14},{65797,15},
  /* [113] */
   {66049,5},{66050,11},{66051,15},
  /* [116] */     
   {66305,6},{66306,12},
  /* [118] */
   {66561,6},{66562,12},
  /* [120] */
   {66817,6},{66818,13},
  /* [122] */
   {67073,6},{67074,13},
  /* [124] */
   {67329,7},{67330,13},
  /* [126] */
   {67585,7},{67586,13},
  /* [128] */
   {67841,7},{67842,13},
  /* [130] */
   {68097,7},{68098,14},
  /* [132] */
   {68353,7},{68354,14},
  /* [134] */
   {68609,8},{68610,14},
  /* [136] */
   {68865,8},{68866,15},
  /* [138] */
   {69121,8},
   {69377,9},
   {69633,9},
   {69889,9},
   {70145,9},
   {70401,9},
   {70657,9},
   {70913,10},
   {71169,10},
   {71425,10},
   {71681,10},
   {71937,10},
   {72193,11},
   {72449,11},
   {72705,11},
   {72961,12},
   {73217,12},
   {73473,12},
   {73729,12},
   {73985,12},
   {74241,12},
   {74497,12},
   {74753,13},
   {75009,13},
   {75265,14},
   {75521,14},
   {75777,14},
   {76033,15},
   {76289,15},
   {76545,15},
   {76801,15},

  /* [169] */
     {7167,4}   /* last entry: escape code */
  
};

   /* Inter RVLC table */
   static const vdxVLCTable_t RvlcTcoefTab1[] = {
  /* [0] */
      {1,3},   {2,4},   {3,5},   {4,7},   {5,8},   {6,8},   {7,9},  {8,10},
     {9,10}, {10,11}, {11,11}, {12,12}, {13,13}, {14,13}, {15,13}, {16,13},
    {17,14}, {18,14}, {19,15},
  /* [19] */
    {257,3}, {258,6}, {259,8}, {260,9},{261,10},{262,11},{263,12},{264,13},
   {265,14},{266,14},
  /* [29] */
    {513,4}, {514,7}, {515,9},{516,11},{517,12},{518,14},{519,14},
  /* [36] */
    {769,5}, {770,8},{771,10},{772,12},{773,13},{774,14},{775,15},
  /* [43] */
   {1025,5},{1026,8},{1027,11},{1028,13},{1029,15},
  /* [48] */      
   {1281,5},{1282,9},{1283,11},{1284,13},
  /* [52] */      
   {1537,6},{1538,10},{1539,12},{1540,14},
  /* [56] */
   {1793,6},{1794,10},{1795,12},{1796,15},
  /* [60] */
   {2049,6},{2050,10},{2051,13},
  /* [63] */      
   {2305,7},{2306,10},{2307,14},
  /* [66] */
   {2561,7},{2562,11},
  /* [68] */
    {2817,7},{2818,12},
  /* [70] */
    {3073,8},{3074,13},
  /* [72] */
   {3329,8},{3330,14},
  /* [74] */
    {3585,8},{3586,14},
  /* [76] */
    {3841,9},{3842,14},
  /* [78] */
    {4097,9},{4098,14},
  /* [80] */
   {4353,9},{4354,15},
  /* [82] */
   {4609,10},
   {4865,10},
   {5121,10},
   {5377,11},
   {5633,11},
   {5889,11},
   {6145,11},
   {6401,11},
   {6657,11},
   {6913,12},
   {7169,12},
   {7425,12},
   {7681,13},
   {7937,13},
   {8193,13},
   {8449,13},
   {8705,14},
   {8961,14},
   {9217,14},
   {9473,15},
   {9729,15},

  /* [103] --> LAST = 1 */
   {65537,4},{65538,8},{65539,11},{65540,13},{65541,14},
  /* [108] */
   {65793,5},{65794,9},{65795,12},{65796,14},{65797,15},
  /* [113] */
   {66049,5},{66050,11},{66051,15},
  /* [116] */     
   {66305,6},{66306,12},
  /* [118] */
   {66561,6},{66562,12},
  /* [120] */
   {66817,6},{66818,13},
  /* [122] */
   {67073,6},{67074,13},
  /* [124] */
   {67329,7},{67330,13},
  /* [126] */
   {67585,7},{67586,13},
  /* [128] */
   {67841,7},{67842,13},
  /* [130] */
   {68097,7},{68098,14},
  /* [132] */
   {68353,7},{68354,14},
  /* [134] */
   {68609,8},{68610,14},
  /* [136] */
   {68865,8},{68866,15},
  /* [138] */
   {69121,8},
   {69377,9},
   {69633,9},
   {69889,9},
   {70145,9},
   {70401,9},
   {70657,9},
  {70913,10},
  {71169,10},
  {71425,10},
  {71681,10},
  {71937,10},
  {72193,11},
  {72449,11},
  {72705,11},
  {72961,12},
  {73217,12},
  {73473,12},
  {73729,12},
  {73985,12},
  {74241,12},
  {74497,12},
  {74753,13},
  {75009,13},
  {75265,14},
  {75521,14},
  {75777,14},
  {76033,15},
  {76289,15},
  {76545,15},
  {76801,15},

  /* [169] */
   {7167,4}          /* last entry: escape code */
};

   vdxVLCTable_t const *tab; /* pointer to lookup table */

   vdxAssert(bitErrorIndication != NULL);

    switch(bits) {
 
    case 0x0:
      if (intra_luma)
        tab = &RvlcTcoefTab0[169];
      else
        tab = &RvlcTcoefTab1[169];
      break;
      
    case 0x1: 
      if (intra_luma)
        tab = &RvlcTcoefTab0[27];
      else
        tab = &RvlcTcoefTab1[1];
      break;
    
    case 0x4: 
      if (intra_luma)
        tab = &RvlcTcoefTab0[40];
      else
        tab = &RvlcTcoefTab1[2];
       break;
    
    case 0x5:
      if (intra_luma)
        tab = &RvlcTcoefTab0[51];
      else
        tab = &RvlcTcoefTab1[36];
      break;
      
    case 0x6:
      if (intra_luma)
        tab = &RvlcTcoefTab0[0];
      else
        tab = &RvlcTcoefTab1[0];
      break;
      
    case 0x7:
      if (intra_luma)
        tab = &RvlcTcoefTab0[1];
      else
        tab = &RvlcTcoefTab1[19];
      break;
      
    case 0x8:
      if (intra_luma)
        tab = &RvlcTcoefTab0[28];
      else
        tab = &RvlcTcoefTab1[43];
      break;
      
    case 0x9:
      if (intra_luma)
        tab = &RvlcTcoefTab0[3];
      else
        tab = &RvlcTcoefTab1[48];
      break;
      
    case 0xa:
      if (intra_luma)
        tab = &RvlcTcoefTab0[2];
      else
        tab = &RvlcTcoefTab1[29];
      break;
      
    case 0xb:
      if (intra_luma)
        tab = &RvlcTcoefTab0[103];
      else
        tab = &RvlcTcoefTab1[103];
      break;
      
    case 0xc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[60];
      else
        tab = &RvlcTcoefTab1[20];
      break;
      
    case 0xd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[66];
      else
        tab = &RvlcTcoefTab1[52];
      break;
      
    case 0x12:
      if (intra_luma)
        tab = &RvlcTcoefTab0[108];
      else
        tab = &RvlcTcoefTab1[108];
      break;
      
    case 0x13:
      if (intra_luma)
        tab = &RvlcTcoefTab0[113];
      else
        tab = &RvlcTcoefTab1[113];
      break;
      
    case 0x14:
      if (intra_luma)
        tab = &RvlcTcoefTab0[4];
      else
        tab = &RvlcTcoefTab1[56];
      break;
      
    case 0x15:
      if (intra_luma)
        tab = &RvlcTcoefTab0[5];
      else
        tab = &RvlcTcoefTab1[60];
      break;
      
    case 0x18:
      if (intra_luma)
        tab = &RvlcTcoefTab0[116];
      else
        tab = &RvlcTcoefTab1[116];
      break;
      
    case 0x19:
      if (intra_luma)
        tab = &RvlcTcoefTab0[118];
      else
        tab = &RvlcTcoefTab1[118];
      break;
      
    case 0x1c:
      if (intra_luma)
        tab = &RvlcTcoefTab0[72];
      else
        tab = &RvlcTcoefTab1[3];
      break;
      
    case 0x1d:
      if (intra_luma)
        tab = &RvlcTcoefTab0[77];
      else
        tab = &RvlcTcoefTab1[30];
      break;
      
    case 0x22:
      if (intra_luma)
        tab = &RvlcTcoefTab0[120];
      else
        tab = &RvlcTcoefTab1[120];
      break;
      
    case 0x23:
      if (intra_luma)
        tab = &RvlcTcoefTab0[122];
      else
        tab = &RvlcTcoefTab1[122];
      break;
      
    case 0x2c:
      if (intra_luma)
        tab = &RvlcTcoefTab0[41];
      else
        tab = &RvlcTcoefTab1[63];
      break;
      
    case 0x2d:
      if (intra_luma)
        tab = &RvlcTcoefTab0[29];
      else
        tab = &RvlcTcoefTab1[66];
      break;
      
    case 0x34:
      if (intra_luma)
        tab = &RvlcTcoefTab0[6];
      else
        tab = &RvlcTcoefTab1[68];
      break;
      
    case 0x35:
      if (intra_luma)
        tab = &RvlcTcoefTab0[124];
      else
        tab = &RvlcTcoefTab1[124];
      break;
      
    case 0x38:
      if (intra_luma)
        tab = &RvlcTcoefTab0[126];
      else
        tab = &RvlcTcoefTab1[126];
      break;
      
    case 0x39:
      if (intra_luma)
        tab = &RvlcTcoefTab0[128];
      else
        tab = &RvlcTcoefTab1[128];
      break;
      
    case 0x3c:
      if (intra_luma)
        tab = &RvlcTcoefTab0[82];
      else
        tab = &RvlcTcoefTab1[4];
      break;
      
    case 0x3d:
      if (intra_luma)
        tab = &RvlcTcoefTab0[86];
      else
        tab = &RvlcTcoefTab1[5];
      break;
      
    case 0x42:
      if (intra_luma)
        tab = &RvlcTcoefTab0[130];
      else
        tab = &RvlcTcoefTab1[130];
      break;
      
    case 0x43:
      if (intra_luma)
        tab = &RvlcTcoefTab0[132];
      else
        tab = &RvlcTcoefTab1[132];
      break;
      
    case 0x5c:
      if (intra_luma)
        tab = &RvlcTcoefTab0[52];
      else
        tab = &RvlcTcoefTab1[21];
      break;
      
    case 0x5d:
      if (intra_luma)
        tab = &RvlcTcoefTab0[61];
      else
        tab = &RvlcTcoefTab1[37];
      break;
      
    case 0x6c:
      if (intra_luma)
        tab = &RvlcTcoefTab0[30];
      else
        tab = &RvlcTcoefTab1[44];
      break;
      
    case 0x6d:
      if (intra_luma)
        tab = &RvlcTcoefTab0[31];
      else
        tab = &RvlcTcoefTab1[70];
      break;
      
    case 0x74:
      if (intra_luma)
        tab = &RvlcTcoefTab0[7];
      else
        tab = &RvlcTcoefTab1[72];
      break;
      
    case 0x75:
      if (intra_luma)
        tab = &RvlcTcoefTab0[8];
      else
        tab = &RvlcTcoefTab1[74];
      break;
      
    case 0x78:
      if (intra_luma)
        tab = &RvlcTcoefTab0[104];
      else
        tab = &RvlcTcoefTab1[104];
      break;
      
    case 0x79:
      if (intra_luma)
        tab = &RvlcTcoefTab0[134];
      else
        tab = &RvlcTcoefTab1[134];
      break;
      
    case 0x7c:
      if (intra_luma)
        tab = &RvlcTcoefTab0[90];
      else
        tab = &RvlcTcoefTab1[6];
      break;
      
    case 0x7d:
      if (intra_luma)
        tab = &RvlcTcoefTab0[67];
      else
        tab = &RvlcTcoefTab1[22];
      break;
      
    case 0x82:
      if (intra_luma)
        tab = &RvlcTcoefTab0[136];
      else
        tab = &RvlcTcoefTab1[136];
      break;
      
    case 0x83:
      if (intra_luma)
        tab = &RvlcTcoefTab0[138];
      else
        tab = &RvlcTcoefTab1[138];
      break;
      
    case 0xbc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[42];
      else
        tab = &RvlcTcoefTab1[31];
      break;
      
    case 0xbd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[53];
      else
        tab = &RvlcTcoefTab1[49];
      break;
      
    case 0xdc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[32];
      else
        tab = &RvlcTcoefTab1[76];
      break;
      
    case 0xdd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[9];
      else
        tab = &RvlcTcoefTab1[78];
      break;
      
    case 0xec:
      if (intra_luma)
        tab = &RvlcTcoefTab0[10];
      else
        tab = &RvlcTcoefTab1[80];
      break;
      
    case 0xed:
      if (intra_luma)
        tab = &RvlcTcoefTab0[109];
      else
        tab = &RvlcTcoefTab1[109];
      break;
      
    case 0xf4:
      if (intra_luma)
        tab = &RvlcTcoefTab0[139];
      else
        tab = &RvlcTcoefTab1[139];
      break;
      
    case 0xf5:
      if (intra_luma)
        tab = &RvlcTcoefTab0[140];
      else
        tab = &RvlcTcoefTab1[140];
      break;
      
    case 0xf8:
      if (intra_luma)
        tab = &RvlcTcoefTab0[141];
      else
        tab = &RvlcTcoefTab1[141];
      break;
      
    case 0xf9:
      if (intra_luma)
        tab = &RvlcTcoefTab0[142];
      else
        tab = &RvlcTcoefTab1[142];
      break;
      
    case 0xfc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[92];
      else
        tab = &RvlcTcoefTab1[7];
      break;
      
    case 0xfd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[94];
      else
        tab = &RvlcTcoefTab1[8];
      break;
      
    case 0x102:
      if (intra_luma)
        tab = &RvlcTcoefTab0[143];
      else
        tab = &RvlcTcoefTab1[143];
      break;
      
    case 0x103:
      if (intra_luma)
        tab = &RvlcTcoefTab0[144];
      else
        tab = &RvlcTcoefTab1[144];
      break;
      
    case 0x17c:
      if (intra_luma)
        tab = &RvlcTcoefTab0[73];
      else
        tab = &RvlcTcoefTab1[23];
      break;
      
    case 0x17d:
      if (intra_luma)
        tab = &RvlcTcoefTab0[78];
      else
        tab = &RvlcTcoefTab1[38];
      break;
      
    case 0x1bc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[83];
      else
        tab = &RvlcTcoefTab1[53];
      break;
      
    case 0x1bd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[62];
      else
        tab = &RvlcTcoefTab1[57];
      break;
      
    case 0x1dc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[43];
      else
        tab = &RvlcTcoefTab1[61];
      break;
      
    case 0x1dd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[33];
      else
        tab = &RvlcTcoefTab1[64];
      break;
      
    case 0x1ec:
      if (intra_luma)
        tab = &RvlcTcoefTab0[11];
      else
        tab = &RvlcTcoefTab1[82];
      break;
      
    case 0x1ed:
      if (intra_luma)
        tab = &RvlcTcoefTab0[12];
      else
        tab = &RvlcTcoefTab1[83];
      break;
      
    case 0x1f4:
      if (intra_luma)
        tab = &RvlcTcoefTab0[13];
      else
        tab = &RvlcTcoefTab1[84];
      break;
      
    case 0x1f5:
      if (intra_luma)
        tab = &RvlcTcoefTab0[145];
      else
        tab = &RvlcTcoefTab1[145];
      break;
      
    case 0x1f8:
      if (intra_luma)
        tab = &RvlcTcoefTab0[146];
      else
        tab = &RvlcTcoefTab1[146];
      break;
      
    case 0x1f9:
      if (intra_luma)
        tab = &RvlcTcoefTab0[147];
      else
        tab = &RvlcTcoefTab1[147];
      break;
      
    case 0x1fc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[96];
      else
        tab = &RvlcTcoefTab1[9];
      break;
      
    case 0x1fd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[87];
      else
        tab = &RvlcTcoefTab1[10];
      break;
      
    case 0x202:
      if (intra_luma)
        tab = &RvlcTcoefTab0[148];
      else
        tab = &RvlcTcoefTab1[148];
      break;
      
    case 0x203:
      if (intra_luma)
        tab = &RvlcTcoefTab0[149];
      else
        tab = &RvlcTcoefTab1[149];
      break;
      
    case 0x2fc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[68];
      else
        tab = &RvlcTcoefTab1[24];
      break;
      
    case 0x2fd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[74];
      else
        tab = &RvlcTcoefTab1[32];
      break;
      
    case 0x37c:
      if (intra_luma)
        tab = &RvlcTcoefTab0[79];
      else
        tab = &RvlcTcoefTab1[45];
      break;
      
    case 0x37d:
      if (intra_luma)
        tab = &RvlcTcoefTab0[54];
      else
        tab = &RvlcTcoefTab1[50];
      break;
      
    case 0x3bc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[44];
      else
        tab = &RvlcTcoefTab1[67];
      break;
      
    case 0x3bd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[45];
      else
        tab = &RvlcTcoefTab1[85];
      break;
      
    case 0x3dc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[34];
      else
        tab = &RvlcTcoefTab1[86];
      break;
      
    case 0x3dd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[35];
      else
        tab = &RvlcTcoefTab1[87];
      break;
      
    case 0x3ec:
      if (intra_luma)
        tab = &RvlcTcoefTab0[14];
      else
        tab = &RvlcTcoefTab1[88];
      break;
      
    case 0x3ed:
      if (intra_luma)
        tab = &RvlcTcoefTab0[15];
      else
        tab = &RvlcTcoefTab1[89];
      break;
      
    case 0x3f4:
      if (intra_luma)
        tab = &RvlcTcoefTab0[16];
      else
        tab = &RvlcTcoefTab1[90];
      break;
      
    case 0x3f5:
      if (intra_luma)
        tab = &RvlcTcoefTab0[105];
      else
        tab = &RvlcTcoefTab1[105];
      break;
      
    case 0x3f8:
      if (intra_luma)
        tab = &RvlcTcoefTab0[114];
      else
        tab = &RvlcTcoefTab1[114];
      break;
      
    case 0x3f9:
      if (intra_luma)
        tab = &RvlcTcoefTab0[150];
      else
        tab = &RvlcTcoefTab1[150];
      break;
      
    case 0x3fc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[91];
      else
        tab = &RvlcTcoefTab1[11];
      break;
      
    case 0x3fd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[63];
      else
        tab = &RvlcTcoefTab1[25];
      break;
      
    case 0x402:
      if (intra_luma)
        tab = &RvlcTcoefTab0[151];
      else
        tab = &RvlcTcoefTab1[151];
      break;
      
    case 0x403:
      if (intra_luma)
        tab = &RvlcTcoefTab0[152];
      else
        tab = &RvlcTcoefTab1[152];
      break;
      
    case 0x5fc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[69];
      else
        tab = &RvlcTcoefTab1[33];
      break;
      
    case 0x5fd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[75];
      else
        tab = &RvlcTcoefTab1[39];
      break;
      
    case 0x6fc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[55];
      else
        tab = &RvlcTcoefTab1[54];
      break;
      
    case 0x6fd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[64];
      else
        tab = &RvlcTcoefTab1[58];
      break;
      
    case 0x77c:
      if (intra_luma)
        tab = &RvlcTcoefTab0[36];
      else
        tab = &RvlcTcoefTab1[69];
      break;
      
    case 0x77d:
      if (intra_luma)
        tab = &RvlcTcoefTab0[17];
      else
        tab = &RvlcTcoefTab1[91];
      break;
      
    case 0x7bc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[18];
      else
        tab = &RvlcTcoefTab1[92];
      break;
      
    case 0x7bd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[21];
      else
        tab = &RvlcTcoefTab1[93];
      break;
      
    case 0x7dc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[110];
      else
        tab = &RvlcTcoefTab1[110];
      break;
      
    case 0x7dd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[117];
      else
        tab = &RvlcTcoefTab1[117];
      break;
      
    case 0x7ec:
      if (intra_luma)
        tab = &RvlcTcoefTab0[119];
      else
        tab = &RvlcTcoefTab1[119];
      break;
      
    case 0x7ed:
      if (intra_luma)
        tab = &RvlcTcoefTab0[153];
      else
        tab = &RvlcTcoefTab1[153];
      break;
      
    case 0x7f4:
      if (intra_luma)
        tab = &RvlcTcoefTab0[154];
      else
        tab = &RvlcTcoefTab1[154];
      break;
      
    case 0x7f5:
      if (intra_luma)
        tab = &RvlcTcoefTab0[155];
      else
        tab = &RvlcTcoefTab1[155];
      break;
      
    case 0x7f8:
      if (intra_luma)
        tab = &RvlcTcoefTab0[156];
      else
        tab = &RvlcTcoefTab1[156];
      break;
      
    case 0x7f9:
      if (intra_luma)
        tab = &RvlcTcoefTab0[157];
      else
        tab = &RvlcTcoefTab1[157];
      break;
      
    case 0x7fc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[97];
      else
        tab = &RvlcTcoefTab1[12];
      break;
      
    case 0x7fd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[98];
      else
        tab = &RvlcTcoefTab1[13];
      break;
      
    case 0x802:
      if (intra_luma)
        tab = &RvlcTcoefTab0[158];
      else
        tab = &RvlcTcoefTab1[158];
      break;
      
    case 0x803:
      if (intra_luma)
        tab = &RvlcTcoefTab0[159];
      else
        tab = &RvlcTcoefTab1[159];
      break;
      
    case 0xbfc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[93];
      else
        tab = &RvlcTcoefTab1[14];
      break;
      
    case 0xbfd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[84];
      else
        tab = &RvlcTcoefTab1[15];
      break;
      
    case 0xdfc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[88];
      else
        tab = &RvlcTcoefTab1[26];
      break;
      
    case 0xdfd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[80];
      else
        tab = &RvlcTcoefTab1[40];
      break;
      
    case 0xefc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[56];
      else
        tab = &RvlcTcoefTab1[46];
      break;
      
    case 0xefd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[46];
      else
        tab = &RvlcTcoefTab1[51];
      break;
      
    case 0xf7c:
      if (intra_luma)
        tab = &RvlcTcoefTab0[47];
      else
        tab = &RvlcTcoefTab1[62];
      break;
      
    case 0xf7d:
      if (intra_luma)
        tab = &RvlcTcoefTab0[48];
      else
        tab = &RvlcTcoefTab1[71];
      break;
      
    case 0xfbc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[37];
      else
        tab = &RvlcTcoefTab1[94];
      break;
      
    case 0xfbd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[19];
      else
        tab = &RvlcTcoefTab1[95];
      break;
      
    case 0xfdc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[20];
      else
        tab = &RvlcTcoefTab1[96];
      break;
      
    case 0xfdd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[22];
      else
        tab = &RvlcTcoefTab1[97];
      break;
      
    case 0xfec:
      if (intra_luma)
        tab = &RvlcTcoefTab0[106];
      else
        tab = &RvlcTcoefTab1[106];
      break;
      
    case 0xfed:
      if (intra_luma)
        tab = &RvlcTcoefTab0[121];
      else
        tab = &RvlcTcoefTab1[121];
      break;
      
    case 0xff4:
      if (intra_luma)
        tab = &RvlcTcoefTab0[123];
      else
        tab = &RvlcTcoefTab1[123];
      break;
      
    case 0xff5:
      if (intra_luma)
        tab = &RvlcTcoefTab0[125];
      else
        tab = &RvlcTcoefTab1[125];
      break;
      
    case 0xff8:
      if (intra_luma)
        tab = &RvlcTcoefTab0[127];
      else
        tab = &RvlcTcoefTab1[127];
      break;
      
    case 0xff9:
      if (intra_luma)
        tab = &RvlcTcoefTab0[129];
      else
        tab = &RvlcTcoefTab1[129];
      break;
      
    case 0xffc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[99];
      else
        tab = &RvlcTcoefTab1[16];
      break;
      
    case 0xffd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[100];
      else
        tab = &RvlcTcoefTab1[17];
      break;
      
    case 0x1002:
      if (intra_luma)
        tab = &RvlcTcoefTab0[160];
      else
        tab = &RvlcTcoefTab1[160];
      break;
      
    case 0x1003:
      if (intra_luma)
        tab = &RvlcTcoefTab0[161];
      else
        tab = &RvlcTcoefTab1[161];
      break;
      
    case 0x17fc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[101];
      else
        tab = &RvlcTcoefTab1[27];
      break;
      
    case 0x17fd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[85];
      else
        tab = &RvlcTcoefTab1[28];
      break;
      
    case 0x1bfc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[70];
      else
        tab = &RvlcTcoefTab1[34];
      break;
      
    case 0x1bfd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[65];
      else
        tab = &RvlcTcoefTab1[35];
      break;
      
    case 0x1dfc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[71];
      else
        tab = &RvlcTcoefTab1[41];
      break;
      
    case 0x1dfd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[57];
      else
        tab = &RvlcTcoefTab1[55];
      break;
      
    case 0x1efc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[58];
      else
        tab = &RvlcTcoefTab1[65];
      break;
      
    case 0x1efd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[49];
      else
        tab = &RvlcTcoefTab1[73];
      break;
      
    case 0x1f7c:
      if (intra_luma)
        tab = &RvlcTcoefTab0[50];
      else
        tab = &RvlcTcoefTab1[75];
      break;
      
    case 0x1f7d:
      if (intra_luma)
        tab = &RvlcTcoefTab0[38];
      else
        tab = &RvlcTcoefTab1[77];
      break;
      
    case 0x1fbc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[39];
      else
        tab = &RvlcTcoefTab1[79];
      break;
      
    case 0x1fbd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[23];
      else
        tab = &RvlcTcoefTab1[98];
      break;
      
    case 0x1fdc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[24];
      else
        tab = &RvlcTcoefTab1[99];
      break;
      
    case 0x1fdd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[25];
      else
        tab = &RvlcTcoefTab1[100];
      break;
      
    case 0x1fec:
      if (intra_luma)
        tab = &RvlcTcoefTab0[107];
      else
        tab = &RvlcTcoefTab1[107];
      break;
      
    case 0x1fed:
      if (intra_luma)
        tab = &RvlcTcoefTab0[111];
      else
        tab = &RvlcTcoefTab1[111];
      break;
      
    case 0x1ff4:
      if (intra_luma)
        tab = &RvlcTcoefTab0[131];
      else
        tab = &RvlcTcoefTab1[131];
      break;
      
    case 0x1ff5:
      if (intra_luma)
        tab = &RvlcTcoefTab0[133];
      else
        tab = &RvlcTcoefTab1[133];
      break;
      
    case 0x1ff8:
      if (intra_luma)
        tab = &RvlcTcoefTab0[135];
      else
        tab = &RvlcTcoefTab1[135];
      break;
      
    case 0x1ff9:
      if (intra_luma)
        tab = &RvlcTcoefTab0[162];
      else
        tab = &RvlcTcoefTab1[162];
      break;
      
    case 0x1ffc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[26];
      else
        tab = &RvlcTcoefTab1[18];
      break;
      
    case 0x1ffd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[59];
      else
        tab = &RvlcTcoefTab1[42];
      break;
      
    case 0x2002:
      if (intra_luma)
        tab = &RvlcTcoefTab0[163];
      else
        tab = &RvlcTcoefTab1[163];
      break;
      
    case 0x2003:
      if (intra_luma)
        tab = &RvlcTcoefTab0[164];
      else
        tab = &RvlcTcoefTab1[164];
      break;
      
    case 0x2ffc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[76];
      else
        tab = &RvlcTcoefTab1[47];
      break;
      
    case 0x2ffd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[81];
      else
        tab = &RvlcTcoefTab1[59];
      break;
      
    case 0x37fc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[89];
      else
        tab = &RvlcTcoefTab1[81];
      break;
      
    case 0x37fd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[95];
      else
        tab = &RvlcTcoefTab1[101];
      break;
      
    case 0x3bfc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[102];
      else
        tab = &RvlcTcoefTab1[102];
      break;
      
    case 0x3bfd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[112];
      else
        tab = &RvlcTcoefTab1[112];
      break;
      
    case 0x3dfc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[115];
      else
        tab = &RvlcTcoefTab1[115];
      break;
      
    case 0x3dfd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[137];
      else
        tab = &RvlcTcoefTab1[137];
      break;
      
    case 0x3efc:
      if (intra_luma)
        tab = &RvlcTcoefTab0[165];
      else
        tab = &RvlcTcoefTab1[165];
      break;
      
    case 0x3efd:
      if (intra_luma)
        tab = &RvlcTcoefTab0[166];
      else
        tab = &RvlcTcoefTab1[166];
      break;
      
    case 0x3f7c:
      if (intra_luma)
        tab = &RvlcTcoefTab0[167];
      else
        tab = &RvlcTcoefTab1[167];
      break;
      
    case 0x3f7d:
      if (intra_luma)
        tab = &RvlcTcoefTab0[168];
      else
        tab = &RvlcTcoefTab1[168];
      break;

    default:
      deb("ERROR - illegal RVLC TCOEF\n");
      return VDX_OK_BUT_BIT_ERROR;
    }

   *index = tab->val;
   *length = tab->len;
   
   return VDX_OK;
}

/*
 * vdxGetRVLCDCTBlock
 *    
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *    startIndex                 the first index in block where to put data
 *    block                      array for block (length 64)
 *   fIntraBlock            indicates an intra "1" or inter "0" Block
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 *
 * Function:
 *    This function reads a block from bit buffer using Huffman codes listed
 *    in RVLC TCOEF table. The place, where the block is read is given as a
 *    pointer parameter.
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 *   
 *
 */

int vdxGetRVLCDCTBlock(
   bibBuffer_t *inBuffer, 
   int startIndex,
   int fIntraBlock,
   int *block,
   int *bitErrorIndication)
{
    int numBitsGot,
      retValue = VDX_OK,
      index = startIndex,  /* index to zigzag table running from 1 to 63 */
      tmpvar;
   u_int32
      bits,
      RVLCIndex = 0,
      RVLCLength = 0;
   int16
      bibError = 0;

   int run,    /* RUN code */
      level;      /* LEVEL code */
   u_int32 
      last,       /* LAST code (see standard) */
      sign;       /* sign for level */

   vdxAssert(inBuffer != NULL);
   vdxAssert(startIndex == 0 || startIndex == 1);
   vdxAssert(block != NULL);
   vdxAssert(bitErrorIndication != NULL);

   do {

      /* Read next codeword */
      bits = (int) bibShowBits(15, inBuffer, &numBitsGot, bitErrorIndication,
         &bibError);
      if (bibError == ERR_BIB_NOT_ENOUGH_DATA) {
         deb("vdxGetRVLCDCTBlock: bibShowBits returned not enough data --> "
            "try to use the data available.\n");
         bits <<= (15 - numBitsGot);
         bibError = 0;
      }
      else if (bibError ) {
         deb("vdxGetRVLCDCTBlock: ERROR - bibShowBits failed.\n");
         goto exitFunction;
      }

      /* Identifying the codeword in the read bits */
      {
         int count, len = 1;
         u_int32 mask = 0x4000; /* mask  100000000000000   */
         
         if (bits & mask) {
            count = 1;
            for (len = 1; count > 0 && len < 15; len++) {
               mask = mask >> 1;
               if (bits & mask) 
                  count--;
            }
         } else {
            count = 2;
            for (len = 1; count > 0 && len < 15; len++) {
               mask = mask >> 1;
               if (!(bits & mask))
                  count--;
            }
         }

         if (len >= 15) {
            deb("vdxGetRVLCDCTBlock:ERROR - illegal RVLC codeword.\n");
            retValue = VDX_OK_BUT_BIT_ERROR;
            goto exitFunction;
         }

         bits = bits & 0x7fff;
         bits = bits >> (15 - (len + 1));
      }

      /* Get the RVLC table Index and length belonging to the codeword */
      if (vdxGetRVLCIndex(bits, &RVLCIndex, (int *) &RVLCLength, fIntraBlock, bitErrorIndication) != VDX_OK)
         goto exitFunction;

      /* Flush the codeword from the buffer */
      bibFlushBits(RVLCLength, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
      if (bibError)
         goto exitFunction;

      if (RVLCIndex == 7167)     /* ESCAPE */
      {  
         /* Flush the rest of the ESCAPE code from the buffer */
         bibFlushBits(1, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         if (bibError)
            goto exitFunction;

         /* LAST */
         last = bibGetBits(1, inBuffer, &numBitsGot, bitErrorIndication, 
            &bibError);
         if (bibError)
            goto exitFunction;
         /* RUN */
         run = (int) bibGetBits(6, inBuffer, &numBitsGot, bitErrorIndication, 
            &bibError);
         if (bibError)
            goto exitFunction;
         /* MARKER BIT */
         tmpvar = bibGetBits(1, inBuffer, &numBitsGot, bitErrorIndication,&bibError);
         if (bibError)
            goto exitFunction;
         if (!tmpvar) {
            deb("vdxGetRVLCDCTBlock:ERROR - Wrong marker bit.\n");
            retValue = VDX_OK_BUT_BIT_ERROR;
            goto exitFunction;
         }
         /* LEVEL */
         level = (int) bibGetBits(11, inBuffer, &numBitsGot, bitErrorIndication, 
            &bibError);
         if (bibError)
            goto exitFunction;
         if (level == 0) {
            deb("vdxGetRVLCDCTBlock:ERROR - Escape level invalid.\n");
            retValue = VDX_OK_BUT_BIT_ERROR;
            goto exitFunction;
         }
         /* MARKER BIT */
         tmpvar = bibGetBits(1, inBuffer, &numBitsGot, bitErrorIndication,&bibError);
         if (bibError)
            goto exitFunction;
         if (!tmpvar) {
            deb("vdxGetRVLCDCTBlock:ERROR - Wrong marker bit.\n");
            retValue = VDX_OK_BUT_BIT_ERROR;
            goto exitFunction;
         }
         /* SIGN */
         sign = bibGetBits(5, inBuffer, &numBitsGot, bitErrorIndication, 
            &bibError);
         if (bibError)
            goto exitFunction;

         if (sign == 1) {
            level = -level;
         } else if (sign != 0) {
            deb("vdxGetRVLCDCTBlock:ERROR - illegal sign.\n");
            retValue = VDX_OK_BUT_BIT_ERROR;
            goto exitFunction;
         }

      } else {

         last = (RVLCIndex >> 16) & 1;
         run = (RVLCIndex >> 8) & 255;
         level = RVLCIndex & 255;

         sign = bibGetBits(1, inBuffer, &numBitsGot, bitErrorIndication, 
            &bibError);
         if (bibError)
            goto exitFunction;

         if (sign)
            level = -level;
      }

      /* If too many coefficients */
      if (index + run > 63) {
         deb("vdxGetRVLCDCTBlock:ERROR - too many TCOEFs.\n");
         retValue = VDX_OK_BUT_BIT_ERROR;
         goto exitFunction;
      }

      /* Do run-length decoding */
      while (run--)
         block[index++] = 0;

      block[index++] = level;

   } while (!last);

   exitFunction:

   /* Set the rest of the coefficients to zero */
   while (index <= 63) {
      block[index++] = 0;
   }

   if (!bibError)
      return retValue;

   else if (bibError == ERR_BIB_NOT_ENOUGH_DATA) {
      return VDX_OK_BUT_BIT_ERROR;
   }

   else
      return VDX_ERR_BIB;
}

/*
 * vdxGetRVLCDCTBlockBackwards
 *    
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *    startIndex                 the first index in block where to put data
 *    fIntraBlock                indicates an intra "1" or inter "0" Block
 *    BitPosBeforeRVLC           bit position of inBuffer before the RVLC block,
 *                               indicates the point to stop decoding backwards
 *    block                      array for block (length 64)
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 *
 * Function:
 *   This function reads a block in backwards direction from the bit buffer
 *   using Huffman codes listed in RVLC TCOEF table. The bit position of the 
 *   buffer at return from the function is where the DCT data of the current 
 *   block starts.
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 *   
 *
 */

int vdxGetRVLCDCTBlockBackwards(
   bibBuffer_t *inBuffer, 
   int startIndex,
   int fIntraBlock,
//   u_int32 BitPosBeforeRVLC,
   int *block,
   int *bitErrorIndication)
{
    int numBitsGot,
      retValue = VDX_OK,
      index = 63; /* index to zigzag table running from 1 to 63 */
   u_int32
      bits,
      RVLCIndex = 0,
      RVLCLength = 0;
   int16
      bibError = 0;

   int run,    /* RUN code */
      level;      /* LEVEL code */
   u_int32 
      last,       /* LAST code (see standard) */
      sign,       /* sign for level */
       escape;

   vdxAssert(inBuffer != NULL);
   vdxAssert(startIndex == 0 || startIndex == 1);
   vdxAssert(block != NULL);
   vdxAssert(bitErrorIndication != NULL);

   do {
      /* SIGN */
      bibRewindBits(1, inBuffer, &bibError);
      if (bibError)
         goto exitFunction;
      sign = bibShowBits(1, inBuffer, &numBitsGot, bitErrorIndication, 
         &bibError);
      if (bibError) {
         goto exitFunction;
      }
      
      /* Read next codeword */
      bibRewindBits(15, inBuffer, &bibError);
      if (bibError)
         goto exitFunction;
      bits = (int) bibGetBits(15, inBuffer, &numBitsGot, bitErrorIndication,
         &bibError);
      if (bibError == ERR_BIB_NOT_ENOUGH_DATA) {
         deb("vdxGetRVLCDCTBlockBackwards: bibShowBits returned not enough data --> "
            "try to use the data available.\n");
         bits <<= (15 - numBitsGot);
         bibError = 0;
      }
      else if (bibError) {
         deb("vdxGetRVLCDCTBlockBackwards: ERROR - bibGetBits failed.\n");
         goto exitFunction;
      }

      /* Identifying the codeword in the read bits */
      {
         int count, len = 1;
         u_int32 mask = 2; /* mask  000000000000010   */
         
         if (bits & mask) {
            count = 1;
            for (len = 1; count > 0 && len < 15; len++) {
               mask = mask << 1;
               if (bits & mask) 
                  count--;
            }
         } else {
            count = 2;
            for (len = 1; count > 0 && len < 15; len++) {
               mask = mask << 1;
               if (!(bits & mask))
                  count--;
            }
         }
         
         if (len >= 15) {
            deb("vdxGetRVLCDCTBlockBackwards:ERROR - illegal RVLC codeword.\n");
            retValue = VDX_OK_BUT_BIT_ERROR;
            goto exitFunction;
         }

         bits = bits & (0x7fff >> (15 - (len + 1)));
      }

      /* Get the RVLC table Index and length belonging to the codeword */
      if (vdxGetRVLCIndex(bits, &RVLCIndex, (int *) &RVLCLength, fIntraBlock, bitErrorIndication) != VDX_OK)
         goto exitFunction;

      /* Flush the codeword from the buffer */
      bibRewindBits(RVLCLength, inBuffer, &bibError);
      if (bibError)
         goto exitFunction;

      if (RVLCIndex == 7167)     /* ESCAPE */
      {  
         /* MARKER BIT */
         bibRewindBits(1, inBuffer, &bibError);
         if (bibError)
            goto exitFunction;
         if(!bibShowBits(1, inBuffer, &numBitsGot, bitErrorIndication,&bibError) 
            || bibError ) {
            deb("vdxGetRVLCDCTBlockBackwards:ERROR - Wrong marker bit.\n");
            if ( !bibError )
               retValue = VDX_OK_BUT_BIT_ERROR;
            goto exitFunction;
         }
         /* LEVEL */
         bibRewindBits(11, inBuffer, &bibError);
         if (bibError)
            goto exitFunction;
         level = (int) bibShowBits(11, inBuffer, &numBitsGot, bitErrorIndication, 
            &bibError);
         if (level == 0 || bibError ) {
            if (!bibError) {
               deb("vdxGetRVLCDCTBlockBackwards:ERROR - Invalid Level.\n");
               retValue = VDX_OK_BUT_BIT_ERROR;
            }
            goto exitFunction;
         }
         /* MARKER BIT */
         bibRewindBits(1, inBuffer, &bibError);
         if (bibError)
            goto exitFunction;
         if(!bibShowBits(1, inBuffer, &numBitsGot, bitErrorIndication,&bibError) 
            || bibError ) {
            if ( !bibError ) {
               deb("vdxGetRVLCDCTBlockBackwards:ERROR - Wrong marker bit.\n");
               retValue = VDX_OK_BUT_BIT_ERROR;
            }
            goto exitFunction;
         }
         /* RUN */
         bibRewindBits(6, inBuffer, &bibError);
         if (bibError)
            goto exitFunction;
         run = (int) bibShowBits(6, inBuffer, &numBitsGot, bitErrorIndication, 
            &bibError);
         if (bibError) {
            goto exitFunction;
         }
         /* LAST */
         bibRewindBits(1, inBuffer, &bibError);
         if (bibError)
            goto exitFunction;
         last = bibShowBits(1, inBuffer, &numBitsGot, bitErrorIndication, 
            &bibError);
         if (bibError) {
            goto exitFunction;
         }

         /* Get the first ESCAPE code from the buffer */
         bibRewindBits(5, inBuffer, &bibError);
         if (bibError)
            goto exitFunction;
         escape = bibShowBits(5, inBuffer, &numBitsGot, bitErrorIndication, 
            &bibError);
         if (bibError) {
            goto exitFunction;
         }

         if (escape != 1) {
            deb("vdxGetRVLCDCTBlockBackwards:ERROR - illegal escape code.\n");
            retValue = VDX_OK_BUT_BIT_ERROR;
            goto exitFunction;
         }

         RVLCLength += 25;

      } else {

         last = (RVLCIndex >> 16) & 1;
         run = (RVLCIndex >> 8) & 255;
         level = RVLCIndex & 255;
      }

      if (sign)
         level = -level;

      if (index == 63) {
         if (!last) {
            deb("vdxGetRVLCDCTBlockBackwards:ERROR - last TCOEFF problem.\n");
            retValue = VDX_OK_BUT_BIT_ERROR;
            goto exitFunction;
         } else 
            last = 0;
      }
      
      if (last) {
         bibFlushBits((RVLCLength + 1), inBuffer, &numBitsGot, bitErrorIndication, 
            &bibError);
         if (bibError)
            goto exitFunction;

      } else if (index - run < startIndex) {
         deb("vdxGetRVLCDCTBlockBackwards:ERROR - too many TCOEFFs.\n");
         retValue = VDX_OK_BUT_BIT_ERROR;
         goto exitFunction;

      } else {
         /* Do run-length decoding. Since we are decoding backwards, level has to be inserted first */
         block[index--] = level;

         while (run--)
            block[index--] = 0;
         
      }

   } while (!last); 

   exitFunction:

   {
      int i;   
      for(i=startIndex,index++; i<=63; i++,index++)
         block[i]= (index <= 63) ? block[index] : 0;
   }


   if (!bibError)
      return retValue;
   else if (bibError == ERR_BIB_NOT_ENOUGH_DATA) {
      return VDX_OK_BUT_BIT_ERROR;
   } else
      return VDX_ERR_BIB;
}

// End of File
