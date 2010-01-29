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
* Implementation for MPEG-4 timing functions.
*
*/


/* 
* Includes
*/

#include "mpeg4timer.h"
#include "vedvideosettings.h"

// Debug print macro
#ifdef _DEBUG
#include <e32svr.h>
#define PRINT(x) RDebug::Print x
#else
#define PRINT(x)
#endif


/*
* ~CMPEG4Timer
*
* Parameters: 
*
* Function:
*    Destruction
* Returns:
*
* Error codes:
*    None
*
*/
CMPEG4Timer::~CMPEG4Timer()
{
}

/*
* NewL
*
* Parameters: 
*     
* Function:
*    Symbian two-phased constructor 
* Returns:
*     pointer to constructed object, or NULL
* Error codes:
*    None
*
*/
CMPEG4Timer* CMPEG4Timer::NewL(CMovieProcessorImpl * aMovProcessor, TInt aTimeIncrementResolution)
{	
	CMPEG4Timer* self = new (ELeave) CMPEG4Timer();    
	CleanupStack::PushL( self );
	self->ConstructL(aMovProcessor, aTimeIncrementResolution);
	CleanupStack::Pop();
	return self; 
}

/*
* ConstructL
*
* Parameters: 
*     
* Function:
*    Symbian 2nd phase constructor (can leave)
* Returns:
*     None
* Error codes:
*    None
*
*/
void CMPEG4Timer::ConstructL(CMovieProcessorImpl * aMovProcessor, TInt aTimeIncrementResolution)
{	
	iProcessor = aMovProcessor;
	iMPEG4TimeStamp.modulo_time_base = 0;
	iMPEG4TimeStamp.time_inc = 0;
	iPrevModuloTimeBaseVal = 0;
	iMPEG4TimeResolution = aTimeIncrementResolution;
	iMPEG4DurationInMsSinceLastModulo = 0;
}



/*
* GetMPEG4DurationInMsSinceLastModulo
*
* Parameters: 
*     
* Function:
*    This function gets the frame duration in millisec from the last frame with modulo base larger than zero
* Returns:
*     Frame duration 
* Error codes:
*    None
*
*/
TInt64 CMPEG4Timer::GetMPEG4DurationInMsSinceLastModulo()
{
	return iMPEG4DurationInMsSinceLastModulo;
}

/*
* UpdateMPEG4Time
*
* Parameters: 
*     
* Function:
*    This function updates the time stamp and duration of the last frame for MPEG-4 video
* Returns:
*     Nothing 
* Error codes:
*    None
*
*/
void CMPEG4Timer::UpdateMPEG4Time(TInt aAbsFrameNumber, TInt /*aFrameNumber*/, TInt aTimeScale)
{
	TInt cur = aAbsFrameNumber; 
	TInt next = cur+1;
	TInt64 frameDuration;
	int Tdiff;

	iPrevModuloTimeBaseVal += iMPEG4TimeStamp.modulo_time_base;
	
	iMPEG4DurationInMsSinceLastModulo = (TInt)((TReal)(iPrevModuloTimeBaseVal * iMPEG4TimeResolution + iMPEG4TimeStamp.time_inc)/ 
		(TReal)(iMPEG4TimeResolution) * 1000000.0 + 0.5);	
	
	if(next >= iProcessor->GetOutputNumberOfFrames())
		frameDuration = iProcessor->GetVideoTimeInMsFromTicks( I64INT( (iProcessor->GetVideoClipDuration() - iProcessor->VideoFrameTimeStamp(cur)) ), EFalse)*1000; 
	else
		frameDuration = iProcessor->GetVideoTimeInMsFromTicks(iProcessor->VideoFrameTimeStamp(next) - iProcessor->VideoFrameTimeStamp(cur), EFalse)*1000; 
	
 	if (frameDuration <0 )
		frameDuration = 100000;
	
	frameDuration = TInt( I64REAL(frameDuration) / (TReal)aTimeScale * 1000.0 + 0.5);
	
    if ( I64INT(frameDuration) > KVedMaxFrameDuration )
        {
        // max duration is limited since there are some variables e.g. in video decoder than can handle only limited length fields. 
        PRINT((_L("CMPEG4Timer::UpdateMPEG4Time() limiting frame duration to 30 sec")));
        frameDuration = KVedMaxFrameDuration;
        }
	Tdiff = TInt(iMPEG4TimeStamp.time_inc + I64REAL(frameDuration) * iMPEG4TimeResolution/1000000.0 + 0.5);
	iMPEG4TimeStamp.modulo_time_base = Tdiff/iMPEG4TimeResolution;
	iMPEG4TimeStamp.time_inc = Tdiff - (iMPEG4TimeResolution * iMPEG4TimeStamp.modulo_time_base);

}

