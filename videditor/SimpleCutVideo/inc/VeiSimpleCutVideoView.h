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


#ifndef VEISIMPLECUTVIDEOVIEW_H
#define VEISIMPLECUTVIDEOVIEW_H


#include <aknview.h>
#include <VedMovie.h>
#include <VedCommon.h>
#include <aknprogressdialog.h> 
#include <utility.h>
#include <caknmemoryselectiondialog.h> 
#include "extprogressdialog.h"
#include "VeiSimpleCutVideoContainer.h"

//class CAknTitlePane;
class CPeriodic;
class CVeiErrorUI;
class CAknMemorySelectionDialog;
class CVeiTempMaker;
class CVeiNaviPaneControl;


/**
*  CVeiCutVideoView view class.
* 
*/
class CVeiSimpleCutVideoView : public CAknView, public MVedMovieObserver,
						 public MVedMovieProcessingObserver, 
						 public MProgressDialogCallback,
						 public MExtProgressDialogCallback
    {
    public:

        /**
        * Default constructor.
        */
        void ConstructL();

        /**
        * Destructor.
        */
        virtual ~CVeiSimpleCutVideoView();

	protected:

		/** 
		* From CAknView, HandleForegroundEventL( TBool aForeground )
		*
		* @param aForeground
		*/
		virtual void HandleForegroundEventL( TBool aForeground );

    private:
 
		/**
		 * From CAknView, DynInitMenuPaneL.
		 *
		 * @param aResourceId  resource id
		 * @param aMenuPane  menu pane
		 */
		void DynInitMenuPaneL(TInt aResourceId,CEikMenuPane* aMenuPane);	

		/** 
		* From MVedMovieProcessingObserver
		*/
		virtual void NotifyMovieProcessingStartedL(CVedMovie& aMovie);
		virtual void NotifyMovieProcessingProgressed(CVedMovie& aMovie, TInt aPercentage);
		virtual void NotifyMovieProcessingCompleted(CVedMovie& aMovie, TInt aError);
	
		/**
		* From MVedMovieObserver
		*/
		virtual void NotifyVideoClipAdded( CVedMovie& aMovie, TInt aIndex );
		virtual void NotifyVideoClipAddingFailed( CVedMovie& aMovie, TInt aError );
		virtual void NotifyVideoClipRemoved( CVedMovie& aMovie, TInt aIndex );
		virtual void NotifyVideoClipIndicesChanged( CVedMovie& aMovie, TInt aOldIndex, 
									           TInt aNewIndex );
		virtual void NotifyVideoClipTimingsChanged( CVedMovie& aMovie,
											   TInt aIndex );
		virtual void NotifyVideoClipColorEffectChanged( CVedMovie& aMovie,
												   TInt aIndex );
		virtual void NotifyVideoClipAudioSettingsChanged( CVedMovie& aMovie,
											         TInt aIndex );
		virtual void NotifyStartTransitionEffectChanged( CVedMovie& aMovie );
		virtual void NotifyMiddleTransitionEffectChanged( CVedMovie& aMovie, 
													 TInt aIndex );
		virtual void NotifyEndTransitionEffectChanged( CVedMovie& aMovie );
		virtual void NotifyAudioClipAdded( CVedMovie& aMovie, TInt aIndex );
		virtual void NotifyAudioClipAddingFailed( CVedMovie& aMovie, TInt aError );
		virtual void NotifyAudioClipRemoved( CVedMovie& aMovie, TInt aIndex );
		virtual void NotifyAudioClipIndicesChanged( CVedMovie& aMovie, TInt aOldIndex, 
									           TInt aNewIndex );
		virtual void NotifyAudioClipTimingsChanged( CVedMovie& aMovie,
											   TInt aIndex );
		virtual void NotifyMovieReseted( CVedMovie& aMovie );

		virtual void NotifyVideoClipGeneratorSettingsChanged(CVedMovie& aMovie,
											             TInt aIndex);

		virtual void NotifyVideoClipDescriptiveNameChanged(CVedMovie& aMovie, TInt aIndex);
		virtual void NotifyMovieQualityChanged(CVedMovie& aMovie);				

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

		/**
		* From MProgressDialogCallback
		*/
		void DialogDismissedL(TInt aButtonId);


	public:
	    /**
        * 
		* 
		* @return VeiSimpleCutVideoView Uid
        */
        TUid Id() const;

		/**
		 * Starts temporary clip processing.
		 */
		void GenerateEffectedClipL();

       /**
        * From CAknView, HandleCommandL(TInt aCommand);
		*
		* @param aCommand
        */
        void HandleCommandL(TInt aCommand);

		/**
		 * Draws the time label navi.
		 */
		void DrawTimeNaviL();

		/**
		 * Clears the in and/or out points.
		 *
		 * @param aClearIn  whether to clear the in point
		 * @param aClearOut  whether to clear the out point
		 */
		void ClearInOutL( TBool aClearIn, TBool aClearOut );

		/**
		 * Sets the movie and index
		 *
		 * @param aVideoClip	movie name
		 * @param aIndex		index of the video clip in movie
		 */
		void SetVideoClipAndIndex(CVedMovie& aVideoClip, TInt aIndex);

		/**
		 * Returns the cut out time.
		 *
		 * @return cut out time
		 */
		TUint OutPointTime();

		/**
		 * Returns the  cut in time.
		 *
		 * @return  cut in time
		 */
		TUint InPointTime();

		/**
		 * Changes the CBA (command button array) according to the edit
		 * state.
		 *
		 * @param aState  current state
		 */
		void UpdateCBAL( TInt aState );

		/**
		 * Updates the time label navi. This method is called by the
		 * static callback function.
		 */
		void UpdateTimeL();

		/**
		 * Starts the navi pane update, that is, the periodic timer.	
		 */
		void StartNaviPaneUpdateL();

		/**
		 * Stops the navi pane update.
		 */
		void StopNaviPaneUpdateL();

		/**
		 * 
		 */
		void ShowVolumeLabelL( TInt aVolume );

		/**
		 * Mutes the volume.
		 */
		void VolumeMuteL();

		/** Possible mark states */
		enum TMarkState
			{
			EMarkStateIn,
			EMarkStateOut,
			EMarkStateInOut
			};

		void HandleStatusPaneSizeChange();
		
		TInt AddClipL( const TDesC& aFilename, TBool aStartNow );
		
		/**
         * No description.
         *
         * 
         * @return No description.
         */
        TBool IsEnoughFreeSpaceToSaveL();// const;
		TTimeIntervalMicroSeconds GetVideoClipCutInTime();
		TTimeIntervalMicroSeconds GetVideoClipCutOutTime();

		/** 
		 * Handles a change to the control's resources.
		 */
		void HandleResourceChange( TInt aType );

	    /**
        * Get pointer to the movie instance.
		* 
		* @return CVedMovie*
        */
        const CVedMovie* Movie() const;
        
        inline TBool AppIsOnTheWayToDestruction() { return iOnTheWayToDestruction; };
        
        void PrepareForTermination();
        
        inline TVeiSettings Settings() { return iMovieSaveSettings; };

	    /**
        * Moves the start or end mark when user drags them.
		* 
		* @param aPosition	position where the mark is moved to
		* @param aMarkType  EStartMark or EEndMark
		* @return -
        */               
		void MoveStartOrEndMarkL(TTimeIntervalMicroSeconds aPosition, CVeiSimpleCutVideoContainer::TCutMark aMarkType);
		
	private:
		/**
		 * Starts playing the clip. If the clip is paused, resumes 
		 * playing.
		 */
		void PlayPreviewL();

		/**
		 * Pauses the playback.
		 */
		void PausePreviewL();

		/**
		 * Plays the marked section of the clip.
		 */
		void PlayMarkedL();

		/**
		 * Marks the in point to the current point.
		 */
		void MarkInL();

		/**
		 * Marks the out point to the current point.
		 */
		void MarkOutL();

		/**
		 * Static callback function for the periodical timer that updates
		 * the time navi.
		 *
		 * @param aPtr  self pointer
		 *
		 * @return dummy value
		 */
		static TInt UpdateTimeCallbackL( TAny* aPtr );
		
		TBool SaveL();
		void StartTempFileProcessingL();
		
		/**
         * Shows error note with given message.
         * 
         * @param aResourceId No description.
         * @param aError No description.
         */
        void ShowErrorNoteL( const TInt aResourceId, TInt aError = 0 ) const;


        void StartProgressNoteL();
        void StartAnimatedProgressNoteL();

        /**
         *  Checks the memory card availability, if MMC is selected as save
         *  store in application settings. An information note is shown in
         *  following situations:
         *  - MMC not inserted
         *  - MMC corrupted (unformatted)
         *  [- MMC is read-only (not implemented)]
         *  
         *  If note is popped up, this function waits until it's dismissed.
         */
        void CheckMemoryCardAvailabilityL();
        
    private:

        void CloseWaitDialog();
        
         /**
         * From AknView, DoActivateL.
		 * 
		 * @param aPrevViewId  previous view id
		 * @param aCustomMessageId  custom message id
		 * @param aCustomMessage  custom message
         */
        void DoActivateL(const TVwsViewId& aPrevViewId,TUid aCustomMessageId,
            const TDesC8& aCustomMessage);
        
		/**
        * From AknView, DoDeactivate
        */
        void DoDeactivate();
		
		/** Callback function */
		static TInt AsyncExit(TAny* aThis);


		/**
         * Sets the current file name as title pane text
         * 
         * @param -
         * @param .
         */		
		void SetTitlePaneTextL ();
		
	    /**	QueryAndSaveL
		*   
		*	Displays Avkon file handling queries and calls
		*	SaveL() 
		*
		*   @param -
		*   @return TInt 1 if the image has been saved, otherwise 0
		*/    	
		TInt QueryAndSaveL();
		
		TBool IsCutMarkSet();
		
    private: // Data
		/* cut video container	*/
        CVeiSimpleCutVideoContainer* iContainer;

		/* index of the video clip in the movie	*/
		TUint iIndex;

		/* movie */
		CVedMovie* iMovie;

		/** Time updater. */
		CPeriodic* iTimeUpdater;

		/** Progress note. */
		CAknProgressDialog* iProgressNote;

		/** Popup menu state flag */
		TBool iPopupMenuOpened;

		/** Audio muted flag */
		TBool iAudioMuted;

		/** current mark state */
		TMarkState iMarkState;

		/** Error number */
		TInt	  iErrorNmb;

		/** Error UI */
		CVeiErrorUI* iErrorUI;

		CAknMemorySelectionDialog::TMemory iMemoryInUse;

		TVeiSettings iMovieSaveSettings;				    	
    	
    	/**
         * Temporary file name for storing preview clip.
         */
        HBufC*                  iTempFile;
        
        /**
         * No description.
         */
        HBufC*                  iSaveToFileName;
        
        /**
         * No description.
         */
        CVeiTempMaker*          iTempMaker;
        
		
		CAknWaitDialog*			iWaitDialog;
		
		/**
	    * Progress dialog.
	    */
	    CExtProgressDialog*		iAnimatedProgressDialog;

		TBool		iSaving;
		
		TBool		iClosing;
		
		TInt iProcessed;
		
		/** Callback utility */
		CAsyncCallBack* iCallBack;

		TBool 				iOnTheWayToDestruction;
		TBool 				iSelectionKeyPopup;

		/** Indicates if the image will be overwritten or not */		
		TBool iOverWriteFile;
		
		/** Indicates if the video has been paused */
		TBool iPaused;
		
		/** ETrue if the user selects "Save" from menu */
		TBool iSaveOnly; 

		/// Own: NaviPaneControl		
		CVeiNaviPaneControl*    iCVeiNaviPaneControl;
		
    };

#endif

// End of File
