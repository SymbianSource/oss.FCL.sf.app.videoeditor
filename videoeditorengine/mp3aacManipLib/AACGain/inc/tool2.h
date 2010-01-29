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
  tool2.h - Interface to AAC decoding functions.
 
  Author(s): Juha Ojanpera
  Copyright (c) 2003-2004 by Nokia Research Center, Speech and Audio Systems.
  *************************************************************************/

#ifndef    AAC_FUNC_H_
#define AAC_FUNC_H_

/*-- Project Headers. --*/
#include "aacaud.h"
#include "tool.h"

/*
 * Sfb initialization. 
 */
void
AACSfbInfoInit(CSfb_Info *sfb, uint8 sIndex, uint8 is960);

int32
AACSampleRate(uint8 sampleRateIdx);


/*
 * Control of channel configuration. 
 */
int16
CCChIndex(CMC_Info *mip, int16 cpe, int16 tag);

int16
ChIndex(int16 nch, int16 tag, int16 wnd, CMC_Info *mip);


/*
 * AAC syntactic channel elements. 
 */
int16 
GetSCE(CAACAudDec *aac, TBitStream *bs, CMC_Info *mip, uint8 *gains, 
       uint32 *gainPos, uint32 bufBitOffset);

int16 
GetCPE(CAACAudDec *aac, TBitStream *bs, CMC_Info *mip, uint8 *gains, 
       uint32 *gainPos, uint32 bufBitOffset);

int16 
GetCCE(CAACAudDec *aac, TBitStream *bs, CMC_Info *mip, CCInfo **ccInfo);

int16 
GetICS(TBitStream *bs, TCh_Info *cip, uint8 *group, uint8 max_sfb, 
       uint8 *cb_map, int16 *quant, int16 global_gain, 
       int16 *factors);

int16
GetICSInfo(TBitStream *bs, CWindowInfo *winInfo, CLTP_Info *ltp_left, CLTP_Info *ltp_right);


/*
 * Huffman decoding interfaces. 
 */
uint32 
GetHcb(Huffman_DecSfInfo *huf_info, TBitStream *bs);

int16
huf_sfac(TBitStream *bs, TCh_Info *cip, uint8 *group, uint8 *cb_map,
         int16 global_gain, int16 *factors, uint8 max_sfb);

int16
huf_spec(TBitStream *bs, CInfo *info, int16 nsect, uint8 *sect, 
         int16 *quant, Huffman_DecInfo **huf, uint8 parseOnly);


/*
 * Global gain element extraction and storage.
 */
int16
GetSCEGain(CAACAudDec *aac, TBitStream *bs, uint8 *gains, 
           uint32 *gainPos, uint32 bufBitOffset);

int16
GetCPEGain(CAACAudDec *aac, TBitStream *bs, uint8 *gains, 
           uint32 *gainPos, uint32 bufBitOffset);

#endif    /*-- AAC_FUNC_H_ --*/
