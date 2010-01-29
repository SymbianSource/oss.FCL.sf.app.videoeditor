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


#ifndef VEICUTVIDEOCONTAINER_H
#define VEICUTVIDEOCONTAINER_H

#include <coecntrl.h>
#include <VideoPlayer.h>
#include <VedCommon.h>
#include <VedMovie.h>
#include <aknprogressdialog.h> 

#include "VeiVideoDisplay.h"
#include "VeiImageConverter.h"
#include "VeiRemConTarget.h"

class CVeiCutVideoView;
class CVeiCutterBar;
class CAknsBasicBackgroundControlContext;
class CVeiErrorUI;
class CVeiVideoDisplay;
class CVeiTextDisplay;

const TInt KMinCutVideoVolumeLevel = 1;
const TInt KVeiCutBarHeight = 20;
const TInt KProgressbarFinalValue = 50;
_LIT( KEncoderType, "JPEG" ); // encoder type for image conversion	

/**
 * CVeiCutVideoContainer container control class.
 *  
 * Container for CVeiCutVideoView.
 */
class CVeiCutVideoContainer: public CCoeControl,
                             public MCoeControlObserver,
                             public MVedVideoClipFrameObserver,
                             public MVedVideoClipInfoObserver, 
                             public MConverterController, 
                             public MProgressDialogCallback,
                             public MVeiVideoDisplayObserver,
                             public MVeiMediakeyObserver
{
public:
    /**
     * Edit state.
     */
    enum TCutVideoState
    {
        EStateInitializing = 1,
        EStateOpening,
        EStateStoppedInitial,
        EStateStopped,
        EStatePlaying,
        EStatePlayingMenuOpen,
        EStatePaused,
        EStateGettingFrame,
        EStateBuffering,
        EStateTerminating
    };
    
			
    /**
     * Start or end mark.
     */
    enum TCutMark
    {
        ENoMark,
        EStartMark,
        EEndMark
    };
			

    
public:

    /**
     * Creates a CVeiCutVideoContainer object, which will draw itself to aRect.
     *
     * @param aRect Frame rectangle for container.
     * @param aView 
     *
     * @return a pointer to the created instance of CStoryboardContainer
     */
    static CVeiCutVideoContainer* NewL( const TRect& aRect, 
                                        CVeiCutVideoView& aView, 
                                        CVeiErrorUI& aErrorUI );

    /**  
     * Creates a CVeiCutVideoContainer object, which will draw itself to aRect.
     * Leaves the created object in the cleanup stack.
     *
     * @param aRect Frame rectangle for container.
     * @param aMovie  movie being edited
     *
     * @return a pointer to the created instance of CStoryboardContainer
     */
    static CVeiCutVideoContainer* NewLC( const TRect& aRect, 
                                         CVeiCutVideoView& aView, 
                                         CVeiErrorUI& aErrorUI );

    /**
     * Default constructor.
     *
     * @param aRect  Frame rectangle for container.
     * @param aView  pointer to the view.
     */
    void ConstructL( const TRect& aRect, 
                     CVeiCutVideoView& aView, 
                     CVeiErrorUI& aErrorUI );

    /**
     * Destructor.
     */
    virtual ~CVeiCutVideoContainer();

public:
    /**
     * Takes one thumbnail bitmap from given file.
     *
     * @param aFilename	name of video clip file
     */
    void GetThumbL( const TDesC& aFilename );

    /**
     * ###Missin' description###
     *
     * @param aState
     */
    void SetStateL( CVeiCutVideoContainer::TCutVideoState aState, 
                    TBool aUpdateCBA = ETrue);
    /**
     * Starts playing.
     *
     * @param aStartTime
     */
    void PlayL( const TDesC& aFilename );
    void PlayMarkedL( const TDesC& aFilename, 
                      const TTimeIntervalMicroSeconds& aStartTime, 
                      const TTimeIntervalMicroSeconds& aEndTime );
    /**
     * Stops playing.
     */
    void StopL();

    /**
     * Pauses playing.
     */
    void PauseL( TBool aUpdateCBA = ETrue );

    /**
     * Closes the stream.
     */
    void CloseStreamL();

    /**
     * Returns the playback position.
     *
     * @return  playback position
     */
    TTimeIntervalMicroSeconds PlaybackPositionL();

    /**
     * Marks the in point.
     */
    void MarkedInL();

    /**
     * Marks the out point.
     */
    void MarkedOutL();

    /**
     * Sets cut in time to cut video bar.
     *
     * @param aTime	Cut ín time
     */
    void SetInTime( const TTimeIntervalMicroSeconds& aTime );

    /**
     * Sets cut out time to cut video bar.
     *
     * @param aTime	Cut out time
     */
    void SetOutTime( const TTimeIntervalMicroSeconds& aTime );

    /**
     * Takes the snapshot from current frame
     */
    void TakeSnapshotL();

    void MuteL();
public:

    /**
     * Update function that is called by the static callback method.
     */
    void DoUpdate();

    // from MVeiVideoDisplayObserver
    virtual void NotifyVideoDisplayEvent( const TPlayerEvent aEvent, 
                                          const TInt& aInfo = 0 );

    virtual void NotifyVideoClipFrameCompleted( CVedVideoClipInfo& aInfo, 
                                                TInt aError, 
                                                CFbsBitmap* aFrame);
    /**
     * Called to notify that video clip info is ready
     * for reading.
     *
     * Possible error codes:
     *	- <code>KErrNotFound</code> if there is no file with the specified name
     *    in the specified directory (but the directory exists)
     *	- <code>KErrPathNotFound</code> if the specified directory
     *    does not exist
     *	- <code>KErrUnknown</code> if the specified file is of unknown format
     *
     * @param aInfo   video clip info
     * @param aError  <code>KErrNone</code> if info is ready
     *                for reading; one of the system wide
     *                error codes if reading file failed
     */
    virtual void NotifyVideoClipInfoReady( CVedVideoClipInfo& aInfo, 
                                           TInt aError );

    /**
     * From CCoeControl, OfferKeyEventL.
     *
     * @param aKeyEvent  key event
     * @param aType  event code
     */
    TKeyResponse OfferKeyEventL( const TKeyEvent& aKeyEvent, TEventCode aType );

    /**
     * From MProgressDialogCallback, DialogDismissedL.
     *
     * @param aButtonId  button id
     */
    virtual void DialogDismissedL( TInt aButtonId );

    /**
     * Opens a video clip file and initializes videoplayerutility.
     *
     * @param aFilename  file to open
     */
    void OpenFileL( const TDesC& aFilename /*, TBool aStartPlaying = EFalse */ );

    /**
     * Prepares the control for termination; stops video playback
     * and sets the state to EStateTerminating.
     * 
     */
    void PrepareForTerminationL();

    // From MConverterController
    void NotifyCompletion( TInt aErr );

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
     * Time increment.
     *
     * @param aKeyCount number a key events
     * @return time 
     */
    TInt TimeIncrement( TInt aKeyCount )const;

    /**
     * Constructor.
     *
     * @param aView	instance of cut video view
     * @param aErrorUI instance of CVeiErrorUI
     */
    CVeiCutVideoContainer( const TRect& aRect, 
                           CVeiCutVideoView& aView, 
                           CVeiErrorUI& aErrorUI );

    /**
     * Gets intra frame bitmap from video clip.
     *
     * @param aTime	intra frame time.
     */
    void GetThumbAtL( const TTimeIntervalMicroSeconds& aTime );

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
    void HandleProgressBarTouchL( TRect aPBRect, TInt aPressedPoint, TBool aDragMarks, CVeiCutVideoContainer::TCutMark aCutMark = ENoMark );

    /*
     * Indicates ProgressNote. 
     *
     */
    void ShowProgressNoteL();

    /**
     * Shows information note with given message.
     * 
     * @param aMessage message to show.
     */
    void ShowInformationNoteL( const TDesC& aMessage )const;

    /**
     * The entity of ProgressCallBackL() function
     * @return 0 when work is done, otherwise return 1.
     */
    TInt UpdateProgressNoteL();

    /**
     * Saves snapshot.
     */
    void SaveSnapshotL();

    /**
     * Cancels saving of the snapshot.
     */
    void CancelSnapshotSave();

    void StopProgressDialog();

    void ShowGlobalErrorNote( const TInt aErr );

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

    /** Callback function */
    static TInt AsyncTakeSnapshot( TAny* aThis );
    /** Callback function */
    static TInt AsyncSaveSnapshot( TAny* aThis );

private:
    //data
    /** cut video view */
    CVeiCutVideoView& iView;

    /** Error UI */
    CVeiErrorUI& iErrorUI;

    /** Video clip info*/
    CVedVideoClipInfo* iVideoClipInfo;

    /** Current state. */
    TCutVideoState iState;

    /** Previous state. */
    TCutVideoState iPreviousState;

    /** Previous state. */
    TCutVideoState iPrevState;

    /** cut video bar. */
    CVeiCutterBar* iCutVideoBar;

    /** Last position. */
    TTimeIntervalMicroSeconds iLastPosition;

    /** Video clip duration */
    TTimeIntervalMicroSeconds iDuration;

    /**
     * Control context that provides a layout background with a 
     * background bitmap and its layout rectangle.
     */
    CAknsBasicBackgroundControlContext* iBgContext;

    TInt iInternalVolume;

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

    /** The actuall calls to ICL are done from this image converter. */
    CVeiImageConverter* iConverter;

    //** Whether we need to take snapshot. */
    TBool iTakeSnapshot;

    /** Progress dialog */
    CAknProgressDialog* iProgressDialog;

    /** Progress info for the progress dialog. */
    CEikProgressInfo* iProgressInfo;

    HBufC* iSaveToFileName;
    TSize iFrameSize;
    CPeriodic* iVideoBarTimer;
    CVeiVideoDisplay* iVideoDisplay;
    TRect iDisplayRect;
    TRect iCutTimeDisplayRect;
    CVeiTextDisplay* iCutTimeDisplay;
    TBool iPlayOrPlayMarked;

    TRect iIconDisplayRect;

    CFbsBitmap* iPauseBitmap;
    CFbsBitmap* iPauseBitmapMask;

    /** Callback utilities */
    CAsyncCallBack* iCallBackSaveSnapshot;
    CAsyncCallBack* iCallBackTakeSnapshot;

    /** Remote connection API used to handle the volume keys. */
    CVeiRemConTarget* iRemConTarget;

    TBool iTakeSnapshotWaiting;

    /** ETrue if user is dragging the start or end mark with a pen,
    	EFalse otherwise */
    TBool iIsMarkDrag;

    /** ETrue if the pen is in start or end mark area when it goes down,
    	EFalse otherwise */		
    TBool iIsMarkTapped;

    /** Indicates which mark the user has tapped */						
    TCutMark iTappedMark;


#include "veicutvideocontainer.inl"

};
#endif 

// End of File
