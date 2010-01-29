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


/*-- Project Headers. --*/
#include "AudPanic.h"
#include "ProcTools.h"
#include "ProcMP3InFileHandler.h"
#include "mpheader.h"
#include "audconstants.h"


#include "ProcMP3FrameHandler.h"

// Debug print macro
#if defined _DEBUG 
#include <e32svr.h>
#define PRINT(x) RDebug::Print x;
#else
#define PRINT(x)
#endif

/*
   Purpose:     Minimum buffer size for determining the payload size
                of a free format mp3 bitstream.
   Explanation: - */
#define MIN_MP3_FREE_FORMAT_BUF_SIZE (42)

/*
   Purpose:     Buffer size for searching the start of a mp3 frame.
   Explanation: - */
const int16 Kmp3BufSize = 8;

/*
   Purpose:     Buffer size for determining an avarage frame size of a mp3 stream.
   Explanation: - */
const int16 Kmp3BitrateRegions = 4;

/*
   Purpose:     # of frames processed from each region.
   Explanation: This is used when the avarage frame size is determined. */
const int16 Kmp3BitrateNumFrames = 125;

/*
   Purpose:     Buffer size for determining if a clip is valid mp3
   Explanation: - */
const TUint Kmp3TempBufferSize = 4096;

// ID3v2 header's tag offset
const TUint KSizeOfTagOffset = 6;

// The size of MP3 header, header must include bits for determining frame length
const TInt KRawMp3FrameHeaderSize = 5;

// Bit rates in bits/sec supported by MPEG2, MPEG1 and MPEG2.5 respectively
const TInt16 cBitRateTable[3][16] =
	{
		{-1,8,16,24,32,40,48,56,64,80,96,112,128,144,160,0},
		{-1,32,40,48,56,64,80,96,112,128,160,192,224,256,320,0},
		{-1,8,16,24,32,40,48,56,64,80,96,112,128,144,160,0},
	};

// Sampling frequencies supported by MPEG2, MPEG1 and MPEG2.5 respectively
const TUint16 cSamplingFrequencyTable[3][4] =
	{
		{22050,24000,16000,0},
		{44100,48000,32000,0},
		{11025,12000,8000,0},
	};

const TInt KRawMp3MaxFrameSize = 1441;     // 320kbit/s @ 32kHz = int((144*320000/32000)+1)

CProcMP3InFileHandler* CProcMP3InFileHandler::NewL(const TDesC& aFileName, RFile* aFileHandle, CAudClip* aClip, 
                            TInt aReadBufferSize, TInt aTargetSampleRate, TChannelMode aChannelMode) 
{
  CProcMP3InFileHandler* self = NewLC(aFileName, aFileHandle, aClip, aReadBufferSize, 
                                          aTargetSampleRate, aChannelMode);
  CleanupStack::Pop(self);
  
  return self;
}

CProcMP3InFileHandler* 
CProcMP3InFileHandler::NewLC(const TDesC& aFileName, RFile* aFileHandle, CAudClip* aClip, 
                            TInt aReadBufferSize, TInt aTargetSampleRate, TChannelMode aChannelMode) 
{
  CProcMP3InFileHandler* self = new (ELeave) CProcMP3InFileHandler();
  CleanupStack::PushL(self);
  self->ConstructL(aFileName, aFileHandle, aClip, aReadBufferSize,
                      aTargetSampleRate, aChannelMode);
  
  return self;
}

CProcMP3InFileHandler::CProcMP3InFileHandler() : CProcInFileHandler(), mp3FileFormat(0),
                                                 isValidMP3(0), mp3HeaderBytes(0), 
                                                 mp3Log(0), isFreeFormatMP3(0), isVbrMP3(0) 
{
}




TBool CProcMP3InFileHandler::GetEncAudioFrameL(HBufC8*& aFrame, TInt& aSize, TInt32& aTime)
    {
    TBool rValue = EFalse;

  if(!iFileOpen) 
  {
    TAudPanic::Panic(TAudPanic::EInternal);
  }

  aSize = 0;
  aTime = 0;
  aFrame = NULL;

  if(isValidMP3)
  {
    TMpFrameState frState;
    int16 frameFound;
    TInt bufSize;
    TPtr8 mp3DataBuf(mp3HeaderBytes, Kmp3BufSize);

    /*-- Search start of next frame. --*/
    
    bufSize = BufferedFileRead((TDes8&) mp3DataBuf);
    if(bufSize < Kmp3BufSize)
      ZERO_MEMORY(mp3HeaderBytes + bufSize, Kmp3BufSize - bufSize);
    frameFound = GetMP3Frame(mp3HeaderBytes, Kmp3BufSize, &frState, 0);

    /*-- Frame found. --*/
    if(frameFound == 0)
    {
      TInt fLen;

      fLen = frState.frameBytes + frState.headerBytes;
      fLen -= Kmp3BufSize - (frState.readBytes - frState.headerBytes);
      if(fLen > 0)
      {
        TInt offset;
        uint8 *audFrameOffsetPtr;
        HBufC8 *audioFrame = HBufC8::NewL(frState.frameBytes + frState.headerBytes);

        offset = frState.readBytes - frState.headerBytes;
        mp3DataBuf.Set(mp3HeaderBytes + offset, Kmp3BufSize - offset, Kmp3BufSize - offset);

        audioFrame->Des().Copy(((const TDesC8 &) mp3DataBuf));

        audFrameOffsetPtr = (uint8 *) audioFrame->Des().Ptr();

        TPtr8 audTmpPtr(audFrameOffsetPtr + Kmp3BufSize - offset, fLen, fLen);

        bufSize = BufferedFileRead(audTmpPtr, fLen);

        fLen = frState.frameBytes + frState.headerBytes;
        audioFrame->Des().SetLength(fLen);

        aFrame = audioFrame;
        aSize = fLen;
        aTime = iMp3Edit->GetFrameTime(mp3FileFormat);
        iCurrentTimeMilliseconds += aTime;
        
        TRAPD(err, ManipulateGainL(aFrame));
    
        if (err != KErrNone)
            {
            // something went wrong with the gain manipulation
            // continue by returning the original frame
            }
    

        rValue = ETrue;
      }
    }
  }




  return (rValue);
    }



CProcMP3InFileHandler::~CProcMP3InFileHandler() 
{
//  if(iFileOpen)
  CloseFile();

  if (iMp3Edit != 0)
    {
      delete iMp3Edit;
      iMp3Edit = 0;
    }

  if (iFileName != 0)
    {
      delete iFileName;
      iFileName = 0;
    }
    
  if (iReadBuffer != 0)
    {
      delete iReadBuffer;
      iReadBuffer = 0;
    }

  if (mp3FileFormat != 0)
    {
      delete mp3FileFormat;
      mp3FileFormat = 0;
    }

  if (mp3HeaderBytes != 0)
    {
      delete mp3HeaderBytes;
      mp3HeaderBytes = 0;
    }
  
  if (iDecoder != 0)
    {
      delete iDecoder;
      iDecoder = 0;
    }
  
  if (iFrameHandler != 0)
    {
      delete iFrameHandler;
      iFrameHandler = 0;
    }

  if (iSilentFrame != 0)
    {
      delete iSilentFrame;
      iSilentFrame = 0;
    }
}

int16 CProcMP3InFileHandler::
GetMP3Frame(uint8 *dataBytes, int16 bufSize, TMpFrameState *frState, uint8 syncStatus)
{


  int16 locateFrame;
  int16 seekSyncStatus = 0;
  TPtr8 mp3DataBuf(dataBytes, bufSize);

  /*-- Search  start of 1st frame. --*/
  locateFrame = 0;
  frState->totalReadBytes = 0;
  
  while(locateFrame == 0)
  {
    if(syncStatus == 3)
      seekSyncStatus = iMp3Edit->FreeMode(mp3FileFormat, dataBytes, bufSize, 
                                   &frState->readBytes, &frState->frameBytes, 
                                   &frState->headerBytes);
    else
      seekSyncStatus = iMp3Edit->SeekSync(mp3FileFormat, dataBytes, bufSize, 
                                   &frState->readBytes, &frState->frameBytes, 
                                   &frState->headerBytes, (uint8) syncStatus);
    
    /*-- Start of 1st frame found. --*/
    if(seekSyncStatus == 0 || seekSyncStatus == 3)
      locateFrame = 1;

    /*-- Update data buffer, start of 1st frame not found yet. --*/
    else if(seekSyncStatus == 2)
    {
      if(frState->readBytes)
      {
        TInt tmpBufLen, tmp2;

        /*-- New data bytes to be read into buffer. --*/
        tmpBufLen = (int16) (bufSize - frState->readBytes);

        /*-- Move data that is possibly left in the buffer. --*/
        if(tmpBufLen) COPY_MEMORY(dataBytes, dataBytes + frState->readBytes, tmpBufLen);

        /*-- Prepare to read. --*/
        mp3DataBuf.Set(dataBytes + tmpBufLen, frState->readBytes, frState->readBytes);

        /*-- Update buffer. --*/
        tmp2 = BufferedFileRead((TDes8&) mp3DataBuf);
        if(tmp2 < frState->readBytes) 
          ZERO_MEMORY(dataBytes + tmpBufLen + tmp2, frState->readBytes - tmp2);

        frState->totalReadBytes += frState->readBytes;
      }
    }

    /*-- Abort, start of 1st frame cannot be located. --*/
    else locateFrame = 1;
  }

  return (seekSyncStatus);


}

void CProcMP3InFileHandler::
ConstructL(const TDesC& aFileName, RFile* aFileHandle, CAudClip* aClip, TInt aReadBufferSize,
            TInt aTargetSampleRate, TChannelMode aChannelMode) 
{


  iTargetSampleRate = aTargetSampleRate;
  iChannelMode = aChannelMode;

    
  int16 locate1stFrame;
  TMpFrameState frState;

  isVbrMP3 = 0;
  isValidMP3 = 0;
  isFreeFormatMP3 = 0;

  //-- Initialize file format handle. --
  mp3FileFormat = (TMpTransportHandle *) new (ELeave) TMpTransportHandle[1];
  ZERO_MEMORY(mp3FileFormat, sizeof(TMpTransportHandle));

  //-- Buffer for header search. --
  mp3HeaderBytes = new (ELeave) uint8[Kmp3BufSize];
  ZERO_MEMORY(mp3HeaderBytes, Kmp3BufSize);
  
  // -- Buffer for validity check --
  HBufC8* vldBuffer = (HBufC8*) HBufC8::NewLC(Kmp3TempBufferSize);

  iMp3Edit = CMp3Edit::NewL();

   // Set the file format parameters to default values. Note that this function
   // should be called only once; when searching the start of 1st frame.
   //
  iMp3Edit->InitTransport(mp3FileFormat);

  iClip = aClip;

  //-- Open file. --//
  InitAndOpenFileL(aFileName, aFileHandle, aReadBufferSize);

  TPtr8 mp3DataBuf(mp3HeaderBytes, Kmp3BufSize);

  /*-- Read 1st bytes in. --*/
  BufferedFileSetFilePos(0);
  BufferedFileRead((TDes8&) mp3DataBuf);
  
  
  // ----------------------------------------->
  
  // Identify ID3v2 header:
  
  // more info in http://www.id3.org/
  
  TInt tagLen = 0;
  if (mp3DataBuf.Left(3).Compare(_L8("ID3")) == 0)
    {
    
    const TInt KLenOffset = 6;
    
    BufferedFileSetFilePos(KLenOffset);
    BufferedFileRead((TDes8&) mp3DataBuf);
    
    // ID3v2 tag found, calculate lenght:

    const TInt K2Pow7 = 128;
    const TInt K2Pow14 = 16384;
    const TInt K2Pow21 = 2097152;
    
    tagLen = K2Pow21 * mp3DataBuf[0] + 
             K2Pow14 * mp3DataBuf[1] +
             K2Pow7 * mp3DataBuf[2] +
             mp3DataBuf[3];
                  
                  
    tagLen += 10; // include ID3v2 header

    }
  
  // <-----------------------------------------

  BufferedFileSetFilePos(tagLen);
  
  TPtr8 vldDes( vldBuffer->Des() );
  
  TInt bytesRead = BufferedFileRead((TDes8&) vldDes);
  vldDes.SetLength(bytesRead);
    
  TBool result = Validate(vldDes);
  
  CleanupStack::PopAndDestroy(vldBuffer);
  
  if (!result)
  {
      User::Leave(KErrCorrupt);
  }

  BufferedFileSetFilePos(tagLen);
  bytesRead = BufferedFileRead((TDes8&) mp3DataBuf);

  /*-- Search start of 1st frame. --*/
  locate1stFrame = GetMP3Frame(mp3HeaderBytes, Kmp3BufSize, &frState, 1);

  /*-- If free format, we must determine the payload size of the frames. --*/
  if(locate1stFrame == 3)
  {
    int16 tmpBufLen;
    uint8 tmpBuf[MIN_MP3_FREE_FORMAT_BUF_SIZE];

    /*-- Switch to larger buffer. --*/
    tmpBufLen = (int16) (Kmp3BufSize - frState.readBytes);
    COPY_MEMORY(tmpBuf, mp3HeaderBytes + frState.readBytes, tmpBufLen);
    if(frState.readBytes)
    {
      int16 tmpBufLen2;
      
      tmpBufLen2 = (int16) (MIN_MP3_FREE_FORMAT_BUF_SIZE - tmpBufLen);
      mp3DataBuf.Set(tmpBuf + tmpBufLen, tmpBufLen2, tmpBufLen2);
      BufferedFileRead((TDes8&) mp3DataBuf);
    }

    /*-- Determine the payload size. --*/
    locate1stFrame = GetMP3Frame(tmpBuf, MIN_MP3_FREE_FORMAT_BUF_SIZE, &frState, 3);

    /*-- If payload size known, then go back to the start of 1st frame. --*/
    if(locate1stFrame == 0)
    {
      isFreeFormatMP3 = 1;

      BufferedFileSetFilePos(0);
      mp3DataBuf.Set(mp3HeaderBytes, Kmp3BufSize, Kmp3BufSize);
      BufferedFileRead((TDes8&) mp3DataBuf);

      locate1stFrame = GetMP3Frame(mp3HeaderBytes, Kmp3BufSize, &frState, 0);
    }
  }

  if(locate1stFrame == 0)
    isValidMP3 = 1;

  BufferedFileSetFilePos(0);

  TAudFileProperties prop;
  GetPropertiesL(&prop);

  if (iProperties != 0)
    {
      
      
      // generate a header -------------->
      
      
      TUint8 byte1 = 0xFF; // sync
      TUint8 byte2 = 0xFB; // sync + V1,L3 (mp3), no CRC
      
      TBuf8<8> byte3B;
      
      switch (iProperties->iBitrate)
      {
          
      case (32000):
          {
              byte3B.Append(_L8("0001"));
              break;
          }
      case (40000):
          {
              byte3B.Append(_L8("0010"));
              break;
          }
      case (48000):
          {
              byte3B.Append(_L8("0011"));
              break;
          }
      case (56000):
          {
              byte3B.Append(_L8("0100"));
              break;
          }
      case (64000):
          {
              byte3B.Append(_L8("0101"));
              break;
          }
      case (80000):
          {
              byte3B.Append(_L8("0110"));
              break;
          }
      case (96000):
          {
              byte3B.Append(_L8("0111"));
              break;
          }
      case (112000):
          {
              byte3B.Append(_L8("1000"));
              break;
          }
      case (128000):
          {
              byte3B.Append(_L8("1001"));
              break;
          }
      case (160000):
          {
              byte3B.Append(_L8("1010"));
              break;
          }
      case (192000):
          {
              byte3B.Append(_L8("1011"));
              break;
          }
      case (224000):
          {
              byte3B.Append(_L8("1100"));
              break;
          }
      case (256000):
          {
              byte3B.Append(_L8("1101"));
              break;
          }
      case (320000):
          {
              byte3B.Append(_L8("1110"));
              break;
          }
      default:
          {
          if ( iProperties->iBitrateMode == EAudVariable )
            {
            // bitrate for silent frames in variable bitrate mode; use the lowest bitrate. 
            // However, mp3 is not an output format so this is not so relevant currently
              byte3B.Append(_L8("0001"));
            
            }
          else
            {
            
              User::Leave(KErrGeneral);
            }
              break;
              
          }
          
      }
      
      switch (iProperties->iSamplingRate)
      {
          
      case(44100):
          {
              byte3B.Append(_L8("00"));
              break;
          }
      case(48000):
          {
              byte3B.Append(_L8("01"));
              break;
          }
      case(32000):
          {
              byte3B.Append(_L8("10"));
              break;
          }
      default:
          {
              User::Leave(KErrGeneral);
              break;
          }
      }
      
      
      byte3B.Append(_L8("00")); // padding + protection bits
      
      TBuf8<8> byte4B;
      
      switch (iProperties->iChannelMode)
      {
          
      case(EAudStereo):
          {
              byte4B.Append(_L8("00"));
              break;
          }
      case(EAudDualChannel):
          {
              byte4B.Append(_L8("10"));
              break;
          }
      case(EAudSingleChannel):
          {
              byte4B.Append(_L8("11"));
              break;
          }
      default:
          {
              User::Leave(KErrGeneral);
              break;
          }
      }
      
      
      byte4B.Append(_L8("000")); // mode extension, 
      //    not copyrighted, 
      
      if (iProperties->iOriginal)
      {
          byte4B.Append(_L8("100"));
      }
      else
      {
          byte4B.Append(_L8("000"));
      }
      //copy of original, no emphasis
      
      TInt frameLength;
      if ( iProperties->iBitrateMode == EAudVariable )
          {
          // Use the lowest bitrate for silent frames in variable bitrate mode. 
          // However, mp3 is not an output format so this is not so relevant currently
          frameLength = (144*32000)/(iProperties->iSamplingRate);
          }
      else
          {
          frameLength = (144*iProperties->iBitrate)/(iProperties->iSamplingRate);
          }
          
      iSilentFrame = HBufC8::NewL(frameLength);
      
      TUint byte3 = 0;
      ProcTools::Bin2Dec(byte3B, byte3);
      
      TUint byte4 = 0;
      ProcTools::Bin2Dec(byte4B, byte4);
      
      TPtr8 silentFramePtr(iSilentFrame->Des());
      
      silentFramePtr.FillZ(frameLength);
      
      silentFramePtr[0] = byte1;
      silentFramePtr[1] = byte2;
      silentFramePtr[2] = static_cast<TUint8>(byte3);
      silentFramePtr[3] = static_cast<TUint8>(byte4);
      iSilentFrameDuration = ProcTools::MilliSeconds(iProperties->iFrameDuration);
      
    }
    
    if (aClip != 0)
        {
        iCutInTime = ProcTools::MilliSeconds(aClip->CutInTime());
        
        }
    
    iDecoder = CProcDecoder::NewL();
    
    iDecodingPossible = iDecoder->InitL(*iProperties, aTargetSampleRate, aChannelMode);
            
    iFrameHandler = CProcMP3FrameHandler::NewL();


    if (iClip != 0 && iClip->Normalizing())
        {
        SetNormalizingGainL(iFrameHandler);    
        }

}

TBool CProcMP3InFileHandler::Validate(TDes8& aDes)
{

    const TUint8* bufferPtr = aDes.Ptr();
    
    TInt bufferSize = aDes.Length();
	
	TInt scannedBuffer = 0;    	
	TInt lenMetaData = 0;    	
	TInt syncOffset = 0;
	TInt bufferPosition = 0;
	
    if (lenMetaData == 0)
        {
        syncOffset = 0;        
        lenMetaData = ID3HeaderLength(aDes, bufferPosition);
        }

    TInt bufferRemaining = bufferSize;
    
    while (lenMetaData > 0)
        {
	    if (lenMetaData >= bufferRemaining)
	        {
	        // this buffer contains all metadata
	        syncOffset += bufferRemaining;
	        lenMetaData -= bufferRemaining;
	        return KErrCorrupt;
	        }
	    else
	        {
	        syncOffset += lenMetaData;
	        scannedBuffer += lenMetaData;
	        // be sure to check for following id3 tags
	        bufferRemaining = bufferSize - scannedBuffer;	        
	        bufferPosition = scannedBuffer;	        
	        lenMetaData = ID3HeaderLength(aDes, bufferPosition);
    	    }
        }


    TInt seekOffset = 0;    
    bufferPosition = scannedBuffer;
    	
	seekOffset = SeekSync(aDes, bufferPosition);

    syncOffset += seekOffset; // offset to this point from content beginning
    scannedBuffer += seekOffset; // offset to this point in this buffer
        
    bufferPosition = scannedBuffer;
    
	if (seekOffset == bufferSize)
		{        
        return EFalse;
		}
		
    return ETrue;
    
    }
    
    
TInt CProcMP3InFileHandler::SeekSync(TDes8& aDes, TInt aBufPos)
    {
    const TUint bufStart = aBufPos;    
    
    TInt bufLen = aDes.Length();
    const TUint8* buf = aDes.Ptr() + bufStart;
    const TInt KMaxFrames = 3;          // number of frames to check
    const TInt KNotFound = bufLen;     // sync not found position
    
    TInt i = 0;
    TInt syncPos = KNotFound;
    TInt maxSeek = KMaxFrames;
    TInt bitRate = 0;
        
    const TUint8* endPtr = buf+bufLen-bufStart;

    // Seek a valid frame candidate byte by byte until a valid frame
    // is found or all bytes have been checked.
    while (buf < endPtr  &&  syncPos == KNotFound)
	    {
        TInt seekCount = 0;
        const TUint8* framePtr = buf;
        TInt frameBufLen = bufLen;
        syncPos = i;

		// Check the validity of this frame candidate and the nearest next
        // frames. If they are not OK, syncPos will be set to KNotFound.
        while (framePtr < endPtr  &&  syncPos != KNotFound  && seekCount < maxSeek)
	        {
	        
            TInt length = FrameInfo(framePtr, frameBufLen, bitRate);

			if (length == 0)
            	{
                syncPos = KNotFound;
				}
            
            if ((length > 0) && (bitRate < 0))
            	{
                maxSeek = 1; // free formatcase
				}
            framePtr += length;
            frameBufLen -= length;
            seekCount++;

			// consider SYNC not found if we reach end of buffer before finding 3 SYNC frames
			if ((framePtr >= endPtr) && (seekCount < maxSeek))
				{
				syncPos = KNotFound;
				buf += (bufLen-1);      // force an exit from while loop				
				}

        	}
        buf++; bufLen--; i++;
    	}
    return syncPos;
    }

TInt CProcMP3InFileHandler::FrameInfo(const TUint8* aBuf,TInt aBufLen,TInt& aBitRate)
    {
    TInt length = 0;
    TUint temp;
    TUint lTempVal;

    TInt samplingRate = 0;
    TInt id = 0;
    TInt Padding = 0;

	if (aBufLen >= KRawMp3FrameHeaderSize)
	    {
		// Extract header fields to aInfo and check their bit syntax
		// (including the sync word!). If the syntax is not OK the length
		// is set to zero.

		temp = 0;
		temp = aBuf[0] << 24;
		temp |= (aBuf[1] << 16);
		temp |= (aBuf[2] << 8);
		temp |= aBuf[3];
		if (((temp >> 21) & 0x7FF) != 0x7FF)
			{
			return length;
			}

		lTempVal = (temp >> 19) & 0x03;
		switch (lTempVal)
			{
			case 0:
				id = 2;  // MPEG2.5				
				break;
			case 1:
				return length;
			case 2:				
				id = 0;  // MPEG 2				
				break;
			case 3:
				id = 1;  // MPEG 1				
				break;
			}

		lTempVal = (temp >> 17) & 0x03;
		if (lTempVal != 1)
			{
			return length;
			}

		lTempVal = (temp >> 12) & 0x0F;
		aBitRate = cBitRateTable[id][lTempVal]*1000;

		if (aBitRate == 0)
			{
			return length;
			}

		lTempVal = (temp >> 10) & 0x03;
		if (lTempVal == 3)
			{
			return length;
			}
		else
			{
			samplingRate = cSamplingFrequencyTable[id][lTempVal];
			}

		Padding = (temp >> 9) & 0x01;

		lTempVal = (temp >> 6) & 0x03;
		

		if (lTempVal == 3)
			{			
			}
		else
            {            
            }        

		if (aBitRate == -1)
			{
			// For free mode operation
			length = KRawMp3MaxFrameSize;
			}

		if (samplingRate > 0  &&  aBitRate > 0)
			{
			length = (144*aBitRate)/samplingRate;

			if (id != 1)
				{
				length >>= 1; /*for MPEG2 and MPEG2.5 */
				}

			if (Padding)
				{
				length++;
				}
			}	
    	}
    return length;
    }


TInt CProcMP3InFileHandler::ID3HeaderLength(TDes8& aDes, TInt aPosition)
    {
	TInt lenMetaData;	
	TUint offset = aPosition;

	_LIT8 (KTagID3, "ID3");
	TPtrC8 searchBuf;

	// Set search buffer	
	searchBuf.Set(aDes);

    const TUint8* ptr = aDes.Ptr();
	TInt len = aDes.Length();
	searchBuf.Set(ptr+offset, len-offset);

    TInt startTagPos = searchBuf.Find (KTagID3);
	if (startTagPos == KErrNotFound || startTagPos != 0)
		{
        lenMetaData = 0;
		}
	else
		{
        lenMetaData = searchBuf[KSizeOfTagOffset];
        lenMetaData = ((lenMetaData << 8) | (searchBuf[KSizeOfTagOffset+1] << 1)) >> 1;
		lenMetaData = ((lenMetaData << 8) | (searchBuf[KSizeOfTagOffset+2] << 1)) >> 1;
		lenMetaData = ((lenMetaData << 8) | (searchBuf[KSizeOfTagOffset+3] << 1)) >> 1;
		lenMetaData += 10;
		}

    return lenMetaData;
    }


int16 
CProcMP3InFileHandler::GetMP3Bitrate(void)
{


  TMpFrameState frState;
  int16 bitRate, frameFound, offsetBytes;
  TPtr8 mp3DataBuf(mp3HeaderBytes, Kmp3BufSize);

  bitRate = 0;
  isVbrMP3 = 0;

  /*-- Search start of 1st frame. --*/
  BufferedFileRead((TDes8&) mp3DataBuf);
  frameFound = GetMP3Frame(mp3HeaderBytes, Kmp3BufSize, &frState, 0);

  /*-- How many unknown bytes at the start of the file? --*/
  offsetBytes = static_cast<int16>(frState.totalReadBytes + (frState.readBytes - frState.headerBytes));

  /*-- Get bitrate information. --*/
  if(frameFound == 0)
  {
    if(isFreeFormatMP3)
      bitRate = iMp3Edit->EstimateBitrate(mp3FileFormat, 0);

    /*
     * Since the mp3 stream is not using free format, we must somehow 
     * determine the (average) bitrate. Yes, mp3 header includes
     * information about the bitrate but that's valid only for
     * the current frame. In case variable coding is used the bitrate
     * can vary quite a lot depending on the goodness of the encoder
     * and signal conditions. Also if the mp3 stream is already an edited
     * version, the bitrate can change quite radicly between different portions
     * of the file. At the moment we determine the avarage frame size by
     * dividing the file into 'Kmp3BitrateRegions' regions of equal width
     * and from each region we determine the average frame size. The 1st
     * 'Kmp3BitrateNumFrames' frames are used from each region. The final
     * average frame size is then averaged across each region.
     * The reason why the bitrate estimation is so complicated is related
     * to seeking. At the moment we jump to the desired position and in order
     * to make this jump as accurate as possible we must have accurate information
     * about the average frame size. The advantages of jumping is that it's fast,
     * the side effect might be that we jump to incorrect position, the deviation
     * is negligible with constant bitrate streams but with variable bitrate streams
     * this can lead to quite large deviations, especially if the stream is using 
     * the full range of allowed bitrates. Fortunately, this is not the case in 
     * typical mp3 streams but after a series of editing we might have a 
     * file where bitrate changes are significant. Of course this means also that
     * the quality is not so good either, so probably the user will never create
     * such files due to poor sound quality...
     * 
     * The other approach would be the loop each frame and store frame positions,
     * let's say for every second. Works great, but the side effect is quite considerable
     * delay, since it certainly takes some time to process 5000-10000 mp3 frames...
     */
    else
    {
      int32 fPosOffset[Kmp3BitrateRegions];
      TInt fileSize, stepSize, nRegions, byteOffset;

      if(iFile.Size(fileSize) == KErrNone)
      {
        uint8 idx;
        int32 nFrames;
        int16 prevBitRate;
        TMPEG_Header *header;
        TInt ProcessingOnGoing;

        fileSize -= offsetBytes;
        if(fileSize < 0)
          return (0);

        header = &mp3FileFormat->header;

        /*-- Build the data region boundaries. --*/
        nRegions = 1;
        stepSize = fileSize / Kmp3BitrateRegions;
        byteOffset = stepSize;
        fPosOffset[0] = stepSize;
        TInt i = 0;
        for(i = 1; i < Kmp3BitrateRegions - 1; i++)
        {
          byteOffset += stepSize;
          if(byteOffset < fileSize)
          {
            nRegions++;
            fPosOffset[i] = byteOffset;
          }
          else break;
        }

        idx = 0;
        nFrames = 0;
        ProcessingOnGoing = 1;
        mp3FileFormat->aveFrameLen = 0;

        prevBitRate = bit_rate(header);

        /*-- Process each data region and accumulate the frame size. --*/
        while(ProcessingOnGoing)
        {
          TInt rValue, bufSize;

          for(i = 0; i < Kmp3BitrateNumFrames; i++)
          {
            TInt fLen;

            nFrames++;
            fLen = static_cast<int16>(frState.frameBytes + frState.headerBytes);

            frameFound = 2;
            mp3FileFormat->aveFrameLen += fLen;

            /*-- Check whether bitrate changed => variable bitrate. --*/
            if(!isVbrMP3)
              if(prevBitRate != bit_rate(header))
                isVbrMP3 = 1;

            /*
             * Skip the payload, remember that the input buffer has 'Kmp3BufSize'
             * bytes already that belong to the current mp3 frame. These bytes
             * need to be compensated before jumping to the start of next frame.
             */
            fLen -= static_cast<int16>(Kmp3BufSize - (frState.readBytes - frState.headerBytes)); 
            if(fLen < 0) fLen = 1;
            rValue = BufferedFileSetFilePos(BufferedFileGetFilePos() + fLen);

            if(rValue) 
            {
              /*-- Read new data for parsing of next frame. --*/
              bufSize = BufferedFileRead((TDes8&) mp3DataBuf);
              if(bufSize < Kmp3BufSize)
                ZERO_MEMORY(mp3HeaderBytes + bufSize, Kmp3BufSize - bufSize);

              frameFound = GetMP3Frame(mp3HeaderBytes, Kmp3BufSize, &frState, 0);
            }
            
            if(frameFound != 0)
            {
              ProcessingOnGoing = 0;
              break;
            }
          }

          if(ProcessingOnGoing && idx < nRegions)
          {
            frameFound = 2;

            /*-- Seek to start of next data region. --*/
            rValue = BufferedFileSetFilePos(fPosOffset[idx++]);

            if(rValue)
            {
              /*-- Read new data and search start of frame. --*/
              bufSize = BufferedFileRead((TDes8&) mp3DataBuf);
              if(bufSize < Kmp3BufSize)
                ZERO_MEMORY(mp3HeaderBytes + bufSize, Kmp3BufSize - bufSize);

              frameFound = GetMP3Frame(mp3HeaderBytes, Kmp3BufSize, &frState, 0);
            }

            if(frameFound != 0)
              ProcessingOnGoing = 0;
          }
          else ProcessingOnGoing = 0;
        }

        if(frameFound != 0)
        {
          mp3FileFormat->execState.execMode = GLITCH_FREE;
          mp3FileFormat->header.header = mp3FileFormat->headerOld.header;
        }

        /*-- Average frame length, in bytes. --*/
        if(nFrames)
        {
          FLOAT tmp;

          tmp = mp3FileFormat->aveFrameLen / (FLOAT) nFrames;
          mp3FileFormat->aveFrameLen = (int16) (tmp + 0.5f);
        }

        /*-- This is our estimated bitrate. --*/
        bitRate = iMp3Edit->EstimateBitrate(mp3FileFormat, 1);
      }
    }
  }
  return (bitRate);

}

void 
CProcMP3InFileHandler::GetPropertiesL(TAudFileProperties* aProperties) 
{
    PRINT((_L("CProcMP3InFileHandler::GetPropertiesL in") ));


    if (iProperties != 0)
        {
        *aProperties = *iProperties;
        return;
        }

    if(iFileOpen) 
        {
        TInt origFilePos = iFilePos;

        aProperties->iFileFormat = EAudFormatUnrecognized;
        aProperties->iAudioType = EAudTypeUnrecognized;
        aProperties->iAudioTypeExtension = EAudExtensionTypeNoExtension;
        aProperties->iBitrate = 0;
        aProperties->iBitrateMode = EAudBitrateModeNotRecognized;
        aProperties->iChannelMode = EAudChannelModeNotRecognized;
        aProperties->iDuration = 0;
        aProperties->iSamplingRate = 0;
        aProperties->iFrameLen = 0;
        aProperties->iFrameCount = 0;

        /*-- First mp3 frame found? --*/
        if(isValidMP3)
            {
            TMPEG_Header *header;

            header = &mp3FileFormat->header;

            /*-- Seek to start of file. --*/
            BufferedFileSetFilePos(0);

          

            if (version(header) != 1)
                {
                PRINT((_L("CProcMP4InFileHandler::GetPropertiesL header unsupported, leaving") ));
                User::Leave(KErrNotSupported);
                return;
                }


            /*-- Determine bitrate. --*/
            aProperties->iBitrate = GetMP3Bitrate();

         
            if(aProperties->iBitrate)
                {
                TInt fileSize;

                iFile.Size(fileSize);

                aProperties->iAudioType = EAudMP3;
                aProperties->iFileFormat = EAudFormatMP3;
                aProperties->iBitrateMode = (!isVbrMP3) ? EAudConstant : EAudVariable;

                /*-- Determine channel mode. --*/
                switch(mode(header))
                    {
                    case 0:
                    case 1:
                        aProperties->iChannelMode = EAudStereo;
                        break;

                    case 2:
                        aProperties->iChannelMode = EAudDualChannel;
                        break;

                    case 3:
                    default:
                        aProperties->iChannelMode = EAudSingleChannel;
                        break;
                    }

                /*-- Estimate duration. --*/
                TInt64 tmp = (TInt64)iMp3Edit->FileLengthInMs(mp3FileFormat, fileSize) * 1000;
                TTimeIntervalMicroSeconds durationMicro(tmp);
                aProperties->iDuration = durationMicro;

                aProperties->iSamplingRate = frequency(header);
                
                // Check that the sample rate is supported    
                if( (aProperties->iSamplingRate != KAedSampleRate8kHz) &&
                    (aProperties->iSamplingRate != KAedSampleRate11kHz) &&
                    (aProperties->iSamplingRate != KAedSampleRate16kHz) &&
                    (aProperties->iSamplingRate != KAedSampleRate22kHz) &&
                    (aProperties->iSamplingRate != KAedSampleRate24kHz) &&
                    (aProperties->iSamplingRate != KAedSampleRate32kHz) &&
                    (aProperties->iSamplingRate != KAedSampleRate44kHz) &&
                    (aProperties->iSamplingRate != KAedSampleRate48kHz) )
                    {
                    User::Leave(KErrNotSupported);
                    }

                aProperties->iFrameLen = mp3FileFormat->aveFrameLen;
            
                // casting for PC-Lint
                tmp = (TInt64) (TInt)(iMp3Edit->GetFrameTime(mp3FileFormat) * 1000);
                aProperties->iFrameDuration = tmp;
                aProperties->iFrameCount = ProcTools::GetTInt((aProperties->iDuration).Int64()/(aProperties->iFrameDuration).Int64());

                if (((TUint32)header->header & 0x4) != 0)
                    {

                    aProperties->iOriginal = ETrue;
                
                    }

                BufferedFileSetFilePos(origFilePos);
                }
            }
        else
            {
            PRINT((_L("CProcMP4InFileHandler::GetPropertiesL could not parse frames, leaving") ));
            User::Leave(KErrNotSupported);
            }
        }
    else 
    {
        TAudPanic::Panic(TAudPanic::EInternal);
    }

    // bitrate is bytes not kilobytes
    aProperties->iBitrate *= 1000;


    if (iProperties == 0)
    {
        iProperties = new (ELeave) TAudFileProperties;
        *iProperties = *aProperties;
        
    }
    
}

TBool 
CProcMP3InFileHandler::SeekAudioFrame(TInt32 aTime) 
{
  TBool rValue = EFalse;



  if(!iFileOpen) 
  {
    TAudPanic::Panic(TAudPanic::EInternal);
  }

  if(isValidMP3)
  {
    int32 fPos;
    
    mp3FileFormat->aveFrameLen = iProperties->iFrameLen;
    fPos = iMp3Edit->GetSeekOffset(mp3FileFormat, aTime);

    BufferedFileSetFilePos(fPos);

    iCurrentTimeMilliseconds = aTime;

    rValue = ETrue;
  }



  return (rValue);
}    

TBool 
CProcMP3InFileHandler::SeekCutInFrame() 
{
  iCurrentTimeMilliseconds = iCutInTime;

  return SeekAudioFrame(iCutInTime);
}


TBool 
CProcMP3InFileHandler::GetDurationL(TInt32& aTime, TInt& aFrameAmount) 
{
  TBool rValue = EFalse;



  if(!iFileOpen) 
  {
    TAudPanic::Panic(TAudPanic::EInternal);
  }

  aTime = 0;
  aFrameAmount = 0;

  if(isValidMP3)
  {
    TInt filePos;
    TAudFileProperties aProperties;
    
    filePos = iFilePos;

    GetPropertiesL(&aProperties); 

    if(aProperties.iBitrate)
    {
      rValue = ETrue;
      aTime = ProcTools::MilliSeconds(aProperties.iDuration);
      aFrameAmount = aTime / iMp3Edit->GetFrameTime(mp3FileFormat);
    }

    BufferedFileSetFilePos(filePos);
  }
    

  return (rValue);
}


TBool 
CProcMP3InFileHandler::SetNormalizingGainL(const CProcFrameHandler* aFrameHandler)
{
    
    HBufC8* point = 0;
    TInt siz;
    TInt32 tim = 0;
    TInt maxGain = 0;
    RArray<TInt> gains;
    TInt maxAverage = 0;
    TInt tmpGain = 0;
    TInt gainCounter = 0;
    TInt timeNow = 0;
    TBitStream bs;

    while(GetEncAudioFrameL(point, siz, tim)) 
    {
        timeNow += tim;
        ((CProcMP3FrameHandler*) aFrameHandler)->GetMP3Gains(point, gains, maxGain, bs);
        
        for (TInt a = 0 ; a < gains.Count() ; a = a+2)
        {
            tmpGain += gains[a];
            gainCounter++;
        }
        gains.Reset();
        
        if (timeNow > 1000)
        {
            if (tmpGain/gainCounter > maxAverage)
            {
                maxAverage = tmpGain/gainCounter;
            }
            
            timeNow = 0;
            tmpGain = 0;
            gainCounter = 0;
        }
        
        delete point;
        
    }

    // bigger value makes normalizing more efficient, but makes
    // dynamic compression more likely to occur
    TInt NormalizingFactor = 179;
    if (iProperties->iBitrate > 20000 && iProperties->iBitrate < 40000)
    {
        
        // 32 kBit/s
        NormalizingFactor = 187;
        
    }
    else if (iProperties->iBitrate > 40000 && iProperties->iBitrate < 80000)
    {
        // 64 kBit/s
        NormalizingFactor = 181;
        
    }
    
    
    else if (iProperties->iBitrate > 80000 && iProperties->iBitrate < 140000)
    {
        // 128 kBit/s
        if (iProperties->iChannelMode == EAudSingleChannel)
            NormalizingFactor = 170;
        else
            NormalizingFactor = 179;
        
    }
    else if (iProperties->iBitrate > 140000)
    {
        // 256 kBit/s
        if (iProperties->iChannelMode == EAudSingleChannel)
            NormalizingFactor = 155;
        else
            NormalizingFactor = 167;
        
    }
    else
    {
        
        if (iProperties->iChannelMode == EAudSingleChannel)
            NormalizingFactor = 170;
        
    }
    
    
    TInt gainBoost = (NormalizingFactor-maxAverage)*3;
    
    if (gainBoost < 0) gainBoost = 0;
    
    iNormalizingMargin = static_cast<TInt8>(gainBoost);

    SeekAudioFrame(0);

    mp3FileFormat->execState.a0_s16[0] = 0;
    mp3FileFormat->execState.a0_s16[1] = 0;
    mp3FileFormat->execState.a0_s16[2] = 0;

    mp3FileFormat->execState.a0_u32[0] = 0;
    mp3FileFormat->execState.a0_u32[1] = 0;
    mp3FileFormat->execState.a0_u32[2] = 0;



    
    return ETrue;
    
    
}


