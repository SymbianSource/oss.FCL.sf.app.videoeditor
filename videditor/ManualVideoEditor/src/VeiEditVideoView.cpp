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
#include <aknappui.h>
#include <akntitle.h> 
#include <caknfilenamepromptdialog.h> 
#include <aknnotewrappers.h> 
#include <aknlists.h> 
#include <aknquerydialog.h> 
#include <pathinfo.h> 
#include <eikmenub.h> 

#include <sendui.h> 
#include <caknfileselectiondialog.h>
#include <stringloader.h> 
#include <eikprogi.h>
#include <mgfetch.h> 
#include <aknnavilabel.h> 
#include <aknnavide.h> 
#include <aknselectionlist.h> 
#include <MdaAudioSampleEditor.h> 
#include <bautils.h>
#include <sysutil.h>
#include <aknwaitdialog.h>
#include <utf.h>
#include <akncolourselectiongrid.h>
//#include <akncontext.h>
#include <MGXFileManagerFactory.h>
#include <CMGXFileManager.h>
#include <audiopreference.h>
#include <senduiconsts.h>
#include <mmsconst.h>
#include <CMessagedata.h>
#include <e32property.h>
#include <e32math.h> 

#include <VedVideoClipInfo.h>

// User includes 
#include "veiapp.h"
#include "veicutaudioview.h"
#include "VeiEditVideoView.h"
#include "VeiEditVideoContainer.h" 
#include "manualvideoeditor.hrh"
#include "veieditvideolabelnavi.h"
#include "veicutvideoview.h"
#include "veiappui.h"
#include "VideoEditorCommon.h"
#include "VideoEditorUtils.h"
#include "VeiTimeLabelNavi.h"
#include "veiaddqueue.h"
#include "veitempmaker.h"
#include "videoeditorcommon.h"
#include "VeiImageClipGenerator.h"
#include "VeiTitleClipGenerator.h"
#include "VeiTextDisplay.h"
#include "VeiPopup.h"
#include "VeiVideoEditorSettings.h"
#include "VeiMGFetchVerifier.h"
#include "VeiErrorUi.h"


const TInt KTitleScreenMaxTextLength = 100; //2048;


void CleanupRestoreOrientation( TAny* object )
    {
    LOG( KVideoEditorLogFile, "CleanupRestoreOrientation: in" );

    CVeiEditVideoView* me = static_cast < CVeiEditVideoView*  > ( object );
    me->RestoreOrientation();

    LOG( KVideoEditorLogFile, "CleanupRestoreOrientation: Out" );
    }

CVeiEditVideoView* CVeiEditVideoView::NewL( CVeiCutVideoView& aCutView,
                                            CVeiCutAudioView& aCutAudioView, 
                                            CSendUi& aSendAppUi )
    {
    CVeiEditVideoView* self = CVeiEditVideoView::NewLC( aCutView, aCutAudioView, aSendAppUi );
    CleanupStack::Pop( self );
    return self;
    }

CVeiEditVideoView* CVeiEditVideoView::NewLC( CVeiCutVideoView& aCutView,
                                             CVeiCutAudioView& aCutAudioView, 
                                             CSendUi& aSendAppUi )
    {
    CVeiEditVideoView* self = new ( ELeave ) CVeiEditVideoView( aCutView, aCutAudioView, aSendAppUi );
    CleanupStack::PushL( self );
    self->ConstructL();
    return self;
    }

CVeiEditVideoView::CVeiEditVideoView( CVeiCutVideoView& aCutView, 
                                      CVeiCutAudioView& aCutAudioView, 
                                      CSendUi& aSendAppUi )
    : iCutView( aCutView ), 
      iCutAudioView( aCutAudioView ), 
      iSendAppUi( aSendAppUi ), 
      iOriginalAudioStartPoint( -1 ), 
      iOriginalAudioDuration( -1 ), 
      iMemoryCardChecked(EFalse),
      iOriginalOrientation( CAknAppUiBase::EAppUiOrientationAutomatic )
    {
    iOriginalVideoClipIndex =  - 1;
    iMovieSavedFlag = ETrue;
    SetNewTempFileNeeded( EFalse );
    iMovieFirstAddFlag = ETrue;
    iWaitMode = ENotWaiting;
    }

void CVeiEditVideoView::ConstructL()
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::ConstructL: in" );

    BaseConstructL( R_VEI_EDIT_VIDEO_VIEW );

    CEikStatusPane* sp = StatusPane();
    iNaviPane = ( CAknNavigationControlContainer* ) sp->ControlL( TUid::Uid( EEikStatusPaneUidNavi ) );

    /** General navilabel(mms state, movie size, movie time). 
    Is visible always except when in moving state */
    iEditLabel = CreateEditNaviLabelL();


    /** Navilabel when previewing video in small preview */
    iPreviewLabel = CreatePreviewNaviLabelL();

    /** Navipane updating timer when editor is on small preview - mode */
    iPreviewUpdatePeriodic = CPeriodic::NewL( CActive::EPriorityLow );

    /* volume bars are showed when volume is changed in small preview */
    //	iVolumeHider = CPeriodic::NewL( CActive::EPriorityLow );

    iVolumeNavi = iNaviPane->CreateVolumeIndicatorL(
        R_AVKON_NAVI_PANE_VOLUME_INDICATOR );

    /** Navilabel when audio or video clip is in moving state */
    iMoveLabel = CreateMoveNaviLabelL();

    iTempMaker = CVeiTempMaker::NewL();

    iErrorUI = CVeiErrorUI::NewL( *iCoeEnv );

    iMovie = CVedMovie::NewL( NULL );
    iMovie->RegisterMovieObserverL( this );

    /** All media files are added to video/audio tracks through mediaqueue */
    iMediaQueue = CVeiAddQueue::NewL( *this, * iMovie, * this );

    iOriginalAudioDuration = TTimeIntervalMicroSeconds(  - 1 );
    /* Create recorder with max priority. */
    iRecorder = CMdaAudioRecorderUtility::NewL( *this, NULL, EMdaPriorityMax,
                       TMdaPriorityPreference( KAudioPrefVideoRecording )
                       /*EMdaPriorityPreferenceQuality*/ );
    iAudioRecordPeriodic = CPeriodic::NewL( CActive::EPriorityStandard );

    /** Popup menus for video editor */
    iPopup = CVeiPopup::NewL( *this );

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::ConstructL: out" );
    }

TUid CVeiEditVideoView::Id()const
    {
    return TUid::Uid( EVeiEditVideoView );
    }

CAknNavigationDecorator* CVeiEditVideoView::CreateMoveNaviLabelL()
    {
    CAknNaviLabel* movelabel = new( ELeave )CAknNaviLabel;
    movelabel->SetNaviLabelType( CAknNaviLabel::ENavigationLabel );

    CleanupStack::PushL( movelabel );

    CAknNavigationDecorator* decoratedFolder = CAknNavigationDecorator::NewL(
        iNaviPane, movelabel, CAknNavigationDecorator::ENotSpecified );

    CleanupStack::Pop( movelabel );

    CleanupStack::PushL( decoratedFolder );
    decoratedFolder->SetContainerWindowL( *iNaviPane );
    CleanupStack::Pop( decoratedFolder );
    decoratedFolder->MakeScrollButtonVisible( ETrue );

    return decoratedFolder;
    }

CAknNavigationDecorator* CVeiEditVideoView::CreatePreviewNaviLabelL()
    {
    CVeiTimeLabelNavi* timelabelnavi = CVeiTimeLabelNavi::NewLC();
    CAknNavigationDecorator* decoratedFolder = CAknNavigationDecorator::NewL(
        iNaviPane, timelabelnavi, CAknNavigationDecorator::ENotSpecified );
    CleanupStack::Pop( timelabelnavi );

    CleanupStack::PushL( decoratedFolder );
    decoratedFolder->SetContainerWindowL( *iNaviPane );
    CleanupStack::Pop( decoratedFolder );
    decoratedFolder->MakeScrollButtonVisible( EFalse );

    return decoratedFolder;
    }

/**
 * Default navilabel. Shows MMS,Size and Time information. 
 */
CAknNavigationDecorator* CVeiEditVideoView::CreateEditNaviLabelL()
    {
    CVeiEditVideoLabelNavi* editvideolabelnavi = CVeiEditVideoLabelNavi::NewLC();

    editvideolabelnavi->SetState( CVeiEditVideoLabelNavi::EStateInitializing );

    CAknNavigationDecorator* navidecorator = CAknNavigationDecorator::NewL(
        iNaviPane, editvideolabelnavi, CAknNavigationDecorator::ENotSpecified );

    CleanupStack::Pop( editvideolabelnavi );

    CleanupStack::PushL( navidecorator );
    navidecorator->SetContainerWindowL( *iNaviPane );
    CleanupStack::Pop( navidecorator );
    navidecorator->MakeScrollButtonVisible( EFalse );

    return navidecorator;
    }

CVeiEditVideoView::~CVeiEditVideoView()
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::~CVeiEditVideoView: In" );

    if ( iProgressNote )
        {
        delete iProgressNote;
        iProgressNote = NULL;
        }

    if ( iPopup )
        {
        delete iPopup;
        iPopup = NULL;
        }

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::~CVeiEditVideoView: iContainer" );
    if ( iContainer )
        {
        AppUi()->RemoveFromStack( iContainer );
        delete iContainer;
        iContainer = NULL;
        }

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::~CVeiEditVideoView: iMediaQueue" );
    if ( iMediaQueue )
        {
        delete iMediaQueue;
        }

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::~CVeiEditVideoView: iMovie" );
    if ( iMovie )
        {
        iMovie->Reset();
        //if application is closed from cut video view, observer is not
        //registered.
        if ( ( EProcessingMovieForCutting != iWaitMode ) &&
            ( ECuttingAudio != iWaitMode ) )
            {
            iMovie->UnregisterMovieObserver( this );
            }
        delete iMovie;
        iMovie = NULL;
        }

    if ( iAudioRecordPeriodic )
        {
        iAudioRecordPeriodic->Cancel();
        delete iAudioRecordPeriodic;
        }

    if ( iPreviewUpdatePeriodic )
        {
        iPreviewUpdatePeriodic->Cancel();
        delete iPreviewUpdatePeriodic;
        }

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::~CVeiEditVideoView: iTempFile" );
    if ( iTempFile )
        {
        TInt err = iEikonEnv->FsSession().Delete( *iTempFile );
        if ( err )
            {
            // what to do when error occurs in destructor???
            }
        delete iTempFile;
        iTempFile = NULL;
        }

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::~CVeiEditVideoView: iSaveToFileName" );
        
    if ( iSaveToFileName )
        {
        delete iSaveToFileName;
        iSaveToFileName = NULL;
        }

    if ( iTempRecordedAudio )
        {
        TInt err = iEikonEnv->FsSession().Delete( *iTempRecordedAudio );
        if ( err )
            {
            // what to do when error occurs in destructor???
            }
        delete iTempRecordedAudio;
        iTempRecordedAudio = NULL;
        }

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::~CVeiEditVideoView: iTempMaker" );
        
    if ( iTempMaker )
        {
        delete iTempMaker;
        iTempMaker = NULL;
        }

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::~CVeiEditVideoView: iErrorUI" );
    if ( iErrorUI )
        {
        delete iErrorUI;
        iErrorUI = NULL;
        }

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::~CVeiEditVideoView: iAudioClipInfo" );
    delete iAudioClipInfo;

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::~CVeiEditVideoView: iEditLabel" );
    delete iEditLabel;

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::~CVeiEditVideoView: iMoveLabel" );
    delete iMoveLabel;

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::~CVeiEditVideoView: iPreviewLabel" );
    delete iPreviewLabel;

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::~CVeiEditVideoView: iRecorder" );
    if ( iRecorder )
        {
        delete iRecorder;
        iRecorder = NULL;
        }

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::~CVeiEditVideoView: iVolumeHider" );
    if ( iVolumeHider )
        {
        iVolumeHider->Cancel();
        delete iVolumeHider;
        }

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::~CVeiEditVideoView: iVolumeNavi" );
    delete iVolumeNavi;

    delete iCallBack;

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::~CVeiEditVideoView: Out" );
    }

void CVeiEditVideoView::DialogDismissedL( TInt aButtonId )
    {
    LOGFMT2( KVideoEditorLogFile, "CVeiEditVideoView::DialogDismissedL: In, aButtonId:%d, iWaitMode:%d", aButtonId, iWaitMode );

    IsEnoughFreeSpaceToSaveL();

    if ( aButtonId != EAknSoftkeyDone )
        {
        LOG( KVideoEditorLogFile, "CVeiEditVideoView::DialogDismissedL: In, 2" );
        CMdaAudioRecorderUtility::TState rState;
        rState = CMdaAudioClipUtility::ENotReady;

        if ( iRecorder )
            {
            rState = iRecorder->State();
            }

        if (( EOpeningAudioInfo == iWaitMode ) || ( CMdaAudioClipUtility::EOpen == rState ))
            {
            delete iAudioClipInfo;
            iAudioClipInfo = NULL;
            iWaitMode = EProcessingError;
            iMovie->CancelProcessing();
            }
        else
            {
            iWaitMode = EProcessingError;
            iMovie->CancelProcessing();
            }
        }
    else if ( EProcessingMovieSend == iWaitMode )
        {
        //SendMovieL();
        if ( !iCallBack )
            {
            TCallBack cb( CVeiEditVideoView::AsyncBackSend, this );
            iCallBack = new ( ELeave ) CAsyncCallBack( cb, CActive::EPriorityStandard );
            }
        iCallBack->CallBack();
        }
    else if ( EProcessingMovieTrimMms == iWaitMode )
        {
        SetNewTempFileNeeded( EFalse );

        LOG( KVideoEditorLogFile, "CVeiEditVideoView::DialogDismissedL: 3, EProcessingMovieTrimMms" );

        RFs& fs = iEikonEnv->FsSession();
        TEntry entry;
        User::LeaveIfError( fs.Entry( *iTempFile, entry ));
        TInt tempFileSize = entry.iSize / 1024;
        TInt maxMmsSize = STATIC_CAST( CVeiEditVideoLabelNavi*, iEditLabel->DecoratedControl() )->GetMaxMmsSize();

        LOGFMT2( KVideoEditorLogFile, 
                "CVeiEditVideoView::DialogDismissedL: 4, tempFileSize:%d, maxMmsSize:%d", tempFileSize, maxMmsSize );

        if ( iMovie->IsMovieMMSCompatible())
            {
            if ( tempFileSize <= maxMmsSize )
                {
                LOG( KVideoEditorLogFile, "CVeiEditVideoView::DialogDismissedL: 5, MMS SIZE OK -- SEND" );
                iWaitMode = EProcessingMovieSend;
                iGivenSendCommand = KSenduiMtmMmsUid; // MMS

                //SendMovieL();
                if ( !iCallBack )
                    {
                    TCallBack cb( CVeiEditVideoView::AsyncBackSend, this );
                    iCallBack = new( ELeave )CAsyncCallBack( cb, CActive::EPriorityStandard );
                    }
                iCallBack->CallBack();
                }
            else
                {
                LOG( KVideoEditorLogFile, "CVeiEditVideoView::DialogDismissedL: 6, MMS SIZE FAILED - to TRIMM" );
                TBuf8 < 255 > conv8Filename;
                CnvUtfConverter::ConvertFromUnicodeToUtf8( conv8Filename, *iTempFile );
                iMovie->UnregisterMovieObserver( iContainer );
                iMovie->UnregisterMovieObserver( this );

                AppUi()->ActivateLocalViewL( TUid::Uid( EVeiTrimForMmsView ), TUid::Uid(0), conv8Filename );
                }
            }
        else
            {
            // if movie is not mms capable, trimming it does not help, instead its quality should be set
            // trimming is made when other compatibility issues are fullfilled 
            LOG( KVideoEditorLogFile, "CVeiEditVideoView::DialogDismissedL: 7, MMS Compatible FAILED - to TRIMM" );
            TBuf8 < KMaxFileName > conv8Filename;
            CnvUtfConverter::ConvertFromUnicodeToUtf8( conv8Filename, *iTempFile );
                
            iMovie->UnregisterMovieObserver( iContainer );
            iMovie->UnregisterMovieObserver( this );

            AppUi()->ActivateLocalViewL( TUid::Uid( EVeiTrimForMmsView ), TUid::Uid( 0 ), conv8Filename );
            }
        iMovie->SetQuality( iBackupSaveQuality );
        return ;
        }
    else if (( EProcessingMovieSave == iWaitMode ) ||
             ( EProcessingMovieSaveThenQuit == iWaitMode ))
        {
        LOG( KVideoEditorLogFile, "CVeiEditVideoView::DialogDismissedL: 8, EProcessingMovieSave|EProcessingMovieSaveThenQuit" );
        RFs& fs = iEikonEnv->FsSession();

        CFileMan* fileman = CFileMan::NewL( fs );
        CleanupStack::PushL( fileman );

        TInt moveErr( KErrNone );

        if ( iTempFile->Left( 1 ) == iSaveToFileName->Left( 1 ))
            {
            moveErr = fileman->Rename( *iTempFile, * iSaveToFileName );
            LOGFMT2( KVideoEditorLogFile, "CVeiEditVideoView::DialogDismissedL: 9, rename %S to %S", iTempFile, iSaveToFileName );
            }
        else
            {
            moveErr = fileman->Move( *iTempFile, * iSaveToFileName );
            LOGFMT2( KVideoEditorLogFile, "CVeiEditVideoView::DialogDismissedL: 10, moved %S to %S", iTempFile, iSaveToFileName );
            }
        CleanupStack::PopAndDestroy( fileman );

        delete iTempFile;
        iTempFile = NULL;

        if ( moveErr )
            {
            ShowGlobalErrorNote( moveErr );
            UpdateEditNaviLabel();
            iWaitMode = ENotWaiting;
            return ;
            }

        iMovieSavedFlag = ETrue;

        if ( EProcessingMovieSaveThenQuit == iWaitMode )
            {
            iMovie->Reset();
            iMovieFirstAddFlag = ETrue; // True for the next edit process.						
            }

        LOG( KVideoEditorLogFile, "CVeiEditVideoView::DialogDismissedL: 11" );
        UpdateMediaGalleryL();
        LOG( KVideoEditorLogFile, "CVeiEditVideoView::DialogDismissedL: 12, media gallery updated" );

        if ( EProcessingMovieSaveThenQuit == iWaitMode )
            {
            if ( !iCallBack )
                {
                TCallBack cb( CVeiEditVideoView::AsyncBackSaveThenExitL, this );
                iCallBack = new( ELeave )CAsyncCallBack( cb, CActive::EPriorityStandard );
                                
                }
            iCallBack->CallBack();
            }

        LOG( KVideoEditorLogFile, "CVeiEditVideoView::DialogDismissedL Out" );
        return ;
        }
    else if ( EProcessingMoviePreview == iWaitMode && ( iTempFile != NULL ))
        {
        iContainer->SetSelectionMode( CVeiEditVideoContainer::EModePreview );
        iContainer->PlayVideoFileL( *iTempFile, iFullScreenSelected );
        }
    else
        {
        if ( EProcessingError == iWaitMode )
            {
            LOG( KVideoEditorLogFile, "CVeiEditVideoView::DialogDismissedL: 13" );
            if ( iTempFile )
                {
                RFs& fs = iEikonEnv->FsSession();

                fs.Delete( *iTempFile );
                delete iTempFile;
                iTempFile = NULL;
                SetNewTempFileNeeded( ETrue );
                }

            iWaitMode = ENotWaiting;
            ShowGlobalErrorNote( iErrorNmb );
            LOG( KVideoEditorLogFile, "CVeiEditVideoView::DialogDismissedL: 14" );
            UpdateEditNaviLabel();
            }

        if ( EProcessingAudioError == iWaitMode )
            {
            LOG( KVideoEditorLogFile, "CVeiEditVideoView::DialogDismissedL: 15" );
            ShowErrorNote( R_VEI_ERRORNOTE_AUDIO_INSERTING_FAILED );
            LOG( KVideoEditorLogFile, "CVeiEditVideoView::DialogDismissedL: 16" );
            UpdateEditNaviLabel();
            iWaitMode = ENotWaiting;
            }

        if ( iChangedFromMMCToPhoneMemory )
            {
            HBufC* noteText = StringLoader::LoadLC( R_VED_MMC_NOT_INSERTED,
                                                        iEikonEnv );
            CAknInformationNote* informationNote = new( ELeave )
                                                    CAknInformationNote( ETrue );
            informationNote->ExecuteLD( *noteText );

            CleanupStack::PopAndDestroy( noteText );
            }
        }
    if ( EProcessingMovieSend != iWaitMode )
        {
        iWaitMode = ENotWaiting;
        }
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::DialogDismissedL: Out" );
    }


TInt CVeiEditVideoView::AsyncBackSend( TAny* aThis )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::AsyncBackSend" );

    // In the asynchronous version, trap the rest of the functions 
    // to make sure that the caller's TRequestStatus is always 
    // completed, also in case of failures.
    CVeiEditVideoView* view = static_cast < CVeiEditVideoView*  > ( aThis );
    TInt err = KErrNone;
    TRAP( err, view->SendMovieL());
    LOGFMT( KVideoEditorLogFile, "CVeiEditVideoView::AsyncBackSend 1, err:%d", err );
           
    view->iWaitMode = ENotWaiting;
    User::LeaveIfError( err );
    return KErrNone;
    }

TInt CVeiEditVideoView::AsyncBackSaveThenExitL( TAny* aThis )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::AsyncBackSaveThenExitL In" );

    // In the asynchronous version, trap the rest of the functions 
    // to make sure that the caller's TRequestStatus is always 
    // completed, also in case of failures.

    CVeiEditVideoView* view = static_cast < CVeiEditVideoView*  > ( aThis );

    view->AppUi()->HandleCommandL( EAknCmdExit );
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::AsyncBackSaveThenExitL 1" );
    return KErrNone;
    }

void CVeiEditVideoView::DynInitMenuPaneL( TInt aResourceId, CEikMenuPane*
    aMenuPane )
    {
    if ( !iContainer || !iMovie )
        {
        return ;
        }
    if ( aResourceId == R_VEI_EDIT_VIDEO_VIEW_MENU && CVeiEditVideoContainer
        ::EModeMixingAudio == iContainer->SelectionMode())
        {
        // Dim all the items and replace the with 
        // R_VEI_EDIT_VIDEO_VIEW_AUDIO_MIXING_MENU
        aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewInsert, ETrue );
        aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditVideo, ETrue );
        aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditAudio, ETrue );
        aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditText, ETrue );
        aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditImage, ETrue );
        aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewMovie, ETrue );
        aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewSettings, ETrue );
        aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditStartTransition, ETrue );
        aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditMiddleTransition, ETrue );
        aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditEndTransition, ETrue );
        aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewHelp, ETrue );
        aMenuPane->SetItemDimmed( EEikCmdExit, ETrue );

        aMenuPane->AddMenuItemsL( R_VEI_EDIT_VIDEO_VIEW_AUDIO_MIXING_MENU );

        return ;
        }

    if (( aResourceId == R_VEI_EDIT_VIDEO_VIEW_EDIT_VIDEO_MENU ) || 
        ( aResourceId == R_VEI_EDIT_VIDEO_VIEW_EDIT_VIDEO_SUBMENU ))
        {
        // @ : if muted, remove AdjustVolume, if video has no audio, remove AdjustVolume
        //if (!iMovie->VideoClipEditedHasAudio(iContainer->CurrentIndex()))				
        if ( !( iMovie->VideoClipInfo( iContainer->CurrentIndex()))->HasAudio())
            {
            aMenuPane->DeleteMenuItem( EVeiCmdEditVideoAdjustVolume );
            }

        if ( iMovie->VideoClipCount() < 2 )
            {
            aMenuPane->DeleteMenuItem( EVeiCmdEditVideoViewEditVideoMove );
            }

        if ( iMovie->VideoClipIsMuteable( iContainer->CurrentIndex()) == EFalse
            )
            {
            aMenuPane->DeleteMenuItem( EVeiCmdEditVideoViewEditVideoMute );
            aMenuPane->DeleteMenuItem( EVeiCmdEditVideoViewEditVideoUnmute );
            }
        else if ( iMovie->VideoClipIsMuted( iContainer->CurrentIndex()))
            {
            aMenuPane->DeleteMenuItem( EVeiCmdEditVideoViewEditVideoMute );
            }
        else
            {
            aMenuPane->DeleteMenuItem( EVeiCmdEditVideoViewEditVideoUnmute );
            }
        // remove "cut" if video duration < 1 sec. because engine/codec(s) do not handle them wholly at the moment
        TTimeIntervalMicroSeconds duration = iMovie->VideoClipInfo( 
                                        iContainer->CurrentIndex())->Duration();
        if ( duration.Int64() < KMinCutVideoLength )
            {
            aMenuPane->DeleteMenuItem( EVeiCmdEditVideoViewEditVideoCutting );
            }
        }

    if ( aResourceId == R_VEI_EDIT_VIDEO_VIEW_MOVIE_SUBMENU )
        {
        if ( STATIC_CAST( CVeiEditVideoLabelNavi* , 
                          iEditLabel->DecoratedControl())->IsMMSAvailable())
            {
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewTrimForMms, ETrue );
            }

        /*TInt tempint;
        if (aMenuPane->MenuItemExists(EVeiCmdEditVideoMixAudio, tempint))
        {
        if ((iMovie->VideoClipCount() > 0 && 
        (iMovie->VideoClipEditedHasAudio(0) && 
        iMovie->VideoClipEditedHasAudio(iMovie->VideoClipCount() - 1)))
        && iMovie->AudioClipCount() > 0)*/
        if ( MixingConditionsOk())
            {
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoMixAudio, EFalse );
            }
        else
            {
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoMixAudio, ETrue );
            }
        //}	
        }

    if ( aResourceId == R_VEI_EDIT_VIDEO_VIEW_MENU )
        {
        if ( iSendKey )
        //Display send menu. 
            {
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewInsert, ETrue );
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditVideo, ETrue );
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditAudio, ETrue );
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditText, ETrue );
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditImage, ETrue );
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewMovie, ETrue );
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewSettings, ETrue );
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditStartTransition, ETrue );
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditMiddleTransition, ETrue );
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditEndTransition, ETrue );
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewHelp, ETrue );
            aMenuPane->SetItemDimmed( EEikCmdExit, ETrue );

            ShowAndHandleSendMenuCommandsL();

            iSendKey = EFalse;
            return ;
            }

        if ( iMovie->VideoClipCount() == 0 )
            {
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditVideo, ETrue );
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditText, ETrue );
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditImage, ETrue );
            }


        if ( iMovie->VideoClipCount() == 0 && iMovie->AudioClipCount() == 0 )
            {
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditStartTransition, ETrue );
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditMiddleTransition, ETrue );
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditEndTransition, ETrue );

            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewMovie, ETrue );
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditAudio, ETrue );
            return ;
            }
        else
            {
            TInt index = 0;

            aMenuPane->ItemAndPos( EVeiCmdEditVideoViewHelp, index );
            iSendAppUi.AddSendMenuItemL( *aMenuPane, index, EVeiCmdEditVideoViewSend );
            }
        /* Remove irrelevant "edit" menus. */

        if ( iContainer->CursorLocation() != VideoEditor::ECursorOnClip )
            {
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditText, ETrue );
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditVideo, ETrue );
            }
        /* Checks if title or image */
        if (( iContainer->CursorLocation() == VideoEditor::ECursorOnClip ) && 
            ( iMovie->VideoClipCount() > 0 ) && 
            ( iMovie->VideoClipInfo( iContainer->CurrentIndex())->Class() 
                                                == EVedVideoClipClassGenerated
            ))
            {
            /* Now refine the menu dimming to specific generators. */
            TUid generatorUid = iMovie->VideoClipInfo( 
                                iContainer->CurrentIndex())->Generator()->Uid();

            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditVideo, ETrue );
            if ( generatorUid == KUidImageClipGenerator )
                {
                aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditText, ETrue );
                }
            else
                {
                aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditImage, ETrue )
                    ;
                }
            }
        else
            {
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditImage, ETrue );
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditText, ETrue );
            }


        if ( iContainer->CurrentClipIsFile())
            {
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditText, ETrue );
            }
        else
            {
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditVideo, ETrue );
            }

        if ( iContainer->CursorLocation() != VideoEditor::ECursorOnTransition )
            {
            aMenuPane->DeleteMenuItem( EVeiCmdEditVideoViewEditStartTransition )
                                      ;
            aMenuPane->DeleteMenuItem( EVeiCmdEditVideoViewEditEndTransition );
            aMenuPane->DeleteMenuItem( EVeiCmdEditVideoViewEditMiddleTransition
                                      );
            }
        if (( iContainer->CursorLocation() != VideoEditor::ECursorOnAudio ) || 
            ( iMovie->AudioClipCount() == 0 ))
            {
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditAudio, ETrue );
            }

        /* Remove irrelevant transition effect menus. */
        if ( iContainer->CursorLocation() == VideoEditor::ECursorOnAudio )
            {
            aMenuPane->SetItemDimmed( EVeiCmdEditVideoViewEditText, ETrue );
            }

        if ( iContainer->CursorLocation() == VideoEditor::ECursorOnTransition )
            {
            if ( iContainer->CurrentIndex() == 0 )
                {
                aMenuPane->DeleteMenuItem(
                    EVeiCmdEditVideoViewEditEndTransition );
                aMenuPane->DeleteMenuItem(
                    EVeiCmdEditVideoViewEditMiddleTransition );
                }
            else if ( iContainer->CurrentIndex() < iMovie->VideoClipCount())
                {
                aMenuPane->DeleteMenuItem(
                    EVeiCmdEditVideoViewEditStartTransition );
                aMenuPane->DeleteMenuItem(
                    EVeiCmdEditVideoViewEditEndTransition );
                }
            else
                {
                aMenuPane->DeleteMenuItem(
                    EVeiCmdEditVideoViewEditStartTransition );
                aMenuPane->DeleteMenuItem(
                    EVeiCmdEditVideoViewEditMiddleTransition );
                }
            }
        }

    if ( aResourceId == R_VEI_EDIT_VIDEO_VIEW_EDIT_START_TRANSITION_SUBMENU )
        {
        switch ( iMovie->StartTransitionEffect())
            {
            case EVedStartTransitionEffectNone:
                aMenuPane->DeleteMenuItem( EVeiCmdEditVideoViewTransitionNone );
                break;
            case EVedStartTransitionEffectFadeFromBlack:
                aMenuPane->DeleteMenuItem(
                    EVeiCmdEditVideoViewTransitionFadeFromBlack );
                break;
            case EVedStartTransitionEffectFadeFromWhite:
                aMenuPane->DeleteMenuItem(
                    EVeiCmdEditVideoViewTransitionFadeFromWhite );
                break;
            default:
                break;
            }
        }

    if ( aResourceId == R_VEI_EDIT_VIDEO_VIEW_EDIT_END_TRANSITION_SUBMENU )
        {
        switch ( iMovie->EndTransitionEffect())
            {
            case EVedEndTransitionEffectNone:
                aMenuPane->DeleteMenuItem( EVeiCmdEditVideoViewTransitionNone );
                break;
            case EVedEndTransitionEffectFadeToBlack:
                aMenuPane->DeleteMenuItem(
                    EVeiCmdEditVideoViewTransitionFadeToBlack );
                break;
            case EVedEndTransitionEffectFadeToWhite:
                aMenuPane->DeleteMenuItem(
                    EVeiCmdEditVideoViewTransitionFadeToWhite );
                break;
            default:
                break;
            }
        }

    if ( aResourceId == R_VEI_EDIT_VIDEO_VIEW_EDIT_MIDDLE_TRANSITION_SUBMENU )
        {
        TInt currentindex = iContainer->CurrentIndex() - 1;
        switch ( iMovie->MiddleTransitionEffect( currentindex ))
            {
            case EVedMiddleTransitionEffectNone:
                aMenuPane->DeleteMenuItem( EVeiCmdEditVideoViewTransitionNone );
                break;
            case EVedMiddleTransitionEffectDipToBlack:
                aMenuPane->DeleteMenuItem(
                    EVeiCmdEditVideoViewTransitionDipToBlack );
                break;
            case EVedMiddleTransitionEffectDipToWhite:
                aMenuPane->DeleteMenuItem(
                    EVeiCmdEditVideoViewTransitionDipToWhite );
                break;
            case EVedMiddleTransitionEffectCrossfade:
                aMenuPane->DeleteMenuItem(
                    EVeiCmdEditVideoViewTransitionCrossfade );
                break;
            case EVedMiddleTransitionEffectWipeLeftToRight:
                aMenuPane->DeleteMenuItem(
                    EVeiCmdEditVideoViewTransitionWipeFromLeft );
                break;
            case EVedMiddleTransitionEffectWipeRightToLeft:
                aMenuPane->DeleteMenuItem(
                    EVeiCmdEditVideoViewTransitionWipeFromRight );
                break;
            case EVedMiddleTransitionEffectWipeTopToBottom:
                aMenuPane->DeleteMenuItem(
                    EVeiCmdEditVideoViewTransitionWipeFromTop );
                break;
            case EVedMiddleTransitionEffectWipeBottomToTop:
                aMenuPane->DeleteMenuItem(
                    EVeiCmdEditVideoViewTransitionWipeFromBottom );
                break;
            default:
                break;
            }
        }
    }

void CVeiEditVideoView::HandleCommandL( TInt aCommand )
    {
    LOGFMT( KVideoEditorLogFile, "CVeiEditVideoView::HandleCommandL( %d ): In", aCommand );

    // At the startup HandleCommandL may get called before iContainer
    // has been constructed.
    if ( !iContainer )
        {
        LOG( KVideoEditorLogFile, "\tiContainer == NULL" );
        AppUi()->HandleCommandL( aCommand );
        return ;
        }

    TInt index;
    switch ( aCommand )
        {
        /* Cursor is on transition and up/key key is pressed*/
        case EVeiCmdEditVideoViewTransitionKeyUp:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewTransitionKeyUp" );
                BrowseStartTransition( ETrue );
                break;
                }
        case EVeiCmdEditVideoViewTransitionKeyDown:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewTransitionKeyDown" );
                BrowseStartTransition( EFalse );
                break;
                }
        case EVeiCmdSendMovie:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdSendMovie" );
                if (( iMovie->VideoClipCount() == 0 ) && 
                    ( iMovie->AudioClipCount() == 0 ))
                    {
                    iSendKey = EFalse;
                    }
                else
                    {
                    MenuBar()->TryDisplayMenuBarL();
                    }
                break;
                }
        case EAknSoftkeyDone:
                {
                LOG( KVideoEditorLogFile, "\tEAknSoftkeyDone" );

                /*if (CVeiEditVideoContainer::EModeMixingAudio == iContainer->SelectionMode())
                {
                Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_OPTIONS_BACK );
                Cba()->DrawDeferred();
                iContainer->SetSelectionMode( CVeiEditVideoContainer::EModeNavigation );
                //iContainer->ArrowsControl();
                break;	
                }	
                 */

                if ( CVeiEditVideoContainer::EModeMixingAudio == iContainer->SelectionMode())
                    
                    {

                    MixAudio();

                    Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_OPTIONS_BACK );
                    Cba()->DrawDeferred();

                    VideoEditor::TCursorLocation cursorLocation = iContainer->CursorLocation();
                    if (( cursorLocation == VideoEditor::ECursorOnClip ) && 
                        ( iContainer->SelectionMode() == CVeiEditVideoContainer::EModeSlowMotion ))
                        
                        {
                        iContainer->SetSelectionMode( CVeiEditVideoContainer::EModeNavigation );
                        iContainer->ArrowsControl();
                        }
                    else
                        {
                        iOriginalAudioClipIndex =  - 1;
                        iOriginalVideoClipIndex =  - 1;
                        iOriginalAudioStartPoint = TTimeIntervalMicroSeconds( -1 );
                        iOriginalAudioDuration = TTimeIntervalMicroSeconds(  -1 );
                            

                        iContainer->SetSelectionMode( CVeiEditVideoContainer::EModeNavigation );
                        }
                    SetEditorState( EEdit );
                    UpdateEditNaviLabel();

                    // Setting the cursor location resets the image in the video display box
                    iContainer->SetCursorLocation( cursorLocation );
                    }
                break;
                }

        case EAknSoftkeyOk:
                {
                LOG( KVideoEditorLogFile, "\tEAknSoftkeyOk" );

                if ( CVeiEditVideoContainer::EModeAdjustVolume == iContainer->SelectionMode())
                    
                    {
                    AdjustVolumeL();
                    SetEditorState( EEdit );
                    }

                Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_OPTIONS_BACK );
                Cba()->DrawDeferred();

                if (( iContainer->CursorLocation() == VideoEditor::ECursorOnClip ) &&
                    ( iContainer->SelectionMode() == CVeiEditVideoContainer::EModeSlowMotion ))
                    
                    {
                    iContainer->SetSelectionMode( CVeiEditVideoContainer::EModeNavigation );
                    iContainer->ArrowsControl();
                    }
                else
                    {
                    iOriginalAudioClipIndex =  - 1;
                    iOriginalVideoClipIndex =  - 1;
                    iOriginalAudioStartPoint = TTimeIntervalMicroSeconds(  -1 );
                    iOriginalAudioDuration = TTimeIntervalMicroSeconds(  -1 );

                    iContainer->SetSelectionMode( CVeiEditVideoContainer::EModeNavigation );
                    }
                UpdateEditNaviLabel();
                break;
                }
        case EAknSoftkeyCancel:
        case EVeiCmdEditVideoMixAudioCancel:
                {
                LOG( KVideoEditorLogFile, "\tEAknSoftkeyCancel||EVeiCmdEditVideoMixAudioCancel" );

                Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_OPTIONS_BACK );
                Cba()->DrawDeferred();

                SetEditorState( EEdit );

                if ( CVeiEditVideoContainer::EModeMixingAudio == iContainer->SelectionMode() ||
                     CVeiEditVideoContainer::EModeAdjustVolume == iContainer->SelectionMode())
                    {
                    iContainer->SetSelectionMode( CVeiEditVideoContainer::EModeNavigation );
                    // Setting the cursor location resets the image in the video display box
                    iContainer->SetCursorLocation( iContainer->CursorLocation());
                    break;
                    }

                if (( iContainer->CursorLocation() == VideoEditor::ECursorOnClip ) &&
                     ( iContainer->SelectionMode() == CVeiEditVideoContainer::EModeSlowMotion ))
                    {
                    iMovie->VideoClipSetSpeed( iContainer->CurrentIndex(), iOriginalVideoSpeed );
                    iContainer->SetSelectionMode( CVeiEditVideoContainer::EModeNavigation );
                        
                    iContainer->ArrowsControl();
                    }
                else
                    {
                    if ( iContainer->CursorLocation() == VideoEditor
                        ::ECursorOnAudio )
                        {
                        if ( iOriginalAudioStartPoint >= TTimeIntervalMicroSeconds( 0 ))
                            {
                            iMovie->AudioClipSetStartTime( iContainer->CurrentIndex(), iOriginalAudioStartPoint );
                            iOriginalAudioStartPoint = TTimeIntervalMicroSeconds(  -1 );
                            iMovie->AudioClipSetCutOutTime( iContainer->CurrentIndex(), iOriginalAudioDuration );
                            iOriginalAudioDuration = TTimeIntervalMicroSeconds( -1 );
                            }
                        else
                            {
                            index = iContainer->CurrentIndex();

                            iContainer->SetCurrentIndex( iOriginalAudioClipIndex );
                            iOriginalAudioClipIndex =  - 1;
                            iOriginalAudioDuration = TTimeIntervalMicroSeconds( -1 );

                            iMovie->RemoveAudioClip( index );
                            }
                        }
                    else
                        {
                        if ( iContainer->SelectionMode() != CVeiEditVideoContainer::EModeDuration )
                            {
                            iMovie->VideoClipSetIndex( iContainer->CurrentIndex(), iOriginalVideoClipIndex );
                            iContainer->SetVideoCursorPosition( iOriginalVideoClipIndex );
                            iOriginalVideoClipIndex =  - 1;
                            }
                        else
                            {
                            CVedVideoClipInfo* info = iMovie->VideoClipInfo( iContainer->CurrentIndex());
                            if ( info->Class() == EVedVideoClipClassGenerated )
                                {
                                if ( info->Generator()->Uid() == KUidTitleClipGenerator )
                                    {
                                    CVeiTitleClipGenerator* generator =
                                                    STATIC_CAST( CVeiTitleClipGenerator* , info->Generator());
                                    generator->SetDuration( TTimeIntervalMicroSeconds(
                                                                iOriginalVideoCutOutTime.Int64() -
                                                                iOriginalVideoCutInTime.Int64()));
                                    }
                                else if ( info->Generator()->Uid() == KUidImageClipGenerator )
                                    {
                                    CVeiImageClipGenerator* generator =
                                            STATIC_CAST( CVeiImageClipGenerator* , info->Generator());
                                    generator->SetDuration( TTimeIntervalMicroSeconds(
                                                                iOriginalVideoCutOutTime.Int64() -
                                                                iOriginalVideoCutInTime.Int64()));
                                    }
                                }
                            }
                        }
                    }

                if ( iContainer->SelectionMode() == CVeiEditVideoContainer
                    ::EModePreview )
                    {
                    iContainer->SetBlackScreen( EFalse );
                    iContainer->SetRect( AppUi()->ClientRect());
                    iContainer->SetSelectionMode( CVeiEditVideoContainer::EModeNavigation );
                    iContainer->ArrowsControl();
                    }
                else
                    {
                    iContainer->SetSelectionMode( CVeiEditVideoContainer::EModeNavigation );
                    UpdateEditNaviLabel();
                    }
                break;
                }
        case EVeiCmdEditVideoViewSend:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewSend" );
                ShowAndHandleSendMenuCommandsL();
                break;
                }

        case EVeiCmdEditVideoViewPreviewLarge:
        case EVeiCmdEditVideoViewPreviewSmall:
                {
                LOG( KVideoEditorLogFile, 
                    "\tEVeiCmdEditVideoViewPreviewLarge||EVeiCmdEditVideoViewPreviewSmall" );

                if ( CVeiEditVideoContainer::EModeMixingAudio == iContainer->SelectionMode())
                    {
                    MixAudio();
                    }

                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    if ( aCommand == EVeiCmdEditVideoViewPreviewLarge )
                        {
                        iFullScreenSelected = ETrue;
                        }
                    else
                        {
                        iFullScreenSelected = EFalse;
                        }
                    iWaitMode = EProcessingMoviePreview;
                    StartTempFileProcessingL();
                    }
                break;
                }

            /**
             * Trim for MMS
             */
        case EVeiCmdEditVideoViewTrimForMms:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewTrimForMms" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    iWaitMode = EProcessingMovieTrimMms;
                    StartTempFileProcessingL();
                    }
                break;
                }
            /**
             * Cut (Audio and Video)
             */
        case EVeiCmdEditVideoViewEditVideoCutting:
                {
                LOG( KVideoEditorLogFile, 
                    "\tEVeiCmdEditVideoViewEditVideoCutting" );

                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    // Cut video
                    if ( iContainer->CursorLocation() == VideoEditor::ECursorOnClip )
                        {
                        __ASSERT_ALWAYS( iMovie->VideoClipCount() > 0, 
                                         User::Panic( _L( "VideoEditor" ), 34 ));

                        iCutVideoIndex = iContainer->CurrentIndex();
                        iWaitMode = EProcessingMovieForCutting;
                        iOriginalCutInTime = iMovie->VideoClipCutInTime( iCutVideoIndex );
                        iOriginalCutOutTime = iMovie->VideoClipCutOutTime( iCutVideoIndex );

                        iMovie->UnregisterMovieObserver( iContainer );
                        iMovie->UnregisterMovieObserver( this );

                        AppUi()->DeactivateActiveViewL();

                        // set file name & clip
                        iCutView.SetVideoClipAndIndex( *iMovie, iCutVideoIndex )
                            ;

                        // activate cut  view
                        AppUi()->ActivateLocalViewL( iCutView.Id());
                        }
                    else if ( iContainer->CursorLocation() == VideoEditor::ECursorOnAudio )
                    // Cut audio
                        {
                        __ASSERT_ALWAYS( iMovie->AudioClipCount() > 0, 
                                         User::Panic( _L( "VideoEditor" ), 34 ));

                        iCutAudioIndex = iContainer->CurrentIndex();
                        iWaitMode = ECuttingAudio;
                        iOriginalAudioCutInTime = iMovie->AudioClipCutInTime( iCutAudioIndex );
                        iOriginalAudioCutOutTime = iMovie->AudioClipCutOutTime( iCutAudioIndex );

                        iMovie->UnregisterMovieObserver( iContainer );
                        iMovie->UnregisterMovieObserver( this );

                        AppUi()->DeactivateActiveViewL();

                        // set file name & clip
                        iCutAudioView.SetVideoClipAndIndex( *iMovie, iCutAudioIndex );
                        // activate cut  view
                        AppUi()->ActivateLocalViewL( TUid::Uid( EVeiCutAudioView ));
                        }
                    }
                break;
                }
            /**
             * Selection (joystick).
             */
        case EVeiCmdEditVideoViewContainerShowMenu:
                {
                LOG( KVideoEditorLogFile, 
                    "\tEVeiCmdEditVideoViewContainerShowMenu" );

                //preview popup
                if ( iEditorState != EEdit )
                    {
                    if ( iEditorState == CVeiEditVideoView::EQuickPreview )
                        {
                        StopNaviPaneUpdateL();
                        iContainer->PauseVideoL();
                        LOG( KVideoEditorLogFile, 
                            "\tEVeiCmdEditVideoViewContainerShowMenu, setting R_VEI_SOFTKEYS_PREVIEW_PLAY_BACK" );
                        Cba()->SetCommandSetL( R_VEI_SOFTKEYS_PREVIEW_PLAY_BACK
                            );
                        Cba()->DrawDeferred();
                        }
                    MenuBar()->SetMenuTitleResourceId( R_VEI_PREVIEW_BAR );
                    MenuBar()->TryDisplayMenuBarL();
                    MenuBar()->SetMenuTitleResourceId(
                            R_VEI_MENUBAR_EDIT_VIDEO_VIEW );
                    break;
                    }
                switch ( iContainer->CursorLocation())
                    {
                    case VideoEditor::ECursorOnAudio: 
                        {
                        iPopup->ShowEditAudioPopupList();
                        break;
                        }
                    case VideoEditor::ECursorOnClip: 
                        {
                        if ( iMovie->VideoClipInfo( iContainer->CurrentIndex())->Class() == EVedVideoClipClassFile )
                            {
                            iPopup->ShowEditVideoPopupList();
                            }
                        else
                            {

                            TUid generatorUid = iMovie->VideoClipInfo(
                                                    iContainer->CurrentIndex())->Generator()->Uid();
                            if ( generatorUid == KUidImageClipGenerator )
                            // Image 
                                {
                                iPopup->ShowEditImagePopupList();
                                }
                            else
                            // Text
                                {
                                iPopup->ShowEditTextPopupList();
                                }
                            }
                        break;
                        }
                    /**
                     * Cursor on video transition.
                     */
                    case VideoEditor::ECursorOnTransition: 
                        {
                        if ( iContainer->CurrentIndex() == 0 )
                            {
                            iPopup->ShowStartTransitionPopupListL();
                            }
                        else if ( iContainer->CurrentIndex() < iMovie->VideoClipCount())
                            {
                            iPopup->ShowMiddleTransitionPopupListL();
                            }
                        else
                            {
                            iPopup->ShowEndTransitionPopupListL();
                            }
                        break;
                        }
                default:
                    break;
                    }
                break;
                }
        case EVeiCmdEditVideoViewInsert:
        case EVeiCmdEditVideoViewEditVideo:
        case EVeiCmdEditVideoViewEditAudio:
            LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewInsert||EVeiCmdEditVideoViewEditVideo||EVeiCmdEditVideoViewEditAudio" );
            break;
        case EVeiCmdEditVideoViewTransitionNone:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewTransitionNone" );

                if ( iContainer->CurrentIndex() == 0 )
                    {
                    iMovie->SetStartTransitionEffect( EVedStartTransitionEffectNone );
                    }
                else if ( iContainer->CurrentIndex() < iMovie->VideoClipCount())
                    {
                    index = iContainer->CurrentIndex() - 1;
                    iMovie->SetMiddleTransitionEffect( EVedMiddleTransitionEffectNone, index );
                    }
                else
                    {
                    iMovie->SetEndTransitionEffect( EVedEndTransitionEffectNone );
                    }
                break;
                }
        case EVeiCmdEditVideoViewTransitionFadeFromBlack:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewTransitionFadeFromBlack" );
                __ASSERT_ALWAYS( iContainer->CurrentIndex() == 0, User::Panic( _L( "VideoEditor" ), 34 ));
                iMovie->SetStartTransitionEffect( EVedStartTransitionEffectFadeFromBlack );
                break;
                }
        case EVeiCmdEditVideoViewTransitionFadeFromWhite:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewTransitionFadeFromWhite" );
                __ASSERT_ALWAYS( iContainer->CurrentIndex() == 0, User::Panic( _L( "VideoEditor" ), 34 ));
                iMovie->SetStartTransitionEffect( EVedStartTransitionEffectFadeFromWhite );
                break;
                }
        case EVeiCmdEditVideoViewTransitionDipToBlack:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewTransitionDipToBlack" );
                __ASSERT_ALWAYS( iContainer->CurrentIndex() < iMovie->VideoClipCount(), User::Panic( _L( "VideoEditor" ), 34 ));
                __ASSERT_ALWAYS( iContainer->CurrentIndex() > 0, User::Panic( _L( "VideoEditor" ), 34 ));
                index = iContainer->CurrentIndex() - 1;
                iMovie->SetMiddleTransitionEffect( EVedMiddleTransitionEffectDipToBlack, index );
                break;
                }
        case EVeiCmdEditVideoViewTransitionDipToWhite:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewTransitionDipToWhite" );
                __ASSERT_ALWAYS( iContainer->CurrentIndex() < iMovie->VideoClipCount(), User::Panic( _L( "VideoEditor" ), 34 ));
                __ASSERT_ALWAYS( iContainer->CurrentIndex() > 0, User::Panic( _L( "VideoEditor" ), 34 ));
                index = iContainer->CurrentIndex() - 1;
                iMovie->SetMiddleTransitionEffect( EVedMiddleTransitionEffectDipToWhite, index );
                break;
                }
        case EVeiCmdEditVideoViewTransitionCrossfade:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewTransitionCrossfade" );
                __ASSERT_ALWAYS( iContainer->CurrentIndex() < iMovie->VideoClipCount(), User::Panic( _L( "VideoEditor" ), 34 ));
                __ASSERT_ALWAYS( iContainer->CurrentIndex() > 0, User::Panic( _L( "VideoEditor" ), 34 ));
                index = iContainer->CurrentIndex() - 1;
                iMovie->SetMiddleTransitionEffect( EVedMiddleTransitionEffectCrossfade, index );
                break;
                }
        case EVeiCmdEditVideoViewTransitionWipeFromLeft:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewTransitionWipeFromLeft" );
                __ASSERT_ALWAYS( iContainer->CurrentIndex() < iMovie->VideoClipCount(), User::Panic( _L( "VideoEditor" ), 34 ));
                __ASSERT_ALWAYS( iContainer->CurrentIndex() > 0, User::Panic( _L( "VideoEditor" ), 34 ));
                index = iContainer->CurrentIndex() - 1;
                iMovie->SetMiddleTransitionEffect( EVedMiddleTransitionEffectWipeLeftToRight, index );
                break;
                }
        case EVeiCmdEditVideoViewTransitionWipeFromRight:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewTransitionWipeFromRight" );
                __ASSERT_ALWAYS( iContainer->CurrentIndex() < iMovie->VideoClipCount(), User::Panic( _L( "VideoEditor" ), 34 ));
                __ASSERT_ALWAYS( iContainer->CurrentIndex() > 0, User::Panic( _L( "VideoEditor" ), 34 ));
                index = iContainer->CurrentIndex() - 1;
                iMovie->SetMiddleTransitionEffect( EVedMiddleTransitionEffectWipeRightToLeft, index );
                break;
                }
        case EVeiCmdEditVideoViewTransitionWipeFromTop:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewTransitionWipeFromTop" );
                __ASSERT_ALWAYS( iContainer->CurrentIndex() < iMovie->VideoClipCount(), User::Panic( _L( "VideoEditor" ), 34 ));
                __ASSERT_ALWAYS( iContainer->CurrentIndex() > 0, User::Panic( _L( "VideoEditor" ), 34 ));
                index = iContainer->CurrentIndex() - 1;
                iMovie->SetMiddleTransitionEffect( EVedMiddleTransitionEffectWipeTopToBottom, index );
                break;
                }
        case EVeiCmdEditVideoViewTransitionWipeFromBottom:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewTransitionWipeFromBottom" );
                __ASSERT_ALWAYS( iContainer->CurrentIndex() < iMovie->VideoClipCount(), User::Panic( _L( "VideoEditor" ), 34 ));
                __ASSERT_ALWAYS( iContainer->CurrentIndex() > 0, User::Panic( _L( "VideoEditor" ), 34 ));
                index = iContainer->CurrentIndex() - 1;
                iMovie->SetMiddleTransitionEffect( EVedMiddleTransitionEffectWipeBottomToTop, index );
                break;
                }
        case EVeiCmdEditVideoViewTransitionFadeToBlack:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewTransitionFadeToBlack" );
                __ASSERT_ALWAYS( iContainer->CurrentIndex() == iMovie->VideoClipCount(), User::Panic( _L( "VideoEditor" ), 34 ));
                iMovie->SetEndTransitionEffect( EVedEndTransitionEffectFadeToBlack );
                break;
                }
        case EVeiCmdEditVideoViewTransitionFadeToWhite:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewTransitionFadeToWhite" );
                __ASSERT_ALWAYS( iContainer->CurrentIndex() == iMovie->VideoClipCount(), User::Panic( _L( "VideoEditor" ), 34 ));
                iMovie->SetEndTransitionEffect( EVedEndTransitionEffectFadeToWhite );
                break;
                }
        case EVeiCmdEditVideoViewEditStartTransition:
        case EVeiCmdEditVideoViewEditMiddleTransition:
        case EVeiCmdEditVideoViewEditEndTransition:
            LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewEditStartTransition||EVeiCmdEditVideoViewEditMiddleTransition||EVeiCmdEditVideoViewEditEndTransition" );
            break;
        case EVeiCmdEditVideoViewSaveTo:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewSaveTo" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    TInt err;
                    TRAP( err, SaveL( EProcessingMovieSave ));
                    if ( err != KErrNone )
                        {
                        ShowErrorNote( R_VEI_ERROR_NOTE );
                        }
                    }
                break;
                }
        case EVeiCmdEditVideoMixAudio:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoMixAudio" );
                SetEditorState( EMixAudio );
                break;
                }

        case EVeiCmdEditVideoAdjustVolume:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoAdjustVolume" );
                SetEditorState( EAdjustVolume );
                break;
                }

        case EVeiCmdEditVideoViewInsertVideo:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewInsertVideo" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    __ASSERT_ALWAYS( iMediaQueue, User::Panic( _L( "CVeiEditVideoView" ), 1 ));

                    if ( !iMediaQueue->ShowVideoClipDialogL( iContainer->CursorLocation(), iContainer->CurrentIndex()))
                        {
                        return ;
                        }
                    }
                HandleScreenDeviceChangedL();
                break;
                }
        case EVeiCmdEditVideoDuplicate:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoDuplicate" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    TEntry fileinfo;

                    SetNewTempFileNeeded( ETrue );
                    iMovieSavedFlag = EFalse;

                    iWaitMode = EDuplicating;

                    if ( iContainer->CursorLocation() == VideoEditor::ECursorOnClip )
                        {
                        CVedVideoClipInfo* previousInfo = iMovie->VideoClipInfo( iContainer->CurrentIndex());

                        RFs& fs = iEikonEnv->FsSession();
                        fs.Entry( previousInfo->FileName(), fileinfo );

                        if ( IsEnoughFreeSpaceToSaveL( fileinfo.iSize ))
                            {
                            iMovie->InsertVideoClipL( previousInfo->FileName(), iContainer->CurrentIndex() + 1 );
                            }
                        }
                    else
                        {
                        if ( iContainer->CursorLocation() == VideoEditor::ECursorOnAudio )
                            {
                            CVedAudioClipInfo* currentInfo = iMovie->AudioClipInfo( iContainer->CurrentIndex());
                            TTimeIntervalMicroSeconds currentDuration = iMovie->AudioClipEditedDuration( iContainer->CurrentIndex());

                            TInt currentIndex = iContainer->CurrentIndex();

                            for ( TInt i = iMovie->AudioClipCount() - 1; i > currentIndex; i-- )
                                {
                                TTimeIntervalMicroSeconds oldStartTime = iMovie->AudioClipStartTime( i );
                                iMovie->AudioClipSetStartTime( i, TTimeIntervalMicroSeconds( oldStartTime.Int64() + currentDuration.Int64()));
                                }

                            TTimeIntervalMicroSeconds currentAudioClipEndTime = iMovie->AudioClipEndTime( currentIndex );
                            TTimeIntervalMicroSeconds currentCutInTime = iMovie->AudioClipCutInTime( currentIndex );
                            TTimeIntervalMicroSeconds currentCutOutTime = iMovie->AudioClipCutOutTime( currentIndex );
                            iMovie->AddAudioClipL( currentInfo->FileName(), currentAudioClipEndTime, currentCutInTime, currentCutOutTime );
                            }
                        }
                    }
                break;
                }

            /*
             * Insert TITLE text *	
             */
        case EVeiCmdEditVideoViewInsertText:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewInsertText" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    iPopup->ShowInsertTextPopupList();
                    }
                break;
                }

        case EVeiCmdEditVideoViewInsertTextTitle:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewInsertTextTitle" );

                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    StoreOrientation();
                    CleanupStack::PushL( TCleanupItem( CleanupRestoreOrientation, this ));
                    AppUi()->SetOrientationL( CAknAppUiBase::EAppUiOrientationPortrait );

                    TSize res = TSize( iMovie->Resolution());

                    CVeiTitleClipGenerator* generator = CVeiTitleClipGenerator::NewLC( res, 
                                                                                       EVeiTitleClipTransitionNone, 
                                                                                       EVeiTitleClipHorizontalAlignmentCenter, 
                                                                                       EVeiTitleClipVerticalAlignmentCenter );

                    HBufC* descriptiveName = StringLoader::LoadLC( R_VEI_EDIT_VIEW_TITLE_NAME, iEikonEnv );
                    generator->SetDescriptiveNameL( *descriptiveName );
                    CleanupStack::PopAndDestroy( descriptiveName );

                    generator->SetTransitionAndAlignmentsL( EVeiTitleClipTransitionNone, 
                                                            EVeiTitleClipHorizontalAlignmentCenter, 
                                                            EVeiTitleClipVerticalAlignmentCenter );

                    /* Ask for text. */
                    HBufC* text = HBufC::NewLC( KTitleScreenMaxTextLength );
                    TPtr txtptr( text->Des());
                    CAknTextQueryDialog* textQuery = CAknTextQueryDialog::NewL( txtptr );
                    CleanupStack::PushL( textQuery );

                    textQuery->SetMaxLength( KTitleScreenMaxTextLength );
                    //textQuery->SetPredictiveTextInputPermitted(ETrue);
                    CleanupStack::Pop( textQuery );

                    if ( !textQuery->ExecuteLD( R_VEI_EDITVIDEO_TITLESCREEN_TEXT_QUERY ))
                        {
                        CleanupStack::PopAndDestroy( text );
                        CleanupStack::PopAndDestroy( generator );
                        CleanupStack::PopAndDestroy( this ); // restore appui orientation
                        break;
                        }

                    generator->SetTextL( *text );
                    CleanupStack::PopAndDestroy( text );

                    /* Insert generator into the movie. */
                    TInt err = 0;
                    index = ( iContainer->CurrentIndex() == iMovie->VideoClipCount()) ? iMovie->VideoClipCount(): iContainer->CurrentIndex() + 1;
                    TRAP( err, iMovie->InsertVideoClipL( *generator, ETrue, index ));

                    CleanupStack::Pop( generator );
                    User::LeaveIfError( err );

                    CleanupStack::PopAndDestroy( this ); // restore appui orientation
                    }
                break;
                }
            /*
             * Insert TITLE (fading) text *	
             */
        case EVeiCmdEditVideoViewInsertTextTitleFading:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewInsertTextTitleFading" );

                if ( IsEnoughFreeSpaceToSaveL())
                    {

                    StoreOrientation();
                    CleanupStack::PushL( TCleanupItem( CleanupRestoreOrientation, this ));
                    AppUi()->SetOrientationL( CAknAppUiBase::EAppUiOrientationPortrait );

                    TSize res = TSize( iMovie->Resolution());
                    CVeiTitleClipGenerator* generator = CVeiTitleClipGenerator::NewLC( res, 
                                                                                       EVeiTitleClipTransitionNone, 
                                                                                       EVeiTitleClipHorizontalAlignmentCenter, 
                                                                                       EVeiTitleClipVerticalAlignmentCenter );

                    HBufC* descriptiveName = StringLoader::LoadLC( R_VEI_EDIT_VIEW_TITLE_FADING_NAME, iEikonEnv );
                    generator->SetDescriptiveNameL( *descriptiveName );
                    CleanupStack::PopAndDestroy( descriptiveName );

                    generator->SetTransitionAndAlignmentsL( EVeiTitleClipTransitionFade, 
                                                            EVeiTitleClipHorizontalAlignmentCenter, 
                                                            EVeiTitleClipVerticalAlignmentCenter );
                    /* Ask for text. */
                    HBufC* text = HBufC::NewLC( KTitleScreenMaxTextLength );
                    TPtr txtptr( text->Des());

                    CAknTextQueryDialog* textQuery = CAknTextQueryDialog::NewL( txtptr );
                    CleanupStack::PushL( textQuery );

                    textQuery->SetMaxLength( KTitleScreenMaxTextLength );
                    //textQuery->SetPredictiveTextInputPermitted(ETrue);
                    CleanupStack::Pop( textQuery );

                    if ( !textQuery->ExecuteLD( R_VEI_EDITVIDEO_TITLESCREEN_TEXT_QUERY ))
                        {
                        CleanupStack::PopAndDestroy( text );
                        CleanupStack::PopAndDestroy( generator );
                        CleanupStack::PopAndDestroy( this ); // restore appui orientation
                        break;
                        }

                    generator->SetTextL( *text );
                    CleanupStack::PopAndDestroy( text );

                    /* Insert generator into the movie. */
                    TInt err = 0;
                    index = ( iContainer->CurrentIndex() == iMovie->VideoClipCount()) ? iMovie->VideoClipCount(): iContainer->CurrentIndex() + 1;
                    TRAP( err, iMovie->InsertVideoClipL( *generator, ETrue, index ));
                    CleanupStack::Pop( generator );
                    User::LeaveIfError( err );

                    CleanupStack::PopAndDestroy( this ); // restore appui orientation
                    }
                break;
                }
            /*
             * Insert SUBTITLE text *	
             */
        case EVeiCmdEditVideoViewInsertTextSubTitle:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewInsertTextSubTitle" );

                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    StoreOrientation();
                    CleanupStack::PushL( TCleanupItem( CleanupRestoreOrientation, this ));
                    AppUi()->SetOrientationL( CAknAppUiBase::EAppUiOrientationPortrait );

                    TSize res = TSize( iMovie->Resolution());
                    CVeiTitleClipGenerator* generator = CVeiTitleClipGenerator::NewLC( res, 
                                                                                       EVeiTitleClipTransitionNone, 
                                                                                       EVeiTitleClipHorizontalAlignmentCenter, 
                                                                                       EVeiTitleClipVerticalAlignmentCenter );

                    HBufC* descriptiveName = StringLoader::LoadLC( R_VEI_EDIT_VIEW_SUBTITLE_NAME, iEikonEnv );
                    generator->SetDescriptiveNameL( *descriptiveName );
                    CleanupStack::PopAndDestroy( descriptiveName );

                    generator->SetTransitionAndAlignmentsL( EVeiTitleClipTransitionNone, 
                                                            EVeiTitleClipHorizontalAlignmentLeft, 
                                                            EVeiTitleClipVerticalAlignmentBottom );

                    /* Ask for text. */
                    HBufC* text = HBufC::NewLC( KTitleScreenMaxTextLength );
                    TPtr txtptr( text->Des());
                    CAknTextQueryDialog* textQuery = CAknTextQueryDialog::NewL( txtptr );
                    CleanupStack::PushL( textQuery );

                    textQuery->SetMaxLength( KTitleScreenMaxTextLength );
                    //textQuery->SetPredictiveTextInputPermitted(ETrue);
                    CleanupStack::Pop( textQuery );

                    if ( !textQuery->ExecuteLD( R_VEI_EDITVIDEO_TITLESCREEN_TEXT_QUERY ))
                        {
                        CleanupStack::PopAndDestroy( text );
                        CleanupStack::PopAndDestroy( generator );
                        CleanupStack::PopAndDestroy( this ); // restore appui orientation
                        break;
                        }

                    generator->SetTextL( *text );
                    CleanupStack::PopAndDestroy( text );

                    /* Insert generator into the movie. */
                    TInt err = 0;
                    index = ( iContainer->CurrentIndex() == iMovie->VideoClipCount()) ? iMovie->VideoClipCount(): iContainer->CurrentIndex() + 1;
                    TRAP( err, iMovie->InsertVideoClipL( *generator, ETrue, index ));
                    CleanupStack::Pop( generator );
                    User::LeaveIfError( err );

                    CleanupStack::PopAndDestroy( this ); // restore appui orientation
                    }
                break;
                }
            /*
             * Insert SUBTITLE (fading) text *	
             */
        case EVeiCmdEditVideoViewInsertTextSubTitleFading:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewInsertTextSubTitleFading" );

                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    StoreOrientation();
                    CleanupStack::PushL( TCleanupItem( CleanupRestoreOrientation, this ));
                    AppUi()->SetOrientationL( CAknAppUiBase::EAppUiOrientationPortrait );

                    TSize res = TSize( iMovie->Resolution());
                    CVeiTitleClipGenerator* generator = CVeiTitleClipGenerator::NewLC( res, 
                                                                                       EVeiTitleClipTransitionNone, 
                                                                                       EVeiTitleClipHorizontalAlignmentCenter, 
                                                                                       EVeiTitleClipVerticalAlignmentCenter );

                    HBufC* descriptiveName = StringLoader::LoadLC( R_VEI_EDIT_VIEW_SUBTITLE_FADING_NAME, iEikonEnv );
                    generator->SetDescriptiveNameL( *descriptiveName );
                    CleanupStack::PopAndDestroy( descriptiveName );

                    generator->SetTransitionAndAlignmentsL( EVeiTitleClipTransitionFade, 
                                                            EVeiTitleClipHorizontalAlignmentLeft, 
                                                            EVeiTitleClipVerticalAlignmentBottom );

                    /* Ask for text. */
                    HBufC* text = HBufC::NewLC( KTitleScreenMaxTextLength );
                    TPtr txtptr( text->Des());
                    CAknTextQueryDialog* textQuery = CAknTextQueryDialog::NewL( txtptr );
                    CleanupStack::PushL( textQuery );

                    textQuery->SetMaxLength( KTitleScreenMaxTextLength );
                    //textQuery->SetPredictiveTextInputPermitted(ETrue);
                    CleanupStack::Pop( textQuery );

                    if ( !textQuery->ExecuteLD( R_VEI_EDITVIDEO_TITLESCREEN_TEXT_QUERY ))
                        {
                        CleanupStack::PopAndDestroy( text );
                        CleanupStack::PopAndDestroy( generator );
                        CleanupStack::PopAndDestroy( this ); // restore appui orientation
                        break;
                        }

                    generator->SetTextL( *text );
                    CleanupStack::PopAndDestroy( text );

                    /* Insert generator into the movie. */
                    TInt err = 0;
                    index = ( iContainer->CurrentIndex() == iMovie->VideoClipCount()) ? iMovie->VideoClipCount(): iContainer->CurrentIndex() + 1;
                    TRAP( err, iMovie->InsertVideoClipL( *generator, ETrue, index ));
                    CleanupStack::Pop( generator );
                    User::LeaveIfError( err );

                    CleanupStack::PopAndDestroy( this ); // restore appui orientation
                    }
                break;
                }
            /*
             * Insert CREDIT text *	
             */
        case EVeiCmdEditVideoViewInsertTextCredits:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewInsertTextCredits" );

                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    StoreOrientation();
                    CleanupStack::PushL( TCleanupItem( CleanupRestoreOrientation, this ));
                    AppUi()->SetOrientationL( CAknAppUiBase::EAppUiOrientationPortrait );

                    TSize res = TSize( iMovie->Resolution());
                    CVeiTitleClipGenerator* generator = CVeiTitleClipGenerator::NewLC( res, 
                                                                                       EVeiTitleClipTransitionNone, 
                                                                                       EVeiTitleClipHorizontalAlignmentCenter, 
                                                                                       EVeiTitleClipVerticalAlignmentCenter );

                    HBufC* descriptiveName = StringLoader::LoadLC( R_VEI_EDIT_VIEW_CREDITS_NAME, iEikonEnv );
                    generator->SetDescriptiveNameL( *descriptiveName );
                    CleanupStack::PopAndDestroy( descriptiveName );

                    generator->SetTransitionAndAlignmentsL( EVeiTitleClipTransitionScrollBottomToTop, 
                                                            EVeiTitleClipHorizontalAlignmentCenter, 
                                                            EVeiTitleClipVerticalAlignmentCenter );

                    /* Ask for text. */
                    HBufC* text = HBufC::NewLC( KTitleScreenMaxTextLength );
                    TPtr txtptr( text->Des());
                    CAknTextQueryDialog* textQuery = CAknTextQueryDialog::NewL( txtptr );
                    CleanupStack::PushL( textQuery );

                    textQuery->SetMaxLength( KTitleScreenMaxTextLength );
                    //textQuery->SetPredictiveTextInputPermitted(ETrue);
                    CleanupStack::Pop( textQuery );

                    if ( !textQuery->ExecuteLD( R_VEI_EDITVIDEO_TITLESCREEN_TEXT_QUERY ))
                        {
                        CleanupStack::PopAndDestroy( text );
                        CleanupStack::PopAndDestroy( generator );
                        CleanupStack::PopAndDestroy( this );
                        break;
                        }
                    generator->SetTextL( *text );
                    CleanupStack::PopAndDestroy( text );

                    /* Insert generator into the movie. */
                    TInt err = 0;
                    index = ( iContainer->CurrentIndex() == iMovie->VideoClipCount()) ? iMovie->VideoClipCount(): iContainer->CurrentIndex() + 1;
                    TRAP( err, iMovie->InsertVideoClipL( *generator, ETrue, index ));
                    CleanupStack::Pop( generator );
                    User::LeaveIfError( err );

                    CleanupStack::PopAndDestroy( this ); // restore appui orientation
                    }
                break;
                }

            /*
             * Edit Text *
             */
        case EVeiCmdEditVideoViewEditTextMove:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewEditTextMove" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_OK_CANCEL );
                    Cba()->DrawDeferred();

                    index = iContainer->CurrentIndex();

                    if ( iContainer->CursorLocation() == VideoEditor::ECursorOnClip )
                        {
                        iOriginalVideoClipIndex = index;
                        }
                    else
                        {
                        User::Panic( _L( "VideoEditor" ), 34 );
                        }
                    iContainer->SetSelectionMode( CVeiEditVideoContainer::EModeMove );
                    UpdateEditNaviLabel();
                    }
                break;
                }
        case EVeiCmdEditVideoViewEditTextChangeDuration:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewEditTextChangeDuration" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_OK_CANCEL );
                    Cba()->DrawDeferred();
                    index = iContainer->CurrentIndex();

                    iOriginalVideoStartPoint = iMovie->VideoClipStartTime( index );
                    iOriginalVideoCutInTime = iMovie->VideoClipCutInTime( index );
                    iOriginalVideoCutOutTime = iMovie->VideoClipCutOutTime( index );

                    iContainer->SetSelectionMode( CVeiEditVideoContainer::EModeDuration );
                    UpdateEditNaviLabel();
                    }
                break;
                }
        case EVeiCmdEditVideoViewEditTextChangeText:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewEditTextChangeText" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    StoreOrientation();
                    CleanupStack::PushL( TCleanupItem( CleanupRestoreOrientation, this ));
                    AppUi()->SetOrientationL( CAknAppUiBase::EAppUiOrientationPortrait );

                    Cba()->DrawDeferred();
                    CVedVideoClipInfo* info = iMovie->VideoClipInfo( iContainer->CurrentIndex());

                    CVeiTitleClipGenerator* generator = STATIC_CAST( CVeiTitleClipGenerator* , info->Generator());
                    HBufC* text = HBufC::NewLC( KTitleScreenMaxTextLength );
                    *text = (( CVeiTitleClipGenerator* )info->Generator())->Text();

                    TPtr txtptr( text->Des());
                    CAknTextQueryDialog* textQuery = CAknTextQueryDialog::NewL( txtptr );
                    CleanupStack::PushL( textQuery );

                    textQuery->SetMaxLength( KTitleScreenMaxTextLength );
                    //textQuery->SetPredictiveTextInputPermitted(ETrue);
                    CleanupStack::Pop( textQuery );

                    if ( !textQuery->ExecuteLD( R_VEI_EDITVIDEO_TITLESCREEN_TEXT_QUERY ))
                        {
                        CleanupStack::PopAndDestroy( text );
                        CleanupStack::PopAndDestroy( this ); // restore appui orientation
                        break;
                        }
                    generator->SetTextL( *text );
                    CleanupStack::PopAndDestroy( text );
                    UpdateEditNaviLabel();

                    CleanupStack::PopAndDestroy( this ); // restore appui orientation
                    }
                break;
                }

        case EVeiCmdEditVideoViewEditTextSetTextColor:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewEditTextSetTextColor" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    CVedVideoClipInfo* info = iMovie->VideoClipInfo( iContainer->CurrentIndex());
                    CVeiTitleClipGenerator* generator = STATIC_CAST( CVeiTitleClipGenerator* , info->Generator());

                    TRgb color = generator->TextColor();
                    if ( !iPopup->ShowColorSelectorL( color ))
                        {
                        break;
                        }
                    generator->SetTextColorL( color );
                    }
                break;
                }
        case EVeiCmdEditVideoViewEditTextSetBackGround:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewEditTextSetBackGround" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    CVedVideoClipInfo* info = iMovie->VideoClipInfo( iContainer->CurrentIndex());
                    CVeiTitleClipGenerator* generator = STATIC_CAST( CVeiTitleClipGenerator* , info->Generator());

                    // ask for color or image
                    TBool imageSelected;
                    TInt err = iPopup->ShowTitleScreenBackgroundSelectionPopupL( imageSelected );
                    if ( err != KErrNone )
                        {
                        break;
                        }

                    if ( imageSelected )
                        {
                        CDesCArrayFlat* selectedFiles = new( ELeave )CDesCArrayFlat( 1 );
                        CleanupStack::PushL( selectedFiles );

                        CVeiMGFetchVerifier* mgFetchVerifier = CVeiMGFetchVerifier::NewLC();

                        if ( MGFetch::RunL( *selectedFiles, EImageFile, EFalse, mgFetchVerifier ) == EFalse )
                            {
                            /* User cancelled the dialog. */
                            CleanupStack::PopAndDestroy( mgFetchVerifier );
                            CleanupStack::PopAndDestroy( selectedFiles );
                            break;
                            }

                        CleanupStack::PopAndDestroy( mgFetchVerifier );

                        if ( !iWaitDialog )
                            {
                            iWaitDialog = new( ELeave )CAknWaitDialog( REINTERPRET_CAST( CEikDialog** , &iWaitDialog ), ETrue );
                            iWaitDialog->ExecuteLD( R_VEI_WAIT_DIALOG_INSERTING_IMAGE );
                            }


                        TRAP( err, generator->SetBackgroundImageL(( *selectedFiles )[0], * this ));

                        if ( err )
                            {
                            if ( iWaitDialog )
                                {
                                CancelWaitDialog();
                                }
                            ShowErrorNote( R_VEI_ERRORNOTE_IMAGE_INSERTING_FAILED );
                            }

                        CleanupStack::PopAndDestroy( selectedFiles );
                        }
                    else
                        {
                        TRgb color = generator->BackgroundColor();
                        if ( !iPopup->ShowColorSelectorL( color ))
                            {
                            break;
                            }
                        generator->SetBackgroundColorL( color );
                        }
                    }
                break;
                }
        case EVeiCmdEditVideoViewEditTextStyle:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewEditTextStyle" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    /* Ask for style. */
                    iPopup->ShowTitleScreenStyleSelectionPopupL();
                    }
                break;
                }
            /*
             * Edit text, AddColorEffect *	
             */
        case EVeiCmdEditVideoViewEditTextAddColorEffect:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewEditTextAddColorEffect" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    iPopup->ShowEffectSelectionPopupListL();
                    }
                break;
                }
            /*
             * Edit text style *	
             */
        case EVeiCmdEditVideoViewEditTextStyleTitle:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewEditTextStyleTitle" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    CVedVideoClipInfo* info = iMovie->VideoClipInfo( iContainer->CurrentIndex());

                    CVeiTitleClipGenerator* generator = STATIC_CAST( CVeiTitleClipGenerator* , info->Generator());

                    HBufC* descriptiveName = StringLoader::LoadLC( R_VEI_EDIT_VIEW_TITLE_NAME, iEikonEnv );
                    generator->SetDescriptiveNameL( *descriptiveName );
                    CleanupStack::PopAndDestroy( descriptiveName );

                    generator->SetTransitionAndAlignmentsL( EVeiTitleClipTransitionNone, 
                                                            EVeiTitleClipHorizontalAlignmentCenter, 
                                                            EVeiTitleClipVerticalAlignmentCenter );
                    }
                break;
                }
        case EVeiCmdEditVideoViewEditTextStyleTitleFading:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewEditTextStyleTitleFading" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    CVedVideoClipInfo* info = iMovie->VideoClipInfo( iContainer->CurrentIndex());

                    CVeiTitleClipGenerator* generator = STATIC_CAST( CVeiTitleClipGenerator* , info->Generator());

                    HBufC* descriptiveName = StringLoader::LoadLC( R_VEI_EDIT_VIEW_TITLE_FADING_NAME, iEikonEnv );
                    generator->SetDescriptiveNameL( *descriptiveName );
                    CleanupStack::PopAndDestroy( descriptiveName );
                    generator->SetTransitionAndAlignmentsL( EVeiTitleClipTransitionFade, 
                                                            EVeiTitleClipHorizontalAlignmentCenter, 
                                                            EVeiTitleClipVerticalAlignmentCenter );
                    }
                break;
                }
        case EVeiCmdEditVideoViewEditTextStyleSubTitle:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewEditTextStyleSubTitle" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    CVedVideoClipInfo* info = iMovie->VideoClipInfo( iContainer->CurrentIndex());

                    CVeiTitleClipGenerator* generator = STATIC_CAST( CVeiTitleClipGenerator* , info->Generator());

                    HBufC* descriptiveName = StringLoader::LoadLC( R_VEI_EDIT_VIEW_SUBTITLE_NAME, iEikonEnv );
                    generator->SetDescriptiveNameL( *descriptiveName );
                    CleanupStack::PopAndDestroy( descriptiveName );
                    generator->SetTransitionAndAlignmentsL( EVeiTitleClipTransitionNone, 
                                                            EVeiTitleClipHorizontalAlignmentLeft, 
                                                            EVeiTitleClipVerticalAlignmentBottom );

                    }
                break;
                }
        case EVeiCmdEditVideoViewEditTextStyleSubTitleFading:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewEditTextStyleSubTitleFading" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    CVedVideoClipInfo* info = iMovie->VideoClipInfo( iContainer->CurrentIndex());

                    CVeiTitleClipGenerator* generator = STATIC_CAST( CVeiTitleClipGenerator* , info->Generator());

                    HBufC* descriptiveName = StringLoader::LoadLC( R_VEI_EDIT_VIEW_SUBTITLE_FADING_NAME, iEikonEnv );
                    generator->SetDescriptiveNameL( *descriptiveName );
                    CleanupStack::PopAndDestroy( descriptiveName );
                    generator->SetTransitionAndAlignmentsL( EVeiTitleClipTransitionFade, 
                                                            EVeiTitleClipHorizontalAlignmentLeft, 
                                                            EVeiTitleClipVerticalAlignmentBottom );
                    }
                break;
                }
        case EVeiCmdEditVideoViewEditTextStyleCredit:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewEditTextStyleCredit" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    CVedVideoClipInfo* info = iMovie->VideoClipInfo( iContainer->CurrentIndex());

                    CVeiTitleClipGenerator* generator = STATIC_CAST( CVeiTitleClipGenerator* , info->Generator());

                    HBufC* descriptiveName = StringLoader::LoadLC( R_VEI_EDIT_VIEW_CREDITS_NAME, iEikonEnv );
                    generator->SetDescriptiveNameL( *descriptiveName );
                    CleanupStack::PopAndDestroy( descriptiveName );
                    generator->SetTransitionAndAlignmentsL( EVeiTitleClipTransitionScrollBottomToTop, 
                                                            EVeiTitleClipHorizontalAlignmentCenter, 
                                                            EVeiTitleClipVerticalAlignmentCenter );
                    }
                break;
                }
            /*
             * Edit text, Duplicate *	
             */
        case EVeiCmdEditVideoViewEditTextDuplicate:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewEditTextDuplicate" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    CVedVideoClipInfo* previousInfo = iMovie->VideoClipInfo( iContainer->CurrentIndex());
                    CVeiTitleClipGenerator* generator = STATIC_CAST( CVeiTitleClipGenerator* , previousInfo->Generator());

                    iWaitMode = EDuplicating;

                    CVeiTitleClipGenerator* generator2 = CVeiTitleClipGenerator::NewLC( iMovie->Resolution(), 
                                                                                        generator->Transition(), 
                                                                                        generator->HorizontalAlignment(), 
                                                                                        generator->VerticalAlignment());

                    generator2->SetDescriptiveNameL( generator->DescriptiveName());
                    generator2->SetTransitionAndAlignmentsL( generator->Transition(), 
                                                             generator->HorizontalAlignment(), 
                                                             generator->VerticalAlignment() );
                    generator2->SetTextL( generator->Text());
                    generator2->SetTextColorL( generator->TextColor());

                    generator2->SetBackgroundColorL( generator->BackgroundColor());
                    if ( generator->BackgroundImage())
                        {
                        generator2->SetBackgroundImageL( generator->BackgroundImage());
                        }

                    generator2->SetDuration( generator->Duration());

                    TInt err = 0;
                    index = ( iContainer->CurrentIndex() == iMovie->VideoClipCount()) ? iMovie->VideoClipCount(): iContainer->CurrentIndex() + 1;
                    TRAP( err, iMovie->InsertVideoClipL( *generator2, ETrue, index ));

                    CleanupStack::Pop( generator2 );
                    User::LeaveIfError( err );
                    }
                break;
                }
            /**
             * Insert Image
             */
        case EVeiCmdEditVideoViewInsertImage:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewInsertImage" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    TTimeIntervalMicroSeconds duration( 5000000 );

                    CDesCArrayFlat* selectedFiles = new( ELeave )CDesCArrayFlat( 1 );
                    CleanupStack::PushL( selectedFiles );

                    CVeiMGFetchVerifier* mgFetchVerifier = CVeiMGFetchVerifier::NewLC();

                    if ( MGFetch::RunL( *selectedFiles, EImageFile, EFalse, mgFetchVerifier ) == EFalse )
                        {
                        /* User cancelled the dialog. */
                        CleanupStack::PopAndDestroy( mgFetchVerifier );
                        CleanupStack::PopAndDestroy( selectedFiles );
                        break;
                        }

                    CleanupStack::PopAndDestroy( mgFetchVerifier );

                    if ( !iWaitDialog )
                        {
                        iWaitDialog = new( ELeave )CAknWaitDialog( REINTERPRET_CAST( CEikDialog** , &iWaitDialog ), ETrue );
                        iWaitDialog->ExecuteLD( R_VEI_WAIT_DIALOG_INSERTING_IMAGE );
                        }

                    RFs& fs = iEikonEnv->FsSession();

                    TRAPD( err, iGenerator = CVeiImageClipGenerator::NewL(( *selectedFiles )[0], 
                                                                            TSize( KMaxVideoFrameResolutionX, KMaxVideoFrameResolutionY ), 
                                                                            duration, 
                                                                            KRgbBlack, 
                                                                            KVideoClipGenetatorDisplayMode, 
                                                                            fs, 
                                                                            *this ));
                    if ( err )
                        {
                        if ( iWaitDialog )
                            {
                            CancelWaitDialog();
                            }
                        ShowErrorNote( R_VEI_ERRORNOTE_IMAGE_INSERTING_FAILED );
                        }

                    CleanupStack::PopAndDestroy( selectedFiles );
                    }
                break;
                }

            /**
             * Edit Image * 
             */
        case EVeiCmdEditVideoViewEditImageMove:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewEditImageMove" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_OK_CANCEL );
                    Cba()->DrawDeferred();

                    index = iContainer->CurrentIndex();

                    if ( iContainer->CursorLocation() == VideoEditor::ECursorOnClip )
                        {
                        iOriginalVideoClipIndex = index;
                        }
                    else
                        {
                        User::Panic( _L( "VideoEditor" ), 34 );
                        }
                    iContainer->SetSelectionMode( CVeiEditVideoContainer::EModeMove );
                    UpdateEditNaviLabel();
                    }
                break;
                }

        case EVeiCmdEditVideoViewEditImageChangeDuration:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewEditImageChangeDuration" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_OK_CANCEL );
                    Cba()->DrawDeferred();
                    index = iContainer->CurrentIndex();

                    iOriginalVideoStartPoint = iMovie->VideoClipStartTime( index );
                    iOriginalVideoCutInTime = iMovie->VideoClipCutInTime( index );
                    iOriginalVideoCutOutTime = iMovie->VideoClipCutOutTime( index );

                    iContainer->SetSelectionMode( CVeiEditVideoContainer::EModeDuration );
                    UpdateEditNaviLabel();
                    }
                break;
                }
        case EVeiCmdEditVideoViewEditImageBackGround:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewEditImageBackGround" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    CVedVideoClipInfo* info = iMovie->VideoClipInfo( iContainer->CurrentIndex());
                    CVeiImageClipGenerator* generator = STATIC_CAST( CVeiImageClipGenerator* , info->Generator());

                    // ask for color			
                    TRgb color = generator->BackgroundColor();
                    if ( !iPopup->ShowColorSelectorL( color ))
                        {
                        break;
                        }

                    generator->SetBackgroundColor( color );
                    }
                break;
                }
        case EVeiCmdEditVideoViewEditImageAddColorEffect:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewEditImageAddColorEffect" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    iPopup->ShowEffectSelectionPopupListL();
                    }
                break;
                }
            // * DUPLICATE Image *
        case EVeiCmdEditVideoViewEditImageDuplicate:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewEditImageDuplicate" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    CVedVideoClipInfo* previousInfo = iMovie->VideoClipInfo( iContainer->CurrentIndex());
                    CVeiImageClipGenerator* generator = STATIC_CAST( CVeiImageClipGenerator* , previousInfo->Generator());

                    RFs& fs = iEikonEnv->FsSession();

                    iWaitMode = EDuplicating;

                    iGenerator = CVeiImageClipGenerator::NewL( generator->ImageFilename(), 
                                                               TSize( KMaxVideoFrameResolutionX, KMaxVideoFrameResolutionY ), 
                                                               previousInfo->Duration(), 
                                                               generator->BackgroundColor(), 
                                                               KVideoClipGenetatorDisplayMode, 
                                                               fs, 
                                                               *this );
                    }
                break;
                }

            /**
             * Insert -> Sound clip
             */
        case EVeiCmdEditVideoViewInsertAudio:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewInsertAudio" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    if ( !iMediaQueue->ShowAudioClipDialogL())
                        {
                        break;
                        }
                    }
                HandleScreenDeviceChangedL();
                break;
                }
            /**
             * Insert -> New sound clip
             */
        case EVeiCmdEditVideoViewInsertNewAudio:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewInsertNewAudio" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    InsertNewAudio();
                    }
                break;
                }

        case EVeiCmdEditVideoViewEditAudioSetDuration:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewEditAudioSetDuration" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_OK_CANCEL );
                    Cba()->DrawDeferred();
                    index = iContainer->CurrentIndex();

                    iOriginalAudioStartPoint = iMovie->AudioClipStartTime( index );
                    iOriginalAudioDuration = iMovie->AudioClipEditedDuration( index );
                    iContainer->SetSelectionMode( CVeiEditVideoContainer::EModeDuration );
                    UpdateEditNaviLabel();
                    }
                break;
                }
        case EVeiCmdEditVideoViewSettings:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewSettings" );
                AppUi()->ActivateLocalViewL( TUid::Uid( EVeiSettingsView ));
                break;
                }
        case EVeiCmdEditVideoViewRecord:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewRecord" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    /* Set the mode, CBAs and Navi label. */
                    iContainer->SetSelectionMode( CVeiEditVideoContainer::EModeRecording );

                    Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_EMPTY );
                    Cba()->DrawDeferred();
                    UpdateEditNaviLabel();

                    iRecordedAudioMaxDuration = TTimeIntervalMicroSeconds(  - 1 );
                    for ( TInt i = 0; i < iMovie->AudioClipCount(); i++ )
                        {

                        if ( iMovie->AudioClipStartTime( i ) > iContainer->RecordedAudioStartTime())
                            {
                            TInt64 startTimeInt = iContainer->RecordedAudioStartTime().Int64();
                            TInt64 nextStartTimeInt = iMovie->AudioClipStartTime( i ).Int64();
                            iRecordedAudioMaxDuration = TTimeIntervalMicroSeconds( nextStartTimeInt - startTimeInt );
                            break;
                            }
                        }

                    iRecorder->RecordL();
                    const TUint delay = 1000 * 1000 / 10;

                    iAudioRecordPeriodic->Start( delay, delay, TCallBack( CVeiEditVideoView::UpdateAudioRecording, this ));

                    iContainer->SetRecordedAudioDuration( TTimeIntervalMicroSeconds( 0 ));
                    }
                break;
                }
        case EVeiCmdEditVideoViewRecordCancel:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewRecordCancel" );
                // cancel recording
                iContainer->SetSelectionMode( CVeiEditVideoContainer::EModeNavigation );
                Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_OPTIONS_BACK );
                iContainer->SetRecordedAudioStartTime( TTimeIntervalMicroSeconds( 0 ));
                iContainer->SetCursorLocation( VideoEditor::ECursorOnAudio );
                Cba()->DrawDeferred();

                UpdateEditNaviLabel();
                break;
                }
            /*
             *   Stop previewing
             */
        case EVeiCmdPlayViewStop:
        case EVeiCmdCutVideoViewStop:
        case EVeiCmdEditVideoViewRecordStop:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdPlayViewStop||EVeiCmdCutVideoViewStop||EVeiCmdEditVideoViewRecordStop" );
                if ( EditorState() == EQuickPreview )
                    {
                    iContainer->StopVideo( EFalse );
                    iContainer->SetFinishedStatus( ETrue );
                    StopNaviPaneUpdateL();
                    LOG( KVideoEditorLogFile, "\tEVeiCmdPlayViewStop||EVeiCmdCutVideoViewStop||EVeiCmdEditVideoViewRecordStop, setting R_VEI_SOFTKEYS_PREVIEW_PLAY_BACK" );
                    Cba()->SetCommandSetL( R_VEI_SOFTKEYS_PREVIEW_PLAY_BACK );
                    Cba()->DrawDeferred();
                    break;
                    }

                // stop recording
                iContainer->SetSelectionMode( CVeiEditVideoContainer::EModeNavigation );
                Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_OPTIONS_BACK );
                Cba()->DrawDeferred();
                UpdateEditNaviLabel();

                iRecorder->Stop();
                iAudioRecordPeriodic->Cancel();

                if ( iAudioClipInfo )
                    {
                    delete iAudioClipInfo;
                    iAudioClipInfo = NULL;
                    }

                iProgressNote = new( ELeave )CAknProgressDialog( REINTERPRET_CAST( CEikDialog** , &iProgressNote ), ETrue );

                iProgressNote->SetCallback( this );
                iProgressNote->ExecuteDlgLD( R_VEI_PROGRESS_NOTE );
                HBufC* stringholder = StringLoader::LoadL( R_VEI_PROGRESS_NOTE_INSERTING_AUDIO, iEikonEnv );
                CleanupStack::PushL( stringholder );

                iProgressNote->SetTextL( *stringholder );
                CleanupStack::PopAndDestroy( this ); // stringholder

                iProgressNote->GetProgressInfoL()->SetFinalValue( 100 );
                iWaitMode = EOpeningAudioInfo;

                iAudioClipInfo = CVedAudioClipInfo::NewL( *iTempRecordedAudio, *this );
                break;
                }
        case EVeiCmdPlayViewPause:
        case EVeiCmdEditVideoViewRecordPause:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdPlayViewPause||EVeiCmdEditVideoViewRecordPause" );
                if ( EditorState() == EQuickPreview )
                    {
                    iContainer->PauseVideoL();
                    StopNaviPaneUpdateL();

                    LOG( KVideoEditorLogFile, "\tEVeiCmdPlayViewPause||EVeiCmdEditVideoViewRecordPause, setting R_VEI_SOFTKEYS_PREVIEW_PLAY_BACK" );
                    Cba()->SetCommandSetL( R_VEI_SOFTKEYS_PREVIEW_PLAY_BACK );
                    Cba()->DrawDeferred();
                    }
                else
                    {
                    Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_EMPTY );
                    Cba()->DrawDeferred();

                    // Pause recording
                    // Cba is set to CONTINUE_STOP in DoUpdateAudioRecording()
                    iContainer->SetSelectionMode( CVeiEditVideoContainer::EModeRecordingPaused );
                    iRecorder->Stop();
                    UpdateEditNaviLabel();
                    }
                break;
                }
            /*
             *	Preview continue:
             */
        case EVeiCmdCutVideoViewPlay:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdCutVideoViewPlay" );

                if ( !iUpdateTemp && !iTempFile && 1 == iMovie->VideoClipCount() )
                    {
                    iContainer->SetSelectionMode( CVeiEditVideoContainer::EModePreview );
                    iContainer->PlayVideo( iMovie->VideoClipInfo( 0 )->DescriptiveName(), iFullScreenSelected );
                    }
                else
                    {
                    iContainer->SetSelectionMode( CVeiEditVideoContainer::EModePreview );
                    iContainer->PlayVideo( *iTempFile, iFullScreenSelected );
                    }

                // @: think should this be put under condition play was started
                // (actually play starts when "loadingComplete" event comes to NotifyVideoDisplayEvent
                //StartNaviPaneUpdateL();            
                break;
                }

        case EVeiCmdCutVideoTakeSnapshot:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdCutVideoTakeSnapshot" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    iContainer->TakeSnapshotL();
                    }
                break;
                }

        case EVeiCmdCutVideoViewContinue:
        case EVeiCmdEditVideoViewContinue:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdCutVideoViewContinue||EVeiCmdEditVideoViewContinue" );
                if (( iRecorder->State() != CMdaAudioClipUtility::ERecording ) && 
                    ( iRecorder->State() != CMdaAudioClipUtility::ENotReady ) )
                    {
                    Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_EMPTY );
                    Cba()->DrawDeferred();

                    // Continue recording
                    iRecorder->RecordL();
                    iContainer->SetSelectionMode( CVeiEditVideoContainer::EModeRecording );

                    UpdateEditNaviLabel();
                    }
                break;
                }
        case EVeiCmdEditVideoViewEditAudioMove:
        case EVeiCmdEditVideoViewEditVideoMove:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewEditAudioMove||EVeiCmdEditVideoViewEditVideoMove" );
                Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_OK_CANCEL );
                Cba()->DrawDeferred();

                index = iContainer->CurrentIndex();

                if ( iContainer->CursorLocation() == VideoEditor::ECursorOnAudio )
                    {
                    iOriginalAudioStartPoint = iMovie->AudioClipStartTime( index );
                    iOriginalAudioDuration = iMovie->AudioClipEditedDuration( index );
                    }
                else if ( iContainer->CursorLocation() == VideoEditor::ECursorOnClip )
                    {
                    iOriginalVideoClipIndex = index;
                    }
                else
                    {
                    User::Panic( _L( "VideoEditor" ), 34 );
                    }
                iContainer->SetSelectionMode( CVeiEditVideoContainer::EModeMove );
                UpdateEditNaviLabel();
                break;
                }
            /**
             * Edit video clip -> Add colour effect
             */
        case EVeiCmdEditVideoViewEditVideoColorEffect:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewEditVideoColorEffect" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    iPopup->ShowEffectSelectionPopupListL();
                    }
                break;
                }
            /**
             * Use slow motion
             */
        case EVeiCmdEditVideoViewEditVideoSlowMotion:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewEditVideoSlowMotion" );
                if ( IsEnoughFreeSpaceToSaveL())
                    {
                    Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_OK_CANCEL );
                    Cba()->DrawDeferred();
                    iOriginalVideoSpeed = iMovie->VideoClipSpeed( iContainer->CurrentIndex());
                    iContainer->SetSelectionMode( CVeiEditVideoContainer::EModeSlowMotion );
                    iContainer->SetSlowMotionStartValueL( iOriginalVideoSpeed );
                    iContainer->ArrowsControl();
                    }
                break;
                }
        case EVeiCmdEditVideoViewEditVideoMute:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewEditVideoMute" );
                SetNewTempFileNeeded( ETrue );
                iMovie->VideoClipSetMuted( iContainer->CurrentIndex(), ETrue );
                break;
                }
        case EVeiCmdEditVideoViewEditVideoUnmute:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewEditVideoUnmute" );
                SetNewTempFileNeeded( ETrue );
                iMovie->VideoClipSetMuted( iContainer->CurrentIndex(), EFalse );
                break;
                }
        case EVeiCmdEditVideoViewEditAudioRemove:
        case EVeiCmdEditVideoViewEditVideoRemove:
        case EVeiCmdEditVideoViewEditTextRemove:
        case EVeiCmdEditVideoViewEditImageRemove:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewEditAudioRemove||EVeiCmdEditVideoViewEditVideoRemove||EVeiCmdEditVideoViewEditTextRemove||EVeiCmdEditVideoViewEditImageRemove" );
                RemoveCurrentClipL();
                break;
                }
            /**
             * Back
             */
        case EAknSoftkeyBack:
                {
                LOG( KVideoEditorLogFile, "CVeiEditVideoView::HandleCommandL: EAknSoftkeyBack" );
                LOGFMT( KVideoEditorLogFile, "\tiFullScreenSelected: %d", iFullScreenSelected );

                if ( EditorState() != EEdit || iFullScreenSelected )
                    {
                    iFullScreenSelected = EFalse;
                    iContainer->StopVideo( ETrue );
                    StopNaviPaneUpdateL();
                    UpdateEditNaviLabel();
                    break;
                    }
                else if ( !iMovieSavedFlag && ( iMovie->VideoClipCount() > 0 || iMovie->AudioClipCount() > 0 ))
                    {
                    HBufC* saveConfirmationString; // String holding the text shown in dialog.
                    CAknQueryDialog* dlg; // Save confirmation dialog.
                    TInt saveEditedVideo; // Query result.

                    saveConfirmationString = StringLoader::LoadLC( R_VEI_CONFIRM_EXIT_SAVE, iEikonEnv );
                    dlg = new( ELeave )CAknQueryDialog( *saveConfirmationString, CAknQueryDialog::ENoTone );
                    saveEditedVideo = dlg->ExecuteLD( R_VEI_CONFIRMATION_QUERY );

                    if ( !saveEditedVideo )
                    // Do not save.
                        {
                        // Activate videos view.                    
                        AppUi()->HandleCommandL( EAknCmdExit );

                        iMovie->Reset();

                        iMovieSavedFlag = ETrue; // Movie is saved.
                        iMovieFirstAddFlag = ETrue; // True for the next edit process.

                        AppUi()->HandleCommandL( aCommand );
                        }
                    else
                        {
                        if ( SaveL( EProcessingMovieSaveThenQuit ))
                        // Quit after saving?
                            {
                            //iMovieSavedFlag = ETrue;		// Movie is saved.
                            iMovieFirstAddFlag = ETrue; // True for the next edit process.
                            iWaitMode = EProcessingMovieSaveThenQuit;
                            }
                        }

                    CleanupStack::PopAndDestroy( saveConfirmationString ); 
                    }
                // No changes to clip(s) or no clip(s) in time line.
                else
                    {
                    // Remove all clips from edit view (for future use).
                    iMovie->Reset();

                    iMovieSavedFlag = ETrue; // Movie is saved.
                    iMovieFirstAddFlag = ETrue; // True for the next edit process.
                    AppUi()->HandleCommandL( aCommand );
                    }

                break;
                }
            //
            // Options->Help
            //
        case EVeiCmdEditVideoViewHelp:
                {
                LOG( KVideoEditorLogFile, "\tEVeiCmdEditVideoViewHelp" );
                // CS Help launching is handled in Video Editor's AppUi.
                AppUi()->HandleCommandL( EVeiCmdEditVideoViewHelp );
                break;
                }
            /**
             * Exit
             */
        case EEikCmdExit:
                {
                LOG( KVideoEditorLogFile, "\tEEikCmdExit" );
                // Edited movie is not saved yet and there are video or audio clip(s) at the time line.
                if ( !iMovieSavedFlag && ( iMovie->VideoClipCount() > 0 || iMovie->AudioClipCount() > 0 ))
                    {
                    HBufC* stringholder; // String holding the text shown in dialog.
                    
                    CAknQueryDialog* dlg; // Save confirmation dialog.
                    TInt queryok; // Query result.

                    stringholder = StringLoader::LoadLC( R_VEI_CONFIRM_EXIT_SAVE, iEikonEnv );

                    dlg = new( ELeave )CAknQueryDialog( *stringholder, CAknQueryDialog::ENoTone );
                    queryok = dlg->ExecuteLD( R_VEI_CONFIRMATION_QUERY );

                    if ( !queryok )
                        {
                        iMovie->Reset();
                        AppUi()->HandleCommandL( aCommand );
                        }
                    else
                        {
                        SaveL( EProcessingMovieSaveThenQuit );
                        iWaitMode = EProcessingMovieSaveThenQuit;
                        }

                    CleanupStack::PopAndDestroy( stringholder );
                    }
                else
                // No changes to clip(s) or no clip(s) in time line.
                    {
                    iMovie->Reset(); // Remove all clips from edit view.
                    AppUi()->HandleCommandL( aCommand ); // Let appUi handle the exit.
                    }
                break;
                }
        default:
                {
                LOG( KVideoEditorLogFile, "\tdefault" );
                AppUi()->HandleCommandL( aCommand );
                break;
                }
        }

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::HandleCommandL: Out" );
    }

void CVeiEditVideoView::HandleResourceChange( TInt aType )
    {
    LOGFMT( KVideoEditorLogFile, "CVeiEditVideoView::HandleResourceChange() In, aType:%d", aType );

    if ( KAknsMessageSkinChange == aType )
        {
        // Handle skin change in the navi label controls - they do not receive 
        // it automatically since they are not in the control stack
        iPreviewLabel->DecoratedControl()->HandleResourceChange( aType );
        iEditLabel->DecoratedControl()->HandleResourceChange( aType );
        iVolumeNavi->DecoratedControl()->HandleResourceChange( aType );
        iMoveLabel->DecoratedControl()->HandleResourceChange( aType );
        }

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::HandleResourceChange() Out" );
    }

void CVeiEditVideoView::SetEditorState( TEditorState aState )
    {
    LOGFMT3( KVideoEditorLogFile, "CVeiEditVideoView::SetEditorState: in, aState:%d, iEditorState:%d, iFullScreenSelected:%d", aState, iEditorState, iFullScreenSelected );

    CAknTitlePane* titlePane;
    CEikStatusPane* statusPane;
    TResourceReader reader1;

    iEditorState = aState;

    switch ( aState )
        {
        case EPreview:
            /*
            if ( iFullScreenSelected )
            {
            // @: this need more elaborating
            // problem is: after large preview signal and battery pane are black in some phone models
            //statusPane = ( ( CAknAppUi* )iEikonEnv->EikAppUi() )->StatusPane(); 
            //statusPane ->MakeVisible( EFalse );
            Cba()->MakeVisible( EFalse );
            Cba()->DrawDeferred();
            }
            else
            {
            iEditorState = EQuickPreview;
            statusPane = ( ( CAknAppUi* )iEikonEnv->EikAppUi() )->StatusPane(); 

            titlePane = (CAknTitlePane*) statusPane->ControlL( TUid::Uid( EEikStatusPaneUidTitle ) );
            iCoeEnv->CreateResourceReaderLC( reader1, R_VEI_PREVIEW_VIEW_TITLE_NAME );
            titlePane->SetFromResourceL( reader1 );
            CleanupStack::PopAndDestroy(); //reader1

            LOG(KVideoEditorLogFile, "CVeiEditVideoView::SetEditorState, 1, setting R_VEI_SOFTKEYS_PREVIEW_PAUSE_BACK");
            Cba()->SetCommandSetL( R_VEI_SOFTKEYS_PREVIEW_PAUSE_BACK );
            Cba()->DrawDeferred();
            }
             */
            Cba()->MakeVisible( EFalse );
            Cba()->DrawDeferred();
            break;
        case EQuickPreview:
                {
                LOGFMT( KVideoEditorLogFile, "CVeiEditVideoView::SetEditorState: 2, iContainer->PreviewState():%d", iContainer->PreviewState());
                statusPane = (( CAknAppUi* )iEikonEnv->EikAppUi())->StatusPane();

                titlePane = ( CAknTitlePane* )statusPane->ControlL( TUid::Uid( EEikStatusPaneUidTitle ));
                iCoeEnv->CreateResourceReaderLC( reader1, R_VEI_PREVIEW_VIEW_TITLE_NAME );
                titlePane->SetFromResourceL( reader1 );
                CleanupStack::PopAndDestroy(); //reader1

                if (( iContainer->PreviewState() == CVeiEditVideoContainer::EStatePaused ) || 
                    ( iContainer->PreviewState() == CVeiEditVideoContainer::EStateStopped ))
                    {
                    LOG( KVideoEditorLogFile, "CVeiEditVideoView::SetEditorState, 3, setting R_VEI_SOFTKEYS_PREVIEW_PLAY_BACK" );
                    Cba()->SetCommandSetL( R_VEI_SOFTKEYS_PREVIEW_PLAY_BACK );
                    }
                else if (( iContainer->PreviewState() == CVeiEditVideoContainer::EStateOpening ) || 
                         ( iContainer->PreviewState() == CVeiEditVideoContainer::EStateGettingFrame ) || 
                         ( iContainer->PreviewState() == CVeiEditVideoContainer::EStateBuffering ))
                    {
                    LOG( KVideoEditorLogFile, "CVeiEditVideoView::SetEditorState, 4, setting R_AVKON_SOFTKEYS_EMPTY" );
                    Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_EMPTY );
                    }
                else
                    {
                    LOG( KVideoEditorLogFile, "CVeiEditVideoView::SetEditorState, 5, setting R_VEI_SOFTKEYS_PREVIEW_PAUSE_BACK" );
                    Cba()->SetCommandSetL( R_VEI_SOFTKEYS_PREVIEW_PAUSE_BACK );
                    }
                Cba()->DrawDeferred();
                break;
                }
        case EEdit:
            iContainer->SetRect( AppUi()->ClientRect());
            statusPane = (( CAknAppUi* )iEikonEnv->EikAppUi())->StatusPane();
            // @: this needs more elaborating
            //statusPane ->MakeVisible( ETrue );

            titlePane = ( CAknTitlePane* )statusPane->ControlL( TUid::Uid( EEikStatusPaneUidTitle ));
            iCoeEnv->CreateResourceReaderLC( reader1, R_VEI_EDIT_VIDEO_VIEW_TITLE_NAME );
            titlePane->SetFromResourceL( reader1 );
            CleanupStack::PopAndDestroy(); //reader1

            Cba()->MakeVisible( ETrue );
            Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_OPTIONS_BACK );
            Cba()->DrawDeferred();
            break;

        case EMixAudio:
            iContainer->SetRect( AppUi()->ClientRect());
            statusPane = (( CAknAppUi* )iEikonEnv->EikAppUi())->StatusPane();

            titlePane = ( CAknTitlePane* )statusPane->ControlL( TUid::Uid( EEikStatusPaneUidTitle ));
            iCoeEnv->CreateResourceReaderLC( reader1, R_VEI_AUDIO_MIX_VIEW_TITLE_NAME );
            titlePane->SetFromResourceL( reader1 );
            CleanupStack::PopAndDestroy(); //reader1

            Cba()->MakeVisible( ETrue );
            Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_OPTIONS_DONE );
            Cba()->DrawDeferred();
            iContainer->SetSelectionMode( CVeiEditVideoContainer::EModeMixingAudio );

            break;

        case EAdjustVolume:
            iContainer->SetRect( AppUi()->ClientRect());
            statusPane = (( CAknAppUi* )iEikonEnv->EikAppUi())->StatusPane();

            titlePane = ( CAknTitlePane* )statusPane->ControlL( TUid::Uid( EEikStatusPaneUidTitle ));
            iCoeEnv->CreateResourceReaderLC( reader1, R_VEI_ADJUST_VOLUME_VIEW_TITLE_NAME );
            titlePane->SetFromResourceL( reader1 );
            CleanupStack::PopAndDestroy(); //reader1

            Cba()->MakeVisible( ETrue );
            //Cba()->SetCommandSetL(R_AVKON_SOFTKEYS_OPTIONS_DONE);			
            Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_OK_CANCEL );
            Cba()->DrawDeferred();
            iContainer->SetSelectionMode( CVeiEditVideoContainer::EModeAdjustVolume );

            break;


        default:
                {
                break;
                }
        }
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::SetEditorState: Out" );
    }


void CVeiEditVideoView::SendMovieL()
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::SendMovieL() -- SENDING" );

    TEntry fileinfo;

    RFs& fs = iEikonEnv->FsSession();
    // Rename movie from xxxx.$$$ to defaultfilename from settingsview.
    // looks better in attachment list..

    // Get default movie name from settings view

    TPtr temppeet = iTempFile->Des();

    TParse parse;
    parse.Set( iMovieSaveSettings.DefaultVideoName(), &temppeet, NULL );

    TFileName orgPathAndName = parse.FullName();

    TVedVideoFormat movieQuality = iMovie->Format();
    if ( movieQuality == EVedVideoFormatMP4 )
        {
        orgPathAndName.Replace( orgPathAndName.Length() - 4, 4, KExtMp4 );
        }
    else
        {
        orgPathAndName.Replace( orgPathAndName.Length() - 4, 4, KExt3gp );
        }

    fs.Replace( *iTempFile, orgPathAndName );
    iTempFile->Des() = orgPathAndName;

    fs.Entry( *iTempFile, fileinfo );

    DEBUGLOG_ARG( TInt tempFileSize = fileinfo.iSize / 1024 );
    LOGFMT3( KVideoEditorLogFile, "CVeiEditVideoView::SendMovieL() 1, iWaitMode:%d, tempFileSize:%d, ValidateServiceL:%d", iWaitMode, tempFileSize, iSendAppUi.ValidateServiceL( iGivenSendCommand, TSendingCapabilities( 0, tempFileSize, TSendingCapabilities::ESupportsAttachments )));

    if ( EProcessingMovieSend == iWaitMode 
                /*&& (iSendAppUi.ValidateServiceL(
                                iGivenSendCommand, 
                                TSendingCapabilities( 0, 
                                                      tempFileSize, 
                                                      TSendingCapabilities::ESupportsAttachments ) ) ) */ )
        {
        RFs shareFServer;
        LOG( KVideoEditorLogFile, "CVeiEditVideoView::SendMovieL: shareFServer connect." );

        User::LeaveIfError( shareFServer.Connect());
        shareFServer.ShareProtected();
        CleanupClosePushL < RFs > ( shareFServer );

        RFile openFileHandle;

        TInt err = openFileHandle.Open( shareFServer, * iTempFile, EFileRead | EFileShareReadersOnly );
        if ( KErrNone != err )
            {
            LOGFMT( KVideoEditorLogFile, "CVeiEditVideoView::SendMovieL 2: Could not open file %S with EFileShareReadersOnly. Trying EFileShareAny", iTempFile );
            User::LeaveIfError( openFileHandle.Open( shareFServer, * iTempFile, EFileRead | EFileShareAny ));
            }

        CMessageData* messageData = CMessageData::NewLC();
        messageData->AppendAttachmentHandleL( openFileHandle );

        LOG( KVideoEditorLogFile, "CVeiEditVideoView::SendMovieL() 3" );

        iSendAppUi.CreateAndSendMessageL( iGivenSendCommand, messageData, KNullUid, EFalse );
        LOG( KVideoEditorLogFile, "CVeiEditVideoView::SendMovieL() 4" );
        CleanupStack::PopAndDestroy( messageData );

        CleanupStack::PopAndDestroy( &shareFServer ); // shareFServer.Close();
        LOG( KVideoEditorLogFile, "CVeiEditVideoView::SendMovieL 5: shareFServer closed." );
        }

    DoUpdateEditNaviLabelL();
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::SendMovieL() Out" );
    }


void CVeiEditVideoView::StartTempFileProcessingL()
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::StartTempFileProcessingL: In" );

    RFs& fs = iEikonEnv->FsSession();

    TBool fileExists( ETrue );
    if ( iTempFile )
        {
        fileExists = BaflUtils::FileExists( fs, * iTempFile );
        }

    /* to save from unnecessary processing before preview
    prerequisites:
    -user selected preview
    -no movie modifying actions taken by user before preview (iUpdateTemp is EFalse)	 
     */
    if ( EProcessingMoviePreview == iWaitMode && !iUpdateTemp && !iTempFile && 1 == iMovie->VideoClipCount())
        {
        if ( iFullScreenSelected )
            {
            iContainer->SetBlackScreen( ETrue );
            iContainer->SetRect( AppUi()->ApplicationRect());
            }

        iContainer->SetSelectionMode( CVeiEditVideoContainer::EModePreview );
        iContainer->PlayVideoFileL( iMovie->VideoClipInfo( 0 )->DescriptiveName(), iFullScreenSelected );
        }
    else if ( EProcessingMovieTrimMms == iWaitMode && !iUpdateTemp && !iTempFile && 1 == iMovie->VideoClipCount() && !FitsToMmsL())
        {
        /*
        Read documentation of FitsToMmsL() in the header file
         */
        TBuf8 < KMaxFileName > conv8Filename;
        CnvUtfConverter::ConvertFromUnicodeToUtf8( conv8Filename, iMovie->VideoClipInfo( 0 )->DescriptiveName());
        iMovie->UnregisterMovieObserver( iContainer );
        iMovie->UnregisterMovieObserver( this );
        AppUi()->ActivateLocalViewL( TUid::Uid( EVeiTrimForMmsView ), TUid::Uid( 0 ), conv8Filename );
        }
    else if ( iUpdateTemp || !fileExists || ( !iTempFile ))
        {
        LOG( KVideoEditorLogFile, "CVeiEditVideoView::StartTempFileProcessingL() 1, -- NEW TEMP" );

        Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_EMPTY );
        Cba()->DrawDeferred();
        if ( iTempFile && fileExists )
            {
            User::LeaveIfError( fs.Delete( *iTempFile ));
            delete iTempFile;
            iTempFile = NULL;
            }

        if ( !IsEnoughFreeSpaceToSaveL()) // modifies iMemoryInUse
            {
            return ;
            }

        iTempFile = HBufC::NewL( KMaxFileName );
        iTempMaker->GenerateTempFileName( *iTempFile, iMemoryInUse, iMovie->Format());
        LOGFMT( KVideoEditorLogFile, "CVeiEditVideoView::StartTempFileProcessingL 2, iTempFile:%S", iTempFile );

        TVeiSettings movieSaveSettings;

        STATIC_CAST( CVeiAppUi* , AppUi())->ReadSettingsL( movieSaveSettings );

        TInt settingsSaveQuality = movieSaveSettings.SaveQuality();
        CVedMovie::TVedMovieQuality saveQuality;

        switch ( settingsSaveQuality )
            {
            case TVeiSettings::EMmsCompatible: 
                {
                saveQuality = CVedMovie::EQualityMMSInteroperability;
                break;                    
                }
            case TVeiSettings::EMedium: 
                {
                saveQuality = CVedMovie::EQualityResolutionMedium;
                break;                    
                }
            case TVeiSettings::EBest: 
                {
                saveQuality = CVedMovie::EQualityResolutionHigh;
                break;                    
                }
            case TVeiSettings::EAuto: default:
                {
                saveQuality = CVedMovie::EQualityAutomatic;
                break;                    
                }
            }

        iMovie->SetQuality( saveQuality );
        iBackupSaveQuality = saveQuality;

        if ( EProcessingMovieTrimMms == iWaitMode )
            {
            if ( saveQuality != CVedMovie::EQualityMMSInteroperability )
                {
                iMovie->SetQuality( CVedMovie::EQualityMMSInteroperability );
                SetNewTempFileNeeded( ETrue );
                }
            else
                {
                SetNewTempFileNeeded( EFalse );
                }
            }

        TInt err;
        LOGFMT( KVideoEditorLogFile, "CVeiEditVideoView::StartTempFileProcessingL 3, calling iMovie->Process(%S)", iTempFile );
        TRAP( err, iMovie->ProcessL( *iTempFile, * this ));
        LOGFMT( KVideoEditorLogFile, "CVeiEditVideoView::StartTempFileProcessingL 4, err:%d", err );
        if ( err )
            {
            Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_OPTIONS_BACK );
            Cba()->DrawDeferred();
            ShowGlobalErrorNote( err );
            }
        }
    else
    /* use old temp file*/
        {
        LOG( KVideoEditorLogFile, "CVeiEditVideoView::StartTempFileProcessingL() 5, -- OLD TEMP" );
        if ( EProcessingMovieSend == iWaitMode )
            {
            SendMovieL();
            }
        else if ( EProcessingMovieTrimMms == iWaitMode )
            {
            if ( iMovie->IsMovieMMSCompatible())
                {
                LOG( KVideoEditorLogFile, "CVeiEditVideoView::StartTempFileProcessingL 6, MMS COMPATIBLE" );

                TEntry entry;
                User::LeaveIfError( fs.Entry( *iTempFile, entry ));
                TInt tempFileSize = entry.iSize / 1024;
                TInt maxMmsSize = STATIC_CAST( CVeiEditVideoLabelNavi* , iEditLabel->DecoratedControl())->GetMaxMmsSize();

                if ( tempFileSize < maxMmsSize )
                    {
                    LOG( KVideoEditorLogFile, "CVeiEditVideoView::StartTempFileProcessingL() 7, -- MMS SIZE OK" );
                    iWaitMode = EProcessingMovieSend;
                    SendMovieL();
                    }
                else
                    {
                    SetNewTempFileNeeded( EFalse );

                    LOG( KVideoEditorLogFile, "CVeiEditVideoView::StartTempFileProcessingL() 8, -- TRIM MMS" );

                    TBuf8 < KMaxFileName > conv8Filename;
                    CnvUtfConverter::ConvertFromUnicodeToUtf8( conv8Filename, * iTempFile );
                    iMovie->UnregisterMovieObserver( iContainer );
                    iMovie->UnregisterMovieObserver( this );
                    AppUi()->ActivateLocalViewL( TUid::Uid( EVeiTrimForMmsView ), TUid::Uid( 0 ), conv8Filename );
                    }
                }
            else
                {
                LOG( KVideoEditorLogFile, "CVeiEditVideoView::StartTempFileProcessingL() 9, -- NO MMS COMPATIBLE" );

                TInt err;
                if ( EProcessingMovieTrimMms == iWaitMode )
                    {
                    iMovie->SetQuality( CVedMovie::EQualityMMSInteroperability );
                    }

                TRAP( err, iMovie->ProcessL( *iTempFile, * this ));

                if ( err )
                    {
                    ShowGlobalErrorNote( err );
                    }
                }
            }
        else if ( EProcessingMoviePreview == iWaitMode )
            {
            if ( iFullScreenSelected )
                {
                iContainer->SetBlackScreen( ETrue );
                iContainer->SetRect( AppUi()->ApplicationRect());
                }
            iContainer->SetSelectionMode( CVeiEditVideoContainer::EModePreview );
            iContainer->PlayVideoFileL( *iTempFile, iFullScreenSelected );
            }
        else if (( EProcessingMovieSave == iWaitMode ) || ( EProcessingMovieSaveThenQuit == iWaitMode ))
            {
            CFileMan* fileman = CFileMan::NewL( fs );
            CleanupStack::PushL( fileman );

            if ( iTempFile->Left( 1 ) == iSaveToFileName->Left( 1 ))
                {
                fileman->Rename( *iTempFile, * iSaveToFileName );
                }
            else
                {
                fileman->Move( *iTempFile, * iSaveToFileName );
                }

            CleanupStack::PopAndDestroy( fileman );

            HBufC* stringholder = StringLoader::LoadL( R_VEI_NOTE_VIDEO_SAVED, iEikonEnv );
            CleanupStack::PushL( stringholder );
            iWaitMode = ENotWaiting;
            ShowInformationNoteL( *stringholder );
            iMovieSavedFlag = ETrue;
            CleanupStack::PopAndDestroy( stringholder);

            UpdateMediaGalleryL();
            }
        }
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::StartTempFileProcessingL: Out" );
    }

void CVeiEditVideoView::InsertNewAudio()
    {
    Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_EMPTY );
    Cba()->DrawDeferred();

    iContainer->SetRecordedAudio( ETrue );

    TInt64 startTimeInt = 0;
    TInt64 durationInt = 1000000;
    TInt currIndex( 0 );

    if ((( iContainer->CursorLocation() == VideoEditor::ECursorOnAudio ) || 
         ( iContainer->CursorLocation() == VideoEditor::ECursorOnEmptyAudioTrack )) && 
         ( iMovie->AudioClipCount() > 0 ))
        {
        currIndex = iContainer->CurrentIndex() + 1;
        startTimeInt = iMovie->AudioClipEndTime( iContainer->CurrentIndex()).Int64();
        }
    else
        {
        currIndex = iMovie->AudioClipCount() - 1;
        if ( currIndex >= 0 )
            {
            startTimeInt = iMovie->AudioClipEndTime( currIndex ).Int64();
            }
        else
            {
            currIndex = 0;
            }
        }

    while ( currIndex < iMovie->AudioClipCount())
        {
        // safety margin: it is not possible to insert new audio if there is less than 1s between clips
        TInt64 adjustedClipStartTimeInt = iMovie->AudioClipStartTime( currIndex ).Int64() - 1000000;

        if ( startTimeInt > adjustedClipStartTimeInt )
            {
            startTimeInt = iMovie->AudioClipEndTime( currIndex ).Int64();
            currIndex++;
            }
        else
            {
            break;
            }
        }

    if ( currIndex < iMovie->AudioClipCount())
        {
        TInt64 endTimeInt = startTimeInt + durationInt;
        TInt64 nextStartTimeInt = iMovie->AudioClipStartTime( currIndex ).Int64();

        if ( endTimeInt > nextStartTimeInt )
            {
            durationInt = nextStartTimeInt - startTimeInt;
            }
        }

    iContainer->SetRecordedAudioStartTime( TTimeIntervalMicroSeconds( startTimeInt ));
    iContainer->SetRecordedAudioDuration( TTimeIntervalMicroSeconds( durationInt ));
    iContainer->SetSelectionMode( CVeiEditVideoContainer::EModeRecordingSetStart );
    iContainer->SetCursorLocation( VideoEditor::ECursorOnAudio );

    UpdateEditNaviLabel();

    /* Create temp file. */
    if ( iTempRecordedAudio )
        {
        delete iTempRecordedAudio;
        iTempRecordedAudio = NULL;
        }

    iTempRecordedAudio = HBufC::NewL( KMaxFileName );
    iTempMaker->GenerateTempFileName( *iTempRecordedAudio, iMemoryInUse, iMovie->Format(), ETrue );

    /* Open the file, this is asynchronous so we'll come to our callback. */
    iRecorder->OpenFileL( *iTempRecordedAudio );
    }

TBool CVeiEditVideoView::IsEnoughFreeSpaceToSaveL( TInt aBytesToAdd )const
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::IsEnoughFreeSpaceToSaveL: in" );
    RFs& fs = iEikonEnv->FsSession();

    TBool spaceBelowCriticalLevel( EFalse );

    TInt sizeEstimate = iMovie->GetSizeEstimateL();
    sizeEstimate += aBytesToAdd;

    if ( iEditorState == EQuickPreview )
        {
        TInt snapShotSize = iContainer->SnapshotSize();
        if ( snapShotSize != 0 )
            {
            sizeEstimate = snapShotSize;
            }
        }

    LOGFMT( KVideoEditorLogFile, "CVeiEditVideoView::IsEnoughFreeSpaceToSaveL: 2, needed space: %d", sizeEstimate );

    if ( iMemoryInUse == CAknMemorySelectionDialog::EPhoneMemory )
        {
        LOG( KVideoEditorLogFile, "CVeiEditVideoView::IsEnoughFreeSpaceToSaveL: " );
        spaceBelowCriticalLevel = SysUtil::DiskSpaceBelowCriticalLevelL( &fs, sizeEstimate, EDriveC );
        }
    else
        {
        LOG( KVideoEditorLogFile, "\tMmc selected" );

        spaceBelowCriticalLevel = SysUtil::MMCSpaceBelowCriticalLevelL( &fs, sizeEstimate );
        }

    if ( spaceBelowCriticalLevel )
        {
        LOG( KVideoEditorLogFile, "CVeiEditVideoView::IsEnoughFreeSpaceToSaveL: 3, space is below critical level" );
        ShowErrorNote( R_VEI_MEMORY_RUNNING_OUT );
        }

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::IsEnoughFreeSpaceToSaveL out" );

    return !spaceBelowCriticalLevel;
    }


TBool CVeiEditVideoView::IsEnoughFreeSpaceToSave2L( TInt aBytesToAdd )const
    {
    RFs& fs = iEikonEnv->FsSession();
    TBool spaceBelowCriticalLevel( EFalse );
    TInt sizeEstimate = iMovie->GetSizeEstimateL();
    sizeEstimate += aBytesToAdd;

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
        return EFalse;
        }
    }

TBool CVeiEditVideoView::FitsToMmsL()
    {
    /*
    Read documentation of FitsToMmsL() in the header file
     */
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::FitsToMmsL() in" );
    TInt maxMmsSize = STATIC_CAST( CVeiEditVideoLabelNavi* , iEditLabel->DecoratedControl())->GetMaxMmsSize();

    CVedMovie::TVedMovieQuality origQuality = iMovie->Quality();
    iMovie->SetQuality( CVedMovie::EQualityMMSInteroperability );
    TInt videoTimeLineSize = iMovie->GetSizeEstimateL() / 1024;
    iMovie->SetQuality( origQuality );
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::FitsToMmsL() out" );
    videoTimeLineSize = videoTimeLineSize * 1.1;
    return videoTimeLineSize < maxMmsSize;
    }

void CVeiEditVideoView::BrowseStartTransition( TBool aUpOrDown )
    {
    TInt currentEffect;
    TInt currentIndex = iContainer->CurrentIndex();

    if ( currentIndex == 0 )
        {
        currentEffect = iMovie->StartTransitionEffect();
        if ( aUpOrDown )
            {
            if ( !( TVedStartTransitionEffect( currentEffect - 1 ) < EVedStartTransitionEffectNone ))
                {
                iMovie->SetStartTransitionEffect( TVedStartTransitionEffect( currentEffect - 1 ));
                }
            else
                {
                iMovie->SetStartTransitionEffect( TVedStartTransitionEffect( EVedStartTransitionEffectLast - 1 ));
                }
            }
        else
            {
            if ( !( TVedStartTransitionEffect( currentEffect + 1 ) >= EVedStartTransitionEffectLast ))
                {
                iMovie->SetStartTransitionEffect( TVedStartTransitionEffect( currentEffect + 1 ));
                }
            else
                {
                iMovie->SetStartTransitionEffect( TVedStartTransitionEffect( EVedStartTransitionEffectNone ));
                }
            }
        }
    else if ( currentIndex < iMovie->VideoClipCount())
        {
        currentEffect = iMovie->MiddleTransitionEffect( currentIndex - 1 );

        if ( aUpOrDown )
            {
            if ( !( TVedMiddleTransitionEffect( currentEffect - 1 ) < EVedMiddleTransitionEffectNone ))
                {
                iMovie->SetMiddleTransitionEffect( TVedMiddleTransitionEffect( currentEffect - 1 ), currentIndex - 1 );
                }
            else
                {
                iMovie->SetMiddleTransitionEffect( TVedMiddleTransitionEffect( EVedMiddleTransitionEffectLast - 1 ), currentIndex - 1 );
                }
            }
        else
            {
            if ( !( TVedMiddleTransitionEffect( currentEffect + 1 ) >= EVedMiddleTransitionEffectLast ))
                {
                iMovie->SetMiddleTransitionEffect( TVedMiddleTransitionEffect( currentEffect + 1 ), currentIndex - 1 );
                }
            else
                {
                iMovie->SetMiddleTransitionEffect( TVedMiddleTransitionEffect( EVedMiddleTransitionEffectNone ), currentIndex - 1 );
                }
            }
        }
    else
        {
        currentEffect = iMovie->EndTransitionEffect();

        if ( aUpOrDown )
            {
            if ( !( TVedEndTransitionEffect( currentEffect - 1 ) < EVedEndTransitionEffectNone ))
                {
                iMovie->SetEndTransitionEffect( TVedEndTransitionEffect( currentEffect - 1 ));
                }
            else
                {
                iMovie->SetEndTransitionEffect( TVedEndTransitionEffect( EVedEndTransitionEffectLast - 1 ));
                }
            }
        else
            {
            if ( !( TVedEndTransitionEffect( currentEffect + 1 ) >= EVedEndTransitionEffectLast ))
                {
                iMovie->SetEndTransitionEffect( TVedEndTransitionEffect( currentEffect + 1 ));
                }
            else
                {
                iMovie->SetEndTransitionEffect( TVedEndTransitionEffect( EVedEndTransitionEffectNone ));
                }
            }
        }

    }

void CVeiEditVideoView::ShowErrorNote( const TInt aResourceId, TInt /*aError*/ )const
    {
    CVeiErrorUI::ShowErrorNote( *iEikonEnv, aResourceId );
    }

void CVeiEditVideoView::ShowGlobalErrorNote( const TInt aError )const
    {
    iErrorUI->ShowGlobalErrorNote( aError );
    }

void CVeiEditVideoView::ShowInformationNoteL( const TDesC& aMessage )const
    {
    CAknInformationNote* note = new( ELeave )CAknInformationNote( ETrue );
    note->ExecuteLD( aMessage );
    }

void CVeiEditVideoView::ShowVolumeLabelL( TInt aVolume )
    {
    STATIC_CAST( CVeiTimeLabelNavi* , iPreviewLabel->DecoratedControl())->SetVolumeIconVisibilityL( ETrue );

    if ( iVolumeHider && iVolumeHider->IsActive())
        {
        iVolumeHider->Cancel();
        }
    if ( aVolume == 0 )
        {
        STATIC_CAST( CVeiTimeLabelNavi* , iPreviewLabel->DecoratedControl())->SetVolumeIconVisibilityL( EFalse );
        HideVolume();
        return ;
        }

    iNaviPane->PushL( *iVolumeNavi );
    if ( !iVolumeHider )
        {
        iVolumeHider = CPeriodic::NewL( CActive::EPriorityLow );
        }
    iVolumeHider->Start( 1000000, 1000000, TCallBack( CVeiEditVideoView::HideVolumeCallbackL, this ));

    STATIC_CAST( CAknVolumeControl* , iVolumeNavi->DecoratedControl())->SetValue( aVolume );

    if ( aVolume > KMinVolume + 1 )
        {
        STATIC_CAST( CVeiTimeLabelNavi* , iPreviewLabel->DecoratedControl())->SetLeftArrowVisibilityL( ETrue );
        }
    else
        {
        STATIC_CAST( CVeiTimeLabelNavi* , iPreviewLabel->DecoratedControl())->SetLeftArrowVisibilityL( EFalse );
        }

    if ( aVolume < KMaxVolume )
        {
        STATIC_CAST( CVeiTimeLabelNavi* , iPreviewLabel->DecoratedControl())->SetRightArrowVisibilityL( ETrue );
        }
    else
        {
        STATIC_CAST( CVeiTimeLabelNavi* , iPreviewLabel->DecoratedControl())->SetRightArrowVisibilityL( EFalse );
        }

    }

void CVeiEditVideoView::RemoveCurrentClipL()
    {
    if ( !iContainer )
        {
        return ;
        }
    TParse fp;

    if ( iContainer->CursorLocation() == VideoEditor::ECursorOnAudio )
        {
        if ( iMovie->AudioClipCount() == 0 )
            {
            return ;
            }
        CVedAudioClipInfo* audioclipinfo = iMovie->AudioClipInfo( iContainer->CurrentIndex());
        fp.Set( audioclipinfo->FileName(), NULL, NULL );
        }
    else
        {
        if ( iMovie->VideoClipCount() == 0 )
            {
            return ;
            }

        /* Get filename to remove query. */
        TBool isFile( iMovie->VideoClipInfo( iContainer->CurrentIndex())->Class() == EVedVideoClipClassFile );
        CVedVideoClipInfo* videoclipinfo = iMovie->VideoClipInfo( iContainer->CurrentIndex());

        if ( isFile )
            {
            fp.Set( videoclipinfo->FileName(), NULL, NULL );
            }
        else
            {
            fp.Set( videoclipinfo->DescriptiveName(), NULL, NULL );
            }
        }

    HBufC* stringholder = StringLoader::LoadL( R_VEI_REMOVE_CLIP_QUERY, fp.Name(), iEikonEnv );
    CleanupStack::PushL( stringholder );
    CAknQueryDialog* dlg = new( ELeave )CAknQueryDialog( *stringholder, CAknQueryDialog::ENoTone );
    TInt queryok = dlg->ExecuteLD( R_VEI_CONFIRMATION_QUERY );
    CleanupStack::PopAndDestroy( stringholder );

    if ( queryok )
        {
        TUint currentIndex = iContainer->GetAndDecrementCurrentIndex();

        if ( iContainer->CursorLocation() == VideoEditor::ECursorOnAudio )
            {
            iMovie->RemoveAudioClip( currentIndex );
            }
        else if ( iContainer->CursorLocation() == VideoEditor::ECursorOnClip )
            {
            iMovie->RemoveVideoClip( currentIndex );
            }
        else
            {
            ShowErrorNote( R_VEI_ERROR_NOTE );
            }
        }
    }


TInt CVeiEditVideoView::AddClipL( const TDesC& aFilename, TBool aStartNow )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::AddClipL: In" );

    if ( !iContainer )
        {
        iContainer = new( ELeave )CVeiEditVideoContainer( *iMovie, * this );
        iContainer->SetMopParent( this );
        iContainer->ConstructL( AppUi()->ClientRect());
        AppUi()->AddToStackL( *this, iContainer );
        }

    iMediaQueue->InsertMediaL( aFilename );

    if ( aStartNow )
        {
        iMediaQueue->StartProcessingL();
        }

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::AddClipL: Out" );
    return KErrNone;
    }


void CVeiEditVideoView::AddNext()
    {
    iMediaQueue->GetNext();
    }

TBool CVeiEditVideoView::SaveL( TWaitMode aQuitAfterSaving )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::SaveL: In" );
    RFs& fs = iEikonEnv->FsSession();

    TParse file;
    TFileName newname;

    // Get default movie name from settings view	
    CAknMemorySelectionDialog::TMemory memory( iMovieSaveSettings.MemoryInUse());

    if ( memory == CAknMemorySelectionDialog::EPhoneMemory )
        {
        newname = PathInfo::PhoneMemoryRootPath();
        }
    else
        {
        newname = PathInfo::MemoryCardRootPath();
        }

    newname.Append( PathInfo::VideosPath());

    TVedVideoFormat movieQuality = iMovie->Format();
    if ( movieQuality == EVedVideoFormatMP4 )
        {
        newname.Append( KExtMp4 );
        }
    else
        {
        newname.Append( KExt3gp );
        }

    file.Set( iMovieSaveSettings.DefaultVideoName(), &newname, NULL );

    TInt error( KErrNone );
    error = fs.MkDirAll( file.DriveAndPath());

    if (( error != KErrAlreadyExists ) && ( error != KErrNone ))
        {
        return EFalse;
        }

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::SaveL: 2" );
    if ( IsEnoughFreeSpaceToSaveL())
        {
        LOG( KVideoEditorLogFile, "CVeiEditVideoView::SaveL: 3" );
        //FileNameQuery
        newname.Zero();
        newname.Append( file.FullName());

        CApaApplication::GenerateFileName( fs, newname );
        CAknFileNamePromptDialog* dlg = CAknFileNamePromptDialog::NewL();
        CleanupStack::PushL( dlg );

        HBufC* filenametitle = StringLoader::LoadLC( R_VEI_QUERY_FILE_NAME, iEikonEnv );

        dlg->SetTitleL( *filenametitle );
        CleanupStack::PopAndDestroy( filenametitle );

        TBool namegiven = dlg->ExecuteL( newname );
        CleanupStack::PopAndDestroy( dlg );

        if ( namegiven )
            {
            newname.Insert( 0, file.DriveAndPath());

            file.Set( newname, NULL, NULL );

            if ( BaflUtils::FileExists( fs, newname ))
                {
                TBool overWrite;
                CAknQueryDialog* queryDlg;

                HBufC* overWriteConfirmationString;
                overWriteConfirmationString = StringLoader::LoadLC( R_VEI_CONFIRM_OVERWRITE, file.Name(), iEikonEnv );
                queryDlg = new( ELeave )CAknQueryDialog( *overWriteConfirmationString, CAknQueryDialog::ENoTone );
                overWrite = queryDlg->ExecuteLD( R_VEI_CONFIRMATION_QUERY );

                CleanupStack::PopAndDestroy( overWriteConfirmationString );
                if ( !overWrite )
                    {
                    return EFalse;
                    }
                }

            iWaitMode = aQuitAfterSaving;

            if ( iSaveToFileName )
                {
                delete iSaveToFileName;
                iSaveToFileName = NULL;
                }

            iSaveToFileName = HBufC::NewL( newname.Length());
            *iSaveToFileName = newname;
            LOGFMT( KVideoEditorLogFile, "CVeiEditVideoView::SaveL: 4, iSaveToFileName:%S", iSaveToFileName );
            StartTempFileProcessingL();

            return ETrue;
            }
        else
            {
            return EFalse;
            }
        }

    else
        {
        return EFalse;
        }
    }


// ----------------------------------------------------------------------------
// CVeiEditVideoView::CheckMemoryCardAvailability()
//
//  Checks the memory card availability if MMC is selected as save store in
//  application settings. An information note is shown in following
//  situations:
//  - MMC not inserted
//  - MMC corrupted (unformatted)
//  [- MMC is read-only (not implemented)]
//  
//  If note is popped up, this function waits until it's dismissed.
// ----------------------------------------------------------------------------
//
void CVeiEditVideoView::CheckMemoryCardAvailabilityL()
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::CheckMemoryCardAvailability(): in" );

    // Read the video editor settings from ini file.

    iChangedFromMMCToPhoneMemory = EFalse;

    TVeiSettings settings;
    STATIC_CAST( CVeiAppUi* , AppUi())->ReadSettingsL( settings );

    CAknMemorySelectionDialog::TMemory memoryInUse( settings.MemoryInUse());

    // Check the MMC accessibility only if MMC is used as saving store.
    if ( memoryInUse == CAknMemorySelectionDialog::EMemoryCard )
        {
        RFs& fs = iEikonEnv->FsSession();
        TDriveInfo driveInfo;

        User::LeaveIfError( fs.Drive( driveInfo, KMmcDrive ));

        // Media is not present (MMC card not inserted).
        if ( driveInfo.iType == EMediaNotPresent )
            {
            LOG( KVideoEditorLogFile, "CVeiEditVideoView::CheckMemoryCardAvailability(): no media" );
            iChangedFromMMCToPhoneMemory = ETrue;

            settings.MemoryInUse() = CAknMemorySelectionDialog::EPhoneMemory;
            STATIC_CAST( CVeiAppUi* , AppUi())->WriteSettingsL( settings );
            }
        // Media is present
        else
            {
            LOG( KVideoEditorLogFile, "CVeiEditVideoView::CheckMemoryCardAvailability(): media present" );

            TVolumeInfo volumeInfo;
            TInt volumeErr = fs.Volume( volumeInfo, KMmcDrive );
            LOGFMT( KVideoEditorLogFile, "CEditVideoView::CheckMemoryCardAvailability() Volume(): %d", volumeErr );

            // Show note if media is corrupted/unformatted.
            if ( volumeErr == KErrCorrupt )
                {
                HBufC* noteText = StringLoader::LoadLC( R_VED_MMC_NOT_INSERTED, iEikonEnv );
                CAknInformationNote* informationNote = new( ELeave )CAknInformationNote( ETrue );
                informationNote->ExecuteLD( *noteText );

                CleanupStack::PopAndDestroy( noteText );

                settings.MemoryInUse() = CAknMemorySelectionDialog::EPhoneMemory;
                STATIC_CAST( CVeiAppUi* , AppUi())->WriteSettingsL( settings );
                }
            }
        }

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::CheckMemoryCardAvailability(): out" );
    }

void CVeiEditVideoView::UpdateEditNaviLabel()const
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::UpdateEditNaviLabel(): In" );
    if ( !iContainer )
        {
        return ;
        }

    TRAPD( err, DoUpdateEditNaviLabelL());

    if ( err != KErrNone )
        {
        ShowGlobalErrorNote( err );
        }
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::UpdateEditNaviLabel(): In" );
    }

void CVeiEditVideoView::DoUpdateEditNaviLabelL()const
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::DoUpdateEditNaviLabelL(): In" );

    HBufC* buf;

    CAknNavigationDecorator* currentDecorator = iNaviPane->Top();

    iNaviPane->Pop( iMoveLabel );
    iNaviPane->Pop( iPreviewLabel );
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::DoUpdateEditNaviLabelL(): navi labels popped" );

    TBool leftArrowVisible = EFalse;
    TBool rightArrowVisible = EFalse;

    TInt currentIndex = iContainer->CurrentIndex();

    TInt test = iContainer->SelectionMode();

    // Draw the time indicators to the navi pane in Small preview state.
    // However, if the volume indicator is being show, do not draw the time label
    if ( iContainer->SelectionMode() == CVeiEditVideoContainer::EModePreview && !iFullScreenSelected && !iVolumeHider )
        {
        LOG( KVideoEditorLogFile, "CVeiEditVideoView::DoUpdateEditNaviLabelL(): iContainer->SelectionMode() == CVeiEditVideoContainer::EModePreview && !iFullScreenSelected && !iVolumeHider" );

        TTime elapsed( iContainer->PlaybackPositionL().Int64());
        TTime total( iContainer->TotalLength().Int64());

        TBuf < 16 > elapsedBuf;
        TBuf < 16 > totalBuf;

        HBufC* dateFormatString;

        // check if time is over 99:59
        if (( total.Int64() / 1000 ) < 3600000 ) 
            {
            dateFormatString = iEikonEnv->AllocReadResourceLC( R_QTN_TIME_DURAT_MIN_SEC );
            }
        else
            {
            dateFormatString = iEikonEnv->AllocReadResourceLC( R_QTN_TIME_DURAT_LONG );
            }

        elapsed.FormatL( elapsedBuf, * dateFormatString );
        total.FormatL( totalBuf, * dateFormatString );
        CleanupStack::PopAndDestroy( dateFormatString );

        CDesCArrayFlat* strings = new CDesCArrayFlat( 2 );
        CleanupStack::PushL( strings );
        strings->AppendL( elapsedBuf );
        strings->AppendL( totalBuf );
        HBufC* stringholder = StringLoader::LoadL( R_VEI_NAVI_TIME, * strings, iEikonEnv );
        CleanupStack::PopAndDestroy( strings );
        CleanupStack::PushL( stringholder );

        STATIC_CAST( CVeiTimeLabelNavi* , iPreviewLabel->DecoratedControl())->SetLabelL( stringholder->Des());

        CleanupStack::PopAndDestroy( stringholder );

        iNaviPane->PushL( *iPreviewLabel );
        }


    else if ( iContainer->SelectionMode() == CVeiEditVideoContainer::EModeMove )
        {
        LOG( KVideoEditorLogFile, "CVeiEditVideoView::DoUpdateEditNaviLabelL(): iContainer->SelectionMode() == CVeiEditVideoContainer::EModeMove" );

        if ( iContainer->CursorLocation() == VideoEditor::ECursorOnAudio )
            {
            buf = StringLoader::LoadLC( R_VEI_MOVE_AUDIO_NAVILABEL, iEikonEnv );

            TTimeIntervalMicroSeconds startTime = iMovie->AudioClipStartTime( currentIndex );

            if (( currentIndex == 0 ) && ( startTime > TTimeIntervalMicroSeconds( 0 )))
                {
                leftArrowVisible = ETrue;
                }
            else
                {
                if ( currentIndex >= 1 )
                    {
                    TTimeIntervalMicroSeconds prevEndTime = iMovie->AudioClipEndTime( currentIndex - 1 );
                    TTimeIntervalMicroSeconds modifiedStartTime = TTimeIntervalMicroSeconds( startTime.Int64() - 100000 );

                    if ( modifiedStartTime > prevEndTime )
                        {
                        leftArrowVisible = ETrue;
                        }
                    }
                }

            if ( currentIndex < ( iMovie->AudioClipCount() - 1 ))
                {
                TTimeIntervalMicroSeconds modifiedEndTime = TTimeIntervalMicroSeconds( iMovie->AudioClipEndTime( currentIndex ).Int64() + 100000 );
                TTimeIntervalMicroSeconds nextStartTime = iMovie->AudioClipStartTime( currentIndex + 1 );
                if ( modifiedEndTime < nextStartTime )
                    {
                    rightArrowVisible = ETrue;
                    }
                }

            if ( currentIndex == iMovie->AudioClipCount() - 1 )
                {
                rightArrowVisible = ETrue;
                }
            }
        else
            {
            buf = StringLoader::LoadLC( R_VEI_MOVE_VIDEO_NAVILABEL, iEikonEnv );

            if ( currentIndex > 0 )
                {
                leftArrowVisible = ETrue;
                }
            if ( currentIndex < iMovie->VideoClipCount() - 1 )
                {
                rightArrowVisible = ETrue;
                }
            }

        STATIC_CAST( CAknNaviLabel* , iMoveLabel->DecoratedControl())->SetTextL( *buf );

        CleanupStack::PopAndDestroy( buf );

        iMoveLabel->DrawNow();

        iMoveLabel->MakeScrollButtonVisible( ETrue );
        iMoveLabel->SetScrollButtonDimmed( CAknNavigationDecorator::ELeftButton, !leftArrowVisible );
        iMoveLabel->SetScrollButtonDimmed( CAknNavigationDecorator::ERightButton, !rightArrowVisible );

        iNaviPane->PushL( *iMoveLabel );
        }
    else if ( iContainer->SelectionMode() == CVeiEditVideoContainer::EModeDuration )
        {
        LOG( KVideoEditorLogFile, "CVeiEditVideoView::DoUpdateEditNaviLabelL(): iContainer->SelectionMode() == CVeiEditVideoContainer::EModeDuration" );

        buf = StringLoader::LoadLC( R_VEI_NAVI_PANE_DURATION, iEikonEnv );

        STATIC_CAST( CAknNaviLabel* , iMoveLabel->DecoratedControl())->SetTextL( *buf );

        CleanupStack::PopAndDestroy( buf );

        iMoveLabel->DrawNow();

        if ( currentDecorator )
            {
            iMoveLabel->MakeScrollButtonVisible( ETrue );

            if ( iContainer->CursorLocation() == VideoEditor::ECursorOnAudio )
                {
                TTimeIntervalMicroSeconds editedDuration = iMovie->AudioClipEditedDuration( currentIndex );
                TTimeIntervalMicroSeconds duration = iMovie->AudioClipInfo( currentIndex )->Duration();

                if ( editedDuration > TTimeIntervalMicroSeconds( 100000 ))
                    {
                    leftArrowVisible = ETrue;
                    }

                if ( editedDuration < duration )
                    {
                    rightArrowVisible = ETrue;
                    }
                }
            else
            // Cursor on generated video clip
                {
                TTimeIntervalMicroSeconds duration = iMovie->VideoClipInfo( currentIndex )->Duration();
                if ( duration > TTimeIntervalMicroSeconds( 100000 ))
                    {
                    leftArrowVisible = ETrue;
                    }
                rightArrowVisible = ETrue;
                }

            iMoveLabel->SetScrollButtonDimmed( CAknNavigationDecorator::ELeftButton, !leftArrowVisible );
            iMoveLabel->SetScrollButtonDimmed( CAknNavigationDecorator::ERightButton, !rightArrowVisible );
            iNaviPane->PushL( *iMoveLabel );
            }
        }
    else
        {
        LOG( KVideoEditorLogFile, "CVeiEditVideoView::DoUpdateEditNaviLabelL(): else-branch" );

        TInt maxMmsSize = STATIC_CAST( CVeiEditVideoLabelNavi* , iEditLabel->DecoratedControl())->GetMaxMmsSize();

        CVedMovie::TVedMovieQuality origQuality = iMovie->Quality();
        iMovie->SetQuality( CVedMovie::EQualityMMSInteroperability );

        TInt videoTimeLineSize = iMovie->GetSizeEstimateL() / 1024;

        iMovie->SetQuality( origQuality );

        // Navipanes MMS icon control. 
        if ( videoTimeLineSize < maxMmsSize )
            {
            STATIC_CAST( CVeiEditVideoLabelNavi* , iEditLabel->DecoratedControl())->SetMmsAvailableL( ETrue );
            }
        else
            {
            STATIC_CAST( CVeiEditVideoLabelNavi* , iEditLabel->DecoratedControl())->SetMmsAvailableL( EFalse );
            }

        TInt size = iMovie->GetSizeEstimateL() / 1024;
        /* If in recording state, show last audio clip end time+ recorded audio clip duration in navipane*/
        LOGFMT( KVideoEditorLogFile, "CVeiEditVideoView::DoUpdateEditNaviLabelL: iMovie->GetSizeEstimateL() OK : %d", size );
        TTimeIntervalMicroSeconds audioEndTime( 0 );
        if ((( iContainer->SelectionMode() == CVeiEditVideoContainer::EModeRecording ) || 
             ( iContainer->SelectionMode() == CVeiEditVideoContainer::EModeRecordingPaused )))
            {
            TInt audioCount = iMovie->AudioClipCount();
            /* check that cursor is on last audio clip */
            if (( audioCount > 0 ) && ( iContainer->CurrentIndex() == audioCount - 1 ))
                {
                audioEndTime = iMovie->AudioClipEndTime( audioCount - 1 );
                }
            audioEndTime = audioEndTime.Int64() + iContainer->RecordedAudioDuration().Int64();
            /*
             * Get recorded audio clip size and add it to engine size estimate
             * The size won't match with size estimate that engine gives when recorded audio clip is added,
             * 
             */
            RFs& fs = iEikonEnv->FsSession();
            TEntry entry;

            User::LeaveIfError( fs.Entry( *iTempRecordedAudio, entry ));
            TInt recordedClipSize = entry.iSize / 1024;
            size += recordedClipSize;
            }

        if ( audioEndTime > iMovie->Duration())
            {
            STATIC_CAST( CVeiEditVideoLabelNavi* , iEditLabel->DecoratedControl())->SetDurationLabelL( audioEndTime.Int64());
            }
        else
            {
            STATIC_CAST( CVeiEditVideoLabelNavi* , iEditLabel->DecoratedControl())->SetDurationLabelL( iMovie->Duration().Int64());
            LOGFMT( KVideoEditorLogFile, "CVeiEditVideoView::DoUpdateEditNaviLabelL: iMovie->Duration() OK : %Ld", iMovie->Duration().Int64());
            }

        // Video line size to navipane.
        STATIC_CAST( CVeiEditVideoLabelNavi* , iEditLabel->DecoratedControl())->SetSizeLabelL( size );

        LOGFMT( KVideoEditorLogFile, "CVeiEditVideoView::DoUpdateEditNaviLabelL: SetSizeLabelL(%d) OK", size );

        // Get default memory from settings view

        if ( iMemoryInUse == CAknMemorySelectionDialog::EPhoneMemory )
            {
            STATIC_CAST( CVeiEditVideoLabelNavi* , iEditLabel->DecoratedControl())->SetMemoryInUseL( ETrue );
            }
        else
            {
            STATIC_CAST( CVeiEditVideoLabelNavi* , iEditLabel->DecoratedControl())->SetMemoryInUseL( EFalse );
            }

        if ( IsEnoughFreeSpaceToSave2L())
            {
            STATIC_CAST( CVeiEditVideoLabelNavi* , iEditLabel->DecoratedControl())->SetMemoryAvailableL( ETrue );
            }
        else
            {
            STATIC_CAST( CVeiEditVideoLabelNavi* , iEditLabel->DecoratedControl())->SetMemoryAvailableL( EFalse );
            }
        if ( !currentDecorator )
            {
            iNaviPane->PushL( *iEditLabel );
            }
        }
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::DoUpdateEditNaviLabelL(): Out" );
    }

void CVeiEditVideoView::NotifyQueueProcessingStarted( MVeiQueueObserver::TProcessing aMode )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyQueueProcessingStarted: in" );

    if ( iProgressNote )
        {
        delete iProgressNote;
        iProgressNote = NULL;
        }

    if ( iWaitDialog )
        {
        CancelWaitDialog();
        }

    HBufC* stringholder;

    switch ( aMode )
        {
        case MVeiQueueObserver::EProcessingAudio: 

        iProgressNote = new( ELeave )CAknProgressDialog( REINTERPRET_CAST( CEikDialog** , &iProgressNote ), ETrue );

        iProgressNote->SetCallback( this );

        iProgressNote->ExecuteDlgLD( R_VEI_PROGRESS_NOTE_WITH_CANCEL );


        stringholder = StringLoader::LoadLC( R_VEI_PROGRESS_NOTE_INSERTING_AUDIO, iEikonEnv );
        iProgressNote->SetTextL( *stringholder );
        CleanupStack::PopAndDestroy( stringholder );

        iWaitMode = EOpeningAudioInfo;
        iProgressNote->GetProgressInfoL()->SetFinalValue( 100 );
        break;
        case MVeiQueueObserver::EProcessingVideo: 

        iWaitDialog = new( ELeave )CAknWaitDialog( REINTERPRET_CAST( CEikDialog** , &iWaitDialog ), ETrue );
        iWaitDialog->ExecuteLD( R_VEI_WAIT_DIALOG_INSERTING_VIDEO );
        break;
        default:
            iProgressNote = new( ELeave )CAknProgressDialog( REINTERPRET_CAST( CEikDialog** , &iProgressNote ), ETrue );

            iProgressNote->SetCallback( this );

            iProgressNote->ExecuteDlgLD( R_VEI_PROGRESS_NOTE );
            UpdateInsertingProgressNoteL( 1 );
            iProgressNote->GetProgressInfoL()->SetFinalValue( 100 );
            break;
        }

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyQueueProcessingStarted: out" );
    }

void CVeiEditVideoView::NotifyQueueProcessingProgressed( TInt aProcessedCount, TInt aPercentage )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyQueueProcessingProgressed: in" );
    if ( iProgressNote )
        {
        UpdateInsertingProgressNoteL( aProcessedCount );
        iProgressNote->GetProgressInfoL()->SetAndDraw( aPercentage );
        }
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyQueueProcessingProgressed: out" );
    }

TBool CVeiEditVideoView::NotifyQueueClipFailed( const TDesC& aFilename, TInt aError )
    {
    LOGFMT( KVideoEditorLogFile, "CVeiEditVideoView::NotifyQueueClipFailed: in, aError:%d", aError );

    TBool result;
    if ( aError == CVeiAddQueue::EInsertingSingleClip )
        {
        LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyQueueClipFailed: 1" );
        ShowErrorNote( R_VEI_VIDEO_FAILED );
        result = ETrue;
        }
    else
        {
        TBool continueProcessing;
        CAknQueryDialog* dlg;
        HBufC* failedContinueString;

        failedContinueString = StringLoader::LoadLC( R_VEI_WARNING_NOTE_INSERTING_FAILED, aFilename, iEikonEnv );
        dlg = new( ELeave )CAknQueryDialog( *failedContinueString, CAknQueryDialog::ENoTone );
        continueProcessing = dlg->ExecuteLD( R_VEI_CONFIRMATION_QUERY );

        CleanupStack::PopAndDestroy( failedContinueString );

        result = continueProcessing;
        }
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyQueueClipFailed: out" );

    return result;
    }

void CVeiEditVideoView::NotifyQueueEmpty( TInt /*aInserted*/, TInt DEBUGLOG_ARG( aFailed ))
    {
    LOGFMT( KVideoEditorLogFile, "CVeiEditVideoView::NotifyQueueEmpty: in, aFailed:%d", aFailed );
    if ( iProgressNote )
        {
        iProgressNote->GetProgressInfoL()->SetAndDraw( 100 );
        TRAP_IGNORE( iProgressNote->ProcessFinishedL());
        }
    if ( iWaitDialog )
        {
        CancelWaitDialog();
        }
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyQueueEmpty: out" );
    }


void CVeiEditVideoView::NotifyMovieProcessingStartedL( CVedMovie&  /*aMovie*/ )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyMovieProcessingStartedL: in" );

    iPercentProcessed = 0;

    iProgressNote = new( ELeave )CAknProgressDialog( REINTERPRET_CAST( CEikDialog** , &iProgressNote ), ETrue );
    iProgressNote->SetCallback( this );
    iProgressNote->ExecuteDlgLD( R_VEI_PROGRESS_NOTE_WITH_CANCEL );

    HBufC* stringholder;

    if (( EProcessingMovieSend == iWaitMode ) && ( KSenduiMtmBtUid == iGivenSendCommand ))
        {
        stringholder = StringLoader::LoadL( R_VEI_PROGRESS_NOTE_PROCESSING, iEikonEnv );
        }
    else if ( EProcessingMovieSend == iWaitMode )
        {
        stringholder = StringLoader::LoadL( R_VEI_PROGRESS_NOTE_SEND, iEikonEnv );
        }
    else if ( EProcessingMoviePreview == iWaitMode )
        {
        stringholder = StringLoader::LoadL( R_VEI_PROGRESS_NOTE_PROCESSING, iEikonEnv );
        }
    else if ( EProcessingMovieTrimMms == iWaitMode )
        {
        stringholder = StringLoader::LoadL( R_VEI_PROGRESS_NOTE_PROCESSING, iEikonEnv );
        }
    else
        {
        stringholder = StringLoader::LoadL( R_VEI_PROGRESS_NOTE_SAVING, iEikonEnv );
        }

    CleanupStack::PushL( stringholder );

    iProgressNote->SetTextL( *stringholder );
    CleanupStack::PopAndDestroy( stringholder );

    iProgressNote->GetProgressInfoL()->SetFinalValue( 100 );

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyMovieProcessingStartedL: out" );
    }

void CVeiEditVideoView::NotifyMovieProcessingProgressed( CVedMovie&  /*aMovie*/, TInt aPercentage )
    {
    iPercentProcessed = aPercentage;
    User::ResetInactivityTime();
    iProgressNote->GetProgressInfoL()->SetAndDraw( aPercentage );
    }

void CVeiEditVideoView::NotifyMovieProcessingCompleted( CVedMovie& aMovie, TInt aError )
    {
    LOGFMT2( KVideoEditorLogFile, "CVeiEditVideoView::NotifyMovieProcessingCompleted: in, aError:%d, iPercentProcessed:%d", aError, iPercentProcessed );

    aMovie.SetMovieSizeLimit( 0 ); // Movie size limit not in use

    if ( aError == KErrNone )
        {
        LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyMovieProcessingCompleted: 2" );
        Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_OPTIONS_BACK );
        Cba()->DrawDeferred();
        SetNewTempFileNeeded( EFalse );
        iProgressNote->GetProgressInfoL()->SetAndDraw( 100 );
        TRAP_IGNORE( iProgressNote->ProcessFinishedL());
        }
    else
        {
        LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyMovieProcessingCompleted: 3" );
        if ( iProgressNote )
            {
            LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyMovieProcessingCompleted: 4" );
            iWaitMode = EProcessingError;
            TRAP_IGNORE( iProgressNote->GetProgressInfoL()->SetAndDraw( 100 ));
            iErrorNmb = aError;
            TRAP_IGNORE( iProgressNote->ProcessFinishedL());
            }
        else
            {
            LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyMovieProcessingCompleted: 5" );
            if ( iTempFile )
                {
                LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyMovieProcessingCompleted: 6" );
                RFs& fs = iEikonEnv->FsSession();

                fs.Delete( *iTempFile );
                delete iTempFile;
                iTempFile = NULL;
                SetNewTempFileNeeded( ETrue );
                }
            }

        // SetEditorState() must be called because of its side effects eventhough state has not changed,
        // it sets CBAs. CBAs are set to empty before calling ProcessL()	
        if ( EMixAudio != iEditorState )
            {
            SetEditorState( EEdit );
            }
        else
            {
            SetEditorState( EMixAudio );
            }

        iContainer->SetBlackScreen( EFalse );
        iContainer->SetRect( AppUi()->ClientRect());
        }
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyMovieProcessingCompleted: out" );
    }

void CVeiEditVideoView::NotifyVideoClipAdded( CVedMovie&  /*aMovie*/, TInt aIndex )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyVideoClipAdded: in" );
    SetNewTempFileNeeded( ETrue );
    iMovieSavedFlag = EFalse;

    if ( iMovieFirstAddFlag )
        {
        CVedVideoClipInfo* info = iMovie->VideoClipInfo( aIndex );

        if ( EVedVideoClipClassGenerated != info->Class())
            {
            SetNewTempFileNeeded( EFalse );
            iMovieSavedFlag = ETrue;
            }
        iMovieFirstAddFlag = EFalse;
        }

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyVideoClipAdded: Completed" );

    STATIC_CAST( CVeiEditVideoLabelNavi* , iEditLabel->DecoratedControl())->SetState( CVeiEditVideoLabelNavi::EStateEditView );
    UpdateEditNaviLabel();
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyVideoClipAdded: out" );
    }

void CVeiEditVideoView::NotifyVideoClipAddingFailed( CVedMovie&  /*aMovie*/, TInt DEBUGLOG_ARG( aError ))
    {
    LOGFMT( KVideoEditorLogFile, "CVeiEditVideoView::NotifyVideoClipAddingFailed: in, aError:%d", aError );
    if ( iProgressNote )
        {
        iProgressNote->GetProgressInfoL()->SetAndDraw( 100 );
        TRAP_IGNORE( iProgressNote->ProcessFinishedL());
        }

    if ( iWaitDialog )
        {
        CancelWaitDialog();
        }
    UpdateEditNaviLabel();
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyVideoClipAddingFailed: out" );
    }

void CVeiEditVideoView::NotifyVideoClipRemoved( CVedMovie&  /*aMovie*/, TInt  /*aIndex*/ )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyVideoClipRemoved: in" );
    SetNewTempFileNeeded( ETrue );
    iMovieSavedFlag = EFalse;
    UpdateEditNaviLabel();
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyVideoClipRemoved: out" );
    }

void CVeiEditVideoView::NotifyVideoClipIndicesChanged( CVedMovie&  /*aMovie*/, TInt /*aOldIndex*/, TInt /*aNewIndex*/ )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyVideoClipIndicesChanged: in" );
    iMovieSavedFlag = EFalse;
    SetNewTempFileNeeded( ETrue );
    UpdateEditNaviLabel();
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyVideoClipIndicesChanged: out" );
    }

void CVeiEditVideoView::NotifyVideoClipTimingsChanged( CVedMovie&  /*aMovie*/, TInt /*aIndex*/ )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyVideoClipTimingsChanged: in" );
    SetNewTempFileNeeded( ETrue );
    iMovieSavedFlag = EFalse;
    UpdateEditNaviLabel();
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyVideoClipTimingsChanged: out" );
    }

void CVeiEditVideoView::NotifyVideoClipColorEffectChanged( CVedMovie&  /*aMovie*/, TInt /*aIndex*/ )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyVideoClipColorEffectChanged: in" );
    SetNewTempFileNeeded( ETrue );
    iMovieSavedFlag = EFalse;
    UpdateEditNaviLabel();
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyVideoClipColorEffectChanged: out" );
    }

void CVeiEditVideoView::NotifyVideoClipAudioSettingsChanged( CVedMovie&  /*aMovie*/, TInt /*aIndex*/ )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyVideoClipAudioSettingsChanged: in" );
    SetNewTempFileNeeded( ETrue );
    iMovieSavedFlag = EFalse;
    UpdateEditNaviLabel();
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyVideoClipAudioSettingsChanged: out" );
    }

void CVeiEditVideoView::NotifyStartTransitionEffectChanged( CVedMovie&  /*aMovie*/ )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyStartTransitionEffectChanged: in" );
    iMovieSavedFlag = EFalse;
    SetNewTempFileNeeded( ETrue );
    UpdateEditNaviLabel();
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyStartTransitionEffectChanged: out" );
    }

void CVeiEditVideoView::NotifyMiddleTransitionEffectChanged( CVedMovie&  /*aMovie*/, TInt /*aIndex*/ )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyMiddleTransitionEffectChanged: in" );
    iMovieSavedFlag = EFalse;
    SetNewTempFileNeeded( ETrue );
    UpdateEditNaviLabel();
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyMiddleTransitionEffectChanged: out" );
    }

void CVeiEditVideoView::NotifyEndTransitionEffectChanged( CVedMovie&  /*aMovie*/ )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyEndTransitionEffectChanged: in" );
    iMovieSavedFlag = EFalse;
    SetNewTempFileNeeded( ETrue );
    UpdateEditNaviLabel();
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyEndTransitionEffectChanged: out" );
    }

void CVeiEditVideoView::NotifyAudioClipAdded( CVedMovie& aMovie, TInt aIndex )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyAudioClipAdded: in" );
    iMovieSavedFlag = EFalse;
    SetNewTempFileNeeded( ETrue );

    if ( iProgressNote )
        {
        iProgressNote->GetProgressInfoL()->SetAndDraw( 100 );
        TRAP_IGNORE( iProgressNote->ProcessFinishedL());
        }

    iOriginalAudioClipIndex = iContainer->CurrentIndex();

    if ( iOriginalAudioClipIndex > aIndex )
        {
        iOriginalAudioClipIndex--;
        }
    iContainer->SetCurrentIndex( aIndex );

    iOriginalAudioStartPoint = TTimeIntervalMicroSeconds(  - 1 );
    CVedAudioClipInfo* audioclipinfo = aMovie.AudioClipInfo( aIndex );
    iOriginalAudioDuration = audioclipinfo->Duration();

    UpdateEditNaviLabel();
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyAudioClipAdded: out" );
    }

void CVeiEditVideoView::NotifyAudioClipAddingFailed( CVedMovie&  /*aMovie*/, TInt DEBUGLOG_ARG( aError ))
    {
    LOGFMT( KVideoEditorLogFile, "CVeiEditVideoView::NotifyAudioClipAddingFailed: in, aError:%d", aError );
    if ( iProgressNote )
        {
        iProgressNote->GetProgressInfoL()->SetAndDraw( 100 );
        TRAP_IGNORE( iProgressNote->ProcessFinishedL());
        }

    if ( iWaitDialog )
        {
        CancelWaitDialog();
        }
    ShowErrorNote( R_VEI_ERRORNOTE_AUDIO_INSERTING_FAILED );

    UpdateEditNaviLabel();
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyAudioClipAddingFailed: out" );
    }

void CVeiEditVideoView::NotifyAudioClipRemoved( CVedMovie&  /*aMovie*/, TInt  /*aIndex*/ )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyAudioClipRemoved: in" );
    if ( iTempRecordedAudio )
        {
        delete iTempRecordedAudio;
        iTempRecordedAudio = NULL;
        }
    iMovieSavedFlag = EFalse;
    SetNewTempFileNeeded( ETrue );
    UpdateEditNaviLabel();
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyAudioClipRemoved: out" );
    }

void CVeiEditVideoView::NotifyAudioClipIndicesChanged( CVedMovie&  /*aMovie*/, TInt /*aOldIndex*/, TInt /*aNewIndex*/ )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyAudioClipIndicesChanged: in" );
    iMovieSavedFlag = EFalse;
    SetNewTempFileNeeded( ETrue );
    UpdateEditNaviLabel();
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyAudioClipIndicesChanged: out" );
    }

void CVeiEditVideoView::NotifyAudioClipTimingsChanged( CVedMovie&  /*aMovie*/, TInt /*aIndex*/ )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyAudioClipTimingsChanged: in" );
    iMovieSavedFlag = EFalse;
    SetNewTempFileNeeded( ETrue );
    UpdateEditNaviLabel();
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyAudioClipTimingsChanged: out" );
    }

void CVeiEditVideoView::NotifyMovieReseted( CVedMovie&  /*aMovie*/ )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyMovieReseted: in" );
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyMovieReseted: out" );
    }

void CVeiEditVideoView::NotifyAudioClipInfoReady( CVedAudioClipInfo& aInfo, TInt aError )
    {
    LOGFMT( KVideoEditorLogFile, "CVeiEditVideoView::NotifyAudioClipInfoReady: in, aError:%d", aError );
    TInt err( KErrNone );

    if ( aError == KErrNone )
        {
        if ( aInfo.Type() == EVedAudioTypeUnrecognized )
            {
            LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyAudioClipInfoReady: 2: EVedAudioTypeUnrecognized" );

            aError = KErrNotSupported;

            /*TPtrC filename = aInfo.FileName();

            iTempRecordedAudio = HBufC::NewL(KMaxFileName);
            iTempMaker->GenerateTempFileName( *iTempRecordedAudio, iMovie );

            TRAP_IGNORE( iConverter->StartConversionL( filename, *iTempRecordedAudio) );	

            delete iTempRecordedAudio;
            iTempRecordedAudio = NULL;
             */
            }
        else if (( aInfo.Type() == EVedAudioTypeAMR ) || 
                 ( aInfo.Type() == EVedAudioTypeAMRWB ) || 
                 ( aInfo.Type() == EVedAudioTypeMP3 ) || 
                 ( aInfo.Type() == EVedAudioTypeAAC_LC ) || 
                 ( aInfo.Type() == EVedAudioTypeAAC_LTP ) || 
                 ( aInfo.Type() == EVedAudioTypeWAV ))
            {

            LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyAudioClipInfoReady: 3" );
            TInt index = 0;
            TInt64 startTimeInt = 0;
            TInt64 durationInt = aInfo.Duration().Int64();

            if ((( iContainer->CursorLocation() == VideoEditor::ECursorOnAudio ) || 
                 ( iContainer->CursorLocation() == VideoEditor::ECursorOnEmptyAudioTrack )) && 
                 ( iMovie->AudioClipCount() > 0 ))
                {
                index = iContainer->CurrentIndex() + 1;
                startTimeInt = iMovie->AudioClipEndTime( iContainer->CurrentIndex()).Int64();
                }

            while ( index < iMovie->AudioClipCount())
                {
                if ( TTimeIntervalMicroSeconds( startTimeInt ) == iMovie->AudioClipStartTime( index ))
                    {
                    startTimeInt = iMovie->AudioClipEndTime( index ).Int64();
                    index++;
                    }
                else
                    {
                    break;
                    }
                }

            if ( index < iMovie->AudioClipCount())
                {
                TInt64 endTimeInt = startTimeInt + durationInt;
                TInt64 nextStartTimeInt = iMovie->AudioClipStartTime( index ).Int64();
                if ( endTimeInt > nextStartTimeInt )
                    {
                    durationInt = nextStartTimeInt - startTimeInt;
                    }
                }
            LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyAudioClipInfoReady: 4" );
            TRAP( err, iMovie->AddAudioClipL( aInfo.FileName(), 
                                              TTimeIntervalMicroSeconds( startTimeInt ), 
                                              TTimeIntervalMicroSeconds( 0 ), 
                                              TTimeIntervalMicroSeconds( durationInt )));
            LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyAudioClipInfoReady: 5" );
            }
        }
    if (( aError != KErrNone ) || ( err != KErrNone ))
        {
        LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyAudioClipInfoReady: 6" );
        if ( aError != KErrCancel )
            {
            if ( aError )
                {
                iErrorNmb = aError;
                }
            else
                {
                iErrorNmb = err;
                }
            }
        iWaitMode = EProcessingAudioError;

        iProgressNote->GetProgressInfoL()->SetAndDraw( 100 );
        TRAP_IGNORE( iProgressNote->ProcessFinishedL());
        }
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyAudioClipInfoReady: out" );
    }



void CVeiEditVideoView::NotifyVideoClipGeneratorSettingsChanged( CVedMovie&  /*aMovie*/, TInt /*aIndex*/ )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyVideoClipGeneratorSettingsChanged: in" );
    if ( iWaitDialog )
        {
        iWaitDialog->ProcessFinishedL();
        }

    SetNewTempFileNeeded( ETrue );
    iMovieSavedFlag = EFalse;
    UpdateEditNaviLabel();
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyVideoClipGeneratorSettingsChanged: out" );
    }

void CVeiEditVideoView::NotifyVideoClipDescriptiveNameChanged( CVedMovie&  /*aMovie*/, TInt /*aIndex*/ )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyVideoClipDescriptiveNameChanged: in and out" );
    }

void CVeiEditVideoView::NotifyMovieQualityChanged( CVedMovie&  /*aMovie*/ )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyMovieQualityChanged: in" );

    // If there are text generator inserted into the movie, they need
    // to be notified that the movie resolution has changed so that
    // they can re-calculate the the wrapping etc. parameters.
    /*	TInt clipCount = iMovie->VideoClipCount();
    for (TInt i = 0; i < clipCount; i++)
    {
    CVedVideoClipInfo* clipInfo = iMovie->VideoClipInfo(i);
    if (clipInfo->Class() == EVedVideoClipClassGenerated)
    {
    TUid generatorUid = clipInfo->Generator()->Uid();
    if (generatorUid == KUidTitleClipGenerator)
    {
    CVeiTitleClipGenerator* generator = static_cast<CVeiTitleClipGenerator*>(clipInfo->Generator());
    generator->RefreshTextFrameParametersL();
    }
    }
    }*/

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyMovieQualityChanged: out" );
    }

void CVeiEditVideoView::NotifyMovieOutputParametersChanged( CVedMovie& /*aMovie*/ )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyMovieOutputParametersChanged: in and out" );
    SetNewTempFileNeeded( ETrue );
    iMovieSavedFlag = EFalse;
    }

void CVeiEditVideoView::NotifyAudioClipDynamicLevelMarkInserted( CVedMovie& /*aMovie*/, 
                                                                 TInt /*aClipIndex*/, 
                                                                 TInt /*aMarkIndex*/ )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyAudioClipDynamicLevelMarkInserted: in and out" );
    SetNewTempFileNeeded( ETrue );
    iMovieSavedFlag = EFalse;
    }

void CVeiEditVideoView::NotifyAudioClipDynamicLevelMarkRemoved( CVedMovie& /*aMovie*/, 
                                                                TInt /*aClipIndex*/, 
                                                                TInt /*aMarkIndex*/ )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyAudioClipDynamicLevelMarkRemoved: in and out" );
    SetNewTempFileNeeded( ETrue );
    iMovieSavedFlag = EFalse;
    }

void CVeiEditVideoView::NotifyVideoClipDynamicLevelMarkInserted( CVedMovie& /*aMovie*/, 
                                                                 TInt /*aClipIndex*/, 
                                                                 TInt /*aMarkIndex*/ )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyVideoClipDynamicLevelMarkInserted: in and out" );
    SetNewTempFileNeeded( ETrue );
    iMovieSavedFlag = EFalse;
    }

void CVeiEditVideoView::NotifyVideoClipDynamicLevelMarkRemoved( CVedMovie& /*aMovie*/, 
                                                                TInt /*aClipIndex*/, 
                                                                TInt /*aMarkIndex*/ )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyVideoClipDynamicLevelMarkRemoved: in and out" );
    SetNewTempFileNeeded( ETrue );
    iMovieSavedFlag = EFalse;
    }

void CVeiEditVideoView::MoscoStateChangeEvent( CBase* aObject, TInt aPreviousState, TInt aCurrentState, TInt aErrorCode )
    {
    LOGFMT3( KVideoEditorLogFile, "CVeiEditVideoView::MoscoStateChangeEvent: In: aPreviousState:%d, aCurrentState:%d, aErrorCode:%d", aPreviousState, aCurrentState, aErrorCode );

    if ( aObject == iRecorder )
        {
        if ( aErrorCode != KErrNone )
            {
            ShowErrorNote( R_VEI_RECORDING_FAILED );

            if ( iTempRecordedAudio )
                {
                TInt err = iEikonEnv->FsSession().Delete( *iTempRecordedAudio );
                if ( err ){

                }
                delete iTempRecordedAudio;
                iTempRecordedAudio = NULL;
                }

            iContainer->SetSelectionMode( CVeiEditVideoContainer::EModeNavigation );
            TRAP_IGNORE( Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_OPTIONS_BACK ));
            Cba()->DrawDeferred();
            UpdateEditNaviLabel();

            iRecorder->Close();
            }
        else if ( aCurrentState == CMdaAudioClipUtility::ERecording )
            {
            const TUint delay = 1000 * 1000 / 10;

            if ( !iAudioRecordPeriodic->IsActive())
                {
                iAudioRecordPeriodic->Start( delay, delay, TCallBack( CVeiEditVideoView::UpdateAudioRecording, this ));
                }

            Cba()->SetCommandSetL( R_VEI_SOFTKEYS_PAUSE_STOP );
            Cba()->DrawDeferred();
            }
        else if ( aPreviousState == CMdaAudioClipUtility::ENotReady )
            {
            TRAP_IGNORE( Cba()->SetCommandSetL( R_VEI_SOFTKEYS_RECORD_CANCEL ));
            Cba()->DrawDeferred();
            }
        }

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::MoscoStateChangeEvent: Out" );
    }


void CVeiEditVideoView::DoActivateL( const TVwsViewId& /*aPrevViewId*/, 
                                     TUid /*aCustomMessageId*/, 
                                     const TDesC8& /*aCustomMessage*/ )
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::DoActivateL: In" );
    if ( !iMemoryCardChecked )
        {
        CheckMemoryCardAvailabilityL();
        iMemoryCardChecked = ETrue;
        }

    if ( !iContainer )
        {
        iContainer = new( ELeave )CVeiEditVideoContainer( *iMovie, * this );
        iContainer->SetMopParent( this );
        iContainer->ConstructL( AppUi()->ClientRect());
        AppUi()->AddToStackL( *this, iContainer );
        }

    SetEditorState( EEdit );

    // Add Context Pane icon

    /*	TUid contextPaneUid;
    contextPaneUid.iUid = EEikStatusPaneUidContext;

    CEikStatusPane* sp = StatusPane();
    CEikStatusPaneBase::TPaneCapabilities subPane = sp->PaneCapabilities( contextPaneUid );

    if ( subPane.IsPresent() && subPane.IsAppOwned() )
    {	
    CAknContextPane* contextPane = (CAknContextPane*)sp->ControlL( contextPaneUid);

    TFileName mbmPath( VideoEditorUtils::IconFileNameAndPath(KManualVideoEditorIconFileId) );
    }*/

    // Quality is taken from settings and set to engine.
    STATIC_CAST( CVeiAppUi* , AppUi())->ReadSettingsL( iMovieSaveSettings );

    TInt settingsSaveQuality = iMovieSaveSettings.SaveQuality();
    CVedMovie::TVedMovieQuality saveQuality;

    switch ( settingsSaveQuality )
        {
        case TVeiSettings::EMmsCompatible: saveQuality = CVedMovie::EQualityMMSInteroperability;
        break;
        case TVeiSettings::EMedium: saveQuality = CVedMovie::EQualityResolutionMedium;
        break;
        case TVeiSettings::EBest: saveQuality = CVedMovie::EQualityResolutionHigh;
        break;
        case TVeiSettings::EAuto: default:
            saveQuality = CVedMovie::EQualityAutomatic;
            break;
        }

    iMovie->SetQuality( saveQuality );
    iMemoryInUse = iMovieSaveSettings.MemoryInUse();

    if (( EProcessingMovieForCutting == iWaitMode ) || 
        ( EProcessingMovieTrimMms == iWaitMode ) || 
        ( ECuttingAudio == iWaitMode ))
        {
        iMovie->RegisterMovieObserverL( this );
        iMovie->RegisterMovieObserverL( iContainer );
        }

    if ( EProcessingMovieForCutting == iWaitMode )
        {
        // miksi laitetaan jos ollaan oltu rimmaamassa?
        //SetNewTempFileNeeded(ETrue);

        TTimeIntervalMicroSeconds cutin;
        TTimeIntervalMicroSeconds cutout;
        cutin = iMovie->VideoClipCutInTime( iCutVideoIndex );
        cutout = iMovie->VideoClipCutOutTime( iCutVideoIndex );

        if (( cutin != TTimeIntervalMicroSeconds( 0 )) || 
            ( cutout != iMovie->VideoClipInfo( iCutVideoIndex )->Duration()))
            {
            iMovieSavedFlag = EFalse;
            }

        if (( cutin != iOriginalCutInTime ) || ( cutout != iOriginalCutOutTime ) )
            {
            SetNewTempFileNeeded( ETrue );
            iContainer->DrawNow();
            Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_EMPTY );
            iContainer->UpdateThumbnailL( iCutVideoIndex );
            }
        iContainer->SetCursorLocation( VideoEditor::ECursorOnClip );
        }
    else if ( ECuttingAudio == iWaitMode )
        {

        //SetNewTempFileNeeded(ETrue);

        TTimeIntervalMicroSeconds cutin;
        TTimeIntervalMicroSeconds cutout;
        cutin = iMovie->AudioClipCutInTime( iCutAudioIndex );
        cutout = iMovie->AudioClipCutOutTime( iCutAudioIndex );

        if (( cutin != TTimeIntervalMicroSeconds( 0 )) || 
            ( cutout != iMovie->AudioClipInfo( iCutAudioIndex )->Duration()))
            {
            iMovieSavedFlag = EFalse;
            }

        TTimeIntervalMicroSeconds currentEndTime;
        TTimeIntervalMicroSeconds nextStartTime;

        for ( TInt i = iContainer->CurrentIndex(); i < ( iMovie->AudioClipCount() - 1 ); i++ )
            {
            currentEndTime = iMovie->AudioClipEndTime( i );
            nextStartTime = iMovie->AudioClipStartTime( i + 1 );

            if ( nextStartTime < currentEndTime )
            // what is the reason behind?
            // is this a typo, should it be like this?:
            //if ( nextStartTime != currentEndTime )
                {
                nextStartTime = currentEndTime;
                iMovie->AudioClipSetStartTime( i + 1, nextStartTime );
                }
            }
        if (( cutin != iOriginalAudioCutInTime ) || ( cutout != iOriginalAudioCutOutTime ))
            {
            SetNewTempFileNeeded( ETrue );
            iContainer->DrawNow();
            }

        iContainer->SetCursorLocation( VideoEditor::ECursorOnAudio );
        }

    iNaviPane->PushL( *iEditLabel );

    iSendKey = EFalse;
    iWaitMode = ENotWaiting;

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::DoActivateL: out" );
    }

void CVeiEditVideoView::HandleForegroundEventL( TBool aForeground )
    {
    LOGFMT( KVideoEditorLogFile, "CVeiEditVideoView::HandleForegroundEventL: in: %d", aForeground );

    if ( !aForeground )
        {
        // If the application is closing down, calling PauseVideoL could result in 
        // a callback from the MMF player after the container is already deleted,
        // causing KERN-EXEC 3
        if ( static_cast < CVeiAppUi*  > ( AppUi())->AppIsOnTheWayToDestruction())
            {
            iContainer->PrepareForTerminationL();
            return ;
            }

        LOG( KVideoEditorLogFile, "CVeiEditVideoView::HandleForegroundEventL 1" );
        //if ( (EditorState() != EEdit ) && iContainer)
        if ( EPreview == EditorState() || EQuickPreview == EditorState())
            {
            iContainer->PauseVideoL();
            }
        // In phones with clamshell (läppäpuhelin) background can be activated with closing the shell
        // iContainer's OfferKeyEvent do not get this kind of shell events
        if ( EPreview == EditorState())
        // large preview
            {
            LOG( KVideoEditorLogFile, "CVeiEditVideoView::HandleForegroundEventL 2" );
            SetEditorState( CVeiEditVideoView::EEdit );
            iContainer->SetBlackScreen( EFalse );
            SetFullScreenSelected( EFalse );
            iContainer->StopVideo( ETrue );
            }

        if (( iContainer->SelectionMode() == CVeiEditVideoContainer::EModeRecordingPaused ) || ( iContainer->SelectionMode() == CVeiEditVideoContainer::EModeRecording ))
            {
            HandleCommandL( EVeiCmdEditVideoViewRecordStop );
            }
        DoDeactivate();
        }
    else
        {
        if ( EditorState() != EEdit )
            {
            if ( EMixAudio != EditorState() && EAdjustVolume != EditorState())
                {
                LOG( KVideoEditorLogFile, "CVeiEditVideoView::HandleForegroundEventL: 3, setting R_VEI_SOFTKEYS_PREVIEW_PLAY_BACK" );
                Cba()->SetCommandSetL( R_VEI_SOFTKEYS_PREVIEW_PLAY_BACK );
                }
            Cba()->DrawDeferred();
            }

        /* When view is activated some clips can be deleted or
        names can be changed.. So check that all video and audio clips are still available.*/
        if ( iMovie )
            {
            TInt i;
            TFileName clipName;
            RFs& fs = iEikonEnv->FsSession();

            for ( i = 0; i < iMovie->VideoClipCount(); i++ )
                {
                if ( iMovie->VideoClipInfo( i )->Class() == EVedVideoClipClassFile )
                    {
                    clipName = iMovie->VideoClipInfo( i )->FileName();

                    if ( !BaflUtils::FileExists( fs, clipName ))
                        {
                        iContainer->GetAndDecrementCurrentIndex();

                        iMovie->RemoveVideoClip( i );
                        i--;
                        UpdateMediaGalleryL();
                        }
                    clipName.Zero();
                    }
                }

            for ( i = 0; i < iMovie->AudioClipCount(); i++ )
                {
                clipName = iMovie->AudioClipInfo( i )->FileName();

                if ( !BaflUtils::FileExists( fs, clipName ))
                    {
                    iContainer->GetAndDecrementCurrentIndex();

                    iMovie->RemoveAudioClip( i );
                    i--;
                    UpdateMediaGalleryL();
                    }
                clipName.Zero();
                }

            }

        UpdateEditNaviLabel();
        HandleScreenDeviceChangedL();
        }

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::HandleForegroundEventL: out" );
    }

void CVeiEditVideoView::DoDeactivate()
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView:DoDeactivate: In" );

    iNaviPane->Pop( iEditLabel );

    if ( iVolumeHider )
        {
        iVolumeHider->Cancel();
        delete iVolumeHider;
        iVolumeHider = NULL;
        }

    LOG( KVideoEditorLogFile, "CVeiEditVideoView:DoDeactivate: Out" );
    }


TTimeIntervalMicroSeconds CVeiEditVideoView::OriginalAudioDuration()const
    {
    return iOriginalAudioDuration;
    }

TInt CVeiEditVideoView::UpdateNaviPreviewing( TAny* aThis )
    {
    STATIC_CAST( CVeiEditVideoView* , aThis )->DoUpdateEditNaviLabelL();
    return 1;
    }


TInt CVeiEditVideoView::UpdateAudioRecording( TAny* aThis )
    {
    STATIC_CAST( CVeiEditVideoView* , aThis )->DoUpdateAudioRecording();
    return 1;
    }

void CVeiEditVideoView::DoUpdateAudioRecording()
    {
    if ( iRecorder->State() != CMdaAudioClipUtility::ERecording )
        {
        iAudioRecordPeriodic->Cancel();
        Cba()->SetCommandSetL( R_VEI_SOFTKEYS_CONTINUE_STOP );
        Cba()->DrawDeferred();
        }
    else
        {
        TTimeIntervalMicroSeconds duration = iRecorder->Duration();

        iContainer->SetRecordedAudioDuration( duration );
        iContainer->DrawTrackBoxes();

        UpdateEditNaviLabel();

        // !!!*** Safety margin of 0.5s because cropping does not work, remove when cropping fixed. ***!!!
        duration = TTimeIntervalMicroSeconds( duration.Int64());

        if (( iRecordedAudioMaxDuration >= TTimeIntervalMicroSeconds( 0 )) && ( duration > iRecordedAudioMaxDuration ))
            {
            TRAP_IGNORE( HandleCommandL( EVeiCmdEditVideoViewRecordStop ));
            }
        }
    }

void CVeiEditVideoView::CancelWaitDialog( TInt aError )
    {
    LOGFMT( KVideoEditorLogFile, "CVeiEditVideoView::CancelWaitDialog: In, aError:%d", aError );
    if ( iWaitDialog )
        {
        LOG( KVideoEditorLogFile, "CVeiEditVideoView::CancelWaitDialog: 2" );
        iWaitDialog->ProcessFinishedL();
        }

    if ( aError )
        {
        LOG( KVideoEditorLogFile, "CVeiEditVideoView::CancelWaitDialog: 3" );
        ShowGlobalErrorNote( aError );
        LOG( KVideoEditorLogFile, "CVeiEditVideoView::CancelWaitDialog: 4" );
        }
    Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_OPTIONS_BACK );
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::CancelWaitDialog: 5" );
    Cba()->DrawDeferred();
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::CancelWaitDialog: Out" );
    }

void CVeiEditVideoView::NotifyImageClipGeneratorInitializationComplete( CVeiImageClipGenerator&  /*aGenerator*/, TInt aError )
    {
    LOGFMT( KVideoEditorLogFile, "CVeiEditVideoView::NotifyImageClipGeneratorInitializationComplete: in, aError:%d", aError );
    if ( aError != KErrNone )
        {
        ShowGlobalErrorNote( aError );
        delete iGenerator;
        iGenerator = 0;
        return ;
        }

    // insert the generator into movie
    TInt index = ( iContainer->CurrentIndex() == iMovie->VideoClipCount()) ? iMovie->VideoClipCount(): iContainer->CurrentIndex() + 1;
    iMovie->InsertVideoClipL( *iGenerator, ETrue, index );

    // Generator is no longer our concern
    iGenerator = 0;
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyImageClipGeneratorInitializationComplete: out" );
    }


void CVeiEditVideoView::NotifyTitleClipBackgroundImageLoadComplete( CVeiTitleClipGenerator&  /*aGenerator*/, TInt aError )
    {
    LOGFMT( KVideoEditorLogFile, "CVeiEditVideoView::NotifyTitleClipBackgroundImageLoadComplete: in, aError:%d", aError );
    if ( aError != KErrNone )
        {
        ShowGlobalErrorNote( aError );
        delete iGenerator;
        iGenerator = 0;
        return ;
        }
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyTitleClipBackgroundImageLoadComplete: out" );
    }

void CVeiEditVideoView::UpdateInsertingProgressNoteL( TInt aProcessed )
    {
    TInt queueCount = iMediaQueue->Count();

    CArrayFix < TInt > * numbers = new CArrayFixFlat < TInt > ( 2 );
    CleanupStack::PushL( numbers );
    numbers->AppendL( aProcessed );
    numbers->AppendL( queueCount );

    HBufC* stringholder = StringLoader::LoadL( R_VEI_PROGRESS_NOTE_INSERTING_MEDIA, * numbers, iEikonEnv );

    CleanupStack::PushL( stringholder );

    iProgressNote->SetTextL( *stringholder );
    iProgressNote->DrawNow(); // otherwise text is not drawn at all 
    CleanupStack::PopAndDestroy( stringholder );
    CleanupStack::PopAndDestroy( numbers );
    }

void CVeiEditVideoView::MmsSendCompatibleCheck()
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::MmsSendCompatibleCheck(): in" );

    TInt maxMmsSize = STATIC_CAST( CVeiEditVideoLabelNavi* , iEditLabel->DecoratedControl())->GetMaxMmsSize()* 1024;

    CVedMovie::TVedMovieQuality origQuality = iMovie->Quality();
    iMovie->SetQuality( CVedMovie::EQualityMMSInteroperability );
    TInt sizeEstimate = 0;
    TRAP_IGNORE( sizeEstimate = iMovie->GetSizeEstimateL());
    iMovie->SetQuality( origQuality );

    LOGFMT2( KVideoEditorLogFile, "CVeiEditVideoView::MmsSendCompatibleCheck(): maxMmsSize: %d, sizeEstimate: %d", maxMmsSize, sizeEstimate );

    TInt movieSizeLimit = ( TInt )( maxMmsSize* 0.9 );
    LOGFMT( KVideoEditorLogFile, "CVeiEditVideoView::MmsSendCompatibleCheck(): testing, test:%d", movieSizeLimit );

    if ( sizeEstimate < ( TInt )( maxMmsSize* 1.1 ))
        {
        LOG( KVideoEditorLogFile, "CVeiEditVideoView::MmsSendCompatibleCheck(): SetMovieSizeLimit..ok" );
        iMovie->SetMovieSizeLimit( movieSizeLimit );
        }

    TVeiSettings movieSaveSettings;

    if ( iMovie->IsMovieMMSCompatible())
        {
        LOG( KVideoEditorLogFile, "CVeiEditVideoView::MmsSendCompatibleCheck: MMSCompatible YES" );
        iWaitMode = EProcessingMovieTrimMms;
        StartTempFileProcessingL();
        }
    else
        {
        LOG( KVideoEditorLogFile, "CVeiEditVideoView::MmsSendCompatibleCheck: MMSCompatible NO" );
        iWaitMode = EProcessingMovieTrimMms;
        movieSaveSettings.SaveQuality() = TVeiSettings::EMmsCompatible;
        StartTempFileProcessingL();
        }
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::MmsSendCompatibleCheck(): out" );
    }

// Screen twisting 
/*
/* experimental code trying to fix this:
EECO-6W39YS 
Manual Video Editor: Wrong layout displays if switching phone mode during large preview playing

void CVeiEditVideoView::HandleScreenDeviceChangedL()
{	
LOG(KVideoEditorLogFile, "CVeiEditVideoView::HandleScreenDeviceChangedL() in");
if ( iContainer )
{
// Orientation changed. Resize container rect and update component
//positions.


//	iContainer->SetRect( ClientOrApplicationRect( iFullScreenSelected ) );
//	
//	iContainer->SetRect( AppUi()->ClientRect() );
//	

if(CVeiEditVideoContainer::EModePreview == iContainer->SelectionMode() && iFullScreenSelected &&
CVeiEditVideoContainer::EStatePlaying == iContainer->PreviewState())
{
LOG(KVideoEditorLogFile, "CVeiEditVideoView::HandleScreenDeviceChangedL() 1");
iContainer->SetBlackScreen( ETrue );
iContainer->SetRect( ClientOrApplicationRect( iFullScreenSelected ) );
return;	
}		
LOG(KVideoEditorLogFile, "CVeiEditVideoView::HandleScreenDeviceChangedL() 2");				
iContainer->SetCursorLocation( iContainer->CursorLocation() );
iContainer->ArrowsControl();

iContainer->DrawDeferred();
}		
LOG(KVideoEditorLogFile, "CVeiEditVideoView::HandleScreenDeviceChangedL() out");	
}
 */
/* Screen twisting */
void CVeiEditVideoView::HandleScreenDeviceChangedL()
    {
    if ( iContainer )
        {
        // Orientation changed. Resize container rect and update component
        //positions.
        iContainer->SetRect( AppUi()->ClientRect());

        iContainer->SetCursorLocation( iContainer->CursorLocation());
        iContainer->ArrowsControl();

        iContainer->DrawDeferred();
        }
    }

void CVeiEditVideoView::HideVolume()
    {
    iNaviPane->Pop( iVolumeNavi );

    delete iVolumeHider;
    iVolumeHider = NULL;
    }

TInt CVeiEditVideoView::HideVolumeCallbackL( TAny* aPtr )
    {
    CVeiEditVideoView* view = ( CVeiEditVideoView* )aPtr;
    view->HideVolume();
    return 0;
    }

void CVeiEditVideoView::UpdateMediaGalleryL()
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::UpdateMediaGalleryL(): In" );

  
    // Publish & Subscribe API used to make the saved file name available to AIW provider
    if ( iSaveToFileName )
        {
        LOG( KVideoEditorLogFile, "CVeiEditVideoView::UpdateMediaGalleryL(): Calling RProperty::Define(KUidVideoEditorProperties, VideoEditor::EPropertyFilename, RProperty::EText); " );
        TInt err = RProperty::Define( KUidVideoEditorProperties, VideoEditor::EPropertyFilename, RProperty::EText );
        if ( err != KErrAlreadyExists )
            {
            User::LeaveIfError( err );
            }
        User::LeaveIfError( RProperty::Set( KUidVideoEditorProperties, VideoEditor::EPropertyFilename, iSaveToFileName->Des()));
        }
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::UpdateMediaGalleryL(): Out" );
    }

void CVeiEditVideoView::SetFullScreenSelected( TBool aFullScreenSelected )
    {
    iFullScreenSelected = aFullScreenSelected;
    }

void CVeiEditVideoView::StartNaviPaneUpdateL()
    {
    if ( iPreviewUpdatePeriodic )
        {
        if ( iPreviewUpdatePeriodic->IsActive())
            {
            iPreviewUpdatePeriodic->Cancel();
            }

        iPreviewUpdatePeriodic->Start( 100000, 100000, TCallBack( CVeiEditVideoView::UpdateNaviPreviewing, this ));
        }
    }

void CVeiEditVideoView::ShowAndHandleSendMenuCommandsL()
    {
    /* Show send menu, postcard dimmed */
    CArrayFix < TUid > * mtmToDim = new( ELeave )CArrayFixFlat < TUid > ( 3 );
    TUid userSelection;
    CleanupStack::PushL( mtmToDim );
    /*
    this uid is empirically got with one device 19.10.2006
    there is currently (19.10.2006) no constans found in headers for Web Upload
     */

    const TInt KSenduiMtmOwnWebUploadIntValue = 536873429;
    const TUid KSenduiMtmOwnWebUpload = 
        {
        KSenduiMtmOwnWebUploadIntValue
    };

    mtmToDim->AppendL( KSenduiMtmPostcardUid );
    mtmToDim->AppendL( KSenduiMtmAudioMessageUid );
    mtmToDim->AppendL( KSenduiMtmOwnWebUpload );


    userSelection = iSendAppUi.ShowSendQueryL( NULL, TSendingCapabilities( 0, 0, TSendingCapabilities::ESupportsAttachments ), mtmToDim );
    CleanupStack::PopAndDestroy( mtmToDim );

    iGivenSendCommand = userSelection;

    if ( IsEnoughFreeSpaceToSaveL())
        {
        switch ( userSelection.iUid )
            {
            case KSenduiMtmSmtpUidValue:
            case KSenduiMtmImap4UidValue:
            case KSenduiMtmPop3UidValue:
                    {
                    LOGFMT( KVideoEditorLogFile, "CVeiEditVideoView::ShowAndHandleSendMenuCommandsL: MTM UID: %d", userSelection.iUid );
                    iWaitMode = EProcessingMovieSend;
                    StartTempFileProcessingL();
                    break;
                    }
            case KSenduiMtmIrUidValue:
                    {
                    LOG( KVideoEditorLogFile, "CVeiEditVideoView::ShowAndHandleSendMenuCommandsL: MTM UID: KSenduiMtmIrUidValue" );
                    iWaitMode = EProcessingMovieSend;
                    StartTempFileProcessingL();
                    break;
                    }
            case KSenduiMtmMmsUidValue:
                    {
                    LOG( KVideoEditorLogFile, "CVeiEditVideoView::ShowAndHandleSendMenuCommandsL: MTM UID: KSenduiMtmMmsUidValue" );
                    iWaitMode = EProcessingMovieTrimMms;
                    MmsSendCompatibleCheck();
                    break;
                    }
            case KSenduiMtmBtUidValue:
                    {
                    LOG( KVideoEditorLogFile, "CVeiEditVideoView::ShowAndHandleSendMenuCommandsL: MTM UID: KSenduiMtmBtUidValue" );
                    iWaitMode = EProcessingMovieSend;
                    StartTempFileProcessingL();
                    break;
                    }
                /*case KSenduiMtmOwnWebUploadIntValue:			// 0x200009D5
                {
                LOG(KVideoEditorLogFile, "CVeiEditVideoView::ShowAndHandleSendMenuCommandsL 6: MTM UID: 536873429");
                iWaitMode = EProcessingMovieSend;
                StartTempFileProcessingL();
                break;            	
                }	
                 */
            default:
                break;

            }
        }
    }

void CVeiEditVideoView::StopNaviPaneUpdateL()
    {
    DoUpdateEditNaviLabelL();

    if ( iPreviewUpdatePeriodic && iPreviewUpdatePeriodic->IsActive())
        {
        iPreviewUpdatePeriodic->Cancel();
        }
    }

TRect CVeiEditVideoView::ClientOrApplicationRect( TBool aFullScreenSelected )const
    {
    if ( aFullScreenSelected )
        {
        return AppUi()->ApplicationRect();
        }
    else
        {
        return AppUi()->ClientRect();
        }

    }

TBool CVeiEditVideoView::MixingConditionsOk()const
    {
    // prerequisites for sound mixing: at least one video with audio and one imported audio exist
    if ( iMovie->VideoClipCount() > 0 && iMovie->AudioClipCount() > 0 )
        {
        for ( TInt i = 0; i < iMovie->VideoClipCount(); i++ )
            {
            //if (iMovie->VideoClipEditedHasAudio(i))
            if ( iMovie->VideoClipInfo( i )->HasAudio())
                {
                return ETrue;
                }
            }
        }
    return EFalse;
    }

/*void CVeiEditVideoView::MixAudio()
{		

//TReal gainVideoNew(0);
//TReal gainAudioNew(0);

TInt gainVideoNew(0);
TInt gainAudioNew(0);

TInt gainVideoCurrent = iMovie->GetVideoClipVolumeGainL(KVedClipIndexAll);
TInt gainAudioCurrent = iMovie->GetAudioClipVolumeGainL(KVedClipIndexAll);

// video clips are faded      
if (iContainer->AudioMixingRatio() > 0)    
{
//@ : think how to tackle situations where value is form x.0, adding 0.5 gets wrong int
//Math::Round(fadevideo, iContainer->AudioMixingRatio()*(KVolumeMaxGain/10), 2);

gainVideoNew = iContainer->AudioMixingRatio()*(KVolumeMaxGain/10);    	
gainVideoNew += 0.5; // for making real to int rounding work in constructor of TVedDynamicLevelMark
gainVideoNew = 0 - gainVideoNew;    	
}
// audio clips are faded
else if (iContainer->AudioMixingRatio() < 0)    
{
//@ : think how to tackle situations where value is form x.0, adding 0.5 gets wrong int
//Math::Round(fadeaudio, iContainer->AudioMixingRatio()*(KVolumeMaxGain/10), 2);    	
gainAudioNew = iContainer->AudioMixingRatio()*(KVolumeMaxGain/10);     	    	    	    	
gainAudioNew -= 0.5;    // for making real to int rounding	work in constructor of TVedDynamicLevelMark    	
}        			    

if (gainVideoNew != gainVideoCurrent)
{
iMovie->SetVideoClipVolumeGainL(KVedClipIndexAll, gainVideoNew);
}
if (gainAudioNew != gainAudioCurrent)
{
iMovie->SetAudioClipVolumeGainL(KVedClipIndexAll, gainAudioNew);	
}    	    	    	
}
 */

void CVeiEditVideoView::MixAudio()
    {
    TReal fadevideo( 0 );
    TReal fadeaudio( 0 );

    // video clips are faded        
    if ( iContainer->AudioMixingRatio() > 0 )
        {
        //@ : think how to tackle situations where value is form x.0, adding 0.5 gets wrong int
        //Math::Round(fadevideo, iContainer->AudioMixingRatio()*(KVolumeMaxGain/10), 2);

        fadevideo = iContainer->AudioMixingRatio()*( KVolumeMaxGain / 10 );
        fadevideo += 0.5; // for making real to int rounding work in constructor of TVedDynamicLevelMark
        fadevideo = 0-fadevideo;
        fadeaudio = 0;
        }
    // audio clips are faded
    else if ( iContainer->AudioMixingRatio() < 0 )
        {
        //@ : think how to tackle situations where value is form x.0, adding 0.5 gets wrong int
        //Math::Round(fadeaudio, iContainer->AudioMixingRatio()*(KVolumeMaxGain/10), 2);

        fadeaudio = iContainer->AudioMixingRatio()*( KVolumeMaxGain / 10 );
        fadeaudio -= 0.5; // for making real to int rounding	work in constructor of TVedDynamicLevelMark
        fadevideo = 0;
        }

    // video clips are faded        
    if ( iContainer->AudioMixingRatio() > 0 && iMovie->VideoClipCount() > 0 )
        {
        TInt gain = iMovie->GetVideoClipVolumeGainL( KVedClipIndexAll ); ///(KVolumeMaxGain/10);
        TInt gainNew = ( TInt )fadevideo;
        if ( gainNew != gain )
            {
            iMovie->SetAudioClipVolumeGainL( KVedClipIndexAll, 0 );
            iMovie->SetVideoClipVolumeGainL( KVedClipIndexAll, gainNew );
            }
        }
    // audio clips are faded
    else if ( iContainer->AudioMixingRatio() < 0 && iMovie->AudioClipCount() > 0 )
        {
        TInt gain = iMovie->GetAudioClipVolumeGainL( KVedClipIndexAll ); ///(KVolumeMaxGain/10);
        TInt gainNew = ( TInt )fadeaudio;
        if ( gainNew != gain )
            {
            iMovie->SetVideoClipVolumeGainL( KVedClipIndexAll, 0 );
            iMovie->SetAudioClipVolumeGainL( KVedClipIndexAll, gainNew );
            }
        }
    else
    //if marks set back to position '0'
        {
        if ( 0 != iMovie->GetVideoClipVolumeGainL( KVedClipIndexAll ))
            {
            iMovie->SetVideoClipVolumeGainL( KVedClipIndexAll, 0 );
            }
        if ( 0 != iMovie->GetAudioClipVolumeGainL( KVedClipIndexAll ))
            {
            iMovie->SetAudioClipVolumeGainL( KVedClipIndexAll, 0 );
            }
        }
    }

void CVeiEditVideoView::AdjustVolumeL()
    {
    TReal adjustVolume = iContainer->Volume()*( KVolumeMaxGain / 10 );
    // to make rounding to int work correctly in constructor of TVedDynamicLevelMark
    // @ : if adjustvolume is x.0, rounding does not work, think how to fix problem
    if ( 0 < adjustVolume )
        {
        adjustVolume += 0.5;
        }
    else
        {
        adjustVolume -= 0.5;
        }

    if ( iContainer->CursorLocation() == VideoEditor::ECursorOnClip )
        {
        TReal currentVolume = iMovie->GetVideoClipVolumeGainL( iContainer->CurrentIndex()); ///(KVolumeMaxGain/10);						
        if ( 0 == iContainer->Volume())
            {
            if ( 0 != ( TInt )currentVolume )
                {
                iMovie->SetVideoClipVolumeGainL( iContainer->CurrentIndex(), 0 );
                }
            }
        else if (( TInt )currentVolume != ( TInt )adjustVolume )
            {
            iMovie->SetVideoClipVolumeGainL( iContainer->CurrentIndex(), ( TInt )adjustVolume );
            }
        }
    else if ( iContainer->CursorLocation() == VideoEditor::ECursorOnAudio )
        {
        TReal currentVolume = iMovie->GetAudioClipVolumeGainL( iContainer->CurrentIndex()); ///(KVolumeMaxGain/10);		
        if ( 0 == iContainer->Volume())
            {
            if ( 0 != ( TInt )currentVolume )
                {
                iMovie->SetAudioClipVolumeGainL( iContainer->CurrentIndex(), 0 );
                }
            }
        else if (( TInt )currentVolume != ( TInt )adjustVolume )
            {
            iMovie->SetAudioClipVolumeGainL( iContainer->CurrentIndex(), ( TInt )adjustVolume );
            }
        }
    }

void CVeiEditVideoView::StoreOrientation()
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::StoreOrientation: in" );

    iOriginalOrientation = AppUi()->Orientation();

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::StoreOrientation: out" );
    }

void CVeiEditVideoView::RestoreOrientation()
    {
    LOG( KVideoEditorLogFile, "CVeiEditVideoView::RestoreOrientation: in" );

    TRAP_IGNORE( AppUi()->SetOrientationL( iOriginalOrientation ));

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::RestoreOrientation: out" );
    }

void CVeiEditVideoView::SetNewTempFileNeeded( const TBool aUpdateNeeded )
    {
    iUpdateTemp = aUpdateNeeded;
    }

// End of File
