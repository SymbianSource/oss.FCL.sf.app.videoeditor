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


#ifndef _BITBUFFER_H_
#define _BITBUFFER_H_


#include "globals.h"
#include "nrctyp32.h"


#define BIB_ERR_BIT_ERROR          -4
#define BIB_INCORRECT_TRAILING_BIT -3
#define BIB_ERR_NO_BITS            -2
#define BIB_ERROR                  -1
#define BIB_OK                      0


typedef struct _bitbuffer_s 
{
  u_int8 *data;     /* point to the bit buffer. The physical buffer is allocated by the main module */
  int dataLen;
  int bytePos;
  int bitpos;
  int32 currentBits;
  int errorStatus;
} bitbuffer_s;


#define bibRaiseError(bitbuf, error) ((bitbuf)->errorStatus = (error))

#define bibGetStatus(bitbuf) (bitbuf)->errorStatus


bitbuffer_s *bibOpen();

int bibInit(bitbuffer_s *bitbuf, u_int8 *streamBytes, int length);

int bibEnd(bitbuffer_s *bitbuf);

void bibEndSlice(bitbuffer_s *bitbuf);

void bibClose(bitbuffer_s *bitbuf);

int bibGetBitFunc(bitbuffer_s *bitbuf);

int32 bibGetBitsFunc(bitbuffer_s *bitbuf, int n);

int32 bibShowBitsFunc(bitbuffer_s *bitbuf, int n);

int bibSkipBitsFunc(bitbuffer_s *bitbuf, int n);

int bibGetByte(bitbuffer_s *bitbuf, int *byteRet);

int bibByteAlign(bitbuffer_s *bitbuf);

int bibSkipTrailingBits(bitbuffer_s *bitbuf);

int bibMoreRbspData(bitbuffer_s *bitbuf);

int32 bibGetNumRemainingBits(bitbuffer_s *bitbuf);

//int bibGetMax16bits(bitbuffer_s *bitbuf, TInt n, u_int32* retValue);
void syncBitBufferBitpos(bitbuffer_s *bitbuf);


/*
 *
 * bibGetBit
 *
 * Parameters:
 *      bitbuf                Bitbuffer object
 *      bit                   Return pointer for bit
 *
 * Function:
 *      Get next bit from bitbuffer.
 *
 * Returns:
 *      -
 *
 */
#define bibGetBit(bitbuf, bit) \
  if ((bitbuf)->bitpos <= 0) { \
    if ((bitbuf)->bytePos < (bitbuf)->dataLen) { \
      (bitbuf)->currentBits = (bitbuf)->data[(bitbuf)->bytePos++]; \
      (bitbuf)->bitpos = 7; \
      *(bit) = ((bitbuf)->currentBits >> (bitbuf)->bitpos) & 1; \
    } \
    else { \
      (bitbuf)->errorStatus = BIB_ERR_NO_BITS; \
      *(bit) = 0; \
    } \
  } \
  else { \
    (bitbuf)->bitpos--; \
    *(bit) = ((bitbuf)->currentBits >> (bitbuf)->bitpos) & 1; \
  }


/*
 *
 * bibGetBits
 *
 * Parameters:
 *      bitbuf                Bitbuffer object
 *      n                     Number of bits requested
 *      bits                  Return pointer for bits
 *
 * Function:
 *      Get next n bits from bitbuffer. If bitbuffer is low on bits,
 *      call bibGetBitsFunc to get more.
 *
 *      NOTE: maximum of 24 bits can be fetched
 *
 * Returns:
 *      -
 *
 */
#define bibGetBits(bitbuf, n, bits) \
  if ((n) > (bitbuf)->bitpos) { \
    if ((bitbuf)->bytePos+2 >= (bitbuf)->dataLen) \
      *(bits) = bibGetBitsFunc(bitbuf, n); \
    else { \
      do { \
        (bitbuf)->currentBits = ((bitbuf)->currentBits << 8) | (bitbuf)->data[(bitbuf)->bytePos++]; \
        (bitbuf)->bitpos += 8; \
      } while ((n) > (bitbuf)->bitpos); \
      (bitbuf)->bitpos -= (n); \
      *(bits) = ((bitbuf)->currentBits >> (bitbuf)->bitpos) & ~(((u_int32)-1)<<(n)); \
    } \
  } \
  else { \
    (bitbuf)->bitpos -= (n); \
    *(bits) = ((bitbuf)->currentBits >> (bitbuf)->bitpos) & ~(((u_int32)-1)<<(n)); \
  }


/*
 *
 * bibGetMax16bits
 *
 * Parameters:
 *      bitbuf                Bitbuffer object
 *      n                     Number of bits requested
 *      bits                  Return pointer for bits
 *
 * Function:
 *      Get next n bits from bitbuffer. If bitbuffer is low on bits,
 *      call bibGetBitsFunc to get more.
 *
 *      NOTE: maximum of 16 bits can be fetched
 *
 * Returns:
 *      -
 *
 */
#define bibGetMax16bits(bitbuf, n, bits) \
  if ((n) > (bitbuf)->bitpos) { \
    if ((bitbuf)->bytePos+1 >= (bitbuf)->dataLen) \
      *(bits) = bibGetBitsFunc(bitbuf, n); \
    else { \
      (bitbuf)->currentBits = ((bitbuf)->currentBits << 8) | (bitbuf)->data[(bitbuf)->bytePos++]; \
      (bitbuf)->currentBits = ((bitbuf)->currentBits << 8) | (bitbuf)->data[(bitbuf)->bytePos++]; \
      (bitbuf)->bitpos += 16; \
      (bitbuf)->bitpos -= (n); \
      *(bits) = ((bitbuf)->currentBits >> (bitbuf)->bitpos) & ~(((u_int32)-1)<<(n)); \
    } \
  } \
  else { \
    (bitbuf)->bitpos -= (n); \
    *(bits) = ((bitbuf)->currentBits >> (bitbuf)->bitpos) & ~(((u_int32)-1)<<(n)); \
  }


/*
 *
 * bibGetMax8bits
 *
 * Parameters:
 *      bitbuf                Bitbuffer object
 *      n                     Number of bits requested
 *      bits                  Return pointer for bits
 *
 * Function:
 *      Get next n bits from bitbuffer. If bitbuffer is low on bits,
 *      call bibGetBitsFunc to get more.
 *
 *      NOTE: maximum of 8 bits can be fetched
 *
 * Returns:
 *      -
 *
 */
#define bibGetMax8bits(bitbuf, n, bits) \
  if ((n) > (bitbuf)->bitpos) { \
    if ((bitbuf)->bytePos >= (bitbuf)->dataLen) \
      *(bits) = bibGetBitsFunc(bitbuf, n); \
    else { \
      (bitbuf)->currentBits = ((bitbuf)->currentBits << 8) | (bitbuf)->data[(bitbuf)->bytePos++]; \
      (bitbuf)->bitpos += 8; \
      (bitbuf)->bitpos -= (n); \
      *(bits) = ((bitbuf)->currentBits >> (bitbuf)->bitpos) & ~(((u_int32)-1)<<(n)); \
    } \
  } \
  else { \
    (bitbuf)->bitpos -= (n); \
    *(bits) = ((bitbuf)->currentBits >> (bitbuf)->bitpos) & ~(((u_int32)-1)<<(n)); \
  }


/*
 *
 * bibShowBits
 *
 * Parameters:
 *      bitbuf                Bitbuffer object
 *      n                     Number of bits requested
 *      bits                  Return pointer for bits
 *
 * Function:
 *      Get next n bits from bitbuffer without advancing bitbuffer pointer.
 *      If bitbuffer is low on bits, call bibShowBitsFunc to get more.
 *
 *      NOTE: maximum of 24 bits can be fetched
 *
 * Returns:
 *      -
 *
 */
#define bibShowBits(bitbuf, n, bits) \
  if ((n) > (bitbuf)->bitpos) { \
    if ((bitbuf)->bytePos+2 >= (bitbuf)->dataLen) \
      *(bits) = bibShowBitsFunc(bitbuf, n); \
    else { \
      do { \
        (bitbuf)->currentBits = ((bitbuf)->currentBits << 8) | (bitbuf)->data[(bitbuf)->bytePos++]; \
        (bitbuf)->bitpos += 8; \
      } while ((n) > (bitbuf)->bitpos); \
      *(bits) = ((bitbuf)->currentBits >> ((bitbuf)->bitpos-(n))) & ~(((u_int32)-1)<<(n)); \
    } \
  } \
  else \
    *(bits) = ((bitbuf)->currentBits >> ((bitbuf)->bitpos-(n))) & ~(((u_int32)-1)<<(n));


/*
 *
 * bibShowMax16bits
 *
 * Parameters:
 *      bitbuf                Bitbuffer object
 *      n                     Number of bits requested
 *      bits                  Return pointer for bits
 *
 * Function:
 *      Get next n bits from bitbuffer without advancing bitbuffer pointer.
 *      If bitbuffer is low on bits, call bibShowBitsFunc to get more.
 *
 *      NOTE: maximum of 16 bits can be fetched
 *
 * Returns:
 *      -
 *
 */
#define bibShowMax16bits(bitbuf, n, bits) \
  if ((n) > (bitbuf)->bitpos) { \
    if ((bitbuf)->bytePos+1 >= (bitbuf)->dataLen) \
      *(bits) = bibShowBitsFunc(bitbuf, n); \
    else { \
      (bitbuf)->currentBits = ((bitbuf)->currentBits << 8) | (bitbuf)->data[(bitbuf)->bytePos++]; \
      (bitbuf)->currentBits = ((bitbuf)->currentBits << 8) | (bitbuf)->data[(bitbuf)->bytePos++]; \
      (bitbuf)->bitpos += 16; \
      *(bits) = ((bitbuf)->currentBits >> ((bitbuf)->bitpos-(n))) & ~(((u_int32)-1)<<(n)); \
    } \
  } \
  else \
    *(bits) = ((bitbuf)->currentBits >> ((bitbuf)->bitpos-(n))) & ~(((u_int32)-1)<<(n));


/*
 *
 * bibShowMax8bits
 *
 * Parameters:
 *      bitbuf                Bitbuffer object
 *      n                     Number of bits requested
 *      bits                  Return pointer for bits
 *
 * Function:
 *      Get next n bits from bitbuffer without advancing bitbuffer pointer.
 *      If bitbuffer is low on bits, call bibShowBitsFunc to get more.
 *
 *      NOTE: maximum of 8 bits can be fetched
 *
 * Returns:
 *      -
 *
 */
#define bibShowMax8bits(bitbuf, n, bits) \
  if ((n) > (bitbuf)->bitpos) { \
    if ((bitbuf)->bytePos >= (bitbuf)->dataLen) \
      *(bits) = bibShowBitsFunc(bitbuf, n); \
    else { \
      (bitbuf)->currentBits = ((bitbuf)->currentBits << 8) | (bitbuf)->data[(bitbuf)->bytePos++]; \
      (bitbuf)->bitpos += 8; \
      *(bits) = ((bitbuf)->currentBits >> ((bitbuf)->bitpos-(n))) & ~(((u_int32)-1)<<(n)); \
    } \
  } \
  else \
    *(bits) = ((bitbuf)->currentBits >> ((bitbuf)->bitpos-(n))) & ~(((u_int32)-1)<<(n));


/*
 *
 * bibSkipBits
 *
 * Parameters:
 *      bitbuf                Bitbuffer object
 *      n                     Number of bits to be skipped
 *
 * Function:
 *      Called after calling bibShowBits to skip the number of bits that
 *      were actually needed. If bibShowBits fetched more bits than there were
 *      bits in bitbuf->currentBits, calls bibSkipBitsFunc to clean up.
 *
 * Returns:
 *      -
 *
 */
#define bibSkipBits(bitbuf, n) \
  (bitbuf)->bitpos -= (n); \
  if ((bitbuf)->bitpos < 0) { \
    (bitbuf)->bitpos += (n); \
    bibSkipBitsFunc(bitbuf, n); \
  }



#endif  /* #ifndef _BITBUFFER_H_ */
