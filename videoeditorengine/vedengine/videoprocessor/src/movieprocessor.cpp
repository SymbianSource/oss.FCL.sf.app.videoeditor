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
* Implementation of the movie processor
* interface class.                
*
*/


//  EXTERNAL RESOURCES

//  Include Files

#include "movieprocessorimpl.h"
#include "VedMovie.h"
#include "movieprocessor.h"
#include "VideoProcessorAudioData.h"


// -----------------------------------------------------------------------------
// CMovieProcessor::NewL
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CMovieProcessor* CMovieProcessor::NewL()    

	{
	CMovieProcessor* self = NewLC();
	CleanupStack::Pop(self);
	return self;
	}

CMovieProcessor* CMovieProcessor::NewLC()
	{
	CMovieProcessor* self = new (ELeave) CMovieProcessor();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
	}

// -----------------------------------------------------------------------------
// CMovieProcessor::CMovieProcessor()
// C++ default constructor can NOT contain any code, that
// might leave.
// -----------------------------------------------------------------------------
//
CMovieProcessor::CMovieProcessor()
	{
	iMovieProcessor=0;
	}

// -----------------------------------------------------------------------------
// CMovieProcessor::ConstructL
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//
void CMovieProcessor::ConstructL()
	{
	iMovieProcessor = CMovieProcessorImpl::NewL();
	}

// -----------------------------------------------------------------------------
// CMovieProcessor::~CMovieProcessor
// Destructor.
// -----------------------------------------------------------------------------
//
CMovieProcessor::~CMovieProcessor()
{
    iAudioDataArray.ResetAndDestroy();
	if(iMovieProcessor)
	{
		delete iMovieProcessor; 
		iMovieProcessor=0;
	}
}

// -----------------------------------------------------------------------------
// CMovieProcessor::StartMovieL
// Prepares the processor for processing a movie and starts processing
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessor::StartMovieL(CVedMovieImp* aMovie, const TDesC& aFileName, 
                                  RFile* aFileHandle,MVedMovieProcessingObserver* aObserver)
	{    
	iMovieProcessor->StartMovieL(aMovie, aFileName, aFileHandle, aObserver); 
	}


// -----------------------------------------------------------------------------
// CMovieProcessor::GetVideoClipPropertiesL
// Retrieves information about the given clip
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessor::GetVideoClipPropertiesL(const TDesC& aFileName,
                                              RFile* aFileHandle,
											  TVedVideoFormat& aFormat,
											  TVedVideoType& aVideoType, 
											  TSize& aResolution,
											  TVedAudioType& aAudioType, 
											  TTimeIntervalMicroSeconds& aDuration,
											  TInt& aVideoFrameCount,
											  TInt& aSamplingRate, 
											  TVedAudioChannelMode& aChannelMode)
	{	

	iMovieProcessor->GetClipPropertiesL(aFileName, aFileHandle, aFormat, aVideoType, 
		aResolution, aAudioType,  aDuration, aVideoFrameCount, aSamplingRate, aChannelMode);

	return; 

	}

// -----------------------------------------------------------------------------
// CMovieProcessor::GenerateVideoFrameInfoArrayL
// Retrieves frames parameters for a clip to array
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessor::GenerateVideoFrameInfoArrayL(const TDesC& aFileName, RFile* aFileHandle, TVedVideoFrameInfo*& aVideoFrameInfoArray)
    {
    iMovieProcessor->GenerateVideoFrameInfoArrayL((const TDesC&)aFileName, aFileHandle,(TVedVideoFrameInfo*&)aVideoFrameInfoArray);
    return;
    }

// -----------------------------------------------------------------------------
// CMovieProcessor::GetMovieSizeEstimateL
// Calculates file size estimate for the output file
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMovieProcessor::GetMovieSizeEstimateL(const CVedMovie* aMovie)
	{
	return iMovieProcessor->GetMovieSizeEstimateL(aMovie); 
	}

// -----------------------------------------------------------------------------
// CMovieProcessor::GetMovieSizeEstimateForMMSL
// Calculates file size estimate for the output file
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMovieProcessor::GetMovieSizeEstimateForMMSL(const CVedMovie* aMovie, TInt aTargetSize, 
												  TTimeIntervalMicroSeconds aStartTime, 
												  TTimeIntervalMicroSeconds& aEndTime)
	{
	return iMovieProcessor->GetMovieSizeEstimateForMMSL(aMovie, aTargetSize, aStartTime, aEndTime); 
	}


// -----------------------------------------------------------------------------
// CMovieProcessor::StartThumbL
// Initiates thumbnail generation
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessor::StartThumbL(const TDesC& aFileName, RFile* aFileHandle, TInt aIndex, 
                                  TSize aResolution, TDisplayMode aDisplayMode, TBool aEnhance)
	{
	iMovieProcessor->StartThumbL(aFileName, aFileHandle, aIndex, aResolution, aDisplayMode, aEnhance); 
	}


// -----------------------------------------------------------------------------
// CMovieProcessor::ProcessThumbL
// Starts thumbnail image generation
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//	
void CMovieProcessor::ProcessThumbL(TRequestStatus &aStatus, TVedTranscodeFactor* aFactor)
{
    iMovieProcessor->ProcessThumbL(aStatus, aFactor);
}

// -----------------------------------------------------------------------------
// CMovieProcessor::FetchThumb
// Gets a pointer to completed thumbnail bitmap
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//	
void CMovieProcessor::FetchThumb(CFbsBitmap*& aThumb)
{
    iMovieProcessor->FetchThumb(aThumb);   
}


// -----------------------------------------------------------------------------
// CMovieProcessor::CancelProcessingL
// Stops processing the movie
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessor::CancelProcessingL()
{
    iMovieProcessor->CancelProcessingL();
}

// -----------------------------------------------------------------------------
// CMovieProcessor::SetMovieSizeLimit
// Sets the maximum size for the movie
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//	
void CMovieProcessor::SetMovieSizeLimit(TInt aLimit)
    {
    iMovieProcessor->SetMovieSizeLimit(aLimit);
    }
	
