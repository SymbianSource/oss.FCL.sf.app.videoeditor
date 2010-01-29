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



#ifndef VEITRIMFORMMSVIEW_H
#define VEITRIMFORMMSVIEW_H

// INCLUDES
// System includes
#include <aknview.h>    // CAknView
#include <CAknMemorySelectionDialog.h> 
#include <VedMovie.h>   // CVedmovie
// User includes
#include "VeiSettings.h"

// FORWARD DECLARATIONS
class CAknNavigationDecorator;
class CAknProgressDialog;
class CVeiErrorUI;
class CSendUi;
class CVeiTrimForMmsContainer;
class CVedMovie;
class CMessageData;
class CEikProgressInfo;
class CVeiTempMaker;



// CLASS DECLARATION

/**
 *
 */
class CVeiTrimForMmsView: public CAknView,
                          public MVedMovieObserver,
                          public MVedMovieProcessingObserver
						  
{
public:
    // Constructors and destructor

    static CVeiTrimForMmsView* NewL( CSendUi& aSendAppUi );

    static CVeiTrimForMmsView* NewLC( CSendUi& aSendAppUi );

    virtual ~CVeiTrimForMmsView();

    /**
     * Preview states
     */
    enum TPreviewState
    {
        EIdle = 0,
        ELoading,
        EPreview,
        EPlaying,
        EStop,
        EStopAndExit,
        EOpeningFile,
        EPause
    };

public:
    void UpdateNaviPaneL( const TInt& aSizeInBytes,
                          const TTimeIntervalMicroSeconds& aTime );

    void UpdateNaviPaneSize();

    /**
     * Trim states
     */
    enum TTrimState
    {
        ESeek = 0,			
        EFullPreview
    };

    void SetTrimStateL( TTrimState aState );

    /**
     * From <code>MProgressDialogCallback</code>, callback method gets
     * called when a dialog is dismissed.
     *
     * @param aButtonId  Button id.
     */
    virtual void DialogDismissedL( TInt aButtonId );
    void ProcessNeeded( TBool aProcessNeed );

    /**
     * Shows global error note for given error.
     * 
     * @param aError No description.
     */
    void ShowGlobalErrorNoteL( TInt aError = 0 )const;

    /** 
     * Handles a change to the control's resources.
     */
    void HandleResourceChange( TInt aType );

private:
    // From CAknView

    TUid Id()const;

    void HandleCommandL( TInt aCommand );

    void DoActivateL( const TVwsViewId& aPrevViewId,
                      TUid aCustomMessageId,
                      const TDesC8& aCustomMessage );

    void DoDeactivate();

    void HandleStatusPaneSizeChange();

    void ReadSettingsL( TVeiSettings& aSettings )const;

protected:
    // From MVedMovieObserver

    virtual void NotifyVideoClipAdded( CVedMovie& aMovie, TInt aIndex );

    virtual void NotifyVideoClipAddingFailed( CVedMovie& aMovie, TInt aError );

    virtual void NotifyVideoClipRemoved( CVedMovie& aMovie, TInt aIndex );

    virtual void NotifyVideoClipIndicesChanged( CVedMovie& aMovie, 
                                                TInt aOldIndex, 
                                                TInt aNewIndex );

    virtual void NotifyVideoClipTimingsChanged( CVedMovie& aMovie,
                                                TInt aIndex );

    virtual void NotifyVideoClipColorEffectChanged( CVedMovie& aMovie,
                                                    TInt aIndex );

    virtual void NotifyVideoClipAudioSettingsChanged( CVedMovie& aMovie,
                                                      TInt aIndex );
                                                      
    virtual void NotifyVideoClipGeneratorSettingsChanged( CVedMovie& aMovie,
                                                          TInt aIndex );

    virtual void NotifyVideoClipDescriptiveNameChanged( CVedMovie& aMovie,
                                                        TInt aIndex );

    virtual void NotifyStartTransitionEffectChanged( CVedMovie& aMovie );

    virtual void NotifyMiddleTransitionEffectChanged( CVedMovie& aMovie, 
                                                      TInt aIndex );

    virtual void NotifyEndTransitionEffectChanged( CVedMovie& aMovie );

    virtual void NotifyAudioClipAdded( CVedMovie& aMovie, TInt aIndex );

    virtual void NotifyAudioClipAddingFailed( CVedMovie& aMovie, TInt aError );

    virtual void NotifyAudioClipRemoved( CVedMovie& aMovie, TInt aIndex );

    virtual void NotifyAudioClipIndicesChanged( CVedMovie& aMovie, 
                                                TInt aOldIndex,
                                                TInt aNewIndex );

    virtual void NotifyAudioClipTimingsChanged( CVedMovie& aMovie,
                                                TInt aIndex );

    virtual void NotifyMovieQualityChanged( CVedMovie& aMovie );

    virtual void NotifyMovieReseted( CVedMovie& aMovie );

    virtual void NotifyMovieOutputParametersChanged( CVedMovie& aMovie );

    virtual void NotifyAudioClipDynamicLevelMarkInserted( CVedMovie& aMovie, 
                                                          TInt aClipIndex, 
                                                          TInt aMarkIndex );

    virtual void NotifyAudioClipDynamicLevelMarkRemoved( CVedMovie& aMovie, 
                                                         TInt aClipIndex, 
                                                         TInt aMarkIndex );

    virtual void NotifyVideoClipDynamicLevelMarkInserted( CVedMovie& aMovie, 
                                                          TInt aClipIndex, 
                                                          TInt aMarkIndex );

    virtual void NotifyVideoClipDynamicLevelMarkRemoved( CVedMovie& aMovie, 
                                                         TInt aClipIndex, 
                                                         TInt aMarkIndex );  

protected:
    // From MVedMovieProcessingObserver

    /**
     * Called to notify that a new movie processing operation has been started. 
     *
     * @param aMovie  movie
     */
    void NotifyMovieProcessingStartedL( CVedMovie& aMovie );

    /**
     * Called to inform about the current progress of the movie processing operation.
     *
     * @param aMovie       movie
     * @param aPercentage  percentage of the operation completed, must be 
     *                     in range 0..100
     */
    void NotifyMovieProcessingProgressed( CVedMovie& aMovie, TInt aPercentage );

    /**
     * Called to notify that the movie processing operation has been completed. 
     * 
     * @param aMovie  movie
     * @param aError  error code why the operation was completed. 
     *                <code>KErrNone</code> if the operation was completed 
     *                successfully.
     */
    void NotifyMovieProcessingCompleted( CVedMovie& aMovie, TInt aError );

private:
    // New functions

    void CmdSoftkeyCancelL();

    /**
     * 
     */
    void CmdSoftkeyOkL();


    /**
     * Send via multimedia command handling.
     */
    void CmdSendViaMultimediaL();

    void CmdSoftkeyBackL();

    void PushKeySoundL( const TInt aResourceId )const;

    void PopKeySound()const;

    /**
     * Sets the text for title pane.
     */
    void SetTitlePaneTextL()const;

    void CreateNaviPaneL();

    void SetNaviPaneDurationLabelL( const TTimeIntervalMicroSeconds& aTime );

    void SetNaviPaneSizeLabelL( const TInt& aSizeInBytes );

    /**
     * Start full screen preview.
     */
    void PlayPreviewL();

private:
    // Constructors

    CVeiTrimForMmsView( CSendUi& aSendAppUi );

    void ConstructL();

private:
    // Data

    /** 
     * Cut in time in microseconds (clip timebase) for trimmed video.
     */
    TTimeIntervalMicroSeconds iCutInTime;

    /** 
     * Cut out time in microseconds (clip timebase) for trimmed video.
     */
    TTimeIntervalMicroSeconds iCutOutTime;

    /**
     * Progress note for saving the trimmed video
     */
    CAknProgressDialog* iProgressNote;

    /**
     * Progress info for the progress dialog.
     */
    CEikProgressInfo* iProgressInfo;

    /**
     * Container
     */
    CVeiTrimForMmsContainer* iContainer;

    /**
     * Navigation pane decorator
     */
    CAknNavigationDecorator* iNaviDecorator;

    /**
     *
     */
    CSendUi& iSendAppUi;

    /**
     *
     */
    CVedMovie* iVedMovie;

    /**
     * Previous view which activated this view.
     */
    TVwsViewId iPreviousViewId;

    /**
     * Utility class to display error notes by applications. 
     */
    CVeiErrorUI* iErrorUi;

    /**
     * No description.
     */
    CVeiTempMaker* iTempMaker;

    /**
     * Temporary file name for storing preview/send clip.
     */
    HBufC* iTempFile;


    TTrimState iTrimState;

    TVeiSettings iMovieSaveSettings;

    /*
     * Indecates if process needed.
     */
    TBool iProcessNeeded;



};

#endif
