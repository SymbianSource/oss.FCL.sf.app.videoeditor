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


#ifndef __VEDVIDEOCONVERSIONIMP_H__
#define __VEDVIDEOCONVERSIONIMP_H__

#include <vedmovie.h>
#include "vedvideoconversion.h"

class CVideoConverterImp : public CVideoConverter, public MVedMovieObserver,
                           public MVedMovieProcessingObserver
{

    public:  // Functions from CVideoConverter 
    
        static CVideoConverterImp* NewL(MVideoConverterObserver& aObserver);
        
        static CVideoConverterImp* NewLC(MVideoConverterObserver& aObserver);
        
        ~CVideoConverterImp();
        
        void InsertFileL(RFile* aFile);

        TMMSCompatibility CheckMMSCompatibilityL(TInt aMaxSize);
                
        void GetDurationEstimateL(TInt aTargetSize, TTimeIntervalMicroSeconds aStartTime, 
                                  TTimeIntervalMicroSeconds& aEndTime);
        
        void ConvertL(RFile* aOutputFile, TInt aSizeLimit, 
                      TTimeIntervalMicroSeconds aCutInTime, 
                      TTimeIntervalMicroSeconds aCutOutTime);
                    
        TInt CancelConversion();
        
        TInt Reset();
        
    public:  // Functions from MVedMovieObserver
    
        //from observer
    	void NotifyVideoClipAdded(CVedMovie& aMovie, TInt aIndex);
    	void NotifyVideoClipAddingFailed(CVedMovie& aMovie, TInt aError);    	
    	void NotifyVideoClipRemoved(CVedMovie& aMovie, TInt aIndex);
    	void NotifyVideoClipIndicesChanged(CVedMovie& aMovie, TInt aOldIndex, TInt aNewIndex);
    	void NotifyVideoClipTimingsChanged(CVedMovie& aMovie, TInt aIndex);
    	void NotifyVideoClipSettingsChanged(CVedMovie& aMovie, TInt aIndex);    	
    	void NotifyStartTransitionEffectChanged(CVedMovie& aMovie);
    	void NotifyMiddleTransitionEffectChanged(CVedMovie& aMovie, TInt aIndex);
    	void NotifyEndTransitionEffectChanged(CVedMovie& aMovie);
    	void NotifyAudioClipAdded(CVedMovie& aMovie, TInt aIndex);
    	void NotifyAudioClipAddingFailed(CVedMovie& aMovie, TInt aError);
    	void NotifyAudioClipRemoved(CVedMovie& aMovie, TInt aIndex);
    	void NotifyAudioClipIndicesChanged(CVedMovie& aMovie, TInt aOldIndex, TInt aNewIndex);
    	void NotifyAudioClipTimingsChanged(CVedMovie& aMovie, TInt aIndex);
        void NotifyMovieReseted(CVedMovie& aMovie);
        void NotifyVideoClipGeneratorSettingsChanged(CVedMovie& /*aMovie*/, TInt /*aIndex*/);
        void NotifyMovieOutputParametersChanged(CVedMovie& aMovie);
        void NotifyVideoClipColorEffectChanged(CVedMovie& /*aMovie*/, TInt /*aIndex*/);
        void NotifyVideoClipAudioSettingsChanged(CVedMovie& /*aMovie*/, TInt /*aIndex*/);                
        void NotifyMovieProcessingStartedL(CVedMovie& /*aMovie*/);
        void NotifyMovieProcessingProgressed(CVedMovie& aMovie, TInt aPercentage);
    	void NotifyMovieProcessingCompleted(CVedMovie& aMovie, TInt aError);    	
        void NotifyVideoClipDescriptiveNameChanged(CVedMovie& aMovie, TInt aIndex);
    	void NotifyMovieQualityChanged(CVedMovie& aMovie);        
        void NotifyAudioClipDynamicLevelMarkInserted(CVedMovie& aMovie, TInt aClipIndex, TInt aMarkIndex);
        void NotifyAudioClipDynamicLevelMarkRemoved(CVedMovie& aMovie,TInt aClipIndex,TInt aMarkIndex);
        void NotifyVideoClipDynamicLevelMarkInserted(CVedMovie& aMovie, TInt aClipIndex, TInt aMarkIndex);
        void NotifyVideoClipDynamicLevelMarkRemoved(CVedMovie& aMovie,TInt aClipIndex,TInt aMarkIndex);        
    
    private:
    
        /*
        * C++ default constructor.
        */
        CVideoConverterImp(MVideoConverterObserver& aObserver);
        
        /**
        * 2nd phase constructor 
        */
        void ConstructL();	    
        
    private:
    
        // Observer
        MVideoConverterObserver& iObserver;
    
        // Movie
        CVedMovie* iMovie;
        
        friend class CVideoConverter;
};

#endif

// End of file
