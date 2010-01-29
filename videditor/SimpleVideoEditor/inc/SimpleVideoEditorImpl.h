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

  
#ifndef _SIMPLEVIDEOEDITORIMPL_H_
#define _SIMPLEVIDEOEDITORIMPL_H_

//<IBUDSW>

// INCLUDES
#include <e32std.h>
#include <coemain.h>
#include <ConeResLoader.h> 
#include <aknappui.h>
#include <VedMovie.h>
#include <aknprogressdialog.h>
#include <aknwaitdialog.h> 
#include <VedAudioClipInfo.h>
#include "SimpleVideoEditorExitObserver.h"
#include "ExtProgressDialog.h"
#include "VeiImageClipGenerator.h"
#include "VeiTitleClipGenerator.h"

#include "SimpleVideoEditor.h"
#include "veisettings.h"

// FORWARD DECLATATIONS
class CVeiTempMaker;
class CExtProgressDialog;
class CDummyControl;
class CErrorUI;


// CLASS DECLARATION
NONSHARABLE_CLASS(CSimpleVideoEditorImpl) : public CActive, 
											public MVedMovieObserver,
											public MVedMovieProcessingObserver,
											public MVeiImageClipGeneratorObserver,
											public MProgressDialogCallback,
											public MExtProgressDialogCallback,
											public MCoeForegroundObserver,
											public MVedAudioClipInfoObserver
	{
	public:	// Constructor and destructor

		static CSimpleVideoEditorImpl* NewL(MSimpleVideoEditorExitObserver& aExitObserver);
		~CSimpleVideoEditorImpl();

	public: // New functions

		void StartMerge( const TDesC& aSourceFileName );
		void StartChangeAudio( const TDesC& aSourceFileName );
		void StartAddText( const TDesC& aSourceFileName );

		void StartWaitDialogL();
		void StartProgressDialogL();
		void StartAnimatedProgressDialogL ();
		void CancelMovieProcessing();

	private: // from CActive

		void DoCancel();
		void RunL();
		TInt RunError(TInt aError);

	private: // from MProgressDialogCallback

		void DialogDismissedL( TInt aButtonId );

	private: //from MVedMovieObserver

		void NotifyVideoClipAdded(CVedMovie& aMovie, TInt aIndex);
		void NotifyVideoClipAddingFailed(CVedMovie& aMovie, TInt aError);
		void NotifyVideoClipRemoved(CVedMovie& aMovie, TInt aIndex);
		void NotifyVideoClipIndicesChanged(CVedMovie& aMovie, TInt aOldIndex, TInt aNewIndex);
		void NotifyVideoClipTimingsChanged(CVedMovie& aMovie, TInt aIndex);
		void NotifyVideoClipSettingsChanged(CVedMovie& aMovie, TInt aIndex);
	    void NotifyVideoClipColorEffectChanged(CVedMovie& aMovie, TInt aIndex);
	    void NotifyVideoClipAudioSettingsChanged(CVedMovie& aMovie, TInt aIndex);
		void NotifyVideoClipGeneratorSettingsChanged(CVedMovie& aMovie, TInt aIndex);
	    void NotifyVideoClipDescriptiveNameChanged(CVedMovie& aMovie, TInt aIndex);
		void NotifyStartTransitionEffectChanged(CVedMovie& aMovie);
		void NotifyMiddleTransitionEffectChanged(CVedMovie& aMovie, TInt aIndex);
		void NotifyEndTransitionEffectChanged(CVedMovie& aMovie);
		void NotifyAudioClipAdded(CVedMovie& aMovie, TInt aIndex);
		void NotifyAudioClipAddingFailed(CVedMovie& aMovie, TInt aError);
		void NotifyAudioClipRemoved(CVedMovie& aMovie, TInt aIndex);
		void NotifyAudioClipIndicesChanged(CVedMovie& aMovie, TInt aOldIndex, TInt aNewIndex);
		void NotifyAudioClipTimingsChanged(CVedMovie& aMovie, TInt aIndex);
		void NotifyMovieQualityChanged(CVedMovie& aMovie);
	    void NotifyMovieReseted(CVedMovie& aMovie);
		void NotifyMovieOutputParametersChanged(CVedMovie& aMovie);
	    void NotifyAudioClipDynamicLevelMarkInserted(CVedMovie& aMovie, TInt aClipIndex,  TInt aMarkIndex);
		void NotifyAudioClipDynamicLevelMarkRemoved(CVedMovie& aMovie, TInt aClipIndex, TInt aMarkIndex);
		void NotifyVideoClipDynamicLevelMarkInserted(CVedMovie& aMovie, TInt aClipIndex, TInt aMarkIndex);
		void NotifyVideoClipDynamicLevelMarkRemoved(CVedMovie& aMovie, TInt aClipIndex, TInt aMarkIndex);

	private: //from MVedMovieProcessingObserver

	    void NotifyMovieProcessingStartedL(CVedMovie& aMovie);
	    void NotifyMovieProcessingProgressed(CVedMovie& aMovie, TInt aPercentage);
		void NotifyMovieProcessingCompleted(CVedMovie& aMovie, TInt aError);
		
    public: // from MVedAudioClipInfoObserver 

        void NotifyAudioClipInfoReady( CVedAudioClipInfo& aInfo, TInt aError );

	public: //from MVeiImageClipGeneratorObserver

		void NotifyImageClipGeneratorInitializationComplete(CVeiImageClipGenerator& aGenerator, TInt aError);
    
	private: // From MCoeForegroundObserver

		virtual void HandleGainingForeground();
		virtual void HandleLosingForeground();

	private: // Construct

		CSimpleVideoEditorImpl(MSimpleVideoEditorExitObserver& aExitObserver);
		void ConstructL();

	private: // New functions

		// Helper functions to keep RunL function smaller
		void InitializeOperationL();
		void GetMergeInputFileL();
		void GetAudioFileL();
		void GetTextL();

		TInt ShowListQueryL( TInt& aPosition, TInt aHeadingResourceId, TInt aQueryResourceId ) const;	
        
        void StartMovieProcessingL(const TDesC& aSourceFile);
        
        void RestoreOrientation();
        
        /** FilterError
		*
		*	Sets error code based on current values of iState and iOperationMode
		*	i.e this is a context sensitive error mapping function
		*
		*	@param 
		*	@return new error code
		*/
        TInt FilterError() const;

		/** CompleteRequest
		*
		*	Force RunL
		*
		*	@param -
		*	@return -
		*/
		void CompleteRequest();

		/**
        *   Shows error note with given message.
        * 
        *   @param aResourceId No description.         
        */
        void ShowErrorNote( const TInt aResourceId ) const;

		/**
		*   HandleError
		* 
		*   @param aErr Error code         
		*/
		void HandleError();
				
		/*	QueryAudioInsertionL
		*
		*	Launches a query dialog in case audio clip is shorter or longer
		*   than the video clip
		*
		*   @param - 
		*   @return 0 if user selects "No", 
		*           1 if video and audio clips are the same length or
		*             the user selects "Yes"
		*/												
        TInt QueryAudioInsertionL( TTimeIntervalMicroSeconds aVideoDuration, 
                                   TTimeIntervalMicroSeconds aAudioDuration );										
		
		/**
		*   ProcessingOkL
		* 
		*   Called after succesfull processing
		*/
		void ProcessingOkL();
		
		/**
		*   ProcessingFailed
		* 
		*   Called after unsuccesfull processing
		*/
		void ProcessingFailed();
		
	    /**	QueryAndSaveL
		*   
		*	Displays Avkon file handling queries
		*
		* 	@param aSourceFileName  name of the video clip that
		*							is selected to be merged
		*   @return TInt 1 if the video should be saved,
		*				 0 if the user has cancelled the saving
		*	
		*/		
		TInt QueryAndSaveL(const TDesC& aSourceFileName); 
		
		/*	LaunchSaveVideoQueryL 
		*
		*	Launches a query dialog "Save video:" with items
		*	"Replace original" and "Save with a new name"
		*
		*   @param - 
		*   @return - list query id or -1 if the user selects No
		*/        		
		TInt LaunchSaveVideoQueryL (); 
		
		/*	LaunchSaveChangesQueryL
		*
		*	Launches a query dialog "Save changes?" query.
		*
		*   @param - 
		*   @return 0 if user selects "No", otherwise 1
		*/		
		TInt LaunchListQueryDialogL (MDesCArray *	aTextItems,
										const TDesC &	aPrompt); 
								
		void ResolveCaptionNameL( TApaAppCaption& aCaption ) const;

	private: // Data

		enum TOperationMode
			{
			EOperationModeMin = 0, // invalid
			EOperationModeMerge,
			EOperationModeMergeWithVideo,
			EOperationModeMergeWithImage,
			EOperationModeChangeAudio,
			EOperationModeAddText,
			EOperationModeMax      // invalid
			} iOperationMode;
					
		enum TState
			{
			EStateMin = 0, // Invalid
			EStateInitializing,
			EStateInsertInputFirst,
			EStateInsertInputSecond,
			EStateInsertVideo,
			EStateCreateImageGenerator,
			EStateInsertImage,
			EStateCheckAudioLength,
			EStateInsertAudio,
			EStateInsertTextToBegin,
			EStateInsertTextToEnd,
			EStateProcessing,
			EStateProcessingOk,
			EStateProcessingFailed,
			EStateFinalizing,
			EStateReady,
			EOpMax      // Invalid
			} iState;

		CEikonEnv&				iEnv;

		// Dummy control to eat key presses while dialogs are not active
		CDummyControl*			iDummyControl;

		// Interface to notify completion
		MSimpleVideoEditorExitObserver& iExitObserver;

		RConeResourceLoader 	iResLoader;

		// 
		CVedMovie*              iMovie;

        // Temporary file name for storing preview clip.
        HBufC*                  iTempFile;

		// Owned by iMovie
		CVeiImageClipGenerator* iImageClipGenerator;
		
		// Owned by iMovie
		CVeiTitleClipGenerator* iTextGenerator;

		// Input and output file names
		TFileName 				iSourceFileName;
		TFileName				iMergeFileName;
		TFileName 				iAudioFileName;
		HBufC*					iAddText;
		TFileName 				iOutputFileName;

		// Store the original orientation when forcing to portrait
		CAknAppUiBase::TAppUiOrientation iOriginalOrientation;

		// For process priority manipulation
		TProcessPriority 		iOriginalProcessPriority;
		TBool 					iProcessPriorityAltered;

		// Progress dialog.
		CAknProgressDialog*		iProgressDialog;

		// Progress dialog.
		CExtProgressDialog*		iAnimatedProgressDialog;

		// Wait dialog.
		CAknWaitDialog*			iWaitDialog;

		TInt 					iPercentagesProcessed;
		TInt 					iCancelPercentage;
		TInt					iError;
		
		// Position where the image or video will be merged
		// 0 if to the beginning, 1 if to the end.
		TInt					iVideoOrImageIndex; 
			
		/** Error UI */
		CErrorUI*            iErrorUI; 
        
        /** Pointer to an instance of CVedAudioClipInfo */
        CVedAudioClipInfo* iAudioClipInfo;

		/** Allowed audio mime types, used as playback in video */
		CDesCArrayFlat *iAcceptedAudioTypes;
		
		/** Used to figure out if the CVeiImageClipGenerator::NewL() has
		 *  completed in a situation where the action is cancelled. */
		TBool				iGeneratorComplete;
		TBool				iDialogDismissed;
	};

//</IBUDSW>
#endif

// End of file
