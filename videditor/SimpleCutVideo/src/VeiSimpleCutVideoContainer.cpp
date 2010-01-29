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
#include <VedSimpleCutVideo.rsg>
#include <videoeditoruicomponents.mbg>
#include <eikbtgpc.h>
#include <vedvideoclipinfo.h>
#include <coemain.h>
#include <eikenv.h>
//#include <CMGXFileManager.h>
//#include <MGXFileManagerFactory.h>
#include <aknsbasicbackgroundcontrolcontext.h> 
#include <aknsdrawutils.h> 
#include <aknsdatacontext.h> 
#include <stringloader.h>
#include <aknnotewrappers.h> 
#include <pathinfo.h> 
#include <eikprogi.h>
#include <aknlayoutscalable_avkon.cdl.h>
#include <aknlayoutscalable_apps.cdl.h>
#include <CAknMemorySelectionDialog.h> 
#include <CAknFileNamePromptDialog.h> 
#include <AknCommonDialogsDynMem.h> 
#include <CAknMemorySelectionDialogMultiDrive.h> 
#include <apgcli.h>
#include <csxhelp/vided.hlp.hrh>

#ifdef RD_TACTILE_FEEDBACK 
#include <touchfeedback.h>
#endif /* RD_TACTILE_FEEDBACK  */

// User includes
#include "VeiSimpleCutVideoAppUi.h"
#include "VeiCutterBar.h"
#include "VeiSimpleCutVideoContainer.h"
#include "VeiSimpleCutVideoView.h"
#include "veitextdisplay.h"
#include "VideoEditorCommon.h"
#include "VideoEditorUtils.h"
#include "VeiErrorUi.h"
#include "VedSimpleCutVideo.hrh"

#define KMediaGalleryUID3           0x101F8599 

// ================= MEMBER FUNCTIONS =======================
void CleanupRarray( TAny* object )
	{
	(( RImageTypeDescriptionArray*)object)->ResetAndDestroy();
	}

CVeiSimpleCutVideoContainer* CVeiSimpleCutVideoContainer::NewL( const TRect& aRect, CVeiSimpleCutVideoView& aView, CVeiErrorUI& aErrorUI )
    {
    CVeiSimpleCutVideoContainer* self = CVeiSimpleCutVideoContainer::NewLC( aRect, aView, aErrorUI );
    CleanupStack::Pop( self );
    return self;
    }

CVeiSimpleCutVideoContainer* CVeiSimpleCutVideoContainer::NewLC( const TRect& aRect, CVeiSimpleCutVideoView& aView, CVeiErrorUI& aErrorUI )
    {
    CVeiSimpleCutVideoContainer* self = new (ELeave) CVeiSimpleCutVideoContainer( aRect, aView, aErrorUI );
    CleanupStack::PushL( self );
    self->ConstructL( aRect, aView, aErrorUI );
    return self;
    }

void CVeiSimpleCutVideoContainer::ConstructL( const TRect& aRect, CVeiSimpleCutVideoView& /*aView*/, CVeiErrorUI& /*aErrorUI*/ )
    {
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::ConstructL: in");
	CreateWindowL();

	iState = EStateInitializing;
	iFrameReady = EFalse;
	iPlayOrPlayMarked = EFalse;

	iSeekPos = TTimeIntervalMicroSeconds( 0 );
	iSeeking = EFalse;
	iCutVideoBar = CVeiCutterBar::NewL( this );

	iConverter = CVeiImageConverter::NewL( this );
	iTakeSnapshot = EFalse;
	iVideoDisplay = CVeiVideoDisplay::NewL( iDisplayRect, this, *this );

	TFileName mbmPath( VideoEditorUtils::IconFileNameAndPath(KVideoEditorUiComponentsIconFileId) );	

	if( !AknLayoutUtils::PenEnabled() )
		{
		iCutTimeDisplay = CVeiTextDisplay::NewL( iCutTimeDisplayRect, this );
               
		// A new icon. Temporarely same bitmap is used for non touch pause as in touch devices
		// A new icon has been requested and it should be changed here as soon as it's in the build
		AknIconUtils::CreateIconL( iPauseBitmap, iPauseBitmapMask,
				mbmPath, EMbmVideoeditoruicomponentsQgn_indi_vded_pause, 
				EMbmVideoeditoruicomponentsQgn_indi_vded_pause_mask );
		}
	else
		{
		AknIconUtils::CreateIconL( iPlayBitmap, iPlayBitmapMask,
				mbmPath, EMbmVideoeditoruicomponentsQgn_indi_vded2_play, 
				EMbmVideoeditoruicomponentsQgn_indi_vded2_play_mask );
				
		AknIconUtils::CreateIconL( iPauseBitmap, iPauseBitmapMask,
				mbmPath, EMbmVideoeditoruicomponentsQgn_indi_vded2_pause, 
				EMbmVideoeditoruicomponentsQgn_indi_vded2_pause_mask );
		}

    SetRect( aRect );

	iBgContext = CAknsBasicBackgroundControlContext::NewL( KAknsIIDQsnBgAreaMain, Rect(), EFalse );
	iVideoBarTimer = CPeriodic::NewL( CActive::EPriorityLow );

#ifdef RD_TACTILE_FEEDBACK 
    iTouchFeedBack = MTouchFeedback::Instance();    
#endif /* RD_TACTILE_FEEDBACK  */     

	iRemConTarget = CVeiRemConTarget::NewL( *this );

	EnableDragEvents();

    ActivateL();

	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::ConstructL: out");
    }

CVeiSimpleCutVideoContainer::CVeiSimpleCutVideoContainer( 
    const TRect& /*aRect*/, CVeiSimpleCutVideoView& aView, 
    CVeiErrorUI& aErrorUI ) : iView( aView ), iErrorUI( aErrorUI )
	{
	}

CVeiSimpleCutVideoContainer::~CVeiSimpleCutVideoContainer()
    {
	if ( iCutVideoBar )
		{
		delete iCutVideoBar;
		}
	if ( iBgContext )
		{
		delete iBgContext;
		}
	if ( iVideoDisplay )
		{
		delete iVideoDisplay;
		}
	if ( iCutTimeDisplay )
		{
		delete iCutTimeDisplay;
		}
	if ( iVideoClipInfo )
		{
		delete iVideoClipInfo;
		iVideoClipInfo = NULL;
		}
	if ( iConverter )
		{
		iConverter->Cancel();
		delete iConverter;
		}
	if ( iVideoBarTimer )
		{
		iVideoBarTimer->Cancel();
		delete iVideoBarTimer;
		}

	if ( iProgressDialog )
		{
		delete iProgressDialog;
		iProgressDialog = NULL;
		}
	if ( iSaveToFileName )
		{
		delete iSaveToFileName;
		iSaveToFileName = NULL;
		}

	delete iCallBackSaveSnapshot;
	delete iCallBackTakeSnapshot;

	if ( iPlayBitmap )
		{
		delete iPlayBitmap;
		}
	if ( iPlayBitmapMask )
		{
		delete iPlayBitmapMask;
		}
	delete iPauseBitmap;
	delete iPauseBitmapMask;

	delete iRemConTarget;
    }

void CVeiSimpleCutVideoContainer::DialogDismissedL( TInt aButtonId )
	{
	if ( aButtonId == -1 )
        { // when pressing cancel button.
		CancelSnapshotSave();
        }
	iTakeSnapshot = EFalse;
	}

void CVeiSimpleCutVideoContainer::SizeChanged()
    {
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::SizeChanged(): In");
	TSize videoScreenSize;
    TRect rect( Rect() ); 
	if ( iBgContext )
		{
		iBgContext->SetRect( rect );
		}
	LOGFMT2(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::SizeChanged(): Rect(): %d,%d", rect.iBr.iX, rect.iBr.iY);

	TInt variety = 0;
	if (VideoEditorUtils::IsLandscapeScreenOrientation())
		{
		variety = 1;
		}

	if( !AknLayoutUtils::PenEnabled() )
		{
		// Progress bar
		TAknLayoutRect progressBarLayout; 
		progressBarLayout.LayoutRect(Rect(),AknLayoutScalable_Apps::vded_slider_pane());
		iCutVideoBar->SetComponentRect(CVeiCutterBar::EProgressBar, progressBarLayout.Rect());

	    TAknLayoutRect sliderLeftEndLayout;
	    TAknLayoutRect sliderRightEndLayout;
	    if ( AknLayoutUtils::LayoutMirrored () )
	        {
	        // left end of the slider when that part is unselected
	    	sliderLeftEndLayout.LayoutRect( progressBarLayout.Rect(),AknLayoutScalable_Apps::vded_slider_pane_g4() );
	    	iCutVideoBar->SetComponentRect( CVeiCutterBar::ESliderLeftEndIcon, sliderLeftEndLayout.Rect() );
	    	
	    	// right end of the slider when that part is unselected
	    	sliderRightEndLayout.LayoutRect(progressBarLayout.Rect(),AknLayoutScalable_Apps::vded_slider_pane_g3());
	    	iCutVideoBar->SetComponentRect( CVeiCutterBar::ESliderRightEndIcon, sliderRightEndLayout.Rect() );	
	        }
	    else
	        {
	        // left end of the slider when that part is unselected
	    	sliderLeftEndLayout.LayoutRect( progressBarLayout.Rect(),AknLayoutScalable_Apps::vded_slider_pane_g3() );
	    	iCutVideoBar->SetComponentRect( CVeiCutterBar::ESliderLeftEndIcon, sliderLeftEndLayout.Rect() );
	    	
	    	// right end of the slider when that part is unselected
	    	sliderRightEndLayout.LayoutRect(progressBarLayout.Rect(),AknLayoutScalable_Apps::vded_slider_pane_g4());
	    	iCutVideoBar->SetComponentRect( CVeiCutterBar::ESliderRightEndIcon, sliderRightEndLayout.Rect() );	
	        }

		// middle part of the slider when that part is unselected	
		TAknLayoutRect sliderMiddleLayout;
		sliderMiddleLayout.LayoutRect(progressBarLayout.Rect(),AknLayoutScalable_Apps::vded_slider_pane_g5());
		iCutVideoBar->SetComponentRect( CVeiCutterBar::ESliderMiddleIcon, sliderMiddleLayout.Rect() );		
	    				
		// left end of the cut selection slider 
		TAknLayoutRect sliderSelectedLeftEndLayout;
		sliderSelectedLeftEndLayout.LayoutRect(progressBarLayout.Rect(),AknLayoutScalable_Apps::vded_slider_pane_g3());
		iCutVideoBar->SetComponentRect( CVeiCutterBar::ESliderSelectedLeftEndIcon, sliderSelectedLeftEndLayout.Rect() );
			
		// middle part of the cut selection slider 
		TAknLayoutRect sliderSelectedMiddleLayout;
		sliderSelectedMiddleLayout.LayoutRect(progressBarLayout.Rect(),AknLayoutScalable_Apps::vded_slider_pane_g5());
		iCutVideoBar->SetComponentRect( CVeiCutterBar::ESliderSelectedMiddleIcon, sliderSelectedMiddleLayout.Rect() );		
		
		// right end of the cut selection slider 
		TAknLayoutRect sliderSelectedRightEndLayout;
		sliderSelectedRightEndLayout.LayoutRect(progressBarLayout.Rect(),AknLayoutScalable_Apps::vded_slider_pane_g4());
		iCutVideoBar->SetComponentRect( CVeiCutterBar::ESliderSelectedRightEndIcon, sliderSelectedRightEndLayout.Rect() ); 

	    // playhead
	    TAknLayoutRect playheadLayout;
		playheadLayout.LayoutRect(progressBarLayout.Rect(),AknLayoutScalable_Apps::vded_slider_pane_g1());
		iCutVideoBar->SetComponentRect( CVeiCutterBar::EPlayheadIcon, playheadLayout.Rect() ); 

	    // left/right border of cut selection slider
	    TAknLayoutRect cutAreaBorderLayout;
		cutAreaBorderLayout.LayoutRect(progressBarLayout.Rect(),AknLayoutScalable_Apps::vded_slider_pane_g2());
		iCutVideoBar->SetComponentRect( CVeiCutterBar::ECutAreaBorderIcon, cutAreaBorderLayout.Rect() ); 
			
		TAknLayoutText startTimeTextLayout;
		TAknLayoutText endTimeTextLayout;
		TAknLayoutRect startTimeIconLayout;
		TAknLayoutRect endTimeIconLayout;

		// Video Display	
		TAknLayoutRect videoDisplayLayout;
		videoDisplayLayout.LayoutRect(Rect(),AknLayoutScalable_Apps::vded_video_pane());
		iVideoDisplay->SetRect(videoDisplayLayout.Rect());
		
		iDisplayRect = videoDisplayLayout.Rect();	
		iIconDisplayRect = videoDisplayLayout.Rect();
		LOGFMT2(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::SizeChanged(): iDisplayRect: %d,%d", iDisplayRect.iBr.iX, iDisplayRect.iBr.iY);

		//CVeiCutterBar	    
	    TRect cutBarRect( sliderLeftEndLayout.Rect().iTl, sliderRightEndLayout.Rect().iBr );
	    iCutVideoBar->SetRect( cutBarRect );
			
		TInt iconWidth = STATIC_CAST( TInt, rect.iBr.iX * 0.07954545455 );
		AknIconUtils::SetSize( iPauseBitmap, TSize(iconWidth,iconWidth), EAspectRatioNotPreserved );
		}
	else
		{
		// Progress bar
		TAknLayoutRect progressBarLayout; 
		progressBarLayout.LayoutRect(Rect(),AknLayoutScalable_Apps::vded2_slider_pane(variety));

		TAknLayoutRect progressBGLayout; 
		progressBGLayout.LayoutRect(progressBarLayout.Rect(),AknLayoutScalable_Apps::vded2_slider_bg_pane());
		iCutVideoBar->SetComponentRect(CVeiCutterBar::EProgressBar, progressBGLayout.Rect());

	    TAknLayoutRect sliderLeftEndLayout;
	    TAknLayoutRect sliderRightEndLayout;
	    if ( AknLayoutUtils::LayoutMirrored () )
	        {
	        // left end of the slider when that part is unselected
	    	sliderLeftEndLayout.LayoutRect( progressBGLayout.Rect(),AknLayoutScalable_Apps::vded2_slider_bg_pane_g2());
	    	iCutVideoBar->SetComponentRect( CVeiCutterBar::ESliderLeftEndIcon, sliderLeftEndLayout.Rect() );
	    	
	    	// right end of the slider when that part is unselected
	    	sliderRightEndLayout.LayoutRect(progressBGLayout.Rect(),AknLayoutScalable_Apps::vded2_slider_bg_pane_g1());
	    	iCutVideoBar->SetComponentRect( CVeiCutterBar::ESliderRightEndIcon, sliderRightEndLayout.Rect() );	

	        }
	    else
	        {
	        // left end of the slider when that part is unselected
	    	sliderLeftEndLayout.LayoutRect( progressBGLayout.Rect(),AknLayoutScalable_Apps::vded2_slider_bg_pane_g1());
	    	iCutVideoBar->SetComponentRect( CVeiCutterBar::ESliderLeftEndIcon, sliderLeftEndLayout.Rect() );
	    	
	    	// right end of the slider when that part is unselected
	    	sliderRightEndLayout.LayoutRect(progressBGLayout.Rect(),AknLayoutScalable_Apps::vded2_slider_bg_pane_g2());
	    	iCutVideoBar->SetComponentRect( CVeiCutterBar::ESliderRightEndIcon, sliderRightEndLayout.Rect() );	
	        }

		// middle part of the slider when that part is unselected	
		TAknLayoutRect sliderMiddleLayout;
		sliderMiddleLayout.LayoutRect(progressBGLayout.Rect(),AknLayoutScalable_Apps::vded2_slider_bg_pane_g3());
		iCutVideoBar->SetComponentRect( CVeiCutterBar::ESliderMiddleIcon, sliderMiddleLayout.Rect() );		
	    				
		// middle part of the cut selection slider 
		TAknLayoutRect sliderSelectedMiddleLayout;
		sliderSelectedMiddleLayout.LayoutRect(progressBGLayout.Rect(),AknLayoutScalable_Apps::vded2_slider_bg_pane_g3());
		iCutVideoBar->SetComponentRect( CVeiCutterBar::ESliderSelectedMiddleIcon, sliderSelectedMiddleLayout.Rect() );		

	    // playhead
	    TAknLayoutRect playheadLayout;
		playheadLayout.LayoutRect(progressBarLayout.Rect(),AknLayoutScalable_Apps::vded2_slider_pane_g3());
		iCutVideoBar->SetComponentRect( CVeiCutterBar::EPlayheadIcon, playheadLayout.Rect() ); 

	    // left/right border of cut selection slider
	    TAknLayoutRect cutAreaBorderLayout;
		cutAreaBorderLayout.LayoutRect(progressBarLayout.Rect(),AknLayoutScalable_Apps::vded2_slider_pane_g1());
		iCutVideoBar->SetComponentRect( CVeiCutterBar::EStartMarkIcon, cutAreaBorderLayout.Rect() ); 
		cutAreaBorderLayout.LayoutRect(progressBarLayout.Rect(),AknLayoutScalable_Apps::vded2_slider_pane_g2());
		iCutVideoBar->SetComponentRect( CVeiCutterBar::EEndMarkIcon, cutAreaBorderLayout.Rect() ); 

		// Video Display	
		TAknLayoutRect videoDisplayLayout;
		videoDisplayLayout.LayoutRect(Rect(),AknLayoutScalable_Apps::vded_video_pane());
		iVideoDisplay->SetRect(videoDisplayLayout.Rect());
		
		iDisplayRect = videoDisplayLayout.Rect();	
		iIconDisplayRect = videoDisplayLayout.Rect();
		LOGFMT2(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::SizeChanged(): iDisplayRect: %d,%d", iDisplayRect.iBr.iX, iDisplayRect.iBr.iY);

		// CVeiCutterBar	    
	    TRect cutBarRect( progressBarLayout.Rect().iTl, progressBarLayout.Rect().iBr );
	    iCutVideoBar->SetRect( cutBarRect );

		// pause icon		
		TAknLayoutRect iconLayout;
		iconLayout.LayoutRect(Rect(),AknLayoutScalable_Apps::main_vded2_pane_g2(variety));
		iIconDisplayRect = iconLayout.Rect();
		AknIconUtils::SetSize( iPlayBitmap, iIconDisplayRect.Size(), EAspectRatioNotPreserved );
		AknIconUtils::SetSize( iPauseBitmap, iIconDisplayRect.Size(), EAspectRatioNotPreserved );

		// cut bar touch areas
	    TAknLayoutRect touchAreaLayout;
		touchAreaLayout.LayoutRect(progressBarLayout.Rect(),AknLayoutScalable_Apps::aid_size_touch_vded2_playhead());
		iCutVideoBar->SetComponentRect( CVeiCutterBar::EPlayheadTouch, touchAreaLayout.Rect() ); 
		touchAreaLayout.LayoutRect(progressBarLayout.Rect(),AknLayoutScalable_Apps::aid_size_touch_vded2_start());
		iCutVideoBar->SetComponentRect( CVeiCutterBar::EStartMarkTouch, touchAreaLayout.Rect() ); 
		touchAreaLayout.LayoutRect(progressBarLayout.Rect(),AknLayoutScalable_Apps::aid_size_touch_vded2_end());
		iCutVideoBar->SetComponentRect( CVeiCutterBar::EEndMarkTouch, touchAreaLayout.Rect() ); 
		}

	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::SizeChanged(): Out");
	}

TTypeUid::Ptr CVeiSimpleCutVideoContainer::MopSupplyObject( TTypeUid aId )
	{
	if ( aId.iUid == MAknsControlContext::ETypeId && iBgContext )
		{
		return MAknsControlContext::SupplyMopObject( aId, iBgContext );
		}
	return CCoeControl::MopSupplyObject( aId );
	}

TInt CVeiSimpleCutVideoContainer::CountComponentControls() const
    {
	if( !AknLayoutUtils::PenEnabled() )
		{
	    return 3; 
		}
	else
		{
	    return 2; 
		}
    }

CCoeControl* CVeiSimpleCutVideoContainer::ComponentControl( TInt aIndex ) const
    {
    switch ( aIndex )
        {
        case 0:
			return iCutVideoBar;
		case 1:
			return iVideoDisplay;
		case 2:
			return iCutTimeDisplay;
        default:
            return NULL;
        }
    }

void CVeiSimpleCutVideoContainer::Draw( const TRect& aRect ) const
    {
    CWindowGc& gc = SystemGc();
	// draw skin background
	MAknsSkinInstance* skin = AknsUtils::SkinInstance();
	MAknsControlContext* cc = AknsDrawUtils::ControlContext( this );	
	AknsDrawUtils::Background( skin, cc, this, gc, aRect );

    if ( EStatePlaying != iState && EStatePlayingMenuOpen != iState
         && AknLayoutUtils::PenEnabled() )
		{
		gc.BitBltMasked( iIconDisplayRect.iTl, iPlayBitmap, 
			TRect(iIconDisplayRect.Size()), 
			iPlayBitmapMask, EFalse );
		}
	else if ( ( EStatePlaying == iState || EStatePlayingMenuOpen == iState )
	          && AknLayoutUtils::PenEnabled() )
		{		
		gc.BitBltMasked( iIconDisplayRect.iTl, iPauseBitmap, 
			TRect(iIconDisplayRect.Size()), 
			iPauseBitmapMask, EFalse );
		}

	if ( EStatePaused == iState && !AknLayoutUtils::PenEnabled() )
		{		
 		TPoint pauseIconTl = TPoint( iIconDisplayRect.iTl.iX - STATIC_CAST( TInt, Rect().iBr.iX*0.105),
			iIconDisplayRect.iTl.iY + STATIC_CAST( TInt, Rect().iBr.iY*0.178 ));
		gc.BitBltMasked( pauseIconTl, iPauseBitmap, 
			TRect( TPoint(0,0), iPauseBitmap->SizeInPixels() ), 
			iPauseBitmapMask, EFalse );
		}
	}

// ----------------------------------------------------------------------------
// CVeiSimpleCutVideoContainer::GetHelpContext(...) const
//
// Gets the control's help context. Associates the control with a particular
// Help file and topic in a context sensitive application.
// ----------------------------------------------------------------------------
//
void CVeiSimpleCutVideoContainer::GetHelpContext( TCoeHelpContext& aContext ) const
    {
    LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::GetHelpContext(): In");

    // Set UID of the CS Help file (same as application UID).
    aContext.iMajor = KUidVideoEditor;

    // Set the context/topic.
    aContext.iContext = KVIE_HLP_CUT;

    LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::GetHelpContext(): Out");
    }


void CVeiSimpleCutVideoContainer::HandleControlEventL(
    CCoeControl* /*aControl*/,TCoeEvent /*aEventType*/)
    {
    }

// ----------------------------------------------------------------------------
// CVeiSimpleCutVideoContainer::HandlePointerEventL
// From CCoeControl
// ----------------------------------------------------------------------------
//		
void CVeiSimpleCutVideoContainer::HandlePointerEventL(const TPointerEvent& aPointerEvent )
	{
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::HandlePointerEventL(): In");
	if( AknLayoutUtils::PenEnabled() && iCutVideoBar )
		{
		CCoeControl::HandlePointerEventL( aPointerEvent );

		switch( aPointerEvent.iType )
			{
			case TPointerEvent::EButton1Down:
				{
				iButtonDownPoint = aPointerEvent.iPosition;
				iIsMarkDrag = EFalse;
				iIsMarkTapped = EFalse;
				iIsIconOrDisplayTapped = EFalse;
				TRect startMarkRect = iCutVideoBar->StartMarkRect();
				TRect endMarkRect = iCutVideoBar->EndMarkRect();				
				TRect playheadRect = iCutVideoBar->PlayHeadRect();
				// check if the pen goes down inside the start mark
				if (startMarkRect.Contains(aPointerEvent.iPosition)) 
					{
					iIsMarkTapped = ETrue;
					iTappedMark = EStartMark;		
					iCutVideoBar->SetPressedComponent( CVeiCutterBar::EPressedStartMarkTouch );					
					}
				// check if the pen goes down inside the end mark	
				else if (endMarkRect.Contains(aPointerEvent.iPosition))
					{
					iIsMarkTapped = ETrue;
					iTappedMark = EEndMark;
					iCutVideoBar->SetPressedComponent( CVeiCutterBar::EPressedEndMarkTouch );
					}					
				// check if the pen goes down inside the playhead	
				else if (playheadRect.Contains(aPointerEvent.iPosition))
					{
					iIsMarkTapped = ETrue;
					iTappedMark = EPlayHead;
					iCutVideoBar->SetPressedComponent( CVeiCutterBar::EPressedPlayheadTouch );					
					}					
                else if ( iDisplayRect.Contains( iButtonDownPoint ) ||
                          iIconDisplayRect.Contains( iButtonDownPoint ) )
                    {
                    iIsIconOrDisplayTapped = ETrue;
                    }
                    
                if ( iIsMarkTapped )
                	{
    	            DrawDeferred();	    	            
                	}    	        	       

#ifdef RD_TACTILE_FEEDBACK 
					if ( iTouchFeedBack && ( iIsMarkTapped || iIsIconOrDisplayTapped ))
	                	{
			            iTouchFeedBack->InstantFeedback( ETouchFeedbackBasic );
	                	}
#endif /* RD_TACTILE_FEEDBACK  */	
				
//				TRect progressBarRect(iCutVideoBar->ProgressBarRect());	
				// check if the pen goes down inside the progress bar				
//				if( progressBarRect.Contains( aPointerEvent.iPosition ) )
//					{
//					iIsMarkDrag = EFalse;					
//					}
				break;
				}
			case TPointerEvent::EDrag:
				{
				if (iIsMarkTapped)
					{
					TRect touchRect(iCutVideoBar->ProgressBarRect());
					TRect smallRect(iCutVideoBar->EndMarkRect());
					TInt pressPoint = aPointerEvent.iPosition.iX;
					TBool dragMarks = ETrue;
					if (iTappedMark == EStartMark)
						{
							pressPoint += iCutVideoBar->StartMarkRect().Width()/2;
						}
					if (iTappedMark == EEndMark)
						{
							pressPoint -= iCutVideoBar->StartMarkRect().Width()/2;
						}
					if (iTappedMark == EPlayHead)
						{
							smallRect = iCutVideoBar->PlayHeadRect();
							dragMarks = EFalse;
						}
					touchRect.iTl.iY = smallRect.iTl.iY;
					touchRect.iBr.iY = smallRect.iBr.iY;
					HandleProgressBarTouchL( touchRect, 
											 pressPoint,
											 dragMarks,
											 iTappedMark );
					iIsMarkDrag = ETrue;
					}
				break;		
				}
			case TPointerEvent::EButton1Up:
				{
				// pen up event is handled if it wasn't dragged
//				if (!iIsMarkDrag)
				if (0)
					{
					TRect progressBarRect(iCutVideoBar->ProgressBarRect());					
					// Check if pressed position is in progress bar's rect
					if( progressBarRect.Contains( aPointerEvent.iPosition ) )
						{
						HandleProgressBarTouchL( progressBarRect, 
											 aPointerEvent.iPosition.iX,
											 EFalse);
						}
					}
				
				if ((iDisplayRect.Contains(aPointerEvent.iPosition) && 
				    iDisplayRect.Contains(iButtonDownPoint)) ||
					(iIconDisplayRect.Contains(aPointerEvent.iPosition) && 
				    iIconDisplayRect.Contains(iButtonDownPoint)))
				    {			        	        
				    HandleVideoClickedL();
				    }
				    iCutVideoBar->SetPressedComponent( CVeiCutterBar::ENoPressedIcon );
				    DrawDeferred();
				break;
				}		
			default:
				{
				break;	
				}	
			}
		}	
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::HandlePointerEventL(): Out");		
	}


// ----------------------------------------------------------------------------
// CVeiSimpleCutVideoContainer::HandleProgressBarTouchL
// 
// ----------------------------------------------------------------------------
//	
void CVeiSimpleCutVideoContainer::HandleProgressBarTouchL( TRect aPBRect, 
												 TInt aPressedPoint,
												 TBool aDragMarks,
												 CVeiSimpleCutVideoContainer::TCutMark aCutMark )
	{
	if ( (AknLayoutUtils::PenEnabled()) && ( iState!=EStateInitializing ))
		{	
		LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::HandleProgressBarTouchL(): In");

		if (iState == EStatePlaying)		
			{
			PauseL();	
			}
		
		// Progress Bar start and end points, and length
		TInt pbEndPoint( aPBRect.iBr.iX );
		TInt pbStartPoint = ( aPBRect.iTl.iX );		
		TInt totalPBLength( pbEndPoint - pbStartPoint );
				
		// calculate the time position from the tapped progress bar coordinates 
		TTimeIntervalMicroSeconds newPosition( 
										( ( aPressedPoint - pbStartPoint ) * 
							  			iVideoClipInfo->Duration().Int64() ) / 
							  			totalPBLength );
		if (newPosition >= iVideoClipInfo->Duration())
			{
			newPosition = TTimeIntervalMicroSeconds( iVideoClipInfo->Duration().Int64() - 1 );
			}
		if (newPosition <= 0)
			{
			newPosition = 0;
			}

		TBool movePlayhead = ETrue;
		
		// move cut marks
		if (aDragMarks)
			{
			movePlayhead = EFalse;
			//one sec buffer:
			const TUint markBuffer = ((KMinCutVideoLength *  totalPBLength) / iVideoClipInfo->Duration().Int64());
			// check that the start mark doesn't go past the end mark - 1 sec buffer
			// and not to the beginning	
			if ((aCutMark == EStartMark) && 
			    (newPosition.Int64() >= 0) &&
				(aPressedPoint < (iCutVideoBar->EndMarkPoint() - markBuffer ) /*- 2*iCutVideoBar->EndMarkRect().Width()*/))
				{				
				iView.MoveStartOrEndMarkL(newPosition, EStartMark);				
				iCutVideoBar->SetInPoint( newPosition );
				movePlayhead = ETrue;
				}
			// check that the end mark doesn't go before the start mark - +1 sec buffer
			// and not too close to the beginning			
			else if ((aCutMark == EEndMark) && 
				(newPosition.Int64() >= KMinCutVideoLength) &&			
				(aPressedPoint > (iCutVideoBar->StartMarkPoint() + markBuffer)/* + 2*iCutVideoBar->StartMarkRect().Width()*/))                
				{				
				iView.MoveStartOrEndMarkL(newPosition, EEndMark);				
				iCutVideoBar->SetOutPoint( newPosition );
				movePlayhead = ETrue;
				}
			}
				
		// move playhead
//		else if (( newPosition != iLastPosition ) && !aDragMarks)
		if (( newPosition != iLastPosition ) && movePlayhead)
			{
			iLastPosition = newPosition;

			iSeekPos = TTimeIntervalMicroSeconds( newPosition );

			iCutVideoBar->SetCurrentPoint( (static_cast<TInt32>(iSeekPos.Int64() / 1000)));
			iVideoDisplay->SetPositionL( iSeekPos );
			GetThumbAtL( iSeekPos );
					
			iView.UpdateTimeL();
			}	
		
		iView.UpdateCBAL(iState);
		LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::HandleProgressBarTouchL(): Out");
			
		}// PenEnabled
		
	}


void CVeiSimpleCutVideoContainer::PlayL( const TDesC& aFilename )
	{
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::PlayL: in");
	iVideoDisplay->SetPositionL( iSeekPos );
	
	if (iVideoClipInfo && !iFrameReady)
		{								
		iVideoClipInfo->CancelFrame();
		}
	iVideoDisplay->PlayL( aFilename );
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::PlayL: out");
	}

void CVeiSimpleCutVideoContainer::PlayMarkedL( const TDesC& aFilename,
									const TTimeIntervalMicroSeconds& aStartTime, 
									const TTimeIntervalMicroSeconds& aEndTime )
	{
	LOGFMT3(KVideoEditorLogFile, "CVeisimpleCutVideoContainer::PlayMarkedL, In, aStartTime:%Ld, aEndTime:%Ld, aFilename:%S", aStartTime.Int64(), aEndTime.Int64(), &aFilename);
	iPlayOrPlayMarked = ETrue;

	if (iVideoClipInfo && !iFrameReady)
		{								
		iVideoClipInfo->CancelFrame();
		}
	iVideoDisplay->PlayL( aFilename, aStartTime, aEndTime );
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::PlayMarkedL, Out");
	}

void CVeiSimpleCutVideoContainer::StopL()
	{
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::StopL, in");
	iVideoDisplay->Stop( EFalse );

	iSeekPos = TTimeIntervalMicroSeconds( 0 );
	
	// When adding a new video clip after saving, PlaybackPositionL() 
	// returns a value > 0 eventhough the playhead is in the beginning
	// so iLastPosition has to be set to the beginning.
	iLastPosition = TTimeIntervalMicroSeconds( 0 ); 
    
	SetStateL( EStateStopped );
	PlaybackPositionL();
	
	if (iVideoBarTimer)
		{		
		iVideoBarTimer->Cancel();
		}
	
	iCutVideoBar->SetFinishedStatus( ETrue );
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::StopL, in");
	}


void CVeiSimpleCutVideoContainer::TakeSnapshotL()
	{
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::TakeSnapshotL in");
	
	if( !iVideoClipInfo || !iFrameReady )
		{
		LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::TakeSnapshotL: 1");
		if (!iFrameReady)
			{
			iTakeSnapshotWaiting = ETrue;	
			}		
		return;
		}
		
	iTakeSnapshotWaiting = EFalse;	
	iTakeSnapshot = ETrue;
	const TTimeIntervalMicroSeconds& pos = PlaybackPositionL();

	GetThumbAtL( pos );
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::TakeSnapshotL out");
	}

void CVeiSimpleCutVideoContainer::PauseL( TBool aUpdateCBA )
	{
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::PauseL: In");

	if (EStatePlaying == iState)
		{		
		iLastPosition = iVideoDisplay->PositionL();					
		iSeekPos = iLastPosition;
		// to set next start point
		iVideoDisplay->SetPositionL(iSeekPos);
		}

	iVideoDisplay->PauseL();
	if (iVideoBarTimer)
		{		
		iVideoBarTimer->Cancel();
		}

	#ifdef GET_PAUSE_THUMBNAIL
	GetThumbAtL( iLastPosition );
	#endif

	if (EStateStoppedInitial == iState || EStateStopped == iState) 
		{
		// just to trigger cba-update
		SetStateL( iState, aUpdateCBA );
		}	
	else if (EStateInitializing != iState) 
		{
		SetStateL( EStatePaused, aUpdateCBA );
		}
	else
		{
		SetStateL( EStateStoppedInitial );
		}
			
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::PauseL: Out");
	}

void CVeiSimpleCutVideoContainer::SaveSnapshotL()
	{
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::SaveSnapshotL: In");	

    if (LaunchSavingDialogsL())
        {		
        //first we have to encode bitmap
        // get encoder types 
        RImageTypeDescriptionArray imageTypes; 
        iConverter->GetEncoderImageTypesL( imageTypes );	

        CleanupStack::PushL( TCleanupItem( CleanupRarray, &imageTypes ) );

        TInt selectedIdx = 0;

        for( TInt i=0; i<imageTypes.Count(); i++ ) 
        	{
        	if ( imageTypes[i]->Description() == KEncoderType )
        		{
        		selectedIdx = i;
        		}
        	}

        //for cancellation
        if ( iSaveToFileName )
        	{
        	delete iSaveToFileName;
        	iSaveToFileName = NULL;
        	}
        iSaveToFileName = HBufC::NewL( iSnapshotFileName.Length() );
        *iSaveToFileName = iSnapshotFileName;

        // request the actuall save/encode
        // asynchronous, the result is reported via callback NotifyCompletion
        LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::SaveSnapshotL: 1, calling iConverter->StartToEncodeL");	
        iConverter->StartToEncodeL( iSnapshotFileName, 
        	imageTypes[selectedIdx]->ImageType(), imageTypes[selectedIdx]->SubType());

        CleanupStack::PopAndDestroy( &imageTypes );
        LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::SaveSnapshotL: Out");	

        ShowProgressNoteL();		            
        }
	}


TBool CVeiSimpleCutVideoContainer::LaunchSavingDialogsL()
    {
   	TBool ret = EFalse;	
	RFs	fs = CCoeEnv::Static()->FsSession();
	
	iSnapshotFileName.Zero();
	iSnapshotFileName.Append( iView.Settings().DefaultSnapshotName() );
    iSnapshotFileName.Append( _L(".JPEG") );	
    
    // Multiple drive support
#ifdef RD_MULTIPLE_DRIVE
	TDriveNumber driveNumber;
    TFileName driveAndPath;
    CAknMemorySelectionDialogMultiDrive* multiDriveDlg = 
                                    CAknMemorySelectionDialogMultiDrive::NewL(ECFDDialogTypeSave, EFalse );			
                                    
	CleanupStack::PushL(multiDriveDlg);
	
	// launch "Select memory" query
    if (multiDriveDlg->ExecuteL( driveNumber, &driveAndPath, NULL ))
		{
		driveAndPath.Append( PathInfo::ImagesPath() );
			        
        iSnapshotFileName.Insert(0,driveAndPath);

		// Generate a default name for the new file
		CApaApplication::GenerateFileName( fs, iSnapshotFileName );	
				
		// launch file name prompt dialog
		if (CAknFileNamePromptDialog::RunDlgLD(iSnapshotFileName, driveAndPath, KNullDesC))
			{
			driveAndPath.Append(iSnapshotFileName);
			iSnapshotFileName = driveAndPath;
            ret = ETrue;
            }
		else
			{
			iTakeSnapshot = EFalse;
			iTakeSnapshotWaiting = EFalse;
			}
		}
	else
		{
		iTakeSnapshot = EFalse;
		iTakeSnapshotWaiting = EFalse;
		}
	CleanupStack::PopAndDestroy( multiDriveDlg ); 
#else // no multiple drive support
	CAknMemorySelectionDialog::TMemory selectedMemory(CAknMemorySelectionDialog::EPhoneMemory);		
	// launch "Select memory" query
	if (CAknMemorySelectionDialog::RunDlgLD(selectedMemory))
		{
		// create path for the image	
		TFileName driveAndPath;        		
		if (selectedMemory == CAknMemorySelectionDialog::EPhoneMemory)
			{
			driveAndPath.Copy( PathInfo::PhoneMemoryRootPath() );
			driveAndPath.Append( PathInfo::ImagesPath() );							
			}
		else if (selectedMemory == CAknMemorySelectionDialog::EMemoryCard)
			{	
			driveAndPath.Copy( PathInfo::MemoryCardRootPath() );
			driveAndPath.Append( PathInfo::ImagesPath() );							
			}        				 
        
        iSnapshotFileName.Insert(0,driveAndPath);

        // Generate a default name for the new file
        CApaApplication::GenerateFileName( fs, iSnapshotFileName );					
        
		// launch file name prompt dialog
		if (CAknFileNamePromptDialog::RunDlgLD(iSnapshotFileName, driveAndPath, KNullDesC))
			{
			driveAndPath.Append(iSnapshotFileName);
			iSnapshotFileName = driveAndPath;
            ret = ETrue;
            }
		else
			{
			iTakeSnapshot = EFalse;
			iTakeSnapshotWaiting = EFalse;
			}
		}
	else
		{
		iTakeSnapshot = EFalse;
		iTakeSnapshotWaiting = EFalse;
		}
#endif
	return ret;
    }

void CVeiSimpleCutVideoContainer::CancelSnapshotSave()
	{
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::CancelSnapshotSave: in");
	if ( iConverter )
		{
		LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::CancelSnapshotSave: 1");
		iConverter->Cancel();
		iConverter->CancelEncoding(); //also close the file
		}
	if ( iSaveToFileName )
		{
		LOGFMT(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::CancelSnapshotSave: 2, iSaveToFileName:%S", iSaveToFileName);

		RFs&	fs = iEikonEnv->FsSession(); 
		/*TInt result =*/ fs.Delete( *iSaveToFileName ); 
		delete iSaveToFileName;
		iSaveToFileName = NULL;
		}
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::CancelSnapshotSave: out");	
	}

void CVeiSimpleCutVideoContainer::CloseStreamL()
	{
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::CloseStreamL, in");
	if( !iFrameReady && iVideoClipInfo )
		{
		iVideoClipInfo->CancelFrame();
		}
	PlaybackPositionL();
	iState = EStateStopped;
	iView.UpdateCBAL(iState);
	
	iVideoDisplay->Stop( ETrue );
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::CloseStreamL, out");
	}

void CVeiSimpleCutVideoContainer::SetInTime( const TTimeIntervalMicroSeconds& aTime )
	{
	if ( iCutTimeDisplay )
		{
		iCutTimeDisplay->SetCutIn( aTime );
		}
	if ( iCutVideoBar )
		{
		iCutVideoBar->SetInPoint( aTime );
		}
	}

void CVeiSimpleCutVideoContainer::SetOutTime( const TTimeIntervalMicroSeconds& aTime )
	{
	if ( iCutTimeDisplay )
		{
		iCutTimeDisplay->SetCutOut( aTime );
		}
	if ( iCutVideoBar )
		{
		iCutVideoBar->SetOutPoint( aTime );
		}
	}

TTimeIntervalMicroSeconds CVeiSimpleCutVideoContainer::PlaybackPositionL()
	{
	if ( ( iSeeking ) || ( EStateStopped == iState ) )
		{
		return iSeekPos;
		}
	if ( iState != EStatePlaying  )
		{
		return iLastPosition;
		}

	if ( iVideoClipInfo && (iVideoDisplay->PositionL() < iVideoClipInfo->Duration()) )
		{
		iLastPosition = iVideoDisplay->PositionL();
		}

	if ( ( iLastPosition == TTimeIntervalMicroSeconds( 0 ) ) &&
		 ( iSeekPos != TTimeIntervalMicroSeconds( 0 ) ) )
		{
		return iSeekPos;
		}

	return iLastPosition;
	}

TKeyResponse CVeiSimpleCutVideoContainer::OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType)
	{
	if ( aType == EEventKeyDown ) 
		{
		iKeyRepeatCount = 0;
		if(aKeyEvent.iScanCode == EStdKeyDevice7) //for camera key
			{
			PauseL(ETrue);
			}
		TBool shiftKeyPressed = (aKeyEvent.iModifiers & EModifierShift );
		if( shiftKeyPressed )
			{
			if( iView.IsEnoughFreeSpaceToSaveL() && (iTakeSnapshot == EFalse )) 
				{
				/*if (EStatePlaying == iState)
					{
					PauseL();	
					}
				*/	
				TakeSnapshotL();
				}
			return EKeyWasConsumed;
			}	

		return EKeyWasNotConsumed;
		}
	else if ( aType == EEventKeyUp ) 
		{
		if ( iSeeking == EFalse )
			{
			return EKeyWasNotConsumed;
			}
		iLastPosition = iSeekPos;
		iVideoDisplay->SetPositionL( iSeekPos );

		iSeeking = EFalse;

		if ( ( ( EStateStopped == iState ) || ( EStateStoppedInitial == iState )
			|| ( EStatePaused == iState )) && 
			 ( ( iLastKeyCode == EKeyLeftArrow ) || ( iLastKeyCode == EKeyRightArrow ) ) )
			{
			LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::OfferKeyEventL: 1, calling GetThumbAtL()");
			GetThumbAtL( iSeekPos );
			return EKeyWasConsumed;
			}
		else if ( EStatePlaying == iState )
			{
			if ( iVideoBarTimer->IsActive() )
				{
				iVideoBarTimer->Cancel();
				}

			if ( iVideoClipInfo &&  (iSeekPos >= iVideoClipInfo->Duration()) )
				{
				LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::OfferKeyEventL: 2, calling iVideoDisplay->Stop");
				iVideoDisplay->Stop( EFalse );
				}
			else
				{
				iVideoDisplay->SetPositionL( iSeekPos );
				iVideoDisplay->ShowBlackScreen();
				if (iVideoClipInfo && !iFrameReady)
					{								
					iVideoClipInfo->CancelFrame();
					}
				iVideoDisplay->Play();
				}
			return EKeyWasConsumed;
			}
		else
			{
			return EKeyWasConsumed;
			}
		}
	else if ( aType == EEventKey )
		{
		iLastKeyCode = aKeyEvent.iCode;

		switch (aKeyEvent.iCode)
			{
			case EKeyEnter:
				{
			    if ( EStateStoppedInitial == iState )
					{
			        iView.HandleCommandL( EVeiCmdCutVideoViewPlay );
					}
				else
					{
					if ( EStatePlaying == iState || EStatePlayingMenuOpen == iState )
						{		
						PauseL();	
						}
					iView.ProcessCommandL( EAknSoftkeyOptions );
					}
				return EKeyWasConsumed;
				}
			case EKeyOK:
				{
				iView.HandleCommandL( EAknSoftkeyOk );
				return EKeyWasConsumed;
				}
			case EKeyDownArrow:
			case EKeyUpArrow:
				{
				iVideoDisplay->OfferKeyEventL( aKeyEvent, aType );
				return EKeyWasConsumed;
				}
			case EKeyRightArrow:
				{
				iKeyRepeatCount++;

				if ( (iKeyRepeatCount > 2)	&& (iSeeking == EFalse) )
					{
					if ( EStatePlaying == iState )
						{
						iSeekPos = iVideoDisplay->PositionL();
						}
	
					iVideoDisplay->PauseL();
					iSeeking = ETrue;
					iKeyRepeatCount = 0;				
					}

				if ( iSeeking && ( iState!=EStateInitializing ) &&
					( iState!=EStatePlayingMenuOpen ) )
					{
					TInt adjustment = TimeIncrement( iKeyRepeatCount );

					TInt64 newPos = iSeekPos.Int64() + adjustment;
					if ( iVideoClipInfo && (newPos > iVideoClipInfo->Duration().Int64()) )
						{
						newPos = iVideoClipInfo->Duration().Int64();
						}
	
					iSeekPos = TTimeIntervalMicroSeconds( newPos );
					
					iCutVideoBar->SetCurrentPoint( (static_cast<TInt32>(iSeekPos.Int64() / 1000)));

					iView.UpdateTimeL();
					return EKeyWasConsumed;
					}
				return EKeyWasNotConsumed;
				}

			case EKeyLeftArrow:
				{
				iKeyRepeatCount++;

				if ( (iKeyRepeatCount > 2)	&& (iSeeking == EFalse) )
					{
					
					if ( EStatePlaying == iState )
						{
						iSeekPos = iVideoDisplay->PositionL();
						}

					iVideoDisplay->PauseL();
					iSeeking = ETrue;
					iKeyRepeatCount = 0;				
					}

				if ( iSeeking && ( iState!=EStateInitializing ) &&
					( iState!=EStatePlayingMenuOpen ) )
					{

					TInt adjustment = TimeIncrement( iKeyRepeatCount );

					TInt64 newPos = iSeekPos.Int64() - adjustment;
					if ( newPos < 0 ) 
						{
						newPos = 0;
						}
					iSeekPos = TTimeIntervalMicroSeconds( newPos );	

					iCutVideoBar->SetCurrentPoint( static_cast<TInt32>((iSeekPos.Int64() / 1000)) );

					iView.UpdateTimeL();

					return EKeyWasConsumed;
					}
				return EKeyWasNotConsumed;
		        }
			case EKeyBackspace:		//Clear 
				{
				if (EStatePlaying != iState)
					{
					iView.ClearInOutL( ETrue, ETrue );						
					}
				
				return EKeyWasConsumed;
				}
			default:
			    {
				return EKeyWasNotConsumed;
		        }
			}
		}
	else
		{
		return EKeyWasNotConsumed;
		}
	}

TInt CVeiSimpleCutVideoContainer::TimeIncrement(TInt aKeyCount) const
	{
	if ( aKeyCount < 3 )
		{
		return 100000;
		}
	else if ( aKeyCount < 4 )
		{
		return 300000;
		}
	else if ( aKeyCount < 5 )
		{
		return 500000;
		}
	else if ( aKeyCount < 10 )
		{
		return 1000000;
		}
	else if ( aKeyCount < 13 )
		{
		return 2000000;
		}
	else if ( aKeyCount < 15 )
		{
		return 3000000;
		}
	else
		{
		return 5000000;
		}	
	}

TInt CVeiSimpleCutVideoContainer::UpdateProgressNote()
    {
    TTime intervalTime;
    intervalTime.HomeTime();
    intervalTime += TTimeIntervalMicroSeconds( 50000 );
    TTime currentTime;
    currentTime.HomeTime();
    while ( intervalTime > currentTime )
        {
        currentTime.HomeTime();
        }

    iProgressDialog->GetProgressInfoL()->IncrementAndDraw( 1 );

    if ( KProgressbarFinalValue <= iProgressDialog->GetProgressInfoL()->CurrentValue() )
        {
        return 0;
        }
    return 1;
    }


void CVeiSimpleCutVideoContainer::GetThumbL( const TDesC& aFilename )
	{
	if ( iVideoClipInfo )
		{
		delete iVideoClipInfo;
		iVideoClipInfo = NULL;		
		}

	/*iVideoClipInfo = */CVedVideoClipInfo::NewL( aFilename, *this );
	}


void CVeiSimpleCutVideoContainer::GetThumbAtL( const TTimeIntervalMicroSeconds& aTime )
	{
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::GetThumbAtL: In");
	if( !iVideoClipInfo )
		{
		LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::GetThumbAtL: 1");
		return;
		}
	if ( !iFrameReady )
		{
		LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::GetThumbAtL: 1.5");
		iVideoClipInfo->CancelFrame();
		}

	TRect clipResolution = iVideoClipInfo->Resolution();
	TSize resolution( iVideoDisplay->Size() );

	TInt frameIndex;
	TInt totalFrameCount;

	frameIndex = iVideoClipInfo->GetVideoFrameIndexL( aTime );
	totalFrameCount = iVideoClipInfo->VideoFrameCount();

	if ( frameIndex > totalFrameCount )
		{
		frameIndex = totalFrameCount;
		}

	//Generates a thumbnail bitmap of the given frame from video clip
	if ( iTakeSnapshot )
		{
		LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::GetThumbAtL: 2");
		TDisplayMode displayMode = ENone;
		TBool enhance = EFalse;
		TSize resol( clipResolution.iBr.iX, clipResolution.iBr.iY ); 
    	/* :
	     check out on every phone before releasing whether videodisplay should be stopped before starting
	     asynchronous GetFrameL()
	     see how EStateGettingFrame is handled in SetPreviewState 
	     Stopping frees memory and it is needed in memory sensible devices 
	    */
		iVideoClipInfo->GetFrameL( *this, frameIndex, &resol ); //, displayMode, enhance );
		SetStateL( EStateGettingFrame );
		iFrameReady = EFalse;			
//		ShowProgressNoteL();		
		}
	else
		{
		LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::GetThumbAtL: 3");
		/* :
	     check out on every phone before releasing whether videodisplay should be stopped before starting
	     asynchronous GetFrameL()
	     see how EStateGettingFrame is handled in SetPreviewState 
	     Stopping frees memory and it is needed in memory sensible devices 
	    */
		TSize resol( clipResolution.iBr.iX, clipResolution.iBr.iY );
		iVideoClipInfo->GetFrameL( *this, frameIndex, &resol ); 
		SetStateL( EStateGettingFrame );
		iFrameReady = EFalse;		
		}
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::GetThumbAtL: out");	
	}

void CVeiSimpleCutVideoContainer::NotifyCompletion( TInt aErr ) 
	{
	LOGFMT(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::NotifyCompletion: In, aErr:%d", aErr);	

	if (EStateTerminating == iState)
		{
		LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::NotifyCompletion(): 1, app is closing...");
		return;
		}

    if ( KErrNone == aErr )
        {
        //Update notification to Media Gallery
        LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::NotifyCompletion: 2");

	    if (iProgressDialog)
		    {
		    TRAP_IGNORE(iProgressDialog->GetProgressInfoL()->SetAndDraw( KProgressbarFinalValue );
		    			iProgressDialog->ProcessFinishedL());
		    }
        }
    else
		{
		if (iProgressDialog)
			{
		    TRAP_IGNORE(iProgressDialog->GetProgressInfoL()->SetAndDraw( KProgressbarFinalValue );
		    			iProgressDialog->ProcessFinishedL());
		    }
		iErrorUI.ShowGlobalErrorNote( aErr );
		}

	//  to eliminate previous (wrong) output file from being deleted in CancelSnapshotSave()
	delete iSaveToFileName;
    iSaveToFileName = NULL;

	// to fix progress bar shadow
	DrawDeferred();

	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::NotifyCompletion: Out");
	}

void CVeiSimpleCutVideoContainer::NotifyVideoClipInfoReady( CVedVideoClipInfo& aInfo, 
										  TInt aError )
	{
	LOGFMT(KVideoEditorLogFile, "CVeisimpleCutVideoContainer::NotifyVideoClipInfoReady, In, aError:%d", aError);
	if (KErrNone == aError)
		{
		if (iVideoClipInfo)		
			{
			delete iVideoClipInfo;
			iVideoClipInfo = NULL;	
			}
		iVideoClipInfo = &aInfo;	

		TRect clipResolution = iVideoClipInfo->Resolution();
		iDuration = iVideoClipInfo->Duration();
		iCutVideoBar->SetTotalDuration( iDuration );
		iView.DrawTimeNaviL();

		TSize resolution( clipResolution.iBr.iX, clipResolution.iBr.iY ); 
		iFrameReady = EFalse;
		iVideoClipInfo->GetFrameL( *this, 0, &resolution );
		}
	SetStateL( EStateStoppedInitial );			
	LOG(KVideoEditorLogFile, "CVeisimpleCutVideoContainer::NotifyVideoClipInfoReady, Out");
	}


void CVeiSimpleCutVideoContainer::NotifyVideoClipFrameCompleted(CVedVideoClipInfo& /*aInfo*/, 
											   TInt aError, 
							 				   CFbsBitmap* aFrame)
	{
	LOGFMT(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::NotifyVideoClipFrameCompleted, In, aError:%d", aError);
	iFrameReady = ETrue;
	
	if (EStateGettingFrame == iState)
		{
		SetStateL(iPreviousState);			
		}		
	
	if (KErrNone == aError && aFrame)
		{
		LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::NotifyVideoClipFrameCompleted 1");	

		if ( iTakeSnapshot )
			{
			LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::NotifyVideoClipFrameCompleted 2");			
			iConverter->SetBitmap( aFrame );
			if (! iCallBackSaveSnapshot)
				{		
				TCallBack cb (CVeiSimpleCutVideoContainer::AsyncSaveSnapshot, this);
				iCallBackSaveSnapshot = new (ELeave) CAsyncCallBack(cb, CActive::EPriorityStandard);
				}
			iCallBackSaveSnapshot->CallBack();
			LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::NotifyVideoClipFrameCompleted 3");
			}
		else
			{
			LOG(KVideoEditorLogFile, "CVeisimpleCutVideoContainer::NotifyVideoClipFrameCompleted 4");
			TRAP_IGNORE(iVideoDisplay->ShowPictureL( *aFrame ));
			delete aFrame;
			aFrame = NULL;
			
			if (iTakeSnapshotWaiting)
				{
				if (! iCallBackTakeSnapshot)
					{		
					TCallBack cb (CVeiSimpleCutVideoContainer::AsyncTakeSnapshot, this);
					iCallBackTakeSnapshot = new (ELeave) CAsyncCallBack(cb, CActive::EPriorityStandard);
					}
				iCallBackTakeSnapshot->CallBack();				
				}
			}
		}
	else
		{
		LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::NotifyVideoClipFrameCompleted 5");
		if (aFrame)	
			{
			delete aFrame;
			aFrame = NULL;	
			}
		
		if (iProgressDialog)
			{
			iProgressDialog->GetProgressInfoL()->SetAndDraw( KProgressbarFinalValue );
		    TRAP_IGNORE(iProgressDialog->ProcessFinishedL());
			}		
		}
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::NotifyVideoClipFrameCompleted, Out");	
	}

TInt CVeiSimpleCutVideoContainer::AsyncSaveSnapshot(TAny* aThis)
	{
    LOG( KVideoEditorLogFile, "CVeiSimpleCutVideoView::AsyncSaveSnapshot in");
	
    // In the asynchronous version, trap the rest of the functions 
    // to make sure that the caller's TRequestStatus is always 
    // completed, also in case of failures.
	CVeiSimpleCutVideoContainer* container = static_cast<CVeiSimpleCutVideoContainer*>(aThis);
	TInt err = KErrNone;
	TRAP(err, container->SaveSnapshotL());
	LOGFMT( KVideoEditorLogFile, "CVeiEditVideoView::AsyncSaveSnapshot 2, err:%d", err);
	
	if (KErrNone != err)
		{
		container->StopProgressDialog();
		container->ShowGlobalErrorNote(err);
		}  
	LOG( KVideoEditorLogFile, "CVeiEditVideoView::AsyncSaveSnapshot 3, returning");
	return KErrNone;				
	}		
	
void CVeiSimpleCutVideoContainer::ShowGlobalErrorNote(const TInt aErr)
	{		
	iErrorUI.ShowGlobalErrorNote( aErr );
	}

void CVeiSimpleCutVideoContainer::StopProgressDialog()
	{
	if (iProgressDialog)
	    {
	    TRAP_IGNORE(iProgressDialog->GetProgressInfoL()->SetAndDraw( KProgressbarFinalValue );
	    			iProgressDialog->ProcessFinishedL());
	    }	
	}

void CVeiSimpleCutVideoContainer::NotifyVideoDisplayEvent( const TPlayerEvent aEvent, const TInt& aInfo )
	{
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::NotifyVideoDisplayEvent, In");
	
	if (EStateTerminating == iState)
		{
		LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::NotifyVideoDisplayEvent(): 1, app is closing...");
		return;
		}
	
	switch (aEvent)
		{
		case MVeiVideoDisplayObserver::ELoadingStarted:
			{	
			SetStateL(EStateOpening);
			LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::ELoadingStarted");
			break;
			}
		case MVeiVideoDisplayObserver::EOpenComplete:
			{
			LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::NotifyVideoDisplayEvent, MVeiVideoDisplayObserver::EOpenComplete 1");		
            iVideoDisplay->SetRotationL( EVideoRotationNone );
			TTimeIntervalMicroSeconds cutInTime = iView.GetVideoClipCutInTime();
			TTimeIntervalMicroSeconds cutOutTime = iView.GetVideoClipCutOutTime();

			if ( iView.IsForeground() )
				{				
				if (iVideoClipInfo && !iFrameReady)
					{								
					iVideoClipInfo->CancelFrame();
					}							
				if ( iPlayOrPlayMarked )
					{
					LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::NotifyVideoDisplayEvent, MVeiVideoDisplayObserver::EOpenComplete 2");					
					iVideoDisplay->PlayMarkedL( cutInTime, cutOutTime );
					}
				else
					{
					LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::NotifyVideoDisplayEvent, MVeiVideoDisplayObserver::EOpenComplete 3");	
					iVideoDisplay->Play();
					}
				iPlayOrPlayMarked = EFalse;				
				}
			else
				{
				LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::EOpenComplete 4");					
				PauseL();					
				}	
			break;
			}
		case MVeiVideoDisplayObserver::EBufferingStarted:
			{
			LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::EBufferingStarted");			
			SetStateL( EStateBuffering );
			if ( iVideoBarTimer )
                {
                iVideoBarTimer->Cancel();
                }
			break;
			}	
		case MVeiVideoDisplayObserver::ELoadingComplete:
			{
			LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::NotifyVideoDisplayEvent, MVeiVideoDisplayObserver::ELoadingComplete 1");
			// if VED is put to background in the middle of the buffering
			// iVideoDisplay->PauseL(); cannot be called during the buffering, so its called here
			if (EStatePaused == iState)
				{
				iVideoDisplay->PauseL();	
				}
			else
				{							
				SetStateL( EStatePlaying );						
				if (iVideoClipInfo && !iFrameReady)
					{								
					LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::NotifyVideoDisplayEvent, MVeiVideoDisplayObserver::ELoadingComplete 2");
					iVideoClipInfo->CancelFrame();
					}
				if ( !iVideoBarTimer->IsActive() )
					{
					LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::NotifyVideoDisplayEvent, MVeiVideoDisplayObserver::ELoadingComplete 3");				
					const TUint delay = 100000;
					iVideoBarTimer->Start( delay, delay, TCallBack( CVeiSimpleCutVideoContainer::DoAudioBarUpdate, this ) );
					}
				iVideoDisplay->ShowBlackScreen();			
				DrawDeferred();
				}
			break;
			}
		case MVeiVideoDisplayObserver::EPlayComplete:
			{
			LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::NotifyVideoDisplayEvent, MVeiVideoDisplayObserver::EPlayComplete");
			iVideoDisplay->SetBlackScreen( EFalse );			
			iSeekPos = TTimeIntervalMicroSeconds( 0 );

			iLastPosition = TotalLength();
			iView.StopNaviPaneUpdateL();
			iCutVideoBar->SetFinishedStatus( ETrue );
			
			if (iVideoBarTimer)
				{
				iVideoBarTimer->Cancel();	
				}			

			GetThumbAtL(0);
			iVideoDisplay->Stop( ETrue );
			SetStateL( EStateStopped );
			
			if (KErrNoMemory == aInfo || KErrSessionClosed == aInfo)
				{
				iErrorUI.ShowGlobalErrorNote( aInfo );					
				StopL();
				CloseStreamL();
				}

			DrawDeferred();
			break;
			}
		case MVeiVideoDisplayObserver::EStop:
			{
			LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::EStop");
			
			if (EStateGettingFrame == iState)
	            {
	            break;	
	            }	           
			
			if ( iVideoBarTimer )
                {                
                iVideoBarTimer->Cancel();
                }
			iSeekPos = TTimeIntervalMicroSeconds( 0 );

			GetThumbAtL(0);
			iLastPosition = TotalLength();
			iView.StopNaviPaneUpdateL();
			iCutVideoBar->SetFinishedStatus( ETrue );
			iVideoDisplay->Stop( ETrue );
			SetStateL( EStateStopped );
			DrawDeferred();
			break;
			}																
		case MVeiVideoDisplayObserver::EVolumeLevelChanged:
			{
			LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::NotifyVideoDisplayEvent, MVeiVideoDisplayObserver::EVolumeLevelChanged");
			TInt playerVolume = iVideoDisplay->Volume();
			iView.ShowVolumeLabelL( playerVolume );
			break;
			}
		case MVeiVideoDisplayObserver::EError:
			{
			LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::EError");
//			iErrorUI.ShowGlobalErrorNoteL( KErrGeneral );
			SetStateL( EStateStoppedInitial );
			break;
			}	
		default:
			{
			LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::NotifyVideoDisplayEvent, MVeiVideoDisplayObserver::default");
			break;
			};
		}
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::NotifyVideoDisplayEvent, Out");
	}


void CVeiSimpleCutVideoContainer::SetStateL(CVeiSimpleCutVideoContainer::TCutVideoState aState, TBool aUpdateCBA)
	{
	LOGFMT2(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::SetStateL: in, iState:%d, aState:%d", iState, aState);
	if (EStateGettingFrame == aState)
		{
		iPreviousState = iState;	
		}
		
	iState = aState;
	if (EStatePaused == iState)
		{
		DrawNow();	
		}

	// If the foreground is lost while an arrow key is down, we do not get
	// the key up -event, and iSeeking remains true. Reseting it here just in case.
	iSeeking = EFalse;

	if ( aUpdateCBA )
		{
		iView.UpdateCBAL( iState );
		}

	//#ifdef STOP_PLAYER_DURING_GETFRAME
	if (EStateGettingFrame == aState)
		{
		/* :
	     check out on every phone before releasing whether videodisplay should be stopped before starting
	     asynchronous GetFrameL()
	     see how EStateGettingFrame is handled in SetPreviewState 
	     Stopping frees memory and it is needed in memory sensible devices 
	    */	
		//iVideoDisplay->Stop(ETrue);		    		
		}
	//#endif	

	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::SetStateL:: out");
	}


void CVeiSimpleCutVideoContainer::MarkedInL()
	{
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::MarkedInL, In");
	if ( EStateInitializing == iState )
		{
		return;
		}
	const TTimeIntervalMicroSeconds& position = PlaybackPositionL();
	iSeekPos = position;

	LOGFMT(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::MarkedInL, 2, setting cutINpoint:%Ld", position.Int64());
	iCutVideoBar->SetInPoint( position	);
	TInt tempPos = static_cast<TInt32>((position.Int64()/1000));
	iCutVideoBar->SetCurrentPoint( tempPos );

	if( !AknLayoutUtils::PenEnabled() )
		{
		iCutTimeDisplay->SetCutIn( position );
		}

	PauseL();
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::MarkedInL, Out");
	}

void CVeiSimpleCutVideoContainer::MarkedOutL()
	{
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::MarkedOutL, In");
	if ( EStateInitializing == iState )
		{
		return;
		}
	const TTimeIntervalMicroSeconds& position = PlaybackPositionL();
	iSeekPos = position;

	LOGFMT(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::MarkedOutL, 2, setting cutOUTpoint:%Ld", position.Int64());
	iCutVideoBar->SetOutPoint( position );
	TInt tempPos = static_cast<TInt32>((position.Int64()/1000));
	iCutVideoBar->SetCurrentPoint( tempPos );

	if( !AknLayoutUtils::PenEnabled() )
		{
		iCutTimeDisplay->SetCutOut( position );
		}

	PauseL();
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::MarkedOutL, Out");
	}

void CVeiSimpleCutVideoContainer::ShowInformationNoteL( const TDesC& aMessage ) const
	{
	CAknInformationNote* note = new ( ELeave ) CAknInformationNote( ETrue );
	note->ExecuteLD( aMessage );
	}

void CVeiSimpleCutVideoContainer::ShowProgressNoteL() 
	{
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::ShowProgressNoteL, in");
	iProgressDialog = new ( ELeave ) CAknProgressDialog( REINTERPRET_CAST( CEikDialog**, 
					&iProgressDialog ), ETrue);
    iProgressDialog->SetCallback( this );
    iProgressDialog->PrepareLC( R_VEI_PROGRESS_NOTE_WITH_CANCEL );

	HBufC* stringholder;
    TApaAppCaption caption;
    TRAPD( err, ResolveCaptionNameL( caption ) );
    
    // If something goes wrong, show basic "Saving" note
    if ( err )
        {
        stringholder = iEikonEnv->AllocReadResourceLC( R_VEI_PROGRESS_NOTE_SAVING );
        }
    else
        {
        stringholder = StringLoader::LoadLC( R_VEI_PROGRESS_NOTE_SAVING_IMAGE, caption, iEikonEnv );
        }        

	iProgressDialog->SetTextL( *stringholder );
	CleanupStack::PopAndDestroy( stringholder );

    iProgressDialog->GetProgressInfoL()->SetFinalValue( KProgressbarFinalValue );
    iProgressDialog->RunLD();

	iProgressDialog->GetProgressInfoL()->SetAndDraw( 50 );
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::ShowProgressNoteL, Out");
	}

//=============================================================================
void CVeiSimpleCutVideoContainer::ResolveCaptionNameL( TApaAppCaption& aCaption ) const
    {   
    RApaLsSession appArcSession;
    CleanupClosePushL( appArcSession );
    User::LeaveIfError( appArcSession.Connect() );       	    

    // Get Media Gallery caption
    TApaAppInfo appInfo;
    User::LeaveIfError( appArcSession.GetAppInfo( appInfo, TUid::Uid( KMediaGalleryUID3 ) ) );

    aCaption = appInfo.iCaption;

    CleanupStack::PopAndDestroy( &appArcSession );  
    }

TInt CVeiSimpleCutVideoContainer::DoAudioBarUpdate( TAny* aThis )
	{
	STATIC_CAST( CVeiSimpleCutVideoContainer*, aThis )->DoUpdate();
	return 42;
	}

void CVeiSimpleCutVideoContainer::DoUpdate()
	{
	TTimeIntervalMicroSeconds time;
	time = iVideoDisplay->PositionL();
	if ( iSeeking )
		{
		time = iSeekPos;
		LOGFMT(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::DoUpdate(): 1, time:%Ld", time.Int64());
		}
	else
		{			
		LOGFMT(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::DoUpdate(): 2, time:%Ld", time.Int64());
		}
	iCutVideoBar->SetCurrentPoint( static_cast<TInt32>((time.Int64() / 1000)));
	iCutVideoBar->DrawDeferred();
	}
void CVeiSimpleCutVideoContainer::MuteL()
	{
	iVideoDisplay->SetMuteL( ETrue );
	}

//=============================================================================
void CVeiSimpleCutVideoContainer::HandleVolumeUpL()
	{
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::HandleVolumeUpL: in");

	iVideoDisplay->AdjustVolumeL( 1 );

	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::HandleVolumeUpL: out");
	}
	
	
void CVeiSimpleCutVideoContainer::SetVolumeLevelL( TInt aVolume )
    {
    iVideoDisplay->AdjustVolumeL( aVolume - iVideoDisplay->Volume() );
    }	

//=============================================================================
void CVeiSimpleCutVideoContainer::HandleVolumeDownL()
	{
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::HandleVolumeDownL: in");

	iVideoDisplay->AdjustVolumeL( -1 );

	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::HandleVolumeDownL: out");
	}

//=============================================================================
void CVeiSimpleCutVideoContainer::PrepareForTerminationL()
	{
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::PrepareForTerminationL: in");

	SetStateL( EStateTerminating );

	if( !iFrameReady && iVideoClipInfo )
		{
		iVideoClipInfo->CancelFrame();
		}
	iState = EStateTerminating;
	iVideoDisplay->Stop( ETrue );

	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::PrepareForTerminationL: out");
	}

TInt CVeiSimpleCutVideoContainer::AsyncTakeSnapshot(TAny* aThis)
	{
    LOG( KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::AsyncTakeSnapshot");
	
    // In the asynchronous version, trap the rest of the functions 
    // to make sure that the caller's TRequestStatus is always 
    // completed, also in case of failures.
	CVeiSimpleCutVideoContainer* container = static_cast<CVeiSimpleCutVideoContainer*>(aThis);
	TInt err = KErrNone;
	TRAP(err, container->TakeSnapshotL());
	LOGFMT( KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::AsyncTakeSnapshot 1, err:%d", err);	
	User::LeaveIfError(err);		
	return KErrNone;				
	}

void CVeiSimpleCutVideoContainer::HandleVideoClickedL()
    {
    LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoContainer::HandleVideoClickedL");

    if ( EStatePlaying != iState && EStatePlayingMenuOpen != iState )
		{
        iView.HandleCommandL( EVeiCmdCutVideoViewPlay );
		}
	else if ( EStatePlaying == iState || EStatePlayingMenuOpen == iState )
		{		
		PauseL();	
//        iView.HandleCommandL( EVeiCmdCutVideoViewStop );
		}
    }

void CVeiSimpleCutVideoContainer::FocusChanged( TDrawNow /*aDrawNow*/ )
	{
	if (IsFocused())
		{
		DrawDeferred();
		}
	}
// End of File  
