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


#ifndef MP_STREAM_H_
#define MP_STREAM_H_

/**************************************************************************
  External Objects Needed
  *************************************************************************/

/*-- Project Headers --*/
#include "nok_bits.h"
#include "defines.h"
#include "param.h"
#include "mp3def.h"
#include "auddef.h"
#include <e32base.h>



/**************************************************************************
  External Objects Provided
  *************************************************************************/

/**************************************************************************
                Common structure definitions for all layers
  *************************************************************************/

/*
   Purpose:     Parent structure for error checking.
   Explanation: The number of bits that are protected varies between different 
                versions (MPEG-1, MPEG-2). */
class TCRC_Check
{
public:
  uint8 crc_payload[CRC_MAX_PAYLOAD]; /* Protected bits.         */
  uint16 bufLen;                       /* Length of 'crc_payload' */
  int16 crc;                         /* CRC error-check word.   */

};

/*
   Purpose:     Parent structure for mp1, mp2, and mp3 header.
   Explanation: - */

class TMPEG_Header
{
public:
  uint32 header; // Header bits. 

};

/*
   Purpose:     Parent Structure for mp1, mp2, and mp3 frames.
   Explanation: - */
class TMPEG_Frame
{
public:
  uint8 *scale_factors;           /* Scale factor of each subband and group. */
  int16 *quant;                  /* Quantized spectral data for this frame. */
  int16 *ch_quant[MAX_CHANNELS]; /* Channel pointers.                       */

};

 /*
   Purpose:     Parent Structure for mp1, mp2, and mp3 buffer.
   Explanation: - */
class TMPEG_Buffer
{
public:
  FLOAT *synthesis_buffer[MAX_CHANNELS]; /* Samples for windowing.     */
  int16 buf_idx[MAX_CHANNELS];           /* Start index for windowing. */
  int16 dct_idx[MAX_CHANNELS];           /* DCT buffer index.          */
  FLOAT *reconstructed;                  /* Dequantized samples.       */
  FLOAT *ch_reconstructed[MAX_CHANNELS]; /* Channel pointers.          */

};

 /*
   Purpose:     Parent Structure for frame detection.
   Explanation: - */
class TSyncInfoMix
{
public:
  int16 sync_length;       /* Length of sync word.                    */
  int16 sync_word;         /* Synchronization word.                   */
  int16 sync_mask;         /* Bitmask for sync word detection.        */
  MIX_SYNC_STATUS sync_status; /* Which layer we supposed to be decoding. */

};

/**************************************************************************
               Structure definitions applicable only to layer III
  *************************************************************************/
/*
   Purpose:     Parent Structure for layer III Granule Info.
   Explanation: - */
class TGranule_Info
{
public:
  uint8 global_gain;
  uint8 flags;
  uint8 table_select[3];
  uint8 subblock_gain[3];
  uint8 region0_count;
  uint8 region1_count;
  int32 part2_3_length;
  int32 big_values;
  int32 scalefac_compress;
  MP3_WINDOW_TYPE block_mode;
  int16 zero_part_start;

};

/*
   Purpose:     Parent Structure for layer III Scale Factors.
   Explanation: - */
class CIII_Scale_Factors : public CBase
{

public:

  uint8 *scalefac_long;     /* Scalefactors for long blocks.  */
  uint8 *scalefac_short[3]; /* Scalefactors for short blocks. */


};

/*
   Purpose:     Parent Structure for layer III Channel Info.
   Explanation: - */
class CIII_Channel_Info : public CBase
{
public:

  static CIII_Channel_Info* NewL();
  ~CIII_Channel_Info();
  TGranule_Info *gr_info[2];     /* Granule info for this channel. */
  CIII_Scale_Factors *scale_fac; /* Scalefactors for this channel. */

private:
   void ConstructL();
   CIII_Channel_Info();

};

/*
   Purpose:     Structure to hold MPEG-2 IS stereo positions.
   Explanation: - */
class TIS_Info
{
public:
  int16 is_len[3];
  int16 nr_sfb[3];

};

/*
   Purpose:     Structure to hold scalefactor band tables
                and related parameters.
   Explanation: - */
class CIII_SfbData : public CBase
{
public:

  static CIII_SfbData* NewL();
  ~CIII_SfbData();

  /* Scalefactor band boundaries for long and short blocks. */
  int16* sfbOffsetLong;
  int16* sfbOffsetShort;

  /* Scalefactor band widths for short blocks. */
  int16* sfbWidthShort;

  int16 *sfbLong;  /* Pointer to long sfb offset table.  */
  int16 *sfbShort; /* Pointer to short sfb offset table. */
  int16 *sfbWidth; /* Pointer to short sfb width table.  */
  int16 bandLimit; /* # of spectral bins to be decoded.  */ 

private:
    void ConstructL();
    CIII_SfbData();

};

/*
   Purpose:     Structure to map the file parameters into
                total length, average bitrate, etc.
   Explanation: - */
class TMP_BrInfo
{
public:
  BOOL vbr;          /* TRUE for VBR files, FALSE otherwise.        */
  BOOL tmp;          /* Used for counting the verage bitrate.       */
  BOOL true_br;      /* The 'start_br' represents the true bitrate. */
  BOOL free_br;      /* Free format flag.                           */
  uint32 cum_br;     /* Cumulative bitrate.                         */
  uint32 cum_frames; /* Cumulative # of frames.                     */
  uint16 start_br;   /* Initial and/or average bitrate.             */
  
};

/*
   Purpose:     Parent Structure for Layer III Side Information.
   Explanation: - */
class CIII_Side_Info : public CBase
{
public:

  static CIII_Side_Info* NewL();
  ~CIII_Side_Info();

  /*-- Side information read from the bit stream. --*/
  uint8 private_bits;
  uint8 scfsi[2][4];
  int32 main_data_begin;
  CIII_Channel_Info *ch_info[MAX_CHANNELS];

  /*-- General side information. --*/
  StereoMode *s_mode_long;    /* Stereo modes for long blocks.               */
  StereoMode *s_mode_short[3];/* Stereo modes for short blocks.              */
  
  int16 max_gr;               /* Number of granules within each stream frame.*/
  BOOL ms_stereo;             /* MS (Mid/Side) stereo used.                  */
  BOOL is_stereo;             /* Intensity stereo used.                      */
  BOOL lsf;                   /* MPEG-2 LSF stream present.                  */
  BOOL mpeg25;                /* MPEG-2.5 stream present.                    */
  int16 sb_limit;
  TIS_Info is_info;
  CIII_SfbData* sfbData;

private:
  void ConstructL();
  CIII_Side_Info();

};

/*
   Purpose:     Parent Structure for Huffman Decoding.
   Explanation: This structure can be used if dynamic memory allocation
                is not available or if memory consumption is important.
                At worst we have to process the number of codewords equal to
                the size of the codebook. */
class CHuffman : public CBase
{
public:
  int16 tree_len;        /* Size of the Huffman tree.                      */
  int16 linbits;         /* Number of extra bits.                          */
  const int16 *codeword;       /* Huffman codewords.                             */
  const int16 *packed_symbols; /* x, y and length of the corresponding codeword. */

};

/*
   Purpose:     Structure for buffer handling.
   Explanation: - */
class CMCUBuf : public CBase
{
public:

   static CMCUBuf* NewL(TInt aBufLen);
  ~CMCUBuf();

  TBitStream *bs;      /* Bitstream parser.                 */
  uint32 bufOffset;   /* Current file or buffer offset.    */
  uint32 bufLen;      /* Length of 'mcuBufbits', in bytes. */
  uint32 writeIdx;    /* Write index of the buffer.        */
  uint32 readSlots;   /* Bytes read from the buffer.       */
  uint8 *mcuBufbits;  /* Buffer for the compressed bits.   */

private:
    
    void ConstructL(TInt aBufLen);
    CMCUBuf();

};

/*
   Purpose:     More definitions for the buffer handling.
   Explanation: - */
typedef enum StreamMode
{
  IS_UNKNOWN,
  IS_FILE,
  IS_STREAM,
  READ_MODE,
  WRITE_MODE,
  APPEND_MODE

} StreamMode;


/*
   Purpose:     Structure implementing ring buffer.
   Explanation: - */
class CRingBuffer : public CBase
{
public:
  TBitStream *bs;    /* Bit parser for the 'bitBuffer'.  */
  uint32 bufLen;    /* Length of 'bitBuffer', in bytes. */
  uint32 readIdx;   /* Read index.                      */
  uint8 *bitBuffer; /* Buffer holding encoded data.     */

};

/*
   Purpose:     Layer 3 bitstream formatter (encoder side).
                The header+side info and payload are in separate
        buffers. The output mp3 frames are a combination
        of these two buffers.
   Explanation: - */
class CL3FormatBitstream : public CBase
{
public:
    static CL3FormatBitstream* NewL(uint16 sideInfoEntries, 
            uint16 sideInfoBytes, uint32 dataBytesBuffer);


    ~CL3FormatBitstream();

    void L3FormatBitstreamAddMainDataEntry(uint16 dataBytes);
    void L3FormatBitstreamAddMainDataBits(uint32 dataBits);
    uint32 L3WriteOutFrame(uint8 *outBuf);
  /* Number of bits present in 'frameData'. */
  uint32 numDataBits;

  /* Number of bytes reserved for payload part of the frame. */
  uint16 *payloadBytes;

  /* Read index to field 'payloadBytes'. */
  uint16 payloadReadIdx;

  /* Write index to field 'payloadBytes'. */
  uint16 payloadWriteIdx;

  /* Number of items in field 'payloadBytes'. */
  uint16 numPayloads;

  /* Payload data for layer 3 frames. */
  CRingBuffer *frameData;


  /* Number of frames present in 'frameHeader'. */
  uint32 framesPresent;

  /* Number of bytes reserved for side info per frame (fixed value). */
  uint16 sideInfoBytes;

  /* Buffer holding header and side info frames. */
  CRingBuffer *frameHeader;
  
private:
    
    void ConstructL(uint16 sideInfoEntries, uint16 sideInfoBytes, uint32 dataBytesBuffer);
    CL3FormatBitstream();
};

/*
   Purpose:     Bitrate of the mixer.
   Explanation: - */
class TL3BitRate
{
public:
  /*-- Bitrate of mixed stream and index of the rate. --*/
  uint16 bitRate;
  uint8 bitRateIdx;

  /*-- Status of padding bit in frame header. --*/
  uint8 padding;
  
  /*-- Number of bytes available for a frame. --*/
  uint16 frameBytes;
  
  /*-- Fractional part of the bytes reserved for each frame. --*/
  FLOAT frac_SpF;
  FLOAT slot_lag;

};

class THuffmanData
{
public:
  /* # of bits reserved for 'hufWord'. */
  uint8 hufBits;
  /* total # of bits reserved for codeword + sign bits + linbits. */
  uint8 hufBitsCount;
  /* Huffman codeword. */
  uint16 hufWord;

};

/*
   Purpose:     Huffman table parameters for layer 3.
   Explanation: - */
class CHuffmanCodTab : public CBase
{
public:
  uint8 xlen;
  uint8 ylen;
  uint8 linbits;
  uint8 xoffset;
  uint16 linmax;
  THuffmanData *hData;

};

/*
   Purpose:     # of pair and quadruple tables.
   Explanation: - */
#define L3HUFPAIRTBLS  (31)
#define L3HUFFQUADTBLS ( 2)

/*
   Purpose:     Parent structure to hold layer 3 Huffman 
                coding parameters.
   Explanation: - */
class CL3HuffmanTab : public CBase
{
public:

    static CL3HuffmanTab* NewL();
    ~CL3HuffmanTab();

    THuffmanData *quadTable[L3HUFFQUADTBLS];  // Tables 32 - 33. 
    CHuffmanCodTab *pairTable[L3HUFPAIRTBLS]; // Tables 1 - 31.  

    CHuffmanCodTab* tree;

private:
    CL3HuffmanTab();
    void ConstructL();

};

/*
   Purpose:     Helper parameters for mp3 mixer.
   Explanation: - */
class CL3MixerHelper : public CBase
{
public:
  /*-------- Encoding related parameters. --------*/

  /*-- Scalefactor encoding patterns for LSF streams. --*/
  uint8 blkNum;
  uint8 slen[4];
  uint8 blkTypeNum;
  int16 numRegions;

  /*-- Huffman tables for layer 3. --*/
  CL3HuffmanTab *l3Huf;
  
  /*-- Bitstream formatter for layer 3 encoder. --*/
  CL3FormatBitstream *l3bs;
  
  /*-- Window sequence of previous frame. --*/
  MP3_WINDOW_TYPE winTypeOld[MAX_CHANNELS];
  
  /*-- Window sequence of previous frame (source stream). --*/
  uint32 winTypeIdx;
  MP3_WINDOW_TYPE *winTypeCurrentFrame;
  MP3_WINDOW_TYPE *winTypePreviousFrame;
  MP3_WINDOW_TYPE *winTypePrt[MAX_CHANNELS];
  MP3_WINDOW_TYPE _winTypeOld1[2 * MAX_CHANNELS];
  MP3_WINDOW_TYPE _winTypeOld2[2 * MAX_CHANNELS];

  /*-- Overlap buffer for MDCT. --*/
  FLOAT** L3EncOverlapBuf;//[MAX_CHANNELS][MAX_MONO_SAMPLES];
  FLOAT* L3EncOverlapBufMem;

  /*-- Granule parameters of previous frame. --*/
  TGranule_Info *grInfoSave[MAX_CHANNELS];

  
  /*-- Scalefactors of previous frame. --*/
  CIII_Scale_Factors *scaleFacSave[MAX_CHANNELS];

  /*-- Spectrum limit of previous frame. --*/
  int16 specBinsPresent;

  /*-- Scalefactor selection information of previous frame. --*/
  BOOL IsScfsi;
  uint8 scfsi[2][4];
  
  /*-- Number of bits unused (bit reservoir). --*/
  uint32 bitPool;
 
  /*-- Level adjustment for the spectrum to be mixed. --*/
  FLOAT mixGain;
  
  /*-- Level adjustment for the mixed spectrum. --*/
  FLOAT overallGain;

  /*-- Level adjustment for the global gain. --*/
  int16 gainDec;

  /*-- Number of spectral bins to be mixed. --*/
  int16 mixSpectralBins;


  /*-------- Bitrate related parameters. --------*/
  
  /*-- Bytes reserved for side info. --*/
  int16 sideInfoBytes;

  /*-- Bitrate mappings. --*/
  BOOL needVBR;
  uint32 brIdx;
  TL3BitRate *l3br;
  TL3BitRate *_l3br[2];
  TL3BitRate l3brLong;
  TL3BitRate l3brShort;
  uint32 numFramesLong;
  uint32 numFramesShort;

  /*-- # of bytes reserved for the payload part of current frame. --*/
  int16 nSlots;
  
  /*-- 'main_data_begin' for next frame. --*/
  int16 main_data_begin;
  
  /*-- Max value of 'main_data_begin'. --*/
  int16 max_br_value;
  
};


/*
   Purpose:     Stream seeking constants.
   Explanation: - */
typedef enum StreamPos
{
  CURRENT_POS,
  START_POS,
  END_POS

} StreamPos;

/*
   Purpose:     Parameters of core engine.
   Explanation: - */
class CMP_Stream : public CBase
{

public:
  /*-- Common to all layers. --*/
  TBitStream *bs;
  TMPEG_Header *header;
  TMPEG_Header headerOld;
  TMPEG_Frame *frame;
  TMPEG_Buffer *buffer;
  TSyncInfoMix syncInfo;
  TMP_BrInfo brInfo;
  TCRC_Check mp3_crc;

  /*-- Layer III specific parameters. --*/
  TBitStream *br;
  CIII_Side_Info *side_info;
  CHuffman *huffman;
  FLOAT *spectrum[MAX_CHANNELS][SBLIMIT];
  uint16 OverlapBufPtr[MAX_CHANNELS];
  FLOAT OverlapBlck[2][MAX_CHANNELS][MAX_MONO_SAMPLES];
  /*
    Purpose:     Indices for reordering the short blocks.
    Explanation: First row describes the destination and second row 
                 the source index. */
  int16 reorder_idx[2][MAX_MONO_SAMPLES];
  
  /*-- Common complexity reduction and output stream parameters. --*/
  Out_Complexity *complex;
  Out_Param *out_param;

  int16 idx_increment;
  int16 FreeFormatSlots;
  int16 PrevSlots;
  int32 FrameStart;
  BOOL SkipBr;
  BOOL WasSeeking;
  int16 SlotTable[15];
  int16 FrameTable[15];
  int32 PrevStreamInfo[2];

  
};


BOOL InitBrInfo(CMP_Stream *mp, TMP_BrInfo *brInfo);
inline void SetBitrate(CMP_Stream *mp, int16 br)
{ mp->brInfo.cum_br += br; mp->brInfo.cum_frames++; }
BOOL CountAveBr(CMP_Stream *mp, TBitStream *bs_mcu, TMP_BrInfo *brInfo);
void FinishAveBr(TMP_BrInfo *brInfo, BOOL FullCount);

/* Implementations defined in module 'mstream.cpp'. */
CMP_Stream *GetMP3HandleL(void);
void ReleaseMP3Decoder(CMP_Stream *mp);
int32 main_data_slots(CMP_Stream *mp);
void decode_header(CMP_Stream *mp, TBitStream *bs);
void  MP3DecPrepareInit(CMP_Stream *mp, Out_Param *out_param, 
            Out_Complexity *complex, DSP_BYTE *br_buffer, 
            uint32 br_size);
void MP3DecCompleteInit(CMP_Stream *mp, int16 *frameBytes);

/* Implementations defined in module 'mp3.cpp'. */
SEEK_STATUS FreeFormat(CMP_Stream *mp, TBitStream *bs_mcu, 
               ExecState *execState, int16 *nSlots);
SEEK_STATUS SeekSync(CMP_Stream *mp, TBitStream *bs_mcu, 
             ExecState *execState, int16 *frameBytes);
void ReInitEngine(CMP_Stream *mp);
void ResetEngine(CMP_Stream *mp);
MP3_Error DecodeFrame(CMP_Stream *mp, int16 *pcm_sample, int16 idx_increment);
int16 L3BitReservoir(CMP_Stream *mp);

/* Implementations defined in module 'sfb.cpp'. */
void III_SfbDataInit(CIII_SfbData *sfbData, TMPEG_Header *header);
void III_BandLimit(CIII_SfbData *sfbData, uint16 binLimit);


/*
 * Low level implementations for the mp3 engine. 
 */

/* Implementations defined in module 'layer3.cpp'. */
BOOL III_get_side_info(CMP_Stream *mp, TBitStream *bs);
void III_get_scale_factors(CMP_Stream *mp, int16 gr, int16 ch);
void init_III_reorder(int16 reorder_idx[2][MAX_MONO_SAMPLES],
              int16 *sfb_table, int16 *sfb_width_table);
void III_reorder(CMP_Stream *mp, int16 ch, int16 gr);

/* Implementations defined in module 'mp3_q.cpp'. */
void III_dequantize(CMP_Stream *mp, int16 gr);

/* Implementations defined in module 'stereo.cpp'. */
void III_stereo_mode(CMP_Stream *mp, int16 gr);

/* Implementations defined in module 'huffman.cpp'. */
int16 III_huffman_decode(CMP_Stream *mp, int16 gr, int16 ch, int32 part2);
void
pairtable(CMP_Stream *mp, int16 section_length, int16 table_num, int16 *quant);
int16
quadtable(CMP_Stream *mp, int16 start, int16 part2, int16 table_num, int16 *quant,
          int16 max_sfb_bins);

void init_huffman(CHuffman *h);

/*-- Include encoder interface. --*/
#ifdef L3ENC
//#include "l3enc.h"
#endif /*-- L3ENC --*/

#endif /* MP_STREAM_H_ */
