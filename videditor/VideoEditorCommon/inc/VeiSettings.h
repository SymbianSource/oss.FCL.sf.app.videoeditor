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


#ifndef __VEISETTINGS_H__
#define __VEISETTINGS_H__

#include <s32strm.h>
#include <CAknMemorySelectionDialog.h>

// Max size of serialized settings object:
//	DefaultSnapshotName	256 bytes
//	DefaultVideoName	256 bytes
//	MemoryInUse			8 bit
//	SaveQuality			8 bit
const TInt KveiSettingsMaxSerializedSizeInBytes = 520;

NONSHARABLE_CLASS( TVeiSettings )
	{
    public:     // Enumerations

        enum TSaveQuality 
            {
            EAuto = 0,
            EMmsCompatible,
            EMedium,
            EBest
            };

	public:
		/** 
		 * Getter/setter for default snapshot name, non-const
		 */
		IMPORT_C TDes& DefaultSnapshotName();
	
		/**
		 * Getter for default snapshot name, const
		 */
		IMPORT_C TPtrC DefaultSnapshotName() const;

		/**
		 * Getter/setter for default video name, non-const
		 */
		IMPORT_C TDes& DefaultVideoName();

		/**
		 * Getter for default video name, const
		 */
		IMPORT_C TPtrC DefaultVideoName() const;

		/**
		 * Getter/setter for used memory, non-const
		 */
		IMPORT_C CAknMemorySelectionDialog::TMemory& MemoryInUse();

		/**
		 * Getter for used memory, const
		 */
		IMPORT_C const CAknMemorySelectionDialog::TMemory& MemoryInUse() const;

        /**
         * Getter for save quality, const
         */
        IMPORT_C TInt& SaveQuality();

        /**
         * Getter for save quality, const
         */
        IMPORT_C TInt SaveQuality() const;

		IMPORT_C void ExternalizeL(RWriteStream& aStream) const;

		IMPORT_C void InternalizeL(RReadStream& aStream);

#ifdef SETTINGS_TO_CENREP
	public:
        /**
         * Load values from Central Repository
         */
		void LoadL();

        /**
         * Save values to Central Repository
         */
		void SaveL() const;
#endif

	private:  // Member data.
		/**
		 * Default snapshot name.
		 */
		TBuf<128>	iDefaultSnapshotName;
	
		/**
		 * Default video name.
		 */
		TBuf<128>	iDefaultVideoName;

		/**
		 * Memory in use.
		 */
		CAknMemorySelectionDialog::TMemory iMemoryInUse;

        /**
         * Save quality.
         */
        TSaveQuality        iSaveQuality;

};

#endif
