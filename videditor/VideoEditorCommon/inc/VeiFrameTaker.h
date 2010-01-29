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


#ifndef __FRAMETAKER_H
#define __FRAMETAKER_H

#include <vedmovie.h>

class MVeiFrameTakerObserver
	{

public:

	virtual void NotifyFramesCompleted( CFbsBitmap* aFirstFrame, CFbsBitmap* aLastFrame, 
        CFbsBitmap* aTimelineFrame, TInt aError ) = 0;
	};


NONSHARABLE_CLASS( CVeiFrameTaker ):	public CActive,
										public MVedVideoClipFrameObserver

{
public:
    IMPORT_C static CVeiFrameTaker* NewL( MVeiFrameTakerObserver& aObserver );
    ~CVeiFrameTaker();

public:
	IMPORT_C void GetFramesL(CVedVideoClipInfo& aInfo,
							TInt const aFirstFrame, TSize* const aFirstResolution, 
							TInt const aLastFrame, TSize* const aLastResolution,
                            TInt const aTimelineFrame, TSize* const aTimelineResolution,
							TInt aPriority);

private:
	CVeiFrameTaker( MVeiFrameTakerObserver& aObserver );
	void ConstructL();

	void DoCancel();
	void RunL();

	virtual void NotifyVideoClipFrameCompleted(CVedVideoClipInfo& aInfo, 
											   TInt aError, 
							 				   CFbsBitmap* aFrame);
private:
	CActiveSchedulerWait *iWaitScheduler;
	CFbsBitmap*	iFirstFrame;
	CFbsBitmap*	iLastFrame;
    CFbsBitmap*	iTimelineFrame;

	TInt		iFrameCount;
	TInt		iError;

	MVeiFrameTakerObserver& iObserver;
};
#endif
