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
#include <aknviewappui.h>
#include <avkon.hrh>
#include <manualvideoeditor.rsg>
#include <VedAudioClipInfo.h>

#include <akntitle.h> 
#include <barsread.h>
#include <stringloader.h> 
#include <aknnotewrappers.h>
#include <aknquerydialog.h>
#include <aknnavide.h>
#include <eikbtgpc.h>
#include <eikmenub.h>
#include <eikmenup.h>
#include <eikprogi.h>
#include <apparc.h>
#include <aknselectionlist.h>

//User includes
#include "VeiAppUi.h"
#include "VeiCutAudioView.h"
#include "VeiCutAudioContainer.h" 
#include "ManualVideoEditor.hrh"
#include "VeiTimeLabelNavi.h"
#include "VideoEditorCommon.h"
#include "VeiVideoEditorSettings.h"
#include "VeiErrorUi.h"

CVeiCutAudioView* CVeiCutAudioView::NewL()
    {
    CVeiCutAudioView* self = CVeiCutAudioView::NewLC();
    CleanupStack::Pop( self );

    return self;
    }


CVeiCutAudioView* CVeiCutAudioView::NewLC()
    {
    CVeiCutAudioView* self = new (ELeave) CVeiCutAudioView();
    CleanupStack::PushL( self );
    self->ConstructL();
    return self;
    }

CVeiCutAudioView::CVeiCutAudioView()
    {
    }

void CVeiCutAudioView::ConstructL()
    {
    BaseConstructL( R_VEI_CUT_AUDIO_VIEW );

    CEikStatusPane* sp = StatusPane();

    iNaviPane = (CAknNavigationControlContainer*) sp->ControlL(
            TUid::Uid(EEikStatusPaneUidNavi));

    iTimeNavi = CreateTimeLabelNaviL();
    iTimeNavi->SetMopParent( this );

    iVolumeNavi = iNaviPane->CreateVolumeIndicatorL(
            R_AVKON_NAVI_PANE_VOLUME_INDICATOR );

    iErrorUI = CVeiErrorUI::NewL( *iCoeEnv );   
    
    iTimeUpdater = CPeriodic::NewL( CActive::EPriorityLow );
    iVolumeHider = CPeriodic::NewL( CActive::EPriorityLow );
    }

// ---------------------------------------------------------
// CVeiCutAudioView::~CVeiCutAudioView()
// ?implementation_description
// ---------------------------------------------------------
//
CVeiCutAudioView::~CVeiCutAudioView()
    {
    if ( iContainer )
        {
        AppUi()->RemoveFromViewStack( *this, iContainer );
        delete iContainer;
        iContainer = 0;
        }
    if ( iTimeUpdater )
        {
        iTimeUpdater->Cancel();
        delete iTimeUpdater;
        }
    if ( iVolumeHider )
        {
        iVolumeHider->Cancel();
        delete iVolumeHider;
        }

    if ( iErrorUI )
        {
        delete iErrorUI;
        }

    delete iVolumeNavi;

    delete iTimeNavi;
    
    delete iCallBack;
    }


void CVeiCutAudioView::DynInitMenuPaneL( TInt aResourceId,CEikMenuPane* aMenuPane )
    {
    TInt state = iContainer->State();

    if (aResourceId == R_VEI_CUT_VIDEO_VIEW_MENU_CLEAR_MARKS)
        {
        // delet in, out, in & out as necessary.

        if (iMovie->AudioClipCutInTime(iIndex) <= TTimeIntervalMicroSeconds(0)) 
            {
            aMenuPane->SetItemDimmed(EVeiCmdCutVideoViewClearMarksIn, ETrue);
            }
        if (iMovie->AudioClipCutOutTime(iIndex) >= iMovie->AudioClipInfo(iIndex)->Duration() ) 
            {
            aMenuPane->SetItemDimmed(EVeiCmdCutVideoViewClearMarksOut, ETrue);
            }
        if (!((iMovie->AudioClipCutOutTime(iIndex) < iMovie->AudioClipInfo(iIndex)->Duration())
            && (iMovie->AudioClipCutInTime(iIndex) > TTimeIntervalMicroSeconds(0))))
            {
            aMenuPane->SetItemDimmed(EVeiCmdCutVideoViewClearMarksInOut, ETrue);
            }
        }

    if ( aResourceId == R_VEI_CUT_VIDEO_VIEW_MENU )
        {
        aMenuPane->SetItemDimmed( EVeiCmdCutVideoTakeSnapshot, ETrue );

        if ( iPopupMenuOpened != EFalse )
            {
            aMenuPane->SetItemDimmed( EVeiCmdCutVideoViewClearMarks, ETrue );
            aMenuPane->SetItemDimmed( EVeiCmdCutVideoViewHelp, ETrue );
            }

        if ( ( iMovie->AudioClipCutInTime( iIndex ) == TTimeIntervalMicroSeconds( 0 ) && 
            iMovie->AudioClipCutOutTime( iIndex ) == iMovie->AudioClipInfo( iIndex )->Duration() ) )
            {
            aMenuPane->DeleteMenuItem( EVeiCmdCutVideoViewPlayMarked );
            }
        if ( state != CVeiCutAudioContainer::EStatePlayingMenuOpen && 
            state != CVeiCutAudioContainer::EStatePaused )
            {
            aMenuPane->DeleteMenuItem( EVeiCmdCutVideoViewContinue );
            }
        if ( ( state == CVeiCutAudioContainer::EStateStopped ) ||
             ( state == CVeiCutAudioContainer::EStateStoppedInitial ) )
            {
            aMenuPane->DeleteMenuItem( EVeiCmdCutVideoViewStop );
            }

        if ( ( iMovie->AudioClipCutOutTime( iIndex ) >= iMovie->AudioClipInfo( iIndex )->Duration() ) &&
            ( iMovie->AudioClipCutInTime( iIndex ) <= TTimeIntervalMicroSeconds( 0 ) ) ) 
            {
            aMenuPane->SetItemDimmed( EVeiCmdCutVideoViewClearMarks, ETrue );
            }
        if ( state == CVeiCutAudioContainer::EStatePaused )
            {
            aMenuPane->SetItemDimmed( EVeiCmdCutVideoViewPlay, ETrue  );
            }
        if ( state == CVeiCutAudioContainer::EStateStoppedInitial )
            {
            //aMenuPane->SetItemDimmed( EVeiCmdCutVideoViewMarkIn, ETrue );
            //aMenuPane->SetItemDimmed( EVeiCmdCutVideoViewMarkOut, ETrue );    
            }
        }

    if ( aResourceId != R_VEI_CUT_VIDEO_VIEW_MENU )
        return;


    if ( ( state != CVeiCutAudioContainer::EStateStopped ) && 
        ( state != CVeiCutAudioContainer::EStateStoppedInitial ) &&
        ( state != CVeiCutAudioContainer::EStatePaused ) )
        {
        aMenuPane->SetItemDimmed( EVeiCmdCutVideoViewPlay, ETrue  );
        aMenuPane->SetItemDimmed( EVeiCmdCutVideoViewMarkIn, ETrue  );
        aMenuPane->SetItemDimmed( EVeiCmdCutVideoViewMarkOut, ETrue  ); 
        }
    else
        {
        if ( iContainer->PlaybackPositionL() <= iMovie->AudioClipCutInTime( iIndex ) )
            {
            aMenuPane->SetItemDimmed( EVeiCmdCutVideoViewMarkOut, ETrue );  
            }
        else 
            {
            if ( iContainer->PlaybackPositionL() >= iMovie->AudioClipCutOutTime( iIndex ) )
                {
                aMenuPane->SetItemDimmed( EVeiCmdCutVideoViewMarkIn, ETrue );
                }
            }
        }

    }

TUid CVeiCutAudioView::Id() const
    {
    return TUid::Uid( EVeiCutAudioView );
    }

void CVeiCutAudioView::HandleCommandL(TInt aCommand)
    {   
    switch ( aCommand )
        {
        case EAknSoftkeyOk:
            {
            iPopupMenuOpened = ETrue;
            if (iContainer->State() == CVeiCutAudioContainer::EStatePlaying) 
                {
                PausePreviewL();
                iContainer->SetStateL(CVeiCutAudioContainer::EStatePlayingMenuOpen);
                }

            MenuBar()->TryDisplayMenuBarL();
            if (iContainer->State() == CVeiCutAudioContainer::EStatePlayingMenuOpen) 
                {
                iContainer->SetStateL(CVeiCutAudioContainer::EStatePaused);
                }
            iPopupMenuOpened = EFalse;
            break;
            }       
        case EVeiCmdCutVideoViewDone:
        case EVeiCmdCutVideoViewBack:
        case EAknSoftkeyBack:
            {
            iContainer->CloseStreamL();

            StopNaviPaneUpdateL();

            // Activate Edit Video view
            AppUi()->ActivateLocalViewL( TUid::Uid(EVeiEditVideoView) );
            break;
            }

        case EVeiCmdCutVideoViewMarkIn:
            {
            MarkInL();
            break;
            }
        case EVeiCmdCutVideoViewMarkOut:
            {
            MarkOutL();
            break;
            }
        case EVeiCmdCutVideoViewClearMarksInOut:
            {
            ClearInOutL( ETrue, ETrue );
            break;
            }
        case EVeiCmdCutVideoViewClearMarksIn:
            {
            ClearInOutL( ETrue, EFalse );
            break;
            }
        case EVeiCmdCutVideoViewClearMarksOut:
            {
            ClearInOutL( EFalse, ETrue );
            break;
            }
        case EVeiCmdCutVideoViewPlayMarked:
            {
            STATIC_CAST( CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl() )->
                SetPauseIconVisibilityL( EFalse );
            PlayMarkedL();
            break;
            }
        case EVeiCmdCutVideoViewPlay:
            {
            STATIC_CAST( CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl() )->
                SetPauseIconVisibilityL( EFalse );
            iNaviPane->PushL( *iTimeNavi );
            
            PlayPreviewL();
            break;
            }
        case EVeiCmdCutVideoViewStop:
            {
            STATIC_CAST( CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl() )->
                SetPauseIconVisibilityL( EFalse );
            iNaviPane->PushL( *iTimeNavi );
            iContainer->StopL();
            StopNaviPaneUpdateL();
            break;
            }
        case EVeiCmdCutVideoViewContinue:
            {
            STATIC_CAST( CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl() )->
                SetPauseIconVisibilityL( EFalse );
            iNaviPane->PushL( *iTimeNavi );
            PlayPreviewL();
            break;
            }
        case EVeiCmdCutVideoVolumeDown:
            {
            if ( !iAudioMuted )
                {
                VolumeDownL();
                }
            break;
            }
        case EVeiCmdCutVideoVolumeUp:
            {
            if ( !iAudioMuted )
                {
                VolumeUpL();    
                }
            break;
            }
        //
        // Options->Help
        //
        case EVeiCmdCutVideoViewHelp:
            {
            // CS Help launching is handled in Video Editor's AppUi.
            AppUi()->HandleCommandL( EVeiCmdCutVideoViewHelp );
            break;
            }
        default:
            {
            AppUi()->HandleCommandL( aCommand );
            break;
            }
        }
    }


void CVeiCutAudioView::DoActivateL(
   const TVwsViewId& /*aPrevViewId*/,TUid /*aCustomMessageId*/,
   const TDesC8& /*aCustomMessage*/)
    {

    if (!iContainer)
        {
        iContainer = CVeiCutAudioContainer::NewL( AppUi()->ClientRect(), *this, *iErrorUI );
        iContainer->SetMopParent( this );
        AppUi()->AddToStackL( *this, iContainer );
        }

    CEikStatusPane *statusPane = ( ( CAknAppUi* )iEikonEnv->EikAppUi() )->StatusPane(); 
    
    CAknTitlePane* titlePane = (CAknTitlePane*) statusPane->ControlL( TUid::Uid( EEikStatusPaneUidTitle ) );
    TResourceReader reader1;
    iCoeEnv->CreateResourceReaderLC( reader1, R_VEI_CUTAUDIO_VIEW_TITLE_NAME );
    titlePane->SetFromResourceL( reader1 );
    CleanupStack::PopAndDestroy(); //reader1

    iNaviPane->PushL( *iTimeNavi );
    
    iAudioMuted = EFalse;

    STATIC_CAST( CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl())->
        SetVolumeIconVisibilityL( ETrue );
    STATIC_CAST( CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl() )->
            SetPauseIconVisibilityL( EFalse );
    
    iContainer->SetInTimeL( iMovie->AudioClipCutInTime( iIndex ) );
    iContainer->SetOutTimeL( iMovie->AudioClipCutOutTime( iIndex ) );
    iContainer->SetDuration( iMovie->AudioClipInfo( iIndex )->Duration() );
    
    // <testing>
/*  TTimeIntervalMicroSeconds time1 = iMovie->AudioClipInfo( iIndex )->Duration();
    TTimeIntervalMicroSeconds time2 = iMovie->AudioClipCutInTime( iIndex );
    TTimeIntervalMicroSeconds time3 = iMovie->AudioClipCutOutTime( iIndex );    
    TTimeIntervalMicroSeconds time4 = iMovie->AudioClipStartTime( iIndex );
    TTimeIntervalMicroSeconds time5 = iMovie->AudioClipEndTime( iIndex );
    TTimeIntervalMicroSeconds time6 = iMovie->AudioClipEditedDuration( iIndex );
    TTimeIntervalMicroSeconds time7(time3.Int64() - time2.Int64());
    TTimeIntervalMicroSeconds time8(time5.Int64() - time4.Int64());
*/  // </testing>

    // Start processing the file asynchronously. This is needed because
    // CVeiCutAudioContainer::OpenAudioFileL launches a progress dialog.
    // If it is called syncronously the previous view deactivation has 
    // not completed, and view shutter dismisses the progress dialog...
    if (! iCallBack)
        {       
        TCallBack cb (CVeiCutAudioView::AsyncOpenAudioFile, this);
        iCallBack = new (ELeave) CAsyncCallBack(cb, CActive::EPriorityStandard);
        }
    iCallBack->CallBack();

    iOriginalCutInTime = iMovie->AudioClipCutInTime( iIndex );
    
    DrawTimeNaviL();

    iErrorNmb = 0;      
    }

// ---------------------------------------------------------
// CVeiCutAudioView::AsyncOpenAudioFile
// ?implementation_description
// ---------------------------------------------------------
//
TInt CVeiCutAudioView::AsyncOpenAudioFile(TAny* aThis)
    {
    LOG( KVideoEditorLogFile, "CVeiCutAudioView::AsyncOpenAudioFile");

    // In the asynchronous version, trap the rest of the functions 
    // to make sure that the caller's TRequestStatus is always 
    // completed, also in case of failures.
    CVeiCutAudioView* view = static_cast<CVeiCutAudioView*>(aThis);
    TRAPD(err, view->OpenAudioFileL() );

    if (err)
        {
        // Display error message here, otherwise it would be quietly ignored.
        TBuf<256> unused;
        view->AppUi()->HandleError(err, SExtendedError(), unused, unused);
        }

    return err;
    }

// ---------------------------------------------------------
// CVeiCutAudioView::OpenAudioFileL()
// ?implementation_description
// ---------------------------------------------------------
//
void CVeiCutAudioView::OpenAudioFileL()
    {
    TFileName audioClipFileName = iMovie->AudioClipInfo( iIndex )->FileName();
    iContainer->OpenAudioFileL( audioClipFileName );
    }

// ---------------------------------------------------------
// CVeiCutAudioView::DoDeactivate()
// ?implementation_description
// ---------------------------------------------------------
//
void CVeiCutAudioView::DoDeactivate()
   {  
    iNaviPane->Pop( iVolumeNavi );

    if ( iTimeUpdater )
        {
        iTimeUpdater->Cancel();
        }
    if ( iVolumeHider )
        {
        iVolumeHider->Cancel();
        }
    if ( iContainer )
        {
        iNaviPane->Pop( iTimeNavi );
        AppUi()->RemoveFromViewStack( *this, iContainer );

        delete iContainer;
        iContainer = NULL;
        }

    }

void CVeiCutAudioView::SetVideoClipAndIndex( CVedMovie& aVideoClip, TInt aIndex )
    {
    iMovie = &aVideoClip;

    iIndex = aIndex;
    }

void CVeiCutAudioView::PlayPreviewL()
    {
    iPlayMarked = EFalse;
    StartNaviPaneUpdateL();
    iContainer->PlayL();
    }

void CVeiCutAudioView::PausePreviewL()
    {
    STATIC_CAST( CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl() )->
        SetPauseIconVisibilityL( ETrue );
    StopNaviPaneUpdateL();
    iContainer->PauseL();
    }

void CVeiCutAudioView::UpdateCBAL(TInt aState)
    {
    switch (aState)
        {
        case CVeiCutAudioContainer::EStateInitializing:
        case CVeiCutAudioContainer::EStateOpening:
            {
            Cba()->SetCommandSetL(R_AVKON_SOFTKEYS_EMPTY); 
            break;
            }
        case CVeiCutAudioContainer::EStateStoppedInitial:
            {           
            if ( ( iMovie->AudioClipCutInTime( iIndex ) == TTimeIntervalMicroSeconds( 0 ) ) &&
                 ( iMovie->AudioClipCutOutTime( iIndex ) == iMovie->AudioClipInfo(iIndex)->Duration() ) )
                {       
                Cba()->SetCommandSetL(R_AVKON_SOFTKEYS_OPTIONS_BACK);   
                }
            else
                {                                                               
                Cba()->SetCommandSetL(R_VEI_SOFTKEYS_OPTIONS_DONE);             
                }
            break;          
            }
        case CVeiCutAudioContainer::EStatePaused:
        case CVeiCutAudioContainer::EStateStopped:
            {
            if ( ( iMovie->AudioClipCutInTime( iIndex ) == TTimeIntervalMicroSeconds( 0 ) ) &&
                 ( iMovie->AudioClipCutOutTime( iIndex ) == iMovie->AudioClipInfo(iIndex)->Duration() ) )
                {       
                Cba()->SetCommandSetL(R_AVKON_SOFTKEYS_OPTIONS_BACK);   
                }
            else
                {                                                               
                Cba()->SetCommandSetL(R_VEI_SOFTKEYS_OPTIONS_DONE);             
                }
            break;
            }
        case CVeiCutAudioContainer::EStatePlaying:
            {

            if ( iContainer->PlaybackPositionL() < iMovie->AudioClipCutInTime( iIndex ) )
                {
                Cba()->SetCommandSetL( R_VEI_SOFTKEYS_IN_EMPTY ); 
                iMarkState = EMarkStateIn;
                }
            else if ( iContainer->PlaybackPositionL() < iMovie->AudioClipCutOutTime( iIndex ) )
                {
                Cba()->SetCommandSetL( R_VEI_SOFTKEYS_IN_OUT ); 
                iMarkState = EMarkStateInOut;
                }
            else if ( ( iContainer->PlaybackPositionL() > iMovie->AudioClipCutOutTime( iIndex ) ) && iPlayMarked )
                {
                iContainer->StopL();
                iPlayMarked = EFalse;
                }
            else
                {
                Cba()->SetCommandSetL( R_VEI_SOFTKEYS_EMPTY_OUT ); 
                iMarkState = EMarkStateOut;
                }
            break;
            }
        default:
            {
            break;  
            }
        }
    Cba()->DrawDeferred();
    }

void CVeiCutAudioView::PlayMarkedL()
    {
    TTimeIntervalMicroSeconds audioPlayStartTime;
    audioPlayStartTime = iMovie->AudioClipCutInTime( iIndex );
    
    TTimeIntervalMicroSeconds audioPlayCutOutTime;
    audioPlayCutOutTime = iMovie->AudioClipCutOutTime( iIndex );
    
    if ( !(( audioPlayStartTime.Int64() + 50000 ) > audioPlayCutOutTime.Int64() ) )
        {
        iPlayMarked = ETrue;    
        StartNaviPaneUpdateL();
    
        iContainer->PlayL( audioPlayStartTime.Int64() + 1000 );
        }
    else    
        {
        iContainer->StopL();
        }
    }

void CVeiCutAudioView::ClearInOutL( TBool aClearIn, TBool aClearOut )
    {
    if ( aClearIn ) 
        {
        iMovie->AudioClipSetCutInTime( iIndex, TTimeIntervalMicroSeconds( 0 ) );
        iContainer->SetInTimeL( iMovie->AudioClipCutInTime( iIndex ) );
        }
    if ( aClearOut ) 
        {
        TTimeIntervalMicroSeconds audioClipOriginalDuration;
        audioClipOriginalDuration = iMovie->AudioClipInfo( iIndex )->Duration();
        iMovie->AudioClipSetCutOutTime( iIndex, audioClipOriginalDuration );
        iContainer->SetOutTimeL( audioClipOriginalDuration );
        }
        
    TTimeIntervalMicroSeconds cutin = iMovie->AudioClipCutInTime( iIndex );
    TTimeIntervalMicroSeconds cutout = iMovie->AudioClipCutOutTime( iIndex );
    
    if ( ( cutin == TTimeIntervalMicroSeconds( 0 ) ) &&
         ( cutout == iMovie->AudioClipInfo(iIndex)->Duration() ) )
        {       
        Cba()->SetCommandSetL(R_AVKON_SOFTKEYS_OPTIONS_BACK);   
        Cba()->DrawDeferred();
        }   
    }

void CVeiCutAudioView::MarkInL()
    {
    STATIC_CAST( CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl() )->
                SetPauseIconVisibilityL( ETrue );
    StopNaviPaneUpdateL();
/* Check that cut in time is before cut out time */
    TTimeIntervalMicroSeconds cutOutTime = iMovie->AudioClipCutOutTime( iIndex );
    TTimeIntervalMicroSeconds cutInTime = iContainer->PlaybackPositionL();
    if ( cutInTime >= cutOutTime )
        {
        cutInTime = cutOutTime.Int64() - 100000;
        }
    
    iMovie->AudioClipSetCutInTime( iIndex, cutInTime );
    iContainer->MarkedInL();
    }

void CVeiCutAudioView::MarkOutL()
    {
    STATIC_CAST( CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl() )->
        SetPauseIconVisibilityL( ETrue );
    StopNaviPaneUpdateL();
    
    TTimeIntervalMicroSeconds cutOutTime = iContainer->PlaybackPositionL();
    TTimeIntervalMicroSeconds cutInTime = iMovie->AudioClipCutInTime( iIndex );
    if ( cutOutTime <= cutInTime )
        {
        cutOutTime = cutInTime.Int64() + 100000;
        }           
    
    //iMovie->AudioClipSetCutOutTime( iIndex, iContainer->PlaybackPositionL() );
    iMovie->AudioClipSetCutOutTime( iIndex, cutOutTime );   
    iContainer->MarkedOutL();
    }

void CVeiCutAudioView::MoveStartOrEndMarkL( TTimeIntervalMicroSeconds aPosition, CVeiCutAudioContainer::TCutMark aMarkType )
	{
	LOG(KVideoEditorLogFile, "CVeiCutAudioView::MoveStartOrEndMarkL, In");
	
	StopNaviPaneUpdateL();
	
	LOG(KVideoEditorLogFile, "CVeiCutAudioView::MoveStartOrEndMarkL, 2");
	
	if ( aMarkType == CVeiCutAudioContainer::EStartMark )
		{
		iMovie->VideoClipSetCutInTime( iIndex, aPosition );
		}
	else if ( aMarkType == CVeiCutAudioContainer::EEndMark )
		{
		iMovie->VideoClipSetCutOutTime( iIndex, aPosition );
		}		
	LOG( KVideoEditorLogFile, "CVeiCutAudioView::MoveStartOrEndMarkL, Out" );
	}


TUint CVeiCutAudioView::InPointTime()
    {
    if ( !iMovie )
        {
        return 0;
        }
    else
        {
        return (static_cast<TInt32>(iMovie->AudioClipCutInTime(iIndex).Int64() / 1000));
        }
    }

TUint CVeiCutAudioView::OutPointTime()
    {
    if ( !iMovie )
        {
        return 0;
        }
    else
        {
        return (static_cast<TInt32>(iMovie->AudioClipCutOutTime(iIndex).Int64() / 1000));
        }
    }

CAknNavigationDecorator* CVeiCutAudioView::CreateTimeLabelNaviL()
    {
    CVeiTimeLabelNavi* timelabelnavi = CVeiTimeLabelNavi::NewLC();
    CAknNavigationDecorator* decoratedFolder =
        CAknNavigationDecorator::NewL(iNaviPane, timelabelnavi, CAknNavigationDecorator::ENotSpecified);
    CleanupStack::Pop(timelabelnavi);
    
    CleanupStack::PushL(decoratedFolder);
    decoratedFolder->SetContainerWindowL(*iNaviPane);
    CleanupStack::Pop(decoratedFolder);
    decoratedFolder->MakeScrollButtonVisible(EFalse);

    return decoratedFolder;
    }

TInt CVeiCutAudioView::UpdateTimeCallbackL(TAny* aPtr)
    {
    CVeiCutAudioView* view = (CVeiCutAudioView*)aPtr;

    view->UpdateTimeL();

    return 1;
    }


void CVeiCutAudioView::UpdateTimeL()
    {
    DrawTimeNaviL();

    TTimeIntervalMicroSeconds playbackPos = iContainer->PlaybackPositionL();

    if (iMarkState == EMarkStateIn) 
        {
        if (playbackPos > iMovie->AudioClipCutInTime( iIndex )) 
            {
            UpdateCBAL(iContainer->State());
            }
        }
    else if (iMarkState == EMarkStateOut) 
        {
        if (playbackPos < iMovie->AudioClipCutOutTime( iIndex )) 
            {
            UpdateCBAL(iContainer->State());
            }
        }
    else 
        {
        if ((playbackPos < iMovie->AudioClipCutInTime( iIndex )) ||
            (playbackPos > iMovie->AudioClipCutOutTime( iIndex ))) 
            {
            UpdateCBAL(iContainer->State());
            }
        }
    }

void CVeiCutAudioView::DrawTimeNaviL()
    {
    TTime elapsed( iContainer->PlaybackPositionL().Int64() );
    TTime total( iContainer->TotalLength().Int64() );

    TBuf<16> elapsedBuf;
    TBuf<16> totalBuf;

    HBufC* dateFormatString;

    if ( ( total.Int64() / 1000 ) < 3600000 )   // check if time is over 99:59
        {
        dateFormatString = iEikonEnv->AllocReadResourceLC( R_QTN_TIME_DURAT_MIN_SEC );
        }
    else
        {
        dateFormatString = iEikonEnv->AllocReadResourceLC( R_QTN_TIME_DURAT_LONG );
        }

    elapsed.FormatL(elapsedBuf, *dateFormatString);
    total.FormatL(totalBuf, *dateFormatString);
    CleanupStack::PopAndDestroy(dateFormatString);
           
    CDesCArrayFlat* strings = new (ELeave) CDesCArrayFlat(2);
    CleanupStack::PushL(strings);
    strings->AppendL(elapsedBuf);
    strings->AppendL(totalBuf);
    HBufC* stringholder = StringLoader::LoadL(R_VEI_NAVI_TIME, *strings, iEikonEnv);
    CleanupStack::PopAndDestroy(strings);
    CleanupStack::PushL(stringholder);

    STATIC_CAST(CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl())->SetLabelL(stringholder->Des());

    CleanupStack::PopAndDestroy(stringholder);


    /* Prevent the screen light dimming. */
    if (elapsed.DateTime().Second() == 0 || elapsed.DateTime().Second() == 15 || elapsed.DateTime().Second() == 30 || elapsed.DateTime().Second() == 45)
        {
        User::ResetInactivityTime();
        }
    }


void CVeiCutAudioView::StartNaviPaneUpdateL()
    {
    DrawTimeNaviL();
    if (iTimeUpdater && !iTimeUpdater->IsActive())
        {
        iTimeUpdater->Start(500000, 500000, TCallBack(CVeiCutAudioView::UpdateTimeCallbackL, this));
        }
    }

void CVeiCutAudioView::StopNaviPaneUpdateL()
    {
    if (iContainer)
        {
        DrawTimeNaviL();
        }
    if (iTimeUpdater && iTimeUpdater->IsActive())
        {
        iTimeUpdater->Cancel();
        }
    }

TInt CVeiCutAudioView::HideVolumeCallbackL(TAny* aPtr)
    {
    CVeiCutAudioView* view = (CVeiCutAudioView*)aPtr;
    view->HideVolume();
    return 0;
    }

void CVeiCutAudioView::HideVolume()
    {
    iNaviPane->Pop(iVolumeNavi);
    }

void CVeiCutAudioView::VolumeMuteL()
    {
    iContainer->SetVolumeL(-1000);
    }

void CVeiCutAudioView::VolumeDownL()
    {
    iContainer->SetVolumeL(-1);
    TInt volume = iContainer->Volume();
    if (iVolumeHider->IsActive())
        {
        iVolumeHider->Cancel();
        }
    if (volume == 0) 
        {
        STATIC_CAST( CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl())->
            SetVolumeIconVisibilityL( EFalse );
        HideVolume();
        volume = 1;
        return;
        }

    iNaviPane->PushL(*iVolumeNavi);
    iVolumeHider->Start(1000000, 100000, TCallBack(CVeiCutAudioView::HideVolumeCallbackL, this));

    STATIC_CAST(CAknVolumeControl*, iVolumeNavi->DecoratedControl())->SetValue(volume);

    STATIC_CAST(CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl())->SetLeftArrowVisibilityL(ETrue);

    if (volume < iContainer->MaxVolume())
        {
        STATIC_CAST(CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl())->SetRightArrowVisibilityL(ETrue);
        }
    else
        {
        STATIC_CAST(CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl())->SetRightArrowVisibilityL(EFalse);
        }
    }

void CVeiCutAudioView::VolumeUpL()
    {
    iContainer->SetVolumeL(1);
    STATIC_CAST( CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl())->
            SetVolumeIconVisibilityL( ETrue );

    TInt volume = iContainer->Volume();
    if (iVolumeHider->IsActive())
        {
        iVolumeHider->Cancel();
        }
    iNaviPane->PushL(*iVolumeNavi);
    iVolumeHider->Start(1000000, 1000000, TCallBack(CVeiCutAudioView::HideVolumeCallbackL, this));

    STATIC_CAST(CAknVolumeControl*, iVolumeNavi->DecoratedControl())->SetValue(volume);

    if (volume > iContainer->MinVolume() + 1)
        {
        STATIC_CAST(CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl())->SetLeftArrowVisibilityL(ETrue);
        }
    else
        {
        STATIC_CAST(CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl())->SetLeftArrowVisibilityL(EFalse);
        }

    if (volume < iContainer->MaxVolume())
        {
        STATIC_CAST(CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl())->SetRightArrowVisibilityL(ETrue);
        }
    else
        {
        STATIC_CAST(CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl())->SetRightArrowVisibilityL(EFalse);
        }
    
    }

void CVeiCutAudioView::HandleForegroundEventL  ( TBool aForeground )
    {
    if ( !aForeground )
        {
        // If the application is closing down, calling PauseL could result in 
        // a callback from the MMF player after the container is already deleted,
        // causing KERN-EXEC 3
        if ( static_cast<CVeiAppUi*>( AppUi() )->AppIsOnTheWayToDestruction() )
            {
            iContainer->PrepareForTerminationL();
            }
        else
            {
            iContainer->PauseL();
            }
        iNaviPane->Pop( iTimeNavi );
        }
    else
        {
        iNaviPane->PushL( *iTimeNavi );
        }
    }

// ---------------------------------------------------------
// CVeiCutAudioView::HandleStatusPaneSizeChange()
// ---------------------------------------------------------
//
void CVeiCutAudioView::HandleStatusPaneSizeChange()
    {
    if ( iContainer )
        {
        iContainer->SetRect( AppUi()->ClientRect() );
        }
    }

void CVeiCutAudioView::GetAudioVisualizationL()
    {
    LOG(KVideoEditorLogFile, "CVeiCutAudioView::GetAudioVisualization(), In");
    CVedAudioClipInfo* audioInfo = NULL;
    if (iMovie)
        {       
        audioInfo = iMovie->AudioClipInfo( iIndex );        
        if (audioInfo && iContainer)
            {
            TInt64 duration = audioInfo->Duration().Int64();
            TInt resolution = (duration/1000)/KAudioSampleInterval;

            LOGFMT(KVideoEditorLogFile, "CVeiCutAudioView::GetAudioVisualization() 2, calling audioInfo->GetVisualizationL(*iContainer, resolution, 1), where resolution is:%Ld", resolution);
            audioInfo->GetVisualizationL(*iContainer, resolution, 1);
            }               
        }

    LOG(KVideoEditorLogFile, "CVeiCutAudioView::GetAudioVisualization(), Out");
    }

void CVeiCutAudioView::CancelVisualizationL()
    {
    CVedAudioClipInfo* audioInfo = NULL;
    if (iMovie)
        {       
        audioInfo = iMovie->AudioClipInfo( iIndex );        
        }
    if (audioInfo && iContainer)
        {       
        TInt resolution = (audioInfo->Duration().Int64()/1000)/KAudioSampleInterval;    
        audioInfo->CancelVisualizationL();      
        }   
    }

void CVeiCutAudioView::HandleResourceChange(TInt aType)
    {
    LOGFMT(KVideoEditorLogFile, "CVeiCutAudioView::HandleResourceChange() In, aType:%d", aType);
    
    if (KAknsMessageSkinChange == aType)
        {
        // Handle skin change in the navi label controls - they do not receive 
        // it automatically since they are not in the control stack
        iTimeNavi->DecoratedControl()->HandleResourceChange( aType );
        iVolumeNavi->DecoratedControl()->HandleResourceChange( aType );
        }
    
    LOG(KVideoEditorLogFile, "CVeiCutAudioView::HandleResourceChange() Out");
    }

// End of File
