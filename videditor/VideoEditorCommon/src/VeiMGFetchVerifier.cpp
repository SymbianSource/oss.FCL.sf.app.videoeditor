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


#include <coemain.h>
#include <aknnotewrappers.h>
#include <drmcommon.h>
#include <stringloader.h>
#include <data_caging_path_literals.hrh>

#include <VideoEditorCommon.rsg>
#include "VeiMGFetchVerifier.h"
#include "VideoEditorCommon.h"
#include "VideoEditorDebugUtils.h"

// CONSTANTS
_LIT(KResourceFile, "VideoEditorCommon.rsc");


EXPORT_C CVeiMGFetchVerifier* CVeiMGFetchVerifier::NewLC()
	{
	CVeiMGFetchVerifier* self = new ( ELeave ) CVeiMGFetchVerifier;
    CleanupStack::PushL( self );
    self->ConstructL();
    return self;
	}

CVeiMGFetchVerifier::CVeiMGFetchVerifier() : iResLoader( *CCoeEnv::Static() )
	{
	}

EXPORT_C CVeiMGFetchVerifier::~CVeiMGFetchVerifier()
	{
	iResLoader.Close();
	}

EXPORT_C TBool CVeiMGFetchVerifier::VerifySelectionL( const MDesCArray* aSelectedFiles )
	{
	LOG(KVideoEditorLogFile, "CVeiMGFetchVerifier::VerifySelectionL: in");
        
	TBool selectionAccepted( ETrue );

	DRMCommon* drmCommon = DRMCommon::NewL();
	CleanupStack::PushL (drmCommon);
	
	User::LeaveIfError( drmCommon->Connect() );
	
	TUint itemCount = aSelectedFiles->MdcaCount();
	for (TUint i=0;i<itemCount;i++)
		{
		TInt fileProtectionStatus(0);
		
		TPtrC fileName = aSelectedFiles->MdcaPoint(i);
		
		drmCommon->IsProtectedFile( fileName, fileProtectionStatus );

		if ( fileProtectionStatus != EFalse )
			{
			LOGFMT(KVideoEditorLogFile, "CVeiMGFetchVerifier::VerifySelectionL: File (%S) is protected. Show error note.", &fileName);
			HBufC* stringholder;
			CCoeEnv* coeEnv = CCoeEnv::Static();

			stringholder = StringLoader::LoadLC( R_VEI_NOTE_DRM_NOT_ALLOWED, coeEnv );

			CAknInformationNote* note = new ( ELeave ) CAknInformationNote( ETrue );
			note->ExecuteLD( *stringholder );

			CleanupStack::PopAndDestroy( stringholder );
			selectionAccepted = EFalse;	
			break;
			}
		LOGFMT(KVideoEditorLogFile, "CVeiMGFetchVerifier::VerifySelectionL: File (%S) is not protected.", &fileName);
		}
	drmCommon->Disconnect();
	CleanupStack::PopAndDestroy (drmCommon);

	LOGFMT(KVideoEditorLogFile, "CVeiMGFetchVerifier::VerifySelectionL: out: %d", selectionAccepted);
	return selectionAccepted;
	}

	
void CVeiMGFetchVerifier::ConstructL()
	{
	// Locate and open the resource file
    TFileName fileName;
    TParse p;    

    Dll::FileName(fileName);
    p.Set(KResourceFile, &KDC_RESOURCE_FILES_DIR, &fileName);
    fileName = p.FullName();
    
    LOGFMT(KVideoEditorLogFile, "\tLoading resource file: %S", &fileName);
	iResLoader.OpenL( fileName ); // RConeResourceLoader selects the correct language file

	}

// End of file
