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


#include "aacdef.h"

CWindowInfo* CWindowInfo::NewL()
    {

    CWindowInfo* self = new (ELeave) CWindowInfo();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;

    }
CWindowInfo::~CWindowInfo()
    {
    if (group != 0) delete[] group;
    if (mask != 0) delete[] mask;
    if (sfac != 0) delete[] sfac;
    if (cb_map != 0) delete[] cb_map;
    if (lpflag != 0) delete[] lpflag;

    }

CWindowInfo::CWindowInfo()
    {

    }

void CWindowInfo::ConstructL()
    {

    group = new (ELeave) uint8[NSHORT];
    mask = new (ELeave) uint8[MAXBANDS];
    sfac = new (ELeave) int16[MAXBANDS];
    cb_map = new (ELeave) uint8[MAXBANDS];
    lpflag= new (ELeave) int16[MAXBANDS];

    }

CMC_Info* CMC_Info::NewL()
    {

    CMC_Info* self = new (ELeave) CMC_Info();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;

    }
CMC_Info::~CMC_Info()
    {
    if (sfbInfo != 0) delete sfbInfo;
    }

CMC_Info::CMC_Info()
    {

    }

void CMC_Info::ConstructL()
    {
    sfbInfo = CSfb_Info::NewL();

    }
    
