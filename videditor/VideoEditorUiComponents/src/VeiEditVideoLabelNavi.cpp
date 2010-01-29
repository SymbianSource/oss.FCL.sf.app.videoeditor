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


#include "VeiEditVideoLabelNavi.h"
#include <videoeditoruicomponents.mbg>
#include <videoeditoruicomponents.rsg>
#include <stringloader.h> 
#include <barsread.h>
#include <e32std.h>
#include <gulfont.h> 
#include <e32math.h> 
#include <aknsutils.h>
#include <data_caging_path_literals.hrh>
#include "VeiVideoEditorSettings.h"
#include "VideoEditorCommon.h"
#include "VideoEditorUtils.h"
#include "VideoEditorDebugUtils.h"

#include <AknFontAccess.h>
#include <AknFontSpecification.h>
#include <AknLayoutFont.h>

// CONSTANTS
_LIT(KResourceFile, "VideoEditorUiComponents.rsc");
const TReal KQvgaTextAndIconShrinkFactor = 0.8;

// ---------------------------------------------------------
// CVeiEditVideoLabelNavi::NewL()
// ---------------------------------------------------------
//
EXPORT_C CVeiEditVideoLabelNavi* CVeiEditVideoLabelNavi::NewL()
    {
    CVeiEditVideoLabelNavi* self = CVeiEditVideoLabelNavi::NewLC();
    CleanupStack::Pop( self );
    return self;
    }
// ---------------------------------------------------------
// CVeiEditVideoLabelNavi::NewLC()
// ---------------------------------------------------------
//
EXPORT_C CVeiEditVideoLabelNavi* CVeiEditVideoLabelNavi::NewLC()
    {
    CVeiEditVideoLabelNavi* self = new (ELeave) CVeiEditVideoLabelNavi;
    CleanupStack::PushL( self );
    self->ConstructL();
    return self;
    }
// ---------------------------------------------------------
// TUid CVeiEditVideoLabelNavi::ConstructL()
// ---------------------------------------------------------
//
void CVeiEditVideoLabelNavi::ConstructL()
    {
	LOG(KVideoEditorLogFile, "CVeiEditVideoLabelNavi::ConstructL: in");

	iState = /*EStateEditView*/EStateInitializing;

	iMmsMaxSize = 0;
	CVeiVideoEditorSettings::GetMaxMmsSizeL( iMmsMaxSize );

	LoadBitmapsL();

	// Open resource file
	TFileName resourceFile;
    Dll::FileName(resourceFile);
    TParse p;
    p.Set(KResourceFile, &KDC_RESOURCE_FILES_DIR, &resourceFile);
    resourceFile = p.FullName();
	iResLoader.OpenL( resourceFile );

	// The primary small font seems to be the smallest font available,
	// but it is still too large to fit all the texts in QVGA mode. 
	// We need to shrink it a little bit.
	TAknFontSpecification spec( KAknFontCategoryPrimarySmall );
	const CFont* myFont = AknLayoutUtils::FontFromId( EAknLogicalFontSecondaryFont );
	spec.SetTextPaneHeight( myFont->HeightInPixels() * KQvgaTextAndIconShrinkFactor );
	CWsScreenDevice* dev = ControlEnv()->ScreenDevice();
	iCustomFont = AknFontAccess::CreateLayoutFontFromSpecificationL( *dev, spec );

	LOG(KVideoEditorLogFile, "CVeiEditVideoLabelNavi::ConstructL: out");
    }
// ---------------------------------------------------------
// TUid CVeiEditVideoLabelNavi::LoadBitmapsL()
// ---------------------------------------------------------
//
void CVeiEditVideoLabelNavi::LoadBitmapsL()
    {
	LOG(KVideoEditorLogFile, "CVeiEditVideoLabelNavi::LoadBitmapsL: in");

	TFileName bitmapfile( VideoEditorUtils::IconFileNameAndPath(KVideoEditorUiComponentsIconFileId) );
	MAknsSkinInstance* skinInstance = AknsUtils::SkinInstance();
/* Mms bitmap */
	AknsUtils::CreateColorIconL(
            skinInstance, KAknsIIDNone,KAknsIIDQsnIconColors,
			EAknsCIQsnIconColorsCG6,
            iMmsBitmap, iMmsBitmapMask,
            bitmapfile,
            EMbmVideoeditoruicomponentsQgn_indi_ve_mms_ok, EMbmVideoeditoruicomponentsQgn_indi_ve_mms_ok_mask,KRgbBlack);

/* No Mms bitmap */
	AknsUtils::CreateColorIconL(
            skinInstance, KAknsIIDNone,KAknsIIDQsnIconColors,
			EAknsCIQsnIconColorsCG6,
            iNoMmsBitmap, iNoMmsBitmapMask,
            bitmapfile,
            EMbmVideoeditoruicomponentsQgn_indi_ve_mms_non, EMbmVideoeditoruicomponentsQgn_indi_ve_mms_non_mask,KRgbBlack);

/* Phone memory bitmap */
	AknsUtils::CreateColorIconL(
            skinInstance, KAknsIIDNone,KAknsIIDQsnIconColors,
			EAknsCIQsnIconColorsCG6,
            iPhoneMemoryBitmap, iPhoneMemoryBitmapMask,
            bitmapfile,
            EMbmVideoeditoruicomponentsQgn_indi_ve_phone, EMbmVideoeditoruicomponentsQgn_indi_ve_phone_mask,KRgbBlack);

/* No phone memory bitmap */
	AknsUtils::CreateColorIconL(
            skinInstance, KAknsIIDNone,KAknsIIDQsnIconColors,
			EAknsCIQsnIconColorsCG6,
            iNoPhoneMemoryBitmap, iNoPhoneMemoryBitmapMask,
            bitmapfile,
            EMbmVideoeditoruicomponentsQgn_indi_ve_phone_non, EMbmVideoeditoruicomponentsQgn_indi_ve_phone_non_mask,KRgbBlack);

/* MMC bitmap */
	AknsUtils::CreateColorIconL(
            skinInstance, KAknsIIDNone,KAknsIIDQsnIconColors,
			EAknsCIQsnIconColorsCG6,
            iMMCBitmap, iMMCBitmapMask,
            bitmapfile,
            EMbmVideoeditoruicomponentsQgn_indi_ve_mmc, EMbmVideoeditoruicomponentsQgn_indi_ve_mmc_mask,KRgbBlack);

/* No MMC bitmap */
	AknsUtils::CreateColorIconL(
            skinInstance, KAknsIIDNone,KAknsIIDQsnIconColors,
			EAknsCIQsnIconColorsCG6,
            iNoMMCBitmap, iNoMMCBitmapMask,
            bitmapfile,
            EMbmVideoeditoruicomponentsQgn_indi_ve_mmc_non, EMbmVideoeditoruicomponentsQgn_indi_ve_mmc_non_mask,KRgbBlack);

/* Time bitmap */
	AknsUtils::CreateColorIconL(
            skinInstance, KAknsIIDNone,KAknsIIDQsnIconColors,
			EAknsCIQsnIconColorsCG6,
            iTimeBitmap, iTimeBitmapMask,
            bitmapfile,
            EMbmVideoeditoruicomponentsQgn_indi_ve_videolength, EMbmVideoeditoruicomponentsQgn_indi_ve_videolength_mask,KRgbBlack);

	LOG(KVideoEditorLogFile, "CVeiEditVideoLabelNavi::LoadBitmapsL: out");
	}

// ---------------------------------------------------------
// TUid CVeiEditVideoLabelNavi::~CVeiEditVideoLabelNavi()
// ---------------------------------------------------------
//
EXPORT_C CVeiEditVideoLabelNavi::~CVeiEditVideoLabelNavi()
	{
	DeleteBitmaps();
	iResLoader.Close();
	delete iCustomFont;
	}

void CVeiEditVideoLabelNavi::DeleteBitmaps()
	{
	delete iMmsBitmap;
	iMmsBitmap = NULL;

	delete iMmsBitmapMask;
	iMmsBitmapMask = NULL;

	delete iNoMmsBitmap;
	iNoMmsBitmap = NULL;

	delete iNoMmsBitmapMask;
	iNoMmsBitmapMask = NULL;

	delete iPhoneMemoryBitmap;
	iPhoneMemoryBitmap = NULL;

	delete iPhoneMemoryBitmapMask;
	iPhoneMemoryBitmapMask = NULL;

	delete iNoPhoneMemoryBitmap;
	iNoPhoneMemoryBitmap = NULL;

	delete iNoPhoneMemoryBitmapMask;
	iNoPhoneMemoryBitmapMask = NULL;

	delete iMMCBitmap;
	iMMCBitmap = NULL;

	delete iMMCBitmapMask;
	iMMCBitmapMask = NULL;

	delete iNoMMCBitmap;
	iNoMMCBitmap = NULL;

	delete iNoMMCBitmapMask;
	iNoMMCBitmapMask = NULL;

	delete iTimeBitmap;
	iTimeBitmap = NULL;

	delete iTimeBitmapMask;
	iTimeBitmapMask = NULL;
	}

// ---------------------------------------------------------
// CVeiEditVideoLabelNavi::CVeiEditVideoLabelNavi()
// ---------------------------------------------------------
//
CVeiEditVideoLabelNavi::CVeiEditVideoLabelNavi() : iResLoader(*CEikonEnv::Static())
	{
	iStoryboardDuration = 0;
	iStoryboardSize = 0;
	}

// ---------------------------------------------------------
// CVeiEditVideoLabelNavi::Draw(const TRect& aRect) const
// ---------------------------------------------------------
//
void CVeiEditVideoLabelNavi::Draw(const TRect& aRect) const
    {
    if ( iState == EStateInitializing )
        {
        return;
        }
	CWindowGc& gc=SystemGc();

	// Get navi pane text color from skin
	TRgb textColor( KRgbBlack );
	MAknsSkinInstance* skinInstance = AknsUtils::SkinInstance();
	AknsUtils::GetCachedColor(skinInstance, textColor, KAknsIIDQsnTextColors, EAknsCIQsnTextColorsCG2 );

	TBuf<25> layoutTime;
	//minutes and seconds "120:13"
	TBuf<15> minsec;  
	TInt64 duration = 0; 
	TTimeIntervalHours hours;
	TTimeIntervalMinutes minutes; 
	TTimeIntervalMicroSeconds seconds; 
	duration = static_cast<TInt64>( iStoryboardDuration / 1000 ); 

	TChar timeSeparator = TLocale().TimeSeparator(2);

	//over 1 hour
	if( duration >= 3600000 )
		{
		hours = TTimeIntervalHours( (TInt)(duration / 3600000) );
		minsec.AppendNum( hours.Int() );
		minsec.Append( timeSeparator );

		duration = duration - TInt64(hours.Int()) * TInt64(3600000); 
		}
	//over 1 minute
	if( duration >= 60000 )
		{
		minutes = TTimeIntervalMinutes( (TInt)(duration / 60000) );
		minsec.AppendNum( minutes.Int() );

		duration = duration - TInt64(minutes.Int()) * TInt64(60000); 
		}
	else
		{
		minsec.Append( _L( "00" ) );
		}

	if( duration >= 1000 ) 
		{ 
		seconds = TTimeIntervalMicroSeconds( duration / 1000 );

		if( seconds.Int64() >= 60 ) 
			{
			minsec.Append( timeSeparator );
			minsec.AppendNum( seconds.Int64() - 60 );
			} 
		else 
			{
			minsec.Append( timeSeparator );
			if ( seconds.Int64() < 10 ) 
				{ 
				minsec.Append( _L("0") ); 
				}

			minsec.AppendNum( seconds.Int64() ); 
			}
		}
	else 
		{
		minsec.Append( timeSeparator );
		minsec.Append( _L("00") ); 
		}
	layoutTime.Append( minsec );
	AknTextUtils::DisplayTextLanguageSpecificNumberConversion( layoutTime );

    //text MMS
	TBuf<25> layoutTextMMS;
	HBufC* stringholder = StringLoader::LoadLC( R_VEI_NAVI_PANE_MMS, iEikonEnv );
	layoutTextMMS.Append( *stringholder );
	AknTextUtils::DisplayTextLanguageSpecificNumberConversion( layoutTextMMS );
	CleanupStack::PopAndDestroy( stringholder );

    //size of the movie in KB, MB or GB
	TBuf<25> layoutSize;

	if ( iStoryboardSize < 1000 )
		{
		HBufC* stringholder = StringLoader::LoadLC( R_VEI_SIZE_KB, iStoryboardSize, iEikonEnv );
		layoutSize.Append( *stringholder );
		AknTextUtils::DisplayTextLanguageSpecificNumberConversion( layoutSize );
		CleanupStack::PopAndDestroy( stringholder );
		}
	else 
		{
		TReal size = (TReal)iStoryboardSize / 1024;
		if ( size >= 1000 )
			{
			// Gigabytes are handled differently from megabytes, because we don't have 
			// appropriate localized string (i.e. with %U param) for gigabytes.
			TInt gigaSize = (TInt) (size/1024 + 0.5);
			stringholder = StringLoader::LoadLC( R_VEI_SIZE_GB, gigaSize, iEikonEnv );
			layoutSize.Append( *stringholder );
			AknTextUtils::DisplayTextLanguageSpecificNumberConversion( layoutSize );
			CleanupStack::PopAndDestroy( stringholder );
			}
		else
			{
			// for megabytes, drop the decimals if the value has 3 or more digits
			TBuf16<256> sizeValue;
			if (size < 100)
				{
				_LIT16( KFormat,"%3.1f" );
				sizeValue.Format( KFormat,size );
				}
			else
				{
				_LIT16( KFormat,"%3.0f" );
				sizeValue.Format( KFormat,size );
				}
			stringholder = StringLoader::LoadLC( R_VEI_SIZE_MB, sizeValue, iCoeEnv );
			layoutSize.Append( *stringholder );
			AknTextUtils::DisplayTextLanguageSpecificNumberConversion( layoutSize );
			CleanupStack::PopAndDestroy( stringholder );
			}
		}


	if ( iState == EStateEditView ) //EditView owns the navipane
		{
		if ( iMmsAvailable )
			{
			iBitmapLayout[0].DrawImage( gc, iMmsBitmap, iMmsBitmapMask );
			}
		else
			{
			iBitmapLayout[0].DrawImage( gc, iNoMmsBitmap, iNoMmsBitmapMask );
			}

		if ( aRect.iBr.iX >= 148 ) // QVGA or bigger
			{

			if ( iPhoneMemory ) //phone memory in use.
				{
				if ( iMemoryAvailable )
					{
					iBitmapLayout[1].DrawImage( gc, iPhoneMemoryBitmap, iPhoneMemoryBitmapMask );
					}
				else
					{
					iBitmapLayout[1].DrawImage( gc, iNoPhoneMemoryBitmap, iNoPhoneMemoryBitmapMask );
					}
				}
			else		//mmc-memory in use.
				{
				if ( iMemoryAvailable )
					{
					iBitmapLayout[1].DrawImage( gc, iMMCBitmap, iMMCBitmapMask );
					}
				else
					{
					iBitmapLayout[1].DrawImage( gc, iNoMMCBitmap, iNoMMCBitmapMask );
					}
				}
			iTextLayout[1].DrawText( gc, layoutSize, ETrue, textColor );	
			}

		iBitmapLayout[2].DrawImage( gc, iTimeBitmap, iTimeBitmapMask );


		iTextLayout[0].DrawText( gc, layoutTextMMS, ETrue, textColor );
		iTextLayout[2].DrawText( gc, layoutTime, ETrue, textColor );

		}
	else if ( iState == EStateTrimForMmsView ) //TrimForMmsView owns the navi pane.
		{
		if ( iMmsAvailable )
		    {
		    iBitmapLayout[0].DrawImage( gc, iMmsBitmap, iMmsBitmapMask );
			}
		else
			{
			iBitmapLayout[0].DrawImage( gc, iNoMmsBitmap, iNoMmsBitmapMask );
			}
		iTextLayout[0].DrawText( gc, layoutSize, ETrue, textColor );
        iBitmapLayout[2].DrawImage( gc, iTimeBitmap, iTimeBitmapMask );
		iTextLayout[2].DrawText( gc, layoutTime, ETrue, textColor );
		}

	// TEST
/*	gc.SetPenColor( KRgbRed );
	iBitmapLayout[0].DrawOutLineRect(gc);
	iBitmapLayout[1].DrawOutLineRect(gc);
	iBitmapLayout[2].DrawOutLineRect(gc);

	gc.SetBrushStyle( CGraphicsContext::ENullBrush );
	gc.DrawRect( iTextLayout[0].TextRect() );
	gc.DrawRect( iTextLayout[1].TextRect() );
	gc.DrawRect( iTextLayout[2].TextRect() );

	gc.DrawRect( Rect() );*/
	// END TEST

	}

// ---------------------------------------------------------
// CVeiEditVideoLabelNavi::HandleResourceChange(TInt aType)
// ---------------------------------------------------------
//
void CVeiEditVideoLabelNavi::HandleResourceChange(TInt aType)
	{
	LOGFMT(KVideoEditorLogFile, "CVeiEditVideoLabelNavi::HandleResourceChange() In, aType:%d", aType);

	if (KAknsMessageSkinChange == aType)
		{
		// Reload the icon bitmaps with the current skin color
		DeleteBitmaps();
		TRAP_IGNORE( LoadBitmapsL() );
		}
	CCoeControl::HandleResourceChange(aType);

	LOG(KVideoEditorLogFile, "CVeiEditVideoLabelNavi::HandleResourceChange() Out");
	}
 
// --------------------------------------------------------
// CVeiEditVideoLabelNavi::SetMmsAvailableL( TBool aIsAvailable )
// Set MMS envelope without red line or with it.
// ---------------------------------------------------------
//
EXPORT_C void CVeiEditVideoLabelNavi::SetMmsAvailableL( TBool aIsAvailable )
	{
	iMmsAvailable = aIsAvailable;
	ReportEventL( MCoeControlObserver::EEventStateChanged );	
	}
EXPORT_C TBool CVeiEditVideoLabelNavi::IsMMSAvailable() const
	{
	return iMmsAvailable;
	}

// --------------------------------------------------------
// CVeiEditVideoLabelNavi::SetMemoryInUseL( TBool aPhoneMemory )
// Set memory in use Phone/MMC.
// ---------------------------------------------------------
//
EXPORT_C void CVeiEditVideoLabelNavi::SetMemoryInUseL( TBool aPhoneMemory )
	{
	iPhoneMemory = aPhoneMemory;
	ReportEventL( MCoeControlObserver::EEventStateChanged );
	}
// --------------------------------------------------------
// CVeiEditVideoLabelNavi::SetMemoryAvailableL( TBool aIsAvailable )
// ?implementation_description
// ---------------------------------------------------------
//
EXPORT_C void CVeiEditVideoLabelNavi::SetMemoryAvailableL( TBool aIsAvailable )
	{
	iMemoryAvailable = aIsAvailable;
	ReportEventL( MCoeControlObserver::EEventStateChanged );
	}
// ---------------------------------------------------------
// CVeiEditVideoLabelNavi::SetDurationLabelL( const TUint& aDuration )
// Set movie duration.
// ---------------------------------------------------------
//
EXPORT_C void CVeiEditVideoLabelNavi::SetDurationLabelL( const TInt64& aDuration )
	{
	iStoryboardDuration = aDuration;
	ReportEventL( MCoeControlObserver::EEventStateChanged );	
	}

// ---------------------------------------------------------
// CVeiEditVideoLabelNavi::SetSizeLabelL( const TUint& aSize )
// Set movie size.
// ---------------------------------------------------------
//
EXPORT_C void CVeiEditVideoLabelNavi::SetSizeLabelL( const TUint& aSize )
	{
	iStoryboardSize = aSize;
	ReportEventL( MCoeControlObserver::EEventStateChanged );	
	}
// ---------------------------------------------------------
// CVeiEditVideoLabelNavi::SetState( CVeiEditVideoLabelNavi::TLabelNaviState aState )
//  Set whether editview or trimformms-view 
// ---------------------------------------------------------
//
EXPORT_C void CVeiEditVideoLabelNavi::SetState( CVeiEditVideoLabelNavi::TLabelNaviState aState )
	{
	iState = aState;
	}

// ---------------------------------------------------------
// CVeiEditVideoLabelNavi::SizeChanged()
// ---------------------------------------------------------
//
void CVeiEditVideoLabelNavi::SizeChanged()
	{
	TRect parentRect = Rect();
    parentRect.iTl.iY = parentRect.iTl.iY +2;
	TInt adjustLeftMemoryIcon(0);
	TInt adjustLeftTimeIcon(0);

	TInt adjustLeftMMSIcon = 0; 
	TInt adjustWidth =  parentRect.Height();
	TInt adjustHeight = parentRect.Height();

	const CFont* myFont = AknLayoutUtils::FontFromId( EAknLogicalFontSecondaryFont );

	// For high resolution(352x416) use different font
	if ( parentRect.iBr.iX >= 216 ) // high resolution(352x416)
		{
		adjustLeftMemoryIcon = STATIC_CAST( TInt, parentRect.Width() * 0.28 );
		adjustLeftTimeIcon = STATIC_CAST( TInt, parentRect.Width() * 0.67 );
		myFont = AknLayoutUtils::FontFromId( ELatinBold12 );
		}
	else if ( parentRect.iBr.iX >= 148 ) // QVGA (240x320)
		{
		adjustLeftMemoryIcon = STATIC_CAST( TInt, parentRect.Width() * 0.27 );
		adjustLeftTimeIcon = STATIC_CAST( TInt, parentRect.Width() * 0.64 );

		// Reduce the relative size of the icons a little bit.
		adjustWidth *= KQvgaTextAndIconShrinkFactor;
		adjustHeight *= KQvgaTextAndIconShrinkFactor;

		// use the extra small font
		myFont = (const CFont*)iCustomFont;
		}
	else
		{
		// in the small resolution (176x208) the memory icon and text are dropped out
		adjustLeftMemoryIcon = STATIC_CAST( TInt, parentRect.Width() * 0.4138 );
		adjustLeftTimeIcon = STATIC_CAST( TInt, parentRect.Width() * 0.42 + 15);
		}

	AknIconUtils::SetSize( iMmsBitmap, TSize( adjustWidth,adjustHeight) );
	AknIconUtils::SetSize( iNoMmsBitmap, TSize(adjustWidth,adjustHeight) );
	AknIconUtils::SetSize( iPhoneMemoryBitmap, TSize(adjustWidth,adjustHeight) );
	AknIconUtils::SetSize( iNoPhoneMemoryBitmap, TSize(adjustWidth,adjustHeight) );
	AknIconUtils::SetSize( iMMCBitmap, TSize(adjustWidth,adjustHeight) );
	AknIconUtils::SetSize( iNoMMCBitmap, TSize(adjustWidth,adjustHeight) );
	AknIconUtils::SetSize( iTimeBitmap, TSize(adjustWidth,adjustHeight) );

	//LayoutRect(const TRect &aParent, TInt C, TInt left, TInt top, TInt right, TInt bottom, TInt Width, TInt Height);
	iBitmapLayout[0].LayoutRect( parentRect, ELayoutEmpty, adjustLeftMMSIcon, 0, ELayoutEmpty, ELayoutEmpty, adjustWidth, adjustHeight );
	iBitmapLayout[1].LayoutRect( parentRect, ELayoutEmpty, adjustLeftMemoryIcon, 0, ELayoutEmpty, ELayoutEmpty, adjustWidth, adjustHeight );
	iBitmapLayout[2].LayoutRect( parentRect, ELayoutEmpty, adjustLeftTimeIcon, 0, ELayoutEmpty, ELayoutEmpty, adjustWidth, adjustHeight );
	
	//layout for MMS-text
	TInt tX = adjustLeftMMSIcon + adjustWidth;
	TInt tY = 2;

	TInt bX = adjustLeftTimeIcon;
	TInt bY = parentRect.Height();
	TRect mmsTextArea( tX, tY, bX, bY ); 

	TInt baseline = ( mmsTextArea.Height() / 2 ) + ( myFont->AscentInPixels() / 2 );
	if (myFont == iCustomFont)
		{
		// It seems that even if the font size is scaled down, AscentInPixels()
		// returns the original fon't ascent. We have to scale it down as well.
		baseline = ( mmsTextArea.Height() / 2 ) + ( myFont->AscentInPixels() / 2 * KQvgaTextAndIconShrinkFactor );
		}
	TInt margin=1; 
	TInt width = mmsTextArea.Width();
	iTextLayout[0].LayoutText( mmsTextArea,0,0,margin,margin,baseline,width,ELayoutAlignLeft,myFont );
	
	//layout for size-text
	tX = adjustLeftMemoryIcon + adjustWidth;
	bX = adjustLeftTimeIcon;
	TRect sizeTextArea( tX, tY, bX, bY ); 
	width = sizeTextArea.Width();
    //LayoutText(const TRect& aParent, TInt fontid, TInt C, TInt l, TInt r, TInt B, TInt W, TInt J, const CFont* aCustomFont=0);
	iTextLayout[1].LayoutText( sizeTextArea, 0, 0, margin, margin, baseline, width, ELayoutAlignLeft, myFont );
	
	//layout for time-text
	tX = adjustLeftTimeIcon + adjustWidth;
	bX = parentRect.Width();
	TRect timeTextArea( tX, tY, bX, bY ); 
	width = timeTextArea.Width();
	iTextLayout[2].LayoutText( timeTextArea, 0, 0, margin, margin, baseline, width, ELayoutAlignLeft, myFont );
	}


//-------------------------------------------------------------------------------
//CVeiEditVideoLabelNavi::GetMaxMmsSize()
// available max size of the MMS.
//-------------------------------------------------------------------------------
EXPORT_C TInt CVeiEditVideoLabelNavi::GetMaxMmsSize() const
	{
	return iMmsMaxSize/1024;
	}
// End of File
