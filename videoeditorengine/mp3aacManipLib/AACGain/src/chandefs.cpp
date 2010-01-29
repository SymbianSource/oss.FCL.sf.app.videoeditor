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




#include "chandefs.h"

CInfo* CInfo::NewL()
    {

    CInfo* self = new (ELeave) CInfo();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;

    }

void CInfo::ConstructL()
    {

    bins_per_sbk = new (ELeave) int16[MAX_SBK];
    sfb_per_sbk = new (ELeave) int16[MAX_SBK];
    bk_sfb_top = new (ELeave) int16[200];
    group_len = new (ELeave) int16[NSHORT];
    group_offs = new (ELeave) int16[NSHORT];

    }

CInfo::CInfo()
    {

    }

CInfo::~CInfo()
    {

    if (bins_per_sbk != 0) delete[] bins_per_sbk;
    if (sfb_per_sbk != 0) delete[] sfb_per_sbk;
    if (bk_sfb_top != 0) delete[] bk_sfb_top;
    if (group_len != 0) delete[] group_len;
    if (group_offs != 0) delete[] group_offs;

    }


CSfb_Info* CSfb_Info::NewL(uint8 isEncoder)
    {

    CSfb_Info* self = new (ELeave) CSfb_Info();
    CleanupStack::PushL(self);
    self->ConstructL(isEncoder);
    CleanupStack::Pop(self);
    return self;

    }

void CSfb_Info::ConstructL(uint8 isEncoder)
    {

    only_long_info = CInfo::NewL();
    eight_short_info = CInfo::NewL();

    if(isEncoder)
        {
        int16 i;

        /*-- Allocate SFB data. --*/
        sect_sfb_offsetL = (int16 *) new (ELeave) int16[MAXLONGSFBBANDS];

        sect_sfb_offsetS = (int16 *) new (ELeave) int16[MAXSHORTSFBBANDS];

        for(i = 0; i < NSHORT; i++)
            {
            sect_sfb_offsetS2[i] = (int16 *) new (ELeave) int16[MAXSHORTSFBBANDS];    
            }
      
          }

    }

CSfb_Info::CSfb_Info()
    {
    int16 i;

    only_long_info = 0;
    eight_short_info = 0;
    
    /*-- Scalefactor offsets. --*/
    sect_sfb_offsetL = 0;
    sect_sfb_offsetS = 0;
    sfbOffsetTablePtr[0] = sfbOffsetTablePtr[1] = 0;
  
    for(i = 0; i < NSHORT; i++)
        {
        sect_sfb_offsetS2[i] = 0;
        }
        

    }

CSfb_Info::~CSfb_Info()

    {
      int16 i;

    if (only_long_info != 0) 
        {
        delete only_long_info;
          only_long_info = 0;
        
        }
    
    if (eight_short_info != 0) 
        {
        delete eight_short_info;
        eight_short_info = 0;
        
        }
    
    if(sect_sfb_offsetL != 0)
        {
        delete[] sect_sfb_offsetL;
          sect_sfb_offsetL = 0;
        
        }
    
    if(sect_sfb_offsetS != 0)
        {
        delete[] sect_sfb_offsetS;
        sect_sfb_offsetS = 0;
        
        }
    
    for(i = 0; i < NSHORT; i++)
        {
        if(sect_sfb_offsetS2[i] != 0)
            {
            delete[] sect_sfb_offsetS2[i];
            sect_sfb_offsetS2[i] = 0;

            }
          }

    }
