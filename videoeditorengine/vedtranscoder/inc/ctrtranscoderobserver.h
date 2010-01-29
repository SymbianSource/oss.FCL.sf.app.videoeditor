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
* Transcoder observer.
*
*/


#ifndef CTRTRANSCODEROBSERVER_H
#define CTRTRANSCODEROBSERVER_H


// INCLUDES
#include <CCMRMediaSink.h>
#include "ctrcommon.h"


/**
*  Transcoder observer interface class. Every client should implement this class. 
*  @lib TRANSCODER.LIB
*  @since 3.1
*/
class MTRTranscoderObserver
    {
    public:
        /**
        * Reports initialize status to the client
        * @param aError Error status
        * @return void
        */
        virtual void MtroInitializeComplete(TInt aError) = 0;

        /**
        * Reports run-time error to the client
        * @param aError Run-time error
        * @return void
        */
        virtual void MtroFatalError(TInt aError) = 0;

        /**
        * Returns media bitstream buffer to the client
        * @param aBuffer Bitstream media buffer
        * @return void
        */
        virtual void MtroReturnCodedBuffer(CCMRMediaBuffer* aBuffer) = 0;

        /**
        * Request to the client to set FrameRate of the input sequence.
        * @param aRate Framerate of the input sequence (in frames per second)
        * @return void
        */
        virtual void MtroSetInputFrameRate(TReal& aRate) = 0;
        
        /**
        * Completes async request
        * @param none
        * @return void
        */
        virtual void MtroAsyncStopComplete() = 0;
        
        /**
        * Notifies that resources were lost and transcoding has to be suspended
        * @param none
        * @return void
        */
        virtual void MtroSuspend() = 0;
        
        /**
        * Notifies that resources were restored and transcoding can now continue
        * @param none
        * @return void
        */
        virtual void MtroResume() = 0;
    };



#endif
