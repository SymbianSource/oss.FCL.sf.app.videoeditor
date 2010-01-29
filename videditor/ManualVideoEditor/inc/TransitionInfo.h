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


#ifndef TRANSITIONINFO_H
#define TRANSITIONINFO_H

#include <e32std.h>
#include <VedVideoClipInfo.h>


/**
 * Transition info.	
 */
class CTransitionInfo: public CBase
{
public:
    /* Constructors & Destructor. */
    static CTransitionInfo* NewL();
    static CTransitionInfo* NewLC();
    virtual ~CTransitionInfo();

    /* 
     * Methods for querying transition icons and names. The icons and
     * names are NOT to be deleted by the caller.
     */
    HBufC* StartTransitionName( TVedStartTransitionEffect aEffect );
    HBufC* MiddleTransitionName( TVedMiddleTransitionEffect aEffect );
    HBufC* EndTransitionName( TVedEndTransitionEffect aEffect );

private:
    /* Private constructors. */
    void ConstructL();
    CTransitionInfo();

private:
    /* Data. */
    RPointerArray < HBufC > iStartTransitionNameArray;
    RPointerArray < HBufC > iMiddleTransitionNameArray;
    RPointerArray < HBufC > iEndTransitionNameArray;
};

#endif 

// End of File
