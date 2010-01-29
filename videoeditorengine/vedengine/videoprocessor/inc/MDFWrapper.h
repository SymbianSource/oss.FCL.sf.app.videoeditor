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
* Definition file for MSL / MDF data types.
*
*/


#ifndef MDFWRAPPER_H
#define MDFWRAPPER_H


// INCLUDES
// Include Symbian MDF header files
#include <devvideorecord.h>
#include <DevVideoConstants.h>

// MACROS
// Redefine Symbian MDF types
typedef CVideoEncoderInfo CDEVVRVideoEncoderInfo;
typedef CMMFDevVideoRecord CDEVVRVideoRecord;
typedef MMMFDevVideoRecordObserver MDEVVRVideoRecordObserver;

// Cast operation
#define CAST_OPTION(x) *(x)

// CONSTANTS
// Exact match specifier for searching available plugins
const TBool KCMRMatch = EFalse;






#endif
