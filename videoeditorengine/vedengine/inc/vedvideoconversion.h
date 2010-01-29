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


#ifndef __VEDVIDEOCONVERSION_H__
#define __VEDVIDEOCONVERSION_H__

#include <e32base.h>
#include <f32file.h>

enum TMMSCompatibility
{
     ECompatible,
     ECutNeeded,  // Only cut or also format conversion
     EConversionNeeded,  // File small enough, format wrong
     EIncompatible  // Wrong format, cannot convert
};

#define KVedOriginalDuration TTimeIntervalMicroSeconds(-1)

class CVideoConverter;

class MVideoConverterObserver
{

    public:
    
        virtual void MvcoFileInserted(CVideoConverter& aConverter) = 0;
        
        virtual void MvcoFileInsertionFailed(CVideoConverter& aConverter, TInt aError) = 0;

        virtual void MvcoConversionStartedL(CVideoConverter& aConverter) = 0;
    
        virtual void MvcoConversionProgressed(CVideoConverter& aConverter, TInt aPercentage) = 0;
    
        virtual void MvcoConversionCompleted(CVideoConverter& aConverter, TInt aError) = 0;
};


class CVideoConverter : public CBase
{

    public:
    
        IMPORT_C static CVideoConverter* NewL(MVideoConverterObserver& aObserver);
        
        IMPORT_C static CVideoConverter* NewLC(MVideoConverterObserver& aObserver);
        
        // Insert file to be checked / converted
        virtual void InsertFileL(RFile* aFile) = 0;

        // check compatibility
        virtual TMMSCompatibility CheckMMSCompatibilityL(TInt aMaxSize) = 0;
                
        // get estimate for end time based on start time and target size
        virtual void GetDurationEstimateL(TInt aTargetSize, TTimeIntervalMicroSeconds aStartTime, 
                                          TTimeIntervalMicroSeconds& aEndTime) = 0;
        
        // Start converting file
        virtual void ConvertL(RFile* aOutputFile, TInt aSizeLimit,
                              TTimeIntervalMicroSeconds aCutInTime = TTimeIntervalMicroSeconds(0),
                              TTimeIntervalMicroSeconds aCutOutTime = KVedOriginalDuration) = 0;
        
        // Cancel
        virtual TInt CancelConversion() = 0;
        
        // Reset converter, remove file etc.
        virtual TInt Reset() = 0;

};

#endif

// End of file
