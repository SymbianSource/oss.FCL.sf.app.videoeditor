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
*
*/


#include <string.h>
#include "globals.h"
#include "nrctyp32.h"
#include "bitbuffer.h"

#include "parameterset.h"


/*
 * Static functions
 */

static int removeStartCodeEmulationBytes(bitbuffer_s *bitbuf);
static int addStartCodeEmulationBytes(bitbuffer_s *bitbuf);


/*
 *
 * bibOpen:
 *
 * Parameters:
 *
 * Function:
 *      Open bitbuffer
 *
 * Returns:
 *      Pointer to bitbuffer object or NULL for allocation failure.
 *
 */
bitbuffer_s *bibOpen()
{
  bitbuffer_s *bitbuf;

  bitbuf = (bitbuffer_s *)User::Alloc(sizeof(bitbuffer_s));

  if (bitbuf != NULL)
    memset(bitbuf, 0, sizeof(bitbuffer_s));

  return bitbuf;
}


/*
 *
 * bibInit:
 *
 * Parameters:
 *      bitbuf                Bitbuffer object
 *      streamBytes           Pointer to data
 *      length                Data length in bytes
 *
 * Function:
 *      Initialize bitbuffer
 *
 * Returns:
 *      BIB_ok for ok, BIB_ERROR for error
 *
 */
int bibInit(bitbuffer_s *bitbuf, u_int8 *streamBytes, int length)
{
  bitbuf->data           = streamBytes;
  bitbuf->dataLen        = length;
  bitbuf->bytePos        = 0;
  bitbuf->bitpos         = 0;
  bitbuf->errorStatus    = BIB_OK;

#if ENCAPSULATED_NAL_PAYLOAD
  if (removeStartCodeEmulationBytes(bitbuf) < 0)
    return BIB_ERROR;
#endif

  return BIB_OK;
}


/*
 *
 * bibClose:
 *
 * Parameters:
 *      bitbuf                Bitbuffer object
 *
 * Function:
 *      Close bitbuffer
 *
 * Returns:
 *      -
 *
 */
void bibClose(bitbuffer_s *bitbuf)
{
  User::Free(bitbuf);
}


/*
 *
 * removeStartCodeEmulationBytes:
 *
 * Parameters:
 *      bitbuf                Bitbuffer object
 *
 * Function:
 *      Remove start code emulation bytes from the bitbuffer
 *
 * Returns:
 *      -
 *
 */
static int removeStartCodeEmulationBytes(bitbuffer_s *bitbuf)
{
  	TInt i;
  	TInt j;
  	TInt numZero;
  	TInt32 lastBytes;


  	// Skip the start code if it exists
  	numZero = 0;
  	i = 0;
  	while (i < bitbuf->dataLen) 
  	{
    	if (bitbuf->data[i] == 0)
      		numZero++;
    	else if (numZero > 1 && bitbuf->data[i] == 1) 
    	{
      		// Start code found
      		i++;
      		break;
    	}
    	else 
    	{
      		// No start code found 
      		i = 0;
      		break;
    	}
    	i++;
  	}

  	// Convert EBSP to RBSP. Note that the nal head byte is kept within the buffer
  	lastBytes = 0xffffffff;
  	j = 0;
  	while (i < bitbuf->dataLen) 
  	{
    	lastBytes = (lastBytes << 8) | bitbuf->data[i];
    	if ((lastBytes & 0xffffff) != 0x000003) 
    	{
	      	bitbuf->data[j] = bitbuf->data[i];
	      	j++;
    	}
    	i++;
  	}

  	// If bytes were removed, set as many bytes zero at the end of the buffer
  	if (j < bitbuf->dataLen)
  	{
  		// Prevention bytes have been removed, set the last bytes to zero
  		TInt removedBytes = bitbuf->dataLen - j;
  		for (i=0; i<removedBytes; i++)
  		{
  			bitbuf->data[bitbuf->dataLen-1-i] = 0;
  		}
	}
  	
  	// Adjust the bitbuffer dataLen
  	bitbuf->dataLen = j;

  	return BIB_OK;
}


/*
 *
 * bibGetBitFunc:
 *
 * Parameters:
 *      bitbuf                Bitbuffer object
 *
 * Function:
 *      Get next bit from bitbuffer.
 *
 * Returns:
 *      Next bit in bitbuffer or BIB_ERR_NO_BITS if no bits left.
 *
 */
int bibGetBitFunc(bitbuffer_s *bitbuf)
{
  /* If there are no bits left in buffer return an error */
  if (bitbuf->bitpos == 0 && bitbuf->bytePos >= bitbuf->dataLen) {
    bitbuf->errorStatus = BIB_ERR_NO_BITS;
    return 0;
  }

  /* Fill bitbuf->currentBits with bits */
  while (bitbuf->bitpos <= 24 && bitbuf->bytePos < bitbuf->dataLen) {
    bitbuf->currentBits = (bitbuf->currentBits << 8) | bitbuf->data[bitbuf->bytePos++];
    bitbuf->bitpos += 8;
  }

  /* Return bit */
  bitbuf->bitpos--;
  return (bitbuf->currentBits >> bitbuf->bitpos) & 1;
}


/*
 *
 * bibGetBitsFunc:
 *
 * Parameters:
 *      bitbuf                Bitbuffer object
 *      n                     Number of bits requested
 *
 * Function:
 *      Get next n bits from bitbuffer.
 *
 *      NOTE: maximum of 24 bits can be fetched
 *
 * Returns:
 *      Next n bits from bitbuffer
 *
 */
int32 bibGetBitsFunc(bitbuffer_s *bitbuf, int n)
{
  /* Fill bitbuf->currentBits with bits */
  while (n > bitbuf->bitpos && bitbuf->bytePos < bitbuf->dataLen) {
    bitbuf->currentBits = (bitbuf->currentBits << 8) | bitbuf->data[bitbuf->bytePos++];
    bitbuf->bitpos += 8;
  }

  /* If there are not enought bits there was an error */
  if (n > bitbuf->bitpos) {
    bitbuf->errorStatus = BIB_ERR_NO_BITS;
    return 0;
  }

  /* Return bits */
  bitbuf->bitpos -= n;
  return (bitbuf->currentBits >> (bitbuf->bitpos)) & ~(((u_int32)-1)<<n);
}


/*
 *
 * bibShowBitsFunc:
 *
 * Parameters:
 *      bitbuf                Bitbuffer object
 *      n                     Number of bits requested
 *
 * Function:
 *      Get next n bits from bitbuffer without advancing bitbuffer pointer.
 *      This function will not failt even if there are not enough bits in
 *      the bitbuffer.
 *
 *      NOTE: maximum of 24 bits can be fetched
 *
 * Returns:
 *      Next n bits of bitbuffer
 *
 */
int32 bibShowBitsFunc(bitbuffer_s *bitbuf, int n)
{
  /* Fill bitbuf->currentBits with bits */
  while (n > bitbuf->bitpos && bitbuf->bytePos < bitbuf->dataLen) {
    bitbuf->currentBits = (bitbuf->currentBits << 8) | bitbuf->data[bitbuf->bytePos++];
    bitbuf->bitpos += 8;
  }

  /* Check if there are enought bits in currentBits */
  if (n <= bitbuf->bitpos)
    /* Return bits normally */
    return (bitbuf->currentBits >> (bitbuf->bitpos-n)) & ~(((u_int32)-1)<<n);
  else
    /* Return bits padded with zero bits */
    return (bitbuf->currentBits << (n-bitbuf->bitpos)) & ~(((u_int32)-1)<<n);
}


/*
 *
 * bibSkipBitsFunc:
 *
 * Parameters:
 *      bitbuf                Bitbuffer object
 *      n                     Number of bits to skip
 *
 * Function:
 *      Skip next n bits in the bitbuffer
 *
 * Returns:
 *      BIB_OK for no error and BIB_ERR_NO_BITS for end of buffer.
 *
 */
int bibSkipBitsFunc(bitbuffer_s *bitbuf, int n)
{
  /* Fill bitbuf->currentBits with bits */
  while (n > bitbuf->bitpos && bitbuf->bytePos < bitbuf->dataLen) {
    bitbuf->currentBits = (bitbuf->currentBits << 8) | bitbuf->data[bitbuf->bytePos++];
    bitbuf->bitpos += 8;
  }

  bitbuf->bitpos -= n;

  /* Check for buffer underrun */
  if (bitbuf->bitpos < 0) {
    bitbuf->errorStatus = BIB_ERR_NO_BITS;
    return BIB_ERR_NO_BITS;
  }

  return BIB_OK;
}


/*
 *
 * bibGetByte:
 *
 * Parameters:
 *      bitbuf                Bitbuffer object
 *      byteRet               Return pointer for byte
 *
 * Function:
 *      Get next byte aligned byte from bitbuffer.
 *
 * Returns:
 *      1 for End Of Stream, 0 otherwise
 *
 */
int bibGetByte(bitbuffer_s *bitbuf, int *byteRet)
{
  if (bitbuf->bitpos >= 8) {
    bitbuf->bitpos = bitbuf->bitpos & ~7;
    *byteRet = (bitbuf->currentBits >> (bitbuf->bitpos - 8)) & 0xff;
    bitbuf->bitpos -= 8;
  }
  else {
    bitbuf->bitpos = 0;

    if (bitbuf->bytePos >= bitbuf->dataLen) {
      return 1;
    }

    *byteRet = bitbuf->data[bitbuf->bytePos++];
  }

  return 0;
}


/*
 *
 * bibByteAlign:
 *
 * Parameters:
 *      bitbuf                Bitbuffer object
 *
 * Function:
 *      Set bitbuffer pointer to next byte aligned position.
 *
 * Returns:
 *      Number of bits skipped as a result of aligning.
 *
 */
int bibByteAlign(bitbuffer_s *bitbuf)
{
  int bitpos = bitbuf->bitpos;

  bitbuf->bitpos = bitbuf->bitpos & ~7;

  return (bitpos - bitbuf->bitpos);
}


/*
 * bibSkipTrailingBits:
 *
 * Parameters:
 *      bitbuf                Bitbuffer object
 *
 * Function:
 *      Skip the trailing bits at the end of a NAL unit
 *
 * Returns:
 *      The number of bits being skipped or <0 for error.
 */
int bibSkipTrailingBits(bitbuffer_s *bitbuf)
{
  int ret;
  int len = 0;
  int bit = 0;

  bit = bibGetBitFunc(bitbuf);
  len++;

  ret = bibGetStatus(bitbuf);
  if (ret < 0)
    return ret;

  /* First we expect to receive 1 bit */
  if (bit != 1) {
    bibRaiseError(bitbuf, BIB_INCORRECT_TRAILING_BIT);
    return BIB_INCORRECT_TRAILING_BIT;
  }

  /* Remaining bits in current byte should be zero */
  while ( bitbuf->bitpos % 8 != 0 ) {  
    bibGetBit(bitbuf, &bit);
    len++;
    if (bit != 0) {
      bibRaiseError(bitbuf, BIB_INCORRECT_TRAILING_BIT);
      return BIB_INCORRECT_TRAILING_BIT;
    }
  }

  ret = bibGetStatus(bitbuf);
  if (ret < 0)
    return ret;

  return len;
}


/*
 * bibMoreRbspData:
 *
 * Parameters:
 *      bitbuf                Bitbuffer object
 *
 * Function:
 *      Check if there is more RBSP data in the bitbuffer.
 *
 * Returns:
 *      0: no more rbsp data
 *      1: more rbsp data
 */
int bibMoreRbspData(bitbuffer_s *bitbuf)
{
  int numBytesLeft;
  u_int32 lastBits;

  numBytesLeft = bitbuf->dataLen - bitbuf->bytePos;

  if (numBytesLeft >= 2 || (numBytesLeft*8 + bitbuf->bitpos >= 9))
    /* If there are at least 9 bits left, it is certain to have more rbsp data */
    return 1;

  if (numBytesLeft == 0 && bitbuf->bitpos == 0)
    /* Something may be wrong. Normally, there should be at least one bit left */
    return 0;

  if (numBytesLeft == 1) {
    /* Insert last byte to currentBits */
    bitbuf->currentBits = (bitbuf->currentBits << 8) | bitbuf->data[bitbuf->bytePos++];
    bitbuf->bitpos += 8;
  }

  /* Copy the last bits into "lastBits", then compare it with 0x1..00 */
  lastBits = bitbuf->currentBits & ~(((u_int32)-1)<<bitbuf->bitpos);

  if (lastBits == ((u_int32)1 << (bitbuf->bitpos-1)))
    return 0;
  else
    return 1;
}


/*
 * bibGetNumOfRemainingBits:
 *
 * Parameters:
 *      bitbuf                Bitbuffer object
 *
 * Function:
 *      Return number of bits in bitbuffer.
 *
 * Returns:
 *      Number of bits
 */
int32 bibGetNumRemainingBits(bitbuffer_s *bitbuf)
{
  return bitbuf->bitpos + 8*(bitbuf->dataLen - bitbuf->bytePos);
}


// syncBitBufferBitpos
// Synchronizes the input bit buffer's bit position to be between one and eight, 
// modifies the byte position and current bits if required.
void syncBitBufferBitpos(bitbuffer_s *bitbuf)
{
	// To be able to edit the bitbuffer, reset the bit position to be eight at the maximum
	while (bitbuf->bitpos > 8)
	{
		// If bit position is modified, then modify byte position and current bits accordingly
		bitbuf->bitpos -= 8;
		bitbuf->bytePos--;
		bitbuf->currentBits >>= 8;
	}
}


// addStartCodeEmulationBytes
// Adds start code emulation bytes to the input bit buffer.
static int addStartCodeEmulationBytes(bitbuffer_s *bitbuf)
{
  	TInt i = 0;
  	TInt32 lastBytes;


  	// Add prevention bytes that were removed for processing
  	lastBytes = 0xffffffff;
  	while (i < bitbuf->dataLen) 
  	{
    	lastBytes = (lastBytes << 8) | bitbuf->data[i];
    	
    	if(((lastBytes & 0xffff) == 0x0000) && ((i+1) < bitbuf->dataLen) && (bitbuf->data[i+1] < 4))
    	{
			// Add byte(s) to the bit buffer
			TInt error = AddBytesToBuffer(bitbuf, 1);

			if (error != 0)
				return error;

    		// Adjust data length
    		bitbuf->dataLen++;

    		// Make room for the emulation prevention byte 0x03 to the buffer
    		for (TInt k=bitbuf->dataLen; k>i; k--)
    		{
    			bitbuf->data[k] = bitbuf->data[k-1];
    		}
    		
    		// Add the emulation prevention byte to the buffer
      		bitbuf->data[i+1] = 0x03;
    	}
    	
    	i++;
  	}
	return KErrNone;
}


// bibEnd
// Takes care of the bit buffer after the frame has been processed.
int bibEnd(bitbuffer_s *bitbuf)
{
#if ENCAPSULATED_NAL_PAYLOAD
  	return addStartCodeEmulationBytes(bitbuf);
#endif
}


// addStartCodeEmulationBytesSlice
// Adds start code emulation bytes to the input bit buffer.
static void addStartCodeEmulationBytesSlice(bitbuffer_s *bitbuf)
{
  	TInt i = 0;
  	TInt32 lastBytes;


  	// Add prevention bytes that were removed for processing
  	lastBytes = 0xffffffff;
  	while (i < bitbuf->dataLen) 
  	{
    	lastBytes = (lastBytes << 8) | bitbuf->data[i];
    	
    	if(((lastBytes & 0xffff) == 0x0000) && ((i+1) < bitbuf->dataLen) && (bitbuf->data[i+1] < 4))
    	{
    		// Make room for the emulation prevention byte 0x03 to the buffer
    		for (TInt k=bitbuf->dataLen; k>i; k--)
    		{
    			bitbuf->data[k] = bitbuf->data[k-1];
    		}
    		// Adjust data length
    		bitbuf->dataLen++;
    		
    		// Add the emulation prevention byte to the buffer
      		bitbuf->data[i+1] = 0x03;
    	}
    	
    	i++;
  	}
}


// bibEndSlice
// Takes care of the bit buffer after the frame has been processed.
void bibEndSlice(bitbuffer_s *bitbuf)
{
#if ENCAPSULATED_NAL_PAYLOAD
  	addStartCodeEmulationBytesSlice(bitbuf);
#endif
}

