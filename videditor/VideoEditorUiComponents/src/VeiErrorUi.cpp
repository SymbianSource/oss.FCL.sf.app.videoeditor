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
#include <errorui.h>
#include <StringLoader.h>
#include <AknNoteWrappers.h>
#include <data_caging_path_literals.hrh>
#include <VideoeditorUiComponents.rsg>
#include "VeiErrorUi.h"
#include "VideoEditorDebugUtils.h"

// CONSTANTS
_LIT(KResourceFile, "VideoEditorUiComponents.rsc");


//=============================================================================
EXPORT_C CVeiErrorUI* CVeiErrorUI::NewL( CCoeEnv& aCoeEnv )
	{
	CVeiErrorUI* self = new (ELeave) CVeiErrorUI( aCoeEnv );
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

//=============================================================================
void CVeiErrorUI::ConstructL()
	{
	iErrorUI = CErrorUI::NewL();

	// Locate and open the resource file
    TFileName fileName;
    TParse p;    

    Dll::FileName(fileName);
    p.Set(KResourceFile, &KDC_RESOURCE_FILES_DIR, &fileName);
    fileName = p.FullName();
    
    LOGFMT(KVideoEditorLogFile, "\tLoading resource file: %S", &fileName);
	iResLoader.OpenL( fileName ); // RConeResourceLoader selects the correct language file
	}

//=============================================================================
CVeiErrorUI::CVeiErrorUI( CCoeEnv& aCoeEnv ) : iCoeEnv( aCoeEnv ), iResLoader( aCoeEnv )
	{
	}

//=============================================================================
CVeiErrorUI::~CVeiErrorUI()
	{
	delete iErrorUI;
	iResLoader.Close();
	}

//=============================================================================
EXPORT_C TInt CVeiErrorUI::ShowGlobalErrorNote( const TInt aError ) const
    {
    LOGFMT(KVideoEditorLogFile, "CVeiErrorUI::ShowGlobalErrorNoteL: In (%d)", aError);

	TRAPD ( err,
	    switch( aError )
	        {
	        case 0:
	            {
	            // do nothing to KErrNone
	            break;
	            }
	        case -3:
	            {
	            // do nothing to KErrCancel
	            break;
	            }
	        case -4: // KErrNoMemory
	            {
	            iErrorUI->ShowGlobalErrorNoteL( aError );
	            break;
	            }
	        case -5: // KErrNotSupported
	            {
	            iErrorUI->ShowGlobalErrorNoteL( aError );
	            break;
	            }
	        case -14: // KErrInUse
	            {
	            iErrorUI->ShowGlobalErrorNoteL( aError );
	            break;
	            }
	        case -26: // KErrDiskFull
	            {
	            iErrorUI->ShowGlobalErrorNoteL( aError );
	            break;
	            }
	        case -33: // KErrTimedOut
	            {
	            iErrorUI->ShowGlobalErrorNoteL( aError );
	            break;
	            }
	        case -46: // KErrPermissionDenied
	            {
	            iErrorUI->ShowGlobalErrorNoteL( aError );
	            break;
	            }
	        default :
	            {
				HBufC* stringholder;
				stringholder = StringLoader::LoadLC( R_VEI_DEFAULT_ERROR_NOTE, &iCoeEnv );
				CAknErrorNote* dlg = new ( ELeave ) CAknErrorNote( ETrue );
				dlg->ExecuteLD( *stringholder );
				CleanupStack::PopAndDestroy( stringholder );
	            break;
	            }
	        }
		);

    LOGFMT(KVideoEditorLogFile, "CVeiErrorUI::ShowGlobalErrorNote: Out: %d", err);

    return err;
    }

//=============================================================================
EXPORT_C TInt CVeiErrorUI::ShowErrorNote( CCoeEnv& aCoeEnv, const TInt aResourceId, TInt aError )
	{
	LOG(KVideoEditorLogFile, "CVeiErrorUI::ShowErrorNote: in");

	TRAPD ( err,

		HBufC* stringholder;
		if ( aError == 0 )
			{
			stringholder = StringLoader::LoadLC( aResourceId, &aCoeEnv );
			}
		else
			{
			stringholder = StringLoader::LoadLC( aResourceId, aError, &aCoeEnv );
			}

		CAknErrorNote* dlg = new ( ELeave ) CAknErrorNote( ETrue );
		dlg->ExecuteLD( *stringholder );

		CleanupStack::PopAndDestroy( stringholder );
	);

	LOGFMT(KVideoEditorLogFile, "CVeiErrorUI::ShowErrorNote: out: %d", err);

	return err;
	}

// End of File
