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
* DevVideoClient observer.
*
*/


#ifndef CTRDEVVICEOCLIENTOBSERVER_H
#define CTRDEVVICEOCLIENTOBSERVER_H


// INCLUDES
#include <devvideobase.h>
#include <devvideorecord.h>
#include <CCMRMediaSink.h>


/**
*  Devvideo client observer
*  @lib TRANSCODER.LIB
*  @since 3.1
*/
class MTRDevVideoClientObserver
    {
    public:
        /**
        * Reports an error from the devVideo client
        * @param aError Error reason
        * @return void
        */
        virtual void MtrdvcoFatalError(TInt aError) = 0;

        /**
        * Reports that data encoding process has been finished
        * @param none
        * @return void
        */
        virtual void MtrdvcoEncStreamEnd() = 0;

        /**
        * Reports that data decoding process has been finished
        * @param none
        * @return void
        */
        virtual void MtrdvcoDecStreamEnd() = 0;

        /**
        * Returns video picture from the video encoder client
        * @param aPicture video picture
        * @return void
        */
        virtual void MtrdvcoEncoderReturnPicture(TVideoPicture* aPicture) = 0;

        /**
        * Returns videopicture from the renderer
        * @param aPicture Video picture
        * @return none
        */
        virtual void MtrdvcoRendererReturnPicture(TVideoPicture* aPicture) = 0;

        /**
        * Supplies new decoded picture
        * @param aPicture Video picture
        * @return none
        */
        virtual void MtrdvcoNewPicture(TVideoPicture* aPicture) = 0;

        /**
        * Supplies new encoded bitstream buffer
        * @param aBuffer Media buffer
        * @return none
        */
        virtual void MtrdvcoNewBuffer(CCMRMediaBuffer* aBuffer) = 0;

        /**
        * Informs about initializing video encoder client
        * @param aError Initializing error status 
        * @return void
        */
        virtual void MtrdvcoEncInitializeComplete(TInt aError) = 0;

        /**
        * Informs about initializing video decoder client
        * @param aError Initializing error status 
        * @return void
        */
        virtual void MtrdvcoDecInitializeComplete(TInt aError) = 0;

        /**
        * Returns media bitstream buffer to the client
        * @param aBuffer Bitstream media buffer
        * @return void
        */
        virtual void MtrdvcoReturnCodedBuffer(CCMRMediaBuffer* aBuffer) = 0;
        
        /**
        * Notifies the transcoder about available picture buffers through BMCI or MDF
        * @param none
        * @return void
        */
        virtual void MtrdvcoNewBuffers() = 0;
        
        /**
        * Indicates that a media device has lost its resources
        * @param aFromDecoder Flag to indicate source
        * @return none
        */
        virtual void MtrdvcoResourcesLost(TBool aFromDecoder) = 0;
        
        /**
        * Indicates that a media device has regained its resources
        * @return none
        */
        virtual void MtrdvcoResourcesRestored() = 0;
    };




#endif // CTRDEVVICEOCLIENTOBSERVER_H
