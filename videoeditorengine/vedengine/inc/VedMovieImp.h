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




#ifndef __VEDMOVIEIMP_H__
#define __VEDMOVIEIMP_H__


#include <e32base.h>
#include <f32file.h>
#include "VedMovie.h"
#include "VedVideoClip.h"
#include "VedAudioClipInfoImp.h"
#include "AudObservers.h"
#include "AudCommon.h"
#include "Vedqualitysettingsapi.h"
#include "ctrtranscoder.h"


#if ( defined (__WINS__) || defined (__WINSCW__) )
const TInt KVEdMaxFrameRate = 10;
#else 
const TInt KVEdMaxFrameRate = 15;
#endif


/*
 *  Forward declarations.
 */
class CMovieProcessor;
class CVedMovie;
class CVedMovieImp;
class CVedMovieAddClipOperation;
class CVedMovieProcessOperation;
class CAudSong;
class CVedCodecChecker;


/**
 * Video movie, which consists of zero or more video clips and zero or more audio clips.
 *
 * @see  CVedVideoClip
 * @see  CVedAudioClip
 */
class CVedMovieImp : public CVedMovie, MAudSongObserver, MVedAudioClipInfoObserver
    {
public:
    /* Constructors. */

    CVedMovieImp(RFs *aFs);

    void ConstructL();

    /**
     * Destroys the object and releases all resources.
     */
    ~CVedMovieImp();


    /* Property methods. */
    
    /**
     * Returns the quality setting of this movie.
     *
     * @return  quality setting of this movie
     */
    TVedMovieQuality Quality() const;

    /**
     * Sets the quality setting of this movie.
     *
     * @param aQuality  quality setting
     */
    void SetQuality(TVedMovieQuality aQuality);

    /**
     * Returns the video format of this movie. 
     * 
     * @return  video format of this movie
     */
    TVedVideoFormat Format() const;

    /**
     * Returns the video type of this movie. 
     * 
     * @return  video type of this movie
     */
    TVedVideoType VideoType() const;

    /**
     * Returns the resolution of this movie. Panics with 
     * <code>TVedPanic::EMovieEmpty</code> if there are no clips 
     * in the movie.
     * 
     * @return  resolution of the movie
     */
    TSize Resolution() const;

    /**
     * Returns the recommended maximum framerate of this movie..
     * <p>
     * Note that the returned maximum framerate is a recommendation,
     * not a guarantee. For example, the video clip generators inserted
     * in this movie should not generate frames at higher framerates 
     * than the recommendation. The movie may, however, exceed this
     * recommendation. For example, the framerate may be higher if the 
     * framerates of some of the video clips are higher than the 
     * recommendation and it is not possible to transcode them to reduce 
     * the framerate.
     *
     * @return  maximum framerate in frames per second
     */
    TInt MaximumFramerate() const;

    /**
     * Returns the audio type of the movie audio track.
     * 
     * @return  audio type of the movie audio track
     */
    TVedAudioType AudioType() const;

    /**
     * Returns the audio sampling rate of the movie audio track.
     *
     * @return  audio sampling rate of the movie audio track.
     */
    TInt AudioSamplingRate() const;

    /**
     * Sets the output parameters for the movie. Leaves
     * with KErrNotSupported if a parameter is illegal,
     * e.g., target bitrate is too high for the given 
     * codec. This method overrides the SetQuality method
     *
     * Possible leave codes:
     *  - <code>KErrNotSupported</code> if setting is not valid
     *
     * @param Output parameters
     */
    virtual void SetOutputParametersL(TVedOutputParameters& aOutputParams);

    /**
     * Returns the audio channel mode of the movie audio track.
     * 
     * @return  audio channel mode of the movie audio track.
     */
    TVedAudioChannelMode AudioChannelMode() const;
    
    /**
     * Returns the audio bitrate mode of the movie audio track.
     *
     * @return audio bitrate mode
     */
    TVedBitrateMode AudioBitrateMode() const;

    /**
     * Returns the target bitrate of the movie audio track.
     * 
     * @return  target bitrate of the movie audio track.
     */
    TInt AudioBitrate() const;

    /**
     * Returns the required bitrate of the movie video track.
     * If nonzero, indicates real target bitrate => requires transcoding
     * 
     * @return  Requested bitrate of the movie video track.
     */
    TInt VideoBitrate() const;
    
    /**
     * Returns the "standard" or default bitrate of the movie video track.
     * If input is something else, no transcoding is needed, but new content 
     * is encoded using this rate. If there is also requested bitrate, the values 
     * should be the same. The "standard" does not necessarily mean the bitrate
     * is from video coding standard, it can be, but it can be also something else,
     * depending on the product variant.
     * 
     * @return  Standard bitrate of the movie video track.
     */
    TInt VideoStandardBitrate() const;

    /**
     * Returns the target framerate of the movie video track.
     * 
     * @return  target framerate of the movie video track.
     */
    TReal VideoFrameRate() const;

    /**
     * Returns the total duration of this movie.
     * 
     * @return  duration in microseconds
     */
    inline TTimeIntervalMicroSeconds Duration() const;

    /**
     * Returns an estimate of the total size of this movie.
     * 
     * @return  size estimate in bytes
     */
    TInt GetSizeEstimateL() const;

    /**
     * Estimates end cutpoint with given target size and start cutpoint for current movie.
     *
     * @param aTargetSize  Target filesize for section indicated by aStartTime and aEndTime.
     * @param aStartTime   Start time for first frame included in cutted section. 
     * @param aEndTime     On return contains estimated end time with given start time and section target filesize.
     */
    void GetDurationEstimateL(TInt aTargetSize, TTimeIntervalMicroSeconds aStartTime, TTimeIntervalMicroSeconds& aEndTime);

    /**
     * Returns whether movie properties meet MMS compatibility
     * 
     * @return  ETrue if MMS compatible, else EFalse
     */
    TBool IsMovieMMSCompatible() const;


    /* Video clip management methods. */

    /**
     * Returns the number of video clips in this movie.
     *
     * @return  number of video clips
     */
    inline TInt VideoClipCount() const;

    /** 
     * Returns the video clip at the specified index. 
     * Used by underlying modules and friend classes. Not part of public API.
     * Panics with code <code>USER-130</code> if the clip index is invalid.
     *
     * @param aIndex  index
     *
     * @return  clip at the specified index.
     */ 
    CVedVideoClip* VideoClip(TInt aIndex) const;

    /** 
     * Inserts the specified video clip to the specified index in this movie. 
     * The observers are notified when the clip has been added or adding clip 
     * has failed. Panics with <code>EMovieAddOperationAlreadyRunning</code> 
     * if another add video or audio clip operation is already running.
     * Panics with code <code>USER-130</code> if the clip index is invalid.
     *
     * Possible leave codes:
     *  - <code>KErrNoMemory</code> if memory allocation fails
     *
     * @param aFileName  file name of the clip to add
     * @param aIndex     index the clip should be inserted at
     */
    void InsertVideoClipL(const TDesC& aFileName, TInt aIndex);


    void InsertVideoClipL(CVedVideoClipGenerator& aGenerator, TBool aIsOwnedByVideoClip,
                          TInt aIndex);

    /** 
     * Removes the video clip at the specified index from this movie.
     * Panics with code <code>USER-130</code> if the clip index is invalid.
     *
     * @param aIndex  index of the clip to be removed
     */
    void RemoveVideoClip(TInt aIndex);


    /* Transition effect management methods. */

    /** 
     * Returns the start transition effect of this movie. Panics with 
     * <code>TVedPanic::EMovieEmpty</code> if there are no video clips 
     * in the movie.
     *
     * @return  start transition effect
     */ 
    TVedStartTransitionEffect StartTransitionEffect() const;

    /** 
     * Sets the start transition effect of this movie. Panics with 
     * <code>TVedPanic::EMovieEmpty</code> if there are no video clips 
     * in the movie.
     *
     * @param aEffect  start transition effect
     */ 
    void SetStartTransitionEffect(TVedStartTransitionEffect aEffect);

    /**
     * Returns the number of middle transition effects in this movie.
     * Note that this is the same as the number of video clips minus one.
     *
     * @return  number of middle transition effects
     */
    TInt MiddleTransitionEffectCount() const;

    /** 
     * Returns the middle transition effect at the specified index. 
     * Panics with code <code>USER-130</code> if the index is invalid.
     *
     * @param aIndex  index
     *
     * @return  middle transition effect at the specified index
     */ 
    TVedMiddleTransitionEffect MiddleTransitionEffect(TInt aIndex) const;

    /** 
     * Sets the middle transition effect at the specified index. 
     * Panics with code <code>USER-130</code> if the index is invalid.
     *
     * @param aEffect  middle transition effect
     * @param aIndex   index
     */ 
    void SetMiddleTransitionEffect(TVedMiddleTransitionEffect aEffect, TInt aIndex);

    /** 
     * Returns the end transition effect of this movie. Panics with 
     * <code>TVedPanic::EMovieEmpty</code> if there are no video clips 
     * in the movie.
     *
     * @return  end transition effect
     */ 
    TVedEndTransitionEffect EndTransitionEffect() const;

    /** 
     * Sets the end transition effect of this movie. Panics with 
     * <code>TVedPanic::EMovieEmpty</code> if there are no video clips 
     * in the movie.
     *
     * @param aEffect  end transition effect
     */ 
    void SetEndTransitionEffect(TVedEndTransitionEffect aEffect);
    

    /* Audio clip management methods. */

    /**
     * Returns the number of audio clips in this movie.
     *
     * @return  number of audio clips
     */
    TInt AudioClipCount() const;    

    /** 
     * Adds the specified audio clip to this movie. The observers are notified
     * when the clip has been added or adding clip has failed. Panics with 
     * <code>EMovieAddOperationAlreadyRunning</code> if another add video or
     * audio clip operation is already running.
     * 
     * Possible leave codes:
     *  - <code>KErrNoMemory</code> if memory allocation fails
     *
     * @param aFileName   file name of the clip to add
     * @param aStartTime  start time of the clip in movie timebase
     * @param aCutInTime  cut in time of the clip
     * @param aCutOutTime cut out time of the clip; or 
     *                    <code>KVedAudioClipOriginalDuration</code> to specify
     *                    that the original duration of the clip should be used
     */
    void AddAudioClipL(const TDesC& aFileName,
            TTimeIntervalMicroSeconds aStartTime,
            TTimeIntervalMicroSeconds aCutInTime,
            TTimeIntervalMicroSeconds aCutOutTime);

    /** 
     * Removes the audio clip at the specified index from this movie.
     * Panics with code <code>USER-130</code> if the clip index is invalid.
     *
     * @param aIndex  index of the clip to be removed
     */
    void RemoveAudioClip(TInt aIndex);
    
    
    /* Whole movie management methods. */
    
    /** 
     * Removes all video and audio clips and clears all transitions.
     */
    void Reset();


    /* Processing methods. */

    /**
     * Starts a video processing operation. This method is asynchronous and 
     * returns immediately. The processing will happen in the background and
     * the observer will be notified about the progress of the operation.
     * Processed data is written into the specified file. Panics with 
     * <code>TVedPanic::EMovieEmpty</code> if there are no clips 
     * in the movie.
     * 
     * Possible leave codes:
     *  - <code>KErrNoMemory</code> if memory allocation fails
     *  - <code>KErrAccessDenied</code> if the file access is denied
     *  - <code>KErrDiskFull</code> if the disk is full
     *  - <code>KErrWrite</code> if not all data could be written
     *  - <code>KErrBadName</code> if the filename is bad
     *  - <code>KErrDirFull</code> if the directory is full
     * 
     * @param aObserver  observer to be notified of the processing status
     * @param aFileName  name of the file to be written
     */
    void ProcessL(const TDesC& aFileName,
                           MVedMovieProcessingObserver& aObserver);

    /**
     * Cancels the current video processing operation. If there is no 
     * operation in progress, the function does nothing.
     */
    void CancelProcessing();


    /* Observer methods. */

    /**
     * Registers a movie observer. Panics with panic code 
     * <code>EMovieObserverAlreadyRegistered</code> if the movie observer is 
     * already registered.
     *
     * @param aObserver  observer that will receive the events
     */
    void RegisterMovieObserverL(MVedMovieObserver* aObserver);

    /**
     * Unregisters a movie observer.
     *
     * @param aObserver  observer to be unregistered
     */
    void UnregisterMovieObserver(MVedMovieObserver* aObserver);

    /* Video Clip Methods */
    /**
     * Returns a video clip info object to get detailed information about
     * the original video clip. Note that the specified editing operations 
     * (for example, cutting or muting audio) do <em>not</em>
     * affect the values returned by the info object.
     * 
     * @param aIndex  index of video clip in movie
     * @return  pointer to a video clip info instance
     */
    CVedVideoClipInfo* VideoClipInfo(TInt aIndex) const;

    /**
     * Returns whether this video clip with the specified editing operations 
     * applied (for example, changing speed or muting) has an audio track or not.
     *
     * @param aIndex  index of video clip in movie
     * @return  <code>ETrue</code> if clip has an audio track;
     *          <code>EFalse</code> otherwise
     */
    TBool VideoClipEditedHasAudio(TInt aIndex) const;

    /**
     * Sets the index of this video clip in the movie. 
     * Panics with code <code>EVideoClipIllegalIndex</code> 
     * if the clip index is invalid.
     *
     * @param aIndex  index of video clip in movie
     * @param aNewIndex  new index of this clip
     */
    void VideoClipSetIndex(TInt aOldIndex, TInt aNewIndex);

    /**
     * Returns the playback speed of this video clip. Playback speed is
     * specified as parts per thousand of the normal playback speed. For example.
     * 1000 means the normal speed, 750 means 75% of the normal speed, and so on.
     *
     * @param aIndex  index of video clip in movie
     * @return  playback speed
     */
    TInt VideoClipSpeed(TInt aIndex) const;

    /**
     * Sets the playback speed of this video clip. Playback speed is
     * specified as parts per thousand of the normal playback speed. For example.
     * 1000 means the normal speed, 750 means 75% of the normal speed, and so on.
     * Panics with <code>EVideoClipIllegalSpeed</code> if playback speed is
     * illegal.
     *
     * @param aIndex  index of video clip in movie
     * @param aSpeed  playback speed; must be between 1 and 1000
     */
    void VideoClipSetSpeed(TInt aIndex, TInt aSpeed);

    /**
     * Returns the color effect of this video clip.
     *
     * @param aIndex  index of video clip in movie
     * @return  color effect
     */
    TVedColorEffect VideoClipColorEffect(TInt aIndex) const;
    
    /**
     * Sets the color effect of this video clip.
     *
     * @param aIndex  index of video clip in movie
     * @param aColorEffect  color effect
     */
    void VideoClipSetColorEffect(TInt aIndex, TVedColorEffect aColorEffect);

    /**
     * Returns whether this video clip can be muted or not (that is,
     * whether the mute setting has any effect). For example, if
     * this video clip has no audio track, it can never have audio
     * even if the mute setting is false.
     *
     * @param aIndex  index of video clip in movie
     * @return  <code>ETrue</code> if this video clip can be muted;
     *          <code>EFalse</code> otherwise
     */
    TBool VideoClipIsMuteable(TInt aIndex) const;

    /**
     * Returns whether the audio track of this video clip is muted or not.
     *
     * @param aIndex  index of video clip in movie
     * @return  <code>ETrue</code> if the audio track is muted;
     *          <code>EFalse</code> otherwise
     */
    TBool VideoClipIsMuted(TInt aIndex) const;

    /**
     * Sets whether the audio track of this video clip is muted or not.
     *
     * @param aIndex  index of video clip in movie
     * @param aVolume  <code>ETrue</code> to mute the audio track;
     *                 <code>EFalse</code> not to mute the audio track
     */
    void VideoClipSetMuted(TInt aIndex, TBool aMuted);

    /**
     * Returns whether the audio track of this video clip is normalized or not.
     *
     * @param aIndex  index of video clip in movie
     * @return  <code>ETrue</code> if the audio track is normalized;
     *          <code>EFalse</code> otherwise
     */
    TBool VideoClipNormalizing(TInt aIndex) const;

    /**
     * Sets whether the audio track of this video clip is normalized or not.
     *
     * @param aIndex  index of video clip in movie
     * @param aVolume  <code>ETrue</code> to normalize the audio track;
     *                 <code>EFalse</code> not to normalize the audio track
     */
    void VideoClipSetNormalizing(TInt aIndex, TBool aNormalizing);

    /**
     * Inserts a new dynamic level mark to the video clip. The mark timing
     * must be within the time boundaries of the video clip.
     *
     * @param aIndex  index of the video clip
     * @param aMark   dynamic level mark to be inserted
     */
    void VideoClipInsertDynamicLevelMarkL(TInt aIndex, TVedDynamicLevelMark aMark);
    
    /**
     * Removes the specified dynamic level mark from the specified video clip.
     * The mark index must be between 0 and number of dynamic level marks in the clip.
     *
     * @param aClipIndex  index of the video clip
     * @param aMarkIndex  index of the mark to be removed
     */
    void VideoClipRemoveDynamicLevelMark(TInt aClipIndex, TInt aMarkIndex);

    /**
     * Returns the number of dynamic level marks in the specified video clip.
     * 
     * @param aIndex  index of the video clip
     */
    TInt VideoClipDynamicLevelMarkCount(TInt aIndex) const;

    /**
     * Returns the specified dynamic level mark from the specified video clip.
     * 
     * @param aClipIndex  index of the video clip
     * @param aMarkIndex  index of the dynamic level mark
     */    
    TVedDynamicLevelMark VideoClipDynamicLevelMark(TInt aClipIndex, TInt aMarkIndex);

    /**
     * Returns the cut in time of this video clip in clip timebase.
     *
     * @param aIndex  index of video clip in movie
     * @return  cut in time in microseconds in clip timebase
     */
    TTimeIntervalMicroSeconds VideoClipCutInTime(TInt aIndex) const;

    /**
     * Sets the cut in time of this video clip in clip timebase.
     * Panics with <code>EVideoClipIllegalCutInTime</code> if
     * cut in time is illegal.
     *
     * @param aIndex  index of video clip in movie
     * @param aCutInTime  cut in time in microseconds in clip timebase
     */
    void VideoClipSetCutInTime(TInt aIndex, TTimeIntervalMicroSeconds aCutInTime);

    /**
     * Returns the cut out time of this video clip in clip timebase.
     *
     * @return  cut out time in microseconds in clip timebase
     */
    TTimeIntervalMicroSeconds VideoClipCutOutTime(TInt aIndex) const;

    /**
     * Sets the cut out time of this video clip in clip timebase. 
     * Panics with <code>EVideoClipIllegalCutOutTime</code> if
     * cut out time is illegal.
     *
     * @param aIndex  index of video clip in movie
     * @param aCutOutTime  cut out time in microseconds in clip timebase
     */
    void VideoClipSetCutOutTime(TInt aIndex, TTimeIntervalMicroSeconds aCutOutTime);

    /**
     * Returns the start time of this video clip in movie timebase.
     *
     * @param aIndex  index of video clip in movie
     * @return  start time in microseconds in movie timebase
     */
    TTimeIntervalMicroSeconds VideoClipStartTime(TInt aIndex) const;

    /**
     * Returns the end time of this video clip in movie timebase.
     *
     * @param aIndex  index of video clip in movie
     * @return  end time in microseconds in movie timebase
     */
    TTimeIntervalMicroSeconds VideoClipEndTime(TInt aIndex) const;

    /**
     * Returns the duration of this video clip with the specified
     * editing operations applied (for example, cutting 
     * and changing speed)
     * 
     * @param aIndex  index of video clip in movie
     * @return  duration in microseconds
     */
    TTimeIntervalMicroSeconds VideoClipEditedDuration(TInt aIndex) const;

    /* Audio Clip Methods */
    /**
     * Returns an audio clip info object to get detailed information about
     * the original audio clip. Note that the specified editing operations 
     * (for example, changing duration) do <em>not</em>
     * affect the values returned by the info object.
     * 
     * @param aIndex  index of audio clip in movie
     * @return  pointer to an audio clip info instance
     */
    CVedAudioClipInfo* AudioClipInfo(TInt aIndex) const;

    /**
     * Returns the start time of this audio clip in movie timebase.
     *
     * @param aIndex  index of audio clip in movie
     * @return  start time in microseconds in movie timebase
     */
    TTimeIntervalMicroSeconds AudioClipStartTime(TInt aIndex) const;

    /**
     * Sets the start time of this audio clip in movie timebase. 
     * Also updates the end time. Duration remains unchanged.
     * Note that since the audio clips are ordered based on their
     * start times, the index of the clip may change as a result
     * of changing the start time.
     *
     * @param aIndex  index of audio clip in movie
     * @param aStartTime  start time in microseconds in movie timebase
     */
    void AudioClipSetStartTime(TInt aIndex, TTimeIntervalMicroSeconds aStartTime);

    /**
     * Returns the end time of this audio clip in movie timebase.
     *
     * @param aIndex  index of audio clip in movie
     * @return  end time in microseconds in movie timebase
     */
    TTimeIntervalMicroSeconds AudioClipEndTime(TInt aIndex) const;

    /**
     * Returns the duration of the selected part of this clip.
     * 
     * @param aIndex  index of audio clip in movie
     * @return  duration in microseconds
     */
    TTimeIntervalMicroSeconds AudioClipEditedDuration(TInt aIndex) const;

    /**
     * Returns the cut in time of this audio clip in clip timebase.
     *
     * @param aIndex  index of audio clip in movie
     * @return  cut in time in microseconds in clip timebase
     */
    TTimeIntervalMicroSeconds AudioClipCutInTime(TInt aIndex) const;

    /**
     * Sets the cut in time of this audio clip in clip timebase.
     * Panics with <code>EAudioClipIllegalCutInTime</code> if
     * cut in time is illegal.
     *
     * @param aIndex  index of audio clip in movie
     * @param aCutInTime  cut in time in microseconds in clip timebase
     */
    void AudioClipSetCutInTime(TInt aIndex, TTimeIntervalMicroSeconds aCutInTime);

    /**
     * Returns the cut out time of this audio clip in clip timebase.
     *
     * @param aIndex  index of audio clip in movie
     * @return  cut out time in microseconds in clip timebase
     */
    TTimeIntervalMicroSeconds AudioClipCutOutTime(TInt aIndex) const;

    /**
     * Sets the cut out time of this audio clip in clip timebase.
     * Panics with <code>EAudioClipIllegalCutOutTime</code> if
     * cut in time is illegal.
     *
     * @param aIndex  index of audio clip in movie
     * @param aCutOutTime  cut out time in microseconds in clip timebase
     */
    void AudioClipSetCutOutTime(TInt aIndex, TTimeIntervalMicroSeconds aCutOutTime);

    /**
     * Returns whether this audio clip is normalized or not.
     *
     * @param aIndex  index of audio clip in movie
     * @return  <code>ETrue</code> if the audio clip is normalized;
     *          <code>EFalse</code> otherwise
     */
    TBool AudioClipNormalizing(TInt aIndex) const;

    /**
     * Sets whether this audio clip is normalized or not.
     *
     * @param aIndex  index of audio clip in movie
     * @param aVolume  <code>ETrue</code> to normalize the audio clip;
     *                 <code>EFalse</code> not to normalize the audio clip
     */
    void AudioClipSetNormalizing(TInt aIndex, TBool aNormalizing);
    
    /**
     * Inserts a new dynamic level mark to the audio clip. The mark timing
     * must be within the time boundaries of the audio clip.
     *
     * @param aIndex  index of the audio clip
     * @param aMark   dynamic level mark to be inserted
     */
    void AudioClipInsertDynamicLevelMarkL(TInt aIndex, TVedDynamicLevelMark aMark);
    
    /**
     * Removes the specified dynamic level mark from the specified audio clip.
     * The mark index must be between 0 and number of dynamic level marks in the clip.
     *
     * @param aClipIndex  index of the audio clip
     * @param aMarkIndex  index of the mark to be removed
     */
    void AudioClipRemoveDynamicLevelMark(TInt aClipIndex, TInt aMarkIndex);

    /**
     * Returns the number of dynamic level marks in the specified audio clip.
     * 
     * @param aIndex  index of the audio clip
     */
    TInt AudioClipDynamicLevelMarkCount(TInt aIndex) const;

    /**
     * Returns the specified dynamic level mark from the specified audio clip.
     * 
     * @param aClipIndex  index of the audio clip
     * @param aMarkIndex  index of the dynamic level mark
     */    
    TVedDynamicLevelMark AudioClipDynamicLevelMark(TInt aClipIndex, TInt aMarkIndex);
    
    /**
     * Returns the color tone of the specified clip.
     *
     * @return color tone
     */
    TRgb VideoClipColorTone(TInt aVideoCLipIndex) const;
    
    /**
     * Sets the color tone of the specified clip.
     *
     * @param aColorTone  color tone
     */
    void VideoClipSetColorTone(TInt aVideoClipIndex, TRgb aColorTone);
    
    /**
     * Returns a pointer to the audio song used as the sound provider for
     * this movie.
     * 
     * @return  song
     */
    CAudSong* Song();
    
    /**
     * Sets the maximum size for the movie
     * 
     * @param aLimit Maximum size in bytes
     */
    virtual void SetMovieSizeLimit(TInt aLimit);
    
    /**
     * Returns an estimate for total processing time
     * 
     * @return processing time
     */
    TTimeIntervalMicroSeconds GetProcessingTimeEstimateL();
    
    /**
     * Returns the MIME-type for the video in the movie
     * 
     * @return Video codec MIME-type
     */
    TPtrC8& VideoCodecMimeType();
    /**
    * Gets the sync interval in picture (H.263 GOB frequency)
    * 
    * @return sync interval 
    */
    TInt SyncIntervalInPicture();
    
    /**
    * Gets the random access rate
    * 
    * @return random access rate in pictures per second
    */
    TReal RandomAccessRate();
    
    /**
     * Checks if a movie observer is registered.
     *
     * @param aObserver observer to be checked
     * @return <code>ETrue</code> if the observer is registered
     *         <code>EFalse</code> otherwise
     */
    TBool MovieObserverIsRegistered(MVedMovieObserver* aObserver);

    /**
    * Set volume gain for the given video clip or all the video clips
    * @param aClipIndex    index of the clip; KVedClipIndexAll if applied for all the clips
    * @param aVolumeGain   Volume gain in +0.1 or -0.5 decibel steps
    * 
    */
    void SetVideoClipVolumeGainL(TInt aClipIndex, TInt aVolumeGain);

    /**
    * Get volume gain of the given video clip or all the video clips
    * @param aClipIndex    index of the clip; KVedClipIndexAll if applied for all the clips
    *
    * @return Volume gain in +0.1 or -0.5 decibel steps
    */
    TInt GetVideoClipVolumeGainL(TInt aClipIndex);

    /**
    * Set volume gain for the given audio clip or all the audio clips
    * @param aClipIndex    index of the clip; KVedClipIndexAll if applied for all the clips
    * @param aVolumeGain   Volume gain in +0.1 or -0.5 decibel steps 
    * 
    */
    void SetAudioClipVolumeGainL(TInt aClipIndex, TInt aVolumeGain);

    /**
    * Get volume gain of the given audio clip or all the audio clips
    * @param aClipIndex    index of the clip; KVedClipIndexAll if applied for all the clips
    *
    * @return Volume gain in +0.1 or -0.5 decibel steps
    */
    TInt GetAudioClipVolumeGainL(TInt aClipIndex);
    
    /** 
     * Inserts a video clip from the specified file to the specified index 
     * in this movie. The observers are notified when the clip has been added 
     * or adding clip has failed. Panics with <code>EMovieAddOperationAlreadyRunning</code> 
     * if another add video or audio clip operation is already running.
     * Panics with code <code>USER-130</code> if the clip index is invalid.
     * The file will be opened in EFileShareReadersOnly mode by default, 
     * and the same mode should be used by the client too if it need to open
     * the file at the same time.
     *  
     * Possible leave codes:
     *  - <code>KErrNoMemory</code> if memory allocation fails
     *
     * @param aFileHandle  file handle of the clip to add
     * @param aIndex     index the clip should be inserted at
     */    
    void InsertVideoClipL(RFile* aFileHandle, TInt aIndex);
    
    /** 
     * Adds the specified audio clip to this movie. The observers are notified
     * when the clip has been added or adding clip has failed. Panics with 
     * <code>EMovieAddOperationAlreadyRunning</code> if another add video or
     * audio clip operation is already running.
     * The file will be opened in EFileShareReadersOnly mode by default, 
     * and the same mode should be used by the client too if it need to open
     * the file at the same time.
     * 
     * Possible leave codes:
     *  - <code>KErrNoMemory</code> if memory allocation fails
     *
     * @param aFilehandle   file handle of the clip to add
     * @param aStartTime  start time of the clip in movie timebase
     * @param aCutInTime  cut in time of the clip
     * @param aCutOutTime cut out time of the clip; or 
     *                    <code>KVedAudioClipOriginalDuration</code> to specify
     *                    that the original duration of the clip should be used
     */    
    void AddAudioClipL(RFile* aFileHandle,
            TTimeIntervalMicroSeconds aStartTime,
            TTimeIntervalMicroSeconds aCutInTime = TTimeIntervalMicroSeconds(0),
            TTimeIntervalMicroSeconds aCutOutTime = KVedAudioClipOriginalDuration);
    
     
    /**
     * Starts a video processing operation. This method is asynchronous and 
     * returns immediately. The processing will happen in the background and
     * the observer will be notified about the progress of the operation.
     * Processed data is written into the specified file. Panics with 
     * <code>TVedPanic::EMovieEmpty</code> if there are no clips 
     * in the movie. Note that calling <code>ProcessL</code> may cause
     * changes in the maximum frame rates of generated clips.
     * 
     * Possible leave codes:
     *  - <code>KErrNoMemory</code> if memory allocation fails
     *  - <code>KErrAccessDenied</code> if the file access is denied
     *  - <code>KErrDiskFull</code> if the disk is full
     *  - <code>KErrWrite</code> if not all data could be written
     *  - <code>KErrBadName</code> if the filename is bad
     *  - <code>KErrDirFull</code> if the directory is full
     * 
     * @param aObserver  observer to be notified of the processing status
     * @param aFileHandle  handle of the file to be written
     */
    void ProcessL(RFile* aFileHandle,
                           MVedMovieProcessingObserver& aObserver);

private:
    /* Movie editing methods. */

    /**
     * Match the selected audio properties with input. This is used with automatic
     * quality to avoid useless transcoding of audio. 
     */
    TBool MatchAudioPropertiesWithInput( TAudFileProperties& aAudioProperties );

    /**
     * Calculates the format, video type, resolution and audio type of 
     * this movie. 
     */
    void CalculatePropertiesL();

    /**
     * Determines the output format based on input and sets up the movie accordingly
     */
    void ApplyAutomaticPropertiesL(TAudFileProperties &aAudioProperties);
    
    /**
     * Sets up the movie according to requested properties
     */
    void ApplyRequestedPropertiesL(TAudFileProperties &aAudioProperties);
    
    /**
     * Get properties for QCIF/subQCIF from quality set and sets up video
     */
    TInt GetQCIFPropertiesL(SVideoQualitySet& aLocalQualitySet);
    
    /**
     * Get properties for CIF/QVGA from quality set and sets up video
     */
    TInt GetCIFQVGAPropertiesL(TSize aSize, TReal aFrameRate, SVideoQualitySet& aLocalQualitySet);
    
    /**
     * Get properties for VGA from quality set and sets up video
     */
    TInt GetVGAPropertiesL(SVideoQualitySet& aLocalQualitySet);
    
    /**
     * Get properties for VGA 16:9 from quality set and sets up video
     */
    TInt GetVGA16By9PropertiesL(SVideoQualitySet& aLocalQualitySet);
    
    /**
    * Get properties for VGA from quality set and sets up video
    */
    TInt GetWVGAPropertiesL(SVideoQualitySet& aLocalQualitySet);
    
    /**
     * Get properties for high quality from quality set
     */
    TInt GetHighPropertiesL(SVideoQualitySet& aLocalQualitySet);
    
    /**
     * Creates temporary instance of CTRTranscoder and asks for complexity factor estimate
     * when converting given video clip to movie with set parameters.
     * The estimate represents how long processing of 1 sec input takes.
     */
    TReal AskComplexityFactorFromTranscoderL(CVedVideoClipInfo* aInfo, CTRTranscoder::TTROperationalMode aMode, TReal aInputFrameRate);
    
    /**
     * Checks whether the specified video clip can be inserted to this movie.
     *
     * @param aClip  video clip
     *
     * @return  <code>KErrNone</code>, if the clip can be inserted; 
     *          one of the system wide error codes, otherwise
     */
    TInt CheckVideoClipInsertable(CVedVideoClip *aClip) const;

    /**
     * Calculates the maximum and minimum resolution supported by this movie.
     *
     * @param aMaxRes       initial upper limit for the maximum resolution; 
     *                      after the method returns, contains the calculated
     *                      maximum resolution supported by this movie
     * @param aMinRes       initial lower limit for the minimum resolution; 
     *                      after the method returns, contains the calculated
     *                      minimum resolution supported by this movie
     * @param aDoFullCheck  <code>ETrue</code>, if all clips should be
     *                      checked to verify that they can be combined
     *                      (not necessary but useful for detecting internal 
     *                      errors); <code>EFalse</code>, if extra checks 
     *                      should be skipped to minimize execution time
     */
    void CalculateMaxAndMinResolution(TSize& aMaxRes, TSize& aMinRes,
                                      TBool aDoFullCheck) const;

    /**
     * Recalculates video clip timings.
     *
     * @param aVideoClip  Video clip
     */
    void RecalculateVideoClipTimings(CVedVideoClip* aVideoClip);    

    /**
     * Reset the movie. 
     */
    void DoReset();
    
    /**
     * Set audio fade in/out on clip boundaries
     */
    void SetAudioFadingL();
    
    /**
     * Internal helper function to set iVideoCodecMimeType to given value
     */
    void SetVideoCodecMimeType(const TText8* aVideoCodecType);

    /* Notification methods. */

    /**
     * Notify observers that the video clip has been added to movie.
     *
     * @param aMovie This movie.
     * @param aClip Added video clip.
     */
    void FireVideoClipAdded(CVedMovie* aMovie, CVedVideoClip* aClip);

    /**
     * Notify observers that the adding of video clip failed.
     *
     * @param 
     */
    void FireVideoClipAddingFailed(CVedMovie* aMovie, TInt aError);

    /**
     * Notify observers that the video clip has been removed from movie.
     *
     * @param aMovie This movie.
     * @param aIndex Index of removed video clip.
     */
    void FireVideoClipRemoved(CVedMovie* aMovie, TInt aIndex);

    /**
     * Notify observers that the video clip indices has changes.
     *
     * @param aMovie This movie.
     * @param aOldIndex Old index of video clip.
     * @param aNewIndex New index of video clip.
     */
    void FireVideoClipIndicesChanged(CVedMovie* aMovie, TInt aOldIndex, 
                                     TInt aNewIndex);

    /**
     * Notify observers that the video clip timings has been changed.
     *
     * @param aMovie This movie.
     * @param aClip The video clip.
     */
    void FireVideoClipTimingsChanged(CVedMovie* aMovie, CVedVideoClip* aClip);

    /**
     * Notify observers that the color effect of video clip has been changed.
     *
     * @param aMovie This movie.
     * @param aClip Video clip that was changed.
     */
    void FireVideoClipColorEffectChanged(CVedMovie* aMovie, CVedVideoClip* aClip);

    /**
     * Notify observers that the audio settings of video clip has been changed.
     *
     * @param aMovie This movie.
     * @param aClip Video clip that audio settings has been changed.
     */
    void FireVideoClipAudioSettingsChanged(CVedMovie* aMovie, CVedVideoClip* aClip);

    /**
     * Notify observers that the generator settings of video clip has been changed.
     *
     * @param aMovie This movie.
     * @param aClip  Video clip whose settings has been changed
     */
    void FireVideoClipGeneratorSettingsChanged(CVedMovie* aMovie, CVedVideoClip* aClip);

    /**
     * Notify observers that the descriptive name of video clip has been changed.
     *
     * @param aMovie  This movie
     * @param aClip   Video clip whose settings has been changed
     */
    void FireVideoClipDescriptiveNameChanged(CVedMovie* aMovie, CVedVideoClip* aClip);


    /**
     * Notify observers that the start transition effect of the movie has been changed.
     *
     * @param aMovie This movie.
     */
    void FireStartTransitionEffectChanged(CVedMovie* aMovie);

    /**
     * Notify observers that the middle transition effect of video clip has been changed.
     *
     * @param aMovie This movie.
     * @param aClip Video clip that was changed.
     */
    void FireMiddleTransitionEffectChanged(CVedMovie* aMovie, TInt aIndex);

    /**
     * Notify observers that the end transition effect of the movie has been changed.
     *
     * @param aMovie This movie.
     */
    void FireEndTransitionEffectChanged(CVedMovie* aMovie);

    /**
     * Notify observers that an audio clip has been added to movie.
     *
     * @param aMovie This movie.
     * @param aIndex index of added clip.
     */
    void FireAudioClipAdded(CVedMovie* aMovie, TInt aIndex);

    /**
     * Notify observers that the adding of audio clip has failed.
     *
     * @param aMovie This movie.
     * @param aError Error code.
     */
    void FireAudioClipAddingFailed(CVedMovie* aMovie, TInt aError);

    /**
     * Notify observers that the audio clip has been removed from movie. 
     *
     * @param aMovie This movie.
     * @param aInder Index of removed audio clip.
     */
    void FireAudioClipRemoved(CVedMovie* aMovie, TInt aIndex);

    /**
     * Notify observers that the audio clip indices has been changed.
     *
     * @param aMovie This movie.
     * @param aOldIndex Old index of the audio clip.
     * @param aNewIndex New index of the audio clip.
     */
    void FireAudioClipIndicesChanged(CVedMovie* aMovie, TInt aOldIndex, TInt aNewIndex);

    /**
     * Notify observers that the audio clip timings has been changed.
     *
     * @param aMovie This movie.
     * @param aClip The audio clip that timings has been changed.
     */
    void FireAudioClipTimingsChanged(CVedMovie* aMovie, CAudClip* aClip);

    /**
     * Notify observers that output parameters has been changed.
     *
     * @param aMovie This movie.
     * @param aClip The audio clip that timings has been changed.
     */
    void FireMovieOutputParametersChanged(CVedMovie* aMovie);

    /**
     * Notify observers that an audio or video clip dynamic level
     * mark has been removed.
     *
     * @param aClip   The audio clip that timings has been changed.
     * @param aIndex  Index of the removed dynamic level mark
     */
    void FireDynamicLevelMarkRemoved(CAudClip& aClip, TInt aIndex);

    /**
     * Notify observers that an audio or video clip dynamic level
     * mark has been inserted.
     *
     * @param aClip   The audio clip that timings has been changed.
     * @param aMark   Dynamic level mark that was inserted
     * @param aIndex  Index of the removed dynamic level mark
     */
    void FireDynamicLevelMarkInserted(CAudClip& aClip, TAudDynamicLevelMark& aMark, TInt aIndex);

    /**
     * Notify observers that the quality setting of the movie has been
     * changed.
     *
     * @param aMovie This movie.
     */
    void FireMovieQualityChanged(CVedMovie* aMovie);

    /**
     * Notify observers that the movie has been reseted.
     *
     * @param aMovie This movie.
     */
    void FireMovieReseted(CVedMovie* aMovie);    

private: // methods from base classes
    void NotifyClipAdded(CAudSong& aSong, CAudClip& aClip, 
        TInt aIndex, TInt aTrackIndex);
    void NotifyClipAddingFailed(CAudSong& aSong, TInt aError, TInt aTrackIndex);
    void NotifyClipRemoved(CAudSong& aSong, TInt aIndex, TInt aTrackIndex);
    void NotifyClipTimingsChanged(CAudSong& aSong, CAudClip& aClip);
    void NotifyClipIndicesChanged(CAudSong& aSong, TInt aOldIndex, 
        TInt aNewIndex, TInt aTrackIndex);
    void NotifySongReseted(CAudSong& aSong);
    void NotifyClipReseted(CAudClip& aClip);
    void NotifyDynamicLevelMarkInserted(CAudClip& aClip, 
        TAudDynamicLevelMark& aMark, TInt aIndex);
    void NotifyDynamicLevelMarkRemoved(CAudClip& aClip, TInt aIndex);
    
    void NotifyAudioClipInfoReady(CVedAudioClipInfo& aInfo, TInt aError);

private:
    // Member variables

    // File server session.
    RFs* iFs;
    // Video clip array.
    RPointerArray<CVedVideoClip> iVideoClipArray;
    // Audio clip array.
    RPointerArray<CVedAudioClipInfoImp> iAudioClipInfoArray;
    // Observer array of the movie class.
    RPointerArray<MVedMovieObserver> iObserverArray;

    // Start transition effect of the movie.
    TVedStartTransitionEffect iStartTransitionEffect;
    // End transition effect of the movie.
    TVedEndTransitionEffect iEndTransitionEffect;
    
    // Add clip operation for movie.    
    CVedMovieAddClipOperation* iAddOperation;
    // Video editor processor.
    CMovieProcessor* iProcessor;

    // Quality
    TVedMovieQuality iQuality;
    // Format.
    TVedVideoFormat iFormat;
    // Quality set assigned for the movie
    SVideoQualitySet iQualitySet;
    // Video type, both internal enum and MIME-type.
    TVedVideoType iVideoType;
    TPtrC8 iVideoCodecMimeType;
    // Resolution.
    TSize iResolution;
    // Maximum framerate.
    TInt iMaximumFramerate;
    // Random access point rate, in pictures per second.
    TReal iRandomAccessRate;
    // Standard video bitrate; can be used when encoding, but doesn't force to transcode input to this bitrate
    TInt iVideoStandardBitrate;
        
    // Used when non-zero
    TInt iVideoRestrictedBitrate;    
    TReal iVideoFrameRate;    
    // Segment interval in picture. In H.263 baseline this means number of non-empty GOB headers (1=every GOB has a header), 
    // in MB-based systems number of MBs per segment. Default is 0 == no segments inside picture
    // Coding standard & used profile etc. limit the value.
    TInt iSyncIntervalInPicture;
    // Latest estimate for processing time of the movie
    TTimeIntervalMicroSeconds iEstimatedProcessingTime;
        
    // Song
    CAudSong* iAudSong;
    
    // Whether output parameters have been set using SetOutputParametersL
    TBool iOutputParamsSet;
    
    // Video clip index stored for inserting video clip first to AudSong, then to Movie
    TInt iAddedVideoClipIndex;
    
    // File name of the added clip
    HBufC* iAddedVideoClipFilename;            
    
    RFile* iAddedVideoClipFileHandle;
    
    // whether to notify observer about dynamic
    // level marker changes
    TBool iNotifyObserver;
    
    // Codec availability checker 
    CVedCodecChecker* iCodecChecker;

    // Volume gains for all video and audio clips
    TInt iVolumeGainForAllVideoClips;
    TInt iVolumeGainForAllAudioClips;
    
    // ETrue if specific dynamic level mark(s) or global volume settings should be applied (EFalse)
    TBool iApplyDynamicLevelMark;
    
    // Quality settings manager
    CVideoQualitySelector* iQualitySelector;
    
    MVedMovieProcessingObserver* iMovieProcessingObserver;
    
    friend class CVedMovieAddClipOperation;
    friend class CVedVideoClip;
    friend class CVedAudioClip;
    friend class CVedVideoClipGenerator;
    };


/**
 * Internal class for adding video and audio clips.
 */
class CVedMovieAddClipOperation : public CActive, 
                                  public MVedVideoClipInfoObserver                                  
    {
public:
    /* 
    * Static constructor.
    */
    static CVedMovieAddClipOperation* NewL(CVedMovie* aMovie);

    /*
    * Notify that the video clip info is ready.
    */
    void NotifyVideoClipInfoReady(CVedVideoClipInfo& aInfo, 
                                          TInt aError);    

protected:
    /*
    * From CActive.
    * Standard active object RunL function.
    */
    void RunL();
 
    /*
    * From CActive.
    * Standard active object DoCancel function.
    */
    void DoCancel();

private:
    /* Default constructor */
    CVedMovieAddClipOperation(CVedMovie* aMovie);
    
    /* Standard Symbian OS two phased contructor */
    void ConstructL();

    /* Destructor */
    ~CVedMovieAddClipOperation();

    /* Called when video add operation has been completed */
    void CompleteAddVideoClipOperation();


private:
    // Pointer to movie class this operation is part of.
    CVedMovieImp* iMovie;

    // Video clip to be added.
    CVedVideoClip* iVideoClip;
    // Does the video clip to be added own the generator?
    TBool iIsVideoGeneratorOwnedByVideoClip;

    // Error code of add operation
    TInt iError;

    friend class CVedMovieImp;
    };


#endif // __VEDMOVIEIMP_H__

