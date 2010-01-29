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



#include "VeiImageClipGenerator.h"

#include <VedMovie.h>
#include <fbs.h>
#include <bitdev.h>
#include <gdi.h>
#include <aknutils.h>
#include <ImageConversion.h>
#include <BitmapTransforms.h>

#define KMiddleFrameDuration TTimeIntervalMicroSeconds(1000000)

const TInt KNumberOfTransitionFrames = 10;

EXPORT_C CVeiImageClipGenerator* CVeiImageClipGenerator::NewL(const TDesC& aFilename,
												     const TSize& aMaxResolution,
													 const TTimeIntervalMicroSeconds& aDuration, 
													 const TRgb& aBackgroundColor,
													 TDisplayMode aMaxDisplayMode,
													 RFs& aFs,
													 MVeiImageClipGeneratorObserver& aObserver)
	{
	CVeiImageClipGenerator* self = 
		CVeiImageClipGenerator::NewLC(aFilename, aMaxResolution, aDuration, aBackgroundColor, aMaxDisplayMode, aFs, aObserver);
	CleanupStack::Pop(self);
	return self;
	}


EXPORT_C CVeiImageClipGenerator* CVeiImageClipGenerator::NewLC(const TDesC& aFilename,
													  const TSize& aMaxResolution,
													  const TTimeIntervalMicroSeconds& aDuration,
													  const TRgb& aBackgroundColor,
													  TDisplayMode aMaxDisplayMode,
													  RFs& aFs,
													  MVeiImageClipGeneratorObserver& aObserver)
	{
	CVeiImageClipGenerator* self = new (ELeave) CVeiImageClipGenerator(aDuration, aBackgroundColor, aMaxResolution);
	CleanupStack::PushL(self);
	self->ConstructL(aFilename, aObserver, aMaxDisplayMode, aFs);
	return self;
	}


CVeiImageClipGenerator::CVeiImageClipGenerator(const TTimeIntervalMicroSeconds& aDuration, 
											   const TRgb& aBackgroundColor,
											   const TSize& aMaxResolution)
   : iReady(EFalse), iMaxResolution(aMaxResolution), iBackgroundColor(aBackgroundColor), iInitializing(ETrue)
	{
	__ASSERT_ALWAYS(iMaxResolution.iHeight >= 0, TVedPanic::Panic(TVedPanic::EImageClipGeneratorIllegalMaxResolution));
	__ASSERT_ALWAYS(iMaxResolution.iWidth >= 0, TVedPanic::Panic(TVedPanic::EImageClipGeneratorIllegalMaxResolution));

	SetDuration(aDuration);

	iInitializing = EFalse;
	}


void CVeiImageClipGenerator::ConstructL(const TDesC& aFilename, 
										MVeiImageClipGeneratorObserver& aObserver, 
										TDisplayMode aMaxDisplayMode, RFs& aFs)
	{
	iDecodeOperation = CVeiImageClipDecodeOperation::NewL(*this, aFilename, aObserver, aFs);
	iFrameOperation = CVeiImageClipFrameOperation::NewL(*this);
	
	iFilename = aFilename.AllocL();

	TParse parse;
	parse.Set(aFilename, 0, 0);
	iDescriptiveName = parse.Name().AllocL();

	iDecodeOperation->StartOperationL(iMaxResolution, aMaxDisplayMode);
	}


EXPORT_C CVeiImageClipGenerator::~CVeiImageClipGenerator()
	{
	delete iDecodeOperation;
	delete iFrameOperation;
	delete iDescriptiveName;
	delete iBitmap;
	delete iMask;
	delete iFilename;
	}


EXPORT_C TPtrC CVeiImageClipGenerator::DescriptiveName() const
	{
	__ASSERT_ALWAYS(iReady,
		TVedPanic::Panic(TVedPanic::EImageClipGeneratorNotReady));

	return *iDescriptiveName;
	}


EXPORT_C TUid CVeiImageClipGenerator::Uid() const
	{
	return KUidImageClipGenerator;
	}

EXPORT_C TTimeIntervalMicroSeconds CVeiImageClipGenerator::Duration() const
	{
	__ASSERT_ALWAYS(iReady,
		TVedPanic::Panic(TVedPanic::EImageClipGeneratorNotReady));

	return iDuration;
	}


EXPORT_C TInt CVeiImageClipGenerator::VideoFrameCount() const
	{
	__ASSERT_ALWAYS(iReady,
		TVedPanic::Panic(TVedPanic::EImageClipGeneratorNotReady));

	TInt frameCount = 0;

	TInt maxFramerate = 10;
	if (IsInserted()) 
		{
		maxFramerate = Movie()->MaximumFramerate();
		}

	TTimeIntervalMicroSeconds frameDuration(TInt64(1000000 / maxFramerate));
	if (iDuration.Int64() < (TInt64(KNumberOfTransitionFrames * 2 + 1) * frameDuration.Int64()))
		{
		frameCount = (static_cast<TInt>(iDuration.Int64() / frameDuration.Int64()));
		if ((iDuration.Int64() % frameDuration.Int64()) != 0)
			{
			frameCount++;
			}
		}
	else
		{
		frameCount = KNumberOfTransitionFrames * 2; 
		TTimeIntervalMicroSeconds middleTime(iDuration.Int64() - (TInt64(KNumberOfTransitionFrames * 2) * frameDuration.Int64()));
		frameCount += (static_cast<TInt32>(middleTime.Int64() / KMiddleFrameDuration.Int64()));
		if ((middleTime.Int64() % KMiddleFrameDuration.Int64()) != 0)
			{
			frameCount++;
			}
		}
	return frameCount;
	}


EXPORT_C TTimeIntervalMicroSeconds CVeiImageClipGenerator::VideoFrameStartTime(TInt aIndex) const
	{
	__ASSERT_ALWAYS(iReady,
		TVedPanic::Panic(TVedPanic::EImageClipGeneratorNotReady));
	__ASSERT_ALWAYS(aIndex >= 0 && aIndex < VideoFrameCount(), TVedPanic::Panic(TVedPanic::EVideoClipGeneratorIllegalVideoFrameIndex));
	
	TInt maxFramerate = 10;
	if (IsInserted()) 
		{
		maxFramerate = Movie()->MaximumFramerate();
		}

	TTimeIntervalMicroSeconds frameDuration(TInt64(1000000 / maxFramerate));
	TTimeIntervalMicroSeconds finalThreshold(iDuration.Int64() - TInt64(KNumberOfTransitionFrames) * frameDuration.Int64());
	TTimeIntervalMicroSeconds startThreshold(TInt64(KNumberOfTransitionFrames) * frameDuration.Int64());
	TTimeIntervalMicroSeconds startTime(-1);
	TInt frameCount = VideoFrameCount();


	if (frameCount < (KNumberOfTransitionFrames * 2 + 1))
		{		
		// Special case: less than KNumberOfTransitionFrames frames in the movie
		startTime = TTimeIntervalMicroSeconds(TInt64(aIndex) * frameDuration.Int64());
		}
	else if (aIndex < KNumberOfTransitionFrames) 
		{
		// Start frames
		startTime = TTimeIntervalMicroSeconds(TInt64(aIndex) * frameDuration.Int64());
		}
	else if (aIndex >= (frameCount - KNumberOfTransitionFrames))
		{
		// End frames
		startTime = TTimeIntervalMicroSeconds(
			static_cast<TInt64>((aIndex - frameCount) + KNumberOfTransitionFrames) 
			* frameDuration.Int64() + finalThreshold.Int64() );
		}
	else  
		{
		// Middle frames
		startTime = TTimeIntervalMicroSeconds(startThreshold.Int64() 
			+ TInt64(aIndex- KNumberOfTransitionFrames) * KMiddleFrameDuration.Int64());
		}

	return startTime;
	}


EXPORT_C TTimeIntervalMicroSeconds CVeiImageClipGenerator::VideoFrameEndTime(TInt aIndex) const
	{
	__ASSERT_ALWAYS(iReady,
		TVedPanic::Panic(TVedPanic::EImageClipGeneratorNotReady));
	__ASSERT_ALWAYS(aIndex >= 0 && aIndex < VideoFrameCount(), TVedPanic::Panic(TVedPanic::EVideoClipGeneratorIllegalVideoFrameIndex));
	
	if (aIndex == VideoFrameCount() - 1) 
		{
		return iDuration;
		}

	TInt maxFramerate = 10;
	if (IsInserted()) 
		{
		maxFramerate = Movie()->MaximumFramerate();
		}

	TTimeIntervalMicroSeconds frameDuration(TInt64(1000000 / maxFramerate));
	TTimeIntervalMicroSeconds finalThreshold(iDuration.Int64() - TInt64(KNumberOfTransitionFrames) * frameDuration.Int64());
	TTimeIntervalMicroSeconds startThreshold(TInt64(KNumberOfTransitionFrames) * frameDuration.Int64());
	TTimeIntervalMicroSeconds endTime(-1);


	TInt frameCount = VideoFrameCount();

	if (frameCount < (KNumberOfTransitionFrames * 2 + 1))
		{
		// Special case: less than KNumberOfTransitionFrames frames in the movie
		endTime = TTimeIntervalMicroSeconds(TInt64(aIndex + 1) * frameDuration.Int64());
		}
	else if (aIndex < KNumberOfTransitionFrames) 
		{
		// start frames
		endTime = TTimeIntervalMicroSeconds(TInt64(aIndex + 1) * frameDuration.Int64());
		}
	else if (aIndex > (frameCount - KNumberOfTransitionFrames))
		{
		// end frames
		endTime = TTimeIntervalMicroSeconds(TInt64((aIndex - frameCount) 
			+ KNumberOfTransitionFrames) * frameDuration.Int64() 
			+ finalThreshold.Int64());
		}
	else  
		{
		// middle frames
		endTime = TTimeIntervalMicroSeconds(startThreshold.Int64() 
			+ TInt64(aIndex - (KNumberOfTransitionFrames - 1)) 
			* KMiddleFrameDuration.Int64());

		if (endTime.Int64() >= finalThreshold.Int64())
			{
			// last of the middle frames may be shorter than normal
			endTime = finalThreshold;
			}
		}
	return endTime;
	}


EXPORT_C TTimeIntervalMicroSeconds CVeiImageClipGenerator::VideoFrameDuration(TInt aIndex) const
	{
	__ASSERT_ALWAYS(iReady,
		TVedPanic::Panic(TVedPanic::EImageClipGeneratorNotReady));
	__ASSERT_ALWAYS(aIndex >= 0 && aIndex < VideoFrameCount(), TVedPanic::Panic(TVedPanic::EVideoClipGeneratorIllegalVideoFrameIndex));


	// check maximum framerate
	TInt maxFramerate = 10;
	if (IsInserted()) 
		{
		maxFramerate = Movie()->MaximumFramerate();
		}

	// calculate some timing values. 
	TTimeIntervalMicroSeconds frameDuration(TInt64(1000000 / maxFramerate));
	TTimeIntervalMicroSeconds finalThreshold(iDuration.Int64() - TInt64(KNumberOfTransitionFrames) * frameDuration.Int64());
	TTimeIntervalMicroSeconds startThreshold(TInt64(KNumberOfTransitionFrames) * frameDuration.Int64());


	TInt frameCount = VideoFrameCount();
	TInt finalThresholdIndex = GetVideoFrameIndex(finalThreshold);

	if ((frameCount < (KNumberOfTransitionFrames * 2 + 1)) && (aIndex == (frameCount - 1)))
		{
		// Special case: short clip with only frames that have max framerate 
		// - all of the frames are of equal duration (frameDuration) except 
		// the last one.
		frameDuration = TTimeIntervalMicroSeconds(iDuration.Int64() - (TInt64(frameCount - 1) * frameDuration.Int64()));
		}
	else if (aIndex >= KNumberOfTransitionFrames && aIndex < finalThresholdIndex) 
		{
		if (aIndex == (finalThresholdIndex - 1)) 
			{
			// Last one of the middle frames
			frameDuration = TTimeIntervalMicroSeconds(finalThreshold.Int64() - VideoFrameStartTime(aIndex).Int64());
			}
		else
			{
			// Ordinary middle frame
			frameDuration = KMiddleFrameDuration;
			}
		}
	return frameDuration;
	}

EXPORT_C TBool CVeiImageClipGenerator::VideoFrameIsIntra(TInt aIndex) const
	{
	__ASSERT_ALWAYS(iReady,
		TVedPanic::Panic(TVedPanic::EImageClipGeneratorNotReady));
	if (aIndex == 0) 
		{
		return ETrue;
		}
	return EFalse;
	}

EXPORT_C TInt CVeiImageClipGenerator::VideoFirstFrameComplexityFactor() const
	{
	__ASSERT_ALWAYS(iReady,
		TVedPanic::Panic(TVedPanic::EImageClipGeneratorNotReady));
	return iFirstFrameComplexityFactor;
	}

EXPORT_C TInt CVeiImageClipGenerator::VideoFrameDifferenceFactor(TInt aIndex) const
	{
	__ASSERT_ALWAYS(iReady,
		TVedPanic::Panic(TVedPanic::EImageClipGeneratorNotReady));
	__ASSERT_ALWAYS(aIndex >= 0 && aIndex < VideoFrameCount(), TVedPanic::Panic(TVedPanic::EVideoClipGeneratorIllegalVideoFrameIndex));

	if (aIndex == 0) 
		{
		return 1000;
		}
	else 
		{
		return 0;
		}
	}


EXPORT_C TInt CVeiImageClipGenerator::GetVideoFrameIndex(TTimeIntervalMicroSeconds aTime) const
	{
	__ASSERT_ALWAYS(iReady,
		TVedPanic::Panic(TVedPanic::EImageClipGeneratorNotReady));
	__ASSERT_ALWAYS(aTime.Int64() >= 0, TVedPanic::Panic(TVedPanic::EVideoClipInfoIllegalVideoFrameTime));
	__ASSERT_ALWAYS(aTime.Int64() <= iDuration.Int64(), TVedPanic::Panic(TVedPanic::EVideoClipInfoIllegalVideoFrameTime));

	TInt index = -1;

	TInt maxFramerate = 10;
	if (IsInserted()) 
		{
		maxFramerate = Movie()->MaximumFramerate();
		}

	TTimeIntervalMicroSeconds frameDuration(TInt64(1000000 / maxFramerate));
	TTimeIntervalMicroSeconds finalThreshold(
		iDuration.Int64() - TInt64(KNumberOfTransitionFrames) * frameDuration.Int64());
	TTimeIntervalMicroSeconds startThreshold(
		TInt64(KNumberOfTransitionFrames) * frameDuration.Int64());

	if (iDuration <= (TInt64(KNumberOfTransitionFrames * 2) * frameDuration.Int64())) 
		{
		index = static_cast<TInt>(aTime.Int64() / frameDuration.Int64());
		}
	else if (aTime < startThreshold) 
		{
		index = static_cast<TInt>(aTime.Int64() / frameDuration.Int64());
		}
	else if (aTime >= finalThreshold) 
		{
		TTimeIntervalMicroSeconds middleDuration(finalThreshold.Int64() - startThreshold.Int64());
		TInt numberOfMiddleFrames = 
		    static_cast<TInt32>(middleDuration.Int64() / KMiddleFrameDuration.Int64());
		if (middleDuration.Int64() % KMiddleFrameDuration.Int64() != 0) 
			{
			numberOfMiddleFrames++;
			}

		index = KNumberOfTransitionFrames + numberOfMiddleFrames 
			+ static_cast<TInt>((aTime.Int64() - finalThreshold.Int64()) / frameDuration.Int64()); 
		}
	else  
		{
		index = KNumberOfTransitionFrames 
			+ static_cast<TInt>((aTime.Int64() - startThreshold.Int64()) / KMiddleFrameDuration.Int64());
		}
	
	return index;
	}


EXPORT_C void CVeiImageClipGenerator::GetFrameL(MVedVideoClipGeneratorFrameObserver& aObserver,
								  TInt aIndex, TSize* const aResolution,
								  TDisplayMode aDisplayMode, TBool aEnhance,
								  TInt aPriority)
	{
	__ASSERT_ALWAYS(iReady,
		TVedPanic::Panic(TVedPanic::EImageClipGeneratorNotReady));
	__ASSERT_ALWAYS((aIndex >= 0  && aIndex < VideoFrameCount()) || 
					 aIndex == KFrameIndexBestThumb, 
					TVedPanic::Panic(TVedPanic::EVideoClipGeneratorIllegalVideoFrameIndex));
	__ASSERT_ALWAYS((aResolution->iHeight <= iMaxResolution.iHeight &&
					 aResolution->iWidth  <= iMaxResolution.iWidth), 
					TVedPanic::Panic(TVedPanic::EVideoClipGeneratorIllegalFrameResolution));
	__ASSERT_ALWAYS((aResolution->iHeight >= 0 && aResolution->iWidth  >= 0), 
					TVedPanic::Panic(TVedPanic::EVideoClipGeneratorIllegalFrameResolution));
	

    TDisplayMode displayMode = aDisplayMode;

    // check validity of thumbnail and associated operation
    if(aEnhance)	// for saving to file
        {
        if(displayMode == ENone)					// if no preference
			{
            displayMode = EColor16M;				// 24-bit color image for enhancement
			}
        else if(displayMode != EColor16M)	// invalid combination
			{
            User::Leave(KErrNotSupported);
			}
        }
    else								// for screen display
        {
        if(displayMode == ENone)					// if no preference
			{
            displayMode = EColor64K;				// 16-bit image 
			}
        }
    
    CFbsBitmap* destBitmap = new (ELeave) CFbsBitmap;
	CleanupStack::PushL(destBitmap);
	User::LeaveIfError(destBitmap->Create(*aResolution, displayMode));
	CleanupStack::Pop(destBitmap);

	iFrameOperation->StartOperationL(&aObserver, aIndex, aEnhance, iBitmap,
		destBitmap, iMask, aPriority);
	}


EXPORT_C void CVeiImageClipGenerator::CancelFrame()
	{
	iFrameOperation->Cancel();
	}

	
EXPORT_C void CVeiImageClipGenerator::SetDuration(const TTimeIntervalMicroSeconds& aDuration)
	{
	__ASSERT_ALWAYS(iReady || iInitializing,
		TVedPanic::Panic(TVedPanic::EImageClipGeneratorNotReady));	
	__ASSERT_ALWAYS(aDuration.Int64() > 0, 
		TVedPanic::Panic(TVedPanic::EVideoClipGeneratorIllegalDuration));

	iDuration = aDuration;

	if (!iInitializing) 
		{
		ReportDurationChanged();
		}
	}


EXPORT_C void CVeiImageClipGenerator::SetBackgroundColor(const TRgb& aBackgroundColor)
	{
	__ASSERT_ALWAYS(iReady,
		TVedPanic::Panic(TVedPanic::EImageClipGeneratorNotReady));	

	iBackgroundColor = aBackgroundColor;
	ReportSettingsChanged();
	}

EXPORT_C const TRgb& CVeiImageClipGenerator::BackgroundColor() const
	{
	__ASSERT_ALWAYS(iReady,
		TVedPanic::Panic(TVedPanic::EImageClipGeneratorNotReady));	

	return iBackgroundColor;
	}

EXPORT_C TPtrC CVeiImageClipGenerator::ImageFilename() const
	{
	__ASSERT_ALWAYS(iReady,
		TVedPanic::Panic(TVedPanic::EImageClipGeneratorNotReady));	
	return *iFilename;
	}

void CVeiImageClipGenerator::UpdateFirstFrameComplexityFactorL()
	{
	iFirstFrameComplexityFactor = CalculateFrameComplexityFactor(iBitmap);
	}

//////////////////////////////////////////////////////////////////////////
//  Decode operation
//////////////////////////////////////////////////////////////////////////


CVeiImageClipDecodeOperation* CVeiImageClipDecodeOperation::NewL(CVeiImageClipGenerator& aGenerator, 
																 const TDesC& aFilename, 
																 MVeiImageClipGeneratorObserver& aObserver,
																 RFs& aFs,
																 TInt aPriority)
	{
    CVeiImageClipDecodeOperation* self = 
		new (ELeave) CVeiImageClipDecodeOperation(aGenerator, aObserver, aPriority);
    CleanupStack::PushL(self);
    self->ConstructL(aFilename, aFs);
    CleanupStack::Pop(self);
    return self;	
	}


CVeiImageClipDecodeOperation::CVeiImageClipDecodeOperation(CVeiImageClipGenerator& aGenerator, 
														   MVeiImageClipGeneratorObserver& aObserver,
														   TInt aPriority)
  : CActive(aPriority), iGenerator(aGenerator), iObserver(aObserver)
	{
	CActiveScheduler::Add(this);
	}


void CVeiImageClipDecodeOperation::ConstructL(const TDesC& aFilename, RFs& aFs)
	{
	iDecoder = CImageDecoder::FileNewL(aFs, aFilename);
	}


CVeiImageClipDecodeOperation::~CVeiImageClipDecodeOperation()
	{
	Cancel();

	delete iDecoder;
	iDecoder = 0;
	delete iBitmap;
	iBitmap = 0;
	delete iMask;
	iMask = 0;
	}


void CVeiImageClipDecodeOperation::DoCancel()
	{
	if (iDecoder) 
		{
		iDecoder->Cancel();
		}

	delete iDecoder;
	iDecoder = 0;

	delete iBitmap;
	iBitmap = 0;

	delete iMask;
	iMask = 0;

	iObserver.NotifyImageClipGeneratorInitializationComplete(iGenerator, KErrCancel);
	}


void CVeiImageClipDecodeOperation::RunL()
	{
	/* Transfer ownership of iBitmap to generator. */
	iGenerator.iBitmap = iBitmap;
	iBitmap = 0;
	iGenerator.iMask = iMask;
	iMask = 0;
	iGenerator.iReady = ETrue;
	iGenerator.UpdateFirstFrameComplexityFactorL();

	/* Notify observer. */
	iObserver.NotifyImageClipGeneratorInitializationComplete(iGenerator, KErrNone);
	delete iDecoder;
	iDecoder = 0;
	}

TInt CVeiImageClipDecodeOperation::RunError(TInt aError)
	{
	if (iDecoder) 
		{
		iDecoder->Cancel();
		}
	delete iDecoder;
	iDecoder = 0;
	delete iBitmap;
	iBitmap = 0;
	delete iMask;
	iMask = 0;

	iObserver.NotifyImageClipGeneratorInitializationComplete(iGenerator, aError);
	return KErrNone;
	}



void CVeiImageClipDecodeOperation::StartOperationL(const TSize& aMaxResolution, TDisplayMode aDisplayMode)
	{
	__ASSERT_ALWAYS(!IsActive(), TVedPanic::Panic(TVedPanic::EInternal));

	const TFrameInfo& info = iDecoder->FrameInfo();
	TSize targetResolution(0, 0);
	const TSize sourceResolution(info.iOverallSizeInPixels);

	/* Calculate resolution. */

	if ((sourceResolution.iWidth <= aMaxResolution.iWidth) 
		&& (sourceResolution.iHeight <= aMaxResolution.iHeight))
		{
		targetResolution.iWidth = sourceResolution.iWidth;
		targetResolution.iHeight = sourceResolution.iHeight;
		}
	else if (info.iFlags & TFrameInfo::EFullyScaleable) 
		{
		if ((sourceResolution.iWidth * aMaxResolution.iHeight) > 
			(sourceResolution.iHeight * aMaxResolution.iWidth))
			{
			targetResolution.iWidth = aMaxResolution.iWidth;
			targetResolution.iHeight = 
				(targetResolution.iWidth * sourceResolution.iHeight) / sourceResolution.iWidth;
			}
		else
			{
			targetResolution.iHeight = aMaxResolution.iHeight;
			targetResolution.iWidth = 
				(targetResolution.iHeight * sourceResolution.iWidth) / sourceResolution.iHeight;
			}
		}
	else 
		{
		targetResolution.iWidth = (sourceResolution.iWidth / 8) + 1;
		targetResolution.iHeight = (sourceResolution.iHeight / 8) + 1;
		
		if ((targetResolution.iWidth < aMaxResolution.iWidth) 
			&& (targetResolution.iHeight < aMaxResolution.iHeight))
			{
			targetResolution.iWidth = (sourceResolution.iWidth / 4) + 1;
			targetResolution.iHeight = (sourceResolution.iHeight / 4) + 1;
			}

		if ((targetResolution.iWidth < aMaxResolution.iWidth) 
			&& (targetResolution.iHeight < aMaxResolution.iHeight))
			{
			targetResolution.iWidth = (sourceResolution.iWidth / 2) + 1;
			targetResolution.iHeight = (sourceResolution.iHeight / 2) + 1;
			}

		if ((targetResolution.iWidth < aMaxResolution.iWidth) 
			&& (targetResolution.iHeight < aMaxResolution.iHeight))
			{
			targetResolution.iWidth = (sourceResolution.iWidth);
			targetResolution.iHeight = (sourceResolution.iHeight);
			}
		}


	iBitmap = new (ELeave) CFbsBitmap;
	TInt err = iBitmap->Create(targetResolution, aDisplayMode);

	if (err != KErrNone) 
		{
		delete iBitmap;
		iBitmap = 0;
		iObserver.NotifyImageClipGeneratorInitializationComplete(iGenerator, err);
		return;
		}

	if (info.iFlags & TFrameInfo::ETransparencyPossible)
		{
		iMask = new (ELeave) CFbsBitmap;
		if (info.iFlags & TFrameInfo::EAlphaChannel) 
			{
			err = iMask->Create(targetResolution, EGray256);
			}
		else 
			{
			err = iMask->Create(targetResolution, EGray2);
			}
		}
	
	if (err != KErrNone) 
		{
		delete iBitmap;
		iBitmap = 0;
		delete iMask;
		iMask = 0;
		iObserver.NotifyImageClipGeneratorInitializationComplete(iGenerator, err);
		return;
		}

	if (iMask != 0)
		{
		iDecoder->Convert(&iStatus, *iBitmap, *iMask);
		}
	else
		{
		iDecoder->Convert(&iStatus, *iBitmap);
		}

	SetActive();
	}


//////////////////////////////////////////////////////////////////////////
// Frame operation
//////////////////////////////////////////////////////////////////////////

CVeiImageClipFrameOperation* CVeiImageClipFrameOperation::NewL(CVeiImageClipGenerator& aGenerator)
    {
    CVeiImageClipFrameOperation* self = 
		new (ELeave) CVeiImageClipFrameOperation(aGenerator);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }


CVeiImageClipFrameOperation::CVeiImageClipFrameOperation(CVeiImageClipGenerator& aGenerator)
		: CActive(EPriorityStandard), iGenerator(aGenerator)
    {
	CActiveScheduler::Add(this);
	}


void CVeiImageClipFrameOperation::ConstructL()
    {
	}


CVeiImageClipFrameOperation::~CVeiImageClipFrameOperation()
    {
	Cancel();
	delete iScaler;
	iScaler = 0;
	delete iDestBitmap;
	iDestBitmap = 0;
	delete iScaledBitmap;
	iScaledBitmap = 0;
	delete iScaledMask;
	iScaledMask = 0;

	iSourceBitmap = 0;
	iSourceMask = 0;
	iObserver = 0;
    }


void CVeiImageClipFrameOperation::StartOperationL(MVedVideoClipGeneratorFrameObserver* aObserver,
												 TInt aIndex, TBool aEnhance, 
												 CFbsBitmap* aSourceBitmap, CFbsBitmap* aDestBitmap, 
												 CFbsBitmap* aSourceMask, TInt aPriority)
	{
	__ASSERT_ALWAYS(!IsActive(), TVedPanic::Panic(TVedPanic::EImageClipGeneratorFrameOperationAlreadyRunning));

	iObserver = aObserver;
	iSourceBitmap = aSourceBitmap;
	iDestBitmap = aDestBitmap;
	iSourceMask = aSourceMask;
	iIndex = aIndex;
	iEnhance = aEnhance;

	SetPriority(aPriority);


	TSize sourceRes = iSourceBitmap->SizeInPixels();
	TSize destRes = iDestBitmap->SizeInPixels();
	TSize movieRes = iGenerator.Movie()->Resolution();

	TSize imageResInMovie(0,0);
	if ((sourceRes.iWidth >= movieRes.iWidth) || (sourceRes.iHeight >= movieRes.iHeight)) 
		{
		// Downscaling
		if ((sourceRes.iWidth * movieRes.iHeight) > 
			(sourceRes.iHeight * movieRes.iWidth))
			{
			imageResInMovie.iWidth = movieRes.iWidth;
			imageResInMovie.iHeight =
				(movieRes.iWidth * sourceRes.iHeight) / sourceRes.iWidth;
			}
		else 
			{
			imageResInMovie.iHeight = movieRes.iHeight;
			imageResInMovie.iWidth = 
				(movieRes.iHeight * sourceRes.iWidth) / sourceRes.iHeight;
			}
		}
	else
		{
		// Upscaling - limit to a factor of two
		if ((sourceRes.iWidth * movieRes.iHeight) > 
			(sourceRes.iHeight * movieRes.iWidth))
			{
			imageResInMovie.iWidth = Min(movieRes.iWidth, (sourceRes.iWidth * 2));
			imageResInMovie.iHeight = (imageResInMovie.iWidth * sourceRes.iHeight) / sourceRes.iWidth;
			}
		else 
			{
			imageResInMovie.iHeight = Min((sourceRes.iHeight * 2), movieRes.iHeight);
			imageResInMovie.iWidth = (imageResInMovie.iHeight * sourceRes.iWidth) / sourceRes.iHeight;
			}
		}

	TSize movieResInDestBitmap(-1,-1);
	if ((movieRes.iWidth * destRes.iHeight) > 
		(movieRes.iHeight * destRes.iWidth))
		{
		movieResInDestBitmap.iWidth = destRes.iWidth;
		movieResInDestBitmap.iHeight =
			(movieResInDestBitmap.iWidth * movieRes.iHeight) / movieRes.iWidth;
		}
	else 
		{
		movieResInDestBitmap.iHeight = destRes.iHeight;
		movieResInDestBitmap.iWidth = 
			(movieResInDestBitmap.iHeight * movieRes.iWidth) / movieRes.iHeight;
		}
	

	TSize targetRes(imageResInMovie);
	targetRes.iWidth = imageResInMovie.iWidth * movieResInDestBitmap.iWidth / movieRes.iWidth;
	targetRes.iHeight = imageResInMovie.iHeight * movieResInDestBitmap.iHeight / movieRes.iHeight;

	TSize cachedRes(-1, -1);
	if (iScaledBitmap) 
		{
		cachedRes = iScaledBitmap->SizeInPixels();
		}

	/* Check if we already have scaled this bitmap.*/
	if ((cachedRes.iWidth == targetRes.iWidth) || 
		(cachedRes.iHeight == targetRes.iHeight))
		{
		SetActive();
		TRequestStatus* status = &iStatus;
		User::RequestComplete(status, KErrNone);
		return;
		}
	else if (iScaledBitmap) 
		{
		delete iScaledBitmap;
		iScaledBitmap = 0;
		delete iScaledMask;
		iScaledMask = 0;
		}

	delete iScaler;
	iScaler = NULL;
	iScaler = CBitmapScaler::NewL();

	iScaledBitmap = new (ELeave) CFbsBitmap;
	User::LeaveIfError(iScaledBitmap->Create(targetRes, iDestBitmap->DisplayMode()));
	iScaler->Scale(&iStatus, *iSourceBitmap, *iScaledBitmap, ETrue);
	SetActive();
	}



void CVeiImageClipFrameOperation::RunL()
	{
	if (!iNoScaling && iSourceMask && !iScaledMask) 
		{
		/* Scale the mask. */
		iScaledMask = new (ELeave) CFbsBitmap;
		User::LeaveIfError(iScaledMask->Create(iScaledBitmap->SizeInPixels(), iSourceMask->DisplayMode()));
		iScaler->Scale(&iStatus, *iSourceMask, *iScaledMask, ETrue);
		SetActive();
		return;
		}

	/* Select source. */
	CFbsBitmap* bitmap = 0;
	CFbsBitmap* mask = 0;

	if (iScaledBitmap) 
		{
		bitmap = iScaledBitmap;
		}
	else
		{
		bitmap = iSourceBitmap;
		}

	if (iScaledMask) 
		{
		mask = iScaledMask;
		}
	else
		{
		mask = iSourceMask;
		}


	/* Initialize context. */
	CFbsDevice* device = CFbsBitmapDevice::NewL(iDestBitmap);
	CleanupStack::PushL(device);
	CFbsBitGc* gc = NULL;
	User::LeaveIfError(device->CreateContext(gc));

	/* Calculate source point. */
	TSize destRes = iDestBitmap->SizeInPixels();
	TSize sourceRes = bitmap->SizeInPixels();	
	TPoint sourcePoint((destRes.iWidth - sourceRes.iWidth) / 2,
						(destRes.iHeight - sourceRes.iHeight) / 2);

	/* Draw background (this is relevant for scaled images and transparency). */
	gc->SetBrushColor(iGenerator.BackgroundColor());
	gc->SetBrushStyle(CGraphicsContext::ESolidBrush);
	gc->DrawRect(TRect(TPoint(0, 0), destRes));

	if (mask) 
		{
		TRect sourceRect(bitmap->SizeInPixels());
		gc->BitBltMasked(sourcePoint, bitmap, sourceRect, mask, EFalse);
		}
	else
		{
		gc->BitBlt(sourcePoint, bitmap);
		}

	delete gc;
	CleanupStack::PopAndDestroy(device);

	/* This transfers the bitmap ownership to the observer. */
	iObserver->NotifyVideoClipGeneratorFrameCompleted(iGenerator, KErrNone, iDestBitmap);

	delete iScaler;
	iScaler = 0;

	iSourceBitmap = 0;
	iSourceMask = 0;
	iDestBitmap = 0;
	iObserver = 0;
	iIndex = -1;
	iNoScaling = EFalse;
	}

TInt CVeiImageClipFrameOperation::RunError(TInt aError)
	{
	iObserver->NotifyVideoClipGeneratorFrameCompleted(iGenerator, aError, NULL);

	if (iScaler) 
		{
		iScaler->Cancel();
		}
	
	delete iScaler;
	iScaler = 0;
	delete iScaledBitmap;
	iScaledBitmap = 0;
	delete iScaledMask;
	iScaledMask = 0;
	delete iDestBitmap;
	iDestBitmap = 0;
	iSourceBitmap = 0;
	iSourceMask = 0;
	iObserver = 0;
	iIndex = -1;

	return KErrNone;
	}


void CVeiImageClipFrameOperation::DoCancel()
	{
	iObserver->NotifyVideoClipGeneratorFrameCompleted(iGenerator, KErrCancel, 0);

	if (iScaler) 
		{
		iScaler->Cancel();
		}
	
	delete iScaler;
	iScaler = 0;
	delete iScaledBitmap;
	iScaledBitmap = 0;
	delete iScaledMask;
	iScaledMask = 0;

	delete iDestBitmap;
	iDestBitmap = 0;
	iSourceBitmap = 0;
	iSourceMask = 0;
	iObserver = 0;
	iIndex = -1;
	}
// End of File

