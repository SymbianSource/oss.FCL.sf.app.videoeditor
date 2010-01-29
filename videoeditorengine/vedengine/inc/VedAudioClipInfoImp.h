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



#ifndef __VEDAUDIOCLIPINFOIMP_H__
#define __VEDAUDIOCLIPINFOIMP_H__


#include <e32base.h>
#include "VedCommon.h"
#include "VedAudioClipInfo.h"

#include "AudClip.h"
#include "AudObservers.h"

/*
 *  Forward declarations.
 */
class CVedAudioClipInfo;
class CVedAudioClipInfoOperation;

/**
 * Utility class for getting information about audio clip files.
 *
 */
class CVedAudioClipInfoImp : public CVedAudioClipInfo, public MAudClipInfoObserver,
                             public MAudVisualizationObserver
    {
public:
    /* Constuctor */
    CVedAudioClipInfoImp();

    /* Symbian two phased constructor. */
    void ConstructL(const TDesC& aFileName,
                    MVedAudioClipInfoObserver& aObserver);
                    
    /* Symbian two phased constructor. */
    void ConstructL(RFile* aFileHandle,
                    MVedAudioClipInfoObserver& aObserver);

    /* Two-phased constructor variant for use within a movie. */
    void ConstructL(CAudClip* aAudClip,
                    MVedAudioClipInfoObserver& aObserver);

    /**
     * Destroys the object and releases all resources.
     */    
    ~CVedAudioClipInfoImp();


    /* Property methods. */

    /**
     * Returns the file name of the clip. Panics if info
     * is not yet ready for reading.
     * 
     * @return  file name
     */
    TPtrC FileName() const;

    /**
     * Returns the audio format of the clip. Panics if info
     * is not yet ready for reading.
     * 
     * @return  audio format of the clip
     */
    TVedAudioFormat Format() const;

    /**
     * Returns the audio type of the clip. Panics if info
     * is not yet ready for reading.
     * 
     * @return  audio type
     */
    TVedAudioType Type() const;

    /**
     * Returns the duration of the clip. Panics if info
     * is not yet ready for reading.
     * 
     * @return  duration in microseconds
     */
    TTimeIntervalMicroSeconds Duration() const;

    /**
     * Returns the channel mode of the audio if applicable.
     *
     * @return  channel mode
     */
    TVedAudioChannelMode ChannelMode() const;

    /**
     * Returns the sampling rate.
     *
     * @return  sampling rate in hertz
     */
    TInt SamplingRate() const;

    /**
     * Returns the bitrate mode.
     *
     * @return  bitrate mode
     */
    TVedBitrateMode BitrateMode() const;

    /**
     * Returns the bitrate.
     *
     * @return  bitrate in bits per second
     */
    TInt Bitrate() const;


    /**
     * Comparison method for sorting arrays of audio clip infos.
     *
     * @param c1  first info to compare
     * @param c2  second info to compare
     *
     * @return 
     */
    static TInt Compare(const CVedAudioClipInfoImp& c1, const CVedAudioClipInfoImp& c2);

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
    void GetVisualizationL(MVedAudioClipVisualizationObserver& aObserver,
                           TInt aResolution, TInt aPriority);
    
    /**
     * Cancels visualization generation. If no visualization is currently being 
     * generated, the function does nothing.
     */
    void CancelVisualizationL();
    

    /**
     * Factory constructor for constucting the info without creating an audio clip
     * info object, using the supplied audio clip pointer instead.This variant is for
     * use in movie.
     * 
     * @param aAudClip   audio clip instance
     * @param aObserver  observer to notify when clip info is ready
     */
    static CVedAudioClipInfoImp* NewL(CAudClip* aAudClip, MVedAudioClipInfoObserver& aObserver);

    /**
     * Factory constructor for constucting the info without creating an audio clip
     * info object, using the supplied audio clip pointer instead.This variant is for
     * use in movie. Leaves the created instance on the cleanup stack.
     * 
     * @param aAudClip   audio clip instance
     * @param aObserver  observer to notify when clip info is ready
     */
    static CVedAudioClipInfoImp* NewLC(CAudClip* aAudClip, MVedAudioClipInfoObserver& aObserver);
    
    /**
     * Returns the file handle of the clip. Panics if info
     * is not yet ready for reading.
     * 
     * @return  file name
     */
    RFile* FileHandle() const;

private: // Functions from base classes

    /* From base class MAudVisualizationObserver */
    
    void NotifyClipInfoVisualizationCompleted(const CAudClipInfo& aClipInfo, 
        TInt aError, TInt8* aVisualization, TInt aSize);
    
    void NotifyClipInfoVisualizationStarted(const CAudClipInfo& aClipInfo, 
        TInt aError);

    void NotifyClipInfoVisualizationProgressed(const CAudClipInfo& aClipInfo, 
        TInt aPercentage);

    /* From base class MAudClipInfoObserver */
    void NotifyClipInfoReady(CAudClipInfo& aInfo, TInt aError);

private:

    // Audio clip info from Audio Engine
    CAudClipInfo* iAudClipInfo;

    // Audio clip if this clip info is included in a song
    CAudClip* iAudClip;

    // Operation class
    CVedAudioClipInfoOperation* iOperation;

    // Audio properties
    TAudFileProperties iAudioProperties;

    // Whether we are ready to return info.
    TBool iReady;

    MVedAudioClipInfoObserver* iObserver;
    
    // Visualization observer
    MVedAudioClipVisualizationObserver* iVisualizationObserver;
    friend class CVedAudioClip;
    friend class CVedMovieImp;
    };

class CVedAudioClipInfoOperation : public CActive
    {
public:
    static CVedAudioClipInfoOperation* NewL(CVedAudioClipInfoImp* aInfo, MVedAudioClipInfoObserver& aObserver);
    ~CVedAudioClipInfoOperation();

protected:
    TInt RunError(TInt aError);
    void RunL();
    void DoCancel();
    CVedAudioClipInfoOperation(CVedAudioClipInfoImp* aInfo,MVedAudioClipInfoObserver& aObserver);
    void ConstructL();

private:
    MVedAudioClipInfoObserver& iObserver;
    CVedAudioClipInfoImp* iInfo;
    };
#endif // __VEDAUDIOCLIPINFOIMP_H__
