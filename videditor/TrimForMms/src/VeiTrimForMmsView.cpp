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



// INCLUDES
// System includes
#include <aknviewappui.h>
#include <akntitle.h>
#include <aknnavi.h>        // CAknNavigationControlContainer
#include <aknnavide.h>      // CAknNavigationDecorator
#include <AknProgressDialog.h>      // CAknProgressDialog
#include <aknsoundsystem.h>         // CAknKeySoundSystem
#include <bautils.h>        // BaflUtils
#include <barsread.h>
#include <eikprogi.h>       // CEikProgressInfo
#include <trimformms.rsg>
#include <utf.h>        // CnvUtfConverter
#include <sendui.h>     // CSendAppUi
#include <SenduiMtmUids.h>
#include <StringLoader.h>   // StringLoader 
#include <CMessageData.h>
// User includes
#include "TrimForMms.hrh"
#include "TrimForMmsAppUi.h"
#include "VeiSettings.h"
#include "VeiTrimForMmsView.h"
#include "VeiTrimForMmsContainer.h"
#include "VeiEditVideoLabelNavi.h"
#include "VideoEditorCommon.h"
#include "VeiTempMaker.h"
#include "VideoEditorCommon.h"
#include "VideoEditorDebugUtils.h"
#include "VeiErrorUi.h"

// CONSTANTS
const TInt KVedVideoClipIndex(0);
const TInt KProgressNoteMaxValue(100);
const TInt KVedTrimForMmsDefaultCba( R_AVKON_SOFTKEYS_OPTIONS_BACK );


CVeiTrimForMmsView* CVeiTrimForMmsView::NewL( CSendUi& aSendAppUi )
	{
    CVeiTrimForMmsView* self = CVeiTrimForMmsView::NewLC( aSendAppUi );
    CleanupStack::Pop( self );

    return self;
	}


CVeiTrimForMmsView* CVeiTrimForMmsView::NewLC( CSendUi& aSendAppUi )
	{
    CVeiTrimForMmsView* self = new (ELeave) CVeiTrimForMmsView( aSendAppUi );
    CleanupStack::PushL( self );
    self->ConstructL();

    return self;
	}

// -----------------------------------------------------------------------------
// CVeiTrimForMmsView::ConstructL
// Symbian 2nd phase constructor that can leave.
// -----------------------------------------------------------------------------
//
void CVeiTrimForMmsView::ConstructL()
    {
    BaseConstructL( R_VEI_TRIM_FOR_MMS_VIEW );

    iErrorUi = CVeiErrorUI::NewL( *iCoeEnv );

	iTempMaker = CVeiTempMaker::NewL();
    }

// -----------------------------------------------------------------------------
// CVeiTrimForMmsView::CVeiTrimForMmsView( CSendUi& aSendAppUi )
// C++ default constructor.
// -----------------------------------------------------------------------------
//
CVeiTrimForMmsView::CVeiTrimForMmsView( CSendUi& aSendAppUi ):
    iSendAppUi( aSendAppUi )
    {
    }


CVeiTrimForMmsView::~CVeiTrimForMmsView()
    {
    if( iContainer )
        {
        AppUi()->RemoveFromStack( iContainer );
        delete iContainer;
        }
    if( iNaviDecorator )
        {
        delete iNaviDecorator;
        }

    if( iVedMovie )
        {
		iVedMovie->UnregisterMovieObserver( this );
        delete iVedMovie;
        }

	if ( iTempMaker )
		{
		delete iTempMaker;
		iTempMaker = NULL;
		}
	if( iTempFile )
        {
		TInt err = CCoeEnv::Static()->FsSession().Delete( *iTempFile );
		if ( err ) 
			{
			// what to do when error occurs in destructor???
			}

        delete iTempFile;
        }
	
    delete iErrorUi;
    
    iProgressInfo = NULL;
    }


TUid CVeiTrimForMmsView::Id() const
    {
    return TUid::Uid( EVeiTrimForMmsView );
    }


// ----------------------------------------------------------------------------
// CVeiTrimForMmsView::HandleCommandL( TInt aCommand )
//
//  
// ----------------------------------------------------------------------------
//
void CVeiTrimForMmsView::HandleCommandL( TInt aCommand )
    {
    LOG(KVideoEditorLogFile, "CVeiTrimForMmsView::HandleCommandL: In");
	
	TInt state;
	state = iContainer->PreviewState();

    switch ( aCommand )
        {
        /**
         * Options -> Send via multimedia
         */
        case EVeiCmdSendViaMms:
            {
            CmdSendViaMultimediaL();
            break;
            }
        /**
         * Options -> Preview
         */
        case EVeiCmdPreview:
            {		
			if(	state == EIdle || state == EStop ) 
				{
				SetTrimStateL( EFullPreview );
				PlayPreviewL();
				}
            break;
            }
        //
        // Options -> Help
        //
        case EVeiCmdTrimForMmsViewHelp:
            {
            AppUi()->HandleCommandL( aCommand );
            break;
            }
        /**
         * Options -> Back
         */
        case EAknSoftkeyBack:
            {
			if( state != EFullPreview )
				{
				CmdSoftkeyBackL();
				}
			else
				{
				SetTrimStateL( ESeek );
				}
            break;
            }
        /**
         * Adjust video length -> Ok
         */
        case EAknSoftkeyOk:
            {
			if( state == EPause )
				{
				SetTrimStateL( ESeek );
				iContainer->Stop( ETrue );
				}
			else
				{	
				CmdSoftkeyOkL();  
				}
            break;
            }
        /**
         * Adjust video length -> Cancel
         */
        case EAknSoftkeyCancel:
            {
			if( state == EPause )
				{
				SetTrimStateL( ESeek );
				iContainer->Stop( ETrue );
				}
			else
				{	
				CmdSoftkeyCancelL();
				}
            break;
            }
        default:
            {
            AppUi()->HandleCommandL( aCommand );
            break;
            }
        }

    LOG(KVideoEditorLogFile, "CVeiTrimForMmsView::HandleCommandL: Out");
    }


// ----------------------------------------------------------------------------
// CVeiTrimForMmsView::CmdSendViaMultimediaL()
//  Function for handling the Send Via Multimedia command.
//  
// ----------------------------------------------------------------------------
//
void CVeiTrimForMmsView::CmdSendViaMultimediaL()
    {
    LOG(KVideoEditorLogFile, "CVeiTrimForMmsView::CmdSendViaMultimediaL: In");

        // Start processing the trimmed video
	    // Possible leave codes:
	    //	- KErrNoMemory if memory allocation fails
	    //	- KErrAccessDenied if the file access is denied
	    //	- KErrDiskFull if the disk is full
	    //	- KErrWrite if not all data could be written
	    //	- KErrBadName if the filename is bad
	    //  - KErrDirFull if the directory is full
        // : If video clip is already processed and frame is in same position
        //       do not reprocess the movie.


	UpdateNaviPaneSize();

	TBool fileExists( ETrue );
	RFs	fs = CCoeEnv::Static()->FsSession();

	if ( iTempFile )
		{
		fileExists = BaflUtils::FileExists( fs, *iTempFile );
		}

	if ( !fileExists || ( !iTempFile  ) || iProcessNeeded )
		{

		if ( iTempFile && fileExists )
			{
			User::LeaveIfError( fs.Delete( *iTempFile ) );
			delete iTempFile;
			iTempFile = NULL;
			}

		iTempFile = HBufC::NewL(KMaxFileName);

// @: check the quality setting. should we set it here to MMS compatible?

		// Quality is taken from settings and set to engine.
		ReadSettingsL( iMovieSaveSettings );

		iTempMaker->GenerateTempFileName( *iTempFile, iMovieSaveSettings.MemoryInUse(), iVedMovie->Format() );

		TEntry fileinfo;
		// Rename movie from xxxx.$$$ to defaultfilename from settingsview.
		// looks better in attachment list..

		// Get default movie name from settings view
		TPtr temppeet = iTempFile->Des();
		TParse parse;

		parse.Set( iMovieSaveSettings.DefaultVideoName(), &temppeet, NULL );

		TFileName orgPathAndName = parse.FullName();
		
//		TVedVideoFormat movieQuality = iVedMovie->Format();
		
		orgPathAndName.Replace( orgPathAndName.Length()-4, 4, KExt3gp );

		fs.Replace( *iTempFile, orgPathAndName );
		iTempFile->Des() = orgPathAndName;
		fs.Entry( *iTempFile, fileinfo );

				
		iVedMovie->SetQuality( CVedMovie::EQualityMMSInteroperability );
		LOGFMT(KVideoEditorLogFile, "CVeiTrimForMmsView::CmdSendViaMultimediaL: 1, iTempFile:%S", iTempFile);
        TRAPD( processError, iVedMovie->ProcessL( *iTempFile, *this ) );
		LOG(KVideoEditorLogFile, "CVeiTrimForMmsView::CmdSendViaMultimediaL: 2");
        if( processError == KErrNone )
            {
            // Text for the progress note is loaded from TBUF resource
            HBufC* noteText;
            noteText = StringLoader::LoadLC( R_VED_PROCESSING_FOR_MMS, iEikonEnv );

            // Construct and execute progress note.
            iProgressNote = new(ELeave) CAknProgressDialog( REINTERPRET_CAST(CEikDialog**, 
                                                             &iProgressNote), ETrue);
			iProgressNote->PrepareLC( R_VEI_PROGRESS_NOTE );
            iProgressNote->SetTextL( *noteText );
            
            iProgressInfo = iProgressNote->GetProgressInfoL();
			iProgressInfo->SetFinalValue( KProgressNoteMaxValue );
            
            iProgressNote->RunLD(); 

            CleanupStack::PopAndDestroy( noteText );    // Pop and destroy the text
            }
            else 
            {
            // : add error handling here
            }

        }
    else
        {
        
		RFs shareFServer;
	   	LOG(KVideoEditorLogFile, "CVeiTrimForMmsView::CmdSendViaMultimediaL: shareFServer connect.");

	 	User::LeaveIfError(shareFServer.Connect());
	    shareFServer.ShareProtected();
		
		RFile openFileHandle;
    	
		TInt err = openFileHandle.Open(shareFServer,*iTempFile,EFileRead|EFileShareReadersOnly);
		if (KErrNone != err)
			{
			LOGFMT(KVideoEditorLogFile, "CVeiTrimForMmsView::CmdSendViaMultimediaL: Could not open file %S with EFileShareReadersOnly. Trying EFileShareAny", iTempFile);
			User::LeaveIfError( openFileHandle.Open (shareFServer, *iTempFile, EFileRead|EFileShareAny) );
			}
		 
		CMessageData* messageData = CMessageData::NewLC();	
		 
		 
		messageData->AppendAttachmentHandleL( openFileHandle );

		iSendAppUi.CreateAndSendMessageL( KSenduiMtmMmsUid, messageData, KNullUid, EFalse );	
		
		CleanupStack::PopAndDestroy( messageData );
		 
        shareFServer.Close();
	   	LOG(KVideoEditorLogFile, "CVeiTrimForMmsView::CmdSendViaMultimediaL: shareFServer closed.");
        }

    LOG(KVideoEditorLogFile, "CVeiTrimForMmsView::CmdSendViaMultimediaL: Out");
    }

void CVeiTrimForMmsView::CmdSoftkeyBackL()
    {
    // Compare previous view's application uid to video editor uid.
    if( iPreviousViewId.iAppUid == KUidVideoEditor )
        {
	    CEikStatusPane* statusPane = AppUi()->StatusPane();
	    TUid naviPaneUid = TUid::Uid( EEikStatusPaneUidNavi );  // Navi pane UID

        CAknNavigationControlContainer *naviContainer = (CAknNavigationControlContainer*)statusPane->ControlL( naviPaneUid );
        naviContainer->Pop( iNaviDecorator );
        
        // Activate previous local view.
        AppUi()->ActivateLocalViewL( iPreviousViewId.iViewUid );
        }
    else
        {
        // Exit video editor
        AppUi()->HandleCommandL( EEikCmdExit);
        }
    }


void CVeiTrimForMmsView::CmdSoftkeyOkL()
    {
    // Set CBA labels back to view default
    Cba()->SetCommandSetL( KVedTrimForMmsDefaultCba );
    Cba()->DrawDeferred();

    PushKeySoundL( R_VED_LEFT_RIGHT_SILENT_SKEY_LIST );
    }


void CVeiTrimForMmsView::CmdSoftkeyCancelL()
    {
    // Set CBA labels back to view default
    Cba()->SetCommandSetL( KVedTrimForMmsDefaultCba );
    Cba()->DrawDeferred();

    PushKeySoundL( R_VED_LEFT_RIGHT_SILENT_SKEY_LIST );
    }


void CVeiTrimForMmsView::DoActivateL( const TVwsViewId& aPrevViewId,
                                        TUid /*aCustomMessageId*/,
                                        const TDesC8& aCustomMessage )
    {
    if( !iContainer ) 
        {
        iPreviousViewId = aPrevViewId;  // Save the previous view id

        // Disable left and right navi-key sounds
        PushKeySoundL( R_VED_LEFT_RIGHT_SILENT_SKEY_LIST );

        SetTitlePaneTextL();
        CreateNaviPaneL();
		
        STATIC_CAST( CVeiEditVideoLabelNavi*, iNaviDecorator->DecoratedControl() )->SetState( CVeiEditVideoLabelNavi::EStateTrimForMmsView );
        
        TFileName inputFileName;
       	CnvUtfConverter::ConvertToUnicodeFromUtf8( inputFileName, aCustomMessage );


		// TEST
		if (inputFileName.Length() == 0)
			{
			inputFileName.Copy( _L("c:\\Data\\Videos\\test.mp4") );
			}
		// END TEST

    
        if( !iVedMovie )
            {
            iVedMovie = CVedMovie::NewL( NULL );
            iVedMovie->RegisterMovieObserverL( this );
            iVedMovie->InsertVideoClipL( inputFileName, KVedVideoClipIndex );
			iVedMovie->SetQuality( CVedMovie::EQualityMMSInteroperability );
            }

        iContainer = CVeiTrimForMmsContainer::NewL( AppUi()->ClientRect(),
                                                    *iVedMovie, *this );
        iContainer->SetMopParent( this );
        
        AppUi()->AddToStackL( *this, iContainer );
        }

    }


void CVeiTrimForMmsView::DoDeactivate()
    {
    if( iContainer )
        {
        AppUi()->RemoveFromStack( iContainer );
        delete iContainer;
        iContainer = NULL;

        if( iVedMovie )
            {
            iVedMovie->Reset();
            iVedMovie->UnregisterMovieObserver( this );
             delete iVedMovie;
            iVedMovie = NULL;
            }
        }
    }


void CVeiTrimForMmsView::PushKeySoundL( const TInt aResourceId ) const
    {
    CAknKeySoundSystem* aknKeySoundSystem = AppUi()->KeySounds();
    aknKeySoundSystem->PushContextL( aResourceId );
    }


void CVeiTrimForMmsView::PopKeySound() const
    {
    AppUi()->KeySounds()->PopContext();
    }


void CVeiTrimForMmsView::SetTitlePaneTextL() const
    {
    TUid titleUid;
    titleUid.iUid = EEikStatusPaneUidTitle;

    CEikStatusPane* statusPane = AppUi()->StatusPane();

    CEikStatusPaneBase::TPaneCapabilities titlePaneCap = 
        statusPane->PaneCapabilities( titleUid );

    if( titlePaneCap.IsPresent() && titlePaneCap.IsAppOwned() )
        {
        CAknTitlePane* titlePane = 
            (CAknTitlePane*)statusPane->ControlL( titleUid );

		TResourceReader reader;
		iCoeEnv->CreateResourceReaderLC( reader,
            R_VEI_TRIM_FOR_MMS_VIEW_TITLE_NAME );
		titlePane->SetFromResourceL( reader );

        CleanupStack::PopAndDestroy(); //reader
        }

    }


void CVeiTrimForMmsView::CreateNaviPaneL()
    {
    TUid naviPaneUid = TUid::Uid( EEikStatusPaneUidNavi );  // Navi pane UID

    CEikStatusPane* statusPane = AppUi()->StatusPane();     // Get status pane

    CEikStatusPaneBase::TPaneCapabilities naviPaneCap =
        statusPane->PaneCapabilities( naviPaneUid );

    if( naviPaneCap.IsPresent() && naviPaneCap.IsAppOwned() )
        {
        CAknNavigationControlContainer *naviContainer = (CAknNavigationControlContainer*)statusPane->ControlL( naviPaneUid );
        
        CVeiEditVideoLabelNavi* editvideolabelnavi = CVeiEditVideoLabelNavi::NewLC();
        editvideolabelnavi->SetState( CVeiEditVideoLabelNavi::EStateInitializing );

        iNaviDecorator = 
            CAknNavigationDecorator::NewL( naviContainer, editvideolabelnavi,
                                       CAknNavigationDecorator::ENotSpecified );
        CleanupStack::Pop( editvideolabelnavi );

        iNaviDecorator->SetContainerWindowL( *naviContainer );		
	    iNaviDecorator->MakeScrollButtonVisible( EFalse );

        naviContainer->PushL( *iNaviDecorator );
 
        }
    }

void CVeiTrimForMmsView::UpdateNaviPaneSize()
    {
    if( iContainer )
        {
        iVedMovie->VideoClipSetCutInTime( KVedVideoClipIndex, iContainer->CutInTime() );
        iVedMovie->VideoClipSetCutOutTime( KVedVideoClipIndex, iContainer->CutOutTime() );
        }
    }
void CVeiTrimForMmsView::SetNaviPaneSizeLabelL( const TInt& aSizeInBytes )
    {
    STATIC_CAST(CVeiEditVideoLabelNavi*,
        iNaviDecorator->DecoratedControl())->SetSizeLabelL( aSizeInBytes );
    }


void CVeiTrimForMmsView::SetNaviPaneDurationLabelL(
                                    const TTimeIntervalMicroSeconds& aTime )
    {
    STATIC_CAST(CVeiEditVideoLabelNavi*, 
       iNaviDecorator->DecoratedControl())->SetDurationLabelL( aTime.Int64() );
    }

void CVeiTrimForMmsView::UpdateNaviPaneL( const TInt& aSizeInBytes, 
								const TTimeIntervalMicroSeconds& aTime )
	{ 
	TInt maxMmsSize = STATIC_CAST( CVeiEditVideoLabelNavi*, iNaviDecorator->DecoratedControl() )->GetMaxMmsSize();

		// Navipanes MMS icon control. 
	if( aSizeInBytes < maxMmsSize )
		{
		STATIC_CAST( CVeiEditVideoLabelNavi*, iNaviDecorator->DecoratedControl() )->SetMmsAvailableL( ETrue );
		}
	else 
		{
		STATIC_CAST( CVeiEditVideoLabelNavi*, iNaviDecorator->DecoratedControl() )->SetMmsAvailableL( EFalse );
		}

    STATIC_CAST(CVeiEditVideoLabelNavi*,
        iNaviDecorator->DecoratedControl())->SetSizeLabelL( aSizeInBytes );
	
    STATIC_CAST(CVeiEditVideoLabelNavi*, 
       iNaviDecorator->DecoratedControl())->SetDurationLabelL( aTime.Int64() );
 	}

void CVeiTrimForMmsView::HandleResourceChange(TInt aType)
    {
    LOGFMT(KVideoEditorLogFile, "CVeiTrimForMmsView::HandleResourceChange() In, aType:%d", aType);
    
    if (KAknsMessageSkinChange == aType && iNaviDecorator)
        {
        // Handle skin change in the navi label controls - they do not receive 
        // it automatically since they are not in the control stack
        CCoeControl* navi = iNaviDecorator->DecoratedControl();
        if (navi)
        	{
        	navi->HandleResourceChange( aType );
        	}
        }
    
    LOG(KVideoEditorLogFile, "CVeiTrimForMmsView::HandleResourceChange() Out");
    }

// ============= MVedMovieProcessingObserver FUNCTIONS START ==================

// ----------------------------------------------------------------------------
// CVeiTrimForMmsView::NotifyMovieProcessingStartedL( CVedMovie& aMovie )
//
// Called to notify that a new movie processing operation has been started.
// ----------------------------------------------------------------------------
//
void CVeiTrimForMmsView::NotifyMovieProcessingStartedL( CVedMovie& /*aMovie*/ )
    {
    }


// ----------------------------------------------------------------------------
// CVeiTrimForMmsView::NotifyMovieProcessingProgressed( CVedMovie& aMovie,
//                                                      TInt aPercentage )
//
// Called to inform about the current progress of the movie processing
// operation.
// ----------------------------------------------------------------------------
//
void CVeiTrimForMmsView::NotifyMovieProcessingProgressed( CVedMovie&/*aMovie*/,
                                                         TInt aPercentage )
    {
    LOG(KVideoEditorLogFile, "CVeiTrimForMmsView::NotifyMovieProcessingProgressed(): In");

    // Increment the progress bar.
    iProgressInfo->SetAndDraw( aPercentage );
    }


// ----------------------------------------------------------------------------
// CVeiTrimForMmsView::NotifyMovieProcessingCompleted( CVedMovie& aMovie,
//                                                     TInt aError )
//
// Called to notify that the movie processing operation has been completed.
// ----------------------------------------------------------------------------
//
void CVeiTrimForMmsView::NotifyMovieProcessingCompleted( CVedMovie& /*aMovie*/,
                                                        TInt aError )
    {
    LOGFMT(KVideoEditorLogFile, "CVeiTrimForMmsView::NotifyMovieProcessingCompleted(): In, aError:%d", aError);

	if( !aError )
		{
		ProcessNeeded( EFalse );
		}

    __ASSERT_ALWAYS( iProgressNote, User::Panic(_L("CVeiTrimForMmsView"),1) );

	// Draw the progress bar to 100%.
    iProgressInfo->SetAndDraw( 100 );

    // Delete the note.
    TRAP_IGNORE( iProgressNote->ProcessFinishedL() );

	RFs shareFServer;
	LOG(KVideoEditorLogFile, "CVeiTrimForMmsView::NotifyMovieProcessingCompleted: shareFServer connect.");

	User::LeaveIfError(shareFServer.Connect());
	shareFServer.ShareProtected();
		
	RFile openFileHandle;

	TInt err = openFileHandle.Open(shareFServer,*iTempFile,EFileRead|EFileShareReadersOnly);
	if (KErrNone != err)
		{
		LOGFMT(KVideoEditorLogFile, "CVeiTrimForMmsView::NotifyMovieProcessingCompleted: Could not open file %S with EFileShareReadersOnly. Trying EFileShareAny", iTempFile);
		User::LeaveIfError( openFileHandle.Open (shareFServer, *iTempFile, EFileRead|EFileShareAny) );
		}

	CMessageData* messageData = CMessageData::NewLC();	
	messageData->AppendAttachmentHandleL( openFileHandle );

	iSendAppUi.CreateAndSendMessageL( KSenduiMtmMmsUid, messageData, KNullUid, EFalse );	
		
	CleanupStack::PopAndDestroy( messageData );
		 
    shareFServer.Close();

    LOG(KVideoEditorLogFile, "CVeiTrimForMmsView::NotifyMovieProcessingCompleted: shareFServer closed.");
	LOG(KVideoEditorLogFile, "CVeiTrimForMmsView::NotifyMovieProcessingCompleted(): Out");
    }

// ============== MVedMovieProcessingObserver FUNCTIONS END ===================


/**
 * Called to notify that a new video clip has been successfully
 * added to the movie. Note that the indices and the start and end times
 * of the video clips after the new clip have also changed as a result.
 * Note that the transitions may also have changed. 
 *
 * @param aMovie  movie
 * @param aIndex  index of video clip in movie
 */
void CVeiTrimForMmsView::NotifyVideoClipAdded( CVedMovie& /*aMovie*/,
                                               TInt /*aIndex*/ )
    {
		ProcessNeeded( ETrue );
    }


/**
 * Called to notify that adding a new video clip to the movie has failed.
 *
 * Possible error codes:
 *	- <code>KErrNotFound</code> if there is no file with the specified name
 *    in the specified directory (but the directory exists)
 *	- <code>KErrPathNotFound</code> if the specified directory
 *    does not exist
 *	- <code>KErrUnknown</code> if the specified file is of unknown format
 *	- <code>KErrNotSupported</code> if the format of the file is recognized but
 *    adding it to the movie is not supported (e.g., it is of different resolution
 *    or format than the other clips)
 *
 * @param aMovie  movie
 * @param aError  one of the system wide error codes
 */
void CVeiTrimForMmsView::NotifyVideoClipAddingFailed( CVedMovie& /*aMovie*/, 
                                                      TInt /*aError*/ )
    {
    User::Panic( _L("MmsView"), 20 );
    }


void CVeiTrimForMmsView::NotifyVideoClipRemoved( CVedMovie& /*aMovie*/, 
                                                 TInt /*aIndex*/ )
    {
    }
	

void CVeiTrimForMmsView::NotifyVideoClipIndicesChanged( CVedMovie& /*aMovie*/,
                                                        TInt /*aOldIndex*/, 
									                    TInt /*aNewIndex*/ )
    {
    }


// ----------------------------------------------------------------------------
// CVeiTrimForMmsView::NotifyVideoClipTimingsChanged( CVedMovie& aMovie, 
//                                                    TInt aIndex )
//
// Called to notify that the timings (that is, the cut in or cut out time or
// the speed and consequently the end time, edited duration, and possibly audio
// settings) of a video clip have changed (but the index of the clip has 
// not changed). Note that the start and end times of the video clips 
// after the changed clip have also changed.
// ----------------------------------------------------------------------------
//
void CVeiTrimForMmsView::NotifyVideoClipTimingsChanged( CVedMovie& /*aMovie*/,
            										    TInt /*aIndex*/ )
    {
    LOG(KVideoEditorLogFile, "CVeiTrimForMmsView::NotifyVideoClipTimingsChanged()");
    }


void CVeiTrimForMmsView::NotifyVideoClipColorEffectChanged( CVedMovie& /*aMovie*/,
												   TInt /*aIndex*/ )
    {
    }

	
void CVeiTrimForMmsView::NotifyVideoClipAudioSettingsChanged( CVedMovie& /*aMovie*/,
											         TInt /*aIndex*/ )
    {
    }


void CVeiTrimForMmsView::NotifyVideoClipGeneratorSettingsChanged( CVedMovie& /*aMovie*/,
											             TInt /*aIndex*/ )
    {
    }


void CVeiTrimForMmsView::NotifyVideoClipDescriptiveNameChanged( CVedMovie& /*aMovie*/,
																TInt /*aIndex*/ )
    {
    }


void CVeiTrimForMmsView::NotifyStartTransitionEffectChanged( CVedMovie& /*aMovie*/ )
    {
    }


void CVeiTrimForMmsView::NotifyMiddleTransitionEffectChanged( CVedMovie& /*aMovie*/, 
													 TInt /*aIndex*/ )
    {
    }


void CVeiTrimForMmsView::NotifyEndTransitionEffectChanged( CVedMovie& /*aMovie*/ )
    {
    }


void CVeiTrimForMmsView::NotifyAudioClipAdded( CVedMovie& /*aMovie*/, TInt /*aIndex*/ )
    {
    }


void CVeiTrimForMmsView::NotifyAudioClipAddingFailed( CVedMovie& /*aMovie*/, TInt /*aError*/ )
    {
    }


void CVeiTrimForMmsView::NotifyAudioClipRemoved( CVedMovie& /*aMovie*/, TInt /*aIndex*/ )
    {
    }


void CVeiTrimForMmsView::NotifyAudioClipIndicesChanged( CVedMovie& /*aMovie*/, TInt /*aOldIndex*/, 
									           TInt /*aNewIndex*/ )
    {
    }


void CVeiTrimForMmsView::NotifyAudioClipTimingsChanged( CVedMovie& /*aMovie*/,
											   TInt /*aIndex*/ )
    {
    }


void CVeiTrimForMmsView::NotifyMovieQualityChanged( CVedMovie& /*aMovie*/ )
    {
    }

void CVeiTrimForMmsView::NotifyMovieReseted(CVedMovie& /*aMovie*/ )
    {
    }

void CVeiTrimForMmsView::NotifyMovieOutputParametersChanged(CVedMovie& /*aMovie*/)
	{
	}

void CVeiTrimForMmsView::NotifyAudioClipDynamicLevelMarkInserted(CVedMovie& /*aMovie*/, 
                                                         TInt /*aClipIndex*/, 
                                                         TInt /*aMarkIndex*/)
	{
	}

void CVeiTrimForMmsView::NotifyAudioClipDynamicLevelMarkRemoved(CVedMovie& /*aMovie*/, 
                                                        TInt /*aClipIndex*/, 
                                                        TInt /*aMarkIndex*/)
	{
	}

void CVeiTrimForMmsView::NotifyVideoClipDynamicLevelMarkInserted(CVedMovie& /*aMovie*/,
                                                         TInt /*aClipIndex*/, 
                                                         TInt /*aMarkIndex*/)
	{
	}

void CVeiTrimForMmsView::NotifyVideoClipDynamicLevelMarkRemoved(CVedMovie& /*aMovie*/, 
                                                        TInt /*aClipIndex*/, 
                                                        TInt /*aMarkIndex*/)
	{
	}

void CVeiTrimForMmsView::PlayPreviewL()
	{
	TRect rect( AppUi()->ApplicationRect() );
	iContainer->SetRect( rect );
	
	iContainer->PlayL( iVedMovie->VideoClipInfo( 0 )->FileName(), rect  );
	}

void CVeiTrimForMmsView::HandleStatusPaneSizeChange()
	{
	if ( iContainer )
		{
		iContainer->SetRect( AppUi()->ClientRect() );
		}

	}
void CVeiTrimForMmsView::SetTrimStateL( TTrimState aState )
	{
	iTrimState = aState;
	CEikStatusPane *statusPane;
	CAknTitlePane* titlePane;

	TResourceReader reader1;

	switch ( iTrimState )
        {
	case EFullPreview:		               

			Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_EMPTY );
 			Cba()->DrawDeferred();
			
			break;

		case ESeek:
			iContainer->SetRect( AppUi()->ClientRect() );
			statusPane = ( ( CAknAppUi* )iEikonEnv->EikAppUi() )->StatusPane(); 
	
			titlePane = (CAknTitlePane*) statusPane->ControlL( TUid::Uid( EEikStatusPaneUidTitle ) );
   			iCoeEnv->CreateResourceReaderLC( reader1, R_VEI_TRIM_FOR_MMS_VIEW_TITLE_NAME );
			titlePane->SetFromResourceL( reader1 );
			CleanupStack::PopAndDestroy(); //reader1
			
			Cba()->SetCommandSetL( R_AVKON_SOFTKEYS_OPTIONS_BACK );
 			Cba()->DrawDeferred();
			
			break;
			
		default:
			{
			break;
			}

		}
	}

void CVeiTrimForMmsView::DialogDismissedL( TInt aButtonId )
	{
	if ( aButtonId != EAknSoftkeyDone )
		{	
		iVedMovie->CancelProcessing();        
        }
    iProgressInfo = NULL;
	}

void CVeiTrimForMmsView::ProcessNeeded( TBool aProcessNeed )
	{
	iProcessNeeded = aProcessNeed;
	}

void CVeiTrimForMmsView::ShowGlobalErrorNoteL( const TInt aError ) const
	{	
	iErrorUi->ShowGlobalErrorNote( aError );
	}	

//=============================================================================
void CVeiTrimForMmsView::ReadSettingsL( TVeiSettings& aSettings ) const
	{
	LOG(KVideoEditorLogFile, "CVeiTrimForMmsView::ReadSettingsL: in");

	STATIC_CAST( CTrimForMmsAppUi*, AppUi() )->ReadSettingsL( aSettings );

	LOG(KVideoEditorLogFile, "CVeiTrimForMmsView::ReadSettingsL: out");
	}


// End of File
