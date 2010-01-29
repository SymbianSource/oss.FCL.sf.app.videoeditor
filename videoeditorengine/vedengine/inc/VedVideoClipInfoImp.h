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



#ifndef __VEDVIDEOCLIPINFOIMP_H__
#define __VEDVIDEOCLIPINFOIMP_H__


#include <e32base.h>

#include "movieprocessor.h"
#include "VedCommon.h"
#include "VedVideoClipInfo.h"

#include "AudObservers.h"

/*
 *  Forward declarations.
 */
class CFbsBitmap;  // cannot include bitmap.h since video processor includes
                   // this file and fails to compile due to a strange compiler error
                   // "operator delete must return type void" if bitmap.h
                   // is included

class CVedVideoClipInfo;
class TVedVideoFrameInfo;
class CVedVideoClipInfoOperation;
class CVedVideoClipFrameOperation;
class CAudClipInfo;
class CVedVideoClip;


/**
 * Utility class for getting information about video clip files.
 */
class CVedVideoClipInfoImp : public CVedVideoClipInfo
    {
public:

    static CVedVideoClipInfo* NewL(CAudClipInfo* aAudClipInfo,
                                   const TDesC& aFileName, MVedVideoClipInfoObserver& aObserver);
    /**
     * Destroys the object and releases all resources.
     */    
    ~CVedVideoClipInfoImp();


    /* General property methods. */


    TPtrC DescriptiveName() const;

    /**
     * Returns the file name of the clip. Panics if info
     * is not yet ready for reading.
     * 
     * @return  file name of the clip
     */
    TPtrC FileName() const;

    CVedVideoClipGenerator* Generator() const;

    TVedVideoClipClass Class() const;

    /**
     * Returns the video format of the clip. Panics if info
     * is not yet ready for reading.
     * 
     * @return  video format of the clip
     */
    TVedVideoFormat Format() const;

    /**
     * Returns the video type of the clip. Panics if info
     * is not yet ready for reading.
     * 
     * @return  video type of the clip
     */
    TVedVideoType VideoType() const;

    /**
     * Returns the resolution of the clip. Panics if info
     * is not yet ready for reading.
     * 
     * @return  resolution of the clip
     */
    TSize Resolution() const;

    /**
     * Returns whether this video clip has an audio track or not.
     * Panics if info is not yet ready for reading.  
     *
     * @return  <code>ETrue</code> if clip has an audio track;
     *          <code>EFalse</code> otherwise
     */
    TBool HasAudio() const;

    /**
     * Returns the audio type of the clip audio track. Panics if info
     * is not yet ready for reading.
     * 
     * @return  audio type of the clip audio track
     */
    TVedAudioType AudioType() const;

    /**
     * Returns the channel mode of the audio if applicable.
     *
     * @return  channel mode
     */
    TVedAudioChannelMode AudioChannelMode() const;

    /**
     * Returns the sampling rate in kilohertz.
     *
     * @return  sampling rate
     */
    TInt AudioSamplingRate() const;

    /**
     * Returns the duration of the clip in microseconds. Panics if info
     * is not yet ready for reading.
     * 
     * @return  duration of the clip
     */
    TTimeIntervalMicroSeconds Duration() const;


    /* Video frame property methods. */

    /**
     * Returns the number of video frames in this clip. Panics if info
     * is not yet ready for reading.
     * 
     * @return  number of video frames in this clip
     */
    TInt VideoFrameCount() const;

    /** 
     * Generates video frame info that is needed in VideoFrame API functions.
     * 
     * @return  error code
     */ 
    TInt GenerateVideoFrameInfoArrayL();    

    /** 
     * Returns the start time of the video frame at the specified index. 
     * Panics if info is not yet ready for reading or the index is illegal.
     *
     * @param aIndex  index
     *
     * @return  start time of the video frame at the specified index in microseconds
     */ 
    TTimeIntervalMicroSeconds VideoFrameStartTimeL(TInt aIndex);

    /** 
     * Returns the end time of the video frame at the specified index. 
     * Panics if info is not yet ready for reading or the index is illegal.
     *
     * @param aIndex  index
     *
     * @return  end time of the video frame at the specified index in microseconds
     */ 
    TTimeIntervalMicroSeconds VideoFrameEndTimeL(TInt aIndex);

    /** 
     * Returns the duration of the video frame at the specified index. 
     * Panics if info is not yet ready for reading or the index is illegal.
     *
     * @param aIndex  index
     *
     * @return  duration of the video frame at the specified index in microseconds
     */ 
    TTimeIntervalMicroSeconds VideoFrameDurationL(TInt aIndex);

    /** 
     * Returns the size of the video frame at the specified index. 
     * Panics if info is not yet ready for reading or the index is illegal.
     *
     * @param aIndex  index
     *
     * @return  size of the video frame at the specified index in bytes
     */ 
    TInt VideoFrameSizeL(TInt aIndex);

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
    TBool VideoFrameIsIntraL(TInt aIndex);

    /**
     * Returns the video frame index at the specified time. Panics if info is not yet 
     * ready for reading or the time is illegal.
     *
     * @param aTime  time
     *
     * @return  video frame index at the specified time
     */
    TInt GetVideoFrameIndexL(TTimeIntervalMicroSeconds aTime);


    /* Frame methods. */

    void GetFrameL(MVedVideoClipFrameObserver& aObserver,
                            TInt aIndex,
                            TSize* const aResolution,
                            TDisplayMode aDisplayMode,
                            TBool aEnhance,
                            TInt aPriority);
    
    void CancelFrame();

    void SetTranscodeFactor(TVedTranscodeFactor aFactor);

    TVedTranscodeFactor TranscodeFactor();

    /**
     * Returns whether video clip is MMSCompatible.
     *
     * @return  ETrue if compatible with MMS
     */
    TBool IsMMSCompatible();
    
    static CVedVideoClipInfo* NewL(CAudClipInfo* aAudClipInfo,
                                   RFile* aFileHandle, MVedVideoClipInfoObserver& aObserver);
                                   
    RFile* FileHandle() const;

private:
    CVedVideoClipInfoImp(CAudClipInfo* aAudClipInfo);

    void ConstructL(const TDesC& aFileName,
                    MVedVideoClipInfoObserver& aObserver);
                    
    void ConstructL(RFile* aFileHandle,
                    MVedVideoClipInfoObserver& aObserver);                    
                    
private:
    // Member variables

    // Get audio info operation.
    CVedVideoClipInfoOperation* iInfoOperation;
    // Flag to indicate then info is available
    TBool iReady;
    
    // Filename of the video clip.
    HBufC* iFileName;
    
    // File handle of the video clip
    RFile* iFileHandle;
    
    // Vídeo format.
    TVedVideoFormat iFormat;
    // Video type/codec.
    TVedVideoType iVideoType;
    // Resolution of video clip.
    TSize iResolution;    
    // Duration of the video clip.
    TTimeIntervalMicroSeconds iDuration;
    // Frame count of video
    TInt iVideoFrameCount;
    // Array of frame information.
    TVedVideoFrameInfo* iVideoFrameInfoArray;
    
    // Operation to retrieve thumbnail of video clip.
    CVedVideoClipFrameOperation* iFrameOperation;

    // Transcode factor
    TVedTranscodeFactor iTimeFactor;

    // Is the frame info array ready
    TBool iVideoFrameInfoArrayReady;
    
    // Flag for audio clip info ownership
    TBool iAudClipInfoOwnedByVideoClipInfo;

    // Whether the video clip info is part of a video clip or just the info
    TBool iVideoClipIsStandalone;        
    
    // Audio clip info.
    CAudClipInfo* iAudClipInfo;
    
    // These are got from iAudClipInfo
    
    // Audio type/codec.
    //TVedAudioType iAudioType;
    // Following members are only used for AAC audio
    // Channel mode
    //TVedAudioChannelMode iAudioChannelMode;
    // Sampling rate
    //TInt iAudioSamplingRate;

    friend class CVedVideoClipInfoOperation;
    friend class CVedVideoClipFrameOperation;
    friend class CVedVideoClipInfo;
    };


/**
 * Internal class for storing information about video frames.
 */
class TVedVideoFrameInfo
    {
public:
    /** Frame start time in MILLISECONDS (not microseconds). */
    TInt iStartTime;

    /** Frame size in bytes. */
    TInt iSize;

    /** Frame information flags. */
    TInt8 iFlags;
    };

#define KVedVideoFrameInfoFlagIntra (1 << 0)


/**
 * Internal class for reading information from a video clip file.
 * Also implements a simple active object to notify the video clip info 
 * observer when reading has been completed.
 */
class CVedVideoClipInfoOperation : public CActive, MAudClipInfoObserver
    {
public:
    /* Static constructor */
    static CVedVideoClipInfoOperation* NewL(CVedVideoClipInfoImp* aInfo,
                                        MVedVideoClipInfoObserver& aObserver);
protected:
    /*
    * From CActive
    * Standard active object RunL 
    */
    void RunL();

    /*
    * From CActive
    * Standard active object DoCancel
    */
    void DoCancel();

private:
    /* Default constructor */
    CVedVideoClipInfoOperation(CVedVideoClipInfoImp* aInfo, 
                               MVedVideoClipInfoObserver& aObserver);
    /* Standard Symbian OS two phased constructor */
    void ConstructL();
    /* Destructor */
    ~CVedVideoClipInfoOperation();

private: // functions from base classes

    /* From MAudClipInfoObserver */
    void NotifyClipInfoReady(CAudClipInfo& aInfo, TInt aError);
private:
    // Class to contain video clip info.
    CVedVideoClipInfoImp* iInfo;
    // Observer of video clip info operation.
    MVedVideoClipInfoObserver* iObserver;
    // Error code from prosessor.
    TInt iMovieProcessorError;
	// This flag tells us whether we're reading the audio clip info
    TBool iGettingAudio;

    friend class CVedVideoClipInfoImp;
    };

/**
 * Internal class for generating a frame from a video clip file.
 */
class CVedVideoClipFrameOperation : public CActive
    {
public:
    /* Static constructor */
    static CVedVideoClipFrameOperation* NewL(CVedVideoClipInfoImp* iInfo);

protected:
    /*
    * From CActive
    * Standard active object RunL 
    */
    void RunL();

    /*
    * From CActive
    * Standard active object DoCancel
    */
    void DoCancel();

private:
    /* Default constructor */
    CVedVideoClipFrameOperation(CVedVideoClipInfoImp* iInfo);
    /* Standard Symbian OS two phased constructor */
    void ConstructL();
    /* Destructor */
    ~CVedVideoClipFrameOperation();

    /*
    * Start frame operation.
    *
    * @aparam aObserver Observer of thumbnail operation.
    * @aparam aIndex Index of frame that is converted to thumbnail.
    * @aparam aResolution Wanted resolution of thumbnail.
    * @aparam aPriority Priority of active object.
    */
    void StartL(MVedVideoClipFrameObserver& aObserver,
                TInt aIndex, TSize* const aResolution, 
                TDisplayMode aDisplayMode, TBool aEnhance, TInt aPriority);

private:
    // Pointer to info class this thumbnail operation is part of.
    
    CVedVideoClipInfoImp* iInfo;
    // Index of the wanted frame.
    TInt iIndex;

    // Observer of the thumbnail operation.
    MVedVideoClipFrameObserver* iObserver;

    TVedTranscodeFactor iFactor;
    
    TBool iThumbRequestPending;

    CMovieProcessor* iProcessor;

    friend class CVedVideoClipInfoImp;
    };

#endif // __VEDVIDEOCLIPINFOIMP_H__

