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




#include "ProcAACFrameHandler.h"
#include "ProcFrameHandler.h"
#include "ProcTools.h"
#include "nok_bits.h"
#include "AACAPI.h"

#include <f32file.h>
#include <e32math.h>


TBool CProcAACFrameHandler::ManipulateGainL(const HBufC8* aFrameIn, HBufC8*& aFrameOut, TInt8 aGain) 
    {

    if (aFrameIn == 0)
        return EFalse;

    
    TInt numBlocks = 1;
    TInt headerBytes = CalculateNumberOfHeaderBytes(aFrameIn, numBlocks);
    
    // Unfortunately const casting is needed to avoid unecessary temporary arrays
    // and we don't have a separate bitstream class for const objects
    // Just have to make very sure not to modify const descriptors!!
    TUint8* buf = const_cast<TUint8*>(aFrameIn->Right(aFrameIn->Size()-headerBytes).Ptr());
    
    uint8* gains = new (ELeave) uint8[16];
    uint32* gainPos = new (ELeave) uint32[16];

    TInt bufLen = aFrameIn->Size()-headerBytes;

    TBitStream bs;
    TBitStream bs2;
    
    BsInit(&bs, buf, bufLen);
    BsInit(&bs2, buf, bufLen);
    
    for (TInt b = 0 ; b < numBlocks ; b++)
        {
        
        TInt b_i = bs.buf_index;
        iFrameStarts.Append(b_i+headerBytes);

        BsSaveBufState(&bs, &bs2);

        uint8 numberOfGains = GetAACGlobalGains(&bs, iDecHandle, 16, gains, gainPos);
        
        if (numberOfGains > 2)
            {
            // illegal frame??
            delete[] gainPos;
            delete[] gains;
            return KErrGeneral;

            }

        if (bs.buf_index > 0)
            iFrameLengths.Append(bs.buf_index - b_i);
        else
            iFrameLengths.Append(bs.buf_len - b_i);
        
        if (headerBytes > 7)
            {
            bs.buf_index += 2; // crc
            bs.slots_read += 2;
            bs.bits_read += 16;

            }
                
        TInt tmpGain = aGain*500;
        int16 newGain = static_cast<int16>(tmpGain/1500);
        
        
        for (TInt a = 0 ; a < numberOfGains ; a++)
        {
            if (gains[a] + newGain > 255)
            {
                gains[a] = 255;
            }
            else if (gains[a] + newGain < 0)
            {
                gains[a] = 0;
            }
            else
            {
                gains[a] = static_cast<TUint8>(gains[a]+newGain);
            }
        }
        
    
        if (iAACInfo.isSBR || iAACInfo.iIsParametricStereo)
            {

            uint8 *data = new (ELeave) uint8[1024];
            CleanupStack::PushL(data);
            TBitStream bsOut;

            BsInit(&bsOut, data, 1024);

            SetAACPlusGlobalGains(&bs2, &bsOut, iDecHandle, static_cast<int16>(-newGain), numberOfGains, gains, gainPos);
            
            TInt incBytes = (bsOut.bits_read >> 3) - bs2.buf_len;
            iFrameLengths[iFrameLengths.Count()-1] +=incBytes; 
            
            TInt newSize = aFrameIn->Size()+incBytes;
            aFrameOut = HBufC8::NewL(newSize);
            aFrameOut->Des().Append(aFrameIn->Left(headerBytes));
            aFrameOut->Des().Append(bsOut.bit_buffer, bsOut.bits_read >> 3);
            
            if (headerBytes != 0) UpdateHeaderL(aFrameOut);
                
            CleanupStack::Pop(data);
            delete[] data;
            data = 0;
            
                
            }
        else
            {
            SetAACGlobalGains(&bs2, numberOfGains, gains, gainPos);
            aFrameOut = HBufC8::NewL(aFrameIn->Size());
            aFrameOut->Des().Copy(aFrameIn->Ptr(), aFrameIn->Size());
        
            }
            

            
        //    }

        }
    delete[] gains;
    delete[] gainPos;
    
    
    return ETrue;
    }

TBool CProcAACFrameHandler::GetGainL(const HBufC8* aFrame, RArray<TInt>& aGains, TInt& aMaxGain) const
    {

    
    TInt numBlocks = 1;
    TInt headerBytes = CalculateNumberOfHeaderBytes(aFrame, numBlocks);
    
    //TBitStream bs;
    TUint8* buf = const_cast<TUint8*>(aFrame->Right(aFrame->Size()-headerBytes).Ptr());
    //BsInit(&bs, buf, aFrame->Size()-headerBytes);

    uint8* gains = new (ELeave) uint8[16];
    CleanupStack::PushL(gains);
    uint32* gainPos = new (ELeave) uint32[16];
    CleanupStack::PushL(gainPos);

    //TPtr8 frameWithoutHeader = aFrame->Right(aFrame->Size()-headerBytes));
    TInt bufLen = aFrame->Size()-headerBytes;

    TBitStream bs;

    
    BsInit(&bs, buf, bufLen);
    
    
    for (TInt b = 0 ; b < numBlocks ; b++)
    {
                
        uint8 numberOfGains = GetAACGlobalGains(&bs, iDecHandle, 16, gains, gainPos);

        for (TInt a = 0 ; a < numberOfGains ; a++)
        {
            aGains.Append(gains[a]);
        }

        if (headerBytes > 7)
            {
            bs.buf_index += 2; // crc
            bs.slots_read += 2;
            bs.bits_read += 16;

            }
                
    }
    CleanupStack::Pop(); //(gainPos)
    CleanupStack::Pop(); //(gains)
    delete[] gains;
    delete[] gainPos;

    aMaxGain = 255;
    return EFalse;



    }


TBool CProcAACFrameHandler::GetNormalizingMargin(const HBufC8* aFrame, TInt8& aMargin) const

    {
        
    TUint8* buf = const_cast<TUint8*>(aFrame->Ptr());
    TInt bufLen = aFrame->Size();

    uint8* gains = new uint8[16];

    uint32* gainPos = new uint32[16];
    
    
    TBitStream bs;
    
    BsInit(&bs, buf, bufLen);

    uint8 numberOfGains = GetAACGlobalGains(&bs, iDecHandle, 16, gains, gainPos);

    TUint8 maxGain = 0;

    for (TInt a = 0 ; a < numberOfGains ; a++)
        {
        if (gains[a] > maxGain)
            {
            maxGain = gains[a];
            }
        }
    delete[] gains;
    delete[] gainPos;

    TInt marginInt = (255-maxGain)*6;

    if (marginInt > 127)
        {
        aMargin = 127;
        }
    else if (marginInt < 0)
        {
        aMargin = 0;
        }
    else
        {
        aMargin = static_cast<TInt8>(marginInt);
        }

    return ETrue;


    }

CProcAACFrameHandler::~CProcAACFrameHandler()
    {

    iFrameStarts.Reset();
    iFrameLengths.Reset();
    if (iDecHandle != 0)
        {
        
        DeleteAACAudDec(iDecHandle);
        }


    }

CProcAACFrameHandler* CProcAACFrameHandler::NewL(TAACFrameHandlerInfo aAACInfo) 
    {

    
    CProcAACFrameHandler* self = NewLC(aAACInfo);
    CleanupStack::Pop(self);
    return self;

    }
CProcAACFrameHandler* CProcAACFrameHandler::NewLC(TAACFrameHandlerInfo aAACInfo) 
    {

    CProcAACFrameHandler* self = new (ELeave) CProcAACFrameHandler();
    CleanupStack::PushL(self);
    self->ConstructL(aAACInfo);
    return self;

    }

void CProcAACFrameHandler::ConstructL(TAACFrameHandlerInfo aAACInfo) 
    {
    
    CreateAACAudDecL(iDecHandle, static_cast<TInt16>(aAACInfo.iNumChannels), 
                                static_cast<TInt16>(aAACInfo.iNumCouplingChannels));
    
    InitAACAudDec(iDecHandle, static_cast<TInt16>(aAACInfo.iProfileID), 
                                static_cast<TInt16>(aAACInfo.iSampleRateID), 
                                static_cast<TUint8>(aAACInfo.iIs960));
    
    if(aAACInfo.isSBR)
        {
        uint8 isStereo;
        
        isStereo = (aAACInfo.iNumChannels > 1) ? 1 : 0;
        
        CreateAACPlusAudDecL(iDecHandle, static_cast<TInt16>(aAACInfo.iSampleRateID), isStereo, (uint8) 0);
        }
    iAACInfo = aAACInfo;
    
}

CProcAACFrameHandler::CProcAACFrameHandler() : iDecHandle(0)
{
    
}

TInt CProcAACFrameHandler::CalculateNumberOfHeaderBytes(const HBufC8* aFrame, TInt& aNumBlocksInFrame) const
    {

    if (aFrame->Size() < 7) return 0;
    TBuf8<7> possibleHeader(aFrame->Left(7));

    TUint8 byte2 = possibleHeader[1];
    TBuf8<8> byte2b;
    ProcTools::Dec2Bin(byte2, byte2b);
    TUint8 byte7 = possibleHeader[6];

    // lets confirm that we have found a legal AAC header
    if (possibleHeader[0] == 0xFF &&
        byte2b[0] == '1' &&
        byte2b[1] == '1' &&
        byte2b[2] == '1' &&
        byte2b[3] == '1' &&
        //    byte2b[4] == '1' &&
        byte2b[5] == '0' &&
        byte2b[6] == '0')
        {
        
        aNumBlocksInFrame = (byte7 & 0x3)+1;
        
        
        // protection_absent -> the last bit of the second byte
        if (byte2b[7] == '0')
            {
            return 9 + 2*(aNumBlocksInFrame-1);
            }
        else
            {
            return 7;
            }
        
        
        


        }
    else
        {
        // it seems like a raw data block
        return 0;



        }
    }



TBool CProcAACFrameHandler::ParseFramesL(HBufC8* aFrame, RArray<TInt>& aFrameStarts, RArray<TInt>& aFrameLengths)
{
    if (iFrameStarts.Count() > 0)
    {
        for (TInt a = 0 ; a < iFrameStarts.Count() ; a++)
        {
            aFrameStarts.Append(iFrameStarts[a]);
            aFrameLengths.Append(iFrameLengths[a]);
        }    
        iFrameStarts.Reset();
        iFrameLengths.Reset();
        return ETrue;
    }
    else
    {
        
        TInt numBlocks = 1;
        TInt headerBytes = CalculateNumberOfHeaderBytes(aFrame, numBlocks);
        if (headerBytes > 0)
        {
            //TBuf8<headerBytes> hB(aFrame->Left(headerBytes));
            
            
        }
        
        //TBitStream bs;
        TUint8* buf = const_cast<TUint8*>(aFrame->Right(aFrame->Size()-headerBytes).Ptr());
        //BsInit(&bs, buf, aFrame->Size()-headerBytes);
        
        uint8* gains = new (ELeave) uint8[16];
        CleanupStack::PushL(gains);
        uint32* gainPos = new (ELeave) uint32[16];
        CleanupStack::PushL(gainPos);
        
        //TPtr8 frameWithoutHeader = aFrame->Right(aFrame->Size()-headerBytes));
        TInt bufLen = aFrame->Size()-headerBytes;
        
        TBitStream bs;
    
        BsInit(&bs, buf, bufLen);
    
        for (TInt b = 0 ; b < numBlocks ; b++)
        {
            
            TInt b_i = bs.buf_index;
            iFrameStarts.Append(b_i+headerBytes);
            
            uint8 numberOfGains = GetAACGlobalGains(&bs, iDecHandle, 16, gains, gainPos);
            if (numberOfGains > 2) 
                {
                CleanupStack::Pop(); // gainPos
                CleanupStack::Pop(); // gains
                delete[] gains;
                delete[] gainPos;
                return EFalse;

                }
            
            if (bs.buf_index > 0)
                iFrameLengths.Append(bs.buf_index - b_i);
            else
                iFrameLengths.Append(bs.buf_len - b_i);
            
            if (headerBytes > 7)
            {
                bs.buf_index += 2; // crc
                bs.slots_read += 2;
                bs.bits_read += 16;
                
            }
                        
        }
        CleanupStack::Pop(); // gainPos
        CleanupStack::Pop(); // gains
        delete[] gains;
        delete[] gainPos;

        
        
        for (TInt c = 0 ; c < iFrameStarts.Count() ; c++)
            {
            aFrameStarts.Append(iFrameStarts[c]);
            aFrameLengths.Append(iFrameLengths[c]);
            }    
        iFrameStarts.Reset();
        iFrameLengths.Reset();
        return ETrue;    
        
    }
    
    
    
    
}



TBool CProcAACFrameHandler::UpdateHeaderL(HBufC8* aFrame)
    {

    _LIT8(KZero, "0");

    TInt frameLength = aFrame->Size();

    HBufC8* lenBin;
    ProcTools::Dec2BinL(frameLength, lenBin);
    
    HBufC8* len13Bin = HBufC8::NewL(13);
    TInt zerosNeeded = 13-lenBin->Size();
    
    TPtr8 framePtr(aFrame->Des());
    for (TInt w = 0 ; w < zerosNeeded ; w++)
        {
        len13Bin->Des().Append(_L8("0"));
        }
    len13Bin->Des().Append(lenBin->Des());

    if (len13Bin->Mid(0,1).Compare(KZero) == 0)
        {
        framePtr[3] &= 0xFD; // 1111 1101
        }
    else
        {
        framePtr[3] |= 2;
        }

    if (len13Bin->Mid(1,1).Compare(KZero) == 0)
        {
        framePtr[3] &= 0xFE; // 1111 1110

        }
    else
        {
        framePtr[3] |= 1;
        }


    TUint byte5 = 0;
    ProcTools::Bin2Dec(len13Bin->Mid(2,8),byte5);
    framePtr[4] = static_cast<TUint8>(byte5);

    if (len13Bin->Mid(10,1).Compare(KZero) == 0)
        {
        framePtr[5] &= 0x7F;

        }
    else
        {
        framePtr[5] |= 0x80;
        }

    if (len13Bin->Mid(11,1).Compare(KZero) == 0)
        {
        framePtr[5] &= 0xBF;

        }
    else
        {
        framePtr[5] |= 0x40;
        }

    if (len13Bin->Mid(12,1).Compare(KZero) == 0)
        {
        framePtr[5] &= 0xDF;

        }
    else
        {
        framePtr[5] |= 0x20;
        }
    delete lenBin;
    delete len13Bin;
    return ETrue;
    }



void CProcAACFrameHandler::
GetEnhancedAACPlusParametersL(TUint8* buf, TInt bufLen, 
                              TAudFileProperties* aProperties, 
                              TAACFrameHandlerInfo *aAACInfo)
{
  TBitStream bs;
  uint8 sbrStatus;
  int16 bytesInFrame;
  CAACAudDec* decHandle = 0;

  //-- No SBR by default. --//
  aAACInfo->isSBR = 0;
  aAACInfo->iIsParametricStereo = 0;
  aProperties->iAudioTypeExtension = EAudExtensionTypeNoExtension;
  aProperties->iChannelModeExtension = EAudChannelModeNotRecognized;

  //-- Create AAC handle. --//
  CreateAACAudDecL(decHandle, 
                   static_cast<TInt16>(aAACInfo->iNumChannels), 
                   static_cast<TInt16>(aAACInfo->iNumCouplingChannels));
  CleanupStack::PushL(decHandle);

  //-- Initialize AAC handle. --/
  InitAACAudDec(decHandle, 
                static_cast<TInt16>(aAACInfo->iProfileID), 
                                static_cast<TInt16>(aAACInfo->iSampleRateID), 
                                static_cast<TUint8>(aAACInfo->iIs960));

  //-- Create SBR handle on top of the AAC handle. --
  sbrStatus = CreateAACPlusAudDecL(decHandle, static_cast<TInt16>(aAACInfo->iSampleRateID), 
                                   (uint8) ((aAACInfo->iNumChannels > 1) ? 1 : 0), 
                                   (uint8) 0);

  if(sbrStatus)
  {
    //-- Initialize bitstream. --
    BsInit(&bs, buf, bufLen);

    //-- Parse the AAC frame. --/
    CountAACChunkLength(&bs, decHandle, &bytesInFrame);

    //-- Were any SBR elements found? --
    if(IsAACSBREnabled(decHandle))
    {
      aAACInfo->isSBR = 1;
      aProperties->iAudioTypeExtension = EAudExtensionTypeEnhancedAACPlus;
      
      aAACInfo->iIsParametricStereo = IsAACParametricStereoEnabled(decHandle);

      if(aAACInfo->iIsParametricStereo)
      {
        aProperties->iChannelModeExtension = EAudParametricStereoChannel;
        aProperties->iAudioTypeExtension = EAudExtensionTypeEnhancedAACPlusParametricStereo;
      }
    }
  }


  //-- Delete resources. --/
  CleanupStack::Pop(decHandle);
  if(decHandle != 0)
      {
      DeleteAACAudDec(decHandle);    
      }
    
  decHandle = 0;
}
