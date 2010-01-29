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


#ifndef VEDEDITVIDEOCONTAINER_H
#define VEDEDITVIDEOCONTAINER_H

#include <coecntrl.h>
#include <eiksbobs.h> 
#include <VedMovie.h>
#include <VedVideoClipInfo.h>
#include <aknprogressdialog.h> 
#include <aknutils.h>
#include <aknlayoutdef.h> 

#include "veivideodisplay.h"
#include "veiframetaker.h"
#include "veicutterbar.h"
#include "VeiDisplayLighter.h"
#include "VeiImageClipGenerator.h"
#include "VeiTitleClipGenerator.h"
#include "VeiImageConverter.h"
#include "VeiRemConTarget.h"
#include "VideoEditorCommon.h"

const TInt KMaxZoomFactorX = 4;
const TInt KMaxZoomFactorY = 2;

const TInt KVolumeSliderMin = -10;
const TInt KVolumeSliderMax = 10;
const TInt KVolumeSliderStep = 1;

class CVedMovie;
class CVeiEditVideoView;
class CAknProgressDialog;
class CEikProgressInfo;
class CAknsBasicBackgroundControlContext;
class CVeiVideoDisplay;
class CVeiTextDisplay;
class CVeiCutterBar;
class CPeriodic;
class CVeiIconBox;
class CAknNavigationDecorator;
class CVeiSlider;
class CStoryboardVideoItem;
class CStoryboardAudioItem;
class CTransitionInfo;

using namespace VideoEditor;


/**
 *  CVeiEditVideoContainer  container control class.
 *  
 */
class CVeiEditVideoContainer: public CCoeControl, 
                              public MCoeControlObserver,
                              public MVedMovieObserver,
                              public MVedVideoClipFrameObserver, 
                              public MVedVideoClipInfoObserver, 
                              public MProgressDialogCallback,
                              public MConverterController,
                              public MVeiVideoDisplayObserver,
                              public MVeiFrameTakerObserver,
                              public MVeiMediakeyObserver


{
public:

    /**
     * Creates a CVeiEditVideoContainer object, which will draw 
     * itself to aRect.
     *
     * @param aRect Frame rectangle for container.
     * @param aMovie  movie being edited
     * @param aView	owner
     *
     * @return a pointer to the created instance of CVeiEditVideoContainer
     */
    static CVeiEditVideoContainer* NewL( const TRect& aRect,
                                         CVedMovie& aMovie, 
                                         CVeiEditVideoView& aView );

    /**  
     * Creates a CVeiEditVideoContainer object, which will draw
     * itself to aRect.
     * Leaves the created object in the cleanup stack.
     *
     * @param aRect Frame rectangle for container.
     * @param aMovie  movie being edited
     * @param aView	owner
     *
     * @return a pointer to the created instance of CVeiEditVideoContainer
     */
    static CVeiEditVideoContainer* NewLC( const TRect& aRect, 
                                          CVedMovie& aMovie, 
                                          CVeiEditVideoView& aView );

    /**
     * Constructor.
     *
     * @param aMovie instance of CVedMovie
     * @param aView instance of Edit Video View
     */
    CVeiEditVideoContainer( CVedMovie& aMovie, CVeiEditVideoView& aView );

    /**
     * Default constructor.
     * @param aRect Frame rectangle for container.
     */
    void ConstructL( const TRect& aRect );

    /**
     * Destructor.
     */
    virtual ~CVeiEditVideoContainer();

    /**
     * Enumeration for selection mode
     */
    enum TSelectionMode
    {
        EModeNavigation = 0,
        EModeMove,
        EModeDuration,
        EModeRecordingSetStart,
        EModeRecording,
        EModeRecordingPaused,
        EModeSlowMotion,
        EModePreview,
        EModeMixingAudio,
        EModeAdjustVolume
    };

    /**
     * Enumeration for preview state
     */
    enum TPreviewState
    {
        EStateInitializing = 0,
        EStateOpening,
        EStateStopped,
        EStatePlaying,
        EStatePaused,
        EStateClosed,
        EStateBuffering,
        EStateGettingFrame,
        EStateTerminating
    };

    TBool CurrentClipIsFile();

    /**
     * Sets cursor position to aCurrentIndex. If cursor is on audio track,
     * audio index is set and if in video track, video cursor is set.
     *
     * @param aCurrentIndex	Cursor position
     */
    void SetCurrentIndex( TInt aCurrentIndex );

    /**
     * Returns the current index.
     *
     * @return  current index
     */
    TInt CurrentIndex()const;

    /** 
     * Updates thumbnail in video array.
     *
     * @aIndex Index in video array
     */
    void UpdateThumbnailL( TInt aIndex );

    /**
     * Sets the selected status.
     * 
     * @param aSelected  status
     */
    void SetSelectionMode( TSelectionMode aSelectionMode );

    /**
     * Control iInfoDisplay's arrows visibility .
     */
    void ArrowsControl()const;

    /**
     * Returns the current index and decrements index.
     *
     * @return  current index
     */
    TUint GetAndDecrementCurrentIndex();

    void SetCursorLocation( TCursorLocation aCursorLocation );

    void PlayVideoFileL( const TDesC& aFilename, const TBool& aFullScreen );

    void PauseVideoL();

    /**
     * Starts playing video		 
     */
    void PlayVideo( const TDesC& aFilename, TBool& aFullScreen );

    void StopVideo( TBool aCloseStream );

    /**
     * Set starting value of slow motion.
     *
     * @param aSlowMotionStartValue slow motion value 
     */
    void SetSlowMotionStartValueL( TInt aSlowMotionStartValue );

    void SetRecordedAudioDuration( const TTimeIntervalMicroSeconds& aDuration );
    void DrawTrackBoxes()const;

public:

    /**
     * Update function that is called by the static callback method.
     */
    void DoUpdate();
    /**
     *
     */
    void SetFinishedStatus( TBool aStatus );
    /**
     *
     */
    void TakeSnapshotL();

    /**
     * Saves snapshot.
     */
    void SaveSnapshotL();
    /**
     * Gets intra frame bitmap from video clip.
     *
     * @param aTime	intra frame time.
     */
    void GetThumbAtL( const TTimeIntervalMicroSeconds& aTime );

    /**
     *
     */
    TTimeIntervalMicroSeconds TotalLength();

    /**
     * Returns the playback position.
     * @return  playback position
     */
    TTimeIntervalMicroSeconds PlaybackPositionL();


    /**
     *	Sets hole screen to black
     *
     *
     */
    void SetBlackScreen( TBool aBlack );


    /**
     * Returns snapshots size by bytes.
     *
     *
     */
    TInt SnapshotSize();

    /**
     * Returns audio mixing ratio between original and imported track.
     * Used in EModeMixAudio
     *
     */
    TInt AudioMixingRatio()const;

    /**
     * Returns clip volume, used in EModeAdjustVolume
     *
     */
    TInt Volume()const;

    /**
     * Prepares the control for termination; stops video playback
     * and set the state to EStateTerminating.
     * 
     */
    void PrepareForTerminationL();

private:

    /**
     * Callback function for the timer.
     *
     * @param aThis  self pointer
     *
     * @return  dummy value 42
     */
    static TInt Update( TAny* aThis );

    void StartZooming();

    void ShowMiddleAnimationL( TVedMiddleTransitionEffect aMiddleEffect );
    
    void ShowEndAnimationL( TVedEndTransitionEffect aEndEffect );
    
    void ShowStartAnimationL( TVedStartTransitionEffect aStartEffect );
    
    void SetColourToningIcons( TInt aIndex );
    
     /**
    * Handles video item timeline touch events
    * @param aPointerEvent pointer event
    */
    void HandleVideoTimelineTouchL( TPointerEvent aPointerEvent );

     /**
    * Handles volume slider touch events
    * @param aPointerEvent pointer event
    */
    void HandleVolumeSliderTouchL( TPointerEvent aPointerEvent );

    /**
     * Calculates the rects for the video items in the timeline
     * and adds them to iVideoItemRectArray
     */
    void CalculateVideoClipRects();   
    
    /**
     * Goes through the table iVideoItemRectArray and 
     * finds the clip that the user has clicked.
     * @param aPressedPointX The x coordinate value that was pressed
     * 			             inside the timeline
     * @return index of the clip that the user has clicked or -1 if 
     *         the click was in the empty part of the timeline
     */    
    TInt FindClickedClip( TInt aPressedPointX );
    
private:

    /**
     * Gets album from aFilename. If file belongs to album, album name is
     * returned in aAlbumName, otherwise KNullDesc.
     */
    void GetAlbumL( const TDesC& aFilename, TDes& aAlbumName )const;

    /**
     * Starts active object which takes thumbnails from videoclip. 
     * When completed -> NotifyFramesCompleted(...)
     */
    void StartFrameTakerL( TInt aIndex );

    /**
     *
     */
    void ConvertBW( CFbsBitmap& aBitmap )const;

    /**
     *
     */
    void ConvertToning( CFbsBitmap& aBitmap )const;


    /**
     * Gets an object whose type is encapsulated by the specified TTypeUid 
     * object.
     *
     * @param aId Encapsulates the Uid that identifies the type of object
     * required.
     *
     * @return
     */
    virtual TTypeUid::Ptr MopSupplyObject( TTypeUid aId );


    virtual void NotifyFramesCompleted( CFbsBitmap* aFirstFrame,
                                        CFbsBitmap* aLastFrame,
                                        CFbsBitmap* aTimelineFrame, 
                                        TInt aError );


    virtual void NotifyVideoDisplayEvent( const TPlayerEvent aEvent, 
                                          const TInt& aInfo = 0 );
    /**
    * From MVedVideoClipInfoObserver
    */
    virtual void NotifyVideoClipInfoReady( CVedVideoClipInfo& aInfo,
                                           TInt aError );
    void NotifyCompletion( TInt aErr );

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
    
    virtual void NotifyAudioClipAdded( CVedMovie& aMovie, 
                                       TInt aIndex );
                                       
    virtual void NotifyAudioClipAddingFailed( CVedMovie& aMovie, 
                                              TInt aError );
                                              
    virtual void NotifyAudioClipRemoved( CVedMovie& aMovie, 
                                         TInt aIndex );
                                         
    virtual void NotifyAudioClipIndicesChanged( CVedMovie& aMovie, 
                                                TInt aOldIndex, 
                                                TInt aNewIndex );
                                                
    virtual void NotifyAudioClipTimingsChanged( CVedMovie& aMovie,
                                                TInt aIndex );
                                                
    virtual void NotifyMovieReseted( CVedMovie& aMovie );

    virtual void NotifyVideoClipFrameCompleted( CVedVideoClipInfo& aInfo, 
                                                TInt aError,
                                                CFbsBitmap* aFrame );

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
                                                         TInt aMarkIndex );

    virtual void DialogDismissedL( TInt aButtonId );

    /**
     * Moves selected audio clip right. Only for OfferKeyEventL use.
     *
     * @return EKeyWasConsumed if key event was processed and 
     * EKeyWasNotConsumed if it was not.
     */
    TKeyResponse MoveAudioRight();

    /**
     * Moves selected audio clip left. Only for OfferKeyEventL use.
     *
     * @return EKeyWasConsumed if key event was processed and 
     * EKeyWasNotConsumed if it was not.
     */
    TKeyResponse MoveAudioLeft();


    /**
     * Time increment.
     *
     * @return Time in milliseconds
     */
    TInt TimeIncrement( TInt aKeyCount )const;

    /*
     * From CoeControl,SizeChanged.
     */
    void SizeChanged();

    TKeyResponse OfferKeyEventL( const TKeyEvent& aKeyEvent, TEventCode aType );
    /**
     * From CoeControl,CountComponentControls.
     */
    TInt CountComponentControls()const;

    /**
     * From CCoeControl,ComponentControl.
     */
    CCoeControl* ComponentControl( TInt aIndex )const;

    /**
     * From CCoeControl,Draw.
     */
    void Draw( const TRect& aRect )const;

    /**
     * From CCoeControl. Handles a change to the control's resources.  
     * @param aType A message UID value.
     */
    void HandleResourceChange( TInt aType );

    /**
     * From CCoeControl, gets the control's help context. Associates the
     * control with a particular Help file and topic in a context sensitive
     * application.
     *
     * @param aContext Control's help context.
     */
    void GetHelpContext( TCoeHelpContext& aContext )const;

    /**
     * Draws play head when previewing video
     */
    void DrawPlayHead() /*const*/;

    /**
     * From MCoeControlObserver, called when there is a control event
     * to handle.
     *
     * @param aControl  control originating the event
     * @param aEventType  event type
     */
    void HandleControlEventL( CCoeControl* aControl, TCoeEvent aEventType );

    /**
    * From CCoeControl
    *
    * @param aPointerEvent  pointer event
    */
    void HandlePointerEventL( const TPointerEvent& aPointerEvent );

    /**
     * Callback function for the  playhead timer.
     * @param aThis  self pointer
     * @return  dummy value 42
     */
    static TInt UpdatePosition( TAny* aThis );

    /**
     * Update function that is called by the static 
     * callback method - UpdatePosition.
     */
    void DoUpdatePosition();

    /**
     * Returns the boolean is audioclip cutted.
     *
     * @return  TBool
     */
    TBool IsAudioClipCutted();

    TKeyResponse HandleScrollBarL( const TKeyEvent& aKeyEvent,
                                   TEventCode aType );

    void CreateScrollBarL( const TRect& aRect );

    /**
     * Cancels saving of the snapshot.
     */
    void CancelSnapshotSave();

    /**
     * Starts progress dialog
     *
     * @param aDialogResId resource id of the dialog
     * @param aTextResId text of dialog
     */
    void StartProgressDialogL( const TInt aDialogResId, const TInt aTextResId );

    void SetPreviewState( const TPreviewState aNewState );

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

private:
    CVeiDisplayLighter* iScreenLight;
    /** View eg.owner*/
    CVeiEditVideoView& iView;

    /** Movie being edited. */
    CVedMovie& iMovie;

    /** Currently processed thumbnail index. */
    TInt iCurrentlyProcessedIndex;

    /** Video cursor position. */
    TInt iVideoCursorPos;

    /** Audio cursor position. */
    TInt iAudioCursorPos;

    /** No thumbnail icon. */
    CFbsBitmap* iNoThumbnailIcon;
    CFbsBitmap* iNoThumbnailIconMask;

    /** Audio icon. */
    CFbsBitmap* iAudioIcon;

    /** Audio mixing icon. */
    CFbsBitmap* iAudioMixingIcon;

    CFbsBitmap* iAudioTrackIcon;
    CFbsBitmap* iAudioTrackIconMask;
    /** Video track icon. */
    CFbsBitmap* iVideoTrackIcon;
    CFbsBitmap* iVideoTrackIconMask;

    /** Array of video items. */
    RPointerArray < CStoryboardVideoItem > iVideoItemArray;

    /** Array of audio items. */
    RPointerArray < CStoryboardAudioItem > iAudioItemArray;

    /** Array of video item rects. */
    RArray < TRect > iVideoItemRectArray;

    /** The index of the clip that the user has clicked. */
    TInt iClickedClip;
    
    /** The part of the timeline that doesn't include video clips.
        If there are no video clips, the rect is the same as the
        whole timeline (=iVideoBarBox) */    
    TRect iEmptyVideoTimeLineRect;

    /** Cursor location. */
    TCursorLocation iCursorLocation;

    /** Selection mode. */
    TSelectionMode iSelectionMode;

    /** Transition info. */
    CTransitionInfo* iTransitionInfo;

    /** Previous cursor location. */
    TCursorLocation iPrevCursorLocation;

    /** Key repeat count. Incremented in OfferKeyEventL() */
    TInt iKeyRepeatCount;

    TSize iTransitionMarkerSize;
    TRect iVideoBarBox;
    TRect iAudioBarBox;
    TPoint iAudioBarIconPos;
    TRect iAudioTrackBox;
    TPoint iVideoBarIconPos;
    TRect iVideoTrackBox;
    TRect iBarArea;
    TRect iTextArea;
    TInt iTextBaseline;
    TRect iIconArea;
    TRect iIconTopEmptyArea;
    TRect iIconBottomEmptyArea;
    TInt iIconTextBaseline;
    TPoint iHorizontalSliderPoint;
    TSize iHorizontalSliderSize;
    TPoint iVerticalSliderPoint;
    TSize iVerticalSliderSize;

    CAknProgressDialog* iProgressDialog;
    //CEikProgressInfo*	iProgressInfo;

    /** Background context. Skin stuff. */
    CAknsBasicBackgroundControlContext* iBgContext;

    TTimeIntervalMicroSeconds iRecordedAudioStartTime;
    TTimeIntervalMicroSeconds iRecordedAudioDuration;

    TBool iRecordedAudio;

    TInt iZoomFactorX;
    TInt iZoomFactorY;
    CPeriodic* iZoomTimer;

    CVeiVideoDisplay* iVideoDisplay;

    CVeiVideoDisplay* iTransitionDisplayLeft; // left hand side
    CVeiVideoDisplay* iTransitionDisplayRight; // right hand side
    TRect iTransitionDisplayLeftBox;
    TRect iTransitionDisplayRightBox;

    CVeiTextDisplay* iInfoDisplay;
    CVeiTextDisplay* iArrowsDisplay;

    TRect iVideoDisplayBox;
    TRect iVideoDisplayBoxOnTransition;
    TRect iInfoDisplayBox;
    TRect iSlowMotionBox;
    TRect iTransitionArrowsBox;

    CVeiCutterBar* iDummyCutBar;
    CVeiCutterBar* iDummyCutBarLeft;

    /* Iconbox */
    CVeiIconBox* iEffectSymbols;
    TRect iEffectSymbolBox;

    TRect iDummyCutBarBox;
    TRect iDummyCutBarBoxOnTransition;

    CFbsBitmap* iGradientBitmap;

    TBool iFinished;
    TBool iTakeSnapshot;
    HBufC* iSaveToFileName;

    /** Seek thumbnail position in video clip. */
    TTimeIntervalMicroSeconds iSeekPos;

    /** Seek - flag. */
    TBool iSeeking;

    /** Last keycode, used in OfferKeyEventL(); */
    TUint iLastKeyCode;

    /** Last position. */
    TTimeIntervalMicroSeconds iLastPosition;


    /** Periodic timer used for playhead update . */
    CPeriodic* iPeriodic;
    /** Current point. This is where the horizontal bar is drawn. */
    TUint iCurrentPoint;
    CVeiImageConverter* iConverter;

    TInt iSlowMotionValue;

    //** Preview flag. */
    TBool iFullScreenSelected;

    CVeiFrameTaker* iFrameTaker;

    //** GetFrame-flag for seeking functionality. */
    TBool iSeekingFrame;

    HBufC* iTempFileName;

    CVedVideoClipInfo* iTempVideoInfo;

    /** Current preview state. */
    TPreviewState iPreviewState;
    /** Previous preview state. */
    TPreviewState iPreviousPreviewState;

    TInt iCurrentPointX;
    TBool iCloseStream;
    TBool iFrameReady;
    TBool iBackKeyPressed;
    /** Previous cursor location for preview. */
    TCursorLocation iCursorPreviousLocation;

    /** Flag indicating to draw the screen black. */
    TBool iBlackScreen;

    /** Horizontal slider component. */
    CVeiSlider* iHorizontalSlider;
    /** Vertical slider component. */
    CVeiSlider* iVerticalSlider;

    /** Flag telling whether video file must be reopened in preview. */
    TBool iMustBeReopened;

    /** Pause indicator. */
    TRect iPauseIconBox;
    CFbsBitmap* iPauseBitmap;
    CFbsBitmap* iPauseBitmapMask;

    /** Remote connection API used to handle the volume keys. */
    CVeiRemConTarget* iRemConTarget;

    TBool iTakeSnapshotWaiting;

    /** Callback utility */
    CAsyncCallBack* iCallBack;

    /** ETrue if user is dragging a video clip with touch,
    	EFalse otherwise */
    TBool iIsVideoDrag;

    /** ETrue if the pen goes down inside a timeline video clip
    	EFalse otherwise */		
    TBool iIsVideoTapped;
    
    /** The position where the user is dragging the clip into */    
    TInt iNewClipPosition;

#include "veieditvideocontainer.inl"
};

#endif 
// End of File
