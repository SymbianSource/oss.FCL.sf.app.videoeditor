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
#include <akntitle.h> 
#include <barsread.h>
#include <stringloader.h> 
#include <aknnotewrappers.h>
#include <aknquerydialog.h>
#include <aknlists.h>
#include <aknPopup.h>
#include <AknProgressDialog.h>
#include <aknnavide.h>
#include <eikbtgpc.h>
#include <eikmenub.h>
#include <eikmenup.h>
#include <eikprogi.h>
#include <CAknMemorySelectionDialog.h>
#include <CAknFileNamePromptDialog.h>
#include <apparc.h>
#include <aknselectionlist.h>
#include <sysutil.h>

// User includes
#include "VeiAppUi.h"
#include "VeiCutVideoView.h"
#include "VeiCutVideoContainer.h" 
#include "manualvideoeditor.hrh"
#include "veitempmaker.h"
#include "VeiTimeLabelNavi.h"
#include "VeiEditVideoView.h"
#include "VideoEditorCommon.h"
#include "VeiErrorUi.h"

void CVeiCutVideoView::ConstructL()
    {
    LOG(KVideoEditorLogFile, "CVeiCutVideoView::ConstructL: in");

    BaseConstructL( R_VEI_CUT_VIDEO_VIEW );

    CEikStatusPane* sp = StatusPane();

    iNaviPane = (CAknNavigationControlContainer*) sp->ControlL(
            TUid::Uid(EEikStatusPaneUidNavi));

    iTimeNavi = CreateTimeLabelNaviL();

    iVolumeNavi = iNaviPane->CreateVolumeIndicatorL(
            R_AVKON_NAVI_PANE_VOLUME_INDICATOR );

    iErrorUI = CVeiErrorUI::NewL( *iCoeEnv );   
    
    iTimeUpdater = CPeriodic::NewL( CActive::EPriorityLow );
    iVolumeHider = CPeriodic::NewL( CActive::EPriorityLow );

    LOG(KVideoEditorLogFile, "CVeiCutVideoView::ConstructL: out");
    }

// ---------------------------------------------------------
// CVeiCutVideoView::~CVeiCutVideoView()
// ?implementation_description
// ---------------------------------------------------------
//
CVeiCutVideoView::~CVeiCutVideoView()
    {
    if ( iProgressNote )
        {
        delete iProgressNote;
        iProgressNote = NULL;
        }
    if ( iContainer )
        {
        AppUi()->RemoveFromViewStack( *this, iContainer );
        delete iContainer;
        iContainer = 0;
        }
    if ( iTempMovie )
        {
        iTempMovie->Reset();
        delete iTempMovie;
        iTempMovie = NULL;
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

    delete iProcessedTempFile;
    }

TUid CVeiCutVideoView::Id() const
     {
     return TUid::Uid( EVeiCutVideoView );
     }

void CVeiCutVideoView::DialogDismissedL( TInt aButtonId )
    {
    if ( aButtonId != EAknSoftkeyDone )
        {   
        iTempMovie->CancelProcessing();
        }
    else
        {
        if ( iErrorNmb == KErrNone )
            {       
            iContainer->SetInTime( iMovie->VideoClipCutInTime( iIndex ) );
            iContainer->SetOutTime( iMovie->VideoClipCutOutTime( iIndex ) );

            iContainer->GetThumbL( *iProcessedTempFile ); 
            }
        else
            {
            if ( iProcessedTempFile )       // delete temp
                {
                iEikonEnv->FsSession().Delete( *iProcessedTempFile );
                delete iProcessedTempFile;
                iProcessedTempFile = NULL;
                }       

            iErrorUI->ShowGlobalErrorNote( iErrorNmb );
            iErrorNmb = 0;
            HandleCommandL( EVeiCmdCutVideoViewBack );
            }
        }
    }

void CVeiCutVideoView::DynInitMenuPaneL( TInt aResourceId,CEikMenuPane* aMenuPane )
    {
    TInt state = iContainer->State();

    if (aResourceId == R_VEI_CUT_VIDEO_VIEW_MENU_CLEAR_MARKS)
        {
        // delete in, out, in & out as necessary.

        if (iMovie->VideoClipCutInTime(iIndex) <= TTimeIntervalMicroSeconds(0)) 
            {
            aMenuPane->SetItemDimmed(EVeiCmdCutVideoViewClearMarksIn, ETrue);
            }
        if (iMovie->VideoClipCutOutTime(iIndex) >= iMovie->VideoClipInfo(iIndex)->Duration()) 
            {
            aMenuPane->SetItemDimmed(EVeiCmdCutVideoViewClearMarksOut, ETrue);
            }

        if (!((iMovie->VideoClipCutOutTime(iIndex) < iMovie->VideoClipInfo(iIndex)->Duration())
            && (iMovie->VideoClipCutInTime(iIndex) > TTimeIntervalMicroSeconds(0))))
            {
            aMenuPane->SetItemDimmed(EVeiCmdCutVideoViewClearMarksInOut, ETrue);
            }
        }

    if ( aResourceId == R_VEI_CUT_VIDEO_VIEW_MENU )
        {
        if ( iPopupMenuOpened != EFalse )
            {
            aMenuPane->SetItemDimmed( EVeiCmdCutVideoViewClearMarks, ETrue );
            aMenuPane->SetItemDimmed( EVeiCmdCutVideoTakeSnapshot, ETrue );
            aMenuPane->SetItemDimmed( EVeiCmdCutVideoViewHelp, ETrue );
            }
        }
    if ( !(( aResourceId == R_VEI_CUT_VIDEO_VIEW_MENU ) ||
            ( aResourceId == R_VEI_CUT_VIDEO_VIEW_MENU ) ) )
        return;

    if ( iContainer->PlaybackPositionL() >= iContainer->TotalLength() )
        {
        aMenuPane->SetItemDimmed( EVeiCmdCutVideoViewPlay, ETrue );

        }
    if ( ( state == CVeiCutVideoContainer::EStatePaused ) ||
            ( state == CVeiCutVideoContainer::EStateInitializing ) )
        {
        aMenuPane->SetItemDimmed( EVeiCmdCutVideoViewPlay, ETrue  );
        }

    if ( ( state != CVeiCutVideoContainer::EStateStopped ) && 
        ( state != CVeiCutVideoContainer::EStateStoppedInitial ) &&
        ( state != CVeiCutVideoContainer::EStatePaused ) )
        {
        aMenuPane->SetItemDimmed( EVeiCmdCutVideoViewPlay, ETrue  );
        aMenuPane->SetItemDimmed( EVeiCmdCutVideoViewMarkIn, ETrue  );
        aMenuPane->SetItemDimmed( EVeiCmdCutVideoViewMarkOut, ETrue  ); 
        }
    else
        {
        TTimeIntervalMicroSeconds pos = iContainer->PlaybackPositionL(); 
        CVedVideoClipInfo* info = iMovie->VideoClipInfo( iIndex );
        
        TInt cutInFrameIndex = info->GetVideoFrameIndexL( iMovie->VideoClipCutInTime( iIndex ));
        TInt cutOutFrameIndex = info->GetVideoFrameIndexL( iMovie->VideoClipCutOutTime( iIndex ));
        TInt videoFrameCount = info->VideoFrameCount();
        
        // if we are in the existing start/end mark position the start/end mark is removed from the menu
        if (info->GetVideoFrameIndexL(pos) == cutInFrameIndex)
            {
            aMenuPane->DeleteMenuItem( EVeiCmdCutVideoViewMarkIn ); 
            }       
        else if (info->GetVideoFrameIndexL(pos) == cutOutFrameIndex)
            {
            aMenuPane->DeleteMenuItem( EVeiCmdCutVideoViewMarkOut );    
            }
        
        if( cutInFrameIndex < (videoFrameCount-1 ) )
            {
            cutInFrameIndex++;
            }

        if( cutOutFrameIndex > 0 )
            {
            cutOutFrameIndex--;
            }

        TTimeIntervalMicroSeconds nextFramePosCutIn = info->VideoFrameStartTimeL( cutInFrameIndex );
        TTimeIntervalMicroSeconds previousFramePosCutOut = info->VideoFrameStartTimeL( cutOutFrameIndex );

        if ( pos < nextFramePosCutIn )
            {
            aMenuPane->DeleteMenuItem( EVeiCmdCutVideoViewMarkOut );    
            }
        else 
            {
            if ( pos > previousFramePosCutOut )
                {
                aMenuPane->DeleteMenuItem( EVeiCmdCutVideoViewMarkIn );
                }
            }
        // something crashes somewhere outside VED UI if end mark is put to near to begin               
        TInt ind = -1;  
        if (aMenuPane->MenuItemExists(EVeiCmdCutVideoViewMarkOut, ind) && pos.Int64() < KMinCutVideoLength) 
            {
            aMenuPane->DeleteMenuItem( EVeiCmdCutVideoViewMarkOut );    
            }   
        }
    if ( ( iMovie->VideoClipCutInTime( iIndex ) == TTimeIntervalMicroSeconds( 0 ) && 
        iMovie->VideoClipCutOutTime( iIndex ) == iMovie->VideoClipInfo( iIndex )->Duration() ) )
        {
        aMenuPane->DeleteMenuItem( EVeiCmdCutVideoViewPlayMarked );
        }
    if ( state != CVeiCutVideoContainer::EStatePlayingMenuOpen && 
            state != CVeiCutVideoContainer::EStatePaused )
        {
        aMenuPane->DeleteMenuItem( EVeiCmdCutVideoViewContinue );
        }
    if ( ( state == CVeiCutVideoContainer::EStateStopped ) ||
         ( state == CVeiCutVideoContainer::EStateStoppedInitial ) ||
         ( state == CVeiCutVideoContainer::EStateOpening ) ||
         ( state == CVeiCutVideoContainer::EStateBuffering ) )
        {
        aMenuPane->DeleteMenuItem( EVeiCmdCutVideoViewStop );
        }       

    if ( ( iMovie->VideoClipCutOutTime( iIndex ) >= iMovie->VideoClipInfo( iIndex )->Duration() ) &&
        ( iMovie->VideoClipCutInTime( iIndex ) <= TTimeIntervalMicroSeconds( 0 ) ) ) 
        {
        aMenuPane->SetItemDimmed( EVeiCmdCutVideoViewClearMarks, ETrue );
        }
    }

void CVeiCutVideoView::HandleCommandL(TInt aCommand)
    {   
    switch ( aCommand )
        {
        case EAknSoftkeyOk:
            {
            iPopupMenuOpened = ETrue;
            if (iContainer->State() == CVeiCutVideoContainer::EStatePlaying) 
                {
                PausePreviewL();
                iContainer->SetStateL(CVeiCutVideoContainer::EStatePlayingMenuOpen);
                }

            MenuBar()->TryDisplayMenuBarL();
            if (iContainer->State() == CVeiCutVideoContainer::EStatePlayingMenuOpen) 
                {
                iContainer->SetStateL(CVeiCutVideoContainer::EStatePaused);
                }
            iPopupMenuOpened = EFalse;
            break;
            }       
        case EVeiCmdCutVideoViewDone:
        case EVeiCmdCutVideoViewBack:
        case EAknSoftkeyBack:
            {
            StopNaviPaneUpdateL();

            iContainer->StopL();
            iContainer->CloseStreamL();

            // Activate Edit Video view
            AppUi()->ActivateLocalViewL( TUid::Uid(EVeiEditVideoView) );
            break;
            }

        case EVeiCmdCutVideoViewMarkIn:
            {
            STATIC_CAST( CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl() )->
                SetPauseIconVisibilityL( ETrue );
            MarkInL();
            break;
            }
        case EVeiCmdCutVideoViewMarkOut:
            {
            STATIC_CAST( CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl() )->
                SetPauseIconVisibilityL( ETrue );
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
            StopNaviPaneUpdateL();
            iContainer->StopL();
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
        case EVeiCmdCutVideoTakeSnapshot:
            {
            if( IsEnoughFreeSpaceToSaveL() )
                {
                iContainer->TakeSnapshotL();
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

void CVeiCutVideoView::NotifyMovieProcessingStartedL( CVedMovie& /*aMovie*/ )
    {
    LOG(KVideoEditorLogFile, "CVeiCutVideoView::NotifyMovieProcessingStartedL: In");
    iProgressNote = 
        new (ELeave) CAknProgressDialog(REINTERPRET_CAST(CEikDialog**, 
        &iProgressNote), ETrue);
    iProgressNote->SetCallback(this);
    iProgressNote->ExecuteDlgLD( R_VEI_PROGRESS_NOTE_WITH_CANCEL );

    HBufC* stringholder;

    stringholder = StringLoader::LoadL( R_VEI_PROGRESS_NOTE_CUT, iEikonEnv );
    CleanupStack::PushL( stringholder );

    iProgressNote->SetTextL( *stringholder );
    CleanupStack::PopAndDestroy( stringholder );  

    iProgressInfo = iProgressNote->GetProgressInfoL();
    iProgressInfo->SetFinalValue(100);
    LOG(KVideoEditorLogFile, "CVeiCutVideoView::NotifyMovieProcessingStartedL: Out");
    }

void CVeiCutVideoView::NotifyMovieProcessingProgressed( CVedMovie& /*aMovie*/, TInt aPercentage )
    {
    iProgressInfo->SetAndDraw( aPercentage );
    }

void CVeiCutVideoView::NotifyMovieProcessingCompleted( CVedMovie& /*aMovie*/, TInt aError )
    {
    LOGFMT(KVideoEditorLogFile, "CVeiCutVideoView::NotifyMovieProcessingCompleted: In, aError:%d", aError);
    if ( iTempMovie )   //Effected clip is ready, Delete tempmovie
        {
        iTempMovie->Reset();
        delete iTempMovie;
        iTempMovie = NULL;
        }

    if ( aError == KErrCancel )
        {
        HandleCommandL( EVeiCmdCutVideoViewBack );
        }
    else
        {
        iProgressInfo->SetAndDraw(100);
        iErrorNmb = aError;
        iProgressNote->ProcessFinishedL();
        }
    LOG(KVideoEditorLogFile, "CVeiCutVideoView::NotifyMovieProcessingCompleted: Out");
    }

void CVeiCutVideoView::NotifyVideoClipAdded( CVedMovie& /*aMovie*/, TInt /*aIndex*/ )
    {
    LOG(KVideoEditorLogFile, "CVeiCutVideoView::NotifyVideoClipAdded: In");

    iTempMovie->UnregisterMovieObserver( this );
    iTempMovie->VideoClipSetColorEffect( 0, iMovie->VideoClipColorEffect( iIndex ) );
    iTempMovie->VideoClipSetColorTone( 0, iMovie->VideoClipColorTone( iIndex ) );       
    iTempMovie->SetVideoClipVolumeGainL( 0, iMovie->GetVideoClipVolumeGainL( iIndex ) );

    iProcessedTempFile = HBufC::NewL( KMaxFileName );

    TRAPD( err, 
        CVeiTempMaker* maker = CVeiTempMaker::NewL();
        maker->GenerateTempFileName( *iProcessedTempFile, iMemoryInUse, iMovie->Format() );
        delete maker;
        );

    if ( err == KErrNone )
        {
        TRAP( err, iTempMovie->ProcessL( *iProcessedTempFile, *this ) );
        }

    if ( err != KErrNone )
        {
        iErrorUI->ShowGlobalErrorNote( err );

        if ( iProcessedTempFile )       // delete temp
            {
            iEikonEnv->FsSession().Delete( *iProcessedTempFile );
            delete iProcessedTempFile;
            iProcessedTempFile = NULL;
            }       
        }
    LOG(KVideoEditorLogFile, "CVeiCutVideoView::NotifyVideoClipAdded: Out");
    }

void CVeiCutVideoView::NotifyVideoClipAddingFailed( CVedMovie& /*aMovie*/, TInt /*aError*/ )
    {
    }

void CVeiCutVideoView::NotifyVideoClipRemoved( CVedMovie& /*aMovie*/, TInt /*aIndex*/ )
    {
    }

void CVeiCutVideoView::NotifyVideoClipIndicesChanged( CVedMovie& /*aMovie*/, TInt /*aOldIndex*/, 
                                               TInt /*aNewIndex*/ )
    {
    }

void CVeiCutVideoView::NotifyVideoClipTimingsChanged( CVedMovie& /*aMovie*/, TInt /*aIndex*/ )
    {
    }

void CVeiCutVideoView::NotifyVideoClipColorEffectChanged( CVedMovie& /*aMovie*/, TInt /*aIndex*/ )
    {
    }

void CVeiCutVideoView::NotifyVideoClipAudioSettingsChanged( CVedMovie& /*aMovie*/,
                                                     TInt /*aIndex*/ )
    {
    }

void CVeiCutVideoView::NotifyStartTransitionEffectChanged( CVedMovie& /*aMovie*/ )
    {
    }

void CVeiCutVideoView::NotifyMiddleTransitionEffectChanged( CVedMovie& /*aMovie*/, 
                                                     TInt /*aIndex*/ )
    {
    }

void CVeiCutVideoView::NotifyEndTransitionEffectChanged( CVedMovie& /*aMovie*/ )
    {
    }

void CVeiCutVideoView::NotifyAudioClipAdded( CVedMovie& /*aMovie*/, TInt /*aIndex*/ )
    {
    }

void CVeiCutVideoView::NotifyAudioClipAddingFailed( CVedMovie& /*aMovie*/, TInt /*aError*/ )
    {
    }

void CVeiCutVideoView::NotifyAudioClipRemoved( CVedMovie& /*aMovie*/, TInt /*aIndex*/ )
    {
    }

void CVeiCutVideoView::NotifyAudioClipIndicesChanged( CVedMovie& /*aMovie*/, TInt /*aOldIndex*/, 
                                               TInt /*aNewIndex*/ )
    {
    }

void CVeiCutVideoView::NotifyAudioClipTimingsChanged( CVedMovie& /*aMovie*/,
                                               TInt /*aIndex*/ )
    {
    }

void CVeiCutVideoView::NotifyMovieReseted( CVedMovie& /*aMovie*/ )
    {
    }

void CVeiCutVideoView::NotifyVideoClipGeneratorSettingsChanged(CVedMovie& /*aMovie*/,
                                                         TInt /*aIndex*/) 
    {
    }

void CVeiCutVideoView::NotifyVideoClipDescriptiveNameChanged(CVedMovie& /*aMovie*/, TInt /*aIndex*/)
    {
    }

void CVeiCutVideoView::NotifyMovieQualityChanged(CVedMovie& /*aMovie*/)
    {
    }

void CVeiCutVideoView::NotifyMovieOutputParametersChanged(CVedMovie& /*aMovie*/)
    {
    }

void CVeiCutVideoView::NotifyAudioClipDynamicLevelMarkInserted(CVedMovie& /*aMovie*/, 
                                                         TInt /*aClipIndex*/, 
                                                         TInt /*aMarkIndex*/)
    {
    }

void CVeiCutVideoView::NotifyAudioClipDynamicLevelMarkRemoved(CVedMovie& /*aMovie*/, 
                                                        TInt /*aClipIndex*/, 
                                                        TInt /*aMarkIndex*/)
    {
    }

void CVeiCutVideoView::NotifyVideoClipDynamicLevelMarkInserted(CVedMovie& /*aMovie*/,
                                                         TInt /*aClipIndex*/, 
                                                         TInt /*aMarkIndex*/)
    {
    }

void CVeiCutVideoView::NotifyVideoClipDynamicLevelMarkRemoved(CVedMovie& /*aMovie*/, 
                                                        TInt /*aClipIndex*/, 
                                                        TInt /*aMarkIndex*/)
    {
    }

void CVeiCutVideoView::DoActivateL(
   const TVwsViewId& /*aPrevViewId*/,TUid /*aCustomMessageId*/,
   const TDesC8& /*aCustomMessage*/)
    {

    if (!iContainer)
        {
        iContainer = CVeiCutVideoContainer::NewL( AppUi()->ClientRect(), *this, *iErrorUI );
        iContainer->SetMopParent( this );
        AppUi()->AddToStackL( *this, iContainer );
        }

    CEikStatusPane *statusPane = ( ( CAknAppUi* )iEikonEnv->EikAppUi() )->StatusPane(); 

    CAknTitlePane* titlePane = (CAknTitlePane*) statusPane->ControlL( TUid::Uid( EEikStatusPaneUidTitle ) );
    TResourceReader reader1;
    iCoeEnv->CreateResourceReaderLC( reader1, R_VEI_CUTVIDEO_VIEW_TITLE_NAME );
    titlePane->SetFromResourceL( reader1 );
    CleanupStack::PopAndDestroy(); //reader1

    UpdateCBAL( CVeiCutVideoContainer::EStateInitializing );
    iNaviPane->PushL( *iTimeNavi );

    iAudioMuted = !( iMovie->VideoClipEditedHasAudio( iIndex ) );

    STATIC_CAST( CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl() )->
            SetPauseIconVisibilityL( EFalse );

    if ( iAudioMuted  )
        {
        STATIC_CAST( CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl())->
            SetVolumeIconVisibilityL( EFalse );
        VolumeMuteL();
        }
    else
        {
        STATIC_CAST( CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl())->
            SetVolumeIconVisibilityL( ETrue );
        }
    iErrorNmb = 0;

    iContainer->SetInTime( iMovie->VideoClipCutInTime( iIndex ) );
    iContainer->SetOutTime( iMovie->VideoClipCutOutTime( iIndex ) );

    CVedVideoClipInfo* clipinfo = iMovie->VideoClipInfo( iIndex );

/**
* First try to open video clip in videoplayerutility. If it leaves with error code 
* KErrNotSupported(-5), then GenerateEffectedClipL() is called. 
*/  if (EVedColorEffectBlackAndWhite == iMovie->VideoClipColorEffect( iIndex ) ||
        EVedColorEffectToning == iMovie->VideoClipColorEffect( iIndex ))
        {
        GenerateEffectedClipL();
        }
    else if (iMovie->VideoClipInfo(iIndex)->HasAudio() && iMovie->GetVideoClipVolumeGainL(iIndex))
        {       
        GenerateEffectedClipL();
        } 
    else
        {
        iContainer->GetThumbL( clipinfo->FileName() );
        }
    }

void CVeiCutVideoView::GenerateEffectedClipL()
    {
    TEntry fileinfo;
// check if there is enough space to create temp file
    RFs&    fs = iEikonEnv->FsSession();

    CVedVideoClipInfo* videoclipinfo = iMovie->VideoClipInfo( iIndex );

    fs.Entry( videoclipinfo->FileName(), fileinfo );

    TBool spaceBelowCriticalLevel( EFalse );

    TVeiSettings movieSaveSettings;

   
    STATIC_CAST( CVeiAppUi*, iEikonEnv->AppUi() )->ReadSettingsL( movieSaveSettings );  

    CAknMemorySelectionDialog::TMemory memoryInUse( movieSaveSettings.MemoryInUse() );


    if ( memoryInUse == CAknMemorySelectionDialog::EPhoneMemory )
        {   
        spaceBelowCriticalLevel = SysUtil::DiskSpaceBelowCriticalLevelL( 
                                        &fs, fileinfo.iSize, EDriveC );
        }
    else
        {
        spaceBelowCriticalLevel = SysUtil::MMCSpaceBelowCriticalLevelL( 
                                        &fs, fileinfo.iSize );
        }   
    
    if ( !spaceBelowCriticalLevel )
        {
        iTempMovie = CVedMovie::NewL( NULL );
        iTempMovie->RegisterMovieObserverL( this );
        CVedVideoClipInfo* clipinfo = iMovie->VideoClipInfo( iIndex );
        iTempMovie->InsertVideoClipL( clipinfo->FileName(), 0 );
        }
    else 
        {
        HBufC* stringholder;

        stringholder = StringLoader::LoadLC( R_VEI_NOT_ENOUGH_SPACE, iEikonEnv );

        CAknErrorNote* dlg = new ( ELeave ) CAknErrorNote( ETrue );
        dlg->ExecuteLD( *stringholder );

        CleanupStack::PopAndDestroy( stringholder);

        HandleCommandL( EVeiCmdCutVideoViewBack );
        }

    }

// ---------------------------------------------------------
// CVeiCutVideoView::HandleCommandL(TInt aCommand)
// ?implementation_description
// ---------------------------------------------------------
//
void CVeiCutVideoView::DoDeactivate()
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

    if ( iProcessedTempFile )
        {
        iEikonEnv->FsSession().Delete( *iProcessedTempFile );
        delete iProcessedTempFile;
        iProcessedTempFile = NULL;
        }

    }

void CVeiCutVideoView::SetVideoClipAndIndex( CVedMovie& aVideoClip, TInt aIndex )
    {
    iMovie = &aVideoClip;
    iIndex = aIndex;
    }

void CVeiCutVideoView::PlayPreviewL()
    {
    StartNaviPaneUpdateL();
    iContainer->PlayL( iMovie->VideoClipInfo( iIndex )->FileName() );
    }

void CVeiCutVideoView::PausePreviewL()
    {
    STATIC_CAST( CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl() )->
            SetPauseIconVisibilityL( ETrue );
    StopNaviPaneUpdateL();

    iContainer->PauseL();
    }

void CVeiCutVideoView::UpdateCBAL(TInt aState)
    {
    switch (aState)
        {
        case CVeiCutVideoContainer::EStateInitializing:
        case CVeiCutVideoContainer::EStateOpening:
        case CVeiCutVideoContainer::EStateBuffering:        
            {
            Cba()->SetCommandSetL(R_AVKON_SOFTKEYS_EMPTY); 
            break;
            }
        case CVeiCutVideoContainer::EStateStoppedInitial:       
            {
            if ( ( iMovie->VideoClipCutInTime( iIndex ) == TTimeIntervalMicroSeconds( 0 ) ) &&
                 ( iMovie->VideoClipCutOutTime( iIndex ) == iMovie->VideoClipInfo(iIndex)->Duration() ) )
                {       
                Cba()->SetCommandSetL(R_AVKON_SOFTKEYS_OPTIONS_BACK);   
                }
            else
                {                                                               
                Cba()->SetCommandSetL(R_VEI_SOFTKEYS_OPTIONS_DONE);             
                }
            break;
            }
        case CVeiCutVideoContainer::EStatePaused:
        case CVeiCutVideoContainer::EStateStopped:
            {
            if ( ( iMovie->VideoClipCutInTime( iIndex ) == TTimeIntervalMicroSeconds( 0 ) ) &&
                 ( iMovie->VideoClipCutOutTime( iIndex ) == iMovie->VideoClipInfo(iIndex)->Duration() ) )
                {           
                Cba()->SetCommandSetL(R_AVKON_SOFTKEYS_OPTIONS_BACK);   
                }
            else
                {                                                               
                Cba()->SetCommandSetL(R_VEI_SOFTKEYS_OPTIONS_DONE);             
                }
            break;
            }
        case CVeiCutVideoContainer::EStatePlaying:
            {
            if ( iContainer->PlaybackPositionL() < iMovie->VideoClipCutInTime( iIndex ) )
                {
                Cba()->SetCommandSetL( R_VEI_SOFTKEYS_IN_EMPTY ); 
                iMarkState = EMarkStateIn;
                }
            else if ( iContainer->PlaybackPositionL() < iMovie->VideoClipCutOutTime( iIndex ) )
                {
                Cba()->SetCommandSetL( R_VEI_SOFTKEYS_IN_OUT ); 
                iMarkState = EMarkStateInOut;
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

void CVeiCutVideoView::PlayMarkedL()
    {
    LOGFMT3(KVideoEditorLogFile, "CVeiCutVideoView::PlayMarkedL: In: iIndex:%d, iMovie->VideoClipCutInTime():%Ld, iMovie->VideoClipCutOutTime():%Ld", iIndex, iMovie->VideoClipCutInTime( iIndex ).Int64(), iMovie->VideoClipCutOutTime( iIndex ).Int64());

    StartNaviPaneUpdateL(); 
    iContainer->PlayMarkedL( iMovie->VideoClipInfo( iIndex )->FileName(),
        iMovie->VideoClipCutInTime( iIndex ), iMovie->VideoClipCutOutTime( iIndex ) );      
    
    LOG(KVideoEditorLogFile, "CVeiCutVideoView::PlayMarkedL: Out");
    }

void CVeiCutVideoView::ClearInOutL( TBool aClearIn, TBool aClearOut )
    {
    if ( aClearIn ) 
        {
        iMovie->VideoClipSetCutInTime( iIndex, TTimeIntervalMicroSeconds( 0 ) );
        iContainer->SetInTime( iMovie->VideoClipCutInTime( iIndex ) );
        }
    if ( aClearOut ) 
        {
        iMovie->VideoClipSetCutOutTime( iIndex, iMovie->VideoClipInfo( iIndex )->Duration() );
        iContainer->SetOutTime( iMovie->VideoClipInfo( iIndex )->Duration() );
        }
    TTimeIntervalMicroSeconds cutin = iMovie->VideoClipCutInTime( iIndex );
    TTimeIntervalMicroSeconds cutout = iMovie->VideoClipCutOutTime( iIndex );
    
    if ( ( cutin == TTimeIntervalMicroSeconds( 0 ) ) &&
         ( cutout == iMovie->VideoClipInfo(iIndex)->Duration() ) )
        {       
        Cba()->SetCommandSetL(R_AVKON_SOFTKEYS_OPTIONS_BACK);   
        Cba()->DrawDeferred();
        }   
    }

void CVeiCutVideoView::MarkInL()
    {
    LOG(KVideoEditorLogFile, "CVeiCutVideoView::MarkInL, In");
    TTimeIntervalMicroSeconds pos = iContainer->PlaybackPositionL();
    
//  TTimeIntervalMicroSeconds clipDuration = iMovie->VideoClipInfo( iIndex )->Duration();
//  CVedVideoClipInfo* info = iMovie->VideoClipInfo( iIndex );
//  TTimeIntervalMicroSeconds intraPos = info->VideoFrameStartTimeL( 
//          info->GetVideoFrameIndexL( pos ) );

    if (iMovie->VideoClipCutOutTime(iIndex) > pos)
        {
        StopNaviPaneUpdateL();      
        LOGFMT2(KVideoEditorLogFile, "CVeiCutVideoView::MarkInL, 2, iIndex:%d, pos:%Ld", iIndex, pos.Int64());
        iMovie->VideoClipSetCutInTime( iIndex, pos );
        iContainer->MarkedInL();    
        }
    
    LOG(KVideoEditorLogFile, "CVeiCutVideoView::MarkInL, Out");
    }

void CVeiCutVideoView::MarkOutL()
    {
    LOG(KVideoEditorLogFile, "CVeiCutVideoView::MarkOutL, In");
    TTimeIntervalMicroSeconds pos = iContainer->PlaybackPositionL();
//  CVedVideoClipInfo* info = iMovie->VideoClipInfo( iIndex );
//  TTimeIntervalMicroSeconds intraPos = info->VideoFrameStartTimeL( 
//          info->GetVideoFrameIndexL( pos ) );

    if (iMovie->VideoClipCutInTime(iIndex) < pos)
        {           
        StopNaviPaneUpdateL();
        iMovie->VideoClipSetCutOutTime( iIndex, pos );
        LOGFMT2(KVideoEditorLogFile, "CVeiCutVideoView::MarkOutL, 2, iIndex:%d, pos:%Ld", iIndex, pos.Int64() );        
        iContainer->MarkedOutL();
        }
    LOG(KVideoEditorLogFile, "CVeiCutVideoView::MarkOutL, Out");
    }

void CVeiCutVideoView::MoveStartOrEndMarkL( TTimeIntervalMicroSeconds aPosition, CVeiCutVideoContainer::TCutMark aMarkType )
	{
	LOG( KVideoEditorLogFile, "CVeiCutVideoView::MoveStartOrEndMarkL, In" );
	
	StopNaviPaneUpdateL();
	
	LOG( KVideoEditorLogFile, "CVeiCutVideoView::MoveStartOrEndMarkL, 2" );
	
	if ( aMarkType == CVeiCutVideoContainer::EStartMark )
		{
		iMovie->VideoClipSetCutInTime( iIndex, aPosition );
		}
	else if ( aMarkType == CVeiCutVideoContainer::EEndMark )
		{
		iMovie->VideoClipSetCutOutTime( iIndex, aPosition );
		}		
	LOG( KVideoEditorLogFile, "CVeiCutVideoView::MoveStartOrEndMarkL, Out" );
	}


TUint CVeiCutVideoView::InPointTime()
    {
    if ( !iMovie )
        {
        return 0;
        }
    else
        {
        return static_cast<TInt32>((iMovie->VideoClipCutInTime(iIndex).Int64() / 1000));
        }
    }

TUint CVeiCutVideoView::OutPointTime()
    {
    if ( !iMovie )
        {
        return 0;
        }
    else
        {
        return static_cast<TInt32>(iMovie->VideoClipCutOutTime(iIndex).Int64() / 1000);
        }
    }

CAknNavigationDecorator* CVeiCutVideoView::CreateTimeLabelNaviL()
    {
    LOG(KVideoEditorLogFile, "CVeiCutVideoView::CreateTimeLabelNaviL: in");
    CVeiTimeLabelNavi* timelabelnavi = CVeiTimeLabelNavi::NewLC();
    CAknNavigationDecorator* decoratedFolder =
        CAknNavigationDecorator::NewL(iNaviPane, timelabelnavi, CAknNavigationDecorator::ENotSpecified);
    CleanupStack::Pop(timelabelnavi);
    
    CleanupStack::PushL(decoratedFolder);
    decoratedFolder->SetContainerWindowL(*iNaviPane);
    CleanupStack::Pop(decoratedFolder);
    decoratedFolder->MakeScrollButtonVisible(EFalse);
    LOG(KVideoEditorLogFile, "CVeiCutVideoView::CreateTimeLabelNaviL: out");
    return decoratedFolder;
    }

TInt CVeiCutVideoView::UpdateTimeCallbackL(TAny* aPtr)
    {
    CVeiCutVideoView* view = (CVeiCutVideoView*)aPtr;

    view->UpdateTimeL();

    return 1;
    }


void CVeiCutVideoView::UpdateTimeL()
    {
    DrawTimeNaviL();

    if (iMarkState == EMarkStateIn) 
        {
        if (iContainer->PlaybackPositionL() > iMovie->VideoClipCutInTime( iIndex )) 
            {
            UpdateCBAL(iContainer->State());
            }
        }
    else if (iMarkState == EMarkStateOut) 
        {
        if (iContainer->PlaybackPositionL() < iMovie->VideoClipCutOutTime( iIndex )) 
            {
            UpdateCBAL(iContainer->State());
            }
        }
    else 
        {
        if ((iContainer->PlaybackPositionL() < iMovie->VideoClipCutInTime( iIndex )) ||
            (iContainer->PlaybackPositionL() > iMovie->VideoClipCutOutTime( iIndex ))) 
            {
            UpdateCBAL(iContainer->State());
            }
        }
    }

void CVeiCutVideoView::DrawTimeNaviL()
    {
    TTime elapsed( iContainer->PlaybackPositionL().Int64() );
    TTime total( iContainer->TotalLength().Int64() );

    TBuf<16> elapsedBuf;
    TBuf<16> totalBuf;

    HBufC* dateFormatString;
    HBufC* stringholder;

    if ( ( total.Int64() / 1000 ) < 3600000 )   // check if time is over 99:59
        {
        dateFormatString = iEikonEnv->AllocReadResourceLC( R_QTN_TIME_DURAT_MIN_SEC );
    
        elapsed.FormatL(elapsedBuf, *dateFormatString);
        total.FormatL(totalBuf, *dateFormatString);
        CleanupStack::PopAndDestroy(dateFormatString);
           
        CDesCArrayFlat* strings = new (ELeave) CDesCArrayFlat(2);
        CleanupStack::PushL(strings);
        strings->AppendL(elapsedBuf);
        strings->AppendL(totalBuf);
        stringholder = StringLoader::LoadL(R_VEI_NAVI_TIME, *strings, iEikonEnv);
        CleanupStack::PopAndDestroy(strings);

        
        CleanupStack::PushL(stringholder);  

        STATIC_CAST(CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl())->SetLabelL(stringholder->Des());

        CleanupStack::PopAndDestroy(stringholder);
        }
    else
        {
        TBuf<25> layoutTime;
        TBuf<15> minsec;   
        TTimeIntervalMinutes minutes; 
        TTimeIntervalMicroSeconds32 seconds; 
        
        TInt64 duration = ( iContainer->PlaybackPositionL().Int64() / 1000 ); 
        TChar timeSeparator = TLocale().TimeSeparator(2);
        //over 1 minute
        if( duration >= 60000 ) 
            { 
            minutes = TTimeIntervalMinutes(static_cast<TInt32>( duration) / 60000 ); 
            minsec.AppendNum( minutes.Int() ); 
            minsec.Append( timeSeparator ); 

            duration = duration - minutes.Int() * 60000; 
            }
        else
            {
            minsec.Append( _L( "00" ) ); 
            minsec.Append( timeSeparator ); 
            }   
        if( duration >= 1000 ) 
            { 
            seconds = TTimeIntervalMicroSeconds32( static_cast<TInt32>(duration) / 1000 ); 

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
        layoutTime.Append( minsec );
        AknTextUtils::DisplayTextLanguageSpecificNumberConversion( layoutTime );

        STATIC_CAST(CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl())->SetLabelL(layoutTime);
        }


    /* Prevent the screen light dimming. */
    if (elapsed.DateTime().Second() == 0 || elapsed.DateTime().Second() == 15 || elapsed.DateTime().Second() == 30 || elapsed.DateTime().Second() == 45)
        {
        User::ResetInactivityTime();
        }
    }


void CVeiCutVideoView::StartNaviPaneUpdateL()
    {
    DrawTimeNaviL();
    if (iTimeUpdater && !iTimeUpdater->IsActive())
        {
        iTimeUpdater->Start(200000, 1000000/10, TCallBack(CVeiCutVideoView::UpdateTimeCallbackL, this));
        }
    }

void CVeiCutVideoView::StopNaviPaneUpdateL()
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

TInt CVeiCutVideoView::HideVolumeCallbackL(TAny* aPtr)
    {
    CVeiCutVideoView* view = (CVeiCutVideoView*)aPtr;
    view->HideVolume();
    return 0;
    }

void CVeiCutVideoView::HideVolume()
    {
    iNaviPane->Pop(iVolumeNavi);
    }

void CVeiCutVideoView::VolumeMuteL()
    {
    iContainer->MuteL();
    }



void CVeiCutVideoView::ShowVolumeLabelL( TInt aVolume )
    {
    STATIC_CAST( CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl())->
            SetVolumeIconVisibilityL( ETrue );

    if (iVolumeHider->IsActive())
        {
        iVolumeHider->Cancel();
        }
    if (aVolume == 0) 
        {
        STATIC_CAST( CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl())->
            SetVolumeIconVisibilityL( EFalse );
        HideVolume();
        return;
        }

    iNaviPane->PushL(*iVolumeNavi);
    iVolumeHider->Start(1000000, 1000000, TCallBack(CVeiCutVideoView::HideVolumeCallbackL, this));

    STATIC_CAST(CAknVolumeControl*, iVolumeNavi->DecoratedControl())->SetValue(aVolume);
    
    if (aVolume > iContainer->MinVolume() + 1 )
        {
        STATIC_CAST(CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl())->SetLeftArrowVisibilityL(ETrue);
        }
    else
        {
        STATIC_CAST(CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl())->SetLeftArrowVisibilityL(EFalse);
        }

    if (aVolume < iContainer->MaxVolume())
        {
        STATIC_CAST(CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl())->SetRightArrowVisibilityL(ETrue);
        }
    else
        {
        STATIC_CAST(CVeiTimeLabelNavi*, iTimeNavi->DecoratedControl())->SetRightArrowVisibilityL(EFalse);
        }
    
    }

void CVeiCutVideoView::HandleForegroundEventL  ( TBool aForeground )
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
            iContainer->PauseL( EFalse );
            }
        iNaviPane->Pop( iTimeNavi );
        }
    else
        {
        UpdateCBAL( iContainer->State() );
        iNaviPane->PushL( *iTimeNavi );
        }
    }

// ---------------------------------------------------------
// CVeiCutVideoView::HandleStatusPaneSizeChange()
// ---------------------------------------------------------
//
void CVeiCutVideoView::HandleStatusPaneSizeChange()
    {
    if ( iContainer )
        {
        iContainer->SetRect( AppUi()->ClientRect() );
        }
    }

TBool CVeiCutVideoView::IsEnoughFreeSpaceToSaveL()// const
    {

    STATIC_CAST( CVeiAppUi*, AppUi() )->ReadSettingsL( iMovieSaveSettings );
    iMemoryInUse = iMovieSaveSettings.MemoryInUse();

    RFs&    fs = iEikonEnv->FsSession();
    TBool spaceBelowCriticalLevel( EFalse );

    /* seek position of clip */
    TTimeIntervalMicroSeconds frame;
    frame = iContainer->PlaybackPositionL();

    /* frame index of position */
    TInt frameIndex;
    frameIndex = iMovie->VideoClipInfo( iIndex )->GetVideoFrameIndexL( frame );

    /* frame size */
    TInt sizeEstimate; 
    sizeEstimate = iMovie->VideoClipInfo( iIndex )->VideoFrameSizeL( frameIndex );

    if ( iMemoryInUse == CAknMemorySelectionDialog::EPhoneMemory )
        {   
        spaceBelowCriticalLevel = SysUtil::DiskSpaceBelowCriticalLevelL( &fs, sizeEstimate, EDriveC );
        }
    else
        {
        spaceBelowCriticalLevel = SysUtil::MMCSpaceBelowCriticalLevelL( &fs, sizeEstimate );
        }   


    if ( !spaceBelowCriticalLevel )
        {
        return ETrue;
        }
    else 
        {
        HBufC* stringholder;
        stringholder = StringLoader::LoadLC( R_VEI_MEMORY_RUNNING_OUT, iEikonEnv );
        CAknErrorNote* dlg = new ( ELeave ) CAknErrorNote( ETrue );
        dlg->ExecuteLD( *stringholder );
        CleanupStack::PopAndDestroy( stringholder );  

        return EFalse;
        }
    }

TTimeIntervalMicroSeconds CVeiCutVideoView::GetVideoClipCutInTime()
    {
    TTimeIntervalMicroSeconds cutInTime(0);
    if ( iMovie )
        {
        cutInTime = iMovie->VideoClipCutInTime( iIndex );
        }
    return cutInTime;
    }

TTimeIntervalMicroSeconds CVeiCutVideoView::GetVideoClipCutOutTime()
    {
    TTimeIntervalMicroSeconds cutOutTime(0);
    if ( iMovie )
        {
        cutOutTime = iMovie->VideoClipCutOutTime( iIndex );
        }
    return cutOutTime;
    }

void CVeiCutVideoView::HandleResourceChange(TInt aType)
    {
    LOGFMT(KVideoEditorLogFile, "CVeiEditVideoView::HandleResourceChange() In, aType:%d", aType);
    
    if (KAknsMessageSkinChange == aType)
        {
        // Handle skin change in the navi label controls - they do not receive 
        // it automatically since they are not in the control stack
        iTimeNavi->DecoratedControl()->HandleResourceChange( aType );
        iVolumeNavi->DecoratedControl()->HandleResourceChange( aType );
        }
    
    LOG(KVideoEditorLogFile, "CVeiEditVideoView::HandleResourceChange() Out");
    }
 
// End of File

