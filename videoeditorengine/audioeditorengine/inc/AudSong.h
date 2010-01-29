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




#ifndef __AUDSONG_H__
#define __AUDSONG_H__


#include <e32base.h>
#include <f32file.h>
#include <e32std.h>

#include "AudCommon.h"
#include "AudObservers.h"


// define _WRITE_OUTPUT_TO_FILE_ if you want audio engine to write its output to a file
//#define _WRITE_OUTPUT_TO_FILE_;


/*
*    Forward declarations.
*/


class MAudVisualizationObserver;
class MAudSongProcessingObserver;
class CAudSongProcessOperation;
class CAudSongVisualizationOperation;
class MAudSongObserver;
class CAudClipInfo;
class MAudClipInfoObserver;
class CAudSongAddClipOperation;
class CAudProcessor;

// constants that represent special parameter values

// clip end time (used when setting cutOut-times)
const TInt KClipEndTime = -1;

// KAllTrackIndices represents all tracks in ClipCount()-function
const TInt KAllTrackIndices = -1;

// Let audio engine to decide the bitrate
const TInt KAudBitRateDefault = -1;


/**
* Audio song, which consists of zero or more audio clips
*
* @see  CAudClip
*/
class CAudSong : public CBase
    {
public:
    /* Constructors. */
    
    /**
    * Constructs a new empty CAudSong object. May leave if no resources are available.
    * 
    * Possible leave codes:
    *    - <code>KErrNoMemory</code> if memory allocation fails
    *
    * @param aFs  file server session to use to lock the audio
    *             clip files of the new song; or NULL to not to lock the files
    * @return  pointer to a new CAudSong instance
    */
    IMPORT_C static CAudSong* NewL(RFs *aFs);
    
    /**
    * Constructs a new empty CAudSong object and leaves the object in the cleanup stack.
    * 
    * Possible leave codes:
    *    - <code>KErrNoMemory</code> if memory allocation fails
    * 
    * @param aFs  file server session to use to lock the audio
    *             clip files of the new song; or NULL to not to lock the files
    * @return  pointer to a new CAudSong instance
    */
    IMPORT_C static CAudSong* NewLC(RFs *aFs);
    
    /**
    * Destroys the object and releases all resources.
    */
    IMPORT_C virtual ~CAudSong();
    
    
    /* Property methods. */
    
    /**
    * Returns an estimate of the total size of this song.
    * 
    * @return  size estimate in bytes
    */
    IMPORT_C TInt GetSizeEstimateL() const;
    
    /**
    * Returns an estimate of the size of this song between the given time interval.
    * 
    * @param aStartTime Beginning of the time interval
    * @param aEndTime End of the time interval
    * @return size estimate in bytes
    */
    IMPORT_C TInt GetFrameSizeEstimateL(TTimeIntervalMicroSeconds aStartTime, 
                                        TTimeIntervalMicroSeconds aEndTime) const;
    
    /**
    * Returns the properties of this song.
    * 
    * @return  output file properties
    */
    IMPORT_C TAudFileProperties OutputFileProperties() const;
    
    /**
    *
    * Generates decoder specific info needed for MP4
    * Caller is responsible for releasing aDecSpecInfo in the case ETrue is returned
    *
    * @param aMaxSize    maximum aDecSpecInfo size allowed    
    *
    */
    IMPORT_C TBool GetMP4DecoderSpecificInfoLC(HBufC8*& aDecSpecInfo, TInt aMaxSize) const;

    /**
    * Get processing time estimate based on trial. Asynchronous version, requires observer.
    *
    */
    IMPORT_C TBool GetTimeEstimateL(MAudTimeEstimateObserver& aObserver,
                                    TAudType aAudType,
                                    TInt aSamplingRate,
                                    TChannelMode aChannelMode,
                                    TInt aBitRate = -1);
                                    
    /**
    * Get processing time estimate based on hardcoded constants. Synchronous version.
    *
    * @return   estimated processing time for the song
    */
    IMPORT_C TTimeIntervalMicroSeconds GetTimeEstimateL();
    
    /**
    * Get the frame duration in microseconds. 
    *
    * @return   frame duration in microseconds
    */
    IMPORT_C TInt GetFrameDurationMicro();


    /* Audio clip management methods. */
    
    /**
    * Returns the number of audio clips in this song on a certain track.
    *
    * @param   aTrackIndex track index
    *
    * @return  number of audio clips on a defined track
    */
    IMPORT_C TInt ClipCount(TInt aTrackIndex = 0) const;
    
    /** 
    * Returns the audio clip at the specified index. 
    * Panics with code <code>EAudioClipIllegalIndex</code> if the clip index is invalid.
    *
    * @param aIndex            index of the clip on the certain track
    * @param aTrackIndex    index of the track
    *
    * @return  clip at the specified index.
    */    
    IMPORT_C CAudClip* Clip(TInt aIndex, TInt aTrackIndex = 0) const;
    
    /** 
    * Adds the specified audio clip to this song. Asynchronous operation
    * CAudSongObserver::NotifyClipAdded or CAudSongObserver::NotifyClipAddingFailed
    * is called once the operation has completed.
    *
    * Possible leave codes:
    *    - <code>KErrNoMemory</code> if memory allocation fails
    *    - <code>KErrNotSupported</code> if audio format is not supported
    *
    * @param aFileName   file name of the clip to add
    * @param aStartTime  start time of the clip in song timebase
    * @param aTrackIndex track index. Used to categorize clips.
    */
    IMPORT_C void AddClipL(const TDesC& aFileName,
        TTimeIntervalMicroSeconds aStartTime, TInt aTrackIndex = 0,
        TTimeIntervalMicroSeconds aCutInTime = TTimeIntervalMicroSeconds(0),
        TTimeIntervalMicroSeconds aCutOutTime = TTimeIntervalMicroSeconds(KClipEndTime));
    
    
    /** 
    * Removes the audio clip at the specified index from this song.
    * Panics with code <code>USER-130</code> if the clip index is invalid.
    *
    * @param aIndex  index of the clip to be removed
    * @param aTrackIndex track index
    */
    IMPORT_C void RemoveClip(TInt aIndex, TInt aTrackIndex = 0);
    
    /** 
    * Removes all audio clips and clears all effects
    *
    * @param aNotify ETrue if observer should be notified
    */
    IMPORT_C void Reset(TBool aNotify);

    /**
    *
    * Sets the duration of the output clip
    * If necessary, silence is generated in the end
    *
    * @param    aDuration    output clip length in microseconds
    * 
    * @return    ETrue if a new duration was set, EFalse otherwise
    *
    */
    IMPORT_C TBool SetDuration(TTimeIntervalMicroSeconds aDuration);

    
    /* Processing methods. */
    
    
    /**
    * Removes a dynamic level mark in a song. 
    * Panics with code <code>EAudioClipIllegalIndex</code> 
    * if the clip index is invalid.
    *
    * @param    aIndex index of the removed mark in this song
    * @return  ETrue if mark was removed, EFalse otherwise
    */
    IMPORT_C TBool RemoveDynamicLevelMark(TInt aIndex);
    
    
    /**
    * Sets an output file format
    *
    * NOTE: only the following combinations are allowed: 
    * AMR: single channel, 8000 Hz, 12000 kbps
    *
    * AAC/16kHz mono/stereo 
    * AAC/48kHz mono/stereo
    *
    * @return   ETrue if format was successfully changed, 
    *             EFalse if format cannot be set
    */
    IMPORT_C TBool SetOutputFileFormat(TAudType aAudType,
                                        TInt aSamplingRate,
                                        TChannelMode aChannelMode,
                                        TInt aBitRate = KAudBitRateDefault);
    
    /**
    * Checks if given properties are supported
    *
    *
    * @param    aProperties audio properties
    * @return   ETrue if properties are supported
    *           EFalse if not
    */
    IMPORT_C TBool AreOutputPropertiesSupported(const TAudFileProperties& aProperties );
    
    
    /*
    * Processing methods
    */

    /**
    * Starts a synchronous audio processing operation. After calling this function
    * the current song can be processed frame by frame by calling ProcessPieceL-function
    * Panics with 
    * <code>TAudPanic::ESongEmpty</code> if there are no clips 
    * in the song.
    *     
    */
    IMPORT_C TBool SyncStartProcessingL();
    
    /**
    * Processes synchronously next audio frame and passes it to the caller. 
    * If a processing operation has not been started, panic 
    * <code>TAudPanic::ESongProcessingNotRunning</code> is raised.
    *
    * Possibly leave codes:
    *    - <code>KErrNoMemory</code> if memory allocation fails
    *
    * NOTE: This function allocates memory and the caller is responsible for
    * releasing it.
    *
    * @param aFrame        pointer to a processed audio frame. Has to be deleted
    *                    when not needed anymore.
    * @param aDuration    Duration of processed audio frame in microseconds 
    *                    after decoding
    * 
    * @return ETrue        Processing operation completed, no more frames to be processed. 
    *                    No memory allocated.
    *          EFalse    Processing operation not completed. Last processed frame stored
    *                    in <code>aFrame</code>. Memory allocated for the processed frame
    *                      
    */
    IMPORT_C TBool SyncProcessFrameL(HBufC8*& aFrame, TInt& aProgress, 
                                TTimeIntervalMicroSeconds& aDuration);


    /**
    * Cancels the processing operation. Used for only synchronous processing 
    * operations. If an operation is not running, 
    * panic <code>TAudPanic::ESongProcessingNotRunning</code> is raised
    */
    IMPORT_C void SyncCancelProcess();


    /* Observer methods. */

    /**
     * Registers a song observer. Panics with panic code 
     * <code>ESongObserverAlreadyRegistered</code> if the song observer is 
     * already registered.
     *
     * @param aObserver  observer that will receive the events
     */
    IMPORT_C void RegisterSongObserverL(MAudSongObserver* aObserver);

    /**
     * Unregisters a song observer. Panics with panic code 
     * <code>ESongObserverNotRegistered</code> if the song observer is not registered.
     *
     * @param aObserver  observer to be unregistered
     */
    IMPORT_C void UnregisterSongObserver(MAudSongObserver* aObserver);
    
    
    /** 
    * Adds the specified audio clip to this song. Asynchronous operation
    * CAudSongObserver::NotifyClipAdded or CAudSongObserver::NotifyClipAddingFailed
    * is called once the operation has completed.
    *
    * Possible leave codes:
    *    - <code>KErrNoMemory</code> if memory allocation fails
    *    - <code>KErrNotSupported</code> if audio format is not supported
    *
    * @param aFileHandle file handle of the clip to add
    * @param aStartTime  start time of the clip in song timebase
    * @param aTrackIndex track index. Used to categorize clips.
    */
    IMPORT_C void AddClipL(RFile* aFileHandle,
        TTimeIntervalMicroSeconds aStartTime, TInt aTrackIndex = 0,
        TTimeIntervalMicroSeconds aCutInTime = TTimeIntervalMicroSeconds(0),
        TTimeIntervalMicroSeconds aCutOutTime = TTimeIntervalMicroSeconds(KClipEndTime));


private:


    // C++ constructor
    CAudSong(RFs *aFs);

    // functions for keeping clip indexes and arrays up to date
    void UpdateClipIndexes();
    void UpdateClipArray();

    /**
    * Returns a track domain clip index
    * of the clip whose song doman index is <code>aIndex</code>
    */
    TInt Index2IndexOnTrack(TInt aIndex);

    /*
    * Returns a song domain clip index
    */
    TInt FindClipIndexOnSong(const CAudClip* aClip) const;
    
    /*
    * ConstructL
    */
    
    void ConstructL();

private:
    
    // Member variables
    
    // File server session.
    RFs* iFs;
    // Audio clip array.
    RPointerArray<CAudClip> iClipArray;
    
    // Marks used for manual level controlling
    RPointerArray<TAudDynamicLevelMark> iDynamicLevelMarkArray;
    
    // Properties of the output file.
    TAudFileProperties* iProperties;
    
    // Duration of the song
    // if iSongDuration is greater than the cut out time of the last output clip,
    // silence is generated in the end
    TTimeIntervalMicroSeconds iSongDuration;
    
    // a flag to indicate whether the user has set the duration manually
    // if not, the duration is the cutOutTime of the last input clip
    TBool iSongDurationManuallySet;

    // Observer array of the song class.
    RPointerArray<MAudSongObserver> iObserverArray;
    
    // Normalizing of this audio clip.
    // If ETrue, the song as a whole is to be normalized
    TBool iNormalize;
    
    // process operation owned by this
    CAudSongProcessOperation* iProcessOperation;
    
    // clip adding operation owned by this
    CAudSongAddClipOperation* iAddOperation;
    
    // song visualization operation owned by this
    CAudSongVisualizationOperation* iVisualizationOperation;
    
    
private:

    // Notifications to fire callbacks to all registered listeners -------->
    
    void FireClipAdded(CAudSong* aSong, CAudClip* aClip, TInt aIndex, TInt aTrackIndex);
    
    void FireClipAddingFailed(CAudSong* aSong, TInt aError, TInt aTrackIndex);

    void FireClipRemoved(CAudSong* aSong, TInt aIndex, TInt aTrackIndex);

    void FireClipIndicesChanged(CAudSong* aSong, TInt aOldIndex, 
                                     TInt aNewIndex, TInt aTrackIndex);

    void FireClipTimingsChanged(CAudSong* aSong, CAudClip* aClip);

    void FireDynamicLevelMarkInserted(CAudSong& aSong, 
        TAudDynamicLevelMark& aMark, 
        TInt aIndex);
    
    void FireDynamicLevelMarkInserted(CAudClip& aClip, 
        TAudDynamicLevelMark& aMark, 
        TInt aIndex);

    void FireDynamicLevelMarkRemoved(CAudSong& aSong, TInt aIndex);
    void FireDynamicLevelMarkRemoved(CAudClip& aClip, TInt aIndex);
    void FireSongReseted(CAudSong& aSong);
    void FireClipReseted(CAudClip& aClip);

    // <------------ Notifications to fire callbacks to all registered listeners

    friend class CAudSongAddClipOperation;
    friend class CAudClip;
    friend class CAudProcessorImpl;
    
    
#ifdef _WRITE_OUTPUT_TO_FILE_

    TBool iFileOpen;
    RFs iDebFs;
    RFile iAudioFile;
    RFile iTextFile;
    
    
#endif
    
    };


// Abstract interface class to get events from audio processor

class MProcProcessObserver 
    {

public:
    /**
     * Called to notify that a new audio processing operation has been started. 
     *
     */
    virtual void NotifyAudioProcessingStartedL() = 0;

    /**
     * Called to inform about the current progress of the audio processing operation.
     *
     * @param aPercentage  percentage of the operation completed, must be 
       *                     in range 0..100
     */
    virtual void NotifyAudioProcessingProgressed(TInt aPercentage) = 0;

    /**
    * Called to notify that the song processing operation has been completed. 
    * 
    * @param aError  error code why the operation was completed. 
    *                <code>KErrNone</code> if the operation was completed 
    *                successfully.
    */
    virtual void NotifyAudioProcessingCompleted(TInt aError) = 0;

    };



/**
* Internal class for processing a song.
*/
class CAudSongProcessOperation : public CBase, public MProcProcessObserver, public MAudTimeEstimateObserver
    {
    
    
    
public:

    
    static CAudSongProcessOperation* NewL(CAudSong* aSong);

    // From MProcProcessObserver

    void NotifyAudioProcessingStartedL();
    void NotifyAudioProcessingProgressed(TInt aPercentage);
    void NotifyAudioProcessingCompleted(TInt aError);
    
    // From // MAudTimeEstimateObserver
    void NotifyTimeEstimateReady(TInt64 aTimeEstimate);

    /**
    * Starts asyncronous processing a song
    * 
    * Can leave with one of the system wide error codes
    *
    * Possible panic code
    * <code>ESongProcessingOperationAlreadyRunning</code>
    *
    * @param    aFileName    output file name
    * @param    aObserver    an observer to be notified of progress
    * @param    aPriority    priority of audio processing operation
    */
    void StartASyncProcL(const TDesC& aFileName, 
        MAudSongProcessingObserver& aObserver, TInt aPriority);


    /**
    * Starts syncronous song processing
    * 
    * Can leave with one of the system wide error codes
    *
    * Possible panic code
    * <code>ESongProcessingOperationAlreadyRunning</code>
    *
    */
    TBool StartSyncProcL();
    
    
    /**
    * Processes synchronously next audio frame and passes it to the caller. 
    * If a processing operation has not been started, panic 
    * <code>TAudPanic::ESongProcessingNotRunning</code> is raised.
    *
    * Possibly leave codes:
    *    - <code>KErrNoMemory</code> if memory allocation fails
    *
    * NOTE: This function allocates memory and the caller is responsible for
    * releasing it.
    *
    * @param aFrame        pointer to a processed audio frame. Has to be deleted
    *                    when not needed anymore.
    * @param aDuration    Duration of processed audio frame in microseconds 
    *                    after decoding
    * 
    * @return ETrue        Processing operation completed, no more frames to be processed. 
    *                    No memory allocated.
    *          EFalse    Processing operation not completed. Last processed frame stored
    *                    in <code>aFrame</code>. Memory allocated for the processed frame
    *                      
    */
    TBool ProcessSyncPieceL(HBufC8*& aFrame, TInt& aProgress,
                                       TTimeIntervalMicroSeconds& aDuration);

    /**
    * Cancels both syncronous and and asyncronous song processing operation and deletes the output file
    *
    * Possible panic code
    * <code>ESongProcessingOperationNotRunning</code>
    *
    */
    void Cancel();
    
    TBool GetTimeEstimateL(MAudTimeEstimateObserver& aTEObserver);


private:

    CAudSongProcessOperation(CAudSong* aSong);
    void ConstructL();
    virtual ~CAudSongProcessOperation();
    
private:
    
   /* 
    * Member Variables
    */
    
    // song
    CAudSong *iSong;
    
    // song processing observer
    MAudSongProcessingObserver* iObserver;
    
    // time estimate observer
    MAudTimeEstimateObserver* iTEObserver;
    
    // A flag that indicates whether this song has changed since
    // the last processing
    TBool iChanged;
    
    // processor owned by this
    CAudProcessor* iProcessor;
    
    friend class CAudSong;
    
    
    };


/**
* Internal class for adding clips.
*/
class CAudSongAddClipOperation : public CBase, public MAudClipInfoObserver 
    {

public:

    /*
    * Constuctor & destructor
    */

    static CAudSongAddClipOperation* NewL(CAudSong* aSong);

    virtual ~CAudSongAddClipOperation();
        
    /*
    * From base class MAudClipInfoObserver
    */
    virtual void NotifyClipInfoReady(CAudClipInfo& aInfo, 
        TInt aError);
    
    
private:
    
    /*
    * ConstructL
    */
    void ConstructL();
    
    /*
    * C++ constructor
    */
    CAudSongAddClipOperation(CAudSong* aSong);
    
    /*
    * Completes add clip operation
    */
    void CompleteAddClipOperation();
 
    
private:

    // song
    CAudSong* iSong;
    
    // clip that we are trying to add
    CAudClip* iClip;
    
    // error possible caught by callback function NotifyClipInfoReady
    TInt iError;
    
    friend class CAudSong;
    };



#endif
