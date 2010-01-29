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


#ifndef VEITRIMFORMMSCONTAINER_H
#define VEITRIMFORMMSCONTAINER_H

// INCLUDES
// System includes
#include <AknUtils.h>   // TAknLayoutText
#include <coecntrl.h>
#include <AknProgressDialog.h> 
#include <VedMovie.h>
#include <VedVideoClipInfo.h>
// User includes
#include "VeiVideoDisplay.h"
#include "VeiFrameTaker.h"
#include "VeiTrimForMmsView.h"
#include "VeiDisplayLighter.h"

// FORWARD DECLARATIONS
class CVeiCutterBar;
class CVeiTrimForMmsView;
class CVeiTrimForMmsView;
class CAknsBasicBackgroundControlContext;
class CAknProgressDialog;
//class CEikProgressInfo;

// CLASS DECLARATION

/**
 *
 */
class CVeiTrimForMmsContainer: public CCoeControl,
                               public MVedMovieObserver,
                               public MVeiVideoDisplayObserver,
                               public MVeiFrameTakerObserver,
                               public MProgressDialogCallback
{
public:

    static CVeiTrimForMmsContainer* NewL( const TRect& aRect,
                                          CVedMovie& aMovie,
                                          CVeiTrimForMmsView& aView );

    static CVeiTrimForMmsContainer* NewLC( const TRect& aRect,
                                           CVedMovie& aMovie,
                                           CVeiTrimForMmsView& aView );

    ~CVeiTrimForMmsContainer();

public:
    // New functions

    const TTimeIntervalMicroSeconds& CutInTime()const;

    const TTimeIntervalMicroSeconds& CutOutTime()const;

    void SetMaxMmsSize( TInt aMaxSizeInBytes );

    /**
     * Start full screen preview. Calls CVeiVideoDisplay. 
     *
     * @param aFilename filename.
     * @param aRect full screen size.
     */
    void PlayL( const TDesC& aFilename, const TRect& aRect );

    /**
     *	Stop preview. Stops CVeiVideoDisplay
     *
     * @param aCloseStream 
     */
    void Stop( TBool aCloseStream );

    /**
     *
     */
    void PauseL();

    /**
     * Return the video display state of CVeiTrimForMmsContainer. 
     *
     * @param 
     */
    TInt PreviewState()const;

    void StartFrameTakerL( TInt aIndex );

private:
    // From CCoeControl

    virtual void DialogDismissedL( TInt aButtonId );

    CCoeControl* ComponentControl( TInt aIndex )const;

    TInt CountComponentControls()const;

    /**
     * Gets the control's help context. Associates the control with a 
     * particular Help file and topic in a context sensitive application.
     *
     * @param aContext The control's help context.
     */
    void GetHelpContext( TCoeHelpContext& aContext )const;

    void Draw( const TRect& aRect )const;

    void SizeChanged();

    TKeyResponse OfferKeyEventL( const TKeyEvent& aKeyEvent, 
                                 TEventCode aType );

private:
    // From MVeiVideoDisplayObserver
    
    virtual void NotifyVideoDisplayEvent( const TPlayerEvent aEvent, 
                                          const TInt& aInfo = 0 );

    virtual void NotifyFramesCompleted( CFbsBitmap* aFirstFrame, 
                                        CFbsBitmap* aLastFrame,
                                        CFbsBitmap* /*aTimelineFrame*/, 
                                        TInt aError );


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

private:
    // New functions
    TInt TimeIncrement( TInt aKeyCount )const;

private:
    // Constructors

    CVeiTrimForMmsContainer( CVedMovie& aMovie, CVeiTrimForMmsView& aView );

    void ConstructL( const TRect& aRect );

private:

    /**
     * Controls
     */
    enum TTrimForMmsControls
    {
        ECutFrame,
        EVideoDisplayStart,
        EVideoDisplayEnd,
        EVideoPreview,
        ENumberOfControls   // This is always the last one!
    };

public:

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

private:
    // Data

    /**
     * Whether or not the last key pressed was left or right navi-key.
     */
    TBool iLastKeyLeftOrRight;

    /**
     *
     */
    TInt iVideoDisplayStartIndex;

    /**
     *
     */
    TInt iVideoDisplayEndIndex;

    /**
     *
     */
    TRect iVideoDisplayEndRect;

    /**
     * Text layout for main pane's end text.
     */
    TAknLayoutText iLayoutTextEnd;

    /**
     * Text layout for main pane's start text.
     */
    TAknLayoutText iLayoutTextStart;

    /**
     * Layout for start thumbnail.
     */
    TAknLayoutRect iLayoutRectStart;

    /**
     * Layout for end thumbnail.
     */
    TAknLayoutRect iLayoutRectEnd;

    /**
     * Layout for trim timeline icon.
     */
    TAknLayoutRect iLayoutRectIcon;

    /**
     * Layout for cut frame.
     */
    TAknLayoutRect iLayoutRectCutFrame;

    /**
     * Text shown above the end thumbnail.
     */
    HBufC* iEndText;

    /**
     * Text shown above the start thumbnail.
     */
    HBufC* iStartText;

    /**
     *
     */
    CVeiCutterBar* iCutterBar;
    /**
     *
     */
    CVedMovie& iMovie;

    /**
     *
     */
    CVeiVideoDisplay* iVideoDisplayStart;

    /**
     *
     */
    CVeiVideoDisplay* iVideoDisplayEnd;

    CVeiVideoDisplay* iVideoDisplay;

    TPreviewState iPreviewState;

    TRect iPreviewRect;

    CVeiFrameTaker* iFrameTaker;

    /*
     * Updated by NotifyVideoClipAdded( CVedMovie& aMovie, TInt aIndex ) 
     */
    TInt iClipIndex;

    CVeiTrimForMmsView& iView;


    TRect iStartTextBox;
    TRect iEndTextBox;
    TRect iVideoDisplayStartRect;
    TRect iTimelineRect;
    TRect iCutIconRect;
    TRect iAdjustRect;

    TPoint iCutIconPoint;
    TSize iCutIconSize;

    TTimeIntervalMicroSeconds iSeekPos;
    TTimeIntervalMicroSeconds iSeekEndPos;
    TTimeIntervalMicroSeconds iDuration;
    TInt iKeyRepeatCount;
    /** Background context. Skin stuff. */
    CAknsBasicBackgroundControlContext* iBgContext;

    CVeiDisplayLighter* iScreenLight;
    TInt iMaxMmsSize;
    TBool iBlack;


    TBool iSeekEvent;
    /**
     * Progress dialog.
     */
    CAknProgressDialog* iProgressNote;

    /** Progress info for the progress note. */
    CEikProgressInfo* iProgressInfo;
    TBool iKeyEnable;
};

#endif
