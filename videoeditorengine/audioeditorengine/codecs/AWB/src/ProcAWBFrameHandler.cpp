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




#include "ProcAWBFrameHandler.h"
#include "ProcFrameHandler.h"
#include "ProcTools.h"
#include "q_gain2.h"
#include "AWBConstants.h"

#include <f32file.h>
#include <e32math.h>


TBool CProcAWBFrameHandler::ManipulateGainL(const HBufC8* aFrameIn, HBufC8*& aFrameOut, TInt8 aGain) 
    {
    
    aFrameOut = HBufC8::NewLC(aFrameIn->Size());
    aFrameOut->Des().Copy(aFrameIn->Ptr(), aFrameIn->Size());

    aGain = static_cast<TInt8>(aGain/2);

    const TUint8 modeByte = (*aFrameOut)[0];
    
    TUint8 toc = modeByte;
    TUint8 mode = static_cast<TUint8>((toc >> 3) & 0x0F);
            
    TInt bitrate = KAWBBitRates[mode];
    
    RArray<TInt> gains;
    CleanupClosePushL(gains);
    TInt maxGain = 0;
    GetAWBGainsL(aFrameOut, gains, maxGain);

    TInt gamma10000 = KAwbGain_dB2Gamma[aGain+127];

    TInt a = 0;
    TInt old_gain = 0;
    TInt old_pitch = 0;
    TInt new_gain = 0;
    TInt8 new_index = 0;
    TInt bitRateIndex = -1;

    switch (bitrate)
        {

        case 23850:
        case 23050:
        case 19850:
        case 18250:
        case 15850:
        case 14250:
        case 12650:
            {

            for (a = 0; a < gains.Count()-1 ; a+=2) 
                {
        
                old_gain = gains[a+1];
                new_gain = static_cast<TInt>((gamma10000*old_gain)/10000);
                old_pitch = gains[a];

                new_index = ProcTools::FindNewIndexVQ2(new_gain, old_pitch, 
                                                    t_qua_gain7b, 
                                                    2*128);

            
                bitRateIndex = ProcTools::FindIndex(bitrate, KAWBBitRates, 9);


                if        (a == 0) ProcTools::SetValueToShuffledAWBFrame(new_index, 
                                                                       aFrameOut, 
                                                                       bitrate,
                                                                       KGainPositions[bitRateIndex*4], 7);
                else if (a == 2) ProcTools::SetValueToShuffledAWBFrame(new_index, 
                                                                       aFrameOut, 
                                                                       bitrate,
                                                                       KGainPositions[bitRateIndex*4+1], 7);
                else if (a == 4) ProcTools::SetValueToShuffledAWBFrame(new_index, 
                                                                       aFrameOut, 
                                                                       bitrate,
                                                                       KGainPositions[bitRateIndex*4+2], 7);
                else if (a == 6) ProcTools::SetValueToShuffledAWBFrame(new_index, 
                                                                       aFrameOut, 
                                                                       bitrate,
                                                                       KGainPositions[bitRateIndex*4+3], 7);
                
            }

    

            break;
            }

        case 8850:
        case 6600:
            {
            for (a = 0; a < gains.Count()-1 ; a+=2) 
                {
        
                old_gain = gains[a+1];
                new_gain = static_cast<TInt>((gamma10000*old_gain)/10000);
                old_pitch = gains[a];

                new_index = ProcTools::FindNewIndexVQ2(new_gain, old_pitch, 
                                                    t_qua_gain6b, 
                                                    2*64);

                bitRateIndex = ProcTools::FindIndex(bitrate, KAWBBitRates, 9);


                if        (a == 0) ProcTools::SetValueToShuffledAWBFrame(new_index, 
                                                                       aFrameOut, 
                                                                       bitrate,
                                                                       KGainPositions[bitRateIndex*4], 6);
                else if (a == 2) ProcTools::SetValueToShuffledAWBFrame(new_index, 
                                                                       aFrameOut, 
                                                                       bitrate,
                                                                       KGainPositions[bitRateIndex*4+1], 6);
                else if (a == 4) ProcTools::SetValueToShuffledAWBFrame(new_index, 
                                                                       aFrameOut, 
                                                                       bitrate,
                                                                       KGainPositions[bitRateIndex*4+2], 6);
                else if (a == 6) ProcTools::SetValueToShuffledAWBFrame(new_index, 
                                                                       aFrameOut, 
                                                                       bitrate,
                                                                       KGainPositions[bitRateIndex*4+3], 6);
                }

            break;
            }
        }

    CleanupStack::PopAndDestroy(&gains);
    CleanupStack::Pop(); // aFrameOut
    return ETrue;
    }

TBool CProcAWBFrameHandler::GetGainL(const HBufC8* aFrame, RArray<TInt>& aGains, TInt& aMaxGain) const
    {

    RArray<TInt> AWBGains;
    CleanupClosePushL(AWBGains);
    GetAWBGainsL(aFrame, AWBGains, aMaxGain);
    
    for (TInt a = 0; a < AWBGains.Count() ; a++)
        {
        if (a%2 == 1)
            {
            aGains.Append(AWBGains[a]);
            }

        }
    
    CleanupStack::PopAndDestroy(&AWBGains);

    return ETrue;
    }


TBool CProcAWBFrameHandler::GetNormalizingMargin(const HBufC8* aFrame, TInt8& aMargin) const

    {

    RArray<TInt> gains;

    TInt error = KErrNone;
    TRAP( error, CleanupClosePushL(gains) );
    
    if (error != KErrNone)
        return EFalse;
    
    TInt maxGain;
    
    TRAPD(err, GetAWBGainsL(aFrame, gains, maxGain));
    
    if (err != KErrNone)
        {
        return EFalse;
        }
    
    TInt largestGain = 1;

    for (TInt a = 1 ; a < gains.Count() ; a+=2)
        {
        if (gains[a] > largestGain) 
            {
            largestGain = gains[a];
            }

        }
    TInt gamma10000 = (32767*10000)/largestGain;


    CleanupStack::PopAndDestroy(&gains);

    TUint8 newIndex = ProcTools::FindNewIndexSQ(gamma10000, KAwbGain_dB2Gamma, 256);

    TInt8 newGain = static_cast<TInt8>(newIndex-127); 

    if (newGain > 63)
        {
        newGain = 63;
        }
    else if (newGain < -63)
        {
        newGain = -63;
        }

    aMargin = static_cast<TInt8>(newGain*2);
    // aMargin is now in dB/2:s
    return ETrue;

    }

CProcAWBFrameHandler::~CProcAWBFrameHandler() 
    {

        }

CProcAWBFrameHandler* CProcAWBFrameHandler::NewL() 
    {

    
    CProcAWBFrameHandler* self = NewLC();
    CleanupStack::Pop(self);
    return self;

    }
CProcAWBFrameHandler* CProcAWBFrameHandler::NewLC() 
    {

    CProcAWBFrameHandler* self = new (ELeave) CProcAWBFrameHandler();
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;

    }

void CProcAWBFrameHandler::ConstructL() 
    {

    }

CProcAWBFrameHandler::CProcAWBFrameHandler()
    {

    }
 
TBool CProcAWBFrameHandler::GetAWBGainsL(const HBufC8* aFrame, 
                                        RArray<TInt>& aGains, 
                                        TInt& aMaxGain) const

    {

    const TUint8 modeByte = (*aFrame)[0];

    TUint8 toc = modeByte;
    TUint8 mode = static_cast<TUint8>((toc >> 3) & 0x0F);
            
    TInt bitrate = KAWBBitRates[mode];
    TInt cbIndex = -1;
    TInt a = 0;

    switch (bitrate)
        {

        case 23850:
            {

            for (a = 0 ; a < 4 ; a++)
                {
                cbIndex = ProcTools::GetValueFromShuffledAWBFrameL(aFrame, bitrate, 
                                                                KGainPositions[TAWBBitRate23850*4+a], 7);
                TInt pitch = t_qua_gain7b[cbIndex*2]; 
                TInt gain = t_qua_gain7b[cbIndex*2+1];
                aGains.Append(pitch);
                aGains.Append(gain);
                }

            break;
            }

        case 23050:
            {
            for (a = 0 ; a < 4 ; a++)
                {
                cbIndex = ProcTools::GetValueFromShuffledAWBFrameL(aFrame, bitrate, 
                                                                KGainPositions[TAWBBitRate23050*4+a], 7);
                TInt pitch = t_qua_gain7b[cbIndex*2];
                TInt gain = t_qua_gain7b[cbIndex*2+1];
                aGains.Append(pitch);
                aGains.Append(gain);
                }
            break;
            }
        case 19850:
            {
            for (a = 0 ; a < 4 ; a++)
                {
                cbIndex = ProcTools::GetValueFromShuffledAWBFrameL(aFrame, bitrate, 
                                                                KGainPositions[TAWBBitRate19850*4+a], 7);
                TInt pitch = t_qua_gain7b[cbIndex*2];
                TInt gain = t_qua_gain7b[cbIndex*2+1];
                aGains.Append(pitch);
                aGains.Append(gain);
                }
            break;
            }
        case 18250:
            {
            for (a = 0 ; a < 4 ; a++)
                {
            
                cbIndex = ProcTools::GetValueFromShuffledAWBFrameL(aFrame, bitrate, 
                                                                KGainPositions[TAWBBitRate18250*4+a], 7);
                TInt pitch = t_qua_gain7b[cbIndex*2];
                TInt gain = t_qua_gain7b[cbIndex*2+1];
                aGains.Append(pitch);
                aGains.Append(gain);
                }
            break;
            }
        case 15850:
            {
            for (a = 0 ; a < 4 ; a++)
                {
                cbIndex = ProcTools::GetValueFromShuffledAWBFrameL(aFrame, bitrate, 
                                                                KGainPositions[TAWBBitRate15850*4+a], 7);
                TInt pitch = t_qua_gain7b[cbIndex*2];
                TInt gain = t_qua_gain7b[cbIndex*2+1];
                aGains.Append(pitch);
                aGains.Append(gain);
                }

            break;
            }
        case 14250:
            {
            for (a = 0 ; a < 4 ; a++)
                {
                cbIndex = ProcTools::GetValueFromShuffledAWBFrameL(aFrame, bitrate, 
                                                                KGainPositions[TAWBBitRate14250*4+a], 7);
                TInt pitch = t_qua_gain7b[cbIndex*2];
                TInt gain = t_qua_gain7b[cbIndex*2+1];
                aGains.Append(pitch);
                aGains.Append(gain);
                }

            break;
            }
        case 12650:
            {
            for (a = 0 ; a < 4 ; a++)
                {
                cbIndex = ProcTools::GetValueFromShuffledAWBFrameL(aFrame, bitrate, 
                                                                KGainPositions[TAWBBitRate12650*4+a], 7);
                TInt pitch = t_qua_gain7b[cbIndex*2];
                TInt gain = t_qua_gain7b[cbIndex*2+1];
                aGains.Append(pitch);
                aGains.Append(gain);
                }

            break;
            }
        case 8850:
            {
            for (a = 0 ; a < 4 ; a++)
                {
                cbIndex = ProcTools::GetValueFromShuffledAWBFrameL(aFrame, bitrate, 
                                                                KGainPositions[TAWBBitRate8850*4+a], 6);
                TInt pitch = t_qua_gain6b[cbIndex*2];
                TInt gain = t_qua_gain6b[cbIndex*2+1];
                aGains.Append(pitch);
                aGains.Append(gain);
                }

            break;
            }
        case 6600:
            {
            for (a = 0 ; a < 4 ; a++)
                {
                cbIndex = ProcTools::GetValueFromShuffledAWBFrameL(aFrame, bitrate, 
                                                                KGainPositions[TAWBBitRate6600*4+a], 6);
                TInt pitch = t_qua_gain6b[cbIndex*2];
                TInt gain = t_qua_gain6b[cbIndex*2+1];
                aGains.Append(pitch);
                aGains.Append(gain);
                }

            break;
            }

        }
    aMaxGain = 32767;
    
    return ETrue;


    }
    
