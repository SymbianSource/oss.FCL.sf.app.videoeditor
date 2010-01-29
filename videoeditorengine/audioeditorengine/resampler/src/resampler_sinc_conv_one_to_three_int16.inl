#ifndef __RESAMPLER_SINC_CONV_ONE_TO_THREE_INT16_INL__
#define __RESAMPLER_SINC_CONV_ONE_TO_THREE_INT16_INL__
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


#include "resampler_common_defs.h"
#include "resampler_data_types.h"

static FORCEINLINE int32 
RESAMPLER_SincConvOneToThreeFilterInt16(const int16 *signalBuf, 
                                  const int16 *filterPtr1,
                                  const int16 *filterPtr2,
                                  int length)
{
    int32 newSample = 0;
    const int16 *bufferPtr1 = signalBuf;
    const int16 *bufferPtr2 = signalBuf - 1;

    (void)length;
    for (int l = length; l > 0; l-=4)
    {
        newSample += (int32)(*filterPtr1++) * (*bufferPtr1++) + (int32)(*filterPtr2++) * (*bufferPtr2--);
        newSample += (int32)(*filterPtr1++) * (*bufferPtr1++) + (int32)(*filterPtr2++) * (*bufferPtr2--);
        newSample += (int32)(*filterPtr1++) * (*bufferPtr1++) + (int32)(*filterPtr2++) * (*bufferPtr2--);
        newSample += (int32)(*filterPtr1++) * (*bufferPtr1++) + (int32)(*filterPtr2++) * (*bufferPtr2--);
    }

    return newSample;
}


#endif /* __RESAMPLER_SINC_CONV_ONE_TO_THREE_INT16_INL__ */
