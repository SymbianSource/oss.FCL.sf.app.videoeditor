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
#include <remconcoreapitarget.h>
#include <remconinterfaceselector.h>
#include <aknconsts.h>					

#include "VeiRemConTarget.h"
#include "VideoEditorDebugUtils.h"

// ================= MEMBER FUNCTIONS =======================

// C++ default constructor can NOT contain any code, that
// might leave.
//
CVeiRemConTarget::CVeiRemConTarget(MVeiMediakeyObserver& aObserver) : iObserver(aObserver)
	{
	}

// Default constructor can leave.
void CVeiRemConTarget::ConstructL()
	{
	// Create interface selector.
	iInterfaceSelector = CRemConInterfaceSelector::NewL();
	// Create a new CRemConCoreApiTarget, owned by the interface selector.
	iCoreTarget = CRemConCoreApiTarget::NewL(*iInterfaceSelector, *this);
	// Start being a target.
	iInterfaceSelector->OpenTargetL();
	}

// Two-phased constructor.
EXPORT_C CVeiRemConTarget* CVeiRemConTarget::NewL(MVeiMediakeyObserver& aObserver)
	{
	LOG(KVideoEditorLogFile, "CVeiRemConTarget::NewL");

	CVeiRemConTarget* self = new (ELeave) CVeiRemConTarget(aObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop( self );
	return self;
	}

// Destructor
CVeiRemConTarget::~CVeiRemConTarget()
    {
    delete iInterfaceSelector;
    // iCoreTarget was owned by iInterfaceSelector.
    iCoreTarget = NULL;
    }

// ---------------------------------------------------------
// CVeiRemConTarget::MrccatoCommand
// ---------------------------------------------------------
//
void CVeiRemConTarget::MrccatoCommand(TRemConCoreApiOperationId aOperationId, TRemConCoreApiButtonAction DEBUGLOG_ARG(aButtonAct))
	{
	LOGFMT(KVideoEditorLogFile, "CVeiRemConTarget::MrccatoCommand: buttonact:%d >>", aButtonAct );
	switch (aOperationId)
		{
		case ERemConCoreApiVolumeUp:
			iObserver.HandleVolumeUpL();
			break;
		case ERemConCoreApiVolumeDown:
			iObserver.HandleVolumeDownL();
			break;

		default:
			break;
		}
	LOG(KVideoEditorLogFile, "CVeiRemConTarget::MrccatoCommand << " );
	}

// ---------------------------------------------------------
// CDmhRemConTarget::MrccatoPlay
// ---------------------------------------------------------
//
void CVeiRemConTarget::MrccatoPlay(TRemConCoreApiPlaybackSpeed /*aSpeed*/, TRemConCoreApiButtonAction /*aButtonAct*/)
	{
	LOG(KVideoEditorLogFile, "CVeiRemConTarget::MrccatoPlay <<");
	}
	
// ---------------------------------------------------------
// CDmhRemConTarget::MrccatoTuneFunction
// ---------------------------------------------------------
//
void CVeiRemConTarget::MrccatoTuneFunction(TBool /*aTwoPart*/, TUint /*aMajorChannel*/, TUint /*aMinorChannel*/, TRemConCoreApiButtonAction /*aButtonAct*/)
	{

	}
	
// ---------------------------------------------------------
// CDmhRemConTarget::MrccatoSelectDiskFunction
// ---------------------------------------------------------
//
void CVeiRemConTarget::MrccatoSelectDiskFunction(TUint /*aDisk*/, TRemConCoreApiButtonAction /*aButtonAct*/)
	{

	}
	
// ---------------------------------------------------------
// CVeiRemConTarget::MrccatoSelectAvInputFunction
// ---------------------------------------------------------
//
void CVeiRemConTarget::MrccatoSelectAvInputFunction(TUint8 /*aAvInputSignalNumber*/, TRemConCoreApiButtonAction /*aButtonAct*/)
	{

	}
	
// ---------------------------------------------------------
// CVeiRemConTarget::MrccatoSelectAudioInputFunction
// ---------------------------------------------------------
//
void CVeiRemConTarget::MrccatoSelectAudioInputFunction(TUint8 /*aAudioInputSignalNumber*/, TRemConCoreApiButtonAction /*aButtonAct*/)
	{

	}
	
//  End of File  
