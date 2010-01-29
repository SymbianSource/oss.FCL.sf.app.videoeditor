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
* Definition for CMPEG4Timer.
*
*/




#ifndef     __MPEG4TIMER_H__
#define     __MPEG4TIMER_H__

/* 
* Includes
*/

#include <e32base.h>
#include <gdi.h>
#include <e32std.h>
#include "vedcommon.h"
#include "movieprocessorimpl.h"

/* 
*  Class Declarations
*/

class CMPEG4Timer: public CBase
{
public:

	 /**
	 * Public member functions 
	 */

	 /**
	 * C++ default constructor
	 */
	CMPEG4Timer() {};

	/** 
	 * Destructor can be called at any time (i.e., also in the middle of a processing operation)
	 * Should release all allocated resources, including releasing all allocated memory and 
	 * *deleting* all output files that are currently being processed but not yet completed.
	 */
	~CMPEG4Timer();

	 /** 
	 * Constructors for instantiating new video processors.
	 * Should reserve as little resources as possible at this point.
	 */
	static CMPEG4Timer * NewL(CMovieProcessorImpl * aMovProcessor, TInt aTimeIncrementResolution);

	 /** 
     * Get the frame duration in millisec from the last frame with modulo base larger than zero 
     *          
     * @return Duration in millisec
	 *
     */
	TInt64 GetMPEG4DurationInMsSinceLastModulo();

	 /** 
     * Update the time stamp and duration of the last frame for MPEG-4 video 
     *          
	 * @param aAbsFrameNumber        frame number in the movie
	 * @param aFrameNumber           frame number in the current video clip
	 * @param aTimeScale             time scale 
	 *
     */
	void UpdateMPEG4Time(TInt aAbsFrameNumber, TInt aFrameNumber, TInt aTimeScale);

	 /** 
     * Pointer to iMPEG4TimeStamp object (contains MPEG-4 frame timing information)
     *          
     */
	tMPEG4TimeParameter * GetMPEG4TimeStampPtr() { return &iMPEG4TimeStamp; }

	 /** 
     * Pointer to iMPEG4TimeResolution object (contains MPEG-4 frame time resolution information)
     *          
     */
	TInt * GetMPEG4TimeResolutionPtr() { return &iMPEG4TimeResolution; }


private:

	/* 
	 * Private member functions 
	 */

	 /** 
     * Symbian OS C++ style constructor 
	 *
     */
	void ConstructL(CMovieProcessorImpl * aMovProcessor, TInt aTimeIncrementResolution);

	/* 
	 * Member variables 
	 */
	
	/* frame duration in millisec from the last frame with modulo base larger than zero */
	TInt iMPEG4DurationInMsSinceLastModulo;

	/* structure for timing information of MPEG-4 frame */
	tMPEG4TimeParameter iMPEG4TimeStamp;

	/* time resolution of MPEG-4 video clip */
	TInt iMPEG4TimeResolution;

	/* video processor object */
	CMovieProcessorImpl * iProcessor;	

	TInt iPrevModuloTimeBaseVal;

};

#endif      /* __TRANSCODER_H__ */
            
/* End of File */
