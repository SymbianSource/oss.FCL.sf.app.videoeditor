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
#include <vedsimplecutvideo.rsg>

#include <akntitle.h> 
#include <stringloader.h> 
#include <aknnotewrappers.h>
#include <aknquerydialog.h>
#include <aknlists.h>
#include <aknPopup.h>
#include <AknProgressDialog.h>
#include <eikprogi.h>
#include <CAknMemorySelectionDialog.h>
#include <CAknFileNamePromptDialog.h>
#include <apparc.h>
#include <aknselectionlist.h>
#include <sysutil.h>
#include <aknwaitdialog.h>
#include <e32property.h>
#include <PathInfo.h> 
#include <AknCommonDialogsDynMem.h> 
#include <CAknMemorySelectionDialogMultiDrive.h> 

// User includes
#include "VeiSimpleCutVideoAppUi.h"
#include "VeiSimpleCutVideoView.h"
#include "VeiSimpleCutVideoContainer.h" 
#include "VedSimpleCutVideo.hrh"
#include "veitempmaker.h"
#include "VeiTimeLabelNavi.h"
#include "videoeditorcommon.h"
#include "VideoeditorUtils.h"
#include "VeiErrorUi.h"
#include "veinavipanecontrol.h"

void CVeiSimpleCutVideoView::ConstructL()
    {
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::ConstructL: in");

    BaseConstructL( R_VEI_CUT_VIDEO_VIEW );

	iCVeiNaviPaneControl = CVeiNaviPaneControl::NewL( StatusPane() );

	iErrorUI = CVeiErrorUI::NewL( *iCoeEnv );	

	iTimeUpdater = CPeriodic::NewL( CActive::EPriorityLow );

	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::ConstructL: creating iMovie");
	iMovie = CVedMovie::NewL( NULL );
	iMovie->RegisterMovieObserverL( this );
	
	iTempMaker = CVeiTempMaker::NewL();
	
	iOverWriteFile = EFalse;
	iPaused = EFalse; 
		
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::ConstructL: out");
    }

// ---------------------------------------------------------
TInt CVeiSimpleCutVideoView::AddClipL( const TDesC& aFilename, TBool /*aStartNow*/ )
	{
	LOGFMT(KVideoEditorLogFile, "CVeiSimpleCutVideoView::AddClipL: In: %S", &aFilename);
	
	iMovie->InsertVideoClipL( aFilename, 0 );
	
	iWaitDialog = new ( ELeave ) CAknWaitDialog( 
	    REINTERPRET_CAST( CEikDialog**, &iWaitDialog ), ETrue);
	iWaitDialog->PrepareLC(R_VEI_WAIT_NOTE_WITH_CANCEL);
	iWaitDialog->SetCallback(this);

	HBufC* stringholder = StringLoader::LoadLC( R_VEI_OPENING, iEikonEnv );
	iWaitDialog->SetTextL( *stringholder );	
	CleanupStack::PopAndDestroy(stringholder);

	iWaitDialog->RunLD();

    LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::AddClipL: Out");
	return KErrNone;
	}
    

// ---------------------------------------------------------
// CVeiSimpleCutVideoView::~CVeiSimpleCutVideoView()
// ?implementation_description
// ---------------------------------------------------------
//
CVeiSimpleCutVideoView::~CVeiSimpleCutVideoView()
    {
    LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::~CVeiSimpleCutVideoView(): In");

	if ( iProgressNote )
		{
		delete iProgressNote;
		iProgressNote = NULL;
		}
	if ( iAnimatedProgressDialog )
		{
		delete iAnimatedProgressDialog;
		iAnimatedProgressDialog = NULL;
		}
	
	if (iWaitDialog)
		{
		CloseWaitDialog();
		iWaitDialog->MakeVisible( EFalse );
		delete iWaitDialog;
		iWaitDialog = NULL;	
		}
    
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
		
	if ( iErrorUI )
		{
		delete iErrorUI;
		}
			
	if ( iMovie )
		{
		iMovie->Reset();		
		iMovie->UnregisterMovieObserver( this );			
		delete iMovie;
		iMovie = NULL;
		}
	
	if ( iTempMaker )
		{
		delete iTempMaker;
		iTempMaker = NULL;
		}
		
	if ( iSaveToFileName )
		{
		delete iSaveToFileName;
		iSaveToFileName = NULL;
		}
				
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
	
	delete iCallBack;	
	delete iCVeiNaviPaneControl;

	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::~CVeiSimpleCutVideoView(): Out");
    }

TUid CVeiSimpleCutVideoView::Id() const
     {
     return TUid::Uid( EVeiSimpleCutVideoView );
     }

void CVeiSimpleCutVideoView::DialogDismissedL( TInt aButtonId )
	{
	LOGFMT(KVideoEditorLogFile, "CVeiSimpleCutVideoView::DialogDismissedL: In, abuttonId:%d", aButtonId);	
	if (iSaving)
		{	
		LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::DialogDismissedL: 2");
		iSaving = EFalse;	
		if ( aButtonId != EAknSoftkeyDone )
			{	
			LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::DialogDismissedL: 3");
			iMovie->CancelProcessing();
			LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::DialogDismissedL: 4");
			}
		else if (KErrNone == iErrorNmb)
			{	
			LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::DialogDismissedL: 5");
			iErrorNmb = 0;
			RFs& fs = iEikonEnv->FsSession();		
			CFileMan* fileman = CFileMan::NewL( fs );	
			CleanupStack::PushL( fileman );
			
			TInt moveErr( KErrNone );

			// the user selects to overwrite the existing file
			if (iOverWriteFile) 
				{
				// temp file located in the same drive as the target file
				if ( iTempFile->Left(1) == iMovie->VideoClipInfo( iIndex )->FileName().Left(1) )
					{
					moveErr = fileman->Rename( *iTempFile, iMovie->VideoClipInfo( iIndex )->FileName(), CFileMan::EOverWrite ); 
					}
				else
					{
					moveErr = fileman->Move( *iTempFile, iMovie->VideoClipInfo( iIndex )->FileName(), CFileMan::EOverWrite );
					}
				if (!moveErr)
					{
					//AddClipL( iMovie->VideoClipInfo( iIndex )->FileName(), NULL );
					ClearInOutL(ETrue, ETrue); 
					iContainer->StopL(); 	
					TTimeIntervalMicroSeconds pos = iContainer->PlaybackPositionL();
					iContainer->GetThumbL( iMovie->VideoClipInfo( iIndex )->FileName() );		
					}
				}
			// the user selects to save with a new file name						
			else 
				{	
				// both files located in the same drive
				if ( iTempFile->Left(1) == iSaveToFileName->Left(1) )
					{
					moveErr = fileman->Rename( *iTempFile, *iSaveToFileName );	
					}
				else
					{
					moveErr = fileman->Move( *iTempFile, *iSaveToFileName );	
					}
				if (!moveErr)
					{
					ClearInOutL(ETrue, ETrue); 
					iContainer->StopL(); 
					iMovie->RemoveVideoClip(0);
					AddClipL( *iSaveToFileName, NULL );
					}
				}
			CleanupStack::PopAndDestroy( fileman ); 

			delete iTempFile;
			iTempFile = NULL;

			if ( moveErr )
				{
				iErrorUI->ShowGlobalErrorNote( moveErr );			
				}
			
			//if ( BaflUtils::FileExists(fs, *iSaveToFileName) )
			// Checking for iSaveToFileName doesn't work when overwriting so moveErr has to be used. 
			else
				{
				// Video saved successfully!
				LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::DialogDismissedL: 6");
				
				// The marks can be removed now that the video is saved
			//	ClearInOutL(ETrue, ETrue); 
			//	iContainer->StopL(); 
				//TTimeIntervalMicroSeconds pos = iContainer->PlaybackPositionL();
							
				// Publish & Subscribe API used to make the saved file name available to AIW provider
				LOG(KVideoEditorLogFile, "CVeiEditVideoView::UpdateMediaGalleryL(): Calling RProperty::Define(KUidVideoEditorProperties, VideoEditor::EPropertyFilename, RProperty::EText); ");
				TInt err = RProperty::Define(KUidVideoEditorProperties, VideoEditor::EPropertyFilename, RProperty::EText);
				if (err != KErrAlreadyExists)
					{
					User::LeaveIfError(err);
					}
				User::LeaveIfError(RProperty::Set(KUidVideoEditorProperties, VideoEditor::EPropertyFilename, iSaveToFileName->Des()));

	
				// the user has selected "Cut" or "Exit"			
				if (!iSaveOnly) 
					{					
					//AppUi()->Exit();
					if (! iCallBack)
						{		
						TCallBack cb (CVeiSimpleCutVideoView::AsyncExit, this);
						iCallBack = new (ELeave) CAsyncCallBack(cb, CActive::EPriorityStandard);
						}
					iCallBack->CallBack();									
					}	

				}				
				LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::DialogDismissedL: 7");
			}
		else
			{
			LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::DialogDismissedL: 8");
			iErrorUI->ShowGlobalErrorNote( iErrorNmb );
			iErrorNmb = 0;
			}
		}
	else if (KErrCancel != aButtonId && iErrorNmb)
			{			
			LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::DialogDismissedL: 9");
			if (KErrCorrupt == iErrorNmb || KErrTooShortVideoForCut == iErrorNmb)
				{
				HBufC* stringholder = StringLoader::LoadLC( R_VEI_VIDEO_FAILED, iEikonEnv );								
				CAknErrorNote* dlg = new ( ELeave ) CAknErrorNote( ETrue );
				dlg->ExecuteLD( *stringholder );
				CleanupStack::PopAndDestroy( stringholder );	
				}
			else
				{				
				iErrorUI->ShowGlobalErrorNote( iErrorNmb );
				}
			
			if (! iCallBack)
				{		
				TCallBack cb (CVeiSimpleCutVideoView::AsyncExit, this);
				iCallBack = new (ELeave) CAsyncCallBack(cb, CActive::EPriorityStandard);
				}
			iCallBack->CallBack();
			iErrorNmb = 0;
			}
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::DialogDismissedL: Out");
	}
		
TInt CVeiSimpleCutVideoView::AsyncExit(TAny* aThis)
	{
    LOG( KVideoEditorLogFile, "CVeiSimpleCutVideoView::AsyncExit");

    // In the asynchronous version, trap the rest of the functions 
    // to make sure that the caller's TRequestStatus is always 
    // completed, also in case of failures.
	CVeiSimpleCutVideoView* view = static_cast<CVeiSimpleCutVideoView*>(aThis);
	view->PrepareForTermination();
	view->AppUi()->Exit();
	return KErrNone;
	}

void CVeiSimpleCutVideoView::DynInitMenuPaneL( TInt aResourceId,CEikMenuPane* aMenuPane )
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
		if ( !IsCutMarkSet() )
			{
			aMenuPane->SetItemDimmed( EVeiCmdCutVideoViewSave, ETrue );
			}			
		}
	if ( !(( aResourceId == R_VEI_CUT_VIDEO_VIEW_MENU ) ||
			( aResourceId == R_VEI_CUT_VIDEO_VIEW_CONTEXT_MENU ) ) ) 
		return;

	if ( iContainer->PlaybackPositionL() >= iContainer->TotalLength() )
		{
		if (aResourceId == R_VEI_CUT_VIDEO_VIEW_MENU) 
			{
			aMenuPane->SetItemDimmed( EVeiCmdCutVideoViewPlay, ETrue );
			}

		}
	if ( ( state == CVeiSimpleCutVideoContainer::EStatePaused ) ||
			( state == CVeiSimpleCutVideoContainer::EStateInitializing ) )
		{
		if (aResourceId == R_VEI_CUT_VIDEO_VIEW_MENU) 
			{		
			aMenuPane->SetItemDimmed( EVeiCmdCutVideoViewPlay, ETrue  );
			}
		}

	if ( ( state != CVeiSimpleCutVideoContainer::EStateStopped ) && 
		( state != CVeiSimpleCutVideoContainer::EStateStoppedInitial ) &&
		( state != CVeiSimpleCutVideoContainer::EStatePaused ) )
		{
		if (aResourceId == R_VEI_CUT_VIDEO_VIEW_MENU) 
			{		
			aMenuPane->SetItemDimmed( EVeiCmdCutVideoViewPlay, ETrue  );
			}
		aMenuPane->SetItemDimmed( EVeiCmdCutVideoViewMarkIn, ETrue  );
		aMenuPane->SetItemDimmed( EVeiCmdCutVideoViewMarkOut, ETrue  );	
		}
	else
		{
		TTimeIntervalMicroSeconds pos = iContainer->PlaybackPositionL(); 
		CVedVideoClipInfo* info = iMovie->VideoClipInfo( iIndex );
		
		TInt cutInFrameIndex = info->GetVideoFrameIndexL( iMovie->VideoClipCutInTime( iIndex ));
		TInt cutOutFrameIndex =	info->GetVideoFrameIndexL( iMovie->VideoClipCutOutTime( iIndex ));
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
	if ( state != CVeiSimpleCutVideoContainer::EStatePlayingMenuOpen && 
			state != CVeiSimpleCutVideoContainer::EStatePaused )
		{
		aMenuPane->DeleteMenuItem( EVeiCmdCutVideoViewContinue );
		}
	if ( ( state == CVeiSimpleCutVideoContainer::EStateStopped ) ||
		 ( state == CVeiSimpleCutVideoContainer::EStateStoppedInitial ) ||
		 ( state == CVeiSimpleCutVideoContainer::EStateOpening ) ||
		 ( state == CVeiSimpleCutVideoContainer::EStateBuffering ) )
		{
			aMenuPane->DeleteMenuItem( EVeiCmdCutVideoViewStop );
		}

	if ( ( iMovie->VideoClipCutOutTime( iIndex ) >= iMovie->VideoClipInfo( iIndex )->Duration() ) &&
		( iMovie->VideoClipCutInTime( iIndex ) <= TTimeIntervalMicroSeconds( 0 ) ) ) 
		{
		if (aResourceId == R_VEI_CUT_VIDEO_VIEW_MENU) 
			{		
			aMenuPane->SetItemDimmed( EVeiCmdCutVideoViewClearMarks, ETrue );
			}
		}
	}

void CVeiSimpleCutVideoView::HandleCommandL(TInt aCommand)
    {
    LOGFMT(KVideoEditorLogFile, "CVeiSimpleCutVideoView::HandleCommandL( %d ): In", aCommand);

    switch ( aCommand )
        {
		case EAknSoftkeyOk:
			{
			iPopupMenuOpened = ETrue;
			if (iContainer->State() == CVeiSimpleCutVideoContainer::EStatePlaying) 
				{
				PausePreviewL();
				iContainer->SetStateL(CVeiSimpleCutVideoContainer::EStatePlayingMenuOpen);
				}

			MenuBar()->TryDisplayMenuBarL();
			if (iContainer->State() == CVeiSimpleCutVideoContainer::EStatePlayingMenuOpen) 
				{
				iContainer->SetStateL(CVeiSimpleCutVideoContainer::EStatePaused);
				}
			iPopupMenuOpened = EFalse;
			break;
			}		
		case EVeiCmdCutVideoViewBack:
        case EAknSoftkeyBack:
        	{	
        	PrepareForTermination();
        	AppUi()->Exit();
        	break;
        	}
       	case EVeiCmdCutVideoViewExit: 
       		{
            iSaveOnly = EFalse; 
			StopNaviPaneUpdateL();

			iContainer->StopL();
			iContainer->CloseStreamL();

			if ( ( iMovie->VideoClipCutInTime( 0 ) > TTimeIntervalMicroSeconds( 0 ) ) ||
				 ( iMovie->VideoClipCutOutTime( 0 ) < iMovie->VideoClipInfo(0)->Duration() ) )
				{
				if (VideoEditorUtils::LaunchSaveChangesQueryL())
					{
	            	QueryAndSaveL();	    
					}				
				else
					{
					PrepareForTermination();
					AppUi()->Exit();
					}	
				}
			else
				{
				PrepareForTermination();
				AppUi()->Exit();
				}       		
       		}
       		break;
		case EVeiCmdCutVideoViewCut: 
            {
            iSaveOnly = EFalse; 
			StopNaviPaneUpdateL();

			iContainer->StopL();
			iContainer->CloseStreamL();

			if ( ( iMovie->VideoClipCutInTime( 0 ) > TTimeIntervalMicroSeconds( 0 ) ) ||
				 ( iMovie->VideoClipCutOutTime( 0 ) < iMovie->VideoClipInfo(0)->Duration() ) )
				{
				QueryAndSaveL(); 
				}
            break;
            }
        case EVeiCmdCutVideoViewSave:
        	{
        	iSaveOnly = ETrue; 

			StopNaviPaneUpdateL();
			iContainer->StopL();
			iContainer->CloseStreamL();

        	QueryAndSaveL(); 
        	break;
        	}
        case EVeiCmdCutVideoViewMarkIn:
            {
			iCVeiNaviPaneControl->SetPauseIconVisibilityL( ETrue );
			MarkInL();
            break;
            }
        case EVeiCmdCutVideoViewMarkOut:
            {
			iCVeiNaviPaneControl->SetPauseIconVisibilityL( ETrue );
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
			iCVeiNaviPaneControl->SetPauseIconVisibilityL( EFalse );
			PlayMarkedL();
            break;
            }
		case EVeiCmdCutVideoViewPlay:
			{
			iCVeiNaviPaneControl->SetPauseIconVisibilityL( EFalse );
			PlayPreviewL();
			break;
			}
		case EVeiCmdCutVideoViewStop:
			{
			iCVeiNaviPaneControl->SetPauseIconVisibilityL( EFalse );
			StopNaviPaneUpdateL();
			iContainer->StopL();
			break;
			}
		case EVeiCmdCutVideoViewContinue:
            {
			iCVeiNaviPaneControl->SetPauseIconVisibilityL( EFalse );
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

    LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::HandleCommandL: out");
    }

void CVeiSimpleCutVideoView::NotifyMovieProcessingStartedL( CVedMovie& /*aMovie*/ )
	{
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::NotifyMovieProcessingStartedL: In");	
	iProcessed = 0;
	// @ : r_ved_cutting_note_animation
	//StartAnimatedProgressNote();
	StartProgressNoteL();
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::NotifyMovieProcessingStartedL: Out");	
	}

void CVeiSimpleCutVideoView::NotifyMovieProcessingProgressed( CVedMovie& /*aMovie*/, TInt aPercentage )
	{
	//Not allow screensaver, when processing image.
	User::ResetInactivityTime();
	
	iProcessed = aPercentage;
	if (iAnimatedProgressDialog)
		{		
		iAnimatedProgressDialog->GetProgressInfoL()->SetAndDraw( aPercentage );
		}
   	else if (iProgressNote)
	   	{   		
		iProgressNote->GetProgressInfoL()->SetAndDraw( aPercentage );
	   	}
	}

void CVeiSimpleCutVideoView::NotifyMovieProcessingCompleted( CVedMovie& /*aMovie*/, TInt aError )
	{
	LOGFMT(KVideoEditorLogFile, "CVeiSimpleCutVideoView::NotifyMovieProcessingCompleted: In, aError:%d", aError);			
	iErrorNmb = aError;
	if (iAnimatedProgressDialog)
		{
   		iAnimatedProgressDialog->GetProgressInfoL()->SetAndDraw( 100 );	
		delete iAnimatedProgressDialog;
		iAnimatedProgressDialog = NULL;
		DialogDismissedL(EAknSoftkeyDone);
		//TRAP_IGNORE( iAnimatedProgressDialog->ProcessFinishedL() );
		}
	else if (iProgressNote)
		{
		iProgressNote->GetProgressInfoL()->SetAndDraw(100);		
		iProgressNote->ProcessFinishedL();
		}
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::NotifyMovieProcessingCompleted: Out");	
	}

void CVeiSimpleCutVideoView::NotifyVideoClipAdded( CVedMovie& /*aMovie*/, TInt aIndex)
	{
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::NotifyVideoClipAdded: In");	

	iErrorNmb = 0;
	
	TTimeIntervalMicroSeconds duration = iMovie->Duration();
	LOGFMT(KVideoEditorLogFile, "CVeiCutVideCVeiSimpleCutVideoView::NotifyVideoClipAdded, 2, duration:%Ld", duration.Int64());
//	if (duration.Int64() < KMinCutVideoLength)
//		{
//		LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::NotifyVideoClipAdded: 3");	
//		iErrorNmb = KErrTooShortVideoForCut;			
//		iClosing = ETrue;
//		}
//	else
		{
		iAudioMuted = !( iMovie->VideoClipEditedHasAudio( aIndex ) );	

		iCVeiNaviPaneControl->SetPauseIconVisibilityL( EFalse );

		if ( iAudioMuted  )
			{
			iCVeiNaviPaneControl->SetVolumeIconVisibilityL( EFalse );
			VolumeMuteL();
			}
		else
			{
			iCVeiNaviPaneControl->SetVolumeIconVisibilityL( ETrue );
			}
			
//		iContainer->SetInTime( iMovie->VideoClipCutInTime( aIndex ) );
//		iContainer->SetOutTime( iMovie->VideoClipCutOutTime( aIndex ) );	
		iContainer->GetThumbL( iMovie->VideoClipInfo( iIndex )->FileName() );		
		}

	if (iWaitDialog)
		{
		delete iWaitDialog;
		iWaitDialog = NULL;	
		}
//	CloseWaitDialog();
	iContainer->DrawDeferred();

	// video clip has to be added before setting the title pane text	
	// : handle leave
	SetTitlePaneTextL(); 
			
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::NotifyVideoClipAdded: Out");		
	}

void CVeiSimpleCutVideoView::NotifyVideoClipAddingFailed( CVedMovie& /*aMovie*/, TInt aError )
	{
	LOGFMT(KVideoEditorLogFile, "CVeiSimpleCutVideoView::NotifyVideoClipAddingFailed: In, aError:%d", aError);	
	iErrorNmb = aError;	
	iClosing = ETrue;
	
	CloseWaitDialog();
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::NotifyVideoClipAddingFailed: Out");			
	}

void CVeiSimpleCutVideoView::NotifyVideoClipRemoved( CVedMovie& /*aMovie*/, TInt /*aIndex*/ )
	{
	}

void CVeiSimpleCutVideoView::NotifyVideoClipIndicesChanged( CVedMovie& /*aMovie*/, TInt /*aOldIndex*/, 
									           TInt /*aNewIndex*/ )
	{
	}

void CVeiSimpleCutVideoView::NotifyVideoClipTimingsChanged( CVedMovie& /*aMovie*/, TInt /*aIndex*/ )
	{
	}

void CVeiSimpleCutVideoView::NotifyVideoClipColorEffectChanged( CVedMovie& /*aMovie*/, TInt /*aIndex*/ )
	{
	}

void CVeiSimpleCutVideoView::NotifyVideoClipAudioSettingsChanged( CVedMovie& /*aMovie*/,
											         TInt /*aIndex*/ )
	{
	}

void CVeiSimpleCutVideoView::NotifyStartTransitionEffectChanged( CVedMovie& /*aMovie*/ )
	{
	}

void CVeiSimpleCutVideoView::NotifyMiddleTransitionEffectChanged( CVedMovie& /*aMovie*/, 
													 TInt /*aIndex*/ )
	{
	}

void CVeiSimpleCutVideoView::NotifyEndTransitionEffectChanged( CVedMovie& /*aMovie*/ )
	{
	}

void CVeiSimpleCutVideoView::NotifyAudioClipAdded( CVedMovie& /*aMovie*/, TInt /*aIndex*/ )
	{
	}

void CVeiSimpleCutVideoView::NotifyAudioClipAddingFailed( CVedMovie& /*aMovie*/, TInt /*aError*/ )
	{
	}

void CVeiSimpleCutVideoView::NotifyAudioClipRemoved( CVedMovie& /*aMovie*/, TInt /*aIndex*/ )
	{
	}

void CVeiSimpleCutVideoView::NotifyAudioClipIndicesChanged( CVedMovie& /*aMovie*/, TInt /*aOldIndex*/, 
									           TInt /*aNewIndex*/ )
	{
	}

void CVeiSimpleCutVideoView::NotifyAudioClipTimingsChanged( CVedMovie& /*aMovie*/,
											   TInt /*aIndex*/ )
	{
	}

void CVeiSimpleCutVideoView::NotifyMovieReseted( CVedMovie& /*aMovie*/ )
	{
	}

void CVeiSimpleCutVideoView::NotifyVideoClipGeneratorSettingsChanged(CVedMovie& /*aMovie*/,
											             TInt /*aIndex*/) 
	{
	}

void CVeiSimpleCutVideoView::NotifyVideoClipDescriptiveNameChanged(CVedMovie& /*aMovie*/, TInt /*aIndex*/)
	{
	}

void CVeiSimpleCutVideoView::NotifyMovieQualityChanged(CVedMovie& /*aMovie*/)
	{
	}

void CVeiSimpleCutVideoView::NotifyMovieOutputParametersChanged(CVedMovie& /*aMovie*/)
	{
	}

void CVeiSimpleCutVideoView::NotifyAudioClipDynamicLevelMarkInserted(CVedMovie& /*aMovie*/, 
                                                         TInt /*aClipIndex*/, 
                                                         TInt /*aMarkIndex*/)
	{
	}

void CVeiSimpleCutVideoView::NotifyAudioClipDynamicLevelMarkRemoved(CVedMovie& /*aMovie*/, 
                                                        TInt /*aClipIndex*/, 
                                                        TInt /*aMarkIndex*/)
	{
	}

void CVeiSimpleCutVideoView::NotifyVideoClipDynamicLevelMarkInserted(CVedMovie& /*aMovie*/,
                                                         TInt /*aClipIndex*/, 
                                                         TInt /*aMarkIndex*/)
	{
	}

void CVeiSimpleCutVideoView::NotifyVideoClipDynamicLevelMarkRemoved(CVedMovie& /*aMovie*/, 
                                                        TInt /*aClipIndex*/, 
                                                        TInt /*aMarkIndex*/)
	{
	}

void CVeiSimpleCutVideoView::CloseWaitDialog()
    {
    if ( iWaitDialog )
        {
        TRAP_IGNORE( iWaitDialog->ProcessFinishedL() );
        }       
    }


void CVeiSimpleCutVideoView::DoActivateL(
   const TVwsViewId& /*aPrevViewId*/,TUid /*aCustomMessageId*/,
   const TDesC8& /*aCustomMessage*/)
    {
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::DoActivateL, In");
	iPaused = EFalse;
    if (!iContainer)
        {
		iContainer = CVeiSimpleCutVideoContainer::NewL( AppUi()->ClientRect(), *this, *iErrorUI );
		iContainer->SetMopParent( this );
		AppUi()->AddToStackL( *this, iContainer );
		iCVeiNaviPaneControl->SetObserver( iContainer );
        }

	UpdateCBAL( CVeiSimpleCutVideoContainer::EStateInitializing );
	
	CheckMemoryCardAvailabilityL();
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::DoActivateL, out");
	}

// ---------------------------------------------------------
// CVeiSimpleCutVideoView::HandleCommandL(TInt aCommand)
// ?implementation_description
// ---------------------------------------------------------
//
void CVeiSimpleCutVideoView::DoDeactivate()
   {    
	if ( iTimeUpdater )
		{
		iTimeUpdater->Cancel();
		}

	if ( iContainer )
        {
        AppUi()->RemoveFromViewStack( *this, iContainer );

		delete iContainer;
		iContainer = NULL;
        }
	}

void CVeiSimpleCutVideoView::PlayPreviewL()
	{
	iPaused = EFalse;
	StartNaviPaneUpdateL();
	iContainer->PlayL( iMovie->VideoClipInfo( iIndex )->FileName() );
	}

void CVeiSimpleCutVideoView::PausePreviewL()
	{
	iCVeiNaviPaneControl->SetPauseIconVisibilityL( ETrue );
	StopNaviPaneUpdateL();

	iContainer->PauseL();
	}

void CVeiSimpleCutVideoView::UpdateCBAL(TInt aState)
	{	
	MenuBar()->SetContextMenuTitleResourceId( R_VEI_MENUBAR_CUT_VIDEO_VIEW_CONTEXT ); 
	switch (aState)
		{
		case CVeiSimpleCutVideoContainer::EStateInitializing:
		case CVeiSimpleCutVideoContainer::EStateOpening:
		case CVeiSimpleCutVideoContainer::EStateBuffering:		
			{
			if (! iClosing)
				{				
				Cba()->SetCommandSetL(R_AVKON_SOFTKEYS_EMPTY); 
				}
			break;
			}
		case CVeiSimpleCutVideoContainer::EStateStoppedInitial:
			{
			if (! iClosing)
				{		
				// no marks set
				if ( ( iMovie->VideoClipCutInTime( 0 ) == TTimeIntervalMicroSeconds( 0 ) ) &&
				 ( iMovie->VideoClipCutOutTime( 0 ) == iMovie->VideoClipInfo(0)->Duration() ) )
					{		
					// playhead in the beginning	
					if ( iContainer->PlaybackPositionL() == 0 || AknLayoutUtils::PenEnabled() )
						{
						Cba()->SetCommandSetL(R_VEI_SOFTKEYS_OPTIONS_BACK__PLAY); 
					}
					// playhead not in the beginning
					else
						{
						Cba()->SetCommandSetL(R_VEI_SOFTKEYS_IN_OUT__MENU);	
						}
					}
				//start or end mark has been set	
				else 
					{
					// start mark has been set, end mark has not been set 
					// and the the playhead is on the right hand side of the start mark
					if (( iMovie->VideoClipCutInTime( 0 ) != TTimeIntervalMicroSeconds( 0 ) ) && 
					    ( iMovie->VideoClipCutOutTime( 0 ) == iMovie->VideoClipInfo(0)->Duration() ) &&
					    ( iContainer->PlaybackPositionL() > iMovie->VideoClipCutInTime( 0 ))
					    && !AknLayoutUtils::PenEnabled() )
					    {
					    Cba()->SetCommandSetL( R_VEI_SOFTKEYS_OPTIONS_OUT__MENU ); 
					    }
					else
					    {
					    Cba()->SetCommandSetL(R_VEI_SOFTKEYS_OPTIONS_CUT__MENU);
					    }
					}
				}
			break;
			}
		case CVeiSimpleCutVideoContainer::EStatePaused:
		case CVeiSimpleCutVideoContainer::EStateStopped:
			{							
			
			// no marks set
			if ( ( iMovie->VideoClipCutInTime( 0 ) == TTimeIntervalMicroSeconds( 0 ) ) &&
				 ( iMovie->VideoClipCutOutTime( 0 ) == iMovie->VideoClipInfo(0)->Duration() ) )
				{
				// playhead in the beginning		
				if (iContainer->PlaybackPositionL() == 0 || AknLayoutUtils::PenEnabled() )
					{
					Cba()->SetCommandSetL(R_VEI_SOFTKEYS_OPTIONS_BACK__PLAY); 
					}
				else
					{
					Cba()->SetCommandSetL(R_VEI_SOFTKEYS_IN_OUT__MENU);	
					}
				}
			// start or end mark has been set
			else
				{		
					// start mark has been set, end mark has not been set 
					// and the the playhead is on the right hand side of the start mark
					if (( iMovie->VideoClipCutInTime( 0 ) != TTimeIntervalMicroSeconds( 0 ) ) && 
					    ( iMovie->VideoClipCutOutTime( 0 ) == iMovie->VideoClipInfo(0)->Duration() ) &&					
					    ( iContainer->PlaybackPositionL() > iMovie->VideoClipCutInTime( 0 ))
					    && !AknLayoutUtils::PenEnabled() ) 
					    {
					    Cba()->SetCommandSetL( R_VEI_SOFTKEYS_OPTIONS_OUT__MENU );    
					    }
					else
					    {																
        				Cba()->SetCommandSetL(R_VEI_SOFTKEYS_OPTIONS_CUT__MENU); 
					    }
				}
			break;
			}
		case CVeiSimpleCutVideoContainer::EStatePlaying:
			{			
			// playhead outside cut area
			if ( iContainer->PlaybackPositionL() < iMovie->VideoClipCutInTime( iIndex ) )
				{
				if ( AknLayoutUtils::PenEnabled() )
					{
					Cba()->SetCommandSetL( R_VEI_SOFTKEYS_IN_NONE__MENU ); 
					}
				else
					{
					Cba()->SetCommandSetL( R_VEI_SOFTKEYS_IN_STOP__MENU ); 
					}
				iMarkState = EMarkStateIn;
				}
			// playdhead inside cut area	
			else if ( iContainer->PlaybackPositionL() < iMovie->VideoClipCutOutTime( iIndex ) )
				{
				Cba()->SetCommandSetL( R_VEI_SOFTKEYS_IN_OUT__MENU ); 
				iMarkState = EMarkStateInOut;
				}
			// playhead outside cut area
			else
				{
				if ( AknLayoutUtils::PenEnabled() )
					{
					Cba()->SetCommandSetL( R_VEI_SOFTKEYS_NONE_OUT__MENU ); 
					}
				else
					{
					Cba()->SetCommandSetL( R_VEI_SOFTKEYS_STOP_OUT__MENU ); 
					}
				iMarkState = EMarkStateOut;
				}
				
			break;
			}
		case CVeiSimpleCutVideoContainer::EStateTerminating:
			{
			return;
			}
		default:
			{
			break;	
			}
		}
	Cba()->DrawDeferred();
	}

void CVeiSimpleCutVideoView::PlayMarkedL()
	{
	LOGFMT3(KVideoEditorLogFile, "CVeiSimpleCutVideoView::PlayMarkedL: In: iIndex:%d, iMovie->VideoClipCutInTime():%Ld, iMovie->VideoClipCutOutTime():%Ld", iIndex, iMovie->VideoClipCutInTime( iIndex ).Int64(), iMovie->VideoClipCutOutTime( iIndex ).Int64());
	iPaused = EFalse;
	StartNaviPaneUpdateL();	
	iContainer->PlayMarkedL( iMovie->VideoClipInfo( iIndex )->FileName(),
		iMovie->VideoClipCutInTime( iIndex ), iMovie->VideoClipCutOutTime( iIndex ) );		
	
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::PlayMarkedL: Out");
	}

void CVeiSimpleCutVideoView::ClearInOutL( TBool aClearIn, TBool aClearOut )
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
	TTimeIntervalMicroSeconds cutin = iMovie->VideoClipCutInTime( 0 );
	TTimeIntervalMicroSeconds cutout = iMovie->VideoClipCutOutTime( 0 );
	
	if ( ( cutin == TTimeIntervalMicroSeconds( 0 ) ) &&
		 ( cutout == iMovie->VideoClipInfo(0)->Duration() ) )
		{		
		Cba()->SetCommandSetL(R_VEI_SOFTKEYS_OPTIONS_BACK__MENU); 
		Cba()->DrawDeferred();
		}
	}

void CVeiSimpleCutVideoView::MarkInL()
	{
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::MarkInL, In");
	TTimeIntervalMicroSeconds pos = iContainer->PlaybackPositionL();

    // the start mark can't be put right to the beginning
    // because start mark is at position 0 when it is not set
    if (pos == 0)
        {
        pos = 1;
        }
	
//	TTimeIntervalMicroSeconds clipDuration = iMovie->VideoClipInfo( iIndex )->Duration();
//	CVedVideoClipInfo* info = iMovie->VideoClipInfo( iIndex );
//	TTimeIntervalMicroSeconds intraPos = info->VideoFrameStartTimeL( 
//			info->GetVideoFrameIndexL( pos ) );
	if (iMovie->VideoClipCutOutTime(iIndex) > pos)
		{
		StopNaviPaneUpdateL();		
		LOGFMT2(KVideoEditorLogFile, "CVeiSimpleCutVideoView::MarkInL, 2, iIndex:%d, pos:%Ld", iIndex, pos.Int64());
		iMovie->VideoClipSetCutInTime( iIndex, pos );
		iContainer->MarkedInL();	
		}
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::MarkInL, Out");
	}

void CVeiSimpleCutVideoView::MarkOutL()
	{
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::MarkOutL, In");
	TTimeIntervalMicroSeconds pos = iContainer->PlaybackPositionL();
//	CVedVideoClipInfo* info = iMovie->VideoClipInfo( iIndex );
//	TTimeIntervalMicroSeconds intraPos = info->VideoFrameStartTimeL( 
//			info->GetVideoFrameIndexL( pos ) );

	if (iMovie->VideoClipCutInTime(iIndex) < pos)
		{			
		StopNaviPaneUpdateL();
		iMovie->VideoClipSetCutOutTime( iIndex, pos );
		LOGFMT2(KVideoEditorLogFile, "CVeiSimpleCutVideoView::MarkOutL, 2, iIndex:%d, pos:%Ld", iIndex, pos.Int64() );		
		iContainer->MarkedOutL();
		}
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::MarkOutL, Out");
	}


void CVeiSimpleCutVideoView::MoveStartOrEndMarkL(TTimeIntervalMicroSeconds aPosition, CVeiSimpleCutVideoContainer::TCutMark aMarkType)
	{
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::MoveStartOrEndMarkL, In");
	
	StopNaviPaneUpdateL();
	
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::MoveStartOrEndMarkL, 2");
	
	if (aMarkType == CVeiSimpleCutVideoContainer::EStartMark)
		{
		iMovie->VideoClipSetCutInTime( iIndex, aPosition);
		}
	else if (aMarkType == CVeiSimpleCutVideoContainer::EEndMark)
		{
		iMovie->VideoClipSetCutOutTime( iIndex, aPosition);
		}		
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::MoveStartOrEndMarkL, Out");
	}

TUint CVeiSimpleCutVideoView::InPointTime()
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

TUint CVeiSimpleCutVideoView::OutPointTime()
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

TInt CVeiSimpleCutVideoView::UpdateTimeCallbackL(TAny* aPtr)
	{
	CVeiSimpleCutVideoView* view = (CVeiSimpleCutVideoView*)aPtr;

    view->UpdateTimeL();

	return 1;
	}


void CVeiSimpleCutVideoView::UpdateTimeL()
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

void CVeiSimpleCutVideoView::DrawTimeNaviL()
	{
	TTime elapsed( iContainer->PlaybackPositionL().Int64() );
	TTime total( iContainer->TotalLength().Int64() );
	iCVeiNaviPaneControl->DrawTimeNaviL( elapsed, total );
	}


void CVeiSimpleCutVideoView::StartNaviPaneUpdateL()
	{
	DrawTimeNaviL();
	if (iTimeUpdater && !iTimeUpdater->IsActive())
		{
		iTimeUpdater->Start(200000, 1000000/10, TCallBack(CVeiSimpleCutVideoView::UpdateTimeCallbackL, this));
		}
	}

void CVeiSimpleCutVideoView::StopNaviPaneUpdateL()
	{
	if (iContainer)
		{
		DrawTimeNaviL();
		}
	if (iTimeUpdater)
		{
		iTimeUpdater->Cancel();
		}
	}


void CVeiSimpleCutVideoView::VolumeMuteL()
	{
	iContainer->MuteL();
	}



void CVeiSimpleCutVideoView::ShowVolumeLabelL( TInt aVolume )
	{
	iCVeiNaviPaneControl->ShowVolumeLabelL( aVolume );	
	}

void CVeiSimpleCutVideoView::HandleForegroundEventL  ( TBool aForeground )
	{
	if ( !aForeground )
		{
		// If the application is closing down, calling PauseL could result in 
		// a callback from the MMF player after the container is already deleted,
		// causing KERN-EXEC 3
		if ( iOnTheWayToDestruction || 
		     static_cast<CVeiSimpleCutVideoAppUi*>( AppUi() )->AppIsOnTheWayToDestruction() )
			{
			iContainer->PrepareForTerminationL();
			return;
			}
		else
			{
			if( iContainer->State() == CVeiSimpleCutVideoContainer::EStatePlaying ||
		    	iContainer->State() == CVeiSimpleCutVideoContainer::EStatePlayingMenuOpen )
				{
				iPaused = ETrue;
				iContainer->PauseL( EFalse );
				}
			}
		}
	else
		{
		UpdateCBAL( iContainer->State() );
		if( 0 )
//		if( iPaused )
			{
			iPaused = EFalse;
			PlayPreviewL();
			}
		}
	}

// ---------------------------------------------------------
// CVeiSimpleCutVideoView::HandleStatusPaneSizeChange()
// ---------------------------------------------------------
//
void CVeiSimpleCutVideoView::HandleStatusPaneSizeChange()
    {
    if ( iContainer )
        {
        iContainer->SetRect( AppUi()->ClientRect() );
        }
    }

TBool CVeiSimpleCutVideoView::IsEnoughFreeSpaceToSaveL()// const
	{
	RFs&	fs = iEikonEnv->FsSession();
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

	// In this case, we decide the target drive automatically when starting 
	// to save the video. Thus it is enough to know that there is free space 
	// on either drive at this stage.
	spaceBelowCriticalLevel = SysUtil::DiskSpaceBelowCriticalLevelL( &fs, sizeEstimate, EDriveC );
	if( spaceBelowCriticalLevel )
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

TTimeIntervalMicroSeconds CVeiSimpleCutVideoView::GetVideoClipCutInTime()
	{
	TTimeIntervalMicroSeconds cutInTime(0);
	if ( iMovie )
		{
		cutInTime = iMovie->VideoClipCutInTime( iIndex );
		}
	return cutInTime;
	}

TTimeIntervalMicroSeconds CVeiSimpleCutVideoView::GetVideoClipCutOutTime()
	{
	TTimeIntervalMicroSeconds cutOutTime(0);
	if ( iMovie )
		{
		cutOutTime = iMovie->VideoClipCutOutTime( iIndex );
		}
	return cutOutTime;
	}

void CVeiSimpleCutVideoView::HandleResourceChange(TInt aType)
    {
    LOGFMT(KVideoEditorLogFile, "CVeiSimpleCutVideoView::HandleResourceChange() In, aType:%d", aType);
    
    if ( iContainer )
    	{    	
    	if ( aType == KEikMessageFadeAllWindows && (iContainer->State() == CVeiSimpleCutVideoContainer::EStatePlaying) )
    		{
    		this->PausePreviewL();
    		iSelectionKeyPopup = ETrue;
    		}
    	else if ( iSelectionKeyPopup && aType == KEikMessageUnfadeWindows && (iContainer->State() == CVeiSimpleCutVideoContainer::EStatePaused) )
    		{
    		this->PlayPreviewL();
    		iSelectionKeyPopup = EFalse;
    		}
    	}
    
    if (KAknsMessageSkinChange == aType)
        {
        // Handle skin change in the navi label controls - they do not receive 
        // it automatically since they are not in the control stack
        iCVeiNaviPaneControl->HandleResourceChange( aType );
        }
    
    LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::HandleResourceChange() Out");
    }

//=======================================================================================================
const CVedMovie* CVeiSimpleCutVideoView::Movie() const
	{
	return iMovie;
	}

//=======================================================================================================
TBool CVeiSimpleCutVideoView::SaveL()
	{
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::SaveL: in");

	TFileName outputFile;
	TBool ret = EFalse;

	TTimeIntervalMicroSeconds cutin;
	TTimeIntervalMicroSeconds cutout;
	cutin = iMovie->VideoClipCutInTime( 0 );
	cutout = iMovie->VideoClipCutOutTime( 0 );

	if ( ( cutin != TTimeIntervalMicroSeconds( 0 ) ) ||
		 ( cutout!= iMovie->VideoClipInfo(0)->Duration() ) )
		{
		RFs&	fs = iEikonEnv->FsSession();

		// GenerateNewDocumentNameL also checks disk space
		TInt err(KErrNone);
		if(!iOverWriteFile)
			{
			err = VideoEditorUtils::GenerateNewDocumentNameL (
				fs,
				iMovie->VideoClipInfo( iIndex )->FileName(),
				outputFile,
				iMovie->Format(),
				iMovie->GetSizeEstimateL() );
			}
		else
			{
			outputFile.Zero();
			outputFile.Copy(iMovie->VideoClipInfo( iIndex )->FileName());
			}
		
		LOGFMT(KVideoEditorLogFile, "CVeiSimpleCutVideoView::SaveL: 1, err:%d", err);

		if ( KErrNone == err )
			{
			if (iSaveToFileName)
				{
				delete iSaveToFileName;
				iSaveToFileName = NULL;	
				}
			iSaveToFileName = HBufC::NewL( outputFile.Length() );
			*iSaveToFileName = outputFile;

			// Start saviong the video. 
			// To be finished in DialogDismissedL...
			iSaving = ETrue;
			StartTempFileProcessingL();

            ret = ETrue;
			}
		else 
			{
			iErrorUI->ShowGlobalErrorNote( err );
			ret = EFalse;
			}						
		}
	else 
		{
		ret = EFalse;
		}

	LOGFMT(KVideoEditorLogFile, "CVeiSimpleCutVideoView::SaveL: out: %d", ret);

	return ret;
	}


//=============================================================================
TInt CVeiSimpleCutVideoView::QueryAndSaveL()
{

	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::QueryAndSaveL: in");

	TFileName outputFile;
	TBool ret = EFalse;

	TTimeIntervalMicroSeconds cutin;
	TTimeIntervalMicroSeconds cutout;
	cutin = iMovie->VideoClipCutInTime( 0 );
	cutout = iMovie->VideoClipCutOutTime( 0 );

	if ( ( cutin != TTimeIntervalMicroSeconds( 0 ) ) ||
		 ( cutout!= iMovie->VideoClipInfo(0)->Duration() ) )
		{
		RFs	fs = CCoeEnv::Static()->FsSession();


 		// launch query with choices "Replace original" and "Save with a new file name" 
 		TInt userSelection = VideoEditorUtils::LaunchSaveVideoQueryL(); 
 		
	  	if(userSelection == 0) 
	  	// the user selects to save with a new file name
			{
			iOverWriteFile = EFalse;

            // Multiple drive support enabled
#ifdef RD_MULTIPLE_DRIVE	
		    TDriveNumber driveNumber;
            TFileName driveAndPath;
            CAknMemorySelectionDialogMultiDrive* multiDriveDlg = CAknMemorySelectionDialogMultiDrive::NewL(ECFDDialogTypeSave, EFalse );			
		    CleanupStack::PushL( multiDriveDlg );
		    
			// launch "Select memory" query
            if (multiDriveDlg->ExecuteL( driveNumber, &driveAndPath, NULL ))
    			{
	    		outputFile.Zero();				
			
		    	// Generate a default name for the new file
			    TInt err = VideoEditorUtils::GenerateFileNameL (
                                    					fs,
                                    					iMovie->VideoClipInfo( iIndex )->FileName(),		
                                    					outputFile,
                                    					iMovie->Format(),
                                    					iMovie->GetSizeEstimateL(),
                                    					driveAndPath);	
				
                driveAndPath.Append( PathInfo::VideosPath() );					
				
			    if ( KErrNone == err )
					{
					if (iSaveToFileName)
						{
						delete iSaveToFileName;
						iSaveToFileName = NULL;	
						}				 

					// launch file name prompt dialog
					if (CAknFileNamePromptDialog::RunDlgLD(outputFile, driveAndPath, KNullDesC))
						{
						driveAndPath.Append(outputFile);
						outputFile.Copy(driveAndPath);
						iSaveToFileName = HBufC::NewL( outputFile.Length() );
						*iSaveToFileName = outputFile;

						// Start saving the video. 
						// To be finished in DialogDismissedL...
						iSaving = ETrue;
						StartTempFileProcessingL();

			            ret = ETrue;
			            }
					}
				else // err != KErrNone 
					{
					ret = EFalse;
					}						
    			}
		    CleanupStack::PopAndDestroy( multiDriveDlg );
#else // no multiple drive support
			// launch "Select memory" query
		    CAknMemorySelectionDialog::TMemory selectedMemory(CAknMemorySelectionDialog::EPhoneMemory);		
			if (CAknMemorySelectionDialog::RunDlgLD(selectedMemory))
				{
				// create path for the image	
				TFileName driveAndPath;        		
				VideoEditor::TMemory memorySelection = VideoEditor::EMemPhoneMemory;		 
				if (selectedMemory == CAknMemorySelectionDialog::EPhoneMemory)
					{
					memorySelection = VideoEditor::EMemPhoneMemory;
					driveAndPath.Copy( PathInfo::PhoneMemoryRootPath() );
					driveAndPath.Append( PathInfo::VideosPath() );							
					}
				else if (selectedMemory == CAknMemorySelectionDialog::EMemoryCard)
					{	
					memorySelection = VideoEditor::EMemMemoryCard;				
					driveAndPath.Copy( PathInfo::MemoryCardRootPath() );
					driveAndPath.Append( PathInfo::VideosPath() );							
					}        				 

				// GenerateNewDocumentNameL also checks disk space
				TInt err = VideoEditorUtils::GenerateNewDocumentNameL (
					fs,
					iMovie->VideoClipInfo( iIndex )->FileName(),
					outputFile,
					iMovie->Format(),
					iMovie->GetSizeEstimateL(),
					memorySelection );
					
				if ( KErrNone == err )
					{
					if (iSaveToFileName)
						{
						delete iSaveToFileName;
						iSaveToFileName = NULL;	
						}				 

					// launch file name prompt dialog
					if (CAknFileNamePromptDialog::RunDlgLD(outputFile, driveAndPath, KNullDesC))
						{
						driveAndPath.Append(outputFile);
						outputFile.Copy(driveAndPath);
						iSaveToFileName = HBufC::NewL( outputFile.Length() );
						*iSaveToFileName = outputFile;

						// Start saving the video. 
						// To be finished in DialogDismissedL...
						iSaving = ETrue;
						StartTempFileProcessingL();

			            ret = ETrue;
			            }
					}
				else // err != KErrNone 
					{
					ret = EFalse;
					}						
				}
#endif				
			}
		// user selects to overwrite
		else if (userSelection == 1)
		
			{
			iOverWriteFile = ETrue;
			ret = SaveL();
			return ret;	
			}
		else // user cancelled
			{
			ret = EFalse;
			}
		}
	else 
		{
		ret = EFalse;
		}

	LOGFMT(KVideoEditorLogFile, "CVeiSimpleCutVideoView::QueryAndSaveL: out: %d", ret);

	return ret;
}



//=======================================================================================================
TBool CVeiSimpleCutVideoView::IsCutMarkSet()
	{
	ASSERT( iMovie ); // We always have iMovie
	TTimeIntervalMicroSeconds cutIn( iMovie->VideoClipCutInTime( iIndex ) ); 
	TTimeIntervalMicroSeconds cutOut( iMovie->VideoClipCutOutTime( iIndex ) ); 
	TTimeIntervalMicroSeconds duration( iMovie->VideoClipInfo(iIndex)->Duration() );
	// cutIn or cutOut mark is set
	return (cutIn != TTimeIntervalMicroSeconds( 0 ) || cutOut != duration);
	}

//=======================================================================================================
void CVeiSimpleCutVideoView::StartTempFileProcessingL()
	{
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::StartTempFileProcessingL: in");

	RFs&	fs = iEikonEnv->FsSession();

	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::StartTempFileProcessingL() -- NEW TEMP");

	if (iTempFile)
		{
		delete iTempFile;
		iTempFile = NULL;
		}
	iTempFile = HBufC::NewL(KMaxFileName);
	iTempMaker->GenerateTempFileName( *iTempFile, iMovieSaveSettings.MemoryInUse(), iMovie->Format() );
	LOGFMT(KVideoEditorLogFile, "CVeiSimpleCutVideoView::StartTempFileProcessingL: 1, iTempFile:%S", iTempFile);

	if ( !IsEnoughFreeSpaceToSaveL() )
		{
		LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::StartTempFileProcessingL: 2");
		return;
		}

	iMovie->ProcessL( *iTempFile, *this );

	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::StartTempFileProcessingL: out");
	}

//=======================================================================================================
void CVeiSimpleCutVideoView::ShowErrorNoteL( const TInt aResourceId, TInt aError ) const
	{
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::ShowErrorNoteL: in");

	HBufC* stringholder;
	if ( aError == 0 )
		{
		stringholder = StringLoader::LoadLC( aResourceId, iEikonEnv );
		}
	else
		{
		stringholder = StringLoader::LoadLC( aResourceId, aError, iEikonEnv );
		}

	CAknErrorNote* dlg = new ( ELeave ) CAknErrorNote( ETrue );
	dlg->ExecuteLD( *stringholder );

	CleanupStack::PopAndDestroy( stringholder );

	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::ShowErrorNoteL: out");
	}

//=======================================================================================================

void CVeiSimpleCutVideoView::StartProgressNoteL()
{
	iProgressNote = 
		new (ELeave) CAknProgressDialog(REINTERPRET_CAST(CEikDialog**, 
		&iProgressNote), ETrue);
	iProgressNote->SetCallback(this);
	iProgressNote->ExecuteDlgLD( R_VEI_PROGRESS_NOTE_WITH_CANCEL );

	HBufC* stringholder;
    TApaAppCaption caption;
    TRAPD( err, iContainer->ResolveCaptionNameL( caption ) );
    
    // If something goes wrong, show basic "Saving" note
    if ( err )
        {
        stringholder = iEikonEnv->AllocReadResourceLC( R_VEI_PROGRESS_NOTE_SAVING );
        }
    else
        {
        stringholder = StringLoader::LoadLC( R_VEI_PROGRESS_NOTE_CUTTING_VIDEO, caption, iEikonEnv );
        }        

	iProgressNote->SetTextL( *stringholder );
	CleanupStack::PopAndDestroy( stringholder );	
	
	iProgressNote->GetProgressInfoL()->SetFinalValue(100);	
}

//=======================================================================================================
void CVeiSimpleCutVideoView::StartAnimatedProgressNoteL()
{
	if (iAnimatedProgressDialog)
    	{
    	delete iAnimatedProgressDialog;
    	iAnimatedProgressDialog = NULL;
    	}
    	
   
	iAnimatedProgressDialog = new (ELeave) CExtProgressDialog( &iAnimatedProgressDialog);
	iAnimatedProgressDialog->PrepareLC(R_WAIT_DIALOG);	
	iAnimatedProgressDialog->SetCallback( this );
	
	HBufC* stringholder = StringLoader::LoadLC( R_VEI_PROGRESS_NOTE_CUTTING_VIDEO, iEikonEnv );
	iAnimatedProgressDialog->SetTextL( *stringholder );
	CleanupStack::PopAndDestroy(stringholder);
	
	iAnimatedProgressDialog->SetAnimationResourceIdL( VideoEditor::EAnimationCut );
	iAnimatedProgressDialog->GetProgressInfoL()->SetFinalValue( 100 );
	iAnimatedProgressDialog->StartAnimationL();
	iAnimatedProgressDialog->RunLD();
}

// ----------------------------------------------------------------------------
// CVeiSimpleCutVideoView::CheckMemoryCardAvailability()
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
void CVeiSimpleCutVideoView::CheckMemoryCardAvailabilityL()
    {
    LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::CheckMemoryCardAvailability(): in");

    // Read the video editor settings from ini file.
        
    STATIC_CAST(CVeiSimpleCutVideoAppUi*,AppUi())->ReadSettingsL( iMovieSaveSettings );
    
    CAknMemorySelectionDialog::TMemory memoryInUse( iMovieSaveSettings.MemoryInUse() );

    // Check the MMC accessibility only if MMC is used as saving store.
    if( memoryInUse == CAknMemorySelectionDialog::EMemoryCard )
        {
        RFs& fs = iEikonEnv->FsSession();
        TDriveInfo driveInfo;
        
        User::LeaveIfError( fs.Drive( driveInfo, KMmcDrive ) );

        // Media is not present (MMC card not inserted).
        if( EMediaNotPresent == driveInfo.iType )
            {
            LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::CheckMemoryCardAvailability(): no media");
            
            iMovieSaveSettings.MemoryInUse() = CAknMemorySelectionDialog::EPhoneMemory;
            // do not overwrite because doing so permanently sets memory to phone memory
			//STATIC_CAST( CVeiSimpleCutVideoAppUi*, AppUi() )->WriteSettingsL( settings );
    	    }
        // Media is present
        else
            {
            LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::CheckMemoryCardAvailability(): media present");

            TVolumeInfo volumeInfo;
            TInt volumeErr = fs.Volume( volumeInfo, KMmcDrive );
            LOGFMT(KVideoEditorLogFile, "CEditVideoView::CheckMemoryCardAvailability() Volume(): %d", volumeErr);

            // Show note if media is corrupted/unformatted.
            if( KErrCorrupt == volumeErr )
                {
                HBufC* noteText = StringLoader::LoadLC( R_VED_MMC_NOT_INSERTED,
                                                    iEikonEnv );
                CAknInformationNote* informationNote = 
                                new(ELeave)CAknInformationNote( ETrue );
                informationNote->ExecuteLD( *noteText );

                CleanupStack::PopAndDestroy( noteText );

				iMovieSaveSettings.MemoryInUse() = CAknMemorySelectionDialog::EPhoneMemory;
				// do not overwrite because doing so permanently sets memory to phone memory
				//STATIC_CAST( CVeiSimpleCutVideoAppUi*, AppUi() )->WriteSettingsL( settings );
                }
            }
        }

    LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoView::CheckMemoryCardAvailability(): out");
    }

void CVeiSimpleCutVideoView::PrepareForTermination()
	{
	iOnTheWayToDestruction = ETrue;
	}

void CVeiSimpleCutVideoView::SetTitlePaneTextL ()
{

	TPtrC fileName = iMovie->VideoClipInfo( iIndex )->FileName();
	HBufC * title_text = HBufC::NewLC( fileName.Length() );
	TPtr title_text_ptr = title_text->Des();
			
	title_text_ptr.Copy (fileName);
	TParsePtr parser (title_text_ptr); 
	title_text_ptr = parser.Name();
	
    //  Set title pane text
   	CEikStatusPane *statusPane = ( ( CAknAppUi* )iEikonEnv->EikAppUi() )->StatusPane();
    CAknTitlePane* titlePane = ( CAknTitlePane* ) statusPane->ControlL( TUid::Uid( EEikStatusPaneUidTitle ) );
	titlePane->SetTextL(title_text_ptr);
	
	CleanupStack::PopAndDestroy( title_text ); 
}

// End of File

