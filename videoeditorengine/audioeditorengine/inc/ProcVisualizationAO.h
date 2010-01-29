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



#ifndef __CPROCVISUALIZATIONAO_H__
#define __CPROCVISUALIZATIONAO_H__

#include <e32base.h>
#include "AudSong.h"
#include "AudObservers.h"

#include "AudProcessorImpl.h"

class MProcProcessObserver;
class CProcVisProcessor;

class CProcVisualizationAO : public CActive 
    {

public:

    static CProcVisualizationAO* NewL();

    virtual ~CProcVisualizationAO();
        
    /**
    * Starts a clip visualization operation
    * 
    * Can leave with one of the system wide error codes
    *
    * Possible panic code
    * <code>EVisualizationProcessAlreadyRunning</code>
    *
    * @param aClip        song to be visualized
    * @param aSize        size of the visualization array (time resolution)
    * @param aObserver    observer to be notified of progress
    *
    * @return void
    *
    */
    void StartClipVisualizationL(const CAudClipInfo* aClipInfo, TInt aSize, MAudVisualizationObserver& aObserver, TInt aPriority);
    
    /**
    * Cancels a visualization operation
    * 
    * Possible panic code
    * <code>EVisualizationProcessNotRunning</code>
    *
    */    
    void CancelVisualization();
    
    /**
    * Enumeration that represents the state of this object
    */
    enum TVisualizationState 
        {
        EProcGettingClipVisualization = 100,
        EProcVisualizationIdle
        };

protected:
    virtual void RunL();
    virtual void DoCancel();

private:
    
    void ConstructL();

private:
    
    CProcVisualizationAO();

    // visualization observer 
    MAudVisualizationObserver* iObserver;
    
    // visualization processor
    CProcVisProcessor* iProcVisProcessor;
    // visualization state
    TVisualizationState iVisualizationState;
    
    // clipinfo visualized
    const CAudClipInfo* iClipInfo;
    
    // visualization size
    TInt iSize;
    
    // previous progress value sent to the observer
    TInt iPreviousProgressValue;
    };


#endif
