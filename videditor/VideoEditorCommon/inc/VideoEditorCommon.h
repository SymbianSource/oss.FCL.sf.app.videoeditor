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




#ifndef VIDEOEDITORCOMMON_H
#define VIDEOEDITORCOMMON_H

// INCLUDES
// System includes
#include <f32file.h>    // TDriveNumber
#include <gdi.h>        // TDisplayMode
#include "VideoEditorCommon.hrh"

// TYPE DEFINITIONS
namespace VideoEditor
	{
	/**
	* Enumeration for possible cursor locations.
	*/
	enum TCursorLocation 
		{
		ECursorOnClip = 0,
		ECursorOnTransition,
		ECursorOnAudio,
		ECursorOnEmptyVideoTrack,
		ECursorOnEmptyAudioTrack
		};

	/**
	* Enumeration for used memory locations.
	*/
	enum TMemory 
		{
		EMemAutomatic = 0,
		EMemPhoneMemory,
		EMemMemoryCard
		};

	/**
	* Publish & Subscribe property keys.
	*/
	enum TPropertyKeys
		{
		EPropertyFilename   // Document file name
		};
	
	
	/**
	* Animations for simple functions
	*/
	enum TSimpleFunctionAnimations
		{
		EAnimationMerging,
		EAnimationChangeAudio,
		EAnimationAddText,
		EAnimationCut
		};
	}


// CONSTANTS
const TUid KUidVideoEditor = { 0x10208A29 };		// app. UID of the manual video editor
const TUid KUidSimpleCutVideo = {0x200009DF};		// app. UID of the simple cut video
const TUid KUidVideoProvider = { 0x101FFA8E };		// interface UID of the AIW provider
const TUid KUidTrimForMms = {0x200009D};            // app. UID of the trim for mms
const TUid KUidVideoEditorProperties = {0x03341234};// Property UID for Publish & Subscribe API. NOTE: currently unregistered UID

const TInt KMmcDrive( EDriveE );    // Memory card drive number

const TInt KAudioSampleInterval = 200; // Audio sample rate in audio visualization (in milliseconds)
const TReal KVolumeMaxGain = 127; // scale is -127 - 127

const TInt KMaxVideoFrameResolutionX = 640; // use VGA as maximum resolution
const TInt KMaxVideoFrameResolutionY = 480;
const TDisplayMode KVideoClipGenetatorDisplayMode = EColor64K;
const TInt KMinCutVideoLength = 1000000; // in microseconds

// If the number of files given to AIW provider (i.e. files selected in Gallery)
// exceeds this number, the AIW provider does not provide menu items.
// Otherwise the menu slows down dramatically when a very large number
// of files is selected.
const TInt KAiwMaxNumberOfFilesSimultaneouslyHandled = KMaxTInt;

// erros codes used in simple functions "Merge", "Add text", "Change sound"
const TInt KErrUnableToInsertVideo 			= -50000;
const TInt KErrUnableToInsertSound 			= KErrUnableToInsertVideo - 1;
const TInt KErrUnableToInsertImage 			= KErrUnableToInsertVideo - 2;
const TInt KErrUnableToInsertText 			= KErrUnableToInsertVideo - 3;
const TInt KErrUnableToMergeVideos 			= KErrUnableToInsertVideo - 4;
const TInt KErrUnableToMergeVideoAndImage 	= KErrUnableToInsertVideo - 5;
const TInt KErrUnableToChangeSound 			= KErrUnableToInsertVideo - 6;
const TInt KErrVideoFormatNotSupported 		= KErrUnableToInsertVideo - 7;
const TInt KErrAudioFormatNotSupported 		= KErrUnableToInsertVideo - 8;
const TInt KErrImageFormatNotSupported 		= KErrUnableToInsertVideo - 9;
const TInt KErrUnableToEditVideo 		    = KErrUnableToInsertVideo - 10;

// error code(s) used in simple function cut
const TInt KErrTooShortVideoForCut 			= -60000;

// file name extensions
_LIT (KExtMp4, ".mp4");
_LIT (KExt3gp, ".3gp");
_LIT (KExtAmr, ".amr");

#endif      // VIDEOEDITORCOMMON_H   
            
// End of File
