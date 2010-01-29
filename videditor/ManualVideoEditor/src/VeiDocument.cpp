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
#include "VeiDocument.h"
#include "VeiAppUi.h"
#include "videoeditorcommon.h"
#include <aiwgenericparam.h>

// ================= MEMBER FUNCTIONS =======================

// constructor
CVeiDocument::CVeiDocument(CEikApplication& aApp)
: CAiwGenericParamConsumer(aApp)    
    {
    }

// destructor
CVeiDocument::~CVeiDocument()
    {
    }

// Default constructor can leave.
void CVeiDocument::ConstructL()
    {
    }

// Two-phased constructor.
CVeiDocument* CVeiDocument::NewL(
        CEikApplication& aApp)     // CVeiApp reference
    {
    CVeiDocument* self = new (ELeave) CVeiDocument( aApp );
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop( self );

    return self;
    }

// ----------------------------------------------------
// CVeiDocument::OpenFileL
// 
// ----------------------------------------------------
//
void CVeiDocument::OpenFileL(CFileStore*& /*aFileStore*/, RFile& aFile)
    {
    LOG(KVideoEditorLogFile, "CVeiDocument::OpenFileL: In");

    // File handle is not used, but close it because Open File Service
    // duplicates the handle and may not close it.
    aFile.Close();

    // Get the input files
    CCoeEnv* coeEnv = CCoeEnv::Static();
    const CAiwGenericParamList* inParamList = NULL;
    inParamList = GetInputParameters();
    const TAiwGenericParam* param = NULL;
    TInt index(0);
    if (inParamList && inParamList->Count() > 0)
        {
        LOGFMT(KVideoEditorLogFile, "\tAIW parameter count: %d", inParamList->Count());

        param = inParamList->FindFirst( index,EGenericParamFile );  

        while( index != KErrNotFound )  
            {
            TFileName filename = param->Value().AsDes();
            LOGFMT(KVideoEditorLogFile, "\tInserting video clip to movie: %S", &filename);
            STATIC_CAST( CVeiAppUi*, coeEnv->AppUi() )->InsertVideoClipToMovieL( 
                EFalse, filename );

            param = inParamList->FindNext( index, EGenericParamFile );
            }
        }

    TFileName filee;
    filee.Append( _L("eeee"));
    STATIC_CAST( CVeiAppUi*, coeEnv->AppUi() )->InsertVideoClipToMovieL( 
            ETrue, filee);

    LOG(KVideoEditorLogFile, "CVeiDocument::OpenFileL: Out");
    }

// ----------------------------------------------------
// CVeiDocument::CreateAppUiL()
// constructs CVeiAppUi
// ----------------------------------------------------
//
CEikAppUi* CVeiDocument::CreateAppUiL()
    {
    return new (ELeave) CVeiAppUi;
    }

// End of File  
