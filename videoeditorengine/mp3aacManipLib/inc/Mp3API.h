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



#ifndef _MP3API_
#define    _MP3API_


#include "defines.h"
#include "mstream.h"





/*
   Purpose:     Structure interface for MP1/2/3 transport stream.
   Explanation: - */
class TMpTransportHandle
{
public:

  TMPEG_Header header;
  TMPEG_Header headerOld;

  int16 SlotTable[15];
  int16 FreeFormatSlots;      /*-- Number of bytes in free format frame.     */

  int16 mainDataSlots;        /*-- Payload size.                             */

  BOOL ms_stereo;             /* MS (Mid/Side) stereo used.                  */
  BOOL is_stereo;             /* Intensity stereo used.                      */
  BOOL lsf;                   /* MPEG-2 LSF stream present.                  */
  BOOL mpeg25;                /* MPEG-2.5 stream present.                    */

  /*-- Transport stream to be used/in use. --*/
  SYNC_STATUS transportType;
  SyncInfo syncInfo;

  /*-- Exec state when seeking next MPx frame. --*/
  int16 offsetBits;
  ExecState execState;

  /*-- CRC codeword. --*/
  TCRC_Check crc;

  /*-- Average number of bytes reserved for each frame. --*/
  int32 aveFrameLen;
  
};


/**
 * Data structure for mp3 frame search.
 */
class TMpFrameState
{
public:
  /**
   * Number of read bytes when searching start of frame. 
   * This value is within the input buffer and also the header
   * is read after start of frame has been found.
   */
  int16 readBytes;

  /**
   * Number of header bytes present for current MP3 frame.
   */
  int16 headerBytes;

  /**
   * Number of payload bytes for current MP3 frame. The total frame
   * size is 'headerBytes' + 'frameBytes'.
   */
  int16 frameBytes;

  /**
   * Total number of bytes read when searching the start of frame.
   */
  int32 totalReadBytes;

};




/*
   Purpose:     Parameters of core engine.
   Explanation: - */
class CMPAudDec : public CBase
{
public:

    IMPORT_C static CMPAudDec* NewL();
    IMPORT_C ~CMPAudDec();
    IMPORT_C void Init(TMpTransportHandle *aMpFileFormat);

    /*-- Common to all layers. --*/
    uint8 *bitReserv;
    TMPEG_Frame *frame;

    /*-- Layer III specific parameters. --*/
    TBitStream br;
    CHuffman *huffman;
    CIII_Side_Info *side_info;

    int16 PrevSlots;
    //int32 FrameStart;
    BOOL SkipBr;
    BOOL WasSeeking;

    /*-- Transport handle. --*/
    TMpTransportHandle *mpFileFormat;

  
private:

    CMPAudDec();
    void ConstructL();

};

class CMp3Edit : public CBase
    {

public:

    IMPORT_C static CMp3Edit* NewL();
    IMPORT_C ~CMp3Edit();

    IMPORT_C uint32
    FileLengthInMs(TMpTransportHandle *tHandle, int32 fileSize) const;

    IMPORT_C int32
    GetSeekOffset(TMpTransportHandle *tHandle, int32 seekPos) const;

    IMPORT_C int32
    GetFrameTime(TMpTransportHandle *tHandle) const;

    IMPORT_C void 
    InitTransport(TMpTransportHandle *tHandle) const;

    IMPORT_C int16
    SeekSync(TMpTransportHandle *tHandle, uint8 *syncBuf, 
            uint32 syncBufLen, int16 *readBytes, 
            int16 *frameBytes, int16 *headerBytes,
            uint8 initMode) const;

    IMPORT_C int16
        FreeMode(TMpTransportHandle *tHandle, uint8 *syncBuf, 
        uint32 syncBufLen, int16 *readBytes, 
        int16 *frameBytes, int16 *headerBytes) const;
    
    IMPORT_C int16
        EstimateBitrate(TMpTransportHandle *tHandle, uint8 isVbr) const;
    



    /*-- Closes MPEG Layer I/II/III decoder handle. --*/
    IMPORT_C CMPAudDec *
        DeleteMPAudDec(CMPAudDec *mpAud);
    
    /*-- Creates MPEG Layer I/II/III decoder handle. --*/
    IMPORT_C CMPAudDec *
        CreateMPAudDec(void);
    
    /*-- Initializes MPEG Layer I/II/III decoder handle. --*/
    IMPORT_C void
        InitMPAudDec(CMPAudDec *mpAud, TMpTransportHandle *mpFileFormat);
    
    /*-- Resets MPEG Layer I/II/III decoder handle. --*/
    IMPORT_C void
        ResetMPAudDec(CMPAudDec *mpAud);
    
    /*-- Stores global gain values. --*/
    IMPORT_C void
        SetMPGlobalGains(TBitStream *bs, uint8 numGains, uint8 *globalGain, uint32 *gainPos);
    
    /*-- Extracts global gain values. --*/
    IMPORT_C uint8
        GetMPGlobalGains(TBitStream *bs, TMpTransportHandle *tHandle, uint8 *globalGain, uint32 *gainPos);

private:
    
    void ConstructL();
    CMp3Edit();

    };


#endif
