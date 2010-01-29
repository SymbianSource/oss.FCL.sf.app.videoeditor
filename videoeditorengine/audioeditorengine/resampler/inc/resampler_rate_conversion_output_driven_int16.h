#ifndef __RESAMPLER_RATE_CONVERSION_OUTPUT_DRIVEN_INT16_H__
#define __RESAMPLER_RATE_CONVERSION_OUTPUT_DRIVEN_INT16_H__
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


/** @ingroup rate_conversion

Base class for output-driven sampling rate conversion.

*/

#include "resampler_data_types.h"
#include "resampler_rate_conversion_quality.h"


class RESAMPLER_RateConversionOutputDrivenInt16
{
public:
    
    /** @name Construction & Destruction */
    
    //@{

    /** This method checks if the desired conversion is available.

    @param[in] inRate       Input sampling rate
    @param[in] outRate      Output sampling rate
    @param[in] channelCount Number of audio channels
    */
    static bool RateSupported(float inRate, 
                              float outRate, 
                              int channelCount);

    /** This method creates a sampling rate converter for input-driven
    operation.

    @param[in] inRate       Input sampling rate
    @param[in] outRate      Output sampling rate
    @param[in] channelCount Number of audio channels
    */

    static RESAMPLER_RateConversionOutputDrivenInt16 * New(float inRate, 
                                                     float outRate, 
                                                     int channelCount);
    
    /** Constructor
     */
    
    RESAMPLER_RateConversionOutputDrivenInt16() { ; }
    
    /** Destructor
    */
    
    virtual ~RESAMPLER_RateConversionOutputDrivenInt16() { ; }
    
    //@}
    
    /** @name Object lifetime methods */
    
    //@{
    
    /** Initializes the converter.
    
    This method initializes the sampling rate converter for output-driven
    operation. Calling this method twice for the same converter instance
    is considered a programming error, and behaviour in that case is 
    undefined.

    @return true if successful false otherwise
    */
    
    virtual bool InitOutputDriven() = 0;
    
    //@}

    /** @name Operation */

    //@{

    /** Set the quality mode for the subsequent operations */

    virtual bool SetQualityOutputDriven(int mode);

    /** Enable one of the channels.
    
    This method is used to (re-)enable a rate conversion channel after it's
    been disabled. (All channels start in the enabled mode.) This is used to
    tell the algorithm that the channel must be properly processed, if the
    algorithm wants to take advantage of the optimization possibilities 
    provided by the DisableInputDriven() method.

    @note The channel index MUST be valid, i.e., it has to be smaller than
          @c numChannels given at initialization time.

    @param[in] channel  The index of the channel to be enabled.
    */

    virtual void EnableChannelOutputDriven(int channel);

    /** Disable one of the channels.
    
    This method can be used to tell the algorithm that the output from a
    specific channel will not be used in subsequent operations, and the 
    implementation may choose to optimize and leave the disabled channel
    unprocessed. However, the caller must always provide valid pointers for
    the actual processing methods for the case where it's difficult to 
    actually disable memory access for the specific channel in an 
    implementation.

    @note The channel index MUST be valid, i.e., it has to be smaller than
          @c numChannels given at initialization time.

    @param[in] channel  The index of the channel to be enabled.
    */

    virtual void DisableChannelOutputDriven(int channel);

    /** Query the scratch memory need of the converter.
    
    This method queries the amount of scratch memory the converter needs to be 
    able to handle in a single call to the processing functions.

    @param[in] maxOutputBlockSize  Maximum output blocksize
    @return memory need in bytes
    */

    virtual size_t ScratchMemoryNeedOutputDriven(int maxOutputBlockSize) const = 0;

    /** Set scratch buffer for the converter

    This method sets scratch buffer needed by the converter. The caller of this
    function is responsible of allocating enough memory.

    @param[in] *buffer pointer to the allocated buffer
    */

    virtual void SetScratchBufferOutputDriven(char *buffer) = 0;


    /** Get the maximum number of input samples needed.
    
    This method returns the maximum number of input samples needed 
    for a given output sample count.

    @param[in] outSamples  Number of output samples
    @return Maximum number of input samples
    */

    virtual int MaxInputSampleCount(int outSamples) const = 0;
    
    /** Get the amount of samples needed for input.
      
    This method returns the amount of samples needed for converter input
    given the number of output samples. This method must be called before
    each call to the processing methods. It is a serious error to have
    \a outSamples greater than the value set with ScratchMemoryNeedOutputDriven().
 
    @param[in] outSamples  Amount of samples the converter produces
    @return number of input samples needed
    */

    virtual int InSamplesNeeded(int outSamples) = 0;

    /** Run the sampling rate conversion for a block of samples

    This method does the sampling rate conversion. The pointer arrays
    have to contain valid pointers for numChannels channel data buffers.
    The input buffers MUST contain the amount of samples returned by
    a previous call to InSamplesNeeded. Output buffers must have space
    for \a outSamples audio samples.
 
    @param[out] outputBuffers  Pointer to output buffer array
    @param[in]  inputBuffers   Pointer to input buffer array
    @param[in]  outSamples     The number of samples the converter produces
    @return Number of output samples
    */

    virtual int ProcessToOutput(int16 *outputBuffers[], 
                                int16 *inputBuffers[], 
                                int outSamples) = 0;

    //@}

};

#endif  // __RESAMPLER_RATE_CONVERSION_OUTPUT_DRIVEN_INT16_H__
