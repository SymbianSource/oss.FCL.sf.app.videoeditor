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




#ifndef __AUDPROCESSOR_H__
#define __AUDPROCESSOR_H__


#include <e32base.h>
#include <e32std.h>

#include "AudCommon.h"
#include "AudClipInfo.h"
#include "ProcClipInfoAO.h"
#include "ProcProcessAO.h"
#include "ProcVisualizationAO.h"
#include "ProcTimeEstimateAO.h"

/*
 * Forward declarations.
 */
class CAudSong;
class CAudClip;
class MProcProcessObserver;
class MProcClipInfoObserver;
class CProcClipInfoAO;
class CProcProcess;
class CProcVisualizationAO;
class CProcTimeEstimateAO;

/**
 * Audio processor.
 */
class CAudProcessor : public CBase
    {
public:

    /** 
     * Constructors for instantiating new audio processors.
     * Should reserve as little resources as possible at this point.
     */
    static CAudProcessor* NewL();
    static CAudProcessor* NewLC();

    
    virtual ~CAudProcessor();

    /**
    * Starts a syncronous song processing
    */
    TBool StartSyncProcessingL(const CAudSong* aSong);

    TBool ProcessSyncPieceL(HBufC8*& aFrame, TInt& aProgress,
                                       TTimeIntervalMicroSeconds& aDuration);

    /**
     * Cancel song processing.
     */
    void CancelProcessing(MProcProcessObserver& aObserver);



    /**
     * Read the header from the specified audio clip file and return its properties.
     *
     *
     * Possible leave codes:
     *    - <code>KErrNoMemory</code> if memory allocation fails
     *    - <code>KErrNotFound</code> if there is no file with the specified name
     *    in the specified directory (but the directory exists)
     *    - <code>KErrPathNotFound</code> if the specified directory
     *    does not exist
     *    - <code>KErrUnknown</code> if the specified file is of unknown format
     *
     * @param aFileName             name of the file to read
     * @param aFileHandle             handle of the file to read
     * @param aProperties            Output parameter. Properties of the input file. 
     *                                Allocated and released by the caller
     * @param aObserver                Observer to be notified as soon as operation completes
    */
    void GetAudFilePropertiesL(const TDesC& aFileName, RFile* aFileHandle, 
                               TAudFileProperties* aProperties, 
                               MProcClipInfoObserver& aObserver, TInt aPriority);

    /**
    * Starts an asynchronous visualization operation for a clip. Progress
    * indications are sent to an observer
    *
    * @param aClip        Clip to be visualized
    * @param aSize        Lenght of the visualization array, must be positive
    * @param aPriority  Priority of the operation
    */
    void StartGetClipVisualizationL(const CAudClipInfo* aClipInfo, TInt aSize,
        MAudVisualizationObserver& aObserver, 
        TInt aPriority);

    /**
    * Cancels a clip visualization operation. If an operation is not running
    * a <code>TAudPanic::EVisualizationProcessNotRunning<code> is thrown
    */
    void CancelClipVisualization();

    TBool StartTimeEstimateL(const CAudSong* aSong, MAudTimeEstimateObserver& aTEObserver);
    
    void CancelTimeEstimate();
    


protected:
    CAudProcessor();

    void ConstructL();

private:

    // clip info active object owned by this
    CProcClipInfoAO* iClipInfoAO;
    
    // processing object owned by this
    CProcProcess* iProcessingObject;
    
    // visualization active object owned by this
    CProcVisualizationAO* iVisualizationAO;
    
    // Time estimation active object owned by this
    CProcTimeEstimateAO* iTimeEstimateAO;
    
    
    
    // a flag to indicate whether there is some ongoing processing
    TBool iProcessing;

    friend class CProcProcess;
    friend class CProcClipInfoAO;


    };


#endif // __AudProcessor_H__

