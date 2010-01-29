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


/*
* ============================================================================
*  Name     : DummyControl.cpp
*  Part of  : Video Editor
*  Interface : 
*  Description:
*     Class implementation.
* ============================================================================
*/

#include "DummyControl.h"
#include <SimpleVideoEditor.rsg>

void CDummyControl::ConstructL ()
	{
	MakeVisible(EFalse);
	this->ExecuteLD(R_DUMMY_DIALOG);
	}

CDummyControl::~CDummyControl()	
	{
	}

TKeyResponse CDummyControl::OfferKeyEventL (const TKeyEvent& /*aKeyEvent*/, TEventCode /*aType*/)
	{
	return EKeyWasConsumed;
	}

TBool CDummyControl::OkToExitL (TInt /*aButtonId*/)
	{
	return EFalse;
	}

// End of File
