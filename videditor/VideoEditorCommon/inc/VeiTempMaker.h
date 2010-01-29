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


#ifndef __VEITEMPMAKER_H
#define __VEITEMPMAKER_H

#include <e32base.h>
#include <e32def.h>
#include <caknmemoryselectiondialog.h>
#include <vedcommon.h>
/**
* CVeiTempMaker. Class for temporary file actions(filename, clean up).
*
*/
NONSHARABLE_CLASS( CVeiTempMaker ) : public CBase
	{
	public:
		/**
		 * Static factory constructor.
		 *
		 * @return  created instance
		 */
		IMPORT_C static CVeiTempMaker* NewL();

		/**
		 * Static factory constructor. Leaves the created instance in the
		 * cleanup stack.
		 *
		 * @return  created instance
		 */
		IMPORT_C static CVeiTempMaker* NewLC();

		/**
		 * Destructor.
		 */
		IMPORT_C ~CVeiTempMaker();

		IMPORT_C void EmptyTempFolder() const;

		IMPORT_C void GenerateTempFileName( 
			HBufC& aTempPathAndName, 
			CAknMemorySelectionDialog::TMemory aMemory,
			TVedVideoFormat aVideoFormat,
			TBool aExtAMR = EFalse ) const;

	private:
		/**
		 * Default constructor.
		 */
		void ConstructL();

		/**
		 * Default constructor.
		 */
		CVeiTempMaker();
	
		TBool GetTempPath( const CAknMemorySelectionDialog::TMemory& aMemory, TDes& aTempPath ) const;
		
//		void ListFilesL(const TDesC& aFindFromDir, const TDesC& aWriteResultTo) const;
		
		void DoEmptyTempFolderL() const;
	};
#endif
