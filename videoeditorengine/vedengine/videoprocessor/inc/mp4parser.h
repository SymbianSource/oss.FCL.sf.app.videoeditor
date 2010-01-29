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
* Header for mp4parser.cpp.
*
*/



#ifndef __MP4PARSER_H__
#define __MP4PARSER_H__

//  INCLUDES

#include "Parser.h"
#include <mp4lib.h>

// CONSTANTS
// MACROS
// DATA TYPES
// FUNCTION PROTOTYPES
// FORWARD DECLARATIONS

struct TAudioClipParameters; 

// CLASS DECLARATION
class CMovieProcessorImpl;

/**
*  MP4 -format parser class
*  ?other_description_lines
*/
class CMP4Parser : public CParser
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
	CMP4Parser();
	
	/**
	* Two-phased constructor.
	*/

	static CMP4Parser* NewL(CMovieProcessorImpl* aProcessor, const TDesC &aFileName);
	
    static CMP4Parser* NewL(CMovieProcessorImpl* aProcessor, RFile* aFileHandle);
	
	/**
	* Destructor.
	*/
	~CMP4Parser();
	
		
public: // New functions                        
	
    /**
    * Determine if the file/data inserted is in streamable, i.e. interleaved format
    * @return TBool ETrue if streamable
	*/	
	TInt IsStreamable();                
	
	/**
	* Seek to new position in input file 
	* @param aPositionInMs Position to be seeked to in milliseconds
	* @param anAudioTimeAfter audio position in milliseconds after seek
	* @param aVideoTimeAfter video position in milliseconds after seek
	* @return TInt error code
	*/	
	TInt Seek(TUint32 aPositionInMs, TUint32& anAudioTimeAfter, TUint32& aVideoTimeAfter);        
	
	/**
	* Reads information of next available packet in inserted input data
	* @param aType Frame type
    * @param aFrameLength Frame length in bytes
	* @return TInt error code
	*/
    TInt GetNextFrameInformation(TFrameType& aType, TUint& aLength, TBool& aIsAvailable);
		
    /**
    * Read a number of frames of requested type from inserted data or file
    * @param aDstBuffer Destination buffer
    * @param aType Type of frame(s) to be read        
    * @param aNumRead Number of frames actually read
    * @return TInt error code
    */		
    TInt ReadFrames(TDes8& aDstBuffer, TFrameType aType, TUint32& aNumRead, TUint32& aTimeStamp);
	
    /**
    * Initializes the module for AMR audio processing
	* @param aStartPosition Time to start reading from
    * @param aEndPosition Ending time for reading
    * @param aCurrentTime How long has audio been processed so far
    * @param aAudioOffset Offset from the beginning of movie timeline
    * @param aAudioPending ETrue if audio operation is pending
    * @param aPendingAudioDuration Duration for pending audio
    * @param aAudioClipParameters Audio clip parameter structure
    *
	* @return Error code
	*/		        
    void InitAudioProcessingL(TInt64 aStartPosition, TInt64 aEndPosition, 
                              TInt64& aCurrentTime, TInt64 aAudioOffset, 
                              TBool& aAudioPending, TInt64& aPendingAudioDuration);       

	/**
	* Gets information about video frames
	* @param aVideoFrameInfoArray Array to store frame properties
    * @param aStartIndex Index of the first frame to get properties
	* @param aSizeOfArray Size of aVideoFrameInfoArray
	* @return TInt error code
	*/
	TInt GetVideoFrameProperties(TFrameInfoParameters* aVideoFrameInfoArray,
			                     TUint32 aStartIndex, TUint32 aSizeOfArray);		                         

	/**
	*	Obtains the AAC audio decoder specific info for current clip
	*   @param aBytes -- buffer to be filled with the info	
	*	@param aBufferSize -- indicating the maximum size to be allocated
	*/
	TInt ReadAudioDecoderSpecificInfoL(HBufC8*& aBytes, TInt aBufferSize);

	/**
	*	Obtains the AAC audio decoder specific info default for 16khz and LC and Single channel
	*   @param aBytes -- buffer to be filled with the info	
	*	@param aBufferSize -- indicating the maximum size to be allocated
	*/
	TInt SetDefaultAudioDecoderSpecificInfoL(HBufC8*& aBytes, TInt aBufferSize);
	
public: // Functions from base classes           
	
    /**
    * From CParser Writes a block of data for parsing        
    * @param aBlock Block to be written
    * @return TInt error code
	*/
	TInt WriteDataBlock(const TDes8& aBlock);
	
	/**
	* From CParser Parser the stream header.
	* @param aStreamParameters Common stream parameters	
	*/
	TInt ParseHeaderL(CParser::TStreamParameters& aStreamParameters);
	
	/**
	* From CParser Resets parser to initial state.
	* @return TInt error code
	*/     
		
	TInt Reset();	
    
    /**
	* From CParser Seek to optimal intra before given time
	*/       

    TInt SeekOptimalIntraFrame(TTimeIntervalMicroSeconds aStartTime, TInt aIndex, TBool aFirstTime); 
    
    /**
	* From CParser 
	* Gets the number of frames in current clip
	*/ 
    TInt GetNumberOfVideoFrames();
    TInt GetNumberOfFrames();

    /**
	* From CParser 
	* Gets the size of video frame at given index
	*/ 
    TInt GetVideoFrameSize(TInt aIndex);

    /**
	* From CParser 
	* Gets the timestamp of video frame at given index
	*/ 
    TInt GetVideoFrameStartTime(TInt aIndex, TInt* iTimeStamp);

    /**
	* From CParser 
	* Gets the type of video frame at given index
	*/ 
    TInt8 GetVideoFrameType(TInt aIndex);        

    /**
	* From CParser Parser the stream header.
	* @param aAudioFrameSize average frame size of audio frame	
	*/
	TInt ParseAudioInfo(TInt& aAudioFrameSize);

	TInt GetMP4SpecificSize();  // added for Mpeg-4 Support 

    /**
	* From CParser Retrieves  average audio bitrate of current clip
	* @param aBitrate Average bitrate
    *
    * @return error code
	*/
    TInt GetAudioBitrate(TInt& aBitrate);
    
    /**
	* From CParser Retrieves average video frame rate of current clip
	* @param aBitrate Average frame rate
    *
    * @return error code
	*/
    TInt GetVideoFrameRate(TReal& aFrameRate);    
    
    /**
	* From CParser Returns the size of decoder specific info
	*
	* @return size in bytes
	*/
    TInt GetDecoderSpecificInfoSize();
    
    /**
	* From CParser Reads AVC decoder specific info to buffer
	* @param aBuf Destination buffer
	*
	* @return error code
	*/
    TInt ReadAVCDecoderSpecificInfo(TDes8& aBuf);
    
    /**
	* From CParser Returns the duration of video track in milliseconds
	* @param aDurationInMs Duration
	*
	* @return error code
	*/
    TInt GetVideoDuration(TInt& aDurationInMs);    
	
private:        
	
   /**
    * By default Symbian OS constructor is private.
	*/
	void ConstructL(CMovieProcessorImpl* aProcessor, const TDesC &aFileName);	
	
	void ConstructL(CMovieProcessorImpl* aProcessor, RFile* aFileHandle);
	
private:  // Internal constants
	
	// Stream source type
	enum TStreamSource
	{
		ESourceNone = 0, // not set yet
	    ESourceFile, // reading from a file
		ESourceUser // user of this object provides data
	};        

public:
	
	
private:    // Data

    // video processor instance
    CMovieProcessorImpl *iProcessor;
        
	TStreamSource iStreamSource;        
	
	mp4_u32 iVideoType;
	mp4_u32 iAudioType;
    
	TUint iBytesRead;
		
	// The MP4 parser library handle
	MP4Handle iMP4Handle;
	
	TFrameType iNextFrameType;
	TUint iNextFrameLength;
	
	TBool iFirstRead;
	TBool iFirstFrameInfo;

    // Max video frame length & AMR sample size, used for sanity checks to avoid crashes in case of corrupted input
    TInt iMaxVideoFrameLength;
    TInt iMaxAMRSampleSize;

    };
		
#endif      // __MP4PARSER_H__
		
		
// End of File
