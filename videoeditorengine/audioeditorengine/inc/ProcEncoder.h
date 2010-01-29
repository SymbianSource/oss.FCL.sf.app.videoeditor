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



#ifndef __CPROCENCODER_H__
#define __CPROCENCODER_H__

#include <e32base.h>
#include <f32file.h>
#include <MMFCodec.h>

#include "AudCommon.h"
#include "ProcConstants.h"

class CProcEncoder : public CBase
    {

public:
    
    /*
    * Symbian constructors
    */
    
    static CProcEncoder* NewL();
                                        
    static CProcEncoder* NewLC();

    /*
    * Destructor
    */
    ~CProcEncoder();

    
    /*
    * Initializes encoder
    *
    * @param aAudioType             target audio type, EAudAMR and EAudAAC_MPEG4 supported
    * @param aTargetSamplingRate    target sampling rate, 8000, 16000 supported, later 48000
    * @param aChannelMode           target channel mode EAudSingleChannel supported, stereo later
    *
    * @return   ETrue if successful, EFalse if not
    *
    */
    
    TBool InitL(TAudType aAudioType, TInt aTargetSamplingRate, TChannelMode aChannelMode, TInt aBitrate);
    
    /*
    * Feed the encoder
    * If the encoder has enough data, output is written to aEncBuffer
    *
    * @param aRawFrame             input for the encoder, must have the same sample rate
    *                              and channel mode as the desired target
    * @param aEncBuffer            buffer for output
    * @param aOutputDurationMilli  duration of the output buffer in return
    *
    * @return   ETrue if data was written to aEncBuffer, EFalse otherwise
    *
    */
    TBool FillEncBufferL(const TDesC8& aRawFrame, HBufC8* aEncBuffer, TInt& aOutputDurationMilli);
    
    /*
    * Returns the destination audio type
    *
    * @return destination audio type
    */
    TAudType DestAudType();
    
protected:
    
    // constructL    
    void ConstructL();
    
    // C++ constructor
    CProcEncoder();
    
    // reallocates a data buffer
    void ReAllocBufferL( CMMFDataBuffer* aBuffer, TInt aNewMaxSize );
    
    // feed codec
    void FeedCodecL( CMMFCodec* aCodec, CMMFDataBuffer* aSourceBuffer, CMMFDataBuffer* aDestBuffer );
    
    // encodeL
    TCodecProcessResult EncodeL( CMMFCodec* aCodec, CMMFDataBuffer* aInBuffer, CMMFDataBuffer* aOutBuffer);
 
    // is the destination codec supported?
    TBool GetIsSupportedDestCodec();
     
    // configure AMR encoder
    void ConfigureAMREncoderL();
    
    // configure AAC encoder
    void ConfigureAACEncoderL();
    
    // set destination codec
    void SetDestCodecL();
     
    // is codec available?
    TBool CheckIfCodecAvailableL(const TDesC8& aCodecFourCCString, const TUid& aCodecUId);
    
protected:    

    // target audio type
    TAudType iAudioType;
    
    // is encoder ready for processing
    TBool iReady;
    
    // Input buffer for raw data
    CMMFDataBuffer* iSourceInputBuffer;
    
    // Input buffer for encoder (alternative output buffer for decoder).
    CMMFDataBuffer* iDestInputBuffer;
        
    // Codec used in decoding input audio to PCM16
    CMMFCodec* iDestCodec;
    
    // Samplerate to encode to.
    TInt iToSampleRate;
    // Number of channels in output and input
    TInt iToChannels;
    
    // target bitrate
    TInt iToBitRate;
    
    // output duration
    TInt iOutputFrameDurationMilli;
    
    // sometimes more than one AMR frame are written to the output with one call
    TInt iNumberOfFramesInOutputBuffer;
    
    // buffer for output
    HBufC8* iEncBuffer;
      
    };

#endif
