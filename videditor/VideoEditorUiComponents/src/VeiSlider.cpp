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


#include "VeiSlider.h"
#include "VeiSlider.pan"
#include "VideoEditorDebugUtils.h"

#include <VideoEditorUiComponents.mbg>
#include <fbs.h>
#include <aknutils.h>
#include <AknIconUtils.h>
#include <eikenv.h>

// CONSTANTS
_LIT(KSliderMifFile, "\\resource\\apps\\VideoEditorUiComponents.mif");

const TInt KTabAspectRatioX = 10;
const TInt KTabAspectRatioY = 24;

//=============================================================================
//
//	class CVeiSlider member functions
//
//=============================================================================

//=============================================================================
EXPORT_C CVeiSlider::~CVeiSlider()
    {
    delete iSliderBg;
	delete iSliderBgMask;
	delete iSliderTab;
	delete iSliderTabMask;
    }

//=============================================================================
CVeiSlider::CVeiSlider()
    {
    // no implementation required
    }

//=============================================================================
TInt CVeiSlider::CountComponentControls() const
    {
    return 0;
    }

//=============================================================================
CCoeControl* CVeiSlider::ComponentControl(TInt /*aIndex*/) const
    {
    return NULL;
    }

// setters

//=============================================================================
EXPORT_C void CVeiSlider::SetMinimum(TInt aValue)
	{
	iMinimumValue = aValue;
	}

//=============================================================================
EXPORT_C void CVeiSlider::SetMaximum(TInt aValue)
	{
	iMaximumValue = aValue;
	}

//=============================================================================
EXPORT_C void CVeiSlider::SetStep(TUint aValue)
	{
	iStep = aValue;
	}

//=============================================================================
EXPORT_C void CVeiSlider::SetStepAmount(TUint8 aValue)
	{
	iNumberOfSteps = aValue;
	
	if(aValue == 0)
		{
		iStep = 0;
		}
	else
		{
		iStep = (iMaximumValue-iMinimumValue) / aValue;
		}
	}

//=============================================================================
EXPORT_C void CVeiSlider::SetPosition(TInt aValue)
	{
	__ASSERT_ALWAYS( aValue >= iMinimumValue, Panic(EVeiSliderPanicIndexUnderflow) );
	__ASSERT_ALWAYS( aValue <= iMaximumValue, Panic(EVeiSliderPanicIndexOverflow) );

	iPosition = aValue;
	}

// getters

//=============================================================================
EXPORT_C TInt CVeiSlider::Minimum() const
	{
	return iMinimumValue;
	}

//=============================================================================
EXPORT_C TInt CVeiSlider::Maximum() const
	{
	return iMaximumValue;
	}

//=============================================================================
EXPORT_C TInt CVeiSlider::Step() const
	{
	return iStep;
	}

//=============================================================================
EXPORT_C TInt CVeiSlider::SliderPosition() const
	{
	return iPosition;
	}

//=============================================================================
EXPORT_C void CVeiSlider::Increment()
	{
	iPosition += iStep;
	if(iPosition > iMaximumValue)
		{
		iPosition = iMaximumValue;
		}
	}

//=============================================================================
EXPORT_C void CVeiSlider::Decrement()
	{
	iPosition -= iStep;
	if(iPosition < iMinimumValue)
		{
		iPosition = iMinimumValue;
		}
	}

//=============================================================================
void CVeiSlider::LoadBitmapL( CFbsBitmap*& aBitmap, CFbsBitmap*& aMask, TInt aBitmapId, TInt aMaskId ) const
	{
    TFileName iconFile( KSliderMifFile );
  
    User::LeaveIfError( CompleteWithAppPath(iconFile) );

    // Get ids for bitmap and mask
    AknIconUtils::CreateIconL(
        aBitmap,
        aMask,
        iconFile,
        aBitmapId,
        aMaskId
        );                
	}


//=============================================================================
//
//	class CVeiVerticalSlider member functions
//
//=============================================================================

//=============================================================================
EXPORT_C CVeiVerticalSlider* CVeiVerticalSlider::NewL(const TRect& aRect, const CCoeControl& aControl)
	{
	CVeiVerticalSlider* self = CVeiVerticalSlider::NewLC(aRect, aControl);
	CleanupStack::Pop(self);
	return self;
	}

//=============================================================================
EXPORT_C CVeiVerticalSlider* CVeiVerticalSlider::NewLC(const TRect& aRect, const CCoeControl& aControl)
    {
    CVeiVerticalSlider* self = new (ELeave) CVeiVerticalSlider;
    CleanupStack::PushL(self);
    self->ConstructL(aRect, aControl);
    return self;
    }

//=============================================================================
EXPORT_C CVeiVerticalSlider::~CVeiVerticalSlider()
    {
    }

//=============================================================================
void CVeiVerticalSlider::ConstructL(const TRect& aRect, const CCoeControl& aControl)
	{
	LOG(KVideoEditorLogFile, "CVeiVerticalSlider::ConstructL: in");

	SetContainerWindowL(aControl);

    // Load the bitmaps 
    LoadBitmapL( iSliderBg, iSliderBgMask, EMbmVideoeditoruicomponentsQgn_graf_ied_vslider, EMbmVideoeditoruicomponentsQgn_graf_ied_vslider_mask );
    LoadBitmapL( iSliderTab, iSliderTabMask, EMbmVideoeditoruicomponentsQgn_graf_ied_vtab, EMbmVideoeditoruicomponentsQgn_graf_ied_vtab_mask );

	SetRect(aRect);
	ActivateL();

	LOG(KVideoEditorLogFile, "CVeiVerticalSlider::ConstructL: out");
	}

//=============================================================================
CVeiVerticalSlider::CVeiVerticalSlider()
    {
    // no implementation required
    }

//=============================================================================
void CVeiVerticalSlider::Draw(const TRect& aRect) const
	{
    if ( Minimum() <= Maximum() )
    	{
        TUint height = Maximum() - Minimum(); // height of the slider
        TUint pixelsFromMin = SliderPosition() - Minimum(); // tab position from the beginning
    
        TReal factor = 0.0;
        if (Minimum() < Maximum() )
        	{
            factor = (TReal) pixelsFromMin / height; // tab position from the beginning in percentage
        	}
        TUint sliderTabHeight = iSliderTab->SizeInPixels().iHeight; // slider tab height

        // slider bitmap is actually a bit longer but this resolves the problem
        // where the tab is drawn outside of the slider when in maximum position
        TUint sliderBitmapHeight = iSliderBg->SizeInPixels().iHeight - sliderTabHeight;
    
        TUint tabPositionFromMinInPixels = (TUint) (factor * sliderBitmapHeight + 0.5); // calculate tab position

        // top left coordinate
        const TPoint tl = aRect.iTl;

        CWindowGc& gc = SystemGc();

        // draw actual slider using mask bitmap
        TRect bgRect(TPoint(0,0), iSliderBg->SizeInPixels());
        gc.BitBltMasked(tl, iSliderBg, bgRect, iSliderBgMask, ETrue);

        // draw the tab using mask bitmap
        TRect tabRect(TPoint(0,0), iSliderTab->SizeInPixels());
        gc.BitBltMasked(TPoint(tl.iX, tl.iY+tabPositionFromMinInPixels), iSliderTab, tabRect, iSliderTabMask, ETrue);
    	}
	}

//=============================================================================
void CVeiVerticalSlider::SizeChanged()
	{
	LOG(KVideoEditorLogFile, "CVeiVerticalSlider::SizeChanged: in");

	__ASSERT_ALWAYS( iSliderBg && iSliderTab, Panic(EVeiSliderPanicBitmapsNotLoaded) );

	// Set size for scalable icons - MUST BE CALLED BEFORE ICON IS USABLE
	TSize sliderSize;
	TSize tabSize;

	TInt w = Rect().Width();
	TInt h = Rect().Height();

	// NOTE: this assumes that the slider and the slider tab have the same width.
	// If that is not the case, it should be handled with transparency in the SVG graphic.

	// Set the slider bg to fill the whole rect
	sliderSize.iWidth = w;
	sliderSize.iHeight = h;
	AknIconUtils::SetSize( iSliderBg, sliderSize, EAspectRatioNotPreserved);

	// The slider tab is set to have the same width.
	// The height is calculated from the aspect ratio (set based on the original SVG)
	tabSize.iWidth = w;
	tabSize.iHeight = (TInt)( w * KTabAspectRatioY / KTabAspectRatioX );
	AknIconUtils::SetSize( iSliderTab, tabSize, EAspectRatioNotPreserved);

	LOGFMT4(KVideoEditorLogFile, "CVeiVerticalSlider::SizeChanged: out: sliderSize(%d,%d), tabSize(%d,%d)", sliderSize.iWidth,sliderSize.iHeight,tabSize.iWidth,tabSize.iHeight);
	}

//=============================================================================
TSize CVeiVerticalSlider::MinimumSize()
	{
	LOG(KVideoEditorLogFile, "CVeiVerticalSlider::MinimumSize()");

	return iSliderBg->SizeInPixels();
	}

//=============================================================================
//
//	class CVeiHorizontalSlider member functions
//
//=============================================================================

//=============================================================================
EXPORT_C CVeiHorizontalSlider* CVeiHorizontalSlider::NewL(const TRect& aRect, const CCoeControl& aControl)
	{
	CVeiHorizontalSlider* self = CVeiHorizontalSlider::NewLC(aRect, aControl);
	CleanupStack::Pop(self);
	return self;
	}

//=============================================================================
EXPORT_C CVeiHorizontalSlider* CVeiHorizontalSlider::NewLC(const TRect& aRect, const CCoeControl& aControl)
    {
    CVeiHorizontalSlider* self = new (ELeave) CVeiHorizontalSlider;
    CleanupStack::PushL(self);
    self->ConstructL(aRect, aControl);
    return self;
    }

//=============================================================================
EXPORT_C CVeiHorizontalSlider::~CVeiHorizontalSlider()
    {
    }

//=============================================================================
void CVeiHorizontalSlider::ConstructL(const TRect& aRect, const CCoeControl& aControl)
	{
	LOG(KVideoEditorLogFile, "CVeiVerticalSlider::ConstructL: in");

	SetContainerWindowL(aControl);

    // Load the bitmaps 
    LoadBitmapL( iSliderBg, iSliderBgMask, EMbmVideoeditoruicomponentsQgn_graf_ied_hslider, EMbmVideoeditoruicomponentsQgn_graf_ied_hslider_mask );
    LoadBitmapL( iSliderTab, iSliderTabMask, EMbmVideoeditoruicomponentsQgn_graf_ied_htab, EMbmVideoeditoruicomponentsQgn_graf_ied_htab_mask );

	SetRect(aRect);
	ActivateL();

	LOG(KVideoEditorLogFile, "CVeiVerticalSlider::ConstructL: out");
	}

//=============================================================================
CVeiHorizontalSlider::CVeiHorizontalSlider()
    {
    // no implementation required
    }

//=============================================================================
void CVeiHorizontalSlider::Draw(const TRect& aRect) const
	{
    if ( Minimum() <= Maximum() )
    	{
        TUint height = Maximum() - Minimum(); // height of the slider
        TUint pixelsFromMin = SliderPosition() - Minimum(); // tab position from the beginning
    
        TReal factor = 0.0;
        if (Minimum() < Maximum() )
        	{
            factor = (TReal) pixelsFromMin / height; // tab position from the beginning in percentage
        	}
        TUint sliderTabWidth = iSliderTab->SizeInPixels().iWidth; // slider tab width

        // slider bitmap is actually a bit longer but this resolves the problem
        // where the tab is drawn outside of the slider when in maximum position
        TUint sliderBitmapWidth = iSliderBg->SizeInPixels().iWidth - sliderTabWidth;
    
        TUint tabPositionFromMinInPixels = (TUint) (factor * sliderBitmapWidth + 0.5); // calculate tab position

        // top left coordinate
        const TPoint tl = aRect.iTl;

        CWindowGc& gc = SystemGc();

        // draw actual slider using mask bitmap
        TRect bgRect(TPoint(0,0), iSliderBg->SizeInPixels());
        gc.BitBltMasked(tl, iSliderBg, bgRect, iSliderBgMask, ETrue);

        // draw the tab using mask bitmap
        TRect tabRect(TPoint(0,0),iSliderTab->SizeInPixels());
        gc.BitBltMasked(TPoint(tl.iX + tabPositionFromMinInPixels, tl.iY), iSliderTab, tabRect, iSliderTabMask, ETrue);
    	}
	}

//=============================================================================
void CVeiHorizontalSlider::SizeChanged()
	{
	LOG(KVideoEditorLogFile, "CVeiHorizontalSlider::SizeChanged: in");

	__ASSERT_ALWAYS( iSliderBg && iSliderTab, Panic(EVeiSliderPanicBitmapsNotLoaded) );

	// Set size for scalable icons - MUST BE CALLED BEFORE ICON IS USABLE
	TSize sliderSize;
	TSize tabSize;

	TInt w = Rect().Width();
	TInt h = Rect().Height();

	// NOTE: this assumes that the slider and the slider tab have the same height.
	// If that is not the case, it should be handled with transparency in the SVG graphic.

	// Set the slider bg to fill the whole rect.
	sliderSize.iWidth = w;
	sliderSize.iHeight = h;
	AknIconUtils::SetSize( iSliderBg, sliderSize, EAspectRatioNotPreserved);

	// The slider tab is set to have the same height with the bg.
	// The width is calculated from the aspect ratio (set based on the original SVG).
	tabSize.iWidth = (TInt)( h * KTabAspectRatioY / KTabAspectRatioX );
	tabSize.iHeight = h;
	AknIconUtils::SetSize( iSliderTab, tabSize, EAspectRatioNotPreserved);

	LOGFMT4(KVideoEditorLogFile, "CVeiHorizontalSlider::SizeChanged: out: sliderSize(%d,%d), tabSize(%d,%d)", sliderSize.iWidth,sliderSize.iHeight,tabSize.iWidth,tabSize.iHeight);
	}

//=============================================================================
TSize CVeiHorizontalSlider::MinimumSize()
	{
	LOG(KVideoEditorLogFile, "CVeiHorizontalSlider::MinimumSize()");

	return iSliderBg->SizeInPixels();
	}

// End of File
