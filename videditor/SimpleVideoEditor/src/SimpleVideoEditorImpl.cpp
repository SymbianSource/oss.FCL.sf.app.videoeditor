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
#include "SimpleVideoEditorImpl.h"
#include <SimpleVideoEditor.rsg>
#include <e32std.h>
#include <aknutils.h>
#include <bautils.h>
#include <data_caging_path_literals.hrh>
#include <mgfetch.h> 
#include <sysutil.h>
#include <stringloader.h>
#include <aknnotewrappers.h> 
#include <eikenv.h>
#include <errorui.h>
#include <PathInfo.h>
#include <eikprogi.h>
#include <stringloader.h>
#include <VedAudioClipInfo.h>
#include <CAknMemorySelectionDialog.h> 
#include <CAknFileNamePromptDialog.h> 
#include <AknCommonDialogsDynMem.h>
#include <CAknMemorySelectionDialogMultiDrive.h>
#include <apgcli.h>

#include "VideoEditorUtils.h"
#include "VeiAddQueue.h"
#include "VideoEditorCommon.h"
#include "VideoEditorDebugUtils.h"
#include "VeiTempMaker.h"
#include "ExtProgressDialog.h"
#include "VeiMGFetchVerifier.h"
#include "VeiImageClipGenerator.h"
#include "DummyControl.h"
#include "CMultiLineQueryDialog.h"

// CONSTANTS
_LIT(KResourceFile, "SimpleVideoEditor.rsc");
const TProcessPriority KLowPriority = EPriorityLow;
const TInt KAudioLevelMin = -127;
const TUint KFadeInTimeMicroSeconds = 50000;

#define KMediaGalleryUID3           0x101F8599 

//=======================================================================================================
CSimpleVideoEditorImpl* CSimpleVideoEditorImpl::NewL(MSimpleVideoEditorExitObserver& aExitObserver)
	{
	CSimpleVideoEditorImpl* self = new (ELeave)	CSimpleVideoEditorImpl(aExitObserver);
	CleanupStack::PushL (self);
	self->ConstructL();
	CleanupStack::Pop (self);
	return self;
	}
	
//=======================================================================================================
CSimpleVideoEditorImpl::~CSimpleVideoEditorImpl()
	{
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::~CSimpleVideoEditorImpl, In");

	// Remove foreground event observer
	iEnv.RemoveForegroundObserver( *this );

	Cancel();
	iResLoader.Close();

	if ( iMovie )
		{
		iMovie->Reset();	
		iMovie->UnregisterMovieObserver( this );				
		delete iMovie;
		iMovie = NULL;
		}		

	if ( iTempFile )
		{
		(void) iEnv.FsSession().Delete( *iTempFile );
		delete iTempFile;
		iTempFile = NULL;
		}

    if ( iProgressDialog )
		{
		iProgressDialog->SetCallback( NULL );		
		delete iProgressDialog;
		iProgressDialog = NULL;
		}
		
	if ( iWaitDialog )
		{
		iWaitDialog->SetCallback(NULL);
		delete iWaitDialog;
		iWaitDialog = NULL;
		}
	if ( iAnimatedProgressDialog )
		{			
		delete iAnimatedProgressDialog;	
		}
		
	if (iImageClipGenerator)
		{
		delete iImageClipGenerator;
		}
		
	if (iTextGenerator)
		{
		delete iTextGenerator;	
		}
		
	if (iAddText)
		{
		delete iAddText;	
		}
	
	if ( iErrorUI )
		{
		delete iErrorUI;
		}
    
    if ( iAudioClipInfo )
        {
        delete iAudioClipInfo;
        }

	if ( iAcceptedAudioTypes )
		{
		delete iAcceptedAudioTypes;
		iAcceptedAudioTypes = 0;
		}

	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::~CSimpleVideoEditorImpl, Out");
    }

//=======================================================================================================
void CSimpleVideoEditorImpl::RestoreOrientation()
	{
	CAknAppUiBase* appUi = static_cast<CAknAppUiBase *>( iEnv.EikAppUi() );
	CAknAppUiBase::TAppUiOrientation orientation = appUi->Orientation();

	if (orientation != iOriginalOrientation)
		{
		TRAP_IGNORE( appUi->SetOrientationL(iOriginalOrientation) );

		// Send screen device change event to validate screen
		TWsEvent event;

		RWsSession& rws = iEnv.WsSession();
		event.SetType( EEventScreenDeviceChanged );
		event.SetTimeNow(); 
		event.SetHandle( rws.WsHandle() ); 

		(void)rws.SendEventToAllWindowGroups( event );
		}
	}

//=======================================================================================================
CSimpleVideoEditorImpl::CSimpleVideoEditorImpl(MSimpleVideoEditorExitObserver& aExitObserver) 
: 	CActive (EPriorityStandard), 
	iEnv( *CEikonEnv::Static() ), 
	iExitObserver (aExitObserver), 
	iResLoader( iEnv ),
	iGeneratorComplete( ETrue ),
	iDialogDismissed( EFalse )
	{
	CActiveScheduler::Add(this);   
	}

//=======================================================================================================
void CSimpleVideoEditorImpl::ConstructL()
	{
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::ConstructL: In");

	// Locate and open the resource file
    TFileName fileName;
    TParse p;    

    Dll::FileName(fileName);
    p.Set(KResourceFile, &KDC_RESOURCE_FILES_DIR, &fileName);
    fileName = p.FullName();
    
    LOGFMT(KVideoEditorLogFile, "\tLoading resource file: %S", &fileName);
	iResLoader.OpenL( fileName ); // RConeResourceLoader selects the correct language file

	// Always use automatic save quality for the result movie
	iMovie = CVedMovie::NewL( NULL );	
	iMovie->RegisterMovieObserverL( this );
	iMovie->SetQuality( CVedMovie::EQualityAutomatic );

	CVeiTempMaker* maker = CVeiTempMaker::NewLC();
	// this call can leave even though it does not end with 'L'
	maker->EmptyTempFolder();
	CleanupStack::PopAndDestroy(maker);
	//delete maker;
		
	iEnv.AddForegroundObserverL( *this );
	
		
	iErrorUI = CErrorUI::NewL( iEnv );

	iAcceptedAudioTypes = new ( ELeave ) CDesCArrayFlat( 4 );

	iAcceptedAudioTypes->Reset();

	iAcceptedAudioTypes->AppendL( _L( "audio/mpeg" ) );
	iAcceptedAudioTypes->AppendL( _L( "audio/aac" ) );
	iAcceptedAudioTypes->AppendL( _L( "audio/amr" ) );
	iAcceptedAudioTypes->AppendL( _L( "audio/mp3" ) );
	iAcceptedAudioTypes->AppendL( _L( "audio/x-mp3" ) );
	iAcceptedAudioTypes->AppendL( _L( "audio/3gpp" ) );
	iAcceptedAudioTypes->AppendL( _L( "audio/3gpp2" ) );
	iAcceptedAudioTypes->AppendL( _L( "audio/m4a" ) );
	iAcceptedAudioTypes->AppendL( _L( "audio/mp4" ) );
	iAcceptedAudioTypes->AppendL( _L( "audio/mpeg4" ) );
	iAcceptedAudioTypes->AppendL( _L( "audio/wav" ) );
	iAcceptedAudioTypes->AppendL( _L( "audio/x-wav" ) );
	iAcceptedAudioTypes->AppendL( _L( "audio/x-realaudio" ) );
	iAcceptedAudioTypes->AppendL( _L( "audio/wma" ) );
		
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::ConstructL: Out");
	}

//=======================================================================================================
void CSimpleVideoEditorImpl::StartMerge( const TDesC& aSourceFileName )
	{
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::StartMerge: In");

	iSourceFileName = aSourceFileName;
	iOperationMode = EOperationModeMerge;
	iState = EStateInitializing;

	CompleteRequest();

	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::StartMerge: Out");
	}

//=======================================================================================================
void CSimpleVideoEditorImpl::StartChangeAudio( const TDesC& aSourceFileName )
	{
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::StartChangeAudio: In");

	iSourceFileName = aSourceFileName;
	iOperationMode = EOperationModeChangeAudio;
	iState = EStateInitializing;

	CompleteRequest();

	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::StartChangeAudio: Out");
	}

//=======================================================================================================
void CSimpleVideoEditorImpl::StartAddText( const TDesC& aSourceFileName )
	{
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::StartAddText: In");

	iSourceFileName = aSourceFileName;
	iOperationMode = EOperationModeAddText;
	iState = EStateInitializing;

	CompleteRequest();

	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::StartAddText: Out");
	}


//=============================================================================
void CSimpleVideoEditorImpl::RunL()
	{	
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::RunL: In");

	// Resetting these indicators. This is not necessary right now,
	// but might be in the future if the code is changed so it's reasonable to
	// to do it anyway, just in case.
	iGeneratorComplete = ETrue;
	iDialogDismissed = EFalse;
	
	// if RunL() leaves, RunError() is called and iState is changed to EStateFinalizing
	// (or to EStateReady if already in EStateFinalizing
	
	switch (iState)
		{
		case EStateInitializing:
			{
			InitializeOperationL();

			iState = EStateInsertInputFirst;
			CompleteRequest();
			break;
			}
		case EStateInsertInputFirst:
			{
			// Common to all operation modes: Insert the original video clip to the movie
			// (Operation continues from the NotifyVideoClipAdded() callback method).
			iMovie->InsertVideoClipL( iSourceFileName, 0 );
			StartWaitDialogL();

			break;
			}
		case EStateInsertInputSecond:
			{
			// Get the input - text, image, video or sound clip
			switch (iOperationMode)
				{
				case EOperationModeMerge:
					{											
					GetMergeInputFileL();						
					CompleteRequest();	
					break;
					}
								
				case EOperationModeChangeAudio:
					{
					GetAudioFileL();
					CompleteRequest();
					break;
					}
				
				case EOperationModeAddText:
					{
					GetTextL();
					CompleteRequest();
					break;
					}
					
				default:
					User::Invariant();
					break;
				}			
				break;
			}				
	    case EStateCheckAudioLength:
	        {
	        // NotifyAudioClipInfoReady is called instead of RunL
	        break;    
	        }
		case EStateInsertVideo:
			{																				
			iMovie->InsertVideoClipL(iMergeFileName, iVideoOrImageIndex); 
			StartWaitDialogL();
			break;
			}
		case EStateCreateImageGenerator:	
			{																						
			TTimeIntervalMicroSeconds duration( 3000000 );
		    TRgb background = KRgbBlack;
		    // Setting to false to indicate that the
		    // NewL() called below hasn't yet completed.
		    iGeneratorComplete = EFalse;

			// Create the image clip generator
		    iImageClipGenerator = CVeiImageClipGenerator::NewL( 
    			iMergeFileName, 
    			iMovie->Resolution(),
    			duration, 
    			background, 
    			KVideoClipGenetatorDisplayMode, 
    			iEnv.FsSession(), 
    			*this );
    		StartWaitDialogL();	    						    			
			break;
			}
		case EStateInsertImage:
			{		
			TRAPD(err, iMovie->InsertVideoClipL( *iImageClipGenerator, ETrue, iVideoOrImageIndex )); // generator owned by movie 
			if (KErrNone == err)
				{				
				// Generator is no longer our concern, ownership transferred to iMovie							
				iImageClipGenerator = 0;				
				StartWaitDialogL();
				}
			else
				{					
				iError = err;
				iError = FilterError();
				iState = EStateFinalizing;
				CompleteRequest();
				}
			break;			
			}	
		
		case EStateInsertAudio:
			{
			LOGFMT(KVideoEditorLogFile, "CSimpleVideoEditorImpl::RunL: Inserting audio clip %S", &iAudioFileName);

			iMovie->AddAudioClipL( iAudioFileName, TTimeIntervalMicroSeconds( 0 ));    
			StartWaitDialogL();
			break;
			}
					
		case EStateInsertTextToBegin:
			{	
			LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::RunL: 1");
			
			
			iTextGenerator = CVeiTitleClipGenerator::NewL( iMovie->Resolution(),
																			   EVeiTitleClipTransitionNone, 
																			   EVeiTitleClipHorizontalAlignmentCenter,
																			   EVeiTitleClipVerticalAlignmentCenter);
			
			HBufC* descriptiveName = StringLoader::LoadLC(R_VESM_EDIT_VIEW_TITLE_NAME, &iEnv );
			iTextGenerator->SetDescriptiveNameL(*descriptiveName);
			CleanupStack::PopAndDestroy(descriptiveName);			
			iTextGenerator->SetTextL(*iAddText);
			
			iTextGenerator->SetTransitionAndAlignmentsL(EVeiTitleClipTransitionNone, 
														EVeiTitleClipHorizontalAlignmentCenter, 
														EVeiTitleClipVerticalAlignmentCenter);
			LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::RunL: 2");			
			TRAPD(err, iMovie->InsertVideoClipL( *iTextGenerator, ETrue, 0));	
												
			if (KErrNone == err)
				{				
				// Generator is no longer our concern, ownership transferred to iMovie							
				iTextGenerator = 0;				
				LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::RunL: 3");		
				StartWaitDialogL();
				}
			else
				{				
				iError = err;
				iError = FilterError();
				iState = EStateFinalizing;
				CompleteRequest();
				}		
			break;
			}
		
		case EStateInsertTextToEnd:
			{
			LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::RunL: 4");									
			
			iTextGenerator = CVeiTitleClipGenerator::NewL( iMovie->Resolution(),
																		   EVeiTitleClipTransitionScrollBottomToTop, 
																		   EVeiTitleClipHorizontalAlignmentCenter,
																		   EVeiTitleClipVerticalAlignmentCenter);
			
			HBufC* descriptiveName = StringLoader::LoadLC(R_VESM_EDIT_VIEW_TITLE_NAME, &iEnv );
			iTextGenerator->SetDescriptiveNameL(*descriptiveName);
			CleanupStack::PopAndDestroy(descriptiveName);			
			iTextGenerator->SetTextL(*iAddText);
			
			iTextGenerator->SetTransitionAndAlignmentsL(EVeiTitleClipTransitionScrollBottomToTop,
													EVeiTitleClipHorizontalAlignmentCenter, 
													EVeiTitleClipVerticalAlignmentCenter);
			LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::RunL: 5");
			
			TRAPD(err, iMovie->InsertVideoClipL( *iTextGenerator, ETrue, 1));
				
			if (KErrNone == err)
				{				
				// Generator is no longer our concern, ownership transferred to iMovie							
				iTextGenerator = 0;				
				LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::RunL: 6");
				StartWaitDialogL();
				}
			else
				{	
				iError = err;
				iError = FilterError();
				iState = EStateFinalizing;
				CompleteRequest();
				}
			break;	
			}	

		case EStateProcessing:
			{			
			StartMovieProcessingL(iSourceFileName);
			break;
			}
		
		case EStateProcessingOk:
			{			
			ProcessingOkL();
			CompleteRequest();
			break;
			}
				
		case EStateProcessingFailed:
			{			
			ProcessingFailed();
			CompleteRequest();					
			break;
			}
			
		case EStateFinalizing:
			{			
			// Show possible error dialog etc.
			// in TRAP because endless call loop may otherwise be resulted in case of leave in HandleErrorL
			// @ : should FilterError() be called only from here?
			HandleError();
			iState = EStateReady;	
			CompleteRequest();
			break;
			}
			
		case EStateReady:
			{				
			// Notify completion to observer
			iExitObserver.HandleSimpleVideoEditorExit( iError, iOutputFileName );
			break;
			}

		default:
			{
			User::Invariant();
			break;
			}
		}

	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::RunL: Out");
	}

//=============================================================================
void CSimpleVideoEditorImpl::DoCancel()
	{
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::DoCancel: In");
	CancelMovieProcessing();
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::DoCancel: Out");
	}
//=============================================================================	
	
void CSimpleVideoEditorImpl::CancelMovieProcessing()
	{
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::CancelMovieProcessing: In");

	if (iMovie)
		{
		iMovie->CancelProcessing();
		}	

	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::CancelMovieProcessing: Out");
	}	
//=============================================================================

TInt CSimpleVideoEditorImpl::RunError( TInt aError )
	{
	LOGFMT(KVideoEditorLogFile, "CSimpleVideoEditorImpl::RunError: %d", aError);
	//@ :  think how to solve this
	
	// Show possible error dialog etc.
	iError = aError;
	// @: if leave and error happens so that Notify callbacks are not called
	iError = FilterError();
	
	// if leave happens in HandleError, iState must be changed to prevent same leave happening eternally 
	if (EStateReady == iState)
		{
		iExitObserver.HandleSimpleVideoEditorExit( iError, iOutputFileName );
		}
	else
		{			
		iState = EStateReady;	
		HandleError();
		iExitObserver.HandleSimpleVideoEditorExit( iError, iOutputFileName );
		// If CompleteRequest() is called here, system crashes because ~CSimpleVideoEditorImpl() gets
		// called from iExitObserver.HandleSimpleVideoEditorExitL(), stray signal resulted
		//CompleteRequest();
		}							

	return KErrNone;	
	}
//=======================================================================================================

void CSimpleVideoEditorImpl::InitializeOperationL()
	{
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::InitializeOperationL: In");

	iOutputFileName.Zero();

	if( !AknLayoutUtils::PenEnabled() && EOperationModeAddText == iOperationMode )
		{
		// Text input is always inserted in portrait mode.
		// Store the original screen orientation.
		CAknAppUiBase* appUi = static_cast<CAknAppUiBase *>( iEnv.EikAppUi() );
		iOriginalOrientation = appUi->Orientation();
		appUi->SetOrientationL(CAknAppUiBase::EAppUiOrientationPortrait);
		}

	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::InitializeOperationL: Out");
	}

//=======================================================================================================
void CSimpleVideoEditorImpl::GetMergeInputFileL()
	{
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::GetMergeInputFileL: In");

	iVideoOrImageIndex = 0; 

	CDesCArrayFlat* selectedFiles = new ( ELeave ) CDesCArrayFlat( 1 );
	CleanupStack::PushL(selectedFiles);

	CDesCArrayFlat* mimetypesVideo = new ( ELeave ) CDesCArrayFlat( 1 );
	CleanupStack::PushL(mimetypesVideo);
	mimetypesVideo->AppendL(_L("video/*"));

	CDesCArrayFlat* mimetypesImage = new ( ELeave ) CDesCArrayFlat( 1 );
	CleanupStack::PushL(mimetypesImage);
	mimetypesImage->AppendL(_L("image/*"));
	
	TInt videoOrImage = -1;
	TBool chosen = EFalse;

	// Select the input type: video clip or image
	if ( ShowListQueryL(videoOrImage, R_VEI_QUERY_HEADING_MERGE_WITH, R_VED_VIDEO_OR_IMAGE_QUERY) )
		{
		CVeiMGFetchVerifier* mgFetchVerifier = CVeiMGFetchVerifier::NewLC();
		if (0 == videoOrImage) // video chosen
			{
			if ( MGFetch::RunL( *selectedFiles, EVideoFile, EFalse, KNullDesC(), KNullDesC(), mimetypesVideo, mgFetchVerifier))
				{
				chosen = ETrue;
				}
			}
		else if (1 == videoOrImage) // image chosen
			{
		    if ( MGFetch::RunL( *selectedFiles, EImageFile, EFalse, KNullDesC(), KNullDesC(), mimetypesImage, mgFetchVerifier))
				{
				chosen = ETrue;
				}
			}
		CleanupStack::PopAndDestroy( mgFetchVerifier );

		if (chosen)
			{
			iMergeFileName = selectedFiles->MdcaPoint(0);
			TInt headingResourceId = R_VIE_QUERY_HEADING_ADD_VIDEO_TO; 
			if (0 == videoOrImage) // video chosen
				{
				iOperationMode = EOperationModeMergeWithVideo;
				iState = EStateInsertVideo;				
				headingResourceId = R_VIE_QUERY_HEADING_ADD_VIDEO_TO; 
				LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::GetMergeInputFileL: 4");								
				} 
			else if (1 == videoOrImage) // image chosen
				{
				iOperationMode = EOperationModeMergeWithImage;
				iState = EStateCreateImageGenerator;			    			    
				headingResourceId = R_VIE_QUERY_HEADING_ADD_IMAGE_TO; 
				LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::GetMergeInputFileL: 5");				
				}

			TInt begOrEnd = -1;
			if ( ShowListQueryL(begOrEnd, headingResourceId, R_VED_INSERT_POSITION_QUERY) )
				{			
				LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::GetMergeInputFileL: 6");
																
				if (0 == begOrEnd) // video or image to the beginning
					{			
					iVideoOrImageIndex = 0;				
					}
				else // video or image to end 
					{				
					iVideoOrImageIndex = 1;				
					}		    
				}
			else
				{
				iState = EStateFinalizing;	
				}	
			} // if (chosen)
		else
			{
			iState = EStateFinalizing;	
			}
		} // if ( ShowListQueryL for image or video
	else
		{
		iState = EStateFinalizing;	
		}
	
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::GetMergeInputFileL: 7");
	CleanupStack::PopAndDestroy(mimetypesImage);
	CleanupStack::PopAndDestroy(mimetypesVideo);
	CleanupStack::PopAndDestroy(selectedFiles);
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::GetMergeInputFileL: Out");	
	}

//=======================================================================================================
void CSimpleVideoEditorImpl::GetAudioFileL()
	{
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::GetAudioFileL: In");

	TFileName outputFile;

	CDesCArrayFlat* selectedFiles = new ( ELeave ) CDesCArrayFlat( 1 );
	CleanupStack::PushL(selectedFiles);
	CVeiMGFetchVerifier* mgFetchVerifier = CVeiMGFetchVerifier::NewLC();

	if ( MGFetch::RunL( *selectedFiles, EAudioFile, EFalse, KNullDesC, KNullDesC , iAcceptedAudioTypes, mgFetchVerifier ) )
		{
        iAudioFileName = selectedFiles->MdcaPoint(0);
        iAudioClipInfo = CVedAudioClipInfo::NewL( iAudioFileName, *this );
        iState = EStateCheckAudioLength;
		}
	else
		{
		iState = EStateFinalizing;	
		}
				
	CleanupStack::PopAndDestroy( mgFetchVerifier );
	CleanupStack::PopAndDestroy(selectedFiles);			

	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::GetAudioFileL: Out");
	}
	
//=======================================================================================================
void CSimpleVideoEditorImpl::GetTextL()
	{	
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::GetTextL: In");

	// Ask for text. 
	iAddText = HBufC::NewL(AKNTEXT_QUERY_WIDTH * AKNTEXT_QUERY_LINES); // think what these limit values should be?	
	TPtr textPtr = iAddText->Des();
	CMultiLineQueryDialog* textQuery = CMultiLineQueryDialog::NewL(textPtr);	
	textQuery->SetMaxLength(AKNTEXT_QUERY_WIDTH * AKNTEXT_QUERY_LINES);	
	//textQuery->SetPredictiveTextInputPermitted(ETrue);

	if (textQuery->ExecuteLD(R_VESM_EDITVIDEO_TITLESCREEN_TEXT_QUERY))
		{					
		// Restore the original screen orientation immediately after the text input ends
		RestoreOrientation();
		
		TInt begOrEnd = -1;		
		
		if ( ShowListQueryL(begOrEnd, R_VEI_QUERY_HEADING_ADD_TEXT_TO, R_VED_INSERT_POSITION_QUERY) )
			{			
			LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::GetTextL: 1");
															
			if (0 == begOrEnd) // text to begin
				{			
				iState = EStateInsertTextToBegin;				
				}
			else // text to end in credits style (rolling down the screen)
				{				
				iState = EStateInsertTextToEnd;				
				}		    		    
			}
		else
			{
			iState = EStateFinalizing;	
			}
		}
	else
		{
		// Restore the original screen orientation immediately after the text input ends
		RestoreOrientation();
		iState = EStateFinalizing;
		}
	
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::GetTextL: Out");
	}

//=======================================================================================================
TInt CSimpleVideoEditorImpl::ShowListQueryL (TInt& aPosition, 
											 TInt aHeadingResourceId,
											 TInt aQueryResourceId) const
    {
    CAknListQueryDialog* dlg = new( ELeave ) CAknListQueryDialog( &aPosition );
    dlg->PrepareLC( aQueryResourceId );
    
    CAknPopupHeadingPane* heading = dlg->QueryHeading();
    HBufC* noteText = StringLoader::LoadLC( aHeadingResourceId, &iEnv );
    heading->SetTextL( noteText->Des() );
    CleanupStack::PopAndDestroy( noteText );

    return dlg->RunLD();        
    }

//=======================================================================================================
void CSimpleVideoEditorImpl::StartMovieProcessingL(const TDesC& aSourceFile)
	{
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::StartMovieProcessingL: In");

	RFs&	fs = iEnv.FsSession();

	if (QueryAndSaveL(aSourceFile)) 
	{	
		// Generate temp file.
		// Take the drive from the target file name.
		CAknMemorySelectionDialog::TMemory memory( CAknMemorySelectionDialog::EMemoryCard );
		if( 0 != iOutputFileName.Left(1).CompareF( PathInfo::MemoryCardRootPath().Left(1) ) )
			{
			memory = CAknMemorySelectionDialog::EPhoneMemory;
			}

		iTempFile = HBufC::NewL(KMaxFileName);
		CVeiTempMaker* maker = CVeiTempMaker::NewL();
		maker->GenerateTempFileName( *iTempFile, memory, iMovie->Format() );
		delete maker;

		// Start rendering video to the temporary file.
		LOGFMT(KVideoEditorLogFile, "CSimpleVideoEditorImpl::StartMovieProcessingL: 1, calling iMovie->ProcessL(%S)", iTempFile);
		iMovie->ProcessL(*iTempFile, *this);
	}
	else
	{
		LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::StartMovieProcessingL: User cancelled saving");
		iState = EStateProcessingFailed;
		CompleteRequest();
	}
	
	// Next: wait for MVedMovieProcessingObserver callback
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::StartMovieProcessingL: Out");
	}

//=============================================================================
TInt CSimpleVideoEditorImpl::QueryAndSaveL(const TDesC& aSourceFileName)
{

	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::QueryAndSaveL: in");

	TBool ret = EFalse;
	
	RFs	fs = CCoeEnv::Static()->FsSession();

	// launch query with choices "Replace original" and "Save with a new file name" 
	TInt userSelection = LaunchSaveVideoQueryL(); 
	
  	if(userSelection == 0) 
  	// the user selects to save with a new file name
		{
		CAknMemorySelectionDialog::TMemory selectedMemory(CAknMemorySelectionDialog::EPhoneMemory);		

        // Multiple drive support
#ifdef RD_MULTIPLE_DRIVE
		
		TDriveNumber driveNumber;
        TFileName driveAndPath;
        CAknMemorySelectionDialogMultiDrive* multiDriveDlg = CAknMemorySelectionDialogMultiDrive::NewL(ECFDDialogTypeSave, EFalse );			
		CleanupStack::PushL(multiDriveDlg);
		
		// launch "Select memory" query
        if (multiDriveDlg->ExecuteL( driveNumber, &driveAndPath, NULL ))
			{
			iOutputFileName.Zero();				
			
			// Generate a default name for the new file
			TInt err = VideoEditorUtils::GenerateFileNameL (
                                    					fs,
                                    					aSourceFileName,		
                                    					iOutputFileName,
                                    					iMovie->Format(),
                                    					iMovie->GetSizeEstimateL(),
                                    					driveAndPath);	
				
            driveAndPath.Append( PathInfo::VideosPath() );					
				
			if ( KErrNone == err )
				{				
				// launch file name prompt dialog
				if (CAknFileNamePromptDialog::RunDlgLD(iOutputFileName, driveAndPath, KNullDesC))
					{
					driveAndPath.Append(iOutputFileName);
					iOutputFileName = driveAndPath;
		            ret = ETrue;
		            }
				}
			else // err != KErrNone 
				{
				iErrorUI->ShowGlobalErrorNoteL( err );
				ret = EFalse;
				}											
			}
		CleanupStack::PopAndDestroy( multiDriveDlg );

#else // no multiple drive support
			
		// launch "Select memory" query
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
			iOutputFileName.Zero();
			TInt err = VideoEditorUtils::GenerateNewDocumentNameL (
				fs,
				aSourceFileName,		
				iOutputFileName,
				iMovie->Format(),
				iMovie->GetSizeEstimateL(),
				memorySelection);	
				
			if ( KErrNone == err )
				{				
				// launch file name prompt dialog
				if (CAknFileNamePromptDialog::RunDlgLD(iOutputFileName, driveAndPath, KNullDesC))
					{
					driveAndPath.Append(iOutputFileName);
					iOutputFileName = driveAndPath;
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
		iOutputFileName = aSourceFileName;
		return ETrue;	
		}
	else // user cancelled
		{
		ret = EFalse;
		}

	LOGFMT(KVideoEditorLogFile, "CSimpleVideoEditorImpl::QueryAndSaveL: out: %d", ret);

	return ret;
}



//=============================================================================
TInt CSimpleVideoEditorImpl::LaunchListQueryDialogL (
	MDesCArray *	aTextItems,
	const TDesC &	aPrompt
	) 
{

	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::LaunchListQueryDialogL: in");
	//	Selected text item index
	TInt index (-1);

	//	Create a new list dialog
    CAknListQueryDialog * dlg = new (ELeave) CAknListQueryDialog (&index);

	//	Prepare list query dialog
	dlg->PrepareLC (R_VIE_LIST_QUERY);

	//	Set heading
	dlg->QueryHeading()->SetTextL (aPrompt);

	//	Set text item array
	dlg->SetItemTextArray (aTextItems);	

	//	Set item ownership
	dlg->SetOwnershipType (ELbmDoesNotOwnItemArray);

	//	Execute
	if (dlg->RunLD())
	{
		LOGFMT(KVideoEditorLogFile, "CSimpleVideoEditorImpl::LaunchListQueryDialogL: out: return %d", index);	
		return index;
	}
	else
	{
		LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::LaunchListQueryDialogL: out: return -1");		
		return -1;
	}
}

//=============================================================================
TInt CSimpleVideoEditorImpl::LaunchSaveVideoQueryL () 
{
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::LaunchSaveVideoQueryL: in");
	//	Create dialog heading and options
    HBufC * heading = CEikonEnv::Static()->AllocReadResourceLC (R_VIE_QUERY_HEADING_SAVE);
    HBufC * option1 = CEikonEnv::Static()->AllocReadResourceLC (R_VIE_QUERY_SAVE_NEW);       
    HBufC * option2 = CEikonEnv::Static()->AllocReadResourceLC (R_VIE_QUERY_SAVE_REPLACE); 
                
	//	Query dialog texts
	CDesCArray * options = new (ELeave) CDesCArraySeg (2);
	CleanupStack::PushL (options);
	options->AppendL( option1->Des() );
	options->AppendL( option2->Des() );

	//	Execute query dialog
	TInt ret = LaunchListQueryDialogL (options, *heading);

	options->Reset();
	
	CleanupStack::PopAndDestroy( options ); 
	CleanupStack::PopAndDestroy( option2 ); 
	CleanupStack::PopAndDestroy( option1 ); 		
	CleanupStack::PopAndDestroy( heading ); 
	
	LOGFMT(KVideoEditorLogFile, "CSimpleVideoEditorImpl::LaunchListQueryDialogL: out: return %d", ret);			
	return ret;
}



//=======================================================================================================
void CSimpleVideoEditorImpl::HandleLosingForeground()
	{
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::HandleLosingForeground(): In");

	// Set the priority to low. This is needed to handle the situations 
	// where the engine is performing heavy processing while the application 
	// is in background.
	RProcess myProcess;
	iOriginalProcessPriority = myProcess.Priority();
	LOGFMT3(KVideoEditorLogFile, "CSimpleVideoEditorImpl::HandleLosingForeground: changing priority of process %Ld from %d to %d", myProcess.Id().Id(), iOriginalProcessPriority, KLowPriority);
	myProcess.SetPriority( KLowPriority );
	iProcessPriorityAltered = ETrue;

	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::HandleLosingForeground(): Out");
	}

//=======================================================================================================
void CSimpleVideoEditorImpl::HandleGainingForeground()
	{
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::HandleGainingForeground(): In");

	// Return to normal priority.
	RProcess myProcess;
	TProcessPriority priority = myProcess.Priority();
	if ( priority < iOriginalProcessPriority )
		{
		myProcess.SetPriority( iOriginalProcessPriority );
		}
	iProcessPriorityAltered = EFalse;

	LOGFMT2(KVideoEditorLogFile, "CSimpleVideoEditorImpl::HandleGainingForeground: Out: process %Ld back to normal priority %d", myProcess.Id().Id(), priority);
	}

//=======================================================================================================
TInt CSimpleVideoEditorImpl::FilterError(/*const TInt& aErrEngine, const TInt& aErrUi*/) const
	{
	// here standard leave codes are converted to our own codes having correspondent localized error messages						
	if (KErrNone != iError && KErrCancel != iError)
		{		
		if (EStateInsertInputFirst == iState )
			{
			return KErrUnableToEditVideo;
			}
		else if (EStateInsertVideo == iState)
		    {
		    if (KErrNotSupported == iError)	
				{
				return KErrVideoFormatNotSupported;	
				}
			else return KErrUnableToInsertVideo;
		    }
		else if (EStateInsertImage == iState || EStateCreateImageGenerator == iState)
			{
			if (KErrNotSupported == iError)	
				{
				return KErrImageFormatNotSupported;	
				}		
			else return KErrUnableToInsertImage;
			}		
		else if (EStateInsertAudio == iState || EStateCheckAudioLength == iState )
			{
			if (KErrNotSupported == iError)	
				{
				return KErrAudioFormatNotSupported;	
				}
			else 
				{
				return KErrUnableToInsertSound;
				}
			}
		else if (EStateInsertTextToBegin == iState || EStateInsertTextToEnd == iState)
			{		
			return KErrUnableToInsertText;	
			}			
		else if (EOperationModeMergeWithVideo == iOperationMode)
			{
			return KErrUnableToMergeVideos;
			} 
		else if (EOperationModeMergeWithImage == iOperationMode)
			{
			return KErrUnableToMergeVideoAndImage;
			} 
		else if (EOperationModeChangeAudio == iOperationMode)
			{
			return KErrUnableToChangeSound;
			}
		else if (EOperationModeAddText == iOperationMode)
			{
			if (iError == KErrNoMemory) return KErrNoMemory;
			return KErrUnableToInsertText;
			} 
		}
	
	return iError;					
	}


//=============================================================================
void CSimpleVideoEditorImpl::CompleteRequest()
	{
	if ( IsActive() )
		{
		Cancel();
		}
	TRequestStatus * p = &iStatus;
	SetActive();
	User::RequestComplete (p, KErrNone);
	}

//=============================================================================
void CSimpleVideoEditorImpl::HandleError()
	{		
	LOGFMT(KVideoEditorLogFile, "CSimpleVideoEditorImpl::HandleErrorL: In, iError:%d", iError);

	if (KErrNone != iError && KErrCancel != iError)
		{
						
		if (iTempFile)
			{			
			TInt delErr = iEnv.FsSession().Delete( *iTempFile );
			LOGFMT(KVideoEditorLogFile, "CSimpleVideoEditorImpl::HandleErrorL: 1, delErr:%d", delErr);
			delete iTempFile;
			iTempFile = NULL;
			}
		
		switch (iError)
			{
		    case KErrInUse:
	            {
	            ShowErrorNote(R_VEI_IN_USE);
                break;	            
	            }
		    case KErrUnableToEditVideo:
	            {
	            ShowErrorNote(R_VEI_UNABLE_TO_EDIT);
                break;	            
	            }
			case KErrUnableToInsertVideo:
				{
				ShowErrorNote(R_VEI_UNABLE_TO_INSERT_VIDEO);
				break;
				}
			case KErrUnableToInsertSound:
				{
				ShowErrorNote(R_VEI_UNABLE_TO_INSERT_SOUND);
				break;
				}
			case KErrUnableToInsertImage:
				{
				ShowErrorNote(R_VEI_UNABLE_TO_INSERT_IMAGE);
				break;
				}
			case KErrUnableToInsertText:
				{
				ShowErrorNote(R_VEI_UNABLE_TO_INSERT_TEXT);
				break;
				}
			case KErrUnableToMergeVideos:
				{
				ShowErrorNote(R_VEI_UNABLE_TO_MERGE_VIDEOS);
				break;
				}		
			case KErrUnableToMergeVideoAndImage:
				{
				ShowErrorNote(R_VEI_UNABLE_TO_MERGE_VIDEO_IMAGE);
				break;
				}
			case KErrUnableToChangeSound:
				{
				ShowErrorNote(R_VEI_UNABLE_TO_CHANGE_SOUND);
				break;
				}
			case KErrVideoFormatNotSupported:
				{
				ShowErrorNote(R_VEI_UNABLE_TO_INSERT_VIDEO);
				break;
				}
			case KErrAudioFormatNotSupported:
				{
				ShowErrorNote(R_VEI_AUDIO_FORMAT_NOT_SUPPORTED);
				break;
				}
			case KErrImageFormatNotSupported:
				{
				ShowErrorNote(R_VEI_IMAGE_FORMAT_NOT_SUPPORTED);
				break;
				}					
			case KErrNoMemory:
				{
				ShowErrorNote(R_VEI_NOT_ENOUGH_MEMORY);
				break;
				}	
			default:
				{			
				LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::HandleErrorL: 4");
				ShowErrorNote(R_VEI_ERROR_NOTE);
				LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::HandleErrorL: 5");
				break;
				}
			}
		}

	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::HandleErrorL: Out");
	}

//=============================================================================
void CSimpleVideoEditorImpl::ShowErrorNote( const TInt aResourceId ) const
	{
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::ShowErrorNoteL: In");

	TRAP_IGNORE(
		HBufC* stringholder = StringLoader::LoadLC( aResourceId, &iEnv );								
		CAknErrorNote* dlg = new ( ELeave ) CAknErrorNote( ETrue );
		dlg->ExecuteLD( *stringholder );
		CleanupStack::PopAndDestroy( stringholder ); 
		);

	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::ShowErrorNoteL: out");
	}

//=======================================================================================================
void CSimpleVideoEditorImpl::StartWaitDialogL()
	{
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::StartWaitDialogL: In");
	if (iWaitDialog)
		{
		LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::StartWaitDialogL: 2");
		delete iWaitDialog;
		iWaitDialog = NULL;	
		}
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::StartWaitDialogL: 3");

	iWaitDialog = new ( ELeave ) CAknWaitDialog( 
		reinterpret_cast<CEikDialog**>(&iWaitDialog), ETrue ); // !!!
	iWaitDialog->PrepareLC(R_VEI_WAIT_NOTE_WITH_CANCEL);
	iWaitDialog->SetCallback( this );

	HBufC* stringholder = StringLoader::LoadLC( R_VEI_NOTE_PROCESSING, &iEnv );
	iWaitDialog->SetTextL( *stringholder );	
	CleanupStack::PopAndDestroy(stringholder);

	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::StartWaitDialogL: 4");
	iWaitDialog->RunLD();
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::StartWaitDialogL: Out");
	}

//=======================================================================================================
void CSimpleVideoEditorImpl::StartProgressDialogL()
	{
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::StartProgressDialogL: In");	
	
	if (iProgressDialog)
		{		
		LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::StartProgressDialogL: 1");
		delete iProgressDialog;
		iProgressDialog = NULL;
		}
	
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::StartProgressDialogL: 2");	
	
	iProgressDialog = new (ELeave) CAknProgressDialog( 
		reinterpret_cast<CEikDialog**>(&iProgressDialog), ETrue );
	iProgressDialog->PrepareLC(R_VESM_PROGRESS_NOTE_WITH_CANCEL);	
	iProgressDialog->SetCallback( this );

	TInt resId = -1;
	HBufC* stringholder = NULL;
	switch (iOperationMode)
		{
		case EOperationModeMergeWithVideo:
		case EOperationModeMergeWithImage:
		    TApaAppCaption caption;
		    TRAPD( err, ResolveCaptionNameL( caption ) );
		    
		    // If something goes wrong, show basic "Saving" note
		    if ( err )
		        {
		        stringholder = iEnv.AllocReadResourceLC( R_VEI_NOTE_PROCESSING );
		        }
		    else
		        {
		        stringholder =  StringLoader::LoadLC( R_VEI_NOTE_MERGING, caption, &iEnv );
		        }        
			break;
		case EOperationModeChangeAudio:
			resId = R_VEI_NOTE_ADDING_AUDIO;
			stringholder = StringLoader::LoadLC( resId, &iEnv );
			break;
		case EOperationModeAddText:
			resId = R_VEI_NOTE_ADDING_TEXT;
			stringholder = StringLoader::LoadLC( resId, &iEnv );
			break;
		default :
			resId = R_VEI_NOTE_PROCESSING;
			stringholder = StringLoader::LoadLC( resId, &iEnv );
			break;
		}

	iProgressDialog->SetTextL( *stringholder );
	CleanupStack::PopAndDestroy(stringholder);

	iProgressDialog->GetProgressInfoL()->SetFinalValue( 100 );
	iProgressDialog->RunLD();
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::StartProgressDialogL: Out");	
	}

//=============================================================================
void CSimpleVideoEditorImpl::ResolveCaptionNameL( TApaAppCaption& aCaption ) const
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

//=======================================================================================================	
void CSimpleVideoEditorImpl::StartAnimatedProgressDialogL ()
	{
    delete iAnimatedProgressDialog;
    iAnimatedProgressDialog = NULL;
	iAnimatedProgressDialog = new (ELeave) CExtProgressDialog( &iAnimatedProgressDialog );
	
	iAnimatedProgressDialog->PrepareLC(R_WAIT_DIALOG);	
	iAnimatedProgressDialog->SetCallback( this );

	TInt labelResId = -1;
	TInt animResId = -1;
	switch (iOperationMode)
		{
		case EOperationModeMerge:
			labelResId = R_VEI_NOTE_MERGING;
			animResId = VideoEditor::EAnimationMerging;
			break;
		case EOperationModeChangeAudio:
			labelResId = R_VEI_NOTE_ADDING_AUDIO;
			animResId = VideoEditor::EAnimationChangeAudio;
			break;
		case EOperationModeAddText:
			labelResId = R_VEI_NOTE_ADDING_TEXT;
			animResId = VideoEditor::EAnimationAddText;
			break;
		default :
			labelResId = R_VEI_NOTE_PROCESSING;
			// @ : what is best default animation?
			animResId = VideoEditor::EAnimationMerging;
			break;
		}

	HBufC* stringholder = StringLoader::LoadLC( labelResId, &iEnv );
	iAnimatedProgressDialog->SetTextL( *stringholder );
	CleanupStack::PopAndDestroy(stringholder);
		
	iAnimatedProgressDialog->SetAnimationResourceIdL( animResId );

	iAnimatedProgressDialog->GetProgressInfoL()->SetFinalValue( 100 );
	iAnimatedProgressDialog->StartAnimationL();
	iAnimatedProgressDialog->RunLD();		 	    
	}


//=======================================================================================================
void CSimpleVideoEditorImpl::NotifyMovieProcessingStartedL(CVedMovie& /*aMovie*/)
	{
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::NotifyMovieProcessingStartedL: In");

	iPercentagesProcessed = 0;

	StartProgressDialogL();
	//StartAnimatedProgressDialogL();

	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::NotifyMovieProcessingStartedL: Out");
	}

//=======================================================================================================
void CSimpleVideoEditorImpl::NotifyMovieProcessingProgressed(CVedMovie& /*aMovie*/, TInt aPercentage)
    {
    iPercentagesProcessed = aPercentage;
   	User::ResetInactivityTime();
   	if (iAnimatedProgressDialog)
   	    {   		
		TRAP_IGNORE( iAnimatedProgressDialog->GetProgressInfoL()->SetAndDraw( aPercentage ) );
   	    }

   	if (iProgressDialog)
   	    {
		TRAP_IGNORE( iProgressDialog->GetProgressInfoL()->SetAndDraw( aPercentage ) );
   	    }
    
    if ( iCancelPercentage <= aPercentage )
        {
        iCancelPercentage = 101;
        }
    }

//=======================================================================================================
void CSimpleVideoEditorImpl::NotifyMovieProcessingCompleted(CVedMovie& /*aMovie*/, TInt aError)
    {    
    LOGFMT2(KVideoEditorLogFile, "CSimpleVideoEditorImpl::NotifyMovieProcessingCompleted: In, aError:%d, iPercentagesProcessed:%d", aError, iPercentagesProcessed);		

	iError = aError;	
	iError = FilterError();
    
    if (KErrNone == aError)
		{		
		LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::NotifyMovieProcessingCompleted: 1");
		iState = EStateProcessingOk;
		}
    else 
		{		
		LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::NotifyMovieProcessingCompleted: 2");
		iState = EStateProcessingFailed;
		}
    
    if (iAnimatedProgressDialog)
   		{
   		TRAP_IGNORE( iAnimatedProgressDialog->GetProgressInfoL()->SetAndDraw( 100 ) );
		delete iAnimatedProgressDialog;
		iAnimatedProgressDialog = NULL;
   		}
    if (iProgressDialog)
    	{
    	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::NotifyMovieProcessingCompleted: 3");
		TRAP_IGNORE( iProgressDialog->GetProgressInfoL()->SetAndDraw( 100 ) );
		TRAP_IGNORE( iProgressDialog->ProcessFinishedL() );
		LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::NotifyMovieProcessingCompleted: 4");
    	}

    // CompleteRequest() moved to DialogDismissed()    
    LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::NotifyMovieProcessingCompleted: Out");
    }
    
    
    
//=======================================================================================================    
void CSimpleVideoEditorImpl::NotifyAudioClipInfoReady( CVedAudioClipInfo& aInfo, TInt aError )
    {    
    
    LOGFMT( KVideoEditorLogFile, "CVeiEditVideoView::NotifyAudioClipInfoReady: in, aError:%d", aError );
        
    if ( aError == KErrNone )
        {        
        LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyAudioClipInfoReady: 1" );

        TTimeIntervalMicroSeconds audioDuration = aInfo.Duration();
        TTimeIntervalMicroSeconds videoDuration = iMovie->Duration();
        TInt changeSound = 1;

        TRAP_IGNORE( changeSound = QueryAudioInsertionL( videoDuration, audioDuration ));
        
        if ( changeSound )
            {
            iState = EStateInsertAudio;
            }
        else 
            {
            iState = EStateFinalizing;             
            }
        CompleteRequest();
        
        LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyAudioClipInfoReady: 2" );
        }
    else
        {
        iError = aError;
        iError = FilterError();
        iState = EStateFinalizing;
        CompleteRequest();   
        }

    LOG( KVideoEditorLogFile, "CVeiEditVideoView::NotifyAudioClipInfoReady: out" );    
    }
    
    
//=======================================================================================================
TInt CSimpleVideoEditorImpl::QueryAudioInsertionL( TTimeIntervalMicroSeconds aVideoDuration, 
                                                   TTimeIntervalMicroSeconds aAudioDuration )
    {
    TInt changeSound = 1;
    
    // round the durations to seconds so that the comparing will be more realistic
    TTimeIntervalSeconds videoDurationInSeconds = aVideoDuration.Int64()/1000000;
    TTimeIntervalSeconds audioDurationInSeconds = aAudioDuration.Int64()/1000000;
        
    if ( audioDurationInSeconds < videoDurationInSeconds )
        {
        HBufC* queryString = StringLoader::LoadLC( R_VIE_QUERY_INSERT_SHORT_AUDIO );
        CAknQueryDialog* dlg = new( ELeave )CAknQueryDialog( *queryString, CAknQueryDialog::ENoTone );
        changeSound = dlg->ExecuteLD( R_VIE_CONFIRMATION_QUERY );
        CleanupStack::PopAndDestroy( queryString );
        }
    else if ( audioDurationInSeconds > videoDurationInSeconds )
        {
        HBufC* queryString = StringLoader::LoadLC( R_VIE_QUERY_INSERT_LONG_AUDIO );
        CAknQueryDialog* dlg = new( ELeave )CAknQueryDialog( *queryString, CAknQueryDialog::ENoTone );
        changeSound = dlg->ExecuteLD( R_VIE_CONFIRMATION_QUERY );
        CleanupStack::PopAndDestroy( queryString );            
        }
    //else the audio clip is the same length as the video clip

    return changeSound;
    }
    
    
    
//=======================================================================================================

void CSimpleVideoEditorImpl::ProcessingOkL()
	{
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::ProcessingOkL: In");
	RFs& fs = iEnv.FsSession();
	CFileMan* fileman = CFileMan::NewL( fs );	
	CleanupStack::PushL( fileman );
	
	TInt moveErr( KErrNone );
	if ( iTempFile->Left(1) == iOutputFileName.Left(1) )
		{
		moveErr = fileman->Rename( *iTempFile, iOutputFileName );
		LOGFMT(KVideoEditorLogFile, "CSimpleVideoEditorImpl::ProcessingOkL: 1 renamed temp file: err %d", moveErr);
		}
	else
		{
		moveErr = fileman->Move( *iTempFile, iOutputFileName );
		LOGFMT(KVideoEditorLogFile, "CSimpleVideoEditorImpl::ProcessingOkL: 2 moved tem file: err %d", moveErr);
		}
	CleanupStack::PopAndDestroy( fileman );  

	delete iTempFile;
	iTempFile = NULL;		
	iError = moveErr;

	iState = EStateFinalizing;	
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::ProcessingOkL: Out");
	}
//=======================================================================================================	

void CSimpleVideoEditorImpl::ProcessingFailed()
	{		
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::ProcessingFailed: In");
	TInt delErr = iEnv.FsSession().Delete( *iTempFile );
	LOGFMT(KVideoEditorLogFile, "CSimpleVideoEditorImpl::ProcessingFailed: 1, delErr:%d", delErr);
	if ( delErr ) 
		{
		// @: should something be done?
		}
	delete iTempFile;
	iTempFile = NULL;		
	iState = EStateFinalizing;	
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::ProcessingFailed: Out");
	}	

//=======================================================================================================
void CSimpleVideoEditorImpl::NotifyVideoClipAdded(CVedMovie& /*aMovie*/, TInt /*aIndex*/)
    {
    LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::NotifyVideoClipAdded: In");	   	

	if ( EStateInsertInputFirst == iState )
		{
		// Next insert the second item (i.e. video|image|audio|text)
		iState = EStateInsertInputSecond;
		}
	else if ( EStateInsertVideo == iState ||	EStateInsertImage == iState 
				|| EStateInsertTextToBegin == iState || EStateInsertTextToEnd == iState)
		{
		// Next start processing the movie		
		iState = EStateProcessing;
		}	
	// if cancel is pressed in the middle of iMovie->InsertVideoClip(), state is put to iStateFinalizing to stop
	// the process			
	else if ( EStateFinalizing == iState || EStateReady == iState )	
		{
		;
		}
	else
		{
		User::Invariant();
		}
		
	if ( iWaitDialog )
		{		
		TRAP_IGNORE(iWaitDialog->ProcessFinishedL());
		}	
	//CompleteRequest();

    LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::NotifyVideoClipAdded: Out");
    }

//=======================================================================================================
void CSimpleVideoEditorImpl::NotifyVideoClipAddingFailed(CVedMovie& /*aMovie*/, TInt aError)
	{
	LOGFMT(KVideoEditorLogFile, "CSimpleVideoEditorImpl::NotifyVideoClipAddingFailed: In, aError:%d", aError);
	
	iError = aError;		
	iError = FilterError();
	
	// Next handle error and exit
	iState = EStateFinalizing;

	if ( iWaitDialog )
		{
		TRAP_IGNORE(iWaitDialog->ProcessFinishedL());
		}

	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::NotifyVideoClipAddingFailed: Out");
	}



//=======================================================================================================
void CSimpleVideoEditorImpl::NotifyAudioClipAdded(CVedMovie& /*aMovie*/, TInt /*aIndex*/)
	{
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::NotifyAudioClipAdded: In");

	ASSERT(iMovie && iMovie->VideoClipCount() == 1);

	if ( EStateInsertAudio == iState )
		{
		TInt audioClipCount = iMovie->AudioClipCount();
		TTimeIntervalMicroSeconds currentAudioClipEndTime = iMovie->AudioClipEndTime( audioClipCount - 1 ); // in movie timebase

		TTimeIntervalMicroSeconds videoClipEndTime = iMovie->VideoClipEndTime(0); // in movie timebase
		if( currentAudioClipEndTime > videoClipEndTime )
			{
			// Adjust the length so that the audio duration does not exceed video duration. 				
			CVedAudioClipInfo* audioClip = iMovie->AudioClipInfo( audioClipCount - 1 );
			TInt64 cutOutTime = audioClip->Duration().Int64() - ( currentAudioClipEndTime.Int64() - videoClipEndTime.Int64() );
			iMovie->AudioClipSetCutOutTime( audioClipCount - 1, TTimeIntervalMicroSeconds(cutOutTime) ); // in clip timebase
			}
		ASSERT( iMovie->Duration() == videoClipEndTime );
		
		TTimeIntervalMicroSeconds audioClipEndTime = iMovie->AudioClipEndTime( audioClipCount - 1);				

        ASSERT( audioClipEndTime.Int64() <= videoClipEndTime.Int64() );

        TInt error = KErrNone;
        
		if ( audioClipEndTime == videoClipEndTime )
		    {
		    if ( iMovie->VideoClipIsMuteable(0) )
		        {
		        iMovie->VideoClipSetMuted(0, ETrue);
		        }
		    } 
		    
        // if the audio clip is shorter than the video clip, the original sound 
        // of the video clip has to be muted until the end of the new sound clip
		else if ( audioClipEndTime < videoClipEndTime )
		    {
		
		    if ( iMovie->VideoClipEditedHasAudio(0) )
		        {
    		
		        TVedDynamicLevelMark mark1( TTimeIntervalMicroSeconds(0), KAudioLevelMin );
    		    
    		    TInt64 time = audioClipEndTime.Int64() - KFadeInTimeMicroSeconds; 
    		    
		        TVedDynamicLevelMark mark2( TTimeIntervalMicroSeconds(time) , KAudioLevelMin );
    		    
    		    TVedDynamicLevelMark mark3( audioClipEndTime, 0 );
    		
    		    TRAP( error, iMovie->VideoClipInsertDynamicLevelMarkL( 0, mark1 ) );
    		    
    		    if ( error == KErrNone )
    		        {
    		        TRAP( error, iMovie->VideoClipInsertDynamicLevelMarkL( 0, mark2 ) );
    		        }
    		        
    		    if ( error == KErrNone )
    		        {		    
    		        TRAP( error, iMovie->VideoClipInsertDynamicLevelMarkL( 0, mark3 ) );
    		        }
    		        
    		    if ( error != KErrNone ) 
        		    {
        		    iError = error;	
    	            iError = FilterError();		        		    
        		    }
        		        
		        }
		    }
		    
		if ( error == KErrNone )
		    {
		    iState = EStateProcessing;        
		    }
		else
		    {
		    iState = EStateProcessingFailed;    
		    }
		}
		
	if( iWaitDialog )
		{
		TRAP_IGNORE(iWaitDialog->ProcessFinishedL());
		}	
		
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::NotifyAudioClipAdded: Out");
	}

//=======================================================================================================
void CSimpleVideoEditorImpl::NotifyAudioClipAddingFailed(CVedMovie& /*aMovie*/, TInt aError)
    {    
    LOGFMT(KVideoEditorLogFile, "CSimpleVideoEditorImpl::NotifyAudioClipAddingFailed: In, aError:%d", aError);

    iError = aError;	    
		
	iError = FilterError();	

	// Next handle error and exit
	iState = EStateFinalizing;
	if ( iWaitDialog )
		{
		TRAP_IGNORE(iWaitDialog->ProcessFinishedL());
		}

    LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::NotifyAudioClipAddingFailed: Out");
    }

//=======================================================================================================
void CSimpleVideoEditorImpl::NotifyImageClipGeneratorInitializationComplete(CVeiImageClipGenerator& /*aGenerator*/, TInt aError)
	{
	LOGFMT(KVideoEditorLogFile, "CSimpleVideoEditorImpl::NotifyImageClipGeneratorInitializationComplete, In, aError:%d", aError);

	iError = aError;			
	
	// If the wait dialog has been dismissed, i.e the action has been cancelled,
	// we expect that the DialogDismissedL() method has set the correct state and won't
	// set the state ourselves.
	iGeneratorComplete = ETrue;
	if( !iDialogDismissed ) 
		{
		if (KErrNone == aError)
			{
			iState = EStateInsertImage;
			LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::NotifyImageClipGeneratorInitializationComplete: 2");		    
			}
		else
			{		
			iError = FilterError();
			// Next handle error and exit				
					
			/*
			If iImageClipGenerator is deleted here (in NotifyImageClipGeneratorInitializationComplete), endless
			loop is  resulted, because calling iImageClipGenerator's destructor leads to calling NotifyImageClipGeneratorInitializationComplete
			again until stack overdoses etc.
			iImageClipGenerator does not have to be deleted here, it is deleted in destructor.
			delete iImageClipGenerator;
			iImageClipGenerator = 0;		
			*/
		
			iState = EStateFinalizing;
			}
		}

	CompleteRequest();	

	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::NotifyImageClipGeneratorInitializationComplete, Out");
	}

//=======================================================================================================
void CSimpleVideoEditorImpl::DialogDismissedL( TInt aButtonId )
	{
	LOGFMT(KVideoEditorLogFile, "CSimpleVideoEditorImpl::DialogDismissedL, In, aButtonId:%d", aButtonId);

	if ( aButtonId != EAknSoftkeyDone)
		{
		// If the action was cancelled, we set up the correspondig state
		// and also set the iDialogDismissed property to indicate that.
		iDialogDismissed = ETrue;
		
		LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::DialogDismissedL, 1");
	
		if (EStateProcessing == iState && iMovie)
			{
			LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::DialogDismissedL, 2, canceling processing...");
			iState = EStateProcessingFailed;
			/* 
			 It depends on scheduling of active objects whether NotifyMovieProcessingCompleted()
			 gets called. If iProgressDialog is NULLed by itself in the time DialogDismissed() gets called
			 it should be nonrelevant whether NotifyMovieProcessingCompleted() called or not because
			 in NotifyMovieProcessingCompleted() iProgressDialog->ProcessFinished() is called only if
			 it is not NULL leading to another call to DialogDismissed(). But even if DialogDismissed()
			 gets called multiple times it should not cause any harm.
			*/ 
			iMovie->CancelProcessing();									
			LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::DialogDismissedL, 4, ...canceling called");
			}
		else
			{
			// if wait dialog is canceled during inserts
			iError = KErrCancel;
			iState = EStateFinalizing;	
			}
		
		// this problem is because in add text mode orientation is forced to portrait in 
		// such an early stage
		// ask Heikki's opinion, why appUi->SetOrientationL(CAknAppUiBase::EAppUiOrientationPortrait);
		// is called in InitializeOperationL()?
		// Can it be moved to where it is neede, in GetText()?
		if (EOperationModeAddText == iOperationMode)
			{
			RestoreOrientation();
			}
		}
	
	// Only if the VeiImageClipGenerator::NewL() called from the RunL() has finished and called 
	// the NotifyImageClipGeneratorInitializationComplete() method we complete and activate again.
	// If not, we rely that the NotifyImageClipGeneratorInitializationComplete() method
	// calls CompleteRequest() when it's called. 
	if( iGeneratorComplete )
		{
		CompleteRequest();		
		}
	
	LOG(KVideoEditorLogFile, "CSimpleVideoEditorImpl::DialogDismissedL, Out");
	}

//=======================================================================================================
void CSimpleVideoEditorImpl::NotifyVideoClipRemoved(CVedMovie& /*aMovie*/, TInt /*aIndex*/){}
void CSimpleVideoEditorImpl::NotifyVideoClipIndicesChanged(CVedMovie& /*aMovie*/, TInt /*aOldIndex*/, TInt /*aNewIndex*/){}
void CSimpleVideoEditorImpl::NotifyVideoClipTimingsChanged(CVedMovie& /*aMovie*/, TInt /*aIndex*/){}
void CSimpleVideoEditorImpl::NotifyVideoClipSettingsChanged(CVedMovie& /*aMovie*/, TInt /*aIndex*/){}
void CSimpleVideoEditorImpl::NotifyVideoClipColorEffectChanged(CVedMovie& /*aMovie*/, TInt /*aIndex*/){}
void CSimpleVideoEditorImpl::NotifyVideoClipAudioSettingsChanged(CVedMovie& /*aMovie*/, TInt /*aIndex*/){}
void CSimpleVideoEditorImpl::NotifyVideoClipGeneratorSettingsChanged(CVedMovie& /*aMovie*/, TInt /*aIndex*/){}
void CSimpleVideoEditorImpl::NotifyVideoClipDescriptiveNameChanged(CVedMovie& /*aMovie*/, TInt /*aIndex*/){}
void CSimpleVideoEditorImpl::NotifyStartTransitionEffectChanged(CVedMovie& /*aMovie*/){}
void CSimpleVideoEditorImpl::NotifyMiddleTransitionEffectChanged(CVedMovie& /*aMovie*/, TInt /*aIndex*/){}
void CSimpleVideoEditorImpl::NotifyEndTransitionEffectChanged(CVedMovie& /*aMovie*/){}
void CSimpleVideoEditorImpl::NotifyAudioClipRemoved(CVedMovie& /*aMovie*/, TInt /*aIndex*/){}
void CSimpleVideoEditorImpl::NotifyAudioClipIndicesChanged(CVedMovie& /*aMovie*/, TInt /*aOldIndex*/, TInt /*aNewIndex*/){}
void CSimpleVideoEditorImpl::NotifyAudioClipTimingsChanged(CVedMovie& /*aMovie*/, TInt /*aIndex*/){}
void CSimpleVideoEditorImpl::NotifyMovieQualityChanged(CVedMovie& /*aMovie*/){}
void CSimpleVideoEditorImpl::NotifyMovieReseted(CVedMovie& /*aMovie*/){}
void CSimpleVideoEditorImpl::NotifyMovieOutputParametersChanged(CVedMovie& /*aMovie*/){}
void CSimpleVideoEditorImpl::NotifyAudioClipDynamicLevelMarkInserted(CVedMovie& /*aMovie*/, TInt /*aClipIndex*/, TInt /*aMarkIndex*/){}
void CSimpleVideoEditorImpl::NotifyAudioClipDynamicLevelMarkRemoved(CVedMovie& /*aMovie*/, TInt /*aClipIndex*/, TInt /*aMarkIndex*/){}
void CSimpleVideoEditorImpl::NotifyVideoClipDynamicLevelMarkInserted(CVedMovie& /*aMovie*/, TInt /*aClipIndex*/, TInt /*aMarkIndex*/){}
void CSimpleVideoEditorImpl::NotifyVideoClipDynamicLevelMarkRemoved(CVedMovie& /*aMovie*/, TInt /*aClipIndex*/, TInt /*aMarkIndex*/){}
// End of file
