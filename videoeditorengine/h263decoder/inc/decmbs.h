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
* Header for multiple macroblock decoding module.
*
*/



#ifndef _DECMBS_H_
#define _DECMBS_H_

/*
 * Includes
 */

#include "decmb.h"


/*
 * Defines
 */

// unify error codes
#define DMBS_OK H263D_OK
#define DMBS_ERR H263D_ERROR


/*
 * Function prototypes
 */

int dmbsGetAndDecodeIMBsInScanOrder(
   const dmbIFrameMBInParam_t *inParam,
   dmbIFrameMBInOutParam_t *inOutParam,
   int *quant, CMPEG4Transcoder *hTranscoder);

int dmbsGetAndDecodePMBsInScanOrder(
   const dmbPFrameMBInParam_t *inParam,
   dmbPFrameMBInOutParam_t *inOutParam,
   int *quant, CMPEG4Transcoder *hTranscoder);

int dmbsGetAndDecodeIMBsDataPartitioned(
   dmbIFrameMBInParam_t *inParam,
   dmbIFrameMBInOutParam_t *inOutParam,
   int *quantParams, CMPEG4Transcoder *hTranscoder);

int dmbsGetAndDecodePMBsDataPartitioned(
   const dmbPFrameMBInParam_t *inParam,
   dmbPFrameMBInOutParam_t *inOutParam,
   int *quantParams, CMPEG4Transcoder *hTranscoder);
   
#endif
// End of File
