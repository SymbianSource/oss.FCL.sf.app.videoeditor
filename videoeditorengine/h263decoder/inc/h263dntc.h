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
* Header for control interface for threadless video decoder.
*
*/



#ifndef _H263DNTC_H_
#define _H263DNTC_H_


/* 
 * Includes
 */

#include "h263dext.h"
#include "h263dmai.h"

/*
 * Function prototypes
 */

H263D_DS void H263D_CC h263dClose(h263dHInstance_t hInstance);

H263D_DS int H263D_CC h263dDecodeFrameL(
   h263dHInstance_t hInstance,
   void *pStreamBuffer,
   void *pOutStreamBuffer,
   unsigned streamBufferSize,
   u_char *fFirstFrame,
   TInt *frameSize,
   TInt *outframeSize,   
   vdeDecodeParamters_t *aDecoderInfo);

int H263D_CC h263dGetLatestFrame(
   h263dHInstance_t hInstance,
   u_char **ppy, u_char **ppu, u_char **ppv,
   int *pLumWidth, int *pLumHeight,
   int *pFrameNum);

H263D_DS int H263D_CC h263dIsPB(
   h263dHInstance_t hInstance,
   void *frameStart,
   unsigned frameLength);

H263D_DS int H263D_CC h263dIsIntra(
   h263dHInstance_t hInstance,
   void *frameStart,
   unsigned frameLength);

H263D_DS h263dHInstance_t H263D_CC h263dOpen(h263dOpen_t *openParam);

int GetNewTrValue(int aTrCurOrig, int* aTrPrevNew, int* aTrPrevOrig, int aSMSpeed);

#endif
// End of File
