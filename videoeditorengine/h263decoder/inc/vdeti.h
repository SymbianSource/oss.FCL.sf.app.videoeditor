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
* Header file for the vdeti.c file.
*
*/


#ifndef _VDETI_H_
#define _VDETI_H_



#include "vdemain.h"

/*
 * Structs and typedefs
 */


/* 
 * Function prototypes
 */

class CMPEG4Transcoder;
int vdeDetermineStreamType(vdeHInstance_t hInstance, CMPEG4Transcoder *hTranscoder);

int vdeDecodeFrame(vdeHInstance_t hInstance, int aStartByteIndex, int aStartBitIndex,CMPEG4Transcoder *hTranscoder);

#endif
// End of File
