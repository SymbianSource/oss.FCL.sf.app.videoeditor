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
* Resampling framework for YUV 4.2.0.
*
*/



#ifndef CTRSCALER_H
#define CTRSCALER_H


// INCLUDES
#include <e32base.h>
#include "ctrcommon.h"


// Scaler operations
enum EResamplingOpereation
    {
    EOperationNone,
    EDoubleSize,          // Resize to 200%
    EUpSampling,          // Resize to 101% or more
    EOperationCopy,       // No resize (100%)
    EOperationCopyWithBB, // No resize, add black borders
    EDownSampling,        // Resize to 99% or less
    EHalveSize,           // Resize to 50%
    EUpDownSampling       // Resize with strech    
    };

// Scaler class, supports resampling operations with YUV 4.2.0 format;
NONSHARABLE_CLASS(CTRScaler) : public CBase
    {
    public:
        static CTRScaler* NewL();

        // Destructor
        ~CTRScaler();

    public:
        void Scale();

        // Sets operations
        void SetScalerOptionsL( TPtr8& aSrc, TPtr8& aTrg, TSize& aSrcSize, TSize& aTrgSize );
        
        // Checks if aspect ratio is wide
        TBool IsWideAspectRatio(TSize aSize);        
        
        // Calculates intermediate resolution for use with black boxing
        TBool GetIntermediateResolution(TSize aSrcSize, TSize aTrgSize, TSize& aTargetResolution, TSize& aBlackBoxing);
        
        // Returns a time estimate of how long it takes to resample a frame
        TReal EstimateResampleFrameTime(const TTRVideoFormat& aInput, const TTRVideoFormat& aOutput);

    private:
        CTRScaler();

        // Second phase constructor
        void ConstructL();

        // Resampling operations
        void ResampleBilinear(TSize& aSrcSize, TSize& aTrgSize, TSize& aBlackBoxing);
        void ResampleHalve(TSize& aSrcSize, TSize& aTrgSize, TSize& aBlackBoxing);
        void ResampleDouble(TSize& aSrcSize, TSize& aTrgSize);
        
        // Called when resizing to less than 50% of the original size
        void DoHalveAndBilinearResampleL();
        
        // No resampling, copy frame and add black borders.
        void CopyWithBlackBoxing(TSize& aSrcSize, TSize& aTrgSize, TSize& aBlackBoxing);

    private:
        // Source buffer
        TUint8* iSrc;

        // Destination buffer
        TUint8* iTrg;

        // Source resolution
        TSize iSrcSize; 

        // Target resolution
        TSize iTrgSize;
        
        // Resampling operation
        TInt iOperation;

        // Trg data size
        TUint iTrgDataSize;
        
        // Initial src ptr
        TUint8* iSrcInit;

        // Initial trg ptr
        TUint8* iTrgInit;

        // Scale X
        TInt iScaleXInt;

        // Scale Y
        TInt iScaleYInt;
        
        TPtr8* iTrgBuffer;
        
        // For storing width/height of black border area
        TSize iBlackBoxing;
        
    };




#endif
