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


#ifndef VEICUTAUDIOVIEW_H
#define VEICUTAUDIOVIEW_H


#include <aknview.h>
#include <VedMovie.h>
#include <VedCommon.h>
#include <utility.h>

#include "VeiCutAudioContainer.h" 


class CVeiCutAudioContainer;
class CAknTitlePane;
class CAknNavigationDecorator;
class CAknNavigationControlContainer;
class CPeriodic;
class CMdaAudioRecorderUtility;
class CMdaAudioType;
class TMdaClipLocation;
class CVeiErrorUI;

/**
 *  CVeiCutAudioView view class.
 * 
 */
class CVeiCutAudioView: public CAknView
{
public:
    // Constructors and destructor

    /**
     * Two-phased constructor.
     */
    static CVeiCutAudioView* NewL();

    /**
     * Two-phased constructor.
     */
    static CVeiCutAudioView* NewLC();

    /**
     * Destructor.
     */
    virtual ~CVeiCutAudioView();

protected:

    /** 
     * From CAknView, HandleForegroundEventL( TBool aForeground )
     *
     * @param aForeground
     */
    virtual void HandleForegroundEventL( TBool aForeground );

private:
    // From CAknView

    /**
     * From CAknView, DynInitMenuPaneL.
     *
     * @param aResourceId  resource id
     * @param aMenuPane  menu pane
     */
    void DynInitMenuPaneL( TInt aResourceId, CEikMenuPane* aMenuPane );

public:
    /**
     * From CAknView, Id.
     *
     * @return view id.
     */
    TUid Id()const;


    CVeiCutAudioView();

    /**
     * Default constructor.
     */
    void ConstructL();

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
     * Sets the volume
     *
     * @param aVolume	Volume level.
     */
    void SetVolume( TInt aVolume );

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
     * Adjusts the volume up.
     */
    void VolumeUpL();

    /**
     * Adjusts the volume down.
     */
    void VolumeDownL();

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

    /**
     * Gets visualization values.
     */
    void GetAudioVisualizationL();

    /**
     * Cancels visualization process.
     */
    void CancelVisualizationL();

    /** Possible mark states */
    enum TMarkState
    {
        EMarkStateIn,
        EMarkStateOut,
        EMarkStateInOut
    };
    
    /**
     *  
     */
    void HandleStatusPaneSizeChange();

    /** Callback function */
    static TInt AsyncOpenAudioFile( TAny* aThis );

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
    void MoveStartOrEndMarkL( TTimeIntervalMicroSeconds aPosition, CVeiCutAudioContainer::TCutMark aMarkType );
		
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

    /**
     * start processing the input file
     */
    void OpenAudioFileL();

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
    CVeiCutAudioContainer* iContainer;

    /* index of the video clip in the movie	*/
    TUint iIndex;

    /* movie */
    CVedMovie* iMovie;

    /** Time updater. */
    CPeriodic* iTimeUpdater;

    /** Pointer to the navi pane. */
    CAknNavigationControlContainer* iNaviPane;

    /** Time navi. */
    CAknNavigationDecorator* iTimeNavi;

    /** Volume hiding timer. */
    CPeriodic* iVolumeHider;

    /** Volume navi decorator. */
    CAknNavigationDecorator* iVolumeNavi;

    /** Popup menu state flag */
    TBool iPopupMenuOpened;

    /** Audio muted flag */
    TBool iAudioMuted;

    /** play marked flag */
    TBool iPlayMarked;

    /** current mark state */
    TMarkState iMarkState;

    /** Error number */
    TInt iErrorNmb;

    TTimeIntervalMicroSeconds iOriginalCutInTime;
    /** Error UI */
    CVeiErrorUI* iErrorUI;

    /** Callback utility */
    CAsyncCallBack* iCallBack;
};

#endif 

// End of File
