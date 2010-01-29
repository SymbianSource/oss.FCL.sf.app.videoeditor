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




#ifndef __AUDPANIC_H__
#define __AUDPANIC_H__

#include <e32base.h>


class TAudPanic 
    {
public:
    enum TCode
        {
        EInternal = 0,  // Internal error (that is, a  in the engine)
        ESongEmpty,
        ESongAddOperationAlreadyRunning,
        ESongProcessingOperationAlreadyRunning,
        ESongProcessingOperationNotRunning,
        EVisualizationProcessAlreadyRunning,
        EVisualizationProcessNotRunning,
        EClipInfoProcessAlreadyRunning,
        EAudioClipIllegalIndex,
        EAudioClipIllegalCutInTime,
        EAudioClipIllegalCutOutTime,
        EIllegalDynamicLevelMark,
        EIllegalDynamicLevelMarkIndex,
        EAudioClipIllegalStartTime,
        ESongObserverAlreadyRegistered,
        ESongObserverNotRegistered


        };

public:
    static void Panic(TCode aCode);
    };

#endif // __AUDPANIC_H__

