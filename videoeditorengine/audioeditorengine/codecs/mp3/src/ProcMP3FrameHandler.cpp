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
#include "ProcMP3FrameHandler.h"
#include "ProcMP3InFileHandler.h"
#include "ProcFrameHandler.h"

#include "MP3API.h"


//mixing files



CProcMP3FrameHandler* 
CProcMP3FrameHandler::NewL() 
{    
  CProcMP3FrameHandler* self = NewLC();
  CleanupStack::Pop(self);
  return self;
}

CProcMP3FrameHandler* 
CProcMP3FrameHandler::NewLC() 
{
  CProcMP3FrameHandler* self = new (ELeave) CProcMP3FrameHandler();
  CleanupStack::PushL(self);
  self->ConstructL();
  return self;
}

CProcMP3FrameHandler::~CProcMP3FrameHandler() 
{

    if (iMp3Edit != 0) delete iMp3Edit;

    if (iMpAud != 0)
        {
        
        delete iMpAud;
        }
    if (iTHandle != 0)
        delete iTHandle;

    
}

CProcMP3FrameHandler::CProcMP3FrameHandler() : decInitialized(EFalse)
{
    
}

void CProcMP3FrameHandler::
ConstructL() 
{

    iMp3Edit = CMp3Edit::NewL();
    
    iTHandle = new (ELeave) TMpTransportHandle();

    iMp3Edit->InitTransport(iTHandle);

    iMpAud = CMPAudDec::NewL();
    iMpAud->Init(iTHandle);
    

}

TBool CProcMP3FrameHandler::
ManipulateGainL(const HBufC8* aFrameIn, HBufC8*& aFrameOut, TInt8 aGain) 
{

    aFrameOut = HBufC8::NewLC(aFrameIn->Size());

    aFrameOut->Des().Copy(aFrameIn->Ptr(), aFrameIn->Size());


    RArray<TInt> gains;
    CleanupClosePushL(gains);
    TInt aMaxGain = 0;
    TBitStream bs;
    GetMP3Gains(aFrameOut, gains, aMaxGain, bs);
    TUint8* globalGains = new (ELeave) TUint8[gains.Count()/2];
    CleanupStack::PushL(globalGains);
    TUint* globalGainPos = new (ELeave) TUint[gains.Count()/2];
    CleanupStack::PushL(globalGainPos);


    //for (TInt a = 0 ; a < gains.Count(); a++)
    for (TInt a = 0 ; a < gains.Count()/2; a++)
        {

        
        TInt newGain = aGain*500;
        newGain = newGain/1500;

        if (gains[a]+newGain < 0) globalGains[a] = 0;
        else if(gains[a]+newGain > 255) globalGains[a] = 255;
        else globalGains[a] = static_cast<TUint8>(gains[a*2]+newGain);
        globalGainPos[a] = gains[a*2+1];
        }
    
//SetMPGlobalGains(BitStream *bs, uint8 numGains, uint8 *globalGain, uint32 *gainPos);


    iMp3Edit->SetMPGlobalGains(&bs, static_cast<TUint8>(gains.Count()/2), globalGains, globalGainPos);
    

    //delete[] globalGains;
    //delete[] globalGainPos;
    CleanupStack::PopAndDestroy(globalGainPos);
    CleanupStack::PopAndDestroy(globalGains);
    CleanupStack::PopAndDestroy(&gains);
    CleanupStack::Pop(); // aFrameOut
    return ETrue;
    
}

TBool CProcMP3FrameHandler::
GetGainL(const HBufC8* aFrame, RArray<TInt>& aGains, TInt& aMaxGain) const
    {
    RArray<TInt> gains;
    TBitStream bs;
    GetMP3Gains(aFrame, gains, aMaxGain, bs);
    

    for (TInt a = 0 ; a < gains.Count(); a++)
        {
        TInt ga = gains[a]-120;

        if (ga < 0) ga = 0;

        ga = ga*ga;
        ga = ga/50;

        aGains.Append(ga);
        a++;
        }

    gains.Reset();
    aMaxGain = 120;
    return ETrue;
    }

TBool CProcMP3FrameHandler::
GetMP3Gains(const HBufC8* aFrame, RArray<TInt>& aGains, TInt& aMaxGain, TBitStream& aBs) const
    {

    TInt16 readBytes, frameBytes, headerBytes = 0;

    if (!decInitialized)
        {
        iMp3Edit->SeekSync(iTHandle, const_cast<unsigned char*>(aFrame->Ptr()), 
        aFrame->Length(), &readBytes, 
        &frameBytes, &headerBytes, (uint8) 1);
        iMpAud->Init(iTHandle);
        decInitialized = ETrue;
        }


    int16 i;
    uint32 globalGainPos[4];
    uint8 globalGain[4], nBufs;
    TBitStream bsTmp;

    const TInt headerLength = 4;

    TUint8* buf = const_cast<TUint8*>(aFrame->Right(aFrame->Size()-headerLength).Ptr());
    BsInit(&aBs, buf, aFrame->Size() - headerLength);
    BsSaveBufState(&aBs, &bsTmp);

    //BsSaveBufState(&bsIn, &bsTmp);

    nBufs = iMp3Edit->GetMPGlobalGains(&bsTmp, iTHandle, globalGain, globalGainPos);

    for(i = 0; i < nBufs; i++)
        {
        TInt gg = globalGain[i];
        TInt ggp = globalGainPos[i];

        aGains.Append(gg);
        aGains.Append(ggp);
        }
    aMaxGain = 255;

    return (EFalse);
    
    }

TBool CProcMP3FrameHandler::
GetNormalizingMargin(const HBufC8* /*aFrame*/, TInt8& /*aMargin*/) const
{
  return (EFalse);
}

TBool CProcMP3FrameHandler::IsMixingAvailable() const
{
    return EFalse;
}

