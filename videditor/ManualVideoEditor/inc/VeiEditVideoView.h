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



#ifndef VEDEDITVIDEOVIEW_H
#define VEDEDITVIDEOVIEW_H

//  INCLUDES
#include <aknview.h>
#include <maknfilefilter.h> 
#include <aknprogressdialog.h> 
#include <VedMovie.h>
#include <utility.h>
#include <caknmemoryselectiondialog.h> 
#include "VeiAddQueue.h"
#include "VeiTitleClipGenerator.h"
#include "VeiImageClipGenerator.h"
#include "VeiPopup.h"

#include "veisettingsview.h"

#include <AknQueryDialog.h> 

//  CONSTANTS
const TUid KView4Id = {4};
const TInt KMinVolume = 1;
const TInt KMaxVolume = 10;

//  FORWARD DECLARATIONS
class CVeiEditVideoContainer;
class CVeiCutVideoView;
class CVeiCutAudioView;
class CAknNaviLabel;
class CVeiEditVideoLabelNavi;
class CAknNavigationControlContainer;
class CAknNavigationDecorator;
class MGFetch;
class CAknProgressDialog;
class CSendUi;
class CVedMovie;
class CMdaAudioConvertUtility;
class MAknsDataContext;
class CMdaAudioRecorderUtility;
class CVeiErrorUI;
class CVeiTempMaker;
class CAknMemorySelectionDialog;
class CAknWaitDialog;

class CVeiAddQueue;

class CVeiTextDisplay;
class CVeiPopup;



//  CLASS DECLARATION

/**
 *  CVeiEditVideoView view class.
 */
class CVeiEditVideoView: public CAknView, 
                         public MProgressDialogCallback, 
                         public MVedMovieObserver, 
                         public MVedMovieProcessingObserver,
                         public MVedAudioClipInfoObserver, 
                         public MMdaObjectStateChangeObserver,
                         public MVeiImageClipGeneratorObserver,
                         public MVeiTitleClipGeneratorObserver,
                         public MVeiQueueObserver

{

public:
    //Constructors and destructor

    /**
     * Static factory constructor.
     *
     * @param aCutView Instance of cut video view.
     * @param aCutAudioView No description.
     * @param aSendAppUi No description.
     * @return Created <code>CVeiEditVideoView</code> instance.
     */
    static CVeiEditVideoView* NewL( CVeiCutVideoView& aCutView, 
                                    CVeiCutAudioView& aCutAudioView, 
                                    CSendUi& aSendAppUi );

    /**
     * Static factory constructor. Leaves the created object in the
     * cleanup stack.
     *
     * @param aCutView Instance of cut video view.
     * @param aCutAudioView No description.
     * @param aSendAppUi No description.
     * @return Created <code>CVeiEditVideoView</code> instance.
     */
    static CVeiEditVideoView* NewLC( CVeiCutVideoView& aCutView,
                                     CVeiCutAudioView& aCutAudioView, 
                                     CSendUi& aSendAppUi );

    /**
     * Destructor.
     */
    virtual ~CVeiEditVideoView();

    TUid Id()const;

public:
    // New functions

    void CancelWaitDialog( TInt aError = 0 );
    /**
     * Inserts video clip to movie
     *
     * @param aFilename Video clip to insert.
     * @param aStartNow No description.
     * @return <code>KErrNone</code> if process was started,
     *		   <code>KErrNotReady</code> if movie was not ready.
     */
    TInt AddClipL( const TDesC& aFilename, TBool aStartNow );

    /**
     * Shows error note with given message.
     * 
     * @param aResourceId No description.
     * @param aError No description.
     */
    void ShowErrorNote( const TInt aResourceId, TInt aError = 0 )const;

    /**
     * Shows global error note for given error.
     * There are empirically tested error codes for which platform shows something
     * For other ones platform does not show anything and for them default string is showed.
     * These error codes must be individually tested for every phone model.
     *
     * @param aError standard error code
     */
    void ShowGlobalErrorNote( TInt aError = 0 )const;

    /**
     * Returns audio duration of the movie.
     * 
     * @return  movie audio duration in microseconds
     */
    TTimeIntervalMicroSeconds OriginalAudioDuration()const;

    /**
     * Inserts next video clip to movie from videoclip array.
     */
    void AddNext();

    /**
     * Processing wait modes
     */
    enum TWaitMode
    {
        ENotWaiting = 0x6300,  // 25344
        EProcessingMovieSave,
        EProcessingMovieSaveThenQuit,
        EOpeningAudioInfo,
        EConvertingAudio,
        EProcessingMovieForCutting,
        EProcessingMoviePreview,
        EProcessingMovieSend,
        EProcessingError,
        ECuttingAudio,
        EDuplicating,
        EProcessingMovieTrimMms,
        ESlowMotion,
        EProcessingAudioError
    };

    /**
     * Editor states
     */
    enum TEditorState
    {
        EPreview = 0x6500,  // 25856
        EQuickPreview,
        EEdit,
        EBufferingVideo,
        EMixAudio,
        EAdjustVolume
    };

    void SetEditorState( TEditorState aState );

    void UpdateMediaGalleryL();

    TRect ClientOrApplicationRect( TBool aFullScreenSelected )const;

    /**
     * Stores App UI orientation.
     *
     * @return No description.
     */
    void StoreOrientation();

    /**
     * Restores the previously stored App UI orientation.
     *
     * @return No description.
     */
    void RestoreOrientation();

    /** 
     * Handles a change to the control's resources.
     */
    void HandleResourceChange( TInt aType );

public:
    // Functions from base classes

    /**
     * From <code>MProgressDialogCallback</code>, callback method gets
     * called when a dialog is dismissed.
     *
     * @param aButtonId  Button id.
     */
    virtual void DialogDismissedL( TInt aButtonId );

    /**
     * From <code>MEikMenuObserver</code>, dynamically initialises a menu
     * pane. The Uikon framework calls this function, if it is implemented
     * in a menu’s observer, immediately before the menu pane is activated.
     *
     * @param aResourceId Resource ID identifying the menu pane to
     *        initialise.
     * @param aMenuPane The in-memory representation of the menu pane.
     */
    void DynInitMenuPaneL( TInt aResourceId, CEikMenuPane* aMenuPane );

    /**
     * From <code>CAknView</code>, no description.
     *
     * @param aCommand No description.
     */
    void HandleCommandL( TInt aCommand );


    /**
     * No description.
     */
    void InsertNewAudio();

    /**
     * No description.
     */
    void SetFullScreenSelected( TBool aFullScreenSelected );

    /**
     * Starts navipane update.
     */
    void StartNaviPaneUpdateL();

    /**
     * Stops the navi pane update.
     */
    void StopNaviPaneUpdateL();

    /**
     * No description.
     */
    void ShowVolumeLabelL( TInt aVolume );

    /**
     * No description.
     */
    void DoUpdateEditNaviLabelL()const;
    virtual void HandleScreenDeviceChangedL();

    /**
     * No description.
     *
     * 
     * @return No description.
     */
    TBool IsEnoughFreeSpaceToSaveL( TInt aBytesToAdd = 0 )const;

    void SetCbaL( TEditorState aState );

protected:
    // New functions

    /**
     * Callback function to be called when the 
     * <code>iAudioRecordPeriodic</code> is scheduled after a timer event.
     * 
     * @param aThis Pointer to this class.
     * @return Always one (1); indicates that this callback function
     *         should be called again. 
     */
    static TInt UpdateAudioRecording( TAny* aThis );

    static TInt UpdateNaviPreviewing( TAny* aThis );

    /**
     * No description.
     */
    void DoUpdateAudioRecording();

    /**
     * No description.
     *
     * 
     * @return No description.
     */
    TBool IsEnoughFreeSpaceToSave2L( TInt aBytesToAdd = 0 )const;

    /**
     * Renames generated <code>iTempfile</code> and opens 
     * <code>CSendUi</code>.
     */
    void SendMovieL();

    /**
     * If needed, creates temporary video clip for CVeiPreviewView and 
     * activates PreviewView.
     */
    void StartTempFileProcessingL();

    /**
     * Shows information note with given message.
     * 
     * @param aMessage Message to show.
     */
    void ShowInformationNoteL( const TDesC& aMessage )const;

    /** 
     * Updates EditLabel
     */
    void UpdateEditNaviLabel()const;

    /**
     * Removes selected clip(audio or video) from the movie.
     * Confirmation note is shown.
     */
    void RemoveCurrentClipL();

    /**
     * Creates editlabel.
     *
     * @return No description.
     */
    CAknNavigationDecorator* CreateEditNaviLabelL();

    /**
     * Creates editlabel.
     *
     * @return No description.
     */
    CAknNavigationDecorator* CreatePreviewNaviLabelL();


    /**
     * Creates movelabel.
     *
     * @return No description.
     */
    CAknNavigationDecorator* CreateMoveNaviLabelL();

    /**
     * Save movie to file.
     *
     * @param aQuitAfterSaving	after saving move to VideosView
     * @return <code>ETrue if quit after saving,
     *         <code>EFalse</code> otherwise.
     */
    TBool SaveL( TWaitMode aQuitAfterSaving );

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

    static TInt HideVolumeCallbackL( TAny* aPtr );

    /**
     * Hides the volume indicator.
     */
    void HideVolume();

protected:
    // Functions from base classes

    /**
     * From <code>CAknView</code>, activates this view.
     * 
     * @param aPrevViewId Previous view id.
     * @param aCustomMessageId Custom message id.
     * @param aCustomMessage Custom message.
     */
    void DoActivateL( const TVwsViewId& aPrevViewId, TUid aCustomMessageId,
                     const TDesC8& aCustomMessage );

    /**
     * From <code>CAknView</code>, deactivates this view.
     */
    void DoDeactivate();

    /**
     * From <code>CAknView</code>, HandleForegroundEventL( TBool aForeground ).
     *
     * @param aForeground No description.
     */
    virtual void HandleForegroundEventL( TBool aForeground );

    virtual void NotifyQueueProcessingStarted( 
                        MVeiQueueObserver::TProcessing aMode = 
                        MVeiQueueObserver::ENotProcessing );

    virtual void NotifyQueueEmpty( TInt aInserted, TInt aFailed );

    virtual void NotifyQueueProcessingProgressed( TInt aProcessedCount, 
                                                  TInt aPercentage );

    virtual TBool NotifyQueueClipFailed( const TDesC& aFilename, TInt aError );

    /**
     * From <code>MMdaObjectStateChangeObserver</code>, no description.
     *
     * @param aObject No description.
     * @param aPreviousState No description.
     * @param aCurrentState No description.
     * @param aErrorCode No description.
     */
    virtual void MoscoStateChangeEvent( CBase* aObject, 
                                        TInt aPreviousState,
                                        TInt aCurrentState, 
                                        TInt aErrorCode );

    /**
     * From <code>MVedAudioClipInfoObserver</code>, no description.
     *
     * @param aInfo No description.
     * @param aError No description.
     */
    virtual void NotifyAudioClipInfoReady( CVedAudioClipInfo& aInfo,
                                           TInt aError );

    /**
     * From <code>MVedMovieProcessingObserver</code>, no description.
     *
     * @param aMovie No description.
     */
    virtual void NotifyMovieProcessingStartedL( CVedMovie& aMovie );

    /**
     * From <code>MVedMovieProcessingObserver</code>, no description.
     *
     * @param aMovie No description.
     * @param aPercentage No description.
     */
    virtual void NotifyMovieProcessingProgressed( CVedMovie& aMovie, 
                                                  TInt aPercentage );

    /**
     * From <code>MVedMovieProcessingObserver</code>, no description.
     *
     * @param aMovie No description.
     * @param aError No description.
     */
    virtual void NotifyMovieProcessingCompleted( CVedMovie& aMovie,
                                                 TInt aError );

    /**
     * From <code>MVedMovieObserver</code>, no description.
     *
     * @param aMovie No description.
     * @param aIndex No description.
     */
    virtual void NotifyVideoClipAdded( CVedMovie& aMovie, TInt aIndex );

    /**
     * From <code>MVedMovieObserver</code>, no description.
     *
     * @param aMovie No description.
     * @param aError No description.
     */
    virtual void NotifyVideoClipAddingFailed( CVedMovie& aMovie,
                                              TInt aError );

    /**
     * From <code>MVedMovieObserver</code>, no description.
     *
     * @param aMovie No description.
     * @param aIndex No description.
     */
    virtual void NotifyVideoClipRemoved( CVedMovie& aMovie, TInt aIndex );

    /**
     * From <code>MVedMovieObserver</code>, no description.
     *
     * @param aMovie No description.
     * @param aOldIndex No description.
     * @param aNewIndex No description.
     */
    virtual void NotifyVideoClipIndicesChanged( CVedMovie& aMovie,
                                                TInt aOldIndex, 
                                                TInt aNewIndex );

    /**
     * From <code>MVedMovieObserver</code>, no description.
     *
     * @param aMovie No description.
     * @param aIndex No description.
     */
    virtual void NotifyVideoClipTimingsChanged( CVedMovie& aMovie,
                                                TInt aIndex );

    /**
     * From <code>MVedMovieObserver</code>, no description.
     *
     * @param aMovie No description.
     * @param aIndex No description.
     */
    virtual void NotifyVideoClipColorEffectChanged( CVedMovie& aMovie,
                                                    TInt aIndex );

    /**
     * From <code>MVedMovieObserver</code>, no description.
     *
     * @param aMovie No description.
     */
    virtual void NotifyStartTransitionEffectChanged( CVedMovie& aMovie );

    /**
     * From <code>MVedMovieObserver</code>, no description.
     *
     * @param aMovie No description.
     * @param aIndex No description.
     */
    virtual void NotifyMiddleTransitionEffectChanged( CVedMovie& aMovie,
                                                      TInt aIndex );
    /**
     * From <code>MVedMovieObserver</code>, no description.
     *
     * @param aMovie No description.
     */
    virtual void NotifyEndTransitionEffectChanged( CVedMovie& aMovie );

    /**
     * From <code>MVedMovieObserver</code>, no description.
     *
     * @param aMovie No description.
     * @param aIndex No description.
     */
    virtual void NotifyAudioClipAdded( CVedMovie& aMovie, TInt aIndex );

    /**
     * From <code>MVedMovieObserver</code>, no description.
     *
     * @param aMovie No description.
     * @param aError No description.
     */
    virtual void NotifyAudioClipAddingFailed( CVedMovie& aMovie,
                                              TInt aError );

    /**
     * From <code>MVedMovieObserver</code>, no description.
     *
     * @param aMovie No description.
     * @param aIndex No description.
     */
    virtual void NotifyAudioClipRemoved( CVedMovie& aMovie, TInt aIndex );

    /**
     * From <code>MVedMovieObserver</code>, no description.
     *
     * @param aMovie No description.
     * @param aOldIndex No description.
     * @param aNewIndex No description.
     */
    virtual void NotifyAudioClipIndicesChanged( CVedMovie& aMovie, 
                                                TInt aOldIndex, 
                                                TInt aNewIndex );

    /**
     * From <code>MVedMovieObserver</code>, no description.
     *
     * @param aMovie No description.
     * @param aIndex No description.
     */
    virtual void NotifyAudioClipTimingsChanged( CVedMovie& aMovie,
                                                TInt aIndex );

    /**
     * From <code>MVedMovieObserver</code>, no description.
     *
     * @param aMovie No description.
     */
    virtual void NotifyMovieReseted( CVedMovie& aMovie );
    
    /**
     * Called to notify that the audio settings of a video clip have changed. 
     *
     * @param aMovie  movie
     * @param aClip   changed video clip
     */
    virtual void NotifyVideoClipAudioSettingsChanged( CVedMovie& aMovie,
                                                      TInt aIndex );

    /**
     * Called to notify that some generator-specific settings of 
     * a generated video clip have changed.
     *
     * @param aMovie  movie
     * @param aClip   changed video clip
     */
    virtual void NotifyVideoClipGeneratorSettingsChanged( CVedMovie& aMovie,
                                                          TInt aIndex );

    /**
     * Called to notify that the descriptive name of a clip has changed. 
     *
     * @param aMovie  movie
     * @param aIndex  changed video clip index
     */
    virtual void NotifyVideoClipDescriptiveNameChanged( CVedMovie& aMovie, 
                                                        TInt aIndex );
   
    /**
     * Called to notify that the quality setting of the movie has been
     * changed.
     *
     * @param aMovie  movie
     */
    virtual void NotifyMovieQualityChanged( CVedMovie& aMovie );

    /**
     * Called to notify that the output parameters have been changed
     *
     * @param aMovie  movie
     */
    virtual void NotifyMovieOutputParametersChanged( CVedMovie& aMovie );

    /**
     * Called to notify that a dynamic level mark has been inserted 
     * to an audio clip.
     *
     * @param aMovie       movie
     * @param aClipIndex   audio clip index
     * @param aMarkIndex   index of the inserted level mark
     */
    virtual void NotifyAudioClipDynamicLevelMarkInserted( CVedMovie& aMovie, 
                                                          TInt aClipIndex, 
                                                          TInt aMarkIndex );

    /**
     * Called to notify that a dynamic level mark has been inserted 
     * to an audio clip.
     *
     * @param aMovie       movie
     * @param aClipIndex   audio clip index
     * @param aMarkIndex   index of the inserted level mark
     */
    virtual void NotifyAudioClipDynamicLevelMarkRemoved( CVedMovie& aMovie, 
                                                         TInt aClipIndex, 
                                                         TInt aMarkIndex );

    /**
     * Called to notify that a dynamic level mark has been inserted 
     * to an audio clip.
     *
     * @param aMovie       movie
     * @param aClipIndex   audio clip index
     * @param aMarkIndex   index of the inserted level mark
     */
    virtual void NotifyVideoClipDynamicLevelMarkInserted( CVedMovie& aMovie, 
                                                          TInt aClipIndex, 
                                                          TInt aMarkIndex );

    /**
     * Called to notify that a dynamic level mark has been inserted 
     * to an audio clip.
     *
     * @param aMovie       movie
     * @param aClipIndex   audio clip index
     * @param aMarkIndex   index of the inserted level mark
     */
    virtual void NotifyVideoClipDynamicLevelMarkRemoved( CVedMovie& aMovie, 
                                                         TInt aClipIndex, 
                                                         TInt aMarkIndex );    


    /**
     * From MVedImageClipGeneratorObserver
     */
    virtual void NotifyImageClipGeneratorInitializationComplete(
    	                                    CVeiImageClipGenerator& aGenerator,
    	                                    TInt aError );

    virtual void NotifyTitleClipBackgroundImageLoadComplete(
                                            CVeiTitleClipGenerator& aGenerator,
                                            TInt aError);


private:

    void UpdateInsertingProgressNoteL( TInt aProcessed );
    
    /**
     * C++ default constructor.
     *
     * @param aCutView	instance of cut video view
     * @param aVideoList instance of videolist
     */
    CVeiEditVideoView( CVeiCutVideoView& aCutView, 
                       CVeiCutAudioView& aCutAudioView,
			           CSendUi& aSendAppUi );


    /**
     * Symbian 2nd phase constructor.
     */
    void ConstructL();


    void ShowAndHandleSendMenuCommandsL();

    void BrowseStartTransition( TBool aUpOrDown );

    /**
     * Check is videoclip mms compatible.
     *
     * @param 
     */
    void MmsSendCompatibleCheck();


    /**
     * this function decides whether audio mixing can be made		 
     */
    TBool MixingConditionsOk()const;

    /**
     * this function fades either the videos' audio level or the imported sounds' audio level in the movie
     *		  
     */
    void MixAudio();


    /**
     * this function removes dynamic level marks from all the video and audio clip(s) in the movie		 
     *		  
     */
    //void RemoveAudioMixingLevelMarks();

    /**
     * this function removes dynamic level marks from the individual video clip(s) in the movie
     * @param aIndex       index of video clip where marks are to be removed from
     * if aIndex is empty or has negative value, marks are removed from every video clip currently in the movie
     *		  
     */
    //void RemoveVideoClipVolumeLevelMarks(TInt aIndex = -1);

    /**
     * this function removes video dynamic level marks from the individual audio clip(s) in the movie
     * @param aIndex       index of audio clip where marks are to be removed from
     * if aIndex is empty or has negative value, marks are removed from every audio clip currently in the movie
     *		  
     */
    //void RemoveAudioClipVolumeLevelMarks(TInt aIndex = -1);

    /**
     * this function adjusts volume of the video's audio or the imported audio
     *		  
     */
    void AdjustVolumeL();

    /** Callback function */
    static TInt AsyncBackSend( TAny* aThis );
    /** Callback function */
    static TInt AsyncBackSaveThenExitL( TAny* aThis );

    /**
     * This function is called to tell whether the movie's state has changed so that new temporary movie 
     * file have to be processed. The file is needed in previewing for example. 
     * The processing starts when the temporary movie file is next time needed.
     * @param aUpdateNeeded tells whether new temporary movie file is needed 		 
     */
    void SetNewTempFileNeeded( const TBool aUpdateNeeded );

    /*
    There is an GetDurationEstimateL() method in CVedMovie that 
    estimates end cutpoint with given target size and start cutpoint for current movie.
    This method gives end cutpoint time in movie's time scale. 

    If end cutpoint is not in the first clip but in some subsequent clip things are not straightforward.
    The clip accomodatint the end cutpoint must be localized as well as the exact timepoint in clip's
    time scale.

    If clips have been edited (e.g. marks set) before call to GetDurationEstimateL(), things get
    more complicated.

    If slow motion is applied to clips things get even more complicated.

    There is no resrictions of usage presented in the documentation of CVedMovie::GetDurationEstimateL() (vedmovie.h) 

     */
    TBool FitsToMmsL();


private:
    // Data

    /**
     * Cut video view.
     */
    CVeiCutVideoView& iCutView;

    /**
     * Cut audio view.
     */
    CVeiCutAudioView& iCutAudioView;

    /**
     * No description.
     */
    CSendUi& iSendAppUi;

    /**
     * No description.
     */
    CVedAudioClipInfo* iAudioClipInfo;

    /**
     * No description.
     */
    CAknNavigationControlContainer* iNaviPane;

    /**
     * Edit label.
     */
    CAknNavigationDecorator* iEditLabel;

    /**
     * Preview label.
     */
    CAknNavigationDecorator* iPreviewLabel;

    /** 
     * Move label.
     */
    CAknNavigationDecorator* iMoveLabel;

    /**
     * Progress info for the progress dialog.
     */
    //CEikProgressInfo*       iProgressInfo;

    /**
     * Progress dialog.
     */
    CAknProgressDialog* iProgressNote;

    /**
     * Original audio clip index.
     */
    TInt iOriginalAudioClipIndex;

    /**
     * Original video clip index.
     */
    TInt iOriginalVideoClipIndex;

    /**
     * Original audio clip starting point.
     */
    TTimeIntervalMicroSeconds iOriginalAudioStartPoint;

    /**
     *Original audio clip duration.
     */
    TTimeIntervalMicroSeconds iOriginalAudioDuration;

    /**
     * Original video clip cut in time.
     */
    TTimeIntervalMicroSeconds iOriginalCutInTime;
    TTimeIntervalMicroSeconds iOriginalCutOutTime;

    /**
     * Original audio clip cut in time.
     */
    TTimeIntervalMicroSeconds iOriginalAudioCutInTime;
    TTimeIntervalMicroSeconds iOriginalAudioCutOutTime;

    TTimeIntervalMicroSeconds iOriginalVideoStartPoint;

    TTimeIntervalMicroSeconds iOriginalVideoCutInTime;

    TTimeIntervalMicroSeconds iOriginalVideoCutOutTime;

    /**
     * No description.
     */
    TInt iCutVideoIndex;

    /**
     * No description.
     */
    TInt iCutAudioIndex;

    /**
     * No description.
     */
    TUid iGivenSendCommand;

    /**
     * Temporary file name for storing preview clip.
     */
    HBufC* iTempFile;

    /**
     * Temporary file name for storing preview clip. 
     */
    HBufC* iTempRecordedAudio;

    /**
     * Wait mode; are we waiting for progress or wait note? 
     */
    TWaitMode iWaitMode;

    /**
     * No description.
     */
    TBool iMovieSavedFlag;

    /**
     * For indicating the first movie clip adding. 
     */
    TBool iMovieFirstAddFlag;

    /**
     * No description.
     */
    TBool iUpdateTemp;

    /**
     * No description.
     */
    TBool iConverting;

    /**
     * No description.
     */
    CMdaAudioRecorderUtility* iRecorder;

    /**
     * No description.
     */
    CPeriodic* iAudioRecordPeriodic;

    CPeriodic* iPreviewUpdatePeriodic;

    /** Volume navi decorator. */
    CAknNavigationDecorator* iVolumeNavi;

    /**
     * No description.
     */
    TTimeIntervalMicroSeconds iRecordedAudioMaxDuration;

    /**
     * No description.
     */
    CVeiErrorUI* iErrorUI;

    /**
     * No description.
     */
    TInt iErrorNmb;

    /**
     * No description.
     */
    HBufC* iSaveToFileName;

    /**
     * No description.
     */
    TBool iSendKey;

    /**
     * No description.
     */
    CVeiTempMaker* iTempMaker;

    TEditorState iEditorState;

    CAknWaitDialog* iWaitDialog;

    CVedVideoClipGenerator* iGenerator;

    CVeiAddQueue* iMediaQueue;

    TBool iFullScreenSelected;

    /** Volume hiding timer. */
    CPeriodic* iVolumeHider;


    /**
     * Indicates whether or not the memory card accessibility is checked.
     * Usually accessibility is checked only once.
     */
    TBool iMemoryCardChecked;

    CVeiPopup* iPopup;

    /**
     * No description.
     */
    CVedMovie* iMovie;

    /**
     * No description.
     */
    CVeiEditVideoContainer* iContainer;

    TInt iOriginalVideoSpeed;

    TVeiSettings iMovieSaveSettings;
    CAknMemorySelectionDialog::TMemory iMemoryInUse;


    CVedMovie::TVedMovieQuality iBackupSaveQuality;

    TBool iChangedFromMMCToPhoneMemory;

    TInt iPercentProcessed;

    /**
     * Store the original orientation when forcing to portrait
     */
    CAknAppUiBase::TAppUiOrientation iOriginalOrientation;

    /** Callback utility */
    CAsyncCallBack* iCallBack;

#include "veieditvideoview.inl"
};

#endif // VEDEDITVIDEOVIEW_H

// End of File
