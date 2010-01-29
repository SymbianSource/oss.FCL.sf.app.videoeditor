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
*  Name     : DummyControl.h
*  Part of  : Video Editor
*  Description:
*     Declares dummy control class
* ============================================================================
*/
  
#ifndef _DUMMYCONTROL_H_
#define _DUMMYCONTROL_H_

#include <coecntrl.h>
#include <akndialog.h>

NONSHARABLE_CLASS( CDummyControl ) : public CAknDialog 
	{
public:
	
	void ConstructL ();
	virtual ~CDummyControl();	
	TKeyResponse OfferKeyEventL (const TKeyEvent& /*aKeyEvent*/, TEventCode /*aType*/);
	TBool OkToExitL (TInt aButtonId);

	};

#endif _DUMMYCONTROL_H_
