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
* Implementation for video processor.
*
*/


//  Include Files

#include "vedcommon.h"
#include "movieprocessorimpl.h"
#include "statusmonitor.h"
#include "activequeue.h"
#include "dataprocessor.h"
#include "h263dmai.h"  // CVedH263Dec
#include "mp4parser.h"
#include "videoencoder.h"
#include "videoprocessor.h"
#include "mpeg4timer.h"
#include "vedvolreader.h"
#include "vedvideosettings.h"
#include "vedavcedit.h"

// Local constants
const TUint KInitialDataBufferSize = 8192; // initial frame data buffer size
const TUint KH263StartCodeLength = 3;  // H.263 picture start code length
const TUint KMPEG4StartCodeLength = 4; // MPEG4 picture start code length
//const TUint KMaxEncodingDelay = 500000; // time to wait for encoding to complete in microsec.
const TUint KDefaultTimeIncrementResolution = 30000;
const TUint KAVCNotCodedFrameBuffer = 128;
const TUint KMaxItemsInProcessingQueue = 3;     // note! this must be synchronized with KTRMinNumberOfBuffersCodedPicture setting in transcoder!


#ifdef _DEBUG
const TInt KErrorCode = CVideoProcessor::EDecoderFailure;
#else
const TInt KErrorCode = KErrGeneral;
#endif

// An assertion macro wrapper to clean up the code a bit
#define VDASSERT(x, n) __ASSERT_DEBUG(x, User::Panic(_L("CVideoProcessor"), EInternalAssertionFailure+n)) 

// Debug print macro

#ifdef _DEBUG
#include <e32svr.h>
#define PRINT(x) RDebug::Print x;
#else
#define PRINT(x)
#endif

// ================= STATIC FUNCTIONS =======================

// ---------------------------------------------------------
// AddBits
// Static helper function to add bits to byte-buffer
// ---------------------------------------------------------
//
static void AddBits(TUint8* aBuf, TInt& aBitIndex, TInt& aByteIndex, TInt aBits, TInt aNrOfBits)
    {
    // aBitIndex = 8 => first bit in the left
    // aBitIndex = 1 => last bit in the right
    while ( aBitIndex < aNrOfBits )
        {
        // divide into n bytes
        aBuf[aByteIndex++] |= TUint8( aBits >> (aNrOfBits-aBitIndex) );
        aNrOfBits -= aBitIndex;
        aBitIndex = 8;
        }
    // all bits fit into 1 byte
    aBitIndex -= aNrOfBits;
    aBuf[aByteIndex] |= TUint8( aBits << aBitIndex );
    if (aBitIndex == 0)
        {
        aBitIndex = 8;
        aByteIndex++;
        }
    }



// ================= MEMBER FUNCTIONS =======================



// ---------------------------------------------------------
// CVideoProcessor::NewL
// Symbian two-phased constructor.
// ---------------------------------------------------------
//

CVideoProcessor* CVideoProcessor::NewL(CActiveQueue *anInputQueue,
                                       CVideoProcessor::TStreamParameters *aStreamParameters,
                                       CMovieProcessorImpl* aProcessor,
                                       CStatusMonitor *aStatusMonitor,
                                       CVedAVCEdit *aAvcEdit,
                                       TBool aThumbnailMode,
                                       TInt aPriority)
{
    CVideoProcessor *self = new (ELeave) CVideoProcessor(anInputQueue,
                                                         aStreamParameters,
                                                         aProcessor,
                                                         aStatusMonitor,  
                                                         aAvcEdit,                                                 
                                                         aThumbnailMode,
                                                         aPriority);

    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();

    return self;

}

// ---------------------------------------------------------
// CVideoProcessor::CVideoProcessor
// Constructor.
// ---------------------------------------------------------
//
CVideoProcessor::CVideoProcessor(CActiveQueue *anInputQueue,
                                 TStreamParameters *aStreamParameters,
                                 CMovieProcessorImpl* aProcessor,
                                 CStatusMonitor *aStatusMonitor,
                                 CVedAVCEdit *aAvcEdit,
                                 TBool aThumbnailMode, 
                                 TInt aPriority) : CVideoDecoder(aPriority),
                                 iWriteDes(0,0),
                                 iThumbnailMode(aThumbnailMode)
{

	// Remember the objects
	iQueue = anInputQueue;
	iMonitor = aStatusMonitor;
    iProcessor = aProcessor;	
    iAvcEdit = aAvcEdit;

	// Remember the stream parameters
	iVideoWidth = aStreamParameters->iWidth;
	iVideoHeight = aStreamParameters->iHeight;	

    // Color Toning
	iFirstFrameQp = 0;
	
	iTiming = aStreamParameters->iTiming;
	// Reset state
	iReaderSet = EFalse;
	iDecoding = EFalse;
	iStreamEnd = EFalse;	
	iPreviousFrameIncluded = EFalse; 
	iFrameOperation = EDecodeAndWrite;
	
	iTrPrevious = -1;	

    iFirstFrameFlag = ETrue;
    iDecodePending = EFalse;
    
    iTranscoderStarted = EFalse;
    iDecodingSuspended = EFalse;

	iStartTransitionColor = EColorNone;	
	iEndTransitionColor = EColorNone;
	iStartNumberOfTransitionFrames = KNumTransitionFrames;
	iEndNumberOfTransitionFrames = KNumTransitionFrames;
	iNextTransitionNumber = -1;
	iProcessingComplete = EFalse;
	iStreamEndRead = EFalse;
	
	iPreviousTimeStamp = TTimeIntervalMicroSeconds(-1);
	
	iFirstRead = ETrue;
	
	iThumbDecoded = EFalse;		
	
	iMaxItemsInProcessingQueue = KMaxItemsInProcessingQueue;

    iDataFormat = EDataUnknown;

    iLastWrittenFrameNumber = -1;
    
    iInitializing = ETrue;
}


// ---------------------------------------------------------
// CVideoProcessor::~CVideoProcessor()
// Destructor
// ---------------------------------------------------------
//
CVideoProcessor::~CVideoProcessor()
{

	// If we are decoding, stop
	if (iDecoding)
		Stop();
	
	// Remove from being a reader
	if (iReaderSet)
	{
		// Return current block and all 
		// blocks from input queue
		if (iBlock)    
		{
		    if (iQueue)
			    iQueue->ReturnBlock(iBlock);
		}
					
        if (iQueue)						
		    iBlock = iQueue->ReadBlock();
		
		while (iBlock)
		{
		    if (iQueue)
		    {		        
			    iQueue->ReturnBlock(iBlock);
			    iBlock = iQueue->ReadBlock();
		    }
		}
		iBlockPos = 0;
		
		if (iQueue)
		    iQueue->RemoveReader();
	}
	Cancel();
	
	if (iTransCoder)
	{	    
	    if (iTranscoderStarted)
	    {
	        TRAPD(error, iTransCoder->StopL());
		    if (error != KErrNone) { }
	    }
	    delete iTransCoder;
	    iTransCoder = 0;	
	}
	
	iFrameInfoArray.Reset();
	
	// Close the decoder instance if one has been opened
    if (iDecoder)
        delete iDecoder;
    iDecoder = 0;   
    
	// Deallocate buffers
	if (iDataBuffer)
		User::Free(iDataBuffer);
	
	if (iOutVideoFrameBuffer)
		User::Free(iOutVideoFrameBuffer);

    if (iFrameBuffer)
        User::Free(iFrameBuffer);

	if ( iColorTransitionBuffer )
		User::Free( iColorTransitionBuffer );

	if ( iOrigPreviousYUVBuffer )
		User::Free( iOrigPreviousYUVBuffer );
	
	if (iMediaBuffer)
	    delete iMediaBuffer;
	iMediaBuffer = 0;
	
	if (iDecoderSpecificInfo)
	    delete iDecoderSpecificInfo;
    iDecoderSpecificInfo = 0;
    
    if (iOutputVolHeader)
	    delete iOutputVolHeader;
    iOutputVolHeader = 0;
    
    if (iDelayedBuffer)
        delete iDelayedBuffer;
    iDelayedBuffer = 0;
    
    if (iMPEG4Timer)
        delete iMPEG4Timer;
    iMPEG4Timer = 0;
    
    if (iTimer)
        delete iTimer;
    iTimer = 0;
    
    if (iNotCodedFrame)
        delete iNotCodedFrame;
    iNotCodedFrame = 0;
       
}


// ---------------------------------------------------------
// CVideoProcessor::ConstructL()
// Symbian 2nd phase constructor can leave.
// ---------------------------------------------------------
//
void CVideoProcessor::ConstructL()
{
 	// Set as a reader to the input queue
	iQueue->SetReader(this, NULL);
	iReaderSet = ETrue;

    // Add us to active scheduler
    CActiveScheduler::Add(this);

    iMediaBuffer = new (ELeave)CCMRMediaBuffer;
	
	// Allocate buffers
	iDataBuffer = (TUint8*) User::AllocL(KInitialDataBufferSize);
	iBufferLength = KInitialDataBufferSize; 

    if ( iThumbnailMode )
    {           
    	TSize a = iProcessor->GetMovieResolution();
    	TInt length = a.iWidth*a.iHeight;
        
        length += (length>>1);
        iFrameBuffer = (TUint8*)User::AllocL(length);
    }
	
    TSize size(iVideoWidth, iVideoHeight);

    if (!iThumbnailMode)
    {        
        // Open a decoder instance
        iDecoder = CVedH263Dec::NewL(size, 1 /*iReferencePicturesNeeded*/);
    	    	
    	// create timer
    	iTimer = CCallbackTimer::NewL(*this);
    }
    
    // Make us active
    SetActive();
    iStatus = KRequestPending;
}


// ---------------------------------------------------------
// CVideoProcessor::Start
// Starts decoding
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoProcessor::Start()
{
	if ( iDecoding )
		return;

    // Activate the object if we have data
    if ( (!iDecodePending) && (iStatus == KRequestPending) && iQueue->NumDataBlocks() )
    {
        TRequestStatus *status = &iStatus;
        User::RequestComplete(status, KErrNone);
    }
    
    iDecoding = ETrue;
}

// ---------------------------------------------------------
// CVideoProcessor::Stop
// Stops decoding
// (other items were commented in a header).
// ---------------------------------------------------------
//

void CVideoProcessor::Stop()
{
	iDecoding = EFalse;
		
    if (iTimer)
        iTimer->CancelTimer();
    
	if (iTranscoderStarted)
    {                
        TRAPD(error, iTransCoder->StopL());
        if (error != KErrNone) { }
        iTranscoderStarted = EFalse;
    }    	
}

// ---------------------------------------------------------
// CVideoProcessor::RunL
// Standard active object running method			
// (other items were commented in a header).
// ---------------------------------------------------------
//
					
void CVideoProcessor::RunL()
{
	PRINT((_L("CVideoProcessor::RunL() in")))
	
	// Don't decode if we aren't decoding
    if (!iDecoding)
    {
        if (!IsActive())
        {
            SetActive();
            iStatus = KRequestPending;
        }        
    	PRINT((_L("CVideoProcessor::RunL() out from !iDecoding branch")))
        return;		
    }            
    
    if (iTranscoderInitPending)
    {        
        iTranscoderInitPending = EFalse;        
        if (iStatus != KErrNone)
        {
            if (!iThumbnailMode)
            {                
                VDASSERT(iMonitor, 101);
                iMonitor->Error(iStatus.Int());
            } 
            else
            {
                iProcessor->NotifyThumbnailReady(iStatus.Int());
            }
                
            return;
        }
        // at this point we have already read a frame, 
        // so now start processing                
        iTransCoder->StartL();
                
        // stop if a fatal error has occurred in starting 
        // the transcoder (decoding stopped in MtroFatalError)
        if (!iDecoding)
            return;
        
        iTranscoderStarted = ETrue;
        
        if (!iThumbnailMode)
        {
            ProcessFrameL();
            if (iDecodePending)
                return;
        }
        else
        {
            ProcessThumb(ETrue);
            return;
        }        
    }
        
    if (iDecodePending)
    {
        iDecodePending = EFalse;        

        if (iThumbnailMode)
        {
             if (iThumbDecoded)
             {
                 PRINT((_L("CVideoProcessor::RunL() - thumb decoded")))  
                 ProcessThumb(EFalse);
             }
             else
             {
                 PRINT((_L("CVideoProcessor::RunL() - thumb not decoded")))    
                 ReadAndWriteThumbFrame();
             }
             return;
        }
    }
    
    if (iProcessingComplete)
    {       
        PRINT((_L("CVideoProcessor::RunL() iProcessingComplete == ETrue")))
        VDASSERT(iMonitor, 102);
        iMonitor->ClipProcessed();
        return;
    }
    
    if (iFirstColorTransitionFrame)
    {
        Process2ndColorTransitionFrameL();
        return;
    }

    while (!iDecodePending && !iDelayedWrite && !iTranscoderInitPending &&
             ReadFrame() )
    {    
        // process it       
        if ( ProcessFrameL() )
        {                        
            // clip processed up until cut-out time, stop 
            if (iFrameInfoArray.Count())            
            {
                PRINT((_L("CVideoProcessor::RunL() - stream end reached, wait for frames")));
                iStreamEnd = iStreamEndRead = ETrue;
                
                // if there are still frames to be encoded, start timer 
                // since encoder may skip the rest of the frames
                if ( IsNextFrameBeingEncoded() )
                {
                    PRINT((_L("CVideoProcessor::RunL(), set timer")));
                    if ( !iTimer->IsPending() )
                        iTimer->SetTimer( TTimeIntervalMicroSeconds32( iMaxEncodingDelay ) );                                        
                }
                return;   
            }
            
            iTimer->CancelTimer();
            if (iTranscoderStarted)
            {                
                iTransCoder->StopL();
                iTranscoderStarted = EFalse;
            }
            VDASSERT(iMonitor, 103);
            iMonitor->ClipProcessed();
        	PRINT((_L("CVideoProcessor::RunL() out from ProcessFrameL == ETrue")))
            return;
        }        
    }
    
    if ( !iDecodePending && !iDelayedWrite && !iTranscoderInitPending )
    {
    
        // We didn't get a frame
        if (iStreamEnd)
        {
            iStreamEndRead = ETrue;
            PRINT((_L("CVideoProcessor::RunL() - stream end reached")));    
            if (iFrameInfoArray.Count())
            {
                PRINT((_L("CVideoProcessor::RunL() - stream end reached, wait for frames")));    
                // wait until frames have been processed   
                
                // if there are still frames to be encoded, start timer 
                // since encoder may skip the rest of the frames
                if ( IsNextFrameBeingEncoded() )
                {
                    PRINT((_L("CVideoProcessor::RunL(), set timer")));
                    if ( !iTimer->IsPending() )
                        iTimer->SetTimer( TTimeIntervalMicroSeconds32( iMaxEncodingDelay ) );
                }
                return;
            }
            else
            {    
                iTimer->CancelTimer();
                VDASSERT(iMonitor, 104);                
                iMonitor->ClipProcessed();
            }
        }
        else
        {
            if (!IsActive())
            {
                SetActive();
                iStatus = KRequestPending;
            }
        }
    }

	PRINT((_L("CVideoProcessor::RunL() out")))
      
}

// -----------------------------------------------------------------------------
// CVideoProcessor::RunError
// Called by the AO framework when RunL method has leaved
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//	
TInt CVideoProcessor::RunError(TInt aError)
{

   if ((aError == CVedH263Dec::EDecoderNoIntra) || (aError == CVedH263Dec::EDecoderCorrupted))
   {       
       if (!iThumbnailMode)
           iMonitor->Error(KErrCorrupt);
       else
           iProcessor->NotifyThumbnailReady(KErrCorrupt);
   }
   else
   {
       if (!iThumbnailMode)
           iMonitor->Error(aError);
       else
           iProcessor->NotifyThumbnailReady(aError);
   }

    return KErrNone;
}


// ---------------------------------------------------------
// CVideoProcessor::Process2ndColorTransitionFrameL
// Processes the second frame of a color transition double frame
// (other items were commented in a header).
// ---------------------------------------------------------
//
TBool CVideoProcessor::Process2ndColorTransitionFrameL()
{    

    TFrameInformation frameInfo;
    frameInfo.iTranscoderMode = EFullWithIM;
    frameInfo.iFrameNumber = iFrameNumber;
    frameInfo.iEncodeFrame = ETrue;
    frameInfo.iKeyFrame = EFalse;
    frameInfo.iTransitionFrame = ETrue;
    frameInfo.iTransitionPosition = EPositionStartOfClip;
    frameInfo.iTransitionColor = iStartTransitionColor;
    frameInfo.iTransitionFrameNumber = iTransitionFrameNumber;
    frameInfo.iModificationApplied = EFalse;   
    frameInfo.iRepeatFrame = ETrue;
        
    TInt duration;
    // get timestamp
    iProcessor->GetNextFrameDuration(duration, frameInfo.iTimeStamp, iTimeStampIndex, iTimeStampOffset);
    iTimeStampIndex++;        
    
    frameInfo.iTimeStamp += iCutInTimeStamp;
    
    TTimeIntervalMicroSeconds ts = (iProcessor->GetVideoTimeInMsFromTicks(frameInfo.iTimeStamp, EFalse)) * 1000;
    
    if (ts <= iPreviousTimeStamp)
    {            
        // adjust timestamp so that its bigger than ts of previous frame
        TReal frameRate = iProcessor->GetVideoClipFrameRate();
        VDASSERT(frameRate > 0.0, 105);
        TInt64 durationMs =  TInt64( ( 1000.0 / frameRate ) + 0.5 );
        durationMs /= 2;  // add half the duration of one frame
        
        ts = TTimeIntervalMicroSeconds( iPreviousTimeStamp.Int64() + durationMs*1000 );
        
        frameInfo.iTimeStamp = iProcessor->GetVideoTimeInTicksFromMs( ts.Int64()/1000, EFalse );
        
        ts = iProcessor->GetVideoTimeInMsFromTicks(frameInfo.iTimeStamp, EFalse) * 1000;
        
        PRINT((_L("CVideoProcessor::Process2ndColorTransitionFrameL() - adjusted timestamp, prev = %d, new = %d"), 
                   I64INT( iPreviousTimeStamp.Int64() ) / 1000, I64INT( ts.Int64() ) / 1000));    

    }        
    
 
    iFrameInfoArray.Append(frameInfo);
    
    iPreviousTimeStamp = ts;
    
    iFirstColorTransitionFrame = EFalse;   
    
    CCMRMediaBuffer::TBufferType bt = 
        (iDataFormat == EDataH263) ? CCMRMediaBuffer::EVideoH263 : CCMRMediaBuffer::EVideoMPEG4;
    
    if (!iNotCodedFrame)
        GenerateNotCodedFrameL();
    
    PRINT((_L("CVideoProcessor::Process2ndColorTransitionFrameL() - sending not coded")));
    
#ifdef VIDEOEDITORENGINE_AVC_EDITING
    if (iDataFormat == EDataAVC)
    {        
        TPtr8 ptr(iNotCodedFrame->Des());
        
        TInt length = iNotCodedFrame->Length();
        iAvcEdit->ProcessAVCBitStreamL((TDes8&)(ptr), length, 0 /*dummy*/, EFalse );
        ptr.SetLength(length);
        iDataLength = iCurrentFrameLength = length;
    }
#endif

    iMediaBuffer->Set( TPtrC8(iNotCodedFrame->Des().Ptr(), iNotCodedFrame->Length()),                           
                       bt, 
                       iNotCodedFrame->Length(), 
                       EFalse,
                       ts );

    iDecodePending = ETrue;
    if (!IsActive())
    {
        SetActive();
        iStatus = KRequestPending;                                
    }                
                                      
    PRINT((_L("CVideoProcessor::Process2ndColorTransitionFrameL() - WriteCodedBuffer, frame #%d, timestamp %d ms"),
           iFrameNumber, I64INT( ts.Int64() ) / 1000 ));                                                  
    iTransCoder->WriteCodedBufferL(iMediaBuffer);
                
    iFrameNumber++;
    
    return ETrue;
    
}

    
// ---------------------------------------------------------
// CVideoProcessor::GenerateNotCodedFrameL
// Generate bitstream for not coded frame
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoProcessor::GenerateNotCodedFrameL()
{

    TSize resolution = iProcessor->GetVideoClipResolution();

    if (iDataFormat == EDataH263)
    {
        // H.263 QCIF picture header
        TInt headerSize = 7;
        TUint8 notCodedH263[] = { 0x00, 0x00, 0x80, 0x02, 0x0a, 0x0c, 0x3f };
        TUint8 lsbMask[8] = { 255, 254, 252, 248, 240, 224, 192, 128 };
        
        
        if ( resolution == TSize(128,96) )
            notCodedH263[4] = 0x06;  // set source format as sub-QCIF
        
        else if ( resolution == TSize(352, 288) )
            notCodedH263[4] = 0x0e;  // set source format as CIF
            
        else if ( resolution != TSize(176,144) )
            User::Panic(_L("CVideoProcessor"), EInternalAssertionFailure);
        
        TInt numMBs = ( resolution.iWidth / 16 ) * ( resolution.iHeight / 16 );    
        
        // one COD bit for each MB, the last byte of the pic header already contains 6 MB bits
        TInt bitsLeft = numMBs - 6;  
        
        TInt bufSize = headerSize + ( bitsLeft / 8 ) + ( bitsLeft % 8 != 0 );

        VDASSERT(!iNotCodedFrame, 117);
        iNotCodedFrame = (HBufC8*) HBufC8::NewL(bufSize);
        
        TPtr8 buf(iNotCodedFrame->Des());
        buf.Copy(notCodedH263, headerSize);
        
        TInt index = headerSize;
        TUint8* ptr = const_cast<TUint8*>(buf.Ptr());
        
        // set COD bit to 1 for all macroblocks
        while (bitsLeft >= 8)
        {
            ptr[index++] = 0xff;
            bitsLeft -= 8;
        }
        
        if (bitsLeft)
        {        
            TUint8 val = 0;    
            val |= lsbMask[8 - bitsLeft];
            ptr[index] = val;
        }
        buf.SetLength(bufSize);
    } 
    
    else if (iDataFormat == EDataMPEG4)
    {
        VDASSERT(iDataFormat == EDataMPEG4, 115);

        TUint8 vopStartCodeMPEG4[] = { 0x00, 0x00, 0x01, 0xb6 };
        
        TInt headerSize = 4;

        // calculate the number of bits needed for vop_time_increment
        TInt numTiBits;
        for (numTiBits = 1; ((iInputTimeIncrementResolution - 1) >> numTiBits) != 0; numTiBits++)
        {
        }        
        
        VDASSERT(numTiBits <= 16, 116);

        TInt numMBs = ( resolution.iWidth / 16 ) * ( resolution.iHeight / 16 );
        
        // get VOP size
        //   vop_start_code: 32
        //   vop_coding_type + modulo_time_base + marker_bit: 4
        //   no. of bits for vop_time_increment
        //   marker_bit + vop_coded bit: 2
        //   rounding_type: 1
        //   intra_dc_vlc_thr: 3
        //   vop_quant: 5
        //   vop_fcode_forward: 3
        //   not_coded for each MB: numMBs
        TInt bufSizeBits = headerSize * 8 + 4 + numTiBits + 2 + 1 + 3 + 5 + 3 + numMBs;//DP mode not included!
        if ( (iInputStreamMode == EVedVideoBitstreamModeMPEG4DP)
            || (iInputStreamMode == EVedVideoBitstreamModeMPEG4DP_RVLC)
            || (iInputStreamMode == EVedVideoBitstreamModeMPEG4Resyn_DP)
            || (iInputStreamMode == EVedVideoBitstreamModeMPEG4Resyn_DP_RVLC)
            )
            {
            // Motion marker in DP mode
            bufSizeBits+=17;
            }
        TInt bufSize = ( bufSizeBits / 8 ) + 1;        // always 1-8 stuffing bits
        
        VDASSERT(!iNotCodedFrame, 118);
        iNotCodedFrame = (HBufC8*) HBufC8::NewL(bufSize);
        
        TPtr8 buf(iNotCodedFrame->Des());
        buf.SetLength(bufSize);
        buf.FillZ();
        buf.SetLength(0);
        buf.Copy(vopStartCodeMPEG4, headerSize);
        
        TUint8* ptr = const_cast<TUint8*>(buf.Ptr());        
        TInt shift = 8;
        TInt index = headerSize;
        AddBits(ptr, shift, index, 1, 2); // vop_coding_type
        AddBits(ptr, shift, index, 0, 1); // modulo_time_base
        AddBits(ptr, shift, index, 1, 1); // marker_bit
        
        // vop_time_increment is left to zero (skip FillZ bits)
        AddBits(ptr, shift, index, 0, numTiBits);
            
        // marker (1 bit; 1) 
        AddBits(ptr, shift, index, 1, 1);
        // vop_coded (1 bit; 1=coded)
        AddBits(ptr, shift, index, 1, 1);
        
        // vop_rounding_type (1 bit) (0)
        AddBits(ptr, shift, index, 0, 1);
        
        // intra_dc_vlc_thr (3 bits) (0 = Intra DC, but don't care)
        AddBits(ptr, shift, index, 0, 3);
        
        // vop_quant (5 bits) (1-31)
        AddBits(ptr, shift, index, 10, 5);
        
        // vop_fcode_forward (3 bits) (1-7, 0 forbidden)
        AddBits(ptr, shift, index, 1, 3);
        
        // Macroblocks
        
        // one COD bit for each MB
        TInt bitsLeft = numMBs;  
        
        // set COD bit to 1 for all macroblocks (== not coded)
        while (bitsLeft >= 8)
        {
            AddBits(ptr, shift, index, 0xff, 8);
            bitsLeft -= 8;
        }
        
        TUint8 lsb[8] = { 0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01 };
        if (bitsLeft)
        {        
            TUint8 val = 0;    
            val = lsb[8 - bitsLeft];
            AddBits(ptr, shift, index, val, bitsLeft);
        }
        // If DP mode is used, should add motion marker here: 1 11110000 00000001
        if ( (iInputStreamMode == EVedVideoBitstreamModeMPEG4DP)
            || (iInputStreamMode == EVedVideoBitstreamModeMPEG4DP_RVLC)
            || (iInputStreamMode == EVedVideoBitstreamModeMPEG4Resyn_DP)
            || (iInputStreamMode == EVedVideoBitstreamModeMPEG4Resyn_DP_RVLC)
            )
            {
            AddBits(ptr, shift, index, 0x01, 1);
            AddBits(ptr, shift, index, 0xf0, 8);
            AddBits(ptr, shift, index, 0x01, 8);
            }
        
        // insert stuffing in last byte
        TUint8 stuffing[8] = { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f };
        ptr[bufSize - 1] |= stuffing[shift-1];
                       
        buf.SetLength(bufSize);
            
    } 
    else
    {        
#ifdef VIDEOEDITORENGINE_AVC_EDITING    

        VDASSERT(iDataFormat == EDataAVC, 115);
        
        VDASSERT(!iNotCodedFrame, 118);
        iNotCodedFrame = (HBufC8*) HBufC8::NewL(KAVCNotCodedFrameBuffer);
        
        TPtr8 buf( const_cast<TUint8*>(iNotCodedFrame->Des().Ptr()), 
                   KAVCNotCodedFrameBuffer, KAVCNotCodedFrameBuffer );
        
        TInt len = iAvcEdit->GenerateNotCodedFrame( buf, iModifiedFrameNumber++ );        
        
        if (len == 0)
            User::Leave(KErrArgument);
                
        TPtr8 temp(iNotCodedFrame->Des());
        temp.SetLength(len);        
#else
        VDASSERT(0, 190);
#endif
    }
    
    
}

// ---------------------------------------------------------
// CVideoProcessor::ProcessFrameL
// Processes one input frame
// (other items were commented in a header).
// ---------------------------------------------------------
//
TBool CVideoProcessor::ProcessFrameL()
    {


	PRINT((_L("CVideoProcessor::ProcessFrameL() begin")));

    VDASSERT(iCurrentFrameLength,1);    
        
    TInt frameInRange = 0;    
    TInt frameDuration = 0;    
    TBool keyFrame = EFalse; 
    TTimeIntervalMicroSeconds startCutTime = TTimeIntervalMicroSeconds(0);
    TTimeIntervalMicroSeconds endCutTime = TTimeIntervalMicroSeconds(0);
    TTimeIntervalMicroSeconds frameTime = TTimeIntervalMicroSeconds(0);

    TInt trP = iProcessor->GetTrPrevNew();    
    TInt trD = iProcessor->GetTrPrevOrig();

    // transitions
    iTransitionFrame = 0;			// is this a transition frame?
    iTransitionPosition = EPositionNone;
    iTransitionColor = EColorNone;    
    iFirstTransitionFrame = 0;		// is this the first transition frame in this instance?
    TBool endColorTransitionFrame = EFalse;
    
    TBool decodeCurrentFrame = 0;	// do we need to decode frame for transition effect?
   
    // book-keeping
    startCutTime = iProcessor->GetStartCutTime();
    endCutTime = iProcessor->GetEndCutTime();
	iRepeatFrame = EFalse;

    if(iInitializing)
    {
    
        // determine if we need to do full transcoding
		iFullTranscoding = DetermineResolutionChange() || DetermineFrameRateChange() || 
		                   DetermineBitRateChange();
		                   		
        // Do full transcoding for MPEG-4 => H.263. MPEG-4 => MPEG-4 and H.263 => MPEG-4 can be done in compressed domain
        if ( iProcessor->GetCurrentClipVideoType() == EVedVideoTypeMPEG4SimpleProfile &&
             iProcessor->GetOutputVideoType() != EVedVideoTypeMPEG4SimpleProfile )
            iFullTranscoding = ETrue;
		               
#ifdef VIDEOEDITORENGINE_AVC_EDITING
        // Do full transcoding for AVC => H.263/MPEG-4
		if ( iProcessor->GetCurrentClipVideoType() == EVedVideoTypeAVCBaselineProfile &&
		     iProcessor->GetOutputVideoType() != EVedVideoTypeAVCBaselineProfile )
            iFullTranscoding = ETrue;
            
        // Do full transcoding for H.263/MPEG-4 => AVC
        if ( iProcessor->GetCurrentClipVideoType() != EVedVideoTypeAVCBaselineProfile &&
		     iProcessor->GetOutputVideoType() == EVedVideoTypeAVCBaselineProfile )
            iFullTranscoding = ETrue;
                                            
        // Do color effects for AVC in spatial domain                    
        if ( iProcessor->GetOutputVideoType() == EVedVideoTypeAVCBaselineProfile &&
             iProcessor->GetColorEffect() != EVedColorEffectNone )
            iFullTranscoding = ETrue;
#endif

        // determine transition parameters for the clip 
        DetermineClipTransitionParameters(iTransitionEffect,iStartOfClipTransition,
            iEndOfClipTransition,iStartTransitionColor,iEndTransitionColor);
    
        if (!iTransCoder)
        {
            // initialize transcoder, normal mode
            CreateAndInitializeTranscoderL(iProcessor->GetCurrentClipVideoType(), CTRTranscoder::EFullTranscoding);
            return EFalse;
        }
    
		if ( iColorTransitionBuffer )
		{
			User::Free( iColorTransitionBuffer );
			iColorTransitionBuffer = 0;
		}
		
		if ( iOrigPreviousYUVBuffer )
		{
			User::Free( iOrigPreviousYUVBuffer );
			iOrigPreviousYUVBuffer = 0;
		}

        iFirstFrameInRange = 0;
        iFirstIncludedFrameNumber = -1;
        iTimeStampIndex = 0;
        iTimeStampOffset = 0;
        //iProcessor->iOutputFramesInClip=0; 
        iPreviousFrameIncluded = EFalse; 
        iNumberOfFrames = iProcessor->GetClipNumberOfFrames();	
        
        // calculate number of included frames
        if(iTransitionEffect)
        {
            GetNumberOfTransitionFrames(startCutTime, endCutTime);                    
        }
        iInitializing = EFalse;
    }

    TInt startFrame = iProcessor->GetOutputNumberOfFrames() - iNumberOfFrames;
    TInt absFrameNumber = startFrame + iFrameNumber;
    
    VDASSERT(startCutTime <= endCutTime,2);

    // microseconds
    frameTime = TTimeIntervalMicroSeconds(iProcessor->GetVideoTimeInMsFromTicks(
        iProcessor->VideoFrameTimeStamp(absFrameNumber), EFalse) * 1000);
    keyFrame = iProcessor->VideoFrameType(absFrameNumber);

	TInt cur = absFrameNumber; 
	TInt next = cur+1;
	
	TTimeIntervalMicroSeconds frameDurationInMicroSec(0);

	// frameDuration is in ticks, with timescale of the current input clip
	if(next >= iProcessor->GetOutputNumberOfFrames())	
	{	    
		frameDurationInMicroSec = 
			(iProcessor->GetVideoTimeInMsFromTicks(iProcessor->GetVideoClipDuration(), EFalse) * TInt64(1000)) - frameTime.Int64();		
		
		frameDuration = I64INT(iProcessor->GetVideoClipDuration() - iProcessor->VideoFrameTimeStamp(cur) );
	}
	else
	{
		frameDuration = I64INT( iProcessor->VideoFrameTimeStamp(next) - iProcessor->VideoFrameTimeStamp(cur) );
		frameDurationInMicroSec = 
			TTimeIntervalMicroSeconds(iProcessor->GetVideoTimeInMsFromTicks(TInt64(frameDuration), EFalse) * 1000);
	}
	
	TTimeIntervalMicroSeconds frameEndTime = 
		TTimeIntervalMicroSeconds( frameTime.Int64() + frameDurationInMicroSec.Int64() );    

	// endCutTime is in TTimeIntervalMicroSeconds

    // check if frame is in range for decoding/editing 
    frameInRange = ((frameEndTime <= endCutTime) ? 1 : 0);
    if(frameInRange)
    {        

        // transition is applied only for frame included in the output movie
        if(frameTime >= startCutTime)
        {
            // find the offset for the first included frame in the clip
            if(!iFirstFrameInRange)
            {
                iFirstFrameInRange = 1;
                iFirstIncludedFrameNumber = iFrameNumber;
                iModifiedFrameNumber = iFrameNumber + 1;  // +1 since number is incremented after modifying
            }
            TInt relativeIncludedFrameNumber = iFrameNumber - iFirstIncludedFrameNumber;            
            
            if(iTransitionEffect)
            {                
                // check if this is a transition frame & set transition parameters             
                SetTransitionFrameParams(relativeIncludedFrameNumber, decodeCurrentFrame);
            }
        }
        
        // check if this is an end color transition frame        
        if ( iTransitionFrame && iTransitionPosition == EPositionEndOfClip &&
		     iEndTransitionColor == EColorTransition )
		{
		    endColorTransitionFrame = ETrue;
			iFrameToEncode = EFalse; 
		}
				
        // check if we need to include this frame into output movie 
        if (frameTime < startCutTime)
        {
            // decode, but do not include in output movie 
            // iPreviousFrameIncluded = EFalse; 
            iFrameToEncode = EFalse; 
            iFrameOperation = EDecodeNoWrite; 
            // for decoding frames not writable to output movie, do not decode 
            // with any effects, because all information is need at P->I conversion            
        }
        else	// include in output movie 
        {
            
            // check if we need to encode it again as I-frame
            if (iFullTranscoding || (!iPreviousFrameIncluded && !keyFrame) || iTransitionFrame)
            {
                 // need to decode as P and encode as I
                 
                if (!endColorTransitionFrame)
                    iFrameToEncode = ETrue; 
				
                iFrameOperation = EDecodeNoWrite; 
                // for first decoding of P frame in a clip, do not decode with any effects;
                // instead, apply the effects in the spatial domain after decoding it as P;
                // then feed it to the encoder with the applied special effects
            }
            else
            {
            
#ifdef VIDEOEDITORENGINE_AVC_EDITING
                // check if we need to encode AVC frames after 
                // encoded cut frame or starting transition
                if (iDataFormat == EDataAVC && iEncodeUntilIDR)
                {
                    TPtr8 ptr(iDataBuffer, iCurrentFrameLength, iBufferLength);
                                        
                    if (iAvcEdit->IsNALUnitIDR(ptr))
                        iEncodeUntilIDR = 0; 
                    else
                    {
                        // encode
                        iFrameOperation = EDecodeNoWrite;				
				        if (!endColorTransitionFrame)
				            iFrameToEncode = ETrue;                        
                    }
                }
#endif
                
                if (!iEncodeUntilIDR)
                {   
                    // just copy the frame data as it is 
                
                    TInt colorEffect = TColorEffect2TInt(iProcessor->GetColorEffect());
                                        
                    iFrameToEncode = EFalse;
                    if(decodeCurrentFrame)
                        iFrameOperation = EDecodeAndWrite; 
                    else
                        iFrameOperation = (colorEffect==0/*None*/ ? EWriteNoDecode : EDecodeAndWrite);                
                }                
            }
            iPreviousFrameIncluded = ETrue;
        }

    }
    else
    {
        // no need to include frame in output movie
        iPreviousFrameIncluded = EFalse; 
        iFrameToEncode = EFalse;
        iFrameOperation = ENoDecodeNoWrite;

        // stop processing
        return ETrue;
    }
	
    TBool modeChanged = GetModeChangeL(); // do we need to change the current mode? 

	/* added to handle Mp4Specific size problem */
	if(modeChanged && !iFullTranscoding) 
	{
        iProcessor->SetClipModeChanged(modeChanged);	//if it is not set, it will be default false
	}

    if (iFrameOperation == EDecodeAndWrite)
        PRINT((_L("CVideoProcessor::ProcessFrameL() frame operation = EDecodeAndWrite")));
    if (iFrameOperation == EWriteNoDecode)
        PRINT((_L("CVideoProcessor::ProcessFrameL() frame operation = EWriteNoDecode")));        
    if (iFrameOperation == EDecodeNoWrite)
        PRINT((_L("CVideoProcessor::ProcessFrameL() frame operation = EDecodeNoWrite")));    
    if (iFrameOperation == ENoDecodeNoWrite)
        PRINT((_L("CVideoProcessor::ProcessFrameL() frame operation = ENoDecodeNoWrite")));    
    
    PRINT((_L("CVideoProcessor::ProcessFrameL() iFrameToEncode = %d"), iFrameToEncode));    
    
    TBool volHeaderIncluded = EFalse;
    
    if( (iFrameOperation == EDecodeAndWrite) || (iFrameOperation == EWriteNoDecode) ||
       ((iFrameOperation == EDecodeNoWrite) && !iFullTranscoding && iFirstFrameFlag) )
       // the last line is to enable processing of the 1st frame also if it would be decoded with transcoder, 
       // to enable processing of the MPEG-4 VOL header by vedh263d.
    {               
        
        TPtr8 ptr(0,0);
        TBool doCompressedDomainTC =  modeChanged || iProcessor->GetColorEffect() != EVedColorEffectNone;                        

        // If we need to do compressed domain bitstream manipulation at some
        // point of the clip, all frames must be decoded by vedh263d to be 
        // able to start bitstream modification in the middle of the clip, e.g.
        // after a transition. If we are processing MPEG-4, all frames are
        // manipulated by the decoder for changing timing information
                   
        if ( doCompressedDomainTC || (iDataFormat == EDataMPEG4 /*&& !iTransitionFrame*/) )
        {
            // use h263decoder to do bitstream modification
            
            // (if this is an end color transition frame, iFrameOperation is 
            //  EDecodeNoWrite && iFrameToEncode == 0
                
            TInt frameOp = 1;  // EDecodeAndWrite
            
            if ( iFrameOperation == EDecodeNoWrite )
                frameOp = 2; 
            
            if ( iFrameOperation == EWriteNoDecode && !modeChanged )
                frameOp = 3;  // EWriteNoDecode
        
            TInt frameSize;                                          
            
            if (iDataFormat == EDataMPEG4 && iFirstFrameFlag)
            {
                InsertDecoderSpecificInfoL();
                volHeaderIncluded = ETrue;
            }
            
            // use h263decoder to do compressed domain transcoding
            PRINT((_L("CVideoProcessor::ProcessFrameL() decode using vedh263d")));    
            DecodeFrameL(frameOp, modeChanged, frameSize);
            ptr.Set(iOutVideoFrameBuffer, frameSize, frameSize);                    
        }
        else
        {   
            // copy bitstream directly
            ptr.Set(iDataBuffer, iCurrentFrameLength, iCurrentFrameLength);
        }
        
        if (iFrameOperation == EDecodeAndWrite || iFrameOperation == EWriteNoDecode)
        {
#ifdef VIDEOEDITORENGINE_AVC_EDITING
            if (iDataFormat == EDataAVC && iTransitionEffect && 
                iStartTransitionColor == EColorTransition) 
            {
                if (!(iAvcEdit->IsNALUnitIDR(ptr)))
                {        
                    // modify frame number
                    VDASSERT( (iFrameNumber > iFirstIncludedFrameNumber), 182 );
                    iAvcEdit->ModifyFrameNumber(ptr, iModifiedFrameNumber++);       
                    PRINT((_L("CVideoProcessor::ProcessFrameL() modified frame no. => #%d"), iModifiedFrameNumber - 1));
                } 
                else
                    iModifiedFrameNumber = 1;  // this frame is IDR, start numbering from zero
            }
#endif
            // Write to file            
            if ( WriteFrameToFileL(ptr, frameDuration, absFrameNumber) )
                return ETrue;
        }
        
        if (iFrameOperation == EWriteNoDecode || 
            (iFrameOperation == EDecodeAndWrite && !decodeCurrentFrame && doCompressedDomainTC))
        {   
            // NOTE: The 2nd condition is for B&W & compr.domain TC
            
            // if we are doing only compressed domain transcoding, theres no need to
            // decode the frame using transcoder
            
            // Throw away the data for this frame:
            VDASSERT(iDataLength >= iCurrentFrameLength,4);
            Mem::Copy(iDataBuffer, iDataBuffer + iCurrentFrameLength,
                iDataLength - iCurrentFrameLength);
            iDataLength = iDataLength - iCurrentFrameLength;        
            iCurrentFrameLength = 0;			  
                        
            // update and fetch the new Time Code for MPEG4 ES
            if (iProcessor->GetOutputVideoType() == EVedVideoTypeMPEG4SimpleProfile &&
                frameInRange && (frameTime >= startCutTime) )
				
		    {
    			iMPEG4Timer->UpdateMPEG4Time(absFrameNumber, iFrameNumber, iProcessor->GetSlowMotionSpeed());
		    }
            
            iFrameNumber++;                    
            return EFalse; 
        }
    }
    
    // process using transcoder 
    if (iFrameOperation == EDecodeNoWrite || iFrameOperation == EDecodeAndWrite)
    {
        WriteFrameToTranscoderL(absFrameNumber, keyFrame, volHeaderIncluded);
    }
    
    // update and fetch the new Time Code for MPEG4 ES
    if (iProcessor->GetOutputVideoType() == EVedVideoTypeMPEG4SimpleProfile &&
        frameInRange && (frameTime >= startCutTime) )
	{
        iMPEG4Timer->UpdateMPEG4Time(absFrameNumber, iFrameNumber, iProcessor->GetSlowMotionSpeed());
    }

    iProcessor->SetTrPrevNew(trP);
    iProcessor->SetTrPrevOrig(trD);
    
    if (!iFirstColorTransitionFrame)
        iFrameNumber++;
               
    PRINT((_L("CVideoProcessor::ProcessFrameL() end")));

    return EFalse; 
}


// ---------------------------------------------------------
// CVideoProcessor::WriteFrameToFileL
// Write frame to file
// (other items were commented in a header).
// ---------------------------------------------------------
//
TBool CVideoProcessor::WriteFrameToFileL(TPtr8& aBuf, TInt aDurationInTicks, TInt aFrameNumber)
{    
   
    // if there's a frame waiting to be encoded, we must not
    // write this frame now. It will be written after all to-be-encoded
    // frames have been written. New frames must not be processed
    // until the delayed write has been completed                                
                            
    iDelayedWrite = !IsEncodeQueueEmpty();
    
    if (iDelayedWrite)
    {
        PRINT((_L("CVideoProcessor::WriteFrameToFileL() delayed write")));    
        // save frame for later writing            
        if (iDelayedBuffer)
            delete iDelayedBuffer;
        iDelayedBuffer = 0;
        
        iDelayedBuffer = (HBufC8*) HBufC8::NewL(aBuf.Length());                                
                        
        TPtr8 db(iDelayedBuffer->Des());        
        db.Copy(aBuf);
        
        iDelayedTimeStamp = iProcessor->VideoFrameTimeStamp(aFrameNumber) + iTimeStampOffset;
        iDelayedKeyframe = iProcessor->VideoFrameType(aFrameNumber);
        iDelayedFrameNumber = iFrameNumber;
        if ( IsNextFrameBeingEncoded() )
        {
            // start timer to wait for encoding to complete
            if ( !iTimer->IsPending() )
                iTimer->SetTimer( TTimeIntervalMicroSeconds32( iMaxEncodingDelay ) );
        }        
    } 
    else
    {
        // write now
        TInt error = iProcessor->WriteVideoFrameToFile(aBuf, 
          ( iProcessor->VideoFrameTimeStamp(aFrameNumber) + iTimeStampOffset ),
            aDurationInTicks, iProcessor->VideoFrameType(aFrameNumber), EFalse, EFalse, EFalse );
        
        // If movie has reached maximum size then stop processing   
        if (error == KErrCompletion)
        {
            iFrameInfoArray.Reset();
            return ETrue;
        }
        
        // save frame number
        iLastWrittenFrameNumber = iFrameNumber;
        
        User::LeaveIfError(error);                           
    }            
                  
    return EFalse;
    
}


// ---------------------------------------------------------
// CVideoProcessor::WriteFrameToTranscoderL
// Write frame to transcoder
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoProcessor::WriteFrameToTranscoderL(TInt aFrameNumber, TBool aKeyFrame, TBool aVolHeaderInBuffer)
{
     
    VDASSERT(iDataFormat != EDataUnknown, 30);
      
    // TODO: new buffertype for H.264
    CCMRMediaBuffer::TBufferType bt = 
        (iDataFormat == EDataH263) ? CCMRMediaBuffer::EVideoH263 : CCMRMediaBuffer::EVideoMPEG4;
   
    // insert dec. specific info header to beginning of buffer if it has not been sent
    if (!iDecoderSpecificInfoSent && 
        ( iDataFormat == EDataAVC || (iDataFormat == EDataMPEG4 && !aVolHeaderInBuffer) ) )
    {
        PRINT((_L("CVideoProcessor::WriteFrameToTranscoderL() - insert header"), iTranscoderMode));    
        InsertDecoderSpecificInfoL();
    }
    
    // determine transcoder mode for this frame
    TTranscoderMode mode;
            
    if (iFrameToEncode)
    {        
        if ( iTransitionFrame || (iProcessor->GetColorEffect() != EVedColorEffectNone) )
        {
            PRINT((_L("CVideoProcessor::WriteFrameToTranscoderL() encode current frame with intermediate modification")));
            mode = EFullWithIM;
        } 
        else
        {
            PRINT((_L("CVideoProcessor::WriteFrameToTranscoderL() encode current frame")));   
            mode = EFull;
        }            
    } 
    else
    {
        PRINT((_L("CVideoProcessor::WriteFrameToTranscoderL() only decode current frame")));   
        mode = EDecodeOnly;
    }                
    
    PRINT((_L("CVideoProcessor::WriteFrameToTranscoderL() iTranscoderMode %d"), iTranscoderMode));   
    if (iTranscoderMode != mode)
    {
        if (mode == EDecodeOnly)
        {
            iTransCoder->EnableEncoder(EFalse);
            iTransCoder->EnablePictureSink(ETrue);
        } 
        else if (mode == EFull)
        {
            iTransCoder->EnableEncoder(ETrue);
            iTransCoder->EnablePictureSink(EFalse);
        } 
        else
        {
            iTransCoder->EnableEncoder(ETrue);
            iTransCoder->EnablePictureSink(ETrue);
        }                                  
        iTranscoderMode = mode;
    }
    
    if (iFrameToEncode)
    {
        // should we encode an I-frame ?
        if( (iTransitionFrame && iFirstTransitionFrame) || 
            (iFrameNumber == iFirstIncludedFrameNumber) ||
             iFirstFrameAfterTransition )
        {
            iTransCoder->SetRandomAccessPoint();
        }
    }
    
    TFrameInformation frameInfo;        
    frameInfo.iTranscoderMode = iTranscoderMode;
    frameInfo.iFrameNumber = iFrameNumber;
    frameInfo.iEncodeFrame = iFrameToEncode;
    frameInfo.iKeyFrame = aKeyFrame;
    
    // this timestamp is in ticks for writing the frame
    frameInfo.iTimeStamp = iProcessor->VideoFrameTimeStamp(aFrameNumber) + iTimeStampOffset;
    frameInfo.iTransitionFrame = iTransitionFrame;
    frameInfo.iTransitionPosition = iTransitionPosition;
    frameInfo.iTransitionColor = iTransitionColor;
    frameInfo.iTransitionFrameNumber = iTransitionFrameNumber;
    frameInfo.iModificationApplied = EFalse;
    
    if(iTransitionFrame && iTransitionPosition == EPositionStartOfClip && 
       iStartTransitionColor == EColorTransition)
    {
        TInt duration;
        TInt64 currentTimeStamp = iProcessor->VideoFrameTimeStamp(aFrameNumber);
        
        // get timestamp for 1st frame            
        iProcessor->GetNextFrameDuration(duration, frameInfo.iTimeStamp, iTimeStampIndex, iTimeStampOffset);
        iTimeStampIndex++;
                    
        if (iFirstTransitionFrame)
            iCutInTimeStamp = currentTimeStamp;
        
        frameInfo.iTimeStamp += iCutInTimeStamp;
        
        // the duration parameter is not used actually, so no need to figure it out
        iProcessor->AppendNextFrameDuration(duration, currentTimeStamp - iCutInTimeStamp);
        
#ifdef VIDEOEDITORENGINE_AVC_EDITING    
        if (iDataFormat == EDataAVC)
        {        
        
            TPtr8 ptr(iDataBuffer, iCurrentFrameLength, iBufferLength);
            if (!iDecoderSpecificInfoSent)
            {
                ptr.Set(iDataBuffer + iProcessor->GetDecoderSpecificInfoSize(), 
                        iCurrentFrameLength - iProcessor->GetDecoderSpecificInfoSize(), iBufferLength);
            }
            
            // Store PPS id
            iAvcEdit->StoreCurrentPPSId( ptr );
           
            if (iNotCodedFrame)
                delete iNotCodedFrame;
            iNotCodedFrame = 0;                                 
        }        
#endif        
        
        frameInfo.iRepeatFrame = EFalse;            
        iFirstColorTransitionFrame = ETrue; // to indicate that iDataBuffer must not be flushed
                                            // in MtroReturnCodedBuffer
    }    
    
#ifdef VIDEOEDITORENGINE_AVC_EDITING    
    if ( iDataFormat == EDataAVC && iTransitionEffect && iStartTransitionColor == EColorTransition &&
         (iFrameNumber > iFirstIncludedFrameNumber) && iFrameOperation != EDecodeAndWrite)
    {
       TPtr8 ptr(iDataBuffer, iCurrentFrameLength, iBufferLength);
       
       if (!iDecoderSpecificInfoSent)
       {
           ptr.Set(iDataBuffer + iProcessor->GetDecoderSpecificInfoSize(), 
                        iCurrentFrameLength - iProcessor->GetDecoderSpecificInfoSize(), iBufferLength);
       }
       
       if (!(iAvcEdit->IsNALUnitIDR(ptr)))
       {        
           // modify frame number
           iAvcEdit->ModifyFrameNumber(ptr, iModifiedFrameNumber++);       
           PRINT((_L("CVideoProcessor::WriteFrameToTranscoderL() modified frame no. => #%d"), iModifiedFrameNumber - 1));
       }
       else
           iModifiedFrameNumber = 1;   // this frame is IDR, start numbering from zero
    }
#endif

    // get timestamp in microseconds
    TTimeIntervalMicroSeconds ts = 
        (iProcessor->GetVideoTimeInMsFromTicks(frameInfo.iTimeStamp, EFalse)) * 1000;
        
    if (ts <= iPreviousTimeStamp)
    {            
        // adjust timestamp so that its bigger than ts of previous frame
        TReal frameRate = iProcessor->GetVideoClipFrameRate();
        VDASSERT(frameRate > 0.0, 106);
        TInt64 durationMs =  TInt64( ( 1000.0 / frameRate ) + 0.5 );
        durationMs /= 2;   // add half the duration of one frame
        
        ts = TTimeIntervalMicroSeconds( iPreviousTimeStamp.Int64() + durationMs*1000 );
        
        frameInfo.iTimeStamp = iProcessor->GetVideoTimeInTicksFromMs( ts.Int64()/1000, EFalse );
                    
        ts = iProcessor->GetVideoTimeInMsFromTicks(frameInfo.iTimeStamp, EFalse) * 1000;
        
        PRINT((_L("CVideoProcessor::WriteFrameToTranscoderL() - adjusted timestamp, prev = %d, new = %d"), 
                   I64INT( iPreviousTimeStamp.Int64() ) / 1000, I64INT( ts.Int64() ) / 1000));    

    }
    
    iFrameInfoArray.Append(frameInfo);
            
    iPreviousTimeStamp = ts;
            
#ifdef VIDEOEDITORENGINE_AVC_EDITING
    if (iDataFormat == EDataAVC)
    {
        TPtr8 ptr(iDataBuffer, iCurrentFrameLength, iBufferLength);
        // This has to be updated when cutting from beginning ??
        iAvcEdit->ProcessAVCBitStreamL((TDes8&)(ptr), (TInt&)(iCurrentFrameLength), 
            iProcessor->GetDecoderSpecificInfoSize(), !iDecoderSpecificInfoSent );
        iDataLength = iCurrentFrameLength;
        
    }
#endif

    iMediaBuffer->Set( TPtrC8(iDataBuffer, iBufferLength), 
                              bt, 
                              iCurrentFrameLength, 
                              aKeyFrame,
                              ts);
                                       
    iDecodePending = ETrue;
    if (!IsActive())
    {
        SetActive();
        iStatus = KRequestPending;                                
    }                
    
    
    PRINT((_L("CVideoProcessor::WriteFrameToTranscoderL() - transcoder mode is %d"), iTranscoderMode));    
                              
    PRINT((_L("CVideoProcessor::WriteFrameToTranscoderL() - WriteCodedBuffer, frame #%d, timestamp %d ms"),
               iFrameNumber, I64INT( ts.Int64() ) / 1000 ));    
               
    PRINT((_L("CVideoProcessor::WriteFrameToTranscoderL() - %d items in queue"), iFrameInfoArray.Count()));                       
               
    iTransCoder->WriteCodedBufferL(iMediaBuffer);
    
}


// ---------------------------------------------------------
// CVideoProcessor::InsertDecoderSpecificInfoL
// Insert AVC dec. config record in the beginning of slice NAL('s)
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoProcessor::InsertDecoderSpecificInfoL()
{
    
    if (iDataFormat == EDataMPEG4)
    {
        if ( (iDataLength + iDecoderSpecificInfo->Length()) > iBufferLength )
		{
			// extend buffer size
			TUint newSize = iDataLength + iDecoderSpecificInfo->Length();															
            iDataBuffer = (TUint8*) User::ReAllocL(iDataBuffer, newSize);					                    
			iBufferLength = newSize;
		}            
    	Mem::Copy(iDataBuffer+iDecoderSpecificInfo->Length(), iDataBuffer, iCurrentFrameLength);
    	Mem::Copy(iDataBuffer, iDecoderSpecificInfo->Des().Ptr(), iDecoderSpecificInfo->Length());
    	iCurrentFrameLength += iDecoderSpecificInfo->Length();
    	iDataLength += iDecoderSpecificInfo->Length();
     
        return;   
    }
    
    VDASSERT( iDataFormat == EDataAVC, 182 );
            
    // get number of slice NAL's in buffer
    TInt frameLen = 0;        
    TInt numSliceNalUnits = 0;
    TUint8* frameLenPtr = iDataBuffer;
    
    while (frameLen < iCurrentFrameLength)
    {                
        TInt nalLen = 0;
        
        nalLen = (frameLenPtr[0] << 24) + (frameLenPtr[1] << 16) + 
                 (frameLenPtr[2] << 8) + frameLenPtr[3] + 4;  // +4 for length field
                                     
        frameLenPtr += nalLen;
        frameLen += nalLen;
        numSliceNalUnits++;
    }
    
    // get no. of SPS & PPS        

    TUint8* ptr = const_cast<TUint8*>(iDecoderSpecificInfo->Des().Ptr());                
    
    TInt index = 4;  // Skip version and length information                    
    ptr[index] |= 0x03;  // set no. bytes used for length to 4
    
    index++;            
    TInt numSPS = ptr[index] & 0x1f;
    
    index++;
                   
    // Loop all SPS units
    for (TInt i = 0; i < numSPS; ++i)
    {                                
        TInt SPSSize = (ptr[index] << 8) + ptr[index + 1];
        index += 2;
        index += SPSSize;
    }
    TInt numPPS = ptr[index];

    // Align at 32-bit boundrary
    TInt payLoadLen = iCurrentFrameLength + iDecoderSpecificInfo->Length();
    TInt alignmentBytes = (payLoadLen % 4 != 0) * ( 4 - (payLoadLen % 4) );                                
                    
    // get needed buffer length
    TInt minBufLen = iCurrentFrameLength + iDecoderSpecificInfo->Length() + alignmentBytes +
                     ( (numSliceNalUnits + numSPS + numPPS) * 8 ) + 4;
    
    // ReAllocate buffer
    if (iBufferLength < minBufLen)
    {            
        iDataBuffer = (TUint8*) User::ReAllocL(iDataBuffer, minBufLen);
        iBufferLength = minBufLen;
        
        PRINT((_L("CVideoProcessor::XXX() reallocated databuffer, new length = %d"),iBufferLength));
    }

    // move slice NAL's the amount of DCR length
    Mem:: Copy(iDataBuffer + iDecoderSpecificInfo->Length(), iDataBuffer, iCurrentFrameLength);
    
    // copy SPS/PPS data in the beginning
    Mem:: Copy(iDataBuffer, iDecoderSpecificInfo->Des().Ptr(), iDecoderSpecificInfo->Length());
    
    iCurrentFrameLength += iDecoderSpecificInfo->Length();
    
}

// ---------------------------------------------------------
// CVideoProcessor::GetModeChangeL
// Determine need to compr. domain transcoding
// (other items were commented in a header).
// ---------------------------------------------------------
//
TBool CVideoProcessor::GetModeChangeL()
{

    TInt videoClipNumber = iProcessor->GetVideoClipNumber(); 
	
	// iProcessor->GetModeTranslationMPEG4() returns the overall decision for inserted MPEG4 clips

	TVedTranscodeFactor tFact = iProcessor->GetVideoClipTranscodeFactor(videoClipNumber);
	
	TBool fModeChanged = EFalse;
	
	if (iProcessor->GetOutputVideoType() == EVedVideoTypeMPEG4SimpleProfile) //MPEG4
	{

		switch (tFact.iStreamType)
		{
		case EVedVideoBitstreamModeUnknown:
		case EVedVideoBitstreamModeMPEG4Regular:
		case EVedVideoBitstreamModeMPEG4Resyn:
			fModeChanged = EFalse; // already the target mode
			break;
		case EVedVideoBitstreamModeH263:
			fModeChanged = ETrue;	
			break;
		case EVedVideoBitstreamModeMPEG4ShortHeader:
		default: // other MPEG4 modes
			// if all the MPEG4 (note: it is also considered as MPEG4 type) have the same mode
			// no need to do the mode translation
			fModeChanged = iProcessor->GetModeTranslationMpeg4() ? ETrue: EFalse;
			break;
		}

	}
	else if ( (iProcessor->GetOutputVideoType() == EVedVideoTypeH263Profile0Level10) ||
		      (iProcessor->GetOutputVideoType() == EVedVideoTypeH263Profile0Level45) )
	{

		if (tFact.iStreamType == EVedVideoBitstreamModeH263 || 
			tFact.iStreamType == EVedVideoBitstreamModeMPEG4ShortHeader||
			tFact.iStreamType ==EVedVideoBitstreamModeUnknown)
		{
			fModeChanged = EFalse;
		}
		else
		{
			fModeChanged = ETrue;
		}
	} 
	
#ifdef VIDEOEDITORENGINE_AVC_EDITING
	else if (iProcessor->GetOutputVideoType() == EVedVideoTypeAVCBaselineProfile)
	    fModeChanged = EFalse;
#endif
	
	else // EVedVideoTypeNoVideo
	{
        User::Leave(KErrNotSupported);
	}
	
	return fModeChanged;
    
}

// ---------------------------------------------------------
// CVideoProcessor::DecodeFrameL
// Decode frame in iDataBuffer using vedh263d
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoProcessor::DecodeFrameL(TInt aOperation, TBool aModeChanged, TInt& aFrameSizeInBytes)
{

    VDASSERT(iDataFormat == EDataH263 || iDataFormat ==  EDataMPEG4, 136);
    
    // go into the decoder 
    TVedColorEffect vedEffect = iProcessor->GetColorEffect();
    CVedH263Dec::TColorEffect effect = (vedEffect == EVedColorEffectNone) ? 
	    CVedH263Dec::EColorEffectNone : ((vedEffect == EVedColorEffectBlackAndWhite) ? CVedH263Dec::EColorEffectBlackAndWhite : CVedH263Dec::EColorEffectToning);

	vdeDecodeParamters_t decodeParams;
	
	// Assign the ColorTone value of the ColorTone	
	// U,V value for the color toning
    TInt colorToneU;
    TInt colorToneV;
    iProcessor->GetColorTone((TInt&)colorToneU, (TInt&)colorToneV);
	
    decodeParams.aColorToneU = colorToneU;
    decodeParams.aColorToneV = colorToneV;
	decodeParams.aColorEffect = effect;
	decodeParams.aFrameOperation = aOperation;
	decodeParams.aGetDecodedFrame = EFalse; //getDecodedFrame; // no need to get YUV
	decodeParams.aSMSpeed = iProcessor->GetSlowMotionSpeed();
	
	TInt trD = iProcessor->GetTrPrevOrig();
	TInt trP = iProcessor->GetTrPrevNew();
	
	decodeParams.aTrD = &trD;
	decodeParams.aTrP = &trP;
	decodeParams.aVideoClipNumber = iProcessor->GetVideoClipNumber();
	
	TVedTranscodeFactor tFact = iProcessor->GetVideoClipTranscodeFactor(decodeParams.aVideoClipNumber);
	
	decodeParams.streamMode = tFact.iStreamType;
	decodeParams.iTimeIncrementResolution = tFact.iTRes;
	decodeParams.aGetVideoMode = EFalse;
	decodeParams.aOutputVideoFormat = iProcessor->GetOutputVideoType();
	
	decodeParams.fModeChanged = aModeChanged;
	decodeParams.fHaveDifferentModes = iProcessor->GetModeTranslationMpeg4() ? ETrue: EFalse;
	/* Color Toning */
	decodeParams.aFirstFrameQp = iFirstFrameQp;
		
	// : Optimisation - If the frame is to be encoded, there is no need
	//       to process it using vedh263d in all cases, for example when
	//       doing end transition. In start transition case it has to be done
	//       so that compressed domain transcoding can continue after transition

	// before decoding, set the time infomation in the decoder parameters
	decodeParams.aMPEG4TimeStamp = iMPEG4Timer->GetMPEG4TimeStampPtr();
	decodeParams.aMPEG4TargetTimeResolution = iMPEG4Timer->GetMPEG4TimeResolutionPtr();
	    
	decodeParams.vosHeaderSize = 0;
    
    // +3 includes the next PSC or EOS in the bit buffer
    TPtrC8 inputPtr(iDataBuffer, iCurrentFrameLength + (iDataFormat==EDataH263 ? KH263StartCodeLength : KMPEG4StartCodeLength));
    
    // check output buffer size & reallocate if its too small    
    if ( TReal(iOutVideoFrameBufferLength) < TReal(iCurrentFrameLength) * 1.5 )
    {
        TInt newLen = TInt( TReal(iCurrentFrameLength) * 1.5 );
        
        iOutVideoFrameBuffer = (TUint8*) User::ReAllocL(iOutVideoFrameBuffer, newLen);
        iOutVideoFrameBufferLength = newLen;                
        
        PRINT((_L("CVideoProcessor::DecodeFrameL() reallocated output buffer, new size = %d"),
                   iOutVideoFrameBufferLength));
    }
    
    TPtr8 outputPtr(iOutVideoFrameBuffer, 0, iOutVideoFrameBufferLength);        
    
    TInt frameSize = 0;
    TBool wasFirstFrame = iFirstFrameFlag; // need to save the value since it may be changed inside
    iDecoder->DecodeFrameL(inputPtr, outputPtr, iFirstFrameFlag, frameSize, &decodeParams);

	if (frameSize > (TInt)iCurrentFrameLength)
    {
        // decoder used more data than was in the buffer => corrupted bitstream
        PRINT((_L("CVideoProcessor::DecodeFrameL() decoder used more data than was in the buffer => corrupted bitstream")));
        User::Leave( KErrCorrupt );
    }
    
    aFrameSizeInBytes = outputPtr.Length();    
    
    /* record first frame QP */
    if ((iFrameNumber==0) && wasFirstFrame) 
    {
        iFirstFrameQp = decodeParams.aFirstFrameQp;
	    
        if (iProcessor->GetOutputVideoType() == EVedVideoTypeMPEG4SimpleProfile)
            {
            PRINT((_L("CVideoProcessor::DecodeFrameL() save VOS header, size %d"), decodeParams.vosHeaderSize));
            // sync the vol headers
            if ( decodeParams.vosHeaderSize > iOutputVolHeader->Des().MaxLength() )
                {
                delete iOutputVolHeader;
                iOutputVolHeader = NULL;
                iOutputVolHeader = HBufC8::NewL(decodeParams.vosHeaderSize);
                }
            iOutputVolHeader->Des().Copy( outputPtr.Ptr(), decodeParams.vosHeaderSize );
            }
    }
    
    PRINT((_L("CVideoProcessor::DecodeFrameL() out")));
}

// ---------------------------------------------------------
// CVideoProcessor::CreateAndInitializeTranscoderL
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoProcessor::CreateAndInitializeTranscoderL(TVedVideoType aInputType, CTRTranscoder::TTROperationalMode aMode)
{

    PRINT((_L("CVideoProcessor::CreateAndInitializeTranscoderL() begin")));    
    
    VDASSERT(iTransCoder == 0, 27);
    
    iTransCoder = CTRTranscoder::NewL(*this); 
    
    TBufC8<256> inputMime;
    
//  TInt volLength = 0;    
    TInt outputBufferSize = 0;
    iInputMPEG4ProfileLevelId = 0;

    if (aInputType == EVedVideoTypeMPEG4SimpleProfile)
    {                
        // Get the VOL header from the frame data
	    CVedVolReader* reader = CVedVolReader::NewL();
	    CleanupStack::PushL(reader);
	    // Get pointer to the frame data
	    TPtrC8 inputPtr(iDecoderSpecificInfo->Des());
	    reader->ParseVolHeaderL((TDesC8&) inputPtr);
	    
	    iInputMPEG4ProfileLevelId = reader->ProfileLevelId();	    
	    
	    iInputTimeIncrementResolution = reader->TimeIncrementResolution();
	    
	    iInputStreamMode = reader->BitstreamMode();

	    switch (iInputMPEG4ProfileLevelId)
	    {		    
	    	case 8:
	    	    inputMime = _L8("video/mp4v-es; profile-level-id=8");
	    	    outputBufferSize = KMaxCodedPictureSizeMPEG4QCIF / 2;
	    	    break;
	    	
	    	case 9:	    	
	    	    inputMime = _L8("video/mp4v-es; profile-level-id=9");
	    	    outputBufferSize = KMaxCodedPictureSizeMPEG4L0BQCIF / 2;
	    	    break;
	    	
	    	case 1:
	    	    inputMime = _L8("video/mp4v-es; profile-level-id=1");	    	    
	    	    outputBufferSize = KMaxCodedPictureSizeMPEG4QCIF / 2;
	    	    break;
	    	
	    	case 2:
	    	    inputMime = _L8("video/mp4v-es; profile-level-id=2");
	    	    outputBufferSize = KMaxCodedPictureSizeMPEG4CIF / 2;
	    	    break;
	    	
	    	case 3:
	    	    inputMime = _L8("video/mp4v-es; profile-level-id=3");
	    	    outputBufferSize = KMaxCodedPictureSizeMPEG4CIF / 2;
	    	    break;
	    	    
	        case 4:
	    	    inputMime = _L8("video/mp4v-es; profile-level-id=4");
	    	    outputBufferSize = KMaxCodedPictureSizeVGA / 2;
	    	    break;
	    		    		    	
	    	default:
	    	    inputMime = _L8("video/mp4v-es; profile-level-id=8");
	    	    outputBufferSize = KMaxCodedPictureSizeMPEG4QCIF / 2;
	    	    break;
	    }
	    
//	    volLength = reader->HeaderSize();
	    CleanupStack::PopAndDestroy(reader);        
    }    
    
    else if (aInputType == EVedVideoTypeH263Profile0Level10)
    {        
        inputMime = _L8("video/H263-2000; profile=0; level=10");
        outputBufferSize = KMaxCodedPictureSizeQCIF / 2;
    }
    
    else if (aInputType == EVedVideoTypeH263Profile0Level45)
    {        
        inputMime = _L8("video/H263-2000; profile=0; level=45");
        outputBufferSize = KMaxCodedPictureSizeQCIF / 2;
    }
    
    else if (aInputType == EVedVideoTypeAVCBaselineProfile)
    {
        // get input avc level        
        VDASSERT( iAvcEdit != 0, 181 );        
        VDASSERT( iDecoderSpecificInfo, 181 );
        
        TPtr8 info = iDecoderSpecificInfo->Des();
        User::LeaveIfError( iAvcEdit->GetLevel(info, iInputAVCLevel) );
                
        switch (iInputAVCLevel)
        {
            case 10:
                inputMime = _L8("video/H264; profile-level-id=42800A");                
                outputBufferSize = KMaxCodedPictureSizeAVCLevel1 / 2;
                break;
           
            case 101:
                inputMime = _L8("video/H264; profile-level-id=42900B");
                outputBufferSize = KMaxCodedPictureSizeAVCLevel1B / 2;
                break;
            
            case 11:
                inputMime = _L8("video/H264; profile-level-id=42800B");
                outputBufferSize = KMaxCodedPictureSizeAVCLevel1_1 / 2;
                break;
            
            case 12:
                inputMime = _L8("video/H264; profile-level-id=42800C");
                outputBufferSize = KMaxCodedPictureSizeAVCLevel1_2 / 2;
                break;
                
            // NOTE: Levels 1.3 and 2 are enabled for testing purposes,
            //       to be removed
            case 13:
                inputMime = _L8("video/H264; profile-level-id=42800D");
                outputBufferSize = KMaxCodedPictureSizeAVCLevel1_3 / 2;
                break;  
                
            case 20:
                inputMime = _L8("video/H264; profile-level-id=428014");
                outputBufferSize = KMaxCodedPictureSizeAVCLevel2 / 2;
                break;
                
            //WVGA task
            case 21:
                inputMime = _L8("video/H264; profile-level-id=428015");
                outputBufferSize = KMaxCodedPictureSizeAVCLevel2_1 / 2;
                break;
                
            case 22:
            	inputMime = _L8("video/H264; profile-level-id=428016");
            	outputBufferSize = KMaxCodedPictureSizeAVCLevel2_2 / 2;
            	break;                

            case 30:
            	inputMime = _L8("video/H264; profile-level-id=42801E");
            	outputBufferSize = KMaxCodedPictureSizeAVCLevel3 / 2;
            	break;   
               
            case 31:
            	inputMime = _L8("video/H264; profile-level-id=42801F");
            	outputBufferSize = KMaxCodedPictureSizeAVCLevel3_1 / 2;
            	break;                             
   
            default:
                User::Leave(KErrNotSupported);
                break;            
        }              
    }
           
    else
        User::Leave(KErrNotSupported);                  
    
    if ( !(iTransCoder->SupportsInputVideoFormat(inputMime) ) )
        {
        User::Leave(KErrNotSupported);
        }            
        
        
    // default framerate is 15 fps
    TReal frameRate = 15.0;
    
    iOutputBitRate = 64000;
        
    if ( aMode == CTRTranscoder::EFullTranscoding )
    {                
        // get output mime type 
        SetOutputVideoCodecL(iProcessor->GetOutputVideoMimeType());    
        
        if ( !(iTransCoder->SupportsOutputVideoFormat(iOutputMimeType) ) )
            {
            User::Leave(KErrNotSupported);
            }            
        
        // check output resolution
        TSize outputResolution = iProcessor->GetMovieResolution();
        
        if ( (outputResolution.iWidth > iMaxOutputResolution.iWidth) || (outputResolution.iHeight > iMaxOutputResolution.iHeight))
            {
            if ( iArbitrarySizeAllowed ) // This is for future-proofness. Currently the checking of standard sizes below overrules this one
                {
                if ( outputResolution.iWidth * outputResolution.iHeight > iMaxOutputResolution.iWidth*iMaxOutputResolution.iHeight )
                    {
                    PRINT((_L("CVideoProcessor::CreateAndInitializeTranscoderL() too high resolution requested")));
                    User::Leave( KErrNotSupported );
                    }
                }
            else
                {
                PRINT((_L("CVideoProcessor::CreateAndInitializeTranscoderL() incompatible or too high resolution requested")));
                User::Leave( KErrNotSupported );
                }
            }
        
    	// check size. For now only standard sizes are allowed
    	if ( (outputResolution != KVedResolutionSubQCIF) && 
    		 (outputResolution != KVedResolutionQCIF) &&
    		 (outputResolution != KVedResolutionCIF) &&
    		 (outputResolution != KVedResolutionQVGA) &&
    		 (outputResolution != KVedResolutionVGA16By9) &&
    		 (outputResolution != KVedResolutionVGA) &&
    		 //WVGA task
    		 (outputResolution != KVedResolutionWVGA) )
    	{
    		 User::Leave( KErrArgument );
    	}
    	
    	// check output frame rate    	
    	TReal movieFrameRate = iProcessor->GetMovieFrameRate();
    	
    	if ( movieFrameRate > 0.0 )
    	{	    	
        	if ( movieFrameRate <= iMaxOutputFrameRate )
            {
                frameRate = TReal32(movieFrameRate);
            }
            else
            {
                frameRate = iMaxOutputFrameRate;
            }
    	}    	
            
        // check output bitrate    
        TInt movieBitRate = iProcessor->GetMovieVideoBitrate();
        TInt standardBitRate = iProcessor->GetMovieStandardVideoBitrate();
        
        if ( movieBitRate > 0 )
        {            
            if ( movieBitRate <= iMaxOutputBitRate )
            {
                iOutputBitRate = movieBitRate;
            }
            else
            {
                iOutputBitRate = iMaxOutputBitRate;
            }
        } 
        else if ( standardBitRate > 0 )
        {
            if ( standardBitRate <= iMaxOutputBitRate )
            {
                iOutputBitRate = standardBitRate;
            }
            else
            {
                iOutputBitRate = iMaxOutputBitRate;
            }
        }    
    } 
    else 
    {
       iOutputMimeType = KNullDesC8;   
    }   
                
    TTRVideoFormat videoInputFormat;
    TTRVideoFormat videoOutputFormat;
    
    if (!iThumbnailMode)
    {
        videoInputFormat.iSize = iProcessor->GetVideoClipResolution();
        videoOutputFormat.iSize = iProcessor->GetMovieResolution();
    }
    else
    {
        videoInputFormat.iSize = videoOutputFormat.iSize = TSize(iVideoWidth, iVideoHeight);
    }
    
    videoInputFormat.iDataType = CTRTranscoder::ETRDuCodedPicture;
    
    if (aMode == CTRTranscoder::EFullTranscoding)
        videoOutputFormat.iDataType = CTRTranscoder::ETRDuCodedPicture;
    else
        videoOutputFormat.iDataType = CTRTranscoder::ETRYuvRawData420;
    

    iTransCoder->OpenL( this,
                        aMode,                        
                        inputMime,
                        iOutputMimeType,
                        videoInputFormat,
                        videoOutputFormat,
                        EFalse );    
    
     
    iTransCoder->SetVideoBitRateL(iOutputBitRate);
    
    if (!iThumbnailMode)
    { 
        // check framerate: target framerate cannot be larger than source framerate
        TReal inputFR = iProcessor->GetVideoClipFrameRate();       
        if ( inputFR <= 15.0 )
        {
            inputFR = 15.0;
        }
        else
        {
            inputFR = 30.0;
        }    
        if (frameRate > inputFR)
            frameRate = inputFR;
    }
    
    iTransCoder->SetFrameRateL(frameRate);           
    iTransCoder->SetChannelBitErrorRateL(0.0);         
       
    // dummy
    TTRVideoCodingOptions codingOptions;
    codingOptions.iSyncIntervalInPicture = iProcessor->GetSyncIntervalInPicture();
    codingOptions.iMinRandomAccessPeriodInSeconds = (TInt) (1.0 / iProcessor->GetRandomAccessRate());        
    codingOptions.iDataPartitioning = EFalse;
    codingOptions.iReversibleVLC = EFalse;
    codingOptions.iHeaderExtension = 0;
   
    iTransCoder->SetVideoCodingOptionsL(codingOptions);
   
    TSize targetSize;
    if (!iThumbnailMode)
        targetSize = iProcessor->GetMovieResolution();
    else
        targetSize = TSize(iVideoWidth, iVideoHeight);
    
    iTransCoder->SetVideoPictureSinkOptionsL(targetSize, this);
    
    iTransCoder->EnableEncoder(EFalse);
    iTransCoder->EnablePictureSink(ETrue);
    iTranscoderMode = EDecodeOnly;
    
    // set init. data
    TPtrC8 initData;
    if (aInputType == EVedVideoTypeMPEG4SimpleProfile || 
        aInputType == EVedVideoTypeAVCBaselineProfile)
    {
        initData.Set(iDecoderSpecificInfo->Des());
    }
    else
        initData.Set(iDataBuffer, iCurrentFrameLength); 
    
    iDecoderSpecificInfoSent = EFalse;
    
    iTransCoder->SetDecoderInitDataL( initData );
    
    if (!iThumbnailMode)
    {        
        // allocate output bitstream buffer for processing with vedh263d
        VDASSERT( outputBufferSize != 0, 52 );
        iOutVideoFrameBuffer = (TUint8*) User::AllocL(outputBufferSize);
    	iOutVideoFrameBufferLength = outputBufferSize;
    }
        
    iTranscoderInitPending = ETrue;               
    
    if (!IsActive())    
    {        
        SetActive();
        iStatus = KRequestPending;        
    }
    
    
    iTransCoder->InitializeL();
    
    // Get processing time estimate from transcoder, divide it by the framerate to get processing time per frame
    // and then multiply it by 2 to get some safety margin and by unit conversion factor 1000000. 
    // The delay is used to determine if a frame was skipped, hence there should be some margin.
#ifdef __WINSCW__
    iMaxEncodingDelay = 5000000;    // emulator can be really slow, use 5 seconds timeout
#else    
    iMaxEncodingDelay = (TUint)(2*1000000*iTransCoder->EstimateTranscodeTimeFactorL(videoInputFormat,videoOutputFormat)/frameRate);
#endif
    
    iMaxItemsInProcessingQueue = iTransCoder->GetMaxFramesInProcessing();
    
    PRINT((_L("CVideoProcessor::CreateAndInitializeTranscoderL() end")));    
}

// ---------------------------------------------------------
// CVideoProcessor::MtroInitializeComplete
// Called by transcoder to indicate init. completion
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoProcessor::MtroInitializeComplete(TInt aError)
{    

    TInt error = aError;
    TInt outputTimeIncrementResolution = KDefaultTimeIncrementResolution;
    
    PRINT((_L("CVideoProcessor::MtroInitializeComplete() error = %d"), aError));
    
    if ( !iThumbnailMode && (aError == KErrNone) &&
         ( (iProcessor->GetOutputVideoType() == EVedVideoTypeMPEG4SimpleProfile) 
#ifdef VIDEOEDITORENGINE_AVC_EDITING
        || (iProcessor->GetOutputVideoType() == EVedVideoTypeAVCBaselineProfile) 
#endif
         ) )
    {    
        PRINT((_L("CVideoProcessor::MtroInitializeComplete() calling GetCodingStandardSpecificInitOutputLC")));
        
        // get & save vol header from encoder
	    TRAP(error,
	    {
	        iOutputVolHeader = iTransCoder->GetCodingStandardSpecificInitOutputLC();
	        CleanupStack::Pop();
	    });
	    
	    iOutputVolHeaderWritten = EFalse;
	    
	    if ( error == KErrNone )
	    {
	    
#ifdef VIDEOEDITORENGINE_AVC_EDITING
	        if (iProcessor->GetOutputVideoType() == EVedVideoTypeAVCBaselineProfile)
	        {
	            // Check if we need to use encoder
	            // : Are there any other cases where encoder is used _
	            if ( iFullTranscoding || iTransitionEffect || 
	                 iProcessor->GetStartCutTime() != TTimeIntervalMicroSeconds(0) )
	            {	                
    	            HBufC8* outputAVCHeader = 0;
    	            // is the max. size of AVCDecoderConfigurationRecord known here ??
    	            TRAP(error, outputAVCHeader = (HBufC8*) HBufC8::NewL(16384));	            

                    if (error == KErrNone)
                    {   
        	            TPtr8 ptr = outputAVCHeader->Des();
        	            
        	            // parse header & convert it to AVCDecoderConfigurationRecord -format
        	            TRAP(error, iAvcEdit->ConvertAVCHeaderL(*iOutputVolHeader, ptr));
        	            
        	            if (error == KErrNone)
        	            {
        	                TRAP(error, iAvcEdit->SaveAVCDecoderConfigurationRecordL(ptr, ETrue));    	                
        	            }    	        
                    } 
                    if (outputAVCHeader)
                        delete outputAVCHeader;
	            }
	            
	            iEncodeUntilIDR = 0;
	            if ( iStartOfClipTransition != 0 || 
	                 iProcessor->GetStartCutTime() != TTimeIntervalMicroSeconds(0) )
	            {
	                // we need to use encoder at the beginning, now determine
	                // if we have to encode frames after cut / transition until
	                // input frame is IDR
	                iEncodeUntilIDR = iAvcEdit->EncodeUntilIDR();
	            }
	        }
	        else
#endif
	        {
	        
    	        VDASSERT(iOutputVolHeader, 49);

    	        // get time increment resolution using vol reader
    	        CVedVolReader* reader = NULL;	                                
    	        TRAP( error, reader = CVedVolReader::NewL() );	        
    	        
    	        if ( error == KErrNone )
    	        {	            	            
    	            TRAP( error, reader->ParseVolHeaderL( (TDesC8&) *iOutputVolHeader ) );
    	            if (error == KErrNone)
    	            {	                
                        outputTimeIncrementResolution = reader->TimeIncrementResolution();     
    	            }
    	            delete reader;	            	            	            	            
    	        }
	        }

	    }
    }

    if (error == KErrNone)
    {        
        // create MPEG-4 timing instance
        TRAP(error, iMPEG4Timer = CMPEG4Timer::NewL(iProcessor, outputTimeIncrementResolution));
    }   
    
    // enable pausing
    if ( ((iFullTranscoding) 
           || (iProcessor->GetStartCutTime() > 0))
        && (iStartOfClipTransition == 0) 
        && (iEndOfClipTransition == 0) )
        {
        // safe to enable pausing during transcoding: 
        // only when doing full transcoding or cutting from the beginning, but not if transitions
        // rules out e.g. thumbnails, and codec-free cases
        iTransCoder->EnablePausing(ETrue);
        }
            
    VDASSERT(iTranscoderInitPending, 28);
    // complete request    
    TRequestStatus *status = &iStatus;
    User::RequestComplete(status, error);
        
}

// ---------------------------------------------------------
// CVideoProcessor::MtroFatalError
// Called by transcoder to indicate a fatal error
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoProcessor::MtroFatalError(TInt aError)
{
    PRINT((_L("CVideoProcessor::MtroFatalError() %d"), aError));   
    
    if (iFullTranscoding || iThumbnailMode || iTransitionEffect || (iProcessor->GetStartCutTime() > 0))
        {
        // ok, this is fatal, continue the method
        PRINT((_L("CVideoProcessor::MtroFatalError() transcoder is in use, this is fatal")));   
        }
    else
        {
        // transcoder not in use, ignore
        PRINT((_L("CVideoProcessor::MtroFatalError() transcoder not in use, ignore")));   
        return;
        }
        
    // stop decoding
    Stop();
    
    if (!iThumbnailMode)
        iMonitor->Error(aError);
    else
        iProcessor->NotifyThumbnailReady(aError);

}

// ---------------------------------------------------------
// CVideoProcessor::MtroSuspend
// Suspends processing
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoProcessor::MtroSuspend()
{

    PRINT((_L("CVideoProcessor::MtroSuspend()")));   
    
    if (iProcessingComplete || (!iProcessor->NeedTranscoderAnyMore()))
        {
        PRINT((_L("CVideoProcessor::MtroSuspend(), this clip done or no video at all to process any more, ignore")));
        return;
        }
    
    Cancel();
	iDecoding = EFalse;
	iDecodePending = EFalse;
	iDecodingSuspended = EFalse;
		
    if (iTimer)
        iTimer->CancelTimer();
            
    iProcessor->SuspendProcessing();
    
    // flush input queue
    if (iBlock)    
		iQueue->ReturnBlock(iBlock);
	iBlock = iQueue->ReadBlock();
	while (iBlock)
	{
		iQueue->ReturnBlock(iBlock);
		iBlock = iQueue->ReadBlock();
	}
	iBlockPos = 0;
	
	iTranscoderMode = EUndefined;   // force to reset the mode when writing the next picture
	
}

// ---------------------------------------------------------
// CVideoProcessor::MtroResume
// Re-starts processing after pause
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoProcessor::MtroResume()
{
    PRINT((_L("CVideoProcessor::MtroResume()")));
    
    if (iProcessingComplete)
        {
        PRINT((_L("CVideoProcessor::MtroResume(), processing of this clip completed, continue")));   
        // fake RunL with this flag
        iDecoding = ETrue;
        if (!IsActive())
            {
             // Make us active
            PRINT((_L("CVideoProcessor::MtroResume() set active")));
            SetActive();
            iStatus = KRequestPending;
            }
        }
        
    if (!iProcessor->NeedTranscoderAnyMore())
        {
        PRINT((_L("CVideoProcessor::MtroResume(), no video to process any more, ignore")));   
        return;
        }
    
    // flush frame info array and cancel timer
    if (iTimer)
        iTimer->CancelTimer();
    iFrameInfoArray.Reset();
    
    Start();

    TInt error = iProcessor->ResumeProcessing(iFrameNumber, iLastWrittenFrameNumber);
    if (error != KErrNone)
        iMonitor->Error(error);
    
    iNumberOfFrames = iProcessor->GetClipNumberOfFrames();                 
    iPreviousTimeStamp = TTimeIntervalMicroSeconds(-1);
    
    iDataLength = iCurrentFrameLength = 0;
    iDataFormat = EDataUnknown;
    
    iStreamEnd = iStreamEndRead = 0;
    
    // reset also delayed buffer; it will need to be anyway re-read
    delete iDelayedBuffer;
    iDelayedBuffer = 0;
    iDelayedWrite = EFalse;        
        
    PRINT((_L("CVideoProcessor::MtroResume() - iFrameNumber = %d"), iFrameNumber));
    
    if (!IsActive())
    {
         // Make us active
        PRINT((_L("CVideoProcessor::MtroResume() set active")));
        SetActive();
        iStatus = KRequestPending;
    }

    PRINT((_L("CVideoProcessor::MtroResume() out")));
    
}

// ---------------------------------------------------------
// CVideoProcessor::MtroReturnCodedBuffer
// Called by transcoder to return bitstream buffer
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoProcessor::MtroReturnCodedBuffer(CCMRMediaBuffer* aBuffer)
{

    VDASSERT(aBuffer == iMediaBuffer, 29);

    iIsThumbFrameBeingCopied = EFalse;
    iDecoderSpecificInfoSent = ETrue;
    
#ifdef _DEBUG
    TTimeIntervalMicroSeconds ts = aBuffer->TimeStamp();        
    
    PRINT((_L("CVideoProcessor::MtroReturnCodedBuffer() timeStamp = %d ms"), 
             I64INT( ts.Int64() ) / 1000 ));                    

#endif
    
    if (!iFirstColorTransitionFrame)
    {        
        // Throw away the data for this frame:
        VDASSERT(iDataLength >= iCurrentFrameLength,4);
        Mem::Copy(iDataBuffer, iDataBuffer + iCurrentFrameLength,
            iDataLength - iCurrentFrameLength);
        iDataLength = iDataLength - iCurrentFrameLength;        
        iCurrentFrameLength = 0;
    }
    
    if (!iThumbnailMode)
    {            
    
        // check if the next frame in queue is waiting to be encoded
        // and start timer to detect possible frameskip
        if ( IsNextFrameBeingEncoded() )
        {
            if ( !iTimer->IsPending() )
                iTimer->SetTimer( TTimeIntervalMicroSeconds32( iMaxEncodingDelay ) );
        }       

        if (iFrameInfoArray.Count() >= iMaxItemsInProcessingQueue)
        {
            PRINT((_L("CVideoProcessor::MtroReturnCodedBuffer() - %d items in queue, suspend decoding"), 
             iFrameInfoArray.Count() ));   
        
            iDecodingSuspended = ETrue;
                                                         
            return;                        
        }
    
        VDASSERT(IsActive(), 40);        
        VDASSERT(iDecodePending, 41);
            
        // complete request    
        TRequestStatus *status = &iStatus;
        User::RequestComplete(status, KErrNone);
    } 
    else if (iDataFormat == EDataAVC)
    {

        VDASSERT(IsActive(), 40);        
        VDASSERT(iDecodePending, 41);
        
        // NOTE: it would make sense to call AsyncStopL() here, 
        // but at least in WINSCW it didn't  have any effect        
         //if (iThumbFramesToWrite == 0)
               //{
               //iTransCoder->AsyncStopL();
               //iTranscoderStarted = EFalse;               
               //}
            
        if (iStatus == KRequestPending)
        {
            PRINT((_L("CVideoProcessor::MtroReturnCodedBuffer() - completing request")));
            // complete request    
            TRequestStatus *status = &iStatus;
            User::RequestComplete(status, KErrNone);
        }
    }
}

// ---------------------------------------------------------
// CVideoProcessor::MtroSetInputFrameRate
// Called by transcoder to request inout framerate
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoProcessor::MtroSetInputFrameRate(TReal& aRate)
{    
    TReal rate = iProcessor->GetVideoClipFrameRate();
       
    if ( rate <= 15.0 )
    {
        rate = 15.0;
    }
    else
    {
        rate = 30.0;
    }
   
    aRate = rate;
}

// ---------------------------------------------------------
// CVideoProcessor::MtroAsyncStopComplete
// Called by transcoder after async. stop is complete
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoProcessor::MtroAsyncStopComplete()
{
    PRINT((_L("CVideoProcessor::MtroAsyncStopComplete()")));
}

// ---------------------------------------------------------
// CVideoProcessor::MtroPictureFromTranscoder
// Called by transcoder to return a decoded picture
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoProcessor::MtroPictureFromTranscoder(TTRVideoPicture* aPicture)
{    

    TTimeIntervalMicroSeconds decodedTs = aPicture->iTimestamp;        
    
    PRINT((_L("CVideoProcessor::MtroPictureFromTranscoder(), timestamp %d ms"),
               I64INT( decodedTs.Int64() ) / 1000 ));
               
	if (iThumbnailMode)
	{	    
	    iThumbDecoded = ETrue;
	    
	    // handle thumbnail
	    HandleThumbnailFromTranscoder(aPicture);
	    
        return;
	}                

    // search the decoded frame from list                  
    TInt index;
    for (index = 0; index < iFrameInfoArray.Count(); )
    {      
        PRINT((_L("CVideoProcessor::MtroPictureFromTranscoder(), checking frame with index %d"), index));
          
        TTimeIntervalMicroSeconds ts = 
            (iProcessor->GetVideoTimeInMsFromTicks(iFrameInfoArray[index].iTimeStamp, EFalse)) * 1000;
            
        if (ts < decodedTs && ( (iFrameInfoArray[index].iEncodeFrame == EFalse) ||
                                 (iFrameInfoArray[index].iTranscoderMode == EFullWithIM &&
                                  iFrameInfoArray[index].iModificationApplied == 0) ) )        
        {
            // if there are decode-only or transcoding w/intermediate modification
            // frames in the queue before this one, remove those                
            PRINT((_L("CVideoProcessor::MtroPictureFromTranscoder(), removing frame with timestamp %d ms"),
                I64INT( ts.Int64() ) / 1000 ));
        
            iFrameInfoArray.Remove(index);
            // don't increment index
            continue;
        }
            
        if (ts == decodedTs)    
        {
            PRINT((_L("CVideoProcessor::MtroPictureFromTranscoder(), found decoded frame at index %d"), index));
            break;
        }
            
        index++;
    }

    // If decoded frame is unexpected, i.e. it is not found from book-keeping, 
    // or it is not an intermediate modification frame, return frame here
    // and continue
    if ( index >= iFrameInfoArray.Count() || 
        ( iFrameInfoArray[index].iEncodeFrame == 1 && 
          iFrameInfoArray[index].iTranscoderMode != EFullWithIM ) )
        {
        PRINT((_L("CVideoProcessor::MtroPictureFromTranscoder(), unexpected decoded frame, iTranscoderMode %d"), iTranscoderMode));
        // send picture back to transcoder
        TInt error = KErrNone;
        TRAP( error, iTransCoder->SendPictureToTranscoderL(aPicture) );
        if ( error != KErrNone )
        {
            iMonitor->Error(error);
        }    
        return;
        }

    if (iFrameInfoArray[index].iEncodeFrame == EFalse)
    {    
        // handle decode-only frame
        HandleDecodeOnlyFrameFromTranscoder(aPicture, index);        
        return;
    }

    // check color effect
    TInt colorEffect = TColorEffect2TInt( iProcessor->GetColorEffect() );
    if (colorEffect != 0/*None*/)
    {
        // U,V value for the color toning 
        TInt colorToneU;
        TInt colorToneV;
        iProcessor->GetColorTone((TInt&)colorToneU, (TInt&)colorToneV);
    	// apply color effect
    	ApplySpecialEffect( colorEffect, const_cast<TUint8*>(aPicture->iRawData->Ptr()), colorToneU, colorToneV );
        
    }
    
    if(iFrameInfoArray[index].iTransitionFrame == 1)
    {
        // apply transition to frame
		HandleTransitionFrameFromTranscoder(aPicture, index);
    }
        
    iFrameInfoArray[index].iModificationApplied = ETrue;
    
    // send picture back to transcoder for encoding
    TInt error = KErrNone;
    TRAP( error, iTransCoder->SendPictureToTranscoderL(aPicture) );
    if ( error != KErrNone )
    {
        iMonitor->Error(error);
        return;
    }    
    
    // check if the next frame is waiting to be encoded, set timer if so
    if ( IsNextFrameBeingEncoded() )
    {
        if ( !iTimer->IsPending() )
            iTimer->SetTimer( TTimeIntervalMicroSeconds32( iMaxEncodingDelay ) );
    }
    
}


// ---------------------------------------------------------
// CVideoProcessor::HandleThumbnailFromTranscoder
// Handle thumbnail frame
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoProcessor::HandleThumbnailFromTranscoder(TTRVideoPicture* aPicture)
{

    TInt error = KErrNone;
    
    PRINT((_L("CVideoProcessor::HandleThumbnailFromTranscoder() begin")));   
 
    if (iProcessingComplete)
    {   	    
        // if requested thumbnail has been done already,
        // just release picture and return
        PRINT((_L("CVideoProcessor::HandleThumbnailFromTranscoder(), thumb already finished, returning")));   
        
        TRAP( error, iTransCoder->SendPictureToTranscoderL(aPicture) );
        if ( error != KErrNone )
        {
            iMonitor->Error(error);
            return;
        }
        return;
    }	   

    TInt yuvLength = iVideoWidth*iVideoHeight;
    yuvLength += (yuvLength >> 1);
    // copy to iFrameBuffer	
    Mem::Copy(iFrameBuffer, aPicture->iRawData->Ptr(), yuvLength);
    
    // release picture
    TRAP( error, iTransCoder->SendPictureToTranscoderL(aPicture) );
    if ( error != KErrNone )
    {
        iProcessor->NotifyThumbnailReady(error);
        return;
    }
    
    VDASSERT(iDecodePending, 33);
    VDASSERT(IsActive(), 150);
    
    if (iStatus == KRequestPending)
    {            
    
        PRINT((_L("CVideoProcessor::HandleThumbnailFromTranscoder(), complete request")));   
        // complete request    
        TRequestStatus *status = &iStatus;
        User::RequestComplete(status, KErrNone);
    }    
    
    PRINT((_L("CVideoProcessor::HandleThumbnailFromTranscoder() end")));   
}

// ---------------------------------------------------------
// CVideoProcessor::HandleDecodeOnlyFrameFromTranscoder
// Handle decode-only frame
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoProcessor::HandleDecodeOnlyFrameFromTranscoder(TTRVideoPicture* aPicture, TInt aIndex)
{

    VDASSERT(iFrameInfoArray[aIndex].iTranscoderMode == EDecodeOnly, 43);
        
    if ( iFrameInfoArray[aIndex].iTransitionFrame && 
         iFrameInfoArray[aIndex].iTransitionPosition == EPositionEndOfClip  )
    {
		if ( iEndTransitionColor == EColorTransition )
        {    			
		    // Save decoded frame to file
		    
		    TSize a = iProcessor->GetMovieResolution();
            TInt yuvLength = a.iWidth*a.iHeight;						
			yuvLength += (yuvLength>>1); 
			TPtr8 ptr(0,0);
			TUint8* tmpBuf=0;					
			
			ptr.Set( *aPicture->iRawData );
            tmpBuf = const_cast<TUint8*>(aPicture->iRawData->Ptr());
							
			TInt colorEffect = TColorEffect2TInt( iProcessor->GetColorEffect() );
			if (colorEffect != 0 /*None*/)
			{
			    // U,V value for the color toning
                TInt colorToneU;
                TInt colorToneV;
                iProcessor->GetColorTone((TInt&)colorToneU, (TInt&)colorToneV);
				// apply special effect
				ApplySpecialEffect( colorEffect, tmpBuf, colorToneU, colorToneV ); 
			}
			    				    				    				
            TInt frameDuration = GetFrameDuration(iFrameInfoArray[aIndex].iFrameNumber);

            if (frameDuration <= 0)
            {
                TReal frameRate = iProcessor->GetVideoClipFrameRate();
                VDASSERT(frameRate > 0.0, 107);
                TInt timeScale = iProcessor->GetVideoClipTimeScale();                    
                TInt64 durationMs =  TInt64( ( 1000.0 / frameRate ) + 0.5 );            

                // in ticks
                frameDuration = TInt( ( (TReal)durationMs * (TReal)timeScale / 1000.0 ) + 0.5 ); 
            }                                     

            TInt error = iProcessor->SaveVideoFrameToFile( ptr, frameDuration, iFrameInfoArray[aIndex].iTimeStamp );
			if ( error != KErrNone )
			{
			    PRINT((_L("CVideoProcessor::HandleDecodeOnlyFrameFromTranscoder() - SaveVideoFrameToFile failed")));
				iMonitor->Error(error);
				return;
			}    				
        }
    }       
    
    iFrameInfoArray.Remove(aIndex);
    
    PRINT((_L("CVideoProcessor::HandleDecodeOnlyFrameFromTranscoder() - removed decode only pic, %d items in queue"), 
        iFrameInfoArray.Count()));

    // release picture
    TInt error = KErrNone;
    TRAP( error, iTransCoder->SendPictureToTranscoderL(aPicture) );
    if ( error != KErrNone )
    {
        iMonitor->Error(error);
        return;
    }
    
    if (iStreamEndRead && iFrameInfoArray.Count() == 0 )
    {            
        PRINT((_L("CVideoProcessor::HandleDecodeOnlyFrameFromTranscoder() - stream end read, no frames left")));       
        if (!IsActive())
        {
            SetActive();
            iStatus = KRequestPending;
        }
        iTimer->CancelTimer();
        iProcessingComplete = ETrue;
        // activate object to end processing
        TRequestStatus *status = &iStatus;
        User::RequestComplete(status, KErrNone);
        return;
    }
    
    if (iDecodingSuspended && !iStreamEndRead)
    {
        if (iFrameInfoArray.Count() < iMaxItemsInProcessingQueue && !iDelayedWrite)
        {            
            PRINT((_L("CVideoProcessor::HandleDecodeOnlyFrameFromTranscoder() - Resume decoding")));
            iDecodingSuspended = EFalse;
            // activate object to start decoding
            TRequestStatus *status = &iStatus;
            User::RequestComplete(status, KErrNone);
            return;
        }
    }
    
    // check if the next frame is waiting to be encoded, set timer if so
    if ( IsNextFrameBeingEncoded() )
    {
        if ( !iTimer->IsPending() )
            iTimer->SetTimer( TTimeIntervalMicroSeconds32( iMaxEncodingDelay ) );
    }
                    
    return;
    
}


// ---------------------------------------------------------
// CVideoProcessor::HandleTransitionFrameFromTranscoder
// Handle transition frame
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoProcessor::HandleTransitionFrameFromTranscoder(TTRVideoPicture* aPicture, TInt aIndex)
{

    // apply transition effect in spatial domain (to yuv data in encoder buffer)    
	if ( iFrameInfoArray[aIndex].iTransitionPosition == EPositionStartOfClip && 
	     iStartTransitionColor == EColorTransition )
	{
	    // Do blending transition: wipe / crossfade
	    	
	    TSize a = iProcessor->GetMovieResolution();
        TInt yuvLength = a.iWidth*a.iHeight;
        yuvLength += (yuvLength>>1);
            
	    if (iFrameInfoArray[aIndex].iRepeatFrame == 0) 
	    {		             
            if( !iColorTransitionBuffer )
            {                    
    			iColorTransitionBuffer = (TUint8*)User::Alloc( yuvLength );
    			if (!iColorTransitionBuffer)
    			{
    			    iMonitor->Error(KErrNoMemory);
    			    return;        			    
    			}
            }
            
    		if( !iOrigPreviousYUVBuffer )
    		{        		    
    			iOrigPreviousYUVBuffer = (TUint8*)User::Alloc( yuvLength );
    			if (!iOrigPreviousYUVBuffer)
    			{
    			    iMonitor->Error(KErrNoMemory);
    			    return;        			    
    			}
    		}

    		TPtr8 ptr( iColorTransitionBuffer, 0, yuvLength );
    		
    		if ( iProcessor->GetVideoFrameFromFile( ptr, yuvLength, iFrameDuration, iTimeStamp ) != KErrNone )
    		    //|| iFrameDuration == 0 || iTimeStamp == 0 )
    		{
    			// failure in reading frame data from previous clip
    			// continue using the current frame data        			
    			Mem::Copy( iColorTransitionBuffer, aPicture->iRawData->Ptr(), yuvLength );
    		}
    		else
    		{
    		    // buffer frame from previous clip (read from file)
    			Mem::Copy( iOrigPreviousYUVBuffer, iColorTransitionBuffer, yuvLength );
    			if ( iStartOfClipTransition == (TInt)EVedMiddleTransitionEffectCrossfade )
    			{        				
    				// Target frame is the one read from file, iColorTransitionBuffer
    				ApplyBlendingTransitionEffect( iColorTransitionBuffer, const_cast<TUint8*>(aPicture->iRawData->Ptr()), 
    				                               0 /* repeatFrame */, iFrameInfoArray[aIndex].iTransitionFrameNumber);
    			}
    			else
    			{
    			    // Target frame is the one read from file, iColorTransitionBuffer
    				ApplySlidingTransitionEffect( iColorTransitionBuffer, const_cast<TUint8*>(aPicture->iRawData->Ptr()), (TVedMiddleTransitionEffect)iStartOfClipTransition, 
    				                              0 /* repeatFrame */, iFrameInfoArray[aIndex].iTransitionFrameNumber);
    			}
    			// copy frame from edited buffer to transcoder buffer
    			Mem::Copy( const_cast<TUint8*>(aPicture->iRawData->Ptr()), iColorTransitionBuffer, yuvLength );        			        			
    		}
	    }
	    else
	    {
	        // repeatFrame
	        
	        if ( iStartOfClipTransition == (TInt)EVedMiddleTransitionEffectCrossfade )
			{
				ApplyBlendingTransitionEffect( iOrigPreviousYUVBuffer, const_cast<TUint8*>(aPicture->iRawData->Ptr()), 
				                               1 /* repeatFrame */, iFrameInfoArray[aIndex].iTransitionFrameNumber);
			}
			else
			{
				ApplySlidingTransitionEffect( iOrigPreviousYUVBuffer, const_cast<TUint8*>(aPicture->iRawData->Ptr()), (TVedMiddleTransitionEffect)iStartOfClipTransition, 
				                              1 /* repeatFrame */, iFrameInfoArray[aIndex].iTransitionFrameNumber );
			}
			// copy frame from edited buffer to transcoder buffer
	        Mem::Copy( const_cast<TUint8*>(aPicture->iRawData->Ptr()), iOrigPreviousYUVBuffer, yuvLength );
	    }
	}
	else
	{
		// apply transition effect in spatial domain (to yuv data in encoder buffer)
		
		// Do fading transition
		ApplyFadingTransitionEffect(const_cast<TUint8*>(aPicture->iRawData->Ptr()), iFrameInfoArray[aIndex].iTransitionPosition, iFrameInfoArray[aIndex].iTransitionColor,
		                            iFrameInfoArray[aIndex].iTransitionFrameNumber);
	}		
    
}

// ---------------------------------------------------------
// CVideoProcessor::ProcessThumb
// Starts thumbnail generation
// (other items were commented in a header).
// ---------------------------------------------------------
//
TInt CVideoProcessor::ProcessThumb(MThumbnailObserver* aThumbObserver, TInt aFrameIndex, TInt aStartFrameIndex, TVedTranscodeFactor* aFactor)
{
    TInt error;
//    TInt goodFrame = 0; 
//    TInt frameSkip = 10; 
//    TInt frameNumber = aStartFrameIndex;
    TPtrC8 inputPtr;
    TPtr8 outputPtr(0,0);
    
    iThumbObserver = aThumbObserver;
    iThumbFrameIndex = aFrameIndex;
    iThumbFrameNumber = aStartFrameIndex;
    iFramesToSkip = 0;
    iNumThumbFrameSkips = 0;
    iPreviousTimeStamp = TTimeIntervalMicroSeconds(-1);
    iProcessingComplete = EFalse;
    
    iThumbFramesToWrite  = iProcessor->GetOutputNumberOfFrames() - iThumbFrameNumber;    

    // get transcode factor to determine input stream type    
    TRAP(error, GetTranscodeFactorL(*aFactor));
    if (error != KErrNone)
        return error;
    
    TVedVideoType inType;
    if (aFactor->iStreamType == EVedVideoBitstreamModeH263)
        inType = EVedVideoTypeH263Profile0Level10;
    
    else if (aFactor->iStreamType == EVedVideoBitstreamModeAVC)
    	inType = EVedVideoTypeAVCBaselineProfile;
    
    else
        inType = EVedVideoTypeMPEG4SimpleProfile;
    
    if (aFactor->iStreamType == EVedVideoTypeUnrecognized ||
        aFactor->iStreamType == EVedVideoTypeNoVideo)
        return KErrorCode;
        
    iDecoding = ETrue;
      
    // first frame is now read in iDataBuffer, initialize transcoder
    TRAP(error, CreateAndInitializeTranscoderL(inType, CTRTranscoder::EDecoding))
    if (error != KErrNone)
        return error;
    
    // wait for initialisation to complete => RunL
    
    return KErrNone;
    
}
           
// ---------------------------------------------------------
// CVideoProcessor::ProcessThumb
// Processes a thumbnail frame internally
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoProcessor::ProcessThumb(TBool aFirstFrame)
{

    PRINT((_L("CVideoProcessor::ProcessThumb() - begin()")));

    iThumbDecoded = EFalse;
 
    if (aFirstFrame)
    {    
        // frame read in iDataBuffer, decode
        CCMRMediaBuffer::TBufferType bt = 
            (iDataFormat == EDataH263) ? CCMRMediaBuffer::EVideoH263 : CCMRMediaBuffer::EVideoMPEG4;                        
        TInt index = iThumbFrameNumber - ( iProcessor->GetOutputNumberOfFrames() - 
                                           iProcessor->GetClipNumberOfFrames() );
        TTimeIntervalMicroSeconds ts = 
            TTimeIntervalMicroSeconds(iProcessor->GetVideoTimeInMsFromTicks(iProcessor->VideoFrameTimeStamp(index), ETrue) * TInt64(1000) );
                    
   		// Get the AVC bit stream and add NAL headers
   		if(iDataFormat == EDataAVC)   		
   		{   
   		    TInt error = KErrNone;
   		    
   		    // insert dec.config. record in the beginning of buffer
   		    TRAP( error, InsertDecoderSpecificInfoL() );
   		    if (error != KErrNone)
            {                                
                iProcessor->NotifyThumbnailReady(error);
                return;
            }
   		
   		    PRINT((_L("CVideoProcessor::ProcessThumb() - ProcessAVCBitStream()")));
   		
   		    TPtr8 ptr(iDataBuffer, iCurrentFrameLength, iBufferLength);
            TRAP( error, iAvcEdit->ProcessAVCBitStreamL((TDes8&)(ptr), (TInt&)(iCurrentFrameLength), 
                iProcessor->GetDecoderSpecificInfoSize(), ETrue ) );
                
            if (error != KErrNone)
            {                                
                iProcessor->NotifyThumbnailReady(error);
                return;
            }
            iDataLength = iCurrentFrameLength;
            
   		}
   		
   		// insert VOL header to beginning of buffer
        if (iDataFormat == EDataMPEG4)
        {
   		    TRAPD( error, InsertDecoderSpecificInfoL() );
   		    if (error != KErrNone)
            {                                
                iProcessor->NotifyThumbnailReady(error);
                return;
            }
        }
                
        iMediaBuffer->Set( TPtrC8(iDataBuffer, iBufferLength), 
                                  bt, 
                                  iCurrentFrameLength, 
                                  ETrue, // keyFrame
                                  ts
                                  );
                                  
        iPreviousTimeStamp = ts;

        iIsThumbFrameBeingCopied = ETrue;
        iDecodePending = ETrue;
        if (!IsActive())
        {
            SetActive();
            iStatus = KRequestPending;                                
        }
         
        PRINT((_L("CVideoProcessor::ProcessThumb() - WriteCodedBuffer, thumb frame #%d, timestamp %d ms"),
               iThumbFrameNumber, I64INT( ts.Int64() ) / 1000 ));    
               
        TRAPD( err, iTransCoder->WriteCodedBufferL(iMediaBuffer) );
        if (err != KErrNone)
        {
            // ready
            FinalizeThumb(err);
            return;            
        }
        iThumbFramesToWrite--;
        
        return;
    }
    
    if (iThumbFrameIndex == 0)
    {       
        // ready
        FinalizeThumb(KErrNone);
        return;
    }
 
    iThumbFrameNumber++;

    if (iDataFormat == EDataAVC)
    {
        // In AVC case, we have to stop decoding before the very last
        // frames are decoded, since for some reason the transcoder/decoder
        // does not decode those frames
        
        // get max number of buffered frames according to spec
        TInt buffered = iAvcEdit->GetMaxAVCFrameBuffering(iInputAVCLevel, TSize(iVideoWidth, iVideoHeight));
        
        if (iThumbFrameNumber > iProcessor->GetOutputNumberOfFrames() - 1 - buffered )   
        {            
            // ready                
            FinalizeThumb(KErrNone);
            return;
        }
    }
    
    if (iThumbFrameIndex < 0)
    {        
        if (iFramesToSkip == 0)
        {            
             PRINT((_L("CVideoProcessor::ProcessThumb() frameskip done %d times"), iNumThumbFrameSkips));

             // limit the number of frame skip cycles to 3, because with
		     // near-black or near-white videos we may never find a good thumb.
		     // => max. 30 frames are decoded to get the thumb
             
             // check quality & frame skip cycles
             if ( CheckFrameQuality(iFrameBuffer) || iNumThumbFrameSkips >= 3 )
             {
                 // quality ok or searched long enough, return
                 FinalizeThumb(KErrNone);
                 return;              
             }
             iFramesToSkip = 10;
             iNumThumbFrameSkips++;
        }
        else
            iFramesToSkip--;
        
        // read new frame & decode                           
    }
    
    if (iThumbFrameIndex > 0)
    {            
        if (iThumbFrameNumber > iThumbFrameIndex)        
        {
            // ready                
            FinalizeThumb(KErrNone);
            return;
        }
        // read new frame & decode
    }
    
    if (iIsThumbFrameBeingCopied)
    {
        PRINT((_L("CVideoProcessor::ProcessThumb() - thumb being copied, activate")));
        // Re-activate to wait for MtroReturnCodedBuffer
        iDecodePending = ETrue;
        if (!IsActive())
        {
            SetActive();
            iStatus = KRequestPending;                                
        }
    } 
    else 
    {
        PRINT((_L("CVideoProcessor::ProcessThumb() - read and write new")));
        // send new frame for decoding
        ReadAndWriteThumbFrame();
    }
    
    PRINT((_L("CVideoProcessor::ProcessThumb() - end")));
    
}

// ---------------------------------------------------------
// CVideoProcessor::ReadAndWriteThumbFrame
// Reads a new frame to input queue and sends it to transcoder
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoProcessor::ReadAndWriteThumbFrame()
{

    PRINT((_L("CVideoProcessor::ReadAndWriteThumbFrame() - begin, thumb frames to write %d"),
        iThumbFramesToWrite));

    TInt error = KErrNone;

    if ( iThumbFramesToWrite )
    {
        // read new frame to input queue
        if(iThumbFrameNumber < (iProcessor->GetOutputNumberOfFrames())) // do not read last frame (already read!)
        {   
            CMP4Demux *demux = (CMP4Demux *)iProcessor->GetDemux();
            error = demux->ReadVideoFrames(1);
            if (error != KErrNone)
            {
                FinalizeThumb(error);
                return;
            }
        }
        else
        {
            // no frames left, return            
            FinalizeThumb(KErrNone);
            return;
        }
        
        iDataLength = 0;
        iCurrentFrameLength = 0;
        iDataFormat = EDataUnknown;
        
        if (ReadFrame())
        {
            // frame read in iDataBuffer, decode
            CCMRMediaBuffer::TBufferType bt = 
                (iDataFormat == EDataH263) ? CCMRMediaBuffer::EVideoH263 : CCMRMediaBuffer::EVideoMPEG4;
            
            TInt index = iThumbFrameNumber - ( iProcessor->GetOutputNumberOfFrames() - 
                iProcessor->GetClipNumberOfFrames() );
            
            TTimeIntervalMicroSeconds ts = 
                TTimeIntervalMicroSeconds(iProcessor->GetVideoTimeInMsFromTicks(iProcessor->VideoFrameTimeStamp(index), ETrue) * TInt64(1000) );
                
            if (ts <= iPreviousTimeStamp)
            {            
                // adjust timestamp so that its bigger than ts of previous frame
                TReal frameRate = iProcessor->GetVideoClipFrameRate();
                VDASSERT(frameRate > 0.0, 108);
                TInt64 durationMs =  TInt64( ( 1000.0 / frameRate ) + 0.5 );
                durationMs /= 2;  // add half the duration of one frame
                
                ts = TTimeIntervalMicroSeconds( iPreviousTimeStamp.Int64() + durationMs*1000 );
            }
            
            iPreviousTimeStamp = ts;
            
       		// Get the AVC bit stream and add NAL headers
       		if(iDataFormat == EDataAVC)
       		{
       		    TPtr8 ptr(iDataBuffer, iCurrentFrameLength, iBufferLength);
                TRAPD( error, iAvcEdit->ProcessAVCBitStreamL((TDes8&)(ptr), (TInt&)(iCurrentFrameLength), 
                    iProcessor->GetDecoderSpecificInfoSize(), EFalse ) );
                    
                if (error != KErrNone)
                {
                    FinalizeThumb(error);
                    return;
                }
                iDataLength = iCurrentFrameLength;
       		}                    

            iMediaBuffer->Set( TPtrC8(iDataBuffer, iBufferLength), 
                                      bt, 
                                      iCurrentFrameLength, 
                                      iProcessor->GetVideoFrameType(index),
                                      ts );                                                                            

            iIsThumbFrameBeingCopied = ETrue;
            iDecodePending = ETrue;
            if (!IsActive())
            {
                SetActive();
                iStatus = KRequestPending;                                
            }
            
            PRINT((_L("CVideoProcessor::ProcessThumb() - WriteCodedBuffer, thumb frame #%d, timestamp %d ms"),
               iThumbFrameNumber, I64INT( ts.Int64() ) / 1000 ));    
                   
            TRAPD( err, iTransCoder->WriteCodedBufferL(iMediaBuffer) );
            if (err != KErrNone)
            {
                FinalizeThumb(err);
            }            
            iThumbFramesToWrite--;
            return;        
        }
    } 
    
    else
    {
        if (iDataFormat == EDataAVC)
        {
            PRINT((_L("CVideoProcessor::ReadAndWriteThumbFrame() - all frames written, wait for output")));
            // all necessary frames written to decoder, now wait for output frames            
            iDecodePending = ETrue;
            if (!IsActive())
            {
                SetActive();
                iStatus = KRequestPending;                                
            }            
        } 
        else
        {
            FinalizeThumb(KErrNone);            
        }
    }    
    
    PRINT((_L("CVideoProcessor::ReadAndWriteThumbFrame() - end")));     
}
    
// ---------------------------------------------------------
// CVideoProcessor::FinalizeThumb
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoProcessor::FinalizeThumb(TInt aError)
{       
    iProcessingComplete = ETrue;
    if (iTranscoderStarted) 
    {        
        TRAPD( err, iTransCoder->StopL() );
        if (err != KErrNone) { }
        iTranscoderStarted = EFalse;
    }
    iProcessor->NotifyThumbnailReady(aError);
}
    
// ---------------------------------------------------------
// CVideoProcessor::FetchThumb
// For getting a pointer to YUV thumbnail frame
// (other items were commented in a header).
// ---------------------------------------------------------
//
TInt CVideoProcessor::FetchThumb(TUint8** aYUVDataPtr)
{
    *aYUVDataPtr = iFrameBuffer;    
        
    return KErrNone;
}


// ---------------------------------------------------------
// CVideoProcessor::GetTranscodeFactorL
// Gets the transcode factor from the current clip
// (other items were commented in a header).
// ---------------------------------------------------------
//
TInt CVideoProcessor::GetTranscodeFactorL(TVedTranscodeFactor& aFactor)
{
    // start reading video frames
    CMP4Demux *demux = (CMP4Demux *)iProcessor->GetDemux();
    TInt error = demux->ReadVideoFrames(1);

    if (error != KErrNone)
        User::Leave(error);

    // seek to and decode first frame
    if (!ReadFrame())
        User::Leave(KErrCorrupt);
    
    // Get pointer to the frame data
    TPtr8 inputPtr(0,0);
    if ( iDataFormat == EDataH263 )
        inputPtr.Set(iDataBuffer, iCurrentFrameLength + KH263StartCodeLength, iCurrentFrameLength + KH263StartCodeLength);
    else
        inputPtr.Set(iDecoderSpecificInfo->Des());
    
	if(iDataFormat == EDataAVC)
	{
		// @@ HARI AVC harcode for now    
	    // Set transcode factors
	    aFactor.iTRes = 30;
	    aFactor.iStreamType = EVedVideoBitstreamModeAVC;
	} 
	else
	{
        // Get the VOL header from the frame data
        CVedVolReader* reader = CVedVolReader::NewL();
        CleanupStack::PushL(reader);
        reader->ParseVolHeaderL((TDesC8&) inputPtr);
        
        // Set transcode factors
        aFactor.iTRes = reader->TimeIncrementResolution();
        aFactor.iStreamType = reader->BitstreamMode();
        
        CleanupStack::PopAndDestroy(reader);
	}
    
    return KErrNone;    
}



// ---------------------------------------------------------
// CVideoProcessor::CheckFrameQuality
// Checks if a frame has "good" or "legible" quality
// (other items were commented in a header).
// ---------------------------------------------------------
//
TInt CVideoProcessor::CheckFrameQuality(TUint8* aYUVDataPtr)
{
    TInt i;
    TInt minValue = 255;
    TInt maxValue = 0;
    TInt goodFrame = 1;
    TInt runningSum=0;
    TInt averageValue=0;
    TInt pixelSkips = 4;
    TInt numberOfSamples=0;
    TInt minMaxDeltaThreshold = 20; 
    TInt extremeRegionThreshold = 20; 
    TInt ySize = iVideoWidth*iVideoHeight; 
    
    // gather image statistics
    for(i=0, numberOfSamples=0; i<ySize; i+=pixelSkips, aYUVDataPtr+=pixelSkips, numberOfSamples++)
    {
        runningSum += *aYUVDataPtr;
        if(*aYUVDataPtr > maxValue)
            maxValue = *aYUVDataPtr;
        if(*aYUVDataPtr < minValue)
            minValue = *aYUVDataPtr;
    }
    VDASSERT(numberOfSamples,10);
    averageValue = runningSum/numberOfSamples;
    
    // make decision based statistics
    if((maxValue - minValue) < minMaxDeltaThreshold)
        goodFrame = 0;
    else 
    {
        if(averageValue < (minValue + extremeRegionThreshold) || 
            averageValue > (maxValue - extremeRegionThreshold))
            goodFrame = 0;
    }
    return goodFrame;
}


// ---------------------------------------------------------
// CVideoProcessor::ReadFrame
// Reads a frame from input queue to internal buffer
// (other items were commented in a header).
// ---------------------------------------------------------
//				
TInt CVideoProcessor::ReadFrame()
{

	TUint doNow;		
	
	if (iProcessor->GetCurrentClipVideoType() == EVedVideoTypeAVCBaselineProfile)
        iDataFormat = EDataAVC;  
	
	// Determine data format if needed
	if ( iDataFormat == EDataUnknown )
	{
		// We'll need four bytes of data
		while ( iDataLength < 4 )
		{
			// Get a block (we can't have one as we go here only at the stream
			// start)
			VDASSERT(!iBlock,11);
			while ( (!iBlock) || (iBlock->Length() == 0) )
			{
				if ( iBlock )
					iQueue->ReturnBlock(iBlock);
				if ( (iBlock = iQueue->ReadBlock()) == NULL )
					return EFalse;
			}
			iBlockPos = 0;
			
			// get timestamp for first frame
			if ( iTiming == ETimeStamp )
			{
				VDASSERT( (TUint)iBlock->Length() >= 8,12 );		
				iBlockPos += 4;
			}
			
			// Copy data from block to buffer:
			doNow = 4 - iDataLength;
			if ( doNow > (TUint) iBlock->Length() - iBlockPos )
				doNow = iBlock->Length() - iBlockPos;
			Mem::Copy(iDataBuffer+iDataLength, iBlock->Ptr()+iBlockPos, doNow);
			iDataLength += doNow;
			iBlockPos += doNow;
			
			// Return the block if it doesn't have any more data
			if ( ((TInt)iBlockPos == iBlock->Length()) )
			{
				iQueue->ReturnBlock(iBlock);
				iBlock = 0;
			}
		}          
    
		// OK, we have 4 bytes of data. Check if the buffer starts with a
		// H.263 PSC:
		if ( (iDataBuffer[0] == 0) && (iDataBuffer[1] == 0) &&
			((iDataBuffer[2] & 0xfc) == 0x80) )
		{
			// Yes, this is a H.263 stream
			iDataFormat = EDataH263;
		}
		
		// It should be MPEG-4, check if it starts with MPEG 4 Visual
		// Object Sequence start code, Visual Object start code, Video
		// Object start code, or Video Object Layer start code
		else if ( ((iDataBuffer[0] == 0) && (iDataBuffer[1] == 0) && (iDataBuffer[2] == 1) && (iDataBuffer[3] == 0xb0)) ||
			((iDataBuffer[0] == 0) && (iDataBuffer[1] == 0) && (iDataBuffer[2] == 1) && (iDataBuffer[3] == 0xb6)) ||
			((iDataBuffer[0] == 0) && (iDataBuffer[1] == 0) && (iDataBuffer[2] == 1) && (iDataBuffer[3] == 0xb3)) ||
			((iDataBuffer[0] == 0) && (iDataBuffer[1] == 0) && (iDataBuffer[2] == 1) && (iDataBuffer[3] == 0xb5)) ||
			((iDataBuffer[0] == 0) && (iDataBuffer[1] == 0) && (iDataBuffer[2] == 1) && ((iDataBuffer[3] >> 5) == 0)) ||
			((iDataBuffer[0] == 0) && (iDataBuffer[1] == 0) && (iDataBuffer[2] == 1) && ((iDataBuffer[3] >> 4) == 2)))
		{
			iDataFormat = EDataMPEG4;                
		}
		else
		{
            PRINT((_L("CVideoProcessor::ReadFrame() - no PSC or MPEG-4 start code in the start of the buffer")));
            if (iMonitor)
			    iMonitor->Error(KErrCorrupt);
			return EFalse;
		}
	}
	
	// Determine the start code length
	TUint startCodeLength = 0;
	switch (iDataFormat)
	{
	case EDataH263:
		startCodeLength = KH263StartCodeLength;
		break;
	case EDataMPEG4:
		startCodeLength = KMPEG4StartCodeLength ;
		break;		
    case EDataAVC:
		break;

	default:
		User::Panic(_L("CVideoPlayer"), EInternalAssertionFailure);
	}

	// If the stream has ended, we have no blocks and no data for even a
	// picture start code, we can't get a frame
	if( iDataFormat == EDataH263 )
	{
		if ( iStreamEnd && (iQueue->NumDataBlocks() == 0) &&
			(iCurrentFrameLength <= startCodeLength) &&	(iDataLength <= startCodeLength) )
			return EFalse;
	}
	else
	{
		if ( iStreamEnd && (iQueue->NumDataBlocks() == 0) &&
			(iCurrentFrameLength <= startCodeLength) &&	(iDataLength < startCodeLength) )
			return EFalse;
	}
		
    switch(iDataFormat)
    {
        case EDataH263:
            return ReadH263Frame();
//            break;
            
        case EDataMPEG4:
            return ReadMPEG4Frame();
//            break;
            
        case EDataAVC:
            return ReadAVCFrame();
//            break;
            
        default:
            User::Panic(_L("CVideoProcessor"), EInternalAssertionFailure);        
        
    }
    
    return ETrue;
}


// ---------------------------------------------------------
// CVideoProcessor::ReadH263Frame
// Reads a H.263 frame from input queue to internal buffer
// (other items were commented in a header).
// ---------------------------------------------------------
//				
TBool CVideoProcessor::ReadH263Frame()
{
     
    VDASSERT( iDataFormat == EDataH263, 17 );    
       
    TInt offset = 0;    
    if ( iTiming == ETimeStamp )
	    offset = 4;		
    
    // There should be one PSC at the buffer start, 
    // and no other PSCs up to iDataLength    	
	if ( (iDataLength >= KH263StartCodeLength) && 
        ((iDataBuffer[0] != 0) || (iDataBuffer[1] != 0) || ((iDataBuffer[2] & 0xfc) != 0x80)) )
    {
		PRINT((_L("CVideoProcessor::ReadH263Frame() - no PSC in the start of the buffer")))
		if (iMonitor)
            iMonitor->Error(KErrCorrupt);
		return EFalse;
    }
    
	if (iCurrentFrameLength < KH263StartCodeLength )
		iCurrentFrameLength = KH263StartCodeLength;

    TBool gotPSC = EFalse;
	while (!gotPSC)
	{
		// If we don't have a block at the moment, get one and check if it
		// has a new PSC
		while (!iBlock)
		{
			if ((iBlock = iQueue->ReadBlock()) == NULL)
			{
				if (!iStreamEnd && !iProcessor->IsThumbnailInProgress())
					return EFalse;
				
				// No more blocks in the stream. If we have more data than
				// just a PSC, use the remaining as the last frame. We'll
				// append an End Of Stream (EOS) codeword to the stream end
				// to keep the decoder happy
				if (iDataLength <= 3)
					return EFalse;
				iCurrentFrameLength = iDataLength;
				if (iBufferLength < (iDataLength+3))
				{
					iBufferLength += 3;						                       
                    TUint8* tmp = (TUint8*) User::ReAlloc(iDataBuffer, iBufferLength);
					if ( !tmp  )
					{
					    if (iMonitor)
						    iMonitor->Error(KErrNoMemory);							
						return EFalse;
					}
                    iDataBuffer = tmp;
				}
				iDataBuffer[iCurrentFrameLength] = 0;
				iDataBuffer[iCurrentFrameLength+1] = 0;
				iDataBuffer[iCurrentFrameLength+2] = 0xfc;
				iDataLength += 3;
				return ETrue;
			}
							
			iBlockPos = 0;
			// Return empty blocks immediately
			if ( iBlock->Length() == 0 )
			{
				iQueue->ReturnBlock(iBlock);
				iBlock = 0;
			}                
		}
		
		// If we are at the start of a block, check if it begins with a PSC
		if ( iBlockPos == 0 )
		{                
			if ( (iBlock->Length() > 2 + offset) &&
				( ((*iBlock)[0+offset] == 0) && ((*iBlock)[1+offset] == 0) && (((*iBlock)[2+offset] & 0xfc) == 0x80) ) )
			{
				gotPSC = ETrue;
				iCurrentFrameLength = iDataLength;  // timestamps not copied to buffer
				
				if (iTiming == ETimeStamp)
				{						
					iBlockPos += offset;
				}
			}
            else
            {
                PRINT((_L("CVideoProcessor::ReadH263Frame() - no PSC in the start of the buffer")))
                if (iMonitor)
                    iMonitor->Error( KErrCorrupt );
                return EFalse;
            }
		}
		
		// If we still have data in our current block, copy it to the buffer
		// Make sure we have enough space
		TUint copyBytes = iBlock->Length() - iBlockPos;
		if (copyBytes)
		{
			while (iBufferLength < (iDataLength + copyBytes))
			{
				// New size is 3/2ths of the old size, rounded up to the next
				// full kilobyte
				TUint newSize = (3 * iBufferLength) / 2;
				newSize = (newSize + 1023) & (~1023);
				
                TUint8* tmp = (TUint8*) User::ReAlloc(iDataBuffer, newSize);
				if (!tmp)
				{
				    if (iMonitor)
					    iMonitor->Error(KErrNoMemory);
					return EFalse;
				}
                iDataBuffer = tmp;
				iBufferLength = newSize;
			}
			Mem::Copy(&iDataBuffer[iDataLength], iBlock->Ptr() + iBlockPos,
				copyBytes);
			iBlockPos += copyBytes;
			iDataLength += copyBytes;
		}
		
		// OK, block used, throw it away
		VDASSERT(iBlock->Length() == (TInt)iBlockPos,16);
		iQueue->ReturnBlock(iBlock);
		iBlock = 0;
	}
	
	return ETrue;	
}
		


// ---------------------------------------------------------
// CVideoProcessor::ReadMPEG4Frame
// Reads a MPEG-4 frame from input queue to internal buffer
// (other items were commented in a header).
// ---------------------------------------------------------
//				
TBool CVideoProcessor::ReadMPEG4Frame()
{
    
    VDASSERT( (iDataFormat == EDataMPEG4 && iTiming == ETimeStamp), 17 );    
		
    // The following code assumes that there is one complete video frame
    // in each input block. This is true for 3GP input streams.     
	
    // get a new block if we don't have one
    while (!iBlock)
    {
        if ((iBlock = iQueue->ReadBlock()) == NULL)        
            return EFalse;        
        
        iBlockPos = 0;
        // Return empty blocks immediately
        if (iBlock->Length() == 0)
        {
            iQueue->ReturnBlock(iBlock);
            iBlock = 0;
        }
    }    

    // If we are at the start of a block, save timestamp
    if (iBlockPos == 0)
    {
        //TUint* p = (TUint*)iBlock->Ptr();
        //AI:         iRenderFrameTime = TInt64( (TUint)((*p)*1000) );
        iBlockPos += 4; // skip timestamp
    }
    
    if (iFirstRead)
    {
        // allocate buffer for header        
        VDASSERT(!iDecoderSpecificInfo, 160);        
        iDecoderSpecificInfo = (HBufC8*) HBufC8::New(iProcessor->GetDecoderSpecificInfoSize());
        if (!iDecoderSpecificInfo)
        {
            iMonitor->Error(KErrNoMemory);
            return EFalse;
        }
        
        TPtr8 ptr(iDecoderSpecificInfo->Des());
        
        // first copy already read bytes from iDataBuffer
        ptr.Copy(iDataBuffer, iDataLength);
        
        TInt copyNow = iProcessor->GetDecoderSpecificInfoSize() - iDataLength;
        iDataLength = 0;
        
        // then copy the rest from input buffer
        ptr.Append(iBlock->Ptr() + iBlockPos, copyNow);
        iBlockPos += copyNow;
        iDecoderSpecificInfoSent = EFalse;
            
        iFirstRead = EFalse;
    
    }
	
    TUint copyBytes = iBlock->Length() - iBlockPos;
    if (copyBytes)
    {
        // Make sure we have enough space
        //   +4 is for inserting a start code at the end of the frame 
        while (iBufferLength < (iDataLength + copyBytes + 4))
        {
            // New size is 3/2ths of the old size, rounded up to the next
            // full kilobyte
            TUint newSize = (3 * iBufferLength) / 2;
            newSize = (newSize + 1023) & (~1023);				
            TUint8* tmp = (TUint8*) User::ReAlloc(iDataBuffer, newSize);
            if (!tmp)
            {
                if (iMonitor)
                    iMonitor->Error(KErrNoMemory);
                return EFalse;
            }
            iDataBuffer = tmp;
            iBufferLength = newSize;
        }
        Mem::Copy(&iDataBuffer[iDataLength], iBlock->Ptr() + iBlockPos,
            copyBytes);
        iBlockPos += copyBytes;
        iDataLength += copyBytes;        
    }

    // OK, block used, throw it away
    VDASSERT((iBlock->Length() == (TInt)iBlockPos),18);
    iQueue->ReturnBlock(iBlock);
    iBlock = 0;
    
    // check for VOS end code
    if ( (iDataBuffer[0] == 0 ) && (iDataBuffer[1] == 0 ) && 
        (iDataBuffer[2] == 0x01) && (iDataBuffer[3] == 0xb1) )
        return EFalse;
    
    // insert VOP start code at the end, the decoder needs it
    iDataBuffer[iDataLength++] = 0;
    iDataBuffer[iDataLength++] = 0;
    iDataBuffer[iDataLength++] = 0x01;
    iDataBuffer[iDataLength++] = 0xb6;
    iCurrentFrameLength = iDataLength;
    
    // we have a complete frame
    return ETrue;
    
}


// ---------------------------------------------------------
// CVideoProcessor::ReadAVCFrame
// Reads an AVC frame from input queue to internal buffer
// (other items were commented in a header).
// ---------------------------------------------------------
//				
TBool CVideoProcessor::ReadAVCFrame()
{

    VDASSERT( iDataFormat == EDataAVC, 17 );    

    // get a new block if we don't have one
    while (!iBlock)
    {
        if ((iBlock = iQueue->ReadBlock()) == NULL)        
            return EFalse;        
        
        iBlockPos = 0;
        // Return empty blocks immediately
        if (iBlock->Length() == 0)
        {
            iQueue->ReturnBlock(iBlock);
            iBlock = 0;
        }
    }    
    
    // skip 4 bytes for the timestamp
    TInt skip = 4;
//    TInt numSPS = 0;
//    TInt numPPS = 0;        

    // set this to point to the start of frame length field
    TUint8* frameLenPtr = const_cast<TUint8*>(iBlock->Ptr()) + skip;
    // how much space needed for frame data 
    TInt frameLen = 0;                

    TInt totalFrameLen = iBlock->Length() - skip;
    
    if (iFirstRead)
    {
        TInt index = skip + 4;     // Skip timestamp + version etc.
        TUint8* temp = const_cast<TUint8*>(iBlock->Ptr());                    
        
        // get no. bytes used for length
        iFrameLengthBytes = ( temp[index] & 0x3 ) + 1;

        // save DCR
        VDASSERT(!iDecoderSpecificInfo, 160);
        
        iDecoderSpecificInfo = (HBufC8*) HBufC8::New(iProcessor->GetDecoderSpecificInfoSize());
        if (!iDecoderSpecificInfo)
        {
            iMonitor->Error(KErrNoMemory);
            return EFalse;
        }
        
        TPtr8 ptr(iDecoderSpecificInfo->Des());
        ptr.Copy(iBlock->Ptr() + skip, iProcessor->GetDecoderSpecificInfoSize());
        iDecoderSpecificInfoSent = EFalse;
        
        // advance pointer over info to point to length field
        frameLenPtr += iProcessor->GetDecoderSpecificInfoSize();
        
        // add to frame len. since it is used to calculate the minimum buffer size
        //frameLen += iProcessor->GetDecoderSpecificInfoSize();
        
        totalFrameLen -= iProcessor->GetDecoderSpecificInfoSize();
        skip += iProcessor->GetDecoderSpecificInfoSize();
        
        iFirstRead = EFalse;
    }
       
    

    TInt numSliceNalUnits = 0;
    while (frameLen < totalFrameLen)
    {                
        TInt nalLen = 0;
        switch (iFrameLengthBytes)
        {
            case 1:
                nalLen = frameLenPtr[0] + 1;  // +1 for length field
                break;                
                
            case 2:            
                nalLen = (frameLenPtr[0] << 8) + frameLenPtr[1] + 2; // +2 for length field
                break;

            case 3:
                nalLen = (frameLenPtr[0] << 16) + (frameLenPtr[1] << 8) +
                          frameLenPtr[2] + 3; // +3 for length field
                break;
                
            case 4:
                nalLen = (frameLenPtr[0] << 24) + (frameLenPtr[1] << 16) + 
                         (frameLenPtr[2] << 8) + frameLenPtr[3] + 4;  // +4 for length field
                break;
                
            default:
                if (iMonitor)
                    iMonitor->Error(KErrCorrupt);
                return EFalse;
        }
        frameLenPtr += nalLen;
        frameLen += nalLen;
        numSliceNalUnits++;
    }
    
    if ( iFrameLengthBytes != 4 )
        frameLen += numSliceNalUnits * ( (iFrameLengthBytes == 1) ? 3 : 2 );                     

    // reserve space for alignment
    TInt addBytes = (frameLen % 4 != 0) * ( 4 - (frameLen % 4) );
    
    // reserve space for slice NAL unit offset and size fields
    addBytes += (numSliceNalUnits * 8);
    
    // reserve space for number of NAL units (4)                
    addBytes += 4;

    // Make sure we have enough space
    while (iBufferLength < (iDataLength + frameLen + addBytes))
    {
        // New size is 3/2ths of the old size, rounded up to the next
        // full kilobyte
        TUint newSize = (3 * iBufferLength)  / 2;
        newSize = (newSize + 1023) & (~1023);				
        TUint8* tmp = (TUint8*) User::ReAlloc(iDataBuffer, newSize);
        if (!tmp)
        {
            iMonitor->Error(KErrNoMemory);
            return EFalse;
        }
        
        iDataBuffer = tmp;
        iBufferLength = newSize;
    }                

    iBlockPos += skip;
    
    if (iFrameLengthBytes == 4)
    {
        // just copy directly, no need to change length field
        Mem::Copy(&iDataBuffer[iDataLength], iBlock->Ptr() + skip, frameLen);
        iDataLength += frameLen;
        iBlockPos += frameLen;
    } 
    else
    {
        // have to change length field for each NAL
        TUint8* srcPtr = const_cast<TUint8*>(iBlock->Ptr()) + skip;
        while (numSliceNalUnits--)
        {
            // read length
            TInt nalLen = 0;
            switch (iFrameLengthBytes)
            {
                case 1:
                    nalLen = srcPtr[0];
                    srcPtr += 1;  // skip length field
                    iBlockPos += 1;
                    break;
                    
                case 2:            
                    nalLen = (srcPtr[0] << 8) + srcPtr[1];
                    srcPtr += 2;  // skip length field  
                    iBlockPos += 2;
                    break;
                    
                case 3:
                    nalLen = (srcPtr[0] << 16) + (srcPtr[1] << 8) + srcPtr[2];
                    srcPtr += 3;  // skip length field  
                    iBlockPos += 3;
                    break;
                    
                default:
                    if (iMonitor)
                        iMonitor->Error(KErrCorrupt);
                    return EFalse;
            }
                    
            // code length with 4 bytes
            iDataBuffer[iDataLength] =     TUint8((nalLen >> 24) & 0xff);
            iDataBuffer[iDataLength + 1] = TUint8((nalLen >> 16) & 0xff);
            iDataBuffer[iDataLength + 2] = TUint8((nalLen >> 8) & 0xff);
            iDataBuffer[iDataLength + 3] = TUint8(nalLen & 0xff);
            iDataLength += 4;
            // copy NAL data
            Mem::Copy(&iDataBuffer[iDataLength], srcPtr, nalLen);
            iDataLength += nalLen;
            srcPtr += nalLen;
            iBlockPos += nalLen;
        }
    }               

    // OK, block used, throw it away
    VDASSERT((iBlock->Length() == (TInt)iBlockPos),18);
    iQueue->ReturnBlock(iBlock);
    iBlock = 0;
            
    iCurrentFrameLength = iDataLength;
    
    // we have a complete frame
    return ETrue;

}
			
			

// ---------------------------------------------------------
// CVideoProcessor::DetermineClipTransitionParameters
// Sets transition frame parameters
// (other items were commented in a header).
// ---------------------------------------------------------
//	
TInt CVideoProcessor::DetermineClipTransitionParameters(TInt& aTransitionEffect,
                                                        TInt& aStartOfClipTransition,
													    TInt& aEndOfClipTransition,
													    TTransitionColor& aStartTransitionColor,
													    TTransitionColor& aEndTransitionColor)
{
	TInt error=KErrNone;
	// find if transition effect is to be applied 
	TInt numberOfVideoClips = iProcessor->GetNumberOfVideoClips();
	TInt videoClipNumber = iProcessor->GetVideoClipNumber(); 
	TVedStartTransitionEffect startTransitionEffect = iProcessor->GetStartTransitionEffect();
	TVedEndTransitionEffect endTransitionEffect = iProcessor->GetEndTransitionEffect();
	TVedMiddleTransitionEffect middleTransitionEffect = iProcessor->GetMiddleTransitionEffect();
	TVedMiddleTransitionEffect previousMiddleTransitionEffect = iProcessor->GetPreviousMiddleTransitionEffect();

	// is transition effect to be applied anywhere in the movie?
	if(startTransitionEffect==EVedStartTransitionEffectNone && 
		middleTransitionEffect==EVedMiddleTransitionEffectNone &&
		endTransitionEffect==EVedEndTransitionEffectNone &&
		previousMiddleTransitionEffect==EVedMiddleTransitionEffectNone)
		aTransitionEffect=0;
	else
		aTransitionEffect=1;
	// where is the transition effect to be applied - beginning, end or both?
	if(aTransitionEffect)
	{
		// if first video clip
		if(videoClipNumber==0)		
		{
			switch(startTransitionEffect)
			{
			default:
			case EVedStartTransitionEffectNone:
			case EVedStartTransitionEffectLast:
				aStartOfClipTransition=0;
				aStartTransitionColor = EColorNone;
				break;
			case EVedStartTransitionEffectFadeFromBlack:
				aStartOfClipTransition=1;
				aStartTransitionColor = EColorBlack;
				break;
			case EVedStartTransitionEffectFadeFromWhite:
				aStartOfClipTransition=1;
				aStartTransitionColor = EColorWhite;
				break;
			}
			// do we need transition at the end of this clip?
			if(videoClipNumber==numberOfVideoClips-1)	// last clip?
			{
				switch(endTransitionEffect)
				{
				default:
				case EVedEndTransitionEffectNone:
				case EVedEndTransitionEffectLast:
					aEndOfClipTransition=0;
					aEndTransitionColor = EColorNone;
					break;
				case EVedEndTransitionEffectFadeToBlack:
					aEndOfClipTransition=1;
					aEndTransitionColor = EColorBlack;
					break;
				case EVedEndTransitionEffectFadeToWhite:
					aEndOfClipTransition=1;
					aEndTransitionColor = EColorWhite;
					break;
				}
			}
			else	// middle clip
			{
				switch(middleTransitionEffect)
				{
				default:
				case EVedMiddleTransitionEffectNone:
				case EVedMiddleTransitionEffectLast:
					aEndOfClipTransition=0;
					aEndTransitionColor = EColorNone;
					break;
				case EVedMiddleTransitionEffectDipToBlack:
					aEndOfClipTransition=1;
					aEndTransitionColor = EColorBlack;
					break;
				case EVedMiddleTransitionEffectDipToWhite:
					aEndOfClipTransition=1;
					aEndTransitionColor = EColorWhite;
					break;
					//change
				case EVedMiddleTransitionEffectCrossfade:
					aEndOfClipTransition=TInt(EVedMiddleTransitionEffectCrossfade);
					aEndTransitionColor = EColorTransition;
					break;
				case EVedMiddleTransitionEffectWipeLeftToRight:
					aEndOfClipTransition=TInt(EVedMiddleTransitionEffectWipeLeftToRight);
					aEndTransitionColor = EColorTransition;
					break;
				case EVedMiddleTransitionEffectWipeRightToLeft:
					aEndOfClipTransition=TInt(EVedMiddleTransitionEffectWipeRightToLeft);
					aEndTransitionColor = EColorTransition;
					break;
				case EVedMiddleTransitionEffectWipeTopToBottom:
					aEndOfClipTransition=TInt(EVedMiddleTransitionEffectWipeTopToBottom);
					aEndTransitionColor = EColorTransition;
					break;
				case EVedMiddleTransitionEffectWipeBottomToTop:
					aEndOfClipTransition=TInt(EVedMiddleTransitionEffectWipeBottomToTop);
					aEndTransitionColor = EColorTransition;
					break;
				}
			}
		}
		// else its the middle or last clip
		else	
		{
			// do we need transition at the beginning of this clip?
			switch(previousMiddleTransitionEffect)
			{
			default:
			case EVedMiddleTransitionEffectNone:
			case EVedMiddleTransitionEffectLast:
				aStartOfClipTransition=0;
				aStartTransitionColor = EColorNone;
				break;
			case EVedMiddleTransitionEffectDipToBlack:
				aStartOfClipTransition=1;
				aStartTransitionColor = EColorBlack;
				break;
			case EVedMiddleTransitionEffectDipToWhite:
				aStartOfClipTransition=1;
				aStartTransitionColor = EColorWhite;
				break;
				
			case EVedMiddleTransitionEffectCrossfade:
				aStartOfClipTransition=TInt(EVedMiddleTransitionEffectCrossfade);
				aStartTransitionColor = EColorTransition;
				break;
			case EVedMiddleTransitionEffectWipeLeftToRight:
				aStartOfClipTransition=TInt(EVedMiddleTransitionEffectWipeLeftToRight);
				aStartTransitionColor = EColorTransition;
				break;
			case EVedMiddleTransitionEffectWipeRightToLeft:
				aStartOfClipTransition=TInt(EVedMiddleTransitionEffectWipeRightToLeft);
				aStartTransitionColor = EColorTransition;
				break;
			case EVedMiddleTransitionEffectWipeTopToBottom:
				aStartOfClipTransition=TInt(EVedMiddleTransitionEffectWipeTopToBottom);
				aStartTransitionColor = EColorTransition;
				break;
			case EVedMiddleTransitionEffectWipeBottomToTop:
				aStartOfClipTransition=TInt(EVedMiddleTransitionEffectWipeBottomToTop);
				aStartTransitionColor = EColorTransition;
				break;
			}
			// do we need transition at the end of this clip?
			if(videoClipNumber==numberOfVideoClips-1)	// last clip?
			{
				switch(endTransitionEffect)
				{
				default:
				case EVedEndTransitionEffectNone:
				case EVedEndTransitionEffectLast:
					aEndOfClipTransition=0;
					aEndTransitionColor = EColorNone;
					break;
				case EVedEndTransitionEffectFadeToBlack:
					aEndOfClipTransition=1;
					aEndTransitionColor = EColorBlack;
					break;
				case EVedEndTransitionEffectFadeToWhite:
					aEndOfClipTransition=1;
					aEndTransitionColor = EColorWhite;
					break;
				}
			}
			else	// middle clip
			{
				switch(middleTransitionEffect)
				{
				default:
				case EVedMiddleTransitionEffectNone:
				case EVedMiddleTransitionEffectLast:
					aEndOfClipTransition=0;
					aEndTransitionColor = EColorNone;
					break;
				case EVedMiddleTransitionEffectDipToBlack:
					aEndOfClipTransition=1;
					aEndTransitionColor = EColorBlack;
					break;
				case EVedMiddleTransitionEffectDipToWhite:
					aEndOfClipTransition=1;
					aEndTransitionColor = EColorWhite;
					break;
					//change
				case EVedMiddleTransitionEffectCrossfade:
					aEndOfClipTransition=TInt(EVedMiddleTransitionEffectCrossfade);
					aEndTransitionColor = EColorTransition;
					break;
				case EVedMiddleTransitionEffectWipeLeftToRight:
					aEndOfClipTransition=TInt(EVedMiddleTransitionEffectWipeLeftToRight);
					aEndTransitionColor = EColorTransition;
					break;
				case EVedMiddleTransitionEffectWipeRightToLeft:
					aEndOfClipTransition=TInt(EVedMiddleTransitionEffectWipeRightToLeft);
					aEndTransitionColor = EColorTransition;
					break;
				case EVedMiddleTransitionEffectWipeTopToBottom:
					aEndOfClipTransition=TInt(EVedMiddleTransitionEffectWipeTopToBottom);
					aEndTransitionColor = EColorTransition;
					break;
				case EVedMiddleTransitionEffectWipeBottomToTop:
					aEndOfClipTransition=TInt(EVedMiddleTransitionEffectWipeBottomToTop);
					aEndTransitionColor = EColorTransition;
					break;
				}
			}
		}
	}
	return error;
}


// ---------------------------------------------------------
// CVideoProcessor::GetNumberOfTransitionFrames
// Calculate the number of transition frames
// (other items were commented in a header).
// ---------------------------------------------------------
//	
void CVideoProcessor::GetNumberOfTransitionFrames(TTimeIntervalMicroSeconds aStartCutTime, 
                                                  TTimeIntervalMicroSeconds aEndCutTime)
{

    TInt startFrameIndex = iProcessor->GetVideoFrameIndex(aStartCutTime);
    // the following is because binary search gives us frame with timestamp < startCutTime
    // this frame would be out of range for  movie
    if(startFrameIndex > 0 && startFrameIndex < iNumberOfFrames-1)
        startFrameIndex++;
    TInt endFrameIndex = iProcessor->GetVideoFrameIndex(aEndCutTime);
    // adjust frame indices for cut video clip
    startFrameIndex -= iProcessor->GetStartFrameIndex();
    endFrameIndex -= iProcessor->GetStartFrameIndex();
       if(startFrameIndex < 0)
        startFrameIndex = 0;
    if(endFrameIndex < 0)
        endFrameIndex = 0;
	if(endFrameIndex<startFrameIndex)
		endFrameIndex = startFrameIndex;

    // determine the total number of included frames in the clip
    iNumberOfIncludedFrames = endFrameIndex-startFrameIndex+1;
                
    // make sure there are enough frames to apply transition
    // for transition at both ends
    if(iStartOfClipTransition && iEndOfClipTransition)
    {

		if ( iStartTransitionColor == EColorTransition &&
			iEndTransitionColor == EColorTransition )
		{
			iStartNumberOfTransitionFrames >>= 1;
						
			// if there are not enough frames saved from previous
			// clip, the transition must be shortened accordingly
			if (iProcessor->GetNumberOfSavedFrames() < iStartNumberOfTransitionFrames)
                iStartNumberOfTransitionFrames = iProcessor->GetNumberOfSavedFrames();			
			
			iEndNumberOfTransitionFrames >>= 1;
			if ( iNumberOfIncludedFrames < (iStartNumberOfTransitionFrames + iEndNumberOfTransitionFrames) )
			{
				iStartNumberOfTransitionFrames = iNumberOfIncludedFrames >> 1;
				iEndNumberOfTransitionFrames = iNumberOfIncludedFrames - iStartNumberOfTransitionFrames;
			}
		}
		else
		{
			if ( iStartTransitionColor == EColorTransition )
			{
				iStartNumberOfTransitionFrames >>= 1;
				
				// if there are not enough frames saved from previous
			    // clip, the transition must be shortened accordingly
			    if (iProcessor->GetNumberOfSavedFrames() < iStartNumberOfTransitionFrames)
                    iStartNumberOfTransitionFrames = iProcessor->GetNumberOfSavedFrames();			
			}
			
			if ( iEndTransitionColor == EColorTransition )
				iEndNumberOfTransitionFrames >>= 1;
			
			if ( iNumberOfIncludedFrames < (iStartNumberOfTransitionFrames + iEndNumberOfTransitionFrames) )
			{
				if ( iStartTransitionColor == EColorTransition )
				{
					if ( ( iNumberOfIncludedFrames >> 1 ) > iStartNumberOfTransitionFrames )
					{
						iEndNumberOfTransitionFrames = iNumberOfIncludedFrames - iStartNumberOfTransitionFrames;
					}
					else
					{
						iStartNumberOfTransitionFrames = iNumberOfIncludedFrames >> 1;
						iEndNumberOfTransitionFrames = iNumberOfIncludedFrames - iStartNumberOfTransitionFrames;
					}
				}
				else if ( iEndTransitionColor == EColorTransition )
				{
					if ( ( iNumberOfIncludedFrames >> 1 ) > iEndNumberOfTransitionFrames )
					{
						iStartNumberOfTransitionFrames = iNumberOfIncludedFrames - iEndNumberOfTransitionFrames;
					}
					else
					{
						iStartNumberOfTransitionFrames = iNumberOfIncludedFrames >> 1;
						iEndNumberOfTransitionFrames = iNumberOfIncludedFrames - iStartNumberOfTransitionFrames;
					}
				}
				else
				{
					iStartNumberOfTransitionFrames = iNumberOfIncludedFrames >> 1;
					iEndNumberOfTransitionFrames = iNumberOfIncludedFrames - iStartNumberOfTransitionFrames;
				}
			}
		}            
    }
    // if transition is at one end only
    else 
    {
		if ( iStartOfClipTransition )
		{   
			iEndNumberOfTransitionFrames = 0;
			if ( iStartTransitionColor == EColorTransition )
			{
				iStartNumberOfTransitionFrames >>= 1;
				
				// if there are not enough frames saved from previous
			    // clip, the transition must be shortened accordingly
			    if (iProcessor->GetNumberOfSavedFrames() < iStartNumberOfTransitionFrames)
                    iStartNumberOfTransitionFrames = iProcessor->GetNumberOfSavedFrames();			
			}

			if ( iNumberOfIncludedFrames < iStartNumberOfTransitionFrames )
			{
				iStartNumberOfTransitionFrames = iNumberOfIncludedFrames; 
			}
		}
		else
		{
			iStartNumberOfTransitionFrames = 0;
			if ( iEndTransitionColor == EColorTransition )
			{
				iEndNumberOfTransitionFrames >>= 1;
			}
			if ( iNumberOfIncludedFrames < iEndNumberOfTransitionFrames )
			{
				iEndNumberOfTransitionFrames = iNumberOfIncludedFrames; 
			}
		}             
    }
    // fetch the last Intra before transition begins.
    // should be done after the cutting as well.
    
    iLastIntraFrameBeforeTransition=0;
    if(iNumberOfIncludedFrames > 2) //so that we could loop to find the last intra.
    {
        TInt i;
        TInt j=iProcessor->GetStartFrameIndex(); // processor needs frame index from beginning of clip
        for(i=endFrameIndex-iEndNumberOfTransitionFrames; i>=startFrameIndex;i--)
        {
            if(iProcessor->GetVideoFrameType(i+j) == 1)	// absolute index needed here!
            {
                iLastIntraFrameBeforeTransition=i;
                break;
            }
        }
    }
    
}

// ---------------------------------------------------------
// CVideoProcessor::SetTransitionFrameParams
// Set parameters for a transition frame
// (other items were commented in a header).
// ---------------------------------------------------------
//	
void CVideoProcessor::SetTransitionFrameParams(TInt aIncludedFrameNumber, TBool& aDecodeFrame)
{

    // if transition is to be applied at the beginning of the clip
    if(iStartOfClipTransition)
    {
        iFirstFrameAfterTransition = EFalse;
        // this is for start-of-clip transition
        if(aIncludedFrameNumber < iStartNumberOfTransitionFrames)
        {
            // if its first transition frame
            if(aIncludedFrameNumber == 0)
            {
                iFirstTransitionFrame = 1;
                iTransitionFrameNumber = 0;
            }
            else
            {
                iTransitionFrameNumber++;							
            }

			if ( iStartTransitionColor == EColorTransition )
			{
				// ignore this transition if the previous clip has less transition frames
				// than this clip's transition frames
				if ( iTransitionFrameNumber < ( iProcessor->NumberOfTransition() - iTransitionFrameNumber ) )
				{
					iTransitionFrame = 1;
					iTransitionPosition = EPositionStartOfClip;
					aDecodeFrame = ETrue;
					iTransitionColor = iStartTransitionColor;
				}
				else
				{
					iPreviousFrameIncluded = EFalse; 
				}
			}
			else
			{
				iTransitionFrame = 1;
				iTransitionPosition = EPositionStartOfClip;
				aDecodeFrame = EFalse;
				iTransitionColor = iStartTransitionColor;
			}
			
        }
        else
        {
            // if this is first frame after transition, we need to encode it as intra
            // treat/simulate it as if its the start of the cut point. 
            if(aIncludedFrameNumber == iStartNumberOfTransitionFrames)
            {
                iFirstFrameAfterTransition = ETrue;
                iPreviousFrameIncluded = EFalse; 
            }
        }
    }
    
    // if transition is to be applied at the end of the clip
    if(iEndOfClipTransition && iTransitionFrame == 0)
    {
        // this is for end-of-clip transition
        if(aIncludedFrameNumber >= iNumberOfIncludedFrames - iEndNumberOfTransitionFrames)
        {
            // if its first transition frame
            if(aIncludedFrameNumber == iNumberOfIncludedFrames - iEndNumberOfTransitionFrames)
            {
                iFirstTransitionFrame = 1;
                iTransitionFrameNumber = 0;
            }
            else
            {
                iTransitionFrameNumber++;
            }

			if ( iEndTransitionColor == EColorTransition )
			{
				// get the next clip's start transition information
				GetNextClipTransitionInfo();
				// if next clip's start transition number is less than current clip's
				// end transition number, then DO NOT treat current frame as the
				// the transition frame
				if ( ( iEndNumberOfTransitionFrames - iTransitionFrameNumber ) <= iNextTransitionNumber )
				{
					iTransitionFrame = 1;
					iTransitionPosition = EPositionEndOfClip;
					aDecodeFrame = ETrue;
					iTransitionColor = iEndTransitionColor;
				}
			}
			else
			{		
				iTransitionFrame = 1;
				iTransitionPosition = EPositionEndOfClip;
				aDecodeFrame = EFalse;
				iTransitionColor = iEndTransitionColor;
			}
		}
        else
        {
            // if this is first frame, we need to start decoding from here
            // treat/simulate it as if its the nearest preceding intra frame. 
            if(iFrameNumber >= iLastIntraFrameBeforeTransition)
                aDecodeFrame = ETrue;
            
            // In AVC case, if there is also starting transition, decode
            // all frames after that since frame numbering must be consistent
            // for AVC decoding to work
            if (iDataFormat == EDataAVC && iStartOfClipTransition)
                aDecodeFrame = ETrue;
            
        }	// end-of-clip transition
    }
    
}



// ---------------------------------------------------------
// CVideoProcessor::ApplyFadingTransitionEffect
// Applies fading transition effect for a YUV frame
// (other items were commented in a header).
// ---------------------------------------------------------
//	

void CVideoProcessor::ApplyFadingTransitionEffect(TUint8* aYUVPtr,
                                                  TTransitionPosition aTransitionPosition,
                                                  TTransitionColor aTransitionColor,
                                                  TInt aTransitionFramenumber)
{
	TInt i;

	TSize movieSize = iProcessor->GetMovieResolution();
	TInt yLength = movieSize.iWidth * movieSize.iHeight;	

	TInt uLength = yLength>>2;	
	TUint8* yFrame = (TUint8*)aYUVPtr;
	TUint8* uFrame = (TUint8*)(yFrame+yLength); 
	TUint8* vFrame = (TUint8*)(uFrame+uLength); 	
	TInt position;
	TChar chr; 
	TInt value; 
	TPtr8 ptr(0,0);

	// look-up tables to avoid floating point operations due to fractional weighting
	// corresponding to 0.1, 26 values quantized to 8
	const TUint8 quantTable1[8] = { 
		  4,   7,  10,  13,  16,  19,  22,  24 };
	// corresponding to 0.2, 52 values quantized to 16
	const TUint8 quantTable2[16] = { 
		  4,   8,  12,  15,  18,  21,  24,  27,  30,  33,  36,  39,  42,  45,  48,  50 };
	// corresponding to 0.3, 77 values quantized to 16
	const TUint8 quantTable3[16] = { 
		  5,  10,  15,  20,  24,  29,  33,  38,  42,  47,  51,  56,  61,  66,  71,  75 };
	// corresponding to 0.4, 103 values quantized to 32
	const TUint8 quantTable4[32] = { 
		  5,  10,  14,  18,  21,  24,  27,  30,  33,  36,  39,  42,  45,  48,  51,  54,
		 57,  60,  63,  66,  69,  72,  75,  78,  81,  84,  87,  90,  93,  96,  99, 101 };
	// corresponding to 0.5, 128 values quantized to 32
	const TUint8 quantTable5[32] = { 
		  3,   7,  11,  15,  19,  23,  27,  31,  35,  39,  43,  47,  51,  55,  59,  63,
		 67,  71,  75,  79,  83,  87,  91,  95,  99, 103, 107, 111, 115, 119, 123, 126 };
	// corresponding to 0.6, 154 values quantized to 32
	const TUint8 quantTable6[32] = { 
		  5,  13,  20,  27,  32,  36,  41,  45,  50,  54,  59,  63,  68,  72,  77,  81,
		 86,  90,  95,  99, 103, 108, 112, 117, 121, 126, 130, 135, 139, 144, 148, 152 };
	// corresponding to 0.7, 179 values quantized to 64
	const TUint8 quantTable7[64] = { 
		  5,   8,  11,  14,  17,  20,  23,  26,  29,  32,  35,  38,  41,  44,  47,  50,
		 53,  56,  59,  62,  65,  68,  71,  74,  77,  80,  83,  86,  89,  92,  95,  98,
		101, 104, 107, 110, 113, 116, 119, 122, 125, 128, 131, 134, 137, 140, 143, 146,
		149, 151, 153, 155, 157, 159, 161, 163, 165, 167, 169, 171, 173, 175, 177, 178 };
	// corresponding to 0.8, 204 values quantized to 64
	const TUint8 quantTable8[64] = { 
		  5,  10,  15,  20,  25,  29,  33,  36,  40,  42,  45,  48,  51,  54,  57,  60,
		 63,  66,  69,  72,  75,  78,  81,  84,  87,  90,  93,  96,  99, 102, 105, 108,
		111, 114, 117, 120, 123, 126, 129, 132, 135, 138, 141, 144, 147, 150, 153, 156,
		159, 162, 165, 168, 171, 174, 177, 180, 183, 186, 189, 192, 195, 198, 201, 203 };
	// corresponding to 0.9, 230 values quantized to 64
	const TUint8 quantTable9[64] = { 
		  5,  10,  15,  19,  23,  26,  30,  33,  37,  40,  44,  47,  51,  54,  58,  61,
		 65,  68,  72,  75,  79,  82,  86,  89,  93,  96, 100, 103, 107, 110, 114, 117,
		121, 124, 128, 131, 135, 138, 142, 145, 149, 152, 156, 159, 163, 166, 170, 173,
		177, 180, 184, 187, 191, 194, 198, 201, 205, 208, 212, 215, 219, 222, 226, 228 };
	const TUint8 indexTable[10]={1,2,3,4,5,6,7,8,9,10};

	// figure out if the transition is at the beginning or end of the clip	
    TInt index;
	switch(aTransitionPosition) 
	{
		case EPositionStartOfClip:		// start-of-clip transition
			if( (index = iStartNumberOfTransitionFrames - aTransitionFramenumber-1) < 0 ) index = 0;
		    break;
		case EPositionEndOfClip:		// end-of-clip transition
	        if( (index = aTransitionFramenumber) >= iEndNumberOfTransitionFrames ) index = iEndNumberOfTransitionFrames - 1;
			break;
		default:
			index = 0; 
			break;
	}
    position = indexTable[index];

	if(aTransitionColor==EColorWhite)
	{
		switch(position) // white
		{
		case 10: 	// 0% frame1, 100% frame2
			// Y
			value = 254; chr = value;
			ptr.Set(yFrame, yLength, yLength); 
			ptr.Fill(chr);
            // U,V
			value = 128; chr = value;
			ptr.Set(uFrame, uLength<<1, uLength<<1); 
			ptr.Fill(chr); 
			break;
		case 9:		// 10% frame1, 90% frame2
			value = quantTable9[63];	// 90% of 254 (white)
			for(i=0; i<yLength; i++, yFrame++) // Y
				*yFrame = (TUint8)(quantTable1[(*yFrame)>>5] + value);
			value = 113;							// 90% of 128
			for(i=0; i<uLength; i++, uFrame++,vFrame++)	// U
			{
				*uFrame = (TUint8)(quantTable1[(*uFrame)>>5] + value);
				*vFrame = (TUint8)(quantTable1[(*vFrame)>>5] + value);
			}
			break;
		case 8:		// 20% frame1, 80% frame2
			value = quantTable8[63];	// 80% of 254 (white)
			for(i=0; i<yLength; i++, yFrame++) // Y
				*yFrame = (TUint8)(quantTable2[(*yFrame)>>4] + value);
			value = 98;							// 80% of 128
			for(i=0; i<uLength; i++, uFrame++,vFrame++)	// U
			{
				*uFrame = (TUint8)(quantTable2[(*uFrame)>>4] + value);
				*vFrame = (TUint8)(quantTable2[(*vFrame)>>4] + value);
			}
			break;
		case 7:		// 30% frame1, 70% frame2
			value = quantTable7[63];	// 70% of 254 (white)
			for(i=0; i<yLength; i++, yFrame++) // Y
				*yFrame = (TUint8)(quantTable3[(*yFrame)>>4] + value);
			value = 86;							  // 70% of 128
			for(i=0; i<uLength; i++, uFrame++,vFrame++)	// U
			{
				*uFrame = (TUint8)(quantTable3[(*uFrame)>>4] + value);
				*vFrame = (TUint8)(quantTable3[(*vFrame)>>4] + value);
			}
			break;
		case 6:		// 40% frame1, 60% frame2
			value = quantTable6[31];	// 60% of 254 (white)
			for(i=0; i<yLength; i++, yFrame++) // Y
				*yFrame = (TUint8)(quantTable4[(*yFrame)>>3] + value);
			value = 72; //77;							  // 60% of 128
			for(i=0; i<uLength; i++, uFrame++,vFrame++)	// U
			{
				*uFrame = (TUint8)(quantTable4[(*uFrame)>>3] + value);
				*vFrame = (TUint8)(quantTable4[(*vFrame)>>3] + value);
			}
			break;
		case 5:		// 50% frame1, 50% frame2
			value = quantTable5[31];	// 50% of 254 (white)
			for(i=0; i<yLength; i++, yFrame++) // Y
				*yFrame = (TUint8)(quantTable5[(*yFrame)>>3] + value);
			value = 62;							  // 50% of 128
			for(i=0; i<uLength; i++, uFrame++,vFrame++)	// U
			{
				*uFrame = (TUint8)(quantTable5[(*uFrame)>>3] + value);
				*vFrame = (TUint8)(quantTable5[(*vFrame)>>3] + value);
			}
			break;
		case 4: 	// 60% frame1, 40% frame2
			value = quantTable4[31];	// 40% of 254 (white)
			for(i=0; i<yLength; i++, yFrame++) // Y
				*yFrame = (TUint8)(quantTable6[(*yFrame)>>3] + value);
			value = 44; //51;							  // 40% of 128
			for(i=0; i<uLength; i++, uFrame++,vFrame++)	// U
			{
				*uFrame = (TUint8)(quantTable6[(*uFrame)>>3] + value);
				*vFrame = (TUint8)(quantTable6[(*vFrame)>>3] + value);
			}
			break;
		case 3: 	// 70% frame1, 30% frame2
			value = quantTable3[15];	// 30% of 254 (white)
			for(i=0; i<yLength; i++, yFrame++) // Y
				*yFrame = (TUint8)(quantTable7[(*yFrame)>>2] + value);
			value = 28; //38;							  // 30% of 128
			for(i=0; i<uLength; i++, uFrame++,vFrame++)	// U
			{
				*uFrame = (TUint8)(quantTable7[(*uFrame)>>2] + value);
				*vFrame = (TUint8)(quantTable7[(*vFrame)>>2] + value);
			}
			break;
		case 2: 	// 80% frame1, 20% frame2
			value = quantTable2[15];	// 20% of 254 (white)
			for(i=0; i<yLength; i++, yFrame++) // Y
				*yFrame = (TUint8)(quantTable8[(*yFrame)>>2] + value);
			value = 18; //25;							  // 20% of 128
			for(i=0; i<uLength; i++, uFrame++,vFrame++)	// U
			{
				*uFrame = (TUint8)(quantTable8[(*uFrame)>>2] + value);
				*vFrame = (TUint8)(quantTable8[(*vFrame)>>2] + value);
			}
			break;
		case 1: 	// 90% frame1, 10% frame2
			value = quantTable1[7];	  // 10% of 254 (white)
			for(i=0; i<yLength; i++, yFrame++) // Y
				*yFrame = (TUint8)(quantTable9[(*yFrame)>>2] + value);
			value = 8; //13;							  // 10% of 128
			for(i=0; i<uLength; i++, uFrame++,vFrame++)	// U
			{
				*uFrame = (TUint8)(quantTable9[(*uFrame)>>2] + value);
				*vFrame = (TUint8)(quantTable9[(*vFrame)>>2] + value);
			}
			break;
		default: 	// e.g., 100% frame1, 0% frame2
			break;
		}
	}
	else if(aTransitionColor==EColorBlack) // black
	{
		switch(position)
		{
		case 10: 	// 0% frame1, 100% frame2
			// Y
			value = 4; chr = value;
			ptr.Set(yFrame, yLength, yLength); 
			ptr.Fill(chr);
            // U,V
			value = 128; chr = value;
			ptr.Set(uFrame, uLength<<1, uLength<<1); 
			ptr.Fill(chr); 
			break;
		case 9:		// 10% frame1, 90% frame2
			for(i=0; i<yLength; i++, yFrame++) // Y
				*yFrame = (TUint8)(quantTable1[(*yFrame)>>5]);
			value = 113;							// 90% of 128
			for(i=0; i<uLength; i++, uFrame++,vFrame++)	// U
			{
				*uFrame = (TUint8)(quantTable1[(*uFrame)>>5] + value);
				*vFrame = (TUint8)(quantTable1[(*vFrame)>>5] + value);
			}
			break;
		case 8:		// 20% frame1, 80% frame2
			for(i=0; i<yLength; i++, yFrame++) // Y
				*yFrame = (TUint8)(quantTable2[(*yFrame)>>4]);
			value = 98;							// 80% of 128
			for(i=0; i<uLength; i++, uFrame++,vFrame++)	// U
			{
				*uFrame = (TUint8)(quantTable2[(*uFrame)>>4] + value);
				*vFrame = (TUint8)(quantTable2[(*vFrame)>>4] + value);
			}
			break;
		case 7:		// 30% frame1, 70% frame2
			for(i=0; i<yLength; i++, yFrame++) // Y
				*yFrame = (TUint8)(quantTable3[(*yFrame)>>4]);
			value = 86;							  // 70% of 128
			for(i=0; i<uLength; i++, uFrame++,vFrame++)	// U
			{
				*uFrame = (TUint8)(quantTable3[(*uFrame)>>4] + value);
				*vFrame = (TUint8)(quantTable3[(*vFrame)>>4] + value);
			}
			break;
		case 6:		// 40% frame1, 60% frame2
			for(i=0; i<yLength; i++, yFrame++) // Y
				*yFrame = (TUint8)(quantTable4[(*yFrame)>>3]);
			value = 72; //77;							  // 60% of 128
			for(i=0; i<uLength; i++, uFrame++,vFrame++)	// U
			{
				*uFrame = (TUint8)(quantTable4[(*uFrame)>>3] + value);
				*vFrame = (TUint8)(quantTable4[(*vFrame)>>3] + value);
			}
			break;
		case 5:		// 50% frame1, 50% frame2
			for(i=0; i<yLength; i++, yFrame++) // Y
				*yFrame = (TUint8)(quantTable5[(*yFrame)>>3]);
			value = 62;							  // 50% of 128
			for(i=0; i<uLength; i++, uFrame++,vFrame++)	// U
			{
				*uFrame = (TUint8)(quantTable5[(*uFrame)>>3] + value);
				*vFrame = (TUint8)(quantTable5[(*vFrame)>>3] + value);
			}
			break;
		case 4: 	// 60% frame1, 40% frame2
			for(i=0; i<yLength; i++, yFrame++) // Y
				*yFrame = (TUint8)(quantTable6[(*yFrame)>>3]);
			value = 44; //51;							  // 40% of 128
			for(i=0; i<uLength; i++, uFrame++,vFrame++)	// U
			{
				*uFrame = (TUint8)(quantTable6[(*uFrame)>>3] + value);
				*vFrame = (TUint8)(quantTable6[(*vFrame)>>3] + value);
			}
			break;
		case 3: 	// 70% frame1, 30% frame2
			for(i=0; i<yLength; i++, yFrame++) // Y
				*yFrame = (TUint8)(quantTable7[(*yFrame)>>2]);
			value = 28; //38;							  // 30% of 128
			for(i=0; i<uLength; i++, uFrame++,vFrame++)	// U
			{
				*uFrame = (TUint8)(quantTable7[(*uFrame)>>2] + value);
				*vFrame = (TUint8)(quantTable7[(*vFrame)>>2] + value);
			}
			break;
		case 2: 	// 80% frame1, 20% frame2
			for(i=0; i<yLength; i++, yFrame++) // Y
				*yFrame = (TUint8)(quantTable8[(*yFrame)>>2]);
			value = 18; //25;							  // 20% of 128
			for(i=0; i<uLength; i++, uFrame++,vFrame++)	// U
			{
				*uFrame = (TUint8)(quantTable8[(*uFrame)>>2] + value);
				*vFrame = (TUint8)(quantTable8[(*vFrame)>>2] + value);
			}
			break;
		case 1: 	// 90% frame1, 10% frame2
			for(i=0; i<yLength; i++, yFrame++) // Y
				*yFrame = (TUint8)(quantTable9[(*yFrame)>>2]);
			value = 8; //13;							  // 10% of 128
			for(i=0; i<uLength; i++, uFrame++,vFrame++)	// U
			{
				*uFrame = (TUint8)(quantTable9[(*uFrame)>>2] + value);
				*vFrame = (TUint8)(quantTable9[(*vFrame)>>2] + value);
			}
			break;
		default: 	// e.g., 100% frame1, 0% frame2
			break;
		}
	} 
	return;
}

// ---------------------------------------------------------
// CVideoProcessor::ApplyBlendingTransitionEffect
// Applies blending transition effect between two YUV frames
// (other items were commented in a header).
// ---------------------------------------------------------
//	
void CVideoProcessor::ApplyBlendingTransitionEffect(TUint8* aYUVPtr1, 
												    TUint8* aYUVPtr2, 
												    TInt aRepeatFrame,
												    TInt aTransitionFramenumber)
{
	TInt i;
	TSize tempSize = iProcessor->GetMovieResolution();

	TInt yLength = tempSize.iWidth*tempSize.iHeight; 
	TInt uLength = yLength>>2;	
	TInt yuvLength = yLength + (yLength>>1);
	TUint8* yFrame1 = (TUint8*)aYUVPtr1;
	TUint8* uFrame1 = (TUint8*)(yFrame1+yLength); 
	TUint8* vFrame1 = (TUint8*)(uFrame1+uLength); 	
	TUint8* yFrame2 = (TUint8*)aYUVPtr2;
	TUint8* uFrame2 = (TUint8*)(yFrame2+yLength); 
	TUint8* vFrame2 = (TUint8*)(uFrame2+uLength); 	
	TInt position;
	TPtr8 ptr(0,0);
	const TInt numberOfTables = 10; 

	// corresponding to 0.1, 26 values quantized to 8
	const TUint8 quantTable1[8] = { 
		  4,   7,  10,  13,  16,  19,  22,  24 };
	// corresponding to 0.2, 52 values quantized to 16
	const TUint8 quantTable2[16] = { 
		  4,   8,  12,  15,  18,  21,  24,  27,  30,  33,  36,  39,  42,  45,  48,  50 };
	// corresponding to 0.3, 77 values quantized to 16
	const TUint8 quantTable3[16] = { 
		  5,  10,  15,  20,  24,  29,  33,  38,  42,  47,  51,  56,  61,  66,  71,  75 };
	// corresponding to 0.4, 103 values quantized to 32
	const TUint8 quantTable4[32] = { 
		  5,  10,  14,  18,  21,  24,  27,  30,  33,  36,  39,  42,  45,  48,  51,  54,
		 57,  60,  63,  66,  69,  72,  75,  78,  81,  84,  87,  90,  93,  96,  99, 101 };
	// corresponding to 0.5, 128 values quantized to 32
	const TUint8 quantTable5[32] = { 
		  3,   7,  11,  15,  19,  23,  27,  31,  35,  39,  43,  47,  51,  55,  59,  63,
		 67,  71,  75,  79,  83,  87,  91,  95,  99, 103, 107, 111, 115, 119, 123, 126 };
	// corresponding to 0.6, 154 values quantized to 32
	const TUint8 quantTable6[32] = { 
		  5,  13,  20,  27,  32,  36,  41,  45,  50,  54,  59,  63,  68,  72,  77,  81,
		 86,  90,  95,  99, 103, 108, 112, 117, 121, 126, 130, 135, 139, 144, 148, 152 };
	// corresponding to 0.7, 179 values quantized to 64
	const TUint8 quantTable7[64] = { 
		  5,   8,  11,  14,  17,  20,  23,  26,  29,  32,  35,  38,  41,  44,  47,  50,
		 53,  56,  59,  62,  65,  68,  71,  74,  77,  80,  83,  86,  89,  92,  95,  98,
		101, 104, 107, 110, 113, 116, 119, 122, 125, 128, 131, 134, 137, 140, 143, 146,
		149, 151, 153, 155, 157, 159, 161, 163, 165, 167, 169, 171, 173, 175, 177, 178 };
	// corresponding to 0.8, 204 values quantized to 64
	const TUint8 quantTable8[64] = { 
		  5,  10,  15,  20,  25,  29,  33,  36,  40,  42,  45,  48,  51,  54,  57,  60,
		 63,  66,  69,  72,  75,  78,  81,  84,  87,  90,  93,  96,  99, 102, 105, 108,
		111, 114, 117, 120, 123, 126, 129, 132, 135, 138, 141, 144, 147, 150, 153, 156,
		159, 162, 165, 168, 171, 174, 177, 180, 183, 186, 189, 192, 195, 198, 201, 203 };
	// corresponding to 0.9, 230 values quantized to 64
	const TUint8 quantTable9[64] = { 
		  5,  10,  15,  19,  23,  26,  30,  33,  37,  40,  44,  47,  51,  54,  58,  61,
		 65,  68,  72,  75,  79,  82,  86,  89,  93,  96, 100, 103, 107, 110, 114, 117,
		121, 124, 128, 131, 135, 138, 142, 145, 149, 152, 156, 159, 163, 166, 170, 173,
		177, 180, 184, 187, 191, 194, 198, 201, 205, 208, 212, 215, 219, 222, 226, 228 };

	const TUint8 indexTable[10]={1,2,3,4,5,6,7,8,9,10};

	// figure out the position of the index (determines which table to use) 
	TInt frameNumber = aTransitionFramenumber; 
	if(frameNumber>=iStartNumberOfTransitionFrames) frameNumber=iStartNumberOfTransitionFrames-1;
  TInt index = (frameNumber<<1) + aRepeatFrame;
	if(index>=numberOfTables) index=numberOfTables-1;
	position = indexTable[index];

	// calculate new values
	switch(position) 
	{
	case 10: 	// 0% frame1, 100% frame2
		ptr.Set(yFrame1,yuvLength,yuvLength);
		ptr.Copy(yFrame2,yuvLength);
		break;
	case 9:		// 10% frame1, 90% frame2
		for(i=0; i<yLength; i++, yFrame1++,yFrame2++) // Y
			*yFrame1 = (TUint8)(quantTable1[(*yFrame1)>>5] + quantTable9[(*yFrame2)>>2]);
		for(i=0; i<uLength; i++, uFrame1++,uFrame2++,vFrame1++,vFrame2++)	// U
		{
			*uFrame1 = (TUint8)(quantTable1[(*uFrame1)>>5] + quantTable9[(*uFrame2)>>2] - 10);
			*vFrame1 = (TUint8)(quantTable1[(*vFrame1)>>5] + quantTable9[(*vFrame2)>>2] - 10);
		}
		break;
	case 8:		// 20% frame1, 80% frame2
		for(i=0; i<yLength; i++, yFrame1++,yFrame2++) // Y
			*yFrame1 = (TUint8)(quantTable2[(*yFrame1)>>4] + quantTable8[(*yFrame2)>>2]);
		for(i=0; i<uLength; i++, uFrame1++,uFrame2++,vFrame1++,vFrame2++)	// U
		{
			*uFrame1 = (TUint8)(quantTable2[(*uFrame1)>>4] + quantTable8[(*uFrame2)>>2] - 15);
			*vFrame1 = (TUint8)(quantTable2[(*vFrame1)>>4] + quantTable8[(*vFrame2)>>2] - 15);
		}
		break;
	case 7:		// 30% frame1, 70% frame2
		for(i=0; i<yLength; i++, yFrame1++,yFrame2++) // Y
			*yFrame1 = (TUint8)(quantTable3[(*yFrame1)>>4] + quantTable7[(*yFrame2)>>2]);
		for(i=0; i<uLength; i++, uFrame1++,uFrame2++,vFrame1++,vFrame2++)	// U
		{
			*uFrame1 = (TUint8)(quantTable3[(*uFrame1)>>4] + quantTable7[(*uFrame2)>>2] - 15);
			*vFrame1 = (TUint8)(quantTable3[(*vFrame1)>>4] + quantTable7[(*vFrame2)>>2] - 15);
		}
		break;
	case 6:		// 40% frame1, 60% frame2
		for(i=0; i<yLength; i++, yFrame1++,yFrame2++) // Y
			*yFrame1 = (TUint8)(quantTable4[(*yFrame1)>>3] + quantTable6[(*yFrame2)>>3]);
		for(i=0; i<uLength; i++, uFrame1++,uFrame2++,vFrame1++,vFrame2++)	// U
		{
			*uFrame1 = (TUint8)(quantTable4[(*uFrame1)>>3] + quantTable6[(*uFrame2)>>3] - 10);
			*vFrame1 = (TUint8)(quantTable4[(*vFrame1)>>3] + quantTable6[(*vFrame2)>>3] - 10);
		}
		break;
	case 5:		// 50% frame1, 50% frame2
		for(i=0; i<yLength; i++, yFrame1++,yFrame2++) // Y
			*yFrame1 = (TUint8)(quantTable5[(*yFrame1)>>3] + quantTable5[(*yFrame2)>>3]);
		for(i=0; i<uLength; i++, uFrame1++,uFrame2++,vFrame1++,vFrame2++)	// U
		{
			*uFrame1 = (TUint8)(quantTable5[(*uFrame1)>>3] + quantTable5[(*uFrame2)>>3] - 5);
			*vFrame1 = (TUint8)(quantTable5[(*vFrame1)>>3] + quantTable5[(*vFrame2)>>3] - 5);
		}
		break;
	case 4: 	// 60% frame1, 40% frame2
		for(i=0; i<yLength; i++, yFrame1++,yFrame2++) // Y
			*yFrame1 = (TUint8)(quantTable6[(*yFrame1)>>3] + quantTable4[(*yFrame2)>>3]);
		for(i=0; i<uLength; i++, uFrame1++,uFrame2++,vFrame1++,vFrame2++)	// U
		{
			*uFrame1 = (TUint8)(quantTable6[(*uFrame1)>>3] + quantTable4[(*uFrame2)>>3] - 10);
			*vFrame1 = (TUint8)(quantTable6[(*vFrame1)>>3] + quantTable4[(*vFrame2)>>3] - 10);
		}
		break;
	case 3: 	// 70% frame1, 30% frame2
		for(i=0; i<yLength; i++, yFrame1++,yFrame2++) // Y
			*yFrame1 = (TUint8)(quantTable7[(*yFrame1)>>2] + quantTable3[(*yFrame2)>>4]);
		for(i=0; i<uLength; i++, uFrame1++,uFrame2++,vFrame1++,vFrame2++)	// U
		{
			*uFrame1 = (TUint8)(quantTable7[(*uFrame1)>>2] + quantTable3[(*uFrame2)>>4] - 8);
			*vFrame1 = (TUint8)(quantTable7[(*vFrame1)>>2] + quantTable3[(*vFrame2)>>4] - 8);
		}
		break;
	case 2: 	// 80% frame1, 20% frame2
		for(i=0; i<yLength; i++, yFrame1++,yFrame2++) // Y
			*yFrame1 = (TUint8)(quantTable8[(*yFrame1)>>2] + quantTable2[(*yFrame2)>>4]);
		for(i=0; i<uLength; i++, uFrame1++,uFrame2++,vFrame1++,vFrame2++)	// U
		{
			*uFrame1 = (TUint8)(quantTable8[(*uFrame1)>>2] + quantTable2[(*uFrame2)>>4] - 8);
			*vFrame1 = (TUint8)(quantTable8[(*vFrame1)>>2] + quantTable2[(*vFrame2)>>4] - 8);
		}
		break;
	case 1: 	// 90% frame1, 10% frame2
		for(i=0; i<yLength; i++, yFrame1++,yFrame2++) // Y
			*yFrame1 = (TUint8)(quantTable9[(*yFrame1)>>2] + quantTable1[(*yFrame2)>>5]);
		for(i=0; i<uLength; i++, uFrame1++,uFrame2++,vFrame1++,vFrame2++)	// U
		{
			*uFrame1 = (TUint8)(quantTable9[(*uFrame1)>>2] + quantTable1[(*uFrame2)>>5] - 5);
			*vFrame1 = (TUint8)(quantTable9[(*vFrame1)>>2] + quantTable1[(*vFrame2)>>5] - 5);
		}
		break;
	default: 	// e.g., 100% frame1, 0% frame2
		break;
	}
	return;
}


// ---------------------------------------------------------
// CVideoProcessor::ApplySlidingTransitionEffect
// Applies sliding transition effect between two YUV frames
// (other items were commented in a header).
// ---------------------------------------------------------
//	
void CVideoProcessor::ApplySlidingTransitionEffect(TUint8* aYUVPtr1, 
												   TUint8* aYUVPtr2,
												   TVedMiddleTransitionEffect aVedMiddleTransitionEffect,
												   TInt aRepeatFrame,
												   TInt aTransitionFramenumber)
{
	TInt i;
	TSize tempSize = iProcessor->GetMovieResolution();
	TInt yLength = tempSize.iWidth*tempSize.iHeight;
	TInt uLength = yLength>>2;
	TInt yWidth = tempSize.iWidth;
	TInt uWidth = tempSize.iWidth>>1;
	TInt yHeight = tempSize.iHeight; 
	TInt uHeight = tempSize.iHeight>>1; 
	TUint8* yFrame1 = (TUint8*)aYUVPtr1;
	TUint8* uFrame1 = (TUint8*)(yFrame1+yLength); 
	TUint8* vFrame1 = (TUint8*)(uFrame1+uLength); 	
	TUint8* yFrame2 = (TUint8*)aYUVPtr2;
	TUint8* uFrame2 = (TUint8*)(yFrame2+yLength); 
	TUint8* vFrame2 = (TUint8*)(uFrame2+uLength); 	
	TPtr8 ptr(0,0);
	TInt offset = 0;
	TInt ySliceWidth = 0;
	TInt uSliceWidth = 0;
	TInt sliceSize = 0;
	TInt frameNumber = (aTransitionFramenumber<<1) + aRepeatFrame;

	switch(aVedMiddleTransitionEffect)
	{
		case EVedMiddleTransitionEffectWipeLeftToRight:
			// figure out the amount of data to change 
            VDASSERT(iStartNumberOfTransitionFrames,19);
			ySliceWidth = (TInt)((TReal)yWidth * (TReal)(frameNumber+1)/(TReal)(iStartNumberOfTransitionFrames<<1) + 0.5); 
			if(ySliceWidth>yWidth) ySliceWidth = yWidth; 
			uSliceWidth = ySliceWidth>>1; 
			// copy the relevant portions of the image from frame2 to frame1 
			// y
			for(i=0; i<yHeight; i++, yFrame1+=yWidth,yFrame2+=yWidth)
			{
				ptr.Set(yFrame1,ySliceWidth,ySliceWidth);
				ptr.Copy(yFrame2,ySliceWidth);
			}
			// u,v
			for(i=0; i<uHeight; i++, uFrame1+=uWidth,uFrame2+=uWidth,vFrame1+=uWidth,vFrame2+=uWidth)
			{
				ptr.Set(uFrame1,uSliceWidth,uSliceWidth);
				ptr.Copy(uFrame2,uSliceWidth);
				ptr.Set(vFrame1,uSliceWidth,uSliceWidth);
				ptr.Copy(vFrame2,uSliceWidth);
			}
			break;
		case EVedMiddleTransitionEffectWipeRightToLeft:
			// figure out the amount of data to change 
            VDASSERT(iStartNumberOfTransitionFrames,20);
			ySliceWidth = (TInt)((TReal)yWidth * (TReal)(frameNumber+1)/(TReal)(iStartNumberOfTransitionFrames<<1) + 0.5); 
			if(ySliceWidth>yWidth) ySliceWidth = yWidth; 
			uSliceWidth = ySliceWidth>>1; 
			// evaluate the yuv offsets and new positions to point to in the buffer
			offset = yWidth-ySliceWidth;
			yFrame1+=offset;
			yFrame2+=offset;
			offset = uWidth-uSliceWidth;
			uFrame1+=offset;
			uFrame2+=offset;
			vFrame1+=offset;
			vFrame2+=offset;
			// copy the relevant portions of the image from frame2 to frame1 
			// y
			for(i=0; i<yHeight; i++, yFrame1+=yWidth,yFrame2+=yWidth)
			{
				ptr.Set(yFrame1,ySliceWidth,ySliceWidth);
				ptr.Copy(yFrame2,ySliceWidth);
			}
			// u,v
			for(i=0; i<uHeight; i++, uFrame1+=uWidth,uFrame2+=uWidth,vFrame1+=uWidth,vFrame2+=uWidth)
			{
				ptr.Set(uFrame1,uSliceWidth,uSliceWidth);
				ptr.Copy(uFrame2,uSliceWidth);
				ptr.Set(vFrame1,uSliceWidth,uSliceWidth);
				ptr.Copy(vFrame2,uSliceWidth);
			}
			break;
		case EVedMiddleTransitionEffectWipeTopToBottom:
			// figure out the amount of data to change 
            VDASSERT(iStartNumberOfTransitionFrames,21);
			ySliceWidth = (TInt)((TReal)yHeight * (TReal)(frameNumber+1)/(TReal)(iStartNumberOfTransitionFrames<<1) + 0.5); 
			if(ySliceWidth>yHeight) ySliceWidth = yHeight; 
			uSliceWidth = ySliceWidth>>1; 
			// copy the relevant portions of the image from frame2 to frame1 
			// y
			sliceSize = ySliceWidth * yWidth;
			ptr.Set(yFrame1,sliceSize,sliceSize);
			ptr.Copy(yFrame2,sliceSize);
			// u,v
			sliceSize = uSliceWidth * uWidth;
			ptr.Set(uFrame1,sliceSize,sliceSize);
			ptr.Copy(uFrame2,sliceSize);
			ptr.Set(vFrame1,sliceSize,sliceSize);
			ptr.Copy(vFrame2,sliceSize);
			break;
		case EVedMiddleTransitionEffectWipeBottomToTop:
			// figure out the amount of data to change 
            VDASSERT(iStartNumberOfTransitionFrames,22);
			ySliceWidth = (TInt)((TReal)yHeight * (TReal)(frameNumber+1)/(TReal)(iStartNumberOfTransitionFrames<<1) + 0.5); 
			if(ySliceWidth>yHeight) ySliceWidth = yHeight; 
			uSliceWidth = ySliceWidth>>1; 
			// evaluate the yuv offsets and new positions to point to in the buffer
			offset = (yHeight-ySliceWidth) * yWidth;
			yFrame1+=offset;
			yFrame2+=offset;
			offset = (uHeight-uSliceWidth) * uWidth;
			uFrame1+=offset;
			uFrame2+=offset;
			vFrame1+=offset;
			vFrame2+=offset;
			// copy the relevant portions of the image from frame2 to frame1 
			// y
			sliceSize = ySliceWidth * yWidth;
			ptr.Set(yFrame1,sliceSize,sliceSize);
			ptr.Copy(yFrame2,sliceSize);
			// u,v
			sliceSize = uSliceWidth * uWidth;
			ptr.Set(uFrame1,sliceSize,sliceSize);
			ptr.Copy(uFrame2,sliceSize);
			ptr.Set(vFrame1,sliceSize,sliceSize);
			ptr.Copy(vFrame2,sliceSize);
			break;		
		case EVedMiddleTransitionEffectNone:
		case EVedMiddleTransitionEffectDipToBlack:
		case EVedMiddleTransitionEffectDipToWhite:
		case EVedMiddleTransitionEffectCrossfade:
		case EVedMiddleTransitionEffectLast:
		default:
			break;
	}
	return;
}


// ---------------------------------------------------------
// CVideoProcessor::ApplySpecialEffect
// Applies color effect for a YUV frame
// (other items were commented in a header).
// ---------------------------------------------------------
//	
void CVideoProcessor::ApplySpecialEffect(TInt aColorEffect, TUint8* aYUVDataPtr, 
                                         TInt aColorToneU, TInt aColorToneV)
{
	VDASSERT(aYUVDataPtr,23); 
	VDASSERT(iVideoWidth,24); 
	VDASSERT(iVideoHeight,25); 
	TChar chr; 
	TInt value; 
	TInt offset;
	TInt length;
	// Values for the U & V Fill parameters
	TInt uFillValue, vFillValue;
	TPtr8 ptr(0,0); 
	TSize tempSize = iProcessor->GetMovieResolution();
	
	// asad - check if mpeg4, then change pixel range from (-128,127) to (0,255)
	if (iProcessor->GetOutputVideoType() == EVedVideoTypeMPEG4SimpleProfile)
	{
		// U
		aColorToneU += 128;
		if (aColorToneU<0)	 aColorToneU = 0;
		if (aColorToneU>255) aColorToneU = 255;
		// V
		aColorToneV += 128; 
		if (aColorToneV<0)	 aColorToneV = 0;
		if (aColorToneV>255) aColorToneV = 255;
	}
	TChar uChr, vChr;
	switch(aColorEffect)
	{
		case 0/*None*/: 
			return; 
		case 1/*BW*/:	
			value = 128; 
			chr = value;
			offset = tempSize.iWidth*tempSize.iHeight;
			length = offset>>1;								// u,v data length (2*L/2*W/2)
			ptr.Set((TUint8*)(aYUVDataPtr+offset), length, length); 
			ptr.Fill((TChar)chr); 
			break;
		case 2:
			offset = tempSize.iWidth*tempSize.iHeight;
			length = offset>>2;
			uFillValue = aColorToneU;
			uChr = uFillValue;
			vFillValue = aColorToneV;
			vChr = vFillValue;
			
			ptr.Set((TUint8*)(aYUVDataPtr + offset), length, length);
			ptr.Fill((TChar)uChr);
			
			offset = 1.25 * offset; // For filling the v-value
			ptr.Set((TUint8*)(aYUVDataPtr + offset), length, length);
			ptr.Fill((TChar)vChr);
			break;
		default:			
			return; 
	}
}

// ---------------------------------------------------------
// CVideoProcessor::TFrameOperation2TInt
// Converts frame operation enumeration to int
// (other items were commented in a header).
// ---------------------------------------------------------
//	
TInt CVideoProcessor::TFrameOperation2TInt(TDecoderFrameOperation aFrameOperation)
{
	switch(aFrameOperation)
	{
		case EDecodeAndWrite:
			return 1;
		case EDecodeNoWrite:
			return 2;
		case EWriteNoDecode:
			return 3;
		case ENoDecodeNoWrite:
			return 4;
		default:
			return KErrGeneral;
	}
}

// ---------------------------------------------------------
// CVideoProcessor::TColorEffect2TInt
// Converts  color effect enumeration to int
// (other items were commented in a header).
// ---------------------------------------------------------
//	
TInt CVideoProcessor::TColorEffect2TInt(TVedColorEffect aColorEffect)
{
	switch(aColorEffect)
	{
		case EVedColorEffectNone:
			return 0;
		case EVedColorEffectBlackAndWhite:
			return 1;
	    case EVedColorEffectToning:
			return 2;		
		default:
			return KErrGeneral;
	}
}



// ---------------------------------------------------------
// CVideoProcessor::InputDataAvailable
// Overridden CDataProcessor::InputDataAvailable() method
// Called when new input blocks are available
// (other items were commented in a header).
// ---------------------------------------------------------
//	
void CVideoProcessor::InputDataAvailable(TAny* /*aUserPointer*/)
{
    PRINT((_L("CVideoProcessor::InputDataAvailable()")));   
	// Signal ourselves if we are decoding and a request is not
	// pending:
	if ( iDecoding && !iTranscoderInitPending && !iDecodePending &&
	     (iStatus == KRequestPending) )
	{
	    PRINT((_L("CVideoProcessor::InputDataAvailable() - complete request")));   
		TRequestStatus *status = &iStatus;
		User::RequestComplete(status, KErrNone);
	}
}


// ---------------------------------------------------------
// CVideoProcessor::StreamEndReached
// Overridden CDataProcessor::StreamEndReached() method
// Called when input stream has ended
// (other items were commented in a header).
// ---------------------------------------------------------
//

void CVideoProcessor::StreamEndReached(TAny* /*aUserPointer*/)
{
    PRINT((_L("CVideoProcessor::StreamEndReached()")));   
	iStreamEnd = ETrue;
}

// ---------------------------------------------------------
// CVideoProcessor::DoCancel
// Cancels any asynchronous requests pending.
//
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoProcessor::DoCancel()
{

	// Cancel our internal request
	if ( iStatus == KRequestPending )
	{
		TRequestStatus *status = &iStatus;
		User::RequestComplete(status, KErrCancel);
	}

}

// ---------------------------------------------------------
// CVideoProcessor::GetNextClipTransitionInfo
// Get the start transition info of the next clip.
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoProcessor::GetNextClipTransitionInfo()
{
	if ( iNextTransitionNumber == -1 )
	{
		iNextTransitionNumber = iProcessor->NextClipStartTransitionNumber();
	}
}

// ---------------------------------------------------------
// CVideoProcessor::DetermineResolutionChange
// This function checks if the video clip is needed to be resampled
// (other items were commented in a header).
// ---------------------------------------------------------
// Resolution Transcoder, check if this video clip need to be resample

TBool CVideoProcessor::DetermineResolutionChange()
{
	TSize VideoClipResolution = iProcessor->GetVideoClipResolution();
	TSize MovieResolution = iProcessor->GetMovieResolution();
	if (VideoClipResolution != MovieResolution)
		return ETrue;
	else
		return EFalse;
}

// ---------------------------------------------------------
// CVideoProcessor::DetermineFrameRateChange
// This function checks if the frame rate must be changed
// => clip re-encoded
// (other items were commented in a header).
// ---------------------------------------------------------
//
TBool CVideoProcessor::DetermineFrameRateChange()
    {

    TReal clipFrameRate = iProcessor->GetVideoClipFrameRate();
    TReal movieFrameRate = iProcessor->GetMovieFrameRate();

    // Do re-encoding only when reducing frame rate, 
    // otherwise we would have to come up with new frames
    if ( movieFrameRate > 0 && clipFrameRate > movieFrameRate )
        return ETrue;

    return EFalse;
        
    }

// ---------------------------------------------------------
// CVideoProcessor::DetermineFrameRateChange
// This function checks if the bitrate must be changed
// => clip re-encoded
// (other items were commented in a header).
// ---------------------------------------------------------
//
TBool CVideoProcessor::DetermineBitRateChange()
    {

    // : AVC bitrate transcoding from a higher level
    //       to lower if resolution transcoding is not needed

    if ( iProcessor->GetMovieVideoBitrate() > 0 ) // movie has bitrate restriction => need to transcode
        return ETrue;
    
    if ( iProcessor->GetOutputVideoType() == EVedVideoTypeH263Profile0Level10 )
    {
        if (iDataFormat == EDataH263)
        {
            if ( (iProcessor->GetCurrentClipVideoType() != EVedVideoTypeH263Profile0Level10) &&
                 (iProcessor->GetCurrentClipVideoType() != EVedVideoTypeUnrecognized) )
                return ETrue;
        }
        
        else if (iDataFormat == EDataMPEG4)
        {
            // level 0 and level 1 max bitrate is 64 kb/s
            // others need to be transcoded
            if (iInputMPEG4ProfileLevelId != 8 && iInputMPEG4ProfileLevelId != 1)
                return ETrue;
        }
    }

    return EFalse;

    }


// ---------------------------------------------------------
// CVideoProcessor::GetFrameDuration
// Calculates the duration of current frame
// (other items were commented in a header).
// ---------------------------------------------------------
//
TInt CVideoProcessor::GetFrameDuration(TInt aFrameNumber)
{
    // calculate frame duration in ticks
    TInt startFrame = iProcessor->GetOutputNumberOfFrames() - iNumberOfFrames;
    TInt absFrameNumber = startFrame + aFrameNumber;
    TInt cur = absFrameNumber;
    TInt next = cur+1;
        
    // frameDuration is in ticks, with timescale of the current input clip
    if(next >= iProcessor->GetOutputNumberOfFrames())	
    {                		
    	return I64INT(iProcessor->GetVideoClipDuration() - iProcessor->VideoFrameTimeStamp(cur) );
    }
    else
    {
    	return I64INT( iProcessor->VideoFrameTimeStamp(next) - iProcessor->VideoFrameTimeStamp(cur) );                	
    }
}

// ---------------------------------------------------------
// CVideoProcessor::CheckVosHeaderL
// Checks whether the resynch bit is set if set then resets to zero
// (other items were commented in a header).
// @return TBool 
// ---------------------------------------------------------
//

TBool CVideoProcessor::CheckVosHeaderL(TPtrC8& aBuf)
{
	return iDecoder->CheckVOSHeaderL((TPtrC8&)aBuf);
}


// ---------------------------------------------------------
// CVideoProcessor::RenderFrame
// The H.263 decoder calls this function when a decoded
// frame is available for retrieving
// @return TInt
// ---------------------------------------------------------
//
TInt CVideoProcessor::RenderFrame(TAny* aFrame)
{	
	iDecoder->FrameRendered(aFrame);		
	return KErrNone;
}

// ---------------------------------------------------------
// CVideoProcessor::MtoTimerElapsed
// Called when timer has elapsed
// ---------------------------------------------------------
//
void CVideoProcessor::MtoTimerElapsed(TInt aError)
{   

    PRINT((_L("CVideoProcessor::MtoTimerElapsed() begin")));     

    if (aError != KErrNone)
    {
        iMonitor->Error(aError);
        return;   
    }
    
    VDASSERT(iFrameInfoArray[0].iEncodeFrame == 1, 110);        

    // if next frame in encode queue is an intermediate modification frame
    // and modification has not yet been applied, start waiting timer again        
    if ( ( (iFrameInfoArray[0].iTransitionFrame == 1) ||
           (iProcessor->GetColorEffect() != EVedColorEffectNone) ) &&
           iFrameInfoArray[0].iModificationApplied == 0 )
    {
        PRINT((_L("CVideoProcessor::MtoTimerElapsed() - modification not done yet, set timer")));
        
        iTimer->SetTimer( TTimeIntervalMicroSeconds32( GetEncodingDelay() ) );
        return;
    }
       
    PRINT((_L("CVideoProcessor::MtoTimerElapsed() - removing pic with ts %d ms"), 
        I64INT(iProcessor->GetVideoTimeInMsFromTicks(iFrameInfoArray[0].iTimeStamp, EFalse)) ))

    // save frame number to be able to recover in case the frame 
    // gets encoded regardless of the delay    
    iSkippedFrameNumber = iFrameInfoArray[0].iFrameNumber;
    
    // remove skipped frame from queue
    iFrameInfoArray.Remove(0);        
    
    PRINT((_L("CVideoProcessor::MtoTimerElapsed() - %d items in queue"), iFrameInfoArray.Count()));

    if (iDecodingSuspended && !iStreamEndRead)
    {
        if (iFrameInfoArray.Count() < iMaxItemsInProcessingQueue && !iDelayedWrite)
        {            
            PRINT((_L("CVideoProcessor::MtoTimerElapsed() - Resume decoding")));
            iDecodingSuspended = EFalse;
            // activate object to start decoding
            TRequestStatus *status = &iStatus;
            User::RequestComplete(status, KErrNone);
            return;
        }
    }    
    
    if ( !IsEncodeQueueEmpty() )
    {    
        // if there are still frames to be encoded, and next one is waiting,
        // set timer again
        if ( IsNextFrameBeingEncoded() )
        {
            if ( !iTimer->IsPending() )
            {
                PRINT((_L("CVideoProcessor::MtoTimerElapsed(), set timer again")));   
                iTimer->SetTimer( TTimeIntervalMicroSeconds32( iMaxEncodingDelay ) );
            }
        }
        return;
    }        
        
    if (iDelayedWrite)
    {     
        PRINT((_L("CVideoProcessor::MtoTimerElapsed(), writing delayed frame")));
        // write delayed frame
        TRAPD( error, WriteDelayedFrameL() );
        if (error != KErrNone)
        {
            iMonitor->Error(aError);
            return;   
        }
    }          
    
    if ( iStreamEndRead )
    {
        if ( iFrameInfoArray.Count() != 0 )
        {            
            // return now if we have read stream end, but there are still frames waiting
            // to be decoded => processing will be completed in MtroPictureFromTranscoder   
            PRINT((_L("CVideoProcessor::MtoTimerElapsed(), end read but frames in progress, return")));
            return;
        }
        
        else
        {
            // activate to stop processing
            PRINT((_L("CVideoProcessor::MtoTimerElapsed(), end read and all frames processed, stopping")));
            iProcessingComplete = ETrue;
        }
    }

    if (!IsActive())    
    {
        SetActive();
        iStatus = KRequestPending;
    }    
    // activate object to continue/end processing
    TRequestStatus *status = &iStatus;
    User::RequestComplete(status, KErrNone);               
    PRINT((_L("CVideoProcessor::MtoTimerElapsed() end")));
}

// ---------------------------------------------------------
// CVideoProcessor::WriteBufferL
// Called by transcoder to pass an encoded buffer
// ---------------------------------------------------------
//
void CVideoProcessor::WriteBufferL(CCMRMediaBuffer* aBuffer)
{
    if ( iFrameInfoArray.Count() == 0 )
        {
        PRINT((_L("CVideoProcessor::WriteBufferL() not ready to receive buffer, return")));
        return;
        }

    if ( ((iFrameInfoArray[0].iTranscoderMode != EFullWithIM) &&
          (iFrameInfoArray[0].iTranscoderMode != EFull)) ||
         (iFrameInfoArray[0].iEncodeFrame != 1) )
        {
        PRINT((_L("CVideoProcessor::WriteBufferL() encoded picture received but not expected, ignore & return")));
        return;
        }
        
               
    // cancel timer
    iTimer->CancelTimer();                	
	
	if (aBuffer->Type() == CCMRMediaBuffer::EVideoMPEG4DecSpecInfo)
	{
	    // should not happen
	    return;
	}		

    TTimeIntervalMicroSeconds encodedTs = aBuffer->TimeStamp();        
    
    PRINT((_L("CVideoProcessor::WriteBufferL(), timestamp %d ms, keyFrame = %d"),
               I64INT( encodedTs.Int64() ) / 1000, aBuffer->RandomAccessPoint() ));
    
    TBool removeItem = ETrue;
    TInt64 timeStamp = 0;    
    TInt frameNumber = 0;
    
    while (iFrameInfoArray.Count())
    {
        TTimeIntervalMicroSeconds ts = (iProcessor->GetVideoTimeInMsFromTicks(iFrameInfoArray[0].iTimeStamp, EFalse)) * 1000;

        if (ts < encodedTs)
        {            
            // a frame has been skipped
            iFrameInfoArray.Remove(0);
            PRINT((_L("CVideoProcessor::WriteBufferL() frame skipped - number of items in queue: %d"), iFrameInfoArray.Count()));    
        } 
        else if (ts > encodedTs)
        {
            // this frame has most likely been treated as skipped using timer, 
            // but it was encoded regardless
            removeItem = EFalse;
            frameNumber = iSkippedFrameNumber;
            timeStamp = iProcessor->GetVideoTimeInTicksFromMs( encodedTs.Int64() / 1000, EFalse );

            PRINT((_L("CVideoProcessor::WriteBufferL() frame skipped falsely, ts in ticks = %d"), I64INT(timeStamp)));            
            break;
        }       
        else
        {
            // normal case, encoded frame timestamp is as expected
            timeStamp = iFrameInfoArray[0].iTimeStamp;
            frameNumber = iFrameInfoArray[0].iFrameNumber;
            break;
        }
    }
    
    VDASSERT(iFrameInfoArray.Count(), 50);    
    
    // set descriptor for writing the frame
    TPtr8 writeDes(0,0);			
	writeDes.Set(const_cast<TUint8*>(aBuffer->Data().Ptr()), 
	             aBuffer->Data().Length(), aBuffer->Data().Length());

    HBufC8* tempBuffer = 0;
    TInt error;           

    if ( (iProcessor->GetOutputVideoType() == EVedVideoTypeMPEG4SimpleProfile) &&
         (!iOutputVolHeaderWritten) && (frameNumber >= iFirstIncludedFrameNumber) )         
    {
	    VDASSERT(iOutputVolHeader, 51);
	    
	    // MPEG-4 output:
	    // Encoded frame is the first one of the clip, insert VOL header at the beginning
	    // Allocate a temp buffer to include vol header and	the encoded frame	    
	    TInt length = iOutputVolHeader->Length() + aBuffer->Data().Length();	    	   
	    TRAP(error, tempBuffer = (HBufC8*) HBufC8::NewL(length) );
	    
	    if (error != KErrNone)
        {
            iMonitor->Error(error);
            return;
        }        
	    
	    TPtr8 ptr( tempBuffer->Des() );
	    ptr.Copy(iOutputVolHeader->Des());	    
	    ptr.Append(aBuffer->Data());
	    
	    writeDes.Set(tempBuffer->Des());
	    iOutputVolHeaderWritten = ETrue;
	}
	
#ifdef VIDEOEDITORENGINE_AVC_EDITING
	if (iProcessor->GetOutputVideoType() == EVedVideoTypeAVCBaselineProfile)
	{		   

	   // get number of NAL units	   
       TUint8* tmp = const_cast<TUint8*>(aBuffer->Data().Ptr() + aBuffer->Data().Length());
       
       tmp -= 4;
       TInt numNalUnits = TInt(tmp[0]) + (TInt(tmp[1])<<8) + (TInt(tmp[2])<<16) + (TInt(tmp[3])<<24);
              
       TInt totalLength = 0;
              
       // get nal_unit_type of first NAL
       TUint8* type = const_cast<TUint8*>(aBuffer->Data().Ptr());
       TInt nalType = *type & 0x1F;       
       
       PRINT((_L("CVideoProcessor::WriteBufferL() - # of NAL units = %d, frame # = %d, nal_unit_type = %d"), 
           numNalUnits, frameNumber, nalType));       
       
       if (nalType != 1 && nalType != 5)
       {
           // there are extra SPS/PPS units in the beginning
           // of the buffer, skip those
           numNalUnits--;
           
           if (numNalUnits == 0)
           {
               PRINT((_L("CVideoProcessor::WriteBufferL() No NAL units left, return")));            
               return;
           }
           
           // point to first length field
           tmp = const_cast<TUint8*>(aBuffer->Data().Ptr() + aBuffer->Data().Length());
           tmp -= 4;  // #
           tmp -= numNalUnits * 8; // offset & length for each NAL
           tmp += 4;  // skip offset

           // get NAL length
           TInt len = TInt(tmp[0]) + (TInt(tmp[1])<<8) + (TInt(tmp[2])<<16) + (TInt(tmp[3])<<24);           
           type += len;
           nalType = *type & 0x1F;
           
           while (nalType != 1 && nalType != 5 && numNalUnits)
           {
               numNalUnits--;
               tmp += 8;
               len = TInt(tmp[0]) + (TInt(tmp[1])<<8) + (TInt(tmp[2])<<16) + (TInt(tmp[3])<<24);
               type += len;
               nalType = *type & 0x1F;
           }
           tmp = const_cast<TUint8*>(aBuffer->Data().Ptr() + aBuffer->Data().Length()) - 4;
       }
       
       if (numNalUnits == 0)
       {
           PRINT((_L("CVideoProcessor::WriteBufferL() No NAL units left, return")));
           return;
       }
       
       // rewind to last length field
       tmp -= 4;
       
       // get total length of slices
       for (TInt x = numNalUnits; x > 0; x--)
       {
           totalLength += TInt(tmp[0]) + (TInt(tmp[1])<<8) + (TInt(tmp[2])<<16) + (TInt(tmp[3])<<24);
           tmp -= 8;
       }

       TInt tempLength = totalLength + numNalUnits*4;
              
       // allocate output buffer
	   TRAP(error, tempBuffer = (HBufC8*) HBufC8::NewL(tempLength) );
       if (error != KErrNone)
       {
           iMonitor->Error(error);
           return;  
       }

	   TUint8* dst = const_cast<TUint8*>(tempBuffer->Des().Ptr());
	   TUint8* src = const_cast<TUint8*>(aBuffer->Data().Ptr());
       
       // point to first offset field
       tmp += 4;

       for (TInt x = numNalUnits; x > 0; x--)
       {
           // get length
           tmp += 4;
           TInt length = TInt(tmp[0]) + (TInt(tmp[1])<<8) + (TInt(tmp[2])<<16) + (TInt(tmp[3])<<24);
           
           // set length           
           dst[0] = TUint8((length >> 24) & 0xff);
           dst[1] = TUint8((length >> 16) & 0xff);
           dst[2] = TUint8((length >> 8) & 0xff);
           dst[3] = TUint8(length & 0xff);
           dst += 4;
           
           // copy data
           TPtr8 ptr(dst, length);
           ptr.Copy(src, length);
           
           dst += length;
           src += length;
           
           // point to next offset field
           tmp +=4;
       }       

       writeDes.Set(tempBuffer->Des());
       writeDes.SetLength(tempLength);

	}
#endif
	
	// Figure out are we writing frames from the first
	// or second clip in color transition
			
    TBool colorTransitionFlag = ETrue;
    TInt index = KNumTransitionFrames / 4;    
	
	if ( iFrameInfoArray[0].iTransitionFrame == 1 &&
         iFrameInfoArray[0].iTransitionPosition == EPositionStartOfClip && 
         iStartTransitionColor == EColorTransition )
	{
	    if ( ( (iFrameInfoArray[0].iTransitionFrameNumber == index) && 
	           (iFrameInfoArray[0].iRepeatFrame == 0) ) ||
	            iFrameInfoArray[0].iTransitionFrameNumber < index )
	    {
            colorTransitionFlag = EFalse;
	    }
	}				

    // write frame
    error = iProcessor->WriteVideoFrameToFile((TDesC8&)writeDes, 
                     timeStamp, 0 /*dummy*/, 
                     aBuffer->RandomAccessPoint(), EFalse, colorTransitionFlag, ETrue );

    if (tempBuffer)    
        delete tempBuffer;                         
                     
    if (error == KErrCompletion)
    {
        PRINT((_L("CVideoProcessor::WriteBufferL() - processing complete")));    
        // stop processing
        iProcessingComplete = ETrue;
        iFrameInfoArray.Reset();
        VDASSERT(iTranscoderStarted, 51);
        iTransCoder->StopL();
        iTranscoderStarted = EFalse;
        if (!IsActive())
        {
            SetActive();
            iStatus = KRequestPending;
        }        
            
        // activate object to end processing
        TRequestStatus *status = &iStatus;
        User::RequestComplete(status, KErrNone);       
        return;
    }
    
    else if (error != KErrNone)
    {
        iMonitor->Error(error);
        return;
    }        
                     
    TInt startFrame = iProcessor->GetOutputNumberOfFrames() - iNumberOfFrames;
    TInt absFrameNumber = startFrame + frameNumber;
    
    // save frame number
    iLastWrittenFrameNumber = frameNumber;
    
    iProcessor->SetFrameType(absFrameNumber, aBuffer->RandomAccessPoint());
    
    if (removeItem)
    {        
        iFrameInfoArray.Remove(0);     
           
        PRINT((_L("CVideoProcessor::WriteBufferL() - removed encoded pic, %d items in queue"), iFrameInfoArray.Count()));
    } 
    else
        PRINT((_L("CVideoProcessor::WriteBufferL() - did not remove encoded pic, %d items in queue"), iFrameInfoArray.Count()));
        
           
    if (iDecodingSuspended && !iStreamEndRead)
    {
        if (iFrameInfoArray.Count() < iMaxItemsInProcessingQueue && !iDelayedWrite)
        {            
            PRINT((_L("CVideoProcessor::WriteBufferL() - Resume decoding")));
            iDecodingSuspended = EFalse;
            // activate object to start decoding
            TRequestStatus *status = &iStatus;
            User::RequestComplete(status, KErrNone);
            return;
        }
    }
        
    // if there are still frames to be encoded, start timer again        
    if ( !IsEncodeQueueEmpty() )
    {
        // check if the next frame in queue is waiting to be encoded, set timer if so
        if ( IsNextFrameBeingEncoded() )
        {
            if ( !iTimer->IsPending() )
            {
                PRINT((_L("CVideoProcessor::WriteBufferL(), set timer")));   
                iTimer->SetTimer( TTimeIntervalMicroSeconds32( iMaxEncodingDelay ) );
            }
        }
        return;
    }

    if (iStreamEndRead && !iDelayedWrite)
    {
    
        PRINT((_L("CVideoProcessor::WriteBufferL() - stream end read & !iDelayedWrite")));    
    
        // stream end has been read
        if (iFrameInfoArray.Count() == 0)
        {
            PRINT((_L("CVideoProcessor::WriteBufferL() - stream end read, no frames left")));
            // end
            if (!IsActive())
            {
                SetActive();
                iStatus = KRequestPending;
            }        
            iProcessingComplete = ETrue;
            // activate object to end processing
            TRequestStatus *status = &iStatus;
            User::RequestComplete(status, KErrNone);                   
        }        
        // else there are frames to be decoded, processing will be completed
        // MtroPictureFromTranscoder
        return;
    }

    if (iDelayedWrite)
    {
        if ( IsEncodeQueueEmpty() )
        {
            PRINT((_L("CVideoProcessor::WriteBufferL() writing delayed frame")));            
            
            TRAP(error, WriteDelayedFrameL());
            if (error != KErrNone)
            {
                iMonitor->Error(error);
                return;
            }

            if ( iStreamEndRead )
            {
                if ( iFrameInfoArray.Count() != 0 )
                {            
                    // return now if we have read stream end, but there are still frames waiting
                    // to be decoded => processing will be completed in MtroPictureFromTranscoder   
                    PRINT((_L("CVideoProcessor::WriteBufferL(), end read but frames in progress, return")));
                    return;
                }                
                else
                {
                    // activate to stop processing
                    PRINT((_L("CVideoProcessor::WriteBufferL(), end read and all frames processed, stopping")));
                    iProcessingComplete = ETrue;
                }
            }
            
            if (!IsActive())
            {
                SetActive();
                iStatus = KRequestPending;
            }
            // activate object to continue/end processing
            TRequestStatus *status = &iStatus;
            User::RequestComplete(status, KErrNone);
            return;
        }
        else
        {                
            // check if the next frame in queue is waiting to be encoded, set timer if so
            if ( IsNextFrameBeingEncoded() )
            {
                if ( !iTimer->IsPending() )
                {
                    PRINT((_L("CVideoProcessor::WriteBufferL() - iDelayedWrite, set timer")));   
                    iTimer->SetTimer( TTimeIntervalMicroSeconds32( iMaxEncodingDelay ) );
                }            
                return;
            }
        }
    }        
      
}

// ---------------------------------------------------------
// CVideoProcessor::IsEncodeQueueEmpty
// (other items were commented in a header)
// ---------------------------------------------------------
//
TBool CVideoProcessor::IsEncodeQueueEmpty()
{

    // check if there are still frames waiting to be encoded
    for (TInt i = 0; i < iFrameInfoArray.Count(); i++)
    {
         if (iFrameInfoArray[i].iEncodeFrame == 1)             
         {
             return EFalse;         
         }
    }
    return ETrue;
    
}

// ---------------------------------------------------------
// CVideoProcessor::IsNextFrameBeingEncoded
// (other items were commented in a header)
// ---------------------------------------------------------
//
TBool CVideoProcessor::IsNextFrameBeingEncoded()
{
   
    // check if the next frame in queue is waiting to be encoded
    if ( iFrameInfoArray.Count() && (iFrameInfoArray[0].iEncodeFrame == 1) )
    {
        
        VDASSERT( ( iFrameInfoArray[0].iTranscoderMode == EFull ||
                    iFrameInfoArray[0].iTranscoderMode == EFullWithIM ), 120 );
        
        if ( (iFrameInfoArray[0].iTranscoderMode == EFull) ||
             (iFrameInfoArray[0].iModificationApplied == 1) )
        {
            return ETrue;            
        }
    }
       
    return EFalse;
    
}

// ---------------------------------------------------------
// CVideoProcessor::GetEncodingDelay
// (other items were commented in a header)
// ---------------------------------------------------------
//
TInt CVideoProcessor::GetEncodingDelay()
{

    // number of decode only -frames in queue before first encode frame
    TInt numDecodeFrames = 0;
    
    TInt i;
    for (i = 0; i < iFrameInfoArray.Count(); i++)
    {
        // get index of next encode frame in queue
        if (iFrameInfoArray[i].iEncodeFrame == 1)
            break;
        else 
            numDecodeFrames++;
    }
    
    VDASSERT(i < iFrameInfoArray.Count(), 112);
    
    TInt delay = iMaxEncodingDelay;
    
    // If the next frame in encoding queue is an intermediate modification frame
    // (either transition frame or color effect has to be applied)    
    // and modification has not been applied to it, double the default delay    
    if ( ( (iFrameInfoArray[0].iTransitionFrame == 1) ||
           (iProcessor->GetColorEffect() != EVedColorEffectNone) ) &&
           iFrameInfoArray[0].iModificationApplied == 0 )
    {
        PRINT((_L("CVideoProcessor::GetEncodingDelay() - double the delay")));
        delay <<= 1;
    }        
    
    // add time to process decode-only frames to delay
    delay += numDecodeFrames * (iMaxEncodingDelay / 2);
    
    PRINT((_L("CVideoProcessor::GetEncodingDelay() - encoding delay = %d ms, num decode frames %d"), delay/1000, numDecodeFrames));
            
    return delay;
        
    
    
}

// ---------------------------------------------------------
// CVideoProcessor::WriteDelayedFrameL
// (other items were commented in a header)
// ---------------------------------------------------------
//
void CVideoProcessor::WriteDelayedFrameL()
{

    PRINT((_L("CVideoProcessor::WriteDelayedFrameL() begin")));
    
    // write the delayed frame
    TPtr8 ptr(iDelayedBuffer->Des());            
    
    TInt error = iProcessor->WriteVideoFrameToFile(ptr, 
                iDelayedTimeStamp, 0 /*dummy*/, 
                iDelayedKeyframe, EFalse, EFalse, EFalse );
                
    if (error == KErrCompletion)
    {
        PRINT((_L("CVideoProcessor::WriteDelayedFrameL() write delayed frame, processing complete")));
        VDASSERT(iTranscoderStarted, 51);
        iTransCoder->StopL();
        iTranscoderStarted = EFalse;
        iFrameInfoArray.Reset();
        iTimer->CancelTimer();
        iProcessingComplete = ETrue;                                  
    } 
    else if (error != KErrNone)
    {
        User::Leave(error);        
    }      
    
    // save frame number
    iLastWrittenFrameNumber = iDelayedFrameNumber;
    
    delete iDelayedBuffer;
    iDelayedBuffer = 0;
    iDelayedWrite = EFalse;        
    
    PRINT((_L("CVideoProcessor::WriteDelayedFrameL() end")));   
    
}
            
TInt CVideoProcessor::SetVideoFrameSize(TSize /*aSize*/)
{

    PRINT((_L("CVideoProcessor::SetVideoFrameSize()")))    
    
    return KErrNone;
}
            
TInt CVideoProcessor::SetAverageVideoBitRate(TInt /*aBitRate*/)
{

    PRINT((_L("CVideoProcessor::SetAverageVideoBitRate()")))
    
    return KErrNone;
}
        
   
TInt CVideoProcessor::SetMaxVideoBitRate(TInt /*aBitRate*/)
{
    PRINT((_L("CVideoProcessor::SetMaxVideoBitRate()")))
    
    return KErrNone;
}
            
TInt CVideoProcessor::SetAverageAudioBitRate(TInt /*aBitRate*/)
{
    PRINT((_L("CVideoProcessor::SetAverageAudioBitRate()")))    
    
    return KErrNone;
}

// ---------------------------------------------------------
// CVideoProcessor::SetVideoCodecL()
// Interpret and store video mime type
// ---------------------------------------------------------
//
void CVideoProcessor::SetOutputVideoCodecL(const TPtrC8& aMimeType)
    {
    TBuf8<256> string;
    TBuf8<256> newMimeType;
    string = KVedMimeTypeH263;
    string += _L8( "*" );		

    iMaxOutputFrameRate = 15.0;
    iArbitrarySizeAllowed = EFalse;

    if ( aMimeType.MatchF( (const TDesC8& )string ) != KErrNotFound ) 
        {
        // H.263

        newMimeType = KVedMimeTypeH263;

        if ( aMimeType.MatchF( _L8("*profile*") ) != KErrNotFound )
            {
            // profile given, check if we support it
            if ( aMimeType.MatchF( _L8("*profile=0*")) != KErrNotFound )
                {
                // profile 0 requested
                newMimeType += _L8( "; profile=0" );
                }
            else
                {
                // no other profiles supported
                PRINT((_L("CVideoEncoder::SetVideoCodecL() unsupported profile")));
                User::Leave(KErrNotSupported);
                }
            }
        else
            {
            // no profile is given => assume 0
            newMimeType += _L8( "; profile=0" );
            }

        if ( aMimeType.MatchF( _L8("*level=10*") ) != KErrNotFound )
            {
    		iMaxOutputBitRate = iOutputBitRate = KVedBitRateH263Level10;
    		iMaxOutputResolution = KVedResolutionQCIF;
    		//dataBufferSize = KMaxCodedPictureSizeQCIF;
            newMimeType += _L8( "; level=10" );
            }
        else if ( aMimeType.MatchF( _L8("*level=45*") ) != KErrNotFound )
            {
    		iMaxOutputBitRate = iOutputBitRate = KVedBitRateH263Level45;
    		iMaxOutputResolution = KVedResolutionQCIF;
    		//dataBufferSize = KMaxCodedPictureSizeQCIF;
            newMimeType += _L8( "; level=45" );
            }
        else if ( aMimeType.MatchF( _L8("*level*") ) != KErrNotFound )
            {
            // no other levels supported
            PRINT((_L("CVideoEncoder::SetVideoCodecL() unsupported level")));
            User::Leave(KErrNotSupported);
            }
        else
            {
            // if no level is given assume 10
    		iMaxOutputBitRate = iOutputBitRate = KVedBitRateH263Level10;
    		iMaxOutputResolution = KVedResolutionQCIF;
    		//dataBufferSize = KMaxCodedPictureSizeQCIF;
            newMimeType += _L8( "; level=10" );
            }
        }
    else
        {
        string = KVedMimeTypeMPEG4Visual;
        string += _L8( "*" );

        if ( aMimeType.MatchF( string ) != KErrNotFound ) 
            {
            // MPEG-4 Visual
            newMimeType = KVedMimeTypeMPEG4Visual;
            if ( aMimeType.MatchF( _L8("*profile-level-id=8*") ) != KErrNotFound )
                {
                // simple profile level 0
        		iMaxOutputBitRate = iOutputBitRate = KVedBitRateMPEG4Level0;
        		iMaxOutputResolution = KVedResolutionQCIF;
                // define max size 10K
                //dataBufferSize = KMaxCodedPictureSizeMPEG4QCIF;
                newMimeType += _L8("; profile-level-id=8");
                }
            else if ( aMimeType.MatchF( _L8("*profile-level-id=9*") ) != KErrNotFound )
                {
                // simple profile level 0b
        		iMaxOutputBitRate = iOutputBitRate = KVedBitRateMPEG4Level0;
        		iMaxOutputResolution = KVedResolutionQCIF;
                // define max size 10K
                //dataBufferSize = KMaxCodedPictureSizeMPEG4QCIF;
                newMimeType += _L8("; profile-level-id=9");
                }
            else if ( aMimeType.MatchF( _L8("*profile-level-id=1*") ) != KErrNotFound )
                {
                // simple profile level 1
        		iMaxOutputBitRate = iOutputBitRate = KVedBitRateMPEG4Level0;
        		iMaxOutputResolution = KVedResolutionQCIF;
                // define max size 10K
                //dataBufferSize = KMaxCodedPictureSizeMPEG4QCIF;
                iArbitrarySizeAllowed = ETrue;                
                newMimeType += _L8("; profile-level-id=1");
                }
            else if ( aMimeType.MatchF( _L8("*profile-level-id=2*") ) != KErrNotFound )
                {
                // simple profile level 2
			    //dataBufferSize = KMaxCodedPictureSizeMPEG4CIF;
        		iMaxOutputResolution = KVedResolutionCIF;
			    iMaxOutputBitRate = iOutputBitRate = KVedBitRateMPEG4Level2;
                iArbitrarySizeAllowed = ETrue;                
                newMimeType += _L8("; profile-level-id=2");
                }
            else if ( aMimeType.MatchF( _L8("*profile-level-id=3*") ) != KErrNotFound )
                {
                // simple profile level 3
			    //dataBufferSize = KMaxCodedPictureSizeMPEG4CIF;
			    iMaxOutputBitRate = iOutputBitRate = KVedBitRateMPEG4Level2;
        		iMaxOutputResolution = KVedResolutionCIF;
			    iMaxOutputFrameRate = 30.0;
                iArbitrarySizeAllowed = ETrue;                
                newMimeType += _L8("; profile-level-id=3");
                }
            else if ( aMimeType.MatchF( _L8("*profile-level-id=4*") ) != KErrNotFound )
                {
                // simple profile level 4a
			    iMaxOutputBitRate = iOutputBitRate = KVedBitRateMPEG4Level4A;	        	
			    //dataBufferSize = KMaxCodedPictureSizeVGA;
        		iMaxOutputResolution = KVedResolutionVGA;
			    iMaxOutputFrameRate = 30.0;
                iArbitrarySizeAllowed = ETrue;                
                newMimeType += _L8("; profile-level-id=4");
                }
            else if ( aMimeType.MatchF( _L8("*profile-level-id=*") ) != KErrNotFound )
                {
                // no other profile-level ids supported
                PRINT((_L("CVideoEncoder::SetVideoCodecL() unsupported MPEG-4 profile-level")));
                User::Leave(KErrNotSupported);
                }
            else
                {
                // Default is level 0 in our case (normally probably level 1)
        		iMaxOutputBitRate = iOutputBitRate = KVedBitRateMPEG4Level0;
        		iMaxOutputResolution = KVedResolutionQCIF;
                // define max size 10K
                //dataBufferSize = KMaxCodedPictureSizeMPEG4QCIF;
                newMimeType += _L8("; profile-level-id=8");
                }
            }
                       
        else
            {
            
#ifdef VIDEOEDITORENGINE_AVC_EDITING
            string = KVedMimeTypeAVC;
            string += _L8( "*" );
            
            if ( aMimeType.MatchF( string ) != KErrNotFound ) 
                {
                // AVC
                newMimeType = KVedMimeTypeAVC;
                if ( aMimeType.MatchF( _L8("*profile-level-id=42800A*") ) != KErrNotFound )
                    {
                    // baseline profile level 1
            		iMaxOutputBitRate = iOutputBitRate = KVedBitRateAVCLevel1;
            		iMaxOutputResolution = KVedResolutionQCIF;                    
                    newMimeType += _L8("; profile-level-id=42800A");
                    }                    
                else if ( aMimeType.MatchF( _L8("*profile-level-id=42900B*") ) != KErrNotFound )
                    {
                    // baseline profile level 1b
            		iMaxOutputBitRate = iOutputBitRate = KVedBitRateAVCLevel1b;
            		iMaxOutputResolution = KVedResolutionQCIF;                    
                    newMimeType += _L8("; profile-level-id=42900B");
                    }
                else if ( aMimeType.MatchF( _L8("*profile-level-id=42800B*") ) != KErrNotFound )
                    {
                    // baseline profile level 1.1
            		iMaxOutputBitRate = iOutputBitRate = KVedBitRateAVCLevel1_1;
            		iMaxOutputResolution = KVedResolutionCIF;                    
                    newMimeType += _L8("; profile-level-id=42800B");
                    }
                else if ( aMimeType.MatchF( _L8("*profile-level-id=42800C*") ) != KErrNotFound )
                    {
                    // baseline profile level 1.2
            		iMaxOutputBitRate = iOutputBitRate = KVedBitRateAVCLevel1_2;
            		iMaxOutputResolution = KVedResolutionCIF;                    
                    newMimeType += _L8("; profile-level-id=42800C");
                    }
                //WVGA task
                else if ( aMimeType.MatchF( _L8("*profile-level-id=42801E*") ) != KErrNotFound )
                    {
                    // baseline profile level 3.0
                    iMaxOutputBitRate = iOutputBitRate = KVedBitRateAVCLevel3;
                    iMaxOutputResolution = KVedResolutionWVGA;                    
                    newMimeType += _L8("; profile-level-id=42801E");
                    }
                
                else if ( aMimeType.MatchF( _L8("*profile-level-id=42801F*") ) != KErrNotFound )
                    {
                    // baseline profile level 3.1
            		iMaxOutputBitRate = iOutputBitRate = KVedBitRateAVCLevel3_1;
            		iMaxOutputResolution = KVedResolutionWVGA;                    
                    newMimeType += _L8("; profile-level-id=42801F");
                    }        
                else if ( aMimeType.MatchF( _L8("*profile-level-id=*") ) != KErrNotFound )
                    {
                    // no other profile-level ids supported
                    PRINT((_L("CVideoEncoder::SetVideoCodecL() unsupported AVC profile-level")));
                    User::Leave(KErrNotSupported);
                    }
                else
                    {
                        // Default is level 1 (?)
            	 	    iMaxOutputBitRate = iOutputBitRate = KVedBitRateAVCLevel1;
            	 	    iMaxOutputResolution = KVedResolutionQCIF;
            		    newMimeType += _L8("; profile-level-id=42800A");
                    }                
                }

            else
                {
                // unknown mimetype
                User::Leave( KErrNotSupported );
                }
#else
           
               // unknown mimetype
               User::Leave( KErrNotSupported );
           
#endif
            }
        }

    // successfully interpreted the input mime type
    iOutputMimeType = newMimeType;
        
    /*if ( iDataBuffer )
        {
        delete iDataBuffer;
        iDataBuffer = NULL;
        }
	iDataBuffer = (HBufC8*) HBufC8::NewL(dataBufferSize);    */
	
    }
    
// ---------------------------------------------------------
// CVideoProcessor::GetVosHeaderSize()
// Gets the size of MPEG-4 VOS header (from encoder)
// ---------------------------------------------------------
//
TInt CVideoProcessor::GetVosHeaderSize()
{
    VDASSERT(iOutputVolHeader, 190);
    
    return iOutputVolHeader->Length();
}
    
// ---------------------------------------------------------
// CCallbackTimer::NewL()
// Two-phased constructor
// ---------------------------------------------------------
//
CCallbackTimer* CCallbackTimer::NewL(MTimerObserver& aObserver)
{
 
    CCallbackTimer* self = new (ELeave) CCallbackTimer(aObserver);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    
    return self;    
}

// ---------------------------------------------------------
// CCallbackTimer::CCallbackTimer()
// C++ default constructor.
// ---------------------------------------------------------
//
CCallbackTimer::CCallbackTimer(MTimerObserver& aObserver)  :
    CActive(EPriorityStandard), iObserver(aObserver)
    
{
}

// ---------------------------------------------------------
// CCallbackTimer::~CCallbackTimer()
// Destructor
// ---------------------------------------------------------
//
CCallbackTimer::~CCallbackTimer()
{ 
    Cancel();
       
    if ( iTimerCreated )
	{
        iTimer.Close();
		iTimerCreated = EFalse;
	}
}

// ---------------------------------------------------------
// CCallbackTimer::ConstructL()
// Symbian 2nd phase constructor 
// ---------------------------------------------------------
//
void CCallbackTimer::ConstructL()
{
  	// Create a timer 
	User::LeaveIfError(iTimer.CreateLocal());
	iTimerCreated = ETrue;
	
	// Add us to active scheduler
	CActiveScheduler::Add(this);	
}

// ---------------------------------------------------------
// CCallbackTimer::SetTimer()
// Set timer
// ---------------------------------------------------------
//
void CCallbackTimer::SetTimer(TTimeIntervalMicroSeconds32 aDuration)
{

//    __ASSERT_DEBUG(!iTimerRequestPending != 0, -5000); //CSI: #174-D: expression has no effect, just an assert debug no effect intended    
//    __ASSERT_DEBUG(iTimerCreated, -5001);
    
    PRINT((_L("CCallbackTimer::SetTimer()")))
    
    // activate timer to wait for encoding
    SetActive();
    iStatus = KRequestPending;        
    iTimer.After(iStatus, aDuration);
    iTimerRequestPending = ETrue;        
    
}

// ---------------------------------------------------------
// CCallbackTimer::CancelTimer()
// Cancel timer
// ---------------------------------------------------------
//
void CCallbackTimer::CancelTimer()
{ 
     PRINT((_L("CCallbackTimer::CancelTimer()")))
     Cancel();
}

// ---------------------------------------------------------
// CCallbackTimer::RunL()
// AO running method
// ---------------------------------------------------------
//
void CCallbackTimer::RunL()
{
    if ( iTimerRequestPending )
    {
        iTimerRequestPending = EFalse;
        
        // call observer
        iObserver.MtoTimerElapsed(KErrNone);                
    }
}

// ---------------------------------------------------------
// CCallbackTimer::DoCancel()
// AO cancelling method 
// ---------------------------------------------------------
//
void CCallbackTimer::DoCancel()
{

    // Cancel our timer request if we have one
    if ( iTimerRequestPending )
    {
        iTimer.Cancel();
        iTimerRequestPending = EFalse;
        return;
    }
    
}

// ---------------------------------------------------------
// CCallbackTimer::RunError()
// AO RunL error method
// ---------------------------------------------------------
//
TInt CCallbackTimer::RunError(TInt aError)
{
    Cancel();
    
    // call observer
    iObserver.MtoTimerElapsed(aError);

    return KErrNone;
}


// End of File

