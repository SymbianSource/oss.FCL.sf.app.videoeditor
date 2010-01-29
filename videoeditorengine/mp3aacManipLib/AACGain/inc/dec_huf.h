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
  dec_huf.h - AAC Huffman decoding declarations.
 
  Author(s): Juha Ojanpera
  Copyright (c) 2000-2004 by Nokia Research Center, Speech and Audio Systems.
  *************************************************************************/

#ifndef DEC_HUFFMAN_H_
#define DEC_HUFFMAN_H_

/*-- Project Headers. --*/
#include "defines.h"

#define MAX_AAC_QHUFTABLES (11)

/*
  Purpose:      Structure defining Huffman codeword values for
                quantized spectral coefficients.
  Explanation:  - */

typedef struct Huffman_DecCodeStr
{
  uint16 huf_param;  /* Quantized spectral coefficients. */
  uint16 codeword;   /* Huffman codeword.                */
    
} Huffman_DecCode;

/*
  Purpose:      Structure defining Huffman codebook parameters for
                quantized spectral coefficients.
  Explanation:  - */
typedef struct Huffman_DecInfoStr
{
  int16 cb_len;               /* Codebook size.                         */
  const Huffman_DecCode *huf; /* Codeword parameters for this codebook. */
    
} Huffman_DecInfo;

/*
  Purpose:      Structure defining Huffman codebook parameters for
                scalefactors.
  Explanation:  - */
typedef struct Huffman_DecSfInfoStr
{
  int16 cb_len;           /* Codebook size.       */
  const uint32 *sf_param; /* Codeword parameters. */
    
} Huffman_DecSfInfo;



/*-- Following functions are implemented in module 'dec_huftables.c'. --*/
Huffman_DecInfo **LoadHuffmanDecTablesL(void);
void CloseHuffmanDecTables(Huffman_DecInfo **);
Huffman_DecSfInfo *LoadSfHuffmanTableL(void);
void CloseSfHuffmanTable(Huffman_DecSfInfo *sfHuf);

#endif /*-- DEC_HUFFMAN_H_ --*/
