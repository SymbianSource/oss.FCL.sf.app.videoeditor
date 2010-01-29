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



#ifndef __AUDCLIPINFO_H__
#define __AUDCLIPINFO_H__


#include <e32base.h>
#include <MdaAudioSamplePlayer.h>

#include "AudCommon.h"
#include "AudObservers.h"



/*
*    Forward declarations.
*/
class CAudClipInfoOperation;
class CAudProcessor;

/**
* Utility class for getting information about audio clip files.
*/
class CAudClipInfo : public CBase    

    {
    
public:
    
    /* Constructors & destructor. */
    
    /**
    * Constructs a new CAudClipInfo object to get information
    * about the specified audio clip file. The specified observer
    * is notified when info is ready for reading. This method
    * may leave if no resources are available to construct 
    * a new object.
    * 
    * Possible leave codes:
    *    - <code>KErrNoMemory</code> if memory allocation fails
    *  - <code>KErrNotSupported</code> if the given file format is not supported 
    * or is invalid
    *
    * @param aFileName  name of audio clip file
    * @param aObserver  observer to notify when info is ready for reading
    *
    * @return  pointer to a new CAudClipInfo instance
    */
    IMPORT_C static CAudClipInfo* NewL(const TDesC& aFileName, MAudClipInfoObserver& aObserver);
    
    /**
    * Constructs a new CAudClipInfo object to get information
    * about the specified audio clip file. The constructed object
    * is left in the cleanup stack. The specified observer
    * is notified when info is ready for reading. This method
    * may leave if no resources are available to construct a new
    * object.
    * 
    * Possible leave codes:
    *    - <code>KErrNoMemory</code> if memory allocation fails
    *  - <code>KErrNotSupported</code> if the given file format is not supported 
    *    or is invalid
    *
    * @param aFileName  name of video clip file
    * @param aObserver  observer to notify when info is ready for reading
    *
    * @return  pointer to a new CAudClipInfo instance
    */
    IMPORT_C static CAudClipInfo* NewLC(const TDesC& aFileName, MAudClipInfoObserver& aObserver);
    
    
    /**
    * Returns the properties of this audio file
    * 
    * @return  properties
    */
    IMPORT_C TAudFileProperties Properties() const;
    
    /**
    * Generates a visualization of the current clip. 
    * 
    * Asynchronous operation; MAudVisualizationObserver::NotifyClipVisualizationCompleted
    * is called as soon as the visualization process has completed. This method leads to
    * memory reservation, so once NotifyClipVisualizationCompleted has occurred, 
    * the caller is responsible for memory releasing.
    * 
    * Possible leave codes:
    *    - <code>KErrNoMemory</code> if memory allocation fails
    *
    * @param aObserver            observer to be notified when visualization is completed
    * @param aSize                the size of aVisualization array (time resolution). 
    * @param aPriority            priority
    *
    */
    IMPORT_C void GetVisualizationL(MAudVisualizationObserver& aObserver,
        TInt aSize, TInt aPriority) const;
    
    /**
    * Cancels visualization generation. If no visualization is currently being 
    * generated, the function does nothing.
    */
    IMPORT_C void CancelVisualization();
    
    /**
     * Returns the file name of the clip. Panics if info
     * is not yet ready for reading.
     * 
     * @return  file name
     */
    IMPORT_C TPtrC FileName() const;

    /**
    * Destroys the object and releases all resources.
    */    
    IMPORT_C ~CAudClipInfo();
    
    /**
    * Constructs a new CAudClipInfo object to get information
    * about the specified audio clip file. The specified observer
    * is notified when info is ready for reading. This method
    * may leave if no resources are available to construct 
    * a new object.
    * 
    * Possible leave codes:
    *    - <code>KErrNoMemory</code> if memory allocation fails
    *  - <code>KErrNotSupported</code> if the given file format is not supported 
    * or is invalid
    *
    * @param aFileHandle file handle of audio clip file
    * @param aObserver  observer to notify when info is ready for reading
    *
    * @return  pointer to a new CAudClipInfo instance
    */
    IMPORT_C static CAudClipInfo* NewL(RFile* aFileHandle, MAudClipInfoObserver& aObserver);
    
    /**
    * Constructs a new CAudClipInfo object to get information
    * about the specified audio clip file. The constructed object
    * is left in the cleanup stack. The specified observer
    * is notified when info is ready for reading. This method
    * may leave if no resources are available to construct a new
    * object.
    * 
    * Possible leave codes:
    *    - <code>KErrNoMemory</code> if memory allocation fails
    *  - <code>KErrNotSupported</code> if the given file format is not supported 
    *    or is invalid
    *
    * @param aFileHandle file handle of audio clip file    
    * @param aObserver  observer to notify when info is ready for reading
    *
    * @return  pointer to a new CAudClipInfo instance
    */
    IMPORT_C static CAudClipInfo* NewLC(RFile* aFileHandle, MAudClipInfoObserver& aObserver);
    
    /**
     * Returns the file handle of the clip. Panics if info
     * is not yet ready for reading.
     * 
     * @return  file name
     */
    IMPORT_C RFile* FileHandle() const;
    
    
private:
    
    // C++ constructor
    CAudClipInfo();
    
    // ConstructL
    // parameters: see NewL()
    void ConstructL(const TDesC& aFileName, MAudClipInfoObserver& aObserver);
    
    void ConstructL(RFile* aFileHandle, MAudClipInfoObserver& aObserver);
    
private:

    // properties of this clip
    TAudFileProperties* iProperties;
    
    // file name of this clip
    HBufC* iFileName;
    
    // file handle of this clip
    RFile* iFileHandle;
    
    // flag to indicate whether info class is ready
    TBool iInfoReady;

    // operation class
    CAudClipInfoOperation* iOperation;
    
    // friends
    friend class CAudClipInfoOperation;
    friend class CAudClip;

    
    };



/**
 * Observer for notifying that audio clip info
 * is ready for reading.
 *
 */
class MProcClipInfoObserver
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
     * @param aError  <code>KErrNone</code> if info is ready
     *                for reading; one of the system wide
     *                error codes if reading file failed
     */
    virtual void NotifyClipInfoReady(TInt aError) = 0;
    };



/**
 * Internal class for reading information from the audio clip file.
 * Implements a simple object 
 * to report to the audio clip info observer certain common error conditions 
 * that would otherwise leave.
 */
class CAudClipInfoOperation :    public CBase, 
                                public MProcClipInfoObserver, 
                                public MAudVisualizationObserver
    {
    
public:


    /*
    * Constructor
    *
    *
    * @param aInfo        CAudClipInfo class that owns this object
    * @param aObserver    observer for callbacks
    */

    static CAudClipInfoOperation* NewL(CAudClipInfo* aInfo,
                                            MAudClipInfoObserver& aObserver);

    /*
    * Destructor
    */
    ~CAudClipInfoOperation();
    
    
    // from base class MProcClipInfoObserver
    void NotifyClipInfoReady(TInt aError);

    // from base class MAudVisualizationObserver

    void NotifySongVisualizationCompleted(const CAudSong& aSong, 
        TInt aError, 
        TInt8* aVisualization,
        TInt aSize);

    void NotifyClipInfoVisualizationCompleted(const CAudClipInfo& aClipInfo, 
        TInt aError, 
        TInt8* aVisualization,
        TInt aSize);

    void NotifySongVisualizationStarted(const CAudSong& aSong, 
        TInt aError);

    void NotifyClipInfoVisualizationStarted(const CAudClipInfo& aClipInfo, 
        TInt aError);

    void NotifySongVisualizationProgressed(const CAudSong& aSong, 
        TInt aPercentage);

    void NotifyClipInfoVisualizationProgressed(const CAudClipInfo& aClipInfo, 
        TInt aPercentage);

    /**
    * Generates a visualization of the current clip. 
    * 
    * Asynchronous operation; MAudVisualizationObserver::NotifyClipVisualizationCompleted
    * is called as soon as the visualization process has completed. This method leads to
    * memory reservation, so once NotifyClipVisualizationCompleted has occurred, 
    * the caller is responsible for memory releasing.
    * 
    * Possible leave codes:
    *    - <code>KErrNoMemory</code> if memory allocation fails
    *
    * @param aObserver            observer to be notified when visualization is completed
    * @param aSize                the size of aVisualization array (time resolution).
    * @param aPriority            priority
    *
    */
        
    void StartVisualizationL(MAudVisualizationObserver& aObserver, TInt aSize, TInt aPriority);

    /**
    * Cancels visualization generation. If no visualization is currently being 
    * generated, the function does nothing.
    */
        
    void CancelVisualization();

    /**
     * Called to start getting file properties
     *
     * NotifyClipInfoReady is called as soon as the operation completes
     */
    void StartGetPropertiesL();


private:
    
    CAudClipInfoOperation(CAudClipInfo* aInfo, 
                               MAudClipInfoObserver& aObserver);
    void ConstructL();
    

private:

    // info class that owns this object
    CAudClipInfo* iInfo;
    
    // observer for clip info callbacks
    MAudClipInfoObserver* iObserver;
    
    // observer for visualization callbacks
    MAudVisualizationObserver* iVisualizationObserver;
    
    // processor class owned by this object
    CAudProcessor* iProcessor;

    friend class CAudClipInfo;
    friend class CAudClip;

    };




#endif
