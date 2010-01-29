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




#include "ProcWAVFrameHandler.h"
#include "ProcFrameHandler.h"
#include "ProcTools.h"
#include "ProcConstants.h"

#include <f32file.h>
#include <e32math.h>


TBool CProcWAVFrameHandler::ManipulateGainL(const HBufC8* aFrameIn, HBufC8*& aFrameOut, TInt8 aGain) 
    {
    TInt a = 0;

    aFrameOut = HBufC8::NewLC(aFrameIn->Size());
    
    aFrameOut->Des().Copy(aFrameIn->Ptr(), aFrameIn->Size());

    aGain = static_cast<TInt8>(aGain / 2);

    TPtr8 framePtr(aFrameOut->Des());
    if (iBitsPerSample == 8)
        {

        TReal multiplier = 0;
        TReal gaR(aGain);
        TReal exp = gaR/20;
        Math::Pow(multiplier, 10, exp);

        for (a = 0 ; a < aFrameOut->Length() ; a++)
            {
            TInt oldGain = (*aFrameOut)[a]-128;
            TInt newGain = 0;
            
            newGain = static_cast<TInt>(oldGain * multiplier);
            
            if (newGain > 128) 
                {
                newGain = 128;
                }
            else if (newGain < -128)
                {
                newGain = -128;
                }
        
            framePtr[a] = static_cast<TUint8>(newGain+128);
            
            }
        }
    else if (iBitsPerSample == 16)
        {

        TReal multiplier = 0;
        TReal gaR(aGain);
        TReal exp = gaR/20;
        Math::Pow(multiplier, 10, exp);


        for (a = 0 ; a < aFrameOut->Length()-1 ; a += 2)
            {
            
            TUint16 oldGain = static_cast<TUint16>((*aFrameOut)[a+1]*256 + (*aFrameOut)[a]);

            TBool negative = EFalse;

            if (oldGain > 32767) 
                {
                oldGain = static_cast<TUint16>(~oldGain+1);// - 65793;
                negative = ETrue;
                }

            TUint16 newGain = static_cast<TUint16>(oldGain * multiplier);

            if (newGain > 32727) 
                {
                newGain = 32727;
                }

            
            if (negative)
                {
                newGain = static_cast<TUint16>(~newGain-1);// - 65793;

                }
            framePtr[a+1] = static_cast<TUint8>(newGain/256);
            framePtr[a] = static_cast<TUint8>(newGain%256);

            

            }
        
        }


    CleanupStack::Pop(); // aFrameOut
    return ETrue;
    }

TBool CProcWAVFrameHandler::GetGainL(const HBufC8* aFrame, RArray<TInt>& aGains, TInt& aMaxGain) const
    {

    TInt a = 0;

    TInt highest = 0;

    if (iBitsPerSample == 8)
        {
        
        for (a = 0 ; a < aFrame->Length() ; a++)
            {


            if (Abs((*aFrame)[a]-128) > highest)
                highest = Abs((*aFrame)[a]-128);
    
            }
        }
    else if (iBitsPerSample == 16)
        {
        
        for (a = 0 ; a < aFrame->Length()-1 ; a += 2)
            {
            TInt ga = ((*aFrame)[a+1]*256 + (*aFrame)[a]);

            if (ga > 32767) ga-= 65792;



            if (ga > highest) highest = ga;

    
            }
        
        }


    aGains.Append(highest);

    if (iBitsPerSample == 8) aMaxGain = 128;
    else if (iBitsPerSample == 16) aMaxGain = 32767;


    return ETrue;
    }


TBool CProcWAVFrameHandler::GetNormalizingMargin(const HBufC8* aFrame, TInt8& aMargin) const

    {


    TInt maxGain = 0;
    TInt highestGain = GetHighestGain(aFrame, maxGain);


    TInt ma = ((maxGain - highestGain)*2)/5;
    aMargin = static_cast<TInt8>(ma);

    return ETrue;


    }

TBool CProcWAVFrameHandler::IsMixingAvailable() const
    {
    return ETrue;
    }
TBool CProcWAVFrameHandler::MixL(const HBufC8* aFrame1, const HBufC8* aFrame2, HBufC8*& aMixedFrame)
    {

    if (aFrame1->Length() != aFrame2->Length()) 
        {
        aMixedFrame = 0;
        return EFalse;
        }

    aMixedFrame = HBufC8::NewL(aFrame1->Length());

    TInt a = 0;
    TInt newGain = 0;
    
    if (iBitsPerSample == 8)
        {

        for (a = 0 ; a < aFrame1->Length() ; a++)
            {
        
            TInt oldGain1 = (*aFrame1)[a]-128;
            TInt oldGain2 = (*aFrame2)[a]-128;
            
            newGain = oldGain1 + oldGain2;
            
            if (newGain > 128) 
                {
                newGain = 128;
                }
            else if (newGain < -128)
                {
                newGain = -128;
                }
        
            aMixedFrame->Des().Append(static_cast<TUint8>(newGain+128));
            
            }
        }
    else if (iBitsPerSample == 16)
        {


        for (a = 0 ; a < aFrame1->Length()-1 ; a += 2)
            {
            
            TUint16 oldGain1 = static_cast<TUint16>((*aFrame1)[a+1]*256 + (*aFrame1)[a]);
            TUint16 oldGain2 = static_cast<TUint16>((*aFrame2)[a+1]*256 + (*aFrame2)[a]);


            TBool negative1 = EFalse;
            TBool negative2 = EFalse;

            if (oldGain1 > 32767) 
                {
                //oldGain1 = ~oldGain1+1;// - 65793;
                negative1 = ETrue;
//                oldGain1 = oldGain1+((65536-oldGain1)/2);

                }
            else
                {

//                oldGain1 /= 2; 

                }

            if (oldGain2 > 32767) 
                {
                //oldGain2 = ~oldGain2+1;// - 65793;
                negative2 = ETrue;
//                oldGain2 = oldGain2+((65536-oldGain2)/2);

                }
            else
                {
//                oldGain2 /= 2; 
                }


            newGain = static_cast<TUint16>(oldGain1 + oldGain2);


            if (negative1 && negative2)
                {
                if (newGain < 32767)
                    {    
                    newGain = 32768;
                    }
                }
            else if (!negative1 && !negative2)
                {
                if (newGain > 32767)
                    {    
                    newGain = 32767;
                    }
                
                }


            aMixedFrame->Des().Append(static_cast<TUint8>(newGain%256));
            aMixedFrame->Des().Append(static_cast<TUint8>(newGain/256));

            }
        
        }

    return ETrue;

    }



CProcWAVFrameHandler::~CProcWAVFrameHandler() 
    {

    }

CProcWAVFrameHandler* CProcWAVFrameHandler::NewL(TInt aBitsPerSample) 
    {

    
    CProcWAVFrameHandler* self = NewLC(aBitsPerSample);
    CleanupStack::Pop(self);
    return self;

    }
CProcWAVFrameHandler* CProcWAVFrameHandler::NewLC(TInt aBitsPerSample) 
    {

    CProcWAVFrameHandler* self = new (ELeave) CProcWAVFrameHandler(aBitsPerSample);
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;

    }

void CProcWAVFrameHandler::ConstructL() 
    {

    }

CProcWAVFrameHandler::CProcWAVFrameHandler(TInt aBitsPerSample) : iBitsPerSample(aBitsPerSample)
    {

    }
 
TInt CProcWAVFrameHandler::GetHighestGain(const HBufC8* aFrame, TInt& aMaxGain) const
    {

    TInt maxGain = 0;
    TInt a = 0;

    if (iBitsPerSample == 8)
        {
        aMaxGain = 210;
            
        for (a = 0 ; a < aFrame->Length() ; a++)
            {
            
            TInt gain = Abs((*aFrame)[a]-128);
            if (gain > maxGain) maxGain = gain;

            }
        }
    else if (iBitsPerSample == 16)
        {

        aMaxGain = 452;
        for (a = 0 ; a < aFrame->Length()-1 ; a += 2)
            {
            TInt ga = ((*aFrame)[a+1]*256 + (*aFrame)[a]);

            if (ga > 32767) ga-= 65792;

            ga = Abs(ga);
            
            if (ga > maxGain) maxGain = ga;

            }
        
        }

    TReal result = 0;
    TReal gai(maxGain);

    if (gai!= 0) Math::Log(result, gai);
    result *= 100;

    return (TInt) result;





    }
    
    
