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


#include "resampler_sinc_conv_two_to_three_int16.h"
#include "resampler_clip.h"
#include "resampler_sinc_conv_two_to_three_tables_standard.h"

#include "resampler_sinc_conv_two_to_three_int16.inl"


#include <string.h>


static const int LC_ZERO_CROSSINGS = RESAMPLER_TWO_TO_THREE_ZERO_CROSSINGS_STANDARD;
static const int LC_MOD3[6] = { 0, 1, 2, 0, 1, 2 };


RESAMPLER_SincConvTwoToThreeInt16::RESAMPLER_SincConvTwoToThreeInt16(int channelCount) :
m_memBuffers(0),
m_scratchBuffer(0),
m_filter1(RESAMPLER_TWO_TO_THREE_FILTER1_STANDARD),
m_filter2(RESAMPLER_TWO_TO_THREE_FILTER2_STANDARD),
m_channelCount(channelCount),
m_blockSize(0),
m_channelEnabled(0),
m_inputSamples(0),
m_state(2)
{
}


RESAMPLER_SincConvTwoToThreeInt16::~RESAMPLER_SincConvTwoToThreeInt16()
{
    DeInit();
}


bool RESAMPLER_SincConvTwoToThreeInt16::InitInputDriven()
{
    return Init();
}


bool RESAMPLER_SincConvTwoToThreeInt16::InitOutputDriven()
{
    return Init();
}


/*
*   This function must be called before using the converter.
*   This function reserves memory for ring buffers and 
*   prepares the ring buffer indexing.
*/
bool RESAMPLER_SincConvTwoToThreeInt16::Init()
{
    int i = 0;

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


void RESAMPLER_SincConvTwoToThreeInt16::DeInit()
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


void RESAMPLER_SincConvTwoToThreeInt16::EnableChannel(int channel)
{
    m_channelEnabled[channel] = true;
}


void RESAMPLER_SincConvTwoToThreeInt16::DisableChannel(int channel)
{
    m_channelEnabled[channel] = false;
}


int RESAMPLER_SincConvTwoToThreeInt16::MaxOutputSampleCount(int inSamples) const 
{ 
    return 3 * inSamples / 2 + 1; 
}


int RESAMPLER_SincConvTwoToThreeInt16::ProcessFromInput(int16 *outputBuffers[], 
                                                  int16 *inputBuffers[], 
                                                  int inSamples)
{
    int outSamples( (3*inSamples + 2 - m_state) / 2 );
    m_inputSamples = inSamples;

    return ProcessToOutput(outputBuffers, inputBuffers, outSamples);
}


size_t 
RESAMPLER_SincConvTwoToThreeInt16::ScratchMemoryNeedOutputDriven(int maxOutputBlockSize) const
{
    return ScratchMemoryNeedInputDriven((2*maxOutputBlockSize + 2) / 3);
}


void 
RESAMPLER_SincConvTwoToThreeInt16::SetScratchBufferOutputDriven(char *buffer)
{
    m_scratchBuffer = (int16 *)buffer;
}

size_t 
RESAMPLER_SincConvTwoToThreeInt16::ScratchMemoryNeedInputDriven(int maxInputBlockSize) const
{
    return sizeof(int16) * (LC_ZERO_CROSSINGS * 2 + maxInputBlockSize);
}


void 
RESAMPLER_SincConvTwoToThreeInt16::SetScratchBufferInputDriven(char *buffer)
{
    m_scratchBuffer = (int16 *)buffer;
}

int RESAMPLER_SincConvTwoToThreeInt16::MaxInputSampleCount(int outSamples) const 
{ 
    return (2*outSamples+2)/3; 
}

    
/*
*   This function returns the value, which is given as
*   a parameter to the nextSamplesOut() function. This
*   function must be called every time before calling
*   nextSamplesOut.
*/
int RESAMPLER_SincConvTwoToThreeInt16::InSamplesNeeded(int outSamples)
{
    m_inputSamples = (2 * outSamples + m_state) / 3;
    return m_inputSamples;
}


int RESAMPLER_SincConvTwoToThreeInt16::ProcessToOutput(int16 *outputBuffers[], 
                                                 int16 *inputBuffers[], 
                                                 int outSamples)
{
    static const int FILTER_LENGTH = 2 * LC_ZERO_CROSSINGS; 
    int i, j, k;
    
    int bufReadHead1 = LC_ZERO_CROSSINGS + m_state;
    int bufReadHead2 = LC_ZERO_CROSSINGS + (m_state == 1 ? 1 : 0);
    int bufReadHead3 = LC_ZERO_CROSSINGS + (m_state != 1 ? 1 : 0);

    for (i = 0; i < m_channelCount; i++) 
    {
        if (!m_channelEnabled[i])
        {
            break;
        }

        int16 *tempBuf = m_scratchBuffer;
        int16 *outBuf  = outputBuffers[i];

        memcpy(m_scratchBuffer, m_memBuffers[i], FILTER_LENGTH * sizeof(int16));

        memcpy(tempBuf + FILTER_LENGTH, 
               inputBuffers[i], 
               m_inputSamples * sizeof(int16));

        // copy every second sample into every third index of output buffer.
        tempBuf = m_scratchBuffer + bufReadHead1;
        for (j = 0, k = m_state; k < outSamples; j+=2, k+=3)
        {
            outBuf[k] = tempBuf[j]; 
        }
        
        // Do band limited interpolation and set the result into 
        // every third index in the output buffer. 
        tempBuf = m_scratchBuffer + bufReadHead2;
        for (j = 0, k = LC_MOD3[m_state+1]; k < outSamples; j += 2, k += 3) 
        {
             // Note that the filters are reversed
 
            int32 newSample = RESAMPLER_SincConvTwoToThreeFilterInt16(tempBuf + j, 
                                                                m_filter2,
                                                                m_filter1,
                                                                LC_ZERO_CROSSINGS);

            // round and shift down 
            outBuf[k] = (int16)RESAMPLER_Clip16((newSample + 16384) >> 15);
        }
       
        // Do band limited interpolation and set the result into 
        // every third index in the output buffer. 
        tempBuf = m_scratchBuffer + bufReadHead3;
        for (j = 0, k = LC_MOD3[m_state+2]; k < outSamples; j += 2, k += 3) 
        {
            int32 newSample = RESAMPLER_SincConvTwoToThreeFilterInt16(tempBuf + j, 
                                                                m_filter1,
                                                                m_filter2,
                                                                LC_ZERO_CROSSINGS);

            // round and shift down 
            outBuf[k] = (int16)RESAMPLER_Clip16((newSample + 16384) >> 15);
        }

        // Copy the newest samples to the beginning of the buffer
        memcpy(m_memBuffers[i], 
               m_scratchBuffer + m_inputSamples, 
               FILTER_LENGTH * sizeof(int16));       
    }    
    
    // Update state according to amount of output samples.
    m_state = LC_MOD3[m_state + (3 - (outSamples % 3))];

    return outSamples;
}


