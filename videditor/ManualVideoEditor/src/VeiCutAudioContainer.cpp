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
#include <coemain.h>
#include <eikenv.h>
#include <aknsbasicbackgroundcontrolcontext.h> 
#include <aknsdrawutils.h> 
#include <aknsdatacontext.h> 
#include <stringloader.h>
#include <aknnotewrappers.h> 
#include <MdaAudioSamplePlayer.h>
#include <eikprogi.h>
#include <audiopreference.h>
#include <aknlayoutscalable_avkon.cdl.h>
#include <aknlayoutscalable_apps.cdl.h>
// User includes
#include "manualvideoeditor.hrh"
#include "VeiAppUi.h"
#include "VeiCutAudioContainer.h"
#include "VeiCutAudioView.h"
#include "VeiCutterBar.h"
#include "veitextdisplay.h"
#include "VideoEditorCommon.h"      // Video Editor UID
#include "VideoEditorHelp.hlp.hrh"  // Topic contexts (literals)
#include "VeiVideoEditorSettings.h"
#include "VideoEditorUtils.h"
#include "SampleArrayHandler.h"
#include "VeiErrorUi.h"


// ================= MEMBER FUNCTIONS =======================
CVeiCutAudioContainer* CVeiCutAudioContainer::NewL( const TRect& aRect, CVeiCutAudioView& aView, CVeiErrorUI& aErrorUI )
    {
    CVeiCutAudioContainer* self = CVeiCutAudioContainer::NewLC( aRect, aView, aErrorUI );
    CleanupStack::Pop( self );
    return self;
    }

CVeiCutAudioContainer* CVeiCutAudioContainer::NewLC( const TRect& aRect, CVeiCutAudioView& aView, CVeiErrorUI& aErrorUI )
    {
    CVeiCutAudioContainer* self = new (ELeave) CVeiCutAudioContainer( aRect, aView, aErrorUI );
    CleanupStack::PushL( self );
    self->ConstructL( aRect, aView, aErrorUI );
    return self;
    }

void CVeiCutAudioContainer::ConstructL( const TRect& aRect, CVeiCutAudioView& /*aView*/, CVeiErrorUI& /*aErrorUI*/ )
    {
    CreateWindowL();

    iState = EStateInitializing;
    iFrameReady = EFalse;

    iSeekPos = TTimeIntervalMicroSeconds( 0 );
    iSeeking = EFalse;
    iCutAudioBar = CVeiCutterBar::NewL( this );
    iCutTimeDisplay = CVeiTextDisplay::NewL( iCutTimeDisplayRect, this );

    iPreviousScreenMode = -1;
    iCurrentScreenMode = -1;

    TFileName mbmPath( VideoEditorUtils::IconFileNameAndPath(KVideoEditorUiComponentsIconFileId) );

    AknIconUtils::CreateIconL( iPauseBitmap, iPauseBitmapMask,
        mbmPath, EMbmVideoeditoruicomponentsQgn_prop_ve_pause, 
        EMbmVideoeditoruicomponentsQgn_prop_ve_pause_mask );

    SetRect( aRect );

    iBgContext = CAknsBasicBackgroundControlContext::NewL( KAknsIIDQsnBgAreaMain, Rect(), EFalse );

    iAudioSamplePlayer = CMdaAudioPlayerUtility::NewL( *this,EMdaPriorityMax, 
        TMdaPriorityPreference( KAudioPrefVideoRecording ) );

    iVideoBarTimer = CPeriodic::NewL( CActive::EPriorityLow );

    iSampleArrayHandler = CSampleArrayHandler::NewL();

    EnableDragEvents();

    ActivateL();
    }

CVeiCutAudioContainer::CVeiCutAudioContainer( const TRect& /*aRect*/, CVeiCutAudioView& aView, CVeiErrorUI& aErrorUI ):iView( aView ), iErrorUI( aErrorUI )
    {
    }       

void CVeiCutAudioContainer::OpenAudioFileL( const TDesC& aFileName )
    {
    if ( iAudioSamplePlayer )
        {
        iAudioSamplePlayer->OpenFileL( aFileName );
        }
    GetVisualizationL();
    }

CVeiCutAudioContainer::~CVeiCutAudioContainer()
    {
    if ( iProgressNote )
        {
        delete iProgressNote;
        iProgressNote = NULL;
        }
    if ( iCutAudioBar )
        {
        delete iCutAudioBar;
        }
    if ( iCutTimeDisplay )
        {
        delete iCutTimeDisplay;
        }
    if ( iBgContext )
        {
        delete iBgContext;
        }
    delete iPauseBitmap;
    delete iPauseBitmapMask;

    if ( iVideoBarTimer )
        {
        iVideoBarTimer->Cancel();
        delete iVideoBarTimer;
        }
    if ( iAudioSamplePlayer )
        {
        delete iAudioSamplePlayer;
        }
    if (iSampleArrayHandler)
        {
        delete iSampleArrayHandler;
        }
        
    delete iCallBack;

    delete iBufBitmap;
    iBufBitmap = NULL;

    delete iRemConTarget;
    }

void CVeiCutAudioContainer::SizeChanged()
    {
    TSize videoScreenSize;
    TRect rect( Rect() ); 
    if ( iBgContext )
        {
        iBgContext->SetRect( rect );
        }

    iPreviousScreenMode = iCurrentScreenMode;
    iCurrentScreenMode = iEikonEnv->ScreenDevice()->CurrentScreenMode();


	// Scissor icon
	TAknLayoutRect scissorsIconLayout;
	scissorsIconLayout.LayoutRect(Rect(),AknLayoutScalable_Apps::main_vded_pane_g1());
	iCutAudioBar->SetComponentRect(CVeiCutterBar::EScissorsIcon, scissorsIconLayout.Rect());
		
	// Progress bar
	TAknLayoutRect progressBarLayout; 
	progressBarLayout.LayoutRect(Rect(),AknLayoutScalable_Apps::vded_slider_pane());
	iCutAudioBar->SetComponentRect(CVeiCutterBar::EProgressBar, progressBarLayout.Rect());

	// left end of the slider when that part is unselected
	TAknLayoutRect sliderLeftEndLayout;
	sliderLeftEndLayout.LayoutRect( progressBarLayout.Rect(),AknLayoutScalable_Apps::vded_slider_pane_g3() );
	iCutAudioBar->SetComponentRect( CVeiCutterBar::ESliderLeftEndIcon, sliderLeftEndLayout.Rect() );
		
	// middle part of the slider when that part is unselected	
	TAknLayoutRect sliderMiddleLayout;
	sliderMiddleLayout.LayoutRect(progressBarLayout.Rect(),AknLayoutScalable_Apps::vded_slider_pane_g5());
	iCutAudioBar->SetComponentRect( CVeiCutterBar::ESliderMiddleIcon, sliderMiddleLayout.Rect() );		
	
	// right end of the slider when that part is unselected
	TAknLayoutRect sliderRightEndLayout;
	sliderRightEndLayout.LayoutRect(progressBarLayout.Rect(),AknLayoutScalable_Apps::vded_slider_pane_g4());
	iCutAudioBar->SetComponentRect( CVeiCutterBar::ESliderRightEndIcon, sliderRightEndLayout.Rect() );	
		
	// left end of the cut selection slider 
	TAknLayoutRect sliderSelectedLeftEndLayout;
	sliderSelectedLeftEndLayout.LayoutRect(progressBarLayout.Rect(),AknLayoutScalable_Apps::vded_slider_pane_g3());
	iCutAudioBar->SetComponentRect( CVeiCutterBar::ESliderSelectedLeftEndIcon, sliderSelectedLeftEndLayout.Rect() );
		
	// middle part of the cut selection slider 
	TAknLayoutRect sliderSelectedMiddleLayout;
	sliderSelectedMiddleLayout.LayoutRect(progressBarLayout.Rect(),AknLayoutScalable_Apps::vded_slider_pane_g5());
	iCutAudioBar->SetComponentRect( CVeiCutterBar::ESliderSelectedMiddleIcon, sliderSelectedMiddleLayout.Rect() );		
	
	// right end of the cut selection slider 
	TAknLayoutRect sliderSelectedRightEndLayout;
	sliderSelectedRightEndLayout.LayoutRect(progressBarLayout.Rect(),AknLayoutScalable_Apps::vded_slider_pane_g4());
	iCutAudioBar->SetComponentRect( CVeiCutterBar::ESliderSelectedRightEndIcon, sliderSelectedRightEndLayout.Rect() ); 

    // playhead
    TAknLayoutRect playheadLayout;
	playheadLayout.LayoutRect(progressBarLayout.Rect(),AknLayoutScalable_Apps::vded_slider_pane_g1());
	iCutAudioBar->SetComponentRect( CVeiCutterBar::EPlayheadIcon, playheadLayout.Rect() ); 

    // left/right border of cut selection slider
    TAknLayoutRect cutAreaBorderLayout;
	cutAreaBorderLayout.LayoutRect(progressBarLayout.Rect(),AknLayoutScalable_Apps::vded_slider_pane_g2());
	iCutAudioBar->SetComponentRect( CVeiCutterBar::ECutAreaBorderIcon, cutAreaBorderLayout.Rect() ); 
		
		
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

	iIconDisplayRect = videoDisplayLayout.Rect();
	LOGFMT2(KVideoEditorLogFile, "CVeiCutAudioContainer::SizeChanged(): iIconDisplayRect: %d,%d", iIconDisplayRect.iBr.iX, iIconDisplayRect.iBr.iY);

	//CVeiCutterBar
	// : Change this when LAF data is ready
	TPoint cutBarTl = TPoint(STATIC_CAST( TInt, rect.iBr.iX*0.0114 ),
		STATIC_CAST( TInt, rect.iBr.iY*0.875 ) );
	TSize cutBarSize = TSize(STATIC_CAST( TInt, rect.iBr.iX*0.9773 ),
		STATIC_CAST( TInt, rect.iBr.iY*0.0973 ) );

	TRect cutBarRect( cutBarTl, cutBarSize );
	iCutAudioBar->SetRect( cutBarRect );

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

    //  Create buffer bitmap with the correct size
    if (iBufBitmap)
        {
        delete iBufBitmap;
        iBufBitmap = NULL;
        }
    TSize size = Rect().Size();
    TDisplayMode dmode = EColor64K;
    // " ... If the instantiation process really needs 
    // not to leave, use "new CXxx" and check for NULL."
    iBufBitmap = new CFbsBitmap;
    TInt err = KErrNone;
    if (iBufBitmap)
        {
        err = iBufBitmap->Create(size, dmode);
        }

    if (iState != EStateInitializing && !err)
        {
        TRAP_IGNORE( DrawToBufBitmapL() );
        }
    }


TTypeUid::Ptr CVeiCutAudioContainer::MopSupplyObject( TTypeUid aId )
    {
    if ( aId.iUid == MAknsControlContext::ETypeId && iBgContext )
        {
        return MAknsControlContext::SupplyMopObject( aId, iBgContext );
        }
    return CCoeControl::MopSupplyObject( aId );
    }

TInt CVeiCutAudioContainer::CountComponentControls() const
    {
    return 2;
    }

CCoeControl* CVeiCutAudioContainer::ComponentControl( TInt aIndex ) const
    {
    switch ( aIndex )
        {
        case 0:
            return iCutAudioBar;
        case 1:
            return iCutTimeDisplay;
        default:
            return NULL;
        }
    }

void CVeiCutAudioContainer::DrawToBufBitmapL()
    {
    //  Create bitmap graphics context
    CFbsBitmapDevice * bitmapDevice = CFbsBitmapDevice::NewL (iBufBitmap);
    CleanupStack::PushL (bitmapDevice);
    CFbsBitGc * bitmapContext = 0;
    User::LeaveIfError (bitmapDevice->CreateContext (bitmapContext));
    CleanupStack::PushL (bitmapContext);
    CGraphicsContext * graphicsContext = 0;
    User::LeaveIfError (bitmapDevice->CreateContext (graphicsContext));
    CleanupStack::PushL (graphicsContext);

    // Draw skin background
    MAknsSkinInstance* skin = AknsUtils::SkinInstance();
    MAknsControlContext* cc = AknsDrawUtils::ControlContext( this );    
    AknsDrawUtils::DrawBackground( skin, cc, this, *((CBitmapContext*)graphicsContext), TPoint(0,0), Rect(), KAknsDrawParamDefault );

    if ( iState == EStatePaused )
        {
        TPoint pauseIconTl = TPoint( iIconDisplayRect.iTl.iX - STATIC_CAST( TInt, Rect().iBr.iX*0.105),
            iIconDisplayRect.iTl.iY + STATIC_CAST( TInt, Rect().iBr.iY*0.178 ));
        bitmapContext->BitBltMasked( pauseIconTl, iPauseBitmap, 
            TRect( TPoint(0,0), iPauseBitmap->SizeInPixels() ), 
            iPauseBitmapMask, EFalse );
        }

    if (iSampleArrayHandler)
        {
        bitmapContext->SetPenSize( TSize(2,1) );    
        bitmapContext->SetBrushStyle(CGraphicsContext::ESolidBrush);
        bitmapContext->SetBrushColor(KRgbWhite);
        bitmapContext->SetPenColor( KRgbBlack); 
        bitmapContext->DrawRoundRect( iIconDisplayRect, TSize(4,4));

        iSampleArrayHandler->ScaleAudioVisualization(iIconDisplayRect.Height()/2 - 3); // -2 to eliminate drawing columns to long

        // how many pixels are reserved for each vertical sample line
        TInt diff = 1;          
        // how many samples fit in rect
        TInt samplesInDisplay = iIconDisplayRect.Width()/diff;  

        TBool started = EFalse;
        TPoint topLeftHighlighted(iIconDisplayRect.iTl.iX, iIconDisplayRect.iTl.iY);    
        TPoint bottomRightLighted(iIconDisplayRect.iTl.iX, iIconDisplayRect.iBr.iY);

        for (TInt i = 0; (iSampleArrayHandler->CurrentPoint()+i) - samplesInDisplay/2 < iSampleArrayHandler->Size() && 
            iIconDisplayRect.iTl.iX + i*diff < iIconDisplayRect.iBr.iX;  i++)                               
            {
            TInt x = iIconDisplayRect.iTl.iX + i*diff;  
            TInt y = iIconDisplayRect.iBr.iY - iIconDisplayRect.Height()/2;

            // in the beginning of clip, nothing is drawn on the left side of the rect                      
            if ((iSampleArrayHandler->CurrentPoint() + i) - samplesInDisplay/2 > 1)
                {
                if (iSampleArrayHandler->SampleCutted((iSampleArrayHandler->CurrentPoint()+i) - samplesInDisplay/2))
                    {
                    if (!started)
                        {                       
                        topLeftHighlighted.iX = x;
                        started = ETrue;
                        }
                    bottomRightLighted.iX = x;
                    bitmapContext->SetPenColor( KRgbRed );
                    }
                else
                    {
                    bitmapContext->SetBrushStyle(CGraphicsContext::EDiamondCrossHatchBrush);    
                    TRect rec(TPoint(1,1), TPoint(2,2));
                    bitmapContext->DrawRect( rec);
                    bitmapContext->SetPenColor( KRgbBlack); 
                    }

                // to eliminate from drawing over surrounding black rect
                if (x > iIconDisplayRect.iTl.iX + 1)
                    {                   
                    bitmapContext->DrawLine(TPoint(x, y),
                        TPoint(x, (y - 1) - iSampleArrayHandler->Sample((iSampleArrayHandler->CurrentPoint()+i) - samplesInDisplay/2)));

                    bitmapContext->DrawLine(TPoint(x, y),
                        TPoint(x, (y + 1) + iSampleArrayHandler->Sample((iSampleArrayHandler->CurrentPoint()+i) - samplesInDisplay/2)));
                    }
                }
            }

        // not selected area is "dimmed"
        bitmapContext->SetPenColor( KRgbBlack); 
        bitmapContext->SetDrawMode(CGraphicsContext::EDrawModeAND);
        bitmapContext->SetBrushStyle(CGraphicsContext::EDiamondCrossHatchBrush);

        TPoint brArea1(topLeftHighlighted.iX, iIconDisplayRect.iBr.iY);
        TRect rect1(iIconDisplayRect.iTl, brArea1);
        bitmapContext->DrawRoundRect( rect1, TSize(4,4));

        TPoint tlArea2(bottomRightLighted.iX, iIconDisplayRect.iTl.iY);
        TRect rect2(tlArea2, iIconDisplayRect.iBr);
        bitmapContext->DrawRoundRect( rect2, TSize(4,4));

        bitmapContext->SetPenColor( KRgbGreen );    
        bitmapContext->DrawLine(TPoint((iIconDisplayRect.iTl.iX + iIconDisplayRect.iBr.iX)/2, 
            iIconDisplayRect.iTl.iY + 1),
            TPoint((iIconDisplayRect.iTl.iX + iIconDisplayRect.iBr.iX)/2, 
            iIconDisplayRect.iBr.iY - 2));
        }

    CleanupStack::PopAndDestroy( graphicsContext ); 
    CleanupStack::PopAndDestroy( bitmapContext );
    CleanupStack::PopAndDestroy( bitmapDevice );         
    DrawDeferred();
    }

void CVeiCutAudioContainer::Draw( const TRect& /*aRect*/ ) const
    {
    CWindowGc& gc = SystemGc();

    if ( iBufBitmap && iBufBitmap->Handle() )
        {
        gc.BitBlt(TPoint(0,0), iBufBitmap);
        }
    }

// ----------------------------------------------------------------------------
// CVeiCutAudioContainer::GetHelpContext(...) const
//
// Gets the control's help context. Associates the control with a particular
// Help file and topic in a context sensitive application.
// ----------------------------------------------------------------------------
//
void CVeiCutAudioContainer::GetHelpContext( TCoeHelpContext& aContext ) const
    {
    LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::GetHelpContext(): In");

    // Set UID of the CS Help file (same as application UID).
    aContext.iMajor = KUidVideoEditor;

    // Set the context/topic.
    aContext.iContext = KVED_HLP_CUT_AUDIO_VIEW;

    LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::GetHelpContext(): Out");
    }


void CVeiCutAudioContainer::HandleControlEventL(
    CCoeControl* /*aControl*/,TCoeEvent /*aEventType*/)
    {
    }

// ----------------------------------------------------------------------------
// CVeiCutAudioContainer::HandlePointerEventL
// From CCoeControl
// ----------------------------------------------------------------------------
//		
void CVeiCutAudioContainer::HandlePointerEventL(const TPointerEvent& aPointerEvent )
	{
	LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::HandlePointerEventL(): In");
	if( AknLayoutUtils::PenEnabled() && iCutAudioBar )
		{
		CCoeControl::HandlePointerEventL( aPointerEvent );
		
		switch( aPointerEvent.iType )
			{
			case TPointerEvent::EButton1Down:
				{
				iIsMarkDrag = EFalse;
				iIsMarkTapped = EFalse;
				TRect startMarkRect = iCutAudioBar->StartMarkRect();
				TRect endMarkRect = iCutAudioBar->EndMarkRect();				
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
				
				TRect progressBarRect(iCutAudioBar->ProgressBarRect());	
				// check if the pen goes down inside the progress bar				
				if( progressBarRect.Contains( aPointerEvent.iPosition ) )
					{
					iIsMarkDrag = EFalse;					
					}
				break;
				}
			case TPointerEvent::EDrag:
				{
				
				TRect progressBarRect(iCutAudioBar->ProgressBarRect());
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
					TRect progressBarRect(iCutAudioBar->ProgressBarRect());					
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
	LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::HandlePointerEventL(): Out");		
	}


// ----------------------------------------------------------------------------
// CVeiCutAudioContainer::HandleProgressBarTouchL
// 
// ----------------------------------------------------------------------------
//	
void CVeiCutAudioContainer::HandleProgressBarTouchL( TRect aPBRect, 
												 TInt aPressedPoint,
												 TBool aDragMarks,
												 CVeiCutAudioContainer::TCutMark aCutMark )
	{
	if ( (AknLayoutUtils::PenEnabled()) && ( iState!=EStateInitializing ))
		{	
		LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::HandleProgressBarTouchL(): In");

		if (iState == EStatePlaying)		
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
							  			iAudioSamplePlayer->Duration().Int64() ) / 
							  			totalPBLength );
		
		// move cut marks
		if (aDragMarks)
		{
			// check that the start mark doesn't go past the end mark
			// and not to the beginning			
			if ((aCutMark == EStartMark) && 
			    (newPosition.Int64() > 0) &&
				(aPressedPoint < iCutAudioBar->EndMarkPoint() - 2*iCutAudioBar->EndMarkRect().Width()))
				{				
				iView.MoveStartOrEndMarkL(newPosition, EStartMark);				
				iCutAudioBar->SetInPoint( newPosition	);
				iCutTimeDisplay->SetCutIn( newPosition );
				}
			// check that the end mark doesn't go before the start mark	
			// and not too close to the beginning			
			else if ((aCutMark == EEndMark) && 
				(newPosition.Int64() >= KMinCutVideoLength) &&			
				(aPressedPoint > iCutAudioBar->StartMarkPoint() + 2*iCutAudioBar->StartMarkRect().Width()))
                
				{				
				iView.MoveStartOrEndMarkL(newPosition, EEndMark);				
				iCutAudioBar->SetOutPoint( newPosition	);
				iCutTimeDisplay->SetCutOut( newPosition );
				}
		}
				
		// move playhead
		else if (( newPosition != iLastPosition ) && !aDragMarks)
			{
			iLastPosition = newPosition;
			
			iSeekPos = TTimeIntervalMicroSeconds( newPosition );
			
			iCutAudioBar->SetCurrentPoint( (static_cast<TInt32>(iSeekPos.Int64() / 1000)));
			iAudioSamplePlayer->SetPosition( iSeekPos );
			UpdateVisualizationL();
					
			iView.UpdateTimeL();
			}	
			
		LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::HandleProgressBarTouchL(): Out");
			
		}// PenEnabled
		
	}


void CVeiCutAudioContainer::PlayL( const TTimeIntervalMicroSeconds& aStartTime )
    {
    LOGFMT(KVideoEditorLogFile, "CVeiCutAudioContainer::PlayL(): In, aStartTime:%Ld", aStartTime.Int64());
    if ( aStartTime != TTimeIntervalMicroSeconds(0) )
        {
        LOGFMT(KVideoEditorLogFile, "CVeiCutAudioContainer::PlayL() 2, setting pos:%Ld", aStartTime.Int64());
        iAudioSamplePlayer->SetPosition( aStartTime );
        UpdateVisualizationL();
        }
    else
        {
        LOGFMT(KVideoEditorLogFile, "CVeiCutAudioContainer::PlayL() 3, setting pos:%Ld", iSeekPos.Int64());     
        iAudioSamplePlayer->SetPosition( iSeekPos );
        UpdateVisualizationL();
        }   

    SetStateL( EStatePlaying );

    iAudioSamplePlayer->Play();
    const TUint delay = 100000;

    if ( !iVideoBarTimer->IsActive() )
        {
        LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::PlayL() 4");
        iVideoBarTimer->Start( delay, delay, TCallBack( CVeiCutAudioContainer::DoAudioBarUpdate, this ) );
        LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::PlayL() 5");
        }
    LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::PlayL(): Out");    
    }

void CVeiCutAudioContainer::StopL()
    {
    LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::StopL(): In");
    iAudioSamplePlayer->Stop();

    iSeekPos = TTimeIntervalMicroSeconds( 0 );
    iLastPosition = TTimeIntervalMicroSeconds( 0 );
    SetStateL( EStateStopped );
    PlaybackPositionL();

    iVideoBarTimer->Cancel();
    iCutAudioBar->SetFinishedStatus( ETrue );

    UpdateVisualizationL();

    LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::StopL(): Out");
    }

void CVeiCutAudioContainer::PauseL()
    {
    LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::PauseL(): In");

    PlaybackPositionL();

    iVideoBarTimer->Cancel();
    iAudioSamplePlayer->Pause();

    if (iState != EStateInitializing) 
        {
        SetStateL( EStatePaused );
        }
    else
        {
        SetStateL( EStateStoppedInitial );
        }
    iView.UpdateCBAL(iState);
    // draw new visualization to bitmap
    DrawToBufBitmapL();

    LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::PauseL(): Out");
    }

void CVeiCutAudioContainer::CloseStreamL()
    {
    LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::CloseStreamL(): In");

    PlaybackPositionL();
    SetStateL( EStateStopped );

    iAudioSamplePlayer->Stop();
    iAudioSamplePlayer->Close();

    LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::CloseStreamL(): Out");
    }

void CVeiCutAudioContainer::SetInTimeL( const TTimeIntervalMicroSeconds& aTime )
    {
    iMarkedInTime = aTime;
    if ( iCutTimeDisplay )
        {       
        iCutTimeDisplay->SetCutIn( aTime );
        }
    if ( iCutAudioBar )
        {
        iCutAudioBar->SetInPoint( aTime );
        }
    if (iSampleArrayHandler)
        {
        iSampleArrayHandler->SetCutInPoint(aTime);          
        }
    // draw new visualization to bitmap
    DrawToBufBitmapL();
    }

void CVeiCutAudioContainer::SetOutTimeL( const TTimeIntervalMicroSeconds& aTime )
    {
    iMarkedOutTime = aTime;
    if ( iCutTimeDisplay )
        {
        iCutTimeDisplay->SetCutOut( aTime );
        }
    if ( iCutAudioBar )
        {
        iCutAudioBar->SetOutPoint( aTime );
        }
    if (iSampleArrayHandler)
        {
        iSampleArrayHandler->SetCutOutPoint(aTime);         
        }
    // draw new visualization to bitmap
    DrawToBufBitmapL();
    }

const TTimeIntervalMicroSeconds& CVeiCutAudioContainer::TotalLength()
    {
    return iDuration;
    }

const TTimeIntervalMicroSeconds& CVeiCutAudioContainer::PlaybackPositionL()
    {
    if (iSeeking)
        {
        return iSeekPos;
        }
    if (iState != EStatePlaying)
        {
        return iLastPosition;
        }

    TInt posError = iAudioSamplePlayer->GetPosition( iLastPosition );
    //LOGFMT2(KVideoEditorLogFile, "CVeiCutAudioContainer::PlaybackPositionL(): %Ld, error: %d", iLastPosition.Int64(), posError);
    posError = 0;

    return iLastPosition;
    }

TKeyResponse CVeiCutAudioContainer::OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType)
    {
    if ( aType == EEventKeyDown ) 
        {
        iKeyRepeatCount = 0;

        iAudioSamplePlayer->GetPosition( iSeekPos );

        LOGFMT(KVideoEditorLogFile, "CVeiCutAudioContainer::OfferKeyEventL(): EEventKeyDown, pos: %Ld", iSeekPos.Int64());

        return EKeyWasConsumed;
        }
    else if ( aType == EEventKeyUp ) 
        {

        if ( iSeeking == EFalse )
            {
            LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::OfferKeyEventL(): EEventKeyUp, seeking false");
            return EKeyWasNotConsumed;
            }

        iSeeking = EFalse;
        iLastPosition = iSeekPos;
        iAudioSamplePlayer->SetPosition( iSeekPos );
        UpdateVisualizationL();
        if ( iState == EStatePlaying )
            {
            iAudioSamplePlayer->Play();
            }

        LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::OfferKeyEventL(): EEventKeyUp, seeking true");

        return EKeyWasConsumed;
        }
    else if ( aType == EEventKey )
        {
        if ( ( aKeyEvent.iRepeats == 0 ) &&
            ( (aKeyEvent.iCode != EKeyDownArrow ) &&
            ( aKeyEvent.iCode != EKeyUpArrow ) ) )
            {
            iAudioSamplePlayer->Pause();
            }
        iLastKeyCode = aKeyEvent.iCode;

        switch (aKeyEvent.iCode)
            {
            case EKeyOK:
                {
                iView.HandleCommandL( EAknSoftkeyOk );
                return EKeyWasConsumed;
                }
            case EKeyDownArrow:
                {
                iView.ProcessCommandL( EVeiCmdCutVideoVolumeDown );
                return EKeyWasConsumed;
                }
            case EKeyUpArrow:
                {
                iView.ProcessCommandL( EVeiCmdCutVideoVolumeUp );
                return EKeyWasConsumed;
                }
            case EKeyRightArrow:
                {

                if ( iSeeking == EFalse )
                    {
                    iAudioSamplePlayer->Pause();
                    }

                iSeeking = ETrue;
                iKeyRepeatCount++;

                TInt adjustment = TimeIncrement( iKeyRepeatCount );

                TInt64 newPos = iSeekPos.Int64() + adjustment;

                if ( newPos > iAudioSamplePlayer->Duration().Int64() )
                    {
                    newPos = iAudioSamplePlayer->Duration().Int64();
                    }
                iSeekPos = TTimeIntervalMicroSeconds( newPos );
                iCutAudioBar->SetCurrentPoint( static_cast<TInt32>((iSeekPos.Int64() / 1000)));

                iView.UpdateTimeL();

                // mieti onko eka rivi tarpeen
                iAudioSamplePlayer->SetPosition( iSeekPos );
                UpdateVisualizationL();
                return EKeyWasConsumed;
                }

            case EKeyLeftArrow:
                {
                if ( iSeeking == EFalse )
                    {
                    iAudioSamplePlayer->Pause();
                    }

                iSeeking = ETrue;
                iKeyRepeatCount++;

                TInt adjustment = TimeIncrement( iKeyRepeatCount );

                TInt64 newPos = iSeekPos.Int64() - adjustment;
                if ( newPos < 0 ) 
                    {
                    newPos = 0;
                    }
                iSeekPos = TTimeIntervalMicroSeconds( newPos ); 
                iCutAudioBar->SetCurrentPoint(static_cast<TInt32>( (iSeekPos.Int64() / 1000)));

                iView.UpdateTimeL();                
                iAudioSamplePlayer->SetPosition( iSeekPos );
                UpdateVisualizationL();
                return EKeyWasConsumed;
                }
            case EKeyBackspace:     //Clear 
                {
                iView.ClearInOutL( ETrue, ETrue );
                UpdateVisualizationL();             
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



TInt CVeiCutAudioContainer::TimeIncrement(TInt aKeyCount) const
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

void CVeiCutAudioContainer::SetStateL(CVeiCutAudioContainer::TCutAudioState aState)
    {
    LOGFMT(KVideoEditorLogFile, "CVeiCutAudioContainer::SetStateL(): In: aState:%d", aState);

    iState = aState;
    iView.UpdateCBAL( aState );

    // If the foreground is lost while an arrow key is down, we do not get
    // the key up -event, and iSeeking remains true. Reseting it here just in case.
    iSeeking = EFalse;

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

    LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::SetStateL(): Out");
    }

void CVeiCutAudioContainer::MarkedInL()
    {
    LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::MarkedInL(): In");             

    if (iState == EStateInitializing || iState == EStateOpening)
        {
        return;
        }

    TTimeIntervalMicroSeconds cutIn = PlaybackPositionL();
    if ( iCutTimeDisplay )
        {
        iCutTimeDisplay->SetCutIn( cutIn );
        }
    LOGFMT2(KVideoEditorLogFile, "\tIn point: %Ld, state: %d", cutIn.Int64(), iState);

    iCutAudioBar->SetInPoint( cutIn );
    
    if (iSampleArrayHandler)
        {
        iSampleArrayHandler->SetCutInPoint(cutIn);          
        }
        
    if ( iState == EStatePlaying )
        {
        PauseL();
        }
    else
        {
        iView.UpdateCBAL( iState );
        }
    
    UpdateVisualizationL();     
    LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::MarkedInL(): Out");    
    }

void CVeiCutAudioContainer::MarkedOutL()
    {
    LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::MarkedOutL(): In");
    
    if (iState == EStateInitializing || iState == EStateOpening)
        {
        return;
        }
    
    TTimeIntervalMicroSeconds cutOut = PlaybackPositionL();
    
    if ( iCutTimeDisplay )
        {
        iCutTimeDisplay->SetCutOut( cutOut );
        }

    LOGFMT2(KVideoEditorLogFile, "\tOut point: %Ld, state: %d", cutOut.Int64(), iState);
    iCutAudioBar->SetOutPoint( cutOut );
    
    if (iSampleArrayHandler)
        {
        iSampleArrayHandler->SetCutOutPoint(cutOut);            
        }
    
    if ( iState == EStatePlaying )
        {
        PauseL();
        }
    else
        {
        iView.UpdateCBAL( iState );
        }
        
    UpdateVisualizationL();                 
    LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::MarkedOutL(): Out");
    }
    
void CVeiCutAudioContainer::SetVolumeL( TInt aVolumeChange )
    {
    iInternalVolume += aVolumeChange;

    if ( iInternalVolume < 0 )
        {
        iInternalVolume = 0;
        }
    if ( iInternalVolume > KMaxCutAudioVolumeLevel )
        {
        iInternalVolume = KMaxCutAudioVolumeLevel;
        }
    
    if ( iAudioSamplePlayer )   
        {
        TInt vol = STATIC_CAST( TInt, (iInternalVolume*iMaxVolume)/KMaxCutAudioVolumeLevel );
        iAudioSamplePlayer->SetVolume( vol );
        }
    }

TInt CVeiCutAudioContainer::DoAudioBarUpdate( TAny* aThis )
    {
    STATIC_CAST( CVeiCutAudioContainer*, aThis )->DoUpdate();
    return 42;
    }

void CVeiCutAudioContainer::DoUpdate()
    {
    TTimeIntervalMicroSeconds time;
    iAudioSamplePlayer->GetPosition( time );

    if ( iSeeking )
        {
        time = iSeekPos;
        LOGFMT(KVideoEditorLogFile, "CVeiCutAudioContainer::DoUpdate(): 1, time:%Ld", time.Int64());
        }
    else
        {
        LOGFMT(KVideoEditorLogFile, "CVeiCutAudioContainer::DoUpdate(): 2, time:%Ld", time.Int64());
        }
    
    iCutAudioBar->SetCurrentPoint(static_cast<TInt32>( (time.Int64() / 1000)));
    iCutAudioBar->DrawDeferred();

    TRAP_IGNORE( UpdateVisualizationL() );
    }
    

void CVeiCutAudioContainer::UpdateVisualizationL()
    {   
    TTimeIntervalMicroSeconds time;
    iAudioSamplePlayer->GetPosition( time );
    iSampleArrayHandler->SetCurrentPoint(time);

    DrawToBufBitmapL();
    }

// @: not leave safe!
void CVeiCutAudioContainer::MapcInitComplete( TInt aError,
                    const TTimeIntervalMicroSeconds& DEBUGLOG_ARG(aDuration) )
    {
    LOGFMT2(KVideoEditorLogFile, "CVeiCutAudioContainer::MapcInitComplete(): In, aError:%d, aDuration:%Ld", aError, aDuration.Int64());
    if( aError == KErrNone )    // The sample is ready to play.
        {
        CVeiVideoEditorSettings::GetMediaPlayerVolumeLevelL( iInternalVolume );

        iMaxVolume = iAudioSamplePlayer->MaxVolume();
        TInt vol = STATIC_CAST( TInt, (iInternalVolume*iMaxVolume)/KMaxCutAudioVolumeLevel );
        iAudioSamplePlayer->SetVolume( vol );
        /* Show mute icon in navipane */
        if ( vol == 0 )
            {
            iView.VolumeDownL();
            }
        }
    iCutAudioBar->SetTotalDuration( iDuration );

    SetStateL( EStateStoppedInitial );
    iView.UpdateTimeL();

    LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::MapcInitComplete(): Out");
    }
    
void CVeiCutAudioContainer::SetDuration( const TTimeIntervalMicroSeconds& aDuration )
    {
    iDuration = aDuration;
    iCutAudioBar->SetTotalDuration( iDuration );
    }   

void CVeiCutAudioContainer::LaunchProgressNoteL()
    {
    iProgressNote = new ( ELeave ) CAknProgressDialog( REINTERPRET_CAST( CEikDialog**, 
                    &iProgressNote), ETrue);
    iProgressNote->SetCallback(this);
    iProgressNote->PrepareLC( R_VEI_PROGRESS_NOTE_WITH_CANCEL );

    HBufC* stringholder  = StringLoader::LoadLC( R_VEI_PROGRESS_NOTE_CUT, iEikonEnv );
    iProgressNote->SetTextL( *stringholder );
    CleanupStack::PopAndDestroy(stringholder);
        
    iProgressNote->GetProgressInfoL()->SetFinalValue( 100 );
    iProgressNote->RunLD();
    }

void CVeiCutAudioContainer::MapcPlayComplete( TInt DEBUGLOG_ARG(aError) )
    {
    LOGFMT(KVideoEditorLogFile, "CVeiCutAudioContainer::MapcPlayComplete(): In, error:%d", aError);

    if (EStateTerminating == iState)
        {
        LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::MapcPlayComplete: app is closing...");
        return;
        }

    TRAP_IGNORE( SetStateL( EStateStopped ) );
    iVideoBarTimer->Cancel();

    iCutAudioBar->SetFinishedStatus( ETrue );
    iLastPosition = TTimeIntervalMicroSeconds( 0 );
    iSeekPos = 0;
    TRAP_IGNORE( iView.StopNaviPaneUpdateL() );

    TRAP_IGNORE( UpdateVisualizationL() );

    LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::MapcPlayComplete(): Out");
    }

void CVeiCutAudioContainer::NotifyAudioClipVisualizationStarted(const CVedAudioClipInfo& /*aInfo*/)
    {
    TRAP_IGNORE( LaunchProgressNoteL() );
    }

void CVeiCutAudioContainer::NotifyAudioClipVisualizationProgressed(const CVedAudioClipInfo& /*aInfo*/, 
                                                        TInt aPercentage)
    {   
    if ( iProgressNote )
        {
        TRAP_IGNORE( iProgressNote->GetProgressInfoL()->SetAndDraw( aPercentage ) );
        }   
    }
                                                        
void CVeiCutAudioContainer::NotifyAudioClipVisualizationCompleted(const CVedAudioClipInfo& /*aInfo*/, 
                                                       TInt aError, TInt8* aVisualization,
                                                       TInt aResolution)
    {       
    if ( iProgressNote )
        {
        TRAP_IGNORE( iProgressNote->GetProgressInfoL()->SetAndDraw(100) );
        TRAP_IGNORE( iProgressNote->ProcessFinishedL() );
        }
    if (KErrNone == aError)
        {       
        iSampleArrayHandler->SetVisualizationArray(aVisualization, aResolution);
        TRAP_IGNORE( DrawToBufBitmapL() );
        }
    }

TInt CVeiCutAudioContainer::VisualizationResolution() const
    {
    return iIconDisplayRect.Width();    
    }   

void CVeiCutAudioContainer::DialogDismissedL( TInt aButtonId )
    {
    if (aButtonId != EAknSoftkeyDone )
        {   
        iView.CancelVisualizationL();
        if (! iCallBack)
            {       
            TCallBack cb (CVeiCutAudioContainer::AsyncBack, this);
            iCallBack = new (ELeave) CAsyncCallBack(cb, CActive::EPriorityStandard);
            }
        iCallBack->CallBack();
        }
    }
    
TInt CVeiCutAudioContainer::AsyncBack(TAny* aThis)
    {
    LOG( KVideoEditorLogFile, "CVeiCutAudioContainer::AsyncExit");

    CVeiCutAudioContainer* view = static_cast<CVeiCutAudioContainer*>(aThis);
    TRAPD( err, view->HandleCommandL(EVeiCmdCutVideoViewBack) );
    
    return err;
    }   


void CVeiCutAudioContainer::HandleCommandL(TInt aCommand)       
    {
    iView.HandleCommandL(aCommand); 
    }

void CVeiCutAudioContainer::GetVisualizationL()
    {   
    iView.GetAudioVisualizationL();
    }

//=============================================================================
void CVeiCutAudioContainer::HandleVolumeUpL()
    {
    LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::HandleVolumeUpL: in");

    iView.VolumeUpL();

    LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::HandleVolumeUpL: out");
    }

//=============================================================================
void CVeiCutAudioContainer::HandleVolumeDownL()
    {
    LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::HandleVolumeDownL: in");

    iView.VolumeDownL();

    LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::HandleVolumeDownL: out");
    }

//=============================================================================
void CVeiCutAudioContainer::PrepareForTerminationL()
    {
    LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::PrepareForTerminationL: in");

    SetStateL( EStateTerminating );

    iVideoBarTimer->Cancel();
    iAudioSamplePlayer->Stop();
    iAudioSamplePlayer->Close();

    LOG(KVideoEditorLogFile, "CVeiCutAudioContainer::PrepareForTerminationL: out");
    }

// End of File
