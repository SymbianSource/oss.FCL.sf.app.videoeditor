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

  
#ifndef _SIMPLEVIDEOEDITOREXITOBSERVER_H_
#define _SIMPLEVIDEOEDITOREXITOBSERVER_H_

//<IBUDSW>
#include <e32base.h>

// CLASS DECLARATION
class MSimpleVideoEditorExitObserver
	{
	public:
		/**
		*   @param aReason - Error code.
		*   @param aResultFileName - 
		*			The name of the created file.
		*			In case of failure contains empty descriptor.
		*/
		virtual void HandleSimpleVideoEditorExit (TInt aReason, const TDesC& aResultFileName) = 0;
	};

//</IBUDSW>
#endif

// End of file
