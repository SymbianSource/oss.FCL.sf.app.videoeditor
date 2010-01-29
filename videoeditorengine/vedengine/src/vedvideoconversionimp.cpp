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


#include "vedvideoclip.h"
#include "vedvideoconversionimp.h"

// Print macro
#ifdef _DEBUG
#include <e32svr.h>
#define PRINT(x) RDebug::Print x
#else
#define PRINT(x)
#endif

// An assertion macro wrapper to clean up the code a bit
#define VCASSERT(x) __ASSERT_DEBUG(x, User::Panic(_L("CVideoConverterImp"), -10000 ))


// ---------------------------------------------------------------------------
// NewL() of CVideoConverter
// ---------------------------------------------------------------------------
//
EXPORT_C CVideoConverter* CVideoConverter::NewL(MVideoConverterObserver& aObserver) 
    {
    PRINT(_L("CVideoConverter::NewL in"));
    
    CVideoConverterImp* self = (CVideoConverterImp*)NewLC(aObserver);
    CleanupStack::Pop(self);        
    PRINT(_L("CVideoConverter::NewL out"));
    
    return self;
    }
    
// ---------------------------------------------------------------------------
// NewLC() of CVideoConverter
// ---------------------------------------------------------------------------
//
EXPORT_C CVideoConverter* CVideoConverter::NewLC(MVideoConverterObserver& aObserver)
    {
    PRINT(_L("CVideoConverter::NewLC in"));
    
    CVideoConverterImp* self = new (ELeave) CVideoConverterImp(aObserver);
    CleanupStack::PushL(self);
    self->ConstructL();

    PRINT(_L("CVideoConverter::NewLC out"));
    return self;
    }

// ---------------------------------------------------------------------------
// Constructor of CVideoConverter
// ---------------------------------------------------------------------------
//
CVideoConverterImp::CVideoConverterImp(MVideoConverterObserver& aObserver) : iObserver(aObserver)
{    
}

    
// ---------------------------------------------------------------------------
// Destructor of CVideoConverter
// ---------------------------------------------------------------------------
//
CVideoConverterImp::~CVideoConverterImp()
{
    delete iMovie;
}

// ---------------------------------------------------------------------------
// ConstructL() of CVideoConverter
// ---------------------------------------------------------------------------
//
void CVideoConverterImp::ConstructL()
{   
    iMovie = CVedMovie::NewL(NULL);
 
    iMovie->RegisterMovieObserverL(this);
 
    // set quality to MMS   
    iMovie->SetQuality(CVedMovie::EQualityMMSInteroperability);
}

// ---------------------------------------------------------------------------
// Insert file to be checked / converted
// ---------------------------------------------------------------------------
//
void CVideoConverterImp::InsertFileL(RFile* aFile)
{

    if ( iMovie->VideoClipCount() != 0 )
        User::Leave(KErrAlreadyExists);

    iMovie->InsertVideoClipL(aFile, 0);
}

// ---------------------------------------------------------------------------
// Check compatibility
// ---------------------------------------------------------------------------
//
TMMSCompatibility CVideoConverterImp::CheckMMSCompatibilityL(TInt aMaxSize)
{

    VCASSERT( iMovie->VideoClipCount() == 1 );
    
    if ( iMovie->VideoClipCount() != 1 )
        User::Leave(KErrNotFound);
    
    CVedVideoClipInfo* info = iMovie->VideoClipInfo(0);
    
    // check file size
    RFile* file = info->FileHandle();
    
    TInt size = 0;
    User::LeaveIfError( file->Size(size) );
    
    TBool sizeOK = ( size <= aMaxSize );
    
    // check format
    TVedVideoFormat fileFormat = info->Format();
    TVedVideoType videoCodec = info->VideoType();
    TVedAudioType audioCodec = info->AudioType();

    TBool formatOK = ( fileFormat == EVedVideoFormat3GPP ) &&
                     ( videoCodec == EVedVideoTypeH263Profile0Level10 ) &&
                     ( audioCodec == EVedAudioTypeNoAudio || 
                       audioCodec == EVedAudioTypeAMR );

    if ( formatOK && sizeOK )
        return ECompatible;
    
    if ( sizeOK || (!sizeOK && !formatOK) )
    { 
         VCASSERT( !formatOK );
         // check estimated size after conversion
         // NOTE: This is checked also for sizeOK, because it's possible
         //       that size increases in conversion
        TInt sizeEstimate = iMovie->GetSizeEstimateL();
        
        if ( sizeEstimate <= aMaxSize )
            return EConversionNeeded;
        
        else
            return ECutNeeded;   
    } 
    
    // size not ok, format ok
    VCASSERT( !sizeOK && formatOK );

    return ECutNeeded;    

}

// ---------------------------------------------------------------------------
// Get end time estimate on basis of target size and start time
// ---------------------------------------------------------------------------
//
void CVideoConverterImp::GetDurationEstimateL(TInt aTargetSize, TTimeIntervalMicroSeconds aStartTime, 
                                           TTimeIntervalMicroSeconds& aEndTime)
{
 
    iMovie->GetDurationEstimateL(aTargetSize, aStartTime, aEndTime);

}

// ---------------------------------------------------------------------------
// Start conversion
// ---------------------------------------------------------------------------
//
void CVideoConverterImp::ConvertL(RFile* aOutputFile, TInt aSizeLimit,
                                  TTimeIntervalMicroSeconds aCutInTime, 
                                  TTimeIntervalMicroSeconds aCutOutTime)
{

    VCASSERT( iMovie && iMovie->VideoClipCount() == 1 );
    
    TTimeIntervalMicroSeconds temp = iMovie->VideoClipCutOutTime(0);
    
    iMovie->VideoClipSetCutInTime(0, aCutInTime);
    
    if ( aCutOutTime != KVedOriginalDuration )
        iMovie->VideoClipSetCutOutTime(0, aCutOutTime);
    
    iMovie->SetMovieSizeLimit(aSizeLimit);
    
    iMovie->ProcessL(aOutputFile, *this);

}

// ---------------------------------------------------------------------------
// Cancel conversion
// ---------------------------------------------------------------------------
//
TInt CVideoConverterImp::CancelConversion()
{
    iMovie->CancelProcessing();
    
    return KErrNone;
}

// ---------------------------------------------------------------------------
// Reset to initial state
// ---------------------------------------------------------------------------
//
TInt CVideoConverterImp::Reset()
{
    if (iMovie)
        iMovie->Reset();
    
    return KErrNone;
}

// ---------------------------------------------------------------------------
// Implementation of clip added -callback
// ---------------------------------------------------------------------------
//
void CVideoConverterImp::NotifyVideoClipAdded(CVedMovie& /*aMovie*/, TInt /*aIndex*/)
{
    iObserver.MvcoFileInserted(*this);
}


// ---------------------------------------------------------------------------
// Implementation of clip adding failed -callback
// ---------------------------------------------------------------------------
//
void CVideoConverterImp::NotifyVideoClipAddingFailed(CVedMovie& /*aMovie*/, TInt aError)
{
    iObserver.MvcoFileInsertionFailed(*this, aError);
}

// ---------------------------------------------------------------------------
// Implementation of clip movie processing started -callback
// ---------------------------------------------------------------------------
//
void CVideoConverterImp::NotifyMovieProcessingStartedL(CVedMovie& /*aMovie*/)
{
    iObserver.MvcoConversionStartedL(*this);
}

// ---------------------------------------------------------------------------
// Implementation of clip movie processing progressed -callback
// ---------------------------------------------------------------------------
//
void CVideoConverterImp::NotifyMovieProcessingProgressed(CVedMovie& /*aMovie*/, TInt aPercentage)
{
    iObserver.MvcoConversionProgressed(*this, aPercentage);
}

// ---------------------------------------------------------------------------
// Implementation of clip movie processing completed -callback
// ---------------------------------------------------------------------------
//
void CVideoConverterImp::NotifyMovieProcessingCompleted(CVedMovie& /*aMovie*/, TInt aError)
{
    iObserver.MvcoConversionCompleted(*this, aError);
}

void CVideoConverterImp::NotifyVideoClipRemoved(CVedMovie& /*aMovie*/, TInt /*aIndex*/){}
void CVideoConverterImp::NotifyAudioClipAdded(CVedMovie& /*aMovie*/, TInt /*aIndex*/){}
void CVideoConverterImp::NotifyAudioClipAddingFailed(CVedMovie& /*aMovie*/, TInt /*aError*/){}
void CVideoConverterImp::NotifyVideoClipIndicesChanged(CVedMovie& /*aMovie*/, TInt /*aOldIndex*/, TInt /*aNewIndex*/){}
void CVideoConverterImp::NotifyVideoClipTimingsChanged(CVedMovie& /*aMovie*/, TInt /*aIndex*/){}
void CVideoConverterImp::NotifyVideoClipSettingsChanged(CVedMovie& /*aMovie*/, TInt /*aIndex*/){}
void CVideoConverterImp::NotifyStartTransitionEffectChanged(CVedMovie& /*aMovie*/){}
void CVideoConverterImp::NotifyMiddleTransitionEffectChanged(CVedMovie& /*aMovie*/, TInt /*aIndex*/){}
void CVideoConverterImp::NotifyEndTransitionEffectChanged(CVedMovie& /*aMovie*/){}
void CVideoConverterImp::NotifyAudioClipRemoved(CVedMovie& /*aMovie*/, TInt /*aIndex*/){}
void CVideoConverterImp::NotifyAudioClipIndicesChanged(CVedMovie& /*aMovie*/, TInt /*aOldIndex*/, TInt /*aNewIndex*/){}
void CVideoConverterImp::NotifyAudioClipTimingsChanged(CVedMovie& /*aMovie*/, TInt /*aIndex*/){}
void CVideoConverterImp::NotifyMovieReseted(CVedMovie& /*aMovie*/){}
void CVideoConverterImp::NotifyVideoClipGeneratorSettingsChanged(CVedMovie& /*aMovie*/, TInt /*aIndex*/) {} 
void CVideoConverterImp::NotifyMovieOutputParametersChanged(CVedMovie& /*aMovie*/){}
void CVideoConverterImp::NotifyVideoClipColorEffectChanged(CVedMovie& /*aMovie*/, TInt /*aIndex*/){}
void CVideoConverterImp::NotifyVideoClipAudioSettingsChanged(CVedMovie& /*aMovie*/, TInt /*aIndex*/){}
void CVideoConverterImp::NotifyVideoClipDescriptiveNameChanged(CVedMovie& /*aMovie*/, TInt /*aIndex*/){}
void CVideoConverterImp::NotifyMovieQualityChanged(CVedMovie& /*aMovie*/){}
void CVideoConverterImp::NotifyAudioClipDynamicLevelMarkInserted(CVedMovie& /*aMovie*/, TInt /*aClipIndex*/, TInt /*aMarkIndex*/){}
void CVideoConverterImp::NotifyAudioClipDynamicLevelMarkRemoved(CVedMovie& /*aMovie*/, TInt /*aClipIndex*/, TInt /*aMarkIndex*/){}
void CVideoConverterImp::NotifyVideoClipDynamicLevelMarkInserted(CVedMovie& /*aMovie*/, TInt /*aClipIndex*/, TInt /*aMarkIndex*/){}
void CVideoConverterImp::NotifyVideoClipDynamicLevelMarkRemoved(CVedMovie& /*aMovie*/, TInt /*aClipIndex*/, TInt /*aMarkIndex*/){}
