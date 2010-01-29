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



#ifndef __CRateConverter_H__
#define __CRateConverter_H__

#include <e32base.h>

#include "resampler_rate_conversion_input_driven_int16.h"

class CRateConverter : public CBase
    {

public:
    
    /*
    * Symbian constructors
    *
    */
    static CRateConverter* NewL(TInt aFromSampleRate, TInt aToSampleRate, TInt aFromChannels, TInt aToChannels);
                                        
    static CRateConverter* NewLC(TInt aFromSampleRate, TInt aToSampleRate, TInt aFromChannels, TInt aToChannels);

    /*
    * Destructor
    */
    ~CRateConverter();
    
    /*
    * Initialize the rate converter
    *
    * @param aInputBufferSize Max size of the input buffer in number of samples
    * @return ETrue if successful
    *
    */
    TBool InitL(TInt aInputBufferSize);

    /*
    * Does rate and channel conversion for given buffer
    *
    * @param aInput Pointer to input buffer (16-bit samples)
    * @param aOutput Pointer to output buffer (16-bit samples)
    * @param aInputSampleCount Number of samples in the input buffer
    * @return Number of samples in the output buffer
    */
    TInt ConvertBufferL(TInt16* aInput, TInt16* aOutput, TInt aInputSampleCount);  
    
    /*
    * Returns the size of the output buffer
    *
    */
    TInt GetOutputBufferSize() { return iOuputBlockSize; }
    

protected:
    
    // constructL    
    void ConstructL();
    
    // C++ constructor
    CRateConverter(TInt aFromSampleRate, TInt aToSampleRate, TInt aFromChannels, TInt aToChannels);
    
private:
    
    /*
    * Does internal initialization
    *
    */
    TBool DoInitL(TInt aInputBufferSize);
    
    /*
    * Does the actual conversion
    *
    */
    TInt DoConvertL(TInt16** aInput, TInt aInputSampleCount);
    
    /*
    * Returns pointer to the output buffer
    *
    */
    TInt16** GetOutputBuffer() { return iOutBuffer; }
          
private:

    // Another converter incase two phase converting is needed
    CRateConverter* iChild;

    // The actual converter
    RESAMPLER_RateConversionInputDrivenInt16* iConverter;
    
    // Input data for rate converter
    TInt16** iInBuffer;
    
    // Output data from rate converter
    TInt16** iOutBuffer;
    
    // Temp buffer used by converter
    TInt8* iScratchBuffer;

    // Size of the input buffer
    TInt iInputBlockSize;
    
    // Size of the output buffer
    TInt iOuputBlockSize;
    
    // Number of channels used in converter
    TInt iChannels;

    // Samplerate to convert from
    TInt iFromSampleRate;
    
    // Samplerate to convert to
    TInt iToSampleRate;
    
    // Number of channels in input
    TInt iFromChannels;
    
    // Number of channels in output
    TInt iToChannels;
    };

#endif
