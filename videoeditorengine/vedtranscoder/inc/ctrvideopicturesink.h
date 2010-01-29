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


#ifndef CTRVIDEOPICTURESINK_H
#define CTRVIDEOPICTURESINK_H


// INCLUDES
#include "ctrcommon.h"


/**
*  Transcoder observer interface class. Every client should implement this class. 
*  @lib TRANSCODER.LIB
*  @since 3.1
*/
class MTRVideoPictureSink
    {
    public:
        /**
        * Sends videopicture to the client (if retrieve intermediate content was called) / returns encoded picture back
        * @param TTRVideoPicture Uncompressed videopicture
        * @return void
        */
        virtual void MtroPictureFromTranscoder(TTRVideoPicture* aPicture) = 0;
    };


#endif  // CTRVIDEOPICTURESINK_H
