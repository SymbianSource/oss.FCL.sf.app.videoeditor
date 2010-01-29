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




#ifndef VEIMGFETCHVERIFIER_H
#define VEIMGFETCHVERIFIER_H

#include <mmgfetchverifier.h>
#include <ConeResLoader.h>

/**
 * CVeiMGFetchVerifier class.
 */
NONSHARABLE_CLASS( CVeiMGFetchVerifier ) : public CBase, public MMGFetchVerifier
	{
	public:
		IMPORT_C static CVeiMGFetchVerifier* NewLC();
		IMPORT_C virtual ~CVeiMGFetchVerifier();
        IMPORT_C virtual TBool VerifySelectionL( const MDesCArray* aSelectedFiles );

	private:
		CVeiMGFetchVerifier();
		void ConstructL();
	
	private:
		RConeResourceLoader 	iResLoader;
	};
#endif

//End of file
