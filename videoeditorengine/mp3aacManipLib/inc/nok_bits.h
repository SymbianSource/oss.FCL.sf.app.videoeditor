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


/**************************************************************************
  nok_bits.h - Interface for bitstream handling.

  Author(s): Juha Ojanpera
  Copyright (c) 1999-2004 by Nokia Research Center, Audio-Visual Systems.
  *************************************************************************/

#ifndef NOKBITSTREAM_H_
#define NOKBITSTREAM_H_

/*-- Project Headers. --*/
#include "defines.h"

/*-- Bitstream supports any length... --*/
#define BITSMODULO_BUFFER

#ifndef BITSMODULO_BUFFER
#define MOD_OPCODE(x, y) (x & y)
#else
#define MOD_OPCODE(x, y) (x % y)
#endif /*-- MODULO_BUFFER --*/

const uint32 bsBitMask[] =
{0x0, 0x1, 0x3, 0x7, 0xF, 0x1F, 0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF,
 0x1FFFL, 0x3FFFL, 0x7FFFL, 0xFFFFL, 0x1FFFFL, 0x3FFFFL, 0x7FFFFL, 0xFFFFFL,
 0x1FFFFFL, 0x3FFFFFL, 0x7FFFFFL, 0xFFFFFFL, 0x1FFFFFFL, 0x3FFFFFFL, 0x7FFFFFFL,
 0xFFFFFFFL, 0x1FFFFFFFL, 0x3FFFFFFFL, 0x7FFFFFFFL, 0xFFFFFFFFL};

#ifdef BYTE_16bit
typedef uint16 DSP_BYTE;
#else
typedef uint8 DSP_BYTE;
#endif

/**
 * Data structure for accessing bits from input bitstram.
 */
typedef enum BYTE_MODE
{
  BIT8 = 0,
  BIT16
  
} BYTE_MODE;

#define MAX_BUFS (1)
 
typedef struct BufMapperStr
{
  uint8 numBufs;
  uint8 **bsBuf;
  uint32 *bsBufLen;
  uint32 *bytesRead;

} BufMapper;
 
class TBitStream
{
public:


  /** 
   * Bitstream (modulo/ring) buffer. */
  DSP_BYTE *dsp_buffer;
  uint8 *bit_buffer;

  /** 
   * Number of bits in the data type. 
   */
  int16 slotBits;

  /* Which bit buffer is to be used. */
  BYTE_MODE mode;

  /** 
   * Size of the buffer. 
   */
  uint32 buf_len;

  /** 
   * Bit counter. 
   */
  int16 bit_counter;

  /** 
   * Read index. 
   */
  uint32 buf_index;

  /** 
   * Write index. 
   */
  uint32 buf_writeIndex;

  /** 
   * Bitmask for 'buf_index'. 
   */
  uint32 buf_mask;

  /** 
   * Number of bytes read from the buffer. This field will be used to check
   * whether there are enough bits in the buffer to result a successfull
   * decoding of current frame.
   */
  uint32 slots_read;

  /** 
   * Number of bits read from the buffer. This will be resetted on a call-by-call
   * basis and therefore gives the number of bits that each call read from
   * the buffer. The buffer is updated according to this value.
   */
  uint32 bits_read;

};

/**
 * Bitstream manipulation methods. 
 */


/**
 * Initializes input bitstream.
 *
 * @param   bs         Bitstream handle
 * @param   bit_buffer Input bitstream
 * @param   size       Size of input bitstream in bytes
 */
IMPORT_C void BsInit(TBitStream *bs, uint8 *bit_buffer, uint32 size);

IMPORT_C void BsInit1(TBitStream *bs, uint8 *bit_buffer, uint32 size);

IMPORT_C void BsInit2(TBitStream *bs, DSP_BYTE *dsp_buffer, uint32 size);


/**
 * Retrieves size of bitstream in bytes.
 *
 * @param   bs   Bitstream handle
 *
 * @return       Size of bitstream      
 */
IMPORT_C uint32 BsGetBufSize(TBitStream *bs);

IMPORT_C uint32 BsGetBufOriginalSize(TBitStream *bs);

/**
 * Retrieves number of bits read from the bitstream.
 *
 * @param   bs   Bitstream handle
 */
IMPORT_C uint32 BsGetBitsRead(TBitStream *bs);

IMPORT_C void BsSetBitsRead(TBitStream *bs, uint32 bits_read);
IMPORT_C void BsClearBitsRead(TBitStream *bs);

/**
 * Resets bitstream.
 *
 * @param   bs   Bitstream handle
 */
IMPORT_C void BsReset(TBitStream *bs);

/**
 * Retrieves the number of bytes left in the bitstream.
 *
 * @param   bs     Bitstream handle
 *
 * @return  Number of bytes left in the bitstream
 */
IMPORT_C uint32 BsSlotsLeft(TBitStream *bs);

/**
 * Appends bits from one bitstream to another bitstream.
 *
 * @param   bs     Source btstream handle
 * @param   br     Destination btstream handle
 * @param   n      Number of bytes to copy
 */
IMPORT_C void BsMoveBytes(TBitStream *bs, TBitStream *br, int16 n);

/**
 * Appends bits from one specified buffer to bitstream.
 *
 * @param   bs           Destination btstream handle
 * @param   outBuf       Source buffer
 * @param   bytesToCopy  Number of bytes to copy
 *
 * @return  Number of bytes appended
 */
IMPORT_C uint32 BsCopyBytes(TBitStream *bs, uint8 *outBuf, uint32 bytesToCopy);

/**
 * Appends bits from one source to destination bitstream.
 *
 * @param   bs          Destination btstream handle
 * @param   outBuf      Source bitstream handle
 * @param   bitsToCopy  Number of bits to copy
 */
IMPORT_C void 
BsCopyBits(TBitStream *bsSrc, TBitStream *bsDst, int32 bitsToCopy);

/**
 * Rewinds the read index of the bitstream.
 *
 * @param   bs     Bitstream handle
 * @param   n      Number of bits to rewind
 */
IMPORT_C void BsRewindNBits(TBitStream *bs, uint32 nBits);

IMPORT_C void BsBufferUpdate(TBitStream *bs, int32 bytesRead);

IMPORT_C void BsSaveBufState(TBitStream *bsSrc, TBitStream *bsDst);

/**
 * Writes bits to bitstream.
 *
 * @param   bs     Bitstream handle
 * @param   n      Number of bits to write
 * @param   word   Data bits to write
 */
IMPORT_C void BsPutBits(TBitStream *bs, int16 n, uint32 word);

/**
 * Byte aligns bitstream by writing '0' bits.
 *
 * @param   bs  Bitstream handle
 *
 * @return  Number of bits written
 */
IMPORT_C int16 BsPutBitsByteAlign(TBitStream *bs);

/**
 * Reads bits from bitstream.
 *
 * @param   bs     Bitstream handle
 * @param   n      Number of bits to read
 *
 * @return  Read data bits
 */
IMPORT_C uint32 BsGetBits(TBitStream *bs, int16 n);

/**
 * Byte aligns bitstream by advanding read index.
 *
 * @param   bs  Bitstream handle
 *
 * @return  Number of bits skipped
 */
IMPORT_C int16 BsByteAlign(TBitStream *bs);

/**
 * Reads bits from bitstream without updating read index.
 *
 * @param   bs  Bitstream handle
 * @param   n   Number of bits to lookahead
 *
 * @return  Read data bits
 */
IMPORT_C uint32 BsLookAhead(TBitStream *bs, int16 n);

/**
 * Advances bitstream read index (8 bits at maximum!).
 *
 * @param   bs  Bitstream handle
 * @param   n   Number of bits to skip. Note that this can at maximum be 8 bits!
 */
IMPORT_C void BsSkipBits(TBitStream *bs, int16 n);

/**
 * Advances bitstream read index.
 *
 * @param   bs  Bitstream handle
 * @param   n   Number of bits to skip
 */
IMPORT_C void BsSkipNBits(TBitStream *bs, int32 n);

#endif /*-- BITSTREAM_H_ --*/
