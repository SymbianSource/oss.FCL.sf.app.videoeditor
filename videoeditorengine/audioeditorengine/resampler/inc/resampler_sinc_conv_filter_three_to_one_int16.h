#ifndef __RESAMPLER_SINC_CONV_FILTER_THREE_TO_ONE_INT16_H__
#define __RESAMPLER_SINC_CONV_FILTER_THREE_TO_ONE_INT16_H__
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
RESAMPLER_SincConvFilterThreeToOneInt16(const int16 *signalBuf, 
                                  const int16 *filterPtr,
                                  int length)
{
    int32 newSample((int32)(*filterPtr++) * signalBuf[0]);
    
    const int16 *bufferPtr1 = signalBuf - 1; 
    const int16 *bufferPtr2 = signalBuf + 1;
    for (int l = length - 1; l > 0 ; l--)
    {
        // Calculate the filter. 
        // Keep newSample in s1.30 format to reduce rounding errors
        int16 coeff = *filterPtr++;
        newSample += (int32)coeff * (*bufferPtr1) + (int32)coeff * (*bufferPtr2);
        bufferPtr1--;
        bufferPtr2++;
	}

    return newSample;
}

#endif /* __RESAMPLER_SINC_CONV_FILTER_THREE_TO_ONE_INT16_H__ */
