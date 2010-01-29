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


#include "resampler_sinc_conv_two_to_one_int16.h"
#include "resampler_clip.h"
#include "resampler_sinc_conv_two_to_one_tables_standard.h"
#include "resampler_sinc_conv_filter_two_to_one_int16.h"

#include <string.h>

/*
 * The amount of zero crossings in positive or negative
 * side of the sinc function. Because of filter symmetry
 * the amount of filter taps used in convolution is
 * zero crossings * 2.
 */
static const int LC_MAX_ZERO_CROSSINGS = RESAMPLER_TWO_TO_ONE_ZERO_CROSSINGS_STANDARD;

static const int LC_BUFFER_ACCESS_OUTPUT = 2 * LC_MAX_ZERO_CROSSINGS;


RESAMPLER_SincConvTwoToOneInt16::RESAMPLER_SincConvTwoToOneInt16(int channelCount) :
m_memBuffers(0),
m_scratchBuffer(0),
m_channelCount(channelCount),
m_blockSize(0),
m_channelEnabled(0),
m_state(0),
m_zeroCrossings(RESAMPLER_TWO_TO_ONE_ZERO_CROSSINGS_STANDARD),
m_filterMatrix(RESAMPLER_TWO_TO_ONE_FILTERS_STANDARD)
{
}


RESAMPLER_SincConvTwoToOneInt16::~RESAMPLER_SincConvTwoToOneInt16()
{
    DeInit();
}

bool RESAMPLER_SincConvTwoToOneInt16::InitInputDriven()
{
    return Init();
}

bool RESAMPLER_SincConvTwoToOneInt16::InitOutputDriven()
{
    return Init();
}

bool 
RESAMPLER_SincConvTwoToOneInt16::Init()
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
        m_memBuffers[i] = new int16[LC_MAX_ZERO_CROSSINGS * 4];
        if (!m_memBuffers[i])
        {
            DeInit();
            return false;
        }
        memset(m_memBuffers[i], 0, sizeof(int16) * (LC_MAX_ZERO_CROSSINGS * 4));
    }

    return true;
}

void 
RESAMPLER_SincConvTwoToOneInt16::DeInit()
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


void 
RESAMPLER_SincConvTwoToOneInt16::EnableChannel(int channel)
{
    m_channelEnabled[channel] = true;
}

void 
RESAMPLER_SincConvTwoToOneInt16::DisableChannel(int channel)
{
    m_channelEnabled[channel] = false;
}

size_t 
RESAMPLER_SincConvTwoToOneInt16::ScratchMemoryNeedInputDriven(int maxInputBlockSize) const
{
    return sizeof(int16) * (LC_MAX_ZERO_CROSSINGS * 4 + maxInputBlockSize);
}

void 
RESAMPLER_SincConvTwoToOneInt16::SetScratchBufferInputDriven(char *buffer)
{
    m_scratchBuffer = (int16 *)buffer;
}

size_t 
RESAMPLER_SincConvTwoToOneInt16::ScratchMemoryNeedOutputDriven(int maxOutputBlockSize) const
{
    int blockSize = 2 * maxOutputBlockSize; 
    
    return ScratchMemoryNeedInputDriven(blockSize);
}

void 
RESAMPLER_SincConvTwoToOneInt16::SetScratchBufferOutputDriven(char *buffer)
{
    m_scratchBuffer = (int16 *)buffer;
}


int 
RESAMPLER_SincConvTwoToOneInt16::MaxOutputSampleCount(int inSamples) const 
{ 
    return ( inSamples + 1 ) / 2; 
}

int 
RESAMPLER_SincConvTwoToOneInt16::MaxInputSampleCount(int outSamples) const
{ 
    return 2 * outSamples; 
}


int 
RESAMPLER_SincConvTwoToOneInt16::InSamplesNeeded(int outSamples)
{
    return 2 * outSamples;
}


int 
RESAMPLER_SincConvTwoToOneInt16::ProcessToOutput(int16 *outputBuffers[], 
                                           int16 *inputBuffers[], 
                                           int outSamples)
{
    int inSamples( 2 * outSamples );
    
    return ProcessFromInput(outputBuffers, inputBuffers, inSamples);
}


int 
RESAMPLER_SincConvTwoToOneInt16::ProcessFromInput(int16 *outputBuffers[], 
                                            int16 *inputBuffers[], 
                                            int inSamples)
{
    static const int FILTER_LENGTH = 4 * LC_MAX_ZERO_CROSSINGS;
	int i, j;
    int state = m_state;
	int outSamples = (inSamples + state) >> 1;

    for (i = 0; i < m_channelCount; i++) 
    {
        if (!m_channelEnabled[i])
        {
            break;
        }

        int16 *tempBuf = m_scratchBuffer;
        int16 *outBuf  = outputBuffers[i];
        state = m_state;
        
        memcpy(m_scratchBuffer, 
               m_memBuffers[i], 
               FILTER_LENGTH * sizeof(int16));

        // Read samples into memory
        memcpy(tempBuf + FILTER_LENGTH, 
               inputBuffers[i], 
               inSamples * sizeof(int16));

        tempBuf += LC_BUFFER_ACCESS_OUTPUT - state;

        for (j = 0; j < outSamples; j++) 
        {
            int32 newSample = 
                RESAMPLER_SincConvFilterTwoToOneInt16(tempBuf,
												m_filterMatrix,
												m_zeroCrossings);
            tempBuf += 2;

            // round and shift down
	        outBuf[j] = (int16)RESAMPLER_Clip16((newSample + 16384 ) >> 15);
		}
        
        // Copy the newest samples to the beginning of the buffer
        memcpy(m_memBuffers[i], 
               m_scratchBuffer + inSamples, 
               FILTER_LENGTH * sizeof(int16));       
    }

    m_state = (state + inSamples) & 1;

	return outSamples;
}

