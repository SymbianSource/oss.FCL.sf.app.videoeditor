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
#include <AknIconUtils.h>   // AknIconUtils
#include <eikenv.h>     // iEikonEnv
#include <trimformms.rsg>    // Video Editor resources
#include <StringLoader.h>   // StringLoader

#include <aknbiditextutils.h>
#include <gulfont.h>

#include <aknsdrawutils.h> 
#include <aknsdatacontext.h> 
#include <aknsbasicbackgroundcontrolcontext.h>

#include <AknProgressDialog.h> 
#include <eikprogi.h>
#include <AknWaitDialog.h>


// User includes
#include "VeiTrimForMmsContainer.h"
#include "VeiVideoDisplay.h"
#include "VideoEditorCommon.h"
//#include "VideoEditorHelp.hlp.hrh"  // Topic contexts (literals)
#include "veiframetaker.h"
#include "VeiTrimForMmsView.h"
#include "VeiCutterBar.h"
#include "VeiVideoEditorSettings.h"
#include "VideoEditorUtils.h"
#include "VideoEditorDebugUtils.h"

void CVeiTrimForMmsContainer::DialogDismissedL( TInt /*aButtonId*/ )
	{
	iProgressInfo = NULL;
	}


CVeiTrimForMmsContainer* CVeiTrimForMmsContainer::NewL( const TRect& aRect,
                                                        CVedMovie& aMovie, CVeiTrimForMmsView& aView )
    {
    CVeiTrimForMmsContainer* self = CVeiTrimForMmsContainer::NewLC( aRect,
                                                                    aMovie,
																	aView );
    CleanupStack::Pop( self );

    return self;
    }

CVeiTrimForMmsContainer* CVeiTrimForMmsContainer::NewLC( const TRect& aRect,
                                                         CVedMovie& aMovie,
														 CVeiTrimForMmsView& aView)
    {
    CVeiTrimForMmsContainer* self = 
            new(ELeave)CVeiTrimForMmsContainer( aMovie, aView );

    CleanupStack::PushL( self );
    self->ConstructL( aRect );

    return self;
    }


CVeiTrimForMmsContainer::CVeiTrimForMmsContainer( CVedMovie& aMovie, CVeiTrimForMmsView& aView ):
                                                  iLastKeyLeftOrRight(EFalse),
                                                  iMovie(aMovie),
												  iView( aView )
                                                  
    {
    }


void CVeiTrimForMmsContainer::ConstructL( const TRect& aRect )
    {
    LOG(KVideoEditorLogFile, "CVeiTrimForMmsContainer::ConstructL: in");

    // Make this compound control window-owning.
    CreateWindowL();

    iMovie.RegisterMovieObserverL( this );    

    // Read the texts shown above the thumbnails from resources
    iStartText = StringLoader::LoadL( R_VED_THUMBNAIL_START_TEXT, iEikonEnv );
    iEndText = StringLoader::LoadL( R_VED_THUMBNAIL_END_TEXT, iEikonEnv );
	
	iSeekPos = TTimeIntervalMicroSeconds( 0 );

    iVideoDisplayStart = CVeiVideoDisplay::NewL( iVideoDisplayStartRect, this, *this );

    iVideoDisplayEnd = CVeiVideoDisplay::NewL( iVideoDisplayEndRect, this, *this );

	iCutterBar = CVeiCutterBar::NewL( this );
	iCutterBar->SetPlayHeadVisible( EFalse );
	iVideoDisplay = CVeiVideoDisplay::NewL( aRect, this, *this );

	iFrameTaker = CVeiFrameTaker::NewL( *this );

	/* Timer to keep back light on when user is not giving key events */
	iScreenLight = CVeiDisplayLighter::NewL();
	CVeiVideoEditorSettings::GetMaxMmsSizeL( iMaxMmsSize );
	/* SharedData returns maxmmssize in kbytes. Change it to bytes(1000) and
	   add some margin to final value.*/
	iMaxMmsSize = STATIC_CAST( TInt, iMaxMmsSize*0.98 );
	iBlack = EFalse;

	// Set this control extent.
    SetRect( aRect );
	iBgContext = CAknsBasicBackgroundControlContext::NewL( KAknsIIDQsnBgAreaMain, Rect(), EFalse );

    ActivateL();

	iKeyEnable = EFalse;
    LOG(KVideoEditorLogFile, "CVeiTrimForMmsContainer::ConstructL: out");
    }



CVeiTrimForMmsContainer::~CVeiTrimForMmsContainer()
    {
	iMovie.UnregisterMovieObserver( this );

    delete iEndText;
    delete iStartText;
	delete iScreenLight;

    delete iCutterBar;
    
    delete iVideoDisplayStart;
    delete iVideoDisplayEnd;
	if( iVideoDisplay )
        {
        delete iVideoDisplay;
		iVideoDisplay = NULL;
        }

	if (iFrameTaker)
		{
		delete iFrameTaker;
		iFrameTaker = NULL;
		}

	if ( iProgressNote )
		{
		delete iProgressNote;
		iProgressNote = NULL;
		}
	iProgressInfo = NULL;

	delete iBgContext;
    }

void CVeiTrimForMmsContainer::SetMaxMmsSize( TInt aMaxSizeInBytes )
	{
	iMaxMmsSize = aMaxSizeInBytes;
	}

const TTimeIntervalMicroSeconds& CVeiTrimForMmsContainer::CutInTime() const
    {
	return iSeekPos;
	}


const TTimeIntervalMicroSeconds& CVeiTrimForMmsContainer::CutOutTime() const
    {
	return iSeekEndPos;
	}


// ----------------------------------------------------------------------------
// CVeiTrimForMmsContainer::ComponentControl(...) const
//
// Gets the specified component of a compound control. 
// ----------------------------------------------------------------------------
//
CCoeControl* CVeiTrimForMmsContainer::ComponentControl( TInt aIndex ) const
    {
    LOG(KVideoEditorLogFile, "CVeiTrimForMmsContainer::ComponentControl()");

    switch( aIndex )
        {
        //
        // iCutterDisplay
        //
        case ECutFrame:
            {
            return iCutterBar;
            }
        //
        // iVideoDisplayStart
        //
        case EVideoDisplayStart:
            {
            return iVideoDisplayStart;
            }
        //
        // iVideoDisplayEnd
        //
        case EVideoDisplayEnd:
            {
            return iVideoDisplayEnd;
            }
		//
        // iVideoDisplay
        //
		case EVideoPreview:
			{
			return iVideoDisplay;
	
			}
			
        //
        // Default
        //
        default:
            {
            return NULL;
            }
        }

    }


// ----------------------------------------------------------------------------
// CVeiTrimForMmsContainer::CountComponentControls() const
//
// Gets the number of controls contained in a compound control.
// ----------------------------------------------------------------------------
//
TInt CVeiTrimForMmsContainer::CountComponentControls() const
    {
    return ENumberOfControls;
    }


void CVeiTrimForMmsContainer::Draw( const TRect& aRect ) const
    {
    LOG(KVideoEditorLogFile, "CVeiTrimForMmsContainer::Draw(): In");

    CWindowGc& gc = SystemGc();
	gc.Clear( aRect );

	// Black backbround for the preview
	if ( iBlack )
        {
		iVideoDisplay->MakeVisible( EFalse );
        gc.SetPenStyle( CWindowGc::ENullPen );
		gc.SetBrushColor( KRgbBlack );
		gc.SetBrushStyle( CGraphicsContext::ESolidBrush );
		gc.DrawRect( aRect );
        gc.SetPenStyle( CWindowGc::ESolidPen );	
	    gc.DrawRoundRect(aRect, TSize(4,4));
		return;
        }
	else
	{
	 // Draw skin background
	MAknsSkinInstance* skin = AknsUtils::SkinInstance();
	MAknsControlContext* cc = AknsDrawUtils::ControlContext( this );
	AknsDrawUtils::Background( skin, cc, this, gc, aRect );

	const CFont* font = AknLayoutUtils::FontFromId( EAknLogicalFontSecondaryFont );
	TBuf<95> startVisualText;
	TPoint startTextPoint;
	TBuf<95> endVisualText;
	TPoint endTextPoint;

	startTextPoint.iY = iStartTextBox.iTl.iY + font->HeightInPixels();
	endTextPoint.iY = iEndTextBox.iTl.iY + font->HeightInPixels();
	
	gc.UseFont( font );
		
	TBidiText::TDirectionality textDirectionality;

	textDirectionality = AknTextUtils::CurrentScriptDirectionality();

	TInt maxWidthNonClippingStart = iStartTextBox.Width();

	AknBidiTextUtils::ConvertToVisualAndClip( *iStartText, startVisualText, *font, maxWidthNonClippingStart, 
		maxWidthNonClippingStart );
	/** check text alignment */
	if ( textDirectionality == TBidiText::ELeftToRight )
		{
		startTextPoint.iX = iVideoDisplayStartRect.iTl.iX;
		}
	else
		{
		startTextPoint.iX = iVideoDisplayStartRect.iBr.iX - font->TextWidthInPixels( startVisualText );
		}

	gc.DrawText( startVisualText, startTextPoint );	


	TInt maxWidthNonClippingEnd = iEndTextBox.Width();

	AknBidiTextUtils::ConvertToVisualAndClip( *iEndText, endVisualText, *font, maxWidthNonClippingEnd, 
		maxWidthNonClippingEnd );
	/** check text alignment */
	if ( textDirectionality == TBidiText::ELeftToRight )
		{
		endTextPoint.iX = iVideoDisplayEndRect.iTl.iX;
		}
	else
		{
		endTextPoint.iX = iVideoDisplayEndRect.iBr.iX - font->TextWidthInPixels( endVisualText );
		}

	gc.DrawText( endVisualText, endTextPoint );	

	gc.DiscardFont();

	// Draw Start/End displays
	iVideoDisplayStart->SetRect( iVideoDisplayStartRect );
	iVideoDisplayEnd->SetRect( iVideoDisplayEndRect );

	if( iPreviewState == EPlaying )
		{
		iVideoDisplay->MakeVisible( ETrue );
		}	
	else
		{
		iVideoDisplay->MakeVisible( EFalse );
		}

	}

    LOG(KVideoEditorLogFile, "CVeiTrimForMmsContainer::Draw(): Out");
    }


// ----------------------------------------------------------------------------
// CVeiTrimForMmsContainer::GetHelpContext(...) const
//
// Gets the control's help context. Associates the control with a particular
// Help file and topic in a context sensitive application.
// ----------------------------------------------------------------------------
//
void CVeiTrimForMmsContainer::GetHelpContext( TCoeHelpContext& /*aContext*/ ) const
    {
    LOG(KVideoEditorLogFile, "CVeiTrimForMmsContainer::GetHelpContext(): In");

    // Set UID of the CS Help file (same as application UID).
    //aContext.iMajor = KUidVideoEditor;

    // Set the context/topic.
    //aContext.iContext = KVED_HLP_TRIM_FOR_MMS_VIEW;

    LOG(KVideoEditorLogFile, "CVeiTrimForMmsContainer::GetHelpContext(): Out");
    }

// ----------------------------------------------------------------------------
// CVeiTrimForMmsContainer::SizeChanged()
//
// The function is called whenever SetExtent(), SetSize(), SetRect(),
// SetCornerAndSize(), or SetExtentToWholeScreen() are called on the control.
// ----------------------------------------------------------------------------
//
void CVeiTrimForMmsContainer::SizeChanged()
    {
	LOG(KVideoEditorLogFile, "CVeiTrimForMmsContainer::SizeChanged(): In");

	TRect rect = Rect();
	if ( iBgContext )
		{
		iBgContext->SetRect( rect );
		}

	if ( VideoEditorUtils::IsLandscapeScreenOrientation() ) //Landscape
	{
		//  Start Text rect
		TInt startTextTlX = STATIC_CAST( TInt, rect.iBr.iX * 0.00962 );
		TInt startTextTlY = STATIC_CAST( TInt, rect.iBr.iY * 0.01389 );	
		TInt startTextBrX = STATIC_CAST( TInt, rect.iBr.iX * 0.49512 );
		TInt startTextBrY = STATIC_CAST( TInt, rect.iBr.iY * 0.09375 );
			
		iStartTextBox = TRect(startTextTlX, startTextTlY, startTextBrX, 
			startTextBrY);
		//  End Text rect
		TInt endTextTlX = STATIC_CAST( TInt, rect.iBr.iX * 0.50481 );
		TInt endTextTlY = STATIC_CAST( TInt, rect.iBr.iY * 0.01389 );	
		TInt endTextBrX = STATIC_CAST( TInt, rect.iBr.iX * 0.99039 );
		TInt endTextBrY = STATIC_CAST( TInt, rect.iBr.iY * 0.09375 );
			
		iEndTextBox = TRect(endTextTlX, endTextTlY, endTextBrX, 
			endTextBrY);

		// Start Video rect
		TInt startVideoTlX = STATIC_CAST( TInt, rect.iBr.iX * 0.00962 );
		TInt startVideoTlY = STATIC_CAST( TInt, rect.iBr.iY * 0.10764 );	
		TInt startVideoBrX = STATIC_CAST( TInt, rect.iBr.iX * 0.49512 );
		TInt startVideoBrY = STATIC_CAST( TInt, rect.iBr.iY * 0.68056 );
			
		iVideoDisplayStartRect = TRect(startVideoTlX, startVideoTlY, startVideoBrX, 
			startVideoBrY);
		//  End Video rect
		TInt endVideoTlX = STATIC_CAST( TInt, rect.iBr.iX * 0.50481 );
		TInt endVideoTlY = STATIC_CAST( TInt, rect.iBr.iY * 0.10764 );	
		TInt endVideoBrX = STATIC_CAST( TInt, rect.iBr.iX * 0.99039 );
		TInt endVideoBrY = STATIC_CAST( TInt, rect.iBr.iY * 0.68056 );
			
		iVideoDisplayEndRect = TRect(endVideoTlX, endVideoTlY, endVideoBrX, 
			endVideoBrY);

		// Timeline rect
		TInt timeLineTlX = STATIC_CAST( TInt, rect.iBr.iX * 0.0114 );
		TInt timeLineTlY = STATIC_CAST( TInt, rect.iBr.iY * 0.80208 );
			
		TSize cutBarSize = TSize(STATIC_CAST( TInt, rect.iBr.iX*0.9773 ),
			STATIC_CAST( TInt, rect.iBr.iY*0.0973 ) );

		iTimelineRect = TRect( TPoint(timeLineTlX, timeLineTlY), cutBarSize );
	}
	else // Portrait
	{
		//  Start Text rect
		TInt startTextTlX = STATIC_CAST( TInt, rect.iBr.iX * 0.01136 );
		TInt startTextTlY = STATIC_CAST( TInt, rect.iBr.iY * 0.01389 );	
		TInt startTextBrX = STATIC_CAST( TInt, rect.iBr.iX * 0.49432 );
		TInt startTextBrY = STATIC_CAST( TInt, rect.iBr.iY * 0.10417 );
			
		iStartTextBox = TRect(startTextTlX, startTextTlY, startTextBrX, 
			startTextBrY);

		//  End Text rect
		TInt endTextTlX = STATIC_CAST( TInt, rect.iBr.iX * 0.50568 );
		TInt endTextTlY = STATIC_CAST( TInt, rect.iBr.iY * 0.01389 );	
		TInt endTextBrX = STATIC_CAST( TInt, rect.iBr.iX * 0.98864 );
		TInt endTextBrY = STATIC_CAST( TInt, rect.iBr.iY * 0.10417 );
			
		iEndTextBox = TRect(endTextTlX, endTextTlY, endTextBrX, 
			endTextBrY);

		// Start Video rect
		TInt startVideoTlX = STATIC_CAST( TInt, rect.iBr.iX * 0.01136 );
		TInt startVideoTlY = STATIC_CAST( TInt, rect.iBr.iY * 0.11806 );	
		TInt startVideoBrX = STATIC_CAST( TInt, rect.iBr.iX * 0.49432 );
		TInt startVideoBrY = STATIC_CAST( TInt, rect.iBr.iY * 0.60069 );
			
		iVideoDisplayStartRect = TRect(startVideoTlX, startVideoTlY, startVideoBrX, 
			startVideoBrY);

		//  End Video rect
		TInt endVideoTlX = STATIC_CAST( TInt, rect.iBr.iX * 0.50568 );
		TInt endVideoTlY = STATIC_CAST( TInt, rect.iBr.iY * 0.11806 );	
		TInt endVideoBrX = STATIC_CAST( TInt, rect.iBr.iX * 0.98864 );
		TInt endVideoBrY = STATIC_CAST( TInt, rect.iBr.iY * 0.60069 );
			
		iVideoDisplayEndRect = TRect(endVideoTlX, endVideoTlY, endVideoBrX, 
			endVideoBrY);
		
		// Timeline rect
		TInt timeLineTlX = STATIC_CAST( TInt, rect.iBr.iX * 0.0114 );
		TInt timeLineTlY = STATIC_CAST( TInt, rect.iBr.iY * 0.767361 );

			
		TSize cutBarSize = TSize(STATIC_CAST( TInt, rect.iBr.iX*0.9773 ),
			STATIC_CAST( TInt, rect.iBr.iY*0.0973 ) );

		iTimelineRect = TRect(TPoint(timeLineTlX, timeLineTlY), cutBarSize);				
		}

	iCutterBar->SetRect( iTimelineRect );

	LOG(KVideoEditorLogFile, "CVeiTrimForMmsContainer::SizeChanged(): Out");
    }


TKeyResponse CVeiTrimForMmsContainer::OfferKeyEventL( 
                                        const TKeyEvent& aKeyEvent,
									    TEventCode aType )
    {
	if( iKeyEnable )
	{
    switch( aType )
        {
        //
        // Key down event
        //
        case EEventKeyDown:
            {				
			if( iPreviewState == EPlaying )
				{
//				iView.SetTrimStateL( CVeiTrimForMmsView::ESeek );
//				Stop( ETrue );
				return EKeyWasConsumed;
				}
			else
				{
				iKeyRepeatCount = 0;

				iMovie.VideoClipSetCutInTime( 0,TTimeIntervalMicroSeconds(0) );
		    	iMovie.VideoClipSetCutOutTime( 0, iDuration );
				return EKeyWasConsumed;
				}
			}
        //
        // The key event
        //
        case EEventKey:
            {
			if( iPreviewState == EPlaying )
				{
				return EKeyWasConsumed;
				}
            switch( aKeyEvent.iCode )
                {
                //
                // Navi-key right
                //
                case EKeyRightArrow:
                    {
					if( iSeekEndPos != iDuration )
						{
						iSeekEvent = ETrue;
						iLastKeyLeftOrRight = ETrue;
						if(iKeyRepeatCount < 18)
							{
							iKeyRepeatCount++;
							}

						TInt adjustment = TimeIncrement( iKeyRepeatCount );
						TInt64 newPos = iSeekPos.Int64() + adjustment;

						TTimeIntervalMicroSeconds endTime(0);
				        iMovie.GetDurationEstimateL( iMaxMmsSize, newPos, endTime );
						
						if ( endTime.Int64() >= iDuration.Int64() - adjustment )			
							{
							iKeyRepeatCount-=3;
							adjustment = TimeIncrement( iKeyRepeatCount );						
							endTime = iDuration;
							newPos+=adjustment;
							}
	
						iSeekPos = TTimeIntervalMicroSeconds( newPos );
				
						iSeekEndPos = endTime;
						iCutterBar->SetInPoint(iSeekPos);
						iCutterBar->SetOutPoint(endTime);
						}
				
					return EKeyWasConsumed;
                    }
                //
                // Navi-key left
                //
                case EKeyLeftArrow:
                    {
					if( iSeekPos.Int64() > 0 )
						{
					iSeekEvent = ETrue;
					iLastKeyLeftOrRight = ETrue;
                    // Process the command only when repeat count is zero.
  					iKeyRepeatCount++;


					TInt adjustment = TimeIncrement( iKeyRepeatCount );

					TInt64 newPos = iSeekPos.Int64() - adjustment;
					if ( newPos < 0 ) 
						{
						newPos = 0;
						}
					iSeekPos = TTimeIntervalMicroSeconds( newPos );	

					TTimeIntervalMicroSeconds endTime(0);
			        iMovie.GetDurationEstimateL( iMaxMmsSize, newPos, endTime );

					iSeekEndPos = endTime;

					iCutterBar->SetInPoint(iSeekPos);
					iCutterBar->SetOutPoint(endTime);
						}
					return EKeyWasConsumed;
                    }
                //
                // Default
                //
                default:
                    {
                    return EKeyWasNotConsumed;
                    }
                }
              }
        //
        // Key up event
        //

        case EEventKeyUp:  
			{
			if( iPreviewState == EPlaying )
				{
				iView.SetTrimStateL( CVeiTrimForMmsView::ESeek );
		
				Stop( ETrue );
				return EKeyWasConsumed;
				}
			else
				{
				if ( iLastKeyLeftOrRight )
					{
					iView.ProcessNeeded( ETrue );
				    iMovie.VideoClipSetCutInTime( 0, iSeekPos );
					iMovie.VideoClipSetCutOutTime( 0, iSeekEndPos );
					iView.UpdateNaviPaneL( iMovie.GetSizeEstimateL()/1024,iMovie.Duration() );
					iLastKeyLeftOrRight = EFalse;
					if( iSeekEvent )
					{
					StartFrameTakerL( iClipIndex );
					iSeekEvent = EFalse;
					}
					return EKeyWasConsumed;
					}
				}
            break;
			}
        //
        // Default
        //
        default:
            return EKeyWasNotConsumed;
        }
		} //iKeyEnable
	return EKeyWasNotConsumed;  
    }



TInt CVeiTrimForMmsContainer::TimeIncrement(TInt aKeyCount) const
	{
	if ( aKeyCount < 3 )
		{
		return 100000;
		}
	else if ( aKeyCount < 6 ) // 4
		{
		return 300000;
		}
	else if ( aKeyCount < 9 ) // 5
		{
		return 500000;
		}
	else if ( aKeyCount < 12 ) // 10
		{
		return 1000000;
		}
	else if ( aKeyCount < 15 ) // 13
		{
		return 2000000;
		}
	else if ( aKeyCount < 18 ) // 15
		{
		return 3000000;
		}
	else
		{
		return 5000000;
		}	
	}


void CVeiTrimForMmsContainer::NotifyVideoDisplayEvent( const TPlayerEvent aEvent, const TInt& aInfo  )
    {
	switch (aEvent)
		{
		case MVeiVideoDisplayObserver::EOpenComplete:
			{
			iVideoDisplay->SetRect( iPreviewRect );
			if ( !VideoEditorUtils::IsLandscapeScreenOrientation() ) //Portrait
				{
				iVideoDisplay->SetRotationL( EVideoRotationClockwise90 );
				}
			iPreviewState = ELoading;

			iVideoDisplay->SetPositionL( CutInTime() );
			iVideoDisplay->PlayL( iMovie.VideoClipInfo( iClipIndex )->FileName(), CutInTime(), CutOutTime() );

			break;
			}

		case MVeiVideoDisplayObserver::ELoadingComplete:
			{

			iVideoDisplay->MakeVisible( ETrue );			
			iPreviewState = EPlaying;
			break;
			}

		case MVeiVideoDisplayObserver::EStop:
			{	
			iPreviewState = EIdle;
			iView.SetTrimStateL( CVeiTrimForMmsView::ESeek );

			iVideoDisplay->MakeVisible(EFalse);

			DrawDeferred();			
			break;
			}
		case MVeiVideoDisplayObserver::EPlayComplete:
			{
			iView.SetTrimStateL( CVeiTrimForMmsView::ESeek );
			Stop( ETrue );
			if (KErrNoMemory == aInfo)
				{
				iView.ShowGlobalErrorNoteL( KErrNoMemory );	
				}
			break;
			}
		default:
			{
			break;
			}
		}
    }


/**
 * Called to notify that a new video clip has been successfully
 * added to the movie. Note that the indices and the start and end times
 * of the video clips after the new clip have also changed as a result.
 * Note that the transitions may also have changed. 
 *
 * @param aMovie  movie
 * @param aIndex  index of video clip in movie
 */
void CVeiTrimForMmsContainer::NotifyVideoClipAdded( CVedMovie& aMovie, TInt aIndex )
    {
    LOG(KVideoEditorLogFile, "CVeiTrimForMmsContainer::NotifyVideoClipAdded: in");

	iSeekPos = TTimeIntervalMicroSeconds(0);

	aMovie.GetDurationEstimateL( iMaxMmsSize, iSeekPos, iSeekEndPos );
	LOGFMT3(KVideoEditorLogFile, "CVeiTrimForMmsContainer::NotifyVideoClipAdded: 1, iMaxMmsSize:%d, iSeekPos:%Ld, iSeekEndPos:%Ld", 
	iMaxMmsSize, iSeekPos.Int64(), iSeekEndPos.Int64());

	iCutterBar->SetInPoint( iSeekPos );
	iCutterBar->SetOutPoint( iSeekEndPos );
	iCutterBar->SetTotalDuration( aMovie.Duration() );
	iDuration = aMovie.Duration();
    
	iClipIndex = aIndex;

    aMovie.VideoClipSetCutInTime( 0, iSeekPos );
    aMovie.VideoClipSetCutOutTime( 0, iSeekEndPos );
    
    TInt movieSizeLimit = static_cast<TInt>(iMaxMmsSize*0.9);
	aMovie.SetMovieSizeLimit( movieSizeLimit );
	
	LOGFMT(KVideoEditorLogFile, "CVeiTrimForMmsContainer::NotifyVideoClipAdded(): 2, movie size set to:%d", movieSizeLimit);

    StartFrameTakerL( iClipIndex );

	iSeekEvent = EFalse;


	iView.UpdateNaviPaneL( iMovie.GetSizeEstimateL()/1024,iMovie.Duration() );

	iKeyEnable = ETrue;

    LOG(KVideoEditorLogFile, "CVeiTrimForMmsContainer::NotifyVideoClipAdded: out");
    }


void CVeiTrimForMmsContainer::NotifyVideoClipAddingFailed( CVedMovie& /*aMovie*/,
                                                           TInt /*aError*/ )
    {
    }

void CVeiTrimForMmsContainer::NotifyVideoClipRemoved( CVedMovie& /*aMovie*/,
                                                      TInt /*aIndex*/ )
    {
    }
	
void CVeiTrimForMmsContainer::NotifyVideoClipIndicesChanged( CVedMovie& /*aMovie*/,
                                                             TInt /*aOldIndex*/, 
									                         TInt /*aNewIndex*/ )
    {
    }

void CVeiTrimForMmsContainer::NotifyVideoClipTimingsChanged( 
                                                CVedMovie& /*aMovie*/,
											    TInt /*aIndex*/ )
    {
    }

void CVeiTrimForMmsContainer::NotifyVideoClipColorEffectChanged(
                                                CVedMovie& /*aMovie*/,
												TInt /*aIndex*/ )
    {
    }
	
void CVeiTrimForMmsContainer::NotifyVideoClipAudioSettingsChanged(
                                                CVedMovie& /*aMovie*/,
											    TInt /*aIndex*/ )
    {
    }

void CVeiTrimForMmsContainer::NotifyVideoClipGeneratorSettingsChanged(
                                                CVedMovie& /*aMovie*/,
											    TInt /*aIndex*/ )
    {
    }

void CVeiTrimForMmsContainer::NotifyVideoClipDescriptiveNameChanged(
                                                CVedMovie& /*aMovie*/,
												TInt /*aIndex*/ )
    {
    }

void CVeiTrimForMmsContainer::NotifyStartTransitionEffectChanged(
                                                CVedMovie& /*aMovie*/ )
    {
    }


void CVeiTrimForMmsContainer::NotifyMiddleTransitionEffectChanged(
                                                CVedMovie& /*aMovie*/, 
											    TInt /*aIndex*/ )
    {
    }

void CVeiTrimForMmsContainer::NotifyEndTransitionEffectChanged(
                                                CVedMovie& /*aMovie*/ )
    {
    }

void CVeiTrimForMmsContainer::NotifyAudioClipAdded( CVedMovie& /*aMovie*/,
                                                    TInt /*aIndex*/ )
    {
    }

void CVeiTrimForMmsContainer::NotifyAudioClipAddingFailed( 
                                                CVedMovie& /*aMovie*/,
                                                TInt /*aError*/ )
    {
    }

void CVeiTrimForMmsContainer::NotifyAudioClipRemoved( CVedMovie& /*aMovie*/,
                                                      TInt /*aIndex*/ )
    {
    }

void CVeiTrimForMmsContainer::NotifyAudioClipIndicesChanged(
                                                CVedMovie& /*aMovie*/,
                                                TInt /*aOldIndex*/, 
									            TInt /*aNewIndex*/ )
    {
    }

void CVeiTrimForMmsContainer::NotifyAudioClipTimingsChanged( 
                                                CVedMovie& /*aMovie*/,
											    TInt /*aIndex*/ )
    {
    }

void CVeiTrimForMmsContainer::NotifyMovieQualityChanged( CVedMovie& /*aMovie*/ )
    {
    }


void CVeiTrimForMmsContainer::NotifyMovieReseted( CVedMovie& /*aMovie*/ )
    {
    }

void CVeiTrimForMmsContainer::NotifyMovieOutputParametersChanged(CVedMovie& /*aMovie*/)
	{
	}

void CVeiTrimForMmsContainer::NotifyAudioClipDynamicLevelMarkInserted(CVedMovie& /*aMovie*/, 
                                                         TInt /*aClipIndex*/, 
                                                         TInt /*aMarkIndex*/)
	{
	}

void CVeiTrimForMmsContainer::NotifyAudioClipDynamicLevelMarkRemoved(CVedMovie& /*aMovie*/, 
                                                        TInt /*aClipIndex*/, 
                                                        TInt /*aMarkIndex*/)
	{
	}

void CVeiTrimForMmsContainer::NotifyVideoClipDynamicLevelMarkInserted(CVedMovie& /*aMovie*/,
                                                         TInt /*aClipIndex*/, 
                                                         TInt /*aMarkIndex*/)
	{
	}

void CVeiTrimForMmsContainer::NotifyVideoClipDynamicLevelMarkRemoved(CVedMovie& /*aMovie*/, 
                                                        TInt /*aClipIndex*/, 
                                                        TInt /*aMarkIndex*/)
	{
	}

void CVeiTrimForMmsContainer::PlayL( const TDesC& aFilename, const TRect& aRect )
	{
	iCutterBar->MakeVisible( EFalse );
	iVideoDisplayStart->MakeVisible( EFalse );
	iVideoDisplayEnd->MakeVisible( EFalse );
	iVideoDisplay->MakeVisible( EFalse );

	iVideoDisplay->ShowBlackScreen();

	iPreviewRect = aRect; 
	iBlack = ETrue;
	iScreenLight->Start();
	
	iPreviewState = EOpeningFile;
	iVideoDisplay->OpenFileL( aFilename );
	}

void CVeiTrimForMmsContainer::Stop( TBool aCloseStream )
	{
		iCutterBar->MakeVisible( ETrue );
		iVideoDisplayStart->MakeVisible( ETrue );
		iVideoDisplayEnd->MakeVisible( ETrue );
		iBlack = EFalse;
		iScreenLight->Stop();
		iVideoDisplay->Stop( aCloseStream ); 
		DrawNow();
	}

void CVeiTrimForMmsContainer::PauseL()
	{	
		iPreviewState = EPause;
		iVideoDisplay->PauseL();
		iVideoDisplay->MakeVisible(EFalse);
		DrawNow();
	}

TInt CVeiTrimForMmsContainer::PreviewState() const
	{
	return iPreviewState;
	}	

void CVeiTrimForMmsContainer::NotifyFramesCompleted( CFbsBitmap* aFirstFrame, 
									   CFbsBitmap* aLastFrame,  CFbsBitmap* /*aTimelineFrame*/,  TInt aError )
	{
	if( aError==KErrNone )
		{
		iVideoDisplayStart->ShowPictureL( *aFirstFrame );
		iVideoDisplayEnd->ShowPictureL( *aLastFrame );
		}

	if( iProgressNote )
		{
		iProgressInfo->SetAndDraw(100);	
		iProgressNote->ProcessFinishedL();	
		}
	}

void CVeiTrimForMmsContainer::StartFrameTakerL( TInt aIndex )
	{
	iProgressNote = 
		new (ELeave) CAknProgressDialog(REINTERPRET_CAST(CEikDialog**, &iProgressNote), ETrue);
	iProgressNote->SetCallback(this);
	iProgressNote->ExecuteDlgLD( R_VEI_PROGRESS_NOTE );

	HBufC* stringholder;
	stringholder = StringLoader::LoadL( R_VEI_PROGRESS_NOTE_PROCESSING, iEikonEnv );
	CleanupStack::PushL( stringholder );
	iProgressNote->SetTextL( *stringholder );
	CleanupStack::PopAndDestroy( stringholder );	

	iProgressInfo = iProgressNote->GetProgressInfoL();
	iProgressInfo->SetFinalValue(100);
	iProgressInfo->SetAndDraw(50);	

// First frame is shown in main display so it is bigger.. Last frame is always
// on transition display and one frame for the video timeline.
	TSize firstThumbResolution = iVideoDisplayStart->GetScreenSize();
	TSize lastThumbResolution = iVideoDisplayEnd->GetScreenSize();
    TSize timelineThumbResolution = TSize( 34, 28 );
	 
	TInt frameCount = iMovie.VideoClipInfo(aIndex)->VideoFrameCount();

	TInt firstThumbNailIndex =  iMovie.VideoClipInfo(aIndex)->GetVideoFrameIndexL( CutInTime() );	
	TInt lastThumbNailIndex =  iMovie.VideoClipInfo(aIndex)->GetVideoFrameIndexL( CutOutTime() );    
	if ( lastThumbNailIndex >= frameCount )
		{
		lastThumbNailIndex = frameCount-1;
		}


	iFrameTaker->GetFramesL( *iMovie.VideoClipInfo(aIndex), 
			firstThumbNailIndex, &firstThumbResolution,
			lastThumbNailIndex, &lastThumbResolution, 
            firstThumbNailIndex, &timelineThumbResolution,
			EPriorityLow );
	}

TTypeUid::Ptr CVeiTrimForMmsContainer::MopSupplyObject( TTypeUid aId )
	{
	if ( aId.iUid == MAknsControlContext::ETypeId && iBgContext )
		{
		return MAknsControlContext::SupplyMopObject( aId, iBgContext );
		}
	return CCoeControl::MopSupplyObject( aId );
	}
// End of File

