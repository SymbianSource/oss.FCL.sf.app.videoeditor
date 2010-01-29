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



#include "VeiTitleClipGenerator.h"
#include "VideoEditorCommon.h"
#include "VideoEditorDebugUtils.h"

#include <VedMovie.h>
#include <fbs.h>
#include <bitdev.h>
#include <gdi.h>
#include <aknutils.h>
#include <ImageConversion.h>
#include <BitmapTransforms.h>
#include <aknbiditextutils.h>

#include <AknFontAccess.h>
#include <AknFontSpecification.h>
#include <AknLayoutFont.h>

#include "VideoEditorDebugUtils.h"

const TInt KDefaultNumberOfLines = 15;
const TInt KLineSpacing = 4;
const TInt KMargin = 10;
const TInt KVeiTitleClipDefaultDuration = 5000000;
const TInt KVeiTitleClipFadeLimit = 2000000;
// Restrict the maximum frame rate to 15fps regardless of the movie properties
const TInt KMaxFrameRate = 15;


EXPORT_C CVeiTitleClipGenerator* CVeiTitleClipGenerator::NewL(const TSize& aMaxResolution,
	TVeiTitleClipTransition aTransition, TVeiTitleClipHorizontalAlignment aHorizontalAlignment,
		TVeiTitleClipVerticalAlignment aVerticalAlignment)
	{
	LOGFMT2(KVideoEditorLogFile, "CVeiTitleClipGenerator::NewL: aMaxResolution: (%d,%d)", aMaxResolution.iWidth,aMaxResolution.iHeight);

	__ASSERT_ALWAYS(aMaxResolution.iHeight >= 0, TVedPanic::Panic(TVedPanic::ETitleClipGeneratorIllegalMaxResolution));
	__ASSERT_ALWAYS(aMaxResolution.iWidth >= 0, TVedPanic::Panic(TVedPanic::ETitleClipGeneratorIllegalMaxResolution));

	CVeiTitleClipGenerator* self = CVeiTitleClipGenerator::NewLC(aMaxResolution, aTransition, aHorizontalAlignment, aVerticalAlignment);
	CleanupStack::Pop(self);
	return self;
	}

EXPORT_C CVeiTitleClipGenerator* CVeiTitleClipGenerator::NewLC(const TSize& aMaxResolution, 
	TVeiTitleClipTransition aTransition, TVeiTitleClipHorizontalAlignment aHorizontalAlignment,
		TVeiTitleClipVerticalAlignment aVerticalAlignment)
	{
	LOGFMT2(KVideoEditorLogFile, "CVeiTitleClipGenerator::NewLC: In: aMaxResolution: (%d,%d)", aMaxResolution.iWidth,aMaxResolution.iHeight);

	__ASSERT_ALWAYS(aMaxResolution.iHeight >= 0, TVedPanic::Panic(TVedPanic::ETitleClipGeneratorIllegalMaxResolution));
	__ASSERT_ALWAYS(aMaxResolution.iWidth >= 0, TVedPanic::Panic(TVedPanic::ETitleClipGeneratorIllegalMaxResolution));

	CVeiTitleClipGenerator* self = new (ELeave) CVeiTitleClipGenerator(aMaxResolution, aTransition, aHorizontalAlignment, aVerticalAlignment);
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
	}

void CVeiTitleClipGenerator::ConstructL()
	{
	iDescriptiveName = HBufC::NewL(0);
	iWrappedArray = new (ELeave) CArrayFixFlat<TPtrC>(KDefaultNumberOfLines);
	iText = HBufC::NewL(0);
	GetTextFont();
	}

EXPORT_C CVeiTitleClipGenerator::~CVeiTitleClipGenerator()
	{
	LOG(KVideoEditorLogFile, "CVeiTitleClipGenerator::~CVeiTitleClipGenerator(): In");
	delete iDecodeOperation;
	delete iText;
	if (iWrappedArray) 
		{ 
		iWrappedArray->Reset();
		}
	delete iWrappedArray;
	delete iBackgroundImage;
	delete iScaledBackgroundImage;
	delete iDescriptiveName;

	LOG(KVideoEditorLogFile, "CVeiTitleClipGenerator::~CVeiTitleClipGenerator(): Out");
	}

CVeiTitleClipGenerator::CVeiTitleClipGenerator(const TSize& aMaxResolution, 
											   TVeiTitleClipTransition aTransition, 
											   TVeiTitleClipHorizontalAlignment aHorizontalAlignment,
											   TVeiTitleClipVerticalAlignment aVerticalAlignment)
   : iFirstFrameComplexityFactor(0),
     iGetFontResolution(aMaxResolution),
     iMaxResolution(aMaxResolution),
     iTransition(aTransition),
     iHorizontalAlignment(aHorizontalAlignment),
	 iVerticalAlignment(aVerticalAlignment)
	{
	// Set some default values
	iDuration = TTimeIntervalMicroSeconds(KVeiTitleClipDefaultDuration);
	iBackgroundColor = KRgbBlack;
	iTextColor = KRgbWhite;
	}

EXPORT_C TPtrC CVeiTitleClipGenerator::DescriptiveName() const
	{
	return *iDescriptiveName;
	}

EXPORT_C TUid CVeiTitleClipGenerator::Uid() const
	{
	return KUidTitleClipGenerator;
	}

EXPORT_C TTimeIntervalMicroSeconds CVeiTitleClipGenerator::Duration() const
	{
	return iDuration;
	}

EXPORT_C TInt CVeiTitleClipGenerator::VideoFrameCount() const
	{
	TReal countReal((TReal)( (static_cast<TInt32>(iDuration.Int64())) / 1000000.0 ) * (TReal)(MaximumFramerate()) + 0.5);
	return (TInt) countReal;
	}

EXPORT_C TTimeIntervalMicroSeconds CVeiTitleClipGenerator::VideoFrameStartTime(TInt aIndex) const
	{
	__ASSERT_ALWAYS(aIndex >= 0 && aIndex < VideoFrameCount(), 
		TVedPanic::Panic(TVedPanic::EVideoClipGeneratorIllegalVideoFrameIndex));

	TTimeIntervalMicroSeconds frameTime(1000000 / MaximumFramerate());
	return TTimeIntervalMicroSeconds(TInt64(aIndex) * frameTime.Int64());
	}

EXPORT_C TTimeIntervalMicroSeconds CVeiTitleClipGenerator::VideoFrameEndTime(TInt aIndex) const
	{
	__ASSERT_ALWAYS(aIndex >= 0 && aIndex < VideoFrameCount(), 
		TVedPanic::Panic(TVedPanic::EVideoClipGeneratorIllegalVideoFrameIndex));

	TTimeIntervalMicroSeconds frameTime(1000000 / MaximumFramerate());
	return TTimeIntervalMicroSeconds( (static_cast<TInt64>(aIndex) + 1) * frameTime.Int64() ); 
	}

EXPORT_C TTimeIntervalMicroSeconds CVeiTitleClipGenerator::VideoFrameDuration(TInt aIndex) const
	{
	__ASSERT_ALWAYS(aIndex >= 0 && aIndex < VideoFrameCount(), 
		TVedPanic::Panic(TVedPanic::EVideoClipGeneratorIllegalVideoFrameIndex));

	return TTimeIntervalMicroSeconds(VideoFrameEndTime(aIndex).Int64() - VideoFrameStartTime(aIndex).Int64());
	}

EXPORT_C TInt CVeiTitleClipGenerator::VideoFirstFrameComplexityFactor() const
	{
	return iFirstFrameComplexityFactor;
	}

EXPORT_C TInt CVeiTitleClipGenerator::VideoFrameDifferenceFactor(TInt aIndex) const
	{
	__ASSERT_ALWAYS(aIndex > 0 && aIndex < VideoFrameCount(), 
		TVedPanic::Panic(TVedPanic::EVideoClipGeneratorIllegalVideoFrameIndex));

	TInt differenceFactor = 0;
	TInt pixels = iMaxResolution.iHeight * iMaxResolution.iWidth;
	TInt scale = 1000;
	TInt fontHeight = iTextFont->HeightInPixels();
	
	switch(iTransition) 
		{
	case EVeiTitleClipTransitionNone:
		{
		differenceFactor = 0; // if there is no transition, it's just a static text
		}
		break;
	case EVeiTitleClipTransitionFade:
		{
		TInt inEndFrame;
		TInt outStartFrame;
		CalculateTransitionFrameIndices(inEndFrame, outStartFrame);
		if ((aIndex < inEndFrame) || (aIndex > outStartFrame)) 
			{
			for (TInt i = 0; i < iWrappedArray->Count(); i++)
				{
				TInt lineWidth = iTextFont->TextWidthInPixels(iWrappedArray->At(i));
				differenceFactor += fontHeight * lineWidth;
				}
			differenceFactor *= scale;
			differenceFactor /= pixels;
			}
		else
			{
			differenceFactor = 0;
			}
		}
		break;
	case EVeiTitleClipTransitionScrollBottomToTop:
	case EVeiTitleClipTransitionScrollTopToBottom:
		{
		TInt yTravelPerFrame = (iMaxResolution.iHeight + iWrappedTextBoxHeight) / VideoFrameCount() + 1;

		TInt maxLineWidth = 0;
		for (TInt i = 0; i < iWrappedArray->Count(); i++)
			{
			TInt lineWidth = iTextFont->TextWidthInPixels(iWrappedArray->At(i));
			if (lineWidth > maxLineWidth) 
				{
				maxLineWidth = lineWidth;
				}
			}
		differenceFactor = scale * maxLineWidth * yTravelPerFrame / pixels;
		break;
		}
	case EVeiTitleClipTransitionScrollLeftToRight:
	case EVeiTitleClipTransitionScrollRightToLeft:
		{
		TInt xTravelPerFrame = (iMaxResolution.iWidth + iTextFont->TextWidthInPixels(*iText)) / VideoFrameCount() + 1;
		differenceFactor = scale * xTravelPerFrame * fontHeight / pixels;
		break;
		}
	default:
		TVedPanic::Panic(TVedPanic::EInternal);
		}

	return differenceFactor;
	}

EXPORT_C TInt CVeiTitleClipGenerator::GetVideoFrameIndex(TTimeIntervalMicroSeconds aTime) const
	{
	__ASSERT_ALWAYS(aTime.Int64() >= 0 && aTime.Int64() <= iDuration.Int64(),
		TVedPanic::Panic(TVedPanic::EVideoClipGeneratorIllegalVideoFrameIndex));

	TTimeIntervalMicroSeconds frameTime(1000000 / MaximumFramerate());
	return (static_cast<TInt32>(aTime.Int64() / frameTime.Int64()));
	}

EXPORT_C void CVeiTitleClipGenerator::GetFrameL(MVedVideoClipGeneratorFrameObserver& aObserver,
								  TInt aIndex, TSize* const aResolution,
								  TDisplayMode aDisplayMode, TBool aEnhance,
								  TInt aPriority)
	{
#ifdef VERBOSE
	LOGFMT3(KVideoEditorLogFile, "CVeiTitleClipGenerator::GetFrameL(): In: aIndex:%d, aResolution:(%d,%d)",\
		aIndex, aResolution->iWidth, aResolution->iHeight);
#endif

	__ASSERT_ALWAYS(aIndex == KFrameIndexBestThumb || aIndex >= 0 
		&& aIndex < VideoFrameCount(), 
		TVedPanic::Panic(TVedPanic::EVideoClipGeneratorIllegalVideoFrameIndex));

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


	iUseScaledImage = EFalse;
	iGetFrameObserver = &aObserver;
    iGetFrameIndex = aIndex;
	iGetFrameResolution = TSize(aResolution->iWidth, aResolution->iHeight);
	iGetFrameDisplayMode = displayMode;
	iGetFrameEnhance = aEnhance;
	iGetFramePriority = aPriority;	

	if (!iBackgroundImage)
		{
		// No background image, just finish.
		FinishGetFrameL();
		}
	else if	((aResolution->iHeight == iBackgroundImage->SizeInPixels().iHeight) || 
			 (aResolution->iWidth == iBackgroundImage->SizeInPixels().iWidth)) 
		{
		// The background image does not need scaling - deleting scaled image
		// causes FinishGetFrameL() to use the original.
		iUseScaledImage = EFalse;
		FinishGetFrameL();
		}
	else if (iScaledBackgroundImage &&
			 ((aResolution->iHeight == iScaledBackgroundImage->SizeInPixels().iHeight) || 
			 (aResolution->iWidth == iScaledBackgroundImage->SizeInPixels().iWidth)))
		{
		// Scaled image is in correct resolution
		iUseScaledImage = ETrue;
		FinishGetFrameL();
		}
	else
		{
		// We need to scale the image.
		iUseScaledImage = ETrue;
		if (iScaledBackgroundImage) 
			{
			delete iScaledBackgroundImage;
			iScaledBackgroundImage = 0;
			}

		if (iDecodeOperation) 
			{
			delete iDecodeOperation;
			iDecodeOperation = 0;
			}

		CFbsBitmap* bitmap = new (ELeave) CFbsBitmap;
		CleanupStack::PushL(bitmap);
		User::LeaveIfError(bitmap->Create(iBackgroundImage->SizeInPixels(), iBackgroundImage->DisplayMode()));
		CFbsDevice* device = CFbsBitmapDevice::NewL(bitmap);
		CFbsBitGc* gc = NULL;
		device->CreateContext(gc);
		gc->BitBlt(TPoint(0,0), iBackgroundImage);
		delete gc;
		delete device;		
		iDecodeOperation = CVeiTitleClipImageDecodeOperation::NewL(*this, bitmap);
		CleanupStack::Pop(bitmap);
		iDecodeOperation->StartScalingOperationL(iGetFrameResolution);
		}

#ifdef VERBOSE
	LOG(KVideoEditorLogFile, "CVeiTitleClipGenerator::GetFrameL(): Out");
#endif
	}


EXPORT_C CFbsBitmap* CVeiTitleClipGenerator::FinishGetFrameL(TInt aError)
	{
#ifdef VERBOSE
	LOGFMT(KVideoEditorLogFile, "CVeiTitleClipGenerator::FinishGetFrameL(): In: aError:%d", aError);
#endif

	ASSERT(iGetFrameResolution.iWidth > 0 && iGetFrameResolution.iHeight > 0);

	if (aError != KErrNone) 
		{
		iGetFrameObserver->NotifyVideoClipGeneratorFrameCompleted(*this, aError, NULL);
		return NULL;
		}

	CFbsBitmap* bitmap = new (ELeave) CFbsBitmap;
	CleanupStack::PushL(bitmap);
	TInt err = bitmap->Create(iGetFrameResolution, iGetFrameDisplayMode);
	if (err != KErrNone) 
		{
		delete bitmap;
		bitmap = NULL;
		iGetFrameObserver->NotifyVideoClipGeneratorFrameCompleted(*this, err, NULL);
		}
	else
		{
		CFbsDevice* device = CFbsBitmapDevice::NewL(bitmap);
		CFbsBitGc* gc = NULL;
		device->CreateContext(gc);

		/* Draw the background. */
		if (iUseScaledImage) 
			{
			TSize bgImageSize(iScaledBackgroundImage->SizeInPixels());
			TPoint bgPoint((iGetFrameResolution.iWidth - bgImageSize.iWidth) / 2,
						   (iGetFrameResolution.iHeight - bgImageSize.iHeight) / 2);
			gc->BitBlt(bgPoint, iScaledBackgroundImage);
			}
		else if (iBackgroundImage) 
			{
			TSize bgImageSize(iBackgroundImage->SizeInPixels());
			TPoint bgPoint((iGetFrameResolution.iWidth - bgImageSize.iWidth) / 2,
						   (iGetFrameResolution.iHeight - bgImageSize.iHeight) / 2);
			gc->BitBlt(bgPoint, iBackgroundImage);
			}
		else
			{
			gc->SetBrushColor(iBackgroundColor);
			gc->SetBrushStyle(CGraphicsContext::ESolidBrush);
			gc->DrawRect(bitmap->SizeInPixels());
			}

		delete gc;
		delete device;

		switch(iTransition) 
			{
		case EVeiTitleClipTransitionNone:
		case EVeiTitleClipTransitionFade:
			{
			DrawMainTitleFrameL(*bitmap, iGetFrameIndex);
			break;
			}
		case EVeiTitleClipTransitionScrollBottomToTop:
		case EVeiTitleClipTransitionScrollTopToBottom:
		case EVeiTitleClipTransitionScrollLeftToRight:
		case EVeiTitleClipTransitionScrollRightToLeft:
			{
			DrawScrollTitleFrameL(*bitmap, iGetFrameIndex);
			break;
			}
		default:
			TVedPanic::Panic(TVedPanic::EInternal);
			}

		/* Notify the observer. */
		if (iGetFrameObserver) 
			{
			iGetFrameObserver->NotifyVideoClipGeneratorFrameCompleted(*this, KErrNone, bitmap);
			}

		iGetFrameObserver = 0;
	    iGetFrameIndex = -5;

		CleanupStack::Pop(bitmap);
		}

#ifdef VERBOSE
	LOG(KVideoEditorLogFile, "CVeiTitleClipGenerator::FinishGetFrameL(): Out");
#endif

	return bitmap;
	}

EXPORT_C void CVeiTitleClipGenerator::CancelFrame()
	{
	if (iDecodeOperation) 
		{
		iDecodeOperation->Cancel();
		}
	}

EXPORT_C TVeiTitleClipTransition CVeiTitleClipGenerator::Transition() const
	{
	return iTransition;
	}

EXPORT_C TVeiTitleClipHorizontalAlignment CVeiTitleClipGenerator::HorizontalAlignment() const
	{
	return iHorizontalAlignment;
	}

EXPORT_C TVeiTitleClipVerticalAlignment CVeiTitleClipGenerator::VerticalAlignment() const
	{
	return iVerticalAlignment;
	}

EXPORT_C void CVeiTitleClipGenerator::SetTransitionAndAlignmentsL(												
		TVeiTitleClipTransition aTransition,
		TVeiTitleClipHorizontalAlignment aHorizontalAlignment,
		TVeiTitleClipVerticalAlignment aVerticalAlignment)
	{
	TBool updateNeeded = EFalse;
	if (iTransition != aTransition) 
		{
		iTransition = aTransition;
		updateNeeded = ETrue;
		}

	if (iHorizontalAlignment != aHorizontalAlignment) 
		{
		iHorizontalAlignment = aHorizontalAlignment;
		updateNeeded = ETrue;
		}

	if (iVerticalAlignment != aVerticalAlignment) 
		{
		iVerticalAlignment = aVerticalAlignment;
		updateNeeded = ETrue;
		}

	if (updateNeeded) 
		{
		UpdateFirstFrameComplexityFactorL();
		ReportSettingsChanged();
		}
	}

EXPORT_C void CVeiTitleClipGenerator::SetTextL(const TDesC& aText)
	{
	LOG(KVideoEditorLogFile, "CVeiTitleClipGenerator::SetTextL(): In");

	/* Don't do anything if the texts are equal. */
	if (iText && !iText->Compare(aText)) 
		{
		return;
		}

	/* Delete old text. */
	if (iText) 
		{
		delete iText;
		iText = 0;
		}

	/* Wrap the string to lines. */
	WrapTextToArrayL(aText);
	ReportSettingsChanged();

	LOG(KVideoEditorLogFile, "CVeiTitleClipGenerator::SetTextL(): Out");
	}

EXPORT_C void CVeiTitleClipGenerator::SetDescriptiveNameL(const TDesC& aDescriptiveName)
	{
	if (iDescriptiveName && !iDescriptiveName->Compare(aDescriptiveName)) 
		{
		return;
		}

	if (iDescriptiveName) 
		{
		delete iDescriptiveName;
		iDescriptiveName = 0;
		}
	iDescriptiveName = HBufC::NewL(aDescriptiveName.Length());
	*iDescriptiveName = aDescriptiveName;
	ReportDescriptiveNameChanged();
	}

EXPORT_C TPtrC CVeiTitleClipGenerator::Text() const
	{
	return *iText;
	}


EXPORT_C void CVeiTitleClipGenerator::SetBackgroundColorL(const TRgb& aBackgroundColor)
	{
	if (iBackgroundImage) 
		{
		delete iBackgroundImage;
		iBackgroundImage = 0;
		delete iScaledBackgroundImage;
		iScaledBackgroundImage = 0;
		}
	else if (aBackgroundColor == iBackgroundColor) 
		{
		return;
		}

	iBackgroundColor = aBackgroundColor;
	UpdateFirstFrameComplexityFactorL();
	ReportSettingsChanged();
	}

EXPORT_C TRgb CVeiTitleClipGenerator::BackgroundColor() const
	{
	return iBackgroundColor;
	}

EXPORT_C void CVeiTitleClipGenerator::SetTextColorL(const TRgb& aTextColor)
	{
	if (iTextColor != aTextColor) 
		{
		iTextColor = aTextColor;
		ReportSettingsChanged();
		UpdateFirstFrameComplexityFactorL();
		}
	}

EXPORT_C TRgb CVeiTitleClipGenerator::TextColor() const
	{
	return iTextColor;
	}

EXPORT_C void CVeiTitleClipGenerator::SetBackgroundImageL(const CFbsBitmap* aBackgroundImage)
	{
	if (iBackgroundImage) 
		{
		delete iBackgroundImage;
		iBackgroundImage = 0;
		delete iScaledBackgroundImage;
		iScaledBackgroundImage = 0;
		}

	iBackgroundImage = new (ELeave) CFbsBitmap;
	iBackgroundImage->Duplicate(aBackgroundImage->Handle());
	UpdateFirstFrameComplexityFactorL();
	ReportSettingsChanged();
	}

EXPORT_C void CVeiTitleClipGenerator::SetBackgroundImageL(const TDesC& aFilename, MVeiTitleClipGeneratorObserver& aObserver)
	{
	delete iDecodeOperation;
	iDecodeOperation = 0;
	delete iBackgroundImage;
	iBackgroundImage = 0;
	delete iScaledBackgroundImage;
	iScaledBackgroundImage = 0;

	iDecodeOperation = CVeiTitleClipImageDecodeOperation::NewL(*this, aObserver, aFilename);
	iDecodeOperation->StartLoadOperationL(iMaxResolution);
	}

EXPORT_C CFbsBitmap* CVeiTitleClipGenerator::BackgroundImage() const
	{
	return iBackgroundImage;
	}

EXPORT_C void CVeiTitleClipGenerator::SetDuration(const TTimeIntervalMicroSeconds& aDuration)
	{
	if (iDuration != aDuration) 
		{
		iDuration = aDuration;
		ReportDurationChanged();
		}
	}

void CVeiTitleClipGenerator::WrapTextToArrayL(const TDesC& aText)
	{
	LOGFMT2(KVideoEditorLogFile, "CVeiTitleClipGenerator::WrapTextToArrayL(): In: iMaxResolution: (%d,%d)", iMaxResolution.iWidth, iMaxResolution.iHeight);

	TInt fontdivisor = 11;
	
	do
		{
		
			if (iText) 
				{
				delete iText;
				iText = 0;
				}
				
    	if (iWrappedArray) 
    		{
    		iWrappedArray->Reset();
    		}

    	TInt width = iMaxResolution.iWidth - KMargin * 2;
    	GetTextFont(fontdivisor);
    	iText = AknBidiTextUtils::ConvertToVisualAndWrapToArrayL(aText, width, *iTextFont, *iWrappedArray);
    
    	/* Recalculate text dependant values. */
    	TInt lineHeight = iTextFont->HeightInPixels() + KLineSpacing;
    	iWrappedTextBoxHeight = iWrappedArray->Count() * lineHeight;
    	LOGFMT2(KVideoEditorLogFile, "CVeiTitleClipGenerator::WrapTextToArrayL(): In: iWrappedArray->Count %d, iWrappedTextBoxHeight  %d", iWrappedArray->Count(), iWrappedTextBoxHeight);

    	fontdivisor++;
		}
	while(iWrappedTextBoxHeight > iMaxResolution.iHeight);

	UpdateFirstFrameComplexityFactorL();

	LOG(KVideoEditorLogFile, "CVeiTitleClipGenerator::WrapTextToArrayL(): Out");
	}

void CVeiTitleClipGenerator::UpdateFirstFrameComplexityFactorL()
	{
	CFbsBitmap* bitmap = GetFirstFrameL();
	CleanupStack::PushL(bitmap);
	iFirstFrameComplexityFactor = CalculateFrameComplexityFactor(bitmap);	
	CleanupStack::PopAndDestroy(bitmap);
	}

void CVeiTitleClipGenerator::CalculateTransitionFrameIndices(TInt& aInEndFrame, TInt& aOutStartFrame) const
	{
	/* Calculate some values for timing the transitions */
	TInt64 divider = 3;
	TInt64 durationInt = iDuration.Int64();
	TTimeIntervalMicroSeconds inEndTime(durationInt / divider);
	TTimeIntervalMicroSeconds outStartTime(durationInt - durationInt / divider);

	/* Limit the fade in/out times to a reasonable value. */
	if (inEndTime.Int64() > TInt64(KVeiTitleClipFadeLimit)) 
		{
		inEndTime = TTimeIntervalMicroSeconds(KVeiTitleClipFadeLimit);
		outStartTime = TTimeIntervalMicroSeconds(durationInt - KVeiTitleClipFadeLimit);
		}

	if (IsInserted()) 
		{
		aInEndFrame = GetVideoFrameIndex(inEndTime);
		aOutStartFrame = GetVideoFrameIndex(outStartTime);
		}
	else
		{
		aInEndFrame = 5;
		aOutStartFrame = 10;
		}
	}


void CVeiTitleClipGenerator::GetTextFont(TInt aFontDivisor)
	{
#ifdef VERBOSE
	LOGFMT4(KVideoEditorLogFile, "CVeiTitleClipGenerator::GetTextFont(): iGetFrameResolution: (%d,%d), iMaxResolution: (%d,%d)",\
		iGetFrameResolution.iWidth, iGetFrameResolution.iHeight, iMaxResolution.iWidth, iMaxResolution.iHeight);
#endif

	if (!iTextFont || iGetFontResolution != iGetFrameResolution)
		{
		LOGFMT6(KVideoEditorLogFile, "CVeiTitleClipGenerator::GetTextFont(): Getting new font. iGetFontResolution: (%d,%d), iGetFrameResolution: (%d,%d), iMaxResolution: (%d,%d)",\
			iGetFontResolution.iWidth, iGetFontResolution.iHeight, iGetFrameResolution.iWidth, iGetFrameResolution.iHeight, iMaxResolution.iWidth, iMaxResolution.iHeight);

		TInt movieHeight = iGetFrameResolution.iHeight;
		iGetFontResolution = iGetFrameResolution;
		if (movieHeight <= 0)
			{
			movieHeight = iMaxResolution.iHeight;
			iGetFontResolution = iMaxResolution;
			}

		// If the generator is inserted, we can use the actual movie height
		// otherwise, we'll just assume max resolution.
		if (IsInserted())
			{
			CVedMovie* movie = Movie();
			movieHeight = movie->Resolution().iHeight;
			}
		TInt fontHeightInPixels;
		if(aFontDivisor)
			{
			fontHeightInPixels = movieHeight / (aFontDivisor -3);
			if (movieHeight >= 200)
				{
				fontHeightInPixels = movieHeight / aFontDivisor;
				}
			}
		else
			{
			fontHeightInPixels = movieHeight / 8;
			if (movieHeight >= 200)
				{
				fontHeightInPixels = movieHeight / 11;
				}
			}

		TAknFontSpecification spec( EAknFontCategoryPrimary );
		spec.SetTextPaneHeight( fontHeightInPixels );
		spec.SetWeight( EStrokeWeightBold );
		spec.SetPosture( EPostureUpright );

		CWsScreenDevice* dev = CCoeEnv::Static()->ScreenDevice();
		CAknLayoutFont* layoutFont = NULL;
		TRAPD(err, layoutFont = AknFontAccess::CreateLayoutFontFromSpecificationL( *dev, spec ) );
		if (err)
			{
			LOGFMT(KVideoEditorLogFile, "CVeiTitleClipGenerator::GetTextFont(): Creating font from font spec failed (%d)", err);

			// In case of failure (should not happen...) make sure that some font is still available
			ASSERT(EFalse);
			TInt fontId = 0; 
			if (movieHeight >= 200)
			    {
				fontId =EAknLogicalFontPrimaryFont;
			    }
			else
				{
				fontId = EAknLogicalFontSecondaryFont; 
				}
			iTextFont = AknLayoutUtils::FontFromId( fontId );
			}
		else
			{
			iTextFont = (const CFont*)layoutFont;
			}
		}

#ifdef VERBOSE
	LOG(KVideoEditorLogFile, "CVeiTitleClipGenerator::GetTextFont(): Out");
#endif
	}

void CVeiTitleClipGenerator::DrawWrappedTextL(CFbsBitmap& aBitmap,
	const TPoint& aTextPoint, const TRgb& aTextColor, const TRgb& aBgColor, const TRgb& aShadowColor,
	TBool aDrawBackground)
	{
#ifdef VERBOSE
	LOG(KVideoEditorLogFile, "CVeiTitleClipGenerator::DrawWrappedTextL(): In");
#endif

	// Make sure the text is wrapped using the movie's frame resolution.
	// If the generator is not inserted, just use the maximum resolution.
	//  - unfortunately this does not seem to work, if the text is re-wrapped
	//    here, the program seems to get stuck.
/*	if (IsInserted())
		{
		CVedMovie* movie = Movie();
		TSize res = movie->Resolution();
		if (iMaxResolution != res)
			{
			HBufC* oldText = iText->Des().AllocLC();
			delete iText;
			iText = NULL;
			iMaxResolution = res;
			WrapTextToArrayL(oldText->Des());
			CleanupStack::PopAndDestroy(oldText);
			}
		}
*/
	CFbsDevice* device = CFbsBitmapDevice::NewL(&aBitmap);
	CleanupStack::PushL(device);
	CFbsBitGc* gc = NULL;
	device->CreateContext(gc);
	CleanupStack::PushL(gc);
	gc->SetBrushStyle(CGraphicsContext::ESolidBrush);

	if (aDrawBackground)
		{
		gc->SetPenColor(aBgColor);
		gc->SetBrushColor(aBgColor);
		gc->DrawRect(TRect(aBitmap.SizeInPixels()));
		}

	gc->SetPenSize(TSize(1,1));

	GetTextFont();
	gc->UseFont(iTextFont);
	
	TPoint textPoint(aTextPoint);
	TRect rect(aBitmap.SizeInPixels());

	/* Draw text and shadow. */
	
	textPoint = aTextPoint;

	for (TInt i = 0; i < iWrappedArray->Count(); i++)
		{
		TInt textWidth = iTextFont->TextWidthInPixels(iWrappedArray->At(i));
		TInt baseX = 0;
		switch(iHorizontalAlignment) 
			{
		case EVeiTitleClipHorizontalAlignmentCenter:
			baseX = (rect.Width() - textWidth) / 2;
			break;
		case EVeiTitleClipHorizontalAlignmentLeft:
			baseX = KMargin;
			break;
		case EVeiTitleClipHorizontalAlignmentRight:
			baseX = (rect.iBr.iX - textWidth) - KMargin;
			break;
		default:
			TVedPanic::Panic(TVedPanic::EInternal);
			}
		textPoint.iX = baseX + 1;
		textPoint.iY += iTextFont->AscentInPixels() + KLineSpacing / 2 + 2;
		gc->SetBrushColor(aShadowColor);
		gc->SetPenColor(aShadowColor);
		gc->DrawText(iWrappedArray->At(i), textPoint);
		textPoint.iX -= 1;
		textPoint.iY -= 2;
		gc->SetBrushColor(aTextColor);
		gc->SetPenColor(aTextColor);
		gc->DrawText(iWrappedArray->At(i), textPoint);
		textPoint.iY += iTextFont->DescentInPixels() + KLineSpacing / 2;
		}

	gc->DiscardFont();
	CleanupStack::PopAndDestroy(gc);
	CleanupStack::PopAndDestroy(device);

#ifdef VERBOSE
	LOG(KVideoEditorLogFile, "CVeiTitleClipGenerator::DrawWrappedTextL(): Out");
#endif
	}

EXPORT_C CFbsBitmap* CVeiTitleClipGenerator::GetFirstFrameL()
	{
	LOG(KVideoEditorLogFile, "CVeiTitleClipGenerator::GetFirstFrameL()");

	iUseScaledImage = EFalse;
	iGetFrameObserver = 0;
    iGetFrameIndex = 0;

	if (iBackgroundImage) 
		{
		iGetFrameResolution = iBackgroundImage->SizeInPixels();

		LOGFMT2(KVideoEditorLogFile, "CVeiTitleClipGenerator::GetFirstFrameL(): Using BG image resolution: (%d,%d)", \
			iGetFrameResolution.iWidth, iGetFrameResolution.iHeight);
		}
	else
		{
		iGetFrameResolution = iMaxResolution;

		LOGFMT2(KVideoEditorLogFile, "CVeiTitleClipGenerator::GetFirstFrameL(): Using iMaxResolution: (%d,%d)", \
			iGetFrameResolution.iWidth, iGetFrameResolution.iHeight);
		}

	iGetFrameDisplayMode = KVideoClipGenetatorDisplayMode;
	iGetFrameEnhance = EFalse;
	return FinishGetFrameL();
	}

void CVeiTitleClipGenerator::DrawMainTitleFrameL(CFbsBitmap& aBitmap, TInt aIndex)
	{
	ASSERT(iGetFrameResolution.iWidth > 0 && iGetFrameResolution.iHeight > 0);

	TInt inEndFrame;
	TInt outStartFrame;

	CalculateTransitionFrameIndices(inEndFrame, outStartFrame);
	
	TInt index = aIndex;

	/* Best thumbnail frame would be the one just after the transition. */
	if (aIndex == KFrameIndexBestThumb) 
		{
		if (iTransition == EVeiTitleClipTransitionFade) 
			{
			/* Select a bit different looking frame for fading effects. */
			index = inEndFrame / 2;
			}
		else
			{
			index = inEndFrame;
			}
		}

	TRect rect(aBitmap.SizeInPixels());

	/* Calculate text points */
	TInt yAdjust;
	if (iVerticalAlignment == EVeiTitleClipVerticalAlignmentBottom)
		{
		yAdjust = (rect.Height() - iWrappedTextBoxHeight) - KMargin; 
		}
	else if (iVerticalAlignment == EVeiTitleClipVerticalAlignmentCenter) 
		{
		yAdjust = (rect.Height() - iWrappedTextBoxHeight) / 2;
		}
	else
		{
		yAdjust = KMargin;
		}

	TPoint textPoint(rect.iTl.iX, rect.iTl.iY + yAdjust);

	/* Initializations for drawing the text. */
	CFbsBitmap* textBitmap = new (ELeave) CFbsBitmap;
	CleanupStack::PushL(textBitmap);
	User::LeaveIfError(textBitmap->Create(aBitmap.SizeInPixels(), aBitmap.DisplayMode()));

	/* Determine shadow color. */

	TRgb shadowColor;
	if (iTextColor.Gray2() > 0) 
		{
		shadowColor = KRgbBlack;
		}
	else
		{
		shadowColor = KRgbWhite;
		}

	/* Draw the text. */
	DrawWrappedTextL(*textBitmap, textPoint, iTextColor, iBackgroundColor, shadowColor, ETrue);

	/* Create mask. */
	CFbsBitmap* maskBitmap = new (ELeave) CFbsBitmap;
	CleanupStack::PushL(maskBitmap);
	User::LeaveIfError(maskBitmap->Create(aBitmap.SizeInPixels(), EGray256));

	/* Calculate fading if necessary. */
	TInt colorIndex = 255; // default: obilque text
	if (iTransition == EVeiTitleClipTransitionFade)
		{
		if (index < inEndFrame) 
			{
			// fade in
			colorIndex = index * 255 / inEndFrame;
			}
		else if (index > outStartFrame) 
			{
			// fade out
			TInt endFrame = VideoFrameCount() - 1;
			colorIndex = (index - endFrame) * 255 / (outStartFrame - endFrame);
			}
		}
	
	textPoint = TPoint(rect.iTl.iX, rect.iTl.iY + yAdjust);
	DrawWrappedTextL(*maskBitmap, textPoint, TRgb::Gray256(colorIndex), TRgb::Gray256(0), TRgb::Gray256(colorIndex), ETrue);

	/* Combine aBitmap & mask. */
	CFbsDevice* device = CFbsBitmapDevice::NewL(&aBitmap);
	CleanupStack::PushL(device);
	CFbsBitGc* gc = NULL;
	device->CreateContext(gc);
	CleanupStack::PushL(gc);

	gc->BitBltMasked(TPoint(0,0), textBitmap, rect, maskBitmap, EFalse);

	CleanupStack::PopAndDestroy(gc);
	CleanupStack::PopAndDestroy(device);
	CleanupStack::PopAndDestroy(maskBitmap);
	CleanupStack::PopAndDestroy(textBitmap);
	}


void CVeiTitleClipGenerator::DrawScrollTitleFrameL(CFbsBitmap& aBitmap, TInt aIndex)
	{
	TInt index = aIndex;

	TInt numberOfFrames = VideoFrameCount();

	/* Select thumbnail so that it's a bit different from the centered ones. */
	if (aIndex == KFrameIndexBestThumb) 
		{
		index = numberOfFrames - numberOfFrames / 3;
		}

	TRect rect(aBitmap.SizeInPixels());
	TPoint textPoint;
	
	/* Calculate amounts for vertical scrollers. */
	TInt yTravelPerFrame = (rect.Height() + iWrappedTextBoxHeight) / numberOfFrames + 1;
	TInt yStartPoint = (rect.iTl.iY - iWrappedTextBoxHeight) - 10;

	TRgb shadowColor;
	if (iTextColor.Gray2() > 0) 
		{
		shadowColor = KRgbBlack;
		}
	else
		{
		shadowColor = KRgbWhite;
		}

	if (iTransition == EVeiTitleClipTransitionScrollBottomToTop) 
		{
		textPoint.iY = yStartPoint + yTravelPerFrame * (numberOfFrames - index);
		DrawWrappedTextL(aBitmap, textPoint, iTextColor, iBackgroundColor, shadowColor, EFalse);
		}
	else 
		{
		CFbsDevice* device = CFbsBitmapDevice::NewL(&aBitmap);
		CFbsBitGc* gc = NULL;
		device->CreateContext(gc);

		GetTextFont();
		gc->UseFont(iTextFont);

		gc->SetBrushStyle(CGraphicsContext::ESolidBrush);
		gc->SetPenSize(TSize(1,1));

		/* Calculate scroll amounts for horizontal scrollers. */
		TInt xTravelPerFrame = (rect.Width() + iTextFont->TextWidthInPixels(*iText)) / numberOfFrames + 1;
		TInt xStartPoint = ( rect.iTl.iX - iTextFont->TextWidthInPixels(*iText) ) - 10;

		if (iTransition == EVeiTitleClipTransitionScrollTopToBottom) 
			{
			textPoint.iY = yStartPoint + yTravelPerFrame * index;

			/* Draw texts. */	
			for (TInt i = iWrappedArray->Count(); i > 0; i--)
				{
				TInt textWidth = iTextFont->TextWidthInPixels(iWrappedArray->At(i - 1));
				TInt baseX = 0;
				switch(HorizontalAlignment()) 
					{
				case EVeiTitleClipHorizontalAlignmentCenter:
					baseX = (rect.Width() - textWidth) / 2;
					break;
				case EVeiTitleClipHorizontalAlignmentLeft:
					baseX = 0;
					break;
				case EVeiTitleClipHorizontalAlignmentRight:
					baseX = rect.iBr.iX - textWidth;
					break;
				default:
					TVedPanic::Panic(TVedPanic::EInternal);
					}
				textPoint.iX = baseX + 1;
				textPoint.iY += iTextFont->AscentInPixels() + KLineSpacing / 2 + 2;
				gc->SetBrushColor(shadowColor);
				gc->SetPenColor(shadowColor);
				gc->DrawText(iWrappedArray->At(i), textPoint);
				textPoint.iX -= 1;
				textPoint.iY -= 2;
				gc->SetBrushColor(TextColor());
				gc->SetPenColor(TextColor());
				gc->DrawText(iWrappedArray->At(i), textPoint);
				textPoint.iY += iTextFont->DescentInPixels() + KLineSpacing / 2;
				}
			}
		else if (iTransition == EVeiTitleClipTransitionScrollRightToLeft) 
			{
			textPoint.iY = rect.Height() / 2 + 2;
			textPoint.iX = xStartPoint + xTravelPerFrame * (numberOfFrames - index) + 1;
			gc->SetBrushColor(shadowColor);
			gc->SetPenColor(shadowColor);
			gc->DrawText(*iText, textPoint);
			textPoint.iX -= 1;
			textPoint.iY -= 2;
			gc->SetBrushColor(TextColor());
			gc->SetPenColor(TextColor());
			gc->DrawText(*iText, textPoint);
			}
		else if (iTransition == EVeiTitleClipTransitionScrollLeftToRight) 
			{
			textPoint.iX = xStartPoint + xTravelPerFrame * index + 1;
			textPoint.iY = rect.Height() / 2 + 2;
			gc->SetBrushColor(shadowColor);
			gc->SetPenColor(shadowColor);
			gc->DrawText(*iText, textPoint);
			textPoint.iX -= 1;
			textPoint.iY -= 2;
			gc->SetBrushColor(TextColor());
			gc->SetPenColor(TextColor());
			gc->DrawText(*iText, textPoint);
			}
		
		gc->DiscardFont();
		delete gc;
		delete device;
		}
	}

TInt CVeiTitleClipGenerator::MaximumFramerate() const
	{
	TInt maxFrameRate;
	if (IsInserted()) 
		{
		maxFrameRate = Movie()->MaximumFramerate();
		if (maxFrameRate > KMaxFrameRate)
			{
			maxFrameRate = KMaxFrameRate;
			}
		}
	else
		{
		maxFrameRate = KMaxFrameRate;
		}

#ifdef VERBOSE
	LOGFMT(KVideoEditorLogFile, "CVeiTitleClipGenerator::MaximumFramerate(): %d", maxFrameRate);
#endif

	return maxFrameRate;
	}

//////////////////////////////////////////////////////////////////////////
//  Decode operation
//////////////////////////////////////////////////////////////////////////


CVeiTitleClipImageDecodeOperation* CVeiTitleClipImageDecodeOperation::NewL(
								CVeiTitleClipGenerator& aGenerator,
								MVeiTitleClipGeneratorObserver& aObserver,
								const TDesC& aFilename, 
 							    TInt aPriority)
	{
    CVeiTitleClipImageDecodeOperation* self = 
		new (ELeave) CVeiTitleClipImageDecodeOperation(aGenerator, 
													   aObserver, 
													   aPriority);
    CleanupStack::PushL(self);
    self->ConstructL(aFilename);
    CleanupStack::Pop(self);
    return self;	
	}

CVeiTitleClipImageDecodeOperation::CVeiTitleClipImageDecodeOperation(CVeiTitleClipGenerator& aGenerator, 
														   MVeiTitleClipGeneratorObserver& aObserver,
														   TInt aPriority)
  : CActive(aPriority), iGenerator(aGenerator), iObserver(&aObserver)
	{
	CActiveScheduler::Add(this);
	}


CVeiTitleClipImageDecodeOperation* CVeiTitleClipImageDecodeOperation::NewL(
								CVeiTitleClipGenerator& aGenerator,
								CFbsBitmap* aSourceBitmap,
 							    TInt aPriority)
	{
    CVeiTitleClipImageDecodeOperation* self = 
		new (ELeave) CVeiTitleClipImageDecodeOperation(aGenerator, 
													   aSourceBitmap, 
													   aPriority);
    return self;	
	}


CVeiTitleClipImageDecodeOperation::CVeiTitleClipImageDecodeOperation(
										CVeiTitleClipGenerator& aGenerator, 
										CFbsBitmap* aSourceBitmap,
										TInt aPriority)
  : CActive(aPriority), iGenerator(aGenerator), iObserver(0), iBitmap(aSourceBitmap), iNotifyObserver(EFalse)
	{
	CActiveScheduler::Add(this);
	}


void CVeiTitleClipImageDecodeOperation::ConstructL(const TDesC& aFilename)
	{
	LOGFMT(KVideoEditorLogFile, "CVeiTitleClipImageDecodeOperation::constructL(): In: aFilename: %S", &aFilename);

	RFs&	fs = CCoeEnv::Static()->FsSession();
	iDecoder = CImageDecoder::FileNewL(fs, aFilename);
	iNotifyObserver = ETrue;
	
	LOG(KVideoEditorLogFile, "CVeiTitleClipImageDecodeOperation::ConstructL(): Out");
	}

CVeiTitleClipImageDecodeOperation::~CVeiTitleClipImageDecodeOperation()
	{
	Cancel();

	delete iDecoder;
	iDecoder = 0;
	delete iScaler;
	iScaler = 0;
	delete iBitmap;
	iBitmap = 0;
	iObserver = 0;
	}

void CVeiTitleClipImageDecodeOperation::DoCancel()
	{
	LOG(KVideoEditorLogFile, "CVeiTitleClipImageDecodeOperation::DoCancel(): In");

	if (iDecoder) 
		{
		iDecoder->Cancel();
		}

	delete iScaler;
	iScaler = 0;
	delete iDecoder;
	iDecoder = 0;
	delete iBitmap;
	iBitmap = 0;

	if (iNotifyObserver) 
		{
		iObserver->NotifyTitleClipBackgroundImageLoadComplete(iGenerator, KErrCancel);
		}
	else
		{
		TRAP_IGNORE( iGenerator.FinishGetFrameL() );
		}

	LOG(KVideoEditorLogFile, "CVeiTitleClipImageDecodeOperation::DoCancel(): Out");
	}

void CVeiTitleClipImageDecodeOperation::RunL()
	{
	switch(iDecodePhase) 
		{
	case EPhaseLoading:
			{
			StartScalingOperationL(iGenerator.iMaxResolution);
			break;
			}
	case EPhaseScaling:
			{
			delete iDecoder;
			iDecoder = 0;
			delete iScaler;
			iScaler = 0;
			iDecodePhase = EPhaseComplete;
			if (iNotifyObserver) 
				{
				/* Notify observer. */
				iObserver->NotifyTitleClipBackgroundImageLoadComplete(iGenerator, KErrNone);

				/* Transfer ownership of iBitmap to generator. */
				iGenerator.iBackgroundImage = iBitmap;
				iBitmap = 0;
				iGenerator.UpdateFirstFrameComplexityFactorL();
				iGenerator.ReportSettingsChanged();
				}
			else
				{
				iGenerator.iScaledBackgroundImage = iBitmap;
				iBitmap = 0;
				iGenerator.FinishGetFrameL();
				}
			break;
			}
	default:
		TVedPanic::Panic(TVedPanic::EInternal);
		}
	}

TInt CVeiTitleClipImageDecodeOperation::RunError(TInt aError)
	{
	LOGFMT(KVideoEditorLogFile, "CVeiTitleClipImageDecodeOperation::RunError(): In: %d", aError);
	if (iDecoder) 
		{
		iDecoder->Cancel();
		}

	delete iScaler;
	iScaler = 0;
	delete iDecoder;
	iDecoder = 0;
	delete iBitmap;
	iBitmap = 0;

	TInt err = KErrNone;
	if (iNotifyObserver) 
		{
		iObserver->NotifyTitleClipBackgroundImageLoadComplete(iGenerator, aError);
		}
	else
		{
		TRAP(err, iGenerator.FinishGetFrameL(aError) );
		}

	LOGFMT(KVideoEditorLogFile, "CVeiTitleClipImageDecodeOperation::RunError(): Out: %d", err);
	return err;
	}

void CVeiTitleClipImageDecodeOperation::StartLoadOperationL(const TSize& aMaxResolution)
	{
	LOGFMT2(KVideoEditorLogFile, "CVeiTitleClipImageDecodeOperation::StartLoadOperationL(): In: aMaxResolution: (%d,%d)", aMaxResolution.iWidth, aMaxResolution.iHeight);

	__ASSERT_ALWAYS(!IsActive(), TVedPanic::Panic(TVedPanic::EInternal));

	iDecodePhase = EPhaseLoading;

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
		if ((sourceResolution.iWidth * aMaxResolution.iWidth) > 
			(sourceResolution.iHeight * aMaxResolution.iHeight))
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
	TInt err = iBitmap->Create(targetResolution, EColor64K);

	if (err != KErrNone) 
		{
		delete iBitmap;
		iBitmap = 0;
		iObserver->NotifyTitleClipBackgroundImageLoadComplete(iGenerator, err);
		return;
		}

	iDecoder->Convert(&iStatus, *iBitmap);
	SetActive();

	LOG(KVideoEditorLogFile, "CVeiTitleClipImageDecodeOperation::StartLoadingOperationL(): Out");
	}


void CVeiTitleClipImageDecodeOperation::StartScalingOperationL(const TSize& aResolution)
	{
	LOGFMT2(KVideoEditorLogFile, "CVeiTitleClipImageDecodeOperation::StartScalingOperationL(): In: aResolution: (%d,%d)", aResolution.iWidth, aResolution.iHeight);

	__ASSERT_ALWAYS(!IsActive(), TVedPanic::Panic(TVedPanic::EInternal));

	iDecodePhase = EPhaseScaling;

	TSize sourceRes = iBitmap->SizeInPixels();
	TSize destRes(aResolution);
	TSize movieRes = iGenerator.Movie()->Resolution();

	if (destRes.iHeight > movieRes.iHeight || destRes.iWidth > movieRes.iWidth) 
		{
		movieRes = destRes;
		}
	
	TSize imageResInMovie(0,0);
	if ((sourceRes.iWidth > movieRes.iWidth) || (sourceRes.iHeight > movieRes.iHeight)) 
		{
		LOG(KVideoEditorLogFile, "CVeiTitleClipImageDecodeOperation::StartScalingOperationL downscaling");
		// Downscaling
		if ((sourceRes.iWidth * movieRes.iHeight) < 
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
	else if (iNotifyObserver) 
		{
		imageResInMovie = sourceRes;
		}
	else
		{
		LOG(KVideoEditorLogFile, "CVeiTitleClipImageDecodeOperation::StartScalingOperationL upscaling");
		// Upscaling - limit to a factor of two
		if ((sourceRes.iWidth * movieRes.iHeight) < 
			(sourceRes.iHeight * movieRes.iWidth))
			{
			imageResInMovie.iWidth = Min(movieRes.iWidth, (sourceRes.iWidth * 2));
			imageResInMovie.iHeight = Min((sourceRes.iHeight * 2),
				((movieRes.iWidth * sourceRes.iHeight) / sourceRes.iWidth));
			}
		else 
			{
			imageResInMovie.iHeight = Min((sourceRes.iHeight * 2), movieRes.iHeight);
			imageResInMovie.iWidth = Min((sourceRes.iWidth * 2),
				((movieRes.iHeight * sourceRes.iWidth) / sourceRes.iHeight));
			}
		}

	TSize movieResInDestBitmap(-1,-1);
	if ((movieRes.iWidth * destRes.iHeight) < 
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

	LOGFMT12(KVideoEditorLogFile,\
		"CVeiTitleClipOperation::StartOperation() sourceRes=(%d,%d) movieRes=(%d,%d) destRes=(%d,%d) targetRes=(%d,%d) imageResInMovie=(%d,%d) movieResInDestBitmap=(%d,%d)", \
		sourceRes.iWidth, sourceRes.iHeight, \
		movieRes.iWidth, movieRes.iHeight, \
		destRes.iWidth, destRes.iHeight, \
		targetRes.iWidth, targetRes.iHeight, \
		imageResInMovie.iWidth, imageResInMovie.iHeight, \
		movieResInDestBitmap.iWidth, movieResInDestBitmap.iHeight);
	

	if (iGenerator.iScaledBackgroundImage)
		{
		TSize scaledRes = iGenerator.iScaledBackgroundImage->SizeInPixels();
		if ((scaledRes.iWidth == targetRes.iWidth) || (scaledRes.iHeight == targetRes.iHeight)) 
			{
			LOG(KVideoEditorLogFile, "CVeiTitleClipImageDecodeOperation::StartScalingOperationL Using scaled");
			iGenerator.FinishGetFrameL();
			return;
			}
		}

	delete iScaler;	
	iScaler = NULL;
	iScaler = CBitmapScaler::NewL();
	iScaler->Scale(&iStatus, *iBitmap, targetRes, EFalse);
	SetActive();

	LOG(KVideoEditorLogFile, "CVeiTitleClipImageDecodeOperation::StartScalingOperationL(): Out");
	}

// End of File
