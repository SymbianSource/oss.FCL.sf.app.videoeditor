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
#include <eikenv.h>
#include <pathinfo.h>
#include <eikappui.h>
#include <bautils.h>
#include <e32math.h>
#include <vedcommon.h>

// User includes
#include "VeiTempMaker.h"
#include "VideoEditorCommon.h"
#include "VeiSettings.h"
#include "VideoEditorDebugUtils.h"


EXPORT_C CVeiTempMaker* CVeiTempMaker::NewL()
    {
    CVeiTempMaker* self = NewLC();
    CleanupStack::Pop(self);
    return self;
    }

    
EXPORT_C CVeiTempMaker* CVeiTempMaker::NewLC()
    {
    CVeiTempMaker* self = new (ELeave) CVeiTempMaker();
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
    }

void CVeiTempMaker::ConstructL()
	{
	}

CVeiTempMaker::CVeiTempMaker()
    {
    }


EXPORT_C CVeiTempMaker::~CVeiTempMaker()
    {
    }

EXPORT_C void CVeiTempMaker::EmptyTempFolder() const
	{
	LOG(KVideoEditorLogFile, "CVeiTempMaker::EmptyTempFolder(): In");

	TRAP_IGNORE( DoEmptyTempFolderL() );

	LOG(KVideoEditorLogFile, "CVeiTempMaker::EmptyTempFolder(): Out");
	}

void CVeiTempMaker::DoEmptyTempFolderL() const
	{
	LOG(KVideoEditorLogFile, "CVeiTempMaker::DoEmptyTempFolderL(): In");

	RFs&	fs = CCoeEnv::Static()->FsSession();

	CFileMan* fileManager = CFileMan::NewL( fs );
	CleanupStack::PushL( fileManager );
		
	TFileName tempDir;
	// First try to delete from Phone Memory
	TBool dirExists = GetTempPath( CAknMemorySelectionDialog::EPhoneMemory, tempDir );
	if ( dirExists )
		{
		LOGFMT(KVideoEditorLogFile, "\tFolder \"%S\" exists...Deleting...", &tempDir);
		fileManager->RmDir( tempDir );
		}
	// ..then from MMC
	dirExists = GetTempPath( CAknMemorySelectionDialog::EMemoryCard, tempDir );
	if ( dirExists )
		{
		LOGFMT(KVideoEditorLogFile, "\tFolder \"%S\" exists...Deleting...", &tempDir);
		fileManager->RmDir( tempDir );
		}

	CleanupStack::PopAndDestroy( fileManager );

	LOG(KVideoEditorLogFile, "CVeiTempMaker::DoEmptyTempFolderL(): Out");
	}

EXPORT_C void CVeiTempMaker::GenerateTempFileName( 
	HBufC& aTempPathAndName, 
	CAknMemorySelectionDialog::TMemory aMemory,
	TVedVideoFormat aVideoFormat,
	TBool aExtAMR ) const
	{
	LOG(KVideoEditorLogFile, "CVeiTempMaker::GenerateTempFileName(): In");

	RFs&	fs = CCoeEnv::Static()->FsSession();
	

// Parse tempPath. MMC or memoryroot
	TFileName tempPath;
	RFile temp;
// Temp files are processed to \\data\\videos\\[application uid]\\[random name]
	TBool tempFolderExists = GetTempPath( aMemory, tempPath );
	if ( !tempFolderExists )
		{
		fs.MkDirAll( tempPath );
		fs.SetAtt( tempPath, KEntryAttHidden, KEntryAttDir );
		}
	
	TUint32 randomName;

	randomName = Math::Random();
	tempPath.AppendNum( randomName, EHex );
	
	if ( aExtAMR )
		{
		tempPath.Append( KExtAmr );
		}	
	else if ( aVideoFormat == EVedVideoFormatMP4 )
		{
		tempPath.Append( KExtMp4 );
		}
	else
		{
		tempPath.Append( KExt3gp );
		}

	temp.Create( fs, tempPath, EFileWrite );
	temp.Close();

	aTempPathAndName = tempPath;

	LOGFMT(KVideoEditorLogFile, "CVeiTempMaker::GenerateTempFileName(): Out: %S", &tempPath);
	}
	
TBool CVeiTempMaker::GetTempPath( const CAknMemorySelectionDialog::TMemory& aMemory, TDes& aTempPath ) const
	{
	LOG(KVideoEditorLogFile, "CVeiTempMaker::GetTempPath: In");

	if ( aMemory == CAknMemorySelectionDialog::EPhoneMemory )
		{
		aTempPath = PathInfo::PhoneMemoryRootPath();
		}
	else
		{
		aTempPath = PathInfo::MemoryCardRootPath();
		}
		
	aTempPath.Append( PathInfo::VideosPath() ); 	
	aTempPath.AppendNum( KUidVideoEditor.iUid, EHex );
	aTempPath.Append(_L("\\"));	

	LOGFMT(KVideoEditorLogFile, "CVeiTempMaker::GetTempPath: Out: %S", &aTempPath);

	return BaflUtils::FolderExists( CCoeEnv::Static()->FsSession(), aTempPath );
	}

/*void CVeiTempMaker::ListFilesL(const TDesC& aFindFromDir, const TDesC& aWriteResultTo) const
{	
	LOGFMT(KVideoEditorLogFile, "CVeiTempMaker::ListFiles(): In, aFindFromDir:%S", &aFindFromDir);
	CDir* dir = NULL;
	//RFile file;
	RFs fileSession;
	
	TInt err = fileSession.Connect();	

//	err = file.Replace(fileSession, aWriteResultTo, EFileWrite);

	
	CleanupClosePushL(fileSession);
	//CleanupClosePushL(file);

	//_LIT8(KNewLine, "\r\n");
	//TBuf8<255> buf8;
	TBuf<KMaxFileName> buf;
	TFileName fileName;
	fileName.Append(aFindFromDir); 
	
	fileSession.GetDir(fileName, KEntryAttNormal, ESortNone, dir);
	LOG(KVideoEditorLogFile, "CVeiTempMaker::ListFiles(): 2");
	if (dir)
	{
		CleanupStack::PushL(dir);
		

		for(TInt index=0; index < dir->Count(); index++)
		{
			LOG(KVideoEditorLogFile, "CVeiTempMaker::ListFiles(): 3 in loop");

			//buf8.Copy((*dir)[index].iName);		
			//buf.Copy((*dir)[index].iName);		
			
			LOG(KVideoEditorLogFile, "CVeiTempMaker::ListFiles(): 4 in loop");

			LOGFMT(KVideoEditorLogFile, "%S", &((*dir)[index].iName));			
			//file.Write(buf8);

			//file.Write(KNewLine);		

		}
		CleanupStack::PopAndDestroy( dir );	
	}	
	LOG(KVideoEditorLogFile, "CVeiTempMaker::ListFiles(): 5");
	//CleanupStack::PopAndDestroy( file );	
	CleanupStack::PopAndDestroy( fileSession );	
	LOG(KVideoEditorLogFile, "CVeiTempMaker::ListFiles(): Out");
}*/

// End of File
