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



// INCLUDES
#include "ctrscaler.h"
#include "ctrsettings.h"
#include "ctrhwsettings.h"
#include <e32math.h>


// Debug print macro
#ifdef _DEBUG
    #include <e32svr.h>
    #define PRINT(x) RDebug::Print x;
#else
    #define PRINT(x)
#endif


// An assertion macro wrapper to clean up the code a bit
#define VPASSERT(x) __ASSERT_DEBUG(x, User::Panic(_L("CTRScaler"), KErrAbort))


// Macros for fixed point math
#define FP_BITS         15      // Number of bits to use for FP decimals
#define FP_FP(x)        (static_cast<TInt>((x) * 32768.0))
#define FP_ONE          (1 << FP_BITS)
#define FP_MUL(x,y)     (((x) * (y)) >> FP_BITS)
#define FP_FRAC(x)      ((x) & (FP_ONE - 1))
#define FP_INT(x)       ((x) >> FP_BITS)


// ============================ MEMBER FUNCTIONS ===============================

// -----------------------------------------------------------------------------
// CTRScaler::NewL
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CTRScaler* CTRScaler::NewL()
    {
    // Standard two phase construction
    CTRScaler* self = new (ELeave) CTRScaler();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();

    return self;
    }


// -----------------------------------------------------------------------------
// CTRScaler::ConstructL
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//
void CTRScaler::ConstructL()
    {
    }


// -----------------------------------------------------------------------------
// CTRScaler::CTRScaler
// C++ default constructor can NOT contain any code, that
// might leave.
// -----------------------------------------------------------------------------
CTRScaler::CTRScaler()
    {
    // Scaler does not perform any operation before initializing
    iOperation = EOperationNone;
    iTrgBuffer = NULL;
    }


// ---------------------------------------------------------
// CTRScaler::~CTRScaler()
// Destructor
// ---------------------------------------------------------
//
CTRScaler::~CTRScaler()
    {
    }

// ---------------------------------------------------------
// CTRScaler::IsWideAspectRatio()
// Checks if aspect ratio is wide
// ---------------------------------------------------------
//
TBool CTRScaler::IsWideAspectRatio(TSize aSize)
    {
    return ( TReal(aSize.iWidth) / TReal(aSize.iHeight) ) > KTRWideThreshold;
    }

// -----------------------------------------------------------------------------
// CTRScaler::SetupScalerL
// Sets the scaler options (src buffer, dest buffer (could be the same as a src), src resolution, trg resolution)
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRScaler::SetScalerOptionsL(TPtr8& aSrc, TPtr8& aTrg, TSize& aSrcSize, TSize& aTrgSize )
    {
    
    
    PRINT((_L("CTRScaler::SetScalerOptionsL, src = (%d, %d), trg = (%d, %d)"), 
        aSrcSize.iWidth, aSrcSize.iHeight, aTrgSize.iWidth, aTrgSize.iHeight));
    
    // Check settings
    if ( ( !aSrc.Ptr() ) || ( !aTrg.Ptr() ) || 
       ( aSrcSize.iWidth == 0) || ( aSrcSize.iHeight == 0 )   || 
       ( aTrgSize.iWidth == 0) || ( aTrgSize.iHeight == 0 )   ||
       ( aSrc.MaxLength() < ( aSrcSize.iWidth * aSrcSize.iHeight * 3 / 2 ) ) || 
       ( aTrg.MaxLength() < ( aTrgSize.iWidth * aTrgSize.iHeight * 3 / 2 ) )
        )
        {
        PRINT((_L("CTRScaler::SetupScalerL(), Given options are not supported")))
        User::Leave(KErrNotSupported);
        }
    else
        {
        TReal remainder = 0.0;
        iTrgBuffer = NULL;

        // We don't support non-multiple output yet
        Math::Mod( remainder, static_cast<TReal>(aTrgSize.iWidth), 4.0 );

        if ( remainder == 0.0 )
            {
            Math::Mod( remainder, static_cast<TReal>(aTrgSize.iHeight), 4.0 );
            }

        if ( remainder != 0.0 )
            {
            PRINT((_L("CTRScaler::SetupScalerL(), Scaler does not support output resolution that is not multiple by 4")))
            User::Leave(KErrNotSupported);
            }
            
        TSize targetSize = aTrgSize;
        // check if black boxing is needed
        TBool srcWide = IsWideAspectRatio(aSrcSize);
        TBool dstWide = IsWideAspectRatio(aTrgSize);
        
        iBlackBoxing = TSize(0,0);
        
        TBool doScaling = ETrue;
        if (srcWide != dstWide)
            {
                TSize resolution(0,0);
            
                doScaling = GetIntermediateResolution(aSrcSize, aTrgSize, resolution, iBlackBoxing);
            
                // Set the whole image to black
                TUint yLength = aTrgSize.iWidth * aTrgSize.iHeight;
		        TUint uvLength = yLength >> 1;
                
                // Y
        		TInt data = 0;
        		TPtr8 tempPtr(0,0);        		
        		tempPtr.Set(const_cast<TUint8*>(aTrg.Ptr()), yLength, yLength);        		
        		tempPtr.Fill((TChar)data, yLength);

        		// U,V        		
        		data = 127;
        		tempPtr.Set(const_cast<TUint8*>(aTrg.Ptr()) + yLength, uvLength, uvLength); 
        		tempPtr.Fill((TChar)data, uvLength);                

                aTrgSize = resolution;
                PRINT((_L("CTRScaler::SetScalerOptionsL, blackboxing width = %d, height = %d"), iBlackBoxing.iWidth, iBlackBoxing.iHeight));
            }
         
        if ( !doScaling )
            {
            // No need to perform resampling operation, copy data with black boxing
            iOperation = EOperationCopyWithBB;
            }        
        
        else if ( (aTrgSize.iWidth == aSrcSize.iWidth) && (aTrgSize.iHeight == aSrcSize.iHeight) )
            {
            // No need to perform resampling operation, just copy data
            iOperation = EOperationCopy;
            }
        else if ( (aTrgSize.iWidth == aSrcSize.iWidth * 2) && (aTrgSize.iHeight == aSrcSize.iHeight * 2) )
            {
            // Resolution is doubled
            iOperation = EDoubleSize;
            }
        else if ( (aTrgSize.iWidth * 2 == aSrcSize.iWidth) && (aTrgSize.iHeight * 2 == aSrcSize.iHeight) )
            {
            // Resolution is halved
            iOperation = EHalveSize;
            }
        else if ( (aTrgSize.iWidth > aSrcSize.iWidth) && (aTrgSize.iHeight > aSrcSize.iHeight) )
            {
            // Resolution is increased
            iOperation = EUpSampling;
            }
        else if ( (aTrgSize.iWidth < aSrcSize.iWidth) && (aTrgSize.iHeight < aSrcSize.iHeight) )
            {
            // Resolution is decreased
            iOperation = EDownSampling;
            }
        else
            {
            // The image is streched ie. vertical resolution increases and horizontal decreases or vice versa
            iOperation = EUpDownSampling;
            }

        // Set given settings
        iSrc = const_cast<TUint8*>( aSrc.Ptr() );
        iTrg = const_cast<TUint8*>( aTrg.Ptr() );
        iSrcSize = aSrcSize;
        iTrgSize = aTrgSize;
        iSrcInit = iSrc;
        iTrgInit = iTrg;
        aTrgSize = targetSize;  // recover target size since it's a reference
        iTrgDataSize = aTrgSize.iWidth * aTrgSize.iHeight * 3 / 2;
        iTrgBuffer = &aTrg;
        }
    }
    
// ---------------------------------------------------------
// CTRScaler::GetIntermediateResolution()
// Calculates intermediate resolution for use with black boxing
// ---------------------------------------------------------
//
TBool CTRScaler::GetIntermediateResolution(TSize aSrcSize, TSize aTrgSize, 
                                           TSize& aTargetResolution, TSize& aBlackBoxing)
    {

    TSize resolution;
    TBool doScaling = ETrue;
       
    TBool srcWide = IsWideAspectRatio(aSrcSize);
    TBool dstWide = IsWideAspectRatio(aTrgSize);
    
    VPASSERT(srcWide != dstWide);
    
    if (dstWide)
        {
        // Pillarboxing
        
        // scale height to destination
        TReal factor = TReal(aTrgSize.iHeight) / TReal(aSrcSize.iHeight);
        
        resolution.iWidth = TInt( aSrcSize.iWidth * factor );
        
        if (resolution.iWidth & 0x1 > 0)
            resolution.iWidth++;
        
        resolution.iHeight = aTrgSize.iHeight;
        
        while ( (aTrgSize.iWidth - resolution.iWidth) % 4 != 0 )
        {
            resolution.iWidth += 2;
        }

        aBlackBoxing.iWidth = (aTrgSize.iWidth - resolution.iWidth) / 2;
        
        if ( factor == 1.0 )
            {
            // source and destination heights are the same, 
            // meaning source width is smaller and we don't
            // have to scale, just do pillarboxing
            doScaling = EFalse;

            // set target width
            resolution.iWidth = aTrgSize.iWidth;
            }
                        
        }
    else
        {
        // Letterboxing
    
        // scale width to destination
        TReal factor = TReal(aTrgSize.iWidth) / TReal(aSrcSize.iWidth);                                
                    
        resolution.iHeight = TInt( aSrcSize.iHeight * factor );                

        if (resolution.iHeight & 0x1 > 0)
            resolution.iHeight++;
        
        resolution.iWidth = aTrgSize.iWidth;
        
        while ( (aTrgSize.iHeight - resolution.iHeight) % 4 != 0 )
            {
            resolution.iHeight += 2;
            }                                    
        
        aBlackBoxing.iHeight = (aTrgSize.iHeight - resolution.iHeight) / 2;
        
        if ( factor == 1.0 )
            {
            // source and destination widths are the same, 
            // meaning source height is smaller and we don't
            // have to scale, just do letterboxing
            doScaling = EFalse;

            // set target height
            resolution.iHeight = aTrgSize.iHeight;
            }
        }

    PRINT((_L("CTRScaler::GetIntermediateResolution, resolution = (%d, %d), bb = (%d, %d)"), 
        resolution.iWidth, resolution.iHeight, aBlackBoxing.iWidth, aBlackBoxing.iHeight));

    aTargetResolution = resolution;
    
    return doScaling;
    

}


// -----------------------------------------------------------------------------
// CTRScaler::Scale()
// Scale the image
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CTRScaler::Scale()
    {
    TSize srcSizeUV = TSize( iSrcSize.iWidth / 2, iSrcSize.iHeight / 2 );
    TSize trgSizeUV = TSize( iTrgSize.iWidth / 2, iTrgSize.iHeight / 2 );
    TSize blackBoxingUV = TSize( iBlackBoxing.iWidth / 2, iBlackBoxing.iHeight / 2 );
    
    switch( iOperation )
        {
        case EOperationCopy:
            {
            // Src / Trg resolutions are the same, no needs to perform resampling
            if ( iSrc != iTrg )
                {
                    // Copy data, if different memory areas are specified
                    Mem::Copy( iTrg, iSrc, iTrgDataSize );            
                }
            else
                {
                // The same memory fragment is specified for the output; Keep it without changes;
                }
            }
            break;
            
        case EOperationCopyWithBB:
            {                
            // Copy with black boxing
            CopyWithBlackBoxing(iSrcSize, iTrgSize, iBlackBoxing);
            CopyWithBlackBoxing(srcSizeUV, trgSizeUV, blackBoxingUV);
            CopyWithBlackBoxing(srcSizeUV, trgSizeUV, blackBoxingUV);
            }
            break;

        case EDownSampling:
            {
            TInt error = KErrNoMemory;
            
            // If scaling to less than 50% of the source size
            if ( (iTrgSize.iWidth * 2 < iSrcSize.iWidth) && (iTrgSize.iHeight * 2 < iSrcSize.iHeight) )
                {
                // Try to do the scaling in two steps
                TRAP( error, DoHalveAndBilinearResampleL() );
                }
               
            // If the above failed or scaling to 51% or higher        
            if ( error != KErrNone )
                {
                // Resample the Y, U & V components
                ResampleBilinear(iSrcSize, iTrgSize, iBlackBoxing);
                ResampleBilinear(srcSizeUV, trgSizeUV, blackBoxingUV);
                ResampleBilinear(srcSizeUV, trgSizeUV, blackBoxingUV);
                }
            }
            break;
            
        case EUpSampling:
        case EUpDownSampling:
            {            
            // Resample the Y, U & V components
            ResampleBilinear(iSrcSize, iTrgSize, iBlackBoxing);
            ResampleBilinear(srcSizeUV, trgSizeUV, blackBoxingUV);
            ResampleBilinear(srcSizeUV, trgSizeUV, blackBoxingUV);
            }
            break;
            
        case EDoubleSize:
            {
            // Resample the Y, U & V components to double size
            ResampleDouble(iSrcSize, iTrgSize);
            ResampleDouble(srcSizeUV, trgSizeUV);
            ResampleDouble(srcSizeUV, trgSizeUV);
            }
            break;
            
        case EHalveSize:
            {
            // Resample the Y, U & V components to half size
            ResampleHalve(iSrcSize, iTrgSize, iBlackBoxing);
            ResampleHalve(srcSizeUV, trgSizeUV, blackBoxingUV);
            ResampleHalve(srcSizeUV, trgSizeUV, blackBoxingUV);
            }
            break;

        case EOperationNone:
            {
            PRINT((_L("CTRScaler::Scale(), Scaler was not initialized yet to perform any operation")))
            return;
            }
//            break;

        default:
            {
            }
        }
        
    // Recover source and target data pointers
    iSrc = iSrcInit;
    iTrg = iTrgInit;
    
    // Set Dsc length
    if (iTrgBuffer)
        {
        iTrgBuffer->SetLength(iTrgDataSize);
        }
    }

// -----------------------------------------------------------------------------
// CTRScaler::CopyWithBlackBoxing()
// Copies frame to target buffer applying black borders
// -----------------------------------------------------------------------------
//       
void CTRScaler::CopyWithBlackBoxing(TSize& aSrcSize, TSize& aTrgSize, TSize& aBlackBoxing)
{

    if (aBlackBoxing.iHeight != 0)
        {
        
        TInt copyLength = aSrcSize.iWidth * aSrcSize.iHeight;
        
        iTrg += aBlackBoxing.iHeight * aTrgSize.iWidth;
        Mem::Copy(iTrg, iSrc, copyLength);
        
        iTrg += copyLength;
        iTrg += aBlackBoxing.iHeight * aTrgSize.iWidth;                                        
        iSrc += copyLength;
        
        } 
                    
    else if (aBlackBoxing.iWidth != 0)
        {
                                                
        TInt i;
        iTrg += aBlackBoxing.iWidth;
        
        for (i = 0; i < iTrgSize.iHeight; i++)
            {
            // copy one row
            Mem::Copy(iTrg, iSrc, aSrcSize.iWidth);
            iSrc += aSrcSize.iWidth;
            iTrg += aSrcSize.iWidth;
            iTrg += aBlackBoxing.iWidth * 2;
            }
            
        // subtract the width of one pillar
        iTrg -= aBlackBoxing.iWidth;
            
        }
}


// -----------------------------------------------------------------------------
// CTRScaler::DoHalveAndBilinearResampleL()
// First resamples an image to half size and then uses bilinear resample to
// scale it to requested size.
// -----------------------------------------------------------------------------
//    
void CTRScaler::DoHalveAndBilinearResampleL()
    {
    // Make sure the scale factor is correct
    VPASSERT( (iTrgSize.iWidth * 2 < iSrcSize.iWidth) &&
              (iTrgSize.iHeight * 2 < iSrcSize.iHeight) );
    
    TSize srcSizeUV = TSize( iSrcSize.iWidth / 2, iSrcSize.iHeight / 2 );
    TSize trgSizeUV = TSize( iTrgSize.iWidth / 2, iTrgSize.iHeight / 2 );
    TSize blackBoxingUV = TSize( iBlackBoxing.iWidth / 2, iBlackBoxing.iHeight / 2 );
    
    // Calculate the size for the temporary image where we store the intermediate result         
    TSize tempSize = TSize( iSrcSize.iWidth / 2, iSrcSize.iHeight / 2 );
    TSize tempSizeUV = TSize( tempSize.iWidth / 2, tempSize.iHeight / 2 );
    
    // Allocate memory for the temporary image
    TUint8* tempBuffer = (TUint8*) User::AllocLC(tempSize.iWidth * tempSize.iHeight * 3 / 2);
    
    // Set the temporary image as the target
    iTrg = tempBuffer;

    TSize zeroBlackBox = TSize(0,0);
    // Resample the Y, U & V components to half size
    ResampleHalve(iSrcSize, tempSize, zeroBlackBox);
    ResampleHalve(srcSizeUV, tempSizeUV, zeroBlackBox);
    ResampleHalve(srcSizeUV, tempSizeUV, zeroBlackBox);
    
    // Set the temporary image as the source and recover the original target
    iSrc = tempBuffer;
    iTrg = iTrgInit;
    
    // Resample the Y, U & V components
    ResampleBilinear(tempSize, iTrgSize, iBlackBoxing);
    ResampleBilinear(tempSizeUV, trgSizeUV, blackBoxingUV);
    ResampleBilinear(tempSizeUV, trgSizeUV, blackBoxingUV);
    
    // Release the temporary buffer
    CleanupStack::PopAndDestroy(tempBuffer);
    }

// -----------------------------------------------------------------------------
// CTRScaler::ResampleBilinear()
// Resamples an image with bilinear filtering. The target pixel is generated by
// linearly interpolating the four nearest source pixels in x- and y-directions.
// -----------------------------------------------------------------------------
//    
void CTRScaler::ResampleBilinear(TSize& aSrcSize, TSize& aTrgSize, TSize& aBlackBoxing)
    {
    TInt i = 0, j = 0;
    TInt x = 0, y = 0;
    TInt fx = 0, fy = 0;
    TInt weightFactor = 0;
    
    // Pointers to the source memory
    TUint8* srcRowPosition = 0;
    TUint8* srcPixelPosition = 0;
    
    // Calculate the scale factor using the max indices of the source and target images
    TReal scaleX = TReal(aSrcSize.iWidth - 1) / TReal(aTrgSize.iWidth - 1);
    TReal scaleY = TReal(aSrcSize.iHeight - 1) / TReal(aTrgSize.iHeight - 1);
    
    // Convert the scale factor to fixed point
    iScaleXInt = FP_FP(scaleX) - 1;     // subtract 1 so we don't go outside the source image
    iScaleYInt = FP_FP(scaleY) - 1;
        
    if ( aBlackBoxing.iWidth != 0 )
        {
        // increment target pointer over first 'pillar'
        iTrg += aBlackBoxing.iWidth;
        }
    else if ( aBlackBoxing.iHeight != 0 )
        {
        // increment target pointer over top letterboxed area
        iTrg += aTrgSize.iWidth * aBlackBoxing.iHeight;
        }

    // Loop target rows
    for( i = 0, y = 0; i < aTrgSize.iHeight; i++ )
        {
        // Calculate the row position of the source        
        srcRowPosition = iSrc + FP_INT(y) * aSrcSize.iWidth;
        
        fy = FP_FRAC(y);    // Fractational part of the row position
         
        // Loop target columns
        for( j = 0, x = 0; j < aTrgSize.iWidth; j++ )
            {
            // Calculate the pixel position in the source            
            srcPixelPosition = srcRowPosition + FP_INT(x);
            
            // Calculate the weight factor for blending
            fx = FP_FRAC(x); 
            weightFactor = FP_MUL(fx, fy);
            
            // Blend using the four nearest pixels
            *(iTrg) = FP_INT(
                *(srcPixelPosition) * (weightFactor + FP_ONE - fx - fy) + 
                *(srcPixelPosition + 1) * (fx - weightFactor) + 
                *(srcPixelPosition + aSrcSize.iWidth) * (fy - weightFactor) +
                *(srcPixelPosition + 1 + aSrcSize.iWidth) * weightFactor );
            
            iTrg++;             // Move on to the next target pixel
            x += iScaleXInt;    // Calculate the column for the next source pixel
            }

        y += iScaleYInt;        // Calculate the row for the next source pixels
        
        if ( aBlackBoxing.iWidth != 0 )
            {
            // increment target pointer over two pillars, one at the end of this row, 
            // other one at the beginning of the next row
            iTrg += aBlackBoxing.iWidth * 2;
            }
        }

    // Update pointers 
    iSrc += aSrcSize.iWidth * aSrcSize.iHeight;
    
    if ( aBlackBoxing.iWidth != 0 )
        {
        // subtract the width of one pillar
        iTrg -= aBlackBoxing.iWidth;
        }
    else if ( aBlackBoxing.iHeight != 0 )
        {
        // increment over bottom letterboxed area
        iTrg += aBlackBoxing.iHeight * aTrgSize.iWidth;
        }
        
    }

// -----------------------------------------------------------------------------
// CTRScaler::ResampleHalve()
// Resamples an image to half size. For each target pixel a 2x2 pixel area is
// read from the source and blended together to produce the target color.
// -----------------------------------------------------------------------------
//    
void CTRScaler::ResampleHalve(TSize& aSrcSize, TSize& aTrgSize, TSize& aBlackBoxing)
    {
    TInt i = 0, j = 0;
    
    // Make sure the scale factor is correct
    VPASSERT( (aTrgSize.iWidth * 2 == aSrcSize.iWidth) &&
              (aTrgSize.iHeight * 2 == aSrcSize.iHeight) );
              
    if ( aBlackBoxing.iHeight != 0 )
        {
        // increment target pointer over top letterboxed area
        iTrg += aTrgSize.iWidth * aBlackBoxing.iHeight;
        }          

    // Loop target rows
    for( i = 0; i < aTrgSize.iHeight; i++ )
        {
        // Loop target columns    
        for( j = 0; j < aTrgSize.iWidth; j++ )
            {
            // Calculate the target pixel by blending the 4 nearest source pixels          
            *(iTrg) = (
                *(iSrc) +
                *(iSrc + 1) +
                *(iSrc + aSrcSize.iWidth) +
                *(iSrc + 1 + aSrcSize.iWidth)
                ) >> 2;  // divide by 4
                
            iTrg++;     // Move on to the next target pixel
            iSrc += 2;  // Sample every second column from the source
            }
            
        iSrc += aSrcSize.iWidth;    // Sample every second row from the source
        }
        
    if ( aBlackBoxing.iHeight != 0 )
        {
        // increment over bottom letterboxed area
        iTrg += aBlackBoxing.iHeight * aTrgSize.iWidth;
        }
    }

// -----------------------------------------------------------------------------
// CTRScaler::ResampleDouble()
// Resamples an image to double size. A 2x2 pixel area is generated using
// the four nearest pixels from the source and written to the target image.
// -----------------------------------------------------------------------------
//      
void CTRScaler::ResampleDouble(TSize& aSrcSize, TSize& aTrgSize)
    {
    TInt i = 0, j = 0;
    
    // Make sure the scale factor is correct
    VPASSERT( (aTrgSize.iWidth == aSrcSize.iWidth * 2) &&
              (aTrgSize.iHeight == aSrcSize.iHeight * 2) );

    // Generate 2x2 target pixels in each loop
    
    // Loop every second target row
    for( i = 0; i < aTrgSize.iHeight - 2; i += 2 )
        {
        // Loop every second target column      
        for( j = 0; j < aTrgSize.iWidth - 2; j += 2 )
            {
            // Top-left pixel: Copy as it is
            *(iTrg) = *(iSrc);
            
            // Top-right pixel: Blend the pixels on the left and right
            *(iTrg + 1) = (*(iSrc) + *(iSrc + 1)) >> 1; 
             
            // Bottom-left pixel: Blend the above and below pixels
            *(iTrg + aTrgSize.iWidth) = (*(iSrc) + *(iSrc + aSrcSize.iWidth)) >> 1;
               
            // Bottom-right pixel: Blend the four nearest pixels
            *(iTrg + 1 + aTrgSize.iWidth) = (
                *(iSrc) +
                *(iSrc + 1) +
                *(iSrc + aSrcSize.iWidth) +
                *(iSrc + 1 + aSrcSize.iWidth)
                ) >> 2;
                
            iTrg += 2;      // Move on to the next 2x2 group of pixels
            iSrc++;         // Sample the next pixel from source       
            }
            
        // The last 2x2 pixels on this row need to be handled separately
        
        // Top-left and top-right pixels: Copy as it is
        *(iTrg) = *(iTrg + 1) = *(iSrc);
        
        // Bottom-left and bottom-right pixels: Blend the above and below pixels
        *(iTrg + aTrgSize.iWidth) = *(iTrg + 1 + aTrgSize.iWidth) = (
            *(iSrc) +
            *(iSrc + aSrcSize.iWidth)
            ) >> 1;
            
        iTrg += 2 + aTrgSize.iWidth;        // Move on to the beginning of the next row
        iSrc++;                             // Sample the next pixel from source   
        }
        
    // Handle the last row    
    for( j = 0; j < aTrgSize.iWidth - 2; j += 2 )
        {
        // Top-left and bottom-left pixels: Copy as it is
        *(iTrg) = *(iTrg + aTrgSize.iWidth) = *(iSrc);
        
        // Top-right and bottom-right pixels: Blend the pixels on the left and right
        *(iTrg + 1) = *(iTrg + 1 + aTrgSize.iWidth) = (
            *(iSrc) +
            *(iSrc + 1)
            ) >> 1;
            
        iTrg += 2;      // Move on to the next 2x2 group of pixels
        iSrc++;         // Sample the next pixel from source               
        }
    
    // Handle the last 2x2 group of pixels  
    
    // Copy all four pixels
    *(iTrg) = *(iTrg + 1) = *(iTrg + aTrgSize.iWidth) = *(iTrg + 1 + aTrgSize.iWidth) = *(iSrc);
    
    // Update pointers to the beginning of the next image
    iTrg += 2 + aTrgSize.iWidth;
    iSrc++;
    }

// -----------------------------------------------------------------------------
// CTRScaler::EstimateResampleFrameTime
// Returns a time estimate of how long it takes to resample a frame
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//    
TReal CTRScaler::EstimateResampleFrameTime(const TTRVideoFormat& aInput, const TTRVideoFormat& aOutput)
    {
    // Assume bilinear filtering is used by default
    TReal time = KTRResampleTimeFactorBilinear;
    
    if ( (aOutput.iSize.iWidth == aInput.iSize.iWidth) && (aOutput.iSize.iHeight == aInput.iSize.iHeight) )
        {
        // No need for resampling
        return 0.0;
        }
    else if ( (aOutput.iSize.iWidth == aInput.iSize.iWidth * 2) && (aOutput.iSize.iHeight == aInput.iSize.iHeight * 2) )
        {
        // Resolution is doubled
        time = KTRResampleTimeFactorDouble;
        }
    else if ( (aOutput.iSize.iWidth * 2 == aInput.iSize.iWidth) && (aOutput.iSize.iHeight * 2 == aInput.iSize.iHeight) )
        {
        // Resolution is halved
        time = KTRResampleTimeFactorHalve;
        }
    
    // Multiply the time by the resolution of the output frame
    time *= static_cast<TReal>(aOutput.iSize.iWidth + aOutput.iSize.iHeight) * KTRTimeFactorScale;
    
    PRINT((_L("CTRScaler::EstimateResampleFrameTime(), resample frame time: %.2f"), time))
    
    return time;
    }


// End of file
