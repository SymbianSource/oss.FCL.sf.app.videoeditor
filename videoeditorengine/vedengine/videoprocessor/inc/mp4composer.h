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


//FC: A wrapper class for mp4composer
// Based on the CMP4Parser class
// end FC.


#ifndef __MP4COMPOSER_H__
#define __MP4COMPOSER_H__

//  INCLUDES


#include "Composer.h"
#include <mp4lib.h>
#include "parser.h"
#include "vedavcedit.h"

// CONSTANTS
// MACROS
// DATA TYPES
// FUNCTION PROTOTYPES
// FORWARD DECLARATIONS


// CLASS DECLARATION


/**
*  MP4 -format Composer class
*  ?other_description_lines
*/
class CMP4Composer : public CComposer
    {

    public: // Constants

        enum TFrameType
        {
            EFrameTypeNone = 0,
            EFrameTypeAudio,
            EFrameTypeVideo
        };

    public:  // Constructors and destructor

        /**
        * C++ default constructor.
        */
        CMP4Composer();

		/**
        * Two-phased constructor.(overloaded for Mp4 support)
        */
        static CMP4Composer* NewL(const TDesC &aFileName, 
                                  CParser::TVideoFormat aVideoFormat,
                                  CParser::TAudioFormat aAudioFormat,
                                  CVedAVCEdit *aAvcEdit);
                                  
        static CMP4Composer* NewL(RFile* aFileHandle, 
                                  CParser::TVideoFormat aVideoFormat,
                                  CParser::TAudioFormat aAudioFormat,
                                  CVedAVCEdit *aAvcEdit);

        /**
        * Destructor.
        */
        ~CMP4Composer();
    
    public: // Functions from base classes     

                
		/**
        * Write a number of frames of requested type from inserted data or file
        * @param aSrcBuffer SOurce buffer
        * @param aType Type of frame(s) to write
        * @param aNumWritten Number of frames actually written
        * @return TInt error code
        */

        TInt WriteFrames(TDesC8& aSrcBuffer, TInt aFrameSize,TInt aDuration, 
					TInt aKeyFrame,TInt aNumberOfFrames,TInt aFrameType);               

		TInt WriteFrames(TDesC8& aSrcBuffer, TInt aFrameSize,TInt aDuration, 
					TInt aKeyFrame,TInt aNumberOfFrames,TInt aFrameType,TInt& aMP4Size,
					TBool aModeChanged,TBool aFirstFrameOfClip,TInt aMode, TBool aFromEncoder);

        /**
        * From CComposer composes the stream header.
        * @param aStreamParameters Common stream parameters
        * @param aOutputVideoTimeScale Video time scale for output file
        * @param aOutputAudioTimeScale Audio time scale for output file
        * @param aAudioFramesInSample Number of audio frames in sample
        */
		void ComposeHeaderL(CComposer::TStreamParameters& aStreamParameters,
			TInt aOutputVideoTimeScale, TInt aOutputAudioTimeScale, TInt aAudioFramesInSample);		

        /**
        * From CComposer Closes the composer instance
        */
        TInt Close();

        /**
        * From CComposer Calculate drive free space 
        */
        TInt64 DriveFreeSpaceL();

        /**
        * Get the Composed Buffer
        */
		TUint8* GetComposedBuffer();

		/**
        * Get the Composed Buffer size till now
        */
		TUint GetComposedBufferSize();

		TInt GetMp4SpecificSize(TDesC8& aSrcBuf,TBool aModeChange,TInt aStreamMode);

    private:

		/**
        * By default Symbian OS constructor is private.
        */
        void ConstructL(const TDesC &aFileName,
                        CParser::TVideoFormat aVideoFormat,
                        CParser::TAudioFormat aAudioFormat,
                        CVedAVCEdit *aAvcEdit);
                        
        void ConstructL(RFile* aFileHandle,
                        CParser::TVideoFormat aVideoFormat,
                        CParser::TAudioFormat aAudioFormat,
                        CVedAVCEdit *aAvcEdit);
                        
        void SetMediaOptions(CParser::TVideoFormat aVideoFormat, 
                             CParser::TAudioFormat aAudioFormat,
                             TUint& aMediaFlag);
                        
        void SetComposerOptionsL(CParser::TVideoFormat aVideoFormat, 
                                 CParser::TAudioFormat aAudioFormat);
                        
        TInt GetAVCDecoderSpecificInfoSize(TDesC8& aSrcBuf);

	public: // New Functions 

		/**
        * Writes audio decoder specific info which is required in case of AAC
		*
		* @param aSrcBuf - buffer containing the data to be written(decoder specific informtion)
        */
		TInt WriteAudioSpecificInfo(HBufC8*& aSrcBuf);

        /**
        * Writes audio decoder specific info which is required in case of AAC
		*
		* @param aSampleRate Output sample rate of the movie
        * @param aNumChannels Output num. of audio channels
        */
        TInt WriteAudioSpecificInfo(TInt aSampleRate, TInt aNumChannels);

    private:    // Data
       
        // The MP4 parser library handle
        MP4Handle iMP4Handle;
        mp4_u32 iVideoType;
		mp4_u32 iAudioType;

         // File server session handle
        RFs iFS;
        TBool iFsOpened;

        // Output filename
        TFileName iOutputMovieFileName;

        // Drive number of the output file
        TInt iDriveNumber;

        // Remaining free disk space
        TInt64 iFreeDiskSpace;        

        // Counter to check the real free disk space
        TUint iFreeDiskSpaceCounter;

		TBool iFirstWrite;

        // for compose buffer
        TUint8 *iComposeBuffer;
        mp4_u32 iComposedSize;
        
        // For AVC writing
        TInt iFrameNumber;

	    CVedAVCEdit* iAvcEdit;  // Avc editing instance
	    
	    RFile* iFileHandle;
    };

#endif      // __MP4COMPOSER_H__


// End of File
