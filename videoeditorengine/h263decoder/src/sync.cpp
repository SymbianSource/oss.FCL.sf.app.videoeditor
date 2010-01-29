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
* H.263 sync code functions.
*
*/




#include "h263dConfig.h"

#include "sync.h"
#include "errcodes.h"
#include "debug.h"
#include "biblin.h"



/*
 * Global functions
 */

/* {{-output"sncCheckSync.txt"}} */
/*
 * sncCheckSync
 *    
 *
 * Parameters:
 *    buffer                     a pointer to a bit buffer structure
 *    numStuffBits               the number of stuffing bits
 *    error                      error code
 *
 * Function:
 *    This function checks if the Picture Start Code (PSC),
 *    the End Of Sequence code (EOS) or the GOB synchronization code (GBSC)
 *    are the next codes in the bit buffer.
 *    Stuffing (PSTUF, ESTUF or GSTUF) is allowed before the code and
 *    the number of stuffing bits is returned in numStuffBits parameter.
 *    The code is not flushed from the buffer.
 *
 * Returns:
 *    SNC_NO_SYNC                if no sync code is found
 *    SNC_PSC                    if PSC is found
 *    SNC_GBSC                   if GBSC is found
 *    SNC_EOS                    if EOS is found
 *    SNC_STUFFING               if there are less than 8 zeros starting from 
 *                               the current position and after 
 *                               them the buffer ends
 *
 * Error codes:
 *    Error codes returned by bibFlushBits/bibGetBits/bibShowBits.
 *
 */

int sncCheckSync(bibBuffer_t *buffer, int *numStuffBits, int16 *error)
/* {{-output"sncCheckSync.txt"}} */
{
   u_int32 result;
   int numBitsGot, i;
   int16 newError = 0;
   int bitErrorIndication = 0;

   for(i = 0; i < 8; i++) {
      bitErrorIndication = 0;
      result = bibShowBits(i + 22, buffer, &numBitsGot, &bitErrorIndication, 
         &newError);
      if (result == 32) {
         /* PSC */
         *numStuffBits = i;
         return SNC_PSC;
      }

      else if (result == 63) {
        /* EOS */
        *numStuffBits = i;
        return SNC_EOS;
      }

      else if (result > 32 && result < 63) {
         /* GBSC */
         /* Notice that the allowed Group Numbers are listed in section 5.2.3
            of the H.263 recommendation. They are not checked here but rather
            they are assumed to be checked in the corresponding header reading
            function, e.g. vdxGetGOBHeader. */
         *numStuffBits = i;
         return SNC_GBSC;
      }

      else if (result >= 64) {
         return SNC_NO_SYNC;
      }
      else if ( buffer->error )
          {
          // out of bits
          *error = (int16)buffer->error;
          return SNC_NO_SYNC;
          }
   }

   return SNC_NO_SYNC;
}


/* {{-output"sncRewindAndSeekNewSync.txt"}} */
/*
 * sncRewindAndSeekNewSync
 *    
 *
 * Parameters:
 *    earliestBitPos             location of previously found sync code
 *                               so no sense to rewind over it
 *    buffer                     a pointer to a bit buffer structure
 *    error                      error code
 *
 * Function:
 *    This function is intended to be called after GOB header is corrupted.
 *    It rewinds some (already read) bits in order not to miss any sync code.
 *    Then, this function finds the next Picture Start Code (PSC),
 *    End Of Sequence code (EOS) or Group of Blocks Start Code (GBSC)
 *    The function discards the bits before the synchronization code 
 *    but does not remove the found code from the buffer.
 *
 * Returns:
 *    SNC_NO_SYNC                if no PSC, EOS either GBSC was found and 
 *                               no more bits are available
 *    SNC_PSC                    if PSC is found
 *    SNC_GBSC                   if GBSC is found
 *    SNC_EOS                    if EOS is found
 *
 * Error codes:
 *    Error codes returned by bibFlushBits/bibGetBits/bibShowBits.
 *
 */

int sncRewindAndSeekNewSync(u_int32 earliestBitPos, bibBuffer_t *buffer, 
   int16 *error)
/* {{-output"sncRewindAndSeekNewSync.txt"}} */
{
   int sncCode;                     /* found sync code */
   u_int32 numRewindableBits;       /* number of bits which can be rewinded */
   u_int32 bitPosBeforeRewinding;   /* initial buffer bit index */
   u_int32 numBitsToRewind;         /* number of bits to rewind before seeking the sync code */

   *error = 0;

   bitPosBeforeRewinding = bibNumberOfFlushedBits(buffer);

   if ( bitPosBeforeRewinding > earliestBitPos+17 ) {
      numBitsToRewind = bitPosBeforeRewinding - earliestBitPos - 17; 
      /* 17 is the sync code length, and one sync code exists in earliestBitPos => do not read it again */
   }
   else if ( bitPosBeforeRewinding > earliestBitPos ) {
      /* This might be possible in slice mode */
      numBitsToRewind = bitPosBeforeRewinding - earliestBitPos; 
   }
   else {
      /* Bit counter overrun? */
      numBitsToRewind = bitPosBeforeRewinding + (0xffffffff - earliestBitPos) - 17; 
   }
   numRewindableBits = bibNumberOfRewBits(buffer);

   if (numRewindableBits < (u_int32) numBitsToRewind)
      numBitsToRewind = (int) numRewindableBits;

   bibRewindBits(numBitsToRewind, buffer, error);
   if (*error)
      return SNC_NO_SYNC;

   /* Seek the next synchronization code */
   sncCode = sncSeekSync(buffer, error);
   if (*error)
      return sncCode;

   return sncCode;
}


/* {{-output"sncSeekFrameSync.txt"}} */
/*
 * sncSeekFrameSync
 *    
 *
 * Parameters:
 *    buffer                     a pointer to a bit buffer structure
 *    error                      error code
 *
 * Function:
 *    This function finds the next Picture Start Code (PSC) or
 *    End Of Sequence code (EOS) from the bit buffer. The function skips
 *    any other codes. It discards the bits before the found synchronization
 *    code but does not remove the found code from the buffer.
 *
 * Returns:
 *    SNC_NO_SYNC                if no PSC or EOS was found and 
 *                               no more bits are available
 *    SNC_PSC                    if PSC is found
 *    SNC_EOS                    if EOS is found
 *
 * Error codes:
 *    Error codes returned by bibFlushBits/bibGetBits/bibShowBits.
 *
 */

int sncSeekFrameSync(bibBuffer_t *buffer, int16 *error)
/* {{-output"sncSeekFrameSync.txt"}} */
{
   int numBitsGot, syncCode, bitErrorIndication = 0;
   int16 newError = 0;

   for (;;) {
      syncCode = sncSeekSync(buffer, &newError);

      if (newError) {
         *error = newError;
         return SNC_NO_SYNC;
      }

      if (syncCode == SNC_GBSC) {
         bibFlushBits(17, buffer, &numBitsGot, &bitErrorIndication, &newError);
      }

      else
         return syncCode;
   }
}


/* {{-output"sncSeekSync.txt"}} */
/*
 * sncSeekSync
 *    
 *
 * Parameters:
 *    buffer                     a pointer to a bit buffer structure
 *    error                      error code
 *
 * Function:
 *    This function finds the next Picture Start Code (PSC),
 *    End Of Sequence code (EOS) or Group of Blocks Start Code (GBSC)
 *    from the bit buffer. It discards the bits before the synchronization
 *    code but does not remove the found code from the buffer.
 *
 * Returns:
 *    SNC_NO_SYNC                if no PSC, EOS either GBSC was found and 
 *                               no more bits are available
 *    SNC_PSC                    if PSC is found
 *    SNC_GBSC                   if GBSC is found
 *    SNC_EOS                    if EOS is found
 *
 * Error codes:
 *    Error codes returned by bibFlushBits/bibGetBits/bibShowBits.
 *
 */

int sncSeekSync(bibBuffer_t *buffer, int16 *error)
/* {{-output"sncSeekSync.txt"}} */
{
   u_int32 result;
   int numBitsGot;
   int16 newError = 0;
   int bitErrorIndication = 0;

   for (;;) {
      bitErrorIndication = 0;
      result = bibShowBits(22, buffer, &numBitsGot, &bitErrorIndication, 
         &newError);


      if (result == 32) {
         /* PSC */
         return SNC_PSC;
      }

      else if (result == 63) {
            /* EOS */
            return SNC_EOS;
      }

      else if (result > 32 && result < 63) {
         /* GBSC */
         /* Notice that the allowed Group Numbers are listed in section 5.2.3
            of the H.263 recommendation. They are not checked here but rather
            they are assumed to be checked in the corresponding header reading
            function, e.g. vdxGetGOBHeader. */
         return SNC_GBSC;
      }
      else if ( buffer->error )
          {
          // out of bits
          *error = (int16)buffer->error;
          return SNC_NO_SYNC;
          }

      bibFlushBits(1, buffer, &numBitsGot, &bitErrorIndication, error);
   }
}

// End of file
