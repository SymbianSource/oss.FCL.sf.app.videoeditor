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


/* Copyright (c) 2004, Nokia. All rights reserved. */

#ifndef __VEDVIDEOCLIPINFOGENERATEDIMP_H__
#define __VEDVIDEOCLIPINFOGENERATEDIMP_H__


#include <e32base.h>

#include "VedCommon.h"
#include "VedVideoClipInfo.h"
#include "VedVideoClipGenerator.h"

/*
 *  Forward declarations.
 */
class CVedVideoClipInfoGeneratedOperation;
class CVedVideoClipGeneratedFrameToFrameAdapter;

/**
 * Utility class for getting information about generated video clips.
 */
class CVedVideoClipInfoGeneratedImp : public CVedVideoClipInfo
    {
public:
    CVedVideoClipInfoGeneratedImp(CVedVideoClipGenerator& aGenerator, TBool aOwnsGenerator);

    void ConstructL(MVedVideoClipInfoObserver& aObserver);

    /**
     * Destroys the object and releases all resources.
     */    
    ~CVedVideoClipInfoGeneratedImp();


    /* General property methods. */

    TPtrC DescriptiveName() const;

    TPtrC FileName() const;
    
    RFile* FileHandle() const;

    CVedVideoClipGenerator* Generator() const;

    TVedVideoClipClass Class() const;

    TVedVideoFormat Format() const;

    TVedVideoType VideoType() const;

    TSize Resolution() const;

    TBool HasAudio() const;

    TVedAudioType AudioType() const;

    TVedAudioChannelMode AudioChannelMode() const;

    TInt AudioSamplingRate() const;

    TTimeIntervalMicroSeconds Duration() const;


    /* Video frame property methods. */

    TInt VideoFrameCount() const;

    TTimeIntervalMicroSeconds VideoFrameStartTimeL(TInt aIndex);

    TTimeIntervalMicroSeconds VideoFrameEndTimeL(TInt aIndex);

    TTimeIntervalMicroSeconds VideoFrameDurationL(TInt aIndex);

    TInt VideoFrameSizeL(TInt aIndex);

    TBool VideoFrameIsIntraL(TInt aIndex);

    TInt GetVideoFrameIndexL(TTimeIntervalMicroSeconds aTime);


    void SetTranscodeFactor(TVedTranscodeFactor aFactor);

    TVedTranscodeFactor TranscodeFactor();

    TBool IsMMSCompatible();

    /* Frame methods. */

    void GetFrameL(MVedVideoClipFrameObserver& aObserver,
                            TInt aIndex,
                            TSize* const aResolution,
                            TDisplayMode aDisplayMode,
                            TBool aEnhance,
                            TInt aPriority);
    
    void CancelFrame();

private:
    // Member variables

    // Get audio info operation.
    CVedVideoClipInfoGeneratedOperation* iInfoOperation;
    // Flag to indicate then info is available
    TBool iReady;
    // Frame generator
    CVedVideoClipGenerator* iGenerator;
    TBool iOwnsGenerator;

    CVedVideoClipGeneratedFrameToFrameAdapter* iAdapter;
        
    TVedTranscodeFactor iTFactor; 
    friend class CVedVideoClipInfoGeneratedOperation;
    };



class CVedVideoClipGeneratedFrameToFrameAdapter : public CBase, public MVedVideoClipGeneratorFrameObserver
    {
public:
    CVedVideoClipGeneratedFrameToFrameAdapter(CVedVideoClipInfo& aInfo);
    void NotifyVideoClipGeneratorFrameCompleted(CVedVideoClipGenerator& aInfo, 
                                                        TInt aError, 
                                                        CFbsBitmap* aFrame);
private:
    MVedVideoClipFrameObserver* iFrameObserver;
    CVedVideoClipInfo& iInfo;

    friend class CVedVideoClipInfoGeneratedImp;
    };


/**
 * Internal class for asynchronous construction of info class.
 */
class CVedVideoClipInfoGeneratedOperation : public CActive
    {
public:
    /* Static constructor */
    static CVedVideoClipInfoGeneratedOperation* NewL(CVedVideoClipInfoGeneratedImp* aInfo,
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
    CVedVideoClipInfoGeneratedOperation(CVedVideoClipInfoGeneratedImp* aInfo, 
                                        MVedVideoClipInfoObserver& aObserver);
    /* Standard Symbian OS two phased constructor */
    void ConstructL();
    /* Destructor */
    ~CVedVideoClipInfoGeneratedOperation();

private:
    // Class to contain video clip info.
    CVedVideoClipInfoGeneratedImp* iInfo;
    // Observer of video clip info operation.
    MVedVideoClipInfoObserver* iObserver;

    friend class CVedVideoClipInfoGeneratedImp;
    };


#endif // __VEDVIDEOCLIPINFOGENERATEDIMP_H__

