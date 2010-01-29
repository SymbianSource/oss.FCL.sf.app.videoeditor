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
*
*/
/*
* ============================================================================
*  Name     : Mp4demux.h
*  Part of  : Videoplayer
*  Origin   : 
*  Created  : Mon Sep 10 13:48:59 2001 by jykokkon
*  Description:
*     
*  Version  :
*   %version: 3 %, %date_modified: Wed Jan 28 15:16:13 2009 % by %derived_by: mkarimak % 
* 
*  ---------------------------------------------------------------------------
*  Version history:
*  Template version: 1.02, 21.6.2000 by maparnan
*  <ccm_history>
*
*  Version: 1, Mon Sep 10 13:49:00 2001 by jykokkon
*  Ref: ?reference_to_reason_doc_if_any (e.g. ActionDb Id)
*  ?change_reason_comment
*
*  </ccm_history>
* ============================================================================
*/

#ifndef     __MP4DEMUX_H__
#define     __MP4DEMUX_H__

//  INCLUDES

#ifndef __DEMULTIPLEXER_H__
#include "demultiplexer.h"
#endif

//  FORWARD DECLARATIONS

class CActiveQueue;
class CStatusMonitor;
class CMP4Parser;

//  CLASS DEFINITIONS

class CMP4Demux : public CDemultiplexer
    {    
    public: // Constants
    
        enum TErrorCode
        {
            EInternalAssertionFailure = -2300
        };
        
        // The type of data in an output channel
        enum TChannelDataType
        {
            EDataNone = 0,
            EDataAudio,
            EDataVideo
        };

    public: // Data structures
     
        // Demux parameters
        struct TStreamParameters
        {
            TUint iPicturePeriodMs;
            TUint iAudioFramesInSample;
        };

        // One output channel
        struct TOutputChannel
        {        
            TChannelDataType iDataType;
            CActiveQueue *iTargetQueue; 
        };
        
        
    public: // Constructors and destructor
            
        /**
        * Two-phased constructor.
        */

        static CMP4Demux* NewL(CActiveQueue *anInputQueue,              
                               TUint aNumChannels, TOutputChannel *aOutputChannels,
                               TStreamParameters *aParameters,
                               CStatusMonitor *aStatusMonitor,
                               CMP4Parser *aParser,                  
                               TInt aPriority=EPriorityStandard);
            
        /**
        * Destructor.
        */
        ~CMP4Demux();
            
    public: // New functions

        /**
        * Read a number of frames to video queue
        *
        * @param aCount Number of frames to read
        * @return Error code
        */
        TInt ReadVideoFrames(TInt aCount);
        
    public: // Functions from base classes
        
        /**
        * From CDemultiplexer Start demultiplexing        
        */
        void Start();
        
        /**
        * From CDemultiplexer Stop demultiplexing        
        */
        void Stop();
        
        /**
        * From CDataProcessor Standard active object running method
        */
        void RunL();
        
        /**
        * From CDataProcessor Called by the input queue object when 
        * the input stream has ended
        * @param aUserPointer user data pointer
        */
        void StreamEndReached(TAny *aUserPointer);
        
        /**
        * From CDataProcessor Cancels any asynchronous requests pending    
        */
        void DoCancel();
        
    private: // Internal methods

        /**
        * C++ default constructor.
        */
        CMP4Demux(CActiveQueue *anInputQueue,              
                  TUint aNumChannels, TOutputChannel *aOutputChannels,
                  TStreamParameters *aParameters,
                  CStatusMonitor *aStatusMonitor,
                  CMP4Parser *aParser,                  
                  TInt aPriority);    

        /**
        * Second-phase constructor
        */
        void ConstructL();
        
        /**
        * Select audio/video frame to be demuxed next 
        * (used when stream is in a local file)
        */
        void SetFrameType();
        
        /**
        * Get number of free blocks in target queue        
        * @return TUint number of blocks
        */
        TUint NumFreeBlocks();
        
        /**
        * Get information about next frame from parser        
        * @return TInt error code
        */
        TInt GetFrameInfo();
        
        /**
        * Read next frame(s) from parser & send them to 
        * decoder        
        * @return TInt error code
        */
        TInt ReadAndSendFrames();    
        
    private: // Data
        CStatusMonitor *iMonitor; // status monitor object
        CActiveQueue *iInputQueue; // input data queue
        CMP4Parser *iParser; // MP4 format parser object
        
        TUint iNumOutputChannels; // the number of output channels used
        TOutputChannel *iOutputChannels; // the output channels
                
        TBool iGotFrame;       // TRUE if a frame is available

        TUint iBytesDemuxed;  // bytes demuxed during current run
                
        TOutputChannel *iVideoChannel; // the channel used for output space checks
        TOutputChannel *iAudioChannel; // the channel used for output space checks               

        TUint iAudioFramesInSample; // number of audio frames in MP4 sample
        TUint iPicturePeriodMs;  // average coded picture period
        
        TBool iDemultiplexing; // are we demuxing?
        
        TUint iFrameLen; // length of next frame to be retrieved 
        TChannelDataType iFrameType; // type of next frame to be retrieved
        
        TBool iAudioEnd;
        TBool iVideoEnd;
        TBool iStreamEnd; // has the stream end been reached?
        TBool iStreamEndDemuxed; // have we demultiplexed everything up to strem end?
        TBool iReaderSet; // have we been set as a reader to the input queue?
        TBool iWriterSet; // have we been set as a writer to the output queues?
        
        TPtr8 *iInputBlock; // current input queue block                
        
    };
    
#endif      //  __MP4DEMUX_H__
    
// End of File
