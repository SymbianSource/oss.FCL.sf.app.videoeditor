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
* YUV to EColor16M colorspace converter concrete classes. 
*
*/

    

#ifndef     __YUV2RGB24_H__
#define     __YUV2RGB24_H__


//  INCLUDES

#ifndef __YUVCONVERTER_H__
#include "yuvconverter.h"
#endif


//  CLASS DEFINITIONS

// Normal YUV to Rgb24
class CYuv2Rgb24 : public CYuvConverter
{
public: // CYuvConverter methods
    // Constructors & destructor
    CYuv2Rgb24();
    ~CYuv2Rgb24();
    void ConstructL(TUint aWidth, TUint aHeight, TUint aMaxWidth, TUint aMaxHeight);

    void SetGamma(TInt aGamma);
    void SetBrightnessContrast(TInt aBCIndex);
    void Convert(const TUint8 *aYBuf, const TUint8 *aUBuf,
                 const TUint8 *aVBuf,
                 TUint aBufWidth, TUint aBufHeight,
                 TUint8 *aTarget, TUint aTargetScanlineLength);

protected: // Data
    enum
    {
        ESaturateOffset = 276, // saturation table offset
        ESaturateLength = 561+ESaturateOffset // table length
    };
    TUint8 *iRgbLookupTable; // RGB clamp/gamma lookup table
    TUint iWidth, iHeight, iCropWidth, iCropHeight;
    TInt iGamma;
    TInt iBrightnessContrast;
};




#endif      //  __YUV2RGB24_H__
            
// End of File
