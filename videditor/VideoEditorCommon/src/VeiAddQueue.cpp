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
#include <bautils.h>
#include <coemain.h>
#include <mgfetch.h>

// User includes
#include "Veiaddqueue.h"
#include "Veiimageclipgenerator.h"
#include "VeiMGFetchVerifier.h"
#include "VideoEditorUtils.h"
#include "VideoEditorDebugUtils.h"


EXPORT_C CVeiAddQueue* CVeiAddQueue::NewL( MVedAudioClipInfoObserver& aView, CVedMovie& aMovie, MVeiQueueObserver& aObserver )
	{
    CVeiAddQueue* self = CVeiAddQueue::NewLC( aView, aMovie, aObserver );
    CleanupStack::Pop( self );

    return self;
	}


EXPORT_C CVeiAddQueue* CVeiAddQueue::NewLC( MVedAudioClipInfoObserver& aView, CVedMovie& aMovie, MVeiQueueObserver& aObserver )
	{
    CVeiAddQueue* self = new (ELeave) CVeiAddQueue( aView, aMovie, aObserver );
    CleanupStack::PushL( self ); 
    self->ConstructL();

    return self;
	}

CVeiAddQueue::CVeiAddQueue( MVedAudioClipInfoObserver& aView, CVedMovie& aMovie, MVeiQueueObserver& aObserver ) : 
	CActive(CActive::EPriorityStandard), iObserver( &aObserver ), iMovie( aMovie ), iView( aView )
	{
	}

void CVeiAddQueue::ConstructL()
	{
	LOG(KVideoEditorLogFile, "CVeiAddQueue::ConstructL");

	CActiveScheduler::Add(this);
	iInsertVideoDialogOn = EFalse;
    iWaitScheduler = new (ELeave) CActiveSchedulerWait;
	iMovie.RegisterMovieObserverL( this );
	}

EXPORT_C CVeiAddQueue::~CVeiAddQueue()
	{
	iAddQueue.ResetAndDestroy();
	delete iWaitScheduler;

	if ( iAudioClipInfo )
		{
		delete iAudioClipInfo;
		iAudioClipInfo = NULL;
		}

	iMovie.UnregisterMovieObserver( this );

	iObserver = NULL;
	iGenerator = NULL;
	}


EXPORT_C TBool CVeiAddQueue::ShowAudioClipDialogL()
	{
	LOG(KVideoEditorLogFile, "CVeiAddQueue::ShowAudioClipDialogL() in");
	// Audio insert dialog for single file
	CDesCArrayFlat* selectedFiles = new ( ELeave ) CDesCArrayFlat( 1 );
	CleanupStack::PushL(selectedFiles);

	CVeiMGFetchVerifier* mgFetchVerifier = CVeiMGFetchVerifier::NewLC();

	if ( MGFetch::RunL( *selectedFiles, EAudioFile, EFalse, mgFetchVerifier ) == EFalse )
		{
		// User cancelled the dialog.
		CleanupStack::PopAndDestroy( mgFetchVerifier );
		CleanupStack::PopAndDestroy( selectedFiles );
		return EFalse;
		}

	CleanupStack::PopAndDestroy( mgFetchVerifier );

	if ( iAudioClipInfo )
		{
		delete iAudioClipInfo;
		iAudioClipInfo = NULL;
		}
	iObserver->NotifyQueueProcessingStarted( MVeiQueueObserver::EProcessingAudio );
	// AudioClipInfoReady notifier is in EditVideoView
	iAudioClipInfo = CVedAudioClipInfo::NewL( ( *selectedFiles )[0], iView );

	CleanupStack::PopAndDestroy( selectedFiles );
	LOG(KVideoEditorLogFile, "CVeiAddQueue::ShowAudioClipDialogL() out");
	return ETrue;
	}



EXPORT_C TBool CVeiAddQueue::ShowVideoClipDialogL( VideoEditor::TCursorLocation aLocation, TInt aCurrentIndex )
	{
	LOG(KVideoEditorLogFile, "CVeiAddQueue::ShowVideoClipDialogL In");
	// Video insert dialog for single file
	CDesCArrayFlat* selectedFiles = new ( ELeave ) CDesCArrayFlat( 1 );
	CleanupStack::PushL(selectedFiles);

	CVeiMGFetchVerifier* mgFetchVerifier = CVeiMGFetchVerifier::NewLC();

	if ( MGFetch::RunL( *selectedFiles, EVideoFile, EFalse, mgFetchVerifier ) == EFalse )
		{
		CleanupStack::PopAndDestroy( mgFetchVerifier );
		CleanupStack::PopAndDestroy( selectedFiles );
		return EFalse;
		}

	CleanupStack::PopAndDestroy( mgFetchVerifier );

	TInt insertIndex;
	iInsertVideoDialogOn = ETrue;

	// Video clip is added next to selected video clip. If cursor is not on videotrack, clip is 
	// inserted last. 
	if ( iMovie.VideoClipCount() == 0 )
		{
		insertIndex = 0;
		}
	else if ( aLocation == VideoEditor::ECursorOnAudio || aLocation == VideoEditor::ECursorOnTransition )
		{
		insertIndex = iMovie.VideoClipCount();
		}
	else
		{
		insertIndex = aCurrentIndex + 1;
		}

	HBufC* filename = HBufC::NewLC( (*selectedFiles )[0].Length() );
	*filename = (*selectedFiles )[0];
	iAddQueue.Append( filename );
	
	iFailedCount = 0;
	iInsertedCount = 0;

	iTotalCount = iAddQueue.Count();

	iObserver->NotifyQueueProcessingStarted( MVeiQueueObserver::EProcessingVideo );
	AddNextL( insertIndex );	

	CleanupStack::Pop(filename);
	CleanupStack::PopAndDestroy( selectedFiles );
	LOG(KVideoEditorLogFile, "CVeiAddQueue::ShowVideoClipDialogL Out");
	return ETrue;
	}

		
EXPORT_C void CVeiAddQueue::InsertMediaL( const TDesC& aFilename )
	{
	// aFilename is added to queue. Queue processing is started with StartProcessingL() function. 
	RFs&	fs = CCoeEnv::Static()->FsSession();

	if ( BaflUtils::FileExists( fs, aFilename ) )
		{
		HBufC* filename = HBufC::NewLC( aFilename.Length() );
		*filename = aFilename;
		iAddQueue.Append( filename );

		iTotalCount = iAddQueue.Count();

		CleanupStack::Pop( filename ); 
		}
	}

EXPORT_C TInt CVeiAddQueue::Count() const
	{
	return iTotalCount;
	}

EXPORT_C void CVeiAddQueue::StartProcessingL()
	{
	LOG(KVideoEditorLogFile, "CVeiAddQueue::StartProcessingL");

	iFailedCount = 0;
	iInsertedCount = 0;

	iTotalCount = iAddQueue.Count();

	if ( iTotalCount > 0 )
		{
		iObserver->NotifyQueueProcessingStarted();
		
		AddNextL();
		}
	}

TInt CVeiAddQueue::AddNextL( TInt aPosition )
	{
	LOG(KVideoEditorLogFile, "CVeiAddQueue::AddNextL: In");

// Image params
	TTimeIntervalMicroSeconds imageDuration( 5000000 );

	RFs&	fs = CCoeEnv::Static()->FsSession();
	TInt insertErr( KErrNone );
	TInt insertPosition;

	for( TInt i=0;i<iTotalCount;i++ )
		{
		insertErr = KErrNone;

		if ( iAddQueue.Count() > 0 )
			{
			TInt percentage;
			percentage = STATIC_CAST( TInt, ( TReal(iInsertedCount+iFailedCount) / TReal(iTotalCount) )*100 + 0.5 );
			iObserver->NotifyQueueProcessingProgressed( iInsertedCount+iFailedCount+1, percentage );

			TDesC* filename = iAddQueue[0];

			TBool fileExists = BaflUtils::FileExists( fs, *filename );

			TParse file;
			file.Set( *iAddQueue[0], NULL, NULL );

			// Do not allow inserting DRM protected content.
			if( VideoEditorUtils::IsDrmProtectedL(*filename) )
				{
				LOGFMT(KVideoEditorLogFile, "CVeiAddQueue::AddNextL: DRM protected file is rejected: %S", &filename);
				insertErr = KErrAccessDenied;
				}

			if ( file.ExtPresent() && fileExists && KErrNone == insertErr )
				{
				if ( (file.Ext().CompareF( KExt3gp )== 0) ||
						(file.Ext().CompareF( KExtMp4 )== 0 ))
					{
					if ( aPosition == -1 )
						{
						insertPosition = iMovie.VideoClipCount();
						}
					else
						{
						insertPosition = aPosition;
						}
					LOG(KVideoEditorLogFile, "CVeiAddQueue::AddNextL: 2");
					iMovie.InsertVideoClipL( *filename, insertPosition );
					LOG(KVideoEditorLogFile, "CVeiAddQueue::AddNextL: 3");
					}
				else
					{
					LOG(KVideoEditorLogFile, "CVeiAddQueue::AddNextL: 4");
					TRAP( insertErr, 
							iGenerator = CVeiImageClipGenerator::NewL(
							file.FullName(), 
							TSize(KMaxVideoFrameResolutionX,KMaxVideoFrameResolutionY), 
							imageDuration, 
							KRgbBlack, 
							KVideoClipGenetatorDisplayMode, 
							fs, 
							*this) 
						);
					LOG(KVideoEditorLogFile, "CVeiAddQueue::AddNextL: 5");     		 
					}
						
				}
			if ( fileExists && insertErr == KErrNone )
				{
				iWaitScheduler->Start();
				}
			else
				{
				LOG(KVideoEditorLogFile, "CVeiAddQueue::AddNextL: 7");
				if ( iInsertVideoDialogOn )
					{
					insertErr = EInsertingSingleClip;
					}
				else
					{
					insertErr = EInsertingFromGallery;
					}
				TFileName fileName = file.Name();
				TBool cntn = iObserver->NotifyQueueClipFailed( fileName, insertErr );
				if ( !cntn )
					{
					iAddQueue.Reset();
					break;
					}	
				iFailedCount++;
				}

			delete iAddQueue[0];
			iAddQueue.Remove( 0 );
			} // if
		} // for
	LOGFMT2(KVideoEditorLogFile, "CVeiAddQueue::AddNextL: 8, iInsertedCount:%d, iFailedCount:%d", iInsertedCount, iFailedCount);
	iObserver->NotifyQueueEmpty( iInsertedCount, iFailedCount );
	LOG(KVideoEditorLogFile, "CVeiAddQueue::AddNextL: Out");
	return insertErr;
	}

EXPORT_C void CVeiAddQueue::GetNext()
	{
	LOG(KVideoEditorLogFile, "CVeiAddQueue::GetNext: In");
	if (iWaitScheduler->IsStarted() )
		{
		LOG(KVideoEditorLogFile, "CVeiAddQueue::GetNext: 1");
		iWaitScheduler->AsyncStop();
		}
	LOG(KVideoEditorLogFile, "CVeiAddQueue::GetNext: Out");
	}

void CVeiAddQueue::DoCancel()
    {
    }

void CVeiAddQueue::RunL()
    {
    }


void CVeiAddQueue::NotifyImageClipGeneratorInitializationComplete(
	CVeiImageClipGenerator& /*aGenerator*/, TInt DEBUGLOG_ARG(aError) )
	{
	LOGFMT(KVideoEditorLogFile, "CVeiAddQueue::NotifyImageClipGeneratorInitializationComplete: in, aError:%d", aError);
	TRAP_IGNORE( iMovie.InsertVideoClipL(*iGenerator, ETrue, 0) );

	// Generator is no longer our concern
	iGenerator = NULL;
	LOG(KVideoEditorLogFile, "CVeiAddQueue::NotifyImageClipGeneratorInitializationComplete: out");
	}


void CVeiAddQueue::NotifyVideoClipAdded(CVedMovie& /*aMovie*/, TInt /*aIndex*/)
	{
	LOG(KVideoEditorLogFile, "CVeiAddQueue::NotifyVideoClipAdded: In");
	iInsertedCount++;
	LOG(KVideoEditorLogFile, "CVeiAddQueue::NotifyVideoClipAdded: Out");
	}

void CVeiAddQueue::NotifyVideoClipAddingFailed(CVedMovie& /*aMovie*/, TInt DEBUGLOG_ARG(aError) )
	{
	LOGFMT(KVideoEditorLogFile, "CVeiAddQueue::NotifyVideoClipAddingFailed: In, aError:%d", aError);
	TInt error;

	if ( iInsertVideoDialogOn )
		{
		error = EInsertingSingleClip;
		}
	else
		{
		error = EInsertingFromGallery;
		}


	TParse file;
	file.Set( *iAddQueue[0], NULL, NULL );

	TFileName fileName = file.Name();
	TBool ifContinue = iObserver->NotifyQueueClipFailed( fileName, error );
	if ( ifContinue && (error == EInsertingFromGallery) )
		{
		LOG(KVideoEditorLogFile, "CVeiAddQueue::NotifyVideoClipAddingFailed: 1");
		iObserver->NotifyQueueProcessingStarted();
		GetNext();
		}
	else
		{
		// @: release iWaitScheduler
		LOG(KVideoEditorLogFile, "CVeiAddQueue::NotifyVideoClipAddingFailed: 2");
		iWaitScheduler->AsyncStop();
		LOG(KVideoEditorLogFile, "CVeiAddQueue::NotifyVideoClipAddingFailed: 3");
		}
	iFailedCount++;
	LOG(KVideoEditorLogFile, "CVeiAddQueue::NotifyVideoClipAddingFailed: Out");
	}

void CVeiAddQueue::NotifyVideoClipRemoved(CVedMovie& /*aMovie*/, TInt /*aIndex*/)
	{
	LOG(KVideoEditorLogFile, "CVeiAddQueue::NotifyVideoClipRemoved: In and out");
	}

void CVeiAddQueue::NotifyVideoClipIndicesChanged(CVedMovie& /*aMovie*/, TInt /*aOldIndex*/,
									           TInt /*aNewIndex*/)
   	{
   	LOG(KVideoEditorLogFile, "CVeiAddQueue::NotifyVideoClipIndicesChanged: In and out");
	}

void CVeiAddQueue::NotifyVideoClipTimingsChanged(CVedMovie& /*aMovie*/,
										   TInt /*aIndex*/)
	{
	LOG(KVideoEditorLogFile, "CVeiAddQueue::NotifyVideoClipTimingsChanged: In and out");
	}

void CVeiAddQueue::NotifyVideoClipColorEffectChanged(CVedMovie& /*aMovie*/,
												   TInt /*aIndex*/)
	{
	LOG(KVideoEditorLogFile, "CVeiAddQueue::NotifyVideoClipColorEffectChanged: In and out");
	}

void CVeiAddQueue::NotifyVideoClipAudioSettingsChanged(CVedMovie& /*aMovie*/,
											         TInt /*aIndex*/)
	{
	LOG(KVideoEditorLogFile, "CVeiAddQueue::NotifyVideoClipAudioSettingsChanged: In and out");
	}

void CVeiAddQueue::NotifyVideoClipGeneratorSettingsChanged(CVedMovie& /*aMovie*/,
											             TInt /*aIndex*/)
	{
	LOG(KVideoEditorLogFile, "CVeiAddQueue::NotifyVideoClipGeneratorSettingsChanged: In and out");
	}

void CVeiAddQueue::NotifyVideoClipDescriptiveNameChanged(CVedMovie& /*aMovie*/,
																TInt /*aIndex*/)
	{
	LOG(KVideoEditorLogFile, "CVeiAddQueue::NotifyVideoClipDescriptiveNameChanged: In and out");
	}

void CVeiAddQueue::NotifyStartTransitionEffectChanged(CVedMovie& /*aMovie*/)
	{
	LOG(KVideoEditorLogFile, "CVeiAddQueue::NotifyStartTransitionEffectChanged: In and out");
	}

void CVeiAddQueue::NotifyMiddleTransitionEffectChanged(CVedMovie& /*aMovie*/, 
													 TInt /*aIndex*/)
	{
	LOG(KVideoEditorLogFile, "CVeiAddQueue::NotifyMiddleTransitionEffectChanged: In and out");
	}

void CVeiAddQueue::NotifyEndTransitionEffectChanged(CVedMovie& /*aMovie*/)
	{
	LOG(KVideoEditorLogFile, "CVeiAddQueue::NotifyEndTransitionEffectChanged: In and out");
	}

void CVeiAddQueue::NotifyAudioClipAdded(CVedMovie& /*aMovie*/, TInt /*aIndex*/)
	{
	LOG(KVideoEditorLogFile, "CVeiAddQueue::NotifyAudioClipAdded: In and out");
	}

void CVeiAddQueue::NotifyAudioClipAddingFailed(CVedMovie& /*aMovie*/, TInt DEBUGLOG_ARG(aError) )
	{
	LOGFMT(KVideoEditorLogFile, "CVeiAddQueue::NotifyAudioClipAddingFailed: In and out, aError:%d", aError);
	}

void CVeiAddQueue::NotifyAudioClipRemoved(CVedMovie& /*aMovie*/, TInt /*aIndex*/)
	{
	LOG(KVideoEditorLogFile, "CVeiAddQueue::NotifyAudioClipRemoved: In and out");
	}

void CVeiAddQueue::NotifyAudioClipIndicesChanged(CVedMovie& /*aMovie*/, TInt /*aOldIndex*/, 
									           TInt /*aNewIndex*/)
	{
	LOG(KVideoEditorLogFile, "CVeiAddQueue::NotifyAudioClipIndicesChanged: In and out");
	}

void CVeiAddQueue::NotifyAudioClipTimingsChanged(CVedMovie& /*aMovie*/,
											   TInt /*aIndex*/)
	{
	LOG(KVideoEditorLogFile, "CVeiAddQueue::NotifyAudioClipTimingsChanged: In and out");
	}

void CVeiAddQueue::NotifyMovieQualityChanged(CVedMovie& /*aMovie*/)
	{
	LOG(KVideoEditorLogFile, "CVeiAddQueue::NotifyMovieQualityChanged: In and out");
	}

void CVeiAddQueue::NotifyMovieReseted(CVedMovie& /*aMovie*/)
	{
	LOG(KVideoEditorLogFile, "CVeiAddQueue::NotifyMovieReseted: In and out");
	}

void CVeiAddQueue::NotifyMovieOutputParametersChanged(CVedMovie& /*aMovie*/)
	{
	LOG(KVideoEditorLogFile, "CVeiAddQueue::NotifyMovieOutputParametersChanged: In and out");
	}

void CVeiAddQueue::NotifyAudioClipDynamicLevelMarkInserted(CVedMovie& /*aMovie*/, 
                                                         TInt /*aClipIndex*/, 
                                                         TInt /*aMarkIndex*/)
	{
	LOG(KVideoEditorLogFile, "CVeiAddQueue::NotifyAudioClipDynamicLevelMarkInserted: In and out");
	}

void CVeiAddQueue::NotifyAudioClipDynamicLevelMarkRemoved(CVedMovie& /*aMovie*/, 
                                                        TInt /*aClipIndex*/, 
                                                        TInt /*aMarkIndex*/)
	{
	LOG(KVideoEditorLogFile, "CVeiAddQueue::NotifyAudioClipDynamicLevelMarkRemoved: In and out");
	}

void CVeiAddQueue::NotifyVideoClipDynamicLevelMarkInserted(CVedMovie& /*aMovie*/,
                                                         TInt /*aClipIndex*/, 
                                                         TInt /*aMarkIndex*/)
	{
	LOG(KVideoEditorLogFile, "CVeiAddQueue::NotifyVideoClipDynamicLevelMarkInserted: In and out");
	}

void CVeiAddQueue::NotifyVideoClipDynamicLevelMarkRemoved(CVedMovie& /*aMovie*/, 
                                                        TInt /*aClipIndex*/, 
                                                        TInt /*aMarkIndex*/)
	{
	LOG(KVideoEditorLogFile, "CVeiAddQueue::NotifyVideoClipDynamicLevelMarkRemoved: In and out");
	}

// End of File
