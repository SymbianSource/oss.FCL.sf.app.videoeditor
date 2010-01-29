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
* API class for avc editing module.
*
*/



// INCLUDE FILES
#include "vedavcedit.h"
#include "vedavceditimp.h"



// ============================ MEMBER FUNCTIONS ===============================

// -----------------------------------------------------------------------------
// CTRTranscoder::NewL
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
EXPORT_C CVedAVCEdit* CVedAVCEdit::NewL()
    {
    CVedAVCEditImp* self = CVedAVCEditImp::NewL();
    return self;
    }


// ---------------------------------------------------------
// CTRTranscoder::~CTRTranscoder()
// Destructor
// ---------------------------------------------------------
//
CVedAVCEdit::~CVedAVCEdit()
    {
    }



#ifndef EKA2

// -----------------------------------------------------------------------------
// E32Dll DLL Entry point
// -----------------------------------------------------------------------------
//
GLDEF_C TInt E32Dll(TDllReason /*aReason*/)
    {
    return(KErrNone);
    }

#endif


// End of file
