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
* Class implementation.
*
*/
/*
* ============================================================================
*  Name     : CMultiLineQueryDialog.cpp
*  Part of  : Video Editor
*  Interface : 
*  Description:
*     Class implementation.
*  Version  :
* ============================================================================
*/

// INCLUDES
#include "CMultiLineQueryDialog.h"
#include "VideoEditorCommon.h"

// CONSTANTS

_LIT( KNewLine, "\n" );

// MEMBER FUNCTIONS

// -----------------------------------------------------------------------------
// CMultiLineQueryDialog::CMultiLineQueryDialog
// C++ default constructor can NOT contain any code, that
// might leave.
// -----------------------------------------------------------------------------
//
EXPORT_C CMultiLineQueryDialog::CMultiLineQueryDialog( TDes& aDataText, 
                                        const TTone& aTone /*= ENoTone*/ )
    : CAknTextQueryDialog( aDataText, aTone )
    {
    }

// -----------------------------------------------------------------------------
// CMultiLineQueryDialog::NewL
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
EXPORT_C CMultiLineQueryDialog* CMultiLineQueryDialog::NewL( TDes& aDataText, 
                                              const TTone& aTone /*= ENoTone*/ )
    {
    CMultiLineQueryDialog* self = new( ELeave ) CMultiLineQueryDialog( aDataText, aTone );
    return self;
    }


// -----------------------------------------------------------------------------
//  Destructor
// -----------------------------------------------------------------------------
//    

EXPORT_C CMultiLineQueryDialog::~CMultiLineQueryDialog()
    {    
    }

// -----------------------------------------------------------------------------
// CMultiLineQueryDialog::OfferKeyEventL
// -----------------------------------------------------------------------------
//
EXPORT_C TKeyResponse CMultiLineQueryDialog::OfferKeyEventL( const TKeyEvent& aKeyEvent, 
                                         TEventCode aType )
    { 
    TKeyResponse response;
	if ( aType == EEventKey && aKeyEvent.iCode == EKeyEnter )
		{
		CAknQueryControl* control = static_cast<CAknQueryControl*>( ControlOrNull( EGeneralQuery ) );
		CEikEdwin* edWin = static_cast<CEikEdwin*>( control->ControlByLayoutOrNull( EDataLayout ) );
		if (edWin)
	    	{
	    	edWin->GetText( iDataText );
	    	iDataText.Append( KNewLine );
	    	edWin->SetTextL( &iDataText );
	    	edWin->ClearSelectionL();
	    	}
        response = EKeyWasConsumed;
		}
	else
		{
		response = CAknTextQueryDialog::OfferKeyEventL( aKeyEvent, aType );
		}
	
	return response;
    }
	
