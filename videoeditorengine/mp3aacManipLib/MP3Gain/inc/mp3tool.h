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
  mp3tool.h - Interface to MP3 core structures.
 
  Author(s): Juha Ojanpera
  Copyright (c) 2004 by Nokia Research Center, Audio-Visual Systems.
  *************************************************************************/

#ifndef    MP3TOOL_H_
#define MP3TOOL_H_

/*-- Project Headers. --*/
#include "nok_bits.h"
#include "mpaud.h"

/*
 * Low level implementations for the mp3 engine. 
 */

/*-- Implementations defined in module 'layer3.cpp'. --*/
BOOL III_get_side_info(CMPAudDec *mp, TBitStream *bs);
void III_get_scale_factors(CMP_Stream *mp, int16 gr, int16 ch);
void L3WriteSideInfo(TBitStream *bs, CIII_Side_Info *sideInfo, TMPEG_Header *header);

/*-- Implementations defined in module 'l3huffman.cpp'. */ 
void InitL3Huffman(CHuffman *huf);
int16 III_huffman_decode(CMPAudDec *mp, int16 gr, int16 ch, int32 part2);

/*-- Implementations defined in module 'l3sfb.cpp'. */ 
void III_SfbDataInit(CIII_SfbData *sfbData, TMPEG_Header *header);

#endif    /*-- MP3TOOL_H_ --*/
