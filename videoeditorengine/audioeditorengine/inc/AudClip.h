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




#ifndef __AUDCLIP_H__
#define __AUDCLIP_H__

    

#include <e32base.h>
#include <f32file.h>


#include "AudCommon.h"
#include "AudObservers.h"

/*
*    Forward declarations.
*/

class CAudSong;
class CAudSongObserver;
class CAudClipInfo;
class MAudClipInfoObserver;


/*
*  Constant definitions.
*/


/**
* Individual audio clip stored as a single bitstream.
*
* @see CAudSong
*/
class CAudClip : public CBase
    {
public:
    
    /* Property methods. */
    
    /**
    * Returns an audio clip info object to get detailed information about
    * the original audio clip. Note that the specified editing operations 
    * (for example, cutting or muting) do <em>not</em>
    * affect the values returned by the info object.
    * 
    * @return  pointer to an audio clip info instance
    */
    IMPORT_C CAudClipInfo* Info() const;
    
    /**
    * Returns a boolean value that indicates whether this clip is normalized
    * 
    * @return  ETrue if normalized, EFalse otherwise
    */
    IMPORT_C TBool Normalizing() const;
    
    /**
    * Returns a dynamic level mark at the specified index
    * If the index is illegal, the method panics with the code 
    * <code>EIllegalDynamicLevelMarkIndex</code> 
    *
    * @return  A dynamic level mark
    */
    IMPORT_C TAudDynamicLevelMark DynamicLevelMark(TInt aIndex) const;
    
    /**
    * Returns the number of dynamic level marks
    * 
    * @return  The number of dynamic level mark
    */
    IMPORT_C TInt DynamicLevelMarkCount() const;

    /**
    * Returns whether this clip is muted or not
    * 
    * @return  muting
    */
    IMPORT_C TBool Muting() const;

    /**
    * Sets the start time of this audio clip in clip timebase.
    * Panics with <code>EAudioClipIllegalStartTime</code> if
    * cut in time is illegal (negative).
    *
    * @param aCutInTime  cut in time in microseconds in clip timebase
    */
    IMPORT_C void SetStartTime(TTimeIntervalMicroSeconds aStartTime);

    /**
    * Returns the start time of this audio clip in song timebase.
    *
    * @return  start time in microseconds in song timebase
    */
    IMPORT_C TTimeIntervalMicroSeconds StartTime() const;
    
    /**
    * Returns the start time of this audio clip in song timebase.
    *
    * @return  start time in milliseconds in song timebase
    */
    TInt32 StartTimeMilliSeconds() const;
    

    /**
    * Returns the end time of this audio clip in song timebase.
    *
    * @return  end time in microseconds in song timebase
    */
    IMPORT_C TTimeIntervalMicroSeconds EndTime() const;
    
    /**
    * Returns the duration of this audio clip with the specified
    * editing operations applied (for example, cutting)
    * 
    * @return  duration in microseconds
    */
    IMPORT_C TTimeIntervalMicroSeconds EditedDuration() const;
    
    /**
    * Returns the priority of this audio clip 
    *
    * @return  priority
    */
    IMPORT_C TInt Priority() const;

    /**
    * Returns the index of this audio clip __on a track__
    *
    * @return  index
    */
    IMPORT_C TInt IndexOnTrack() const;

    
    /**
    * Returns the track index of this audio clip in CAudSong
    *
    * @return  track index
    */
    IMPORT_C TInt TrackIndex() const;



    /* Song methods. */
    
    /**
    * Returns the song that this audio clip is part of.
    * 
    * @return  song
    */
    IMPORT_C CAudSong* Song() const;
    
    /* Timing methods. */
    
    /**
    * Returns the cut in time of this audio clip in clip timebase.
    *
    * @return  cut in time in microseconds in clip timebase
    */
    IMPORT_C TTimeIntervalMicroSeconds CutInTime() const;

    /**
    * Returns the cut in time of this audio clip in clip timebase.
    *
    * @return  cut in time in milliseconds in clip timebase
    */
    TInt32 CutInTimeMilliSeconds() const;
    
    /**
    * Sets the cut in time of this audio clip in clip timebase.
    * Panics with <code>EAudioClipIllegalCutInTime</code> if
    * cut in time is illegal.
    *
    * @param aCutInTime  cut in time in microseconds in clip timebase
    */
    IMPORT_C void SetCutInTime(TTimeIntervalMicroSeconds aCutInTime);
    
    /**
    * Returns the cut out time of this audio clip in clip timebase.
    *
    * @return  cut out time in microseconds in clip timebase
    */
    IMPORT_C TTimeIntervalMicroSeconds CutOutTime() const;
    
    /**
    * Returns the cut out time of this audio clip in clip timebase.
    *
    * @return  cut out time in milliseconds in clip timebase
    */
    TInt32 CutOutTimeMilliSeconds() const;

    /**
    * Sets the cut out time of this audio clip in clip timebase. 
    * Panics with <code>EAudioClipIllegalCutOutTime</code> if
    * cut out time is illegal.
    *
    * @param aCutOutTime  cut out time in microseconds in clip timebase
    */
    IMPORT_C void SetCutOutTime(TTimeIntervalMicroSeconds aCutOutTime);
        
    /**
    * Sets the priority of this audio clip
    *
    * @param        aPriority priority, >= 0
    *
    * @return        ETrue if aPriority >= 0
    *                EFalse if aPriority < 0, priority not set
    *
    */
    IMPORT_C TBool SetPriority(TInt aPriority);
    
    
    /* Processing methods */
    
    /**
    * Inserts a dynamic level mark to a clip.
    * If time of the given mark is illegal
    * <code>EIllegalDynamicLevelMark</code>-panic is raised
    *
    * Possible leave codes:
    *    - <code>KErrNoMemory</code> if memory allocation fails
    *
    * @param    aMark mark to be added in clip time domain
    * @return   index of the mark inserted
    */
    IMPORT_C TInt InsertDynamicLevelMarkL(const TAudDynamicLevelMark& aMark);
    
    /**
    * Removes a dynamic level mark in a clip.
    * Panics with code <code>EAudioClipIllegalIndex</code> 
    * if the clip index is invalid.
    *
    * @param    aIndex index of the removed mark in this clip
    * @return  ETrue if mark was removed, EFalse otherwise
    */
    IMPORT_C TBool RemoveDynamicLevelMark(TInt aIndex);
    
    /**
    * Sets whether this clip is muted or not.
    *
    * @param aMuting  <code>ETrue</code> to mute the audio clip;
    *                 <code>EFalse</code> not to mute the audio clip
    */
    IMPORT_C void SetMuting(TBool aMuted);
    
    /**
    * Sets whether this clip is normalized
    * 
    * @param  aNormalizing <code>ETrue</code> if normalized 
    *                        <code>EFalse</code> otherwise
    */
    IMPORT_C void SetNormalizing(TBool aNormalizing);
    
    IMPORT_C void Reset(TBool aNotify);

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
    IMPORT_C void SetVolumeGain(TInt aVolumeGain);
    
    /**
    * Gets common volume gain for the clip. 
    * Since global gain setting may affect the dynamic level mark,
    * we need different variable to store the clip-specific gain also after the processing.
    *
    * @param aVolumeGain
    */
    IMPORT_C TInt GetVolumeGain();
    
    /**
    *
    * Compare is used by RPointerArray::Sort()
    *
    */
    static TInt Compare(const CAudClip &c1, const CAudClip &c2);


protected: 
    
    
private:
    
    
    
    /* Constructors. */
    
    static CAudClip* NewL(CAudSong* aSong, const TDesC& aFileName,
        TTimeIntervalMicroSeconds aStartTime,                            
        MAudClipInfoObserver& aObserver, TInt aTrackIndex);
        
    static CAudClip* NewL(CAudSong* aSong, RFile* aFileHandle,
        TTimeIntervalMicroSeconds aStartTime,                            
        MAudClipInfoObserver& aObserver, TInt aTrackIndex);
    
    CAudClip(CAudSong* aSong);
    
    void ConstructL(const TDesC& aFileName,
        TTimeIntervalMicroSeconds aStartTime,                            
        MAudClipInfoObserver& aObserver, TInt aTrackIndex);
        
    void ConstructL(RFile* aFileHandle,
        TTimeIntervalMicroSeconds aStartTime,                            
        MAudClipInfoObserver& aObserver, TInt aTrackIndex);
    
    /* Destructor. */
    
    /**
    * Destroys the object and releases all resources.
    */    
    virtual ~CAudClip();
    
private:
    // Member variables
    
    // Song class this clip is part of.
    CAudSong* iSong;
    // File of the audio clip.
    RFile iFile;        
    
    
    // Info class of this audio clip.
    CAudClipInfo* iInfo;
    
    // Cut in time of this audio clip.
    TTimeIntervalMicroSeconds iCutInTime;
    // Cut out time of this audio clip.
    TTimeIntervalMicroSeconds iCutOutTime;
    // Start time of this audio clip.
    TTimeIntervalMicroSeconds iStartTime;
    
    // Muting of this audio clip.
    // If ETrue, the clip is to be muted
    TBool iMute;
    
    // Normalizing of this audio clip.
    // If ETrue, the clip is to be normalized
    TBool iNormalize;
    
    // Priority of this audio clip.
    TInt iPriority;
 
    // index of this clip in CAudSong
    TInt iIndex;

    // track index of this clip
    TInt iTrackIndex;
    
    TInt iVolumeGain;
    
    // Marks used for manual level controlling
    RPointerArray<TAudDynamicLevelMark> iDynamicLevelMarkArray;
    
    friend class CAudSong;
    friend class CAudSongAddClipOperation;
    friend class RPointerArray<CAudClip>;
    
    };


#endif // __AUDCLIP_H__

