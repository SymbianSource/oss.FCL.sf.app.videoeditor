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


/* ====================================================================
 * File: TrimForMmsDocument.cpp
 * Created: 04/18/06
 * Author: 
 * 
 * ==================================================================== */

#include "TrimForMmsAppUi.h"
#include "TrimForMmsDocument.h"
#include <utf.h>


// Standard Symbian OS construction sequence
CTrimForMmsDocument* CTrimForMmsDocument::NewL(CEikApplication& aApp)
    {
    CTrimForMmsDocument* self = NewLC(aApp);
    CleanupStack::Pop(self);
    return self;
    }

CTrimForMmsDocument* CTrimForMmsDocument::NewLC(CEikApplication& aApp)
    {
    CTrimForMmsDocument* self = new (ELeave) CTrimForMmsDocument(aApp);
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
    }

void CTrimForMmsDocument::ConstructL()
    {
	// no implementation required
    }    

CTrimForMmsDocument::CTrimForMmsDocument(CEikApplication& aApp) : CAknDocument(aApp) 
    {
	// no implementation required
    }

CTrimForMmsDocument::~CTrimForMmsDocument()
    {
	// no implementation required
    }

CEikAppUi* CTrimForMmsDocument::CreateAppUiL()
    {
    // Create the application user interface, and return a pointer to it,
    // the framework takes ownership of this object
    CEikAppUi* appUi = new (ELeave) CTrimForMmsAppUi;
    return appUi;
    }

void CTrimForMmsDocument::OpenFileL(CFileStore*& /*aFileStore*/, RFile& aFile)
 	{
 	TFileName file;
 	User::LeaveIfError(aFile.FullName(file)); 	
 	
 	TBuf8<KMaxFileName> conv8Filename;
					CnvUtfConverter::ConvertFromUnicodeToUtf8( conv8Filename,file );
 	
 	STATIC_CAST( CTrimForMmsAppUi*, CCoeEnv::Static()->AppUi() )
 	    ->ActivateLocalViewL(TUid::Uid(0), TUid::Uid(0), conv8Filename);
 	
 	aFile.Close();
 	
 	}
