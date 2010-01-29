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



#ifndef __VEDVIDEOCLIPINFO_H__
#define __VEDVIDEOCLIPINFO_H__

#include "VedCommon.h"

#include <gdi.h>

#define KFrameIndexBestThumb (-1) // search for best possible thumbnail from video

/*
 *  Forward declarations.
 */
class CFbsBitmap;  
class CVedVideoClipInfo;
class CVedVideoClipGenerator;

/**
 * Observer for notifying that video clip info
 * is ready for reading.
 *
 * @see  CVedVideoClipInfo
 */
class MVedVideoClipInfoObserver 
    {
public:
    /**
     * Called to notify that video clip info is ready
     * for reading.
     *
     * Possible error codes:
     *  - <code>KErrNotFound</code> if there is no file with the specified name
     *    in the specified directory (but the directory exists)
     *  - <code>KErrPathNotFound</code> if the specified directory
     *    does not exist
     *  - <code>KErrUnknown</code> if the specified file is of unknown format
     *
     * @param aInfo   video clip info
     * @param aError  <code>KErrNone</code> if info is ready
     *                for reading; one of the system wide
     *                error codes if reading file failed
     */
    virtual void NotifyVideoClipInfoReady(CVedVideoClipInfo& aInfo, 
                                          TInt aError) = 0;
    };


/**
 * Observer for notifying that video clip frame has been completed.
 *
 * @see  CVedVideoClipInfo
 */
class MVedVideoClipFrameObserver
    {
public:
    /**
     * Called to notify that video clip frame has been completed. 
     * 
     * @param aInfo   video clip info
     * @param aError  <code>KErrNone</code> if frame was
     *                completed successfully; one of the system wide
     *                error codes if generating frame failed
     * @param aFrame  pointer to frame if it was completed successfully;
     *                <code>NULL</code> if generating frame failed
     */
    virtual void NotifyVideoClipFrameCompleted(CVedVideoClipInfo& aInfo, 
                                               TInt aError, 
                                               CFbsBitmap* aFrame) = 0;
    };

/**
 * Utility class for getting information about video clip files.
 */
class CVedVideoClipInfo : public CBase
    {
public:

    /* Constructors & destructor. */

    /**
     * Constructs a new CVedVideoClipInfo object to get information
     * about the specified video clip file. The specified observer
     * is notified when info is ready for reading. This method
     * may leave if no resources are available to construct 
     * a new object.
     * The file will be opened in EFileShareReadersOnly mode by default, 
     * and the same mode should be used by the client too if it need to open
     * the file at the same time.
     * 
     * Possible leave codes:
     *  - <code>KErrNoMemory</code> if memory allocation fails
     *
     * @param aFileName  name of video clip file
     * @param aObserver  observer to notify when info is ready for reading
     *
     * @return  pointer to a new CVedVideoClipInfo instance
     */
    IMPORT_C static CVedVideoClipInfo* NewL(const TDesC& aFileName,
                                            MVedVideoClipInfoObserver& aObserver);

    /**
     * Constructs a new CVedVideoClipInfo object to get information
     * about the specified video clip file. The constructed object
     * is left in the cleanup stack. The specified observer
     * is notified when info is ready for reading. This method
     * may leave if no resources are available to construct a new
     * object.
     * The file will be opened in EFileShareReadersOnly mode by default, 
     * and the same mode should be used by the client too if it need to open
     * the file at the same time.
     * 
     * Possible leave codes:
     *  - <code>KErrNoMemory</code> if memory allocation fails
     *
     * @param aFileName  name of video clip file
     * @param aObserver  observer to notify when info is ready for reading
     *
     * @return  pointer to a new CVedVideoClipInfo instance
     */
    IMPORT_C static CVedVideoClipInfo* NewLC(const TDesC& aFileName,
                                             MVedVideoClipInfoObserver& aObserver);

    /* General property methods. */


    /**
     * Returns a descriptive name for the clip. Panics if info is not yet
     * ready for reading.
     *
     * @return  descriptive name of the clip
     */
    virtual TPtrC DescriptiveName() const = 0;

    /**
     * Returns the file name of the clip. Panics if there is no file 
     * associated with this clip or info is not yet ready for reading.
     * 
     * @return  file name of the clip
     */
    virtual TPtrC FileName() const = 0;

    /**
     * Returns the generator of the clip. Panics if there is no video clip
     * generator associated with the clip or info is not yet ready for reading.
     *
     * @return  generator of the clip
     */
    virtual CVedVideoClipGenerator* Generator() const = 0;

    /**
     * Returns the class of the clip.
     *
     * @return  class of the clip
     */
    virtual TVedVideoClipClass Class() const = 0;

    /**
     * Returns the video format of the clip. Panics if there is no file 
     * associated with this clip or info is not yet ready for reading.
     * 
     * @return  video format of the clip
     */
    virtual TVedVideoFormat Format() const = 0;

    /**
     * Returns the video type of the clip. Panics if there is no file 
     * associated with this clip or info is not yet ready for reading.
     * 
     * @return  video type of the clip
     */
    virtual TVedVideoType VideoType() const = 0;

    /**
     * Returns the resolution of the clip. Panics if info
     * is not yet ready for reading.
     * 
     * @return  resolution of the clip
     */
    virtual TSize Resolution() const = 0;

    /**
     * Returns whether this video clip has an audio track or not.
     * Panics if info is not yet ready for reading.  
     *
     * @return  <code>ETrue</code> if clip has an audio track;
     *          <code>EFalse</code> otherwise
     */
    virtual TBool HasAudio() const = 0;

    /**
     * Returns the audio type of the clip audio track. Panics if there is no file 
     * associated with this clip or info is not yet ready for reading.
     * 
     * @return  audio type of the clip audio track
     */
    virtual TVedAudioType AudioType() const = 0;

    /**
     * Returns the channel mode of the audio if applicable.
     *
     * @return  channel mode
     */
    virtual TVedAudioChannelMode AudioChannelMode() const = 0;

    /**
     * Returns the sampling rate in kilohertz.
     *
     * @return  sampling rate
     */
    virtual TInt AudioSamplingRate() const = 0;

    /**
     * Returns the duration of the clip in microseconds. Panics if info
     * is not yet ready for reading.
     * 
     * @return  duration of the clip
     */
    virtual TTimeIntervalMicroSeconds Duration() const = 0;


    /* Video frame property methods. */

    /**
     * Returns the number of video frames in this clip. Panics if info
     * is not yet ready for reading.
     * 
     * @return  number of video frames in this clip
     */
    virtual TInt VideoFrameCount() const = 0;
    
    /** 
     * Returns the start time of the video frame at the specified index. 
     * Panics if info is not yet ready for reading or the index is illegal.
     *
     * @param aIndex  index
     *
     * @return  start time of the video frame at the specified index in microseconds
     */ 
    virtual TTimeIntervalMicroSeconds VideoFrameStartTimeL(TInt aIndex) = 0;

    /** 
     * Returns the end time of the video frame at the specified index. 
     * Panics if info is not yet ready for reading or the index is illegal.
     *
     * @param aIndex  index
     *
     * @return  end time of the video frame at the specified index in microseconds
     */ 
    virtual TTimeIntervalMicroSeconds VideoFrameEndTimeL(TInt aIndex) = 0;

    /** 
     * Returns the duration of the video frame at the specified index. 
     * Panics if info is not yet ready for reading or the index is illegal.
     *
     * @param aIndex  index
     *
     * @return  duration of the video frame at the specified index in microseconds
     */ 
    virtual TTimeIntervalMicroSeconds VideoFrameDurationL(TInt aIndex) = 0;

    /** 
     * Returns the size of the video frame at the specified index. 
     * Panics if there is no file associated with this clip, or info is not 
     * yet ready for reading, or the index is illegal.
     *
     * @param aIndex  index
     *
     * @return  size of the video frame at the specified index in bytes
     */ 
    virtual TInt VideoFrameSizeL(TInt aIndex) = 0;

    /** 
     * Returns whether the video frame at the specified index is an intra
     * frame or not. Panics if info is not yet ready for reading or 
     * the index is illegal.
     *
     * @param aIndex  index
     *
     * @return  <code>ETrue</code>, if the video frame at the specified index is an
     *          intra frame; <code>EFalse</code>, otherwise
     */ 
    virtual TBool VideoFrameIsIntraL(TInt aIndex) = 0;

    /**
     * Returns the video frame index at the specified time. Panics if info is not yet 
     * ready for reading or the time is illegal.
     *
     * @param aTime  time
     *
     * @return  video frame index at the specified time
     */
    virtual TInt GetVideoFrameIndexL(TTimeIntervalMicroSeconds aTime) = 0;


    /* Frame methods. */

    /**
     * Generates a bitmap of the given frame from video clip.
     * The frame bitmap is scaled to the specified resolution and converted
     * to the specified display mode. This method is asynchronous. The frame
     * is generated in background and the observer is notified when the frame
     * is complete. This method panics if info is not yet ready for reading or 
     * the resolution is illegal.
     * 
     * Possible leave codes:
     *  - <code>KErrNoMemory</code> if memory allocation fails
     *  - <code>KErrNotSupported</code>, if the specified combination of 
     *                                   parameters is not supported
     *
     * @param aObserver    observer to be notified when the frame is completed
     * @param aIndex       index of frame, or KFrameIndexBestThumb to look for
     *                     most suitable thumbnail frame.
     * @param aResolution  resolution of the desired frame bitmap, or
     *                     <code>NULL</code> if the frame should be
     *                     in the original resolution
     * @param aDisplayMode desired display mode; or <code>ENone</code> if 
     *                     any display mode is acceptable
     * @param aEnhance     apply image enhancement algorithms to improve
     *                     frame quality; note that this may considerably
     *                     increase the processing time needed to prepare
     *                     the frame
     * @param aPriority    priority of the frame generation
     */
    virtual void GetFrameL(MVedVideoClipFrameObserver& aObserver,
                            TInt aIndex = KFrameIndexBestThumb,
                            TSize* const aResolution = 0,
                            TDisplayMode aDisplayMode = ENone,
                            TBool aEnhance = EFalse,
                            TInt aPriority = CActive::EPriorityIdle) = 0;
    
    /**
     * Cancels frame generation. If no frame is currently being 
     * generated, the function does nothing.
     */
    virtual void CancelFrame() = 0;

    /**
     * Sets the transcode factor.
     *
     * @param aFactor  transcode factor
     */
    virtual void SetTranscodeFactor(TVedTranscodeFactor aFactor) = 0;

    /**
     * Returns the transcode factor.
     *
     * @return  transcode factor.
     */
    virtual TVedTranscodeFactor TranscodeFactor() = 0;

    /**
     * Returns whether video clip is MMSCompatible.
     *
     * @return  ETrue if compatible with MMS
     */
    virtual TBool IsMMSCompatible() = 0;
    
    IMPORT_C static CVedVideoClipInfo* NewL(RFile* aFileHandle,
                                            MVedVideoClipInfoObserver& aObserver);
                                            
    IMPORT_C static CVedVideoClipInfo* NewLC(RFile* aFileHandle,
                                             MVedVideoClipInfoObserver& aObserver);
                                             
     /**
     * Returns the file name of the clip. Panics if there is no file 
     * associated with this clip or info is not yet ready for reading.
     * 
     * @return  file name of the clip
     */
    virtual RFile* FileHandle() const = 0;
    
    };



#endif // __VEDVIDEOCLIPINFO_H__

