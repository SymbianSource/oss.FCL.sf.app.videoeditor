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



#include "RateConverter.h"


// CONSTANTS



// MACROS

// Debug print macro
#if defined _DEBUG 
#include <e32svr.h>
#define PRINT(x) RDebug::Print x;
#else
#define PRINT(x)
#endif

// -----------------------------------------------------------------------------
// CRateConverter::NewL
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CRateConverter* CRateConverter::NewL(TInt aFromSampleRate, TInt aToSampleRate, TInt aFromChannels, TInt aToChannels)                          
    {
    CRateConverter* self = NewLC(aFromSampleRate, aToSampleRate, aFromChannels, aToChannels);
    CleanupStack::Pop(self);
    return self;
    }

// -----------------------------------------------------------------------------
// CRateConverter::NewLC
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CRateConverter* CRateConverter::NewLC(TInt aFromSampleRate, TInt aToSampleRate, TInt aFromChannels, TInt aToChannels)                             
    {
    CRateConverter* self = new (ELeave) CRateConverter(aFromSampleRate, aToSampleRate, aFromChannels, aToChannels);
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
    }

// -----------------------------------------------------------------------------
// CRateConverter::ConstructL
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//    
void CRateConverter::ConstructL()
    {
    }

// -----------------------------------------------------------------------------
// CRateConverter::CRateConverter
// C++ default constructor can NOT contain any code, that
// might leave.
// -----------------------------------------------------------------------------
//
CRateConverter::CRateConverter(TInt aFromSampleRate, TInt aToSampleRate, TInt aFromChannels, TInt aToChannels)
 : iFromSampleRate(aFromSampleRate), iToSampleRate(aToSampleRate),
   iFromChannels(aFromChannels), iToChannels(aToChannels)
    {
    // Select the smaller of the two
    iChannels = (iFromChannels < iToChannels) ? iFromChannels : iToChannels;
    }

// ---------------------------------------------------------
// CRateConverter::~CRateConverter
// Destructor
// ---------------------------------------------------------
//
CRateConverter::~CRateConverter()
    {
    if (iChild)
        {
        delete iChild;
        iChild = NULL;
        }
    
    if (iConverter)
        {
    	delete iConverter;
    	iConverter = NULL;
        }
        
    if (iInBuffer)
        {
        for (TInt i = 0; (i < iChannels) && iInBuffer[i]; i++)
            {
    		User::Free(iInBuffer[i]);
    		iInBuffer[i] = NULL;
            }
    	User::Free(iInBuffer);
    	iInBuffer = NULL;
        }
    
    if (iOutBuffer)
        {
        for (TInt i = 0; (i < iChannels) && iOutBuffer[i]; i++)
            {
    		User::Free(iOutBuffer[i]);
    		iOutBuffer[i] = NULL;
            }
    	User::Free(iOutBuffer);
    	iOutBuffer = NULL;
        }
        
    if (iScratchBuffer)
        {
        User::Free(iScratchBuffer);
        iScratchBuffer = NULL;
        }
    }

// -----------------------------------------------------------------------------
// CRateConverter::InitL
// Initialize the rate converter
// -----------------------------------------------------------------------------
//  
TBool CRateConverter::InitL(TInt aInputBufferSize)
    {
    PRINT((_L("CRateConverter::InitL() In")));
    
    if (iConverter)
        {
        PRINT((_L("CRateConverter::InitL() Already initialized")));
        return EFalse;
        }
    
    // Check that the input and output are either mono or stereo    
    if ( ((iFromChannels != 1) && (iFromChannels != 2)) ||
         ((iToChannels != 1) && (iToChannels != 2)) )
        {
        PRINT((_L("CRateConverter::InitL() Only mono and stereo are supported for input/output")));
        return EFalse;
        }
        
    if (iFromSampleRate == iToSampleRate)
        {
        // No sample rate conversion needed so do only channel conversion
        iInputBlockSize = iOuputBlockSize = aInputBufferSize;
        
        PRINT((_L("CRateConverter::InitL() Out")));
        return ETrue;
        }
        
    if (!DoInitL(aInputBufferSize))
        {
        return EFalse;
        }
    
    // Allocate internal input buffer
    iInBuffer = (TInt16**) User::AllocL(iChannels * sizeof(TInt16*));
    
    for (TInt i = 0; i < iChannels; i++)
        {
        iInBuffer[i] = (TInt16*) User::AllocL(iInputBlockSize * sizeof(TInt16));
        }
        
    PRINT((_L("CRateConverter::InitL() Out")));     
    return ETrue;
    }

// -----------------------------------------------------------------------------
// CRateConverter::DoInitL
// Does internal initialization
// -----------------------------------------------------------------------------
//    
TBool CRateConverter::DoInitL(TInt aInputBufferSize)
    {
    iInputBlockSize = aInputBufferSize;
    
    // Supported direct conversions:
    //
    // 16000 ->  8000: /2
    // 24000 ->  8000: /3
    //
    // 8000  -> 16000: *2
    // 32000 -> 16000: /2
    // 48000 -> 16000: /3
    //
    // 16000 -> 48000: *3
    // 24000 -> 48000: *2
    // 32000 -> 48000: *3/2
    // 44100 -> 48000: *i
    
    // Try to use direct conversion
    iConverter = RESAMPLER_RateConversionInputDrivenInt16::New(iFromSampleRate, iToSampleRate, iChannels);
    
    if (!iConverter)
        {
        // Direct conversion is not possible so multi phase conversion is needed

        // Conversions are done in the following order:
        // (*i means 160/147 conversion)
        //
        // 11025 ->  8000: *i *2 /3    (three phase)
        // 22050 ->  8000: *i /3       (two phase)
        // 32000 ->  8000: /2 /2       (two phase)
        // 44100 ->  8000: *i /3 /2    (three phase)
        // 48000 ->  8000: /3 /2       (two phase)
        // 
        // 11025 -> 16000: *2 *2 *i /3 (four phase)
        // 22050 -> 16000: *2 *i /3    (three phase)
        // 24000 -> 16000: *2 /3       (two phase)
        // 44100 -> 16000: *i /3       (two phase)
        // 
        // 8000  -> 48000: *3 *2       (two phase)
        // 11025 -> 48000: *2 *2 *i    (three phase)
        // 22050 -> 48000: *2 *i       (two phase)
        
        // Check the last phase in the chain and make the decision where the child should convert
        if( ((iToSampleRate == 8000) && ((iFromSampleRate == 11025) || (iFromSampleRate == 22050))) ||
            (iToSampleRate == 16000) )
            {
            // Last phase is /3 so the child converter needs to do
            // conversion from iFromSampleRate to iToSampleRate*3
            
            // Create the child converter
            iChild = CRateConverter::NewL(iFromSampleRate, iToSampleRate * 3, iChannels, iChannels);
            if (!iChild->DoInitL(aInputBufferSize))
                {
                return EFalse;
                }
            
            // Update sample rates and buffer sizes
            iFromSampleRate = iToSampleRate * 3;
            aInputBufferSize = iChild->GetOutputBufferSize();
        
            // Try to create our converter  
            iConverter = RESAMPLER_RateConversionInputDrivenInt16::New(iFromSampleRate, iToSampleRate, iChannels);
            if (!iConverter)
                {
                return EFalse;
                }
            }
        else if( (iToSampleRate == 8000) )
            {
            // Last phase is /2 so the child converter needs to do
            // conversion from iFromSampleRate to iToSampleRate*2
            
            // Create the child converter
            iChild = CRateConverter::NewL(iFromSampleRate, iToSampleRate * 2, iChannels, iChannels);
            if (!iChild->DoInitL(aInputBufferSize))
                {
                return EFalse;
                }
            
            // Update sample rates and buffer sizes
            iFromSampleRate = iToSampleRate * 2;
            aInputBufferSize = iChild->GetOutputBufferSize();
            
            // Try to create our converter  
            iConverter = RESAMPLER_RateConversionInputDrivenInt16::New(iFromSampleRate, iToSampleRate, iChannels);
            if (!iConverter)
                {
                return EFalse;
                }
            }
        else if( ((iToSampleRate == 48000) && ((iFromSampleRate == 11025) || (iFromSampleRate == 22050))) )
            {
            // Last phase is *i so the child converter needs to do
            // conversion from iFromSampleRate to 44100
            
            // Create the child converter
            iChild = CRateConverter::NewL(iFromSampleRate, 44100, iChannels, iChannels);
            if (!iChild->DoInitL(aInputBufferSize))
                {
                return EFalse;
                }
            
            // Update sample rates and buffer sizes
            iFromSampleRate = 44100;
            aInputBufferSize = iChild->GetOutputBufferSize();
            
            // Try to create our converter  
            iConverter = RESAMPLER_RateConversionInputDrivenInt16::New(iFromSampleRate, iToSampleRate, iChannels);
            if (!iConverter)
                {
                return EFalse;
                }
            }
        else if( ((iFromSampleRate == 11025) && ((iToSampleRate == 24000) || (iToSampleRate == 44100))) ||
                 ((iFromSampleRate == 8000) && (iToSampleRate == 48000)) )
            {
            // Last phase is *2 so the child converter needs to do
            // conversion from iFromSampleRate to iToSampleRate/2
            
            // Create the child converter
            iChild = CRateConverter::NewL(iFromSampleRate, iToSampleRate / 2, iChannels, iChannels);
            if (!iChild->DoInitL(aInputBufferSize))
                {
                return EFalse;
                }
                
            // Update sample rates and buffer sizes
            iFromSampleRate = iToSampleRate / 2;
            aInputBufferSize = iChild->GetOutputBufferSize();
            
            // Try to create our converter  
            iConverter = RESAMPLER_RateConversionInputDrivenInt16::New(iFromSampleRate, iToSampleRate, iChannels);
            if (!iConverter)
                {
                return EFalse;
                }
            }
        else
            {
            // We don't know how to convert, probably this is an unsupported conversion
            PRINT((_L("CRateConverter::DoInitL() Can not convert from %d to %d"), iFromSampleRate, iToSampleRate));
            return EFalse;
            }
        }
        
    if (!iConverter->InitInputDriven())
        {
        PRINT((_L("CRateConverter::InitL() Failed to initialize converter")));
        return EFalse;
        }
    
    // Set scratch memory buffer for converter    
    size_t scratchBufferSize = iConverter->ScratchMemoryNeedInputDriven(aInputBufferSize);
    
    if (scratchBufferSize == 0)
        {
        PRINT((_L("CRateConverter::InitL() Scratch buffer size is too big")));
        return EFalse;
        }
        
    iScratchBuffer = (TInt8*) User::AllocL(scratchBufferSize);
    iConverter->SetScratchBufferInputDriven((char *)iScratchBuffer);
        
    iOuputBlockSize = iConverter->MaxOutputSampleCount(aInputBufferSize);
    
    // Allocate internal output buffer
    iOutBuffer = (TInt16**) User::AllocL(iChannels * sizeof(TInt16*));
    
    for (TInt i = 0; i < iChannels; i++)
        {
        iOutBuffer[i] = (TInt16*) User::AllocL(iOuputBlockSize * sizeof(TInt16));
        }
        
    iConverter->SetQualityInputDriven(RESAMPLER_RATE_CONVERSION_QUALITY_STANDARD);
    
    PRINT((_L("CRateConverter::DoInitL() Convert %d -> %d Hz, %d -> %d channels"), iFromSampleRate, iToSampleRate, iFromChannels, iToChannels));
    
    PRINT((_L("CRateConverter::DoInitL() Input buffer %d, Output buffer %d"), iInputBlockSize, iOuputBlockSize));
    
    return ETrue;
    }

// -----------------------------------------------------------------------------
// CRateConverter::ConvertBufferL
// Does rate and channel conversion for given buffer
// -----------------------------------------------------------------------------
//    
TInt CRateConverter::ConvertBufferL(TInt16* aInput, TInt16* aOutput, TInt aInputSampleCount)
    {
    if (iFromSampleRate == iToSampleRate)
        {
        // No sample rate conversion needed
        if (iFromChannels == iToChannels)
            {
            // No channel conversion needed either so copy input directly to output
            Mem::Copy(aOutput, aInput, aInputSampleCount * 2 * iFromChannels);
            }    
        else if (iFromChannels == 2)
            {
            // Convert stereo to mono
            for (TInt i = 0; i < aInputSampleCount; ++i)
                {
                // Average left and right samples
                aOutput[i] = (aInput[2*i + 0] + aInput[2*i + 1]) >> 1;
                }
            }
        else if (iToChannels == 2)
            {
            // Convert mono to stereo
            for (TInt i = 0; i < aInputSampleCount; ++i)
                {
                // Duplicate left channel to right channel
                aOutput[2*i + 0] = aInput[i];
                aOutput[2*i + 1] = aInput[i];
                }
            }
        
        return aInputSampleCount;
        }
    
    if (!iConverter)
        {
        PRINT((_L("CRateConverter::ConvertBufferL() Not initialized")));
        User::Leave(KErrNotReady);
        }
    
    if (aInputSampleCount > iInputBlockSize)
        {
        PRINT((_L("CRateConverter::ConvertBufferL() Too many input samples")));
        User::Leave(KErrArgument);
        }
    
    // Copy to input buffers and do channel conversion if needed    
    if (iChannels == 2)
        {
        // Both channels are stereo so copy both channels to own buffers
        for (TInt i = 0; i < aInputSampleCount; ++i)
            {
            iInBuffer[0][i] = aInput[2*i + 0];
            iInBuffer[1][i] = aInput[2*i + 1];
            }
        }
    else if (iFromChannels == 2)
        {
        // Source is stereo so convert stereo to mono
        for (TInt i = 0; i < aInputSampleCount; ++i)
            {
            // Average left and right samples
            iInBuffer[0][i] = (aInput[2*i + 0] + aInput[2*i + 1]) >> 1;
            }
        }
    else
        {
        // Source is mono so copy it directly
        Mem::Copy(iInBuffer[0], aInput, aInputSampleCount * 2);
        }
        
    TInt outputSampleCount = DoConvertL(iInBuffer, aInputSampleCount);
    
    // Copy to output buffers and do channel conversion if needed    
    if (iChannels == 2)
        {
        // Both channels are stereo so copy both channels to own buffers
        for (TInt i = 0; i < outputSampleCount; ++i)
            {
            aOutput[2*i + 0] = iOutBuffer[0][i];
            aOutput[2*i + 1] = iOutBuffer[1][i];
            }
        }
    else if (iToChannels == 2)
        {
        // Ouput is stereo so convert mono to stereo
        for (TInt i = 0; i < outputSampleCount; ++i)
            {
            // Duplicate left channel to right channel
            aOutput[2*i + 0] = iOutBuffer[0][i];
            aOutput[2*i + 1] = iOutBuffer[0][i];
            }
        }
    else
        {
        // Output is mono so copy it directly
        Mem::Copy(aOutput, iOutBuffer[0], outputSampleCount * 2);
        }
        
    PRINT((_L("CRateConverter::ConvertBufferL() Output %d samples"), outputSampleCount));
         
    return outputSampleCount;
    }

// -----------------------------------------------------------------------------
// CRateConverter::DoConvertL
// Does the actual conversion
// -----------------------------------------------------------------------------
//     
TInt CRateConverter::DoConvertL(TInt16** aInput, TInt aInputSampleCount)
    {
    if (iChild)
        {
        // If we have a child then we need to do a multi phase conversion
        TInt tempSampleCount = iChild->DoConvertL(aInput, aInputSampleCount);
        
        // Get pointer to child's output and use it as an input
        TInt16** tempBuf = iChild->GetOutputBuffer();
        
        return iConverter->ProcessFromInput(iOutBuffer, tempBuf, tempSampleCount);
        }
    else
        {
        // Otherwise process directly from input to output
        return iConverter->ProcessFromInput(iOutBuffer, aInput, aInputSampleCount);
        }
    }
    
    
