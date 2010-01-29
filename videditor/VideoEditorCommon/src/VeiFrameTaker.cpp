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


// INCLUDE FILES

// System includes
#include <e32base.h>
#include <vedmovie.h>
#include <fbs.h>
// User includes

#include "VideoEditorDebugUtils.h"
#include "veiframetaker.h"

CVeiFrameTaker::CVeiFrameTaker( MVeiFrameTakerObserver& aObserver ) : CActive(CActive::EPriorityLow), iObserver( aObserver )
	{
	}


CVeiFrameTaker::~CVeiFrameTaker()
	{
	delete iWaitScheduler;
	delete iFirstFrame;
	delete iLastFrame;
    delete iTimelineFrame;
	}



EXPORT_C CVeiFrameTaker* CVeiFrameTaker::NewL( MVeiFrameTakerObserver& aObserver )
	{
	CVeiFrameTaker* self = new (ELeave) CVeiFrameTaker( aObserver );
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
	}

EXPORT_C void CVeiFrameTaker::GetFramesL( CVedVideoClipInfo& aInfo,
									  TInt const aFirstFrame,  TSize* const aFirstResolution,
									  TInt const aLastFrame,  TSize* const aLastResolution,
                                      TInt const aTimelineFrame,  TSize* const aTimelineResolution,
									  TInt aPriority)
	{
	LOG(KVideoEditorLogFile, "CVeiFrameTaker::GetFramesL: In");
	iError = KErrNone;
	iFrameCount = 0;

	delete iFirstFrame;
	iFirstFrame = NULL;
	delete iLastFrame;
	iLastFrame = NULL;
    delete iTimelineFrame;
    iTimelineFrame = NULL;

	LOG(KVideoEditorLogFile, "CVeiFrameTaker::GetFramesL: 1");
	aInfo.GetFrameL( *this, aFirstFrame, aFirstResolution, EColor64K, EFalse, aPriority );
	LOG(KVideoEditorLogFile, "CVeiFrameTaker::GetFramesL: 2");
	
	if ( iFrameCount == 0 )		// GetFrameL is still active, so start waiting
		{		
		LOG(KVideoEditorLogFile, "CVeiFrameTaker::GetFramesL: 3");
		iWaitScheduler->Start();
		LOG(KVideoEditorLogFile, "CVeiFrameTaker::GetFramesL: 4");
		}
	LOG(KVideoEditorLogFile, "CVeiFrameTaker::GetFramesL: 5");
	if ( iError == KErrNone )
		{
		LOG(KVideoEditorLogFile, "CVeiFrameTaker::GetFramesL: 6");
		aInfo.GetFrameL( *this, aLastFrame, aLastResolution, EColor64K, EFalse, aPriority );
		LOG(KVideoEditorLogFile, "CVeiFrameTaker::GetFramesL: 7");
		if ( iFrameCount == 1 )	
			{			
			LOG(KVideoEditorLogFile, "CVeiFrameTaker::GetFramesL: 8");
			iWaitScheduler->Start();
			LOG(KVideoEditorLogFile, "CVeiFrameTaker::GetFramesL: 9");
			}
			LOG(KVideoEditorLogFile, "CVeiFrameTaker::GetFramesL: 10");
		}
	LOG(KVideoEditorLogFile, "CVeiFrameTaker::GetFramesL: 11");	
    if ( iError == KErrNone )
        {
        LOG(KVideoEditorLogFile, "CVeiFrameTaker::GetFramesL: 12");
        aInfo.GetFrameL( *this, aTimelineFrame, aTimelineResolution, EColor64K, EFalse, aPriority );
		LOG(KVideoEditorLogFile, "CVeiFrameTaker::GetFramesL: 13");
        if ( iFrameCount == 2 )	
        	{
        	LOG(KVideoEditorLogFile, "CVeiFrameTaker::GetFramesL: 14");        
			iWaitScheduler->Start();
			LOG(KVideoEditorLogFile, "CVeiFrameTaker::GetFramesL: 15");
        	}        	
        }
	LOG(KVideoEditorLogFile, "CVeiFrameTaker::GetFramesL: 16");
	iObserver.NotifyFramesCompleted( iFirstFrame, iLastFrame, iTimelineFrame, iError );
	LOG(KVideoEditorLogFile, "CVeiFrameTaker::GetFramesL: Out");
	}

void CVeiFrameTaker::ConstructL()
	{
    iWaitScheduler = new (ELeave) CActiveSchedulerWait;
	CActiveScheduler::Add(this);
	}

void CVeiFrameTaker::DoCancel()
    {
    }

void CVeiFrameTaker::RunL()
    {
    iWaitScheduler->AsyncStop();
    }

void CVeiFrameTaker::NotifyVideoClipFrameCompleted(CVedVideoClipInfo& /*aInfo*/, TInt aError, CFbsBitmap* aFrame)
    {
	iError = aError;

	if ( iFrameCount == 0 )
		{
		iFirstFrame = aFrame;
		}
	else if ( iFrameCount == 1 )
		{
		iLastFrame = aFrame;
		}
    else if ( iFrameCount == 2 )
		{
		iTimelineFrame = aFrame;
		}

	iFrameCount++;
// Text frame generator is quite fast, so waitscheduler is not
// started everytime.
	if (iWaitScheduler->IsStarted() )
		iWaitScheduler->AsyncStop();
    }
// End of File
