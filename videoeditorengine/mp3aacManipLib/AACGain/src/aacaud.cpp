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
  aacaud.cpp - High level interface implementations for AAC decoder.
 
  Author(s): Juha Ojanpera
  Copyright (c) 2000-2004 by Nokia Research Center, Speech and Audio Systems.
  *************************************************************************/

/*-- System Headers. --*/
#include <math.h>

/*-- Project Headers. --*/
#include "aacaud.h"
#include "dec_huf.h"
#include "tool2.h"

CAACAudDec* CAACAudDec::NewL(int16 aNumCh, int16 aNumCCh)
    {
    CAACAudDec* self = new (ELeave) CAACAudDec();
    CleanupStack::PushL(self);
    self->ConstructL(aNumCh, aNumCCh);
    CleanupStack::Pop(self);
    return self;
    }

void CAACAudDec::ConstructL(int16 aNumCh, int16 aNumCCh)
    {

    //----------------->

    int16 i, tmp;

    numCh = aNumCh;
    numCCh = aNumCCh;

    /*-- Set the limit for the number of decoded audio elements. --*/
    if(numCh > ChansD - XChansD)
        numCh = ChansD - XChansD;

    /*-- How many coupling channels are accepted ? --*/
    if(numCCh > CChansD - XCChansD)
        numCCh = CChansD - XCChansD;

    /*-- # of channels outputted. --*/
    numOutCh = numCh;

    /*-- Allocate main channel elements. --*/
    tmp = (int16) (numCh + XChansD);
    
    winInfo = new (ELeave) CWindowInfo*[tmp];
    tool = new (ELeave) CToolInfo*[tmp];

    windowAmount = tmp;

    for(i = 0; i < tmp; i++)
        {
        winInfo[i] = CWindowInfo::NewL();
        tool[i] = CToolInfo::NewL();
        }

    /*-- Allocate coupling channel elements. --*/
    tmp = (int16) (numCCh + XCChansD);
    ccInfo = new (ELeave) CCInfo*[tmp];
  
    for(i = 0; i < tmp; i++)
        {
        ccInfo[i] = CCInfo::NewL();
        }

    /*-- Get spectral codebooks parameters. --*/
    huf = LoadHuffmanDecTablesL();
  
    /*-- Get scalefactor codebook parameters. --*/
    sf_huf = LoadSfHuffmanTableL();

    mc_info = CMC_Info::NewL();
  
    for(i = 0; i < ChansD; i++)
        {
        mc_info->ch_info[i].huf = huf;
        mc_info->ch_info[i].sf_huf = sf_huf;
        }

    //<------------------

    }

CAACAudDec::CAACAudDec() : tool(0), mc_info(0), ccInfo(0), winInfo(0),
                           huf(0), sf_huf(0)
    {
#ifdef EAACPLUS_DECODER
    sbrStream = NULL;
    sbrDecInfo = NULL;
#endif /*-- EAACPLUS_DECODER --*/
    }

CAACAudDec::~CAACAudDec()
    {

    /*-- Allocate main channel elements. --*/
    TInt tmp = (int16) (numCh + XChansD);
    TInt i = 0;
    if (winInfo != 0)
        {
        for(i = 0; i < tmp; i++)
            {
            if (winInfo[i] != 0) delete winInfo[i];
            if (tool[i] != 0) delete tool[i];
            }

        delete[] winInfo;
        }
    
    if (tool != 0) delete[] tool;


    /*-- Allocate coupling channel elements. --*/
    tmp = (int16) (numCCh + XCChansD);
    
    if (ccInfo != 0)
        {
        for(i = 0; i < tmp; i++)
            {
            if (ccInfo[i] != 0) delete ccInfo[i];
            }
        }

    if (ccInfo != 0) delete[] ccInfo;
  
    CloseHuffmanDecTables(huf); huf = NULL;
    CloseSfHuffmanTable(sf_huf); sf_huf = NULL;

    if (mc_info != 0) delete mc_info;


  sbrStream = CloseSBRBitStream(sbrStream);
  sbrDecInfo = CloseSBR(sbrDecInfo);

  }

/*
 * Prepares Ch_Info structure for given audio element. The return value is
 * channel index into 'mip->ch_into' structure.
 */
static INLINE int16
enter_chn(int16 nch, int16 tag, int16 common_window, CMC_Info *mip)
{
  TCh_Info *cip;
  BOOL parseCh;
  int16 cidx = 0;

  /*-- Build configuration. --*/
  if(mip->nch + nch > (mip->maxnCh + 1) || mip->dummyAlways)
  {
    parseCh = FALSE;
    cidx = mip->dummyCh;
    mip->dummyAlways = TRUE;
  }
  else
  {
    parseCh = TRUE;
    cidx = mip->nch;
    mip->nch = (int16) (mip->nch + nch);
  }

  if(nch == 1) /*-- SCE. --*/
  {
    cip = &mip->ch_info[cidx];

    cip->cpe = 0;
    cip->ncch = 0;
    cip->tag = tag;
    cip->widx = cidx;
    cip->present = 1;
    cip->paired_ch = cidx;
    cip->parseCh = parseCh;
  }
  else         /*-- CPE. --*/
  {
    /*-- Left. --*/
    cip = &mip->ch_info[cidx];
    cip->cpe = 1;
    cip->ncch = 0;
    cip->tag = tag;
    cip->widx = cidx;
    cip->present = 1;
    cip->parseCh = parseCh;
    cip->paired_ch = (int16) (cidx + 1);

    /*-- Right. ---*/
    cip = &mip->ch_info[cidx + 1];
    cip->cpe = 1;
    cip->ncch = 0;
    cip->tag = tag;
    cip->present = 1;
    cip->paired_ch = cidx;
    cip->parseCh = parseCh;
    cip->widx = (common_window) ? (int16) cidx : (int16) (cidx + 1);
  }

  return (cidx);
}

/*
 * Retrieve appropriate channel index for the program and decoder configuration.
 */
int16
ChIndex(int16 nch, int16 tag, int16 wnd, CMC_Info *mip)
{
  /*
   * Channel index to position mapping for 5.1 configuration is :
   *  0 center
   *  1 left  front
   *  2 right front
   *  3 left surround
   *  4 right surround
   *  5 lfe
   */
  return (enter_chn(nch, tag, wnd, mip));
}

/*
 * Given cpe and tag, returns channel index of SCE or left channel in CPE.
 */
int16
CCChIndex(CMC_Info *mip, int16 cpe, int16 tag)
{
  int16 ch;
  TCh_Info *cip = &mip->ch_info[0];

  for(ch = 0; ch < mip->nch; ch++, cip++)
    if(cip->cpe == cpe && cip->tag == tag)
      return (ch);

  /*
   * No match, so channel is not in this program. Just parse the channel(s).
   */
  cip = &mip->ch_info[mip->dummyCh];
  cip->cpe = cpe;
  cip->widx = mip->dummyCh;
  cip->parseCh = TRUE;

  return (mip->dummyCh);
}

/* 
 * Checks continuity of configuration from one block to next.
 */
static INLINE void
ResetMCInfo(CMC_Info *mip)
{
  /*-- Reset channels counts. --*/
  mip->nch = 0;
  mip->ncch = 0;
  mip->dummyAlways = 0;
}

/*
 * Deletes resources allocated to the specified AAC decoder.
 */
EXPORT_C CAACAudDec *
DeleteAACAudDec(CAACAudDec *aac)
{ 
  if(aac)
  {
    delete (aac);
    aac = 0;
  }
  
  return (NULL);
}

/*
 * Creates handle to AAC decoder core. The input parameters are
 * the number of main ('numCh') and coupling ('numCCh') channels 
 * to be supported.
 *
 * Return AAC decoder handle on success, NULL on failure.
 */
EXPORT_C void
CreateAACAudDecL(CAACAudDec*& aDecHandle, int16 numCh, int16 numCCh)
{
    aDecHandle = CAACAudDec::NewL(numCh, numCCh);

    return;
}

/*
 * Creates handle to eAAC+ decoder.
 */
EXPORT_C uint8
CreateAACPlusAudDecL(CAACAudDec *aDecHandle, int16 sampleRateIdx, uint8 isStereo, uint8 isDualMono)
{

  int32 sampleRate = AACSampleRate(sampleRateIdx);

  aDecHandle->sbrStream = OpenSBRBitStreamL();
  aDecHandle->sbrDecInfo = OpenSBRDecoderL(sampleRate, 1024, isStereo, isDualMono);

  return (1);


}

/*
 * Prepares AAC core engine for decoding. The input parameters are the 
 * AAC profile ID (or object type in case MPEG-4 AAC) and the sampling 
 * rate index.
 */
EXPORT_C void
InitAACAudDec(CAACAudDec *aac, int16 profile, int16 sampleRateIdx, uint8 is960)
{
  int16 i, j;
  CMC_Info *mip;
  PredType predType;

  mip = aac->mc_info;

  mip->profile = (uint8) profile;
  mip->sfreq_idx = (uint8) sampleRateIdx;
  
  mip->cur_prog = -1;
  mip->default_config = 1;

  /*
   * Set channel restrictions so that we know how to handle 
   * unused channel elements.
   */
  mip->maxnCh = aac->numCh;
  mip->dummyCh = mip->maxnCh;
  mip->maxnCCh = aac->numCCh;
  mip->dummyCCh = mip->maxnCCh;

  /*-- Initialize sfb parameters. --*/
  AACSfbInfoInit(mip->sfbInfo, mip->sfreq_idx, is960);

  ResetAACAudDec(aac);

  /*-- How many bands used for prediction (BWAP or LTP). --*/
  if(profile == LTP_Object)
  {
    predType = LTP_PRED;
    j = LTP_MAX_PRED_BANDS;
  }
  else
  {
    j = 0;
    predType = NO_PRED;
  }
  
  for(i = 0; i < aac->numCh + XChansD; i++)
  {
    aac->winInfo[i]->predBands = (uint8) j;
    aac->winInfo[i]->predType = predType;
  }
  for(i = 0; i < aac->numCCh + XCChansD; i++)
  {
    aac->ccInfo[i]->winInfo->predBands = (uint8) j;
    aac->ccInfo[i]->winInfo->predType = predType;
  }

  aac->samplesPerFrame = (is960) ? (int16) LN2_960 : (int16) LN2;
}

/*
 * Resets internal members from the specified AAC decoder core.
 */
EXPORT_C void
ResetAACAudDec(CAACAudDec *aac)
{
  int16 i;
  CMC_Info *mip;
  CWindowInfo *winInfo;

  mip = aac->mc_info;

  /*-- Reset some modules. --*/
  ResetMCInfo(mip);

  /*
   * Assume that the first window shape is of type Kaiser-Bessel
   * Derived (KBD). If not, then we use it anyway. The mismatch
   * in the first frame is not of prime importance.
   */

  for(i = 0; i < aac->numCh; i++)
  {    
    //tool = aac->tool[i];
    winInfo = aac->winInfo[i];

    winInfo->wshape[0].prev_bk = WS_KBD;
    winInfo->wshape[1].prev_bk = WS_KBD;
  }

  for(i = 0; i < aac->numCCh; i++)
  {
    //tool = aac->ccInfo[i]->tool;

    aac->ccInfo[i]->winInfo->wshape[0].prev_bk = WS_KBD;
    aac->ccInfo[i]->winInfo->wshape[1].prev_bk = WS_KBD;
  }
}

/*
 * Reads data stream element from the specified bitstream.
 * 
 * Returns # of read bits.
 */
static INLINE int16
GetDSE(TBitStream *bs)
{
  int16 align_flag, cnt, bitsRead;
  
  bitsRead = (int16) BsGetBitsRead(bs);
  
  // read tag
  BsGetBits(bs, LEN_TAG);
  align_flag = (int16) BsGetBits(bs, LEN_D_ALIGN);
  cnt = (int16) BsGetBits(bs, LEN_D_CNT);

  if(cnt == (1 << LEN_D_CNT) - 1)
    cnt = (int16) (cnt + BsGetBits(bs, LEN_D_ESC));
  
  if(align_flag) BsByteAlign(bs);

  BsSkipNBits(bs, cnt << 3);

  bitsRead = (int16) (BsGetBitsRead(bs) - bitsRead);

  return (bitsRead);
}

/*
 * Reads fill element from the bitstream.
 */
int32
CAACAudDec::extension_payload(TBitStream *bs, int32 cnt, uint32 prevEleID)
{  
  uint8 extType = (uint8) BsGetBits(bs, LEN_EX_TYPE);

  switch(extType)
  {
    case EX_FILL_DATA:
    default:
      if(sbrStream && !ReadSBRExtensionData(bs, sbrStream, extType, prevEleID, cnt))
      {

      BsGetBits(bs, LEN_NIBBLE);
      BsSkipNBits(bs, (cnt - 1) << 3); 
      break;  

      }
      else if (!sbrStream)
      {
      BsGetBits(bs, LEN_NIBBLE);
      BsSkipNBits(bs, (cnt - 1) << 3); 
      break;
          
      }
      
      
      
  }

  return (cnt);
}

/*
 * Reads fill data from the bitstream.
 */
void
CAACAudDec::GetFIL(TBitStream *bs, uint32 prevEleID)
{
  int32 cnt;
  
  cnt = BsGetBits(bs, LEN_F_CNT);
  if(cnt == (1 << LEN_F_CNT) - 1)
    cnt += BsGetBits(bs, LEN_F_ESC) - 1;

  while(cnt > 0)
    cnt -= extension_payload(bs, cnt, prevEleID);
}

/*
 * Reads program configuration element from the specified bitstream.
 */
int16
GetPCE(TBitStream *bs, TProgConfig *p)
{
  int16 i;

  p->pce_present = TRUE;
  p->tag = (int16) BsGetBits(bs, LEN_TAG);
  p->profile = (int16) BsGetBits(bs, LEN_PROFILE);
  p->sample_rate_idx = (int16) BsGetBits(bs, LEN_SAMP_IDX);
  p->front.num_ele = (int16) BsGetBits(bs, LEN_NUM_ELE);
  p->side.num_ele = (int16) BsGetBits(bs, LEN_NUM_ELE);
  p->back.num_ele = (int16) BsGetBits(bs, LEN_NUM_ELE);
  p->lfe.num_ele = (int16) BsGetBits(bs, LEN_NUM_LFE);
  p->data.num_ele = (int16) BsGetBits(bs, LEN_NUM_DAT);
  p->coupling.num_ele = (int16) BsGetBits(bs, LEN_NUM_CCE);

  p->mono_mix.present = (int16) BsGetBits(bs, 1);
  if(p->mono_mix.present == 1)
    p->mono_mix.ele_tag = (int16) BsGetBits(bs, LEN_TAG);

  p->stereo_mix.present = (int16) BsGetBits(bs, 1);
  if(p->stereo_mix.present == 1)
    p->stereo_mix.ele_tag = (int16) BsGetBits(bs, LEN_TAG);

  p->matrix_mix.present = (int16) BsGetBits(bs, 1);
  if(p->matrix_mix.present == 1)
  {
    p->matrix_mix.ele_tag = (int16) BsGetBits(bs, LEN_MMIX_IDX);
    p->matrix_mix.pseudo_enab = (int16) BsGetBits(bs, LEN_PSUR_ENAB);
  }

  for(i = 0; i < p->front.num_ele; i++)
  {
    p->front.ele_is_cpe[i] = (int16) BsGetBits(bs, LEN_ELE_IS_CPE);
    p->front.ele_tag[i] = (int16) BsGetBits(bs, LEN_TAG);
  }

  for(i = 0; i < p->side.num_ele; i++)
  {
    p->side.ele_is_cpe[i] = (int16) BsGetBits(bs, LEN_ELE_IS_CPE);
    p->side.ele_tag[i] = (int16) BsGetBits(bs, LEN_TAG);
  }

  for(i = 0; i < p->back.num_ele; i++)
  {
    p->back.ele_is_cpe[i] = (int16) BsGetBits(bs, LEN_ELE_IS_CPE);
    p->back.ele_tag[i] = (int16) BsGetBits(bs, LEN_TAG);
  }

  for(i = 0; i < p->lfe.num_ele; i++)
  {
    p->lfe.ele_is_cpe[i] = 0;
    p->lfe.ele_tag[i] = (int16) BsGetBits(bs, LEN_TAG);
  }

  for(i = 0; i < p->data.num_ele; i++)
  {
    p->data.ele_is_cpe[i] = 0;
    p->data.ele_tag[i] = (int16) BsGetBits(bs, LEN_TAG);
  }

  for(i = 0; i < p->coupling.num_ele; i++)
  {
    p->coupling.ele_is_cpe[i] = (int16) BsGetBits(bs, LEN_ELE_IS_CPE);
    p->coupling.ele_tag[i] = (int16) BsGetBits(bs, LEN_TAG);
  }

  BsByteAlign(bs);

  p->num_comment_bytes = (int16) BsGetBits(bs, LEN_COMMENT_BYTES);
  BsSkipNBits(bs, p->num_comment_bytes << 3);

  return (p->tag);
}

/**
 * Saves the start position of the channel element within the AAC frame.
 */
static void
SaveSBRChannelElementPos(SbrBitStream *sbrStream, uint32 bitOffset, uint32 ele_id)
{
  if(sbrStream->NrElements < MAX_NR_ELEMENTS)
  {
    /*-- Save starting position of the channel element. --*/
    sbrStream->sbrElement[sbrStream->NrElements].elementOffset = bitOffset - 3;  
    sbrStream->sbrElement[sbrStream->NrElements].ElementID = ele_id;
  }
}

/**
 * Saves the length of the channel element within the AAC frame.
 */
static void
SaveSBRChannelElementLen(SbrBitStream *sbrStream, uint32 presentPos)
{
  if(sbrStream->NrElements < MAX_NR_ELEMENTS)
  {
    /*-- Save length of the channel element. --*/
    sbrStream->sbrElement[sbrStream->NrElements].chElementLen = 
      presentPos - sbrStream->sbrElement[sbrStream->NrElements].elementOffset;
  }
}
/**************************************************************************
  Title        : CountAACChunkLength

  Purpose      : Counts the number of bytes reserved for the payload part of
                 current AAC frame. This functions should only be called if
                 no other methods exist to determine the payload legth
                 (e.g., ADIF does not include this kind of information).

  Usage        : y = CountAACChunkLength(bs, aac, bytesInFrame)

  Input        : bs           - input bitstream
                 aac          - AAC decoder parameters
  
  Output       : y            - status of operation
                 bytesInFrame - # of bytes reserved for this frame

  Author(s)    : Juha Ojanpera
  *************************************************************************/

EXPORT_C int16
CountAACChunkLength(TBitStream *bs, CAACAudDec *aac, int16 *bytesInFrame)
{
  uint32 bitsRead;
  int16 frameStatus;

  /*-- Save the status of input bitstream. --*/
  bitsRead = BsGetBitsRead(bs);

  /*-- Parse the frame. --*/
  frameStatus = (!GetAACGlobalGains(bs, aac, 15, NULL, NULL)) ? (int16) 0 : (int16) 1;
  ResetMCInfo(aac->mc_info);

  /*-- Determine the # of bytes for this frame. --*/
  bitsRead = BsGetBitsRead(bs) - bitsRead;
  *bytesInFrame = (int16) ((bitsRead >> 3) + ((bitsRead & 0x7) ? 1 : 0));

  return (frameStatus);
}

/**************************************************************************
  Title        : GetAACGlobalGains

  Purpose      : Extracts 'global_gain' bitstream elements from the specified
                 AAC data buffer.

  Usage        : y = GetAACGlobalGains(bs, aac, nBufs, globalGain, globalGainPos)

  Input        : bs            - input bitstream
                 aac           - AAC decoder handle
                 nBufs         - # of gain buffers present
  
  Output       : y             - # of gain elements extracted
                 globalGain    - 'global_gain' elements
                 globalGainPos - position location (in bits) of each gain value.
                                 This information is used when storing the gain 
                                 elements back to bitstream.

  Author(s)    : Juha Ojanpera
  *************************************************************************/

EXPORT_C uint8
GetAACGlobalGains(TBitStream* bs, CAACAudDec *aac, uint8 nBufs, uint8 *globalGain, uint32 *globalGainPos)
{
  uint8 numGains;
  uint8 loopControl;
  TProgConfig progCfg;
  CMC_Info *mip = aac->mc_info;
  uint32 ele_id, prevEleID, bufBitOffset;

  numGains = 0;
  loopControl = 0;
  prevEleID = ID_END;
  bufBitOffset = BsGetBitsRead(bs);

  /*-- Reset some modules. --*/
  ResetMCInfo(mip);

  /*-- No SBR elements present by default. --*/
  if(aac->sbrStream)
    aac->sbrStream->NrElements = 0;

  /*-- Loop until termination code found. --*/
  while((ele_id = BsGetBits(bs, LEN_SE_ID)) != ID_END)
  {
    int16 parserErr;

    if(loopControl > 64) break;

    /*-- Get audio syntactic element. --*/
    switch(ele_id)
    {
      /*-- Single and lfe channel. --*/
      case ID_SCE:
      case ID_LFE:
        if((numGains + 1) > nBufs) break;

        if(aac->sbrStream)
          SaveSBRChannelElementPos(aac->sbrStream, BsGetBitsRead(bs) - bufBitOffset, ele_id);

        if(globalGain)
          parserErr = GetSCEGain(aac, bs, &globalGain[numGains], &globalGainPos[numGains], bufBitOffset);
        else
          parserErr = GetSCE(aac, bs, mip, NULL, NULL, 0);

        if(parserErr < 0)
          goto f_out;
        numGains += 1;

        if(aac->sbrStream)
          SaveSBRChannelElementLen(aac->sbrStream, BsGetBitsRead(bs));
        break;

      /*-- Channel pair element. --*/
      case ID_CPE:
        if((numGains + 2) > nBufs) break;
        if(aac->sbrStream)
          SaveSBRChannelElementPos(aac->sbrStream, BsGetBitsRead(bs) - bufBitOffset, ele_id);

        if(globalGain)
          parserErr = GetCPEGain(aac, bs, &globalGain[numGains], &globalGainPos[numGains], bufBitOffset);
        else
          parserErr = GetCPE(aac, bs, mip, NULL, NULL, 0);

        if(parserErr < 0)
          goto f_out;
        numGains += 2;

        if(aac->sbrStream)
          SaveSBRChannelElementLen(aac->sbrStream, BsGetBitsRead(bs));
        break;

      /*-- Coupling channel. --*/
      case ID_CCE:
        if(GetCCE(aac, bs, mip, aac->ccInfo) < 0)
          goto f_out;
        break;

      /*-- Data element. --*/
      case ID_DSE:
        loopControl++;
        GetDSE(bs);
        break;

      /*-- Program config element. --*/
      case ID_PCE:
        loopControl++;
        GetPCE(bs, &progCfg);
        break;

      /*-- Fill element. --*/
      case ID_FIL:
        loopControl++;
        TInt error;
        TRAP( error, aac->GetFIL(bs, prevEleID) );
        if (error != KErrNone)
            goto f_out;
        break;
    
      default:
        goto f_out;
    }

    prevEleID = ele_id;

    bufBitOffset = BsGetBitsRead(bs) - bufBitOffset;
  }

  BsByteAlign(bs);

f_out:
  
  return (numGains);
}

/*
 * Stores 'global_gain' bitstream elements to AAC data buffer.
 */
INLINE void 
SetAACGain(TBitStream* bs, uint32 gainPos, uint8 globalGains)
{  
  BsSkipNBits(bs, gainPos); 
  BsPutBits(bs, LEN_SCL_PCM, globalGains); 
}

/*
 * Saves modified 'global_gain' bitstream elements to specified AAC data buffer.
 */
EXPORT_C void
SetAACGlobalGains(TBitStream* bs, uint8 numGains, uint8 *globalGain, uint32 *globalGainPos)
{
  int16 i;
  TBitStream bsIn;

  BsSaveBufState(bs, &bsIn);

  /*-- Store the gain element back to bitstream. --*/ 
  for(i = 0; i < numGains; i++)
    SetAACGain(bs, globalGainPos[i], globalGain[i]);

  
}

/*
 * Retrieves sample rate index corresponding to the specified sample rate.
 */
INLINE uint8
GetSampleRateIndex(int32 sampleRate)
{
  uint8 sIndex = 0xF;

  switch(sampleRate)
  {
    case 96000:
      sIndex = 0x0;
      break;

    case 88200:
      sIndex = 0x1;
      break;

    case 64000:
      sIndex = 0x2;
      break;

    case 48000:
      sIndex = 0x3;
      break;

    case 44100:
      sIndex = 0x4;
      break;

    case 32000:
      sIndex = 0x5;
      break;

    case 24000:
      sIndex = 0x6;
      break;

    case 22050:
      sIndex = 0x7;
      break;

    case 16000:
      sIndex = 0x8;
      break;

    case 12000:
      sIndex = 0x9;
      break;

    case 11025:
      sIndex = 0xa;
      break;

    case 8000:
      sIndex = 0xb;
      break;
  }

  return (sIndex);
}

INLINE int16
WriteGASpecificConfig(TBitStream *bsOut, int16 bitsWritten, int16 frameLen)
{ 
  /*-- Frame length flag (1024-point or 960-point MDCT). --*/
  bitsWritten += 1;
  BsPutBits(bsOut, 1, (frameLen == LN2) ? 0 : 1);
  
  /*-- No core coder. --*/
  bitsWritten += 1;
  BsPutBits(bsOut, 1, 0);
  
  /*-- No extension flag. --*/
  bitsWritten += 1;
  BsPutBits(bsOut, 1, 0);
  
  return (bitsWritten);
}



EXPORT_C int16
AACGetMP4ConfigInfo(int32 sampleRate, uint8 profile, uint8 nChannels, 
                    int16 frameLen, uint8 *pBuf, uint8 nBytesInBuf)
{
  TBitStream bsOut;
  int16 nConfigBytes, bitsWritten;

  BsInit(&bsOut, pBuf, nBytesInBuf);

  /*-- Object type. --*/
  bitsWritten = 5;
  BsPutBits(&bsOut, 5, profile + 1);

  /*-- Sample rate index. --*/
  bitsWritten += 4;
  BsPutBits(&bsOut, 4, GetSampleRateIndex(sampleRate));
  if(GetSampleRateIndex(sampleRate) == 0xF)
  {
    bitsWritten += 24;
    BsPutBits(&bsOut, 24, sampleRate);
  }
  
  /*-- # of channels. --*/
  bitsWritten += 4;
  BsPutBits(&bsOut, 4, nChannels);

  /*-- Write GA specific info. --*/
  bitsWritten = WriteGASpecificConfig(&bsOut, bitsWritten, frameLen);

  nConfigBytes = int16 ((bitsWritten & 7) ? (bitsWritten >> 3) + 1 : (bitsWritten >> 3));

  return (nConfigBytes);
}

/*
 * Saves modified 'global_gain' bitstream elements to specified AAC data buffer.
 */
EXPORT_C void
SetAACPlusGlobalGains(TBitStream* bs, TBitStream* bsOut, CAACAudDec *aac, int16 gainChangeValue, 
                  uint8 numGains, uint8 *globalGain, uint32 *globalGainPos)
    {
    int16 i;
    TBitStream bsIn;

    BsSaveBufState(bs, &bsIn);

    /*-- Store the gain element back to bitstream. --*/ 
    for(i = 0; i < numGains; i++)
        {
        SetAACGain(bs, globalGainPos[i], globalGain[i]);
        }
        

    if(aac->sbrStream && aac->sbrDecInfo)
          {
    
        if(aac->sbrStream->NrElements)
            {
              ParseSBR(&bsIn, bsOut, aac->sbrDecInfo, aac->sbrStream, gainChangeValue);
            }
      
      
        }

      }


EXPORT_C uint8
IsAACParametricStereoEnabled(CAACAudDec *aac)
    {

    return (IsSBRParametricStereoEnabled(aac->sbrDecInfo, aac->sbrStream));

    }

EXPORT_C uint8
IsAACSBREnabled(CAACAudDec *aac)
    {

    return (IsSBREnabled(aac->sbrStream));


    }
