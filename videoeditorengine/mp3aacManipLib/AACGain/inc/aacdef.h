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
  aacdef.h - Interface to AAC core structures.
 
  Author(s): Juha Ojanpera
  Copyright (c) 2000-2004 by Nokia Research Center, Speech and Audio Systems.
  *************************************************************************/

#ifndef AACDEF_H_
#define AACDEF_H_

/*-- Project Headers. --*/
#include "chandefs.h"
#include "defines.h"
#include "dec_huf.h"

/*
   Purpose:     Structure interface for window parameters.
   Explanation: - */
class CWindowInfo : public CBase
{
public:

    static CWindowInfo* NewL();
    ~CWindowInfo();
    uint8 wnd;
    uint8 max_sfb;
    uint8 hasmask;
    uint8 predBands;
    PredType predType;
    int16 prstflag[2];
    Wnd_Shape wshape[2];
    uint8* group;//[NSHORT];
    uint8* mask;//[MAXBANDS];
    int16* sfac;//[MAXBANDS];
    uint8* cb_map;//[MAXBANDS];
    int16* lpflag;//[MAXBANDS];

private:

    void ConstructL();
    CWindowInfo();

};

/*
   Purpose:     Information about the audio channel.
   Explanation: - */
class TCh_Info
{
public:

    int16 present;         /* Channel present.                              */
    int16 num_bins;        /* # of active (i.e., nonzero) bins for this ch. */
    int16 tag;             /* Element tag.                                  */
    int16 cpe;             /* 0 if single channel, 1 if channel pair.       */
    int16 paired_ch;       /* Index of paired channel in cpe.               */
    int16 widx;            /* Window element index for this channel.        */
    
    BOOL is_present;       /* Intensity stereo is used.                     */
    BOOL pns_present;      /* PNS is used.                                  */
    BOOL tns_present;      /* TNS is used.                                  */
    BOOL parseCh;          /* TRUE if channel only parsed not decoded.      */
    
    int16 ncch;            /* Number of coupling channels for this ch.      */
    int16 cch[CChansD];    /* Coupling channel idx.                         */
    int16 cc_dom[CChansD]; /* Coupling channel domain.                      */
    int16 cc_ind[CChansD]; /* Independently switched coupling channel flag. */
    CInfo *info;            /* Block parameters for this channel.            */
    
    /*-- Huffman tables. --*/
    Huffman_DecInfo **huf;
    Huffman_DecSfInfo *sf_huf;
    
};


/*
   Purpose:     Channel mapping information.
   Explanation: - */
class CMC_Info : public CBase
{
public:

    static CMC_Info* NewL();
    ~CMC_Info();
    /*
    * Max number of supported main and coulping channels.
    */
    int16 maxnCh;
    int16 maxnCCh;

    /*
    * Audio channels (LFE, SCE, and CPE) up to 'maxnCh' will be decoded.
    * All the other channels will be only parsed. 'dummyCh' is therefore
    * the channel index into 'ch_info' structure which is used for the unused
    * audio channels. 'dummyCCh' identifies the channel index for unused CCE
    * channels.
    */
    int16 dummyCh;
    int16 dummyCCh;
    
    /*
    * This will be set to 1 when the channel limit has been reached.
    */
    int16 dummyAlways;
    
    /*
    * These members identify how many audio channels (LFE, SCE, CPE, CCE)
    * were found from the bitstream on a frame-by-frame basis.
    */
    int16 nch;
    int16 ncch;

    int16 cc_tag[1 << LEN_TAG]; /* Tags of valid CCE's.                           */
    int16 cc_ind[1 << LEN_TAG]; /* Independently switched CCE's.                  */
    uint8 profile;
    uint8 sfreq_idx;
    
    int16 cur_prog;
    int16 default_config;
    
    CSfb_Info* sfbInfo;
    
    TCh_Info ch_info[ChansD];

private:

    void ConstructL();
    CMC_Info();


};

/*
   Purpose:     Pulse noiseless coding.
   Explanation: - */
typedef struct PulseInfoStr
{
  int16 number_pulse;
  int16 pulse_start_sfb;
  int16 pulse_data_present;
  int16 pulse_amp[NUM_PULSE_LINES];
  int16 pulse_offset[NUM_PULSE_LINES];
  int16 pulse_position[NUM_PULSE_LINES];

} PulseInfo;

#endif /*-- AACDEF_H_ --*/
