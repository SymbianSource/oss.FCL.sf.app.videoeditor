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


/* Copyright (C) 2004 Nokia Corporation. */

#include "AudPanic.h"


void TAudPanic::Panic(TCode aCode)
    {
    User::Panic(_L("Audio Editor Eng"), aCode);
    }

