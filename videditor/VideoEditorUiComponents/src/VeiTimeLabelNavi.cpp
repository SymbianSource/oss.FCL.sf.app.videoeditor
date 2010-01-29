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


//  INCLUDE FILES
// System includes
#include <coecntrl.h>
#include <aknconsts.h>
#include <aknvolumecontrol.h> 
#include <akniconutils.h>
#include <barsread.h>
#include <aknsutils.h>
#include <aknsconstants.h>
#include <aknsdrawutils.h>
#include <aknsitemdef.h>
#include <VideoEditorUiComponents.mbg>
#include <avkon.mbg>

#ifdef RD_TACTILE_FEEDBACK 
#include <touchfeedback.h>
#endif /* RD_TACTILE_FEEDBACK  */

// User includes
#include "VeiTimeLabelNavi.h"
#include "VideoEditorCommon.h"
#include "VideoEditorUtils.h"
#include "VideoEditorDebugUtils.h"
#include "mtimelabelnaviobserver.h"

// CONSTANTS
_LIT(KDefaultLabel,""); // empty label text

enum TVeiTimeLabelLayout
    {
    EArrayItem = 0,
    ESpeakerItem,
    EPausedItem
    };

// ================= MEMBER FUNCTIONS =======================

// ---------------------------------------------------------
// CTimeLabelNavi::~CTimeLabelNavi()
// Destructor
// ---------------------------------------------------------
//
EXPORT_C CVeiTimeLabelNavi::~CVeiTimeLabelNavi()
	{
	DeleteBitmaps();
	}

void CVeiTimeLabelNavi::DeleteBitmaps()
	{
	delete iArrowBitmap;
	iArrowBitmap = NULL;
	
    delete iArrowBitmapMask;
    iArrowBitmapMask = NULL;	

	delete iVolumeBitmap;
	iVolumeBitmap = NULL;

	delete iVolumeBitmapMask;
	iVolumeBitmapMask = NULL;

	delete iMutedBitmap;
	iMutedBitmap = NULL;

	delete iMutedBitmapMask;
	iMutedBitmapMask = NULL;

	delete iPausedBitmap;
	iPausedBitmap = NULL;

	delete iPausedBitmapMask;
	iPausedBitmapMask = NULL;
	}

// ---------------------------------------------------------
// CTimeLabelNavi::CTimeLabelNavi()
// Constructor
// ---------------------------------------------------------
//
CVeiTimeLabelNavi::CVeiTimeLabelNavi():
iArrowVisible(ETrue),
iVolumeIconVisible(ETrue),
iPauseIconVisible(EFalse)
	{
    iLabel = KDefaultLabel;	
	}

// ---------------------------------------------------------
// CTimeLabelNavi::ConstructL()
// Constructor
// ---------------------------------------------------------
//
void CVeiTimeLabelNavi::ConstructL()
	{
	LoadBitmapsL();
	}

void CVeiTimeLabelNavi::LoadBitmapsL()
	{
	LOG(KVideoEditorLogFile, "CVeiTimeLabelNavi::LoadBitmapsL: in");

	TFileName bitmapfile( VideoEditorUtils::IconFileNameAndPath(
	        KVideoEditorUiComponentsIconFileId) );

	//Loads a specific bitmap from a multi-bitmap file.
	//const bitmapfile = the filename of the multi-bitmap (.mbm) file. 
	MAknsSkinInstance* skinInstance = AknsUtils::SkinInstance();

/* muted bitmap */
	AknsUtils::CreateColorIconL(
            skinInstance, KAknsIIDQgnIndiSpeakerMuted,KAknsIIDQsnIconColors,
			EAknsCIQsnIconColorsCG6,
            iMutedBitmap, iMutedBitmapMask,
            AknIconUtils::AvkonIconFileName(),
            EMbmAvkonQgn_indi_speaker_muted, 
            EMbmAvkonQgn_indi_speaker_muted_mask,KRgbBlack);
// Arrow bitmap
	AknsUtils::CreateColorIconL(
	            skinInstance, KAknsIIDNone,KAknsIIDQsnIconColors,
	            EAknsCIQsnIconColorsCG6,
	            iArrowBitmap, iArrowBitmapMask,
	            bitmapfile,
	            EMbmVideoeditoruicomponentsQgn_indi_vded_volume_up_down, 
	            EMbmVideoeditoruicomponentsQgn_indi_vded_volume_up_down_mask,
	            KRgbBlack);
	
/* Speaker bitmap */
	AknsUtils::CreateColorIconL(
            skinInstance, KAknsIIDQgnIndiSpeaker,KAknsIIDQsnIconColors,
			EAknsCIQsnIconColorsCG6,
            iVolumeBitmap, iVolumeBitmapMask,
            AknIconUtils::AvkonIconFileName(),
            EMbmAvkonQgn_indi_speaker, EMbmAvkonQgn_indi_speaker_mask,
            KRgbBlack);

/* Pause bitmap */	
// Pause bitmap in not used anywhere at the moment. There is not proper
// icon to be used for it anyway in the platform. If needed in the future,
// a new icon graphic need to be requested.
/*    
    AknsUtils::CreateColorIconL(
            skinInstance, KAknsIIDNone,KAknsIIDQsnIconColors,
			EAknsCIQsnIconColorsCG6,
            iPausedBitmap, iPausedBitmapMask,
            bitmapfile,
            EMbmVideoeditoruicomponentsQgn_prop_ve_pause, 
            EMbmVideoeditoruicomponentsQgn_prop_ve_pause_mask,KRgbBlack);
*/

#ifdef RD_TACTILE_FEEDBACK 
    iTouchFeedBack = MTouchFeedback::Instance();    
#endif /* RD_TACTILE_FEEDBACK  */     

	LOG(KVideoEditorLogFile, "CVeiTimeLabelNavi::LoadBitmapsL: out");
	}


// ---------------------------------------------------------
// CTimeLabelNavi::NewL()
// Symbian OS two-phase contruction
// ---------------------------------------------------------
//
EXPORT_C CVeiTimeLabelNavi* CVeiTimeLabelNavi::NewL()
	{
	LOG(KVideoEditorLogFile, "CVeiTimeLabelNavi::NewL: in");

	CVeiTimeLabelNavi* self = CVeiTimeLabelNavi::NewLC();
	CleanupStack::Pop( self );

	LOG(KVideoEditorLogFile, "CVeiTimeLabelNavi::NewL: out");
	return self;
	}

EXPORT_C CVeiTimeLabelNavi* CVeiTimeLabelNavi::NewLC()
	{
	LOG(KVideoEditorLogFile, "CVeiTimeLabelNavi::NewLC: in");

	CVeiTimeLabelNavi* self = new(ELeave) CVeiTimeLabelNavi();
	CleanupStack::PushL(self);
	self->ConstructL();

	LOG(KVideoEditorLogFile, "CVeiTimeLabelNavi::NewLC: out");
	return self;
	}


EXPORT_C void CVeiTimeLabelNavi::SetLabelL( const TDesC& aLabel )
	{
	#ifdef VERBOSE
	LOGFMT(KVideoEditorLogFile, "CVeiTimeLabelNavi::SetLabelL: %S", &aLabel);
	#endif

	if (aLabel.Length() <= iLabel.MaxLength())
		{
		iLabel = aLabel;
		}
	ReportEventL(MCoeControlObserver::EEventStateChanged);
	}

EXPORT_C void CVeiTimeLabelNavi::SetLeftArrowVisibilityL(TBool aVisible)
	{
	iArrowVisible = aVisible;
	ReportEventL(MCoeControlObserver::EEventStateChanged);	
	}

EXPORT_C void CVeiTimeLabelNavi::SetRightArrowVisibilityL(TBool aVisible)
	{
	iArrowVisible = aVisible;
	ReportEventL(MCoeControlObserver::EEventStateChanged);	
	}

EXPORT_C void CVeiTimeLabelNavi::SetVolumeIconVisibilityL(TBool aVisible)
	{
	iVolumeIconVisible = aVisible;
	ReportEventL(MCoeControlObserver::EEventStateChanged);	
	}

EXPORT_C void CVeiTimeLabelNavi::SetPauseIconVisibilityL(TBool aVisible)
	{
	iPauseIconVisible = aVisible;
	ReportEventL(MCoeControlObserver::EEventStateChanged);	
	}

void CVeiTimeLabelNavi::SizeChanged()
	{
    TRect parentRect(Rect());
	parentRect.iTl.iY+=2;
	/*
	const TRect &    aParent,  
	TInt    C,  
	TInt    l,  
	TInt    t,  
	TInt    r,  
	TInt    b,  
	TInt    W,  
	TInt    H 

	//C  colour index, 0..255  
	//l  left margin  
	//r  right margin  
	//B  Baseline from top of the parent rectangle  
	//W  text width in pixels  
	//J  justification. ELayoutAlignNone; ELayoutAlignCenter; ELayoutAlignLeft; ELayoutAlignRight  
*/

	const CFont* myFont = AknLayoutUtils::FontFromId( ELatinBold19/*EAknLogicalFontSecondaryFont*/ );
	TInt baseline = ( parentRect.Height() / 2 ) + ( myFont->AscentInPixels() / 2 );
	TInt iconHeightWidth = parentRect.Height();
		
	TSize iconSize = TSize(STATIC_CAST(TInt,0.0649*parentRect.Width()), 
		STATIC_CAST(TInt,0.5625*parentRect.Height() )); //14,18
	
	AknIconUtils::SetSize( 
	    iArrowBitmap,  
	    iconSize,
	    EAspectRatioNotPreserved );
	    
	AknIconUtils::SetSize( 
	    iPausedBitmap, 
	    iconSize,
	    EAspectRatioNotPreserved );
	    
	TInt arrowTop = ( parentRect.iBr.iY/2 - iconSize.iHeight/2 );

	iBitmapLayout[EArrayItem].LayoutRect(parentRect, ELayoutEmpty, STATIC_CAST(TInt, 0.0926*parentRect.Width()), arrowTop, ELayoutEmpty, ELayoutEmpty,iconHeightWidth,iconSize.iHeight);
	iBitmapLayout[ESpeakerItem].LayoutRect(parentRect, ELayoutEmpty, STATIC_CAST(TInt, 0.1625*parentRect.Width()), 0, ELayoutEmpty, ELayoutEmpty, iconHeightWidth, iconHeightWidth);
	iBitmapLayout[EPausedItem].LayoutRect(parentRect, ELayoutEmpty, 0, 0, ELayoutEmpty, ELayoutEmpty, 17, 16);

	TInt textLeftMargin = STATIC_CAST( TInt, 0.6045*parentRect.Width() );
	TInt textRightMargin = STATIC_CAST( TInt, 0.9546*parentRect.Width() );

	if( parentRect.iBr.iX != 108 )
		{
		iTextLayout.LayoutText(parentRect, ELatinBold19, 0, textLeftMargin, 0, 
			baseline, textRightMargin-textLeftMargin, ELayoutAlignLeft);

		}
	else
		{
		arrowTop = STATIC_CAST( TInt, parentRect.iBr.iY/2 - 9/2 );
		iBitmapLayout[EArrayItem].LayoutRect(parentRect, ELayoutEmpty, 14, arrowTop, ELayoutEmpty, ELayoutEmpty,iconHeightWidth,9);
		iBitmapLayout[ESpeakerItem].LayoutRect(parentRect, ELayoutEmpty, 20, 0,		ELayoutEmpty, ELayoutEmpty,iconHeightWidth,iconHeightWidth);
		iBitmapLayout[EPausedItem].LayoutRect(parentRect, ELayoutEmpty, 0,  0,		ELayoutEmpty, ELayoutEmpty,17,			   16);
		
		
		textLeftMargin = STATIC_CAST( TInt, 0.7273*parentRect.Width()/2 +20);	
		textRightMargin = STATIC_CAST( TInt, 0.9546*parentRect.Width()/2 +20);

		iTextLayout.LayoutText(parentRect, ELatinBold19, 0, textLeftMargin, 0, 
			baseline, textRightMargin-textLeftMargin, ELayoutAlignLeft);	
		}


	AknIconUtils::SetSize( iVolumeBitmap, TSize( iconHeightWidth, iconHeightWidth) );

	AknIconUtils::SetSize( iMutedBitmap, TSize( iconHeightWidth, iconHeightWidth) );
	}

void CVeiTimeLabelNavi::Draw(const TRect& /*aRect*/) const
	{
	CWindowGc& gc=SystemGc();

	if ( iVolumeIconVisible )
		{
		iBitmapLayout[ESpeakerItem].DrawImage( gc, iVolumeBitmap, iVolumeBitmapMask );

		if ( iArrowVisible && !AknLayoutUtils::PenEnabled() )
			{
			iBitmapLayout[EArrayItem].DrawImage( gc, iArrowBitmap, iArrowBitmapMask );
			}
		}
	else
		{
		iBitmapLayout[ESpeakerItem].DrawImage( gc, iMutedBitmap, iMutedBitmapMask );
		}

	if ( iPauseIconVisible )
		{
		// not in use at the moment, pause icon is drawed in main pane instead
		//iBitmapLayout[EPausedItem].DrawImage( gc, iPausedBitmap, NULL );
		}

	TBuf<32> labelWithConvNumbers;
	labelWithConvNumbers.Append( iLabel );
	AknTextUtils::DisplayTextLanguageSpecificNumberConversion( labelWithConvNumbers );

	MAknsSkinInstance* skinInstance = AknsUtils::SkinInstance();
	//AknsDrawUtils::DrawCachedImage(skinInstance,gc,TRect(0,0,24,24),KAknsIIDQgnIndiSpeaker);

	// Get navi pane text color from skin
	TRgb textColor( KRgbBlack );
	// Note: we are using the navi icon color for the text. There should
	// be separate color definition for the text, but I could not find
	// out what it is. Apparently KAknsIIDQsnTextColors, EAknsCIQsnTextColorsCG2
	// is NOT the correct color. Someone should find out which color is used 
	// for the label in CAknNavigationDecorator.
	//AknsUtils::GetCachedColor(skinInstance, textColor, KAknsIIDQsnTextColors, EAknsCIQsnTextColorsCG2 );
	AknsUtils::GetCachedColor(skinInstance, textColor, KAknsIIDQsnIconColors, EAknsCIQsnIconColorsCG6 );

    gc.SetBrushStyle( CGraphicsContext::ENullBrush );
	iTextLayout.DrawText(gc, labelWithConvNumbers, ETrue, textColor);
	}

void CVeiTimeLabelNavi::HandleResourceChange(TInt aType)
	{
	LOGFMT(KVideoEditorLogFile, "CVeiTimeLabelNavi::HandleResourceChange() In, aType:%d", aType);

	if (KAknsMessageSkinChange == aType)
		{
		// Reload the icon bitmaps with the current skin color
		DeleteBitmaps();
		TRAP_IGNORE( LoadBitmapsL() );
		}
	CCoeControl::HandleResourceChange(aType);

	LOG(KVideoEditorLogFile, "CVeiTimeLabelNavi::HandleResourceChange() Out");
    }

void CVeiTimeLabelNavi::HandlePointerEventL(const TPointerEvent& aPointerEvent)
    {
    if( iObserver && 
        iBitmapLayout[ESpeakerItem].Rect().Contains( aPointerEvent.iPosition ) )
        {
#ifdef RD_TACTILE_FEEDBACK 
		if ( iTouchFeedBack )
        	{
            iTouchFeedBack->InstantFeedback( ETouchFeedbackBasic );
        	}
#endif /* RD_TACTILE_FEEDBACK  */			        	        

        iObserver->HandleNaviEventL();
        }
    }

// End of File

