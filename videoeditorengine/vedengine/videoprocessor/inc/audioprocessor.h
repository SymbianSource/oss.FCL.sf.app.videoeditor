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
* Audio processor class.
*
*/


#ifndef __AUDIOPROCESSOR_H__
#define __AUDIOPROCESSOR_H__

#ifndef __E32BASE_H__
#include <e32base.h>
#endif

class CMovieProcessorImpl;
class CAudSong;

/**
 * Audio processor.
 */
class CAudioProcessor : public CActive
	{
public:  // Constructors and destructor

    /**
    * Two-phased constructor
    */

    static CAudioProcessor* NewL(CMovieProcessorImpl *aMovieProcessor, CAudSong* aSong);
    static CAudioProcessor* NewLC(CMovieProcessorImpl *aMovieProcessor, CAudSong* aSong);       

    /**
    * Destructor.
    */

    virtual ~CAudioProcessor();
	
public:  // Functions from base classes

    /**
    * From CActive Active object cancelling method
    */
    void DoCancel();

    /**
    * From CActive Active object running method
    */
    void RunL();

    /**
    * From CActive Active object error method
    */
    TInt RunError(TInt aError);

public:  // New functions

    /**
    * Starts audio processing    
    */      
    void StartL();

    /**
    * Stops audio processing    
    */          
    void StopL();    
    
private:

    /**
    * Constructor
    */
    CAudioProcessor(CMovieProcessorImpl *aMovieProcessor, CAudSong* aSong);

    /**
    * By default EPOC constructor is private.
    */
    void ConstructL();
    
    /**
    * Process a number of audio frames
    */
    void ProcessFramesL();
    
private:  // Data

    // movie processor instance
    CMovieProcessorImpl* iMovieProcessor; 

    // song
    CAudSong* iSong;
        
    TBool iProcessing;

    };
    

#endif

// End of file
