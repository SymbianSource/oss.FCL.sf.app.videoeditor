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



#ifndef __CPROCProcDecoder_H__
#define __CPROCProcDecoder_H__

#include <e32base.h>
#include <f32file.h>
#include <MMFCodec.h>

#include "AudCommon.h"
#include "ProcConstants.h"
#include "RateConverter.h"

class CProcDecoder : public CBase
    {

public:
    
    /*
    * Symbian constructors
    *
    */
    
    static CProcDecoder* NewL();
                                        
    static CProcDecoder* NewLC();

    /*
    * Destructor
    */
    ~CProcDecoder();
    
    /*
    * Initialize the decoder
    *
    * @param aProperties            properties of the input clip (in compressed domain)
    * @param aTargetSamplingRate    sampling rate to decode to
    * @param aChannelMode           channel mode to decode to
    *
    * @return ETrue if successful
    *
    */

    TBool InitL(TAudFileProperties aProperties, TInt aTargetSamplingRate, TChannelMode aChannelMode);

    /*
    * Feed the encoder
    *
    * @param    aEncFrame   encoded input frame
    * @param    aDecbuffer  buffer for decoded output data
    *
    * @return   ETrue if output was generated, EFalse if more data is needed
    */
        
    TBool FillDecBufferL(const HBufC8* aEncFrame, HBufC8*& aDecBuffer);
    
    

protected:
    
    // constructL    
    void ConstructL();
    
    // C++ constructor
    CProcDecoder();
    
    // configure AAC decoder
    // configure AAC Plus decoder
    void ConfigureAACPlusDecoderL();
    
    // configure MP3 decoder
    void ConfigureMP3DecoderL();
    
    // if input buffer is too small
    void ReAllocBufferL( CMMFDataBuffer* aBuffer, TInt aNewMaxSize );
    
    // feed the codec
    void FeedCodecL( CMMFCodec* aCodec, CMMFDataBuffer* aSourceBuffer, CMMFDataBuffer* aDestBuffer );
    
    // decode
    TCodecProcessResult DecodeL( CMMFCodec* aCodec, CMMFDataBuffer* aInBuffer, CMMFDataBuffer* aOutBuffer);
 
    // is supported dest codec?
    TBool GetIsSupportedSourceCodec();
     
    // set source codec
    void SetSourceCodecL();
     
    // is codec available?
    TBool CheckIfCodecAvailableL(const TDesC8& aCodecFourCCString, const TUid& aCodecUId );
 
    // update output buffer
    TBool UpdateOutputBufferL(CMMFDataBuffer* aInBuffer);
     
 protected:
     
    // properties of the input data
    TAudFileProperties iProperties;
    
    // a flag to indicate if decoding can take place
    TBool iReady;
    
    // Input buffer for decoder.
    CMMFDataBuffer* iSourceInputBuffer;
    
    // Input buffer for encoder (alternative output buffer for decoder).
    CMMFDataBuffer* iDestInputBuffer;
    
    CMMFDataBuffer* iSampleRateChannelBuffer;
        
    // Codec used in decoding input audio to PCM16
    CMMFCodec* iSourceCodec;
    
    // Channel and sample rate converter
    CRateConverter* iRateConverter;    
    
    // Whether audio converter needs to do channel and/or samplerate conversion.
    TBool iDoSampleRateChannelConversion;
    
    // Whether decoding is required (not required if input is WAV)
    TBool iDoDecoding;

    // Samplerate to convert from.
    TInt iFromSampleRate;
    // Samplerate to convert to.
    TInt iToSampleRate;
    // Number of channels in input.
    TInt iFromChannels;
    // Number of channels in output.
    TInt iToChannels;
    
    // a pointer to store output buffer
    HBufC8* iDecBuffer;
   
    // decimation factor for decoder
    TInt iDecimFactor;

    
    };

#endif
