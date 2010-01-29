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


#ifndef _MOTION_H_
#define _MOTION_H_


#include "nrctyp32.h"
#include "framebuffer.h"


#define MOT_NUM_MODES     7
#define MOT_COPY          0
#define MOT_16x16         1
#define MOT_16x8          2   
#define MOT_8x16          3
#define MOT_8x8           4
#define MOT_8x4           5   
#define MOT_4x8           6
#define MOT_4x4           7


int mcpGetNumMotVecs(int interMode, int subMbTypes[4]);


#endif
