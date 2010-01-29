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


#ifndef VIDEOEDITORUTILS_H
#define VIDEOEDITORUTILS_H

// INCLUDES
#include <e32base.h>
#include <coemain.h>
#include <VedCommon.h>
#include "VideoEditorCommon.h"

// FORWARD DECLARATIONS
class RFs;
class TVeiSettings;

// CONSTANTS
const TInt KManualVideoEditorIconFileId = 0;
const TInt KVideoEditorUiComponentsIconFileId = 1;
const TInt KVeiNonScalableIconFileId = 2;

/**	CLASS:	VideoEditorUtils
* 
*	Static utility class.
*
*/
NONSHARABLE_CLASS( VideoEditorUtils )
{
public:

    /** @name Methods:*/
    //@{

    /**	GenerateNewDocumentNameL 
	*
	*   Generates a new name for the loaded document at the startup.
	*
    *   The generated file is located on the MMC dy default. If the 
    *   MMC is full or not present, the file will be in phone memory.
    *   Uses internally VideoFitsToDriveL to check the space.
    *
	*   Media gallery album id list now obsolete after Media Gallery removal.
    *	If an invalid album list is passed the behaviour is undefined.
	*
	*   @param aFsSession -
	*   @param aSourceFileName -
	*   @param aTargetFileName -
	*   @param aOutputFormat - video format of the target file
	*   @param aTargetSizeEstimate - the expected target file size. 
	*             This value is used to check whether there is enough space on the disks.
	*   @param aMemoryInUse - where the target file is created.
	*   @return - KErrNone if successfully generated file name
	*             KSIEENotEnoughDiskSpace if no disk space to save the file
	*             (size of aSourceFileName used to check the space)
	*             KSIEEOpenFile if the source filename is invalid
	*/
    IMPORT_C static TInt GenerateNewDocumentNameL (
        RFs& aFsSession, 
        const TDesC& aSourceFileName, 
        TDes& aTargetFileName,
        TVedVideoFormat aOutputFormat,
        TInt aTargetSizeEstimate,
        VideoEditor::TMemory aMemoryInUse = VideoEditor::EMemAutomatic
        );
        
    /**	GenerateFileNameL 
    *
    *   Generates a new file name.
    *
    *   Generates a file name into a given drive. Increments a running number
    *   at the end of the file name until a file with the same name doesn't exist.
    *   Uses internally VideoFitsToDriveL to check the space.
    *
    *
    *   @param aFsSession -
    *   @param aSourceFileName - name of the source file
    *   @param aTargetFileName - name of the new file
    *   @param aOutputFormat - video format of the target file
    *   @param aTargetSizeEstimate - the expected target file size. 
    *             This value is used to check whether there is enough space on the disks.
    *   @param aDrive - drive where the target file is created.
    *   @return - KErrNone if successfully generated file name
    *             KSIEENotEnoughDiskSpace if no disk space to save the file
    *             (size of aSourceFileName used to check the space)
    *             KSIEEOpenFile if the source filename is invalid
    */        
    IMPORT_C static TInt GenerateFileNameL (
        RFs& aFsSession, 
        const TDesC& aSourceFileName, 
        TDes& aTargetFileName,
        TVedVideoFormat aOutputFormat,
        TInt aTargetSizeEstimate,
        TFileName aDrive );


    /**	NotifyNewMediaDocumentL 
	*
	*   Notifies the system that a new media file has been saved,
	*   making it visible in the Media Gallery.
	*   
	*   @param aFileName -
	*   @return - 
	*/
    IMPORT_C static void NotifyNewMediaDocumentL (
        RFs& aFsSession,
        const TDesC& aFileName
        );

    /**	GetMGAlbumsListForMediaFileL 
	*
	*   Finds out which Media Gallery Albums the file belongs to.
	*   (if compiled without album support, this function
	*   returns an empty array). Album support is now removed.
	*
	*   @param aFileName - The media file
	*   @param aAlbumList -
    *           On return, contains the ID:s of the Media Gallery
    *           albums the file belongs to.
    *           If the list is not empty, it is retained and
    *           new IDs are appended into the list.
	*   @return - 
	*/
    IMPORT_C static void GetMGAlbumsListForMediaFileL ( 
        RArray<TInt>& aAlbumIdList,
        const TDesC& aFilename
        );

    /**	AddMediaFileToMGAlbumL 
	*
	*   Add the given media file to the given album.
	*   (if compiled without album support, this function
	*   does nothing)
	*   
	*   @param aFileName -
	*   @param aAlbumId -
	*   @return - 
	*/
    IMPORT_C static void AddMediaFileToMGAlbumL ( 
        const TDesC& aFileName,
        TInt aAlbumId
        );

    /**	IsEnoughFreeSpaceToSaveL
	*
	*   Checks whether there is enough disk space to save
	*   the given file.
	*
	*   @param aFsSession -
	*   @param aFileName - The path to save (only drive part needed)
	*   @param aSizeEstimate - The space required for the file to save
	*   @return - TBool
	*/
	IMPORT_C static TBool IsEnoughFreeSpaceToSaveL( 
		RFs& aFsSession, 
		const TDesC& aFileName,
		TInt aSizeEstimate  );

    /**	IsDrmProtectedL
	*
	*   Checks whether the given file is DRM protected.
	*
	*   @param aFileName - The path of the file to check
	*   @return - TBool
	*/
	IMPORT_C static TBool IsDrmProtectedL( const TDesC& aFileName );

    /**	IconFileNameAndPath 
	*
	*	Returns file name and path for one of the icon files used in this module.
	*	Currently there is the primary icon file (MIF file), and the secondary 
	*	icon file (a MBM file), which contains the non-scalable (bitmap) graphics.
	*   
	*	@param  TInt aInconFileIndex -	Which file. Supported values:
	*				KManualVideoEditorIconFileId
	*				KVideoEditorUiComponentsIconFileId
	*				KVeiNonScalableIconFileId 
	*	@return TFileName - 
	*/
   	IMPORT_C static TFileName IconFileNameAndPath( TInt aInconFileIndex );

    /**	IsLandscapeScreenOrientation
	*
	*   Check if the screen is in landscape mode. In other words, see
	*   if the X dimension of the screen is greater than the Y dimendion.
	*
	*   @param -
	*   @return - TBool
	*/
	IMPORT_C static TBool IsLandscapeScreenOrientation();


	/** LaunchQueryDialogL
	*
    *   Launches a confirmation query dialog.
    *
	*	@param aPrompt - dialog prompt descriptor
	*	@return -
	*/
    IMPORT_C static TInt LaunchQueryDialogL (const TDesC & aPrompt);

	/** LaunchListQueryDialogL
	*
    *   Launches a confirmation query dialog.
    *
	*	@param aPrompt - dialog prompt descriptor
	*	@return 0 if user selects "No", otherwise 1
	*/
	IMPORT_C static TInt LaunchListQueryDialogL (MDesCArray *	aTextItems,
										const TDesC &	aPrompt);
										
	/*	LaunchSaveVideoQueryL 
	*
	*	Launches a query dialog "Save video:" with items
	*	"Replace original" and "Save with a new name"
	*
	*   @param - 
	*   @return - list query id or -1 if the user selects No
	*/        
	IMPORT_C static TInt LaunchSaveVideoQueryL ();


	/*	LaunchSaveChangesQueryL
	*
	*	Launches a query dialog "Save changes?" query.
	*
	*   @param - 
	*   @return 0 if user selects "No", otherwise 1
	*/
	IMPORT_C static TInt LaunchSaveChangesQueryL ();

    //@}

private:

    /** @name Methods:*/
    //@{

    /**	FindSuffix 
	*
	*   Finds the offset of the edited file name suffix of form "-NNN", where
    *   NNN is the edit sequence number.
	*
	*   @param aName - File name without extension
	*   @return - 
	*/
    static TInt FindSuffix ( 
        const TDesC &   aName
        );

    /**	FileAlreadyExistsL 
	*
	*   Check if a file with the specified name already exista on the system.
	*
	*   @param - aFs
	*   @param - aFileName
	*   @return - TBool
	*/
    static TBool FileAlreadyExistsL ( 
        RFs& aFsSession, 
        const TDesC& aFileName 
        );

    //@}
};

#endif

// End of File

