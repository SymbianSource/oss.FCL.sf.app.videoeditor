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
* Header file for external usage of the services provided by 
* the H.263 Video Decoder Engine.
*
*/


#ifndef _VDEMAIN_H_
#define _VDEMAIN_H_




/*
 * Includes
 */

#include "biblin.h"

#include "epoclib.h"
#include "vdefrt.h"


/*
 * Structs and typedefs
 */

/* {{-output"vdeHInstance_t_info.txt" -ignore"*" -noCR}}
   vdeHInstance_t is used as a unique identifier of a Video Decoder
   Engine instance.
   The type can be casted to u_int32 or void pointer and then back to 
   vdeHInstance_t without any problems.
   {{-output"vdeHInstance_t_info.txt"}} */

/* {{-output"vdeHInstance_t.txt"}} */
typedef void * vdeHInstance_t;
/* {{-output"vdeHInstance_t.txt"}} */



/*
 * Function prototypes
 */

int vdeFree(void);

int vdeGetLatestFrame(
   vdeHInstance_t hInstance,
   u_char **ppy, u_char **ppu, u_char **ppv,
   int *pLumWidth, int *pLumHeight,
   int *pFrameNum);

vdeHInstance_t vdeInit(h263dOpen_t *param);

int vdeIsINTRA(
   vdeHInstance_t hInstance,
   void *frameStart,
   unsigned frameLength);

int vdeLoad(const char *rendererFileName);

int vdeReturnImageBuffer(vdeHInstance_t hInstance, u_int32 typelessParam);

int vdeSetInputBuffer(vdeHInstance_t hInstance, bibBuffer_t *buffer);


int vdeSetOutputBuffer(vdeHInstance_t hInstance, bibBuffer_t *buffer);

int vdeSetBufferEdit(vdeHInstance_t hInstance, bibBufferEdit_t *bufEdit);

int vdeSetVideoEditParams(vdeHInstance_t hInstance, int aColorEffect, 
  TBool aGetDecodedFrame, TInt aColorToneU, TInt aColorToneV);

int vdeShutDown(vdeHInstance_t hInstance);

#endif
// End of File
