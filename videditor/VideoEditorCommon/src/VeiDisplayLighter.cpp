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

// User includes
#include "VeiDisplayLighter.h"

EXPORT_C CVeiDisplayLighter* CVeiDisplayLighter::NewL()
	{
	CVeiDisplayLighter* self = new (ELeave) CVeiDisplayLighter();
	CleanupStack::PushL( self );
	self->ConstructL();
	CleanupStack::Pop( self );
	return self;
	}

CVeiDisplayLighter::CVeiDisplayLighter(): CActive( CActive::EPriorityLow )
	{
	CActiveScheduler::Add( this );
	}

CVeiDisplayLighter::~CVeiDisplayLighter()
	{
	Cancel();
	iTimer.Close();
	}

void CVeiDisplayLighter::ConstructL()
	{
	iTimeout = 10;
	iTimer.CreateLocal();
	}

EXPORT_C void CVeiDisplayLighter::Reset()
	{
	Cancel();
	Start();
	}

EXPORT_C void CVeiDisplayLighter::Stop()
	{
	Cancel();
	}


void CVeiDisplayLighter::DoCancel()
	{
	iTimer.Cancel();
	}

EXPORT_C void CVeiDisplayLighter::Start()
	{
	if ( !IsActive() )
		{
		iTimer.Inactivity( iStatus, iTimeout );
		SetActive();
		}
	}

void CVeiDisplayLighter::RunL()
	{
	if ( iStatus == KErrNone )
		{
		TInt inactivity = User::InactivityTime().Int();
		if ( inactivity >= iTimeout )
			{
			User::ResetInactivityTime();
			iTimer.Inactivity( iStatus, iTimeout );
			SetActive();
			}
		}
	}
// End of File
