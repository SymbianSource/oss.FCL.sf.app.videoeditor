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
#include "VeiSimpleCutVideoDocument.h"
#include "VeiSimpleCutVideoAppUi.h"

// ================= MEMBER FUNCTIONS =======================

// constructor
CVeiSimpleCutVideoDocument::CVeiSimpleCutVideoDocument(CEikApplication& aApp)
: CAknDocument(aApp)    
    {
    }

// destructor
CVeiSimpleCutVideoDocument::~CVeiSimpleCutVideoDocument()
    {
    LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoDocument::~CVeiSimpleCutVideoDocument(): In");    
    LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoDocument::~CVeiSimpleCutVideoDocument(): Out");
    }

// Default constructor can leave.
void CVeiSimpleCutVideoDocument::ConstructL()
    {
    }

// Two-phased constructor.
CVeiSimpleCutVideoDocument* CVeiSimpleCutVideoDocument::NewL(
        CEikApplication& aApp)     // CVeiApp reference
    {
    LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoDocument::NewL: In ");
    
    CVeiSimpleCutVideoDocument* self = new (ELeave) CVeiSimpleCutVideoDocument( aApp );
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop( self );
    
    LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoDocument::NewL: Out");
    return self;
    }

void CVeiSimpleCutVideoDocument::OpenFileL(CFileStore*& /*aFileStore*/, RFile& aFile)
 	{
 	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoDocument::OpenFileL: In");

	CleanupClosePushL( aFile );

    // File handle must be closed. Open File Service won't do it. 
    // Using cleanup stack, because iAppUi->OpenFileL() may leave.
#ifdef DEBUG_ON
	TFileName file;
	User::LeaveIfError( aFile.FullName(file) );
 	LOGFMT(KVideoEditorLogFile, "CVeiSimpleCutVideoDocument::OpenFileL: %S", &file);
#endif
 
 	static_cast<CVeiSimpleCutVideoAppUi*>(CCoeEnv::Static()->AppUi())->CutVideoL(ETrue, aFile);
	
	CleanupStack::PopAndDestroy( &aFile ); 
	
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoDocument::OpenFileL: Out");
 	}   

// ----------------------------------------------------
// CVeiSimpleCutVideoDocument::CreateAppUiL()
// constructs CVeiSimpleCutVideoAppUi
// ----------------------------------------------------
//
CEikAppUi* CVeiSimpleCutVideoDocument::CreateAppUiL()
    {
    return new (ELeave) CVeiSimpleCutVideoAppUi;
    }

// End of File  
