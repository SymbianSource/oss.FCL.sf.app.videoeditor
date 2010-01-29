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

// User includes
#include "VeiSettings.h"

#ifdef SETTINGS_TO_CENREP
#include <centralrepository.h>
#include "VideoEditorInternalCRKeys.h"
#endif

EXPORT_C TDes& TVeiSettings::DefaultSnapshotName() 
	{
	return iDefaultSnapshotName;
	}

EXPORT_C TPtrC TVeiSettings::DefaultSnapshotName() const
	{
	return iDefaultSnapshotName;
	}

EXPORT_C TDes& TVeiSettings::DefaultVideoName()
	{
	return iDefaultVideoName;
	}

EXPORT_C TPtrC TVeiSettings::DefaultVideoName() const
	{
	return iDefaultVideoName;
	}

EXPORT_C CAknMemorySelectionDialog::TMemory& TVeiSettings::MemoryInUse()
	{
	return iMemoryInUse;
	}

EXPORT_C const CAknMemorySelectionDialog::TMemory& TVeiSettings::MemoryInUse() const
	{
	return iMemoryInUse;
	}

EXPORT_C TInt& TVeiSettings::SaveQuality()
    {
    return (TInt&)iSaveQuality;
    }

EXPORT_C TInt TVeiSettings::SaveQuality() const
    {
    return (TInt)iSaveQuality;
    }

EXPORT_C void TVeiSettings::ExternalizeL(RWriteStream& aStream) const 
	{
    aStream << iDefaultVideoName;

    aStream.WriteUint8L( static_cast<TUint8>(iSaveQuality) );

    aStream << iDefaultSnapshotName;

    aStream.WriteUint8L( static_cast<TUint8>(iMemoryInUse) );
	}

EXPORT_C void TVeiSettings::InternalizeL(RReadStream& aStream) 
	{
    aStream >> iDefaultVideoName;

    iSaveQuality = static_cast<TSaveQuality>(aStream.ReadUint8L());

    aStream >> iDefaultSnapshotName;

    iMemoryInUse = static_cast<CAknMemorySelectionDialog ::TMemory> (aStream.ReadUint8L());
	}

#ifdef SETTINGS_TO_CENREP

void TVeiSettings::LoadL()
	{
	CRepository* repository = NULL;
	User::LeaveIfError( repository = CRepository::NewLC(KCRUidVideoEditor) );
	
	TInt saveQuality = 0;
	repository->Get(KVedSaveQuality, saveQuality);
	SetSaveQuality( (TSaveQuality)saveQuality );

	TInt memoryInUse = 0;
	repository->Get(KVedMemoryInUse, memoryInUse);
	SetMemoryInUse( (CAknMemorySelectionDialog::TMemory)memoryInUse );

	repository->Get(KVedDefaultVideoName, iDefaultVideoName);

	repository->Get(KVedDefaultSnapshotName, iDefaultSnapshotName);

	CleanupStack::PopAndDestroy(repository);	
	}

void TVeiSettings::SaveL() const
	{
	CRepository* repository = CRepository::NewLC(KCRUidVideoEditor);
	
	repository->Set(KVedSaveQuality, (TInt)iSaveQuality);

	repository->Set(KVedMemoryInUse, (TInt)iMemoryInUse);

	repository->Set(KVedDefaultVideoName, iDefaultVideoName);

	repository->Set(KVedDefaultSnapshotName, iDefaultSnapshotName);

	CleanupStack::PopAndDestroy(repository);
	}

#endif

// End of File
