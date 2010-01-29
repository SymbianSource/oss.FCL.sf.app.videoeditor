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



// INCLUDE FILES

#include    "movieprocessorimpl.h"
#include    "mp4parser.h"
#include    "vedvideosettings.h"
#include    "vedaudiosettings.h"
#include    "vedvolreader.h"
#include    "vedavcedit.h"

// ASSERTIONS
#define MPASSERT(x) __ASSERT_DEBUG(x, User::Panic(_L("CMovieProcessorImpl"), EInternalAssertionFailure))

#ifdef _DEBUG
#include <e32svr.h>
#define PRINT(x) RDebug::Print x;
#else
#define PRINT(x)
#endif

// LOCAL CONSTANTS AND MACROS
#ifdef _DEBUG
const TInt KErrorCode = CParser::EParserFailure;
#else
const TInt KErrorCode = KErrGeneral;
#endif

//const TUint KNumFramesInOneRun = 10;
const TUint KVOLHeaderBufferSize = 256;
const TUint KAVCDCRBufferSize = 1024;
const TUint KMinBitrate = 128;

// ================= MEMBER FUNCTIONS =======================

// C++ default constructor can NOT contain any code, that
// might leave.
//

CMP4Parser::CMP4Parser()
{       
	iMP4Handle = 0;
	iVideoType = 0;
	iAudioType = 0; 
	iBytesRead = 0;
	iFirstRead = ETrue;		// added for Mp4
	iFirstFrameInfo = ETrue;  // added for Mp4
	iOutputNumberOfFrames = 0; 
    iStreamSource = ESourceNone;
	iFrameNumber=0;
	iFirstTimeClipParsing=ETrue;
    iStartFrameIndex=0;		
}

// Two-phased constructor.

CMP4Parser* CMP4Parser::NewL(CMovieProcessorImpl* aProcessor, const TDesC &aFileName)
{
	CMP4Parser *self = new (ELeave) CMP4Parser;
	CleanupStack::PushL(self);    
	
	if ( aFileName.Length() > 0 )   
		self->iStreamSource = ESourceFile;
	else
		self->iStreamSource = ESourceUser;
	
	self->ConstructL(aProcessor,aFileName);

	CleanupStack::Pop();  // self
	
	return self;
}

CMP4Parser* CMP4Parser::NewL(CMovieProcessorImpl* aProcessor, RFile* aFileHandle)
{
	CMP4Parser *self = new (ELeave) CMP4Parser;
	CleanupStack::PushL(self);    
	
	self->iStreamSource = ESourceFile;	
	
	self->ConstructL(aProcessor,aFileHandle);

	CleanupStack::Pop();  // self
	
	return self;
}

// Symbian OS default constructor can leave.

void CMP4Parser::ConstructL(CMovieProcessorImpl* aProcessor, const TDesC &aFileName)
{
	MP4Err error;
	iProcessor = aProcessor; 

	// open MP4 library
    if ( iStreamSource == ESourceFile )
        {        
        TBuf<258> temp(aFileName);
        temp.ZeroTerminate();			

        MP4FileName name = reinterpret_cast<MP4FileName>( const_cast<TUint16*>(temp.Ptr()) );

        error = MP4ParseOpen(&iMP4Handle, name);
        if ( error == MP4_OK )
            {
            // set buffer sizes; only parser buffer size is effective for this instance
            error = MP4SetCustomFileBufferSizes(iMP4Handle, K3gpMp4ComposerWriteBufferSize, K3gpMp4ComposerNrOfWriteBuffers, K3gpMp4ParserReadBufferSize );
            }
        }
    else
        {
        // buffer
        error = MP4ParseOpen(&iMP4Handle, 0);        
        }        
		
    if (error == MP4_OUT_OF_MEMORY)
        {
        User::Leave(KErrNoMemory);
        }
    else if ( error != MP4_OK )
        {
        User::Leave(KErrorCode);
		}
}

void CMP4Parser::ConstructL(CMovieProcessorImpl* aProcessor, RFile* aFileHandle)
{
	MP4Err error;
	iProcessor = aProcessor;

	// open MP4 library
    error = MP4ParseOpenFileHandle(&iMP4Handle, aFileHandle);
    
    if ( error == MP4_OK )
        {
        // set buffer sizes; only parser buffer size is effective for this instance
        error = MP4SetCustomFileBufferSizes(iMP4Handle, K3gpMp4ComposerWriteBufferSize, K3gpMp4ComposerNrOfWriteBuffers, K3gpMp4ParserReadBufferSize );
        }
        
    if (error == MP4_OUT_OF_MEMORY)
        {
        User::Leave(KErrNoMemory);
        }
    else if ( error != MP4_OK )
        {
        User::Leave(KErrorCode);
		}
}

// Destructor
CMP4Parser::~CMP4Parser()
{
    if (iMP4Handle)
        MP4ParseClose(iMP4Handle);

    iMP4Handle = 0;
}



// ---------------------------------------------------------
// CMP4Parser::WriteDataBlockL
// Write a block of data to parser
// (other items were commented in a header).
// ---------------------------------------------------------
//

TInt CMP4Parser::WriteDataBlock(const TDes8& aBlock)
{               
	
	MPASSERT(iStreamSource != ESourceFile);    
	MP4Err error = MP4ParseWriteData(iMP4Handle, (mp4_u8*)(aBlock.Ptr()), mp4_u32(aBlock.Length()) );
	
	if ( error == MP4_OUT_OF_MEMORY )
		return KErrNoMemory;
	else if ( error == MP4_ERROR )
		return KErrorCode;
    else
        return KErrNone;
	
}

// ---------------------------------------------------------
// CMP4Parser::GetNextFrameInformation
// Get type (streaming-case), length and availability of next frame to be fetched
// using MP4 library API functions
// (other items were commented in a header).
// ---------------------------------------------------------
//

TInt CMP4Parser::GetNextFrameInformation(TFrameType& aType, TUint& aLength, TBool& aIsAvailable)
{   
	
	// If aType == EFrameTypeNone, the type of next frame is retrieved 
	// (valid only in streaming case)
	// Otherwise, only the length of next specified type of frame is retrieved
	MPASSERT(iStreamSource != ESourceNone);            
	
	mp4_u32 type = MP4_TYPE_NONE;
	MP4Err error;

	aIsAvailable = 0;    
	if ( iNextFrameType == EFrameTypeNone )
		// if the mp4 library is reading a file, a frame has always been read when 
		// we come here
		// otherwise it might be that a complete frame was not available yet
		// and we come here to ask again        
	{
		if ( aType == EFrameTypeNone ) 
		{
			MPASSERT(iStreamSource == ESourceUser);
			
			// get next frame type
			error = MP4ParseNextFrameType(iMP4Handle, &type);

			if ( error == MP4_NOT_AVAILABLE )
				return KErrNone;                
			else if ( error == MP4_NO_FRAME )                
				return EParserEndOfStream; // no video or audio frames left, stream ended
			else if ( error == MP4_INVALID_INPUT_STREAM )
				return KErrCorrupt;
			else if ( error != MP4_OK )
				return KErrorCode;
            else
            {
                MPASSERT(error == MP4_OK);
            }
			
			switch ( type ) 
			{
			case MP4_TYPE_H263_PROFILE_0:
		    case MP4_TYPE_MPEG4_VIDEO:
          
				MPASSERT( type == iVideoType );
				iNextFrameType = EFrameTypeVideo;         
				break;
				
			case MP4_TYPE_AMR_NB:
				MPASSERT( type == iAudioType );
				iNextFrameType = EFrameTypeAudio;                    
				break;        

			default:
				return KErrNotSupported;
			}
		}
		else 
		{
			// library reads the file            
			//MPASSERT(iStreamSource == ESourceFile);
			type = ( aType == EFrameTypeVideo ) ? iVideoType : iAudioType;
			iNextFrameType = aType;
		}
		
		// get length for the frame
		mp4_u32 length = 0, mp4Specific = 0;
		MPASSERT( type != MP4_TYPE_NONE );
		if ( (iFirstFrameInfo) &&
		     ((iVideoType == MP4_TYPE_MPEG4_VIDEO) || (iVideoType == MP4_TYPE_AVC_PROFILE_BASELINE)) ) 		   
		{
			error = MP4ParseReadVideoDecoderSpecificInfo( iMP4Handle, 0, 0, &mp4Specific );
			iFirstFrameInfo = EFalse;
		}
		error = MP4ParseNextFrameSize(iMP4Handle, type, &length);
		MPASSERT( error != MP4_NOT_AVAILABLE );
		
		if ( length == 0 || error == MP4_NO_REQUESTED_FRAME )
		{            
			// file-reading case: all frames of this type have been read
			//   and the caller should try with the other type
			MPASSERT( length == 0 );
			iNextFrameType = EFrameTypeNone;
			aLength = 0;            
			return KErrNone;            
		}
		else if ( error == MP4_INVALID_INPUT_STREAM )
			return KErrCorrupt;
		else if ( error != MP4_OK )
			return KErrorCode;
       else if ( length > iMaxVideoFrameLength )
            {
            PRINT((_L("CMP4Parser::GetNextFrameInformation() too large video frame size %d, return KErrCorrupt"),length));
			return KErrCorrupt;
            }
        else
            iNextFrameLength = TUint(length + mp4Specific);

	}
	MPASSERT(iNextFrameType != EFrameTypeNone);
	MPASSERT(iNextFrameLength != 0);
	
	// check if frame is available
	if ( iStreamSource == ESourceUser )
	{
		error = MP4ParseIsFrameAvailable(iMP4Handle, type);
		if ( error != MP4_OK && error != MP4_NOT_AVAILABLE )
			return KErrorCode;
		aIsAvailable = ( error == MP4_NOT_AVAILABLE ) ? EFalse : ETrue;
	}
	else 
		aIsAvailable = ETrue;
	
	aType = iNextFrameType;
	aLength = iNextFrameLength;	
	
	return KErrNone;
}   

// ---------------------------------------------------------
// CMP4Parser::ReadFrames
// Read the next frame(s) from file / stream
// (other items were commented in a header).
// ---------------------------------------------------------
//

TInt CMP4Parser::ReadFrames(TDes8& aDstBuffer, TFrameType aType, TUint32& aNumRead, 
														TUint32& aTimeStamp)
{
	MP4Err error;
	mp4_u32 returnedSize = 0;    
	mp4_bool keyFrame = 0;

	MPASSERT( iNextFrameType != EFrameTypeNone && aType == iNextFrameType ); 
	MPASSERT( iNextFrameLength != 0 );

#ifdef _DEBUG
	mp4_u32 type = MP4_TYPE_NONE; // buffer support
	type = ( aType == EFrameTypeVideo ) ? iVideoType : iAudioType; // buffer support
	if (iStreamSource == ESourceUser)
		MPASSERT( MP4ParseIsFrameAvailable(iMP4Handle, type) == MP4_OK );
#endif


	if (aType == EFrameTypeVideo) 
	{	
		TUint32 iTimeStampInTicks=0;
		mp4_u32 mp4Specific = 0;

		if ( (iFirstRead) &&
		     ((iVideoType == MP4_TYPE_MPEG4_VIDEO) || (iVideoType == MP4_TYPE_AVC_PROFILE_BASELINE)) ) 		   
		{
			error = MP4ParseReadVideoDecoderSpecificInfo( iMP4Handle, (mp4_u8*)(aDstBuffer.Ptr() + aDstBuffer.Length()), 
				mp4_u32( aDstBuffer.MaxLength() ), &mp4Specific );
			iFirstRead = EFalse;
		}
		
		error = MP4ParseReadVideoFrame(iMP4Handle, (mp4_u8*)(aDstBuffer.Ptr() + aDstBuffer.Length()+ mp4Specific), 
			mp4_u32( aDstBuffer.MaxLength() ), &returnedSize, (mp4_u32*)&aTimeStamp,
			&keyFrame, &iTimeStampInTicks);					    

		returnedSize += mp4Specific;
		iFrameNumber++;
		aNumRead = 1;
	}
	else 
	{       
		error = MP4ParseReadAudioFrames(iMP4Handle, (mp4_u8*)(aDstBuffer.Ptr()), 
			mp4_u32(aDstBuffer.MaxLength()), &returnedSize, (mp4_u32*)&aTimeStamp, 
			(mp4_u32*)&aNumRead, NULL);               
		
		//PRINT((_L("audio TS:%d, "), aTimeStamp));
	}
	
	MPASSERT(error != MP4_BUFFER_TOO_SMALL);    
	aDstBuffer.SetLength(aDstBuffer.Length() + TInt(returnedSize));
	iBytesRead += returnedSize;
	iNextFrameType = EFrameTypeNone;
	iNextFrameLength = 0;
		
	//PRINT((_L("error=%d, numReturned=%d, returnedSize=%d, bufferSize=%d\n"), 
	//	error, aNumRead, returnedSize, aDstBuffer.MaxLength()));
		
	if (error == MP4_NOT_AVAILABLE)
		return EParserNotEnoughData;    
	else if ( error == MP4_INVALID_INPUT_STREAM )
		return KErrCorrupt;
	else if ( error != MP4_OK )
		return KErrorCode;
    else
        return KErrNone;
}


// ---------------------------------------------------------
// CMP4Parser::Reset
// Resets the parser to its initial state
// (other items were commented in a header).
// ---------------------------------------------------------
//

TInt CMP4Parser::Reset()
{
	MP4Err error;
	
	if ( iStreamSource == ESourceFile )
	{
		mp4_u32 audioPos, videoPos;
		
		// seek to very beginning
		error = MP4ParseSeek(iMP4Handle, 0, &audioPos, &videoPos, EFalse);
		if ( error != MP4_OK )
			return KErrorCode;
		
		MPASSERT( videoPos == 0 && (iAudioType == 0 || audioPos == 0) );		
		
	}
	else 
	{
		// close & open library to make sure old data is flushed
		error = MP4ParseClose(iMP4Handle);
		
		if ( error != MP4_OK )
			return KErrorCode;
		error = MP4ParseOpen(&iMP4Handle, 0);
		if ( error != MP4_OK )
			return KErrorCode;                    
	}
	
	iBytesRead = 0;
	iNextFrameType = EFrameTypeNone;
	iNextFrameLength = 0;
	
	return KErrNone;
}


// ---------------------------------------------------------
// CMP4Parser::ParseHeaderL
// Get relevant stream parameters by calling mp4 library functions
// (other items were commented in a header).
// ---------------------------------------------------------
//

TInt CMP4Parser::ParseHeaderL(CParser::TStreamParameters& aStreamParameters)
{

    PRINT((_L("CMP4Parser::ParseHeaderL() begin")));

	MP4Err error;
	mp4_double frameRate = 0;    
	TBool hasVideo = ETrue;
	
	// Reset channel info
	aStreamParameters.iHaveVideo = EFalse;
	aStreamParameters.iHaveAudio = EFalse;
	aStreamParameters.iNumDemuxChannels = 0;
	aStreamParameters.iFileFormat = EFileFormatUnrecognized;
	aStreamParameters.iVideoFormat = EVideoFormatNone;
	aStreamParameters.iAudioFormat = EAudioFormatNone;
	aStreamParameters.iVideoLength = 0;
	aStreamParameters.iAudioLength = 0;
	aStreamParameters.iStreamLength = 0;
	aStreamParameters.iAudioFramesInSample = 0;
	aStreamParameters.iVideoPicturePeriodNsec = 0;
	aStreamParameters.iCanSeek = EFalse;
	aStreamParameters.iFrameRate=0;
	aStreamParameters.iVideoTimeScale=0; 
	aStreamParameters.iAudioTimeScale=0; 
	iAudioType = 0;
	iVideoType = 0;
	iNumberOfFrames=0;
	
	// get video description
	error = MP4ParseRequestVideoDescription(iMP4Handle, (mp4_u32 *)&aStreamParameters.iVideoLength,
		&frameRate, &iVideoType, (mp4_u32 *)&aStreamParameters.iVideoWidth, 
		(mp4_u32 *)&aStreamParameters.iVideoHeight, (mp4_u32 *)&aStreamParameters.iVideoTimeScale);

	if ( error == MP4_NOT_AVAILABLE )
		User::Leave(KErrorCode);
	else if ( error == MP4_INVALID_INPUT_STREAM )
		User::Leave(KErrCorrupt);
	else if ( error == MP4_NO_VIDEO )  
	{
		hasVideo = EFalse;
		aStreamParameters.iVideoWidth = aStreamParameters.iVideoHeight = 0;
	}
	else if ( error != MP4_OK )
		User::Leave(KErrorCode);
    else
    {
        MPASSERT(error == MP4_OK);
    }

	// get audio description. ask also for averagebitrate to get error if the track is empty; the information is needed later on
    mp4_u32 averagebitrate = 0;

	error = MP4ParseRequestAudioDescription(iMP4Handle, (mp4_u32 *)&aStreamParameters.iAudioLength, 
		&iAudioType, (mp4_u8*)&aStreamParameters.iAudioFramesInSample, (mp4_u32 *)&aStreamParameters.iAudioTimeScale, &averagebitrate );

    if ( (error == MP4_ERROR) && ((iAudioType == MP4_TYPE_MPEG4_AUDIO) || (iAudioType == MP4_TYPE_AMR_NB) || (iAudioType == MP4_TYPE_AMR_WB)))
        {
        // a special case: there may be audio track but it is empty; if type was recognized, mark as no audio
        PRINT((_L("CMP4Parser::ParseHeaderL() problems with getting audio description, mark no audio since audio type was recognized")));
        iAudioType = MP4_NO_AUDIO;
        error = MP4_NO_AUDIO;
        }
	if(error == MP4_NOT_AVAILABLE)     
		User::Leave(EParserNotEnoughData);        
	else if ( error == MP4_INVALID_INPUT_STREAM )
		User::Leave(KErrCorrupt);
	else if ( error != MP4_NO_AUDIO && error != MP4_OK )
		User::Leave(KErrorCode);
    else
    {
        MPASSERT(error == MP4_OK || error == MP4_NO_AUDIO);
    }

    // store the sample size for sanity checking purposes
    iMaxAMRSampleSize = KVedMaxAMRFrameSize * aStreamParameters.iAudioFramesInSample;

	if (aStreamParameters.iVideoLength > 0)
		aStreamParameters.iStreamLength = aStreamParameters.iVideoLength;

	if (aStreamParameters.iAudioLength > aStreamParameters.iVideoLength)
		aStreamParameters.iStreamLength = aStreamParameters.iAudioLength;	
	
	aStreamParameters.iFrameRate = frameRate;

	if(hasVideo)
	{
		    
	    if ( iVideoType == MP4_TYPE_MPEG4_VIDEO )
	    {
	        // read video resolution from VOL header
	        
	        HBufC8* tmpBuffer = (HBufC8*) HBufC8::NewLC(KVOLHeaderBufferSize);
            TPtr8 tmpPtr = tmpBuffer->Des();
            mp4_u32 volSize = 0;
            MP4Err volError = 0;
            
            volError = MP4ParseReadVideoDecoderSpecificInfo( iMP4Handle, 
                                                            (mp4_u8*)(tmpPtr.Ptr()),
                                                            KVOLHeaderBufferSize,
                                                            &volSize );
                                                            
            if ( volError != MP4_OK )
            {
                User::Leave(KErrorCode);
            }
            tmpPtr.SetLength(volSize);
            
            CVedVolReader* tmpVolReader = CVedVolReader::NewL();
            CleanupStack::PushL(tmpVolReader);

            tmpVolReader->ParseVolHeaderL(tmpPtr);

            aStreamParameters.iVideoWidth = tmpVolReader->Width();
            aStreamParameters.iVideoHeight = tmpVolReader->Height();

            CleanupStack::PopAndDestroy(tmpVolReader);
            CleanupStack::PopAndDestroy(tmpBuffer);		

	    }		
	    	    
	    else if ( iVideoType == MP4_TYPE_AVC_PROFILE_BASELINE )
	    {
	        // read resolution from SPS

	        HBufC8* tmpBuffer = (HBufC8*) HBufC8::NewLC(KAVCDCRBufferSize);	        	        
            TPtr8 ptr = tmpBuffer->Des();
            
            // read decoder specific info
            User::LeaveIfError( ReadAVCDecoderSpecificInfo(ptr) );
            
            // create AVC editing instance
            CVedAVCEdit* avcEdit = CVedAVCEdit::NewL();
            CleanupStack::PushL(avcEdit);
            
            TSize resolution(0,0);
            User::LeaveIfError( avcEdit->GetResolution(ptr, resolution) );
            
            CleanupStack::PopAndDestroy(avcEdit);
            CleanupStack::PopAndDestroy(tmpBuffer);

            aStreamParameters.iVideoWidth = resolution.iWidth;
            aStreamParameters.iVideoHeight = resolution.iHeight;
	    }


		iNumberOfFrames = GetNumberOfVideoFrames();
		MPASSERT(iNumberOfFrames);
    
		if (iFirstTimeClipParsing)	// update only at the first parsing of a clip
		{
			// update the frame numbers
			
			iOutputNumberOfFrames = iProcessor->GetOutputNumberOfFrames();		
			
			if (iOutputNumberOfFrames == 0)
			{
				iOutputNumberOfFrames += iNumberOfFrames;  // total number of frames in all clips	
			}
			else if (!iProcessor->IsThumbnailInProgress())
			{
    			iOutputNumberOfFrames += iNumberOfFrames; 
			}
		}
		
		MPASSERT(frameRate > 0);
		if (frameRate > 0)
			aStreamParameters.iVideoPicturePeriodNsec = TInt64( TReal(1000000000) / TReal(frameRate) );
		else
			aStreamParameters.iVideoPicturePeriodNsec = TInt64(33366667);
		
		if ( iVideoType == MP4_TYPE_H263_PROFILE_0 || iVideoType == MP4_TYPE_H263_PROFILE_3 )
		{
			TVideoClipProperties prop;
			prop.iH263Level = 0;
			MP4ParseGetVideoClipProperties(iMP4Handle, prop);

            iMaxVideoFrameLength = KMaxCodedPictureSizeQCIF;
			if (prop.iH263Level == 45)
                {
				aStreamParameters.iVideoFormat = EVideoFormatH263Profile0Level45;
                }
			else 
                {
				aStreamParameters.iVideoFormat = EVideoFormatH263Profile0Level10;
                }
		}
		else if ( iVideoType == MP4_TYPE_MPEG4_VIDEO )
            {
			aStreamParameters.iVideoFormat = EVideoFormatMPEG4;		
            if ( aStreamParameters.iVideoWidth <= KVedResolutionQCIF.iWidth )
                {
                iMaxVideoFrameLength = KMaxCodedPictureSizeMPEG4L0BQCIF;//distinction between L0 and L0B not possible here
                }
            else if (aStreamParameters.iVideoWidth <= KVedResolutionCIF.iWidth )
                {
                iMaxVideoFrameLength = KMaxCodedPictureSizeMPEG4CIF;
                }
            else
                {
                // VGA
                iMaxVideoFrameLength = KMaxCodedPictureSizeVGA;
                }
            }
            
        else if ( iVideoType == MP4_TYPE_AVC_PROFILE_BASELINE )
            {

            // : Is it possible to dig up the level here ??
			aStreamParameters.iVideoFormat = EVideoFormatAVCProfileBaseline;
			
            if ( aStreamParameters.iVideoWidth <= KVedResolutionQCIF.iWidth )
                {
                iMaxVideoFrameLength = KMaxCodedPictureSizeAVCLevel1B;  //distinction between L0 and L0B not possible here ??
                }
            else if (aStreamParameters.iVideoWidth <= KVedResolutionCIF.iWidth )
                {
                iMaxVideoFrameLength = KMaxCodedPictureSizeAVCLevel1_2;
                }
            else
                {
                // default
                iMaxVideoFrameLength = KMaxCodedPictureSizeAVCLevel1_2;
                }
            }
	}	

	if ( error == MP4_OK )
	{
		// stream contains audio 
		if ( iAudioType == MP4_TYPE_AMR_NB )
			aStreamParameters.iAudioFormat = EAudioFormatAMR;
		else
		{
			if ( iAudioType == MP4_TYPE_MPEG4_AUDIO )
				aStreamParameters.iAudioFormat = EAudioFormatAAC;
		}		
	}
	
	TBool videoMpeg4OrAVC = ( iVideoType == MP4_TYPE_MPEG4_VIDEO || 
	                          iVideoType == MP4_TYPE_AVC_PROFILE_BASELINE );

	if ( (videoMpeg4OrAVC && iAudioType == MP4_TYPE_MPEG4_AUDIO) ||
		 (videoMpeg4OrAVC && iAudioType == MP4_TYPE_NONE) ||
		 (iVideoType == MP4_TYPE_NONE && iAudioType == MP4_TYPE_MPEG4_AUDIO) )
		aStreamParameters.iFileFormat = EFileFormatMP4;

	else if (iVideoType != MP4_TYPE_NONE || iAudioType != MP4_TYPE_NONE)
		aStreamParameters.iFileFormat = EFileFormat3GP;

	if ( aStreamParameters.iStreamLength == 0 )
		aStreamParameters.iFileFormat = EFileFormatUnrecognized;
	
	if ( aStreamParameters.iVideoFormat != EVideoFormatNone ) 
	{
		aStreamParameters.iHaveVideo = ETrue;
		aStreamParameters.iNumDemuxChannels++;
	}
	
	if ( aStreamParameters.iAudioFormat != EAudioFormatNone )
	{
		aStreamParameters.iHaveAudio = ETrue;
		aStreamParameters.iNumDemuxChannels++;
	}	
	
	aStreamParameters.iMaxPacketSize = 0;  // N/A
	aStreamParameters.iLogicalChannelNumberVideo = 0;  // N/A
	aStreamParameters.iLogicalChannelNumberAudio = 0;  // N/A
	
	// get stream description
	error = MP4ParseRequestStreamDescription(iMP4Handle, (mp4_u32 *)&aStreamParameters.iStreamSize, 
		(mp4_u32 *)&aStreamParameters.iStreamBitrate);    
	if ( error != MP4_OK )     
		User::Leave(KErrorCode);
	
	// do sanity-check for bitrate
	if (aStreamParameters.iStreamBitrate < KMinBitrate)
	    User::Leave(KErrCorrupt);
	
	aStreamParameters.iReferencePicturesNeeded = 0;     
	aStreamParameters.iNumScalabilityLayers = 0;
	
	// determine if the stream contains INTRA frames so seeking is possible
	// If the stream contains more than one INTRA frame, seeking is 
	// allowed. 
	
	if (hasVideo)
	{
		mp4_u32 position = aStreamParameters.iStreamLength + 1000;
		mp4_u32 audioTime, videoTime;
		
		// Seek past stream duration to find out the position of last INTRA frame
		error = MP4ParseSeek(iMP4Handle, position, &audioTime, &videoTime, ETrue);
		if ( error != MP4_OK )     
			User::Leave(KErrorCode);
		
		if (videoTime != 0) 
		{
			// at least two INTRAs
			aStreamParameters.iCanSeek = ETrue;
		}
		
		// rewind file back to beginning
		error = MP4ParseSeek(iMP4Handle, 0, &audioTime, &videoTime, EFalse);
		if ( error != MP4_OK )     
			User::Leave(KErrorCode);
	}

    PRINT((_L("CMP4Parser::ParseHeaderL() end")));

	return KErrNone; 
}


// ---------------------------------------------------------
// CMP4Parser::IsStreamable
// Finds out whether input stream is multiplexed so that
// it can be streamed, i.e. played while receiving the stream.
// (other items were commented in a header).
// ---------------------------------------------------------
//

TInt CMP4Parser::IsStreamable()
{
	MP4Err error;
	
	error = MP4ParseIsStreamable(iMP4Handle);
	if ( error == MP4_NOT_AVAILABLE )
		return EParserNotEnoughData;
	else if ( error == MP4_INVALID_INPUT_STREAM )
		return KErrNotSupported;
	else if ( error == MP4_ERROR )
		return KErrorCode;    
	else
		return ( error == MP4_OK ) ? 1 : 0;
}


// ---------------------------------------------------------
// CMP4Parser::Seek
// Seeks the file to desired position in milliseconds
// (other items were commented in a header).
// ---------------------------------------------------------
//
TInt CMP4Parser::Seek(TUint32 aPositionInMs, TUint32& anAudioTimeAfter, TUint32& aVideoTimeAfter)
{
	
	MP4Err error = MP4_OK;
	MPASSERT(iStreamSource == ESourceFile);
	error = MP4ParseSeek(iMP4Handle, aPositionInMs, &anAudioTimeAfter, &aVideoTimeAfter, ETrue);
	if (error != MP4_OK)
		return KErrorCode;    
	
	return KErrNone;
}



TInt CMP4Parser::GetNumberOfFrames()
{
	return iNumberOfFrames;
}

// ---------------------------------------------------------
// CMP4Parser::SeekOptimalIntraFrame
// Seeks to INTRA frame position before given time
// (other items were commented in a header).
// ---------------------------------------------------------
//
TInt CMP4Parser::SeekOptimalIntraFrame(TTimeIntervalMicroSeconds aStartTime, TInt /*aIndex*/, TBool aFirstTime)
{
	MP4Err error = KErrNone; 
	TInt revisedNumberOfFrames = 0;
	mp4_u32 audioTime = 0; 
	mp4_u32 videoTime = 0; 
//	mp4_u32 timeScale = 0; 
 
	// calculate the start time of the cut operation
	TInt64 startTime = aStartTime.Int64() / TInt64(1000);    
	mp4_u32 startPosition = (mp4_u32)( I64INT(startTime) ); // in milliseconds

	TBool intraFound = (startPosition == 0);
	
	if (!aFirstTime)	
	    intraFound = 0;
		
	// First check if the first included frame is intra	
	if (!intraFound)
	{
	    // seek to previous frame preceding start time or at start time
	    error = MP4ParseSeek(iMP4Handle, startPosition, &audioTime, &videoTime, EFalse);
	    if (error != MP4_OK)
	    {		
		    return KErrorCode; 
	    }
	    MPASSERT(videoTime <= startPosition);

	    // get index of the frame
	    TInt index = iProcessor->GetVideoFrameIndex(TTimeIntervalMicroSeconds(videoTime*1000));
	    
	    if (videoTime < startPosition)	    
	    {
	        // if there was no frame at start time, seek 
	        // one frame forward to the first included frame
            index++;
	    }
	    
        // get frame type
        mp4_bool frameType;
	    error = MP4ParseGetVideoFrameType(iMP4Handle, index, &frameType);	    
	    if (error != MP4_OK)
	    {		
            return KErrorCode; 
	    }
	    if (frameType == 1)
	    {
	        intraFound = ETrue;
	        iStartFrameIndex = index;
	        
	        mp4_u32 timeInTicks;
	        // get timestamp of matched frame to startPosition
	        error = MP4ParseGetVideoFrameStartTime(iMP4Handle, index, &timeInTicks, &startPosition);
	        if (error != MP4_OK)
	        {		
		        return KErrorCode; 
	        }	        
	        
	        // Now seek to found Intra in 1 ms increments. The loop is needed 
	        // because due to rounding error, MP4Parser may seek to previous
	        // frame instead of the I-frame at startPosition
	        TInt seekTime = startPosition;
	        videoTime = 0;
	        
	        while (videoTime != startPosition)
	        {	            	        
    	        error = MP4ParseSeek(iMP4Handle, seekTime, &audioTime, &videoTime, EFalse);
        	    if (error != MP4_OK)
        	    {		
        		    return KErrorCode; 
        	    }
        	    MPASSERT(videoTime <= startPosition);
        	    seekTime++;  // add 1 ms
	        }
        	
	    }                
	}	
	
	if (!intraFound)
	{	    
    	// seek to the I-frame preceding the start time
    	error = MP4ParseSeek(iMP4Handle, startPosition, &audioTime, &videoTime, ETrue);
    	if (error != MP4_OK)
    	{		
    		return KErrorCode; 
    	}
	}
	
	if (videoTime != 0) 
	{
	    if (!intraFound)
        {                
            // get index of the intra frame
            TInt64 time = TInt64(TUint(videoTime)) * TInt64(1000);
            iStartFrameIndex = iProcessor->GetVideoFrameIndex(TTimeIntervalMicroSeconds( time ));            
        }        		
		
		if (aFirstTime)
		{	
		    revisedNumberOfFrames = iNumberOfFrames - iStartFrameIndex;	    
		    // update movie and clip number of frames
            iOutputNumberOfFrames -= iStartFrameIndex;            
            iNumberOfFrames = revisedNumberOfFrames; 
		}
				
		PRINT((_L("CMP4Parser::SeekOptimalIntraFrame() revised = %d"),revisedNumberOfFrames));
        PRINT((_L("CMP4Parser::SeekOptimalIntraFrame() iNumberOfFrames = %d"),iNumberOfFrames));
        PRINT((_L("CMP4Parser::SeekOptimalIntraFrame() iStartFrameIndex = %d"),iStartFrameIndex));                                
        PRINT((_L("CMP4Parser::SeekOptimalIntraFrame() iOutputNumberOfFrames = %d"),iOutputNumberOfFrames));        				
        iFrameNumber = iStartFrameIndex;
	}
	return KErrNone;
}

// ---------------------------------------------------------
// CMP4Parser::GetNumberOfVideoFrames
// Gets the number of video frames in clip
// (other items were commented in a header).
// ---------------------------------------------------------
//
TInt CMP4Parser::GetNumberOfVideoFrames()
{
    mp4_u32 numberOfFrames = 0;
    MP4Err error = 0;    

    error = MP4ParseGetNumberOfVideoFrames(iMP4Handle, &numberOfFrames);

    if (error != MP4_OK)
        return 0;

    return numberOfFrames;
  
}

// ---------------------------------------------------------
// CMP4Parser::GetVideoFrameSize
// Gets the size of video frame at given index
// (other items were commented in a header).
// ---------------------------------------------------------
//
TInt CMP4Parser::GetVideoFrameSize(TInt aIndex)
{
    mp4_u32 frameSize = 0;
	mp4_u32 mp4Specific = 0;
    MP4Err error = 0;

	if ( aIndex == 0 && iVideoType == MP4_TYPE_MPEG4_VIDEO )
	{
		error = MP4ParseReadVideoDecoderSpecificInfo( iMP4Handle, 0, 0, &mp4Specific );
		
		if ( error != MP4_OK && error != MP4_BUFFER_TOO_SMALL )
			return KErrorCode;
	}

    error = MP4ParseGetVideoFrameSize(iMP4Handle, aIndex, &frameSize);

    if (error != MP4_OK)
        return KErrorCode;

    return frameSize + mp4Specific;
	
}



// ---------------------------------------------------------
// CMP4Parser::GetVideoFrameStartTime
// Returns frame start time in millisec - optional iTimestamp for start time in ticks
// (other items were commented in a header).
// ---------------------------------------------------------
//
TInt CMP4Parser::GetVideoFrameStartTime(TInt aIndex, TInt* aTimeStampInTicks)
{

    MP4Err error = 0;
    mp4_u32 timeStampInMs = 0;

    MPASSERT(aTimeStampInTicks);
  
    PRINT((_L("CMP4Parser::GetVideoFrameStartTime(), get time for index %d"), aIndex));
    error = MP4ParseGetVideoFrameStartTime(iMP4Handle, aIndex, (mp4_u32*)aTimeStampInTicks, &timeStampInMs);

    if (error != MP4_OK)
        {
        PRINT((_L("CMP4Parser::GetVideoFrameStartTime(), error from MP4 parser %d"), error));
        return KErrorCode;
        }

    PRINT((_L("CMP4Parser::GetVideoFrameStartTime(), time in ms %d"), timeStampInMs));
    return timeStampInMs;
  
}


// ---------------------------------------------------------
// CMP4Parser::GetVideoFrameType
// Gets the type of video frame at given index
// (other items were commented in a header).
// ---------------------------------------------------------
//
TInt8 CMP4Parser::GetVideoFrameType(TInt aIndex)
{

    MP4Err error = 0;
    mp4_bool frameType;
    
    error = MP4ParseGetVideoFrameType(iMP4Handle, aIndex, &frameType);

    if (error != MP4_OK)
        return KErrGeneral;
 
	return TInt8(frameType);
}


// ---------------------------------------------------------
// CMP4Parser::GetVideoFrameProperties
// Gets frame properties
// (other items were commented in a header).
// ---------------------------------------------------------
//
TInt CMP4Parser::GetVideoFrameProperties(TFrameInfoParameters* aVideoFrameInfoArray,
										 TUint32 aStartIndex, TUint32 aSizeOfArray)
{	
	MP4Err error;
	error = MP4GetVideoFrameProperties(iMP4Handle,aStartIndex,aSizeOfArray,aVideoFrameInfoArray);

	if (error != MP4_OK)
		return KErrorCode;

	return KErrNone;
}


// ---------------------------------------------------------
// CMP4Parser::ParseAudioInfo
// Gets the frame information (frame size) for audio in the current clip
// (other items were commented in a header).
// ---------------------------------------------------------
//
TInt CMP4Parser::ParseAudioInfo(TInt& aAudioFrameSize)
{
	MP4Err error;
	mp4_u32 audioLength=0;
	mp4_u32 audioType=0;
	mp4_u32 audioFramesInSample=0;
	mp4_u32 audioTimeScale=0;
    mp4_u32 avgBitRate=0;
	mp4_double frameLength = 0.02; // 20 ms

	error = MP4ParseRequestAudioDescription(iMP4Handle, (mp4_u32 *)&audioLength, 
		&audioType, (mp4_u8*)&audioFramesInSample, (mp4_u32 *)&audioTimeScale, (mp4_u32*)&avgBitRate);
    aAudioFrameSize = ((mp4_u32)((mp4_double)avgBitRate*frameLength)>>3); 

	return error;
}

TInt CMP4Parser::GetMP4SpecificSize()
{
	MP4Err error = 0;
	mp4_u32 mp4Specific = 0;
	if ( iVideoType == MP4_TYPE_MPEG4_VIDEO )
	{
		error = MP4ParseReadVideoDecoderSpecificInfo( iMP4Handle, 0, 0, &mp4Specific );
		if ( error != MP4_OK && error != MP4_BUFFER_TOO_SMALL )
			return KErrGeneral;
	}
	return mp4Specific;
}

// ---------------------------------------------------------
// CMP4Parser::ReadAudioDecoderSpecificInfoL
// Gets the decoder specific info from the current file filled so that it can be composed to output file
// (other items were commented in a header).
// ---------------------------------------------------------
//
TInt CMP4Parser::ReadAudioDecoderSpecificInfoL(HBufC8*& aBytes, TInt aBufferSize)
{
	aBytes = HBufC8::NewL(aBufferSize);
	CleanupStack::PushL(aBytes);
	mp4_u8 *buffer = new (ELeave) mp4_u8[aBufferSize];
	mp4_u32 decspecinfosize = 0;
	
	MP4Err err = MP4ParseReadAudioDecoderSpecificInfo(
		iMP4Handle,
		buffer,
		aBufferSize,
		&decspecinfosize);
	if (err == MP4_OK)
	{
		for (TInt a = 0 ; a < (TInt)decspecinfosize ; a++)
		{
			aBytes->Des().Append(buffer[a]);
		}
	}
	else
	{
		delete[] buffer;
		buffer = 0;
		CleanupStack::PopAndDestroy(aBytes);
		aBytes = 0;
		User::Leave(KErrGeneral);
	}
	delete[] buffer;
	buffer = 0;
	CleanupStack::Pop(aBytes);
	return ETrue;
}

// ---------------------------------------------------------
// CMP4Parser::SetDefaultAudioDecoderSpecificInfoL
// Gets the default decoder specific info filled so that it can be composed to output file
// the default info is for a 16 KHz, mono LC type AAC only
// (other items were commented in a header).
// ---------------------------------------------------------
//
TInt CMP4Parser::SetDefaultAudioDecoderSpecificInfoL(HBufC8*& aBytes, TInt aBufferSize)
{
	aBytes = HBufC8::NewL(aBufferSize);
	CleanupStack::PushL(aBytes);
	
	const TUint8 frameDecSpecInfo[] = {0x14,0x08};	//the decoder specific 
	const TInt frameSize = 2;  //constant as maximum size of decoderspecific info is 2
	
		for (TInt a = 0 ; a < frameSize ; a++)
		{
			aBytes->Des().Append(frameDecSpecInfo[a]);
		}
	CleanupStack::Pop(aBytes);
	return ETrue;
}


TInt CMP4Parser::GetAudioBitrate(TInt& aBitrate)
{

    mp4_u32 length;
    mp4_u32 type;
    mp4_u8 framesPerSample;
    mp4_u32 timeScale;
    mp4_u32 averageBitrate;

    MP4Err error = 0;
    
    error = MP4ParseRequestAudioDescription(iMP4Handle, &length, 
        &type, &framesPerSample, &timeScale, &averageBitrate);

    if ( error != 0 )
        return KErrGeneral;

    aBitrate = averageBitrate;

    return KErrNone;


}

TInt CMP4Parser::GetVideoFrameRate(TReal& aFrameRate)
{

    mp4_u32 length;
    mp4_double frameRate; 
    mp4_u32 videoType;
    mp4_u32 width;
    mp4_u32 height;
    mp4_u32 timeScale;

    MP4Err error = 0;

    // get video description
	error = MP4ParseRequestVideoDescription(iMP4Handle, &length, &frameRate, 
        &videoType, &width, &height, &timeScale);		

    if ( error != 0 )
        return KErrGeneral;
    
    TReal temp = frameRate * 2.0;
    TInt temp2 = TInt(temp + 0.5);

    aFrameRate = temp2 / 2.0;

    return KErrNone;
}

TInt CMP4Parser::GetDecoderSpecificInfoSize()
{

    MPASSERT(iMP4Handle);

    mp4_u32 mp4Specific = 0;
        
    MP4Err error = MP4ParseReadVideoDecoderSpecificInfo( iMP4Handle, 0, 0, &mp4Specific );        
    
    if ( error != MP4_OK && error != MP4_BUFFER_TOO_SMALL )
        return KErrGeneral;
    
    return mp4Specific;
}

TInt CMP4Parser::ReadAVCDecoderSpecificInfo(TDes8& buf)
{

    mp4_u32 mp4Specific = 0;
    
    MP4Err error = MP4ParseReadVideoDecoderSpecificInfo( iMP4Handle, (mp4_u8*)(buf.Ptr()), 
				mp4_u32( buf.MaxLength() ), &mp4Specific );
        
        
    if (error != MP4_OK)
	    return KErrorCode;
    
    buf.SetLength(mp4Specific);
    
    return KErrNone;
    
}

TInt CMP4Parser::GetVideoDuration(TInt& aDurationInMs)
{

    mp4_u32 length;
    mp4_double frameRate; 
    mp4_u32 videoType;
    mp4_u32 width;
    mp4_u32 height;
    mp4_u32 timeScale;

    MP4Err error = 0;

    // get video description
	error = MP4ParseRequestVideoDescription(iMP4Handle, &length, &frameRate, 
        &videoType, &width, &height, &timeScale);		

    if ( error != MP4_OK )
        return KErrGeneral;
    
    aDurationInMs = length;

    return KErrNone;
    
}


// End of File
