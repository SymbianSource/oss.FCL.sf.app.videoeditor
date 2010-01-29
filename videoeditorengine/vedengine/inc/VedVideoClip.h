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





#ifndef __VEDVIDEOCLIP_H__
#define __VEDVIDEOCLIP_H__


#include <e32base.h>
#include <f32file.h>

#include "VedCommon.h"
#include "VedVideoClipInfo.h"


/*
 *  Forward declarations.
 */
class CVedMovieImp;
class CAudClip;


/**
 * Individual video clip stored as a single bitstream.
 *
 * @see CVedMovie
 */
class CVedVideoClip : public CBase
{
public:

    /* Constructors. */

    static CVedVideoClip* NewL(CVedMovieImp* aMovie, const TDesC& aFileName,
							   TInt aIndex, CAudClip* aAudioClip,
							   MVedVideoClipInfoObserver& aObserver);

    static CVedVideoClip* NewL(CVedMovieImp* aMovie, 
                               CVedVideoClipGenerator& aGenerator, TInt aIndex, 
                               MVedVideoClipInfoObserver& aObserver, TBool aIsOwnedByVideoClip);
                               
    static CVedVideoClip* NewL(CVedMovieImp* aMovie, RFile* aFileHandle,
							   TInt aIndex, CAudClip* aAudioClip,
							   MVedVideoClipInfoObserver& aObserver);                               

    /* Destructor. */

    /**
     * Destroys the object and releases all resources.
     */    
    virtual ~CVedVideoClip();


    /* Property methods. */
    
    /**
     * Returns a video clip info object to get detailed information about
     * the original video clip. Note that the specified editing operations 
     * (for example, cutting or muting audio) do <em>not</em>
     * affect the values returned by the info object.
     * 
     * @return  pointer to a video clip info instance
     */
    CVedVideoClipInfo* Info();

    /**
     * Returns whether this video clip with the specified editing operations 
     * applied (for example, changing speed or muting) has an audio track or not.
     *
     * @return  <code>ETrue</code> if clip has an audio track;
     *          <code>EFalse</code> otherwise
     */
    TBool EditedHasAudio() const;


    /* Movie methods. */

    /**
     * Returns the movie that this video clip is part of.
     * 
     * @return  movie
     */
    CVedMovieImp* Movie();

    /**
     * Returns the index of this video clip in the movie.
     * 
     * @return  index of the clip
     */
     TInt Index() const;

    /**
     * Sets the index of this video clip in the movie. 
     * Panics with code <code>EVideoClipIllegalIndex</code> 
     * if the clip index is invalid.
     *
     * @param aIndex  new index of this clip
     */
    void SetIndex(TInt aIndex);


    /* Effect methods. */

    /**
     * Returns the playback speed of this video clip. Playback speed is
     * specified as parts per thousand of the normal playback speed. For example.
     * 1000 means the normal speed, 750 means 75% of the normal speed, and so on.
     *
     * @return  playback speed
     */
    TInt Speed() const;

    /**
     * Sets the playback speed of this video clip. Playback speed is
     * specified as parts per thousand of the normal playback speed. For example.
     * 1000 means the normal speed, 750 means 75% of the normal speed, and so on.
     * Panics with <code>EVideoClipIllegalSpeed</code> if playback speed is
     * illegal.
     *
     * @param aSpeed  playback speed; must be between 1 and 1000
     */
    void SetSpeed(TInt aSpeed);
    
    /**
     * Returns the color effect of this video clip.
     *
     * @return  color effect
     */
    TVedColorEffect ColorEffect() const;
    
    /**
     * Sets the color effect of this video clip.
     *
     * @param aColorEffect  color effect
     */
    void SetColorEffect(TVedColorEffect aColorEffect);


    /** 
     * Returns the color tone.
     *
     * @return color tone
     */
    TRgb ColorTone() const;
    
    /**
     * Sets the color tone.
     *
     * @param aColorTone  color tone
     */
    void SetColorTone(TRgb aColorTone);
    
    /* Audio methods. */
    
    /**
     * Returns whether this video clip can be muted or not (that is,
     * whether the mute setting has any effect). For example, if
     * this video clip has no audio track, it can never have audio
     * even if the mute setting is false.
     *
     * @return  <code>ETrue</code> if this video clip can be muted;
     *          <code>EFalse</code> otherwise
     */
    TBool IsMuteable() const;

    /**
     * Returns whether the audio track of this video clip is mute or not.
     * This covers all mute cases: user muted, automatically muted (slow motion), 
     * and missing audio track.
     *
     * @return  <code>ETrue</code> if the audio track is muted;
     *          <code>EFalse</code> otherwise
     */
    TBool Muting() const;
    
    /**
     * Returns whether the audio track of this video clip is explicitly 
     * muted by user or not. Returns ETrue also if there is no audio track at all.
     *
     * @return  <code>ETrue</code> if the audio track is muted;
     *          <code>EFalse</code> otherwise
     */
    TBool IsMuted() const;
    
    /**
     * Returns whether the audio track of this video clip is normalized or not.
     *
     * @return  <code>ETrue</code> if the audio track is normalized;
     *          <code>EFalse</code> otherwise
     */
    TBool Normalizing() const;

    
    /**
     * Sets whether the audio track of this video clip is muted or not.
     *
     * @param aVolume  <code>ETrue</code> to mute the audio track;
     *                 <code>EFalse</code> not to mute the audio track
     */
    void SetMuted(TBool aMuted);
    
    /**
     * Sets whether the audio track of this video clip is normalized or not.
     *
     * @param aNormalizing  <code>ETrue</code> to normalize the audio track,
     *                      <code>EFalse</code> not to normalize 
     */
    void SetNormalizing(TBool aNormalizing);
    
    /**
     * Inserts a dynamic level mark to a clip.
     *
     * Possible leave codes:
     *	- <code>KErrNoMemory</code> if memory allocation fails
     *
     * @param	aMark mark to be added 
     * @return  index of the mark inserted
     */
    TInt InsertDynamicLevelMarkL(const TVedDynamicLevelMark& aMark);
	
    /**
     * Removes a dynamic level mark in a clip.
     *
     * @param	aIndex index of the removed mark in this clip
     * @return  ETrue if mark was removed, EFalse otherwise
     */
    TBool RemoveDynamicLevelMark(TInt aIndex);
    
    /**
     * Returns a dynamic level mark at the specified index
     * If the index is illegal, the method panics with the code 
     * <code>EIllegalDynamicLevelMarkIndex</code> 
     *
     * @return  A dynamic level mark
     */
    TVedDynamicLevelMark DynamicLevelMark(TInt aIndex) const;
	
    /**
     * Returns the number of dynamic level marks
     * 
     * @return  The number of dynamic level mark
     */
    TInt DynamicLevelMarkCount() const;
    
    /**
    * Sets common volume gain for the clip. It is used to store
    * the gain; the actual processing will be based on dynamic level
    * marks which are set based on the gain value just before processing.
    * Since global gain setting may affect the dynamic level mark,
    * we need different variable to store the clip-specific gain also after the processing.
    * I.e. dynamic level marks do not have effect to this value.
    *
    * @param aVolumeGain
    */
    void SetVolumeGain(TInt aVolumeGain);
    
    /**
    * Gets common volume gain for the clip. 
    * Since global gain setting may affect the dynamic level mark,
    * we need different variable to store the clip-specific gain also after the processing.
    *
    * @param aVolumeGain
    */
    TInt GetVolumeGain();

    /* Timing methods. */

    /**
     * Returns the cut in time of this video clip in clip timebase.
     *
     * @return  cut in time in microseconds in clip timebase
     */
    TTimeIntervalMicroSeconds CutInTime() const;

    /**
     * Sets the cut in time of this video clip in clip timebase.
     * Panics with <code>EVideoClipIllegalCutInTime</code> if
     * cut in time is illegal.
     *
     * @param aCutInTime  cut in time in microseconds in clip timebase
     */
    void SetCutInTime(TTimeIntervalMicroSeconds aCutInTime);

    /**
     * Returns the cut out time of this video clip in clip timebase.
     *
     * @return  cut out time in microseconds in clip timebase
     */
    TTimeIntervalMicroSeconds CutOutTime() const;

    /**
     * Sets the cut out time of this video clip in clip timebase. 
     * Panics with <code>EVideoClipIllegalCutOutTime</code> if
     * cut out time is illegal.
     *
     * @param aCutOutTime  cut out time in microseconds in clip timebase
     */
    void SetCutOutTime(TTimeIntervalMicroSeconds aCutOutTime);

    /**
     * Returns the start time of this video clip in movie timebase.
     *
     * @return  start time in microseconds in movie timebase
     */
    TTimeIntervalMicroSeconds StartTime() const;

    /**
     * Returns the end time of this video clip in movie timebase.
     *
     * @return  end time in microseconds in movie timebase
     */
    TTimeIntervalMicroSeconds EndTime() const;

    /**
     * Returns the duration of this video clip with the specified
     * editing operations applied (for example, cutting 
     * and changing speed)
     * 
     * @return  duration in microseconds
     */
    TTimeIntervalMicroSeconds EditedDuration() const;

    /**
     * Update the audio clip timings.
     */
    void UpdateAudioClip();
    
protected: 


private:
    /* 
    * Default contructor 
    */
    CVedVideoClip(CVedMovieImp* aMovie, CAudClip* aAudioClip);

    /*
    * Symbian OS two phased constructor.
    */ 
    void ConstructL(const TDesC& aFileName, TInt aIndex,
                    MVedVideoClipInfoObserver& aObserver);

    void ConstructL(CVedVideoClipGenerator& aGenerator, TInt aIndex, 
                    MVedVideoClipInfoObserver& aObserver, TBool aIsOwnedByVideoClip);
                    
    void ConstructL(RFile* aFileHandle, TInt aIndex,
                    MVedVideoClipInfoObserver& aObserver);                    


private:
    // Member variables
    
    // Movie class this clip is part of.
    CVedMovieImp* iMovie;
    // Index of this clip in movie.
    TInt iIndex;
    // File of the video clip.
    RFile iLockFile;
    // Info class of this video clip.
    CVedVideoClipInfo* iInfo;

    // Speed effect of this video clip.
    TInt iSpeed;
    // Color effect of this video clip.
    TVedColorEffect iColorEffect;
    // Transition effect of this video clip.
    TVedMiddleTransitionEffect iMiddleTransitionEffect;
    
    // Cut in time of this video clip.
    TTimeIntervalMicroSeconds iCutInTime;
    // Cut out time of this video clip.
    TTimeIntervalMicroSeconds iCutOutTime;
    // Start time of this video clip.
    TTimeIntervalMicroSeconds iStartTime;

	// Audio track of this video clip
	CAudClip* iAudClip;
	// Audio track muting: slow motion mutes it automatically, but switching back to normal speed 
	// should unmute it unless user has muted it on purpose.
	TBool iUserMuted;
	// color tone
	TRgb iColorTone;
	
	TBool iLockFileOpened;
	
    friend class CVedMovieImp;
    friend class CVedMovieAddClipOperation;
};


#endif // __VEDVIDEOCLIP_H__

