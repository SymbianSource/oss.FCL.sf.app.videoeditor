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


#include "resampler_sinc_conv_44_to_48_int16.h"
#include "resampler_clip.h"
#include "resampler_sinc_conv_44_to_48_tables_economy.h"
#include "resampler_sinc_conv_44_to_48_tables_standard.h"
#include "resampler_sinc_conv_44_to_48_tables_premium.h"

#include <string.h>

#include "resampler_sinc_conv_filter_int16.h"

/*
 * The amount of zero crossings in positive or negative
 * side of the sinc function. Because of filter symmetry
 * the amount of filter taps used in convolution is
 * zero crossings * 2.
 */
static const int LC_MAX_ZERO_CROSSINGS = RESAMPLER_44_TO_48_ZERO_CROSSINGS_PREMIUM;

static const int LC_BUFFER_ACCESS_OUTPUT = LC_MAX_ZERO_CROSSINGS;

/*
 * The number of filters needed in 44.1 to 48 kHz sinc interpolation
 * sampling rate conversion. Filter symmetry is utilized.
 */
static const int LC_FILTER_COUNT = 160;

/*
 * Calculated as 160-147.
 * 147/160 is the smallest ratio of 44100/48000.
 * Used as a hop increment in the filter table defined below.
 */
static const int LC_MATRIX_HOP_SIZE = 13;       

/*
 * This constant is calculated as ceil(2^23/160).
 * and used in InSamplesNeeded() method to 
 * avoid the division by 160. The result of the multiplication
 * has to be shifted afterwards by 23 bits right.
 */
static const int LC_SAMPLE_COUNT_MULT = 52429;   

/** Do not remove! LC_MAX_BLOCK_SIZE used in assert!
 */
#if 0
/* 
 * M_INPUT_COUNT_MULT * (6289*13+160) = 4294826393 (0xfffdd999).
 * 6289 is the largest number that fits 32 bits in the
 * input sample count multiplication method.
 */
static const int LC_MAX_BLOCK_SIZE = 6289;      
#endif // 0


RESAMPLER_SincConv44To48Int16::RESAMPLER_SincConv44To48Int16(int channelCount) :
m_memBuffers(0),
m_scratchBuffer(0),
m_channelCount(channelCount),
m_blockSize(0),
m_channelEnabled(0),
m_inputSamples(0),
m_index1(LC_FILTER_COUNT),
m_index2(0),
m_zeroCrossings(RESAMPLER_44_TO_48_ZERO_CROSSINGS_STANDARD),
m_filterMatrix(RESAMPLER_44_TO_48_FILTERS_STANDARD)
{
}


RESAMPLER_SincConv44To48Int16::~RESAMPLER_SincConv44To48Int16()
{
    DeInit();
}

bool RESAMPLER_SincConv44To48Int16::InitInputDriven()
{
    return Init();
}


bool RESAMPLER_SincConv44To48Int16::InitOutputDriven()
{
    return Init();
}

bool RESAMPLER_SincConv44To48Int16::Init()
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
        m_memBuffers[i] = new int16[LC_MAX_ZERO_CROSSINGS * 2];
        if (!m_memBuffers[i])
        {
            DeInit();
            return false;
        }
        memset(m_memBuffers[i], 0, sizeof(int16) * (LC_MAX_ZERO_CROSSINGS * 2));
    }

    return true;
}

void 
RESAMPLER_SincConv44To48Int16::DeInit()
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

bool 
RESAMPLER_SincConv44To48Int16::SetQualityInputDriven(int mode)
{
    return SetQualityOutputDriven(mode);
}

bool 
RESAMPLER_SincConv44To48Int16::SetQualityOutputDriven(int mode)
{
    switch (mode)
    {
    case RESAMPLER_RATE_CONVERSION_QUALITY_ECONOMY:
        m_zeroCrossings = RESAMPLER_44_TO_48_ZERO_CROSSINGS_ECONOMY;
        m_filterMatrix  = RESAMPLER_44_TO_48_FILTERS_ECONOMY;
        break;
    case RESAMPLER_RATE_CONVERSION_QUALITY_STANDARD:
        m_zeroCrossings = RESAMPLER_44_TO_48_ZERO_CROSSINGS_STANDARD;
        m_filterMatrix  = RESAMPLER_44_TO_48_FILTERS_STANDARD;
        break;
    case RESAMPLER_RATE_CONVERSION_QUALITY_PREMIUM:
        m_zeroCrossings = RESAMPLER_44_TO_48_ZERO_CROSSINGS_PREMIUM;
        m_filterMatrix  = RESAMPLER_44_TO_48_FILTERS_PREMIUM;
        break;
    default:
        return false;
    }
    return true;
}

void 
RESAMPLER_SincConv44To48Int16::EnableChannelInputDriven(int channel)
{
    EnableChannelOutputDriven(channel);
}


void 
RESAMPLER_SincConv44To48Int16::DisableChannelInputDriven(int channel)
{
    DisableChannelOutputDriven(channel);
}


void 
RESAMPLER_SincConv44To48Int16::EnableChannelOutputDriven(int channel)
{
    m_channelEnabled[channel] = true;
}


void 
RESAMPLER_SincConv44To48Int16::DisableChannelOutputDriven(int channel)
{
    m_channelEnabled[channel] = false;
}

size_t 
RESAMPLER_SincConv44To48Int16::ScratchMemoryNeedInputDriven(int maxInputBlockSize) const
{
    /** 6289 == LC_MAX_BLOCK_SIZE
     */
    if (maxInputBlockSize > 6289 * 44100.0/48000.0)
        {
        return 0;
        }

    return sizeof(int16) * (LC_MAX_ZERO_CROSSINGS * 2 + maxInputBlockSize);
}

void 
RESAMPLER_SincConv44To48Int16::SetScratchBufferInputDriven(char *buffer)
{
    m_scratchBuffer = (int16 *)buffer;
}

size_t 
RESAMPLER_SincConv44To48Int16::ScratchMemoryNeedOutputDriven(int maxOutputBlockSize) const
{
    int blockSize =  (int)((maxOutputBlockSize * 44100.0/48000.0) + 1); 
    
    return ScratchMemoryNeedInputDriven(blockSize);
}

void 
RESAMPLER_SincConv44To48Int16::SetScratchBufferOutputDriven(char *buffer)
{
    m_scratchBuffer = (int16 *)buffer;
}



int 
RESAMPLER_SincConv44To48Int16::MaxInputSampleCount(int outSamples) const
{ 
    return (int)((outSamples * (44100.0 / 48000.0)) + 1); 
}


int 
RESAMPLER_SincConv44To48Int16::InSamplesNeeded(int outSamples)
{
    m_inputSamples = outSamples - ((LC_SAMPLE_COUNT_MULT * (outSamples*13+m_index2)) >> 23);
 
    return m_inputSamples;
}

int 
RESAMPLER_SincConv44To48Int16::MaxOutputSampleCount(int inSamples) const
{ 
    return (int)(inSamples * 160.0 / 147.0 + 1.0); 
}


int RESAMPLER_SincConv44To48Int16::ProcessFromInput(int16 *outputBuffers[], 
                                              int16 *inputBuffers[], 
                                              int inSamples)
{
    int outSamples = (int)((inSamples * 160.0 + m_index2) / 147.0);
    m_inputSamples = inSamples;
    return ProcessToOutput(outputBuffers, inputBuffers, outSamples);
}

int 
RESAMPLER_SincConv44To48Int16::ProcessToOutput(int16 *outputBuffers[], 
                                         int16 *inputBuffers[], 
                                         int outSamples)
{
    static const int FILTER_LENGTH = 2 * LC_MAX_ZERO_CROSSINGS;
	int i, j;
    int index1 = m_index1;
    int index2 = m_index2;

    for (i = 0; i < m_channelCount; i++) 
    {
        if (!m_channelEnabled[i])
        {
            break;
        }

        int16 *tempBuf = m_scratchBuffer;
        int16 *outBuf  = outputBuffers[i];
        index1 = m_index1; /* 160 SRC filter number */
        index2 = m_index2; /* 0 */
        
        int state = 0;

        memcpy(m_scratchBuffer, 
               m_memBuffers[i], 
               FILTER_LENGTH * sizeof(int16));

        // Read samples into memory
        memcpy(tempBuf + FILTER_LENGTH, 
               inputBuffers[i], 
               m_inputSamples * sizeof(int16));

        // Do band limited interpolation and set the result into 
        // every index in the output buffer. 
        for (j = 0; j < outSamples; j++) 
        {
            index1 -= LC_MATRIX_HOP_SIZE; /* -13 */ 
            index2 += LC_MATRIX_HOP_SIZE;  /* +13 */ 

            if (index1 <= 0)
            {
                index1 += LC_FILTER_COUNT; /* +160 */
                index2 -= LC_FILTER_COUNT;  /* -160 */
                state++;
            }

            /*lint -e{662} */
            const int16 *filterPtr1 = m_filterMatrix + index1 * m_zeroCrossings;
            const int16 *filterPtr2 = m_filterMatrix + index2 * m_zeroCrossings;
              
            int32 newSample = 
                RESAMPLER_SincConvFilterInt16(tempBuf + LC_BUFFER_ACCESS_OUTPUT - state + j,
                                        filterPtr1,
                                        filterPtr2,
                                        m_zeroCrossings);
             
            // round and shift down
            outBuf[j] = (int16)RESAMPLER_Clip16((newSample + 16384) >> 15);
        }
        
        // Copy the newest samples to the beginning of the buffer
        memcpy(m_memBuffers[i], 
               m_scratchBuffer + m_inputSamples, 
               FILTER_LENGTH * sizeof(int16));       
    }

    m_index1 = index1;
    m_index2 = index2;

	return outSamples;
}

