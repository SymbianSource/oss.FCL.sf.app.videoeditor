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



#ifndef __VEDAUDIOCLIPINFO_H__
#define __VEDAUDIOCLIPINFO_H__

#include <f32file.h>
#include "VedCommon.h"

/*
 *  Forward declarations.
 */
class CVedAudioClipInfo;

/**
 * Observer for notifying that audio clip info
 * is ready for reading.
 *
 * @see  CVedAudioClipInfo
 */
class MVedAudioClipInfoObserver 
    {
public:
    /**
     * Called to notify that audio clip info is ready
     * for reading.
     *
     * Possible error codes:
     *  - <code>KErrNotFound</code> if there is no file with the specified name
     *    in the specified directory (but the directory exists)
     *  - <code>KErrPathNotFound</code> if the specified directory
     *    does not exist
     *  - <code>KErrUnknown</code> if the specified file is of unknown type
     *
     * @param aInfo   audio clip info
     * @param aError  <code>KErrNone</code> if info is ready
     *                for reading; one of the system wide
     *                error codes if reading file failed
     */
    virtual void NotifyAudioClipInfoReady(CVedAudioClipInfo& aInfo, 
                                          TInt aError) = 0;
    };


/**
 * Observer for audio clip visualization.
 *
 * @see  CVedAudioClipInfo
 */
class MVedAudioClipVisualizationObserver
    {
public:

    /**
     * Called to notify that audio clip visualization has been started. 
     * 
     * @param aInfo  audio clip info
     */
    virtual void NotifyAudioClipVisualizationStarted(const CVedAudioClipInfo& aInfo) = 0;

    /**
     * Called to inform about the current progress of the audio clip visualization.
     *
     * @param aInfo        audio clip info
     * @param aPercentage  percentage of the operation completed, must be 
       *                     in range 0..100
     */
    virtual void NotifyAudioClipVisualizationProgressed(const CVedAudioClipInfo& aInfo, 
                                                        TInt aPercentage) = 0;

    /**
     * Called to notify that audio clip visualization has been completed. 
     * Note that if the visualization was successfully completed, the ownership 
     * of the visualization array is passed to the observer and the observer is 
     * responsible for freeing the array).
     * 
     * @param aInfo           audio clip info
     * @param aError          <code>KErrNone</code> if visualization was
     *                          completed successfully; one of the system wide
     *                          error codes if generating visualization failed
     * @param aVisualization  pointer to the array containing the visualization values;
     *                        note that the ownership of the array is passed to the
     *                        observer (i.e., the observer is responsible for freeing
     *                        the array); or 0, if generating the visualization failed
     * @param aResolution     resolution of the visualization (i.e., the number of values
     *                        in the visualization array); or 0, if generating the 
     *                        visualization failed
     */
    virtual void NotifyAudioClipVisualizationCompleted(const CVedAudioClipInfo& aInfo, 
                                                       TInt aError, TInt8* aVisualization,
                                                       TInt aResolution) = 0;
    };

    
/**
 * Utility class for getting information about audio clip files.
 *
 */
class CVedAudioClipInfo : public CBase
    {
public:

    /* Constructors & destructor. */

    /**
     * Constructs a new CVedAudioClipInfo object to get information
     * about the specified audio clip file. The specified observer
     * is notified when info is ready for reading. This method
     * may leave if no resources are available to construct 
     * a new object.
     * 
     * Possible leave codes:
     *  - <code>KErrNoMemory</code> if memory allocation fails
     *
     * @param aFileName  name of audio clip file
     * @param aObserver  observer to notify when info is ready for reading
     *
     * @return  pointer to a new CVedAudioClipInfo instance
     */
    IMPORT_C static CVedAudioClipInfo* NewL(const TDesC& aFileName,
                                            MVedAudioClipInfoObserver& aObserver);

    /**
     * Constructs a new CVedAudioClipInfo object to get information
     * about the specified audio clip file. The constructed object
     * is left in the cleanup stack. The specified observer
     * is notified when info is ready for reading. This method
     * may leave if no resources are available to construct a new
     * object.
     * 
     * Possible leave codes:
     *  - <code>KErrNoMemory</code> if memory allocation fails
     *
     * @param aFileName  name of audio clip file
     * @param aObserver  observer to notify when info is ready for reading
     *
     * @return  pointer to a new CVedAudioClipInfo instance
     */
    IMPORT_C static CVedAudioClipInfo* NewLC(const TDesC& aFileName,
                                             MVedAudioClipInfoObserver& aObserver);

    /* Property methods. */

    /**
     * Returns the file name of the clip. Panics if info
     * is not yet ready for reading.
     * 
     * @return  file name
     */
    virtual TPtrC FileName() const = 0;

    /**
     * Returns the audio type of the clip. Panics if info
     * is not yet ready for reading.
     * 
     * @return  audio type
     */
    virtual TVedAudioType Type() const = 0;

    /**
     * Returns the duration of the clip. Panics if info
     * is not yet ready for reading.
     * 
     * @return  duration in microseconds
     */
    virtual TTimeIntervalMicroSeconds Duration() const = 0;

    /**
     * Returns the channel mode of the audio if applicable.
     *
     * @return  channel mode
     */
    virtual TVedAudioChannelMode ChannelMode() const = 0;

    /**
     * Returns the audio format of the clip. Panics if info
     * is not yet ready for reading.
     * 
     * @return  audio format of the clip
     */
    virtual TVedAudioFormat Format() const = 0;

    /**
     * Returns the sampling rate in hertz.
     *
     * @return  sampling rate
     */
    virtual TInt SamplingRate() const = 0;

    /**
     * Returns the bitrate mode.
     *
     * @return  bitrate mode
     */
    virtual TVedBitrateMode BitrateMode() const = 0;

    /**
     * Returns the bitrate.
     *
     * @return  bitrate in bits per second
     */
    virtual TInt Bitrate() const = 0;

    /* Visualization methods. */

    /**
     * Generates a visualization of the audio clip. The visualization consists
     * of an array of values with the specified resolution. This method is 
     * asynchronous. The visualization is generated in background and the observer 
     * is notified when the visualization is complete. This method panics if info 
     * is not yet ready for reading or the resolution is illegal.
     * 
     * Possible leave codes:
     *  - <code>KErrNoMemory</code> if memory allocation fails
     *
     * @param aObserver    observer to be notified when the visualization is completed
     * @param aResolution  resolution of the visualization (i.e., the number of values
     *                     in the visualization array)
     * @param aPriority    priority of the visualization
     */
    virtual void GetVisualizationL(MVedAudioClipVisualizationObserver& aObserver,
                                   TInt aResolution, TInt aPriority) = 0;
    
    /**
     * Cancels visualization generation. If no visualization is currently being 
     * generated, the function does nothing.
     */
    virtual void CancelVisualizationL() = 0;
    
     /**
     * Constructs a new CVedAudioClipInfo object to get information
     * about the specified audio clip file. The specified observer
     * is notified when info is ready for reading. This method
     * may leave if no resources are available to construct 
     * a new object.
     * 
     * Possible leave codes:
     *  - <code>KErrNoMemory</code> if memory allocation fails
     *
     * @param aFileHandle handle of audio clip file
     * @param aObserver  observer to notify when info is ready for reading
     *
     * @return  pointer to a new CVedAudioClipInfo instance
     */
    IMPORT_C static CVedAudioClipInfo* NewL(RFile* aFileHandle,
                                            MVedAudioClipInfoObserver& aObserver);

    /**
     * Constructs a new CVedAudioClipInfo object to get information
     * about the specified audio clip file. The constructed object
     * is left in the cleanup stack. The specified observer
     * is notified when info is ready for reading. This method
     * may leave if no resources are available to construct a new
     * object.
     * 
     * Possible leave codes:
     *  - <code>KErrNoMemory</code> if memory allocation fails
     *     
     * @param aFileHandle handle of audio clip file
     * @param aObserver  observer to notify when info is ready for reading
     *
     * @return  pointer to a new CVedAudioClipInfo instance
     */
    IMPORT_C static CVedAudioClipInfo* NewLC(RFile* aFileHandle,
                                             MVedAudioClipInfoObserver& aObserver);
                                             
     /**
     * Returns the file handle of the clip. Panics if there is no file 
     * associated with this clip or info is not yet ready for reading.
     * 
     * @return  file name of the clip
     */
    virtual RFile* FileHandle() const = 0;
    
    
    };

#endif // __VEDAUDIOCLIPINFO_H__
