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


#include "Tool.h"

CLTP_Info* CLTP_Info::NewL()
    {
    
    CLTP_Info* self = new (ELeave) CLTP_Info();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;    

    }


CLTP_Info::~CLTP_Info()
    {
    if (delay != 0)
        delete[] delay;
    }

CLTP_Info::CLTP_Info()
    {

    }

void CLTP_Info::ConstructL()
    {
    delay = new (ELeave) int16[NSHORT];

    }



CToolInfo* CToolInfo::NewL()
    {

    CToolInfo* self = new (ELeave) CToolInfo();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;

    }

void CToolInfo::ConstructL()
    {
    quant = new (ELeave) int16[LN2];
    ltp = CLTP_Info::NewL();

    }

CToolInfo::CToolInfo()
    {

    }

CToolInfo::~CToolInfo()
    {

    if (quant != 0)
        {
        delete [] quant;
        }

    if (ltp != 0)
        delete ltp;

    }


CCInfo* CCInfo::NewL()
    {
    CCInfo* self = new (ELeave) CCInfo();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;


    }

CCInfo::CCInfo()
    {

    }

CCInfo::~CCInfo()
    {
    if (tool != 0) delete tool;
    if (winInfo != 0) delete winInfo;
    }

void CCInfo::ConstructL()
    {

    tool = CToolInfo::NewL();
    winInfo = CWindowInfo::NewL();
    }



