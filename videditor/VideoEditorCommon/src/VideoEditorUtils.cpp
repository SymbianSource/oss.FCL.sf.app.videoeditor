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
#include <bautils.h>
#include <eikenv.h>
#include <badesca.h>
#include <PathInfo.h>
#include <sysutil.h>
#include <DRMCommon.h>
#include <AknUtils.h>
#include <data_caging_path_literals.hrh>
#include <AknListQueryDialog.h> 
#include <VedSimpleCutVideo.rsg>
#include <BAUTILS.H> 

#include "VideoEditorUtils.h"
#include "VideoEditorCommon.h"
#include "VeiSettings.h"
#include "VideoEditorDebugUtils.h"


// CONSTANTS
_LIT (KEditedSuffix, "-");
_LIT(KManualVideoEditorMifFile,        "ManualVideoEditor.mif");
_LIT(KVideoEditorUiComponentsMifFile,  "VideoEditorUiComponents.mif");
_LIT(KVideoEditorMbmFile,              "VideoEditorBitmaps.mbm");

//=============================================================================
EXPORT_C void VideoEditorUtils::NotifyNewMediaDocumentL (
    RFs& /*aFsSession*/, 
    const TDesC& aFileName )
{
	LOGFMT(KVideoEditorLogFile, "VideoEditorUtils::NotifyNewMediaDocumentL: In (%S)", &aFileName);
    LOG(KVideoEditorLogFile, "VideoEditorUtils::NotifyNewMediaDocumentL: Out");
}

//=============================================================================
EXPORT_C TInt VideoEditorUtils::GenerateNewDocumentNameL (
    RFs& aFsSession, 
    const TDesC& aSourceFileName, 
    TDes& aTargetFileName,
    TVedVideoFormat aOutputFormat,
    TInt aTargetSizeEstimate,
    VideoEditor::TMemory aMemoryInUse )
{
	LOG(KVideoEditorLogFile, "VideoEditorUtils::GenerateNewDocumentNameL, in:");
	LOGFMT4(KVideoEditorLogFile, "VideoEditorUtils::GenerateNewDocumentNameL 1\tSource file: %S, output format: %d, required space: %d, memory in use: %d", &aSourceFileName, aOutputFormat, aTargetSizeEstimate, aMemoryInUse);

	TInt err = KErrNone;

	//	Set file name to parser
	TParsePtrC fileParse (aSourceFileName);

	//  Test filename is already too long
	if (fileParse.NameAndExt().Length() > KMaxFileName - 5)
	{
		err = KErrArgument;
	}

	// Otherwise proceed to generate the filename
	else
	{
		//	If the memory is specified as EMemAutomatic, the target is primarily
		//	on the memory card, and if that is full, on the phone memory.
		//	If EMemPhoneMemory or EMemMemoryCard is specified, only that one is used.
		VideoEditor::TMemory selectedMemoryInUse;
		aMemoryInUse == VideoEditor::EMemAutomatic ? 
			selectedMemoryInUse = VideoEditor::EMemMemoryCard : 
			selectedMemoryInUse = aMemoryInUse;

		//  Find file suffix that is not yet used 
		TInt val = 1;
		TFileName temp;

		//  First try the above selected primary location.
		TFileName driveAndPath;
		if (selectedMemoryInUse == VideoEditor::EMemPhoneMemory)
		{
			driveAndPath.Copy( PathInfo::PhoneMemoryRootPath() );
		}
		else
		{
			driveAndPath.Copy( PathInfo::MemoryCardRootPath() );
		}
		driveAndPath.Append( PathInfo::VideosPath() );
		TRAPD(errBafl, BaflUtils::EnsurePathExistsL (aFsSession, driveAndPath));
		LOGFMT(KVideoEditorLogFile, "VideoEditorUtils::GenerateNewDocumentNameL 2, errBafl:%d", errBafl );
		TBool primaryLocationFull = ( KErrNone != errBafl 
			|| !IsEnoughFreeSpaceToSaveL ( aFsSession, driveAndPath, aTargetSizeEstimate ) );


		//	If the memory is full, and the memory is selected as automatic,
		//	try alternative location.
		if (primaryLocationFull && aMemoryInUse == VideoEditor::EMemAutomatic)
		{
			if (selectedMemoryInUse == VideoEditor::EMemMemoryCard)
			{
				driveAndPath.Copy (PathInfo::PhoneMemoryRootPath() );
			}
			else
			{
				driveAndPath.Copy (PathInfo::MemoryCardRootPath() );
			}
			driveAndPath.Append ( PathInfo::VideosPath() );
			TBool secondaryLocationFull = ( !BaflUtils::FolderExists (aFsSession, driveAndPath)
            || !IsEnoughFreeSpaceToSaveL ( aFsSession, driveAndPath, aTargetSizeEstimate ) );
			if (secondaryLocationFull)
			{
				err = KErrDiskFull;
			}
		}

        //	Now sufficient disk space has been verified.
        //	Proceed to generate the unique file name.
        if (KErrNone == err)
        {
            LOGFMT(KVideoEditorLogFile, "VideoEditorUtils::GenerateNewDocumentNameL 3, File will be saved to path: %S", &driveAndPath );

            //  Copy drive and path to temporary file name
            temp.Copy( driveAndPath );

            //  Add file name without suffix 
            TPtrC name = fileParse.Name();
            TInt offset = FindSuffix ( name );
            if (offset == KErrNotFound)
	        {
                temp.Append ( fileParse.Name() );
            }
            else
            {
                temp.Append ( name.Left (offset) );
            }
    
            temp.Append ( KEditedSuffix );
            temp.AppendNumFixedWidth (val, EDecimal, 3);
            temp.Append ( aOutputFormat == EVedVideoFormat3GPP ? KExt3gp : KExtMp4 );

            //  Increase edit number until we find a file name that is not used
            while ( FileAlreadyExistsL(aFsSession, temp) )
            {
                ++val;
                temp.Zero();
                temp.Copy ( driveAndPath );
                if (offset == KErrNotFound)
                {
                    temp.Append ( fileParse.Name() );
                }
		        else
                {
                    temp.Append ( name.Left (offset) );
                }

                temp.Append ( KEditedSuffix );
                if (val < 1000)
                {
                    temp.AppendNumFixedWidth ( val, EDecimal, 3);
                }
                else
                {
                    temp.AppendNumFixedWidth ( val, EDecimal, 4);
                }

                temp.Append ( aOutputFormat == EVedVideoFormat3GPP ? KExt3gp : KExtMp4 );
            }

            //  Set document name 
            aTargetFileName.Copy ( temp );
        }
    }

    LOGFMT(KVideoEditorLogFile, "VideoEditorUtils::GenerateNewDocumentNameL: out (%d)", err);    
    return err;
}


//=============================================================================
EXPORT_C TInt VideoEditorUtils::GenerateFileNameL (
    RFs& aFsSession, 
    const TDesC& aSourceFileName, 
    TDes& aTargetFileName,
    TVedVideoFormat aOutputFormat,
    TInt aTargetSizeEstimate,
    TFileName aDrive )
{
	LOG(KVideoEditorLogFile, "VideoEditorUtils::GenerateNewDocumentNameL, in:");
	LOGFMT4(KVideoEditorLogFile, "VideoEditorUtils::GenerateNewDocumentNameL 1\tSource file: %S, output format: %d, required space: %d, memory in use: %d", &aSourceFileName, &aOutputFormat, &aTargetSizeEstimate, &aDrive);

	TInt err = KErrNone;

	//	Set file name to parser
	TParsePtrC fileParse (aSourceFileName);

	//  Test if filename is already too long
	if (fileParse.NameAndExt().Length() > KMaxFileName - 5)
	{
		err = KErrArgument;
	}

	// Otherwise proceed to generate the filename
	else
	{
		//  Find file suffix that is not yet used 
		TInt val = 1;
		TFileName temp;

		TFileName driveAndPath = aDrive;
		driveAndPath.Append( PathInfo::VideosPath() );
		
		// create the folder if it doesn't exist
		TRAPD(errBafl, BaflUtils::EnsurePathExistsL (aFsSession, driveAndPath));
		LOGFMT(KVideoEditorLogFile, "VideoEditorUtils::GenerateNewDocumentNameL 2, errBafl:%d", errBafl );
				
		TBool selectedLocationFull = ( KErrNone != errBafl 

			|| !IsEnoughFreeSpaceToSaveL ( aFsSession, driveAndPath, aTargetSizeEstimate ) );

		if (selectedLocationFull)
			{
				err = KErrDiskFull;
			}
	
	    //	Now sufficient disk space has been verified.
        //	Proceed to generate the unique file name.
        if (KErrNone == err)
        {
           LOGFMT(KVideoEditorLogFile, "VideoEditorUtils::GenerateNewDocumentNameL 3, File will be saved to path: %S", &driveAndPath );

            //  Copy drive and path to temporary file name
            temp.Copy( driveAndPath );

            //  Add file name without suffix 
            TPtrC name = fileParse.Name();

            TInt offset = FindSuffix ( name );

            if (offset == KErrNotFound)
	        {
                temp.Append ( fileParse.Name() );
            }
            else
            {
                temp.Append ( name.Left (offset) );
            }
    
            temp.Append ( KEditedSuffix );
            temp.AppendNumFixedWidth (val, EDecimal, 3);
            temp.Append ( aOutputFormat == EVedVideoFormat3GPP ? KExt3gp : KExtMp4 );

            //  Increase edit number until we find a file name that is not used
            while ( FileAlreadyExistsL(aFsSession, temp) )
            {
                ++val;
                temp.Zero();
                temp.Copy ( driveAndPath );
                if (offset == KErrNotFound)
                {
                    temp.Append ( fileParse.Name() );
                }
		        else
                {
                    temp.Append ( name.Left (offset) );
                }

                temp.Append ( KEditedSuffix );
                if (val < 1000)
                {
                    temp.AppendNumFixedWidth ( val, EDecimal, 3);
                }
                else
                {
                    temp.AppendNumFixedWidth ( val, EDecimal, 4);
                }

                temp.Append ( aOutputFormat == EVedVideoFormat3GPP ? KExt3gp : KExtMp4 );
            }

            //  Set document name 
            aTargetFileName.Copy ( temp );
        }
    }

    LOGFMT(KVideoEditorLogFile, "VideoEditorUtils::GenerateNewDocumentNameL: out (%d)", err);    
    return err;
}


//=============================================================================
EXPORT_C TBool VideoEditorUtils::IsEnoughFreeSpaceToSaveL( 
	RFs& aFsSession, 
	const TDesC& aFileName, 
	TInt aSizeEstimate ) 
	{
	LOGFMT(KVideoEditorLogFile, "VideoEditorUtils::IsEnoughFreeSpaceToSaveL: in: %S", &aFileName);

	TBool spaceBelowCriticalLevel( EFalse );

	TParsePtrC fileParse (aFileName);

	TInt mmc = fileParse.Drive().Left(1).CompareF( PathInfo::MemoryCardRootPath().Left(1) );
	if( mmc == 0 )
		{
		spaceBelowCriticalLevel = SysUtil::MMCSpaceBelowCriticalLevelL( 
										&aFsSession, aSizeEstimate );
		}
	else
		{
		spaceBelowCriticalLevel = SysUtil::DiskSpaceBelowCriticalLevelL( 
										&aFsSession, aSizeEstimate, EDriveC );
		}	

	LOGFMT(KVideoEditorLogFile, "VideoEditorUtils::IsEnoughFreeSpaceToSaveL: out: %d", !spaceBelowCriticalLevel);
	return !spaceBelowCriticalLevel;
	}

//=============================================================================
EXPORT_C TBool VideoEditorUtils::IsDrmProtectedL( const TDesC& aFileName ) 
	{
	LOGFMT(KVideoEditorLogFile, "VideoEditorUtils::IsDrmProtectedL: in: %S", &aFileName);

	TBool isProtected = EFalse;
	DRMCommon* drm = DRMCommon::NewL();
	CleanupStack::PushL (drm);
	drm->IsProtectedFile( aFileName, isProtected );
	CleanupStack::PopAndDestroy (drm);

	LOGFMT(KVideoEditorLogFile, "VideoEditorUtils::IsDrmProtectedL: out: %d", isProtected);
	return isProtected;
	}

//=============================================================================
EXPORT_C TFileName VideoEditorUtils::IconFileNameAndPath( TInt aInconFileIndex )
	{
	LOG(KVideoEditorLogFile, "VideoEditorUtils::IconFileNameAndPath: in:");

	TFileName fileName;
	Dll::FileName(fileName);
	TParse p;

	switch (aInconFileIndex)
		{
		case KManualVideoEditorIconFileId:
			p.Set(KManualVideoEditorMifFile, &KDC_APP_BITMAP_DIR, &fileName);
			break;
		case KVideoEditorUiComponentsIconFileId:
			p.Set(KVideoEditorUiComponentsMifFile, &KDC_APP_BITMAP_DIR, &fileName);
			break;
		case KVeiNonScalableIconFileId:
			p.Set(KVideoEditorMbmFile, &KDC_APP_BITMAP_DIR, &fileName);
			break;
		default:
			User::Invariant();
		}

	TPtrC fullName = p.FullName();
	LOGFMT2(KVideoEditorLogFile, "VideoEditorUtils::IconFileNameAndPath: Id: %d, name: %S", aInconFileIndex, &fullName);

	return fullName;
	}

//=============================================================================
TInt VideoEditorUtils::FindSuffix ( 
    const TDesC &   aName
    )
{
    TInt offset = KErrNotFound;
    TInt l = aName.Length();

    while (l)
    {
        l--;
                
        if ( l <= (aName.Length() - 3) && aName[l] == TChar('-') )
        {
            offset = l;
            break;    
        }
        else if ( aName[l] < 0x30 || aName[l] > 0x39 )
        {
            break;                
        }
        
    }

    return offset;
}

//=============================================================================
TBool VideoEditorUtils::FileAlreadyExistsL ( RFs& aFsSession, const TDesC& aFileName )
{
	LOGFMT(KVideoEditorLogFile, "VideoEditorUtils::FileAlreadyExistsL: %S", &aFileName);

    TBool fileExists = BaflUtils::FileExists( aFsSession, aFileName );

	LOGFMT(KVideoEditorLogFile, "VideoEditorUtils::FileAlreadyExistsL: Out: %d", fileExists);
    return fileExists;
}

//=============================================================================
EXPORT_C void VideoEditorUtils::GetMGAlbumsListForMediaFileL ( 
    RArray<TInt>& /*aAlbumIdList*/,
    const TDesC& /*aFileName*/ )
{
    LOG(KVideoEditorLogFile, "VideoEditorUtils::GetMGAlbumsListForMediaFileL: In" );
	LOG(KVideoEditorLogFile, "VideoEditorUtils::GetMGAlbumsListForMediaFileL: Out" );
}


//=============================================================================
EXPORT_C void VideoEditorUtils::AddMediaFileToMGAlbumL( 
    const TDesC& /*aFilename*/, 
    TInt /*aAlbumId*/ )
	{
	LOG(KVideoEditorLogFile, "VideoEditorUtils::AddMediaFileToMGAlbumL: In");
	LOG(KVideoEditorLogFile, "VideoEditorUtils::AddMediaFileToMGAlbumL: Out");
	}

//=============================================================================
EXPORT_C TBool VideoEditorUtils::IsLandscapeScreenOrientation()
	{
	TRect rect;
	AknLayoutUtils::LayoutMetricsRect(AknLayoutUtils::EScreen, rect);
	return rect.Width() > rect.Height();
	}


//=============================================================================
EXPORT_C TInt VideoEditorUtils::LaunchQueryDialogL (const TDesC & aPrompt)
{
	CAknQueryDialog * dlg = 
		new (ELeave) CAknQueryDialog ( const_cast<TDesC&>(aPrompt) );

	return dlg->ExecuteLD (R_VIE_CONFIRMATION_QUERY);
}


//=============================================================================
EXPORT_C TInt VideoEditorUtils::LaunchListQueryDialogL (
	MDesCArray *	aTextItems,
	const TDesC &	aPrompt
	) 
{
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
		return index;
	}
	else
	{
		return -1;
	}
}

//=============================================================================
EXPORT_C TInt VideoEditorUtils::LaunchSaveVideoQueryL () 
{
	//	Create dialog heading and options
    HBufC * heading = CEikonEnv::Static()->AllocReadResourceLC ( R_VIE_QUERY_HEADING_SAVE );
    HBufC * option1 = CEikonEnv::Static()->AllocReadResourceLC ( R_VIE_QUERY_SAVE_NEW );       
    HBufC * option2 = CEikonEnv::Static()->AllocReadResourceLC ( R_VIE_QUERY_SAVE_REPLACE ); 
                
	//	Query dialog texts
	CDesCArray * options = new (ELeave) CDesCArraySeg ( 2 );
	CleanupStack::PushL (options);
	options->AppendL( option1->Des() );
	options->AppendL( option2->Des() );

	//	Execute query dialog
	TInt ret = LaunchListQueryDialogL ( options, *heading );

	options->Reset();
	
	CleanupStack::PopAndDestroy( options ); 
	CleanupStack::PopAndDestroy( option2 );
	CleanupStack::PopAndDestroy( option1 );
	CleanupStack::PopAndDestroy( heading );
		
	return ret;
}


//=============================================================================
EXPORT_C TInt VideoEditorUtils::LaunchSaveChangesQueryL () 
{
	//	Create dialog prompt
    HBufC * prompt = CEikonEnv::Static()->AllocReadResourceLC ( R_VIE_QUERY_SAVE_CHANGES );
    
	//	Execute query dialog
	TInt ret = LaunchQueryDialogL ( *prompt );

	CleanupStack::PopAndDestroy( prompt );

	return ret;
}

// End of File
