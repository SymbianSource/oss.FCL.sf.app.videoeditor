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



#include "VedVideoClipGenerator.h"
#include "VedMovieImp.h"
#include "VedVideoClip.h"
#include <e32std.h>

EXPORT_C CVedVideoClipGenerator::CVedVideoClipGenerator()
        : iVideoClip(0), iIsOwnedByVideoClip(EFalse), iMaximumFramerate(KVEdMaxFrameRate)
    {
    }


EXPORT_C CVedVideoClipGenerator::~CVedVideoClipGenerator()
    {
    }


EXPORT_C TBool CVedVideoClipGenerator::IsInserted() const
    {
    return (iVideoClip != 0);
    }


EXPORT_C CVedMovie* CVedVideoClipGenerator::Movie() const
    {
    __ASSERT_ALWAYS(iVideoClip != 0,
                    TVedPanic::Panic(TVedPanic::EVideoClipGeneratorNotInserted));

    return iVideoClip->Movie();
    }


EXPORT_C TInt CVedVideoClipGenerator::VideoClipIndex() const
    {
    __ASSERT_ALWAYS(iVideoClip != 0,
                    TVedPanic::Panic(TVedPanic::EVideoClipGeneratorNotInserted));

    return iVideoClip->Index();
    }


EXPORT_C TBool CVedVideoClipGenerator::IsOwnedByVideoClip() const
    {
    __ASSERT_ALWAYS(iVideoClip != 0,
                    TVedPanic::Panic(TVedPanic::EVideoClipGeneratorNotInserted));

    return iIsOwnedByVideoClip;
    }


void CVedVideoClipGenerator::SetVideoClip(CVedVideoClip& aVideoClip,
                                          TBool aIsOwnedByVideoClip)
    {
    __ASSERT_ALWAYS(iVideoClip == 0,
                    TVedPanic::Panic(TVedPanic::EVideoClipGeneratorAlreadyInserted));

    iVideoClip = &aVideoClip;
    iIsOwnedByVideoClip = aIsOwnedByVideoClip;
    }


EXPORT_C void CVedVideoClipGenerator::ReportDurationChanged() const
    {
    if (iVideoClip != 0)
        {
        iVideoClip->Movie()->RecalculateVideoClipTimings(iVideoClip);
        iVideoClip->Movie()->FireVideoClipTimingsChanged(iVideoClip->Movie(), iVideoClip);
        }
    }


EXPORT_C void CVedVideoClipGenerator::ReportSettingsChanged() const
    {
    if (iVideoClip != 0)
        {
        iVideoClip->Movie()->FireVideoClipGeneratorSettingsChanged(iVideoClip->Movie(), iVideoClip);
        }
    }

EXPORT_C void CVedVideoClipGenerator::ReportDescriptiveNameChanged() const
    {
    if (iVideoClip != 0)
        {
        iVideoClip->Movie()->FireVideoClipDescriptiveNameChanged(iVideoClip->Movie(), iVideoClip);
        }
    }

EXPORT_C TInt CVedVideoClipGenerator::CalculateFrameComplexityFactor(CFbsBitmap* /*aFrame*/) const
    {
    RDebug::Print(_L("CVedVideoClipGenerator::CalculateFrameComplexityFactor IN"));
    
    return 10;

    // Disabled since the calculation takes too long and is not very accurate
    /*if ( !aFrame ) 
        {
        return 0;
        }

    TInt height = aFrame->SizeInPixels().iHeight;
    TInt width = aFrame->SizeInPixels().iWidth;
    TInt numberOfPixels = height * width;

    TInt mean = 0;

    TInt y; 
    TInt x;

    for (y = 0; y < height; y++) 
        {
        for (x = 0; x < width; x++)
            {
            TRgb color;
            TPoint point(x, y);
            aFrame->GetPixel(color, point);
            mean += color.Green();   
            }
        }
    mean = (TInt)((TReal)mean/(TReal)numberOfPixels + 0.5);

    TInt moment = 0;
    TInt tmp;
    for (y = 0; y < height; y++) 
        {
        for (x = 0; x < width; x++)
            {
            TRgb color;
            TPoint point(x, y);
            aFrame->GetPixel(color, point);
            tmp = color.Green() - mean;
            if (tmp<0)
                tmp = -tmp;
            moment += tmp;
            }
        }
    moment = (TInt)((TReal)moment/(TReal)numberOfPixels + 0.5);
    moment *= 10;

    RDebug::Print(_L("CVedVideoClipGenerator::CalculateFrameComplexityFactor returning moment: %d"), moment);
    return moment;*/
    }

// End of file
