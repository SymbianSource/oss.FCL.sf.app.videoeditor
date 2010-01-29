#ifndef __RESAMPLER_SINC_CONV_ONE_TO_TWO_INT16_H__
#define __RESAMPLER_SINC_CONV_ONE_TO_TWO_INT16_H__
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
#include "resampler_rate_conversion_output_driven_int16.h"


/** @ingroup rate_conversion

  @brief Sampling rate conversion implementation for upsampling by two.
        
*/

class RESAMPLER_SincConvOneToTwoInt16 : public RESAMPLER_RateConversionInputDrivenInt16, 
                                  public RESAMPLER_RateConversionOutputDrivenInt16
{
    friend class RESAMPLER_RateConversionInputDrivenInt16;
    friend class RESAMPLER_RateConversionOutputDrivenInt16;

public:
    
    /** @name Construction & Destruction */
    
    //@{
    
   /** Destructor
    */
    
    virtual ~RESAMPLER_SincConvOneToTwoInt16();
    
    //@}
    
    /** @name Query methods */

    //@{

    static bool RateSupported(float inRate, float outRate)
    { return (2*inRate == outRate); }

    //@}

    /** @name Object lifetime methods */
    
    //@{
    
    /** Initializes the converter.
    
    This method initializes the sampling rate converter for input-driven
    operation.
      
    @return true if successful false otherwise
    */
    
    virtual bool InitInputDriven();
    
    /** Initializes the converter.
    
    This method initializes the sampling rate converter for output-driven
    operation.
      
    @return true if successful false otherwise
    */
    
    virtual bool InitOutputDriven();
    
    //@}
    
    /** @name Input-driven operation */
    
    //@{
    
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

    virtual void EnableChannelInputDriven(int channel)
    { EnableChannel(channel); }

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

    virtual void DisableChannelInputDriven(int channel)
    { DisableChannel(channel); }

    /** Query the scratch memory need of the converter.
    
    This method queries the amount of scratch memory the input driven converter
    needs to be able to handle in a single call to the processing functions.

    @param[in] maxInputBlockSize  Maximum input blocksize
    @return memory need in bytes
    */
    
    virtual size_t ScratchMemoryNeedInputDriven(int maxInputBlockSize) const;

    /** Set scratch buffer for the converter

    This method sets scratch buffer needed by the converter when the converter is 
    used in output driven mode. . The caller of this function is responsible of 
    allocating enough memory.

    @param[in] *buffer pointer to the allocated buffer
    */

    virtual void SetScratchBufferInputDriven(char *buffer);

    /** Get the maximum number of input samples needed.
    
    This method returns the maximum number of input samples needed 
    for a given output sample count.

    @param[in] outSamples  Number of output samples
    @return Maximum number of input samples
    */

    virtual int MaxInputSampleCount(int outSamples) const;
    
    /** Get the amount of samples needed for input.
      
    This method returns the amount of samples needed for converter input
    given the number of output samples. This method must be called before
    each call to the processing methods. It is a serious error to have
    \a outSamples greater than the value set with ScratchMemoryNeedOutputDriven().
 
    @param[in] outSamples  Amount of samples the converter produces
    @return number of input samples needed
    */

    virtual int InSamplesNeeded(int outSamples);

    /** Run the sampling rate conversion for a block of samples
    
    This method runs the actual sampling rate conversion. The pointer arrays
    have to contain valid pointers for numChannels channel data buffers. The 
    output buffers must have space for MaxOutputSampleCount(inSamples) audio 
    samples.
      
    @param[out] outputBuffers  Pointer to output buffer array
    @param[in]  inputBuffers   Pointer to input buffer array
    @param[in]  inSamples      The number of input samples to process
    @return Number of output samples
    */

    virtual int ProcessFromInput(int16 *outputBuffers[], 
                                 int16 *inputBuffers[], 
                                 int inSamples);

    //@}
    
    /** @name Output-driven operation */
    
    //@{
    
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

    virtual void EnableChannelOutputDriven(int channel)
    { EnableChannel(channel); }

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

    virtual void DisableChannelOutputDriven(int channel)
    { DisableChannel(channel); }

    /** Query the scratch memory need of the converter.
    
    This method queries the amount of scratch memory the output driven converter
    needs to be able to handle in a single call to the processing functions.

    @param[in] maxOutputBlockSize  Maximum output blocksize
    @return memory need in bytes
    */

    virtual size_t ScratchMemoryNeedOutputDriven(int maxOutputBlockSize) const;

    /** Set scratch buffer for the converter

    This method sets scratch buffer needed by the converter when then converter is 
    used in output driven mode. The caller of this function is responsible of 
    allocating enough memory.

    @param[in] *buffer pointer to the allocated buffer
    */

    virtual void SetScratchBufferOutputDriven(char *buffer);

    /** Get the maximum number of output samples for a given input sample count
    
    This method returns the maximum number of output samples the converter
    can ever produce from a given number of input samples. The method assumes
    that the converter be used either input-driven or output-driven; the
    two modes should not be mixed.
      
    @param[in] inSamples  Number of input samples
    @return Maximum number of output samples
    */

    virtual int MaxOutputSampleCount(int inSamples) const;

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
                                int outSamples);

    //@}
    
private:
    
    /** Constructor
     */
    RESAMPLER_SincConvOneToTwoInt16(int channelCount);

    /** Disabled copy contructor 
     */
    RESAMPLER_SincConvOneToTwoInt16(const RESAMPLER_SincConvOneToTwoInt16 &);
    
    /** Initializes the converter.
    
      This method initializes the sampling rate converter. The 
      initialization will be successful only if 2*inRate == outRate.
      
      @return true if successful false otherwise
    */
    
    bool Init();
    
    /** Deallocate all the allocated memory 

    This methods deallocates all memory previously allocated by 
    Init(). Can be used both for recovering from a memory 
    allocation error and in destructor.
    */
    void DeInit();

    /** Enable one of the channels */
    void EnableChannel(int channel);

    /** Disable one of the channels */
    void DisableChannel(int channel);

    int16       **m_memBuffers;
    int16        *m_scratchBuffer;
    const int16  *m_filter;
    int           m_channelCount;
    int           m_blockSize;
    bool         *m_channelEnabled;
    
    int           m_state;
};

#endif  // __RESAMPLER_SINC_CONV_ONE_TO_TWO_INT16_H__
