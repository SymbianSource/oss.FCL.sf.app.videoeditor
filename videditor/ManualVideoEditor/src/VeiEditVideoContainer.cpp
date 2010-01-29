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
#include <manualvideoeditor.mbg>
#include <videoeditoruicomponents.rsg>
#include <videoeditoruicomponents.mbg>
#include <videoeditorbitmaps.mbg>
#include <gulicon.h>
#include <stringloader.h>
#include <aknsdrawutils.h> 
#include <aknsdatacontext.h> 
#include <aknsbasicbackgroundcontrolcontext.h> 
#include <aknquerydialog.h> 
#include <eikprogi.h> 
#include <aknnotewrappers.h> 
#include <errorui.h>
#include <aknbiditextutils.h>
#include <gulfont.h>
#include <akniconutils.h>
#include <pathinfo.h>
#include <f32file.h>
#include <CMGAlbumManager.h>
#include <vedcommon.h>
#include <mmf/common/mmferrors.h>

// User includes
#include "VeiEditVideoContainer.h"
#include "VeiVideoDisplay.h"
#include "VeiTextDisplay.h"
#include "VeiCutterBar.h"
#include "veiappui.h"
#include "veiframetaker.h"
#include "VeiIconBox.h"
#include "VideoEditorCommon.h"
#include "VideoEditorUtils.h"
#include "VideoEditorHelp.hlp.hrh"  // Topic contexts (literals)
#include "VeiEditVideoView.h"
#include "ManualVideoEditor.hrh"
#include "VeiSlider.h"
#include "StoryboardItems.h"
#include "TransitionInfo.h"


// ================= MEMBER FUNCTIONS =======================
void CleanupRArray( TAny* object )
    {
    (( RImageTypeDescriptionArray*)object)->ResetAndDestroy();
    }

CVeiEditVideoContainer* CVeiEditVideoContainer::NewL( const TRect& aRect, 
                                CVedMovie& aMovie, CVeiEditVideoView& aView )
    {
    CVeiEditVideoContainer* self = CVeiEditVideoContainer::NewLC( aRect, 
                                                            aMovie, aView );
    CleanupStack::Pop( self );
    return self;
    }

CVeiEditVideoContainer* CVeiEditVideoContainer::NewLC( const TRect& aRect,
                                CVedMovie& aMovie, CVeiEditVideoView& aView )
    {
    CVeiEditVideoContainer* self = new (ELeave) CVeiEditVideoContainer( aMovie, aView );
    CleanupStack::PushL( self );
    self->ConstructL( aRect );
    return self;
    }

// ---------------------------------------------------------
// CVeiEditVideoContainer::ConstructL(const TRect& aRect)
// EPOC two phased constructor
// ---------------------------------------------------------
//
void CVeiEditVideoContainer::ConstructL( const TRect& aRect )
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::ConstructL in");

    CreateWindowL();

    iMovie.RegisterMovieObserverL( this );

    iConverter = CVeiImageConverter::NewL( this );

    TFileName mbmPath(  VideoEditorUtils::IconFileNameAndPath(KManualVideoEditorIconFileId) );
    TFileName mbmPath2( VideoEditorUtils::IconFileNameAndPath(KVideoEditorUiComponentsIconFileId) );
    TFileName mbmPath3( VideoEditorUtils::IconFileNameAndPath(KVeiNonScalableIconFileId) );

    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::ConstructL  Loading bitmaps..");

    // No thumbnail icon (shown when video thumb nail cannot be generated)
    AknIconUtils::CreateIconL( iNoThumbnailIcon, iNoThumbnailIconMask,
            mbmPath2, EMbmVideoeditoruicomponentsQgn_graf_ve_novideo, 
            EMbmVideoeditoruicomponentsQgn_graf_ve_novideo_mask );

    iAudioIcon = AknIconUtils::CreateIconL( mbmPath, 
                    EMbmManualvideoeditorQgn_graf_ve_symbol_audio );

    // Video timeline icon

    AknIconUtils::CreateIconL( iVideoTrackIcon, iVideoTrackIconMask,
            mbmPath, EMbmManualvideoeditorQgn_prop_ve_file_video, 
            EMbmManualvideoeditorQgn_prop_ve_file_video_mask );

    // Audio timeline icon

    AknIconUtils::CreateIconL( iAudioTrackIcon, iAudioTrackIconMask,
            mbmPath, EMbmManualvideoeditorQgn_prop_ve_file_audio, 
            EMbmManualvideoeditorQgn_prop_ve_file_audio_mask );    

    // Audio mixing icon
    iAudioMixingIcon = AknIconUtils::CreateIconL( mbmPath3, 
            EMbmVideoeditorbitmapsMix_audio_background );

    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::ConstructL  Bitmaps OK!");

    iTransitionInfo = CTransitionInfo::NewL();

    iZoomTimer = CPeriodic::NewL( CActive::EPriorityLow );

    iVideoDisplayBox = TRect(0,0,100,100);

    iVideoDisplay = CVeiVideoDisplay::NewL( iVideoDisplayBox, this, *this );

/* Video Display components for transitioin state*/
    iTransitionDisplayRight = CVeiVideoDisplay::NewL( iVideoDisplayBox, this, *this );
    iTransitionDisplayLeft = CVeiVideoDisplay::NewL( iVideoDisplayBox, this, *this );

    iDummyCutBar = CVeiCutterBar::NewL( this, ETrue );
    iDummyCutBarLeft = CVeiCutterBar::NewL( this, ETrue );

/* IconBox */
    iEffectSymbolBox = TRect(0,0,10,10);
    iEffectSymbols = CVeiIconBox::NewL( iEffectSymbolBox, this );

    iInfoDisplay = CVeiTextDisplay::NewL( iVideoDisplayBox, this );
    iInfoDisplay->SetMopParent( this );
    iArrowsDisplay = CVeiTextDisplay::NewL( iVideoDisplayBox, this );

    SetRect( aRect );
    iBgContext = CAknsBasicBackgroundControlContext::NewL( KAknsIIDQsnBgAreaMain, Rect(), EFalse );

    iFrameTaker = CVeiFrameTaker::NewL( *this );

    iGradientBitmap = new(ELeave)CFbsBitmap;
    TRgb startColor = TRgb( 0x7a, 0xbe, 0xe7);
    TRgb endColor = TRgb( 0x00, 0x3e, 0x80 );
    TInt breadth = 30;
    ColorUtils::TBitmapOrientation bitmapOrientation = ColorUtils::EBitmapOrientationHorizontal;
    ColorUtils::CreateGradientBitmapL( *iGradientBitmap, iEikonEnv->WsSession(), breadth,
        bitmapOrientation, startColor, endColor );
    LOG(KVideoEditorLogFile, "Gradient bitmap created..");
    SetCursorLocation( ECursorOnEmptyVideoTrack );

/* Timer to keep back light on when user is not giving key events */
    iScreenLight = CVeiDisplayLighter::NewL();

    iCurrentPoint = 0;
/* Timer. Draws playhead */
    iPeriodic = CPeriodic::NewL( CActive::EPriorityStandard );
    iSeekPos = TTimeIntervalMicroSeconds( 0 );
    SetPreviewState(EStateInitializing);    

    iBlackScreen = EFalse;  

    CreateScrollBarL(aRect);                
                
    AknIconUtils::CreateIconL( iPauseBitmap, iPauseBitmapMask,
            mbmPath2, EMbmVideoeditoruicomponentsQgn_prop_ve_pause, 
            EMbmVideoeditoruicomponentsQgn_prop_ve_pause_mask );
            
    EnableDragEvents();            
    
    ActivateL();
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::ConstructL: out");
    }
    
//===========================================================================    
void CVeiEditVideoContainer::CreateScrollBarL(const TRect& aRect)
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::CreateScrollBarL: in");
    // create vertical slider control in the middle of the
    // screen in vertical direction and 10 pixels from the
    // right side of the screen

    iVerticalSlider = CVeiVerticalSlider::NewL(aRect, *this);
    iVerticalSlider->SetMinimum( KVolumeSliderMin );
    iVerticalSlider->SetMaximum( KVolumeSliderMax );
    iVerticalSlider->SetStep( KVolumeSliderStep );

    iVerticalSlider->SetPosition(0);

    iHorizontalSlider = CVeiHorizontalSlider::NewL(aRect, *this);
    iHorizontalSlider->SetMinimum(-10);
    iHorizontalSlider->SetMaximum(10);
    iHorizontalSlider->SetStep(1);

    iHorizontalSlider->SetPosition(0);

    iVerticalSlider->MakeVisible(EFalse);
    iHorizontalSlider->MakeVisible(EFalse); 
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::CreateScrollBarL: out");
    }

//===========================================================================    
CVeiEditVideoContainer::CVeiEditVideoContainer( CVedMovie& aMovie, CVeiEditVideoView& aView )
    :iView( aView ), iMovie( aMovie )
    {
    iCurrentlyProcessedIndex = -1;
    iSelectionMode = EModeNavigation;
    iVideoCursorPos = 0;
    iAudioCursorPos = 0;

    iCursorLocation = ECursorOnEmptyVideoTrack;
    iPrevCursorLocation = ECursorOnClip;
    iTakeSnapshot = EFalse;
    iSeeking = EFalse;
    iCloseStream = EFalse;
    iBackKeyPressed = EFalse;
    /* Flag to make sure that engine has finished frame before trying to get next one. */
    iFrameReady = ETrue;
    }


CVeiEditVideoContainer::~CVeiEditVideoContainer()
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::~CVeiEditVideoContainer(): In");
    if ( iMovie.MovieObserverIsRegistered( this ) )
        {
        iMovie.UnregisterMovieObserver( this );
        }

    if ( iTempVideoInfo )
        {       
        iTempVideoInfo->CancelFrame();
        delete iTempVideoInfo;
        iTempVideoInfo = NULL;      
        LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::~CVeiEditVideoContainer(): iTempVideoInfo delete OK..");
        }

    if ( iConverter )
        {
        iConverter->CancelEncoding();
        delete iConverter;
        LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::~CVeiEditVideoContainer(): iConverter delete OK..");
        }

    delete iTransitionInfo;
    delete iScreenLight;
    delete iVideoTrackIcon;
    delete iVideoTrackIconMask;
    delete iAudioTrackIcon;
    delete iAudioTrackIconMask;
    delete iAudioIcon;
    delete iAudioMixingIcon;
    delete iNoThumbnailIcon;
    delete iNoThumbnailIconMask;
    delete iGradientBitmap;
    delete iBgContext;
    delete iVideoDisplay;
    delete iTransitionDisplayRight;
    delete iTransitionDisplayLeft;
    delete iDummyCutBar;
    delete iDummyCutBarLeft;
    delete iEffectSymbols;
    delete iInfoDisplay;
    delete iArrowsDisplay;
    delete iFrameTaker;
    delete iRemConTarget;

    if ( iZoomTimer )
        {
        iZoomTimer->Cancel();
        delete iZoomTimer;
        }
    iVideoItemArray.ResetAndDestroy();
    iAudioItemArray.ResetAndDestroy();
    iVideoItemRectArray.Close();
    
    if ( iPeriodic ) 
        {
        iPeriodic->Cancel();
        delete iPeriodic;
        }
            
    if ( iTempFileName )
        {
        delete iTempFileName;
        iTempFileName = NULL;
        }
    if ( iSaveToFileName )
        {
        delete iSaveToFileName;
        iSaveToFileName = NULL;
        }
        
    delete iHorizontalSlider;    
    delete iVerticalSlider;        

    delete iPauseBitmap;
    delete iPauseBitmapMask;

    delete iCallBack;           

    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::~CVeiEditVideoContainer(): Out");
    }

// ---------------------------------------------------------
// CVeiEditVideoContainer::SizeChanged()
// Called by framework when the view size is changed
// ---------------------------------------------------------
//
void CVeiEditVideoContainer::SizeChanged()
    {
    LOGFMT2(KVideoEditorLogFile, "CVeiEditVideoContainer SizeChanged: In: (%d,%d)", Size().iWidth, Size().iHeight);

    TRect rect = Rect();

    if ( iBgContext )
        iBgContext->SetRect( rect );

    TInt audioTrackBoxTlX = -1;
    TInt audioTrackBoxSizeWidth = -1;
    TInt videoScreenSizeWidth = -1;
    TInt videoScreenSizeHeight = -1;
    TInt effectSymbolBoxTlXPortrait = -1; 
    TInt videoScreenXPortrait = -1;
    TInt videoScreenYPortrait = -1;
    TInt infoDisplayBoxSizeHeight = -1;
    TInt thumbnailStartPaneBoxTlY = -1;
    TInt thumbnailStartPaneBoxSizeWidth = -1;
    TInt thumbnailStartPaneBoxSizeHeight = -1;
    TInt thumbnailEndPaneBoxTlY = -1;
    TInt sliderThickness = -1;

    switch( rect.iBr.iX )
        {
            case 240:
            case 320:
                audioTrackBoxTlX = 25;              
                audioTrackBoxSizeWidth = 204;
                videoScreenSizeWidth = 156;
                videoScreenSizeHeight = 128;
                effectSymbolBoxTlXPortrait = 23;
                videoScreenXPortrait = 47;
                videoScreenYPortrait = 2;
                infoDisplayBoxSizeHeight = 24;
                thumbnailStartPaneBoxTlY = 12;
                thumbnailStartPaneBoxSizeWidth = 78;
                thumbnailStartPaneBoxSizeHeight = 64;
                thumbnailEndPaneBoxTlY = 12;
                sliderThickness = 12;
                break;
            case 176:
            case 208:
                audioTrackBoxTlX = 19;              
                audioTrackBoxSizeWidth = rect.iBr.iX-25;//153;
                videoScreenSizeWidth = 96;
                videoScreenSizeHeight = 78;
                effectSymbolBoxTlXPortrait = 25;
                videoScreenXPortrait = 40;
                videoScreenYPortrait = 2;
                infoDisplayBoxSizeHeight = 15;
                thumbnailStartPaneBoxTlY = 8;
                thumbnailStartPaneBoxSizeWidth = 56;
                thumbnailStartPaneBoxSizeHeight = 46;
                thumbnailEndPaneBoxTlY = 8;
                sliderThickness = 8;
                break;
            case 352:
            case 416:
                audioTrackBoxTlX = 38;
                audioTrackBoxSizeWidth = 306;
                videoScreenSizeWidth = 193;
                videoScreenSizeHeight = 158;
                effectSymbolBoxTlXPortrait = 43;
                videoScreenXPortrait = 80;
                videoScreenYPortrait = 4;
                infoDisplayBoxSizeHeight = 31;
                thumbnailStartPaneBoxTlY = 16;
                thumbnailStartPaneBoxSizeWidth = 112;
                thumbnailStartPaneBoxSizeHeight = 93;
                thumbnailEndPaneBoxTlY = 16;
                sliderThickness = 12;
                break;
            default:
                audioTrackBoxTlX = 38;
                audioTrackBoxSizeWidth = 306;
                videoScreenSizeWidth = 193;
                videoScreenSizeHeight = 158;
                effectSymbolBoxTlXPortrait = 43;
                videoScreenXPortrait = 80;
                videoScreenYPortrait = 4;
                infoDisplayBoxSizeHeight = 31;
                thumbnailStartPaneBoxTlY = 16;
                thumbnailStartPaneBoxSizeWidth = 112;
                thumbnailStartPaneBoxSizeHeight = 93;
                thumbnailEndPaneBoxTlY = 16;
                sliderThickness = 12;
                break;          
        };

    iTransitionMarkerSize.iWidth = 7;
    iTransitionMarkerSize.iHeight = 9;

    iAudioBarBox.iTl.iX = (rect.iTl.iX + 1) + iTransitionMarkerSize.iWidth / 2 + 20;
    iAudioBarBox.iTl.iY = rect.iBr.iY - 16;
    iAudioBarBox.iBr.iX = (rect.iBr.iX - 1) - iTransitionMarkerSize.iWidth / 2;
    iAudioBarBox.iBr.iY = rect.iBr.iY - 1;

    TPoint audioTrackBoxTl = TPoint(audioTrackBoxTlX, 
        STATIC_CAST(TInt, rect.iBr.iY*0.8994 ));

    TSize audioTrackBoxSize = TSize(audioTrackBoxSizeWidth, 
        STATIC_CAST(TInt, rect.iBr.iY*0.0764 )); 
    iAudioTrackBox = TRect( audioTrackBoxTl, audioTrackBoxSize );

    iAudioBarIconPos.iX = STATIC_CAST(TInt,0.01137*rect.iBr.iX);//rect.iTl.iX;
    iAudioBarIconPos.iY = iAudioTrackBox.iTl.iY;

    AknIconUtils::SetSize( iVideoTrackIcon, TSize( iAudioTrackBox.Height(), iAudioTrackBox.Height() ) );
    AknIconUtils::SetSize( iAudioTrackIcon, TSize( iAudioTrackBox.Height(), iAudioTrackBox.Height() ) );

    iAudioBarBox = iAudioTrackBox;

    iVideoBarBox.iTl.iX = iAudioBarBox.iTl.iX;
    iVideoBarBox.iTl.iY = iAudioBarBox.iTl.iY - iTransitionMarkerSize.iHeight - 16;
    iVideoBarBox.iBr.iX = iAudioBarBox.iBr.iX;
    iVideoBarBox.iBr.iY = iAudioBarBox.iTl.iY - 10;

    TPoint videoTrackBoxTl = TPoint(audioTrackBoxTl.iX,
    STATIC_CAST(TInt,rect.iBr.iY*0.7882 ));

    TSize videoTrackBoxSize = TSize(audioTrackBoxSize.iWidth, 
        STATIC_CAST(TInt, rect.iBr.iY*0.0764 )); 

    iVideoTrackBox = TRect( videoTrackBoxTl, videoTrackBoxSize );

    iVideoBarIconPos.iX = iAudioBarIconPos.iX;
    iVideoBarIconPos.iY = iVideoTrackBox.iTl.iY;

    iVideoBarBox = iVideoTrackBox;

    iBarArea.iTl.iX = rect.iTl.iX;
    iBarArea.iTl.iY = iVideoBarBox.iTl.iY;
    iBarArea.iBr.iX = rect.iBr.iX;
    iBarArea.iBr.iY = iAudioBarBox.iBr.iY;

    TBool landscape = VideoEditorUtils::IsLandscapeScreenOrientation();

    if ( landscape ) //Landscape
        {
        // clip thumbnail pane
        TInt videoScreenX = STATIC_CAST( TInt, rect.iBr.iX*0.0097 );
        TInt videoScreenY = STATIC_CAST( TInt, rect.iBr.iY*0.0139 );

        TSize videoScreenSize( videoScreenSizeWidth, videoScreenSizeHeight );

        iVideoDisplayBox = TRect( TPoint( videoScreenX, videoScreenY ), videoScreenSize );
        iVideoDisplay->SetRect( iVideoDisplayBox );

        //clip cut timeline pane
        TSize cutBarBoxSize = TSize(videoScreenSize.iWidth, 
            STATIC_CAST(TInt,rect.iBr.iY*0.09375 ));
        iDummyCutBarBox = TRect( TPoint(iVideoDisplayBox.iTl.iX, iVideoDisplayBox.iBr.iY), cutBarBoxSize );
        iDummyCutBar->SetRect( iDummyCutBarBox );

        //clip info pane
        TSize infoDisplayBoxSize = TSize( STATIC_CAST(TInt,rect.iBr.iX*0.4159),STATIC_CAST(TInt,rect.iBr.iY*0.56) );
        iInfoDisplayBox = TRect( TPoint(iVideoDisplayBox.iBr.iX+videoScreenX, iVideoDisplayBox.iTl.iY), 
            infoDisplayBoxSize );

        //clip indicator pane 
        TInt iconHeight = STATIC_CAST( TInt, rect.iBr.iY * 0.0972222222 ); 

        TPoint effectSymbolBoxTl = TPoint( iInfoDisplayBox.iTl.iX, iInfoDisplayBox.iBr.iY/*+videoScreenY*/);
        TSize effectSymbolBoxSize = TSize( STATIC_CAST(TInt,rect.iBr.iX*0.22115385), iconHeight );

        iEffectSymbolBox = TRect( effectSymbolBoxTl, effectSymbolBoxSize);
        iEffectSymbols->SetLandscapeScreenOrientation( landscape );

        //pause indicator box (for preview state)
        iPauseIconBox = TRect( effectSymbolBoxTl, TSize(iconHeight, iconHeight) );

        //slider controls
        if (iVerticalSlider)
            {
            iVerticalSliderSize = TSize(sliderThickness, iVideoDisplayBox.Height() + iDummyCutBarBox.Height());
            iVerticalSliderPoint = TPoint( rect.Width() - sliderThickness * 2, videoScreenY );
            iVerticalSlider->SetExtent( iVerticalSliderPoint, iVerticalSliderSize );
            }
        if (iHorizontalSlider)
            {
            iHorizontalSliderSize = TSize(videoScreenSize.iWidth, sliderThickness);
            iHorizontalSliderPoint = TPoint( videoScreenX, videoScreenY + videoScreenSize.iHeight + sliderThickness);
            iHorizontalSlider->SetExtent( iHorizontalSliderPoint, iHorizontalSliderSize );
            }

        //transition

        //ved_clip_thumbnail_start_pane
        TPoint thumbnailEndPaneBoxTl = TPoint(videoScreenX,videoScreenY);

        TSize thumbnailEndPaneBoxSize = TSize(STATIC_CAST(TInt, 0.3198*rect.iBr.iX),
            STATIC_CAST(TInt,rect.iBr.iY*0.3785));

        iTransitionDisplayLeftBox = TRect( thumbnailEndPaneBoxTl, thumbnailEndPaneBoxSize );
        iTransitionDisplayLeft->SetRect( iTransitionDisplayLeftBox );


        //ved_clip_thumbnail_end_pane
        TPoint thumbnailStartPaneBoxTl = TPoint(STATIC_CAST(TInt, 0.6707*rect.iBr.iX),videoScreenY);

        TSize thumbnailStartPaneBoxSize = thumbnailEndPaneBoxSize; 
        
        iTransitionDisplayRightBox = TRect( thumbnailStartPaneBoxTl, thumbnailStartPaneBoxSize );
        iTransitionDisplayRight->SetRect( iTransitionDisplayRightBox );

        //ved_transition_info_pane
        TPoint transitionArrowsBoxTl = TPoint(STATIC_CAST(TInt, 0.0866*rect.iBr.iX),
            STATIC_CAST(TInt,0.4896*rect.iBr.iY ) );

        TSize transitionArrowsBoxSize = TSize(STATIC_CAST(TInt, 0.827*rect.iBr.iX),
            STATIC_CAST(TInt,0.2848*rect.iBr.iY ) );

        iTransitionArrowsBox = TRect( transitionArrowsBoxTl, transitionArrowsBoxSize );

        TInt SlowMotionBoxTlX = STATIC_CAST( TInt, rect.iBr.iX * 0.1346 );
        TInt SlowMotionBoxTlY = STATIC_CAST( TInt, rect.iBr.iY * 0.6806 );  
        TInt SlowMotionBoxBrX = STATIC_CAST( TInt, rect.iBr.iX * 0.4423077 );
        TInt SlowMotionBoxBrY = STATIC_CAST( TInt, rect.iBr.iY * 0.7639 );
            
        iSlowMotionBox = TRect(SlowMotionBoxTlX, SlowMotionBoxTlY, SlowMotionBoxBrX, 
            SlowMotionBoxBrY);

        TInt widthAndheight = STATIC_CAST( TInt, rect.iBr.iX * 0.057692 );

        iArrowsDisplay->SetArrowSize(TSize( widthAndheight, widthAndheight ));

        // video display when cursor is on transition. 
        iVideoDisplayBoxOnTransition = TRect(TPoint((rect.iBr.iX/2) - 
                STATIC_CAST( TInt, 1.19*iTransitionDisplayRightBox.Size().iHeight)/2,
                iTransitionDisplayLeftBox.iTl.iY), 
                TSize(STATIC_CAST( TInt, 1.19*iTransitionDisplayRightBox.Size().iHeight), 
                iTransitionDisplayRightBox.Size().iHeight)); // w:108, h:91             
                
        iDummyCutBarBoxOnTransition = TRect( TPoint(iTransitionDisplayRightBox.iTl.iX,
            iTransitionDisplayRightBox.iBr.iY), TSize( iTransitionDisplayRightBox.Width(), 
            iDummyCutBarBox.Height() ));
        }
        else    //Portrait
        {
        // clip thumbnail pane
        TInt videoScreenX = videoScreenXPortrait;
        TInt videoScreenY = videoScreenYPortrait;

        TSize videoScreenSize( videoScreenSizeWidth, videoScreenSizeHeight );

        iVideoDisplayBox = TRect( TPoint( videoScreenX, videoScreenY ), videoScreenSize );
        iVideoDisplay->SetRect( iVideoDisplayBox );

        //clip cut timeline pane
        TSize cutBarBoxSize = TSize(videoScreenSize.iWidth, STATIC_CAST(TInt,rect.iBr.iY*0.0938 ));
        iDummyCutBarBox = TRect( TPoint(iVideoDisplayBox.iTl.iX, 
                        iVideoDisplayBox.iBr.iY - iVideoDisplay->GetBorderWidth()), cutBarBoxSize );
        iDummyCutBar->SetRect( iDummyCutBarBox );
        iDummyCutBarLeft->MakeVisible( EFalse );

        //clip info pane
        TPoint infoDisplayBoxTl = TPoint(STATIC_CAST(TInt, rect.iBr.iX*0.074), 
            STATIC_CAST(TInt, rect.iBr.iY*0.6598 ));
        TSize infoDisplayBoxSize = TSize(STATIC_CAST(TInt, rect.iBr.iX*0.855),
            infoDisplayBoxSizeHeight );
        iInfoDisplayBox = TRect( infoDisplayBoxTl, infoDisplayBoxSize );

        //clip indicator pane
        TInt iconWidth = STATIC_CAST( TInt, rect.iBr.iX * 0.07954545455 );

        TInt effectSymbolBoxTlX = effectSymbolBoxTlXPortrait;
        TInt effectSymbolBoxTlY = STATIC_CAST( TInt, rect.iBr.iY * 0.131944444 );
        TSize effectSymbolBoxSize = TSize(iconWidth, STATIC_CAST(TInt,rect.iBr.iY * 0.3194444444 ));

        iEffectSymbolBox = TRect( TPoint(effectSymbolBoxTlX, effectSymbolBoxTlY), effectSymbolBoxSize);
        iEffectSymbols->SetLandscapeScreenOrientation( landscape );

        //pause indicator box (for preview state)
        iPauseIconBox = TRect( iEffectSymbolBox.iTl, TSize(iconWidth, iconWidth) );

        //slider controls
        if (iVerticalSlider)
            {
            iVerticalSliderSize = TSize(sliderThickness, iVideoDisplayBox.Height() + iDummyCutBarBox.Height());
            iVerticalSliderPoint = TPoint( rect.Width() - sliderThickness * 2, videoScreenY );
            iVerticalSlider->SetExtent( iVerticalSliderPoint, iVerticalSliderSize );
            }
        if (iHorizontalSlider)
            {
            iHorizontalSliderSize = TSize(videoScreenSize.iWidth, sliderThickness);
            iHorizontalSliderPoint = TPoint( videoScreenX, videoScreenY + videoScreenSize.iHeight + sliderThickness);
            iHorizontalSlider->SetExtent( iHorizontalSliderPoint, iHorizontalSliderSize );
            }

        //transition

        //ved_clip_thumbnail_start_pane
        TPoint thumbnailStartPaneBoxTl = TPoint(STATIC_CAST(TInt, 0.6705*rect.iBr.iX),
            thumbnailStartPaneBoxTlY);

        TSize thumbnailStartPaneBoxSize = TSize(thumbnailStartPaneBoxSizeWidth,
            thumbnailStartPaneBoxSizeHeight);

        iTransitionDisplayRightBox = TRect( thumbnailStartPaneBoxTl, thumbnailStartPaneBoxSize );
        iTransitionDisplayRight->SetRect( iTransitionDisplayRightBox );

        //ved_clip_thumbnail_end_pane
        TPoint thumbnailEndPaneBoxTl = TPoint(STATIC_CAST(TInt, 0.0116*rect.iBr.iX),
            thumbnailEndPaneBoxTlY );

        TSize thumbnailEndPaneBoxSize = thumbnailStartPaneBoxSize;

        iTransitionDisplayLeftBox = TRect( thumbnailEndPaneBoxTl, thumbnailEndPaneBoxSize );
        iTransitionDisplayLeft->SetRect( iTransitionDisplayLeftBox );

        TInt SlowMotionBoxTlX = STATIC_CAST( TInt, rect.iBr.iX * 0.3 );
        TInt SlowMotionBoxTlY = STATIC_CAST( TInt, rect.iBr.iY * 0.65 );    
        TInt SlowMotionBoxBrX = STATIC_CAST( TInt, rect.iBr.iX * 0.7 );
        TInt SlowMotionBoxBrY = STATIC_CAST( TInt, rect.iBr.iY * 0.76 );

        iSlowMotionBox = TRect(SlowMotionBoxTlX, SlowMotionBoxTlY, SlowMotionBoxBrX, 
            SlowMotionBoxBrY);

        //ved_transition_info_pane
        TPoint transitionArrowsBoxTl = TPoint(STATIC_CAST(TInt, 0.0116*rect.iBr.iX),
            STATIC_CAST(TInt,0.4792*rect.iBr.iY ) );

        TSize transitionArrowsBoxSize = TSize(STATIC_CAST(TInt, 0.9773*rect.iBr.iX),
            STATIC_CAST(TInt,0.2848*rect.iBr.iY ) );

        iTransitionArrowsBox = TRect( transitionArrowsBoxTl, transitionArrowsBoxSize );


        TInt widthAndheight = STATIC_CAST( TInt, rect.iBr.iX * 0.068182 );

        iInfoDisplay->SetArrowSize(TSize( widthAndheight, widthAndheight ));
        iArrowsDisplay->SetArrowSize(TSize( widthAndheight, widthAndheight ));

        // video display when cursor is on transition. 
        iVideoDisplayBoxOnTransition = TRect( TPoint(STATIC_CAST( TInt, 0.341*rect.iBr.iX),
                iTransitionDisplayLeftBox.iTl.iY), iTransitionDisplayRightBox.Size() );

        iDummyCutBarBoxOnTransition = TRect( TPoint(iTransitionDisplayRightBox.iTl.iX,
            iTransitionDisplayRightBox.iBr.iY - iVideoDisplay->GetBorderWidth()), 
            TSize( iTransitionDisplayRightBox.Width(), iDummyCutBarBox.Height() ));
        }

    iInfoDisplay->SetRect( iInfoDisplayBox );
    iInfoDisplay->SetLandscapeScreenOrientation( landscape );
    iArrowsDisplay->SetRect(iSlowMotionBox);

    AknIconUtils::SetSize( iNoThumbnailIcon, TSize( iVideoDisplayBox.Size() ) );
    AknIconUtils::SetSize( iAudioIcon, TSize( iVideoDisplayBox.Size() ) );
    AknIconUtils::SetSize( iPauseBitmap, TSize( iPauseIconBox.Size() ), EAspectRatioNotPreserved );     

    // Update iconbox after screen rotation
    iEffectSymbols->SetRect( iEffectSymbolBox );
    if ( CursorLocation() == ECursorOnTransition && 
            iView.EditorState() != CVeiEditVideoView::EPreview )
        {
        SetCursorLocation( CursorLocation() );
        }

    iArrowsDisplay->DrawDeferred();
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer SizeChanged: Out");
    }

TInt CVeiEditVideoContainer::CurrentIndex() const
    {
    if ((iCursorLocation == ECursorOnAudio) ||
        (iCursorLocation == ECursorOnEmptyAudioTrack))
        {
        return iAudioCursorPos;
        }
    else
        {
        if ( iCursorLocation == ECursorOnEmptyVideoTrack )
            {
            return iMovie.VideoClipCount() - 1;
            }

        return iVideoCursorPos;
        }
    }

TUint CVeiEditVideoContainer::GetAndDecrementCurrentIndex()
    {
    TUint ret = 0;

    if ( iCursorLocation == ECursorOnClip )
        {
        ret = iVideoCursorPos;
        if ( iVideoCursorPos > 0 )
            {
            iVideoCursorPos--;
            }
        }
    else if ( iCursorLocation == ECursorOnAudio ) 
        {
        ret = iAudioCursorPos;
        if ( iAudioCursorPos > 0 )
            {
            iAudioCursorPos--;
            }
        }
    else
        {
        User::Panic( _L("VideoEditor"), 0 );
        }
    return ret;
    }
    
void CVeiEditVideoContainer::GetThumbAtL( const TTimeIntervalMicroSeconds& aTime )
    {
    LOGFMT(KVideoEditorLogFile, "CVeiEditVideoContainer::GetThumbAtL: In, iFrameReady:%d", iFrameReady);
    if( !iTempVideoInfo || !iFrameReady )
        {
        LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::GetThumbAtL: 1");
        return;
        }

    TSize resolution( iTempVideoInfo->Resolution() );
    TInt frameIndex = iTempVideoInfo->GetVideoFrameIndexL( aTime );

    TInt totalFrameCount = iTempVideoInfo->VideoFrameCount();
    iFrameReady = EFalse;
    if ( frameIndex > totalFrameCount )
        {
        frameIndex = totalFrameCount;
        }    

    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::GetThumbAtL: 2");        
    /* :
     check out on every phone before releasing whether videodisplay should be stopped before starting
     asynchronous GetFrameL()
     see how EStateGettingFrame is handled in SetPreviewState 
     Stopping frees memory and it is needed in memory sensible devices 
    */
    iTempVideoInfo->GetFrameL( *this, frameIndex, &resolution ); 
    SetPreviewState(EStateGettingFrame);                
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::GetThumbAtL: Out");
    }

void CVeiEditVideoContainer::StartZooming()
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::StartZooming: In");

    const TUint delay = 90000;

    if (iView.EditorState() != CVeiEditVideoView::EQuickPreview )
        {
        iZoomFactorX = 0;
        iZoomFactorY = 0;
        }
    else
        {
        iZoomFactorX = KMaxZoomFactorX;
        iZoomFactorY = KMaxZoomFactorY;
        }

    if ( iZoomTimer->IsActive() )
        {
        iZoomTimer->Cancel();
        }
    iZoomTimer->Start( delay, delay, TCallBack( CVeiEditVideoContainer::Update, this ) );

    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::StartZooming: Out");
    }                       

TKeyResponse CVeiEditVideoContainer::HandleScrollBarL (
    const TKeyEvent &   aKeyEvent,
    TEventCode          aType
    )
    {
    TKeyResponse ret = EKeyWasNotConsumed;

    if (EEventKey == aType)
    {
        switch (aKeyEvent.iCode)
        {

            case EKeyDownArrow:
            {
                iVerticalSlider->Increment();                   
                iVerticalSlider->DrawDeferred();
                ret = EKeyWasConsumed;
                break;
            }
            case EKeyUpArrow:
            {
                iVerticalSlider->Decrement();
                iVerticalSlider->DrawDeferred();
                ret = EKeyWasConsumed;
                break;
            }

            case EKeyLeftArrow:
            {
                iHorizontalSlider->Decrement();
                iHorizontalSlider->DrawDeferred();
                ret = EKeyWasConsumed;
                break;
            }
            case EKeyRightArrow:
            {
                iHorizontalSlider->Increment();
                iHorizontalSlider->DrawDeferred();
                ret = EKeyWasConsumed;
                break;
            }

            case EKeyOK:
            {
                ret = EKeyWasConsumed;
                break;
            }

            /*case EKeyLeftArrow:
            case EKeyRightArrow:
            {
                ret = EKeyWasConsumed;
                break;
            }*/

            default:
            {
                break;
            }
        }
    }

    return ret;
    }


TKeyResponse CVeiEditVideoContainer::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
    {
    if (EModeMixingAudio == iSelectionMode || EModeAdjustVolume == iSelectionMode)
        {       
        CVeiEditVideoView::TEditorState editorState = iView.EditorState();
        if (CVeiEditVideoView::EMixAudio == editorState ||
            CVeiEditVideoView::EAdjustVolume == editorState)
            {           
            TKeyResponse ret = HandleScrollBarL(aKeyEvent, aType);
            DrawDeferred();
            return ret;
            }
        }

    if ( iSeeking )
        {       
        DoUpdatePosition();
        }

    if ( aType == EEventKeyDown ) 
        {

        if ( iView.EditorState() == CVeiEditVideoView::EPreview )//large preview
            {
            LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::OfferKeyEventL preview back");
            iVideoDisplay->Stop( ETrue );
            iBackKeyPressed = ETrue;
            SetBlackScreen( EFalse );
            }

        iKeyRepeatCount = 0;

        // Shift key check. move clip - state
        TBool shiftKeyPressed = (aKeyEvent.iModifiers & EModifierShift );
        if( shiftKeyPressed )
            {
            if ( (iView.EditorState() == CVeiEditVideoView::EPreview ) &&
                (iPreviewState == EStatePlaying ))//large preview
                {
                iVideoDisplay->Stop( ETrue );

                iView.SetFullScreenSelected( EFalse );
                SetBlackScreen( EFalse );
                return EKeyWasConsumed;
                }           
            else if  (((((iCursorLocation == ECursorOnClip) && (iMovie.VideoClipCount()>1) ) && (iView.EditorState() != CVeiEditVideoView::EQuickPreview))  ||
                 ( (iCursorLocation == ECursorOnAudio) && (iMovie.AudioClipCount()>0) ) ) &&
                 iSelectionMode == EModeNavigation )
                {
                iView.ProcessCommandL( EVeiCmdEditVideoViewEditVideoMove );
                return EKeyWasConsumed;
                }
            else if (iView.EditorState() == CVeiEditVideoView::EQuickPreview)
                {
                if( iView.IsEnoughFreeSpaceToSaveL() && !iTakeSnapshot )
                    {
                    /*if (EStatePlaying == iPreviewState)   
                        {
                        PauseVideoL();                      
                        }                   
                        */
                    TakeSnapshotL();
                    return EKeyWasConsumed;
                    }
                }
            return EKeyWasNotConsumed;
            }
        //Check that it's a seeking key and we're in a suitable state. 
        if ( iPreviewState != EStatePlaying ) 
            {
            return EKeyWasNotConsumed;
            }

        iSeekPos = iVideoDisplay->PositionL();

        return EKeyWasConsumed;
        }  
    else if ( aType == EEventKeyUp ) 
        {
        iBackKeyPressed = EFalse;

        if ( (iView.EditorState() == CVeiEditVideoView::EPreview ) &&
            (iPreviewState == EStatePaused ))//large preview
            {
            LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::OfferKeyEventL preview back"); 
            iVideoDisplay->Stop( ETrue );
            iBackKeyPressed = ETrue;
            SetBlackScreen( EFalse );
            }

             
        if ( iSeeking == EFalse )
            {
            return EKeyWasNotConsumed;
            }
        iLastPosition = iSeekPos;

        iSeeking = EFalse;

        if ( (iPreviewState == EStatePaused || iPreviewState == EStateStopped) && 
             (iLastKeyCode == EKeyLeftArrow || iLastKeyCode == EKeyRightArrow) )
            {
            GetThumbAtL( iSeekPos );            
            return EKeyWasConsumed;
            }
        else if ( iPreviewState == EStatePlaying )
            {
            if ( iTempVideoInfo && (iSeekPos >= iTempVideoInfo->Duration().Int64()) )
                {
                iVideoDisplay->Stop( EFalse );
                }
            else
                {
                iVideoDisplay->SetPositionL( iSeekPos );
                iVideoDisplay->ShowBlackScreen();
                DrawDeferred();
                if ( iTempVideoInfo && !iFrameReady)
                    {                               
                    iTempVideoInfo->CancelFrame();
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
        iKeyRepeatCount++;  

        if( iBackKeyPressed )
            {
            iView.SetFullScreenSelected( EFalse );
            }

        if ( iView.EditorState() == CVeiEditVideoView::EPreview )//large preview
            {
            LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::OfferKeyEventL preview back");
            iVideoDisplay->Stop( ETrue );
            iBackKeyPressed = ETrue;
            SetBlackScreen( EFalse );   
            }
       
        switch ( aKeyEvent.iCode )
            {               
            case EKeyOK:
                {
                if ( iBackKeyPressed  )
                    {
                    return EKeyWasNotConsumed;
                    }
                 if ( iView.EditorState() == CVeiEditVideoView::EPreview )
                     {
                     iView.HandleCommandL( EAknSoftkeyBack );
                     return EKeyWasConsumed;
                     }

                if ( ( iCursorLocation == ECursorOnClip && iMovie.VideoClipCount() == 0 ) ||
                        ( iCursorLocation == ECursorOnEmptyVideoTrack ) ) 
                    {
                    iView.Popup()->ShowInsertStuffPopupList();
                    }
                else if ( ( iSelectionMode == EModeRecordingSetStart ) ||
                          ( iSelectionMode == EModeRecording ) ||
                          ( iSelectionMode == EModeRecordingPaused ) )
                    {
                    return EKeyWasConsumed;
                    }
                else if ( ( iCursorLocation == ECursorOnAudio && iMovie.AudioClipCount() == 0 ) ||
                        ( iCursorLocation == ECursorOnEmptyAudioTrack  ) ) 
                    {
                    iView.Popup()->ShowInsertAudioPopupList();
                    }
                else if ( ( iCursorLocation == ECursorOnClip || iCursorLocation == ECursorOnAudio )
                    && ( iSelectionMode == EModeMove ) )
                    {
                    iView.HandleCommandL( EAknSoftkeyOk );
                    }

                else if (iCursorLocation == ECursorOnClip && iSelectionMode == EModeDuration)
                    {
                    iView.HandleCommandL( EAknSoftkeyOk );  
                    }
                else if( iSelectionMode == EModeSlowMotion && iCursorLocation == ECursorOnClip )
                    {
                    SetSelectionMode( CVeiEditVideoContainer::EModeSlowMotion ); 
                    iView.HandleCommandL( EAknSoftkeyOk );
                    iInfoDisplay->SetSlowMotionOn( EFalse );
                    iInfoDisplay->SetLayout( CVeiTextDisplay::ENameAndDuration );

                    ArrowsControl();
                    }
                else if ( iSelectionMode == EModeDuration && iCursorLocation == ECursorOnAudio )
                    {
                    iView.HandleCommandL( EAknSoftkeyOk );
                    }

                else
                    {
                    if (EStateGettingFrame != iPreviewState && EStateBuffering != iPreviewState &&
                        EStateTerminating != iPreviewState && EStateOpening != iPreviewState)
                        {
                        iView.ProcessCommandL( EVeiCmdEditVideoViewContainerShowMenu );     
                        }               
                    }
                    
                return EKeyWasConsumed;
                }// case EKeyOk
            
            case EKeyRightArrow:
                {
                if ( iView.EditorState() == CVeiEditVideoView::EQuickPreview )
                    {
                    if ( (iKeyRepeatCount > 2)  && (iSeeking == EFalse) )
                        {
                        if ( iPreviewState == EStatePlaying )
                            {
                            iLastPosition = iVideoDisplay->PositionL();
                            iSeekPos = iVideoDisplay->PositionL();
                            }

                        iVideoDisplay->PauseL();
                        if (iPeriodic)
                            {                           
                            iPeriodic->Cancel();                                
                            }
                        iSeeking = ETrue;
                        iKeyRepeatCount = 0;                
                        }

                    if ( iSeeking &&( iPreviewState == EStateStopped ) ||
                                    ( iPreviewState == EStatePlaying ) || 
                                    ( iPreviewState == EStatePaused ) )
                        {
                        TInt adjustment = TimeIncrement( iKeyRepeatCount );

                        TInt64 newPos = iSeekPos.Int64() + adjustment;

                        if ( iTempVideoInfo && (newPos > iTempVideoInfo->Duration().Int64()) )
                            {
                            newPos = iTempVideoInfo->Duration().Int64();
                            }

                        iSeekPos = TTimeIntervalMicroSeconds( newPos );

                        iView.DoUpdateEditNaviLabelL();
                        return EKeyWasConsumed;
                        }
                    }

                if ( iView.EditorState() != CVeiEditVideoView::EEdit || iBackKeyPressed )
                    {
                    return EKeyWasNotConsumed;
                    }
                if ( iCursorLocation == ECursorOnClip )
                    {
                    //SM - RIGHT = MORE
                    if((iSelectionMode == EModeSlowMotion) && (iCursorLocation == ECursorOnClip))
                        {
                        if( iSlowMotionValue < 1000)
                            {
                            iVideoDisplay->SetFrameIntervalL( -25 );
                            iSlowMotionValue = iSlowMotionValue + 50;
                            iMovie.VideoClipSetSpeed( CurrentIndex(), iSlowMotionValue );
                            iArrowsDisplay->SetSlowMotionPreset( iSlowMotionValue / 10);
                            ArrowsControl();            
                            }
                        return EKeyWasConsumed;
                        }

                    if ( iSelectionMode == EModeNavigation )
                        {
                        if ( iMovie.VideoClipCount() > 0 ) 
                            {
                            ++iVideoCursorPos;
                            SetCursorLocation( ECursorOnTransition );
                            }
                        }
                    else
                        {
                        if ( ( iSelectionMode == EModeMove ) && ( iVideoCursorPos  < iMovie.VideoClipCount() - 1 ) ) 
                            {
                            TInt oldplace = iVideoCursorPos;
                            ++iVideoCursorPos;
                            iMovie.VideoClipSetIndex( oldplace, iVideoCursorPos );
                            }
                        }

                    if (iSelectionMode == EModeDuration) 
                        {
                        CVedVideoClipInfo* info = iMovie.VideoClipInfo( iVideoCursorPos );

                        TInt64 newDurationInt = iMovie.VideoClipEditedDuration( iVideoCursorPos ).Int64() + TimeIncrement(iKeyRepeatCount);

                        if (info->Class() == EVedVideoClipClassGenerated) 
                            {
                            if (info->Generator()->Uid() == KUidTitleClipGenerator) 
                                {
                                CVeiTitleClipGenerator* generator = STATIC_CAST(CVeiTitleClipGenerator*, info->Generator());
                                generator->SetDuration(TTimeIntervalMicroSeconds(newDurationInt));
                                }
                            else if (info->Generator()->Uid() == KUidImageClipGenerator) 
                                {
                                CVeiImageClipGenerator* generator = STATIC_CAST(CVeiImageClipGenerator*, info->Generator());
                                generator->SetDuration(TTimeIntervalMicroSeconds(newDurationInt));
                                }
                            }
                        }

                    } // if ( iCursorLocation == ECursorOnClip )
                    
                else if ( iCursorLocation == ECursorOnTransition )
                    {
                    if ( iVideoCursorPos < iMovie.VideoClipCount() )
                        {   
                        SetCursorLocation( ECursorOnClip );
                        }
                    else
                        {
                        SetCursorLocation( ECursorOnEmptyVideoTrack );  
                        }
                    }
                else if ( iCursorLocation == ECursorOnAudio )
                    {
                    if ( iSelectionMode == EModeNavigation )
                        {
                        if ( iAudioCursorPos < iMovie.AudioClipCount() - 1 ) 
                            {
                            ++iAudioCursorPos;
                            SetCursorLocation( ECursorOnAudio );
                            }
                        else
                            {
                            SetCursorLocation( ECursorOnEmptyAudioTrack );  
                            }
                        }
                    else if ( iSelectionMode == EModeMove ) 
                        {
                        return MoveAudioRight();
                        }
                    else if (iSelectionMode == EModeDuration) 
                        {

                        TTimeIntervalMicroSeconds clipCutOutTime = iMovie.AudioClipCutOutTime( iAudioCursorPos );   

                        TInt64 newEndTimeInt = clipCutOutTime.Int64() + TimeIncrement(iKeyRepeatCount);

                        if (iAudioCursorPos < (iMovie.AudioClipCount() - 1))
                            {
                            TInt64 nextStartTimeInt = iMovie.AudioClipStartTime( iAudioCursorPos + 1 ).Int64();
                            TInt64 currentEndTimeInt = iMovie.AudioClipEndTime( iAudioCursorPos ).Int64() + TimeIncrement(iKeyRepeatCount);

                            if ( currentEndTimeInt > nextStartTimeInt)
                                {
                                newEndTimeInt = nextStartTimeInt - iMovie.AudioClipStartTime( iAudioCursorPos ).Int64();
                                }
                            }
                        CVedAudioClipInfo* audioclipinfo = iMovie.AudioClipInfo( iAudioCursorPos );
                        if (newEndTimeInt > audioclipinfo->Duration().Int64() )                 
                            {
                            newEndTimeInt = audioclipinfo->Duration().Int64();
                            }
                        iMovie.AudioClipSetCutOutTime( iAudioCursorPos, TTimeIntervalMicroSeconds( newEndTimeInt ) );                                           
                        }
                    } // else if ( iCursorLocation == ECursorOnAudio )
                    DrawDeferred();

                    return EKeyWasConsumed;
                } // case EKeyRightArrow
            case EKeyLeftArrow:
                {
                if ( iView.EditorState() == CVeiEditVideoView::EQuickPreview )
                    {
                    iKeyRepeatCount++;

                    if ( (iKeyRepeatCount > 2)  && (iSeeking == EFalse) )
                        {
                    
                        if ( iPreviewState == EStatePlaying )
                            {
                            iLastPosition = iVideoDisplay->PositionL();
                            iSeekPos = iVideoDisplay->PositionL();
                            }                                                   
                        
                        iVideoDisplay->PauseL();
                        if (iPeriodic)
                            {                           
                            iPeriodic->Cancel();                            
                            }
                        iSeeking = ETrue;
                                                
                        iKeyRepeatCount = 0;                
                        }

                    if ( iSeeking&&( iPreviewState == EStateStopped ) ||
                        ( iPreviewState == EStatePlaying ) || 
                        ( iPreviewState == EStatePaused ) )
                        {
                        TInt adjustment = TimeIncrement( iKeyRepeatCount );

                        TInt64 newPos = iSeekPos.Int64() - adjustment;
                        if ( newPos < 0 ) 
                            {
                            newPos = 0;
                            }
                        iSeekPos = TTimeIntervalMicroSeconds( newPos ); 
                        
                        iView.DoUpdateEditNaviLabelL();
                        return EKeyWasConsumed;
                        }
                    }
                 
                if ( iView.EditorState() != CVeiEditVideoView::EEdit || iBackKeyPressed  )
                    {
                    return EKeyWasNotConsumed;
                    }               
                if ( iCursorLocation == ECursorOnClip )
                    {
                    // SM - LEFT = LESS
                    if(( iSelectionMode == EModeSlowMotion) && (iCursorLocation == ECursorOnClip))
                        {   
                        
                        if( iSlowMotionValue > 250)
                            {
                            iVideoDisplay->SetFrameIntervalL( 25 );
                            iSlowMotionValue = iSlowMotionValue - 50; 
                            iMovie.VideoClipSetSpeed( CurrentIndex(), iSlowMotionValue );
                            iArrowsDisplay->SetSlowMotionPreset( iSlowMotionValue / 10);
                            ArrowsControl();
                            }
                        return EKeyWasConsumed;
                        }
                    

                    if ( iSelectionMode == EModeNavigation )
                        {
                        if ( iMovie.VideoClipCount() > 0 ) 
                            {
                            SetCursorLocation( ECursorOnTransition );
                            }
                        }
                    else
                        {
                        if ( ( iSelectionMode == EModeMove ) && ( iVideoCursorPos > 0 ) ) 
                            {
                            TInt oldplace = iVideoCursorPos;
                            iVideoCursorPos--;
                            iMovie.VideoClipSetIndex( oldplace, iVideoCursorPos );
                            }
                        }

                    if (iSelectionMode == EModeDuration) 
                        {
                        TInt64 newDurationInt = iMovie.VideoClipEditedDuration(iVideoCursorPos).Int64() - TimeIncrement(iKeyRepeatCount);

                        if (newDurationInt < 1000000)
                            {
                            newDurationInt = 1000000;
                            }

                        CVedVideoClipInfo* info = iMovie.VideoClipInfo(iVideoCursorPos);
                        if (info->Class() == EVedVideoClipClassGenerated) 
                            {
                            if (info->Generator()->Uid() == KUidTitleClipGenerator) 
                                {
                                CVeiTitleClipGenerator* generator = STATIC_CAST(CVeiTitleClipGenerator*, info->Generator());
                                generator->SetDuration(TTimeIntervalMicroSeconds(newDurationInt));
                                }
                            else if (info->Generator()->Uid() == KUidImageClipGenerator) 
                                {
                                CVeiImageClipGenerator* generator = STATIC_CAST(CVeiImageClipGenerator*, info->Generator());
                                generator->SetDuration(TTimeIntervalMicroSeconds(newDurationInt));
                                }
                            }
                        }

                    }
                else if ( iCursorLocation == ECursorOnTransition )
                    {
                    if ( iVideoCursorPos > 0 )
                        {   
                        --iVideoCursorPos;
                        SetCursorLocation( ECursorOnClip );
                        }
                    }
                else if ( iCursorLocation == ECursorOnAudio )
                    {
                    if ( iSelectionMode == EModeNavigation )
                        {
                        if ( iAudioCursorPos > 0 ) 
                            {
                            iAudioCursorPos--;
                            SetCursorLocation( ECursorOnAudio );
                            }
                        }
                    else if (iSelectionMode == EModeMove) 
                        {
                        return MoveAudioLeft();
                        }
                    else if (iSelectionMode == EModeDuration) 
                        {

                        TInt64 newDurationInt = iMovie.AudioClipCutOutTime( iAudioCursorPos ).Int64() - TimeIncrement(iKeyRepeatCount);                     
                        if (newDurationInt < 1000000)
                            {
                            newDurationInt = 1000000;
                            }
                        TInt64 newEndTimeInt = iMovie.AudioClipStartTime( iAudioCursorPos ).Int64() + newDurationInt;
                        for (TInt i = iMovie.VideoClipCount() - 1; i >= 0; i--)
                            {
                            if ( ( iMovie.AudioClipEndTime( iAudioCursorPos ) > iMovie.VideoClipEndTime( i ) ) &&
                                TTimeIntervalMicroSeconds(newEndTimeInt) < iMovie.VideoClipEndTime( i ) )
                                {
                                newDurationInt = iMovie.VideoClipEndTime( i ).Int64() - iMovie.AudioClipStartTime( iAudioCursorPos ).Int64();
                                break;
                                }
                            }
                        if ( newDurationInt < ( iMovie.AudioClipCutInTime( iAudioCursorPos ).Int64() ) )
                            {
                            newDurationInt = iMovie.AudioClipCutInTime( iAudioCursorPos ).Int64();
                            }
                        iMovie.AudioClipSetCutOutTime( iAudioCursorPos, TTimeIntervalMicroSeconds( newDurationInt ) );              
                        }
                    }

                else if ( iCursorLocation == ECursorOnEmptyVideoTrack )
                    {
                    if ( iMovie.VideoClipCount() > 0 )
                        {
                        SetCursorLocation( ECursorOnTransition );
                        }
                    }
                else if ( iCursorLocation == ECursorOnEmptyAudioTrack )
                    {
                    if ( iMovie.AudioClipCount() > 0 )
                        {
                        SetCursorLocation( ECursorOnAudio );
                        }
                    }

                DrawDeferred();
                return EKeyWasConsumed;
                } // EKeyLeftArrow
            
            case EKeyUpArrow:
            case EStdKeyIncVolume:
                {           
                if ( iView.EditorState() != CVeiEditVideoView::EEdit || iBackKeyPressed )
                    {
                    if ( iView.EditorState() == CVeiEditVideoView::EQuickPreview )
                        {
                        iVideoDisplay->OfferKeyEventL( aKeyEvent, aType );
                        return EKeyWasConsumed;
                        }
                    return EKeyWasNotConsumed;
                    }
                if ( ((iCursorLocation==ECursorOnAudio) || (iCursorLocation==ECursorOnEmptyAudioTrack) )
                        && (iSelectionMode == EModeNavigation ) ) 
                    {
                    SetCursorLocation( iPrevCursorLocation );
                    DrawDeferred();
                    return EKeyWasConsumed;
                    }
                else if ( iCursorLocation == ECursorOnTransition ) 
                    {
                    iView.HandleCommandL( EVeiCmdEditVideoViewTransitionKeyUp );
                    return EKeyWasConsumed;
                    }
                else
                    {
                    return EKeyWasNotConsumed;
                    }
                }
            case EKeyDownArrow:
            case EStdKeyDecVolume:
                {
                if ( iView.EditorState() != CVeiEditVideoView::EEdit || iBackKeyPressed )
                    {
                    if ( iView.EditorState() == CVeiEditVideoView::EQuickPreview )
                        {
                        iVideoDisplay->OfferKeyEventL( aKeyEvent, aType );
                        return EKeyWasConsumed;
                        }
                    return EKeyWasNotConsumed;
                    }
                if ( iCursorLocation == ECursorOnTransition ) 
                    {
                    iView.HandleCommandL( EVeiCmdEditVideoViewTransitionKeyDown );
                    return EKeyWasConsumed;
                    }
                else if ( (iCursorLocation != ECursorOnAudio )&& (iSelectionMode == EModeNavigation) &&
                        (iCursorLocation != ECursorOnEmptyAudioTrack ) ) 
                    {
                    iPrevCursorLocation = iCursorLocation;
                    SetCursorLocation( ECursorOnAudio );
                    DrawDeferred();
                    return EKeyWasConsumed;
                    }
                else
                    {
                    return EKeyWasNotConsumed;
                    }
                }
            case EKeyBackspace:     //Clear 0x08
                {
                if ( iView.EditorState() != CVeiEditVideoView::EEdit || iBackKeyPressed )
                    {
                    return EKeyWasNotConsumed;
                    }
                if ( ( ( iCursorLocation == ECursorOnClip ) || ( iCursorLocation == ECursorOnAudio ) )
                    && ( ( iMovie.VideoClipCount() > 0 ) || ( iMovie.AudioClipCount() > 0 ) ) && ( iSelectionMode == EModeNavigation ) )
                    {
                    iView.ProcessCommandL( EVeiCmdEditVideoViewEditVideoRemove );
                    return EKeyWasConsumed;
                    }
                else if ((iCursorLocation == ECursorOnTransition) && (iSelectionMode == EModeNavigation))
                    {
                    iView.ProcessCommandL( EVeiCmdEditVideoViewTransitionNone );
                    }
                else if ( ( iSelectionMode == EModeSlowMotion ))
                    {
                    return EKeyWasNotConsumed;
                    }
                return EKeyWasNotConsumed;
                }
            case EKeyYes:       //Send 63586
                {
                if ( iView.EditorState() != CVeiEditVideoView::EEdit || iBackKeyPressed  )
                    {
                    return EKeyWasNotConsumed;
                    }
                if ( iSelectionMode == EModeNavigation )
                    {
                    iView.SetSendKey( ETrue );
                    iView.ProcessCommandL( EVeiCmdSendMovie );  
                    return EKeyWasConsumed;
                    }
                break;
                }
            default:
                {
                return EKeyWasNotConsumed;
                }
            }
        }
    return EKeyWasNotConsumed;
    }


TKeyResponse CVeiEditVideoContainer::MoveAudioLeft()
    {
    TInt i;

    TInt index = iAudioCursorPos;

    if ( iAudioCursorPos > 0 )
        {
        TTimeIntervalMicroSeconds startTime = iMovie.AudioClipStartTime( iAudioCursorPos ); //second
        TTimeIntervalMicroSeconds prevEndTime = iMovie.AudioClipEndTime( iAudioCursorPos-1 ); //first
                    
        if ( startTime == prevEndTime )
            {
            TInt newIndex = iAudioCursorPos - 1;
            while ( newIndex > 0 )
                {
                TTimeIntervalMicroSeconds newNextStartTime = iMovie.AudioClipStartTime( newIndex ); //first
                TTimeIntervalMicroSeconds newPrevEndTime = iMovie.AudioClipEndTime( newIndex - 1);

                if ( newPrevEndTime == newNextStartTime )
                    {
                    newIndex--;
                    }
                else
                    {
                    break;
                    }
                }
            if ( ( newIndex == 0 ) && ( iMovie.AudioClipStartTime( 0 ) == TTimeIntervalMicroSeconds(0) ) )
                {
                return EKeyWasConsumed;
                }

            TInt64 newEndTimeInt = iMovie.AudioClipStartTime( newIndex ).Int64();   //first

            TInt64 newStartTimeInt = newEndTimeInt - iView.OriginalAudioDuration().Int64();

            TInt64 newCutOutTimeInt = (newEndTimeInt - newStartTimeInt) + iMovie.AudioClipCutInTime( index ).Int64();

            if (newStartTimeInt < 0)
                {
                newStartTimeInt = 0;
                }

            if ( newIndex > 0 )
                {
                TInt64 newPrevEndTimeInt = iMovie.AudioClipEndTime( newIndex - 1 ).Int64();
                if ( newStartTimeInt < newPrevEndTimeInt )
                    {
                    newStartTimeInt = newPrevEndTimeInt;
                    }
                }

            if ( (newCutOutTimeInt+newStartTimeInt) > iMovie.AudioClipStartTime( index - 1 ).Int64() )
                {
                TInt64 audioStartTime = iMovie.AudioClipStartTime( index - 1 ).Int64();
                newCutOutTimeInt = (audioStartTime - newStartTimeInt) + iMovie.AudioClipCutInTime( index ).Int64();
                }

            iMovie.AudioClipSetStartTime( index,TTimeIntervalMicroSeconds( newStartTimeInt ) );
            iMovie.AudioClipSetCutOutTime( newIndex, TTimeIntervalMicroSeconds( newCutOutTimeInt ) );

            iAudioCursorPos = newIndex;

            DrawDeferred();

            return EKeyWasConsumed;
            }
        }

    TTimeIntervalMicroSeconds audioclipstart =  iMovie.AudioClipStartTime( index );
    TTimeIntervalMicroSeconds audioclipend =    iMovie.AudioClipEndTime( index );
    TInt64 audioclipeditedduration =            iMovie.AudioClipEditedDuration( index ).Int64();

    TInt64 newStartTimeInt = audioclipstart.Int64() - TimeIncrement( iKeyRepeatCount );

    if ( newStartTimeInt < 0 )
        {
        newStartTimeInt = 0;
        }

    for ( i = iMovie.VideoClipCount() - 1; i >= 0; i-- )
        {
        TTimeIntervalMicroSeconds endtime = iMovie.VideoClipEndTime( i );

        if ( ( audioclipstart > endtime ) && TTimeIntervalMicroSeconds(newStartTimeInt) < endtime )
            {
            newStartTimeInt = endtime.Int64();
            break;
            }
        }

    TInt64 newEndTimeInt = newStartTimeInt + audioclipeditedduration;

    for (i = iMovie.VideoClipCount() - 1; i >= 0; i-- )
        {
        if ((audioclipend > iMovie.VideoClipEndTime(i)) && TTimeIntervalMicroSeconds(newEndTimeInt) < iMovie.VideoClipEndTime(i))
            {
            newStartTimeInt = iMovie.VideoClipEndTime(i).Int64() - audioclipeditedduration;
            break;
            }
        }

    if (iAudioCursorPos > 0)
        {
        TInt64 prevEndTimeInt = iMovie.AudioClipEndTime( iAudioCursorPos - 1 ).Int64();
                        
        if (newStartTimeInt < prevEndTimeInt)
            {
            newStartTimeInt = prevEndTimeInt;
            }
        }
    iMovie.AudioClipSetStartTime( index, TTimeIntervalMicroSeconds(newStartTimeInt) );
    iAudioCursorPos = index;
    return EKeyWasConsumed;
    }


TKeyResponse CVeiEditVideoContainer::MoveAudioRight()
    {
    TInt i;
    TInt index = iAudioCursorPos;

    if ( iAudioCursorPos < ( iMovie.AudioClipCount() - 1 ) )
        {
        TTimeIntervalMicroSeconds endTime = iMovie.AudioClipEndTime( iAudioCursorPos );
        TTimeIntervalMicroSeconds nextStartTime = iMovie.AudioClipStartTime( iAudioCursorPos+1 );

        if (endTime == nextStartTime)
            {
            TInt newIndex = iAudioCursorPos + 1;
            while ( newIndex < ( iMovie.AudioClipCount() - 1 ) )
                {
                TTimeIntervalMicroSeconds newPrevEndTime = iMovie.AudioClipEndTime( newIndex );
                TTimeIntervalMicroSeconds newNextStartTime = iMovie.AudioClipStartTime( newIndex+1 );

                if (newPrevEndTime == newNextStartTime)
                    {
                    newIndex++;
                    }
                else
                    {
                    break;
                    }
                }

            TInt64 newStartTimeInt = iMovie.AudioClipEndTime( newIndex ).Int64();
            TInt64 newEndTimeInt = newStartTimeInt + iView.OriginalAudioDuration().Int64();

            TInt64 newCutOutTimeInt = (newEndTimeInt - newStartTimeInt) + iMovie.AudioClipCutInTime( index ).Int64();

            if ( newIndex < ( iMovie.AudioClipCount()-1 ) )
                {
                TInt64 newNextStartTimeInt = iMovie.AudioClipStartTime(newIndex + 1).Int64();
                if ( newEndTimeInt > newNextStartTimeInt )
                    {
                    newEndTimeInt = newNextStartTimeInt;
                    }

                if ( (newCutOutTimeInt+newStartTimeInt) > iMovie.AudioClipStartTime( newIndex + 1 ).Int64() )
                    {
                    TInt64 audioStartTime = iMovie.AudioClipStartTime( newIndex + 1 ).Int64();
                    newCutOutTimeInt = audioStartTime - newStartTimeInt;
                    }
                }

            iMovie.AudioClipSetStartTime( index,TTimeIntervalMicroSeconds( newStartTimeInt ) );
            iMovie.AudioClipSetCutOutTime( newIndex, TTimeIntervalMicroSeconds( newCutOutTimeInt ) );

            iAudioCursorPos = newIndex;

            DrawDeferred();

            return EKeyWasConsumed;
            }
        }

    TInt64 newStartTimeInt = iMovie.AudioClipStartTime( iAudioCursorPos ).Int64() + TimeIncrement( iKeyRepeatCount );
                    
    for ( i = 0; i < iMovie.VideoClipCount(); i++ )
        {
        TTimeIntervalMicroSeconds endtime = iMovie.VideoClipEndTime( i );
        TTimeIntervalMicroSeconds audioclipstart =  iMovie.AudioClipStartTime( index );

        if ( ( audioclipstart < endtime ) && TTimeIntervalMicroSeconds(newStartTimeInt) > endtime )
            {
            newStartTimeInt = endtime.Int64();
            break;
            }
        }

    TInt64 audioclipeditedduration = iMovie.AudioClipEditedDuration( index ).Int64();

    TInt64 newEndTimeInt = newStartTimeInt + audioclipeditedduration;
    TTimeIntervalMicroSeconds audioclipend = iMovie.AudioClipEndTime( index );

    for (i = 0; i < iMovie.VideoClipCount(); i++)
        {
        if ( ( audioclipend < iMovie.VideoClipEndTime(i)) && 
            TTimeIntervalMicroSeconds( newEndTimeInt ) > iMovie.VideoClipEndTime( i ) )
            {
            newStartTimeInt = iMovie.VideoClipEndTime(i).Int64() - audioclipeditedduration;
            break;
            }
        }

    if (iAudioCursorPos < (iMovie.AudioClipCount() - 1))
        {
        newEndTimeInt = newStartTimeInt + audioclipeditedduration;
        TInt64 nextStartTimeInt = iMovie.AudioClipStartTime( iAudioCursorPos + 1 ).Int64();
                        
        if (newEndTimeInt > nextStartTimeInt)
            {
            newStartTimeInt -= (newEndTimeInt - nextStartTimeInt);
            }
        }
    iMovie.AudioClipSetStartTime( index, TTimeIntervalMicroSeconds(newStartTimeInt) );
    iAudioCursorPos = index;
    return EKeyWasConsumed;
    }


TInt CVeiEditVideoContainer::TimeIncrement(TInt aKeyCount) const
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
    else if ( aKeyCount < 19 )
        {
        return 1000000;
        }
    else
        {
        return 5000000;
        }   
    }


TTimeIntervalMicroSeconds CVeiEditVideoContainer::TotalLength()
    {
    TTimeIntervalMicroSeconds duration = TTimeIntervalMicroSeconds(0);

    if ( iVideoDisplay )
        {
        duration = iVideoDisplay->TotalLengthL();
        }

    return duration;
    }
    
// ---------------------------------------------------------
// CVeiEditVideoContainer::CountComponentControls() const
// ---------------------------------------------------------
//
TInt CVeiEditVideoContainer::CountComponentControls() const
    {
    if (EModeMixingAudio == iSelectionMode || EModeAdjustVolume == iSelectionMode)
        {
        return 9; // return nbr of controls inside this container   
        }
    else
        {       
        return 10; // return nbr of controls inside this container
        }
    }

// ---------------------------------------------------------
// CVeiEditVideoContainer::ComponentControl(TInt aIndex) const
// ---------------------------------------------------------
//
CCoeControl* CVeiEditVideoContainer::ComponentControl( TInt aIndex ) const
    {
    switch ( aIndex )
        {
        case 0:
            return iVideoDisplay;
        case 1:
            return iInfoDisplay;
        case 2:
            return iDummyCutBarLeft;
        case 3:         
            return iTransitionDisplayLeft;
        case 4:
            return iTransitionDisplayRight;
        case 5:
            return iEffectSymbols;
        case 6:
            return iArrowsDisplay;
        case 7:         
            return iVerticalSlider; 
        case 8:
            return iHorizontalSlider;
        case 9:
            // this one is not used in modes EModeMixingAudio and EModeAdjustVolume
            if (EModeMixingAudio != iSelectionMode && EModeAdjustVolume != iSelectionMode)
                {       
                return iDummyCutBar;
                }
            else
                {
                return NULL;
                }
        default: 
            return NULL;
        }
    }

void CVeiEditVideoContainer::SaveSnapshotL()
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::SaveSnapshotL(): in");

    //first we have to encode bitmap
    // get encoder types 
    RImageTypeDescriptionArray imageTypes; 
    iConverter->GetEncoderImageTypesL( imageTypes );    

    CleanupStack::PushL( TCleanupItem( CleanupRArray, &imageTypes ) );

    TInt selectedIdx = 0;

    _LIT( KEncoderType, "JPEG" ); // encoder type for image conversion

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
    iConverter->StartToEncodeL( newname, 
        imageTypes[selectedIdx]->ImageType(), imageTypes[selectedIdx]->SubType() );

    /*
    if (iProgressDialog)
        {
        delete iProgressDialog;
        iProgressDialog = NULL; 
        }
   
    iProgressDialog = 
           new (ELeave) CAknProgressDialog(REINTERPRET_CAST(CEikDialog**, 
          &iProgressDialog), ETrue);
    iProgressDialog->SetCallback(this);
    iProgressDialog->ExecuteDlgLD( R_VEI_PROGRESS_NOTE );


    HBufC* stringholder = StringLoader::LoadLC( R_VEI_PROGRESS_NOTE_SAVING_IMAGE, iEikonEnv );      
    iProgressDialog->SetTextL( *stringholder );
    CleanupStack::PopAndDestroy( stringholder );

    iProgressDialog->GetProgressInfoL()->SetFinalValue(100);
    */
    
    CleanupStack::PopAndDestroy( &imageTypes ); 
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::SaveSnapshotL(): out");
    }


void CVeiEditVideoContainer::SetFinishedStatus( TBool aStatus )
    {
    iFinished = aStatus;
    iCurrentPoint = 0;
    DrawPlayHead();
    }

TInt CVeiEditVideoContainer::Update( TAny* aThis )
    {
    STATIC_CAST( CVeiEditVideoContainer*, aThis )->DoUpdate();
    return 42;
    }

void CVeiEditVideoContainer::DoUpdate()
    {
    if (iView.EditorState() != CVeiEditVideoView::EQuickPreview )
        {
        iZoomFactorX++;
        iZoomFactorY++;
        if ( iZoomFactorX > KMaxZoomFactorX )
            {
            iZoomFactorX = KMaxZoomFactorX;
            }
        if ( iZoomFactorY > KMaxZoomFactorY )
            {
            LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::DoUpdate: Zooming completed");
            iZoomTimer->Cancel();
            }
        }
    else
        {
        iZoomFactorX--;
        iZoomFactorY--;
        if ( iZoomFactorX < 0 )
            {
            iZoomFactorX = 0;
            LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::DoUpdate: Zooming completed");
            iZoomTimer->Cancel();
            }
        if ( iZoomFactorY < 0 )
            {
            iZoomFactorY = 0;
            }
        }

    if( iSelectionMode != EModePreview )
        {
        DrawTrackBoxes();
        }
    }

TInt CVeiEditVideoContainer::UpdatePosition( TAny* aThis )
    {   
    STATIC_CAST( CVeiEditVideoContainer*, aThis )->DoUpdatePosition();          
    return 42;
    }   

void CVeiEditVideoContainer::DoUpdatePosition()
    {
    
    TUint time = static_cast<TInt32>(PlaybackPositionL().Int64() / 1000);
    
    //LOGFMT(KVideoEditorLogFile, "CVeiEditVideoContainer::DoUpdatePosition(): 1, time:%d", time);    

/** Check if playhead needs to be drawn again*/
    TInt64 barDuration = iMovie.Duration().Int64();

    if (barDuration < 30000000)
        {
        barDuration = 30000000;
        }
    else if (barDuration < 45000000)
        {
        barDuration = 45000000;
        }
    else{
        barDuration = ((barDuration / 30000000) + 1) * 30000000;
        }

    TInt barWidth = iVideoBarBox.Width();
    
    TInt videoClipCount = iMovie.VideoClipCount();
    TInt audioClipCount = iMovie.AudioClipCount();  
    TInt lastVideoClipX(0);
    TTimeIntervalMicroSeconds endTime;

    __ASSERT_ALWAYS( ((iMovie.VideoClipCount()>0) || (iMovie.AudioClipCount()>0)), 
                    User::Panic( _L("VideoEditor" ), 34 ) );


    if (( videoClipCount > 0 ) && ( audioClipCount > 0 ))
        {
        TTimeIntervalMicroSeconds videoClipEndTime = iMovie.VideoClipEndTime( videoClipCount-1 );
        TTimeIntervalMicroSeconds audioClipEndTime = iMovie.AudioClipEndTime( audioClipCount-1 );
        if ( videoClipEndTime > audioClipEndTime )
            {
            endTime = videoClipEndTime;
            }
        else
            {
            endTime = audioClipEndTime;
            }
        }
    else if ( videoClipCount > 0 )
        {
        endTime = iMovie.VideoClipEndTime( videoClipCount-1 );
        }
    else
        {
        endTime = iMovie.AudioClipEndTime( audioClipCount-1 );
        }

    lastVideoClipX = iVideoBarBox.iTl.iX
        + static_cast<TInt32>((((endTime.Int64() * barWidth)) / barDuration)) + 1;
    
    TInt width = lastVideoClipX - iVideoBarBox.iTl.iX;

    TUint totalTime = static_cast<TInt32>(iMovie.Duration().Int64() /1000);
    TInt nextPosition = time * width / totalTime + iVideoBarBox.iTl.iX;

    if ( nextPosition != iCurrentPointX ) 
        {
        iCurrentPoint = time;
        DrawPlayHead();
        }
    else
        {
        LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::DoUpdatePosition 3, drawplayhead skipped");               
        }
    }

void CVeiEditVideoContainer::PlayVideoFileL( const TDesC& aFilename, const TBool& aFullScreen )
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::PlayVideoFileL(): In");

     if ( iTempFileName )
        {
        delete iTempFileName;
        iTempFileName = NULL;
        }

    iTempFileName = HBufC::NewL(KMaxFileName);
    *iTempFileName = aFilename;
    
    // Get default movie name from settings view

    TFileName newname;
    TVeiSettings movieSaveSettings;

    STATIC_CAST( CVeiAppUi*, iEikonEnv->AppUi() )->ReadSettingsL( movieSaveSettings );  

    newname.Append( movieSaveSettings.DefaultVideoName() );
    iInfoDisplay->SetName( newname );

    TTimeIntervalMicroSeconds movieDuration = iMovie.Duration();
    iInfoDisplay->SetDuration( movieDuration );

    if ( iTempVideoInfo ) 
        {
        delete iTempVideoInfo;
        iTempVideoInfo = 0;     
        }
    iFullScreenSelected = aFullScreen;

    if ( iCursorLocation == ECursorOnTransition )
        {
        iTransitionDisplayLeft->MakeVisible( EFalse );
        iTransitionDisplayRight->MakeVisible( EFalse );
        iArrowsDisplay->SetUpperArrowVisibility( EFalse );
        iArrowsDisplay->SetLowerArrowVisibility( EFalse );
        iDummyCutBar->SetRect( iDummyCutBarBox );
        }
    iInfoDisplay->MakeVisible( EFalse );
    iDummyCutBar->MakeVisible( EFalse );
    iDummyCutBarLeft->MakeVisible( EFalse );
    iEffectSymbols->MakeVisible( EFalse );
    iVideoDisplay->StopAnimation();
    
    SetPreviewState(EStateOpening);    

    if ( !iScreenLight->IsActive() )
        {
        iScreenLight->Start();
        }
    
    if ( iFullScreenSelected )
        {        
        LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::PlayVideoFileL: fullscreen selected");
        iView.SetEditorState( CVeiEditVideoView::EPreview);    

        SetBlackScreen( ETrue );        
        iVideoDisplay->OpenFileL( aFilename );    
        }
    else
        {
        LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::PlayVideoFileL: fullscreen not selected");
        iView.SetEditorState( CVeiEditVideoView::EQuickPreview);    
        iVideoDisplay->SetRect( iVideoDisplayBox );
        iVideoDisplay->ShowBlackScreen();
        iInfoDisplay->SetRect( iInfoDisplayBox );

        iDummyCutBar->MakeVisible( ETrue );
        iDummyCutBar->Dim( ETrue );
        iVideoDisplay->OpenFileL( aFilename );
        }
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::PlayVideoFileL(): Out");
    }

void CVeiEditVideoContainer::PlayVideo(const TDesC& /*aFilename*/, TBool& /*aFullScreen*/ )
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::PlayVideo In");
    
    if ( iVideoDisplay )
        {
        LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::PlayVideo(): 1");             

        //  associated with, when put into background and back to foreground, play starts from different
        // position it was paused (iSeekPos is not iLastPosition)
        iVideoDisplay->SetPositionL( iSeekPos );
                        
        if ( iTempVideoInfo && !iFrameReady)
            {           
            iTempVideoInfo->CancelFrame();
            }
        iVideoDisplay->Play();      
        }

    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::PlayVideo(): Out");   
    }

void CVeiEditVideoContainer::StopVideo( TBool aCloseStream )
    { 
    LOGFMT(KVideoEditorLogFile, "CVeiEditVideoContainer::StopVideo: In, aCloseStream:%d", aCloseStream);
    SetPreviewState(EStateStopped);    
    if ( iPeriodic )
        {
        iPeriodic->Cancel();
        }

    iSeekPos = TTimeIntervalMicroSeconds( 0 );

    iCloseStream = aCloseStream;

    iVideoDisplay->Stop( aCloseStream ); 

    iScreenLight->Stop();
    if ( aCloseStream )
        {
        SetPreviewState(EStateClosed);        
        }
    else
        {
        SetFinishedStatus( ETrue );
        }
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::StopVideo: out");
    }

void CVeiEditVideoContainer::TakeSnapshotL()
    {   
    LOGFMT(KVideoEditorLogFile, "CVeiEditVideoContainer::TakeSnapshotL: In, iFrameReady:%d", iFrameReady);

    if( !iTempVideoInfo || !iFrameReady )
        {
        LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::TakeSnapshotL: 1");
        if (!iFrameReady)
            {
            iTakeSnapshotWaiting = ETrue;   
            }   
        return;
        }
        
    iTakeSnapshotWaiting = EFalse;  
    iTakeSnapshot = ETrue;
    TTimeIntervalMicroSeconds pos = PlaybackPositionL();

    TInt frameIndex;
    TInt totalFrameCount;

    frameIndex = iTempVideoInfo->GetVideoFrameIndexL( pos );
    totalFrameCount = iTempVideoInfo->VideoFrameCount();

    if ( frameIndex > totalFrameCount )
        {
        frameIndex = totalFrameCount;
        }

    TRect clipResolution = Rect();
    TSize resol( clipResolution.iBr.iX, clipResolution.iBr.iY ); 
    TDisplayMode displayMode = ENone;    
    iFrameReady = EFalse;

    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::TakeSnapshotL: 2");
    //iTempVideoInfo->GetFrameL(*this, frameIndex, &resol, displayMode, ETrue);         

    /* :
     check out on every phone before releasing whether videodisplay should be stopped before starting
     asynchronous GetFrameL()
     see how EStateGettingFrame is handled in SetPreviewState 
     Stopping frees memory and it is needed in memory sensible devices 
    */
    iTempVideoInfo->GetFrameL(*this, frameIndex, NULL, displayMode, ETrue);
    SetPreviewState(EStateGettingFrame);

    StartProgressDialogL(R_VEI_PROGRESS_NOTE_WITH_CANCEL, R_VEI_PROGRESS_NOTE_SAVING_IMAGE);

    /*
    if (iProgressDialog)
        {
        delete iProgressDialog;
        iProgressDialog = NULL; 
        }

    iProgressDialog = 
           new (ELeave) CAknProgressDialog(REINTERPRET_CAST(CEikDialog**, 
          &iProgressDialog), ETrue);
    iProgressDialog->SetCallback(this);
    iProgressDialog->ExecuteDlgLD( R_VEI_PROGRESS_NOTE_WITH_CANCEL );


    HBufC* stringholder = StringLoader::LoadLC( R_VEI_PROGRESS_NOTE_SAVING_IMAGE, iEikonEnv );      
    iProgressDialog->SetTextL( *stringholder );
    CleanupStack::PopAndDestroy( stringholder );

    iProgressDialog->GetProgressInfoL()->SetFinalValue(100);
    */
    // this is good place to start Progress Note, but for some reason this causes some phones to crash
    // that is why progress note is started now in NotifyVideoClipFrameCompleted
    //StartProgressDialogL(R_VEI_PROGRESS_NOTE_WITH_CANCEL, R_VEI_PROGRESS_NOTE_SAVING_IMAGE);
            
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::TakeSnapshotL: Out");
    }

void CVeiEditVideoContainer::PauseVideoL()
    {
    LOGFMT(KVideoEditorLogFile, "CVeiEditVideoContainer::PauseVideoL: In, iPreviewState:%d", iPreviewState);        

    // if-condition added in order to prevent entering to pause state (icon is showed) from "wrong" state
    // etc. from "stopped" state
    if (EStatePlaying == iPreviewState)
        {                           
        LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::PauseVideoL 1");          
        iLastPosition = iVideoDisplay->PositionL();

        // think whether this should be here?
        iSeekPos = iVideoDisplay->PositionL();
        iVideoDisplay->SetPositionL(iSeekPos);
        }
    iVideoDisplay->PauseL();
    SetPreviewState(EStatePaused);
    iScreenLight->Stop();
    if ( iPeriodic )
        {
        iPeriodic->Cancel();
        }
    iView.StopNaviPaneUpdateL();
            
    #ifdef GET_PAUSE_THUMBNAIL
    GetThumbAtL( iLastPosition );
    #endif      

    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::PauseVideoL out");            
    }

TTimeIntervalMicroSeconds CVeiEditVideoContainer::PlaybackPositionL()
    {
    if ( ( iSeeking ) || ( EStateStopped == iPreviewState) )
        {
        return iSeekPos;
        }
    if ( EStatePlaying != iPreviewState )
        {
        return iLastPosition;
        }
        
    iLastPosition = iVideoDisplay->PositionL();

    // for what situation is this for?
    if ( ( iLastPosition == TTimeIntervalMicroSeconds( 0 ) ) &&
         ( iSeekPos != TTimeIntervalMicroSeconds( 0 ) ) )
        {
        return iSeekPos;
        }
    // for what situation is this for?
    return iLastPosition;
    }


void CVeiEditVideoContainer::SetSelectionMode( TSelectionMode aSelectionMode )
    {
    LOGFMT(KVideoEditorLogFile, "CVeiEditVideoContainer::SetSelectionMode: In, aSelectionMode:%d", aSelectionMode);
    iSelectionMode = aSelectionMode;

    if (EModeMixingAudio == iSelectionMode)
        {               
        iHorizontalSlider->SetPosition(0);
            
        if (0 != iMovie.GetVideoClipVolumeGainL(KVedClipIndexAll))
            {               
            TReal gain = iMovie.GetVideoClipVolumeGainL(KVedClipIndexAll);///(KVolumeMaxGain/10);
            gain = gain/(KVolumeMaxGain/10);
            if (0 < gain)
                {
                gain += 0.5;
                }
            else
                {
                gain -= 0.5;
                }                   
            iHorizontalSlider->SetPosition( (TInt)(-gain) ); // see impl. of CSliderBar
            }           
                        
        else if(0 != iMovie.GetAudioClipVolumeGainL(KVedClipIndexAll))
            {           
            TReal gain = iMovie.GetAudioClipVolumeGainL(KVedClipIndexAll);///(KVolumeMaxGain/10);
            gain = gain/(KVolumeMaxGain/10);
            if (0 < gain)
                {
                gain += 0.5;
                }
            else
                {
                gain -= 0.5;
                }           
            iHorizontalSlider->SetPosition( (TInt)gain );
            }           
                 
        iHorizontalSlider->MakeVisible(ETrue);
        iVerticalSlider->MakeVisible(EFalse);       
        iVideoDisplay->ShowPictureL( *iAudioMixingIcon );
        iInfoDisplay->MakeVisible(EFalse);


        iVideoDisplay->StopAnimation();
        if ( iCursorLocation == ECursorOnTransition )
            {        
            iTransitionDisplayLeft->MakeVisible( EFalse );
            iTransitionDisplayRight->MakeVisible( EFalse );
            iArrowsDisplay->SetUpperArrowVisibility( EFalse );
            iArrowsDisplay->SetLowerArrowVisibility( EFalse );
            iDummyCutBar->SetRect( iDummyCutBarBox );
            }


        }
    else if (EModeAdjustVolume == iSelectionMode)
        {       
        
        iVerticalSlider->SetPosition(0);
        
        if (VideoEditor::ECursorOnClip == CursorLocation())
            {                                       
            if (0 != iMovie.GetVideoClipVolumeGainL(CurrentIndex()))
                {   
                TReal adjustVolume = iMovie.GetVideoClipVolumeGainL(CurrentIndex());///(KVolumeMaxGain/10);
                adjustVolume = adjustVolume/(KVolumeMaxGain/10);                                                
                
                if (0 < adjustVolume)
                    {
                    adjustVolume += 0.5;
                    }
                else if (0 > adjustVolume)
                    {
                    adjustVolume -= 0.5;
                    }                   
                iVerticalSlider->SetPosition(-adjustVolume); // see impl. of CSliderBar             
                }                               
            }
        else if (VideoEditor::ECursorOnAudio == CursorLocation())
            {                   
            if (0 != iMovie.GetAudioClipVolumeGainL(CurrentIndex()))
                {                                   
                TReal adjustVolume = iMovie.GetAudioClipVolumeGainL(CurrentIndex());///(KVolumeMaxGain/10);
                adjustVolume = adjustVolume/(KVolumeMaxGain/10);                                        
                
                if (0 < adjustVolume)
                    {
                    adjustVolume += 0.5;
                    }
                else if (0 > adjustVolume)
                    {
                    adjustVolume -= 0.5;
                    }           
                iVerticalSlider->SetPosition(-adjustVolume);                
                }                           
            }
                
        iHorizontalSlider->MakeVisible(EFalse);
        iVerticalSlider->MakeVisible(ETrue);
        }
    else
        {
        iHorizontalSlider->MakeVisible(EFalse);
        iVerticalSlider->MakeVisible(EFalse);
        }

    if ( iSelectionMode == EModeSlowMotion )
        {
        ArrowsControl();
        }
        
    else 
        {
        iInfoDisplay->SetSlowMotionOn( EFalse );
        switch(iSelectionMode)
            {
            case EModeRecordingSetStart:
                iEffectSymbols->MakeVisible( ETrue );
                iEffectSymbols->SetPauseAudioIconVisibility( EFalse );
                iEffectSymbols->SetRecAudioIconVisibility( EFalse );
                break;
            case EModeRecordingPaused:
                iEffectSymbols->MakeVisible( ETrue );
                iInfoDisplay->SetLayout( CVeiTextDisplay::ERecordingPaused );
                iEffectSymbols->SetRecAudioIconVisibility( EFalse );
                iEffectSymbols->SetPauseAudioIconVisibility( ETrue );   
                iEffectSymbols->DrawNow();
                break;
            case EModeRecording:
                iEffectSymbols->MakeVisible( ETrue );
                iEffectSymbols->SetPauseAudioIconVisibility( EFalse );
                iEffectSymbols->SetRecAudioIconVisibility( ETrue );
                break;
            default:
                iEffectSymbols->SetPauseAudioIconVisibility( EFalse );
                iEffectSymbols->SetRecAudioIconVisibility( EFalse );
                if ( !iFullScreenSelected )
                {
                if ( VideoEditorUtils::IsLandscapeScreenOrientation() ) //Landscape     
                    {
                    iInfoDisplay->SetLayout( CVeiTextDisplay::EEverything );
                    }
                else
                    {
                    iInfoDisplay->SetLayout( CVeiTextDisplay::ENameAndDuration );
                    }
                }
                break;
            }
        }

    DrawDeferred();
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::SetSelectionMode: Out");
    }

TTypeUid::Ptr CVeiEditVideoContainer::MopSupplyObject( TTypeUid aId )
    {
    if ( aId.iUid == MAknsControlContext::ETypeId && iBgContext )
        {
        return MAknsControlContext::SupplyMopObject( aId, iBgContext );
        }
    return CCoeControl::MopSupplyObject( aId );
    }
// ---------------------------------------------------------
// CVeiEditVideoContainer::Draw(const TRect& aRect) const
// ---------------------------------------------------------
//
void CVeiEditVideoContainer::Draw( const TRect& /*aRect*/ ) const
    {
    LOGFMT(KVideoEditorLogFile, "CVeiEditVideoContainer::Draw() In, iPreviewState:%d", iPreviewState);
    CWindowGc& gc = SystemGc();

    if(iBlackScreen)
        {
        gc.Clear( Rect() );
        gc.SetPenStyle( CWindowGc::ESolidPen );
        gc.SetBrushColor( KRgbBlack );
        gc.SetBrushStyle( CGraphicsContext::ESolidBrush );
        gc.DrawRect( Rect() );
        gc.SetPenStyle( CWindowGc::ESolidPen ); 
        gc.DrawRoundRect( Rect(), TSize(4,4));
        }
    else
        {
        // Draw skin background
        MAknsSkinInstance* skin = AknsUtils::SkinInstance();
        MAknsControlContext* cc = AknsDrawUtils::ControlContext( this );
        AknsDrawUtils::Background( skin, cc, this, gc, Rect() );            

        if ( EStatePaused == iPreviewState  )
            {               
            gc.BitBltMasked( iPauseIconBox.iTl, iPauseBitmap, 
                TRect( TPoint(0,0), iPauseBitmap->SizeInPixels() ), 
                iPauseBitmapMask, EFalse );
            }

        if (EModeMixingAudio == iSelectionMode && CVeiEditVideoView::EMixAudio != iView.EditorState())
            {
            iHorizontalSlider->MakeVisible(EFalse);
            }
        else if (EModeMixingAudio == iSelectionMode && CVeiEditVideoView::EMixAudio == iView.EditorState())
            {
            iHorizontalSlider->MakeVisible(ETrue);
            TPoint mixAudioVideoIconPoint(iHorizontalSliderPoint.iX, iHorizontalSliderPoint.iY + 20);   
            TRect videoTrackIconSourceRect(0, 0, iVideoTrackIcon->SizeInPixels().iWidth, 
                iVideoTrackIcon->SizeInPixels().iHeight);
            gc.BitBltMasked( mixAudioVideoIconPoint, iVideoTrackIcon, videoTrackIconSourceRect,
                iVideoTrackIconMask, EFalse);

            TPoint mixAudioAudioIconPoint(iHorizontalSliderPoint.iX + 
                iHorizontalSliderSize.iWidth - 15, iHorizontalSliderPoint.iY + 20);
            TRect audioTrackIconSourceRect(0, 0, iAudioTrackIcon->SizeInPixels().iWidth, 
                iAudioTrackIcon->SizeInPixels().iHeight);
            gc.BitBltMasked(mixAudioAudioIconPoint, iAudioTrackIcon, audioTrackIconSourceRect,
                iAudioTrackIconMask, EFalse);

            return;
            }

        else if (EModeAdjustVolume == iSelectionMode && CVeiEditVideoView::EAdjustVolume != iView.EditorState())
            {
            iVerticalSlider->MakeVisible(EFalse);
            }
        else if (EModeAdjustVolume == iSelectionMode && CVeiEditVideoView::EAdjustVolume && iView.EditorState())
            {
            iVerticalSlider->MakeVisible(ETrue);
            TPoint pluspoint(iVerticalSliderPoint.iX - 12, iVerticalSliderPoint.iY + 20); 
            TPoint minuspoint(iVerticalSliderPoint.iX - 12, iVerticalSliderPoint.iY + iVerticalSliderSize.iHeight);
            _LIT(KPlus, "+");
            _LIT(KMinus, "-");

            const CFont* font = AknLayoutUtils::FontFromId( EAknLogicalFontSecondaryFont );
            gc.UseFont( font );

            // Get text color from skin
            TRgb textColor( KRgbBlack );
            MAknsSkinInstance* skinInstance = AknsUtils::SkinInstance();
            AknsUtils::GetCachedColor(skinInstance, textColor, KAknsIIDQsnTextColors, EAknsCIQsnTextColorsCG6 );
            gc.SetPenColor( textColor );

            gc.DrawText( KPlus, pluspoint );
            gc.DrawText( KMinus, minuspoint );

            gc.DiscardFont();

            return;
            }

        /* Initialize icon & text areas. */

        CFbsBitmap* thumbnail = 0;

        TInt i;

        TSize roundEdgeSize(2,2);

        /* Draw bar area. */

        TRgb rgbUnselectedBorder = TRgb( 132,132,132 );

        TRgb rgbUnselectedTrackFill = TRgb(221,221,221);
        TRgb rgbUnselectedTrackBorder = TRgb( 201,201,201 );
        TRgb rgbUnselectedTrackBorderOuterRect = TRgb( 162,162,162 );

        TRgb rgbUnselectedAudioMarker = KRgbWhite;

        TRgb rgbUnselectedClip = TRgb( 140,166,198 );

        TRgb rgbUnselectedTransition = KRgbWhite;
        TRgb rgbSelectedBorder = TRgb( 94,97,101 );
        TRgb rgbSelectedAudioMarker = KRgbBlack;
        TRgb rgbSelectedClip = TRgb( 108,139,182 );
        TRgb rgbSelectedTransition = TRgb( 0x00, 0x9b, 0xff );
        TRgb rgbActiveBorder = KRgbRed;
        TRgb rgbActiveAudioMarker = KRgbRed;
        TRgb rgbActiveClip = TRgb( 140,166,198 );
        
        TRect videoTrackIconSourceRect(0, 0, iVideoTrackIcon->SizeInPixels().iWidth, 
                iVideoTrackIcon->SizeInPixels().iHeight);

        gc.BitBltMasked( iVideoBarIconPos, iVideoTrackIcon, videoTrackIconSourceRect,
            iVideoTrackIconMask, EFalse);

        TRect audioTrackIconSourceRect(0, 0, iAudioTrackIcon->SizeInPixels().iWidth, 
            iAudioTrackIcon->SizeInPixels().iHeight);
        gc.BitBltMasked(iAudioBarIconPos, iAudioTrackIcon, audioTrackIconSourceRect,
            iAudioTrackIconMask, EFalse);

        gc.SetPenStyle( CGraphicsContext::ESolidPen );
        gc.SetPenColor( rgbUnselectedTrackBorderOuterRect );
        gc.SetBrushColor( rgbUnselectedTrackFill );
        gc.SetBrushStyle( CGraphicsContext::ESolidBrush );

        if ( ( iMovie.VideoClipCount() == 0 ) && ( iCursorLocation != ECursorOnAudio ) &&
             ( iCursorLocation != ECursorOnEmptyAudioTrack )  || 
                ( iCursorLocation == ECursorOnEmptyVideoTrack ) )
            {
            gc.SetPenColor(rgbSelectedBorder);
            gc.SetBrushColor( TRgb( 180,206,238 ) );
            }

        gc.DrawRoundRect(iVideoTrackBox, TSize(2,2));
        gc.SetBrushStyle( CGraphicsContext::ENullBrush );
        gc.SetPenColor(rgbUnselectedTrackBorder);
        TRect outerRect(iVideoTrackBox);
        outerRect.Shrink(1,1);
        gc.DrawRoundRect(outerRect, TSize(2,2) );


        gc.SetPenColor( rgbUnselectedTrackBorderOuterRect );
        gc.SetBrushColor( rgbUnselectedTrackFill );
        gc.SetBrushStyle( CGraphicsContext::ESolidBrush );

        if (((iMovie.AudioClipCount() == 0) && (iCursorLocation == ECursorOnAudio) )|| 
                ( iCursorLocation == ECursorOnEmptyAudioTrack ) )
            {
            gc.SetPenColor(rgbSelectedBorder);
            gc.SetBrushColor( TRgb( 180,206,238 ) );
            }

        gc.DrawRoundRect(iAudioTrackBox, TSize(2,2));

        gc.SetBrushStyle( CGraphicsContext::ENullBrush );
        gc.SetPenColor(rgbUnselectedTrackBorder);
        outerRect = iAudioTrackBox;
        outerRect.Shrink(1,1);
        gc.DrawRoundRect(outerRect, TSize(2,2) );

        gc.SetBrushStyle( CGraphicsContext::ESolidBrush );

        TInt64 barDuration = iMovie.Duration().Int64();
        TInt64 audioDuration(0);
        if ( (iMovie.AudioClipCount() != 0) && ((iSelectionMode == EModeRecording ) ||
            (iSelectionMode == EModeRecordingPaused)))
            {
            audioDuration = (iMovie.AudioClipEndTime( iMovie.AudioClipCount() - 1 )).Int64();
            }

        audioDuration+= iRecordedAudioDuration.Int64();

        if ( audioDuration > barDuration )
            {
            barDuration = audioDuration;
            }

        if ( iRecordedAudioStartTime > barDuration )
            {
            barDuration = iRecordedAudioStartTime.Int64();
            }
        if (barDuration < 30000000)
            {
            barDuration = 30000000;
            }
        else if (barDuration < 45000000)
            {
            barDuration = 45000000;
            }
        else{
            barDuration = ((barDuration / 30000000) + 1) * 30000000;
            }
        
        TBool drawSelectedRect = EFalse;
        TInt selectedAudioClipIndex( -1 );
        TRect selectedRect;

        gc.SetPenColor(rgbUnselectedBorder);
        gc.SetBrushColor(rgbUnselectedClip);

        TRect audioBoxes  = TRect();
       
        TRect box;
        box.iTl.iY = iAudioBarBox.iTl.iY;
        box.iBr.iY = iAudioBarBox.iBr.iY;
        TInt barWidth = iAudioBarBox.Width();
        TInt videoIndex = 0;

        for (i = 0; i < iMovie.AudioClipCount(); i++)
            {
            box.iTl.iX = iAudioBarBox.iTl.iX
                + static_cast<TInt32>((iMovie.AudioClipStartTime(i).Int64() * barWidth) / 
                barDuration);
            box.iBr.iX = iAudioBarBox.iTl.iX
                + static_cast<TInt32>((iMovie.AudioClipEndTime(i).Int64() * barWidth) / barDuration)+ 1;

            if ((iCursorLocation == ECursorOnAudio)
                && (i == CurrentIndex()) && (( iSelectionMode == EModeNavigation )
                || ( iSelectionMode == EModeMove ) ||
                   ( iSelectionMode == EModeDuration ) ))
                {
                drawSelectedRect = ETrue;
                selectedAudioClipIndex = i;
                selectedRect = box;
                }
            else
                {
                gc.DrawRect(box);

                gc.SetPenColor(rgbUnselectedAudioMarker);

                audioBoxes.Resize( box.Size() );
               
                if ( ( iMovie.AudioClipCutOutTime( i ).Int64() -
                     iMovie.AudioClipCutInTime( i ).Int64() ) <
                     iMovie.AudioClipInfo( i )->Duration().Int64() )        
                    {
                    TRect truncateBox;
                    truncateBox.iTl.iY = box.iTl.iY + 2;
                    truncateBox.iBr.iY = box.iBr.iY - 2;

                    truncateBox.iTl.iX = box.iBr.iX - 4;
                    truncateBox.iBr.iX = truncateBox.iTl.iX + 1;
                    if (truncateBox.iTl.iX >= (box.iTl.iX + 2))
                        {
                        gc.DrawRect(truncateBox);
                        }
                
                    truncateBox.iTl.iX = box.iBr.iX - 7;
                    truncateBox.iBr.iX = truncateBox.iTl.iX + 1;
                    if (truncateBox.iTl.iX >= (box.iTl.iX + 2))
                        {
                        gc.DrawRect(truncateBox);
                        }

                    truncateBox.iTl.iX = box.iBr.iX - 10;
                    truncateBox.iBr.iX = truncateBox.iTl.iX + 1;
                    if (truncateBox.iTl.iX >= (box.iTl.iX + 2))
                        {
                        gc.DrawRect(truncateBox);
                        }
                    }

                gc.SetPenColor(rgbUnselectedBorder);
                }

            TTimeIntervalMicroSeconds audioClipStartTimeInt = iMovie.AudioClipStartTime(i).Int64() + iMovie.AudioClipCutInTime(i).Int64(); 
            TTimeIntervalMicroSeconds audioClipEndTimeInt =   iMovie.AudioClipEndTime(i).Int64(); 

            if ((audioClipStartTimeInt == TTimeIntervalMicroSeconds(0)) && (iMovie.VideoClipCount() > 0))
                {
                TRect syncBox;
                syncBox.iTl.iX = box.iTl.iX;
                syncBox.iTl.iY = iVideoBarBox.iBr.iY;
                syncBox.iBr.iX = box.iTl.iX + 1;
                syncBox.iBr.iY = iAudioBarBox.iTl.iY;

                gc.DrawRect(syncBox);
                }

            while (videoIndex < iMovie.VideoClipCount())
                {
                TInt oldIndex = videoIndex;
                if ( iMovie.VideoClipEndTime( oldIndex ) < audioClipStartTimeInt )
                    {
                    videoIndex++;
                    }
                else if ( iMovie.VideoClipEndTime( oldIndex ) == audioClipStartTimeInt )
                    {
                    TRect syncBox;
                    syncBox.iTl.iX = box.iTl.iX;
                    syncBox.iTl.iY = iVideoBarBox.iBr.iY;
                    syncBox.iBr.iX = box.iTl.iX + 1;
                    syncBox.iBr.iY = iAudioBarBox.iTl.iY;

                    gc.DrawRect(syncBox);
                    break;
                    }
                else
                    {
                    break;
                    }
                }

            while (videoIndex < iMovie.VideoClipCount())
                {
                TInt oldIndex = videoIndex;

                if ( iMovie.VideoClipEndTime( oldIndex ) < audioClipEndTimeInt )
                    {
                    videoIndex++;
                    }
                else if ( iMovie.VideoClipEndTime( oldIndex ) == audioClipEndTimeInt )
                    {
                    TRect syncBox;
                    syncBox.iTl.iX = box.iBr.iX - 1;
                    syncBox.iTl.iY = iVideoBarBox.iBr.iY;
                    syncBox.iBr.iX = box.iBr.iX;
                    syncBox.iBr.iY = iAudioBarBox.iTl.iY;

                    gc.DrawRect(syncBox);
                    break;
                    }
                else
                    {
                    break;
                    }
                }
            }

        if ((iSelectionMode == EModeRecordingSetStart) || (iSelectionMode == EModeRecording)
            || (iSelectionMode == EModeRecordingPaused))
            {
            box.iTl.iX = iAudioBarBox.iTl.iX
                + static_cast<TInt32>((iRecordedAudioStartTime.Int64() * barWidth) / barDuration);
            TInt64 recordedAudioEndTimeInt = iRecordedAudioStartTime.Int64() + iRecordedAudioDuration.Int64();
            box.iBr.iX = iAudioBarBox.iTl.iX
                + static_cast<TInt32>((recordedAudioEndTimeInt * barWidth) / barDuration)+ 1;

            gc.SetPenColor(rgbActiveBorder);
            gc.SetBrushColor(rgbSelectedClip);
            gc.DrawRect(box);
            gc.SetPenColor(rgbUnselectedBorder);
            gc.SetBrushColor(rgbUnselectedClip);
            }

        box.iTl.iY = iVideoBarBox.iTl.iY;
        box.iBr.iY = iVideoBarBox.iBr.iY;
        barWidth = iVideoBarBox.Width();
        
        TRect videoBoxes  = TRect();
        
        for (i = 0; i < iMovie.VideoClipCount(); i++)
            {
            box.iTl.iX = iVideoBarBox.iTl.iX
                + static_cast<TInt32>((iMovie.VideoClipStartTime( i ).Int64() * barWidth) / barDuration);
            box.iBr.iX = iVideoBarBox.iTl.iX
                + static_cast<TInt32>((iMovie.VideoClipEndTime( i ).Int64() * barWidth) / barDuration)+ 1;
            
            videoBoxes.Resize( box.Size() );
            
            if ((iCursorLocation == ECursorOnClip)
                && (i == CurrentIndex()) && ( iView.EditorState() == CVeiEditVideoView::EEdit ))
                {
                drawSelectedRect = ETrue;
                selectedRect = box;
                gc.DrawRect(box);
                }
            else
                {
                gc.DrawRect(box);
                //Draw thumbnail in video box.
                if ( i < iVideoItemArray.Count() )
                    {
                    thumbnail = iVideoItemArray[i]->iTimelineBitmap;
                    }
                if ( thumbnail )
                    {
                    //TSize thumbnailSizeInPixels = thumbnail->SizeInPixels();
                    TPoint pos( box.iTl.iX+1, box.iTl.iY+1 );
                    TSize pieceSize = TSize(STATIC_CAST(TInt, (box.Height()-2)*1.22), box.Height()-2);
                    
                    if ( pieceSize.iWidth >= box.Width()-2  )
                        {
                        pieceSize.SetSize( box.Width()-2, box.Height()-2 );
                        }
                    TRect pieceRect( TPoint(0,0), pieceSize );
                    gc.BitBlt( pos, thumbnail, pieceRect ); 
                    }

             
                //Draw play head.
                if( iPreviewState == EStatePaused || iPreviewState == EStateStopped)
                    {
                    TRect bar( iBarArea );//bar rect.

                    const TUint barY = 6;

                    bar.iTl.iY += barY;
                    bar.iTl.iX += barY;
                    bar.iTl.iX += iVideoTrackIcon->SizeInPixels().iWidth;
                    bar.iBr.iY -= barY / 2;
                    bar.iBr.iX -= barY;

                        
                    gc.SetBrushColor( KRgbBlack );
                    gc.SetBrushStyle( CGraphicsContext::ESolidBrush );
                    gc.SetPenColor( KRgbDarkGray );
                    gc.SetPenStyle( CGraphicsContext::ESolidPen );

                    TPoint inPointList[3];
                    inPointList[0] = TPoint( (iCurrentPointX - barY) + 1, iBarArea.iTl.iY - 5 );
                    inPointList[1] = TPoint( (iCurrentPointX + barY) - 1, iBarArea.iTl.iY - 5 );
                    inPointList[2] = TPoint( iCurrentPointX, bar.iTl.iY - 6  );
                    gc.DrawPolygon( inPointList, 3 );

                    inPointList[0] = TPoint( (iCurrentPointX - barY) + 1, iBarArea.iBr.iY + 5 );
                    inPointList[1] = TPoint( (iCurrentPointX + barY) - 1, iBarArea.iBr.iY + 5 );
                    inPointList[2] = TPoint( iCurrentPointX, bar.iBr.iY );
                    gc.DrawPolygon( inPointList, 3 );

                    gc.SetPenSize( TSize( 3, 1 ) );
                    gc.DrawLine( TPoint( iCurrentPointX, bar.iTl.iY - 5 ), 
                    TPoint( iCurrentPointX, bar.iBr.iY ) );
                    
                    gc.SetPenColor(rgbUnselectedBorder);
                    gc.SetBrushColor(rgbUnselectedClip);
                    gc.SetPenSize( TSize( 1, 1 ) );
                    }
                }
            }

        if (drawSelectedRect)
            {
            selectedRect.Grow( iZoomFactorX,iZoomFactorY );


            TRect outerBlackRect = selectedRect;
            outerBlackRect.Grow(1,1);
            gc.SetPenColor( KRgbBlack );
            gc.SetPenStyle( CGraphicsContext::ESolidPen );
            gc.DrawRoundRect( outerBlackRect, TSize(2,2) );


            if (iSelectionMode == EModeMove )   //Move, draw dashed outline
                {
                gc.SetPenStyle( CGraphicsContext::EDashedPen );
                gc.SetPenColor( KRgbBlack );
                gc.SetBrushColor(rgbActiveClip);
                }
            else    
                {
                gc.SetPenColor(rgbSelectedBorder);
                gc.SetBrushColor(rgbSelectedClip);
                }

            gc.DrawRoundRect( selectedRect,roundEdgeSize );
            
            selectedRect.Shrink(2,2);
            gc.DrawBitmap( selectedRect, iGradientBitmap );

             //Draw thumbnail in video box
            if ( iCursorLocation == ECursorOnClip )
                {
                selectedRect.Grow(2,2);
                
                thumbnail = iVideoItemArray[CurrentIndex()]->iTimelineBitmap;
                if ( thumbnail )
                    {
                    TPoint pos( selectedRect.iTl.iX+1, selectedRect.iTl.iY+1 );
                    TSize pieceSize = TSize(STATIC_CAST(TInt, (selectedRect.Height()-2)*1.22), 
                        selectedRect.Height()-2);

                    if ( pieceSize.iWidth >= selectedRect.Width()-2 )
                        {
                        pieceSize.SetSize( selectedRect.Width()-2, selectedRect.Height()-2 );
                        }
                    TRect pieceRect( TPoint(0,0), pieceSize );
                    gc.BitBlt( pos, thumbnail, pieceRect ); 
                    }
                }
            drawSelectedRect = EFalse;

            if (iSelectionMode == EModeNavigation)
                {
                gc.SetPenColor(rgbSelectedAudioMarker);
                }
            else
                {
                gc.SetPenColor(rgbActiveAudioMarker);
                }

            if ((selectedAudioClipIndex != -1) && 
                 ( ( iMovie.AudioClipCutOutTime( selectedAudioClipIndex ).Int64() -
                     iMovie.AudioClipCutInTime( selectedAudioClipIndex ).Int64() ) <
                     iMovie.AudioClipInfo( selectedAudioClipIndex )->Duration().Int64() ) )
                {
                TRect truncateBox;
                truncateBox.iTl.iY = selectedRect.iTl.iY + 2;
                truncateBox.iBr.iY = selectedRect.iBr.iY - 2;

                truncateBox.iTl.iX = selectedRect.iBr.iX - 4;
                truncateBox.iBr.iX = truncateBox.iTl.iX + 1;
                if (truncateBox.iTl.iX >= (selectedRect.iTl.iX + 2))
                    {
                    gc.DrawRect(truncateBox);
                    }
            
                truncateBox.iTl.iX = selectedRect.iBr.iX - 7;
                truncateBox.iBr.iX = truncateBox.iTl.iX + 1;
                if (truncateBox.iTl.iX >= (selectedRect.iTl.iX + 2))
                    {
                    gc.DrawRect(truncateBox);
                    }

                truncateBox.iTl.iX = selectedRect.iBr.iX - 10;
                truncateBox.iBr.iX = truncateBox.iTl.iX + 1;
                if (truncateBox.iTl.iX >= (selectedRect.iTl.iX + 2))
                    {
                    gc.DrawRect(truncateBox);
                    }
                }
            }

        gc.SetPenStyle(CGraphicsContext::ESolidPen);
        gc.SetPenColor(rgbUnselectedBorder);
        gc.SetBrushColor(rgbUnselectedTransition);

        box.iTl.iY += (box.Height() - iTransitionMarkerSize.iHeight) / 2; 
        box.iBr.iY = box.iTl.iY + iTransitionMarkerSize.iHeight;
        for (i = iMovie.VideoClipCount(); (i >= 0) && (iMovie.VideoClipCount() > 0); i--)
            {
            if ((i == 0) && (iMovie.VideoClipCount() > 0))
                {
                box.iTl.iX = iVideoBarBox.iTl.iX
                    - (iTransitionMarkerSize.iWidth / 2);
                }
            else
                {
                box.iTl.iX = iVideoBarBox.iTl.iX
                    + static_cast<TInt32>((iMovie.VideoClipEndTime( i-1 ).Int64() * barWidth) / barDuration)
                    - (iTransitionMarkerSize.iWidth / 2);
                }

            box.iBr.iX = box.iTl.iX + iTransitionMarkerSize.iWidth;

            if ((iCursorLocation == ECursorOnTransition) && (i == CurrentIndex() ) ) 
                {
                drawSelectedRect = ETrue;
                selectedRect = box;
                }
            else
                {
                if ( (iCursorLocation == ECursorOnClip) && (i == CurrentIndex() ) &&( iSelectionMode != EModePreview ) )
                    {
                    box.Move( -iZoomFactorX,0);
                    box.Grow(0,iZoomFactorX/2);
                    gc.DrawRect(box);
                    box.Shrink(0,iZoomFactorX/2);
                    box.Move( iZoomFactorX,0 );
                    }
                else if ( (iCursorLocation == ECursorOnClip) && (i == CurrentIndex()+1 ) && ( iSelectionMode != EModePreview ) )
                    {
                    box.Move( iZoomFactorX,0 );
                    box.Grow(0,iZoomFactorX/2);
                    gc.DrawRect(box);
                    box.Shrink(0,iZoomFactorX/2);
                    box.Move( -iZoomFactorX,0 );
                    }
                else
                    {
                    gc.DrawRect(box);
                    }
                }
            }

        if (drawSelectedRect)
            {
            selectedRect.Grow(1,iZoomFactorX/2);
            gc.SetPenColor(rgbSelectedBorder);
            gc.SetBrushColor(rgbSelectedTransition);
            gc.DrawRect(selectedRect);
            }
            
        // draw the new position for the clip when a clip is dragged with touch
        if ( iIsVideoDrag ) 
            {    
            TRgb rgbSelectedBorder = KRgbDarkMagenta;
            TRgb rgbSelectedTransition = KRgbDarkMagenta;
            gc.SetPenColor( rgbSelectedBorder );
            gc.SetBrushColor( rgbSelectedTransition );
            
            box.iTl.iY = iVideoBarBox.iTl.iY;
            box.iBr.iY = iVideoBarBox.iBr.iY;

            // clip is moved from right to left
            if ( iClickedClip > iNewClipPosition )
                {
                if ( iNewClipPosition == 0 )
                    {
                    box.iTl.iX = iVideoItemRectArray[ iNewClipPosition ].iTl.iX;
                    box.iBr.iX = iVideoItemRectArray[ iNewClipPosition ].iTl.iX + 5;                        
                    }
                else
                    {
                    box.iTl.iX = iVideoItemRectArray[ iNewClipPosition ].iTl.iX - 5;
                    box.iBr.iX = iVideoItemRectArray[ iNewClipPosition ].iTl.iX + 5;                        
                    }
                gc.DrawRect( box );
                }
            // clip is moved from left to right
            else if ( iClickedClip < iNewClipPosition )
                {
                box.iTl.iX = iVideoItemRectArray[ iNewClipPosition ].iBr.iX - 5;
                box.iBr.iX = iVideoItemRectArray[ iNewClipPosition ].iBr.iX + 5;                
                gc.DrawRect( box );
                }
            }            
        }//else 
    }

void CVeiEditVideoContainer::HandleResourceChange(TInt aType)
    {
    LOGFMT(KVideoEditorLogFile, "CVeiEditVideoContainer::HandleResourceChange() In, aType:%d", aType);
    
    if (KAknsMessageSkinChange == aType)
        {
        HandleComponentControlsResourceChange(aType);
        }
    CCoeControl::HandleResourceChange(aType);
    
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::HandleResourceChange() Out");
    }

void CVeiEditVideoContainer::DrawTrackBoxes() const
    {
    if(!iBlackScreen)
        {
        TRect drawableRect = iBarArea;
        drawableRect.iTl.iX = (iVideoTrackBox.iTl.iX - KMaxZoomFactorX) - 2;
        drawableRect.iBr.iX = (iVideoTrackBox.iBr.iX + KMaxZoomFactorX) + 2;
        drawableRect.iTl.iY = (iVideoTrackBox.iTl.iY - KMaxZoomFactorY) - 2;
        drawableRect.iBr.iY = (iAudioTrackBox.iBr.iY + KMaxZoomFactorY) + 2;

        Window().Invalidate( drawableRect );
        ActivateGc();
    //Redraw of the window's invalid region.
        Window().BeginRedraw( drawableRect );
        Draw( drawableRect );

        Window().EndRedraw();
        DeactivateGc();
        }
    }




// ----------------------------------------------------------------------------
// CVeiEditVideoContainer::GetHelpContext(...) const
//
// Gets the control's help context. Associates the control with a particular
// Help file and topic in a context sensitive application.
// ----------------------------------------------------------------------------
//
void CVeiEditVideoContainer::GetHelpContext( TCoeHelpContext& aContext ) const
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::GetHelpContext(): In");

    // Set UID of the CS Help file (same as application UID).
    aContext.iMajor = KUidVideoEditor;

    // Set the context/topic.
    aContext.iContext = KVED_HLP_EDIT_VIDEO_VIEW;

    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::GetHelpContext(): Out");
    }


void CVeiEditVideoContainer::ArrowsControl() const
    {           
    
    iInfoDisplay->SetLayout( CVeiTextDisplay::EOnlyName );
    iInfoDisplay->SetUpperArrowVisibility( EFalse );
    iInfoDisplay->SetLowerArrowVisibility( EFalse );
    iInfoDisplay->SetRightArrowVisibility( EFalse );
    iInfoDisplay->SetLeftArrowVisibility( EFalse );
 
    iArrowsDisplay->MakeVisible( EFalse );
    iArrowsDisplay->SetUpperArrowVisibility( EFalse );
    iArrowsDisplay->SetLowerArrowVisibility( EFalse );
    iArrowsDisplay->SetRightArrowVisibility( EFalse );
    iArrowsDisplay->SetLeftArrowVisibility( EFalse );
    
    iInfoDisplay->SetSlowMotionOn( EFalse );

    iDummyCutBar->MakeVisible( ETrue );

    if ( iCursorLocation == ECursorOnClip)
        {
        iInfoDisplay->MakeVisible( ETrue );
        iInfoDisplay->SetRect( iInfoDisplayBox );

        if( VideoEditorUtils::IsLandscapeScreenOrientation() )
            {
            iInfoDisplay->SetLayout( CVeiTextDisplay::EEverything );
            }
        else
            {
            iInfoDisplay->SetLayout( CVeiTextDisplay::ENameAndDuration );
            }

        if (iSelectionMode == EModeSlowMotion)
            {
            iArrowsDisplay->MakeVisible( ETrue );

            if( !VideoEditorUtils::IsLandscapeScreenOrientation() )
                {
                iInfoDisplay->MakeVisible( EFalse );
                }

            iArrowsDisplay->SetSlowMotionOn( ETrue );
            iArrowsDisplay->SetLayout( CVeiTextDisplay::EArrowsHorizontal );

            iDummyCutBar->MakeVisible( EFalse );
            
            if ( iSlowMotionValue < 1000  )
                {
                iArrowsDisplay->SetRightArrowVisibility( ETrue );
                }
            if ( iSlowMotionValue > 250  ) 
                {
                iArrowsDisplay->SetLeftArrowVisibility( ETrue );
                }
                iArrowsDisplay->SetRect( iSlowMotionBox );
            }       
        else
            {
            iVideoDisplay->StopAnimation();
            }
        }
    else if ( iCursorLocation == ECursorOnAudio )
        {
        iInfoDisplay->MakeVisible( ETrue );
        iInfoDisplay->SetRect( iInfoDisplayBox );

        if( VideoEditorUtils::IsLandscapeScreenOrientation() )
            {
            iInfoDisplay->SetLayout( CVeiTextDisplay::EEverything );
            }
        else
            {
            iInfoDisplay->SetLayout( CVeiTextDisplay::ENameAndDuration );
            }

        }
     else if ( iCursorLocation == ECursorOnTransition )
        {            
        iDummyCutBar->MakeVisible( ETrue );
        iInfoDisplay->MakeVisible( ETrue );

        iInfoDisplay->SetUpperArrowVisibility( ETrue );
        iInfoDisplay->SetLowerArrowVisibility( ETrue );
        iInfoDisplay->SetLayout( CVeiTextDisplay::EArrowsVertical );
        iInfoDisplay->SetRect( iTransitionArrowsBox );      
        }
    if ( iView.EditorState() == CVeiEditVideoView::EMixAudio || 
            iView.EditorState() == CVeiEditVideoView::EAdjustVolume)
        {               
        iDummyCutBar->MakeVisible( EFalse );
        }                   
    
    if ( iView.EditorState() == CVeiEditVideoView::EMixAudio)
        {
        iInfoDisplay->MakeVisible(EFalse);          
        }       
    if ( iView.EditorState() == CVeiEditVideoView::EAdjustVolume)
        {
        iInfoDisplay->MakeVisible(ETrue);           
        }       
    }

void CVeiEditVideoContainer::DrawPlayHead()
    {
    CFbsBitmap* thumbnail = 0;
    TRect redrawArea = TRect(iVideoBarIconPos.iX+iVideoTrackIcon->SizeInPixels().iWidth, 
        iBarArea.iTl.iY - 5, iBarArea.iBr.iX, iBarArea.iBr.iY + 10 );
    
    Window().Invalidate( redrawArea );
    ActivateGc();
    //Redraw of the window's invalid region.
    Window().BeginRedraw( redrawArea );
    CWindowGc& gc = SystemGc();
    
    // Draw skin background
    MAknsSkinInstance* skin = AknsUtils::SkinInstance();
    MAknsControlContext* cc = AknsDrawUtils::ControlContext( this );
    AknsDrawUtils::Background( skin, cc, this, gc, Rect() );

    /* Draw bar area. */
    TInt i;
    TRgb rgbUnselectedBorder = TRgb( 132,132,132 );
    TRgb rgbUnselectedTrackFill = TRgb(221,221,221);
    TRgb rgbUnselectedTrackBorder = TRgb( 201,201,201 );
    TRgb rgbUnselectedTrackBorderOuterRect = TRgb( 162,162,162 );

    TRgb rgbUnselectedAudioMarker = KRgbWhite;
    TRgb rgbUnselectedClip = TRgb( 140,166,198 );
    TRgb rgbUnselectedTransition = KRgbWhite;
    
    gc.SetPenStyle( CGraphicsContext::ESolidPen );
    gc.SetPenColor( rgbUnselectedTrackBorderOuterRect );
    gc.SetBrushColor( rgbUnselectedTrackFill );
    gc.SetBrushStyle( CGraphicsContext::ESolidBrush );
    gc.DrawRoundRect(iVideoTrackBox, TSize(2,2));

    gc.SetBrushStyle( CGraphicsContext::ENullBrush );
    gc.SetPenColor( rgbUnselectedTrackBorder );
    TRect outerRect( iVideoTrackBox );
    outerRect.Shrink(1,1);
    gc.DrawRoundRect(outerRect, TSize(2,2) );

    gc.SetPenColor( rgbUnselectedTrackBorderOuterRect );
    gc.SetBrushColor( rgbUnselectedTrackFill );
    gc.SetBrushStyle( CGraphicsContext::ESolidBrush );
    gc.DrawRoundRect(iAudioTrackBox, TSize(2,2));

    gc.SetBrushStyle( CGraphicsContext::ENullBrush );
    gc.SetPenColor(rgbUnselectedTrackBorder);
    outerRect = iAudioTrackBox;
    outerRect.Shrink(1,1);
    gc.DrawRoundRect(outerRect, TSize(2,2) );

    gc.SetBrushStyle( CGraphicsContext::ESolidBrush );

    TInt64 barDuration = iMovie.Duration().Int64();
    TInt64 audioDuration(0);
    if ( (iMovie.AudioClipCount() != 0) && (iSelectionMode == EModeRecording ))
        {
        audioDuration = (iMovie.AudioClipEndTime( iMovie.AudioClipCount() - 1 )).Int64();
        }

    audioDuration+= iRecordedAudioDuration.Int64();

    if ( audioDuration > barDuration )
        {
        barDuration = audioDuration;
        }

    if ( iRecordedAudioStartTime > barDuration )
        {
        barDuration = iRecordedAudioStartTime.Int64();
        }
    if (barDuration < 30000000)
        {
        barDuration = 30000000;
        }
    else if (barDuration < 45000000)
        {
        barDuration = 45000000;
        }
    else{
        barDuration = ((barDuration / 30000000) + 1) * 30000000;
        }

    gc.SetPenColor(rgbUnselectedBorder);
    gc.SetBrushColor(rgbUnselectedClip);

    TRect audioBoxes  = TRect(0,0,0,0);
    
    TRect box;
    box.iTl.iY = iAudioBarBox.iTl.iY;
    box.iBr.iY = iAudioBarBox.iBr.iY;
    TInt barWidth = iAudioBarBox.Width();
    TInt videoIndex = 0;

    for (i = 0; i < iMovie.AudioClipCount(); i++)
        {
        box.iTl.iX = iAudioBarBox.iTl.iX
            + static_cast<TInt32>((iMovie.AudioClipStartTime(i).Int64() * barWidth) / 
            barDuration);
        box.iBr.iX = iAudioBarBox.iTl.iX
            + static_cast<TInt32>((iMovie.AudioClipEndTime(i).Int64() * barWidth) / barDuration)+ 1;

            {
            gc.DrawRect(box);

            gc.SetPenColor(rgbUnselectedAudioMarker);
            
            audioBoxes = box;
            
            gc.SetPenColor(rgbUnselectedAudioMarker);

            if ( ( iMovie.AudioClipCutOutTime( i ).Int64() -
                 iMovie.AudioClipCutInTime( i ).Int64() ) <
                 iMovie.AudioClipInfo( i )->Duration().Int64() )        
                {
                TRect truncateBox;
                truncateBox.iTl.iY = box.iTl.iY + 2;
                truncateBox.iBr.iY = box.iBr.iY - 2;

                truncateBox.iTl.iX = box.iBr.iX - 4;
                truncateBox.iBr.iX = truncateBox.iTl.iX + 1;
                if (truncateBox.iTl.iX >= (box.iTl.iX + 2))
                    {
                    gc.DrawRect(truncateBox);
                    }
            
                truncateBox.iTl.iX = box.iBr.iX - 7;
                truncateBox.iBr.iX = truncateBox.iTl.iX + 1;
                if (truncateBox.iTl.iX >= (box.iTl.iX + 2))
                    {
                    gc.DrawRect(truncateBox);
                    }

                truncateBox.iTl.iX = box.iBr.iX - 10;
                truncateBox.iBr.iX = truncateBox.iTl.iX + 1;
                if (truncateBox.iTl.iX >= (box.iTl.iX + 2))
                    {
                    gc.DrawRect(truncateBox);
                    }
                }

            gc.SetPenColor(rgbUnselectedBorder);
            }

        TTimeIntervalMicroSeconds audioClipStartTimeInt = iMovie.AudioClipStartTime(i).Int64() + iMovie.AudioClipCutInTime(i).Int64(); 
        TTimeIntervalMicroSeconds audioClipEndTimeInt =   iMovie.AudioClipEndTime(i).Int64(); 

        if ((audioClipStartTimeInt == TTimeIntervalMicroSeconds(0)) && (iMovie.VideoClipCount() > 0))
            {
            TRect syncBox;
            syncBox.iTl.iX = box.iTl.iX;
            syncBox.iTl.iY = iVideoBarBox.iBr.iY;
            syncBox.iBr.iX = box.iTl.iX + 1;
            syncBox.iBr.iY = iAudioBarBox.iTl.iY;

            gc.DrawRect(syncBox);
            }

        while (videoIndex < iMovie.VideoClipCount())
            {
            TInt oldIndex = videoIndex;
            if ( iMovie.VideoClipEndTime( oldIndex ) < audioClipStartTimeInt )
                {
                videoIndex++;
                }
            else if ( iMovie.VideoClipEndTime( oldIndex ) == audioClipStartTimeInt )
                {
                TRect syncBox;
                syncBox.iTl.iX = box.iTl.iX;
                syncBox.iTl.iY = iVideoBarBox.iBr.iY;
                syncBox.iBr.iX = box.iTl.iX + 1;
                syncBox.iBr.iY = iAudioBarBox.iTl.iY;

                gc.DrawRect(syncBox);
                break;
                }
            else
                {
                break;
                }
            }

        while (videoIndex < iMovie.VideoClipCount())
            {
            TInt oldIndex = videoIndex;

            if ( iMovie.VideoClipEndTime( oldIndex ) < audioClipEndTimeInt )
                {
                videoIndex++;
                }
            else if ( iMovie.VideoClipEndTime( oldIndex ) == audioClipEndTimeInt )
                {
                TRect syncBox;
                syncBox.iTl.iX = box.iBr.iX - 1;
                syncBox.iTl.iY = iVideoBarBox.iBr.iY;
                syncBox.iBr.iX = box.iBr.iX;
                syncBox.iBr.iY = iAudioBarBox.iTl.iY;

                gc.DrawRect(syncBox);
                break;
                }
            else
                {
                break;
                }
            }
        }

    box.iTl.iY = iVideoBarBox.iTl.iY;
    box.iBr.iY = iVideoBarBox.iBr.iY;
    barWidth = iVideoBarBox.Width();


    TRect videoBoxes  = TRect();
    
    for (i = 0; i < iMovie.VideoClipCount(); i++)
        {
        box.iTl.iX = iVideoBarBox.iTl.iX
            + static_cast<TInt32>((iMovie.VideoClipStartTime( i ).Int64() * barWidth) / barDuration);
        box.iBr.iX = iVideoBarBox.iTl.iX
            + static_cast<TInt32>((iMovie.VideoClipEndTime( i ).Int64() * barWidth) / barDuration) + 1;
        
        videoBoxes.Resize( box.Size() );
        
        gc.DrawRect(box);
        //Draw thumbnail in video boxes on the timeline. 
        thumbnail = iVideoItemArray[i]->iTimelineBitmap;
        if ( thumbnail )
            {
            TPoint pos( box.iTl.iX+1, box.iTl.iY+1 );

            TSize pieceSize = TSize(STATIC_CAST(TInt, (box.Height()-2)*1.22), box.Height()-2);
            
            if ( pieceSize.iWidth >= box.Width()-2  )
                {
                pieceSize.SetSize( box.Width()-2, box.Height()-2 );
                }
            TRect pieceRect( TPoint(0,0), pieceSize );
            gc.BitBlt( pos, thumbnail, pieceRect );
            }
        }

    gc.SetPenStyle(CGraphicsContext::ESolidPen);
    gc.SetPenColor(rgbUnselectedBorder);
    gc.SetBrushColor(rgbUnselectedTransition);

    box.iTl.iY += (box.Height() - iTransitionMarkerSize.iHeight) / 2; 
    box.iBr.iY = box.iTl.iY + iTransitionMarkerSize.iHeight;
    for (i = iMovie.VideoClipCount(); (i >= 0) && (iMovie.VideoClipCount() > 0); i--)
        {
        if ((i == 0) && (iMovie.VideoClipCount() > 0))
            {
            box.iTl.iX = iVideoBarBox.iTl.iX
                - (iTransitionMarkerSize.iWidth / 2);
            }
        else
            {
            box.iTl.iX = iVideoBarBox.iTl.iX
                + static_cast<TInt32>((iMovie.VideoClipEndTime( i-1 ).Int64() * barWidth) / barDuration)
                - (iTransitionMarkerSize.iWidth / 2);
            }

        box.iBr.iX = box.iTl.iX + iTransitionMarkerSize.iWidth;

        
        gc.DrawRect(box);
        }

    //Draw play head.
    TRect bar( iBarArea );//bar rect.
    const TUint barY = 6;
    TUint width = videoBoxes.Width();
    TInt audioTrackWidth(0);
    if ( audioBoxes.iBr.iX > 0 )
        {
        audioTrackWidth = audioBoxes.iBr.iX - iAudioTrackBox.iTl.iX;
        }

    if ( videoBoxes.Width() >= audioTrackWidth )
        {
        width = videoBoxes.Width();
        }
    else
        {
        width = audioTrackWidth;
        }
    TUint totalTime = static_cast<TInt32>(iMovie.Duration().Int64() /1000);//( iVideoDisplay->TotalLengthL().Int64() / 1000 ).Low();

    if ( totalTime == 0 ) 
        {
        totalTime = 1;
        }

    TInt currentPointX = iCurrentPoint * width / totalTime + iVideoBarBox.iTl.iX;
    iCurrentPointX = currentPointX;
    
    bar.iTl.iY += barY;
    bar.iTl.iX += barY;
    bar.iTl.iX += iVideoTrackIcon->SizeInPixels().iWidth;
    bar.iBr.iY -= barY / 2;
    bar.iBr.iX -= barY;

                    
    gc.SetBrushColor( KRgbBlack );
    gc.SetBrushStyle( CGraphicsContext::ESolidBrush );
    gc.SetPenColor( KRgbDarkGray );
    gc.SetPenStyle( CGraphicsContext::ESolidPen );

    TPoint inPointList[3];
    inPointList[0] = TPoint( currentPointX - barY + 1, iBarArea.iTl.iY - 5 );
    inPointList[1] = TPoint( currentPointX + barY - 1, iBarArea.iTl.iY - 5 );
    inPointList[2] = TPoint( currentPointX, bar.iTl.iY - 6  );
    gc.DrawPolygon( inPointList, 3 );

    inPointList[0] = TPoint( currentPointX - barY + 1, iBarArea.iBr.iY + 5 );
    inPointList[1] = TPoint( currentPointX + barY - 1, iBarArea.iBr.iY + 5 );
    inPointList[2] = TPoint( currentPointX, bar.iBr.iY  );
    gc.DrawPolygon( inPointList, 3 );

    gc.SetPenSize( TSize( 3, 1 ) );
    gc.DrawLine( TPoint( currentPointX, bar.iTl.iY - 5 ), 
    TPoint( currentPointX, bar.iBr.iY ) );
    
    Window().EndRedraw();
    DeactivateGc();
    }

void CVeiEditVideoContainer::DialogDismissedL( TInt aButtonId )
    {
    iTakeSnapshot = EFalse;     
    if ( aButtonId == -1 )  
        { 
        // when pressing cancel button.
        /*if ( iTempVideoInfo && !iFrameReady)
            {                           
            iTempVideoInfo->CancelFrame();          
            }       
            */
        CancelSnapshotSave();
        }       
    }
    
void CVeiEditVideoContainer::CancelSnapshotSave()
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::CancelSnapshotSave: in");
    if ( iConverter )
        {
        LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::CancelSnapshotSave: 1");       
        iConverter->Cancel();
        iConverter->CancelEncoding(); //also close the file
        }
    if ( iSaveToFileName )
        {
        LOGFMT(KVideoEditorLogFile, "CVeiEditVideoContainer::CancelSnapshotSave: 2, iSaveToFileName:%S", iSaveToFileName);

        RFs&    fs = iEikonEnv->FsSession(); 
        TInt result = fs.Delete( *iSaveToFileName ); 
        delete iSaveToFileName;
        iSaveToFileName = NULL;
        }
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::CancelSnapshotSave: out"); 
    }   

void CVeiEditVideoContainer::SetCurrentIndex( TInt aCurrentIndex )
    {
    if ( (iCursorLocation == ECursorOnAudio) ||
        (iCursorLocation == ECursorOnEmptyAudioTrack) )
        {
        iAudioCursorPos = aCurrentIndex;
        }
    else
        {
        iVideoCursorPos = aCurrentIndex;
        }
    }

void CVeiEditVideoContainer::NotifyVideoClipAdded( CVedMovie& /*aMovie*/, 
                                                        TInt aIndex )
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipAdded: in");

    CStoryboardVideoItem* item=0;

    if ( iView.WaitMode() == CVeiEditVideoView::EDuplicating )
        {
        TTimeIntervalMicroSeconds cutInTime = iMovie.VideoClipCutInTime( aIndex - 1 );
        TTimeIntervalMicroSeconds cutOutTime = iMovie.VideoClipCutOutTime( aIndex - 1 );

        iMovie.VideoClipSetColorEffect( aIndex, iMovie.VideoClipColorEffect( aIndex - 1 ) );
        iMovie.VideoClipSetColorTone( aIndex, iMovie.VideoClipColorTone( aIndex - 1 ) );                
        iMovie.SetVideoClipVolumeGainL(aIndex, iMovie.GetVideoClipVolumeGainL(aIndex - 1));

        iMovie.VideoClipSetSpeed( aIndex, iMovie.VideoClipSpeed( aIndex - 1 ) );
        iMovie.VideoClipSetMuted( aIndex, iMovie.VideoClipIsMuted( aIndex - 1 ) );

        TBool isFile(iMovie.VideoClipInfo( aIndex )->Class() == EVedVideoClipClassFile);
        if ( isFile )
            {
            iMovie.VideoClipSetCutInTime( aIndex, cutInTime );
            iMovie.VideoClipSetCutOutTime( aIndex, cutOutTime );
            }
        /* Copy bitmaps, names etc. to new storyboarditem. */
        TRAP_IGNORE( item = CStoryboardVideoItem::NewL( 
            *iVideoItemArray[aIndex-1]->iIconBitmap, 
            *iVideoItemArray[aIndex-1]->iIconMask, 
            *iVideoItemArray[aIndex-1]->iFilename, 
            iVideoItemArray[aIndex-1]->iIsFile,
            *iVideoItemArray[aIndex-1]->iAlbumName ) );
        if (item)
            {
            item->InsertLastFrameL( *iVideoItemArray[aIndex-1]->iLastFrameBitmap,
                *iVideoItemArray[aIndex-1]->iLastFrameMask );
            item->InsertTimelineFrameL( *iVideoItemArray[aIndex-1]->iTimelineBitmap,
                *iVideoItemArray[aIndex-1]->iTimelineMask );

            iVideoItemArray.Insert( item, aIndex );
            }

        iVideoCursorPos = aIndex;
        iCursorLocation = ECursorOnClip; 
        DrawDeferred();
        iView.SetWaitMode( CVeiEditVideoView::ENotWaiting );
        SetCursorLocation( CursorLocation() );
        return;
        }

    iCurrentlyProcessedIndex = aIndex;
    TFileName fileName;

    TBool isFile(iMovie.VideoClipInfo( aIndex )->Class() == EVedVideoClipClassFile);
    if ( isFile )
        {
        fileName = iMovie.VideoClipInfo( aIndex )->FileName();
        }
    else
        {
        CVedVideoClipInfo* info = iMovie.VideoClipInfo( aIndex );

        if (info->Class() == EVedVideoClipClassGenerated) 
            {
            if (info->Generator()->Uid() == KUidTitleClipGenerator) 
                {
                fileName = iMovie.VideoClipInfo( aIndex )->DescriptiveName();
                }
            else if (info->Generator()->Uid() == KUidImageClipGenerator) 
                {
                CVeiImageClipGenerator* generator = STATIC_CAST(CVeiImageClipGenerator*, info->Generator());
                fileName = generator->ImageFilename();
                }
            }
        }

    TFileName albumName;
    GetAlbumL( fileName, albumName );

    TRAPD( error, (item = CStoryboardVideoItem::NewL( *iNoThumbnailIcon, 
        *iNoThumbnailIconMask, fileName, isFile, albumName )) );

    if ( error == KErrNone )
        {
        iVideoItemArray.Insert( item, iCurrentlyProcessedIndex );
    
        iVideoCursorPos = aIndex;
        iCursorLocation = ECursorOnClip; 
        iZoomFactorX = 0;
        iZoomFactorY = 0;

        TRAPD( frameError, StartFrameTakerL( aIndex ) );
        if ( frameError )
            {
            iMovie.RemoveVideoClip( aIndex );
            iView.ShowErrorNote( R_VEI_VIDEO_FAILED );
            iView.CancelWaitDialog();
            iView.AddNext();
            }
        }
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipAdded: out");
    }

void CVeiEditVideoContainer::NotifyVideoClipAddingFailed( CVedMovie& 
                /*aMovie*/, TInt DEBUGLOG_ARG(aError) )
    {
    LOGFMT(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipAddingFailed: In and Out, aError:%d", aError);
    }

void CVeiEditVideoContainer::NotifyVideoClipInfoReady( CVedVideoClipInfo& aInfo,
                                                      TInt aError )
    {
    LOGFMT(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipInfoReady: In, aError:%d", aError);
    // video info ready, get thumbnail
    if (KErrNone == aError)
        {   
        if (iTempVideoInfo)     
            {
            delete iTempVideoInfo;
            iTempVideoInfo = NULL;  
            }
        iTempVideoInfo = &aInfo;
        TSize thumbResolution;
        thumbResolution = iVideoDisplay->GetScreenSize();
        /* Check if cursor is on transition. When editvideocontainer is activated
            and right key is pressed very fast application crashes without this check */

        TInt currentIndex;
        currentIndex = CurrentIndex();

        if ( iCursorLocation == ECursorOnTransition )
            {
            currentIndex--;
            }
        LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipInfoReady: 1");
        //we are in preview mode.
        if ( iView.EditorState() == CVeiEditVideoView::EQuickPreview )
            {
            return;
            }
        
        TTimeIntervalMicroSeconds cutInTime = iMovie.VideoClipCutInTime( currentIndex );
        TTimeIntervalMicroSeconds cutOutTime = iMovie.VideoClipCutOutTime( currentIndex );

        TInt firstThumbNailIndex = iTempVideoInfo->GetVideoFrameIndexL( cutInTime );    
    //  TInt lastThumbNailIndex = aInfo.GetVideoFrameIndexL( cutOutTime );    
    //  lastThumbNailIndex--;   

        TDisplayMode thumbnailDisplayMode( ENone ); 

        LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipInfoReady: 2");                        
        /* :
         check out on every phone before releasing whether videodisplay should be stopped before starting
         asynchronous GetFrameL()
         see how EStateGettingFrame is handled in SetPreviewState 
         Stopping frees memory and it is needed in memory sensible devices 
        */
        TRAPD( err, iTempVideoInfo->GetFrameL( *this, firstThumbNailIndex, &thumbResolution, 
            thumbnailDisplayMode ) );
        if (KErrNone == err)
            {
            SetPreviewState(EStateGettingFrame);    
            }       
        LOGFMT(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipInfoReady: 3, err:%d", err);    
        if ( KErrNone != err )
            {
            if ( iProgressDialog )
                {
                iProgressDialog->GetProgressInfoL()->SetAndDraw( 100 );
                TRAP_IGNORE( iProgressDialog->ProcessFinishedL() );
                //iProgressDialog = NULL;
                }
            User::Panic( _L("VideoEditor"), 65 );
            }
        }
    
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipInfoReady: Out");  
    }

void CVeiEditVideoContainer::NotifyVideoClipFrameCompleted(CVedVideoClipInfo& /*aInfo*/, 
                                               TInt aError, 
                                               CFbsBitmap* aFrame)
    {
    LOGFMT2(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipFrameCompleted: In, aError:%d, iTakeSnapshot:%d", aError, iTakeSnapshot);  
    
    iFrameReady = ETrue;
        
    if (EStateGettingFrame == iPreviewState)
        {
        SetPreviewState(iPreviousPreviewState); 
        // SetEditorState is effective because iPreviewState is changed     
        iView.SetEditorState( iView.EditorState() );
        }       

    if(KErrNone == aError && aFrame)
        {
        LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipFrameCompleted: 3 calling iVideoDisplay->ShowPictureL()"); 
        //TRAP_IGNORE(iVideoDisplay->ShowPictureL( *aFrame ));

        if ( iTakeSnapshot)
            {
            LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipFrameCompleted: 2");   

            // For some reason some phones crash in taking snapshot if this progress note is started 
            // earlier from TakeSnapshotL, that is why it is started now here
            //StartProgressDialogL(R_VEI_PROGRESS_NOTE_WITH_CANCEL, R_VEI_PROGRESS_NOTE_SAVING_IMAGE);
            iConverter->SetBitmap( aFrame );
            SaveSnapshotL();
            return;         
            }
        TRAP_IGNORE(iVideoDisplay->ShowPictureL( *aFrame ));    
        delete aFrame;
        aFrame = NULL;  
        if (iProgressDialog )
            {
            iProgressDialog->GetProgressInfoL()->SetAndDraw( 100 );
            TRAP_IGNORE( iProgressDialog->ProcessFinishedL() );
            //iProgressDialog = NULL;       
            }
        DrawDeferred();
        if (iTakeSnapshotWaiting)
            {
            if (! iCallBack)
                {       
                TCallBack cb (CVeiEditVideoContainer::AsyncTakeSnapshot, this);
                iCallBack = new (ELeave) CAsyncCallBack(cb, CActive::EPriorityStandard);
                }
            iCallBack->CallBack();              
            }
        return;
        }   
    
    if ( aFrame )
        {
        LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipFrameCompleted: 4");
        delete aFrame;
        aFrame = NULL;  
        }
    
    if (iProgressDialog )
        {
        iProgressDialog->GetProgressInfoL()->SetAndDraw( 100 );
        TRAP_IGNORE( iProgressDialog->ProcessFinishedL() );
        //iProgressDialog = NULL;       
        }
    else if ( KErrNone == aError)
        {
        iView.HandleCommandL( EAknSoftkeyOk );
        }
/* In case of an error, we'll do nothing. */
/* If clip is too short, we won't get new thumbnail, so use old one->return; */
    if (KErrNone != aError)
        {
        iCurrentlyProcessedIndex = -1;
        return;
        }
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipFrameCompleted: Out");     
    }

void CVeiEditVideoContainer::NotifyVideoClipGeneratorSettingsChanged(CVedMovie& /*aMovie*/,
                                                         TInt aIndex) 
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipGeneratorSettingsChanged: In");        
    iCurrentlyProcessedIndex = aIndex;
    StartFrameTakerL( aIndex );
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipGeneratorSettingsChanged: Out");       
    }

void CVeiEditVideoContainer::NotifyVideoClipDescriptiveNameChanged(CVedMovie& /*aMovie*/, TInt /*aIndex*/)
    {   
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipDescriptiveNameChanged: In");      
    TFileName fileName;
    fileName = iMovie.VideoClipInfo( CurrentIndex() )->DescriptiveName();
    iInfoDisplay->SetName(fileName);
    iVideoDisplay->DrawDeferred();
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipDescriptiveNameChanged: Out");     
    }

void CVeiEditVideoContainer::NotifyMovieQualityChanged(CVedMovie& /*aMovie*/)
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyMovieQualityChanged: In and Out");      
    }
void CVeiEditVideoContainer::NotifyVideoClipRemoved( 
                                    CVedMovie& /*aMovie*/, TInt aIndex )
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipRemoved: In");     
    CStoryboardVideoItem* item = iVideoItemArray[ aIndex ];
    iVideoItemArray.Remove( aIndex );
    delete item;
    SetCursorLocation( ECursorOnClip );
    DrawDeferred();
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipRemoved: Out");        
    }

void CVeiEditVideoContainer::NotifyVideoClipIndicesChanged( 
                    CVedMovie& /*aMovie*/, TInt aOldIndex, TInt aNewIndex )
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipIndicesChanged: In");      
    CStoryboardVideoItem* item = iVideoItemArray[ aOldIndex ];
    iVideoItemArray.Remove( aOldIndex );    
    iVideoItemArray.Insert( item, aNewIndex );
    DrawDeferred();
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipIndicesChanged: Out");     
    }

void CVeiEditVideoContainer::NotifyVideoClipTimingsChanged( 
                                    CVedMovie& aMovie, TInt aIndex )
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipTimingsChanged: In");      
    /* Check is slowmotion on */
    if ( iMovie.VideoClipSpeed( iVideoCursorPos ) != 1000 )
        {
        iEffectSymbols->SetSlowMotionIconVisibility(ETrue);
        }
    else
        {
        // Engine sets mute automatically on if speed is set under 1000, but it does not return
        // mute state to original if speed is reset to 1000, reclaimed to nokia
        // problem is that we should respect user's mute settings prior to slow motion
        // i.e. when removing slow motion mute settings should be reset to user defined value
    //  iMovie.VideoClipSetMuted(aIndex, EFalse);
        iEffectSymbols->SetSlowMotionIconVisibility(EFalse);
        }
    /* If SM is on, audio is muted */
    if ((iMovie.VideoClipIsMuted(aIndex) != EFalse) || 
            (iMovie.VideoClipEditedHasAudio(aIndex) == EFalse))
        {       
        iEffectSymbols->SetVolumeMuteIconVisibility(ETrue); 
        }
    else
        {       
        iEffectSymbols->SetVolumeMuteIconVisibility(EFalse);
        }

    TTimeIntervalMicroSeconds editedDuration = aMovie.VideoClipEditedDuration( aIndex );
    
    iInfoDisplay->SetDuration( editedDuration );

    DrawTrackBoxes();
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipTimingsChanged: Out");     
    }
    
void CVeiEditVideoContainer::NotifyVideoClipColorEffectChanged( 
                                    CVedMovie& /*aMovie*/, TInt aIndex )
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipColorEffectChanged: In");      

    if ( iView.WaitMode() != CVeiEditVideoView::EDuplicating )
        TRAP_IGNORE( UpdateThumbnailL( aIndex ) );

    SetColourToningIcons(aIndex);   
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipColorEffectChanged: Out");     
    }

void CVeiEditVideoContainer::NotifyVideoClipAudioSettingsChanged( 
                                    CVedMovie& /*aMovie*/, TInt aIndex )
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipAudioSettingsChanged: In");        
    /* Check is Mute on */
    if ((iMovie.VideoClipIsMuted(aIndex) != EFalse) || 
            (iMovie.VideoClipEditedHasAudio(aIndex) == EFalse))
        {       
        iEffectSymbols->SetVolumeMuteIconVisibility(ETrue); 
        }
    else
        {
        iEffectSymbols->SetVolumeMuteIconVisibility(EFalse);
        }
    DrawDeferred();
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipAudioSettingsChanged: Out");       
    }

void CVeiEditVideoContainer::NotifyStartTransitionEffectChanged( 
                                                    CVedMovie& aMovie )
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyStartTransitionEffectChanged: In");     
    iInfoDisplay->SetName( *iTransitionInfo->StartTransitionName( aMovie.StartTransitionEffect() ) );
        
    ShowStartAnimationL( aMovie.StartTransitionEffect() );
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyStartTransitionEffectChanged: Out");        
    }

void CVeiEditVideoContainer::NotifyMiddleTransitionEffectChanged( 
                                    CVedMovie& aMovie, TInt aIndex )
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyMiddleTransitionEffectChanged: In");        
    iInfoDisplay->SetName( *iTransitionInfo->MiddleTransitionName( aMovie.MiddleTransitionEffect( aIndex ) ) );

    ShowMiddleAnimationL( aMovie.MiddleTransitionEffect( aIndex ) );
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyMiddleTransitionEffectChanged: Out");       
    }

void CVeiEditVideoContainer::NotifyEndTransitionEffectChanged( 
                                                    CVedMovie& aMovie )
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyEndTransitionEffectChanged: In");       
    iInfoDisplay->SetName( *iTransitionInfo->EndTransitionName( aMovie.EndTransitionEffect() ) );
    ShowEndAnimationL( aMovie.EndTransitionEffect() );
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyEndTransitionEffectChanged: Out");      
    }

void CVeiEditVideoContainer::NotifyAudioClipAdded( 
                                            CVedMovie& aMovie, TInt aIndex )
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyAudioClipAdded: In");
    CStoryboardAudioItem* item;
/*  When audio clip is duplicated, check if original was recorded.
    TMPXXXXX.XXX name is not shown on main pane if iRecordedAudio is ETrue.
**/
    if ( (aIndex > 0 ) && (iView.WaitMode() == CVeiEditVideoView::EDuplicating ))
        {   
        if ( iAudioItemArray[aIndex-1]->iRecordedAudio == 1 )
            {
            iRecordedAudio = ETrue; 
            }
        }
    
    if (CVeiEditVideoView::EDuplicating == iView.WaitMode())
        {   
        //copy adjust volume parameter from original clip (aIndex - 1)
        TReal adjustVolume = aMovie.GetAudioClipVolumeGainL(aIndex-1);
        aMovie.SetAudioClipVolumeGainL(aIndex, (TInt)(adjustVolume));
        }   
    item = CStoryboardAudioItem::NewLC( iRecordedAudio,
                        aMovie.AudioClipInfo( aIndex )->FileName() );

    iAudioCursorPos = aIndex;

    iAudioItemArray.Insert( item, aIndex );
    CleanupStack::Pop( item );

    iView.SetWaitMode( CVeiEditVideoView::ENotWaiting );    
    SetCursorLocation( ECursorOnAudio );
    iRecordedAudio = EFalse;
    DrawDeferred();
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyAudioClipAdded: Out");
    }

void CVeiEditVideoContainer::NotifyAudioClipAddingFailed( 
                                    CVedMovie& /*aMovie*/, TInt DEBUGLOG_ARG(aError) )
    {
    LOGFMT(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyAudioClipAddingFailed: In, aError:%d", aError);
    DrawDeferred();
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyAudioClipAddingFailed: Out");
    }

void CVeiEditVideoContainer::NotifyAudioClipRemoved( 
                                    CVedMovie& /*aMovie*/, TInt aIndex )
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyAudioClipRemoved: In");
    CStoryboardAudioItem* item = iAudioItemArray[aIndex];
    iAudioItemArray.Remove(aIndex);
    delete item;
    SetCursorLocation( ECursorOnAudio );
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyAudioClipRemoved: Out");
    }

void CVeiEditVideoContainer::NotifyAudioClipIndicesChanged( 
                        CVedMovie& /*aMovie*/, TInt aOldIndex, TInt aNewIndex )
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyAudioClipIndicesChanged: In");
    CStoryboardAudioItem* item = iAudioItemArray[ aOldIndex ];
    iAudioItemArray.Remove( aOldIndex );
    TInt err = iAudioItemArray.Insert( item, aNewIndex );
    if ( err != KErrNone )
        {
        TBuf<30>buf;
        buf.Format( _L("Audio clip moving failed (%d)."), err );
        }

    iAudioCursorPos = aNewIndex;
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyAudioClipIndicesChanged: Out");
    }

void CVeiEditVideoContainer::NotifyAudioClipTimingsChanged( 
                                CVedMovie& /*aMovie*/, TInt /*aIndex*/ )
    {   
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyAudioClipTimingsChanged: In");
    IsAudioClipCutted();
    DrawDeferred();
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyAudioClipTimingsChanged: Out");
    }

void CVeiEditVideoContainer::NotifyMovieReseted( CVedMovie& /*aMovie*/ )
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyMovieReseted: In");
    iAudioItemArray.ResetAndDestroy();
    iVideoItemArray.ResetAndDestroy();
    iVideoCursorPos = 0;
    iAudioCursorPos = 0;
    iCursorLocation = ECursorOnClip;
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyMovieReseted: Out");
    }

void CVeiEditVideoContainer::NotifyMovieOutputParametersChanged(CVedMovie& /*aMovie*/)
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyMovieOutputParametersChanged: In and Out");
    // @
    }

void CVeiEditVideoContainer::NotifyAudioClipDynamicLevelMarkInserted(CVedMovie& /*aMovie*/, 
                                                         TInt /*aClipIndex*/, 
                                                         TInt /*aMarkIndex*/)
    {
    // @
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyAudioClipDynamicLevelMarkInserted: In and out");
    }

void CVeiEditVideoContainer::NotifyAudioClipDynamicLevelMarkRemoved(CVedMovie& /*aMovie*/, 
                                                        TInt /*aClipIndex*/, 
                                                        TInt /*aMarkIndex*/)
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyAudioClipDynamicLevelMarkRemoved: In and out");
    // @
    }

void CVeiEditVideoContainer::NotifyVideoClipDynamicLevelMarkInserted(CVedMovie& /*aMovie*/,
                                                         TInt /*aClipIndex*/, 
                                                         TInt /*aMarkIndex*/)
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipDynamicLevelMarkInserted: In and out");
    // @
    }

void CVeiEditVideoContainer::NotifyVideoClipDynamicLevelMarkRemoved(CVedMovie& /*aMovie*/, 
                                                        TInt /*aClipIndex*/, 
                                                        TInt /*aMarkIndex*/)
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoClipDynamicLevelMarkRemoved: In and out");
    // @
    }

void CVeiEditVideoContainer::UpdateThumbnailL( TInt aIndex )
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::UpdateThumbnailL: In");
    /*HBufC* stringholder;
    
    if (iProgressNote)
    {
        delete iProgressNote;
        iProgressNote = NULL;   
    }
    
    iProgressNote = 
        new (ELeave) CAknProgressDialog(REINTERPRET_CAST(CEikDialog**, 
        &iProgressNote), ETrue);
    iProgressNote->SetCallback(this);
    
    if( iView.WaitMode() == CVeiEditVideoView::EProcessingMovieForCutting )
        {
        stringholder = StringLoader::LoadLC( R_VEI_PROGRESS_NOTE_CUTTING_VIDEO, iEikonEnv );
        }
    else
        {
        stringholder = StringLoader::LoadLC( R_VEI_PROGRESS_NOTE_COLOR_EFFECT, iEikonEnv );
        }
        
    iProgressNote->ExecuteDlgLD( R_VEI_PROGRESS_NOTE );
    iProgressNote->SetTextL( *stringholder );
    CleanupStack::PopAndDestroy( stringholder );

    iProgressNote->GetProgressInfoL()->SetFinalValue(100);
    */
    TInt resid; 
    if( iView.WaitMode() == CVeiEditVideoView::EProcessingMovieForCutting )
        {
        resid = R_VEI_PROGRESS_NOTE_CUTTING_VIDEO;
        }
    else
        {
        resid = R_VEI_PROGRESS_NOTE_COLOR_EFFECT;
        }
    StartProgressDialogL(R_VEI_PROGRESS_NOTE, resid);

    iCurrentlyProcessedIndex = iVideoCursorPos; 

    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::UpdateThumbnailL: 2");
    StartFrameTakerL( aIndex );
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::UpdateThumbnailL: Out");
    }

// ---------------------------------------------------------
// CVeiEditVideoContainer::ConvertBW(CFbsBitmap& aBitmap)     
// ---------------------------------------------------------
//
void CVeiEditVideoContainer::ConvertBW( CFbsBitmap& aBitmap ) const
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::ConvertBW: In");
    TInt width =  aBitmap.SizeInPixels().iWidth;
    TInt height = aBitmap.SizeInPixels().iHeight;

    TBitmapUtil bitmapUtil( &aBitmap );
    bitmapUtil.Begin( TPoint(0,0) );

    for ( TInt y=0;y<height;y++ )
        {
        for ( TInt x=0;x<width;x++ )
            {
            bitmapUtil.SetPos( TPoint( x,y ) );

            TUint32 colorr = ( bitmapUtil.GetPixel() );

            TRgb vari = TRgb::Color64K( colorr );

            TInt red = vari.Red();
            TInt green = vari.Green();
            TInt blue = vari.Blue();
            
            TUint Yy = STATIC_CAST( TUint, red*0.299 + green*0.587 + blue*0.114 );

            bitmapUtil.SetPixel( TRgb(Yy,Yy,Yy).Color64K() );
            }
        }
    bitmapUtil.End();
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::ConvertBW: Out");
    }

// ---------------------------------------------------------
// CVeiEditVideoContainer::ConvertToning(CFbsBitmap& aBitmap)     
// ---------------------------------------------------------
//
void CVeiEditVideoContainer::ConvertToning( CFbsBitmap& aBitmap ) const
    {   
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::ConvertToning: In");
//  TInt ind = CurrentIndex();
    TRgb toning = iMovie.VideoClipColorTone(CurrentIndex());
    
    TInt width =  aBitmap.SizeInPixels().iWidth;
    TInt height = aBitmap.SizeInPixels().iHeight;

    TBitmapUtil bitmapUtil( &aBitmap );
    bitmapUtil.Begin( TPoint(0,0) );
    
    TInt R_ct = toning.Red();
    TInt G_ct = toning.Green();         
    TInt B_ct = toning.Blue();      
                
    /*
    //vihreä
    TInt R_ct = 185;
    TInt G_ct = 255;            
    TInt B_ct = 0;          
    */
    
    TInt kr =   45808*R_ct - 38446*G_ct -  7362*B_ct + 32768;
    TInt kg = - 19496*R_ct + 26952*G_ct -  3750*B_ct + 32768;
    TInt kb = - 19608*R_ct - 38184*G_ct + 57792*B_ct + 32768;  

    for ( TInt y=0;y<height;y++ )
        {
        for ( TInt x=0;x<width;x++ )
            {
            bitmapUtil.SetPos( TPoint( x,y ) );
                                                                                
            TUint32 colorr = ( bitmapUtil.GetPixel() );
            TRgb vari = TRgb::Color64K( colorr );
        
            TInt alpha = 19668*vari.Red() + 38442*vari.Green() + 7450*vari.Blue(); 
            TInt R_out = (alpha + kr)>>16;
            TInt G_out = (alpha + kg)>>16;
            TInt B_out = (alpha + kb)>>16;                          
            
            if(R_out<0) R_out=0;  if(R_out>255) R_out=255;
            if(G_out<0) G_out=0;  if(G_out>255) G_out=255;
            if(B_out<0) B_out=0;  if(B_out>255) B_out=255;                      

            bitmapUtil.SetPixel( TRgb(R_out,G_out,B_out).Color64K() );
            }
        }
    bitmapUtil.End();
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::ConvertToning: Out");
    }
    
    
// ---------------------------------------------------------
// CVeiEditVideoContainer::HandleControlEventL(
//     CCoeControl* aControl,TCoeEvent aEventType)
// ---------------------------------------------------------
//
void CVeiEditVideoContainer::HandleControlEventL(
    CCoeControl* /*aControl*/,TCoeEvent /*aEventType*/)
    {
    // : Add your control event handler code here
    }

// ----------------------------------------------------------------------------
// CVeiEditVideoContainer::HandlePointerEventL
// From CCoeControl
// ----------------------------------------------------------------------------
//		
void CVeiEditVideoContainer::HandlePointerEventL( const TPointerEvent& aPointerEvent )
	{
	LOG( KVideoEditorLogFile, "CVeiEditVideoContainer::HandlePointerEventL(): In" );
    	
	if( AknLayoutUtils::PenEnabled() )
		{
		CCoeControl::HandlePointerEventL( aPointerEvent );
				
		switch( aPointerEvent.iType )
			{
			case TPointerEvent::EButton1Down:
				{
				LOG( KVideoEditorLogFile, "CVeiEditVideoContainer::HandlePointerEventL(): EButton1Down" );
								
                // Initialise the touch related member variables
				iIsVideoDrag = EFalse;
				iIsVideoTapped = EFalse;
				
				// the user taps the timeline bar				
				if( iVideoBarBox.Contains( aPointerEvent.iPosition ) )
					{
					iCursorLocation = ECursorOnClip;
                    HandleVideoTimelineTouchL( aPointerEvent );
					}
				// the user double-taps the cut bar
				else if (( iDummyCutBar->Rect().Contains( aPointerEvent.iPosition )) &&
				         ( aPointerEvent.iModifiers & EModifierDoubleClick ))
				    {
                    // open cut view    
                    iView.HandleCommandL( EVeiCmdEditVideoViewEditVideoCutting );
				    }
				// : the user double taps the thumbnail	(wait for the layout data)			
				// : the user taps the transition arrows (wait for the layout data)
				
				// the volume adjustment view is active and the user taps the volume slider
				else if (( EModeAdjustVolume == iSelectionMode ) && 
				         ( iVerticalSlider->Rect().Contains( aPointerEvent.iPosition )))
				    {
				    HandleVolumeSliderTouchL( aPointerEvent );
				    }
				break;
				}
			case TPointerEvent::EDrag:
				{
				LOGFMT( KVideoEditorLogFile, "CVeiEditVideoContainer::HandlePointerEventL(): \
				                              EDrag, iIsVideoTapped = %d", iIsVideoTapped );				
				                              
                // video drag takes effect only when the pointer has gone down inside a 
                // video clip (i.e. iIsVideoTapped == ETrue)
				if ( iVideoBarBox.Contains( aPointerEvent.iPosition ) && ( iIsVideoTapped ))
	    			{
	    			iIsVideoDrag = ETrue;
                    HandleVideoTimelineTouchL( aPointerEvent );
                    }
				// the volume adjustment view is active and the user taps the volume slider
				else if (( EModeAdjustVolume == iSelectionMode ) && 
				         ( iVerticalSlider->Rect().Contains( aPointerEvent.iPosition )))
				    {
				    HandleVolumeSliderTouchL( aPointerEvent );
				    }                    
                break;		
                }
			case TPointerEvent::EButton1Up:
				{				    
				// pen up event is handled if it was dragged
				if (iIsVideoDrag)
					{
                    iIsVideoDrag = EFalse;   
					// pressed position is inside the timeline bar
					if( iVideoBarBox.Contains( aPointerEvent.iPosition ) )
						{
						HandleVideoTimelineTouchL( aPointerEvent );
						}						
				    else
				        {
				        // the new position indicator has to be removed from the UI
				        iNewClipPosition = iClickedClip;
                        DrawNow();
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
	LOG( KVideoEditorLogFile, "CVeiEditVideoContainer::HandlePointerEventL(): Out" );		
	}


// ----------------------------------------------------------------------------
// CVeiEditVideoContainer::HandleVideoTimelineTouchL
// 
// ----------------------------------------------------------------------------
//	
void CVeiEditVideoContainer::HandleVideoTimelineTouchL( TPointerEvent aPointerEvent )
	{
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::HandleVideoTimelineTouchL(): In" );
    	
    CVeiEditVideoView::TEditorState state = iView.EditorState();

	if (( AknLayoutUtils::PenEnabled() ) && 
	    ( state!=CVeiEditVideoView::EPreview ) && 
	    ( state!=CVeiEditVideoView::EQuickPreview ))
		{	
        
        LOGFMT(KVideoEditorLogFile, "CVeiEditVideoContainer::HandleVideoTimelineTouchL, \
                                     iClickedClip:%d", iClickedClip );
				
		// move video clip by dragging
		if ( aPointerEvent.iType == TPointerEvent::EDrag )
		    {
		    LOGFMT(KVideoEditorLogFile, "CVeiEditVideoContainer::HandleVideoTimelineTouchL, EDrag, \
		                                 aPointerEvent.iPosition.iX = %d", aPointerEvent.iPosition.iX );
		    
		    // Find the clip that includes the current pointer position. 
		    TInt clipIncludingDrag = FindClickedClip( aPointerEvent.iPosition.iX );
		    
		    // the pen is inside the same clip where it went down
		    if (( clipIncludingDrag == iClickedClip ))
		        {
		        // the new position indicator has to be removed from UI
                iNewClipPosition = iClickedClip; 		            
		        }
	        
	        // the pen is on the empty part of the timeline
	        else if ( aPointerEvent.iPosition.iX > iEmptyVideoTimeLineRect.iTl.iX ) 
	            {
	            // the last clip can't be moved right
	            if ( iClickedClip < iMovie.VideoClipCount() - 1 )
		            {
		            iNewClipPosition = iMovie.VideoClipCount() - 1;
		            }
	            }
   		    // the pen is on the left end of a video clip
		    else if (( clipIncludingDrag >= 0) &&
		             ( iVideoItemRectArray[ clipIncludingDrag ].Contains( TPoint( aPointerEvent.iPosition.iX, iVideoBarBox.Center().iY ))) && 
		             ( aPointerEvent.iPosition.iX <= iVideoItemRectArray[ clipIncludingDrag ].iTl.iX + iVideoItemRectArray[ clipIncludingDrag ].Width()/2 ))
		        {
    		    // moving a clip from left to right
    		    if (( iClickedClip < clipIncludingDrag ) && ( iClickedClip < clipIncludingDrag - 1 ))
    		        {
        		    iNewClipPosition = clipIncludingDrag - 1;
    		        }
    		    // moving a clip from right to left
    		    else if ( iClickedClip > clipIncludingDrag )
    		        {
        		    iNewClipPosition = clipIncludingDrag;
    		        }
    		    else
    		        {
    		        iNewClipPosition = iClickedClip;    
    		        }
		        }
		    // the pen is on the right end of a video clip
		    else if (( clipIncludingDrag >= 0) &&
          		     ( iVideoItemRectArray[ clipIncludingDrag ].Contains( TPoint( aPointerEvent.iPosition.iX, iVideoBarBox.Center().iY ))) && 
		             ( aPointerEvent.iPosition.iX > iVideoItemRectArray[ clipIncludingDrag ].iTl.iX + iVideoItemRectArray[ clipIncludingDrag ].Width()/2 ))
		        {
    		    // moving a clip from left to right
    		    if (( iClickedClip < clipIncludingDrag ))
    		        {
        		    iNewClipPosition = clipIncludingDrag;
    		        }
    		    // moving a clip from right to left
    		    else if ( iClickedClip > clipIncludingDrag + 1)
    		        {
        		    iNewClipPosition = clipIncludingDrag + 1;
    		        }
    		    else
    		        {
    		        iNewClipPosition = iClickedClip;    
    		        }
		        }
		    }
		    
		// pen up event after dragging
        else if ( aPointerEvent.iType == TPointerEvent::EButton1Up )
            {
            iMovie.VideoClipSetIndex( iClickedClip, iNewClipPosition );    
            if ( iVideoCursorPos != iNewClipPosition ) // eliminates blinking of the already selected clip
                {
                iVideoCursorPos = iNewClipPosition;
                SetCursorLocation( ECursorOnClip );                                        
                }

            }
        // user taps a clip
		else if ( aPointerEvent.iType == TPointerEvent::EButton1Down ) 
		    {
			CalculateVideoClipRects();	

            iClickedClip = FindClickedClip( aPointerEvent.iPosition.iX );
            iNewClipPosition = iClickedClip;
				    
		    if ( iClickedClip >= 0 )
		        {
		        iIsVideoTapped = ETrue;
		        if ( iClickedClip != iVideoCursorPos ) // eliminates blinking of the already selected clip
			        {  
                    iVideoCursorPos = iClickedClip;
                    SetCursorLocation( ECursorOnClip );        
                    }
		        }
		    }

        // : when the user clicks a transition marker, the transition view should open
        
        DrawNow();
		LOG( KVideoEditorLogFile, "CVeiEditVideoContainer::HandleVideoTimelineTouchL(): Out" );
			
		}// PenEnabled
		
	}


// ----------------------------------------------------------------------------
// CVeiEditVideoContainer::HandleVolumeSliderTouchL
// 
// ----------------------------------------------------------------------------
//	
void CVeiEditVideoContainer::HandleVolumeSliderTouchL( TPointerEvent aPointerEvent )
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::HandleVolumeSliderTouchL in");    

    // calculate the new slider position
    TInt newSliderPosition;
    TInt volumeSliderSteps = KVolumeSliderMax - KVolumeSliderMin + 1;
    TInt pointerPosInSlider = aPointerEvent.iPosition.iY - iVerticalSlider->Rect().iTl.iY;
    newSliderPosition = (( volumeSliderSteps * pointerPosInSlider ) / iVerticalSlider->Rect().Height()) + KVolumeSliderMin;            
    
    iVerticalSlider->SetPosition( newSliderPosition);
    iVerticalSlider->DrawNow();
    
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::HandleVolumeSliderTouchL out");        
    }


// ----------------------------------------------------------------------------
// CVeiEditVideoContainer::ClipContainingClick
// 
// ----------------------------------------------------------------------------
//	
TInt CVeiEditVideoContainer::FindClickedClip( TInt aPressedPointX )
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::FindClickedClip in"); 
	// Video Timeline start and end points, and length
	TInt timelineLeftEnd( iVideoBarBox.iTl.iX );
	TInt timelineRightEnd( iVideoBarBox.iBr.iX );
	TInt totalPBLength( timelineRightEnd - timelineLeftEnd );

    // check which part of the timeline contains the click
    TInt clickedClip = -1;
    TInt i = 0;
    while (( clickedClip < 0) && ( i < iMovie.VideoClipCount() ))
        {
        if ( ( aPressedPointX > iVideoItemRectArray[i].iTl.iX ) && 
             ( aPressedPointX < iVideoItemRectArray[i].iBr.iX ))
            {
            clickedClip = i;    
            }
        else
            {
            i++;
            }
        }		
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::FindClickedClip out");         
    return clickedClip;
    }
            
            

void CVeiEditVideoContainer::SetCursorLocation( TCursorLocation aCursorLocation ) 
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::SetCursorLocation in");   
    
    iCursorLocation = aCursorLocation; 
    
    if ( iView.EditorState() == CVeiEditVideoView::EMixAudio)
        {       
        iVideoDisplay->MakeVisible( ETrue );
        iVideoDisplay->SetRect( iVideoDisplayBox ); 
        iVideoDisplay->ShowPictureL(*iAudioMixingIcon);
        iInfoDisplay->MakeVisible(EFalse);
        iHorizontalSlider->MakeVisible(ETrue);
        return;
        }           
    
    iTransitionDisplayLeft->MakeVisible( EFalse );
    iTransitionDisplayRight->MakeVisible( EFalse );
    iDummyCutBarLeft->MakeVisible( EFalse );
    iInfoDisplay->SetRect( iInfoDisplayBox );
    iEffectSymbols->MakeVisible( EFalse );
    if (CVeiEditVideoContainer::EModeSlowMotion != iSelectionMode)
        {
        // currently slow motion wastes processing time in background
        iVideoDisplay->StopAnimation(); 
        }   

    if ( iCursorLocation == ECursorOnClip && iInfoDisplay )
        {
        iVideoDisplay->MakeVisible( ETrue );
        iVideoDisplay->SetRect( iVideoDisplayBox ); 

        if ( iMovie.VideoClipCount() > 0 )
            {
            if ( (iVideoCursorPos > (iMovie.VideoClipCount()-1)))
                {
                iVideoCursorPos--;
                }

            TParse parser;

            parser.Set( iVideoItemArray[ CurrentIndex() ]->iFilename->Des(), NULL, NULL );          
            iVideoDisplay->ShowPictureL( *iVideoItemArray[CurrentIndex()]->iIconBitmap,
                    *iVideoItemArray[CurrentIndex()]->iIconMask );              
                
            iDummyCutBar->SetRect( iDummyCutBarBox );
            iDummyCutBar->Dim( EFalse );

            // : start using LAF data when it is available
            // iDummyCutBar->Rect() == (80, 160, 273, 204) includes the scissor icon and the progress bar
            iDummyCutBar->SetComponentRect( CVeiCutterBar::EProgressBar, TRect(115, 160,273,204)); 
            iDummyCutBar->SetComponentRect( CVeiCutterBar::ESliderLeftEndIcon, TRect(115, 160,135,204) );
            iDummyCutBar->SetComponentRect( CVeiCutterBar::ESliderMiddleIcon,  TRect(135,160,244,204) );
            iDummyCutBar->SetComponentRect( CVeiCutterBar::ESliderRightEndIcon,TRect(244,160,273,204) );                        
            iDummyCutBar->SetComponentRect( CVeiCutterBar::ESliderSelectedLeftEndIcon, TRect(115, 160,135,204) );
            iDummyCutBar->SetComponentRect( CVeiCutterBar::ESliderSelectedMiddleIcon,  TRect(135,160,244,204) );
            iDummyCutBar->SetComponentRect( CVeiCutterBar::ESliderSelectedRightEndIcon,TRect(244,160,273,204) );                     
            iDummyCutBar->SetComponentRect( CVeiCutterBar::EScissorsIcon, TRect(80, 167,110,197) );
            iDummyCutBar->SetComponentRect( CVeiCutterBar::ECutAreaBorderIcon,TRect(115,160,124,204) );            

            TTimeIntervalMicroSeconds clipDuration = iMovie.VideoClipInfo( CurrentIndex())->Duration();
            TTimeIntervalMicroSeconds clipCutInTime = iMovie.VideoClipCutInTime( CurrentIndex() );
            TTimeIntervalMicroSeconds clipCutOutTime = iMovie.VideoClipCutOutTime( CurrentIndex() );

            iDummyCutBar->SetTotalDuration( clipDuration );
            iDummyCutBar->SetInPoint( clipCutInTime );
            iDummyCutBar->SetOutPoint( clipCutOutTime );

            TTime       fileModified;

            RFs& fs = iEikonEnv->FsSession();
            if ( parser.ExtPresent() )
                {
                fs.Modified( *iVideoItemArray[CurrentIndex()]->iFilename, fileModified );
                }
            else
                {
                fileModified = iVideoItemArray[CurrentIndex()]->iDateModified;
                }

            if ( VideoEditorUtils::IsLandscapeScreenOrientation() ) //Landscape     
                {
                iInfoDisplay->SetLayout( CVeiTextDisplay::EEverything );
                }
            else
                {
                iInfoDisplay->SetLayout( CVeiTextDisplay::ENameAndDuration );
                }

            iInfoDisplay->SetLocation( *iVideoItemArray[CurrentIndex()]->iAlbumName );

            TFileName clipName = parser.Name();
            TTimeIntervalMicroSeconds editedDuration = iMovie.VideoClipEditedDuration( CurrentIndex() );
            iInfoDisplay->MakeVisible( ETrue );

            if( !CurrentClipIsFile() )
                {
                TFileName fileName;
                fileName = iMovie.VideoClipInfo( CurrentIndex() )->DescriptiveName();
                iInfoDisplay->SetName(fileName);
                }
            else
                {
                iInfoDisplay->SetName( clipName );
                }

            iInfoDisplay->SetDuration( editedDuration );
            iInfoDisplay->SetTime( fileModified );

            // *** IconBox drawing ****
            // indicator icons are hidden in preview state 
            CVeiEditVideoView::TEditorState state = iView.EditorState();
            if (state != CVeiEditVideoView::EPreview && state != CVeiEditVideoView::EQuickPreview)
                {
                if ((iMovie.VideoClipIsMuted(iVideoCursorPos) != EFalse) || 
                    (iMovie.VideoClipEditedHasAudio(iVideoCursorPos) == EFalse))
                    {               
                    iEffectSymbols->SetVolumeMuteIconVisibility(ETrue); 
                    }
                else
                    {               
                    iEffectSymbols->SetVolumeMuteIconVisibility(EFalse);    
                    }
            
                if ( iMovie.VideoClipSpeed( iVideoCursorPos ) != 1000 )
                    {
                    iEffectSymbols->SetSlowMotionIconVisibility(ETrue);
                    }
                else
                    {
                    iEffectSymbols->SetSlowMotionIconVisibility(EFalse);    
                    }

                SetColourToningIcons(iVideoCursorPos);

                iEffectSymbols->MakeVisible( ETrue );
                iEffectSymbols->SetRect( iEffectSymbolBox );
                }
            }
        else
            {
            iInfoDisplay->SetLayout( CVeiTextDisplay::EOnlyName );
            iVideoDisplay->ShowBlankScreen();
            
            iDummyCutBar->Dim( ETrue );
            HBufC* stringholder = StringLoader::LoadLC( R_VEI_EDIT_VIEW_NO_VIDEO, iEikonEnv );
            iInfoDisplay->SetName( *stringholder );         
            CleanupStack::PopAndDestroy( stringholder );
            }
        StartZooming();
        }
    else if ( iCursorLocation == ECursorOnEmptyVideoTrack )
        {
        iVideoDisplay->MakeVisible( ETrue );
        iVideoDisplay->SetRect( iVideoDisplayBox );
        iDummyCutBar->SetRect( iDummyCutBarBox );
        iDummyCutBar->Dim( ETrue );
        iDummyCutBar->MakeVisible( EFalse); 
        
        iInfoDisplay->SetLayout( CVeiTextDisplay::EOnlyName );
        iVideoDisplay->ShowBlankScreen();
        iInfoDisplay->MakeVisible( ETrue );

        HBufC* stringholder = StringLoader::LoadLC( R_VEI_EDIT_VIEW_NO_VIDEO, iEikonEnv );
        iInfoDisplay->SetName( *stringholder );         
        CleanupStack::PopAndDestroy( stringholder );
        }
    else if (( iCursorLocation == ECursorOnAudio ) &&
            (( iSelectionMode == EModeRecordingSetStart ) ||
             ( iSelectionMode == EModeRecording )))
        {
        HBufC* stringholder = StringLoader::LoadLC( R_VEI_EDIT_VIEW_NEW_AUDIO, iEikonEnv );

        iVideoDisplay->MakeVisible( ETrue );
        iInfoDisplay->MakeVisible( ETrue );
        iInfoDisplay->SetName( *stringholder );     
        iVideoDisplay->ShowPictureL( *iAudioIcon );
        
        iEffectSymbols->SetVolumeMuteIconVisibility( EFalse );
        iEffectSymbols->SetBlackAndWhiteIconVisibility( EFalse );
        iEffectSymbols->SetColourIconVisibility( EFalse );
        iEffectSymbols->SetSlowMotionIconVisibility( EFalse );  

        iVideoDisplay->SetRect( iVideoDisplayBox ); 
        iDummyCutBar->SetRect( iDummyCutBarBox );

        if ( iSelectionMode == EModeRecordingSetStart )
            {
            TTimeIntervalMicroSeconds duration = TTimeIntervalMicroSeconds(0);
            iInfoDisplay->SetDuration( duration );
            }
        else
            {
            iInfoDisplay->SetDuration( iRecordedAudioDuration );
            }
        CleanupStack::PopAndDestroy( stringholder );
        }
    else if ( iCursorLocation == ECursorOnAudio )
        {

        iVideoDisplay->MakeVisible( ETrue );
        iEffectSymbols->SetVolumeMuteIconVisibility( EFalse );
        iEffectSymbols->SetBlackAndWhiteIconVisibility( EFalse );
        iEffectSymbols->SetColourIconVisibility( EFalse );
        iEffectSymbols->SetSlowMotionIconVisibility( EFalse );

        iVideoDisplay->SetRect( iVideoDisplayBox ); 
        iDummyCutBar->SetRect( iDummyCutBarBox );

        if ( iMovie.AudioClipCount() > 0 )
            {

            CVedAudioClipInfo* audioclipinfo = iMovie.AudioClipInfo( CurrentIndex() );

            TTimeIntervalMicroSeconds audioClipEditedDuration = iMovie.AudioClipEditedDuration( CurrentIndex() );
            TTimeIntervalMicroSeconds audioClipDuration = audioclipinfo->Duration();
            TTimeIntervalMicroSeconds audioClipCutInTime = iMovie.AudioClipCutInTime( CurrentIndex() );
            TTimeIntervalMicroSeconds audioClipCutOutTime = iMovie.AudioClipCutOutTime( CurrentIndex() );
            
            iDummyCutBar->Dim( EFalse );

            iDummyCutBar->SetTotalDuration( audioClipDuration );
            iDummyCutBar->SetInPoint( audioClipCutInTime );
            iDummyCutBar->SetOutPoint( audioClipCutOutTime );

            TParse parser;
            parser.Set( *iAudioItemArray[CurrentIndex()]->iFilename, NULL, NULL );          
            iVideoDisplay->ShowPictureL( *iAudioIcon );

            TTime       fileModified;

            RFs& fs = iEikonEnv->FsSession();
            fs.Modified( parser.FullName(), fileModified );

            if ( VideoEditorUtils::IsLandscapeScreenOrientation() ) //Landscape     
                {
                iInfoDisplay->SetLayout( CVeiTextDisplay::EEverything );
                }
            else
                {
                iInfoDisplay->SetLayout( CVeiTextDisplay::ENameAndDuration );
                }
            TFileName audioClipName;

            if ( iAudioItemArray[CurrentIndex()]->iRecordedAudio )
                {
                HBufC* stringholder = StringLoader::LoadLC( R_VEI_EDIT_VIEW_NEW_AUDIO, iEikonEnv );
                audioClipName = *stringholder;
                CleanupStack::PopAndDestroy( stringholder );
                }
            else
                {
                audioClipName = parser.Name();
                }

            iInfoDisplay->MakeVisible( ETrue );
            iInfoDisplay->SetName( audioClipName );
            iInfoDisplay->SetDuration( audioClipEditedDuration );
            iInfoDisplay->SetTime( fileModified );
            }
        else
            {
            iVideoDisplay->MakeVisible( ETrue );
            iInfoDisplay->SetLayout( CVeiTextDisplay::EOnlyName );
            iVideoDisplay->ShowBlankScreen();
            iDummyCutBar->Dim( ETrue );
            iInfoDisplay->MakeVisible( ETrue );
            HBufC* stringholder = StringLoader::LoadLC( R_VEI_EDIT_VIEW_NO_AUDIO, iEikonEnv );
            iInfoDisplay->SetName( *stringholder );         
            CleanupStack::PopAndDestroy( stringholder );
            }
        StartZooming();
        }
    else if ( iCursorLocation == ECursorOnTransition )
        {
        iVideoDisplay->MakeVisible( ETrue );
        iTransitionDisplayLeft->MakeVisible( ETrue );
        iTransitionDisplayRight->MakeVisible( ETrue );

        TTimeIntervalMicroSeconds clipDuration;
        TTimeIntervalMicroSeconds clipCutInTime;
        TTimeIntervalMicroSeconds clipCutOutTime;

        TInt nextIndex;
        nextIndex = iVideoCursorPos;

        if ( ( nextIndex < iMovie.VideoClipCount() ) ) // video on both sides OR no video on the left hand side 
            {
            CStoryboardVideoItem* item = iVideoItemArray[ nextIndex ];
    
            iConverter->ScaleL( item->iIconBitmap, item->iIconBitmap, iTransitionDisplayRight->GetScreenSize() );

            clipDuration = iMovie.VideoClipInfo( nextIndex )->Duration();
            clipCutInTime = iMovie.VideoClipCutInTime( nextIndex );
            clipCutOutTime = iMovie.VideoClipCutOutTime( nextIndex );

            iDummyCutBar->Dim( EFalse );
            iDummyCutBar->SetTotalDuration( clipDuration );
            iDummyCutBar->SetInPoint( clipCutInTime );
            iDummyCutBar->SetOutPoint( clipCutOutTime );
            }
        else // no video on the right hand side 
            {
            iTransitionDisplayRight->ShowBlankScreen();
            iDummyCutBar->Dim( ETrue );
            }
        nextIndex--;
                
        if ( nextIndex >= 0 ) // video on both sides OR no video on the right hand side 
            {
            CStoryboardVideoItem* item = iVideoItemArray[ nextIndex ];
            if ( item->iLastFrameBitmap != NULL )
                {               
                iTransitionDisplayLeft->ShowPictureL( *item->iLastFrameBitmap, *item->iLastFrameMask);
                }
            iDummyCutBarLeft->Dim( EFalse );

            clipDuration = iMovie.VideoClipInfo( nextIndex )->Duration();
            clipCutInTime = iMovie.VideoClipCutInTime( nextIndex );
            clipCutOutTime = iMovie.VideoClipCutOutTime( nextIndex );

            iDummyCutBarLeft->SetTotalDuration( clipDuration );
            iDummyCutBarLeft->SetInPoint( clipCutInTime );
            iDummyCutBarLeft->SetOutPoint( clipCutOutTime );
            }
        else // no video on the left hand side
            {
            iTransitionDisplayLeft->ShowBlankScreen();
            iDummyCutBarLeft->Dim( ETrue );
            }

        iVideoDisplay->ShowBlankScreen();

        iVideoDisplay->SetRect( iVideoDisplayBoxOnTransition );
        iDummyCutBar->SetRect( iDummyCutBarBoxOnTransition ); 
        
        // : start using LAF data when it is available
        // iDummyCutBar->Rect() == (321, 107, 433, 151) includes the scissor icon and the progress bar
        iDummyCutBar->SetComponentRect( CVeiCutterBar::EProgressBar, TRect(355,107,433,151)); 
        iDummyCutBar->SetComponentRect( CVeiCutterBar::ESliderLeftEndIcon, TRect(355,107,360 ,151) );
        iDummyCutBar->SetComponentRect( CVeiCutterBar::ESliderMiddleIcon,  TRect(360,107,400 ,151) );
        iDummyCutBar->SetComponentRect( CVeiCutterBar::ESliderRightEndIcon,TRect(400,107,433 ,151) );                    
        iDummyCutBar->SetComponentRect( CVeiCutterBar::ESliderSelectedLeftEndIcon, TRect(355,107,360 ,151) );
        iDummyCutBar->SetComponentRect( CVeiCutterBar::ESliderSelectedMiddleIcon,  TRect(360,107,400 ,151) );
        iDummyCutBar->SetComponentRect( CVeiCutterBar::ESliderSelectedRightEndIcon,TRect(400,107,433 ,151) );            
        iDummyCutBar->SetComponentRect( CVeiCutterBar::EScissorsIcon, TRect(321,107,350 ,151) );
        iDummyCutBar->SetComponentRect( CVeiCutterBar::ECutAreaBorderIcon,TRect(321,107,350 ,151) );                        

        iDummyCutBarLeft->MakeVisible( ETrue );
        iDummyCutBarLeft->SetPosition( TPoint(iTransitionDisplayLeftBox.iTl.iX,
            iTransitionDisplayLeftBox.iBr.iY- iVideoDisplay->GetBorderWidth() ) );
        iDummyCutBarLeft->SetSize( TSize( iTransitionDisplayLeftBox.Width(), iDummyCutBarBox.Height() ) );

        // : start using LAF data when it is available
        // iDummyCutBarLeft->Rect() == (5, 107, 117, 151) includes the scissor icon and the progress bar
        iDummyCutBarLeft->SetComponentRect( CVeiCutterBar::EProgressBar, TRect(35,107,117,151)); 
        iDummyCutBarLeft->SetComponentRect( CVeiCutterBar::ESliderLeftEndIcon, TRect(35,107,55 ,151) );
        iDummyCutBarLeft->SetComponentRect( CVeiCutterBar::ESliderMiddleIcon,  TRect(55,107,100 ,151) );
        iDummyCutBarLeft->SetComponentRect( CVeiCutterBar::ESliderRightEndIcon,TRect(100,107,117 ,151) );                    
        iDummyCutBarLeft->SetComponentRect( CVeiCutterBar::ESliderSelectedLeftEndIcon, TRect(35,107,55 ,151) );
        iDummyCutBarLeft->SetComponentRect( CVeiCutterBar::ESliderSelectedMiddleIcon,  TRect(55,107,100 ,151) );
        iDummyCutBarLeft->SetComponentRect( CVeiCutterBar::ESliderSelectedRightEndIcon,TRect(100,107,117 ,151) );                    
        iDummyCutBarLeft->SetComponentRect( CVeiCutterBar::EScissorsIcon, TRect(5,107,30 ,151) );
        iDummyCutBarLeft->SetComponentRect( CVeiCutterBar::ECutAreaBorderIcon,TRect(5,107,30 ,151) );                        

        ArrowsControl();

        if ( iVideoCursorPos == 0 )
            {
            ShowStartAnimationL( iMovie.StartTransitionEffect() );
            iInfoDisplay->SetName( *iTransitionInfo->StartTransitionName( iMovie.StartTransitionEffect() ) );
            }
        else if ( iVideoCursorPos == iMovie.VideoClipCount() )
            {
            iInfoDisplay->SetName( *iTransitionInfo->EndTransitionName( iMovie.EndTransitionEffect() ) );
            ShowEndAnimationL( iMovie.EndTransitionEffect() );
            }
        else
            {           
            iInfoDisplay->SetName( *iTransitionInfo->MiddleTransitionName( iMovie.MiddleTransitionEffect( CurrentIndex() - 1 ) ) );
            ShowMiddleAnimationL( iMovie.MiddleTransitionEffect( CurrentIndex() - 1 ) );
            }
        StartZooming();
        }
    else if ( iCursorLocation == ECursorOnEmptyAudioTrack )
        {
        iVideoDisplay->MakeVisible( ETrue );
        iInfoDisplay->SetLayout( CVeiTextDisplay::EOnlyName );
        iVideoDisplay->ShowBlankScreen();
        iInfoDisplay->MakeVisible( ETrue );

        HBufC* stringholder = StringLoader::LoadLC( R_VEI_EDIT_VIEW_NO_AUDIO, iEikonEnv );
        iInfoDisplay->SetName( *stringholder );         
        CleanupStack::PopAndDestroy( stringholder );

        iVideoDisplay->SetRect( iVideoDisplayBox );
        iDummyCutBar->SetRect( iDummyCutBarBox );
        iDummyCutBar->Dim( ETrue );
        }
    if ( iView.EditorState() == CVeiEditVideoView::EQuickPreview)
        {
        TFileName newname;
        TVeiSettings movieSaveSettings;
        STATIC_CAST( CVeiAppUi*, iEikonEnv->AppUi() )->ReadSettingsL( movieSaveSettings );  
        newname.Append( movieSaveSettings.DefaultVideoName() );
        iInfoDisplay->SetName( newname );

        TTimeIntervalMicroSeconds tempFileDuration;
        tempFileDuration = iVideoDisplay->TotalLengthL();
        iInfoDisplay->SetDuration( tempFileDuration );
        }
    /*if ( iView.EditorState() == CVeiEditVideoView::EMixAudio)
        {       
        
        //iHorizontalSliderSize = TSize(iHorizontalSlider->MinimumSize().iWidth, 50);
        //  iHorizontalSliderPoint = TPoint( videoScreenX - 25, + videoScreenY + videoScreenSize.iHeight + 70); 
        iHorizontalSlider->SetExtent( iHorizontalSliderPoint, iHorizontalSliderSize );              
        iHorizontalSlider->MakeVisible(ETrue);
        iHorizontalSlider->DrawDeferred();
        iVideoDisplay->ShowPictureL(*iAudioMixingIcon);
        }   
        */
    }

TBool CVeiEditVideoContainer::CurrentClipIsFile()
    {
    if ( (iCursorLocation == ECursorOnClip) && (iMovie.VideoClipCount() > 0 ) )
        {
        return iVideoItemArray[ CurrentIndex() ]->iIsFile;
        }
    else
        return EFalse;
    }


void CVeiEditVideoContainer::ShowMiddleAnimationL( TVedMiddleTransitionEffect aMiddleEffect )
    {
    switch( aMiddleEffect )
        {
        case EVedMiddleTransitionEffectNone:
            iVideoDisplay->StopAnimation();
            break;
        case EVedMiddleTransitionEffectDipToBlack:
            iVideoDisplay->ShowAnimationL( R_VEI_DIP_TO_BLACK_ANIMATION );
            break;
        case EVedMiddleTransitionEffectDipToWhite:
            iVideoDisplay->ShowAnimationL( R_VEI_DIP_TO_WHITE_ANIMATION );
            break;
        case EVedMiddleTransitionEffectCrossfade:
            iVideoDisplay->ShowAnimationL( R_VEI_CROSSFADE_ANIMATION );
            break;
        case EVedMiddleTransitionEffectWipeLeftToRight:
            iVideoDisplay->ShowAnimationL( R_VEI_WIPE_LEFT_ANIMATION );
            break;
        case EVedMiddleTransitionEffectWipeRightToLeft:
            iVideoDisplay->ShowAnimationL( R_VEI_WIPE_RIGHT_ANIMATION );
            break;
        case EVedMiddleTransitionEffectWipeTopToBottom:
            iVideoDisplay->ShowAnimationL( R_VEI_WIPE_TOP_TO_BOTTOM_ANIMATION );
            break;
        case EVedMiddleTransitionEffectWipeBottomToTop:
            iVideoDisplay->ShowAnimationL( R_VEI_WIPE_BOTTOM_TO_TOP_ANIMATION );
            break;
        default:
            break;
        }
    }


void CVeiEditVideoContainer::ShowStartAnimationL( TVedStartTransitionEffect aStartEffect )
    {
    switch( aStartEffect )
        {
        case EVedStartTransitionEffectNone:
            iVideoDisplay->StopAnimation();
            break;
        case EVedStartTransitionEffectFadeFromBlack:
            iVideoDisplay->ShowAnimationL( R_VEI_FADE_FROM_BLACK_ANIMATION );
            break;
        case EVedStartTransitionEffectFadeFromWhite:
            iVideoDisplay->ShowAnimationL( R_VEI_FADE_FROM_WHITE_ANIMATION );
            break;
        default:
            break;
        }
    }

void CVeiEditVideoContainer::ShowEndAnimationL( TVedEndTransitionEffect aEndEffect )
    {
    switch( aEndEffect )
        {
        case EVedEndTransitionEffectNone:
            iVideoDisplay->StopAnimation();
            break;
        case EVedEndTransitionEffectFadeToBlack:
            iVideoDisplay->ShowAnimationL( R_VEI_FADE_TO_BLACK_ANIMATION );
            break;
        case EVedEndTransitionEffectFadeToWhite:
            iVideoDisplay->ShowAnimationL( R_VEI_FADE_TO_WHITE_ANIMATION );
            break;
        default:
            break;
        }
    }


void CVeiEditVideoContainer::NotifyVideoDisplayEvent( const TPlayerEvent aEvent, const TInt& aInfo  )
    {
    LOGFMT4(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoDisplayEvent() In, \
                aEvent:%d, iFullScreenSelected:%d, iView.EditorState():%d, iPreviewState:%d", \
                aEvent, iFullScreenSelected, iView.EditorState(), iPreviewState);

    if (EStateTerminating == iPreviewState)
        {
        LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoDisplayEvent(): app is closing...");
        return;
        }

    switch (aEvent)
        {
        case MVeiVideoDisplayObserver::ELoadingStarted:
            {
            SetPreviewState(EStateOpening);     
            LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::ELoadingStarted");            
            break;
            }
        case MVeiVideoDisplayObserver::EOpenComplete:
            {
            LOGFMT(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::EOpenComplete 1:%Ld", iVideoDisplay->PositionL().Int64());
                
            iCursorPreviousLocation = CursorLocation();
            
            if ( !iFullScreenSelected )
                {
                TRAP_IGNORE( (/*iTempVideoInfo =*/ CVedVideoClipInfo::NewL( *iTempFileName,*this) ) );
                
                //for draw function
                iCursorLocation = ECursorOnClip;
                }

            if ( CVeiEditVideoView::EPreview == iView.EditorState() )//Large preview
                {
                TRect wholeScreenRect = iView.ClientOrApplicationRect( iFullScreenSelected );
                SetRect( iView.ClientOrApplicationRect( iFullScreenSelected ) );
                iVideoDisplay->SetRect( wholeScreenRect );  

                if ( !VideoEditorUtils::IsLandscapeScreenOrientation() ) //Portrait
                    {
                    iVideoDisplay->SetRotationL( EVideoRotationClockwise90 );                           
                    }
                
                if ( iView.IsForeground() )
                    {
                    iVideoDisplay->ShowBlackScreen();
                    DrawDeferred();
                    if ( iTempVideoInfo && !iFrameReady)
                        {                           
                        iTempVideoInfo->CancelFrame();
                        }
                    iVideoDisplay->Play();      
                    }
                else
                    {
                    iView.SetEditorState( CVeiEditVideoView::EEdit );
                    SetBlackScreen( EFalse );
                    iView.SetFullScreenSelected( EFalse );
                    iVideoDisplay->Stop( ETrue ); 
                    }           
                    
                }       
             else //Small preview
                {
                iVideoDisplay->SetRect( iVideoDisplayBox );
                iInfoDisplay->SetRect( iInfoDisplayBox );
                iVideoDisplay->SetRotationL( EVideoRotationNone );        

                if ( !VideoEditorUtils::IsLandscapeScreenOrientation() ) //Portrait
                    {
                    iInfoDisplay->SetLayout( CVeiTextDisplay::ENameAndDuration );
                    iInfoDisplay->MakeVisible( ETrue );
                    }
                else //Landscape
                    {
                    iInfoDisplay->SetLayout( CVeiTextDisplay::EEverything );
                    iInfoDisplay->MakeVisible( ETrue );
                    }
                
                if ( iView.IsForeground() )
                    {
                    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::EOpenComplete 2");
                    iVideoDisplay->ShowBlackScreen();
                    DrawDeferred();
                    if ( iTempVideoInfo && !iFrameReady)
                        {                           
                        iTempVideoInfo->CancelFrame();
                        }
                    iVideoDisplay->Play();                          
                    }
                else
                    {
                    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::EOpenComplete 3");                    
                    PauseVideoL();
                    }           
                }
            break;
            }                                                                                           
        case MVeiVideoDisplayObserver::EBufferingStarted:
            {
            LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::EBufferingStarted");          
            SetPreviewState( EStateBuffering );
            if ( iPeriodic )
                {
                iPeriodic->Cancel();
                }
            break;
            }
        case MVeiVideoDisplayObserver::ELoadingComplete:
            {
            LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::ELoadingComplete 1");                     
            
            if (EStatePaused == iPreviewState)
                {
                iVideoDisplay->PauseL();    
                }
            else
                {
                SetPreviewState( EStatePlaying );           
            
                if (iFullScreenSelected)
                    {               
                    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::ELoadingComplete 2");
                    iView.SetEditorState( CVeiEditVideoView::EPreview);
                    }
                else 
                    {               
                    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::ELoadingComplete 3");
                    iView.SetEditorState( CVeiEditVideoView::EQuickPreview);
                    DrawDeferred();
                    iVideoDisplay->ShowBlackScreen();
                    iScreenLight->Start();
                    const TUint delay = 100000;             
                    iPeriodic->Start( delay, delay, TCallBack( CVeiEditVideoContainer::UpdatePosition, this ) );
                    TRAP_IGNORE(iView.StartNaviPaneUpdateL()); 
                    }
                }
            break;
            }
        case MVeiVideoDisplayObserver::EPlayComplete:
            {
            LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::EPlayComplete 1");
            iScreenLight->Stop();
            if ( iPeriodic )
                {
                iPeriodic->Cancel();
                }
            
            if ( !iFullScreenSelected )
                {
                SetPreviewState(EStateStopped);
                
                iLastPosition = iVideoDisplay->TotalLengthL();
                iSeekPos = TTimeIntervalMicroSeconds( 0 );
                
                SetFinishedStatus( ETrue );
                LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::EPlayComplete 2");                
                GetThumbAtL(0);  
                LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::EPlayComplete 3"); 
                
                iView.SetEditorState( CVeiEditVideoView::EQuickPreview );
                iView.StopNaviPaneUpdateL();
                }
            else
                {
                if (EModeMixingAudio != iSelectionMode)
                    {                                               
                    iView.SetEditorState( CVeiEditVideoView::EEdit );                   
                    }
                else
                    {
                    iView.SetEditorState(CVeiEditVideoView::EMixAudio);                 
                    }
                                
                SetBlackScreen( EFalse );
                iView.SetFullScreenSelected( EFalse );
                iVideoDisplay->Stop( ETrue ); 
                DrawDeferred();
                }
            
            if (KErrNoMemory == aInfo || KErrSessionClosed == aInfo)
                {
                LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::EPlayComplete 4");
                iView.ShowGlobalErrorNote( aInfo );
                StopVideo(ETrue);               
                }  
            
            LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::EPlayComplete 5");
            break;
            }
        case MVeiVideoDisplayObserver::EStop:
            {
            LOGFMT3(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::EStop 1, \
            iFullScreenSelected:%d, iSelectionMode:%d, iCloseStream:%d", \
            iFullScreenSelected, iSelectionMode, iCloseStream);  
            
            if ( iPeriodic )
                {                
                iPeriodic->Cancel();
                }
            iView.StopNaviPaneUpdateL();    
                
            if (EStateGettingFrame == iPreviewState)
                {
                break;  
                }
            // position must be set here to 0 because state EStateGettingFrame cannot be resoluted in player                    
            // and position must not be set to 0 in that state
            iVideoDisplay->SetPositionL(TTimeIntervalMicroSeconds( 0 ));
                                               
            if ( iFullScreenSelected || iCloseStream )
                {
                LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::EStop 4");
                SetPreviewState(EStateClosed);                
                iVideoDisplay->SetBlackScreen( EFalse );
                iDummyCutBar->MakeVisible( ETrue );
                
                if (EModeMixingAudio != iSelectionMode)
                    {                                   
                    iView.SetEditorState( CVeiEditVideoView::EEdit );
                    SetSelectionMode( EModeNavigation );
                    }
                else
                    {
                    iView.SetEditorState(CVeiEditVideoView::EMixAudio);
                    break;
                    }
                SetCursorLocation( iCursorPreviousLocation );
                LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::EStop 5");
                DrawDeferred();
                iCloseStream = EFalse;
                break;
                }
                
            if (EModeMixingAudio != iSelectionMode)
                {   
                LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::EStop 2");
                GetThumbAtL(0); 
                LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::EStop 3");
                }    

            SetPreviewState(EStateStopped);            
            iLastPosition = iVideoDisplay->TotalLengthL();
            iSeekPos = TTimeIntervalMicroSeconds( 0 );
            iView.DoUpdateEditNaviLabelL();
            SetFinishedStatus( ETrue );
            iView.SetEditorState( CVeiEditVideoView::EQuickPreview );
            // redraw needed at least to erase pause icon
            DrawDeferred();            
            break;
            }
        case MVeiVideoDisplayObserver::EVolumeLevelChanged:
            {
            LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::EVolumeLevelChanged 1");
            TInt playerVolume = iVideoDisplay->Volume();
            iView.ShowVolumeLabelL( playerVolume );
            break;
            }
        case MVeiVideoDisplayObserver::EError:
            {
            LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoDisplayEvent() MVeiVideoDisplayObserver::EError 1");
            iView.ShowGlobalErrorNote( aInfo );         
            if (KErrMMAudioDevice  == aInfo)
                {
                PauseVideoL();  
                }
            else
                {
                StopVideo(ETrue);
                }               
            break;
            }                           
        default:
            {
            LOGFMT(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoDisplayEvent() default case, aEvent:%d", aEvent);
            };
        }
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyVideoDisplayEvent() Out");
    }


void CVeiEditVideoContainer::StartFrameTakerL( TInt aIndex )
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::StartFrameTakerL: In");

// First frame is shown in main display so it is bigger.. Last frame is always
// on transition display and one frame for the video timeline.
    TSize firstThumbResolution = iVideoDisplay->GetScreenSize();
    TSize lastThumbResolution = iTransitionDisplayLeft->GetScreenSize();
    TSize timelineThumbResolution = TSize( 34, /*iVideoBarBox.Height()-2*/28 );
    
    TTimeIntervalMicroSeconds cutInTime = iMovie.VideoClipCutInTime( aIndex );
    TTimeIntervalMicroSeconds cutOutTime = iMovie.VideoClipCutOutTime( aIndex );


    TInt frameCount = iMovie.VideoClipInfo(aIndex)->VideoFrameCount();

    TInt firstThumbNailIndex =  iMovie.VideoClipInfo(aIndex)->GetVideoFrameIndexL( cutInTime ); 
    TInt lastThumbNailIndex =  iMovie.VideoClipInfo(aIndex)->GetVideoFrameIndexL( cutOutTime );    
    if ( lastThumbNailIndex >= frameCount )
        {
        lastThumbNailIndex = frameCount-1;
        }
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::StartFrameTakerL: 2");

    iFrameTaker->GetFramesL( *iMovie.VideoClipInfo(aIndex), 
            firstThumbNailIndex, &firstThumbResolution,
            lastThumbNailIndex, &lastThumbResolution, 
            firstThumbNailIndex, &timelineThumbResolution,
            EPriorityLow );

    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::StartFrameTakerL: Out");          
    }

void CVeiEditVideoContainer::NotifyFramesCompleted( CFbsBitmap* aFirstFrame, 
                                       CFbsBitmap* aLastFrame,  CFbsBitmap* aTimelineFrame,  TInt aError )
    {
    LOGFMT(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyFramesCompleted: In, aError:%d", aError);
    CStoryboardVideoItem* item;
    if( aError==KErrNone )
        {
        if ( iMovie.VideoClipColorEffect( iVideoCursorPos ) == EVedColorEffectBlackAndWhite ) 
            {
            ConvertBW( *aFirstFrame );
            ConvertBW( *aLastFrame );
            ConvertBW( *aTimelineFrame );
            }
        if ( iMovie.VideoClipColorEffect( iVideoCursorPos ) == EVedColorEffectToning ) 
            {
            //TRgb toning = iMovie.VideoClipColorTone(CurrentIndex());
            ConvertToning(*aFirstFrame);
            ConvertToning(*aLastFrame);
            ConvertToning(*aTimelineFrame);
            }   

        item = iVideoItemArray[ iCurrentlyProcessedIndex ];
        TRAP_IGNORE( 
            item->InsertLastFrameL( *aLastFrame, *aLastFrame );
            item->InsertFirstFrameL( *aFirstFrame, *aFirstFrame );
            item->InsertTimelineFrameL( *aTimelineFrame, *aTimelineFrame );         
            iVideoDisplay->ShowPictureL( *item->iIconBitmap, *item->iIconMask);
            );

        // UpdateThumbnail launches progressnote. 
        if (iProgressDialog )
            {
            iProgressDialog->GetProgressInfoL()->SetAndDraw( 100 );
            TRAP_IGNORE( iProgressDialog->ProcessFinishedL() );
            //iProgressDialog = NULL;           
            }
        iView.HandleCommandL( EAknSoftkeyOk );
        }
        
    // UpdateThumbnail launches progressnote. 
    if (iProgressDialog )
        {
        LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyFramesCompleted: 2");
        iProgressDialog->GetProgressInfoL()->SetAndDraw( 100 );
        TRAP_IGNORE( iProgressDialog->ProcessFinishedL() );
        //iProgressDialog = NULL;
        }   
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyFramesCompleted: 3");
    iCurrentlyProcessedIndex = -1;
    iView.SetWaitMode( CVeiEditVideoView::ENotWaiting );
    SetCursorLocation( CursorLocation() );
    
    iView.CancelWaitDialog(aError);
    iView.AddNext();
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyFramesCompleted: Out");         
    }

void CVeiEditVideoContainer::NotifyCompletion( TInt DEBUGLOG_ARG(aErr) )
    {
    LOGFMT(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyCompletion: In, err:%d", aErr);

    if (EStateTerminating == iPreviewState)
        {
        LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyCompletion(): app is closing...");
        return;
        }

    if ( iTakeSnapshot )
        {
        //  to eliminate previous (wrong) output file from being deleted in CancelSnapshotSave()    
        delete iSaveToFileName;
        iSaveToFileName = NULL;
        
        if ( iProgressDialog )
            {
            iProgressDialog->GetProgressInfoL()->SetAndDraw( 100 );
            TRAP_IGNORE( iProgressDialog->ProcessFinishedL() );
            //iProgressDialog = NULL;
            }
        return;
        }
    
    TRAP_IGNORE(iTransitionDisplayRight->ShowPictureL( *iConverter->GetBitmap()));
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::NotifyCompletion: Out");          
    }

void CVeiEditVideoContainer::SetSlowMotionStartValueL(TInt aSlowMotionStartValue)
    {       
    iSlowMotionValue = aSlowMotionStartValue; 
    iArrowsDisplay->SetSlowMotionPreset( iSlowMotionValue / 10 );
    
    TInt frameInterval = (1000-iSlowMotionValue)/2;

    iVideoDisplay->ShowAnimationL( R_VEI_SLOW_MOTION_ANIMATION, frameInterval );
    }

void CVeiEditVideoContainer::SetRecordedAudioDuration( const TTimeIntervalMicroSeconds& aDuration )
    {
    iRecordedAudioDuration = aDuration;
        
    if ( (iCursorLocation==ECursorOnTransition) && (CurrentIndex() > 0) )
        {
        iVideoCursorPos--;
        }
    iInfoDisplay->SetLayout( CVeiTextDisplay::ERecording ); // Name and duration RED
    iInfoDisplay->SetDuration( iRecordedAudioDuration );
    }


TBool CVeiEditVideoContainer::IsAudioClipCutted()
    {       
        TTimeIntervalMicroSeconds audioClipDuration = iMovie.AudioClipInfo( CurrentIndex())->Duration();
        TTimeIntervalMicroSeconds audioClipCutInTime = iMovie.AudioClipCutInTime( CurrentIndex() );
        TTimeIntervalMicroSeconds audioClipCutOutTime = iMovie.AudioClipCutOutTime( CurrentIndex() );
    
        TTimeIntervalMicroSeconds appendTime(500000);
        TTimeIntervalMicroSeconds cuttedAudioDuration = ( audioClipCutOutTime.Int64() - audioClipCutInTime.Int64() ) + appendTime.Int64();
        iInfoDisplay->SetDuration( cuttedAudioDuration );

        iDummyCutBar->SetTotalDuration( audioClipDuration );
        iDummyCutBar->SetInPoint( audioClipCutInTime );
        iDummyCutBar->SetOutPoint( audioClipCutOutTime );
        iDummyCutBar->Dim( EFalse );

        if ( ( audioClipCutOutTime.Int64() - audioClipCutInTime.Int64() ) == audioClipDuration.Int64() )
            {
            return EFalse;
            }
        else
            {
            return ETrue; 
            }           
    }

/* Checks if aFilename is belongs to any album. Album name is returned, or KNullDesC
if aFilename does not belong to album. */
void CVeiEditVideoContainer::GetAlbumL( const TDesC& aFilename, TDes& aAlbumName ) const
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::GetAlbumL: in");

    CMGAlbumManager* albumManager = MGAlbumManagerFactory::NewAlbumManagerL();
    TInt albumCount = albumManager->AlbumCount();

    // Get album Id 
    for( TInt i=0;i<albumCount;i++ )
        {
        CMGAlbumInfo* albumInfo = albumManager->AlbumInfoLC( i );
        TInt albumId = albumInfo->Id();
        TInt itemCount = albumInfo->ItemCount();

        TFileName albumName = albumInfo->Name();
        CleanupStack::PopAndDestroy(albumInfo);

        if ( itemCount > 0 )
            {
            TInt itemPos;
            CDesCArrayFlat* filenameArray = new (ELeave) CDesCArrayFlat( itemCount );
            CleanupStack::PushL (filenameArray);

            albumManager->GetAlbumFileArrayL( albumId, *filenameArray );
        
            TInt isFound = filenameArray->Find( aFilename, itemPos );
            CleanupStack::PopAndDestroy (filenameArray);

            if ( isFound == KErrNone ) /* filename was found on filenamearray */
                {
                delete albumManager;
                aAlbumName.Append(albumName);
                LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::GetAlbumL: out1");
                return;
                }
            }
        }
    delete albumManager;

    aAlbumName = KNullDesC;
    
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::GetAlbumL: out2");
    }


void CVeiEditVideoContainer::SetBlackScreen( TBool aBlack )
    {
    LOGFMT(KVideoEditorLogFile, "CVeiEditVideoContainer::SetBlackScreen: In, aBlack:%d", aBlack);

    iBlackScreen = aBlack;

    // Black backbround for the preview
    if ( iBlackScreen )
        {
        iVideoDisplay->MakeVisible( EFalse );
        /* Video Display components for transitioin state*/
        iTransitionDisplayRight->MakeVisible( EFalse );
        iTransitionDisplayLeft->MakeVisible( EFalse );

        iDummyCutBar->MakeVisible( EFalse );
        iDummyCutBarLeft->MakeVisible( EFalse );

        /* IconBox */   
        iEffectSymbols->MakeVisible( EFalse );
        iInfoDisplay->MakeVisible( EFalse );
        iArrowsDisplay->MakeVisible( EFalse );

        iHorizontalSlider->MakeVisible( EFalse );
        iVerticalSlider->MakeVisible( EFalse );
    
        if( iCursorLocation == ECursorOnTransition )
            {
            iVideoDisplay->StopAnimation();
            }
        }
    else
        {
        SetCursorLocation( CursorLocation() );
        }
    DrawDeferred();
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::SetBlackScreen: Out");
    }

TInt CVeiEditVideoContainer::SnapshotSize()
    {
    TInt sizeEstimate = 0;

    if( iTempVideoInfo )
        {       
        TTimeIntervalMicroSeconds playBackPos = PlaybackPositionL();
        TInt frame = iTempVideoInfo->GetVideoFrameIndexL( playBackPos );
        sizeEstimate = iTempVideoInfo->VideoFrameSizeL( frame );
        }

    return sizeEstimate;
    }

TInt CVeiEditVideoContainer::AudioMixingRatio() const
    {
    return iHorizontalSlider->SliderPosition(); 
    }


TInt CVeiEditVideoContainer::Volume() const
    {
    return -(iVerticalSlider->SliderPosition());
    }

//=============================================================================
void CVeiEditVideoContainer::SetColourToningIcons(TInt /*aIndex*/)
    {
    if ( ( iMovie.VideoClipColorEffect( iVideoCursorPos ) ) == EVedColorEffectBlackAndWhite )
        {
        iEffectSymbols->SetBlackAndWhiteIconVisibility(ETrue);
        }
    else
        {
        iEffectSymbols->SetBlackAndWhiteIconVisibility(EFalse);
        }
    if ( ( iMovie.VideoClipColorEffect( iVideoCursorPos ) ) == EVedColorEffectToning )
        {
        iEffectSymbols->SetColourIconVisibility(ETrue);
        }
    else
        {
        iEffectSymbols->SetColourIconVisibility(EFalse);
        }
    }
//=======================================================================================================
void CVeiEditVideoContainer::StartProgressDialogL(const TInt aDialogResId, const TInt aTextResId)
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::StartProgressDialogL: In");   
    
    if (iProgressDialog)
        {       
        LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::StartProgressDialogL: 1");
        delete iProgressDialog;
        iProgressDialog = NULL;
        }
    
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::StartProgressDialogL: 2");    
    
    iProgressDialog = new (ELeave) CAknProgressDialog( 
        reinterpret_cast<CEikDialog**>(&iProgressDialog), ETrue );
    iProgressDialog->PrepareLC(aDialogResId);   
    iProgressDialog->SetCallback( this );   

                                                        
    HBufC* stringholder = StringLoader::LoadLC( aTextResId, iEikonEnv );
    iProgressDialog->SetTextL( *stringholder );
    CleanupStack::PopAndDestroy(stringholder);

    iProgressDialog->GetProgressInfoL()->SetFinalValue( 100 );
    iProgressDialog->RunLD();
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::StartProgressDialogL: Out");  
    }

//=============================================================================
void CVeiEditVideoContainer::SetPreviewState(const TPreviewState aNewState)
    {   
    LOGFMT3(KVideoEditorLogFile, "CVeiEditVideoContainer::SetPreviewState In, aNewState:%d, iPreviewState:%d, iPreviousPreviewState:%d", aNewState, iPreviewState, iPreviousPreviewState);

    if (EStateGettingFrame == aNewState)
        {
        iPreviousPreviewState = iPreviewState;                                  
        }               
    iPreviewState = aNewState;
    iSeeking = EFalse;
    
    /*if (EStateTerminating != iPreviewState)
        {
        iView.SetEditorState( iView.EditorState() );    
        }   
        */

    // Make sure that the pause indicator is drawn immediately
    if (EStatePaused == iPreviewState)
        {
        DrawNow();
        }

    // While playing, grab the volume keys for adjusting playback volume.
    // In other states let them pass e.g. to the music player.
    if(EStatePlaying == aNewState)
        {
        if (!iRemConTarget)
            {
            // We can ignore the possible error - the if the remote connection
            // fails, we just won't receive volume keys, which is a minor problem.
            TRAPD(err, iRemConTarget = CVeiRemConTarget::NewL( *this ) );
            if (KErrNone != err)
                {
                LOGFMT(KVideoEditorLogFile, "CVeiEditVideoContainer::SetPreviewState: CVeiRemConTarget::NewL failed: %d", err);
                }
            }
        }
    else
        {
        delete iRemConTarget;
        iRemConTarget = NULL;
        }
    
    // : implement #ifdef here to facilitate easy remove of unnecessary stopping    
    //#ifdef STOP_PLAYER_DURING_GETFRAME
    if (EStateGettingFrame == aNewState)
        {   
        /* :
         check out on every phone before releasing whether videodisplay should be stopped before starting
         asynchronous GetFrameL()
         see how EStateGettingFrame is handled in SetPreviewState 
         Stopping frees memory and it is needed in memory sensible devices 
        */
        //iVideoDisplay->Stop(ETrue);   
        // SetEditorState is effective because iPreviewState is changed
        iView.SetEditorState( iView.EditorState() );        
        }
    //#endif    
    }

//=============================================================================
void CVeiEditVideoContainer::HandleVolumeUpL()
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::HandleVolumeUpL: in");

    iVideoDisplay->AdjustVolumeL( 1 );

    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::HandleVolumeUpL: out");
    }

//=============================================================================
void CVeiEditVideoContainer::HandleVolumeDownL()
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::HandleVolumeDownL: in");

    iVideoDisplay->AdjustVolumeL( -1 );

    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::HandleVolumeDownL: out");
    }

//=============================================================================
void CVeiEditVideoContainer::PrepareForTerminationL()
    {
    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::PrepareForTerminationL: in");

    SetPreviewState( EStateTerminating );
    iVideoDisplay->Stop(ETrue);
    iScreenLight->Stop();

    LOG(KVideoEditorLogFile, "CVeiEditVideoContainer::PrepareForTerminationL: out");
    }

//=============================================================================
TInt CVeiEditVideoContainer::AsyncTakeSnapshot(TAny* aThis)
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoContainer::AsyncTakeSnapshot");
    
    // In the asynchronous version, trap the rest of the functions 
    // to make sure that the caller's TRequestStatus is always 
    // completed, also in case of failures.
    CVeiEditVideoContainer* container = static_cast<CVeiEditVideoContainer*>(aThis);
    TInt err = KErrNone;
    TRAP(err, container->TakeSnapshotL());
    LOGFMT( KVideoEditorLogFile, "CVeiEditVideoContainer::AsyncTakeSnapshot 1, err:%d", err);   
    User::LeaveIfError(err);        
    return KErrNone;
    }

//=============================================================================    
void CVeiEditVideoContainer::CalculateVideoClipRects()
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoContainer::CalculateVideoClipRects: in" );
    iVideoItemRectArray.Reset();
    
    TRect selectedRect; // rect of the highlighted clip
    TRect videoClipRect; // rect of the video clip in timeline
    TInt barWidth = iVideoBarBox.Width();
    TInt64 barDuration = iMovie.Duration().Int64();
    
    TInt64 audioDuration(0);
    if ( (iMovie.AudioClipCount() != 0) && ((iSelectionMode == EModeRecording ) ||
        (iSelectionMode == EModeRecordingPaused)))
        {
        audioDuration = (iMovie.AudioClipEndTime( iMovie.AudioClipCount() - 1 )).Int64();
        }

    audioDuration+= iRecordedAudioDuration.Int64();

    if ( audioDuration > barDuration )
        {
        barDuration = audioDuration;
        }
    if ( iRecordedAudioStartTime > barDuration )
        {
        barDuration = iRecordedAudioStartTime.Int64();
        }
    if (barDuration < 30000000)
        {
        barDuration = 30000000;
        }
    else if (barDuration < 45000000)
        {
        barDuration = 45000000;
        }
    else{
        barDuration = ((barDuration / 30000000) + 1) * 30000000;
        }

    videoClipRect.iTl.iY = iVideoBarBox.iTl.iY;
    videoClipRect.iBr.iY = iVideoBarBox.iBr.iY;
      
    // calculate the rect of each of the video clip  
    for (TInt i = 0; i < iMovie.VideoClipCount(); i++ )
        {
        videoClipRect.iTl.iX = iVideoBarBox.iTl.iX
            + static_cast<TInt32>( (iMovie.VideoClipStartTime( i ).Int64() * barWidth ) / barDuration );
        videoClipRect.iBr.iX = iVideoBarBox.iTl.iX
            + static_cast<TInt32>( (iMovie.VideoClipEndTime( i ).Int64() * barWidth ) / barDuration )+ 1;
                
        if ((iCursorLocation == ECursorOnClip)
            && (i == CurrentIndex()) && ( iView.EditorState() == CVeiEditVideoView::EEdit ))
            // the current video clip selected
            {
            LOG( KVideoEditorLogFile, "CVeiEditVideoContainer::CalculateVideoClipRects: selected clip" );
            selectedRect = videoClipRect;
            selectedRect.Grow( iZoomFactorX,iZoomFactorY );            
            iVideoItemRectArray.Insert( selectedRect, i );
            }
        else
            {
            iVideoItemRectArray.Insert( videoClipRect, i );
            }

        LOGFMT6( KVideoEditorLogFile, "CVeiEditVideoContainer::CalculateVideoClipRects, iVideoItemRectArray.[%d]->Rect(): \
                                                    (%d,%d,%d,%d), barWidth = %d ", \
                                                    i,
                                                    iVideoItemRectArray[i].iTl.iX, 
                                                    iVideoItemRectArray[i].iTl.iY, 
                                                    iVideoItemRectArray[i].iBr.iX,
                                                    iVideoItemRectArray[i].iBr.iY,
                                                    barWidth );
        }

    // Calculate the empty rect of the timeline
    if ( iMovie.VideoClipCount() > 0 )
        {
        iEmptyVideoTimeLineRect.iTl = TPoint ( iVideoItemRectArray[ iMovie.VideoClipCount()-1 ].iBr.iX, iVideoBarBox.iTl.iY );
        iEmptyVideoTimeLineRect.iBr = iVideoBarBox.iBr; 
        }
    else
        {
        iEmptyVideoTimeLineRect = iVideoBarBox; 
        }

    LOGFMT4( KVideoEditorLogFile, "CVeiEditVideoContainer::CalculateVideoClipRects, iEmptyVideoTimeLineRect: \
                                                    (%d,%d,%d,%d) ", \
                                                    iEmptyVideoTimeLineRect.iTl.iX, 
                                                    iEmptyVideoTimeLineRect.iTl.iY, 
                                                    iEmptyVideoTimeLineRect.iBr.iX,
                                                    iEmptyVideoTimeLineRect.iBr.iY );                                                    
                    
    LOG( KVideoEditorLogFile, "CVeiEditVideoContainer::CalculateVideoClipRects: out" );
    }
// End of File  
