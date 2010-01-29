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


#ifndef VIDEOEDITORINTERNALCRKEYS_H
#define VIDEOEDITORINTERNALCRKEYS_H

const TUid KCRUidVideoEditor = {0xE3341234}; // test uid

/**
* KVedSaveQuality 
* Defines the quality of saved video clip.
* Possible values are 	0 ( TVeiSettings::EAuto ) 
* 						1 ( TVeiSettings::EMmsCompatible ) 
*						2 ( TVeiSettings::EMedium )
*						2 ( TVeiSettings::EBest )
* Default value is 0.
*/
const TUint32 KVedSaveQuality 					= 0x0000001;

/**
* KVedMemoryInUse 
* Defines where to same the video clip.
* Possible values are 	0 ( CAknMemorySelectionDialog::EPhoneMemory ) 
* 						1 ( CAknMemorySelectionDialog::EMemoryCard ) 
* Default value is 1.
*/
const TUint32 KVedMemoryInUse 					= 0x0000002;

/** Default file name for saved video
*
* Text type
*
* Default value: ""
**/
const TUint32 KVedDefaultVideoName				= 0x00000003;

/** Default file name for snapshot image
*
* Text type
*
* Default value: ""
**/
const TUint32 KVedDefaultSnapshotName			= 0x00000004;


#endif      // VIDEOEDITORINTERNALCRKEYS_H

