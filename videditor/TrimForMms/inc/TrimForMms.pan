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
 * File: TrimForMms.pan
 * Created: 04/18/06
 * Author: 
 * 
 * ==================================================================== */

#ifndef __TRIMFORMMS_PAN__
#define __TRIMFORMMS_PAN__

/** TrimForMms application panic codes */
enum TTrimForMmsPanics 
    {
    ETrimForMmsBasicUi = 1
    // add further panics here
    };

inline void Panic(TTrimForMmsPanics aReason)
    {
	_LIT(applicationName,"TrimForMms");
    User::Panic(applicationName, aReason);
    }

#endif // __TRIMFORMMS_PAN__
