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


#ifndef VEICUTAUDIOCONTAINER_H
#define VEICUTAUDIOCONTAINER_H

#include <coecntrl.h>
#include <aknprogressdialog.h> 
#include <VideoPlayer.h>
#include <VedCommon.h>
#include <VedMovie.h>
#include <MdaAudioSamplePlayer.h>
#include <VedAudioClipInfo.h>

#include "VeiRemConTarget.h"

class CVeiCutAudioView;
class CVeiCutterBar;
class CAknsBasicBackgroundControlContext;
class CVeiErrorUI;
class CVeiTextDisplay;
class CSampleArrayHandler;


const TInt KMinCutAudioVolumeLevel = 1;
const TInt KMaxCutAudioVolumeLevel = 10;

/**
 * CVeiCutAudioContainer container control class.
 *  
 * Container for CVeiCutAudioView.
 */
class CVeiCutAudioContainer: public CCoeControl, 
                             public MCoeControlObserver,
                             public MMdaAudioPlayerCallback,
                             public MVedAudioClipVisualizationObserver, 
                             public MProgressDialogCallback,
                             public MVeiMediakeyObserver
{
public:
    /**
     * Edit state.
     */
    enum TCutAudioState
    {
        EStateInitializing = 1,
        EStateOpening,
        EStateStoppedInitial,
        EStateStopped,
        EStatePlaying,
        EStatePlayingMenuOpen,
        EStatePaused,
        EStateTerminating
    };

    enum TMarkState
    {
        ENoMark = 0,
        EMarked			
    };
    
    
			
    /**
     * Start or end mark.
     */
    enum TCutMark
    {
        ENoMarks,
        EStartMark,
        EEndMark
    };
			

        
public:
    /**
     * Creates a CStoryboardContainer object, which will draw itself to aRect.
     *
     * @param aRect Frame rectangle for container.
     * @param aMovie  movie being edited
     *
     * @return a pointer to the created instance of CStoryboardContainer
     */
    static CVeiCutAudioContainer* NewL( const TRect& aRect, 
                                        CVeiCutAudioView& aView, 
                                        CVeiErrorUI& aErrorUI );

    /**  
     * Creates a CStoryboardContainer object, which will draw itself to aRect.
     * Leaves the created object in the cleanup stack.
     *
     * @param aRect Frame rectangle for container.
     * @param aMovie  movie being edited
     *
     * @return a pointer to the created instance of CStoryboardContainer
     */
    static CVeiCutAudioContainer* NewLC( const TRect& aRect, 
                                         CVeiCutAudioView& aView, 
                                         CVeiErrorUI& aErrorUI );

    /**
     * Default constructor.
     *
     * @param aRect  Frame rectangle for container.
     * @param aView  pointer to the view.
     */
    void ConstructL( const TRect& aRect, 
                     CVeiCutAudioView& aView, 
                     CVeiErrorUI& aErrorUI );

    /**
     * Destructor.
     */
    virtual ~CVeiCutAudioContainer();

public:
    void OpenAudioFileL( const TDesC& aFileName );

    /**
     * ###Missin' description###
     *
     * @param aState
     */
    void SetStateL( CVeiCutAudioContainer::TCutAudioState aState );
    /**
     * Starts playing.
     *
     * @param aStartTime
     */
    void PlayL( const TTimeIntervalMicroSeconds& aStartTime = TTimeIntervalMicroSeconds( 0 ) );

    /**
     * Stops playing.
     */
    void StopL();

    /**
     * Pauses playing.
     */
    void PauseL();

    /**
     * Closes the stream.
     */
    void CloseStreamL();

    /**
     * Returns the playback position.
     *
     * @return  playback position
     */
    const TTimeIntervalMicroSeconds& PlaybackPositionL();

    /**
     * Marks the in point.
     */
    void MarkedInL();

    /**
     * Marks the out point.
     */
    void MarkedOutL();

    /**
     * Change volume level. Changes current volume level by given amount.
     *
     * @param aVolumeChange		volume change
     */
    void SetVolumeL( TInt aVolumeChange );

    /**
     * Sets cut in time to cut video bar.
     *
     * @param aTime	Cut in time
     */
    void SetInTimeL( const TTimeIntervalMicroSeconds& aTime );

    /**
     * Sets cut out time to cut video bar.
     *
     * @param aTime	Cut out time
     */
    void SetOutTimeL( const TTimeIntervalMicroSeconds& aTime );

    /**
     * Sets duration to cut video bar.
     *
     * @param aTime	duration
     */
    void SetDuration( const TTimeIntervalMicroSeconds& aDuration );

public:
    /**
     * Update function that is called by the static callback method.
     */
    void DoUpdate();

    /**
     * From CCoeControl, OfferKeyEventL.
     *
     * @param aKeyEvent  key event
     * @param aType  event code
     */
    TKeyResponse OfferKeyEventL( const TKeyEvent& aKeyEvent, TEventCode aType );

    /**
     * Returns total length of the video clip.
     *
     * @return  total length
     */
    const TTimeIntervalMicroSeconds& TotalLength();

    /**
     * Returns the player state.
     *
     * @return  player state
     */
    inline TCutAudioState State() { return iState; };
    /**
     * Returns current volume level.
     *
     * @return	volume level
     */
    inline TInt Volume() const { return iInternalVolume; };

    /**
     * Returns minimum volume level.
     *
     * @return	min volume level
     */
    inline TInt MinVolume() const { return 1; };

    /**
     * Returns maximum volume level.
     *
     * @return	max volume level
     */
    inline TInt MaxVolume()const { return iMaxVolume; };

    /**
     * Returns the visualization resolution.
     *
     * @return	The visualization resolution, i.e. the size of the sample array
     */
    TInt VisualizationResolution()const;

    /**
     * Prepares the control for termination; stops audio playback
     * and sets the state to EStateTerminating.
     * 
     */
    void PrepareForTerminationL();

public:
    virtual void NotifyAudioClipVisualizationStarted( const CVedAudioClipInfo& aInfo );

    virtual void NotifyAudioClipVisualizationProgressed( const CVedAudioClipInfo& aInfo, 
                                                         TInt aPercentage );
    virtual void NotifyAudioClipVisualizationCompleted( const CVedAudioClipInfo& aInfo, 
                                                        TInt aError, 
                                                        TInt8* aVisualization,
                                                        TInt aResolution);

private:
    /**
     * Callback function for the timer.
     *
     * @param aThis  self pointer
     *
     * @return  dummy value
     */
    static TInt DoAudioBarUpdate( TAny* aThis );

    /** 
     * From MMdaAudioPlayerCallback
     */
    virtual void MapcInitComplete( TInt aError,
                                   const TTimeIntervalMicroSeconds& aDuration );

    virtual void MapcPlayComplete( TInt aError );

    /**
     * Time increment.
     *
     * @param aKeyCount number a key events
     * @return time 
     */
    TInt TimeIncrement( TInt aKeyCount )const;

    /**
     * Constructor.
     *
     * @param aView	instance of cut audio view
     * @param aErrorUI instance of CVeiErrorUI
     */
    CVeiCutAudioContainer( const TRect& aRect, 
                           CVeiCutAudioView& aView, 
                           CVeiErrorUI& aErrorUI );


    /**
     * Continue playing
     */
    void ResumeL();

    /**
     * Gets intra frame bitmap from video clip.
     *
     * @param aTime	intra frame time.
     */
    void GetThumbAtL( TTimeIntervalMicroSeconds aTime );

    /**
     * From CoeControl, MopSupplyObject.
     *
     * @param aId  
     */
    virtual TTypeUid::Ptr MopSupplyObject( TTypeUid aId );

    /**
     * From CoeControl, SizeChanged.
     */
    void SizeChanged();

    /**
     * From CoeControl, CountComponentControls.
     * 
     * @return  number of component controls in this control
     */
    TInt CountComponentControls()const;

    /**
     * From CCoeControl, ComponentControl.
     *
     * @param aIndex  index of the control to return
     */
    CCoeControl* ComponentControl( TInt aIndex )const;

    /**
     * From CCoeControl,Draw.
     *
     * @param aRect  region of the control to be redrawn
     */
    void Draw( const TRect& aRect )const;

    /**
     * From CCoeControl, gets the control's help context. Associates the
     * control with a particular Help file and topic in a context sensitive
     * application.
     *
     * @param aContext Control's help context.
     */
    void GetHelpContext( TCoeHelpContext& aContext )const;

    /**
     * From CCoeControl, HandleControlEventL
     */
    void HandleControlEventL( CCoeControl* aControl, TCoeEvent aEventType );

    /**
    * HandlePointerEventL
    * Handles pen inputs
    *
    * @param aPointerEvent  pointer event
    */
    void HandlePointerEventL( const TPointerEvent& aPointerEvent );

    /**
    * Handles progress bar touch events
    * @param aPBRect Current progress bar rectangle 
    * @param aPressedPoint The x coordinate value that was pressed
    * 			inside the progress bar
    * @param aDragMarks ETrue if the user drags start or end marks.
    *					EFalse otherwise
    */
    void HandleProgressBarTouchL( TRect aPBRect, TInt aPressedPoint, TBool aDragMarks, CVeiCutAudioContainer::TCutMark aCutMark = ENoMarks );



    /**
     * Update function that is called when visualization must be changed.
     */
    void UpdateVisualizationL();

    /**
     * Get the visualization data from the engine
     */
    void GetVisualizationL();

    /**
     * From MProgressDialogCallback
     */
    void DialogDismissedL( TInt aButtonId );

    /** Callback function */
    static TInt AsyncBack( TAny* aThis );

    /**
     * Propagate command to view's HandleCommandL
     */
    void HandleCommandL( TInt aCommand );

    /**
     * Draw the visualization, including background and indicators etc.
     * to a bitmap, which can be blitted to the screen.
     */
    void DrawToBufBitmapL();

    /**
     * Start a progress note dialog
     */
    void LaunchProgressNoteL();

    /**	HandleVolumeUpL 
     *
     *   @see MVeiMediakeyObserver
     */
    virtual void HandleVolumeUpL();

    /**	HandleVolumeDownL 
     *
     *   @see MVeiMediakeyObserver
     */
    virtual void HandleVolumeDownL();

private:
    //data
    /** cut video view */
    CVeiCutAudioView& iView;

    /** Error UI */
    CVeiErrorUI& iErrorUI;

    /** Current state. */
    TCutAudioState iState;

    /** Previous state. */
    TCutAudioState iPrevState;

    /** cut audio bar. */
    CVeiCutterBar* iCutAudioBar;

    /** Last position. */
    TTimeIntervalMicroSeconds iLastPosition;

    /** Video clip duration */
    TTimeIntervalMicroSeconds iDuration;

    /**
     * Control context that provides a layout background with a 
     * background bitmap and its layout rectangle.
     */
    CAknsBasicBackgroundControlContext* iBgContext;

    /** Videoplayerutility volume */
    TInt iInternalVolume;

    /** Max volume */
    TInt iMaxVolume;

    /** Key repeat count in seek function. */
    TInt iKeyRepeatCount;

    /** Seek thumbnail position in video clip. */
    TTimeIntervalMicroSeconds iSeekPos;

    /** Seek - flag. */
    TBool iSeeking;

    /** Frame ready - flag */
    TBool iFrameReady;

    /** Last keycode, used in OfferKeyEventL(); */
    TUint iLastKeyCode;

    CFbsBitmap* iPauseBitmap;
    CFbsBitmap* iPauseBitmapMask;

    ///	Double buffer bitmap
    CFbsBitmap* iBufBitmap;

    CPeriodic* iVideoBarTimer;
    CMdaAudioPlayerUtility* iAudioSamplePlayer;
    TRect iCutTimeDisplayRect;
    CVeiTextDisplay* iCutTimeDisplay;
    TRect iIconDisplayRect;

    CSampleArrayHandler* iSampleArrayHandler;

    TInt iMarkOutCounter;
    TInt iMarkInCounter;

    TInt iPreviousScreenMode;
    TInt iCurrentScreenMode;

    TMarkState iMarkInState;
    TMarkState iMarkOutState;

    TTimeIntervalMicroSeconds iMarkedInTime;
    TTimeIntervalMicroSeconds iMarkedOutTime;

    /** Progress note. */
    CAknProgressDialog* iProgressNote;

    /** Callback utility */
    CAsyncCallBack* iCallBack;

    /** Remote connection API used to handle the volume keys. */
    CVeiRemConTarget* iRemConTarget;
    
    /** ETrue if user is dragging the start or end mark with a pen,
    	EFalse otherwise */
    TBool iIsMarkDrag;

    /** ETrue if the pen is in start or end mark area when it goes down,
    	EFalse otherwise */		
    TBool iIsMarkTapped;

    /** Indicates which mark the user has tapped */						
    TCutMark iTappedMark;    
};
#endif 

// End of File
