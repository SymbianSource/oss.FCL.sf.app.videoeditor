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
  l3huffman.cpp - Huffman decoding subroutines for layer III.

  Author(s): Juha Ojanpera
  Copyright (c) 1999-2004 by Nokia Research Center, Speech and Audio Systems.
  *************************************************************************/

/**************************************************************************
  External Objects Needed
  *************************************************************************/

/*-- Project Headers --*/
#include "mpaud.h"
#include "mp3tables.h"

/**************************************************************************
  Internal Objects
  *************************************************************************/

/*
   Purpose:     Table to hold the Huffman symbol codebooks.
   Explanation: - 
const int16 *huff_symbols_tbl[] = {
NULL, huff_symbols1, huff_symbols2, huff_symbols3, NULL, huff_symbols5,
huff_symbols6, huff_symbols7, huff_symbols8, huff_symbols9, huff_symbols10,
huff_symbols11, huff_symbols12, huff_symbols13, NULL, huff_symbols15,
huff_symbols16, huff_symbols24, huff_symbols32};


   Purpose:     Table to hold the Huffman codeword codebooks.
   Explanation: - 
   
const int16 *huff_cwords_tbl[] = {
NULL, huff_cwords1, huff_cwords2, huff_cwords3, NULL, huff_cwords5,
huff_cwords6, huff_cwords7, huff_cwords8, huff_cwords9, huff_cwords10,
huff_cwords11, huff_cwords12, huff_cwords13, NULL, huff_cwords15, huff_cwords16,
huff_cwords24, huff_cwords32};
*/

/**************************************************************************
  Title        : InitL3Huffman

  Purpose      : Allocates resources for layer III Huffman decoding.

  Usage        : InitL3Huffman(huf)

  Output       : huf - Huffman tables

  Author(s)    : Juha Ojanpera
  *************************************************************************/

void
InitL3Huffman(CHuffman *huf)
{
  int16 i, j;


  /*
   Purpose:     Table to hold the Huffman symbol codebooks.
   Explanation: - */
const int16 *huff_symbols_tbl[] = {
NULL, huff_symbols1, huff_symbols2, huff_symbols3, NULL, huff_symbols5,
huff_symbols6, huff_symbols7, huff_symbols8, huff_symbols9, huff_symbols10,
huff_symbols11, huff_symbols12, huff_symbols13, NULL, huff_symbols15,
huff_symbols16, huff_symbols24, huff_symbols32};

/*
   Purpose:     Table to hold the Huffman codeword codebooks.
   Explanation: - */
const int16 *huff_cwords_tbl[] = {
NULL, huff_cwords1, huff_cwords2, huff_cwords3, NULL, huff_cwords5,
huff_cwords6, huff_cwords7, huff_cwords8, huff_cwords9, huff_cwords10,
huff_cwords11, huff_cwords12, huff_cwords13, NULL, huff_cwords15, huff_cwords16,
huff_cwords24, huff_cwords32};


  for(i = 0; i < 33; i++)
  {
    huf[i].tree_len = huf_tree_len[i];
    huf[i].linbits = linbits[i];
  }

  for(i = 0; i < 16; i++)
  {
    huf[i].codeword = huff_cwords_tbl[i];
    huf[i].packed_symbols = huff_symbols_tbl[i];
  }

  j = i;
  while(j < 24)
  {
    huf[j].codeword = huff_cwords_tbl[i];
    huf[j].packed_symbols = huff_symbols_tbl[i];
    j++;
  }

  i++;
  while(j < 32)
  {
    huf[j].codeword = huff_cwords_tbl[i];
    huf[j].packed_symbols = huff_symbols_tbl[i];
    j++;
  }

  i++;
  huf[j].codeword = huff_cwords_tbl[i];
  huf[j].packed_symbols = huff_symbols_tbl[i];
}


/**************************************************************************
  Title        : L3decode_codeword

  Purpose      : Decodes one Huffman codeword from the bitstream and returns
                 the corresponding symbol.

  Usage        : L3decode_codeword(br, h)

  Input        : br - bitstream parameters
                 h  - Huffman table to be utilized

  Explanation  : This decoding routine works only if the codewords are
                 sorted according to their length, i.e, the codeword that has
                 the minimum length appears first in the Huffman table, the
                 length of the second codeword in the table is either greater
                 or equal than the first one and so on.

  Author(s)    : Juha Ojanpera
  *************************************************************************/

INLINE int16
L3decode_codeword(TBitStream *br, CHuffman *h)
{
  const int16 *h_len = h->packed_symbols;
  const int16 *h_codeword = h->codeword;
  uint16 clen, items;
  uint16 shift, N = 19; /* N = 19 maximum codeword length */
  uint32 cand_codeword;
  uint32 codeword;

  /* Read the maximum codeword. */
  cand_codeword = BsLookAhead(br, N);

  /*
   * Length of the codeword that has the
   * highest probability.
   */
  shift = N - (*h_len & 31);

  /* Candidate codeword for the first codebook entry. */
  codeword = cand_codeword >> shift;

  /*
   * Sequentially goes through the whole Huffman table
   * until a match has been found.
   */
  items = 1;
  while(items < h->tree_len && codeword != (uint16)*h_codeword)
  {
    h_len++;
    h_codeword++;
    items++;

    /* How many bits the codeword need to be updated. */
    clen = *h_len & 31;
    if(clen)
    {
      shift -= clen;

      /* New candidate codeword. */
      codeword = cand_codeword >> shift;
    }
  }

  /* # of bits actually read. */
  BsSkipNBits(br, N - shift);

  /* Decoded Huffman symbol. */
  return (*h_len >> 5);
}

/**************************************************************************
  Layer III subroutines for decoding spectral samples.

  Author(s): Juha Ojanpera
  Copyright (c) 1999-2004 by Nokia Research Center, Speech and Audio Systems.
  *************************************************************************/

int16
III_huffman_decode(CMP_Stream *mp, int16 gr, int16 ch, int32 part2)
{
  int16 i, x, region1Start, region2Start, table_num, limit;
  int16 *quant, count1Len;
  TGranule_Info *gr_info;

  /* Quantized spectral samples. */
  quant = mp->frame->ch_quant[ch];

  /* Granule info for this frame. */
  gr_info = mp->side_info->ch_info[ch]->gr_info[gr];

  /* Find region boundaries. */
  region1Start = mp->side_info->sfbData->sfbLong[gr_info->region0_count + 1];
  region2Start = mp->side_info->sfbData->sfbLong[gr_info->region0_count +
                            gr_info->region1_count + 2];

  /*
   * How many samples actually need to be Huffman decoded.
   */
  limit = mp->side_info->sfbData->bandLimit;
  if(gr_info->big_values > limit)
    gr_info->big_values = limit;

  /* Read bigvalues area. */
  int16 section1 = MIN(gr_info->big_values, region1Start);
  if(section1 > 0)
  {
    pairtable(mp, section1, gr_info->table_select[0], quant);
    quant += section1;
  }

  int16 section2 = MIN(gr_info->big_values, region2Start) - region1Start;
  if(section2 > 0)
  {
    pairtable(mp, section2, gr_info->table_select[1], quant);
    quant += section2;
  }

  int16 section3 = gr_info->big_values - region2Start;
  if(section3 > 0)
  {
    pairtable(mp, section3, gr_info->table_select[2], quant);
    quant += section3;
  }

  count1Len = 0;
  i = gr_info->big_values;
  part2 += gr_info->part2_3_length;

  /* Check whether the samples between -1,...,1 need to be Huffman decoded. */
  if(i < limit)
  {
    /* Read count1 area. */
    table_num = 32 + ((gr_info->flags & COUNT_1_TABLE_SELECT) ? 1 : 0);

    x = quadtable(mp, i, part2, table_num, quant, limit);
    count1Len = x - i;
    quant += count1Len;
    count1Len >>= 2;
    i = x;
  }

  if(BsGetBitsRead(mp->br) > (uint32)part2)
  {
    count1Len--;
    quant -= 4; i -= 4;
    BsRewindNBits(mp->br, BsGetBitsRead(mp->br) - part2);
  }

  /* Dismiss stuffing Bits */
  if(BsGetBitsRead(mp->br) < (uint32)part2)
    BsSkipNBits(mp->br, part2 - BsGetBitsRead(mp->br));

  gr_info->zero_part_start = (i < limit) ? i : limit;

  return (count1Len);
}

void
pairtable(CMP_Stream *mp, int16 section_length, int16 table_num, int16 *quant)
{
  register CHuffman *h = &mp->huffman[table_num];

  if(h->tree_len == 0)
      Mem::Fill(quant, section_length << 1 /** sizeof(int16)*/, 0);
  else
  {
    if(h->linbits)
    {
      register int16 *q = quant;

      for(int16 i = 0; i < section_length; i += 2, q += 2)
      {
        uint32 codeword = L3decode_codeword(mp->br, h);

        /* Unpack coefficients. */
        *q = codeword >> 4;
        q[1] = codeword & 15;

        /* Read extra bits (if needed) and sign bits (if needed). */
        if(*q == 15)
          *q += BsGetBits(mp->br, h->linbits);
        if(*q)
          *q = (BsGetBits(mp->br, 1)) ? -*q : *q;

        if(q[1] == 15)
          q[1] += BsGetBits(mp->br, h->linbits);
        if(q[1])
          q[1] = (BsGetBits(mp->br, 1)) ? -q[1] : q[1];
      }
    }
    else /* no linbits */
    {
      register int16 *q = quant;

      for(int16 i = 0; i < section_length; i += 2, q += 2)
      {
        uint32 codeword = L3decode_codeword(mp->br, h);

        /* Unpack coefficients. */
        *q = codeword >> 4;
        q[1] = codeword & 15;

        /* Read extra bits (not needed) and sign bits (if needed). */
        if(*q)
          *q = (BsGetBits(mp->br, 1)) ? -*q : *q;

        if(q[1])
          q[1] = (BsGetBits(mp->br, 1)) ? -q[1] : q[1];
      }
    }
  }
}

int16
quadtable(CMP_Stream *mp, int16 start, int16 part2, int16 table_num, int16 *quant,
          int16 max_sfb_bins)
{
  int16 i;
  register int16 *q = quant;

  for(i = start; (BsGetBitsRead(mp->br) < (uint32)part2 && i < max_sfb_bins);
      i += 4, q += 4)
  {
    uint32 codeword;
    uint16 mask = 8, sbits = 0;

    if(table_num == 33)
      codeword = 15 - BsGetBits(mp->br, 4);
    else
      codeword = L3decode_codeword(mp->br, &mp->huffman[table_num]);

    /* Unpack coefficients. */
    *q = (codeword >> 3) & 1;
    q[1] = (codeword >> 2) & 1;
    q[2] = (codeword >> 1) & 1;
    q[3] = codeword & 1;

    /* Sign bits. */
    codeword = BsLookAhead(mp->br, 4);
    if(*q)   { sbits++;   *q = (codeword & mask) ?   -*q : *q;   mask >>= 1; }  
    if(q[1]) { sbits++; q[1] = (codeword & mask) ? -q[1] : q[1]; mask >>= 1; }
    if(q[2]) { sbits++; q[2] = (codeword & mask) ? -q[2] : q[2]; mask >>= 1; }
    if(q[3]) { sbits++; q[3] = (codeword & mask) ? -q[3] : q[3]; mask >>= 1; }
    if(sbits) BsSkipBits(mp->br, sbits);
  }

  return (i);
}
