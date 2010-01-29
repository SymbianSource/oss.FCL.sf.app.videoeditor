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
  tool.h - Interface to AAC core structures.
 
  Author(s): Juha Ojanpera
  Copyright (c) 2000-2004 by Nokia Research Center, Speech and Audio Systems.
  *************************************************************************/

#ifndef    AACTOOL_H_
#define AACTOOL_H_

/*-- Project Headers. --*/
#include "nok_bits.h"
#include "dec_huf.h"
#include "aacdef.h"

/*
   Purpose:     LTP parameters for MPEG AAC.
   Explanation: - */
class CLTP_Info : public CBase
{

public:

    static CLTP_Info* NewL();
    ~CLTP_Info();

    /*-- Max. sfb's used for this channel. --*/
    int16 max_sfb;

    /*-- Codebook index. --*/
    uint8 cbIdx;

    /*-- Boolean flag to indicate the presence of LTP. --*/
    int16 ltp_present;

    /*-- LTP lag. --*/
    int16* delay;

    /*-- Prediction status for each sfb. --*/
    uint32 sfbflags[2];

private:

    CLTP_Info();
    void ConstructL();

};

/*
   Purpose:     Structure interface for AAC decoding tools.
   Explanation: - */
class CToolInfo : public CBase
{
public:

    static CToolInfo* NewL();
    ~CToolInfo();
    CLTP_Info *ltp;
    int16* quant;
 
private:

    void ConstructL();
    CToolInfo();

};

/*
   Purpose:     Structure interface for coupling channel.
   Explanation: - */
class CCInfo : public CBase
{

public:
    static CCInfo* NewL();
    ~CCInfo();

    CToolInfo* tool;
    CWindowInfo* winInfo;

private:
    CCInfo();
    void ConstructL();
  
};

/*
   Purpose:     Information about the audio channel.
   Explanation: - */
class TEleList
{
public:
  int16 num_ele;
  int16 ele_is_cpe[1 << LEN_TAG];
  int16 ele_tag[1 << LEN_TAG];

};

/*
   Purpose:     Mixing information for downmixing multichannel input
                into two-channel output.
   Explanation: - */
class TMIXdown
{
public:
  int16 present;
  int16 ele_tag;
  int16 pseudo_enab;

};


/*
   Purpose:     Program configuration element.
   Explanation: - */
class TProgConfig
{
public:
  int16 tag;
  int16 profile;
  int16 sample_rate_idx;

  BOOL pce_present;

  TEleList front;
  TEleList side;
  TEleList back;
  TEleList lfe;
  TEleList data;
  TEleList coupling;

  TMIXdown mono_mix;
  TMIXdown stereo_mix;
  TMIXdown matrix_mix;

  int16 num_comment_bytes;

};

#endif    /*-- AACTOOL_H_ --*/
