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


// INCLUDE FILES

// System includes
#include <eikenv.h>
#include <VideoEditorUiComponents.mbg>
#include <akniconutils.h>
#include <aknsdrawutils.h> 
#include <aknsdatacontext.h> 
#include <aknutils.h>
#include <aknlayoutscalable_avkon.cdl.h>
#include <aknlayoutscalable_apps.cdl.h>


// User includes
#include "VeiCutterBar.h"
#include "VideoEditorUtils.h"


EXPORT_C CVeiCutterBar* CVeiCutterBar::NewL( const CCoeControl* aParent, TBool aDrawBorder )
    {
    CVeiCutterBar* self = CVeiCutterBar::NewLC( aParent, aDrawBorder );
    CleanupStack::Pop(self);
    return self;
    }

EXPORT_C CVeiCutterBar* CVeiCutterBar::NewLC( const CCoeControl* aParent, TBool aDrawBorder )
    {
    CVeiCutterBar* self = new (ELeave) CVeiCutterBar;
    CleanupStack::PushL( self );
    self->ConstructL( aParent, aDrawBorder );
    return self;
    }

void CVeiCutterBar::ConstructL( const CCoeControl* aParent, TBool aDrawBorder )
    {
	SetContainerWindowL( *aParent );

	TFileName mbmPath( VideoEditorUtils::IconFileNameAndPath(KVideoEditorUiComponentsIconFileId) );

	if( !AknLayoutUtils::PenEnabled() )
		{
		AknIconUtils::CreateIconL( iScissorsIcon, iScissorsIconMask,
				mbmPath, EMbmVideoeditoruicomponentsQgn_indi_vded_scissors, 
				EMbmVideoeditoruicomponentsQgn_indi_vded_scissors_mask );

	    // left end of the slider when that part is unselected
		AknIconUtils::CreateIconL( iSliderLeftEndIcon, iSliderLeftEndIconMask,
				mbmPath, EMbmVideoeditoruicomponentsQgn_graf_nslider_vded_end_left, 
				EMbmVideoeditoruicomponentsQgn_graf_nslider_vded_end_left_mask );
				
		// middle part of the slider when that part is unselected	
		// should be qgn_graf_nslider_vded_middle but that icon is currently incorrect	
		AknIconUtils::CreateIconL( iSliderMiddleIcon, iSliderMiddleIconMask,
				mbmPath, EMbmVideoeditoruicomponentsQgn_graf_nslider_vded_end_middle, 
				EMbmVideoeditoruicomponentsQgn_graf_nslider_vded_end_middle_mask );	
				
		// right end of the slider when that part is unselected		
		AknIconUtils::CreateIconL( iSliderRightEndIcon, iSliderRightEndIconMask,
				mbmPath, EMbmVideoeditoruicomponentsQgn_graf_nslider_vded_end_right, 
				EMbmVideoeditoruicomponentsQgn_graf_nslider_vded_end_right_mask );					

	    // left end of the cut selection slider 
		AknIconUtils::CreateIconL( iSliderSelectedLeftEndIcon, iSliderSelectedLeftEndIconMask,
				mbmPath, EMbmVideoeditoruicomponentsQgn_graf_nslider_vded_end_left_selected, 
				EMbmVideoeditoruicomponentsQgn_graf_nslider_vded_end_left_selected_mask );					

	    // middle part of the cut selection slider 
		AknIconUtils::CreateIconL( iSliderSelectedMiddleIcon, iSliderSelectedMiddleIconMask,
				mbmPath, EMbmVideoeditoruicomponentsQgn_graf_nslider_vded_middle_selected, 
				EMbmVideoeditoruicomponentsQgn_graf_nslider_vded_middle_selected_mask );					

	    // right end of the cut selection slider 
		AknIconUtils::CreateIconL( iSliderSelectedRightEndIcon, iSliderSelectedRightEndIconMask,
				mbmPath, EMbmVideoeditoruicomponentsQgn_graf_nslider_vded_end_right_selected, 
				EMbmVideoeditoruicomponentsQgn_graf_nslider_vded_end_right_selected_mask );					
	    
	    // playhead
		AknIconUtils::CreateIconL( iPlayheadIcon, iPlayheadIconMask,
				mbmPath, EMbmVideoeditoruicomponentsQgn_indi_vded_playhead,
				EMbmVideoeditoruicomponentsQgn_indi_vded_playhead_mask );					
	    
	    // left/right border of cut selection slider
		AknIconUtils::CreateIconL( iCutAreaBorderIcon, iCutAreaBorderIconMask,
				mbmPath, EMbmVideoeditoruicomponentsQgn_indi_vded_timeline_selected,
				EMbmVideoeditoruicomponentsQgn_indi_vded_timeline_selected_mask );								
		}
	else
		{
	    // left end of the slider when that part is unselected
		AknIconUtils::CreateIconL( iSliderLeftEndIcon, iSliderLeftEndIconMask,
				mbmPath, EMbmVideoeditoruicomponentsQgn_graf_nslider_vded2_end_left, 
				EMbmVideoeditoruicomponentsQgn_graf_nslider_vded2_end_left_mask );
				
		// middle part of the slider when that part is unselected	
		// should be qgn_graf_nslider_vded_middle but that icon is currently incorrect	
		AknIconUtils::CreateIconL( iSliderMiddleIcon, iSliderMiddleIconMask,
				mbmPath, EMbmVideoeditoruicomponentsQgn_graf_nslider_vded2_middle, 
				EMbmVideoeditoruicomponentsQgn_graf_nslider_vded2_middle_mask );	
				
		// right end of the slider when that part is unselected		
		AknIconUtils::CreateIconL( iSliderRightEndIcon, iSliderRightEndIconMask,
				mbmPath, EMbmVideoeditoruicomponentsQgn_graf_nslider_vded2_end_right, 
				EMbmVideoeditoruicomponentsQgn_graf_nslider_vded2_end_right_mask );					

	    // middle part of the cut selection slider 
		AknIconUtils::CreateIconL( iSliderSelectedMiddleIcon, iSliderSelectedMiddleIconMask,
				mbmPath, EMbmVideoeditoruicomponentsQgn_graf_nslider_vded2_middle_selected, 
				EMbmVideoeditoruicomponentsQgn_graf_nslider_vded2_middle_selected_mask );					
	    
	    // playhead
		AknIconUtils::CreateIconL( iPlayheadIcon, iPlayheadIconMask,
				mbmPath, EMbmVideoeditoruicomponentsQgn_indi_vded2_playhead,
				EMbmVideoeditoruicomponentsQgn_indi_vded2_playhead_mask );					
	    
	    // pressed playhead
		AknIconUtils::CreateIconL( iPlayheadIconPressed, iPlayheadIconPressedMask,
				mbmPath, EMbmVideoeditoruicomponentsQgn_indi_vded2_playhead_pressed,
				EMbmVideoeditoruicomponentsQgn_indi_vded2_playhead_pressed_mask );	
				
	    // Start mark
		AknIconUtils::CreateIconL( iStartMarkIcon, iStartMarkIconMask,
				mbmPath, EMbmVideoeditoruicomponentsQgn_indi_vded2_start,
				EMbmVideoeditoruicomponentsQgn_indi_vded2_start_mask );								
        
        // Pressed Start mark
		AknIconUtils::CreateIconL( iStartMarkIconPressed, iStartMarkIconPressedMask,
				mbmPath, EMbmVideoeditoruicomponentsQgn_indi_vded2_start_pressed,
				EMbmVideoeditoruicomponentsQgn_indi_vded2_start_pressed_mask );								

	    // End Mark
		AknIconUtils::CreateIconL( iEndMarkIcon, iEndMarkIconMask,
				mbmPath, EMbmVideoeditoruicomponentsQgn_indi_vded2_end,
				EMbmVideoeditoruicomponentsQgn_indi_vded2_end_mask );
		
		// Pressed End Mark
		AknIconUtils::CreateIconL( iEndMarkIconPressed, iEndMarkIconPressedMask,
				mbmPath, EMbmVideoeditoruicomponentsQgn_indi_vded2_end_pressed,
				EMbmVideoeditoruicomponentsQgn_indi_vded2_end_pressed_mask );										
		}

	iDrawBorder = aDrawBorder;
	iInPoint = 0;
	iCurrentPoint = 0;
	iTotalDuration = 1;
	iDimmed = EFalse;
	iDrawPlayHead = ETrue;
	iOutPoint = 13;
	iFinished = ETrue;
    }

EXPORT_C CVeiCutterBar::~CVeiCutterBar()
	{
	if (iScissorsIcon)
		{
		delete iScissorsIcon;
		delete iScissorsIconMask;
		}

	delete iSliderLeftEndIcon;
	delete iSliderLeftEndIconMask;
	delete iSliderMiddleIcon;
	delete iSliderMiddleIconMask;
	delete iSliderRightEndIcon;
	delete iSliderRightEndIconMask;	

	if (iSliderSelectedLeftEndIcon)
		{
		delete iSliderSelectedLeftEndIcon;
		delete iSliderSelectedLeftEndIconMask;
		}

	delete iSliderSelectedMiddleIcon;
	delete iSliderSelectedMiddleIconMask;		

	if (iSliderSelectedRightEndIcon)
		{
		delete iSliderSelectedRightEndIcon;
		delete iSliderSelectedRightEndIconMask;
		}

	delete iPlayheadIcon;
	delete iPlayheadIconMask;
    
    if (iPlayheadIconPressed)
		{
		delete iPlayheadIconPressed;
		delete iPlayheadIconPressedMask;		
		}    
	if (iStartMarkIcon)
		{
		delete iStartMarkIcon;
		delete iStartMarkIconMask;		
		}
    if (iStartMarkIconPressed)
		{
		delete iStartMarkIconPressed;
		delete iStartMarkIconPressedMask;		
		}
	if (iEndMarkIcon)
		{
		delete iEndMarkIcon;
		delete iEndMarkIconMask;		
		}
    if (iEndMarkIconPressed)
		{
		delete iEndMarkIconPressed;
		delete iEndMarkIconPressedMask;		
		}
	if (iCutAreaBorderIcon)
		{
		delete iCutAreaBorderIcon;
		delete iCutAreaBorderIconMask;				
		}
	}

TInt CVeiCutterBar::CountComponentControls() const
    {
    return 0;
    }

void CVeiCutterBar::DrawCoordinate(TInt aX, TInt aY, TInt aData1, TInt aData2, const TDesC& aInfo) const
{
	CWindowGc& gc = SystemGc();
	TPoint writepoint(aX, aY);

	_LIT(KXY, "%S:(%d,%d)");
	_LIT(KX, "%S:%d");
	TBuf<200> buffer;
	if (aData2 > 0)
		{
		buffer.Format(KXY, &aInfo, aData1, aData2);
		}
		else
		{
		buffer.Format(KX, &aInfo, aData1);
		}
	const CFont* font = AknLayoutUtils::FontFromId( EAknLogicalFontSecondaryFont );
	gc.UseFont( font );	

	// Get text color from skin
	TRgb textColor( KRgbBlack );
	MAknsSkinInstance* skin = AknsUtils::SkinInstance();
	AknsUtils::GetCachedColor(skin, textColor, KAknsIIDQsnTextColors, EAknsCIQsnTextColorsCG6 );
	gc.SetPenColor( textColor );

	gc.DrawText( buffer, writepoint );

	gc.DiscardFont();
}


void CVeiCutterBar::Draw( const TRect& /*aRect*/ ) const
    {
    CWindowGc& gc = SystemGc();

	if( !AknLayoutUtils::PenEnabled() )
		{
		gc.BitBltMasked(iScissorsIconRect.iTl, iScissorsIcon, iScissorsIconRect.Size(), 
				iScissorsIconMask, EFalse );	
		}

    // The following three icons are visible only when the user has selected an area to be cut.
    // In the initial state, the whole slider is selected so these icons won't be shown.

    // draw left end of the slider 
	gc.BitBltMasked(iSliderLeftEndIconRect.iTl, iSliderLeftEndIcon, iSliderLeftEndIconRect.Size(), 
			iSliderLeftEndIconMask, EFalse );				

	// draw middle part of the slider 
	gc.BitBltMasked(iSliderMiddleIconRect.iTl, iSliderMiddleIcon, iSliderMiddleIconRect.Size(), 
			iSliderMiddleIconMask, EFalse );							

	// draw right end of the slider 
	gc.BitBltMasked(iSliderRightEndIconRect.iTl, iSliderRightEndIcon, iSliderRightEndIconRect.Size(), 
			iSliderRightEndIconMask, EFalse );							

    //  the selected area of the slider is constructed from 3 icons:
    //
    //  |<-------------- whole slider ------------------------>|
    //
    //        |<-------- selected area ------------>|
    //
    //   ----- -----a-------------------------b----- ----------
    //  |     |left |          middle         |right|          |
    //  |     |end  |           part          |end  |          |
    //   ----- ----- ------------------------- ----- ---------- 
      
    TRect selectedArea(CalculateCutAreaRect());  

	if( !AknLayoutUtils::PenEnabled() )
		{
		// if the start and end mark are close to each other, the width of the left and
		// right end icons has to be decreased	
	    TRect leftRect(iSliderSelectedLeftEndIconRect); 
	    TRect rightRect(iSliderSelectedRightEndIconRect); 
	    
		// the width of the selected area is smaller than the width of the left/right end
		if (selectedArea.Width() <= iSliderSelectedRightEndIconRect.Width()) 
			{ 	    
			leftRect = selectedArea; 
			rightRect = selectedArea; 
			}    	        

	    // calculate point a
	    TInt leftEndWidth = iSliderSelectedLeftEndIconRect.Width();
	    TInt xA = selectedArea.iTl.iX + leftEndWidth;
	    TPoint pointA = TPoint(xA, selectedArea.iTl.iY);
	    
	    // calculate point b
	    TInt rightEndWidth = rightRect.Width();
	    TInt xB = selectedArea.iBr.iX - rightEndWidth;
	    TPoint pointB = TPoint(xB, selectedArea.iTl.iY);    

	    // calculate the size of the middle part
	    TInt middlePartWidth = pointB.iX - pointA.iX;
	    TSize middlePartSize = TSize(middlePartWidth, iSliderSelectedMiddleIconRect.Height());
	    
	    // draw left end of the cut selection slider
		gc.BitBltMasked(selectedArea.iTl, iSliderSelectedLeftEndIcon, leftRect.Size(), 
				iSliderSelectedLeftEndIconMask, EFalse );				
							
		// draw middle part of the cut selection slider
		gc.BitBltMasked(pointA, iSliderSelectedMiddleIcon, middlePartSize, 
				iSliderSelectedMiddleIconMask, EFalse );							
				
		// draw right end of the cut selection slider
		gc.BitBltMasked(pointB, iSliderSelectedRightEndIcon, rightRect.Size(), 
				iSliderSelectedRightEndIconMask, EFalse );							

		if ( iFinished && !iDimmed )
			{    
	        // draw the left border of cut selection slider if the start mark has been set
	        if ( iInPoint > 0 )  
	            {	
	        	gc.BitBltMasked(iStartMarkRect.iTl, iCutAreaBorderIcon, iCutAreaBorderIconRect.Size(), 
	        			iCutAreaBorderIconMask, EFalse );										                
	            }

	        // draw the right border of the cut selection slider if the end mark has been set
	        if (iOutPoint < iTotalDuration)
	            {
	        	gc.BitBltMasked(iEndMarkRect.iTl, iCutAreaBorderIcon, iCutAreaBorderIconRect.Size(), 
	        			iCutAreaBorderIconMask, EFalse );													                
	            }
			}
		}
	else
		{
		// draw middle part of the cut selection slider
		gc.BitBltMasked(selectedArea.iTl, iSliderSelectedMiddleIcon, selectedArea.Size(), 
				iSliderSelectedMiddleIconMask, EFalse );							

		if ( iFinished && !iDimmed )
			{    
			TPoint startPoint(selectedArea.iTl);
			startPoint.iX = startPoint.iX - iStartMarkRect.Width();
        	
        	if( iPressedComponent == EPressedStartMarkTouch )
	            {
        	    gc.BitBltMasked(startPoint, iStartMarkIconPressed, iStartMarkRect.Size(), 
        			    iStartMarkIconPressedMask, EFalse );										                
	            }
	        else
	            {
	            gc.BitBltMasked(startPoint, iStartMarkIcon, iStartMarkRect.Size(), 
        			    iStartMarkIconMask, EFalse );
	            }    
	            
			TPoint endPoint(selectedArea.iBr);
			endPoint.iY = iEndMarkRect.iTl.iY;
			
			if( iPressedComponent == EPressedEndMarkTouch )
	            {
	            gc.BitBltMasked(endPoint, iEndMarkIconPressed, iEndMarkRect.Size(), 
        			iEndMarkIconPressedMask, EFalse );
	            }
	        else
	            {
	            gc.BitBltMasked(endPoint, iEndMarkIcon, iEndMarkRect.Size(), 
        			iEndMarkIconMask, EFalse );
	            }
        														                
			}
		}

    // calculate the playhead position
    TUint width = iCutBarRect.Width();
    TInt currentPointX = 0;
    if (iTotalDuration > 0)
        {
        currentPointX = iCurrentPoint * width / iTotalDuration + iCutBarRect.iTl.iX;   
         
        // set the center of the playhead icon to the current position
        currentPointX = currentPointX - ( iPlayheadIconRect.Width()/2 );         
        }

	if( !AknLayoutUtils::PenEnabled() )
		{
	    // don't draw the playhead outside the cut bar area because it is not refreshed often enough
	    if ( currentPointX < iCutBarRect.iTl.iX )
	        {
	        currentPointX = iCutBarRect.iTl.iX;
	        }
	    else if ( currentPointX > iCutBarRect.iBr.iX )
	        {
	        currentPointX = iCutBarRect.iBr.iX;
	        }

	    // draw playhead
		gc.BitBltMasked(TPoint(currentPointX,iCutBarRect.iTl.iY), iPlayheadIcon, iPlayheadIconRect.Size(), 
				iPlayheadIconMask, EFalse );											
		}
	else
		{
	    // draw playhead	    
	    if( iPressedComponent == EPressedPlayheadTouch )
	        {
	        gc.BitBltMasked(TPoint(currentPointX,iCutBarRect.iBr.iY - iPlayheadIconRect.Height()), iPlayheadIconPressed, 
	            iPlayheadIconRect.Size(), iPlayheadIconPressedMask, EFalse );
	        }
	    else
	        {
	        gc.BitBltMasked(TPoint(currentPointX,iCutBarRect.iBr.iY - iPlayheadIconRect.Height()), iPlayheadIcon, iPlayheadIconRect.Size(), 
				iPlayheadIconMask, EFalse );
	        }    													
		}
    }

CCoeControl* CVeiCutterBar::ComponentControl( TInt /*aIndex*/ ) const
    {
    return NULL;
    }


EXPORT_C void CVeiCutterBar::Dim( TBool aDimmed )
	{
	iDimmed = aDimmed;
	DrawDeferred();
	}

EXPORT_C void CVeiCutterBar::SetPlayHeadVisible( TBool aVisible )
	{
	iDrawPlayHead = aVisible;
	}

void CVeiCutterBar::HandleControlEventL( CCoeControl* /*aControl*/,
											TCoeEvent /*aEventType*/ )
	{
	DrawDeferred();
	}

void CVeiCutterBar::SizeChanged()
	{
	// the component rects are set in CVeiSimpleCutVideoContainer::SizeChanged(),
	// CVeiEditVideoContainer::SetCursorLocation() or CVeiCutVideoContainer::SizeChanged()

	// left end of the slider when that part is unselected
	AknIconUtils::SetSize( iSliderLeftEndIcon, iSliderLeftEndIconRect.Size(), EAspectRatioNotPreserved);
		
	// middle part of the slider when that part is unselected	
	AknIconUtils::SetSize( iSliderMiddleIcon, iSliderMiddleIconRect.Size(), EAspectRatioNotPreserved);		
	
	// right end of the slider when that part is unselected
	AknIconUtils::SetSize( iSliderRightEndIcon, iSliderRightEndIconRect.Size(), EAspectRatioNotPreserved); 

	if( !AknLayoutUtils::PenEnabled() )
		{
		// left end of the cut selection slider 
		AknIconUtils::SetSize( iSliderSelectedLeftEndIcon, iSliderSelectedLeftEndIconRect.Size(), EAspectRatioNotPreserved);
			
		// middle part of the cut selection slider 
		AknIconUtils::SetSize( iSliderSelectedMiddleIcon, iSliderSelectedMiddleIconRect.Size(), EAspectRatioNotPreserved);		
		
		// right end of the cut selection slider 
		AknIconUtils::SetSize( iSliderSelectedRightEndIcon, iSliderSelectedRightEndIconRect.Size(), EAspectRatioNotPreserved); 

	    // left/right border of cut selection slider
		AknIconUtils::SetSize( iCutAreaBorderIcon, iCutAreaBorderIconRect.Size(), EAspectRatioNotPreserved); 
			
		AknIconUtils::SetSize( iScissorsIcon, iScissorsIconRect.Size(), EAspectRatioNotPreserved);
		
		iStartMarkRect = TRect(CalculateCutAreaRect().iTl, iCutAreaBorderIconRect.Size());
		
	    TInt xD = CalculateCutAreaRect().iBr.iX - iCutAreaBorderIconRect.Width();
	    TPoint pointD = TPoint (xD, iSliderSelectedLeftEndIconRect.iTl.iY);		
			
		// set the end mark rect
	    iEndMarkRect = TRect(pointD, iCutAreaBorderIconRect.Size());
		}
	else
		{
		// middle part of the cut selection slider 
		AknIconUtils::SetSize( iSliderSelectedMiddleIcon, TSize(iCutBarRect.Size().iWidth, iSliderSelectedMiddleIconRect.Size().iHeight), EAspectRatioNotPreserved);

	    // left border of cut selection slider
		AknIconUtils::SetSize( iStartMarkIcon, iStartMarkRect.Size(), EAspectRatioNotPreserved); 
		AknIconUtils::SetSize( iStartMarkIconPressed, iStartMarkRect.Size(), EAspectRatioNotPreserved);
			
	    // right border of cut selection slider
		AknIconUtils::SetSize( iEndMarkIcon, iEndMarkRect.Size(), EAspectRatioNotPreserved); 
		AknIconUtils::SetSize( iEndMarkIconPressed, iEndMarkRect.Size(), EAspectRatioNotPreserved); 
		}

    // playhead
	AknIconUtils::SetSize( iPlayheadIcon, iPlayheadIconRect.Size(), EAspectRatioNotPreserved); 
	AknIconUtils::SetSize( iPlayheadIconPressed, iPlayheadIconRect.Size(), EAspectRatioNotPreserved);
	}

TKeyResponse CVeiCutterBar::OfferKeyEventL( const TKeyEvent& /*aKeyEvent*/,
											 TEventCode /*aType*/ )
	{
	return EKeyWasNotConsumed;
	}

EXPORT_C void CVeiCutterBar::SetInPoint( const TTimeIntervalMicroSeconds& aIn )
	{
	iInPoint = static_cast<TInt32>((aIn.Int64()/1000));

	if( !AknLayoutUtils::PenEnabled() )
		{
	    iStartMarkRect = TRect(CalculateCutAreaRect().iTl, iCutAreaBorderIconRect.Size());
		DrawDeferred();
		return;
		}

    TInt xD = CalculateCutAreaRect().iTl.iX - iStartMarkRect.Width();
    TPoint pointD = TPoint (xD, iStartMarkRect.iTl.iY);		
	TInt delta = pointD.iX - iStartMarkRect.iTl.iX;
	iStartMarkTouchRect.iTl.iX += delta;
	iStartMarkTouchRect.iBr.iX += delta;

	// set the start mark rect
    iStartMarkRect = TRect(pointD, iStartMarkRect.Size());
	DrawDeferred();
	}

EXPORT_C void CVeiCutterBar::SetOutPoint( const TTimeIntervalMicroSeconds& aOutPoint )
	{
	iOutPoint = static_cast<TInt32>((aOutPoint.Int64()/1000));

    // calculate the top left point (d in the picture below) for the end mark rect	
    // for more clarification, see the picture in Draw function
    //	
    //          right    
    //      |<- end ->|
    // -----b-----d---c---------
    //      |     |bor|
    //      |     |der|           
    // -------------------------

	if( !AknLayoutUtils::PenEnabled() )
		{
	    TInt xD = CalculateCutAreaRect().iBr.iX - iCutAreaBorderIconRect.Width();
	    TPoint pointD = TPoint (xD, iSliderSelectedLeftEndIconRect.iTl.iY);		
			
		// set the end mark rect
	    iEndMarkRect = TRect(pointD, iCutAreaBorderIconRect.Size());
		}
	else
		{
	    TPoint pointD = TPoint (CalculateCutAreaRect().iBr.iX, iEndMarkRect.iTl.iY);
		TInt delta = pointD.iX - iEndMarkRect.iTl.iX;
		iEndMarkTouchRect.iTl.iX += delta;
		iEndMarkTouchRect.iBr.iX += delta;

	    iEndMarkRect = TRect(pointD, iEndMarkRect.Size());
		}

	DrawDeferred();
	}

EXPORT_C void CVeiCutterBar::SetFinishedStatus( TBool aStatus )
	{
	iFinished = aStatus;
//	iCurrentPoint = 0;
	SetCurrentPoint( 0 );
	DrawDeferred();
	}

EXPORT_C void CVeiCutterBar::SetTotalDuration( const TTimeIntervalMicroSeconds& aDuration ) 
	{
	TTimeIntervalMicroSeconds duration (aDuration);
	if ( duration.Int64() == 0 ) 
		{
		duration = 1;	
		}

	TInt64 i = duration.Int64();
	for (;0 == (i/1000);i++) 
		{
		;
		}
	duration = i;

	iTotalDuration = static_cast<TInt32>((duration.Int64()/1000)); 
	SetOutPoint(aDuration);
//	DrawDeferred();
	}

EXPORT_C void CVeiCutterBar::SetCurrentPoint( TInt aLocation )
	{ 
	if( AknLayoutUtils::PenEnabled() )
		{
	    // calculate the playhead position
	    TUint width = iCutBarRect.Width();
	    TInt currentPointX = 0;
	    if (iTotalDuration > 0)
	        {
	        currentPointX = aLocation * width / iTotalDuration + iCutBarRect.iTl.iX;   
	        }
	    currentPointX = currentPointX - ( iPlayheadTouchRect.Width()/2 );         
		iPlayheadTouchRect.iBr.iX = currentPointX + iPlayheadTouchRect.Width();
		iPlayheadTouchRect.iTl.iX = currentPointX;
		}

	iCurrentPoint = aLocation; 
	DrawDeferred();
	}

// ----------------------------------------------------------------------------
// CVeiCutterBar::ProgressBarRect
// 
// ----------------------------------------------------------------------------
//	
EXPORT_C TRect CVeiCutterBar::ProgressBarRect()
	{
	return iCutBarRect;	
	}

// ----------------------------------------------------------------------------
// CVeiCutterBar::PlayHeadRect
// 
// ----------------------------------------------------------------------------
//	
EXPORT_C TRect CVeiCutterBar::PlayHeadRect()
	{
	return iPlayheadTouchRect;
	}

// ----------------------------------------------------------------------------
// CVeiCutterBar::StartMarkRect
// 
// ----------------------------------------------------------------------------
//	
EXPORT_C TRect CVeiCutterBar::StartMarkRect()
	{
	if( AknLayoutUtils::PenEnabled() )
		{
		return iStartMarkTouchRect;
		}

	TRect startMarkArea;
	
	// start mark has not been set
	if (iInPoint == 0)
		{
		startMarkArea = TRect(0,0,0,0);
		}
	else
	    {
	    startMarkArea = iStartMarkRect;	        
	    }
	return startMarkArea;
	}

// ----------------------------------------------------------------------------
// CVeiCutterBar::EndMarkRect
// 
// ----------------------------------------------------------------------------
//	
EXPORT_C TRect CVeiCutterBar::EndMarkRect()
	{
	if( AknLayoutUtils::PenEnabled() )
		{
		return iEndMarkTouchRect;
		}

	TRect endMarkArea;
	
	// end mark has not been set
	if (iOutPoint == iTotalDuration)
		{
		endMarkArea = TRect(0,0,0,0);
		}
	else
	    {
	    endMarkArea = iEndMarkRect;    	
	    }
	return endMarkArea;
	}

// ----------------------------------------------------------------------------
// CVeiCutterBar::StartMarkPoint
// 
// ----------------------------------------------------------------------------
//	
EXPORT_C TUint CVeiCutterBar::StartMarkPoint()
{
	// start mark hasn't been set
	if (iInPoint == 0)
		{	
		return 0;
		}
	// start mark has been set
	else
		{
		TInt startPoint = iInPoint * iCutBarRect.Width() / iTotalDuration + iCutBarRect.iTl.iX;	
		return startPoint;
		}	
}

// ----------------------------------------------------------------------------
// CVeiCutterBar::EndMarkPoint
// 
// ----------------------------------------------------------------------------
//	
EXPORT_C TUint CVeiCutterBar::EndMarkPoint()
{
	// end mark hasn't been set
	if (iOutPoint == iTotalDuration)
		{
		return iCutBarRect.iBr.iX;
		}
	// end mark has been set	
	else
		{
		TInt endPoint = iOutPoint * iCutBarRect.Width() / iTotalDuration + iCutBarRect.iTl.iX;
		return endPoint;
		}
}


// ----------------------------------------------------------------------------
// CVeiCutterBar::SetComponentRect
// 
// ----------------------------------------------------------------------------
//	
EXPORT_C void CVeiCutterBar::SetComponentRect(TCutterBarComponent aComponentIndex, TRect aRect)
	{
    switch ( aComponentIndex )
        {
        case EScissorsIcon:
        	{
       		iScissorsIconRect = aRect;
       		break;
        	}
        case EProgressBar:
        	{
    		iCutBarRect = aRect;
    		break;
        	}
        case ESliderLeftEndIcon:
            {
            iSliderLeftEndIconRect = aRect;
            break;    
            }
		case ESliderMiddleIcon:
		    {
		    iSliderMiddleIconRect = aRect;
		    break;    
		    }
		case ESliderRightEndIcon:
		    {
		    iSliderRightEndIconRect = aRect;
		    break;
		    }
		case ESliderSelectedLeftEndIcon:
		    {
		    iSliderSelectedLeftEndIconRect = aRect;
		    break;    
		    }
		case ESliderSelectedMiddleIcon:
		    {
		    iSliderSelectedMiddleIconRect = aRect;
		    break;    
		    }
		case ESliderSelectedRightEndIcon:
		    {
		    iSliderSelectedRightEndIconRect = aRect;
		    break;    
		    }
		case EPlayheadIcon:
		    {
		    iPlayheadIconRect = aRect;
		    break;    
		    }
		case ECutAreaBorderIcon:
		    {
		    iCutAreaBorderIconRect = aRect;
		    break;    
		    }
		case EStartMarkIcon:
		    {
		    iStartMarkRect = aRect;
		    break;    
		    }
		case EEndMarkIcon:
		    {
		    iEndMarkRect = aRect;
		    break;    
		    }
		case EPlayheadTouch:
		    {
		    iPlayheadTouchRect = aRect;
			SetCurrentPoint(iCurrentPoint);
		    break;    
		    }
		case EStartMarkTouch:
		    {
		    iStartMarkTouchRect = aRect;
		    break;    
		    }
		case EEndMarkTouch:
		    {
		    iEndMarkTouchRect = aRect;
		    break;    
		    }
        }
	}
    
// ----------------------------------------------------------------------------
// CVeiCutterBar::SetPressedComponent
// 
// ----------------------------------------------------------------------------
//	
EXPORT_C void CVeiCutterBar::SetPressedComponent(TCutterBarPressedIcon aComponentIndex)
    {
    iPressedComponent = aComponentIndex;
    }
    
// ----------------------------------------------------------------------------
// CVeiCutterBar::CalculateCutAreaRect
// 
// ----------------------------------------------------------------------------
//	
TRect CVeiCutterBar::CalculateCutAreaRect() const
    {

	TUint width = iCutBarRect.Width();
	TInt inPointX = 0;
	TInt outPointX = 0;
	
	if (0 < iTotalDuration)
	{
		inPointX = iInPoint * width / iTotalDuration + iCutBarRect.iTl.iX;
		outPointX = iOutPoint * width / iTotalDuration + iCutBarRect.iTl.iX;
	}

	if ( outPointX > iCutBarRect.iBr.iX )
		{
		outPointX = iCutBarRect.iBr.iX;
		}
	
	// area to be cut
	TRect selectedArea( iCutBarRect );

	if ( iFinished && !iDimmed ) 
		{
		selectedArea.iTl.iX = inPointX;
		selectedArea.iBr.iX = outPointX;
		}
	
	return selectedArea;
        
    }

// End of File
