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




#ifndef __AUDOBSERVERS_H__
#define __AUDOBSERVERS_H__


#include <e32base.h>
//#include "AudSong.h"
//#include "AudClip.h"
//#include "AudClipInfo.h"
#include "AudCommon.h"

class CAudSong;
class CAudClip;
class CAudClipInfo;

/**
 * Observer for movie processing operations. 
 */
class MAudSongProcessingObserver
    {
public:
    /**
     * Called to notify that a new audio processing operation has been started. 
     *
     * @param aSong  song
     */
    virtual void NotifyAudioProcessingStartedL(CAudSong& aSong) = 0;

    /**
     * Called to inform about the current progress of the audio processing operation.
     *
     * @param aSong       song
     * @param aPercentage  percentage of the operation completed, must be 
       *                     in range 0..100
     */
    virtual void NotifyAudioProcessingProgressed(CAudSong& aSong, TInt aPercentage) = 0;

    /**
    * Called to notify that the song processing operation has been completed. 
    * 
    * @param aSong  song
    * @param aError  error code why the operation was completed. 
    *                <code>KErrNone</code> if the operation was completed 
    *                successfully.
    */
    virtual void NotifyAudioProcessingCompleted(CAudSong& aSong, TInt aError) = 0;
    
    };

/**
* Observer for audio visualization processes. 
*/
class MAudVisualizationObserver
    {
public:
        
    /**
    * Called to notify that audio _clip_ visualization process has been completed. 
    * The receiver is responsible for releasing the memory in 
    * <code>aVisualization</code> if <code>aError = KErrNone</code>
    * Note: if aError != KErrNone, aVisualization = NULL and aSize = 0;
    * 
    * @param aClipInfo        audio clip info
    * @param aError            <code>KErrNone</code> if visualization was
    *                        completed successfully; one of the system wide
    *                        error codes if generating visualization failed
    * @param aVisualization    pointer to TInt8 visualization array 
    * @param aSize                size of the visualization array
    */
    virtual void NotifyClipInfoVisualizationCompleted(const CAudClipInfo& aClipInfo, 
        TInt aError, 
        TInt8* aVisualization,
        TInt aSize) = 0;
    

    /**
    * Called to notify that audio _clip_ visualization process has been started. 
    * 
    * @param aClipInfo            clip info
    * @param aError                <code>KErrNone</code> if visualization was
    *                            started successfully; one of the system wide
    *                            error codes if generating visualization failed
    */
    virtual void NotifyClipInfoVisualizationStarted(const CAudClipInfo& aClipInfo, 
        TInt aError) = 0;


    /**
     * Called to inform about the current progress of the audio clip visualization.
     *
     * @param aClipInfo    clip info
     * @param aPercentage  percentage of the operation completed, must be 
       *                     in range 0..100
     */
    virtual void NotifyClipInfoVisualizationProgressed(const CAudClipInfo& aClipInfo, 
        TInt aPercentage) = 0;


    };


/**
 * Observer for song events. 
 * <p>
 * Note that every change operation that is made to a song or the clips it consists of 
 * results in a maximum of one notification method called (that is, more than one 
 * notification method is never called as a result of a single change). For example,
 * changing the index of a clip results in the <code>NotifyClipIndicesChanged()</code>
 * method being called once. The <code>NotifyClipTimingsChanged()</code> method is not 
 * called even if the timings of several clips may have changed as a result. See the
 * descriptions of the notification methods for more detailed information.
 *
 * @see  CAudSong
 */
class MAudSongObserver 
    {
public:

    /**
     * Called to notify that a new audio clip has been successfully
     * added to the song. Note that the indices and the start and end times
     * of the audio clips after the new clip have also changed as a result.
     * Note that the transitions may also have changed. 
     *
     * @param aSong            song
     * @param aClip            new audio clip
     * @param aIndex        index of the new audio clip on a track
     * @param aTrackIndex    track index of the new clip
     *
     */
    virtual void NotifyClipAdded(CAudSong& aSong, CAudClip& aClip, TInt aIndex, TInt aTrackIndex) = 0;

    /**
     * Called to notify that adding a new audio clip to the song has failed.
     *
     * Possible error codes:
     *    - <code>KErrNotFound</code> if there is no file with the specified name
     *    in the specified directory (but the directory exists)
     *    - <code>KErrPathNotFound</code> if the specified directory
     *    does not exist
     *    - <code>KErrUnknown</code> if the specified file is of unknown format
     *    - <code>KErrNotSupported</code> if the format of the file is recognized but
     *    adding it to the song is not supported (e.g., it is of different codec
     *    or format than the other clips)
     *
     * @param aSong   song
     * @param aError  one of the system wide error codes
     */
    virtual void NotifyClipAddingFailed(CAudSong& aSong, TInt aError,  TInt aTrackIndex) = 0;

    /**
     * Called to notify that an audio clip has been removed from the song.
     * Note that the indices and the start and end times of the audio clips after 
     * the removed clip have also changed as a result. Note that the 
     * transitions may also have changed.
     *
     * @param aSong            song
     * @param aIndex        index of the removed clip (on a track)
     * @param aTrackIndex    track index
     */
    virtual void NotifyClipRemoved(CAudSong& aSong, TInt aIndex, TInt aTrackIndex) = 0;
    
    /**
     * Called to notify that the timings of the clip has changed.
     * Note that the start and end times of the audio clips 
     * after the changed clip have also changed.
     *
     * @param aSong  song
     * @param aClip  changed clip
     */
    virtual void NotifyClipTimingsChanged(CAudSong& aSong,
        CAudClip& aClip) = 0;
    

    /**
     * Called to notify that the index of the clip has changed.
     *
     * @param aSong            song
     * @param aOldIndex        old index on a track
     * @param aNewIndex        new index on a track
     * @param aTrackIndex    track index
     */
    virtual void NotifyClipIndicesChanged(CAudSong& aSong, TInt aOldIndex, 
        TInt aNewIndex, TInt aTrackIndex) = 0;
    
    /**
     * Called to notify that the song has been reseted.
     *
     * @param aSong  song
     */
    virtual void NotifySongReseted(CAudSong& aSong) = 0;


    /**
     * Called to notify that a clip has been reseted.
     *
     * @param aClip  clip
     */
    virtual void NotifyClipReseted(CAudClip& aClip) = 0;

    
    /**
     * Called to notify that a new dynamic level mark has been successfully
     * added to a _clip_.
     *
     * @param aClip  clip
     * @param aMark  new level mark
     * @param aIndex index of the new level mark
     */
    virtual void NotifyDynamicLevelMarkInserted(CAudClip& aClip, 
        TAudDynamicLevelMark& aMark, 
        TInt aIndex) = 0;

    
    /**
     * Called to notify that a dynamic level mark has been removed from a _clip_.
     * Note that indices of dynamic level marks has also changed as a result
     *
     * @param aClip   clip
     * @param aIndex  index of the removed mark
     */
    virtual void NotifyDynamicLevelMarkRemoved(CAudClip& aClip, TInt aIndex) = 0;



    };




/**
 * Observer for notifying that audio clip info
 * is ready for reading.
 *
 * @see  CAudClipInfo
 */
class MAudClipInfoObserver
    {
public:
    /**
     * Called to notify that audio clip info is ready
     * for reading.
     *
     * Possible error codes:
     *    - <code>KErrNotFound</code> if there is no file with the specified name
     *    in the specified directory (but the directory exists)
     *    - <code>KErrPathNotFound</code> if the specified directory
     *    does not exist
     *    - <code>KErrUnknown</code> if the specified file is of unknown type
     *
     * @param aInfo   audio clip info
     * @param aError  <code>KErrNone</code> if info is ready
     *                for reading; one of the system wide
     *                error codes if reading file failed
     */
    virtual void NotifyClipInfoReady(CAudClipInfo& aInfo, 
                                          TInt aError) = 0;
    };
    
class MAudTimeEstimateObserver
    {
public:
    /**
     * Called to notify that time estimate is ready
     * for reading.
     *
     *
     * @param aTimeEstimate   time estimate in microseconds
     */
    
    virtual void NotifyTimeEstimateReady(TInt64 aTimeEstimate) = 0;

    };
#endif
