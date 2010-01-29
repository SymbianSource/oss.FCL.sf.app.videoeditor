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
#include <manualvideoeditor.rsg>
#include <videoeditoruicomponents.mbg>
#include <eikbtgpc.h>
#include <vedvideoclipinfo.h>
#include <coemain.h>
#include <eikenv.h>
#include <aknsbasicbackgroundcontrolcontext.h> 
#include <aknsdrawutils.h> 
#include <aknsdatacontext.h> 
#include <stringloader.h>
#include <aknnotewrappers.h> 
#include <pathinfo.h> 
#include <eikprogi.h>
#include <aknlayoutscalable_avkon.cdl.h>
#include <aknlayoutscalable_apps.cdl.h>
// User includes
#include "VeiAppUi.h"
#include "VeiCutterBar.h"
#include "VeiCutVideoContainer.h"
#include "VeiCutVideoView.h"
#include "veitextdisplay.h"
#include "VideoEditorHelp.hlp.hrh"  // Topic contexts (literals)
#include "VideoEditorCommon.h"
#include "VideoEditorUtils.h"
#include "VeiErrorUi.h"


// ================= MEMBER FUNCTIONS =======================
void CleanupRarray( TAny* object )
    {
    (( RImageTypeDescriptionArray*)object)->ResetAndDestroy();
    }

CVeiCutVideoContainer* CVeiCutVideoContainer::NewL( const TRect& aRect, CVeiCutVideoView& aView, CVeiErrorUI& aErrorUI )
    {
    CVeiCutVideoContainer* self = CVeiCutVideoContainer::NewLC( aRect, aView, aErrorUI );
    CleanupStack::Pop( self );
    return self;
    }

CVeiCutVideoContainer* CVeiCutVideoContainer::NewLC( const TRect& aRect, CVeiCutVideoView& aView, CVeiErrorUI& aErrorUI )
    {
    CVeiCutVideoContainer* self = new (ELeave) CVeiCutVideoContainer( aRect, aView, aErrorUI );
    CleanupStack::PushL( self );
    self->ConstructL( aRect, aView, aErrorUI );
    return self;
    }

void CVeiCutVideoContainer::ConstructL( const TRect& aRect, CVeiCutVideoView& /*aView*/, CVeiErrorUI& /*aErrorUI*/ )
    {
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::ConstructL: in");
    CreateWindowL();

    iInternalVolume = 0;

    iState = EStateInitializing;
    iFrameReady = EFalse;
    iPlayOrPlayMarked = EFalse;

    iSeekPos = TTimeIntervalMicroSeconds( 0 );
    iSeeking = EFalse;
    iCutVideoBar = CVeiCutterBar::NewL( this );

    iConverter = CVeiImageConverter::NewL( this );
    iTakeSnapshot = EFalse;
    iVideoDisplay = CVeiVideoDisplay::NewL( iDisplayRect, this, *this );
    iCutTimeDisplay = CVeiTextDisplay::NewL( iCutTimeDisplayRect, this );

    TFileName mbmPath( VideoEditorUtils::IconFileNameAndPath(KVideoEditorUiComponentsIconFileId) ); 

    AknIconUtils::CreateIconL( iPauseBitmap, iPauseBitmapMask,
            mbmPath, EMbmVideoeditoruicomponentsQgn_prop_ve_pause, 
            EMbmVideoeditoruicomponentsQgn_prop_ve_pause_mask );

    SetRect( aRect );

    iBgContext = CAknsBasicBackgroundControlContext::NewL( KAknsIIDQsnBgAreaMain, Rect(), EFalse );
    iVideoBarTimer = CPeriodic::NewL( CActive::EPriorityLow );

    EnableDragEvents();

    ActivateL();

    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::ConstructL: out");
    }

CVeiCutVideoContainer::CVeiCutVideoContainer( const TRect& /*aRect*/, CVeiCutVideoView& aView, CVeiErrorUI& aErrorUI ):iView( aView ), iErrorUI( aErrorUI )
    {
    }


CVeiCutVideoContainer::~CVeiCutVideoContainer()
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

    delete iPauseBitmap;
    delete iPauseBitmapMask;

    delete iRemConTarget;
    
    delete iCallBackSaveSnapshot;
    delete iCallBackTakeSnapshot;
    }

void CVeiCutVideoContainer::DialogDismissedL( TInt aButtonId )
    {
    if ( aButtonId == -1 )
        { // when pressing cancel button.
        CancelSnapshotSave();
        }
    iTakeSnapshot = EFalse;
    }

void CVeiCutVideoContainer::SizeChanged()
    {
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::SizeChanged(): In");
	TSize videoScreenSize;
    TRect rect( Rect() ); 
	if ( iBgContext )
		{
		iBgContext->SetRect( rect );
		}
	LOGFMT2(KVideoEditorLogFile, "CVeiCutVideoContainer::SizeChanged(): Rect(): %d,%d", rect.iBr.iX, rect.iBr.iY);

	// Scissor icon
	TAknLayoutRect scissorsIconLayout;
	scissorsIconLayout.LayoutRect(Rect(),AknLayoutScalable_Apps::main_vded_pane_g1());
	iCutVideoBar->SetComponentRect(CVeiCutterBar::EScissorsIcon, scissorsIconLayout.Rect());
		
	// Progress bar
	TAknLayoutRect progressBarLayout; 
	progressBarLayout.LayoutRect(Rect(),AknLayoutScalable_Apps::vded_slider_pane());
	iCutVideoBar->SetComponentRect(CVeiCutterBar::EProgressBar, progressBarLayout.Rect());

	// left end of the slider when that part is unselected
	TAknLayoutRect sliderLeftEndLayout;
	sliderLeftEndLayout.LayoutRect( progressBarLayout.Rect(),AknLayoutScalable_Apps::vded_slider_pane_g3() );
	iCutVideoBar->SetComponentRect( CVeiCutterBar::ESliderLeftEndIcon, sliderLeftEndLayout.Rect() );
		
	// middle part of the slider when that part is unselected	
	TAknLayoutRect sliderMiddleLayout;
	sliderMiddleLayout.LayoutRect(progressBarLayout.Rect(),AknLayoutScalable_Apps::vded_slider_pane_g5());
	iCutVideoBar->SetComponentRect( CVeiCutterBar::ESliderMiddleIcon, sliderMiddleLayout.Rect() );		
	
	// right end of the slider when that part is unselected
	TAknLayoutRect sliderRightEndLayout;
	sliderRightEndLayout.LayoutRect(progressBarLayout.Rect(),AknLayoutScalable_Apps::vded_slider_pane_g4());
	iCutVideoBar->SetComponentRect( CVeiCutterBar::ESliderRightEndIcon, sliderRightEndLayout.Rect() );	
		
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
		
		
	// Start cut time text
	TAknLayoutText startTimeTextLayout;
	startTimeTextLayout.LayoutText(rect, AknLayoutScalable_Apps::main_vded_pane_t1() );
	iCutTimeDisplay->SetComponentRect(CVeiTextDisplay::EStartTimeText, startTimeTextLayout.TextRect());
	
	// End cut time text
	TAknLayoutText endTimeTextLayout;
	endTimeTextLayout.LayoutText(rect, AknLayoutScalable_Apps::main_vded_pane_t2() );
	iCutTimeDisplay->SetComponentRect(CVeiTextDisplay::EEndTimeText, endTimeTextLayout.TextRect());
	
	// Start cut time icon
	TAknLayoutRect startTimeIconLayout;
	startTimeIconLayout.LayoutRect(rect, AknLayoutScalable_Apps::main_vded_pane_g2() );
	iCutTimeDisplay->SetComponentRect(CVeiTextDisplay::EStartTimeIcon, startTimeIconLayout.Rect());
	
	// End cut time icon
	TAknLayoutRect endTimeIconLayout;
	endTimeIconLayout.LayoutRect(rect, AknLayoutScalable_Apps::main_vded_pane_g3() );
	iCutTimeDisplay->SetComponentRect(CVeiTextDisplay::EEndTimeIcon, endTimeIconLayout.Rect());
		
	// Pause icon
	


	// Video Display	
	TAknLayoutRect videoDisplayLayout;
	videoDisplayLayout.LayoutRect(Rect(),AknLayoutScalable_Apps::vded_video_pane());
	iVideoDisplay->SetRect(videoDisplayLayout.Rect());
	
	iDisplayRect = videoDisplayLayout.Rect();	
	iIconDisplayRect = videoDisplayLayout.Rect();
	LOGFMT2(KVideoEditorLogFile, "CVeiCutVideoContainer::SizeChanged(): iDisplayRect: %d,%d", iDisplayRect.iBr.iX, iDisplayRect.iBr.iY);

	//CVeiCutterBar
	// : Change this when LAF data is ready
	TPoint cutBarTl = TPoint(STATIC_CAST( TInt, rect.iBr.iX*0.0114 ),
		STATIC_CAST( TInt, rect.iBr.iY*0.875 ) );
	TSize cutBarSize = TSize(STATIC_CAST( TInt, rect.iBr.iX*0.9773 ),
		STATIC_CAST( TInt, rect.iBr.iY*0.0973 ) );

	TRect cutBarRect( cutBarTl, cutBarSize );
	iCutVideoBar->SetRect( cutBarRect );

	//CVeiTextDisplay
	// : Change this when LAF data is ready
	TPoint cutTimeDisplayTl = TPoint(cutBarTl.iX,
		STATIC_CAST( TInt, rect.iBr.iY*0.757 ) );
	TSize cutTimeDisplaySize = TSize(cutBarSize.iWidth,
		STATIC_CAST( TInt, rect.iBr.iY*0.0903 ) );

	iCutTimeDisplayRect = TRect( cutTimeDisplayTl, cutTimeDisplaySize );
	iCutTimeDisplay->SetRect( iCutTimeDisplayRect );
	iCutTimeDisplay->SetLayout( CVeiTextDisplay::ECutInCutOut );

		
	TInt iconWidth = STATIC_CAST( TInt, rect.iBr.iX * 0.07954545455 );
	AknIconUtils::SetSize( iPauseBitmap, TSize(iconWidth,iconWidth), EAspectRatioNotPreserved );
        
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::SizeChanged(): Out");
    }

TTypeUid::Ptr CVeiCutVideoContainer::MopSupplyObject( TTypeUid aId )
    {
    if ( aId.iUid == MAknsControlContext::ETypeId && iBgContext )
        {
        return MAknsControlContext::SupplyMopObject( aId, iBgContext );
        }
    return CCoeControl::MopSupplyObject( aId );
    }

TInt CVeiCutVideoContainer::CountComponentControls() const
    {
    return 3; 
    }

CCoeControl* CVeiCutVideoContainer::ComponentControl( TInt aIndex ) const
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

void CVeiCutVideoContainer::Draw( const TRect& aRect ) const
    {
    CWindowGc& gc = SystemGc();
    // draw skin background
    MAknsSkinInstance* skin = AknsUtils::SkinInstance();
    MAknsControlContext* cc = AknsDrawUtils::ControlContext( this );    
    AknsDrawUtils::Background( skin, cc, this, gc, aRect );

    if ( EStatePaused == iState )
        {       
        TPoint pauseIconTl = TPoint( iIconDisplayRect.iTl.iX - STATIC_CAST( TInt, Rect().iBr.iX*0.105),
            iIconDisplayRect.iTl.iY + STATIC_CAST( TInt, Rect().iBr.iY*0.178 ));
        gc.BitBltMasked( pauseIconTl, iPauseBitmap, 
            TRect( TPoint(0,0), iPauseBitmap->SizeInPixels() ), 
            iPauseBitmapMask, EFalse );
        }
    }


// ----------------------------------------------------------------------------
// CVeiCutVideoContainer::GetHelpContext(...) const
//
// Gets the control's help context. Associates the control with a particular
// Help file and topic in a context sensitive application.
// ----------------------------------------------------------------------------
//
void CVeiCutVideoContainer::GetHelpContext( TCoeHelpContext& aContext ) const
    {
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::GetHelpContext(): In");

    // Set UID of the CS Help file (same as application UID).
    aContext.iMajor = KUidVideoEditor;

    // Set the context/topic.
    aContext.iContext = KVED_HLP_CUT_VIDEO_VIEW;

    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::GetHelpContext(): Out");
    }


void CVeiCutVideoContainer::HandleControlEventL(
    CCoeControl* /*aControl*/,TCoeEvent /*aEventType*/)
    {
    }

// ----------------------------------------------------------------------------
// CVeiCutVideoContainer::HandlePointerEventL
// From CCoeControl
// ----------------------------------------------------------------------------
//		
void CVeiCutVideoContainer::HandlePointerEventL( const TPointerEvent& aPointerEvent )
	{
	LOG( KVideoEditorLogFile, "CVeiCutVideoContainer::HandlePointerEventL(): In" );
	if( AknLayoutUtils::PenEnabled() && iCutVideoBar )
		{
		CCoeControl::HandlePointerEventL( aPointerEvent );
		
		switch( aPointerEvent.iType )
			{
			case TPointerEvent::EButton1Down:
				{
				iIsMarkDrag = EFalse;
				iIsMarkTapped = EFalse;
				TRect startMarkRect = iCutVideoBar->StartMarkRect();
				TRect endMarkRect = iCutVideoBar->EndMarkRect();				
				// check if the pen goes down inside the start mark
				if (startMarkRect.Contains(aPointerEvent.iPosition)) 
					{
					iIsMarkTapped = ETrue;
					iTappedMark = EStartMark;
					}
				// check if the pen goes down inside the end mark	
				else if (endMarkRect.Contains(aPointerEvent.iPosition))
					{
					iIsMarkTapped = ETrue;
					iTappedMark = EEndMark;
					}					
				
				TRect progressBarRect(iCutVideoBar->ProgressBarRect());	
				// check if the pen goes down inside the progress bar				
				if( progressBarRect.Contains( aPointerEvent.iPosition ) )
					{
					iIsMarkDrag = EFalse;					
					}
				break;
				}
			case TPointerEvent::EDrag:
				{
				
				TRect progressBarRect(iCutVideoBar->ProgressBarRect());
				if ( progressBarRect.Contains( aPointerEvent.iPosition ) )
				{
					
					if (iIsMarkTapped)
						{
						iIsMarkDrag = ETrue;
						HandleProgressBarTouchL( progressBarRect, 
												 aPointerEvent.iPosition.iX,
												 ETrue,
												 iTappedMark );
						}
					else 
						{
						
						HandleProgressBarTouchL( progressBarRect, 
												 aPointerEvent.iPosition.iX,
												 EFalse);
						}
				}
				break;		
				}
			case TPointerEvent::EButton1Up:
				{
				// pen up event is handled if it wasn't dragged
				if (!iIsMarkDrag)
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
				break;
				}		
			default:
				{
				break;	
				}	
			}
		}	
	LOG( KVideoEditorLogFile, "CVeiCutVideoContainer::HandlePointerEventL(): Out" );		
	}


// ----------------------------------------------------------------------------
// CVeiCutVideoContainer::HandleProgressBarTouchL
// 
// ----------------------------------------------------------------------------
//	
void CVeiCutVideoContainer::HandleProgressBarTouchL( TRect aPBRect, 
												 TInt aPressedPoint,
												 TBool aDragMarks,
												 CVeiCutVideoContainer::TCutMark aCutMark )
	{
	if (( AknLayoutUtils::PenEnabled() ) && ( iState!=EStateInitializing ))
		{	
		LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::HandleProgressBarTouchL(): In");

		if ( iState == EStatePlaying )		
			{
			StopL();	
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
		
		// move cut marks
		if (aDragMarks)
		{
			// check that the start mark doesn't go past the end mark
			// and not to the beginning			
			if ((aCutMark == EStartMark) && 
			    (newPosition.Int64() > 0) &&
				(aPressedPoint < iCutVideoBar->EndMarkPoint() - 2*iCutVideoBar->EndMarkRect().Width()))
				{				
				iView.MoveStartOrEndMarkL(newPosition, EStartMark);				
				iCutVideoBar->SetInPoint( newPosition	);
				iCutTimeDisplay->SetCutIn( newPosition );
				}
			// check that the end mark doesn't go before the start mark	
			// and not too close to the beginning			
			else if ((aCutMark == EEndMark) && 
				(newPosition.Int64() >= KMinCutVideoLength) &&			
				(aPressedPoint > iCutVideoBar->StartMarkPoint() + 2*iCutVideoBar->StartMarkRect().Width()))
                
				{				
				iView.MoveStartOrEndMarkL(newPosition, EEndMark);				
				iCutVideoBar->SetOutPoint( newPosition	);
				iCutTimeDisplay->SetCutOut( newPosition );
				}
		}
				
		// move playhead
		else if (( newPosition != iLastPosition ) && !aDragMarks)
			{
			iLastPosition = newPosition;
			
			iSeekPos = TTimeIntervalMicroSeconds( newPosition );
			
			iCutVideoBar->SetCurrentPoint(( static_cast<TInt32>(iSeekPos.Int64() / 1000) ));
			iVideoDisplay->SetPositionL( iSeekPos );
			GetThumbAtL( iSeekPos );
					
			iView.UpdateTimeL();
			}	
			
		LOG( KVideoEditorLogFile, "CVeiCutVideoContainer::HandleProgressBarTouchL(): Out" );
			
		}// PenEnabled
		
	}




void CVeiCutVideoContainer::PlayL( const TDesC& aFilename )
    {
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::PlayL: in");   
    iVideoDisplay->SetPositionL( iSeekPos );
    
    if (iVideoClipInfo && !iFrameReady)
        {                               
        iVideoClipInfo->CancelFrame();
        }
    iVideoDisplay->PlayL( aFilename );
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::PlayL: out");
    }

void CVeiCutVideoContainer::PlayMarkedL( const TDesC& aFilename,
                                    const TTimeIntervalMicroSeconds& aStartTime, 
                                    const TTimeIntervalMicroSeconds& aEndTime )
    {
    LOGFMT3(KVideoEditorLogFile, "CVeiCutVideoContainer::PlayMarkedL, In, aStartTime:%Ld, aEndTime:%Ld, aFilename:%S", aStartTime.Int64(), aEndTime.Int64(), &aFilename);
    iPlayOrPlayMarked = ETrue;

    if (iVideoClipInfo && !iFrameReady)
        {                               
        iVideoClipInfo->CancelFrame();
        }
    iVideoDisplay->PlayL( aFilename, aStartTime, aEndTime );
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::PlayMarkedL, Out");
    }

void CVeiCutVideoContainer::StopL()
    {
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::StopL, in");
    iVideoDisplay->Stop( EFalse );

    iSeekPos = TTimeIntervalMicroSeconds( 0 );
    
    SetStateL( EStateStopped );
    PlaybackPositionL();
    
    if (iVideoBarTimer)
        {       
        iVideoBarTimer->Cancel();
        }
    
    iCutVideoBar->SetFinishedStatus( ETrue );
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::StopL, in");
    }

void CVeiCutVideoContainer::TakeSnapshotL()
    {
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::TakeSnapshotL in");
    
    if( !iVideoClipInfo || !iFrameReady )
        {
        LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::TakeSnapshotL: 1");
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
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::TakeSnapshotL out");
    }

void CVeiCutVideoContainer::PauseL( TBool aUpdateCBA )
    {
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::PauseL: In");

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

    if (EStateStoppedInitial == iState || EStateStopped == iState ||
        (EStateGettingFrame == iState && 
            (EStateStoppedInitial == iPreviousState || EStateStopped == iPreviousState)))
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
            
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::PauseL: Out");
    }

void CVeiCutVideoContainer::SaveSnapshotL()
    {
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::SaveSnapshotL: In");   
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

    RFs&    fs = iEikonEnv->FsSession();

    TParse file;
    TFileName newname;
    TFileName snapshotdir;

    TVeiSettings saveSettings;
    // Get default snapshot name from settings view

    STATIC_CAST( CVeiAppUi*, iEikonEnv->AppUi() )->ReadSettingsL( saveSettings );   

    CAknMemorySelectionDialog::TMemory memory( saveSettings.MemoryInUse() );

    newname.Append( saveSettings.DefaultSnapshotName() );
    newname.Append( _L(".JPEG") );
    file.Set( newname, NULL, NULL );

    TInt error = KErrNone;

    snapshotdir.Zero();

    if ( memory == CAknMemorySelectionDialog::EPhoneMemory )
        {
        snapshotdir.Append( PathInfo::PhoneMemoryRootPath() ); 
        }
    else
        {
        snapshotdir.Append( PathInfo::MemoryCardRootPath() ); 
        }
    snapshotdir.Append( PathInfo::ImagesPath() );

    error = fs.MkDirAll( file.Path() );
    if ( ( error != KErrAlreadyExists ) && ( error != KErrNone ) )
        {
        return;
        }

    newname.Zero();
    newname.Append( file.NameAndExt() );
    newname.Insert( 0, snapshotdir );
    CApaApplication::GenerateFileName( fs, newname );

    //for cancellation
    if ( iSaveToFileName )
        {
        delete iSaveToFileName;
        iSaveToFileName = NULL;
        }
    iSaveToFileName = HBufC::NewL( newname.Length() );
    *iSaveToFileName = newname;

    // request the actuall save/encode
    // asynchronous, the result is reported via callback NotifyCompletion
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::SaveSnapshotL: 1, calling iConverter->StartToEncodeL");    
    iConverter->StartToEncodeL( newname, 
        imageTypes[selectedIdx]->ImageType(), imageTypes[selectedIdx]->SubType());

    CleanupStack::PopAndDestroy( &imageTypes );
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::SaveSnapshotL: Out");  
    }

void CVeiCutVideoContainer::CancelSnapshotSave()
    {
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::CancelSnapshotSave: in");
    if ( iConverter )
        {
        LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::CancelSnapshotSave: 1");
        iConverter->Cancel();
        iConverter->CancelEncoding(); //also close the file
        }
    if ( iSaveToFileName )
        {
        LOGFMT(KVideoEditorLogFile, "CVeiCutVideoContainer::CancelSnapshotSave: 2, iSaveToFileName:%S", iSaveToFileName);

        RFs&    fs = iEikonEnv->FsSession(); 
        /*TInt result =*/ fs.Delete( *iSaveToFileName ); 
        delete iSaveToFileName;
        iSaveToFileName = NULL;
        }
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::CancelSnapshotSave: out"); 
    }

void CVeiCutVideoContainer::CloseStreamL()
    {
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::CloseStreamL, in");
    if( !iFrameReady && iVideoClipInfo )
        {
        iVideoClipInfo->CancelFrame();
        }
    PlaybackPositionL();
    SetStateL( EStateStopped, EFalse );
    iView.UpdateCBAL(iState);
    
    iVideoDisplay->Stop( ETrue );
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::CloseStreamL, out");
    }

void CVeiCutVideoContainer::SetInTime( const TTimeIntervalMicroSeconds& aTime )
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

void CVeiCutVideoContainer::SetOutTime( const TTimeIntervalMicroSeconds& aTime )
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

TTimeIntervalMicroSeconds CVeiCutVideoContainer::PlaybackPositionL()
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

TKeyResponse CVeiCutVideoContainer::OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType)
    {
    if ( aType == EEventKeyDown ) 
        {
        iKeyRepeatCount = 0;

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
            LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::OfferKeyEventL: 1, calling GetThumbAtL()");
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
                LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::OfferKeyEventL: 2, calling iVideoDisplay->Stop");
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

                if ( (iKeyRepeatCount > 2)  && (iSeeking == EFalse) )
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

                if ( (iKeyRepeatCount > 2)  && (iSeeking == EFalse) )
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
            case EKeyBackspace:     //Clear 
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



TInt CVeiCutVideoContainer::TimeIncrement(TInt aKeyCount) const
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

TInt CVeiCutVideoContainer::UpdateProgressNoteL()
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

    iProgressInfo->IncrementAndDraw( 1 );

    if ( KProgressbarFinalValue <= iProgressInfo->CurrentValue() )
        {
        return 0;
        }
    return 1;
    }


void CVeiCutVideoContainer::GetThumbL( const TDesC& aFilename )
    {
    if ( iVideoClipInfo )
        {
        delete iVideoClipInfo;
        iVideoClipInfo = NULL;      
        }

    /*iVideoClipInfo = */CVedVideoClipInfo::NewL( aFilename, *this );
    }


void CVeiCutVideoContainer::GetThumbAtL( const TTimeIntervalMicroSeconds& aTime )
    {
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::GetThumbAtL: In");
    if( !iVideoClipInfo || ( !iFrameReady ) )
        {
        LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::GetThumbAtL: 1");
        return;
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
        LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::GetThumbAtL: 2");
        TDisplayMode displayMode = ENone;
        TBool enhance = ETrue;
        TSize resol( clipResolution.iBr.iX, clipResolution.iBr.iY ); 
                
        /* :
         check out on every phone before releasing whether videodisplay should be stopped before starting
         asynchronous GetFrameL()
         see how EStateGettingFrame is handled in SetPreviewState 
         Stopping frees memory and it is needed in memory sensible devices 
        */
        iVideoClipInfo->GetFrameL( *this, frameIndex, &resol, displayMode, enhance );
        SetStateL( EStateGettingFrame );
        iFrameReady = EFalse;           
        ShowProgressNoteL();        
        }
    else
        {
        LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::GetThumbAtL: 3");              
        /* :
         check out on every phone before releasing whether videodisplay should be stopped before starting
         asynchronous GetFrameL()
         see how EStateGettingFrame is handled in SetPreviewState 
         Stopping frees memory and it is needed in memory sensible devices 
        */
        iVideoClipInfo->GetFrameL( *this, frameIndex, &resolution );
        SetStateL( EStateGettingFrame );
        iFrameReady = EFalse;       
        }
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::GetThumbAtL: out");    
    }

void CVeiCutVideoContainer::NotifyCompletion( TInt aErr ) 
    {
    LOGFMT(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyCompletion: In, aErr:%d", aErr);  

    if (EStateTerminating == iState)
        {
        LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyCompletion(): app is closing...");
        return;
        }

    if ( KErrNone == aErr )
        {
        LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyCompletion: 1");

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

    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyCompletion: Out");   
    }

void CVeiCutVideoContainer::NotifyVideoClipInfoReady( CVedVideoClipInfo& aInfo, 
                                          TInt aError )
    {
    LOGFMT(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyVideoClipInfoReady, In, aError:%d", aError);
    if (KErrNone == aError)
        {
        if (iVideoClipInfo)     
            {
            delete iVideoClipInfo;
            iVideoClipInfo = NULL;  
            }
        iVideoClipInfo = &aInfo;    

        TRect clipResolution = Rect();
        iDuration = iVideoClipInfo->Duration();
        iCutVideoBar->SetTotalDuration( iDuration );
        iView.DrawTimeNaviL();

        TSize resolution( clipResolution.iBr.iX, clipResolution.iBr.iY-KVeiCutBarHeight ); 
        iFrameReady = EFalse;
        iVideoClipInfo->GetFrameL( *this, 0, &resolution );
        }
    SetStateL( EStateStoppedInitial );          
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyVideoClipInfoReady, Out");
    }


void CVeiCutVideoContainer::NotifyVideoClipFrameCompleted(CVedVideoClipInfo& /*aInfo*/, 
                                               TInt aError, 
                                               CFbsBitmap* aFrame)
    {
    LOGFMT(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyVideoClipFrameCompleted, In, aError:%d", aError);
    iFrameReady = ETrue;
    
    if (EStateGettingFrame == iState)
        {
        SetStateL(iPreviousState);          
        }       
    
    if (KErrNone == aError && aFrame)
        {
        LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyVideoClipFrameCompleted 1");         

        if ( iTakeSnapshot )
            {
            LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyVideoClipFrameCompleted 2");         
            iConverter->SetBitmap( aFrame );
            if (! iCallBackSaveSnapshot)
                {       
                TCallBack cb (CVeiCutVideoContainer::AsyncSaveSnapshot, this);
                iCallBackSaveSnapshot = new (ELeave) CAsyncCallBack(cb, CActive::EPriorityStandard);
                }
            iCallBackSaveSnapshot->CallBack();
            LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyVideoClipFrameCompleted 3");
            }
        else
            {
            LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyVideoClipFrameCompleted 4");
            TRAP_IGNORE(iVideoDisplay->ShowPictureL( *aFrame ));
            delete aFrame;
            aFrame = NULL;
            
            if (iTakeSnapshotWaiting)
                {
                if (! iCallBackTakeSnapshot)
                    {       
                    TCallBack cb (CVeiCutVideoContainer::AsyncTakeSnapshot, this);
                    iCallBackTakeSnapshot = new (ELeave) CAsyncCallBack(cb, CActive::EPriorityStandard);
                    }
                iCallBackTakeSnapshot->CallBack();              
                }
            }
        }
    else
        {
        LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyVideoClipFrameCompleted 5");
        if (aFrame) 
            {
            delete aFrame;
            aFrame = NULL;  
            }       
        
        if (iProgressDialog)
            {
            iProgressInfo->SetAndDraw( KProgressbarFinalValue );
            TRAP_IGNORE(iProgressDialog->ProcessFinishedL());
            }       
        }
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyVideoClipFrameCompleted, Out");  
    }

TInt CVeiCutVideoContainer::AsyncSaveSnapshot(TAny* aThis)
    {
    LOG( KVideoEditorLogFile, "CVeiCutVideoView::AsyncSaveSnapshot in");
    
    // In the asynchronous version, trap the rest of the functions 
    // to make sure that the caller's TRequestStatus is always 
    // completed, also in case of failures.
    CVeiCutVideoContainer* container = static_cast<CVeiCutVideoContainer*>(aThis);
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
    
void CVeiCutVideoContainer::ShowGlobalErrorNote(const TInt aErr)
    {       
    iErrorUI.ShowGlobalErrorNote( aErr );
    }

void CVeiCutVideoContainer::StopProgressDialog()
    {
    if (iProgressDialog)
        {
        TRAP_IGNORE(iProgressDialog->GetProgressInfoL()->SetAndDraw( KProgressbarFinalValue );
                    iProgressDialog->ProcessFinishedL());
        }   
    }

void CVeiCutVideoContainer::NotifyVideoDisplayEvent( const TPlayerEvent aEvent, const TInt& aInfo  )
    {
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyVideoDisplayEvent, In");

    if (EStateTerminating == iState)
        {
        LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyVideoDisplayEvent(): app is closing...");
        return;
        }

    switch (aEvent)
        {
        case MVeiVideoDisplayObserver::ELoadingStarted:
            {   
            SetStateL(EStateOpening);
            LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::ELoadingStarted");
            break;
            }
        case MVeiVideoDisplayObserver::EOpenComplete:
            {
            LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyVideoDisplayEvent, MVeiVideoDisplayObserver::EOpenComplete 1");      
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
                    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyVideoDisplayEvent, MVeiVideoDisplayObserver::EOpenComplete 2");                  
                    iVideoDisplay->PlayMarkedL( cutInTime, cutOutTime );
                    }
                else
                    {
                    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyVideoDisplayEvent, MVeiVideoDisplayObserver::EOpenComplete 3");  
                    iVideoDisplay->Play();
                    }
                iPlayOrPlayMarked = EFalse;             
                }
            else
                {
                LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::EOpenComplete 4");                 
                PauseL();                   
                }   
            break;
            }
        case MVeiVideoDisplayObserver::EBufferingStarted:
            {
            LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::EBufferingStarted");           
            SetStateL( EStateBuffering );
            if ( iVideoBarTimer )
                {
                iVideoBarTimer->Cancel();
                }
            break;
            }   
        case MVeiVideoDisplayObserver::ELoadingComplete:
            {           
            LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyVideoDisplayEvent, MVeiVideoDisplayObserver::ELoadingComplete 1");
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
                    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyVideoDisplayEvent, MVeiVideoDisplayObserver::ELoadingComplete 2");
                    iVideoClipInfo->CancelFrame();
                    }
                if ( !iVideoBarTimer->IsActive() )
                    {
                    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyVideoDisplayEvent, MVeiVideoDisplayObserver::ELoadingComplete 3");               
                    const TUint delay = 100000;
                    iVideoBarTimer->Start( delay, delay, TCallBack( CVeiCutVideoContainer::DoAudioBarUpdate, this ) );
                    }
                iVideoDisplay->ShowBlackScreen();           
                DrawDeferred();
                }
            break;
            }
        case MVeiVideoDisplayObserver::EPlayComplete:
            {
            LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyVideoDisplayEvent, MVeiVideoDisplayObserver::EPlayComplete");
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
            LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::EStop");
            
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
        
            SetStateL( EStateStopped );
            DrawDeferred();
            break;
            }                                                               
        case MVeiVideoDisplayObserver::EVolumeLevelChanged:
            {
            LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyVideoDisplayEvent, MVeiVideoDisplayObserver::EVolumeLevelChanged");
            TInt playerVolume = iVideoDisplay->Volume();
            iView.ShowVolumeLabelL( playerVolume );
            break;
            }
        case MVeiVideoDisplayObserver::EError:
            {
            LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::EError");
//          iErrorUI.ShowGlobalErrorNoteL( KErrGeneral );
            SetStateL( EStateStoppedInitial );
            break;
            }
        default:
            {
            LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyVideoDisplayEvent, MVeiVideoDisplayObserver::default");
            break;
            };
        }
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::NotifyVideoDisplayEvent, Out");
    }


void CVeiCutVideoContainer::SetStateL(CVeiCutVideoContainer::TCutVideoState aState, TBool aUpdateCBA)
    {
    LOGFMT2(KVideoEditorLogFile, "CVeiCutVideoContainer::SetStateL: in, iState:%d, aState:%d", iState, aState);
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

    // While playing, grab the volume keys for adjusting playback volume.
    // In other states let them pass e.g. to the music player.
    if(EStatePlaying == iState)
        {
        if (!iRemConTarget)
            {
            iRemConTarget = CVeiRemConTarget::NewL( *this );
            }
        }
    else
        {
        delete iRemConTarget;
        iRemConTarget = NULL;
        }

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

    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::SetStateL:: out");
    }

void CVeiCutVideoContainer::MarkedInL()
    {
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::MarkedInL, In");
    if ( EStateInitializing == iState )
        {
        return;
        }
    const TTimeIntervalMicroSeconds& position = PlaybackPositionL();
    iSeekPos = position;

    LOGFMT(KVideoEditorLogFile, "CVeiCutVideoContainer::MarkedInL, 2, setting cutINpoint:%Ld", position.Int64());
    iCutVideoBar->SetInPoint( position  );
    iCutTimeDisplay->SetCutIn( position );
    PauseL();
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::MarkedInL, Out");
    }

void CVeiCutVideoContainer::MarkedOutL()
    {
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::MarkedOutL, In");
    if ( EStateInitializing == iState )
        {
        return;
        }
    const TTimeIntervalMicroSeconds& position = PlaybackPositionL();
    iSeekPos = position;

    LOGFMT(KVideoEditorLogFile, "CVeiCutVideoContainer::MarkedOutL, 2, setting cutOUTpoint:%Ld", position.Int64());
    iCutVideoBar->SetOutPoint( position );
    iCutTimeDisplay->SetCutOut( position );

    PauseL();
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::MarkedOutL, Out");
    }

void CVeiCutVideoContainer::ShowInformationNoteL( const TDesC& aMessage ) const
    {
    CAknInformationNote* note = new ( ELeave ) CAknInformationNote( ETrue );
    note->ExecuteLD( aMessage );
    }

void CVeiCutVideoContainer::ShowProgressNoteL() 
    {
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::ShowProgressNoteL, in");
    iProgressDialog = new ( ELeave ) CAknProgressDialog( REINTERPRET_CAST( CEikDialog**, 
                    &iProgressDialog ), ETrue);
    iProgressDialog->SetCallback( this );
    iProgressDialog->PrepareLC( R_VEI_PROGRESS_NOTE_WITH_CANCEL );

    HBufC* stringholder  = StringLoader::LoadL( R_VEI_PROGRESS_NOTE_SAVING_IMAGE, iEikonEnv );
    CleanupStack::PushL( stringholder );

    iProgressDialog->SetTextL( *stringholder );
    CleanupStack::PopAndDestroy( stringholder );

    iProgressInfo = iProgressDialog->GetProgressInfoL();
    iProgressInfo->SetFinalValue( KProgressbarFinalValue );
    iProgressDialog->RunLD();

    iProgressInfo->SetAndDraw( 50 );
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::ShowProgressNoteL, Out");
    }

TInt CVeiCutVideoContainer::DoAudioBarUpdate( TAny* aThis )
    {
    STATIC_CAST( CVeiCutVideoContainer*, aThis )->DoUpdate();
    return 42;
    }

void CVeiCutVideoContainer::DoUpdate()
    {
    TTimeIntervalMicroSeconds time;
    time = iVideoDisplay->PositionL();
    if ( iSeeking )
        {
        time = iSeekPos;
        LOGFMT(KVideoEditorLogFile, "CVeiCutVideoContainer::DoUpdate(): 1, time:%Ld", time.Int64());
        }
    else
        {           
        LOGFMT(KVideoEditorLogFile, "CVeiCutVideoContainer::DoUpdate(): 2, time:%Ld", time.Int64());
        }
    iCutVideoBar->SetCurrentPoint( static_cast<TInt32>((time.Int64() / 1000)));
    iCutVideoBar->DrawDeferred();
    }
void CVeiCutVideoContainer::MuteL()
    {
    iVideoDisplay->SetMuteL( ETrue );
    }

//=============================================================================
void CVeiCutVideoContainer::HandleVolumeUpL()
    {
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::HandleVolumeUpL: in");

    iVideoDisplay->AdjustVolumeL( 1 );

    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::HandleVolumeUpL: out");
    }

//=============================================================================
void CVeiCutVideoContainer::HandleVolumeDownL()
    {
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::HandleVolumeDownL: in");

    iVideoDisplay->AdjustVolumeL( -1 );

    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::HandleVolumeDownL: out");
    }

//=============================================================================
void CVeiCutVideoContainer::PrepareForTerminationL()
    {
    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::PrepareForTerminationL: in");

    SetStateL( EStateTerminating );

    if( !iFrameReady && iVideoClipInfo )
        {
        iVideoClipInfo->CancelFrame();
        }
    iState = EStateTerminating;
    iVideoDisplay->Stop( ETrue );

    LOG(KVideoEditorLogFile, "CVeiCutVideoContainer::PrepareForTerminationL: out");
    }


TInt CVeiCutVideoContainer::AsyncTakeSnapshot(TAny* aThis)
    {
    LOG( KVideoEditorLogFile, "CVeiCutVideoContainer::AsyncTakeSnapshot");
    
    // In the asynchronous version, trap the rest of the functions 
    // to make sure that the caller's TRequestStatus is always 
    // completed, also in case of failures.
    CVeiCutVideoContainer* container = static_cast<CVeiCutVideoContainer*>(aThis);
    TInt err = KErrNone;
    TRAP(err, container->TakeSnapshotL());
    LOGFMT( KVideoEditorLogFile, "CVeiCutVideoContainer::AsyncTakeSnapshot 1, err:%d", err);    
    User::LeaveIfError(err);        
    return KErrNone;                
    }
// End of File  
