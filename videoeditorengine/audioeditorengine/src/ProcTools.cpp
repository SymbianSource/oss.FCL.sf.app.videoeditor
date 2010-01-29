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



#include "ProcTools.h"
#include "AudPanic.h"

#include "mime_io.h"


TBool ProcTools::Dec2Bin(TUint8 aDec, TBuf8<8>& aBinary) 
    {

    // clear aBinary just in case
    aBinary.Delete(0, aBinary.Length());

    aBinary.AppendNum(aDec, EBinary);

    TInt zerosNeeded = 0;
    if (aBinary.Length() == 8) 
        {
        // the MSB is one -> no padding needed
        }
    else 
        {
            
        zerosNeeded = 8 - aBinary.Length();
        for(TInt as = 0 ; as < zerosNeeded ; as++) 
            {
            aBinary.AppendNum(0);
            }

        for (TInt tr = 8 - 1 ; tr >= 0 ; tr--) 
            {

            if (tr >= zerosNeeded) 
                { 
                aBinary[tr] = aBinary[tr-zerosNeeded];
                }
            else 
                {
                aBinary[tr] = '0';
                }
        
            }
        }


    return ETrue;
    }

TBool ProcTools::Dec2BinL(TUint32 aDec, HBufC8*& aBin)
    {
    
    // 32 bits, the leftmost is one
    TUint32 bitMask = 0x80000000;

    TBool onlyZeros = ETrue;
    for (TInt a = 32 ; a > 0 ; a--)
        {
        TUint32 res = bitMask & aDec;
        
        if (res > 0 && onlyZeros)
            {
            // the first one from left found
            onlyZeros = EFalse;
            aBin = HBufC8::NewL(a);
            aBin->Des().Append('1');
            }
        else if (res > 0 && !onlyZeros)
            {
            aBin->Des().Append('1');
            }
        else if (res == 0 && !onlyZeros)
            {    
            aBin->Des().Append('0');
        
            }
        bitMask >>= 1;
        }

    return ETrue;

    }

TBool ProcTools::Bin2Dec(const TDesC8& aBin, TUint& aDec) 
    {

    TLex8 leks(aBin);

    if (leks.Val(aDec, EBinary) != KErrNone) 
        {
        return EFalse;
        }
    return ETrue;

    }

TBool ProcTools::Des2Dec(TDesC8& aDes, TUint& aDec) 
    {

    TLex8 leks(aDes);

    if (leks.Val(aDec, EDecimal) != KErrNone) 
        {
        return EFalse;
        }
    return ETrue;


    }

TBool ProcTools::Des2BinL(const TDesC8& aDes, HBufC8*& aBin) 
    {

    aBin = HBufC8::NewL(aDes.Length()*8);
    // clear aBinary just in case
    
    for (TInt a = 0 ; a < aDes.Length() ; a++) 
        {
    
        TUint8 chDec = aDes[a];

        aBin->Des().AppendNum(chDec, EBinary);
        
        TInt zerosNeeded = 0;
        if (aBin->Des().Length() == (a+1)*8) 
            {
            // the MSB is one -> no padding needed
            }
        else 
            {
            
            zerosNeeded = (a+1)*8 - aBin->Des().Length();
            for(TInt as = 0 ; as < zerosNeeded ; as++) 
                {
                aBin->Des().Insert(a*8, _L8("0"));
                //AppendNum(0);
                }
           
            }
        }


    return ETrue;

    }
    
TInt ProcTools::MilliSeconds(TTimeIntervalMicroSeconds aMicroSeconds)
    {
    
#ifndef EKA2
    return (aMicroSeconds.Int64()/1000).GetTInt();
#else
    return (aMicroSeconds.Int64()/1000);
#endif
    
    }

TTimeIntervalMicroSeconds ProcTools::MicroSeconds(TInt aMilliSeconds)
    {
    
    TTimeIntervalMicroSeconds mic(aMilliSeconds*1000);
    return mic;
    }
    
TInt ProcTools::GetTInt(TInt64 aTInt64)
    {

#ifndef EKA2
    return aTInt64.GetTInt();
#else        
    return aTInt64;
#endif
        
    }

TInt ProcTools::GetValueFromShuffledFrame(const HBufC8* aFrame, 
                                          const TUint8 aBitPositions[], 
                                          const TInt aSize) 
    {

    HBufC8* inBytes = 0;
    TRAPD(err, inBytes = HBufC8::NewL(aSize));
    if (err != KErrNone)
        {
        inBytes = 0;
        return -1;
        }
    
    
    _LIT8(KZero, "0");
    _LIT8(KOne, "1");


    for (TInt a = 0 ; a < aSize ; a++) 
        {
        
        TUint8 mask = 0x80; // 1000 0000b
        TUint byteNumber = aBitPositions[a]/8;
        TUint bitNumber = aBitPositions[a]%8;
        
        const TUint8 byteNow = (*aFrame)[byteNumber];

        mask >>= bitNumber;

        //TUint8 masked = byteNow & mask;
        // why casting is needed, dunno?

        TUint8 masked = static_cast<TUint8>(byteNow & mask);

        if (masked == 0) 
            {
            inBytes->Des().Append(KZero);
            }
        else 
            {
            inBytes->Des().Append(KOne);
            }
        }
    TUint dec = 0;
    Bin2Dec(inBytes->Des(), dec);
    delete inBytes;
    inBytes = 0;
    return dec;


    }



TInt ProcTools::GetValueFromShuffledAWBFrameL(const HBufC8* aFrame, TInt aBitRate, TInt aBitPosition, TInt aLength)
    {

    HBufC8* inBytes = HBufC8::NewLC(aLength);
    
    
    _LIT8(KZero, "0");
    _LIT8(KOne, "1");

    const TInt* table = 0;
    TInt tableSize = 0;

    switch(aBitRate)
        {
        case(23850):
            {
            table = sort_2385;
            tableSize = 477;
            break;
            }
        case(23050):
            {
            table = sort_2305;
            tableSize = 461;
            break;
            }
        case(19850):
            {
            table = sort_1985;
            tableSize = 397;
            break;
            }
        case(18250):
            {
            table = sort_1825;
            tableSize = 365;
            break;
            }
        case(15850):
            {
            table = sort_1585;
            tableSize = 317;
            break;
            }
        case(14250):
            {
            table = sort_1425;
            tableSize = 285;
            break;
            }
        case(12650):
            {
            table = sort_1265;
            tableSize = 253;
            break;
            }
        case(8850):
            {
            table = sort_885;
            tableSize = 177;
            break;
            }
        case(6600):
            {
            table = sort_660;
            tableSize = 132;
            break;
            }
        default:
            {
            // illegal bitrate
            CleanupStack::PopAndDestroy(inBytes);
            return -1;
            }

        }


    for (TInt a = 0 ; a < aLength ; a++) 
        {
        TInt bitIndex = FindIndex(aBitPosition+a, table, tableSize)+8;

        TUint8 mask = 0x80; // 1000 0000b
        TUint byteNumber = bitIndex/8;
        TUint bitNumber = bitIndex%8;
        
        const TUint8 byteNow = (*aFrame)[byteNumber];

        mask >>= bitNumber;

        //TUint8 masked = byteNow & mask;
        // why casting is needed, dunno?

        TUint8 masked = static_cast<TUint8>(byteNow & mask);

        if (masked == 0) 
            {
            inBytes->Des().Append(KZero);
            }
        else 
            {
            inBytes->Des().Append(KOne);
            }
        }

    TUint dec = 0;
    Bin2Dec(inBytes->Des(), dec);
    CleanupStack::PopAndDestroy(inBytes);

    inBytes = 0;
    return dec;

    }


TBool ProcTools::SetValueToShuffledAWBFrame(TUint8 aNewValue, HBufC8* aFrame, 
                                            TInt aBitRate, TInt aBitPosition, TInt aLength)
    {

    const TInt* table = 0;
    TInt tableSize = 0;

    switch(aBitRate)
        {
        case(23850):
            {
            table = sort_2385;
            tableSize = 477;
            
            break;
            }
        case(23050):
            {
            table = sort_2305;
            tableSize = 461;
            
            break;
            }
        case(19850):
            {
            table = sort_1985;
            tableSize = 397;
            
            break;
            }
        case(18250):
            {
            table = sort_1825;
            tableSize = 365;
            
            break;
            }
        case(15850):
            {
            table = sort_1585;
            tableSize = 317;
            
            break;
            }
        case(14250):
            {
            table = sort_1425;
            tableSize = 285;
            
            break;
            }
        case(12650):
            {
            table = sort_1265;
            tableSize = 253;
            
            break;
            }
        case(8850):
            {
            table = sort_885;
            tableSize = 177;
            
            break;
            }
        case(6600):
            {
            table = sort_660;
            tableSize = 132;
            
            break;
            }
        default:
            {
            // illegal bitrate
            return -1;
            }

        }


    _LIT8(KZero, "0");

    TBuf8<8> newValueBin;
    Dec2Bin(aNewValue, newValueBin);
    // remove leading zeros
    newValueBin.Delete(0, 8-aLength);



    TPtr8 framePtr(aFrame->Des());

    for (TInt a = 0 ; a < newValueBin.Size() ; a++) 
        {

        TInt bitIndex = FindIndex(aBitPosition+a, table, tableSize)+8;

        TUint8 bitPositionInByte = static_cast<TUint8>(bitIndex%8);
        TUint byteNumber = bitIndex/8;

        if (newValueBin.Mid(a,1).Compare(KZero) == 0) 
            {

            TUint8 mask = 0x80; // 0111 1111b
            mask >>= bitPositionInByte;
            mask = static_cast<TUint8>(~mask);

            //--->
//            TUint8 oldByte = framePtr[byteNumber];
    //        TUint8 newByte = oldByte & mask;
            //<--

            framePtr[byteNumber] = static_cast<TUint8>(framePtr[byteNumber] & mask);  
    
            }
        else 
            {

            TUint8 mask = 0x80; // 1000 0000b
            mask >>= bitPositionInByte;
            
            //--->
//            TUint8 oldByte = framePtr[byteNumber];
//            TUint8 newByte = oldByte & mask;
            //<--
            framePtr[byteNumber] = static_cast<TUint8>(framePtr[byteNumber] | mask);

            }
        
        }

    return EFalse;
    }

TBool ProcTools::SetValueToShuffledFrame(HBufC8* aFrame, TUint8 aNewValue, 
                                         const TUint8 aBitPositions[], 
                                         TInt aSize) 
    {

    
    _LIT8(KZero, "0");
    
    TBuf8<8> newValueBin;
    Dec2Bin(aNewValue, newValueBin);
    newValueBin.Delete(0, 8-aSize);

    TPtr8 framePtr(aFrame->Des());

    for (TInt a = 0 ; a < aSize ; a++) 
        {

        TUint8 bitPosition = static_cast<TUint8>(aBitPositions[a]%8);
        TUint byteNumber = aBitPositions[a]/8;

        if (newValueBin.Mid(a,1).Compare(KZero) == 0) 
            {

            TUint8 mask = 0x80; // 0111 1111b
            mask >>= bitPosition;
            mask = static_cast<TUint8>(~mask);

            //--->
//            TUint8 oldByte = framePtr[byteNumber];
    //        TUint8 newByte = oldByte & mask;
            //<--

            framePtr[byteNumber] = static_cast<TUint8>(framePtr[byteNumber] & mask);  
    
            }
        else 
            {

            TUint8 mask = 0x80; // 1000 0000b
            mask >>= bitPosition;
            
            //--->
//            TUint8 oldByte = framePtr[byteNumber];
//            TUint8 newByte = oldByte & mask;
            //<--
            framePtr[byteNumber] = static_cast<TUint8>(framePtr[byteNumber] | mask);

            }


        
        }
    
    return ETrue;

    }



TBool ProcTools::WriteValuesToFileL(const RArray<TInt>& aArray, const TDesC& aFilename) 
    {

    RFs fs;
    User::LeaveIfError(fs.Connect());

    RFile file;
    TInt err=file.Open(fs, aFilename, EFileWrite);
    
    if (err==KErrNotFound) 
        {
        err=file.Create(fs, aFilename, EFileWrite);
        }
    TInt nolla = 0;
    file.Seek(ESeekEnd, nolla);
    for (TInt a = 0; a < aArray.Count() ; a++) 
        {
        TBuf8<8> num;
        num.AppendNum(aArray[a]);
        file.Write(num);
        file.Write(_L8("\n"));


        }

    file.Close();
    fs.Close();

    return ETrue;
    }


TUint8 ProcTools::FindNewIndexSQ(TInt aNewGain, const TInt aGainTable[], TInt aTableSize) 
    {

    if (aTableSize > 256)
        {
        TAudPanic::Panic(TAudPanic::EInternal);
        }

    TUint8 tableSize = static_cast<TUint8>(aTableSize-1);

    TInt minimum = 0xFFFF;
    TUint8 indexFound = 0;

    for (TUint a = 0 ; a <= tableSize ; a++) 
        {
            
        if (Abs(aGainTable[a]-aNewGain) < minimum) 
            {
            minimum = Abs(aGainTable[a]-aNewGain);
            indexFound = static_cast<TUint8>(a);
            }
        }

    return indexFound;

    }

TUint8 ProcTools::FindNewIndexVQ(TInt aNewGain, TInt aOldPitch, const TInt aGainTable[], TInt aTableSize) 
    {

    if (aTableSize > 256)
        {
        TAudPanic::Panic(TAudPanic::EInternal);
        }

    TUint8 tableSize = static_cast<TUint8>(aTableSize-1);



    TInt minimum = KMaxTInt;
    TUint8 indexFound = 0;

    for (TUint a = 0 ; a <= tableSize ; a+=2) 
        {

        // gpitch: Q14 , 2^14 = 16384
        TInt distance_pitch = (100*(Abs(aGainTable[a]-aOldPitch)))/16384;

        // g_fac: Q12 , 2^12 = 4096
        TInt distance_fc = (100*Abs(aGainTable[a+1]-aNewGain))/4096;
        

        TInt distance = distance_pitch*distance_pitch + distance_fc*distance_fc;
        
        if (distance < minimum) 
            {
            minimum = distance;
            indexFound = static_cast<TUint8>(a);
            }
        }

    return static_cast<TUint8>(indexFound/2);

    }

TUint8 ProcTools::FindNewIndexVQ2(TInt aNewGain, TInt aOldPitch, const TInt aGainTable[], TInt aTableSize)
    {

    if (aTableSize > 256)
        {
        TAudPanic::Panic(TAudPanic::EInternal);
        }

    TUint8 tableSize = static_cast<TUint8>(aTableSize-1);

    TInt minimum = KMaxTInt;
    TUint8 indexFound = 0;

    for (TUint a = 0 ; a <= tableSize ; a+=2) 
        {

        // gpitch: Q14 , 2^14 = 16384
        TInt distance_pitch = (100*(Abs(aGainTable[a]-aOldPitch)))/16384;

        // g_fac: Q11 , 2^11 = 2048
        TInt distance_fc = (100*Abs(aGainTable[a+1]-aNewGain))/2048;
        

        TInt distance = distance_pitch*distance_pitch + distance_fc*distance_fc;
        
        if (distance < minimum) 
            {
            minimum = distance;
            indexFound = static_cast<TUint8>(a);
            }
        }

    return static_cast<TUint8>(indexFound/2);

    


    }


TUint8 ProcTools::FindNewIndex475VQ(TInt aNewGain0, TInt aOldPitch0, TInt aNewGain1, TInt aOldPitch1)
    {

    TInt minimum = KMaxTInt;
    TInt indexFound = -1;

    TInt tableSize = 256*4;


    for (TInt a = 0 ; a < tableSize-3 ; a+=4) 
        {

        // gpitch: Q14 , 2^14 = 16384
        TInt distance_pitch0 = (100*(Abs(KAmrGainTable475[a]-aOldPitch0)))/16384;
        TInt distance_pitch1 = (100*(Abs(KAmrGainTable475[a+2]-aOldPitch1)))/16384;

        // g_fac: Q12 , 2^12 = 4096
        TInt distance_fc0 = (100*Abs(KAmrGainTable475[a+1]-aNewGain0))/4096;
        TInt distance_fc1 = (100*Abs(KAmrGainTable475[a+3]-aNewGain1))/4096;        

        TInt distance = distance_pitch0*distance_pitch0 + distance_fc0*distance_fc0 +
                        distance_pitch1*distance_pitch1 + distance_fc1*distance_fc1;
        
        if (distance < minimum) 
            {
            minimum = distance;
            indexFound = a;
            }
        }

    return static_cast<TUint8>(indexFound/4);

    }

TInt ProcTools::FindIndex(TInt aKey, const TInt aBitPositions[], TInt aTableLength)
    {
    
    for (TInt a = 0 ; a < aTableLength ; a++)
        {
        if (aBitPositions[a] == aKey) return a;
        }

    return -1;

    }

TBool ProcTools::GenerateADTSHeaderL(TBuf8<7>& aHeader, TInt aFrameLength, TAudFileProperties aProperties)
    {
    TUint8 byte1 = 0xFF;

    TUint8 byte2 = 0x0;
    if (aProperties.iAudioType == EAudAAC_MPEG2)
        {
        byte2 = 0xF9;
        }
    else if (aProperties.iAudioType == EAudAAC_MPEG4)
        {
        byte2 = 0xF1;
        }
    else return EFalse;
    TBuf8<8> byte3b(8);
    TBuf8<8> byte4b(8);
    TBuf8<8> byte5b(8);
    TBuf8<8> byte6b(8);
    TBuf8<8> byte7b(8);

    byte3b.Fill('0');
    byte4b.Fill('0');
    byte5b.Fill('0');
    byte6b.Fill('0');
    byte7b.Fill('0');

    
    const TInt KAAC_SAMPLING_RATES[16] = {96000,88200,64000,48000,44100,32000,24000,
                                            22050,16000,12000,11025,8000,0,0,0,0};
    
    TBool srFound = EFalse;
    TUint8 srIndex = 0; 
    for (srIndex = 0 ; srIndex < 14 ; srIndex++)
        {
        if (KAAC_SAMPLING_RATES[srIndex] == aProperties.iSamplingRate) 
            {
            srFound = ETrue;
            break;
            }
        }

    if (srFound)
        {
        TBuf8<8> srB;
        ProcTools::Dec2Bin(srIndex, srB);
    
        // Sampling rate
        byte3b[2] = srB[4];
        byte3b[3] = srB[5];
        byte3b[4] = srB[6];
        byte3b[5] = srB[7];

        }

    

    // private bit
    byte3b[6] = '0';

    // channel configuration
    byte3b[7] = '0';
    if (aProperties.iChannelMode == EAudStereo)
        {
        byte4b[0] = '1';
        byte4b[1] = '0';

        }
    else if(aProperties.iChannelMode == EAudSingleChannel)
        {
        byte4b[0] = '0';
        byte4b[1] = '1';

        }
    else
        {
        return EFalse;
        }

    //original/copy & home
    byte4b[2] = '0';
    byte4b[3] = '0';
    // copyright identification & start
    byte4b[4] = '0';
    byte4b[5] = '0';

    HBufC8* lenBin = 0;
    if (ProcTools::Dec2BinL(aFrameLength+7, lenBin))
        {

        TInt fromRight = 0;
        for(TInt i = lenBin->Size()-1 ; i >= 0 ; i--)
            {
            if (fromRight < 3)
                {
                // byte7
                byte6b[2-fromRight] = lenBin->Des()[i];    
                }
            else if (fromRight < 11)
                {
                // byte6
                byte5b[10-fromRight] = lenBin->Des()[i];

                }
            else if (fromRight < 13)
                {
                // byte5
                byte4b[18-fromRight] = lenBin->Des()[i];
                }

            fromRight++;
            }

        delete lenBin;
        }
    
    TInt bitInd = 0; 
    for (bitInd = 3 ; bitInd < 8 ; bitInd++)
    {
        byte6b[bitInd] = '1';
    }
    for (bitInd = 0 ; bitInd < 6 ; bitInd++)
    {
        byte7b[bitInd] = '1';
    }
    
    
    aHeader.Append(byte1);
    aHeader.Append(byte2);

    TUint tmpByte = 0;
    ProcTools::Bin2Dec(byte3b, tmpByte);
    
    // profile
    TUint8 bitMask = aProperties.iAACObjectType;
    bitMask <<= 6;
    tmpByte = tmpByte | bitMask;

    aHeader.Append(static_cast<TUint8>(tmpByte));

    ProcTools::Bin2Dec(byte4b, tmpByte);
    aHeader.Append(static_cast<TUint8>(tmpByte));

    ProcTools::Bin2Dec(byte5b, tmpByte);
    aHeader.Append(static_cast<TUint8>(tmpByte));

    ProcTools::Bin2Dec(byte6b, tmpByte);
    aHeader.Append(static_cast<TUint8>(tmpByte));

    ProcTools::Bin2Dec(byte7b, tmpByte);
    aHeader.Append(static_cast<TUint8>(tmpByte));

    return ETrue;

    }

TInt ProcTools::GetNextAMRFrameLength(const HBufC8* aFrame, TInt aPosNow)
    {


    if (aPosNow >= aFrame->Size()) return -1;


    const TUint8 ch = (*aFrame)[aPosNow];

    TUint dec_mode = (enum Mode)((ch & 0x0078) >> 3);
    
    switch (dec_mode)
        {
        case 0:
            {
            return 12+1;
            }
            
        case 1:
            {
            return 13+1;
            }
            
        case 2:
            {
            return 15+1;
            }
            
        case 3:
            {
            return 17+1;
            }
            
        case 4:
            {
            return 19+1;
            }
            
        case 5:
            {
            return 20+1;
            }
            
        case 6:
            {
            return 26+1;
            }
            
        case 7:
            {
            return 31+1;
            }
            
        case 8:
            {
            return 5+1;
            }
            
        case 15:
            {
            return 0+1;
            }
            
        default:
            return 0+1;
            
        };
    

    }


CProcessingEvent* CProcessingEvent::NewL()
    {
    CProcessingEvent* self = new (ELeave) CProcessingEvent();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;

    }

void CProcessingEvent::ConstructL()
    {
    }

CProcessingEvent::~CProcessingEvent()
    {
    iAllIndexes.Close();
    }


CProcessingEvent::CProcessingEvent()
    {
    }

void CProcessingEvent::InsertIndex(TInt aIndex)
        {
        iAllIndexes.Append(aIndex);
        }

TInt CProcessingEvent::GetIndex(TInt aProcessingEventIndex)
    {
    return iAllIndexes[aProcessingEventIndex];
    }

TBool CProcessingEvent::GetAllIndexes(RArray<TInt>& aAllIndexes)
    {
    for (TInt a = 0 ; a < iAllIndexes.Count() ; a++)
        {
        aAllIndexes.Append(iAllIndexes[a]);

        }
    return ETrue;
    }

TInt CProcessingEvent::IndexCount()
    {
    return iAllIndexes.Count();
    }

TInt CProcessingEvent::FindIndex(TInt aClipIndex)
    {
    return iAllIndexes.Find(aClipIndex);
    }

void CProcessingEvent::RemoveIndex(TInt aProcessingEventIndex)
    {
    iAllIndexes.Remove(aProcessingEventIndex);
    }

    
TInt CProcessingEvent::Compare(const CProcessingEvent& c1, const CProcessingEvent& c2) 
    {
                
    if (c1.iPosition > c2.iPosition) 
        {
        return 1;
        }
    else if (c1.iPosition < c2.iPosition) 
        {
        return -1;
        }
    else 
        {
        return 0;
        }
    }
    

