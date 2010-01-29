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
* Declares VeiCutAudioBar control for the video editor application.
* 
*/



#ifndef VEIDISPLAYLIGHTER_H
#define VEIDISPLAYLIGHTER_H

#include <e32base.h>
/**
 * CVeiDisplayLighter control class.
 */
NONSHARABLE_CLASS( CVeiDisplayLighter ) : public CActive
	{
	public:
		IMPORT_C static CVeiDisplayLighter* NewL();
		IMPORT_C void Start();
		IMPORT_C void Reset();
		IMPORT_C void Stop();
		virtual ~CVeiDisplayLighter();
	protected:
		void DoCancel();
		void RunL();

		CVeiDisplayLighter();
		void ConstructL();

	private:
		RTimer iTimer;
		TInt   iTimeout;
	
	};
#endif
