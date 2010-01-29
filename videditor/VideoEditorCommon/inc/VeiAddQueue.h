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


#ifndef VEIADDQUEUE_H
#define VEIADDQUEUE_H

#include <e32base.h>
#include <vedmovie.h>
#include "veiimageclipgenerator.h"
#include "VideoEditorCommon.h"

class CVedVideoClipGenerator;


class MVeiQueueObserver
	{
public:
	enum TProcessing
		{
		EProcessingAudio = 0x50,
		EProcessingVideo,
		EProcessingImage,
		ENotProcessing
		};

public:

	virtual void NotifyQueueProcessingStarted( MVeiQueueObserver::TProcessing aMode = MVeiQueueObserver::ENotProcessing ) = 0;

	virtual void NotifyQueueEmpty( TInt aInserted, TInt aFailed ) = 0;

	virtual void NotifyQueueProcessingProgressed( TInt aProcessedCount, TInt aPercentage ) = 0;

	virtual TBool NotifyQueueClipFailed( const TDesC& aFilename, TInt aError ) = 0;
	};


class CVeiAddQueue :	public CActive,
						public MVedMovieObserver,
						public MVeiImageClipGeneratorObserver
    {
	public:
	    IMPORT_C static CVeiAddQueue* NewL( MVedAudioClipInfoObserver& aView, CVedMovie& aMovie, MVeiQueueObserver& aObserver );

        IMPORT_C static CVeiAddQueue* NewLC( MVedAudioClipInfoObserver& aView, CVedMovie& aMovie, MVeiQueueObserver& aObserver );

        IMPORT_C virtual ~CVeiAddQueue();

    public:

		IMPORT_C TBool ShowAudioClipDialogL();

		IMPORT_C TBool ShowVideoClipDialogL( VideoEditor::TCursorLocation aLocation, TInt aCurrentIndex );

		IMPORT_C void InsertMediaL( const TDesC& aFilename );

		IMPORT_C void StartProcessingL();

		IMPORT_C void GetNext();

		IMPORT_C TInt Count() const;

		enum TErrorCases
			{
			EInsertingSingleClip = -90,
			EInsertingFromGallery
			};
	private:

		TInt AddNextL( TInt aPosition = -1 );

		void DoCancel();
		
		void RunL();

		CVeiAddQueue( MVedAudioClipInfoObserver& aView, CVedMovie& aMovie, MVeiQueueObserver& aObserver );

	    void ConstructL();

  

	private:
// From MVeiImageClipGeneratorObserver
		virtual void NotifyImageClipGeneratorInitializationComplete(
			CVeiImageClipGenerator& aGenerator, TInt aError);

// From MVedMovieObserver
		virtual void NotifyVideoClipAdded(CVedMovie& aMovie, TInt aIndex);
		virtual void NotifyVideoClipAddingFailed(CVedMovie& aMovie, TInt aError);
		virtual void NotifyVideoClipRemoved(CVedMovie& aMovie, TInt aIndex);
		virtual void NotifyVideoClipIndicesChanged(CVedMovie& aMovie, TInt aOldIndex, 
									           TInt aNewIndex);
		virtual void NotifyVideoClipTimingsChanged(CVedMovie& aMovie,
											   TInt aIndex);
		virtual void NotifyVideoClipColorEffectChanged(CVedMovie& aMovie,
												   TInt aIndex);
		virtual void NotifyVideoClipAudioSettingsChanged(CVedMovie& aMovie,
											         TInt aIndex);
		virtual void NotifyVideoClipGeneratorSettingsChanged(CVedMovie& aMovie,
											             TInt aIndex);
		virtual void NotifyVideoClipDescriptiveNameChanged(CVedMovie& aMovie,
																TInt aIndex);
		virtual void NotifyStartTransitionEffectChanged(CVedMovie& aMovie);
		virtual void NotifyMiddleTransitionEffectChanged(CVedMovie& aMovie, 
													 TInt aIndex);
		virtual void NotifyEndTransitionEffectChanged(CVedMovie& aMovie);
		virtual void NotifyAudioClipAdded(CVedMovie& aMovie, TInt aIndex);
		virtual void NotifyAudioClipAddingFailed(CVedMovie& aMovie, TInt aError);
		virtual void NotifyAudioClipRemoved(CVedMovie& aMovie, TInt aIndex);
		virtual void NotifyAudioClipIndicesChanged(CVedMovie& aMovie, TInt aOldIndex, 
									           TInt aNewIndex);
		virtual void NotifyAudioClipTimingsChanged(CVedMovie& aMovie,
											   TInt aIndex);
		virtual void NotifyMovieQualityChanged(CVedMovie& aMovie);
		virtual void NotifyMovieReseted(CVedMovie& aMovie);

		virtual void NotifyMovieOutputParametersChanged(CVedMovie& aMovie);
	    virtual void NotifyAudioClipDynamicLevelMarkInserted(CVedMovie& aMovie, 
                                                         TInt aClipIndex, 
                                                         TInt aMarkIndex);
		virtual void NotifyAudioClipDynamicLevelMarkRemoved(CVedMovie& aMovie, 
                                                        TInt aClipIndex, 
                                                        TInt aMarkIndex);
		virtual void NotifyVideoClipDynamicLevelMarkInserted(CVedMovie& aMovie, 
                                                         TInt aClipIndex, 
                                                         TInt aMarkIndex);
		virtual void NotifyVideoClipDynamicLevelMarkRemoved(CVedMovie& aMovie, 
                                                        TInt aClipIndex, 
                                                        TInt aMarkIndex);

	private:
	     /**
         * No description.
         */
        RPointerArray<TDesC>    iAddQueue;

	     /**
         * No description.
         */
		MVeiQueueObserver*		iObserver;

	     /**
         * No description.
         */
		CVedMovie&				iMovie;

		MVedAudioClipInfoObserver&		iView;
	     /**
         * Inserting failed to movie
         */
		TInt					iFailedCount;

	     /**
         * Files added to movie.
         */
		TInt					iInsertedCount;

	     /**
         * Total number of files in queue when processing is started.
         */
		TInt					iTotalCount;

	     /**
         * No description.
         */
		CVedVideoClipGenerator* iGenerator;

	     /**
         * No description.
         */
        CVedAudioClipInfo*      iAudioClipInfo;
		CActiveSchedulerWait *iWaitScheduler;
		TInt				iError;
		TBool				iInsertVideoDialogOn;
	};
#endif
