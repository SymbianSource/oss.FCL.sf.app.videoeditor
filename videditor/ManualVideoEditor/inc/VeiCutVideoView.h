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


#ifndef VEICUTVIDEOVIEW_H
#define VEICUTVIDEOVIEW_H


#include <aknview.h>
#include <VedMovie.h>
#include <VedCommon.h>
#include <aknprogressdialog.h> 
#include <utility.h>
#include <caknmemoryselectiondialog.h> 

#include "VeiCutVideoContainer.h" 


class CVeiCutVideoContainer;
class CAknTitlePane;
class CAknNavigationDecorator;
class CAknNavigationControlContainer;
class CPeriodic;
class CVeiErrorUI;
class CAknMemorySelectionDialog;

/**
 *  CVeiCutVideoView view class.
 * 
 */
class CVeiCutVideoView: public CAknView, 
                        public MVedMovieObserver,
                        public MVedMovieProcessingObserver, 
                        public MProgressDialogCallback 
{
public:

    /**
     * Default constructor.
     */
    void ConstructL();

    /**
     * Destructor.
     */
    virtual ~CVeiCutVideoView();

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
    void DynInitMenuPaneL( TInt aResourceId, CEikMenuPane* aMenuPane );

    /** 
     * From MVedMovieProcessingObserver
     */
    virtual void NotifyMovieProcessingStartedL( CVedMovie& aMovie );
    virtual void NotifyMovieProcessingProgressed( CVedMovie& aMovie, 
                                                  TInt aPercentage );
    virtual void NotifyMovieProcessingCompleted( CVedMovie& aMovie, 
                                                 TInt aError );

    /**
     * From MVedMovieObserver
     */
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
                                                
    virtual void NotifyMovieReseted( CVedMovie& aMovie );

    virtual void NotifyVideoClipGeneratorSettingsChanged( CVedMovie& aMovie,
                                                          TInt aIndex );

    virtual void NotifyVideoClipDescriptiveNameChanged( CVedMovie& aMovie, 
                                                        TInt aIndex );
                                                        
    virtual void NotifyMovieQualityChanged( CVedMovie& aMovie );				

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
                                                         TInt aMarkIndex);

    /**
     * From MProgressDialogCallback
     */
    void DialogDismissedL( TInt aButtonId );


public:
    /**
     * 
     * 
     * @return VeiCutVideoView Uid
     */
    TUid Id()const;

    /**
     * Starts temporary clip processing.
     */
    void GenerateEffectedClipL();

    /**
     * From CAknView, HandleCommandL(TInt aCommand);
     *
     * @param aCommand
     */
    void HandleCommandL( TInt aCommand );

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
    void SetVideoClipAndIndex( CVedMovie& aVideoClip, TInt aIndex );

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

    /**
     * Static callback function for hiding the volume indicator.
     *
     * @param aPtr  self pointer
     *
     * @return dummy value
     */
    static TInt HideVolumeCallbackL( TAny* aPtr );

    /**
     * Hides the volume indicator.
     */
    void HideVolume();

    /** Possible mark states */
    enum TMarkState
    {
        EMarkStateIn,
        EMarkStateOut,
        EMarkStateInOut
    };

    void HandleStatusPaneSizeChange();

    /**
     * No description.
     *
     * 
     * @return No description.
     */
    TBool IsEnoughFreeSpaceToSaveL(); // const;
    TTimeIntervalMicroSeconds GetVideoClipCutInTime();
    TTimeIntervalMicroSeconds GetVideoClipCutOutTime();

    /** 
     * Handles a change to the control's resources.
     */
    void HandleResourceChange( TInt aType );
    

    /**
    * Moves the start or end mark when user drags them.
    * 
    * @param aPosition	position where the mark is moved to
    * @param aMarkType  EStartMark or EEndMark
    * @return -
    */               
    void MoveStartOrEndMarkL( TTimeIntervalMicroSeconds aPosition, CVeiCutVideoContainer::TCutMark aMarkType );
		
    

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
     * Creates the time label navi. 
     *
     * @return  time label navi 
     */
    CAknNavigationDecorator* CreateTimeLabelNaviL();

    /**
     * Static callback function for the periodical timer that updates
     * the time navi.
     *
     * @param aPtr  self pointer
     *
     * @return dummy value
     */
    static TInt UpdateTimeCallbackL( TAny* aPtr );

private:

    /**
     * From AknView, DoActivateL.
     * 
     * @param aPrevViewId  previous view id
     * @param aCustomMessageId  custom message id
     * @param aCustomMessage  custom message
     */
    void DoActivateL( const TVwsViewId& aPrevViewId, TUid aCustomMessageId,
                     const TDesC8& aCustomMessage );

    /**
     * From AknView, DoDeactivate
     */
    void DoDeactivate();

private:
    // Data
    /* cut video container	*/
    CVeiCutVideoContainer* iContainer;

    /* index of the video clip in the movie	*/
    TUint iIndex;

    /* name of the temp file. possibly effected*/
    HBufC* iProcessedTempFile;

    /* movie */
    CVedMovie* iMovie;

    /* temp movie. used to create effected clip */
    CVedMovie* iTempMovie;

    /** Time updater. */
    CPeriodic* iTimeUpdater;

    /** Pointer to the navi pane. */
    CAknNavigationControlContainer* iNaviPane;

    /** Time navi. */
    CAknNavigationDecorator* iTimeNavi;

    /** Progress note. */
    CAknProgressDialog* iProgressNote;

    /** Progress info for the progress note. */
    CEikProgressInfo* iProgressInfo;

    /** Volume hiding timer. */
    CPeriodic* iVolumeHider;

    /** Volume navi decorator. */
    CAknNavigationDecorator* iVolumeNavi;

    /** Popup menu state flag */
    TBool iPopupMenuOpened;

    /** Audio muted flag */
    TBool iAudioMuted;

    /** current mark state */
    TMarkState iMarkState;

    /** Error number */
    TInt iErrorNmb;

    /** Error UI */
    CVeiErrorUI* iErrorUI;

    CAknMemorySelectionDialog::TMemory iMemoryInUse;

    TVeiSettings iMovieSaveSettings;


#include "veicutvideoview.inl"
};

#endif 

// End of File
