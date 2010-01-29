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
* MPEG-4 sync code functions.
*
*/


#include "h263dConfig.h"

#include "sync.h"
#include "errcodes.h"
#include "debug.h"
#include "mpegcons.h"
#include "biblin.h"


/*
 * Global functions
 */

/* {{-output"sncCheckMpegVOP.txt"}} */
/*
 *
 * sncCheckMpegVOP
 *
 * Parameters:
 *    buffer                     a pointer to a bit buffer structure
 *    error                      error code
 *
 * Function:
 *    This function checks if the GOV, VOP start codes
 *    are the next codes in the bit buffer.
 *    NO stuffing is allowed before the code, the routine checks from the
 *    current position in the buffer-
 *    The code is not flushed from the buffer.
 *
 * Returns:
 *    SNC_NO_SYNC                if no sync code is found
 *    SNC_PSC                    if GOV or VOP start code is found
 *
 * Error codes:
 *    Error codes returned by bibFlushBits/bibGetBits/bibShowBits.
 *
 *    
 *    
 */

int sncCheckMpegVOP(bibBuffer_t *buffer, int16 *error)
/* {{-output"sncCheckMpegVOP.txt"}} */
{
   u_int32 bits;
   int16 newError = 0;
   int numBitsGot, bitErrorIndication = 0;

   bits = bibShowBits(32, buffer, &numBitsGot, &bitErrorIndication, &newError);
   
   if ((newError == ERR_BIB_NOT_ENOUGH_DATA) && (numBitsGot > 0 )) {
       /* Still bits in the buffer */
       deb("sncCheckSync: bibShowReturned not enough data but there are "
           "still bits in the buffer --> error cleared.\n");
       return SNC_NO_SYNC;
   } else if (newError) {
       *error = newError;
       return SNC_NO_SYNC;
   }

   if (bits == MP4_GROUP_START_CODE || bits == MP4_VOP_START_CODE || bits == MP4_EOB_CODE)
        return SNC_PSC;
   else 
        return SNC_NO_SYNC;
}

/* {{-output"sncCheckMpegSync.txt"}} */
/*
 *
 * sncCheckMpegSync
 *
 * Parameters:
 *    buffer                     a pointer to a bit buffer structure
 *    f_code                     f_code of the last P-vop
 *    error                      error code
 *
 * Function:
 *    This function checks if the GOV, VOP or Video Pcaket start codes
 *    are the next codes in the bit buffer.
 *    Stuffing is needed before the code and the number of stuffing bits
 *    is returned in numStuffBits parameter.
 *    The code is not flushed from the buffer.
 *
 * Returns:
 *    SNC_NO_SYNC                if no sync code is found
 *    SNC_GOV                    if GOV start code is found
 *    SNC_VOP                    if VOP start code is found
 *    SNC_VOS                    if VOS start code is found
 *    SNC_VIDPACK                if Video Packet start code is found
 *    SNC_STUFFING               if there is a bit with the value zero 
 *                               followed by less than 7 bits with the 
 *                               value one starting from the current position 
 *                               and after them the buffer (picture segment) ends
 *
 * Error codes:
 *    Error codes returned by bibFlushBits/bibGetBits/bibShowBits.
 *
 *    
 *    
 *    
 *    
 *    
 */

int sncCheckMpegSync(bibBuffer_t *buffer, int f_code, int16 *error)
/* {{-output"sncCheckMpegSync.txt"}} */
{
   u_int32 result, bits, start_code_val;
   int numBitsGot, i, shift_bits;
   int16 newError = 0;
   int bitErrorIndication = 0;

   shift_bits = 32-(16+f_code);

   bits = bibShowBits(32, buffer, &numBitsGot, &bitErrorIndication, &newError);
   
   if ( buffer->error )
      {
      // out of bits
      *error = (int16)buffer->error;
      return SNC_NO_SYNC;
      }


   for(i = 0; i <= 8; i++) {

       /* if stuffing is correct */
       if ( (i==0) || ((bits >> (32-i)) == (u_int32) ((1 << (i-1))-1)) ) {

           result = bits << i;

           if ((result >> 8) == 1) {

               /* Stuff start code */
               if (i != 0) {
                   bibFlushBits(i, buffer, &numBitsGot, &bitErrorIndication, &newError);
                   if (newError) {
                       *error = newError;
                       return SNC_NO_SYNC;
                   }
               }

               /* Check start code */
               start_code_val = 
                   bibShowBits(32, buffer, &numBitsGot, &bitErrorIndication, &newError);
               if ((newError == ERR_BIB_NOT_ENOUGH_DATA) && (numBitsGot > 0 )) {
                       /* Still bits in the buffer */
                       deb("sncCheckSync: bibShowReturned not enough data but there are "
                           "still bits in the buffer --> error cleared.\n");
                       if (i) bibRewindBits(i, buffer, &newError);
                       return SNC_NO_SYNC;
               } else if (newError) {
                   *error = newError;
                   if (i) bibRewindBits(i, buffer, &newError);
                   return SNC_NO_SYNC;
               }

               if (start_code_val == MP4_VOP_START_CODE) {
                   return SNC_VOP;
               } else if (start_code_val == MP4_VOS_START_CODE) {
                   return SNC_VOS;
               } else if (start_code_val == MP4_GROUP_START_CODE) {
                   return SNC_GOV;
               } else if (start_code_val == MP4_USER_DATA_START_CODE) {
                   return SNC_USERDATA;
               } else if (start_code_val == MP4_EOB_CODE) {
                   return SNC_EOB;
               } else if (((start_code_val >> (32-MP4_VID_START_CODE_LENGTH)) == MP4_VID_START_CODE) ||
                          ((start_code_val >> (32-MP4_VOL_START_CODE_LENGTH)) == MP4_VOL_START_CODE)) {
                   return SNC_VID;
               } else {
                   if (i) bibRewindBits(i, buffer, &newError);
                   continue;               
               }

           } else if (f_code && ((result >> shift_bits) == 1)) {
               if (i != 0) {
                   /* Stuff start code */
                   bibFlushBits(i, buffer, &numBitsGot, &bitErrorIndication, &newError);
                   if (newError) {
                       *error = newError;
                       return SNC_NO_SYNC;
                   }
               }
               return SNC_VIDPACK;
           }
       }
   }

   return SNC_NO_SYNC;
}


/* {{-output"sncRewindAndSeekNewMPEGSync.txt"}} */
/*
 * sncRewindAndSeekNewMPEGSync
 *    
 *
 * Parameters:
 *    numBitsToRewind            the number of bits to rewind before seeking
 *                               the synchronization code,
 *                               if negative, a default number of bits is
 *                               rewinded. It is recommended to use 
 *                               the SNC_DEFAULT_REWIND definition to represent
 *                               negative values in order to increase code 
 *                               readability.
 *    buffer                     a pointer to a bit buffer structure
 *    f_code                     f_code of the last P-vop
 *    error                      error code
 *
 * Function:
 *    This function is intended to be called after bit error detection.
 *    It rewinds some (already read) bits in order not to miss any sync code.
 *    Then, this function finds the next GOV, VOP or Video Packet start code
 *    which is not within the rewinded bits. The function discards the bits 
 *    before the synchronization code but does not remove the found code from 
 *    the buffer.
 *
 *
 * Returns:
 *    SNC_NO_SYNC                if no sync code was found and 
 *                               no more bits are available
 *    SNC_GOV                    if GOV start code is found
 *    SNC_VOP                    if VOP start code is found
 *    SNC_VIDPACK                if Video Packet start code is found
 *
 * Error codes:
 *    Error codes returned by bibFlushBits/bibGetBits/bibShowBits.
 *
 *    
 *    
 */

int sncRewindAndSeekNewMPEGSync(int numBitsToRewind, bibBuffer_t *buffer,
                                int f_code, int16 *error)
/* {{-output"sncRewindAndSeekNewMPEGSync.txt"}} */
{
   int sncCode;                     /* found sync code */
   u_int32 numRewindableBits;       /* number of bits which can be rewinded */
   u_int32 bitPosBeforeRewinding;   /* initial buffer bit index */
   u_int32 syncStartBitPos;         /* 1st bit index in found sync code */
   u_int32 syncEndBitPos;
    int nb = 0;
    int bei = 0;

   *error = 0;

   /* If default number of rewinded bits requested */
   if (numBitsToRewind < 0)
      /* 32 bits is considered to be enough */
      numBitsToRewind = 32;

   numRewindableBits = bibNumberOfRewBits(buffer);

   if (numRewindableBits < (u_int32) numBitsToRewind)
      numBitsToRewind = (int) numRewindableBits;

   bitPosBeforeRewinding = bibNumberOfFlushedBits(buffer);

   if (numBitsToRewind) bibRewindBits(numBitsToRewind, buffer, error);
   if (*error)
      return SNC_NO_SYNC;

   /* Loop */
   do {

      /* Seek the next synchronization code */
      sncCode = sncSeekMPEGStartCode (buffer, f_code, 0 /* this method used with DP and VP => VP resync is relevant */, 0, error);
      if (*error)
         return sncCode;

      syncStartBitPos = bibNumberOfFlushedBits(buffer);

      syncEndBitPos = syncStartBitPos + 
                      (u_int32) ((sncCode == SNC_VIDPACK) ? (16 + f_code) : 32);

      if (syncEndBitPos <= bitPosBeforeRewinding)
          bibFlushBits( 1, buffer, &nb, &bei, error );

   /* While the found sync code has been previously read */
   } while (syncEndBitPos <= bitPosBeforeRewinding);

   return sncCode;
}

/* {{-output"sncSeekMPEGSync.txt"}} */
/*
 * sncSeekMPEGSync
 *    
 *
 * Parameters:
 *    buffer                     a pointer to a bit buffer structure
 *    f_code                     f_code of the last P-vop
 *    error                      error code
 *
 * Function:
 *    Then, this function finds the next GOV, VOP or Video Packet start code
 *    from the buffer. The function discards the bits before the sync code
 *    but does not remove the found code from the buffer.
 *
 * Returns:
 *    SNC_NO_SYNC                if no sync code was found and 
 *                               no more bits are available
 *    SNC_GOV                    if GOV start code is found
 *    SNC_VOP                    if VOP start code is found
 *    SNC_VOS                    if VOS start code is found
 *    SNC_VIDPACK                if Video Packet start code is found
 *
 * Error codes:
 *    Error codes returned by bibFlushBits/bibGetBits/bibShowBits.
 *
 *    
 */

int sncSeekMPEGSync(bibBuffer_t *buffer, int f_code, int16 *error)
/* {{-output"sncSeekMPEGSync.txt"}} */
{
   u_int32 result;
   int numBitsGot, shift_bits;
   int16 newError = 0;
   int bitErrorIndication = 0;

   shift_bits = 32-(16+f_code);

   for (;;) {
       
       bitErrorIndication = 0;

       result = bibShowBits(32, buffer, &numBitsGot, &bitErrorIndication, &newError);

      
       if (newError == ERR_BIB_NOT_ENOUGH_DATA && numBitsGot) {
           /* Use the available bits */
           result <<= (32 - numBitsGot);
           newError = 0;
       } else if (newError) {
           deb("sncSeekSync: ERROR - bibShowBits failed.\n");
           *error = newError;
           return SNC_NO_SYNC;
       }

       if (result == MP4_GROUP_START_CODE)
           return SNC_GOV;
       else if (result == MP4_VOP_START_CODE)
           return SNC_VOP;
       else if (result == MP4_VOS_START_CODE)
           return SNC_VOS;
       else if (result == MP4_EOB_CODE)
           return SNC_EOB;
       else if (f_code && ((result >> shift_bits) == 1))
           return SNC_VIDPACK;
       else if ( buffer->error )
          {
          // out of bits
          *error = (int16)buffer->error;
          return SNC_NO_SYNC;
          }

       
       bibFlushBits(1, buffer, &numBitsGot, &bitErrorIndication, error);
   }
}

   /* {{-output"sncSeekMPEGStartCode.txt"}} */
/*
 * sncSeekMPEGStartCode
 *    
 *
 * Parameters:
 *    buffer                     a pointer to a bit buffer structure
 *    f_code                     f_code of the last P-vop
 *    skipVPSync                 nonzero if Video Packet sync codes can be skipped
 *    error                      error code
 *
 * Function:
 *    This function finds the next GOV, VOP or Video Packet start code
 *    from the buffer in byte-aligned positions. The function discards the bits before the sync code
 *    but does not remove the found code from the buffer.
 *
 * Returns:
 *    SNC_NO_SYNC                if no sync code was found and 
 *                               no more bits are available
 *    SNC_GOV                    if GOV start code is found
 *    SNC_VOP                    if VOP start code is found
 *    SNC_VOS                    if VOS start code is found
 *    SNC_VIDPACK                if Video Packet start code is found
 *
 * Error codes:
 *    Error codes returned by bibFlushBits/bibGetBits/bibShowBits.
 *
 *    
 */

int sncSeekMPEGStartCode(bibBuffer_t *buffer, int f_code, int skipVPSync, int checkUD, int16 *error)
/* {{-output"sncSeekMPEGSync.txt"}} */
{
  u_int32 result;
  int numBitsGot, shift_bits;
  int16 newError = 0;
  int bitErrorIndication = 0;
  int flush = 8;
  const u_int32 MAX_MPEG4_START_CODE = 0x000001ff;
  shift_bits = 32-(16+f_code);
  
  /* start codes are always byte aligned */
  /* move to the next byte aligned position, if not already there */
  if (buffer->bitIndex != 7)
  {
    bibForwardBits(buffer->bitIndex + 1, buffer); 
  }
  
  for (;;) 
  {
    bitErrorIndication = 0;
    result = bibShowBits(32, buffer, &numBitsGot, &bitErrorIndication, &newError);
    

    if ( buffer->error )
      {
      // out of bits
      *error = (int16)buffer->error;
      return SNC_NO_SYNC;
      }

    /* don't check all start codes, if it is not one of them */
    if (result <= MAX_MPEG4_START_CODE)
    {
      if (result == MP4_GROUP_START_CODE)
        return SNC_GOV;
      else if (result == MP4_VOP_START_CODE)
        return SNC_VOP;
      else if (result == MP4_VOS_START_CODE)
        return SNC_VOS;
      else if (result == MP4_EOB_CODE)
        return SNC_EOB;
      else if ( checkUD && (result == MP4_USER_DATA_START_CODE) )
      	return SNC_USERDATA;
      
    }
    else if (!skipVPSync && f_code && ((result >> shift_bits) == 1))
        {
        return SNC_VIDPACK;
        }

    // we continue here after either if all the if-else's inside the if above are false or if the last else-if is false

    // Note! the following optimization is based on MPEG-4 sync code prefix 0x000001. It doesn't work with any other prefixes.
    if ( !skipVPSync )
        {
        flush = 8;
        // a small optimization could be reached also with video packet sync markers, but is probably not worth the effort since 
        // it seems that it could be used to shift at most 16 bits at least in cases with typical f_code values
        // idea:
        //  at least if fcode == 15, possible vp resync markers are in the form
        //  00008xxx, 00009xxx, 0000axxx, 0000bxxx, 0000cxxx, 0000dxxx, 0000exxx, 0000fxxx
        //  the shifting above already removes the 15 lowest bits => 16th bit must be 1 and 16 highest bits
        //  should be 0
        //  in the old way, the sync code from 00008xxx is found in the following steps
        //  8xxxyyyy, 008xxxyy, => match
        //  hence we can skip 16 bits (or f-code dependent nr of bits) if 
        //    a) 32nd bit is 1
        //  If 32nd bit is 0, we can skip the 16-(nr of successive 0-MSB bits)
        }
        
    // then check for the other sync codes
    
    // the 1st check here is needed to optimize the checking: in hopeless cases only a single check is needed
    else if ( (result & 0x000000ff) <= 1 )
        {
        // the 1st check is needed to optimize the checking: in hopeless cases only a single check is needed
        if ( ((result & 0x000000ff ) == 1) && ((result & 0x00ffff00 ) > 0))
            {
            // yyxxxx01, where one of the x != 0 => hopeless
            flush = 32;
            }
        else if ( (result & 0x0000ffff ) == 0 )
            {
            // xxxx0000 => could be the 1st 2 bytes of sync code
            flush = 16;
            }
        else if ( (result & 0x000000ff) == 0 )
            {
            // yyyyxx00, where xx != 00 (checked above), could be the 1st byte of sync code
            flush = 24;
            }
        else if ( (result & 0x00ffffff) == 1 )
            {
            // xx000001 => shift 1 byte
            flush = 8;
            }
        else
            {
            // this looks duplicate to the 1st one, but is kept here for simplicity. The 1st one is there since it is the most probable and
            // hence most cases fall under it. If it was not there, then in most cases all the if's were evaluated and that means extra processing
            flush = 32;
            }
        }
    else
        {
        // hopeless
        flush = 32;
        }

    // flush bits
    bibFlushBits(flush, buffer, &numBitsGot, &bitErrorIndication, error);
    }
}


/* {{-output"sncSeekBitPattern.txt"}} */
/*
 * sncSeekBitPattern
 *    
 *
 * Parameters:
 *    buffer                     a pointer to a bit buffer structure
 *    error                      error code
 *    BitPattern                 to bit pattern to be found
 *    BitPatternLength           length of the bit pattern to be found
 *
 * Function:
 *    This function finds the next occurance of the given BitPattern 
 *    from the buffer. The function discards the bits before the BitPattern
 *    but does not remove the found code from the buffer.
 *
 * Returns:
 *    SNC_NO_SYNC                if the bit pattern was not found and 
 *                               no more bits are available
 *    SNC_PATTERN                if the BitPattern is found
 *
 * Error codes:
 *    Error codes returned by bibFlushBits/bibGetBits/bibShowBits.
 *
 *    
 */

int sncSeekBitPattern(bibBuffer_t *buffer, u_int32 BitPattern, int BitPatternLength, int16 *error)
/* {{-output"sncSeekBitPattern.txt"}} */
{
   u_int32 result;
   int numBitsGot;
   int16 newError = 0;
   int bitErrorIndication = 0;

   for (;;) {
       
       bitErrorIndication = 0;

       result = bibShowBits(32, buffer, &numBitsGot, &bitErrorIndication, &newError);

       if (newError == ERR_BIB_NOT_ENOUGH_DATA && numBitsGot >= BitPatternLength) {
           /* Use the available bits */
           result <<= (32 - numBitsGot);
           newError = 0;
       } else if (newError) {
           deb("sncSeekBitPattern: ERROR - bibShowBits failed.\n");
           *error = newError;
           return SNC_NO_SYNC;
       }

       if ((result >> (32 - BitPatternLength)) == BitPattern)
           return SNC_PATTERN;
       else if (result == MP4_GROUP_START_CODE)
           return SNC_GOV;
       else if (result == MP4_VOP_START_CODE)
           return SNC_VOP;
       else if (result == MP4_EOB_CODE)
           return SNC_EOB;
       else if ( buffer->error )
          {
          // out of bits
          *error = (int16)buffer->error;
          return SNC_NO_SYNC;
          }
       
       bibFlushBits(1, buffer, &numBitsGot, &bitErrorIndication, error);
   }
}

/* {{-output"sncRewindStuffing.txt"}} */
/*
 * sncRewindStuffing
 *    
 *
 * Parameters:
 *    buffer                     a pointer to a bit buffer structure
 *    error                      error code
 *
 * Function:
 *    This function recognizes and rewinds the stuffing bits (1..8) from
 *    the current position of the buffer.
 *
 * Returns:
 *    SNC_NO_SYNC                if the stuffing was not found
 *    SNC_PATTERN                if the stuffing has been rewinded successfully
 *
 * Error codes:
 *    Error codes returned by bibFlushBits/bibGetBits/bibShowBits.
 *
 *    
 */

int sncRewindStuffing(bibBuffer_t *buffer, int16 *error)
/* {{-output"sncRewindStuffing.txt"}} */
{
   u_int32 result;
   int numBitsGot, i;
   int16 newError = 0;
   int bitErrorIndication = 0;

   bibRewindBits(8, buffer, &newError);
   result = bibGetBits(8, buffer, &numBitsGot, &bitErrorIndication, &newError);
   if (newError) {
       deb("sncRewindStuffing: ERROR - bibShowBits failed.\n");
       *error = newError;
       return SNC_NO_SYNC;
   }

   for(i = 1; i <= 8; i++) {       
       /* if stuffing is correct */
       if ((result & (1 << (i-1))) == 0) {
           bibRewindBits(i, buffer, &newError);
           return SNC_PATTERN;
       }
   }

   return SNC_NO_SYNC;
}
// End of File
