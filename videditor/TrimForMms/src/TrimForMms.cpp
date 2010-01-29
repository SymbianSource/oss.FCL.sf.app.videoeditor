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
 * File: TrimForMms.cpp
 * Created: 04/18/06
 * Author: 
 *
 * ==================================================================== */

#include "TrimForMmsApplication.h"
#include <eikstart.h>

// Create an application, and return a pointer to it
CApaApplication* NewApplication()
	{
	return new CTrimForMmsApplication;
	}


TInt E32Main()
	{
	return EikStart::RunApplication(NewApplication);
	
	}

