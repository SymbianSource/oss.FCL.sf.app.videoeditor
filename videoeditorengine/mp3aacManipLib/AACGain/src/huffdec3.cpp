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
  huffdec3.cpp - Huffman decoding and inverse scaling routines for AAC decoder.
 
  Author(s): Juha Ojanpera
  Copyright (c) 2000-2004 by Nokia Research Center, Speech and Audio Systems.
  *************************************************************************/

/*-- Projetc Headers. --*/
#include "tool2.h"
#include "dec_huf.h"

/**************************************************************************
  Title:       DecodeSpectralCodeword

  Purpose:     Decodes one spectral Huffman codeword from the bitstream.

  Usage:       y = DecodeSpectralCodeword(huf_info, bs)

  Input:       huf_info - Huffman codebook parameters
               bs       - bitstream parameters

  Output:      y        - decoded Huffman index

  Author(s):   Juha Ojanpera
  *************************************************************************/

INLINE uint32
DecodeSpectralCodeword(Huffman_DecInfo *huf_info, TBitStream *bs)
{
  int16 len;
  int32 codeword, items;
  const Huffman_DecCode *huf_code;
  
  items = 1;
  huf_code = huf_info->huf;

  /*-- The first 5 LSB bits contain the length of codeword. --*/
  codeword = (int32) BsGetBits(bs, (int16)(huf_code->huf_param & 31));
  
  while(items < huf_info->cb_len && codeword != huf_code->codeword)
  {
    items++;
    huf_code++;
    
    len = (int16) (huf_code->huf_param & 31);
    if(len)
    {
      codeword <<= len;
      codeword |= BsGetBits(bs, len);
    }
  }

  /*-- Remove the length part from the decoded symbol. --*/
  return (huf_code->huf_param >> 5);
}

const uint32 hCbBitMask[20] =
{0x0, 0x1, 0x3, 0x7, 0xF, 0x1F, 0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF,
0x1FFFL, 0x3FFFL, 0x7FFFL, 0xFFFFL, 0x1FFFFL, 0x3FFFFL, 0x7FFFFL};

/**************************************************************************
  Title:        DecodeSfCodeword

  Purpose:      Decodes one scalefactor Huffman codeword from the bitstream.

  Usage:        y = DecodeCodeword(huf_info, bs)

  Input:        huf_info - Huffman codebook parameters
                bs       - bitstream parameters

  Output:       y        - decoded Huffman index

  Explanation:  The Huffman parameters are packed as follows :

                bits 0  -  4 : Length of codeword
                     5  - 23 : Huffman codeword
                     24 - 31 : Huffman symbol

  Author(s):    Juha Ojanpera
  *************************************************************************/

INLINE uint32
DecodeSfCodeword(Huffman_DecSfInfo *huf_info, TBitStream *bs)
{
#define SFCW_MASK ((1 << 19) - 1)
#define EXTRACT_SFCW(x) ((x >> 5) & SFCW_MASK)
  int16 bitsLeft;
  uint32 lookaheadBits;
  int32 codeword, len, items;
  const uint32 *sf_param = huf_info->sf_param;
  
  items = 1;

  /*-- Maximum codeword length is 19 bits! --*/
  lookaheadBits = BsLookAhead(bs, 19);
  bitsLeft = (int16) (19 - (*sf_param & 31));
  codeword = lookaheadBits >> bitsLeft;
  
  while(items < huf_info->cb_len && codeword != (int32) EXTRACT_SFCW(*sf_param))
  {
    items++;
    sf_param++;
    
    len = (int16) (*sf_param & 31);
    if(len)
    {
      bitsLeft -= len;
      codeword <<= len;
      codeword |= (lookaheadBits >> bitsLeft) & hCbBitMask[len];
    }
  }

  BsSkipNBits(bs, 19 - bitsLeft);
  
  return (*sf_param >> 24);
}

uint32 
GetHcb(Huffman_DecSfInfo *huf_info, TBitStream *bs)
{
  return (DecodeSfCodeword(huf_info, bs));
}

/**************************************************************************
  Title:        huf_sfac

  Purpose:      Reads scalefactors from the bitstream.

  Usage:        y = huf_sfac(bs, cip, group, cb_map, global_gain, factors, max_sfb)

  Input:        bs          - bitstream parameters
                cip :
                 info       - block (long/short) parameters
                group       - grouping info
                cb_map      - codebook for each sfb
                global_gain - start value for the difference decoding of scalefactors
                max_sfb     - max # of sfb's present in this channel

  Output:       y           - TRUE on success, FALSE otherwise
                cip :
                 is_present - TRUE if IS stereo is used, FALSE otherwise

  Author(s):    Juha Ojanpera
  *************************************************************************/

int16
huf_sfac(TBitStream *bs, TCh_Info *cip, uint8 *group, uint8 *cb_map,
         int16 global_gain, int16 *factors, uint8 max_sfb)
{
  Huffman_DecSfInfo *sf_huf;
  int16 i, b, bb, n, is_pos;
  int16 noise_pcm_flag, noise_nrg;

  sf_huf = cip->sf_huf;
  
  noise_pcm_flag = 1;

  /* 
   * Scale factors are dpcm relative to global gain,
   * intensity positions are dpcm relative to zero
   */
  is_pos = 0;
  noise_nrg = (int16) (global_gain - NOISE_OFFSET);
  cip->is_present = cip->pns_present = FALSE;

  /*-- Get scale factors. --*/
  n = cip->info->sfb_per_sbk[0];
  for(b = 0, bb = 0; b < cip->info->nsbk; )
  {
    b = *group++;
    for(i = 0; i < max_sfb; i++)
    {
      switch(cb_map[i])
      {
        /*-- Zero book. --*/
        case ZERO_HCB:
          factors[i] = 0;
          break;

        /*-- Invalid books. --*/
        case BOOKSCL:
          return (FALSE);
  
        /*-- Intensity books. --*/
        case INTENSITY_HCB:
        case INTENSITY_HCB2:
          cip->is_present = TRUE;
          is_pos = (int16) (is_pos + DecodeSfCodeword(sf_huf, bs) - MIDFAC);
          factors[i] = is_pos;          
          break;

        /*-- Noise books. --*/
        case NOISE_HCB:
          cip->pns_present = TRUE;
          if(noise_pcm_flag)
          {
            noise_pcm_flag = 0;
            noise_nrg = (int16) (noise_nrg + BsGetBits(bs, NOISE_PCM_BITS) - NOISE_PCM_OFFSET);
          }
          else
            noise_nrg = (int16) (noise_nrg + DecodeSfCodeword(sf_huf, bs) - MIDFAC);

          factors[i] = noise_nrg;
          break;

        /*-- Spectral books. --*/
        default:
          global_gain = (int16) (global_gain + DecodeSfCodeword(sf_huf, bs) - MIDFAC);
          factors[i] = (int16) (global_gain & TEXP_MASK);
          break;
      }
    }
    
    /*-- Expand short block grouping. --*/
    if(!cip->info->islong)
      for(bb++; bb < b; bb++)
      {
        COPY_MEMORY(factors + n, factors, n * sizeof(int16));
        factors += n;
      }
    cb_map += n; factors += n;
  }

  return (TRUE);
}

/**************************************************************************
  Title:        unpack_index...

  Purpose:      Translates Huffman index into n-tuple spectral values.

  Usage:        unpack_index(index, quant)

  Input:        index - decoded Huffman index

  Output:       quant - quantized spectral values

  Explanation:  The index contains already the translated n-tuple spectral 
                values due to computational reasons. The unpacking routines
                will only extract the codebook dependent bit fields within
                'index' to obtain the quantized values. 

  Author(s):    Juha Ojanpera
  *************************************************************************/

/*
 * 2-tuple, quantized values are unsigned.
 */
INLINE void
unpack_index2noOff(uint32 index, int16 *quant)
{
  quant[0] = (index >> 5) & 31; quant[1] = index & 31;
}

/*
 * 2-tuple, quantized values are signed.
 */
INLINE void
unpack_index2Off(uint32 index, int16 *quant)
{
  quant[0] = (index >> 4) & 7; if(index & 128) quant[0] = -quant[0];
  quant[1] = index & 7;        if(index & 8)   quant[1] = -quant[1];
}

/*
 * 4-tuple, quantized values are unsigned.
 */
INLINE void
unpack_index4noOff(uint32 index, int16 *quant)
{
  quant[0] = (index >> 6) & 3; quant[1] = (index >> 4) & 3;
  quant[2] = (index >> 2) & 3; quant[3] = index & 3;
}

/*
 * 4-tuple, quantized values are signed.
 */
INLINE void
unpack_index4Off(uint32 index, int16 *quant)
{
  quant[0] = (index >> 6) & 1; if(index & 128) quant[0] = -quant[0];
  quant[1] = (index >> 4) & 1; if(index & 32)  quant[1] = -quant[1];
  quant[2] = (index >> 2) & 1; if(index & 8)   quant[2] = -quant[2];
  quant[3] = index & 1;        if(index & 2)   quant[3] = -quant[3];
}

/*
 * Reads, at maximum, 2 sign bits from the bitstream.
 */
INLINE void
get_sign_bits2(int16 *q, TBitStream *bs)
{
  /*-- 1 signals negative, as in 2's complement. --*/
  if(q[0]) { if(BsGetBits(bs, 1)) q[0] = -q[0]; }
  if(q[1]) { if(BsGetBits(bs, 1)) q[1] = -q[1]; }
}

/*
 * Reads, at maximum, 4 sign bits from the bitstream.
 */
INLINE void
get_sign_bits4(int16 *q, TBitStream *bs)
{
  /*-- 1 signals negative, as in 2's complement. --*/
  if(q[0]) { if(BsGetBits(bs, 1)) q[0] = -q[0]; }
  if(q[1]) { if(BsGetBits(bs, 1)) q[1] = -q[1]; }
  if(q[2]) { if(BsGetBits(bs, 1)) q[2] = -q[2]; }
  if(q[3]) { if(BsGetBits(bs, 1)) q[3] = -q[3]; }
}

/**************************************************************************
  Title:        getescape

  Purpose:      Decodes escape sequences for codebook 11.

  Usage:        y = getescape(bs, q)

  Input:        bs - bitstream parameters
                q  - decoded quantized value (0-16)

  Output:       y - actual quantized value

  Author(s):    Juha Ojanpera
  *************************************************************************/

INLINE int16
getescape(TBitStream *bs, int16 q)
{
  int16 i;
  
  i = q;

  /*-- Value 16 is used to indicate that the actual value is escape coded. --*/
  if(q == 16 || q == -16)
  {
    /*
     * The length of escape sequence, according to the AAC standard,
     * is less than 24 bits.
     */
    for(i = 4; i < 24; i++)
      if(BsGetBits(bs, 1) == 0)
        break;
    
    i = (int16) (BsGetBits(bs, i) + (1 << i));
    if(q < 0)
      i = -i;
  }
  
  return (i);
}

/**************************************************************************
  Title:        huf_spec

  Purpose:      Huffman decodes and inverse scales quantized spectral values.

  Usage:        y = huf_spec(bs, info, nsect, sect, quant, huf, parseOnly)

  Input:        bs         - bitstream parameters
                info       - block (long/short) parameters
                nsect      - # of sections present in this channel
                sect       - sectioning (codebook and length of section) info        
                huf        - Spectral Huffman tables
                parseOnly  - 1 if bitstream need to be only parsed, 0 otherwise

  Output:       y          - # of spectral bins decoded
                quant      - quantized spectral coefficients

  Author(s):    Juha Ojanpera
  *************************************************************************/

int16
huf_spec(TBitStream *bs, CInfo *info, int16 nsect, uint8 *sect, int16 *quant, 
         Huffman_DecInfo **huf, uint8 parseOnly)
{
  uint32 temp;
  int16 i, j, k, stop, *qp;

  qp = quant;
  for(k = 0, i = nsect; i; i--, sect += 2)
  {
    register Huffman_DecInfo *huf_info;
    int16 table = sect[0];
    int16 top = sect[1];

    if(top > 200 || top < 1) goto error_exit;

    stop = info->bk_sfb_top[top - 1];

    switch(table)
    {
      /*-- 4-tuple signed quantized coefficients. --*/
      case 1:
      case 2:
        huf_info = huf[table - 1];
        for (j = k; j < stop; j += 4, qp += 4)
        {
          temp = DecodeSpectralCodeword(huf_info, bs);
          if(!parseOnly) unpack_index4Off(temp, qp);
        }
        break;

      /*-- 4-tuple unsigned quantized coefficients. --*/
      case 3:
      case 4:
        huf_info = huf[table - 1];    
        for (j = k; j < stop; j += 4, qp += 4)
        {
          temp = DecodeSpectralCodeword(huf_info, bs);
          unpack_index4noOff(temp, qp);
          get_sign_bits4(qp, bs);
        }
        break;
    
      /*-- 2-tuple unsigned quantized coefficients. --*/
      case 5:
      case 6:
        huf_info = huf[table - 1];
        for (j = k; j < stop; j += 2, qp += 2)
        {
          temp = DecodeSpectralCodeword(huf_info, bs);
          if(!parseOnly) unpack_index2Off(temp, qp);
        }
        break;
    
      /*-- 2-tuple signed quantized coefficients. --*/
      case 7:
      case 8:
      case 9:
      case 10:
        huf_info = huf[table - 1];
        for (j = k; j < stop; j += 2, qp += 2)
        {
          temp = DecodeSpectralCodeword(huf_info, bs);
          unpack_index2noOff(temp, qp);
          get_sign_bits2(qp, bs);
        }
        break;
    
      /*-- 2-tuple signed quantized coefficients (escape codebook). --*/
      case 11:
        huf_info = huf[table - 1];
        for (j = k; j < stop; j += 2, qp += 2)
        {
          temp = DecodeSpectralCodeword(huf_info, bs);
          unpack_index2noOff(temp, qp);
          get_sign_bits2(qp, bs);
          qp[0] = getescape(bs, qp[0]);
          qp[1] = getescape(bs, qp[1]);
        }
        break;
    
      default:
        if(stop > k)
        {
          if(!parseOnly) ZERO_MEMORY(qp, (stop - k) * sizeof(int16));
          qp = quant + stop;
        } 
        else goto error_exit;
        break;
    }
    k = stop;
  }

  if(!parseOnly) if(k < LN2) ZERO_MEMORY(quant + k, (LN2 - k) * sizeof(int16));

  return (k);

 error_exit:

  return (0);
}
