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


#include "resampler_rate_conversion_input_driven_int16.h"
#include "resampler_sinc_conv_one_to_two_int16.h"
#include "resampler_sinc_conv_one_to_three_int16.h"
#include "resampler_sinc_conv_two_to_three_int16.h"
#include "resampler_sinc_conv_two_to_one_int16.h"
#include "resampler_sinc_conv_three_to_one_int16.h"
#include "resampler_sinc_conv_44_to_48_int16.h"

bool 
RESAMPLER_RateConversionInputDrivenInt16::RateSupported(float inRate, 
                                                  float outRate, 
                                                  int channelCount)
{
    /* Note that this is a dummy test just to get rid of some warnings. */
    if (inRate == 0 || outRate == 0 || channelCount == 0)
    {
        return false;
    }

    if (RESAMPLER_SincConvOneToTwoInt16::RateSupported(inRate, outRate))
    {
        return true;
    }

    if (RESAMPLER_SincConvOneToThreeInt16::RateSupported(inRate, outRate))
    {
        return true;
    }

    if (RESAMPLER_SincConvTwoToThreeInt16::RateSupported(inRate, outRate))
    {
        return true;
    }

    if (RESAMPLER_SincConvTwoToOneInt16::RateSupported(inRate, outRate))
    {
        return true;
    }

    if (RESAMPLER_SincConvThreeToOneInt16::RateSupported(inRate, outRate))
    {
        return true;
    }

    if (RESAMPLER_SincConv44To48Int16::RateSupported(inRate, outRate))
    {
        return true;
    }

    return false;
}


RESAMPLER_RateConversionInputDrivenInt16 *
RESAMPLER_RateConversionInputDrivenInt16::New(float inRate, 
                                        float outRate, 
                                        int channelCount)
{
    RESAMPLER_RateConversionInputDrivenInt16 *converter = 0;

    /* Note that this is a dummy test just to make configuration easier. */
    if (inRate == 0 || outRate == 0 || channelCount == 0)
    {
        converter = 0;
    }

    else if (2*inRate == outRate)
    {
        converter = new RESAMPLER_SincConvOneToTwoInt16(channelCount);
    }

    else if (3*inRate == outRate)
    {
        converter = new RESAMPLER_SincConvOneToThreeInt16(channelCount);
    }

    else if (3*inRate == 2*outRate)
    {
        converter = new RESAMPLER_SincConvTwoToThreeInt16(channelCount);
    }

    else if (inRate == 2 * outRate)
    {
        converter = new RESAMPLER_SincConvTwoToOneInt16(channelCount);
    }

    else if (inRate == 3 * outRate)
    {
        converter = new RESAMPLER_SincConvThreeToOneInt16(channelCount);
    }
    
    else if (160 * inRate == 147 * outRate)
    {
        converter = new RESAMPLER_SincConv44To48Int16(channelCount);
    }

    return converter;
}

bool RESAMPLER_RateConversionInputDrivenInt16::SetQualityInputDriven(int /* mode */)
{
    return true;
}

void RESAMPLER_RateConversionInputDrivenInt16::EnableChannelInputDriven(int /* channel */)
{
}

void RESAMPLER_RateConversionInputDrivenInt16::DisableChannelInputDriven(int /* channel */)
{
}
