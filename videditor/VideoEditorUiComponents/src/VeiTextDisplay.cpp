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


// System includes
#include <aknbiditextutils.h>
#include <aknutils.h>
#include <AknsSkinInstance.h>
#include <AknsUtils.h> 
#include <gulfont.h>
#include <VideoEditorUiComponents.rsg>
#include <avkon.rsg>
#include <stringloader.h>
#include <akniconutils.h>
#include <VideoEditorUiComponents.mbg>
#include <data_caging_path_literals.hrh>
#include <aknlayoutscalable_avkon.cdl.h>
#include <aknlayoutscalable_apps.cdl.h>

// User includes
#include "VeiTextDisplay.h"
#include "VideoEditorCommon.h"
#include "VideoEditorUtils.h"
#include "VideoEditorDebugUtils.h"

// CONSTANTS
_LIT(KResourceFile, "VideoEditorUiComponents.rsc");

EXPORT_C CVeiTextDisplay* CVeiTextDisplay::NewL( const TRect& aRect, const CCoeControl* aParent )
    {
    CVeiTextDisplay* self = CVeiTextDisplay::NewLC( aRect, aParent );
    CleanupStack::Pop(self);
    return self;
    }

EXPORT_C CVeiTextDisplay* CVeiTextDisplay::NewLC( const TRect& aRect, const CCoeControl* aParent )
    {
    CVeiTextDisplay* self = new (ELeave) CVeiTextDisplay;
    CleanupStack::PushL( self );
    self->ConstructL( aRect, aParent );
    return self;
    }

void CVeiTextDisplay::ConstructL( const TRect& aRect, const CCoeControl* aParent )
    {
	iClipName = HBufC::NewL( 0 );
	iClipLocation = HBufC::NewL( 0 );

	iLayout = EOnlyName;

	TFileName mbmPath( VideoEditorUtils::IconFileNameAndPath(KVideoEditorUiComponentsIconFileId) );

	AknIconUtils::CreateIconL( iUpperArrow, iUpperArrowMask,
			mbmPath, EMbmVideoeditoruicomponentsQgn_indi_volume_arrow_up, 
			EMbmVideoeditoruicomponentsQgn_indi_volume_arrow_up_mask );

	AknIconUtils::CreateIconL( iLowerArrow, iLowerArrowMask,
			mbmPath, EMbmVideoeditoruicomponentsQgn_indi_volume_arrow_up, 
			EMbmVideoeditoruicomponentsQgn_indi_volume_arrow_up_mask );

	AknIconUtils::CreateIconL( iRightArrow, iRightArrowMask,
		    mbmPath, EMbmVideoeditoruicomponentsQgn_indi_volume_arrow_up, 
			EMbmVideoeditoruicomponentsQgn_indi_volume_arrow_up_mask );
	
	AknIconUtils::CreateIconL( iLeftArrow, iLeftArrowMask,
			mbmPath, EMbmVideoeditoruicomponentsQgn_indi_volume_arrow_up, 
			EMbmVideoeditoruicomponentsQgn_indi_volume_arrow_up_mask );

	AknIconUtils::CreateIconL( iStartMarkIcon, iStartMarkIconMask,
			mbmPath, EMbmVideoeditoruicomponentsQgn_indi_vded_start, 
			EMbmVideoeditoruicomponentsQgn_indi_vded_start_mask );
			
	AknIconUtils::CreateIconL( iEndMarkIcon, iEndMarkIconMask,
			mbmPath, EMbmVideoeditoruicomponentsQgn_indi_vded_end, 
			EMbmVideoeditoruicomponentsQgn_indi_vded_end_mask );			

	iUpperArrowVisible = EFalse;
	iLowerArrowVisible = EFalse;
	iRightArrowVisible = EFalse;
	iLeftArrowVisible = EFalse;

	iSlowMotionOn = EFalse;

	iBlinkTimer = CPeriodic::NewL( CActive::EPriorityLow );
	
	// Open resource file
	TFileName resourceFile;
    Dll::FileName(resourceFile);
    TParse p;
    p.Set(KResourceFile, &KDC_RESOURCE_FILES_DIR, &resourceFile);
    resourceFile = p.FullName();
	iResLoader.OpenL( resourceFile );

	SetContainerWindowL( *aParent );
    SetRect( aRect );
    ActivateL();
    }

CVeiTextDisplay::CVeiTextDisplay() : iResLoader(*CEikonEnv::Static())
	{
	}

EXPORT_C CVeiTextDisplay::~CVeiTextDisplay()
	{
	if ( iClipName )
		{
		delete iClipName;
		}
  
	delete iClipLocation;
    delete iUpperArrow;
    delete iUpperArrowMask;

    delete iLowerArrow;
    delete iLowerArrowMask;

    delete iRightArrow;
    delete iRightArrowMask;

    delete iLeftArrow;
    delete iLeftArrowMask;

	delete iStartMarkIcon;
	delete iStartMarkIconMask;
	
	delete iEndMarkIcon;
	delete iEndMarkIconMask;
	
	if ( iBlinkTimer )
		{
		iBlinkTimer->Cancel();
		delete iBlinkTimer;
		}
	iResLoader.Close();
	}

void CVeiTextDisplay::SizeChanged()
	{
	TRect rect( Rect() );
	const CFont* font = AknLayoutUtils::FontFromId( EAknLogicalFontSecondaryFont );
	TInt fontHeight = font->HeightInPixels();
	
	TSize arrowSize;
	
	if ( iLayout == EArrowsVertical )
		{
		arrowSize = TSize( (rect.Height()-fontHeight) / 2, (rect.Height()-fontHeight) / 2 );
		}
	else
		{
		arrowSize = iDynamicArrowSize;
		}
	
	AknIconUtils::SetSize( iUpperArrow, arrowSize, 
		EAspectRatioNotPreserved );

	AknIconUtils::SetSizeAndRotation( iLowerArrow, 
		arrowSize, EAspectRatioNotPreserved, 180 );

	AknIconUtils::SetSizeAndRotation( iRightArrow, 
		arrowSize, EAspectRatioNotPreserved, 90 );

	AknIconUtils::SetSizeAndRotation( iLeftArrow,
        arrowSize, EAspectRatioNotPreserved, 270 );
        
   	AknIconUtils::SetSize( iStartMarkIcon,
        iStartTimeIconRect.Size(), EAspectRatioNotPreserved);    
        
   	AknIconUtils::SetSize( iEndMarkIcon,
        iEndTimeIconRect.Size(), EAspectRatioNotPreserved);            
        
	TInt upperArrowY = ( (rect.iBr.iY - rect.Height() / 2) - fontHeight / 2) - iUpperArrow->SizeInPixels().iHeight;
	TInt upperArrowX = (rect.iBr.iX - rect.Width() / 2) - iUpperArrow->SizeInPixels().iWidth / 2;

	TInt lowerArrowY = (rect.iBr.iY - rect.Height() / 2) + fontHeight / 2;

	iUpperArrowPoint.SetXY( upperArrowX, upperArrowY - 2 );			
	iLowerArrowPoint.SetXY( upperArrowX, lowerArrowY );
	}

EXPORT_C void CVeiTextDisplay::SetName( const TDesC& aName )
	{
	if ( iClipName )
		{
		delete iClipName;
		iClipName = NULL;
		}

	TRAP_IGNORE(
		iClipName = HBufC::NewL( aName.Length() );
		*iClipName = aName );

	DrawDeferred();
	}


EXPORT_C void CVeiTextDisplay::SetDuration( const TTimeIntervalMicroSeconds& aDuration )
	{
	iDuration = aDuration;
	DrawDeferred();
	}


EXPORT_C void CVeiTextDisplay::SetTime( const TTime& aClipTime )
	{
	iClipTime = aClipTime;
	}

EXPORT_C void CVeiTextDisplay::SetLocation( const TDesC& aClipLocation )
	{
	if ( iClipLocation )
		{
		delete iClipLocation;
		iClipLocation = NULL;
		}

	TRAP_IGNORE(
		iClipLocation = HBufC::NewL( aClipLocation.Length() );
		*iClipLocation = aClipLocation );

	DrawDeferred();
	}

EXPORT_C void CVeiTextDisplay::SetLandscapeScreenOrientation( TBool aLandscapeScreenOrientation )
	{
	iLandscapeScreenOrientation = aLandscapeScreenOrientation;

	DrawDeferred();
	}

EXPORT_C void CVeiTextDisplay::SetCutIn( const TTimeIntervalMicroSeconds& aCutInTime )
									
	{
	iCutInTime = aCutInTime;
	DrawDeferred();
	}
EXPORT_C void CVeiTextDisplay::SetCutOut(  const TTimeIntervalMicroSeconds& aCutOutTime )
	{
	iCutOutTime = aCutOutTime;
	DrawDeferred();
	}

EXPORT_C void CVeiTextDisplay::SetLayout( TVeiLayout aLayout )
	{
	iLayout = aLayout;

	if ( iBlinkTimer->IsActive() )
		{
		iBlinkTimer->Cancel();
		}
	iBlinkFlag = ETrue;

	if ( iLayout == ERecordingPaused )
		{
		const TUint delay = 350000;
		iBlinkTimer->Start( delay, delay, TCallBack( CVeiTextDisplay::UpdateBlinker, this ) );
		}
	DrawDeferred();
	}

TInt CVeiTextDisplay::UpdateBlinker( TAny* aThis )
	{
	STATIC_CAST( CVeiTextDisplay*, aThis )->DoUpdateBlinker();
	return 1;
	}

void CVeiTextDisplay::DoUpdateBlinker()
	{
	if ( iBlinkFlag )
		iBlinkFlag = EFalse;
	else
		iBlinkFlag = ETrue;

	DrawDeferred();
	}

EXPORT_C void CVeiTextDisplay::SetUpperArrowVisibility(TBool aVisible)
	{
	iUpperArrowVisible = aVisible;
	}

EXPORT_C void CVeiTextDisplay::SetLowerArrowVisibility(TBool aVisible)
	{
	iLowerArrowVisible = aVisible;		
	}

EXPORT_C void CVeiTextDisplay::SetRightArrowVisibility(TBool aVisible)
	{
	iRightArrowVisible = aVisible;	
	}

EXPORT_C void CVeiTextDisplay::SetLeftArrowVisibility(TBool aVisible)
	{
	iLeftArrowVisible = aVisible;	
	}

EXPORT_C void CVeiTextDisplay::SetSlowMotionOn(TBool aOn)
	{
	iSlowMotionOn = aOn;
	}

EXPORT_C TBool CVeiTextDisplay::SlowMotionOn() const
	{
	return iSlowMotionOn;
	}

EXPORT_C void CVeiTextDisplay::SetSlowMotionPreset(TInt aPreset)
	{
	iPresetValue = aPreset;	
	DrawDeferred();
	}

EXPORT_C TInt CVeiTextDisplay::SlowMotionPreset() const
	{
	return iPresetValue;	
	}

EXPORT_C void CVeiTextDisplay::SetArrowSize(const TSize& aArrowSize)
	{
	iDynamicArrowSize = aArrowSize;
	}

void CVeiTextDisplay::Draw( const TRect& /*aRect*/ ) const
    {
    CWindowGc& gc = SystemGc();
	
	const CFont* font = AknLayoutUtils::FontFromId( EAknLogicalFontSecondaryFont );
	TFileName visualText;
	TPoint textPoint;
	TPoint persentPoint;
	TBuf<60> layoutTime;



	// Get text color from skin
	TRgb textColor( KRgbBlack );
	MAknsSkinInstance* skin = AknsUtils::SkinInstance();
	AknsUtils::GetCachedColor(skin, textColor, KAknsIIDQsnTextColors, EAknsCIQsnTextColorsCG6 );
	gc.SetPenColor( textColor );

	if ( (iLayout == EOnlyName) && (iClipName && iClipName->Length() > 0) )
		{
		gc.UseFont( font );

		TInt maxWidthNonClipping = Rect().Width() - 20;

		AknBidiTextUtils::ConvertToVisualAndClip( *iClipName, 
			visualText, *font, maxWidthNonClipping, maxWidthNonClipping + 10 );

		TInt xOffset = ( Rect().Width() - font->TextWidthInPixels( visualText ) ) / 2;
		if( (xOffset < 0) || iLandscapeScreenOrientation )
			{
			xOffset = 0;
			}

		textPoint.iX = Rect().iTl.iX + xOffset;

		gc.DrawText( visualText, textPoint );
	
		gc.DiscardFont();
		}
	else if (((iLayout == ENameAndDuration) || (iLayout == ERecording) || 
		(iLayout == ERecordingPaused)) && !iLandscapeScreenOrientation )
		{
		gc.UseFont( font );

		TBuf<10> durationValue;

		TInt maxWidthNonClipping = Rect().Width() - 20;

		ParseTimeToMinSec( layoutTime, iDuration );
		durationValue.Append( layoutTime);
		
		TFileName nameAndDuration;
		if (iClipName)
			{
			nameAndDuration.Append( *iClipName );
			}

		TInt durationWidthValue = font->TextWidthInPixels(durationValue);
		
		AknBidiTextUtils::ConvertToVisualAndClip( nameAndDuration, visualText, *font, 
			maxWidthNonClipping - durationWidthValue,
			maxWidthNonClipping );
		
		LOGFMT(KVideoEditorLogFile, "CVeiTextDisplay::Draw: duration width: %d", durationWidthValue);


		visualText.Append( _L(" ") );
		visualText.Append( durationValue );

		textPoint.iX = Rect().iTl.iX
			+ ( Rect().Width() - font->TextWidthInPixels( visualText ) ) / 2;
	
		if ((iLayout == ERecording) || (iLayout == ERecordingPaused))
			{
			if ( iBlinkFlag )
				gc.SetPenColor( KRgbRed );
			else
				{
				gc.DiscardFont();
				return;
				}
			}
		gc.DrawText( visualText, textPoint );
	
		gc.DiscardFont();
		}
	else if (iLayout == ECutInCutOut)
		{
		gc.UseFont( font );

		TBuf<40> cutInValue;
		TBuf<40> cutOutValue;
		TFileName visualText2;

		ParseTimeToMinSec( layoutTime, iCutInTime );
		cutInValue.Append( layoutTime );
		ParseTimeToMinSec( layoutTime, iCutOutTime );
		cutOutValue.Append( layoutTime );

		AknBidiTextUtils::ConvertToVisualAndClip( cutInValue, visualText, *font, iStartTimeTextRect.Width(),
			iStartTimeTextRect.Width());

		AknBidiTextUtils::ConvertToVisualAndClip( cutOutValue, visualText2, *font, iEndTimeTextRect.Width(),
			iEndTimeTextRect.Width() );

		TInt cutInWidth = font->TextWidthInPixels( visualText );
		TInt cutOutWidth = font->TextWidthInPixels( visualText2 );
		TInt marginWidth( font->MaxCharWidthInPixels() /2 );

		gc.DrawText( visualText, iStartTimeTextRect, font->AscentInPixels(), CGraphicsContext::ELeft, 0  );
		gc.DrawText( visualText2, iEndTimeTextRect, font->AscentInPixels(), CGraphicsContext::ELeft, 0 );			
		gc.DiscardFont();		

		gc.BitBltMasked( iStartTimeIconRect.iTl, iStartMarkIcon, iStartTimeIconRect.Size(),
						iStartMarkIconMask, EFalse);
		gc.BitBltMasked( iEndTimeIconRect.iTl, iEndMarkIcon, iEndTimeIconRect.Size(),
					iEndMarkIconMask, EFalse);									
		
		}
	else if (((iLayout == ENameAndDuration ) ||(iLayout == EEverything) || (iLayout == ERecording) || 
		(iLayout == ERecordingPaused)) && iLandscapeScreenOrientation )
		{
		gc.UseFont( font );
		textPoint.iX = Rect().iTl.iX;

		TBuf<24> dateFormatString;
		TBuf<24> timeFormatString;
		StringLoader::Load( dateFormatString, R_QTN_DATE_USUAL_WITH_ZERO, iEikonEnv ); 
		StringLoader::Load( timeFormatString, R_QTN_TIME_LONG_WITH_ZERO, iEikonEnv );

		TBuf<50> dateValue;
		TBuf<40> timeValue;
		TBuf<40> durationValue;

		// these should not fail...
		TRAPD(err1, iClipTime.FormatL( dateValue, dateFormatString ));
		if (KErrNone != err1)
			{
			dateValue.Zero();
			}
		TRAP(err1, iClipTime.FormatL( timeValue, timeFormatString ));
		if (KErrNone != err1)
			{
			timeValue.Zero();
			}

		ParseTimeToMinSec( layoutTime, iDuration );
		durationValue.Append( layoutTime );

		TBuf<64> durString; 
		StringLoader::Load( durString, R_VEI_EDIT_VIDEO_DURATION, iEikonEnv );
		durationValue.Insert( 0, durString );

		TInt maxWidthNonClipping = Rect().Width() - 10;
		/* name */
		if (iClipName)
			{
			AknBidiTextUtils::ConvertToVisualAndClip( *iClipName, visualText, *font, 
				maxWidthNonClipping, maxWidthNonClipping + 10 );

			if ((iLayout == ERecording) || (iLayout == ERecordingPaused))
				{
				if ( iBlinkFlag )
					{
					gc.SetPenColor( KRgbRed );
					gc.DrawText( visualText, textPoint );
					}
				}
			else
				{
				gc.DrawText( visualText, textPoint );
				}
			gc.SetPenColor( textColor );
			}

		/* date */
		AknBidiTextUtils::ConvertToVisualAndClip( dateValue, visualText, *font, 
			maxWidthNonClipping, maxWidthNonClipping + 10 );

		textPoint.iY += font->HeightInPixels() + 3;
		gc.DrawText( visualText, textPoint );
		/* time */
		AknBidiTextUtils::ConvertToVisualAndClip( timeValue, visualText, *font, 
			maxWidthNonClipping, maxWidthNonClipping + 10 );

		textPoint.iY += font->HeightInPixels() + 3;
		gc.DrawText( visualText, textPoint );
		/* location */
		if( iClipLocation && iClipLocation->Length() > 0 )
			{
			AknBidiTextUtils::ConvertToVisualAndClip( *iClipLocation, visualText, *font, 
			maxWidthNonClipping, maxWidthNonClipping + 10 );

			textPoint.iY += font->HeightInPixels() + 3;
			gc.DrawText( visualText, textPoint );
			}
		/* duration */
		AknBidiTextUtils::ConvertToVisualAndClip( durationValue, visualText, *font, 
		maxWidthNonClipping, maxWidthNonClipping + 10 );

		textPoint.iY += font->HeightInPixels() + 3;
		gc.DrawText( visualText, textPoint );	

		gc.DiscardFont();
		}
	else if ( iLayout == EArrowsVertical )
		{
		// ** Transition ** 	
		gc.UseFont( font );
	
		TInt maxWidthNonClipping = Rect().Width()-20;

		if (iClipName)
			{
			AknBidiTextUtils::ConvertToVisualAndClip( *iClipName, visualText, *font, maxWidthNonClipping,
				maxWidthNonClipping+10);

			TInt xOffset = ( Rect().Width() - font->TextWidthInPixels( visualText ) ) / 2;
			if( xOffset < 0)
				{
				xOffset = 0;
				}

			textPoint.iX = Rect().iTl.iX + xOffset;
			textPoint.iY = (Rect().iBr.iY - iLowerArrow->SizeInPixels().iHeight) - 2;

			gc.DrawText( visualText, textPoint );	
			gc.DiscardFont();
			}

		TPoint upperArrowPos( Rect().iTl );
		if(iUpperArrowVisible)
			{				
			TRect upArrowIconSourceRect(0, 0, iUpperArrow->SizeInPixels().iWidth, 
				iUpperArrow->SizeInPixels().iHeight);

			gc.BitBltMasked( iUpperArrowPoint, iUpperArrow, upArrowIconSourceRect,
					iUpperArrowMask, EFalse);

			}
			
		if(iLowerArrowVisible)
			{
			TRect downArrowIconSourceRect(0, 0, iLowerArrow->SizeInPixels().iWidth, 
				iLowerArrow->SizeInPixels().iHeight);

			gc.BitBltMasked( iLowerArrowPoint, iLowerArrow, downArrowIconSourceRect,
					iLowerArrowMask, EFalse);
			}				
		}

	else if ( iLayout == EArrowsHorizontal )
		{
		// ** SlowMotion **
		if(iSlowMotionOn)
			{
			gc.UseFont( font );		
			visualText.Format( _L("%d"),iPresetValue );			

			// *** % char added in asciicode format
			visualText.Append(37);

			textPoint.iY = Rect().iTl.iY;					
			textPoint.iY = textPoint.iY + font->HeightInPixels();
			TInt NumberWidthInPixels = font->TextWidthInPixels( visualText );

			// TRect's middle point
			textPoint.iX = Rect().iTl.iX + (Rect().iBr.iX - Rect().iTl.iX) / 2; 
			textPoint.iX =  textPoint.iX - NumberWidthInPixels / 2;
			gc.DrawText( visualText, textPoint );
			gc.DiscardFont();
			
			if(iLeftArrowVisible)
				{
				TPoint leftArrowPos( Rect().iTl.iX, ( Rect().iBr.iY - Rect().Height() / 2 ) -
					iLeftArrow->SizeInPixels().iHeight / 2 );		
				
				TRect leftArrowIconSourceRect(0, 0, iLeftArrow->SizeInPixels().iWidth, 
				iLeftArrow->SizeInPixels().iHeight);

				gc.BitBltMasked( leftArrowPos, iLeftArrow, leftArrowIconSourceRect,
					iLeftArrowMask, EFalse);
				}		
		
			if(iRightArrowVisible)
				{
				TPoint rightArrowPos( Rect().iBr.iX - iRightArrow->SizeInPixels().iWidth, 
					( Rect().iBr.iY - Rect().Height() / 2 ) - iLeftArrow->SizeInPixels().iHeight / 2 );						
								
				TRect rightArrowIconSourceRect(0, 0, iRightArrow->SizeInPixels().iWidth, 
				iRightArrow->SizeInPixels().iHeight);

				gc.BitBltMasked( rightArrowPos, iRightArrow, rightArrowIconSourceRect,
					iRightArrowMask, EFalse);
				}	
			}
		else
			{
		// ** Trim for MMS **
			TPoint rightArrowPos( Rect().iTl );
			if(iRightArrowVisible)
				{	
				rightArrowPos.SetXY(  Rect().iBr.iX, Rect().iBr.iY - iRightArrow->SizeInPixels().iHeight );

				TRect rightArrowIconSourceRect(0, 0, iRightArrow->SizeInPixels().iWidth, 
				iRightArrow->SizeInPixels().iHeight);

				gc.BitBltMasked( rightArrowPos, iRightArrow, rightArrowIconSourceRect,
					iRightArrowMask, EFalse);
				}
			TPoint leftArrowPos( Rect().iTl);
			if(iLeftArrowVisible)
				{
				leftArrowPos.SetXY( Rect().iTl.iX - iLeftArrow->SizeInPixels().iWidth,  rightArrowPos.iY ); 
			
				TRect leftArrowIconSourceRect(0, 0, iLeftArrow->SizeInPixels().iWidth, 
				iLeftArrow->SizeInPixels().iHeight);

				gc.BitBltMasked( leftArrowPos, iLeftArrow, leftArrowIconSourceRect,
					iLeftArrowMask, EFalse);
				}	

			if (iClipName)
				{
				gc.UseFont( font );

				persentPoint.iX = Rect().iTl.iX + Rect().Width() / 2;
				persentPoint.iX = persentPoint.iX - (font->TextWidthInPixels( *iClipName ) ) / 2;

				persentPoint.iY = rightArrowPos.iY + iLeftArrow->SizeInPixels().iHeight;
				persentPoint.iY = persentPoint.iY;

				TInt maxWidthNonClipping = Rect().Width() - 20;
				AknBidiTextUtils::ConvertToVisualAndClip( *iClipName, visualText, *font, maxWidthNonClipping, maxWidthNonClipping + 20 );

				gc.DrawText( visualText, persentPoint );	
				gc.DiscardFont();
				}
			}
		}						
	}

void CVeiTextDisplay::ParseTimeToMinSec( TDes& aLayoutTime, const TTimeIntervalMicroSeconds& aDuration ) const 
	{
	//minutes and seconds "120:13"
	aLayoutTime.Zero();
	TBuf<30> minsec;  
	TInt64 duration = 0; 
	TTimeIntervalMinutes minutes; 
	TTimeIntervalMicroSeconds32 seconds; 
	duration = ( aDuration.Int64() / 1000 ); 

	TChar timeSeparator = TLocale().TimeSeparator(2);
	//over 1 minute
	if( duration >= 60000 ) 
		{ 
		minutes = TTimeIntervalMinutes (static_cast<TInt32>(duration) / 60000 ); 
		minsec.AppendNum( minutes.Int() ); 
		minsec.Append( timeSeparator ); 

		duration = duration - TInt64(minutes.Int()) * TInt64(60000); 
		}
	else
		{
		minsec.Append( _L( "0" ) ); 
		minsec.Append( timeSeparator ); 
		}	
	if( duration >= 1000 ) 
		{ 
		seconds = TTimeIntervalMicroSeconds32 (static_cast<TInt32>(duration) / 1000 ); 

		if( seconds.Int() >= 60 ) 
			{ 
			minsec.AppendNum( seconds.Int() - 60 ); 
			} 
		else 
			{ 
			if ( seconds.Int() < 10 ) 
				{ 
				minsec.Append( _L("0") ); 
				} 
     
			minsec.AppendNum( seconds.Int() ); 
			} 
		}
	else 
		{ 
		minsec.Append( _L("00") ); 
		} 
	aLayoutTime.Append( minsec );

	AknTextUtils::DisplayTextLanguageSpecificNumberConversion( aLayoutTime );
	}

// ----------------------------------------------------------------------------
// CVeiTextDisplay::SetComponentRect
// 
// ----------------------------------------------------------------------------
//	
EXPORT_C void CVeiTextDisplay::SetComponentRect(TTextDisplayComponent aComponentIndex, TRect aRect)
	{
    switch ( aComponentIndex )
        {
        case EStartTimeText:
        	{
       		iStartTimeTextRect = aRect;
       		break;
        	}
        case EEndTimeText:
        	{
    		iEndTimeTextRect= aRect;
    		break;    	
        	}        	        
        case EStartTimeIcon:
        	{
       		iStartTimeIconRect = aRect;
       		break;
        	}
        case EEndTimeIcon:
        	{
    		iEndTimeIconRect= aRect;
    		break;    	
        	}
        }
	}

// End of File
