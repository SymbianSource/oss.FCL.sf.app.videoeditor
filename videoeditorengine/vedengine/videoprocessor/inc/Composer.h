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



#ifndef __COMPOSER_H__
#define __COMPOSER_H__

//  INCLUDES
#ifndef __E32BASE_H__
#include <e32base.h>
#endif

// CONSTANTS
// MACROS
// DATA TYPES
// FUNCTION PROTOTYPES
// FORWARD DECLARATIONS


// CLASS DECLARATION

/**
*  Base class for file format parser
*  ?other_description_lines
*/
class CComposer : public CBase
    {

    public: // Constants

        enum TErrorCode 
        {
            EInternalAssertionFailure   = -2200,
            EComposerNotEnoughData = -2201, //FC Can this happen?
            EComposerEndOfStream = -2202,//FC Can this happen?
            EComposerBufferTooSmall = -2203,//FC Can this happen?
            EComposerUnsupportedFormat = -2204,//FC Can this happen?
            EComposerStreamCorrupted = -2205,//FC Can this happen?
            EComposerFailure = -2206
        };

		// file format
		enum TFileFormat {
			EFileFormatUnrecognized = 0,
		    EFileFormat3GP,
			EFileFormatMP4
		};

        // video format
        enum TVideoFormat {
            EVideoFormatNone = 0,
            EVideoFormatH263Profile0Level10,
			EVideoFormatH263Profile0Level45,
            EVideoFormatMPEG4,
            EVideoFormatAVCProfileBaseline
        };

        // audio format
        enum TAudioFormat
        {
            EAudioFormatNone = 0,
            EAudioFormatAMR,
			EAudioFormatAAC
        };               

    public: // Data structures

        // common stream parameters
        struct TStreamParameters
        {
            TBool iHaveVideo;  // is there video in the stream ?
            TBool iHaveAudio;  // is there audio in the stream ?
            TUint iNumDemuxChannels;  // number of demux channels
			TFileFormat iFileFormat;  // file format
            TVideoFormat iVideoFormat;  // video format
            TAudioFormat iAudioFormat;  // audio format

            TUint iAudioFramesInSample; // audio frames per one sample (3GPP)

            TUint iVideoWidth;  // width of a video frame
            TUint iVideoHeight;   // height of a video frame
            TInt64 iVideoPicturePeriodNsec;  // one PCF tick period in nanoseconds
            TUint iVideoIntraFrequency;  // intra frame frequency in stream

            TUint iStreamLength;  // stream length in milliseconds
            TUint iVideoLength;
            TUint iAudioLength;

            TBool iCanSeek;   // TRUE if seeking in file is possible

            TUint iStreamSize;  // stream size in bytes
            TUint iStreamBitrate;  // stream average bitrate

            TUint iMaxPacketSize; // The maximum media packet size
            TUint iLogicalChannelNumberVideo; // Logical channel number for video data
            TUint iLogicalChannelNumberAudio; // Logical channel number for audio data
            TUint iReferencePicturesNeeded; // Number of reference pictures
                                            // the video decoder needs to store.
            TUint iNumScalabilityLayers; // The number of different scalability layers used
            TUint iLayerFrameRates[8];  // Picture rate for each layer
            
            TReal iFrameRate;
            TInt iVideoTimeScale;
            TInt iAudioTimeScale;

        };

    public:  // Constructors and destructor

    public: // New functions

        /**
        * Composer the stream header.
        * @param aStreamParameters Stream parameters
        */	        
        virtual void ComposeHeaderL(CComposer::TStreamParameters& aStreamParameters, 
            TInt aOutputVideoTimeScale, TInt aOutputAudioTimeScale, TInt aAudioFramesInSample) = 0;		

        virtual TInt Close() = 0;
                                
        virtual TInt WriteFrames(TDesC8& aSrcBuffer, TInt aFrameSize,TInt aDuration,
            TInt aKeyFrame,TInt aNumberOfFrames,TInt aFrameType) = 0;

		virtual TInt WriteFrames(TDesC8& aSrcBuffer, TInt aFrameSize,TInt aDuration,
            TInt aKeyFrame,TInt aNumberOfFrames,TInt aFrameType, TInt& aMP4Size,
			TBool aModeChanged,TBool aFirstFrameOfClip,TInt aMode, TBool aFromEncoder) = 0;

        virtual TUint8* GetComposedBuffer() = 0;  //Added for buffer support

		virtual TUint GetComposedBufferSize() = 0;	//Added for buffer support

        /**
        * Calculate free space on the target output drive in bytes.
        *
        * @param None
        * @return Free space in bytes
        */
        virtual TInt64 DriveFreeSpaceL() = 0;


    };

#endif      // __PARSER_H__

// End of File
