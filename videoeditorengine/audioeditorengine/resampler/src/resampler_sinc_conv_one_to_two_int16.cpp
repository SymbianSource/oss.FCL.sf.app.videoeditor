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


#include "resampler_sinc_conv_one_to_two_int16.h"
#include "resampler_clip.h"
#include "resampler_sinc_conv_one_to_two_tables_standard.h"

#include <string.h>

#include "resampler_sinc_conv_one_to_two_int16.inl"

static const int LC_ZERO_CROSSINGS = RESAMPLER_ONE_TO_TWO_ZERO_CROSSINGS_STANDARD; /* Needs to be divisible by two */


RESAMPLER_SincConvOneToTwoInt16::RESAMPLER_SincConvOneToTwoInt16(int channelCount) :
m_memBuffers(0),
m_scratchBuffer(0),
m_filter(RESAMPLER_ONE_TO_TWO_FILTERS_STANDARD),
m_channelCount(channelCount),
m_blockSize(0),
m_channelEnabled(0),
m_state(0)
{
}


RESAMPLER_SincConvOneToTwoInt16::~RESAMPLER_SincConvOneToTwoInt16()
{
    DeInit();
}


bool RESAMPLER_SincConvOneToTwoInt16::InitInputDriven()
{
    return Init();
}


bool RESAMPLER_SincConvOneToTwoInt16::InitOutputDriven()
{
    return Init();
}


bool RESAMPLER_SincConvOneToTwoInt16::Init()
{
    int i(0);
  
    m_memBuffers = new int16 *[m_channelCount];
    if (!m_memBuffers)
    {
        return false;
    }
    
    for (i = 0; i < m_channelCount; i++) 
    {
        m_memBuffers[i] = 0;
    }

    m_channelCount = m_channelCount;  
    
    m_channelEnabled = new bool[m_channelCount];
    if (!m_channelEnabled)
    {
        DeInit();
        return false;
    }
    for (i = 0; i < m_channelCount; i++) 
    {
        m_channelEnabled[i] = true;
    }

    for (i = 0; i < m_channelCount; i++) 
    {
        m_memBuffers[i] = new int16[LC_ZERO_CROSSINGS * 2];
        if (!m_memBuffers[i])
        {
            DeInit();
            return false;
        }
        memset(m_memBuffers[i], 0, sizeof(int16) * (LC_ZERO_CROSSINGS * 2));
    }

    return true;
}


void RESAMPLER_SincConvOneToTwoInt16::DeInit()
{
    if (m_channelCount)
    {
        for (int i = 0; i < m_channelCount; i++)
        {
            delete [] m_memBuffers[i];
        }
        delete [] m_memBuffers;
        delete [] m_channelEnabled;
    }
}


void RESAMPLER_SincConvOneToTwoInt16::EnableChannel(int channel)
{
    m_channelEnabled[channel] = true;
}


void RESAMPLER_SincConvOneToTwoInt16::DisableChannel(int channel)
{
    m_channelEnabled[channel] = false;
}


size_t 
RESAMPLER_SincConvOneToTwoInt16::ScratchMemoryNeedInputDriven(int maxInputBlockSize) const
{
    return sizeof(int16) * (LC_ZERO_CROSSINGS * 2 + maxInputBlockSize);
}


void 
RESAMPLER_SincConvOneToTwoInt16::SetScratchBufferInputDriven(char *buffer)
{
    m_scratchBuffer = (int16 *)buffer;
}


int RESAMPLER_SincConvOneToTwoInt16::MaxOutputSampleCount(int inSamples) const 
{ 
    return 2 * inSamples; 
}


int RESAMPLER_SincConvOneToTwoInt16::ProcessFromInput(int16 *outputBuffers[], 
                                                int16 *inputBuffers[], 
                                                int inSamples)
{
    int outSamples(2 * inSamples + m_state);
    return ProcessToOutput(outputBuffers, inputBuffers, outSamples);
}


size_t 
RESAMPLER_SincConvOneToTwoInt16::ScratchMemoryNeedOutputDriven(int maxOutputBlockSize) const
{
    return ScratchMemoryNeedInputDriven((maxOutputBlockSize + 1) >> 1);
}


void 
RESAMPLER_SincConvOneToTwoInt16::SetScratchBufferOutputDriven(char *buffer)
{
    m_scratchBuffer = (int16 *)buffer;
}


int RESAMPLER_SincConvOneToTwoInt16::MaxInputSampleCount(int outSamples) const 
{ 
    return (outSamples + 1) >> 1; 
}


int RESAMPLER_SincConvOneToTwoInt16::InSamplesNeeded(int outSamples)
{
    int inSamples( (outSamples >> 1) + (outSamples & 1) * (1 - m_state) );
    
    return inSamples;       
}


int RESAMPLER_SincConvOneToTwoInt16::ProcessToOutput(int16 *outputBuffers[], 
                                               int16 *inputBuffers[], 
                                               int outSamples)
{
    static const int FILTER_LENGTH = 2 * LC_ZERO_CROSSINGS;

    int i, j, k;
    int oldState = m_state;
    int newState = (oldState + outSamples) & 1;
    
    for (i = 0; i < m_channelCount; i++) 
    {
        if (!m_channelEnabled[i])
        {
            break;
        }

        int16 *tempBuf     = m_scratchBuffer;
        const int16 *inBuf = inputBuffers[i];
        int16 *outBuf      = outputBuffers[i];

        memcpy(m_scratchBuffer, m_memBuffers[i], FILTER_LENGTH * sizeof(int16));

        // Read samples into the memory buffer and 
        // copy samples into every second index of output buffer.
        for (j = 0, k = oldState; k < outSamples; j++, k+=2)
        {
            int16 inputSample(inBuf[j]);
            tempBuf[j + FILTER_LENGTH - 1 + oldState] = inputSample;
            outBuf[k] = tempBuf[j + LC_ZERO_CROSSINGS - 1 + oldState];
        }
        int inSamples = j;
        
        // Do band-limited interpolation and set the result into 
        // every second index in the output buffer. 
#ifdef RESAMPLER_SINC_CONV_ONE_TO_TWO_DUAL_FILTERING
        for (j = 0, k = 1 - oldState; k < outSamples; j+=2, k+=4) 
        {
            int newSample1(0);
            int newSample2(0);

            RESAMPLER_SincConvOneToTwoFilterDualInt16(newSample1,
                                                newSample2,
                                                tempBuf + LC_ZERO_CROSSINGS + j,
                                                m_filter,
                                                LC_ZERO_CROSSINGS);
            // Round, shift down, and clip to 16 bits
            outBuf[k] = (int16)RESAMPLER_Clip16((newSample1 + 16384) >> 15);
            if (k + 2 < outSamples)
            {
                outBuf[k+2] = (int16)RESAMPLER_Clip16((newSample2 + 16384) >> 15);
            }
        }
#else
        for (j = 0, k = 1 - oldState; k < outSamples; j++, k+=2) 
        {
            int32 newSample = 
                RESAMPLER_SincConvOneToTwoFilterInt16(tempBuf + LC_ZERO_CROSSINGS + j,
                                                m_filter,
                                                LC_ZERO_CROSSINGS);
            // Round, shift down, and clip to 16 bits
            outBuf[k] = (int16)RESAMPLER_Clip16((newSample + 16384) >> 15);
        }
#endif

        // Copy the newest samples to the beginning of the buffer
        memcpy(m_memBuffers[i], 
               tempBuf + inSamples + oldState - newState, 
               (FILTER_LENGTH - 1 + newState) * sizeof(int16));
    }
    
    // Update state according to even or odd amount of output samples.
    m_state = newState;
    
    return outSamples;
}
