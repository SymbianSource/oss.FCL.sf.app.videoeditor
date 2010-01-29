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


  

#ifndef __VEDMOVIE_H__
#define __VEDMOVIE_H__

/*
 *  Constant definitions.
 */
#define KVedAudioClipOriginalDuration TTimeIntervalMicroSeconds(-1)

const TInt KVedClipIndexAll(-1);

#include <e32base.h>
#include <f32file.h>
#include "VedVideoClipInfo.h"
#include "VedAudioClipInfo.h"

class CVedMovie;
class TVedDynamicLevelMark;

/**
 * Observer for movie events. 
 * <p>
 * Note that every change operation that is made to a movie or the clips it consists of 
 * results in a maximum of one notification method called (that is, more than one 
 * notification method is never called as a result of a single change). For example,
 * changing the index of a clip results in the <code>NotifyVideoClipIndicesChanged()</code>
 * method being called once. The <code>NotifyVideoClipTimingsChanged()</code> method is not 
 * called even if the timings of several clips may have changed as a result. See the
 * descriptions of the notification methods for more detailed information.
 *
 * @see  CVedMovie
 */
class MVedMovieObserver 
    {
public:

    /**
     * Called to notify that a new video clip has been successfully
     * added to the movie. Note that the indices and the start and end times
     * of the video clips after the new clip have also changed as a result.
     * Note that the transitions may also have changed. 
     *
     * @param aMovie  movie
     * @param aIndex  index of video clip in movie
     */
    virtual void NotifyVideoClipAdded(CVedMovie& aMovie, TInt aIndex) = 0;

    /**
     * Called to notify that adding a new video clip to the movie has failed.
     *
     * Possible error codes:
     *  - <code>KErrNotFound</code> if there is no file with the specified name
     *    in the specified directory (but the directory exists)
     *  - <code>KErrPathNotFound</code> if the specified directory
     *    does not exist
     *  - <code>KErrUnknown</code> if the specified file is of unknown format
     *  - <code>KErrNotSupported</code> if the format of the file is recognized but
     *    adding it to the movie is not supported (e.g., it is of different resolution
     *    or format than the other clips)
     *
     * @param aMovie  movie
     * @param aError  one of the system wide error codes
     */
    virtual void NotifyVideoClipAddingFailed(CVedMovie& aMovie, TInt aError) = 0;

    /**
     * Called to notify that a video clip has been removed from the movie.
     * Note that the indices and the start and end times of the video clips after 
     * the removed clip have also changed as a result. Note that the 
     * transitions may also have changed.
     *
     * @param aMovie  movie
     * @param aIndex  index of the removed video clip
     */
    virtual void NotifyVideoClipRemoved(CVedMovie& aMovie, TInt aIndex) = 0;
    
    /**
     * Called to notify that a video clip has moved (that is, its index and 
     * start and end times have changed). Note that the indices and the start and
     * end times of the clips between the old and new indices have also changed 
     * as a result. Note that the transitions may also have changed.
     *
     * @param aMovie     movie
     * @param aOldIndex  old index of the moved clip
     * @param aNewIndex  new index of the moved clip
     */
    virtual void NotifyVideoClipIndicesChanged(CVedMovie& aMovie, TInt aOldIndex, 
                                               TInt aNewIndex) = 0;

    /**
     * Called to notify that the timings (that is, the cut in or cut out time or
     * the speed and consequently the end time, edited duration, and possibly audio
     * settings) of a video clip have changed (but the index of the clip has 
     * <em>not</em> changed). Note that the start and end times of the video clips 
     * after the changed clip have also changed.
     *
     * @param aMovie  movie
     * @param aClip   changed video clip
     */
    virtual void NotifyVideoClipTimingsChanged(CVedMovie& aMovie,
                                               TInt aIndex) = 0;

    /**
     * Called to notify that the color effect or a color tone of the existing effect
     * of a video clip has changed.
     *
     * @param aMovie  movie
     * @param aClip   changed video clip
     */
    virtual void NotifyVideoClipColorEffectChanged(CVedMovie& aMovie,
                                                   TInt aIndex) = 0;
    
    /**
     * Called to notify that the audio settings of a video clip have changed. 
     *
     * @param aMovie  movie
     * @param aClip   changed video clip
     */
    virtual void NotifyVideoClipAudioSettingsChanged(CVedMovie& aMovie,
                                                     TInt aIndex) = 0;

    /**
     * Called to notify that some generator-specific settings of 
     * a generated video clip have changed.
     *
     * @param aMovie  movie
     * @param aClip   changed video clip
     */
    virtual void NotifyVideoClipGeneratorSettingsChanged(CVedMovie& aMovie,
                                                         TInt aIndex) = 0;

    /**
     * Called to notify that the descriptive name of a clip has changed. 
     *
     * @param aMovie  movie
     * @param aIndex  changed video clip index
     */
    virtual void NotifyVideoClipDescriptiveNameChanged(CVedMovie& aMovie,
                                                                TInt aIndex) = 0;

    /**
     * Called to notify that the start transition effect of the movie
     * has changed (but no other changes have occurred).
     *
     * @param aMovie  movie
     */
    virtual void NotifyStartTransitionEffectChanged(CVedMovie& aMovie) = 0;

    /**
     * Called to notify that a middle transition effect has changed 
     * (but no other changes have occurred).
     *
     * @param aMovie  movie
     * @param aIndex  index of the changed middle transition effect
     */
    virtual void NotifyMiddleTransitionEffectChanged(CVedMovie& aMovie, 
                                                     TInt aIndex) = 0;

    /**
     * Called to notify that the end transition effect of the movie
     * has changed (but no other changes have occurred).
     *
     * @param aMovie  movie
     */
    virtual void NotifyEndTransitionEffectChanged(CVedMovie& aMovie) = 0;

    /**
     * Called to notify that a new audio clip has been successfully
     * added to the movie. Note that the indices of the audio clips
     * starting after the new clip have also changed as a result.
     *
     * @param aMovie  movie
     * @param aClip   new audio clip
     */
    virtual void NotifyAudioClipAdded(CVedMovie& aMovie, TInt aIndex) = 0;

    /**
     * Called to notify that adding a new audio clip to the movie has failed.
     *
     * Possible error codes:
     *  - <code>KErrNotFound</code> if there is no file with the specified name
     *    in the specified directory (but the directory exists)
     *  - <code>KErrPathNotFound</code> if the specified directory
     *    does not exist
     *  - <code>KErrUnknown</code> if the specified file is of unknown format
     *
     * @param aMovie  movie
     * @param aError  one of the system wide error codes
     */
    virtual void NotifyAudioClipAddingFailed(CVedMovie& aMovie, TInt aError) = 0;

    /**
     * Called to notify that an audio clip has been removed from the movie.
     * Note that the indices of the audio clips starting after the removed
     * clip have also changed as a result.
     *
     * @param aMovie  movie
     * @param aIndex  index of the removed audio clip
     */
    virtual void NotifyAudioClipRemoved(CVedMovie& aMovie, TInt aIndex) = 0;

    /**
     * Called to notify that an audio clip has moved (that is, its
     * index has changed). This may happen when the start time of the audio 
     * clip is changed. Note that the indices of the clips between the old and 
     * new indices have also changed as a result.
     *
     * @param aMovie     movie
     * @param aOldIndex  old index of the moved clip
     * @param aNewIndex  new index of the moved clip
     */
    virtual void NotifyAudioClipIndicesChanged(CVedMovie& aMovie, TInt aOldIndex, 
                                               TInt aNewIndex) = 0;

    /**
     * Called to notify that the timings (for example, the start time or
     * the duration) of an audio clip have changed (but the index of the
     * clip has <em>not</em> changed as a result).
     *
     * @param aMovie  movie
     * @param aClip   changed audio clip
     */
    virtual void NotifyAudioClipTimingsChanged(CVedMovie& aMovie,
                                               TInt aIndex) = 0;

    /**
     * Called to notify that the quality setting of the movie has been
     * changed.
     *
     * @param aMovie  movie
     */
    virtual void NotifyMovieQualityChanged(CVedMovie& aMovie) = 0;

    /**
     * Called to notify that the movie has been reseted.
     *
     * @param aMovie  movie
     */
    virtual void NotifyMovieReseted(CVedMovie& aMovie) = 0;
    
    /**
     * Called to notify that the output parameters have been changed
     *
     * @param aMovie  movie
     */
    virtual void NotifyMovieOutputParametersChanged(CVedMovie& aMovie) = 0;
    
    /**
     * Called to notify that a dynamic level mark has been inserted 
     * to an audio clip.
     *
     * @param aMovie       movie
     * @param aClipIndex   audio clip index
     * @param aMarkIndex   index of the inserted level mark
     */
    virtual void NotifyAudioClipDynamicLevelMarkInserted(CVedMovie& aMovie, 
                                                         TInt aClipIndex, 
                                                         TInt aMarkIndex) = 0;

    /**
     * Called to notify that a dynamic level mark has been inserted 
     * to an audio clip.
     *
     * @param aMovie       movie
     * @param aClipIndex   audio clip index
     * @param aMarkIndex   index of the inserted level mark
     */
    virtual void NotifyAudioClipDynamicLevelMarkRemoved(CVedMovie& aMovie, 
                                                        TInt aClipIndex, 
                                                        TInt aMarkIndex) = 0;

    /**
     * Called to notify that a dynamic level mark has been inserted 
     * to an audio clip.
     *
     * @param aMovie       movie
     * @param aClipIndex   audio clip index
     * @param aMarkIndex   index of the inserted level mark
     */
    virtual void NotifyVideoClipDynamicLevelMarkInserted(CVedMovie& aMovie, 
                                                         TInt aClipIndex, 
                                                         TInt aMarkIndex) = 0;

    /**
     * Called to notify that a dynamic level mark has been inserted 
     * to an audio clip.
     *
     * @param aMovie       movie
     * @param aClipIndex   audio clip index
     * @param aMarkIndex   index of the inserted level mark
     */
    virtual void NotifyVideoClipDynamicLevelMarkRemoved(CVedMovie& aMovie, 
                                                        TInt aClipIndex, 
                                                        TInt aMarkIndex) = 0;    
    };


/**
 * Observer for movie processing operations. 
 *
 * 
 * @see  CVedMovie
 */
class MVedMovieProcessingObserver
    {
public:
    /**
     * Called to notify that a new movie processing operation has been started. 
     *
     * @param aMovie  movie
     */
    virtual void NotifyMovieProcessingStartedL(CVedMovie& aMovie) = 0;

    /**
     * Called to inform about the current progress of the movie processing operation.
     *
     * @param aMovie       movie
     * @param aPercentage  percentage of the operation completed, must be 
     *                     in range 0..100
     */
    virtual void NotifyMovieProcessingProgressed(CVedMovie& aMovie, TInt aPercentage) = 0;

    /**
     * Called to notify that the movie processing operation has been completed. 
     * 
     * @param aMovie  movie
     * @param aError  error code why the operation was completed. 
     *                <code>KErrNone</code> if the operation was completed 
     *                successfully.
     */
    virtual void NotifyMovieProcessingCompleted(CVedMovie& aMovie, TInt aError) = 0;
    };


/**
 * Video movie, which consists of zero or more video clips and zero or more audio clips.
 * 
 * @see  CVedVideoClip
 * @see  CVedAudioClip
 */
class CVedMovie : public CBase
    {
public:

    /**
     * Enumeration for movie quality settings.
     */
    enum TVedMovieQuality
        {
        EQualityAutomatic = 0,
        EQualityMMSInteroperability,
        EQualityResolutionCIF,      // Obsolete, please use Medium/High instead
        EQualityResolutionQCIF,     // Obsolete, please use Medium/High instead
        EQualityResolutionMedium,
        EQualityResolutionHigh,        
        EQualityLast  // this should always be the last
        };


public:

    /* Constructors & destructor. */

    /**
     * Constructs a new empty CVedMovie object. May leave if no resources are available.
     * 
     * Possible leave codes:
     *  - <code>KErrNoMemory</code> if memory allocation fails
     *
     * @param aFs  file server session to use to lock the video and audio
     *             clip files of the new movie; or NULL to not to lock the files
     *
     * @return  pointer to a new CVedMovie instance
     */
    IMPORT_C static CVedMovie* NewL(RFs* aFs);

    /**
     * Constructs a new empty CVedMovie object and leaves the object in the cleanup stack.
     * 
     * Possible leave codes:
     *  - <code>KErrNoMemory</code> if memory allocation fails
     * 
     * @param aFs  file server session to use to lock the video and audio
     *             clip files of the new movie; or NULL to not to lock the files
     *
     * @return  pointer to a new CVedMovie instance
     */
    IMPORT_C static CVedMovie* NewLC(RFs* aFs);

    /* Property methods. */

    /**
     * Returns the quality setting of this movie.
     *
     * @return  quality setting of this movie
     */
    virtual TVedMovieQuality Quality() const = 0;

    /**
     * Sets the quality setting of this movie.
     *
     * @param aQuality  quality setting
     */
    virtual void SetQuality(TVedMovieQuality aQuality) = 0;

    /**
     * Returns the video format of this movie. 
     * 
     * @return  video format of this movie
     */
    virtual TVedVideoFormat Format() const = 0;

    /**
     * Returns the video type of this movie. 
     * 
     * @return  video type of this movie
     */
    virtual TVedVideoType VideoType() const = 0;

    /**
     * Returns the resolution of this movie. Panics with 
     * <code>TVedPanic::EMovieEmpty</code> if there are no clips 
     * in the movie.
     * 
     * @return  resolution of the movie
     */
    virtual TSize Resolution() const = 0;

    /**
     * Returns the recommended maximum framerate of this movie..
     * <p>
     * Note that the returned maximum framerate is a recommendation,
     * not a guarantee. For example, the video clip generators inserted
     * in this movie should not generate frames at higher framerates 
     * than the recommendation. The movie may, however, exceed this
     * framerate (for example, if the framerates of some of the video
     * clips are higher than the recommendation and it is not possible
     * to drop the framerate).
     *
     * @return  maximum framerate in frames per second
     */
    virtual TInt MaximumFramerate() const = 0;

    /**
     * Returns the audio type of the movie audio track.
     * 
     * @return  audio type of the movie audio track
     */
    virtual TVedAudioType AudioType() const = 0;

    /**
     * Returns the audio sampling rate of the movie audio track.
     *
     * @return  audio sampling rate of the movie audio track.
     */
    virtual TInt AudioSamplingRate() const = 0;

    /**
     * Returns the audio channel mode of the movie audio track.
     * 
     * @return  audio channel mode of the movie audio track.
     */
    virtual TVedAudioChannelMode AudioChannelMode() const = 0;

    /**
     * Returns the total duration of this movie.
     * 
     * @return  duration in microseconds
     */
    virtual TTimeIntervalMicroSeconds Duration() const = 0;

    /**
     * Returns an estimate of the total size of this movie.
     * 
     * @return  size estimate in bytes
     */
    virtual TInt GetSizeEstimateL() const = 0;

    /**
     * Estimates end cutpoint with given target size and start cutpoint for current movie.
     *
     * @param aTargetSize  Target filesize for section indicated by aStartTime and aEndTime.
     * @param aStartTime   Start time for first frame included in cutted section. 
     * @param aEndTime     On return contains estimated end time for given target size and start cutpoint for current movie..
     */
    virtual void GetDurationEstimateL(TInt aTargetSize, TTimeIntervalMicroSeconds aStartTime, TTimeIntervalMicroSeconds& aEndTime) = 0;

    /**
     * Returns whether movie properties meet MMS compatibility
     * 
     * @return  ETrue if MMS compatible, else EFalse
     */
    virtual TBool IsMovieMMSCompatible() const = 0;

    /* Video clip management methods. */

    /**
     * Returns the number of video clips in this movie.
     *
     * @return  number of video clips
     */
    virtual TInt VideoClipCount() const = 0;

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
     * @param aFileName  file name of the clip to add
     * @param aIndex     index the clip should be inserted at
     */
    virtual void InsertVideoClipL(const TDesC& aFileName, TInt aIndex) = 0;

    /** 
     * Inserts a video clip generated by the specified generator to the 
     * specified index in this movie. The observers are notified when the clip 
     * has been added or adding clip has failed. Note that a video clip
     * generator can be inserted to a movie only once. Panics with
     * <code>EVideoClipGeneratorAlreadyInserted</code> if the generator has
     * already been inserted to a movie. Panics with 
     * <code>EMovieAddOperationAlreadyRunning</code> if another add video 
     * or audio clip operation is already running. Panics with code 
     * <code>USER-130</code> if the clip index is invalid.
     *
     * Possible leave codes:
     *  - <code>KErrNoMemory</code> if memory allocation fails
     *
     * @param aGenerator           generator to add
     * @param aIsOwnedByVideoClip  <code>ETrue</code>, if this movie is responsible
     *                             for deleting the generator when the clip
     *                             is removed from this movie; <code>EFalse</code>,
     *                             otherwise
     * @param aIndex               index the clip should be inserted at
     */
    virtual void InsertVideoClipL(CVedVideoClipGenerator& aGenerator, TBool aIsOwnedByVideoClip,
                                  TInt aIndex) = 0;

    /** 
     * Removes the video clip at the specified index from this movie.
     * Panics with code <code>USER-130</code> if the clip index is invalid.
     *
     * @param aIndex  index of the clip to be removed
     */
    virtual void RemoveVideoClip(TInt aIndex) = 0;


    /* Transition effect management methods. */

    /** 
     * Returns the start transition effect of this movie. Panics with 
     * <code>TVedPanic::EMovieEmpty</code> if there are no video clips 
     * in the movie.
     *
     * @return  start transition effect
     */ 
    virtual TVedStartTransitionEffect StartTransitionEffect() const = 0;

    /** 
     * Sets the start transition effect of this movie. Panics with 
     * <code>TVedPanic::EMovieEmpty</code> if there are no video clips 
     * in the movie.
     *
     * @param aEffect  start transition effect
     */ 
    virtual void SetStartTransitionEffect(TVedStartTransitionEffect aEffect) = 0;

    /**
     * Returns the number of middle transition effects in this movie.
     * Note that this is the same as the number of video clips minus one.
     *
     * @return  number of middle transition effects
     */
    virtual TInt MiddleTransitionEffectCount() const = 0;

    /** 
     * Returns the middle transition effect at the specified index. 
     * Panics with code <code>USER-130</code> if the index is invalid.
     *
     * @param aIndex  index
     *
     * @return  middle transition effect at the specified index
     */ 
    virtual TVedMiddleTransitionEffect MiddleTransitionEffect(TInt aIndex) const = 0;

    /** 
     * Sets the middle transition effect at the specified index. 
     * Panics with code <code>USER-130</code> if the index is invalid.
     *
     * @param aEffect  middle transition effect
     * @param aIndex   index
     */ 
    virtual void SetMiddleTransitionEffect(TVedMiddleTransitionEffect aEffect, TInt aIndex) = 0;

    /** 
     * Returns the end transition effect of this movie. Panics with 
     * <code>TVedPanic::EMovieEmpty</code> if there are no video clips 
     * in the movie.
     *
     * @return  end transition effect
     */ 
    virtual TVedEndTransitionEffect EndTransitionEffect() const = 0;

    /** 
     * Sets the end transition effect of this movie. Panics with 
     * <code>TVedPanic::EMovieEmpty</code> if there are no video clips 
     * in the movie.
     *
     * @param aEffect  end transition effect
     */ 
    virtual void SetEndTransitionEffect(TVedEndTransitionEffect aEffect) = 0;
    

    /* Audio clip management methods. */

    /**
     * Returns the number of audio clips in this movie.
     *
     * @return  number of audio clips
     */
    virtual TInt AudioClipCount() const = 0;

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
     * @param aFileName   file name of the clip to add
     * @param aStartTime  start time of the clip in movie timebase
     * @param aCutInTime  cut in time of the clip
     * @param aCutOutTime cut out time of the clip; or 
     *                    <code>KVedAudioClipOriginalDuration</code> to specify
     *                    that the original duration of the clip should be used
     */
    virtual void AddAudioClipL(const TDesC& aFileName,
            TTimeIntervalMicroSeconds aStartTime,
            TTimeIntervalMicroSeconds aCutInTime = TTimeIntervalMicroSeconds(0),
            TTimeIntervalMicroSeconds aCutOutTime = KVedAudioClipOriginalDuration) = 0;

    /** 
     * Removes the audio clip at the specified index from this movie.
     * Panics with code <code>USER-130</code> if the clip index is invalid.
     *
     * @param aIndex  index of the clip to be removed
     */
    virtual void RemoveAudioClip(TInt aIndex) = 0;
    
    
    /* Whole movie management methods. */
    
    /** 
     * Removes all video and audio clips and clears all transitions.
     */
    virtual void Reset() = 0;


    /* Processing methods. */

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
     * @param aFileName  name of the file to be written
     */
    virtual void ProcessL(const TDesC& aFileName,
                           MVedMovieProcessingObserver& aObserver) = 0;

    /**
     * Cancels the current video processing operation. If there is no 
     * operation in progress, the function does nothing.
     */
    virtual void CancelProcessing() = 0;


    /* Observer methods. */

    /**
     * Registers a movie observer. Panics with panic code 
     * <code>EMovieObserverAlreadyRegistered</code> if the movie observer is 
     * already registered.
     *
     * @param aObserver  observer that will receive the events
     */
    virtual void RegisterMovieObserverL(MVedMovieObserver* aObserver) = 0;

    /**
     * Unregisters a movie observer.
     *
     * @param aObserver  observer to be unregistered
     */
    virtual void UnregisterMovieObserver(MVedMovieObserver* aObserver) = 0;

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
    virtual CVedVideoClipInfo* VideoClipInfo(TInt aIndex) const = 0;

    /**
     * Returns whether this video clip with the specified editing operations 
     * applied (for example, changing speed or muting) has an audio track or not.
     *
     * @param aIndex  index of video clip in movie
     * @return  <code>ETrue</code> if clip has an audio track;
     *          <code>EFalse</code> otherwise
     */
    virtual TBool VideoClipEditedHasAudio(TInt aIndex) const = 0;

    /**
     * Sets the index of this video clip in the movie. 
     * Panics with code <code>EVideoClipIllegalIndex</code> 
     * if the clip index is invalid.
     *
     * @param aIndex  index of video clip in movie
     * @param aNewIndex  new index of this clip
     */
    virtual void VideoClipSetIndex(TInt aOldIndex, TInt aNewIndex) = 0;
 
    /**
     * Returns the playback speed of this video clip. Playback speed is
     * specified as parts per thousand of the normal playback speed. For example.
     * 1000 means the normal speed, 750 means 75% of the normal speed, and so on.
     *
     * @param aIndex  index of video clip in movie
     * @return  playback speed
     */
    virtual TInt VideoClipSpeed(TInt aIndex) const = 0;

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
    virtual void VideoClipSetSpeed(TInt aIndex, TInt aSpeed) = 0;
    
    /**
     * Returns the color effect of this video clip.
     *
     * @param aIndex  index of video clip in movie
     * @return  color effect
     */
    virtual TVedColorEffect VideoClipColorEffect(TInt aIndex) const = 0;

    /**
     * Sets the color effect of this video clip.
     *
     * @param aIndex  index of video clip in movie
     * @param aColorEffect  color effect
     */
    virtual void VideoClipSetColorEffect(TInt aIndex, TVedColorEffect aColorEffect) = 0;

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
    virtual TBool VideoClipIsMuteable(TInt aIndex) const = 0;

    /**
     * Returns whether the audio track of this video clip is muted or not.
     * Note that this returns ETrue only for cases where user has explicitly 
     * muted the audio or if there is no audio track even in the input; 
     * if the track is muted automatically, e.g. due to slow motion effect, 
     * but not explicitly by the user, the return value is EFalse.
     * If the user need to know for sure if there is audio track in the output, 
     * and this method returns EFalse, then the user should also compare 
     * the return value of VideoClipSpeed() to KVedNormalSpeed, 
     * and if they are not equal, assume the audio track is muted.
     *
     * @param aIndex  index of video clip in movie
     * @return  <code>ETrue</code> if the audio track is muted;
     *          <code>EFalse</code> otherwise
     */
    virtual TBool VideoClipIsMuted(TInt aIndex) const = 0;

    /**
     * Sets whether the audio track of this video clip is muted or not.
     *
     * @param aIndex  index of video clip in movie
     * @param aVolume  <code>ETrue</code> to mute the audio track;
     *                 <code>EFalse</code> not to mute the audio track
     */
    virtual void VideoClipSetMuted(TInt aIndex, TBool aMuted) = 0;

    /**
     * Returns the cut in time of this video clip in clip timebase.
     *
     * @param aIndex  index of video clip in movie
     * @return  cut in time in microseconds in clip timebase
     */
    virtual TTimeIntervalMicroSeconds VideoClipCutInTime(TInt aIndex) const = 0;

    /**
     * Sets the cut in time of this video clip in clip timebase.
     * Panics with <code>EVideoClipIllegalCutInTime</code> if
     * cut in time is illegal.
     *
     * @param aIndex  index of video clip in movie
     * @param aCutInTime  cut in time in microseconds in clip timebase
     */
    virtual void VideoClipSetCutInTime(TInt aIndex, TTimeIntervalMicroSeconds aCutInTime) = 0;

    /**
     * Returns the cut out time of this video clip in clip timebase.
     *
     * @param aIndex  index of video clip in movie
     * @return  cut out time in microseconds in clip timebase
     */
    virtual TTimeIntervalMicroSeconds VideoClipCutOutTime(TInt aIndex) const = 0;

    /**
     * Sets the cut out time of this video clip in clip timebase. 
     * Panics with <code>EVideoClipIllegalCutOutTime</code> if
     * cut out time is illegal.
     *
     * @param aIndex  index of video clip in movie
     * @param aCutOutTime  cut out time in microseconds in clip timebase
     */
    virtual void VideoClipSetCutOutTime(TInt aIndex, TTimeIntervalMicroSeconds aCutOutTime) = 0;

    /**
     * Returns the start time of this video clip in movie timebase.
     *
     * @param aIndex  index of video clip in movie
     * @return  start time in microseconds in movie timebase
     */
    virtual TTimeIntervalMicroSeconds VideoClipStartTime(TInt aIndex) const = 0;

    /**
     * Returns the end time of this video clip in movie timebase.
     *
     * @param aIndex  index of video clip in movie
     * @return  end time in microseconds in movie timebase
     */
    virtual TTimeIntervalMicroSeconds VideoClipEndTime(TInt aIndex) const = 0;

    /**
     * Returns the duration of this video clip with the specified
     * editing operations applied (for example, cutting 
     * and changing speed)
     * 
     * @param aIndex  index of video clip in movie
     * @return  duration in microseconds
     */
    virtual TTimeIntervalMicroSeconds VideoClipEditedDuration(TInt aIndex) const = 0;

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
    virtual CVedAudioClipInfo* AudioClipInfo(TInt aIndex) const = 0;

    /**
     * Returns the start time of this audio clip in movie timebase.
     *
     * @param aIndex  index of audio clip in movie
     * @return  start time in microseconds in movie timebase
     */
    virtual TTimeIntervalMicroSeconds AudioClipStartTime(TInt aIndex) const = 0;

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
    virtual void AudioClipSetStartTime(TInt aIndex, TTimeIntervalMicroSeconds aStartTime) = 0;

    /**
     * Returns the end time of this audio clip in movie timebase.
     *
     * @param aIndex  index of audio clip in movie
     * @return  end time in microseconds in movie timebase
     */
    virtual TTimeIntervalMicroSeconds AudioClipEndTime(TInt aIndex) const = 0;

    /**
     * Returns the duration of the selected part of this clip.
     * 
     * @param aIndex  index of audio clip in movie
     * @return  duration in microseconds
     */
    virtual TTimeIntervalMicroSeconds AudioClipEditedDuration(TInt aIndex) const = 0;

    /**
     * Returns the cut in time of this audio clip in clip timebase.
     *
     * @param aIndex  index of audio clip in movie
     * @return  cut in time in microseconds in clip timebase
     */
    virtual TTimeIntervalMicroSeconds AudioClipCutInTime(TInt aIndex) const = 0;

    /**
     * Sets the cut in time of this audio clip in clip timebase.
     * Panics with <code>EAudioClipIllegalCutInTime</code> if
     * cut in time is illegal.
     *
     * @param aIndex  index of audio clip in movie
     * @param aCutInTime  cut in time in microseconds in clip timebase
     */
    virtual void AudioClipSetCutInTime(TInt aIndex, TTimeIntervalMicroSeconds aCutInTime) = 0;

    /**
     * Returns the cut out time of this audio clip in clip timebase.
     *
     * @param aIndex  index of audio clip in movie
     * @return  cut out time in microseconds in clip timebase
     */
    virtual TTimeIntervalMicroSeconds AudioClipCutOutTime(TInt aIndex) const = 0;

    /**
     * Sets the cut out time of this audio clip in clip timebase.
     * Panics with <code>EAudioClipIllegalCutOutTime</code> if
     * cut in time is illegal.
     *
     * @param aIndex  index of audio clip in movie
     * @param aCutOutTime  cut out time in microseconds in clip timebase
     */
    virtual void AudioClipSetCutOutTime(TInt aIndex, TTimeIntervalMicroSeconds aCutOutTime) = 0;

    /**
     * Returns the target bitrate of the movie audio track.
     * 
     * @return  target bitrate of the movie audio track.
     */
    virtual TInt AudioBitrate() const = 0;

    /**
     * Returns the target bitrate of the movie video track.
     * 
     * @return  target bitrate of the movie video track.
     */
    virtual TInt VideoBitrate() const = 0;

    /**
     * Returns the target framerate of the movie video track.
     * 
     * @return  target framerate of the movie video track.
     */
    virtual TReal VideoFrameRate() const = 0;

    /**
     * Sets the output parameters for the movie. Leaves
     * with KErrNotSupported if a parameter is illegal,
     * e.g., target bitrate is too high for the given 
     * codec. Setting a integer parameter to zero indicates
     * that a default value will be used for that parameter.
     *
     * This method overrides the SetQuality method
     *
     * Possible leave codes:
     *  - <code>KErrNotSupported</code> if setting is not valid
     *
     * @param Output parameters
     */
    
    virtual void SetOutputParametersL(TVedOutputParameters& aOutputParams) = 0;
    
    /**
     * Sets the maximum size for the movie
     * 
     * @param aLimit Maximum size in bytes
     */
    virtual void SetMovieSizeLimit(TInt aLimit) = 0;
    
     /**
     * Returns whether this audio clip is normalized or not.
     *
     * @param aIndex  index of audio clip in movie
     * @return  <code>ETrue</code> if the audio clip is normalized;
     *          <code>EFalse</code> otherwise
     */
    virtual TBool AudioClipNormalizing(TInt aIndex) const = 0;

    /**
     * Sets whether this audio clip is normalized or not.
     *
     * @param aIndex  index of audio clip in movie
     * @param aVolume  <code>ETrue</code> to normalize the audio clip;
     *                 <code>EFalse</code> not to normalize the audio clip
     */
    virtual void AudioClipSetNormalizing(TInt aIndex, TBool aNormalizing) = 0;

    /**
     * Inserts a new dynamic level mark to the audio clip. The mark timing
     * must be within the time boundaries of the audio clip.
     *
     * Note! This method should not be used at the same time with SetAudioClipVolumeGainL
     * since these overrule each other; the latter one used stays valid.
     *
     * @param aIndex  index of the audio clip
     * @param aMark   dynamic level mark to be inserted
     */
    virtual void AudioClipInsertDynamicLevelMarkL(TInt aIndex, TVedDynamicLevelMark aMark) = 0;
    
    /**
     * Removes the specified dynamic level mark from the specified audio clip.
     * The mark index must be between 0 and number of dynamic level marks in the clip.
     *
     * @param aClipIndex  index of the audio clip
     * @param aMarkIndex  index of the mark to be removed
     */
    virtual void AudioClipRemoveDynamicLevelMark(TInt aClipIndex, TInt aMarkIndex) = 0;

    /**
     * Returns the number of dynamic level marks in the specified audio clip.
     * 
     * @param aIndex  index of the audio clip
     */
    virtual TInt AudioClipDynamicLevelMarkCount(TInt aIndex) const = 0;

    /**
     * Returns the specified dynamic level mark from the specified audio clip.
     * 
     * @param aClipIndex  index of the audio clip
     * @param aMarkIndex  index of the dynamic level mark
     */    
    virtual TVedDynamicLevelMark AudioClipDynamicLevelMark(TInt aClipIndex, TInt aMarkIndex) = 0;

    /**
     * Returns whether the audio track of this video clip is normalized or not.
     *
     * @param aIndex  index of video clip in movie
     * @return  <code>ETrue</code> if the audio track is normalized;
     *          <code>EFalse</code> otherwise
     */
    virtual TBool VideoClipNormalizing(TInt aIndex) const = 0;

    /**
     * Sets whether the audio track of this video clip is normalized or not.
     *
     * @param aIndex  index of video clip in movie
     * @param aVolume  <code>ETrue</code> to normalize the audio track;
     *                 <code>EFalse</code> not to normalize the audio track
     */
    virtual void VideoClipSetNormalizing(TInt aIndex, TBool aNormalizing) = 0;

    /**
     * Inserts a new dynamic level mark to the video clip. The mark timing
     * must be within the time boundaries of the video clip.
     *
     * Note! This method should not be used at the same time with SetVideoClipVolumeGainL
     * since these overrule each other; the latter one used stays valid.
     *
     * @param aIndex  index of the video clip
     * @param aMark   dynamic level mark to be inserted
     */
    virtual void VideoClipInsertDynamicLevelMarkL(TInt aIndex, TVedDynamicLevelMark aMark) = 0;
    
    /**
     * Removes the specified dynamic level mark from the specified video clip.
     * The mark index must be between 0 and number of dynamic level marks in the clip.
     *
     * @param aClipIndex  index of the video clip
     * @param aMarkIndex  index of the mark to be removed
     */
    virtual void VideoClipRemoveDynamicLevelMark(TInt aClipIndex, TInt aMarkIndex) = 0;

    /**
     * Returns the number of dynamic level marks in the specified video clip.
     * 
     * @param aIndex  index of the video clip
     */
    virtual TInt VideoClipDynamicLevelMarkCount(TInt aIndex) const = 0;

    /**
     * Returns the specified dynamic level mark from the specified video clip.
     * 
     * @param aClipIndex  index of the video clip
     * @param aMarkIndex  index of the dynamic level mark
     */    
    virtual TVedDynamicLevelMark VideoClipDynamicLevelMark(TInt aClipIndex, TInt aMarkIndex) = 0;    
    
     /**
     * Returns the color tone of the specified clip.
     *
     * @return color tone
     */
    virtual TRgb VideoClipColorTone(TInt aVideoCLipIndex) const = 0;
    
    /**
     * Sets the color tone of the specified clip.
     *
     * @param aColorTone  color tone
     */
    virtual void VideoClipSetColorTone(TInt aVideoClipIndex, TRgb aColorTone) = 0;
    
     /**
     * Returns an estimate for movie processing time
     *
     * @return Processing time
     */
    virtual TTimeIntervalMicroSeconds GetProcessingTimeEstimateL() = 0;

    /**
     * Checks if a movie observer is registered.
     *
     * @param aObserver observer to be checked
     * @return <code>ETrue</code> if the observer is registered
     *         <code>EFalse</code> otherwise
     */
    virtual TBool MovieObserverIsRegistered(MVedMovieObserver* aObserver) = 0;

    /**
     * Set volume gain for audio track in the given video clip. Value 0 means no gain.
     * In practice calls VideoClipInsertDynamicLevelMarkL to set dynamic level mark
     * to the beginning and end of the clip. 
     * Also the observer callback NotifyVideoClipDynamicLevelMarkInserted is called if gain
     * is nonzero; if it is zero, callback NotifyVideoClipDynamicLevelMarkRemoved is called.
     *
     * If index is KVedClipIndexAll, the setting is applied to all video clips in the movie.
     *
     * Note! This method should not be used at the same time with VideoClipInsertDynamicLevelMarkL
     * since these overrule each other; the latter one used stays valid.
     *
     * @param aClipIndex    index of the clip; KVedClipIndexAll if applied for all the clips
     * @param aVolumeGain   Volume gain. One step equals 0.1 dedibels for positive values and
     *                      0.5 decibels for negative values.
     *                      Value = 0 sets the original level (no gain)
     *                      Value range -127...127; if exceeded, the value is saturated to max
     */
    virtual void SetVideoClipVolumeGainL(TInt aClipIndex, TInt aVolumeGain) = 0;

    /**
     * Get volume gain for audio track in the given video clip.
     * If index is KVedClipIndexAll, the global gain set for all video clips in the movie is returned.
     *
     * @param aClipIndex    index of the clip; KVedClipIndexAll if applied for all the clips (whole movie)
     * @return Volume gain in +0.1 or -0.5 decibel steps
     */
    virtual TInt GetVideoClipVolumeGainL(TInt aClipIndex) = 0;

    /**
     * Set volume gain for the given audio clip. Value 0 means no gain.
     * In practice calls AudioClipInsertDynamicLevelMarkL to set dynamic level mark
     * to the beginning and end of the clip. 
     * Also the observer callback NotifyAudioClipDynamicLevelMarkInserted is called if gain
     * is nonzero; if it is zero, callback NotifyAudioClipDynamicLevelMarkRemoved is called.
     *
     * If index is KVedClipIndexAll, the setting is applied to all audio clips in the movie.
     *
     * Note! This method should not be used at the same time with AudioClipInsertDynamicLevelMarkL
     * since these overrule each other; the latter one used stays valid.
     *
     * @param aClipIndex    Index of the clip; KVedClipIndexAll if applied for all the clips
     * @param aVolumeGain   Volume gain. One step equals 0.1 dedibels for positive values and
     *                      0.5 decibels for negative values. 
     *                      Value = 0 sets the original level (no gain)
     *                      Value range -127...127; if exceeded, the value is saturated to max
     */
    virtual void SetAudioClipVolumeGainL(TInt aClipIndex, TInt aVolumeGain) = 0;

    /**
     * Get volume gain for the given audio clip.
     * If index is KVedClipIndexAll, the global gain set for all video clips in the movie is returned.
     *
     * @param aClipIndex    index of the clip; KVedClipIndexAll if applied for all the clips
     * @return Volume gain in +0.1 or -0.5 decibel steps
     */
    virtual TInt GetAudioClipVolumeGainL(TInt aClipIndex) = 0;
    
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
    virtual void InsertVideoClipL(RFile* aFileHandle, TInt aIndex) = 0;
    
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
    
    virtual void AddAudioClipL(RFile* aFileHandle,
            TTimeIntervalMicroSeconds aStartTime,
            TTimeIntervalMicroSeconds aCutInTime = TTimeIntervalMicroSeconds(0),
            TTimeIntervalMicroSeconds aCutOutTime = KVedAudioClipOriginalDuration) = 0;
    
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
    
    virtual void ProcessL(RFile* aFileHandle,
                          MVedMovieProcessingObserver& aObserver) = 0;
    

    };

#endif // __VEDMOVIE_H__

