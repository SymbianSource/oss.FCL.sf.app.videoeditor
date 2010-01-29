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
  dec_huftables.cpp - Huffman tables for AAC decoder.
 
  Author(s): Juha Ojanpera
  Copyright (c) 2000-2004 by Nokia Research Center, Speech and Audio Systems.
  *************************************************************************/

/*-- Project Headers. --*/
#include "dec_huf.h"
#include "defines.h"
#include "dec_const.h"

/**************************************************************************
   Internal Objects
  *************************************************************************/

/*
   Purpose:      Number of Huffman items within each codebook.
   Explanation:  - */


/*
 * Deletes resources allocated to the Huffman codebooks.
 */
void
CloseHuffmanDecTables(Huffman_DecInfo **huf)
{
  if(huf)
  {
    int16 i;
    
    for(i = 0; i < MAX_AAC_QHUFTABLES; i++)
    {
      if(huf[i] != 0)
        delete huf[i];
      huf[i] = NULL;
    }
    
    delete[] huf;
    huf = NULL;
  }
}

/**************************************************************************
  Title:      LoadHuffmanDecTablesL

  Purpose:    Loads Huffman spectral codebooks for AAC decoder.

  Usage:      y = LoadHuffmanDecTablesL()

  Output:     y - spectral codebooks

  Author(s):  Juha Ojanpera
  *************************************************************************/

Huffman_DecInfo **
LoadHuffmanDecTablesL(void)
{

    /*
  Purpose:      Array holding the AAC Huffman decoding codebooks.
  Explanation:  - */
const Huffman_DecCode *dec_huffman_tables[] = {
  dec_hftable1, dec_hftable2, dec_hftable3, dec_hftable4, dec_hftable5, 
  dec_hftable6, dec_hftable7, dec_hftable8, dec_hftable9, dec_hftable10, 
  dec_hftable11
};


  int16 i;
  Huffman_DecInfo **huf;
  const Huffman_DecCode **hf_code;

  huf = (Huffman_DecInfo **) new (ELeave) Huffman_DecInfo*[MAX_AAC_QHUFTABLES];
  CleanupStack::PushL(huf);

  ZERO_MEMORY(huf, MAX_AAC_QHUFTABLES * sizeof(Huffman_DecInfo *));
  
  hf_code = dec_huffman_tables;
  for(i = 0; i < MAX_AAC_QHUFTABLES; i++)
  {
    huf[i] = (Huffman_DecInfo *) new (ELeave) Huffman_DecInfo[1];
    CleanupStack::PushL(huf[i]);

    ZERO_MEMORY(huf[i], sizeof(Huffman_DecInfo));
    
    huf[i]->huf = hf_code[i];
    
    huf[i]->cb_len = cb_len[i];
  }

  CleanupStack::Pop(MAX_AAC_QHUFTABLES + 1);
  
  return (huf);
}

/*
 * Deletes resources allocated to the Huffman scalefactor codebook.
 */
void
CloseSfHuffmanTable(Huffman_DecSfInfo *sfHuf)
{
  if(sfHuf)
  {
    delete sfHuf;
    sfHuf = NULL;
  }
}

/**************************************************************************
  Title:      LoadSfHuffmanTableL

  Purpose:    Loads Huffman scalefactor codebook for AAC decoder.

  Usage:      y = LoadSfHuffmanTableL()

  Output:     y - scalefactor codebook

  Author(s):  Juha Ojanpera
  *************************************************************************/

Huffman_DecSfInfo *
LoadSfHuffmanTableL(void)
{
  Huffman_DecSfInfo *huf;

  huf = (Huffman_DecSfInfo *) new (ELeave) Huffman_DecSfInfo[1];

  ZERO_MEMORY(huf, sizeof(Huffman_DecSfInfo));

  huf->cb_len = cb_len[11];

  huf->sf_param = dec_hftable12;

  return (huf);
}
