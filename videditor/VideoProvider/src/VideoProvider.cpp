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


#include <eikmenup.h>
#include "VideoProvider.h"
#include <AiwMenu.h>
#include <AiwCommon.h>
#include <AiwGenericParam.h>
#include <eikenv.h>
#include <VideoProviderInternal.rsg>
#include <ImplementationProxy.h>
#include "VideoProviderUids.hrh"
#include "VideoProvider.rh"
#include <aknutils.h>
#include <bautils.h>
#include <AknOpenFileService.h>
#include <data_caging_path_literals.hrh>
#include <apgcli.h>
#include <apmrec.h>
#include <caf.h> // for DRM checks
#include <e32property.h>
#include <f32file.h>
#include "VideoEditorCommon.h"
#include "VideoEditorUtils.h"
#include "VideoEditorDebugUtils.h"
#if defined(INCLUDE_SIMPLE_VIDEO_EDITOR)
#include "SimpleVideoEditor.h"
#endif

using namespace ContentAccess;


//=============================================================================
CVideoProvider* CVideoProvider::NewL()
	{
    LOG(KVideoProviderLogFile, "CVideoProvider::NewL");

	return new (ELeave)	CVideoProvider();
	}
	
//=============================================================================
CVideoProvider::CVideoProvider() : iResLoader(*CEikonEnv::Static())
	{
	LOG(KVideoProviderLogFile, "CVideoProvider::CVideoProvider: In");

    _LIT(KResourceFile, "VideoProviderInternal.rsc");
    TFileName fileName;
    TParse p;    

    Dll::FileName(fileName);
    p.Set(KResourceFile, &KDC_RESOURCE_FILES_DIR, &fileName);
    iResourceFile = p.FullName();

	iResFileIsLoaded = EFalse;

	LOG(KVideoProviderLogFile, "CVideoProvider::CVideoProvider: Out");
	}

//=============================================================================
CVideoProvider::~CVideoProvider()
	{
	LOG(KVideoProviderLogFile, "CVideoProvider::~CVideoProvider(): In");

	#if defined(INCLUDE_SIMPLE_VIDEO_EDITOR)
	if (iSimpleVideoEditor)
		{
		iSimpleVideoEditor->Cancel();
		}
	delete iSimpleVideoEditor;
	iSimpleVideoEditor = NULL;
	#endif

LOG(KVideoProviderLogFile, "CVideoProvider::~CVideoProvider(): 1");
	CloseFsSession();
LOG(KVideoProviderLogFile, "CVideoProvider::~CVideoProvider(): 2");	

    iAiwNotifyCallback = NULL;
    iInParamList->Reset();
    delete iInParamList;
    iOutParamList->Reset();
    delete iOutParamList;
LOG(KVideoProviderLogFile, "CVideoProvider::~CVideoProvider(): 3");
	delete iOpenFileService;
	iOpenFileService = NULL;
	iResLoader.Close();
LOG(KVideoProviderLogFile, "CVideoProvider::~CVideoProvider(): 4");
	iSourceMGAlbumIdList.Close();

	LOG(KVideoProviderLogFile, "CVideoProvider::~CVideoProvider(): Out");
	}

//=============================================================================
void CVideoProvider::InitialiseL(MAiwNotifyCallback& /*aFrameworkCallback*/,
								      const RCriteriaArray& /*aInterest*/)
	{
    LOG(KVideoProviderLogFile, "CVideoProvider::InitialiseL: in");

    if (!iInParamList)
    	{
        iInParamList = CAiwGenericParamList::NewL();    
    	}

    if (!iOutParamList)
    	{
        iOutParamList = CAiwGenericParamList::NewL();    
    	}

	if ( !iResFileIsLoaded )
		{
		BaflUtils::NearestLanguageFile( CEikonEnv::Static()->FsSession(), iResourceFile );
		LOGFMT(KVideoProviderLogFile, "CVideoProvider::InitialiseL: Loading resource file: %S", &iResourceFile);
		iResLoader.OpenL( iResourceFile );
		}

	iResFileIsLoaded = ETrue;

    // Publish & Subscribe API used for delivering document name from application to AIW provider
    // NOTE: this assumes only a single instance of video editor(s) at a time.
    TInt err = RProperty::Define(KUidVideoEditorProperties, VideoEditor::EPropertyFilename, RProperty::EText);
    if (err != KErrAlreadyExists)
        {
    	LOGFMT(KVideoProviderLogFile, "CVideoProvider::InitialiseL: Calling RProperty::Define(KUidVideoEditorProperties, VideoEditor::EPropertyFilename, RProperty::EText): error %d", err);
        User::LeaveIfError(err);
        }

    LOG(KVideoProviderLogFile, "CVideoProvider::InitialiseL: out");
	}

//=============================================================================
void CVideoProvider::InitializeMenuPaneL(CAiwMenuPane& aMenuPane,
											  TInt aIndex,
											  TInt /*aCascadeId*/,
											  const CAiwGenericParamList& aInParamList)
	{
	LOGFMT2(KVideoProviderLogFile, "CVideoProvider::InitializeMenuPaneL: In (aIndex: %d, aInParamList.Count(): %d)", aIndex, aInParamList.Count());

#ifdef LOG_TIMING
	TTime startTime(0);
	startTime.UniversalTime();
	TTime inputAnalyzedTime(0);
	TTime endTime(0);
#endif // LOG_TIMING

	RFs& fs = CEikonEnv::Static()->FsSession();

	// First check what kin of files the parameter list contains
	// and what we can do with them
	TInt numberOfEditableVideoClips = 0;
	TInt numberOfEditableAudioClips = 0;
	TInt numberOfEditableImages = 0;

	// We trust that aInParamList is always the same, i.e.
	// - a few (seems to be 3) parameters in the begining
	// - after that, filename/mimetype pairs
	TInt paramCount = aInParamList.Count();
	TInt fileCount = aInParamList.Count( EGenericParamFile );

	if ( fileCount <= KAiwMaxNumberOfFilesSimultaneouslyHandled )
		{
		for ( TInt i=0; i < paramCount ; i++ )
			{
			TBool isDRMProtected( EFalse );

			// Extract file names from the parameter list.
			const TAiwGenericParam& param = aInParamList[i];
			if (param.SemanticId() == EGenericParamFile)
				{
				TPtrC fileName = param.Value().AsDes();
				LOGFMT(KVideoProviderLogFile, "\tfile name: %S", &fileName);

				// Next we need to get the MIME typ of the file.
				TBuf<KMaxDataTypeLength> mimeType;
				RFile file;
				TInt err( file.Open( fs, fileName, EFileShareReadersOnly ) );
				if( KErrNone != err ) 
					{
					err = file.Open( fs, fileName, EFileShareAny );
					}
					
				if( KErrNone == err )
					{
					TDataRecognitionResult dataType;
					CleanupClosePushL( file );
			        // Check if the file is valid
			        RApaLsSession lsSession;
			        err = lsSession.Connect();
			        CleanupClosePushL( lsSession );
				    err = lsSession.RecognizeData( file, dataType );
				    if ( KErrNone == err )
				        {
                        const TInt confidence( dataType.iConfidence );
                        if( CApaDataRecognizerType::ECertain == confidence ||
                            CApaDataRecognizerType::EProbable == confidence ||
                            CApaDataRecognizerType::EPossible == confidence )
                            {
							mimeType = dataType.iDataType.Des();
                            }
				        }		
				    CleanupStack::PopAndDestroy( 2 ); // file, lsSession
					}

				// Based on the MIME type, decice whether we support this file.
				if (mimeType.Length())
						{
						// Create CContent-object
                        CContent* pContent = CContent::NewLC(fileName); 
						// See if the content object is protected
                        User::LeaveIfError( pContent->GetAttribute( EIsProtected, isDRMProtected ) );
                        
                        CleanupStack::PopAndDestroy (pContent);
						if (!isDRMProtected)
							{
							if ( IsSupportedVideoFile(mimeType) )
								{
								numberOfEditableVideoClips++;
								}
							else if ( IsSupportedAudioFile(mimeType) )
								{
								numberOfEditableAudioClips++;
								}
							else if ( IsSupportedImageFile(mimeType) )
								{
								numberOfEditableImages++;
								}
							}
						else
							{
							LOGFMT(KVideoProviderLogFile, "\tCannot edit DRM protected file: %S", &fileName);
							}
						}		
				}
			}
		}
	else
		{
		LOGFMT(KVideoProviderLogFile, "CVideoProvider::InitialiseL: too many files to handle (%d). Ignored.", fileCount);
		}

	// When the content of the parameter list is analyzed, 
	// add the appropriate menu items
	LOGFMT3(KVideoProviderLogFile, "CVideoProvider::InitializeMenuPaneL: AIW parameter list content analyzed: numberOfEditableVideoClips: %d, numberOfEditableAudioClips: %d, numberOfEditableImages: %d", numberOfEditableVideoClips,numberOfEditableAudioClips,numberOfEditableImages);

#ifdef LOG_TIMING
	inputAnalyzedTime.UniversalTime();
#endif // LOG_TIMING

	// CASE 1: one video selected: all options available
	if (numberOfEditableVideoClips == 1 && (numberOfEditableAudioClips+numberOfEditableImages) == 0)
		{

#if defined(INCLUDE_MANUAL_VIDEO_EDITOR)
		
		// Advanced (manual) editor
		aMenuPane.AddMenuItemsL(
			iResourceFile, 
			R_VIDEOEDITORPROVIDER_ADVANCED_MENU,
			KAiwCmdEdit,
			aIndex );

#endif // INCLUDE_MANUAL_VIDEO_EDITOR

#if defined(INCLUDE_SIMPLE_VIDEO_EDITOR)

		// Simple Cut
		aMenuPane.AddMenuItemsL(
			iResourceFile, 
			R_VIDEOEDITORPROVIDER_CUT_MENU,
			KAiwCmdEdit,
			aIndex );

		// Simple Add text
		aMenuPane.AddMenuItemsL(
			iResourceFile, 
			R_VIDEOEDITORPROVIDER_ADD_TEXT_MENU,
			KAiwCmdEdit,
			aIndex );

		// Simple Add audio
		aMenuPane.AddMenuItemsL(
			iResourceFile, 
			R_VIDEOEDITORPROVIDER_ADD_AUDIO_MENU,
			KAiwCmdEdit,
			aIndex );

		// Simple merge
		aMenuPane.AddMenuItemsL(
			iResourceFile, 
			R_VIDEOEDITORPROVIDER_MERGE_MENU,
			KAiwCmdEdit,
			aIndex );

		// Sub-menu title "Edit"
		aMenuPane.AddMenuItemsL(
			iResourceFile, 
			R_VIDEOEDITORPROVIDER_SUBMENU_TITLE,
			KAiwCmdEdit,
			aIndex );

#endif // INCLUDE_SIMPLE_VIDEO_EDITOR

		}

#if defined(INCLUDE_MANUAL_VIDEO_EDITOR)

	// CASE 2: several video clips or other files selected: only manual editor available,
	// and only if there is at least one video.
	if (numberOfEditableVideoClips > 0 
		&& (numberOfEditableVideoClips+numberOfEditableAudioClips+numberOfEditableImages) > 1)
		{
		// Advanced (manual) editor only
		aMenuPane.AddMenuItemsL(
			iResourceFile, 
			R_VIDEOEDITORPROVIDER_EDIT_MENU,
			KAiwCmdEdit, 
			aIndex );
		}

#endif // INCLUDE_MANUAL_VIDEO_EDITOR

#ifdef LOG_TIMING
	endTime.UniversalTime();

	TInt64 totalTime = endTime.Int64() - startTime.Int64();
	TInt64 inputlistAnalyzingtime = inputAnalyzedTime.Int64() - startTime.Int64();
	TInt64 menuPaneAddingTime = endTime.Int64() - inputAnalyzedTime.Int64();

	_LIT(KText1, "AIW Parameter list contains %d files (total %d parameters). Times taken:");
	_LIT(KText2, "    Total: %Ld, Analyzing input param list: %Ld, Adding menu items: %Ld");
	TFileName path(KLogsFolder);
	path.Append(KLogDir);
	path.Append(KBackslash);
	TFileName fileNameAndPath(path);
	fileNameAndPath.Append(_L("VideoProviderTiming.log"));
	if(BaflUtils::FolderExists(fs,path))
		{
		TLogFileDes16OverflowHandler ofh;
		TBuf<KMaxLogLineLength> buf1;
		TBuf<KMaxLogLineLength> buf2;
		buf1.AppendFormat(KText1,&ofh,fileCount,paramCount);
		buf2.AppendFormat(KText2,&ofh,totalTime,inputlistAnalyzingtime,menuPaneAddingTime);
		CLogFile::StaticLog(fileNameAndPath,buf1);
		CLogFile::StaticLog(fileNameAndPath,buf2);
		}
	RDebug::Print(KText1, fileCount, paramCount);
	RDebug::Print(KText2, totalTime, inputlistAnalyzingtime, menuPaneAddingTime);
#endif // LOG_TIMING

	LOG(KVideoProviderLogFile, "CVideoProvider::InitializeMenuPaneL: out");
	}

//=============================================================================
void CVideoProvider::HandleServiceCmdL(const TInt& aCmdId,
								    	    const CAiwGenericParamList& aInParamList,
											CAiwGenericParamList& aOutParamList,
											TUint aCmdOptions,
											const MAiwNotifyCallback* aCallback)
	{
	LOGFMT(KVideoProviderLogFile, "CVideoProvider::HandleServiceCmdL (%d)", aCmdId);
	HandleCmdsL(aCmdId, aInParamList, aOutParamList, aCmdOptions, aCallback);
	}

//=============================================================================
void CVideoProvider::HandleMenuCmdL (
    TInt                            aMenuCmdId,
    const CAiwGenericParamList &	aInParamList,
    CAiwGenericParamList &          aOutParamList,
    TUint                           aCmdOptions,
    const MAiwNotifyCallback *      aCallback)

	{
	LOGFMT(KVideoProviderLogFile, "CVideoProvider::HandleMenuCmdL (%d)", aMenuCmdId);
	HandleCmdsL(aMenuCmdId, aInParamList, aOutParamList, aCmdOptions, aCallback);
	}

//=============================================================================
void CVideoProvider::HandleCmdsL(TInt aMenuCmdId, 
										 const CAiwGenericParamList& aInParamList,
										 CAiwGenericParamList& aOutParamList,
										 TUint /*aCmdOptions*/,
										 const MAiwNotifyCallback* aCallback)
	{
	LOGFMT(KVideoProviderLogFile, "CVideoProvider::HandleCmdsL (%d): In", aMenuCmdId);

	switch ( aMenuCmdId )
		{
		case EVideoEditorProviderCmdMerge:
		case EVideoEditorProviderCmdAddAudio:
		case EVideoEditorProviderCmdAddText:
		case EVideoEditorProviderCmdCut:
		case EVideoEditorProviderCmdAdvanced:
			{
			// Store input parameters
			if (aCallback)
				{
				iAiwNotifyCallback = aCallback;
				iInParamList->Reset();
				iInParamList->AppendL(aInParamList);
				iOutParamList->Reset();
				iOutParamList->AppendL(aOutParamList);
				LOG(KVideoProviderLogFile, "CVideoProvider: Using AIW call back");
				}
			else
				{
				iAiwNotifyCallback = NULL;
				}

			// Open file server session
			User::LeaveIfError(iSharableFS.Connect());
			iSharableFS.ShareProtected();

			// Find the first file on the generic param list...
			// There must be at least one file, and all the files must exist.
			TPtrC fileName;
			TInt count = aInParamList.Count();
			iSourceMGAlbumIdList.Reset();
			for (TInt i = count - 1; i >= 0; --i)
				{
				const TAiwGenericParam& param = aInParamList[i];
				if (param.SemanticId() == EGenericParamFile)
					{
					fileName.Set(param.Value().AsDes());

					// Check that that the file exists and is accessible.
					// The AIW consumer should provide us only valid files.
					// If this is not the case, just leave and let the consumer handle the error.
					TEntry entry;
					TInt err = iSharableFS.Entry( fileName, entry );
					LOGFMT2(KVideoProviderLogFile, "CVideoProvider::HandleCmdsL: Could not open file: %S, error: %d", &fileName, err);
					User::LeaveIfError( err );

					// Store the time stamp of the most recent file (needed later).
					TTime time = entry.iModified;
					if (iOriginalTimeStamp < time)
						{
						iOriginalTimeStamp = time;
						}

					// Find out whether the source file belongs to any albums
					VideoEditorUtils::GetMGAlbumsListForMediaFileL (
						iSourceMGAlbumIdList,
						fileName );
					}
				}

			// Launch the editor    
			LaunchEditorL( aMenuCmdId, fileName, aInParamList);

			break; 
			}
		default:
			{
			break;
			}
		}
	LOG(KVideoProviderLogFile, "CVideoProvider::HandleCmdsL: out");
	}

//=============================================================================
void CVideoProvider::LaunchEditorL( 
	TInt aMenuCmdId, 
	const TDesC & 					aFileName,
    const CAiwGenericParamList &	aInParamList
	)
	{
    LOGFMT(KVideoProviderLogFile, "CVideoProvider::LaunchEditorL: file: %S", &aFileName);

	RFile fileHandle;
	TInt err = fileHandle.Open(iSharableFS,aFileName,EFileWrite|EFileShareReadersOrWriters);
	if (KErrNone != err)
		{
		LOG(KVideoEditorLogFile, "CVideoProvider::LaunchEditorL: Could not open file with EFileWrite. Trying EFileRead");
		User::LeaveIfError( fileHandle.Open (iSharableFS,aFileName,EFileRead|EFileShareReadersOrWriters) );
		}
	CleanupClosePushL (fileHandle);

	if (iAiwNotifyCallback)
		{
	    const_cast<MAiwNotifyCallback*>(iAiwNotifyCallback)->HandleNotifyL(KAiwCmdEdit, KAiwEventStarted, *iOutParamList, *iInParamList);    
		}

    switch ( aMenuCmdId )
		{
		case EVideoEditorProviderCmdMerge:
			{
			LOG(KVideoProviderLogFile, "\tEVideoEditorProviderCmdMerge");

			#if defined(INCLUDE_SIMPLE_VIDEO_EDITOR)
			delete iSimpleVideoEditor;
			iSimpleVideoEditor = NULL;
			iSimpleVideoEditor = CSimpleVideoEditor::NewL( *this );
			iSimpleVideoEditor->Merge (aFileName);
			#endif // INCLUDE_SIMPLE_VIDEO_EDITOR

			break;
			}
		case EVideoEditorProviderCmdAddAudio:
			{
			LOG(KVideoProviderLogFile, "\tEVideoEditorProviderCmdAddAudio");

			#if defined(INCLUDE_SIMPLE_VIDEO_EDITOR)
			delete iSimpleVideoEditor;
			iSimpleVideoEditor = NULL;
			iSimpleVideoEditor = CSimpleVideoEditor::NewL( *this );
			iSimpleVideoEditor->ChangeAudio (aFileName);
			#endif // INCLUDE_SIMPLE_VIDEO_EDITOR

			break;
			}
		case EVideoEditorProviderCmdAddText:
			{
			LOG(KVideoProviderLogFile, "\tEVideoEditorProviderCmdAddText");

			#if defined(INCLUDE_SIMPLE_VIDEO_EDITOR)
			delete iSimpleVideoEditor;
			iSimpleVideoEditor = NULL;
			iSimpleVideoEditor = CSimpleVideoEditor::NewL( *this );
			iSimpleVideoEditor->AddText (aFileName);
			#endif // INCLUDE_SIMPLE_VIDEO_EDITOR

			break;
			}
		case EVideoEditorProviderCmdCut:
			{
			LOG(KVideoProviderLogFile, "\tEVideoEditorProviderCmdCut");
			iOpenFileService = CAknOpenFileService::NewL (KUidSimpleCutVideo,fileHandle,(MAknServerAppExitObserver *)this,&const_cast<CAiwGenericParamList &>(aInParamList));
			break;
			}
		case EVideoEditorProviderCmdAdvanced:
			{
			LOG(KVideoProviderLogFile, "\tEVideoEditorProviderCmdAdvanced");
			iOpenFileService = CAknOpenFileService::NewL (KUidVideoEditor,fileHandle,(MAknServerAppExitObserver *)this,&const_cast<CAiwGenericParamList &>(aInParamList));
			break;
			}
		default:
			LOG(KVideoProviderLogFile, "\tUnknown command!");
			break;
		}

	CleanupStack::PopAndDestroy( &fileHandle ); // close fileHandle

	LOG(KVideoProviderLogFile, "CVideoProvider::LaunchEditorL: out");
	}

//=============================================================================
TBool CVideoProvider::IsSupportedVideoFile (const TDesC& aDataType) const
	{
	_LIT(KMime3gp, "video/3gpp");
	_LIT(KMimeMp4, "video/mp4");

	return aDataType.CompareF( KMime3gp ) == 0 || aDataType.CompareF( KMimeMp4 ) == 0;
	}

//=============================================================================
TBool CVideoProvider::IsSupportedAudioFile (const TDesC& aDataType) const
	{
	_LIT(KMimeAllAudio, "audio/");
	return aDataType.Left(6).CompareF( KMimeAllAudio ) == 0;
	}

//=============================================================================
TBool CVideoProvider::IsSupportedImageFile (const TDesC& aDataType) const
	{
	_LIT(KMimeAllImages, "image/");
	return aDataType.Left(6).CompareF( KMimeAllImages ) == 0;
	}

//=============================================================================
void CVideoProvider::HandleServerAppExit (TInt aReason)
	{
    LOGFMT(KVideoProviderLogFile, "CVideoProvider::HandleServerAppExit: In: %d", aReason);

	delete iOpenFileService;
	iOpenFileService = NULL;

	// Get the output file name provided by the editor application
    TFileName newFileName;
    (void) RProperty::Get(KUidVideoEditorProperties, VideoEditor::EPropertyFilename, newFileName);

	// Report new file to AIW consumer
	TRAP_IGNORE( FinalizeL (newFileName) );
    MAknServerAppExitObserver::HandleServerAppExit(aReason);

    LOG(KVideoProviderLogFile, "CVideoProvider::HandleServerAppExit: Out");
    }

//=============================================================================
void CVideoProvider::HandleSimpleVideoEditorExit (TInt DEBUGLOG_ARG(aReason), const TDesC& aResultFileName)
	{
    LOGFMT2(KVideoProviderLogFile, "CVideoProvider::HandleSimpleVideoEditorExitL: In: %d, %S", aReason, &aResultFileName);

#if defined(INCLUDE_SIMPLE_VIDEO_EDITOR)

	// Report new file to AIW consumer
	TRAP_IGNORE( FinalizeL (aResultFileName) );

	delete iSimpleVideoEditor;
	iSimpleVideoEditor = NULL;


#endif // INCLUDE_SIMPLE_VIDEO_EDITOR

    LOG(KVideoProviderLogFile, "CVideoProvider::HandleSimpleVideoEditorExitL: Out");
    }

//=============================================================================
void CVideoProvider::FinalizeL (const TDesC& aFileName)
	{
	LOGFMT(KVideoProviderLogFile, "CVideoProvider::FinalizeL: In: %S", &aFileName);

#ifdef FILE_TIME_STAMP_UPDATE    
	LOG(KVideoProviderLogFile, "CVideoProvider::FinalizeL: 2, executing FILE_TIME_STAMP_UPDATE");
	// Set the timestamp of the saved file to original file's timestamp + 1 second.
	// The idea is to make the original and edited images appear next to each other.
	if( aFileName.Length() && BaflUtils::FileExists(iSharableFS,aFileName) )
		{
		// The requirement is to increment the time by 1 second.
		// For some weird reason, setting Attribs to 
		// iOriginalTimeStamp + TTimeIntervalSeconds (1) has no effect, 
		// but 2 seconds works fine... 
		TTime newTime = iOriginalTimeStamp + TTimeIntervalSeconds (2);

		CFileMan* fileMan = CFileMan::NewL( iSharableFS );
		CleanupStack::PushL (fileMan);
		// do not set or clear any attribute, mofify time attribute
		fileMan->Attribs(aFileName, 0, 0, newTime);
		CleanupStack::PopAndDestroy (fileMan);
		}
#endif

	iSourceMGAlbumIdList.Reset();

	// Notify the AIW consumer
	if (iAiwNotifyCallback)
		{
		// Insert the file name to the output parameter list
		// (we assume that it is always the first item on the list)
		iOutParamList->Reset();
		TAiwVariant variant(aFileName);
		TAiwGenericParam param(EGenericParamFile, variant);
		iOutParamList->AppendL(param);

	    LOG(KVideoProviderLogFile, "CVideoProvider: Calling HandleNotifyL");

		// Non-leaving function shall use TRAP
		TRAP_IGNORE ( 
			const_cast<MAiwNotifyCallback*>(iAiwNotifyCallback)->HandleNotifyL(KAiwCmdEdit, KAiwEventCompleted, *iOutParamList, *iInParamList);    	    	    
			const_cast<MAiwNotifyCallback*>(iAiwNotifyCallback)->HandleNotifyL(KAiwCmdEdit, KAiwEventStopped, *iOutParamList, *iInParamList);    
			);

		// Reset new filename property and out paramlist
        User::LeaveIfError(RProperty::Set(KUidVideoEditorProperties, VideoEditor::EPropertyFilename, KNullDesC));
        iOutParamList->Reset();
		}

	CloseFsSession();

	LOG(KVideoProviderLogFile, "CVideoProvider::FinalizeL: Out");
	}

//=============================================================================
void CVideoProvider::CloseFsSession()
	{
	LOG(KVideoProviderLogFile, "CVideoProvider::CloseFsSession(): In");

	if (iSharableFS.Handle() != 0)
        {
        LOG(KVideoProviderLogFile, "\tClosing iSharableFS");
        iSharableFS.Close();
        }

    LOG(KVideoProviderLogFile, "CVideoProvider::CloseFsSession(): Out");
	}


//
// Rest of the file is for ECom initialization. 
//

// Map the interface UIDs to implementation factory functions
LOCAL_D const TImplementationProxy ImplementationTable[] =
	{
	IMPLEMENTATION_PROXY_ENTRY( KVideoEditorProviderImplUid, CVideoProvider::NewL)
	};

// ---------------------------------------------------------
//
// Exported proxy for instantiation method resolution
// ---------------------------------------------------------
//
EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
	{
	aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);
	return ImplementationTable;
	}


// End of file
