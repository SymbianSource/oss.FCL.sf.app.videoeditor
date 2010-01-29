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
 * File: TrimForMmsApplication.cpp
 * Created: 04/18/06
 * Author: 
 * 
 * ==================================================================== */

#include "TrimForMmsDocument.h"
#include "TrimForMmsApplication.h"
#include "VideoEditorCommon.h"  // Application UID

CApaDocument* CTrimForMmsApplication::CreateDocumentL()
    {  
    // Create an TrimForMms document, and return a pointer to it
    CApaDocument* document = CTrimForMmsDocument::NewL(*this);
    return document;
    }

TUid CTrimForMmsApplication::AppDllUid() const
    {
    // Return the UID for the TrimForMms application
    return KUidTrimForMms;
    }

