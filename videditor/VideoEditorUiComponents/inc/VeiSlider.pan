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


#ifndef __VEISLIDER_PAN__
#define __VEISLIDER_PAN__

enum TVeiSliderPanic
    {
    EVeiSliderPanicMinMax = -0x100,
    EVeiSliderPanicBitmapsNotLoaded,
    EVeiSliderPanicStepNotPositive,
    EVeiSliderPanicIndexUnderflow,
    EVeiSliderPanicIndexOverflow,
    EVeiSliderPanicOther
    };

void Panic(TInt aCategory)
    {
    _LIT(KComponentName, "VEISLIDER");
    User::Panic(KComponentName, aCategory);
    }

#endif __VEISLIDER_PAN__
