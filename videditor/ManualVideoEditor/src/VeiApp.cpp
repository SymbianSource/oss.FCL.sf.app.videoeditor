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
#include <eikstart.h>
// User includes
#include "VeiApp.h"
#include "VeiDocument.h"
#include "VideoEditorCommon.h"  // Application UID
#include "VideoEditorDebugUtils.h"

// ================= MEMBER FUNCTIONS =======================

// ================= OTHER EXPORTED FUNCTIONS ==============
//
// ---------------------------------------------------------
// NewApplication() 
// Constructs CVeiApp
// Returns: created application object
// ---------------------------------------------------------
//
EXPORT_C CApaApplication* NewApplication()
    {
    return new CVeiApp;
    }

GLDEF_C TInt E32Main()
    {
    return EikStart::RunApplication( NewApplication );
    }

// ---------------------------------------------------------
// CVeiApp::AppDllUid()
// Returns application UID
// ---------------------------------------------------------
//
TUid CVeiApp::AppDllUid()const
    {
    return KUidVideoEditor;
    }


// ---------------------------------------------------------
// CVeiApp::CreateDocumentL()
// Creates CVeiDocument object
// ---------------------------------------------------------
//
CApaDocument* CVeiApp::CreateDocumentL()
    {
    LOG_RESET( KVideoEditorLogFile );
    return CVeiDocument::NewL( *this );
    }

// --------------------------------------------------------- 
// CVeiApp::OpenIniFileLC( RFs& aFs ) 
// Enables INI file creation 
// Returns: 
// --------------------------------------------------------- 
// 
CDictionaryStore* CVeiApp::OpenIniFileLC( RFs& aFs )const
    {
    //Opens the application’s ini file if it exists. If an ini
    //file does not exist for this application, or if it is corrupt,
    //this function creates a new ini file and opens that.
    //ini files are located on KIniFileDrive (by default, c:),
    //in the same directory as the application DLL.

    return CEikApplication::OpenIniFileLC( aFs );
    }
// End of File
