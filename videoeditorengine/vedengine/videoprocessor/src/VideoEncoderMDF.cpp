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
* Implementation for video encoder.
*
*/


#include "statusmonitor.h"
#include "videoencoder.h"
#include "vedvideosettings.h"
#include "vedvolreader.h"
#include "vedavcedit.h"


// Assertion macro wrapper for code cleanup
#define VEASSERT(x) __ASSERT_DEBUG(x, User::Panic(_L("CVIDEOENCODER"), EInternalAssertionFailure)) 

// Debug print macro
#ifdef _DEBUG
#include <e32svr.h>
#define PRINT(x) RDebug::Print x
#else
#define PRINT(x)
#endif

// Constants


// time increment resolution
const TUint KTimeIncrementResolutionHW = 30000;
//const TUint KTimeIncrementResolutionSW = 29;

const TReal KMinRandomAccessRate = 0.2;
//const TUint KPictureQuality = 50;
//const TReal KLatencyQyalityTradeoff = 1.0;
//const TReal KQualityTemporalTradeoff = 0.8;
//const TBool KEncodingRealTime = EFalse;


// ================= MEMBER FUNCTIONS =======================

// ---------------------------------------------------------
// CVideoEncoder::NewL
// Two-phased constructor.
// ---------------------------------------------------------
//

CVideoEncoder* CVideoEncoder::NewL(CStatusMonitor *aMonitor, CVedAVCEdit* aAvcEdit, 
                                   const TPtrC8& aVideoMimeType)
{
	PRINT((_L("CVideoEncoder::NewL() : MDF"))); 
	
	CVideoEncoder* self = new (ELeave) CVideoEncoder();    
	CleanupStack::PushL( self );
	self->ConstructL(aMonitor, aAvcEdit, aVideoMimeType);
	CleanupStack::Pop();
	
	return self;    
}


CVideoEncoder::CVideoEncoder() : CActive(EPriorityStandard), iInputBuffer(0,0)
{    
    iPreviousTimeStamp = TTimeIntervalMicroSeconds(0);
    iKeyFrame = EFalse;
}


void CVideoEncoder::ConstructL(CStatusMonitor *aMonitor, CVedAVCEdit* aAvcEdit, 
                               const TPtrC8& aVideoMimeType)
{
	iTranscoder = NULL;	
	iMonitor = aMonitor;
	iAvcEdit = aAvcEdit;

	iFrameSize = KVedResolutionQCIF;
	iFrameRate = 15.0;	
	iInputFrameRate = 15.0;
	iRandomAccessRate = KMinRandomAccessRate;

    // interpret and store video codec MIME-type. Allocates also iDataBuffer
    SetVideoCodecL( aVideoMimeType );	

	iStatus = NULL;	
	
	iVolReader = CVedVolReader::NewL();
	
	// allocate input picture
	iInputPicture = new (ELeave) TTRVideoPicture;
	
	// Create a timer 
	User::LeaveIfError(iTimer.CreateLocal());
	iTimerCreated = ETrue;
	
	// Add us to active scheduler
	CActiveScheduler::Add(this);
}


// ---------------------------------------------------------
// CVideoEncoder::~CVideoEncoder()
// Destructor
// ---------------------------------------------------------
//
CVideoEncoder::~CVideoEncoder()
    {

    Cancel();

    if ( iInputPicture )
	{
        delete iInputPicture;
		iInputPicture = 0;
	}
	
	if ( iDataBuffer )
	{	        
	    delete iDataBuffer;
	    iDataBuffer = 0;
	}
	
    if ( iTranscoder )
	{
        delete iTranscoder;
		iTranscoder = 0;
	}
	
	if ( iVolReader )
	{
	    delete iVolReader;
	    iVolReader = 0;
	}
    
    if ( iTimerCreated )
	{
        iTimer.Close();
		iTimerCreated = EFalse;
	}


    }


// ---------------------------------------------------------
// CVideoEncoder::SetVideoCodecL()
// Interpret and store video mime type
// ---------------------------------------------------------
//
void CVideoEncoder::SetVideoCodecL(const TPtrC8& aMimeType)
    {
    TBuf8<256> string;
    TBuf8<256> newMimeType;
    string = KVedMimeTypeH263;
    string += _L8( "*" );
	TInt dataBufferSize = KMaxCodedPictureSizeQCIF;

    iMaxFrameRate = 15.0;
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
    		iMaxBitRate = iBitRate = KVedBitRateH263Level10;
    		iMaxResolution = KVedResolutionQCIF;
    		dataBufferSize = KMaxCodedPictureSizeQCIF;
            newMimeType += _L8( "; level=10" );
            }
        else if ( aMimeType.MatchF( _L8("*level=45*") ) != KErrNotFound )
            {
    		iMaxBitRate = iBitRate = KVedBitRateH263Level45;
    		iMaxResolution = KVedResolutionQCIF;
    		dataBufferSize = KMaxCodedPictureSizeQCIF;
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
    		iMaxBitRate = iBitRate = KVedBitRateH263Level10;
    		iMaxResolution = KVedResolutionQCIF;
    		dataBufferSize = KMaxCodedPictureSizeQCIF;
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
        		iMaxBitRate = iBitRate = KVedBitRateMPEG4Level0;
        		iMaxResolution = KVedResolutionQCIF;
                // define max size 10K
                dataBufferSize = KMaxCodedPictureSizeMPEG4QCIF;
                newMimeType += _L8("; profile-level-id=8");
                }
            else if ( aMimeType.MatchF( _L8("*profile-level-id=9*") ) != KErrNotFound )
                {
                // simple profile level 0b
        		iMaxBitRate = iBitRate = KVedBitRateMPEG4Level0;
        		iMaxResolution = KVedResolutionQCIF;
                // define max size 10K
                dataBufferSize = KMaxCodedPictureSizeMPEG4QCIF;
                newMimeType += _L8("; profile-level-id=9");
                }
            else if ( aMimeType.MatchF( _L8("*profile-level-id=1*") ) != KErrNotFound )
                {
                // simple profile level 1
        		iMaxBitRate = iBitRate = KVedBitRateMPEG4Level0;
        		iMaxResolution = KVedResolutionQCIF;
                // define max size 10K
                dataBufferSize = KMaxCodedPictureSizeMPEG4QCIF;
                iArbitrarySizeAllowed = ETrue;                
                newMimeType += _L8("; profile-level-id=1");
                }
            else if ( aMimeType.MatchF( _L8("*profile-level-id=2*") ) != KErrNotFound )
                {
                // simple profile level 2
			    dataBufferSize = KMaxCodedPictureSizeMPEG4CIF;
        		iMaxResolution = KVedResolutionCIF;
			    iMaxBitRate = iBitRate = KVedBitRateMPEG4Level2;
                iArbitrarySizeAllowed = ETrue;                
                newMimeType += _L8("; profile-level-id=2");
                }
            else if ( aMimeType.MatchF( _L8("*profile-level-id=3*") ) != KErrNotFound )
                {
                // simple profile level 3
			    dataBufferSize = KMaxCodedPictureSizeMPEG4CIF;
			    iMaxBitRate = iBitRate = KVedBitRateMPEG4Level2;
        		iMaxResolution = KVedResolutionCIF;
			    iMaxFrameRate = 30.0;
                iArbitrarySizeAllowed = ETrue;                
                newMimeType += _L8("; profile-level-id=3");
                }
            else if ( aMimeType.MatchF( _L8("*profile-level-id=4*") ) != KErrNotFound )
                {
                // simple profile level 4a
			    iMaxBitRate = iBitRate = KVedBitRateMPEG4Level4A;	        	
			    dataBufferSize = KMaxCodedPictureSizeVGA;
        		iMaxResolution = KVedResolutionVGA;
			    iMaxFrameRate = 30.0;
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
        		iMaxBitRate = iBitRate = KVedBitRateMPEG4Level0;
        		iMaxResolution = KVedResolutionQCIF;
                // define max size 10K
                dataBufferSize = KMaxCodedPictureSizeMPEG4QCIF;
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
            		iMaxBitRate = iBitRate = KVedBitRateAVCLevel1;
            		iMaxResolution = KVedResolutionQCIF;                        
                    dataBufferSize = KMaxCodedPictureSizeAVCLevel1;
                    iArbitrarySizeAllowed = ETrue;                
                    newMimeType += _L8("; profile-level-id=42800A");
                    }                    
                else if ( aMimeType.MatchF( _L8("*profile-level-id=42900B*") ) != KErrNotFound )
                    {                                                
                    // baseline profile level 1b
                    iMaxBitRate = iBitRate = KVedBitRateAVCLevel1b;
            		iMaxResolution = KVedResolutionQCIF;                        
                    dataBufferSize = KMaxCodedPictureSizeAVCLevel1B;
                    iArbitrarySizeAllowed = ETrue;                
                    newMimeType += _L8("; profile-level-id=42900B");
                    }
                else if ( aMimeType.MatchF( _L8("*profile-level-id=42800B*") ) != KErrNotFound )
                    {
                    // baseline profile level 1.1
                    iMaxBitRate = iBitRate = KVedBitRateAVCLevel1_1;
            		iMaxResolution = KVedResolutionCIF;                        
                    dataBufferSize = KMaxCodedPictureSizeAVCLevel1_1;
                    iArbitrarySizeAllowed = ETrue;                
                    newMimeType += _L8("; profile-level-id=42800B");
                    }
                else if ( aMimeType.MatchF( _L8("*profile-level-id=42800C*") ) != KErrNotFound )
                    {
                    // baseline profile level 1.2
                    iMaxBitRate = iBitRate = KVedBitRateAVCLevel1_2;
            		iMaxResolution = KVedResolutionCIF;                        
                    dataBufferSize = KMaxCodedPictureSizeAVCLevel1_2;
                    iArbitrarySizeAllowed = ETrue;                
                    newMimeType += _L8("; profile-level-id=42800C");                    
                    }      
                //WVGA task
                else if ( aMimeType.MatchF( _L8("*profile-level-id=42801F*") ) != KErrNotFound )
                    {
                    // baseline profile level 1.3
                    iMaxBitRate = iBitRate = KVedBitRateAVCLevel3_1;
            		iMaxResolution = KVedResolutionWVGA;                        
                    dataBufferSize = KMaxCodedPictureSizeAVCLevel3_1;
                    iArbitrarySizeAllowed = ETrue;                
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
                        iMaxBitRate = iBitRate = KVedBitRateAVCLevel1;
                		iMaxResolution = KVedResolutionQCIF;                            
                        dataBufferSize = KMaxCodedPictureSizeAVCLevel1;
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
    iMimeType = newMimeType;
    if ( iDataBuffer )
        {
        delete iDataBuffer;
        iDataBuffer = NULL;
        }
	iDataBuffer = (HBufC8*) HBufC8::NewL(dataBufferSize);    

    }
    
// ---------------------------------------------------------
// CVideoEncoder::SetFrameSizeL
// Sets the used frame size
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoEncoder::SetFrameSizeL(const TSize& aSize)
    {
  
    if (iFrameSize == aSize)
        return;

    if ( (aSize.iWidth > iMaxResolution.iWidth) || (aSize.iHeight > iMaxResolution.iHeight))
        {
        if ( iArbitrarySizeAllowed ) // This is for future-proofness. Currently the checking of standard sizes below overrules this one
            {
            if ( aSize.iWidth * aSize.iHeight > iMaxResolution.iWidth*iMaxResolution.iHeight )
                {
                PRINT((_L("CVideoEncoder::SetFrameSizeL() too high resolution requested")));
                User::Leave( KErrNotSupported );
                }
            }
        else
            {
            PRINT((_L("CVideoEncoder::SetFrameSizeL() incompatible or too high resolution requested")));
            User::Leave( KErrNotSupported );
            }
        }
    
	// check new size. For now only standard sizes are allowed
	if ( (aSize != KVedResolutionSubQCIF) && 
		 (aSize != KVedResolutionQCIF) &&
		 (aSize != KVedResolutionCIF) &&
		 (aSize != KVedResolutionQVGA) &&
		 (aSize != KVedResolutionVGA16By9) &&
		 (aSize != KVedResolutionVGA) &&
		 //WVGA task
		 (aSize != KVedResolutionWVGA) )
	{
		 User::Leave( KErrArgument );
	}

    iFrameSize = aSize;    
    
    }

// ---------------------------------------------------------
// CVideoEncoder::GetFrameSize
// Gets the used frame size
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoEncoder::FrameSize(TSize& aSize) const
    {

    aSize = iFrameSize;

    }

// ---------------------------------------------------------
// CVideoEncoder::SetFrameRate
// Sets the used targt frame rate
// (other items were commented in a header).
// ---------------------------------------------------------
//
TInt CVideoEncoder::SetFrameRate(const TReal aFrameRate)
{
    VEASSERT(aFrameRate > 0.0);

    if ( aFrameRate <= iMaxFrameRate )
        {
        iFrameRate = TReal32(aFrameRate);
        }
    else
        {
        iFrameRate = iMaxFrameRate;
        }
        
    // target framerate cannot be larger than source framerate
    if (iFrameRate > iInputFrameRate)
        iFrameRate = iInputFrameRate;

    return KErrNone;
}

// ---------------------------------------------------------
// CVideoEncoder::SetInputFrameRate
// Sets the used input sequence frame rate
// (other items were commented in a header).
// ---------------------------------------------------------
//
TInt CVideoEncoder::SetInputFrameRate(const TReal aFrameRate)
    {
    
    VEASSERT(aFrameRate > 0.0);
         
    iInputFrameRate = aFrameRate;
        
    // target framerate cannot be larger than source framerate
    if (iFrameRate > iInputFrameRate)
        iFrameRate = iInputFrameRate;
    
    return KErrNone;
        
    }

    
// ---------------------------------------------------------
// CVideoEncoder::SetRandomAccessRate
// Sets the used target random access rate
// (other items were commented in a header).
// ---------------------------------------------------------
//
TInt CVideoEncoder::SetRandomAccessRate(const TReal aRate)
{
    VEASSERT(aRate > 0.0);

    iRandomAccessRate = aRate;

    return KErrNone;
}

// ---------------------------------------------------------
// CVideoEncoder::SetBitrate
// Sets the used target bitrate
// (other items were commented in a header).
// ---------------------------------------------------------
//
TInt CVideoEncoder::SetBitrate(const TInt aBitrate)
{    
    VEASSERT(aBitrate > 0);

    if ( aBitrate <= iMaxBitRate )
        {
        iBitRate = aBitrate;
        }
    else
        {
        iBitRate = iMaxBitRate;
        }

    return KErrNone;

}

// --------------------------------------------------------



// ---------------------------------------------------------
// CVideoEncoder::InitializeL
// Initializes the encoder
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoEncoder::InitializeL(TRequestStatus &aStatus)
    {
    
    PRINT((_L("CVideoEncoder::InitializeL() in")));

    iResetRequestStatus = &aStatus;

    // Create DevVideoRecord & select video encoder

    if ( iTranscoder ) 
        {
        // need to recreate since parameters changed
        delete iTranscoder;
        iTranscoder = 0;
        }

    iFatalError = EFalse;

    iTranscoder = CTRTranscoder::NewL(*this);

    // Select & set parameters to transcoder    
    TRAPD(err, SetupEncoderL());
        
    if ( err != KErrNone )
        {
        // error
        User::Leave( err );
        }
    
    iTranscoder->InitializeL();
    
    PRINT((_L("CVideoEncoder::InitializeL() out")));
    

    }

// ---------------------------------------------------------
// CVideoEncoder::Start
// Starts the encoder
// (other items were commented in a header).
// ---------------------------------------------------------
//

void CVideoEncoder::Start()
{
    TRAPD( error, iTranscoder->StartL() );
    
    if (error != KErrNone)
        {
        if (iMonitor)
            iMonitor->Error(error);
        }
}


// ---------------------------------------------------------
// CVideoEncoder::SetRandomAccessPoint
// Forces the next encoded frame to Intra
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoEncoder::SetRandomAccessPoint()
{
    VEASSERT(iTranscoder);
    
    iTranscoder->SetRandomAccessPoint();
       
}

// ---------------------------------------------------------
// CVideoEncoder::Stop
// Stops the encoder
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoEncoder::Stop()
{
	
    if ( !iFatalError )
        {
        
        if (iStarted)
            {
            TRAPD( error, iTranscoder->StopL() );
            if (error != KErrNone)
                {
                if (iMonitor)
                    iMonitor->Error(error);
                }
            }
        
        iStarted = EFalse;
        }
}
	

// ---------------------------------------------------------
// CVideoEncoder::Reset
// Resets the encoder
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoEncoder::Reset(TRequestStatus &aStatus)
{

    PRINT((_L("CVideoEncoder::Reset() in")));    

    iPreviousTimeStamp = TTimeIntervalMicroSeconds(0);

    iResetRequestStatus = &aStatus;

    if ( iFatalError )
        {
        MtroAsyncStopComplete();
        }
    else
        {
        TRAPD(err, iTranscoder->AsyncStopL());
        if (err != KErrNone)
            iMonitor->Error(err);
        }

	iStarted = EFalse;

    PRINT((_L("CVideoEncoder::Reset() out")));
}

void CVideoEncoder::Reset()
{

    PRINT((_L("CVideoEncoder::Reset() (sync.) in")));

    iPreviousTimeStamp = TTimeIntervalMicroSeconds(0);

    if ( !iFatalError )
    {
        if (iStarted) 
        {            
            TRAPD(err, iTranscoder->StopL());
            if (err != KErrNone)
                iMonitor->Error(err);
        }
    }

	iStarted = EFalse;

    PRINT((_L("CVideoEncoder::Reset() (sync.) out")));
}

// ---------------------------------------------------------
// CVideoEncoder::RunL
// Active object running method.
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoEncoder::RunL()
{

    // Timer elapsed, complete encoding request
    // with KErrCancel

    PRINT((_L("CVideoEncoder::RunL() in")));

    if (iTimerRequestPending)
    {
        iTimerRequestPending = EFalse;
        if (iEncodeRequestStatus)
        {
#ifdef _DEBUG
            TTime current;
            current.UniversalTime();
            TInt64 time = current.MicroSecondsFrom(iEncodeStartTime).Int64();    
            PRINT((_L("CVideoEncoder::RunL(), completing request, time since encoding started %d"), I64INT( time )));
#endif

            VEASSERT(*iEncodeRequestStatus == KRequestPending);
            // complete request
            User::RequestComplete(iEncodeRequestStatus, KErrCancel);
            iEncodeRequestStatus = 0;
			iEncodePending = EFalse;
        }
    }
    PRINT((_L("CVideoEncoder::RunL() out")));

}

// ---------------------------------------------------------
// CVideoEncoder::RunError
// Active object error method.
// (other items were commented in a header).
// ---------------------------------------------------------
//
TInt CVideoEncoder::RunError(TInt aError)
{
    PRINT((_L("CVideoEncoder::RunError() in")));

    Cancel();
        
    iMonitor->Error(aError);

    PRINT((_L("CVideoEncoder::RunError() out")));
    
    return KErrNone;
}

// ---------------------------------------------------------
// CVideoEncoder::DoCancel
// Active object cancelling method
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoEncoder::DoCancel()
{

    PRINT((_L("CVideoEncoder::DoCancel() in")));

    // Cancel our timer request if we have one
    if ( iTimerRequestPending )
    {
        iTimer.Cancel();
        iTimerRequestPending = EFalse;
        return;
    }

}
       
// ---------------------------------------------------------
// CVideoEncoder::EncodeFrameL
// Encodes a frame
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoEncoder::EncodeFrameL(TPtr8& aYUVFrame, TRequestStatus &aStatus, TTimeIntervalMicroSeconds aTimeStamp)
    {
    PRINT((_L("CVideoEncoder::EncodeFrameL() in, aTimeStamp = %d"), I64INT( aTimeStamp.Int64() ) ));

    if ( iFatalError )
        {
        PRINT((_L("CVideoEncoder::EncodeFrameL() can't encode since fatal error has occurred earlier")));
        User::Leave( KErrGeneral );
        }

    iEncodeRequestStatus = &aStatus;

	if (!iStarted)
	{
		PRINT((_L("CVideoEncoder::EncodeFrameL() - starting devVideoRec")));
		//iTranscoder->StopL();  // NOTE: needed ??
		iTranscoder->StartL();
		iStarted = ETrue;
	}	 

    // wrap input frame to encoder input buffer
    iInputBuffer.Set(aYUVFrame);
    iInputPicture->iRawData = &iInputBuffer;   
    iInputPicture->iDataSize.SetSize(iFrameSize.iWidth, iFrameSize.iHeight);

    if (aTimeStamp > TTimeIntervalMicroSeconds(0))
    {
        TInt64 diff = aTimeStamp.Int64() - iPreviousTimeStamp.Int64();        

        if (diff < 0)
        {
            aTimeStamp = iPreviousTimeStamp.Int64() + TInt64(66667);
        }
        // NOTE: Could the real difference between two consecutive
        //       frames be used instead of assuming 15 fps ?
    }
    iPreviousTimeStamp = aTimeStamp;

    iInputPicture->iTimestamp = aTimeStamp;
    //iInputPicture->iLink = NULL;
    iInputPicture->iUser = this;

#ifdef _DEBUG
    iEncodeStartTime.UniversalTime();
#endif


	iEncodePending = ETrue;

    iTranscoder->SendPictureToTranscoderL( iInputPicture );

    PRINT((_L("CVideoEncoder::EncodeFrameL() out")));

    }


// ---------------------------------------------------------
// CVideoEncoder::GetBuffer
// Gets encoded frame bitstream buffer
// (other items were commented in a header).
// ---------------------------------------------------------
//
TPtrC8& CVideoEncoder::GetBufferL(TBool& aKeyFrame)
    {
    if ( iFatalError )
        {
        User::Leave( KErrNotReady );
        }

    VEASSERT(iDataBuffer->Length() > 0);

    PRINT((_L("CVideoEncoder::GetBufferL(), keyFrame = %d"), iKeyFrame));    

    aKeyFrame = iKeyFrame;    
        
    iReturnDes.Set(iDataBuffer->Des()); 
    return iReturnDes;    
 
    }

// ---------------------------------------------------------
// CVideoEncoder::ReturnBuffer
// Returns used bitstream buffer to devVideoRec
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoEncoder::ReturnBuffer()
    {
    if ( iFatalError )
        {
        return;
        }
        
    iDataBuffer->Des().Zero();

    }

// ---------------------------------------------------------
// CVideoEncoder::SetupEncoderL
// Private helper method to select & setup the encoder 
// plugin devvr must use
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVideoEncoder::SetupEncoderL()
    {

    if ( !(iTranscoder->SupportsOutputVideoFormat(iMimeType) ) )
        {
        User::Leave(KErrNotSupported);
        }             
        
    TTRVideoFormat videoInputFormat;
    TTRVideoFormat videoOutputFormat;
    
    videoInputFormat.iSize = iFrameSize;
    videoInputFormat.iDataType = CTRTranscoder::ETRYuvRawData420;

    videoOutputFormat.iSize = iFrameSize;
    videoOutputFormat.iDataType = CTRTranscoder::ETRDuCodedPicture;
        
    iTranscoder->OpenL( this,
                        CTRTranscoder::EEncoding,
                        KNullDesC8,
                        iMimeType,
                        videoInputFormat,
                        videoOutputFormat,
                        EFalse );

    // default, will be updated by ParseVolHeader
    iTimeIncrementResolution = KTimeIncrementResolutionHW;    //KTimeIncrementResolutionSW;
    
    iTranscoder->SetVideoBitRateL(iBitRate);
    
    iTranscoder->SetFrameRateL(iFrameRate);    
           
    iTranscoder->SetChannelBitErrorRateL(0.0);
       
    // Get processing time estimate from transcoder, divide it by the framerate to get processing time per frame
    // and then multiply it by 2 to get some safety margin and by unit conversion factor 1000000. 
    // The delay is used to determine if a frame was skipped, hence there should be some margin.
#ifdef __WINSCW__
    iMaxEncodingDelay = 5000000;    // emulator can be really slow, use 5 seconds timeout
#else    
    iMaxEncodingDelay = (TUint)(2*1000000*iTranscoder->EstimateTranscodeTimeFactorL(videoInputFormat,videoOutputFormat)/iFrameRate);
    iMaxEncodingDelay += 100000;  // Add 100 ms for safety margin
#endif
    
    TTRVideoCodingOptions codingOptions;
    codingOptions.iSyncIntervalInPicture = 0;
    codingOptions.iMinRandomAccessPeriodInSeconds = (TInt) (1.0 / iRandomAccessRate);
    
    // NOTE: What about these ???
    codingOptions.iDataPartitioning = EFalse;
    codingOptions.iReversibleVLC = EFalse;
    codingOptions.iHeaderExtension = 0;
   
    iTranscoder->SetVideoCodingOptionsL(codingOptions);
   
    iTranscoder->SetVideoPictureSinkOptionsL(iFrameSize, this);

    }


// -----------------------------------------------------------------------------
// CVideoEncoder::MtroSetInputFrameRate
// Sets input sequence frame rate for encoding
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CVideoEncoder::MtroSetInputFrameRate(TReal& aRate)
    {
     
    aRate = iInputFrameRate;
        
    }


// -----------------------------------------------------------------------------
// CVideoEncoder::MtroPictureFromTranscoder
// Called by transcoder to return the input picture buffer
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CVideoEncoder::MtroPictureFromTranscoder(TTRVideoPicture* aPicture)
    {
    PRINT((_L("CVideoEncoder::MtroPictureFromTranscoder() %x"), aPicture));
    
    if (iInputPicture != aPicture)
    {
        iMonitor->Error(KErrGeneral);
        return;
    }
        
    if (iEncodePending)
        {                
        if (!iTimerRequestPending)
            {
            // Activate timer to wait for encoding, if the encoding didn't complete already. 
            // Timeout is needed since we don't get notification if frame was skipped.
            SetActive();
            iStatus = KRequestPending;        
            iTimer.After(iStatus, TTimeIntervalMicroSeconds32(iMaxEncodingDelay));
            iTimerRequestPending = ETrue;
            }
        }
    }


// -----------------------------------------------------------------------------
// CVideoEncoder::WriteBufferL
// Called by transcoder to notify that new bitsream buffer is ready
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CVideoEncoder::WriteBufferL(CCMRMediaBuffer* aBuffer)
    {
    
    PRINT((_L("CVideoEncoder::WriteBufferL(), iEncodePending = %d"), iEncodePending));
    
#ifdef _DEBUG
    TTime current;
    current.UniversalTime();
    TInt64 time = current.MicroSecondsFrom(iEncodeStartTime).Int64();
	PRINT((_L("CVideoEncoder::WriteBufferL(), time spent encoding = %d ms, iEncodeRequestStatus = 0x%x"), 
		(I64INT(time))/1000 ,iEncodeRequestStatus));

#endif

    if (aBuffer->Type() == CCMRMediaBuffer::EVideoMPEG4DecSpecInfo)
	{
		// copy data to bitstream buffer
	    TPtr8 ptr = iDataBuffer->Des();		    
	    ptr.Copy( aBuffer->Data() );	    
	    return;		
	}

    // Cancel timer
    Cancel();

	iEncodePending = EFalse;	
	
	if (aBuffer->BufferSize() == 0)
	{
	    PRINT((_L("CVideoEncoder::WriteBufferL() buffer length == 0")));
	    
    	if ( iEncodeRequestStatus != 0 )
        {
    		// complete request
    		User::RequestComplete(iEncodeRequestStatus, KErrUnderflow);
    		iEncodeRequestStatus = 0;
    	}	    
	    return;
	}
		
	VEASSERT(aBuffer->Data().Length() <= KMaxCodedPictureSizeVGA);		    
	
#ifdef VIDEOEDITORENGINE_AVC_EDITING
    TInt avcFrameLen = 0;
    if ( iMimeType.FindF(KVedMimeTypeAVC) != KErrNotFound )
    {           
       // get avc frame length
       TUint8* tmp = const_cast<TUint8*>(aBuffer->Data().Ptr() + aBuffer->Data().Length());
       // support for 1 frame in 1 NAL unit currently
       tmp -= 4;
       TInt numNalUnits = TInt(tmp[0]) + (TInt(tmp[1])<<8) + (TInt(tmp[2])<<16) + (TInt(tmp[3])<<24);
       if (numNalUnits > 0) { }
       
       VEASSERT( numNalUnits == 1 );       
       tmp -=4;
              
       avcFrameLen = tmp[0] + (TInt(tmp[1])<<8) + (TInt(tmp[2])<<16) + (TInt(tmp[3])<<24);                     
       
       TInt error = KErrNone;
       // check that there is enough room for length field and the frame
       if ( iDataBuffer->Des().MaxLength() < (avcFrameLen + 4) )
           TRAP( error, iDataBuffer->ReAllocL(avcFrameLen + 4) );
           
       if (error != KErrNone)
       {
           iMonitor->Error(error);
           return;
       }
       
       TUint8* buf = const_cast<TUint8*>(iDataBuffer->Des().Ptr());
       
       // set length
       buf[0] = TUint8((avcFrameLen >> 24) & 0xff);
       buf[1] = TUint8((avcFrameLen >> 16) & 0xff);
       buf[2] = TUint8((avcFrameLen >> 8) & 0xff);
       buf[3] = TUint8(avcFrameLen & 0xff);
       
       iDataBuffer->Des().SetLength(4);
    }
#endif        
			
	// copy data to bitstream buffer
	TPtr8 ptr = iDataBuffer->Des();
	
#ifdef VIDEOEDITORENGINE_AVC_EDITING
	if ( iMimeType.FindF(KVedMimeTypeAVC) != KErrNotFound )
	{
	    ptr.Append( aBuffer->Data().Ptr(), avcFrameLen );
	}
	else
#endif
    {
        ptr.Append( aBuffer->Data() );
    }
	
	// save keyframe flag
	iKeyFrame = aBuffer->RandomAccessPoint();
			
	if ( iEncodeRequestStatus != 0 )
    {
		// complete request
		User::RequestComplete(iEncodeRequestStatus, KErrNone);
		iEncodeRequestStatus = 0;
	}

    // --> user calls GetBuffer();	    
    
    }
    
TInt CVideoEncoder::SetVideoFrameSize(TSize /*aSize*/)
    {
    return KErrNone;    
    }
    
TInt CVideoEncoder::SetAverageVideoBitRate(TInt /*aSize*/)
    {
    return KErrNone;    
    }
    
TInt CVideoEncoder::SetMaxVideoBitRate(TInt /*aSize*/)
    {
    return KErrNone;    
    }
    
TInt CVideoEncoder::SetAverageAudioBitRate(TInt /*aSize*/)
    {
    return KErrNone;    
    }

// -----------------------------------------------------------------------------
// CVideoEncoder::MtroFatalError
// Called by transcoder when a fatal error has occurred
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CVideoEncoder::MtroFatalError(TInt aError)
    {

    PRINT((_L("CVideoEncoder::MtroFatalError() %d"), aError));
    iFatalError = ETrue;

    iMonitor->Error(aError);
    
    }
    
void CVideoEncoder::MtroReturnCodedBuffer(CCMRMediaBuffer* /*aBuffer*/)
    {
        
        User::Panic(_L("CVIDEOENCODER"), EInternalAssertionFailure);
    }


// -----------------------------------------------------------------------------
// CVideoEncoder::MdvroInitializeComplete
// Called by transcoder when initialization is complete
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CVideoEncoder::MtroInitializeComplete(TInt aError)
    {
    PRINT((_L("CVideoEncoder::MtroInitializeComplete %d, iResetRequestStatus %x"), aError, iResetRequestStatus));

    VEASSERT(iResetRequestStatus);

    if ( aError != KErrNone )    
    {        
        iMonitor->Error(aError);
        if ( iResetRequestStatus != 0 )
        {            
            User::RequestComplete(iResetRequestStatus, aError);
            iResetRequestStatus = 0;
        }
        return;
    }
    
    TInt error = KErrNone;    
    // Handle MPEG-4 VOL / AVC SPS/PPS data reading/writing            
    TRAP(error, HandleCodingStandardSpecificInfoL());
    
    if ( error != KErrNone )
        iMonitor->Error(error); 

	if ( iResetRequestStatus != 0 )
    {		
		PRINT((_L("CVideoEncoder::MtroInitializeComplete() complete request")));
		// complete request:
        User::RequestComplete(iResetRequestStatus, error);
		iResetRequestStatus = 0;
	}
    
    }

// -----------------------------------------------------------------------------
// CVideoEncoder::MtroAsyncStopComplete
// Called by transcoder when all pictures have been processed
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CVideoEncoder::MtroAsyncStopComplete()
    {
    
    PRINT((_L("CVideoEncoder::MtroAsyncStopComplete()")));
    
    if ( iResetRequestStatus != 0 )
    {
		PRINT((_L("CVideoEncoder::MdvroStreamEnd() complete request")));
        // complete request
        User::RequestComplete(iResetRequestStatus, KErrNone);
        iResetRequestStatus = 0;
    }
       
    }

// -----------------------------------------------------------------------------
// CVideoEncoder::GetTimeIncrementResolution
// Gets time increment resolution used in MPEG-4 
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CVideoEncoder::GetTimeIncrementResolution() const
    {
    if ( iVolReader->TimeIncrementResolution() != 0 )
        return iVolReader->TimeIncrementResolution();
    
    return iTimeIncrementResolution;
    }
    
// -----------------------------------------------------------------------------
// CVideoEncoder::HandleCodingStandardSpecificInfoL
// Parses the MPEG-4 VOL header / AVC Dec. configuration record
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CVideoEncoder::HandleCodingStandardSpecificInfoL()
    {

    // Parse the VOL header for mpeg-4
    if ( iMimeType.FindF(KVedMimeTypeMPEG4Visual) != KErrNotFound )
        {
        HBufC8* headerData = iTranscoder->GetCodingStandardSpecificInitOutputLC();
        if ( headerData != 0 )
            {
            iVolReader->ParseVolHeaderL( (TDesC8&) *headerData );
            
            // Insert VOL header to bitstream buffer
	        TPtr8 ptr = iDataBuffer->Des();		
	        ptr.Append( *headerData );
            
            CleanupStack::PopAndDestroy( headerData );
            }
        }

#ifdef VIDEOEDITORENGINE_AVC_EDITING         
    // Parse & write SPS/PPS data for AVC
    else if ( iMimeType.FindF(KVedMimeTypeAVC) != KErrNotFound )
        {
        HBufC8* headerData = iTranscoder->GetCodingStandardSpecificInitOutputLC();
        
        if ( headerData != 0 )
            {   
            
            HBufC8* outputAVCHeader = 0;
            // is the max. size of AVCDecoderConfigurationRecord known here ??
            outputAVCHeader = (HBufC8*) HBufC8::NewLC(16384);            
            TPtr8 ptr = outputAVCHeader->Des();
	            
	        // parse header & convert it to AVCDecoderConfigurationRecord -format
	        iAvcEdit->ConvertAVCHeaderL(*headerData, ptr);	            
	        
	        // save it to avc module for later use
	        iAvcEdit->SaveAVCDecoderConfigurationRecordL(ptr, ETrue /* aFromEncoder */); 

            CleanupStack::PopAndDestroy( 2 );  // headerData, outputAVCHeader
            
            }
        }
#endif
        
    }
