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


//FC Based on Cmp4parser


// INCLUDE FILES
#include <f32file.h>
#include "mp4composer.h"
#include "vedvideosettings.h"
#include "vedavcedit.h"


// ASSERTIONS
//#define MPASSERT(x) __ASSERT_DEBUG(x, User::Panic(_L("CVideoProcessorImpl"), EInternalAssertionFailure))


// MACROS
//#define ?macro ?macro_def

#ifdef _DEBUG
#include <e32svr.h>
#define PRINT(x) RDebug::Print x;
#else
#define PRINT(x)
#endif

// LOCAL CONSTANTS AND MACROS

const TUint KFreeDiskSpaceCounter = 10;        // Interval when to find out real free disk space
const TUint KMaxComposeBufferSize = 512000;    // : Adjust buffer size dynamically

#ifdef _DEBUG
const TInt KLeaveCode = CComposer::EComposerFailure;
#else
const TInt KLeaveCode = KErrGeneral;
#endif

// ============================= LOCAL FUNCTIONS ===============================

// -----------------------------------------------------------------------------
// RateIndex: converts sample rate to rate index used in decoder config info
// -----------------------------------------------------------------------------
//
static TUint8 RateIndex(TInt aRate)
    {
    switch ( aRate )
        {
        case 96000:
            return 0x0;
        case 88200:
            return 0x1;
        case 64000:
            return 0x2;
        case 48000:
            return 0x3;
        case 44100:
            return 0x4;
        case 32000:
            return 0x5;
        case 24000:
            return 0x6;
        case 22050:
            return 0x7;
        case 16000:
            return 0x8;
        case 12000:
            return 0x9;
        case 11025:
            return 0xa;
        case 8000:
            return 0xb;
        case 7350:
            return 0xc;
        default:
            return 0xf;
        }
    }


// ================= MEMBER FUNCTIONS =======================

// C++ default constructor can NOT contain any code, that
// might leave.
//

CMP4Composer::CMP4Composer()
{
	iMP4Handle = 0;	
    iFreeDiskSpace = 0;
    iFreeDiskSpaceCounter = 0;
	iFirstWrite = ETrue;
    iFsOpened = EFalse;
}

// Two-phased constructor. 
CMP4Composer* CMP4Composer::NewL(const TDesC &aFileName, 
                                 CParser::TVideoFormat aVideoFormat, 
                                 CParser::TAudioFormat aAudioFormat,
                                 CVedAVCEdit *aAvcEdit)
{
	CMP4Composer *self = new (ELeave) CMP4Composer;
	CleanupStack::PushL(self);
	self->ConstructL(aFileName, aVideoFormat, aAudioFormat, aAvcEdit);
	CleanupStack::Pop();  // self 
	return self;
}

CMP4Composer* CMP4Composer::NewL(RFile* aFileHandle, 
                                 CParser::TVideoFormat aVideoFormat, 
                                 CParser::TAudioFormat aAudioFormat,
                                 CVedAVCEdit *aAvcEdit)
{
	CMP4Composer *self = new (ELeave) CMP4Composer;
	CleanupStack::PushL(self);
	self->ConstructL(aFileHandle, aVideoFormat, aAudioFormat, aAvcEdit);
	CleanupStack::Pop();  // self 
	return self;
}

// Symbian OS default constructor can leave.
void CMP4Composer::ConstructL(const TDesC &aFileName,
							  CParser::TVideoFormat aVideoFormat, 
                              CParser::TAudioFormat aAudioFormat,
                              CVedAVCEdit *aAvcEdit)
{
	TUint mediaType = 0;	
	
    iOutputMovieFileName = TFileName(aFileName);
    iFileHandle = NULL;

	iAvcEdit = aAvcEdit;
	
    // open MP4 library	
	
    TBuf<258> temp(aFileName);
    temp.ZeroTerminate();        
        
    MP4FileName name = reinterpret_cast<MP4FileName>( const_cast<TUint16*>(temp.Ptr()) );
    
    SetMediaOptions(aVideoFormat, aAudioFormat, mediaType);
    
    MP4Err error;
    
    // if the filename length is greater 0, then compose to file else buffer
    if(aFileName.Length() > 0)   
    {
        error = MP4ComposeOpen(&iMP4Handle, name, mediaType);		
    }
    else
    {								
        // initialize to compose to buffer
        iComposeBuffer = (TUint8*)HBufC::NewL( KMaxComposeBufferSize );
        iComposedSize = KMaxComposeBufferSize;
        error = MP4ComposeOpenToBuffer(&iMP4Handle, mediaType,(mp4_u8*)iComposeBuffer,&iComposedSize);
    }
    
    if ( error != MP4_OK )
        User::Leave(KLeaveCode);
    
    SetComposerOptionsL(aVideoFormat, aAudioFormat);
    
}

void CMP4Composer::ConstructL(RFile* aFileHandle,
							  CParser::TVideoFormat aVideoFormat, 
                              CParser::TAudioFormat aAudioFormat,
                              CVedAVCEdit *aAvcEdit)
{
	TUint mediaType = 0;	
	
	iOutputMovieFileName.Zero();
    iFileHandle = aFileHandle;

	iAvcEdit = aAvcEdit;
	    	        
    SetMediaOptions(aVideoFormat, aAudioFormat, mediaType);
    
    // open MP4 library	    
    MP4Err error;

    error = MP4ComposeOpenFileHandle(&iMP4Handle, aFileHandle, EDriveC, mediaType);

    if ( error != MP4_OK )
        User::Leave(KLeaveCode);
    
    SetComposerOptionsL(aVideoFormat, aAudioFormat);
    
}


void CMP4Composer::SetMediaOptions(CParser::TVideoFormat aVideoFormat, 
                                   CParser::TAudioFormat aAudioFormat,
                                   TUint& aMediaFlags)
                                 
{

	if ( (aVideoFormat == CParser::EVideoFormatH263Profile0Level10) ||  
		 (aVideoFormat == CParser::EVideoFormatH263Profile0Level45) )
	{
		aMediaFlags = MP4_TYPE_H263_PROFILE_0;
		iVideoType = MP4_TYPE_H263_PROFILE_0;    
	}
	
#ifdef VIDEOEDITORENGINE_AVC_EDITING
	else if (aVideoFormat == CParser::EVideoFormatAVCProfileBaseline)
	{
		aMediaFlags = MP4_TYPE_AVC_PROFILE_BASELINE;
		iVideoType = MP4_TYPE_AVC_PROFILE_BASELINE;    
	}
#endif
	else
	{
		aMediaFlags = MP4_TYPE_MPEG4_VIDEO;
		iVideoType = MP4_TYPE_MPEG4_VIDEO;
	}

    if ( aAudioFormat == CParser::EAudioFormatAMR )
	{
		aMediaFlags |= MP4_TYPE_AMR_NB;
		iAudioType = MP4_TYPE_AMR_NB;
	}

	else if ( aAudioFormat == CParser::EAudioFormatAAC )
	{
		aMediaFlags |= MP4_TYPE_MPEG4_AUDIO;   // added for AAC support. 
		iAudioType = MP4_TYPE_MPEG4_AUDIO;
	}
	
}

void CMP4Composer::SetComposerOptionsL(CParser::TVideoFormat aVideoFormat,
                                       CParser::TAudioFormat aAudioFormat)
{

    MP4Err error;

    TBool videoMpeg4OrAvc = ( aVideoFormat == CParser::EVideoFormatMPEG4 ||
                              aVideoFormat == CParser::EVideoFormatAVCProfileBaseline );

    mp4_u32 flags = 0;
    flags |= MP4_FLAG_LARGEFILEBUFFER;  // Note: What does this do when using RFile ?
	flags |= MP4_FLAG_METADATALAST;	
    
	// generate MP4 file format if MPEG-4/AVC video & AAC audio
	if ( (videoMpeg4OrAvc && aAudioFormat == CParser::EAudioFormatAAC) ||
		 (videoMpeg4OrAvc && aAudioFormat == CParser::EAudioFormatNone) ||
		 (aVideoFormat == CParser::EVideoFormatNone && aAudioFormat == CParser::EAudioFormatAAC) )
	{
		flags |= MP4_FLAG_GENERATE_MP4;
	}
    	 
    error = MP4ComposeSetFlags(iMP4Handle, flags);
    
    if (error != MP4_OK)
    	if (error == MP4_OUT_OF_MEMORY)
    		{
				User::LeaveNoMemory();
    		}
    	else
    		{
        User::Leave(KLeaveCode);
    		}

    // set buffer sizes; only composer buffer settings are effective for this instance
    error = MP4SetCustomFileBufferSizes(iMP4Handle, K3gpMp4ComposerWriteBufferSize, K3gpMp4ComposerNrOfWriteBuffers, K3gpMp4ParserReadBufferSize );

    if (error == MP4_OUT_OF_MEMORY)
    {
        User::Leave(KErrNoMemory);
    }
    else if ( error != MP4_OK )
    {
        User::Leave(KLeaveCode);
    }
}


// Destructor
CMP4Composer::~CMP4Composer()
{
    if (iMP4Handle)
    {
        MP4ComposeClose(iMP4Handle);
    }

	if(iComposeBuffer)			// added for Buffer support
	{
		User::Free(iComposeBuffer);
		iComposeBuffer=0;
	}

    iMP4Handle = 0;

    if (iFsOpened) 
    {        
        iFS.Close();
        iFsOpened = EFalse;        
    }    
	

}

// ---------------------------------------------------------
// CMP4Composer::WriteFrames
// Write the next frame(s) to file
// ---------------------------------------------------------
//
TInt CMP4Composer::WriteFrames(TDesC8& aSrcBuffer, TInt aFrameSize,
							   TInt aDuration, TInt aKeyFrame, 
							   TInt aNumberOfFrames, TInt aFrameType)
{
	MP4Err error = KErrNone;
	
	// get the parameters 
	TUint32	frameSize = aFrameSize;
	TUint32	duration = aDuration;
	mp4_bool keyframe = (mp4_bool)aKeyFrame; 
	TInt numberOfFrames = aNumberOfFrames; 
	TFrameType	frameType = (aFrameType>0 ? (aFrameType==1 ? EFrameTypeAudio : EFrameTypeVideo) 
        : EFrameTypeNone); 
    
    if (frameType == EFrameTypeVideo)
    {            
        error = MP4ComposeWriteVideoFrame(iMP4Handle, (mp4_u8*)(aSrcBuffer.Ptr()),
            frameSize, duration, keyframe);
        if ( error != MP4_OK )
            return KLeaveCode;		
    }
    
    else if (frameType == EFrameTypeAudio)
    {
        error = MP4ComposeWriteAudioFrames(iMP4Handle,(mp4_u8*)(aSrcBuffer.Ptr()),
            frameSize, numberOfFrames, duration);
        if ( error != MP4_OK )
            return KLeaveCode;		
    }
    else 
        User::Panic(_L("CMovieProcessorImpl"), EComposerFailure);
    
    return KErrNone;
		
}


TInt CMP4Composer::WriteFrames(TDesC8& aSrcBuffer, TInt aFrameSize,
							   TInt aDuration, TInt aKeyFrame, 
							   TInt aNumberOfFrames, TInt aFrameType,
                               TInt& aMP4Size, TBool aModeChanged,
							   TBool aFirstFrameOfClip, TInt aMode, TBool /*aFromEncoder*/)
{
	MP4Err error = KErrNone;
	
	// get the parameters 
	TUint32	frameSize = aFrameSize;
	TUint32	duration = aDuration;
	mp4_bool keyframe = ( aKeyFrame ) ? ETrue : EFalse;
	TInt numberOfFrames = aNumberOfFrames; 
	TFrameType	frameType = (aFrameType > 0 ? (aFrameType == 1 ? EFrameTypeAudio : EFrameTypeVideo) 
		: EFrameTypeNone); 
	TUint8* dataPtr = (TUint8*)(aSrcBuffer.Ptr());
	TInt tmpSize = 0;
	
	// call this only for the first frame of every clip 
	if ( aFirstFrameOfClip && (iVideoType == MP4_TYPE_MPEG4_VIDEO) && (aMP4Size == 0)) 
	{
		if ((tmpSize = GetMp4SpecificSize(aSrcBuffer,aModeChanged,aMode)) != 0)
			aMP4Size = tmpSize;	//This will be the new Mp4Size as it will be consider and since by reference it is maintained
	}
	
	if (frameType == EFrameTypeVideo)
	{            
		if ( iVideoType == MP4_TYPE_MPEG4_VIDEO )
		{
			if ( iFirstWrite )
			{
				// VOS
				error = MP4ComposeWriteVideoDecoderSpecificInfo( iMP4Handle,
					(mp4_u8*)(dataPtr/*aSrcBuffer.Ptr()*/), aMP4Size );
				iFirstWrite = EFalse;
				
				error = MP4ComposeWriteVideoFrame(iMP4Handle, (mp4_u8*)(dataPtr/*aSrcBuffer.Ptr()*/ + aMP4Size),
					( frameSize - aMP4Size ), duration, keyframe);
			}
			else
			{
				// for MPEG4 - check the first 32 bits to make sure we don't 
				// have VOS pre-pended to VOP data
				if (dataPtr[0] == 0x00 && dataPtr[1] == 0x00 && dataPtr[2] == 0x01 && dataPtr[3] == 0xb0) 
				{	// since intermediate Vos set to proper value
					// Not Short Header may have User Data space problem with PSC
					if ((tmpSize = GetMp4SpecificSize(aSrcBuffer,aModeChanged,aMode)) != 0)
						aMP4Size = tmpSize;	//This will be the new Mp4Size as it will be considered		
					dataPtr += aMP4Size;
					frameSize -= aMP4Size;
				}

                if (frameSize == 0)
                    return KErrWrite;

				error = MP4ComposeWriteVideoFrame(iMP4Handle, (mp4_u8*)(dataPtr),
					frameSize, duration, keyframe);
			}
		} 
		
#ifdef VIDEOEDITORENGINE_AVC_EDITING
		else if ( iVideoType == MP4_TYPE_AVC_PROFILE_BASELINE )
		{
		
			if ( iFirstWrite || aFirstFrameOfClip)
			{
				if(iFirstWrite)
				{				    
					iFrameNumber = 0;
					iFirstWrite = EFalse;
				}
				
				aMP4Size = 0;
				error = MP4ComposeWriteVideoFrame(iMP4Handle, (mp4_u8*)(dataPtr/*aSrcBuffer.Ptr()*/ + aMP4Size),
					( frameSize - aMP4Size ), duration, keyframe);
			}
			else
			{
				error = MP4ComposeWriteVideoFrame(iMP4Handle, (mp4_u8*)(dataPtr),
					frameSize, duration, keyframe);
			}
			
			iFrameNumber++;
		}
#endif
		
		else // H.263
		{
			error = MP4ComposeWriteVideoFrame(iMP4Handle, (mp4_u8*)(dataPtr/*aSrcBuffer.Ptr()*/),
				frameSize, duration, keyframe);
				
		}
		if ( error != MP4_OK )
			return KLeaveCode;		
	}
	else if (frameType == EFrameTypeAudio)
	{
		error = MP4ComposeWriteAudioFrames(iMP4Handle,(mp4_u8*)(dataPtr/*aSrcBuffer.Ptr()*/),
			frameSize, numberOfFrames, duration);
		if ( error != MP4_OK )
			return KLeaveCode;          
	}
	else 
		User::Panic(_L("CMovieProcessorImpl"), EComposerFailure);
	
	return KErrNone;
}



// ---------------------------------------------------------
// CMP4Composer::ComposeHeaderL
// Get relevant stream parameters by calling mp4 library functions
// (other items were commented in a header).
// ---------------------------------------------------------
//


void CMP4Composer::ComposeHeaderL(CComposer::TStreamParameters& aStreamParameters, TInt aOutputVideoTimeScale, 
                                  TInt aOutputAudioTimeScale, TInt aAudioFramesInSample)
{
    MP4Err iError=KErrNone;	
	
	TInt width = aStreamParameters.iVideoWidth;
	TInt height = aStreamParameters.iVideoHeight;
	TInt avgbitrate = aStreamParameters.iStreamBitrate;
	TInt maxbitrate = avgbitrate; 
	TInt audioFramesPerSample = aAudioFramesInSample;//aStreamParameters.iAudioFramesInSample;       
	TInt audioTimescale = aOutputAudioTimeScale;    
	TInt modeSet=0x8180; 

	if(iAudioType == MP4_TYPE_MPEG4_AUDIO)		// added for AAC support 
	{	
		// reset for AAC audio according to the code sent for AAC support.
		audioFramesPerSample = 0;
		modeSet = 0;
	}

	// set this to first clip's time scale 
	TInt timescale = aOutputVideoTimeScale;
        
    iError = MP4ComposeAddAudioDescription(iMP4Handle,(mp4_u32)audioTimescale,
        (mp4_u8)audioFramesPerSample,(mp4_u16)modeSet); 
    if (iError != MP4_OK)
        User::Leave(KLeaveCode);
		
    // write video description
    iError = MP4ComposeAddVideoDescription(iMP4Handle,(mp4_u32)timescale,
        (mp4_u16)width,(mp4_u16)height,(mp4_u32)maxbitrate,(mp4_u32)avgbitrate);
    
    if (iError != MP4_OK)
        User::Leave(KLeaveCode);

	if ( aStreamParameters.iVideoFormat == EVideoFormatH263Profile0Level10 )
	{
		TVideoClipProperties prop;
		prop.iH263Level = 10;
		MP4ComposeSetVideoClipProperties(iMP4Handle, prop);
	}

	else if ( aStreamParameters.iVideoFormat == EVideoFormatH263Profile0Level45 )
	{
		TVideoClipProperties prop;
		prop.iH263Level = 45;
		MP4ComposeSetVideoClipProperties(iMP4Handle, prop);
	}
	
	if (!iFsOpened) // Check if file server is open already
        {        
            User::LeaveIfError(iFS.Connect());        
            iFsOpened = ETrue;
        }

    if (iFileHandle == 0)
    {
        // get target drive number
        TParse fp;
        User::LeaveIfError(iFS.Parse(iOutputMovieFileName, fp));
        TPtrC driveletter = fp.Drive();
        TChar drl = driveletter[0];
        User::LeaveIfError(RFs::CharToDrive(drl, iDriveNumber));
    } 
    else
    {
        // get target drive number
        TDriveInfo info;
        TInt error = iFileHandle->Drive(iDriveNumber, info);
    }
}

// -----------------------------------------------------------------------------
// CMP4Composer::DriveFreeSpaceL
// Calculate free space on a drive in bytes.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt64 CMP4Composer::DriveFreeSpaceL()
{
    TVolumeInfo volumeinfo;

    if (iFreeDiskSpaceCounter % KFreeDiskSpaceCounter == 0)
        {
        User::LeaveIfError(iFS.Volume(volumeinfo, iDriveNumber));
        iFreeDiskSpace = volumeinfo.iFree;
        }

    iFreeDiskSpaceCounter++;
    return iFreeDiskSpace;

}

// -----------------------------------------------------------------------------
// CMP4Composer::GetComposedBuffer New Function added for Buffer support 
// Gets the Composed buffer from the composer 
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TUint8* CMP4Composer::GetComposedBuffer()
{
	return iComposeBuffer;
}

// -----------------------------------------------------------------------------
// CMP4Composer::GetComposedBufferSize New Function added for Buffer support 
// Gets the Composed buffer size from the composer 
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TUint CMP4Composer::GetComposedBufferSize()
{
	return iComposedSize;
}

// ---------------------------------------------------------
// CMP4Composer::Close
// Closes the composer instance & creates the output 3gp file 
// (other items were commented in a header).
// ---------------------------------------------------------
//
TInt CMP4Composer::Close()
{

#ifdef VIDEOEDITORENGINE_AVC_EDITING

    if (iVideoType == MP4_TYPE_AVC_PROFILE_BASELINE)
    {
        HBufC8* AVCDCR = NULL;
    	TInt error;

    	TRAP(error, AVCDCR = (HBufC8*) HBufC8::NewL(16384));
    	if (error != KErrNone)
    	    return error;

        TPtr8 ptr = AVCDCR->Des();    
        
        // Construct AVC Decoder Configuration Record from the SPS / PPS sets
        TRAP(error, iAvcEdit->ConstructAVCDecoderConfigurationRecordL(ptr));
        if (error != KErrNone)
        {
            delete AVCDCR;        
    	    return error;
        }                       

    	// Pass the AVC Decoder Configuration Record to the 3GPMP4 library
    	MP4Err mp4Error = MP4ComposeWriteVideoDecoderSpecificInfo( iMP4Handle,
    											    (mp4_u8*)(ptr.Ptr()), ptr.Length());    	    	

        delete AVCDCR;
        
        if (mp4Error != MP4_OK)
            return EComposerFailure;
    }
    
#endif

    MP4Err error = MP4ComposeClose(iMP4Handle);

    iMP4Handle = 0;

    if (error != MP4_OK)
        return EComposerFailure;

    if (iFsOpened) 
    {        
        iFS.Close();
        iFsOpened = EFalse;        
    }
    
    return KErrNone;
}


// -----------------------------------------------------------------------------
// CMP4Composer::GetMp4SpecificSize
// Gets the length of MPEG-4 decoder specific info in aSrcBuf
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMP4Composer::GetMp4SpecificSize(TDesC8& aSrcBuf,TBool aModeChange,TInt aStreamMode)
{
	TUint8* dtPtr = (TUint8*)(aSrcBuf.Ptr());
	TInt mp4size =0;
	TInt bufSize;
	TInt flag;

	if (!aModeChange && (aStreamMode == 2)) //2 indicates short header in VedCommon
	{
		bufSize = aSrcBuf.Size()-3;
		for(TInt i=0;i<bufSize;i++)
		{
			flag = (dtPtr[i+2]>>2)<<2;
			if (dtPtr[i] == 0x00 && dtPtr[i+1] == 0x00 && flag == 0x80)	// user data ????????????
			{
				mp4size=i;
				break;
			}
		}
	}
	else
	{
		bufSize = aSrcBuf.Size()-4;
		for(TInt i=0;i<bufSize;i++)
		{
			if ((dtPtr[i] == 0x00 && dtPtr[i+1] == 0x00 && dtPtr[i+2] == 0x01 && dtPtr[i+3] == 0xb3) ||
				(dtPtr[i] == 0x00 && dtPtr[i+1] == 0x00 && dtPtr[i+2] == 0x01 && dtPtr[i+3] == 0xb6))
			{
				mp4size=i;
				break;
			}
		}
	}

	// MP4 specific size will be zero, if there is a GOV or VOP header in the
	// very beginning of the buffer	
	
	return mp4size;
}


// ---------------------------------------------------------
// CMP4Composer::WriteAudioSpecificInfo
// Writes the Audio decoder Specific info required in case of AAC
// Decoder specific info is provided externally
// (other items were commented in a header).
// decoder info is provided externally
// ---------------------------------------------------------
//
// added to Support AAC audio files	
TInt CMP4Composer::WriteAudioSpecificInfo(HBufC8*& aSrcBuf)
{
	TInt error = KErrNone;
	mp4_u8* aSrcB = const_cast<mp4_u8*>(aSrcBuf->Ptr());
	error = MP4ComposeWriteAudioDecoderSpecificInfo(iMP4Handle, aSrcB, aSrcBuf->Size());
	
	if (error != MP4_OK)
		return KErrGeneral;

	return KErrNone;
}

// ---------------------------------------------------------
// CMP4Composer::WriteAudioSpecificInfo
// Writes the Audio decoder Specific info required in case of AAC
// Decoder specific info is derived from samplerate and # of channels
// (other items were commented in a header).
// ---------------------------------------------------------
//
// added to Support AAC audio files	
TInt CMP4Composer::WriteAudioSpecificInfo(TInt aSampleRate, TInt aNumChannels)
{
	TInt error = KErrNone;
	
    TUint8 data[2];

    data[0] = 2<<3; // AAC-LC
    TUint8 rate = RateIndex(aSampleRate);
    data[0] |= rate>>1;
    data[1] = TUint8(rate<<7);
    data[1] |= TUint8(aNumChannels<<3);

    error = MP4ComposeWriteAudioDecoderSpecificInfo(iMP4Handle, data, 2);
	
	if (error != MP4_OK)
		return KErrGeneral;

	return KErrNone;
}


// -----------------------------------------------------------------------------
// CMP4Composer::GetAVCDecoderSpecificInfoSize
// Gets the length of AVC decoder specific info in aSrcBuf
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMP4Composer::GetAVCDecoderSpecificInfoSize(TDesC8& aSrcBuf)
{
	TUint8* srcPtr = (TUint8*)(aSrcBuf.Ptr());
	TInt skip = 0;
//	TInt error = KErrNone;

	// skip 4 bytes for 
	// configVersion, profile, profile compatibility and Level
	skip += 4;
	
	// skip 1 bytes for lengthSizeMinusOne
	skip += 1;
	
	// skip 1 bytes for num of seq Param sets
	TInt numOfSSP = 0x1F & srcPtr[skip];
	skip += 1;
	
	for (TInt i = 0; i < numOfSSP; i++)
    {
      	TInt sspSize = srcPtr[skip]*256 + srcPtr[skip+1];      	
       	skip += 2;
        skip += sspSize;
    }

	TInt numOfPSP = srcPtr[skip];
	skip += 1;

	for (TInt i = 0; i < numOfPSP; i++)
    {
      	TInt pspSize = srcPtr[skip]*256 + srcPtr[skip+1];      	
       	skip += 2;
        skip += pspSize;
    }

	return skip;
}



// End of File
