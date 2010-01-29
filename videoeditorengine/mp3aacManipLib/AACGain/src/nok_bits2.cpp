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
  nok_bits2.c - Bitstream subroutines.

  Author(s): Juha Ojanpera
  Copyright (c) 1999-2003 by Nokia Research Center, Speech and Audio Systems.
  *************************************************************************/

/**************************************************************************
  External Objects Needed
  *************************************************************************/

/*-- Project Headers --*/
#include "nok_bits.h"

/**************************************************************************
  Internal Objects
  *************************************************************************/

#ifdef USE_ASSERT
#define BITSASSERT(x) { \
  MY_ASSERT(UpdateBufIdx(bs) == 1); \
}
#else
#define BITSASSERT(x) { \
  UpdateBufIdx(bs); \
}
#endif

//typedef uint32 (*GETBITS_FUNCTION)(BitStream *bs, int16 n);

const uint32 bitMask[] =
{0x0, 0x1, 0x3, 0x7, 0xF, 0x1F, 0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF,
 0x1FFFL, 0x3FFFL, 0x7FFFL, 0xFFFFL, 0x1FFFFL, 0x3FFFFL, 0x7FFFFL, 0xFFFFFL,
 0x1FFFFFL, 0x3FFFFFL, 0x7FFFFFL, 0xFFFFFFL, 0x1FFFFFFL, 0x3FFFFFFL, 0x7FFFFFFL,
 0xFFFFFFFL, 0x1FFFFFFFL, 0x3FFFFFFFL, 0x7FFFFFFFL, 0xFFFFFFFFL};



/*
 * Updates the read index of the bit buffer.
 */
static INLINE int16
UpdateBufIdx(TBitStream *bs)
{
  bs->buf_index++;
  bs->buf_index = MOD_OPCODE(bs->buf_index, bs->buf_mask);
  bs->slots_read++;
#if 0
  if(bs->slots_read == bs->buf_len)
    return (0);
#endif
  
  return (1);
}

/*
 * Writes at most 8 bits to the bit buffer.
 */
void
PutBits(TBitStream *bs, uint16 n, uint32 word)
{ 
  uint8 *buf = (bs->mode == BIT8) ? bs->bit_buffer : bs->dsp_buffer;
  /*
   * The number of bits left in the current buffer slot is too low. 
   * Therefore, the input word needs to be splitted into two parts. 
   * Each part is stored separately.
   */
  int16 tmp = bs->bit_counter - n;
  if(tmp < 0)
  {
    /*-- Zero those bits that are to be modified. --*/
    buf[bs->buf_index] &= ~bitMask[bs->bit_counter];
  
    /*-- Store the upper (MSB) part to the bitstream buffer. --*/
    tmp = -tmp;
    buf[bs->buf_index] |= (word >> tmp);

    /*
     * Explicitly update the bitstream buffer index. No need to 
     * test whether the bit counter is zero since it is known
     * to be zero at this point.
     */
    BITSASSERT(bs);
    bs->bit_counter = bs->slotBits;

    /*-- Store the lower (LSB) part to the bitstream buffer. --*/
    bs->bit_counter =  int16(bs->bit_counter  - tmp);
    buf[bs->buf_index] &= bitMask[bs->bit_counter];
    buf[bs->buf_index] |= ((word & bitMask[tmp]) << bs->bit_counter);
  }
  else
  {
    /*
     * Obtain the bit mask for the part that is to be modified. For example,
     * 'bit_counter' = 5
     * 'n' = 3,
     * 'tmp' = 2
     *
     * x x x m m m x x (x = ignore, m = bits to be modified)
     *
     * mask :
     *
     * 1 1 1 0 0 0 1 1
     *
     */
    buf[bs->buf_index] &= (~bitMask[bs->bit_counter]) | bitMask[tmp];
    bs->bit_counter = tmp;
    buf[bs->buf_index] |= (word << bs->bit_counter);
  }

  /*-- Update the bitstream buffer index. --*/
  if(bs->bit_counter == 0)
  {
    BITSASSERT(bs);
    bs->bit_counter = bs->slotBits;
  }
}

/*
 * Reads at most 8 bits from the bit buffer.
 */
uint32
GetBits(TBitStream *bs, int16 n)
{
  int16 idx;
  uint32 tmp;

  idx = bs->bit_counter - n;
  if(idx < 0)
  {
    /* Mask the unwanted bits to zero. */
    tmp = (bs->bit_buffer[bs->buf_index] & bitMask[bs->bit_counter]) << -idx;

    /* Update the bitstream buffer. */
    BITSASSERT(bs);
    bs->bit_counter = bs->slotBits + idx;
    tmp |= (bs->bit_buffer[bs->buf_index] >> bs->bit_counter) & bitMask[-idx];
  }
  else
  {
    bs->bit_counter = idx;
    tmp = (bs->bit_buffer[bs->buf_index] >> bs->bit_counter) & bitMask[n];
  }

  /* Update the bitstream buffer index. */
  if(bs->bit_counter == 0)
  {
    BITSASSERT(bs);
    bs->bit_counter = bs->slotBits;
  }

  return (tmp);
}

/*
 * Reads at most 16 bits from the bit buffer.
 */
uint32
GetBitsDSP(TBitStream *bs, int16 n)
{
  int16 idx;
  uint32 tmp;

  idx = bs->bit_counter - n;
  if(idx < 0)
  {
    /* Mask the unwanted bits to zero. */
    tmp = (bs->dsp_buffer[bs->buf_index] & bitMask[bs->bit_counter]) << -idx;

    /* Update the bitstream buffer. */
    BITSASSERT(bs);
    bs->bit_counter = bs->slotBits + idx;
    tmp |= (bs->dsp_buffer[bs->buf_index] >> bs->bit_counter) & bitMask[-idx];
  }
  else
  {
    bs->bit_counter = idx;
    tmp = (bs->dsp_buffer[bs->buf_index] >> bs->bit_counter) & bitMask[n];
  }

  /* Update the bitstream buffer index. */
  if(bs->bit_counter == 0)
  {
    BITSASSERT(bs);
    bs->bit_counter = bs->slotBits;
  }

  return (tmp);
}

//typedef uint32 (*GETBITS_FUNCTION)(BitStream *bs, int16 n);
//static GETBITS_FUNCTION GetFunc[2] = {GetBits, GetBitsDSP};

/*
 * Returns the size of the buffer.
 */
EXPORT_C uint32 BsGetBufSize(TBitStream *bs)
{ return (bs->buf_len); }

/*
 * Returns the size of the original buffer. Note that it is 
 * possible that the update procedure of the bit buffer 
 * (which is up to the implementor to specify), reduces the size 
 * temporarily for some reasons.
 */
EXPORT_C uint32 BsGetBufOriginalSize(TBitStream *bs)
{
#ifndef BITSMODULO_BUFFER
  return (bs->buf_mask + 1);
#else
  return (bs->buf_mask);
#endif 
}

/*
 * Returns the number of bits read.
 */
EXPORT_C uint32 BsGetBitsRead(TBitStream *bs)
{ return (bs->bits_read); }

/*
 * Increments the value that describes how many 
 * bits are read from the bitstream.
 */
EXPORT_C void BsSetBitsRead(TBitStream *bs, uint32 bits_read) 
{ bs->bits_read += bits_read; }

/*
 * Clears the value that describes how many 
 * bits are read from the bitstream.
 */
EXPORT_C void BsClearBitsRead(TBitStream *bs)  
{ bs->bits_read = 0; }

/*
 * Returns the number of unread elements in the bit buffer.
 */
EXPORT_C uint32 BsSlotsLeft(TBitStream *bs)
{ return ((bs->buf_len < bs->slots_read) ? 0 : bs->buf_len - bs->slots_read); }

/*
 * Resets the bitstream.
 */
EXPORT_C void BsReset(TBitStream *bs)
{
  bs->buf_index = 0;
  bs->buf_writeIndex = 0;
  bs->bit_counter = bs->slotBits;
  bs->slots_read = 0;
  bs->bits_read = 0;
}

/*
 * Updates the bitstream values according to the given input parameter.
 */
EXPORT_C void BsBufferUpdate(TBitStream *bs, int32 bytesRead)
{
  int32 diff = bs->bits_read - (bytesRead << 3);
  
  bs->buf_len = (bs->buf_len - bs->slots_read) + bytesRead;
  bs->slots_read = 0;
  
  if(diff < 0)
    bs->bits_read = -diff;
  else
    bs->bits_read = diff;
}

/**************************************************************************
  Title       : BsInit

  Purpose     : Initializes the bit buffer.

  Usage       : BsInit(bs, bit_buffer, size)

  Input       : bs         - bitstream structure
                bit_buffer - address of bit buffer
                size       - size of buffer

  Author(s)   : Juha Ojanpera
  *************************************************************************/

EXPORT_C void
BsInit(TBitStream *bs, uint8 *bit_buffer, uint32 size)
{
  bs->dsp_buffer = NULL;
  bs->bit_buffer = bit_buffer;
  bs->mode = BIT8;
#ifndef BITSMODULO_BUFFER
  bs->buf_mask = size - 1;
#else
  bs->buf_mask = size;
#endif
  bs->buf_len = size;
  bs->slotBits = sizeof(uint8) << 3;
  BsReset(bs);
}

void
BsInit2(TBitStream *bs, DSP_BYTE *dsp_buffer, uint32 size)
{
  bs->dsp_buffer = dsp_buffer;
  bs->bit_buffer = NULL;
  bs->mode = BIT16;
#ifndef BITSMODULO_BUFFER
  bs->buf_mask = size - 1;
#else
  bs->buf_mask = size;
#endif
  bs->buf_len = size;
  bs->slotBits = sizeof(DSP_BYTE) << 3;
  BsReset(bs);
}

/**************************************************************************
  Title        : BsByteAlign

  Purpose      : Byte aligns the bit counter of the buffer.

  Usage        : y = BsByteAlign(bs)

  Input        : bs - bitstream parameters

  Output       : y  - # of bits read

  Author(s)    : Juha Ojanpera
  *************************************************************************/

EXPORT_C int16
BsByteAlign(TBitStream *bs)
{
  int16 bits_to_byte_align;

  bits_to_byte_align = bs->bit_counter & 7;
  if(bits_to_byte_align)
    BsSkipBits(bs, bits_to_byte_align);

  return (bits_to_byte_align);
}

/**************************************************************************
  Title        : BsLookAhead

  Purpose      : Looks ahead for the next 'n' bits from the bit buffer and
                 returns the read 'n' bits.

  Usage        : y = BsLookAhead(bs, n)

  Input        : bs - bitstream parameters
                 n  - number of bits to be read from the bit buffer

  Output       : y  - bits read

  Author(s)    : Juha Ojanpera
  *************************************************************************/

EXPORT_C uint32
BsLookAhead(TBitStream *bs, int16 n)
{
  TBitStream bs_tmp;
  uint32 dword;
  
  /*-- Save the current state. --*/
  COPY_MEMORY(&bs_tmp, bs, sizeof(TBitStream));

  dword = BsGetBits(bs, n);

  /*-- Restore the original state. --*/
  COPY_MEMORY(bs, &bs_tmp, sizeof(TBitStream));

  return (dword);
}

/**************************************************************************
  Title        : BsPutBits

  Purpose      : Writes bits to the bit buffer.

  Usage        : BsPutBits(bs, n, word);

  Input        : bs   - bitstream parameters
                 n    - number of bits to write
                 word - bits to write

  Author(s)    : Juha Ojanpera
  *************************************************************************/

EXPORT_C void
BsPutBits(TBitStream *bs, int16 n, uint32 word)
{
  int16 rbits;

  BsSetBitsRead(bs, n);

  /*-- Mask the unwanted bits to zero, just for safety. --*/
  word &= bitMask[n];

  while(n)
  {
    rbits = (n > (int16)bs->slotBits) ? bs->slotBits : n;
    n -= rbits;
    PutBits(bs, rbits, ((word >> n) & bitMask[rbits]));
  }
}

/**************************************************************************
  Title        : BsPutBitsByteAlign

  Purpose      : Byte aligns the write buffer.

  Usage        : y = BsPutBitsByteAlign(bs)

  Input        : bs - bitstream parameters

  Output       : y  - # of bits written

  Author(s)    : Juha Ojanpera
  *************************************************************************/

EXPORT_C int16
BsPutBitsByteAlign(TBitStream *bs)
{
  int16 bits_to_byte_align;

  bits_to_byte_align = bs->bit_counter & 7;
  if(bits_to_byte_align)
    BsPutBits(bs, bits_to_byte_align, 0);

  return (bits_to_byte_align);
}

/**************************************************************************
  Title        : BsGetBits

  Purpose      : Reads bits from the bit buffer.

  Usage        : y = BsGetBits(bs, n);

  Input        : bs - bitstream parameters
                 n  - number of bits to be read

  Output       : y  - bits read

  Author(s)    : Juha Ojanpera
  *************************************************************************/

EXPORT_C uint32
BsGetBits(TBitStream *bs, int16 n)
{
  int16 rbits = 0;
  uint32 value = 0;

  BsSetBitsRead(bs, n);

  while(n)
  {
    rbits = (n > (int16)bs->slotBits) ? bs->slotBits : n;
    value <<= rbits;

    // modified by Ali Ahmaniemi 7.6.04

    if (bs->mode == BIT8)
        {
        value |= GetBits(bs, rbits);
        }
    else
        {
        value |= GetBitsDSP(bs, rbits);
        }

    //value |= GetFunc[bs->mode](bs, rbits);
    n -= rbits;
  }

  return (value);
}

/**************************************************************************
  Title        : BsSkipBits

  Purpose      : Advances the bit buffer index.

  Usage        : BsSkipBits(bs, n);

  Input        : bs - bitstream parameters
                 n  - number of bits to be discarded

  Explanation  : The maximum number of bits that can be discarded is 'bs->slotBits'.

  Author(s)    : Juha Ojanpera
  *************************************************************************/

EXPORT_C void
BsSkipBits(TBitStream *bs, int16 n)
{
  int16 idx;

#ifdef USE_ASSERT
  MY_ASSERT(n < ((int16)bs->slotBits + 1));
#endif

  BsSetBitsRead(bs, n);

  idx = bs->bit_counter - n;
  if(idx < 0)
  {
    BITSASSERT(bs);
    bs->bit_counter = bs->slotBits + idx;
  }
  else
    bs->bit_counter = idx;

  if(bs->bit_counter == 0)
  {
    BITSASSERT(bs);
    bs->bit_counter = bs->slotBits;
  }
}

/**************************************************************************
  Title        : BsSkipNbits

  Purpose      : Same as 'BsSkipBits' with the exception that the number 
                 of bits to be discarded can be of any value.

  Usage        : BsSkipNbits(bs, n);

  Input        : bs - bitstream parameters
                 n  - number of bits to be discarded

  Author(s)    : Juha Ojanpera
  *************************************************************************/

EXPORT_C void
BsSkipNBits(TBitStream *bs, int32 n)
{
  int32 slots, bits_left, scale;

#ifdef BYTE_16bit
  scale = (bs->mode == BIT8) ? 3 : 4;
#else
  scale = 3;
#endif  
  slots = (n >> scale);
  BsSetBitsRead(bs, slots << scale);
  bs->buf_index += slots;
  bs->buf_index = MOD_OPCODE(bs->buf_index, bs->buf_mask);
  bs->slots_read += slots;
  bits_left = n - (slots << scale);
  if(bits_left)
    BsSkipBits(bs, (int16) bits_left);
}

/**************************************************************************
  Title        : BsCopyBytes

  Purpose      : Copies 'bytesToCopy' bytes from the bit buffer (starting from
                 the current read index) to the specified output buffer.
  
  Usage        : y = BsCopyBytes(bs, outBuf, bytesToCopy)

  Input        : bs          - bitstream to be copied
                 bytesToCopy - # of bytes to copy

  Output       : y           - # of bytes copied
                 outBuf      - copied bytes

  Author(s)    : Juha Ojanpera
  *************************************************************************/

EXPORT_C uint32
BsCopyBytes(TBitStream *bs, uint8 *outBuf, uint32 bytesToCopy)
{
  uint32 i;
  uint8 *buf0 = (bs->mode == BIT8) ? bs->bit_buffer : bs->dsp_buffer;
  
  for(i = bs->buf_index; i < (bytesToCopy + bs->buf_index); i++, outBuf++)
    *outBuf = buf0[MOD_OPCODE(i, bs->buf_mask)];
  
  return (i - bs->buf_index);
}

/**************************************************************************
  Title        : BsCopyBits

  Purpose      : Copies 'bitsToCopy' bits from the source bit buffer (starting from
                 the current read index) to the specified destination bit buffer.
  
  Usage        : y = BsCopyBits(bsSrc, bsDst, bitsToCopy)

  Input        : bsSrc       - source bit buffer
                 bytesToCopy - # of bits to copy

  Output       : bsDst       - destination bit buffer

  Author(s)    : Juha Ojanpera
  *************************************************************************/

EXPORT_C void 
BsCopyBits(TBitStream *bsSrc, TBitStream *bsDst, int32 bitsToCopy)
{
  int32 i, nBytes;

  nBytes = bitsToCopy >> 3;

  for(i = 0; i < nBytes; i++)
    BsPutBits(bsDst, 8, BsGetBits(bsSrc, 8));

  i = bitsToCopy - (i << 3);
  if(i > 0) BsPutBits(bsDst, (int16) i, BsGetBits(bsSrc, (uint16) i));
}

/**************************************************************************
  Title        : BsRewindNBits

  Purpose      : Rewinds the bit buffer 'nBits' bits.

  Usage        : BsRewindNbits(br, nBits)

  Input        : br    - bitstream parameters
                 nBits - number of bits to rewind

  Author(s)    : Juha Ojanpera
  *************************************************************************/

EXPORT_C void
BsRewindNBits(TBitStream *bs, uint32 nBits)
{
  int32 tmp;
  int16 new_buf_idx, new_bit_idx;  

  new_buf_idx = (int16) (nBits / bs->slotBits);
  new_bit_idx = (int16) (nBits % bs->slotBits);

  tmp = bs->bit_counter + new_bit_idx;
  if(tmp > (int16)bs->slotBits)
  {
    new_buf_idx++;
    bs->bit_counter = tmp - bs->slotBits;
  }
  else
    bs->bit_counter += new_bit_idx;

  bs->buf_index = MOD_OPCODE(bs->buf_index, bs->buf_mask);
  tmp = bs->buf_index - new_buf_idx;
  if(tmp > 0)
    bs->buf_index = tmp;
  else
    bs->buf_index = MOD_OPCODE(bs->buf_len + tmp, bs->buf_mask);

  bs->bits_read -= nBits;
}

EXPORT_C void 
BsSaveBufState(TBitStream *bsSrc, TBitStream *bsDst)
{
  COPY_MEMORY(bsDst, bsSrc, sizeof(TBitStream));
}
