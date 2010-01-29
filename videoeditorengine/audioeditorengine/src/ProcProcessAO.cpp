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



#include "ProcProcessAO.h"
#include "AudCommon.h"
#include "AudPanic.h"
#include "audconstants.h"




TBool CProcProcess::StartSyncProcessingL(const CAudSong* aSong, TBool aGetTimeEstimation)
    {

    iSong = aSong;
    
    if (iEncoder != 0)
        {
        delete iEncoder;
        iEncoder = 0;
        }
    iEncoder = CProcEncoder::NewL();
    iEncoder->InitL(iSong->OutputFileProperties().iAudioType, 
                    iSong->OutputFileProperties().iSamplingRate,
                    iSong->OutputFileProperties().iChannelMode, 
                    iSong->OutputFileProperties().iBitrate );
                    
    
    if (iAMRBuf)
        {
        delete iAMRBuf;
        iAMRBuf = 0;
        
        }
    // buffer for extra AMR-frames
    iAMRBuf = HBufC8::NewL(KAedMaxAMRFrameLength);
    
    if (iDecBuffer)
        {
        delete iDecBuffer;   
        iDecBuffer = 0;
        
        }
        
    iDecBuffer = HBufC8::NewL(KAedMaxFeedBufferSize);
    
    TInt rawFrameSize = KAedSizeAMRBuffer;
    if (aSong->OutputFileProperties().iSamplingRate == KAedSampleRate8kHz)
        {
        rawFrameSize = KAedSizeAMRBuffer;
        }
    else 
        {
        rawFrameSize = KAedSizeAACBuffer;
        if ( aSong->OutputFileProperties().iChannelMode == EAudStereo )
            {
            rawFrameSize *= 2;
            }
        }
    

    CAudProcessorImpl* processorImpl = CAudProcessorImpl::NewLC();

    processorImpl->ProcessSongL(aSong, rawFrameSize, aGetTimeEstimation);

    CleanupStack::Pop(processorImpl);
    iProcessorImpl = processorImpl;


    return ETrue;

    }

TBool CProcProcess::ProcessSyncPieceL(HBufC8*& aFrame, TInt& aProgress,
                                       TTimeIntervalMicroSeconds& aDuration)
    {
    
    // first check if we have frames left from the previous encoding
    
    if (iAMRBuf->Length() > 0)
        {
        // we do have some frames left
        
        TInt frLen = ProcTools::GetNextAMRFrameLength(iAMRBuf, 0);
        
        if (frLen > iAMRBuf->Length())
            {
            // something has gone wrong, we don't have a proper AMR frame in the buffer
            User::Leave(KErrGeneral);
            }
        
        aFrame = HBufC8::NewL(frLen);
        aFrame->Des().Append(iAMRBuf->Left(frLen));
        iAMRBuf->Des().Delete(0, frLen);
        
        aDuration = KAedAMRFrameDuration;
        iProgress = aProgress;
        
        return EFalse;
        }
    
    iDecBuffer->Des().Delete(0, iDecBuffer->Size());
    
    HBufC8* frame = 0;
        
    TBool rawFrame = EFalse;
    
    TInt outDurationMilli = 0;
    
    TBool ret = iProcessorImpl->ProcessSyncPieceL(frame, aProgress, aDuration, rawFrame);
    iProgress = aProgress;
    
    if (!rawFrame)
        {
        
        // frame already in compressed domain, no need for encoding
        if (ret || frame == 0) 
            {
            
            iTimeEstimate = iProcessorImpl->GetFinalTimeEstimate();
            
            aFrame = frame;
            // no more frames left -> processing ready
            delete iProcessorImpl;
            iProcessorImpl = 0;
            delete iDecBuffer;
            iDecBuffer = 0;
            return ETrue;
            }
        else 
            {
            aFrame = frame;
            return EFalse;
            }
    
        }
    else
        {
        // frame needs to be encoded
        if (ret || frame == 0) 
            {
            
            iTimeEstimate = iProcessorImpl->GetFinalTimeEstimate();
            // no more frames left -> processing ready
            delete iProcessorImpl;
            iProcessorImpl = 0;
            delete iDecBuffer;
            iDecBuffer = 0;
            return ETrue;
            }    
        
        
        // feed encoder until we have something in the output buffer

        
        iEncoder->FillEncBufferL(*frame, iDecBuffer, outDurationMilli);
 
        delete frame;
        frame = 0;
   
        while (iDecBuffer->Size() == 0)
            {
            
            // we need more input for encoder
            
            TTimeIntervalMicroSeconds dur;
                       ret = iProcessorImpl->ProcessSyncPieceL(frame, aProgress, dur, rawFrame);
                
            if (!ret)
                {
                
                }
            else
                {
                
                // no more frames left -> processing ready
                
                
                iTimeEstimate = iProcessorImpl->GetFinalTimeEstimate();
                delete iProcessorImpl;
                iProcessorImpl = 0;
                delete iDecBuffer;
                iDecBuffer = 0;
                return ETrue;
                }
                
  
            
            iProgress = aProgress;
            
            if (ret) 
                {
                
                // no more frames left -> processing ready
                
                iTimeEstimate = iProcessorImpl->GetFinalTimeEstimate();
                delete iProcessorImpl;
                iProcessorImpl = 0;
                delete iDecBuffer;
                iDecBuffer = 0;
                return ETrue;
                }
            
            if (!rawFrame)
                {
          
                // we got encoded frame -> return it
                aFrame = frame;
                return EFalse;
                }
            
            // we should now have a raw frame in aFrame -> feed the encoder
            
            aDuration = aDuration.Int64() + dur.Int64();

            iEncoder->FillEncBufferL(*frame, iDecBuffer, outDurationMilli);
            delete frame;
            frame = 0;
          
            }
        
        }
        
    // read the encoded frame to aFrame
    
    TInt frameLen = 0;
    if (iEncoder->DestAudType() == EAudAMR)
        {
        frameLen = ProcTools::GetNextAMRFrameLength(iDecBuffer, 0);
        }
    
    if (frameLen < iDecBuffer->Length() && iEncoder->DestAudType() == EAudAMR)
        {
        
        // we got more than one AMR frame, return the first now
        // and the second when this function is called next time
        aFrame = HBufC8::NewL(frameLen);
        aFrame->Des().Append(iDecBuffer->Des().Left(frameLen));
        
        if ( (iDecBuffer->Length() + iAMRBuf->Length() - frameLen) > iAMRBuf->Des().MaxLength())
            {
            // if the temporary storage is too small, we need more space for it
            iAMRBuf = iAMRBuf->ReAlloc(iDecBuffer->Length() + iAMRBuf->Length() - frameLen);
            }
            
        iAMRBuf->Des().Append(iDecBuffer->Right(iDecBuffer->Length() - frameLen));
        
        }
    else
        {
        
        aFrame = HBufC8::NewL(iDecBuffer->Size());
        aFrame->Des().Append(iDecBuffer->Des());
        
        }
        
    iDecBuffer->Des().Delete(0, iDecBuffer->Size());
           
    aDuration = outDurationMilli*1000;
    
    return EFalse;
    
    }

TInt64 CProcProcess::GetFinalTimeEstimate() const
    {
    
    return iTimeEstimate;
    
    }


CProcProcess* CProcProcess::NewL() 
    {


    CProcProcess* self = new (ELeave) CProcProcess();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }

CProcProcess::~CProcProcess() 
    {

    if (iProcessorImpl != 0)
        {
        delete iProcessorImpl;
        iProcessorImpl = 0;
        }
        
    delete iEncoder;
    
    delete iDecBuffer;
    iDecBuffer = 0;
    
    delete iAMRBuf;
    
    
    }
    

void CProcProcess::ConstructL() 
    {
    
    
    }

CProcProcess::CProcProcess() : iProcessorImpl(0), iSong(0)
    {

    }
    
    
