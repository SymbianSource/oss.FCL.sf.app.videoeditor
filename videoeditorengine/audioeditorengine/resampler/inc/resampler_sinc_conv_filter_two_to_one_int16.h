#ifndef __RESAMPLER_SINC_CONV_FILTER_TWO_TO_ONE_INT16_H__
#define __RESAMPLER_SINC_CONV_FILTER_TWO_TO_ONE_INT16_H__
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
RESAMPLER_SincConvFilterTwoToOneInt16(const int16 *signalBuf, 
								const int16 *filterPtr,
								int length)
{
    int32 newSample((int32)signalBuf[0] << 14);
    
    const int16 *bufferPtr1 = signalBuf - 1; 
    const int16 *bufferPtr2 = signalBuf + 1;
    for (int l = length; l >0 ; l--)
    {
        // Calculate the filter. 
        // Keep newSample in s1.30 format to reduce rounding errors
        newSample += (int32)(*filterPtr) * (*bufferPtr1) + (int32)(*filterPtr) * (*bufferPtr2);
		filterPtr++;
		bufferPtr1 -= 2;
		bufferPtr2 += 2;
	}

    return newSample;
}


#endif /* __RESAMPLER_SINC_CONV_FILTER_TWO_TO_ONE_INT16_H__ */
