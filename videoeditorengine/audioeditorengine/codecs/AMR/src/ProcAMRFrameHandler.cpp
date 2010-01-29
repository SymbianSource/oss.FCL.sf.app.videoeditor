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




#include "ProcAMRFrameHandler.h"
#include "ProcFrameHandler.h"
#include "ProcTools.h"

#include <e32math.h>


TBool CProcAMRFrameHandler::ManipulateGainL(const HBufC8* aFrameIn, HBufC8*& aFrameOut, TInt8 aGain) 
{
    
    aFrameOut = HBufC8::NewLC(aFrameIn->Size());
    
    if (aFrameIn->Size() == 1) // noData element
        {
        aFrameOut->Des().Append(aFrameIn[0]);
        CleanupStack::Pop(); // aFrameOut
    
        return ETrue;
        }
    

    aGain = static_cast<TInt8>(aGain/2);
    
    
    TInt posNow = 0;
    TInt nextFrameLength = 1;
    
    while (nextFrameLength > 0)
        {

        nextFrameLength = GetNextFrameLength(aFrameIn, posNow);
        if (nextFrameLength < 1) break;
        
        if (nextFrameLength + posNow > aFrameIn->Size())
            {
            // something wrong with the frame, return the original
            aFrameOut->Des().Append(aFrameIn[0]);
            CleanupStack::Pop(); // aFrameOut
            return ETrue;
            }
        
        HBufC8* tmpFrame = HBufC8::NewLC(nextFrameLength);
        tmpFrame->Des().Copy(aFrameIn->Mid(posNow, nextFrameLength));

        posNow += nextFrameLength;
        
        
        const TUint8 ch = (*tmpFrame)[0];
        
        TUint dec_mode = (enum Mode)((ch & 0x0078) >> 3);
        
        TInt maxGain;
        
        RArray<TInt> gains;
        GetAMRGains(tmpFrame, gains, maxGain);

        
        TInt gamma10000 = KAmrGain_dB2Gamma[aGain+127];
        
        TInt a = 0;
        TInt old_gain, old_gain1 = 0;
        TInt old_pitch, old_pitch1 = 0;
        TInt new_gain, new_gain1 = 0;
        TInt8 new_index = 0;
        
        
        
        switch (dec_mode){
        case 0:
            
            for (a = 0; a < gains.Count()-1 ; a+=4) 
            {
                
                old_gain = gains[a+1];
                new_gain = static_cast<TInt>((gamma10000*old_gain)/10000);
                old_pitch = gains[a];
                
                old_gain1 = gains[a+3];
                new_gain1 = static_cast<TInt>((gamma10000*old_gain1)/10000);
                old_pitch1 = gains[a+2];
                
                
                new_index = ProcTools::FindNewIndex475VQ(new_gain, 
                    old_pitch, 
                    new_gain1, 
                    old_pitch1);
                
                if (a == 0) 
                {
                    ProcTools::SetValueToShuffledFrame(tmpFrame, 
                        new_index, 
                        KAmrGains475_1_2, 
                        8);
                }
                else if(a == 4) 
                {
                    ProcTools::SetValueToShuffledFrame(tmpFrame, 
                        new_index, 
                        KAmrGains475_3_4, 
                        8);
                }
            }
            
            break;
        case 1:
            
            for (a = 0; a < gains.Count()-1 ; a+=2) 
            {
                
                old_gain = gains[a+1];
                new_gain = static_cast<TInt>((gamma10000*old_gain)/10000);
                old_pitch = gains[a];
                
                new_index = ProcTools::FindNewIndexVQ(new_gain, old_pitch, KAmrGainTable590, 128);
                if (a == 0) ProcTools::SetValueToShuffledFrame(tmpFrame, new_index, KAmrGains515_1, 6);
                else if (a == 2) ProcTools::SetValueToShuffledFrame(tmpFrame, new_index, KAmrGains515_2, 6);
                else if (a == 4) ProcTools::SetValueToShuffledFrame(tmpFrame, new_index, KAmrGains515_3, 6);
                else if (a == 6) ProcTools::SetValueToShuffledFrame(tmpFrame, new_index, KAmrGains515_4, 6);
            }
            
            break;
        case 2:

            for (a = 0; a < gains.Count()-1 ; a+=2) 
            {
                
                old_gain = gains[a+1];
                new_gain = static_cast<TInt>((gamma10000*old_gain)/10000);
                old_pitch = gains[a];
                
                new_index = ProcTools::FindNewIndexVQ(new_gain, old_pitch, KAmrGainTable590, 128);
                if (a == 0) ProcTools::SetValueToShuffledFrame(tmpFrame, new_index, KAmrGains590_1, 6);
                else if (a == 2) ProcTools::SetValueToShuffledFrame(tmpFrame, new_index, KAmrGains590_2, 6);
                else if (a == 4) ProcTools::SetValueToShuffledFrame(tmpFrame, new_index, KAmrGains590_3, 6);
                else if (a == 6) ProcTools::SetValueToShuffledFrame(tmpFrame, new_index, KAmrGains590_4, 6);
            }
            
            break;
        case 3:

            for (a = 0; a < gains.Count()-1 ; a+=2) 
            {
                
        
                old_gain = gains[a+1];
                new_gain = static_cast<TInt>((gamma10000*old_gain)/10000);
                old_pitch = gains[a];
                
                new_index = ProcTools::FindNewIndexVQ(new_gain, old_pitch, KAmrGainTable102, 256);
                if (a == 0) ProcTools::SetValueToShuffledFrame(tmpFrame, new_index, KAmrGains670_1, 7);
                else if (a == 2) ProcTools::SetValueToShuffledFrame(tmpFrame, new_index, KAmrGains670_2, 7);
                else if (a == 4) ProcTools::SetValueToShuffledFrame(tmpFrame, new_index, KAmrGains670_3, 7);
                else if (a == 6) ProcTools::SetValueToShuffledFrame(tmpFrame, new_index, KAmrGains670_4, 7);
            }
            
            break;
        case 4:
    
            for (a = 0; a < gains.Count()-1 ; a+=2) 
            {
                
                old_gain = gains[a+1];
                new_gain = static_cast<TInt>((gamma10000*old_gain)/10000);
                old_pitch = gains[a];
                
                new_index = ProcTools::FindNewIndexVQ(new_gain, old_pitch, KAmrGainTable102, 256);
                if (a == 0) ProcTools::SetValueToShuffledFrame(tmpFrame, new_index, KAmrGains740_1, 7);
                else if (a == 2) ProcTools::SetValueToShuffledFrame(tmpFrame, new_index, KAmrGains740_2, 7);
                else if (a == 4) ProcTools::SetValueToShuffledFrame(tmpFrame, new_index, KAmrGains740_3, 7);
                else if (a == 6) ProcTools::SetValueToShuffledFrame(tmpFrame, new_index, KAmrGains740_4, 7);
            }
            
            break;
        case 5:
        
            for (a = 0; a < gains.Count() ; a++) 
            {
                
                old_gain = gains[a];
                new_gain = static_cast<TInt>((gamma10000*old_gain)/10000);
                
                new_index = ProcTools::FindNewIndexSQ(new_gain, KAmrGainTable122, 32);
                if (a == 0) ProcTools::SetValueToShuffledFrame(tmpFrame, new_index, KAmrGains795_1, 5);
                else if (a == 1) ProcTools::SetValueToShuffledFrame(tmpFrame, new_index, KAmrGains795_2, 5);
                else if (a == 2) ProcTools::SetValueToShuffledFrame(tmpFrame, new_index, KAmrGains795_3, 5);
                else if (a == 3) ProcTools::SetValueToShuffledFrame(tmpFrame, new_index, KAmrGains795_4, 5);
            }
            
            break;
        case 6:
    
            
            for (a = 0; a < gains.Count()-1 ; a+=2) 
            {
                
                old_gain = gains[a+1];
                new_gain = static_cast<TInt>((gamma10000*old_gain)/10000);
                old_pitch = gains[a];
                
                new_index = ProcTools::FindNewIndexVQ(new_gain, old_pitch, KAmrGainTable102, 256);
                if (a == 0) ProcTools::SetValueToShuffledFrame(tmpFrame, new_index, KAmrGains102_1, 7);
                else if (a == 2) ProcTools::SetValueToShuffledFrame(tmpFrame, new_index, KAmrGains102_2, 7);
                else if (a == 4) ProcTools::SetValueToShuffledFrame(tmpFrame, new_index, KAmrGains102_3, 7);
                else if (a == 6) ProcTools::SetValueToShuffledFrame(tmpFrame, new_index, KAmrGains102_4, 7);
            }
            
            break;
            
        case 7:
        
                        
            for (a = 0; a < gains.Count() ; a++) 
            {
                
                old_gain = gains[a];
                new_gain = static_cast<TInt>((gamma10000*old_gain)/10000);
                
                new_index = ProcTools::FindNewIndexSQ(new_gain, KAmrGainTable122, 32);
                if (a == 0) ProcTools::SetValueToShuffledFrame(tmpFrame, new_index, KAmrGains122_1, 5);
                else if (a == 1) ProcTools::SetValueToShuffledFrame(tmpFrame, new_index, KAmrGains122_2, 5);
                else if (a == 2) ProcTools::SetValueToShuffledFrame(tmpFrame, new_index, KAmrGains122_3, 5);
                else if (a == 3) ProcTools::SetValueToShuffledFrame(tmpFrame, new_index, KAmrGains122_4, 5);
            }
            
            break;
        case 8:
        
            break;
        case 15:
        
            break;
        default:
            
            break;
            };
    
    
        gains.Reset();
        aFrameOut->Des().Append(tmpFrame->Des());
        CleanupStack::PopAndDestroy(tmpFrame);

        }
    CleanupStack::Pop(); // aFrameOut
    
    return ETrue;
    }

TBool CProcAMRFrameHandler::GetGainL(const HBufC8* aFrame, RArray<TInt>& aGains, TInt& aMaxGain) const
    {
    
    TBool vectorQuant = EFalse;
    
	const TUint8 ch = (*aFrame)[0];

	TUint dec_mode = (enum Mode)((ch & 0x0078) >> 3);
	
	TBool mode475 = EFalse;

	switch (dec_mode)
		{
		case 0:

			mode475 = ETrue;
			vectorQuant = ETrue;
			break;
		
		case 1:
		case 2:
		case 3:
		case 4:
		case 6:
			vectorQuant = ETrue;
			break;
		
		default:
			break;
		};


	RArray<TInt> amrGains;
	RArray<TInt> gpGains;

	TInt maxGain = 0;
	
	TReal b[4] = {0.68, 0.58, 0.34, 0.19};


	if (aFrame->Size() < 10)

		{
		// SID or no data
		for (TInt r = 0; r < 4 ; r++)
			{
			iPreviousRn[r] = 0;
			
			iPreviousEnergy = 0;
			
			
			}

		aGains.Append(0);
		aMaxGain = 1;
        return ETrue;
		}

	GetAMRGains(aFrame, amrGains, maxGain);
	GetGPGains(aFrame, gpGains);

	TInt gains = 4;
	if (mode475)
		{
		gains = 2;
		}

	for (TInt subFrame = 0 ; subFrame < gains ; subFrame++)
		{

		// calculate gc (fixed codebook gain) ------------------------->

		TReal currentGamma = 0;
		if (vectorQuant)
			{
			currentGamma = (TReal)(amrGains[subFrame*2+1])/4096.0; // Q12, 2^12 = 4096
			}
		else
			{
			currentGamma = (TReal)(amrGains[subFrame])/4096.0; 
			}

		TReal E = iPreviousRn[3]*b[0]+iPreviousRn[2]*b[1]+iPreviousRn[1]*b[2]+iPreviousRn[0]*b[3];
		
		TReal logGamma = 0;
		Math::Log(logGamma, currentGamma);
		TReal R = 20 * logGamma;

		TReal gcPred = 0; //g'c
		Math::Pow10(gcPred, 0.05*E);
		TReal gc = gcPred*currentGamma;


		// update previous R(n) values --------->
		for (TInt re = 0; re < 3 ; re++)
			{
			iPreviousRn[re] = iPreviousRn[re+1];
			}
		iPreviousRn[3] = R;
		// <---------- update ends

		// <---------- gc calculated

		// calculate energy from adaptive codebook ------------->
		
		TReal currentGp = 0;
		
		if (vectorQuant)
			{
			currentGp = (TReal)(amrGains[subFrame*2])/16384.0; // Q14 = 2^14 = 16384
			}
		else
			{
			currentGp = (TReal)(gpGains[subFrame])/16384.0; // Q14, 2^13 = 8192
			}


		TReal energyFromAdaptiveCB = (iPreviousEnergy*currentGp)/2;

		// <----------------------- energy from adaptive codebook calculated

		TReal totalEnergy = energyFromAdaptiveCB+(gc*45);

		iPreviousEnergy = totalEnergy;

		TReal logTotalEnergy = 0;
		
		if (totalEnergy < 1) totalEnergy = 1;
		
		Math::Log(logTotalEnergy, totalEnergy);
		
		if (logTotalEnergy > 3.5) 
		    {
		    logTotalEnergy = 3.5;
		    }
		
		aGains.Append(logTotalEnergy*100);

        const TInt KMaxGain = 350;

        aMaxGain = KMaxGain;
        
        
		}
		
		
	amrGains.Reset();	
	gpGains.Reset();
	return ETrue;
    }


TBool CProcAMRFrameHandler::GetNormalizingMargin(const HBufC8* aFrame, TInt8& aMargin) const

    {
        
    const TUint8 ch = (*aFrame)[0];

    TUint dec_mode = (enum Mode)((ch & 0x0078) >> 3);
    RArray<TInt> gains;
    
    TInt error = KErrNone;
    TRAP(error, CleanupClosePushL(gains) );
    
    if (error != KErrNone)
        return EFalse;
    
    TInt gamma10000 = 1;
    TInt largestGain = 1;
    TInt a = 0;
    TInt maxGain;

        switch (dec_mode)
        {
        case 0:
        
            GetAMRGains(aFrame, gains, maxGain);

            for (a = 1 ; a < gains.Count() ; a+=2)
                {
                if (gains[a] > largestGain) 
                    {
                    largestGain = gains[a];
                    }

                }

            gamma10000 = (KAmrLargestGain475*10000)/largestGain;

            break;
        case 1:
        case 2:

            // bitrates 515, 590
            GetAMRGains(aFrame, gains, maxGain);
            
            for (a = 1 ; a < gains.Count() ; a+=2)
                {
                if (gains[a] > largestGain) 
                    {
                    largestGain = gains[a];
                    }

                }
            gamma10000 = (KAmrLargestGain590*10000)/largestGain;


            break;
        case 3:
        case 4:
        case 6:
            // bitRate = 670, 740, 1020;
            GetAMRGains(aFrame, gains, maxGain);
            
            for (a = 1 ; a < gains.Count() ; a+=2)
                {
                if (gains[a] > largestGain) 
                    {
                    largestGain = gains[a];
                    }

                }
            gamma10000 = (KAmrLargestGain102*10000)/largestGain;

            break;
        case 5:
        case 7:
            //bitRate = 795 & 12.2
            GetAMRGains(aFrame, gains, maxGain);
        
            
            for (a = 0 ; a < gains.Count() ; a++)
                {
                if (gains[a] > largestGain) 
                    {
                    largestGain = gains[a];
                    }

                }
            gamma10000 = (KAmrLargestGain122*10000)/largestGain;


            break;
        
        case 8:
            
        case 15:
            
        default:
            aMargin = 100;
            return ETrue;
            
        };


    CleanupStack::PopAndDestroy(&gains);

    TUint8 newIndex = ProcTools::FindNewIndexSQ(gamma10000, KAmrGain_dB2Gamma, 256);

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

CProcAMRFrameHandler::~CProcAMRFrameHandler() 
    {

    }

CProcAMRFrameHandler* CProcAMRFrameHandler::NewL() 
    {

    
    CProcAMRFrameHandler* self = NewLC();
    CleanupStack::Pop(self);
    return self;

    }
CProcAMRFrameHandler* CProcAMRFrameHandler::NewLC() 
    {

    CProcAMRFrameHandler* self = new (ELeave) CProcAMRFrameHandler();
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;

    }

void CProcAMRFrameHandler::ConstructL() 
    {


    }

CProcAMRFrameHandler::CProcAMRFrameHandler()
    {

    }
 
TBool CProcAMRFrameHandler::GetAMRGains(const HBufC8* aFrame, 
                                        RArray<TInt>& aGains, 
                                        TInt& aMaxGain) const

    {

    
    const TUint8 ch = (*aFrame)[0];

    TUint dec_mode = (enum Mode)((ch & 0x0078) >> 3);
    TInt gainIndex = 0;
    

        switch (dec_mode)
        {
        case 0:
//            bitRate = 475;
            
            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains475_1_2, 8);
            aGains.Append(KAmrGainTable475[gainIndex*4]);
            aGains.Append(KAmrGainTable475[gainIndex*4+1]);
            aGains.Append(KAmrGainTable475[gainIndex*4+2]);
            aGains.Append(KAmrGainTable475[gainIndex*4+3]);

            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains475_3_4, 8);
            aGains.Append(KAmrGainTable475[gainIndex*4]);
            aGains.Append(KAmrGainTable475[gainIndex*4+1]);
            aGains.Append(KAmrGainTable475[gainIndex*4+2]);
            aGains.Append(KAmrGainTable475[gainIndex*4+3]);

            break;
        case 1:
//            bitRate = 515;

            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains515_1, 6);
            aGains.Append(KAmrGainTable590[gainIndex*2]);
            aGains.Append(KAmrGainTable590[gainIndex*2+1]);

            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains515_2, 6);
            aGains.Append(KAmrGainTable590[gainIndex*2]);
            aGains.Append(KAmrGainTable590[gainIndex*2+1]);

            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains515_3, 6);
            aGains.Append(KAmrGainTable590[gainIndex*2]);
            aGains.Append(KAmrGainTable590[gainIndex*2+1]);

            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains515_4, 6);
            aGains.Append(KAmrGainTable590[gainIndex*2]);
            aGains.Append(KAmrGainTable590[gainIndex*2+1]);
            break;
        case 2:
//            bitRate = 590;

            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains590_1, 6);
            aGains.Append(KAmrGainTable590[gainIndex*2]);
            aGains.Append(KAmrGainTable590[gainIndex*2+1]);

            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains590_2, 6);
            aGains.Append(KAmrGainTable590[gainIndex*2]);
            aGains.Append(KAmrGainTable590[gainIndex*2+1]);

            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains590_3, 6);
            aGains.Append(KAmrGainTable590[gainIndex*2]);
            aGains.Append(KAmrGainTable590[gainIndex*2+1]);

            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains590_4, 6);
            aGains.Append(KAmrGainTable590[gainIndex*2]);
            aGains.Append(KAmrGainTable590[gainIndex*2+1]);

            break;
        case 3:
//            bitRate = 670;

            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains670_1, 7);
            aGains.Append(KAmrGainTable102[gainIndex*2]);
            aGains.Append(KAmrGainTable102[gainIndex*2+1]);

            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains670_2, 7);
            aGains.Append(KAmrGainTable102[gainIndex*2]);
            aGains.Append(KAmrGainTable102[gainIndex*2+1]);

            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains670_3, 7);
            aGains.Append(KAmrGainTable102[gainIndex*2]);
            aGains.Append(KAmrGainTable102[gainIndex*2+1]);

            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains670_4, 7);
            aGains.Append(KAmrGainTable102[gainIndex*2]);
            aGains.Append(KAmrGainTable102[gainIndex*2+1]);

            break;
        case 4:
//            bitRate = 740;

            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains740_1, 7);
            aGains.Append(KAmrGainTable102[gainIndex*2]);
            aGains.Append(KAmrGainTable102[gainIndex*2+1]);

            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains740_2, 7);
            aGains.Append(KAmrGainTable102[gainIndex*2]);
            aGains.Append(KAmrGainTable102[gainIndex*2+1]);

            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains740_3, 7);
            aGains.Append(KAmrGainTable102[gainIndex*2]);
            aGains.Append(KAmrGainTable102[gainIndex*2+1]);

            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains740_4, 7);
            aGains.Append(KAmrGainTable102[gainIndex*2]);
            aGains.Append(KAmrGainTable102[gainIndex*2+1]);

            break;
        case 5:
//            bitRate = 795;
            
            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains795_1, 5);
            aGains.Append(KAmrGainTable122[gainIndex]);

            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains795_2, 5);
            aGains.Append(KAmrGainTable122[gainIndex]);

            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains795_3, 5);
            aGains.Append(KAmrGainTable122[gainIndex]);

            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains795_4, 5);
            aGains.Append(KAmrGainTable122[gainIndex]);
                
            break;
        case 6:
//            bitRate = 1020;

            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains102_1, 7);
            aGains.Append(KAmrGainTable102[gainIndex*2]);
            aGains.Append(KAmrGainTable102[gainIndex*2+1]);

            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains102_2, 7);
            aGains.Append(KAmrGainTable102[gainIndex*2]);
            aGains.Append(KAmrGainTable102[gainIndex*2+1]);

            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains102_3, 7);
            aGains.Append(KAmrGainTable102[gainIndex*2]);
            aGains.Append(KAmrGainTable102[gainIndex*2+1]);

            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains102_4, 7);
            aGains.Append(KAmrGainTable102[gainIndex*2]);
            aGains.Append(KAmrGainTable102[gainIndex*2+1]);

            break;
        case 7:
//            bitRate = 1220;
                
            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains122_1, 5);
            aGains.Append(KAmrGainTable122[gainIndex]);

            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains122_2, 5);
            aGains.Append(KAmrGainTable122[gainIndex]);

            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains122_3, 5);
            aGains.Append(KAmrGainTable122[gainIndex]);

            gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGains122_4, 5);
            aGains.Append(KAmrGainTable122[gainIndex]);
                
                
            break;
        case 8:
//            bitRate = 0;
            break;
        case 15:
//            bitRate = 0;
            break;
        default:
//            bitRate = 0;
            break;
        };


    aMaxGain = KAmrLargestGain102;

    return ETrue;


    }

TInt CProcAMRFrameHandler::GetNextFrameLength(const HBufC8* aFrame, TInt aPosNow)
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
            return 0;
            }
            
        default:
            return 0+1;
            
        };
    

    }
    

TBool CProcAMRFrameHandler::GetGPGains(const HBufC8* aFrame, 
										RArray<TInt>& aGains) const

	{

	
	const TUint8 ch = (*aFrame)[0];

	TUint dec_mode = (enum Mode)((ch & 0x0078) >> 3);
	TInt gainIndex = 0;
	

		switch (dec_mode)
		{
		
		case 7:
//			bitRate = 1220;
				
			gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGPGains122_1, 4);
			aGains.Append(KAmrGPTable[gainIndex]);

			gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGPGains122_2, 4);
			aGains.Append(KAmrGPTable[gainIndex]);

			gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGPGains122_3, 4);
			aGains.Append(KAmrGPTable[gainIndex]);

			gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGPGains122_4, 4);
			aGains.Append(KAmrGPTable[gainIndex]);
				
				
			break;

		case 5:
		// 7.95
			gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGPGains795_1, 4);
			aGains.Append(KAmrGPTable[gainIndex]);

			gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGPGains795_2, 4);
			aGains.Append(KAmrGPTable[gainIndex]);

			gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGPGains795_3, 4);
			aGains.Append(KAmrGPTable[gainIndex]);

			gainIndex = ProcTools::GetValueFromShuffledFrame(aFrame, KAmrGPGains795_4, 4);
			aGains.Append(KAmrGPTable[gainIndex]);
			break;



		default:
//			return EFalse;
			break;
		};


	return ETrue;


	}
