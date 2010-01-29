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


#ifndef __CPROCVISPROCESSOR_H__
#define __CPROCVISPROCESSOR_H__

#include <e32base.h>
#include <e32math.h>
#include "AudClipInfo.h"
#include "AudCommon.h"
#include "AudSong.h"
#include "ProcInFileHandler.h"

#include "ProcFrameHandler.h"

#include "ProcTools.h"

class CProcVisProcessor : public CBase
    {
    
public:

    /*
    * Symbian constructors
    */

    static CProcVisProcessor* NewL();
    static CProcVisProcessor* NewLC();

    ~CProcVisProcessor();

    /**
    * Performs all initializations needed for a clip visualization
    * 
    * Can leave with one of the system wide error codes
    *
    * Possible panic code
    * <code>EVisualizationProcessAlreadyRunning</code>
    *
    * @param aClip        clip to be visualized
    * @param aSize        size of the visualization array (time resolution)
    *
    * @return void
    *
    */
    void VisualizeClipL(const CAudClipInfo* aClipInfo, TInt aSize);
    
    /**
    * Visualizes one piece of clip
    *
    * Possible panic code
    * <code>EVisualizationProcessNotRunning</code>
    *
    * @param aProgress    output parameter to indicate progress in percents
    * @return            ETrue if visualization completed, EFalse otherwise
    *
    */
    TBool VisualizeClipPieceL(TInt &aProgress);

    /**
    * Once visualization process has been completed,
    * visualization array can be retrieved with this function
    * NOTE: This function allocates memory and the caller
    * is responsible for releasing it
    *
    * @param aVisualization    visualization array
    * @param aSize            size of the visualization array
    *
    */
    void GetFinalVisualizationL(TInt8*& aVisualization, TInt& aSize);


private:
    
    // constructL
    void ConstructL();

    // C++ constructor
    CProcVisProcessor();
    
private:

    // visualization size
    TInt iVisualizationSize;
    // array for visualization
    TInt8* iVisualization;
    // infilehandler for clip visualized
    CProcInFileHandler *iInFile;

    // song
    const CAudSong* iSong;

    // clip that is visualized
    const CAudClipInfo* iClipInfo;

    // frame handler for getting gain
    CProcFrameHandler* iFrameHandler;
    
    // how many percents have been written to visualization array
    TInt iVisualizationWritten;
    
    // how many percents have been processed
    TInt iVisualizationProcessed;
    
    // the number of frames altogether
    TInt iFrameAmount;
    
    // how many frames have been processed
    TInt iFramesProcessed;
    
    // current frame being processed
    TInt iVisualizationPos;

    };


#endif
